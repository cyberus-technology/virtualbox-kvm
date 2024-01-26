/* $Id: UIVirtualBoxManagerWidget.cpp $ */
/** @file
 * VBox Qt GUI - UIVirtualBoxManagerWidget class implementation.
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
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

/* GUI includes: */
#include "QISplitter.h"
#include "UIActionPoolManager.h"
#include "UIDesktopWidgetWatchdog.h"
#include "UIExtraDataManager.h"
#include "UIChooser.h"
#include "UIMessageCenter.h"
#include "UINotificationCenter.h"
#include "UIVirtualBoxManager.h"
#include "UIVirtualBoxManagerWidget.h"
#include "UITabBar.h"
#include "QIToolBar.h"
#include "UIVirtualBoxEventHandler.h"
#include "UIVirtualMachineItemCloud.h"
#include "UIVirtualMachineItemLocal.h"
#include "UITools.h"
#ifndef VBOX_WS_MAC
# include "UIMenuBar.h"
#endif
#ifdef VBOX_WS_MAC
# ifdef VBOX_IS_QT6_OR_LATER /* we are branding as Dev Preview since 6.0 */
#  include "UIIconPool.h"
# endif
#endif


UIVirtualBoxManagerWidget::UIVirtualBoxManagerWidget(UIVirtualBoxManager *pParent)
    : m_pActionPool(pParent->actionPool())
    , m_pSplitter(0)
    , m_pToolBar(0)
    , m_pPaneChooser(0)
    , m_pStackedWidget(0)
    , m_pPaneToolsGlobal(0)
    , m_pPaneToolsMachine(0)
    , m_pSlidingAnimation(0)
    , m_pPaneTools(0)
    , m_enmSelectionType(SelectionType_Invalid)
    , m_fSelectedMachineItemAccessible(false)
    , m_pSplitterSettingsSaveTimer(0)
{
    prepare();
}

UIVirtualBoxManagerWidget::~UIVirtualBoxManagerWidget()
{
    cleanup();
}

UIVirtualMachineItem *UIVirtualBoxManagerWidget::currentItem() const
{
    return m_pPaneChooser->currentItem();
}

QList<UIVirtualMachineItem*> UIVirtualBoxManagerWidget::currentItems() const
{
    return m_pPaneChooser->currentItems();
}

bool UIVirtualBoxManagerWidget::isGroupItemSelected() const
{
    return m_pPaneChooser->isGroupItemSelected();
}

bool UIVirtualBoxManagerWidget::isGlobalItemSelected() const
{
    return m_pPaneChooser->isGlobalItemSelected();
}

bool UIVirtualBoxManagerWidget::isMachineItemSelected() const
{
    return m_pPaneChooser->isMachineItemSelected();
}

bool UIVirtualBoxManagerWidget::isLocalMachineItemSelected() const
{
    return m_pPaneChooser->isLocalMachineItemSelected();
}

bool UIVirtualBoxManagerWidget::isCloudMachineItemSelected() const
{
    return m_pPaneChooser->isCloudMachineItemSelected();
}

bool UIVirtualBoxManagerWidget::isSingleGroupSelected() const
{
    return m_pPaneChooser->isSingleGroupSelected();
}

bool UIVirtualBoxManagerWidget::isSingleLocalGroupSelected() const
{
    return m_pPaneChooser->isSingleLocalGroupSelected();
}

bool UIVirtualBoxManagerWidget::isSingleCloudProviderGroupSelected() const
{
    return m_pPaneChooser->isSingleCloudProviderGroupSelected();
}

bool UIVirtualBoxManagerWidget::isSingleCloudProfileGroupSelected() const
{
    return m_pPaneChooser->isSingleCloudProfileGroupSelected();
}

bool UIVirtualBoxManagerWidget::isAllItemsOfOneGroupSelected() const
{
    return m_pPaneChooser->isAllItemsOfOneGroupSelected();
}

QString UIVirtualBoxManagerWidget::fullGroupName() const
{
    return m_pPaneChooser->fullGroupName();
}

bool UIVirtualBoxManagerWidget::isGroupSavingInProgress() const
{
    return m_pPaneChooser->isGroupSavingInProgress();
}

bool UIVirtualBoxManagerWidget::isCloudProfileUpdateInProgress() const
{
    return m_pPaneChooser->isCloudProfileUpdateInProgress();
}

void UIVirtualBoxManagerWidget::switchToGlobalItem()
{
    AssertPtrReturnVoid(m_pPaneChooser);
    m_pPaneChooser->setCurrentGlobal();
}

void UIVirtualBoxManagerWidget::openGroupNameEditor()
{
    m_pPaneChooser->openGroupNameEditor();
}

void UIVirtualBoxManagerWidget::disbandGroup()
{
    m_pPaneChooser->disbandGroup();
}

void UIVirtualBoxManagerWidget::removeMachine()
{
    m_pPaneChooser->removeMachine();
}

void UIVirtualBoxManagerWidget::moveMachineToGroup(const QString &strName /* = QString() */)
{
    m_pPaneChooser->moveMachineToGroup(strName);
}

QStringList UIVirtualBoxManagerWidget::possibleGroupsForMachineToMove(const QUuid &uId)
{
    return m_pPaneChooser->possibleGroupsForMachineToMove(uId);
}

QStringList UIVirtualBoxManagerWidget::possibleGroupsForGroupToMove(const QString &strFullName)
{
    return m_pPaneChooser->possibleGroupsForGroupToMove(strFullName);
}

