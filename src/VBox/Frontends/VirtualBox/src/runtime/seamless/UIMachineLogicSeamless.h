/* $Id: UIMachineLogicSeamless.h $ */
/** @file
 * VBox Qt GUI - UIMachineLogicSeamless class declaration.
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

#ifndef FEQT_INCLUDED_SRC_runtime_seamless_UIMachineLogicSeamless_h
#define FEQT_INCLUDED_SRC_runtime_seamless_UIMachineLogicSeamless_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIMachineLogic.h"

/* Forward declarations: */
class UIMultiScreenLayout;

/** UIMachineLogic subclass used as seamless machine logic implementation. */
class UIMachineLogicSeamless : public UIMachineLogic
{
    Q_OBJECT;

public:

    /** Constructs seamless logic passing @a pParent to the base-class.
      * @param  pSession  Brings the session UI reference. */
    UIMachineLogicSeamless(QObject *pParent, UISession *pSession);
    /** Destructs seamless logic. */
    virtual ~UIMachineLogicSeamless() RT_OVERRIDE;

    /** Returns an index of host-screen for guest-screen with @a iScreenId specified. */
    int hostScreenForGuestScreen(int iScreenId) const;
    /** Returns whether there is a host-screen for guest-screen with @a iScreenId specified. */
    bool hasHostScreenForGuestScreen(int iScreenId) const;

protected:

    /* Check if this logic is available: */
    bool checkAvailability();

    /** Returns machine-window flags for 'Seamless' machine-logic and passed @a uScreenId. */
    virtual Qt::WindowFlags windowFlags(ulong uScreenId) const { Q_UNUSED(uScreenId); return Qt::FramelessWindowHint; }

    /** Adjusts machine-window geometry if necessary for 'Seamless'. */
    virtual void adjustMachineWindowsGeometry();

private slots:

    /** Checks if some visual-state type was requested. */
    void sltCheckForRequestedVisualStateType();

    /* Handler: Console callback stuff: */
    void sltMachineStateChanged();

    /** Updates machine-window(s) location/size on screen-layout changes. */
    void sltScreenLayoutChanged();

    /** Handles guest-screen count change. */
    virtual void sltGuestMonitorChange(KGuestMonitorChangedEventType changeType, ulong uScreenId, QRect screenGeo);
    /** Handles host-screen count change. */
    virtual void sltHostScreenCountChange();
    /** Handles additions-state change. */
    virtual void sltAdditionsStateChanged();

#ifndef RT_OS_DARWIN
    /** Invokes popup-menu. */
    void sltInvokePopupMenu();
#endif /* !RT_OS_DARWIN */

private:

    /* Prepare helpers: */
    void prepareActionGroups();
    void prepareActionConnections();
    void prepareMachineWindows();
#ifndef VBOX_WS_MAC
    void prepareMenu();
#endif /* !VBOX_WS_MAC */

    /* Cleanup helpers: */
#ifndef VBOX_WS_MAC
    void cleanupMenu();
#endif /* !VBOX_WS_MAC */
    void cleanupMachineWindows();
    void cleanupActionConnections();
    void cleanupActionGroups();

    /* Variables: */
    UIMultiScreenLayout *m_pScreenLayout;

#ifndef RT_OS_DARWIN
    /** Holds the popup-menu instance. */
    QMenu *m_pPopupMenu;
#endif /* !RT_OS_DARWIN */
};

#endif /* !FEQT_INCLUDED_SRC_runtime_seamless_UIMachineLogicSeamless_h */
