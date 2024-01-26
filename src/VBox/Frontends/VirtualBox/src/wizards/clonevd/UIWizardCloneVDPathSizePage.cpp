/* $Id: UIWizardCloneVDPathSizePage.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVDPathSizePage class implementation.
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
#include "UINotificationCenter.h"
#include "UIWizardCloneVDPathSizePage.h"
#include "UIWizardDiskEditors.h"
#include "UIWizardCloneVD.h"

UIWizardCloneVDPathSizePage::UIWizardCloneVDPathSizePage(qulonglong uSourceDiskLogicaSize)
    : m_pMediumSizePathGroupBox(0)
{
    prepare(uSourceDiskLogicaSize);
}

void UIWizardCloneVDPathSizePage::prepare(qulonglong uSourceDiskLogicaSize)
{
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    m_pMediumSizePathGroupBox = new UIMediumSizeAndPathGroupBox(false /* expert mode */, 0 /* parent */, uSourceDiskLogicaSize);
    if (m_pMediumSizePathGroupBox)
    {
        pMainLayout->addWidget(m_pMediumSizePathGroupBox);
        connect(m_pMediumSizePathGroupBox, &UIMediumSizeAndPathGroupBox::sigMediumLocationButtonClicked,
                this, &UIWizardCloneVDPathSizePage::sltSelectLocationButtonClicked);
        connect(m_pMediumSizePathGroupBox, &UIMediumSizeAndPathGroupBox::sigMediumPathChanged,
                this, &UIWizardCloneVDPathSizePage::sltMediumPathChanged);
        connect(m_pMediumSizePathGroupBox, &UIMediumSizeAndPathGroupBox::sigMediumSizeChanged,
                this, &UIWizardCloneVDPathSizePage::sltMediumSizeChanged);
    }
    pMainLayout->addStretch();
    retranslateUi();
}

void UIWizardCloneVDPathSizePage::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardCloneVD::tr("Location and size of the disk image"));
}

void UIWizardCloneVDPathSizePage::initializePage()
{
    AssertReturnVoid(wizardWindow<UIWizardCloneVD>() && m_pMediumSizePathGroupBox);
    /* Translate page: */
    retranslateUi();
    UIWizardCloneVD *pWizard = wizardWindow<UIWizardCloneVD>();
    m_pMediumSizePathGroupBox->blockSignals(true);

    /* Initialize medium size widget and wizard's medium size parameter: */
    if (!m_userModifiedParameters.contains("MediumSize"))
    {
        m_pMediumSizePathGroupBox->setMediumSize(pWizard->sourceDiskLogicalSize());
        pWizard->setMediumSize(m_pMediumSizePathGroupBox->mediumSize());
    }

    if (!m_userModifiedParameters.contains("MediumPath"))
    {
        QString strExtension = UIWizardDiskEditors::defaultExtension(pWizard->mediumFormat(), pWizard->deviceType());
        QString strSourceDiskPath = QDir::toNativeSeparators(QFileInfo(pWizard->sourceDiskFilePath()).absolutePath());
        /* Disk name without the format extension: */
        QString strDiskName = QString("%1_%2").arg(QFileInfo(pWizard->sourceDiskName()).completeBaseName()).arg(UIWizardCloneVD::tr("copy"));

        QString strMediumFilePath =
            UIWizardDiskEditors::constructMediumFilePath(UIWizardDiskEditors::appendExtension(strDiskName,
                                                                                              strExtension), strSourceDiskPath);
        m_pMediumSizePathGroupBox->setMediumFilePath(strMediumFilePath);
        pWizard->setMediumPath(strMediumFilePath);
    }
    m_pMediumSizePathGroupBox->blockSignals(false);
}

bool UIWizardCloneVDPathSizePage::isComplete() const
{
    AssertReturn(m_pMediumSizePathGroupBox, false);
    return m_pMediumSizePathGroupBox->isComplete();
}

bool UIWizardCloneVDPathSizePage::validatePage()
{
    UIWizardCloneVD *pWizard = wizardWindow<UIWizardCloneVD>();
    AssertReturn(pWizard, false);
    /* Make sure such file doesn't exists already: */
    QString strMediumPath(pWizard->mediumPath());
    if (QFileInfo(strMediumPath).exists())
    {
        UINotificationMessage::cannotOverwriteMediumStorage(strMediumPath, wizard()->notificationCenter());
        return false;
    }
    return pWizard->copyVirtualDisk();
}

void UIWizardCloneVDPathSizePage::sltSelectLocationButtonClicked()
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

void UIWizardCloneVDPathSizePage::sltMediumPathChanged(const QString &strPath)
{
    UIWizardCloneVD *pWizard = wizardWindow<UIWizardCloneVD>();
    AssertReturnVoid(pWizard);
    m_userModifiedParameters << "MediumPath";
    QString strMediumPath =
        UIWizardDiskEditors::appendExtension(strPath,
                                             UIWizardDiskEditors::defaultExtension(pWizard->mediumFormat(), pWizard->deviceType()));
    pWizard->setMediumPath(strMediumPath);
    emit completeChanged();
}

void UIWizardCloneVDPathSizePage::sltMediumSizeChanged(qulonglong uSize)
{
    UIWizardCloneVD *pWizard = wizardWindow<UIWizardCloneVD>();
    AssertReturnVoid(pWizard);
    m_userModifiedParameters << "MediumSize";
    pWizard->setMediumSize(uSize);
    emit completeChanged();
}
