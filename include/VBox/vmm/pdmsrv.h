/** @file
 * PDM - Pluggable Device Manager, VM Services.
 *
 * @todo    This has not been implemented, consider dropping the concept.
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

#ifndef VBOX_INCLUDED_vmm_pdmsrv_h
#define VBOX_INCLUDED_vmm_pdmsrv_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/vmm/pdmifs.h>
#include <VBox/vmm/ssm.h>
#include <VBox/vmm/cfgm.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_pdm_services  The PDM Services API
 * @ingroup grp_pdm
 * @{
 */

/**
 * Construct a service instance for a VM.
 *
 * @returns VBox status.
 * @param   pSrvIns     The service instance data.
 *                      If the registration structure is needed, pSrvIns->pReg points to it.
 * @param   pCfg        Configuration node handle for the service. Use this to obtain the configuration
 *                      of the driver instance. It's also found in pSrvIns->pCfg, but since it's primary
 *                      usage is expected in this function it is passed as a parameter.
 */
typedef DECLCALLBACKTYPE(int, FNPDMSRVCONSTRUCT,(PPDMSRVINS pSrvIns, PCFGMNODE pCfg));
/** Pointer to a FNPDMSRVCONSTRUCT() function. */
typedef FNPDMSRVCONSTRUCT *PFNPDMSRVCONSTRUCT;

/**
 * Destruct a driver instance.
 *
 * Most VM resources are freed by the VM. This callback is provided so that any non-VM
 * resources can be freed correctly.
 *
 * @param   pSrvIns     The service instance data.
 */
typedef DECLCALLBACKTYPE(void, FNPDMSRVDESTRUCT,(PPDMSRVINS pSrvIns));
/** Pointer to a FNPDMSRVDESTRUCT() function. */
typedef FNPDMSRVDESTRUCT *PFNPDMSRVDESTRUCT;

/**
 * Power On notification.
 *
 * @param   pSrvIns     The service instance data.
 */
typedef DECLCALLBACKTYPE(void, FNPDMSRVPOWERON,(PPDMSRVINS pSrvIns));
/** Pointer to a FNPDMSRVPOWERON() function. */
typedef FNPDMSRVPOWERON *PFNPDMSRVPOWERON;

/**
 * Reset notification.
 *
 * @param   pSrvIns     The service instance data.
 */
typedef DECLCALLBACKTYPE(void, FNPDMSRVRESET,(PPDMSRVINS pSrvIns));
/** Pointer to a FNPDMSRVRESET() function. */
typedef FNPDMSRVRESET *PFNPDMSRVRESET;

/**
 * Suspend notification.
 *
 * @param   pSrvIns     The service instance data.
 */
typedef DECLCALLBACKTYPE(void, FNPDMSRVSUSPEND,(PPDMSRVINS pSrvIns));
/** Pointer to a FNPDMSRVSUSPEND() function. */
typedef FNPDMSRVSUSPEND *PFNPDMSRVSUSPEND;

/**
 * Resume notification.
 *
 * @param   pSrvIns     The service instance data.
 */
typedef DECLCALLBACKTYPE(void, FNPDMSRVRESUME,(PPDMSRVINS pSrvIns));
/** Pointer to a FNPDMSRVRESUME() function. */
typedef FNPDMSRVRESUME *PFNPDMSRVRESUME;

/**
 * Power Off notification.
 *
 * @param   pSrvIns     The service instance data.
 */
typedef DECLCALLBACKTYPE(void, FNPDMSRVPOWEROFF,(PPDMSRVINS pSrvIns));
/** Pointer to a FNPDMSRVPOWEROFF() function. */
typedef FNPDMSRVPOWEROFF *PFNPDMSRVPOWEROFF;

/**
 * Detach notification.
 *
 * This is called when a driver or device is detached from the service
 *
 * @param   pSrvIns     The service instance data.
 * @param   pDevIns     The device instance to detach.
 * @param   pDrvIns     The driver instance to detach.
 */
