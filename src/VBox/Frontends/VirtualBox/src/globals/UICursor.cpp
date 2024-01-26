/* $Id: UICursor.cpp $ */
/** @file
 * VBox Qt GUI - UICursor namespace implementation.
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
#include <QGraphicsWidget>
#include <QWidget>

/* GUI includes: */
#include "UICommon.h"
#include "UICursor.h"


/* static */
void UICursor::setCursor(QWidget *pWidget, const QCursor &cursor)
{
    if (!pWidget)
        return;

#ifdef VBOX_WS_X11
    /* As reported in https://www.virtualbox.org/ticket/16348,
     * in X11 QWidget::setCursor(..) call uses RENDER
     * extension. Qt (before 5.11) fails to handle the case where the mentioned extension
     * is missing. Please see https://codereview.qt-project.org/#/c/225665/ for Qt patch: */
    if ((UICommon::qtRTMajorVersion() < 5) ||
        (UICommon::qtRTMajorVersion() == 5 && UICommon::qtRTMinorVersion() < 11))
    {
        if (NativeWindowSubsystem::X11CheckExtension("RENDER"))
            pWidget->setCursor(cursor);
    }
    else
    {
        pWidget->setCursor(cursor);
    }
#else
    pWidget->setCursor(cursor);
#endif
}

/* static */
void UICursor::setCursor(QGraphicsWidget *pWidget, const QCursor &cursor)
{
    if (!pWidget)
        return;

#ifdef VBOX_WS_X11
    /* As reported in https://www.virtualbox.org/ticket/16348,
     * in X11 QGraphicsWidget::setCursor(..) call uses RENDER
     * extension. Qt (before 5.11) fails to handle the case where the mentioned extension
     * is missing. Please see https://codereview.qt-project.org/#/c/225665/ for Qt patch: */
    if ((UICommon::qtRTMajorVersion() < 5) ||
        (UICommon::qtRTMajorVersion() == 5 && UICommon::qtRTMinorVersion() < 11))
    {
        if (NativeWindowSubsystem::X11CheckExtension("RENDER"))
            pWidget->setCursor(cursor);
    }
    else
    {
        pWidget->setCursor(cursor);
    }
#else
    pWidget->setCursor(cursor);
#endif
}

/* static */
void UICursor::unsetCursor(QWidget *pWidget)
{
    if (!pWidget)
        return;

#ifdef VBOX_WS_X11
    /* As reported in https://www.virtualbox.org/ticket/16348,
     * in X11 QWidget::unsetCursor(..) call uses RENDER
     * extension. Qt (before 5.11) fails to handle the case where the mentioned extension
     * is missing. Please see https://codereview.qt-project.org/#/c/225665/ for Qt patch: */
    if ((UICommon::qtRTMajorVersion() < 5) ||
        (UICommon::qtRTMajorVersion() == 5 && UICommon::qtRTMinorVersion() < 11))
    {
        if (NativeWindowSubsystem::X11CheckExtension("RENDER"))
            pWidget->unsetCursor();
    }
    else
    {
        pWidget->unsetCursor();
    }
#else
    pWidget->unsetCursor();
#endif
}

/* static */
void UICursor::unsetCursor(QGraphicsWidget *pWidget)
{
    if (!pWidget)
        return;

#ifdef VBOX_WS_X11
    /* As reported in https://www.virtualbox.org/ticket/16348,
     * in X11 QGraphicsWidget::unsetCursor(..) call uses RENDER
     * extension. Qt (before 5.11) fails to handle the case where the mentioned extension
     * is missing. Please see https://codereview.qt-project.org/#/c/225665/ for Qt patch: */
    if ((UICommon::qtRTMajorVersion() < 5) ||
        (UICommon::qtRTMajorVersion() == 5 && UICommon::qtRTMinorVersion() < 11))
    {
        if (NativeWindowSubsystem::X11CheckExtension("RENDER"))
            pWidget->unsetCursor();
    }
    else
    {
        pWidget->unsetCursor();
    }
#else
    pWidget->unsetCursor();
#endif
}
