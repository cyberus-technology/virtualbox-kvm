/* $Id: UIVisoCreator.cpp $ */
/** @file
 * VBox Qt GUI - UIVisoCreator classes implementation.
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
#include <QGridLayout>
#include <QMenuBar>
#include <QPushButton>
#include <QStyle>

/* GUI includes: */
#include "UIActionPool.h"
#include "QIDialogButtonBox.h"
#include "UIDesktopWidgetWatchdog.h"
#include "UIExtraDataManager.h"
#include "UIIconPool.h"
#include "UIMessageCenter.h"
#include "UIModalWindowManager.h"
#include "QIToolBar.h"
#include "UIVisoHostBrowser.h"
#include "UIVisoCreator.h"
#include "UIVisoConfigurationPanel.h"
#include "UIVisoCreatorOptionsPanel.h"
#include "UIVisoContentBrowser.h"
#include "UICommon.h"
#ifdef VBOX_WS_MAC
# include "VBoxUtils-darwin.h"
#endif

/* Other VBox includes: */
#include <iprt/getopt.h>
#include <iprt/path.h>


/*********************************************************************************************************************************
*   UIVisoCreatorWidget implementation.                                                                                          *
*********************************************************************************************************************************/

UIVisoCreatorWidget::UIVisoCreatorWidget(UIActionPool *pActionPool, QWidget *pParent,
                                         bool fShowToolBar,const QString& strMachineName /* = QString() */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_pActionConfiguration(0)
    , m_pActionOptions(0)
    , m_pAddAction(0)
    , m_pRemoveAction(0)
    , m_pCreateNewDirectoryAction(0)
    , m_pRenameAction(0)
    , m_pResetAction(0)
    , m_pMainLayout(0)
    , m_pHostBrowser(0)
    , m_pVISOContentBrowser(0)
    , m_pToolBar(0)
    , m_pVerticalToolBar(0)
    , m_pMainMenu(0)
    , m_strMachineName(strMachineName)
    , m_pCreatorOptionsPanel(0)
    , m_pConfigurationPanel(0)
    , m_pActionPool(pActionPool)
    , m_fShowToolBar(fShowToolBar)
{
    m_visoOptions.m_strVisoName = !strMachineName.isEmpty() ? strMachineName : "ad-hoc";
    prepareWidgets();
    populateMenuMainToolbar();
    prepareConnections();
    manageEscapeShortCut();
    retranslateUi();
}

QStringList UIVisoCreatorWidget::entryList() const
{
    if (!m_pVISOContentBrowser)
        return QStringList();
    return m_pVISOContentBrowser->entryList();
}

const QString &UIVisoCreatorWidget::visoName() const
{
    return m_visoOptions.m_strVisoName;
}

const QStringList &UIVisoCreatorWidget::customOptions() const
{
    return m_visoOptions.m_customOptions;
}

QString UIVisoCreatorWidget::currentPath() const
{
    if (!m_pHostBrowser)
        return QString();
    return m_pHostBrowser->currentPath();
}

void UIVisoCreatorWidget::setCurrentPath(const QString &strPath)
{
    if (!m_pHostBrowser)
        return;
    m_pHostBrowser->setCurrentPath(strPath);
}

QMenu *UIVisoCreatorWidget::menu() const
{
    return m_pMainMenu;
}

void UIVisoCreatorWidget::retranslateUi()
{
    if (m_pHostBrowser)
        m_pHostBrowser->setTitle(tr("Host File System"));
    if (m_pVISOContentBrowser)
        m_pVISOContentBrowser->setTitle(tr("VISO Content"));
}

void UIVisoCreatorWidget::sltHandleAddObjectsToViso(QStringList pathList)
{
    if (m_pVISOContentBrowser)
        m_pVISOContentBrowser->addObjectsToViso(pathList);
}

void UIVisoCreatorWidget::sltPanelActionToggled(bool fChecked)
{
    QAction *pSenderAction = qobject_cast<QAction*>(sender());
    if (!pSenderAction)
        return;
    UIDialogPanel* pPanel = 0;
    /* Look for the sender() within the m_panelActionMap's values: */
    for (QMap<UIDialogPanel*, QAction*>::const_iterator iterator = m_panelActionMap.begin();
        iterator != m_panelActionMap.end(); ++iterator)
    {
        if (iterator.value() == pSenderAction)
            pPanel = iterator.key();
    }
    if (!pPanel)
        return;
    if (fChecked)
        showPanel(pPanel);
    else
        hidePanel(pPanel);
}

void UIVisoCreatorWidget::sltHandleVisoNameChanged(const QString &strVisoName)
{
    if (m_visoOptions.m_strVisoName == strVisoName)
        return;
    m_visoOptions.m_strVisoName = strVisoName;
    if(m_pVISOContentBrowser)
        m_pVISOContentBrowser->setVisoName(m_visoOptions.m_strVisoName);
    emit sigVisoNameChanged(strVisoName);
}

void UIVisoCreatorWidget::sltHandleCustomVisoOptionsChanged(const QStringList &customVisoOptions)
{
    if (m_visoOptions.m_customOptions == customVisoOptions)
        return;
    m_visoOptions.m_customOptions = customVisoOptions;
}

void UIVisoCreatorWidget::sltHandleShowHiddenObjectsChange(bool fShow)
{
    if (m_browserOptions.m_fShowHiddenObjects == fShow)
        return;
    m_browserOptions.m_fShowHiddenObjects = fShow;
    m_pHostBrowser->showHideHiddenObjects(fShow);
}

void UIVisoCreatorWidget::sltHandleHidePanel(UIDialogPanel *pPanel)
{
    hidePanel(pPanel);
}

void UIVisoCreatorWidget::sltHandleBrowserTreeViewVisibilityChanged(bool fVisible)
{
    Q_UNUSED(fVisible);
    manageEscapeShortCut();
}

void UIVisoCreatorWidget::sltHandleHostBrowserTableSelectionChanged(bool fIsSelectionEmpty)
{
    if (m_pAddAction)
        m_pAddAction->setEnabled(!fIsSelectionEmpty);
}

void UIVisoCreatorWidget::sltHandleContentBrowserTableSelectionChanged(bool fIsSelectionEmpty)
{
    if (m_pRemoveAction)
        m_pRemoveAction->setEnabled(!fIsSelectionEmpty);
}

void UIVisoCreatorWidget::sltHandleShowContextMenu(const QWidget *pContextMenuRequester, const QPoint &point)
{
    if (!pContextMenuRequester)
        return;

    QMenu menu;

    if (sender() == m_pHostBrowser)
    {
        menu.addAction(m_pAddAction);
    }
    else if (sender() == m_pVISOContentBrowser)
    {
        menu.addAction(m_pRemoveAction);
        menu.addAction(m_pCreateNewDirectoryAction);
        menu.addAction(m_pResetAction);
    }

    menu.exec(pContextMenuRequester->mapToGlobal(point));
}

void UIVisoCreatorWidget::prepareWidgets()
{
    m_pMainLayout = new QGridLayout(this);
    if (!m_pMainLayout)
        return;

    /* Configure layout: */
    const int iL = qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin) / 2;
    const int iT = qApp->style()->pixelMetric(QStyle::PM_LayoutTopMargin) / 2;
    const int iR = qApp->style()->pixelMetric(QStyle::PM_LayoutRightMargin) / 2;
    const int iB = qApp->style()->pixelMetric(QStyle::PM_LayoutBottomMargin) / 2;
    m_pMainLayout->setContentsMargins(iL, iT, iR, iB);
#ifdef VBOX_WS_MAC
    m_pMainLayout->setSpacing(10);
#else
    m_pMainLayout->setSpacing(qApp->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing) / 2);
