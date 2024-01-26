/* $Id: UISession.h $ */
/** @file
 * VBox Qt GUI - UISession class declaration.
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

#ifndef FEQT_INCLUDED_SRC_runtime_UISession_h
#define FEQT_INCLUDED_SRC_runtime_UISession_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QObject>
#include <QCursor>
#include <QEvent>
#include <QMap>
#include <QPixmap>

/* GUI includes: */
#include "UIExtraDataDefs.h"
#include "UIMediumDefs.h"
#include "UIMousePointerShapeData.h"

/* COM includes: */
#include "COMEnums.h"
#include "CSession.h"
#include "CMachine.h"
#include "CConsole.h"
#include "CDisplay.h"
#include "CGuest.h"
#include "CMouse.h"
#include "CKeyboard.h"
#include "CMachineDebugger.h"
#include "CMedium.h"

/* Forward declarations: */
class QMenu;
class UIFrameBuffer;
class UIMachine;
class UIMachineLogic;
class UIMachineWindow;
class UIActionPool;
class CUSBDevice;
class CNetworkAdapter;
class CMediumAttachment;
#ifdef VBOX_WS_MAC
class QMenuBar;
#else /* !VBOX_WS_MAC */
class QIcon;
#endif /* !VBOX_WS_MAC */

class UISession : public QObject
{
    Q_OBJECT;

public:

    /** Factory constructor. */
    static bool create(UISession *&pSession, UIMachine *pMachine);
    /** Factory destructor. */
    static void destroy(UISession *&pSession);

    /* API: Runtime UI stuff: */
    bool initialize();
    /** Powers VM up. */
    bool powerUp();
    /** Detaches and closes Runtime UI. */
    void detachUi();
    /** Saves VM state, then closes Runtime UI. */
    void saveState();
    /** Calls for guest shutdown to close Runtime UI. */
    void shutdown();
    /** Powers VM off, then closes Runtime UI. */
    void powerOff(bool fIncludingDiscard);

    /** Returns the session instance. */
    CSession& session() { return m_session; }
    /** Returns the session's machine instance. */
    CMachine& machine() { return m_machine; }
    /** Returns the session's console instance. */
    CConsole& console() { return m_console; }
    /** Returns the console's display instance. */
    CDisplay& display() { return m_display; }
    /** Returns the console's guest instance. */
    CGuest& guest() { return m_guest; }
    /** Returns the console's mouse instance. */
    CMouse& mouse() { return m_mouse; }
    /** Returns the console's keyboard instance. */
    CKeyboard& keyboard() { return m_keyboard; }
    /** Returns the console's debugger instance. */
    CMachineDebugger& debugger() { return m_debugger; }

    /** Returns the machine name. */
    const QString& machineName() const { return m_strMachineName; }

    UIActionPool* actionPool() const { return m_pActionPool; }
    KMachineState machineStatePrevious() const { return m_machineStatePrevious; }
    KMachineState machineState() const { return m_machineState; }
    UIMachineLogic* machineLogic() const;
    QWidget* mainMachineWindow() const;
    WId mainMachineWindowId() const;
    UIMachineWindow *activeMachineWindow() const;

    /** Returns currently cached mouse cursor shape pixmap. */
    QPixmap cursorShapePixmap() const { return m_cursorShapePixmap; }
    /** Returns currently cached mouse cursor mask pixmap. */
    QPixmap cursorMaskPixmap() const { return m_cursorMaskPixmap; }
    /** Returns currently cached mouse cursor size. */
    QSize cursorSize() const { return m_cursorSize; }
    /** Returns currently cached mouse cursor hotspot. */
    QPoint cursorHotspot() const { return m_cursorHotspot; }
    /** Returns currently cached mouse cursor position. */
    QPoint cursorPosition() const { return m_cursorPosition; }

    /** @name Branding stuff.
     ** @{ */
    /** Returns the cached machine-window icon. */
    QIcon *machineWindowIcon() const { return m_pMachineWindowIcon; }
#ifndef VBOX_WS_MAC
    /** Returns redefined machine-window name postfix. */
    QString machineWindowNamePostfix() const { return m_strMachineWindowNamePostfix; }
#endif
    /** @} */

    /** @name Host-screen configuration variables.
     ** @{ */
    /** Returns the list of host-screen geometries we currently have. */
    QList<QRect> hostScreens() const { return m_hostScreens; }
    /** @} */

