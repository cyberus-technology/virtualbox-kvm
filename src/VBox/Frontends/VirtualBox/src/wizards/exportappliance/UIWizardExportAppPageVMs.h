/* $Id: UIWizardExportAppPageVMs.h $ */
/** @file
 * VBox Qt GUI - UIWizardExportAppPageVMs class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_exportappliance_UIWizardExportAppPageVMs_h
#define FEQT_INCLUDED_SRC_wizards_exportappliance_UIWizardExportAppPageVMs_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes */
#include <QUuid>

/* GUI includes: */
#include "UINativeWizardPage.h"

/* Forward declarations: */
class QListWidget;
class QIRichTextLabel;
class UIWizardExportApp;

/** Namespace for VMs page of the Export Appliance wizard. */
namespace UIWizardExportAppVMs
{
    /** Populates @a pVMSelector with items on the basis of passed @a selectedVMNames. */
    void populateVMItems(QListWidget *pVMSelector, const QStringList &selectedVMNames);

    /** Refresh a list of saved machines selected in @a pVMSelector. */
    void refreshSavedMachines(QStringList &savedMachines, QListWidget *pVMSelector);

    /** Returns a list of machine names selected in @a pVMSelector. */
    QStringList machineNames(QListWidget *pVMSelector);
    /** Returns a list of machine IDs selected in @a pVMSelector. */
    QList<QUuid> machineIDs(QListWidget *pVMSelector);
}

/** UINativeWizardPage extension for VMs page of the Export Appliance wizard,
  * based on UIWizardExportAppVMs namespace functions. */
class UIWizardExportAppPageVMs : public UINativeWizardPage
{
    Q_OBJECT;

public:

    /** Constructs VMs page.
      * @param  selectedVMNames        Brings the list of selected VM names.
      * @param  fFastTravelToNextPage  Brings whether we should fast-travel to next page. */
    UIWizardExportAppPageVMs(const QStringList &selectedVMNames, bool fFastTravelToNextPage);

protected:

    /** Returns wizard this page belongs to. */
    UIWizardExportApp *wizard() const;

    /** Handles translation event. */
    virtual void retranslateUi() /* override final */;

    /** Performs page initialization. */
    virtual void initializePage() /* override final */;

    /** Returns whether page is complete. */
    virtual bool isComplete() const /* override final */;

    /** Performs page validation. */
    virtual bool validatePage() /* override final */;

private slots:

    /** Handles VM item selection change. */
    void sltHandleVMItemSelectionChanged();

private:

    /** Holds the list of selected VM names. */
    const QStringList  m_selectedVMNames;
    /** Holds whether we should fast travel to next page. */
    bool               m_fFastTravelToNextPage;

    /** Holds the main label instance. */
    QIRichTextLabel *m_pLabelMain;

    /** Holds the VM selector instance. */
    QListWidget *m_pVMSelector;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_exportappliance_UIWizardExportAppPageVMs_h */
