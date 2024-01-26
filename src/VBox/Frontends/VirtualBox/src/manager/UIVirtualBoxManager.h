/* $Id: UIVirtualBoxManager.h $ */
/** @file
 * VBox Qt GUI - UIVirtualBoxManager class declaration.
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

#ifndef FEQT_INCLUDED_SRC_manager_UIVirtualBoxManager_h
#define FEQT_INCLUDED_SRC_manager_UIVirtualBoxManager_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QMainWindow>
#include <QUrl>

/* GUI includes: */
#include "QIWithRestorableGeometry.h"
#include "QIWithRetranslateUI.h"
#include "UICloudMachineSettingsDialog.h"
#include "UICommon.h"
#include "UIExtraDataDefs.h"
#include "UISettingsDialog.h"

/* Forward declarations: */
class QMenu;
class QIManagerDialog;
class UIAction;
class UIActionPool;
struct UIUnattendedInstallData;
class UIVirtualBoxManagerWidget;
class UIVirtualMachineItem;
class CCloudMachine;
class CUnattended;

/* Type definitions: */
typedef QIWithRestorableGeometry<QMainWindow> QMainWindowWithRestorableGeometry;
typedef QIWithRetranslateUI<QMainWindowWithRestorableGeometry> QMainWindowWithRestorableGeometryAndRetranslateUi;

/** Singleton QMainWindow extension used as VirtualBox Manager instance. */
class UIVirtualBoxManager : public QMainWindowWithRestorableGeometryAndRetranslateUi
{
    Q_OBJECT;

    /** Pointer to menu update-handler for this class: */
    typedef void (UIVirtualBoxManager::*MenuUpdateHandler)(QMenu *pMenu);

signals:

    /** Notifies listeners about this window remapped to another screen. */
    void sigWindowRemapped();

public:

    /** Singleton constructor. */
    static void create();
    /** Singleton destructor. */
    static void destroy();
    /** Singleton instance provider. */
    static UIVirtualBoxManager *instance() { return s_pInstance; }

    /** Returns the action-pool instance. */
    UIActionPool *actionPool() const { return m_pActionPool; }

    /** Opens Cloud Profile Manager. */
    void openCloudProfileManager() { sltOpenManagerWindow(UIToolType_Cloud); }

protected:

    /** Constructs VirtualBox Manager. */
    UIVirtualBoxManager();
    /** Destructs VirtualBox Manager. */
    virtual ~UIVirtualBoxManager() RT_OVERRIDE;

    /** Returns whether the window should be maximized when geometry being restored. */
    virtual bool shouldBeMaximized() const RT_OVERRIDE;

    /** @name Event handling stuff.
      * @{ */
#ifdef VBOX_WS_MAC
        /** Mac OS X: Preprocesses any @a pEvent for passed @a pObject. */
        virtual bool eventFilter(QObject *pObject, QEvent *pEvent) RT_OVERRIDE;
#endif

        /** Handles translation event. */
        virtual void retranslateUi() RT_OVERRIDE;

        /** Handles any Qt @a pEvent. */
        virtual bool event(QEvent *pEvent) RT_OVERRIDE;
        /** Handles show @a pEvent. */
        virtual void showEvent(QShowEvent *pEvent) RT_OVERRIDE;
        /** Handles first show @a pEvent. */
        virtual void polishEvent(QShowEvent *pEvent);
        /** Handles close @a pEvent. */
        virtual void closeEvent(QCloseEvent *pEvent) RT_OVERRIDE;
        /** Handles drag enter @a pEvent. */
        virtual void dragEnterEvent(QDragEnterEvent *event) RT_OVERRIDE;
        /** Handles drop @a pEvent. */
        virtual void dropEvent(QDropEvent *event) RT_OVERRIDE;
    /** @} */

private slots:

    /** @name Common stuff.
      * @{ */
#ifdef VBOX_WS_X11
        /** Handles host-screen available-area change. */
        void sltHandleHostScreenAvailableAreaChange();
#endif

        /** Handles request to update actions. */
        void sltHandleUpdateActionAppearanceRequest() { updateActionsAppearance(); }

        /** Handles request to commit data. */
        void sltHandleCommitData();