void UIVirtualBoxManagerWidget::refreshMachine()
{
    m_pPaneChooser->refreshMachine();
}

void UIVirtualBoxManagerWidget::sortGroup()
{
    m_pPaneChooser->sortGroup();
}

void UIVirtualBoxManagerWidget::setMachineSearchWidgetVisibility(bool fVisible)
{
    m_pPaneChooser->setMachineSearchWidgetVisibility(fVisible);
}

void UIVirtualBoxManagerWidget::setToolsType(UIToolType enmType)
{
    m_pPaneTools->setToolsType(enmType);
}

UIToolType UIVirtualBoxManagerWidget::toolsType() const
{
    return m_pPaneTools ? m_pPaneTools->toolsType() : UIToolType_Invalid;
}

UIToolType UIVirtualBoxManagerWidget::currentGlobalTool() const
{
    return m_pPaneToolsGlobal ? m_pPaneToolsGlobal->currentTool() : UIToolType_Invalid;
}

UIToolType UIVirtualBoxManagerWidget::currentMachineTool() const
{
    return m_pPaneToolsMachine ? m_pPaneToolsMachine->currentTool() : UIToolType_Invalid;
}

bool UIVirtualBoxManagerWidget::isGlobalToolOpened(UIToolType enmType) const
{
    return m_pPaneToolsGlobal ? m_pPaneToolsGlobal->isToolOpened(enmType) : false;
}

bool UIVirtualBoxManagerWidget::isMachineToolOpened(UIToolType enmType) const
{
    return m_pPaneToolsMachine ? m_pPaneToolsMachine->isToolOpened(enmType) : false;
}

void UIVirtualBoxManagerWidget::switchToGlobalTool(UIToolType enmType)
{
    /* Open corresponding tool: */
    m_pPaneToolsGlobal->openTool(enmType);

    /* Let the parent know: */
    emit sigToolTypeChange();

    /* Update toolbar: */
    updateToolbar();
}

void UIVirtualBoxManagerWidget::switchToMachineTool(UIToolType enmType)
{
    /* Open corresponding tool: */
    m_pPaneToolsMachine->openTool(enmType);

    /* Let the parent know: */
    emit sigToolTypeChange();

    /* Update toolbar: */
    updateToolbar();
}

void UIVirtualBoxManagerWidget::closeGlobalTool(UIToolType enmType)
{
    m_pPaneToolsGlobal->closeTool(enmType);
}

void UIVirtualBoxManagerWidget::closeMachineTool(UIToolType enmType)
{
    m_pPaneToolsMachine->closeTool(enmType);
}

bool UIVirtualBoxManagerWidget::isCurrentStateItemSelected() const
{
    return m_pPaneToolsMachine->isCurrentStateItemSelected();
}

void UIVirtualBoxManagerWidget::updateToolBarMenuButtons(bool fSeparateMenuSection)
{
    QToolButton *pButton = qobject_cast<QToolButton*>(m_pToolBar->widgetForAction(actionPool()->action(UIActionIndexMN_M_Machine_M_StartOrShow)));
    if (pButton)
        pButton->setPopupMode(fSeparateMenuSection ? QToolButton::MenuButtonPopup : QToolButton::DelayedPopup);
}

void UIVirtualBoxManagerWidget::showHelpBrowser()
{
    QString strHelpKeyword;
    if (isGlobalItemSelected())
        strHelpKeyword = m_pPaneToolsGlobal->currentHelpKeyword();
    else if (isMachineItemSelected())
        strHelpKeyword = m_pPaneToolsMachine->currentHelpKeyword();

    msgCenter().sltHandleHelpRequestWithKeyword(strHelpKeyword);
}

void UIVirtualBoxManagerWidget::sltHandleToolBarContextMenuRequest(const QPoint &position)
{
    /* Populate toolbar actions: */
    QList<QAction*> actions;
    /* Add 'Show Toolbar Text' action: */
    QAction *pShowToolBarText = new QAction(UIVirtualBoxManager::tr("Show Toolbar Text"), 0);
    if (pShowToolBarText)
    {
        pShowToolBarText->setCheckable(true);
        pShowToolBarText->setChecked(m_pToolBar->toolButtonStyle() == Qt::ToolButtonTextUnderIcon);
        actions << pShowToolBarText;
    }

    /* Prepare the menu position: */
    QPoint globalPosition = position;
    QWidget *pSender = qobject_cast<QWidget*>(sender());
    if (pSender)
        globalPosition = pSender->mapToGlobal(position);

    /* Execute the menu: */
    QAction *pResult = QMenu::exec(actions, globalPosition);

    /* Handle the menu execution result: */
    if (pResult == pShowToolBarText)
    {
        m_pToolBar->setUseTextLabels(pResult->isChecked());
        gEDataManager->setSelectorWindowToolBarTextVisible(pResult->isChecked());
    }
}

void UIVirtualBoxManagerWidget::retranslateUi()
{
    /* Make sure chosen item fetched: */
    sltHandleChooserPaneIndexChange();

#ifdef VBOX_WS_MAC
    // WORKAROUND:
    // There is a bug in Qt Cocoa which result in showing a "more arrow" when
    // the necessary size of the toolbar is increased. Also for some languages
    // the with doesn't match if the text increase. So manually adjust the size
    // after changing the text.
    m_pToolBar->updateLayout();
#endif
}

