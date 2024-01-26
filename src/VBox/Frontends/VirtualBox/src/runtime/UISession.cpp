/* $Id: UISession.cpp $ */
/** @file
 * VBox Qt GUI - UISession class implementation.
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
#include <QBitmap>
#include <QMenuBar>
#include <QWidget>
#ifdef VBOX_WS_MAC
# include <QTimer>
#endif
#ifdef VBOX_WS_WIN
# include <iprt/win/windows.h> /* Workaround for compile errors if included directly by QtWin. */
# include <QtWin>
#endif

/* GUI includes: */
#include "UICommon.h"
#include "UIDesktopWidgetWatchdog.h"
#include "UIExtraDataManager.h"
#include "UIIconPool.h"
#include "UISession.h"
#include "UIMachine.h"
#include "UIMedium.h"
#include "UIActionPoolRuntime.h"
#include "UIMachineLogic.h"
#include "UIMachineView.h"
#include "UIMachineWindow.h"
#include "UIMessageCenter.h"
#include "UIMousePointerShapeData.h"
#include "UINotificationCenter.h"
#include "UIConsoleEventHandler.h"
#include "UIFrameBuffer.h"
#include "UISettingsDialogSpecific.h"
#ifdef VBOX_WS_MAC
# include "UICocoaApplication.h"
# include "VBoxUtils-darwin.h"
#endif
#ifdef VBOX_GUI_WITH_KEYS_RESET_HANDLER
# include "UIKeyboardHandler.h"
# include <signal.h>
#endif

/* COM includes: */
#include "CAudioAdapter.h"
#include "CAudioSettings.h"
#include "CGraphicsAdapter.h"
#include "CHostUSBDevice.h"
#include "CRecordingSettings.h"
#include "CSystemProperties.h"
#include "CStorageController.h"
#include "CMediumAttachment.h"
#include "CNetworkAdapter.h"
#include "CHostNetworkInterface.h"
#include "CVRDEServer.h"
#include "CUSBController.h"
#include "CUSBDeviceFilter.h"
#include "CUSBDeviceFilters.h"
#include "CHostVideoInputDevice.h"
#include "CSnapshot.h"
#include "CMedium.h"

/* External includes: */
#ifdef VBOX_WS_X11
# include <X11/Xlib.h>
# include <X11/Xutil.h>
#endif


#ifdef VBOX_GUI_WITH_KEYS_RESET_HANDLER
static void signalHandlerSIGUSR1(int sig, siginfo_t *, void *);
#endif

#ifdef VBOX_WS_MAC
/**
 * MacOS X: Application Services: Core Graphics: Display reconfiguration callback.
 *
 * Notifies UISession about @a display configuration change.
 * Corresponding change described by Core Graphics @a flags.
 * Uses UISession @a pHandler to process this change.
 *
 * @note Last argument (@a pHandler) must always be valid pointer to UISession object.
 * @note Calls for UISession::sltHandleHostDisplayAboutToChange() slot if display configuration changed.
 */
void cgDisplayReconfigurationCallback(CGDirectDisplayID display, CGDisplayChangeSummaryFlags flags, void *pHandler)
{
    /* Which flags we are handling? */
    int iHandledFlags = kCGDisplayAddFlag     /* display added */
                      | kCGDisplayRemoveFlag  /* display removed */
                      | kCGDisplaySetModeFlag /* display mode changed */;

    /* Handle 'display-add' case: */
    if (flags & kCGDisplayAddFlag)
        LogRelFlow(("GUI: UISession::cgDisplayReconfigurationCallback: Display added.\n"));
    /* Handle 'display-remove' case: */
    else if (flags & kCGDisplayRemoveFlag)
        LogRelFlow(("GUI: UISession::cgDisplayReconfigurationCallback: Display removed.\n"));
    /* Handle 'mode-set' case: */
    else if (flags & kCGDisplaySetModeFlag)
        LogRelFlow(("GUI: UISession::cgDisplayReconfigurationCallback: Display mode changed.\n"));

    /* Ask handler to process our callback: */
    if (flags & iHandledFlags)
        QTimer::singleShot(0, static_cast<UISession*>(pHandler),
                           SLOT(sltHandleHostDisplayAboutToChange()));

    Q_UNUSED(display);
}
#endif /* VBOX_WS_MAC */

/* static */
bool UISession::create(UISession *&pSession, UIMachine *pMachine)
{
    /* Make sure null pointer passed: */
    AssertReturn(pSession == 0, false);

    /* Create session UI: */
    pSession = new UISession(pMachine);
    /* Make sure it's prepared: */
    if (!pSession->prepare())
    {
        /* Destroy session UI otherwise: */
        destroy(pSession);
        /* False in that case: */
        return false;
    }
    /* True by default: */
    return true;
}

/* static */
void UISession::destroy(UISession *&pSession)
{
    /* Make sure valid pointer passed: */
    AssertReturnVoid(pSession != 0);

    /* Cleanup session UI: */
    pSession->cleanup();
    /* Destroy session: */
    delete pSession;
    pSession = 0;
}

bool UISession::initialize()
{
    /* Preprocess initialization: */
    if (!preprocessInitialization())
        return false;

    /* Notify user about mouse&keyboard auto-capturing: */
    if (gEDataManager->autoCaptureEnabled())
        UINotificationMessage::remindAboutAutoCapture();

    m_machineState = machine().GetState();

    /* Apply debug settings from the command line. */
    if (!debugger().isNull() && debugger().isOk())
    {
        if (uiCommon().areWeToExecuteAllInIem())
            debugger().SetExecuteAllInIEM(true);
        if (!uiCommon().isDefaultWarpPct())
            debugger().SetVirtualTimeRate(uiCommon().getWarpPct());
    }

    /* Apply ad-hoc reconfigurations from the command line: */
    if (uiCommon().hasFloppyImageToMount())
        mountAdHocImage(KDeviceType_Floppy, UIMediumDeviceType_Floppy, uiCommon().getFloppyImage().toString());
    if (uiCommon().hasDvdImageToMount())
        mountAdHocImage(KDeviceType_DVD, UIMediumDeviceType_DVD, uiCommon().getDvdImage().toString());

    /* Power UP if this is NOT separate process: */
    if (!uiCommon().isSeparateProcess())
        if (!powerUp())
            return false;

    /* Make sure all the pending Console events converted to signals
     * during the powerUp() progress above reached their destinations.
     * That is necessary to make sure all the pending machine state change events processed.
     * We can't just use the machine state directly acquired from IMachine because there
     * will be few places which are using stale machine state, not just this one. */
    QApplication::sendPostedEvents(0, QEvent::MetaCall);

    /* Check if we missed a really quick termination after successful startup: */
    if (isTurnedOff())
    {
        LogRel(("GUI: Aborting startup due to invalid machine state detected: %d\n", machineState()));
        return false;
    }

    /* Postprocess initialization: */
    if (!postprocessInitialization())
        return false;

    /* Fetch corresponding states: */
    if (uiCommon().isSeparateProcess())
    {
        m_fIsMouseSupportsAbsolute = mouse().GetAbsoluteSupported();
        m_fIsMouseSupportsRelative = mouse().GetRelativeSupported();
        m_fIsMouseSupportsTouchScreen = mouse().GetTouchScreenSupported();
        m_fIsMouseSupportsTouchPad = mouse().GetTouchPadSupported();
        m_fIsMouseHostCursorNeeded = mouse().GetNeedsHostCursor();
        sltAdditionsChange();
    }
    machineLogic()->initializePostPowerUp();

    /* Load VM settings: */
    loadVMSettings();

/* Log whether HID LEDs sync is enabled: */
#if defined(VBOX_WS_MAC) || defined(VBOX_WS_WIN)
    LogRel(("GUI: HID LEDs sync is %s\n",
            uimachine()->machineLogic()->isHidLedsSyncEnabled()
            ? "enabled" : "disabled"));
#else /* !VBOX_WS_MAC && !VBOX_WS_WIN */
    LogRel(("GUI: HID LEDs sync is not supported on this platform\n"));
#endif /* !VBOX_WS_MAC && !VBOX_WS_WIN */

#ifdef VBOX_GUI_WITH_PIDFILE
    uiCommon().createPidfile();
#endif /* VBOX_GUI_WITH_PIDFILE */

    /* Warn listeners about we are initialized: */
    m_fInitialized = true;
    emit sigInitialized();

    /* True by default: */
    return true;
}

bool UISession::powerUp()
{
    /* Power UP machine: */
    CProgress progress = uiCommon().shouldStartPaused() ? console().PowerUpPaused() : console().PowerUp();

    /* Check for immediate failure: */
    if (!console().isOk() || progress.isNull())
    {
        if (uiCommon().showStartVMErrors())
            msgCenter().cannotStartMachine(console(), machineName());
        LogRel(("GUI: Aborting startup due to power up issue detected...\n"));
        return false;
    }

    /* Some logging right after we powered up: */
    LogRel(("Qt version: %s\n", UICommon::qtRTVersionString().toUtf8().constData()));
#ifdef VBOX_WS_X11
    LogRel(("X11 Window Manager code: %d\n", (int)uiCommon().typeOfWindowManager()));
#endif

    /* Enable 'manual-override',
     * preventing automatic Runtime UI closing
     * and visual representation mode changes: */
    setManualOverrideMode(true);

    /* Show "Starting/Restoring" progress dialog: */
    if (isSaved())
    {
        msgCenter().showModalProgressDialog(progress, machineName(), ":/progress_state_restore_90px.png", 0, 0);
        /* After restoring from 'saved' state, machine-window(s) geometry should be adjusted: */
        machineLogic()->adjustMachineWindowsGeometry();
    }
    else
    {
#ifdef VBOX_IS_QT6_OR_LATER /** @todo why is this any problem on qt6? */
        msgCenter().showModalProgressDialog(progress, machineName(), ":/progress_start_90px.png", 0, 0);
#else
        msgCenter().showModalProgressDialog(progress, machineName(), ":/progress_start_90px.png");
#endif
        /* After VM start, machine-window(s) size-hint(s) should be sent: */
        machineLogic()->sendMachineWindowsSizeHints();
    }

    /* Check for progress failure: */
    if (!progress.isOk() || progress.GetResultCode() != 0)
    {
        if (uiCommon().showStartVMErrors())
            msgCenter().cannotStartMachine(progress, machineName());
        LogRel(("GUI: Aborting startup due to power up progress issue detected...\n"));
        return false;
    }

    /* Disable 'manual-override' finally: */
    setManualOverrideMode(false);

    /* True by default: */
    return true;
}

void UISession::detachUi()
{
    /* Enable 'manual-override',
     * preventing automatic Runtime UI closing: */
    setManualOverrideMode(true);

    /* Manually close Runtime UI: */
    LogRel(("GUI: Detaching UI..\n"));
    closeRuntimeUI();
}

