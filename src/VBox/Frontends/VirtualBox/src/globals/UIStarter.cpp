/* $Id: UIStarter.cpp $ */
/** @file
 * VBox Qt GUI - UIStarter class implementation.
 */

/*
 * Copyright (C) 2018-2023 Oracle and/or its affiliates.
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

/* GUI includes: */
#include "UICommon.h"
#include "UIExtraDataManager.h"
#include "UIMessageCenter.h"
#include "UINotificationCenter.h"
#include "UIStarter.h"
#ifndef VBOX_RUNTIME_UI
# include "UIVirtualBoxManager.h"
#else
# include "UIMachine.h"
# include "UISession.h"
#endif


/* static */
UIStarter *UIStarter::s_pInstance = 0;

/* static */
void UIStarter::create()
{
    /* Pretect versus double 'new': */
    if (s_pInstance)
        return;

    /* Create instance: */
    new UIStarter;
}

/* static */
void UIStarter::destroy()
{
    /* Pretect versus double 'delete': */
    if (!s_pInstance)
        return;

    /* Destroy instance: */
    delete s_pInstance;
}

UIStarter::UIStarter()
{
    /* Assign instance: */
    s_pInstance = this;
}

UIStarter::~UIStarter()
{
    /* Unassign instance: */
    s_pInstance = 0;
}

void UIStarter::init()
{
    /* Listen for UICommon signals: */
    connect(&uiCommon(), &UICommon::sigAskToRestartUI,
            this, &UIStarter::sltRestartUI);
    connect(&uiCommon(), &UICommon::sigAskToCloseUI,
            this, &UIStarter::sltCloseUI);
}

void UIStarter::deinit()
{
    /* Listen for UICommon signals no more: */
    disconnect(&uiCommon(), &UICommon::sigAskToRestartUI,
               this, &UIStarter::sltRestartUI);
    disconnect(&uiCommon(), &UICommon::sigAskToCloseUI,
               this, &UIStarter::sltCloseUI);
}

void UIStarter::sltStartUI()
{
    /* Exit if UICommon is not valid: */
    if (!uiCommon().isValid())
        return;

#ifndef VBOX_RUNTIME_UI

    /* Make sure Selector UI is permitted, quit if not: */
    if (gEDataManager->guiFeatureEnabled(GUIFeatureType_NoSelector))
    {
        msgCenter().cannotStartSelector();
        return QApplication::quit();
    }

    /* Create/show manager-window: */
    UIVirtualBoxManager::create();

# ifdef VBOX_BLEEDING_EDGE
    /* Show EXPERIMENTAL BUILD warning: */
    UINotificationMessage::remindAboutExperimentalBuild();
# else /* !VBOX_BLEEDING_EDGE */
#  ifndef DEBUG
    /* Show BETA warning if necessary: */
    const QString vboxVersion(uiCommon().virtualBox().GetVersion());
    if (   vboxVersion.contains("BETA")
        && gEDataManager->preventBetaBuildWarningForVersion() != vboxVersion)
        UINotificationMessage::remindAboutBetaBuild();
#  endif /* !DEBUG */
# endif /* !VBOX_BLEEDING_EDGE */

#else /* VBOX_RUNTIME_UI */

    /* Make sure Runtime UI is even possible, quit if not: */
    if (uiCommon().managedVMUuid().isNull())
    {
        msgCenter().cannotStartRuntime();
        return QApplication::quit();
    }

    /* Make sure machine is started, quit if not: */
    if (!UIMachine::startMachine(uiCommon().managedVMUuid()))
        return QApplication::quit();

#endif /* VBOX_RUNTIME_UI */
}

void UIStarter::sltRestartUI()
{
#ifndef VBOX_RUNTIME_UI
    /* Recreate/show manager-window: */
    UIVirtualBoxManager::destroy();
    UIVirtualBoxManager::create();
#endif
}

void UIStarter::sltCloseUI()
{
#ifndef VBOX_RUNTIME_UI
    /* Destroy Manager UI: */
    if (gpManager)
        UIVirtualBoxManager::destroy();
#else
    /* Destroy Runtime UI: */
    if (gpMachine)
        UIMachine::destroy();
#endif
}
