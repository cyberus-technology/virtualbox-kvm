/** @file
 * PDM - Pluggable Device Manager, Drivers.
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

#ifndef VBOX_INCLUDED_vmm_pdmdrv_h
#define VBOX_INCLUDED_vmm_pdmdrv_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/vmm/pdmqueue.h>
#include <VBox/vmm/pdmcritsect.h>
#include <VBox/vmm/pdmifs.h>
#include <VBox/vmm/pdmins.h>
#include <VBox/vmm/pdmcommon.h>
#ifdef IN_RING3
# include <VBox/vmm/pdmthread.h>
# include <VBox/vmm/pdmasynccompletion.h>
# include <VBox/vmm/pdmblkcache.h>
#endif
#include <VBox/vmm/tm.h>
#include <VBox/vmm/ssm.h>
#include <VBox/vmm/cfgm.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/mm.h>
#include <iprt/stdarg.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_pdm_driver    The PDM Drivers API
 * @ingroup grp_pdm
 * @{
 */

/** Pointer const PDM Driver API, ring-3. */
typedef R3PTRTYPE(struct PDMDRVHLPR3 const *) PCPDMDRVHLPR3;
/** Pointer const PDM Driver API, ring-0. */
typedef R0PTRTYPE(struct PDMDRVHLPR0 const *) PCPDMDRVHLPR0;
/** Pointer const PDM Driver API, raw-mode context. */
typedef RCPTRTYPE(struct PDMDRVHLPRC const *) PCPDMDRVHLPRC;


/**
 * Construct a driver instance for a VM.
 *
 * @returns VBox status.
 * @param   pDrvIns     The driver instance data. If the registration structure
 *                      is needed, it can be accessed thru pDrvIns->pReg.
 * @param   pCfg        Configuration node handle for the driver.  This is
 *                      expected to be in high demand in the constructor and is
 *                      therefore passed as an argument.  When using it at other
 *                      times, it can be accessed via pDrvIns->pCfg.
 * @param   fFlags      Flags, combination of the PDM_TACH_FLAGS_* \#defines.
 */
typedef DECLCALLBACKTYPE(int, FNPDMDRVCONSTRUCT,(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags));
/** Pointer to a FNPDMDRVCONSTRUCT() function. */
typedef FNPDMDRVCONSTRUCT *PFNPDMDRVCONSTRUCT;

/**
 * Destruct a driver instance.
 *
 * Most VM resources are freed by the VM. This callback is provided so that
 * any non-VM resources can be freed correctly.
 *
 * @param   pDrvIns     The driver instance data.
 */
typedef DECLCALLBACKTYPE(void, FNPDMDRVDESTRUCT,(PPDMDRVINS pDrvIns));
/** Pointer to a FNPDMDRVDESTRUCT() function. */
typedef FNPDMDRVDESTRUCT *PFNPDMDRVDESTRUCT;

/**
 * Driver relocation callback.
 *
 * This is called when the instance data has been relocated in raw-mode context
 * (RC).  It is also called when the RC hypervisor selects changes.  The driver
 * must fixup all necessary pointers and re-query all interfaces to other RC
 * devices and drivers.
 *
 * Before the RC code is executed the first time, this function will be called
 * with a 0 delta so RC pointer calculations can be one in one place.
 *
 * @param   pDrvIns     Pointer to the driver instance.
 * @param   offDelta    The relocation delta relative to the old location.
 *
 * @remark  A relocation CANNOT fail.
 */
typedef DECLCALLBACKTYPE(void, FNPDMDRVRELOCATE,(PPDMDRVINS pDrvIns, RTGCINTPTR offDelta));
/** Pointer to a FNPDMDRVRELOCATE() function. */
typedef FNPDMDRVRELOCATE *PFNPDMDRVRELOCATE;

/**
 * Driver I/O Control interface.
 *
 * This is used by external components, such as the COM interface, to
 * communicate with a driver using a driver specific interface. Generally,
 * the driver interfaces are used for this task.
 *
 * @returns VBox status code.
 * @param   pDrvIns     Pointer to the driver instance.
 * @param   uFunction   Function to perform.
 * @param   pvIn        Pointer to input data.
 * @param   cbIn        Size of input data.
 * @param   pvOut       Pointer to output data.
 * @param   cbOut       Size of output data.
 * @param   pcbOut      Where to store the actual size of the output data.
 */
typedef DECLCALLBACKTYPE(int, FNPDMDRVIOCTL,(PPDMDRVINS pDrvIns, uint32_t uFunction,
                                             void *pvIn, uint32_t cbIn,
                                             void *pvOut, uint32_t cbOut, uint32_t *pcbOut));
/** Pointer to a FNPDMDRVIOCTL() function. */
typedef FNPDMDRVIOCTL *PFNPDMDRVIOCTL;

/**
 * Power On notification.
 *
 * @param   pDrvIns     The driver instance data.
 */
typedef DECLCALLBACKTYPE(void, FNPDMDRVPOWERON,(PPDMDRVINS pDrvIns));
/** Pointer to a FNPDMDRVPOWERON() function. */
typedef FNPDMDRVPOWERON *PFNPDMDRVPOWERON;

/**
 * Reset notification.
 *
 * @param   pDrvIns     The driver instance data.
 */
typedef DECLCALLBACKTYPE(void, FNPDMDRVRESET,(PPDMDRVINS pDrvIns));
/** Pointer to a FNPDMDRVRESET() function. */
typedef FNPDMDRVRESET *PFNPDMDRVRESET;

/**
 * Suspend notification.
 *
 * @param   pDrvIns     The driver instance data.
 */
typedef DECLCALLBACKTYPE(void, FNPDMDRVSUSPEND,(PPDMDRVINS pDrvIns));
/** Pointer to a FNPDMDRVSUSPEND() function. */
typedef FNPDMDRVSUSPEND *PFNPDMDRVSUSPEND;

/**
 * Resume notification.
 *
 * @param   pDrvIns     The driver instance data.
 */
typedef DECLCALLBACKTYPE(void, FNPDMDRVRESUME,(PPDMDRVINS pDrvIns));
/** Pointer to a FNPDMDRVRESUME() function. */
typedef FNPDMDRVRESUME *PFNPDMDRVRESUME;

/**
 * Power Off notification.
 *
 * This is always called when VMR3PowerOff is called.
 * There will be no callback when hot plugging devices or when replumbing the driver
 * stack.
 *
 * @param   pDrvIns     The driver instance data.
 */
typedef DECLCALLBACKTYPE(void, FNPDMDRVPOWEROFF,(PPDMDRVINS pDrvIns));
/** Pointer to a FNPDMDRVPOWEROFF() function. */
typedef FNPDMDRVPOWEROFF *PFNPDMDRVPOWEROFF;

/**
 * Attach command.
 *
 * This is called to let the driver attach to a driver at runtime.  This is not
 * called during VM construction, the driver constructor have to do this by
 * calling PDMDrvHlpAttach.
 *
 * This is like plugging in the keyboard or mouse after turning on the PC.
 *
 * @returns VBox status code.
 * @param   pDrvIns     The driver instance.
 * @param   fFlags      Flags, combination of the PDM_TACH_FLAGS_* \#defines.
 */
typedef DECLCALLBACKTYPE(int, FNPDMDRVATTACH,(PPDMDRVINS pDrvIns, uint32_t fFlags));
/** Pointer to a FNPDMDRVATTACH() function. */
typedef FNPDMDRVATTACH *PFNPDMDRVATTACH;

/**
 * Detach notification.
 *
 * This is called when a driver below it in the chain is detaching itself
 * from it. The driver should adjust it's state to reflect this.
 *
 * This is like ejecting a cdrom or floppy.
 *
 * @param   pDrvIns     The driver instance.
 * @param   fFlags      PDM_TACH_FLAGS_NOT_HOT_PLUG or 0.
 */
typedef DECLCALLBACKTYPE(void, FNPDMDRVDETACH,(PPDMDRVINS pDrvIns, uint32_t fFlags));
/** Pointer to a FNPDMDRVDETACH() function. */
typedef FNPDMDRVDETACH *PFNPDMDRVDETACH;



/**
 * PDM Driver Registration Structure.
 *
 * This structure is used when registering a driver from VBoxInitDrivers() (in
 * host ring-3 context).  PDM will continue use till the VM is terminated.
 */
typedef struct PDMDRVREG
{
    /** Structure version. PDM_DRVREG_VERSION defines the current version. */
    uint32_t            u32Version;
    /** Driver name. */
    char                szName[32];
    /** Name of the raw-mode context module (no path).
     * Only evalutated if PDM_DRVREG_FLAGS_RC is set. */
    char                szRCMod[32];
    /** Name of the ring-0 module (no path).
     * Only evalutated if PDM_DRVREG_FLAGS_R0 is set. */
    char                szR0Mod[32];
    /** The description of the driver. The UTF-8 string pointed to shall, like this structure,
     * remain unchanged from registration till VM destruction. */
    const char         *pszDescription;

    /** Flags, combination of the PDM_DRVREG_FLAGS_* \#defines. */
    uint32_t            fFlags;
    /** Driver class(es), combination of the PDM_DRVREG_CLASS_* \#defines. */
    uint32_t            fClass;
    /** Maximum number of instances (per VM). */
    uint32_t            cMaxInstances;
    /** Size of the instance data. */
    uint32_t            cbInstance;

    /** Construct instance - required. */
    PFNPDMDRVCONSTRUCT  pfnConstruct;
    /** Destruct instance - optional. */
    PFNPDMDRVDESTRUCT   pfnDestruct;
    /** Relocation command - optional. */
    PFNPDMDRVRELOCATE   pfnRelocate;
    /** I/O control - optional. */
    PFNPDMDRVIOCTL      pfnIOCtl;
    /** Power on notification - optional. */
    PFNPDMDRVPOWERON    pfnPowerOn;
    /** Reset notification - optional. */
    PFNPDMDRVRESET      pfnReset;
    /** Suspend notification  - optional. */
    PFNPDMDRVSUSPEND    pfnSuspend;
    /** Resume notification - optional. */
    PFNPDMDRVRESUME     pfnResume;
    /** Attach command - optional. */
    PFNPDMDRVATTACH     pfnAttach;
    /** Detach notification - optional. */
    PFNPDMDRVDETACH     pfnDetach;
    /** Power off notification - optional. */
    PFNPDMDRVPOWEROFF   pfnPowerOff;
    /** @todo */
    PFNRT               pfnSoftReset;
    /** Initialization safty marker. */
    uint32_t            u32VersionEnd;
} PDMDRVREG;
/** Pointer to a PDM Driver Structure. */
typedef PDMDRVREG *PPDMDRVREG;
/** Const pointer to a PDM Driver Structure. */
typedef PDMDRVREG const *PCPDMDRVREG;

/** Current DRVREG version number. */
#define PDM_DRVREG_VERSION                      PDM_VERSION_MAKE(0xf0ff, 1, 0)

/** PDM Driver Flags.
 * @{ */
/** @def PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT
 * The bit count for the current host. */
#if HC_ARCH_BITS == 32
# define PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT     UINT32_C(0x00000001)
#elif HC_ARCH_BITS == 64
# define PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT     UINT32_C(0x00000002)
#else
# error Unsupported HC_ARCH_BITS value.
#endif
/** The host bit count mask. */
#define PDM_DRVREG_FLAGS_HOST_BITS_MASK         UINT32_C(0x00000003)
/** This flag is used to indicate that the driver has a RC component. */
#define PDM_DRVREG_FLAGS_RC                     UINT32_C(0x00000010)
/** This flag is used to indicate that the driver has a R0 component. */
#define PDM_DRVREG_FLAGS_R0                     UINT32_C(0x00000020)

/** @} */


/** PDM Driver Classes.
 * @{ */
/** Mouse input driver. */
#define PDM_DRVREG_CLASS_MOUSE          RT_BIT(0)
/** Keyboard input driver. */
#define PDM_DRVREG_CLASS_KEYBOARD       RT_BIT(1)
/** Display driver. */
#define PDM_DRVREG_CLASS_DISPLAY        RT_BIT(2)
/** Network transport driver. */
#define PDM_DRVREG_CLASS_NETWORK        RT_BIT(3)
/** Block driver. */
#define PDM_DRVREG_CLASS_BLOCK          RT_BIT(4)
/** Media driver. */
#define PDM_DRVREG_CLASS_MEDIA          RT_BIT(5)
/** Mountable driver. */
#define PDM_DRVREG_CLASS_MOUNTABLE      RT_BIT(6)
/** Audio driver. */
#define PDM_DRVREG_CLASS_AUDIO          RT_BIT(7)
/** VMMDev driver. */
#define PDM_DRVREG_CLASS_VMMDEV         RT_BIT(8)
/** Status driver. */
#define PDM_DRVREG_CLASS_STATUS         RT_BIT(9)
/** ACPI driver. */
#define PDM_DRVREG_CLASS_ACPI           RT_BIT(10)
/** USB related driver. */
#define PDM_DRVREG_CLASS_USB            RT_BIT(11)
/** ISCSI Transport related driver. */
#define PDM_DRVREG_CLASS_ISCSITRANSPORT RT_BIT(12)
/** Char driver. */
#define PDM_DRVREG_CLASS_CHAR           RT_BIT(13)
/** Stream driver. */
#define PDM_DRVREG_CLASS_STREAM         RT_BIT(14)
/** SCSI driver. */
#define PDM_DRVREG_CLASS_SCSI           RT_BIT(15)
/** Generic raw PCI device driver. */
#define PDM_DRVREG_CLASS_PCIRAW         RT_BIT(16)
/** @} */


/**
 * PDM Driver Instance.
 *
 * @implements  PDMIBASE
 */