void UISession::saveState()
{
    /* Saving state? */
    bool fSaveState = true;

    /* If VM is not paused, we should pause it first: */
    if (!isPaused())
        fSaveState = pause();

    /* Save state: */
    if (fSaveState)
    {
        /* Enable 'manual-override',
         * preventing automatic Runtime UI closing: */
        setManualOverrideMode(true);

        /* Now, do the magic: */
        LogRel(("GUI: Saving VM state..\n"));
        UINotificationProgressMachineSaveState *pNotification = new UINotificationProgressMachineSaveState(machine());
        connect(pNotification, &UINotificationProgressMachineSaveState::sigMachineStateSaved,
                this, &UISession::sltHandleMachineStateSaved);
        gpNotificationCenter->append(pNotification);
    }
}

void UISession::shutdown()
{
    /* Warn the user about ACPI is not available if so: */
    if (!console().GetGuestEnteredACPIMode())
        return UINotificationMessage::cannotSendACPIToMachine();

    /* Send ACPI shutdown signal if possible: */
    LogRel(("GUI: Sending ACPI shutdown signal..\n"));
    console().PowerButton();
    if (!console().isOk())
        UINotificationMessage::cannotACPIShutdownMachine(console());
}

void UISession::powerOff(bool fIncludingDiscard)
{
    /* Enable 'manual-override',
     * preventing automatic Runtime UI closing: */
    setManualOverrideMode(true);

    /* Now, do the magic: */
    LogRel(("GUI: Powering VM off..\n"));
    UINotificationProgressMachinePowerOff *pNotification =
        new UINotificationProgressMachinePowerOff(machine(), console(), fIncludingDiscard);
    connect(pNotification, &UINotificationProgressMachinePowerOff::sigMachinePoweredOff,
            this, &UISession::sltHandleMachinePoweredOff);
    gpNotificationCenter->append(pNotification);
}

UIMachineLogic* UISession::machineLogic() const
{
    return uimachine() ? uimachine()->machineLogic() : 0;
}

QWidget* UISession::mainMachineWindow() const
{
    return machineLogic() ? machineLogic()->mainMachineWindow() : 0;
}

WId UISession::mainMachineWindowId() const
{
    return mainMachineWindow()->winId();
}

UIMachineWindow *UISession::activeMachineWindow() const
{
    return machineLogic() ? machineLogic()->activeMachineWindow() : 0;
}

bool UISession::isVisualStateAllowed(UIVisualStateType state) const
{
    return m_pMachine->isVisualStateAllowed(state);
}

void UISession::changeVisualState(UIVisualStateType visualStateType)
{
    m_pMachine->asyncChangeVisualState(visualStateType);
}

void UISession::setRequestedVisualState(UIVisualStateType visualStateType)
{
    m_pMachine->setRequestedVisualState(visualStateType);
}

UIVisualStateType UISession::requestedVisualState() const
{
    return m_pMachine->requestedVisualState();
}

bool UISession::setPause(bool fOn)
{
    if (fOn)
        console().Pause();
    else
        console().Resume();

    bool ok = console().isOk();
    if (!ok)
    {
        if (fOn)
            UINotificationMessage::cannotPauseMachine(console());
        else
            UINotificationMessage::cannotResumeMachine(console());
    }

    return ok;
}

void UISession::sltInstallGuestAdditionsFrom(const QString &strSource)
{
    if (!guestAdditionsUpgradable())
        return sltMountDVDAdHoc(strSource);

    /* Update guest additions automatically: */
    UINotificationProgressGuestAdditionsInstall *pNotification =
            new UINotificationProgressGuestAdditionsInstall(guest(), strSource);
    connect(pNotification, &UINotificationProgressGuestAdditionsInstall::sigGuestAdditionsInstallationFailed,
            this, &UISession::sltMountDVDAdHoc);
    gpNotificationCenter->append(pNotification);
}

void UISession::sltMountDVDAdHoc(const QString &strSource)
{
    mountAdHocImage(KDeviceType_DVD, UIMediumDeviceType_DVD, strSource);
}

void UISession::closeRuntimeUI()
{
    /* First, we have to hide any opened modal/popup widgets.
     * They then should unlock their event-loops asynchronously.
     * If all such loops are unlocked, we can close Runtime UI. */
    QWidget *pWidget = QApplication::activeModalWidget()
                     ? QApplication::activeModalWidget()
                     : QApplication::activePopupWidget()
                     ? QApplication::activePopupWidget()
                     : 0;
    if (pWidget)
    {
        /* First we should try to close this widget: */
        pWidget->close();
        /* If widget rejected the 'close-event' we can
         * still hide it and hope it will behave correctly
         * and unlock his event-loop if any: */
        if (!pWidget->isHidden())
            pWidget->hide();
        /* Asynchronously restart this slot: */
        QMetaObject::invokeMethod(this, "closeRuntimeUI", Qt::QueuedConnection);
        return;
    }

    /* Asynchronously ask UIMachine to close Runtime UI: */
    LogRel(("GUI: Passing request to close Runtime UI from UI session to UI machine.\n"));
    QMetaObject::invokeMethod(uimachine(), "closeRuntimeUI", Qt::QueuedConnection);
}

void UISession::sltDetachCOM()
{
    /* Cleanup everything COM related: */
    cleanupFramebuffers();
    cleanupConsoleEventHandlers();
    cleanupNotificationCenter();
    cleanupSession();
}

#ifdef RT_OS_DARWIN
void UISession::sltHandleMenuBarConfigurationChange(const QUuid &uMachineID)
{
    /* Skip unrelated machine IDs: */
    if (uiCommon().managedVMUuid() != uMachineID)
        return;

    /* Update Mac OS X menu-bar: */
    updateMenu();
}
#endif /* RT_OS_DARWIN */

void UISession::sltMousePointerShapeChange(const UIMousePointerShapeData &shapeData)
{
    /* In case if shape itself is present: */
    if (shapeData.shape().size() > 0)
    {
        /* We are ignoring visibility flag: */
        m_fIsHidingHostPointer = false;

        /* And updating current shape data: */
        m_shapeData = shapeData;
        updateMousePointerShape();
    }
    /* In case if shape itself is NOT present: */
    else
    {
        /* Remember if we should hide the cursor: */
        m_fIsHidingHostPointer = !shapeData.isVisible();
    }

    /* Notify listeners about mouse capability changed: */
    emit sigMousePointerShapeChange();
}

void UISession::sltMouseCapabilityChange(bool fSupportsAbsolute, bool fSupportsRelative,
                                         bool fSupportsTouchScreen, bool fSupportsTouchPad,
                                         bool fNeedsHostCursor)
{
    LogRelFlow(("GUI: UISession::sltMouseCapabilityChange: "
                "Supports absolute: %s, Supports relative: %s, "
                "Supports touchscreen: %s, Supports touchpad: %s, "
                "Needs host cursor: %s\n",
                fSupportsAbsolute ? "TRUE" : "FALSE", fSupportsRelative ? "TRUE" : "FALSE",
                fSupportsTouchScreen ? "TRUE" : "FALSE", fSupportsTouchPad ? "TRUE" : "FALSE",
                fNeedsHostCursor ? "TRUE" : "FALSE"));

    /* Check if something had changed: */
    if (   m_fIsMouseSupportsAbsolute != fSupportsAbsolute
        || m_fIsMouseSupportsRelative != fSupportsRelative
        || m_fIsMouseSupportsTouchScreen != fSupportsTouchScreen
        || m_fIsMouseSupportsTouchPad != fSupportsTouchPad
        || m_fIsMouseHostCursorNeeded != fNeedsHostCursor)
    {
        /* Store new data: */
        m_fIsMouseSupportsAbsolute = fSupportsAbsolute;
        m_fIsMouseSupportsRelative = fSupportsRelative;
        m_fIsMouseSupportsTouchScreen = fSupportsTouchScreen;
        m_fIsMouseSupportsTouchPad = fSupportsTouchPad;
        m_fIsMouseHostCursorNeeded = fNeedsHostCursor;

        /* Notify listeners about mouse capability changed: */
        emit sigMouseCapabilityChange();
    }
}

void UISession::sltCursorPositionChange(bool fContainsData, unsigned long uX, unsigned long uY)
{
    LogRelFlow(("GUI: UISession::sltCursorPositionChange: "
                "Cursor position valid: %d, Cursor position: %dx%d\n",
                fContainsData ? "TRUE" : "FALSE", uX, uY));

    /* Check if something had changed: */
    if (   m_fIsValidCursorPositionPresent != fContainsData
        || m_cursorPosition.x() != (int)uX
        || m_cursorPosition.y() != (int)uY)
    {
        /* Store new data: */
        m_fIsValidCursorPositionPresent = fContainsData;
        m_cursorPosition = QPoint(uX, uY);

        /* Notify listeners about cursor position changed: */
        emit sigCursorPositionChange();
    }
}

void UISession::sltKeyboardLedsChangeEvent(bool fNumLock, bool fCapsLock, bool fScrollLock)
{
    /* Check if something had changed: */
    if (   m_fNumLock != fNumLock
        || m_fCapsLock != fCapsLock
        || m_fScrollLock != fScrollLock)
    {
        /* Store new num lock data: */
        if (m_fNumLock != fNumLock)
        {
            m_fNumLock = fNumLock;
            m_uNumLockAdaptionCnt = 2;
        }

        /* Store new caps lock data: */
        if (m_fCapsLock != fCapsLock)
        {
            m_fCapsLock = fCapsLock;
            m_uCapsLockAdaptionCnt = 2;
        }

        /* Store new scroll lock data: */
        if (m_fScrollLock != fScrollLock)
        {
            m_fScrollLock = fScrollLock;
        }

        /* Notify listeners about mouse capability changed: */
        emit sigKeyboardLedsChange();
    }
}

void UISession::sltStateChange(KMachineState state)
{
    /* Check if something had changed: */
    if (m_machineState != state)
    {
        /* Store new data: */
        m_machineStatePrevious = m_machineState;
        m_machineState = state;

        /* Notify listeners about machine state changed: */
        emit sigMachineStateChange();
    }
}

void UISession::sltVRDEChange()
{
    /* Make sure VRDE server is present: */
    const CVRDEServer server = machine().GetVRDEServer();
    AssertMsgReturnVoid(machine().isOk() && !server.isNull(),
                        ("VRDE server should NOT be null!\n"));

    /* Check/Uncheck VRDE Server action depending on feature status: */
    actionPool()->action(UIActionIndexRT_M_View_T_VRDEServer)->blockSignals(true);
    actionPool()->action(UIActionIndexRT_M_View_T_VRDEServer)->setChecked(server.GetEnabled());
    actionPool()->action(UIActionIndexRT_M_View_T_VRDEServer)->blockSignals(false);

    /* Notify listeners about VRDE change: */
    emit sigVRDEChange();
}

