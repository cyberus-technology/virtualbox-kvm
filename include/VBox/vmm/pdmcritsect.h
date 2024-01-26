/** @file
 * PDM - Pluggable Device Manager, Critical Sections.
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

#ifndef VBOX_INCLUDED_vmm_pdmcritsect_h
#define VBOX_INCLUDED_vmm_pdmcritsect_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#include <iprt/critsect.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_pdm_critsect      The PDM Critical Section API
 * @ingroup grp_pdm
 * @{
 */

/**
 * A PDM critical section.
 * Initialize using PDMDRVHLP::pfnCritSectInit().
 */
typedef union PDMCRITSECT
{
    /** Padding. */
    uint8_t padding[HC_ARCH_BITS == 32 ? 0xc0 : 0x100];
#ifdef PDMCRITSECTINT_DECLARED
    /** The internal structure (not normally visible). */
    struct PDMCRITSECTINT s;
#endif
} PDMCRITSECT;

VMMR3_INT_DECL(int)     PDMR3CritSectBothTerm(PVM pVM);
VMMR3_INT_DECL(void)    PDMR3CritSectLeaveAll(PVM pVM);
VMM_INT_DECL(void)      PDMCritSectBothFF(PVMCC pVM, PVMCPUCC pVCpu);


VMMR3DECL(uint32_t) PDMR3CritSectCountOwned(PVM pVM, char *pszNames, size_t cbNames);

VMMR3DECL(int)      PDMR3CritSectInit(PVM pVM, PPDMCRITSECT pCritSect, RT_SRC_POS_DECL,
                                      const char *pszNameFmt, ...) RT_IPRT_FORMAT_ATTR(6, 7);
VMMR3DECL(int)      PDMR3CritSectEnterEx(PVM pVM, PPDMCRITSECT pCritSect, bool fCallRing3);
VMMR3DECL(bool)     PDMR3CritSectYield(PVM pVM, PPDMCRITSECT pCritSect);
VMMR3DECL(const char *) PDMR3CritSectName(PCPDMCRITSECT pCritSect);
VMMR3DECL(int)      PDMR3CritSectDelete(PVM pVM, PPDMCRITSECT pCritSect);
#if defined(IN_RING0) || defined(IN_RING3)
VMMDECL(int)        PDMHCCritSectScheduleExitEvent(PPDMCRITSECT pCritSect, SUPSEMEVENT hEventToSignal);
#endif

VMMDECL(DECL_CHECK_RETURN_NOT_R3(int))
                    PDMCritSectEnter(PVMCC pVM, PPDMCRITSECT pCritSect, int rcBusy);
VMMDECL(DECL_CHECK_RETURN_NOT_R3(int))
                    PDMCritSectEnterDebug(PVMCC pVM, PPDMCRITSECT pCritSect, int rcBusy, RTHCUINTPTR uId, RT_SRC_POS_DECL);
VMMDECL(DECL_CHECK_RETURN(int))
                    PDMCritSectTryEnter(PVMCC pVM, PPDMCRITSECT pCritSect);
VMMDECL(DECL_CHECK_RETURN(int))
                    PDMCritSectTryEnterDebug(PVMCC pVM, PPDMCRITSECT pCritSect, RTHCUINTPTR uId, RT_SRC_POS_DECL);
VMMDECL(int)        PDMCritSectLeave(PVMCC pVM, PPDMCRITSECT pCritSect);

VMMDECL(bool)       PDMCritSectIsOwner(PVMCC pVM, PCPDMCRITSECT pCritSect);
VMMDECL(bool)       PDMCritSectIsOwnerEx(PVMCPUCC pVCpu, PCPDMCRITSECT pCritSect);
VMMDECL(bool)       PDMCritSectIsInitialized(PCPDMCRITSECT pCritSect);
VMMDECL(bool)       PDMCritSectHasWaiters(PVMCC pVM, PCPDMCRITSECT pCritSect);
VMMDECL(uint32_t)   PDMCritSectGetRecursion(PCPDMCRITSECT pCritSect);

VMMR3DECL(PPDMCRITSECT)             PDMR3CritSectGetNop(PVM pVM);

/* Strict build: Remap the two enter calls to the debug versions. */
#ifdef VBOX_STRICT
# ifdef IPRT_INCLUDED_asm_h
#  define PDMCritSectEnter(a_pVM, pCritSect, rcBusy)   PDMCritSectEnterDebug((a_pVM), (pCritSect), (rcBusy), (uintptr_t)ASMReturnAddress(), RT_SRC_POS)
#  define PDMCritSectTryEnter(a_pVM, pCritSect)        PDMCritSectTryEnterDebug((a_pVM), (pCritSect), (uintptr_t)ASMReturnAddress(), RT_SRC_POS)
# else
#  define PDMCritSectEnter(a_pVM, pCritSect, rcBusy)   PDMCritSectEnterDebug((a_pVM), (pCritSect), (rcBusy), 0, RT_SRC_POS)
#  define PDMCritSectTryEnter(a_pVM, pCritSect)        PDMCritSectTryEnterDebug((a_pVM), (pCritSect), 0, RT_SRC_POS)
# endif
#endif

/** @def PDM_CRITSECT_RELEASE_ASSERT_RC
 * Helper for PDMCritSectEnter w/ rcBusy VINF_SUCCESS when there is no way
 * to forward failures to the caller. */
#define PDM_CRITSECT_RELEASE_ASSERT_RC(a_pVM, a_pCritSect, a_rc) \
    AssertReleaseMsg(RT_SUCCESS(a_rc), ("pVM=%p pCritSect=%p: %Rrc\n", (a_pVM), (a_pCritSect), (a_rc)))

/** @def PDM_CRITSECT_RELEASE_ASSERT_RC_DEV
 * Helper for PDMCritSectEnter w/ rcBusy VINF_SUCCESS when there is no way
 * to forward failures to the caller, device edition. */
#define PDM_CRITSECT_RELEASE_ASSERT_RC_DEV(a_pDevIns, a_pCritSect, a_rc) \
    AssertReleaseMsg(RT_SUCCESS(a_rc), ("pDevIns=%p pCritSect=%p: %Rrc\n", (a_pDevIns), (a_pCritSect), (a_rc)))

/** @def PDM_CRITSECT_RELEASE_ASSERT_RC_DRV
 * Helper for PDMCritSectEnter w/ rcBusy VINF_SUCCESS when there is no way
 * to forward failures to the caller, driver edition. */
#define PDM_CRITSECT_RELEASE_ASSERT_RC_DRV(a_pDrvIns, a_pCritSect, a_rc) \
    AssertReleaseMsg(RT_SUCCESS(a_rc), ("pDrvIns=%p pCritSect=%p: %Rrc\n", (a_pDrvIns), (a_pCritSect), (a_rc)))

/** @def PDM_CRITSECT_RELEASE_ASSERT_RC_USB
 * Helper for PDMCritSectEnter w/ rcBusy VINF_SUCCESS when there is no way
 * to forward failures to the caller, USB device edition. */
#define PDM_CRITSECT_RELEASE_ASSERT_RC_USB(a_pUsbIns, a_pCritSect, a_rc) \
    AssertReleaseMsg(RT_SUCCESS(a_rc), ("pUsbIns=%p pCritSect=%p: %Rrc\n", (a_pUsbIns), (a_pCritSect), (a_rc)))


/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_pdmcritsect_h */