typedef struct PDMDRVINS
{
    /** Structure version. PDM_DRVINS_VERSION defines the current version. */
    uint32_t                    u32Version;
    /** Driver instance number. */
    uint32_t                    iInstance;

    /** Pointer the PDM Driver API. */
    RCPTRTYPE(PCPDMDRVHLPRC)    pHlpRC;
    /** Pointer to driver instance data. */
    RCPTRTYPE(void *)           pvInstanceDataRC;

    /** Pointer the PDM Driver API. */
    R0PTRTYPE(PCPDMDRVHLPR0)    pHlpR0;
    /** Pointer to driver instance data. */
    R0PTRTYPE(void *)           pvInstanceDataR0;

    /** Pointer the PDM Driver API. */
    R3PTRTYPE(PCPDMDRVHLPR3)    pHlpR3;
    /** Pointer to driver instance data. */
    R3PTRTYPE(void *)           pvInstanceDataR3;

    /** Pointer to driver registration structure.  */
    R3PTRTYPE(PCPDMDRVREG)      pReg;
    /** Configuration handle. */
    R3PTRTYPE(PCFGMNODE)        pCfg;

    /** Pointer to the base interface of the device/driver instance above. */
    R3PTRTYPE(PPDMIBASE)        pUpBase;
    /** Pointer to the base interface of the driver instance below. */
    R3PTRTYPE(PPDMIBASE)        pDownBase;

    /** The base interface of the driver.
     * The driver constructor initializes this. */
    PDMIBASE                    IBase;

    /** Tracing indicator. */
    uint32_t                    fTracing;
    /** The tracing ID of this device.  */
    uint32_t                    idTracing;
#if HC_ARCH_BITS == 32
    /** Align the internal data more naturally. */
    uint32_t                    au32Padding[HC_ARCH_BITS == 32 ? 7 : 0];
#endif

    /** Internal data. */
    union
    {
#ifdef PDMDRVINSINT_DECLARED
        PDMDRVINSINT            s;
#endif
        uint8_t                 padding[HC_ARCH_BITS == 32 ? 40 + 32 : 72 + 24];
    } Internal;

    /** Driver instance data. The size of this area is defined
     * in the PDMDRVREG::cbInstanceData field. */
    char                        achInstanceData[4];
} PDMDRVINS;

/** Current DRVREG version number. */
#define PDM_DRVINS_VERSION                      PDM_VERSION_MAKE(0xf0fe, 2, 0)

/** Converts a pointer to the PDMDRVINS::IBase to a pointer to PDMDRVINS. */
#define PDMIBASE_2_PDMDRV(pInterface)   ( (PPDMDRVINS)((char *)(pInterface) - RT_UOFFSETOF(PDMDRVINS, IBase)) )

/** @def PDMDRVINS_2_RCPTR
 * Converts a PDM Driver instance pointer a RC PDM Driver instance pointer.
 */
#define PDMDRVINS_2_RCPTR(pDrvIns)      ( (RCPTRTYPE(PPDMDRVINS))((RTRCUINTPTR)(pDrvIns)->pvInstanceDataRC - (RTRCUINTPTR)RT_UOFFSETOF(PDMDRVINS, achInstanceData)) )

/** @def PDMDRVINS_2_R3PTR
 * Converts a PDM Driver instance pointer a R3 PDM Driver instance pointer.
 */
#define PDMDRVINS_2_R3PTR(pDrvIns)      ( (R3PTRTYPE(PPDMDRVINS))((RTHCUINTPTR)(pDrvIns)->pvInstanceDataR3 - RT_UOFFSETOF(PDMDRVINS, achInstanceData)) )

/** @def PDMDRVINS_2_R0PTR
 * Converts a PDM Driver instance pointer a R0 PDM Driver instance pointer.
 */
#define PDMDRVINS_2_R0PTR(pDrvIns)      ( (R0PTRTYPE(PPDMDRVINS))((RTR0UINTPTR)(pDrvIns)->pvInstanceDataR0 - RT_UOFFSETOF(PDMDRVINS, achInstanceData)) )



/**
 * Checks the structure versions of the drive instance and driver helpers,
 * returning if they are incompatible.
 *
 * Intended for the constructor.
 *
 * @param   pDrvIns             Pointer to the PDM driver instance.
 */
#define PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns) \
    do \
    { \
        PPDMDRVINS pDrvInsTypeCheck = (pDrvIns); NOREF(pDrvInsTypeCheck); \
        AssertLogRelMsgReturn(PDM_VERSION_ARE_COMPATIBLE((pDrvIns)->u32Version, PDM_DRVINS_VERSION), \
                              ("DrvIns=%#x  mine=%#x\n", (pDrvIns)->u32Version, PDM_DRVINS_VERSION), \
                              VERR_PDM_DRVINS_VERSION_MISMATCH); \
        AssertLogRelMsgReturn(PDM_VERSION_ARE_COMPATIBLE((pDrvIns)->pHlpR3->u32Version, PDM_DRVHLPR3_VERSION), \
                              ("DrvHlp=%#x  mine=%#x\n", (pDrvIns)->pHlpR3->u32Version, PDM_DRVHLPR3_VERSION), \
                              VERR_PDM_DRVHLPR3_VERSION_MISMATCH); \
    } while (0)

/**
 * Quietly checks the structure versions of the drive instance and driver
 * helpers, returning if they are incompatible.
 *
 * Intended for the destructor.
 *
 * @param   pDrvIns             Pointer to the PDM driver instance.
 */
#define PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns) \
    do \
    { \
        PPDMDRVINS pDrvInsTypeCheck = (pDrvIns); NOREF(pDrvInsTypeCheck); \
        if (RT_LIKELY(   PDM_VERSION_ARE_COMPATIBLE((pDrvIns)->u32Version, PDM_DRVINS_VERSION) \
                      && PDM_VERSION_ARE_COMPATIBLE((pDrvIns)->pHlpR3->u32Version, PDM_DRVHLPR3_VERSION)) ) \
        { /* likely */ } else return; \
    } while (0)

/**
 * Wrapper around CFGMR3ValidateConfig for the root config for use in the
 * constructor - returns on failure.
 *
 * This should be invoked after having initialized the instance data
 * sufficiently for the correct operation of the destructor.  The destructor is
 * always called!
 *
 * @param   pDrvIns             Pointer to the PDM driver instance.
 * @param   pszValidValues      Patterns describing the valid value names.  See
 *                              RTStrSimplePatternMultiMatch for details on the
 *                              pattern syntax.
 * @param   pszValidNodes       Patterns describing the valid node (key) names.
 *                              Pass empty string if no valid nodess.
 */
#define PDMDRV_VALIDATE_CONFIG_RETURN(pDrvIns, pszValidValues, pszValidNodes) \
    do \
    { \
        int rcValCfg = pDrvIns->pHlpR3->pfnCFGMValidateConfig((pDrvIns)->pCfg, "/", pszValidValues, pszValidNodes, \
                                                              (pDrvIns)->pReg->szName, (pDrvIns)->iInstance); \
        if (RT_SUCCESS(rcValCfg)) \
        { /* likely */ } else return rcValCfg; \
    } while (0)



/**
 * USB hub registration structure.
 */
typedef struct PDMUSBHUBREG
{
    /** Structure version number. PDM_USBHUBREG_VERSION defines the current version. */
    uint32_t            u32Version;

    /**
     * Request the hub to attach of the specified device.
     *
     * @returns VBox status code.
     * @param   pDrvIns            The hub instance.
     * @param   pUsbIns            The device to attach.
     * @param   pszCaptureFilename Path to the file for USB traffic capturing, optional.
     * @param   piPort             Where to store the port number the device was attached to.
     * @thread EMT.
     */
    DECLR3CALLBACKMEMBER(int, pfnAttachDevice,(PPDMDRVINS pDrvIns, PPDMUSBINS pUsbIns, const char *pszCaptureFilename, uint32_t *piPort));

    /**
     * Request the hub to detach of the specified device.
     *
     * The device has previously been attached to the hub with the
     * pfnAttachDevice call. This call is not currently expected to
     * fail.
     *
     * @returns VBox status code.
     * @param   pDrvIns     The hub instance.
     * @param   pUsbIns     The device to detach.
     * @param   iPort       The port number returned by the attach call.
     * @thread EMT.
     */
    DECLR3CALLBACKMEMBER(int, pfnDetachDevice,(PPDMDRVINS pDrvIns, PPDMUSBINS pUsbIns, uint32_t iPort));

    /** Counterpart to u32Version, same value. */
    uint32_t            u32TheEnd;
} PDMUSBHUBREG;
/** Pointer to a const USB hub registration structure. */
typedef const PDMUSBHUBREG *PCPDMUSBHUBREG;

/** Current PDMUSBHUBREG version number. */
#define PDM_USBHUBREG_VERSION                   PDM_VERSION_MAKE(0xf0fd, 2, 0)


/**
 * USB hub helpers.
 * This is currently just a place holder.
 */
typedef struct PDMUSBHUBHLP
{
    /** Structure version. PDM_USBHUBHLP_VERSION defines the current version. */
    uint32_t                    u32Version;

    /** Just a safety precaution. */
    uint32_t                    u32TheEnd;
} PDMUSBHUBHLP;
/** Pointer to PCI helpers. */
typedef PDMUSBHUBHLP *PPDMUSBHUBHLP;
/** Pointer to const PCI helpers. */
typedef const PDMUSBHUBHLP *PCPDMUSBHUBHLP;
/** Pointer to const PCI helpers pointer. */
typedef PCPDMUSBHUBHLP *PPCPDMUSBHUBHLP;

/** Current PDMUSBHUBHLP version number. */
#define PDM_USBHUBHLP_VERSION                   PDM_VERSION_MAKE(0xf0fc, 1, 0)


/**
 * PDM Driver API - raw-mode context variant.
 */
typedef struct PDMDRVHLPRC
{
    /** Structure version. PDM_DRVHLPRC_VERSION defines the current version. */
    uint32_t                    u32Version;

    /**
     * Assert that the current thread is the emulation thread.
     *
     * @returns True if correct.
     * @returns False if wrong.
     * @param   pDrvIns         Driver instance.
     * @param   pszFile         Filename of the assertion location.
     * @param   iLine           Linenumber of the assertion location.
     * @param   pszFunction     Function of the assertion location.
     */
    DECLRCCALLBACKMEMBER(bool, pfnAssertEMT,(PPDMDRVINS pDrvIns, const char *pszFile, unsigned iLine, const char *pszFunction));

    /**
     * Assert that the current thread is NOT the emulation thread.
     *
     * @returns True if correct.
     * @returns False if wrong.
     * @param   pDrvIns         Driver instance.
     * @param   pszFile         Filename of the assertion location.
     * @param   iLine           Linenumber of the assertion location.
     * @param   pszFunction     Function of the assertion location.
     */
    DECLRCCALLBACKMEMBER(bool, pfnAssertOther,(PPDMDRVINS pDrvIns, const char *pszFile, unsigned iLine, const char *pszFunction));

    /** @name Exported PDM Critical Section Functions
     * @{ */
    DECLRCCALLBACKMEMBER(int,      pfnCritSectEnter,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect, int rcBusy));
    DECLRCCALLBACKMEMBER(int,      pfnCritSectEnterDebug,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect, int rcBusy, RTHCUINTPTR uId, RT_SRC_POS_DECL));
    DECLRCCALLBACKMEMBER(int,      pfnCritSectTryEnter,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect));
    DECLRCCALLBACKMEMBER(int,      pfnCritSectTryEnterDebug,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect, RTHCUINTPTR uId, RT_SRC_POS_DECL));
    DECLRCCALLBACKMEMBER(int,      pfnCritSectLeave,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect));
    DECLRCCALLBACKMEMBER(bool,     pfnCritSectIsOwner,(PPDMDRVINS pDrvIns, PCPDMCRITSECT pCritSect));
    DECLRCCALLBACKMEMBER(bool,     pfnCritSectIsInitialized,(PPDMDRVINS pDrvIns, PCPDMCRITSECT pCritSect));
    DECLRCCALLBACKMEMBER(bool,     pfnCritSectHasWaiters,(PPDMDRVINS pDrvIns, PCPDMCRITSECT pCritSect));
    DECLRCCALLBACKMEMBER(uint32_t, pfnCritSectGetRecursion,(PPDMDRVINS pDrvIns, PCPDMCRITSECT pCritSect));
    /** @} */

    /**
     * Obtains bandwidth in a bandwidth group.
     *
     * @returns True if bandwidth was allocated, false if not.
     * @param   pDrvIns         The driver instance.
     * @param   pFilter         Pointer to the filter that allocates bandwidth.
     * @param   cbTransfer      Number of bytes to allocate.
     */
    DECLRCCALLBACKMEMBER(bool, pfnNetShaperAllocateBandwidth,(PPDMDRVINS pDrvIns, PPDMNSFILTER pFilter, size_t cbTransfer));

    /** Just a safety precaution. */
    uint32_t                        u32TheEnd;
} PDMDRVHLPRC;
/** Current PDMDRVHLPRC version number. */
#define PDM_DRVHLPRC_VERSION                    PDM_VERSION_MAKE(0xf0f9, 6, 0)


/**
 * PDM Driver API, ring-0 context.
 */