void UIVirtualBoxManagerWidget::sltHandleStateChange(const QUuid &uId)
{
    // WORKAROUND:
    // In certain intermediate states VM info can be NULL which
    // causing annoying assertions, such updates can be ignored?
    CVirtualBox comVBox = uiCommon().virtualBox();
    CMachine comMachine = comVBox.FindMachine(uId.toString());
    if (comVBox.isOk() && comMachine.isNotNull())
    {
        switch (comMachine.GetState())
        {
            case KMachineState_DeletingSnapshot:
                return;
            default:
                break;
        }
    }

    /* Recache current item info if machine or group item selected: */
    if (isMachineItemSelected() || isGroupItemSelected())
        recacheCurrentItemInformation();
}

void UIVirtualBoxManagerWidget::sltHandleSplitterMove()
{
    /* Create timer if isn't exist already: */
    if (!m_pSplitterSettingsSaveTimer)
    {
        m_pSplitterSettingsSaveTimer = new QTimer(this);
        if (m_pSplitterSettingsSaveTimer)
        {
            m_pSplitterSettingsSaveTimer->setInterval(300);
            m_pSplitterSettingsSaveTimer->setSingleShot(true);
            connect(m_pSplitterSettingsSaveTimer, &QTimer::timeout,
                    this, &UIVirtualBoxManagerWidget::sltSaveSplitterSettings);
        }
    }
    /* [Re]start timer finally: */
    m_pSplitterSettingsSaveTimer->start();
}

void UIVirtualBoxManagerWidget::sltSaveSplitterSettings()
{
    const QList<int> splitterSizes = m_pSplitter->sizes();
    LogRel2(("GUI: UIVirtualBoxManagerWidget: Saving splitter as: Size=%d,%d\n",
             splitterSizes.at(0), splitterSizes.at(1)));
    gEDataManager->setSelectorWindowSplitterHints(splitterSizes);
}

void UIVirtualBoxManagerWidget::sltHandleToolBarResize(const QSize &newSize)
{
    emit sigToolBarHeightChange(newSize.height());
}

void UIVirtualBoxManagerWidget::sltHandleChooserPaneIndexChange()
{
    /* Let the parent know: */
    emit sigChooserPaneIndexChange();

    /* If global item is selected and we are on machine tools pane => switch to global tools pane: */
    if (   isGlobalItemSelected()
        && m_pStackedWidget->currentWidget() != m_pPaneToolsGlobal)
    {
        /* Just start animation and return, do nothing else.. */
        m_pStackedWidget->setCurrentWidget(m_pPaneToolsGlobal); // rendering w/a
        m_pStackedWidget->setCurrentWidget(m_pSlidingAnimation);
        m_pSlidingAnimation->animate(SlidingDirection_Reverse);
        return;
    }

    else

    /* If machine or group item is selected and we are on global tools pane => switch to machine tools pane: */
    if (   (isMachineItemSelected() || isGroupItemSelected())
        && m_pStackedWidget->currentWidget() != m_pPaneToolsMachine)
    {
        /* Just start animation and return, do nothing else.. */
        m_pStackedWidget->setCurrentWidget(m_pPaneToolsMachine); // rendering w/a
        m_pStackedWidget->setCurrentWidget(m_pSlidingAnimation);
        m_pSlidingAnimation->animate(SlidingDirection_Forward);
        return;
    }

    /* Recache current item info if machine or group item selected: */
    if (isMachineItemSelected() || isGroupItemSelected())
        recacheCurrentItemInformation();

    /* Calculate selection type: */
    const SelectionType enmSelectedItemType = isSingleLocalGroupSelected()
                                            ? SelectionType_SingleLocalGroupItem
                                            : isSingleCloudProviderGroupSelected() || isSingleCloudProfileGroupSelected()
                                            ? SelectionType_SingleCloudGroupItem
                                            : isGlobalItemSelected()
                                            ? SelectionType_FirstIsGlobalItem
                                            : isLocalMachineItemSelected()
                                            ? SelectionType_FirstIsLocalMachineItem
                                            : isCloudMachineItemSelected()
                                            ? SelectionType_FirstIsCloudMachineItem
                                            : SelectionType_Invalid;
    /* Acquire current item: */
    UIVirtualMachineItem *pItem = currentItem();
    const bool fCurrentItemIsOk = pItem && pItem->accessible();

    /* Update toolbar if selection type or item accessibility got changed: */
    if (   m_enmSelectionType != enmSelectedItemType
        || m_fSelectedMachineItemAccessible != fCurrentItemIsOk)
        updateToolbar();

    /* Remember the last selection type: */
    m_enmSelectionType = enmSelectedItemType;
    /* Remember whether the last selected item was accessible: */
    m_fSelectedMachineItemAccessible = fCurrentItemIsOk;
}

void UIVirtualBoxManagerWidget::sltHandleSlidingAnimationComplete(SlidingDirection enmDirection)
{
    /* First switch the panes: */
    switch (enmDirection)
    {
        case SlidingDirection_Forward:
        {
            m_pPaneTools->setToolsClass(UIToolClass_Machine);
            m_pStackedWidget->setCurrentWidget(m_pPaneToolsMachine);
            m_pPaneToolsGlobal->setActive(false);
            m_pPaneToolsMachine->setActive(true);
            break;
        }
        case SlidingDirection_Reverse:
        {
            m_pPaneTools->setToolsClass(UIToolClass_Global);
            m_pStackedWidget->setCurrentWidget(m_pPaneToolsGlobal);
            m_pPaneToolsMachine->setActive(false);
            m_pPaneToolsGlobal->setActive(true);
            break;
        }
    }
    /* Then handle current item change (again!): */
    sltHandleChooserPaneIndexChange();
}

