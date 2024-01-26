/** @file
 * tstDevice: Shared definitions between the framework and the shim library.
 */

/*
 * Copyright (C) 2017-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_SRC_testcase_tstDeviceInternal_h
#define VBOX_INCLUDED_SRC_testcase_tstDeviceInternal_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/param.h>
#include <VBox/types.h>
#include <iprt/assert.h>
#include <iprt/list.h>
#include <iprt/semaphore.h>
#include <iprt/critsect.h>

#include "tstDeviceCfg.h"
#include "tstDevicePlugin.h"

RT_C_DECLS_BEGIN

#define PDM_MAX_DEVICE_INSTANCE_SIZE      _4M

/** Converts PDM device instance to the device under test structure. */
#define TSTDEV_PDMDEVINS_2_DUT(a_pDevIns) ((a_pDevIns)->Internal.s.pDut)

/** Forward declaration of internal test device instance data. */
typedef struct TSTDEVDUTINT *PTSTDEVDUTINT;


/** Pointer to a const PDM module descriptor. */
typedef const struct TSTDEVPDMMOD *PCTSTDEVPDMMOD;


/**
 * PDM device descriptor.
 */
typedef struct TSTDEVPDMDEV
{
    /** Node for the known device list. */
    RTLISTNODE                      NdPdmDevs;
    /** Pointer to the PDM module containing the device. */
    PCTSTDEVPDMMOD                  pPdmMod;
    /** Device registration structure. */
    const struct PDMDEVREGR3        *pReg;
} TSTDEVPDMDEV;
/** Pointer to a PDM device descriptor .*/
typedef TSTDEVPDMDEV *PTSTDEVPDMDEV;
/** Pointer to a constant PDM device descriptor .*/
typedef const TSTDEVPDMDEV *PCTSTDEVPDMDEV;


/**
 * CFGM node structure.
 */
typedef struct CFGMNODE
{
    /** Device under test this CFGM node is for. */
    PTSTDEVDUTINT        pDut;
    /** @todo: */
} CFGMNODE;


/**
 * Private device instance data.
 */
typedef struct PDMDEVINSINTR3
{
    /** Pointer to the device under test the PDM device instance is for. */
    PTSTDEVDUTINT                   pDut;
} PDMDEVINSINTR3;
AssertCompile(sizeof(PDMDEVINSINTR3) <= (HC_ARCH_BITS == 32 ? 72 : 112 + 0x28));

/**
 * Private device instance data.
 */
typedef struct PDMDEVINSINTR0
{
    /** Pointer to the device under test the PDM device instance is for. */
    PTSTDEVDUTINT                   pDut;
} PDMDEVINSINTR0;
AssertCompile(sizeof(PDMDEVINSINTR0) <= (HC_ARCH_BITS == 32 ? 72 : 112 + 0x28));

/**
 * Private device instance data.
 */
typedef struct PDMDEVINSINTRC
{
    /** Pointer to the device under test the PDM device instance is for. */
    PTSTDEVDUTINT                   pDut;
} PDMDEVINSINTRC;
AssertCompile(sizeof(PDMDEVINSINTRC) <= (HC_ARCH_BITS == 32 ? 72 : 112 + 0x28));

typedef struct PDMPCIDEVINT
{
    bool                            fRegistered;
} PDMPCIDEVINT;


/**
 * Internal PDM critical section structure.
 */
typedef struct PDMCRITSECTINT
{
    /** The actual critical section used for emulation. */
    RTCRITSECT           CritSect;
} PDMCRITSECTINT;
AssertCompile(sizeof(PDMCRITSECTINT) <= (HC_ARCH_BITS == 32 ? 0x80 : 0xc0));


/**
 * SSM handle state.
 */
typedef struct SSMHANDLE
{
    /** Pointer to the device under test the handle is for. */
    PTSTDEVDUTINT                   pDut;
    /** The saved state data buffer. */
    uint8_t                         *pbSavedState;
    /** Size of the saved state. */
    size_t                          cbSavedState;
    /** Current offset into the data buffer. */
    uint32_t                        offDataBuffer;
    /** Current unit version. */
    uint32_t                        uCurUnitVer;
    /** Status code. */
    int                             rc;
} SSMHANDLE;


