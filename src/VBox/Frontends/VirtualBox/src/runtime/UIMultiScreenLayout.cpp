/* $Id: UIMultiScreenLayout.cpp $ */
/** @file
 * VBox Qt GUI - UIMultiScreenLayout class implementation.
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
#include <QMenu>

/* GUI includes: */
#include "UIDefs.h"
#include "UIMultiScreenLayout.h"
#include "UIActionPoolRuntime.h"
#include "UIMachineLogic.h"
#include "UIFrameBuffer.h"
#include "UISession.h"
#include "UIMessageCenter.h"
#include "UIExtraDataManager.h"
#include "UIDesktopWidgetWatchdog.h"
#include "UICommon.h"

/* COM includes: */
#include "COMEnums.h"
#include "CSession.h"
#include "CConsole.h"
#include "CMachine.h"
#include "CDisplay.h"
#include "CGraphicsAdapter.h"


UIMultiScreenLayout::UIMultiScreenLayout(UIMachineLogic *pMachineLogic)
    : m_pMachineLogic(pMachineLogic)
    , m_cGuestScreens(m_pMachineLogic->machine().GetGraphicsAdapter().GetMonitorCount())
    , m_cHostScreens(0)
{
    /* Calculate host/guest screen count: */
    calculateHostMonitorCount();
    calculateGuestScreenCount();

    /* Prpeare connections: */
    prepareConnections();
}

void UIMultiScreenLayout::update()
{
    LogRelFlow(("UIMultiScreenLayout::update: Started...\n"));

    /* Clear screen-map initially: */
    m_screenMap.clear();

    /* Make a pool of available host screens: */
    QList<int> availableScreens;
    for (int i = 0; i < m_cHostScreens; ++i)
        availableScreens << i;

    /* Load all combinations stored in the settings file.
     * We have to make sure they are valid, which means there have to be unique combinations
     * and all guests screens need there own host screen. */
    bool fShouldWeAutoMountGuestScreens = gEDataManager->autoMountGuestScreensEnabled(uiCommon().managedVMUuid());
    LogRel(("GUI: UIMultiScreenLayout::update: GUI/AutomountGuestScreens is %s\n", fShouldWeAutoMountGuestScreens ? "enabled" : "disabled"));
    foreach (int iGuestScreen, m_guestScreens)
    {
        /* Initialize variables: */
        bool fValid = false;
        int iHostScreen = -1;

        if (!fValid)
        {
            /* If the user ever selected a combination in the view menu, we have the following entry: */
            iHostScreen = gEDataManager->hostScreenForPassedGuestScreen(iGuestScreen, uiCommon().managedVMUuid());
            /* Revalidate: */
            fValid =    iHostScreen >= 0 && iHostScreen < m_cHostScreens /* In the host screen bounds? */
                     && m_screenMap.key(iHostScreen, -1) == -1; /* Not taken already? */
        }

        if (!fValid)
        {
            /* Check the position of the guest window in normal mode.
             * This makes sure that on first use fullscreen/seamless window opens on the same host-screen as the normal window was before.
             * This even works with multi-screen. The user just have to move all the normal windows to the target host-screens
             * and they will magically open there in fullscreen/seamless also. */
            QRect geo = gEDataManager->machineWindowGeometry(UIVisualStateType_Normal, iGuestScreen, uiCommon().managedVMUuid());
            /* If geometry is valid: */
            if (!geo.isNull())
            {
                /* Get top-left corner position: */
                QPoint topLeftPosition(geo.topLeft());
                /* Check which host-screen the position belongs to: */
                iHostScreen = UIDesktopWidgetWatchdog::screenNumber(topLeftPosition);
                /* Revalidate: */
                fValid =    iHostScreen >= 0 && iHostScreen < m_cHostScreens /* In the host screen bounds? */
                         && m_screenMap.key(iHostScreen, -1) == -1; /* Not taken already? */
            }
        }

        if (!fValid)
        {
            /* If still not valid, pick the next one
             * if there is still available host screen: */
            if (!availableScreens.isEmpty())
            {
                iHostScreen = availableScreens.first();
                fValid = true;
            }
        }

        if (fValid)
        {
            /* Register host screen for the guest screen: */
            m_screenMap.insert(iGuestScreen, iHostScreen);
            /* Remove it from the list of available host screens: */
            availableScreens.removeOne(iHostScreen);
        }
        /* Do we have opinion about what to do with excessive guest-screen? */
        else if (fShouldWeAutoMountGuestScreens)
        {
            /* Then we have to disable excessive guest-screen: */
            LogRel(("GUI: UIMultiScreenLayout::update: Disabling excessive guest-screen %d\n", iGuestScreen));
            m_pMachineLogic->uisession()->setScreenVisibleHostDesires(iGuestScreen, false);
            m_pMachineLogic->display().SetVideoModeHint(iGuestScreen, false, false, 0, 0, 0, 0, 0, true);
        }
    }

    /* Are we still have available host-screens
     * and have opinion about what to do with disabled guest-screens? */
    if (!availableScreens.isEmpty() && fShouldWeAutoMountGuestScreens)
    {
        /* How many excessive host-screens do we have? */
        int cExcessiveHostScreens = availableScreens.size();
        /* How many disabled guest-screens do we have? */
        int cDisabledGuestScreens = m_disabledGuestScreens.size();
        /* We have to try to enable disabled guest-screens if any: */
        int cGuestScreensToEnable = qMin(cExcessiveHostScreens, cDisabledGuestScreens);
        UISession *pSession = m_pMachineLogic->uisession();
        for (int iGuestScreenIndex = 0; iGuestScreenIndex < cGuestScreensToEnable; ++iGuestScreenIndex)
        {
            /* Defaults: */
            ULONG uWidth = 800;
            ULONG uHeight = 600;
            /* Try to get previous guest-screen arguments: */
            int iGuestScreen = m_disabledGuestScreens[iGuestScreenIndex];
            if (UIFrameBuffer *pFrameBuffer = pSession->frameBuffer(iGuestScreen))
            {
                if (pFrameBuffer->width() > 0)
                    uWidth = pFrameBuffer->width();
                if (pFrameBuffer->height() > 0)
                    uHeight = pFrameBuffer->height();
            }
            /* Re-enable guest-screen with proper resolution: */
            LogRel(("GUI: UIMultiScreenLayout::update: Enabling guest-screen %d with following resolution: %dx%d\n",
                    iGuestScreen, uWidth, uHeight));
            m_pMachineLogic->uisession()->setScreenVisibleHostDesires(iGuestScreen, true);
            m_pMachineLogic->display().SetVideoModeHint(iGuestScreen, true, false, 0, 0, uWidth, uHeight, 32, true);
        }
    }

    /* Make sure action-pool knows whether multi-screen layout has host-screen for guest-screen: */
    m_pMachineLogic->actionPool()->toRuntime()->setHostScreenForGuestScreenMap(m_screenMap);

    LogRelFlow(("UIMultiScreenLayout::update: Finished!\n"));
}

