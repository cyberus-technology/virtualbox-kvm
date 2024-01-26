/* $Id: UIWizardNewVM.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardNewVM class implementation.
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
#include <QAbstractButton>
#include <QLayout>

/* GUI includes: */
#include "UICommon.h"
#include "UIMedium.h"
#include "UINotificationCenter.h"
#include "UIWizardNewVM.h"
#include "UIWizardNewVMNameOSTypePage.h"
#include "UIWizardNewVMUnattendedPage.h"
#include "UIWizardNewVMHardwarePage.h"
#include "UIWizardNewVMDiskPage.h"
#include "UIWizardNewVMExpertPage.h"
#include "UIWizardNewVMSummaryPage.h"

/* COM includes: */
#include "CAudioAdapter.h"
#include "CAudioSettings.h"
#include "CBIOSSettings.h"
#include "CGraphicsAdapter.h"
#include "CExtPackManager.h"
#include "CMediumFormat.h"
#include "CStorageController.h"
#include "CUSBController.h"
#include "CUSBDeviceFilters.h"
#include "CUnattended.h"

/* Namespaces: */
using namespace UIExtraDataDefs;


UIWizardNewVM::UIWizardNewVM(QWidget *pParent,
                             UIActionPool *pActionPool,
                             const QString &strMachineGroup,
                             CUnattended &comUnattended,
                             const QString &strISOFilePath /* = QString() */)
    : UINativeWizard(pParent, WizardType_NewVM, WizardMode_Auto, "create-vm-wizard" /* help keyword */)
    , m_strMachineGroup(strMachineGroup)
    , m_iIDECount(0)
    , m_iSATACount(0)
    , m_iSCSICount(0)
    , m_iFloppyCount(0)
    , m_iSASCount(0)
    , m_iUSBCount(0)
    , m_fInstallGuestAdditions(false)
    , m_fSkipUnattendedInstall(false)
    , m_fEFIEnabled(false)
    , m_iCPUCount(1)
    , m_iMemorySize(0)
    , m_iUnattendedInstallPageIndex(-1)
    , m_uMediumVariant(0)
    , m_uMediumSize(0)
    , m_enmDiskSource(SelectedDiskSource_New)
    , m_fEmptyDiskRecommended(false)
    , m_pActionPool(pActionPool)
    , m_comUnattended(comUnattended)
    , m_fStartHeadless(false)
    , m_strInitialISOFilePath(strISOFilePath)
{
#ifndef VBOX_WS_MAC
    /* Assign watermark: */
    setPixmapName(":/wizard_new_welcome.png");
#else /* VBOX_WS_MAC */
    /* Assign background image: */
    setPixmapName(":/wizard_new_welcome_bg.png");
#endif /* VBOX_WS_MAC */
    /* Register classes: */
    qRegisterMetaType<CGuestOSType>();

    connect(this, &UIWizardNewVM::rejected, this, &UIWizardNewVM::sltHandleWizardCancel);
}

void UIWizardNewVM::populatePages()
{
    switch (mode())
    {
        case WizardMode_Basic:
        {
            UIWizardNewVMNameOSTypePage *pNamePage = new UIWizardNewVMNameOSTypePage;
            addPage(pNamePage);
            if (!m_strInitialISOFilePath.isEmpty())
                pNamePage->setISOFilePath(m_strInitialISOFilePath);
            m_iUnattendedInstallPageIndex = addPage(new UIWizardNewVMUnattendedPage);
            setUnattendedPageVisible(false);
            addPage(new UIWizardNewVMHardwarePage);
            addPage(new UIWizardNewVMDiskPage(m_pActionPool));
            addPage(new UIWizardNewVMSummaryPage);
            break;
        }
        case WizardMode_Expert:
        {
            UIWizardNewVMExpertPage *pExpertPage = new UIWizardNewVMExpertPage(m_pActionPool);
            addPage(pExpertPage);
            if (!m_strInitialISOFilePath.isEmpty())
                pExpertPage->setISOFilePath(m_strInitialISOFilePath);
            break;
        }
        default:
        {
            AssertMsgFailed(("Invalid mode: %d", mode()));
            break;
        }
    }
}

