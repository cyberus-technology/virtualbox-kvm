/* $Id: UIMachineLogic.h $ */
/** @file
 * VBox Qt GUI - UIMachineLogic class declaration.
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

#ifndef FEQT_INCLUDED_SRC_runtime_UIMachineLogic_h
#define FEQT_INCLUDED_SRC_runtime_UIMachineLogic_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "UIExtraDataDefs.h"
#include "UISettingsDialog.h"

/* COM includes: */
#include "COMEnums.h"

/* Forward declarations: */
class QAction;
class QActionGroup;
class QIManagerDialog;
class UISession;
class UIActionPool;
class UIKeyboardHandler;
class UIMouseHandler;
class UIMachineWindow;
class UIMachineView;
class UIDockIconPreview;
class UISoftKeyboard;
class UIVMInformationDialog;
class CSession;
class CMachine;
class CConsole;
class CDisplay;
class CGuest;
class CMouse;
class CKeyboard;
class CMachineDebugger;
class CSnapshot;
class CUSBDevice;
class CVirtualBoxErrorInfo;
#if defined(VBOX_WS_X11)
 struct X11ScreenSaverInhibitMethod;
#endif

#ifdef VBOX_WITH_DEBUGGER_GUI
typedef struct DBGGUIVT const *PCDBGGUIVT;
typedef struct DBGGUI *PDBGGUI;
#endif /* VBOX_WITH_DEBUGGER_GUI */

/* Machine logic interface: */
class UIMachineLogic : public QIWithRetranslateUI3<QObject>
{
    Q_OBJECT;

    /** Pointer to menu update-handler for this class: */
    typedef void (UIMachineLogic::*MenuUpdateHandler)(QMenu *pMenu);

signals:

    /** Notifies about frame-buffer resize. */
    void sigFrameBufferResize();

public:

    /* Factory functions to create/destroy required logic sub-child: */
    static UIMachineLogic* create(QObject *pParent, UISession *pSession, UIVisualStateType visualStateType);
    static void destroy(UIMachineLogic *pWhichLogic);

    /* Check if this logic is available: */
    virtual bool checkAvailability() = 0;

    /** Returns machine-window flags for current machine-logic and passed @a uScreenId. */
    virtual Qt::WindowFlags windowFlags(ulong uScreenId) const = 0;

    /* Prepare/cleanup the logic: */
    virtual void prepare();
    virtual void cleanup();

    void initializePostPowerUp();

    /* Main getters/setters: */
    UISession* uisession() const { return m_pSession; }
    UIActionPool* actionPool() const;

    /** Returns the session reference. */
    CSession& session() const;
    /** Returns the session's machine reference. */
    CMachine& machine() const;
    /** Returns the session's console reference. */
    CConsole& console() const;
    /** Returns the console's display reference. */
    CDisplay& display() const;
    /** Returns the console's guest reference. */
    CGuest& guest() const;
    /** Returns the console's mouse reference. */
    CMouse& mouse() const;
    /** Returns the console's keyboard reference. */
    CKeyboard& keyboard() const;
    /** Returns the console's debugger reference. */
    CMachineDebugger& debugger() const;

    /** Returns the machine name. */
    const QString& machineName() const;

    UIVisualStateType visualStateType() const { return m_visualStateType; }
    const QList<UIMachineWindow*>& machineWindows() const { return m_machineWindowsList; }
    UIKeyboardHandler* keyboardHandler() const { return m_pKeyboardHandler; }
    UIMouseHandler* mouseHandler() const { return m_pMouseHandler; }
    UIMachineWindow* mainMachineWindow() const;
    UIMachineWindow* activeMachineWindow() const;

    /** Adjusts machine-window(s) geometry if necessary. */
    virtual void adjustMachineWindowsGeometry();

    /** Send machine-window(s) size-hint(s) to the guest. */
    virtual void sendMachineWindowsSizeHints();

    /* Wrapper to open Machine settings / Network page: */
    void openNetworkSettingsDialog() { sltOpenSettingsDialogNetwork(); }

#ifdef VBOX_WS_MAC
    void updateDockIcon();
    void updateDockIconSize(int screenId, int width, int height);
    UIMachineView* dockPreviewView() const;
    virtual void updateDock();
#endif /* VBOX_WS_MAC */

    /** Returns whether VM should perform HID LEDs synchronization. */
    bool isHidLedsSyncEnabled() const { return m_fIsHidLedsSyncEnabled; }
    /** An public interface to sltTypeHostKeyComboPressRelease. */
    void typeHostKeyComboPressRelease(bool fToggleSequence);

#ifdef VBOX_WITH_DEBUGGER_GUI
    /** Adjusts relative position for debugger window. */
    void dbgAdjustRelativePos();
#endif /* VBOX_WITH_DEBUGGER_GUI */

protected slots:

