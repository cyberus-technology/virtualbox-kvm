/* $Id: UIMachine.cpp $ */
/** @file
 * VBox Qt GUI - UIMachine class implementation.
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

/* GUI includes: */
#include "UICommon.h"
#include "UIExtraDataManager.h"
#include "UIMachine.h"
#include "UISession.h"
#include "UIActionPoolRuntime.h"
#include "UIMachineLogic.h"
#include "UIMachineWindow.h"
#include "UIMessageCenter.h"

/* COM includes: */
#include "CMachine.h"
#include "CSession.h"
#include "CConsole.h"
#include "CSnapshot.h"
#include "CProgress.h"


/* static */
UIMachine *UIMachine::m_spInstance = 0;

/* static */
bool UIMachine::startMachine(const QUuid &uID)
{
    /* Make sure machine is not created: */
    AssertReturn(!m_spInstance, false);

    /* Restore current snapshot if requested: */
    if (uiCommon().shouldRestoreCurrentSnapshot())
    {
        /* Create temporary session: */
        CSession session = uiCommon().openSession(uID, KLockType_VM);
        if (session.isNull())
            return false;

        /* Which VM we operate on? */
        CMachine machine = session.GetMachine();
        /* Which snapshot we are restoring? */
        CSnapshot snapshot = machine.GetCurrentSnapshot();

        /* Prepare restore-snapshot progress: */
        CProgress progress = machine.RestoreSnapshot(snapshot);
        if (!machine.isOk())
            return msgCenter().cannotRestoreSnapshot(machine, snapshot.GetName(), machine.GetName());

        /* Show the snapshot-discarding progress: */
        msgCenter().showModalProgressDialog(progress, machine.GetName(), ":/progress_snapshot_discard_90px.png");
        if (progress.GetResultCode() != 0)
            return msgCenter().cannotRestoreSnapshot(progress, snapshot.GetName(), machine.GetName());

        /* Unlock session finally: */
        session.UnlockMachine();

        /* Clear snapshot-restoring request: */
        uiCommon().setShouldRestoreCurrentSnapshot(false);
    }

    /* For separate process we should launch VM before UI: */
    if (uiCommon().isSeparateProcess())
    {
        /* Get corresponding machine: */
        CMachine machine = uiCommon().virtualBox().FindMachine(uiCommon().managedVMUuid().toString());
        AssertMsgReturn(!machine.isNull(), ("UICommon::managedVMUuid() should have filter that case before!\n"), false);

        /* Try to launch corresponding machine: */
        if (!UICommon::launchMachine(machine, UILaunchMode_Separate))
            return false;
    }

    /* Try to create machine UI: */
    return create();
}

/* static */
bool UIMachine::create()
{
    /* Make sure machine is not created: */
    AssertReturn(!m_spInstance, false);

    /* Create machine UI: */
    new UIMachine;
    /* Make sure it's prepared: */
    if (!m_spInstance->prepare())
    {
        /* Destroy machine UI otherwise: */
        destroy();
        /* False in that case: */
        return false;
    }
    /* True by default: */
    return true;
}

/* static */
void UIMachine::destroy()
{
    /* Make sure machine is created: */
    if (!m_spInstance)
        return;

    /* Protect versus recursive call: */
    UIMachine *pInstance = m_spInstance;
    m_spInstance = 0;
    /* Cleanup machine UI: */
    pInstance->cleanup();
    /* Destroy machine UI: */
    delete pInstance;
}

QWidget* UIMachine::activeWindow() const
{
    return   machineLogic() && machineLogic()->activeMachineWindow()
           ? machineLogic()->activeMachineWindow()
           : 0;
}

void UIMachine::asyncChangeVisualState(UIVisualStateType visualState)
{
    emit sigRequestAsyncVisualStateChange(visualState);
}

void UIMachine::setRequestedVisualState(UIVisualStateType visualStateType)
{
    /* Remember requested visual state: */
    m_enmRequestedVisualState = visualStateType;

    /* Save only if it's different from Invalid and from current one: */
    if (   m_enmRequestedVisualState != UIVisualStateType_Invalid
        && gEDataManager->requestedVisualState(uiCommon().managedVMUuid()) != m_enmRequestedVisualState)
        gEDataManager->setRequestedVisualState(m_enmRequestedVisualState, uiCommon().managedVMUuid());
}

UIVisualStateType UIMachine::requestedVisualState() const
{
    return m_enmRequestedVisualState;
}

void UIMachine::closeRuntimeUI()
{
    /* Quit application: */
    QApplication::quit();
}

