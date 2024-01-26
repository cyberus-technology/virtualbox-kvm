/* $Id: UICommon.h $ */
/** @file
 * VBox Qt GUI - UICommon class declaration.
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

#ifndef FEQT_INCLUDED_SRC_globals_UICommon_h
#define FEQT_INCLUDED_SRC_globals_UICommon_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QMap>
#include <QReadWriteLock>
#include <QObject>

/* GUI includes: */
#include "UIDefs.h"
#include "UILibraryDefs.h"
#include "UIMediumDefs.h"
#ifdef VBOX_WS_X11
# include "VBoxUtils-x11.h"
#endif

/* COM includes: */
#include "CGuestOSType.h"
#include "CHost.h"
#include "CMedium.h"
#include "CSession.h"
#include "CVirtualBox.h"
#include "CVirtualBoxClient.h"

/* Other VBox includes: */
#include "VBox/com/Guid.h"

/* Forward declarations: */
class QGraphicsWidget;
class QMenu;
class QSessionManager;
class QSpinBox;
class QToolButton;
class CCloudMachine;
class CHostVideoInputDevice;
class CMachine;
class CUSBDevice;
class UIActionPool;
class UIMedium;
class UIMediumEnumerator;
class UIThreadPool;

/** QObject subclass containing common GUI functionality. */
class SHARED_LIBRARY_STUFF UICommon : public QObject
{
    Q_OBJECT;

signals:

    /** @name Common stuff.
     * @{ */
        /** Asks #UIStarter listener to restart UI. */
        void sigAskToRestartUI();
        /** Asks #UIStarter listener to close UI. */
        void sigAskToCloseUI();

        /** Notifies listeners about the VBoxSVC availability change. */
        void sigVBoxSVCAvailabilityChange();

        /** Asks listeners to commit data. */
        void sigAskToCommitData();
        /** Asks listeners to detach COM. */
        void sigAskToDetachCOM();
    /** @} */

    /** @name COM: Extension Pack stuff.
     * @{ */
        /** Notifies listeners about extension pack @a strName was installed. */
        void sigExtensionPackInstalled(const QString &strName);
    /** @} */

    /** @name Cloud Virtual Machine stuff.
     * @{ */
        /** Notifies listeners about cloud VM was unregistered.
          * @param  strProviderShortName  Brings provider short name.
          * @param  strProfileName        Brings profile name.
          * @param  uId                   Brings cloud VM id. */
        void sigCloudMachineUnregistered(const QString &strProviderShortName,
                                         const QString &strProfileName,
                                         const QUuid &uId);
        /** Notifies listeners about cloud VM was registered.
          * @param  strProviderShortName  Brings provider short name.
          * @param  strProfileName        Brings profile name.
          * @param  comMachine            Brings cloud VM. */
        void sigCloudMachineRegistered(const QString &strProviderShortName,
                                       const QString &strProfileName,
                                       const CCloudMachine &comMachine);
    /** @} */

    /** @name COM: Virtual Media stuff.
     * @{ */
        /** Notifies listeners about medium with certain @a uMediumID created. */
        void sigMediumCreated(const QUuid &uMediumID);
        /** Notifies listeners about medium with certain @a uMediumID deleted. */
        void sigMediumDeleted(const QUuid &uMediumID);

        /** Notifies listeners about medium-enumeration started. */
        void sigMediumEnumerationStarted();
        /** Notifies listeners about medium with certain @a uMediumID enumerated. */
        void sigMediumEnumerated(const QUuid &uMediumID);
        /** Notifies listeners about medium-enumeration finished. */
        void sigMediumEnumerationFinished();
        /** Notifies listeners about update of recently media list. */
        void sigRecentMediaListUpdated(UIMediumDeviceType enmMediumType);
    /** @} */

public:

    /** UI types. */
    enum UIType
    {
        UIType_SelectorUI,
        UIType_RuntimeUI
    };