typedef struct PDMDRVHLPR0
{
    /** Structure version. PDM_DRVHLPR0_VERSION defines the current version. */
    uint32_t                    u32Version;

    /**
     * Assert that the current thread is the emulation thread.
     *
     * @returns True if correct.
     * @returns False if wrong.
     * @param   pDrvIns         Driver instance.
     * @param   pszFile         Filename of the assertion location.
     * @param   iLine           Linenumber of the assertion location.
     * @param   pszFunction     Function of the assertion location.
     */
    DECLR0CALLBACKMEMBER(bool, pfnAssertEMT,(PPDMDRVINS pDrvIns, const char *pszFile, unsigned iLine, const char *pszFunction));

    /**
     * Assert that the current thread is NOT the emulation thread.
     *
     * @returns True if correct.
     * @returns False if wrong.
     * @param   pDrvIns         Driver instance.
     * @param   pszFile         Filename of the assertion location.
     * @param   iLine           Linenumber of the assertion location.
     * @param   pszFunction     Function of the assertion location.
     */
    DECLR0CALLBACKMEMBER(bool, pfnAssertOther,(PPDMDRVINS pDrvIns, const char *pszFile, unsigned iLine, const char *pszFunction));

    /** @name Exported PDM Critical Section Functions
     * @{ */
    DECLR0CALLBACKMEMBER(int,      pfnCritSectEnter,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect, int rcBusy));
    DECLR0CALLBACKMEMBER(int,      pfnCritSectEnterDebug,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect, int rcBusy, RTHCUINTPTR uId, RT_SRC_POS_DECL));
    DECLR0CALLBACKMEMBER(int,      pfnCritSectTryEnter,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect));
    DECLR0CALLBACKMEMBER(int,      pfnCritSectTryEnterDebug,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect, RTHCUINTPTR uId, RT_SRC_POS_DECL));
    DECLR0CALLBACKMEMBER(int,      pfnCritSectLeave,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect));
    DECLR0CALLBACKMEMBER(bool,     pfnCritSectIsOwner,(PPDMDRVINS pDrvIns, PCPDMCRITSECT pCritSect));
    DECLR0CALLBACKMEMBER(bool,     pfnCritSectIsInitialized,(PPDMDRVINS pDrvIns, PCPDMCRITSECT pCritSect));
    DECLR0CALLBACKMEMBER(bool,     pfnCritSectHasWaiters,(PPDMDRVINS pDrvIns, PCPDMCRITSECT pCritSect));
    DECLR0CALLBACKMEMBER(uint32_t, pfnCritSectGetRecursion,(PPDMDRVINS pDrvIns, PCPDMCRITSECT pCritSect));
    DECLR0CALLBACKMEMBER(int,      pfnCritSectScheduleExitEvent,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect, SUPSEMEVENT hEventToSignal));
    /** @} */

    /**
     * Obtains bandwidth in a bandwidth group.
     *
     * @returns True if bandwidth was allocated, false if not.
     * @param   pDrvIns         The driver instance.
     * @param   pFilter         Pointer to the filter that allocates bandwidth.
     * @param   cbTransfer      Number of bytes to allocate.
     */
    DECLR0CALLBACKMEMBER(bool, pfnNetShaperAllocateBandwidth,(PPDMDRVINS pDrvIns, PPDMNSFILTER pFilter, size_t cbTransfer));

    /** Just a safety precaution. */
    uint32_t                        u32TheEnd;
} PDMDRVHLPR0;
/** Current DRVHLP version number. */
#define PDM_DRVHLPR0_VERSION                    PDM_VERSION_MAKE(0xf0f8, 6, 0)


#ifdef IN_RING3

/**
 * PDM Driver API.
 */
