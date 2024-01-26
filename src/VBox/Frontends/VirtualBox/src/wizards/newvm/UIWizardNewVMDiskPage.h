/* $Id: UIWizardNewVMDiskPage.h $ */
/** @file
 * VBox Qt GUI - UIWizardNewVMDiskPage class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_newvm_UIWizardNewVMDiskPage_h
#define FEQT_INCLUDED_SRC_wizards_newvm_UIWizardNewVMDiskPage_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QSet>

/* GUI includes: */
#include "QIFileDialog.h"
#include "UINativeWizardPage.h"

/* COM includes: */
#include "COMEnums.h"
#include "CMedium.h"

/* Forward declarations: */
class QButtonGroup;
class QCheckBox;
class QRadioButton;
class QLabel;
class QIRichTextLabel;
class QIToolButton;
class UIActionPool;
class UIMediaComboBox;
class UIMediumSizeEditor;

namespace UIWizardNewVMDiskCommon
{
    QUuid getWithFileOpenDialog(const QString &strOSTypeID,
                                const QString &strMachineFolder,
                                QWidget *pCaller, UIActionPool *pActionPool);
}


class UIWizardNewVMDiskPage : public UINativeWizardPage
{
    Q_OBJECT;

public:

    UIWizardNewVMDiskPage(UIActionPool *pActionPool);

protected:


private slots:

    void sltSelectedDiskSourceChanged();
    void sltMediaComboBoxIndexChanged();
    void sltGetWithFileOpenDialog();
    void sltHandleSizeEditorChange(qulonglong uSize);
    void sltFixedCheckBoxToggled(bool fChecked);

private:

    void prepare();
    void createConnections();
    QWidget *createNewDiskWidgets();
    void setEnableNewDiskWidgets(bool fEnable);
    QWidget *createDiskWidgets();
    QWidget *createMediumVariantWidgets(bool fWithLabels);

    virtual void retranslateUi() /* override final */;
    virtual void initializePage() /* override final */;
    virtual bool isComplete() const /* override final */;

    void setEnableDiskSelectionWidgets(bool fEnabled);
    void setWidgetVisibility(const CMediumFormat &mediumFormat);

    /** @name Widgets
     * @{ */
       QButtonGroup *m_pDiskSourceButtonGroup;
       QRadioButton *m_pDiskEmpty;
       QRadioButton *m_pDiskNew;
       QRadioButton *m_pDiskExisting;
       UIMediaComboBox *m_pDiskSelector;
       QIToolButton *m_pDiskSelectionButton;
       QIRichTextLabel *m_pLabel;
       QLabel          *m_pMediumSizeEditorLabel;
       UIMediumSizeEditor *m_pMediumSizeEditor;
       QIRichTextLabel *m_pDescriptionLabel;
       QIRichTextLabel *m_pDynamicLabel;
       QIRichTextLabel *m_pFixedLabel;
       QCheckBox *m_pFixedCheckBox;
    /** @} */

    /** @name Variables
      * @{ */
        QSet<QString> m_userModifiedParameters;
        bool m_fVDIFormatFound;
        qulonglong m_uMediumSizeMin;
        qulonglong m_uMediumSizeMax;
    /** @} */
    UIActionPool *m_pActionPool;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_newvm_UIWizardNewVMDiskPage_h */