    /** VM launch running options. */
    enum LaunchRunning
    {
        LaunchRunning_Default, /**< Default (depends on debug settings). */
        LaunchRunning_No,      /**< Start the VM paused. */
        LaunchRunning_Yes      /**< Start the VM running. */
    };

    /** Returns UICommon instance. */
    static UICommon *instance() { return s_pInstance; }
    /** Creates UICommon instance of passed @a enmType. */
    static void create(UIType enmType);
    /** Destroys UICommon instance. */
    static void destroy();

    /** @name General stuff.
     * @{ */
        /** Returns the UI type. */
        UIType uiType() const { return m_enmType; }

        /** Returns whether UICommon instance is properly initialized. */
        bool isValid() const { return m_fValid; }
        /** Returns whether UICommon instance cleanup is in progress. */
        bool isCleaningUp() const { return m_fCleaningUp; }
    /** @} */

    /** @name Versioning stuff.
     * @{ */
        /** Returns Qt runtime version string. */
        static QString qtRTVersionString();
        /** Returns Qt runtime version. */
        static uint qtRTVersion();
        /** Returns Qt runtime major version. */
        static uint qtRTMajorVersion();
        /** Returns Qt runtime minor version. */
        static uint qtRTMinorVersion();
        /** Returns Qt runtime revision number. */
        static uint qtRTRevisionNumber();

        /** Returns Qt compiled version string. */
        static QString qtCTVersionString();
        /** Returns Qt compiled version. */
        static uint qtCTVersion();

        /** Returns VBox version string. */
        QString vboxVersionString() const;
        /** Returns normalized VBox version string. */
        QString vboxVersionStringNormalized() const;
        /** Returns whether VBox version string contains BETA word. */
        bool isBeta() const;
        /** Returns whether BETA label should be shown. */
        bool showBetaLabel() const;

        /** Returns whether branding is active. */
        bool brandingIsActive(bool fForce = false);
        /** Returns value for certain branding @a strKey from custom.ini file. */
        QString brandingGetKey(QString strKey) const;
    /** @} */

    /** @name Host OS stuff.
     * @{ */
#ifdef VBOX_WS_WIN
        /** Loads the color theme. */
        static void loadColorTheme();
#endif

#ifdef VBOX_WS_X11
        /** X11: Returns the type of the Window Manager we are running under. */
        X11WMType typeOfWindowManager() const { return m_enmWindowManagerType; }
        /** X11: Returns whether the Window Manager we are running under is composition one. */
        bool isCompositingManagerRunning() const { return m_fCompositingManagerRunning; }
#endif
    /** @} */

    /** @name Process arguments stuff.
     * @{ */
        /** Process application args. */
        bool processArgs();

        /** Returns whether there are unhandled URL arguments present. */
        bool argumentUrlsPresent() const;
        /** Takes and returns the URL argument list while clearing the source. */
        QList<QUrl> takeArgumentUrls();

        /** Returns the --startvm option value (managed VM id). */
        QUuid managedVMUuid() const { return m_strManagedVMId; }
        /** Returns the --separate option value (whether GUI process is separate from VM process). */
        bool isSeparateProcess() const { return m_fSeparateProcess; }
        /** Returns the --no-startvm-errormsgbox option value (whether startup VM errors are disabled). */
        bool showStartVMErrors() const { return m_fShowStartVMErrors; }

        /** Returns the --aggressive-caching / --no-aggressive-caching option value (whether medium-enumeration is required). */
        bool agressiveCaching() const { return m_fAgressiveCaching; }

        /** Returns the --restore-current option value (whether we should restore current snapshot before VM started). */
        bool shouldRestoreCurrentSnapshot() const { return m_fRestoreCurrentSnapshot; }
        /** Defines whether we should fRestore current snapshot before VM started. */
        void setShouldRestoreCurrentSnapshot(bool fRestore) { m_fRestoreCurrentSnapshot = fRestore; }