    /** Handles the VBoxSVC availability change. */
    void sltHandleVBoxSVCAvailabilityChange();

    /** Checks if some visual-state type was requested. */
    virtual void sltCheckForRequestedVisualStateType() {}

    /** Requests visual-state change to 'normal' (window). */
    virtual void sltChangeVisualStateToNormal();
    /** Requests visual-state change to 'fullscreen'. */
    virtual void sltChangeVisualStateToFullscreen();
    /** Requests visual-state change to 'seamless'. */
    virtual void sltChangeVisualStateToSeamless();
    /** Requests visual-state change to 'scale'. */
    virtual void sltChangeVisualStateToScale();

    /* Console callback handlers: */
    virtual void sltMachineStateChanged();
    virtual void sltAdditionsStateChanged();
    virtual void sltMouseCapabilityChanged();
    virtual void sltKeyboardLedsChanged();
    virtual void sltUSBDeviceStateChange(const CUSBDevice &device, bool fIsAttached, const CVirtualBoxErrorInfo &error);
    virtual void sltRuntimeError(bool fIsFatal, const QString &strErrorId, const QString &strMessage);
#ifdef RT_OS_DARWIN
    virtual void sltShowWindows();
#endif /* RT_OS_DARWIN */
    /** Handles guest-screen count change. */
    virtual void sltGuestMonitorChange(KGuestMonitorChangedEventType changeType, ulong uScreenId, QRect screenGeo);

    /** Handles host-screen count change. */
    virtual void sltHostScreenCountChange();
    /** Handles host-screen geometry change. */
    virtual void sltHostScreenGeometryChange();
    /** Handles host-screen available-area change. */
    virtual void sltHostScreenAvailableAreaChange();

protected:

    /* Constructor: */
    UIMachineLogic(QObject *pParent, UISession *pSession, UIVisualStateType visualStateType);
    /* Destructor: */
    ~UIMachineLogic();

    /* Protected getters/setters: */
    bool isMachineWindowsCreated() const { return m_fIsWindowsCreated; }
    void setMachineWindowsCreated(bool fIsWindowsCreated) { m_fIsWindowsCreated = fIsWindowsCreated; }

    /* Protected members: */
    void setKeyboardHandler(UIKeyboardHandler *pKeyboardHandler);
    void setMouseHandler(UIMouseHandler *pMouseHandler);
    void addMachineWindow(UIMachineWindow *pMachineWindow);
    void retranslateUi();
#ifdef VBOX_WS_MAC
    bool isDockIconPreviewEnabled() const { return m_fIsDockIconEnabled; }
    void setDockIconPreviewEnabled(bool fIsDockIconPreviewEnabled) { m_fIsDockIconEnabled = fIsDockIconPreviewEnabled; }
    void updateDockOverlay();
#endif /* VBOX_WS_MAC */

    /* Prepare helpers: */
    virtual void prepareRequiredFeatures() {}
    virtual void prepareSessionConnections();
    virtual void prepareActionGroups();
    virtual void prepareActionConnections();
    virtual void prepareOtherConnections();
    virtual void prepareHandlers();
    virtual void prepareMachineWindows() = 0;
    virtual void prepareMenu() {}
#ifdef VBOX_WS_MAC
    virtual void prepareDock();
#endif /* VBOX_WS_MAC */
#ifdef VBOX_WITH_DEBUGGER_GUI
    virtual void prepareDebugger();
#endif /* VBOX_WITH_DEBUGGER_GUI */
    virtual void loadSettings();

    /* Cleanup helpers: */
#ifdef VBOX_WITH_DEBUGGER_GUI
    virtual void cleanupDebugger();
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
    virtual void cleanupDock();
#endif /* VBOX_WS_MAC */
    virtual void cleanupMenu() {}
    virtual void cleanupMachineWindows() = 0;
    virtual void cleanupHandlers();
    //virtual void cleanupOtherConnections() {}
    virtual void cleanupActionConnections() {}
    virtual void cleanupActionGroups() {}
    virtual void cleanupSessionConnections();
    //virtual void cleanupRequiredFeatures() {}

    /* Handler: Event-filter stuff: */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);

