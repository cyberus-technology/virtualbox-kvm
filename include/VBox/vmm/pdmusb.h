/** @file
 * PDM - Pluggable Device Manager, USB Devices.
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
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef VBOX_INCLUDED_vmm_pdmusb_h
#define VBOX_INCLUDED_vmm_pdmusb_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/vmm/pdmqueue.h>
#include <VBox/vmm/pdmcritsect.h>
#include <VBox/vmm/pdmthread.h>
#include <VBox/vmm/pdmifs.h>
#include <VBox/vmm/pdmins.h>
#include <VBox/vmm/pdmcommon.h>
#include <VBox/vmm/tm.h>
#include <VBox/vmm/ssm.h>
#include <VBox/vmm/cfgm.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/mm.h>
#include <VBox/vusb.h>
#include <iprt/errcore.h>
#include <iprt/stdarg.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_pdm_usbdev    The USB Devices API
 * @ingroup grp_pdm
 * @{
 */


/**
 * A string entry for the USB descriptor cache.
 */
typedef struct PDMUSBDESCCACHESTRING
{
    /** The string index. */
    uint8_t     idx;
    /** The UTF-8 representation of the string. */
    const char *psz;
} PDMUSBDESCCACHESTRING;
/** Pointer to a const string entry. */
typedef PDMUSBDESCCACHESTRING const *PCPDMUSBDESCCACHESTRING;


/**
 * A language entry for the USB descriptor cache.
 */
typedef struct PDMUSBDESCCACHELANG
{
    /** The language ID for the strings in this block. */
    uint16_t                idLang;
    /** The number of strings in the array. */
    uint16_t                cStrings;
    /** Pointer to an array of associated strings.
     * This must be sorted in ascending order by string index as a binary lookup
     * will be performed. */
    PCPDMUSBDESCCACHESTRING paStrings;
} PDMUSBDESCCACHELANG;
/** Pointer to a const language entry. */
typedef PDMUSBDESCCACHELANG const *PCPDMUSBDESCCACHELANG;


/**
 * USB descriptor cache.
 *
 * This structure is owned by the USB device but provided to the PDM/VUSB layer
 * thru the PDMUSBREG::pfnGetDescriptorCache method.  PDM/VUSB will use the
 * information here to map addresses to endpoints, perform SET_CONFIGURATION
 * requests, and optionally perform GET_DESCRIPTOR requests (see flag).
 *
 * Currently, only device and configuration descriptors are cached.
 */
typedef struct PDMUSBDESCCACHE
{
    /** USB device descriptor */
    PCVUSBDESCDEVICE        pDevice;
    /** USB Descriptor arrays (pDev->bNumConfigurations) */
    PCVUSBDESCCONFIGEX      paConfigs;
    /** Language IDs and their associated strings.
     * This must be sorted in ascending order by language ID as a binary lookup
     * will be used. */
    PCPDMUSBDESCCACHELANG   paLanguages;
    /** The number of entries in the array pointed to by paLanguages. */
    uint16_t                cLanguages;
    /** Use the cached descriptors for GET_DESCRIPTOR requests. */
    bool                    fUseCachedDescriptors;
    /** Use the cached string descriptors. */
    bool                    fUseCachedStringsDescriptors;
} PDMUSBDESCCACHE;
/** Pointer to an USB descriptor cache. */
typedef PDMUSBDESCCACHE *PPDMUSBDESCCACHE;
/** Pointer to a const USB descriptor cache. */
typedef const PDMUSBDESCCACHE *PCPDMUSBDESCCACHE;


/** PDM Device Flags.
 * @{ */
/** A high-speed capable USB 2.0 device (also required to support full-speed). */
#define PDM_USBREG_HIGHSPEED_CAPABLE        RT_BIT(0)
/** Indicates that the device implements the saved state handlers. */
#define PDM_USBREG_SAVED_STATE_SUPPORTED    RT_BIT(1)
/** A SuperSpeed USB 3.0 device. */
#define PDM_USBREG_SUPERSPEED_CAPABLE       RT_BIT(2)
/** @} */

/** PDM USB Device Registration Structure,
 *
 * This structure is used when registering a device from VBoxUsbRegister() in HC Ring-3.
 * The PDM will make use of this structure until the VM is destroyed.
 */
