﻿// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codegeex4suggestion.h"

#include <texteditor/texteditor.h>

#include <utils/stringutils.h>

using namespace Utils;
using namespace TextEditor;
using namespace LanguageServerProtocol;

namespace CodeGeeX4::Internal {

CodeGeeX4Suggestion::CodeGeeX4Suggestion(const QList<Completion> &completions,
                                     QTextDocument *origin,
                                     int currentCompletion)
    : m_completions(completions)
    , m_currentCompletion(currentCompletion)
{
    const Completion completion = completions.value(currentCompletion);
    const Position start = completion.range().start();
    const Position end = completion.range().end();
    QString text = start.toTextCursor(origin).block().text();
    int length = text.length() - start.character();
    if (start.line() == end.line())
        length = end.character() - start.character();
    text.replace(start.character(), length, completion.text());
    document()->setPlainText(text);
    m_start = completion.position().toTextCursor(origin);
    m_start.setKeepPositionOnInsert(true);
    setCurrentPosition(m_start.position());
}

bool CodeGeeX4Suggestion::apply()
{
    reset();
    const Completion completion = m_completions.value(m_currentCompletion);
    QTextCursor cursor = completion.range().toSelection(m_start.document());
    cursor.insertText(completion.text());
    return true;
}

bool CodeGeeX4Suggestion::applyLine()
{
    reset();
    const Completion completion = m_completions.value(m_currentCompletion);
    QTextCursor cursor = completion.range().toSelection(m_start.document());
    QString str=completion.text();
    int ind=str.indexOf('\n');
    if(ind<0){
        return apply();
    }
    cursor.insertText(str.left(ind+1));
    return false;
}

bool CodeGeeX4Suggestion::applyWord(TextEditorWidget *widget)
{
    const Completion completion = m_completions.value(m_currentCompletion);
    const QTextCursor cursor = completion.range().toSelection(m_start.document());
    QTextCursor currentCursor = widget->textCursor();
    const QString text = completion.text();
    const int startPos = currentCursor.positionInBlock() - cursor.positionInBlock()
                         + (cursor.selectionEnd() - cursor.selectionStart());
    const int next = endOfNextWord(text, startPos);

    if (next == -1)
        return apply();

    // TODO: Allow adding more than one line
    QString subText = text.mid(startPos, next - startPos);
    subText = subText.left(subText.indexOf('\n'));
    if (subText.isEmpty())
        return false;

    currentCursor.insertText(subText);
    return false;
}

void CodeGeeX4Suggestion::reset()
{
    m_start.removeSelectedText();
}

int CodeGeeX4Suggestion::position()
{
    return m_start.selectionEnd();
}

} // namespace CodeGeeX4::Internal

