/* $Id: UIVisoCreator.h $ */
/** @file
 * VBox Qt GUI - UIVisoCreator classes declaration.
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

#ifndef FEQT_INCLUDED_SRC_medium_viso_UIVisoCreator_h
#define FEQT_INCLUDED_SRC_medium_viso_UIVisoCreator_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QModelIndex>

/* GUI includes: */
#include "QIMainDialog.h"
#include "QIWithRestorableGeometry.h"
#include "QIWithRetranslateUI.h"

#include <iprt/stream.h>

/* Forward declarations: */
class QMenu;
class QGridLayout;
class QIDialogButtonBox;
class UIDialogPanel;
class QIToolBar;
class UIActionPool;
class UIVisoHostBrowser;
class UIVisoContentBrowser;
class UIVisoCreatorOptionsPanel;
class UIVisoConfigurationPanel;

/** A QIMainDialog extension. It hosts two UIVisoBrowserBase extensions, one for host and one
  * for VISO file system. It has the main menu, main toolbar, and a vertical toolbar and corresponding
  * actions. */
class SHARED_LIBRARY_STUFF UIVisoCreatorWidget : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

signals:

    void sigSetCancelButtonShortCut(QKeySequence keySequence);
    void sigVisoNameChanged(const QString &strVisoName);

public:

    UIVisoCreatorWidget(UIActionPool *pActionPool, QWidget *pParent,
                        bool fShowToolBar, const QString& strMachineName = QString());
    /** Returns the content of the .viso file. Each element of the list corresponds to a line in the .viso file. */
    QStringList       entryList() const;
    const QString     &visoName() const;
    /** Returns custom ISO options (if any). */
    const QStringList &customOptions() const;
    /** Returns the current path that the host browser is listing. */
    QString currentPath() const;
    void    setCurrentPath(const QString &strPath);
    QMenu *menu() const;

    /** Creates a VISO by using the VISO creator dialog.
      * @param  pParent           Passes the dialog parent.
      * @param  strDefaultFolder  Passes the folder to save the VISO file.
      * @param  strMachineName    Passes the name of the machine,
      * returns the UUID of the created medium or a null QUuid. */
    static QUuid createViso(UIActionPool *pActionPool, QWidget *pParent,
                            const QString &strDefaultFolder = QString(),
                            const QString &strMachineName  = QString());

#ifdef VBOX_WS_MAC
    /** Returns the toolbar. */
    QIToolBar *toolbar() const { return m_pToolBar; }
#endif

    /**
      * Helper for createVisoMediumWithVisoCreator.
      * @returns IPRT status code.
      * @param   pStrmDst            Where to write the quoted string.
      * @param   pszPrefix           Stuff to put in front of it.
      * @param   rStr                The string to quote and write out.
      * @param   pszPrefix           Stuff to put after it.
      */
    static int visoWriteQuotedString(PRTSTREAM pStrmDst, const char *pszPrefix,
                                     QString const &rStr, const char *pszPostFix);

protected:

    virtual void retranslateUi() final override;

private slots:

    void sltHandleAddObjectsToViso(QStringList pathList);
    void sltPanelActionToggled(bool fChecked);
    void sltHandleVisoNameChanged(const QString& strVisoName);
    void sltHandleCustomVisoOptionsChanged(const QStringList &customVisoOptions);
    void sltHandleShowHiddenObjectsChange(bool fShow);
    void sltHandleHidePanel(UIDialogPanel *pPanel);
    void sltHandleBrowserTreeViewVisibilityChanged(bool fVisible);
    void sltHandleHostBrowserTableSelectionChanged(bool fIsSelectionEmpty);
    void sltHandleContentBrowserTableSelectionChanged(bool fIsSelectionEmpty);
    void sltHandleShowContextMenu(const QWidget *pContextMenuRequester, const QPoint &point);

