/* $Id: UIMachineLogic.cpp $ */
/** @file
 * VBox Qt GUI - UIMachineLogic class implementation.
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
#include <QActionGroup>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QImageWriter>
#include <QPainter>
#include <QRegExp>
#include <QRegularExpression>
#include <QTimer>
#ifdef VBOX_WS_MAC
# include <QMenuBar>
#endif /* VBOX_WS_MAC */

/* GUI includes: */
#include "QIFileDialog.h"
#include "UIActionPoolRuntime.h"
#include "UIAddDiskEncryptionPasswordDialog.h"
#include "UIBootFailureDialog.h"
#include "UICommon.h"
#include "UIConverter.h"
#include "UIDesktopWidgetWatchdog.h"
#include "UIExtraDataManager.h"
#include "UIFileManagerDialog.h"
#include "UIFrameBuffer.h"
#include "UIGuestProcessControlDialog.h"
#include "UIHostComboEditor.h"
#include "UIIconPool.h"
#include "UIKeyboardHandler.h"
#include "UIMachineLogic.h"
#include "UIMachineLogicFullscreen.h"
#include "UIMachineLogicNormal.h"
#include "UIMachineLogicSeamless.h"
#include "UIMachineLogicScale.h"
#include "UIMachineView.h"
#include "UIMachineWindow.h"
#include "UIMedium.h"
#include "UIMessageCenter.h"
#include "UIModalWindowManager.h"
#include "UIMouseHandler.h"
#include "UINotificationCenter.h"
#include "UISession.h"
#include "UISettingsDialogSpecific.h"
#include "UISoftKeyboard.h"
#include "UITakeSnapshotDialog.h"
#include "UIVMLogViewerDialog.h"
#include "UIVMInformationDialog.h"
#ifdef VBOX_WS_MAC
# include "DockIconPreview.h"
# include "UIExtraDataManager.h"
#endif
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
# include "UINetworkRequestManager.h"
#endif
#ifdef VBOX_WS_X11
# include "VBoxUtils-x11.h"
#endif

/* COM includes: */
#include "CAudioAdapter.h"
#include "CAudioSettings.h"
#include "CDisplay.h"
#include "CEmulatedUSB.h"
#include "CGraphicsAdapter.h"
#include "CHostUSBDevice.h"
#include "CHostVideoInputDevice.h"
#include "CMachineDebugger.h"
#include "CMediumAttachment.h"
#include "CNetworkAdapter.h"
#include "CRecordingSettings.h"
#include "CSnapshot.h"
#include "CStorageController.h"
#include "CSystemProperties.h"
#include "CUSBDevice.h"
#include "CVirtualBoxErrorInfo.h"
#include "CVRDEServer.h"
#ifdef VBOX_WS_MAC
# include "CGuest.h"
#endif

/* Other VBox includes: */
#include <iprt/path.h>
#include <iprt/thread.h>
#ifdef VBOX_WITH_DEBUGGER_GUI
# include <VBox/dbggui.h>
# include <iprt/ldr.h>
#endif

/* VirtualBox interface declarations: */
#include <VBox/com/VirtualBox.h>

/* External / Other VBox includes: */
#ifdef VBOX_WS_MAC
# include "DarwinKeyboard.h"
#endif
#ifdef VBOX_WS_WIN
# include "WinKeyboard.h"
# include "VBoxUtils-win.h"
#endif
#ifdef VBOX_WS_X11
# include <XKeyboard.h>
#endif

#define VBOX_WITH_REWORKED_SESSION_INFORMATION /**< Define for reworked session-information window.  @todo r-bird: What's this for? */

struct USBTarget
{
    USBTarget() : attach(false), id(QUuid()) {}
    USBTarget(bool fAttach, const QUuid &uId)
        : attach(fAttach), id(uId) {}
    bool attach;
    QUuid id;
};
Q_DECLARE_METATYPE(USBTarget);

/** Describes enumerated webcam item. */
struct WebCamTarget
{
    WebCamTarget() : attach(false), name(QString()), path(QString()) {}
    WebCamTarget(bool fAttach, const QString &strName, const QString &strPath)
        : attach(fAttach), name(strName), path(strPath) {}
    bool attach;
    QString name;
    QString path;
};
Q_DECLARE_METATYPE(WebCamTarget);

/* static */
UIMachineLogic* UIMachineLogic::create(QObject *pParent,
                                       UISession *pSession,
                                       UIVisualStateType visualStateType)
{
    UIMachineLogic *pLogic = 0;
    switch (visualStateType)
    {
        case UIVisualStateType_Normal:
            pLogic = new UIMachineLogicNormal(pParent, pSession);
            break;
        case UIVisualStateType_Fullscreen:
            pLogic = new UIMachineLogicFullscreen(pParent, pSession);
            break;
        case UIVisualStateType_Seamless:
            pLogic = new UIMachineLogicSeamless(pParent, pSession);
            break;
        case UIVisualStateType_Scale:
            pLogic = new UIMachineLogicScale(pParent, pSession);
            break;

        case UIVisualStateType_Invalid: case UIVisualStateType_All: break; /* Shut up, MSC! */
    }
    return pLogic;
}

/* static */
void UIMachineLogic::destroy(UIMachineLogic *pWhichLogic)
{
    delete pWhichLogic;
}

void UIMachineLogic::prepare()
{
    /* Prepare required features: */
    prepareRequiredFeatures();

    /* Prepare session connections: */
    prepareSessionConnections();

    /* Prepare action groups:
     * Note: This has to be done before prepareActionConnections
     * cause here actions/menus are recreated. */
    prepareActionGroups();
    /* Prepare action connections: */
    prepareActionConnections();

    /* Prepare other connections: */
    prepareOtherConnections();

    /* Prepare handlers: */
    prepareHandlers();

    /* Prepare menu: */
    prepareMenu();

    /* Prepare machine-window(s): */
    prepareMachineWindows();

#ifdef VBOX_WS_MAC
    /* Prepare dock: */
    prepareDock();
#endif /* VBOX_WS_MAC */

#if 0 /* To early! The debugger needs a VM handle to work. So, must be done after power on.  Moved to initializePostPowerUp. */
#ifdef VBOX_WITH_DEBUGGER_GUI
    /* Prepare debugger: */
    prepareDebugger();
#endif /* VBOX_WITH_DEBUGGER_GUI */
#endif

    /* Load settings: */
    loadSettings();

    /* Retranslate logic part: */
    retranslateUi();
}

void UIMachineLogic::cleanup()
{
#ifdef VBOX_WITH_DEBUGGER_GUI
    /* Cleanup debugger: */
    cleanupDebugger();
#endif /* VBOX_WITH_DEBUGGER_GUI */

#ifdef VBOX_WS_MAC
    /* Cleanup dock: */
    cleanupDock();
#endif /* VBOX_WS_MAC */

    /* Cleanup menu: */
    cleanupMenu();

    /* Cleanup machine-window(s): */
    cleanupMachineWindows();

    /* Cleanup handlers: */
    cleanupHandlers();

    /* Cleanup action connections: */
    cleanupActionConnections();
    /* Cleanup action groups: */
    cleanupActionGroups();

    /* Cleanup session connections: */
    cleanupSessionConnections();
}

void UIMachineLogic::initializePostPowerUp()
{
#ifdef VBOX_WITH_DEBUGGER_GUI
    prepareDebugger();
#endif
    sltMachineStateChanged();
    sltAdditionsStateChanged();
    sltMouseCapabilityChanged();
}

UIActionPool* UIMachineLogic::actionPool() const
{
    return uisession()->actionPool();
}

CSession& UIMachineLogic::session() const
{
    return uisession()->session();
}

CMachine& UIMachineLogic::machine() const
{
    return uisession()->machine();
}

CConsole& UIMachineLogic::console() const
{
    return uisession()->console();
}

CDisplay& UIMachineLogic::display() const
{
    return uisession()->display();
}

CGuest& UIMachineLogic::guest() const
{
    return uisession()->guest();
}

CMouse& UIMachineLogic::mouse() const
{
    return uisession()->mouse();
}

CKeyboard& UIMachineLogic::keyboard() const
{
    return uisession()->keyboard();
}

CMachineDebugger& UIMachineLogic::debugger() const
{
    return uisession()->debugger();
}

const QString& UIMachineLogic::machineName() const
{
    return uisession()->machineName();
}

UIMachineWindow* UIMachineLogic::mainMachineWindow() const
{
    /* Null if machine-window(s) not yet created: */
    if (!isMachineWindowsCreated())
        return 0;

    /* First machine-window by default: */
    return machineWindows().value(0);
}

UIMachineWindow* UIMachineLogic::activeMachineWindow() const
{
    /* Null if machine-window(s) not yet created: */
    if (!isMachineWindowsCreated())
        return 0;

    /* Check if there is an active window present: */
    for (int i = 0; i < machineWindows().size(); ++i)
    {
        UIMachineWindow *pIteratedWindow = machineWindows()[i];
        if (pIteratedWindow->isActiveWindow())
            return pIteratedWindow;
    }

    /* Main machine-window by default: */
    return mainMachineWindow();
}

void UIMachineLogic::adjustMachineWindowsGeometry()
{
    /* By default, the only thing we need is to
     * adjust machine-view size(s) if necessary: */
    foreach(UIMachineWindow *pMachineWindow, machineWindows())
        pMachineWindow->adjustMachineViewSize();
}

void UIMachineLogic::sendMachineWindowsSizeHints()
{
    /* By default, the only thing we need is to
     * send machine-view(s) size-hint(s) to the guest: */
    foreach(UIMachineWindow *pMachineWindow, machineWindows())
        pMachineWindow->sendMachineViewSizeHint();
}

#ifdef VBOX_WS_MAC
void UIMachineLogic::updateDockIcon()
{
    if (!isMachineWindowsCreated())
        return;

    if (   m_fIsDockIconEnabled
        && m_pDockIconPreview)
        if(UIMachineView *pView = machineWindows().at(m_DockIconPreviewMonitor)->machineView())
            if (CGImageRef image = pView->vmContentImage())
            {
                m_pDockIconPreview->updateDockPreview(image);
                CGImageRelease(image);
            }
}

void UIMachineLogic::updateDockIconSize(int screenId, int width, int height)
{
    if (!isMachineWindowsCreated())
        return;

    if (   m_fIsDockIconEnabled
        && m_pDockIconPreview
        && m_DockIconPreviewMonitor == screenId)
        m_pDockIconPreview->setOriginalSize(width, height);
}

UIMachineView* UIMachineLogic::dockPreviewView() const
{
    if (   m_fIsDockIconEnabled
        && m_pDockIconPreview)
        return machineWindows().at(m_DockIconPreviewMonitor)->machineView();
    return 0;
}
#endif /* VBOX_WS_MAC */

void UIMachineLogic::sltHandleVBoxSVCAvailabilityChange()
{
    /* Do nothing if VBoxSVC still availabile: */
    if (uiCommon().isVBoxSVCAvailable())
        return;

    /* Warn user about that: */
    msgCenter().warnAboutVBoxSVCUnavailable();

    /* Power VM off: */
    LogRel(("GUI: Request to power VM off due to VBoxSVC is unavailable.\n"));
    uisession()->powerOff(false /* do NOT restore current snapshot */);
}

void UIMachineLogic::sltChangeVisualStateToNormal()
{
    uisession()->setRequestedVisualState(UIVisualStateType_Invalid);
    uisession()->changeVisualState(UIVisualStateType_Normal);
}

void UIMachineLogic::sltChangeVisualStateToFullscreen()
{
    uisession()->setRequestedVisualState(UIVisualStateType_Invalid);
    uisession()->changeVisualState(UIVisualStateType_Fullscreen);
}

void UIMachineLogic::sltChangeVisualStateToSeamless()
{
    uisession()->setRequestedVisualState(UIVisualStateType_Invalid);
    uisession()->changeVisualState(UIVisualStateType_Seamless);
}

void UIMachineLogic::sltChangeVisualStateToScale()
{
    uisession()->setRequestedVisualState(UIVisualStateType_Invalid);
    uisession()->changeVisualState(UIVisualStateType_Scale);
}

void UIMachineLogic::sltMachineStateChanged()
{
    /* Get machine state: */
    KMachineState state = uisession()->machineState();

    /* Update action groups: */
    m_pRunningActions->setEnabled(uisession()->isRunning());
    m_pRunningOrPausedActions->setEnabled(uisession()->isRunning() || uisession()->isPaused());
    m_pRunningOrPausedOrStackedActions->setEnabled(uisession()->isRunning() || uisession()->isPaused() || uisession()->isStuck());

    switch (state)
    {
        case KMachineState_Stuck:
        {
            /* Prevent machine-view from resizing: */
            uisession()->setGuestResizeIgnored(true);
            /* Get log-folder: */
            QString strLogFolder = machine().GetLogFolder();
            /* Take the screenshot for debugging purposes: */
            takeScreenshot(strLogFolder + "/VBox.png", "png");
            /* How should we handle Guru Meditation? */
            switch (gEDataManager->guruMeditationHandlerType(uiCommon().managedVMUuid()))
            {
                /* Ask how to proceed; Power off VM if proposal accepted: */
                case GuruMeditationHandlerType_Default:
                {
                    if (msgCenter().warnAboutGuruMeditation(QDir::toNativeSeparators(strLogFolder)))
                    {
                        LogRel(("GUI: User requested to power VM off on Guru Meditation.\n"));
                        uisession()->powerOff(false /* do NOT restore current snapshot */);
                    }
                    break;
                }
                /* Power off VM silently: */
                case GuruMeditationHandlerType_PowerOff:
                {
                    LogRel(("GUI: Automatic request to power VM off on Guru Meditation.\n"));
                    uisession()->powerOff(false /* do NOT restore current snapshot */);
                    break;
                }
                /* Just ignore it: */
                case GuruMeditationHandlerType_Ignore:
                default:
                    break;
            }
            break;
        }
        case KMachineState_Paused:
        case KMachineState_TeleportingPausedVM:
        {
            QAction *pPauseAction = actionPool()->action(UIActionIndexRT_M_Machine_T_Pause);
            if (!pPauseAction->isChecked())
            {
                /* Was paused from CSession side: */
                pPauseAction->blockSignals(true);
                pPauseAction->setChecked(true);
                pPauseAction->blockSignals(false);
            }
            break;
        }
        case KMachineState_Running:
        case KMachineState_Teleporting:
        case KMachineState_LiveSnapshotting:
        {
            QAction *pPauseAction = actionPool()->action(UIActionIndexRT_M_Machine_T_Pause);
            if (pPauseAction->isChecked())
            {
                /* Was resumed from CSession side: */
                pPauseAction->blockSignals(true);
                pPauseAction->setChecked(false);
                pPauseAction->blockSignals(false);
            }
            break;
        }
        case KMachineState_PoweredOff:
        case KMachineState_Saved:
        case KMachineState_Teleported:
        case KMachineState_Aborted:
        case KMachineState_AbortedSaved:
        {
            /* If not in 'manual-override' mode: */
            if (!uisession()->isManualOverrideMode())
            {
                /* VM has been powered off, saved, teleported or aborted.
                 * We must close Runtime UI: */
                if (uiCommon().isSeparateProcess())
                {
                    /* Hack: The VM process is terminating, so wait a bit to make sure that
                     * the session is unlocked and the GUI process can save extradata
                     * in UIMachine::cleanupMachineLogic.
                     */
                    /** @todo Probably should wait for the session state change event. */
                    KSessionState sessionState = uisession()->session().GetState();
                    int c = 0;
                    while (   sessionState == KSessionState_Locked
                           || sessionState == KSessionState_Unlocking)
                    {
                         if (++c > 50) break;

                         RTThreadSleep(100);
                         sessionState = uisession()->session().GetState();
                    }
                }

                LogRel(("GUI: Request to close Runtime UI because VM is powered off already.\n"));
                uisession()->closeRuntimeUI();
                return;
            }
            break;
        }
        case KMachineState_Saving:
        {
            /* Insert a host combo release if press has been inserted: */
            typeHostKeyComboPressRelease(false);
            break;
        }
#ifdef VBOX_WS_X11
        case KMachineState_Starting:
        case KMachineState_Restoring:
        case KMachineState_TeleportingIn:
        {
            /* The keyboard handler may wish to do some release logging on startup.
             * Tell it that the logger is now active. */
            doXKeyboardLogging(NativeWindowSubsystem::X11GetDisplay());
            break;
        }
#endif
        default:
            break;
    }

#ifdef VBOX_WS_MAC
    /* Update Dock Overlay: */
    updateDockOverlay();
#endif /* VBOX_WS_MAC */
}

void UIMachineLogic::sltAdditionsStateChanged()
{
    /* Update action states: */
    LogRel3(("GUI: UIMachineLogic::sltAdditionsStateChanged: Adjusting actions availability according to GA state.\n"));
    actionPool()->action(UIActionIndexRT_M_View_T_Seamless)->setEnabled(uisession()->isVisualStateAllowed(UIVisualStateType_Seamless) &&
                                                                        uisession()->isGuestSupportsSeamless());
}

