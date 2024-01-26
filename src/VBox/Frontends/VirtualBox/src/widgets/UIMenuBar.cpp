/* $Id: UIMenuBar.cpp $ */
/** @file
 * VBox Qt GUI - UIMenuBar class implementation.
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
#include <QPainter>
#include <QPaintEvent>
#include <QPixmapCache>

/* GUI includes: */
#include "UICommon.h"
#include "UIDesktopWidgetWatchdog.h"
#include "UIImageTools.h"
#include "UIMenuBar.h"


UIMenuBar::UIMenuBar(QWidget *pParent /* = 0 */)
    : QMenuBar(pParent)
    , m_fShowBetaLabel(false)
{
    /* Check for beta versions: */
    if (uiCommon().showBetaLabel())
        m_fShowBetaLabel = true;
}

void UIMenuBar::paintEvent(QPaintEvent *pEvent)
{
    /* Call to base-class: */
    QMenuBar::paintEvent(pEvent);

    /* Draw BETA label if necessary: */
    if (m_fShowBetaLabel)
    {
        QPixmap betaLabel;
        const QString key("vbox:betaLabel");
        if (!QPixmapCache::find(key, &betaLabel))
        {
            betaLabel = ::betaLabel(QSize(80, 16), this);
            QPixmapCache::insert(key, betaLabel);
        }
        QSize s = size();
        QPainter painter(this);
        painter.setClipRect(pEvent->rect());
        const double dDpr = UIDesktopWidgetWatchdog::devicePixelRatio(this);
        painter.drawPixmap(s.width() - betaLabel.width() / dDpr - 10, (height() - betaLabel.height() / dDpr) / 2, betaLabel);
    }
}