void UISession::sltRecordingChange()
{
    CRecordingSettings comRecordingSettings = machine().GetRecordingSettings();

    /* Check/Uncheck Capture action depending on feature status: */
    actionPool()->action(UIActionIndexRT_M_View_M_Recording_T_Start)->blockSignals(true);
    actionPool()->action(UIActionIndexRT_M_View_M_Recording_T_Start)->setChecked(comRecordingSettings.GetEnabled());
    actionPool()->action(UIActionIndexRT_M_View_M_Recording_T_Start)->blockSignals(false);

    /* Notify listeners about Recording change: */
    emit sigRecordingChange();
}

void UISession::sltGuestMonitorChange(KGuestMonitorChangedEventType changeType, ulong uScreenId, QRect screenGeo)
{
    /* Ignore KGuestMonitorChangedEventType_NewOrigin change event: */
    if (changeType == KGuestMonitorChangedEventType_NewOrigin)
        return;
    /* Ignore KGuestMonitorChangedEventType_Disabled event for primary screen: */
    AssertMsg(countOfVisibleWindows() > 0, ("All machine windows are hidden!"));
    if (changeType == KGuestMonitorChangedEventType_Disabled && uScreenId == 0)
        return;

    /* Process KGuestMonitorChangedEventType_Enabled change event: */
    if (   !isScreenVisible(uScreenId)
        && changeType == KGuestMonitorChangedEventType_Enabled)
        setScreenVisible(uScreenId, true);
    /* Process KGuestMonitorChangedEventType_Disabled change event: */
    else if (   isScreenVisible(uScreenId)
             && changeType == KGuestMonitorChangedEventType_Disabled)
        setScreenVisible(uScreenId, false);

    /* Notify listeners about the change: */
    emit sigGuestMonitorChange(changeType, uScreenId, screenGeo);
}

void UISession::sltHandleStorageDeviceChange(const CMediumAttachment &attachment, bool fRemoved, bool fSilent)
{
    /* Update action restrictions: */
    updateActionRestrictions();

    /* Notify listeners about storage device change: */
    emit sigStorageDeviceChange(attachment, fRemoved, fSilent);
}

void UISession::sltAudioAdapterChange()
{
    /* Make sure Audio adapter is present: */
    const CAudioSettings comAudioSettings = machine().GetAudioSettings();
    const CAudioAdapter  comAdapter  = comAudioSettings.GetAdapter();
    AssertMsgReturnVoid(machine().isOk() && comAdapter.isNotNull(),
                        ("Audio adapter should NOT be null!\n"));

    /* Check/Uncheck Audio adapter output/input actions depending on features status: */
    actionPool()->action(UIActionIndexRT_M_Devices_M_Audio_T_Output)->blockSignals(true);
    actionPool()->action(UIActionIndexRT_M_Devices_M_Audio_T_Output)->setChecked(comAdapter.GetEnabledOut());
    actionPool()->action(UIActionIndexRT_M_Devices_M_Audio_T_Output)->blockSignals(false);
    actionPool()->action(UIActionIndexRT_M_Devices_M_Audio_T_Input)->blockSignals(true);
    actionPool()->action(UIActionIndexRT_M_Devices_M_Audio_T_Input)->setChecked(comAdapter.GetEnabledIn());
    actionPool()->action(UIActionIndexRT_M_Devices_M_Audio_T_Input)->blockSignals(false);

    /* Notify listeners about Audio adapter change: */
    emit sigAudioAdapterChange();

}

void UISession::sltClipboardModeChange(KClipboardMode enmMode)
{
    emit sigClipboardModeChange(enmMode);
}

void UISession::sltDnDModeChange(KDnDMode enmMode)
{
    emit sigDnDModeChange(enmMode);
}

#ifdef RT_OS_DARWIN
/**
 * MacOS X: Restarts display-reconfiguration watchdog timer from the beginning.
 * @note Watchdog is trying to determine display reconfiguration in
 *       UISession::sltCheckIfHostDisplayChanged() slot every 500ms for 40 tries.
 */
void UISession::sltHandleHostDisplayAboutToChange()
{
    LogRelFlow(("GUI: UISession::sltHandleHostDisplayAboutToChange()\n"));

    if (m_pWatchdogDisplayChange->isActive())
        m_pWatchdogDisplayChange->stop();
    m_pWatchdogDisplayChange->setProperty("tryNumber", 1);
    m_pWatchdogDisplayChange->start();
}

/**
 * MacOS X: Determines display reconfiguration.
 * @note Calls for UISession::sltHandleHostScreenCountChange() if screen count changed.
 * @note Calls for UISession::sltHandleHostScreenGeometryChange() if screen geometry changed.
 */
void UISession::sltCheckIfHostDisplayChanged()
{
    LogRelFlow(("GUI: UISession::sltCheckIfHostDisplayChanged()\n"));

    /* Check if display count changed: */
    if (UIDesktopWidgetWatchdog::screenCount() != m_hostScreens.size())
    {
        /* Reset watchdog: */
        m_pWatchdogDisplayChange->setProperty("tryNumber", 0);
        /* Notify listeners about screen-count changed: */
        return sltHandleHostScreenCountChange();
    }
    else
    {
        /* Check if at least one display geometry changed: */
        for (int iScreenIndex = 0; iScreenIndex < UIDesktopWidgetWatchdog::screenCount(); ++iScreenIndex)
        {
            if (gpDesktop->screenGeometry(iScreenIndex) != m_hostScreens.at(iScreenIndex))
            {
                /* Reset watchdog: */
                m_pWatchdogDisplayChange->setProperty("tryNumber", 0);
                /* Notify listeners about screen-geometry changed: */
                return sltHandleHostScreenGeometryChange();
            }
        }
    }

    /* Check if watchdog expired, restart if not: */
    int cTryNumber = m_pWatchdogDisplayChange->property("tryNumber").toInt();
    if (cTryNumber > 0 && cTryNumber < 40)
    {
        /* Restart watchdog again: */
        m_pWatchdogDisplayChange->setProperty("tryNumber", ++cTryNumber);
        m_pWatchdogDisplayChange->start();
    }
    else
    {
        /* Reset watchdog: */
        m_pWatchdogDisplayChange->setProperty("tryNumber", 0);
    }
}
#endif /* RT_OS_DARWIN */

void UISession::sltHandleHostScreenCountChange()
{
    LogRelFlow(("GUI: UISession: Host-screen count changed.\n"));

    /* Recache display data: */
    updateHostScreenData();

    /* Notify current machine-logic: */
    emit sigHostScreenCountChange();
}

void UISession::sltHandleHostScreenGeometryChange()
{
    LogRelFlow(("GUI: UISession: Host-screen geometry changed.\n"));

    /* Recache display data: */
    updateHostScreenData();

    /* Notify current machine-logic: */
    emit sigHostScreenGeometryChange();
}

void UISession::sltHandleHostScreenAvailableAreaChange()
{
    LogRelFlow(("GUI: UISession: Host-screen available-area changed.\n"));

    /* Notify current machine-logic: */
    emit sigHostScreenAvailableAreaChange();
}

void UISession::sltHandleMachineStateSaved(bool fSuccess)
{
    /* Disable 'manual-override' finally: */
    setManualOverrideMode(false);

    /* Close Runtime UI if state was saved: */
    if (fSuccess)
        closeRuntimeUI();
}

void UISession::sltHandleMachinePoweredOff(bool fSuccess, bool fIncludingDiscard)
{
    /* Disable 'manual-override' finally: */
    setManualOverrideMode(false);

    /* Do we have other tasks? */
    if (fSuccess)
    {
        if (!fIncludingDiscard)
            closeRuntimeUI();
        else
        {
            /* Now, do more magic! */
            UINotificationProgressSnapshotRestore *pNotification =
                new UINotificationProgressSnapshotRestore(uiCommon().managedVMUuid());
            connect(pNotification, &UINotificationProgressSnapshotRestore::sigSnapshotRestored,
                    this, &UISession::sltHandleSnapshotRestored);
            gpNotificationCenter->append(pNotification);
        }
    }
}

void UISession::sltHandleSnapshotRestored(bool)
{
    /* Close Runtime UI independent of snapshot restoring state: */
    closeRuntimeUI();
}

void UISession::sltAdditionsChange()
{
    /* Variable flags: */
    ULONG ulGuestAdditionsRunLevel = guest().GetAdditionsRunLevel();
    LONG64 lLastUpdatedIgnored;
    bool fIsGuestSupportsGraphics = guest().GetFacilityStatus(KAdditionsFacilityType_Graphics, lLastUpdatedIgnored)
                                    == KAdditionsFacilityStatus_Active;
    bool fIsGuestSupportsSeamless = guest().GetFacilityStatus(KAdditionsFacilityType_Seamless, lLastUpdatedIgnored)
                                    == KAdditionsFacilityStatus_Active;
    /* Check if something had changed: */
    if (m_ulGuestAdditionsRunLevel != ulGuestAdditionsRunLevel ||
        m_fIsGuestSupportsGraphics != fIsGuestSupportsGraphics ||
        m_fIsGuestSupportsSeamless != fIsGuestSupportsSeamless)
    {
        /* Store new data: */
        m_ulGuestAdditionsRunLevel = ulGuestAdditionsRunLevel;
        m_fIsGuestSupportsGraphics = fIsGuestSupportsGraphics;
        m_fIsGuestSupportsSeamless = fIsGuestSupportsSeamless;

        /* Make sure action-pool knows whether GA supports graphics: */
        actionPool()->toRuntime()->setGuestSupportsGraphics(m_fIsGuestSupportsGraphics);

        if (actionPool()->action(UIActionIndexRT_M_Devices_S_UpgradeGuestAdditions))
            actionPool()->action(UIActionIndexRT_M_Devices_S_UpgradeGuestAdditions)->setEnabled(guestAdditionsUpgradable());

        /* Notify listeners about GA state really changed: */
        LogRel(("GUI: UISession::sltAdditionsChange: GA state really changed, notifying listeners\n"));
        emit sigAdditionsStateActualChange();
    }

    /* Notify listeners about GA state change event came: */
    LogRel(("GUI: UISession::sltAdditionsChange: GA state change event came, notifying listeners\n"));
    emit sigAdditionsStateChange();
}

UISession::UISession(UIMachine *pMachine)
    : QObject(pMachine)
    /* Base variables: */
    , m_pMachine(pMachine)
    , m_pActionPool(0)
#ifdef VBOX_WS_MAC
    , m_pMenuBar(0)
#endif /* VBOX_WS_MAC */
    /* Common variables: */
    , m_machineStatePrevious(KMachineState_Null)
    , m_machineState(KMachineState_Null)
    , m_pMachineWindowIcon(0)
#ifdef VBOX_WS_MAC
    , m_pWatchdogDisplayChange(0)