        /** Returns the --fda option value (whether we have floppy image). */
        bool hasFloppyImageToMount() const { return !m_uFloppyImage.isNull(); }
        /** Returns the --dvd | --cdrom option value (whether we have DVD image). */
        bool hasDvdImageToMount() const { return !m_uDvdImage.isNull(); }
        /** Returns floppy image name. */
        QUuid getFloppyImage() const { return m_uFloppyImage; }
        /** Returns DVD image name. */
        QUuid getDvdImage() const { return m_uDvdImage; }

        /** Returns the --execute-all-in-iem option value. */
        bool areWeToExecuteAllInIem() const { return m_fExecuteAllInIem; }
        /** Returns whether --warp-factor option value is equal to 100. */
        bool isDefaultWarpPct() const { return m_uWarpPct == 100; }
        /** Returns the --warp-factor option value. */
        uint32_t getWarpPct() const { return m_uWarpPct; }

#ifdef VBOX_WITH_DEBUGGER_GUI
        /** Holds whether the debugger should be accessible. */
        bool isDebuggerEnabled() const;
        /** Holds whether to show the debugger automatically with the console. */
        bool isDebuggerAutoShowEnabled() const;
        /** Holds whether to show the command line window when m_fDbgAutoShow is set. */
        bool isDebuggerAutoShowCommandLineEnabled() const;
        /** Holds whether to show the statistics window when m_fDbgAutoShow is set. */
        bool isDebuggerAutoShowStatisticsEnabled() const;
        /** Returns the combined --statistics-expand values. */
        QString const getDebuggerStatisticsExpand() const { return m_strDbgStatisticsExpand; }
        /** Returns the --statistics-filter value. */
        QString const getDebuggerStatisticsFilter() const { return m_strDbgStatisticsFilter; }

        /** VBoxDbg module handle. */
        RTLDRMOD getDebuggerModule() const { return m_hVBoxDbg; }
#endif

        /** Returns whether VM should start paused. */
        bool shouldStartPaused() const;

#ifdef VBOX_GUI_WITH_PIDFILE
        /** Creates PID file. */
        void createPidfile();
        /** Deletes PID file. */
        void deletePidfile();
#endif
    /** @} */

    /** @name COM stuff.
     * @{ */
        /** Try to acquire COM cleanup protection token for reading. */
        bool comTokenTryLockForRead() { return m_comCleanupProtectionToken.tryLockForRead(); }
        /** Unlock previously acquired COM cleanup protection token. */
        void comTokenUnlock() { return m_comCleanupProtectionToken.unlock(); }

        /** Returns the copy of VirtualBox client wrapper. */
        CVirtualBoxClient virtualBoxClient() const { return m_comVBoxClient; }
        /** Returns the copy of VirtualBox object wrapper. */
        CVirtualBox virtualBox() const { return m_comVBox; }
        /** Returns the copy of VirtualBox host-object wrapper. */
        CHost host() const { return m_comHost; }
        /** Returns the symbolic VirtualBox home-folder representation. */
        QString homeFolder() const { return m_strHomeFolder; }

        /** Returns the VBoxSVC availability value. */
        bool isVBoxSVCAvailable() const { return m_fVBoxSVCAvailable; }
    /** @} */

    /** @name COM: Guest OS Type stuff.
     * @{ */
        /** Returns the list of family IDs. */
        QList<QString> vmGuestOSFamilyIDs() const { return m_guestOSFamilyIDs; }

        /** Returns a family description with passed @a strFamilyId. */
        QString vmGuestOSFamilyDescription(const QString &strFamilyId) const;
        /** Returns a list of all guest OS types with passed @a strFamilyId. */
        QList<CGuestOSType> vmGuestOSTypeList(const QString &strFamilyId) const;

        /** Returns the guest OS type for passed @a strTypeId.
          * It is being serached through the list of family with passed @a strFamilyId if specified. */
        CGuestOSType vmGuestOSType(const QString &strTypeId, const QString &strFamilyId = QString()) const;
        /** Returns a type description with passed @a strTypeId. */
        QString vmGuestOSTypeDescription(const QString &strTypeId) const;

