// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codegeex4client.h"
#include "codegeex4constants.h"
#include "codegeex4icons.h"
#include "codegeex4projectpanel.h"
#include "codegeex4settings.h"
#include "codegeex4suggestion.h"
#include "codegeex4tr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/statusbarmanager.h>

#include <extensionsystem/iplugin.h>

#include <languageclient/languageclientmanager.h>

#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditor.h>

#include <QAction>
#include <QToolButton>
#include <QPointer>

#include <QApplication>
#include <QTranslator>
#include <QTimeZone>

using namespace Utils;
using namespace Core;
using namespace ProjectExplorer;

namespace CodeGeeX4::Internal {

enum Direction { Previous, Next };

static void cycleSuggestion(TextEditor::TextEditorWidget *editor, Direction direction)
{
    QTextBlock block = editor->textCursor().block();
    if (auto suggestion = dynamic_cast<CodeGeeX4Suggestion *>(
            TextEditor::TextDocumentLayout::suggestion(block))) {
        int index = suggestion->currentCompletion();
        if (direction == Previous)
            --index;
        else
            ++index;
        if (index < 0)
            index = suggestion->completions().count() - 1;
        else if (index >= suggestion->completions().count())
            index = 0;
        suggestion->reset();
        editor->insertSuggestion(std::make_unique<CodeGeeX4Suggestion>(suggestion->completions(),
                                                                     editor->document(),
                                                                     index));
    }
}

class CodeGeeX4Plugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CodeGeeX4.json")

public:
    void initialize() final
    {
        QTranslator *translator=new QTranslator(QApplication::instance());
        QTimeZone localPosition = QDateTime::currentDateTime().timeZone();
        if(QLocale::Country::China==localPosition.country()){
            if(translator->load("CodeGeeX4_zh_CN",QApplication::applicationDirPath()+"/../share/qtcreator/translations")){
                QApplication::installTranslator(translator);
            }
        }
        ActionBuilder requestAction(this,  Constants::CODEGEEX4_REQUEST_SUGGESTION);
        requestAction.setText(Tr::tr("Request CodeGeeX4 Suggestion"));
        requestAction.setToolTip(Tr::tr(
            "Request CodeGeeX4 suggestion at the current editor's cursor position."));
        requestAction.addOnTriggered(this, [this] {
            if (auto editor = TextEditor::TextEditorWidget::currentTextEditorWidget()) {
                if (m_client && m_client->reachable())
                    m_client->requestCompletions(editor);
            }
        });

        ActionBuilder nextSuggestionAction(this, Constants::CODEGEEX4_NEXT_SUGGESTION);
        nextSuggestionAction.setText(Tr::tr("Show Next CodeGeeX4 Suggestion"));
        nextSuggestionAction.setToolTip(Tr::tr(
            "Cycles through the received CodeGeeX4 Suggestions showing the next available Suggestion."));
        nextSuggestionAction.addOnTriggered(this, [] {
            if (auto editor = TextEditor::TextEditorWidget::currentTextEditorWidget())
                cycleSuggestion(editor, Next);
        });

        ActionBuilder previousSuggestionAction(this, Constants::CODEGEEX4_PREVIOUS_SUGGESTION);
        previousSuggestionAction.setText(Tr::tr("Show Previous CodeGeeX4 Suggestion"));
        previousSuggestionAction.setToolTip(Tr::tr("Cycles through the received CodeGeeX4 Suggestions "
                                                   "showing the previous available Suggestion."));
        previousSuggestionAction.addOnTriggered(this, [] {
            if (auto editor = TextEditor::TextEditorWidget::currentTextEditorWidget())
                cycleSuggestion(editor, Previous);
        });

        ActionBuilder disableAction(this, Constants::CODEGEEX4_DISABLE);
        disableAction.setText(Tr::tr("Disable CodeGeeX4"));
        disableAction.setToolTip(Tr::tr("Disable CodeGeeX4."));
        disableAction.addOnTriggered(this, [] {
            settings().enableCodeGeeX4.setValue(true);
            settings().apply();
        });

        ActionBuilder enableAction(this, Constants::CODEGEEX4_ENABLE);
        enableAction.setText(Tr::tr("Enable CodeGeeX4"));
        enableAction.setToolTip(Tr::tr("Enable CodeGeeX4."));
        enableAction.addOnTriggered(this, [] {
            settings().enableCodeGeeX4.setValue(false);
            settings().apply();
        });

        ActionBuilder toggleAction(this, Constants::CODEGEEX4_TOGGLE);
        toggleAction.setText(Tr::tr("Toggle CodeGeeX4"));
        toggleAction.setCheckable(true);
        toggleAction.setChecked(settings().enableCodeGeeX4());
        toggleAction.setIcon(CODEGEEX4_ICON.icon());
        toggleAction.addOnTriggered(this, [](bool checked) {
            settings().enableCodeGeeX4.setValue(checked);
            settings().apply();
        });

        QAction *toggleAct = toggleAction.contextAction();
        QAction *requestAct = requestAction.contextAction();
        auto updateActions = [toggleAct, requestAct] {
            const bool enabled = settings().enableCodeGeeX4();
            toggleAct->setToolTip(enabled ? Tr::tr("Disable CodeGeeX4.") : Tr::tr("Enable CodeGeeX4."));
            toggleAct->setChecked(enabled);
            requestAct->setEnabled(enabled);
        };

        connect(&settings().enableCodeGeeX4, &BaseAspect::changed, this, updateActions);

        updateActions();

        auto toggleButton = new QToolButton;
        toggleButton->setDefaultAction(toggleAction.contextAction());
        StatusBarManager::addStatusBarWidget(toggleButton, StatusBarManager::RightCorner);

        setupCodeGeeX4ProjectPanel();
    }

    bool delayedInitialize() final
    {
        restartClient();

        connect(&settings(), &AspectContainer::applied, this, &CodeGeeX4Plugin::restartClient);

        return true;
    }

    void restartClient()
    {
        LanguageClient::LanguageClientManager::shutdownClient(m_client);

        m_client = new CodeGeeX4Client();
    }

    ShutdownFlag aboutToShutdown() final
    {
        if (!m_client)
            return SynchronousShutdown;
        connect(m_client, &QObject::destroyed, this, &IPlugin::asynchronousShutdownFinished);
        return AsynchronousShutdown;
    }

private:
    QPointer<CodeGeeX4Client> m_client;
};

} // CodeGeeX4::Internal

#include "codegeex4plugin.moc"