void UIWizardNewVM::cleanWizard()
{
    /* Try to delete the hard disk in case we have created one: */
    deleteVirtualDisk();
    /* Cleanup the machine folder: */
    UIWizardNewVMNameOSTypeCommon::cleanupMachineFolder(this, true);

    if (!m_machine.isNull())
        m_machine.detach();
}

bool UIWizardNewVM::createVM()
{
    CVirtualBox vbox = uiCommon().virtualBox();
    QString strTypeId = m_comGuestOSType.GetId();

    /* Create virtual machine: */
    if (m_machine.isNull())
    {
        QVector<QString> groups;
        if (!m_strMachineGroup.isEmpty())
            groups << m_strMachineGroup;
        m_machine = vbox.CreateMachine(m_strMachineFilePath,
                                       m_strMachineBaseName,
                                       groups, strTypeId, QString(),
                                       QString(), QString(), QString());
        if (!vbox.isOk())
        {
            UINotificationMessage::cannotCreateMachine(vbox, notificationCenter());
            cleanWizard();
            return false;
        }
    }

#if 0
    /* Configure the newly created vm here in GUI by several calls to API: */
    configureVM(strTypeId, m_comGuestOSType);
#else
    /* The newer and less tested way of configuring vms: */
    m_machine.ApplyDefaults(QString());
    /* Apply user preferences again. IMachine::applyDefaults may have overwritten the user setting: */
    m_machine.SetMemorySize(m_iMemorySize);
    int iVPUCount = qMax(1, m_iCPUCount);
    m_machine.SetCPUCount(iVPUCount);
    /* Correct the VRAM size since API does not take fullscreen memory requirements into account: */
    CGraphicsAdapter comGraphics = m_machine.GetGraphicsAdapter();
    comGraphics.SetVRAMSize(qMax(comGraphics.GetVRAMSize(), (ULONG)(UICommon::requiredVideoMemory(strTypeId) / _1M)));
    /* Enabled I/O APIC explicitly in we have more than 1 VCPU: */
    if (iVPUCount > 1)
        m_machine.GetBIOSSettings().SetIOAPICEnabled(true);

    /* Set recommended firmware type: */
    m_machine.SetFirmwareType(m_fEFIEnabled ? KFirmwareType_EFI : KFirmwareType_BIOS);
#endif

    /* Register the VM prior to attaching hard disks: */
    vbox.RegisterMachine(m_machine);
    if (!vbox.isOk())
    {
        UINotificationMessage::cannotRegisterMachine(vbox, m_machine.GetName(), notificationCenter());
        cleanWizard();
        return false;
    }

    if (!attachDefaultDevices())
    {
        cleanWizard();
        return false;
    }

    if (isUnattendedEnabled())
    {
        m_comUnattended.SetMachine(m_machine);
        if (!checkUnattendedInstallError(m_comUnattended))
        {
            cleanWizard();
            return false;
        }
    }
    return true;
}

bool UIWizardNewVM::createVirtualDisk()
{
    /* Prepare result: */
    bool fResult = false;

    /* Check attributes: */
    AssertReturn(!m_strMediumPath.isNull(), false);
    AssertReturn(m_uMediumSize > 0, false);

    /* Acquire VBox: */
    CVirtualBox comVBox = uiCommon().virtualBox();

    /* Create new virtual hard-disk: */
    CMedium newVirtualDisk = comVBox.CreateMedium(m_comMediumFormat.GetName(), m_strMediumPath, KAccessMode_ReadWrite, KDeviceType_HardDisk);
    if (!comVBox.isOk())
    {
        UINotificationMessage::cannotCreateMediumStorage(comVBox, m_strMediumPath, notificationCenter());
        return fResult;
    }

    /* Create base storage for the new virtual-disk: */
    UINotificationProgressMediumCreate *pNotification = new UINotificationProgressMediumCreate(newVirtualDisk,
                                                                                               m_uMediumSize,
                                                                                               mediumVariants());
    if (!handleNotificationProgressNow(pNotification))
        return fResult;

    /* Inform UICommon about it: */
    uiCommon().createMedium(UIMedium(newVirtualDisk, UIMediumDeviceType_HardDisk, KMediumState_Created));

    /* Remember created virtual-disk: */
    m_virtualDisk = newVirtualDisk;

    /* True finally: */
    fResult = true;

    /* Return result: */
    return fResult;
}

