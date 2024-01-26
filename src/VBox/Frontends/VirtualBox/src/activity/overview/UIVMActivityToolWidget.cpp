/* $Id: UIVMActivityToolWidget.cpp $ */
/** @file
 * VBox Qt GUI - UIVMActivityToolWidget class implementation.
 */

/*
 * Copyright (C) 2009-2023 Oracle and/or its affiliates.
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
#include <QStyle>

/* GUI includes: */
#include "UIActionPoolManager.h"
#include "UICommon.h"
#include "UIVMActivityMonitor.h"
#include "UIVMActivityToolWidget.h"
#include "UIMessageCenter.h"
#include "QIToolBar.h"
#include "UIVirtualMachineItem.h"

#ifdef VBOX_WS_MAC
# include "UIWindowMenuManager.h"
#endif /* VBOX_WS_MAC */

/* COM includes: */
#include "COMEnums.h"
#include "CMachine.h"


UIVMActivityToolWidget::UIVMActivityToolWidget(EmbedTo enmEmbedding, UIActionPool *pActionPool,
                                                 bool fShowToolbar /* = true */, QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QTabWidget>(pParent)
    , m_enmEmbedding(enmEmbedding)
    , m_pActionPool(pActionPool)
    , m_fShowToolbar(fShowToolbar)
    , m_pToolBar(0)
    , m_pExportToFileAction(0)
{
    setTabPosition(QTabWidget::East);
    prepare();
    prepareActions();
    prepareToolBar();
    sltCurrentTabChanged(0);
}

QMenu *UIVMActivityToolWidget::menu() const
{
    return NULL;
}

bool UIVMActivityToolWidget::isCurrentTool() const
{
    return m_fIsCurrentTool;
}

void UIVMActivityToolWidget::setIsCurrentTool(bool fIsCurrentTool)
{
    m_fIsCurrentTool = fIsCurrentTool;
}

void UIVMActivityToolWidget::retranslateUi()
{
}

void UIVMActivityToolWidget::showEvent(QShowEvent *pEvent)
{
    QIWithRetranslateUI<QTabWidget>::showEvent(pEvent);
}

void UIVMActivityToolWidget::prepare()
{
    setTabBarAutoHide(true);
    setLayout(new QHBoxLayout);

    connect(this, &UIVMActivityToolWidget::currentChanged,
            this, &UIVMActivityToolWidget::sltCurrentTabChanged);
}

void UIVMActivityToolWidget::setSelectedVMListItems(const QList<UIVirtualMachineItem*> &items)
{
    QVector<QUuid> selectedMachines;

    foreach (const UIVirtualMachineItem *item, items)
    {
        if (!item)
            continue;
        selectedMachines << item->id();
    }
    setMachines(selectedMachines);
}

void UIVMActivityToolWidget::setMachines(const QVector<QUuid> &machineIds)
{
    /* List of machines that are newly added to selected machine list: */
    QVector<QUuid> newSelections;
    QVector<QUuid> unselectedMachines(m_machineIds);

    foreach (const QUuid &id, machineIds)
    {
        unselectedMachines.removeAll(id);
        if (!m_machineIds.contains(id))
            newSelections << id;
    }
    m_machineIds = machineIds;

    removeTabs(unselectedMachines);
    addTabs(newSelections);
}

void UIVMActivityToolWidget::prepareActions()
{
    QAction *pToResourcesAction =
        m_pActionPool->action(UIActionIndex_M_Activity_S_ToVMActivityOverview);
    if (pToResourcesAction)
        connect(pToResourcesAction, &QAction::triggered, this, &UIVMActivityToolWidget::sigSwitchToActivityOverviewPane);

    m_pExportToFileAction =
        m_pActionPool->action(UIActionIndex_M_Activity_S_Export);
    if (m_pExportToFileAction)
        connect(m_pExportToFileAction, &QAction::triggered, this, &UIVMActivityToolWidget::sltExportToFile);
}

void UIVMActivityToolWidget::prepareToolBar()
{
    /* Create toolbar: */
    m_pToolBar = new QIToolBar(parentWidget());
    AssertPtrReturnVoid(m_pToolBar);
    {
        /* Configure toolbar: */
        const int iIconMetric = (int)(QApplication::style()->pixelMetric(QStyle::PM_LargeIconSize));
        m_pToolBar->setIconSize(QSize(iIconMetric, iIconMetric));
        m_pToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

#ifdef VBOX_WS_MAC
        /* Check whether we are embedded into a stack: */
        if (m_enmEmbedding == EmbedTo_Stack)
        {
            /* Add into layout: */
            layout()->addWidget(m_pToolBar);
        }
#else
        /* Add into layout: */
        layout()->addWidget(m_pToolBar);
#endif
    }
}

void UIVMActivityToolWidget::loadSettings()
{
}

void UIVMActivityToolWidget::removeTabs(const QVector<QUuid> &machineIdsToRemove)
{
    QVector<UIVMActivityMonitor*> removeList;

    for (int i = count() - 1; i >= 0; --i)
    {
        UIVMActivityMonitor *pMonitor = qobject_cast<UIVMActivityMonitor*>(widget(i));
        if (!pMonitor)
            continue;
        if (machineIdsToRemove.contains(pMonitor->machineId()))
        {
            removeList << pMonitor;
            removeTab(i);
        }
    }
    qDeleteAll(removeList.begin(), removeList.end());
}

void UIVMActivityToolWidget::addTabs(const QVector<QUuid> &machineIdsToAdd)
{
    foreach (const QUuid &id, machineIdsToAdd)
    {
        CMachine comMachine = uiCommon().virtualBox().FindMachine(id.toString());
        if (comMachine.isNull())
            continue;
        addTab(new UIVMActivityMonitor(m_enmEmbedding, this, comMachine), comMachine.GetName());
    }
}

void UIVMActivityToolWidget::sltExportToFile()
{
    UIVMActivityMonitor *pActivityMonitor = qobject_cast<UIVMActivityMonitor*>(currentWidget());
    if (pActivityMonitor)
        pActivityMonitor->sltExportMetricsToFile();
}

void UIVMActivityToolWidget::sltCurrentTabChanged(int iIndex)
{
    Q_UNUSED(iIndex);
    UIVMActivityMonitor *pActivityMonitor = qobject_cast<UIVMActivityMonitor*>(currentWidget());
    if (pActivityMonitor)
    {
        CMachine comMachine = uiCommon().virtualBox().FindMachine(pActivityMonitor->machineId().toString());
        if (!comMachine.isNull())
        {
            setExportActionEnabled(comMachine.GetState() == KMachineState_Running);
        }
    }
}

void UIVMActivityToolWidget::setExportActionEnabled(bool fEnabled)
{
    if (m_pExportToFileAction)
        m_pExportToFileAction->setEnabled(fEnabled);
}