#endif

    if (m_pActionPool && m_pActionPool->action(UIActionIndex_M_VISOCreator))
        m_pMainMenu = m_pActionPool->action(UIActionIndex_M_VISOCreator)->menu();
    int iLayoutRow = 0;
    if (m_fShowToolBar)
    {
        m_pToolBar = new QIToolBar(parentWidget());
        if (m_pToolBar)
        {
            /* Configure toolbar: */
            const int iIconMetric = (int)(QApplication::style()->pixelMetric(QStyle::PM_LargeIconSize));
            m_pToolBar->setIconSize(QSize(iIconMetric, iIconMetric));
            m_pToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            m_pMainLayout->addWidget(m_pToolBar, iLayoutRow++, 0, 1, 5);
        }
    }

    m_pHostBrowser = new UIVisoHostBrowser;
    if (m_pHostBrowser)
    {
        m_pMainLayout->addWidget(m_pHostBrowser, iLayoutRow, 0, 1, 4);
        //m_pMainLayout->setColumnStretch(m_pMainLayout->indexOf(m_pHostBrowser), 2);
    }

    prepareVerticalToolBar();
    if (m_pVerticalToolBar)
    {
        m_pMainLayout->addWidget(m_pVerticalToolBar, iLayoutRow, 4, 1, 1);
        //m_pMainLayout->setColumnStretch(m_pMainLayout->indexOf(m_pVerticalToolBar), 1);
    }

    m_pVISOContentBrowser = new UIVisoContentBrowser;
    if (m_pVISOContentBrowser)
    {
        m_pMainLayout->addWidget(m_pVISOContentBrowser, iLayoutRow, 5, 1, 4);
        m_pVISOContentBrowser->setVisoName(m_visoOptions.m_strVisoName);
        //m_pMainLayout->setColumnStretch(m_pMainLayout->indexOf(m_pVISOContentBrowser), 2);
    }
    ++iLayoutRow;
    m_pConfigurationPanel = new UIVisoConfigurationPanel(this);
    if (m_pConfigurationPanel)
    {
        m_pMainLayout->addWidget(m_pConfigurationPanel, iLayoutRow++, 0, 1, 9);
        m_pConfigurationPanel->hide();
        m_pConfigurationPanel->setVisoName(m_visoOptions.m_strVisoName);
        m_pConfigurationPanel->setVisoCustomOptions(m_visoOptions.m_customOptions);
    }

    m_pCreatorOptionsPanel = new UIVisoCreatorOptionsPanel;
    if (m_pCreatorOptionsPanel)
    {
        m_pCreatorOptionsPanel->setShowHiddenbjects(m_browserOptions.m_fShowHiddenObjects);
        m_pMainLayout->addWidget(m_pCreatorOptionsPanel, iLayoutRow++, 0, 1, 9);
        m_pCreatorOptionsPanel->hide();
    }
}