typedef DECLCALLBACKTYPE(void, FNPDMSRVDETACH,(PPDMSRVINS pSrvIns, PPDMDEVINS pDevIns, PPDMDRVINS pDrvIns));
/** Pointer to a FNPDMSRVDETACH() function. */
typedef FNPDMSRVDETACH *PFNPDMSRVDETACH;



/** PDM Service Registration Structure,
 * This structure is used when registering a driver from
 * VBoxServicesRegister() (HC Ring-3). PDM will continue use till
 * the VM is terminated.
 */
typedef struct PDMSRVREG
{
    /** Structure version. PDM_SRVREG_VERSION defines the current version. */
    uint32_t            u32Version;
    /** Driver name. */
    char                szServiceName[32];
    /** The description of the driver. The UTF-8 string pointed to shall, like this structure,
     * remain unchanged from registration till VM destruction. */
    const char         *pszDescription;

    /** Flags, combination of the PDM_SRVREG_FLAGS_* \#defines. */
    RTUINT              fFlags;
    /** Size of the instance data. */
    RTUINT              cbInstance;

    /** Construct instance - required. */
    PFNPDMSRVCONSTRUCT  pfnConstruct;
    /** Destruct instance - optional. */
    PFNPDMSRVDESTRUCT   pfnDestruct;
    /** Power on notification - optional. */
    PFNPDMSRVPOWERON    pfnPowerOn;
    /** Reset notification - optional. */
    PFNPDMSRVRESET      pfnReset;
    /** Suspend notification  - optional. */
    PFNPDMSRVSUSPEND    pfnSuspend;
    /** Resume notification - optional. */
    PFNPDMSRVRESUME     pfnResume;
    /** Detach notification - optional. */
    PFNPDMSRVDETACH     pfnDetach;
    /** Power off notification - optional. */
    PFNPDMSRVPOWEROFF   pfnPowerOff;

} PDMSRVREG;
/** Pointer to a PDM Driver Structure. */
typedef PDMSRVREG *PPDMSRVREG;
/** Const pointer to a PDM Driver Structure. */
typedef PDMSRVREG const *PCPDMSRVREG;



/**
 * PDM Service API.
 */
