/* $Id: UIWizardAddCloudVMPageExpert.h $ */
/** @file
 * VBox Qt GUI - UIWizardAddCloudVMPageExpert class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_addcloudvm_UIWizardAddCloudVMPageExpert_h
#define FEQT_INCLUDED_SRC_wizards_addcloudvm_UIWizardAddCloudVMPageExpert_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIWizardAddCloudVMPageSource.h"

/* Forward declarations: */
class UIToolBox;
class UIWizardAddCloudVM;

/** UINativeWizardPage extension for Expert page of the Add Cloud VM wizard,
  * based on UIWizardAddCloudVMPage1 namespace functions. */
class UIWizardAddCloudVMPageExpert : public UINativeWizardPage
{
    Q_OBJECT;

public:

    /** Constructs expert page. */
    UIWizardAddCloudVMPageExpert();

protected:

    /** Returns wizard this page belongs to. */
    UIWizardAddCloudVM *wizard() const;

    /** Handles translation event. */
    virtual void retranslateUi() /* override final */;

    /** Performs page initialization. */
    virtual void initializePage() /* override final */;

    /** Returns whether page is complete. */
    virtual bool isComplete() const /* override final */;

    /** Performs page validation. */
    virtual bool validatePage() /* override final */;

private slots:

    /** Handles change in provider combo-box. */
    void sltHandleProviderComboChange();

    /** Handles change in profile combo-box. */
    void sltHandleProfileComboChange();
    /** Handles profile tool-button click. */
    void sltHandleProfileButtonClick();

    /** Handles change in instance list. */
    void sltHandleSourceInstanceChange();

private:

    /** Holds the tool-box instance. */
    UIToolBox *m_pToolBox;

    /** Holds the provider type label instance. */
    QLabel      *m_pProviderLabel;
    /** Holds the provider type combo-box instance. */
    QIComboBox  *m_pProviderComboBox;

    /** Holds the profile label instance. */
    QLabel       *m_pProfileLabel;
    /** Holds the profile combo-box instance. */
    QIComboBox   *m_pProfileComboBox;
    /** Holds the profile management tool-button instance. */
    QIToolButton *m_pProfileToolButton;

    /** Holds the source instance label instance. */
    QLabel       *m_pSourceInstanceLabel;
    /** Holds the source instance list instance. */
    QListWidget  *m_pSourceInstanceList;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_addcloudvm_UIWizardAddCloudVMPageExpert_h */
