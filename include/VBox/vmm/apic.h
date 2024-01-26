/** @file
 * APIC - Advanced Programmable Interrupt Controller.
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

#ifndef VBOX_INCLUDED_vmm_apic_h
#define VBOX_INCLUDED_vmm_apic_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#include <VBox/apic.h>
struct PDMDEVREGCB;

/** @defgroup grp_apic   The local APIC VMM API
 * @ingroup grp_vmm
 * @{
 */

RT_C_DECLS_BEGIN

#ifdef VBOX_INCLUDED_vmm_pdmdev_h
extern const PDMDEVREG g_DeviceAPIC;
#endif

/* These functions are exported as they are called from external modules (recompiler). */
VMMDECL(void)               APICUpdatePendingInterrupts(PVMCPUCC pVCpu);
VMMDECL(int)                APICGetTpr(PCVMCPUCC pVCpu, uint8_t *pu8Tpr, bool *pfPending, uint8_t *pu8PendingIntr);
VMMDECL(int)                APICSetTpr(PVMCPUCC pVCpu, uint8_t u8Tpr);

/* These functions are VMM internal. */
VMM_INT_DECL(bool)          APICIsEnabled(PCVMCPUCC pVCpu);
VMM_INT_DECL(bool)          APICGetHighestPendingInterrupt(PVMCPUCC pVCpu, uint8_t *pu8PendingIntr);
VMM_INT_DECL(bool)          APICQueueInterruptToService(PVMCPUCC pVCpu, uint8_t u8PendingIntr);
VMM_INT_DECL(void)          APICDequeueInterruptFromService(PVMCPUCC pVCpu, uint8_t u8PendingIntr);
VMM_INT_DECL(VBOXSTRICTRC)  APICReadMsr(PVMCPUCC pVCpu, uint32_t u32Reg, uint64_t *pu64Value);
VMM_INT_DECL(VBOXSTRICTRC)  APICWriteMsr(PVMCPUCC pVCpu, uint32_t u32Reg, uint64_t u64Value);
VMM_INT_DECL(int)           APICGetTimerFreq(PVMCC pVM, uint64_t *pu64Value);
VMM_INT_DECL(VBOXSTRICTRC)  APICLocalInterrupt(PVMCPUCC pVCpu, uint8_t u8Pin, uint8_t u8Level, int rcRZ);
VMM_INT_DECL(uint64_t)      APICGetBaseMsrNoCheck(PCVMCPUCC pVCpu);
VMM_INT_DECL(VBOXSTRICTRC)  APICGetBaseMsr(PVMCPUCC pVCpu, uint64_t *pu64Value);
VMM_INT_DECL(int)           APICSetBaseMsr(PVMCPUCC pVCpu, uint64_t u64BaseMsr);
VMM_INT_DECL(int)           APICGetInterrupt(PVMCPUCC pVCpu, uint8_t *pu8Vector, uint32_t *pu32TagSrc);
VMM_INT_DECL(int)           APICBusDeliver(PVMCC pVM, uint8_t uDest, uint8_t uDestMode, uint8_t uDeliveryMode, uint8_t uVector,
                                           uint8_t uPolarity, uint8_t uTriggerMode, uint32_t uTagSrc);
VMM_INT_DECL(int)           APICGetApicPageForCpu(PCVMCPUCC pVCpu, PRTHCPHYS pHCPhys, PRTR0PTR pR0Ptr, PRTR3PTR pR3Ptr);

/** @name Hyper-V interface (Ring-3 and all-context API).
 * @{ */
#ifdef IN_RING3
VMMR3_INT_DECL(void)        APICR3HvSetCompatMode(PVM pVM, bool fHyperVCompatMode);
#endif
VMM_INT_DECL(void)          APICHvSendInterrupt(PVMCPUCC pVCpu, uint8_t uVector, bool fAutoEoi, XAPICTRIGGERMODE enmTriggerMode);
VMM_INT_DECL(VBOXSTRICTRC)  APICHvSetTpr(PVMCPUCC pVCpu, uint8_t uTpr);
VMM_INT_DECL(uint8_t)       APICHvGetTpr(PVMCPUCC pVCpu);
VMM_INT_DECL(VBOXSTRICTRC)  APICHvSetIcr(PVMCPUCC pVCpu, uint64_t uIcr);
VMM_INT_DECL(uint64_t)      APICHvGetIcr(PVMCPUCC pVCpu);
VMM_INT_DECL(VBOXSTRICTRC)  APICHvSetEoi(PVMCPUCC pVCpu, uint32_t uEoi);
/** @} */

#ifdef IN_RING3
/** @defgroup grp_apic_r3  The APIC Host Context Ring-3 API
 * @{
 */
VMMR3_INT_DECL(int)         APICR3RegisterDevice(struct PDMDEVREGCB *pCallbacks);
VMMR3_INT_DECL(void)        APICR3InitIpi(PVMCPU pVCpu);
VMMR3_INT_DECL(void)        APICR3HvEnable(PVM pVM);
/** @} */
#endif /* IN_RING3 */

RT_C_DECLS_END

/** @} */

#endif /* !VBOX_INCLUDED_vmm_apic_h */

