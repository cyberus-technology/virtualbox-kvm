/* $Id: ConsoleImpl2.cpp $ */
/** @file
 * VBox Console COM Class implementation - VM Configuration Bits.
 *
 * @remark  We've split out the code that the 64-bit VC++ v8 compiler finds
 *          problematic to optimize so we can disable optimizations and later,
 *          perhaps, find a real solution for it (like rewriting the code and
 *          to stop resemble a tonne of spaghetti).
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_MAIN_CONSOLE
#include "LoggingNew.h"

// VBoxNetCfg-win.h needs winsock2.h and thus MUST be included before any other
// header file includes Windows.h.
#if defined(RT_OS_WINDOWS) && defined(VBOX_WITH_NETFLT)
# include <VBox/VBoxNetCfg-win.h>
#endif

#include "ConsoleImpl.h"
#include "DisplayImpl.h"
#include "NvramStoreImpl.h"
#ifdef VBOX_WITH_DRAG_AND_DROP
# include "GuestImpl.h"
# include "GuestDnDPrivate.h"
#endif
#include "VMMDev.h"
#include "Global.h"
#ifdef VBOX_WITH_PCI_PASSTHROUGH
# include "PCIRawDevImpl.h"
#endif

// generated header
#include "SchemaDefs.h"

#include "AutoCaller.h"

#include <iprt/base64.h>
#include <iprt/buildconfig.h>
#include <iprt/ctype.h>
#include <iprt/dir.h>
#include <iprt/file.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include <iprt/system.h>
#include <iprt/cpp/exception.h>
#if 0 /* enable to play with lots of memory. */
# include <iprt/env.h>
#endif
#include <iprt/stream.h>

#include <iprt/http.h>
#include <iprt/socket.h>
#include <iprt/uri.h>

#include <VBox/vmm/vmmr3vtable.h>
#include <VBox/vmm/vmapi.h>
#include <VBox/err.h>
#include <VBox/param.h>
#include <VBox/settings.h> /* For MachineConfigFile::getHostDefaultAudioDriver(). */
#include <VBox/vmm/pdmapi.h> /* For PDMR3DriverAttach/PDMR3DriverDetach. */
#include <VBox/vmm/pdmusb.h> /* For PDMR3UsbCreateEmulatedDevice. */
#include <VBox/vmm/pdmdev.h> /* For PDMAPICMODE enum. */
#include <VBox/vmm/pdmstorageifs.h>
#include <VBox/vmm/gcm.h>
#include <VBox/version.h>
#ifdef VBOX_WITH_SHARED_CLIPBOARD
# include <VBox/HostServices/VBoxClipboardSvc.h>
#endif
#ifdef VBOX_WITH_GUEST_PROPS
# include <VBox/HostServices/GuestPropertySvc.h>
# include <VBox/com/defs.h>
# include <VBox/com/array.h>
# include <vector>
#endif /* VBOX_WITH_GUEST_PROPS */
#include <VBox/intnet.h>

#include <VBox/com/com.h>
#include <VBox/com/string.h>
#include <VBox/com/array.h>

#ifdef VBOX_WITH_NETFLT
# if defined(RT_OS_SOLARIS)
#  include <zone.h>
# elif defined(RT_OS_LINUX)
#  include <unistd.h>
#  include <sys/ioctl.h>
#  include <sys/socket.h>
#  include <linux/types.h>
#  include <linux/if.h>
# elif defined(RT_OS_FREEBSD)
#  include <unistd.h>
#  include <sys/types.h>
#  include <sys/ioctl.h>
#  include <sys/socket.h>
#  include <net/if.h>
#  include <net80211/ieee80211_ioctl.h>
# endif
# if defined(RT_OS_WINDOWS)
#  include <iprt/win/ntddndis.h>
#  include <devguid.h>
# else
#  include <HostNetworkInterfaceImpl.h>
#  include <netif.h>
#  include <stdlib.h>
# endif
#endif /* VBOX_WITH_NETFLT */

#ifdef VBOX_WITH_AUDIO_VRDE
# include "DrvAudioVRDE.h"
#endif
#ifdef VBOX_WITH_AUDIO_RECORDING
# include "DrvAudioRec.h"
#endif
#include "NetworkServiceRunner.h"
#include "BusAssignmentManager.h"
#ifdef VBOX_WITH_EXTPACK
# include "ExtPackManagerImpl.h"
#endif


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static Utf8Str *GetExtraDataBoth(IVirtualBox *pVirtualBox, IMachine *pMachine, const char *pszName, Utf8Str *pStrValue);


/* Darwin compile kludge */
#undef PVM

/* Comment out the following line to remove VMWare compatibility hack. */
#define VMWARE_NET_IN_SLOT_11

/**
 * Translate IDE StorageControllerType_T to string representation.
 */
static const char* controllerString(StorageControllerType_T enmType)
{
    switch (enmType)
    {
        case StorageControllerType_PIIX3:
            return "PIIX3";
        case StorageControllerType_PIIX4:
            return "PIIX4";
        case StorageControllerType_ICH6:
            return "ICH6";
        default:
            return "Unknown";
    }
}

/**
 * Simple class for storing network boot information.
 */
struct BootNic
{
    ULONG          mInstance;
    PCIBusAddress  mPCIAddress;

    ULONG          mBootPrio;
    bool operator < (const BootNic &rhs) const
    {
        ULONG lval = mBootPrio     - 1; /* 0 will wrap around and get the lowest priority. */
        ULONG rval = rhs.mBootPrio - 1;
        return lval < rval; /* Zero compares as highest number (lowest prio). */
    }
};

#ifndef VBOX_WITH_EFI_IN_DD2
static int findEfiRom(IVirtualBox* vbox, FirmwareType_T aFirmwareType, Utf8Str *pEfiRomFile)
{
    Bstr aFilePath, empty;
    BOOL fPresent = FALSE;
    HRESULT hrc = vbox->CheckFirmwarePresent(aFirmwareType, empty.raw(),
                                             empty.asOutParam(), aFilePath.asOutParam(), &fPresent);
    AssertComRCReturn(hrc, Global::vboxStatusCodeFromCOM(hrc));

    if (!fPresent)
    {
        LogRel(("Failed to find an EFI ROM file.\n"));
        return VERR_FILE_NOT_FOUND;
    }

    *pEfiRomFile = Utf8Str(aFilePath);

    return VINF_SUCCESS;
}
#endif

/**
 * @throws HRESULT on extra data retrival error.
 */
static int getSmcDeviceKey(IVirtualBox *pVirtualBox, IMachine *pMachine, Utf8Str *pStrKey, bool *pfGetKeyFromRealSMC)
{
    *pfGetKeyFromRealSMC = false;

    /*
     * The extra data takes precedence (if non-zero).
     */
    GetExtraDataBoth(pVirtualBox, pMachine, "VBoxInternal2/SmcDeviceKey", pStrKey);
    if (pStrKey->isNotEmpty())
        return VINF_SUCCESS;

#ifdef RT_OS_DARWIN

    /*
     * Work done in EFI/DevSmc
     */
    *pfGetKeyFromRealSMC = true;
    int vrc = VINF_SUCCESS;

#else
    /*
     * Is it apple hardware in bootcamp?
     */
    /** @todo implement + test RTSYSDMISTR_MANUFACTURER on all hosts.
     *        Currently falling back on the product name. */
    char szManufacturer[256];
    szManufacturer[0] = '\0';
    RTSystemQueryDmiString(RTSYSDMISTR_MANUFACTURER, szManufacturer, sizeof(szManufacturer));
    if (szManufacturer[0] != '\0')
    {
        if (   !strcmp(szManufacturer, "Apple Computer, Inc.")
            || !strcmp(szManufacturer, "Apple Inc.")
            )
            *pfGetKeyFromRealSMC = true;
    }
    else
    {
        char szProdName[256];
        szProdName[0] = '\0';
        RTSystemQueryDmiString(RTSYSDMISTR_PRODUCT_NAME, szProdName, sizeof(szProdName));
        if (   (   !strncmp(szProdName, RT_STR_TUPLE("Mac"))
                || !strncmp(szProdName, RT_STR_TUPLE("iMac"))
                || !strncmp(szProdName, RT_STR_TUPLE("Xserve"))
               )
            && !strchr(szProdName, ' ')                             /* no spaces */
            && RT_C_IS_DIGIT(szProdName[strlen(szProdName) - 1])    /* version number */
           )
            *pfGetKeyFromRealSMC = true;
    }

    int vrc = VINF_SUCCESS;
#endif

    return vrc;
}


/*
 * VC++ 8 / amd64 has some serious trouble with the next functions.
 * As a temporary measure, we'll drop global optimizations.
 */
#if defined(_MSC_VER) && defined(RT_ARCH_AMD64)
# if _MSC_VER >= RT_MSC_VER_VC80 && _MSC_VER < RT_MSC_VER_VC100
#  pragma optimize("g", off)
# endif
#endif

class ConfigError : public RTCError
{
public:

    ConfigError(const char *pcszFunction,
                int vrc,
                const char *pcszName)
        : RTCError(Utf8StrFmt(Console::tr("%s failed: vrc=%Rrc, pcszName=%s"), pcszFunction, vrc, pcszName)),
          m_vrc(vrc)
    {
        AssertMsgFailed(("%s\n", what())); // in strict mode, hit a breakpoint here
    }

    int m_vrc;
};


/**
 * Helper that calls CFGMR3InsertString and throws an RTCError if that
 * fails (C-string variant).
 * @param   pNode           See CFGMR3InsertString.
 * @param   pcszName        See CFGMR3InsertString.
 * @param   pcszValue       The string value.
 */
void Console::InsertConfigString(PCFGMNODE pNode, const char *pcszName, const char *pcszValue)
{
    int vrc = mpVMM->pfnCFGMR3InsertString(pNode, pcszName, pcszValue);
    if (RT_FAILURE(vrc))
        throw ConfigError("CFGMR3InsertString", vrc, pcszName);
}

/**
 * Helper that calls CFGMR3InsertString and throws an RTCError if that
 * fails (Utf8Str variant).
 * @param   pNode           See CFGMR3InsertStringN.
 * @param   pcszName        See CFGMR3InsertStringN.
 * @param   rStrValue       The string value.
 */
void Console::InsertConfigString(PCFGMNODE pNode, const char *pcszName, const Utf8Str &rStrValue)
{
    int vrc = mpVMM->pfnCFGMR3InsertStringN(pNode, pcszName, rStrValue.c_str(), rStrValue.length());
    if (RT_FAILURE(vrc))
        throw ConfigError("CFGMR3InsertStringLengthKnown", vrc, pcszName);
}

/**
 * Helper that calls CFGMR3InsertString and throws an RTCError if that
 * fails (Bstr variant).
 *
 * @param   pNode           See CFGMR3InsertStringN.
 * @param   pcszName        See CFGMR3InsertStringN.
 * @param   rBstrValue      The string value.
 */
void Console::InsertConfigString(PCFGMNODE pNode, const char *pcszName, const Bstr &rBstrValue)
{
    InsertConfigString(pNode, pcszName, Utf8Str(rBstrValue));
}

/**
 * Helper that calls CFGMR3InsertStringFV and throws an RTCError if that fails.
 * @param   pNode           See CFGMR3InsertStringF.
 * @param   pcszName        See CFGMR3InsertStringF.
 * @param   pszFormat       See CFGMR3InsertStringF.
 * @param   ...             See CFGMR3InsertStringF.
 */
void Console::InsertConfigStringF(PCFGMNODE pNode, const char *pcszName, const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    int vrc = mpVMM->pfnCFGMR3InsertStringFV(pNode, pcszName, pszFormat, va);
    va_end(va);
    if (RT_FAILURE(vrc))
        throw ConfigError("CFGMR3InsertStringFV", vrc, pcszName);
}


/**
 * Helper that calls CFGMR3InsertPassword and throws an RTCError if that
 * fails (Utf8Str variant).
 * @param   pNode           See CFGMR3InsertPasswordN.
 * @param   pcszName        See CFGMR3InsertPasswordN.
 * @param   rStrValue       The string value.
 */
void Console::InsertConfigPassword(PCFGMNODE pNode, const char *pcszName, const Utf8Str &rStrValue)
{
    int vrc = mpVMM->pfnCFGMR3InsertPasswordN(pNode, pcszName, rStrValue.c_str(), rStrValue.length());
    if (RT_FAILURE(vrc))
        throw ConfigError("CFGMR3InsertPasswordLengthKnown", vrc, pcszName);
}

/**
 * Helper that calls CFGMR3InsertBytes and throws an RTCError if that fails.
 *
 * @param   pNode           See CFGMR3InsertBytes.
 * @param   pcszName        See CFGMR3InsertBytes.
 * @param   pvBytes         See CFGMR3InsertBytes.
 * @param   cbBytes         See CFGMR3InsertBytes.
 */
void Console::InsertConfigBytes(PCFGMNODE pNode, const char *pcszName, const void *pvBytes, size_t cbBytes)
{
    int vrc = mpVMM->pfnCFGMR3InsertBytes(pNode, pcszName, pvBytes, cbBytes);
    if (RT_FAILURE(vrc))
        throw ConfigError("CFGMR3InsertBytes", vrc, pcszName);
}

/**
 * Helper that calls CFGMR3InsertInteger and throws an RTCError if that
 * fails.
 *
 * @param   pNode           See CFGMR3InsertInteger.
 * @param   pcszName        See CFGMR3InsertInteger.
 * @param   u64Integer      See CFGMR3InsertInteger.
 */
void Console::InsertConfigInteger(PCFGMNODE pNode, const char *pcszName, uint64_t u64Integer)
{
    int vrc = mpVMM->pfnCFGMR3InsertInteger(pNode, pcszName, u64Integer);
    if (RT_FAILURE(vrc))
        throw ConfigError("CFGMR3InsertInteger", vrc, pcszName);
}

/**
 * Helper that calls CFGMR3InsertNode and throws an RTCError if that fails.
 *
 * @param   pNode           See CFGMR3InsertNode.
 * @param   pcszName        See CFGMR3InsertNode.
 * @param   ppChild         See CFGMR3InsertNode.
 */
void Console::InsertConfigNode(PCFGMNODE pNode, const char *pcszName, PCFGMNODE *ppChild)
{
    int vrc = mpVMM->pfnCFGMR3InsertNode(pNode, pcszName, ppChild);
    if (RT_FAILURE(vrc))
        throw ConfigError("CFGMR3InsertNode", vrc, pcszName);
}

/**
 * Helper that calls CFGMR3InsertNodeF and throws an RTCError if that fails.
 *
 * @param   pNode           See CFGMR3InsertNodeF.
 * @param   ppChild         See CFGMR3InsertNodeF.
 * @param   pszNameFormat   Name format string, see CFGMR3InsertNodeF.
 * @param   ...             Format arguments.
 */
void Console::InsertConfigNodeF(PCFGMNODE pNode, PCFGMNODE *ppChild, const char *pszNameFormat, ...)
{
    va_list va;
    va_start(va, pszNameFormat);
    int vrc = mpVMM->pfnCFGMR3InsertNodeF(pNode, ppChild, "%N", pszNameFormat, &va);
    va_end(va);
    if (RT_FAILURE(vrc))
        throw ConfigError("CFGMR3InsertNodeF", vrc, pszNameFormat);
}

/**
 * Helper that calls CFGMR3RemoveValue and throws an RTCError if that fails.
 *
 * @param   pNode           See CFGMR3RemoveValue.
 * @param   pcszName        See CFGMR3RemoveValue.
 */
void Console::RemoveConfigValue(PCFGMNODE pNode, const char *pcszName)
{
    int vrc = mpVMM->pfnCFGMR3RemoveValue(pNode, pcszName);
    if (RT_FAILURE(vrc))
        throw ConfigError("CFGMR3RemoveValue", vrc, pcszName);
}

/**
 * Gets an extra data value, consulting both machine and global extra data.
 *
 * @throws  HRESULT on failure
 * @returns pStrValue for the callers convenience.
 * @param   pVirtualBox     Pointer to the IVirtualBox interface.
 * @param   pMachine        Pointer to the IMachine interface.
 * @param   pszName         The value to get.
 * @param   pStrValue       Where to return it's value (empty string if not
 *                          found).
 */
static Utf8Str *GetExtraDataBoth(IVirtualBox *pVirtualBox, IMachine *pMachine, const char *pszName, Utf8Str *pStrValue)
{
    pStrValue->setNull();

    Bstr bstrName(pszName);
    Bstr bstrValue;
    HRESULT hrc = pMachine->GetExtraData(bstrName.raw(), bstrValue.asOutParam());
    if (FAILED(hrc))
        throw hrc;
    if (bstrValue.isEmpty())
    {
        hrc = pVirtualBox->GetExtraData(bstrName.raw(), bstrValue.asOutParam());
        if (FAILED(hrc))
            throw hrc;
    }

    if (bstrValue.isNotEmpty())
        *pStrValue = bstrValue;
    return pStrValue;
}


/** Helper that finds out the next HBA port used
 */
static LONG GetNextUsedPort(LONG aPortUsed[30], LONG lBaseVal, uint32_t u32Size)
{
    LONG lNextPortUsed = 30;
    for (size_t j = 0; j < u32Size; ++j)
    {
        if (   aPortUsed[j] >  lBaseVal
            && aPortUsed[j] <= lNextPortUsed)
           lNextPortUsed = aPortUsed[j];
    }
    return lNextPortUsed;
}

#define MAX_BIOS_LUN_COUNT   4

int Console::SetBiosDiskInfo(ComPtr<IMachine> pMachine, PCFGMNODE pCfg, PCFGMNODE pBiosCfg,
                             Bstr controllerName, const char * const s_apszBiosConfig[4])
{
    RT_NOREF(pCfg);
    HRESULT             hrc;
#define MAX_DEVICES     30
#define H()     AssertLogRelMsgReturn(!FAILED(hrc), ("hrc=%Rhrc\n", hrc), VERR_MAIN_CONFIG_CONSTRUCTOR_COM_ERROR)

    LONG lPortLUN[MAX_BIOS_LUN_COUNT];
    LONG lPortUsed[MAX_DEVICES];
    uint32_t u32HDCount = 0;

    /* init to max value */
    lPortLUN[0] = MAX_DEVICES;

    com::SafeIfaceArray<IMediumAttachment> atts;
    hrc = pMachine->GetMediumAttachmentsOfController(controllerName.raw(),
                                        ComSafeArrayAsOutParam(atts));  H();
    size_t uNumAttachments = atts.size();
    if (uNumAttachments > MAX_DEVICES)
    {
        LogRel(("Number of Attachments > Max=%d.\n", uNumAttachments));
        uNumAttachments = MAX_DEVICES;
    }

    /* Find the relevant ports/IDs, i.e the ones to which a HD is attached. */
    for (size_t j = 0; j < uNumAttachments; ++j)
    {
        IMediumAttachment *pMediumAtt = atts[j];
        LONG lPortNum = 0;
        hrc = pMediumAtt->COMGETTER(Port)(&lPortNum);                   H();
        if (SUCCEEDED(hrc))
        {
            DeviceType_T lType;
            hrc = pMediumAtt->COMGETTER(Type)(&lType);                  H();
            if (SUCCEEDED(hrc) && lType == DeviceType_HardDisk)
            {
                /* find min port number used for HD */
                if (lPortNum < lPortLUN[0])
                    lPortLUN[0] = lPortNum;
                lPortUsed[u32HDCount++] = lPortNum;
                LogFlowFunc(("HD port Count=%d\n", u32HDCount));
            }
        }
    }


    /* Pick only the top 4 used HD Ports as CMOS doesn't have space
     * to save details for all 30 ports
     */
    uint32_t u32MaxPortCount = MAX_BIOS_LUN_COUNT;
    if (u32HDCount < MAX_BIOS_LUN_COUNT)
        u32MaxPortCount = u32HDCount;
    for (size_t j = 1; j < u32MaxPortCount; j++)
        lPortLUN[j] = GetNextUsedPort(lPortUsed, lPortLUN[j-1], u32HDCount);
    if (pBiosCfg)
    {
        for (size_t j = 0; j < u32MaxPortCount; j++)
        {
            InsertConfigInteger(pBiosCfg, s_apszBiosConfig[j], lPortLUN[j]);
            LogFlowFunc(("Top %d HBA ports = %s, %d\n", j, s_apszBiosConfig[j], lPortLUN[j]));
        }
    }
    return VINF_SUCCESS;
}

#ifdef VBOX_WITH_PCI_PASSTHROUGH
HRESULT Console::i_attachRawPCIDevices(PUVM pUVM, BusAssignmentManager *pBusMgr, PCFGMNODE pDevices)
{
# ifndef VBOX_WITH_EXTPACK
    RT_NOREF(pUVM);
# endif
    HRESULT hrc = S_OK;
    PCFGMNODE pInst, pCfg, pLunL0, pLunL1;

    SafeIfaceArray<IPCIDeviceAttachment> assignments;
    ComPtr<IMachine> aMachine = i_machine();

    hrc = aMachine->COMGETTER(PCIDeviceAssignments)(ComSafeArrayAsOutParam(assignments));
    if (   hrc != S_OK
        || assignments.size() < 1)
        return hrc;

    /*
     * PCI passthrough is only available if the proper ExtPack is installed.
     *
     * Note. Configuring PCI passthrough here and providing messages about
     * the missing extpack isn't exactly clean, but it is a necessary evil
     * to patch over legacy compatability issues introduced by the new
     * distribution model.
     */
# ifdef VBOX_WITH_EXTPACK
    static const char *s_pszPCIRawExtPackName = "Oracle VM VirtualBox Extension Pack";
    if (!mptrExtPackManager->i_isExtPackUsable(s_pszPCIRawExtPackName))
        /* Always fatal! */
        return pVMM->pfnVMR3SetError(pUVM, VERR_NOT_FOUND, RT_SRC_POS,
                                     N_("Implementation of the PCI passthrough framework not found!\n"
                                        "The VM cannot be started. To fix this problem, either "
                                        "install the '%s' or disable PCI passthrough via VBoxManage"),
                                     s_pszPCIRawExtPackName);
# endif

    /* Now actually add devices */
    PCFGMNODE pPCIDevs = NULL;

    if (assignments.size() > 0)
    {
        InsertConfigNode(pDevices, "pciraw",  &pPCIDevs);

        PCFGMNODE pRoot = CFGMR3GetParent(pDevices); Assert(pRoot);

        /* Tell PGM to tell GPCIRaw about guest mappings. */
        CFGMR3InsertNode(pRoot, "PGM", NULL);
        InsertConfigInteger(CFGMR3GetChild(pRoot, "PGM"), "PciPassThrough", 1);

        /*
         * Currently, using IOMMU needed for PCI passthrough
         * requires RAM preallocation.
         */
        /** @todo check if we can lift this requirement */
        CFGMR3RemoveValue(pRoot, "RamPreAlloc");
        InsertConfigInteger(pRoot, "RamPreAlloc", 1);
    }

    for (size_t iDev = 0; iDev < assignments.size(); iDev++)
    {
        ComPtr<IPCIDeviceAttachment> const assignment = assignments[iDev];

        LONG host;
        hrc = assignment->COMGETTER(HostAddress)(&host);            H();
        LONG guest;
        hrc = assignment->COMGETTER(GuestAddress)(&guest);          H();
        Bstr bstrDevName;
        hrc = assignment->COMGETTER(Name)(bstrDevName.asOutParam());   H();

        InsertConfigNodeF(pPCIDevs, &pInst, "%d", iDev);
        InsertConfigInteger(pInst, "Trusted", 1);

        PCIBusAddress HostPCIAddress(host);
        Assert(HostPCIAddress.valid());
        InsertConfigNode(pInst,        "Config",  &pCfg);
        InsertConfigString(pCfg,       "DeviceName",  bstrDevName);

        InsertConfigInteger(pCfg,      "DetachHostDriver",  1);
        InsertConfigInteger(pCfg,      "HostPCIBusNo",      HostPCIAddress.miBus);
        InsertConfigInteger(pCfg,      "HostPCIDeviceNo",   HostPCIAddress.miDevice);
        InsertConfigInteger(pCfg,      "HostPCIFunctionNo", HostPCIAddress.miFn);

        PCIBusAddress GuestPCIAddress(guest);
        Assert(GuestPCIAddress.valid());
        hrc = pBusMgr->assignHostPCIDevice("pciraw", pInst, HostPCIAddress, GuestPCIAddress, true);
        if (hrc != S_OK)
            return hrc;

        InsertConfigInteger(pCfg,      "GuestPCIBusNo",      GuestPCIAddress.miBus);
        InsertConfigInteger(pCfg,      "GuestPCIDeviceNo",   GuestPCIAddress.miDevice);
        InsertConfigInteger(pCfg,      "GuestPCIFunctionNo", GuestPCIAddress.miFn);

        /* the driver */
        InsertConfigNode(pInst,        "LUN#0",   &pLunL0);
        InsertConfigString(pLunL0,     "Driver", "pciraw");
        InsertConfigNode(pLunL0,       "AttachedDriver", &pLunL1);

        /* the Main driver */
        InsertConfigString(pLunL1,     "Driver", "MainPciRaw");
        InsertConfigNode(pLunL1,       "Config", &pCfg);
        PCIRawDev *pMainDev = new PCIRawDev(this);
# error This is not allowed any more
        InsertConfigInteger(pCfg,      "Object", (uintptr_t)pMainDev);
    }

    return hrc;
}
#endif


/**
 * Updates the device type for a LED.
 *
 * @param   penmSubTypeEntry    The sub-type entry to update.
 * @param   enmNewType          The new type.
 */
void Console::i_setLedType(DeviceType_T *penmSubTypeEntry, DeviceType_T enmNewType)
{
    /*
     * ASSUMES no race conditions here wrt concurrent type updating.
     */
    if (*penmSubTypeEntry != enmNewType)
    {
        *penmSubTypeEntry = enmNewType;
        ASMAtomicIncU32(&muLedGen);
    }
}


/**
 * Allocate a set of LEDs.
 *
 * This grabs a maLedSets entry and populates it with @a cLeds.
 *
 * @returns Index into maLedSets.
 * @param   cLeds       The number of LEDs in the set.
 * @param   fTypes      Bitmask of DeviceType_T values, e.g.
 *                      RT_BIT_32(DeviceType_Network).
 * @param   ppaSubTypes When not NULL, subtypes for each LED and return the
 *                      array pointer here.
 */
uint32_t Console::i_allocateDriverLeds(uint32_t cLeds, uint32_t fTypes, DeviceType_T **ppaSubTypes)
{
    Assert(cLeds > 0);
    Assert(cLeds < 1024);  /* Adjust if any driver supports >=1024 units! */
    Assert(!(fTypes & (RT_BIT_32(DeviceType_Null) | ~(RT_BIT_32(DeviceType_End) - 1))));

    /* Preallocate the arrays we need, bunching them together. */
    AssertCompile((unsigned)DeviceType_Null == 0);
    PPDMLED volatile *papLeds = (PPDMLED volatile *)RTMemAllocZ(  (sizeof(PPDMLED) + (ppaSubTypes ? sizeof(**ppaSubTypes) : 0))
                                                                * cLeds);
    AssertStmt(papLeds, throw E_OUTOFMEMORY);

    /* Take the LED lock in allocation mode and see if there are more LED set entries availalbe. */
    {
        AutoWriteLock alock(mLedLock COMMA_LOCKVAL_SRC_POS);
        uint32_t const idxLedSet = mcLedSets;
        if (idxLedSet < RT_ELEMENTS(maLedSets))
        {
            /* Initialize the set and return the index. */
            PLEDSET pLS = &maLedSets[idxLedSet];
            pLS->papLeds = papLeds;
            pLS->cLeds = cLeds;
            pLS->fTypes = fTypes;
            if (ppaSubTypes)
                 *ppaSubTypes = pLS->paSubTypes = (DeviceType_T *)&papLeds[cLeds];
            else
                pLS->paSubTypes = NULL;

            mcLedSets = idxLedSet + 1;
            LogRel2(("return idxLedSet=%d (mcLedSets=%u out of max %zu)\n", idxLedSet, mcLedSets, RT_ELEMENTS(maLedSets)));
            return idxLedSet;
        }
    }

    RTMemFree((void *)papLeds);
    AssertFailed();
    throw ConfigError("AllocateDriverPapLeds", VERR_OUT_OF_RANGE, "Too many LED sets");
}


/**
 * @throws ConfigError and std::bad_alloc.
 */
void Console::i_attachStatusDriver(PCFGMNODE pCtlInst, uint32_t fTypes, uint32_t cLeds, DeviceType_T **ppaSubTypes,
                                   Console::MediumAttachmentMap *pmapMediumAttachments,
                                   const char *pcszDevice, unsigned uInstance)
{
    PCFGMNODE pLunL0;
    InsertConfigNode(pCtlInst,  "LUN#999", &pLunL0);
    InsertConfigString(pLunL0,  "Driver",               "MainStatus");
    PCFGMNODE pCfg;
    InsertConfigNode(pLunL0,    "Config", &pCfg);
    uint32_t const iLedSet = i_allocateDriverLeds(cLeds, fTypes, ppaSubTypes);
    InsertConfigInteger(pCfg,   "iLedSet", iLedSet);

    InsertConfigInteger(pCfg,   "HasMediumAttachments", pmapMediumAttachments != NULL);
    if (pmapMediumAttachments)
    {
        AssertPtr(pcszDevice);
        InsertConfigStringF(pCfg, "DeviceInstance", "%s/%u", pcszDevice, uInstance);
    }
    InsertConfigInteger(pCfg,   "First",    0);
    InsertConfigInteger(pCfg,   "Last",     cLeds - 1);
}


/**
 * @throws ConfigError and std::bad_alloc.
 */
void Console::i_attachStatusDriver(PCFGMNODE pCtlInst, DeviceType_T enmType, uint32_t cLeds /*= 1*/)
{
    Assert(enmType > DeviceType_Null && enmType < DeviceType_End);
    i_attachStatusDriver(pCtlInst, RT_BIT_32(enmType), cLeds, NULL, NULL, NULL, 0);
}


/**
 * Construct the VM configuration tree (CFGM).
 *
 * This is a callback for VMR3Create() call. It is called from CFGMR3Init() in
 * the emulation thread (EMT). Any per thread COM/XPCOM initialization is done
 * here.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   pVM         The cross context VM handle.
 * @param   pVMM        The VMM ring-3 vtable.
 * @param   pvConsole   Pointer to the VMPowerUpTask object.
 *
 * @note Locks the Console object for writing.
 */
/*static*/ DECLCALLBACK(int)
Console::i_configConstructor(PUVM pUVM, PVM pVM, PCVMMR3VTABLE pVMM, void *pvConsole)
{
    LogFlowFuncEnter();

    AssertReturn(pvConsole, VERR_INVALID_POINTER);
    ComObjPtr<Console> pConsole = static_cast<Console *>(pvConsole);

    AutoCaller autoCaller(pConsole);
    AssertComRCReturn(autoCaller.hrc(), VERR_ACCESS_DENIED);

    /* lock the console because we widely use internal fields and methods */
    AutoWriteLock alock(pConsole COMMA_LOCKVAL_SRC_POS);

    /*
     * Set the VM handle and do the rest of the job in an worker method so we
     * can easily reset the VM handle on failure.
     */
    pConsole->mpUVM = pUVM;
    pVMM->pfnVMR3RetainUVM(pUVM);
    int vrc;
    try
    {
        vrc = pConsole->i_configConstructorInner(pUVM, pVM, pVMM, &alock);
    }
    catch (...)
    {
        vrc = VERR_UNEXPECTED_EXCEPTION;
    }
    if (RT_FAILURE(vrc))
    {
        pConsole->mpUVM = NULL;
        pVMM->pfnVMR3ReleaseUVM(pUVM);
    }

    return vrc;
}


/**
 * Worker for configConstructor.
 *
 * @return  VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   pVM         The cross context VM handle.
 * @param   pVMM        The VMM vtable.
 * @param   pAlock      The automatic lock instance.  This is for when we have
 *                      to leave it in order to avoid deadlocks (ext packs and
 *                      more).
 */