    /** @name Application Close configuration stuff.
     * @{ */
    /** Defines @a defaultCloseAction. */
    void setDefaultCloseAction(MachineCloseAction defaultCloseAction) { m_defaultCloseAction = defaultCloseAction; }
    /** Returns default close action. */
    MachineCloseAction defaultCloseAction() const { return m_defaultCloseAction; }
    /** Returns merged restricted close actions. */
    MachineCloseAction restrictedCloseActions() const { return m_restrictedCloseActions; }
    /** Returns whether all the close actions are restricted. */
    bool isAllCloseActionsRestricted() const { return m_fAllCloseActionsRestricted; }
    /** @} */

    /** Returns whether visual @a state is allowed. */
    bool isVisualStateAllowed(UIVisualStateType state) const;
    /** Requests visual-state change. */
    void changeVisualState(UIVisualStateType visualStateType);
    /** Requests visual-state to be entered when possible. */
    void setRequestedVisualState(UIVisualStateType visualStateType);
    /** Returns requested visual-state to be entered when possible. */
    UIVisualStateType requestedVisualState() const;

    bool isSaved() const { return machineState() == KMachineState_Saved ||
                                  machineState() == KMachineState_AbortedSaved; }
    bool isTurnedOff() const { return machineState() == KMachineState_PoweredOff ||
                                      machineState() == KMachineState_Saved ||
                                      machineState() == KMachineState_Teleported ||
                                      machineState() == KMachineState_Aborted ||
                                      machineState() == KMachineState_AbortedSaved; }
    bool isPaused() const { return machineState() == KMachineState_Paused ||
                                   machineState() == KMachineState_TeleportingPausedVM; }
    bool isRunning() const { return machineState() == KMachineState_Running ||
                                    machineState() == KMachineState_Teleporting ||
                                    machineState() == KMachineState_LiveSnapshotting; }
    bool isStuck() const { return machineState() == KMachineState_Stuck; }
    bool wasPaused() const { return machineStatePrevious() == KMachineState_Paused ||
                                    machineStatePrevious() == KMachineState_TeleportingPausedVM; }
    bool isInitialized() const { return m_fInitialized; }
    bool isGuestResizeIgnored() const { return m_fIsGuestResizeIgnored; }
    bool isAutoCaptureDisabled() const { return m_fIsAutoCaptureDisabled; }

    /** Returns whether VM is in 'manual-override' mode.
      * @note S.a. #m_fIsManualOverride description for more information. */
    bool isManualOverrideMode() const { return m_fIsManualOverride; }
    /** Defines whether VM is in 'manual-override' mode.
      * @note S.a. #m_fIsManualOverride description for more information. */
    void setManualOverrideMode(bool fIsManualOverride) { m_fIsManualOverride = fIsManualOverride; }

    /* Guest additions state getters: */
    bool isGuestAdditionsActive() const { return (m_ulGuestAdditionsRunLevel > KAdditionsRunLevelType_None); }
    bool isGuestSupportsGraphics() const { return m_fIsGuestSupportsGraphics; }
    /* The double check below is correct, even though it is an implementation
     * detail of the Additions which the GUI should not ideally have to know. */
    bool isGuestSupportsSeamless() const { return isGuestSupportsGraphics() && m_fIsGuestSupportsSeamless; }

    /* Keyboard getters: */
    /** Returns keyboard-state. */
    int keyboardState() const { return m_iKeyboardState; }
    bool isNumLock() const { return m_fNumLock; }
    bool isCapsLock() const { return m_fCapsLock; }
    bool isScrollLock() const { return m_fScrollLock; }
    uint numLockAdaptionCnt() const { return m_uNumLockAdaptionCnt; }
    uint capsLockAdaptionCnt() const { return m_uCapsLockAdaptionCnt; }

    /* Mouse getters: */
    /** Returns mouse-state. */
    int mouseState() const { return m_iMouseState; }
    bool isMouseSupportsAbsolute() const { return m_fIsMouseSupportsAbsolute; }
    bool isMouseSupportsRelative() const { return m_fIsMouseSupportsRelative; }
    bool isMouseSupportsTouchScreen() const { return m_fIsMouseSupportsTouchScreen; }
    bool isMouseSupportsTouchPad() const { return m_fIsMouseSupportsTouchPad; }
    bool isMouseHostCursorNeeded() const { return m_fIsMouseHostCursorNeeded; }
    bool isMouseCaptured() const { return m_fIsMouseCaptured; }
    bool isMouseIntegrated() const { return m_fIsMouseIntegrated; }
    bool isValidPointerShapePresent() const { return m_fIsValidPointerShapePresent; }
    bool isHidingHostPointer() const { return m_fIsHidingHostPointer; }
    /** Returns whether the @a cursorPosition() is valid and could be used by the GUI now. */
    bool isValidCursorPositionPresent() const { return m_fIsValidCursorPositionPresent; }

