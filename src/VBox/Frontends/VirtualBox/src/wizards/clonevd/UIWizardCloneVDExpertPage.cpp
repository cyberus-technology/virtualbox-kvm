/* $Id: UIWizardCloneVDExpertPage.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVDExpertPage class implementation.
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
#include <QDir>
#include <QGridLayout>

/* GUI includes: */
#include "UICommon.h"
#include "UINotificationCenter.h"
#include "UIWizardCloneVD.h"
#include "UIWizardCloneVDExpertPage.h"
#include "UIWizardDiskEditors.h"

/* COM includes: */
#include "CSystemProperties.h"

UIWizardCloneVDExpertPage::UIWizardCloneVDExpertPage(KDeviceType enmDeviceType, qulonglong uSourceDiskLogicaSize)
    : m_pFormatComboBox(0)
    , m_pVariantWidget(0)
    , m_pMediumSizePathGroupBox(0)
    , m_pFormatVariantGroupBox(0)
    , m_enmDeviceType(enmDeviceType)
{
    prepare(enmDeviceType, uSourceDiskLogicaSize);
}

void UIWizardCloneVDExpertPage::prepare(KDeviceType enmDeviceType, qulonglong uSourceDiskLogicaSize)
{
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);

    m_pMediumSizePathGroupBox = new UIMediumSizeAndPathGroupBox(true /* expert mode */, 0 /* parent */, uSourceDiskLogicaSize);

    if (m_pMediumSizePathGroupBox)
    {
        pMainLayout->addWidget(m_pMediumSizePathGroupBox);
        connect(m_pMediumSizePathGroupBox, &UIMediumSizeAndPathGroupBox::sigMediumLocationButtonClicked,
                this, &UIWizardCloneVDExpertPage::sltSelectLocationButtonClicked);
        connect(m_pMediumSizePathGroupBox, &UIMediumSizeAndPathGroupBox::sigMediumPathChanged,
                this, &UIWizardCloneVDExpertPage::sltMediumPathChanged);
        connect(m_pMediumSizePathGroupBox, &UIMediumSizeAndPathGroupBox::sigMediumSizeChanged,
                this, &UIWizardCloneVDExpertPage::sltMediumSizeChanged);
    }

    m_pFormatComboBox = new UIDiskFormatsComboBox(true /* expert mode */, enmDeviceType, 0);
    if (m_pFormatComboBox)
        connect(m_pFormatComboBox, &UIDiskFormatsComboBox::sigMediumFormatChanged,
                this, &UIWizardCloneVDExpertPage::sltMediumFormatChanged);

    m_pVariantWidget = new UIDiskVariantWidget(0);
    if (m_pVariantWidget)
        connect(m_pVariantWidget, &UIDiskVariantWidget::sigMediumVariantChanged,
                this, &UIWizardCloneVDExpertPage::sltMediumVariantChanged);

    m_pFormatVariantGroupBox = new QGroupBox;
    if (m_pFormatVariantGroupBox)
    {
        QHBoxLayout *pFormatVariantLayout = new QHBoxLayout(m_pFormatVariantGroupBox);
        pFormatVariantLayout->addWidget(m_pFormatComboBox, 0, Qt::AlignTop);
        pFormatVariantLayout->addWidget(m_pVariantWidget);
        pMainLayout->addWidget(m_pFormatVariantGroupBox);
    }
}

void UIWizardCloneVDExpertPage::sltMediumFormatChanged()
{
    if (wizardWindow<UIWizardCloneVD>() && m_pFormatComboBox)
        wizardWindow<UIWizardCloneVD>()->setMediumFormat(m_pFormatComboBox->mediumFormat());
    updateDiskWidgetsAfterMediumFormatChange();
    emit completeChanged();
}

void UIWizardCloneVDExpertPage::sltSelectLocationButtonClicked()
{
    UIWizardCloneVD *pWizard = wizardWindow<UIWizardCloneVD>();
    AssertReturnVoid(pWizard);
    CMediumFormat comMediumFormat(pWizard->mediumFormat());
    QString strSelectedPath =
        UIWizardDiskEditors::openFileDialogForDiskFile(pWizard->mediumPath(), comMediumFormat, pWizard->deviceType(), pWizard);

    if (strSelectedPath.isEmpty())
        return;
    QString strMediumPath =
        UIWizardDiskEditors::appendExtension(strSelectedPath,
                                             UIWizardDiskEditors::defaultExtension(pWizard->mediumFormat(), pWizard->deviceType()));
    QFileInfo mediumPath(strMediumPath);
    m_pMediumSizePathGroupBox->setMediumFilePath(QDir::toNativeSeparators(mediumPath.absoluteFilePath()));
}

void UIWizardCloneVDExpertPage::sltMediumVariantChanged(qulonglong uVariant)
{
    if (wizardWindow<UIWizardCloneVD>())
        wizardWindow<UIWizardCloneVD>()->setMediumVariant(uVariant);
}