void UIVirtualBoxManagerWidget::sltHandleCloudMachineStateChange(const QUuid &uId)
{
    /* Not for global items: */
    if (!isGlobalItemSelected())
    {
        /* Acquire current item: */
        UIVirtualMachineItem *pItem = currentItem();
        const bool fCurrentItemIsOk = pItem && pItem->accessible();

        /* If current item is Ok: */
        if (fCurrentItemIsOk)
        {
            /* If Error-pane is chosen currently => open tool currently chosen in Tools-pane: */
            if (m_pPaneToolsMachine->currentTool() == UIToolType_Error)
                sltHandleToolsPaneIndexChange();

            /* If we still have same item selected: */
            if (pItem && pItem->id() == uId)
            {
                /* Propagate current items to update the Details-pane: */
                m_pPaneToolsMachine->setItems(currentItems());
            }
        }
        else
        {
            /* Make sure Error pane raised: */
            if (m_pPaneToolsMachine->currentTool() != UIToolType_Error)
                m_pPaneToolsMachine->openTool(UIToolType_Error);

            /* If we still have same item selected: */
            if (pItem && pItem->id() == uId)
            {
                /* Propagate current items to update the Details-pane (in any case): */
                m_pPaneToolsMachine->setItems(currentItems());
                /* Propagate last access error to update the Error-pane (if machine selected but inaccessible): */
                m_pPaneToolsMachine->setErrorDetails(pItem->accessError());
            }
        }

        /* Pass the signal further: */
        emit sigCloudMachineStateChange(uId);
    }
}

void UIVirtualBoxManagerWidget::sltHandleToolMenuRequested(UIToolClass enmClass, const QPoint &position)
{
    /* Define current tools class: */
    m_pPaneTools->setToolsClass(enmClass);

    /* Compose popup-menu geometry first of all: */
    QRect ourGeo = QRect(position, m_pPaneTools->minimumSizeHint());
    /* Adjust location only to properly fit into available geometry space: */
    const QRect availableGeo = gpDesktop->availableGeometry(position);
    ourGeo = gpDesktop->normalizeGeometry(ourGeo, availableGeo, false /* resize? */);

    /* Move, resize and show: */
    m_pPaneTools->move(ourGeo.topLeft());
    m_pPaneTools->show();
    // WORKAROUND:
    // Don't want even to think why, but for Qt::Popup resize to
    // smaller size often being ignored until it is actually shown.
    m_pPaneTools->resize(ourGeo.size());
}

void UIVirtualBoxManagerWidget::sltHandleToolsPaneIndexChange()
{
    /* Acquire current class/type: */
    const UIToolClass enmCurrentClass = m_pPaneTools->toolsClass();
    const UIToolType enmCurrentType = m_pPaneTools->toolsType();

    /* Invent default for fallback case: */
    const UIToolType enmDefaultType = enmCurrentClass == UIToolClass_Global ? UIToolType_Welcome
                                    : enmCurrentClass == UIToolClass_Machine ? UIToolType_Details
                                    : UIToolType_Invalid;
    AssertReturnVoid(enmDefaultType != UIToolType_Invalid);

    /* Calculate new type to choose: */
    const UIToolType enmNewType = UIToolStuff::isTypeOfClass(enmCurrentType, enmCurrentClass)
                                ? enmCurrentType : enmDefaultType;

    /* Choose new type: */
    switch (m_pPaneTools->toolsClass())
    {
        case UIToolClass_Global: switchToGlobalTool(enmNewType); break;
        case UIToolClass_Machine: switchToMachineTool(enmNewType); break;
        default: break;
    }
}

void UIVirtualBoxManagerWidget::sltSwitchToMachineActivityPane(const QUuid &uMachineId)
{
    AssertPtrReturnVoid(m_pPaneChooser);
    AssertPtrReturnVoid(m_pPaneTools);
    m_pPaneChooser->setCurrentMachine(uMachineId);
    m_pPaneTools->setToolsType(UIToolType_VMActivity);
}

void UIVirtualBoxManagerWidget::sltSwitchToActivityOverviewPane()
{
    AssertPtrReturnVoid(m_pPaneChooser);
    AssertPtrReturnVoid(m_pPaneTools);
    m_pPaneTools->setToolsType(UIToolType_VMActivityOverview);
    m_pPaneChooser->setCurrentGlobal();
}

void UIVirtualBoxManagerWidget::prepare()
{
    /* Prepare everything: */
    prepareWidgets();
    prepareConnections();

    /* Load settings: */
    loadSettings();

    /* Translate UI: */
    retranslateUi();

    /* Make sure current Chooser-pane index fetched: */
    sltHandleChooserPaneIndexChange();
}