int Console::i_configConstructorInner(PUVM pUVM, PVM pVM, PCVMMR3VTABLE pVMM, AutoWriteLock *pAlock)
{
    RT_NOREF(pVM /* when everything is disabled */);
    VMMDev         *pVMMDev   = m_pVMMDev; Assert(pVMMDev);
    ComPtr<IMachine> pMachine = i_machine();

    int             vrc;
    HRESULT         hrc;
    Utf8Str         strTmp;
    Bstr            bstr;

#define H()         AssertLogRelMsgReturn(!FAILED(hrc), ("hrc=%Rhrc\n", hrc), VERR_MAIN_CONFIG_CONSTRUCTOR_COM_ERROR)

    /*
     * Get necessary objects and frequently used parameters.
     */
    ComPtr<IVirtualBox> virtualBox;
    hrc = pMachine->COMGETTER(Parent)(virtualBox.asOutParam());                             H();

    ComPtr<IHost> host;
    hrc = virtualBox->COMGETTER(Host)(host.asOutParam());                                   H();

    ComPtr<ISystemProperties> systemProperties;
    hrc = virtualBox->COMGETTER(SystemProperties)(systemProperties.asOutParam());           H();

    ComPtr<IBIOSSettings> biosSettings;
    hrc = pMachine->COMGETTER(BIOSSettings)(biosSettings.asOutParam());                     H();

    ComPtr<INvramStore> nvramStore;
    hrc = pMachine->COMGETTER(NonVolatileStore)(nvramStore.asOutParam());                   H();

    hrc = pMachine->COMGETTER(HardwareUUID)(bstr.asOutParam());                             H();
    RTUUID HardwareUuid;
    vrc = RTUuidFromUtf16(&HardwareUuid, bstr.raw());
    AssertRCReturn(vrc, vrc);

    ULONG cRamMBs;
    hrc = pMachine->COMGETTER(MemorySize)(&cRamMBs);                                        H();
#if 0 /* enable to play with lots of memory. */
    if (RTEnvExist("VBOX_RAM_SIZE"))
        cRamMBs = RTStrToUInt64(RTEnvGet("VBOX_RAM_SIZE"));
#endif
    uint64_t const cbRam   = cRamMBs * (uint64_t)_1M;
    uint32_t cbRamHole     = MM_RAM_HOLE_SIZE_DEFAULT;
    uint64_t uMcfgBase     = 0;
    uint32_t cbMcfgLength  = 0;

    ParavirtProvider_T enmParavirtProvider;
    hrc = pMachine->GetEffectiveParavirtProvider(&enmParavirtProvider);                     H();

    Bstr strParavirtDebug;
    hrc = pMachine->COMGETTER(ParavirtDebug)(strParavirtDebug.asOutParam());                H();

    BOOL fIOAPIC;
    uint32_t uIoApicPciAddress = NIL_PCIBDF;
    hrc = biosSettings->COMGETTER(IOAPICEnabled)(&fIOAPIC);                                 H();

    ChipsetType_T chipsetType;
    hrc = pMachine->COMGETTER(ChipsetType)(&chipsetType);                                   H();
    if (chipsetType == ChipsetType_ICH9)
    {
        /* We'd better have 0x10000000 region, to cover 256 buses but this put
         * too much load on hypervisor heap. Linux 4.8 currently complains with
         * ``acpi PNP0A03:00: [Firmware Info]: MMCONFIG for domain 0000 [bus 00-3f]
         *   only partially covers this bridge'' */
        cbMcfgLength = 0x4000000; //0x10000000;
        cbRamHole += cbMcfgLength;
        uMcfgBase = _4G - cbRamHole;
    }

    /* Get the CPU profile name. */
    Bstr bstrCpuProfile;
    hrc = pMachine->COMGETTER(CPUProfile)(bstrCpuProfile.asOutParam());                     H();

    /* Check if long mode is enabled. */
    BOOL fIsGuest64Bit;
    hrc = pMachine->GetCPUProperty(CPUPropertyType_LongMode, &fIsGuest64Bit);               H();

    /*
     * Figure out the IOMMU config.
     */
#if defined(VBOX_WITH_IOMMU_AMD) || defined(VBOX_WITH_IOMMU_INTEL)
    IommuType_T enmIommuType;
    hrc = pMachine->COMGETTER(IommuType)(&enmIommuType);                                    H();

    /* Resolve 'automatic' type to an Intel or AMD IOMMU based on the host CPU. */
    if (enmIommuType == IommuType_Automatic)
    {
        if (   bstrCpuProfile.startsWith("AMD")
            || bstrCpuProfile.startsWith("Quad-Core AMD")
            || bstrCpuProfile.startsWith("Hygon"))
            enmIommuType = IommuType_AMD;
        else if (bstrCpuProfile.startsWith("Intel"))
        {
            if (   bstrCpuProfile.equals("Intel 8086")
                || bstrCpuProfile.equals("Intel 80186")
                || bstrCpuProfile.equals("Intel 80286")
                || bstrCpuProfile.equals("Intel 80386")
                || bstrCpuProfile.equals("Intel 80486"))
                enmIommuType = IommuType_None;
            else
                enmIommuType = IommuType_Intel;
        }
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
        else if (ASMIsAmdCpu())
            enmIommuType = IommuType_AMD;
        else if (ASMIsIntelCpu())
            enmIommuType = IommuType_Intel;
# endif
        else
        {
            /** @todo Should we handle other CPUs like Shanghai, VIA etc. here? */
            LogRel(("WARNING! Unrecognized CPU type, IOMMU disabled.\n"));
            enmIommuType = IommuType_None;
        }
    }

    if (enmIommuType == IommuType_AMD)
    {
# ifdef VBOX_WITH_IOMMU_AMD
        /*
         * Reserve the specific PCI address of the "SB I/O APIC" when using
         * an AMD IOMMU. Required by Linux guests, see @bugref{9654#c23}.
         */
        uIoApicPciAddress = VBOX_PCI_BDF_SB_IOAPIC;
# else
        LogRel(("WARNING! AMD IOMMU not supported, IOMMU disabled.\n"));
        enmIommuType = IommuType_None;
# endif
    }

    if (enmIommuType == IommuType_Intel)
    {
# ifdef VBOX_WITH_IOMMU_INTEL
        /*
         * Reserve a unique PCI address for the I/O APIC when using
         * an Intel IOMMU. For convenience we use the same address as
         * we do on AMD, see @bugref{9967#c13}.
         */
        uIoApicPciAddress = VBOX_PCI_BDF_SB_IOAPIC;
# else
        LogRel(("WARNING! Intel IOMMU not supported, IOMMU disabled.\n"));
        enmIommuType = IommuType_None;
# endif
    }

    if (   enmIommuType == IommuType_AMD
        || enmIommuType == IommuType_Intel)
    {
        if (chipsetType != ChipsetType_ICH9)
            return pVMM->pfnVMR3SetError(pUVM, VERR_INVALID_PARAMETER, RT_SRC_POS,
                                         N_("IOMMU uses MSIs which requires the ICH9 chipset implementation."));
        if (!fIOAPIC)
            return pVMM->pfnVMR3SetError(pUVM, VERR_INVALID_PARAMETER, RT_SRC_POS,
                                         N_("IOMMU requires an I/O APIC for remapping interrupts."));
    }
#else
    IommuType_T const enmIommuType = IommuType_None;
#endif

    /* Instantiate the bus assignment manager. */
    Assert(enmIommuType != IommuType_Automatic);
    BusAssignmentManager *pBusMgr = mBusMgr = BusAssignmentManager::createInstance(pVMM, chipsetType, enmIommuType);

    ULONG cCpus = 1;
    hrc = pMachine->COMGETTER(CPUCount)(&cCpus);                                            H();

    ULONG ulCpuExecutionCap = 100;
    hrc = pMachine->COMGETTER(CPUExecutionCap)(&ulCpuExecutionCap);                         H();

    Bstr osTypeId;
    hrc = pMachine->COMGETTER(OSTypeId)(osTypeId.asOutParam());                             H();
    LogRel(("Guest OS type: '%s'\n", Utf8Str(osTypeId).c_str()));

    APICMode_T apicMode;
    hrc = biosSettings->COMGETTER(APICMode)(&apicMode);                                     H();
    uint32_t uFwAPIC;
    switch (apicMode)
    {
        case APICMode_Disabled:
            uFwAPIC = 0;
            break;
        case APICMode_APIC:
            uFwAPIC = 1;
            break;
        case APICMode_X2APIC:
            uFwAPIC = 2;
            break;
        default:
            AssertMsgFailed(("Invalid APICMode=%d\n", apicMode));
            uFwAPIC = 1;
            break;
    }

    ComPtr<IGuestOSType> pGuestOSType;
    virtualBox->GetGuestOSType(osTypeId.raw(), pGuestOSType.asOutParam());

    BOOL fOsXGuest = FALSE;
    BOOL fWinGuest = FALSE;
    BOOL fOs2Guest = FALSE;
    BOOL fW9xGuest = FALSE;
    BOOL fDosGuest = FALSE;
    if (!pGuestOSType.isNull())
    {
        Bstr guestTypeFamilyId;
        hrc = pGuestOSType->COMGETTER(FamilyId)(guestTypeFamilyId.asOutParam());            H();
        fOsXGuest = guestTypeFamilyId == Bstr("MacOS");
        fWinGuest = guestTypeFamilyId == Bstr("Windows");
        fOs2Guest = osTypeId.startsWith("OS2");
        fW9xGuest = osTypeId.startsWith("Windows9");    /* Does not include Windows Me. */
        fDosGuest = osTypeId.startsWith("DOS") || osTypeId.startsWith("Windows31");
    }

    ULONG maxNetworkAdapters;
    hrc = systemProperties->GetMaxNetworkAdapters(chipsetType, &maxNetworkAdapters);        H();

    /*
     * Get root node first.
     * This is the only node in the tree.
     */
    PCFGMNODE pRoot = pVMM->pfnCFGMR3GetRootU(pUVM);
    Assert(pRoot);

    // catching throws from InsertConfigString and friends.
    try
    {

        /*
         * Set the root (and VMM) level values.
         */
        hrc = pMachine->COMGETTER(Name)(bstr.asOutParam());                                 H();
        InsertConfigString(pRoot,  "Name",                 bstr);
        InsertConfigBytes(pRoot,   "UUID", &HardwareUuid, sizeof(HardwareUuid));
        InsertConfigInteger(pRoot, "RamSize",              cbRam);
        InsertConfigInteger(pRoot, "RamHoleSize",          cbRamHole);
        InsertConfigInteger(pRoot, "NumCPUs",              cCpus);
        InsertConfigInteger(pRoot, "CpuExecutionCap",      ulCpuExecutionCap);
        InsertConfigInteger(pRoot, "TimerMillies",         10);

        BOOL fPageFusion = FALSE;
        hrc = pMachine->COMGETTER(PageFusionEnabled)(&fPageFusion);                         H();
        InsertConfigInteger(pRoot, "PageFusionAllowed",    fPageFusion); /* boolean */

        /* Not necessary, but makes sure this setting ends up in the release log. */
        ULONG ulBalloonSize = 0;
        hrc = pMachine->COMGETTER(MemoryBalloonSize)(&ulBalloonSize);                       H();
        InsertConfigInteger(pRoot, "MemBalloonSize",       ulBalloonSize);

        /*
         * EM values (before CPUM as it may need to set IemExecutesAll).
         */
        PCFGMNODE pEM;
        InsertConfigNode(pRoot, "EM", &pEM);

        /* Triple fault behavior. */
        BOOL fTripleFaultReset = false;
        hrc = pMachine->GetCPUProperty(CPUPropertyType_TripleFaultReset, &fTripleFaultReset); H();
        InsertConfigInteger(pEM, "TripleFaultReset", fTripleFaultReset);

        /*
         * CPUM values.
         */
        PCFGMNODE pCPUM;
        InsertConfigNode(pRoot, "CPUM", &pCPUM);
        PCFGMNODE pIsaExts;
        InsertConfigNode(pCPUM, "IsaExts", &pIsaExts);

        /* Host CPUID leaf overrides. */
        for (uint32_t iOrdinal = 0; iOrdinal < _4K; iOrdinal++)
        {
            ULONG uLeaf, uSubLeaf, uEax, uEbx, uEcx, uEdx;
            hrc = pMachine->GetCPUIDLeafByOrdinal(iOrdinal, &uLeaf, &uSubLeaf, &uEax, &uEbx, &uEcx, &uEdx);
            if (hrc == E_INVALIDARG)
                break;
            H();
            PCFGMNODE pLeaf;
            InsertConfigNode(pCPUM, Utf8StrFmt("HostCPUID/%RX32", uLeaf).c_str(), &pLeaf);
            /** @todo Figure out how to tell the VMM about uSubLeaf   */
            InsertConfigInteger(pLeaf, "eax", uEax);
            InsertConfigInteger(pLeaf, "ebx", uEbx);
            InsertConfigInteger(pLeaf, "ecx", uEcx);
            InsertConfigInteger(pLeaf, "edx", uEdx);
        }

        /* We must limit CPUID count for Windows NT 4, as otherwise it stops
        with error 0x3e (MULTIPROCESSOR_CONFIGURATION_NOT_SUPPORTED). */
        if (osTypeId == "WindowsNT4")
        {
            LogRel(("Limiting CPUID leaf count for NT4 guests\n"));
            InsertConfigInteger(pCPUM, "NT4LeafLimit", true);
        }

        if (fOsXGuest)
        {
            /* Expose extended MWAIT features to Mac OS X guests. */
            LogRel(("Using MWAIT extensions\n"));
            InsertConfigInteger(pIsaExts, "MWaitExtensions", true);

            /* Fake the CPU family/model so the guest works.  This is partly
               because older mac releases really doesn't work on newer cpus,
               and partly because mac os x expects more from systems with newer
               cpus (MSRs, power features, whatever). */
            uint32_t uMaxIntelFamilyModelStep = UINT32_MAX;
            if (   osTypeId == "MacOS"
                || osTypeId == "MacOS_64")
                uMaxIntelFamilyModelStep = RT_MAKE_U32_FROM_U8(1, 23, 6, 0); /* Penryn / X5482. */
            else if (   osTypeId == "MacOS106"
                     || osTypeId == "MacOS106_64")
                uMaxIntelFamilyModelStep = RT_MAKE_U32_FROM_U8(1, 23, 6, 0); /* Penryn / X5482 */
            else if (   osTypeId == "MacOS107"
                     || osTypeId == "MacOS107_64")
                uMaxIntelFamilyModelStep = RT_MAKE_U32_FROM_U8(1, 23, 6, 0); /* Penryn / X5482 */ /** @todo figure out
                                                                                what is required here. */
            else if (   osTypeId == "MacOS108"
                     || osTypeId == "MacOS108_64")
                uMaxIntelFamilyModelStep = RT_MAKE_U32_FROM_U8(1, 23, 6, 0); /* Penryn / X5482 */ /** @todo figure out
                                                                                what is required here. */
            else if (   osTypeId == "MacOS109"
                     || osTypeId == "MacOS109_64")
                uMaxIntelFamilyModelStep = RT_MAKE_U32_FROM_U8(1, 23, 6, 0); /* Penryn / X5482 */ /** @todo figure
                                                                                out what is required here. */
            if (uMaxIntelFamilyModelStep != UINT32_MAX)
                InsertConfigInteger(pCPUM, "MaxIntelFamilyModelStep", uMaxIntelFamilyModelStep);
        }

        /* CPU Portability level, */
        ULONG uCpuIdPortabilityLevel = 0;
        hrc = pMachine->COMGETTER(CPUIDPortabilityLevel)(&uCpuIdPortabilityLevel);          H();
        InsertConfigInteger(pCPUM, "PortableCpuIdLevel", uCpuIdPortabilityLevel);

        /* Physical Address Extension (PAE) */
        BOOL fEnablePAE = false;
        hrc = pMachine->GetCPUProperty(CPUPropertyType_PAE, &fEnablePAE);                   H();
        fEnablePAE |= fIsGuest64Bit;
        InsertConfigInteger(pRoot, "EnablePAE", fEnablePAE);

        /* 64-bit guests (long mode) */
        InsertConfigInteger(pCPUM, "Enable64bit", fIsGuest64Bit);

        /* APIC/X2APIC configuration */
        BOOL fEnableAPIC = true;
        BOOL fEnableX2APIC = true;
        hrc = pMachine->GetCPUProperty(CPUPropertyType_APIC, &fEnableAPIC);                 H();
        hrc = pMachine->GetCPUProperty(CPUPropertyType_X2APIC, &fEnableX2APIC);             H();
        if (fEnableX2APIC)
            Assert(fEnableAPIC);

        /* CPUM profile name. */
        InsertConfigString(pCPUM, "GuestCpuName", bstrCpuProfile);

        /*
         * Temporary(?) hack to make sure we emulate the ancient 16-bit CPUs
         * correctly.   There are way too many #UDs we'll miss using VT-x,
         * raw-mode or qemu for the 186 and 286, while we'll get undefined opcodes
         * dead wrong on 8086 (see http://www.os2museum.com/wp/undocumented-8086-opcodes/).
         */
        if (   bstrCpuProfile.equals("Intel 80386") /* just for now */
            || bstrCpuProfile.equals("Intel 80286")
            || bstrCpuProfile.equals("Intel 80186")
            || bstrCpuProfile.equals("Nec V20")
            || bstrCpuProfile.equals("Intel 8086") )
        {
            InsertConfigInteger(pEM, "IemExecutesAll", true);
            if (!bstrCpuProfile.equals("Intel 80386"))
            {
                fEnableAPIC = false;
                fIOAPIC     = false;
            }
            fEnableX2APIC = false;
        }

        /* Adjust firmware APIC handling to stay within the VCPU limits. */
        if (uFwAPIC == 2 && !fEnableX2APIC)
        {
            if (fEnableAPIC)
                uFwAPIC = 1;
            else
                uFwAPIC = 0;
            LogRel(("Limiting the firmware APIC level from x2APIC to %s\n", fEnableAPIC ? "APIC" : "Disabled"));
        }
        else if (uFwAPIC == 1 && !fEnableAPIC)
        {
            uFwAPIC = 0;
            LogRel(("Limiting the firmware APIC level from APIC to Disabled\n"));
        }

        /* Speculation Control. */
        BOOL fSpecCtrl = FALSE;
        hrc = pMachine->GetCPUProperty(CPUPropertyType_SpecCtrl, &fSpecCtrl);      H();
        InsertConfigInteger(pCPUM, "SpecCtrl", fSpecCtrl);

        /* Nested VT-x / AMD-V. */
        BOOL fNestedHWVirt = FALSE;
        hrc = pMachine->GetCPUProperty(CPUPropertyType_HWVirt, &fNestedHWVirt);      H();
        InsertConfigInteger(pCPUM, "NestedHWVirt", fNestedHWVirt ? true : false);

        /*
         * Hardware virtualization extensions.
         */
        /* Sanitize valid/useful APIC combinations, see @bugref{8868}. */
        if (!fEnableAPIC)
        {
            if (fIsGuest64Bit)
                return pVMM->pfnVMR3SetError(pUVM, VERR_INVALID_PARAMETER, RT_SRC_POS,
                                             N_("Cannot disable the APIC for a 64-bit guest."));
            if (cCpus > 1)
                return pVMM->pfnVMR3SetError(pUVM, VERR_INVALID_PARAMETER, RT_SRC_POS,
                                             N_("Cannot disable the APIC for an SMP guest."));
            if (fIOAPIC)
                return pVMM->pfnVMR3SetError(pUVM, VERR_INVALID_PARAMETER, RT_SRC_POS,
                                             N_("Cannot disable the APIC when the I/O APIC is present."));
        }

        BOOL fHMEnabled;
        hrc = pMachine->GetHWVirtExProperty(HWVirtExPropertyType_Enabled, &fHMEnabled);     H();
        if (cCpus > 1 && !fHMEnabled)
        {
            LogRel(("Forced fHMEnabled to TRUE by SMP guest.\n"));
            fHMEnabled = TRUE;
        }

        BOOL fHMForced;
        fHMEnabled = fHMForced = TRUE;
        LogRel(("fHMForced=true - No raw-mode support in this build!\n"));
        if (!fHMForced) /* No need to query if already forced above. */
        {
            hrc = pMachine->GetHWVirtExProperty(HWVirtExPropertyType_Force, &fHMForced); H();
            if (fHMForced)
                LogRel(("fHMForced=true - HWVirtExPropertyType_Force\n"));
        }
        InsertConfigInteger(pRoot, "HMEnabled", fHMEnabled);

        /* /HM/xyz */
        PCFGMNODE pHM;
        InsertConfigNode(pRoot, "HM", &pHM);
        InsertConfigInteger(pHM, "HMForced", fHMForced);
        if (fHMEnabled)
        {
            /* Indicate whether 64-bit guests are supported or not. */
            InsertConfigInteger(pHM, "64bitEnabled", fIsGuest64Bit);

            /** @todo Not exactly pretty to check strings; VBOXOSTYPE would be better,
                but that requires quite a bit of API change in Main. */
            if (    fIOAPIC
                &&  (   osTypeId == "WindowsNT4"
                     || osTypeId == "Windows2000"
                     || osTypeId == "WindowsXP"
                     || osTypeId == "Windows2003"))
            {
                /* Only allow TPR patching for NT, Win2k, XP and Windows Server 2003. (32 bits mode)
                 * We may want to consider adding more guest OSes (Solaris) later on.
                 */
                InsertConfigInteger(pHM, "TPRPatchingEnabled", 1);
            }
        }

        /* HWVirtEx exclusive mode */
        BOOL fHMExclusive = true;
        hrc = systemProperties->COMGETTER(ExclusiveHwVirt)(&fHMExclusive);                  H();
        InsertConfigInteger(pHM, "Exclusive", fHMExclusive);

        /* Nested paging (VT-x/AMD-V) */
        BOOL fEnableNestedPaging = false;
        hrc = pMachine->GetHWVirtExProperty(HWVirtExPropertyType_NestedPaging, &fEnableNestedPaging); H();
        InsertConfigInteger(pHM, "EnableNestedPaging", fEnableNestedPaging);

        /* Large pages; requires nested paging */
        BOOL fEnableLargePages = false;
        hrc = pMachine->GetHWVirtExProperty(HWVirtExPropertyType_LargePages, &fEnableLargePages); H();
        InsertConfigInteger(pHM, "EnableLargePages", fEnableLargePages);

        /* VPID (VT-x) */
        BOOL fEnableVPID = false;
        hrc = pMachine->GetHWVirtExProperty(HWVirtExPropertyType_VPID, &fEnableVPID);       H();
        InsertConfigInteger(pHM, "EnableVPID", fEnableVPID);

        /* Unrestricted execution aka UX (VT-x) */
        BOOL fEnableUX = false;
        hrc = pMachine->GetHWVirtExProperty(HWVirtExPropertyType_UnrestrictedExecution, &fEnableUX); H();
        InsertConfigInteger(pHM, "EnableUX", fEnableUX);

        /* Virtualized VMSAVE/VMLOAD (AMD-V) */
        BOOL fVirtVmsaveVmload = true;
        hrc = host->GetProcessorFeature(ProcessorFeature_VirtVmsaveVmload, &fVirtVmsaveVmload);     H();
        InsertConfigInteger(pHM, "SvmVirtVmsaveVmload", fVirtVmsaveVmload);

        /* Indirect branch prediction boundraries. */
        BOOL fIBPBOnVMExit = false;
        hrc = pMachine->GetCPUProperty(CPUPropertyType_IBPBOnVMExit, &fIBPBOnVMExit); H();
        InsertConfigInteger(pHM, "IBPBOnVMExit", fIBPBOnVMExit);

        BOOL fIBPBOnVMEntry = false;
        hrc = pMachine->GetCPUProperty(CPUPropertyType_IBPBOnVMEntry, &fIBPBOnVMEntry); H();
        InsertConfigInteger(pHM, "IBPBOnVMEntry", fIBPBOnVMEntry);

        BOOL fSpecCtrlByHost = false;
        hrc = pMachine->GetCPUProperty(CPUPropertyType_SpecCtrlByHost, &fSpecCtrlByHost); H();
        InsertConfigInteger(pHM, "SpecCtrlByHost", fSpecCtrlByHost);

        BOOL fL1DFlushOnSched = true;
        hrc = pMachine->GetCPUProperty(CPUPropertyType_L1DFlushOnEMTScheduling, &fL1DFlushOnSched); H();
        InsertConfigInteger(pHM, "L1DFlushOnSched", fL1DFlushOnSched);

        BOOL fL1DFlushOnVMEntry = false;
        hrc = pMachine->GetCPUProperty(CPUPropertyType_L1DFlushOnVMEntry, &fL1DFlushOnVMEntry); H();
        InsertConfigInteger(pHM, "L1DFlushOnVMEntry", fL1DFlushOnVMEntry);

        BOOL fMDSClearOnSched = true;
        hrc = pMachine->GetCPUProperty(CPUPropertyType_MDSClearOnEMTScheduling, &fMDSClearOnSched); H();
        InsertConfigInteger(pHM, "MDSClearOnSched", fMDSClearOnSched);

        BOOL fMDSClearOnVMEntry = false;
        hrc = pMachine->GetCPUProperty(CPUPropertyType_MDSClearOnVMEntry, &fMDSClearOnVMEntry); H();
        InsertConfigInteger(pHM, "MDSClearOnVMEntry", fMDSClearOnVMEntry);

        /* Reset overwrite. */
        mfTurnResetIntoPowerOff = GetExtraDataBoth(virtualBox, pMachine,
                                                   "VBoxInternal2/TurnResetIntoPowerOff", &strTmp)->equals("1");
        if (mfTurnResetIntoPowerOff)
            InsertConfigInteger(pRoot, "PowerOffInsteadOfReset", 1);

        /* Use NEM rather than HM. */
        BOOL fUseNativeApi = false;
        hrc = pMachine->GetHWVirtExProperty(HWVirtExPropertyType_UseNativeApi, &fUseNativeApi); H();
        InsertConfigInteger(pHM, "UseNEMInstead", fUseNativeApi);

        /* Enable workaround for missing TLB flush for OS/2 guests, see ticketref:20625. */
        if (osTypeId.startsWith("OS2"))
            InsertConfigInteger(pHM, "MissingOS2TlbFlushWorkaround", 1);

        /*
         * NEM
         */
        PCFGMNODE pNEM;
        InsertConfigNode(pRoot, "NEM", &pNEM);
        InsertConfigInteger(pNEM, "Allow64BitGuests", fIsGuest64Bit);

        /*
         * Paravirt. provider.
         */
        PCFGMNODE pParavirtNode;
        InsertConfigNode(pRoot, "GIM", &pParavirtNode);
        const char *pcszParavirtProvider;
        bool fGimDeviceNeeded = true;
        switch (enmParavirtProvider)
        {
            case ParavirtProvider_None:
                pcszParavirtProvider = "None";
                fGimDeviceNeeded = false;
                break;

            case ParavirtProvider_Minimal:
                pcszParavirtProvider = "Minimal";
                break;

            case ParavirtProvider_HyperV:
                pcszParavirtProvider = "HyperV";
                break;

            case ParavirtProvider_KVM:
                pcszParavirtProvider = "KVM";
                break;

            default:
                AssertMsgFailed(("Invalid enmParavirtProvider=%d\n", enmParavirtProvider));
                return pVMM->pfnVMR3SetError(pUVM, VERR_INVALID_PARAMETER, RT_SRC_POS, N_("Invalid paravirt. provider '%d'"),
                                             enmParavirtProvider);
        }
        InsertConfigString(pParavirtNode, "Provider", pcszParavirtProvider);

        /*
         * Parse paravirt. debug options.
         */
        bool         fGimDebug          = false;
        com::Utf8Str strGimDebugAddress = "127.0.0.1";
        uint32_t     uGimDebugPort      = 50000;
        if (strParavirtDebug.isNotEmpty())
        {
            /* Hyper-V debug options. */
            if (enmParavirtProvider == ParavirtProvider_HyperV)
            {
                bool         fGimHvDebug = false;
                com::Utf8Str strGimHvVendor;
                bool         fGimHvVsIf = false;
                bool         fGimHvHypercallIf = false;

                size_t       uPos = 0;
                com::Utf8Str strDebugOptions = strParavirtDebug;
                com::Utf8Str strKey;
                com::Utf8Str strVal;
                while ((uPos = strDebugOptions.parseKeyValue(strKey, strVal, uPos)) != com::Utf8Str::npos)
                {
                    if (strKey == "enabled")
                    {
                        if (strVal.toUInt32() == 1)
                        {
                            /* Apply defaults.
                               The defaults are documented in the user manual,
                               changes need to be reflected accordingly. */
                            fGimHvDebug       = true;
                            strGimHvVendor    = "Microsoft Hv";
                            fGimHvVsIf        = true;
                            fGimHvHypercallIf = false;
                        }
                        /* else: ignore, i.e. don't assert below with 'enabled=0'. */
                    }
                    else if (strKey == "address")
                        strGimDebugAddress = strVal;
                    else if (strKey == "port")
                        uGimDebugPort = strVal.toUInt32();
                    else if (strKey == "vendor")
                        strGimHvVendor = strVal;
                    else if (strKey == "vsinterface")
                        fGimHvVsIf = RT_BOOL(strVal.toUInt32());
                    else if (strKey == "hypercallinterface")
                        fGimHvHypercallIf = RT_BOOL(strVal.toUInt32());
                    else
                    {
                        AssertMsgFailed(("Unrecognized Hyper-V debug option '%s'\n", strKey.c_str()));
                        return pVMM->pfnVMR3SetError(pUVM, VERR_INVALID_PARAMETER, RT_SRC_POS,
                                                     N_("Unrecognized Hyper-V debug option '%s' in '%s'"), strKey.c_str(),
                                                     strDebugOptions.c_str());
                    }
                }

                /* Update HyperV CFGM node with active debug options. */
                if (fGimHvDebug)
                {
                    PCFGMNODE pHvNode;
                    InsertConfigNode(pParavirtNode, "HyperV", &pHvNode);
                    InsertConfigString(pHvNode,  "VendorID", strGimHvVendor);
                    InsertConfigInteger(pHvNode, "VSInterface", fGimHvVsIf ? 1 : 0);
                    InsertConfigInteger(pHvNode, "HypercallDebugInterface", fGimHvHypercallIf ? 1 : 0);
                    fGimDebug = true;
                }
            }
        }

        /*
         * Guest Compatibility Manager.
         */
        PCFGMNODE   pGcmNode;
        uint32_t    u32FixerSet = 0;
        InsertConfigNode(pRoot, "GCM", &pGcmNode);
        /* OS/2 and Win9x guests can run DOS apps so they get
         * the DOS specific fixes as well.
         */
        if (fOs2Guest)
            u32FixerSet = GCMFIXER_DBZ_DOS | GCMFIXER_DBZ_OS2;
        else if (fW9xGuest)
            u32FixerSet = GCMFIXER_DBZ_DOS | GCMFIXER_DBZ_WIN9X;
        else if (fDosGuest)
            u32FixerSet = GCMFIXER_DBZ_DOS;
        InsertConfigInteger(pGcmNode, "FixerSet", u32FixerSet);


        /*
         * MM values.
         */
        PCFGMNODE pMM;
        InsertConfigNode(pRoot, "MM", &pMM);
        InsertConfigInteger(pMM, "CanUseLargerHeap", chipsetType == ChipsetType_ICH9);

        /*
         * PDM config.
         *  Load drivers in VBoxC.[so|dll]
         */
        PCFGMNODE pPDM;
        PCFGMNODE pNode;
        PCFGMNODE pMod;
        InsertConfigNode(pRoot,    "PDM", &pPDM);
        InsertConfigNode(pPDM,     "Devices", &pNode);
        InsertConfigNode(pPDM,     "Drivers", &pNode);
        InsertConfigNode(pNode,    "VBoxC", &pMod);
#ifdef VBOX_WITH_XPCOM
        // VBoxC is located in the components subdirectory
        char szPathVBoxC[RTPATH_MAX];
        vrc = RTPathAppPrivateArch(szPathVBoxC, RTPATH_MAX - sizeof("/components/VBoxC"));  AssertRC(vrc);
        strcat(szPathVBoxC, "/components/VBoxC");
        InsertConfigString(pMod,   "Path",  szPathVBoxC);
#else
        InsertConfigString(pMod,   "Path",  "VBoxC");
#endif


        /*
         * Block cache settings.
         */
        PCFGMNODE pPDMBlkCache;
        InsertConfigNode(pPDM, "BlkCache", &pPDMBlkCache);

        /* I/O cache size */
        ULONG ioCacheSize = 5;
        hrc = pMachine->COMGETTER(IOCacheSize)(&ioCacheSize);                               H();
        InsertConfigInteger(pPDMBlkCache, "CacheSize", ioCacheSize * _1M);

        /*
         * Bandwidth groups.
         */
        ComPtr<IBandwidthControl> bwCtrl;

        hrc = pMachine->COMGETTER(BandwidthControl)(bwCtrl.asOutParam());                   H();

        com::SafeIfaceArray<IBandwidthGroup> bwGroups;
        hrc = bwCtrl->GetAllBandwidthGroups(ComSafeArrayAsOutParam(bwGroups));              H();

        PCFGMNODE pAc;
        InsertConfigNode(pPDM, "AsyncCompletion", &pAc);
        PCFGMNODE pAcFile;
        InsertConfigNode(pAc,  "File", &pAcFile);
        PCFGMNODE pAcFileBwGroups;
        InsertConfigNode(pAcFile,  "BwGroups", &pAcFileBwGroups);
#ifdef VBOX_WITH_NETSHAPER
        PCFGMNODE pNetworkShaper;
        InsertConfigNode(pPDM, "NetworkShaper",  &pNetworkShaper);
        PCFGMNODE pNetworkBwGroups;
        InsertConfigNode(pNetworkShaper, "BwGroups", &pNetworkBwGroups);
#endif /* VBOX_WITH_NETSHAPER */

        for (size_t i = 0; i < bwGroups.size(); i++)
        {
            Bstr strName;
            hrc = bwGroups[i]->COMGETTER(Name)(strName.asOutParam());                       H();
            if (strName.isEmpty())
                return pVMM->pfnVMR3SetError(pUVM, VERR_CFGM_NO_NODE, RT_SRC_POS, N_("No bandwidth group name specified"));

            BandwidthGroupType_T enmType = BandwidthGroupType_Null;
            hrc = bwGroups[i]->COMGETTER(Type)(&enmType);                                   H();
            LONG64 cMaxBytesPerSec = 0;
            hrc = bwGroups[i]->COMGETTER(MaxBytesPerSec)(&cMaxBytesPerSec);                 H();

            if (enmType == BandwidthGroupType_Disk)
            {
                PCFGMNODE pBwGroup;
                InsertConfigNode(pAcFileBwGroups, Utf8Str(strName).c_str(), &pBwGroup);
                InsertConfigInteger(pBwGroup, "Max", cMaxBytesPerSec);
                InsertConfigInteger(pBwGroup, "Start", cMaxBytesPerSec);
                InsertConfigInteger(pBwGroup, "Step", 0);
            }
#ifdef VBOX_WITH_NETSHAPER
            else if (enmType == BandwidthGroupType_Network)
            {
                /* Network bandwidth groups. */
                PCFGMNODE pBwGroup;
                InsertConfigNode(pNetworkBwGroups, Utf8Str(strName).c_str(), &pBwGroup);
                InsertConfigInteger(pBwGroup, "Max", cMaxBytesPerSec);
            }
#endif /* VBOX_WITH_NETSHAPER */
        }

        /*
         * Devices
         */
        PCFGMNODE pDevices = NULL;      /* /Devices */
        PCFGMNODE pDev = NULL;          /* /Devices/Dev/ */
        PCFGMNODE pInst = NULL;         /* /Devices/Dev/0/ */
        PCFGMNODE pCfg = NULL;          /* /Devices/Dev/.../Config/ */
        PCFGMNODE pLunL0 = NULL;        /* /Devices/Dev/0/LUN#0/ */
        PCFGMNODE pLunL1 = NULL;        /* /Devices/Dev/0/LUN#0/AttachedDriver/ */
        PCFGMNODE pBiosCfg = NULL;      /* /Devices/pcbios/0/Config/ */
        PCFGMNODE pNetBootCfg = NULL;   /* /Devices/pcbios/0/Config/NetBoot/ */

        InsertConfigNode(pRoot, "Devices", &pDevices);

        /*
         * GIM Device
         */
        if (fGimDeviceNeeded)
        {
            InsertConfigNode(pDevices, "GIMDev", &pDev);
            InsertConfigNode(pDev,     "0", &pInst);
            InsertConfigInteger(pInst, "Trusted",              1); /* boolean */
            //InsertConfigNode(pInst,    "Config", &pCfg);

            if (fGimDebug)
            {
                InsertConfigNode(pInst,     "LUN#998", &pLunL0);
                InsertConfigString(pLunL0,  "Driver", "UDP");
                InsertConfigNode(pLunL0,    "Config", &pLunL1);
                InsertConfigString(pLunL1,  "ServerAddress", strGimDebugAddress);
                InsertConfigInteger(pLunL1, "ServerPort", uGimDebugPort);
            }
        }

        /*
         * PC Arch.
         */
        InsertConfigNode(pDevices, "pcarch", &pDev);
        InsertConfigNode(pDev,     "0", &pInst);
        InsertConfigInteger(pInst, "Trusted",              1); /* boolean */
        InsertConfigNode(pInst,    "Config", &pCfg);

        /*
         * The time offset
         */
        LONG64 timeOffset;
        hrc = biosSettings->COMGETTER(TimeOffset)(&timeOffset);                             H();
        PCFGMNODE pTMNode;
        InsertConfigNode(pRoot, "TM", &pTMNode);
        InsertConfigInteger(pTMNode, "UTCOffset", timeOffset * 1000000);

        /*
         * DMA
         */
        InsertConfigNode(pDevices, "8237A", &pDev);
        InsertConfigNode(pDev,     "0", &pInst);
        InsertConfigInteger(pInst, "Trusted", 1); /* boolean */

        /*
         * PCI buses.
         */
        uint32_t uIocPCIAddress, uHbcPCIAddress;
        switch (chipsetType)
        {
            default:
                AssertFailed();
                RT_FALL_THRU();
            case ChipsetType_PIIX3:
                /* Create the base for adding bridges on demand */
                InsertConfigNode(pDevices, "pcibridge", NULL);

                InsertConfigNode(pDevices, "pci", &pDev);
                uHbcPCIAddress = (0x0 << 16) | 0;
                uIocPCIAddress = (0x1 << 16) | 0; // ISA controller
                break;
            case ChipsetType_ICH9:
                /* Create the base for adding bridges on demand */
                InsertConfigNode(pDevices, "ich9pcibridge", NULL);

                InsertConfigNode(pDevices, "ich9pci", &pDev);
                uHbcPCIAddress = (0x1e << 16) | 0;
                uIocPCIAddress = (0x1f << 16) | 0; // LPC controller
                break;
        }
        InsertConfigNode(pDev,     "0", &pInst);
        InsertConfigInteger(pInst, "Trusted",              1); /* boolean */
        InsertConfigNode(pInst,    "Config", &pCfg);
        InsertConfigInteger(pCfg, "IOAPIC", fIOAPIC);
        if (chipsetType == ChipsetType_ICH9)
        {
            /* Provide MCFG info */
            InsertConfigInteger(pCfg,  "McfgBase",   uMcfgBase);
            InsertConfigInteger(pCfg,  "McfgLength", cbMcfgLength);

#ifdef VBOX_WITH_PCI_PASSTHROUGH
            /* Add PCI passthrough devices */
            hrc = i_attachRawPCIDevices(pUVM, pBusMgr, pDevices);                           H();
#endif

            if (enmIommuType == IommuType_AMD)
            {
                /* AMD IOMMU. */
                InsertConfigNode(pDevices, "iommu-amd", &pDev);
                InsertConfigNode(pDev,     "0", &pInst);
                InsertConfigInteger(pInst, "Trusted",   1); /* boolean */
                InsertConfigNode(pInst,    "Config", &pCfg);
                hrc = pBusMgr->assignPCIDevice("iommu-amd", pInst);                         H();

                /* The AMD IOMMU device needs to know which PCI slot it's in, see @bugref{9654#c104}. */
                {
                    PCIBusAddress Address;
                    if (pBusMgr->findPCIAddress("iommu-amd", 0, Address))
                    {
                        uint32_t const u32IommuAddress = (Address.miDevice << 16) | Address.miFn;
                        InsertConfigInteger(pCfg, "PCIAddress", u32IommuAddress);
                    }
                    else
                        return pVMM->pfnVMR3SetError(pUVM, VERR_INVALID_PARAMETER, RT_SRC_POS,
                                                     N_("Failed to find PCI address of the assigned IOMMU device!"));
                }

                PCIBusAddress PCIAddr = PCIBusAddress((int32_t)uIoApicPciAddress);
                hrc = pBusMgr->assignPCIDevice("sb-ioapic", NULL /* pCfg */, PCIAddr, true /*fGuestAddressRequired*/);  H();
            }
            else if (enmIommuType == IommuType_Intel)
            {
                /* Intel IOMMU. */
                InsertConfigNode(pDevices, "iommu-intel", &pDev);
                InsertConfigNode(pDev,     "0", &pInst);
                InsertConfigInteger(pInst, "Trusted",   1); /* boolean */
                InsertConfigNode(pInst,    "Config", &pCfg);
                hrc = pBusMgr->assignPCIDevice("iommu-intel", pInst);                       H();

                PCIBusAddress PCIAddr = PCIBusAddress((int32_t)uIoApicPciAddress);
                hrc = pBusMgr->assignPCIDevice("sb-ioapic", NULL /* pCfg */, PCIAddr, true /*fGuestAddressRequired*/);  H();
            }
        }

        /*
         * Enable the following devices: HPET, SMC and LPC on MacOS X guests or on ICH9 chipset
         */

        /*
         * High Precision Event Timer (HPET)
         */
        BOOL fHPETEnabled;
        /* Other guests may wish to use HPET too, but MacOS X not functional without it */
        hrc = pMachine->COMGETTER(HPETEnabled)(&fHPETEnabled);                              H();
        /* so always enable HPET in extended profile */
        fHPETEnabled |= fOsXGuest;
        /* HPET is always present on ICH9 */
        fHPETEnabled |= (chipsetType == ChipsetType_ICH9);
        if (fHPETEnabled)
        {
            InsertConfigNode(pDevices, "hpet", &pDev);
            InsertConfigNode(pDev,     "0", &pInst);
            InsertConfigInteger(pInst, "Trusted",   1); /* boolean */
            InsertConfigNode(pInst,    "Config", &pCfg);
            InsertConfigInteger(pCfg,  "ICH9", (chipsetType == ChipsetType_ICH9) ? 1 : 0);  /* boolean */
        }

        /*
         * System Management Controller (SMC)
         */
        BOOL fSmcEnabled;
        fSmcEnabled = fOsXGuest;
        if (fSmcEnabled)
        {
            InsertConfigNode(pDevices, "smc", &pDev);
            InsertConfigNode(pDev,     "0", &pInst);
            InsertConfigInteger(pInst, "Trusted",   1); /* boolean */
            InsertConfigNode(pInst,    "Config", &pCfg);

            bool fGetKeyFromRealSMC;
            Utf8Str strKey;
            vrc = getSmcDeviceKey(virtualBox, pMachine, &strKey, &fGetKeyFromRealSMC);
            AssertRCReturn(vrc, vrc);

            if (!fGetKeyFromRealSMC)
                InsertConfigString(pCfg,   "DeviceKey", strKey);
            InsertConfigInteger(pCfg,  "GetKeyFromRealSMC", fGetKeyFromRealSMC);
        }

        /*
         * Low Pin Count (LPC) bus
         */
        BOOL fLpcEnabled;
        /** @todo implement appropriate getter */
        fLpcEnabled = fOsXGuest || (chipsetType == ChipsetType_ICH9);
        if (fLpcEnabled)
        {
            InsertConfigNode(pDevices, "lpc", &pDev);
            InsertConfigNode(pDev,     "0", &pInst);
            hrc = pBusMgr->assignPCIDevice("lpc", pInst);                                   H();
            InsertConfigInteger(pInst, "Trusted",   1); /* boolean */
        }

        BOOL fShowRtc;
        fShowRtc = fOsXGuest || (chipsetType == ChipsetType_ICH9);

        /*
         * PS/2 keyboard & mouse.
         */
        InsertConfigNode(pDevices, "pckbd", &pDev);
        InsertConfigNode(pDev,     "0", &pInst);
        InsertConfigInteger(pInst, "Trusted",              1); /* boolean */
        InsertConfigNode(pInst,    "Config", &pCfg);

        KeyboardHIDType_T aKbdHID;
        hrc = pMachine->COMGETTER(KeyboardHIDType)(&aKbdHID);                               H();
        if (aKbdHID != KeyboardHIDType_None)
        {
            InsertConfigNode(pInst,    "LUN#0", &pLunL0);
            InsertConfigString(pLunL0, "Driver",               "KeyboardQueue");
            InsertConfigNode(pLunL0,   "Config", &pCfg);
            InsertConfigInteger(pCfg,  "QueueSize",            64);

            InsertConfigNode(pLunL0,   "AttachedDriver", &pLunL1);
            InsertConfigString(pLunL1, "Driver",               "MainKeyboard");
        }

        PointingHIDType_T aPointingHID;
        hrc = pMachine->COMGETTER(PointingHIDType)(&aPointingHID);                          H();
        if (aPointingHID != PointingHIDType_None)
        {
            InsertConfigNode(pInst,    "LUN#1", &pLunL0);
            InsertConfigString(pLunL0, "Driver",               "MouseQueue");
            InsertConfigNode(pLunL0,   "Config", &pCfg);
            InsertConfigInteger(pCfg, "QueueSize",            128);

            InsertConfigNode(pLunL0,   "AttachedDriver", &pLunL1);
            InsertConfigString(pLunL1, "Driver",               "MainMouse");
        }

        /*
         * i8254 Programmable Interval Timer And Dummy Speaker
         */
        InsertConfigNode(pDevices, "i8254", &pDev);
        InsertConfigNode(pDev,     "0", &pInst);
        InsertConfigNode(pInst,    "Config", &pCfg);
#ifdef DEBUG
        InsertConfigInteger(pInst, "Trusted",              1); /* boolean */
#endif

        /*
         * i8259 Programmable Interrupt Controller.
         */
        InsertConfigNode(pDevices, "i8259", &pDev);
        InsertConfigNode(pDev,     "0", &pInst);
        InsertConfigInteger(pInst, "Trusted",              1); /* boolean */
        InsertConfigNode(pInst,    "Config", &pCfg);

        /*
         * Advanced Programmable Interrupt Controller.
         * SMP: Each CPU has a LAPIC, but we have a single device representing all LAPICs states,
         *      thus only single insert
         */
        if (fEnableAPIC)
        {
            InsertConfigNode(pDevices, "apic", &pDev);
            InsertConfigNode(pDev, "0", &pInst);
            InsertConfigInteger(pInst, "Trusted",          1); /* boolean */
            InsertConfigNode(pInst,    "Config", &pCfg);
            InsertConfigInteger(pCfg,  "IOAPIC", fIOAPIC);
            PDMAPICMODE enmAPICMode = PDMAPICMODE_APIC;
            if (fEnableX2APIC)
                enmAPICMode = PDMAPICMODE_X2APIC;
            else if (!fEnableAPIC)
                enmAPICMode = PDMAPICMODE_NONE;
            InsertConfigInteger(pCfg,  "Mode", enmAPICMode);
            InsertConfigInteger(pCfg,  "NumCPUs", cCpus);

            if (fIOAPIC)
            {
                /*
                 * I/O Advanced Programmable Interrupt Controller.
                 */
                InsertConfigNode(pDevices, "ioapic", &pDev);
                InsertConfigNode(pDev,     "0", &pInst);
                InsertConfigInteger(pInst, "Trusted",      1); /* boolean */
                InsertConfigNode(pInst,    "Config", &pCfg);
                InsertConfigInteger(pCfg,  "NumCPUs", cCpus);
                if (enmIommuType == IommuType_AMD)
                    InsertConfigInteger(pCfg, "PCIAddress", uIoApicPciAddress);
                else if (enmIommuType == IommuType_Intel)
                {
                    InsertConfigString(pCfg, "ChipType", "DMAR");
                    InsertConfigInteger(pCfg, "PCIAddress", uIoApicPciAddress);
                }
            }
        }

        /*
         * RTC MC146818.
         */
        InsertConfigNode(pDevices, "mc146818", &pDev);
        InsertConfigNode(pDev,     "0", &pInst);
        InsertConfigNode(pInst,    "Config", &pCfg);
        BOOL fRTCUseUTC;
        hrc = pMachine->COMGETTER(RTCUseUTC)(&fRTCUseUTC);                                  H();
        InsertConfigInteger(pCfg,  "UseUTC", fRTCUseUTC ? 1 : 0);

        /*
         * VGA.
         */
        ComPtr<IGraphicsAdapter> pGraphicsAdapter;
        hrc = pMachine->COMGETTER(GraphicsAdapter)(pGraphicsAdapter.asOutParam());           H();
        GraphicsControllerType_T enmGraphicsController;
        hrc = pGraphicsAdapter->COMGETTER(GraphicsControllerType)(&enmGraphicsController);          H();
        switch (enmGraphicsController)
        {
            case GraphicsControllerType_Null:
                break;
#ifdef VBOX_WITH_VMSVGA
            case GraphicsControllerType_VMSVGA:
                InsertConfigInteger(pHM, "LovelyMesaDrvWorkaround", 1); /* hits someone else logging backdoor. */
                InsertConfigInteger(pNEM, "LovelyMesaDrvWorkaround", 1); /* hits someone else logging backdoor. */
                RT_FALL_THROUGH();
            case GraphicsControllerType_VBoxSVGA:
#endif
            case GraphicsControllerType_VBoxVGA:
                vrc = i_configGraphicsController(pDevices, enmGraphicsController, pBusMgr, pMachine, pGraphicsAdapter, biosSettings,
                                                 RT_BOOL(fHMEnabled));
                if (FAILED(vrc))
                    return vrc;
                break;
            default:
                AssertMsgFailed(("Invalid graphicsController=%d\n", enmGraphicsController));
                return pVMM->pfnVMR3SetError(pUVM, VERR_INVALID_PARAMETER, RT_SRC_POS,
                                             N_("Invalid graphics controller type '%d'"), enmGraphicsController);
        }

        /*
         * Firmware.
         */
        FirmwareType_T eFwType =  FirmwareType_BIOS;
        hrc = pMachine->COMGETTER(FirmwareType)(&eFwType);                                  H();

#ifdef VBOX_WITH_EFI
        BOOL fEfiEnabled = (eFwType >= FirmwareType_EFI) && (eFwType <= FirmwareType_EFIDUAL);
#else
        BOOL fEfiEnabled = false;
#endif
        if (!fEfiEnabled)
        {
            /*
             * PC Bios.
             */
            InsertConfigNode(pDevices, "pcbios", &pDev);
            InsertConfigNode(pDev,     "0", &pInst);
            InsertConfigInteger(pInst, "Trusted",              1); /* boolean */
            InsertConfigNode(pInst,    "Config", &pBiosCfg);
            InsertConfigInteger(pBiosCfg,  "NumCPUs",              cCpus);
            InsertConfigString(pBiosCfg,   "HardDiskDevice",       "piix3ide");
            InsertConfigString(pBiosCfg,   "FloppyDevice",         "i82078");
            InsertConfigInteger(pBiosCfg,  "IOAPIC",               fIOAPIC);
            InsertConfigInteger(pBiosCfg,  "APIC",                 uFwAPIC);
            BOOL fPXEDebug;
            hrc = biosSettings->COMGETTER(PXEDebugEnabled)(&fPXEDebug);                     H();
            InsertConfigInteger(pBiosCfg,  "PXEDebug",             fPXEDebug);
            InsertConfigBytes(pBiosCfg,    "UUID", &HardwareUuid,sizeof(HardwareUuid));
            BOOL fUuidLe;
            hrc = biosSettings->COMGETTER(SMBIOSUuidLittleEndian)(&fUuidLe);                H();
            InsertConfigInteger(pBiosCfg,  "UuidLe",               fUuidLe);
            InsertConfigNode(pBiosCfg,     "NetBoot", &pNetBootCfg);
            InsertConfigInteger(pBiosCfg,  "McfgBase",   uMcfgBase);
            InsertConfigInteger(pBiosCfg,  "McfgLength", cbMcfgLength);

            AssertMsgReturn(SchemaDefs::MaxBootPosition <= 9, ("Too many boot devices %d\n", SchemaDefs::MaxBootPosition),
                            VERR_INVALID_PARAMETER);

            for (ULONG pos = 1; pos <= SchemaDefs::MaxBootPosition; ++pos)
            {
                DeviceType_T enmBootDevice;
                hrc = pMachine->GetBootOrder(pos, &enmBootDevice);                          H();

                char szParamName[] = "BootDeviceX";
                szParamName[sizeof(szParamName) - 2] = (char)(pos - 1 + '0');

                const char *pszBootDevice;
                switch (enmBootDevice)
                {
                    case DeviceType_Null:
                        pszBootDevice = "NONE";
                        break;
                    case DeviceType_HardDisk:
                        pszBootDevice = "IDE";
                        break;
                    case DeviceType_DVD:
                        pszBootDevice = "DVD";
                        break;
                    case DeviceType_Floppy:
                        pszBootDevice = "FLOPPY";
                        break;
                    case DeviceType_Network:
                        pszBootDevice = "LAN";
                        break;
                    default:
                        AssertMsgFailed(("Invalid enmBootDevice=%d\n", enmBootDevice));
                        return pVMM->pfnVMR3SetError(pUVM, VERR_INVALID_PARAMETER, RT_SRC_POS,
                                                     N_("Invalid boot device '%d'"), enmBootDevice);
                }
                InsertConfigString(pBiosCfg, szParamName, pszBootDevice);
            }

            /** @todo @bugref{7145}: We might want to enable this by default for new VMs. For now,
             *        this is required for Windows 2012 guests. */
            if (osTypeId == "Windows2012_64")
                InsertConfigInteger(pBiosCfg, "DmiExposeMemoryTable", 1); /* boolean */
        }
        else
        {
            /* Autodetect firmware type, basing on guest type */
            if (eFwType == FirmwareType_EFI)
                eFwType = fIsGuest64Bit ? FirmwareType_EFI64 : FirmwareType_EFI32;
            bool const f64BitEntry = eFwType == FirmwareType_EFI64;

            Assert(eFwType == FirmwareType_EFI64 || eFwType == FirmwareType_EFI32 || eFwType == FirmwareType_EFIDUAL);
#ifdef VBOX_WITH_EFI_IN_DD2
            const char *pszEfiRomFile = eFwType == FirmwareType_EFIDUAL ? "VBoxEFIDual.fd"
                                      : eFwType == FirmwareType_EFI32   ? "VBoxEFI32.fd"
                                      :                                   "VBoxEFI64.fd";
#else
            Utf8Str efiRomFile;
            vrc = findEfiRom(virtualBox, eFwType, &efiRomFile);
            AssertRCReturn(vrc, vrc);
            const char *pszEfiRomFile = efiRomFile.c_str();
#endif

            /* Get boot args */
            Utf8Str bootArgs;
            GetExtraDataBoth(virtualBox, pMachine, "VBoxInternal2/EfiBootArgs", &bootArgs);

            /* Get device props */
            Utf8Str deviceProps;
            GetExtraDataBoth(virtualBox, pMachine, "VBoxInternal2/EfiDeviceProps", &deviceProps);

            /* Get NVRAM file name */
            Utf8Str strNvram = mptrNvramStore->i_getNonVolatileStorageFile();

            BOOL fUuidLe;
            hrc = biosSettings->COMGETTER(SMBIOSUuidLittleEndian)(&fUuidLe);                H();

            /* Get graphics mode settings */
            uint32_t u32GraphicsMode = UINT32_MAX;
            GetExtraDataBoth(virtualBox, pMachine, "VBoxInternal2/EfiGraphicsMode", &strTmp);
            if (strTmp.isEmpty())
                GetExtraDataBoth(virtualBox, pMachine, "VBoxInternal2/EfiGopMode", &strTmp);
            if (!strTmp.isEmpty())
                u32GraphicsMode = strTmp.toUInt32();

            /* Get graphics resolution settings, with some sanity checking */
            Utf8Str strResolution;
            GetExtraDataBoth(virtualBox, pMachine, "VBoxInternal2/EfiGraphicsResolution", &strResolution);
            if (!strResolution.isEmpty())
            {
                size_t pos = strResolution.find("x");
                if (pos != strResolution.npos)
                {
                    Utf8Str strH, strV;
                    strH.assignEx(strResolution, 0, pos);
                    strV.assignEx(strResolution, pos+1, strResolution.length()-pos-1);
                    uint32_t u32H = strH.toUInt32();
                    uint32_t u32V = strV.toUInt32();
                    if (u32H == 0 || u32V == 0)
                        strResolution.setNull();
                }
                else
                    strResolution.setNull();
            }
            else
            {
                uint32_t u32H = 0;
                uint32_t u32V = 0;
                GetExtraDataBoth(virtualBox, pMachine, "VBoxInternal2/EfiHorizontalResolution", &strTmp);
                if (strTmp.isEmpty())
                    GetExtraDataBoth(virtualBox, pMachine, "VBoxInternal2/EfiUgaHorizontalResolution", &strTmp);
                if (!strTmp.isEmpty())
                    u32H = strTmp.toUInt32();

                GetExtraDataBoth(virtualBox, pMachine, "VBoxInternal2/EfiVerticalResolution", &strTmp);
                if (strTmp.isEmpty())
                    GetExtraDataBoth(virtualBox, pMachine, "VBoxInternal2/EfiUgaVerticalResolution", &strTmp);
                if (!strTmp.isEmpty())
                    u32V = strTmp.toUInt32();
                if (u32H != 0 && u32V != 0)
                    strResolution = Utf8StrFmt("%ux%u", u32H, u32V);
            }

            /*
             * EFI subtree.
             */
            InsertConfigNode(pDevices, "efi", &pDev);
            InsertConfigNode(pDev,     "0", &pInst);
            InsertConfigInteger(pInst, "Trusted", 1); /* boolean */
            InsertConfigNode(pInst,    "Config", &pCfg);
            InsertConfigInteger(pCfg,  "NumCPUs",     cCpus);
            InsertConfigInteger(pCfg,  "McfgBase",    uMcfgBase);
            InsertConfigInteger(pCfg,  "McfgLength",  cbMcfgLength);
            InsertConfigString(pCfg,   "EfiRom",      pszEfiRomFile);
            InsertConfigString(pCfg,   "BootArgs",    bootArgs);
            InsertConfigString(pCfg,   "DeviceProps", deviceProps);
            InsertConfigInteger(pCfg,  "IOAPIC",      fIOAPIC);
            InsertConfigInteger(pCfg,  "APIC",        uFwAPIC);
            InsertConfigBytes(pCfg,    "UUID", &HardwareUuid,sizeof(HardwareUuid));
            InsertConfigInteger(pCfg,  "UuidLe",      fUuidLe);
            InsertConfigInteger(pCfg,  "64BitEntry",  f64BitEntry); /* boolean */
            InsertConfigString(pCfg,   "NvramFile",   strNvram);
            if (u32GraphicsMode != UINT32_MAX)
                InsertConfigInteger(pCfg,  "GraphicsMode",  u32GraphicsMode);
            if (!strResolution.isEmpty())
                InsertConfigString(pCfg,   "GraphicsResolution", strResolution);

            /* For OS X guests we'll force passing host's DMI info to the guest */
            if (fOsXGuest)
            {
                InsertConfigInteger(pCfg, "DmiUseHostInfo", 1);
                InsertConfigInteger(pCfg, "DmiExposeMemoryTable", 1);
            }

            /* Attach the NVRAM storage driver. */
            InsertConfigNode(pInst,    "LUN#0",     &pLunL0);
            InsertConfigString(pLunL0, "Driver",    "NvramStore");
        }

        /*
         * The USB Controllers.
         */
        com::SafeIfaceArray<IUSBController> usbCtrls;
        hrc = pMachine->COMGETTER(USBControllers)(ComSafeArrayAsOutParam(usbCtrls));
        bool fOhciPresent = false; /**< Flag whether at least one OHCI controller is present. */
        bool fXhciPresent = false; /**< Flag whether at least one XHCI controller is present. */

        if (SUCCEEDED(hrc))
        {
            for (size_t i = 0; i < usbCtrls.size(); ++i)
            {
                USBControllerType_T enmCtrlType;
                vrc = usbCtrls[i]->COMGETTER(Type)(&enmCtrlType);                                  H();
                if (enmCtrlType == USBControllerType_OHCI)
                {
                    fOhciPresent = true;
                    break;
                }
                else if (enmCtrlType == USBControllerType_XHCI)
                {
                    fXhciPresent = true;
                    break;
                }
            }
        }
        else if (hrc != E_NOTIMPL)
        {
            H();
        }

        /*
         * Currently EHCI is only enabled when an OHCI or XHCI controller is present as well.
         */
        if (fOhciPresent || fXhciPresent)
            mfVMHasUsbController = true;

        PCFGMNODE pUsbDevices = NULL; /**< Required for USB storage controller later. */
        if (mfVMHasUsbController)
        {
            for (size_t i = 0; i < usbCtrls.size(); ++i)
            {
                USBControllerType_T enmCtrlType;
                vrc = usbCtrls[i]->COMGETTER(Type)(&enmCtrlType);                                  H();

                if (enmCtrlType == USBControllerType_OHCI)
                {
                    InsertConfigNode(pDevices, "usb-ohci", &pDev);
                    InsertConfigNode(pDev,     "0", &pInst);
                    InsertConfigNode(pInst,    "Config", &pCfg);
                    InsertConfigInteger(pInst, "Trusted",              1); /* boolean */
                    hrc = pBusMgr->assignPCIDevice("usb-ohci", pInst);                          H();
                    InsertConfigNode(pInst,    "LUN#0", &pLunL0);
                    InsertConfigString(pLunL0, "Driver",               "VUSBRootHub");
                    InsertConfigNode(pLunL0,   "Config", &pCfg);

                    /*
                     * Attach the status driver.
                     */
                    i_attachStatusDriver(pInst, DeviceType_USB);
                }
#ifdef VBOX_WITH_EHCI
                else if (enmCtrlType == USBControllerType_EHCI)
                {
                    InsertConfigNode(pDevices, "usb-ehci", &pDev);
                    InsertConfigNode(pDev,     "0", &pInst);
                    InsertConfigNode(pInst,    "Config", &pCfg);
                    InsertConfigInteger(pInst, "Trusted", 1); /* boolean */
                    hrc = pBusMgr->assignPCIDevice("usb-ehci", pInst);                  H();

                    InsertConfigNode(pInst,    "LUN#0", &pLunL0);
                    InsertConfigString(pLunL0, "Driver",               "VUSBRootHub");
                    InsertConfigNode(pLunL0,   "Config", &pCfg);

                    /*
                     * Attach the status driver.
                     */
                    i_attachStatusDriver(pInst, DeviceType_USB);
                }
#endif
                else if (enmCtrlType == USBControllerType_XHCI)
                {
                    InsertConfigNode(pDevices, "usb-xhci", &pDev);
                    InsertConfigNode(pDev,     "0", &pInst);
                    InsertConfigNode(pInst,    "Config", &pCfg);
                    InsertConfigInteger(pInst, "Trusted", 1); /* boolean */
                    hrc = pBusMgr->assignPCIDevice("usb-xhci", pInst);                  H();

                    InsertConfigNode(pInst,    "LUN#0", &pLunL0);
                    InsertConfigString(pLunL0, "Driver",               "VUSBRootHub");
                    InsertConfigNode(pLunL0,   "Config", &pCfg);

                    InsertConfigNode(pInst,    "LUN#1", &pLunL1);
                    InsertConfigString(pLunL1, "Driver",               "VUSBRootHub");
                    InsertConfigNode(pLunL1,   "Config", &pCfg);

                    /*
                     * Attach the status driver.
                     */
                    i_attachStatusDriver(pInst, DeviceType_USB, 2);
                }
            } /* for every USB controller. */


            /*
             * Virtual USB Devices.
             */
            InsertConfigNode(pRoot, "USB", &pUsbDevices);

#ifdef VBOX_WITH_USB
            {
                /*
                 * Global USB options, currently unused as we'll apply the 2.0 -> 1.1 morphing
                 * on a per device level now.
                 */
                InsertConfigNode(pUsbDevices, "USBProxy", &pCfg);
                InsertConfigNode(pCfg, "GlobalConfig", &pCfg);
                // This globally enables the 2.0 -> 1.1 device morphing of proxied devices to keep windows quiet.
                //InsertConfigInteger(pCfg, "Force11Device", true);
                // The following breaks stuff, but it makes MSDs work in vista. (I include it here so
                // that it's documented somewhere.) Users needing it can use:
                //      VBoxManage setextradata "myvm" "VBoxInternal/USB/USBProxy/GlobalConfig/Force11PacketSize" 1
                //InsertConfigInteger(pCfg, "Force11PacketSize", true);
            }
#endif

#ifdef VBOX_WITH_USB_CARDREADER
            BOOL aEmulatedUSBCardReaderEnabled = FALSE;
            hrc = pMachine->COMGETTER(EmulatedUSBCardReaderEnabled)(&aEmulatedUSBCardReaderEnabled);    H();
            if (aEmulatedUSBCardReaderEnabled)
            {
                InsertConfigNode(pUsbDevices, "CardReader", &pDev);
                InsertConfigNode(pDev,     "0", &pInst);
                InsertConfigNode(pInst,    "Config", &pCfg);

                InsertConfigNode(pInst,    "LUN#0", &pLunL0);
# ifdef VBOX_WITH_USB_CARDREADER_TEST
                InsertConfigString(pLunL0, "Driver", "DrvDirectCardReader");
                InsertConfigNode(pLunL0,   "Config", &pCfg);
# else
                InsertConfigString(pLunL0, "Driver", "UsbCardReader");
                InsertConfigNode(pLunL0,   "Config", &pCfg);
# endif
             }
#endif

            /* Virtual USB Mouse/Tablet */
            if (   aPointingHID == PointingHIDType_USBMouse
                || aPointingHID == PointingHIDType_USBTablet
                || aPointingHID == PointingHIDType_USBMultiTouch
                || aPointingHID == PointingHIDType_USBMultiTouchScreenPlusPad)
            {
                InsertConfigNode(pUsbDevices, "HidMouse", &pDev);
                InsertConfigNode(pDev,     "0", &pInst);
                InsertConfigNode(pInst,    "Config", &pCfg);

                if (aPointingHID == PointingHIDType_USBMouse)
                    InsertConfigString(pCfg,   "Mode", "relative");
                else
                    InsertConfigString(pCfg,   "Mode", "absolute");
                InsertConfigNode(pInst,    "LUN#0", &pLunL0);
                InsertConfigString(pLunL0, "Driver",        "MouseQueue");
                InsertConfigNode(pLunL0,   "Config", &pCfg);
                InsertConfigInteger(pCfg,  "QueueSize",            128);

                InsertConfigNode(pLunL0,   "AttachedDriver", &pLunL1);
                InsertConfigString(pLunL1, "Driver",        "MainMouse");
            }
            if (   aPointingHID == PointingHIDType_USBMultiTouch
                || aPointingHID == PointingHIDType_USBMultiTouchScreenPlusPad)
            {
                InsertConfigNode(pDev,     "1", &pInst);
                InsertConfigNode(pInst,    "Config", &pCfg);

                InsertConfigString(pCfg,   "Mode", "multitouch");
                InsertConfigNode(pInst,    "LUN#0", &pLunL0);
                InsertConfigString(pLunL0, "Driver",        "MouseQueue");
                InsertConfigNode(pLunL0,   "Config", &pCfg);
                InsertConfigInteger(pCfg,  "QueueSize",            128);

                InsertConfigNode(pLunL0,   "AttachedDriver", &pLunL1);
                InsertConfigString(pLunL1, "Driver",        "MainMouse");
            }
            if (aPointingHID == PointingHIDType_USBMultiTouchScreenPlusPad)
            {
                InsertConfigNode(pDev,     "2", &pInst);
                InsertConfigNode(pInst,    "Config", &pCfg);

                InsertConfigString(pCfg,   "Mode", "touchpad");
                InsertConfigNode(pInst,    "LUN#0", &pLunL0);
                InsertConfigString(pLunL0, "Driver",        "MouseQueue");
                InsertConfigNode(pLunL0,   "Config", &pCfg);
                InsertConfigInteger(pCfg,  "QueueSize",            128);

                InsertConfigNode(pLunL0,   "AttachedDriver", &pLunL1);
                InsertConfigString(pLunL1, "Driver",        "MainMouse");
            }

            /* Virtual USB Keyboard */
            if (aKbdHID == KeyboardHIDType_USBKeyboard)
            {
                InsertConfigNode(pUsbDevices, "HidKeyboard", &pDev);
                InsertConfigNode(pDev,     "0", &pInst);
                InsertConfigNode(pInst,    "Config", &pCfg);

                InsertConfigNode(pInst,    "LUN#0", &pLunL0);
                InsertConfigString(pLunL0, "Driver",               "KeyboardQueue");
                InsertConfigNode(pLunL0,   "Config", &pCfg);
                InsertConfigInteger(pCfg,  "QueueSize",            64);

                InsertConfigNode(pLunL0,   "AttachedDriver", &pLunL1);
                InsertConfigString(pLunL1, "Driver",               "MainKeyboard");
            }
        }

        /*
         * Storage controllers.
         */
        com::SafeIfaceArray<IStorageController> ctrls;
        PCFGMNODE aCtrlNodes[StorageControllerType_VirtioSCSI + 1] = {};
        hrc = pMachine->COMGETTER(StorageControllers)(ComSafeArrayAsOutParam(ctrls));       H();

        bool fFdcEnabled = false;
        for (size_t i = 0; i < ctrls.size(); ++i)
        {
            DeviceType_T *paLedDevType = NULL;

            StorageControllerType_T enmCtrlType;
            hrc = ctrls[i]->COMGETTER(ControllerType)(&enmCtrlType);                        H();
            AssertRelease((unsigned)enmCtrlType < RT_ELEMENTS(aCtrlNodes)
                          || enmCtrlType == StorageControllerType_USB);

            StorageBus_T enmBus;
            hrc = ctrls[i]->COMGETTER(Bus)(&enmBus);                                        H();

            Bstr controllerName;
            hrc = ctrls[i]->COMGETTER(Name)(controllerName.asOutParam());                   H();

            ULONG ulInstance = 999;
            hrc = ctrls[i]->COMGETTER(Instance)(&ulInstance);                               H();

            BOOL fUseHostIOCache;
            hrc = ctrls[i]->COMGETTER(UseHostIOCache)(&fUseHostIOCache);                    H();

            BOOL fBootable;
            hrc = ctrls[i]->COMGETTER(Bootable)(&fBootable);                                H();

            PCFGMNODE pCtlInst = NULL;
            const char *pszCtrlDev = i_storageControllerTypeToStr(enmCtrlType);
            if (enmCtrlType != StorageControllerType_USB)
            {
                /* /Devices/<ctrldev>/ */
                pDev = aCtrlNodes[enmCtrlType];
                if (!pDev)
                {
                    InsertConfigNode(pDevices, pszCtrlDev, &pDev);
                    aCtrlNodes[enmCtrlType] = pDev; /* IDE variants are handled in the switch */
                }

                /* /Devices/<ctrldev>/<instance>/ */
                InsertConfigNode(pDev, Utf8StrFmt("%u", ulInstance).c_str(), &pCtlInst);

                /* Device config: /Devices/<ctrldev>/<instance>/<values> & /ditto/Config/<values> */
                InsertConfigInteger(pCtlInst, "Trusted",   1);
                InsertConfigNode(pCtlInst,    "Config",    &pCfg);
            }

            static const char * const apszBiosConfigScsi[MAX_BIOS_LUN_COUNT] =
            { "ScsiLUN1", "ScsiLUN2", "ScsiLUN3", "ScsiLUN4" };

            static const char * const apszBiosConfigSata[MAX_BIOS_LUN_COUNT] =
            { "SataLUN1", "SataLUN2", "SataLUN3", "SataLUN4" };

            switch (enmCtrlType)
            {
                case StorageControllerType_LsiLogic:
                {
                    hrc = pBusMgr->assignPCIDevice("lsilogic", pCtlInst);                   H();

                    InsertConfigInteger(pCfg, "Bootable",  fBootable);

                    /* BIOS configuration values, first SCSI controller only. */
                    if (   !pBusMgr->hasPCIDevice("lsilogic", 1)
                        && !pBusMgr->hasPCIDevice("buslogic", 0)
                        && !pBusMgr->hasPCIDevice("lsilogicsas", 0)
                        && pBiosCfg)
                    {
                        InsertConfigString(pBiosCfg, "ScsiHardDiskDevice", "lsilogicscsi");
                        hrc = SetBiosDiskInfo(pMachine, pCfg, pBiosCfg, controllerName, apszBiosConfigScsi);    H();
                    }

                    /* Attach the status driver */
                    i_attachStatusDriver(pCtlInst, RT_BIT_32(DeviceType_HardDisk) | RT_BIT_32(DeviceType_DVD),
                                         16, &paLedDevType, &mapMediumAttachments, pszCtrlDev, ulInstance);
                    break;
                }

                case StorageControllerType_BusLogic:
                {
                    hrc = pBusMgr->assignPCIDevice("buslogic", pCtlInst);                   H();

                    InsertConfigInteger(pCfg, "Bootable",  fBootable);

                    /* BIOS configuration values, first SCSI controller only. */
                    if (   !pBusMgr->hasPCIDevice("lsilogic", 0)
                        && !pBusMgr->hasPCIDevice("buslogic", 1)
                        && !pBusMgr->hasPCIDevice("lsilogicsas", 0)
                        && pBiosCfg)
                    {
                        InsertConfigString(pBiosCfg, "ScsiHardDiskDevice", "buslogic");
                        hrc = SetBiosDiskInfo(pMachine, pCfg, pBiosCfg, controllerName, apszBiosConfigScsi);    H();
                    }

                    /* Attach the status driver */
                    i_attachStatusDriver(pCtlInst, RT_BIT_32(DeviceType_HardDisk) | RT_BIT_32(DeviceType_DVD),
                                         16, &paLedDevType, &mapMediumAttachments, pszCtrlDev, ulInstance);
                    break;
                }

                case StorageControllerType_IntelAhci:
                {
                    hrc = pBusMgr->assignPCIDevice("ahci", pCtlInst);                       H();

                    ULONG cPorts = 0;
                    hrc = ctrls[i]->COMGETTER(PortCount)(&cPorts);                          H();
                    InsertConfigInteger(pCfg, "PortCount", cPorts);
                    InsertConfigInteger(pCfg, "Bootable",  fBootable);

                    com::SafeIfaceArray<IMediumAttachment> atts;
                    hrc = pMachine->GetMediumAttachmentsOfController(controllerName.raw(),
                                                                     ComSafeArrayAsOutParam(atts));  H();

                    /* Configure the hotpluggable flag for the port. */
                    for (unsigned idxAtt = 0; idxAtt < atts.size(); ++idxAtt)
                    {
                        IMediumAttachment *pMediumAtt = atts[idxAtt];

                        LONG lPortNum = 0;
                        hrc = pMediumAtt->COMGETTER(Port)(&lPortNum);                       H();

                        BOOL fHotPluggable = FALSE;
                        hrc = pMediumAtt->COMGETTER(HotPluggable)(&fHotPluggable);          H();
                        if (SUCCEEDED(hrc))
                        {
                            PCFGMNODE pPortCfg;
                            char szName[24];
                            RTStrPrintf(szName, sizeof(szName), "Port%d", lPortNum);

                            InsertConfigNode(pCfg, szName, &pPortCfg);
                            InsertConfigInteger(pPortCfg, "Hotpluggable", fHotPluggable ? 1 : 0);
                        }
                    }

                    /* BIOS configuration values, first AHCI controller only. */
                    if (   !pBusMgr->hasPCIDevice("ahci", 1)
                        && pBiosCfg)
                    {
                        InsertConfigString(pBiosCfg, "SataHardDiskDevice", "ahci");
                        hrc = SetBiosDiskInfo(pMachine, pCfg, pBiosCfg, controllerName, apszBiosConfigSata);    H();
                    }

                    /* Attach the status driver */
                    i_attachStatusDriver(pCtlInst, RT_BIT_32(DeviceType_HardDisk) | RT_BIT_32(DeviceType_DVD),
                                         cPorts, &paLedDevType, &mapMediumAttachments, pszCtrlDev, ulInstance);
                    break;
                }

                case StorageControllerType_PIIX3:
                case StorageControllerType_PIIX4:
                case StorageControllerType_ICH6:
                {
                    /*
                     * IDE (update this when the main interface changes)
                     */
                    hrc = pBusMgr->assignPCIDevice("piix3ide", pCtlInst);                   H();
                    InsertConfigString(pCfg,   "Type", controllerString(enmCtrlType));

                    /* Attach the status driver */
                    i_attachStatusDriver(pCtlInst, RT_BIT_32(DeviceType_HardDisk) | RT_BIT_32(DeviceType_DVD),
                                         4, &paLedDevType, &mapMediumAttachments, pszCtrlDev, ulInstance);

                    /* IDE flavors */
                    aCtrlNodes[StorageControllerType_PIIX3] = pDev;
                    aCtrlNodes[StorageControllerType_PIIX4] = pDev;
                    aCtrlNodes[StorageControllerType_ICH6]  = pDev;
                    break;
                }

                case StorageControllerType_I82078:
                {
                    /*
                     * i82078 Floppy drive controller
                     */
                    fFdcEnabled = true;
                    InsertConfigInteger(pCfg, "IRQ",       6);
                    InsertConfigInteger(pCfg, "DMA",       2);
                    InsertConfigInteger(pCfg, "MemMapped", 0 );
                    InsertConfigInteger(pCfg, "IOBase",    0x3f0);

                    /* Attach the status driver */
                    i_attachStatusDriver(pCtlInst, RT_BIT_32(DeviceType_Floppy),
                                         2, NULL, &mapMediumAttachments, pszCtrlDev, ulInstance);
                    break;
                }

                case StorageControllerType_LsiLogicSas:
                {
                    hrc = pBusMgr->assignPCIDevice("lsilogicsas", pCtlInst);                H();

                    InsertConfigString(pCfg,  "ControllerType", "SAS1068");
                    InsertConfigInteger(pCfg, "Bootable",  fBootable);

                    /* BIOS configuration values, first SCSI controller only. */
                    if (   !pBusMgr->hasPCIDevice("lsilogic", 0)
                        && !pBusMgr->hasPCIDevice("buslogic", 0)
                        && !pBusMgr->hasPCIDevice("lsilogicsas", 1)
                        && pBiosCfg)
                    {
                        InsertConfigString(pBiosCfg, "ScsiHardDiskDevice", "lsilogicsas");
                        hrc = SetBiosDiskInfo(pMachine, pCfg, pBiosCfg, controllerName, apszBiosConfigScsi);    H();
                    }

                    ULONG cPorts = 0;
                    hrc = ctrls[i]->COMGETTER(PortCount)(&cPorts);                          H();
                    InsertConfigInteger(pCfg, "NumPorts", cPorts);

                    /* Attach the status driver */
                    i_attachStatusDriver(pCtlInst, RT_BIT_32(DeviceType_HardDisk) | RT_BIT_32(DeviceType_DVD) /*?*/,
                                         8, &paLedDevType, &mapMediumAttachments, pszCtrlDev, ulInstance);
                    break;
                }

                case StorageControllerType_USB:
                {
                    if (pUsbDevices)
                    {
                        /*
                         * USB MSDs are handled a bit different as the device instance
                         * doesn't match the storage controller instance but the port.
                         */
                        InsertConfigNode(pUsbDevices, "Msd", &pDev);
                        pCtlInst = pDev;
                    }
                    else
                        return pVMM->pfnVMR3SetError(pUVM, VERR_NOT_FOUND, RT_SRC_POS,
                                N_("There is no USB controller enabled but there\n"
                                   "is at least one USB storage device configured for this VM.\n"
                                   "To fix this problem either enable the USB controller or remove\n"
                                   "the storage device from the VM"));
                    break;
                }

                case StorageControllerType_NVMe:
                {
                    hrc = pBusMgr->assignPCIDevice("nvme", pCtlInst);                       H();

                    ULONG cPorts = 0;
                    hrc = ctrls[i]->COMGETTER(PortCount)(&cPorts);                          H();
                    InsertConfigInteger(pCfg, "NamespacesMax", cPorts);

                    /* Attach the status driver */
                    i_attachStatusDriver(pCtlInst, RT_BIT_32(DeviceType_HardDisk),
                                         cPorts, NULL, &mapMediumAttachments, pszCtrlDev, ulInstance);
                    break;
                }

                case StorageControllerType_VirtioSCSI:
                {
                    hrc = pBusMgr->assignPCIDevice("virtio-scsi", pCtlInst);                H();

                    ULONG cPorts = 0;
                    hrc = ctrls[i]->COMGETTER(PortCount)(&cPorts);                          H();
                    InsertConfigInteger(pCfg, "NumTargets", cPorts);
                    InsertConfigInteger(pCfg, "Bootable",   fBootable);

                    /* Attach the status driver */
                    i_attachStatusDriver(pCtlInst, RT_BIT_32(DeviceType_HardDisk) | RT_BIT_32(DeviceType_DVD) /*?*/,
                                         cPorts, &paLedDevType, &mapMediumAttachments, pszCtrlDev, ulInstance);
                    break;
                }

                default:
                    AssertLogRelMsgFailedReturn(("invalid storage controller type: %d\n", enmCtrlType), VERR_MAIN_CONFIG_CONSTRUCTOR_IPE);
            }

            /* Attach the media to the storage controllers. */
            com::SafeIfaceArray<IMediumAttachment> atts;
            hrc = pMachine->GetMediumAttachmentsOfController(controllerName.raw(),
                                                            ComSafeArrayAsOutParam(atts));  H();

            /* Builtin I/O cache - per device setting. */
            BOOL fBuiltinIOCache = true;
            hrc = pMachine->COMGETTER(IOCacheEnabled)(&fBuiltinIOCache);                    H();

            bool fInsertDiskIntegrityDrv = false;
            Bstr strDiskIntegrityFlag;
            hrc = pMachine->GetExtraData(Bstr("VBoxInternal2/EnableDiskIntegrityDriver").raw(),
                                         strDiskIntegrityFlag.asOutParam());
            if (   hrc   == S_OK
                && strDiskIntegrityFlag == "1")
                fInsertDiskIntegrityDrv = true;

            for (size_t j = 0; j < atts.size(); ++j)
            {
                IMediumAttachment *pMediumAtt = atts[j];
                vrc = i_configMediumAttachment(pszCtrlDev,
                                               ulInstance,
                                               enmBus,
                                               !!fUseHostIOCache,
                                               enmCtrlType == StorageControllerType_NVMe ? false : !!fBuiltinIOCache,
                                               fInsertDiskIntegrityDrv,
                                               false /* fSetupMerge */,
                                               0 /* uMergeSource */,
                                               0 /* uMergeTarget */,
                                               pMediumAtt,
                                               mMachineState,
                                               NULL /* phrc */,
                                               false /* fAttachDetach */,
                                               false /* fForceUnmount */,
                                               false /* fHotplug */,
                                               pUVM,
                                               pVMM,
                                               paLedDevType,
                                               NULL /* ppLunL0 */);
                if (RT_FAILURE(vrc))
                    return vrc;
            }
            H();
        }
        H();

        /*
         * Network adapters
         */
#ifdef VMWARE_NET_IN_SLOT_11
        bool fSwapSlots3and11 = false;
#endif
        PCFGMNODE pDevPCNet = NULL;          /* PCNet-type devices */
        InsertConfigNode(pDevices, "pcnet", &pDevPCNet);
#ifdef VBOX_WITH_E1000
        PCFGMNODE pDevE1000 = NULL;          /* E1000-type devices */
        InsertConfigNode(pDevices, "e1000", &pDevE1000);
#endif
#ifdef VBOX_WITH_VIRTIO
        PCFGMNODE pDevVirtioNet = NULL;          /* Virtio network devices */
        InsertConfigNode(pDevices, "virtio-net", &pDevVirtioNet);
#endif /* VBOX_WITH_VIRTIO */
        PCFGMNODE pDevDP8390 = NULL;         /* DP8390-type devices */
        InsertConfigNode(pDevices, "dp8390", &pDevDP8390);
        PCFGMNODE pDev3C501 = NULL;          /* EtherLink-type devices */
        InsertConfigNode(pDevices, "3c501",  &pDev3C501);

        std::list<BootNic> llBootNics;
        for (ULONG uInstance = 0; uInstance < maxNetworkAdapters; ++uInstance)
        {
            ComPtr<INetworkAdapter> networkAdapter;
            hrc = pMachine->GetNetworkAdapter(uInstance, networkAdapter.asOutParam());      H();
            BOOL fEnabledNetAdapter = FALSE;
            hrc = networkAdapter->COMGETTER(Enabled)(&fEnabledNetAdapter);                  H();
            if (!fEnabledNetAdapter)
                continue;

            /*
             * The virtual hardware type. Create appropriate device first.
             */
            const char *pszAdapterName = "pcnet";
            NetworkAdapterType_T adapterType;
            hrc = networkAdapter->COMGETTER(AdapterType)(&adapterType);                     H();
            switch (adapterType)
            {
                case NetworkAdapterType_Am79C970A:
                case NetworkAdapterType_Am79C973:
                case NetworkAdapterType_Am79C960:
                    pDev = pDevPCNet;
                    break;
#ifdef VBOX_WITH_E1000
                case NetworkAdapterType_I82540EM:
                case NetworkAdapterType_I82543GC:
                case NetworkAdapterType_I82545EM:
                    pDev = pDevE1000;
                    pszAdapterName = "e1000";
                    break;
#endif
#ifdef VBOX_WITH_VIRTIO
                case NetworkAdapterType_Virtio:
                    pDev = pDevVirtioNet;
                    pszAdapterName = "virtio-net";
                    break;
#endif /* VBOX_WITH_VIRTIO */
                case NetworkAdapterType_NE1000:
                case NetworkAdapterType_NE2000:
                case NetworkAdapterType_WD8003:
                case NetworkAdapterType_WD8013:
                case NetworkAdapterType_ELNK2:
                    pDev = pDevDP8390;
                    break;
                case NetworkAdapterType_ELNK1:
                    pDev = pDev3C501;
                    break;
                default:
                    AssertMsgFailed(("Invalid network adapter type '%d' for slot '%d'", adapterType, uInstance));
                    return pVMM->pfnVMR3SetError(pUVM, VERR_INVALID_PARAMETER, RT_SRC_POS,
                                                 N_("Invalid network adapter type '%d' for slot '%d'"), adapterType, uInstance);
            }

            InsertConfigNode(pDev, Utf8StrFmt("%u", uInstance).c_str(), &pInst);
            InsertConfigInteger(pInst, "Trusted",              1); /* boolean */
            /* the first network card gets the PCI ID 3, the next 3 gets 8..10,
             * next 4 get 16..19. */
            int iPCIDeviceNo;
            switch (uInstance)
            {
                case 0:
                    iPCIDeviceNo = 3;
                    break;
                case 1: case 2: case 3:
                    iPCIDeviceNo = uInstance - 1 + 8;
                    break;
                case 4: case 5: case 6: case 7:
                    iPCIDeviceNo = uInstance - 4 + 16;
                    break;
                default:
                    /* auto assignment */
                    iPCIDeviceNo = -1;
                    break;
            }
#ifdef VMWARE_NET_IN_SLOT_11
            /*
             * Dirty hack for PCI slot compatibility with VMWare,
             * it assigns slot 0x11 to the first network controller.
             */
            if (iPCIDeviceNo == 3 && adapterType == NetworkAdapterType_I82545EM)
            {
                iPCIDeviceNo = 0x11;
                fSwapSlots3and11 = true;
            }
            else if (iPCIDeviceNo == 0x11 && fSwapSlots3and11)
                iPCIDeviceNo = 3;
#endif
            PCIBusAddress PCIAddr = PCIBusAddress(0, iPCIDeviceNo, 0);
            hrc = pBusMgr->assignPCIDevice(pszAdapterName, pInst, PCIAddr);                 H();

            InsertConfigNode(pInst, "Config", &pCfg);
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE   /* not safe here yet. */ /** @todo Make PCNet ring-0 safe on 32-bit mac kernels! */
            if (pDev == pDevPCNet)
                InsertConfigInteger(pCfg, "R0Enabled",    false);
#endif
            /*
             * Collect information needed for network booting and add it to the list.
             */
            BootNic     nic;

            nic.mInstance    = uInstance;
            /* Could be updated by reference, if auto assigned */
            nic.mPCIAddress  = PCIAddr;

            hrc = networkAdapter->COMGETTER(BootPriority)(&nic.mBootPrio);                  H();

            llBootNics.push_back(nic);

            /*
             * The virtual hardware type. PCNet supports three types, E1000 three,
             * but VirtIO only one.
             */
            switch (adapterType)
            {
                case NetworkAdapterType_Am79C970A:
                    InsertConfigString(pCfg, "ChipType", "Am79C970A");
                    break;
                case NetworkAdapterType_Am79C973:
                    InsertConfigString(pCfg, "ChipType", "Am79C973");
                    break;
                case NetworkAdapterType_Am79C960:
                    InsertConfigString(pCfg, "ChipType", "Am79C960");
                    break;
                case NetworkAdapterType_I82540EM:
                    InsertConfigInteger(pCfg, "AdapterType", 0);
                    break;
                case NetworkAdapterType_I82543GC:
                    InsertConfigInteger(pCfg, "AdapterType", 1);
                    break;
                case NetworkAdapterType_I82545EM:
                    InsertConfigInteger(pCfg, "AdapterType", 2);
                    break;
                case NetworkAdapterType_Virtio:
                    break;
                case NetworkAdapterType_NE1000:
                    InsertConfigString(pCfg, "DeviceType", "NE1000");
                    break;
                case NetworkAdapterType_NE2000:
                    InsertConfigString(pCfg, "DeviceType", "NE2000");
                    break;
                case NetworkAdapterType_WD8003:
                    InsertConfigString(pCfg, "DeviceType", "WD8003");
                    break;
                case NetworkAdapterType_WD8013:
                    InsertConfigString(pCfg, "DeviceType", "WD8013");
                    break;
                case NetworkAdapterType_ELNK2:
                    InsertConfigString(pCfg, "DeviceType", "3C503");
                    break;
                case NetworkAdapterType_ELNK1:
                    break;
                case NetworkAdapterType_Null:      AssertFailedBreak(); /* (compiler warnings) */
#ifdef VBOX_WITH_XPCOM_CPP_ENUM_HACK
                case NetworkAdapterType_32BitHack: AssertFailedBreak(); /* (compiler warnings) */
#endif
            }

            /*
             * Get the MAC address and convert it to binary representation
             */
            Bstr macAddr;
            hrc = networkAdapter->COMGETTER(MACAddress)(macAddr.asOutParam());              H();
            Assert(!macAddr.isEmpty());
            Utf8Str macAddrUtf8 = macAddr;
#ifdef VBOX_WITH_CLOUD_NET
            NetworkAttachmentType_T eAttachmentType;
            hrc = networkAdapter->COMGETTER(AttachmentType)(&eAttachmentType);                 H();
            if (eAttachmentType == NetworkAttachmentType_Cloud)
            {
                mGateway.setLocalMacAddress(macAddrUtf8);
                /* We'll insert cloud MAC later, when it becomes known. */
            }
            else
            {
#endif
            char *macStr = (char*)macAddrUtf8.c_str();
            Assert(strlen(macStr) == 12);
            RTMAC Mac;
            RT_ZERO(Mac);
            char *pMac = (char*)&Mac;
            for (uint32_t i = 0; i < 6; ++i)
            {
                int c1 = *macStr++ - '0';
                if (c1 > 9)
                    c1 -= 7;
                int c2 = *macStr++ - '0';
                if (c2 > 9)
                    c2 -= 7;
                *pMac++ = (char)(((c1 & 0x0f) << 4) | (c2 & 0x0f));
            }
            InsertConfigBytes(pCfg, "MAC", &Mac, sizeof(Mac));
#ifdef VBOX_WITH_CLOUD_NET
            }
#endif
            /*
             * Check if the cable is supposed to be unplugged
             */
            BOOL fCableConnected;
            hrc = networkAdapter->COMGETTER(CableConnected)(&fCableConnected);              H();
            InsertConfigInteger(pCfg, "CableConnected", fCableConnected ? 1 : 0);

            /*
             * Line speed to report from custom drivers
             */
            ULONG ulLineSpeed;
            hrc = networkAdapter->COMGETTER(LineSpeed)(&ulLineSpeed);                       H();
            InsertConfigInteger(pCfg, "LineSpeed", ulLineSpeed);

            /*
             * Attach the status driver.
             */
            i_attachStatusDriver(pInst, DeviceType_Network);

            /*
             * Configure the network card now
             */
            bool fIgnoreConnectFailure = mMachineState == MachineState_Restoring;
            vrc = i_configNetwork(pszAdapterName,
                                  uInstance,
                                  0,
                                  networkAdapter,
                                  pCfg,
                                  pLunL0,
                                  pInst,
                                  false /*fAttachDetach*/,
                                  fIgnoreConnectFailure,
                                  pUVM,
                                  pVMM);
            if (RT_FAILURE(vrc))
                return vrc;
        }

        /*
         * Build network boot information and transfer it to the BIOS.
         */
        if (pNetBootCfg && !llBootNics.empty())  /* NetBoot node doesn't exist for EFI! */
        {
            llBootNics.sort();  /* Sort the list by boot priority. */

            char        achBootIdx[] = "0";
            unsigned    uBootIdx = 0;

            for (std::list<BootNic>::iterator it = llBootNics.begin(); it != llBootNics.end(); ++it)
            {
                /* A NIC with priority 0 is only used if it's first in the list. */
                if (it->mBootPrio == 0 && uBootIdx != 0)
                    break;

                PCFGMNODE pNetBtDevCfg;
                achBootIdx[0] = (char)('0' + uBootIdx++);   /* Boot device order. */
                InsertConfigNode(pNetBootCfg, achBootIdx, &pNetBtDevCfg);
                InsertConfigInteger(pNetBtDevCfg, "NIC", it->mInstance);
                InsertConfigInteger(pNetBtDevCfg, "PCIBusNo",      it->mPCIAddress.miBus);
                InsertConfigInteger(pNetBtDevCfg, "PCIDeviceNo",   it->mPCIAddress.miDevice);
                InsertConfigInteger(pNetBtDevCfg, "PCIFunctionNo", it->mPCIAddress.miFn);
            }
        }

        /*
         * Serial (UART) Ports
         */
        /* serial enabled mask to be passed to dev ACPI */
        uint16_t auSerialIoPortBase[SchemaDefs::SerialPortCount] = {0};
        uint8_t auSerialIrq[SchemaDefs::SerialPortCount] = {0};
        InsertConfigNode(pDevices, "serial", &pDev);
        for (ULONG ulInstance = 0; ulInstance < SchemaDefs::SerialPortCount; ++ulInstance)
        {
            ComPtr<ISerialPort> serialPort;
            hrc = pMachine->GetSerialPort(ulInstance, serialPort.asOutParam());             H();
            BOOL fEnabledSerPort = FALSE;
            if (serialPort)
            {
                hrc = serialPort->COMGETTER(Enabled)(&fEnabledSerPort);                     H();
            }
            if (!fEnabledSerPort)
            {
                m_aeSerialPortMode[ulInstance] = PortMode_Disconnected;
                continue;
            }

            InsertConfigNode(pDev, Utf8StrFmt("%u", ulInstance).c_str(), &pInst);
            InsertConfigInteger(pInst, "Trusted", 1); /* boolean */
            InsertConfigNode(pInst, "Config", &pCfg);

            ULONG ulIRQ;
            hrc = serialPort->COMGETTER(IRQ)(&ulIRQ);                                       H();
            InsertConfigInteger(pCfg, "IRQ", ulIRQ);
            auSerialIrq[ulInstance] = (uint8_t)ulIRQ;

            ULONG ulIOBase;
            hrc = serialPort->COMGETTER(IOBase)(&ulIOBase);                                 H();
            InsertConfigInteger(pCfg, "IOBase", ulIOBase);
            auSerialIoPortBase[ulInstance] = (uint16_t)ulIOBase;

            BOOL  fServer;
            hrc = serialPort->COMGETTER(Server)(&fServer);                                  H();
            hrc = serialPort->COMGETTER(Path)(bstr.asOutParam());                           H();
            UartType_T eUartType;
            const char *pszUartType;
            hrc = serialPort->COMGETTER(UartType)(&eUartType);                              H();
            switch (eUartType)
            {
                case UartType_U16450: pszUartType = "16450"; break;
                case UartType_U16750: pszUartType = "16750"; break;
                default: AssertFailed(); RT_FALL_THRU();
                case UartType_U16550A: pszUartType = "16550A"; break;
            }
            InsertConfigString(pCfg, "UartType", pszUartType);

            PortMode_T eHostMode;
            hrc = serialPort->COMGETTER(HostMode)(&eHostMode);                              H();

            m_aeSerialPortMode[ulInstance] = eHostMode;
            if (eHostMode != PortMode_Disconnected)
            {
                vrc = i_configSerialPort(pInst, eHostMode, Utf8Str(bstr).c_str(), RT_BOOL(fServer));
                if (RT_FAILURE(vrc))
                    return vrc;
            }
        }

        /*
         * Parallel (LPT) Ports
         */
        /* parallel enabled mask to be passed to dev ACPI */
        uint16_t auParallelIoPortBase[SchemaDefs::ParallelPortCount] = {0};
        uint8_t auParallelIrq[SchemaDefs::ParallelPortCount] = {0};
        InsertConfigNode(pDevices, "parallel", &pDev);
        for (ULONG ulInstance = 0; ulInstance < SchemaDefs::ParallelPortCount; ++ulInstance)
        {
            ComPtr<IParallelPort> parallelPort;
            hrc = pMachine->GetParallelPort(ulInstance, parallelPort.asOutParam());         H();
            BOOL fEnabledParPort = FALSE;
            if (parallelPort)
            {
                hrc = parallelPort->COMGETTER(Enabled)(&fEnabledParPort);                   H();
            }
            if (!fEnabledParPort)
                continue;

            InsertConfigNode(pDev, Utf8StrFmt("%u", ulInstance).c_str(), &pInst);
            InsertConfigNode(pInst, "Config", &pCfg);

            ULONG ulIRQ;
            hrc = parallelPort->COMGETTER(IRQ)(&ulIRQ);                                     H();
            InsertConfigInteger(pCfg, "IRQ", ulIRQ);
            auParallelIrq[ulInstance] = (uint8_t)ulIRQ;
            ULONG ulIOBase;
            hrc = parallelPort->COMGETTER(IOBase)(&ulIOBase);                               H();
            InsertConfigInteger(pCfg,   "IOBase", ulIOBase);
            auParallelIoPortBase[ulInstance] = (uint16_t)ulIOBase;

            hrc = parallelPort->COMGETTER(Path)(bstr.asOutParam());                         H();
            if (!bstr.isEmpty())
            {
                InsertConfigNode(pInst,     "LUN#0", &pLunL0);
                InsertConfigString(pLunL0,  "Driver", "HostParallel");
                InsertConfigNode(pLunL0,    "Config", &pLunL1);
                InsertConfigString(pLunL1,  "DevicePath", bstr);
            }
        }

        /*
         * VMM Device
         */
        InsertConfigNode(pDevices, "VMMDev", &pDev);
        InsertConfigNode(pDev,     "0", &pInst);
        InsertConfigNode(pInst,    "Config", &pCfg);
        InsertConfigInteger(pInst, "Trusted",              1); /* boolean */
        hrc = pBusMgr->assignPCIDevice("VMMDev", pInst);                                    H();

        Bstr hwVersion;
        hrc = pMachine->COMGETTER(HardwareVersion)(hwVersion.asOutParam());                 H();
        if (hwVersion.compare(Bstr("1").raw()) == 0) /* <= 2.0.x */
            InsertConfigInteger(pCfg, "HeapEnabled", 0);
        Bstr snapshotFolder;
        hrc = pMachine->COMGETTER(SnapshotFolder)(snapshotFolder.asOutParam());             H();
        InsertConfigString(pCfg, "GuestCoreDumpDir", snapshotFolder);

        /* the VMM device's Main driver */
        InsertConfigNode(pInst,    "LUN#0", &pLunL0);
        InsertConfigString(pLunL0, "Driver",               "HGCM");
        InsertConfigNode(pLunL0,   "Config", &pCfg);

        /*
         * Attach the status driver.
         */
        i_attachStatusDriver(pInst, DeviceType_SharedFolder);

        /*
         * Audio configuration.
         */

        /*
         * AC'97 ICH / SoundBlaster16 audio / Intel HD Audio.
         */
        ComPtr<IAudioSettings> audioSettings;
        hrc = pMachine->COMGETTER(AudioSettings)(audioSettings.asOutParam());               H();

        BOOL fAudioEnabled = FALSE;
        ComPtr<IAudioAdapter> audioAdapter;
        hrc = audioSettings->COMGETTER(Adapter)(audioAdapter.asOutParam());                 H();
        if (audioAdapter)
        {
            hrc = audioAdapter->COMGETTER(Enabled)(&fAudioEnabled);                         H();
        }

        if (fAudioEnabled)
        {
            AudioControllerType_T enmAudioController;
            hrc = audioAdapter->COMGETTER(AudioController)(&enmAudioController);            H();
            AudioCodecType_T enmAudioCodec;
            hrc = audioAdapter->COMGETTER(AudioCodec)(&enmAudioCodec);                      H();

            GetExtraDataBoth(virtualBox, pMachine, "VBoxInternal2/Audio/Device/TimerHz", &strTmp);
            const uint64_t uTimerHz = strTmp.toUInt64();

            GetExtraDataBoth(virtualBox, pMachine, "VBoxInternal2/Audio/Device/BufSizeInMs", &strTmp);
            const uint64_t uBufSizeInMs = strTmp.toUInt64();

            GetExtraDataBoth(virtualBox, pMachine, "VBoxInternal2/Audio/Device/BufSizeOutMs", &strTmp);
            const uint64_t uBufSizeOutMs = strTmp.toUInt64();

            GetExtraDataBoth(virtualBox, pMachine, "VBoxInternal2/Audio/Debug/Enabled", &strTmp);
            const bool fDebugEnabled = strTmp.equalsIgnoreCase("true") || strTmp.equalsIgnoreCase("1");

            GetExtraDataBoth(virtualBox, pMachine, "VBoxInternal2/Audio/Debug/Level", &strTmp);
            const uint32_t uDebugLevel = strTmp.toUInt32();

            Utf8Str strDebugPathOut;
            GetExtraDataBoth(virtualBox, pMachine, "VBoxInternal2/Audio/Debug/PathOut", &strDebugPathOut);

#ifdef VBOX_WITH_AUDIO_VALIDATIONKIT
            GetExtraDataBoth(virtualBox, pMachine, "VBoxInternal2/Audio/VaKit/Enabled", &strTmp); /* Deprecated; do not use! */
            if (strTmp.isEmpty())
                GetExtraDataBoth(virtualBox, pMachine, "VBoxInternal2/Audio/ValKit/Enabled", &strTmp);
            /* Whether the Validation Kit audio backend runs as the primary backend.
             * Can also be used with VBox release builds. */
            const bool fValKitEnabled = strTmp.equalsIgnoreCase("true") || strTmp.equalsIgnoreCase("1");
#endif
            /** @todo Implement an audio device class, similar to the audio backend class, to construct the common stuff
             *        without duplicating (more) code. */

            const char *pszAudioDevice;
            switch (enmAudioController)
            {
                case AudioControllerType_AC97:
                {
                    /* ICH AC'97. */
                    pszAudioDevice = "ichac97";

                    InsertConfigNode(pDevices,      pszAudioDevice,         &pDev);
                    InsertConfigNode(pDev,          "0",                    &pInst);
                    InsertConfigInteger(pInst,      "Trusted",              1); /* boolean */
                    hrc = pBusMgr->assignPCIDevice(pszAudioDevice, pInst);              H();
                    InsertConfigNode(pInst,         "Config",               &pCfg);
                    switch (enmAudioCodec)
                    {
                        case AudioCodecType_STAC9700:
                            InsertConfigString(pCfg, "Codec",               "STAC9700");
                            break;
                        case AudioCodecType_AD1980:
                            InsertConfigString(pCfg, "Codec",               "AD1980");
                            break;
                        default: AssertFailedBreak();
                    }
                    if (uTimerHz)
                        InsertConfigInteger(pCfg,   "TimerHz",              uTimerHz);
                    if (uBufSizeInMs)
                        InsertConfigInteger(pCfg,   "BufSizeInMs",          uBufSizeInMs);
                    if (uBufSizeOutMs)
                        InsertConfigInteger(pCfg,   "BufSizeOutMs",         uBufSizeOutMs);
                    InsertConfigInteger(pCfg,       "DebugEnabled",         fDebugEnabled);
                    if (strDebugPathOut.isNotEmpty())
                        InsertConfigString(pCfg,    "DebugPathOut",         strDebugPathOut);
                    break;
                }
                case AudioControllerType_SB16:
                {
                    /* Legacy SoundBlaster16. */
                    pszAudioDevice = "sb16";

                    InsertConfigNode(pDevices,      pszAudioDevice,         &pDev);
                    InsertConfigNode(pDev,          "0", &pInst);
                    InsertConfigInteger(pInst,      "Trusted",              1); /* boolean */
                    InsertConfigNode(pInst,         "Config",               &pCfg);
                    InsertConfigInteger(pCfg,       "IRQ",                  5);
                    InsertConfigInteger(pCfg,       "DMA",                  1);
                    InsertConfigInteger(pCfg,       "DMA16",                5);
                    InsertConfigInteger(pCfg,       "Port",                 0x220);
                    InsertConfigInteger(pCfg,       "Version",              0x0405);
                    if (uTimerHz)
                        InsertConfigInteger(pCfg,   "TimerHz",              uTimerHz);
                    InsertConfigInteger(pCfg,       "DebugEnabled",         fDebugEnabled);
                    if (strDebugPathOut.isNotEmpty())
                        InsertConfigString(pCfg,    "DebugPathOut",         strDebugPathOut);
                    break;
                }
                case AudioControllerType_HDA:
                {
                    /* Intel HD Audio. */
                    pszAudioDevice = "hda";

                    InsertConfigNode(pDevices,      pszAudioDevice,         &pDev);
                    InsertConfigNode(pDev,          "0",                    &pInst);
                    InsertConfigInteger(pInst,      "Trusted",              1); /* boolean */
                    hrc = pBusMgr->assignPCIDevice(pszAudioDevice, pInst);              H();
                    InsertConfigNode(pInst,         "Config",               &pCfg);
                    if (uBufSizeInMs)
                        InsertConfigInteger(pCfg,   "BufSizeInMs",          uBufSizeInMs);
                    if (uBufSizeOutMs)
                        InsertConfigInteger(pCfg,   "BufSizeOutMs",         uBufSizeOutMs);
                    InsertConfigInteger(pCfg,       "DebugEnabled",         fDebugEnabled);
                    if (strDebugPathOut.isNotEmpty())
                        InsertConfigString(pCfg,    "DebugPathOut",         strDebugPathOut);

                    /* macOS guests uses a different HDA variant to make 10.14+ (or maybe 10.13?) recognize the device. */
                    if (fOsXGuest)
                        InsertConfigString(pCfg,    "DeviceName",           "Intel Sunrise Point");
                    break;
                }
                default:
                    pszAudioDevice = "oops";
                    AssertFailedBreak();
            }

            PCFGMNODE pCfgAudioAdapter = NULL;
            InsertConfigNode(pInst, "AudioConfig", &pCfgAudioAdapter);
            SafeArray<BSTR> audioProps;
            hrc = audioAdapter->COMGETTER(PropertiesList)(ComSafeArrayAsOutParam(audioProps));  H();

            std::list<Utf8Str> audioPropertyNamesList;
            for (size_t i = 0; i < audioProps.size(); ++i)
            {
                Bstr bstrValue;
                audioPropertyNamesList.push_back(Utf8Str(audioProps[i]));
                hrc = audioAdapter->GetProperty(audioProps[i], bstrValue.asOutParam());
                Utf8Str strKey(audioProps[i]);
                InsertConfigString(pCfgAudioAdapter, strKey.c_str(), bstrValue);
            }

            /*
             * The audio driver.
             */
            const char *pszAudioDriver = NULL;
#ifdef VBOX_WITH_AUDIO_VALIDATIONKIT
            if (fValKitEnabled)
            {
                pszAudioDriver = "ValidationKitAudio";
                LogRel(("Audio: ValidationKit driver active\n"));
            }
#endif
            /* If nothing else was selected before, ask the API. */
            if (pszAudioDriver == NULL)
            {
                AudioDriverType_T enmAudioDriver;
                hrc = audioAdapter->COMGETTER(AudioDriver)(&enmAudioDriver);                    H();

                /* The "Default" audio driver needs special treatment, as we need to figure out which driver to use
                 * by default on the current platform. */
                bool const fUseDefaultDrv = enmAudioDriver == AudioDriverType_Default;

                AudioDriverType_T const enmDefaultAudioDriver = settings::MachineConfigFile::getHostDefaultAudioDriver();

                if (fUseDefaultDrv)
                {
                    enmAudioDriver = enmDefaultAudioDriver;
                    if (enmAudioDriver == AudioDriverType_Null)
                        LogRel(("Audio: Warning: No default driver detected for current platform -- defaulting to Null audio backend\n"));
                }

                switch (enmAudioDriver)
                {
                    case AudioDriverType_Default: /* Can't happen, but handle it anyway. */
                        RT_FALL_THROUGH();
                    case AudioDriverType_Null:
                        pszAudioDriver = "NullAudio";
                        break;
#ifdef RT_OS_WINDOWS
# ifdef VBOX_WITH_WINMM
                    case AudioDriverType_WinMM:
#  error "Port WinMM audio backend!" /** @todo Still needed? */
                        break;
# endif
                    case AudioDriverType_DirectSound:
                        /* Use the Windows Audio Session (WAS) API rather than Direct Sound on Windows
                           versions we've tested it on (currently W7+).  Since Vista, Direct Sound has
                           been emulated on top of WAS according to the docs, so better use WAS directly.

                           Set extradata value "VBoxInternal2/Audio/WindowsDrv" "dsound" to no use WasAPI.

                           Keep this hack for backwards compatibility (introduced < 7.0).
                        */
                        GetExtraDataBoth(virtualBox, pMachine, "VBoxInternal2/Audio/WindowsDrv", &strTmp); H();
                        if (   enmDefaultAudioDriver == AudioDriverType_WAS
                            && (   strTmp.isEmpty()
                                || strTmp.equalsIgnoreCase("was")
                                || strTmp.equalsIgnoreCase("wasapi")) )
                        {
                            /* Nothing to do here, fall through to WAS driver. */
                        }
                        else
                        {
                            pszAudioDriver = "DSoundAudio";
                            break;
                        }
                        RT_FALL_THROUGH();
                    case AudioDriverType_WAS:
                        if (enmDefaultAudioDriver == AudioDriverType_WAS) /* WAS supported? */
                            pszAudioDriver = "HostAudioWas";
                        else if (enmDefaultAudioDriver == AudioDriverType_DirectSound)
                        {
                            LogRel(("Audio: Warning: Windows Audio Session (WAS) not supported, defaulting to DirectSound backend\n"));
                            pszAudioDriver = "DSoundAudio";
                        }
                        break;
#endif /* RT_OS_WINDOWS */
#ifdef RT_OS_SOLARIS
                    case AudioDriverType_SolAudio:
                        /* Should not happen, as the Solaris Audio backend is not around anymore.
                         * Remove this sometime later. */
                        LogRel(("Audio: Warning: Solaris Audio is deprecated, please switch to OSS!\n"));
                        LogRel(("Audio: Automatically setting host audio backend to OSS\n"));

                        /* Manually set backend to OSS for now. */
                        pszAudioDriver = "OSSAudio";
                        break;
#endif
#ifdef VBOX_WITH_AUDIO_OSS
                    case AudioDriverType_OSS:
                        pszAudioDriver = "OSSAudio";
                        break;
#endif
#ifdef VBOX_WITH_AUDIO_ALSA
                    case AudioDriverType_ALSA:
                        pszAudioDriver = "ALSAAudio";
                        break;
#endif
#ifdef VBOX_WITH_AUDIO_PULSE
                    case AudioDriverType_Pulse:
                        pszAudioDriver = "PulseAudio";
                        break;
#endif
#ifdef RT_OS_DARWIN
                    case AudioDriverType_CoreAudio:
                        pszAudioDriver = "CoreAudio";
                        break;
#endif
                    default:
                        pszAudioDriver = "oops";
                        AssertFailedBreak();
                }

                if (fUseDefaultDrv)
                    LogRel(("Audio: Detected default audio driver type is '%s'\n", pszAudioDriver));
            }

            BOOL fAudioEnabledIn = FALSE;
            hrc = audioAdapter->COMGETTER(EnabledIn)(&fAudioEnabledIn);                            H();
            BOOL fAudioEnabledOut = FALSE;
            hrc = audioAdapter->COMGETTER(EnabledOut)(&fAudioEnabledOut);                          H();

            unsigned idxAudioLun = 0;

            InsertConfigNodeF(pInst, &pLunL0, "LUN#%u", idxAudioLun);
            i_configAudioDriver(virtualBox, pMachine, pLunL0, pszAudioDriver, !!fAudioEnabledIn, !!fAudioEnabledOut);
            idxAudioLun++;

#ifdef VBOX_WITH_AUDIO_VRDE
            /* Insert dummy audio driver to have the LUN configured. */
            InsertConfigNodeF(pInst, &pLunL0, "LUN#%u", idxAudioLun);
            InsertConfigString(pLunL0, "Driver", "AUDIO");
            {
                AudioDriverCfg DrvCfgVRDE(pszAudioDevice, 0 /* Instance */, idxAudioLun, "AudioVRDE",
                                          !!fAudioEnabledIn, !!fAudioEnabledOut);
                vrc = mAudioVRDE->InitializeConfig(&DrvCfgVRDE);
                AssertRCStmt(vrc, throw ConfigError(__FUNCTION__, vrc, "mAudioVRDE->InitializeConfig failed"));
            }
            idxAudioLun++;
#endif

#ifdef VBOX_WITH_AUDIO_RECORDING
            /* Insert dummy audio driver to have the LUN configured. */
            InsertConfigNodeF(pInst, &pLunL0, "LUN#%u", idxAudioLun);
            InsertConfigString(pLunL0, "Driver", "AUDIO");
            {
                AudioDriverCfg DrvCfgVideoRec(pszAudioDevice, 0 /* Instance */, idxAudioLun, "AudioVideoRec",
                                              false /*a_fEnabledIn*/, true /*a_fEnabledOut*/);
                vrc = mRecording.mAudioRec->InitializeConfig(&DrvCfgVideoRec);
                AssertRCStmt(vrc, throw ConfigError(__FUNCTION__, vrc, "Recording.mAudioRec->InitializeConfig failed"));
            }
            idxAudioLun++;
#endif

            if (fDebugEnabled)
            {
#ifdef VBOX_WITH_AUDIO_DEBUG
# ifdef VBOX_WITH_AUDIO_VALIDATIONKIT
                /*
                 * When both, ValidationKit and Debug mode (for audio) are enabled,
                 * skip configuring the Debug audio driver, as both modes can
                 * mess with the audio data and would lead to side effects.
                 *
                 * The ValidationKit audio driver has precedence over the Debug audio driver.
                 *
                 * This also can (and will) be used in VBox release builds.
                 */
                if (fValKitEnabled)
                {
                    LogRel(("Audio: Warning: ValidationKit running and Debug mode enabled -- disabling Debug driver\n"));
                }
                else /* Debug mode active -- run both (nice for catching errors / doing development). */
                {
                    /*
                     * The ValidationKit backend.
                     */
                    InsertConfigNodeF(pInst, &pLunL0, "LUN#%u", idxAudioLun);
                    i_configAudioDriver(virtualBox, pMachine, pLunL0, "ValidationKitAudio",
                                        !!fAudioEnabledIn, !!fAudioEnabledOut);
                    idxAudioLun++;
# endif /* VBOX_WITH_AUDIO_VALIDATIONKIT */
                    /*
                     * The Debug audio backend.
                     */
                    InsertConfigNodeF(pInst, &pLunL0, "LUN#%u", idxAudioLun);
                    i_configAudioDriver(virtualBox, pMachine, pLunL0, "DebugAudio",
                                        !!fAudioEnabledIn, !!fAudioEnabledOut);
                    idxAudioLun++;
# ifdef VBOX_WITH_AUDIO_VALIDATIONKIT
                }
# endif /* VBOX_WITH_AUDIO_VALIDATIONKIT */
#endif /* VBOX_WITH_AUDIO_DEBUG */

                /*
                 * Tweak the logging groups.
                 */
                Utf8Str strGroups("drv_audio.e.l.l2.l3.f"
                                  " audio_mixer.e.l.l2.l3.f"
                                  " dev_hda_codec.e.l.l2.l3.f"
                                  " dev_hda.e.l.l2.l3.f"
                                  " dev_ac97.e.l.l2.l3.f"
                                  " dev_sb16.e.l.l2.l3.f");

                LogRel(("Audio: Debug level set to %RU32\n", uDebugLevel));

                switch (uDebugLevel)
                {
                    case 0:
                        strGroups += " drv_host_audio.e.l.l2.l3.f";
                        break;
                    case 1:
                        RT_FALL_THROUGH();
                    case 2:
                        RT_FALL_THROUGH();
                    case 3:
                        strGroups += " drv_host_audio.e.l.l2.l3.f+audio_test.e.l.l2.l3.f";
                        break;
                    case 4:
                        RT_FALL_THROUGH();
                    default:
                        strGroups += " drv_host_audio.e.l.l2.l3.l4.f+audio_test.e.l.l2.l3.l4.f";
                        break;
                }

                vrc = RTLogGroupSettings(RTLogRelGetDefaultInstance(), strGroups.c_str());
                if (RT_FAILURE(vrc))
                    LogRel(("Audio: Setting debug logging failed, vrc=%Rrc\n", vrc));
            }
        }

#ifdef VBOX_WITH_SHARED_CLIPBOARD
        /*
         * Shared Clipboard.
         */
        {
            ClipboardMode_T enmClipboardMode = ClipboardMode_Disabled;
            hrc = pMachine->COMGETTER(ClipboardMode)(&enmClipboardMode); H();
# ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
            BOOL fFileTransfersEnabled;
            hrc = pMachine->COMGETTER(ClipboardFileTransfersEnabled)(&fFileTransfersEnabled); H();
#endif

            /* Load the service */
            vrc = pVMMDev->hgcmLoadService("VBoxSharedClipboard", "VBoxSharedClipboard");
            if (RT_SUCCESS(vrc))
            {
                LogRel(("Shared Clipboard: Service loaded\n"));

                /* Set initial clipboard mode. */
                vrc = i_changeClipboardMode(enmClipboardMode);
                AssertLogRelMsg(RT_SUCCESS(vrc), ("Shared Clipboard: Failed to set initial clipboard mode (%d): vrc=%Rrc\n",
                                                 enmClipboardMode, vrc));

                /* Setup the service. */
                VBOXHGCMSVCPARM parm;
                HGCMSvcSetU32(&parm, !i_useHostClipboard());
                vrc = pVMMDev->hgcmHostCall("VBoxSharedClipboard", VBOX_SHCL_HOST_FN_SET_HEADLESS, 1, &parm);
                AssertLogRelMsg(RT_SUCCESS(vrc), ("Shared Clipboard: Failed to set initial headless mode (%RTbool): vrc=%Rrc\n",
                                                 !i_useHostClipboard(), vrc));

# ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
                vrc = i_changeClipboardFileTransferMode(RT_BOOL(fFileTransfersEnabled));
                AssertLogRelMsg(RT_SUCCESS(vrc), ("Shared Clipboard: Failed to set initial file transfers mode (%u): vrc=%Rrc\n",
                                                 fFileTransfersEnabled, vrc));

                /** @todo Register area callbacks? (See also deregistration todo in Console::i_powerDown.) */
# endif
            }
            else
                LogRel(("Shared Clipboard: Not available, vrc=%Rrc\n", vrc));
            vrc = VINF_SUCCESS;  /* None of the potential failures above are fatal. */
        }
#endif /* VBOX_WITH_SHARED_CLIPBOARD */

        /*
         * HGCM HostChannel.
         */
        {
            Bstr value;
            hrc = pMachine->GetExtraData(Bstr("HGCM/HostChannel").raw(),
                                         value.asOutParam());

            if (   hrc   == S_OK
                && value == "1")
            {
                vrc = pVMMDev->hgcmLoadService("VBoxHostChannel", "VBoxHostChannel");
                if (RT_FAILURE(vrc))
                {
                    LogRel(("VBoxHostChannel is not available, vrc=%Rrc\n", vrc));
                    /* That is not a fatal failure. */
                    vrc = VINF_SUCCESS;
                }
            }
        }

#ifdef VBOX_WITH_DRAG_AND_DROP
        /*
         * Drag and Drop.
         */
        {
            DnDMode_T enmMode = DnDMode_Disabled;
            hrc = pMachine->COMGETTER(DnDMode)(&enmMode);                                   H();

            /* Load the service */
            vrc = pVMMDev->hgcmLoadService("VBoxDragAndDropSvc", "VBoxDragAndDropSvc");
            if (RT_FAILURE(vrc))
            {
                LogRel(("Drag and drop service is not available, vrc=%Rrc\n", vrc));
                /* That is not a fatal failure. */
                vrc = VINF_SUCCESS;
            }
            else
            {
                vrc = HGCMHostRegisterServiceExtension(&m_hHgcmSvcExtDragAndDrop, "VBoxDragAndDropSvc",
                                                       &GuestDnD::notifyDnDDispatcher,
                                                       GuestDnDInst());
                if (RT_FAILURE(vrc))
                    Log(("Cannot register VBoxDragAndDropSvc extension, vrc=%Rrc\n", vrc));
                else
                {
                    LogRel(("Drag and drop service loaded\n"));
                    vrc = i_changeDnDMode(enmMode);
                }
            }
        }
#endif /* VBOX_WITH_DRAG_AND_DROP */

#if defined(VBOX_WITH_TPM)
        /*
         * Configure the Trusted Platform Module.
         */
        ComObjPtr<ITrustedPlatformModule> ptrTpm;
        TpmType_T enmTpmType = TpmType_None;

        hrc = pMachine->COMGETTER(TrustedPlatformModule)(ptrTpm.asOutParam());              H();
        hrc = ptrTpm->COMGETTER(Type)(&enmTpmType);                                         H();
        if (enmTpmType != TpmType_None)
        {
            InsertConfigNode(pDevices, "tpm", &pDev);
            InsertConfigNode(pDev,     "0", &pInst);
            InsertConfigInteger(pInst, "Trusted", 1); /* boolean */
            InsertConfigNode(pInst,    "Config", &pCfg);
            InsertConfigNode(pInst,    "LUN#0", &pLunL0);

            switch (enmTpmType)
            {
                case TpmType_v1_2:
                case TpmType_v2_0:
                    InsertConfigString(pLunL0, "Driver",               "TpmEmuTpms");
                    InsertConfigNode(pLunL0,   "Config", &pCfg);
                    InsertConfigInteger(pCfg, "TpmVersion", enmTpmType == TpmType_v1_2 ? 1 : 2);
                    InsertConfigNode(pLunL0, "AttachedDriver", &pLunL1);
                    InsertConfigString(pLunL1, "Driver", "NvramStore");
                    break;
                case TpmType_Host:
#if defined(RT_OS_LINUX) || defined(RT_OS_WINDOWS)
                    InsertConfigString(pLunL0, "Driver",               "TpmHost");
                    InsertConfigNode(pLunL0,   "Config", &pCfg);
#endif
                    break;
                case TpmType_Swtpm:
                    hrc = ptrTpm->COMGETTER(Location)(bstr.asOutParam());                   H();
                    InsertConfigString(pLunL0, "Driver",               "TpmEmu");
                    InsertConfigNode(pLunL0,   "Config", &pCfg);
                    InsertConfigString(pCfg,   "Location", bstr);
                    break;
                default:
                    AssertFailedBreak();
            }
        }
#endif

        /*
         * ACPI
         */
        BOOL fACPI;
        hrc = biosSettings->COMGETTER(ACPIEnabled)(&fACPI);                                 H();
        if (fACPI)
        {
            /* Always show the CPU leafs when we have multiple VCPUs or when the IO-APIC is enabled.
             * The Windows SMP kernel needs a CPU leaf or else its idle loop will burn cpu cycles; the
             * intelppm driver refuses to register an idle state handler.
             * Always show CPU leafs for OS X guests. */
            BOOL fShowCpu = fOsXGuest;
            if (cCpus > 1 || fIOAPIC)
                fShowCpu = true;

            BOOL fCpuHotPlug;
            hrc = pMachine->COMGETTER(CPUHotPlugEnabled)(&fCpuHotPlug);                     H();

            InsertConfigNode(pDevices, "acpi", &pDev);
            InsertConfigNode(pDev,     "0", &pInst);
            InsertConfigInteger(pInst, "Trusted", 1); /* boolean */
            InsertConfigNode(pInst,    "Config", &pCfg);
            hrc = pBusMgr->assignPCIDevice("acpi", pInst);                                  H();

            InsertConfigInteger(pCfg,  "NumCPUs",          cCpus);

            InsertConfigInteger(pCfg,  "IOAPIC", fIOAPIC);
            InsertConfigInteger(pCfg,  "FdcEnabled", fFdcEnabled);
            InsertConfigInteger(pCfg,  "HpetEnabled", fHPETEnabled);
            InsertConfigInteger(pCfg,  "SmcEnabled", fSmcEnabled);
            InsertConfigInteger(pCfg,  "ShowRtc",    fShowRtc);
            if (fOsXGuest && !llBootNics.empty())
            {
                BootNic aNic = llBootNics.front();
                uint32_t u32NicPCIAddr = (aNic.mPCIAddress.miDevice << 16) | aNic.mPCIAddress.miFn;
                InsertConfigInteger(pCfg, "NicPciAddress",    u32NicPCIAddr);
            }
            if (fOsXGuest && fAudioEnabled)
            {
                PCIBusAddress Address;
                if (pBusMgr->findPCIAddress("hda", 0, Address))
                {
                    uint32_t u32AudioPCIAddr = (Address.miDevice << 16) | Address.miFn;
                    InsertConfigInteger(pCfg, "AudioPciAddress",    u32AudioPCIAddr);
                }
            }
            if (fOsXGuest)
            {
                PCIBusAddress Address;
                if (pBusMgr->findPCIAddress("nvme", 0, Address))
                {
                    uint32_t u32NvmePCIAddr = (Address.miDevice << 16) | Address.miFn;
                    InsertConfigInteger(pCfg, "NvmePciAddress",    u32NvmePCIAddr);
                }
            }
            if (enmIommuType == IommuType_AMD)
            {
                PCIBusAddress Address;
                if (pBusMgr->findPCIAddress("iommu-amd", 0, Address))
                {
                    uint32_t u32IommuAddress = (Address.miDevice << 16) | Address.miFn;
                    InsertConfigInteger(pCfg, "IommuAmdEnabled", true);
                    InsertConfigInteger(pCfg, "IommuPciAddress", u32IommuAddress);
                    if (pBusMgr->findPCIAddress("sb-ioapic", 0, Address))
                    {
                        uint32_t const u32SbIoapicAddress = (Address.miDevice << 16) | Address.miFn;
                        InsertConfigInteger(pCfg, "SbIoApicPciAddress", u32SbIoapicAddress);
                    }
                    else
                        return pVMM->pfnVMR3SetError(pUVM, VERR_INVALID_PARAMETER, RT_SRC_POS,
                                                     N_("AMD IOMMU is enabled, but the I/O APIC is not assigned a PCI address!"));
                }
            }
            else if (enmIommuType == IommuType_Intel)
            {
                PCIBusAddress Address;
                if (pBusMgr->findPCIAddress("iommu-intel", 0, Address))
                {
                    uint32_t u32IommuAddress = (Address.miDevice << 16) | Address.miFn;
                    InsertConfigInteger(pCfg, "IommuIntelEnabled", true);
                    InsertConfigInteger(pCfg, "IommuPciAddress", u32IommuAddress);
                    if (pBusMgr->findPCIAddress("sb-ioapic", 0, Address))
                    {
                        uint32_t const u32SbIoapicAddress = (Address.miDevice << 16) | Address.miFn;
                        InsertConfigInteger(pCfg, "SbIoApicPciAddress", u32SbIoapicAddress);
                    }
                    else
                        return pVMM->pfnVMR3SetError(pUVM, VERR_INVALID_PARAMETER, RT_SRC_POS,
                                                     N_("Intel IOMMU is enabled, but the I/O APIC is not assigned a PCI address!"));
                }
            }

            InsertConfigInteger(pCfg,  "IocPciAddress", uIocPCIAddress);
            if (chipsetType == ChipsetType_ICH9)
            {
                InsertConfigInteger(pCfg,  "McfgBase",   uMcfgBase);
                InsertConfigInteger(pCfg,  "McfgLength", cbMcfgLength);
                /* 64-bit prefetch window root resource: Only for ICH9 and if PAE or Long Mode is enabled (@bugref{5454}). */
                if (fIsGuest64Bit || fEnablePAE)
                    InsertConfigInteger(pCfg,  "PciPref64Enabled", 1);
            }
            InsertConfigInteger(pCfg,  "HostBusPciAddress", uHbcPCIAddress);
            InsertConfigInteger(pCfg,  "ShowCpu", fShowCpu);
            InsertConfigInteger(pCfg,  "CpuHotPlug", fCpuHotPlug);

            InsertConfigInteger(pCfg,  "Serial0IoPortBase", auSerialIoPortBase[0]);
            InsertConfigInteger(pCfg,  "Serial0Irq", auSerialIrq[0]);

            InsertConfigInteger(pCfg,  "Serial1IoPortBase", auSerialIoPortBase[1]);
            InsertConfigInteger(pCfg,  "Serial1Irq", auSerialIrq[1]);

            if (auSerialIoPortBase[2])
            {
                InsertConfigInteger(pCfg,  "Serial2IoPortBase", auSerialIoPortBase[2]);
                InsertConfigInteger(pCfg,  "Serial2Irq", auSerialIrq[2]);
            }

            if (auSerialIoPortBase[3])
            {
                InsertConfigInteger(pCfg,  "Serial3IoPortBase", auSerialIoPortBase[3]);
                InsertConfigInteger(pCfg,  "Serial3Irq", auSerialIrq[3]);
            }

            InsertConfigInteger(pCfg,  "Parallel0IoPortBase", auParallelIoPortBase[0]);
            InsertConfigInteger(pCfg,  "Parallel0Irq", auParallelIrq[0]);

            InsertConfigInteger(pCfg,  "Parallel1IoPortBase", auParallelIoPortBase[1]);
            InsertConfigInteger(pCfg,  "Parallel1Irq", auParallelIrq[1]);

#if defined(VBOX_WITH_TPM)
            switch (enmTpmType)
            {
                case TpmType_v1_2:
                    InsertConfigString(pCfg, "TpmMode", "tis1.2");
                    break;
                case TpmType_v2_0:
                    InsertConfigString(pCfg, "TpmMode", "fifo2.0");
                    break;
                /** @todo Host and swtpm. */
                default:
                    break;
            }
#endif

            InsertConfigNode(pInst,    "LUN#0", &pLunL0);
            InsertConfigString(pLunL0, "Driver",               "ACPIHost");
            InsertConfigNode(pLunL0,   "Config", &pCfg);

            /* Attach the dummy CPU drivers */
            for (ULONG iCpuCurr = 1; iCpuCurr < cCpus; iCpuCurr++)
            {
                BOOL fCpuAttached = true;

                if (fCpuHotPlug)
                {
                    hrc = pMachine->GetCPUStatus(iCpuCurr, &fCpuAttached);                  H();
                }

                if (fCpuAttached)
                {
                    InsertConfigNode(pInst, Utf8StrFmt("LUN#%u", iCpuCurr).c_str(), &pLunL0);
                    InsertConfigString(pLunL0, "Driver",           "ACPICpu");
                    InsertConfigNode(pLunL0,   "Config", &pCfg);
                }
            }
        }

        /*
         * Configure DBGF (Debug(ger) Facility) and DBGC (Debugger Console).
         */
        {
            PCFGMNODE pDbgf;
            InsertConfigNode(pRoot, "DBGF", &pDbgf);

            /* Paths to search for debug info and such things. */
            hrc = pMachine->COMGETTER(SettingsFilePath)(bstr.asOutParam());                 H();
            Utf8Str strSettingsPath(bstr);
            bstr.setNull();
            strSettingsPath.stripFilename();
            strSettingsPath.append("/");

            char szHomeDir[RTPATH_MAX + 1];
            int vrc2 = RTPathUserHome(szHomeDir, sizeof(szHomeDir) - 1);
            if (RT_FAILURE(vrc2))
                szHomeDir[0] = '\0';
            RTPathEnsureTrailingSeparator(szHomeDir, sizeof(szHomeDir));


            Utf8Str strPath;
            strPath.append(strSettingsPath).append("debug/;");
            strPath.append(strSettingsPath).append(";");
            strPath.append("cache*").append(strSettingsPath).append("dbgcache/;"); /* handy for symlinking to actual cache */
            strPath.append(szHomeDir);

            InsertConfigString(pDbgf, "Path", strPath.c_str());

            /* Tracing configuration. */
            BOOL fTracingEnabled;
            hrc = pMachine->COMGETTER(TracingEnabled)(&fTracingEnabled);                    H();
            if (fTracingEnabled)
                InsertConfigInteger(pDbgf, "TracingEnabled", 1);

            hrc = pMachine->COMGETTER(TracingConfig)(bstr.asOutParam());                    H();
            if (fTracingEnabled)
                InsertConfigString(pDbgf, "TracingConfig", bstr);

            BOOL fAllowTracingToAccessVM;
            hrc = pMachine->COMGETTER(AllowTracingToAccessVM)(&fAllowTracingToAccessVM);    H();
            if (fAllowTracingToAccessVM)
                InsertConfigInteger(pPDM, "AllowTracingToAccessVM", 1);

            /* Debugger console config. */
            PCFGMNODE pDbgc;
            InsertConfigNode(pRoot, "DBGC", &pDbgc);

            hrc = virtualBox->COMGETTER(HomeFolder)(bstr.asOutParam());                     H();
            Utf8Str strVBoxHome = bstr;
            bstr.setNull();
            if (strVBoxHome.isNotEmpty())
                strVBoxHome.append("/");
            else
            {
                strVBoxHome = szHomeDir;
                strVBoxHome.append("/.vbox");
            }

            Utf8Str strFile(strVBoxHome);
            strFile.append("dbgc-history");
            InsertConfigString(pDbgc, "HistoryFile", strFile);

            strFile = strSettingsPath;
            strFile.append("dbgc-init");
            InsertConfigString(pDbgc, "LocalInitScript", strFile);

            strFile = strVBoxHome;
            strFile.append("dbgc-init");
            InsertConfigString(pDbgc, "GlobalInitScript", strFile);

            /*
             * Configure guest debug settings.
             */
            ComObjPtr<IGuestDebugControl> ptrGstDbgCtrl;
            GuestDebugProvider_T enmGstDbgProvider = GuestDebugProvider_None;

            hrc = pMachine->COMGETTER(GuestDebugControl)(ptrGstDbgCtrl.asOutParam());           H();
            hrc = ptrGstDbgCtrl->COMGETTER(DebugProvider)(&enmGstDbgProvider);                  H();
            if (enmGstDbgProvider != GuestDebugProvider_None)
            {
                GuestDebugIoProvider_T enmGstDbgIoProvider = GuestDebugIoProvider_None;
                hrc = ptrGstDbgCtrl->COMGETTER(DebugIoProvider)(&enmGstDbgIoProvider);          H();
                hrc = ptrGstDbgCtrl->COMGETTER(DebugAddress)(bstr.asOutParam());                H();
                Utf8Str strAddress = bstr;
                bstr.setNull();

                ULONG ulPort = 0;
                hrc = ptrGstDbgCtrl->COMGETTER(DebugPort)(&ulPort);                             H();

                PCFGMNODE pDbgSettings;
                InsertConfigNode(pDbgc, "Dbg", &pDbgSettings);
                InsertConfigString(pDbgSettings, "Address", strAddress);
                InsertConfigInteger(pDbgSettings, "Port", ulPort);

                switch (enmGstDbgProvider)
                {
                    case GuestDebugProvider_Native:
                        InsertConfigString(pDbgSettings, "StubType", "Native");
                        break;
                    case GuestDebugProvider_GDB:
                        InsertConfigString(pDbgSettings, "StubType", "Gdb");
                        break;
                    case GuestDebugProvider_KD:
                        InsertConfigString(pDbgSettings, "StubType", "Kd");
                        break;
                    default:
                        AssertFailed();
                        break;
                }

                switch (enmGstDbgIoProvider)
                {
                    case GuestDebugIoProvider_TCP:
                        InsertConfigString(pDbgSettings, "Provider", "tcp");
                        break;
                    case GuestDebugIoProvider_UDP:
                        InsertConfigString(pDbgSettings, "Provider", "udp");
                        break;
                    case GuestDebugIoProvider_IPC:
                        InsertConfigString(pDbgSettings, "Provider", "ipc");
                        break;
                    default:
                        AssertFailed();
                        break;
                }
            }
        }
    }
    catch (ConfigError &x)
    {
        // InsertConfig threw something:
        pVMM->pfnVMR3SetError(pUVM, x.m_vrc, RT_SRC_POS, "Caught ConfigError: %Rrc - %s", x.m_vrc, x.what());
        return x.m_vrc;
    }
    catch (HRESULT hrcXcpt)
    {
        AssertLogRelMsgFailedReturn(("hrc=%Rhrc\n", hrcXcpt), VERR_MAIN_CONFIG_CONSTRUCTOR_COM_ERROR);
    }

#ifdef VBOX_WITH_EXTPACK
    /*
     * Call the extension pack hooks if everything went well thus far.
     */
    if (RT_SUCCESS(vrc))
    {
        pAlock->release();
        vrc = mptrExtPackManager->i_callAllVmConfigureVmmHooks(this, pVM, pVMM);
        pAlock->acquire();
    }
#endif

    /*
     * Apply the CFGM overlay.
     */
    if (RT_SUCCESS(vrc))
        vrc = i_configCfgmOverlay(pRoot, virtualBox, pMachine);

    /*
     * Dump all extradata API settings tweaks, both global and per VM.
     */
    if (RT_SUCCESS(vrc))
        vrc = i_configDumpAPISettingsTweaks(virtualBox, pMachine);

#undef H

    pAlock->release(); /* Avoid triggering the lock order inversion check. */

    /*
     * Register VM state change handler.
     */
    int vrc2 = pVMM->pfnVMR3AtStateRegister(pUVM, Console::i_vmstateChangeCallback, this);
    AssertRC(vrc2);
    if (RT_SUCCESS(vrc))
        vrc = vrc2;

    /*
     * Register VM runtime error handler.
     */
    vrc2 = pVMM->pfnVMR3AtRuntimeErrorRegister(pUVM, Console::i_atVMRuntimeErrorCallback, this);
    AssertRC(vrc2);
    if (RT_SUCCESS(vrc))
        vrc = vrc2;

    pAlock->acquire();

    LogFlowFunc(("vrc = %Rrc\n", vrc));
    LogFlowFuncLeave();

    return vrc;
}