private slots:

    /** Handle menu prepare. */
    void sltHandleMenuPrepare(int iIndex, QMenu *pMenu);

    /* "Application" menu functionality: */
    void sltOpenPreferencesDialog(const QString &strCategory = QString(), const QString &strControl = QString());
    void sltOpenPreferencesDialogDefault() { sltOpenPreferencesDialog(); }
    void sltClosePreferencesDialog();
    void sltClose();

    /* "Machine" menu functionality: */
    void sltOpenSettingsDialog(const QString &strCategory = QString(), const QString &strControl = QString());
    void sltOpenSettingsDialogDefault() { sltOpenSettingsDialog(); }
    void sltCloseSettingsDialog();
    void sltTakeSnapshot();
    void sltShowInformationDialog();
    void sltCloseInformationDialog(bool fAsync = false);
    void sltCloseInformationDialogDefault() { sltCloseInformationDialog(true); }
    void sltShowFileManagerDialog();
    void sltCloseFileManagerDialog();
    void sltShowLogDialog();
    void sltCloseLogDialog();
    void sltPause(bool fOn);
    void sltReset();
    void sltDetach();
    void sltSaveState();
    void sltShutdown();
    void sltPowerOff();

    /* "View" menu functionality: */
    void sltMinimizeActiveMachineWindow();
    void sltAdjustMachineWindows();
    void sltToggleGuestAutoresize(bool fEnabled);
    void sltTakeScreenshot();
    void sltOpenRecordingOptions();
    void sltToggleRecording(bool fEnabled);
    void sltToggleVRDE(bool fEnabled);

    /* "Input" menu functionality: */
    void sltShowKeyboardSettings();
    void sltShowSoftKeyboard();
    void sltCloseSoftKeyboard(bool fAsync = false);
    void sltCloseSoftKeyboardDefault() { sltCloseSoftKeyboard(true); }
    void sltTypeCAD();
#ifdef VBOX_WS_X11
    void sltTypeCABS();
#endif /* VBOX_WS_X11 */
    void sltTypeCtrlBreak();
    void sltTypeInsert();
    void sltTypePrintScreen();
    void sltTypeAltPrintScreen();
    void sltTypeHostKeyComboPressRelease(bool fToggleSequence);
    void sltToggleMouseIntegration(bool fEnabled);

    /* "Device" menu functionality: */
    void sltOpenSettingsDialogStorage();
    void sltMountStorageMedium();
    void sltToggleAudioOutput(bool fEnabled);
    void sltToggleAudioInput(bool fEnabled);
    void sltOpenSettingsDialogNetwork();
    void sltOpenSettingsDialogUSBDevices();
    void sltOpenSettingsDialogSharedFolders();
    void sltAttachUSBDevice();
    void sltAttachWebCamDevice();
    void sltChangeSharedClipboardType(QAction *pAction);
    void sltToggleNetworkAdapterConnection();
    void sltChangeDragAndDropType(QAction *pAction);
    void sltInstallGuestAdditions();

#ifdef VBOX_WITH_DEBUGGER_GUI
    /* "Debug" menu functionality: */
    void sltShowDebugStatistics();
    void sltShowDebugCommandLine();
    void sltLoggingToggled(bool);
    void sltShowGuestControlConsoleDialog();
    void sltCloseGuestControlConsoleDialog();
#endif /* VBOX_WITH_DEBUGGER_GUI */

#ifdef RT_OS_DARWIN /* Something is *really* broken in regards of the moc here */
    /* "Window" menu functionality: */
    void sltSwitchToMachineWindow();

    /* "Dock" menu functionality: */
    void sltDockPreviewModeChanged(QAction *pAction);
    void sltDockPreviewMonitorChanged(QAction *pAction);
    void sltChangeDockIconUpdate(bool fEnabled);
    /** Handles dock icon overlay change event. */
    void sltChangeDockIconOverlayAppearance(bool fDisabled);
    /** Handles dock icon overlay disable action triggering. */
    void sltDockIconDisableOverlayChanged(bool fDisabled);
#endif /* RT_OS_DARWIN */

    /* Handlers: Keyboard LEDs sync logic: */
    void sltHidLedsSyncStateChanged(bool fEnabled);
    void sltSwitchKeyboardLedsToGuestLeds();
    void sltSwitchKeyboardLedsToPreviousLeds();

    /* Handle disabling/enabling host screen saver. */
    void sltDisableHostScreenSaverStateChanged(bool fDisabled);

    /** Handles request for visual state change. */
    void sltHandleVisualStateChange();

    /** Handles request to commit data. */
    void sltHandleCommitData();