/**
 * MM Heap allocation.
 */
typedef struct TSTDEVMMHEAPALLOC
{
    /** Node for the list of allocations. */
    RTLISTNODE                      NdMmHeap;
    /** Pointer to the device under test the allocation was made for. */
    PTSTDEVDUTINT                   pDut;
    /** Size of the allocation. */
    size_t                          cbAlloc;
    /** Start of the real allocation. */
    RT_FLEXIBLE_ARRAY_EXTENSION
    uint8_t                         abAlloc[RT_FLEXIBLE_ARRAY];
} TSTDEVMMHEAPALLOC;
/** Pointer to a MM Heap allocation. */
typedef TSTDEVMMHEAPALLOC *PTSTDEVMMHEAPALLOC;
/** Pointer to a const MM Heap allocation. */
typedef const TSTDEVMMHEAPALLOC *PCTSTDEVMMHEAPALLOC;

AssertCompileMemberAlignment(TSTDEVMMHEAPALLOC, abAlloc, HC_ARCH_BITS == 64 ? 16 : 8);


/**
 * The usual device/driver/internal/external stuff.
 */
typedef enum
{
    /** The usual invalid entry. */
    PDMTHREADTYPE_INVALID = 0,
    /** Device type. */
    PDMTHREADTYPE_DEVICE,
    /** USB Device type. */
    PDMTHREADTYPE_USB,
    /** Driver type. */
    PDMTHREADTYPE_DRIVER,
    /** Internal type. */
    PDMTHREADTYPE_INTERNAL,
    /** External type. */
    PDMTHREADTYPE_EXTERNAL,
    /** The usual 32-bit hack. */
    PDMTHREADTYPE_32BIT_HACK = 0x7fffffff
} PDMTHREADTYPE;


/**
 * The internal structure for the thread.
 */
typedef struct PDMTHREADINT
{
    /** Node for the list of threads. */
    RTLISTNODE                      NdPdmThrds;
    /** Pointer to the device under test the allocation was made for. */
    PTSTDEVDUTINT                   pDut;
    /** The event semaphore the thread blocks on when not running. */
    RTSEMEVENTMULTI                 BlockEvent;
    /** The event semaphore the thread sleeps on while running. */
    RTSEMEVENTMULTI                 SleepEvent;
    /** The thread type. */
    PDMTHREADTYPE                   enmType;
} PDMTHREADINT;


#define PDMTHREADINT_DECLARED
#define PDMCRITSECTINT_DECLARED
#define PDMDEVINSINT_DECLARED
#define PDMPCIDEVINT_DECLARED
#define VMM_INCLUDED_SRC_include_VMInternal_h
#define VMM_INCLUDED_SRC_include_VMMInternal_h
RT_C_DECLS_END
#include <VBox/vmm/pdmcritsect.h>
#include <VBox/vmm/pdmthread.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmpci.h>
#include <VBox/vmm/pdmdrv.h>
#include <VBox/vmm/tm.h>
RT_C_DECLS_BEGIN


/**
 * TM timer structure.
 */
typedef struct TMTIMER
{
    /** List of timers created by the device. */
    RTLISTNODE           NdDevTimers;
    /** Clock this timer belongs to. */
    TMCLOCK              enmClock;
    /** Callback to call when the timer expires. */
    PFNTMTIMERDEV        pfnCallbackDev;
    /** Opaque user data to pass to the callback. */
    void                 *pvUser;
    /** Flags. */
    uint32_t             fFlags;
    /** Assigned critical section. */
    PPDMCRITSECT         pCritSect;
    /** @todo: */
} TMTIMER;


/**
 * PDM module descriptor type.
 */