    /* Common setters: */
    bool pause() { return setPause(true); }
    bool unpause() { return setPause(false); }
    bool setPause(bool fOn);
    void setGuestResizeIgnored(bool fIsGuestResizeIgnored) { m_fIsGuestResizeIgnored = fIsGuestResizeIgnored; }
    void setAutoCaptureDisabled(bool fIsAutoCaptureDisabled) { m_fIsAutoCaptureDisabled = fIsAutoCaptureDisabled; }
    void forgetPreviousMachineState() { m_machineStatePrevious = m_machineState; }

    /* Keyboard setters: */
    void setNumLockAdaptionCnt(uint uNumLockAdaptionCnt) { m_uNumLockAdaptionCnt = uNumLockAdaptionCnt; }
    void setCapsLockAdaptionCnt(uint uCapsLockAdaptionCnt) { m_uCapsLockAdaptionCnt = uCapsLockAdaptionCnt; }

    /* Mouse setters: */
    void setMouseCaptured(bool fIsMouseCaptured) { m_fIsMouseCaptured = fIsMouseCaptured; }
    void setMouseIntegrated(bool fIsMouseIntegrated) { m_fIsMouseIntegrated = fIsMouseIntegrated; }

    /* Screen visibility status for host-desires: */
    bool isScreenVisibleHostDesires(ulong uScreenId) const;
    void setScreenVisibleHostDesires(ulong uScreenId, bool fIsMonitorVisible);

    /* Screen visibility status: */
    bool isScreenVisible(ulong uScreenId) const;
    void setScreenVisible(ulong uScreenId, bool fIsMonitorVisible);

    /* Last screen full-screen size: */
    QSize lastFullScreenSize(ulong uScreenId) const;
    void setLastFullScreenSize(ulong uScreenId, QSize size);

    /** Returns whether guest-screen is undrawable.
     *  @todo: extend this method to all the states when guest-screen is undrawable. */
    bool isGuestScreenUnDrawable() const { return machineState() == KMachineState_Stopping ||
                                                  machineState() == KMachineState_Saving; }

    /* Returns existing framebuffer for the given screen-number;
     * Returns 0 (asserts) if screen-number attribute is out of bounds: */
    UIFrameBuffer* frameBuffer(ulong uScreenId) const;
    /* Sets framebuffer for the given screen-number;
     * Ignores (asserts) if screen-number attribute is out of bounds: */
    void setFrameBuffer(ulong uScreenId, UIFrameBuffer* pFrameBuffer);
    /** Returns existing frame-buffer vector. */
    const QVector<UIFrameBuffer*>& frameBuffers() const { return m_frameBufferVector; }

    /** Updates VRDE Server action state. */
    void updateStatusVRDE() { sltVRDEChange(); }
    /** Updates Recording action state. */
    void updateStatusRecording() { sltRecordingChange(); }
    /** Updates Audio output action state. */
    void updateAudioOutput() { sltAudioAdapterChange(); }
    /** Updates Audio input action state. */
    void updateAudioInput() { sltAudioAdapterChange(); }

    /** @name CPU hardware virtualization features for VM.
     ** @{ */
    /** Returns whether CPU hardware virtualization extension is enabled. */
    KVMExecutionEngine getVMExecutionEngine() const { return m_enmVMExecutionEngine; }
    /** Returns whether nested-paging CPU hardware virtualization extension is enabled. */
    bool isHWVirtExNestedPagingEnabled() const { return m_fIsHWVirtExNestedPagingEnabled; }
    /** Returns whether the VM is currently making use of the unrestricted execution feature of VT-x. */
    bool isHWVirtExUXEnabled() const { return m_fIsHWVirtExUXEnabled; }
    /** @} */

    /** Returns VM's effective paravirtualization provider. */
    KParavirtProvider paraVirtProvider() const { return m_paraVirtProvider; }

    /** Returns the list of visible guest windows. */
    QList<int> listOfVisibleWindows() const;

    /** Returns a vector of media attached to the machine. */
    CMediumVector machineMedia() const;

signals:

    /** Notifies about frame-buffer resize. */
    void sigFrameBufferResize();