typedef struct PDMDRVHLPR3
{
    /** Structure version. PDM_DRVHLPR3_VERSION defines the current version. */
    uint32_t                    u32Version;

    /**
     * Attaches a driver (chain) to the driver.
     *
     * @returns VBox status code.
     * @param   pDrvIns             Driver instance.
     * @param   fFlags              PDM_TACH_FLAGS_NOT_HOT_PLUG or 0.
     * @param   ppBaseInterface     Where to store the pointer to the base interface.
     */
    DECLR3CALLBACKMEMBER(int, pfnAttach,(PPDMDRVINS pDrvIns, uint32_t fFlags, PPDMIBASE *ppBaseInterface));

    /**
     * Detach the driver the drivers below us.
     *
     * @returns VBox status code.
     * @param   pDrvIns             Driver instance.
     * @param   fFlags              PDM_TACH_FLAGS_NOT_HOT_PLUG or 0.
     */
    DECLR3CALLBACKMEMBER(int, pfnDetach,(PPDMDRVINS pDrvIns, uint32_t fFlags));

    /**
     * Detach the driver from the driver above it and destroy this
     * driver and all drivers below it.
     *
     * @returns VBox status code.
     * @param   pDrvIns             Driver instance.
     * @param   fFlags              PDM_TACH_FLAGS_NOT_HOT_PLUG or 0.
     */
    DECLR3CALLBACKMEMBER(int, pfnDetachSelf,(PPDMDRVINS pDrvIns, uint32_t fFlags));

    /**
     * Prepare a media mount.
     *
     * The driver must not have anything attached to itself
     * when calling this function as the purpose is to set up the configuration
     * of an future attachment.
     *
     * @returns VBox status code
     * @param   pDrvIns             Driver instance.
     * @param   pszFilename     Pointer to filename. If this is NULL it assumed that the caller have
     *                          constructed a configuration which can be attached to the bottom driver.
     * @param   pszCoreDriver   Core driver name. NULL will cause autodetection. Ignored if pszFilanem is NULL.
     */
    DECLR3CALLBACKMEMBER(int, pfnMountPrepare,(PPDMDRVINS pDrvIns, const char *pszFilename, const char *pszCoreDriver));

    /**
     * Assert that the current thread is the emulation thread.
     *
     * @returns True if correct.
     * @returns False if wrong.
     * @param   pDrvIns         Driver instance.
     * @param   pszFile         Filename of the assertion location.
     * @param   iLine           Linenumber of the assertion location.
     * @param   pszFunction     Function of the assertion location.
     */
    DECLR3CALLBACKMEMBER(bool, pfnAssertEMT,(PPDMDRVINS pDrvIns, const char *pszFile, unsigned iLine, const char *pszFunction));

    /**
     * Assert that the current thread is NOT the emulation thread.
     *
     * @returns True if correct.
     * @returns False if wrong.
     * @param   pDrvIns         Driver instance.
     * @param   pszFile         Filename of the assertion location.
     * @param   iLine           Linenumber of the assertion location.
     * @param   pszFunction     Function of the assertion location.
     */
    DECLR3CALLBACKMEMBER(bool, pfnAssertOther,(PPDMDRVINS pDrvIns, const char *pszFile, unsigned iLine, const char *pszFunction));

    /**
     * Set the VM error message
     *
     * @returns rc.
     * @param   pDrvIns         Driver instance.
     * @param   rc              VBox status code.
     * @param   SRC_POS         Use RT_SRC_POS.
     * @param   pszFormat       Error message format string.
     * @param   va              Error message arguments.
     */
    DECLR3CALLBACKMEMBER(int, pfnVMSetErrorV,(PPDMDRVINS pDrvIns, int rc, RT_SRC_POS_DECL,
                                              const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(6, 0));

    /**
     * Set the VM runtime error message
     *
     * @returns VBox status code.
     * @param   pDrvIns         Driver instance.
     * @param   fFlags          The action flags. See VMSETRTERR_FLAGS_*.
     * @param   pszErrorId      Error ID string.
     * @param   pszFormat       Error message format string.
     * @param   va              Error message arguments.
     */
    DECLR3CALLBACKMEMBER(int, pfnVMSetRuntimeErrorV,(PPDMDRVINS pDrvIns, uint32_t fFlags, const char *pszErrorId,
                                                     const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(4, 0));

    /**
     * Gets the VM state.
     *
     * @returns VM state.
     * @param   pDrvIns         The driver instance.
     * @thread  Any thread (just keep in mind that it's volatile info).
     */
    DECLR3CALLBACKMEMBER(VMSTATE, pfnVMState, (PPDMDRVINS pDrvIns));

    /**
     * Checks if the VM was teleported and hasn't been fully resumed yet.
     *
     * @returns true / false.
     * @param   pDrvIns         The driver instance.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(bool, pfnVMTeleportedAndNotFullyResumedYet,(PPDMDRVINS pDrvIns));

    /**
     * Gets the support driver session.
     *
     * This is intended for working using the semaphore API.
     *
     * @returns Support driver session handle.
     * @param   pDrvIns         The driver instance.
     */
    DECLR3CALLBACKMEMBER(PSUPDRVSESSION, pfnGetSupDrvSession,(PPDMDRVINS pDrvIns));

    /** @name Exported PDM Queue Functions
     * @{ */
    /**
     * Create a queue.
     *
     * @returns VBox status code.
     * @param   pDrvIns             Driver instance.
     * @param   cbItem              Size a queue item.
     * @param   cItems              Number of items in the queue.
     * @param   cMilliesInterval    Number of milliseconds between polling the queue.
     *                              If 0 then the emulation thread will be notified whenever an item arrives.
     * @param   pfnCallback         The consumer function.
     * @param   pszName             The queue base name. The instance number will be
     *                              appended automatically.
     * @param   phQueue             Where to store the queue handle on success.
     * @thread  The emulation thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnQueueCreate,(PPDMDRVINS pDrvIns, uint32_t cbItem, uint32_t cItems, uint32_t cMilliesInterval,
                                              PFNPDMQUEUEDRV pfnCallback, const char *pszName, PDMQUEUEHANDLE *phQueue));

    DECLR3CALLBACKMEMBER(PPDMQUEUEITEMCORE, pfnQueueAlloc,(PPDMDRVINS pDrvIns, PDMQUEUEHANDLE hQueue));
    DECLR3CALLBACKMEMBER(int, pfnQueueInsert,(PPDMDRVINS pDrvIns, PDMQUEUEHANDLE hQueue, PPDMQUEUEITEMCORE pItem));
    DECLR3CALLBACKMEMBER(bool, pfnQueueFlushIfNecessary,(PPDMDRVINS pDrvIns, PDMQUEUEHANDLE hQueue));
    /** @} */

    /**
     * Query the virtual timer frequency.
     *
     * @returns Frequency in Hz.
     * @param   pDrvIns             Driver instance.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(uint64_t, pfnTMGetVirtualFreq,(PPDMDRVINS pDrvIns));

    /**
     * Query the virtual time.
     *
     * @returns The current virtual time.
     * @param   pDrvIns             Driver instance.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(uint64_t, pfnTMGetVirtualTime,(PPDMDRVINS pDrvIns));

    /**
     * Creates a timer.
     *
     * @returns VBox status.
     * @param   pDrvIns         Driver instance.
     * @param   enmClock        The clock to use on this timer.
     * @param   pfnCallback     Callback function.
     * @param   pvUser          The user argument to the callback.
     * @param   fFlags          Timer creation flags, see grp_tm_timer_flags.
     * @param   pszDesc         Pointer to description string which must stay around
     *                          until the timer is fully destroyed (i.e. a bit after TMTimerDestroy()).
     * @param   phTimer         Where to store the timer handle on success.
     * @thread  EMT
     *
     * @todo    Need to add a bunch of timer helpers for this to be useful again.
     *          Will do when required.
     */
    DECLR3CALLBACKMEMBER(int, pfnTimerCreate,(PPDMDRVINS pDrvIns, TMCLOCK enmClock, PFNTMTIMERDRV pfnCallback, void *pvUser,
                                              uint32_t fFlags, const char *pszDesc, PTMTIMERHANDLE phTimer));

    /**
     * Destroys a timer.
     *
     * @returns VBox status.
     * @param   pDrvIns         Driver instance.
     * @param   hTimer          The timer handle to destroy.
     */
    DECLR3CALLBACKMEMBER(int, pfnTimerDestroy,(PPDMDRVINS pDrvIns, TMTIMERHANDLE hTimer));

    /**
     * Register a save state data unit.
     *
     * @returns VBox status.
     * @param   pDrvIns         Driver instance.
     * @param   uVersion        Data layout version number.
     * @param   cbGuess         The approximate amount of data in the unit.
     *                          Only for progress indicators.
     *
     * @param   pfnLivePrep     Prepare live save callback, optional.
     * @param   pfnLiveExec     Execute live save callback, optional.
     * @param   pfnLiveVote     Vote live save callback, optional.
     *
     * @param   pfnSavePrep     Prepare save callback, optional.
     * @param   pfnSaveExec     Execute save callback, optional.
     * @param   pfnSaveDone     Done save callback, optional.
     *
     * @param   pfnLoadPrep     Prepare load callback, optional.
     * @param   pfnLoadExec     Execute load callback, optional.
     * @param   pfnLoadDone     Done load callback, optional.
     */
    DECLR3CALLBACKMEMBER(int, pfnSSMRegister,(PPDMDRVINS pDrvIns, uint32_t uVersion, size_t cbGuess,
                                              PFNSSMDRVLIVEPREP pfnLivePrep, PFNSSMDRVLIVEEXEC pfnLiveExec, PFNSSMDRVLIVEVOTE pfnLiveVote,
                                              PFNSSMDRVSAVEPREP pfnSavePrep, PFNSSMDRVSAVEEXEC pfnSaveExec, PFNSSMDRVSAVEDONE pfnSaveDone,
                                              PFNSSMDRVLOADPREP pfnLoadPrep, PFNSSMDRVLOADEXEC pfnLoadExec, PFNSSMDRVLOADDONE pfnLoadDone));

    /**
     * Deregister a save state data unit.
     *
     * @returns VBox status.
     * @param   pDrvIns         Driver instance.
     * @param   pszName         Data unit name.
     * @param   uInstance       The instance identifier of the data unit.
     *                          This must together with the name be unique.
     */
    DECLR3CALLBACKMEMBER(int, pfnSSMDeregister,(PPDMDRVINS pDrvIns, const char *pszName, uint32_t uInstance));

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
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryPassword,(    PCFGMNODE pNode, const char *pszName, char *pszString, size_t cchString));
    DECLR3CALLBACKMEMBER(int,       pfnCFGMQueryPasswordDef,( PCFGMNODE pNode, const char *pszName, char *pszString, size_t cchString, const char *pszDef));
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
     * Free memory allocated with pfnMMHeapAlloc() and pfnMMHeapAllocZ().
     *
     * @param   pDrvIns             Driver instance.
     * @param   pv                  Pointer to the memory to free.
     */
    DECLR3CALLBACKMEMBER(void, pfnMMHeapFree,(PPDMDRVINS pDrvIns, void *pv));

    /**
     * Register an info handler with DBGF.
     *
     * @returns VBox status code.
     * @param   pDrvIns         Driver instance.
     * @param   pszName         Data unit name.
     * @param   pszDesc         The description of the info and any arguments
     *                          the handler may take.
     * @param   pfnHandler      The handler function to be called to display the
     *                          info.
     */
    DECLR3CALLBACKMEMBER(int, pfnDBGFInfoRegister,(PPDMDRVINS pDrvIns, const char *pszName, const char *pszDesc, PFNDBGFHANDLERDRV pfnHandler));

    /**
     * Register an info handler with DBGF, argv style.
     *
     * @returns VBox status code.
     * @param   pDrvIns         Driver instance.
     * @param   pszName         Data unit name.
     * @param   pszDesc         The description of the info and any arguments
     *                          the handler may take.
     * @param   pfnHandler      The handler function to be called to display the
     *                          info.
     */
    DECLR3CALLBACKMEMBER(int, pfnDBGFInfoRegisterArgv,(PPDMDRVINS pDrvIns, const char *pszName, const char *pszDesc, PFNDBGFINFOARGVDRV pfnHandler));

    /**
     * Deregister an info handler from DBGF.
     *
     * @returns VBox status code.
     * @param   pDrvIns         Driver instance.
     * @param   pszName         Data unit name.
     */
    DECLR3CALLBACKMEMBER(int, pfnDBGFInfoDeregister,(PPDMDRVINS pDrvIns, const char *pszName));

    /**
     * Registers a statistics sample if statistics are enabled.
     *
     * @param   pDrvIns     Driver instance.
     * @param   pvSample    Pointer to the sample.
     * @param   enmType     Sample type. This indicates what pvSample is pointing at.
     * @param   pszName     Sample name. The name is on this form "/<component>/<sample>".
     *                      Further nesting is possible.  If this does not start
     *                      with a '/', the default prefix will be prepended,
     *                      otherwise it will be used as-is.
     * @param   enmUnit     Sample unit.
     * @param   pszDesc     Sample description.
     */
    DECLR3CALLBACKMEMBER(void, pfnSTAMRegister,(PPDMDRVINS pDrvIns, void *pvSample, STAMTYPE enmType, const char *pszName,
                                                STAMUNIT enmUnit, const char *pszDesc));

    /**
     * Same as pfnSTAMRegister except that the name is specified in a
     * RTStrPrintf like fashion.
     *
     * @param   pDrvIns     Driver instance.
     * @param   pvSample    Pointer to the sample.
     * @param   enmType     Sample type. This indicates what pvSample is pointing at.
     * @param   enmVisibility  Visibility type specifying whether unused statistics should be visible or not.
     * @param   enmUnit     Sample unit.
     * @param   pszDesc     Sample description.
     * @param   pszName     The sample name format string.  If this does not start
     *                      with a '/', the default prefix will be prepended,
     *                      otherwise it will be used as-is.
     * @param   ...         Arguments to the format string.
     */
    DECLR3CALLBACKMEMBER(void, pfnSTAMRegisterF,(PPDMDRVINS pDrvIns, void *pvSample, STAMTYPE enmType, STAMVISIBILITY enmVisibility,
                                                 STAMUNIT enmUnit, const char *pszDesc,
                                                 const char *pszName, ...) RT_IPRT_FORMAT_ATTR(7, 8));

    /**
     * Same as pfnSTAMRegister except that the name is specified in a
     * RTStrPrintfV like fashion.
     *
     * @param   pDrvIns         Driver instance.
     * @param   pvSample        Pointer to the sample.
     * @param   enmType         Sample type. This indicates what pvSample is pointing at.
     * @param   enmVisibility   Visibility type specifying whether unused statistics should be visible or not.
     * @param   enmUnit         Sample unit.
     * @param   pszDesc         Sample description.
     * @param   pszName         The sample name format string.  If this does not
     *                          start with a '/', the default prefix will be prepended,
     *                          otherwise it will be used as-is.
     * @param   args            Arguments to the format string.
     */
    DECLR3CALLBACKMEMBER(void, pfnSTAMRegisterV,(PPDMDRVINS pDrvIns, void *pvSample, STAMTYPE enmType, STAMVISIBILITY enmVisibility,
                                                 STAMUNIT enmUnit, const char *pszDesc,
                                                 const char *pszName, va_list args) RT_IPRT_FORMAT_ATTR(7, 0));

    /**
     * Deregister a statistic item previously registered with pfnSTAMRegister,
     * pfnSTAMRegisterF or pfnSTAMRegisterV
     *
     * @returns VBox status.
     * @param   pDrvIns         Driver instance.
     * @param   pvSample        Pointer to the sample.
     */
    DECLR3CALLBACKMEMBER(int, pfnSTAMDeregister,(PPDMDRVINS pDrvIns, void *pvSample));

    /**
     * Calls the HC R0 VMM entry point, in a safer but slower manner than
     * SUPR3CallVMMR0.
     *
     * When entering using this call the R0 components can call into the host kernel
     * (i.e. use the SUPR0 and RT APIs).
     *
     * See VMMR0Entry() for more details.
     *
     * @returns error code specific to uFunction.
     * @param   pDrvIns     The driver instance.
     * @param   uOperation  Operation to execute.
     *                      This is limited to services.
     * @param   pvArg       Pointer to argument structure or if cbArg is 0 just an value.
     * @param   cbArg       The size of the argument. This is used to copy whatever the argument
     *                      points at into a kernel buffer to avoid problems like the user page
     *                      being invalidated while we're executing the call.
     */
    DECLR3CALLBACKMEMBER(int, pfnSUPCallVMMR0Ex,(PPDMDRVINS pDrvIns, unsigned uOperation, void *pvArg, unsigned cbArg));

    /**
     * Registers a USB HUB.
     *
     * @returns VBox status code.
     * @param   pDrvIns         The driver instance.
     * @param   fVersions       Indicates the kinds of USB devices that can be attached to this HUB.
     * @param   cPorts          The number of ports.
     * @param   pUsbHubReg      The hub callback structure that PDMUsb uses to interact with it.
     * @param   ppUsbHubHlp     The helper callback structure that the hub uses to talk to PDMUsb.
     *
     * @thread  EMT.
     */
    DECLR3CALLBACKMEMBER(int, pfnUSBRegisterHub,(PPDMDRVINS pDrvIns, uint32_t fVersions, uint32_t cPorts, PCPDMUSBHUBREG pUsbHubReg, PPCPDMUSBHUBHLP ppUsbHubHlp));

    /**
     * Set up asynchronous handling of a suspend, reset or power off notification.
     *
     * This shall only be called when getting the notification.  It must be called
     * for each one.
     *
     * @returns VBox status code.
     * @param   pDrvIns             The driver instance.
     * @param   pfnAsyncNotify      The callback.
     * @thread  EMT(0)
     */
    DECLR3CALLBACKMEMBER(int, pfnSetAsyncNotification, (PPDMDRVINS pDrvIns, PFNPDMDRVASYNCNOTIFY pfnAsyncNotify));

    /**
     * Notify EMT(0) that the driver has completed the asynchronous notification
     * handling.
     *
     * This can be called at any time, spurious calls will simply be ignored.
     *
     * @param   pDrvIns             The driver instance.
     * @thread  Any
     */
    DECLR3CALLBACKMEMBER(void, pfnAsyncNotificationCompleted, (PPDMDRVINS pDrvIns));

    /**
     * Creates a PDM thread.
     *
     * This differs from the RTThreadCreate() API in that PDM takes care of suspending,
     * resuming, and destroying the thread as the VM state changes.
     *
     * @returns VBox status code.
     * @param   pDrvIns     The driver instance.
     * @param   ppThread    Where to store the thread 'handle'.
     * @param   pvUser      The user argument to the thread function.
     * @param   pfnThread   The thread function.
     * @param   pfnWakeup   The wakup callback. This is called on the EMT thread when
     *                      a state change is pending.
     * @param   cbStack     See RTThreadCreate.
     * @param   enmType     See RTThreadCreate.
     * @param   pszName     See RTThreadCreate.
     */
    DECLR3CALLBACKMEMBER(int, pfnThreadCreate,(PPDMDRVINS pDrvIns, PPPDMTHREAD ppThread, void *pvUser, PFNPDMTHREADDRV pfnThread,
                                               PFNPDMTHREADWAKEUPDRV pfnWakeup, size_t cbStack, RTTHREADTYPE enmType, const char *pszName));

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
     * Creates an async completion template for a driver instance.
     *
     * The template is used when creating new completion tasks.
     *
     * @returns VBox status code.
     * @param   pDrvIns         The driver instance.
     * @param   ppTemplate      Where to store the template pointer on success.
     * @param   pfnCompleted    The completion callback routine.
     * @param   pvTemplateUser  Template user argument.
     * @param   pszDesc         Description.
     */
    DECLR3CALLBACKMEMBER(int, pfnAsyncCompletionTemplateCreate,(PPDMDRVINS pDrvIns, PPPDMASYNCCOMPLETIONTEMPLATE ppTemplate,
                                                                PFNPDMASYNCCOMPLETEDRV pfnCompleted, void *pvTemplateUser,
                                                                const char *pszDesc));

    /** @name Exported PDM Async Completion Functions
     * @{ */
    DECLR3CALLBACKMEMBER(int, pfnAsyncCompletionTemplateDestroy,(PPDMASYNCCOMPLETIONTEMPLATE pTemplate));
    DECLR3CALLBACKMEMBER(int, pfnAsyncCompletionEpCreateForFile,(PPPDMASYNCCOMPLETIONENDPOINT ppEndpoint,
                                                                 const char *pszFilename, uint32_t fFlags,
                                                                 PPDMASYNCCOMPLETIONTEMPLATE pTemplate));
    DECLR3CALLBACKMEMBER(void, pfnAsyncCompletionEpClose,(PPDMASYNCCOMPLETIONENDPOINT pEndpoint));
    DECLR3CALLBACKMEMBER(int, pfnAsyncCompletionEpGetSize,(PPDMASYNCCOMPLETIONENDPOINT pEndpoint, uint64_t *pcbSize));
    DECLR3CALLBACKMEMBER(int, pfnAsyncCompletionEpSetSize,(PPDMASYNCCOMPLETIONENDPOINT pEndpoint, uint64_t cbSize));
    DECLR3CALLBACKMEMBER(int, pfnAsyncCompletionEpSetBwMgr,(PPDMASYNCCOMPLETIONENDPOINT pEndpoint, const char *pszBwMgr));
    DECLR3CALLBACKMEMBER(int, pfnAsyncCompletionEpFlush,(PPDMASYNCCOMPLETIONENDPOINT pEndpoint, void *pvUser, PPPDMASYNCCOMPLETIONTASK ppTask));
    DECLR3CALLBACKMEMBER(int, pfnAsyncCompletionEpRead,(PPDMASYNCCOMPLETIONENDPOINT pEndpoint, RTFOFF off,
                                                        PCRTSGSEG paSegments, unsigned cSegments,
                                                        size_t cbRead, void *pvUser,
                                                        PPPDMASYNCCOMPLETIONTASK ppTask));
    DECLR3CALLBACKMEMBER(int, pfnAsyncCompletionEpWrite,(PPDMASYNCCOMPLETIONENDPOINT pEndpoint, RTFOFF off,
                                                         PCRTSGSEG paSegments, unsigned cSegments,
                                                         size_t cbWrite, void *pvUser,
                                                         PPPDMASYNCCOMPLETIONTASK ppTask));
    /** @} */


    /**
     * Attaches a network filter driver to a named bandwidth group.
     *
     * @returns VBox status code.
     * @retval  VERR_ALREADY_INITIALIZED if already attached to a group.
     * @param   pDrvIns         The driver instance.
     * @param   pszBwGroup      Name of the bandwidth group to attach to.
     * @param   pFilter         Pointer to the filter we attach.
     */
    DECLR3CALLBACKMEMBER(int, pfnNetShaperAttach,(PPDMDRVINS pDrvIns, const char *pszBwGroup, PPDMNSFILTER pFilter));

    /**
     * Detaches a network filter driver from its current bandwidth group (if any).
     *
     * @returns VBox status code.
     * @param   pDrvIns         The driver instance.
     * @param   pFilter         Pointer to the filter we attach.
     */
    DECLR3CALLBACKMEMBER(int, pfnNetShaperDetach,(PPDMDRVINS pDrvIns, PPDMNSFILTER pFilter));

    /**
     * Obtains bandwidth in a bandwidth group.
     *
     * @returns True if bandwidth was allocated, false if not.
     * @param   pDrvIns         The driver instance.
     * @param   pFilter         Pointer to the filter that allocates bandwidth.
     * @param   cbTransfer      Number of bytes to allocate.
     */
    DECLR3CALLBACKMEMBER(bool, pfnNetShaperAllocateBandwidth,(PPDMDRVINS pDrvIns, PPDMNSFILTER pFilter, size_t cbTransfer));

    /**
     * Resolves the symbol for a raw-mode context interface.
     *
     * @returns VBox status code.
     * @param   pDrvIns         The driver instance.
     * @param   pvInterface     The interface structure.
     * @param   cbInterface     The size of the interface structure.
     * @param   pszSymPrefix    What to prefix the symbols in the list with before
     *                          resolving them.  This must start with 'drv' and
     *                          contain the driver name.
     * @param   pszSymList      List of symbols corresponding to the interface.
     *                          There is generally a there is generally a define
     *                          holding this list associated with the interface
     *                          definition (INTERFACE_SYM_LIST).  For more details
     *                          see PDMR3LdrGetInterfaceSymbols.
     * @thread  EMT
     */
    DECLR3CALLBACKMEMBER(int, pfnLdrGetRCInterfaceSymbols,(PPDMDRVINS pDrvIns, void *pvInterface, size_t cbInterface,
                                                           const char *pszSymPrefix, const char *pszSymList));

    /**
     * Resolves the symbol for a ring-0 context interface.
     *
     * @returns VBox status code.
     * @param   pDrvIns         The driver instance.
     * @param   pvInterface     The interface structure.
     * @param   cbInterface     The size of the interface structure.
     * @param   pszSymPrefix    What to prefix the symbols in the list with before
     *                          resolving them.  This must start with 'drv' and
     *                          contain the driver name.
     * @param   pszSymList      List of symbols corresponding to the interface.
     *                          There is generally a there is generally a define
     *                          holding this list associated with the interface
     *                          definition (INTERFACE_SYM_LIST).  For more details
     *                          see PDMR3LdrGetInterfaceSymbols.
     * @thread  EMT
     */
    DECLR3CALLBACKMEMBER(int, pfnLdrGetR0InterfaceSymbols,(PPDMDRVINS pDrvIns, void *pvInterface, size_t cbInterface,
                                                           const char *pszSymPrefix, const char *pszSymList));
    /**
     * Initializes a PDM critical section.
     *
     * The PDM critical sections are derived from the IPRT critical sections, but
     * works in both RC and R0 as well as R3.
     *
     * @returns VBox status code.
     * @param   pDrvIns             The driver instance.
     * @param   pCritSect           Pointer to the critical section.
     * @param   SRC_POS             Use RT_SRC_POS.
     * @param   pszName             The base name of the critical section.  Will be
     *                              mangeled with the instance number.  For
     *                              statistics and lock validation.
     * @thread  EMT
     */
    DECLR3CALLBACKMEMBER(int, pfnCritSectInit,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect, RT_SRC_POS_DECL, const char *pszName));

    /** @name Exported PDM Critical Section Functions
     * @{ */
    DECLR3CALLBACKMEMBER(bool,     pfnCritSectYield,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect));
    DECLR3CALLBACKMEMBER(int,      pfnCritSectEnter,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect, int rcBusy));
    DECLR3CALLBACKMEMBER(int,      pfnCritSectEnterDebug,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect, int rcBusy, RTHCUINTPTR uId, RT_SRC_POS_DECL));
    DECLR3CALLBACKMEMBER(int,      pfnCritSectTryEnter,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect));
    DECLR3CALLBACKMEMBER(int,      pfnCritSectTryEnterDebug,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect, RTHCUINTPTR uId, RT_SRC_POS_DECL));
    DECLR3CALLBACKMEMBER(int,      pfnCritSectLeave,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect));
    DECLR3CALLBACKMEMBER(bool,     pfnCritSectIsOwner,(PPDMDRVINS pDrvIns, PCPDMCRITSECT pCritSect));
    DECLR3CALLBACKMEMBER(bool,     pfnCritSectIsInitialized,(PPDMDRVINS pDrvIns, PCPDMCRITSECT pCritSect));
    DECLR3CALLBACKMEMBER(bool,     pfnCritSectHasWaiters,(PPDMDRVINS pDrvIns, PCPDMCRITSECT pCritSect));
    DECLR3CALLBACKMEMBER(uint32_t, pfnCritSectGetRecursion,(PPDMDRVINS pDrvIns, PCPDMCRITSECT pCritSect));
    DECLR3CALLBACKMEMBER(int,      pfnCritSectScheduleExitEvent,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect, SUPSEMEVENT hEventToSignal));
    DECLR3CALLBACKMEMBER(int,      pfnCritSectDelete,(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect));
    /** @} */

    /**
     * Call the ring-0 request handler routine of the driver.
     *
     * For this to work, the driver must be ring-0 enabled and export a request
     * handler function.  The name of the function must be the driver name in the
     * PDMDRVREG struct prefixed with 'drvR0' and suffixed with 'ReqHandler'.
     * The driver name will be capitalized.  It shall take the exact same
     * arguments as this function and be declared using PDMBOTHCBDECL.  See
     * FNPDMDRVREQHANDLERR0.
     *
     * @returns VBox status code.
     * @retval  VERR_SYMBOL_NOT_FOUND if the driver doesn't export the required
     *          handler function.
     * @retval  VERR_ACCESS_DENIED if the driver isn't ring-0 capable.
     *
     * @param   pDrvIns             The driver instance.
     * @param   uOperation          The operation to perform.
     * @param   u64Arg              64-bit integer argument.
     * @thread  Any
     */
    DECLR3CALLBACKMEMBER(int, pfnCallR0,(PPDMDRVINS pDrvIns, uint32_t uOperation, uint64_t u64Arg));

    /**
     * Creates a block cache for a driver driver instance.
     *
     * @returns VBox status code.
     * @param   pDrvIns             The driver instance.
     * @param   ppBlkCache          Where to store the handle to the block cache.
     * @param   pfnXferComplete     The I/O transfer complete callback.
     * @param   pfnXferEnqueue      The I/O request enqueue callback.
     * @param   pfnXferEnqueueDiscard   The discard request enqueue callback.
     * @param   pcszId              Unique ID used to identify the user.
     */
    DECLR3CALLBACKMEMBER(int, pfnBlkCacheRetain, (PPDMDRVINS pDrvIns, PPPDMBLKCACHE ppBlkCache,
                                                  PFNPDMBLKCACHEXFERCOMPLETEDRV pfnXferComplete,
                                                  PFNPDMBLKCACHEXFERENQUEUEDRV pfnXferEnqueue,
                                                  PFNPDMBLKCACHEXFERENQUEUEDISCARDDRV pfnXferEnqueueDiscard,
                                                  const char *pcszId));

    /** @name Exported PDM Block Cache Functions
     * @{ */
    DECLR3CALLBACKMEMBER(void,     pfnBlkCacheRelease,(PPDMBLKCACHE pBlkCache));
    DECLR3CALLBACKMEMBER(int,      pfnBlkCacheClear,(PPDMBLKCACHE pBlkCache));
    DECLR3CALLBACKMEMBER(int,      pfnBlkCacheSuspend,(PPDMBLKCACHE pBlkCache));
    DECLR3CALLBACKMEMBER(int,      pfnBlkCacheResume,(PPDMBLKCACHE pBlkCache));
    DECLR3CALLBACKMEMBER(void,     pfnBlkCacheIoXferComplete,(PPDMBLKCACHE pBlkCache, PPDMBLKCACHEIOXFER hIoXfer, int rcIoXfer));
    DECLR3CALLBACKMEMBER(int,      pfnBlkCacheRead,(PPDMBLKCACHE pBlkCache, uint64_t off, PCRTSGBUF pSgBuf, size_t cbRead, void *pvUser));
    DECLR3CALLBACKMEMBER(int,      pfnBlkCacheWrite,(PPDMBLKCACHE pBlkCache, uint64_t off, PCRTSGBUF pSgBuf, size_t cbRead, void *pvUser));
    DECLR3CALLBACKMEMBER(int,      pfnBlkCacheFlush,(PPDMBLKCACHE pBlkCache, void *pvUser));
    DECLR3CALLBACKMEMBER(int,      pfnBlkCacheDiscard,(PPDMBLKCACHE pBlkCache, PCRTRANGE paRanges, unsigned cRanges, void *pvUser));
    /** @} */

    /**
     * Gets the reason for the most recent VM suspend.
     *
     * @returns The suspend reason. VMSUSPENDREASON_INVALID is returned if no
     *          suspend has been made or if the pDrvIns is invalid.
     * @param   pDrvIns             The driver instance.
     */
    DECLR3CALLBACKMEMBER(VMSUSPENDREASON, pfnVMGetSuspendReason,(PPDMDRVINS pDrvIns));

    /**
     * Gets the reason for the most recent VM resume.
     *
     * @returns The resume reason. VMRESUMEREASON_INVALID is returned if no
     *          resume has been made or if the pDrvIns is invalid.
     * @param   pDrvIns             The driver instance.
     */
    DECLR3CALLBACKMEMBER(VMRESUMEREASON, pfnVMGetResumeReason,(PPDMDRVINS pDrvIns));

    /** @name Space reserved for minor interface changes.
     * @{ */
    DECLR3CALLBACKMEMBER(int,  pfnTimerSetMillies,(PPDMDRVINS pDrvIns, TMTIMERHANDLE hTimer, uint64_t cMilliesToNext));

    /**
     * Deregister zero or more samples given their name prefix.
     *
     * @returns VBox status code.
     * @param   pDrvIns     The driver instance.
     * @param   pszPrefix   The name prefix of the samples to remove.  If this does
     *                      not start with a '/', the default prefix will be
     *                      prepended, otherwise it will be used as-is.
     */
    DECLR3CALLBACKMEMBER(int, pfnSTAMDeregisterByPrefix,(PPDMDRVINS pDrvIns, const char *pszPrefix));

    /**
     * Queries a generic object from the VMM user.
     *
     * @returns Pointer to the object if found, NULL if not.
     * @param   pDrvIns     The driver instance.
     * @param   pUuid       The UUID of what's being queried.  The UUIDs and
     *                      the usage conventions are defined by the user.
     */
    DECLR3CALLBACKMEMBER(void *, pfnQueryGenericUserObject,(PPDMDRVINS pDrvIns, PCRTUUID pUuid));

    DECLR3CALLBACKMEMBER(void, pfnReserved0,(PPDMDRVINS pDrvIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved1,(PPDMDRVINS pDrvIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved2,(PPDMDRVINS pDrvIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved3,(PPDMDRVINS pDrvIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved4,(PPDMDRVINS pDrvIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved5,(PPDMDRVINS pDrvIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved6,(PPDMDRVINS pDrvIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved7,(PPDMDRVINS pDrvIns));
    DECLR3CALLBACKMEMBER(void, pfnReserved8,(PPDMDRVINS pDrvIns));
    /** @}  */

    /** Just a safety precaution. */
    uint32_t                        u32TheEnd;
} PDMDRVHLPR3;
/** Current DRVHLP version number. */
#define PDM_DRVHLPR3_VERSION                    PDM_VERSION_MAKE(0xf0fb, 16, 0)


/**
 * Set the VM error message
 *
 * @returns rc.
 * @param   pDrvIns         Driver instance.
 * @param   rc              VBox status code.
 * @param   SRC_POS         Use RT_SRC_POS.
 * @param   pszFormat       Error message format string.
 * @param   ...             Error message arguments.
 * @sa      PDMDRV_SET_ERROR, PDMDrvHlpVMSetErrorV, VMSetError
 */
DECLINLINE(int)  RT_IPRT_FORMAT_ATTR(6, 7) PDMDrvHlpVMSetError(PPDMDRVINS pDrvIns, const int rc, RT_SRC_POS_DECL,
                                                               const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    pDrvIns->CTX_SUFF(pHlp)->pfnVMSetErrorV(pDrvIns, rc, RT_SRC_POS_ARGS, pszFormat, va);
    va_end(va);
    return rc;
}

/** @def PDMDRV_SET_ERROR
 * Set the VM error. See PDMDrvHlpVMSetError() for printf like message formatting.
 */
#define PDMDRV_SET_ERROR(pDrvIns, rc, pszError)  \
    PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS, "%s", pszError)

/**
 * @copydoc PDMDRVHLPR3::pfnVMSetErrorV
 */
DECLINLINE(int)  RT_IPRT_FORMAT_ATTR(6, 0) PDMDrvHlpVMSetErrorV(PPDMDRVINS pDrvIns, const int rc, RT_SRC_POS_DECL,
                                                                const char *pszFormat, va_list va)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnVMSetErrorV(pDrvIns, rc, RT_SRC_POS_ARGS, pszFormat, va);
}


/**
 * Set the VM runtime error message
 *
 * @returns VBox status code.
 * @param   pDrvIns         Driver instance.
 * @param   fFlags          The action flags. See VMSETRTERR_FLAGS_*.
 * @param   pszErrorId      Error ID string.
 * @param   pszFormat       Error message format string.
 * @param   ...             Error message arguments.
 * @sa      PDMDRV_SET_RUNTIME_ERROR, PDMDrvHlpVMSetRuntimeErrorV,
 *          VMSetRuntimeError
 */
DECLINLINE(int)  RT_IPRT_FORMAT_ATTR(4, 5) PDMDrvHlpVMSetRuntimeError(PPDMDRVINS pDrvIns, uint32_t fFlags, const char *pszErrorId,
                                                                      const char *pszFormat, ...)
{
    va_list va;
    int rc;
    va_start(va, pszFormat);
    rc = pDrvIns->CTX_SUFF(pHlp)->pfnVMSetRuntimeErrorV(pDrvIns, fFlags, pszErrorId, pszFormat, va);
    va_end(va);
    return rc;
}

/** @def PDMDRV_SET_RUNTIME_ERROR
 * Set the VM runtime error. See PDMDrvHlpVMSetRuntimeError() for printf like message formatting.
 */
#define PDMDRV_SET_RUNTIME_ERROR(pDrvIns, fFlags, pszErrorId, pszError)  \
    PDMDrvHlpVMSetRuntimeError(pDrvIns, fFlags, pszErrorId, "%s", pszError)

/**
 * @copydoc PDMDRVHLPR3::pfnVMSetRuntimeErrorV
 */
DECLINLINE(int)  RT_IPRT_FORMAT_ATTR(4, 0) PDMDrvHlpVMSetRuntimeErrorV(PPDMDRVINS pDrvIns, uint32_t fFlags,
                                                                       const char *pszErrorId,  const char *pszFormat, va_list va)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnVMSetRuntimeErrorV(pDrvIns, fFlags, pszErrorId, pszFormat, va);
}

#endif /* IN_RING3 */

/** @def PDMDRV_ASSERT_EMT
 * Assert that the current thread is the emulation thread.
 */
#ifdef VBOX_STRICT
# define PDMDRV_ASSERT_EMT(pDrvIns)  pDrvIns->CTX_SUFF(pHlp)->pfnAssertEMT(pDrvIns, __FILE__, __LINE__, __FUNCTION__)
#else
# define PDMDRV_ASSERT_EMT(pDrvIns)  do { } while (0)
#endif

/** @def PDMDRV_ASSERT_OTHER
 * Assert that the current thread is NOT the emulation thread.
 */
#ifdef VBOX_STRICT
# define PDMDRV_ASSERT_OTHER(pDrvIns)  pDrvIns->CTX_SUFF(pHlp)->pfnAssertOther(pDrvIns, __FILE__, __LINE__, __FUNCTION__)
#else
# define PDMDRV_ASSERT_OTHER(pDrvIns)  do { } while (0)
#endif


#ifdef IN_RING3

/**
 * @copydoc PDMDRVHLPR3::pfnAttach
 */
DECLINLINE(int) PDMDrvHlpAttach(PPDMDRVINS pDrvIns, uint32_t fFlags, PPDMIBASE *ppBaseInterface)
{
    return pDrvIns->pHlpR3->pfnAttach(pDrvIns, fFlags, ppBaseInterface);
}

/**
 * Check that there is no driver below the us that we should attach to.
 *
 * @returns VERR_PDM_NO_ATTACHED_DRIVER if there is no driver.
 * @param   pDrvIns     The driver instance.
 */
DECLINLINE(int) PDMDrvHlpNoAttach(PPDMDRVINS pDrvIns)
{
    return pDrvIns->pHlpR3->pfnAttach(pDrvIns, 0, NULL);
}

/**
 * @copydoc PDMDRVHLPR3::pfnDetach
 */
DECLINLINE(int) PDMDrvHlpDetach(PPDMDRVINS pDrvIns, uint32_t fFlags)
{
    return pDrvIns->pHlpR3->pfnDetach(pDrvIns, fFlags);
}

/**
 * @copydoc PDMDRVHLPR3::pfnDetachSelf
 */
DECLINLINE(int) PDMDrvHlpDetachSelf(PPDMDRVINS pDrvIns, uint32_t fFlags)
{
    return pDrvIns->pHlpR3->pfnDetachSelf(pDrvIns, fFlags);
}

/**
 * @copydoc PDMDRVHLPR3::pfnMountPrepare
 */
DECLINLINE(int) PDMDrvHlpMountPrepare(PPDMDRVINS pDrvIns, const char *pszFilename, const char *pszCoreDriver)
{
    return pDrvIns->pHlpR3->pfnMountPrepare(pDrvIns, pszFilename, pszCoreDriver);
}

/**
 * @copydoc PDMDRVHLPR3::pfnVMState
 */
DECLINLINE(VMSTATE) PDMDrvHlpVMState(PPDMDRVINS pDrvIns)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnVMState(pDrvIns);
}

/**
 * @copydoc PDMDRVHLPR3::pfnVMTeleportedAndNotFullyResumedYet
 */
DECLINLINE(bool) PDMDrvHlpVMTeleportedAndNotFullyResumedYet(PPDMDRVINS pDrvIns)
{
    return pDrvIns->pHlpR3->pfnVMTeleportedAndNotFullyResumedYet(pDrvIns);
}

/**
 * @copydoc PDMDRVHLPR3::pfnGetSupDrvSession
 */
DECLINLINE(PSUPDRVSESSION) PDMDrvHlpGetSupDrvSession(PPDMDRVINS pDrvIns)
{
    return pDrvIns->pHlpR3->pfnGetSupDrvSession(pDrvIns);
}

/**
 * @copydoc PDMDRVHLPR3::pfnQueueCreate
 */
DECLINLINE(int) PDMDrvHlpQueueCreate(PPDMDRVINS pDrvIns, uint32_t cbItem, uint32_t cItems, uint32_t cMilliesInterval,
                                        PFNPDMQUEUEDRV pfnCallback, const char *pszName, PDMQUEUEHANDLE *phQueue)
{
    return pDrvIns->pHlpR3->pfnQueueCreate(pDrvIns, cbItem, cItems, cMilliesInterval, pfnCallback, pszName, phQueue);
}

/**
 * @copydoc PDMDRVHLPR3::pfnQueueAlloc
 */
DECLINLINE(PPDMQUEUEITEMCORE) PDMDrvHlpQueueAlloc(PPDMDRVINS pDrvIns, PDMQUEUEHANDLE hQueue)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnQueueAlloc(pDrvIns, hQueue);
}

/**
 * @copydoc PDMDRVHLPR3::pfnQueueInsert
 */
DECLINLINE(int) PDMDrvHlpQueueInsert(PPDMDRVINS pDrvIns, PDMQUEUEHANDLE hQueue, PPDMQUEUEITEMCORE pItem)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnQueueInsert(pDrvIns, hQueue, pItem);
}