typedef enum TSTDEVPDMMODTYPE
{
    /** Invalid module type. */
    TSTDEVPDMMODTYPE_INVALID = 0,
    /** Ring 3 module. */
    TSTDEVPDMMODTYPE_R3,
    /** Ring 0 module. */
    TSTDEVPDMMODTYPE_R0,
    /** Raw context module. */
    TSTDEVPDMMODTYPE_RC,
    /** 32bit hack. */
    TSTDEVPDMMODTYPE_32BIT_HACK = 0x7fffffff
} TSTDEVPDMMODTYPE;

/**
 * Registered I/O port access handler.
 */
typedef struct RTDEVDUTIOPORT
{
    /** Node for the list of registered handlers. */
    RTLISTNODE                      NdIoPorts;
    /** Start I/O port the handler is for. */
    RTIOPORT                        PortStart;
    /** Number of ports handled. */
    RTIOPORT                        cPorts;
    /** Opaque user data - R3. */
    void                            *pvUserR3;
    /** Out handler - R3. */
    PFNIOMIOPORTNEWOUT              pfnOutR3;
    /** In handler - R3. */
    PFNIOMIOPORTNEWIN               pfnInR3;
    /** Out string handler - R3. */
    PFNIOMIOPORTNEWOUTSTRING        pfnOutStrR3;
    /** In string handler - R3. */
    PFNIOMIOPORTNEWINSTRING         pfnInStrR3;

    /** Opaque user data - R0. */
    void                            *pvUserR0;
    /** Out handler - R0. */
    PFNIOMIOPORTNEWOUT              pfnOutR0;
    /** In handler - R0. */
    PFNIOMIOPORTNEWIN               pfnInR0;
    /** Out string handler - R0. */
    PFNIOMIOPORTNEWOUTSTRING        pfnOutStrR0;
    /** In string handler - R0. */
    PFNIOMIOPORTNEWINSTRING         pfnInStrR0;

#ifdef TSTDEV_SUPPORTS_RC
    /** Opaque user data - RC. */
    void                            *pvUserRC;
    /** Out handler - RC. */
    PFNIOMIOPORTNEWOUT              pfnOutRC;
    /** In handler - RC. */
    PFNIOMIOPORTNEWIN               pfnInRC;
    /** Out string handler - RC. */
    PFNIOMIOPORTNEWOUTSTRING        pfnOutStrRC;
    /** In string handler - RC. */
    PFNIOMIOPORTNEWINSTRING         pfnInStrRC;
#endif
} RTDEVDUTIOPORT;
/** Pointer to a registered I/O port handler. */
typedef RTDEVDUTIOPORT *PRTDEVDUTIOPORT;
/** Pointer to a const I/O port handler. */
typedef const RTDEVDUTIOPORT *PCRTDEVDUTIOPORT;


/**
 * Registered MMIO port access handler.
 */
typedef struct RTDEVDUTMMIO
{
    /** Node for the list of registered handlers. */
    RTLISTNODE                      NdMmio;
    /** Start address of the MMIO region when mapped. */
    RTGCPHYS                        GCPhysStart;
    /** Size of the MMIO region in bytes. */
    RTGCPHYS                        cbRegion;
    /** Opaque user data - R3. */
    void                            *pvUserR3;
    /** Write handler - R3. */
    PFNIOMMMIONEWWRITE              pfnWriteR3;
    /** Read handler - R3. */
    PFNIOMMMIONEWREAD               pfnReadR3;
    /** Fill handler - R3. */
    PFNIOMMMIONEWFILL               pfnFillR3;

    /** Opaque user data - R0. */
    void                            *pvUserR0;
    /** Write handler - R0. */
    PFNIOMMMIONEWWRITE              pfnWriteR0;
    /** Read handler - R0. */
    PFNIOMMMIONEWREAD               pfnReadR0;
    /** Fill handler - R0. */
    PFNIOMMMIONEWFILL               pfnFillR0;

#ifdef TSTDEV_SUPPORTS_RC
    /** Opaque user data - RC. */
    void                            *pvUserRC;
    /** Write handler - RC. */
    PFNIOMMMIONEWWRITE              pfnWriteRC;
    /** Read handler - RC. */
    PFNIOMMMIONEWREAD               pfnReadRC;
    /** Fill handler - RC. */
    PFNIOMMMIONEWFILL               pfnFillRC;
#endif
} RTDEVDUTMMIO;
/** Pointer to a registered MMIO handler. */
typedef RTDEVDUTMMIO *PRTDEVDUTMMIO;
/** Pointer to a const MMIO handler. */
typedef const RTDEVDUTMMIO *PCRTDEVDUTMMIO;


