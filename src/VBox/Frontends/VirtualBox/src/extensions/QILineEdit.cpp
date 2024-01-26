/* $Id: QILineEdit.cpp $ */
/** @file
 * VBox Qt GUI - Qt extensions: QILineEdit class implementation.
 */

/*
 * Copyright (C) 2008-2023 Oracle and/or its affiliates.
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
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPalette>
#include <QStyleOptionFrame>

/* GUI includes: */
#include "QILineEdit.h"
#include "UIIconPool.h"

/* Other VBox includes: */
#include "iprt/assert.h"


QILineEdit::QILineEdit(QWidget *pParent /* = 0 */)
    : QLineEdit(pParent)
    , m_fAllowToCopyContentsWhenDisabled(false)
    , m_pCopyAction(0)
    , m_pIconLabel(0)
    , m_fMarkForError(false)
{
    prepare();
}

QILineEdit::QILineEdit(const QString &strText, QWidget *pParent /* = 0 */)
    : QLineEdit(strText, pParent)
    , m_fAllowToCopyContentsWhenDisabled(false)
    , m_pCopyAction(0)
    , m_pIconLabel(0)
    , m_fMarkForError(false)
{
    prepare();
}

void QILineEdit::setAllowToCopyContentsWhenDisabled(bool fAllow)
{
    m_fAllowToCopyContentsWhenDisabled = fAllow;
}

void QILineEdit::setMinimumWidthByText(const QString &strText)
{
    setMinimumWidth(fitTextWidth(strText).width());
}

void QILineEdit::setFixedWidthByText(const QString &strText)
{
    setFixedWidth(fitTextWidth(strText).width());
}

void QILineEdit::mark(bool fError, const QString &strErrorMessage /* = QString() */)
{
    /* Check if something really changed: */
    if (fError == m_fMarkForError && m_strErrorMessage == strErrorMessage)
        return;

    /* Save new values: */
    m_fMarkForError = fError;
    m_strErrorMessage = strErrorMessage;

    /* Update accordingly: */
    if (m_fMarkForError)
    {
        /* Create label if absent: */
        if (!m_pIconLabel)
            m_pIconLabel = new QLabel(this);

        /* Update label content, visibility & position: */
        const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) * .625;
        const int iShift = height() > iIconMetric ? (height() - iIconMetric) / 2 : 0;
        m_pIconLabel->setPixmap(m_markIcon.pixmap(windowHandle(), QSize(iIconMetric, iIconMetric)));
        m_pIconLabel->setToolTip(m_strErrorMessage);
        m_pIconLabel->move(width() - iIconMetric - iShift, iShift);
        m_pIconLabel->show();
    }
    else
    {
        /* Hide label: */
        if (m_pIconLabel)
            m_pIconLabel->hide();
    }
}

bool QILineEdit::event(QEvent *pEvent)
{
    switch (pEvent->type())
    {
        case QEvent::ContextMenu:
        {
            /* For disabled widget if requested: */
            if (!isEnabled() && m_fAllowToCopyContentsWhenDisabled)
            {
                /* Create a context menu for the copy to clipboard action: */
                QContextMenuEvent *pContextMenuEvent = static_cast<QContextMenuEvent*>(pEvent);
                QMenu menu;
                m_pCopyAction->setText(tr("&Copy"));
                menu.addAction(m_pCopyAction);
                menu.exec(pContextMenuEvent->globalPos());
                pEvent->accept();
            }
            break;
        }
        default:
            break;
    }
    return QLineEdit::event(pEvent);
}

void QILineEdit::resizeEvent(QResizeEvent *pResizeEvent)
{
    /* Call to base-class: */
    QLineEdit::resizeEvent(pResizeEvent);

    /* Update error label position: */
    if (m_pIconLabel)
    {
        const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) * .625;
        const int iShift = height() > iIconMetric ? (height() - iIconMetric) / 2 : 0;
        m_pIconLabel->move(width() - iIconMetric - iShift, iShift);
    }
}

void QILineEdit::copy()
{
    /* Copy the current text to the global and selection clipboards: */
    QApplication::clipboard()->setText(text(), QClipboard::Clipboard);
    QApplication::clipboard()->setText(text(), QClipboard::Selection);
}