        /** Handles signal about medium-enumeration finished. */
        void sltHandleMediumEnumerationFinish();

        /** Handles call to open a @a list of URLs. */
        void sltHandleOpenUrlCall(QList<QUrl> list = QList<QUrl>());

        /** Checks if USB device list can be enumerated and host produces any warning during enumeration. */
        void sltCheckUSBAccesibility();

        /** Hnadles singal about Chooser-pane index change.  */
        void sltHandleChooserPaneIndexChange();
        /** Handles signal about group saving progress change. */
        void sltHandleGroupSavingProgressChange();
        /** Handles signal about cloud update progress change. */
        void sltHandleCloudUpdateProgressChange();

        /** Handles singal about Tool type change.  */
        void sltHandleToolTypeChange();

        /** Handles current snapshot item change. */
        void sltCurrentSnapshotItemChange();

        /** Handles state change for cloud machine with certain @a uId. */
        void sltHandleCloudMachineStateChange(const QUuid &uId);
    /** @} */

    /** @name CVirtualBox event handling stuff.
      * @{ */
        /** Handles CVirtualBox event about state change for machine with @a uID. */
        void sltHandleStateChange(const QUuid &uID);
    /** @} */

    /** @name Action-pool stuff.
      * @{ */
        /** Handle menu prepare. */
        void sltHandleMenuPrepare(int iIndex, QMenu *pMenu);
    /** @} */

    /** @name File menu stuff.
      * @{ */
        /** Handles call to open Manager window of certain @a enmType. */
        void sltOpenManagerWindow(UIToolType enmType = UIToolType_Invalid);
        /** Handles call to open Manager window by default. */
        void sltOpenManagerWindowDefault() { sltOpenManagerWindow(); }
        /** Handles call to close Manager window of certain @a enmType. */
        void sltCloseManagerWindow(UIToolType enmType = UIToolType_Invalid);
        /** Handles call to close Manager window by default. */
        void sltCloseManagerWindowDefault() { sltCloseManagerWindow(); }

        /** Handles call to open Import Appliance wizard.
          * @param strFileName can bring the name of file to import appliance from. */
        void sltOpenImportApplianceWizard(const QString &strFileName = QString());
        /** Handles call to open Import Appliance wizard the default way. */
        void sltOpenImportApplianceWizardDefault() { sltOpenImportApplianceWizard(); }
        /** Handles call to open Export Appliance wizard. */
        void sltOpenExportApplianceWizard();

#ifdef VBOX_GUI_WITH_EXTRADATA_MANAGER_UI
        /** Handles call to open Extra-data Manager window. */
        void sltOpenExtraDataManagerWindow();
#endif

        /** Handles call to open Preferences dialog. */
        void sltOpenPreferencesDialog();
        /** Handles call to close Preferences dialog. */
        void sltClosePreferencesDialog();

        /** Handles call to exit application. */
        void sltPerformExit();
    /** @} */

    /** @name Machine menu stuff.
      * @{ */
        /** Handles call to open new machine wizard. */
        void sltOpenNewMachineWizard();
        /** Handles call to open add machine dialog. */
        void sltOpenAddMachineDialog();

        /** Handles call to open group name editor. */
        void sltOpenGroupNameEditor();
        /** Handles call to disband group. */
        void sltDisbandGroup();

        /** Handles call to open Settings dialog.
          * @param strCategory can bring the settings category to start from.
          * @param strControl  can bring the widget of the page to focus.
          * @param uID       can bring the ID of machine to manage. */
        void sltOpenSettingsDialog(QString strCategory = QString(),
                                   QString strControl = QString(),
                                   const QUuid &uID = QUuid());
        /** Handles call to open Settings dialog the default way. */
        void sltOpenSettingsDialogDefault() { sltOpenSettingsDialog(); }
        /** Handles call to close Settings dialog. */
        void sltCloseSettingsDialog();

        /** Handles call to open Clone Machine wizard. */
        void sltOpenCloneMachineWizard();

        /** Handles call to move machine. */
        void sltPerformMachineMove();

        /** Handles call to remove machine. */
        void sltPerformMachineRemove();