#endif /* VBOX_WS_MAC */
    , m_defaultCloseAction(MachineCloseAction_Invalid)
    , m_restrictedCloseActions(MachineCloseAction_Invalid)
    , m_fAllCloseActionsRestricted(false)
    /* Common flags: */
    , m_fInitialized(false)
    , m_fIsGuestResizeIgnored(false)
    , m_fIsAutoCaptureDisabled(false)
    , m_fIsManualOverride(false)
    /* Guest additions flags: */
    , m_ulGuestAdditionsRunLevel(0)
    , m_fIsGuestSupportsGraphics(false)
    , m_fIsGuestSupportsSeamless(false)
    /* Mouse flags: */
    , m_fNumLock(false)
    , m_fCapsLock(false)
    , m_fScrollLock(false)
    , m_uNumLockAdaptionCnt(2)
    , m_uCapsLockAdaptionCnt(2)
    /* Mouse flags: */
    , m_fIsMouseSupportsAbsolute(false)
    , m_fIsMouseSupportsRelative(false)
    , m_fIsMouseSupportsTouchScreen(false)
    , m_fIsMouseSupportsTouchPad(false)
    , m_fIsMouseHostCursorNeeded(false)
    , m_fIsMouseCaptured(false)
    , m_fIsMouseIntegrated(true)
    , m_fIsValidPointerShapePresent(false)
    , m_fIsHidingHostPointer(true)
    , m_fIsValidCursorPositionPresent(false)
    , m_enmVMExecutionEngine(KVMExecutionEngine_NotSet)
    /* CPU hardware virtualization features for VM: */
    , m_fIsHWVirtExNestedPagingEnabled(false)
    , m_fIsHWVirtExUXEnabled(false)
    /* VM's effective paravirtualization provider: */
    , m_paraVirtProvider(KParavirtProvider_None)
{
}

UISession::~UISession()
{
}

bool UISession::prepare()
{
    /* Prepare COM stuff: */
    if (!prepareSession())
        return false;
    prepareNotificationCenter();
    prepareConsoleEventHandlers();
    prepareFramebuffers();

    /* Prepare GUI stuff: */
    prepareActions();
    prepareConnections();
    prepareMachineWindowIcon();
    prepareScreens();
    prepareSignalHandling();

    /* Load settings: */
    loadSessionSettings();

    /* True by default: */
    return true;
}

bool UISession::prepareSession()
{
    /* Open session: */
    m_session = uiCommon().openSession(uiCommon().managedVMUuid(),
                                         uiCommon().isSeparateProcess()
                                       ? KLockType_Shared
                                       : KLockType_VM);
    if (m_session.isNull())
        return false;

    /* Get machine: */
    m_machine = m_session.GetMachine();
    if (m_machine.isNull())
        return false;

    /* Get console: */
    m_console = m_session.GetConsole();
    if (m_console.isNull())
        return false;

    /* Get display: */
    m_display = m_console.GetDisplay();
    if (m_display.isNull())
        return false;

    /* Get guest: */
    m_guest = m_console.GetGuest();
    if (m_guest.isNull())
        return false;

    /* Get mouse: */
    m_mouse = m_console.GetMouse();
    if (m_mouse.isNull())
        return false;

    /* Get keyboard: */
    m_keyboard = m_console.GetKeyboard();
    if (m_keyboard.isNull())
        return false;

    /* Get debugger: */
    m_debugger = m_console.GetDebugger();
    if (m_debugger.isNull())
        return false;

    /* Update machine-name: */
    m_strMachineName = machine().GetName();

    /* Update machine-state: */
    m_machineState = machine().GetState();

    /* True by default: */
    return true;
}

void UISession::prepareNotificationCenter()
{
    UINotificationCenter::create();
}

void UISession::prepareConsoleEventHandlers()
{
    /* Create console event-handler: */
    UIConsoleEventHandler::create(this);

    /* Add console event connections: */
    connect(gConsoleEvents, &UIConsoleEventHandler::sigMousePointerShapeChange,
            this, &UISession::sltMousePointerShapeChange);
    connect(gConsoleEvents, &UIConsoleEventHandler::sigMouseCapabilityChange,
            this, &UISession::sltMouseCapabilityChange);
    connect(gConsoleEvents, &UIConsoleEventHandler::sigCursorPositionChange,
            this, &UISession::sltCursorPositionChange);
    connect(gConsoleEvents, &UIConsoleEventHandler::sigKeyboardLedsChangeEvent,
            this, &UISession::sltKeyboardLedsChangeEvent);
    connect(gConsoleEvents, &UIConsoleEventHandler::sigStateChange,
            this, &UISession::sltStateChange);
    connect(gConsoleEvents, &UIConsoleEventHandler::sigAdditionsChange,
            this, &UISession::sltAdditionsChange);
    connect(gConsoleEvents, &UIConsoleEventHandler::sigVRDEChange,
            this, &UISession::sltVRDEChange);
    connect(gConsoleEvents, &UIConsoleEventHandler::sigRecordingChange,
            this, &UISession::sltRecordingChange);
    connect(gConsoleEvents, &UIConsoleEventHandler::sigNetworkAdapterChange,
            this, &UISession::sigNetworkAdapterChange);
    connect(gConsoleEvents, &UIConsoleEventHandler::sigStorageDeviceChange,
            this, &UISession::sltHandleStorageDeviceChange);
    connect(gConsoleEvents, &UIConsoleEventHandler::sigMediumChange,
            this, &UISession::sigMediumChange);
    connect(gConsoleEvents, &UIConsoleEventHandler::sigUSBControllerChange,
            this, &UISession::sigUSBControllerChange);
    connect(gConsoleEvents, &UIConsoleEventHandler::sigUSBDeviceStateChange,
            this, &UISession::sigUSBDeviceStateChange);
    connect(gConsoleEvents, &UIConsoleEventHandler::sigSharedFolderChange,
            this, &UISession::sigSharedFolderChange);
    connect(gConsoleEvents, &UIConsoleEventHandler::sigRuntimeError,
            this, &UISession::sigRuntimeError);
#ifdef VBOX_WS_MAC
    connect(gConsoleEvents, &UIConsoleEventHandler::sigShowWindow,
            this, &UISession::sigShowWindows, Qt::QueuedConnection);
#endif /* VBOX_WS_MAC */
    connect(gConsoleEvents, &UIConsoleEventHandler::sigCPUExecutionCapChange,
            this, &UISession::sigCPUExecutionCapChange);
    connect(gConsoleEvents, &UIConsoleEventHandler::sigGuestMonitorChange,
            this, &UISession::sltGuestMonitorChange);
    connect(gConsoleEvents, &UIConsoleEventHandler::sigAudioAdapterChange,
            this, &UISession::sltAudioAdapterChange);
    connect(gConsoleEvents, &UIConsoleEventHandler::sigClipboardModeChange,
            this, &UISession::sltClipboardModeChange);
    connect(gConsoleEvents, &UIConsoleEventHandler::sigDnDModeChange,
            this, &UISession::sltDnDModeChange);
}

void UISession::prepareFramebuffers()
{
    /* Each framebuffer will be really prepared on first UIMachineView creation: */
    m_frameBufferVector.resize(machine().GetGraphicsAdapter().GetMonitorCount());
}

void UISession::prepareActions()
{
    /* Create action-pool: */
    m_pActionPool = UIActionPool::create(UIActionPoolType_Runtime);
    if (actionPool())
    {
        /* Make sure action-pool knows guest-screen count: */
        actionPool()->toRuntime()->setGuestScreenCount(m_frameBufferVector.size());
        /* Update action restrictions: */
        updateActionRestrictions();

#ifdef VBOX_WS_MAC
        /* Create Mac OS X menu-bar: */
        m_pMenuBar = new QMenuBar;
        if (m_pMenuBar)
        {
            /* Configure Mac OS X menu-bar: */
            connect(gEDataManager, &UIExtraDataManager::sigMenuBarConfigurationChange,
                    this, &UISession::sltHandleMenuBarConfigurationChange);
            /* Update Mac OS X menu-bar: */
            updateMenu();
        }
#endif /* VBOX_WS_MAC */
        /* Postpone enabling the GA update action until GA's are loaded: */
        if (actionPool()->action(UIActionIndexRT_M_Devices_S_UpgradeGuestAdditions))
            actionPool()->action(UIActionIndexRT_M_Devices_S_UpgradeGuestAdditions)->setEnabled(false);
    }
}

void UISession::prepareConnections()
{
    /* UICommon connections: */
    connect(&uiCommon(), &UICommon::sigAskToDetachCOM, this, &UISession::sltDetachCOM);

#ifdef VBOX_WS_MAC
    /* Install native display reconfiguration callback: */
    CGDisplayRegisterReconfigurationCallback(cgDisplayReconfigurationCallback, this);
#else /* !VBOX_WS_MAC */
    /* Install Qt display reconfiguration callbacks: */
    connect(gpDesktop, &UIDesktopWidgetWatchdog::sigHostScreenCountChanged,
            this, &UISession::sltHandleHostScreenCountChange);
    connect(gpDesktop, &UIDesktopWidgetWatchdog::sigHostScreenResized,
            this, &UISession::sltHandleHostScreenGeometryChange);
# if defined(VBOX_WS_X11) && !defined(VBOX_GUI_WITH_CUSTOMIZATIONS1)
    connect(gpDesktop, &UIDesktopWidgetWatchdog::sigHostScreenWorkAreaRecalculated,
            this, &UISession::sltHandleHostScreenAvailableAreaChange);
# else /* !VBOX_WS_X11 || VBOX_GUI_WITH_CUSTOMIZATIONS1 */
    connect(gpDesktop, &UIDesktopWidgetWatchdog::sigHostScreenWorkAreaResized,
            this, &UISession::sltHandleHostScreenAvailableAreaChange);
# endif /* !VBOX_WS_X11 || VBOX_GUI_WITH_CUSTOMIZATIONS1 */
#endif /* !VBOX_WS_MAC */
}

void UISession::prepareMachineWindowIcon()
{
    /* Acquire user machine-window icon: */
    QIcon icon = generalIconPool().userMachineIcon(machine());
    /* Use the OS type icon if user one was not set: */
    if (icon.isNull())
        icon = generalIconPool().guestOSTypeIcon(machine().GetOSTypeId());
    /* Use the default icon if nothing else works: */
    if (icon.isNull())
        icon = QIcon(":/VirtualBox_48px.png");
    /* Store the icon dynamically: */
    m_pMachineWindowIcon = new QIcon(icon);
}