/**
 * Configures an audio driver via CFGM by getting (optional) values from extra data.
 *
 * @param   pVirtualBox         Pointer to IVirtualBox instance.
 * @param   pMachine            Pointer to IMachine instance.
 * @param   pLUN                Pointer to CFGM node of LUN (the driver) to configure.
 * @param   pszDrvName          Name of the driver to configure.
 * @param   fAudioEnabledIn     IAudioAdapter::enabledIn value.
 * @param   fAudioEnabledOut    IAudioAdapter::enabledOut value.
 *
 * @throws ConfigError or HRESULT on if there is trouble.
 */
void Console::i_configAudioDriver(IVirtualBox *pVirtualBox, IMachine *pMachine, PCFGMNODE pLUN, const char *pszDrvName,
                                  bool fAudioEnabledIn, bool fAudioEnabledOut)
{
#define H()     AssertLogRelMsgStmt(!FAILED(hrc), ("hrc=%Rhrc\n", hrc), \
                                    throw ConfigError(__FUNCTION__, VERR_MAIN_CONFIG_CONSTRUCTOR_COM_ERROR, "line: " RT_XSTR(__LINE__)))

    InsertConfigString(pLUN, "Driver", "AUDIO");

    PCFGMNODE pCfg;
    InsertConfigNode(pLUN,   "Config", &pCfg);
    InsertConfigString(pCfg,  "DriverName",    pszDrvName);
    InsertConfigInteger(pCfg, "InputEnabled",  fAudioEnabledIn);
    InsertConfigInteger(pCfg, "OutputEnabled", fAudioEnabledOut);

    Utf8Str strTmp;
    GetExtraDataBoth(pVirtualBox, pMachine, "VBoxInternal2/Audio/Debug/Enabled", &strTmp);
    const uint64_t fDebugEnabled = strTmp.equalsIgnoreCase("true") || strTmp.equalsIgnoreCase("1");
    if (fDebugEnabled)
    {
        InsertConfigInteger(pCfg, "DebugEnabled",  fDebugEnabled);

        Utf8Str strDebugPathOut;
        GetExtraDataBoth(pVirtualBox, pMachine, "VBoxInternal2/Audio/Debug/PathOut", &strDebugPathOut);
        InsertConfigString(pCfg, "DebugPathOut",  strDebugPathOut.c_str());
    }

    /*
     * PCM input parameters (playback + recording).
     * We have host driver specific ones as: VBoxInternal2/Audio/<DrvName>/<Value>
     * And global ones for all host drivers: VBoxInternal2/Audio/<Value>
     */
    for (unsigned iDir = 0; iDir < 2; iDir++)
    {
        static const struct
        {
            const char *pszExtraName;
            const char *pszCfgmName;
        } s_aToCopy[] =
        {   /* PCM  parameters: */
            { "PCMSampleBit",        "PCMSampleBit"        },
            { "PCMSampleHz",         "PCMSampleHz"         },
            { "PCMSampleSigned",     "PCMSampleSigned"     },
            { "PCMSampleSwapEndian", "PCMSampleSwapEndian" },
            { "PCMSampleChannels",   "PCMSampleChannels"   },
            /* Buffering stuff: */
            { "PeriodSizeMs",        "PeriodSizeMs"        },
            { "BufferSizeMs",        "BufferSizeMs"        },
            { "PreBufferSizeMs",     "PreBufferSizeMs"     },
        };

        PCFGMNODE   pDirNode = NULL;
        const char *pszDir   = iDir == 0 ? "In" : "Out";
        for (size_t i = 0; i < RT_ELEMENTS(s_aToCopy); i++)
        {
            char szExtra[128];
            RTStrPrintf(szExtra, sizeof(szExtra), "VBoxInternal2/Audio/%s/%s%s", pszDrvName, s_aToCopy[i].pszExtraName, pszDir);
            GetExtraDataBoth(pVirtualBox, pMachine, szExtra, &strTmp); /* throws hrc */
            if (strTmp.isEmpty())
            {
                RTStrPrintf(szExtra, sizeof(szExtra), "VBoxInternal2/Audio/%s%s", s_aToCopy[i].pszExtraName, pszDir);
                GetExtraDataBoth(pVirtualBox, pMachine, szExtra, &strTmp);
                if (strTmp.isEmpty())
                    continue;
            }

            uint32_t uValue;
            int vrc = RTStrToUInt32Full(strTmp.c_str(), 0, &uValue);
            if (RT_SUCCESS(vrc))
            {
                if (!pDirNode)
                    InsertConfigNode(pCfg, pszDir, &pDirNode);
                InsertConfigInteger(pDirNode, s_aToCopy[i].pszCfgmName, uValue);
            }
            else
                LogRel(("Ignoring malformed 32-bit unsigned integer config value '%s' = '%s': %Rrc\n", szExtra, strTmp.c_str(), vrc));
        }
    }

    PCFGMNODE pLunL1;
    InsertConfigNode(pLUN, "AttachedDriver", &pLunL1);
    InsertConfigString(pLunL1, "Driver", pszDrvName);
    InsertConfigNode(pLunL1, "Config", &pCfg);

