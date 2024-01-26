/* $Id: QIStatusBarIndicator.cpp $ */
/** @file
 * VBox Qt GUI - Qt extensions: QIStatusBarIndicator interface implementation.
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
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
#include <QHBoxLayout>
#include <QLabel>
#include <QIcon>
#include <QPainter>
#include <QStyle>
#ifdef VBOX_WS_MAC
# include <QContextMenuEvent>
#endif /* VBOX_WS_MAC */

/* GUI includes: */
#include "QIStatusBarIndicator.h"

/* Other VBox includes: */
#include <iprt/assert.h>


/*********************************************************************************************************************************
*   Class QIStatusBarIndicator implementation.                                                                                   *
*********************************************************************************************************************************/

QIStatusBarIndicator::QIStatusBarIndicator(QWidget *pParent /* = 0 */)
    : QWidget(pParent)
{
    /* Configure size-policy: */
    setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
}

#ifdef VBOX_WS_MAC
void QIStatusBarIndicator::mousePressEvent(QMouseEvent *pEvent)
{
    // WORKAROUND:
    // Do this for the left mouse button event only, cause in the case of the
    // right mouse button it could happen that the context menu event is
    // triggered twice. Also this isn't necessary for the middle mouse button
    // which would be some kind of overstated.
    if (pEvent->button() == Qt::LeftButton)
    {
        QContextMenuEvent cme(QContextMenuEvent::Mouse, pEvent->pos(), pEvent->globalPos());
        emit sigContextMenuRequest(this, &cme);
        if (cme.isAccepted())
            pEvent->accept();
        else
            QWidget::mousePressEvent(pEvent);
    }
    else
        QWidget::mousePressEvent(pEvent);
}
#endif /* VBOX_WS_MAC */

void QIStatusBarIndicator::mouseDoubleClickEvent(QMouseEvent *pEvent)
{
    emit sigMouseDoubleClick(this, pEvent);
}

void QIStatusBarIndicator::contextMenuEvent(QContextMenuEvent *pEvent)
{
    emit sigContextMenuRequest(this, pEvent);
}


/*********************************************************************************************************************************
*   Class QIStateStatusBarIndicator implementation.                                                                              *
*********************************************************************************************************************************/

QIStateStatusBarIndicator::QIStateStatusBarIndicator(QWidget *pParent /* = 0 */)
  : QIStatusBarIndicator(pParent)
  , m_iState(0)
{
}

QIcon QIStateStatusBarIndicator::stateIcon(int iState) const
{
    /* Check if state-icon was set before: */
    return m_icons.value(iState, QIcon());
}

void QIStateStatusBarIndicator::setStateIcon(int iState, const QIcon &icon)
{
    /* Adjust size-hint: */
    const QStyle *pStyle = QApplication::style();
    const int iIconMetric = pStyle->pixelMetric(QStyle::PM_SmallIconSize);
    m_size = QSize(iIconMetric, iIconMetric);
    /* Cache passed-icon: */
    m_icons[iState] = icon;
}

void QIStateStatusBarIndicator::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    drawContents(&painter);
}

void QIStateStatusBarIndicator::drawContents(QPainter *pPainter)
{
    if (m_icons.contains(m_iState))
    {
        if (window())
            pPainter->drawPixmap(contentsRect().topLeft(), m_icons.value(m_iState).pixmap(window()->windowHandle(), m_size));
        else
            pPainter->drawPixmap(contentsRect().topLeft(), m_icons.value(m_iState).pixmap(m_size));
    }
}


/*********************************************************************************************************************************
*   Class QITextStatusBarIndicator implementation.                                                                               *
*********************************************************************************************************************************/

QITextStatusBarIndicator::QITextStatusBarIndicator(QWidget *pParent /* = 0 */)
    : QIStatusBarIndicator(pParent)
    , m_pLabel(0)
{
    /* Create main-layout: */
    QHBoxLayout *pMainLayout = new QHBoxLayout(this);
    if (pMainLayout)
    {
        /* Configure main-layout: */
        pMainLayout->setContentsMargins(0, 0, 0, 0);
        pMainLayout->setSpacing(0);
        /* Create label: */
        m_pLabel = new QLabel;
        if (m_pLabel)
        {
            /* Add label into main-layout: */
            pMainLayout->addWidget(m_pLabel);
        }
    }
}

QString QITextStatusBarIndicator::text() const
{
    AssertPtrReturn(m_pLabel, QString());
    return m_pLabel->text();
}

void QITextStatusBarIndicator::setText(const QString &strText)
{
    AssertPtrReturnVoid(m_pLabel);
    m_pLabel->setText(strText);
}