void UIWizardNewVM::deleteVirtualDisk()
{
    /* Do nothing if an existing disk has been selected: */
    if (m_enmDiskSource == SelectedDiskSource_Existing)
        return;

    /* Make sure virtual-disk valid: */
    if (m_virtualDisk.isNull())
        return;

    /* Delete storage of existing disk: */
    UINotificationProgressMediumDeletingStorage *pNotification = new UINotificationProgressMediumDeletingStorage(m_virtualDisk);
    if (!handleNotificationProgressNow(pNotification))
        return;

    /* Detach virtual-disk finally: */
    m_virtualDisk.detach();
}

void UIWizardNewVM::configureVM(const QString &strGuestTypeId, const CGuestOSType &comGuestType)
{
    /* Get graphics adapter: */
    CGraphicsAdapter comGraphics = m_machine.GetGraphicsAdapter();

    /* RAM size: */
    m_machine.SetMemorySize(m_iMemorySize);

    /* VCPU count: */
    int iVPUCount = qMax(1, m_iCPUCount);
    m_machine.SetCPUCount(iVPUCount);

    /* Enabled I/O APIC explicitly in we have more than 1 VCPU: */
    if (iVPUCount > 1)
        m_machine.GetBIOSSettings().SetIOAPICEnabled(true);

    /* Graphics Controller type: */
    comGraphics.SetGraphicsControllerType(comGuestType.GetRecommendedGraphicsController());

    /* VRAM size - select maximum between recommended and minimum for fullscreen: */
    comGraphics.SetVRAMSize(qMax(comGuestType.GetRecommendedVRAM(), (ULONG)(UICommon::requiredVideoMemory(strGuestTypeId) / _1M)));

    /* Selecting recommended chipset type: */
    m_machine.SetChipsetType(comGuestType.GetRecommendedChipset());

    /* Selecting recommended Audio Controller: */
    CAudioSettings const comAudioSettings = m_machine.GetAudioSettings();
    CAudioAdapter        comAdapter  = comAudioSettings.GetAdapter();
    comAdapter.SetAudioController(comGuestType.GetRecommendedAudioController());
    /* And the Audio Codec: */
    comAdapter.SetAudioCodec(comGuestType.GetRecommendedAudioCodec());
    /* Enabling audio by default: */
    comAdapter.SetEnabled(true);
    comAdapter.SetEnabledOut(true);

    /* Enable the OHCI and EHCI controller by default for new VMs. (new in 2.2): */
    CUSBDeviceFilters usbDeviceFilters = m_machine.GetUSBDeviceFilters();
    bool fOhciEnabled = false;
    if (!usbDeviceFilters.isNull() && comGuestType.GetRecommendedUSB3() && m_machine.GetUSBProxyAvailable())
    {
        m_machine.AddUSBController("XHCI", KUSBControllerType_XHCI);
        /* xHCI includes OHCI */
        fOhciEnabled = true;
    }
    if (   !fOhciEnabled
        && !usbDeviceFilters.isNull() && comGuestType.GetRecommendedUSB() && m_machine.GetUSBProxyAvailable())
    {
        m_machine.AddUSBController("OHCI", KUSBControllerType_OHCI);
        fOhciEnabled = true;
        m_machine.AddUSBController("EHCI", KUSBControllerType_EHCI);
    }

    /* Create a floppy controller if recommended: */
    QString strFloppyName = getNextControllerName(KStorageBus_Floppy);
    if (comGuestType.GetRecommendedFloppy())
    {
        m_machine.AddStorageController(strFloppyName, KStorageBus_Floppy);
        CStorageController flpCtr = m_machine.GetStorageControllerByName(strFloppyName);
        flpCtr.SetControllerType(KStorageControllerType_I82078);
    }

    /* Create recommended DVD storage controller: */
    KStorageBus strDVDBus = comGuestType.GetRecommendedDVDStorageBus();
    QString strDVDName = getNextControllerName(strDVDBus);
    m_machine.AddStorageController(strDVDName, strDVDBus);

    /* Set recommended DVD storage controller type: */
    CStorageController dvdCtr = m_machine.GetStorageControllerByName(strDVDName);
    KStorageControllerType dvdStorageControllerType = comGuestType.GetRecommendedDVDStorageController();
    dvdCtr.SetControllerType(dvdStorageControllerType);

    /* Create recommended HD storage controller if it's not the same as the DVD controller: */
    KStorageBus ctrHDBus = comGuestType.GetRecommendedHDStorageBus();
    KStorageControllerType hdStorageControllerType = comGuestType.GetRecommendedHDStorageController();
    CStorageController hdCtr;
    QString strHDName;
    if (ctrHDBus != strDVDBus || hdStorageControllerType != dvdStorageControllerType)
    {
        strHDName = getNextControllerName(ctrHDBus);
        m_machine.AddStorageController(strHDName, ctrHDBus);
        hdCtr = m_machine.GetStorageControllerByName(strHDName);
        hdCtr.SetControllerType(hdStorageControllerType);
    }
    else
    {
        /* The HD controller is the same as DVD: */
        hdCtr = dvdCtr;
        strHDName = strDVDName;
    }

    /* Limit the AHCI port count if it's used because windows has trouble with
       too many ports and other guest (OS X in particular) may take extra long
       to boot: */
    if (hdStorageControllerType == KStorageControllerType_IntelAhci)
        hdCtr.SetPortCount(1 + (dvdStorageControllerType == KStorageControllerType_IntelAhci));
    else if (dvdStorageControllerType == KStorageControllerType_IntelAhci)
        dvdCtr.SetPortCount(1);

    /* Turn on PAE, if recommended: */
    m_machine.SetCPUProperty(KCPUPropertyType_PAE, comGuestType.GetRecommendedPAE());

    /* Set the recommended triple fault behavior: */
    m_machine.SetCPUProperty(KCPUPropertyType_TripleFaultReset, comGuestType.GetRecommendedTFReset());

    /* Set recommended firmware type: */
    m_machine.SetFirmwareType(m_fEFIEnabled ? KFirmwareType_EFI : KFirmwareType_BIOS);

    /* Set recommended human interface device types: */
    if (comGuestType.GetRecommendedUSBHID())
    {
        m_machine.SetKeyboardHIDType(KKeyboardHIDType_USBKeyboard);
        m_machine.SetPointingHIDType(KPointingHIDType_USBMouse);
        if (!fOhciEnabled && !usbDeviceFilters.isNull())
            m_machine.AddUSBController("OHCI", KUSBControllerType_OHCI);
    }

    if (comGuestType.GetRecommendedUSBTablet())
    {
        m_machine.SetPointingHIDType(KPointingHIDType_USBTablet);
        if (!fOhciEnabled && !usbDeviceFilters.isNull())
            m_machine.AddUSBController("OHCI", KUSBControllerType_OHCI);
    }

    /* Set HPET flag: */
    m_machine.SetHPETEnabled(comGuestType.GetRecommendedHPET());

    /* Set UTC flags: */
    m_machine.SetRTCUseUTC(comGuestType.GetRecommendedRTCUseUTC());

    /* Set graphic bits: */
    if (comGuestType.GetRecommended2DVideoAcceleration())
        comGraphics.SetAccelerate2DVideoEnabled(comGuestType.GetRecommended2DVideoAcceleration());

    if (comGuestType.GetRecommended3DAcceleration())
        comGraphics.SetAccelerate3DEnabled(comGuestType.GetRecommended3DAcceleration());
}