void UIMachineLogic::sltMouseCapabilityChanged()
{
    /* Variable falgs: */
    bool fIsMouseSupportsAbsolute = uisession()->isMouseSupportsAbsolute();
    bool fIsMouseSupportsRelative = uisession()->isMouseSupportsRelative();
    bool fIsMouseSupportsTouchScreen = uisession()->isMouseSupportsTouchScreen();
    bool fIsMouseSupportsTouchPad = uisession()->isMouseSupportsTouchPad();
    bool fIsMouseHostCursorNeeded = uisession()->isMouseHostCursorNeeded();

    /* For now MT stuff is not important for MI action: */
    Q_UNUSED(fIsMouseSupportsTouchScreen);
    Q_UNUSED(fIsMouseSupportsTouchPad);

    /* Update action state: */
    QAction *pAction = actionPool()->action(UIActionIndexRT_M_Input_M_Mouse_T_Integration);
    pAction->setEnabled(fIsMouseSupportsAbsolute && fIsMouseSupportsRelative && !fIsMouseHostCursorNeeded);
    if (fIsMouseHostCursorNeeded)
        pAction->setChecked(true);
}

void UIMachineLogic::sltHidLedsSyncStateChanged(bool fEnabled)
{
    m_fIsHidLedsSyncEnabled = fEnabled;
}

void UIMachineLogic::sltDisableHostScreenSaverStateChanged(bool fDisabled)
{
#if defined(VBOX_WS_X11)
    /* Find the methods once and cache them: */
    if (m_methods.isEmpty())
        m_methods = NativeWindowSubsystem::X11FindDBusScrenSaverInhibitMethods();
    NativeWindowSubsystem::X11InhibitUninhibitScrenSaver(fDisabled, m_methods);
#elif defined(VBOX_WS_WIN)
    NativeWindowSubsystem::setScreenSaverActive(fDisabled);
#else
    Q_UNUSED(fDisabled);
#endif
}

void UIMachineLogic::sltKeyboardLedsChanged()
{
    /* Here we have to update host LED lock states using values provided by UISession:
     * [bool] uisession() -> isNumLock(), isCapsLock(), isScrollLock() can be used for that. */

    if (!isHidLedsSyncEnabled())
        return;

    /* Check if we accidentally trying to manipulate LEDs when host LEDs state was deallocated. */
    if (!m_pHostLedsState)
        return;

#if defined(VBOX_WS_MAC)
    DarwinHidDevicesBroadcastLeds(m_pHostLedsState, uisession()->isNumLock(), uisession()->isCapsLock(), uisession()->isScrollLock());
#elif defined(VBOX_WS_WIN)
    if (!winHidLedsInSync(uisession()->isNumLock(), uisession()->isCapsLock(), uisession()->isScrollLock()))
    {
        keyboardHandler()->winSkipKeyboardEvents(true);
        WinHidDevicesBroadcastLeds(uisession()->isNumLock(), uisession()->isCapsLock(), uisession()->isScrollLock());
        keyboardHandler()->winSkipKeyboardEvents(false);
    }
    else
        LogRel2(("GUI: HID LEDs Sync: already in sync\n"));
#else
    LogRelFlow(("UIMachineLogic::sltKeyboardLedsChanged: Updating host LED lock states does not supported on this platform.\n"));
#endif
}

void UIMachineLogic::sltUSBDeviceStateChange(const CUSBDevice &device, bool fIsAttached, const CVirtualBoxErrorInfo &error)
{
    /* Check if USB device have anything to tell us: */
    if (!error.isNull())
    {
        if (fIsAttached)
            UINotificationMessage::cannotAttachUSBDevice(error, uiCommon().usbDetails(device), machineName());
        else
            UINotificationMessage::cannotDetachUSBDevice(error, uiCommon().usbDetails(device), machineName());
    }
}

void UIMachineLogic::sltRuntimeError(bool fIsFatal, const QString &strErrorId, const QString &strMessage)
{
    /* Preprocess known runtime error types: */
    if (strErrorId == "DrvVD_DEKMISSING")
        return askUserForTheDiskEncryptionPasswords();
    else if (strErrorId == "VMBootFail")
    {
        if (!gEDataManager->suppressedMessages().contains(gpConverter->toInternalString(UIExtraDataMetaDefs::DialogType_BootFailure)))
            return showBootFailureDialog();
        else
            return;
    }

    /* Show runtime error: */
    msgCenter().showRuntimeError(console(), fIsFatal, strErrorId, strMessage);
}

#ifdef VBOX_WS_MAC
void UIMachineLogic::sltShowWindows()
{
    for (int i=0; i < machineWindows().size(); ++i)
    {
        UIMachineWindow *pMachineWindow = machineWindows().at(i);
        /* Dunno what Qt thinks a window that has minimized to the dock
         * should be - it is not hidden, neither is it minimized. OTOH it is
         * marked shown and visible, but not activated. This latter isn't of
         * much help though, since at this point nothing is marked activated.
         * I might have overlooked something, but I'm buggered what if I know
         * what. So, I'll just always show & activate the stupid window to
         * make it get out of the dock when the user wishes to show a VM. */
        pMachineWindow->raise();
        pMachineWindow->activateWindow();
    }
}
#endif /* VBOX_WS_MAC */

void UIMachineLogic::sltGuestMonitorChange(KGuestMonitorChangedEventType, ulong, QRect)
{
    LogRel(("GUI: UIMachineLogic: Guest-screen count changed\n"));

    /* Make sure all machine-window(s) have proper geometry: */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        pMachineWindow->showInNecessaryMode();

#ifdef VBOX_WS_MAC
    /* Update dock: */
    updateDock();
#endif
}

void UIMachineLogic::sltHostScreenCountChange()
{
#ifdef VBOX_GUI_WITH_CUSTOMIZATIONS1
    /* Customer request to skip host-screen count change: */
    LogRel(("GUI: UIMachineLogic: Host-screen count change skipped\n"));
#else /* !VBOX_GUI_WITH_CUSTOMIZATIONS1 */
    LogRel(("GUI: UIMachineLogic: Host-screen count changed\n"));

    /* Make sure all machine-window(s) have proper geometry: */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        pMachineWindow->showInNecessaryMode();
#endif /* !VBOX_GUI_WITH_CUSTOMIZATIONS1 */
}

void UIMachineLogic::sltHostScreenGeometryChange()
{
#ifdef VBOX_GUI_WITH_CUSTOMIZATIONS1
    /* Customer request to skip host-screen geometry change: */
    LogRel(("GUI: UIMachineLogic: Host-screen geometry change skipped\n"));
#else /* !VBOX_GUI_WITH_CUSTOMIZATIONS1 */
    LogRel(("GUI: UIMachineLogic: Host-screen geometry changed\n"));

    /* Make sure all machine-window(s) have proper geometry: */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        pMachineWindow->showInNecessaryMode();
#endif /* !VBOX_GUI_WITH_CUSTOMIZATIONS1 */
}

void UIMachineLogic::sltHostScreenAvailableAreaChange()
{
#ifdef VBOX_GUI_WITH_CUSTOMIZATIONS1
    /* Customer request to skip host-screen available-area change: */
    LogRel(("GUI: UIMachineLogic: Host-screen available-area change skipped\n"));
#else /* !VBOX_GUI_WITH_CUSTOMIZATIONS1 */
    LogRel(("GUI: UIMachineLogic: Host-screen available-area changed\n"));

    /* Make sure all machine-window(s) have proper geometry: */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        pMachineWindow->showInNecessaryMode();
#endif /* !VBOX_GUI_WITH_CUSTOMIZATIONS1 */
}

UIMachineLogic::UIMachineLogic(QObject *pParent, UISession *pSession, UIVisualStateType visualStateType)
    : QIWithRetranslateUI3<QObject>(pParent)
    , m_pSession(pSession)
    , m_visualStateType(visualStateType)
    , m_pKeyboardHandler(0)
    , m_pMouseHandler(0)
    , m_pRunningActions(0)
    , m_pRunningOrPausedActions(0)
    , m_pRunningOrPausedOrStackedActions(0)
    , m_pSharedClipboardActions(0)
    , m_pDragAndDropActions(0)
    , m_fIsWindowsCreated(false)
#ifdef VBOX_WITH_DEBUGGER_GUI
    , m_pDbgGui(0)
    , m_pDbgGuiVT(0)
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
    , m_fIsDockIconEnabled(true)
    , m_pDockIconPreview(0)
    , m_pDockPreviewSelectMonitorGroup(0)
    , m_pDockSettingsMenuSeparator(0)
    , m_DockIconPreviewMonitor(0)
    , m_pDockSettingMenuAction(0)
#endif /* VBOX_WS_MAC */
    , m_pHostLedsState(NULL)
    , m_fIsHidLedsSyncEnabled(false)
    , m_pLogViewerDialog(0)
    , m_pFileManagerDialog(0)
    , m_pProcessControlDialog(0)
    , m_pSoftKeyboardDialog(0)
    , m_pVMInformationDialog(0)
{
}

UIMachineLogic::~UIMachineLogic()
{
#if defined(VBOX_WS_X11)
    qDeleteAll(m_methods.begin(), m_methods.end());
    m_methods.clear();
#endif
}

void UIMachineLogic::addMachineWindow(UIMachineWindow *pMachineWindow)
{
    m_machineWindowsList << pMachineWindow;
}

void UIMachineLogic::setKeyboardHandler(UIKeyboardHandler *pKeyboardHandler)
{
    /* Set new handler: */
    m_pKeyboardHandler = pKeyboardHandler;
    /* Connect to session: */
    connect(m_pKeyboardHandler, &UIKeyboardHandler::sigStateChange,
            uisession(), &UISession::setKeyboardState);
}

void UIMachineLogic::setMouseHandler(UIMouseHandler *pMouseHandler)
{
    /* Set new handler: */
    m_pMouseHandler = pMouseHandler;
    /* Connect to session: */
    connect(m_pMouseHandler, &UIMouseHandler::sigStateChange,
            uisession(), &UISession::setMouseState);
}

void UIMachineLogic::retranslateUi()
{
#ifdef VBOX_WS_MAC
    if (m_pDockPreviewSelectMonitorGroup)
    {
        const QList<QAction*> &actions = m_pDockPreviewSelectMonitorGroup->actions();
        for (int i = 0; i < actions.size(); ++i)
        {
            QAction *pAction = actions.at(i);
            pAction->setText(QApplication::translate("UIActionPool", "Preview Monitor %1").arg(pAction->data().toInt() + 1));
        }
    }
#endif /* VBOX_WS_MAC */
    /* Shared Clipboard actions: */
    if (m_pSharedClipboardActions)
    {
        foreach (QAction *pAction, m_pSharedClipboardActions->actions())
            pAction->setText(gpConverter->toString(pAction->data().value<KClipboardMode>()));
    }
    if (m_pDragAndDropActions)
    {
        foreach (QAction *pAction, m_pDragAndDropActions->actions())
            pAction->setText(gpConverter->toString(pAction->data().value<KDnDMode>()));
    }
}

#ifdef VBOX_WS_MAC
void UIMachineLogic::updateDockOverlay()
{
    /* Only to an update to the realtime preview if this is enabled by the user
     * & we are in an state where the framebuffer is likely valid. Otherwise to
     * the overlay stuff only. */
    KMachineState state = uisession()->machineState();
    if (m_fIsDockIconEnabled &&
        (state == KMachineState_Running ||
         state == KMachineState_Paused ||
         state == KMachineState_Teleporting ||
         state == KMachineState_LiveSnapshotting ||
         state == KMachineState_Restoring ||
         state == KMachineState_TeleportingPausedVM ||
         state == KMachineState_TeleportingIn ||
         state == KMachineState_Saving ||
         state == KMachineState_DeletingSnapshotOnline ||
         state == KMachineState_DeletingSnapshotPaused))
        updateDockIcon();
    else if (m_pDockIconPreview)
        m_pDockIconPreview->updateDockOverlay();
}
#endif /* VBOX_WS_MAC */

void UIMachineLogic::prepareSessionConnections()
{
    /* We should watch for VBoxSVC availability changes: */
    connect(&uiCommon(), &UICommon::sigVBoxSVCAvailabilityChange,
            this, &UIMachineLogic::sltHandleVBoxSVCAvailabilityChange);

    /* We should watch for requested modes: */
    connect(uisession(), &UISession::sigInitialized, this, &UIMachineLogic::sltCheckForRequestedVisualStateType, Qt::QueuedConnection);
    connect(uisession(), &UISession::sigAdditionsStateChange, this, &UIMachineLogic::sltCheckForRequestedVisualStateType);

    /* We should watch for console events: */
    connect(uisession(), &UISession::sigMachineStateChange, this, &UIMachineLogic::sltMachineStateChanged);
    connect(uisession(), &UISession::sigAdditionsStateActualChange, this, &UIMachineLogic::sltAdditionsStateChanged);
    connect(uisession(), &UISession::sigMouseCapabilityChange, this, &UIMachineLogic::sltMouseCapabilityChanged);
    connect(uisession(), &UISession::sigKeyboardLedsChange, this, &UIMachineLogic::sltKeyboardLedsChanged);
    connect(uisession(), &UISession::sigUSBDeviceStateChange, this, &UIMachineLogic::sltUSBDeviceStateChange);
    connect(uisession(), &UISession::sigRuntimeError, this, &UIMachineLogic::sltRuntimeError);
#ifdef VBOX_WS_MAC
    connect(uisession(), &UISession::sigShowWindows, this, &UIMachineLogic::sltShowWindows);
#endif /* VBOX_WS_MAC */
    connect(uisession(), &UISession::sigGuestMonitorChange, this, &UIMachineLogic::sltGuestMonitorChange);

    /* We should watch for host-screen-change events: */
    connect(uisession(), &UISession::sigHostScreenCountChange, this, &UIMachineLogic::sltHostScreenCountChange);
    connect(uisession(), &UISession::sigHostScreenGeometryChange, this, &UIMachineLogic::sltHostScreenGeometryChange);
    connect(uisession(), &UISession::sigHostScreenAvailableAreaChange, this, &UIMachineLogic::sltHostScreenAvailableAreaChange);

    /* We should notify about frame-buffer events: */
    connect(this, &UIMachineLogic::sigFrameBufferResize, uisession(), &UISession::sigFrameBufferResize);
}

void UIMachineLogic::prepareActionGroups()
{
    /* Create group for all actions that are enabled only when the VM is running.
     * Note that only actions whose enabled state depends exclusively on the
     * execution state of the VM are added to this group. */
    m_pRunningActions = new QActionGroup(this);
    m_pRunningActions->setExclusive(false);

    /* Create group for all actions that are enabled when the VM is running or paused.
     * Note that only actions whose enabled state depends exclusively on the
     * execution state of the VM are added to this group. */
    m_pRunningOrPausedActions = new QActionGroup(this);
    m_pRunningOrPausedActions->setExclusive(false);

    /* Create group for all actions that are enabled when the VM is running or paused or stucked.
     * Note that only actions whose enabled state depends exclusively on the
     * execution state of the VM are added to this group. */
    m_pRunningOrPausedOrStackedActions = new QActionGroup(this);
    m_pRunningOrPausedOrStackedActions->setExclusive(false);

    /* Move actions into running actions group: */
    m_pRunningActions->addAction(actionPool()->action(UIActionIndexRT_M_Machine_S_Reset));
    m_pRunningActions->addAction(actionPool()->action(UIActionIndexRT_M_Machine_S_Shutdown));
    m_pRunningActions->addAction(actionPool()->action(UIActionIndexRT_M_View_T_Fullscreen));
    m_pRunningActions->addAction(actionPool()->action(UIActionIndexRT_M_View_T_Seamless));
    m_pRunningActions->addAction(actionPool()->action(UIActionIndexRT_M_View_T_Scale));
    m_pRunningActions->addAction(actionPool()->action(UIActionIndexRT_M_View_T_GuestAutoresize));
    m_pRunningActions->addAction(actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard_S_TypeCAD));
#ifdef VBOX_WS_X11
    m_pRunningActions->addAction(actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard_S_TypeCABS));
#endif /* VBOX_WS_X11 */
    m_pRunningActions->addAction(actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard_S_TypeCtrlBreak));
    m_pRunningActions->addAction(actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard_S_TypeInsert));
    m_pRunningActions->addAction(actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard_S_TypePrintScreen));
    m_pRunningActions->addAction(actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard_S_TypeAltPrintScreen));
    m_pRunningActions->addAction(actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard_T_TypeHostKeyCombo));

    /* Move actions into running-n-paused actions group: */
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Machine_S_Detach));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Machine_S_SaveState));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Machine_S_Settings));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Machine_S_TakeSnapshot));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Machine_S_ShowInformation));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Machine_T_Pause));
#ifndef VBOX_WS_MAC
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_View_S_MinimizeWindow));
#endif /* !VBOX_WS_MAC */
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_View_S_AdjustWindow));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_View_S_TakeScreenshot));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_View_M_Recording));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_View_M_Recording_S_Settings));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_View_M_Recording_T_Start));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_View_T_VRDEServer));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_View_M_MenuBar));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_View_M_MenuBar_S_Settings));
#ifndef VBOX_WS_MAC
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_View_M_MenuBar_T_Visibility));
#endif /* !VBOX_WS_MAC */
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_View_M_StatusBar));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_View_M_StatusBar_S_Settings));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_View_M_StatusBar_T_Visibility));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard_S_Settings));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard_S_SoftKeyboard));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Input_M_Mouse));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Input_M_Mouse_T_Integration));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Devices_M_HardDrives));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Devices_M_HardDrives_S_Settings));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Devices_M_OpticalDevices));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Devices_M_FloppyDevices));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Devices_M_Audio));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Devices_M_Audio_T_Output));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Devices_M_Audio_T_Output));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Devices_M_Network));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Devices_M_Network_S_Settings));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Devices_M_USBDevices));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Devices_M_USBDevices_S_Settings));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Devices_M_WebCams));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Devices_M_SharedClipboard));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Devices_M_DragAndDrop));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Devices_M_SharedFolders));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Devices_M_SharedFolders_S_Settings));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Devices_S_InsertGuestAdditionsDisk));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndexRT_M_Devices_S_UpgradeGuestAdditions));
#ifdef VBOX_WS_MAC
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndex_M_Window));
    m_pRunningOrPausedActions->addAction(actionPool()->action(UIActionIndex_M_Window_S_Minimize));