/**
 * @copydoc PDMDRVHLPR3::pfnQueueFlushIfNecessary
 */
DECLINLINE(bool) PDMDrvHlpQueueFlushIfNecessary(PPDMDRVINS pDrvIns, PDMQUEUEHANDLE hQueue)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnQueueFlushIfNecessary(pDrvIns, hQueue);
}

/**
 * @copydoc PDMDRVHLPR3::pfnTMGetVirtualFreq
 */
DECLINLINE(uint64_t) PDMDrvHlpTMGetVirtualFreq(PPDMDRVINS pDrvIns)
{
    return pDrvIns->pHlpR3->pfnTMGetVirtualFreq(pDrvIns);
}

/**
 * @copydoc PDMDRVHLPR3::pfnTMGetVirtualTime
 */
DECLINLINE(uint64_t) PDMDrvHlpTMGetVirtualTime(PPDMDRVINS pDrvIns)
{
    return pDrvIns->pHlpR3->pfnTMGetVirtualTime(pDrvIns);
}

/**
 * @copydoc PDMDRVHLPR3::pfnTimerCreate
 */
DECLINLINE(int) PDMDrvHlpTMTimerCreate(PPDMDRVINS pDrvIns, TMCLOCK enmClock, PFNTMTIMERDRV pfnCallback, void *pvUser,
                                       uint32_t fFlags, const char *pszDesc, PTMTIMERHANDLE phTimer)