bool UIWizardNewVM::attachDefaultDevices()
{
    bool success = false;
    QUuid uMachineId = m_machine.GetId();
    CSession session = uiCommon().openSession(uMachineId);
    if (!session.isNull())
    {
        CMachine machine = session.GetMachine();
        if (!m_virtualDisk.isNull())
        {
            KStorageBus enmHDDBus = m_comGuestOSType.GetRecommendedHDStorageBus();
            CStorageController comHDDController = m_machine.GetStorageControllerByInstance(enmHDDBus, 0);
            if (!comHDDController.isNull())
            {
                machine.AttachDevice(comHDDController.GetName(), 0, 0, KDeviceType_HardDisk, m_virtualDisk);
                if (!machine.isOk())
                    UINotificationMessage::cannotAttachDevice(machine, UIMediumDeviceType_HardDisk, m_strMediumPath,
                                                              StorageSlot(enmHDDBus, 0, 0), notificationCenter());
            }
        }

        /* Attach optical drive: */
        KStorageBus enmDVDBus = m_comGuestOSType.GetRecommendedDVDStorageBus();
        CStorageController comDVDController = m_machine.GetStorageControllerByInstance(enmDVDBus, 0);
        if (!comDVDController.isNull())
        {
            CMedium opticalDisk;
            QString strISOFilePath = ISOFilePath();
            if (!strISOFilePath.isEmpty() && !isUnattendedEnabled())
            {
                CVirtualBox vbox = uiCommon().virtualBox();
                opticalDisk =
                    vbox.OpenMedium(strISOFilePath, KDeviceType_DVD, KAccessMode_ReadWrite, false);
                if (!vbox.isOk())
                    UINotificationMessage::cannotOpenMedium(vbox, strISOFilePath, notificationCenter());
            }
            machine.AttachDevice(comDVDController.GetName(), 1, 0, KDeviceType_DVD, opticalDisk);
            if (!machine.isOk())
                UINotificationMessage::cannotAttachDevice(machine, UIMediumDeviceType_DVD, QString(),
                                                          StorageSlot(enmDVDBus, 1, 0), notificationCenter());
        }

        /* Attach an empty floppy drive if recommended */
        if (m_comGuestOSType.GetRecommendedFloppy()) {
            CStorageController comFloppyController = m_machine.GetStorageControllerByInstance(KStorageBus_Floppy, 0);
            if (!comFloppyController.isNull())
            {
                machine.AttachDevice(comFloppyController.GetName(), 0, 0, KDeviceType_Floppy, CMedium());
                if (!machine.isOk())
                    UINotificationMessage::cannotAttachDevice(machine, UIMediumDeviceType_Floppy, QString(),
                                                              StorageSlot(KStorageBus_Floppy, 0, 0), notificationCenter());
            }
        }

        if (machine.isOk())
        {
            machine.SaveSettings();
            if (machine.isOk())
                success = true;
            else
                UINotificationMessage::cannotSaveMachineSettings(machine, notificationCenter());
        }

        session.UnlockMachine();
    }
    if (!success)
    {
        /* Unregister VM on failure: */
        const QVector<CMedium> media = m_machine.Unregister(KCleanupMode_DetachAllReturnHardDisksOnly);
        if (!m_machine.isOk())
            UINotificationMessage::cannotRemoveMachine(m_machine, notificationCenter());
        else
        {
            UINotificationProgressMachineMediaRemove *pNotification =
                new UINotificationProgressMachineMediaRemove(m_machine, media);
            handleNotificationProgressNow(pNotification);
        }
    }

    /* Make sure we detach CMedium wrapper from IMedium pointer to avoid deletion of IMedium as m_virtualDisk
     * is deallocated.  Or in case of UINotificationProgressMachineMediaRemove handling, IMedium has been
     * already deleted so detach in this case as well. */
    if (!m_virtualDisk.isNull())
        m_virtualDisk.detach();

    return success;
}

