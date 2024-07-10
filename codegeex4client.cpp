﻿// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "codegeex4client.h"
#include "codegeex4settings.h"
#include "codegeex4suggestion.h"
#include "codegeex4tr.h"
#include "codegeex4clientinterface.h"

#include <languageclient/languageclientinterface.h>
#include <languageclient/languageclientmanager.h>
#include <languageclient/languageclientsettings.h>
#include <languageserverprotocol/lsptypes.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/projectmanager.h>

#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditor.h>

#include <utils/checkablemessagebox.h>
#include <utils/filepath.h>
#include <utils/passworddialog.h>

#include <QGuiApplication>
#include <QInputDialog>
#include <QLoggingCategory>
#include <QTimer>
#include <QToolButton>

using namespace LanguageServerProtocol;
using namespace TextEditor;
using namespace Utils;
using namespace ProjectExplorer;
using namespace Core;

Q_LOGGING_CATEGORY(codegeex4ClientLog, "qtc.codegeex4.client", QtWarningMsg)

namespace CodeGeeX4::Internal {

CodeGeeX4Client::CodeGeeX4Client()
    : LanguageClient::Client(new CodeGeeX4ClientInterface())
{
    setName("CodeGeeX4");
    LanguageClient::LanguageFilter langFilter;

    langFilter.filePattern = {"*"};

    setSupportedLanguage(langFilter);

    // registerCustomMethod("LogMessage", [this](const LanguageServerProtocol::JsonRpcMessage &message) {
    //     QString msg = message.toJsonObject().value("params").toObject().value("message").toString();
    //     qCDebug(codegeex4ClientLog) << message.toJsonObject()
    //                                      .value("params")
    //                                      .toObject()
    //                                      .value("message")
    //                                      .toString();

    //     if (msg.contains("Socket Connect returned status code,407")) {
    //         qCWarning(codegeex4ClientLog) << "Proxy authentication required";
    //         QMetaObject::invokeMethod(this,
    //                                   &CodeGeeX4Client::proxyAuthenticationFailed,
    //                                   Qt::QueuedConnection);
    //     }
    // });

    start();

    auto openDoc = [this](IDocument *document) {
        if (auto *textDocument = qobject_cast<TextDocument *>(document))
            openDocument(textDocument);
    };

    connect(EditorManager::instance(), &EditorManager::documentOpened, this, openDoc);
    connect(EditorManager::instance(),
            &EditorManager::documentClosed,
            this,
            [this](IDocument *document) {
                if (auto textDocument = qobject_cast<TextDocument *>(document))
                    closeDocument(textDocument);
            });

    //connect(this, &LanguageClient::Client::initialized, this, &CodeGeeX4Client::requestSetEditorInfo);

    for (IDocument *doc : DocumentModel::openedDocuments())
        openDoc(doc);
}

CodeGeeX4Client::~CodeGeeX4Client()
{
    for (IEditor *editor : DocumentModel::editorsForOpenedDocuments()) {
        if (auto textEditor = qobject_cast<BaseTextEditor *>(editor))
            textEditor->editorWidget()->removeHoverHandler(&m_hoverHandler);
    }
}

void CodeGeeX4Client::openDocument(TextDocument *document)
{
    auto project = ProjectManager::projectForFile(document->filePath());
    if (!isEnabled(project))
        return;

    Client::openDocument(document);
    connect(document,
            &TextDocument::contentsChangedWithPosition,
            this,
            [this, document](int position, int charsRemoved, int charsAdded) {
                Q_UNUSED(charsRemoved)
                if (!settings().autoComplete())
                    return;

                auto project = ProjectManager::projectForFile(document->filePath());
                if (!isEnabled(project))
                    return;

                auto textEditor = BaseTextEditor::currentTextEditor();
                if (!textEditor || textEditor->document() != document)
                    return;
                TextEditorWidget *widget = textEditor->editorWidget();
                if (widget->isReadOnly() || widget->multiTextCursor().hasMultipleCursors())
                    return;
                const int cursorPosition = widget->textCursor().position();
                if (cursorPosition < position || cursorPosition > position + charsAdded)
                    return;
                scheduleRequest(widget);
            });
}

void CodeGeeX4Client::scheduleRequest(TextEditorWidget *editor)
{
    cancelRunningRequest(editor);

    auto it = m_scheduledRequests.find(editor);
    if (it == m_scheduledRequests.end()) {
        auto timer = new QTimer(this);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, [this, editor]() {
            if (m_scheduledRequests[editor].cursorPosition == editor->textCursor().position())
                requestCompletions(editor);
        });
        connect(editor, &TextEditorWidget::destroyed, this, [this, editor]() {
            delete m_scheduledRequests.take(editor).timer;
            cancelRunningRequest(editor);
        });
        connect(editor, &TextEditorWidget::cursorPositionChanged, this, [this, editor] {
            cancelRunningRequest(editor);
        });
        it = m_scheduledRequests.insert(editor, {editor->textCursor().position(), timer});
    } else {
        it->cursorPosition = editor->textCursor().position();
    }
    it->timer->start(500);
}

void CodeGeeX4Client::requestCompletions(TextEditorWidget *editor)
{
    auto project = ProjectManager::projectForFile(editor->textDocument()->filePath());

    if (!isEnabled(project))
        return;

    MultiTextCursor cursor = editor->multiTextCursor();
    if (cursor.hasMultipleCursors() || cursor.hasSelection() || editor->suggestionVisible())
        return;

    const FilePath filePath = editor->textDocument()->filePath();
    GetCompletionRequest request{
        {TextDocumentIdentifier(hostPathToServerUri(filePath)),
         documentVersion(filePath),
         Position(cursor.mainCursor()),editor->textDocument()->plainText(),editor->position()}};
    request.setResponseCallback([this, editor = QPointer<TextEditorWidget>(editor)](
                                    const GetCompletionRequest::Response &response) {
        QTC_ASSERT(editor, return);
        handleCompletions(response, editor);
    });
    m_runningRequests[editor] = request;
    sendMessage(request);
}