void UIWizardCloneVDExpertPage::sltMediumSizeChanged(qulonglong uSize)
{
    UIWizardCloneVD *pWizard = wizardWindow<UIWizardCloneVD>();
    AssertReturnVoid(pWizard);
    pWizard->setMediumSize(uSize);
    emit completeChanged();
}

void UIWizardCloneVDExpertPage::sltMediumPathChanged(const QString &strPath)
{
    UIWizardCloneVD *pWizard = wizardWindow<UIWizardCloneVD>();
    AssertReturnVoid(pWizard);
    QString strMediumPath =
        UIWizardDiskEditors::appendExtension(strPath,
                                             UIWizardDiskEditors::defaultExtension(pWizard->mediumFormat(), pWizard->deviceType()));
    pWizard->setMediumPath(strMediumPath);
    emit completeChanged();
}

void UIWizardCloneVDExpertPage::retranslateUi()
{
    if (m_pFormatVariantGroupBox)
        m_pFormatVariantGroupBox->setTitle(UIWizardCloneVD::tr("Hard Disk File &Type and Variant"));
}

void UIWizardCloneVDExpertPage::initializePage()
{
    AssertReturnVoid(wizardWindow<UIWizardCloneVD>() && m_pMediumSizePathGroupBox && m_pFormatComboBox && m_pVariantWidget);
    UIWizardCloneVD *pWizard = wizardWindow<UIWizardCloneVD>();

    pWizard->setMediumFormat(m_pFormatComboBox->mediumFormat());

    pWizard->setMediumVariant(m_pVariantWidget->mediumVariant());
    m_pVariantWidget->updateMediumVariantWidgetsAfterFormatChange(pWizard->mediumFormat());

    /* Initialize medium size widget and wizard's medium size parameter: */
    m_pMediumSizePathGroupBox->blockSignals(true);
    m_pMediumSizePathGroupBox->setMediumSize(pWizard->sourceDiskLogicalSize());
    pWizard->setMediumSize(m_pMediumSizePathGroupBox->mediumSize());
    QString strExtension = UIWizardDiskEditors::defaultExtension(pWizard->mediumFormat(), pWizard->deviceType());
    QString strSourceDiskPath = QDir::toNativeSeparators(QFileInfo(pWizard->sourceDiskFilePath()).absolutePath());
    /* Disk name without the format extension: */
    QString strDiskName = QString("%1_%2").arg(QFileInfo(pWizard->sourceDiskName()).completeBaseName()).arg(UIWizardCloneVD::tr("copy"));
    QString strMediumFilePath =
        UIWizardDiskEditors::constructMediumFilePath(UIWizardDiskEditors::appendExtension(strDiskName,
                                                                                          strExtension), strSourceDiskPath);
    m_pMediumSizePathGroupBox->setMediumFilePath(strMediumFilePath);
    pWizard->setMediumPath(strMediumFilePath);
    m_pMediumSizePathGroupBox->blockSignals(false);

    /* Translate page: */
    retranslateUi();
}

bool UIWizardCloneVDExpertPage::isComplete() const
{
    bool fResult = true;

    if (m_pFormatComboBox)
        fResult = m_pFormatComboBox->mediumFormat().isNull();
    if (m_pVariantWidget)
        fResult = m_pVariantWidget->isComplete();
    if (m_pMediumSizePathGroupBox)
        fResult =  m_pMediumSizePathGroupBox->isComplete();

    return fResult;
}

bool UIWizardCloneVDExpertPage::validatePage()
{
    UIWizardCloneVD *pWizard = wizardWindow<UIWizardCloneVD>();
    AssertReturn(pWizard, false);

    QString strMediumPath(pWizard->mediumPath());

    if (QFileInfo(strMediumPath).exists())
    {
        UINotificationMessage::cannotOverwriteMediumStorage(strMediumPath, wizard()->notificationCenter());
        return false;
    }
    return pWizard->copyVirtualDisk();
}

void UIWizardCloneVDExpertPage::updateDiskWidgetsAfterMediumFormatChange()
{
    UIWizardCloneVD *pWizard = wizardWindow<UIWizardCloneVD>();
    AssertReturnVoid(pWizard && m_pVariantWidget && m_pMediumSizePathGroupBox && m_pFormatComboBox);
    const CMediumFormat &comMediumFormat = pWizard->mediumFormat();
    AssertReturnVoid(!comMediumFormat.isNull());

    m_pVariantWidget->blockSignals(true);
    m_pVariantWidget->updateMediumVariantWidgetsAfterFormatChange(comMediumFormat);
    m_pVariantWidget->blockSignals(false);

    m_pMediumSizePathGroupBox->blockSignals(true);
    m_pMediumSizePathGroupBox->updateMediumPath(comMediumFormat, m_pFormatComboBox->formatExtensions(), m_enmDeviceType);
    m_pMediumSizePathGroupBox->blockSignals(false);
    /* Update the wizard parameters explicitly since we blocked th signals: */
    pWizard->setMediumPath(m_pMediumSizePathGroupBox->mediumFilePath());
    pWizard->setMediumVariant(m_pVariantWidget->mediumVariant());
}