#endif /* VBOX_WS_MAC */

    /* Move actions into running-n-paused-n-stucked actions group: */
    m_pRunningOrPausedOrStackedActions->addAction(actionPool()->action(UIActionIndexRT_M_Machine_S_PowerOff));
}

void UIMachineLogic::prepareActionConnections()
{
    /* 'Application' actions connection: */
    connect(actionPool()->action(UIActionIndex_M_Application_S_Preferences), &UIAction::triggered,
            this, &UIMachineLogic::sltOpenPreferencesDialogDefault, Qt::UniqueConnection);
    connect(actionPool()->action(UIActionIndex_M_Application_S_Close), &UIAction::triggered,
            this, &UIMachineLogic::sltClose, Qt::QueuedConnection);

    /* 'Machine' actions connections: */
    connect(actionPool()->action(UIActionIndexRT_M_Machine_S_Settings), &UIAction::triggered,
            this, &UIMachineLogic::sltOpenSettingsDialogDefault);
    connect(actionPool()->action(UIActionIndexRT_M_Machine_S_TakeSnapshot), &UIAction::triggered,
            this, &UIMachineLogic::sltTakeSnapshot);
    connect(actionPool()->action(UIActionIndexRT_M_Machine_S_ShowInformation), &UIAction::triggered,
            this, &UIMachineLogic::sltShowInformationDialog);
    connect(actionPool()->action(UIActionIndexRT_M_Machine_S_ShowFileManager), &UIAction::triggered,
            this, &UIMachineLogic::sltShowFileManagerDialog);
    connect(actionPool()->action(UIActionIndexRT_M_Machine_T_Pause), &UIAction::toggled,
            this, &UIMachineLogic::sltPause);
    connect(actionPool()->action(UIActionIndexRT_M_Machine_S_Reset), &UIAction::triggered,
            this, &UIMachineLogic::sltReset);
    connect(actionPool()->action(UIActionIndexRT_M_Machine_S_Detach), &UIAction::triggered,
            this, &UIMachineLogic::sltDetach, Qt::QueuedConnection);
    connect(actionPool()->action(UIActionIndexRT_M_Machine_S_SaveState), &UIAction::triggered,
            this, &UIMachineLogic::sltSaveState, Qt::QueuedConnection);
    connect(actionPool()->action(UIActionIndexRT_M_Machine_S_Shutdown), &UIAction::triggered,
            this, &UIMachineLogic::sltShutdown);
    connect(actionPool()->action(UIActionIndexRT_M_Machine_S_PowerOff), &UIAction::triggered,
            this, &UIMachineLogic::sltPowerOff, Qt::QueuedConnection);
    connect(actionPool()->action(UIActionIndexRT_M_Machine_S_ShowLogDialog), &UIAction::triggered,
            this, &UIMachineLogic::sltShowLogDialog);

    /* 'View' actions connections: */
#ifndef VBOX_WS_MAC
    connect(actionPool()->action(UIActionIndexRT_M_View_S_MinimizeWindow), &UIAction::triggered,
            this, &UIMachineLogic::sltMinimizeActiveMachineWindow, Qt::QueuedConnection);
#endif /* !VBOX_WS_MAC */
    connect(actionPool()->action(UIActionIndexRT_M_View_S_AdjustWindow), &UIAction::triggered,
            this, &UIMachineLogic::sltAdjustMachineWindows);
    connect(actionPool()->action(UIActionIndexRT_M_View_T_GuestAutoresize), &UIAction::toggled,
            this, &UIMachineLogic::sltToggleGuestAutoresize);
    connect(actionPool()->action(UIActionIndexRT_M_View_S_TakeScreenshot), &UIAction::triggered,
            this, &UIMachineLogic::sltTakeScreenshot);
    connect(actionPool()->action(UIActionIndexRT_M_View_M_Recording_S_Settings), &UIAction::triggered,
            this, &UIMachineLogic::sltOpenRecordingOptions);
    connect(actionPool()->action(UIActionIndexRT_M_View_M_Recording_T_Start), &UIAction::toggled,
            this, &UIMachineLogic::sltToggleRecording);
    connect(actionPool()->action(UIActionIndexRT_M_View_T_VRDEServer), &UIAction::toggled,
            this, &UIMachineLogic::sltToggleVRDE);

    /* 'Input' actions connections: */
    connect(actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard_S_Settings), &UIAction::triggered,
            this, &UIMachineLogic::sltShowKeyboardSettings);
    connect(actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard_S_SoftKeyboard), &UIAction::triggered,
            this, &UIMachineLogic::sltShowSoftKeyboard);
    connect(actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard_S_TypeCAD), &UIAction::triggered,
            this, &UIMachineLogic::sltTypeCAD);
#ifdef VBOX_WS_X11
    connect(actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard_S_TypeCABS), &UIAction::triggered,
            this, &UIMachineLogic::sltTypeCABS);
#endif /* VBOX_WS_X11 */
    connect(actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard_S_TypeCtrlBreak), &UIAction::triggered,
            this, &UIMachineLogic::sltTypeCtrlBreak);
    connect(actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard_S_TypeInsert), &UIAction::triggered,
            this, &UIMachineLogic::sltTypeInsert);
    connect(actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard_S_TypePrintScreen), &UIAction::triggered,
            this, &UIMachineLogic::sltTypePrintScreen);
    connect(actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard_S_TypeAltPrintScreen), &UIAction::triggered,
            this, &UIMachineLogic::sltTypeAltPrintScreen);
    connect(actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard_T_TypeHostKeyCombo), &UIAction::toggled,
            this, &UIMachineLogic::sltTypeHostKeyComboPressRelease);
    connect(actionPool()->action(UIActionIndexRT_M_Input_M_Mouse_T_Integration), &UIAction::toggled,
            this, &UIMachineLogic::sltToggleMouseIntegration);

    /* 'Devices' actions connections: */
    connect(actionPool(), &UIActionPool::sigNotifyAboutMenuPrepare, this, &UIMachineLogic::sltHandleMenuPrepare);
    connect(actionPool()->action(UIActionIndexRT_M_Devices_M_HardDrives_S_Settings), &UIAction::triggered,
            this, &UIMachineLogic::sltOpenSettingsDialogStorage);
    connect(actionPool()->action(UIActionIndexRT_M_Devices_M_Audio_T_Output), &UIAction::toggled,
            this, &UIMachineLogic::sltToggleAudioOutput);
    connect(actionPool()->action(UIActionIndexRT_M_Devices_M_Audio_T_Input), &UIAction::toggled,
            this, &UIMachineLogic::sltToggleAudioInput);
    connect(actionPool()->action(UIActionIndexRT_M_Devices_M_Network_S_Settings), &UIAction::triggered,
            this, &UIMachineLogic::sltOpenSettingsDialogNetwork);
    connect(actionPool()->action(UIActionIndexRT_M_Devices_M_USBDevices_S_Settings), &UIAction::triggered,
            this, &UIMachineLogic::sltOpenSettingsDialogUSBDevices);
    connect(actionPool()->action(UIActionIndexRT_M_Devices_M_SharedFolders_S_Settings), &UIAction::triggered,
            this, &UIMachineLogic::sltOpenSettingsDialogSharedFolders);
    connect(actionPool()->action(UIActionIndexRT_M_Devices_S_InsertGuestAdditionsDisk), &UIAction::triggered,
            this, &UIMachineLogic::sltInstallGuestAdditions);
    connect(actionPool()->action(UIActionIndexRT_M_Devices_S_UpgradeGuestAdditions), &UIAction::triggered,
            this, &UIMachineLogic::sltInstallGuestAdditions);

    /* 'Help' menu 'Contents' action. Done here since we react differently to this action
     * in manager and runtime UI: */
    connect(actionPool()->action(UIActionIndex_Simple_Contents), &UIAction::triggered,
            &msgCenter(), &UIMessageCenter::sltShowHelpHelpDialog);

#ifdef VBOX_WITH_DEBUGGER_GUI
    /* 'Debug' actions connections: */
    connect(actionPool()->action(UIActionIndexRT_M_Debug_S_ShowStatistics), &UIAction::triggered,
            this, &UIMachineLogic::sltShowDebugStatistics);
    connect(actionPool()->action(UIActionIndexRT_M_Debug_S_ShowCommandLine), &UIAction::triggered,
            this, &UIMachineLogic::sltShowDebugCommandLine);
    connect(actionPool()->action(UIActionIndexRT_M_Debug_T_Logging), &UIAction::toggled,
            this, &UIMachineLogic::sltLoggingToggled);
    connect(actionPool()->action(UIActionIndexRT_M_Debug_S_GuestControlConsole), &UIAction::triggered,
            this, &UIMachineLogic::sltShowGuestControlConsoleDialog);
#endif /* VBOX_WITH_DEBUGGER_GUI */

#ifdef VBOX_WS_MAC
    /* 'Window' action connections: */
    connect(actionPool()->action(UIActionIndex_M_Window_S_Minimize), &UIAction::triggered,
            this, &UIMachineLogic::sltMinimizeActiveMachineWindow, Qt::QueuedConnection);
#endif /* VBOX_WS_MAC */
}

void UIMachineLogic::prepareOtherConnections()
{
    /* Extra-data connections: */
    connect(gEDataManager, &UIExtraDataManager::sigVisualStateChange,
            this, &UIMachineLogic::sltHandleVisualStateChange);

    /* UICommon connections: */
    connect(&uiCommon(), &UICommon::sigAskToCommitData,
            this, &UIMachineLogic::sltHandleCommitData);
}

void UIMachineLogic::prepareHandlers()
{
    /* Prepare menu update-handlers: */
    m_menuUpdateHandlers[UIActionIndexRT_M_Devices_M_OpticalDevices] =  &UIMachineLogic::updateMenuDevicesStorage;
    m_menuUpdateHandlers[UIActionIndexRT_M_Devices_M_FloppyDevices] =   &UIMachineLogic::updateMenuDevicesStorage;
    m_menuUpdateHandlers[UIActionIndexRT_M_Devices_M_Network] =         &UIMachineLogic::updateMenuDevicesNetwork;
    m_menuUpdateHandlers[UIActionIndexRT_M_Devices_M_USBDevices] =      &UIMachineLogic::updateMenuDevicesUSB;
    m_menuUpdateHandlers[UIActionIndexRT_M_Devices_M_WebCams] =         &UIMachineLogic::updateMenuDevicesWebCams;
    m_menuUpdateHandlers[UIActionIndexRT_M_Devices_M_SharedClipboard] = &UIMachineLogic::updateMenuDevicesSharedClipboard;
    m_menuUpdateHandlers[UIActionIndexRT_M_Devices_M_DragAndDrop] =     &UIMachineLogic::updateMenuDevicesDragAndDrop;
#ifdef VBOX_WITH_DEBUGGER_GUI
    m_menuUpdateHandlers[UIActionIndexRT_M_Debug] =                     &UIMachineLogic::updateMenuDebug;
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
    m_menuUpdateHandlers[UIActionIndex_M_Window] =                      &UIMachineLogic::updateMenuWindow;
#endif /* VBOX_WS_MAC */

    /* Create keyboard/mouse handlers: */
    setKeyboardHandler(UIKeyboardHandler::create(this, visualStateType()));
    setMouseHandler(UIMouseHandler::create(this, visualStateType()));
    /* Update UI session values with current: */
    uisession()->setKeyboardState(keyboardHandler()->state());
    uisession()->setMouseState(mouseHandler()->state());
}

#ifdef VBOX_WS_MAC
void UIMachineLogic::prepareDock()
{
    QMenu *pDockMenu = actionPool()->action(UIActionIndexRT_M_Dock)->menu();
    /* Clear the menu to get rid of any previously added actions and separators: */
    pDockMenu->clear();

    /* Add all the 'Machine' menu entries to the 'Dock' menu: */
    QList<QAction*> actions = actionPool()->action(UIActionIndexRT_M_Machine)->menu()->actions();
    m_dockMachineMenuActions.clear();
    for (int i=0; i < actions.size(); ++i)
    {
        /* Check if we really have correct action: */
        UIAction *pAction = qobject_cast<UIAction*>(actions.at(i));
        /* Skip incorrect actions: */
        if (!pAction)
            continue;
        /* Skip actions which have 'role' (to prevent consuming): */
        if (pAction->menuRole() != QAction::NoRole)
            continue;
        /* Skip actions which have menu (to prevent consuming): */
        if (qobject_cast<UIActionMenu*>(pAction))
            continue;
        if (!pAction->isAllowed())
            continue;
        pDockMenu->addAction(actions.at(i));
        m_dockMachineMenuActions.push_back(actions.at(i));
    }
    if (!m_dockMachineMenuActions.empty())
    {
        m_dockMachineMenuActions.push_back(pDockMenu->addSeparator());
    }

    QMenu *pDockSettingsMenu = actionPool()->action(UIActionIndexRT_M_Dock_M_DockSettings)->menu();
    /* Clear the menu to get rid of any previously added actions and separators: */
    pDockSettingsMenu->clear();
    QActionGroup *pDockPreviewModeGroup = new QActionGroup(this);
    QAction *pDockDisablePreview = actionPool()->action(UIActionIndexRT_M_Dock_M_DockSettings_T_DisableMonitor);
    pDockPreviewModeGroup->addAction(pDockDisablePreview);
    QAction *pDockEnablePreviewMonitor = actionPool()->action(UIActionIndexRT_M_Dock_M_DockSettings_T_PreviewMonitor);
    pDockPreviewModeGroup->addAction(pDockEnablePreviewMonitor);
    pDockSettingsMenu->addActions(pDockPreviewModeGroup->actions());

    connect(pDockPreviewModeGroup, &QActionGroup::triggered, this, &UIMachineLogic::sltDockPreviewModeChanged);
    connect(gEDataManager, &UIExtraDataManager::sigDockIconAppearanceChange, this, &UIMachineLogic::sltChangeDockIconUpdate);

    /* Get dock icon disable overlay action: */
    QAction *pDockIconDisableOverlay = actionPool()->action(UIActionIndexRT_M_Dock_M_DockSettings_T_DisableOverlay);
    /* Prepare dock icon disable overlay action with initial data: */
    pDockIconDisableOverlay->setChecked(gEDataManager->dockIconDisableOverlay(uiCommon().managedVMUuid()));
    /* Connect dock icon disable overlay related signals: */
    connect(pDockIconDisableOverlay, &QAction::triggered, this, &UIMachineLogic::sltDockIconDisableOverlayChanged);
    connect(gEDataManager, &UIExtraDataManager::sigDockIconOverlayAppearanceChange,
            this, &UIMachineLogic::sltChangeDockIconOverlayAppearance);
    /* Add dock icon disable overlay action to the dock settings menu: */
    pDockSettingsMenu->addAction(pDockIconDisableOverlay);

    /* If we have more than one visible window: */
    const QList<int> visibleWindowsList = uisession()->listOfVisibleWindows();
    const int cVisibleGuestScreens = visibleWindowsList.size();
    if (cVisibleGuestScreens > 1)
    {
        /* Add separator: */
        m_pDockSettingsMenuSeparator = pDockSettingsMenu->addSeparator();

        int extraDataUpdateMonitor = gEDataManager->realtimeDockIconUpdateMonitor(uiCommon().managedVMUuid());
        if (visibleWindowsList.contains(extraDataUpdateMonitor))
            m_DockIconPreviewMonitor = extraDataUpdateMonitor;
        else
            m_DockIconPreviewMonitor = visibleWindowsList.at(cVisibleGuestScreens - 1);

        m_pDockPreviewSelectMonitorGroup = new QActionGroup(this);

        /* And dock preview actions: */
        for (int i = 0; i < cVisibleGuestScreens; ++i)
        {
            QAction *pAction = new QAction(m_pDockPreviewSelectMonitorGroup);
            pAction->setCheckable(true);
            pAction->setData(visibleWindowsList.at(i));
            if (m_DockIconPreviewMonitor == visibleWindowsList.at(i))
                pAction->setChecked(true);
        }
        pDockSettingsMenu->addActions(m_pDockPreviewSelectMonitorGroup->actions());
        connect(m_pDockPreviewSelectMonitorGroup, &QActionGroup::triggered,
                this, &UIMachineLogic::sltDockPreviewMonitorChanged);
    }

    m_pDockSettingMenuAction = pDockMenu->addMenu(pDockSettingsMenu);

    /* Add it to the dock: */
    pDockMenu->setAsDockMenu();

    /* Now the dock icon preview: */
    QPixmap pixmap = generalIconPool().userMachinePixmap(machine(), QSize(42, 42));
    if (pixmap.isNull())
        pixmap = generalIconPool().guestOSTypePixmap(guest().GetOSTypeId(), QSize(42, 42));
    m_pDockIconPreview = new UIDockIconPreview(uisession(), pixmap);

    /* Should the dock-icon be updated at runtime? */
    bool fEnabled = gEDataManager->realtimeDockIconUpdateEnabled(uiCommon().managedVMUuid());
    if (fEnabled)
        pDockEnablePreviewMonitor->setChecked(true);
    else
    {
        pDockDisablePreview->setChecked(true);
        if(m_pDockPreviewSelectMonitorGroup)
            m_pDockPreviewSelectMonitorGroup->setEnabled(false);
    }
    setDockIconPreviewEnabled(fEnabled);
    updateDockOverlay();
}