void UIVisoCreatorWidget::prepareConnections()
{
    if (m_pHostBrowser)
    {
        connect(m_pHostBrowser, &UIVisoHostBrowser::sigAddObjectsToViso,
                this, &UIVisoCreatorWidget::sltHandleAddObjectsToViso);
        connect(m_pHostBrowser, &UIVisoHostBrowser::sigTreeViewVisibilityChanged,
                this, &UIVisoCreatorWidget::sltHandleBrowserTreeViewVisibilityChanged);
        connect(m_pHostBrowser, &UIVisoHostBrowser::sigTableSelectionChanged,
                this, &UIVisoCreatorWidget::sltHandleHostBrowserTableSelectionChanged);
        connect(m_pHostBrowser, &UIVisoHostBrowser::sigCreateFileTableViewContextMenu,
                this, &UIVisoCreatorWidget::sltHandleShowContextMenu);
    }

    if (m_pVISOContentBrowser)
    {
        connect(m_pVISOContentBrowser, &UIVisoContentBrowser::sigTableSelectionChanged,
                this, &UIVisoCreatorWidget::sltHandleContentBrowserTableSelectionChanged);
        connect(m_pVISOContentBrowser, &UIVisoContentBrowser::sigCreateFileTableViewContextMenu,
                this, &UIVisoCreatorWidget::sltHandleShowContextMenu);
    }

    if (m_pActionConfiguration)
        connect(m_pActionConfiguration, &QAction::triggered, this, &UIVisoCreatorWidget::sltPanelActionToggled);
    if (m_pActionOptions)
        connect(m_pActionOptions, &QAction::triggered, this, &UIVisoCreatorWidget::sltPanelActionToggled);

    if (m_pConfigurationPanel)
    {
        connect(m_pConfigurationPanel, &UIVisoConfigurationPanel::sigVisoNameChanged,
                this, &UIVisoCreatorWidget::sltHandleVisoNameChanged);
        connect(m_pConfigurationPanel, &UIVisoConfigurationPanel::sigCustomVisoOptionsChanged,
                this, &UIVisoCreatorWidget::sltHandleCustomVisoOptionsChanged);
        connect(m_pConfigurationPanel, &UIVisoConfigurationPanel::sigHidePanel,
                this, &UIVisoCreatorWidget::sltHandleHidePanel);
        m_panelActionMap.insert(m_pConfigurationPanel, m_pActionConfiguration);
    }

    if (m_pCreatorOptionsPanel)
    {
        connect(m_pCreatorOptionsPanel, &UIVisoCreatorOptionsPanel::sigShowHiddenObjects,
                this, &UIVisoCreatorWidget::sltHandleShowHiddenObjectsChange);
        connect(m_pCreatorOptionsPanel, &UIVisoCreatorOptionsPanel::sigHidePanel,
                this, &UIVisoCreatorWidget::sltHandleHidePanel);
        m_panelActionMap.insert(m_pCreatorOptionsPanel, m_pActionOptions);
    }

    if (m_pAddAction)
        connect(m_pAddAction, &QAction::triggered,
                m_pHostBrowser, &UIVisoHostBrowser::sltHandleAddAction);

    if (m_pCreateNewDirectoryAction)
        connect(m_pCreateNewDirectoryAction, &QAction::triggered,
                m_pVISOContentBrowser, &UIVisoContentBrowser::sltHandleCreateNewDirectory);
    if (m_pRemoveAction)
        connect(m_pRemoveAction, &QAction::triggered,
                m_pVISOContentBrowser, &UIVisoContentBrowser::sltHandleRemoveItems);
    if (m_pResetAction)
        connect(m_pResetAction, &QAction::triggered,
                m_pVISOContentBrowser, &UIVisoContentBrowser::sltHandleResetAction);
    if (m_pRenameAction)
        connect(m_pRenameAction, &QAction::triggered,
                m_pVISOContentBrowser,&UIVisoContentBrowser::sltHandleItemRenameAction);
}