void UIVirtualBoxManagerWidget::prepareWidgets()
{
    /* Create main-layout: */
    QHBoxLayout *pLayoutMain = new QHBoxLayout(this);
    if (pLayoutMain)
    {
        /* Configure layout: */
        pLayoutMain->setSpacing(0);
        pLayoutMain->setContentsMargins(0, 0, 0, 0);

        /* Create splitter: */
        m_pSplitter = new QISplitter(Qt::Horizontal, QISplitter::Flat);
        if (m_pSplitter)
        {
            /* Configure splitter: */
            m_pSplitter->setHandleWidth(1);

            /* Create Chooser-pane: */
            m_pPaneChooser = new UIChooser(this, actionPool());
            if (m_pPaneChooser)
            {
                /* Add into splitter: */
                m_pSplitter->addWidget(m_pPaneChooser);
            }

            /* Create right widget: */
            QWidget *pWidgetRight = new QWidget;
            if (pWidgetRight)
            {
                /* Create right-layout: */
                QVBoxLayout *pLayoutRight = new QVBoxLayout(pWidgetRight);
                if(pLayoutRight)
                {
                    /* Configure layout: */
                    pLayoutRight->setSpacing(0);
                    pLayoutRight->setContentsMargins(0, 0, 0, 0);

                    /* Create Main toolbar: */
                    m_pToolBar = new QIToolBar;
                    if (m_pToolBar)
                    {
                        /* Configure toolbar: */
                        const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_LargeIconSize);
                        m_pToolBar->setIconSize(QSize(iIconMetric, iIconMetric));
                        m_pToolBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                        m_pToolBar->setContextMenuPolicy(Qt::CustomContextMenu);
                        m_pToolBar->setUseTextLabels(true);
#ifdef VBOX_WS_MAC
                        m_pToolBar->emulateMacToolbar();
# ifdef VBOX_IS_QT6_OR_LATER /* we are branding as Dev Preview since 6.0 */
                        /* Branding stuff for Qt6 beta: */
                        if (uiCommon().showBetaLabel())
                            m_pToolBar->enableBranding(UIIconPool::iconSet(":/explosion_hazard_32px.png"),
                                                       "Dev Preview", // do we need to make it NLS?
                                                       QColor(246, 179, 0),
                                                       74 /* width of BETA label */);
# endif
#endif /* VBOX_WS_MAC */

                        /* Add toolbar into layout: */
                        pLayoutRight->addWidget(m_pToolBar);
                    }

                    /* Create stacked-widget: */
                    m_pStackedWidget = new QStackedWidget;
                    if (m_pStackedWidget)
                    {
                        /* Create Global Tools-pane: */
                        m_pPaneToolsGlobal = new UIToolPaneGlobal(actionPool());
                        if (m_pPaneToolsGlobal)
                        {
                            if (m_pPaneChooser->isGlobalItemSelected())
                                m_pPaneToolsGlobal->setActive(true);
                            connect(m_pPaneToolsGlobal, &UIToolPaneGlobal::sigSwitchToMachineActivityPane,
                                    this, &UIVirtualBoxManagerWidget::sltSwitchToMachineActivityPane);

                            /* Add into stack: */
                            m_pStackedWidget->addWidget(m_pPaneToolsGlobal);
                        }

                        /* Create Machine Tools-pane: */
                        m_pPaneToolsMachine = new UIToolPaneMachine(actionPool());
                        if (m_pPaneToolsMachine)
                        {
                            if (!m_pPaneChooser->isGlobalItemSelected())
                                m_pPaneToolsMachine->setActive(true);
                            connect(m_pPaneToolsMachine, &UIToolPaneMachine::sigCurrentSnapshotItemChange,
                                    this, &UIVirtualBoxManagerWidget::sigCurrentSnapshotItemChange);
                            connect(m_pPaneToolsMachine, &UIToolPaneMachine::sigSwitchToActivityOverviewPane,
                                    this, &UIVirtualBoxManagerWidget::sltSwitchToActivityOverviewPane);

                            /* Add into stack: */
                            m_pStackedWidget->addWidget(m_pPaneToolsMachine);
                        }

                        /* Create sliding-widget: */
                        // Reverse initial animation direction if group or machine selected!
                        const bool fReverse = !m_pPaneChooser->isGlobalItemSelected();
                        m_pSlidingAnimation = new UISlidingAnimation(Qt::Vertical, fReverse);
                        if (m_pSlidingAnimation)
                        {
                            /* Add first/second widgets into sliding animation: */
                            m_pSlidingAnimation->setWidgets(m_pPaneToolsGlobal, m_pPaneToolsMachine);
                            connect(m_pSlidingAnimation, &UISlidingAnimation::sigAnimationComplete,
                                    this, &UIVirtualBoxManagerWidget::sltHandleSlidingAnimationComplete);

                            /* Add into stack: */
                            m_pStackedWidget->addWidget(m_pSlidingAnimation);
                        }

                        /* Choose which pane should be active initially: */
                        if (m_pPaneChooser->isGlobalItemSelected())
                            m_pStackedWidget->setCurrentWidget(m_pPaneToolsGlobal);
                        else
                            m_pStackedWidget->setCurrentWidget(m_pPaneToolsMachine);

                        /* Add into layout: */
                        pLayoutRight->addWidget(m_pStackedWidget, 1);
                    }
                }

                /* Add into splitter: */
                m_pSplitter->addWidget(pWidgetRight);
            }

            /* Adjust splitter colors according to main widgets it splits: */
            m_pSplitter->configureColor(QApplication::palette().color(QPalette::Active, QPalette::Window).darker(130));
            /* Set the initial distribution. The right site is bigger. */
            m_pSplitter->setStretchFactor(0, 2);
            m_pSplitter->setStretchFactor(1, 3);

            /* Add into layout: */
            pLayoutMain->addWidget(m_pSplitter);
        }

        /* Create Tools-pane: */
        m_pPaneTools = new UITools(this);
        if (m_pPaneTools)
        {
            /* Choose which pane should be active initially: */
            if (m_pPaneChooser->isGlobalItemSelected())
                m_pPaneTools->setToolsClass(UIToolClass_Global);
            else
                m_pPaneTools->setToolsClass(UIToolClass_Machine);
        }
    }

    /* Create notification-center: */
    UINotificationCenter::create(this);

    /* Update toolbar finally: */
    updateToolbar();

    /* Bring the VM list to the focus: */
    m_pPaneChooser->setFocus();
}