void UISession::prepareScreens()
{
    /* Recache display data: */
    updateHostScreenData();

#ifdef VBOX_WS_MAC
    /* Prepare display-change watchdog: */
    m_pWatchdogDisplayChange = new QTimer(this);
    {
        m_pWatchdogDisplayChange->setInterval(500);
        m_pWatchdogDisplayChange->setSingleShot(true);
        connect(m_pWatchdogDisplayChange, &QTimer::timeout,
                this, &UISession::sltCheckIfHostDisplayChanged);
    }
#endif /* VBOX_WS_MAC */

    /* Prepare initial screen visibility status: */
    m_monitorVisibilityVector.resize(machine().GetGraphicsAdapter().GetMonitorCount());
    m_monitorVisibilityVector.fill(false);
    m_monitorVisibilityVector[0] = true;

    /* Prepare empty last full-screen size vector: */
    m_monitorLastFullScreenSizeVector.resize(machine().GetGraphicsAdapter().GetMonitorCount());
    m_monitorLastFullScreenSizeVector.fill(QSize(-1, -1));

    /* If machine is in 'saved' state: */
    if (isSaved())
    {
        /* Update screen visibility status from saved-state: */
        for (int iScreenIndex = 0; iScreenIndex < m_monitorVisibilityVector.size(); ++iScreenIndex)
        {
            BOOL fEnabled = true;
            ULONG uGuestOriginX = 0, uGuestOriginY = 0, uGuestWidth = 0, uGuestHeight = 0;
            machine().QuerySavedGuestScreenInfo(iScreenIndex,
                                                uGuestOriginX, uGuestOriginY,
                                                uGuestWidth, uGuestHeight, fEnabled);
            m_monitorVisibilityVector[iScreenIndex] = fEnabled;
        }
        /* And make sure at least one of them is visible (primary if others are hidden): */
        if (countOfVisibleWindows() < 1)
            m_monitorVisibilityVector[0] = true;
    }
    else if (uiCommon().isSeparateProcess())
    {
        /* Update screen visibility status from display directly: */
        for (int iScreenIndex = 0; iScreenIndex < m_monitorVisibilityVector.size(); ++iScreenIndex)
        {
            KGuestMonitorStatus enmStatus = KGuestMonitorStatus_Disabled;
            ULONG uGuestWidth = 0, uGuestHeight = 0, uBpp = 0;
            LONG iGuestOriginX = 0, iGuestOriginY = 0;
            display().GetScreenResolution(iScreenIndex,
                                          uGuestWidth, uGuestHeight, uBpp,
                                          iGuestOriginX, iGuestOriginY, enmStatus);
            m_monitorVisibilityVector[iScreenIndex] = (   enmStatus == KGuestMonitorStatus_Enabled
                                                       || enmStatus == KGuestMonitorStatus_Blank);
        }
        /* And make sure at least one of them is visible (primary if others are hidden): */
        if (countOfVisibleWindows() < 1)
            m_monitorVisibilityVector[0] = true;
    }

    /* Prepare initial screen visibility status of host-desires (same as facts): */
    m_monitorVisibilityVectorHostDesires.resize(machine().GetGraphicsAdapter().GetMonitorCount());
    for (int iScreenIndex = 0; iScreenIndex < m_monitorVisibilityVector.size(); ++iScreenIndex)
        m_monitorVisibilityVectorHostDesires[iScreenIndex] = m_monitorVisibilityVector[iScreenIndex];

    /* Make sure action-pool knows guest-screen visibility status: */
    for (int iScreenIndex = 0; iScreenIndex < m_monitorVisibilityVector.size(); ++iScreenIndex)
        actionPool()->toRuntime()->setGuestScreenVisible(iScreenIndex, m_monitorVisibilityVector.at(iScreenIndex));
}

void UISession::prepareSignalHandling()
{
#ifdef VBOX_GUI_WITH_KEYS_RESET_HANDLER
    struct sigaction sa;
    sa.sa_sigaction = &signalHandlerSIGUSR1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGUSR1, &sa, NULL);
#endif /* VBOX_GUI_WITH_KEYS_RESET_HANDLER */
}

void UISession::loadSessionSettings()
{
    /* Load extra-data settings: */
    {
        /* Get machine ID: */
        const QUuid uMachineID = uiCommon().managedVMUuid();

#ifndef VBOX_WS_MAC
        /* Load user's machine-window name postfix: */
        m_strMachineWindowNamePostfix = gEDataManager->machineWindowNamePostfix(uMachineID);
#endif

        /* Should guest autoresize? */
        QAction *pGuestAutoresizeSwitch = actionPool()->action(UIActionIndexRT_M_View_T_GuestAutoresize);
        pGuestAutoresizeSwitch->setChecked(gEDataManager->guestScreenAutoResizeEnabled(uMachineID));

#ifdef VBOX_WS_MAC
        /* User-element (Menu-bar and Dock) options: */
        {
            const bool fDisabled = gEDataManager->guiFeatureEnabled(GUIFeatureType_NoUserElements);
            if (fDisabled)
                UICocoaApplication::instance()->hideUserElements();
        }
#else /* !VBOX_WS_MAC */
        /* Menu-bar options: */
        {
            const bool fEnabledGlobally = !gEDataManager->guiFeatureEnabled(GUIFeatureType_NoMenuBar);
            const bool fEnabledForMachine = gEDataManager->menuBarEnabled(uMachineID);
            const bool fEnabled = fEnabledGlobally && fEnabledForMachine;
            QAction *pActionMenuBarSettings = actionPool()->action(UIActionIndexRT_M_View_M_MenuBar_S_Settings);
            pActionMenuBarSettings->setEnabled(fEnabled);
            QAction *pActionMenuBarSwitch = actionPool()->action(UIActionIndexRT_M_View_M_MenuBar_T_Visibility);
            pActionMenuBarSwitch->blockSignals(true);
            pActionMenuBarSwitch->setChecked(fEnabled);
            pActionMenuBarSwitch->blockSignals(false);
        }
#endif /* !VBOX_WS_MAC */

        /* Status-bar options: */
        {
            const bool fEnabledGlobally = !gEDataManager->guiFeatureEnabled(GUIFeatureType_NoStatusBar);
            const bool fEnabledForMachine = gEDataManager->statusBarEnabled(uMachineID);
            const bool fEnabled = fEnabledGlobally && fEnabledForMachine;
            QAction *pActionStatusBarSettings = actionPool()->action(UIActionIndexRT_M_View_M_StatusBar_S_Settings);
            pActionStatusBarSettings->setEnabled(fEnabled);
            QAction *pActionStatusBarSwitch = actionPool()->action(UIActionIndexRT_M_View_M_StatusBar_T_Visibility);
            pActionStatusBarSwitch->blockSignals(true);
            pActionStatusBarSwitch->setChecked(fEnabled);
            pActionStatusBarSwitch->blockSignals(false);
        }

        /* Input options: */
        actionPool()->action(UIActionIndexRT_M_Input_M_Mouse_T_Integration)->setChecked(isMouseIntegrated());

        /* Devices options: */
        {
            const CAudioSettings comAudioSettings = m_machine.GetAudioSettings();
            const CAudioAdapter  comAdapter = comAudioSettings.GetAdapter();
            actionPool()->action(UIActionIndexRT_M_Devices_M_Audio_T_Output)->blockSignals(true);
            actionPool()->action(UIActionIndexRT_M_Devices_M_Audio_T_Output)->setChecked(comAdapter.GetEnabledOut());
            actionPool()->action(UIActionIndexRT_M_Devices_M_Audio_T_Output)->blockSignals(false);
            actionPool()->action(UIActionIndexRT_M_Devices_M_Audio_T_Input)->blockSignals(true);
            actionPool()->action(UIActionIndexRT_M_Devices_M_Audio_T_Input)->setChecked(comAdapter.GetEnabledIn());
            actionPool()->action(UIActionIndexRT_M_Devices_M_Audio_T_Input)->blockSignals(false);
        }

        /* What is the default close action and the restricted are? */
        m_defaultCloseAction = gEDataManager->defaultMachineCloseAction(uMachineID);
        m_restrictedCloseActions = gEDataManager->restrictedMachineCloseActions(uMachineID);
        m_fAllCloseActionsRestricted =  (!uiCommon().isSeparateProcess() || (m_restrictedCloseActions & MachineCloseAction_Detach))
                                     && (m_restrictedCloseActions & MachineCloseAction_SaveState)
                                     && (m_restrictedCloseActions & MachineCloseAction_Shutdown)
                                     && (m_restrictedCloseActions & MachineCloseAction_PowerOff);
    }
}

void UISession::cleanupMachineWindowIcon()
{
    /* Cleanup machine-window icon: */
    delete m_pMachineWindowIcon;
    m_pMachineWindowIcon = 0;
}

void UISession::cleanupConnections()
{
#ifdef VBOX_WS_MAC
    /* Remove display reconfiguration callback: */
    CGDisplayRemoveReconfigurationCallback(cgDisplayReconfigurationCallback, this);
#endif /* VBOX_WS_MAC */
}

void UISession::cleanupActions()
{
#ifdef VBOX_WS_MAC
    /* Destroy Mac OS X menu-bar: */
    delete m_pMenuBar;
    m_pMenuBar = 0;
#endif /* VBOX_WS_MAC */

    /* Destroy action-pool if necessary: */
    if (actionPool())
        UIActionPool::destroy(actionPool());
}

void UISession::cleanupFramebuffers()
{
    /* Cleanup framebuffers finally: */
    for (int i = m_frameBufferVector.size() - 1; i >= 0; --i)
    {
        UIFrameBuffer *pFrameBuffer = m_frameBufferVector[i];
        if (pFrameBuffer)
        {
            /* Mark framebuffer as unused: */
            pFrameBuffer->setMarkAsUnused(true);
            /* Detach framebuffer from Display: */
            pFrameBuffer->detach();
            /* Delete framebuffer reference: */
            delete pFrameBuffer;
        }
    }
    m_frameBufferVector.clear();

    /* Make sure action-pool knows guest-screen count: */
    if (actionPool())
        actionPool()->toRuntime()->setGuestScreenCount(m_frameBufferVector.size());
}

void UISession::cleanupConsoleEventHandlers()
{
    /* Destroy console event-handler if necessary: */
    if (gConsoleEvents)
        UIConsoleEventHandler::destroy();
}

void UISession::cleanupNotificationCenter()
{
    UINotificationCenter::destroy();
}

void UISession::cleanupSession()
{
    /* Detach debugger: */
    if (!m_debugger.isNull())
        m_debugger.detach();

    /* Detach keyboard: */
    if (!m_keyboard.isNull())
        m_keyboard.detach();

    /* Detach mouse: */
    if (!m_mouse.isNull())
        m_mouse.detach();

    /* Detach guest: */
    if (!m_guest.isNull())
        m_guest.detach();

    /* Detach display: */
    if (!m_display.isNull())
        m_display.detach();

    /* Detach console: */
    if (!m_console.isNull())
        m_console.detach();

    /* Detach machine: */
    if (!m_machine.isNull())
        m_machine.detach();

    /* Close session: */
    if (!m_session.isNull() && uiCommon().isVBoxSVCAvailable())
    {
        m_session.UnlockMachine();
        m_session.detach();
    }
}

void UISession::cleanup()
{
    /* Cleanup GUI stuff: */
    //cleanupSignalHandling();
    //cleanupScreens();
    cleanupMachineWindowIcon();
    cleanupConnections();
    cleanupActions();
}