void UIVisoCreatorWidget::prepareActions()
{
    if (!m_pActionPool)
        return;

    m_pActionConfiguration = m_pActionPool->action(UIActionIndex_M_VISOCreator_ToggleConfigPanel);
    m_pActionOptions = m_pActionPool->action(UIActionIndex_M_VISOCreator_ToggleOptionsPanel);

    m_pAddAction = m_pActionPool->action(UIActionIndex_M_VISOCreator_Add);
    if (m_pAddAction && m_pHostBrowser)
        m_pAddAction->setEnabled(m_pHostBrowser->tableViewHasSelection());
    m_pRemoveAction = m_pActionPool->action(UIActionIndex_M_VISOCreator_Remove);
    if (m_pRemoveAction && m_pVISOContentBrowser)
        m_pRemoveAction->setEnabled(m_pVISOContentBrowser->tableViewHasSelection());
    m_pCreateNewDirectoryAction = m_pActionPool->action(UIActionIndex_M_VISOCreator_CreateNewDirectory);
    m_pRenameAction = m_pActionPool->action(UIActionIndex_M_VISOCreator_Rename);
    m_pResetAction = m_pActionPool->action(UIActionIndex_M_VISOCreator_Reset);
}

void UIVisoCreatorWidget::populateMenuMainToolbar()
{
    prepareActions();
    if (m_pToolBar)
    {
        if (m_pActionConfiguration)
            m_pToolBar->addAction(m_pActionConfiguration);
        if (m_pActionOptions)
            m_pToolBar->addAction(m_pActionOptions);
    }
    if (m_pMainMenu)
    {
        m_pMainMenu->addAction(m_pActionConfiguration);
        m_pMainMenu->addAction(m_pActionOptions);
        m_pMainMenu->addSeparator();
        m_pMainMenu->addAction(m_pAddAction);
        m_pMainMenu->addAction(m_pRemoveAction);
        m_pMainMenu->addAction(m_pCreateNewDirectoryAction);
        m_pMainMenu->addAction(m_pResetAction);
    }

    if (m_pVerticalToolBar)
    {
        /* Add to dummy QWidget to toolbar to center the action icons vertically: */
        QWidget *topSpacerWidget = new QWidget(this);
        topSpacerWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        topSpacerWidget->setVisible(true);
        QWidget *bottomSpacerWidget = new QWidget(this);
        bottomSpacerWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        bottomSpacerWidget->setVisible(true);

        m_pVerticalToolBar->addWidget(topSpacerWidget);
        if (m_pAddAction)
            m_pVerticalToolBar->addAction(m_pAddAction);
        if (m_pRemoveAction)
            m_pVerticalToolBar->addAction(m_pRemoveAction);
        if (m_pCreateNewDirectoryAction)
            m_pVerticalToolBar->addAction(m_pCreateNewDirectoryAction);
        if (m_pResetAction)
            m_pVerticalToolBar->addAction(m_pResetAction);

        m_pVerticalToolBar->addWidget(bottomSpacerWidget);
    }
}

