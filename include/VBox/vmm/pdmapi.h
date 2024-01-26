/** @file
 * PDM - Pluggable Device Manager, Core API.
 *
 * The 'Core API' has been put in a different header because everyone
 * is currently including pdm.h. So, pdm.h is for including all of the
 * PDM stuff, while pdmapi.h is for the core stuff.
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

#ifndef VBOX_INCLUDED_vmm_pdmapi_h
#define VBOX_INCLUDED_vmm_pdmapi_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/vmm/pdmcommon.h>
#ifdef IN_RING3
# include <VBox/vmm/vmapi.h>
#endif
#include <VBox/sup.h>

struct PDMDEVMODREGR0;

RT_C_DECLS_BEGIN

/** @defgroup grp_pdm       The Pluggable Device Manager API
 * @ingroup grp_vmm
 * @{
 */

VMMDECL(int)            PDMGetInterrupt(PVMCPUCC pVCpu, uint8_t *pu8Interrupt);
VMMDECL(int)            PDMIsaSetIrq(PVMCC pVM, uint8_t u8Irq, uint8_t u8Level, uint32_t uTagSrc);
VMM_INT_DECL(bool)      PDMHasIoApic(PVM pVM);
VMM_INT_DECL(bool)      PDMHasApic(PVM pVM);
VMM_INT_DECL(PPDMDEVINS) PDMDeviceRing0IdxToInstance(PVMCC pVM, uint64_t idxR0Device);
VMM_INT_DECL(int)       PDMIoApicSetIrq(PVM pVM, PCIBDF uBusDevFn, uint8_t u8Irq, uint8_t u8Level, uint32_t uTagSrc);
VMM_INT_DECL(void)      PDMIoApicBroadcastEoi(PVMCC pVM, uint8_t uVector);
VMM_INT_DECL(void)      PDMIoApicSendMsi(PVMCC pVM, PCIBDF uBusDevFn, PCMSIMSG pMsi, uint32_t uTagSrc);
VMM_INT_DECL(int)       PDMVmmDevHeapR3ToGCPhys(PVM pVM, RTR3PTR pv, RTGCPHYS *pGCPhys);
VMM_INT_DECL(bool)      PDMVmmDevHeapIsEnabled(PVM pVM);

/**
 * Mapping/unmapping callback for an VMMDev heap allocation.
 *
 * @param   pVM                 The cross context VM structure.
 * @param   pvAllocation        The allocation address (ring-3).
 * @param   GCPhysAllocation    The guest physical address of the mapping if
 *                              it's being mapped, NIL_RTGCPHYS if it's being
 *                              unmapped.
 */
typedef DECLCALLBACKTYPE(void, FNPDMVMMDEVHEAPNOTIFY,(PVM pVM, void *pvAllocation, RTGCPHYS GCPhysAllocation));
/** Pointer (ring-3) to a FNPDMVMMDEVHEAPNOTIFY function. */
typedef R3PTRTYPE(FNPDMVMMDEVHEAPNOTIFY *) PFNPDMVMMDEVHEAPNOTIFY;


#if defined(IN_RING3) || defined(DOXYGEN_RUNNING)
/** @defgroup grp_pdm_r3    The PDM Host Context Ring-3 API
 * @{
 */
VMMR3_INT_DECL(int)     PDMR3InitUVM(PUVM pUVM);
VMMR3_INT_DECL(int)     PDMR3LdrLoadVMMR0U(PUVM pUVM);
VMMR3_INT_DECL(int)     PDMR3Init(PVM pVM);
VMMR3_INT_DECL(int)     PDMR3InitCompleted(PVM pVM, VMINITCOMPLETED enmWhat);
VMMR3DECL(void)         PDMR3PowerOn(PVM pVM);
VMMR3_INT_DECL(bool)    PDMR3GetResetInfo(PVM pVM, uint32_t fOverride, uint32_t *pfResetFlags);
VMMR3_INT_DECL(void)    PDMR3ResetCpu(PVMCPU pVCpu);
VMMR3_INT_DECL(void)    PDMR3Reset(PVM pVM);
VMMR3_INT_DECL(void)    PDMR3MemSetup(PVM pVM, bool fAtReset);
VMMR3_INT_DECL(void)    PDMR3SoftReset(PVM pVM, uint32_t fResetFlags);
VMMR3_INT_DECL(void)    PDMR3Suspend(PVM pVM);
VMMR3_INT_DECL(void)    PDMR3Resume(PVM pVM);
VMMR3DECL(void)         PDMR3PowerOff(PVM pVM);
VMMR3_INT_DECL(void)    PDMR3Relocate(PVM pVM, RTGCINTPTR offDelta);
VMMR3_INT_DECL(int)     PDMR3Term(PVM pVM);
VMMR3_INT_DECL(void)    PDMR3TermUVM(PUVM pUVM);
VMMR3_INT_DECL(bool)    PDMR3HasLoadedState(PVM pVM);