    /* Console callback signals: */
    /** Notifies listeners about keyboard state-change. */
    void sigKeyboardStateChange(int iState);
    /** Notifies listeners about mouse state-change. */
    void sigMouseStateChange(int iState);
    /** Notifies listeners about mouse pointer shape change. */
    void sigMousePointerShapeChange();
    /** Notifies listeners about mouse capability change. */
    void sigMouseCapabilityChange();
    /** Notifies listeners about cursor position change. */
    void sigCursorPositionChange();
    void sigKeyboardLedsChange();
    void sigMachineStateChange();
    void sigAdditionsStateChange();
    void sigAdditionsStateActualChange();
    void sigNetworkAdapterChange(const CNetworkAdapter &networkAdapter);
    /** Notifies about storage device change for @a attachment, which was @a fRemoved and it was @a fSilent for guest. */
    void sigStorageDeviceChange(const CMediumAttachment &attachment, bool fRemoved, bool fSilent);
    void sigMediumChange(const CMediumAttachment &mediumAttachment);
    void sigVRDEChange();
    void sigRecordingChange();
    void sigUSBControllerChange();
    void sigUSBDeviceStateChange(const CUSBDevice &device, bool bIsAttached, const CVirtualBoxErrorInfo &error);
    void sigSharedFolderChange();
    void sigRuntimeError(bool bIsFatal, const QString &strErrorId, const QString &strMessage);
#ifdef RT_OS_DARWIN
    void sigShowWindows();
#endif /* RT_OS_DARWIN */
    void sigCPUExecutionCapChange();
    void sigGuestMonitorChange(KGuestMonitorChangedEventType changeType, ulong uScreenId, QRect screenGeo);
    void sigAudioAdapterChange();
    void sigClipboardModeChange(KClipboardMode enmMode);
    void sigDnDModeChange(KDnDMode enmMode);

    /** Notifies about host-screen count change. */
    void sigHostScreenCountChange();
    /** Notifies about host-screen geometry change. */
    void sigHostScreenGeometryChange();
    /** Notifies about host-screen available-area change. */
    void sigHostScreenAvailableAreaChange();

    /* Session signals: */
    void sigInitialized();

public slots:

    /** Handles request to install guest additions image.
      * @param  strSource  Brings the source of image being installed. */
    void sltInstallGuestAdditionsFrom(const QString &strSource);
    /** Mounts DVD adhoc.
      * @param  strSource  Brings the source of image being mounted. */
    void sltMountDVDAdHoc(const QString &strSource);

    /** Defines @a iKeyboardState. */
    void setKeyboardState(int iKeyboardState) { m_iKeyboardState = iKeyboardState; emit sigKeyboardStateChange(m_iKeyboardState); }

    /** Defines @a iMouseState. */
    void setMouseState(int iMouseState) { m_iMouseState = iMouseState; emit sigMouseStateChange(m_iMouseState); }

    /** Closes Runtime UI. */
    void closeRuntimeUI();

private slots:

    /** Detaches COM. */
    void sltDetachCOM();

#ifdef RT_OS_DARWIN
    /** Mac OS X: Handles menu-bar configuration-change. */
    void sltHandleMenuBarConfigurationChange(const QUuid &uMachineID);
#endif /* RT_OS_DARWIN */

    /* Console events slots */
    /** Handles signal about mouse pointer @a shapeData change. */
    void sltMousePointerShapeChange(const UIMousePointerShapeData &shapeData);
    /** Handles signal about mouse capability change to @a fSupportsAbsolute, @a fSupportsRelative,
      * @a fSupportsTouchScreen, @a fSupportsTouchPad and @a fNeedsHostCursor. */
    void sltMouseCapabilityChange(bool fSupportsAbsolute, bool fSupportsRelative,
                                  bool fSupportsTouchScreen, bool fSupportsTouchPad,
                                  bool fNeedsHostCursor);
    /** Handles signal about guest request to change the cursor position to @a uX * @a uY.
      * @param  fContainsData  Brings whether the @a uX and @a uY values are valid and could be used by the GUI now. */
    void sltCursorPositionChange(bool fContainsData, unsigned long uX, unsigned long uY);
    void sltKeyboardLedsChangeEvent(bool fNumLock, bool fCapsLock, bool fScrollLock);
    void sltStateChange(KMachineState state);
    void sltAdditionsChange();
    void sltVRDEChange();
    void sltRecordingChange();
    void sltGuestMonitorChange(KGuestMonitorChangedEventType changeType, ulong uScreenId, QRect screenGeo);
    /** Handles storage device change for @a attachment, which was @a fRemoved and it was @a fSilent for guest. */
    void sltHandleStorageDeviceChange(const CMediumAttachment &attachment, bool fRemoved, bool fSilent);
    /** Handles audio adapter change. */
    void sltAudioAdapterChange();
    /** Handles clip board mode change. */
    void sltClipboardModeChange(KClipboardMode enmMode);
    /** Handles drag and drop mode change. */
    void sltDnDModeChange(KDnDMode enmMode);