#ifdef RT_OS_WINDOWS
    if (strcmp(pszDrvName, "HostAudioWas") == 0)
    {
        Bstr bstrTmp;
        HRESULT hrc = pMachine->COMGETTER(Id)(bstrTmp.asOutParam());                            H();
        InsertConfigString(pCfg, "VmUuid", bstrTmp);
    }
#endif

#if defined(RT_OS_WINDOWS) || defined(RT_OS_LINUX)
    if (   strcmp(pszDrvName, "HostAudioWas") == 0
        || strcmp(pszDrvName, "PulseAudio") == 0)
    {
        Bstr bstrTmp;
        HRESULT hrc = pMachine->COMGETTER(Name)(bstrTmp.asOutParam());                          H();
        InsertConfigString(pCfg, "VmName", bstrTmp);
    }
#endif

    LogFlowFunc(("szDrivName=%s\n", pszDrvName));

#undef H
}

/**
 * Applies the CFGM overlay as specified by VBoxInternal/XXX extra data
 * values.
 *
 * @returns VBox status code.
 * @param   pRoot           The root of the configuration tree.
 * @param   pVirtualBox     Pointer to the IVirtualBox interface.
 * @param   pMachine        Pointer to the IMachine interface.
 */
/* static */
int Console::i_configCfgmOverlay(PCFGMNODE pRoot, IVirtualBox *pVirtualBox, IMachine *pMachine)
{
    /*
     * CFGM overlay handling.
     *
     * Here we check the extra data entries for CFGM values
     * and create the nodes and insert the values on the fly. Existing
     * values will be removed and reinserted. CFGM is typed, so by default
     * we will guess whether it's a string or an integer (byte arrays are
     * not currently supported). It's possible to override this autodetection
     * by adding "string:", "integer:" or "bytes:" (future).
     *
     * We first perform a run on global extra data, then on the machine
     * extra data to support global settings with local overrides.
     */
    int vrc = VINF_SUCCESS;
    bool fFirst = true;
    try
    {
        /** @todo add support for removing nodes and byte blobs. */
        /*
         * Get the next key
         */
        SafeArray<BSTR> aGlobalExtraDataKeys;
        SafeArray<BSTR> aMachineExtraDataKeys;
        HRESULT hrc = pVirtualBox->GetExtraDataKeys(ComSafeArrayAsOutParam(aGlobalExtraDataKeys));
        AssertMsg(SUCCEEDED(hrc), ("VirtualBox::GetExtraDataKeys failed with %Rhrc\n", hrc));

        // remember the no. of global values so we can call the correct method below
        size_t cGlobalValues = aGlobalExtraDataKeys.size();

        hrc = pMachine->GetExtraDataKeys(ComSafeArrayAsOutParam(aMachineExtraDataKeys));
        AssertMsg(SUCCEEDED(hrc), ("Machine::GetExtraDataKeys failed with %Rhrc\n", hrc));

        // build a combined list from global keys...
        std::list<Utf8Str> llExtraDataKeys;

        for (size_t i = 0; i < aGlobalExtraDataKeys.size(); ++i)
            llExtraDataKeys.push_back(Utf8Str(aGlobalExtraDataKeys[i]));
        // ... and machine keys
        for (size_t i = 0; i < aMachineExtraDataKeys.size(); ++i)
            llExtraDataKeys.push_back(Utf8Str(aMachineExtraDataKeys[i]));

        size_t i2 = 0;
        for (std::list<Utf8Str>::const_iterator it = llExtraDataKeys.begin();
            it != llExtraDataKeys.end();
            ++it, ++i2)
        {
            const Utf8Str &strKey = *it;

            /*
             * We only care about keys starting with "VBoxInternal/" (skip "G:" or "M:")
             */
            if (!strKey.startsWith("VBoxInternal/"))
                continue;

            const char *pszExtraDataKey = strKey.c_str() + sizeof("VBoxInternal/") - 1;

            // get the value
            Bstr bstrExtraDataValue;
            if (i2 < cGlobalValues)
                // this is still one of the global values:
                hrc = pVirtualBox->GetExtraData(Bstr(strKey).raw(), bstrExtraDataValue.asOutParam());
            else
                hrc = pMachine->GetExtraData(Bstr(strKey).raw(), bstrExtraDataValue.asOutParam());
            if (FAILED(hrc))
                LogRel(("Warning: Cannot get extra data key %s, hrc = %Rhrc\n", strKey.c_str(), hrc));

            if (fFirst)
            {
                fFirst = false;
                LogRel(("Extradata overrides:\n"));
            }
            LogRel(("  %s=\"%ls\"%s\n", strKey.c_str(), bstrExtraDataValue.raw(), i2 < cGlobalValues ? " (global)" : ""));

            /*
             * The key will be in the format "Node1/Node2/Value" or simply "Value".
             * Split the two and get the node, delete the value and create the node
             * if necessary.
             */
            PCFGMNODE pNode;
            const char *pszCFGMValueName = strrchr(pszExtraDataKey, '/');
            if (pszCFGMValueName)
            {
                /* terminate the node and advance to the value (Utf8Str might not
                offically like this but wtf) */
                *(char *)pszCFGMValueName = '\0';
                ++pszCFGMValueName;

                /* does the node already exist? */
                pNode = mpVMM->pfnCFGMR3GetChild(pRoot, pszExtraDataKey);
                if (pNode)
                    mpVMM->pfnCFGMR3RemoveValue(pNode, pszCFGMValueName);
                else
                {
                    /* create the node */
                    vrc = mpVMM->pfnCFGMR3InsertNode(pRoot, pszExtraDataKey, &pNode);
                    if (RT_FAILURE(vrc))
                    {
                        AssertLogRelMsgRC(vrc, ("failed to insert node '%s'\n", pszExtraDataKey));
                        continue;
                    }
                    Assert(pNode);
                }
            }
            else
            {
                /* root value (no node path). */
                pNode = pRoot;
                pszCFGMValueName = pszExtraDataKey;
                pszExtraDataKey--;
                mpVMM->pfnCFGMR3RemoveValue(pNode, pszCFGMValueName);
            }

            /*
             * Now let's have a look at the value.
             * Empty strings means that we should remove the value, which we've
             * already done above.
             */
            Utf8Str strCFGMValueUtf8(bstrExtraDataValue);
            if (strCFGMValueUtf8.isNotEmpty())
            {
                uint64_t u64Value;

                /* check for type prefix first. */
                if (!strncmp(strCFGMValueUtf8.c_str(), RT_STR_TUPLE("string:")))
                    vrc = mpVMM->pfnCFGMR3InsertString(pNode, pszCFGMValueName, strCFGMValueUtf8.c_str() + sizeof("string:") - 1);
                else if (!strncmp(strCFGMValueUtf8.c_str(), RT_STR_TUPLE("integer:")))
                {
                    vrc = RTStrToUInt64Full(strCFGMValueUtf8.c_str() + sizeof("integer:") - 1, 0, &u64Value);
                    if (RT_SUCCESS(vrc))
                        vrc = mpVMM->pfnCFGMR3InsertInteger(pNode, pszCFGMValueName, u64Value);
                }
                else if (!strncmp(strCFGMValueUtf8.c_str(), RT_STR_TUPLE("bytes:")))
                {
                    char const *pszBase64 = strCFGMValueUtf8.c_str() + sizeof("bytes:") - 1;
                    ssize_t cbValue = RTBase64DecodedSize(pszBase64, NULL);
                    if (cbValue > 0)
                    {
                        void *pvBytes = RTMemTmpAlloc(cbValue);
                        if (pvBytes)
                        {
                            vrc = RTBase64Decode(pszBase64, pvBytes, cbValue, NULL, NULL);
                            if (RT_SUCCESS(vrc))
                                vrc = mpVMM->pfnCFGMR3InsertBytes(pNode, pszCFGMValueName, pvBytes, cbValue);
                            RTMemTmpFree(pvBytes);
                        }
                        else
                            vrc = VERR_NO_TMP_MEMORY;
                    }
                    else if (cbValue == 0)
                        vrc = mpVMM->pfnCFGMR3InsertBytes(pNode, pszCFGMValueName, NULL, 0);
                    else
                        vrc = VERR_INVALID_BASE64_ENCODING;
                }
                /* auto detect type. */
                else if (RT_SUCCESS(RTStrToUInt64Full(strCFGMValueUtf8.c_str(), 0, &u64Value)))
                    vrc = mpVMM->pfnCFGMR3InsertInteger(pNode, pszCFGMValueName, u64Value);
                else
                    vrc = mpVMM->pfnCFGMR3InsertString(pNode, pszCFGMValueName, strCFGMValueUtf8.c_str());
                AssertLogRelMsgRCBreak(vrc, ("failed to insert CFGM value '%s' to key '%s'\n",
                                             strCFGMValueUtf8.c_str(), pszExtraDataKey));
            }
        }
    }
    catch (ConfigError &x)
    {
        // InsertConfig threw something:
        return x.m_vrc;
    }
    return vrc;
}