#ifdef VBOX_WS_MAC
void UISession::updateMenu()
{
    /* Rebuild Mac OS X menu-bar: */
    m_pMenuBar->clear();
    foreach (QMenu *pMenu, actionPool()->menus())
    {
        UIMenu *pMenuUI = qobject_cast<UIMenu*>(pMenu);
        if (!pMenuUI->isConsumable() || !pMenuUI->isConsumed())
            m_pMenuBar->addMenu(pMenuUI);
        if (pMenuUI->isConsumable() && !pMenuUI->isConsumed())
            pMenuUI->setConsumed(true);
    }
    /* Update the dock menu as well: */
    if (machineLogic())
        machineLogic()->updateDock();
}
#endif /* VBOX_WS_MAC */

/** Generate a BGRA bitmap which approximates a XOR/AND mouse pointer.
 *
 * Pixels which has 1 in the AND mask and not 0 in the XOR mask are replaced by
 * the inverted pixel and 8 surrounding pixels with the original color.
 * Fort example a white pixel (W) is replaced with a black (B) pixel:
 *         WWW
 *  W   -> WBW
 *         WWW
 * The surrounding pixels are written only if the corresponding source pixel
 * does not affect the screen, i.e. AND bit is 1 and XOR value is 0.
 */
static void renderCursorPixels(const uint32_t *pu32XOR, const uint8_t *pu8AND,
                               uint32_t u32Width, uint32_t u32Height,
                               uint32_t *pu32Pixels, uint32_t cbPixels)
{
    /* Output pixels set to 0 which allow to not write transparent pixels anymore. */
    memset(pu32Pixels, 0, cbPixels);

    const uint32_t *pu32XORSrc = pu32XOR;  /* Iterator for source XOR pixels. */
    const uint8_t *pu8ANDSrcLine = pu8AND; /* The current AND mask scanline. */
    uint32_t *pu32Dst = pu32Pixels;        /* Iterator for all destination BGRA pixels. */

    /* Some useful constants. */
    const int cbANDLine = ((int)u32Width + 7) / 8;

    int y;
    for (y = 0; y < (int)u32Height; ++y)
    {
        int x;
        for (x = 0; x < (int)u32Width; ++x)
        {
            const uint32_t u32Pixel = *pu32XORSrc; /* Current pixel at (x,y) */
            const uint8_t *pu8ANDSrc = pu8ANDSrcLine + x / 8; /* Byte which containt current AND bit. */

            if ((*pu8ANDSrc << (x % 8)) & 0x80)
            {
                if (u32Pixel)
                {
                    const uint32_t u32PixelInverted = ~u32Pixel;

                    /* Scan neighbor pixels and assign them if they are transparent. */
                    int dy;
                    for (dy = -1; dy <= 1; ++dy)
                    {
                        const int yn = y + dy;
                        if (yn < 0 || yn >= (int)u32Height)
                            continue; /* Do not cross the bounds. */

                        int dx;
                        for (dx = -1; dx <= 1; ++dx)
                        {
                            const int xn = x + dx;
                            if (xn < 0 || xn >= (int)u32Width)
                                continue;  /* Do not cross the bounds. */

                            if (dx != 0 || dy != 0)
                            {
                                /* Check if the neighbor pixel is transparent. */
                                const uint32_t *pu32XORNeighborSrc = &pu32XORSrc[dy * (int)u32Width + dx];
                                const uint8_t *pu8ANDNeighborSrc = pu8ANDSrcLine + dy * cbANDLine + xn / 8;
                                if (   *pu32XORNeighborSrc == 0
                                    && ((*pu8ANDNeighborSrc << (xn % 8)) & 0x80) != 0)
                                {
                                    /* Transparent neighbor pixels are replaced with the source pixel value. */
                                    uint32_t *pu32PixelNeighborDst = &pu32Dst[dy * (int)u32Width + dx];
                                    *pu32PixelNeighborDst = u32Pixel | 0xFF000000;
                                }
                            }
                            else
                            {
                                /* The pixel itself is replaced with inverted value. */
                                *pu32Dst = u32PixelInverted | 0xFF000000;
                            }
                        }
                    }
                }
                else
                {
                    /* The pixel does not affect the screen.
                     * Do nothing. Do not touch destination which can already contain generated pixels.
                     */
                }
            }
            else
            {
                /* AND bit is 0, the pixel will be just drawn. */
                *pu32Dst = u32Pixel | 0xFF000000;
            }

            ++pu32XORSrc; /* Next source pixel. */
            ++pu32Dst;    /* Next destination pixel. */
        }

        /* Next AND scanline. */
        pu8ANDSrcLine += cbANDLine;
    }
}

#ifdef VBOX_WS_WIN
static bool isPointer1bpp(const uint8_t *pu8XorMask,
                          uint uWidth,
                          uint uHeight)
{
    /* Check if the pointer has only 0 and 0xFFFFFF pixels, ignoring the alpha channel. */
    const uint32_t *pu32Src = (uint32_t *)pu8XorMask;

    uint y;
    for (y = 0; y < uHeight ; ++y)
    {
        uint x;
        for (x = 0; x < uWidth; ++x)
        {
            const uint32_t u32Pixel = pu32Src[x] & UINT32_C(0xFFFFFF);
            if (u32Pixel != 0 && u32Pixel != UINT32_C(0xFFFFFF))
                return false;
        }

        pu32Src += uWidth;
    }

    return true;
}
#endif /* VBOX_WS_WIN */

void UISession::updateMousePointerShape()
{
    /* Fetch incoming shape data: */
    const bool fHasAlpha = m_shapeData.hasAlpha();
    const uint uWidth = m_shapeData.shapeSize().width();
    const uint uHeight = m_shapeData.shapeSize().height();
    const uchar *pShapeData = m_shapeData.shape().constData();
    AssertMsgReturnVoid(pShapeData, ("Shape data must not be NULL!\n"));

    /* Invalidate mouse pointer shape initially: */
    m_fIsValidPointerShapePresent = false;
    m_cursorShapePixmap = QPixmap();
    m_cursorMaskPixmap = QPixmap();

    /* Parse incoming shape data: */
    const uchar *pSrcAndMaskPtr = pShapeData;
    const uint uAndMaskSize = (uWidth + 7) / 8 * uHeight;
    const uchar *pSrcShapePtr = pShapeData + ((uAndMaskSize + 3) & ~3);

#if defined (VBOX_WS_WIN)

    /* Create an ARGB image out of the shape data: */

    // WORKAROUND:
    // Qt5 QCursor recommends 32 x 32 cursor, therefore the original data is copied to
    // a larger QImage if necessary. Cursors like 10x16 did not work correctly (Solaris 10 guest).
    // Align the cursor dimensions to 32 bit pixels, because for example a 56x56 monochrome cursor
    // did not work correctly on Windows host.
    const uint uCursorWidth = RT_ALIGN_32(uWidth, 32);
    const uint uCursorHeight = RT_ALIGN_32(uHeight, 32);

    if (fHasAlpha)
    {
        QImage image(uCursorWidth, uCursorHeight, QImage::Format_ARGB32);
        memset(image.bits(), 0, image.byteCount());

        const uint32_t *pu32SrcShapeScanline = (uint32_t *)pSrcShapePtr;
        for (uint y = 0; y < uHeight; ++y, pu32SrcShapeScanline += uWidth)
            memcpy(image.scanLine(y), pu32SrcShapeScanline, uWidth * sizeof(uint32_t));

        m_cursorShapePixmap = QPixmap::fromImage(image);
    }
    else
    {
        if (isPointer1bpp(pSrcShapePtr, uWidth, uHeight))
        {
            /* Incoming data consist of 32 bit BGR XOR mask and 1 bit AND mask.
             * XOR pixels contain either 0x00000000 or 0x00FFFFFF.
             *
             * Originally intended result (F denotes 0x00FFFFFF):
             * XOR AND
             *   0   0 black
             *   F   0 white
             *   0   1 transparent
             *   F   1 xor'd
             *
             * Actual Qt5 result for color table 0:0xFF000000, 1:0xFFFFFFFF
             * (tested on Windows 7 and 10 64 bit hosts):
             * Bitmap Mask
             *  0   0 black
             *  1   0 white
             *  0   1 xor
             *  1   1 transparent
             *
             */

            QVector<QRgb> colors(2);
            colors[0] = UINT32_C(0xFF000000);
            colors[1] = UINT32_C(0xFFFFFFFF);

            QImage bitmap(uCursorWidth, uCursorHeight, QImage::Format_Mono);
            bitmap.setColorTable(colors);
            memset(bitmap.bits(), 0xFF, bitmap.byteCount());

            QImage mask(uCursorWidth, uCursorHeight, QImage::Format_Mono);
            mask.setColorTable(colors);
            memset(mask.bits(), 0xFF, mask.byteCount());

            const uint8_t *pu8SrcAndScanline = pSrcAndMaskPtr;
            const uint32_t *pu32SrcShapeScanline = (uint32_t *)pSrcShapePtr;
            for (uint y = 0; y < uHeight; ++y)
            {
                for (uint x = 0; x < uWidth; ++x)
                {
                    const uint8_t u8Bit = (uint8_t)(1 << (7 - x % 8));

                    const uint8_t u8SrcMaskByte = pu8SrcAndScanline[x / 8];
                    const uint8_t u8SrcMaskBit = u8SrcMaskByte & u8Bit;
                    const uint32_t u32SrcPixel = pu32SrcShapeScanline[x] & UINT32_C(0xFFFFFF);

                    uint8_t *pu8DstMaskByte = &mask.scanLine(y)[x / 8];
                    uint8_t *pu8DstBitmapByte = &bitmap.scanLine(y)[x / 8];

                    if (u8SrcMaskBit == 0)
                    {
                        if (u32SrcPixel == 0)
                        {
                            /* Black: Qt Bitmap = 0, Mask = 0 */
                            *pu8DstMaskByte &= ~u8Bit;
                            *pu8DstBitmapByte &= ~u8Bit;
                        }
                        else
                        {
                            /* White: Qt Bitmap = 1, Mask = 0 */
                            *pu8DstMaskByte &= ~u8Bit;
                            *pu8DstBitmapByte |= u8Bit;
                        }
                    }
                    else
                    {
                        if (u32SrcPixel == 0)
                        {
                            /* Transparent: Qt Bitmap = 1, Mask = 1 */
                            *pu8DstMaskByte |= u8Bit;
                            *pu8DstBitmapByte |= u8Bit;
                        }
                        else
                        {
                            /* Xor'ed: Qt Bitmap = 0, Mask = 1 */
                            *pu8DstMaskByte |= u8Bit;
                            *pu8DstBitmapByte &= ~u8Bit;
                        }
                    }
                }

                pu8SrcAndScanline += (uWidth + 7) / 8;
                pu32SrcShapeScanline += uWidth;
            }

            m_cursorShapePixmap = QBitmap::fromImage(bitmap);
            m_cursorMaskPixmap = QBitmap::fromImage(mask);
        }
        else
        {
            /* Assign alpha channel values according to the AND mask: 1 -> 0x00, 0 -> 0xFF: */
            QImage image(uCursorWidth, uCursorHeight, QImage::Format_ARGB32);
            memset(image.bits(), 0, image.byteCount());

            const uint8_t *pu8SrcAndScanline = pSrcAndMaskPtr;
            const uint32_t *pu32SrcShapeScanline = (uint32_t *)pSrcShapePtr;

            for (uint y = 0; y < uHeight; ++y)
            {
                uint32_t *pu32DstPixel = (uint32_t *)image.scanLine(y);

                for (uint x = 0; x < uWidth; ++x)
                {
                    const uint8_t u8Bit = (uint8_t)(1 << (7 - x % 8));
                    const uint8_t u8SrcMaskByte = pu8SrcAndScanline[x / 8];

                    if (u8SrcMaskByte & u8Bit)
                        *pu32DstPixel++ = pu32SrcShapeScanline[x] & UINT32_C(0x00FFFFFF);
                    else
                        *pu32DstPixel++ = pu32SrcShapeScanline[x] | UINT32_C(0xFF000000);
                }

                pu32SrcShapeScanline += uWidth;
                pu8SrcAndScanline += (uWidth + 7) / 8;
            }

            m_cursorShapePixmap = QPixmap::fromImage(image);
        }
    }

    /* Mark mouse pointer shape valid: */
    m_fIsValidPointerShapePresent = true;

#elif defined(VBOX_WS_X11) || defined(VBOX_WS_MAC)

    /* Create an ARGB image out of the shape data: */
    QImage image(uWidth, uHeight, QImage::Format_ARGB32);

    if (fHasAlpha)
    {
        memcpy(image.bits(), pSrcShapePtr, uHeight * uWidth * 4);
    }
    else
    {
        renderCursorPixels((uint32_t *)pSrcShapePtr, pSrcAndMaskPtr,
                           uWidth, uHeight,
                           (uint32_t *)image.bits(), uHeight * uWidth * 4);
    }

    /* Create cursor-pixmap from the image: */
    m_cursorShapePixmap = QPixmap::fromImage(image);

    /* Mark mouse pointer shape valid: */
    m_fIsValidPointerShapePresent = true;

#else

# warning "port me"

#endif

    /* Cache cursor pixmap size and hotspot: */
    m_cursorSize = m_cursorShapePixmap.size();
    m_cursorHotspot = m_shapeData.hotSpot();
}

