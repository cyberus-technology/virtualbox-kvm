/* $Id: UIFileManager.h $ */
/** @file
 * VBox Qt GUI - UIFileManager class declaration.
 */

/*
 * Copyright (C) 2016-2023 Oracle and/or its affiliates.
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

#ifndef FEQT_INCLUDED_SRC_guestctrl_UIFileManager_h
#define FEQT_INCLUDED_SRC_guestctrl_UIFileManager_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QPointer>
#include <QWidget>
#include <QString>
#include <QUuid>

/* GUI includes: */
#include "QIManagerDialog.h"
#include "QIWithRetranslateUI.h"
#include "UIGuestControlDefs.h"


/* Forward declarations: */
class CMachine;
class CProgress;
class QHBoxLayout;
class QSplitter;
class QTextEdit;
class QVBoxLayout;
class UIActionPool;
class UIDialogPanel;
class UIFileManagerLogPanel;
class UIFileManagerOperationsPanel;
class UIFileManagerOptionsPanel;
class UIFileManagerGuestTable;
class UIFileManagerHostTable;
class UIVirtualMachineItem;
class QITabWidget;
class QIToolBar;

/** A Utility class to manage file  manager options. */
class UIFileManagerOptions
{

public:

    static UIFileManagerOptions* instance();
    static void create();
    static void destroy();

    bool fListDirectoriesOnTop;
    bool fAskDeleteConfirmation;
    bool fShowHumanReadableSizes;
    bool fShowHiddenObjects;

private:

    UIFileManagerOptions();
    ~UIFileManagerOptions();

    static UIFileManagerOptions *m_pInstance;
};

/** A QWidget extension. it includes a QWidget extension for initiating a guest session
 *  one host and one guest file table views, a log viewer
 *  and some other file manager related widgets. */
class SHARED_LIBRARY_STUFF UIFileManager : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

signals:

    void sigSetCloseButtonShortCut(QKeySequence);

public:

    UIFileManager(EmbedTo enmEmbedding, UIActionPool *pActionPool,
                  const CMachine &comMachine, QWidget *pParent, bool fShowToolbar);
    ~UIFileManager();
    QMenu *menu() const;

#ifdef VBOX_WS_MAC
    /** Returns the toolbar. */
    QIToolBar *toolbar() const { return m_pToolBar; }
#endif

    void setSelectedVMListItems(const QList<UIVirtualMachineItem*> &items);

protected:

    void retranslateUi();

private slots:

    void sltReceieveLogOutput(QString strOutput, const QString &strMachineName, FileManagerLogType eLogType);
    void sltCopyGuestToHost();
    void sltCopyHostToGuest();
    void sltPanelActionToggled(bool fChecked);
    void sltReceieveNewFileOperation(const CProgress &comProgress, const QString &strTableName);
    void sltFileOperationComplete(QUuid progressId);
    /** Performs whatever necessary when some signal about option change has been receieved. */
    void sltHandleOptionsUpdated();
    void sltHandleHidePanel(UIDialogPanel *pPanel);
    void sltHandleShowPanel(UIDialogPanel *pPanel);
    void sltCommitDataSignalReceived();
    void sltFileTableSelectionChanged(bool fHasSelection);
    void sltCurrentTabChanged(int iIndex);
    void sltGuestFileTableStateChanged(bool fIsRunning);

private:

    void prepareObjects();
    void prepareConnections();
    void prepareVerticalToolBar(QHBoxLayout *layout);
    void prepareToolBar();
    /** Creates options and sessions panels and adds them to @p pLayout.  */
    void prepareOptionsAndSessionPanels(QVBoxLayout *pLayout);
    void prepareOperationsAndLogPanels(QSplitter *pSplitter);

    /** Saves list of panels and file manager options to the extra data. */
    void saveOptions();
    /** Show the panels that have been visible the last time file manager is closed. */
    void restorePanelVisibility();
    /** Loads file manager options. This should be done before widget creation
     *  since some widgets are initilized with these options */
    void loadOptions();
    void hidePanel(UIDialogPanel *panel);
    void showPanel(UIDialogPanel *panel);
    /** Makes sure escape key is assigned to only a single widget. This is done by checking
        several things in the following order:
        - when there are no more panels visible assign it to the parent dialog
        - grab it from the dialog as soon as a panel becomes visible again
        - assign it to the most recently "unhidden" panel */
    void manageEscapeShortCut();
    void copyToGuest();
    void copyToHost();
    template<typename T>
    QStringList               getFsObjInfoStringList(const T &fsObjectInfo) const;
    void                      appendLog(const QString &strLog, const QString &strMachineName, FileManagerLogType eLogType);
    void                      savePanelVisibility();

    void setMachines(const QVector<QUuid> &machineIDs, const QUuid &lastSelectedMachineId = QUuid());
    void removeTabs(const QVector<QUuid> &machineIdsToRemove);
    void addTabs(const QVector<QUuid> &machineIdsToAdd);
    void setVerticalToolBarActionsEnabled();
    UIFileManagerGuestTable *currentGuestTable();

    QVBoxLayout              *m_pMainLayout;
    QSplitter                *m_pVerticalSplitter;
    /** Splitter hosting host and guest file system tables. */
    QSplitter                *m_pFileTableSplitter;
    QIToolBar                *m_pToolBar;
    QIToolBar                *m_pVerticalToolBar;

    UIFileManagerHostTable   *m_pHostFileTable;

    QITabWidget              *m_pGuestTablesContainer;
    const EmbedTo  m_enmEmbedding;
    QPointer<UIActionPool>  m_pActionPool;
    const bool     m_fShowToolbar;
    QMap<UIDialogPanel*, QAction*> m_panelActionMap;
    QList<UIDialogPanel*>          m_visiblePanelsList;
    UIFileManagerOptionsPanel          *m_pOptionsPanel;
    UIFileManagerLogPanel              *m_pLogPanel;
    UIFileManagerOperationsPanel       *m_pOperationsPanel;

    bool m_fCommitDataSignalReceived;

    QVector<QUuid> m_machineIds;

    friend class UIFileManagerOptionsPanel;
    friend class UIFileManagerDialog;
};

#endif /* !FEQT_INCLUDED_SRC_guestctrl_UIFileManager_h */