/**
 * Dumps the API settings tweaks as specified by VBoxInternal2/XXX extra data
 * values.
 *
 * @returns VBox status code.
 * @param   pVirtualBox     Pointer to the IVirtualBox interface.
 * @param   pMachine        Pointer to the IMachine interface.
 */
/* static */
int Console::i_configDumpAPISettingsTweaks(IVirtualBox *pVirtualBox, IMachine *pMachine)
{
    {
        SafeArray<BSTR> aGlobalExtraDataKeys;
        HRESULT hrc = pVirtualBox->GetExtraDataKeys(ComSafeArrayAsOutParam(aGlobalExtraDataKeys));
        AssertMsg(SUCCEEDED(hrc), ("VirtualBox::GetExtraDataKeys failed with %Rhrc\n", hrc));
        bool hasKey = false;
        for (size_t i = 0; i < aGlobalExtraDataKeys.size(); i++)
        {
            Utf8Str strKey(aGlobalExtraDataKeys[i]);
            if (!strKey.startsWith("VBoxInternal2/"))
                continue;

            Bstr bstrValue;
            hrc = pVirtualBox->GetExtraData(Bstr(strKey).raw(),
                                            bstrValue.asOutParam());
            if (FAILED(hrc))
                continue;
            if (!hasKey)
                LogRel(("Global extradata API settings:\n"));
            LogRel(("  %s=\"%ls\"\n", strKey.c_str(), bstrValue.raw()));
            hasKey = true;
        }
    }

    {
        SafeArray<BSTR> aMachineExtraDataKeys;
        HRESULT hrc = pMachine->GetExtraDataKeys(ComSafeArrayAsOutParam(aMachineExtraDataKeys));
        AssertMsg(SUCCEEDED(hrc), ("Machine::GetExtraDataKeys failed with %Rhrc\n", hrc));
        bool hasKey = false;
        for (size_t i = 0; i < aMachineExtraDataKeys.size(); i++)
        {
            Utf8Str strKey(aMachineExtraDataKeys[i]);
            if (!strKey.startsWith("VBoxInternal2/"))
                continue;

            Bstr bstrValue;
            hrc = pMachine->GetExtraData(Bstr(strKey).raw(),
                                         bstrValue.asOutParam());
            if (FAILED(hrc))
                continue;
            if (!hasKey)
                LogRel(("Per-VM extradata API settings:\n"));
            LogRel(("  %s=\"%ls\"\n", strKey.c_str(), bstrValue.raw()));
            hasKey = true;
        }
    }

    return VINF_SUCCESS;
}

int Console::i_configGraphicsController(PCFGMNODE pDevices,
                                        const GraphicsControllerType_T enmGraphicsController,
                                        BusAssignmentManager *pBusMgr,
                                        const ComPtr<IMachine> &ptrMachine,
                                        const ComPtr<IGraphicsAdapter> &ptrGraphicsAdapter,
                                        const ComPtr<IBIOSSettings> &ptrBiosSettings,
                                        bool fHMEnabled)
{
    // InsertConfig* throws
    try
    {
        PCFGMNODE pDev, pInst, pCfg, pLunL0;
        HRESULT hrc;
        Bstr    bstr;
        const char *pcszDevice = "vga";

#define H()         AssertLogRelMsgReturn(!FAILED(hrc), ("hrc=%Rhrc\n", hrc), VERR_MAIN_CONFIG_CONSTRUCTOR_COM_ERROR)
        InsertConfigNode(pDevices, pcszDevice, &pDev);
        InsertConfigNode(pDev,     "0", &pInst);
        InsertConfigInteger(pInst, "Trusted",              1); /* boolean */

        hrc = pBusMgr->assignPCIDevice(pcszDevice, pInst);                                  H();
        InsertConfigNode(pInst,    "Config", &pCfg);
        ULONG cVRamMBs;
        hrc = ptrGraphicsAdapter->COMGETTER(VRAMSize)(&cVRamMBs);                           H();
        InsertConfigInteger(pCfg,  "VRamSize",             cVRamMBs * _1M);
        ULONG cMonitorCount;
        hrc = ptrGraphicsAdapter->COMGETTER(MonitorCount)(&cMonitorCount);                  H();
        InsertConfigInteger(pCfg,  "MonitorCount",         cMonitorCount);
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
        InsertConfigInteger(pCfg,  "R0Enabled",            fHMEnabled);
#else
        NOREF(fHMEnabled);
#endif
        BOOL f3DEnabled;
        hrc = ptrGraphicsAdapter->COMGETTER(Accelerate3DEnabled)(&f3DEnabled);              H();
        InsertConfigInteger(pCfg,  "3DEnabled",            f3DEnabled);

        i_attachStatusDriver(pInst, DeviceType_Graphics3D);

#ifdef VBOX_WITH_VMSVGA
        if (   enmGraphicsController == GraphicsControllerType_VMSVGA
            || enmGraphicsController == GraphicsControllerType_VBoxSVGA)
        {
            InsertConfigInteger(pCfg, "VMSVGAEnabled", true);
            if (enmGraphicsController == GraphicsControllerType_VMSVGA)
            {
                InsertConfigInteger(pCfg, "VMSVGAPciBarLayout", true);
                InsertConfigInteger(pCfg, "VMSVGAPciId", true);
            }
# ifdef VBOX_WITH_VMSVGA3D
            InsertConfigInteger(pCfg, "VMSVGA3dEnabled", f3DEnabled);
# else
            LogRel(("VMSVGA3d not available in this build!\n"));
# endif /* VBOX_WITH_VMSVGA3D */
        }
#else
        RT_NOREF(enmGraphicsController);
#endif /* VBOX_WITH_VMSVGA */

        /* Custom VESA mode list */
        unsigned cModes = 0;
        for (unsigned iMode = 1; iMode <= 16; ++iMode)
        {
            char szExtraDataKey[sizeof("CustomVideoModeXX")];
            RTStrPrintf(szExtraDataKey, sizeof(szExtraDataKey), "CustomVideoMode%u", iMode);
            hrc = ptrMachine->GetExtraData(Bstr(szExtraDataKey).raw(), bstr.asOutParam());  H();
            if (bstr.isEmpty())
                break;
            InsertConfigString(pCfg, szExtraDataKey, bstr);
            ++cModes;
        }
        InsertConfigInteger(pCfg, "CustomVideoModes", cModes);

        /* VESA height reduction */
        ULONG ulHeightReduction;
        IFramebuffer *pFramebuffer = NULL;
        hrc = i_getDisplay()->QueryFramebuffer(0, &pFramebuffer);
        if (SUCCEEDED(hrc) && pFramebuffer)
        {
            hrc = pFramebuffer->COMGETTER(HeightReduction)(&ulHeightReduction);             H();
            pFramebuffer->Release();
            pFramebuffer = NULL;
        }
        else
        {
            /* If framebuffer is not available, there is no height reduction. */
            ulHeightReduction = 0;
        }
        InsertConfigInteger(pCfg,  "HeightReduction", ulHeightReduction);

        /*
         * BIOS logo
         */
        BOOL fFadeIn;
        hrc = ptrBiosSettings->COMGETTER(LogoFadeIn)(&fFadeIn);                             H();
        InsertConfigInteger(pCfg,  "FadeIn",  fFadeIn ? 1 : 0);
        BOOL fFadeOut;
        hrc = ptrBiosSettings->COMGETTER(LogoFadeOut)(&fFadeOut);                           H();
        InsertConfigInteger(pCfg,  "FadeOut", fFadeOut ? 1: 0);
        ULONG logoDisplayTime;
        hrc = ptrBiosSettings->COMGETTER(LogoDisplayTime)(&logoDisplayTime);                H();
        InsertConfigInteger(pCfg,  "LogoTime", logoDisplayTime);
        Bstr bstrLogoImagePath;
        hrc = ptrBiosSettings->COMGETTER(LogoImagePath)(bstrLogoImagePath.asOutParam());    H();
        InsertConfigString(pCfg,   "LogoFile", bstrLogoImagePath);

        /*
         * Boot menu
         */
        BIOSBootMenuMode_T eBootMenuMode;
        int iShowBootMenu;
        hrc = ptrBiosSettings->COMGETTER(BootMenuMode)(&eBootMenuMode);                     H();
        switch (eBootMenuMode)
        {
            case BIOSBootMenuMode_Disabled: iShowBootMenu = 0;  break;
            case BIOSBootMenuMode_MenuOnly: iShowBootMenu = 1;  break;
            default:                        iShowBootMenu = 2;  break;
        }
        InsertConfigInteger(pCfg, "ShowBootMenu", iShowBootMenu);

        /* Attach the display. */
        InsertConfigNode(pInst,    "LUN#0", &pLunL0);
        InsertConfigString(pLunL0, "Driver",               "MainDisplay");
        InsertConfigNode(pLunL0,   "Config", &pCfg);
    }
    catch (ConfigError &x)
    {
        // InsertConfig threw something:
        return x.m_vrc;
    }

#undef H

    return VINF_SUCCESS;
}