        /** Handles call to move machine to a new group. */
        void sltPerformMachineMoveToNewGroup();
        /** Handles call to move machine to a specific group. */
        void sltPerformMachineMoveToSpecificGroup();

        /** Handles call to start or show machine. */
        void sltPerformStartOrShowMachine();
        /** Handles call to start machine in normal mode. */
        void sltPerformStartMachineNormal();
        /** Handles call to start machine in headless mode. */
        void sltPerformStartMachineHeadless();
        /** Handles call to start machine in detachable mode. */
        void sltPerformStartMachineDetachable();

        /** Handles call to create console connection for group. */
        void sltPerformCreateConsoleConnectionForGroup();
        /** Handles call to create console connection for machine. */
        void sltPerformCreateConsoleConnectionForMachine();
        /** Handles call to delete console connection for group. */
        void sltPerformDeleteConsoleConnectionForGroup();
        /** Handles call to delete console connection for machine. */
        void sltPerformDeleteConsoleConnectionForMachine();
        /** Handles call to copy console connection key fingerprint. */
        void sltCopyConsoleConnectionFingerprint();
        /** Handles call to copy serial console command for Unix. */
        void sltPerformCopyCommandSerialUnix();
        /** Handles call to copy serial console command for Windows. */
        void sltPerformCopyCommandSerialWindows();
        /** Handles call to copy VNC console command for Unix. */
        void sltPerformCopyCommandVNCUnix();
        /** Handles call to copy VNC console command for Windows. */
        void sltPerformCopyCommandVNCWindows();
        /** Handles call to show console log. */
        void sltPerformShowLog();
        /** Handles call about console @a strLog for cloud VM with @a strName read. */
        void sltHandleConsoleLogRead(const QString &strName, const QString &strLog);
        /** Handles call to execute external application. */
        void sltExecuteExternalApplication();

        /** Handles call to discard machine state. */
        void sltPerformDiscardMachineState();

        /** Handles call to @a fPause or resume machine otherwise. */
        void sltPerformPauseOrResumeMachine(bool fPause);

        /** Handles call to reset machine. */
        void sltPerformResetMachine();

        /** Handles call to detach machine UI. */
        void sltPerformDetachMachineUI();
        /** Handles call to save machine state. */
        void sltPerformSaveMachineState();
        /** Handles call to terminate machine. */
        void sltPerformTerminateMachine();
        /** Handles call to ask machine for shutdown. */
        void sltPerformShutdownMachine();
        /** Handles call to power machine off. */
        void sltPerformPowerOffMachine();
        /** Handles signal about machine powered off.
          * @param  fSuccess           Brings whether machine was powered off successfully.
          * @param  fIncludingDiscard  Brings whether machine state should be discarded. */
        void sltHandlePoweredOffMachine(bool fSuccess, bool fIncludingDiscard);

        /** Handles call to show global tool corresponding to passed @a pAction. */
        void sltPerformShowGlobalTool(QAction *pAction);
        /** Handles call to show machine tool corresponding to passed @a pAction. */
        void sltPerformShowMachineTool(QAction *pAction);

        /** Handles call to open machine Log Viewer window. */
        void sltOpenLogViewerWindow();
        /** Handles call to close machine Log Viewer window. */
        void sltCloseLogViewerWindow();

        /** Handles call to refresh machine. */
        void sltPerformRefreshMachine();

        /** Handles call to show machine in File Manager. */
        void sltShowMachineInFileManager();

        /** Handles call to create machine shortcut. */
        void sltPerformCreateMachineShortcut();

        /** Handles call to sort group. */
        void sltPerformGroupSorting();

        /** Handles call to toggle machine search widget visibility to be @a fVisible. */
        void sltPerformMachineSearchWidgetVisibilityToggling(bool fVisible);

        /** Handles call to show help viewer. */
        void sltPerformShowHelpBrowser();
    /** @} */

private:

    /** @name Prepare/Cleanup cascade.
      * @{ */
        /** Prepares window. */
        void prepare();
        /** Prepares icon. */
        void prepareIcon();
        /** Prepares menu-bar. */
        void prepareMenuBar();
        /** Prepares status-bar. */
        void prepareStatusBar();
        /** Prepares toolbar. */
        void prepareToolbar();
        /** Prepares widgets. */
        void prepareWidgets();
        /** Prepares connections. */
        void prepareConnections();
        /** Loads settings. */
        void loadSettings();