void UIWizardNewVM::sltHandleWizardCancel()
{
    cleanWizard();
}

void UIWizardNewVM::retranslateUi()
{
    UINativeWizard::retranslateUi();
    setWindowTitle(tr("Create Virtual Machine"));
}

QString UIWizardNewVM::getNextControllerName(KStorageBus type)
{
    QString strControllerName;
    switch (type)
    {
        case KStorageBus_IDE:
        {
            strControllerName = "IDE";
            ++m_iIDECount;
            if (m_iIDECount > 1)
                strControllerName = QString("%1 %2").arg(strControllerName).arg(m_iIDECount);
            break;
        }
        case KStorageBus_SATA:
        {
            strControllerName = "SATA";
            ++m_iSATACount;
            if (m_iSATACount > 1)
                strControllerName = QString("%1 %2").arg(strControllerName).arg(m_iSATACount);
            break;
        }
        case KStorageBus_SCSI:
        {
            strControllerName = "SCSI";
            ++m_iSCSICount;
            if (m_iSCSICount > 1)
                strControllerName = QString("%1 %2").arg(strControllerName).arg(m_iSCSICount);
            break;
        }
        case KStorageBus_Floppy:
        {
            strControllerName = "Floppy";
            ++m_iFloppyCount;
            if (m_iFloppyCount > 1)
                strControllerName = QString("%1 %2").arg(strControllerName).arg(m_iFloppyCount);
            break;
        }
        case KStorageBus_SAS:
        {
            strControllerName = "SAS";
            ++m_iSASCount;
            if (m_iSASCount > 1)
                strControllerName = QString("%1 %2").arg(strControllerName).arg(m_iSASCount);
            break;
        }
        case KStorageBus_USB:
        {
            strControllerName = "USB";
            ++m_iUSBCount;
            if (m_iUSBCount > 1)
                strControllerName = QString("%1 %2").arg(strControllerName).arg(m_iUSBCount);
            break;
        }
        default:
            break;
    }
    return strControllerName;
}

