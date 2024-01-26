/* $Id: UIWizardNewVDSizeLocationPage.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardNewVDSizeLocationPage class implementation.
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
#include <QVBoxLayout>

/* GUI includes: */
#include "UIWizardNewVDSizeLocationPage.h"
#include "UIWizardNewVD.h"
#include "UICommon.h"
#include "UINotificationCenter.h"
#include "UIWizardDiskEditors.h"

/* COM includes: */
#include "CSystemProperties.h"

UIWizardNewVDSizeLocationPage::UIWizardNewVDSizeLocationPage(const QString &strDefaultName,
                                                             const QString &strDefaultPath, qulonglong uDefaultSize)
    : m_pMediumSizePathGroup(0)
    , m_uMediumSizeMin(_4M)
    , m_uMediumSizeMax(uiCommon().virtualBox().GetSystemProperties().GetInfoVDSize())
    , m_strDefaultName(strDefaultName.isEmpty() ? QString("NewVirtualDisk1") : strDefaultName)
    , m_strDefaultPath(strDefaultPath)
    , m_uDefaultSize(uDefaultSize)
{
    prepare();
}

void UIWizardNewVDSizeLocationPage::prepare()
{
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    AssertReturnVoid(pMainLayout);
    m_pMediumSizePathGroup = new UIMediumSizeAndPathGroupBox(false /* fExpertMode */, 0 /* parent */, _4M /* minimum size */);
    connect(m_pMediumSizePathGroup, &UIMediumSizeAndPathGroupBox::sigMediumSizeChanged,
            this, &UIWizardNewVDSizeLocationPage::sltMediumSizeChanged);
    connect(m_pMediumSizePathGroup, &UIMediumSizeAndPathGroupBox::sigMediumPathChanged,
            this, &UIWizardNewVDSizeLocationPage::sltMediumPathChanged);
    connect(m_pMediumSizePathGroup, &UIMediumSizeAndPathGroupBox::sigMediumLocationButtonClicked,
            this, &UIWizardNewVDSizeLocationPage::sltSelectLocationButtonClicked);
    pMainLayout->addWidget(m_pMediumSizePathGroup);
    pMainLayout->addStretch();
    retranslateUi();
}

void UIWizardNewVDSizeLocationPage::sltSelectLocationButtonClicked()
{
    UIWizardNewVD *pWizard = wizardWindow<UIWizardNewVD>();
    AssertReturnVoid(pWizard);
    QString strSelectedPath =
        UIWizardDiskEditors::openFileDialogForDiskFile(pWizard->mediumPath(), pWizard->mediumFormat(),
                                                                KDeviceType_HardDisk, pWizard);

    if (strSelectedPath.isEmpty())
        return;
    QString strMediumPath =
        UIWizardDiskEditors::appendExtension(strSelectedPath,
                                              UIWizardDiskEditors::defaultExtension(pWizard->mediumFormat(), KDeviceType_HardDisk));
    QFileInfo mediumPath(strMediumPath);
    m_pMediumSizePathGroup->setMediumFilePath(QDir::toNativeSeparators(mediumPath.absoluteFilePath()));
}

void UIWizardNewVDSizeLocationPage::sltMediumSizeChanged(qulonglong uSize)
{
    AssertReturnVoid(wizardWindow<UIWizardNewVD>());
    m_userModifiedParameters << "MediumSize";
    wizardWindow<UIWizardNewVD>()->setMediumSize(uSize);
    emit completeChanged();
}

void UIWizardNewVDSizeLocationPage::sltMediumPathChanged(const QString &strPath)
{
    UIWizardNewVD *pWizard = wizardWindow<UIWizardNewVD>();
    AssertReturnVoid(pWizard);
    m_userModifiedParameters << "MediumPath";
    QString strMediumPath =
        UIWizardDiskEditors::appendExtension(strPath,
                                              UIWizardDiskEditors::defaultExtension(pWizard->mediumFormat(), KDeviceType_HardDisk));
    pWizard->setMediumPath(strMediumPath);
    emit completeChanged();
}

void UIWizardNewVDSizeLocationPage::retranslateUi()
{
    setTitle(UIWizardNewVD::tr("File location and size"));
}

void UIWizardNewVDSizeLocationPage::initializePage()
{
    UIWizardNewVD *pWizard = wizardWindow<UIWizardNewVD>();
    AssertReturnVoid(pWizard && m_pMediumSizePathGroup);

    QString strExtension = UIWizardDiskEditors::defaultExtension(pWizard->mediumFormat(), KDeviceType_HardDisk);
    QString strMediumFilePath;
    /* Initialize the medium file path with default name and path if user has not exclusively modified them yet: */
    if (!m_userModifiedParameters.contains("MediumPath"))
        strMediumFilePath =
            UIWizardDiskEditors::constructMediumFilePath(UIWizardDiskEditors::appendExtension(m_strDefaultName,
                                                                                                strExtension), m_strDefaultPath);
    /* Initialize the medium file path with file path and file name from the location editor. This part is to update the
     * file extention correctly in case user has gone back and changed the file format after modifying medium file path: */
    else
        strMediumFilePath =
            UIWizardDiskEditors::constructMediumFilePath(UIWizardDiskEditors::appendExtension(m_pMediumSizePathGroup->mediumName(),
                                                                                                strExtension), m_pMediumSizePathGroup->mediumPath());
    m_pMediumSizePathGroup->blockSignals(true);
    m_pMediumSizePathGroup->setMediumFilePath(strMediumFilePath);
    m_pMediumSizePathGroup->blockSignals(false);
    pWizard->setMediumPath(m_pMediumSizePathGroup->mediumFilePath());

    if (!m_userModifiedParameters.contains("MediumSize"))
    {
        m_pMediumSizePathGroup->blockSignals(true);
        m_pMediumSizePathGroup->setMediumSize(m_uDefaultSize > m_uMediumSizeMin && m_uDefaultSize < m_uMediumSizeMax ? m_uDefaultSize : m_uMediumSizeMin);
        m_pMediumSizePathGroup->blockSignals(false);
        pWizard->setMediumSize(m_pMediumSizePathGroup->mediumSize());
    }
    retranslateUi();
}

bool UIWizardNewVDSizeLocationPage::isComplete() const
{
    UIWizardNewVD *pWizard = wizardWindow<UIWizardNewVD>();
    AssertReturn(pWizard, false);
    if (pWizard->mediumPath().isEmpty())
        return false;
    if (pWizard->mediumSize() > m_uMediumSizeMax || pWizard->mediumSize() < m_uMediumSizeMin)
        return false;
    return true;
}

bool UIWizardNewVDSizeLocationPage::validatePage()
{
    bool fResult = true;
    UIWizardNewVD *pWizard = wizardWindow<UIWizardNewVD>();
    AssertReturn(pWizard, false);
    /* Make sure such file doesn't exist already: */
    const QString strMediumPath(pWizard->mediumPath());
    fResult = !QFileInfo(strMediumPath).exists();
    if (!fResult)
    {
        UINotificationMessage::cannotOverwriteMediumStorage(strMediumPath, wizard()->notificationCenter());
        return fResult;
    }

    /* Make sure we are passing FAT size limitation: */
    fResult = UIWizardDiskEditors::checkFATSizeLimitation(pWizard->mediumVariant(),
                                     pWizard->mediumPath(),
                                     pWizard->mediumSize());
    if (!fResult)
    {
        UINotificationMessage::cannotCreateMediumStorageInFAT(strMediumPath, wizard()->notificationCenter());
        return fResult;
    }

    fResult = pWizard->createVirtualDisk();
    return fResult;
}
