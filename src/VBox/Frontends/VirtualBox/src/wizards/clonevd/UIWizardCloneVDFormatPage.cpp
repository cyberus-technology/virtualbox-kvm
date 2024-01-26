/* $Id: UIWizardCloneVDFormatPage.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVDFormatPage class implementation.
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
#include <QVBoxLayout>

/* GUI includes: */
#include "UIWizardCloneVDFormatPage.h"
#include "UIWizardCloneVD.h"
#include "UIWizardDiskEditors.h"
#include "UICommon.h"
#include "QIRichTextLabel.h"

/* COM includes: */
#include "CSystemProperties.h"

UIWizardCloneVDFormatPage::UIWizardCloneVDFormatPage(KDeviceType enmDeviceType)
    : m_pLabel(0)
    , m_pFormatGroupBox(0)
{
    prepare(enmDeviceType);
}

void UIWizardCloneVDFormatPage::prepare(KDeviceType enmDeviceType)
{
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    m_pLabel = new QIRichTextLabel(this);
    if (m_pLabel)
        pMainLayout->addWidget(m_pLabel);
    m_pFormatGroupBox = new UIDiskFormatsGroupBox(false /* expert mode */, enmDeviceType, 0);
    if (m_pFormatGroupBox)
    {
        pMainLayout->addWidget(m_pFormatGroupBox);
        connect(m_pFormatGroupBox, &UIDiskFormatsGroupBox::sigMediumFormatChanged,
                this, &UIWizardCloneVDFormatPage::sltMediumFormatChanged);
    }
    pMainLayout->addStretch();
    retranslateUi();
}

void UIWizardCloneVDFormatPage::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardCloneVD::tr("Virtual Hard disk file type"));

    /* Translate widgets: */
    m_pLabel->setText(UIWizardCloneVD::tr("Please choose the type of file that you would like to use "
                                          "for the destination virtual disk image. If you do not need to use it "
                                          "with other virtualization software you can leave this setting unchanged."));
}

void UIWizardCloneVDFormatPage::initializePage()
{
    AssertReturnVoid(wizardWindow<UIWizardCloneVD>());
    /* Translate page: */
    retranslateUi();
    if (!m_userModifiedParameters.contains("MediumFormat"))
    {
        if (m_pFormatGroupBox)
            wizardWindow<UIWizardCloneVD>()->setMediumFormat(m_pFormatGroupBox->mediumFormat());
    }
}

bool UIWizardCloneVDFormatPage::isComplete() const
{
    if (m_pFormatGroupBox)
    {
        if (m_pFormatGroupBox->mediumFormat().isNull())
            return false;
    }
    return true;
}

void UIWizardCloneVDFormatPage::sltMediumFormatChanged()
{
    AssertReturnVoid(wizardWindow<UIWizardCloneVD>());
    if (m_pFormatGroupBox)
        wizardWindow<UIWizardCloneVD>()->setMediumFormat(m_pFormatGroupBox->mediumFormat());
    m_userModifiedParameters << "MediumFormat";
    emit completeChanged();
}