void UIVisoCreatorWidget::hidePanel(UIDialogPanel* panel)
{
    if (panel && panel->isVisible())
        panel->setVisible(false);
    QMap<UIDialogPanel*, QAction*>::iterator iterator = m_panelActionMap.find(panel);
    if (iterator != m_panelActionMap.end())
    {
        if (iterator.value() && iterator.value()->isChecked())
            iterator.value()->setChecked(false);
    }
    m_visiblePanelsList.removeAll(panel);
    manageEscapeShortCut();
}

void UIVisoCreatorWidget::showPanel(UIDialogPanel* panel)
{
    if (panel && panel->isHidden())
        panel->setVisible(true);
    QMap<UIDialogPanel*, QAction*>::iterator iterator = m_panelActionMap.find(panel);
    if (iterator != m_panelActionMap.end())
    {
        if (!iterator.value()->isChecked())
            iterator.value()->setChecked(true);
    }
    if (!m_visiblePanelsList.contains(panel))
        m_visiblePanelsList.push_back(panel);
    manageEscapeShortCut();
}

void UIVisoCreatorWidget::manageEscapeShortCut()
{
    /* Take the escape key from m_pButtonBox and from the panels in case treeview(s) in
       host and/or content browser is open. We use the escape key to close those first: */
    if ((m_pHostBrowser && m_pHostBrowser->isTreeViewVisible()) ||
        (m_pVISOContentBrowser && m_pVISOContentBrowser->isTreeViewVisible()))
    {
        emit sigSetCancelButtonShortCut(QKeySequence());
        for (int i = 0; i < m_visiblePanelsList.size(); ++i)
            m_visiblePanelsList[i]->setCloseButtonShortCut(QKeySequence());
        return;
    }

    /* if there are no visible panels then assign esc. key to cancel button of the button box: */
    if (m_visiblePanelsList.isEmpty())
    {
        emit sigSetCancelButtonShortCut(QKeySequence(Qt::Key_Escape));
        return;
    }
    emit sigSetCancelButtonShortCut(QKeySequence());

    /* Just loop thru the visible panel list and set the esc key to the
       panel which made visible latest */
    for (int i = 0; i < m_visiblePanelsList.size() - 1; ++i)
        m_visiblePanelsList[i]->setCloseButtonShortCut(QKeySequence());
    m_visiblePanelsList.back()->setCloseButtonShortCut(QKeySequence(Qt::Key_Escape));
}

void UIVisoCreatorWidget::prepareVerticalToolBar()
{
    m_pVerticalToolBar = new QIToolBar;
    if (!m_pVerticalToolBar)
        return;

    m_pVerticalToolBar->setOrientation(Qt::Vertical);
}

