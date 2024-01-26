/* $Id: UIVMLogPage.cpp $ */
/** @file
 * VBox Qt GUI - UIVMLogViewer class implementation.
 */

/*
 * Copyright (C) 2010-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

/* Qt includes: */
#include <QDateTime>
#include <QDir>
#include <QVBoxLayout>
#if defined(RT_OS_SOLARIS)
# include <QFontDatabase>
#endif
#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>

/* GUI includes: */
#include "UIVMLogPage.h"
#include "UIVMLogViewerTextEdit.h"


/*********************************************************************************************************************************
*   UIVMLogTab implementation.                                                                                                   *
*********************************************************************************************************************************/

UIVMLogTab::UIVMLogTab(QWidget *pParent, const QUuid &uMachineId, const QString &strMachineName)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_uMachineId(uMachineId)
    , m_strMachineName(strMachineName)
{
}
const QUuid &UIVMLogTab::machineId() const
{
    return m_uMachineId;
}

const QString UIVMLogTab::machineName() const
{
    return m_strMachineName;
}


/*********************************************************************************************************************************
*   UIVMLogPage implementation.                                                                                                  *
*********************************************************************************************************************************/

UIVMLogPage::UIVMLogPage(QWidget *pParent, const QUuid &uMachineId, const QString &strMachineName)
    : UIVMLogTab(pParent, uMachineId, strMachineName)
    , m_pMainLayout(0)
    , m_pTextEdit(0)
    , m_iSelectedBookmarkIndex(-1)
    , m_bFiltered(false)
    , m_iLogFileId(-1)
{
    prepare();
}

UIVMLogPage::~UIVMLogPage()
{
    cleanup();
}

int UIVMLogPage::defaultLogPageWidth() const
{
    if (!m_pTextEdit)
        return 0;

    /* Compute a width for 132 characters plus scrollbar and frame width: */
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    int iDefaultWidth = m_pTextEdit->fontMetrics().horizontalAdvance(QChar('x')) * 132 +
#else
    int iDefaultWidth = m_pTextEdit->fontMetrics().width(QChar('x')) * 132 +
#endif
                        m_pTextEdit->verticalScrollBar()->width() +
                        m_pTextEdit->frameWidth() * 2;

    return iDefaultWidth;
}


void UIVMLogPage::prepare()
{
    prepareWidgets();
    retranslateUi();
}

void UIVMLogPage::prepareWidgets()
{
    m_pMainLayout = new QHBoxLayout();
    setLayout(m_pMainLayout);
    m_pMainLayout->setSpacing(0);
    m_pMainLayout->setContentsMargins(0, 0, 0, 0);

    m_pTextEdit = new UIVMLogViewerTextEdit(this);
    m_pMainLayout->addWidget(m_pTextEdit);

    connect(m_pTextEdit, &UIVMLogViewerTextEdit::sigAddBookmark, this, &UIVMLogPage::sltAddBookmark);
    connect(m_pTextEdit, &UIVMLogViewerTextEdit::sigDeleteBookmark, this, &UIVMLogPage::sltDeleteBookmark);
}

QPlainTextEdit *UIVMLogPage::textEdit()
{
    return m_pTextEdit;
}

QTextDocument* UIVMLogPage::document()
{
    if (!m_pTextEdit)
        return 0;
    return m_pTextEdit->document();
}

void UIVMLogPage::retranslateUi()
{
}

void UIVMLogPage::cleanup()
{
}

void UIVMLogPage::setLogContent(const QString &strLogContent, bool fError)
{
    if (!fError)
    {
        m_strLog = strLogContent;
        setTextEditText(strLogContent);
    }
    else
    {
        markForError();
        setTextEditTextAsHtml(strLogContent);
    }
}

const QString& UIVMLogPage::logString() const
{
    return m_strLog;
}

void UIVMLogPage::setLogFileName(const QString &strLogFileName)
{
    m_strLogFileName = strLogFileName;
}

const QString& UIVMLogPage::logFileName() const
{
    return m_strLogFileName;
}

void UIVMLogPage::setTextEditText(const QString &strText)
{
    if (!m_pTextEdit)
        return;

    m_pTextEdit->setPlainText(strText);
    /* Move the cursor position to end: */
    QTextCursor cursor = m_pTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
    m_pTextEdit->setTextCursor(cursor);
    update();
}