typedef struct PDMUSBREG
{
    /** Structure version. PDM_DEVREG_VERSION defines the current version. */
    uint32_t            u32Version;
    /** Device name. */
    char                szName[32];
    /** The description of the device. The UTF-8 string pointed to shall, like this structure,
     * remain unchanged from registration till VM destruction. */
    const char         *pszDescription;

    /** Flags, combination of the PDM_USBREG_FLAGS_* \#defines. */
    RTUINT              fFlags;
    /** Maximum number of instances (per VM). */
    RTUINT              cMaxInstances;
    /** Size of the instance data. */
    RTUINT              cbInstance;


    /**
     * Construct an USB device instance for a VM.
     *
     * @returns VBox status.
     * @param   pUsbIns     The USB device instance data.
     *                      If the registration structure is needed, it will be
     *                      accessible thru pUsbDev->pReg.
     * @param   iInstance   Instance number. Use this to figure out which registers
     *                      and such to use. The instance number is also found in
     *                      pUsbDev->iInstance, but since it's likely to be
     *                      frequently used PDM passes it as parameter.
     * @param   pCfg        Configuration node handle for the device.  Use this to
     *                      obtain the configuration of the device instance.  It is
     *                      also found in pUsbDev->pCfg, but since it is primary
     *                      usage will in this function it is passed as a parameter.
     * @param   pCfgGlobal  Handle to the global device configuration.  Also found
     *                      in pUsbDev->pCfgGlobal.
     * @remarks This callback is required.
     */
    DECLR3CALLBACKMEMBER(int, pfnConstruct,(PPDMUSBINS pUsbIns, int iInstance, PCFGMNODE pCfg, PCFGMNODE pCfgGlobal));

    /**
     * Destruct an USB device instance.
     *
     * Most VM resources are freed by the VM. This callback is provided so that any non-VM
     * resources can be freed correctly.
     *
     * This method will be called regardless of the pfnConstruct result to avoid
     * complicated failure paths.
     *
     * @param   pUsbIns     The USB device instance data.
     * @remarks Optional.
     */
    DECLR3CALLBACKMEMBER(void, pfnDestruct,(PPDMUSBINS pUsbIns));


    /**
     * Init complete notification.
     *
     * This can be done to do communication with other devices and other
     * initialization which requires everything to be in place.
     *
     * @returns VBox status code.
     * @param   pUsbIns     The USB device instance data.
     * @remarks Optional.
     * @remarks Not called when hotplugged.
     */
    DECLR3CALLBACKMEMBER(int, pfnVMInitComplete,(PPDMUSBINS pUsbIns));

    /**
     * VM Power On notification.
     *
     * @param   pUsbIns     The USB device instance data.
     * @remarks Optional.
     */
    DECLR3CALLBACKMEMBER(void, pfnVMPowerOn,(PPDMUSBINS pUsbIns));

    /**
     * VM Reset notification.
     *
     * @param   pUsbIns     The USB device instance data.
     * @remarks Optional.
     */
    DECLR3CALLBACKMEMBER(void, pfnVMReset,(PPDMUSBINS pUsbIns));

    /**
     * VM Suspend notification.
     *
     * @param   pUsbIns     The USB device instance data.
     * @remarks Optional.
     */
    DECLR3CALLBACKMEMBER(void, pfnVMSuspend,(PPDMUSBINS pUsbIns));

    /**
     * VM Resume notification.
     *
     * This is not called when the device is hotplugged device, instead
     * pfnHotPlugged will be called.
     *
     * @param   pUsbIns     The USB device instance data.
     * @remarks Optional.
     */
    DECLR3CALLBACKMEMBER(void, pfnVMResume,(PPDMUSBINS pUsbIns));

    /**
     * VM Power Off notification.
     *
     * This is only called when the VMR3PowerOff call is made on a running VM.  This
     * means that there is no notification if the VM was suspended before being
     * powered of.  There will also be no callback when hot plugging devices.
     *
     * @param   pUsbIns     The USB device instance data.
     */
    DECLR3CALLBACKMEMBER(void, pfnVMPowerOff,(PPDMUSBINS pUsbIns));

    /**
     * Called after the constructor when attaching a device at run time.
     *
     * This can be used to do tasks normally assigned to pfnInitComplete and/or
     * pfnVMPowerOn.  There will not be a call to pfnVMResume following this.
     *
     * @param   pUsbIns     The USB device instance data.
     * @remarks Optional.
     */
    DECLR3CALLBACKMEMBER(void, pfnHotPlugged,(PPDMUSBINS pUsbIns));

    /**
     * Called before the destructor when a device is unplugged at run time.
     *
     * This can be used to do tasks normally assigned to pfnVMSuspend and/or pfnVMPowerOff.
     *
     * @param   pUsbIns     The USB device instance data.
     * @remarks Optional.
     */
    DECLR3CALLBACKMEMBER(void, pfnHotUnplugged,(PPDMUSBINS pUsbIns));
    /**
     * Driver Attach command.
     *
     * This is called to let the USB device attach to a driver for a specified LUN
     * at runtime. This is not called during VM construction, the device constructor
     * have to attach to all the available drivers.
     *
     * @returns VBox status code.
     * @param   pUsbIns     The USB device instance data.
     * @param   iLUN        The logical unit which is being detached.
     * @param   fFlags      Flags, combination of the PDM_TACH_FLAGS_* \#defines.
     * @remarks Optional.
     */
    DECLR3CALLBACKMEMBER(int, pfnDriverAttach,(PPDMUSBINS pUsbIns, unsigned iLUN, uint32_t fFlags));

    /**
     * Driver Detach notification.
     *
     * This is called when a driver is detaching itself from a LUN of the device.
     * The device should adjust it's state to reflect this.
     *
     * @param   pUsbIns     The USB device instance data.
     * @param   iLUN        The logical unit which is being detached.
     * @param   fFlags      Flags, combination of the PDM_TACH_FLAGS_* \#defines.
     * @remarks Optional.
     */
    DECLR3CALLBACKMEMBER(void, pfnDriverDetach,(PPDMUSBINS pUsbIns, unsigned iLUN, uint32_t fFlags));

    /**
     * Query the base interface of a logical unit.
     *
     * @returns VBox status code.
     * @param   pUsbIns     The USB device instance data.
     * @param   iLUN        The logicial unit to query.
     * @param   ppBase      Where to store the pointer to the base interface of the LUN.
     * @remarks Optional.
     */
    DECLR3CALLBACKMEMBER(int, pfnQueryInterface,(PPDMUSBINS pUsbIns, unsigned iLUN, PPDMIBASE *ppBase));

    /**
     * Requests the USB device to reset.
     *
     * @returns VBox status code.
     * @param   pUsbIns         The USB device instance.
     * @param   fResetOnLinux   A hint to the usb proxy.
     *                          Don't use this unless you're the linux proxy device.
     * @thread  Any thread.
     * @remarks Optional.
     */
    DECLR3CALLBACKMEMBER(int, pfnUsbReset,(PPDMUSBINS pUsbIns, bool fResetOnLinux));

    /**
     * Query device and configuration descriptors for the caching and servicing
     * relevant GET_DESCRIPTOR requests.
     *
     * @returns Pointer to the descriptor cache (read-only).
     * @param   pUsbIns             The USB device instance.
     * @remarks Mandatory.
     */
    DECLR3CALLBACKMEMBER(PCPDMUSBDESCCACHE, pfnUsbGetDescriptorCache,(PPDMUSBINS pUsbIns));

    /**
     * SET_CONFIGURATION request.
     *
     * @returns VBox status code.
     * @param   pUsbIns             The USB device instance.
     * @param   bConfigurationValue The bConfigurationValue of the new configuration.
     * @param   pvOldCfgDesc        Internal - for the device proxy.
     * @param   pvOldIfState        Internal - for the device proxy.
     * @param   pvNewCfgDesc        Internal - for the device proxy.
     * @remarks Optional.
     */
    DECLR3CALLBACKMEMBER(int, pfnUsbSetConfiguration,(PPDMUSBINS pUsbIns, uint8_t bConfigurationValue,
                                                      const void *pvOldCfgDesc, const void *pvOldIfState, const void *pvNewCfgDesc));

    /**
     * SET_INTERFACE request.
     *
     * @returns VBox status code.
     * @param   pUsbIns             The USB device instance.
     * @param   bInterfaceNumber    The interface number.
     * @param   bAlternateSetting   The alternate setting.
     * @remarks Optional.
     */
    DECLR3CALLBACKMEMBER(int, pfnUsbSetInterface,(PPDMUSBINS pUsbIns, uint8_t bInterfaceNumber, uint8_t bAlternateSetting));

    /**
     * Clears the halted state of an endpoint. (Optional)
     *
     * This called when VUSB sees a CLEAR_FEATURE(ENDPOINT_HALT) on request
     * on the zero pipe.
     *
     * @returns VBox status code.
     * @param   pUsbIns             The USB device instance.
     * @param   uEndpoint           The endpoint to clear.
     * @remarks Optional.
     */
    DECLR3CALLBACKMEMBER(int, pfnUsbClearHaltedEndpoint,(PPDMUSBINS pUsbIns, unsigned uEndpoint));

    /**
     * Allocates an URB.
     *
     * This can be used to make use of shared user/kernel mode buffers.
     *
     * @returns VBox status code.
     * @param   pUsbIns             The USB device instance.
     * @param   cbData              The size of the data buffer.
     * @param   cTds                The number of TDs.
     * @param   enmType             The type of URB.
     * @param   ppUrb               Where to store the allocated URB.
     * @remarks Optional.
     * @remarks Not implemented yet.
     */
    DECLR3CALLBACKMEMBER(int, pfnUrbNew,(PPDMUSBINS pUsbIns, size_t cbData, size_t cTds, VUSBXFERTYPE enmType, PVUSBURB *ppUrb));

    /**
     * Queues an URB for processing.
     *
     * @returns VBox status code.
     * @retval  VINF_SUCCESS on success.
     * @retval  VERR_VUSB_DEVICE_NOT_ATTACHED if the device has been disconnected.
     * @retval  VERR_VUSB_FAILED_TO_QUEUE_URB as a general failure kind of thing.
     * @retval  TBD - document new stuff!
     *
     * @param   pUsbIns             The USB device instance.
     * @param   pUrb                The URB to process.
     * @remarks Mandatory.
     */
    DECLR3CALLBACKMEMBER(int, pfnUrbQueue,(PPDMUSBINS pUsbIns, PVUSBURB pUrb));

    /**
     * Cancels an URB.
     *
     * @returns VBox status code.
     * @param   pUsbIns             The USB device instance.
     * @param   pUrb                The URB to cancel.
     * @remarks Mandatory.
     */
    DECLR3CALLBACKMEMBER(int, pfnUrbCancel,(PPDMUSBINS pUsbIns, PVUSBURB pUrb));

    /**
     * Reaps an URB.
     *
     * @returns A ripe URB, NULL if none.
     * @param   pUsbIns             The USB device instance.
     * @param   cMillies            How log to wait for an URB to become ripe.
     * @remarks Mandatory.
     */
    DECLR3CALLBACKMEMBER(PVUSBURB, pfnUrbReap,(PPDMUSBINS pUsbIns, RTMSINTERVAL cMillies));

    /**
     * Wakes a thread waiting in pfnUrbReap.
     *
     * @returns VBox status code.
     * @param   pUsbIns             The USB device instance.
     */
    DECLR3CALLBACKMEMBER(int, pfnWakeup,(PPDMUSBINS pUsbIns));

    /** Just some init precaution. Must be set to PDM_USBREG_VERSION. */
    uint32_t            u32TheEnd;
} PDMUSBREG;
/** Pointer to a PDM USB Device Structure. */
typedef PDMUSBREG *PPDMUSBREG;
/** Const pointer to a PDM USB Device Structure. */
typedef PDMUSBREG const *PCPDMUSBREG;

/** Current USBREG version number. */
#define PDM_USBREG_VERSION                      PDM_VERSION_MAKE(0xeeff, 2, 0)

/** PDM USB Device Flags.
 * @{ */
/* none yet */
/** @} */


#ifdef IN_RING3

/**
 * PDM USB Device API.
 */
