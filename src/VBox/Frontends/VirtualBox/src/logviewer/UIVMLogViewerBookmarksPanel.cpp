/* $Id: UIVMLogViewerBookmarksPanel.cpp $ */
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
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>
#ifdef RT_OS_SOLARIS
# include <QFontDatabase>
#endif

/* GUI includes: */
#include "QIToolButton.h"
#include "UIIconPool.h"
#include "UIVMLogViewerBookmarksPanel.h"
#include "UIVMLogViewerWidget.h"


UIVMLogViewerBookmarksPanel::UIVMLogViewerBookmarksPanel(QWidget *pParent, UIVMLogViewerWidget *pViewer)
    : UIVMLogViewerPanel(pParent, pViewer)
    , m_iMaxBookmarkTextLength(60)
    , m_pBookmarksComboBox(0)
    , m_pGotoSelectedBookmark(0)
    , m_pDeleteAllButton(0)
    , m_pDeleteCurrentButton(0)
    , m_pNextButton(0)
    , m_pPreviousButton(0)
{
    prepare();
}

void UIVMLogViewerBookmarksPanel::updateBookmarkList(const QVector<UIVMLogBookmark>& bookmarkList)
{
    if (!m_pBookmarksComboBox || !viewer())
        return;

    m_pBookmarksComboBox->clear();
    QStringList bList;
    bList << "";
    for (int i = 0; i < bookmarkList.size(); ++i)
    {
        QString strItem = QString("BookMark %1 at Line %2: %3").arg(QString::number(i)).
            arg(QString::number(bookmarkList[i].m_iLineNumber)).arg(bookmarkList[i].m_strBlockText);

        if (strItem.length() > m_iMaxBookmarkTextLength)
        {
            strItem.resize(m_iMaxBookmarkTextLength);
            strItem.replace(m_iMaxBookmarkTextLength, 3, QString("..."));
        }
        bList << strItem;
    }
    m_pBookmarksComboBox->addItems(bList);
    /* Goto last item of the combobox. Avoid emitting sigBookmarkSelected since we dont want text edit to scroll to there: */
    m_pBookmarksComboBox->blockSignals(true);
    m_pBookmarksComboBox->setCurrentIndex(m_pBookmarksComboBox->count()-1);
    m_pBookmarksComboBox->blockSignals(false);
}

void UIVMLogViewerBookmarksPanel::disableEnableBookmarking(bool flag)
{
    m_pBookmarksComboBox->setEnabled(flag);
    m_pGotoSelectedBookmark->setEnabled(flag);
    m_pDeleteAllButton->setEnabled(flag);
    m_pDeleteCurrentButton->setEnabled(flag);
    m_pNextButton->setEnabled(flag);
    m_pPreviousButton->setEnabled(flag);
}

QString UIVMLogViewerBookmarksPanel::panelName() const
{
    return "BookmarksPanel";
}

void UIVMLogViewerBookmarksPanel::setBookmarkIndex(int index)
{
    if (!m_pBookmarksComboBox)
        return;
    /* If there is only Title of the combo, then goto that item: */
    if (m_pBookmarksComboBox->count() == 1 || index >= m_pBookmarksComboBox->count())
    {
        m_pBookmarksComboBox->setCurrentIndex(0);
        return;
    }
    /* index+1 since we always have a 0th item in our combo-box. */
    m_pBookmarksComboBox->setCurrentIndex(index+1);
}

void UIVMLogViewerBookmarksPanel::prepareWidgets()
{
    if (!mainLayout())
        return;

    /* Create bookmark combo/button layout: */
    QHBoxLayout *pComboButtonLayout = new QHBoxLayout;
    if (pComboButtonLayout)
    {
        pComboButtonLayout->setContentsMargins(0, 0, 0, 0);
#ifdef VBOX_WS_MAC
        pComboButtonLayout->setSpacing(5);
#else
        pComboButtonLayout->setSpacing(qApp->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing) / 2);
#endif

        /* Create bookmark combo-box: */
        m_pBookmarksComboBox = new QComboBox;
        if (m_pBookmarksComboBox)
        {
            m_pBookmarksComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
            /* Make sure we have 0th item in our combo-box. */
            m_pBookmarksComboBox->insertItem(0, "");
            pComboButtonLayout->addWidget(m_pBookmarksComboBox);
        }

        /* Create bookmark button layout 1: */
        QHBoxLayout *pButtonLayout1 = new QHBoxLayout;
        if (pButtonLayout1)
        {
            pButtonLayout1->setContentsMargins(0, 0, 0, 0);
            pButtonLayout1->setSpacing(0);

            /* Create goto selected bookmark button: */
            m_pGotoSelectedBookmark = new QIToolButton;
            if (m_pGotoSelectedBookmark)
            {
                m_pGotoSelectedBookmark->setIcon(UIIconPool::iconSet(":/log_viewer_goto_selected_bookmark_16px.png"));
                pButtonLayout1->addWidget(m_pGotoSelectedBookmark);
            }

            /* Create goto previous bookmark button: */
            m_pPreviousButton = new QIToolButton;
            if (m_pPreviousButton)
            {
                m_pPreviousButton->setIcon(UIIconPool::iconSet(":/log_viewer_goto_previous_bookmark_16px.png"));
                pButtonLayout1->addWidget(m_pPreviousButton);
            }

            /* Create goto next bookmark button: */
            m_pNextButton = new QIToolButton;
            if (m_pNextButton)
            {
                m_pNextButton->setIcon(UIIconPool::iconSet(":/log_viewer_goto_next_bookmark_16px.png"));
                pButtonLayout1->addWidget(m_pNextButton);
            }

            pComboButtonLayout->addLayout(pButtonLayout1);
        }

        /* Create bookmark button layout 2: */
        QHBoxLayout *pButtonLayout2 = new QHBoxLayout;
        if (pButtonLayout2)
        {
            pButtonLayout2->setContentsMargins(0, 0, 0, 0);
            pButtonLayout2->setSpacing(0);

            /* Create delete current bookmark button: */
            m_pDeleteCurrentButton = new QIToolButton;
            if (m_pDeleteCurrentButton)
            {
                m_pDeleteCurrentButton->setIcon(UIIconPool::iconSet(":/log_viewer_delete_current_bookmark_16px.png"));
                pButtonLayout2->addWidget(m_pDeleteCurrentButton);
            }

            /* Create delete all bookmarks button: */
            m_pDeleteAllButton = new QIToolButton;
            if (m_pDeleteAllButton)
            {
                m_pDeleteAllButton->setIcon(UIIconPool::iconSet(":/log_viewer_delete_all_bookmarks_16px.png"));
                pButtonLayout2->addWidget(m_pDeleteAllButton);
            }

            pComboButtonLayout->addLayout(pButtonLayout2);
        }

        mainLayout()->addLayout(pComboButtonLayout);
    }
}