/* static */
int UIVisoCreatorWidget::visoWriteQuotedString(PRTSTREAM pStrmDst, const char *pszPrefix,
                                               QString const &rStr, const char *pszPostFix)
{
    QByteArray const utf8Array   = rStr.toUtf8();
    const char      *apszArgv[2] = { utf8Array.constData(), NULL };
    char            *pszQuoted;
    int vrc = RTGetOptArgvToString(&pszQuoted, apszArgv, RTGETOPTARGV_CNV_QUOTE_BOURNE_SH);
    if (RT_SUCCESS(vrc))
    {
        if (pszPrefix)
            vrc = RTStrmPutStr(pStrmDst, pszPrefix);
        if (RT_SUCCESS(vrc))
        {
            vrc = RTStrmPutStr(pStrmDst, pszQuoted);
            if (pszPostFix && RT_SUCCESS(vrc))
                vrc = RTStrmPutStr(pStrmDst, pszPostFix);
        }
        RTStrFree(pszQuoted);
    }
    return vrc;
}

/* static */
QUuid UIVisoCreatorWidget::createViso(UIActionPool *pActionPool, QWidget *pParent,
                                      const QString &strDefaultFolder /* = QString() */,
                                      const QString &strMachineName /* = QString() */)
{
    QWidget *pDialogParent = windowManager().realParentWindow(pParent);
    UIVisoCreatorDialog *pVisoCreator = new UIVisoCreatorDialog(pActionPool, pDialogParent, strMachineName);

    if (!pVisoCreator)
        return QUuid();
    windowManager().registerNewParent(pVisoCreator, pDialogParent);
    pVisoCreator->setCurrentPath(gEDataManager->visoCreatorRecentFolder());

    if (pVisoCreator->exec(false /* not application modal */))
    {
        QStringList files = pVisoCreator->entryList();
        QString strVisoName = pVisoCreator->visoName();
        if (strVisoName.isEmpty())
            strVisoName = strMachineName;

        if (files.empty() || files[0].isEmpty())
        {
            delete pVisoCreator;
            return QUuid();
        }

        gEDataManager->setVISOCreatorRecentFolder(pVisoCreator->currentPath());

        /* Produce the VISO. */
        char szVisoPath[RTPATH_MAX];
        QString strFileName = QString("%1%2").arg(strVisoName).arg(".viso");

        QString strVisoSaveFolder(strDefaultFolder);
        if (strVisoSaveFolder.isEmpty())
            strVisoSaveFolder = uiCommon().defaultFolderPathForType(UIMediumDeviceType_DVD);

        int vrc = RTPathJoin(szVisoPath, sizeof(szVisoPath), strVisoSaveFolder.toUtf8().constData(), strFileName.toUtf8().constData());
        if (RT_SUCCESS(vrc))
        {
            PRTSTREAM pStrmViso;
            vrc = RTStrmOpen(szVisoPath, "w", &pStrmViso);
            if (RT_SUCCESS(vrc))
            {
                RTUUID Uuid;
                vrc = RTUuidCreate(&Uuid);
                if (RT_SUCCESS(vrc))
                {
                    RTStrmPrintf(pStrmViso, "--iprt-iso-maker-file-marker-bourne-sh %RTuuid\n", &Uuid);
                    vrc = UIVisoCreatorWidget::visoWriteQuotedString(pStrmViso, "--volume-id=", strVisoName, "\n");

                    for (int iFile = 0; iFile < files.size() && RT_SUCCESS(vrc); iFile++)
                        vrc = UIVisoCreatorWidget::visoWriteQuotedString(pStrmViso, NULL, files[iFile], "\n");

                    /* Append custom options if any to the file: */
                    const QStringList &customOptions = pVisoCreator->customOptions();
                    foreach (QString strLine, customOptions)
                        RTStrmPrintf(pStrmViso, "%s\n", strLine.toUtf8().constData());

                    RTStrmFlush(pStrmViso);
                    if (RT_SUCCESS(vrc))
                        vrc = RTStrmError(pStrmViso);
                }

                RTStrmClose(pStrmViso);
            }
        }

        /* Done. */
        if (RT_SUCCESS(vrc))
        {
            delete pVisoCreator;
            return uiCommon().openMedium(UIMediumDeviceType_DVD, QString(szVisoPath), pParent);
        }
        /** @todo error message. */
        else
        {
            delete pVisoCreator;
            return QUuid();
        }
    }
    delete pVisoCreator;
    return QUuid();
}