#ifdef IN_RING3
/**
 * Registered SSM handlers.
 */
typedef struct TSTDEVDUTSSM
{
    /** Node for the list of registered SSM handlers. */
    RTLISTNODE                      NdSsm;
    /** Version */
    uint32_t                        uVersion;
    PFNSSMDEVLIVEPREP               pfnLivePrep;
    PFNSSMDEVLIVEEXEC               pfnLiveExec;
    PFNSSMDEVLIVEVOTE               pfnLiveVote;
    PFNSSMDEVSAVEPREP               pfnSavePrep;
    PFNSSMDEVSAVEEXEC               pfnSaveExec;
    PFNSSMDEVSAVEDONE               pfnSaveDone;
    PFNSSMDEVLOADPREP               pfnLoadPrep;
    PFNSSMDEVLOADEXEC               pfnLoadExec;
    PFNSSMDEVLOADDONE               pfnLoadDone;
} TSTDEVDUTSSM;
/** Pointer to the registered SSM handlers. */
typedef TSTDEVDUTSSM *PTSTDEVDUTSSM;
/** Pointer to a const SSM handler. */
typedef const TSTDEVDUTSSM *PCTSTDEVDUTSSM;
#endif


/**
 * The Support Driver session state.
 */
typedef struct TSTDEVSUPDRVSESSION
{
    /** Pointer to the owning device under test instance. */
    PTSTDEVDUTINT                   pDut;
    /** List of event semaphores. */
    RTLISTANCHOR                    LstSupSem;
} TSTDEVSUPDRVSESSION;
/** Pointer to the Support Driver session state. */
typedef TSTDEVSUPDRVSESSION *PTSTDEVSUPDRVSESSION;

/** Converts a Support Driver session handle to the internal state. */
#define TSTDEV_PSUPDRVSESSION_2_PTSTDEVSUPDRVSESSION(a_pSession) ((PTSTDEVSUPDRVSESSION)(a_pSession))
/** Converts the internal session state to a Support Driver session handle. */
#define TSTDEV_PTSTDEVSUPDRVSESSION_2_PSUPDRVSESSION(a_pSession) ((PSUPDRVSESSION)(a_pSession))

/**
 * Support driver event semaphore.
 */
typedef struct TSTDEVSUPSEMEVENT
{
    /** Node for the event semaphore list. */
    RTLISTNODE                      NdSupSem;
    /** Flag whether this is multi event semaphore. */
    bool                            fMulti;
    /** Event smeaphore handles depending on the flag above. */
    union
    {
        RTSEMEVENT                  hSemEvt;
        RTSEMEVENTMULTI             hSemEvtMulti;
    } u;
} TSTDEVSUPSEMEVENT;
/** Pointer to a support event semaphore state. */
typedef TSTDEVSUPSEMEVENT *PTSTDEVSUPSEMEVENT;

/** Converts a Support event semaphore handle to the internal state. */
#define TSTDEV_SUPSEMEVENT_2_PTSTDEVSUPSEMEVENT(a_pSupSemEvt) ((PTSTDEVSUPSEMEVENT)(a_pSupSemEvt))
/** Converts the internal session state to a Support event semaphore handle. */
#define TSTDEV_PTSTDEVSUPSEMEVENT_2_SUPSEMEVENT(a_pSupSemEvt) ((SUPSEMEVENT)(a_pSupSemEvt))

/**
 * The contex the device under test is currently in.
 */