QUuid UIWizardNewVM::createdMachineId() const
{
    if (m_machine.isOk())
        return m_machine.GetId();
    return QUuid();
}

CMedium &UIWizardNewVM::virtualDisk()
{
    return m_virtualDisk;
}

void UIWizardNewVM::setVirtualDisk(const CMedium &medium)
{
    m_virtualDisk = medium;
}

void UIWizardNewVM::setVirtualDisk(const QUuid &mediumId)
{
    if (m_virtualDisk.isOk() && m_virtualDisk.GetId() == mediumId)
        return;
    CMedium medium = uiCommon().medium(mediumId).medium();
    setVirtualDisk(medium);
}

const QString &UIWizardNewVM::machineGroup() const
{
    return m_strMachineGroup;
}

const QString &UIWizardNewVM::machineFilePath() const
{
    return m_strMachineFilePath;
}

void UIWizardNewVM::setMachineFilePath(const QString &strMachineFilePath)
{
    m_strMachineFilePath = strMachineFilePath;
}

QString UIWizardNewVM::machineFileName() const
{
    return QFileInfo(machineFilePath()).completeBaseName();
}

const QString &UIWizardNewVM::machineFolder() const
{
    return m_strMachineFolder;
}

void UIWizardNewVM::setMachineFolder(const QString &strMachineFolder)
{
    m_strMachineFolder = strMachineFolder;
}

const QString &UIWizardNewVM::machineBaseName() const
{
    return m_strMachineBaseName;
}

void UIWizardNewVM::setMachineBaseName(const QString &strMachineBaseName)
{
    m_strMachineBaseName = strMachineBaseName;
}

const QString &UIWizardNewVM::createdMachineFolder() const
{
    return m_strCreatedFolder;
}

void UIWizardNewVM::setCreatedMachineFolder(const QString &strCreatedMachineFolder)
{
    m_strCreatedFolder = strCreatedMachineFolder;
}

QString UIWizardNewVM::detectedOSTypeId() const
{
    AssertReturn(!m_comUnattended.isNull(), QString());
    return m_comUnattended.GetDetectedOSTypeId();
}

const QString &UIWizardNewVM::guestOSFamilyId() const
{
    return m_strGuestOSFamilyId;
}

void UIWizardNewVM::setGuestOSFamilyId(const QString &strGuestOSFamilyId)
{
    m_strGuestOSFamilyId = strGuestOSFamilyId;
}

const CGuestOSType &UIWizardNewVM::guestOSType() const
{
    return m_comGuestOSType;;
}

void UIWizardNewVM::setGuestOSType(const CGuestOSType &guestOSType)
{
    m_comGuestOSType= guestOSType;
}

bool UIWizardNewVM::installGuestAdditions() const
{
    AssertReturn(!m_comUnattended.isNull(), false);
    return m_comUnattended.GetInstallGuestAdditions();
}

void UIWizardNewVM::setInstallGuestAdditions(bool fInstallGA)
{
    AssertReturnVoid(!m_comUnattended.isNull());
    m_comUnattended.SetInstallGuestAdditions(fInstallGA);
    AssertReturnVoid(checkUnattendedInstallError(m_comUnattended));
}

bool UIWizardNewVM::startHeadless() const
{
    return m_fStartHeadless;
}

void UIWizardNewVM::setStartHeadless(bool fStartHeadless)
{
    m_fStartHeadless = fStartHeadless;
}

bool UIWizardNewVM::skipUnattendedInstall() const
{
    return m_fSkipUnattendedInstall;
}