void UIVMLogViewerBookmarksPanel::prepareConnections()
{
    connect(m_pBookmarksComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &UIVMLogViewerBookmarksPanel::sltBookmarkSelected);

    connect(m_pGotoSelectedBookmark, &QIToolButton::clicked, this, &UIVMLogViewerBookmarksPanel::sltGotoSelectedBookmark);
    connect(m_pNextButton, &QIToolButton::clicked, this, &UIVMLogViewerBookmarksPanel::sltGotoNextBookmark);
    connect(m_pPreviousButton, &QIToolButton::clicked, this, &UIVMLogViewerBookmarksPanel::sltGotoPreviousBookmark);

    connect(m_pDeleteAllButton, &QIToolButton::clicked, this, &UIVMLogViewerBookmarksPanel::sigDeleteAllBookmarks);
    connect(m_pDeleteCurrentButton, &QIToolButton::clicked, this, &UIVMLogViewerBookmarksPanel::sltDeleteCurrentBookmark);
}


void UIVMLogViewerBookmarksPanel::retranslateUi()
{
    UIVMLogViewerPanel::retranslateUi();

    m_pDeleteCurrentButton->setToolTip(UIVMLogViewerWidget::tr("Delete the current bookmark"));
    m_pDeleteAllButton->setToolTip(UIVMLogViewerWidget::tr("Delete all bookmarks"));
    m_pNextButton->setToolTip(UIVMLogViewerWidget::tr("Go to the next bookmark"));
    m_pPreviousButton->setToolTip(UIVMLogViewerWidget::tr("Go to the previous bookmark"));
    m_pGotoSelectedBookmark->setToolTip(UIVMLogViewerWidget::tr("Go to selected bookmark"));
}

void UIVMLogViewerBookmarksPanel::sltDeleteCurrentBookmark()
{
    if (!m_pBookmarksComboBox)
        return;

    if (m_pBookmarksComboBox->currentIndex() == 0)
        return;
    emit sigDeleteBookmarkByIndex(m_pBookmarksComboBox->currentIndex() - 1);
}

void UIVMLogViewerBookmarksPanel::sltBookmarkSelected(int index)
{
    /* Do nothing if the index is 0, that is combo-box title item: */
    if (index <= 0)
        return;
    emit sigBookmarkSelected(index - 1);
}


void UIVMLogViewerBookmarksPanel::sltGotoNextBookmark()
{
    /* go to next bookmark or wrap around to the beginning of the list: */
    if (m_pBookmarksComboBox->currentIndex() == m_pBookmarksComboBox->count()-1)
        m_pBookmarksComboBox->setCurrentIndex(1);
    else
        m_pBookmarksComboBox->setCurrentIndex(m_pBookmarksComboBox->currentIndex() + 1);
}


void UIVMLogViewerBookmarksPanel::sltGotoPreviousBookmark()
{
    /* go to previous bookmark or wrap around to the end of the list: */
    if (m_pBookmarksComboBox->currentIndex() <= 1)
        m_pBookmarksComboBox->setCurrentIndex(m_pBookmarksComboBox->count() - 1);
    else
        m_pBookmarksComboBox->setCurrentIndex(m_pBookmarksComboBox->currentIndex() - 1);
}

void UIVMLogViewerBookmarksPanel::sltGotoSelectedBookmark()
{
    if (!m_pBookmarksComboBox || m_pBookmarksComboBox->count() <= 1)
        return;
    emit sigBookmarkSelected(m_pBookmarksComboBox->currentIndex() - 1);
}