void UIMachineLogic::updateDock()
{
    QMenu *pDockSettingsMenu = actionPool()->action(UIActionIndexRT_M_Dock_M_DockSettings)->menu();
    AssertReturnVoid(pDockSettingsMenu);

    QMenu *pDockMenu = actionPool()->action(UIActionIndexRT_M_Dock)->menu();
    AssertReturnVoid(pDockMenu);

    /* Clean previous machine menu actions: */
    for (int i=0; i < m_dockMachineMenuActions.size(); ++i)
    {
        pDockMenu->removeAction(m_dockMachineMenuActions.at(i));
        if (m_dockMachineMenuActions.at(i)->isSeparator())
            delete m_dockMachineMenuActions[i];
    }
    m_dockMachineMenuActions.clear();

    /* Determine the list of actions to be inserted: */
    QList<QAction*> actions = actionPool()->action(UIActionIndexRT_M_Machine)->menu()->actions();
    QList<QAction*> allowedActions;
    for (int i=0; i < actions.size(); ++i)
    {
        /* Check if we really have correct action: */
        UIAction *pAction = qobject_cast<UIAction*>(actions.at(i));
        /* Skip incorrect actions: */
        if (!pAction)
            continue;
        /* Skip actions which have 'role' (to prevent consuming): */
        if (pAction->menuRole() != QAction::NoRole)
            continue;
        /* Skip actions which have menu (to prevent consuming): */
        if (qobject_cast<UIActionMenu*>(pAction))
            continue;
        if (!pAction->isAllowed())
            continue;
        allowedActions.push_back(actions.at(i));
    }

    if (!allowedActions.empty())
    {
        QAction *pSeparator = new QAction(pDockMenu);
        pSeparator->setSeparator(true);
        allowedActions.push_back(pSeparator);
        pDockMenu->insertActions(m_pDockSettingMenuAction, allowedActions);
        m_dockMachineMenuActions = allowedActions;
    }

    /* Clean the previous preview actions: */
    if (m_pDockPreviewSelectMonitorGroup)
    {
        QList<QAction*> previewActions = m_pDockPreviewSelectMonitorGroup->actions();
        foreach (QAction *pAction, previewActions)
        {
            pDockSettingsMenu->removeAction(pAction);
            m_pDockPreviewSelectMonitorGroup->removeAction(pAction);
            delete pAction;
        }
    }
    const QList<int> visibleWindowsList = uisession()->listOfVisibleWindows();
    const int cVisibleGuestScreens = visibleWindowsList.size();
    if (cVisibleGuestScreens > 1)
    {
        if (!m_pDockPreviewSelectMonitorGroup)
            m_pDockPreviewSelectMonitorGroup = new QActionGroup(this);
        /* Only if currently selected monitor for icon preview is not enabled: */
        if (!visibleWindowsList.contains(m_DockIconPreviewMonitor))
        {
            int iExtraDataUpdateMonitor = gEDataManager->realtimeDockIconUpdateMonitor(uiCommon().managedVMUuid());
            if (visibleWindowsList.contains(iExtraDataUpdateMonitor))
                m_DockIconPreviewMonitor = iExtraDataUpdateMonitor;
            else
                m_DockIconPreviewMonitor = visibleWindowsList.at(cVisibleGuestScreens - 1);
        }
        if (!m_pDockSettingsMenuSeparator)
            m_pDockSettingsMenuSeparator = pDockSettingsMenu->addSeparator();
        for (int i=0; i < cVisibleGuestScreens; ++i)
        {
            QAction *pAction = new QAction(m_pDockPreviewSelectMonitorGroup);
            pAction->setCheckable(true);
            pAction->setData(visibleWindowsList.at(i));
            pAction->setText(QApplication::translate("UIActionPool", "Preview Monitor %1").arg(pAction->data().toInt() + 1));
            if (m_DockIconPreviewMonitor == visibleWindowsList.at(i))
                pAction->setChecked(true);
        }
        pDockSettingsMenu->addActions(m_pDockPreviewSelectMonitorGroup->actions());
        connect(m_pDockPreviewSelectMonitorGroup, &QActionGroup::triggered,
                this, &UIMachineLogic::sltDockPreviewMonitorChanged);
    }
    else
    {
        m_DockIconPreviewMonitor = 0;
        /* Remove the seperator as well: */
        if (m_pDockSettingsMenuSeparator)
        {
            pDockSettingsMenu->removeAction(m_pDockSettingsMenuSeparator);
            delete m_pDockSettingsMenuSeparator;
            m_pDockSettingsMenuSeparator = 0;
        }
    }
}
#endif /* VBOX_WS_MAC */

#ifdef VBOX_WITH_DEBUGGER_GUI
void UIMachineLogic::prepareDebugger()
{
    if (uiCommon().isDebuggerAutoShowEnabled())
    {
        if (uiCommon().isDebuggerAutoShowStatisticsEnabled())
            sltShowDebugStatistics();
        if (uiCommon().isDebuggerAutoShowCommandLineEnabled())
            sltShowDebugCommandLine();
    }
}
#endif /* VBOX_WITH_DEBUGGER_GUI */

void UIMachineLogic::loadSettings()
{
#if defined(VBOX_WS_MAC) || defined(VBOX_WS_WIN)
    /* Read cached extra-data value: */
    m_fIsHidLedsSyncEnabled = gEDataManager->hidLedsSyncState(uiCommon().managedVMUuid());
    /* Subscribe to extra-data changes to be able to enable/disable feature dynamically: */
    connect(gEDataManager, &UIExtraDataManager::sigHidLedsSyncStateChange, this, &UIMachineLogic::sltHidLedsSyncStateChanged);
#endif /* VBOX_WS_MAC || VBOX_WS_WIN */
    /* HID LEDs sync initialization: */
    sltSwitchKeyboardLedsToGuestLeds();
    /* */
#if defined(VBOX_WS_X11) || defined(VBOX_WS_WIN)
    connect(gEDataManager, &UIExtraDataManager::sigDisableHostScreenSaverStateChange,
            this, &UIMachineLogic::sltDisableHostScreenSaverStateChanged);
    sltDisableHostScreenSaverStateChanged(gEDataManager->disableHostScreenSaver());
#endif
}

#ifdef VBOX_WITH_DEBUGGER_GUI
void UIMachineLogic::cleanupDebugger()
{
    /* Close debugger: */
    dbgDestroy();
}
#endif /* VBOX_WITH_DEBUGGER_GUI */

#ifdef VBOX_WS_MAC
void UIMachineLogic::cleanupDock()
{
    if (m_pDockIconPreview)
    {
        delete m_pDockIconPreview;
        m_pDockIconPreview = 0;
    }
}
#endif /* VBOX_WS_MAC */

void UIMachineLogic::cleanupHandlers()
{
    /* Cleanup mouse-handler: */
    UIMouseHandler::destroy(mouseHandler());

    /* Cleanup keyboard-handler: */
    UIKeyboardHandler::destroy(keyboardHandler());
}

void UIMachineLogic::cleanupSessionConnections()
{
    /* We should stop watching for VBoxSVC availability changes: */
    disconnect(&uiCommon(), &UICommon::sigVBoxSVCAvailabilityChange,
               this, &UIMachineLogic::sltHandleVBoxSVCAvailabilityChange);

    /* We should stop watching for requested modes: */
    disconnect(uisession(), &UISession::sigInitialized, this, &UIMachineLogic::sltCheckForRequestedVisualStateType);
    disconnect(uisession(), &UISession::sigAdditionsStateChange, this, &UIMachineLogic::sltCheckForRequestedVisualStateType);

    /* We should stop watching for console events: */
    disconnect(uisession(), &UISession::sigMachineStateChange, this, &UIMachineLogic::sltMachineStateChanged);
    disconnect(uisession(), &UISession::sigAdditionsStateActualChange, this, &UIMachineLogic::sltAdditionsStateChanged);
    disconnect(uisession(), &UISession::sigMouseCapabilityChange, this, &UIMachineLogic::sltMouseCapabilityChanged);
    disconnect(uisession(), &UISession::sigKeyboardLedsChange, this, &UIMachineLogic::sltKeyboardLedsChanged);
    disconnect(uisession(), &UISession::sigUSBDeviceStateChange, this, &UIMachineLogic::sltUSBDeviceStateChange);
    disconnect(uisession(), &UISession::sigRuntimeError, this, &UIMachineLogic::sltRuntimeError);
#ifdef VBOX_WS_MAC
    disconnect(uisession(), &UISession::sigShowWindows, this, &UIMachineLogic::sltShowWindows);
#endif /* VBOX_WS_MAC */
    disconnect(uisession(), &UISession::sigGuestMonitorChange, this, &UIMachineLogic::sltGuestMonitorChange);

    /* We should stop watching for host-screen-change events: */
    disconnect(uisession(), &UISession::sigHostScreenCountChange, this, &UIMachineLogic::sltHostScreenCountChange);
    disconnect(uisession(), &UISession::sigHostScreenGeometryChange, this, &UIMachineLogic::sltHostScreenGeometryChange);
    disconnect(uisession(), &UISession::sigHostScreenAvailableAreaChange, this, &UIMachineLogic::sltHostScreenAvailableAreaChange);

    /* We should stop notify about frame-buffer events: */
    disconnect(this, &UIMachineLogic::sigFrameBufferResize, uisession(), &UISession::sigFrameBufferResize);
}

bool UIMachineLogic::eventFilter(QObject *pWatched, QEvent *pEvent)
{
    /* Handle machine-window events: */
    if (UIMachineWindow *pMachineWindow = qobject_cast<UIMachineWindow*>(pWatched))
    {
        /* Make sure this window still registered: */
        if (isMachineWindowsCreated() && m_machineWindowsList.contains(pMachineWindow))
        {
            switch (pEvent->type())
            {
                /* Handle *window activated* event: */
                case QEvent::WindowActivate:
                {
#ifdef VBOX_WS_WIN
                    /* We should save current lock states as *previous* and
                     * set current lock states to guest values we have,
                     * As we have no ipc between threads of different VMs
                     * we are using 100ms timer as lazy sync timout: */

                    /* On Windows host we should do that only in case if sync
                     * is enabled. Otherwise, keyboardHandler()->winSkipKeyboardEvents(false)
                     * won't be called in sltSwitchKeyboardLedsToGuestLeds() and guest
                     * will loose keyboard input forever. */
                    if (isHidLedsSyncEnabled())
                    {
                        keyboardHandler()->winSkipKeyboardEvents(true);
                        QTimer::singleShot(100, this, SLOT(sltSwitchKeyboardLedsToGuestLeds()));
                    }
#else /* VBOX_WS_WIN */
                    /* Trigger callback synchronously for now! */
                    sltSwitchKeyboardLedsToGuestLeds();
#endif /* !VBOX_WS_WIN */
                    break;
                }
                /* Handle *window deactivated* event: */
                case QEvent::WindowDeactivate:
                {
                    /* We should restore lock states to *previous* known: */
                    sltSwitchKeyboardLedsToPreviousLeds();
                    break;
                }
                /* Default: */
                default: break;
            }
        }
    }
    /* Call to base-class: */
    return QIWithRetranslateUI3<QObject>::eventFilter(pWatched, pEvent);
}

void UIMachineLogic::sltHandleMenuPrepare(int iIndex, QMenu *pMenu)
{
    /* Update if there is update-handler: */
    if (m_menuUpdateHandlers.contains(iIndex))
        (this->*(m_menuUpdateHandlers.value(iIndex)))(pMenu);
}

void UIMachineLogic::sltOpenPreferencesDialog(const QString &strCategory /* = QString() */,
                                              const QString &strControl /* = QString() */)
{
    /* Do not process if window(s) missed! */
    if (!isMachineWindowsCreated())
        return;

    /* Create instance if not yet created: */
    if (!m_settings.contains(UISettingsDialog::DialogType_Global))
    {
        m_settings[UISettingsDialog::DialogType_Global] = new UISettingsDialogGlobal(activeMachineWindow(),
                                                                                     strCategory,
                                                                                     strControl);
        connect(m_settings[UISettingsDialog::DialogType_Global], &UISettingsDialogGlobal::sigClose,
                this, &UIMachineLogic::sltClosePreferencesDialog);
        m_settings.value(UISettingsDialog::DialogType_Global)->load();
    }

    /* Expose instance: */
    UIDesktopWidgetWatchdog::restoreWidget(m_settings.value(UISettingsDialog::DialogType_Global));
}

void UIMachineLogic::sltClosePreferencesDialog()
{
    /* Remove instance if exist: */
    delete m_settings.take(UISettingsDialog::DialogType_Global);
}

void UIMachineLogic::sltClose()
{
    /* Do not process if window(s) missed! */
    if (!isMachineWindowsCreated())
        return;

    /* Do not close machine-window in 'manual-override' mode: */
    if (uisession()->isManualOverrideMode())
        return;

    /* First, we have to close/hide any opened modal & popup application widgets.
     * We have to make sure such window is hidden even if close-event was rejected.
     * We are re-throwing this slot if any widget present to test again.
     * If all opened widgets are closed/hidden, we can try to close machine-window: */
    QWidget *pWidget = QApplication::activeModalWidget() ? QApplication::activeModalWidget() :
                       QApplication::activePopupWidget() ? QApplication::activePopupWidget() : 0;
    if (pWidget)
    {
        /* Closing/hiding all we found: */
        pWidget->close();
        if (!pWidget->isHidden())
            pWidget->hide();
        QTimer::singleShot(0, this, SLOT(sltClose()));
        return;
    }

    /* Try to close active machine-window: */
    LogRel(("GUI: Request to close active machine-window.\n"));
    activeMachineWindow()->close();
}

void UIMachineLogic::sltOpenSettingsDialog(const QString &strCategory /* = QString() */,
                                           const QString &strControl /* = QString()*/)
{
    /* Do not process if window(s) missed! */
    if (!isMachineWindowsCreated())
        return;

    /* Create instance if not yet created: */
    if (!m_settings.contains(UISettingsDialog::DialogType_Machine))
    {
        m_settings[UISettingsDialog::DialogType_Machine] = new UISettingsDialogMachine(activeMachineWindow(),
                                                                                       machine().GetId(),
                                                                                       actionPool(),
                                                                                       strCategory,
                                                                                       strControl);
        connect(m_settings[UISettingsDialog::DialogType_Machine], &UISettingsDialogGlobal::sigClose,
                this, &UIMachineLogic::sltCloseSettingsDialog);
        m_settings.value(UISettingsDialog::DialogType_Machine)->load();
    }

    /* Expose instance: */
    UIDesktopWidgetWatchdog::restoreWidget(m_settings.value(UISettingsDialog::DialogType_Machine));
}

void UIMachineLogic::sltCloseSettingsDialog()
{
    /* Remove instance if exist: */
    delete m_settings.take(UISettingsDialog::DialogType_Machine);

    /* We can't rely on MediumChange events as they are not yet properly implemented within Main.
     * We can't watch for MachineData change events as well as they are of broadcast type
     * and console event-handler do not processing broadcast events.
     * But we still want to be updated after possible medium changes at least if they were
     * originated from our side. */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        pMachineWindow->updateAppearanceOf(UIVisualElement_HDStuff | UIVisualElement_CDStuff | UIVisualElement_FDStuff);
}

void UIMachineLogic::sltTakeSnapshot()
{
    /* Do not process if window(s) missed! */
    if (!isMachineWindowsCreated())
        return;

    /* Create take-snapshot dialog: */
    QWidget *pDlgParent = windowManager().realParentWindow(activeMachineWindow());
    QPointer<UITakeSnapshotDialog> pDlg = new UITakeSnapshotDialog(pDlgParent, machine());
    windowManager().registerNewParent(pDlg, pDlgParent);

    /* Assign corresponding icon: */
    if (uisession() && uisession()->machineWindowIcon())
        pDlg->setIcon(*uisession()->machineWindowIcon());

    /* Search for the max available filter index: */
    const QString strNameTemplate = UITakeSnapshotDialog::tr("Snapshot %1");
    int iMaxSnapshotIndex = searchMaxSnapshotIndex(machine(), machine().FindSnapshot(QString()), strNameTemplate);
    pDlg->setName(strNameTemplate.arg(++ iMaxSnapshotIndex));

    /* Exec the dialog: */
    const bool fDialogAccepted = pDlg->exec() == QDialog::Accepted;

    /* Make sure dialog still valid: */
    if (!pDlg)
        return;

    /* Acquire variables: */
    const QString strSnapshotName = pDlg->name().trimmed();
    const QString strSnapshotDescription = pDlg->description();

    /* Destroy dialog early: */
    delete pDlg;

    /* Was the dialog accepted? */
    if (!fDialogAccepted)
        return;

    /* Take snapshot: */
    UINotificationProgressSnapshotTake *pNotification = new UINotificationProgressSnapshotTake(machine(),
                                                                                               strSnapshotName,
                                                                                               strSnapshotDescription);
    gpNotificationCenter->append(pNotification);
}