        /** Returns whether guest type with passed @a strOSTypeId is one of DOS types. */
        static bool isDOSType(const QString &strOSTypeId);
    /** @} */

    /** @name COM: Virtual Machine stuff.
     * @{ */
        /** Switches to certain @a comMachine. */
        static bool switchToMachine(CMachine &comMachine);
        /** Launches certain @a comMachine in specified @a enmLaunchMode. */
        static bool launchMachine(CMachine &comMachine, UILaunchMode enmLaunchMode = UILaunchMode_Default);

        /** Opens session of certain @a enmLockType for VM with certain @a uId. */
        CSession openSession(const QUuid &uId, KLockType enmLockType = KLockType_Write);
        /** Opens session of KLockType_Shared type for VM with certain @a uId. */
        CSession openExistingSession(const QUuid &uId) { return openSession(uId, KLockType_Shared); }
        /** Tries to guess if new @a comSession needs to be opened for certain @a comMachine,
          * if yes, new session of required type will be opened and machine will be updated,
          * otherwise, no session will be created and machine will be left unchanged. */
        CSession tryToOpenSessionFor(CMachine &comMachine);
    /** @} */

    /** @name COM: Cloud Virtual Machine stuff.
     * @{ */
        /** Notifies listeners about cloud VM was unregistered.
          * @param  strProviderShortName  Brings provider short name.
          * @param  strProfileName        Brings profile name.
          * @param  uId                   Brings cloud VM id. */
        void notifyCloudMachineUnregistered(const QString &strProviderShortName,
                                            const QString &strProfileName,
                                            const QUuid &uId);
        /** Notifies listeners about cloud VM was registered.
          * @param  strProviderShortName  Brings provider short name.
          * @param  strProfileName        Brings profile name.
          * @param  comMachine            Brings cloud VM. */
        void notifyCloudMachineRegistered(const QString &strProviderShortName,
                                          const QString &strProfileName,
                                          const CCloudMachine &comMachine);
    /** @} */

    /** @name COM: Virtual Media stuff.
     * @{ */
        /** Enumerates passed @a comMedia. */
        void enumerateMedia(const CMediumVector &comMedia = CMediumVector());
        /** Calls refresh for each medium which has been already enumerated. */
        void refreshMedia();
        /** Returns whether full medium-enumeration is requested. */
        bool isFullMediumEnumerationRequested() const;
        /** Returns whether any medium-enumeration is in progress. */
        bool isMediumEnumerationInProgress() const;
        /** Returns enumerated medium with certain @a uMediumID. */
        UIMedium medium(const QUuid &uMediumID) const;
        /** Returns enumerated medium IDs. */
        QList<QUuid> mediumIDs() const;
        /** Creates medium on the basis of passed @a guiMedium description. */
        void createMedium(const UIMedium &guiMedium);

        /** Opens external medium by passed @a strMediumLocation.
          * @param  enmMediumType      Brings the medium type.
          * @param  pParent            Brings the dialog parent.
          * @param  strMediumLocation  Brings the file path to load medium from.
          * @param  pParent            Brings the dialog parent. */
        QUuid openMedium(UIMediumDeviceType enmMediumType, QString strMediumLocation, QWidget *pParent = 0);

        /** Opens external medium using file-open dialog.
          * @param  enmMediumType     Brings the medium type.
          * @param  pParent           Brings the dialog parent.
          * @param  strDefaultFolder  Brings the folder to browse for medium.
          * @param  fUseLastFolder    Brings whether we should propose to use last used folder. */
        QUuid openMediumWithFileOpenDialog(UIMediumDeviceType enmMediumType, QWidget *pParent = 0,
                                           const QString &strDefaultFolder = QString(), bool fUseLastFolder = false);