typedef struct PDMSRVHLP
{
    /** Structure version. PDM_SRVHLP_VERSION defines the current version. */
    uint32_t                    u32Version;

    /**
     * Assert that the current thread is the emulation thread.
     *
     * @returns True if correct.
     * @returns False if wrong.
     * @param   pSrvIns         Service instance.
     * @param   pszFile         Filename of the assertion location.
     * @param   iLine           Linenumber of the assertion location.
     * @param   pszFunction     Function of the assertion location.
     */
    DECLR3CALLBACKMEMBER(bool, pfnAssertEMT,(PPDMSRVINS pSrvIns, const char *pszFile, unsigned iLine, const char *pszFunction));

    /**
     * Assert that the current thread is NOT the emulation thread.
     *
     * @returns True if correct.
     * @returns False if wrong.
     * @param   pSrvIns         Service instance.
     * @param   pszFile         Filename of the assertion location.
     * @param   iLine           Linenumber of the assertion location.
     * @param   pszFunction     Function of the assertion location.
     */
    DECLR3CALLBACKMEMBER(bool, pfnAssertOther,(PPDMSRVINS pSrvIns, const char *pszFile, unsigned iLine, const char *pszFunction));

    /**
     * Creates a timer.
     *
     * @returns VBox status.
     * @param   pVM             The cross context VM structure.
     * @param   pSrvIns         Service instance.
     * @param   enmClock        The clock to use on this timer.
     * @param   pfnCallback     Callback function.
     * @param   pszDesc         Pointer to description string which must stay around
     *                          until the timer is fully destroyed (i.e. a bit after TMTimerDestroy()).
     * @param   ppTimer         Where to store the timer on success.
     */
    DECLR3CALLBACKMEMBER(int, pfnTMTimerCreate,(PPDMSRVINS pSrvIns, TMCLOCK enmClock, PFNTMTIMERDEV pfnCallback, const char *pszDesc, PPTMTIMERR3 ppTimer));

    /**
     * Query the virtual timer frequency.
     *
     * @returns Frequency in Hz.
     * @param   pSrvIns         Service instance.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(uint64_t, pfnTMGetVirtualFreq,(PPDMSRVINS pSrvIns));

    /**
     * Query the virtual time.
     *
     * @returns The current virtual time.
     * @param   pSrvIns         Service instance.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(uint64_t, pfnTMGetVirtualTime,(PPDMSRVINS pSrvIns));

} PDMSRVHLP;
/** Pointer PDM Service API. */
typedef PDMSRVHLP *PPDMSRVHLP;
/** Pointer const PDM Service API. */
typedef const PDMSRVHLP *PCPDMSRVHLP;

/** Current SRVHLP version number. */
#define PDM_SRVHLP_VERSION                      PDM_VERSION_MAKE(0xdfff, 1, 0)


/**
 * PDM Service Instance.
 */
typedef struct PDMSRVINS
{
    /** Structure version. PDM_SRVINS_VERSION defines the current version. */
    uint32_t                    u32Version;

    /** Internal data. */
    union
    {
#ifdef PDMSRVINSINT_DECLARED
        PDMSRVINSINT            s;
#endif
        uint8_t                 padding[HC_ARCH_BITS == 32 ? 32 : 32];
    } Internal;

    /** Pointer the PDM Service API. */
    R3PTRTYPE(PCPDMSRVHLP)      pHlp;
    /** Pointer to driver registration structure.  */
    R3PTRTYPE(PCPDMSRVREG)      pReg;
    /** Configuration handle. */
    R3PTRTYPE(PCFGMNODE)        pCfg;
    /** The base interface of the service.
     * The service constructor initializes this. */
    PDMIBASE                    IBase;
    /* padding to make achInstanceData aligned at 16 byte boundary. */
    uint32_t                    au32Padding[2];
    /** Pointer to driver instance data. */
    R3PTRTYPE(void *)           pvInstanceData;
    /** Driver instance data. The size of this area is defined
     * in the PDMSRVREG::cbInstanceData field. */
    char                        achInstanceData[4];
} PDMSRVINS;

/** Current PDMSRVREG version number. */
#define PDM_SRVINS_VERSION                      PDM_VERSION_MAKE(0xdffe, 1, 0)

/** Converts a pointer to the PDMSRVINS::IBase to a pointer to PDMSRVINS. */
#define PDMIBASE_2_PDMSRV(pInterface) ( (PPDMSRVINS)((char *)(pInterface) - RT_UOFFSETOF(PDMSRVINS, IBase)) )



/** Pointer to callbacks provided to the VBoxServiceRegister() call. */
typedef struct PDMSRVREGCB *PPDMSRVREGCB;

/**
 * Callbacks for VBoxServiceRegister().
 */
typedef struct PDMSRVREGCB
{
    /** Interface version.
     * This is set to PDM_SRVREG_CB_VERSION. */
    uint32_t                    u32Version;

    /**
     * Registers a service with the current VM instance.
     *
     * @returns VBox status code.
     * @param   pCallbacks      Pointer to the callback table.
     * @param   pSrvReg         Pointer to the device registration record.
     *                          This data must be permanent and readonly.
     */
    DECLR3CALLBACKMEMBER(int, pfnRegister,(PPDMSRVREGCB pCallbacks, PCPDMSRVREG pSrvReg));
} PDMSRVREGCB;

/** Current version of the PDMSRVREGCB structure. */
#define PDM_SRVREG_CB_VERSION                   PDM_VERSION_MAKE(0xdffd, 1, 0)


/**
 * The VBoxServicesRegister callback function.
 *
 * PDM will invoke this function after loading a device module and letting
 * the module decide which devices to register and how to handle conflicts.
 *
 * @returns VBox status code.
 * @param   pCallbacks      Pointer to the callback table.
 * @param   u32Version      VBox version number.
 */
typedef DECLCALLBACKTYPE(int, FNPDMVBOXSERVICESREGISTER,(PPDMSRVREGCB pCallbacks, uint32_t u32Version));


/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_pdmsrv_h */