void UIWizardNewVM::setSkipUnattendedInstall(bool fSkipUnattendedInstall)
{
    m_fSkipUnattendedInstall = fSkipUnattendedInstall;
    /* We hide/show unattended install page depending on the value of isUnattendedEnabled: */
    setUnattendedPageVisible(isUnattendedEnabled());
}

bool UIWizardNewVM::EFIEnabled() const
{
    return m_fEFIEnabled;
}

void UIWizardNewVM::setEFIEnabled(bool fEnabled)
{
    m_fEFIEnabled = fEnabled;
}

QString UIWizardNewVM::ISOFilePath() const
{
    AssertReturn(!m_comUnattended.isNull(), QString());
    return m_comUnattended.GetIsoPath();
}

void UIWizardNewVM::setISOFilePath(const QString &strISOFilePath)
{
    AssertReturnVoid(!m_comUnattended.isNull());
    m_comUnattended.SetIsoPath(strISOFilePath);
    AssertReturnVoid(checkUnattendedInstallError(m_comUnattended));

    m_comUnattended.DetectIsoOS();

    const QVector<ULONG> &indices = m_comUnattended.GetDetectedImageIndices();
    QVector<ulong> qIndices;
    for (int i = 0; i < indices.size(); ++i)
        qIndices << indices[i];
    setDetectedWindowsImageNamesAndIndices(m_comUnattended.GetDetectedImageNames(), qIndices);
    /* We hide/show unattended install page depending on the value of isUnattendedEnabled: */
    setUnattendedPageVisible(isUnattendedEnabled());
}

QString UIWizardNewVM::userName() const
{
    AssertReturn(!m_comUnattended.isNull(), QString());
    return m_comUnattended.GetUser();
}

void UIWizardNewVM::setUserName(const QString &strUserName)
{
    AssertReturnVoid(!m_comUnattended.isNull());
    m_comUnattended.SetUser(strUserName);
    AssertReturnVoid(checkUnattendedInstallError(m_comUnattended));
}

QString UIWizardNewVM::password() const
{
    AssertReturn(!m_comUnattended.isNull(), QString());
    return m_comUnattended.GetPassword();
}

void UIWizardNewVM::setPassword(const QString &strPassword)
{
    AssertReturnVoid(!m_comUnattended.isNull());
    m_comUnattended.SetPassword(strPassword);
    AssertReturnVoid(checkUnattendedInstallError(m_comUnattended));
}

QString UIWizardNewVM::guestAdditionsISOPath() const
{
    AssertReturn(!m_comUnattended.isNull(), QString());
    return m_comUnattended.GetAdditionsIsoPath();
}

void UIWizardNewVM::setGuestAdditionsISOPath(const QString &strGAISOPath)
{
    AssertReturnVoid(!m_comUnattended.isNull());
    m_comUnattended.SetAdditionsIsoPath(strGAISOPath);
    AssertReturnVoid(checkUnattendedInstallError(m_comUnattended));
}

QString UIWizardNewVM::hostnameDomainName() const
{
    AssertReturn(!m_comUnattended.isNull(), QString());
    return m_comUnattended.GetHostname();
}

void UIWizardNewVM::setHostnameDomainName(const QString &strHostnameDomain)
{
    AssertReturnVoid(!m_comUnattended.isNull());
    m_comUnattended.SetHostname(strHostnameDomain);
    AssertReturnVoid(checkUnattendedInstallError(m_comUnattended));
}

QString UIWizardNewVM::productKey() const
{
    AssertReturn(!m_comUnattended.isNull(), QString());
    return  m_comUnattended.GetProductKey();
}

void UIWizardNewVM::setProductKey(const QString &productKey)
{
    AssertReturnVoid(!m_comUnattended.isNull());
    m_comUnattended.SetProductKey(productKey);
    AssertReturnVoid(checkUnattendedInstallError(m_comUnattended));
}

int UIWizardNewVM::CPUCount() const
{
    return m_iCPUCount;
}

void UIWizardNewVM::setCPUCount(int iCPUCount)
{
    m_iCPUCount = iCPUCount;
}

int UIWizardNewVM::memorySize() const
{
    return m_iMemorySize;
}

void UIWizardNewVM::setMemorySize(int iMemory)
{
    m_iMemorySize = iMemory;
}


qulonglong UIWizardNewVM::mediumVariant() const
{
    return m_uMediumVariant;
}