/*********************************************************************************************************************************
*   UIVisoCreatorDialog implementation.                                                                                          *
*********************************************************************************************************************************/
UIVisoCreatorDialog::UIVisoCreatorDialog(UIActionPool *pActionPool, QWidget *pParent, const QString& strMachineName /* = QString() */)
    : QIWithRetranslateUI<QIWithRestorableGeometry<QIMainDialog> >(pParent)
    , m_strMachineName(strMachineName)
    , m_pVisoCreatorWidget(0)
    , m_pButtonBox(0)
    , m_pActionPool(pActionPool)
    , m_iGeometrySaveTimerId(-1)
{
    /* Make sure that the base class does not close this dialog upon pressing escape.
       we manage escape key here with special casing: */
    setRejectByEscape(false);
    prepareWidgets();
    prepareConnections();
    loadSettings();
}

QStringList  UIVisoCreatorDialog::entryList() const
{
    if (m_pVisoCreatorWidget)
        return m_pVisoCreatorWidget->entryList();
    return QStringList();
}

QString UIVisoCreatorDialog::visoName() const
{
    if (m_pVisoCreatorWidget)
        return m_pVisoCreatorWidget->visoName();
    return QString();
}

QStringList UIVisoCreatorDialog::customOptions() const
{
    if (m_pVisoCreatorWidget)
        return m_pVisoCreatorWidget->customOptions();
    return QStringList();
}

QString UIVisoCreatorDialog::currentPath() const
{
    if (m_pVisoCreatorWidget)
        return m_pVisoCreatorWidget->currentPath();
    return QString();
}

void    UIVisoCreatorDialog::setCurrentPath(const QString &strPath)
{
    if (m_pVisoCreatorWidget)
        m_pVisoCreatorWidget->setCurrentPath(strPath);
}