bool UISession::preprocessInitialization()
{
#ifdef VBOX_WITH_NETFLT
    /* Skip further checks if VM in saved state */
    if (isSaved())
        return true;

    /* Make sure all the attached and enabled network
     * adapters are present on the host. This check makes sense
     * in two cases only - when attachement type is Bridged Network
     * or Host-only Interface. NOTE: Only currently enabled
     * attachement type is checked (incorrect parameters check for
     * currently disabled attachement types is skipped). */
    QStringList failedInterfaceNames;
    QStringList availableInterfaceNames;

    /* Create host network interface names list */
    foreach (const CHostNetworkInterface &iface, uiCommon().host().GetNetworkInterfaces())
    {
        availableInterfaceNames << iface.GetName();
        availableInterfaceNames << iface.GetShortName();
    }

    ulong cCount = uiCommon().virtualBox().GetSystemProperties().GetMaxNetworkAdapters(machine().GetChipsetType());
    for (ulong uAdapterIndex = 0; uAdapterIndex < cCount; ++uAdapterIndex)
    {
        CNetworkAdapter na = machine().GetNetworkAdapter(uAdapterIndex);

        if (na.GetEnabled())
        {
            QString strIfName = QString();

            /* Get physical network interface name for currently
             * enabled network attachement type */
            switch (na.GetAttachmentType())
            {
                case KNetworkAttachmentType_Bridged:
                    strIfName = na.GetBridgedInterface();
                    break;
#ifndef VBOX_WITH_VMNET
                case KNetworkAttachmentType_HostOnly:
                    strIfName = na.GetHostOnlyInterface();
                    break;
#endif /* !VBOX_WITH_VMNET */
                default: break; /* Shut up, MSC! */
            }

            if (!strIfName.isEmpty() &&
                !availableInterfaceNames.contains(strIfName))
            {
                LogFlow(("Found invalid network interface: %s\n", strIfName.toStdString().c_str()));
                failedInterfaceNames << QString("%1 (adapter %2)").arg(strIfName).arg(uAdapterIndex + 1);
            }
        }
    }

    /* Check if non-existent interfaces found */
    if (!failedInterfaceNames.isEmpty())
    {
        if (msgCenter().warnAboutNetworkInterfaceNotFound(machineName(), failedInterfaceNames.join(", ")))
            machineLogic()->openNetworkSettingsDialog();
        else
        {
            LogRel(("GUI: Aborting startup due to preprocess initialization issue detected...\n"));
            return false;
        }
    }
#endif /* VBOX_WITH_NETFLT */

    /* Check for USB enumeration warning. Don't return false even if we have a warning: */
    CHost comHost = uiCommon().host();
    if (comHost.GetUSBDevices().isEmpty() && comHost.isWarning())
    {
        /* Do not bitch if USB disabled: */
        if (!machine().GetUSBControllers().isEmpty())
        {
            /* Do not bitch if there are no filters (check if enabled too?): */
            if (!machine().GetUSBDeviceFilters().GetDeviceFilters().isEmpty())
                UINotificationMessage::cannotEnumerateHostUSBDevices(comHost);
        }
    }

    /* True by default: */
    return true;
}

bool UISession::mountAdHocImage(KDeviceType enmDeviceType, UIMediumDeviceType enmMediumType, const QString &strMediumName)
{
    /* Get VBox: */
    CVirtualBox comVBox = uiCommon().virtualBox();

    /* Prepare medium to mount: */
    UIMedium guiMedium;

    /* The 'none' medium name means ejecting what ever is in the drive,
     * in that case => leave the guiMedium variable null. */
    if (strMediumName != "none")
    {
        /* Open the medium: */
        const CMedium comMedium = comVBox.OpenMedium(strMediumName, enmDeviceType, KAccessMode_ReadWrite, false /* fForceNewUuid */);
        if (!comVBox.isOk() || comMedium.isNull())
        {
            UINotificationMessage::cannotOpenMedium(comVBox, strMediumName);
            return false;
        }

        /* Make sure medium ID is valid: */
        const QUuid uMediumId = comMedium.GetId();
        AssertReturn(!uMediumId.isNull(), false);

        /* Try to find UIMedium among cached: */
        guiMedium = uiCommon().medium(uMediumId);
        if (guiMedium.isNull())
        {
            /* Cache new one if necessary: */
            guiMedium = UIMedium(comMedium, enmMediumType, KMediumState_Created);
            uiCommon().createMedium(guiMedium);
        }
    }

    /* Search for a suitable storage slots: */
    QList<ExactStorageSlot> aFreeStorageSlots;
    QList<ExactStorageSlot> aBusyStorageSlots;
    foreach (const CStorageController &comController, machine().GetStorageControllers())
    {
        foreach (const CMediumAttachment &comAttachment, machine().GetMediumAttachmentsOfController(comController.GetName()))
        {
            /* Look for an optical devices only: */
            if (comAttachment.GetType() == enmDeviceType)
            {
                /* Append storage slot to corresponding list: */
                if (comAttachment.GetMedium().isNull())
                    aFreeStorageSlots << ExactStorageSlot(comController.GetName(), comController.GetBus(),
                                                          comAttachment.GetPort(), comAttachment.GetDevice());
                else
                    aBusyStorageSlots << ExactStorageSlot(comController.GetName(), comController.GetBus(),
                                                          comAttachment.GetPort(), comAttachment.GetDevice());
            }
        }
    }

    /* Make sure at least one storage slot found: */
    QList<ExactStorageSlot> sStorageSlots = aFreeStorageSlots + aBusyStorageSlots;
    if (sStorageSlots.isEmpty())
    {
        UINotificationMessage::cannotMountImage(machineName(), strMediumName);
        return false;
    }

    /* Try to mount medium into first available storage slot: */
    while (!sStorageSlots.isEmpty())
    {
        const ExactStorageSlot storageSlot = sStorageSlots.takeFirst();
        machine().MountMedium(storageSlot.controller, storageSlot.port, storageSlot.device, guiMedium.medium(), false /* force */);
        if (machine().isOk())
            break;
    }

    /* Show error message if necessary: */
    if (!machine().isOk())
    {
        msgCenter().cannotRemountMedium(machine(), guiMedium, true /* mount? */, false /* retry? */, activeMachineWindow());
        return false;
    }

    /* Save machine settings: */
    machine().SaveSettings();

    /* Show error message if necessary: */
    if (!machine().isOk())
    {
        UINotificationMessage::cannotSaveMachineSettings(machine());
        return false;
    }

    /* True by default: */
    return true;
}

bool UISession::postprocessInitialization()
{
    /* There used to be some raw-mode warnings here for raw-mode incompatible
       guests (64-bit ones and OS/2).  Nothing to do at present. */
    return true;
}

bool UISession::isScreenVisibleHostDesires(ulong uScreenId) const
{
    /* Make sure index feats the bounds: */
    AssertReturn(uScreenId < (ulong)m_monitorVisibilityVectorHostDesires.size(), false);

    /* Return 'actual' (host-desire) visibility status: */
    return m_monitorVisibilityVectorHostDesires.value((int)uScreenId);
}

void UISession::setScreenVisibleHostDesires(ulong uScreenId, bool fIsMonitorVisible)
{
    /* Make sure index feats the bounds: */
    AssertReturnVoid(uScreenId < (ulong)m_monitorVisibilityVectorHostDesires.size());

    /* Remember 'actual' (host-desire) visibility status: */
    m_monitorVisibilityVectorHostDesires[(int)uScreenId] = fIsMonitorVisible;

    /* And remember the request in extra data for guests with VMSVGA: */
    /* This should be done before the actual hint is sent in case the guest overrides it. */
    gEDataManager->setLastGuestScreenVisibilityStatus(uScreenId, fIsMonitorVisible, uiCommon().managedVMUuid());
}

bool UISession::isScreenVisible(ulong uScreenId) const
{
    /* Make sure index feats the bounds: */
    AssertReturn(uScreenId < (ulong)m_monitorVisibilityVector.size(), false);

    /* Return 'actual' visibility status: */
    return m_monitorVisibilityVector.value((int)uScreenId);
}

void UISession::setScreenVisible(ulong uScreenId, bool fIsMonitorVisible)
{
    /* Make sure index feats the bounds: */
    AssertReturnVoid(uScreenId < (ulong)m_monitorVisibilityVector.size());

    /* Remember 'actual' visibility status: */
    m_monitorVisibilityVector[(int)uScreenId] = fIsMonitorVisible;
    /* Remember 'desired' visibility status: */
    /* See note in UIMachineView::sltHandleNotifyChange() regarding the graphics controller check. */
    if (machine().GetGraphicsAdapter().GetGraphicsControllerType() != KGraphicsControllerType_VMSVGA)
        gEDataManager->setLastGuestScreenVisibilityStatus(uScreenId, fIsMonitorVisible, uiCommon().managedVMUuid());

    /* Make sure action-pool knows guest-screen visibility status: */
    actionPool()->toRuntime()->setGuestScreenVisible(uScreenId, fIsMonitorVisible);
}