void UIMultiScreenLayout::rebuild()
{
    LogRelFlow(("UIMultiScreenLayout::rebuild: Started...\n"));

    /* Recalculate host/guest screen count: */
    calculateHostMonitorCount();
    calculateGuestScreenCount();
    /* Update layout: */
    update();

    LogRelFlow(("UIMultiScreenLayout::rebuild: Finished!\n"));
}

int UIMultiScreenLayout::hostScreenCount() const
{
    return m_cHostScreens;
}

int UIMultiScreenLayout::guestScreenCount() const
{
    return m_guestScreens.size();
}

int UIMultiScreenLayout::hostScreenForGuestScreen(int iScreenId) const
{
    return m_screenMap.value(iScreenId, 0);
}

bool UIMultiScreenLayout::hasHostScreenForGuestScreen(int iScreenId) const
{
    return m_screenMap.contains(iScreenId);
}

quint64 UIMultiScreenLayout::memoryRequirements() const
{
    return memoryRequirements(m_screenMap);
}

void UIMultiScreenLayout::sltHandleScreenLayoutChange(int iRequestedGuestScreen, int iRequestedHostScreen)
{
    /* Search for the virtual screen which is currently displayed on the
     * requested host screen. When there is one found, we swap both. */
    QMap<int,int> tmpMap(m_screenMap);
    int iCurrentGuestScreen = tmpMap.key(iRequestedHostScreen, -1);
    if (iCurrentGuestScreen != -1 && tmpMap.contains(iRequestedGuestScreen))
        tmpMap.insert(iCurrentGuestScreen, tmpMap.value(iRequestedGuestScreen));
    else
        tmpMap.remove(iCurrentGuestScreen);
    tmpMap.insert(iRequestedGuestScreen, iRequestedHostScreen);

    /* Check the memory requirements first: */
    bool fSuccess = true;
    if (m_pMachineLogic->uisession()->isGuestSupportsGraphics())
    {
        quint64 availBits = m_pMachineLogic->machine().GetGraphicsAdapter().GetVRAMSize() * _1M * 8;
        quint64 usedBits = memoryRequirements(tmpMap);
        fSuccess = availBits >= usedBits;
        if (!fSuccess)
        {
            /* We have too little video memory for the new layout, so say it to the user and revert all the changes: */
            if (m_pMachineLogic->visualStateType() == UIVisualStateType_Seamless)
                msgCenter().cannotSwitchScreenInSeamless((((usedBits + 7) / 8 + _1M - 1) / _1M) * _1M);
            else
                fSuccess = msgCenter().cannotSwitchScreenInFullscreen((((usedBits + 7) / 8 + _1M - 1) / _1M) * _1M);
        }
    }
    /* Make sure memory requirements matched: */
    if (!fSuccess)
        return;

    /* Swap the maps: */
    m_screenMap = tmpMap;

    /* Make sure action-pool knows whether multi-screen layout has host-screen for guest-screen: */
    m_pMachineLogic->actionPool()->toRuntime()->setHostScreenForGuestScreenMap(m_screenMap);

    /* Save guest-to-host mapping: */
    saveScreenMapping();

    /* Notifies about layout change: */
    emit sigScreenLayoutChange();
}