void UIMachine::sltChangeVisualState(UIVisualStateType visualState)
{
    /* Create new machine-logic: */
    UIMachineLogic *pMachineLogic = UIMachineLogic::create(this, m_pSession, visualState);

    /* First we have to check if the selected machine-logic is available at all.
     * Only then we delete the old machine-logic and switch to the new one. */
    if (pMachineLogic->checkAvailability())
    {
        /* Delete previous machine-logic if exists: */
        if (m_pMachineLogic)
        {
            m_pMachineLogic->cleanup();
            UIMachineLogic::destroy(m_pMachineLogic);
            m_pMachineLogic = 0;
        }

        /* Set the new machine-logic as current one: */
        m_pMachineLogic = pMachineLogic;
        m_pMachineLogic->prepare();

        /* Remember new visual state: */
        m_visualState = visualState;

        /* Save requested visual state: */
        gEDataManager->setRequestedVisualState(m_visualState, uiCommon().managedVMUuid());
    }
    else
    {
        /* Delete temporary created machine-logic: */
        pMachineLogic->cleanup();
        UIMachineLogic::destroy(pMachineLogic);
    }

    /* Make sure machine-logic exists: */
    if (!m_pMachineLogic)
    {
        /* Reset initial visual state  to normal: */
        m_initialVisualState = UIVisualStateType_Normal;
        /* Enter initial visual state again: */
        enterInitialVisualState();
    }
}

UIMachine::UIMachine()
    : QObject(0)
    , m_pSession(0)
    , m_allowedVisualStates(UIVisualStateType_Invalid)
    , m_initialVisualState(UIVisualStateType_Normal)
    , m_visualState(UIVisualStateType_Invalid)
    , m_enmRequestedVisualState(UIVisualStateType_Invalid)
    , m_pMachineLogic(0)
{
    m_spInstance = this;
}

UIMachine::~UIMachine()
{
    m_spInstance = 0;
}

bool UIMachine::prepare()
{
    /* Try to prepare session UI: */
    if (!prepareSession())
        return false;

    /* Cache media data early if necessary: */
    if (uiCommon().agressiveCaching())
    {
        AssertReturn(m_pSession, false);
        uiCommon().enumerateMedia(m_pSession->machineMedia());
    }

    /* Prepare machine-logic: */
    prepareMachineLogic();

    /* Try to initialize session UI: */
    if (!uisession()->initialize())
        return false;

    /* True by default: */
    return true;
}

bool UIMachine::prepareSession()
{
    /* Try to create session UI: */
    if (!UISession::create(m_pSession, this))
        return false;

    /* True by default: */
    return true;
}

void UIMachine::prepareMachineLogic()
{
    /* Prepare async visual state type change handler: */
    qRegisterMetaType<UIVisualStateType>();
    connect(this, &UIMachine::sigRequestAsyncVisualStateChange,
            this, &UIMachine::sltChangeVisualState,
            Qt::QueuedConnection);

    /* Load restricted visual states: */
    UIVisualStateType restrictedVisualStates = gEDataManager->restrictedVisualStates(uiCommon().managedVMUuid());
    /* Acquire allowed visual states: */
    m_allowedVisualStates = static_cast<UIVisualStateType>(UIVisualStateType_All ^ restrictedVisualStates);

    /* Load requested visual state, it can override initial one: */
    m_enmRequestedVisualState = gEDataManager->requestedVisualState(uiCommon().managedVMUuid());
    /* Check if requested visual state is allowed: */
    if (isVisualStateAllowed(m_enmRequestedVisualState))
    {
        switch (m_enmRequestedVisualState)
        {
            /* Direct transition allowed to scale/fullscreen modes only: */
            case UIVisualStateType_Scale:      m_initialVisualState = UIVisualStateType_Scale; break;
            case UIVisualStateType_Fullscreen: m_initialVisualState = UIVisualStateType_Fullscreen; break;
            default: break;
        }
    }

    /* Enter initial visual state: */
    enterInitialVisualState();
}

void UIMachine::cleanupMachineLogic()
{
    /* Destroy machine-logic if exists: */
    if (m_pMachineLogic)
    {
        m_pMachineLogic->cleanup();
        UIMachineLogic::destroy(m_pMachineLogic);
        m_pMachineLogic = 0;
    }
}

void UIMachine::cleanupSession()
{
    /* Destroy session UI if exists: */
    if (uisession())
        UISession::destroy(m_pSession);
}

void UIMachine::cleanup()
{
    /* Preprocess all the meta-events: */
    QApplication::sendPostedEvents(0, QEvent::MetaCall);

    /* Cleanup machine-logic: */
    cleanupMachineLogic();

    /* Cleanup session UI: */
    cleanupSession();
}

void UIMachine::enterInitialVisualState()
{
    sltChangeVisualState(m_initialVisualState);
}