void UIMachineLogic::sltShowInformationDialog()
{
    /* Do not process if window(s) missed! */
    if (!isMachineWindowsCreated())
        return;

    if (!m_pVMInformationDialog)
        m_pVMInformationDialog = new UIVMInformationDialog(activeMachineWindow());

    if (m_pVMInformationDialog)
    {
        m_pVMInformationDialog->show();
        m_pVMInformationDialog->raise();
        m_pVMInformationDialog->setWindowState(m_pVMInformationDialog->windowState() & ~Qt::WindowMinimized);
        m_pVMInformationDialog->activateWindow();
        connect(m_pVMInformationDialog, &UIVMInformationDialog::sigClose, this, &UIMachineLogic::sltCloseInformationDialogDefault);
    }
}

void UIMachineLogic::sltCloseInformationDialog(bool fAsync /* = false */)
{
    if (!m_pVMInformationDialog)
        return;
    if (fAsync)
        m_pVMInformationDialog->deleteLater();
    else
        delete m_pVMInformationDialog;
    m_pVMInformationDialog = 0;
}

void UIMachineLogic::sltShowFileManagerDialog()
{
    if (machine().isNull() || !activeMachineWindow())
        return;

    /* Create a file manager only if we don't have one already: */
    if (m_pFileManagerDialog)
    {
        m_pFileManagerDialog->activateWindow();
        m_pFileManagerDialog->raise();
        return;
    }

    QIManagerDialog *pFileManagerDialog;
    UIFileManagerDialogFactory dialogFactory(actionPool(), machine().GetId(), machine().GetName());
    dialogFactory.prepare(pFileManagerDialog, activeMachineWindow());
    if (pFileManagerDialog)
    {
        m_pFileManagerDialog = pFileManagerDialog;

        /* Show instance: */
        pFileManagerDialog->show();
        pFileManagerDialog->setWindowState(pFileManagerDialog->windowState() & ~Qt::WindowMinimized);
        pFileManagerDialog->activateWindow();
        pFileManagerDialog->raise();
        connect(pFileManagerDialog, &QIManagerDialog::sigClose,
                this, &UIMachineLogic::sltCloseFileManagerDialog);
    }
}

void UIMachineLogic::sltCloseFileManagerDialog()
{
    if (!m_pFileManagerDialog)
        return;

    QIManagerDialog* pDialog = m_pFileManagerDialog;
    /* Set the m_pFileManagerDialog to NULL before closing the dialog. or we will have redundant deletes*/
    m_pFileManagerDialog = 0;
    pDialog->close();
    UIFileManagerDialogFactory().cleanup(pDialog);
}

void UIMachineLogic::sltShowLogDialog()
{
    if (machine().isNull() || !activeMachineWindow())
        return;

    /* Create a logviewer only if we don't have one already */
    if (m_pLogViewerDialog)
        return;

    QIManagerDialog *pLogViewerDialog;
    UIVMLogViewerDialogFactory dialogFactory(actionPool(), machine().GetId(), machine().GetName());
    dialogFactory.prepare(pLogViewerDialog, activeMachineWindow());
    if (pLogViewerDialog)
    {
        m_pLogViewerDialog = pLogViewerDialog;

        /* Show instance: */
        pLogViewerDialog->show();
        pLogViewerDialog->setWindowState(pLogViewerDialog->windowState() & ~Qt::WindowMinimized);
        pLogViewerDialog->activateWindow();
        connect(pLogViewerDialog, &QIManagerDialog::sigClose,
                this, &UIMachineLogic::sltCloseLogDialog);
    }
}

void UIMachineLogic::sltCloseLogDialog()
{
    if (!m_pLogViewerDialog)
        return;

    QIManagerDialog* pDialog = m_pLogViewerDialog;
    /* Set the m_pLogViewerDialog to NULL before closing the dialog. or we will have redundant deletes*/
    m_pLogViewerDialog = 0;
    pDialog->close();
    UIVMLogViewerDialogFactory().cleanup(pDialog);
}

void UIMachineLogic::sltPause(bool fOn)
{
    uisession()->setPause(fOn);
}

void UIMachineLogic::sltReset()
{
    reset(true);
}

void UIMachineLogic::sltDetach()
{
    /* Make sure machine is in one of the allowed states: */
    if (!uisession()->isRunning() && !uisession()->isPaused())
    {
        AssertMsgFailed(("Invalid machine-state. Action should be prohibited!"));
        return;
    }

    LogRel(("GUI: User requested to detach GUI.\n"));
    uisession()->detachUi();
}

void UIMachineLogic::sltSaveState()
{
    /* Make sure machine is in one of the allowed states: */
    if (!uisession()->isRunning() && !uisession()->isPaused())
    {
        AssertMsgFailed(("Invalid machine-state. Action should be prohibited!"));
        return;
    }

    LogRel(("GUI: User requested to save VM state.\n"));
    uisession()->saveState();
}

void UIMachineLogic::sltShutdown()
{
    /* Make sure machine is in one of the allowed states: */
    if (!uisession()->isRunning())
    {
        AssertMsgFailed(("Invalid machine-state. Action should be prohibited!"));
        return;
    }

    LogRel(("GUI: User requested to shutdown VM.\n"));
    uisession()->shutdown();
}

void UIMachineLogic::sltPowerOff()
{
    /* Make sure machine is in one of the allowed states: */
    if (!uisession()->isRunning() && !uisession()->isPaused() && !uisession()->isStuck())
    {
        AssertMsgFailed(("Invalid machine-state. Action should be prohibited!"));
        return;
    }

    LogRel(("GUI: User requested to power VM off.\n"));
    const bool fDiscardStateOnPowerOff = gEDataManager->discardStateOnPowerOff(uiCommon().managedVMUuid());
    uisession()->powerOff(machine().GetSnapshotCount() > 0 && fDiscardStateOnPowerOff);
}

void UIMachineLogic::sltMinimizeActiveMachineWindow()
{
    /* Do not process if window(s) missed! */
    if (!isMachineWindowsCreated())
        return;

    /* Minimize active machine-window: */
    AssertPtrReturnVoid(activeMachineWindow());
    activeMachineWindow()->showMinimized();
}

void UIMachineLogic::sltAdjustMachineWindows()
{
    /* Do not process if window(s) missed! */
    if (!isMachineWindowsCreated())
        return;

    /* Adjust all window(s)! */
    foreach(UIMachineWindow *pMachineWindow, machineWindows())
    {
        /* Exit maximized window state if actual: */
        if (pMachineWindow->isMaximized())
            pMachineWindow->showNormal();

        /* Normalize window geometry: */
        pMachineWindow->normalizeGeometry(true /* adjust position */, true /* resize window to guest display size */);
    }
}

void UIMachineLogic::sltToggleGuestAutoresize(bool fEnabled)
{
    /* Do not process if window(s) missed! */
    if (!isMachineWindowsCreated())
        return;

    /* Toggle guest-autoresize feature for all view(s)! */
    foreach(UIMachineWindow *pMachineWindow, machineWindows())
    {
        pMachineWindow->machineView()->setGuestAutoresizeEnabled(fEnabled);
        /* Normalize machine windows if auto resize option is toggled to true. */
        if (fEnabled)
        {
            /* Exit maximized window state if actual: */
            if (pMachineWindow->isMaximized())
                pMachineWindow->showNormal();

            /* Normalize window geometry: */
            pMachineWindow->normalizeGeometry(true /* adjust position */, true /* resize window to guest display size */);
        }
    }

    /* Save value to extra-data finally: */
    gEDataManager->setGuestScreenAutoResizeEnabled(fEnabled, uiCommon().managedVMUuid());
}

void UIMachineLogic::sltTakeScreenshot()
{
    /* Do not process if window(s) missed! */
    if (!isMachineWindowsCreated())
        return;

    /* Formatting default filename for screenshot. VM folder is the default directory to save: */
    const QFileInfo fi(machine().GetSettingsFilePath());
    const QString strCurrentTime = QDateTime::currentDateTime().toString("dd_MM_yyyy_hh_mm_ss");
    const QString strFormatDefaultFileName = QString("VirtualBox").append("_").append(machine().GetName()).append("_").append(strCurrentTime);
    const QString strDefaultFileName = QDir(fi.absolutePath()).absoluteFilePath(strFormatDefaultFileName);

    /* Formatting temporary filename for screenshot. It is saved in system temporary directory if available, else in VM folder: */
    QString strTempFile = QDir(fi.absolutePath()).absoluteFilePath("temp").append("_").append(strCurrentTime).append(".png");
    if (QDir::temp().exists())
        strTempFile = QDir::temp().absoluteFilePath("temp").append("_").append(strCurrentTime).append(".png");

    /* Do the screenshot: */
    takeScreenshot(strTempFile, "png");

    /* Which image formats for writing does this Qt version know of? */
    QList<QByteArray> formats = QImageWriter::supportedImageFormats();
    QStringList filters;
    /* Build a filters list out of it: */
    for (int i = 0; i < formats.size(); ++i)
    {
        const QString &s = formats.at(i) + " (*." + formats.at(i).toLower() + ")";
        /* Check there isn't an entry already (even if it just uses another capitalization) */
        if (filters.indexOf(QRegularExpression(QRegularExpression::escape(s), QRegularExpression::CaseInsensitiveOption)) == -1)
            filters << s;
    }
    /* Try to select some common defaults: */
    QString strFilter;
    int i = filters.indexOf(QRegularExpression(".*png.*", QRegularExpression::CaseInsensitiveOption));
    if (i == -1)
    {
        i = filters.indexOf(QRegularExpression(".*jpe+g.*", QRegularExpression::CaseInsensitiveOption));
        if (i == -1)
            i = filters.indexOf(QRegularExpression(".*bmp.*", QRegularExpression::CaseInsensitiveOption));
    }
    if (i != -1)
    {
        filters.prepend(filters.takeAt(i));
        strFilter = filters.first();
    }

#ifdef VBOX_WS_WIN
    /* Due to Qt bug, modal QFileDialog appeared above the active machine-window
     * does not retreive the focus from the currently focused machine-view,
     * as the result guest keyboard remains captured, so we should
     * clear the focus from this machine-view initially: */
    if (activeMachineWindow())
        activeMachineWindow()->machineView()->clearFocus();
#endif /* VBOX_WS_WIN */

    /* Request the filename from the user: */
    const QString strFilename = QIFileDialog::getSaveFileName(strDefaultFileName,
                                                              filters.join(";;"),
                                                              activeMachineWindow(),
                                                              tr("Select a filename for the screenshot ..."),
                                                              &strFilter,
                                                              true /* resolve symlinks */,
                                                              true /* confirm overwrite */);

#ifdef VBOX_WS_WIN
    /* Due to Qt bug, modal QFileDialog appeared above the active machine-window
     * does not retreive the focus from the currently focused machine-view,
     * as the result guest keyboard remains captured, so we already
     * cleared the focus from this machine-view and should return
     * that focus finally: */
    if (activeMachineWindow())
        activeMachineWindow()->machineView()->setFocus();
#endif /* VBOX_WS_WIN */

    if (!strFilename.isEmpty())
    {
        const QString strFormat = strFilter.split(" ").value(0, "png");
        const QImage tmpImage(strTempFile);

        /* On X11 Qt Filedialog returns the filepath without the filetype suffix, so adding it ourselves: */
#ifdef VBOX_WS_X11
        /* Add filetype suffix only if user has not added it explicitly: */
        if (!strFilename.endsWith(QString(".%1").arg(strFormat)))
            tmpImage.save(QDir::toNativeSeparators(QFile::encodeName(QString("%1.%2").arg(strFilename, strFormat))),
                          strFormat.toUtf8().constData());
        else
            tmpImage.save(QDir::toNativeSeparators(QFile::encodeName(strFilename)),
                          strFormat.toUtf8().constData());
#else /* !VBOX_WS_X11 */
        QFile file(strFilename);
        if (file.open(QIODevice::WriteOnly))
            tmpImage.save(&file, strFormat.toUtf8().constData());
#endif /* !VBOX_WS_X11 */
    }
    QFile::remove(strTempFile);
}

void UIMachineLogic::sltOpenRecordingOptions()
{
    /* Open VM settings : Display page : Recording tab: */
    sltOpenSettingsDialog("#display", "m_pCheckboxVideoCapture");
}

void UIMachineLogic::sltToggleRecording(bool fEnabled)
{
    /* Do not process if window(s) missed! */
    if (!isMachineWindowsCreated())
        return;

    /* Make sure something had changed: */
    CRecordingSettings comRecordingSettings = machine().GetRecordingSettings();
    if (comRecordingSettings.GetEnabled() == static_cast<BOOL>(fEnabled))
        return;

    /* Update recording state: */
    comRecordingSettings.SetEnabled(fEnabled);
    if (!comRecordingSettings.isOk())
    {
        /* Make sure action is updated: */
        uisession()->updateStatusRecording();
        /* Notify about the error: */
        return UINotificationMessage::cannotToggleRecording(comRecordingSettings, machine().GetName(), fEnabled);
    }

    /* Save machine-settings: */
    machine().SaveSettings();
    if (!machine().isOk())
    {
        /* Make sure action is updated: */
        uisession()->updateStatusRecording();
        /* Notify about the error: */
        return UINotificationMessage::cannotSaveMachineSettings(machine());
    }
}

void UIMachineLogic::sltToggleVRDE(bool fEnabled)
{
    /* Do not process if window(s) missed! */
    if (!isMachineWindowsCreated())
        return;

    /* Access VRDE server: */
    CVRDEServer server = machine().GetVRDEServer();
    AssertMsgReturnVoid(machine().isOk() && !server.isNull(),
                        ("VRDE server should NOT be null!\n"));

    /* Make sure something had changed: */
    if (server.GetEnabled() == static_cast<BOOL>(fEnabled))
        return;

    /* Update VRDE server state: */
    server.SetEnabled(fEnabled);
    if (!server.isOk())
    {
        /* Make sure action is updated: */
        uisession()->updateStatusVRDE();
        /* Notify about the error: */
        return UINotificationMessage::cannotToggleVRDEServer(server, machineName(), fEnabled);
    }

    /* Save machine-settings: */
    machine().SaveSettings();
    if (!machine().isOk())
    {
        /* Make sure action is updated: */
        uisession()->updateStatusVRDE();
        /* Notify about the error: */
        return UINotificationMessage::cannotSaveMachineSettings(machine());
    }
}

void UIMachineLogic::sltShowKeyboardSettings()
{
    /* Global preferences: Input page: */
    sltOpenPreferencesDialog("#input", "m_pMachineTable");
}

void UIMachineLogic::sltShowSoftKeyboard()
{
    if (machine().isNull() || !activeMachineWindow())
        return;

    if (!m_pSoftKeyboardDialog)
    {
        QWidget *pCenterWidget = windowManager().realParentWindow(activeMachineWindow());
        m_pSoftKeyboardDialog = new UISoftKeyboard(0, uisession(), pCenterWidget, machine().GetName());
        connect(m_pSoftKeyboardDialog, &UISoftKeyboard::sigClose, this, &UIMachineLogic::sltCloseSoftKeyboardDefault);
    }

    if (m_pSoftKeyboardDialog)
    {
        m_pSoftKeyboardDialog->show();
        m_pSoftKeyboardDialog->raise();
        m_pSoftKeyboardDialog->setWindowState(m_pSoftKeyboardDialog->windowState() & ~Qt::WindowMinimized);
        m_pSoftKeyboardDialog->activateWindow();
    }
}

void UIMachineLogic::sltCloseSoftKeyboard(bool fAsync /* = false */)
{
    if (!m_pSoftKeyboardDialog)
        return;
    if (fAsync)
        m_pSoftKeyboardDialog->deleteLater();
    else
        delete m_pSoftKeyboardDialog;
    m_pSoftKeyboardDialog = 0;
}

void UIMachineLogic::sltTypeCAD()
{
    keyboard().PutCAD();
    AssertWrapperOk(keyboard());
}

#ifdef VBOX_WS_X11
void UIMachineLogic::sltTypeCABS()
{
    static QVector<LONG> sequence(6);
    sequence[0] = 0x1d;        /* Ctrl down */
    sequence[1] = 0x38;        /* Alt down */
    sequence[2] = 0x0E;        /* Backspace down */
    sequence[3] = 0x0E | 0x80; /* Backspace up */
    sequence[4] = 0x38 | 0x80; /* Alt up */
    sequence[5] = 0x1d | 0x80; /* Ctrl up */
    keyboard().PutScancodes(sequence);
    AssertWrapperOk(keyboard());
}
#endif /* VBOX_WS_X11 */

void UIMachineLogic::sltTypeCtrlBreak()
{
    static QVector<LONG> sequence(6);
    sequence[0] = 0x1d;        /* Ctrl down */
    sequence[1] = 0xe0;        /* Extended flag */
    sequence[2] = 0x46;        /* Break down */
    sequence[3] = 0xe0;        /* Extended flag */
    sequence[4] = 0x46 | 0x80; /* Break up */
    sequence[5] = 0x1d | 0x80; /* Ctrl up */
    keyboard().PutScancodes(sequence);
    AssertWrapperOk(keyboard());
}