        /** Creates and shows a dialog (wizard) to create a medium of type @a enmMediumType.
          * @param  pParent                  Passes the parent of the dialog,
          * @param  enmMediumType            Passes the medium type,
          * @param  strMachineName           Passes the name of the machine,
          * @param  strMachineFolder         Passes the machine folder,
          * @param  strMachineGuestOSTypeId  Passes the type ID of machine's guest os,
          * @param  fEnableCreate            Passes whether to show/enable create action in the medium selector dialog,
          * returns QUuid of the new medium */
        QUuid openMediumCreatorDialog(UIActionPool *pActionPool, QWidget *pParent, UIMediumDeviceType  enmMediumType,
                                      const QString &strMachineFolder = QString(), const QString &strMachineName = QString(),
                                      const QString &strMachineGuestOSTypeId = QString());

        /** Prepares storage menu according passed parameters.
          * @param  menu               Brings the #QMenu to be prepared.
          * @param  pListener          Brings the listener #QObject, this @a menu being prepared for.
          * @param  pszSlotName        Brings the name of the SLOT in the @a pListener above, this menu will be handled with.
          * @param  comMachine         Brings the #CMachine object, this @a menu being prepared for.
          * @param  strControllerName  Brings the name of the #CStorageController in the @a machine above.
          * @param  storageSlot        Brings the #StorageSlot of the storage controller with @a strControllerName above. */
        void prepareStorageMenu(QMenu &menu,
                                QObject *pListener, const char *pszSlotName,
                                const CMachine &comMachine, const QString &strControllerName, const StorageSlot &storageSlot);
        /** Updates @a comConstMachine storage with data described by @a target. */
        void updateMachineStorage(const CMachine &comConstMachine, const UIMediumTarget &target, UIActionPool *pActionPool);

        /** Generates details for passed @a comMedium.
          * @param  fPredictDiff  Brings whether medium will be marked differencing on attaching.
          * @param  fUseHtml      Brings whether HTML subsets should be used in the generated output. */
        QString storageDetails(const CMedium &comMedium, bool fPredictDiff, bool fUseHtml = true);

        /** Update extra data related to recently used/referred media.
          * @param  enmMediumType       Passes the medium type.
          * @param  strMediumLocation   Passes the medium location. */
        void updateRecentlyUsedMediumListAndFolder(UIMediumDeviceType enmMediumType, QString strMediumLocation);

        /** Searches extra data for the recently used folder path which corresponds to @a enmMediumType. When that search fails
            it looks for recent folder extra data for other medium types. As the last resort returns default vm folder path.
          * @param  enmMediumType       Passes the medium type. */
        QString defaultFolderPathForType(UIMediumDeviceType enmMediumType);
    /** @} */

    /** @name COM: USB stuff.
     * @{ */
#ifdef RT_OS_LINUX
        /** Verifies that USB drivers are properly configured on Linux. */
        static void checkForWrongUSBMounted();
#endif

        /** Generates details for passed USB @a comDevice. */
        static QString usbDetails(const CUSBDevice &comDevice);
        /** Generates tool-tip for passed USB @a comDevice. */
        static QString usbToolTip(const CUSBDevice &comDevice);
        /** Generates tool-tip for passed USB @a comFilter. */
        static QString usbToolTip(const CUSBDeviceFilter &comFilter);
        /** Generates tool-tip for passed USB @a comWebcam. */
        static QString usbToolTip(const CHostVideoInputDevice &comWebcam);
    /** @} */

    /** @name COM: Recording stuff.
     * @{ */
        /** Returns supported recording features flag. */
        int supportedRecordingFeatures() const;
    /** @} */

    /** @name File-system stuff.
     * @{ */
        /** Returns full help file name. */
        static QString helpFile();

        /** Returns documents path. */
        static QString documentsPath();

        /** Returns whether passed @a strFileName ends with one of allowed extension in the @a extensions list. */
        static bool hasAllowedExtension(const QString &strFileName, const QStringList &extensions);