        /** Cleanups connections. */
        void cleanupConnections();
        /** Cleanups widgets. */
        void cleanupWidgets();
        /** Cleanups menu-bar. */
        void cleanupMenuBar();
        /** Cleanups window. */
        void cleanup();
    /** @} */

    /** @name Common stuff.
      * @{ */
        /** Returns current-item. */
        UIVirtualMachineItem *currentItem() const;
        /** Returns a list of current-items. */
        QList<UIVirtualMachineItem*> currentItems() const;

        /** Returns whether group saving is in progress. */
        bool isGroupSavingInProgress() const;
        /** Returns whether all items of one group is selected. */
        bool isAllItemsOfOneGroupSelected() const;
        /** Returns whether single group is selected. */
        bool isSingleGroupSelected() const;
        /** Returns whether single local group is selected. */
        bool isSingleLocalGroupSelected() const;
        /** Returns whether single cloud provider group is selected. */
        bool isSingleCloudProviderGroupSelected() const;
        /** Returns whether single cloud profile group is selected. */
        bool isSingleCloudProfileGroupSelected() const;

        /** Returns whether at least one cloud profile currently being updated. */
        bool isCloudProfileUpdateInProgress() const;

        /** Checks if @a comUnattended has any errors.
          * If so shows an error notification and returns false, else returns true. */
        bool checkUnattendedInstallError(const CUnattended &comUnattended) const;
    /** @} */

    /** @name Various VM helpers.
      * @{ */
        /** Opens add machine dialog specifying initial name with @a strFileName. */
        void openAddMachineDialog(const QString &strFileName = QString());
        /** Opens new machine dialog specifying initial name with @a strFileName. */
        void openNewMachineWizard(const QString &strISOFilePath = QString());
        /** Launches certain @a comMachine in specified @a enmLaunchMode. */
        static void launchMachine(CMachine &comMachine, UILaunchMode enmLaunchMode = UILaunchMode_Default);
        /** Launches certain @a comMachine. */
        static void launchMachine(CCloudMachine &comMachine);

        /** Creates an unattended installer and uses it to install guest os to newly created vm. */
        void startUnattendedInstall(CUnattended &comUnattendedInstaller, bool fStartHeadless, const QString &strMachineId);

        /** Launches or shows virtual machines represented by passed @a items in corresponding @a enmLaunchMode (for launch). */
        void performStartOrShowVirtualMachines(const QList<UIVirtualMachineItem*> &items, UILaunchMode enmLaunchMode);

#ifndef VBOX_WS_WIN
        /** Parses serialized @a strArguments string according to shell rules. */
        QStringList parseShellArguments(const QString &strArguments);
#endif
    /** @} */

    /** @name Action update stuff.
      * @{ */
        /** Updates 'Group' menu. */
        void updateMenuGroup(QMenu *pMenu);
        /** Updates 'Machine' menu. */
        void updateMenuMachine(QMenu *pMenu);
        /** Updates 'Group' : 'Move to Group' menu. */
        void updateMenuGroupMoveToGroup(QMenu *pMenu);
        /** Updates 'Group' : 'Console' menu. */
        void updateMenuGroupConsole(QMenu *pMenu);
        /** Updates 'Group' : 'Close' menu. */
        void updateMenuGroupClose(QMenu *pMenu);
        /** Updates 'Machine' : 'Move to Group' menu. */
        void updateMenuMachineMoveToGroup(QMenu *pMenu);
        /** Updates 'Machine' : 'Console' menu. */
        void updateMenuMachineConsole(QMenu *pMenu);
        /** Updates 'Machine' : 'Close' menu. */
        void updateMenuMachineClose(QMenu *pMenu);

        /** Performs update of actions visibility. */
        void updateActionsVisibility();
        /** Performs update of actions appearance. */
        void updateActionsAppearance();

        /** Returns whether the action with @a iActionIndex is enabled.
          * @param items used to calculate verdict about should the action be enabled. */
        bool isActionEnabled(int iActionIndex, const QList<UIVirtualMachineItem*> &items);