    /* Handlers: Display reconfiguration stuff: */
#ifdef RT_OS_DARWIN
    void sltHandleHostDisplayAboutToChange();
    void sltCheckIfHostDisplayChanged();
#endif /* RT_OS_DARWIN */

    /** Handles host-screen count change. */
    void sltHandleHostScreenCountChange();
    /** Handles host-screen geometry change. */
    void sltHandleHostScreenGeometryChange();
    /** Handles host-screen available-area change. */
    void sltHandleHostScreenAvailableAreaChange();

    /** Handles signal about machine state saved.
      * @param  fSuccess  Brings whether state was saved successfully. */
    void sltHandleMachineStateSaved(bool fSuccess);
    /** Handles signal about machine powered off.
      * @param  fSuccess           Brings whether machine was powered off successfully.
      * @param  fIncludingDiscard  Brings whether machine state should be discarded. */
    void sltHandleMachinePoweredOff(bool fSuccess, bool fIncludingDiscard);
    /** Handles signal about snapshot restored.
      * @param  fSuccess  Brings whether machine was powered off successfully. */
    void sltHandleSnapshotRestored(bool fSuccess);

private:

    /** Constructor. */
    UISession(UIMachine *pMachine);
    /** Destructor. */
    ~UISession();

    /* Private getters: */
    UIMachine* uimachine() const { return m_pMachine; }

    /* Prepare helpers: */
    bool prepare();
    bool prepareSession();
    void prepareNotificationCenter();
    void prepareConsoleEventHandlers();
    void prepareFramebuffers();
    void prepareActions();
    void prepareConnections();
    void prepareMachineWindowIcon();
    void prepareScreens();
    void prepareSignalHandling();

    /* Settings stuff: */
    void loadSessionSettings();

    /* Cleanup helpers: */
    //void cleanupSignalHandling();
    //void cleanupScreens() {}
    void cleanupMachineWindowIcon();
    void cleanupConnections();
    void cleanupActions();
    void cleanupFramebuffers();
    void cleanupConsoleEventHandlers();
    void cleanupNotificationCenter();
    void cleanupSession();
    void cleanup();

#ifdef VBOX_WS_MAC
    /** Mac OS X: Updates menu-bar content. */
    void updateMenu();
#endif /* VBOX_WS_MAC */

    /** Updates mouse pointer shape. */
    void updateMousePointerShape();

    /* Common helpers: */
    bool preprocessInitialization();
    bool mountAdHocImage(KDeviceType enmDeviceType, UIMediumDeviceType enmMediumType, const QString &strMediumName);
    bool postprocessInitialization();
    int countOfVisibleWindows();
    /** Loads VM settings. */
    void loadVMSettings();

    /** Update host-screen data. */
    void updateHostScreenData();

    /** Updates action restrictions. */
    void updateActionRestrictions();

    /* Check if GA can be upgraded. */
    bool guestAdditionsUpgradable();
    /* Private variables: */
    UIMachine *m_pMachine;

    /** Holds the session instance. */
    CSession m_session;
    /** Holds the session's machine instance. */
    CMachine m_machine;
    /** Holds the session's console instance. */
    CConsole m_console;
    /** Holds the console's display instance. */
    CDisplay m_display;
    /** Holds the console's guest instance. */
    CGuest m_guest;
    /** Holds the console's mouse instance. */
    CMouse m_mouse;
    /** Holds the console's keyboard instance. */
    CKeyboard m_keyboard;
    /** Holds the console's debugger instance. */
    CMachineDebugger m_debugger;

    /** Holds the machine name. */
    QString m_strMachineName;

    /** Holds the action-pool instance. */
    UIActionPool *m_pActionPool;

#ifdef VBOX_WS_MAC
    /** Holds the menu-bar instance. */
    QMenuBar *m_pMenuBar;
#endif /* VBOX_WS_MAC */

    /* Screen visibility vector: */
    QVector<bool> m_monitorVisibilityVector;