void UIMachineLogic::sltTypeInsert()
{
    static QVector<LONG> sequence(4);
    sequence[0] = 0xE0;        /* Extended flag */
    sequence[1] = 0x52;        /* Insert down */
    sequence[2] = 0xE0;        /* Extended flag */
    sequence[3] = 0x52 | 0x80; /* Insert up */
    keyboard().PutScancodes(sequence);
    AssertWrapperOk(keyboard());
}

void UIMachineLogic::sltTypePrintScreen()
{
    static QVector<LONG> sequence(8);
    sequence[0] = 0xE0;        /* Extended flag */
    sequence[1] = 0x2A;        /* Print.. down */
    sequence[2] = 0xE0;        /* Extended flag */
    sequence[3] = 0x37;        /* ..Screen down */
    sequence[4] = 0xE0;        /* Extended flag */
    sequence[5] = 0x37 | 0x80; /* ..Screen up */
    sequence[6] = 0xE0;        /* Extended flag */
    sequence[7] = 0x2A | 0x80; /* Print.. up */
    keyboard().PutScancodes(sequence);
    AssertWrapperOk(keyboard());
}

void UIMachineLogic::sltTypeAltPrintScreen()
{
    static QVector<LONG> sequence(10);
    sequence[0] = 0x38;        /* Alt down */
    sequence[1] = 0xE0;        /* Extended flag */
    sequence[2] = 0x2A;        /* Print.. down */
    sequence[3] = 0xE0;        /* Extended flag */
    sequence[4] = 0x37;        /* ..Screen down */
    sequence[5] = 0xE0;        /* Extended flag */
    sequence[6] = 0x37 | 0x80; /* ..Screen up */
    sequence[7] = 0xE0;        /* Extended flag */
    sequence[8] = 0x2A | 0x80; /* Print.. up */
    sequence[9] = 0x38 | 0x80; /* Alt up */
    keyboard().PutScancodes(sequence);
    AssertWrapperOk(keyboard());
}

void UIMachineLogic::sltTypeHostKeyComboPressRelease(bool fToggleSequence)
{
    if (keyboardHandler())
        keyboardHandler()->setHostKeyComboPressedFlag(fToggleSequence);
    QList<unsigned> shortCodes = UIHostCombo::modifiersToScanCodes(gEDataManager->hostKeyCombination());
    QVector<LONG> codes;
    foreach (unsigned idxCode, shortCodes)
    {
        /* Check if we need to include extended code for this key: */
        if (idxCode & 0x100)
            codes << 0xE0;
        if (fToggleSequence)
        {
            /* Add the press code: */
            codes << (idxCode & 0x7F);
        }
        else
        {
            /* Add the release code: */
            codes << ((idxCode & 0x7F) | 0x80);
        }
    }

    keyboard().PutScancodes(codes);
    AssertWrapperOk(keyboard());
}

void UIMachineLogic::sltToggleMouseIntegration(bool fEnabled)
{
    /* Do not process if window(s) missed! */
    if (!isMachineWindowsCreated())
        return;

    /* Disable/Enable mouse-integration for all view(s): */
    mouseHandler()->setMouseIntegrationEnabled(fEnabled);
}

void UIMachineLogic::sltOpenSettingsDialogStorage()
{
    /* Machine settings: Storage page: */
    sltOpenSettingsDialog("#storage");
}

void UIMachineLogic::sltMountStorageMedium()
{
    /* Sender action: */
    QAction *pAction = qobject_cast<QAction*>(sender());
    AssertMsgReturnVoid(pAction, ("This slot should only be called by menu action!\n"));

    /* Current mount-target: */
    const UIMediumTarget target = pAction->data().value<UIMediumTarget>();

    /* Update current machine mount-target: */
    uiCommon().updateMachineStorage(machine(), target, actionPool());
}

void UIMachineLogic::sltToggleAudioOutput(bool fEnabled)
{
    /* Do not process if window(s) missed! */
    if (!isMachineWindowsCreated())
        return;

    /* Access audio adapter: */
    CAudioSettings const comAudioSettings = machine().GetAudioSettings();
    CAudioAdapter        comAdapter       = comAudioSettings.GetAdapter();
    AssertMsgReturnVoid(machine().isOk() && comAdapter.isNotNull(),
                        ("Audio adapter should NOT be null!\n"));

    /* Make sure something had changed: */
    if (comAdapter.GetEnabledOut() == static_cast<BOOL>(fEnabled))
        return;

    /* Update audio output state: */
    comAdapter.SetEnabledOut(fEnabled);
    if (!comAdapter.isOk())
    {
        /* Make sure action is updated: */
        uisession()->updateAudioOutput();
        /* Notify about the error: */
        return UINotificationMessage::cannotToggleAudioOutput(comAdapter, machineName(), fEnabled);
    }

    /* Save machine-settings: */
    machine().SaveSettings();
    if (!machine().isOk())
    {
        /* Make sure action is updated: */
        uisession()->updateAudioOutput();
        /* Notify about the error: */
        return UINotificationMessage::cannotSaveMachineSettings(machine());
    }
}

void UIMachineLogic::sltToggleAudioInput(bool fEnabled)
{
    /* Do not process if window(s) missed! */
    if (!isMachineWindowsCreated())
        return;

    /* Access audio adapter: */
    CAudioSettings const comAudioSettings = machine().GetAudioSettings();
    CAudioAdapter        comAdapter       = comAudioSettings.GetAdapter();
    AssertMsgReturnVoid(machine().isOk() && comAdapter.isNotNull(),
                        ("Audio adapter should NOT be null!\n"));

    /* Make sure something had changed: */
    if (comAdapter.GetEnabledIn() == static_cast<BOOL>(fEnabled))
        return;

    /* Update audio input state: */
    comAdapter.SetEnabledIn(fEnabled);
    if (!comAdapter.isOk())
    {
        /* Make sure action is updated: */
        uisession()->updateAudioInput();
        /* Notify about the error: */
        return UINotificationMessage::cannotToggleAudioInput(comAdapter, machineName(), fEnabled);
    }

    /* Save machine-settings: */
    machine().SaveSettings();
    if (!machine().isOk())
    {
        /* Make sure action is updated: */
        uisession()->updateAudioInput();
        /* Notify about the error: */
        return UINotificationMessage::cannotSaveMachineSettings(machine());
    }
}

void UIMachineLogic::sltOpenSettingsDialogNetwork()
{
    /* Open VM settings : Network page: */
    sltOpenSettingsDialog("#network");
}

void UIMachineLogic::sltOpenSettingsDialogUSBDevices()
{
    /* Machine settings: Storage page: */
    sltOpenSettingsDialog("#usb");
}

void UIMachineLogic::sltOpenSettingsDialogSharedFolders()
{
    /* Do not process if additions are not loaded! */
    if (!uisession()->isGuestAdditionsActive())
        UINotificationMessage::remindAboutGuestAdditionsAreNotActive();

    /* Open VM settings : Shared folders page: */
    sltOpenSettingsDialog("#sharedFolders");
}

void UIMachineLogic::sltAttachUSBDevice()
{
    /* Get and check sender action object: */
    QAction *pAction = qobject_cast<QAction*>(sender());
    AssertMsg(pAction, ("This slot should only be called on selecting USB menu item!\n"));

    /* Get operation target: */
    USBTarget target = pAction->data().value<USBTarget>();

    /* Attach USB device: */
    if (target.attach)
    {
        /* Try to attach corresponding device: */
        console().AttachUSBDevice(target.id, QString(""));
        /* Check if console is OK: */
        if (!console().isOk())
        {
            /* Get current host: */
            CHost host = uiCommon().host();
            /* Search the host for the corresponding USB device: */
            CHostUSBDevice hostDevice = host.FindUSBDeviceById(target.id);
            /* Get USB device from host USB device: */
            CUSBDevice device(hostDevice);
            /* Show a message about procedure failure: */
            UINotificationMessage::cannotAttachUSBDevice(console(), uiCommon().usbDetails(device));
        }
    }
    /* Detach USB device: */
    else
    {
        /* Search the console for the corresponding USB device: */
        CUSBDevice device = console().FindUSBDeviceById(target.id);
        /* Try to detach corresponding device: */
        console().DetachUSBDevice(target.id);
        /* Check if console is OK: */
        if (!console().isOk())
        {
            /* Show a message about procedure failure: */
            UINotificationMessage::cannotDetachUSBDevice(console(), uiCommon().usbDetails(device));
        }
    }
}

void UIMachineLogic::sltAttachWebCamDevice()
{
    /* Get and check sender action object: */
    QAction *pAction = qobject_cast<QAction*>(sender());
    AssertReturnVoid(pAction);

    /* Get operation target: */
    WebCamTarget target = pAction->data().value<WebCamTarget>();

    /* Get current emulated USB: */
    CEmulatedUSB dispatcher = console().GetEmulatedUSB();

    /* Attach webcam device: */
    if (target.attach)
    {
        /* Try to attach corresponding device: */
        dispatcher.WebcamAttach(target.path, "");
        /* Check if dispatcher is OK: */
        if (!dispatcher.isOk())
            UINotificationMessage::cannotAttachWebCam(dispatcher, target.name, machineName());
    }
    /* Detach webcam device: */
    else
    {
        /* Try to detach corresponding device: */
        dispatcher.WebcamDetach(target.path);
        /* Check if dispatcher is OK: */
        if (!dispatcher.isOk())
            UINotificationMessage::cannotDetachWebCam(dispatcher, target.name, machineName());
    }
}

void UIMachineLogic::sltChangeSharedClipboardType(QAction *pAction)
{
    /* Assign new mode (without save): */
    KClipboardMode enmMode = pAction->data().value<KClipboardMode>();
    machine().SetClipboardMode(enmMode);
}

void UIMachineLogic::sltToggleNetworkAdapterConnection()
{
    /* Do not process if window(s) missed! */
    if (!isMachineWindowsCreated())
        return;

    /* Get and check 'the sender' action object: */
    QAction *pAction = qobject_cast<QAction*>(sender());
    AssertMsgReturnVoid(pAction, ("Sender action should NOT be null!\n"));

    /* Get operation target: */
    CNetworkAdapter adapter = machine().GetNetworkAdapter((ULONG)pAction->property("slot").toInt());
    AssertMsgReturnVoid(machine().isOk() && !adapter.isNull(),
                        ("Network adapter should NOT be null!\n"));

    /* Connect/disconnect cable to/from target: */
    const bool fConnect = !adapter.GetCableConnected();
    adapter.SetCableConnected(fConnect);
    if (!adapter.isOk())
        return UINotificationMessage::cannotToggleNetworkCable(adapter, machineName(), fConnect);

    /* Save machine-settings: */
    machine().SaveSettings();
    if (!machine().isOk())
        return UINotificationMessage::cannotSaveMachineSettings(machine());
}

void UIMachineLogic::sltChangeDragAndDropType(QAction *pAction)
{
    /* Assign new mode (without save): */
    KDnDMode enmMode = pAction->data().value<KDnDMode>();
    machine().SetDnDMode(enmMode);
}

void UIMachineLogic::sltInstallGuestAdditions()
{
    /* Do not process if window(s) missed! */
    if (!isMachineWindowsCreated())
        return;

    bool fOnlyMount = sender() == actionPool()->action(UIActionIndexRT_M_Devices_S_InsertGuestAdditionsDisk);

    /* Try to acquire default additions ISO: */
    CSystemProperties comSystemProperties = uiCommon().virtualBox().GetSystemProperties();
    const QString strAdditions = comSystemProperties.GetDefaultAdditionsISO();
    if (comSystemProperties.isOk() && !strAdditions.isEmpty())
    {
        if (fOnlyMount)
            return uisession()->sltMountDVDAdHoc(strAdditions);
        else
            return uisession()->sltInstallGuestAdditionsFrom(strAdditions);
    }

    /* Check whether we have already registered image: */
    CVirtualBox comVBox = uiCommon().virtualBox();
    CMediumVector comMedia = comVBox.GetDVDImages();
    if (!comVBox.isOk())
        UINotificationMessage::cannotAcquireVirtualBoxParameter(comVBox);
    else
    {
        const QString strName = QString("%1_%2.iso").arg(GUI_GuestAdditionsName, uiCommon().vboxVersionStringNormalized());
        foreach (const CMedium &comMedium, comMedia)
        {
            /* Compare the name part ignoring the file case: */
            const QString strPath = comMedium.GetLocation();
            if (!comMedium.isOk())
                UINotificationMessage::cannotAcquireMediumParameter(comMedium);
            {
                const QString strFileName = QFileInfo(strPath).fileName();
                if (RTPathCompare(strName.toUtf8().constData(), strFileName.toUtf8().constData()) == 0)
                {
                    if (fOnlyMount)
                        return uisession()->sltMountDVDAdHoc(strPath);
                    else
                        return uisession()->sltInstallGuestAdditionsFrom(strPath);
                }
            }
        }
    }

#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    /* If downloader is running already: */
    if (UINotificationDownloaderGuestAdditions::exists())
        gpNotificationCenter->invoke();
    /* Else propose to download additions: */
    else if (msgCenter().confirmLookingForGuestAdditions())
    {
        /* Download guest additions: */
        UINotificationDownloaderGuestAdditions *pNotification = UINotificationDownloaderGuestAdditions::instance(GUI_GuestAdditionsName);
        /* After downloading finished => propose to install or just mount the guest additions: */
        if (fOnlyMount)
            connect(pNotification, &UINotificationDownloaderGuestAdditions::sigGuestAdditionsDownloaded,
                    uisession(), &UISession::sltMountDVDAdHoc);
        else
            connect(pNotification, &UINotificationDownloaderGuestAdditions::sigGuestAdditionsDownloaded,
                    uisession(), &UISession::sltInstallGuestAdditionsFrom);
        /* Append and start notification: */
        gpNotificationCenter->append(pNotification);
    }
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
}

#ifdef VBOX_WITH_DEBUGGER_GUI

void UIMachineLogic::sltShowDebugStatistics()
{
    if (dbgCreated())
    {
        keyboardHandler()->setDebuggerActive();
        const QByteArray &expandBytes = uiCommon().getDebuggerStatisticsExpand().toUtf8();
        const QByteArray &filterBytes = uiCommon().getDebuggerStatisticsFilter().toUtf8();
        m_pDbgGuiVT->pfnShowStatistics(m_pDbgGui, filterBytes.constData(), expandBytes.constData());
    }
}

void UIMachineLogic::sltShowDebugCommandLine()
{
    if (dbgCreated())
    {
        keyboardHandler()->setDebuggerActive();
        m_pDbgGuiVT->pfnShowCommandLine(m_pDbgGui);
    }
}

void UIMachineLogic::sltLoggingToggled(bool fState)
{
    NOREF(fState);
    if (!debugger().isNull() && debugger().isOk())
        debugger().SetLogEnabled(fState);
}

void UIMachineLogic::sltShowGuestControlConsoleDialog()
{
    if (machine().isNull() || !activeMachineWindow())
        return;

    /* Create the dialog only if we don't have one already */
    if (m_pProcessControlDialog)
        return;

    QIManagerDialog *pProcessControlDialog;
    UIGuestProcessControlDialogFactory dialogFactory(actionPool(), console().GetGuest(), machine().GetName());
    dialogFactory.prepare(pProcessControlDialog, activeMachineWindow());
    if (pProcessControlDialog)
    {
        m_pProcessControlDialog = pProcessControlDialog;

        /* Show instance: */
        pProcessControlDialog->show();
        pProcessControlDialog->setWindowState(pProcessControlDialog->windowState() & ~Qt::WindowMinimized);
        pProcessControlDialog->activateWindow();
        connect(pProcessControlDialog, &QIManagerDialog::sigClose,
                this, &UIMachineLogic::sltCloseGuestControlConsoleDialog);
    }
}

void UIMachineLogic::sltCloseGuestControlConsoleDialog()
{
    if (!m_pProcessControlDialog)
        return;

    QIManagerDialog* pDialog = m_pProcessControlDialog;
    /* Set the m_pLogViewerDialog to NULL before closing the dialog. or we will have redundant deletes*/
    m_pProcessControlDialog = 0;
    pDialog->close();
    UIGuestProcessControlDialogFactory().cleanup(pDialog);
}
#endif /* VBOX_WITH_DEBUGGER_GUI */

#ifdef VBOX_WS_MAC
void UIMachineLogic::sltSwitchToMachineWindow()
{
    /* Acquire appropriate sender action: */
    const QAction *pSender = qobject_cast<QAction*>(sender());
    AssertReturnVoid(pSender);
    {
        /* Determine sender action index: */
        const int iIndex = pSender->data().toInt();
        AssertReturnVoid(iIndex >= 0 && iIndex < machineWindows().size());
        {
            /* Raise appropriate machine-window: */
            UIMachineWindow *pMachineWindow = machineWindows().at(iIndex);
            AssertPtrReturnVoid(pMachineWindow);
            {
                pMachineWindow->show();
                pMachineWindow->raise();
                pMachineWindow->activateWindow();
            }
        }
    }
}

void UIMachineLogic::sltDockPreviewModeChanged(QAction *pAction)
{
    bool fEnabled = pAction != actionPool()->action(UIActionIndexRT_M_Dock_M_DockSettings_T_DisableMonitor);
    gEDataManager->setRealtimeDockIconUpdateEnabled(fEnabled, uiCommon().managedVMUuid());
    updateDockOverlay();
}