        /** Returns whether all passed @a items are local. */
        static bool isItemsLocal(const QList<UIVirtualMachineItem*> &items);
        /** Returns whether all passed @a items are cloud. */
        static bool isItemsCloud(const QList<UIVirtualMachineItem*> &items);
        /** Returns whether all passed @a items are powered off. */
        static bool isItemsPoweredOff(const QList<UIVirtualMachineItem*> &items);
        /** Returns whether at least one of passed @a items is able to shutdown. */
        static bool isAtLeastOneItemAbleToShutdown(const QList<UIVirtualMachineItem*> &items);
        /** Returns whether at least one of passed @a items supports shortcut creation. */
        static bool isAtLeastOneItemSupportsShortcuts(const QList<UIVirtualMachineItem*> &items);
        /** Returns whether at least one of passed @a items is accessible. */
        static bool isAtLeastOneItemAccessible(const QList<UIVirtualMachineItem*> &items);
        /** Returns whether at least one of passed @a items is inaccessible. */
        static bool isAtLeastOneItemInaccessible(const QList<UIVirtualMachineItem*> &items);
        /** Returns whether at least one of passed @a items is removable. */
        static bool isAtLeastOneItemRemovable(const QList<UIVirtualMachineItem*> &items);
        /** Returns whether at least one of passed @a items can be started. */
        static bool isAtLeastOneItemCanBeStarted(const QList<UIVirtualMachineItem*> &items);
        /** Returns whether at least one of passed @a items can be shown. */
        static bool isAtLeastOneItemCanBeShown(const QList<UIVirtualMachineItem*> &items);
        /** Returns whether at least one of passed @a items can be started or shown. */
        static bool isAtLeastOneItemCanBeStartedOrShown(const QList<UIVirtualMachineItem*> &items);
        /** Returns whether at least one of passed @a items can be discarded. */
        static bool isAtLeastOneItemDiscardable(const QList<UIVirtualMachineItem*> &items);
        /** Returns whether at least one of passed @a items is started. */
        static bool isAtLeastOneItemStarted(const QList<UIVirtualMachineItem*> &items);
        /** Returns whether at least one of passed @a items is running. */
        static bool isAtLeastOneItemRunning(const QList<UIVirtualMachineItem*> &items);
        /** Returns whether at least one of passed @a items is detachable. */
        static bool isAtLeastOneItemDetachable(const QList<UIVirtualMachineItem*> &items);

#ifdef VBOX_WS_X11
        /** Tries to guess default X11 terminal emulator.
          * @returns Data packed into Qt pair of QString(s),
          *          which is `name` and `--execute argument`. */
        static QPair<QString, QString> defaultTerminalData();
#endif
    /** @} */

    /** Holds the static instance. */
    static UIVirtualBoxManager *s_pInstance;

    /** Holds whether the dialog is polished. */
    bool  m_fPolished                      : 1;
    /** Holds whether first medium-enumeration handled. */
    bool  m_fFirstMediumEnumerationHandled : 1;

    /** Holds the action-pool instance. */
    UIActionPool *m_pActionPool;
    /** Holds the map of menu update-handlers. */
    QMap<int, MenuUpdateHandler> m_menuUpdateHandlers;

    /** Holds the map of various global managers. */
    QMap<UIToolType, QIManagerDialog*>  m_managers;

    /** Holds the map of various settings dialogs. */
    QMap<UISettingsDialog::DialogType, UISettingsDialog*>  m_settings;
    /** Holds the cloud settings dialog instance. */
    UISafePointerCloudMachineSettingsDialog                m_pCloudSettings;

    /** Holds the instance of UIVMLogViewerDialog. */
    QIManagerDialog *m_pLogViewerDialog;

    /** Holds the central-widget instance. */
    UIVirtualBoxManagerWidget *m_pWidget;

    /** Holds the geometry save timer ID. */
    int  m_iGeometrySaveTimerId;
};

#define gpManager UIVirtualBoxManager::instance()

#endif /* !FEQT_INCLUDED_SRC_manager_UIVirtualBoxManager_h */