private:

    struct VisoOptions
    {
        VisoOptions()
            :m_strVisoName("ad-hoc-viso"){}
        QString m_strVisoName;
        /** Additions viso options to be inserted to the viso file as separate lines. */
        QStringList m_customOptions;
    };

    struct BrowserOptions
    {
        BrowserOptions()
            :m_fShowHiddenObjects(true){}
        bool m_fShowHiddenObjects;
    };

    void prepareWidgets();
    void prepareConnections();
    void prepareActions();
    /** Creates and configures the vertical toolbar. Should be called after prepareActions() */
    void prepareVerticalToolBar();
    /* Populates the main menu and toolbard with already created actions.
     * Leave out the vertical toolbar which is handled in prepareVerticalToolBar. */
    void populateMenuMainToolbar();
    /** Set the root index of the m_pTableModel to the current index of m_pTreeModel. */
    void setTableRootIndex(QModelIndex index = QModelIndex() );
    void setTreeCurrentIndex(QModelIndex index = QModelIndex() );
    void hidePanel(UIDialogPanel *panel);
    void showPanel(UIDialogPanel *panel);
    /** Makes sure escape key is assigned to only a single widget. This is done by checking
      *  several things in the following order:
      *  - when (drop-down) tree views of browser panes are visible esc. key used to close those. thus it is taken from the dialog and panels
      *  - when there are no more panels visible assign it to the parent dialog
      *  - grab it from the dialog as soon as a panel becomes visible again
      *  - assign it to the most recently "unhidden" panel */
    void manageEscapeShortCut();

    /** @name Main toolbar (and main menu) actions
      * @{ */
        QAction         *m_pActionConfiguration;
        QAction         *m_pActionOptions;
    /** @} */

    /** @name These actions are addded to vertical toolbar, context menus, and the main menu.
      * @{ */
        QAction              *m_pAddAction;
        QAction              *m_pRemoveAction;
        QAction              *m_pCreateNewDirectoryAction;
        QAction              *m_pRenameAction;
        QAction              *m_pResetAction;
    /** @} */

    QGridLayout          *m_pMainLayout;
    UIVisoHostBrowser    *m_pHostBrowser;
    UIVisoContentBrowser *m_pVISOContentBrowser;

    QIToolBar            *m_pToolBar;
    QIToolBar            *m_pVerticalToolBar;
    VisoOptions           m_visoOptions;
    BrowserOptions        m_browserOptions;
    QMenu                *m_pMainMenu;
    QString               m_strMachineName;
    UIVisoCreatorOptionsPanel *m_pCreatorOptionsPanel;
    UIVisoConfigurationPanel  *m_pConfigurationPanel;
    QMap<UIDialogPanel*, QAction*> m_panelActionMap;
    QList<UIDialogPanel*>          m_visiblePanelsList;
    QPointer<UIActionPool> m_pActionPool;
    bool                   m_fShowToolBar;
};


class SHARED_LIBRARY_STUFF UIVisoCreatorDialog : public QIWithRetranslateUI<QIWithRestorableGeometry<QIMainDialog> >
{
    Q_OBJECT;

public:

    UIVisoCreatorDialog(UIActionPool *pActionPool, QWidget *pParent, const QString& strMachineName = QString());

    QStringList  entryList() const;
    QString visoName() const;
    QStringList customOptions() const;
    QString currentPath() const;
    void    setCurrentPath(const QString &strPath);

protected:

    virtual bool event(QEvent *pEvent) final override;

private slots:

    void sltSetCancelButtonShortCut(QKeySequence keySequence);
    void sltsigVisoNameChanged(const QString &strName);

private:
    void prepareWidgets();
    void prepareConnections();
    virtual void retranslateUi() final override;
    void loadSettings();
    void saveDialogGeometry();
    void updateWindowTitle();

    QString m_strMachineName;
    UIVisoCreatorWidget *m_pVisoCreatorWidget;
    QIDialogButtonBox    *m_pButtonBox;
    QPointer<UIActionPool> m_pActionPool;
    int                   m_iGeometrySaveTimerId;
};
#endif /* !FEQT_INCLUDED_SRC_medium_viso_UIVisoCreator_h */