void UIVirtualBoxManagerWidget::prepareConnections()
{
    /* Global VBox event handlers: */
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigMachineStateChange,
            this, &UIVirtualBoxManagerWidget::sltHandleStateChange);

    /* Splitter connections: */
    connect(m_pSplitter, &QISplitter::splitterMoved,
            this, &UIVirtualBoxManagerWidget::sltHandleSplitterMove);

    /* Tool-bar connections: */
    connect(m_pToolBar, &QIToolBar::customContextMenuRequested,
            this, &UIVirtualBoxManagerWidget::sltHandleToolBarContextMenuRequest);
    connect(m_pToolBar, &QIToolBar::sigResized,
            this, &UIVirtualBoxManagerWidget::sltHandleToolBarResize);

    /* Chooser-pane connections: */
    connect(this, &UIVirtualBoxManagerWidget::sigToolBarHeightChange,
            m_pPaneChooser, &UIChooser::setGlobalItemHeightHint);
    connect(m_pPaneChooser, &UIChooser::sigSelectionChanged,
            this, &UIVirtualBoxManagerWidget::sltHandleChooserPaneIndexChange);
    connect(m_pPaneChooser, &UIChooser::sigSelectionInvalidated,
            this, &UIVirtualBoxManagerWidget::sltHandleChooserPaneSelectionInvalidated);
    connect(m_pPaneChooser, &UIChooser::sigToggleStarted,
            m_pPaneToolsMachine, &UIToolPaneMachine::sigToggleStarted);
    connect(m_pPaneChooser, &UIChooser::sigToggleFinished,
            m_pPaneToolsMachine, &UIToolPaneMachine::sigToggleFinished);
    connect(m_pPaneChooser, &UIChooser::sigGroupSavingStateChanged,
            this, &UIVirtualBoxManagerWidget::sigGroupSavingStateChanged);
    connect(m_pPaneChooser, &UIChooser::sigCloudUpdateStateChanged,
            this, &UIVirtualBoxManagerWidget::sigCloudUpdateStateChanged);
    connect(m_pPaneChooser, &UIChooser::sigToolMenuRequested,
            this, &UIVirtualBoxManagerWidget::sltHandleToolMenuRequested);
    connect(m_pPaneChooser, &UIChooser::sigCloudMachineStateChange,
            this, &UIVirtualBoxManagerWidget::sltHandleCloudMachineStateChange);
    connect(m_pPaneChooser, &UIChooser::sigStartOrShowRequest,
            this, &UIVirtualBoxManagerWidget::sigStartOrShowRequest);
    connect(m_pPaneChooser, &UIChooser::sigMachineSearchWidgetVisibilityChanged,
            this, &UIVirtualBoxManagerWidget::sigMachineSearchWidgetVisibilityChanged);

    /* Details-pane connections: */
    connect(m_pPaneToolsMachine, &UIToolPaneMachine::sigLinkClicked,
            this, &UIVirtualBoxManagerWidget::sigMachineSettingsLinkClicked);

    /* Tools-pane connections: */
    connect(m_pPaneTools, &UITools::sigSelectionChanged,
            this, &UIVirtualBoxManagerWidget::sltHandleToolsPaneIndexChange);
}

void UIVirtualBoxManagerWidget::loadSettings()
{
    /* Restore splitter handle position: */
    {
        QList<int> sizes = gEDataManager->selectorWindowSplitterHints();
        /* If both hints are zero, we have the 'default' case: */
        if (sizes.at(0) == 0 && sizes.at(1) == 0)
        {
            sizes[0] = (int)(width() * .9 * (1.0 / 3));
            sizes[1] = (int)(width() * .9 * (2.0 / 3));
        }
        LogRel2(("GUI: UIVirtualBoxManagerWidget: Restoring splitter to: Size=%d,%d\n",
                 sizes.at(0), sizes.at(1)));
        m_pSplitter->setSizes(sizes);
    }

    /* Restore toolbar settings: */
    {
        m_pToolBar->setUseTextLabels(gEDataManager->selectorWindowToolBarTextVisible());
    }

    /* Open tools last chosen in Tools-pane: */
    switchToGlobalTool(m_pPaneTools->lastSelectedToolGlobal());
    switchToMachineTool(m_pPaneTools->lastSelectedToolMachine());
}