    /* Screen visibility vector for host-desires: */
    QVector<bool> m_monitorVisibilityVectorHostDesires;

    /* Screen last full-screen size vector: */
    QVector<QSize> m_monitorLastFullScreenSizeVector;

    /* Frame-buffers vector: */
    QVector<UIFrameBuffer*> m_frameBufferVector;

    /* Common variables: */
    KMachineState m_machineStatePrevious;
    KMachineState m_machineState;

    /** Holds cached mouse cursor shape pixmap. */
    QPixmap  m_cursorShapePixmap;
    /** Holds cached mouse cursor mask pixmap. */
    QPixmap  m_cursorMaskPixmap;
    /** Holds cached mouse cursor size. */
    QSize    m_cursorSize;
    /** Holds cached mouse cursor hotspot. */
    QPoint   m_cursorHotspot;
    /** Holds cached mouse cursor position. */
    QPoint   m_cursorPosition;

    /** @name Branding variables.
     ** @{ */
    /** Holds the cached machine-window icon. */
    QIcon *m_pMachineWindowIcon;
#ifndef VBOX_WS_MAC
    /** Holds redefined machine-window name postfix. */
    QString m_strMachineWindowNamePostfix;
#endif
    /** @} */

    /** @name Host-screen configuration variables.
     * @{ */
    /** Holds the list of host-screen geometries we currently have. */
    QList<QRect> m_hostScreens;
#ifdef VBOX_WS_MAC
    /** Mac OS X: Watchdog timer looking for display reconfiguration. */
    QTimer *m_pWatchdogDisplayChange;
#endif /* VBOX_WS_MAC */
    /** @} */

    /** @name Application Close configuration variables.
     * @{ */
    /** Default close action. */
    MachineCloseAction m_defaultCloseAction;
    /** Merged restricted close actions. */
    MachineCloseAction m_restrictedCloseActions;
    /** Determines whether all the close actions are restricted. */
    bool m_fAllCloseActionsRestricted;
    /** @} */

    /* Common flags: */
    bool m_fInitialized : 1;
    bool m_fIsGuestResizeIgnored : 1;
    bool m_fIsAutoCaptureDisabled : 1;
    /** Holds whether VM is in 'manual-override' mode
      * which means there will be no automatic UI shutdowns,
      * visual representation mode changes and other stuff. */
    bool m_fIsManualOverride : 1;

    /* Guest additions flags: */
    ULONG m_ulGuestAdditionsRunLevel;
    bool  m_fIsGuestSupportsGraphics : 1;
    bool  m_fIsGuestSupportsSeamless : 1;

    /* Keyboard flags: */
    /** Holds the keyboard-state. */
    int m_iKeyboardState;
    bool m_fNumLock : 1;
    bool m_fCapsLock : 1;
    bool m_fScrollLock : 1;
    uint m_uNumLockAdaptionCnt;
    uint m_uCapsLockAdaptionCnt;

    /* Mouse flags: */
    /** Holds the mouse-state. */
    int m_iMouseState;
    bool m_fIsMouseSupportsAbsolute : 1;
    bool m_fIsMouseSupportsRelative : 1;
    bool m_fIsMouseSupportsTouchScreen: 1;
    bool m_fIsMouseSupportsTouchPad: 1;
    bool m_fIsMouseHostCursorNeeded : 1;
    bool m_fIsMouseCaptured : 1;
    bool m_fIsMouseIntegrated : 1;
    bool m_fIsValidPointerShapePresent : 1;
    bool m_fIsHidingHostPointer : 1;
    /** Holds whether the @a m_cursorPosition is valid and could be used by the GUI now. */
    bool m_fIsValidCursorPositionPresent : 1;
    /** Holds the mouse pointer shape data. */
    UIMousePointerShapeData  m_shapeData;

    /** Copy of IMachineDebugger::ExecutionEngine */
    KVMExecutionEngine m_enmVMExecutionEngine;

    /** @name CPU hardware virtualization features for VM.
     ** @{ */
    /** Holds whether nested-paging CPU hardware virtualization extension is enabled. */
    bool m_fIsHWVirtExNestedPagingEnabled;
    /** Holds whether the VM is currently making use of the unrestricted execution feature of VT-x. */
    bool m_fIsHWVirtExUXEnabled;
    /** @} */

    /** Holds VM's effective paravirtualization provider. */
    KParavirtProvider m_paraVirtProvider;
};

#endif /* !FEQT_INCLUDED_SRC_runtime_UISession_h */
