/* $Id: UIVMLogViewerDialog.h $ */
/** @file
 * VBox Qt GUI - UIVMLogViewerDialog class declaration.
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

#ifndef FEQT_INCLUDED_SRC_logviewer_UIVMLogViewerDialog_h
#define FEQT_INCLUDED_SRC_logviewer_UIVMLogViewerDialog_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QMap>
#include <QString>

/* GUI includes: */
#include "QIManagerDialog.h"
#include "QIWithRetranslateUI.h"
#include "UILibraryDefs.h"

/* COM includes: */
#include "COMEnums.h"
#include "CMachine.h"

/* Forward declarations: */
class QDialogButtonBox;
class QVBoxLayout;
class UIActionPool;
class UIVMLogViewerDialog;
class UIVirtualMachineItem;
class CMachine;


/** QIManagerDialogFactory extension used as a factory for Log Viewer dialog. */
class SHARED_LIBRARY_STUFF UIVMLogViewerDialogFactory : public QIManagerDialogFactory
{
public:

    /** Constructs Log Viewer factory acquiring additional arguments.
      * @param  pActionPool  Brings the action-pool reference.
      * @param  uMachineId   Brings the machine id for which VM Log-Viewer is requested. */
    UIVMLogViewerDialogFactory(UIActionPool *pActionPool = 0, const QUuid &uMachineId = QUuid(),
                               const QString &strMachineName = QString());

protected:

    /** Creates derived @a pDialog instance.
      * @param  pCenterWidget  Brings the widget to center wrt. pCenterWidget. */
    virtual void create(QIManagerDialog *&pDialog, QWidget *pCenterWidget) RT_OVERRIDE;

    /** Holds the action-pool reference. */
    UIActionPool *m_pActionPool;
    /** Holds the machine id. */
    QUuid      m_uMachineId;
    QString    m_strMachineName;
};


/** QIManagerDialog extension providing GUI with the dialog displaying machine logs. */
class SHARED_LIBRARY_STUFF UIVMLogViewerDialog : public QIWithRetranslateUI<QIManagerDialog>
{
    Q_OBJECT;

public:

    /** Constructs Log Viewer dialog.
      * @param  pCenterWidget  Brings the widget reference to center according to.
      * @param  pActionPool    Brings the action-pool reference.
      * @param  machine id     Brings the machine id. */
    UIVMLogViewerDialog(QWidget *pCenterWidget, UIActionPool *pActionPool,
                        const QUuid &uMachineId = QUuid(), const QString &strMachineName = QString());
    ~UIVMLogViewerDialog();
    void setSelectedVMListItems(const QList<UIVirtualMachineItem*> &items);
    void addSelectedVMListItems(const QList<UIVirtualMachineItem*> &items);

protected:

    /** @name Event-handling stuff.
      * @{ */
        /** Handles translation event. */
        virtual void retranslateUi() RT_OVERRIDE;
        virtual bool event(QEvent *pEvent) RT_OVERRIDE;
    /** @} */

    /** @name Prepare/cleanup cascade.
     * @{ */
        /** Configures all. */
        virtual void configure() RT_OVERRIDE;
        /** Configures central-widget. */
        virtual void configureCentralWidget() RT_OVERRIDE;
        /** Perform final preparations. */
        virtual void finalize() RT_OVERRIDE;
        /** Loads dialog geometry from extradata. */
        virtual void loadDialogGeometry();

        /** Saves dialog geometry into extradata. */
        virtual void saveDialogGeometry();
    /** @} */

    /** @name Functions related to geometry restoration.
     * @{ */
        /** Returns whether the window should be maximized when geometry being restored. */
        virtual bool shouldBeMaximized() const RT_OVERRIDE;
    /** @} */

private slots:

    /** Must be handles soemthing related to close @a shortcut. */
    void sltSetCloseButtonShortCut(QKeySequence shortcut);

private:

    void manageEscapeShortCut();
    /** Holds the action-pool reference. */
    UIActionPool *m_pActionPool;
    /** Holds the machine id. */
    QUuid      m_uMachineId;
    int m_iGeometrySaveTimerId;
    QString    m_strMachineName;
};


#endif /* !FEQT_INCLUDED_SRC_logviewer_UIVMLogViewerDialog_h */
