/* $Id: UISearchLineEdit.cpp $ */
/** @file
 * VBox Qt GUI - UIsearchLineEdit class definitions.
 */

/*
 * Copyright (C) 2009-2023 Oracle and/or its affiliates.
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

/* Qt includes */
#include <QApplication>
#include <QPainter>

/* GUI includes: */
#include "UISearchLineEdit.h"

UISearchLineEdit::UISearchLineEdit(QWidget *pParent /* = 0 */)
    :QLineEdit(pParent)
    , m_iMatchCount(0)
    , m_iScrollToIndex(-1)
    , m_fMark(true)
    , m_unmarkColor(palette().color(QPalette::Base))
    , m_markColor(QColor(m_unmarkColor.red(),
                         0.7 * m_unmarkColor.green(),
                         0.7 * m_unmarkColor.blue()))
{
}

void UISearchLineEdit::paintEvent(QPaintEvent *pEvent)
{
    QLineEdit::paintEvent(pEvent);

    /* No search terms. no search. nothing to show here: */
    if (text().isEmpty())
    {
        colorBackground(false);
        return;
    }
    /* Draw the total match count and the current scrolled item's index on the right hand side of the line edit: */
    QPainter painter(this);
    QFont pfont = font();
    QString strText = QString("%1/%2").arg(QString::number(m_iScrollToIndex + 1)).arg(QString::number(m_iMatchCount));
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    QSize textSize(QApplication::fontMetrics().horizontalAdvance(strText),
                   QApplication::fontMetrics().height());
#else
    QSize textSize(QApplication::fontMetrics().width(strText),
                   QApplication::fontMetrics().height());
#endif

    /* Dont draw anything if we dont have enough space: */
    if (textSize.width() > 0.5 * width())
        return;
    int iTopMargin = (height() - textSize.height()) / 2;
    int iRightMargin = iTopMargin;

    QColor fontColor(Qt::black);
    painter.setPen(fontColor);
    painter.setFont(pfont);

    painter.drawText(QRect(width() - textSize.width() - iRightMargin, iTopMargin, textSize.width(), textSize.height()),
                     Qt::AlignCenter | Qt::AlignVCenter, strText);
    colorBackground(m_iMatchCount == 0);
}

void UISearchLineEdit::setMatchCount(int iMatchCount)
{
    if (m_iMatchCount == iMatchCount)
        return;
    m_iMatchCount = iMatchCount;
    repaint();
}

void UISearchLineEdit::setScrollToIndex(int iScrollToIndex)
{
    if (m_iScrollToIndex == iScrollToIndex)
        return;
    m_iScrollToIndex = iScrollToIndex;
    repaint();
}

void UISearchLineEdit::reset()
{
    clear();
    m_iMatchCount = 0;
    m_iScrollToIndex = 0;
    colorBackground(false);
}

void UISearchLineEdit::colorBackground(bool fWarning)
{
    QPalette mPalette = QApplication::palette();
    /** Make sure we reset color. */
    if (!fWarning || !m_fMark)
    {
        mPalette.setColor(QPalette::Base, m_unmarkColor);
        setPalette(mPalette);
        return;
    }

    if (m_fMark && fWarning)
    {
        mPalette.setColor(QPalette::Base, m_markColor);
        setPalette(mPalette);
        return;
    }
}