/**
 * Ellipsis to va_list wrapper for calling setVMRuntimeErrorCallback.
 */
void Console::i_atVMRuntimeErrorCallbackF(uint32_t fFlags, const char *pszErrorId, const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    i_atVMRuntimeErrorCallback(NULL, this, fFlags, pszErrorId, pszFormat, va);
    va_end(va);
}

/* XXX introduce RT format specifier */
static uint64_t formatDiskSize(uint64_t u64Size, const char **pszUnit)
{
    if (u64Size > INT64_C(5000)*_1G)
    {
        *pszUnit = "TB";
        return u64Size / _1T;
    }
    else if (u64Size > INT64_C(5000)*_1M)
    {
        *pszUnit = "GB";
        return u64Size / _1G;
    }
    else
    {
        *pszUnit = "MB";
        return u64Size / _1M;
    }
}

/**
 * Checks the location of the given medium for known bugs affecting the usage
 * of the host I/O cache setting.
 *
 * @returns VBox status code.
 * @param   pMedium             The medium to check.
 * @param   pfUseHostIOCache    Where to store the suggested host I/O cache setting.
 */
int Console::i_checkMediumLocation(IMedium *pMedium, bool *pfUseHostIOCache)
{
#define H()         AssertLogRelMsgReturn(!FAILED(hrc), ("hrc=%Rhrc\n", hrc), VERR_MAIN_CONFIG_CONSTRUCTOR_COM_ERROR)
    /*
     * Some sanity checks.
     */
    RT_NOREF(pfUseHostIOCache);
    ComPtr<IMediumFormat> pMediumFormat;
    HRESULT hrc = pMedium->COMGETTER(MediumFormat)(pMediumFormat.asOutParam());             H();
    ULONG uCaps = 0;
    com::SafeArray <MediumFormatCapabilities_T> mediumFormatCap;
    hrc = pMediumFormat->COMGETTER(Capabilities)(ComSafeArrayAsOutParam(mediumFormatCap));    H();

    for (ULONG j = 0; j < mediumFormatCap.size(); j++)
        uCaps |= mediumFormatCap[j];

    if (uCaps & MediumFormatCapabilities_File)
    {
        Bstr bstrFile;
        hrc = pMedium->COMGETTER(Location)(bstrFile.asOutParam());                  H();
        Utf8Str const strFile(bstrFile);

        Bstr bstrSnap;
        ComPtr<IMachine> pMachine = i_machine();
        hrc = pMachine->COMGETTER(SnapshotFolder)(bstrSnap.asOutParam());           H();
        Utf8Str const strSnap(bstrSnap);

        RTFSTYPE enmFsTypeFile = RTFSTYPE_UNKNOWN;
        int vrc2 = RTFsQueryType(strFile.c_str(), &enmFsTypeFile);
        AssertMsgRCReturn(vrc2, ("Querying the file type of '%s' failed!\n", strFile.c_str()), vrc2);

         /* Any VM which hasn't created a snapshot or saved the current state of the VM
          * won't have a Snapshot folder yet so no need to log anything about the file system
          * type of the non-existent directory in such cases. */
        RTFSTYPE enmFsTypeSnap = RTFSTYPE_UNKNOWN;
        vrc2 = RTFsQueryType(strSnap.c_str(), &enmFsTypeSnap);
        if (RT_SUCCESS(vrc2) && !mfSnapshotFolderDiskTypeShown)
        {
            LogRel(("File system of '%s' (snapshots) is %s\n", strSnap.c_str(), RTFsTypeName(enmFsTypeSnap)));
            mfSnapshotFolderDiskTypeShown = true;
        }
        LogRel(("File system of '%s' is %s\n", strFile.c_str(), RTFsTypeName(enmFsTypeFile)));
        LONG64 i64Size;
        hrc = pMedium->COMGETTER(LogicalSize)(&i64Size);                            H();
#ifdef RT_OS_WINDOWS
        if (   enmFsTypeFile == RTFSTYPE_FAT
            && i64Size >= _4G)
        {
            const char *pszUnit;
            uint64_t u64Print = formatDiskSize((uint64_t)i64Size, &pszUnit);
            i_atVMRuntimeErrorCallbackF(0, "FatPartitionDetected",
                                        N_("The medium '%s' has a logical size of %RU64%s "
                                           "but the file system the medium is located on seems "
                                           "to be FAT(32) which cannot handle files bigger than 4GB.\n"
                                           "We strongly recommend to put all your virtual disk images and "
                                           "the snapshot folder onto an NTFS partition"),
                                        strFile.c_str(), u64Print, pszUnit);
        }
#else /* !RT_OS_WINDOWS */
        if (   enmFsTypeFile == RTFSTYPE_FAT
            || enmFsTypeFile == RTFSTYPE_EXT
            || enmFsTypeFile == RTFSTYPE_EXT2
            || enmFsTypeFile == RTFSTYPE_EXT3
            || enmFsTypeFile == RTFSTYPE_EXT4)
        {
            RTFILE file;
            int vrc = RTFileOpen(&file, strFile.c_str(), RTFILE_O_READ | RTFILE_O_OPEN | RTFILE_O_DENY_NONE);
            if (RT_SUCCESS(vrc))
            {
                RTFOFF maxSize;
                /* Careful: This function will work only on selected local file systems! */
                vrc = RTFileQueryMaxSizeEx(file, &maxSize);
                RTFileClose(file);
                if (   RT_SUCCESS(vrc)
                    && maxSize > 0
                    && i64Size > (LONG64)maxSize)
                {
                    const char *pszUnitSiz;
                    const char *pszUnitMax;
                    uint64_t u64PrintSiz = formatDiskSize((LONG64)i64Size, &pszUnitSiz);
                    uint64_t u64PrintMax = formatDiskSize(maxSize, &pszUnitMax);
                    i_atVMRuntimeErrorCallbackF(0, "FatPartitionDetected", /* <= not exact but ... */
                                                N_("The medium '%s' has a logical size of %RU64%s "
                                                   "but the file system the medium is located on can "
                                                   "only handle files up to %RU64%s in theory.\n"
                                                   "We strongly recommend to put all your virtual disk "
                                                   "images and the snapshot folder onto a proper "
                                                   "file system (e.g. ext3) with a sufficient size"),
                                                strFile.c_str(), u64PrintSiz, pszUnitSiz, u64PrintMax, pszUnitMax);
                }
            }
        }
#endif /* !RT_OS_WINDOWS */

        /*
         * Snapshot folder:
         * Here we test only for a FAT partition as we had to create a dummy file otherwise
         */
        if (   enmFsTypeSnap == RTFSTYPE_FAT
            && i64Size >= _4G
            && !mfSnapshotFolderSizeWarningShown)
        {
            const char *pszUnit;
            uint64_t u64Print = formatDiskSize(i64Size, &pszUnit);
            i_atVMRuntimeErrorCallbackF(0, "FatPartitionDetected",
#ifdef RT_OS_WINDOWS
                                        N_("The snapshot folder of this VM '%s' seems to be located on "
                                           "a FAT(32) file system. The logical size of the medium '%s' "
                                           "(%RU64%s) is bigger than the maximum file size this file "
                                           "system can handle (4GB).\n"
                                           "We strongly recommend to put all your virtual disk images and "
                                           "the snapshot folder onto an NTFS partition"),
#else
                                        N_("The snapshot folder of this VM '%s' seems to be located on "
                                            "a FAT(32) file system. The logical size of the medium '%s' "
                                            "(%RU64%s) is bigger than the maximum file size this file "
                                            "system can handle (4GB).\n"
                                            "We strongly recommend to put all your virtual disk images and "
                                            "the snapshot folder onto a proper file system (e.g. ext3)"),
#endif
                                        strSnap.c_str(), strFile.c_str(), u64Print, pszUnit);
            /* Show this particular warning only once */
            mfSnapshotFolderSizeWarningShown = true;
        }

#ifdef RT_OS_LINUX
        /*
         * Ext4 bug: Check if the host I/O cache is disabled and the disk image is located
         *           on an ext4 partition.
         * This bug apparently applies to the XFS file system as well.
         * Linux 2.6.36 is known to be fixed (tested with 2.6.36-rc4).
         */

        char szOsRelease[128];
        int vrc = RTSystemQueryOSInfo(RTSYSOSINFO_RELEASE, szOsRelease, sizeof(szOsRelease));
        bool fKernelHasODirectBug =    RT_FAILURE(vrc)
                                    || (RTStrVersionCompare(szOsRelease, "2.6.36-rc4") < 0);

        if (   (uCaps & MediumFormatCapabilities_Asynchronous)
            && !*pfUseHostIOCache
            && fKernelHasODirectBug)
        {
            if (   enmFsTypeFile == RTFSTYPE_EXT4
                || enmFsTypeFile == RTFSTYPE_XFS)
            {
                i_atVMRuntimeErrorCallbackF(0, "Ext4PartitionDetected",
                                            N_("The host I/O cache for at least one controller is disabled "
                                               "and the medium '%s' for this VM "
                                               "is located on an %s partition. There is a known Linux "
                                               "kernel bug which can lead to the corruption of the virtual "
                                               "disk image under these conditions.\n"
                                               "Either enable the host I/O cache permanently in the VM "
                                               "settings or put the disk image and the snapshot folder "
                                               "onto a different file system.\n"
                                               "The host I/O cache will now be enabled for this medium"),
                                            strFile.c_str(), enmFsTypeFile == RTFSTYPE_EXT4 ? "ext4" : "xfs");
                *pfUseHostIOCache = true;
            }
            else if (  (   enmFsTypeSnap == RTFSTYPE_EXT4
                        || enmFsTypeSnap == RTFSTYPE_XFS)
                     && !mfSnapshotFolderExt4WarningShown)
            {
                i_atVMRuntimeErrorCallbackF(0, "Ext4PartitionDetected",
                                            N_("The host I/O cache for at least one controller is disabled "
                                               "and the snapshot folder for this VM "
                                               "is located on an %s partition. There is a known Linux "
                                               "kernel bug which can lead to the corruption of the virtual "
                                               "disk image under these conditions.\n"
                                               "Either enable the host I/O cache permanently in the VM "
                                               "settings or put the disk image and the snapshot folder "
                                               "onto a different file system.\n"
                                               "The host I/O cache will now be enabled for this medium"),
                                            enmFsTypeSnap == RTFSTYPE_EXT4 ? "ext4" : "xfs");
                *pfUseHostIOCache = true;
                mfSnapshotFolderExt4WarningShown = true;
            }
        }

        /*
         * 2.6.18 bug: Check if the host I/O cache is disabled and the host is running
         *             Linux 2.6.18. See @bugref{8690}. Apparently the same problem as
         *             documented in https://lkml.org/lkml/2007/2/1/14. We saw such
         *             kernel oopses on Linux 2.6.18-416.el5. We don't know when this
         *             was fixed but we _know_ that 2.6.18 EL5 kernels are affected.
         */
        bool fKernelAsyncUnreliable =    RT_FAILURE(vrc)
                                      || (RTStrVersionCompare(szOsRelease, "2.6.19") < 0);
        if (   (uCaps & MediumFormatCapabilities_Asynchronous)
            && !*pfUseHostIOCache
            && fKernelAsyncUnreliable)
        {
            i_atVMRuntimeErrorCallbackF(0, "Linux2618TooOld",
                                        N_("The host I/O cache for at least one controller is disabled. "
                                           "There is a known Linux kernel bug which can lead to kernel "
                                           "oopses under heavy load. To our knowledge this bug affects "
                                           "all 2.6.18 kernels.\n"
                                           "Either enable the host I/O cache permanently in the VM "
                                           "settings or switch to a newer host kernel.\n"
                                           "The host I/O cache will now be enabled for this medium"));
            *pfUseHostIOCache = true;
        }
#endif
    }
#undef H

    return VINF_SUCCESS;
}

/**
 * Unmounts the specified medium from the specified device.
 *
 * @returns VBox status code.
 * @param   pUVM            The usermode VM handle.
 * @param   pVMM            The VMM vtable.
 * @param   enmBus          The storage bus.
 * @param   enmDevType      The device type.
 * @param   pcszDevice      The device emulation.
 * @param   uInstance       Instance of the device.
 * @param   uLUN            The LUN on the device.
 * @param   fForceUnmount   Whether to force unmounting.
 */
int Console::i_unmountMediumFromGuest(PUVM pUVM, PCVMMR3VTABLE pVMM, StorageBus_T enmBus, DeviceType_T enmDevType,
                                      const char *pcszDevice, unsigned uInstance, unsigned uLUN,
                                      bool fForceUnmount) RT_NOEXCEPT
{
    /* Unmount existing media only for floppy and DVD drives. */
    int vrc = VINF_SUCCESS;
    PPDMIBASE pBase;
    if (enmBus == StorageBus_USB)
        vrc = pVMM->pfnPDMR3UsbQueryDriverOnLun(pUVM, pcszDevice, uInstance, uLUN, "SCSI", &pBase);
    else if (   (enmBus == StorageBus_SAS || enmBus == StorageBus_SCSI || enmBus == StorageBus_VirtioSCSI)
             || (enmBus == StorageBus_SATA && enmDevType == DeviceType_DVD))
        vrc = pVMM->pfnPDMR3QueryDriverOnLun(pUVM, pcszDevice, uInstance, uLUN, "SCSI", &pBase);
    else /* IDE or Floppy */
        vrc = pVMM->pfnPDMR3QueryLun(pUVM, pcszDevice, uInstance, uLUN, &pBase);

    if (RT_FAILURE(vrc))
    {
        if (vrc == VERR_PDM_LUN_NOT_FOUND || vrc == VERR_PDM_NO_DRIVER_ATTACHED_TO_LUN)
            vrc = VINF_SUCCESS;
        AssertRC(vrc);
    }
    else
    {
        PPDMIMOUNT pIMount = PDMIBASE_QUERY_INTERFACE(pBase, PDMIMOUNT);
        AssertReturn(pIMount, VERR_INVALID_POINTER);

        /* Unmount the media (but do not eject the medium!) */
        vrc = pIMount->pfnUnmount(pIMount, fForceUnmount, false /*=fEject*/);
        if (vrc == VERR_PDM_MEDIA_NOT_MOUNTED)
            vrc = VINF_SUCCESS;
        /* for example if the medium is locked */
        else if (RT_FAILURE(vrc))
            return vrc;
    }

    return vrc;
}

/**
 * Removes the currently attached medium driver form the specified device
 * taking care of the controlelr specific configs wrt. to the attached driver chain.
 *
 * @returns VBox status code.
 * @param   pCtlInst        The controler instance node in the CFGM tree.
 * @param   pcszDevice      The device name.
 * @param   uInstance       The device instance.
 * @param   uLUN            The device LUN.
 * @param   enmBus          The storage bus.
 * @param   fAttachDetach   Flag whether this is a change while the VM is running
 * @param   fHotplug        Flag whether the guest should be notified about the device change.
 * @param   fForceUnmount   Flag whether to force unmounting the medium even if it is locked.
 * @param   pUVM            The usermode VM handle.
 * @param   pVMM            The VMM vtable.
 * @param   enmDevType      The device type.
 * @param   ppLunL0         Where to store the node to attach the new config to on success.
 */
int Console::i_removeMediumDriverFromVm(PCFGMNODE pCtlInst,
                                        const char *pcszDevice,
                                        unsigned uInstance,
                                        unsigned uLUN,
                                        StorageBus_T enmBus,
                                        bool fAttachDetach,
                                        bool fHotplug,
                                        bool fForceUnmount,
                                        PUVM pUVM,
                                        PCVMMR3VTABLE pVMM,
                                        DeviceType_T enmDevType,
                                        PCFGMNODE *ppLunL0)
{
    int vrc = VINF_SUCCESS;
    bool fAddLun = false;

    /* First check if the LUN already exists. */
    PCFGMNODE pLunL0 = pVMM->pfnCFGMR3GetChildF(pCtlInst, "LUN#%u", uLUN);
    AssertReturn(!RT_VALID_PTR(pLunL0) || fAttachDetach, VERR_INTERNAL_ERROR);

    if (pLunL0)
    {
        /*
         * Unmount the currently mounted medium if we don't just hot remove the
         * complete device (SATA) and it supports unmounting (DVD).
         */
        if (   (enmDevType != DeviceType_HardDisk)
            && !fHotplug)
        {
            vrc = i_unmountMediumFromGuest(pUVM, pVMM, enmBus, enmDevType, pcszDevice, uInstance, uLUN, fForceUnmount);
            if (RT_FAILURE(vrc))
                return vrc;
        }

        /*
         * Don't detach the SCSI driver when unmounting the current medium
         * (we are not ripping out the device but only eject the medium).
         */
        char *pszDriverDetach = NULL;
        if (   !fHotplug
            && (   (enmBus == StorageBus_SATA && enmDevType == DeviceType_DVD)
                || enmBus == StorageBus_SAS
                || enmBus == StorageBus_SCSI
                || enmBus == StorageBus_VirtioSCSI
                || enmBus == StorageBus_USB))
        {
            /* Get the current attached driver we have to detach. */
            PCFGMNODE pDrvLun = pVMM->pfnCFGMR3GetChildF(pCtlInst, "LUN#%u/AttachedDriver/", uLUN);
            if (pDrvLun)
            {
                char szDriver[128];
                RT_ZERO(szDriver);
                vrc  = pVMM->pfnCFGMR3QueryString(pDrvLun, "Driver", &szDriver[0], sizeof(szDriver));
                if (RT_SUCCESS(vrc))
                    pszDriverDetach = RTStrDup(&szDriver[0]);

                pLunL0 = pDrvLun;
            }
        }

        if (enmBus == StorageBus_USB)
            vrc = pVMM->pfnPDMR3UsbDriverDetach(pUVM, pcszDevice, uInstance, uLUN, pszDriverDetach,
                                                0 /* iOccurence */, fHotplug ? 0 : PDM_TACH_FLAGS_NOT_HOT_PLUG);
        else
            vrc = pVMM->pfnPDMR3DriverDetach(pUVM, pcszDevice, uInstance, uLUN, pszDriverDetach,
                                             0 /* iOccurence */, fHotplug ? 0 : PDM_TACH_FLAGS_NOT_HOT_PLUG);

        if (pszDriverDetach)
        {
            RTStrFree(pszDriverDetach);
            /* Remove the complete node and create new for the new config. */
            pVMM->pfnCFGMR3RemoveNode(pLunL0);
            pLunL0 = pVMM->pfnCFGMR3GetChildF(pCtlInst, "LUN#%u", uLUN);
            if (pLunL0)
            {
                try
                {
                    InsertConfigNode(pLunL0, "AttachedDriver", &pLunL0);
                }
                catch (ConfigError &x)
                {
                    // InsertConfig threw something:
                    return x.m_vrc;
                }
            }
        }
        if (vrc == VERR_PDM_NO_DRIVER_ATTACHED_TO_LUN)
            vrc = VINF_SUCCESS;
        AssertRCReturn(vrc, vrc);

        /*
         * Don't remove the LUN except for IDE/floppy/NVMe (which connects directly to the medium driver
         * even for DVD devices) or if there is a hotplug event which rips out the complete device.
         */
        if (   fHotplug
            || enmBus == StorageBus_IDE
            || enmBus == StorageBus_Floppy
            || enmBus == StorageBus_PCIe
            || (enmBus == StorageBus_SATA && enmDevType != DeviceType_DVD))
        {
            fAddLun = true;
            pVMM->pfnCFGMR3RemoveNode(pLunL0);
        }
    }
    else
        fAddLun = true;

    try
    {
        if (fAddLun)
            InsertConfigNodeF(pCtlInst, &pLunL0, "LUN#%u", uLUN);
    }
    catch (ConfigError &x)
    {
        // InsertConfig threw something:
        return x.m_vrc;
    }

    if (ppLunL0)
        *ppLunL0 = pLunL0;

    return vrc;
}

int Console::i_configMediumAttachment(const char *pcszDevice,
                                      unsigned uInstance,
                                      StorageBus_T enmBus,
                                      bool fUseHostIOCache,
                                      bool fBuiltinIOCache,
                                      bool fInsertDiskIntegrityDrv,
                                      bool fSetupMerge,
                                      unsigned uMergeSource,
                                      unsigned uMergeTarget,
                                      IMediumAttachment *pMediumAtt,
                                      MachineState_T aMachineState,
                                      HRESULT *phrc,
                                      bool fAttachDetach,
                                      bool fForceUnmount,
                                      bool fHotplug,
                                      PUVM pUVM,
                                      PCVMMR3VTABLE pVMM,
                                      DeviceType_T *paLedDevType,
                                      PCFGMNODE *ppLunL0)
{
    // InsertConfig* throws
    try
    {
        int vrc = VINF_SUCCESS;
        HRESULT hrc;
        Bstr    bstr;
        PCFGMNODE pCtlInst = NULL;

// #define RC_CHECK()  AssertMsgReturn(RT_SUCCESS(vrc), ("vrc=%Rrc\n", vrc), vrc)
#define H()         AssertLogRelMsgReturn(!FAILED(hrc), ("hrc=%Rhrc\n", hrc), VERR_MAIN_CONFIG_CONSTRUCTOR_COM_ERROR)

        LONG lDev;
        hrc = pMediumAtt->COMGETTER(Device)(&lDev);                                         H();
        LONG lPort;
        hrc = pMediumAtt->COMGETTER(Port)(&lPort);                                          H();
        DeviceType_T enmType;
        hrc = pMediumAtt->COMGETTER(Type)(&enmType);                                        H();
        BOOL fNonRotational;
        hrc = pMediumAtt->COMGETTER(NonRotational)(&fNonRotational);                        H();
        BOOL fDiscard;
        hrc = pMediumAtt->COMGETTER(Discard)(&fDiscard);                                    H();

        if (enmType == DeviceType_DVD)
            fInsertDiskIntegrityDrv = false;

        unsigned uLUN;
        PCFGMNODE pLunL0 = NULL;
        hrc = Console::i_storageBusPortDeviceToLun(enmBus, lPort, lDev, uLUN);                H();

        /* Determine the base path for the device instance. */
        if (enmBus != StorageBus_USB)
            pCtlInst = pVMM->pfnCFGMR3GetChildF(pVMM->pfnCFGMR3GetRootU(pUVM), "Devices/%s/%u/", pcszDevice, uInstance);
        else
        {
            /* If we hotplug a USB device create a new CFGM tree. */
            if (!fHotplug)
                pCtlInst = pVMM->pfnCFGMR3GetChildF(pVMM->pfnCFGMR3GetRootU(pUVM), "USB/%s/", pcszDevice);
            else
                pCtlInst = pVMM->pfnCFGMR3CreateTree(pUVM); /** @todo r=bird: Leaked in error paths! */
        }
        AssertReturn(pCtlInst, VERR_INTERNAL_ERROR);

        if (enmBus == StorageBus_USB)
        {
            PCFGMNODE pCfg = NULL;

            /* Create correct instance. */
            if (!fHotplug)
            {
                if (!fAttachDetach)
                    InsertConfigNodeF(pCtlInst, &pCtlInst, "%d", lPort);
                else
                    pCtlInst = pVMM->pfnCFGMR3GetChildF(pCtlInst, "%d/", lPort);
            }

            if (!fAttachDetach)
                InsertConfigNode(pCtlInst, "Config", &pCfg);

            uInstance = lPort; /* Overwrite uInstance with the correct one. */

            /** @todo No LED after hotplugging. */
            if (!fHotplug && !fAttachDetach)
            {
                USBStorageDevice UsbMsd;
                UsbMsd.iPort = uInstance;
                vrc = RTUuidCreate(&UsbMsd.mUuid);
                AssertRCReturn(vrc, vrc);

                InsertConfigStringF(pCtlInst, "UUID", "%RTuuid", &UsbMsd.mUuid);

                mUSBStorageDevices.push_back(UsbMsd);

                /** @todo This LED set is not freed if the device is unplugged.  We could
                 *        keep the LED set index in the UsbMsd structure and clean it up in
                 *        i_detachStorageDevice. */
                /* Attach the status driver */
                i_attachStatusDriver(pCtlInst, RT_BIT_32(DeviceType_HardDisk),
                                     8, &paLedDevType, &mapMediumAttachments, pcszDevice, 0);
            }
        }

        vrc = i_removeMediumDriverFromVm(pCtlInst, pcszDevice, uInstance, uLUN, enmBus, fAttachDetach,
                                         fHotplug, fForceUnmount, pUVM, pVMM, enmType, &pLunL0);
        if (RT_FAILURE(vrc))
            return vrc;
        if (ppLunL0)
            *ppLunL0 = pLunL0;

        Utf8StrFmt devicePath("%s/%u/LUN#%u", pcszDevice, uInstance, uLUN);
        mapMediumAttachments[devicePath] = pMediumAtt;

        ComPtr<IMedium> ptrMedium;
        hrc = pMediumAtt->COMGETTER(Medium)(ptrMedium.asOutParam());                        H();

        /*
         * 1. Only check this for hard disk images.
         * 2. Only check during VM creation and not later, especially not during
         *    taking an online snapshot!
         */
        if (   enmType == DeviceType_HardDisk
            && (   aMachineState == MachineState_Starting
                || aMachineState == MachineState_Restoring))
        {
            vrc = i_checkMediumLocation(ptrMedium, &fUseHostIOCache);
            if (RT_FAILURE(vrc))
                return vrc;
        }

        BOOL fPassthrough = FALSE;
        if (ptrMedium.isNotNull())
        {
            BOOL fHostDrive;
            hrc = ptrMedium->COMGETTER(HostDrive)(&fHostDrive);                             H();
            if (  (   enmType == DeviceType_DVD
                   || enmType == DeviceType_Floppy)
                && !fHostDrive)
            {
                /*
                 * Informative logging.
                 */
                Bstr bstrFile;
                hrc = ptrMedium->COMGETTER(Location)(bstrFile.asOutParam());                H();
                Utf8Str strFile(bstrFile);
                RTFSTYPE enmFsTypeFile = RTFSTYPE_UNKNOWN;
                (void)RTFsQueryType(strFile.c_str(), &enmFsTypeFile);
                LogRel(("File system of '%s' (%s) is %s\n",
                       strFile.c_str(), enmType == DeviceType_DVD ? "DVD" : "Floppy", RTFsTypeName(enmFsTypeFile)));
            }

            if (fHostDrive)
            {
                hrc = pMediumAtt->COMGETTER(Passthrough)(&fPassthrough);                    H();
            }
        }

        ComObjPtr<IBandwidthGroup> pBwGroup;
        Bstr bstrBwGroup;
        hrc = pMediumAtt->COMGETTER(BandwidthGroup)(pBwGroup.asOutParam());                 H();

        if (!pBwGroup.isNull())
        {
            hrc = pBwGroup->COMGETTER(Name)(bstrBwGroup.asOutParam());                      H();
        }

        /*
         * Insert the SCSI driver for hotplug events on the SCSI/USB based storage controllers
         * or for SATA if the new device is a CD/DVD drive.
         */
        if (   (fHotplug || !fAttachDetach)
            && (   enmBus == StorageBus_SCSI
                || enmBus == StorageBus_SAS
                || enmBus == StorageBus_USB
                || enmBus == StorageBus_VirtioSCSI
                || (enmBus == StorageBus_SATA && enmType == DeviceType_DVD && !fPassthrough)))
        {
            InsertConfigString(pLunL0, "Driver", "SCSI");
            InsertConfigNode(pLunL0, "AttachedDriver", &pLunL0);
        }

        vrc = i_configMedium(pLunL0,
                             !!fPassthrough,
                             enmType,
                             fUseHostIOCache,
                             fBuiltinIOCache,
                             fInsertDiskIntegrityDrv,
                             fSetupMerge,
                             uMergeSource,
                             uMergeTarget,
                             bstrBwGroup.isEmpty() ? NULL : Utf8Str(bstrBwGroup).c_str(),
                             !!fDiscard,
                             !!fNonRotational,
                             ptrMedium,
                             aMachineState,
                             phrc);
        if (RT_FAILURE(vrc))
            return vrc;

        if (fAttachDetach)
        {
            /* Attach the new driver. */
            if (enmBus == StorageBus_USB)
            {
                if (fHotplug)
                {
                    USBStorageDevice UsbMsd;
                    RTUuidCreate(&UsbMsd.mUuid);
                    UsbMsd.iPort = uInstance;
                    vrc = pVMM->pfnPDMR3UsbCreateEmulatedDevice(pUVM, pcszDevice, pCtlInst, &UsbMsd.mUuid, NULL);
                    if (RT_SUCCESS(vrc))
                        mUSBStorageDevices.push_back(UsbMsd);
                }
                else
                    vrc = pVMM->pfnPDMR3UsbDriverAttach(pUVM, pcszDevice, uInstance, uLUN,
                                                        fHotplug ? 0 : PDM_TACH_FLAGS_NOT_HOT_PLUG, NULL /*ppBase*/);
            }
            else if (   !fHotplug
                     && (   (enmBus == StorageBus_SAS || enmBus == StorageBus_SCSI || enmBus == StorageBus_VirtioSCSI)
                         || (enmBus == StorageBus_SATA && enmType == DeviceType_DVD)))
                vrc = pVMM->pfnPDMR3DriverAttach(pUVM, pcszDevice, uInstance, uLUN,
                                                 fHotplug ? 0 : PDM_TACH_FLAGS_NOT_HOT_PLUG, NULL /*ppBase*/);
            else
                vrc = pVMM->pfnPDMR3DeviceAttach(pUVM, pcszDevice, uInstance, uLUN,
                                                 fHotplug ? 0 : PDM_TACH_FLAGS_NOT_HOT_PLUG, NULL /*ppBase*/);
            AssertRCReturn(vrc, vrc);

            /*
             * Make the secret key helper interface known to the VD driver if it is attached,
             * so we can get notified about missing keys.
             */
            PPDMIBASE pIBase = NULL;
            vrc = pVMM->pfnPDMR3QueryDriverOnLun(pUVM, pcszDevice, uInstance, uLUN, "VD", &pIBase);
            if (RT_SUCCESS(vrc) && pIBase)
            {
                PPDMIMEDIA pIMedium = (PPDMIMEDIA)pIBase->pfnQueryInterface(pIBase, PDMIMEDIA_IID);
                if (pIMedium)
                {
                    vrc = pIMedium->pfnSetSecKeyIf(pIMedium, mpIfSecKey, mpIfSecKeyHlp);
                    Assert(RT_SUCCESS(vrc) || vrc == VERR_NOT_SUPPORTED);
                }
            }

            /* There is no need to handle removable medium mounting, as we
             * unconditionally replace everthing including the block driver level.
             * This means the new medium will be picked up automatically. */
        }

        if (paLedDevType)
            i_setLedType(&paLedDevType[uLUN], enmType);

        /* Dump the changed LUN if possible, dump the complete device otherwise */
        if (   aMachineState != MachineState_Starting
            && aMachineState != MachineState_Restoring)
            pVMM->pfnCFGMR3Dump(pLunL0 ? pLunL0 : pCtlInst);
    }
    catch (ConfigError &x)
    {
        // InsertConfig threw something:
        return x.m_vrc;
    }

#undef H

    return VINF_SUCCESS;
}

int Console::i_configMedium(PCFGMNODE pLunL0,
                            bool fPassthrough,
                            DeviceType_T enmType,
                            bool fUseHostIOCache,
                            bool fBuiltinIOCache,
                            bool fInsertDiskIntegrityDrv,
                            bool fSetupMerge,
                            unsigned uMergeSource,
                            unsigned uMergeTarget,
                            const char *pcszBwGroup,
                            bool fDiscard,
                            bool fNonRotational,
                            ComPtr<IMedium> ptrMedium,
                            MachineState_T aMachineState,
                            HRESULT *phrc)
{
    // InsertConfig* throws
    try
    {
        HRESULT hrc;
        Bstr bstr;
        PCFGMNODE pCfg = NULL;

#define H() \
    AssertMsgReturnStmt(SUCCEEDED(hrc), ("hrc=%Rhrc\n", hrc), if (phrc) *phrc = hrc, Global::vboxStatusCodeFromCOM(hrc))


        BOOL fHostDrive = FALSE;
        MediumType_T mediumType = MediumType_Normal;
        if (ptrMedium.isNotNull())
        {
            hrc = ptrMedium->COMGETTER(HostDrive)(&fHostDrive);                             H();
            hrc = ptrMedium->COMGETTER(Type)(&mediumType);                                  H();
        }

        if (fHostDrive)
        {
            Assert(ptrMedium.isNotNull());
            if (enmType == DeviceType_DVD)
            {
                InsertConfigString(pLunL0, "Driver", "HostDVD");
                InsertConfigNode(pLunL0, "Config", &pCfg);

                hrc = ptrMedium->COMGETTER(Location)(bstr.asOutParam());                    H();
                InsertConfigString(pCfg, "Path", bstr);

                InsertConfigInteger(pCfg, "Passthrough", fPassthrough);
            }
            else if (enmType == DeviceType_Floppy)
            {
                InsertConfigString(pLunL0, "Driver", "HostFloppy");
                InsertConfigNode(pLunL0, "Config", &pCfg);

                hrc = ptrMedium->COMGETTER(Location)(bstr.asOutParam());                    H();
                InsertConfigString(pCfg, "Path", bstr);
            }
        }
        else
        {
            if (fInsertDiskIntegrityDrv)
            {
                /*
                 * The actual configuration is done through CFGM extra data
                 * for each inserted driver separately.
                 */
                InsertConfigString(pLunL0, "Driver", "DiskIntegrity");
                InsertConfigNode(pLunL0, "Config", &pCfg);
                InsertConfigNode(pLunL0, "AttachedDriver", &pLunL0);
            }

            InsertConfigString(pLunL0, "Driver", "VD");
            InsertConfigNode(pLunL0, "Config", &pCfg);
            switch (enmType)
            {
                case DeviceType_DVD:
                    InsertConfigString(pCfg, "Type", "DVD");
                    InsertConfigInteger(pCfg, "Mountable", 1);
                    break;
                case DeviceType_Floppy:
                    InsertConfigString(pCfg, "Type", "Floppy 1.44");
                    InsertConfigInteger(pCfg, "Mountable", 1);
                    break;
                case DeviceType_HardDisk:
                default:
                    InsertConfigString(pCfg, "Type", "HardDisk");
                    InsertConfigInteger(pCfg, "Mountable", 0);
            }

            if (    ptrMedium.isNotNull()
                && (   enmType == DeviceType_DVD
                    || enmType == DeviceType_Floppy)
               )
            {
                // if this medium represents an ISO image and this image is inaccessible,
                // the ignore it instead of causing a failure; this can happen when we
                // restore a VM state and the ISO has disappeared, e.g. because the Guest
                // Additions were mounted and the user upgraded VirtualBox. Previously
                // we failed on startup, but that's not good because the only way out then
                // would be to discard the VM state...
                MediumState_T mediumState;
                hrc = ptrMedium->RefreshState(&mediumState);                                H();
                if (mediumState == MediumState_Inaccessible)
                {
                    Bstr loc;
                    hrc = ptrMedium->COMGETTER(Location)(loc.asOutParam());                 H();
                    i_atVMRuntimeErrorCallbackF(0, "DvdOrFloppyImageInaccessible",
                                                N_("The image file '%ls' is inaccessible and is being ignored. "
                                                   "Please select a different image file for the virtual %s drive."),
                                                loc.raw(),
                                                enmType == DeviceType_DVD ? "DVD" : "floppy");
                    ptrMedium.setNull();
                }
            }

            if (ptrMedium.isNotNull())
            {
                /* Start with length of parent chain, as the list is reversed */
                unsigned uImage = 0;
                ComPtr<IMedium> ptrTmp = ptrMedium;
                while (ptrTmp.isNotNull())
                {
                    uImage++;
                    ComPtr<IMedium> ptrParent;
                    hrc = ptrTmp->COMGETTER(Parent)(ptrParent.asOutParam());               H();
                    ptrTmp = ptrParent;
                }
                /* Index of last image */
                uImage--;

# ifdef VBOX_WITH_EXTPACK
                if (mptrExtPackManager->i_isExtPackUsable(ORACLE_PUEL_EXTPACK_NAME))
                {
                    /* Configure loading the VDPlugin. */
                    static const char s_szVDPlugin[] = "VDPluginCrypt";
                    PCFGMNODE pCfgPlugins = NULL;
                    PCFGMNODE pCfgPlugin = NULL;
                    Utf8Str strPlugin;
                    hrc = mptrExtPackManager->i_getLibraryPathForExtPack(s_szVDPlugin, ORACLE_PUEL_EXTPACK_NAME, &strPlugin);
                    // Don't fail, this is optional!
                    if (SUCCEEDED(hrc))
                    {
                        InsertConfigNode(pCfg, "Plugins", &pCfgPlugins);
                        InsertConfigNode(pCfgPlugins, s_szVDPlugin, &pCfgPlugin);
                        InsertConfigString(pCfgPlugin, "Path", strPlugin);
                    }
                }
# endif

                hrc = ptrMedium->COMGETTER(Location)(bstr.asOutParam());                    H();
                InsertConfigString(pCfg, "Path", bstr);

                hrc = ptrMedium->COMGETTER(Format)(bstr.asOutParam());                      H();
                InsertConfigString(pCfg, "Format", bstr);

                if (mediumType == MediumType_Readonly)
                    InsertConfigInteger(pCfg, "ReadOnly", 1);
                else if (enmType == DeviceType_Floppy)
                    InsertConfigInteger(pCfg, "MaybeReadOnly", 1);

                /* Start without exclusive write access to the images. */
                /** @todo Live Migration: I don't quite like this, we risk screwing up when
                 *        we're resuming the VM if some 3rd dude have any of the VDIs open
                 *        with write sharing denied.  However, if the two VMs are sharing a
                 *        image it really is necessary....
                 *
                 *        So, on the "lock-media" command, the target teleporter should also
                 *        make DrvVD undo TempReadOnly.  It gets interesting if we fail after
                 *        that. Grumble. */
                if (   enmType == DeviceType_HardDisk
                    && aMachineState == MachineState_TeleportingIn)
                    InsertConfigInteger(pCfg, "TempReadOnly", 1);

                /* Flag for opening the medium for sharing between VMs. This
                 * is done at the moment only for the first (and only) medium
                 * in the chain, as shared media can have no diffs. */
                if (mediumType == MediumType_Shareable)
                    InsertConfigInteger(pCfg, "Shareable", 1);

                if (!fUseHostIOCache)
                {
                    InsertConfigInteger(pCfg, "UseNewIo", 1);
                    /*
                     * Activate the builtin I/O cache for harddisks only.
                     * It caches writes only which doesn't make sense for DVD drives
                     * and just increases the overhead.
                     */
                    if (   fBuiltinIOCache
                        && (enmType == DeviceType_HardDisk))
                        InsertConfigInteger(pCfg, "BlockCache", 1);
                }

                if (fSetupMerge)
                {
                    InsertConfigInteger(pCfg, "SetupMerge", 1);
                    if (uImage == uMergeSource)
                        InsertConfigInteger(pCfg, "MergeSource", 1);
                    else if (uImage == uMergeTarget)
                        InsertConfigInteger(pCfg, "MergeTarget", 1);
                }

                if (pcszBwGroup)
                    InsertConfigString(pCfg, "BwGroup", pcszBwGroup);

                if (fDiscard)
                    InsertConfigInteger(pCfg, "Discard", 1);

                if (fNonRotational)
                    InsertConfigInteger(pCfg, "NonRotationalMedium", 1);

                /* Pass all custom parameters. */
                bool fHostIP = true;
                bool fEncrypted = false;
                hrc = i_configMediumProperties(pCfg, ptrMedium, &fHostIP, &fEncrypted); H();

                /* Create an inverted list of parents. */
                uImage--;
                ComPtr<IMedium> ptrParentMedium = ptrMedium;
                for (PCFGMNODE pParent = pCfg;; uImage--)
                {
                    ComPtr<IMedium> ptrCurMedium;
                    hrc = ptrParentMedium->COMGETTER(Parent)(ptrCurMedium.asOutParam());    H();
                    if (ptrCurMedium.isNull())
                        break;

                    PCFGMNODE pCur;
                    InsertConfigNode(pParent, "Parent", &pCur);
                    hrc = ptrCurMedium->COMGETTER(Location)(bstr.asOutParam()); H();
                    InsertConfigString(pCur, "Path", bstr);

                    hrc = ptrCurMedium->COMGETTER(Format)(bstr.asOutParam());  H();
                    InsertConfigString(pCur, "Format", bstr);

                    if (fSetupMerge)
                    {
                        if (uImage == uMergeSource)
                            InsertConfigInteger(pCur, "MergeSource", 1);
                        else if (uImage == uMergeTarget)
                            InsertConfigInteger(pCur, "MergeTarget", 1);
                    }

                    /* Configure medium properties. */
                    hrc = i_configMediumProperties(pCur, ptrCurMedium, &fHostIP, &fEncrypted); H();

                    /* next */
                    pParent = pCur;
                    ptrParentMedium = ptrCurMedium;
                }

                /* Custom code: put marker to not use host IP stack to driver
                 * configuration node. Simplifies life of DrvVD a bit. */
                if (!fHostIP)
                    InsertConfigInteger(pCfg, "HostIPStack", 0);

                if (fEncrypted)
                    m_cDisksEncrypted++;
            }
            else
            {
                /* Set empty drive flag for DVD or floppy without media. */
                if (   enmType == DeviceType_DVD
                    || enmType == DeviceType_Floppy)
                    InsertConfigInteger(pCfg, "EmptyDrive", 1);
            }
        }
#undef H
    }
    catch (ConfigError &x)
    {
        // InsertConfig threw something:
        return x.m_vrc;
    }

    return VINF_SUCCESS;
}