QSize UISession::lastFullScreenSize(ulong uScreenId) const
{
    /* Make sure index fits the bounds: */
    AssertReturn(uScreenId < (ulong)m_monitorLastFullScreenSizeVector.size(), QSize(-1, -1));

    /* Return last full-screen size: */
    return m_monitorLastFullScreenSizeVector.value((int)uScreenId);
}

void UISession::setLastFullScreenSize(ulong uScreenId, QSize size)
{
    /* Make sure index fits the bounds: */
    AssertReturnVoid(uScreenId < (ulong)m_monitorLastFullScreenSizeVector.size());

    /* Remember last full-screen size: */
    m_monitorLastFullScreenSizeVector[(int)uScreenId] = size;
}

int UISession::countOfVisibleWindows()
{
    int cCountOfVisibleWindows = 0;
    for (int i = 0; i < m_monitorVisibilityVector.size(); ++i)
        if (m_monitorVisibilityVector[i])
            ++cCountOfVisibleWindows;
    return cCountOfVisibleWindows;
}

QList<int> UISession::listOfVisibleWindows() const
{
    QList<int> visibleWindows;
    for (int i = 0; i < m_monitorVisibilityVector.size(); ++i)
        if (m_monitorVisibilityVector.at(i))
            visibleWindows.push_back(i);
    return visibleWindows;
}

CMediumVector UISession::machineMedia() const
{
    CMediumVector comMedia;
    /* Enumerate all the controllers: */
    foreach (const CStorageController &comController, m_machine.GetStorageControllers())
    {
        /* Enumerate all the attachments: */
        foreach (const CMediumAttachment &comAttachment, m_machine.GetMediumAttachmentsOfController(comController.GetName()))
        {
            /* Skip unrelated device types: */
            const KDeviceType enmDeviceType = comAttachment.GetType();
            if (   enmDeviceType != KDeviceType_HardDisk
                && enmDeviceType != KDeviceType_Floppy
                && enmDeviceType != KDeviceType_DVD)
                continue;
            if (   comAttachment.GetIsEjected()
                || comAttachment.GetMedium().isNull())
                continue;
            comMedia.append(comAttachment.GetMedium());
        }
    }
    return comMedia;
}

void UISession::loadVMSettings()
{
    /* Cache IMachine::ExecutionEngine value. */
    m_enmVMExecutionEngine = m_debugger.GetExecutionEngine();
    /* Load nested-paging CPU hardware virtualization extension: */
    m_fIsHWVirtExNestedPagingEnabled = m_debugger.GetHWVirtExNestedPagingEnabled();
    /* Load whether the VM is currently making use of the unrestricted execution feature of VT-x: */
    m_fIsHWVirtExUXEnabled = m_debugger.GetHWVirtExUXEnabled();
    /* Load VM's effective paravirtualization provider: */
    m_paraVirtProvider = m_machine.GetEffectiveParavirtProvider();
}

UIFrameBuffer* UISession::frameBuffer(ulong uScreenId) const
{
    Assert(uScreenId < (ulong)m_frameBufferVector.size());
    return m_frameBufferVector.value((int)uScreenId, 0);
}

void UISession::setFrameBuffer(ulong uScreenId, UIFrameBuffer* pFrameBuffer)
{
    Assert(uScreenId < (ulong)m_frameBufferVector.size());
    if (uScreenId < (ulong)m_frameBufferVector.size())
        m_frameBufferVector[(int)uScreenId] = pFrameBuffer;
}

void UISession::updateHostScreenData()
{
    /* Rebuild host-screen data vector: */
    m_hostScreens.clear();
    for (int iScreenIndex = 0; iScreenIndex < UIDesktopWidgetWatchdog::screenCount(); ++iScreenIndex)
        m_hostScreens << gpDesktop->screenGeometry(iScreenIndex);

    /* Make sure action-pool knows host-screen count: */
    actionPool()->toRuntime()->setHostScreenCount(m_hostScreens.size());
}

void UISession::updateActionRestrictions()
{
    /* Get host and prepare restrictions: */
    const CHost host = uiCommon().host();
    UIExtraDataMetaDefs::RuntimeMenuMachineActionType restrictionForMachine = UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Invalid;
    UIExtraDataMetaDefs::RuntimeMenuViewActionType restrictionForView = UIExtraDataMetaDefs::RuntimeMenuViewActionType_Invalid;
    UIExtraDataMetaDefs::RuntimeMenuDevicesActionType restrictionForDevices = UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Invalid;

    /* Separate process stuff: */
    {
        /* Initialize 'Machine' menu: */
        if (!uiCommon().isSeparateProcess())
            restrictionForMachine = (UIExtraDataMetaDefs::RuntimeMenuMachineActionType)(restrictionForMachine | UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Detach);
    }

    /* VRDE server stuff: */
    {
        /* Initialize 'View' menu: */
        const CVRDEServer server = machine().GetVRDEServer();
        if (server.isNull())
            restrictionForView = (UIExtraDataMetaDefs::RuntimeMenuViewActionType)(restrictionForView | UIExtraDataMetaDefs::RuntimeMenuViewActionType_VRDEServer);
    }

    /* Storage stuff: */
    {
        /* Initialize CD/FD menus: */
        int iDevicesCountCD = 0;
        int iDevicesCountFD = 0;
        foreach (const CMediumAttachment &attachment, machine().GetMediumAttachments())
        {
            if (attachment.GetType() == KDeviceType_DVD)
                ++iDevicesCountCD;
            if (attachment.GetType() == KDeviceType_Floppy)
                ++iDevicesCountFD;
        }
        QAction *pOpticalDevicesMenu = actionPool()->action(UIActionIndexRT_M_Devices_M_OpticalDevices);
        QAction *pFloppyDevicesMenu = actionPool()->action(UIActionIndexRT_M_Devices_M_FloppyDevices);
        pOpticalDevicesMenu->setData(iDevicesCountCD);
        pFloppyDevicesMenu->setData(iDevicesCountFD);
        if (!iDevicesCountCD)
            restrictionForDevices = (UIExtraDataMetaDefs::RuntimeMenuDevicesActionType)(restrictionForDevices | UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_OpticalDevices);
        if (!iDevicesCountFD)
            restrictionForDevices = (UIExtraDataMetaDefs::RuntimeMenuDevicesActionType)(restrictionForDevices | UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_FloppyDevices);
    }

    /* Audio stuff: */
    {
        /* Check whether audio controller is enabled. */
        const CAudioSettings comAudioSettings = machine().GetAudioSettings();
        const CAudioAdapter  comAdapter = comAudioSettings.GetAdapter();
        if (comAdapter.isNull() || !comAdapter.GetEnabled())
            restrictionForDevices = (UIExtraDataMetaDefs::RuntimeMenuDevicesActionType)(restrictionForDevices | UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Audio);
    }

    /* Network stuff: */
    {
        /* Initialize Network menu: */
        bool fAtLeastOneAdapterActive = false;
        const KChipsetType chipsetType = machine().GetChipsetType();
        ULONG uSlots = uiCommon().virtualBox().GetSystemProperties().GetMaxNetworkAdapters(chipsetType);
        for (ULONG uSlot = 0; uSlot < uSlots; ++uSlot)
        {
            const CNetworkAdapter &adapter = machine().GetNetworkAdapter(uSlot);
            if (adapter.GetEnabled())
            {
                fAtLeastOneAdapterActive = true;
                break;
            }
        }
        if (!fAtLeastOneAdapterActive)
            restrictionForDevices = (UIExtraDataMetaDefs::RuntimeMenuDevicesActionType)(restrictionForDevices | UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Network);
    }

    /* USB stuff: */
    {
        /* Check whether there is at least one USB controller with an available proxy. */
        const bool fUSBEnabled =    !machine().GetUSBDeviceFilters().isNull()
                                 && !machine().GetUSBControllers().isEmpty()
                                 && machine().GetUSBProxyAvailable();
        if (!fUSBEnabled)
            restrictionForDevices = (UIExtraDataMetaDefs::RuntimeMenuDevicesActionType)(restrictionForDevices | UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_USBDevices);
    }

    /* WebCams stuff: */
    {
        /* Check whether there is an accessible video input devices pool: */
        host.GetVideoInputDevices();
        const bool fWebCamsEnabled = host.isOk() && !machine().GetUSBControllers().isEmpty();
        if (!fWebCamsEnabled)
            restrictionForDevices = (UIExtraDataMetaDefs::RuntimeMenuDevicesActionType)(restrictionForDevices | UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_WebCams);
    }

    /* Apply cumulative restriction for 'Machine' menu: */
    actionPool()->toRuntime()->setRestrictionForMenuMachine(UIActionRestrictionLevel_Session, restrictionForMachine);
    /* Apply cumulative restriction for 'View' menu: */
    actionPool()->toRuntime()->setRestrictionForMenuView(UIActionRestrictionLevel_Session, restrictionForView);
    /* Apply cumulative restriction for 'Devices' menu: */
    actionPool()->toRuntime()->setRestrictionForMenuDevices(UIActionRestrictionLevel_Session, restrictionForDevices);
}

bool UISession::guestAdditionsUpgradable()
{
    if (!machine().isOk())
        return false;

    /* Auto GA update is currently for Windows and Linux guests only */
    const CGuestOSType osType = uiCommon().vmGuestOSType(machine().GetOSTypeId());
    if (!osType.isOk())
        return false;

    const QString strGuestFamily = osType.GetFamilyId();
    bool fIsWindowOrLinux = strGuestFamily.contains("windows", Qt::CaseInsensitive) || strGuestFamily.contains("linux", Qt::CaseInsensitive);

    if (!fIsWindowOrLinux)
        return false;

    /* Also check whether we have something to update automatically: */
    const ULONG ulGuestAdditionsRunLevel = guest().GetAdditionsRunLevel();
    if (ulGuestAdditionsRunLevel < (ULONG)KAdditionsRunLevelType_Userland)
        return false;

    return true;
}

#ifdef VBOX_GUI_WITH_KEYS_RESET_HANDLER
/**
 * Custom signal handler. When switching VTs, we might not get release events
 * for Ctrl-Alt and in case a savestate is performed on the new VT, the VM will
 * be saved with modifier keys stuck. This is annoying enough for introducing
 * this hack.
 */
/* static */
static void signalHandlerSIGUSR1(int sig, siginfo_t * /* pInfo */, void * /*pSecret */)
{
    /* Only SIGUSR1 is interesting: */
    if (sig == SIGUSR1)
        if (gpMachine)
            gpMachine->uisession()->machineLogic()->keyboardHandler()->releaseAllPressedKeys();
}
#endif /* VBOX_GUI_WITH_KEYS_RESET_HANDLER */