void UIVirtualBoxManagerWidget::updateToolbar()
{
    /* Make sure toolbar exists: */
    AssertPtrReturnVoid(m_pToolBar);

    /* Clear initially: */
    m_pToolBar->clear();

    /* Basic action set: */
    switch (m_pPaneTools->toolsClass())
    {
        /* Global toolbar: */
        case UIToolClass_Global:
        {
            switch (currentGlobalTool())
            {
                case UIToolType_Welcome:
                {
                    m_pToolBar->addAction(actionPool()->action(UIActionIndex_M_Application_S_Preferences));
                    m_pToolBar->addSeparator();
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_File_S_ImportAppliance));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_File_S_ExportAppliance));
                    m_pToolBar->addSeparator();
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Welcome_S_New));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Welcome_S_Add));
                    break;
                }
                case UIToolType_Extensions:
                {
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Extension_S_Install));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Extension_S_Uninstall));
                    break;
                }
                case UIToolType_Media:
                {
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Medium_S_Add));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Medium_S_Create));
                    m_pToolBar->addSeparator();
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Medium_S_Copy));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Medium_S_Move));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Medium_S_Remove));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Medium_S_Release));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Medium_S_Clear));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Medium_T_Search));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Medium_T_Details));
                    m_pToolBar->addSeparator();
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Medium_S_Refresh));
                    break;
                }
                case UIToolType_Network:
                {
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Network_S_Create));
                    m_pToolBar->addSeparator();
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Network_S_Remove));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Network_T_Details));
                    //m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Network_S_Refresh));
                    break;
                }
                case UIToolType_Cloud:
                {
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Cloud_S_Add));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Cloud_S_Import));
                    m_pToolBar->addSeparator();
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Cloud_S_Remove));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Cloud_T_Details));
                    m_pToolBar->addSeparator();
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Cloud_S_TryPage));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Cloud_S_Help));
                    break;
                }
                case UIToolType_VMActivityOverview:
                {
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_VMActivityOverview_M_Columns));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_VMActivityOverview_S_SwitchToMachineActivity));
                    QToolButton *pButton =
                        qobject_cast<QToolButton*>(m_pToolBar->widgetForAction(actionPool()->action(UIActionIndexMN_M_VMActivityOverview_M_Columns)));
                    if (pButton)
                    {
                        pButton->setPopupMode(QToolButton::InstantPopup);
                        pButton->setAutoRaise(true);
                    }
                    break;
                }

                default:
                    break;
            }
            break;
        }
        /* Machine toolbar: */
        case UIToolClass_Machine:
        {
            switch (currentMachineTool())
            {
                case UIToolType_Details:
                {
                    if (isSingleGroupSelected())
                    {
                        m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Group_S_New));
                        m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Group_S_Add));
                        m_pToolBar->addSeparator();
                        if (isSingleLocalGroupSelected())
                            m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Group_S_Discard));
                        else if (   isSingleCloudProviderGroupSelected()
                                 || isSingleCloudProfileGroupSelected())
                            m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Group_M_Stop_S_Terminate));
                        m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Group_M_StartOrShow));
                    }
                    else
                    {
                        m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_New));
                        m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Add));
                        m_pToolBar->addSeparator();
                        m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Settings));
                        if (isLocalMachineItemSelected())
                            m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Discard));
                        else if (isCloudMachineItemSelected())
                            m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_M_Stop_S_Terminate));
                        m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_M_StartOrShow));
                    }
                    break;
                }
                case UIToolType_Snapshots:
                {
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Snapshot_S_Take));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Snapshot_S_Delete));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Snapshot_S_Restore));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Snapshot_T_Properties));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Snapshot_S_Clone));
                    m_pToolBar->addSeparator();
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Settings));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Discard));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_M_StartOrShow));
                    break;
                }
                case UIToolType_Logs:
                {
                    m_pToolBar->addAction(actionPool()->action(UIActionIndex_M_Log_S_Save));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndex_M_Log_T_Find));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndex_M_Log_T_Filter));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndex_M_Log_T_Bookmark));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndex_M_Log_T_Options));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndex_M_Log_S_Refresh));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndex_M_Log_S_Reload));
                    m_pToolBar->addSeparator();
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Settings));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Discard));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_M_StartOrShow));
                    break;
                }
                case UIToolType_VMActivity:
                {
                    m_pToolBar->addAction(actionPool()->action(UIActionIndex_M_Activity_S_Export));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndex_M_Activity_S_ToVMActivityOverview));
                    m_pToolBar->addSeparator();
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Settings));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Discard));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_M_StartOrShow));
                    break;
                }
                case UIToolType_FileManager:
                {
                    m_pToolBar->addAction(m_pActionPool->action(UIActionIndex_M_FileManager_T_Options));
                    m_pToolBar->addAction(m_pActionPool->action(UIActionIndex_M_FileManager_T_Operations));
                    m_pToolBar->addAction(m_pActionPool->action(UIActionIndex_M_FileManager_T_Log));
                    m_pToolBar->addSeparator();
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Settings));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Discard));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_M_StartOrShow));
                    break;
                }
                case UIToolType_Error:
                {
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_New));
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Add));
                    m_pToolBar->addSeparator();
                    m_pToolBar->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Refresh));
                    break;
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }

#ifdef VBOX_WS_MAC
    // WORKAROUND:
    // Actually Qt should do that itself but by some unknown reason it sometimes
    // forget to update toolbar after changing its actions on Cocoa platform.
    connect(actionPool()->action(UIActionIndexMN_M_Machine_S_New), &UIAction::changed,
            m_pToolBar, static_cast<void(QIToolBar::*)(void)>(&QIToolBar::update));
    connect(actionPool()->action(UIActionIndexMN_M_Machine_S_Settings), &UIAction::changed,
            m_pToolBar, static_cast<void(QIToolBar::*)(void)>(&QIToolBar::update));
    connect(actionPool()->action(UIActionIndexMN_M_Machine_S_Discard), &UIAction::changed,
            m_pToolBar, static_cast<void(QIToolBar::*)(void)>(&QIToolBar::update));
    connect(actionPool()->action(UIActionIndexMN_M_Machine_M_StartOrShow), &UIAction::changed,
            m_pToolBar, static_cast<void(QIToolBar::*)(void)>(&QIToolBar::update));

    // WORKAROUND:
    // There is a bug in Qt Cocoa which result in showing a "more arrow" when
    // the necessary size of the toolbar is increased. Also for some languages
    // the with doesn't match if the text increase. So manually adjust the size
    // after changing the text.
    m_pToolBar->updateLayout();