{
    return pDrvIns->pHlpR3->pfnTimerCreate(pDrvIns, enmClock, pfnCallback, pvUser, fFlags, pszDesc, phTimer);
}

/**
 * @copydoc PDMDRVHLPR3::pfnTimerDestroy
 */
DECLINLINE(int) PDMDrvHlpTimerDestroy(PPDMDRVINS pDrvIns, TMTIMERHANDLE hTimer)

{
    return pDrvIns->pHlpR3->pfnTimerDestroy(pDrvIns, hTimer);
}

/**
 * @copydoc PDMDRVHLPR3::pfnTimerSetMillies
 */
DECLINLINE(int) PDMDrvHlpTimerSetMillies(PPDMDRVINS pDrvIns, TMTIMERHANDLE hTimer, uint64_t cMilliesToNext)

{
    return pDrvIns->pHlpR3->pfnTimerSetMillies(pDrvIns, hTimer, cMilliesToNext);
}

/**
 * Register a save state data unit.
 *
 * @returns VBox status.
 * @param   pDrvIns         Driver instance.
 * @param   uVersion        Data layout version number.
 * @param   cbGuess         The approximate amount of data in the unit.
 *                          Only for progress indicators.
 * @param   pfnSaveExec     Execute save callback, optional.
 * @param   pfnLoadExec     Execute load callback, optional.
 */
DECLINLINE(int) PDMDrvHlpSSMRegister(PPDMDRVINS pDrvIns, uint32_t uVersion, size_t cbGuess,
                                     PFNSSMDRVSAVEEXEC pfnSaveExec, PFNSSMDRVLOADEXEC pfnLoadExec)
{
    return pDrvIns->pHlpR3->pfnSSMRegister(pDrvIns, uVersion, cbGuess,
                                              NULL /*pfnLivePrep*/, NULL /*pfnLiveExec*/, NULL /*pfnLiveVote*/,
                                              NULL /*pfnSavePrep*/, pfnSaveExec,          NULL /*pfnSaveDone*/,
                                              NULL /*pfnLoadPrep*/, pfnLoadExec,          NULL /*pfnLoadDone*/);
}

/**
 * @copydoc PDMDRVHLPR3::pfnSSMRegister
 */
DECLINLINE(int) PDMDrvHlpSSMRegisterEx(PPDMDRVINS pDrvIns, uint32_t uVersion, size_t cbGuess,
                                       PFNSSMDRVLIVEPREP pfnLivePrep, PFNSSMDRVLIVEEXEC pfnLiveExec, PFNSSMDRVLIVEVOTE pfnLiveVote,
                                       PFNSSMDRVSAVEPREP pfnSavePrep, PFNSSMDRVSAVEEXEC pfnSaveExec, PFNSSMDRVSAVEDONE pfnSaveDone,
                                       PFNSSMDRVLOADPREP pfnLoadPrep, PFNSSMDRVLOADEXEC pfnLoadExec, PFNSSMDRVLOADDONE pfnLoadDone)
{
    return pDrvIns->pHlpR3->pfnSSMRegister(pDrvIns, uVersion, cbGuess,
                                              pfnLivePrep, pfnLiveExec, pfnLiveVote,
                                              pfnSavePrep, pfnSaveExec, pfnSaveDone,
                                              pfnLoadPrep, pfnLoadExec, pfnLoadDone);
}

/**
 * Register a load done callback.
 *
 * @returns VBox status.
 * @param   pDrvIns         Driver instance.
 * @param   pfnLoadDone         Done load callback, optional.
 */
DECLINLINE(int) PDMDrvHlpSSMRegisterLoadDone(PPDMDRVINS pDrvIns, PFNSSMDRVLOADDONE pfnLoadDone)
{
    return pDrvIns->pHlpR3->pfnSSMRegister(pDrvIns, 0 /*uVersion*/, 0 /*cbGuess*/,
                                              NULL /*pfnLivePrep*/, NULL /*pfnLiveExec*/, NULL /*pfnLiveVote*/,
                                              NULL /*pfnSavePrep*/, NULL /*pfnSaveExec*/, NULL /*pfnSaveDone*/,
                                              NULL /*pfnLoadPrep*/, NULL /*pfnLoadExec*/, pfnLoadDone);
}

/**
 * @copydoc PDMDRVHLPR3::pfnMMHeapFree
 */
DECLINLINE(void) PDMDrvHlpMMHeapFree(PPDMDRVINS pDrvIns, void *pv)
{
    pDrvIns->pHlpR3->pfnMMHeapFree(pDrvIns, pv);
}

/**
 * @copydoc PDMDRVHLPR3::pfnDBGFInfoRegister
 */
DECLINLINE(int) PDMDrvHlpDBGFInfoRegister(PPDMDRVINS pDrvIns, const char *pszName, const char *pszDesc, PFNDBGFHANDLERDRV pfnHandler)
{
    return pDrvIns->pHlpR3->pfnDBGFInfoRegister(pDrvIns, pszName, pszDesc, pfnHandler);
}

/**
 * @copydoc PDMDRVHLPR3::pfnDBGFInfoRegisterArgv
 */
DECLINLINE(int) PDMDrvHlpDBGFInfoRegisterArgv(PPDMDRVINS pDrvIns, const char *pszName, const char *pszDesc, PFNDBGFINFOARGVDRV pfnHandler)
{
    return pDrvIns->pHlpR3->pfnDBGFInfoRegisterArgv(pDrvIns, pszName, pszDesc, pfnHandler);
}

/**
 * @copydoc PDMDRVHLPR3::pfnDBGFInfoRegister
 */
DECLINLINE(int) PDMDrvHlpDBGFInfoDeregister(PPDMDRVINS pDrvIns, const char *pszName, const char *pszDesc, PFNDBGFHANDLERDRV pfnHandler)
{
    return pDrvIns->pHlpR3->pfnDBGFInfoRegister(pDrvIns, pszName, pszDesc, pfnHandler);
}

/**
 * @copydoc PDMDRVHLPR3::pfnSTAMRegister
 */
DECLINLINE(void) PDMDrvHlpSTAMRegister(PPDMDRVINS pDrvIns, void *pvSample, STAMTYPE enmType, const char *pszName, STAMUNIT enmUnit, const char *pszDesc)
{
    pDrvIns->pHlpR3->pfnSTAMRegister(pDrvIns, pvSample, enmType, pszName, enmUnit, pszDesc);
}

/**
 * @copydoc PDMDRVHLPR3::pfnSTAMRegisterF
 */
DECLINLINE(void)  RT_IPRT_FORMAT_ATTR(7, 8) PDMDrvHlpSTAMRegisterF(PPDMDRVINS pDrvIns, void *pvSample, STAMTYPE enmType,
                                                                   STAMVISIBILITY enmVisibility, STAMUNIT enmUnit,
                                                                   const char *pszDesc, const char *pszName, ...)
{
    va_list va;
    va_start(va, pszName);
    pDrvIns->pHlpR3->pfnSTAMRegisterV(pDrvIns, pvSample, enmType, enmVisibility, enmUnit, pszDesc, pszName, va);
    va_end(va);
}

/**
 * Convenience wrapper that registers counter which is always visible.
 *
 * @param   pDrvIns             The driver instance.
 * @param   pCounter            Pointer to the counter variable.
 * @param   pszName             The name of the sample.  This is prefixed with
 *                              "/Drivers/<drivername>-<instance no>/".
 * @param   enmUnit             The unit.
 * @param   pszDesc             The description.
 */
DECLINLINE(void) PDMDrvHlpSTAMRegCounterEx(PPDMDRVINS pDrvIns, PSTAMCOUNTER pCounter, const char *pszName, STAMUNIT enmUnit, const char *pszDesc)
{
    pDrvIns->pHlpR3->pfnSTAMRegisterF(pDrvIns, pCounter, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, enmUnit, pszDesc,
                                      "/Drivers/%s-%u/%s", pDrvIns->pReg->szName, pDrvIns->iInstance, pszName);
}

/**
 * Convenience wrapper that registers counter which is always visible and has
 * the STAMUNIT_COUNT unit.
 *
 * @param   pDrvIns             The driver instance.
 * @param   pCounter            Pointer to the counter variable.
 * @param   pszName             The name of the sample.  This is prefixed with
 *                              "/Drivers/<drivername>-<instance no>/".
 * @param   pszDesc             The description.
 */
DECLINLINE(void) PDMDrvHlpSTAMRegCounter(PPDMDRVINS pDrvIns, PSTAMCOUNTER pCounter, const char *pszName, const char *pszDesc)
{
    PDMDrvHlpSTAMRegCounterEx(pDrvIns, pCounter, pszName, STAMUNIT_COUNT, pszDesc);
}