/** PDM loader context indicator.  */
typedef enum  PDMLDRCTX
{
    /** Invalid zero value. */
    PDMLDRCTX_INVALID = 0,
    /** Ring-0 context. */
    PDMLDRCTX_RING_0,
    /** Ring-3 context. */
    PDMLDRCTX_RING_3,
    /** Raw-mode context. */
    PDMLDRCTX_RAW_MODE,
    /** End of valid context values. */
    PDMLDRCTX_END,
    /** 32-bit type hack. */
    PDMLDRCTX_32BIT_HACK = 0x7fffffff
} PDMLDRCTX;

/**
 * Module enumeration callback function.
 *
 * @returns VBox status.
 *          Failure will stop the search and return the return code.
 *          Warnings will be ignored and not returned.
 * @param   pVM             The cross context VM structure.
 * @param   pszFilename     Module filename.
 * @param   pszName         Module name. (short and unique)
 * @param   ImageBase       Address where to executable image is loaded.
 * @param   cbImage         Size of the executable image.
 * @param   enmCtx          The context the module is loaded into.
 * @param   pvArg           User argument.
 */
typedef DECLCALLBACKTYPE(int, FNPDMR3ENUM,(PVM pVM, const char *pszFilename, const char *pszName,
                                           RTUINTPTR ImageBase, size_t cbImage, PDMLDRCTX enmCtx, void *pvArg));
/** Pointer to a FNPDMR3ENUM() function. */
typedef FNPDMR3ENUM *PFNPDMR3ENUM;
VMMR3DECL(int)          PDMR3LdrEnumModules(PVM pVM, PFNPDMR3ENUM pfnCallback, void *pvArg);
VMMR3_INT_DECL(void)    PDMR3LdrRelocateU(PUVM pUVM, RTGCINTPTR offDelta);
VMMR3_INT_DECL(int)     PDMR3LdrLoadR0(PUVM pUVM, const char *pszModule, const char *pszSearchPath);
VMMR3_INT_DECL(int)     PDMR3LdrGetSymbolR3(PVM pVM, const char *pszModule, const char *pszSymbol, void **ppvValue);
VMMR3DECL(int)          PDMR3LdrGetSymbolR0(PVM pVM, const char *pszModule, const char *pszSymbol, PRTR0PTR ppvValue);
VMMR3DECL(int)          PDMR3LdrGetSymbolR0Lazy(PVM pVM, const char *pszModule, const char *pszSearchPath, const char *pszSymbol, PRTR0PTR ppvValue);
VMMR3DECL(int)          PDMR3LdrLoadRC(PVM pVM, const char *pszFilename, const char *pszName);
VMMR3DECL(int)          PDMR3LdrGetSymbolRC(PVM pVM, const char *pszModule, const char *pszSymbol, PRTRCPTR pRCPtrValue);
VMMR3DECL(int)          PDMR3LdrGetSymbolRCLazy(PVM pVM, const char *pszModule, const char *pszSearchPath, const char *pszSymbol,
                                                PRTRCPTR pRCPtrValue);
VMMR3_INT_DECL(int)     PDMR3LdrQueryRCModFromPC(PVM pVM, RTRCPTR uPC,
                                                 char *pszModName,  size_t cchModName,  PRTRCPTR pMod,
                                                 char *pszNearSym1, size_t cchNearSym1, PRTRCPTR pNearSym1,
                                                 char *pszNearSym2, size_t cchNearSym2, PRTRCPTR pNearSym2);