        /** Returns a file name (unique up to extension) wrt. @a strFullFolderPath folder content. Starts
          * searching strBaseFileName and adds suffixes until a unique file name is found. */
        static QString findUniqueFileName(const QString &strFullFolderPath, const QString &strBaseFileName);
    /** @} */

    /** @name Widget stuff.
     * @{ */
        /** Assigns minimum @a pSpinBox to correspond to @a cCount digits. */
        static void setMinimumWidthAccordingSymbolCount(QSpinBox *pSpinBox, int cCount);
    /** @} */

    /** @name Display stuff.
     * @{ */
#ifdef VBOX_WITH_3D_ACCELERATION
        /** Returns whether guest OS type with passed @a strGuestOSTypeId is WDDM compatible. */
        static bool isWddmCompatibleOsType(const QString &strGuestOSTypeId);
#endif
        /** Returns the required video memory in bytes for the current desktop
          * resolution at maximum possible screen depth in bpp. */
        static quint64 requiredVideoMemory(const QString &strGuestOSTypeId, int cMonitors = 1);
    /** @} */

    /** @name Thread stuff.
     * @{ */
        /** Returns the thread-pool instance. */
        UIThreadPool *threadPool() const { return m_pThreadPool; }
        /** Returns the thread-pool instance for cloud needs. */
        UIThreadPool *threadPoolCloud() const { return m_pThreadPoolCloud; }
    /** @} */

    /** @name Context sensitive help related functionality
     * @{ */
        /** Sets the property for help keyword on a QObject
          * @param  pObject      The object to set the help keyword property on
          * @param  strKeyword   The values of the key word property. */
        static void setHelpKeyword(QObject *pObject, const QString &strHelpKeyword);
        /** Returns the property for help keyword of a QObject. If no such property exists returns an empty QString.
          * @param  pWidget      The object to get the help keyword property from. */
        static QString helpKeyword(const QObject *pWidget);
    /** @} */

public slots:

    /** @name Process arguments stuff.
     * @{ */
        /** Opens the specified URL using OS/Desktop capabilities. */
        bool openURL(const QString &strURL) const;
    /** @} */

    /** @name Localization stuff.
     * @{ */
        /** Handles language change to new @a strLanguage. */
        void sltGUILanguageChange(QString strLanguage);
    /** @} */

    /** @name Media related stuff.
     * @{ */
        /** Handles signal about medium was created. */
        void sltHandleMediumCreated(const CMedium &comMedium);
    /** @} */

    /** @name Machine related stuff.
     * @{ */
        /** Handles signal about machine was created. */
        void sltHandleMachineCreated(const CMachine &comMachine);
    /** @} */

    /** @name Cloud Machine related stuff.
     * @{ */
        /** Handles signal about cloud machine was added. */
        void sltHandleCloudMachineAdded(const QString &strProviderShortName,
                                        const QString &strProfileName,
                                        const CCloudMachine &comMachine);
    /** @} */

protected:

    /** Preprocesses any Qt @a pEvent for passed @a pObject. */
    virtual bool eventFilter(QObject *pObject, QEvent *pEvent) RT_OVERRIDE;

    /** Handles translation event. */
    virtual void retranslateUi();

protected slots:

    /** Calls for cleanup() functionality. */
    void sltCleanup() { cleanup(); }

#ifndef VBOX_GUI_WITH_CUSTOMIZATIONS1
    /** @name Common stuff.
     * @{ */
        /** Handles @a manager request for emergency session shutdown. */
        void sltHandleCommitDataRequest(QSessionManager &manager);
    /** @} */
#endif /* VBOX_GUI_WITH_CUSTOMIZATIONS1 */

    /** @name COM stuff.
     * @{ */
        /** Handles the VBoxSVC availability change. */
        void sltHandleVBoxSVCAvailabilityChange(bool fAvailable);
    /** @} */