void UIMachineLogic::sltDockPreviewMonitorChanged(QAction *pAction)
{
    gEDataManager->setRealtimeDockIconUpdateMonitor(pAction->data().toInt(), uiCommon().managedVMUuid());
    updateDockOverlay();
}

void UIMachineLogic::sltChangeDockIconUpdate(bool fEnabled)
{
    if (isMachineWindowsCreated())
    {
        setDockIconPreviewEnabled(fEnabled);
        if (m_pDockPreviewSelectMonitorGroup)
        {
            m_pDockPreviewSelectMonitorGroup->setEnabled(fEnabled);
            m_DockIconPreviewMonitor = qMin(gEDataManager->realtimeDockIconUpdateMonitor(uiCommon().managedVMUuid()),
                                            (int)machine().GetGraphicsAdapter().GetMonitorCount() - 1);
        }
        /* Resize the dock icon in the case the preview monitor has changed. */
        QSize size = machineWindows().at(m_DockIconPreviewMonitor)->machineView()->size();
        updateDockIconSize(m_DockIconPreviewMonitor, size.width(), size.height());
        updateDockOverlay();
    }
}

void UIMachineLogic::sltChangeDockIconOverlayAppearance(bool fDisabled)
{
    /* Update dock icon overlay: */
    if (isMachineWindowsCreated())
        updateDockOverlay();
    /* Make sure to update dock icon disable overlay action state when 'GUI_DockIconDisableOverlay' changed from extra-data manager: */
    QAction *pDockIconDisableOverlay = actionPool()->action(UIActionIndexRT_M_Dock_M_DockSettings_T_DisableOverlay);
    if (fDisabled != pDockIconDisableOverlay->isChecked())
    {
        /* Block signals initially to avoid recursive loop: */
        pDockIconDisableOverlay->blockSignals(true);
        /* Update state: */
        pDockIconDisableOverlay->setChecked(fDisabled);
        /* Make sure to unblock signals again: */
        pDockIconDisableOverlay->blockSignals(false);
    }
}

void UIMachineLogic::sltDockIconDisableOverlayChanged(bool fDisabled)
{
    /* Write dock icon disable overlay flag to extra-data: */
    gEDataManager->setDockIconDisableOverlay(fDisabled, uiCommon().managedVMUuid());
}
#endif /* VBOX_WS_MAC */

void UIMachineLogic::sltSwitchKeyboardLedsToGuestLeds()
{
    /* Due to async nature of that feature
     * it can happen that this slot is called when machine-window is
     * minimized or not active anymore, we should ignore those cases. */
    QWidget *pActiveWindow = QApplication::activeWindow();
    if (   !pActiveWindow                                 // no window is active anymore
        || !qobject_cast<UIMachineWindow*>(pActiveWindow) // window is not machine one
        || pActiveWindow->isMinimized())                  // window is minimized
    {
        LogRel2(("GUI: HID LEDs Sync: skipping sync because active window is lost or minimized!\n"));
        return;
    }

//    /* Log statement (printf): */
//    QString strDt = QDateTime::currentDateTime().toString("HH:mm:ss:zzz");
//    printf("%s: UIMachineLogic: sltSwitchKeyboardLedsToGuestLeds called, machine name is {%s}\n",
//           strDt.toUtf8().constData(),
//           machineName().toUtf8().constData());

    /* Here we have to store host LED lock states. */

    /* Here we have to update host LED lock states using values provided by UISession registry.
     * [bool] uisession() -> isNumLock(), isCapsLock(), isScrollLock() can be used for that. */

    if (!isHidLedsSyncEnabled())
        return;

#if defined(VBOX_WS_MAC)
    if (m_pHostLedsState == NULL)
        m_pHostLedsState = DarwinHidDevicesKeepLedsState();
    if (m_pHostLedsState != NULL)
        DarwinHidDevicesBroadcastLeds(m_pHostLedsState, uisession()->isNumLock(), uisession()->isCapsLock(), uisession()->isScrollLock());
#elif defined(VBOX_WS_WIN)
    if (m_pHostLedsState == NULL)
        m_pHostLedsState = WinHidDevicesKeepLedsState();
    keyboardHandler()->winSkipKeyboardEvents(true);
    WinHidDevicesBroadcastLeds(uisession()->isNumLock(), uisession()->isCapsLock(), uisession()->isScrollLock());
    keyboardHandler()->winSkipKeyboardEvents(false);
#else
    LogRelFlow(("UIMachineLogic::sltSwitchKeyboardLedsToGuestLeds: keep host LED lock states and broadcast guest's ones does not supported on this platform\n"));
#endif
}

void UIMachineLogic::sltSwitchKeyboardLedsToPreviousLeds()
{
//    /* Log statement (printf): */
//    QString strDt = QDateTime::currentDateTime().toString("HH:mm:ss:zzz");
//    printf("%s: UIMachineLogic: sltSwitchKeyboardLedsToPreviousLeds called, machine name is {%s}\n",
//           strDt.toUtf8().constData(),
//           machineName().toUtf8().constData());

    if (!isHidLedsSyncEnabled())
        return;

    /* Here we have to restore host LED lock states. */
    void *pvLedState = m_pHostLedsState;
    if (pvLedState)
    {
        /* bird: I've observed recursive calls here when setting m_pHostLedsState to NULL after calling
                 WinHidDevicesApplyAndReleaseLedsState.  The result is a double free(), which the CRT
                 usually detects and I could see this->m_pHostLedsState == NULL.  The windows function
                 does dispatch loop fun, that's probably the reason for it.  Hopefully not an issue on OS X. */
        m_pHostLedsState = NULL;
#if defined(VBOX_WS_MAC)
        DarwinHidDevicesApplyAndReleaseLedsState(pvLedState);
#elif defined(VBOX_WS_WIN)
        keyboardHandler()->winSkipKeyboardEvents(true);
        WinHidDevicesApplyAndReleaseLedsState(pvLedState);
        keyboardHandler()->winSkipKeyboardEvents(false);
#else
        LogRelFlow(("UIMachineLogic::sltSwitchKeyboardLedsToPreviousLeds: restore host LED lock states does not supported on this platform\n"));
#endif
    }
}

void UIMachineLogic::sltHandleVisualStateChange()
{
    /* Check for new requested value stored in extra-data: */
    const UIVisualStateType enmRequestedState = gEDataManager->requestedVisualState(uiCommon().managedVMUuid());
    /* Check whether current value OR old requested value differs from new requested one.
     * That way we will NOT enter seamless mode instantly if it is already planned
     * but is not entered because we're waiting for a guest addition permission. */
    if (   visualStateType() != enmRequestedState
        && uisession()->requestedVisualState() != enmRequestedState)
    {
        switch (enmRequestedState)
        {
            case UIVisualStateType_Normal: return sltChangeVisualStateToNormal();
            case UIVisualStateType_Fullscreen: return sltChangeVisualStateToFullscreen();
            case UIVisualStateType_Seamless: return sltChangeVisualStateToSeamless();
            case UIVisualStateType_Scale: return sltChangeVisualStateToScale();
            default: break;
        }
    }
}

void UIMachineLogic::sltHandleCommitData()
{
#ifdef VBOX_WITH_DEBUGGER_GUI
    /* Cleanup debugger before VBoxDbg module handle cleaned up: */
    cleanupDebugger();
    sltCloseLogDialog();
    sltCloseGuestControlConsoleDialog();
#endif
    activateScreenSaver();
    sltCloseFileManagerDialog();
    sltCloseInformationDialog();
    sltCloseSoftKeyboard();
    sltSwitchKeyboardLedsToPreviousLeds();
    sltCloseSettingsDialog();
    sltClosePreferencesDialog();
}

void UIMachineLogic::typeHostKeyComboPressRelease(bool fToggleSequence)
{
    QAction *pHostKeyAction = actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard_T_TypeHostKeyCombo);
    if (!pHostKeyAction)
        return;
    /* Do nothing if we try to insert host key combo press (release) and it is already in pressed (released) state: */
    if (fToggleSequence == pHostKeyAction->isChecked())
        return;
    pHostKeyAction->toggle();
}

#ifdef VBOX_WITH_DEBUGGER_GUI
void UIMachineLogic::dbgAdjustRelativePos()
{
    if (m_pDbgGui)
    {
        const QRect rct = activeMachineWindow()->frameGeometry();
        m_pDbgGuiVT->pfnAdjustRelativePos(m_pDbgGui, rct.x(), rct.y(), rct.width(), rct.height());
    }
}
#endif /* VBOX_WITH_DEBUGGER_GUI */

void UIMachineLogic::updateMenuDevicesStorage(QMenu *pMenu)
{
    /* Clear contents: */
    pMenu->clear();

    /* Determine device-type: */
    const QMenu *pOpticalDevicesMenu = actionPool()->action(UIActionIndexRT_M_Devices_M_OpticalDevices)->menu();
    const QMenu *pFloppyDevicesMenu = actionPool()->action(UIActionIndexRT_M_Devices_M_FloppyDevices)->menu();
    const KDeviceType deviceType = pMenu == pOpticalDevicesMenu ? KDeviceType_DVD :
                                   pMenu == pFloppyDevicesMenu  ? KDeviceType_Floppy :
                                                                  KDeviceType_Null;
    AssertMsgReturnVoid(deviceType != KDeviceType_Null, ("Incorrect storage device-type!\n"));

    /* Prepare/fill all storage menus: */
    foreach (const CMediumAttachment &attachment, machine().GetMediumAttachments())
    {
        /* Current controller: */
        const CStorageController controller = machine().GetStorageControllerByName(attachment.GetController());
        /* If controller present and device-type correct: */
        if (!controller.isNull() && attachment.GetType() == deviceType)
        {
            /* Current controller/attachment attributes: */
            const QString strControllerName = controller.GetName();
            const StorageSlot storageSlot(controller.GetBus(), attachment.GetPort(), attachment.GetDevice());

            /* Prepare current storage menu: */
            QMenu *pStorageMenu = 0;
            /* If it will be more than one storage menu: */
            if (pMenu->menuAction()->data().toInt() > 1)
            {
                /* We have to create sub-menu for each of them: */
                pStorageMenu = new QMenu(QString("%1 (%2)").arg(strControllerName).arg(gpConverter->toString(storageSlot)), pMenu);
                switch (controller.GetBus())
                {
                    case KStorageBus_IDE:        pStorageMenu->setIcon(QIcon(":/ide_16px.png")); break;
                    case KStorageBus_SATA:       pStorageMenu->setIcon(QIcon(":/sata_16px.png")); break;
                    case KStorageBus_SCSI:       pStorageMenu->setIcon(QIcon(":/scsi_16px.png")); break;
                    case KStorageBus_Floppy:     pStorageMenu->setIcon(QIcon(":/floppy_16px.png")); break;
                    case KStorageBus_SAS:        pStorageMenu->setIcon(QIcon(":/sas_16px.png")); break;
                    case KStorageBus_USB:        pStorageMenu->setIcon(QIcon(":/usb_16px.png")); break;
                    case KStorageBus_PCIe:       pStorageMenu->setIcon(QIcon(":/pcie_16px.png")); break;
                    case KStorageBus_VirtioSCSI: pStorageMenu->setIcon(QIcon(":/virtio_scsi_16px.png")); break;
                    default: break;
                }
                pMenu->addMenu(pStorageMenu);
            }
            /* Otherwise just use existing one: */
            else pStorageMenu = pMenu;

            /* Fill current storage menu: */
            uiCommon().prepareStorageMenu(*pStorageMenu,
                                            this, SLOT(sltMountStorageMedium()),
                                            machine(), strControllerName, storageSlot);
        }
    }
}

void UIMachineLogic::updateMenuDevicesNetwork(QMenu *pMenu)
{
    /* Determine how many adapters we should display: */
    const KChipsetType chipsetType = machine().GetChipsetType();
    const ULONG uCount = qMin((ULONG)4, uiCommon().virtualBox().GetSystemProperties().GetMaxNetworkAdapters(chipsetType));

    /* Enumerate existing network adapters: */
    QMap<int, bool> adapterData;
    for (ULONG uSlot = 0; uSlot < uCount; ++uSlot)
    {
        /* Get and check iterated adapter: */
        const CNetworkAdapter adapter = machine().GetNetworkAdapter(uSlot);
        AssertReturnVoid(machine().isOk() && !adapter.isNull());

        /* Skip disabled adapters: */
        if (!adapter.GetEnabled())
            continue;

        /* Remember adapter data: */
        adapterData.insert((int)uSlot, (bool)adapter.GetCableConnected());
    }

    /* Make sure at least one adapter was enabled: */
    if (adapterData.isEmpty())
        return;

    /* Add new actions: */
    foreach (int iSlot, adapterData.keys())
    {
        QAction *pAction = pMenu->addAction(UIIconPool::iconSetOnOff(":/connect_on_16px.png", ":/connect_16px.png"),
                                            adapterData.size() == 1 ? UIActionPool::tr("&Connect Network Adapter") :
                                                                      UIActionPool::tr("Connect Network Adapter &%1").arg(iSlot + 1),
                                            this, SLOT(sltToggleNetworkAdapterConnection()));
        pAction->setProperty("slot", iSlot);
        pAction->setCheckable(true);
        pAction->setChecked(adapterData[iSlot]);
    }
}

void UIMachineLogic::updateMenuDevicesUSB(QMenu *pMenu)
{
    /* Get current host: */
    const CHost host = uiCommon().host();
    /* Get host USB device list: */
    const CHostUSBDeviceVector devices = host.GetUSBDevices();

    /* If device list is empty: */
    if (devices.isEmpty())
    {
        /* Add only one - "empty" action: */
        QAction *pEmptyMenuAction = pMenu->addAction(UIIconPool::iconSet(":/usb_unavailable_16px.png",
                                                                         ":/usb_unavailable_disabled_16px.png"),
                                                     UIActionPool::tr("No USB Devices Connected"));
        pEmptyMenuAction->setToolTip(UIActionPool::tr("No supported devices connected to the host PC"));
        pEmptyMenuAction->setEnabled(false);
    }
    /* If device list is NOT empty: */
    else
    {
        /* Populate menu with host USB devices: */
        foreach (const CHostUSBDevice& hostDevice, devices)
        {
            /* Get USB device from current host USB device: */
            const CUSBDevice device(hostDevice);

            /* Create USB device action: */
            QAction *pAttachUSBAction = pMenu->addAction(uiCommon().usbDetails(device),
                                                         this, SLOT(sltAttachUSBDevice()));
            pAttachUSBAction->setToolTip(uiCommon().usbToolTip(device));
            pAttachUSBAction->setCheckable(true);

            /* Check if that USB device was already attached to this session: */
            const CUSBDevice attachedDevice = console().FindUSBDeviceById(device.GetId());
            pAttachUSBAction->setChecked(!attachedDevice.isNull());
            pAttachUSBAction->setEnabled(hostDevice.GetState() != KUSBDeviceState_Unavailable);

            /* Set USB attach data: */
            pAttachUSBAction->setData(QVariant::fromValue(USBTarget(!pAttachUSBAction->isChecked(), device.GetId())));
        }
    }
}

void UIMachineLogic::updateMenuDevicesWebCams(QMenu *pMenu)
{
    /* Clear contents: */
    pMenu->clear();

    /* Get current host: */
    const CHost host = uiCommon().host();
    /* Get host webcam list: */
    const CHostVideoInputDeviceVector webcams = host.GetVideoInputDevices();

    /* If webcam list is empty: */
    if (webcams.isEmpty())
    {
        /* Add only one - "empty" action: */
        QAction *pEmptyMenuAction = pMenu->addAction(UIIconPool::iconSet(":/web_camera_unavailable_16px.png",
                                                                         ":/web_camera_unavailable_disabled_16px.png"),
                                                     UIActionPool::tr("No Webcams Connected"));
        pEmptyMenuAction->setToolTip(UIActionPool::tr("No supported webcams connected to the host PC"));
        pEmptyMenuAction->setEnabled(false);
    }
    /* If webcam list is NOT empty: */
    else
    {
        /* Populate menu with host webcams: */
        const QVector<QString> attachedWebcamPaths = console().GetEmulatedUSB().GetWebcams();
        foreach (const CHostVideoInputDevice &webcam, webcams)
        {
            /* Get webcam data: */
            const QString strWebcamName = webcam.GetName();
            const QString strWebcamPath = webcam.GetPath();

            /* Create/configure webcam action: */
            QAction *pAttachWebcamAction = pMenu->addAction(strWebcamName,
                                                            this, SLOT(sltAttachWebCamDevice()));
            pAttachWebcamAction->setToolTip(uiCommon().usbToolTip(webcam));
            pAttachWebcamAction->setCheckable(true);

            /* Check if that webcam was already attached to this session: */
            pAttachWebcamAction->setChecked(attachedWebcamPaths.contains(strWebcamPath));

            /* Set USB attach data: */
            pAttachWebcamAction->setData(QVariant::fromValue(WebCamTarget(!pAttachWebcamAction->isChecked(), strWebcamName, strWebcamPath)));
        }
    }
}

