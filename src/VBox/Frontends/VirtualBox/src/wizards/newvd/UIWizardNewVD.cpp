/* $Id: UIWizardNewVD.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardNewVD class implementation.
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

/* GUI includes: */
#include "UICommon.h"
#include "UIModalWindowManager.h"
#include "UINotificationCenter.h"
#include "UIWizardNewVD.h"
#include "UIWizardNewVDFileTypePage.h"
#include "UIWizardNewVDVariantPage.h"
#include "UIWizardNewVDSizeLocationPage.h"
#include "UIWizardNewVDExpertPage.h"

UIWizardNewVD::UIWizardNewVD(QWidget *pParent,
                             const QString &strDefaultName,
                             const QString &strDefaultPath,
                             qulonglong uDefaultSize,
                             WizardMode mode)
    : UINativeWizard(pParent, WizardType_NewVD, mode, "create-virtual-hard-disk-image" /* help keyword */)
    , m_strDefaultName(strDefaultName)
    , m_strDefaultPath(strDefaultPath)
    , m_uDefaultSize(uDefaultSize)
    , m_iMediumVariantPageIndex(-1)
{
#ifndef VBOX_WS_MAC
    /* Assign watermark: */
    setPixmapName(":/wizard_new_harddisk.png");
#else /* VBOX_WS_MAC */
    /* Assign background image: */
    setPixmapName(":/wizard_new_harddisk_bg.png");
#endif /* VBOX_WS_MAC */
}

qulonglong UIWizardNewVD::mediumVariant() const
{
    return m_uMediumVariant;
}

void UIWizardNewVD::setMediumVariant(qulonglong uMediumVariant)
{
    m_uMediumVariant = uMediumVariant;
}

const CMediumFormat &UIWizardNewVD::mediumFormat()
{
    return m_comMediumFormat;
}

void UIWizardNewVD::setMediumFormat(const CMediumFormat &mediumFormat)
{
    m_comMediumFormat = mediumFormat;
    if (mode() == WizardMode_Basic)
        setMediumVariantPageVisibility();
}

const QString &UIWizardNewVD::mediumPath() const
{
    return m_strMediumPath;
}

void UIWizardNewVD::setMediumPath(const QString &strMediumPath)
{
    m_strMediumPath = strMediumPath;
}

qulonglong UIWizardNewVD::mediumSize() const
{
    return m_uMediumSize;
}

void UIWizardNewVD::setMediumSize(qulonglong uMediumSize)
{
    m_uMediumSize = uMediumSize;
}

QUuid UIWizardNewVD::mediumId() const
{
    return m_uMediumId;
}

void UIWizardNewVD::populatePages()
{
    switch (mode())
    {
        case WizardMode_Basic:
        {
            addPage(new UIWizardNewVDFileTypePage);
            m_iMediumVariantPageIndex = addPage(new UIWizardNewVDVariantPage);
            addPage(new UIWizardNewVDSizeLocationPage(m_strDefaultName, m_strDefaultPath, m_uDefaultSize));
            break;
        }
        case WizardMode_Expert:
        {
            addPage(new UIWizardNewVDExpertPage(m_strDefaultName, m_strDefaultPath, m_uDefaultSize));
            break;
        }
        default:
        {
            AssertMsgFailed(("Invalid mode: %d", mode()));
            break;
        }
    }
}

bool UIWizardNewVD::createVirtualDisk()
{
    AssertReturn(!m_strMediumPath.isNull(), false);
    AssertReturn(m_uMediumSize > 0, false);

    /* Get VBox object: */
    CVirtualBox comVBox = uiCommon().virtualBox();

    /* Create new virtual disk image: */
    CMedium comVirtualDisk = comVBox.CreateMedium(m_comMediumFormat.GetName(),
                                                  m_strMediumPath, KAccessMode_ReadWrite, KDeviceType_HardDisk);
    if (!comVBox.isOk())
    {
        UINotificationMessage::cannotCreateMediumStorage(comVBox, m_strMediumPath, notificationCenter());
        return false;
    }

    /* Compose medium-variant: */
    QVector<KMediumVariant> variants(sizeof(qulonglong) * 8);
    for (int i = 0; i < variants.size(); ++i)
    {
        qulonglong temp = m_uMediumVariant;
        temp &= Q_UINT64_C(1) << i;
        variants[i] = (KMediumVariant)temp;
    }

    UINotificationProgressMediumCreate *pNotification = new UINotificationProgressMediumCreate(comVirtualDisk,
                                                                                               m_uMediumSize,
                                                                                               variants);
    connect(pNotification, &UINotificationProgressMediumCreate::sigMediumCreated,
            &uiCommon(), &UICommon::sltHandleMediumCreated);

    m_uMediumId = comVirtualDisk.GetId();

    gpNotificationCenter->append(pNotification);

    /* Positive: */
    return true;
}

/* static */
QUuid UIWizardNewVD::createVDWithWizard(QWidget *pParent,
                                        const QString &strMachineFolder /* = QString() */,
                                        const QString &strMachineName /* = QString() */,
                                        const QString &strMachineGuestOSTypeId  /* = QString() */)
{
    /* Initialize variables: */
    QString strDefaultFolder = strMachineFolder;
    if (strDefaultFolder.isEmpty())
        strDefaultFolder = uiCommon().defaultFolderPathForType(UIMediumDeviceType_HardDisk);

    /* In case we dont have a 'guest os type id' default back to 'Other': */
    const CGuestOSType comGuestOSType = uiCommon().virtualBox().GetGuestOSType(  !strMachineGuestOSTypeId.isEmpty()
                                                                                 ? strMachineGuestOSTypeId
                                                                                 : "Other");
    const QString strDiskName = uiCommon().findUniqueFileName(strDefaultFolder,   !strMachineName.isEmpty()
                                                                     ? strMachineName
                                                                     : "NewVirtualDisk");

    /* Show New VD wizard: */
    UISafePointerWizardNewVD pWizard = new UIWizardNewVD(pParent,
                                                         strDiskName,
                                                         strDefaultFolder,
                                                         comGuestOSType.GetRecommendedHDD());
    if (!pWizard)
        return QUuid();
    QWidget *pDialogParent = windowManager().realParentWindow(pParent);
    windowManager().registerNewParent(pWizard, pDialogParent);
    QUuid mediumId = pWizard->mediumId();
    pWizard->exec();
    delete pWizard;
    return mediumId;
}

void UIWizardNewVD::retranslateUi()
{
    UINativeWizard::retranslateUi();
    setWindowTitle(tr("Create Virtual Hard Disk"));
}

void UIWizardNewVD::setMediumVariantPageVisibility()
{
    AssertReturnVoid(!m_comMediumFormat.isNull());
    ULONG uCapabilities = 0;
    QVector<KMediumFormatCapabilities> capabilities;
    capabilities = m_comMediumFormat.GetCapabilities();
    for (int i = 0; i < capabilities.size(); i++)
        uCapabilities |= capabilities[i];

    int cTest = 0;
    if (uCapabilities & KMediumFormatCapabilities_CreateDynamic)
        ++cTest;
    if (uCapabilities & KMediumFormatCapabilities_CreateFixed)
        ++cTest;
    if (uCapabilities & KMediumFormatCapabilities_CreateSplit2G)
        ++cTest;
    setPageVisible(m_iMediumVariantPageIndex, cTest > 1);
}