    /* Handle font scale factor change. */
    void sltHandleFontScaleFactorChanged(int iFontScaleFactor);

private:

    /** Construcs global VirtualBox object of passed @a enmType. */
    UICommon(UIType enmType);
    /** Destrucs global VirtualBox object. */
    virtual ~UICommon() /* override final */;

    /** Prepares all. */
    void prepare();
    /** Cleanups all. */
    void cleanup();

    /** @name Process arguments stuff.
     * @{ */
#ifdef VBOX_WITH_DEBUGGER_GUI
        /** Initializes a debugger config variable.
          * @param  piDbgCfgVar       Brings the debugger config variable to init.
          * @param  pszEnvVar         Brings the environment variable name relating to this variable.
          * @param  pszExtraDataName  Brings the extra data name relating to this variable.
          * @param  fDefault          Brings the default value. */
        void initDebuggerVar(int *piDbgCfgVar, const char *pszEnvVar, const char *pszExtraDataName, bool fDefault = false);
        /** Set a debugger config variable according according to start up argument.
          * @param  piDbgCfgVar  Brings the debugger config variable to set.
          * @param  fState       Brings the value from the command line. */
        void setDebuggerVar(int *piDbgCfgVar, bool fState);
        /** Checks the state of a debugger config variable, updating it with the machine settings on the first invocation.
          * @param  piDbgCfgVar       Brings the debugger config variable to consult.
          * @param  pszExtraDataName  Brings the extra data name relating to this variable. */
        bool isDebuggerWorker(int *piDbgCfgVar, const char *pszExtraDataName) const;
#endif
    /** @} */

    /** @name COM stuff.
     * @{ */
        /** Re-initializes COM wrappers and containers. */
        void comWrappersReinit();
    /** @} */

    /** Holds the singleton UICommon instance. */
    static UICommon *s_pInstance;

    /** @name General stuff.
     * @{ */
        /** Holds the UI type. */
        UIType  m_enmType;

        /** Holds whether UICommon instance is properly initialized. */
        bool  m_fValid;
        /** Holds whether UICommon instance cleanup is in progress. */
        bool  m_fCleaningUp;
#ifdef VBOX_WS_WIN
        /** Holds whether overall GUI data is committed. */
        bool  m_fDataCommitted;
#endif
    /** @} */

    /** @name Versioning stuff.
     * @{ */
        /** Holds the VBox branding config file path. */
        QString  m_strBrandingConfigFilePath;
    /** @} */

    /** @name Host OS stuff.
     * @{ */
#ifdef VBOX_WS_X11
        /** X11: Holds the #X11WMType of the Window Manager we are running under. */
        X11WMType  m_enmWindowManagerType;
        /** X11: Holds whether the Window Manager we are running at is composition one. */
        bool       m_fCompositingManagerRunning;
#endif
    /** @} */

    /** @name Process arguments stuff.
     * @{ */
        /** Holds the URL arguments list. */
        QList<QUrl>  m_listArgUrls;

        /** Holds the --startvm option value (managed VM id). */
        QUuid  m_strManagedVMId;
        /** Holds the --separate option value (whether GUI process is separate from VM process). */
        bool   m_fSeparateProcess;
        /** Holds the --no-startvm-errormsgbox option value (whether startup VM errors are disabled). */
        bool   m_fShowStartVMErrors;

        /** Holds the --aggressive-caching / --no-aggressive-caching option value (whether medium-enumeration is required). */
        bool  m_fAgressiveCaching;

        /** Holds the --restore-current option value. */
        bool  m_fRestoreCurrentSnapshot;

        /** Holds the --fda option value (floppy image). */
        QUuid  m_uFloppyImage;
        /** Holds the --dvd | --cdrom option value (DVD image). */
        QUuid  m_uDvdImage;

