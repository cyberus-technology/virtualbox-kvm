/* $Id: UIVMActivityToolWidget.h $ */
/** @file
 * VBox Qt GUI - UIVMActivityToolWidget class declaration.
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

#ifndef FEQT_INCLUDED_SRC_activity_overview_UIVMActivityToolWidget_h
#define FEQT_INCLUDED_SRC_activity_overview_UIVMActivityToolWidget_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QTabWidget>
#include <QUuid>

/* GUI includes: */
#include "QIManagerDialog.h"
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class UIActionPool;
class QIToolBar;
class UIVirtualMachineItem;
class CMachine;

/** QTabWidget extension host machine activity widget(s) in the Manager UI. */
class UIVMActivityToolWidget : public QIWithRetranslateUI<QTabWidget>
{
    Q_OBJECT;

signals:

    void sigSwitchToActivityOverviewPane();

public:

    UIVMActivityToolWidget(EmbedTo enmEmbedding, UIActionPool *pActionPool,
                               bool fShowToolbar = true, QWidget *pParent = 0);
    QMenu *menu() const;

    bool isCurrentTool() const;
    void setIsCurrentTool(bool fIsCurrentTool);

    void setSelectedVMListItems(const QList<UIVirtualMachineItem*> &items);

#ifdef VBOX_WS_MAC
    QIToolBar *toolbar() const { return m_pToolBar; }
#endif

protected:

    /** @name Event-handling stuff.
      * @{ */
        virtual void retranslateUi() RT_OVERRIDE;
        virtual void showEvent(QShowEvent *pEvent) RT_OVERRIDE;
    /** @} */

private slots:

    void sltExportToFile();
    void sltCurrentTabChanged(int iIndex);

private:

    void setMachines(const QVector<QUuid> &machineIDs);
    /** @name Prepare/cleanup cascade.
      * @{ */
        void prepare();
        void prepareToolBar();
        void prepareActions();
        void updateColumnsMenu();
        void loadSettings();
    /** @} */

    /** Remove tabs conaining machine monitors with ids @machineIdsToRemove. */
    void removeTabs(const QVector<QUuid> &machineIdsToRemove);
    /** Add new tabs for each QUuid in @machineIdsToAdd. Does not check for duplicates. */
    void addTabs(const QVector<QUuid> &machineIdsToAdd);
    void setExportActionEnabled(bool fEnabled);

    /** @name General variables.
      * @{ */
        const EmbedTo m_enmEmbedding;
        UIActionPool *m_pActionPool;
        const bool    m_fShowToolbar;
    /** @} */

    QIToolBar *m_pToolBar;

    /** Indicates if this widget's host tool is current tool. */
    bool    m_fIsCurrentTool;
    QVector<QUuid> m_machineIds;
    QAction *m_pExportToFileAction;
};


#endif /* !FEQT_INCLUDED_SRC_activity_overview_UIVMActivityToolWidget_h */
