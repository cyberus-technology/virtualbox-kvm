/* $Id: UIMachine.h $ */
/** @file
 * VBox Qt GUI - UIMachine class declaration.
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

#ifndef FEQT_INCLUDED_SRC_runtime_UIMachine_h
#define FEQT_INCLUDED_SRC_runtime_UIMachine_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QObject>

/* GUI includes: */
#include "UIExtraDataDefs.h"
#include "UIMachineDefs.h"

/* COM includes: */
#include "COMEnums.h"

/* Forward declarations: */
class QWidget;
class UISession;
class UIMachineLogic;

/** Singleton QObject extension
  * used as virtual machine (VM) singleton instance. */
class UIMachine : public QObject
{
    Q_OBJECT;

signals:

    /** Requests async visual-state change. */
    void sigRequestAsyncVisualStateChange(UIVisualStateType visualStateType);

public:

    /** Static factory to start machine with passed @a uID.
      * @return true if machine was started, false otherwise. */
    static bool startMachine(const QUuid &uID);
    /** Static constructor. */
    static bool create();
    /** Static destructor. */
    static void destroy();
    /** Static instance. */
    static UIMachine* instance() { return m_spInstance; }

    /** Returns session UI instance. */
    UISession *uisession() const { return m_pSession; }
    /** Returns machine-logic instance. */
    UIMachineLogic* machineLogic() const { return m_pMachineLogic; }
    /** Returns active machine-window reference (if possible). */
    QWidget* activeWindow() const;

    /** Returns whether requested visual @a state allowed. */
    bool isVisualStateAllowed(UIVisualStateType state) const { return m_allowedVisualStates & state; }

    /** Requests async visual-state change. */
    void asyncChangeVisualState(UIVisualStateType visualStateType);

    /** Requests visual-state to be entered when possible. */
    void setRequestedVisualState(UIVisualStateType visualStateType);
    /** Returns requested visual-state to be entered when possible. */
    UIVisualStateType requestedVisualState() const;

public slots:

    /** Closes Runtime UI. */
    void closeRuntimeUI();

private slots:

    /** Visual state-change handler. */
    void sltChangeVisualState(UIVisualStateType visualStateType);

private:

    /** Constructor. */
    UIMachine();
    /** Destructor. */
    ~UIMachine();

    /** Prepare routine. */
    bool prepare();
    /** Prepare routine: Session stuff. */
    bool prepareSession();
    /** Prepare routine: Machine-logic stuff. */
    void prepareMachineLogic();

    /** Cleanup routine: Machine-logic stuff. */
    void cleanupMachineLogic();
    /** Cleanup routine: Session stuff. */
    void cleanupSession();
    /** Cleanup routine. */
    void cleanup();

    /** Moves VM to initial state. */
    void enterInitialVisualState();

    /** Static instance. */
    static UIMachine* m_spInstance;

    /** Holds the session UI instance. */
    UISession *m_pSession;

    /** Holds allowed visual states. */
    UIVisualStateType m_allowedVisualStates;
    /** Holds initial visual state. */
    UIVisualStateType m_initialVisualState;
    /** Holds current visual state. */
    UIVisualStateType m_visualState;
    /** Holds visual state which should be entered when possible. */
    UIVisualStateType m_enmRequestedVisualState;
    /** Holds current machine-logic. */
    UIMachineLogic *m_pMachineLogic;
};

#define gpMachine UIMachine::instance()

#endif /* !FEQT_INCLUDED_SRC_runtime_UIMachine_h */