typedef enum TSTDEVDUTCTX
{
    /** Invalid context. */
    TSTDEVDUTCTX_INVALID = 0,
    /** R3 context. */
    TSTDEVDUTCTX_R3,
    /** R0 context. */
    TSTDEVDUTCTX_R0,
    /** RC context. */
    TSTDEVDUTCTX_RC,
    /** 32bit hack. */
    TSTDEVDUTCTX_32BIT_HACK = 0x7fffffff
} TSTDEVDUTCTX;

/**
 * PCI region descriptor.
 */
typedef struct TSTDEVDUTPCIREGION
{
    /** Size of the region. */
    RTGCPHYS                        cbRegion;
    /** Address space type. */
    PCIADDRESSSPACE                 enmType;
    /** Region mapping callback. */
    PFNPCIIOREGIONMAP               pfnRegionMap;
} TSTDEVDUTPCIREGION;
/** Pointer to a PCI region descriptor. */
typedef TSTDEVDUTPCIREGION *PTSTDEVDUTPCIREGION;
/** Pointer to a const PCI region descriptor. */
typedef const TSTDEVDUTPCIREGION *PCTSTDEVDUTPCIREGION;

/**
 * Device under test instance data.
 */
typedef struct TSTDEVDUTINT
{
    /** Pointer to the test this device is running under. */
    PCTSTDEVTEST                    pTest;
    /** The PDM device registration record. */
    PCTSTDEVPDMDEV                  pPdmDev;
    /** Pointer to the PDM device instance. */
    struct PDMDEVINSR3             *pDevIns;
    /** Pointer to the PDM R0 device instance. */
    struct PDMDEVINSR0             *pDevInsR0;
    /** CFGM root config node for the device. */
    CFGMNODE                        Cfg;
    /** Current device context. */
    TSTDEVDUTCTX                    enmCtx;
    /** Critical section protecting the lists below. */
    RTCRITSECTRW                    CritSectLists;
    /** List of registered I/O port handlers. */
    RTLISTANCHOR                    LstIoPorts;
    /** List of timers registered. */
    RTLISTANCHOR                    LstTimers;
    /** List of registered MMIO regions. */
    RTLISTANCHOR                    LstMmio;
    /** List of MM Heap allocations. */
    RTLISTANCHOR                    LstMmHeap;
    /** List of PDM threads. */
    RTLISTANCHOR                    LstPdmThreads;
    /** List of SSM handlers (just one normally). */
    RTLISTANCHOR                    LstSsmHandlers;
    /** The SUP session we emulate. */
    TSTDEVSUPDRVSESSION             SupSession;
    /** The NOP critical section. */
    PDMCRITSECT                     CritSectNop;
    /** The VM state associated with this device. */
    PVM                             pVm;
    /** The registered PCI device instance if this is a PCI device. */
    PPDMPCIDEV                      pPciDev;
    /** PCI Region descriptors. */
    TSTDEVDUTPCIREGION              aPciRegions[VBOX_PCI_NUM_REGIONS];
    /** The status port interface we implement. */
    PDMIBASE                        IBaseSts;
    /**  */
} TSTDEVDUTINT;


#ifdef IN_RING3
extern const PDMDEVHLPR3 g_tstDevPdmDevHlpR3;
#endif
extern const PDMDEVHLPR0 g_tstDevPdmDevHlpR0;

DECLHIDDEN(int) tstDevPdmLdrGetSymbol(PTSTDEVDUTINT pThis, const char *pszMod, TSTDEVPDMMODTYPE enmModType,
                                      const char *pszSymbol, PFNRT *ppfn);


DECLINLINE(int) tstDevDutLockShared(PTSTDEVDUTINT pThis)
{
    return RTCritSectRwEnterShared(&pThis->CritSectLists);
}

DECLINLINE(int) tstDevDutUnlockShared(PTSTDEVDUTINT pThis)
{
    return RTCritSectRwLeaveShared(&pThis->CritSectLists);
}

DECLINLINE(int) tstDevDutLockExcl(PTSTDEVDUTINT pThis)
{
    return RTCritSectRwEnterExcl(&pThis->CritSectLists);
}

