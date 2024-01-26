/* $Id: UIWizardNewCloudVMPageExpert.h $ */
/** @file
 * VBox Qt GUI - UIWizardNewCloudVMPageExpert class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_newcloudvm_UIWizardNewCloudVMPageExpert_h
#define FEQT_INCLUDED_SRC_wizards_newcloudvm_UIWizardNewCloudVMPageExpert_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIWizardNewCloudVMPageSource.h"
#include "UIWizardNewCloudVMPageProperties.h"

/* Forward declarations: */
class UIToolBox;
class UIWizardNewCloudVM;

/** UINativeWizardPage extension for Expert page of the New Cloud VM wizard,
  * based on UIWizardNewCloudVMPage1 & UIWizardNewCloudVMPage2 namespace functions. */
class UIWizardNewCloudVMPageExpert : public UINativeWizardPage
{
    Q_OBJECT;

public:

    /** Constructs expert page. */
    UIWizardNewCloudVMPageExpert();

protected:

    /** Returns wizard this page belongs to. */
    UIWizardNewCloudVM *wizard() const;

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

    /** Handles change in source tab-bar. */
    void sltHandleSourceTabBarChange();

    /** Handles change in instance list. */
    void sltHandleSourceImageChange();

private:

    /** Updates properties table. */
    void updatePropertiesTable();

    /** Holds whether we want full wizard form or short one. */
    bool     m_fFullWizard;
    /** Holds the image ID. */
    QString  m_strSourceImageId;

    /** Holds the tool-box instance. */
    UIToolBox *m_pToolBox;

    /** Holds the location type combo-box instance. */
    QIComboBox   *m_pProviderComboBox;
    /** Holds the profile combo-box instance. */
    QIComboBox   *m_pProfileComboBox;
    /** Holds the profile management tool-button instance. */
    QIToolButton *m_pProfileToolButton;

    /** Holds the source tab-bar instance. */
    QTabBar      *m_pSourceTabBar;
    /** Holds the source image list instance. */
    QListWidget  *m_pSourceImageList;

    /** Holds the Form Editor widget instance. */
    UIFormEditorWidget *m_pFormEditor;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_newcloudvm_UIWizardNewCloudVMPageExpert_h */