/**
 * Convenience wrapper that registers profiling sample which is always visible.
 *
 * @param   pDrvIns             The driver instance.
 * @param   pProfile            Pointer to the profiling variable.
 * @param   pszName             The name of the sample.  This is prefixed with
 *                              "/Drivers/<drivername>-<instance no>/".
 * @param   enmUnit             The unit.
 * @param   pszDesc             The description.
 */
DECLINLINE(void) PDMDrvHlpSTAMRegProfileEx(PPDMDRVINS pDrvIns, PSTAMPROFILE pProfile, const char *pszName, STAMUNIT enmUnit, const char *pszDesc)
{
    pDrvIns->pHlpR3->pfnSTAMRegisterF(pDrvIns, pProfile, STAMTYPE_PROFILE, STAMVISIBILITY_ALWAYS, enmUnit, pszDesc,
                                      "/Drivers/%s-%u/%s", pDrvIns->pReg->szName, pDrvIns->iInstance, pszName);
}

/**
 * Convenience wrapper that registers profiling sample which is always visible
 * hand counts ticks per call (STAMUNIT_TICKS_PER_CALL).
 *
 * @param   pDrvIns             The driver instance.
 * @param   pProfile            Pointer to the profiling variable.
 * @param   pszName             The name of the sample.  This is prefixed with
 *                              "/Drivers/<drivername>-<instance no>/".
 * @param   pszDesc             The description.
 */
DECLINLINE(void) PDMDrvHlpSTAMRegProfile(PPDMDRVINS pDrvIns, PSTAMPROFILE pProfile, const char *pszName, const char *pszDesc)
{
    PDMDrvHlpSTAMRegProfileEx(pDrvIns, pProfile, pszName, STAMUNIT_TICKS_PER_CALL, pszDesc);
}

/**
 * Convenience wrapper that registers an advanced profiling sample which is
 * always visible.
 *
 * @param   pDrvIns             The driver instance.
 * @param   pProfile            Pointer to the profiling variable.
 * @param   enmUnit             The unit.
 * @param   pszName             The name of the sample.  This is prefixed with
 *                              "/Drivers/<drivername>-<instance no>/".
 * @param   pszDesc             The description.
 */
DECLINLINE(void) PDMDrvHlpSTAMRegProfileAdvEx(PPDMDRVINS pDrvIns, PSTAMPROFILEADV pProfile, const char *pszName, STAMUNIT enmUnit, const char *pszDesc)
{
    pDrvIns->pHlpR3->pfnSTAMRegisterF(pDrvIns, pProfile, STAMTYPE_PROFILE, STAMVISIBILITY_ALWAYS, enmUnit, pszDesc,
                                      "/Drivers/%s-%u/%s", pDrvIns->pReg->szName, pDrvIns->iInstance, pszName);
}

/**
 * Convenience wrapper that registers an advanced profiling sample which is
 * always visible.
 *
 * @param   pDrvIns             The driver instance.
 * @param   pProfile            Pointer to the profiling variable.
 * @param   pszName             The name of the sample.  This is prefixed with
 *                              "/Drivers/<drivername>-<instance no>/".
 * @param   pszDesc             The description.
 */
DECLINLINE(void) PDMDrvHlpSTAMRegProfileAdv(PPDMDRVINS pDrvIns, PSTAMPROFILEADV pProfile, const char *pszName, const char *pszDesc)
{
    PDMDrvHlpSTAMRegProfileAdvEx(pDrvIns, pProfile, pszName, STAMUNIT_TICKS_PER_CALL, pszDesc);
}

/**
 * @copydoc PDMDRVHLPR3::pfnSTAMDeregister
 */
DECLINLINE(int) PDMDrvHlpSTAMDeregister(PPDMDRVINS pDrvIns, void *pvSample)
{
    return pDrvIns->pHlpR3->pfnSTAMDeregister(pDrvIns, pvSample);
}

/**
 * @copydoc PDMDRVHLPR3::pfnSTAMDeregisterByPrefix
 */
DECLINLINE(int) PDMDrvHlpSTAMDeregisterByPrefix(PPDMDRVINS pDrvIns, const char *pszPrefix)
{
    return pDrvIns->pHlpR3->pfnSTAMDeregisterByPrefix(pDrvIns, pszPrefix);
}

/**
 * @copydoc PDMDRVHLPR3::pfnSUPCallVMMR0Ex
 */
DECLINLINE(int) PDMDrvHlpSUPCallVMMR0Ex(PPDMDRVINS pDrvIns, unsigned uOperation, void *pvArg, unsigned cbArg)
{
    return pDrvIns->pHlpR3->pfnSUPCallVMMR0Ex(pDrvIns, uOperation, pvArg, cbArg);
}

/**
 * @copydoc PDMDRVHLPR3::pfnUSBRegisterHub
 */
DECLINLINE(int) PDMDrvHlpUSBRegisterHub(PPDMDRVINS pDrvIns, uint32_t fVersions, uint32_t cPorts, PCPDMUSBHUBREG pUsbHubReg, PPCPDMUSBHUBHLP ppUsbHubHlp)
{
    return pDrvIns->pHlpR3->pfnUSBRegisterHub(pDrvIns, fVersions, cPorts, pUsbHubReg, ppUsbHubHlp);
}

/**
 * @copydoc PDMDRVHLPR3::pfnSetAsyncNotification
 */
DECLINLINE(int) PDMDrvHlpSetAsyncNotification(PPDMDRVINS pDrvIns, PFNPDMDRVASYNCNOTIFY pfnAsyncNotify)
{
    return pDrvIns->pHlpR3->pfnSetAsyncNotification(pDrvIns, pfnAsyncNotify);
}

/**
 * @copydoc PDMDRVHLPR3::pfnAsyncNotificationCompleted
 */
DECLINLINE(void) PDMDrvHlpAsyncNotificationCompleted(PPDMDRVINS pDrvIns)
{
    pDrvIns->pHlpR3->pfnAsyncNotificationCompleted(pDrvIns);
}

/**
 * @copydoc PDMDRVHLPR3::pfnThreadCreate
 */
DECLINLINE(int) PDMDrvHlpThreadCreate(PPDMDRVINS pDrvIns, PPPDMTHREAD ppThread, void *pvUser, PFNPDMTHREADDRV pfnThread,
                                      PFNPDMTHREADWAKEUPDRV pfnWakeup, size_t cbStack, RTTHREADTYPE enmType, const char *pszName)
{
    return pDrvIns->pHlpR3->pfnThreadCreate(pDrvIns, ppThread, pvUser, pfnThread, pfnWakeup, cbStack, enmType, pszName);
}

/**
 * @copydoc PDMR3ThreadDestroy
 * @param   pDrvIns     The driver instance.
 */
DECLINLINE(int) PDMDrvHlpThreadDestroy(PPDMDRVINS pDrvIns, PPDMTHREAD pThread, int *pRcThread)
{
    return pDrvIns->pHlpR3->pfnThreadDestroy(pThread, pRcThread);
}

/**
 * @copydoc PDMR3ThreadIAmSuspending
 * @param   pDrvIns     The driver instance.
 */
DECLINLINE(int) PDMDrvHlpThreadIAmSuspending(PPDMDRVINS pDrvIns, PPDMTHREAD pThread)
{
    return pDrvIns->pHlpR3->pfnThreadIAmSuspending(pThread);
}

/**
 * @copydoc PDMR3ThreadIAmRunning
 * @param   pDrvIns     The driver instance.
 */
DECLINLINE(int) PDMDrvHlpThreadIAmRunning(PPDMDRVINS pDrvIns, PPDMTHREAD pThread)
{
    return pDrvIns->pHlpR3->pfnThreadIAmRunning(pThread);
}

/**
 * @copydoc PDMR3ThreadSleep
 * @param   pDrvIns     The driver instance.
 */
DECLINLINE(int) PDMDrvHlpThreadSleep(PPDMDRVINS pDrvIns, PPDMTHREAD pThread, RTMSINTERVAL cMillies)
{
    return pDrvIns->pHlpR3->pfnThreadSleep(pThread, cMillies);
}

/**
 * @copydoc PDMR3ThreadSuspend
 * @param   pDrvIns     The driver instance.
 */
DECLINLINE(int) PDMDrvHlpThreadSuspend(PPDMDRVINS pDrvIns, PPDMTHREAD pThread)
{
    return pDrvIns->pHlpR3->pfnThreadSuspend(pThread);
}

/**
 * @copydoc PDMR3ThreadResume
 * @param   pDrvIns     The driver instance.
 */
DECLINLINE(int) PDMDrvHlpThreadResume(PPDMDRVINS pDrvIns, PPDMTHREAD pThread)
{
    return pDrvIns->pHlpR3->pfnThreadResume(pThread);
}

# ifdef VBOX_WITH_PDM_ASYNC_COMPLETION
/**
 * @copydoc PDMDRVHLPR3::pfnAsyncCompletionTemplateCreate
 */
DECLINLINE(int) PDMDrvHlpAsyncCompletionTemplateCreate(PPDMDRVINS pDrvIns, PPPDMASYNCCOMPLETIONTEMPLATE ppTemplate,
                                                       PFNPDMASYNCCOMPLETEDRV pfnCompleted, void *pvTemplateUser, const char *pszDesc)
{
    return pDrvIns->pHlpR3->pfnAsyncCompletionTemplateCreate(pDrvIns, ppTemplate, pfnCompleted, pvTemplateUser, pszDesc);
}

/**
 * @copydoc PDMDRVHLPR3::pfnAsyncCompletionTemplateDestroy
 */
DECLINLINE(int) PDMDrvHlpAsyncCompletionTemplateDestroy(PPDMDRVINS pDrvIns, PPDMASYNCCOMPLETIONTEMPLATE pTemplate)
{
    return pDrvIns->pHlpR3->pfnAsyncCompletionTemplateDestroy(pTemplate);
}

/**
 * @copydoc PDMDRVHLPR3::pfnAsyncCompletionEpCreateForFile
 */
DECLINLINE(int) PDMDrvHlpAsyncCompletionEpCreateForFile(PPDMDRVINS pDrvIns, PPPDMASYNCCOMPLETIONENDPOINT ppEndpoint,
                                                        const char *pszFilename, uint32_t fFlags,
                                                        PPDMASYNCCOMPLETIONTEMPLATE pTemplate)
{
    return pDrvIns->pHlpR3->pfnAsyncCompletionEpCreateForFile(ppEndpoint, pszFilename, fFlags, pTemplate);
}

/**
 * @copydoc PDMDRVHLPR3::pfnAsyncCompletionEpClose
 */
DECLINLINE(void) PDMDrvHlpAsyncCompletionEpClose(PPDMDRVINS pDrvIns, PPDMASYNCCOMPLETIONENDPOINT pEndpoint)
{
    pDrvIns->pHlpR3->pfnAsyncCompletionEpClose(pEndpoint);
}

/**
 * @copydoc PDMDRVHLPR3::pfnAsyncCompletionEpGetSize
 */
DECLINLINE(int) PDMDrvHlpAsyncCompletionEpGetSize(PPDMDRVINS pDrvIns, PPDMASYNCCOMPLETIONENDPOINT pEndpoint, uint64_t *pcbSize)
{
    return pDrvIns->pHlpR3->pfnAsyncCompletionEpGetSize(pEndpoint, pcbSize);
}

/**
 * @copydoc PDMDRVHLPR3::pfnAsyncCompletionEpSetSize
 */
DECLINLINE(int) PDMDrvHlpAsyncCompletionEpSetSize(PPDMDRVINS pDrvIns, PPDMASYNCCOMPLETIONENDPOINT pEndpoint, uint64_t cbSize)
{
    return pDrvIns->pHlpR3->pfnAsyncCompletionEpSetSize(pEndpoint, cbSize);
}

/**
 * @copydoc PDMDRVHLPR3::pfnAsyncCompletionEpSetBwMgr
 */
DECLINLINE(int) PDMDrvHlpAsyncCompletionEpSetBwMgr(PPDMDRVINS pDrvIns, PPDMASYNCCOMPLETIONENDPOINT pEndpoint, const char *pszBwMgr)
{
    return pDrvIns->pHlpR3->pfnAsyncCompletionEpSetBwMgr(pEndpoint, pszBwMgr);
}

/**
 * @copydoc PDMDRVHLPR3::pfnAsyncCompletionEpFlush
 */
DECLINLINE(int) PDMDrvHlpAsyncCompletionEpFlush(PPDMDRVINS pDrvIns, PPDMASYNCCOMPLETIONENDPOINT pEndpoint, void *pvUser,
                                                PPPDMASYNCCOMPLETIONTASK ppTask)
{
    return pDrvIns->pHlpR3->pfnAsyncCompletionEpFlush(pEndpoint, pvUser, ppTask);
}

/**
 * @copydoc PDMDRVHLPR3::pfnAsyncCompletionEpRead
 */
DECLINLINE(int) PDMDrvHlpAsyncCompletionEpRead(PPDMDRVINS pDrvIns, PPDMASYNCCOMPLETIONENDPOINT pEndpoint, RTFOFF off,
                                               PCRTSGSEG paSegments, unsigned cSegments,
                                               size_t cbRead, void *pvUser,
                                               PPPDMASYNCCOMPLETIONTASK ppTask)
{
    return pDrvIns->pHlpR3->pfnAsyncCompletionEpRead(pEndpoint, off, paSegments, cSegments, cbRead, pvUser, ppTask);
}

/**
 * @copydoc PDMDRVHLPR3::pfnAsyncCompletionEpWrite
 */
DECLINLINE(int) PDMDrvHlpAsyncCompletionEpWrite(PPDMDRVINS pDrvIns, PPDMASYNCCOMPLETIONENDPOINT pEndpoint, RTFOFF off,
                                                PCRTSGSEG paSegments, unsigned cSegments,
                                                size_t cbWrite, void *pvUser,
                                                PPPDMASYNCCOMPLETIONTASK ppTask)
{
    return pDrvIns->pHlpR3->pfnAsyncCompletionEpWrite(pEndpoint, off, paSegments, cSegments, cbWrite, pvUser, ppTask);
}
# endif

