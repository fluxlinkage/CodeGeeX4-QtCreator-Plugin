// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "codegeex4hoverhandler.h"
#include "requests/getcompletions.h"

#include <languageclient/client.h>

#include <utils/filepath.h>

#include <QHash>
#include <QTemporaryDir>

namespace CodeGeeX4::Internal {

class CodeGeeX4Client : public LanguageClient::Client
{
public:
    CodeGeeX4Client();
    ~CodeGeeX4Client() override;

    void openDocument(TextEditor::TextDocument *document) override;

    void scheduleRequest(TextEditor::TextEditorWidget *editor);
    void requestCompletions(TextEditor::TextEditorWidget *editor);
    void handleCompletions(const GetCompletionRequest::Response &response,
                           TextEditor::TextEditorWidget *editor);
    void cancelRunningRequest(TextEditor::TextEditorWidget *editor);

    // void requestCheckStatus(
    //     bool localChecksOnly,
    //     std::function<void(const CheckStatusRequest::Response &response)> callback);

    bool canOpenProject(ProjectExplorer::Project *project) override;

    bool isEnabled(ProjectExplorer::Project *project);

private:
    //void requestSetEditorInfo();

    QHash<TextEditor::TextEditorWidget *, GetCompletionRequest> m_runningRequests;
    struct ScheduleData
    {
        int cursorPosition = -1;
        QTimer *timer = nullptr;
    };
    QHash<TextEditor::TextEditorWidget *, ScheduleData> m_scheduledRequests;
    CodeGeeX4HoverHandler m_hoverHandler;
};

} // namespace CodeGeeX4::Internal