typedef struct PDMUSBHLP
{
    /** Structure version. PDM_USBHLP_VERSION defines the current version. */
    uint32_t                    u32Version;

    /**
     * Attaches a driver (chain) to the USB device.
     *
     * The first call for a LUN this will serve as a registration of the LUN. The pBaseInterface and
     * the pszDesc string will be registered with that LUN and kept around for PDMR3QueryUSBDeviceLun().
     *
     * @returns VBox status code.
     * @param   pUsbIns             The USB device instance.
     * @param   iLun                The logical unit to attach.
     * @param   pBaseInterface      Pointer to the base interface for that LUN. (device side / down)
     * @param   ppBaseInterface     Where to store the pointer to the base interface. (driver side / up)
     * @param   pszDesc             Pointer to a string describing the LUN. This string must remain valid
     *                              for the live of the device instance.
     */
    DECLR3CALLBACKMEMBER(int, pfnDriverAttach,(PPDMUSBINS pUsbIns, RTUINT iLun, PPDMIBASE pBaseInterface, PPDMIBASE *ppBaseInterface, const char *pszDesc));

    /**
     * Assert that the current thread is the emulation thread.
     *
     * @returns True if correct.
     * @returns False if wrong.
     * @param   pUsbIns             The USB device instance.
     * @param   pszFile             Filename of the assertion location.
     * @param   iLine               Linenumber of the assertion location.
     * @param   pszFunction         Function of the assertion location.
     */
    DECLR3CALLBACKMEMBER(bool, pfnAssertEMT,(PPDMUSBINS pUsbIns, const char *pszFile, unsigned iLine, const char *pszFunction));

    /**
     * Assert that the current thread is NOT the emulation thread.
     *
     * @returns True if correct.
     * @returns False if wrong.
     * @param   pUsbIns             The USB device instance.
     * @param   pszFile             Filename of the assertion location.
     * @param   iLine               Linenumber of the assertion location.
     * @param   pszFunction         Function of the assertion location.
     */
    DECLR3CALLBACKMEMBER(bool, pfnAssertOther,(PPDMUSBINS pUsbIns, const char *pszFile, unsigned iLine, const char *pszFunction));

    /**
     * Stops the VM and enters the debugger to look at the guest state.
     *
     * Use the PDMUsbDBGFStop() inline function with the RT_SRC_POS macro instead of
     * invoking this function directly.
     *
     * @returns VBox status code which must be passed up to the VMM.
     * @param   pUsbIns             The USB device instance.
     * @param   pszFile             Filename of the assertion location.
     * @param   iLine               The linenumber of the assertion location.
     * @param   pszFunction         Function of the assertion location.
     * @param   pszFormat           Message. (optional)
     * @param   va                  Message parameters.
     */
    DECLR3CALLBACKMEMBER(int, pfnDBGFStopV,(PPDMUSBINS pUsbIns, const char *pszFile, unsigned iLine, const char *pszFunction,
                                            const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(5, 0));

    /**
     * Register a info handler with DBGF, argv style.
     *
     * @returns VBox status code.
     * @param   pUsbIns             The USB device instance.
     * @param   pszName             The identifier of the info.
     * @param   pszDesc             The description of the info and any arguments the handler may take.
     * @param   pfnHandler          The handler function to be called to display the info.
     */
    DECLR3CALLBACKMEMBER(int, pfnDBGFInfoRegisterArgv,(PPDMUSBINS pUsbIns, const char *pszName, const char *pszDesc, PFNDBGFINFOARGVUSB pfnHandler));

    /**
     * Allocate memory which is associated with current VM instance
     * and automatically freed on it's destruction.
     *
     * @returns Pointer to allocated memory. The memory is *NOT* zero-ed.
     * @param   pUsbIns             The USB device instance.
     * @param   cb                  Number of bytes to allocate.
     */
    DECLR3CALLBACKMEMBER(void *, pfnMMHeapAlloc,(PPDMUSBINS pUsbIns, size_t cb));

    /**
     * Allocate memory which is associated with current VM instance
     * and automatically freed on it's destruction. The memory is ZEROed.
     *
     * @returns Pointer to allocated memory. The memory is *NOT* zero-ed.
     * @param   pUsbIns             The USB device instance.
     * @param   cb                  Number of bytes to allocate.
     */
    DECLR3CALLBACKMEMBER(void *, pfnMMHeapAllocZ,(PPDMUSBINS pUsbIns, size_t cb));

    /**
     * Free memory allocated with pfnMMHeapAlloc() and pfnMMHeapAllocZ().
     *
     * @param   pUsbIns             The USB device instance.
     * @param   pv                  Pointer to the memory to free.
     */
    DECLR3CALLBACKMEMBER(void, pfnMMHeapFree,(PPDMUSBINS pUsbIns, void *pv));

    /**
     * Create a queue.
     *
     * @returns VBox status code.
     * @param   pUsbIns             The USB device instance.
     * @param   cbItem              Size a queue item.
     * @param   cItems              Number of items in the queue.
     * @param   cMilliesInterval    Number of milliseconds between polling the queue.
     *                              If 0 then the emulation thread will be notified whenever an item arrives.
     * @param   pfnCallback         The consumer function.
     * @param   pszName             The queue base name. The instance number will be
     *                              appended automatically.
     * @param   ppQueue             Where to store the queue handle on success.
     * @thread  The emulation thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnPDMQueueCreate,(PPDMUSBINS pUsbIns, RTUINT cbItem, RTUINT cItems, uint32_t cMilliesInterval,
                                                 PFNPDMQUEUEUSB pfnCallback, const char *pszName, PPDMQUEUE *ppQueue));

    /**
     * Register a save state data unit.
     *
     * @returns VBox status.
     * @param   pUsbIns             The USB device instance.
     * @param   uVersion            Data layout version number.
     * @param   cbGuess             The approximate amount of data in the unit.
     *                              Only for progress indicators.
     *
     * @param   pfnLivePrep         Prepare live save callback, optional.
     * @param   pfnLiveExec         Execute live save callback, optional.
     * @param   pfnLiveVote         Vote live save callback, optional.
     *
     * @param   pfnSavePrep         Prepare save callback, optional.
     * @param   pfnSaveExec         Execute save callback, optional.
     * @param   pfnSaveDone         Done save callback, optional.
     *
     * @param   pfnLoadPrep         Prepare load callback, optional.
     * @param   pfnLoadExec         Execute load callback, optional.
     * @param   pfnLoadDone         Done load callback, optional.
     */
    DECLR3CALLBACKMEMBER(int, pfnSSMRegister,(PPDMUSBINS pUsbIns, uint32_t uVersion, size_t cbGuess,
                                              PFNSSMUSBLIVEPREP pfnLivePrep, PFNSSMUSBLIVEEXEC pfnLiveExec, PFNSSMUSBLIVEVOTE pfnLiveVote,
                                              PFNSSMUSBSAVEPREP pfnSavePrep, PFNSSMUSBSAVEEXEC pfnSaveExec, PFNSSMUSBSAVEDONE pfnSaveDone,
                                              PFNSSMUSBLOADPREP pfnLoadPrep, PFNSSMUSBLOADEXEC pfnLoadExec, PFNSSMUSBLOADDONE pfnLoadDone));

    /** @name Exported SSM Functions
     * @{ */
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutStruct,(PSSMHANDLE pSSM, const void *pvStruct, PCSSMFIELD paFields));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutStructEx,(PSSMHANDLE pSSM, const void *pvStruct, size_t cbStruct, uint32_t fFlags, PCSSMFIELD paFields, void *pvUser));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutBool,(PSSMHANDLE pSSM, bool fBool));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutU8,(PSSMHANDLE pSSM, uint8_t u8));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutS8,(PSSMHANDLE pSSM, int8_t i8));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutU16,(PSSMHANDLE pSSM, uint16_t u16));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutS16,(PSSMHANDLE pSSM, int16_t i16));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutU32,(PSSMHANDLE pSSM, uint32_t u32));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutS32,(PSSMHANDLE pSSM, int32_t i32));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutU64,(PSSMHANDLE pSSM, uint64_t u64));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutS64,(PSSMHANDLE pSSM, int64_t i64));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutU128,(PSSMHANDLE pSSM, uint128_t u128));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutS128,(PSSMHANDLE pSSM, int128_t i128));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutUInt,(PSSMHANDLE pSSM, RTUINT u));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutSInt,(PSSMHANDLE pSSM, RTINT i));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutGCUInt,(PSSMHANDLE pSSM, RTGCUINT u));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutGCUIntReg,(PSSMHANDLE pSSM, RTGCUINTREG u));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutGCPhys32,(PSSMHANDLE pSSM, RTGCPHYS32 GCPhys));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutGCPhys64,(PSSMHANDLE pSSM, RTGCPHYS64 GCPhys));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutGCPhys,(PSSMHANDLE pSSM, RTGCPHYS GCPhys));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutGCPtr,(PSSMHANDLE pSSM, RTGCPTR GCPtr));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutGCUIntPtr,(PSSMHANDLE pSSM, RTGCUINTPTR GCPtr));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutRCPtr,(PSSMHANDLE pSSM, RTRCPTR RCPtr));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutIOPort,(PSSMHANDLE pSSM, RTIOPORT IOPort));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutSel,(PSSMHANDLE pSSM, RTSEL Sel));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutMem,(PSSMHANDLE pSSM, const void *pv, size_t cb));
    DECLR3CALLBACKMEMBER(int,      pfnSSMPutStrZ,(PSSMHANDLE pSSM, const char *psz));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetStruct,(PSSMHANDLE pSSM, void *pvStruct, PCSSMFIELD paFields));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetStructEx,(PSSMHANDLE pSSM, void *pvStruct, size_t cbStruct, uint32_t fFlags, PCSSMFIELD paFields, void *pvUser));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetBool,(PSSMHANDLE pSSM, bool *pfBool));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetBoolV,(PSSMHANDLE pSSM, bool volatile *pfBool));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetU8,(PSSMHANDLE pSSM, uint8_t *pu8));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetU8V,(PSSMHANDLE pSSM, uint8_t volatile *pu8));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetS8,(PSSMHANDLE pSSM, int8_t *pi8));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetS8V,(PSSMHANDLE pSSM, int8_t volatile *pi8));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetU16,(PSSMHANDLE pSSM, uint16_t *pu16));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetU16V,(PSSMHANDLE pSSM, uint16_t volatile *pu16));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetS16,(PSSMHANDLE pSSM, int16_t *pi16));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetS16V,(PSSMHANDLE pSSM, int16_t volatile *pi16));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetU32,(PSSMHANDLE pSSM, uint32_t *pu32));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetU32V,(PSSMHANDLE pSSM, uint32_t volatile *pu32));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetS32,(PSSMHANDLE pSSM, int32_t *pi32));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetS32V,(PSSMHANDLE pSSM, int32_t volatile *pi32));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetU64,(PSSMHANDLE pSSM, uint64_t *pu64));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetU64V,(PSSMHANDLE pSSM, uint64_t volatile *pu64));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetS64,(PSSMHANDLE pSSM, int64_t *pi64));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetS64V,(PSSMHANDLE pSSM, int64_t volatile *pi64));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetU128,(PSSMHANDLE pSSM, uint128_t *pu128));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetU128V,(PSSMHANDLE pSSM, uint128_t volatile *pu128));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetS128,(PSSMHANDLE pSSM, int128_t *pi128));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetS128V,(PSSMHANDLE pSSM, int128_t  volatile *pi128));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetGCPhys32,(PSSMHANDLE pSSM, PRTGCPHYS32 pGCPhys));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetGCPhys32V,(PSSMHANDLE pSSM, RTGCPHYS32 volatile *pGCPhys));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetGCPhys64,(PSSMHANDLE pSSM, PRTGCPHYS64 pGCPhys));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetGCPhys64V,(PSSMHANDLE pSSM, RTGCPHYS64 volatile *pGCPhys));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetGCPhys,(PSSMHANDLE pSSM, PRTGCPHYS pGCPhys));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetGCPhysV,(PSSMHANDLE pSSM, RTGCPHYS volatile *pGCPhys));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetUInt,(PSSMHANDLE pSSM, PRTUINT pu));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetSInt,(PSSMHANDLE pSSM, PRTINT pi));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetGCUInt,(PSSMHANDLE pSSM, PRTGCUINT pu));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetGCUIntReg,(PSSMHANDLE pSSM, PRTGCUINTREG pu));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetGCPtr,(PSSMHANDLE pSSM, PRTGCPTR pGCPtr));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetGCUIntPtr,(PSSMHANDLE pSSM, PRTGCUINTPTR pGCPtr));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetRCPtr,(PSSMHANDLE pSSM, PRTRCPTR pRCPtr));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetIOPort,(PSSMHANDLE pSSM, PRTIOPORT pIOPort));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetSel,(PSSMHANDLE pSSM, PRTSEL pSel));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetMem,(PSSMHANDLE pSSM, void *pv, size_t cb));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetStrZ,(PSSMHANDLE pSSM, char *psz, size_t cbMax));
    DECLR3CALLBACKMEMBER(int,      pfnSSMGetStrZEx,(PSSMHANDLE pSSM, char *psz, size_t cbMax, size_t *pcbStr));
    DECLR3CALLBACKMEMBER(int,      pfnSSMSkip,(PSSMHANDLE pSSM, size_t cb));
    DECLR3CALLBACKMEMBER(int,      pfnSSMSkipToEndOfUnit,(PSSMHANDLE pSSM));
    DECLR3CALLBACKMEMBER(int,      pfnSSMSetLoadError,(PSSMHANDLE pSSM, int rc, RT_SRC_POS_DECL, const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(6, 7));
    DECLR3CALLBACKMEMBER(int,      pfnSSMSetLoadErrorV,(PSSMHANDLE pSSM, int rc, RT_SRC_POS_DECL, const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(6, 0));
    DECLR3CALLBACKMEMBER(int,      pfnSSMSetCfgError,(PSSMHANDLE pSSM, RT_SRC_POS_DECL, const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(5, 6));
    DECLR3CALLBACKMEMBER(int,      pfnSSMSetCfgErrorV,(PSSMHANDLE pSSM, RT_SRC_POS_DECL, const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(5, 0));
    DECLR3CALLBACKMEMBER(int,      pfnSSMHandleGetStatus,(PSSMHANDLE pSSM));
    DECLR3CALLBACKMEMBER(SSMAFTER, pfnSSMHandleGetAfter,(PSSMHANDLE pSSM));
    DECLR3CALLBACKMEMBER(bool,     pfnSSMHandleIsLiveSave,(PSSMHANDLE pSSM));
    DECLR3CALLBACKMEMBER(uint32_t, pfnSSMHandleMaxDowntime,(PSSMHANDLE pSSM));
    DECLR3CALLBACKMEMBER(uint32_t, pfnSSMHandleHostBits,(PSSMHANDLE pSSM));
    DECLR3CALLBACKMEMBER(uint32_t, pfnSSMHandleRevision,(PSSMHANDLE pSSM));
    DECLR3CALLBACKMEMBER(uint32_t, pfnSSMHandleVersion,(PSSMHANDLE pSSM));
    DECLR3CALLBACKMEMBER(const char *, pfnSSMHandleHostOSAndArch,(PSSMHANDLE pSSM));
    /** @} */

    /** @name Exported CFGM Functions.
     * @{ */
    DECLR3CALLBACKMEMBER(bool,      pfnCFGMExists,(           PCFGMNODE pNode, const char *pszName));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryType,(        PCFGMNODE pNode, const char *pszName, PCFGMVALUETYPE penmType));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQuerySize,(        PCFGMNODE pNode, const char *pszName, size_t *pcb));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryInteger,(     PCFGMNODE pNode, const char *pszName, uint64_t *pu64));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryIntegerDef,(  PCFGMNODE pNode, const char *pszName, uint64_t *pu64, uint64_t u64Def));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryString,(      PCFGMNODE pNode, const char *pszName, char *pszString, size_t cchString));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryStringDef,(   PCFGMNODE pNode, const char *pszName, char *pszString, size_t cchString, const char *pszDef));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryBytes,(       PCFGMNODE pNode, const char *pszName, void *pvData, size_t cbData));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryU64,(         PCFGMNODE pNode, const char *pszName, uint64_t *pu64));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryU64Def,(      PCFGMNODE pNode, const char *pszName, uint64_t *pu64, uint64_t u64Def));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryS64,(         PCFGMNODE pNode, const char *pszName, int64_t *pi64));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryS64Def,(      PCFGMNODE pNode, const char *pszName, int64_t *pi64, int64_t i64Def));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryU32,(         PCFGMNODE pNode, const char *pszName, uint32_t *pu32));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryU32Def,(      PCFGMNODE pNode, const char *pszName, uint32_t *pu32, uint32_t u32Def));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryS32,(         PCFGMNODE pNode, const char *pszName, int32_t *pi32));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryS32Def,(      PCFGMNODE pNode, const char *pszName, int32_t *pi32, int32_t i32Def));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryU16,(         PCFGMNODE pNode, const char *pszName, uint16_t *pu16));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryU16Def,(      PCFGMNODE pNode, const char *pszName, uint16_t *pu16, uint16_t u16Def));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryS16,(         PCFGMNODE pNode, const char *pszName, int16_t *pi16));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryS16Def,(      PCFGMNODE pNode, const char *pszName, int16_t *pi16, int16_t i16Def));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryU8,(          PCFGMNODE pNode, const char *pszName, uint8_t *pu8));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryU8Def,(       PCFGMNODE pNode, const char *pszName, uint8_t *pu8, uint8_t u8Def));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryS8,(          PCFGMNODE pNode, const char *pszName, int8_t *pi8));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryS8Def,(       PCFGMNODE pNode, const char *pszName, int8_t *pi8, int8_t i8Def));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryBool,(        PCFGMNODE pNode, const char *pszName, bool *pf));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryBoolDef,(     PCFGMNODE pNode, const char *pszName, bool *pf, bool fDef));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryPort,(        PCFGMNODE pNode, const char *pszName, PRTIOPORT pPort));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryPortDef,(     PCFGMNODE pNode, const char *pszName, PRTIOPORT pPort, RTIOPORT PortDef));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryUInt,(        PCFGMNODE pNode, const char *pszName, unsigned int *pu));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryUIntDef,(     PCFGMNODE pNode, const char *pszName, unsigned int *pu, unsigned int uDef));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQuerySInt,(        PCFGMNODE pNode, const char *pszName, signed int *pi));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQuerySIntDef,(     PCFGMNODE pNode, const char *pszName, signed int *pi, signed int iDef));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryGCPtr,(       PCFGMNODE pNode, const char *pszName, PRTGCPTR pGCPtr));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryGCPtrDef,(    PCFGMNODE pNode, const char *pszName, PRTGCPTR pGCPtr, RTGCPTR GCPtrDef));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryGCPtrU,(      PCFGMNODE pNode, const char *pszName, PRTGCUINTPTR pGCPtr));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryGCPtrUDef,(   PCFGMNODE pNode, const char *pszName, PRTGCUINTPTR pGCPtr, RTGCUINTPTR GCPtrDef));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryGCPtrS,(      PCFGMNODE pNode, const char *pszName, PRTGCINTPTR pGCPtr));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryGCPtrSDef,(   PCFGMNODE pNode, const char *pszName, PRTGCINTPTR pGCPtr, RTGCINTPTR GCPtrDef));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryStringAlloc,( PCFGMNODE pNode, const char *pszName, char **ppszString));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryStringAllocDef,(PCFGMNODE pNode, const char *pszName, char **ppszString, const char *pszDef));
    DECLR3CALLBACKMEMBER(PCFGMNODE, pfnCFGMGetParent,(PCFGMNODE pNode));
    DECLR3CALLBACKMEMBER(PCFGMNODE, pfnCFGMGetChild,(PCFGMNODE pNode, const char *pszPath));
    DECLR3CALLBACKMEMBER(PCFGMNODE, pfnCFGMGetChildF,(PCFGMNODE pNode, const char *pszPathFormat, ...) RT_IPRT_FORMAT_ATTR(2, 3));
    DECLR3CALLBACKMEMBER(PCFGMNODE, pfnCFGMGetChildFV,(PCFGMNODE pNode, const char *pszPathFormat, va_list Args) RT_IPRT_FORMAT_ATTR(3, 0));
    DECLR3CALLBACKMEMBER(PCFGMNODE, pfnCFGMGetFirstChild,(PCFGMNODE pNode));
    DECLR3CALLBACKMEMBER(PCFGMNODE, pfnCFGMGetNextChild,(PCFGMNODE pCur));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMGetName,(PCFGMNODE pCur, char *pszName, size_t cchName));
    DECLR3CALLBACKMEMBER(size_t,    pfnCFGMGetNameLen,(PCFGMNODE pCur));
    DECLR3CALLBACKMEMBER(bool,      pfnCFGMAreChildrenValid,(PCFGMNODE pNode, const char *pszzValid));
    DECLR3CALLBACKMEMBER(PCFGMLEAF, pfnCFGMGetFirstValue,(PCFGMNODE pCur));
    DECLR3CALLBACKMEMBER(PCFGMLEAF, pfnCFGMGetNextValue,(PCFGMLEAF pCur));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMGetValueName,(PCFGMLEAF pCur, char *pszName, size_t cchName));
    DECLR3CALLBACKMEMBER(size_t,    pfnCFGMGetValueNameLen,(PCFGMLEAF pCur));
    DECLR3CALLBACKMEMBER(CFGMVALUETYPE, pfnCFGMGetValueType,(PCFGMLEAF pCur));
    DECLR3CALLBACKMEMBER(bool,      pfnCFGMAreValuesValid,(PCFGMNODE pNode, const char *pszzValid));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMValidateConfig,(PCFGMNODE pNode, const char *pszNode,
                                                           const char *pszValidValues, const char *pszValidNodes,
                                                           const char *pszWho, uint32_t uInstance));
    /** @} */

    /**
     * Register a STAM sample.
     *
     * Use the PDMUsbHlpSTAMRegister wrapper.
     *
     * @param   pUsbIns             The USB device instance.
     * @param   pvSample            Pointer to the sample.
     * @param   enmType             Sample type. This indicates what pvSample is pointing at.
     * @param   enmVisibility       Visibility type specifying whether unused statistics should be visible or not.
     * @param   enmUnit             Sample unit.
     * @param   pszDesc             Sample description.
     * @param   pszName             The sample name format string.
     * @param   va                  Arguments to the format string.
     */
    DECLR3CALLBACKMEMBER(void, pfnSTAMRegisterV,(PPDMUSBINS pUsbIns, void *pvSample, STAMTYPE enmType,
                                                 STAMVISIBILITY enmVisibility, STAMUNIT enmUnit, const char *pszDesc,
                                                 const char *pszName, va_list va)  RT_IPRT_FORMAT_ATTR(7, 0));

    /**
     * Creates a timer.
     *
     * @returns VBox status.
     * @param   pUsbIns             The USB device instance.
     * @param   enmClock            The clock to use on this timer.
     * @param   pfnCallback         Callback function.
     * @param   pvUser              User argument for the callback.
     * @param   fFlags              Flags, see TMTIMER_FLAGS_*.
     * @param   pszDesc             Pointer to description string which must stay around
     *                              until the timer is fully destroyed (i.e. a bit after TMTimerDestroy()).
     * @param   phTimer             Where to store the timer handle on success.
     */
    DECLR3CALLBACKMEMBER(int, pfnTimerCreate,(PPDMUSBINS pUsbIns, TMCLOCK enmClock, PFNTMTIMERUSB pfnCallback, void *pvUser,
                                              uint32_t fFlags, const char *pszDesc, PTMTIMERHANDLE phTimer));

    /** @name Timer handle method wrappers
     * @{ */
    DECLR3CALLBACKMEMBER(uint64_t, pfnTimerFromMicro,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, uint64_t cMicroSecs));
    DECLR3CALLBACKMEMBER(uint64_t, pfnTimerFromMilli,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, uint64_t cMilliSecs));
    DECLR3CALLBACKMEMBER(uint64_t, pfnTimerFromNano,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, uint64_t cNanoSecs));
    DECLR3CALLBACKMEMBER(uint64_t, pfnTimerGet,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer));
    DECLR3CALLBACKMEMBER(uint64_t, pfnTimerGetFreq,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer));
    DECLR3CALLBACKMEMBER(uint64_t, pfnTimerGetNano,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer));
    DECLR3CALLBACKMEMBER(bool,     pfnTimerIsActive,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer));
    DECLR3CALLBACKMEMBER(bool,     pfnTimerIsLockOwner,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer));
    DECLR3CALLBACKMEMBER(int,      pfnTimerLockClock,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer));
    /** Takes the clock lock then enters the specified critical section. */
    DECLR3CALLBACKMEMBER(int,      pfnTimerLockClock2,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, PPDMCRITSECT pCritSect));
    DECLR3CALLBACKMEMBER(int,      pfnTimerSet,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, uint64_t uExpire));
    DECLR3CALLBACKMEMBER(int,      pfnTimerSetFrequencyHint,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, uint32_t uHz));
    DECLR3CALLBACKMEMBER(int,      pfnTimerSetMicro,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, uint64_t cMicrosToNext));
    DECLR3CALLBACKMEMBER(int,      pfnTimerSetMillies,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, uint64_t cMilliesToNext));
    DECLR3CALLBACKMEMBER(int,      pfnTimerSetNano,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, uint64_t cNanosToNext));
    DECLR3CALLBACKMEMBER(int,      pfnTimerSetRelative,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, uint64_t cTicksToNext, uint64_t *pu64Now));
    DECLR3CALLBACKMEMBER(int,      pfnTimerStop,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer));
    DECLR3CALLBACKMEMBER(void,     pfnTimerUnlockClock,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer));
    DECLR3CALLBACKMEMBER(void,     pfnTimerUnlockClock2,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, PPDMCRITSECT pCritSect));
    DECLR3CALLBACKMEMBER(int,      pfnTimerSetCritSect,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, PPDMCRITSECT pCritSect));
    DECLR3CALLBACKMEMBER(int,      pfnTimerSave,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, PSSMHANDLE pSSM));
    DECLR3CALLBACKMEMBER(int,      pfnTimerLoad,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, PSSMHANDLE pSSM));
    DECLR3CALLBACKMEMBER(int,      pfnTimerDestroy,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer));
    /** @sa TMR3TimerSkip */
    DECLR3CALLBACKMEMBER(int,      pfnTimerSkipLoad,(PSSMHANDLE pSSM, bool *pfActive));
    /** @} */

    /**
     * Set the VM error message
     *
     * @returns rc.
     * @param   pUsbIns             The USB device instance.
     * @param   rc                  VBox status code.
     * @param   SRC_POS             Use RT_SRC_POS.
     * @param   pszFormat           Error message format string.
     * @param   va                  Error message arguments.
     */
    DECLR3CALLBACKMEMBER(int, pfnVMSetErrorV,(PPDMUSBINS pUsbIns, int rc, RT_SRC_POS_DECL,
                                              const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(6, 0));

    /**
     * Set the VM runtime error message
     *
     * @returns VBox status code.
     * @param   pUsbIns             The USB device instance.
     * @param   fFlags              The action flags. See VMSETRTERR_FLAGS_*.
     * @param   pszErrorId          Error ID string.
     * @param   pszFormat           Error message format string.
     * @param   va                  Error message arguments.
     */
    DECLR3CALLBACKMEMBER(int, pfnVMSetRuntimeErrorV,(PPDMUSBINS pUsbIns, uint32_t fFlags, const char *pszErrorId,
                                                     const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(4, 0));

    /**
     * Gets the VM state.
     *
     * @returns VM state.
     * @param   pUsbIns             The USB device instance.
     * @thread  Any thread (just keep in mind that it's volatile info).
     */
    DECLR3CALLBACKMEMBER(VMSTATE, pfnVMState, (PPDMUSBINS pUsbIns));

    /**
     * Creates a PDM thread.
     *
     * This differs from the RTThreadCreate() API in that PDM takes care of suspending,
     * resuming, and destroying the thread as the VM state changes.
     *
     * @returns VBox status code.
     * @param   pUsbIns             The USB device instance.
     * @param   ppThread            Where to store the thread 'handle'.
     * @param   pvUser              The user argument to the thread function.
     * @param   pfnThread           The thread function.
     * @param   pfnWakeup           The wakup callback. This is called on the EMT
     *                              thread when a state change is pending.
     * @param   cbStack             See RTThreadCreate.
     * @param   enmType             See RTThreadCreate.
     * @param   pszName             See RTThreadCreate.
     */
    DECLR3CALLBACKMEMBER(int, pfnThreadCreate,(PPDMUSBINS pUsbIns, PPPDMTHREAD ppThread, void *pvUser, PFNPDMTHREADUSB pfnThread,
                                               PFNPDMTHREADWAKEUPUSB pfnWakeup, size_t cbStack, RTTHREADTYPE enmType, const char *pszName));

    /** @name Exported PDM Thread Functions
     * @{ */
    DECLR3CALLBACKMEMBER(int, pfnThreadDestroy,(PPDMTHREAD pThread, int *pRcThread));
    DECLR3CALLBACKMEMBER(int, pfnThreadIAmSuspending,(PPDMTHREAD pThread));
    DECLR3CALLBACKMEMBER(int, pfnThreadIAmRunning,(PPDMTHREAD pThread));
    DECLR3CALLBACKMEMBER(int, pfnThreadSleep,(PPDMTHREAD pThread, RTMSINTERVAL cMillies));
    DECLR3CALLBACKMEMBER(int, pfnThreadSuspend,(PPDMTHREAD pThread));
    DECLR3CALLBACKMEMBER(int, pfnThreadResume,(PPDMTHREAD pThread));
    /** @} */

    /**
     * Set up asynchronous handling of a suspend, reset or power off notification.
     *
     * This shall only be called when getting the notification.  It must be called
     * for each one.
     *
     * @returns VBox status code.
     * @param   pUsbIns             The USB device instance.
     * @param   pfnAsyncNotify      The callback.
     * @thread  EMT(0)
     */
    DECLR3CALLBACKMEMBER(int, pfnSetAsyncNotification, (PPDMUSBINS pUSbIns, PFNPDMUSBASYNCNOTIFY pfnAsyncNotify));

    /**
     * Notify EMT(0) that the device has completed the asynchronous notification
     * handling.
     *
     * This can be called at any time, spurious calls will simply be ignored.
     *
     * @param   pUsbIns             The USB device instance.
     * @thread  Any
     */
    DECLR3CALLBACKMEMBER(void, pfnAsyncNotificationCompleted, (PPDMUSBINS pUsbIns));

    /**
     * Gets the reason for the most recent VM suspend.
     *
     * @returns The suspend reason. VMSUSPENDREASON_INVALID is returned if no
     *          suspend has been made or if the pUsbIns is invalid.
     * @param   pUsbIns             The driver instance.
     */
    DECLR3CALLBACKMEMBER(VMSUSPENDREASON, pfnVMGetSuspendReason,(PPDMUSBINS pUsbIns));

    /**
     * Gets the reason for the most recent VM resume.
     *
     * @returns The resume reason. VMRESUMEREASON_INVALID is returned if no
     *          resume has been made or if the pUsbIns is invalid.
     * @param   pUsbIns             The driver instance.
     */
    DECLR3CALLBACKMEMBER(VMRESUMEREASON, pfnVMGetResumeReason,(PPDMUSBINS pUsbIns));

    /**
     * Queries a generic object from the VMM user.
     *
     * @returns Pointer to the object if found, NULL if not.
     * @param   pUsbIns             The USB device instance.
     * @param   pUuid               The UUID of what's being queried.  The UUIDs and
     *                              the usage conventions are defined by the user.
     */
    DECLR3CALLBACKMEMBER(void *, pfnQueryGenericUserObject,(PPDMUSBINS pUsbIns, PCRTUUID pUuid));

    /** @name Space reserved for minor interface changes.
     * @{ */
    DECLR3CALLBACKMEMBER(void, pfnReserved0,(PPDMUSBINS pUsbIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved1,(PPDMUSBINS pUsbIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved2,(PPDMUSBINS pUsbIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved3,(PPDMUSBINS pUsbIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved4,(PPDMUSBINS pUsbIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved5,(PPDMUSBINS pUsbIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved6,(PPDMUSBINS pUsbIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved7,(PPDMUSBINS pUsbIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved8,(PPDMUSBINS pUsbIns));
    /** @}  */

    /** Just a safety precaution. */
    uint32_t                        u32TheEnd;
} PDMUSBHLP;
/** Pointer PDM USB Device API. */
typedef PDMUSBHLP *PPDMUSBHLP;
/** Pointer const PDM USB Device API. */
typedef const PDMUSBHLP *PCPDMUSBHLP;

/** Current USBHLP version number. */
#define PDM_USBHLP_VERSION                      PDM_VERSION_MAKE(0xeefe, 7, 0)

#endif /* IN_RING3 */

/**
 * PDM USB Device Instance.
 */
typedef struct PDMUSBINS
{
    /** Structure version. PDM_USBINS_VERSION defines the current version. */
    uint32_t                    u32Version;
    /** USB device instance number. */
    uint32_t                    iInstance;
    /** The base interface of the device.
     * The device constructor initializes this if it has any device level
     * interfaces to export. To obtain this interface call PDMR3QueryUSBDevice(). */
    PDMIBASE                    IBase;
#if HC_ARCH_BITS == 32
    uint32_t                    u32Alignment; /**< Alignment padding. */
#endif

    /** Internal data. */
    union
    {
#ifdef PDMUSBINSINT_DECLARED
        PDMUSBINSINT            s;
#endif
        uint8_t                 padding[HC_ARCH_BITS == 32 ? 96 : 128];
    } Internal;

    /** Pointer the PDM USB Device API. */
    R3PTRTYPE(PCPDMUSBHLP)      pHlpR3;
    /** Pointer to the USB device registration structure.  */
    R3PTRTYPE(PCPDMUSBREG)      pReg;
    /** Configuration handle. */
    R3PTRTYPE(PCFGMNODE)        pCfg;
    /** The (device) global configuration handle. */
    R3PTRTYPE(PCFGMNODE)        pCfgGlobal;
    /** Pointer to device instance data. */
    R3PTRTYPE(void *)           pvInstanceDataR3;
    /** Pointer to the VUSB Device structure.
     * Internal to VUSB, don't touch.
     * @todo Moved this to PDMUSBINSINT. */
    R3PTRTYPE(void *)           pvVUsbDev2;
    /** Device name for using when logging.
     * The constructor sets this and the destructor frees it. */
    R3PTRTYPE(char *)           pszName;
    /** Tracing indicator. */
    uint32_t                    fTracing;
    /** The tracing ID of this device.  */
    uint32_t                    idTracing;
    /** The port/device speed. HCs and emulated devices need to know. */
    VUSBSPEED                   enmSpeed;

    /** Padding to make achInstanceData aligned at 32 byte boundary. */
    uint32_t                    au32Padding[HC_ARCH_BITS == 32 ? 2 : 3];

    /** Device instance data. The size of this area is defined
     * in the PDMUSBREG::cbInstanceData field. */
    char                        achInstanceData[8];
} PDMUSBINS;

/** Current USBINS version number. */
#define PDM_USBINS_VERSION                      PDM_VERSION_MAKE(0xeefd, 3, 0)

/**
 * Checks the structure versions of the USB device instance and USB device
 * helpers, returning if they are incompatible.
 *
 * This shall be the first statement of the constructor!
 *
 * @param   pUsbIns     The USB device instance pointer.
 */
#define PDMUSB_CHECK_VERSIONS_RETURN(pUsbIns) \
    do \
    { \
        PPDMUSBINS pUsbInsTypeCheck = (pUsbIns); NOREF(pUsbInsTypeCheck); \
        AssertLogRelMsgReturn(PDM_VERSION_ARE_COMPATIBLE((pUsbIns)->u32Version, PDM_USBINS_VERSION), \
                              ("DevIns=%#x  mine=%#x\n", (pUsbIns)->u32Version, PDM_USBINS_VERSION), \
                              VERR_PDM_USBINS_VERSION_MISMATCH); \
        AssertLogRelMsgReturn(PDM_VERSION_ARE_COMPATIBLE((pUsbIns)->pHlpR3->u32Version, PDM_USBHLP_VERSION), \
                              ("DevHlp=%#x  mine=%#x\n", (pUsbIns)->pHlpR3->u32Version, PDM_USBHLP_VERSION), \
                              VERR_PDM_USBHLPR3_VERSION_MISMATCH); \
    } while (0)

/**
 * Quietly checks the structure versions of the USB device instance and
 * USB device helpers, returning if they are incompatible.
 *
 * This shall be invoked as the first statement in the destructor!
 *
 * @param   pUsbIns     The USB device instance pointer.
 */
#define PDMUSB_CHECK_VERSIONS_RETURN_VOID(pUsbIns) \
    do \
    { \
        PPDMUSBINS pUsbInsTypeCheck = (pUsbIns); NOREF(pUsbInsTypeCheck); \
        if (RT_LIKELY(PDM_VERSION_ARE_COMPATIBLE((pUsbIns)->u32Version, PDM_USBINS_VERSION) )) \
        { /* likely */ } else return; \
        if (RT_LIKELY(PDM_VERSION_ARE_COMPATIBLE((pUsbIns)->pHlpR3->u32Version, PDM_USBHLP_VERSION) )) \
        { /* likely */ } else return; \
    } while (0)


/** Converts a pointer to the PDMUSBINS::IBase to a pointer to PDMUSBINS. */
#define PDMIBASE_2_PDMUSB(pInterface) ( (PPDMUSBINS)((char *)(pInterface) - RT_UOFFSETOF(PDMUSBINS, IBase)) )


/** @def PDMUSB_ASSERT_EMT
 * Assert that the current thread is the emulation thread.
 */
#ifdef VBOX_STRICT
# define PDMUSB_ASSERT_EMT(pUsbIns)  pUsbIns->pHlpR3->pfnAssertEMT(pUsbIns, __FILE__, __LINE__, __FUNCTION__)
#else
# define PDMUSB_ASSERT_EMT(pUsbIns)  do { } while (0)
#endif

/** @def PDMUSB_ASSERT_OTHER
 * Assert that the current thread is NOT the emulation thread.
 */
#ifdef VBOX_STRICT
# define PDMUSB_ASSERT_OTHER(pUsbIns)  pUsbIns->pHlpR3->pfnAssertOther(pUsbIns, __FILE__, __LINE__, __FUNCTION__)
#else
# define PDMUSB_ASSERT_OTHER(pUsbIns)  do { } while (0)
#endif

/** @def PDMUSB_SET_ERROR
 * Set the VM error. See PDMUsbHlpVMSetError() for printf like message
 * formatting.
 */
#define PDMUSB_SET_ERROR(pUsbIns, rc, pszError) \
    PDMUsbHlpVMSetError(pUsbIns, rc, RT_SRC_POS, "%s", pszError)

/** @def PDMUSB_SET_RUNTIME_ERROR
 * Set the VM runtime error. See PDMUsbHlpVMSetRuntimeError() for printf like
 * message formatting.
 */
#define PDMUSB_SET_RUNTIME_ERROR(pUsbIns, fFlags, pszErrorId, pszError) \
    PDMUsbHlpVMSetRuntimeError(pUsbIns, fFlags, pszErrorId, "%s", pszError)


#ifdef IN_RING3

/**
 * @copydoc PDMUSBHLP::pfnDriverAttach
 */
DECLINLINE(int) PDMUsbHlpDriverAttach(PPDMUSBINS pUsbIns, RTUINT iLun, PPDMIBASE pBaseInterface, PPDMIBASE *ppBaseInterface, const char *pszDesc)
{
    return pUsbIns->pHlpR3->pfnDriverAttach(pUsbIns, iLun, pBaseInterface, ppBaseInterface, pszDesc);
}

/**
 * VBOX_STRICT wrapper for pHlpR3->pfnDBGFStopV.
 *
 * @returns VBox status code which must be passed up to the VMM.
 * @param   pUsbIns             Device instance.
 * @param   SRC_POS             Use RT_SRC_POS.
 * @param   pszFormat           Message. (optional)
 * @param   ...                 Message parameters.
 */
DECLINLINE(int) RT_IPRT_FORMAT_ATTR(5, 6) PDMUsbDBGFStop(PPDMUSBINS pUsbIns, RT_SRC_POS_DECL, const char *pszFormat, ...)
{
#ifdef VBOX_STRICT
    int rc;
    va_list va;
    va_start(va, pszFormat);
    rc = pUsbIns->pHlpR3->pfnDBGFStopV(pUsbIns, RT_SRC_POS_ARGS, pszFormat, va);
    va_end(va);
    return rc;
#else
    NOREF(pUsbIns);
    NOREF(pszFile);
    NOREF(iLine);
    NOREF(pszFunction);
    NOREF(pszFormat);
    return VINF_SUCCESS;
#endif
}

/**
 * @copydoc PDMUSBHLP::pfnVMState
 */
DECLINLINE(VMSTATE) PDMUsbHlpVMState(PPDMUSBINS pUsbIns)
{
    return pUsbIns->pHlpR3->pfnVMState(pUsbIns);
}

/**
 * @copydoc PDMUSBHLP::pfnThreadCreate
 */
DECLINLINE(int) PDMUsbHlpThreadCreate(PPDMUSBINS pUsbIns, PPPDMTHREAD ppThread, void *pvUser, PFNPDMTHREADUSB pfnThread,
                                      PFNPDMTHREADWAKEUPUSB pfnWakeup, size_t cbStack, RTTHREADTYPE enmType, const char *pszName)
{
    return pUsbIns->pHlpR3->pfnThreadCreate(pUsbIns, ppThread, pvUser, pfnThread, pfnWakeup, cbStack, enmType, pszName);
}


/**
 * @copydoc PDMUSBHLP::pfnSetAsyncNotification
 */
DECLINLINE(int) PDMUsbHlpSetAsyncNotification(PPDMUSBINS pUsbIns, PFNPDMUSBASYNCNOTIFY pfnAsyncNotify)
{
    return pUsbIns->pHlpR3->pfnSetAsyncNotification(pUsbIns, pfnAsyncNotify);
}

/**
 * @copydoc PDMUSBHLP::pfnAsyncNotificationCompleted
 */
DECLINLINE(void) PDMUsbHlpAsyncNotificationCompleted(PPDMUSBINS pUsbIns)
{
    pUsbIns->pHlpR3->pfnAsyncNotificationCompleted(pUsbIns);
}

/**
 * Set the VM error message
 *
 * @returns rc.
 * @param   pUsbIns             The USB device instance.
 * @param   rc                  VBox status code.
 * @param   SRC_POS             Use RT_SRC_POS.
 * @param   pszFormat           Error message format string.
 * @param   ...                 Error message arguments.
 */
DECLINLINE(int) RT_IPRT_FORMAT_ATTR(6, 7) PDMUsbHlpVMSetError(PPDMUSBINS pUsbIns, int rc, RT_SRC_POS_DECL,
                                                              const char *pszFormat, ...)
{
    va_list     va;
    va_start(va, pszFormat);
    rc = pUsbIns->pHlpR3->pfnVMSetErrorV(pUsbIns, rc, RT_SRC_POS_ARGS, pszFormat, va);
    va_end(va);
    return rc;
}

/**
 * @copydoc PDMUSBHLP::pfnMMHeapAlloc
 */
DECLINLINE(void *) PDMUsbHlpMMHeapAlloc(PPDMUSBINS pUsbIns, size_t cb)
{
    return pUsbIns->pHlpR3->pfnMMHeapAlloc(pUsbIns, cb);
}

/**
 * @copydoc PDMUSBHLP::pfnMMHeapAllocZ
 */
DECLINLINE(void *) PDMUsbHlpMMHeapAllocZ(PPDMUSBINS pUsbIns, size_t cb)
{
    return pUsbIns->pHlpR3->pfnMMHeapAllocZ(pUsbIns, cb);
}

/**
 * Frees memory allocated by PDMUsbHlpMMHeapAlloc or PDMUsbHlpMMHeapAllocZ.
 *
 * @param   pUsbIns                 The USB device instance.
 * @param   pv                      The memory to free.  NULL is fine.
 */
DECLINLINE(void) PDMUsbHlpMMHeapFree(PPDMUSBINS pUsbIns, void *pv)
{
    pUsbIns->pHlpR3->pfnMMHeapFree(pUsbIns, pv);
}

/**
 * @copydoc PDMUSBHLP::pfnDBGFInfoRegisterArgv
 */
DECLINLINE(int) PDMUsbHlpDBGFInfoRegisterArgv(PPDMUSBINS pUsbIns, const char *pszName, const char *pszDesc, PFNDBGFINFOARGVUSB pfnHandler)
{
    return pUsbIns->pHlpR3->pfnDBGFInfoRegisterArgv(pUsbIns, pszName, pszDesc, pfnHandler);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerCreate
 */
DECLINLINE(int) PDMUsbHlpTimerCreate(PPDMUSBINS pUsbIns, TMCLOCK enmClock, PFNTMTIMERUSB pfnCallback, void *pvUser,
                                     uint32_t fFlags, const char *pszDesc, PTMTIMERHANDLE phTimer)
{
    return pUsbIns->pHlpR3->pfnTimerCreate(pUsbIns, enmClock, pfnCallback, pvUser, fFlags, pszDesc, phTimer);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerFromMicro
 */
DECLINLINE(uint64_t) PDMUsbHlpTimerFromMicro(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, uint64_t cMicroSecs)
{
    return pUsbIns->pHlpR3->pfnTimerFromMicro(pUsbIns, hTimer, cMicroSecs);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerFromMilli
 */
DECLINLINE(uint64_t) PDMUsbHlpTimerFromMilli(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, uint64_t cMilliSecs)
{
    return pUsbIns->pHlpR3->pfnTimerFromMilli(pUsbIns, hTimer, cMilliSecs);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerFromNano
 */
DECLINLINE(uint64_t) PDMUsbHlpTimerFromNano(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, uint64_t cNanoSecs)
{
    return pUsbIns->pHlpR3->pfnTimerFromNano(pUsbIns, hTimer, cNanoSecs);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerGet
 */
DECLINLINE(uint64_t) PDMUsbHlpTimerGet(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer)
{
    return pUsbIns->pHlpR3->pfnTimerGet(pUsbIns, hTimer);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerGetFreq
 */
DECLINLINE(uint64_t) PDMUsbHlpTimerGetFreq(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer)
{
    return pUsbIns->pHlpR3->pfnTimerGetFreq(pUsbIns, hTimer);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerGetNano
 */
DECLINLINE(uint64_t) PDMUsbHlpTimerGetNano(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer)
{
    return pUsbIns->pHlpR3->pfnTimerGetNano(pUsbIns, hTimer);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerIsActive
 */
DECLINLINE(bool)     PDMUsbHlpTimerIsActive(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer)
{
    return pUsbIns->pHlpR3->pfnTimerIsActive(pUsbIns, hTimer);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerIsLockOwner
 */
DECLINLINE(bool)     PDMUsbHlpTimerIsLockOwner(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer)
{
    return pUsbIns->pHlpR3->pfnTimerIsLockOwner(pUsbIns, hTimer);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerLockClock
 */
DECLINLINE(int) PDMUsbHlpTimerLockClock(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer)
{
    return pUsbIns->pHlpR3->pfnTimerLockClock(pUsbIns, hTimer);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerLockClock2
 */
DECLINLINE(int) PDMUsbHlpTimerLockClock2(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, PPDMCRITSECT pCritSect)
{
    return pUsbIns->pHlpR3->pfnTimerLockClock2(pUsbIns, hTimer, pCritSect);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerSet
 */
DECLINLINE(int)      PDMUsbHlpTimerSet(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, uint64_t uExpire)
{
    return pUsbIns->pHlpR3->pfnTimerSet(pUsbIns, hTimer, uExpire);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerSetFrequencyHint
 */
DECLINLINE(int)      PDMUsbHlpTimerSetFrequencyHint(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, uint32_t uHz)
{
    return pUsbIns->pHlpR3->pfnTimerSetFrequencyHint(pUsbIns, hTimer, uHz);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerSetMicro
 */
DECLINLINE(int)      PDMUsbHlpTimerSetMicro(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, uint64_t cMicrosToNext)
{
    return pUsbIns->pHlpR3->pfnTimerSetMicro(pUsbIns, hTimer, cMicrosToNext);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerSetMillies
 */
DECLINLINE(int)      PDMUsbHlpTimerSetMillies(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, uint64_t cMilliesToNext)
{
    return pUsbIns->pHlpR3->pfnTimerSetMillies(pUsbIns, hTimer, cMilliesToNext);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerSetNano
 */
DECLINLINE(int)      PDMUsbHlpTimerSetNano(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, uint64_t cNanosToNext)
{
    return pUsbIns->pHlpR3->pfnTimerSetNano(pUsbIns, hTimer, cNanosToNext);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerSetRelative
 */
DECLINLINE(int)      PDMUsbHlpTimerSetRelative(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, uint64_t cTicksToNext, uint64_t *pu64Now)
{
    return pUsbIns->pHlpR3->pfnTimerSetRelative(pUsbIns, hTimer, cTicksToNext, pu64Now);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerStop
 */
DECLINLINE(int)      PDMUsbHlpTimerStop(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer)
{
    return pUsbIns->pHlpR3->pfnTimerStop(pUsbIns, hTimer);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerUnlockClock
 */
DECLINLINE(void)     PDMUsbHlpTimerUnlockClock(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer)
{
    pUsbIns->pHlpR3->pfnTimerUnlockClock(pUsbIns, hTimer);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerUnlockClock2
 */
DECLINLINE(void)     PDMUsbHlpTimerUnlockClock2(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, PPDMCRITSECT pCritSect)
{
    pUsbIns->pHlpR3->pfnTimerUnlockClock2(pUsbIns, hTimer, pCritSect);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerSetCritSect
 */
DECLINLINE(int) PDMUsbHlpTimerSetCritSect(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, PPDMCRITSECT pCritSect)
{
    return pUsbIns->pHlpR3->pfnTimerSetCritSect(pUsbIns, hTimer, pCritSect);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerSave
 */
DECLINLINE(int) PDMUsbHlpTimerSave(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, PSSMHANDLE pSSM)
{
    return pUsbIns->pHlpR3->pfnTimerSave(pUsbIns, hTimer, pSSM);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerLoad
 */
DECLINLINE(int) PDMUsbHlpTimerLoad(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, PSSMHANDLE pSSM)
{
    return pUsbIns->pHlpR3->pfnTimerLoad(pUsbIns, hTimer, pSSM);
}

/**
 * @copydoc PDMUSBHLP::pfnTimerDestroy
 */
DECLINLINE(int) PDMUsbHlpTimerDestroy(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer)
{
    return pUsbIns->pHlpR3->pfnTimerDestroy(pUsbIns, hTimer);
}

/**
 * @copydoc PDMUSBHLP::pfnSSMRegister
 */
DECLINLINE(int) PDMUsbHlpSSMRegister(PPDMUSBINS pUsbIns, uint32_t uVersion, size_t cbGuess,
                                     PFNSSMUSBLIVEPREP pfnLivePrep, PFNSSMUSBLIVEEXEC pfnLiveExec, PFNSSMUSBLIVEVOTE pfnLiveVote,
                                     PFNSSMUSBSAVEPREP pfnSavePrep, PFNSSMUSBSAVEEXEC pfnSaveExec, PFNSSMUSBSAVEDONE pfnSaveDone,
                                     PFNSSMUSBLOADPREP pfnLoadPrep, PFNSSMUSBLOADEXEC pfnLoadExec, PFNSSMUSBLOADDONE pfnLoadDone)
{
    return pUsbIns->pHlpR3->pfnSSMRegister(pUsbIns, uVersion, cbGuess,
                                           pfnLivePrep, pfnLiveExec, pfnLiveVote,
                                           pfnSavePrep, pfnSaveExec, pfnSaveDone,
                                           pfnLoadPrep, pfnLoadExec, pfnLoadDone);
}

/**
 * @copydoc PDMUSBHLP::pfnQueryGenericUserObject
 */
DECLINLINE(void *) PDMUsbHlpQueryGenericUserObject(PPDMUSBINS pUsbIns, PCRTUUID pUuid)
{
    return pUsbIns->pHlpR3->pfnQueryGenericUserObject(pUsbIns, pUuid);
}

#endif /* IN_RING3 */



/** Pointer to callbacks provided to the VBoxUsbRegister() call. */
typedef const struct PDMUSBREGCB *PCPDMUSBREGCB;

/**
 * Callbacks for VBoxUSBDeviceRegister().
 */
typedef struct PDMUSBREGCB
{
    /** Interface version.
     * This is set to PDM_USBREG_CB_VERSION. */
    uint32_t                    u32Version;

    /**
     * Registers a device with the current VM instance.
     *
     * @returns VBox status code.
     * @param   pCallbacks      Pointer to the callback table.
     * @param   pReg            Pointer to the USB device registration record.
     *                          This data must be permanent and readonly.
     */
    DECLR3CALLBACKMEMBER(int, pfnRegister,(PCPDMUSBREGCB pCallbacks, PCPDMUSBREG pReg));
} PDMUSBREGCB;

/** Current version of the PDMUSBREGCB structure.  */
#define PDM_USBREG_CB_VERSION                   PDM_VERSION_MAKE(0xeefc, 1, 0)


/**
 * The VBoxUsbRegister callback function.
 *
 * PDM will invoke this function after loading a USB device module and letting
 * the module decide which devices to register and how to handle conflicts.
 *
 * @returns VBox status code.
 * @param   pCallbacks      Pointer to the callback table.
 * @param   u32Version      VBox version number.
 */
typedef DECLCALLBACKTYPE(int, FNPDMVBOXUSBREGISTER,(PCPDMUSBREGCB pCallbacks, uint32_t u32Version));

VMMR3DECL(int)  PDMR3UsbCreateEmulatedDevice(PUVM pUVM, const char *pszDeviceName, PCFGMNODE pDeviceNode, PCRTUUID pUuid,
                                             const char *pszCaptureFilename);
VMMR3DECL(int)  PDMR3UsbCreateProxyDevice(PUVM pUVM, PCRTUUID pUuid, const char *pszBackend, const char *pszAddress, PCFGMNODE pSubTree,
                                          VUSBSPEED enmSpeed, uint32_t fMaskedIfs, const char *pszCaptureFilename);
VMMR3DECL(int)  PDMR3UsbDetachDevice(PUVM pUVM, PCRTUUID pUuid);
VMMR3DECL(bool) PDMR3UsbHasHub(PUVM pUVM);
VMMR3DECL(int)  PDMR3UsbDriverAttach(PUVM pUVM, const char *pszDevice, unsigned iDevIns, unsigned iLun, uint32_t fFlags,
                                     PPPDMIBASE ppBase);
VMMR3DECL(int)  PDMR3UsbDriverDetach(PUVM pUVM, const char *pszDevice, unsigned iDevIns, unsigned iLun,
                                     const char *pszDriver, unsigned iOccurrence, uint32_t fFlags);
VMMR3DECL(int)  PDMR3UsbQueryLun(PUVM pUVM, const char *pszDevice, unsigned iInstance, unsigned iLun, PPDMIBASE *ppBase);
VMMR3DECL(int)  PDMR3UsbQueryDriverOnLun(PUVM pUVM, const char *pszDevice, unsigned iInstance, unsigned iLun,
                                         const char *pszDriver, PPPDMIBASE ppBase);

/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_pdmusb_h */