VMMR3_INT_DECL(int)     PDMR3LdrQueryR0ModFromPC(PVM pVM, RTR0PTR uPC,
                                                 char *pszModName,  size_t cchModName,  PRTR0PTR pMod,
                                                 char *pszNearSym1, size_t cchNearSym1, PRTR0PTR pNearSym1,
                                                 char *pszNearSym2, size_t cchNearSym2, PRTR0PTR pNearSym2);
VMMR3_INT_DECL(int)     PDMR3LdrGetInterfaceSymbols(PVM pVM, void *pvInterface, size_t cbInterface,
                                                    const char *pszModule, const char *pszSearchPath,
                                                    const char *pszSymPrefix, const char *pszSymList,
                                                    bool fRing0OrRC);

VMMR3DECL(int)          PDMR3QueryDevice(PUVM pUVM, const char *pszDevice, unsigned iInstance, PPPDMIBASE ppBase);
VMMR3DECL(int)          PDMR3QueryDeviceLun(PUVM pUVM, const char *pszDevice, unsigned iInstance, unsigned iLun, PPPDMIBASE ppBase);
VMMR3DECL(int)          PDMR3QueryLun(PUVM pUVM, const char *pszDevice, unsigned iInstance, unsigned iLun, PPPDMIBASE ppBase);
VMMR3DECL(int)          PDMR3QueryDriverOnLun(PUVM pUVM, const char *pszDevice, unsigned iInstance, unsigned iLun,
                                              const char *pszDriver, PPPDMIBASE ppBase);
VMMR3DECL(int)          PDMR3DeviceAttach(PUVM pUVM, const char *pszDevice, unsigned iInstance, unsigned iLun, uint32_t fFlags,
                                          PPDMIBASE *ppBase);
VMMR3DECL(int)          PDMR3DeviceDetach(PUVM pUVM, const char *pszDevice, unsigned iInstance, unsigned iLun, uint32_t fFlags);
VMMR3_INT_DECL(PPDMCRITSECT) PDMR3DevGetCritSect(PVM pVM, PPDMDEVINS pDevIns);
VMMR3DECL(int)          PDMR3DriverAttach(PUVM pUVM, const char *pszDevice, unsigned iDevIns, unsigned iLun, uint32_t fFlags,
                                          PPPDMIBASE ppBase);
VMMR3DECL(int)          PDMR3DriverDetach(PUVM pUVM, const char *pszDevice, unsigned iDevIns, unsigned iLun,
                                          const char *pszDriver, unsigned iOccurrence, uint32_t fFlags);
VMMR3DECL(int)          PDMR3DriverReattach(PUVM pVM, const char *pszDevice, unsigned iDevIns, unsigned iLun,
                                            const char *pszDriver, unsigned iOccurrence, uint32_t fFlags, PCFGMNODE pCfg,
                                            PPPDMIBASE ppBase);
VMMR3DECL(void)         PDMR3DmaRun(PVM pVM);

VMMR3_INT_DECL(int)     PDMR3VmmDevHeapAlloc(PVM pVM, size_t cbSize, PFNPDMVMMDEVHEAPNOTIFY pfnNotify, RTR3PTR *ppv);
VMMR3_INT_DECL(int)     PDMR3VmmDevHeapFree(PVM pVM, RTR3PTR pv);
VMMR3_INT_DECL(int)     PDMR3TracingConfig(PVM pVM, const char *pszName, size_t cchName, bool fEnable, bool fApply);
VMMR3_INT_DECL(bool)    PDMR3TracingAreAll(PVM pVM, bool fEnabled);
VMMR3_INT_DECL(int)     PDMR3TracingQueryConfig(PVM pVM, char *pszConfig, size_t cbConfig);
/** @} */
#endif /* IN_RING3 */



/** @defgroup grp_pdm_rc    The PDM Raw-Mode Context API
 * @{
 */
/** @} */



/** @defgroup grp_pdm_r0    The PDM Ring-0 Context API
 * @{
 */
VMMR0_INT_DECL(void)    PDMR0Init(void *hMod);
VMMR0DECL(int)          PDMR0DeviceRegisterModule(void *hMod, struct PDMDEVMODREGR0 *pModReg);
VMMR0DECL(int)          PDMR0DeviceDeregisterModule(void *hMod, struct PDMDEVMODREGR0 *pModReg);

VMMR0_INT_DECL(void)    PDMR0InitPerVMData(PGVM pGVM);
VMMR0_INT_DECL(void)    PDMR0CleanupVM(PGVM pGVM);

