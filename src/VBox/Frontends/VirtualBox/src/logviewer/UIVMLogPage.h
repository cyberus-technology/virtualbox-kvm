/* $Id: UIVMLogPage.h $ */
/** @file
 * VBox Qt GUI - UIVMLogViewer class declaration.
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

#ifndef FEQT_INCLUDED_SRC_logviewer_UIVMLogPage_h
#define FEQT_INCLUDED_SRC_logviewer_UIVMLogPage_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QWidget>
#include <QUuid>
#include <QPair>

/* GUI includes: */
#include "QIManagerDialog.h"
#include "QIWithRetranslateUI.h"
#include "UIVMLogBookmark.h"

/* Forward declarations: */
class QITabWidget;
class QHBoxLayout;
class QPlainTextEdit;
class UIVMLogViewerTextEdit;


class UIVMLogBookmarkManager
{
public:
    void addBookmark(const UIVMLogBookmark& newBookmark)
    {
        foreach (const UIVMLogBookmark& bookmark, m_bookmarks)
            if (bookmark == newBookmark)
                return;
        m_bookmarks << newBookmark;
    }

    void addBookmark(int iCursorPosition, int iLineNumber, QString strBlockText)
    {
        foreach (const UIVMLogBookmark& bookmark, m_bookmarks)
            if (bookmark.m_iLineNumber == iLineNumber)
                return;
        m_bookmarks << UIVMLogBookmark(iCursorPosition, iLineNumber, strBlockText);
    }

    void deleteBookmark(const UIVMLogBookmark& bookmark)
    {
        int index = -1;
        for (int i = 0; i < m_bookmarks.size() && index == -1; ++i)
        {
            if (bookmark == m_bookmarks[i])
                index = i;
        }
        deleteBookmarkByIndex(index);
    }

    void deleteBookmarkByIndex(int iIndex)
    {
        if (iIndex >= m_bookmarks.size() || iIndex < 0)
            return;
        m_bookmarks.removeAt(iIndex);
    }

    void deleteAllBookmarks()
    {
        m_bookmarks.clear();
    }

    int cursorPosition(int bookmarkIndex)
    {
        if (bookmarkIndex >= m_bookmarks.size())
            return 0;
        return m_bookmarks[bookmarkIndex].m_iCursorPosition;
    }

    QSet<int> lineSet() const
    {
        QSet<int> lines;
        foreach (const UIVMLogBookmark& bookmark, m_bookmarks)
            lines << bookmark.m_iLineNumber;
        return lines;
    }

    const QVector<UIVMLogBookmark>& bookmarkList() const
    {
        return m_bookmarks;
    }

private:

    QVector<UIVMLogBookmark> m_bookmarks;
};


class UIVMLogTab : public QIWithRetranslateUI<QWidget>
{

    Q_OBJECT;

public:

    UIVMLogTab(QWidget *pParent, const QUuid &uMachineId, const QString &strMachineName);
    const QUuid &machineId() const;
    const QString machineName() const;

private:

    QUuid m_uMachineId;
    QString m_strMachineName;
};

/** UIVMLogPage defines data and functionalities of the each tab page of a UIVMLogViewerWidget.
 *  It stores the original log file content , a list of bookmarks, etc */
class UIVMLogPage  : public UIVMLogTab
{
    Q_OBJECT;

signals:

    void sigBookmarksUpdated();
    void sigLogPageFilteredChanged(bool isFiltered);

public:

    UIVMLogPage(QWidget *pParent, const QUuid &uMachineId, const QString &strMachineName);
    ~UIVMLogPage();

    /** Returns the width of the current log page. return 0 if there is no current log page: */
    int defaultLogPageWidth() const;

    QPlainTextEdit *textEdit();
    QTextDocument  *document();

    void setLogContent(const QString &strLogContent, bool fError);
    const QString& logString() const;

    void setLogFileName(const QString &strFileName);
    const QString& logFileName() const;

    /** Marks the plain text edit When we dont have a log content. */
    void markForError();

    void setScrollBarMarkingsVector(const QVector<float> &vector);
    void clearScrollBarMarkingsVector();

    /** Undos the changes done to textDocument */
    void documentUndo();

    const QVector<UIVMLogBookmark>& bookmarkList() const;

    void deleteAllBookmarks();
    /** Scrolls the plain text edit to the bookmark with index @a bookmarkIndex. */
    void scrollToBookmark(int bookmarkIndex);

    bool isFiltered() const;
    void setFiltered(bool filtered);

    void setShowLineNumbers(bool bShowLineNumbers);
    void setWrapLines(bool bWrapLines);

    QFont currentFont() const;
    void setCurrentFont(QFont font);

    void setLogFileId(int iLogFileId);
    int logFileId() const;

    void scrollToEnd();

    void saveScrollBarPosition();
    void restoreScrollBarPosition();

    void deleteBookmarkByIndex(int iIndex);

private slots:

    void sltAddBookmark(const UIVMLogBookmark& bookmark);
    void sltDeleteBookmark(const UIVMLogBookmark& bookmark);

private:

    void prepare();
    void prepareWidgets();
    void cleanup();
    void retranslateUi();
    void updateTextEditBookmarkLineSet();

    /** Set plaintextEdit's text. Note that the text we
     *  show currently might be different than
     *  m_strLog. For example during filtering. */
    void setTextEditText(const QString &strText);
    void setTextEditTextAsHtml(const QString &strText);

    QHBoxLayout    *m_pMainLayout;
    UIVMLogViewerTextEdit *m_pTextEdit;
    /** Stores the log file (unmodified by filtering etc) content. */
    QString         m_strLog;
    /** Stores full path and name of the log file. */
    QString         m_strLogFileName;
    /** Stores the bookmarks of the logpage. All other bookmark related containers are updated wrt. this one. */
    UIVMLogBookmarkManager m_bookmarkManager;

    /** Keeps the index of the selected bookmark. Used especially when moving from one tab to another. */
    int                  m_iSelectedBookmarkIndex;

    /** @name Filtering related state variables
     * @{ */
    /** Designates whether currently displayed text is log text or a filtered version of it. That is
        if m_bFiltered is false than (m_strLog == m_pTextEdit->text()). */
        bool           m_bFiltered;
    /** @} */
    /** The id we pass to CMachine::ReadLog. Used while refreshing and saving page content. */
    int m_iLogFileId;
};

#endif /* !FEQT_INCLUDED_SRC_logviewer_UIVMLogPage_h */