/**
 * Adds the medium properties to the CFGM tree.
 *
 * @returns VBox status code.
 * @param   pCur        The current CFGM node.
 * @param   pMedium     The medium object to configure.
 * @param   pfHostIP    Where to return the value of the \"HostIPStack\" property if found.
 * @param   pfEncrypted Where to return whether the medium is encrypted.
 */
int Console::i_configMediumProperties(PCFGMNODE pCur, IMedium *pMedium, bool *pfHostIP, bool *pfEncrypted)
{
    /* Pass all custom parameters. */
    SafeArray<BSTR> aNames;
    SafeArray<BSTR> aValues;
    HRESULT hrc = pMedium->GetProperties(NULL, ComSafeArrayAsOutParam(aNames), ComSafeArrayAsOutParam(aValues));
    if (   SUCCEEDED(hrc)
        && aNames.size() != 0)
    {
        PCFGMNODE pVDC;
        InsertConfigNode(pCur, "VDConfig", &pVDC);
        for (size_t ii = 0; ii < aNames.size(); ++ii)
        {
            if (aValues[ii] && *aValues[ii])
            {
                Utf8Str const strName  = aNames[ii];
                Utf8Str const strValue = aValues[ii];
                size_t offSlash = strName.find("/", 0);
                if (   offSlash != strName.npos
                    && !strName.startsWith("Special/"))
                {
                    com::Utf8Str strFilter;
                    hrc = strFilter.assignEx(strName, 0, offSlash);
                    if (FAILED(hrc))
                        break;

                    com::Utf8Str strKey;
                    hrc = strKey.assignEx(strName, offSlash + 1, strName.length() - offSlash - 1); /* Skip slash */
                    if (FAILED(hrc))
                        break;

                    PCFGMNODE pCfgFilterConfig = mpVMM->pfnCFGMR3GetChild(pVDC, strFilter.c_str());
                    if (!pCfgFilterConfig)
                        InsertConfigNode(pVDC, strFilter.c_str(), &pCfgFilterConfig);

                    InsertConfigString(pCfgFilterConfig, strKey.c_str(), strValue);
                }
                else
                {
                    InsertConfigString(pVDC, strName.c_str(), strValue);
                    if (    strName.compare("HostIPStack") == 0
                        &&  strValue.compare("0") == 0)
                        *pfHostIP = false;
                }

                if (   strName.compare("CRYPT/KeyId") == 0
                    && pfEncrypted)
                    *pfEncrypted = true;
            }
        }
    }

    return hrc;
}


/**
 * Configure proxy parameters the Network configuration tree.
 *
 * Parameters may differ depending on the IP address being accessed.
 *
 * @returns VBox status code.
 *
 * @param   virtualBox          The VirtualBox object.
 * @param   pCfg                Configuration node for the driver.
 * @param   pcszPrefix          The prefix for CFGM parameters: "Primary" or "Secondary".
 * @param   strIpAddr           The public IP address to be accessed via a proxy.
 *
 * @thread EMT
 */
int Console::i_configProxy(ComPtr<IVirtualBox> virtualBox, PCFGMNODE pCfg, const char *pcszPrefix, const com::Utf8Str &strIpAddr)
{
/** @todo r=bird: This code doesn't handle cleanup correctly and may leak
 *        when hitting errors or throwing exceptions (bad_alloc). */
    RTHTTPPROXYINFO ProxyInfo;
    ComPtr<ISystemProperties> systemProperties;
    ProxyMode_T enmProxyMode;
    HRESULT hrc = virtualBox->COMGETTER(SystemProperties)(systemProperties.asOutParam());
    if (FAILED(hrc))
    {
        LogRel(("CLOUD-NET: Failed to obtain system properties. hrc=%x\n", hrc));
        return false;
    }
    hrc = systemProperties->COMGETTER(ProxyMode)(&enmProxyMode);
    if (FAILED(hrc))
    {
        LogRel(("CLOUD-NET: Failed to obtain default machine folder. hrc=%x\n", hrc));
        return VERR_INTERNAL_ERROR;
    }

    RTHTTP hHttp;
    int vrc = RTHttpCreate(&hHttp);
    if (RT_FAILURE(vrc))
    {
        LogRel(("CLOUD-NET: Failed to create HTTP context (vrc=%Rrc)\n", vrc));
        return vrc;
    }

    char *pszProxyType = NULL;

    if (enmProxyMode == ProxyMode_Manual)
    {
        /*
         * Unfortunately we cannot simply call RTHttpSetProxyByUrl because it never
         * exposes proxy settings. Calling RTHttpQueryProxyInfoForUrl afterward
         * won't help either as it uses system-wide proxy settings instead of
         * parameters we would have set with RTHttpSetProxyByUrl. Hence we parse
         * proxy URL ourselves here.
         */
        Bstr proxyUrl;
        hrc = systemProperties->COMGETTER(ProxyURL)(proxyUrl.asOutParam());
        if (FAILED(hrc))
        {
            LogRel(("CLOUD-NET: Failed to obtain proxy URL. hrc=%x\n", hrc));
            return false;
        }
        Utf8Str strProxyUrl = proxyUrl;
        if (!strProxyUrl.contains("://"))
            strProxyUrl = "http://" + strProxyUrl;
        const char *pcszProxyUrl = strProxyUrl.c_str();
        RTURIPARSED Parsed;
        vrc = RTUriParse(pcszProxyUrl, &Parsed);
        if (RT_FAILURE(vrc))
        {
            LogRel(("CLOUD-NET: Failed to parse proxy URL: %ls (vrc=%Rrc)\n", proxyUrl.raw(), vrc));
            return false;
        }

        pszProxyType = RTUriParsedScheme(pcszProxyUrl, &Parsed);
        if (!pszProxyType)
        {
            LogRel(("CLOUD-NET: Failed to get proxy scheme from proxy URL: %s\n", pcszProxyUrl));
            return false;
        }
        RTStrToUpper(pszProxyType);

        ProxyInfo.pszProxyHost = RTUriParsedAuthorityHost(pcszProxyUrl, &Parsed);
        if (!ProxyInfo.pszProxyHost)
        {
            LogRel(("CLOUD-NET: Failed to get proxy host name from proxy URL: %s\n", pcszProxyUrl));
            return false;
        }
        ProxyInfo.uProxyPort  = RTUriParsedAuthorityPort(pcszProxyUrl, &Parsed);
        if (ProxyInfo.uProxyPort == UINT32_MAX)
        {
            LogRel(("CLOUD-NET: Failed to get proxy port from proxy URL: %s\n", pcszProxyUrl));
            return false;
        }
        ProxyInfo.pszProxyUsername = RTUriParsedAuthorityUsername(pcszProxyUrl, &Parsed);
        ProxyInfo.pszProxyPassword = RTUriParsedAuthorityPassword(pcszProxyUrl, &Parsed);
    }
    else if (enmProxyMode == ProxyMode_System)
    {
        vrc = RTHttpUseSystemProxySettings(hHttp);
        if (RT_FAILURE(vrc))
        {
            LogRel(("%s: RTHttpUseSystemProxySettings() failed: %Rrc", __FUNCTION__, vrc));
            RTHttpDestroy(hHttp);
            return vrc;
        }
        vrc = RTHttpQueryProxyInfoForUrl(hHttp, ("http://" + strIpAddr).c_str(), &ProxyInfo);
        RTHttpDestroy(hHttp);
        if (RT_FAILURE(vrc))
        {
            LogRel(("CLOUD-NET: Failed to get proxy for %s (vrc=%Rrc)\n", strIpAddr.c_str(), vrc));
            return vrc;
        }

        switch (ProxyInfo.enmProxyType)
        {
            case RTHTTPPROXYTYPE_NOPROXY:
                /* Nothing to do */
                return VINF_SUCCESS;
            case RTHTTPPROXYTYPE_HTTP:
                pszProxyType = RTStrDup("HTTP");
                break;
            case RTHTTPPROXYTYPE_HTTPS:
            case RTHTTPPROXYTYPE_SOCKS4:
            case RTHTTPPROXYTYPE_SOCKS5:
                /* break; -- Fall through until support is implemented */
            case RTHTTPPROXYTYPE_UNKNOWN:
            case RTHTTPPROXYTYPE_INVALID:
            case RTHTTPPROXYTYPE_END:
            case RTHTTPPROXYTYPE_32BIT_HACK:
                LogRel(("CLOUD-NET: Unsupported proxy type %u\n", ProxyInfo.enmProxyType));
                RTHttpFreeProxyInfo(&ProxyInfo);
                return VERR_INVALID_PARAMETER;
        }
    }
    else
    {
        Assert(enmProxyMode == ProxyMode_NoProxy);
        return VINF_SUCCESS;
    }

    /* Resolve proxy host name to IP address if necessary */
    RTNETADDR addr;
    RTSocketParseInetAddress(ProxyInfo.pszProxyHost, ProxyInfo.uProxyPort, &addr);
    if (addr.enmType != RTNETADDRTYPE_IPV4)
    {
        LogRel(("CLOUD-NET: Unsupported address type %u\n", addr.enmType));
        RTHttpFreeProxyInfo(&ProxyInfo);
        return VERR_INVALID_PARAMETER;
    }

    InsertConfigString(      pCfg, Utf8StrFmt("%sProxyType",     pcszPrefix).c_str(), pszProxyType);
    InsertConfigInteger(     pCfg, Utf8StrFmt("%sProxyPort",     pcszPrefix).c_str(), ProxyInfo.uProxyPort);
    if (ProxyInfo.pszProxyHost)
        InsertConfigStringF( pCfg, Utf8StrFmt("%sProxyHost",     pcszPrefix).c_str(), "%RTnaipv4", addr.uAddr.IPv4);
    if (ProxyInfo.pszProxyUsername)
        InsertConfigString(  pCfg, Utf8StrFmt("%sProxyUser",     pcszPrefix).c_str(), ProxyInfo.pszProxyUsername);
    if (ProxyInfo.pszProxyPassword)
        InsertConfigPassword(pCfg, Utf8StrFmt("%sProxyPassword", pcszPrefix).c_str(), ProxyInfo.pszProxyPassword);

    RTHttpFreeProxyInfo(&ProxyInfo);
    RTStrFree(pszProxyType);
    return vrc;
}


/**
 * Construct the Network configuration tree
 *
 * @returns VBox status code.
 *
 * @param   pszDevice           The PDM device name.
 * @param   uInstance           The PDM device instance.
 * @param   uLun                The PDM LUN number of the drive.
 * @param   aNetworkAdapter     The network adapter whose attachment needs to be changed
 * @param   pCfg                Configuration node for the device
 * @param   pLunL0              To store the pointer to the LUN#0.
 * @param   pInst               The instance CFGM node
 * @param   fAttachDetach       To determine if the network attachment should
 *                              be attached/detached after/before
 *                              configuration.
 * @param   fIgnoreConnectFailure
 *                              True if connection failures should be ignored
 *                              (makes only sense for bridged/host-only networks).
 * @param   pUVM                The usermode VM handle.
 * @param   pVMM                The VMM vtable.
 *
 * @note   Locks this object for writing.
 * @thread EMT
 */