/**
 * Request buffer for PDMR0DriverCallReqHandler / VMMR0_DO_PDM_DRIVER_CALL_REQ_HANDLER.
 * @see PDMR0DriverCallReqHandler.
 */
typedef struct PDMDRIVERCALLREQHANDLERREQ
{
    /** The header. */
    SUPVMMR0REQHDR      Hdr;
    /** The driver instance. */
    PPDMDRVINSR0        pDrvInsR0;
    /** The operation. */
    uint32_t            uOperation;
    /** Explicit alignment padding. */
    uint32_t            u32Alignment;
    /** Optional 64-bit integer argument. */
    uint64_t            u64Arg;
} PDMDRIVERCALLREQHANDLERREQ;
/** Pointer to a PDMR0DriverCallReqHandler / VMMR0_DO_PDM_DRIVER_CALL_REQ_HANDLER
 * request buffer. */
typedef PDMDRIVERCALLREQHANDLERREQ *PPDMDRIVERCALLREQHANDLERREQ;

VMMR0_INT_DECL(int) PDMR0DriverCallReqHandler(PGVM pGVM, PPDMDRIVERCALLREQHANDLERREQ pReq);


/**
 * Request buffer for PDMR0DeviceCreateReqHandler / VMMR0_DO_PDM_DEVICE_CREATE.
 * @see PDMR0DeviceCreateReqHandler.
 */
typedef struct PDMDEVICECREATEREQ
{
    /** The header. */
    SUPVMMR0REQHDR          Hdr;
    /** Out: Where to return the address of the ring-3 device instance. */
    PPDMDEVINSR3            pDevInsR3;

    /** Copy of PDMDEVREGR3::fFlags for matching with PDMDEVREGR0::fFlags. */
    uint32_t                fFlags;
    /** Copy of PDMDEVREGR3::fClass for matching with PDMDEVREGR0::fFlags. */
    uint32_t                fClass;
    /** Copy of PDMDEVREGR3::cMaxInstances for matching with
     *  PDMDEVREGR0::cMaxInstances. */
    uint32_t                cMaxInstances;
    /** Copy of PDMDEVREGR3::uSharedVersion for matching with
     *  PDMDEVREGR0::uSharedVersion. */
    uint32_t                uSharedVersion;
    /** Copy of PDMDEVREGR3::cbInstanceShared for matching with
     *  PDMDEVREGR0::cbInstanceShared. */
    uint32_t                cbInstanceShared;
    /** Copy of PDMDEVREGR3::cbInstanceCC. */
    uint32_t                cbInstanceR3;
    /** Copy of PDMDEVREGR3::cbInstanceRC for matching with
     *  PDMDEVREGR0::cbInstanceRC. */
    uint32_t                cbInstanceRC;
    /** Copy of PDMDEVREGR3::cMaxPciDevices for matching with
     *  PDMDEVREGR0::cMaxPciDevices. */
    uint16_t                cMaxPciDevices;
    /** Copy of PDMDEVREGR3::cMaxMsixVectors for matching with
     *  PDMDEVREGR0::cMaxMsixVectors. */
    uint16_t                cMaxMsixVectors;

    /** The device instance ordinal. */
    uint32_t                iInstance;
    /** Set if the raw-mode component is desired. */
    bool                    fRCEnabled;
    /** Explicit padding. */
    bool                    afReserved[3];
    /** DBGF tracer event source handle if configured. */
    DBGFTRACEREVTSRC        hDbgfTracerEvtSrc;

    /** In: Device name. */
    char                    szDevName[32];
    /** In: The module name (no path). */
    char                    szModName[32];
} PDMDEVICECREATEREQ;
/** Pointer to a PDMR0DeviceCreate / VMMR0_DO_PDM_DEVICE_CREATE request buffer. */
typedef PDMDEVICECREATEREQ *PPDMDEVICECREATEREQ;

VMMR0_INT_DECL(int) PDMR0DeviceCreateReqHandler(PGVM pGVM, PPDMDEVICECREATEREQ pReq);

/**
 * The ring-0 device call to make.
 */