#endif /* VBOX_WS_MAC */
}

void UIVirtualBoxManagerWidget::cleanupConnections()
{
    /* Tool-bar connections: */
    disconnect(m_pToolBar, &QIToolBar::customContextMenuRequested,
               this, &UIVirtualBoxManagerWidget::sltHandleToolBarContextMenuRequest);
    disconnect(m_pToolBar, &QIToolBar::sigResized,
               this, &UIVirtualBoxManagerWidget::sltHandleToolBarResize);

    /* Chooser-pane connections: */
    disconnect(this, &UIVirtualBoxManagerWidget::sigToolBarHeightChange,
               m_pPaneChooser, &UIChooser::setGlobalItemHeightHint);
    disconnect(m_pPaneChooser, &UIChooser::sigSelectionChanged,
               this, &UIVirtualBoxManagerWidget::sltHandleChooserPaneIndexChange);
    disconnect(m_pPaneChooser, &UIChooser::sigSelectionInvalidated,
               this, &UIVirtualBoxManagerWidget::sltHandleChooserPaneSelectionInvalidated);
    disconnect(m_pPaneChooser, &UIChooser::sigToggleStarted,
               m_pPaneToolsMachine, &UIToolPaneMachine::sigToggleStarted);
    disconnect(m_pPaneChooser, &UIChooser::sigToggleFinished,
               m_pPaneToolsMachine, &UIToolPaneMachine::sigToggleFinished);
    disconnect(m_pPaneChooser, &UIChooser::sigGroupSavingStateChanged,
               this, &UIVirtualBoxManagerWidget::sigGroupSavingStateChanged);
    disconnect(m_pPaneChooser, &UIChooser::sigCloudUpdateStateChanged,
               this, &UIVirtualBoxManagerWidget::sigCloudUpdateStateChanged);
    disconnect(m_pPaneChooser, &UIChooser::sigToolMenuRequested,
               this, &UIVirtualBoxManagerWidget::sltHandleToolMenuRequested);
    disconnect(m_pPaneChooser, &UIChooser::sigCloudMachineStateChange,
               this, &UIVirtualBoxManagerWidget::sltHandleCloudMachineStateChange);
    disconnect(m_pPaneChooser, &UIChooser::sigStartOrShowRequest,
               this, &UIVirtualBoxManagerWidget::sigStartOrShowRequest);
    disconnect(m_pPaneChooser, &UIChooser::sigMachineSearchWidgetVisibilityChanged,
               this, &UIVirtualBoxManagerWidget::sigMachineSearchWidgetVisibilityChanged);

    /* Details-pane connections: */
    disconnect(m_pPaneToolsMachine, &UIToolPaneMachine::sigLinkClicked,
               this, &UIVirtualBoxManagerWidget::sigMachineSettingsLinkClicked);

    /* Tools-pane connections: */
    disconnect(m_pPaneTools, &UITools::sigSelectionChanged,
               this, &UIVirtualBoxManagerWidget::sltHandleToolsPaneIndexChange);
}

void UIVirtualBoxManagerWidget::cleanupWidgets()
{
    UINotificationCenter::destroy();
}

void UIVirtualBoxManagerWidget::cleanup()
{
    /* Cleanup everything: */
    cleanupConnections();
    cleanupWidgets();
}

void UIVirtualBoxManagerWidget::recacheCurrentItemInformation(bool fDontRaiseErrorPane /* = false */)
{
    /* Get current item: */
    UIVirtualMachineItem *pItem = currentItem();
    const bool fCurrentItemIsOk = pItem && pItem->accessible();

    /* Update machine tools restrictions: */
    QList<UIToolType> retrictedTypes;
    if (pItem && pItem->itemType() != UIVirtualMachineItemType_Local)
        retrictedTypes << UIToolType_Snapshots << UIToolType_Logs << UIToolType_VMActivity;
    if (retrictedTypes.contains(m_pPaneTools->toolsType()))
        m_pPaneTools->setToolsType(UIToolType_Details);
    m_pPaneTools->setRestrictedToolTypes(retrictedTypes);
    /* Update machine tools availability: */
    m_pPaneTools->setToolClassEnabled(UIToolClass_Machine, fCurrentItemIsOk);

    /* Take restrictions into account, closing all restricted tools: */
    foreach (const UIToolType &enmRestrictedType, retrictedTypes)
        m_pPaneToolsMachine->closeTool(enmRestrictedType);

    /* Propagate current item anyway: */
    m_pPaneToolsMachine->setCurrentItem(pItem);

    /* If current item is Ok: */
    if (fCurrentItemIsOk)
    {
        /* If Error-pane is chosen currently => open tool currently chosen in Tools-pane: */
        if (m_pPaneToolsMachine->currentTool() == UIToolType_Error)
            sltHandleToolsPaneIndexChange();
    }
    else
    {
        /* If we were not asked separately: */
        if (!fDontRaiseErrorPane)
        {
            /* Make sure Error pane raised: */
            m_pPaneToolsMachine->openTool(UIToolType_Error);

            /* Propagate last access error to update the Error-pane (if machine selected but inaccessible): */
            if (pItem)
                m_pPaneToolsMachine->setErrorDetails(pItem->accessError());
        }
    }

    /* Propagate current items to update the Details-pane: */
    m_pPaneToolsMachine->setItems(currentItems());
}