#endif /* IN_RING3 */

#ifdef VBOX_WITH_NETSHAPER
# ifdef IN_RING3

/**
 * @copydoc PDMDRVHLPR3::pfnNetShaperAttach
 */
DECLINLINE(int) PDMDrvHlpNetShaperAttach(PPDMDRVINS pDrvIns, const char *pcszBwGroup, PPDMNSFILTER pFilter)
{
    return pDrvIns->pHlpR3->pfnNetShaperAttach(pDrvIns, pcszBwGroup, pFilter);
}

/**
 * @copydoc PDMDRVHLPR3::pfnNetShaperDetach
 */
DECLINLINE(int) PDMDrvHlpNetShaperDetach(PPDMDRVINS pDrvIns, PPDMNSFILTER pFilter)
{
    return pDrvIns->pHlpR3->pfnNetShaperDetach(pDrvIns, pFilter);
}

# endif /* IN_RING3 */

/**
 * @copydoc PDMDRVHLPR3::pfnNetShaperAllocateBandwidth
 */
DECLINLINE(bool) PDMDrvHlpNetShaperAllocateBandwidth(PPDMDRVINS pDrvIns, PPDMNSFILTER pFilter, size_t cbTransfer)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnNetShaperAllocateBandwidth(pDrvIns, pFilter, cbTransfer);
}

#endif /* VBOX_WITH_NETSHAPER*/

#ifdef IN_RING3
/**
 * @copydoc PDMDRVHLPR3::pfnCritSectInit
 */
DECLINLINE(int) PDMDrvHlpCritSectInit(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect, RT_SRC_POS_DECL, const char *pszName)
{
    return pDrvIns->pHlpR3->pfnCritSectInit(pDrvIns, pCritSect, RT_SRC_POS_ARGS, pszName);
}
#endif /* IN_RING3 */

/**
 * @see PDMCritSectEnter
 */
DECLINLINE(int) PDMDrvHlpCritSectEnter(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect, int rcBusy)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnCritSectEnter(pDrvIns, pCritSect, rcBusy);
}

/**
 * @see PDMCritSectEnterDebug
 */
DECLINLINE(int) PDMDrvHlpCritSectEnterDebug(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect, int rcBusy, RTHCUINTPTR uId, RT_SRC_POS_DECL)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnCritSectEnterDebug(pDrvIns, pCritSect, rcBusy, uId, RT_SRC_POS_ARGS);
}

/**
 * @see PDMCritSectTryEnter
 */
DECLINLINE(int)      PDMDrvHlpCritSectTryEnter(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnCritSectTryEnter(pDrvIns, pCritSect);
}

/**
 * @see PDMCritSectTryEnterDebug
 */
DECLINLINE(int)      PDMDrvHlpCritSectTryEnterDebug(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect, RTHCUINTPTR uId, RT_SRC_POS_DECL)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnCritSectTryEnterDebug(pDrvIns, pCritSect, uId, RT_SRC_POS_ARGS);
}

/**
 * @see PDMCritSectLeave
 */
DECLINLINE(int)      PDMDrvHlpCritSectLeave(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnCritSectLeave(pDrvIns, pCritSect);
}

/**
 * @see PDMCritSectIsOwner
 */
DECLINLINE(bool)     PDMDrvHlpCritSectIsOwner(PPDMDRVINS pDrvIns, PCPDMCRITSECT pCritSect)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnCritSectIsOwner(pDrvIns, pCritSect);
}

/**
 * @see PDMCritSectIsInitialized
 */
DECLINLINE(bool)     PDMDrvHlpCritSectIsInitialized(PPDMDRVINS pDrvIns, PCPDMCRITSECT pCritSect)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnCritSectIsInitialized(pDrvIns, pCritSect);
}

/**
 * @see PDMCritSectHasWaiters
 */
DECLINLINE(bool)     PDMDrvHlpCritSectHasWaiters(PPDMDRVINS pDrvIns, PCPDMCRITSECT pCritSect)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnCritSectHasWaiters(pDrvIns, pCritSect);
}

/**
 * @see PDMCritSectGetRecursion
 */
DECLINLINE(uint32_t) PDMDrvHlpCritSectGetRecursion(PPDMDRVINS pDrvIns, PCPDMCRITSECT pCritSect)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnCritSectGetRecursion(pDrvIns, pCritSect);
}

#if defined(IN_RING3) || defined(IN_RING0)
/**
 * @see PDMHCCritSectScheduleExitEvent
 */
DECLINLINE(int) PDMDrvHlpCritSectScheduleExitEvent(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect, SUPSEMEVENT hEventToSignal)
{
    return pDrvIns->CTX_SUFF(pHlp)->pfnCritSectScheduleExitEvent(pDrvIns, pCritSect, hEventToSignal);
}
#endif

/* Strict build: Remap the two enter calls to the debug versions. */
#ifdef VBOX_STRICT
# ifdef IPRT_INCLUDED_asm_h
#  define PDMDrvHlpCritSectEnter(pDrvIns, pCritSect, rcBusy) PDMDrvHlpCritSectEnterDebug((pDrvIns), (pCritSect), (rcBusy), (uintptr_t)ASMReturnAddress(), RT_SRC_POS)
#  define PDMDrvHlpCritSectTryEnter(pDrvIns, pCritSect)      PDMDrvHlpCritSectTryEnterDebug((pDrvIns), (pCritSect), (uintptr_t)ASMReturnAddress(), RT_SRC_POS)
# else
#  define PDMDrvHlpCritSectEnter(pDrvIns, pCritSect, rcBusy) PDMDrvHlpCritSectEnterDebug((pDrvIns), (pCritSect), (rcBusy), 0, RT_SRC_POS)
#  define PDMDrvHlpCritSectTryEnter(pDrvIns, pCritSect)      PDMDrvHlpCritSectTryEnterDebug((pDrvIns), (pCritSect), 0, RT_SRC_POS)
# endif
#endif

#if defined(IN_RING3) || defined(DOXYGEN_RUNNING)

/**
 * @see PDMR3CritSectDelete
 */
DECLINLINE(int) PDMDrvHlpCritSectDelete(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect)
{
    return pDrvIns->pHlpR3->pfnCritSectDelete(pDrvIns, pCritSect);
}

/**
 * @copydoc PDMDRVHLPR3::pfnCallR0
 */
DECLINLINE(int) PDMDrvHlpCallR0(PPDMDRVINS pDrvIns, uint32_t uOperation, uint64_t u64Arg)
{
    return pDrvIns->pHlpR3->pfnCallR0(pDrvIns, uOperation, u64Arg);
}

/**
 * @copydoc PDMDRVHLPR3::pfnBlkCacheRetain
 */
DECLINLINE(int) PDMDrvHlpBlkCacheRetain(PPDMDRVINS pDrvIns, PPPDMBLKCACHE ppBlkCache,
                                        PFNPDMBLKCACHEXFERCOMPLETEDRV pfnXferComplete,
                                        PFNPDMBLKCACHEXFERENQUEUEDRV pfnXferEnqueue,
                                        PFNPDMBLKCACHEXFERENQUEUEDISCARDDRV pfnXferEnqueueDiscard,
                                        const char *pcszId)
{
    return pDrvIns->pHlpR3->pfnBlkCacheRetain(pDrvIns, ppBlkCache, pfnXferComplete, pfnXferEnqueue, pfnXferEnqueueDiscard, pcszId);
}

/**
 * @copydoc PDMDRVHLPR3::pfnBlkCacheRelease
 */
DECLINLINE(void) PDMDrvHlpBlkCacheRelease(PPDMDRVINS pDrvIns, PPDMBLKCACHE pBlkCache)
{
    pDrvIns->pHlpR3->pfnBlkCacheRelease(pBlkCache);
}

/**
 * @copydoc PDMDRVHLPR3::pfnBlkCacheClear
 */
DECLINLINE(int) PDMDrvHlpBlkCacheClear(PPDMDRVINS pDrvIns, PPDMBLKCACHE pBlkCache)
{
    return pDrvIns->pHlpR3->pfnBlkCacheClear(pBlkCache);
}

/**
 * @copydoc PDMDRVHLPR3::pfnBlkCacheSuspend
 */
DECLINLINE(int) PDMDrvHlpBlkCacheSuspend(PPDMDRVINS pDrvIns, PPDMBLKCACHE pBlkCache)
{
    return pDrvIns->pHlpR3->pfnBlkCacheSuspend(pBlkCache);
}

/**
 * @copydoc PDMDRVHLPR3::pfnBlkCacheResume
 */
DECLINLINE(int) PDMDrvHlpBlkCacheResume(PPDMDRVINS pDrvIns, PPDMBLKCACHE pBlkCache)
{
    return pDrvIns->pHlpR3->pfnBlkCacheResume(pBlkCache);
}

/**
 * @copydoc PDMDRVHLPR3::pfnBlkCacheIoXferComplete
 */
DECLINLINE(void) PDMDrvHlpBlkCacheIoXferComplete(PPDMDRVINS pDrvIns, PPDMBLKCACHE pBlkCache,
                                                 PPDMBLKCACHEIOXFER hIoXfer, int rcIoXfer)
{
    pDrvIns->pHlpR3->pfnBlkCacheIoXferComplete(pBlkCache, hIoXfer, rcIoXfer);
}

/**
 * @copydoc PDMDRVHLPR3::pfnBlkCacheRead
 */
DECLINLINE(int) PDMDrvHlpBlkCacheRead(PPDMDRVINS pDrvIns, PPDMBLKCACHE pBlkCache, uint64_t off,
                                      PCRTSGBUF pSgBuf, size_t cbRead, void *pvUser)
{
    return pDrvIns->pHlpR3->pfnBlkCacheRead(pBlkCache, off, pSgBuf, cbRead, pvUser);
}

/**
 * @copydoc PDMDRVHLPR3::pfnBlkCacheWrite
 */
DECLINLINE(int) PDMDrvHlpBlkCacheWrite(PPDMDRVINS pDrvIns, PPDMBLKCACHE pBlkCache, uint64_t off,
                                      PCRTSGBUF pSgBuf, size_t cbRead, void *pvUser)
{
    return pDrvIns->pHlpR3->pfnBlkCacheWrite(pBlkCache, off, pSgBuf, cbRead, pvUser);
}

/**
 * @copydoc PDMDRVHLPR3::pfnBlkCacheFlush
 */
DECLINLINE(int) PDMDrvHlpBlkCacheFlush(PPDMDRVINS pDrvIns, PPDMBLKCACHE pBlkCache, void *pvUser)
{
    return pDrvIns->pHlpR3->pfnBlkCacheFlush(pBlkCache, pvUser);
}

/**
 * @copydoc PDMDRVHLPR3::pfnBlkCacheDiscard
 */
DECLINLINE(int) PDMDrvHlpBlkCacheDiscard(PPDMDRVINS pDrvIns, PPDMBLKCACHE pBlkCache, PCRTRANGE paRanges,
                                         unsigned cRanges, void *pvUser)
{
    return pDrvIns->pHlpR3->pfnBlkCacheDiscard(pBlkCache, paRanges, cRanges, pvUser);
}

/**
 * @copydoc PDMDRVHLPR3::pfnVMGetSuspendReason
 */
DECLINLINE(VMSUSPENDREASON) PDMDrvHlpVMGetSuspendReason(PPDMDRVINS pDrvIns)
{
    return pDrvIns->pHlpR3->pfnVMGetSuspendReason(pDrvIns);
}

/**
 * @copydoc PDMDRVHLPR3::pfnVMGetResumeReason
 */
DECLINLINE(VMRESUMEREASON) PDMDrvHlpVMGetResumeReason(PPDMDRVINS pDrvIns)
{
    return pDrvIns->pHlpR3->pfnVMGetResumeReason(pDrvIns);
}

/**
 * @copydoc PDMDRVHLPR3::pfnQueryGenericUserObject
 */
DECLINLINE(void *) PDMDrvHlpQueryGenericUserObject(PPDMDRVINS pDrvIns, PCRTUUID pUuid)
{
    return pDrvIns->pHlpR3->pfnQueryGenericUserObject(pDrvIns, pUuid);
}


/** Pointer to callbacks provided to the VBoxDriverRegister() call. */
typedef struct PDMDRVREGCB *PPDMDRVREGCB;
/** Pointer to const callbacks provided to the VBoxDriverRegister() call. */
typedef const struct PDMDRVREGCB *PCPDMDRVREGCB;

/**
 * Callbacks for VBoxDriverRegister().
 */
typedef struct PDMDRVREGCB
{
    /** Interface version.
     * This is set to PDM_DRVREG_CB_VERSION. */
    uint32_t                    u32Version;

    /**
     * Registers a driver with the current VM instance.
     *
     * @returns VBox status code.
     * @param   pCallbacks      Pointer to the callback table.
     * @param   pReg            Pointer to the driver registration record.
     *                          This data must be permanent and readonly.
     */
    DECLR3CALLBACKMEMBER(int, pfnRegister,(PCPDMDRVREGCB pCallbacks, PCPDMDRVREG pReg));
} PDMDRVREGCB;

/** Current version of the PDMDRVREGCB structure.  */
#define PDM_DRVREG_CB_VERSION                   PDM_VERSION_MAKE(0xf0fa, 1, 0)


/**
 * The VBoxDriverRegister callback function.
 *
 * PDM will invoke this function after loading a driver module and letting
 * the module decide which drivers to register and how to handle conflicts.
 *
 * @returns VBox status code.
 * @param   pCallbacks      Pointer to the callback table.
 * @param   u32Version      VBox version number.
 */
typedef DECLCALLBACKTYPE(int, FNPDMVBOXDRIVERSREGISTER,(PCPDMDRVREGCB pCallbacks, uint32_t u32Version));

VMMR3DECL(int) PDMR3DrvStaticRegistration(PVM pVM, FNPDMVBOXDRIVERSREGISTER pfnCallback);

#endif /* IN_RING3 */

/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_pdmdrv_h */