void UIMachineLogic::updateMenuDevicesSharedClipboard(QMenu *pMenu)
{
    /* Acquire current clipboard mode: */
    const KClipboardMode enmCurrentMode = machine().GetClipboardMode();

    /* First run: */
    if (!m_pSharedClipboardActions)
    {
        /* Prepare action-group: */
        m_pSharedClipboardActions = new QActionGroup(this);
        /* Load currently supported Clipboard modes: */
        CSystemProperties comProperties = uiCommon().virtualBox().GetSystemProperties();
        QVector<KClipboardMode> clipboardModes = comProperties.GetSupportedClipboardModes();
        /* Take current clipboard mode into account: */
        if (!clipboardModes.contains(enmCurrentMode))
            clipboardModes.prepend(enmCurrentMode);
        /* Create action for all clipboard modes: */
        foreach (const KClipboardMode &enmMode, clipboardModes)
        {
            QAction *pAction = new QAction(gpConverter->toString(enmMode), m_pSharedClipboardActions);
            pMenu->addAction(pAction);
            pAction->setData(QVariant::fromValue(enmMode));
            pAction->setCheckable(true);
            pAction->setChecked(enmMode == enmCurrentMode);
        }
        /* Connect action-group trigger: */
        connect(m_pSharedClipboardActions, &QActionGroup::triggered, this, &UIMachineLogic::sltChangeSharedClipboardType);
    }
    /* Subsequent runs: */
    else
        foreach (QAction *pAction, m_pSharedClipboardActions->actions())
            if (pAction->data().value<KClipboardMode>() == enmCurrentMode)
                pAction->setChecked(true);
}

void UIMachineLogic::updateMenuDevicesDragAndDrop(QMenu *pMenu)
{
    /* Acquire current DnD mode: */
    const KDnDMode enmCurrentMode = machine().GetDnDMode();

    /* First run: */
    if (!m_pDragAndDropActions)
    {
        /* Prepare action-group: */
        m_pDragAndDropActions = new QActionGroup(this);
        /* Load currently supported DnD modes: */
        CSystemProperties comProperties = uiCommon().virtualBox().GetSystemProperties();
        QVector<KDnDMode> dndModes = comProperties.GetSupportedDnDModes();
        /* Take current DnD mode into account: */
        if (!dndModes.contains(enmCurrentMode))
            dndModes.prepend(enmCurrentMode);
        /* Create action for all clipboard modes: */
        foreach (const KDnDMode &enmMode, dndModes)
        {
            QAction *pAction = new QAction(gpConverter->toString(enmMode), m_pDragAndDropActions);
            pMenu->addAction(pAction);
            pAction->setData(QVariant::fromValue(enmMode));
            pAction->setCheckable(true);
            pAction->setChecked(enmMode == enmCurrentMode);
        }
        /* Connect action-group trigger: */
        connect(m_pDragAndDropActions, &QActionGroup::triggered, this, &UIMachineLogic::sltChangeDragAndDropType);
    }
    /* Subsequent runs: */
    else
        foreach (QAction *pAction, m_pDragAndDropActions->actions())
            if (pAction->data().value<KDnDMode>() == enmCurrentMode)
                pAction->setChecked(true);
}

#ifdef VBOX_WITH_DEBUGGER_GUI
void UIMachineLogic::updateMenuDebug(QMenu*)
{
    /* The "Logging" item. */
    bool fEnabled = false;
    bool fChecked = false;
    if (!debugger().isNull() && debugger().isOk())
    {
        fEnabled = true;
        fChecked = debugger().GetLogEnabled() != FALSE;
    }
    if (fEnabled != actionPool()->action(UIActionIndexRT_M_Debug_T_Logging)->isEnabled())
        actionPool()->action(UIActionIndexRT_M_Debug_T_Logging)->setEnabled(fEnabled);
    if (fChecked != actionPool()->action(UIActionIndexRT_M_Debug_T_Logging)->isChecked())
        actionPool()->action(UIActionIndexRT_M_Debug_T_Logging)->setChecked(fChecked);
}
#endif /* VBOX_WITH_DEBUGGER_GUI */

#ifdef VBOX_WS_MAC
void UIMachineLogic::updateMenuWindow(QMenu *pMenu)
{
    /* Make sure 'Switch' action(s) are allowed: */
    AssertPtrReturnVoid(actionPool());
    if (actionPool()->isAllowedInMenuWindow(UIExtraDataMetaDefs::MenuWindowActionType_Switch))
    {
        /* Append menu with actions to switch to machine-window(s): */
        foreach (UIMachineWindow *pMachineWindow, machineWindows())
        {
            /* Create machine-window action: */
            AssertPtrReturnVoid(pMachineWindow);
            QAction *pMachineWindowAction = pMenu->addAction(pMachineWindow->windowTitle(),
                                                             this, SLOT(sltSwitchToMachineWindow()));
            AssertPtrReturnVoid(pMachineWindowAction);
            {
                pMachineWindowAction->setCheckable(true);
                pMachineWindowAction->setChecked(activeMachineWindow() == pMachineWindow);
                pMachineWindowAction->setData((int)pMachineWindow->screenId());
            }
        }
    }
}
#endif /* VBOX_WS_MAC */

void UIMachineLogic::askUserForTheDiskEncryptionPasswords()
{
    /* Prepare the map of the encrypted media: */
    EncryptedMediumMap encryptedMedia;
    foreach (const CMediumAttachment &attachment, machine().GetMediumAttachments())
    {
        /* Acquire hard-drive attachments only: */
        if (attachment.GetType() == KDeviceType_HardDisk)
        {
            /* Get the attachment medium base: */
            const CMedium medium = attachment.GetMedium();
            /* Update the map with this medium if it's encrypted: */
            QString strCipher;
            const QString strPasswordId = medium.GetEncryptionSettings(strCipher);
            if (medium.isOk())
                encryptedMedia.insert(strPasswordId, medium.GetId());
        }
    }

    /* Ask for the disk encryption passwords if necessary: */
    EncryptionPasswordMap encryptionPasswords;
    if (!encryptedMedia.isEmpty())
    {
        /* Create the dialog for acquiring encryption passwords: */
        QWidget *pDlgParent = windowManager().realParentWindow(activeMachineWindow());
        QPointer<UIAddDiskEncryptionPasswordDialog> pDlg =
             new UIAddDiskEncryptionPasswordDialog(pDlgParent,
                                                   machineName(),
                                                   encryptedMedia);
        /* Execute the dialog: */
        if (pDlg->exec() == QDialog::Accepted)
        {
            /* Acquire the passwords provided: */
            encryptionPasswords = pDlg->encryptionPasswords();

            /* Delete the dialog: */
            delete pDlg;

            /* Make sure the passwords were really provided: */
            AssertReturnVoid(!encryptionPasswords.isEmpty());

            /* Apply the disk encryption passwords: */
            foreach (const QString &strKey, encryptionPasswords.keys())
            {
                console().AddEncryptionPassword(strKey, encryptionPasswords.value(strKey), false);
                if (!console().isOk())
                    msgCenter().cannotAddDiskEncryptionPassword(console());
            }
        }
        else
        {
            /* Any modal dialog can be destroyed in own event-loop
             * as a part of VM power-off procedure which closes GUI.
             * So we have to check if the dialog still valid.. */

            /* If dialog still valid: */
            if (pDlg)
            {
                /* Delete the dialog: */
                delete pDlg;

                /* Propose the user to close VM: */
                LogRel(("GUI: Request to close Runtime UI due to DEK was not provided.\n"));
                QMetaObject::invokeMethod(this, "sltClose", Qt::QueuedConnection);
            }
        }
    }
}

int UIMachineLogic::searchMaxSnapshotIndex(const CMachine &machine,
                                           const CSnapshot &snapshot,
                                           const QString &strNameTemplate)
{
    int iMaxIndex = 0;
    QRegExp regExp(QString("^") + strNameTemplate.arg("([0-9]+)") + QString("$"));
    if (!snapshot.isNull())
    {
        /* Check the current snapshot name */
        QString strName = snapshot.GetName();
        int iPos = regExp.indexIn(strName);
        if (iPos != -1)
            iMaxIndex = regExp.cap(1).toInt() > iMaxIndex ? regExp.cap(1).toInt() : iMaxIndex;
        /* Traversing all the snapshot children */
        foreach (const CSnapshot &child, snapshot.GetChildren())
        {
            int iMaxIndexOfChildren = searchMaxSnapshotIndex(machine, child, strNameTemplate);
            iMaxIndex = iMaxIndexOfChildren > iMaxIndex ? iMaxIndexOfChildren : iMaxIndex;
        }
    }
    return iMaxIndex;
}

void UIMachineLogic::takeScreenshot(const QString &strFile, const QString &strFormat /* = "png" */) const
{
    /* Get console: */
    const int cGuestScreens = machine().GetGraphicsAdapter().GetMonitorCount();
    QList<QImage> images;
    ULONG uMaxWidth  = 0;
    ULONG uMaxHeight = 0;
    /* First create screenshots of all guest screens and save them in a list.
     * Also sum the width of all images and search for the biggest image height. */
    for (int i = 0; i < cGuestScreens; ++i)
    {
        ULONG width  = 0;
        ULONG height = 0;
        ULONG bpp    = 0;
        LONG xOrigin = 0;
        LONG yOrigin = 0;
        KGuestMonitorStatus monitorStatus = KGuestMonitorStatus_Enabled;
        display().GetScreenResolution(i, width, height, bpp, xOrigin, yOrigin, monitorStatus);
        uMaxWidth  += width;
        uMaxHeight  = RT_MAX(uMaxHeight, height);
        QImage shot = QImage(width, height, QImage::Format_RGB32);
        /* For separate process: */
        if (uiCommon().isSeparateProcess())
        {
            /* Take screen-data to array first: */
            const QVector<BYTE> screenData = display().TakeScreenShotToArray(i, shot.width(), shot.height(), KBitmapFormat_BGR0);
            /* And copy that data to screen-shot if it is Ok: */
            if (display().isOk() && !screenData.isEmpty())
                memcpy(shot.bits(), screenData.data(), shot.width() * shot.height() * 4);
        }
        /* For the same process: */
        else
        {
            /* Take the screen-shot directly: */
            display().TakeScreenShot(i, shot.bits(), shot.width(), shot.height(), KBitmapFormat_BGR0);
        }
        images << shot;
    }
    /* Create a image which will hold all sub images vertically. */
    QImage bigImg = QImage(uMaxWidth, uMaxHeight, QImage::Format_RGB32);
    QPainter p(&bigImg);
    ULONG w = 0;
    /* Paint them. */
    for (int i = 0; i < images.size(); ++i)
    {
        p.drawImage(w, 0, images.at(i));
        w += images.at(i).width();
    }
    p.end();

    /* Save the big image in the requested format: */
    const QFileInfo fi(strFile);
    const QString &strPathWithoutSuffix = QDir(fi.absolutePath()).absoluteFilePath(fi.baseName());
    const QString &strSuffix = fi.suffix().isEmpty() ? strFormat : fi.suffix();
    bigImg.save(QDir::toNativeSeparators(QFile::encodeName(QString("%1.%2").arg(strPathWithoutSuffix, strSuffix))),
                strFormat.toUtf8().constData());
}

void UIMachineLogic::activateScreenSaver()
{
    /* Do nothing if we did not de-activated the host screen saver: */
    if (!gEDataManager->disableHostScreenSaver())
        return;

    QVector<CMachine> machines = uiCommon().virtualBox().GetMachines();
    bool fAnother = false;
    for (int i = 0; i < machines.size(); ++i)
    {
        if (machines[i].GetState() == KMachineState_Running && machines[i].GetId() != machine().GetId())
        {
            fAnother = true;
            break;
        }
    }

    /* Do nothing if there are other vms running.*/
    if (fAnother)
        return;
    sltDisableHostScreenSaverStateChanged(false);
}

void UIMachineLogic::showBootFailureDialog()
{
    UIBootFailureDialog *pBootFailureDialog = new UIBootFailureDialog(activeMachineWindow(), machine());
    AssertPtrReturnVoid(pBootFailureDialog);

    int iResult = pBootFailureDialog->exec(false);
    QString strISOPath = pBootFailureDialog->bootMediumPath();

    delete pBootFailureDialog;

    QFileInfo bootMediumFileInfo(strISOPath);
    if (bootMediumFileInfo.exists() && bootMediumFileInfo.isReadable())
        mountBootMedium(uiCommon().openMedium(UIMediumDeviceType_DVD, strISOPath));

    if (iResult == static_cast<int>(UIBootFailureDialog::ReturnCode_Reset))
        reset(false);
}

bool UIMachineLogic::mountBootMedium(const QUuid &uMediumId)
{
    AssertReturn(!uMediumId.isNull(), false);

    CVirtualBox comVBox = uiCommon().virtualBox();
    CMachine &comMachine = machine();
    const CGuestOSType &comOsType = comVBox.GetGuestOSType(comMachine.GetOSTypeId());
    /* Get recommended controller bus & type: */
    const KStorageBus enmRecommendedDvdBus = comOsType.GetRecommendedDVDStorageBus();
    const KStorageControllerType enmRecommendedDvdType = comOsType.GetRecommendedDVDStorageController();

    CMediumAttachment comAttachment;
    /* Search for an attachment of required bus & type: */
    foreach (const CMediumAttachment &comCurrentAttachment, comMachine.GetMediumAttachments())
    {
        /* Determine current attachment's controller: */
        const CStorageController &comCurrentController = comMachine.GetStorageControllerByName(comCurrentAttachment.GetController());

        if (   comCurrentController.GetBus() == enmRecommendedDvdBus
            && comCurrentController.GetControllerType() == enmRecommendedDvdType
            && comCurrentAttachment.GetType() == KDeviceType_DVD)
        {
            comAttachment = comCurrentAttachment;
            break;
        }
    }
    AssertMsgReturn(!comAttachment.isNull(), ("Storage Controller is NOT properly configured!\n"), false);

    const UIMedium guiMedium = uiCommon().medium(uMediumId);
    const CMedium comMedium = guiMedium.medium();

    /* Mount medium to the predefined port/device: */
    comMachine.MountMedium(comAttachment.GetController(), comAttachment.GetPort(), comAttachment.GetDevice(), comMedium, false /* force */);
    bool fSuccess = comMachine.isOk();

    QWidget *pParent = windowManager().realParentWindow(activeMachineWindow());

    /* Show error message if necessary: */
    if (!fSuccess)
        msgCenter().cannotRemountMedium(comMachine, guiMedium, true /* mount? */, false /* retry? */, pParent);
    else
    {
        /* Save machine settings: */
        comMachine.SaveSettings();
        fSuccess = comMachine.isOk();

        /* Show error message if necessary: */
        if (!fSuccess)
            UINotificationMessage::cannotSaveMachineSettings(machine());
    }
    return fSuccess;
}

#ifdef VBOX_WITH_DEBUGGER_GUI

bool UIMachineLogic::dbgCreated()
{
    if (m_pDbgGui)
        return true;

    RTLDRMOD hLdrMod = uiCommon().getDebuggerModule();
    if (hLdrMod == NIL_RTLDRMOD)
        return false;

    PFNDBGGUICREATE pfnGuiCreate;
    int rc = RTLdrGetSymbol(hLdrMod, "DBGGuiCreate", (void**)&pfnGuiCreate);
    if (RT_SUCCESS(rc))
    {
        ISession *pISession = session().raw();
        rc = pfnGuiCreate(pISession, &m_pDbgGui, &m_pDbgGuiVT);
        if (RT_SUCCESS(rc))
        {
            if (   DBGGUIVT_ARE_VERSIONS_COMPATIBLE(m_pDbgGuiVT->u32Version, DBGGUIVT_VERSION)
                || m_pDbgGuiVT->u32EndVersion == m_pDbgGuiVT->u32Version)
            {
                m_pDbgGuiVT->pfnSetParent(m_pDbgGui, activeMachineWindow());
                m_pDbgGuiVT->pfnSetMenu(m_pDbgGui, actionPool()->action(UIActionIndexRT_M_Debug));
                dbgAdjustRelativePos();
                return true;
            }

            LogRel(("GUI: DBGGuiCreate failed, incompatible versions (loaded %#x/%#x, expected %#x)\n",
                    m_pDbgGuiVT->u32Version, m_pDbgGuiVT->u32EndVersion, DBGGUIVT_VERSION));
        }
        else
            LogRel(("GUI: DBGGuiCreate failed, rc=%Rrc\n", rc));
    }
    else
        LogRel(("GUI: RTLdrGetSymbol(,\"DBGGuiCreate\",) -> %Rrc\n", rc));

    m_pDbgGui = 0;
    m_pDbgGuiVT = 0;
    return false;
}

void UIMachineLogic::dbgDestroy()
{
    if (m_pDbgGui)
    {
        m_pDbgGuiVT->pfnDestroy(m_pDbgGui);
        m_pDbgGui = 0;
        m_pDbgGuiVT = 0;
    }
}
#endif /* VBOX_WITH_DEBUGGER_GUI */

void UIMachineLogic::reset(bool fShowConfirmation)
{
    if (fShowConfirmation)
    {
        /* Confirm/Reset current console: */
        if (msgCenter().confirmResetMachine(machineName()))
            console().Reset();
    }
    else
        console().Reset();

    /* TODO_NEW_CORE: On reset the additional screens didn't get a display
       update. Emulate this for now until it get fixed. */
    ulong uMonitorCount = machine().GetGraphicsAdapter().GetMonitorCount();
    for (ulong uScreenId = 1; uScreenId < uMonitorCount; ++uScreenId)
        machineWindows().at(uScreenId)->update();
}
