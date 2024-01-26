/* $Id: UIWizardCloneVDVariantPage.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVDVariantPage class implementation.
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
#include "UIWizardDiskEditors.h"
#include "UIWizardCloneVDVariantPage.h"
#include "UIWizardCloneVD.h"
#include "QIRichTextLabel.h"

/* COM includes: */
#include "CMediumFormat.h"

UIWizardCloneVDVariantPage::UIWizardCloneVDVariantPage()
    : m_pDescriptionLabel(0)
    , m_pDynamicLabel(0)
    , m_pFixedLabel(0)
    , m_pSplitLabel(0)
    , m_pVariantWidget(0)
{
    prepare();
}

void UIWizardCloneVDVariantPage::prepare()
{
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);

    m_pDescriptionLabel = new QIRichTextLabel(this);
    if (m_pDescriptionLabel)
        pMainLayout->addWidget(m_pDescriptionLabel);

    m_pDynamicLabel = new QIRichTextLabel(this);
    if (m_pDynamicLabel)
        pMainLayout->addWidget(m_pDynamicLabel);

    m_pFixedLabel = new QIRichTextLabel(this);
    if (m_pFixedLabel)
        pMainLayout->addWidget(m_pFixedLabel);

    m_pSplitLabel = new QIRichTextLabel(this);
    if (m_pSplitLabel)
        pMainLayout->addWidget(m_pSplitLabel);

    m_pVariantWidget = new UIDiskVariantWidget(0);
    if (m_pVariantWidget)
    {
        pMainLayout->addWidget(m_pVariantWidget);
        connect(m_pVariantWidget, &UIDiskVariantWidget::sigMediumVariantChanged,
                this, &UIWizardCloneVDVariantPage::sltMediumVariantChanged);

    }
    retranslateUi();
}


void UIWizardCloneVDVariantPage::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardCloneVD::tr("Storage on physical hard disk"));

    /* Translate widgets: */
    m_pDescriptionLabel->setText(UIWizardCloneVD::tr("Please choose whether the new virtual disk image file should grow as it is used "
                                                     "(dynamically allocated) or if it should be created at its maximum size (fixed size)."));
    m_pDynamicLabel->setText(UIWizardCloneVD::tr("<p>A <b>dynamically allocated</b> disk image file will only use space "
                                                 "on your physical hard disk as it fills up (up to a maximum <b>fixed size</b>), "
                                                 "although it will not shrink again automatically when space on it is freed.</p>"));
    m_pFixedLabel->setText(UIWizardCloneVD::tr("<p>A <b>fixed size</b> disk image file may take longer to create on some "
                                               "systems but is often faster to use.</p>"));
    m_pSplitLabel->setText(UIWizardCloneVD::tr("<p>You can also choose to <b>split</b> the disk image file into several files "
                                               "of up to two gigabytes each. This is mainly useful if you wish to store the "
                                               "virtual machine on removable USB devices or old systems, some of which cannot "
                                               "handle very large files."));
}

void UIWizardCloneVDVariantPage::initializePage()
{
    AssertReturnVoid(wizardWindow<UIWizardCloneVD>());
    /* Translate page: */
    retranslateUi();

    setWidgetVisibility(wizardWindow<UIWizardCloneVD>()->mediumFormat());
    if (m_pVariantWidget)
        wizardWindow<UIWizardCloneVD>()->setMediumVariant(m_pVariantWidget->mediumVariant());
}

bool UIWizardCloneVDVariantPage::isComplete() const
{
    AssertReturn(m_pVariantWidget, false);
    return m_pVariantWidget->isComplete();
}

void UIWizardCloneVDVariantPage::setWidgetVisibility(const CMediumFormat &mediumFormat)
{
    AssertReturnVoid(m_pVariantWidget);

    m_pVariantWidget->updateMediumVariantWidgetsAfterFormatChange(mediumFormat);

    if (m_pDynamicLabel)
        m_pDynamicLabel->setHidden(!m_pVariantWidget->isCreateDynamicPossible());
    if (m_pFixedLabel)
        m_pFixedLabel->setHidden(!m_pVariantWidget->isCreateFixedPossible());
    if (m_pSplitLabel)
        m_pSplitLabel->setHidden(!m_pVariantWidget->isCreateSplitPossible());
}

void UIWizardCloneVDVariantPage::sltMediumVariantChanged(qulonglong uVariant)
{
    if (wizardWindow<UIWizardCloneVD>())
        wizardWindow<UIWizardCloneVD>()->setMediumVariant(uVariant);
}