typedef enum PDMDEVICEGENCALL
{
    PDMDEVICEGENCALL_INVALID = 0,
    PDMDEVICEGENCALL_CONSTRUCT,
    PDMDEVICEGENCALL_DESTRUCT,
    PDMDEVICEGENCALL_REQUEST,
    PDMDEVICEGENCALL_END,
    PDMDEVICEGENCALL_32BIT_HACK = 0x7fffffff
} PDMDEVICEGENCALL;

/**
 * Request buffer for PDMR0DeviceGenCallReqHandler / VMMR0_DO_PDM_DEVICE_GEN_CALL.
 * @see PDMR0DeviceGenCallReqHandler.
 */
typedef struct PDMDEVICEGENCALLREQ
{
    /** The header. */
    SUPVMMR0REQHDR          Hdr;
    /** The ring-3 device instance. */
    PPDMDEVINSR3            pDevInsR3;
    /** The ring-0 device handle. */
    uint32_t                idxR0Device;
    /** The call to make. */
    PDMDEVICEGENCALL        enmCall;
    union
    {
        /** PDMDEVICEGENCALL_REQUEST: */
        struct
        {
            /** The request argument. */
            uint64_t        uArg;
            /** The request number.    */
            uint32_t        uReq;
        } Req;
        /** Size padding. */
        uint64_t            au64[3];
    } Params;
} PDMDEVICEGENCALLREQ;
/** Pointer to a PDMR0DeviceGenCallReqHandler / VMMR0_DO_PDM_DEVICE_GEN_CALL request buffer. */
typedef PDMDEVICEGENCALLREQ *PPDMDEVICEGENCALLREQ;

VMMR0_INT_DECL(int) PDMR0DeviceGenCallReqHandler(PGVM pGVM, PPDMDEVICEGENCALLREQ pReq, VMCPUID idCpu);

/**
 * Request buffer for PDMR0DeviceCompatSetCritSectReqHandler / VMMR0_DO_PDM_DEVICE_COMPAT_SET_CRITSECT
 * @see PDMR0DeviceCompatSetCritSectReqHandler.
 */
typedef struct PDMDEVICECOMPATSETCRITSECTREQ
{
    /** The header. */
    SUPVMMR0REQHDR          Hdr;
    /** The ring-3 device instance. */
    PPDMDEVINSR3            pDevInsR3;
    /** The ring-0 device handle. */
    uint32_t                idxR0Device;
    /** The critical section address (ring-3). */
    R3PTRTYPE(PPDMCRITSECT) pCritSectR3;
} PDMDEVICECOMPATSETCRITSECTREQ;
/** Pointer to a PDMR0DeviceGenCallReqHandler / VMMR0_DO_PDM_DEVICE_GEN_CALL request buffer. */
typedef PDMDEVICECOMPATSETCRITSECTREQ *PPDMDEVICECOMPATSETCRITSECTREQ;

VMMR0_INT_DECL(int) PDMR0DeviceCompatSetCritSectReqHandler(PGVM pGVM, PPDMDEVICECOMPATSETCRITSECTREQ pReq);


/**
 * Request buffer for PDMR0QueueCreateReqHandler / VMMR0_DO_PDM_QUEUE_CREATE.
 * @see PDMR0QueueCreateReqHandler.
 */
typedef struct PDMQUEUECREATEREQ
{
    /** The header. */
    SUPVMMR0REQHDR          Hdr;

    /** Number of queue items. */
    uint32_t                cItems;
    /** Queue item size.   */
    uint32_t                cbItem;
    /** Owner type (PDMQUEUETYPE). */
    uint32_t                enmType;
    /** The ring-3 owner pointer. */
    RTR3PTR                 pvOwner;
    /** The ring-3 callback function address. */
    RTR3PTR                 pfnCallback;
    /** The queue name. */
    char                    szName[40];

    /** Output: The queue handle. */
    PDMQUEUEHANDLE          hQueue;
} PDMQUEUECREATEREQ;
/** Pointer to a PDMR0QueueCreateReqHandler / VMMR0_DO_PDM_QUEUE_CREATE request buffer. */
typedef PDMQUEUECREATEREQ *PPDMQUEUECREATEREQ;

VMMR0_INT_DECL(int) PDMR0QueueCreateReqHandler(PGVM pGVM, PPDMQUEUECREATEREQ pReq);

/** @} */

RT_C_DECLS_END

/** @} */

#endif /* !VBOX_INCLUDED_vmm_pdmapi_h */