        /** Holds the --execute-all-in-iem option value. */
        bool      m_fExecuteAllInIem;
        /** Holds the --warp-factor option value. */
        uint32_t  m_uWarpPct;

#ifdef VBOX_WITH_DEBUGGER_GUI
        /** Holds whether the debugger should be accessible. */
        mutable int  m_fDbgEnabled;
        /** Holds whether to show the debugger automatically with the console. */
        mutable int  m_fDbgAutoShow;
        /** Holds whether to show the command line window when m_fDbgAutoShow is set. */
        mutable int  m_fDbgAutoShowCommandLine;
        /** Holds whether to show the statistics window when m_fDbgAutoShow is set. */
        mutable int  m_fDbgAutoShowStatistics;
        /** Pattern of statistics to expand when opening the viewer. */
        QString      m_strDbgStatisticsExpand;
        /** The statistics viewer filter. */
        QString      m_strDbgStatisticsFilter;

        /** VBoxDbg module handle. */
        RTLDRMOD  m_hVBoxDbg;

        /** Holds whether --start-running, --start-paused or nothing was given. */
        LaunchRunning  m_enmLaunchRunning;
#endif

        /** Holds the --settingspw option value or the content of --settingspwfile. */
        char  m_astrSettingsPw[256];
        /** Holds the --settingspwfile option value. */
        bool  m_fSettingsPwSet;

#ifdef VBOX_GUI_WITH_PIDFILE
        /** Holds the --pidfile option value (application PID file path). */
        QString m_strPidFile;
#endif
    /** @} */

    /** @name COM stuff.
     * @{ */
        /** Holds the COM cleanup protection token. */
        QReadWriteLock  m_comCleanupProtectionToken;

        /** Holds the instance of VirtualBox client wrapper. */
        CVirtualBoxClient  m_comVBoxClient;
        /** Holds the copy of VirtualBox object wrapper. */
        CVirtualBox        m_comVBox;
        /** Holds the copy of VirtualBox host-object wrapper. */
        CHost              m_comHost;
        /** Holds the symbolic VirtualBox home-folder representation. */
        QString            m_strHomeFolder;

        /** Holds whether acquired COM wrappers are currently valid. */
        bool  m_fWrappersValid;
        /** Holds whether VBoxSVC is currently available. */
        bool  m_fVBoxSVCAvailable;

        /** Holds the guest OS family IDs. */
        QList<QString>               m_guestOSFamilyIDs;
        /** Holds the guest OS family descriptions. */
        QMap<QString, QString>       m_guestOSFamilyDescriptions;
        /** Holds the guest OS types for each family ID. */
        QList<QList<CGuestOSType> >  m_guestOSTypes;
    /** @} */

    /** @name Thread stuff.
     * @{ */
        /** Holds the thread-pool instance. */
        UIThreadPool *m_pThreadPool;
        /** Holds the thread-pool instance for cloud needs. */
        UIThreadPool *m_pThreadPoolCloud;
    /** @} */

    /** @name Media related stuff.
     * @{ */
        /** Holds the medium enumerator cleanup protection token. */
        mutable QReadWriteLock  m_meCleanupProtectionToken;

        /** Holds the medium enumerator. */
        UIMediumEnumerator *m_pMediumEnumerator;
        /** List of medium names that should not appears in the recently used media extra data. */
        QStringList         m_recentMediaExcludeList;
    /** @} */

#ifdef VBOX_WS_WIN
    /** @name ATL stuff.
     * @{ */
        /** Holds the ATL module instance (for use with UICommon shared library only).
          * @note  Required internally by ATL (constructor records instance in global variable). */
        ATL::CComModule  _Module;
    /** @} */
#endif
    /** @name Font scaling related variables.
     * @{ */
       int iOriginalFontPixelSize;
       int iOriginalFontPointSize;
    /** @} */

    /** Allows for shortcut access. */
    friend UICommon &uiCommon();
};

/** Singleton UICommon 'official' name. */
inline UICommon &uiCommon() { return *UICommon::instance(); }

#endif /* !FEQT_INCLUDED_SRC_globals_UICommon_h */