void QILineEdit::prepare()
{
    /* Prepare invisible copy action: */
    m_pCopyAction = new QAction(this);
    if (m_pCopyAction)
    {
        m_pCopyAction->setShortcut(QKeySequence(QKeySequence::Copy));
        m_pCopyAction->setShortcutContext(Qt::WidgetShortcut);
        connect(m_pCopyAction, &QAction::triggered, this, &QILineEdit::copy);
        addAction(m_pCopyAction);
    }

    /* Prepare warning icon: */
    m_markIcon = UIIconPool::iconSet(":/status_error_16px.png");
}

QSize QILineEdit::fitTextWidth(const QString &strText) const
{
    QStyleOptionFrame sof;
    sof.initFrom(this);
    sof.rect = contentsRect();
    sof.lineWidth = hasFrame() ? style()->pixelMetric(QStyle::PM_DefaultFrameWidth) : 0;
    sof.midLineWidth = 0;
    sof.state |= QStyle::State_Sunken;

    /** @todo make it wise.. */
    // WORKAROUND:
    // The margins are based on qlineedit.cpp of Qt.
    // Maybe they where changed at some time in the future.
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    QSize sc(fontMetrics().horizontalAdvance(strText) + 2 * 2,
             fontMetrics().xHeight()                  + 2 * 1);
#else
    QSize sc(fontMetrics().width(strText) + 2 * 2,
             fontMetrics().xHeight()     + 2 * 1);
#endif
    const QSize sa = style()->sizeFromContents(QStyle::CT_LineEdit, &sof, sc, this);

    return sa;
}

UIMarkableLineEdit::UIMarkableLineEdit(QWidget *pParent /* = 0 */)
    :QWidget(pParent)
    , m_pLineEdit(0)
    , m_pIconLabel(0)
{
    prepare();
}

void UIMarkableLineEdit::setText(const QString &strText)
{
    if (m_pLineEdit)
        m_pLineEdit->setText(strText);
}

QString UIMarkableLineEdit::text() const
{
    if (!m_pLineEdit)
        return QString();
    return m_pLineEdit->text();
}

void UIMarkableLineEdit::setValidator(const QValidator *pValidator)
{
    if (m_pLineEdit)
        m_pLineEdit->setValidator(pValidator);
}

bool UIMarkableLineEdit::hasAcceptableInput() const
{
    if (!m_pLineEdit)
        return false;
    return m_pLineEdit->hasAcceptableInput();
}

void UIMarkableLineEdit::setPlaceholderText(const QString &strText)
{
    if (m_pLineEdit)
        m_pLineEdit->setPlaceholderText(strText);
}

void UIMarkableLineEdit::mark(bool fError, const QString &strErrorMessage /* = QString() */)
{
    m_pIconLabel->setVisible(true);
    AssertReturnVoid(m_pIconLabel);
    const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);

    if (fError)
        m_pIconLabel->setPixmap(UIIconPool::iconSet(":/status_error_16px.png").pixmap(windowHandle(), QSize(iIconMetric, iIconMetric)));
    else
        m_pIconLabel->setPixmap(UIIconPool::iconSet(":/status_check_16px.png").pixmap(windowHandle(), QSize(iIconMetric, iIconMetric)));
    m_pIconLabel->setToolTip(strErrorMessage);
}

void UIMarkableLineEdit::prepare()
{
    QHBoxLayout *pMainLayout = new QHBoxLayout(this);
    AssertReturnVoid(pMainLayout);
    pMainLayout->setContentsMargins(0, 0, 0, 0);
    m_pLineEdit = new QILineEdit;
    AssertReturnVoid(m_pLineEdit);
    m_pIconLabel = new QLabel;
    AssertReturnVoid(m_pIconLabel);
    /* Show the icon label only if line edit is marked for error/no error.*/
    m_pIconLabel->hide();
    pMainLayout->addWidget(m_pLineEdit);
    pMainLayout->addWidget(m_pIconLabel);
    setFocusProxy(m_pLineEdit);
    connect(m_pLineEdit, &QILineEdit::textChanged, this, &UIMarkableLineEdit::textChanged);
}
