/* $Id: UIVMInformationDialog.h $ */
/** @file
 * VBox Qt GUI - UIVMInformationDialog class declaration.
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

#ifndef FEQT_INCLUDED_SRC_runtime_information_UIVMInformationDialog_h
#define FEQT_INCLUDED_SRC_runtime_information_UIVMInformationDialog_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QMainWindow>

/* GUI includes: */
#include "QIWithRestorableGeometry.h"
#include "QIWithRetranslateUI.h"

/* COM includes: */
#include "COMEnums.h"
#include "CSession.h"

/* Forward declarations: */
class QITabWidget;
class QIDialogButtonBox;
class UIMachineWindow;

/* Type definitions: */
typedef QIWithRestorableGeometry<QMainWindow> QMainWindowWithRestorableGeometry;
typedef QIWithRetranslateUI<QMainWindowWithRestorableGeometry> QMainWindowWithRestorableGeometryAndRetranslateUi;


/** QMainWindow subclass providing user
  * with the dialog unifying VM details and statistics. */
class UIVMInformationDialog : public QMainWindowWithRestorableGeometryAndRetranslateUi
{
    Q_OBJECT;

signals:

    void sigClose();

public:

    /** Constructs information dialog for passed @a pMachineWindow. */
    UIVMInformationDialog(UIMachineWindow *pMachineWindow);
    /** Destructs information dialog. */
    ~UIVMInformationDialog();

    /** Returns whether the dialog should be maximized when geometry being restored. */
    virtual bool shouldBeMaximized() const RT_OVERRIDE;

protected:

    /** Handles translation event. */
    virtual void retranslateUi() RT_OVERRIDE;
    virtual void closeEvent(QCloseEvent *pEvent) RT_OVERRIDE;
    virtual bool event(QEvent *pEvent) RT_OVERRIDE;

private slots:

    /** Handles tab-widget page change. */
    void sltHandlePageChanged(int iIndex);
    void sltMachineStateChange(const QUuid &uMachineId, const KMachineState state);

private:
    enum Tabs
    {
        Tabs_ConfigurationDetails = 0,
        Tabs_RuntimeInformation,
        Tabs_ActivityMonitor,
        Tabs_GuestControl
    };
    /** Prepares all. */
    void prepare();
    /** Prepares this. */
    void prepareThis();
    /** Prepares central-widget. */
    void prepareCentralWidget();
    /** Prepares tab-widget. */
    void prepareTabWidget();
    /** Prepares tab with @a iTabIndex. */
    void prepareTab(int iTabIndex);
    /** Prepares button-box. */
    void prepareButtonBox();
    void loadDialogGeometry();
    void saveDialogGeometry();

    /** @name Widget variables.
     * @{ */
       /** Holds the dialog tab-widget instance. */
       QITabWidget                  *m_pTabWidget;
       /** Holds the map of dialog tab instances. */
       QMap<int, QWidget*>           m_tabs;
       /** Holds the dialog button-box instance. */
       QIDialogButtonBox            *m_pButtonBox;
       /** Holds the machine-window reference. */
       UIMachineWindow              *m_pMachineWindow;
    /** @} */
    bool m_fCloseEmitted;
    int m_iGeometrySaveTimerId;
    QUuid m_uMachineId;
};

#endif /* !FEQT_INCLUDED_SRC_runtime_information_UIVMInformationDialog_h */