DECLINLINE(int) tstDevDutUnlockExcl(PTSTDEVDUTINT pThis)
{
    return RTCritSectRwLeaveExcl(&pThis->CritSectLists);
}

DECLHIDDEN(int) tstDevPdmR3ThreadCreateDevice(PTSTDEVDUTINT pDut, PPDMDEVINS pDevIns, PPPDMTHREAD ppThread, void *pvUser, PFNPDMTHREADDEV pfnThread,
                                              PFNPDMTHREADWAKEUPDEV pfnWakeUp, size_t cbStack, RTTHREADTYPE enmType, const char *pszName);
DECLHIDDEN(int) tstDevPdmR3ThreadCreateUsb(PTSTDEVDUTINT pDut, PPDMUSBINS pUsbIns, PPPDMTHREAD ppThread, void *pvUser, PFNPDMTHREADUSB pfnThread,
                                           PFNPDMTHREADWAKEUPUSB pfnWakeUp, size_t cbStack, RTTHREADTYPE enmType, const char *pszName);
DECLHIDDEN(int) tstDevPdmR3ThreadCreateDriver(PTSTDEVDUTINT pDut, PPDMDRVINS pDrvIns, PPPDMTHREAD ppThread, void *pvUser, PFNPDMTHREADDRV pfnThread,
                                              PFNPDMTHREADWAKEUPDRV pfnWakeUp, size_t cbStack, RTTHREADTYPE enmType, const char *pszName);
DECLHIDDEN(int) tstDevPdmR3ThreadCreate(PTSTDEVDUTINT pDut, PPPDMTHREAD ppThread, void *pvUser, PFNPDMTHREADINT pfnThread,
                                        PFNPDMTHREADWAKEUPINT pfnWakeUp, size_t cbStack, RTTHREADTYPE enmType, const char *pszName);
DECLHIDDEN(int) tstDevPdmR3ThreadCreateExternal(PTSTDEVDUTINT pDut, PPPDMTHREAD ppThread, void *pvUser, PFNPDMTHREADEXT pfnThread,
                                                PFNPDMTHREADWAKEUPEXT pfnWakeUp, size_t cbStack, RTTHREADTYPE enmType, const char *pszName);
DECLHIDDEN(int) tstDevPdmR3ThreadDestroy(PPDMTHREAD pThread, int *pRcThread);
DECLHIDDEN(int) tstDevPdmR3ThreadDestroyDevice(PTSTDEVDUTINT pDut, PPDMDEVINS pDevIns);
DECLHIDDEN(int) tstDevPdmR3ThreadDestroyUsb(PTSTDEVDUTINT pDut, PPDMUSBINS pUsbIns);
DECLHIDDEN(int) tstDevPdmR3ThreadDestroyDriver(PTSTDEVDUTINT pDut, PPDMDRVINS pDrvIns);
DECLHIDDEN(void) tstDevPdmR3ThreadDestroyAll(PTSTDEVDUTINT pDut);
DECLHIDDEN(int) tstDevPdmR3ThreadIAmSuspending(PPDMTHREAD pThread);
DECLHIDDEN(int) tstDevPdmR3ThreadIAmRunning(PPDMTHREAD pThread);
DECLHIDDEN(int) tstDevPdmR3ThreadSleep(PPDMTHREAD pThread, RTMSINTERVAL cMillies);
DECLHIDDEN(int) tstDevPdmR3ThreadSuspend(PPDMTHREAD pThread);
DECLHIDDEN(int) tstDevPdmR3ThreadResume(PPDMTHREAD pThread);


DECLHIDDEN(PCTSTDEVPDMDEV) tstDevPdmDeviceFind(const char *pszName, PCPDMDEVREGR0 *ppR0Reg);
DECLHIDDEN(int) tstDevPdmDeviceR3Construct(PTSTDEVDUTINT pDut);

DECLHIDDEN(int) tstDevPdmDevR0R3Create(const char *pszName, bool fRCEnabled, PTSTDEVDUTINT pDut);


RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_SRC_testcase_tstDeviceInternal_h */