void UIMultiScreenLayout::calculateHostMonitorCount()
{
    m_cHostScreens = UIDesktopWidgetWatchdog::screenCount();
}

void UIMultiScreenLayout::calculateGuestScreenCount()
{
    /* Enumerate all the guest screens: */
    m_guestScreens.clear();
    m_disabledGuestScreens.clear();
    for (uint iGuestScreen = 0; iGuestScreen < m_cGuestScreens; ++iGuestScreen)
        if (m_pMachineLogic->uisession()->isScreenVisible(iGuestScreen))
            m_guestScreens << iGuestScreen;
        else
            m_disabledGuestScreens << iGuestScreen;
}

void UIMultiScreenLayout::prepareConnections()
{
    /* Connect action-pool: */
    connect(m_pMachineLogic->actionPool()->toRuntime(), &UIActionPoolRuntime::sigNotifyAboutTriggeringViewScreenRemap,
            this, &UIMultiScreenLayout::sltHandleScreenLayoutChange);
}

void UIMultiScreenLayout::saveScreenMapping()
{
    foreach (const int &iGuestScreen, m_guestScreens)
    {
        const int iHostScreen = m_screenMap.value(iGuestScreen, -1);
        gEDataManager->setHostScreenForPassedGuestScreen(iGuestScreen, iHostScreen, uiCommon().managedVMUuid());
    }
}

quint64 UIMultiScreenLayout::memoryRequirements(const QMap<int, int> &screenLayout) const
{
    ULONG width = 0;
    ULONG height = 0;
    ULONG guestBpp = 0;
    LONG xOrigin = 0;
    LONG yOrigin = 0;
    quint64 usedBits = 0;
    foreach (int iGuestScreen, m_guestScreens)
    {
        QRect screen;
        if (m_pMachineLogic->visualStateType() == UIVisualStateType_Seamless)
            screen = gpDesktop->availableGeometry(screenLayout.value(iGuestScreen, 0));
        else
            screen = gpDesktop->screenGeometry(screenLayout.value(iGuestScreen, 0));
        KGuestMonitorStatus monitorStatus = KGuestMonitorStatus_Enabled;
        m_pMachineLogic->display().GetScreenResolution(iGuestScreen, width, height, guestBpp, xOrigin, yOrigin, monitorStatus);
        usedBits += screen.width() * /* display width */
                    screen.height() * /* display height */
                    guestBpp + /* guest bits per pixel */
                    _1M * 8; /* current cache per screen - may be changed in future */
    }
    usedBits += 4096 * 8; /* adapter info */
    return usedBits;
}