void UIWizardNewVM::setMediumVariant(qulonglong uMediumVariant)
{
    m_uMediumVariant = uMediumVariant;
}

const CMediumFormat &UIWizardNewVM::mediumFormat()
{
    return m_comMediumFormat;
}

void UIWizardNewVM::setMediumFormat(const CMediumFormat &mediumFormat)
{
    m_comMediumFormat = mediumFormat;
}

const QString &UIWizardNewVM::mediumPath() const
{
    return m_strMediumPath;
}

void UIWizardNewVM::setMediumPath(const QString &strMediumPath)
{
    m_strMediumPath = strMediumPath;
}

qulonglong UIWizardNewVM::mediumSize() const
{
    return m_uMediumSize;
}

void UIWizardNewVM::setMediumSize(qulonglong uMediumSize)
{
    m_uMediumSize = uMediumSize;
}

SelectedDiskSource UIWizardNewVM::diskSource() const
{
    return m_enmDiskSource;
}

void UIWizardNewVM::setDiskSource(SelectedDiskSource enmDiskSource)
{
    m_enmDiskSource = enmDiskSource;
}

bool UIWizardNewVM::emptyDiskRecommended() const
{
    return m_fEmptyDiskRecommended;
}

void UIWizardNewVM::setEmptyDiskRecommended(bool fEmptyDiskRecommended)
{
    m_fEmptyDiskRecommended = fEmptyDiskRecommended;
}

void UIWizardNewVM::setDetectedWindowsImageNamesAndIndices(const QVector<QString> &names, const QVector<ulong> &ids)
{
    AssertMsg(names.size() == ids.size(),
              ("Sizes of the arrays for names and indices of the detected images should be equal."));
    m_detectedWindowsImageNames = names;
    m_detectedWindowsImageIndices = ids;
}

const QVector<QString> &UIWizardNewVM::detectedWindowsImageNames() const
{
    return m_detectedWindowsImageNames;
}

const QVector<ulong> &UIWizardNewVM::detectedWindowsImageIndices() const
{
    return m_detectedWindowsImageIndices;
}

void UIWizardNewVM::setSelectedWindowImageIndex(ulong uIndex)
{
    AssertReturnVoid(!m_comUnattended.isNull());
    m_comUnattended.SetImageIndex(uIndex);
    AssertReturnVoid(checkUnattendedInstallError(m_comUnattended));
}

ulong UIWizardNewVM::selectedWindowImageIndex() const
{
    AssertReturn(!m_comUnattended.isNull(), 0);
    return m_comUnattended.GetImageIndex();
}

QVector<KMediumVariant> UIWizardNewVM::mediumVariants() const
{
    /* Compose medium-variant: */
    QVector<KMediumVariant> variants(sizeof(qulonglong)*8);
    for (int i = 0; i < variants.size(); ++i)
    {
        qulonglong temp = m_uMediumVariant;
        temp &= UINT64_C(1)<<i;
        variants[i] = (KMediumVariant)temp;
    }
    return variants;
}

bool UIWizardNewVM::isUnattendedEnabled() const
{
    if (m_comUnattended.isNull())
        return false;
    if (m_comUnattended.GetIsoPath().isEmpty())
        return false;
    if (m_fSkipUnattendedInstall)
        return false;
    if (!isUnattendedInstallSupported())
        return false;
    return true;
}

bool UIWizardNewVM::isUnattendedInstallSupported() const
{
    AssertReturn(!m_comUnattended.isNull(), false);
    return m_comUnattended.GetIsUnattendedInstallSupported();
}

bool UIWizardNewVM::isGuestOSTypeWindows() const
{
    return m_strGuestOSFamilyId.contains("windows", Qt::CaseInsensitive);
}

void UIWizardNewVM::setUnattendedPageVisible(bool fVisible)
{
    if (m_iUnattendedInstallPageIndex != -1)
        setPageVisible(m_iUnattendedInstallPageIndex, fVisible);
}

bool UIWizardNewVM::checkUnattendedInstallError(const CUnattended &comUnattended) const
{
    if (!comUnattended.isOk())
    {
        UINotificationMessage::cannotRunUnattendedGuestInstall(comUnattended);
        return false;
    }
    return true;
}