private:

    /** Update 'Devices' : 'Optical/Floppy Devices' menu routine. */
    void updateMenuDevicesStorage(QMenu *pMenu);
    /** Update 'Devices' : 'Network' menu routine. */
    void updateMenuDevicesNetwork(QMenu *pMenu);
    /** Update 'Devices' : 'USB Devices' menu routine. */
    void updateMenuDevicesUSB(QMenu *pMenu);
    /** Update 'Devices' : 'Web Cams' menu routine. */
    void updateMenuDevicesWebCams(QMenu *pMenu);
    /** Update 'Devices' : 'Shared Clipboard' menu routine. */
    void updateMenuDevicesSharedClipboard(QMenu *pMenu);
    /** Update 'Devices' : 'Drag and Drop' menu routine. */
    void updateMenuDevicesDragAndDrop(QMenu *pMenu);
#ifdef VBOX_WITH_DEBUGGER_GUI
    /** Update 'Debug' menu routine. */
    void updateMenuDebug(QMenu *pMenu);
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
    /** Update 'Window' menu routine. */
    void updateMenuWindow(QMenu *pMenu);
#endif /* VBOX_WS_MAC */

    /** Asks user for the disks encryption passwords. */
    void askUserForTheDiskEncryptionPasswords();

    /* Helpers: */
    static int searchMaxSnapshotIndex(const CMachine &machine, const CSnapshot &snapshot, const QString &strNameTemplate);
    void takeScreenshot(const QString &strFile, const QString &strFormat /* = "png" */) const;

    /** Reactivates the screen saver. This is possbily called during vm window close and enables host screen
      * if there are no other vms running at the moment. Note that this seems to be not needed on Linux since
      * closing vm windows re-activates screen saver automatically. On Windows explicit re-activation is needed. */
    void activateScreenSaver();
    /* Shows the boot failure dialog through which user can mount a boot DVD and reset the vm. */
    void showBootFailureDialog();
    /** Attempts to mount medium with @p uMediumId to the machine if it can find an appropriate controller and port. */
    bool mountBootMedium(const QUuid &uMediumId);
    /** Resets the machine. If @p fShowConfirmation is true then a confirmation messag box is shown first. */
    void reset(bool fShowConfirmation);

    /* Private variables: */
    UISession *m_pSession;
    UIVisualStateType m_visualStateType;
    UIKeyboardHandler *m_pKeyboardHandler;
    UIMouseHandler *m_pMouseHandler;
    QList<UIMachineWindow*> m_machineWindowsList;

    QActionGroup *m_pRunningActions;
    QActionGroup *m_pRunningOrPausedActions;
    QActionGroup *m_pRunningOrPausedOrStackedActions;
    QActionGroup *m_pSharedClipboardActions;
    QActionGroup *m_pDragAndDropActions;

    /** Holds the map of menu update-handlers. */
    QMap<int, MenuUpdateHandler> m_menuUpdateHandlers;

    bool m_fIsWindowsCreated : 1;

#ifdef VBOX_WITH_DEBUGGER_GUI
    /* Debugger functionality: */
    bool dbgCreated();
    void dbgDestroy();
    /* The handle to the debugger GUI: */
    PDBGGUI m_pDbgGui;
    /* The virtual method table for the debugger GUI: */
    PCDBGGUIVT m_pDbgGuiVT;
#endif /* VBOX_WITH_DEBUGGER_GUI */

#ifdef VBOX_WS_MAC
    bool m_fIsDockIconEnabled;
    UIDockIconPreview *m_pDockIconPreview;
    QActionGroup *m_pDockPreviewSelectMonitorGroup;
    QAction *m_pDockSettingsMenuSeparator;
    int m_DockIconPreviewMonitor;
    QAction *m_pDockSettingMenuAction;
    /* Keeps a list of machine menu actions that we add to dock menu. */
    QList<QAction*> m_dockMachineMenuActions;
#endif /* VBOX_WS_MAC */

    void *m_pHostLedsState;

    /** Holds whether VM should perform HID LEDs synchronization. */
    bool m_fIsHidLedsSyncEnabled;

    /** Holds the map of settings dialogs. */
    QMap<UISettingsDialog::DialogType, UISettingsDialog*>  m_settings;

    /** Holds the log viewer dialog instance. */
    QIManagerDialog       *m_pLogViewerDialog;
    QIManagerDialog       *m_pFileManagerDialog;
    QIManagerDialog       *m_pProcessControlDialog;
    UISoftKeyboard        *m_pSoftKeyboardDialog;
    UIVMInformationDialog *m_pVMInformationDialog;

    /* Holds the cookies returnd by QDBus inhibition calls. Map keys are service name. These are required during uninhibition.*/
    QMap<QString, uint> m_screenSaverInhibitionCookies;
#if defined(VBOX_WS_X11)
    QVector<X11ScreenSaverInhibitMethod*> m_methods;
#endif
};

#endif /* !FEQT_INCLUDED_SRC_runtime_UIMachineLogic_h */