void UIVMLogPage::setTextEditTextAsHtml(const QString &strText)
{
    if (!m_pTextEdit)
        return;
    if (document())
        document()->setHtml(strText);
    update();
}

void UIVMLogPage::markForError()
{
    if (!m_pTextEdit)
        return;
    m_pTextEdit->setWrapLines(true);
}

void UIVMLogPage::setScrollBarMarkingsVector(const QVector<float> &vector)
{
    if (!m_pTextEdit)
        return;
    m_pTextEdit->setScrollBarMarkingsVector(vector);
    update();
}

void UIVMLogPage::clearScrollBarMarkingsVector()
{
    if (!m_pTextEdit)
        return;
    m_pTextEdit->clearScrollBarMarkingsVector();
    update();
}

void UIVMLogPage::documentUndo()
{
    if (!m_pTextEdit)
        return;
    if (m_pTextEdit->document())
        m_pTextEdit->document()->undo();
}



void UIVMLogPage::deleteAllBookmarks()
{
    m_bookmarkManager.deleteAllBookmarks();
    updateTextEditBookmarkLineSet();
}

void UIVMLogPage::scrollToBookmark(int bookmarkIndex)
{
    if (!m_pTextEdit)
        return;
    m_pTextEdit->setCursorPosition(m_bookmarkManager.cursorPosition(bookmarkIndex));
}

const QVector<UIVMLogBookmark>& UIVMLogPage::bookmarkList() const
{
    return m_bookmarkManager.bookmarkList();
}

void UIVMLogPage::sltAddBookmark(const UIVMLogBookmark& bookmark)
{
    m_bookmarkManager.addBookmark(bookmark);
    updateTextEditBookmarkLineSet();
    emit sigBookmarksUpdated();
}

void UIVMLogPage::sltDeleteBookmark(const UIVMLogBookmark& bookmark)
{
    m_bookmarkManager.deleteBookmark(bookmark);
    updateTextEditBookmarkLineSet();
    emit sigBookmarksUpdated();
}

void UIVMLogPage::deleteBookmarkByIndex(int iIndex)
{
    m_bookmarkManager.deleteBookmarkByIndex(iIndex);
    updateTextEditBookmarkLineSet();
    emit sigBookmarksUpdated();
}

void UIVMLogPage::updateTextEditBookmarkLineSet()
{
    if (!m_pTextEdit)
        return;
    m_pTextEdit->setBookmarkLineSet(m_bookmarkManager.lineSet());
}

bool UIVMLogPage::isFiltered() const
{
    return m_bFiltered;
}

void UIVMLogPage::setFiltered(bool filtered)
{
    if (m_bFiltered == filtered)
        return;
    m_bFiltered = filtered;
    if (m_pTextEdit)
    {
        m_pTextEdit->setShownTextIsFiltered(m_bFiltered);
        m_pTextEdit->update();
    }
    emit sigLogPageFilteredChanged(m_bFiltered);
}

void UIVMLogPage::setShowLineNumbers(bool bShowLineNumbers)
{
    if (!m_pTextEdit)
        return;
    m_pTextEdit->setShowLineNumbers(bShowLineNumbers);
}

void UIVMLogPage::setWrapLines(bool bWrapLines)
{
    if (!m_pTextEdit)
        return;
    m_pTextEdit->setWrapLines(bWrapLines);
}

QFont UIVMLogPage::currentFont() const
{
    if (!m_pTextEdit)
        return QFont();
    return m_pTextEdit->font();
}

void UIVMLogPage::setCurrentFont(QFont font)
{
    if (m_pTextEdit)
        m_pTextEdit->setCurrentFont(font);
}

void UIVMLogPage::setLogFileId(int iLogFileId)
{
    m_iLogFileId = iLogFileId;
}

int UIVMLogPage::logFileId() const
{
    return m_iLogFileId;
}

void UIVMLogPage::scrollToEnd()
{
    if (m_pTextEdit)
        m_pTextEdit->scrollToEnd();
}

void UIVMLogPage::saveScrollBarPosition()
{
    if (m_pTextEdit)
        m_pTextEdit->saveScrollBarPosition();
}

void UIVMLogPage::restoreScrollBarPosition()
{
    if (m_pTextEdit)
        m_pTextEdit->restoreScrollBarPosition();
}