void CodeGeeX4Client::handleCompletions(const GetCompletionRequest::Response &response,
                                      TextEditorWidget *editor)
{
    if (response.error())
        log(*response.error());

    int requestPosition = -1;
    if (const auto requestParams = m_runningRequests.take(editor).params())
        requestPosition = requestParams->position().toPositionInDocument(editor->document());

    const MultiTextCursor cursors = editor->multiTextCursor();
    if (cursors.hasMultipleCursors())
        return;

    if (cursors.hasSelection() || cursors.mainCursor().position() != requestPosition)
        return;

    if (const std::optional<GetCompletionResponse> result = response.result()) {
        auto isValidCompletion = [](const Completion &completion) {
            return completion.isValid() && !completion.text().trimmed().isEmpty();
        };
        QList<Completion> completions = Utils::filtered(result->completions().toListOrEmpty(),
                                                              isValidCompletion);

        // remove trailing whitespaces from the end of the completions
        for (Completion &completion : completions) {
            const LanguageServerProtocol::Range range = completion.range();
            if (range.start().line() != range.end().line())
                continue; // do not remove trailing whitespaces for multi-line replacements

            const QString completionText = completion.text();
            const int end = int(completionText.size()) - 1; // empty strings have been removed above
            int delta = 0;
            while (delta <= end && completionText[end - delta].isSpace())
                ++delta;

            if (delta > 0)
                completion.setText(completionText.chopped(delta));
        }
        if (completions.isEmpty())
            return;
        editor->insertSuggestion(
            std::make_unique<CodeGeeX4Suggestion>(completions, editor->document()));
        editor->addHoverHandler(&m_hoverHandler);
    }
}

void CodeGeeX4Client::cancelRunningRequest(TextEditor::TextEditorWidget *editor)
{
    const auto it = m_runningRequests.constFind(editor);
    if (it == m_runningRequests.constEnd())
        return;
    cancelRequest(it->id());
    m_runningRequests.erase(it);
}

// static QString currentProxyPassword;

// void CodeGeeX4Client::requestSetEditorInfo()
// {
//     if (settings().saveProxyPassword())
//         currentProxyPassword = settings().proxyPassword();

//     const EditorInfo editorInfo{QCoreApplication::applicationVersion(),
//                                 QGuiApplication::applicationDisplayName()};
//     const EditorPluginInfo editorPluginInfo{QCoreApplication::applicationVersion(),
//                                             "Qt Creator CodeGeeX4 plugin"};

//     SetEditorInfoParams params(editorInfo, editorPluginInfo);

//     if (settings().useProxy()) {
//         params.setNetworkProxy(
//             CodeGeeX4::NetworkProxy{settings().proxyHost(),
//                                   static_cast<int>(settings().proxyPort()),
//                                   settings().proxyUser(),
//                                   currentProxyPassword,
//                                   settings().proxyRejectUnauthorized()});
//     }

//     SetEditorInfoRequest request(params);
//     sendMessage(request);
// }

// void CodeGeeX4Client::requestCheckStatus(
//     bool localChecksOnly, std::function<void(const CheckStatusRequest::Response &response)> callback)
// {
//     CheckStatusRequest request{localChecksOnly};
//     request.setResponseCallback(callback);

//     sendMessage(request);
// }

// void CodeGeeX4Client::requestSignOut(
//     std::function<void(const SignOutRequest::Response &response)> callback)
// {
//     SignOutRequest request;
//     request.setResponseCallback(callback);

//     sendMessage(request);
// }

// void CodeGeeX4Client::requestSignInInitiate(
//     std::function<void(const SignInInitiateRequest::Response &response)> callback)
// {
//     SignInInitiateRequest request;
//     request.setResponseCallback(callback);

//     sendMessage(request);
// }

// void CodeGeeX4Client::requestSignInConfirm(
//     const QString &userCode,
//     std::function<void(const SignInConfirmRequest::Response &response)> callback)
// {
//     SignInConfirmRequest request(userCode);
//     request.setResponseCallback(callback);

//     sendMessage(request);
// }

bool CodeGeeX4Client::canOpenProject(Project *project)
{
    return isEnabled(project);
}

bool CodeGeeX4Client::isEnabled(Project *project)
{
    if (!project)
        return settings().enableCodeGeeX4();

    CodeGeeX4ProjectSettings settings(project);
    return settings.isEnabled();
}

// void CodeGeeX4Client::proxyAuthenticationFailed()
// {
//     static bool doNotAskAgain = false;

//     if (m_isAskingForPassword || !settings().enableCodeGeeX4())
//         return;

//     m_isAskingForPassword = true;

//     auto answer = PasswordDialog::getUserAndPassword(
//         Tr::tr("CodeGeeX4"),
//         Tr::tr("Proxy username and password required:"),
//         Tr::tr("Do not ask again. This will disable CodeGeeX4 for now."),
//         settings().proxyUser(),
//         &doNotAskAgain,
//         Core::ICore::dialogParent());

//     if (answer) {
//         settings().proxyUser.setValue(answer->first);
//         currentProxyPassword = answer->second;
//     } else {
//         settings().enableCodeGeeX4.setValue(false);
//     }

//     if (settings().saveProxyPassword())
//         settings().proxyPassword.setValue(currentProxyPassword);

//     settings().apply();

//     m_isAskingForPassword = false;
// }

} // namespace CodeGeeX4::Internal