int Console::i_configNetwork(const char *pszDevice,
                             unsigned uInstance,
                             unsigned uLun,
                             INetworkAdapter *aNetworkAdapter,
                             PCFGMNODE pCfg,
                             PCFGMNODE pLunL0,
                             PCFGMNODE pInst,
                             bool fAttachDetach,
                             bool fIgnoreConnectFailure,
                             PUVM pUVM,
                             PCVMMR3VTABLE pVMM)
{
    RT_NOREF(fIgnoreConnectFailure);
    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.hrc(), VERR_ACCESS_DENIED);

    // InsertConfig* throws
    try
    {
        int vrc = VINF_SUCCESS;
        HRESULT hrc;
        Bstr bstr;

#ifdef VBOX_WITH_CLOUD_NET
        /* We'll need device's pCfg for cloud attachments */
        PCFGMNODE pDevCfg = pCfg;
#endif /* VBOX_WITH_CLOUD_NET */

#define H()         AssertLogRelMsgReturn(!FAILED(hrc), ("hrc=%Rhrc\n", hrc), VERR_MAIN_CONFIG_CONSTRUCTOR_COM_ERROR)

        /*
         * Locking the object before doing VMR3* calls is quite safe here, since
         * we're on EMT. Write lock is necessary because we indirectly modify the
         * meAttachmentType member.
         */
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

        ComPtr<IMachine> pMachine = i_machine();

        ComPtr<IVirtualBox> virtualBox;
        hrc = pMachine->COMGETTER(Parent)(virtualBox.asOutParam());                         H();

        ComPtr<IHost> host;
        hrc = virtualBox->COMGETTER(Host)(host.asOutParam());                               H();

        BOOL fSniffer;
        hrc = aNetworkAdapter->COMGETTER(TraceEnabled)(&fSniffer);                          H();

        NetworkAdapterPromiscModePolicy_T enmPromiscModePolicy;
        hrc = aNetworkAdapter->COMGETTER(PromiscModePolicy)(&enmPromiscModePolicy);         H();
        const char *pszPromiscuousGuestPolicy;
        switch (enmPromiscModePolicy)
        {
            case NetworkAdapterPromiscModePolicy_Deny:          pszPromiscuousGuestPolicy = "deny"; break;
            case NetworkAdapterPromiscModePolicy_AllowNetwork:  pszPromiscuousGuestPolicy = "allow-network"; break;
            case NetworkAdapterPromiscModePolicy_AllowAll:      pszPromiscuousGuestPolicy = "allow-all"; break;
            default: AssertFailedReturn(VERR_INTERNAL_ERROR_4);
        }

        if (fAttachDetach)
        {
            vrc = pVMM->pfnPDMR3DeviceDetach(pUVM, pszDevice, uInstance, uLun, 0 /*fFlags*/);
            if (vrc == VINF_PDM_NO_DRIVER_ATTACHED_TO_LUN)
                vrc = VINF_SUCCESS;
            AssertLogRelRCReturn(vrc, vrc);

            /* Nuke anything which might have been left behind. */
            pVMM->pfnCFGMR3RemoveNode(pVMM->pfnCFGMR3GetChildF(pInst, "LUN#%u", uLun));
        }

        Bstr networkName, trunkName, trunkType;
        NetworkAttachmentType_T eAttachmentType;
        hrc = aNetworkAdapter->COMGETTER(AttachmentType)(&eAttachmentType);                 H();

#ifdef VBOX_WITH_NETSHAPER
        ComObjPtr<IBandwidthGroup> pBwGroup;
        Bstr bstrBwGroup;
        hrc = aNetworkAdapter->COMGETTER(BandwidthGroup)(pBwGroup.asOutParam());            H();

        if (!pBwGroup.isNull())
        {
            hrc = pBwGroup->COMGETTER(Name)(bstrBwGroup.asOutParam());                      H();
        }
#endif /* VBOX_WITH_NETSHAPER */

        AssertMsg(uLun == 0, ("Network attachments with LUN > 0 are not supported yet\n"));
        InsertConfigNodeF(pInst, &pLunL0, "LUN#%u", uLun);

        /*
         * Do not insert neither a shaper nor a sniffer if we are not attached to anything.
         * This way we can easily detect if we are attached to anything at the device level.
         */
#ifdef VBOX_WITH_NETSHAPER
        if (bstrBwGroup.isNotEmpty() && eAttachmentType != NetworkAttachmentType_Null)
        {
            InsertConfigString(pLunL0, "Driver", "NetShaper");
            InsertConfigNode(pLunL0, "Config", &pCfg);
            InsertConfigString(pCfg, "BwGroup", bstrBwGroup);
            InsertConfigNode(pLunL0, "AttachedDriver", &pLunL0);
        }
#endif /* VBOX_WITH_NETSHAPER */

        if (fSniffer && eAttachmentType != NetworkAttachmentType_Null)
        {
            InsertConfigString(pLunL0, "Driver", "NetSniffer");
            InsertConfigNode(pLunL0, "Config", &pCfg);
            hrc = aNetworkAdapter->COMGETTER(TraceFile)(bstr.asOutParam());             H();
            if (!bstr.isEmpty()) /* check convention for indicating default file. */
                InsertConfigString(pCfg, "File", bstr);
            InsertConfigNode(pLunL0, "AttachedDriver", &pLunL0);
        }

        switch (eAttachmentType)
        {
            case NetworkAttachmentType_Null:
                break;

            case NetworkAttachmentType_NAT:
            {
                ComPtr<INATEngine> natEngine;
                hrc = aNetworkAdapter->COMGETTER(NATEngine)(natEngine.asOutParam());        H();
                InsertConfigString(pLunL0, "Driver", "NAT");
                InsertConfigNode(pLunL0, "Config", &pCfg);

                /* Configure TFTP prefix and boot filename. */
                hrc = virtualBox->COMGETTER(HomeFolder)(bstr.asOutParam());                 H();
                if (!bstr.isEmpty())
                    InsertConfigStringF(pCfg, "TFTPPrefix", "%ls%c%s", bstr.raw(), RTPATH_DELIMITER, "TFTP");
                hrc = pMachine->COMGETTER(Name)(bstr.asOutParam());                         H();
                InsertConfigStringF(pCfg, "BootFile", "%ls.pxe", bstr.raw());

                hrc = natEngine->COMGETTER(Network)(bstr.asOutParam());                     H();
                if (!bstr.isEmpty())
                    InsertConfigString(pCfg, "Network", bstr);
                else
                {
                    ULONG uSlot;
                    hrc = aNetworkAdapter->COMGETTER(Slot)(&uSlot);                         H();
                    InsertConfigStringF(pCfg, "Network", "10.0.%d.0/24", uSlot+2);
                }
                hrc = natEngine->COMGETTER(HostIP)(bstr.asOutParam());                      H();
                if (!bstr.isEmpty())
                    InsertConfigString(pCfg, "BindIP", bstr);
                ULONG mtu = 0;
                ULONG sockSnd = 0;
                ULONG sockRcv = 0;
                ULONG tcpSnd = 0;
                ULONG tcpRcv = 0;
                hrc = natEngine->GetNetworkSettings(&mtu, &sockSnd, &sockRcv, &tcpSnd, &tcpRcv); H();
                if (mtu)
                    InsertConfigInteger(pCfg, "SlirpMTU", mtu);
                if (sockRcv)
                    InsertConfigInteger(pCfg, "SockRcv", sockRcv);
                if (sockSnd)
                    InsertConfigInteger(pCfg, "SockSnd", sockSnd);
                if (tcpRcv)
                    InsertConfigInteger(pCfg, "TcpRcv", tcpRcv);
                if (tcpSnd)
                    InsertConfigInteger(pCfg, "TcpSnd", tcpSnd);
                hrc = natEngine->COMGETTER(TFTPPrefix)(bstr.asOutParam());                  H();
                if (!bstr.isEmpty())
                {
                    RemoveConfigValue(pCfg, "TFTPPrefix");
                    InsertConfigString(pCfg, "TFTPPrefix", bstr);
                }
                hrc = natEngine->COMGETTER(TFTPBootFile)(bstr.asOutParam());                H();
                if (!bstr.isEmpty())
                {
                    RemoveConfigValue(pCfg, "BootFile");
                    InsertConfigString(pCfg, "BootFile", bstr);
                }
                hrc = natEngine->COMGETTER(TFTPNextServer)(bstr.asOutParam());              H();
                if (!bstr.isEmpty())
                    InsertConfigString(pCfg, "NextServer", bstr);
                BOOL fDNSFlag;
                hrc = natEngine->COMGETTER(DNSPassDomain)(&fDNSFlag);                       H();
                InsertConfigInteger(pCfg, "PassDomain", fDNSFlag);
                hrc = natEngine->COMGETTER(DNSProxy)(&fDNSFlag);                            H();
                InsertConfigInteger(pCfg, "DNSProxy", fDNSFlag);
                hrc = natEngine->COMGETTER(DNSUseHostResolver)(&fDNSFlag);                  H();
                InsertConfigInteger(pCfg, "UseHostResolver", fDNSFlag);

                ULONG aliasMode;
                hrc = natEngine->COMGETTER(AliasMode)(&aliasMode);                          H();
                InsertConfigInteger(pCfg, "AliasMode", aliasMode);

                BOOL fLocalhostReachable;
                hrc = natEngine->COMGETTER(LocalhostReachable)(&fLocalhostReachable);       H();
                InsertConfigInteger(pCfg, "LocalhostReachable", fLocalhostReachable);

                /* port-forwarding */
                SafeArray<BSTR> pfs;
                hrc = natEngine->COMGETTER(Redirects)(ComSafeArrayAsOutParam(pfs));         H();

                PCFGMNODE pPFTree = NULL;
                if (pfs.size() > 0)
                    InsertConfigNode(pCfg, "PortForwarding", &pPFTree);

                for (unsigned int i = 0; i < pfs.size(); ++i)
                {
                    PCFGMNODE pPF = NULL; /* /Devices/Dev/.../Config/PortForwarding/$n/ */

                    uint16_t port = 0;
                    Utf8Str utf = pfs[i];
                    Utf8Str strName;
                    Utf8Str strProto;
                    Utf8Str strHostPort;
                    Utf8Str strHostIP;
                    Utf8Str strGuestPort;
                    Utf8Str strGuestIP;
                    size_t pos, ppos;
                    pos = ppos = 0;
#define ITERATE_TO_NEXT_TERM(res, str, pos, ppos) \
    { \
        pos = str.find(",", ppos); \
        if (pos == Utf8Str::npos) \
        { \
            Log(( #res " extracting from %s is failed\n", str.c_str())); \
            continue; \
        } \
        res = str.substr(ppos, pos - ppos); \
        Log2((#res " %s pos:%d, ppos:%d\n", res.c_str(), pos, ppos)); \
        ppos = pos + 1; \
    } /* no do { ... } while because of 'continue' */
                    ITERATE_TO_NEXT_TERM(strName, utf, pos, ppos);
                    ITERATE_TO_NEXT_TERM(strProto, utf, pos, ppos);
                    ITERATE_TO_NEXT_TERM(strHostIP, utf, pos, ppos);
                    ITERATE_TO_NEXT_TERM(strHostPort, utf, pos, ppos);
                    ITERATE_TO_NEXT_TERM(strGuestIP, utf, pos, ppos);
                    strGuestPort = utf.substr(ppos, utf.length() - ppos);
#undef ITERATE_TO_NEXT_TERM

                    uint32_t proto = strProto.toUInt32();
                    bool fValid = true;
                    switch (proto)
                    {
                        case NATProtocol_UDP:
                            strProto = "UDP";
                            break;
                        case NATProtocol_TCP:
                            strProto = "TCP";
                            break;
                        default:
                            fValid = false;
                    }
                    /* continue with next rule if no valid proto was passed */
                    if (!fValid)
                        continue;

                    InsertConfigNodeF(pPFTree, &pPF, "%u", i);

                    if (!strName.isEmpty())
                        InsertConfigString(pPF, "Name", strName);

                    InsertConfigString(pPF, "Protocol", strProto);

                    if (!strHostIP.isEmpty())
                        InsertConfigString(pPF, "BindIP", strHostIP);

                    if (!strGuestIP.isEmpty())
                        InsertConfigString(pPF, "GuestIP", strGuestIP);

                    port = RTStrToUInt16(strHostPort.c_str());
                    if (port)
                        InsertConfigInteger(pPF, "HostPort", port);

                    port = RTStrToUInt16(strGuestPort.c_str());
                    if (port)
                        InsertConfigInteger(pPF, "GuestPort", port);
                }
                break;
            }

            case NetworkAttachmentType_Bridged:
            {
#if (defined(RT_OS_LINUX) || defined(RT_OS_FREEBSD)) && !defined(VBOX_WITH_NETFLT)
                hrc = i_attachToTapInterface(aNetworkAdapter);
                if (FAILED(hrc))
                {
                    switch (hrc)
                    {
                        case E_ACCESSDENIED:
                            return pVMM->pfnVMR3SetError(pUVM, VERR_HOSTIF_INIT_FAILED, RT_SRC_POS,  N_(
                                                         "Failed to open '/dev/net/tun' for read/write access. Please check the "
                                                         "permissions of that node. Either run 'chmod 0666 /dev/net/tun' or "
                                                         "change the group of that node and make yourself a member of that group. "
                                                         "Make sure that these changes are permanent, especially if you are "
                                                         "using udev"));
                        default:
                            AssertMsgFailed(("Could not attach to host interface! Bad!\n"));
                            return pVMM->pfnVMR3SetError(pUVM, VERR_HOSTIF_INIT_FAILED, RT_SRC_POS,
                                                         N_("Failed to initialize Host Interface Networking"));
                    }
                }

                Assert((intptr_t)maTapFD[uInstance] >= 0);
                if ((intptr_t)maTapFD[uInstance] >= 0)
                {
                    InsertConfigString(pLunL0, "Driver", "HostInterface");
                    InsertConfigNode(pLunL0, "Config", &pCfg);
                    InsertConfigInteger(pCfg, "FileHandle", (intptr_t)maTapFD[uInstance]);
                }

#elif defined(VBOX_WITH_NETFLT)
                /*
                 * This is the new VBoxNetFlt+IntNet stuff.
                 */
                Bstr BridgedIfName;
                hrc = aNetworkAdapter->COMGETTER(BridgedInterface)(BridgedIfName.asOutParam());
                if (FAILED(hrc))
                {
                    LogRel(("NetworkAttachmentType_Bridged: COMGETTER(BridgedInterface) failed, hrc (0x%x)\n", hrc));
                    H();
                }

                Utf8Str BridgedIfNameUtf8(BridgedIfName);
                const char *pszBridgedIfName = BridgedIfNameUtf8.c_str();

                ComPtr<IHostNetworkInterface> hostInterface;
                hrc = host->FindHostNetworkInterfaceByName(BridgedIfName.raw(),
                                                           hostInterface.asOutParam());
                if (!SUCCEEDED(hrc))
                {
                    AssertLogRelMsgFailed(("NetworkAttachmentType_Bridged: FindByName failed, hrc=%Rhrc (0x%x)\n", hrc, hrc));
                    return pVMM->pfnVMR3SetError(pUVM, VERR_INTERNAL_ERROR, RT_SRC_POS,
                                                 N_("Nonexistent host networking interface, name '%ls'"),
                                                 BridgedIfName.raw());
                }

# if defined(RT_OS_DARWIN)
                /* The name is in the format 'ifX: long name', chop it off at the colon. */
                char szTrunk[INTNET_MAX_TRUNK_NAME];
                RTStrCopy(szTrunk, sizeof(szTrunk), pszBridgedIfName);
                char *pszColon = (char *)memchr(szTrunk, ':', sizeof(szTrunk));
// Quick fix for @bugref{5633}
//                 if (!pszColon)
//                 {
//                     /*
//                     * Dynamic changing of attachment causes an attempt to configure
//                     * network with invalid host adapter (as it is must be changed before
//                     * the attachment), calling Detach here will cause a deadlock.
//                     * See @bugref{4750}.
//                     * hrc = aNetworkAdapter->Detach();                                   H();
//                     */
//                     return VMSetError(VMR3GetVM(mpUVM), VERR_INTERNAL_ERROR, RT_SRC_POS,
//                                       N_("Malformed host interface networking name '%ls'"),
//                                       BridgedIfName.raw());
//                 }
                if (pszColon)
                    *pszColon = '\0';
                const char *pszTrunk = szTrunk;

# elif defined(RT_OS_SOLARIS)
                /* The name is in the format 'ifX[:1] - long name, chop it off at space. */
                char szTrunk[256];
                strlcpy(szTrunk, pszBridgedIfName, sizeof(szTrunk));
                char *pszSpace = (char *)memchr(szTrunk, ' ', sizeof(szTrunk));

                /*
                 * Currently don't bother about malformed names here for the sake of people using
                 * VBoxManage and setting only the NIC name from there. If there is a space we
                 * chop it off and proceed, otherwise just use whatever we've got.
                 */
                if (pszSpace)
                    *pszSpace = '\0';

                /* Chop it off at the colon (zone naming eg: e1000g:1 we need only the e1000g) */
                char *pszColon = (char *)memchr(szTrunk, ':', sizeof(szTrunk));
                if (pszColon)
                    *pszColon = '\0';

                const char *pszTrunk = szTrunk;

# elif defined(RT_OS_WINDOWS)
                HostNetworkInterfaceType_T eIfType;
                hrc = hostInterface->COMGETTER(InterfaceType)(&eIfType);
                if (FAILED(hrc))
                {
                    LogRel(("NetworkAttachmentType_Bridged: COMGETTER(InterfaceType) failed, hrc (0x%x)\n", hrc));
                    H();
                }

                if (eIfType != HostNetworkInterfaceType_Bridged)
                    return pVMM->pfnVMR3SetError(pUVM, VERR_INTERNAL_ERROR, RT_SRC_POS,
                                                 N_("Interface ('%ls') is not a Bridged Adapter interface"),
                                                 BridgedIfName.raw());

                hrc = hostInterface->COMGETTER(Id)(bstr.asOutParam());
                if (FAILED(hrc))
                {
                    LogRel(("NetworkAttachmentType_Bridged: COMGETTER(Id) failed, hrc (0x%x)\n", hrc));
                    H();
                }
                Guid hostIFGuid(bstr);

                INetCfg *pNc;
                ComPtr<INetCfgComponent> pAdaptorComponent;
                LPWSTR pszApp;

                hrc = VBoxNetCfgWinQueryINetCfg(&pNc, FALSE, L"VirtualBox", 10, &pszApp);
                Assert(hrc == S_OK);
                if (hrc != S_OK)
                {
                    LogRel(("NetworkAttachmentType_Bridged: Failed to get NetCfg, hrc=%Rhrc (0x%x)\n", hrc, hrc));
                    H();
                }

                /* get the adapter's INetCfgComponent*/
                hrc = VBoxNetCfgWinGetComponentByGuid(pNc, &GUID_DEVCLASS_NET, (GUID*)hostIFGuid.raw(),
                                                      pAdaptorComponent.asOutParam());
                if (hrc != S_OK)
                {
                    VBoxNetCfgWinReleaseINetCfg(pNc, FALSE /*fHasWriteLock*/);
                    LogRel(("NetworkAttachmentType_Bridged: VBoxNetCfgWinGetComponentByGuid failed, hrc (0x%x)\n", hrc));
                    H();
                }
# define VBOX_WIN_BINDNAME_PREFIX "\\DEVICE\\"
                char szTrunkName[INTNET_MAX_TRUNK_NAME];
                char *pszTrunkName = szTrunkName;
                wchar_t * pswzBindName;
                hrc = pAdaptorComponent->GetBindName(&pswzBindName);
                Assert(hrc == S_OK);
                if (hrc == S_OK)
                {
                    int cwBindName = (int)wcslen(pswzBindName) + 1;
                    int cbFullBindNamePrefix = sizeof(VBOX_WIN_BINDNAME_PREFIX);
                    if (sizeof(szTrunkName) > cbFullBindNamePrefix + cwBindName)
                    {
                        strcpy(szTrunkName, VBOX_WIN_BINDNAME_PREFIX);
                        pszTrunkName += cbFullBindNamePrefix-1;
                        if (!WideCharToMultiByte(CP_ACP, 0, pswzBindName, cwBindName, pszTrunkName,
                                                sizeof(szTrunkName) - cbFullBindNamePrefix + 1, NULL, NULL))
                        {
                            DWORD err = GetLastError();
                            hrc = HRESULT_FROM_WIN32(err);
                            AssertMsgFailed(("hrc=%Rhrc %#x\n", hrc, hrc));
                            AssertLogRelMsgFailed(("NetworkAttachmentType_Bridged: WideCharToMultiByte failed, hr=%Rhrc (0x%x) err=%u\n",
                                                   hrc, hrc, err));
                        }
                    }
                    else
                    {
                        AssertLogRelMsgFailed(("NetworkAttachmentType_Bridged: insufficient szTrunkName buffer space\n"));
                        /** @todo set appropriate error code */
                        hrc = E_FAIL;
                    }

                    if (hrc != S_OK)
                    {
                        AssertFailed();
                        CoTaskMemFree(pswzBindName);
                        VBoxNetCfgWinReleaseINetCfg(pNc, FALSE /*fHasWriteLock*/);
                        H();
                    }

                    /* we're not freeing the bind name since we'll use it later for detecting wireless*/
                }
                else
                {
                    VBoxNetCfgWinReleaseINetCfg(pNc, FALSE /*fHasWriteLock*/);
                    AssertLogRelMsgFailed(("NetworkAttachmentType_Bridged: VBoxNetCfgWinGetComponentByGuid failed, hrc (0x%x)",
                                           hrc));
                    H();
                }

                const char *pszTrunk = szTrunkName;
                /* we're not releasing the INetCfg stuff here since we use it later to figure out whether it is wireless */

# elif defined(RT_OS_LINUX) || defined(RT_OS_FREEBSD)
#  if defined(RT_OS_FREEBSD)
                /*
                 * If we bridge to a tap interface open it the `old' direct way.
                 * This works and performs better than bridging a physical
                 * interface via the current FreeBSD vboxnetflt implementation.
                 */
                if (!strncmp(pszBridgedIfName, RT_STR_TUPLE("tap"))) {
                    hrc = i_attachToTapInterface(aNetworkAdapter);
                    if (FAILED(hrc))
                    {
                        switch (hrc)
                        {
                            case E_ACCESSDENIED:
                                return pVMM->pfnVMR3SetError(pUVM, VERR_HOSTIF_INIT_FAILED, RT_SRC_POS,  N_(
                                                             "Failed to open '/dev/%s' for read/write access.  Please check the "
                                                             "permissions of that node, and that the net.link.tap.user_open "
                                                             "sysctl is set.  Either run 'chmod 0666 /dev/%s' or change the "
                                                             "group of that node to vboxusers and make yourself a member of "
                                                             "that group.  Make sure that these changes are permanent."),
                                                             pszBridgedIfName, pszBridgedIfName);
                            default:
                                AssertMsgFailed(("Could not attach to tap interface! Bad!\n"));
                                return pVMM->pfnVMR3SetError(pUVM, VERR_HOSTIF_INIT_FAILED, RT_SRC_POS,
                                                             N_("Failed to initialize Host Interface Networking"));
                        }
                    }

                    Assert((intptr_t)maTapFD[uInstance] >= 0);
                    if ((intptr_t)maTapFD[uInstance] >= 0)
                    {
                        InsertConfigString(pLunL0, "Driver", "HostInterface");
                        InsertConfigNode(pLunL0, "Config", &pCfg);
                        InsertConfigInteger(pCfg, "FileHandle", (intptr_t)maTapFD[uInstance]);
                    }
                    break;
                }
#  endif
                /** @todo Check for malformed names. */
                const char *pszTrunk = pszBridgedIfName;

                /* Issue a warning if the interface is down */
                {
                    int iSock = socket(AF_INET, SOCK_DGRAM, 0);
                    if (iSock >= 0)
                    {
                        struct ifreq Req;
                        RT_ZERO(Req);
                        RTStrCopy(Req.ifr_name, sizeof(Req.ifr_name), pszBridgedIfName);
                        if (ioctl(iSock, SIOCGIFFLAGS, &Req) >= 0)
                            if ((Req.ifr_flags & IFF_UP) == 0)
                                i_atVMRuntimeErrorCallbackF(0, "BridgedInterfaceDown",
                                     N_("Bridged interface %s is down. Guest will not be able to use this interface"),
                                     pszBridgedIfName);

                        close(iSock);
                    }
                }

# else
#  error "PORTME (VBOX_WITH_NETFLT)"
# endif

# if defined(RT_OS_DARWIN) && defined(VBOX_WITH_VMNET)
                InsertConfigString(pLunL0, "Driver", "VMNet");
                InsertConfigNode(pLunL0, "Config", &pCfg);
                InsertConfigString(pCfg, "Trunk", pszTrunk);
                InsertConfigInteger(pCfg, "TrunkType", kIntNetTrunkType_NetFlt);
# else
                InsertConfigString(pLunL0, "Driver", "IntNet");
                InsertConfigNode(pLunL0, "Config", &pCfg);
                InsertConfigString(pCfg, "Trunk", pszTrunk);
                InsertConfigInteger(pCfg, "TrunkType", kIntNetTrunkType_NetFlt);
                InsertConfigInteger(pCfg, "IgnoreConnectFailure", (uint64_t)fIgnoreConnectFailure);
                InsertConfigString(pCfg, "IfPolicyPromisc", pszPromiscuousGuestPolicy);
                char szNetwork[INTNET_MAX_NETWORK_NAME];

#  if defined(RT_OS_SOLARIS) || defined(RT_OS_DARWIN)
                /*
                 * 'pszTrunk' contains just the interface name required in ring-0, while 'pszBridgedIfName' contains
                 * interface name + optional description. We must not pass any description to the VM as it can differ
                 * for the same interface name, eg: "nge0 - ethernet" (GUI) vs "nge0" (VBoxManage).
                 */
                RTStrPrintf(szNetwork, sizeof(szNetwork), "HostInterfaceNetworking-%s", pszTrunk);
#  else
                RTStrPrintf(szNetwork, sizeof(szNetwork), "HostInterfaceNetworking-%s", pszBridgedIfName);
#  endif
                InsertConfigString(pCfg, "Network", szNetwork);
                networkName = Bstr(szNetwork);
                trunkName = Bstr(pszTrunk);
                trunkType = Bstr(TRUNKTYPE_NETFLT);

                BOOL fSharedMacOnWire = false;
                hrc = hostInterface->COMGETTER(Wireless)(&fSharedMacOnWire);
                if (FAILED(hrc))
                {
                    LogRel(("NetworkAttachmentType_Bridged: COMGETTER(Wireless) failed, hrc (0x%x)\n", hrc));
                    H();
                }
                else if (fSharedMacOnWire)
                {
                    InsertConfigInteger(pCfg, "SharedMacOnWire", true);
                    Log(("Set SharedMacOnWire\n"));
                }

#  if defined(RT_OS_SOLARIS)
#   if 0 /* bird: this is a bit questionable and might cause more trouble than its worth.  */
                /* Zone access restriction, don't allow snooping the global zone. */
                zoneid_t ZoneId = getzoneid();
                if (ZoneId != GLOBAL_ZONEID)
                {
                    InsertConfigInteger(pCfg, "IgnoreAllPromisc", true);
                }
#   endif
#  endif
# endif

#elif defined(RT_OS_WINDOWS) /* not defined NetFlt */
            /* NOTHING TO DO HERE */
#elif defined(RT_OS_LINUX)
/// @todo aleksey: is there anything to be done here?
#elif defined(RT_OS_FREEBSD)
/** @todo FreeBSD: Check out this later (HIF networking). */
#else
# error "Port me"
#endif
                break;
            }

            case NetworkAttachmentType_Internal:
            {
                hrc = aNetworkAdapter->COMGETTER(InternalNetwork)(bstr.asOutParam());       H();
                if (!bstr.isEmpty())
                {
                    InsertConfigString(pLunL0, "Driver", "IntNet");
                    InsertConfigNode(pLunL0, "Config", &pCfg);
                    InsertConfigString(pCfg, "Network", bstr);
                    InsertConfigInteger(pCfg, "TrunkType", kIntNetTrunkType_WhateverNone);
                    InsertConfigString(pCfg, "IfPolicyPromisc", pszPromiscuousGuestPolicy);
                    networkName = bstr;
                    trunkType = Bstr(TRUNKTYPE_WHATEVER);
                }
                break;
            }

            case NetworkAttachmentType_HostOnly:
            {
                InsertConfigString(pLunL0, "Driver", "IntNet");
                InsertConfigNode(pLunL0, "Config", &pCfg);

                Bstr HostOnlyName;
                hrc = aNetworkAdapter->COMGETTER(HostOnlyInterface)(HostOnlyName.asOutParam());
                if (FAILED(hrc))
                {
                    LogRel(("NetworkAttachmentType_HostOnly: COMGETTER(HostOnlyInterface) failed, hrc (0x%x)\n", hrc));
                    H();
                }

                Utf8Str HostOnlyNameUtf8(HostOnlyName);
                const char *pszHostOnlyName = HostOnlyNameUtf8.c_str();
#ifdef VBOX_WITH_VMNET
                /* Check if the matching host-only network has already been created. */
                Bstr bstrLowerIP, bstrUpperIP, bstrNetworkMask;
                BstrFmt bstrNetworkName("Legacy %s Network", pszHostOnlyName);
                ComPtr<IHostOnlyNetwork> hostOnlyNetwork;
                hrc = virtualBox->FindHostOnlyNetworkByName(bstrNetworkName.raw(), hostOnlyNetwork.asOutParam());
                if (FAILED(hrc))
                {
                    /*
                    * With VMNET there is no VBoxNetAdp to create vboxnetX adapters,
                    * which means that the Host object won't be able to re-create
                    * them from extra data. Go through existing DHCP/adapter config
                    * to derive the parameters for the new network.
                    */
                    BstrFmt bstrOldNetworkName("HostInterfaceNetworking-%s", pszHostOnlyName);
                    ComPtr<IDHCPServer> dhcpServer;
                    hrc = virtualBox->FindDHCPServerByNetworkName(bstrOldNetworkName.raw(),
                                                                dhcpServer.asOutParam());
                    if (SUCCEEDED(hrc))
                    {
                        /* There is a DHCP server available for this network. */
                        hrc = dhcpServer->COMGETTER(LowerIP)(bstrLowerIP.asOutParam());
                        if (FAILED(hrc))
                        {
                            LogRel(("Console::i_configNetwork: COMGETTER(LowerIP) failed, hrc (%Rhrc)\n", hrc));
                            H();
                        }
                        hrc = dhcpServer->COMGETTER(UpperIP)(bstrUpperIP.asOutParam());
                        if (FAILED(hrc))
                        {
                            LogRel(("Console::i_configNetwork: COMGETTER(UpperIP) failed, hrc (%Rhrc)\n", hrc));
                            H();
                        }
                        hrc = dhcpServer->COMGETTER(NetworkMask)(bstrNetworkMask.asOutParam());
                        if (FAILED(hrc))
                        {
                            LogRel(("Console::i_configNetwork: COMGETTER(NetworkMask) failed, hrc (%Rhrc)\n", hrc));
                            H();
                        }
                    }
                    else
                    {
                        /* No DHCP server for this hostonly interface, let's look at extra data */
                        hrc = virtualBox->GetExtraData(BstrFmt("HostOnly/%s/IPAddress",
                                                            pszHostOnlyName).raw(),
                                                            bstrLowerIP.asOutParam());
                        if (SUCCEEDED(hrc) && !bstrLowerIP.isEmpty())
                            hrc = virtualBox->GetExtraData(BstrFmt("HostOnly/%s/IPNetMask",
                                                                pszHostOnlyName).raw(),
                                                                bstrNetworkMask.asOutParam());

                    }
                    RTNETADDRIPV4 ipAddr, ipMask;
                    vrc = bstrLowerIP.isEmpty() ? VERR_MISSING : RTNetStrToIPv4Addr(Utf8Str(bstrLowerIP).c_str(), &ipAddr);
                    if (RT_FAILURE(vrc))
                    {
                        /* We failed to locate any valid config of this vboxnetX interface, assume defaults. */
                        LogRel(("NetworkAttachmentType_HostOnly: Invalid or missing lower IP '%ls', using '%ls' instead.\n",
                                bstrLowerIP.raw(), getDefaultIPv4Address(Bstr(pszHostOnlyName)).raw()));
                        bstrLowerIP = getDefaultIPv4Address(Bstr(pszHostOnlyName));
                        bstrNetworkMask.setNull();
                        bstrUpperIP.setNull();
                        vrc = RTNetStrToIPv4Addr(Utf8Str(bstrLowerIP).c_str(), &ipAddr);
                        AssertLogRelMsgReturn(RT_SUCCESS(vrc), ("RTNetStrToIPv4Addr(%ls) failed, vrc=%Rrc\n", bstrLowerIP.raw(), vrc),
                                              VERR_MAIN_CONFIG_CONSTRUCTOR_COM_ERROR);
                    }
                    vrc = bstrNetworkMask.isEmpty() ? VERR_MISSING : RTNetStrToIPv4Addr(Utf8Str(bstrNetworkMask).c_str(), &ipMask);
                    if (RT_FAILURE(vrc))
                    {
                        LogRel(("NetworkAttachmentType_HostOnly: Invalid or missing network mask '%ls', using '%s' instead.\n",
                                bstrNetworkMask.raw(), VBOXNET_IPV4MASK_DEFAULT));
                        bstrNetworkMask = VBOXNET_IPV4MASK_DEFAULT;
                        vrc = RTNetStrToIPv4Addr(Utf8Str(bstrNetworkMask).c_str(), &ipMask);
                        AssertLogRelMsgReturn(RT_SUCCESS(vrc), ("RTNetStrToIPv4Addr(%ls) failed, vrc=%Rrc\n", bstrNetworkMask.raw(), vrc),
                                              VERR_MAIN_CONFIG_CONSTRUCTOR_COM_ERROR);
                    }
                    vrc = bstrUpperIP.isEmpty() ? VERR_MISSING : RTNetStrToIPv4Addr(Utf8Str(bstrUpperIP).c_str(), &ipAddr);
                    if (RT_FAILURE(vrc))
                    {
                        ipAddr.au32[0] = RT_H2N_U32((RT_N2H_U32(ipAddr.au32[0]) | ~RT_N2H_U32(ipMask.au32[0])) - 1); /* Do we need to exlude the last IP? */
                        LogRel(("NetworkAttachmentType_HostOnly: Invalid or missing upper IP '%ls', using '%RTnaipv4' instead.\n",
                                bstrUpperIP.raw(), ipAddr));
                        bstrUpperIP = BstrFmt("%RTnaipv4", ipAddr);
                    }

                    /* All parameters are set, create the new network. */
                    hrc = virtualBox->CreateHostOnlyNetwork(bstrNetworkName.raw(), hostOnlyNetwork.asOutParam());
                    if (FAILED(hrc))
                    {
                        LogRel(("NetworkAttachmentType_HostOnly: failed to create host-only network, hrc (0x%x)\n", hrc));
                        H();
                    }
                    hrc = hostOnlyNetwork->COMSETTER(NetworkMask)(bstrNetworkMask.raw());
                    if (FAILED(hrc))
                    {
                        LogRel(("NetworkAttachmentType_HostOnly: COMSETTER(NetworkMask) failed, hrc (0x%x)\n", hrc));
                        H();
                    }
                    hrc = hostOnlyNetwork->COMSETTER(LowerIP)(bstrLowerIP.raw());
                    if (FAILED(hrc))
                    {
                        LogRel(("NetworkAttachmentType_HostOnly: COMSETTER(LowerIP) failed, hrc (0x%x)\n", hrc));
                        H();
                    }
                    hrc = hostOnlyNetwork->COMSETTER(UpperIP)(bstrUpperIP.raw());
                    if (FAILED(hrc))
                    {
                        LogRel(("NetworkAttachmentType_HostOnly: COMSETTER(UpperIP) failed, hrc (0x%x)\n", hrc));
                        H();
                    }
                    LogRel(("Console: created host-only network '%ls' with mask '%ls' and range '%ls'-'%ls'\n",
                            bstrNetworkName.raw(), bstrNetworkMask.raw(), bstrLowerIP.raw(), bstrUpperIP.raw()));
                }
                else
                {
                    /* The matching host-only network already exists. Tell the user to switch to it. */
                    hrc = hostOnlyNetwork->COMGETTER(NetworkMask)(bstrNetworkMask.asOutParam());
                    if (FAILED(hrc))
                    {
                        LogRel(("NetworkAttachmentType_HostOnly: COMGETTER(NetworkMask) failed, hrc (0x%x)\n", hrc));
                        H();
                    }
                    hrc = hostOnlyNetwork->COMGETTER(LowerIP)(bstrLowerIP.asOutParam());
                    if (FAILED(hrc))
                    {
                        LogRel(("NetworkAttachmentType_HostOnly: COMGETTER(LowerIP) failed, hrc (0x%x)\n", hrc));
                        H();
                    }
                    hrc = hostOnlyNetwork->COMGETTER(UpperIP)(bstrUpperIP.asOutParam());
                    if (FAILED(hrc))
                    {
                        LogRel(("NetworkAttachmentType_HostOnly: COMGETTER(UpperIP) failed, hrc (0x%x)\n", hrc));
                        H();
                    }
                }
                return pVMM->pfnVMR3SetError(pUVM, VERR_NOT_FOUND, RT_SRC_POS,
                                             N_("Host-only adapters are no longer supported!\n"
                                                "For your convenience a host-only network named '%ls' has been "
                                                "created with network mask '%ls' and IP address range '%ls' - '%ls'.\n"
                                                "To fix this problem, switch to 'Host-only Network' "
                                                "attachment type in the VM settings.\n"),
                                             bstrNetworkName.raw(), bstrNetworkMask.raw(),
                                             bstrLowerIP.raw(), bstrUpperIP.raw());
#endif /* VBOX_WITH_VMNET */
                ComPtr<IHostNetworkInterface> hostInterface;
                hrc = host->FindHostNetworkInterfaceByName(HostOnlyName.raw(),
                                                           hostInterface.asOutParam());
                if (!SUCCEEDED(hrc))
                {
                    LogRel(("NetworkAttachmentType_HostOnly: FindByName failed, vrc=%Rrc\n", vrc));
                    return pVMM->pfnVMR3SetError(pUVM, VERR_INTERNAL_ERROR, RT_SRC_POS,
                                                N_("Nonexistent host networking interface, name '%ls'"), HostOnlyName.raw());
                }

                char szNetwork[INTNET_MAX_NETWORK_NAME];
                RTStrPrintf(szNetwork, sizeof(szNetwork), "HostInterfaceNetworking-%s", pszHostOnlyName);

#if defined(RT_OS_WINDOWS)
# ifndef VBOX_WITH_NETFLT
                hrc = E_NOTIMPL;
                LogRel(("NetworkAttachmentType_HostOnly: Not Implemented\n"));
                H();
# else  /* defined VBOX_WITH_NETFLT*/
                /** @todo r=bird: Put this in a function. */

                HostNetworkInterfaceType_T eIfType;
                hrc = hostInterface->COMGETTER(InterfaceType)(&eIfType);
                if (FAILED(hrc))
                {
                    LogRel(("NetworkAttachmentType_HostOnly: COMGETTER(InterfaceType) failed, hrc (0x%x)\n", hrc));
                    H();
                }

                if (eIfType != HostNetworkInterfaceType_HostOnly)
                    return pVMM->pfnVMR3SetError(pUVM, VERR_INTERNAL_ERROR, RT_SRC_POS,
                                                 N_("Interface ('%ls') is not a Host-Only Adapter interface"),
                                                 HostOnlyName.raw());

                hrc = hostInterface->COMGETTER(Id)(bstr.asOutParam());
                if (FAILED(hrc))
                {
                    LogRel(("NetworkAttachmentType_HostOnly: COMGETTER(Id) failed, hrc (0x%x)\n", hrc));
                    H();
                }
                Guid hostIFGuid(bstr);

                INetCfg *pNc;
                ComPtr<INetCfgComponent> pAdaptorComponent;
                LPWSTR pszApp;
                hrc = VBoxNetCfgWinQueryINetCfg(&pNc, FALSE, L"VirtualBox", 10, &pszApp);
                Assert(hrc == S_OK);
                if (hrc != S_OK)
                {
                    LogRel(("NetworkAttachmentType_HostOnly: Failed to get NetCfg, hrc=%Rhrc (0x%x)\n", hrc, hrc));
                    H();
                }

                /* get the adapter's INetCfgComponent*/
                hrc = VBoxNetCfgWinGetComponentByGuid(pNc, &GUID_DEVCLASS_NET, (GUID*)hostIFGuid.raw(),
                                                      pAdaptorComponent.asOutParam());
                if (hrc != S_OK)
                {
                    VBoxNetCfgWinReleaseINetCfg(pNc, FALSE /*fHasWriteLock*/);
                    LogRel(("NetworkAttachmentType_HostOnly: VBoxNetCfgWinGetComponentByGuid failed, hrc=%Rhrc (0x%x)\n", hrc, hrc));
                    H();
                }
#  define VBOX_WIN_BINDNAME_PREFIX "\\DEVICE\\"
                char szTrunkName[INTNET_MAX_TRUNK_NAME];
                bool fNdis6 = false;
                wchar_t * pwszHelpText;
                hrc = pAdaptorComponent->GetHelpText(&pwszHelpText);
                Assert(hrc == S_OK);
                if (hrc == S_OK)
                {
                    Log(("help-text=%ls\n", pwszHelpText));
                    if (!wcscmp(pwszHelpText, L"VirtualBox NDIS 6.0 Miniport Driver"))
                        fNdis6 = true;
                    CoTaskMemFree(pwszHelpText);
                }
                if (fNdis6)
                {
                    strncpy(szTrunkName, pszHostOnlyName, sizeof(szTrunkName) - 1);
                    Log(("trunk=%s\n", szTrunkName));
                }
                else
                {
                    char *pszTrunkName = szTrunkName;
                    wchar_t * pswzBindName;
                    hrc = pAdaptorComponent->GetBindName(&pswzBindName);
                    Assert(hrc == S_OK);
                    if (hrc == S_OK)
                    {
                        int cwBindName = (int)wcslen(pswzBindName) + 1;
                        int cbFullBindNamePrefix = sizeof(VBOX_WIN_BINDNAME_PREFIX);
                        if (sizeof(szTrunkName) > cbFullBindNamePrefix + cwBindName)
                        {
                            strcpy(szTrunkName, VBOX_WIN_BINDNAME_PREFIX);
                            pszTrunkName += cbFullBindNamePrefix-1;
                            if (!WideCharToMultiByte(CP_ACP, 0, pswzBindName, cwBindName, pszTrunkName,
                                                     sizeof(szTrunkName) - cbFullBindNamePrefix + 1, NULL, NULL))
                            {
                                DWORD err = GetLastError();
                                hrc = HRESULT_FROM_WIN32(err);
                                AssertLogRelMsgFailed(("NetworkAttachmentType_HostOnly: WideCharToMultiByte failed, hr=%Rhrc (0x%x) err=%u\n",
                                                       hrc, hrc, err));
                            }
                        }
                        else
                        {
                            AssertLogRelMsgFailed(("NetworkAttachmentType_HostOnly: insufficient szTrunkName buffer space\n"));
                            /** @todo set appropriate error code */
                            hrc = E_FAIL;
                        }

                        if (hrc != S_OK)
                        {
                            AssertFailed();
                            CoTaskMemFree(pswzBindName);
                            VBoxNetCfgWinReleaseINetCfg(pNc, FALSE /*fHasWriteLock*/);
                            H();
                        }
                    }
                    else
                    {
                        VBoxNetCfgWinReleaseINetCfg(pNc, FALSE /*fHasWriteLock*/);
                        AssertLogRelMsgFailed(("NetworkAttachmentType_HostOnly: VBoxNetCfgWinGetComponentByGuid failed, hrc=%Rhrc (0x%x)\n",
                                               hrc, hrc));
                        H();
                    }


                    CoTaskMemFree(pswzBindName);
                }

                trunkType = TRUNKTYPE_NETADP;
                InsertConfigInteger(pCfg, "TrunkType", kIntNetTrunkType_NetAdp);

                pAdaptorComponent.setNull();
                /* release the pNc finally */
                VBoxNetCfgWinReleaseINetCfg(pNc, FALSE /*fHasWriteLock*/);

                const char *pszTrunk = szTrunkName;

                InsertConfigString(pCfg, "Trunk", pszTrunk);
                InsertConfigString(pCfg, "Network", szNetwork);
                InsertConfigInteger(pCfg, "IgnoreConnectFailure", (uint64_t)fIgnoreConnectFailure); /** @todo why is this
                                                                                                        windows only?? */
                networkName = Bstr(szNetwork);
                trunkName   = Bstr(pszTrunk);
# endif /* defined VBOX_WITH_NETFLT*/
#elif defined(RT_OS_DARWIN)
                InsertConfigString(pCfg, "Trunk", pszHostOnlyName);
                InsertConfigString(pCfg, "Network", szNetwork);
                InsertConfigInteger(pCfg, "TrunkType", kIntNetTrunkType_NetAdp);
                networkName = Bstr(szNetwork);
                trunkName   = Bstr(pszHostOnlyName);
                trunkType   = TRUNKTYPE_NETADP;
#else
                InsertConfigString(pCfg, "Trunk", pszHostOnlyName);
                InsertConfigString(pCfg, "Network", szNetwork);
                InsertConfigInteger(pCfg, "TrunkType", kIntNetTrunkType_NetFlt);
                networkName = Bstr(szNetwork);
                trunkName   = Bstr(pszHostOnlyName);
                trunkType   = TRUNKTYPE_NETFLT;
#endif
                InsertConfigString(pCfg, "IfPolicyPromisc", pszPromiscuousGuestPolicy);

#if !defined(RT_OS_WINDOWS) && defined(VBOX_WITH_NETFLT)

                Bstr tmpAddr, tmpMask;

                hrc = virtualBox->GetExtraData(BstrFmt("HostOnly/%s/IPAddress",
                                                       pszHostOnlyName).raw(),
                                               tmpAddr.asOutParam());
                if (SUCCEEDED(hrc) && !tmpAddr.isEmpty())
                {
                    hrc = virtualBox->GetExtraData(BstrFmt("HostOnly/%s/IPNetMask",
                                                           pszHostOnlyName).raw(),
                                                   tmpMask.asOutParam());
                    if (SUCCEEDED(hrc) && !tmpMask.isEmpty())
                        hrc = hostInterface->EnableStaticIPConfig(tmpAddr.raw(),
                                                                  tmpMask.raw());
                    else
                        hrc = hostInterface->EnableStaticIPConfig(tmpAddr.raw(),
                                                                  Bstr(VBOXNET_IPV4MASK_DEFAULT).raw());
                }
                else
                {
                    /* Grab the IP number from the 'vboxnetX' instance number (see netif.h) */
                    hrc = hostInterface->EnableStaticIPConfig(getDefaultIPv4Address(Bstr(pszHostOnlyName)).raw(),
                                                              Bstr(VBOXNET_IPV4MASK_DEFAULT).raw());
                }

                ComAssertComRC(hrc); /** @todo r=bird: Why this isn't fatal? (H()) */

                hrc = virtualBox->GetExtraData(BstrFmt("HostOnly/%s/IPV6Address",
                                                       pszHostOnlyName).raw(),
                                               tmpAddr.asOutParam());
                if (SUCCEEDED(hrc))
                    hrc = virtualBox->GetExtraData(BstrFmt("HostOnly/%s/IPV6NetMask", pszHostOnlyName).raw(),
                                                   tmpMask.asOutParam());
                if (SUCCEEDED(hrc) && !tmpAddr.isEmpty() && !tmpMask.isEmpty())
                {
                    hrc = hostInterface->EnableStaticIPConfigV6(tmpAddr.raw(),
                                                                Utf8Str(tmpMask).toUInt32());
                    ComAssertComRC(hrc); /** @todo r=bird: Why this isn't fatal? (H()) */
                }
#endif
                break;
            }

            case NetworkAttachmentType_Generic:
            {
                hrc = aNetworkAdapter->COMGETTER(GenericDriver)(bstr.asOutParam());         H();
                SafeArray<BSTR> names;
                SafeArray<BSTR> values;
                hrc = aNetworkAdapter->GetProperties(Bstr().raw(),
                                                     ComSafeArrayAsOutParam(names),
                                                     ComSafeArrayAsOutParam(values));       H();

                InsertConfigString(pLunL0, "Driver", bstr);
                InsertConfigNode(pLunL0, "Config", &pCfg);
                for (size_t ii = 0; ii < names.size(); ++ii)
                {
                    if (values[ii] && *values[ii])
                    {
                        Utf8Str const strName(names[ii]);
                        Utf8Str const strValue(values[ii]);
                        InsertConfigString(pCfg, strName.c_str(), strValue);
                    }
                }
                break;
            }

            case NetworkAttachmentType_NATNetwork:
            {
                hrc = aNetworkAdapter->COMGETTER(NATNetwork)(bstr.asOutParam());            H();
                if (!bstr.isEmpty())
                {
                    /** @todo add intnet prefix to separate namespaces, and add trunk if dealing with vboxnatX */
                    InsertConfigString(pLunL0, "Driver", "IntNet");
                    InsertConfigNode(pLunL0, "Config", &pCfg);
                    InsertConfigString(pCfg, "Network", bstr);
                    InsertConfigInteger(pCfg, "TrunkType", kIntNetTrunkType_WhateverNone);
                    InsertConfigString(pCfg, "IfPolicyPromisc", pszPromiscuousGuestPolicy);
                    networkName = bstr;
                    trunkType = Bstr(TRUNKTYPE_WHATEVER);
                }
                break;
            }

#ifdef VBOX_WITH_CLOUD_NET
            case NetworkAttachmentType_Cloud:
            {
                static const char *s_pszCloudExtPackName = "Oracle VM VirtualBox Extension Pack";
                /*
                 * Cloud network attachments do not work wihout installed extpack.
                 * Without extpack support they won't work either.
                 */
# ifdef VBOX_WITH_EXTPACK
                if (!mptrExtPackManager->i_isExtPackUsable(s_pszCloudExtPackName))
# endif
                {
                    return pVMM->pfnVMR3SetError(pUVM, VERR_NOT_FOUND, RT_SRC_POS,
                                                 N_("Implementation of the cloud network attachment not found!\n"
                                                    "To fix this problem, either install the '%s' or switch to "
                                                    "another network attachment type in the VM settings."),
                                                 s_pszCloudExtPackName);
                }

                ComPtr<ICloudNetwork> network;
                hrc = aNetworkAdapter->COMGETTER(CloudNetwork)(bstr.asOutParam());            H();
                hrc = pMachine->COMGETTER(Name)(mGateway.mTargetVM.asOutParam());            H();
                hrc = virtualBox->FindCloudNetworkByName(bstr.raw(), network.asOutParam());   H();
                hrc = generateKeys(mGateway);
                if (FAILED(hrc))
                {
                    if (hrc == E_NOTIMPL)
                        return pVMM->pfnVMR3SetError(pUVM, VERR_NOT_FOUND, RT_SRC_POS,
                                                     N_("Failed to generate a key pair due to missing libssh\n"
                                                        "To fix this problem, either build VirtualBox with libssh "
                                                        "support or switch to another network attachment type in "
                                                        "the VM settings."));
                    return pVMM->pfnVMR3SetError(pUVM, VERR_INTERNAL_ERROR, RT_SRC_POS,
                                                 N_("Failed to generate a key pair due to libssh error!"));
                }
                hrc = startCloudGateway(virtualBox, network, mGateway);
                if (FAILED(hrc))
                {
                    if (hrc == VBOX_E_OBJECT_NOT_FOUND)
                        return pVMM->pfnVMR3SetError(pUVM, hrc, RT_SRC_POS,
                                                    N_("Failed to start cloud gateway instance.\nCould not find suitable "
                                                        "standard cloud images. Make sure you ran 'VBoxManage cloud network setup' "
                                                        "with correct '--gateway-os-name' and '--gateway-os-version' parameters. "
                                                        "Check VBoxSVC.log for actual values used to look up cloud images."));
                    return pVMM->pfnVMR3SetError(pUVM, hrc, RT_SRC_POS,
                                                 N_("Failed to start cloud gateway instance.\nMake sure you set up "
                                                    "cloud networking properly with 'VBoxManage cloud network setup'. "
                                                    "Check VBoxSVC.log for details."));
                }
                InsertConfigBytes(pDevCfg, "MAC", &mGateway.mCloudMacAddress, sizeof(mGateway.mCloudMacAddress));
                if (!bstr.isEmpty())
                {
                    InsertConfigString(pLunL0, "Driver", "CloudTunnel");
                    InsertConfigNode(pLunL0, "Config", &pCfg);
                    InsertConfigPassword(pCfg, "SshKey", mGateway.mPrivateSshKey);
                    InsertConfigString(pCfg, "PrimaryIP", mGateway.mCloudPublicIp);
                    InsertConfigString(pCfg, "SecondaryIP", mGateway.mCloudSecondaryPublicIp);
                    InsertConfigBytes(pCfg, "TargetMAC", &mGateway.mLocalMacAddress, sizeof(mGateway.mLocalMacAddress));
                    hrc = i_configProxy(virtualBox, pCfg, "Primary", mGateway.mCloudPublicIp);
                    if (FAILED(hrc))
                    {
                        return pVMM->pfnVMR3SetError(pUVM, hrc, RT_SRC_POS,
                                                    N_("Failed to configure proxy for accessing cloud gateway instance via primary VNIC.\n"
                                                        "Check VirtualBox.log for details."));
                    }
                    hrc = i_configProxy(virtualBox, pCfg, "Secondary", mGateway.mCloudSecondaryPublicIp);
                    if (FAILED(hrc))
                    {
                        return pVMM->pfnVMR3SetError(pUVM, hrc, RT_SRC_POS,
                                                    N_("Failed to configure proxy for accessing cloud gateway instance via secondary VNIC.\n"
                                                        "Check VirtualBox.log for details."));
                    }
                    networkName = bstr;
                    trunkType = Bstr(TRUNKTYPE_WHATEVER);
                }
                break;
            }
#endif /* VBOX_WITH_CLOUD_NET */

#ifdef VBOX_WITH_VMNET
            case NetworkAttachmentType_HostOnlyNetwork:
            {
                Bstr bstrId, bstrNetMask, bstrLowerIP, bstrUpperIP;
                ComPtr<IHostOnlyNetwork> network;
                hrc = aNetworkAdapter->COMGETTER(HostOnlyNetwork)(bstr.asOutParam());            H();
                hrc = virtualBox->FindHostOnlyNetworkByName(bstr.raw(), network.asOutParam());
                if (FAILED(hrc))
                {
                    LogRel(("NetworkAttachmentType_HostOnlyNetwork: FindByName failed, hrc (0x%x)\n", hrc));
                    return pVMM->pfnVMR3SetError(pUVM, VERR_INTERNAL_ERROR, RT_SRC_POS,
                                                 N_("Nonexistent host-only network '%ls'"), bstr.raw());
                }
                hrc = network->COMGETTER(Id)(bstrId.asOutParam());                               H();
                hrc = network->COMGETTER(NetworkMask)(bstrNetMask.asOutParam());                 H();
                hrc = network->COMGETTER(LowerIP)(bstrLowerIP.asOutParam());                     H();
                hrc = network->COMGETTER(UpperIP)(bstrUpperIP.asOutParam());                     H();
                if (!bstr.isEmpty())
                {
                    InsertConfigString(pLunL0, "Driver", "VMNet");
                    InsertConfigNode(pLunL0, "Config", &pCfg);
                    // InsertConfigString(pCfg, "Trunk", bstr);
                    // InsertConfigStringF(pCfg, "Network", "HostOnlyNetworking-%ls", bstr.raw());
                    InsertConfigInteger(pCfg, "TrunkType", kIntNetTrunkType_NetAdp);
                    InsertConfigString(pCfg, "Id", bstrId);
                    InsertConfigString(pCfg, "NetworkMask", bstrNetMask);
                    InsertConfigString(pCfg, "LowerIP", bstrLowerIP);
                    InsertConfigString(pCfg, "UpperIP", bstrUpperIP);
                    // InsertConfigString(pCfg, "IfPolicyPromisc", pszPromiscuousGuestPolicy);
                    networkName.setNull(); // We do not want DHCP server on our network!
                    // trunkType = Bstr(TRUNKTYPE_WHATEVER);
                }
                break;
            }
#endif /* VBOX_WITH_VMNET */

            default:
                AssertMsgFailed(("should not get here!\n"));
                break;
        }

        /*
         * Attempt to attach the driver.
         */
        switch (eAttachmentType)
        {
            case NetworkAttachmentType_Null:
                break;

            case NetworkAttachmentType_Bridged:
            case NetworkAttachmentType_Internal:
            case NetworkAttachmentType_HostOnly:
#ifdef VBOX_WITH_VMNET
            case NetworkAttachmentType_HostOnlyNetwork:
#endif /* VBOX_WITH_VMNET */
            case NetworkAttachmentType_NAT:
            case NetworkAttachmentType_Generic:
            case NetworkAttachmentType_NATNetwork:
#ifdef VBOX_WITH_CLOUD_NET
            case NetworkAttachmentType_Cloud:
#endif /* VBOX_WITH_CLOUD_NET */
            {
                if (SUCCEEDED(hrc) && RT_SUCCESS(vrc))
                {
                    if (fAttachDetach)
                    {
                        vrc = pVMM->pfnPDMR3DriverAttach(mpUVM, pszDevice, uInstance, uLun, 0 /*fFlags*/, NULL /* ppBase */);
                        //AssertRC(vrc);
                    }

                    {
                        /** @todo pritesh: get the dhcp server name from the
                         * previous network configuration and then stop the server
                         * else it may conflict with the dhcp server running  with
                         * the current attachment type
                         */
                        /* Stop the hostonly DHCP Server */
                    }

                    /*
                     * NAT networks start their DHCP server theirself, see NATNetwork::Start()
                     */
                    if (   !networkName.isEmpty()
                        && eAttachmentType != NetworkAttachmentType_NATNetwork)
                    {
                        /*
                         * Until we implement service reference counters DHCP Server will be stopped
                         * by DHCPServerRunner destructor.
                         */
                        ComPtr<IDHCPServer> dhcpServer;
                        hrc = virtualBox->FindDHCPServerByNetworkName(networkName.raw(), dhcpServer.asOutParam());
                        if (SUCCEEDED(hrc))
                        {
                            /* there is a DHCP server available for this network */
                            BOOL fEnabledDhcp;
                            hrc = dhcpServer->COMGETTER(Enabled)(&fEnabledDhcp);
                            if (FAILED(hrc))
                            {
                                LogRel(("DHCP svr: COMGETTER(Enabled) failed, hrc (%Rhrc)\n", hrc));
                                H();
                            }

                            if (fEnabledDhcp)
                                hrc = dhcpServer->Start(trunkName.raw(), trunkType.raw());
                        }
                        else
                            hrc = S_OK;
                    }
                }

                break;
            }

            default:
                AssertMsgFailed(("should not get here!\n"));
                break;
        }

        meAttachmentType[uInstance] = eAttachmentType;
    }
    catch (ConfigError &x)
    {
        // InsertConfig threw something:
        return x.m_vrc;
    }

#undef H

    return VINF_SUCCESS;
}


/**
 * Configures the serial port at the given CFGM node with the supplied parameters.
 *
 * @returns VBox status code.
 * @param   pInst               The instance CFGM node.
 * @param   ePortMode           The port mode to sue.
 * @param   pszPath             The serial port path.
 * @param   fServer             Flag whether the port should act as a server
 *                              for the pipe and TCP mode or connect as a client.
 */
int Console::i_configSerialPort(PCFGMNODE pInst, PortMode_T ePortMode, const char *pszPath, bool fServer)
{
    PCFGMNODE pLunL0 = NULL;        /* /Devices/Dev/0/LUN#0/ */
    PCFGMNODE pLunL1 = NULL;        /* /Devices/Dev/0/LUN#0/AttachedDriver/ */
    PCFGMNODE pLunL1Cfg = NULL;     /* /Devices/Dev/0/LUN#0/AttachedDriver/Config */

    try
    {
        InsertConfigNode(pInst, "LUN#0", &pLunL0);
        if (ePortMode == PortMode_HostPipe)
        {
            InsertConfigString(pLunL0,     "Driver", "Char");
            InsertConfigNode(pLunL0,       "AttachedDriver", &pLunL1);
            InsertConfigString(pLunL1,     "Driver", "NamedPipe");
            InsertConfigNode(pLunL1,       "Config", &pLunL1Cfg);
            InsertConfigString(pLunL1Cfg,  "Location", pszPath);
            InsertConfigInteger(pLunL1Cfg, "IsServer", fServer);
        }
        else if (ePortMode == PortMode_HostDevice)
        {
            InsertConfigString(pLunL0,     "Driver", "Host Serial");
            InsertConfigNode(pLunL0,       "Config", &pLunL1);
            InsertConfigString(pLunL1,     "DevicePath", pszPath);
        }
        else if (ePortMode == PortMode_TCP)
        {
            InsertConfigString(pLunL0,     "Driver", "Char");
            InsertConfigNode(pLunL0,       "AttachedDriver", &pLunL1);
            InsertConfigString(pLunL1,     "Driver", "TCP");
            InsertConfigNode(pLunL1,       "Config", &pLunL1Cfg);
            InsertConfigString(pLunL1Cfg,  "Location", pszPath);
            InsertConfigInteger(pLunL1Cfg, "IsServer", fServer);
        }
        else if (ePortMode == PortMode_RawFile)
        {
            InsertConfigString(pLunL0,     "Driver", "Char");
            InsertConfigNode(pLunL0,       "AttachedDriver", &pLunL1);
            InsertConfigString(pLunL1,     "Driver", "RawFile");
            InsertConfigNode(pLunL1,       "Config", &pLunL1Cfg);
            InsertConfigString(pLunL1Cfg,  "Location", pszPath);
        }
    }
    catch (ConfigError &x)
    {
        /* InsertConfig threw something */
        return x.m_vrc;
    }

    return VINF_SUCCESS;
}