void UIVisoCreatorDialog::prepareWidgets()
{
    QWidget *pCentralWidget = new QWidget;
    setCentralWidget(pCentralWidget);
    QVBoxLayout *pMainLayout = new QVBoxLayout;
    pCentralWidget->setLayout(pMainLayout);


    m_pVisoCreatorWidget = new UIVisoCreatorWidget(m_pActionPool, this, true /* show toolbar */, m_strMachineName);
    if (m_pVisoCreatorWidget)
    {
        menuBar()->addMenu(m_pVisoCreatorWidget->menu());
        pMainLayout->addWidget(m_pVisoCreatorWidget);
        connect(m_pVisoCreatorWidget, &UIVisoCreatorWidget::sigSetCancelButtonShortCut,
                this, &UIVisoCreatorDialog::sltSetCancelButtonShortCut);
        connect(m_pVisoCreatorWidget, &UIVisoCreatorWidget::sigVisoNameChanged,
                this, &UIVisoCreatorDialog::sltsigVisoNameChanged);
    }

    m_pButtonBox = new QIDialogButtonBox;
    if (m_pButtonBox)
    {
        m_pButtonBox->setDoNotPickDefaultButton(true);
        m_pButtonBox->setStandardButtons(QDialogButtonBox::Help | QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
        m_pButtonBox->button(QDialogButtonBox::Cancel)->setShortcut(QKeySequence(Qt::Key_Escape));
        pMainLayout->addWidget(m_pButtonBox);

        connect(m_pButtonBox->button(QIDialogButtonBox::Help), &QPushButton::pressed,
                &(msgCenter()), &UIMessageCenter::sltHandleHelpRequest);
        m_pButtonBox->button(QDialogButtonBox::Help)->setShortcut(QKeySequence::HelpContents);

        uiCommon().setHelpKeyword(m_pButtonBox->button(QIDialogButtonBox::Help), "create-optical-disk-image");
    }
    retranslateUi();
}

void UIVisoCreatorDialog::prepareConnections()
{
    if (m_pButtonBox)
    {
        connect(m_pButtonBox, &QIDialogButtonBox::rejected, this, &UIVisoCreatorDialog::close);
        connect(m_pButtonBox, &QIDialogButtonBox::accepted, this, &UIVisoCreatorDialog::accept);
    }
}

void UIVisoCreatorDialog::retranslateUi()
{
    updateWindowTitle();
    if (m_pButtonBox && m_pButtonBox->button(QDialogButtonBox::Ok))
    {
        m_pButtonBox->button(QDialogButtonBox::Ok)->setText(UIVisoCreatorWidget::tr("C&reate"));
        m_pButtonBox->button(QDialogButtonBox::Ok)->setToolTip(UIVisoCreatorWidget::tr("Creates VISO file with the selected content"));
    }
    if (m_pButtonBox && m_pButtonBox->button(QDialogButtonBox::Help))
        m_pButtonBox->button(QDialogButtonBox::Help)->setToolTip(UIVisoCreatorWidget::tr("Opens the help browser and navigates to the related section"));
}

bool UIVisoCreatorDialog::event(QEvent *pEvent)
{
    if (pEvent->type() == QEvent::Resize || pEvent->type() == QEvent::Move)
    {
        if (m_iGeometrySaveTimerId != -1)
            killTimer(m_iGeometrySaveTimerId);
        m_iGeometrySaveTimerId = startTimer(300);
    }
    else if (pEvent->type() == QEvent::Timer)
    {
        QTimerEvent *pTimerEvent = static_cast<QTimerEvent*>(pEvent);
        if (pTimerEvent->timerId() == m_iGeometrySaveTimerId)
        {
            killTimer(m_iGeometrySaveTimerId);
            m_iGeometrySaveTimerId = -1;
            saveDialogGeometry();
        }
    }
    return QIWithRetranslateUI<QIWithRestorableGeometry<QIMainDialog> >::event(pEvent);
}

void UIVisoCreatorDialog::sltSetCancelButtonShortCut(QKeySequence keySequence)
{
    if (m_pButtonBox && m_pButtonBox->button(QDialogButtonBox::Cancel))
        m_pButtonBox->button(QDialogButtonBox::Cancel)->setShortcut(keySequence);
}

void UIVisoCreatorDialog::sltsigVisoNameChanged(const QString &strName)
{
    Q_UNUSED(strName);
    updateWindowTitle();
}

void UIVisoCreatorDialog::loadSettings()
{
    const QRect availableGeo = gpDesktop->availableGeometry(this);
    int iDefaultWidth = availableGeo.width() / 2;
    int iDefaultHeight = availableGeo.height() * 3 / 4;
    QRect defaultGeo(0, 0, iDefaultWidth, iDefaultHeight);

    QWidget *pParent = windowManager().realParentWindow(parentWidget() ? parentWidget() : windowManager().mainWindowShown());
    /* Load geometry from extradata: */
    const QRect geo = gEDataManager->visoCreatorDialogGeometry(this, pParent, defaultGeo);
    LogRel2(("GUI: UISoftKeyboard: Restoring geometry to: Origin=%dx%d, Size=%dx%d\n",
             geo.x(), geo.y(), geo.width(), geo.height()));

    restoreGeometry(geo);
}

void UIVisoCreatorDialog::saveDialogGeometry()
{
    const QRect geo = currentGeometry();
    LogRel2(("GUI: UIMediumSelector: Saving geometry as: Origin=%dx%d, Size=%dx%d\n",
             geo.x(), geo.y(), geo.width(), geo.height()));
    gEDataManager->setVisoCreatorDialogGeometry(geo, isCurrentlyMaximized());
}

void UIVisoCreatorDialog::updateWindowTitle()
{
    setWindowTitle(QString("%1 - %2.%3").arg(UIVisoCreatorWidget::tr("VISO Creator")).arg(visoName()).arg("viso"));
}
