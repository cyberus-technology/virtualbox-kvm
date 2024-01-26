/* $Id: UIMachineViewSeamless.cpp $ */
/** @file
 * VBox Qt GUI - UIMachineViewSeamless class implementation.
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
#include <QApplication>
#include <QMainWindow>
#include <QTimer>
#ifdef VBOX_WS_MAC
# include <QMenuBar>
#endif /* VBOX_WS_MAC */

/* GUI includes: */
#include "UISession.h"
#include "UIMachineLogicSeamless.h"
#include "UIMachineWindow.h"
#include "UIMachineViewSeamless.h"
#include "UIFrameBuffer.h"
#include "UIExtraDataManager.h"
#include "UIDesktopWidgetWatchdog.h"

/* COM includes: */
#include "CConsole.h"
#include "CDisplay.h"

/* Other VBox includes: */
#include "VBox/log.h"

/* External includes: */
#ifdef VBOX_WS_X11
# include <limits.h>
#endif /* VBOX_WS_X11 */



UIMachineViewSeamless::UIMachineViewSeamless(UIMachineWindow *pMachineWindow, ulong uScreenId)
    : UIMachineView(pMachineWindow, uScreenId)
{
    /* Prepare seamless view: */
    prepareSeamless();
}

void UIMachineViewSeamless::sltAdditionsStateChanged()
{
    adjustGuestScreenSize();
}

void UIMachineViewSeamless::sltHandleSetVisibleRegion(QRegion region)
{
    /* Apply new seamless-region: */
    m_pFrameBuffer->handleSetVisibleRegion(region);
}

bool UIMachineViewSeamless::eventFilter(QObject *pWatched, QEvent *pEvent)
{
    if (pWatched != 0 && pWatched == machineWindow())
    {
        switch (pEvent->type())
        {
            case QEvent::Resize:
            {
                /* Send guest-resize hint only if top window resizing to required dimension: */
                QResizeEvent *pResizeEvent = static_cast<QResizeEvent*>(pEvent);
                if (pResizeEvent->size() != calculateMaxGuestSize())
                    break;

                /* Recalculate maximum guest size: */
                setMaximumGuestSize();

                break;
            }
            default:
                break;
        }
    }

    return UIMachineView::eventFilter(pWatched, pEvent);
}

void UIMachineViewSeamless::prepareCommon()
{
    /* Base class common settings: */
    UIMachineView::prepareCommon();

    /* Setup size-policy: */
    setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
    /* Maximum size to sizehint: */
    setMaximumSize(sizeHint());
    /* Minimum size is ignored: */
    setMinimumSize(0, 0);
    /* No scrollbars: */
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void UIMachineViewSeamless::prepareFilters()
{
    /* Base class filters: */
    UIMachineView::prepareFilters();
}

void UIMachineViewSeamless::prepareConsoleConnections()
{
    /* Base class connections: */
    UIMachineView::prepareConsoleConnections();

    /* Guest additions state-change updater: */
    connect(uisession(), &UISession::sigAdditionsStateActualChange, this, &UIMachineViewSeamless::sltAdditionsStateChanged);
}

void UIMachineViewSeamless::prepareSeamless()
{
    /* Set seamless feature flag to the guest: */
    display().SetSeamlessMode(true);
}

void UIMachineViewSeamless::cleanupSeamless()
{
    /* Reset seamless feature flag if possible: */
    if (uisession()->isRunning())
        display().SetSeamlessMode(false);
}

void UIMachineViewSeamless::adjustGuestScreenSize()
{
    /* Step 1: Is guest-screen visible? */
    if (!uisession()->isScreenVisible(screenId()))
    {
        LogRel(("GUI: UIMachineViewSeamless::adjustGuestScreenSize: "
                "Guest-screen #%d is not visible, adjustment is not required.\n",
                screenId()));
        return;
    }

    /* What are the desired and requested hints? */
    const QSize sizeToApply = calculateMaxGuestSize();
    const QSize desiredSizeHint = scaledBackward(sizeToApply);
    const QSize requestedSizeHint = requestedGuestScreenSizeHint();

    /* Step 2: Is the guest-screen of another size than necessary? */
    if (desiredSizeHint == requestedSizeHint)
    {
        LogRel(("GUI: UIMachineViewSeamless::adjustGuestScreenSize: "
                "Desired hint %dx%d for guest-screen #%d is already in IDisplay, adjustment is not required.\n",
                desiredSizeHint.width(), desiredSizeHint.height(), screenId()));
        return;
    }

    /* Final step: Adjust .. */
    LogRel(("GUI: UIMachineViewSeamless::adjustGuestScreenSize: "
            "Desired hint %dx%d for guest-screen #%d differs from the one in IDisplay, adjustment is required.\n",
            desiredSizeHint.width(), desiredSizeHint.height(), screenId()));
    sltPerformGuestResize(sizeToApply);
    /* And remember the size to know what we are resizing out of when we exit: */
    uisession()->setLastFullScreenSize(screenId(), scaledForward(desiredSizeHint));
}

QRect UIMachineViewSeamless::workingArea() const
{
    /* Get corresponding screen: */
    int iScreen = static_cast<UIMachineLogicSeamless*>(machineLogic())->hostScreenForGuestScreen(screenId());
    /* Return available geometry for that screen: */
    return gpDesktop->availableGeometry(iScreen);
}

QSize UIMachineViewSeamless::calculateMaxGuestSize() const
{
    return workingArea().size();
}
