/** @file
 * NEM - The Native Execution Manager.
 */

/*
 * Copyright (C) 2018-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_vmm_nem_h
#define VBOX_INCLUDED_vmm_nem_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#include <VBox/vmm/vmapi.h>
#include <VBox/vmm/pgm.h>

#ifdef VBOX_WITH_KVM_IRQCHIP_FULL
// For KVMPICSTATE and KVMIRQCHIP
#include <VBox/vmm/pdmdev.h>
#endif

#if defined(VBOX_WITH_KVM) && defined(IN_RING3)
#include <VBox/vmm/cpum.h>      /* for PCPUMCPUIDLEAF */
#endif

RT_C_DECLS_BEGIN

/** @defgroup grp_nem      The Native Execution Manager API
 * @ingroup grp_vmm
 * @{
 */

/** @defgroup grp_nem_r3   The NEM ring-3 Context API
 * @{
 */
VMMR3_INT_DECL(int)  NEMR3InitConfig(PVM pVM);
VMMR3_INT_DECL(int)  NEMR3Init(PVM pVM, bool fFallback, bool fForced);
VMMR3_INT_DECL(int)  NEMR3InitAfterCPUM(PVM pVM);
#ifdef IN_RING3
VMMR3_INT_DECL(int)  NEMR3InitCompleted(PVM pVM, VMINITCOMPLETED enmWhat);
#endif
VMMR3_INT_DECL(int)  NEMR3Term(PVM pVM);
VMMR3DECL(bool)      NEMR3IsEnabled(PUVM pVM);
VMMR3_INT_DECL(bool) NEMR3NeedSpecialTscMode(PVM pVM);
VMMR3_INT_DECL(void) NEMR3Reset(PVM pVM);
VMMR3_INT_DECL(void) NEMR3ResetCpu(PVMCPU pVCpu, bool fInitIpi);
VMMR3DECL(const char *) NEMR3GetExitName(uint32_t uExit);
VMMR3_INT_DECL(VBOXSTRICTRC) NEMR3RunGC(PVM pVM, PVMCPU pVCpu);
VMMR3_INT_DECL(bool) NEMR3CanExecuteGuest(PVM pVM, PVMCPU pVCpu);
VMMR3_INT_DECL(bool) NEMR3SetSingleInstruction(PVM pVM, PVMCPU pVCpu, bool fEnable);
VMMR3_INT_DECL(void) NEMR3NotifyFF(PVM pVM, PVMCPU pVCpu, uint32_t fFlags);

/**
 * Checks if dirty page tracking for MMIO2 ranges is supported.
 *
 * If it is, PGM will not install a physical write access handler for the MMIO2
 * region and instead just forward dirty bit queries NEMR3QueryMmio2DirtyBits.
 * The enable/disable control of the tracking will be ignored, and PGM will
 * always set NEM_NOTIFY_PHYS_MMIO_EX_F_TRACK_DIRTY_PAGES for such ranges.
 *
 * @retval  true if supported.
 * @retval  false if not.
 * @param   pVM     The cross context VM structure.
 */
VMMR3_INT_DECL(bool) NEMR3IsMmio2DirtyPageTrackingSupported(PVM pVM);

/**
 * Worker for PGMR3PhysMmio2QueryAndResetDirtyBitmap.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   GCPhys      The address of the MMIO2 range.
 * @param   cb          The size of the MMIO2 range.
 * @param   uNemRange   The NEM internal range number.
 * @param   pvBitmap    The output bitmap.  Must be 8-byte aligned.  Ignored
 *                      when @a cbBitmap is zero.
 * @param   cbBitmap    The size of the bitmap.  Must be the size of the whole
 *                      MMIO2 range, rounded up to the nearest 8 bytes.
 *                      When zero only a reset is done.
 */
VMMR3_INT_DECL(int)  NEMR3PhysMmio2QueryAndResetDirtyBitmap(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, uint32_t uNemRange,
                                                            void *pvBitmap, size_t cbBitmap);

VMMR3_INT_DECL(int)  NEMR3NotifyPhysRamRegister(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, void *pvR3,
                                                uint8_t *pu2State, uint32_t *puNemRange);
VMMR3_INT_DECL(int)  NEMR3NotifyPhysMmioExMapEarly(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, uint32_t fFlags,
                                                   void *pvRam, void *pvMmio2, uint8_t *pu2State, uint32_t *puNemRange);
VMMR3_INT_DECL(int)  NEMR3NotifyPhysMmioExMapLate(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, uint32_t fFlags,
                                                  void *pvRam, void *pvMmio2, uint32_t *puNemRange);
VMMR3_INT_DECL(int)  NEMR3NotifyPhysMmioExUnmap(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, uint32_t fFlags,
                                                void *pvRam, void *pvMmio2, uint8_t *pu2State, uint32_t *puNemRange);
/** @name Flags for NEMR3NotifyPhysMmioExMap and NEMR3NotifyPhysMmioExUnmap.
 * @{ */
/** Set if the range is replacing RAM rather that unused space. */
#define NEM_NOTIFY_PHYS_MMIO_EX_F_REPLACE               RT_BIT(0)
/** Set if it's MMIO2 being mapped or unmapped. */
#define NEM_NOTIFY_PHYS_MMIO_EX_F_MMIO2                 RT_BIT(1)
/** Set if MMIO2 and dirty page tracking is configured. */
#define NEM_NOTIFY_PHYS_MMIO_EX_F_TRACK_DIRTY_PAGES     RT_BIT(2)
/** @} */

/**
 * Called very early during ROM registration, basically so an existing RAM range
 * can be adjusted if desired.
 *
 * It will be succeeded by a number of NEMHCNotifyPhysPageProtChanged()
 * calls and finally a call to NEMR3NotifyPhysRomRegisterLate().
 *
 * @returns VBox status code
 * @param   pVM             The cross context VM structure.
 * @param   GCPhys          The ROM address (page aligned).
 * @param   cb              The size (page aligned).
 * @param   pvPages         Pointer to the ROM (RAM) pages in simplified mode
 *                          when NEM_NOTIFY_PHYS_ROM_F_REPLACE is set, otherwise
 *                          NULL.
 * @param   fFlags          NEM_NOTIFY_PHYS_ROM_F_XXX.
 * @param   pu2State        New page state or UINT8_MAX to leave as-is.
 * @param   puNemRange      Access to the relevant PGMRAMRANGE::uNemRange field.
 */
VMMR3_INT_DECL(int)  NEMR3NotifyPhysRomRegisterEarly(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, void *pvPages,
                                                     uint32_t fFlags, uint8_t *pu2State, uint32_t *puNemRange);

/**
 * Called after the ROM range has been fully completed.
 *
 * This will be preceeded by a NEMR3NotifyPhysRomRegisterEarly() call as well a
 * number of NEMHCNotifyPhysPageProtChanged calls.
 *
 * @returns VBox status code
 * @param   pVM             The cross context VM structure.
 * @param   GCPhys          The ROM address (page aligned).
 * @param   cb              The size (page aligned).
 * @param   pvPages         Pointer to the ROM pages.
 * @param   fFlags          NEM_NOTIFY_PHYS_ROM_F_XXX.
 * @param   pu2State        Where to return the new NEM page state, UINT8_MAX
 *                          for unchanged.
 * @param   puNemRange      Access to the relevant PGMRAMRANGE::uNemRange field.
 */
VMMR3_INT_DECL(int)  NEMR3NotifyPhysRomRegisterLate(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, void *pvPages,
                                                    uint32_t fFlags, uint8_t *pu2State, uint32_t *puNemRange);

#if defined(VBOX_WITH_KVM) && defined(IN_RING3)

/**
 * Asserts a specific interrupt line on both PIC and I/O APIC.
 * @param  pVM The cross context VM structure.
 * @param  u16Gsi the GSI of the interrupt lines that should be asserted.
 * @param  iLevel Line level, either PDM_IRQ_LEVEL_HIGH, PDM_IRQ_LEVEL_LOW or PDM_IRQ_LEVEL_FLIP_FLOP.
 * @return Vbox status code.
 */
VMMR3_INT_DECL(int) NEMR3KvmSetIrqLine(PVM pVM, uint16_t u16Gsi, int iLevel);

/**
 * Execute state load operation. This sets the correct KVM MP state depending on
 * the VBox vCPUs state.
 * @param pVM The cross context VM structure
 */
VMMR3_INT_DECL(int) NEMR3LoadExec(PVM pVM);

/**
 * Retrieves the local APIC state from the in-kernel irqchip.
 * @param pVCpu The vCpu to retrieve the APIC state from
 * @param pXApicPage Pointer to the memory the APIC state is saved to. Must be
 *                   at least of size KVM_APIC_REG_SIZE.
 * @returns VBox status code
 */
VMMR3_INT_DECL(int) NEMR3KvmGetLapicState(PVMCPU pVCpu, void* pXApicPage);

/**
 * Configures the local APIC state of the in-kernel irqchip.
 * @param pVCpu The vCpu for which to set the APIC state
 * @param pXApicPage Pointer to the memory containing APIC state. Must be at
 *                   least of size KVM_APIC_REG_SIZE.
 * @returns VBox status code
 */
VMMR3_INT_DECL(int) NEMR3KvmSetLapicState(PVMCPU pVCpu, void* pXApicPage);

#ifdef VBOX_WITH_KVM_IRQCHIP_FULL

/**
 * Retrieves the PIC state from the in-kernel irqchip.
 * @param pVM The VM to retrieve the PIC state from
 * @param irqchip Whether to retrieve the state from the master or slave pic
 * @param state Buffer to store the PIC state in.
 * @returns VBox status code
 */
VMMR3_INT_DECL(int) NEMR3KvmGetPicState(PVM pVM, KVMIRQCHIP irqchip, KVMPICSTATE* state);

/**
 * Configures the PIC state of the in-kernel irqchip.
 * @param pVM The VM to for which to set the PIC state
 * @param irqchip Whether to set the state of the master or slave pic
 * @param state Pointer to the memory containing PIC state.
 * @returns VBox status code
 */
VMMR3_INT_DECL(int) NEMR3KvmSetPicState(PVM pVM, KVMIRQCHIP irqchip, KVMPICSTATE* state);

/**
 * Retrieves the I/O APIC state from the in-kernel irqchip.
 * @param pVM The VM to retrieve the I/O APIC state from
 * @param state Buffer where to store I/O APIC state.
 * @returns VBox status code
 */
VMMR3_INT_DECL(int) NEMR3KvmGetIoApicState(PVM pVM, KVMIOAPICSTATE* state);

/**
 * Configures the I/O APIC state of the in-kernel irqchip.
 * @param pVM The VM to for which to set the I/O APIC state
 * @param state Pointer to the memory containing I/O APIC state.
 * @returns VBox status code
 */
VMMR3_INT_DECL(int) NEMR3KvmSetIoApicState(PVM pVM, KVMIOAPICSTATE* state);
#endif
/**
 * Deliver a MSI via the in-kernel irqchip.
 *
 * @returns VBox status code
 * @param pVM The cross context VM structure
 * @param pMsi The MSI to inject into the guest
 */
VMMR3_INT_DECL(int) NEMR3KvmSplitIrqchipDeliverMsi(PVM pVM, PCMSIMSG pMsi);

/**
 * Add or update the Entry in the Redirection Table indexed by the GSI number.
 *
 * Interrupts configured via this interface will cause an EOI exit when the
 * guest acknowledges them. Typically, this is only necessary for level
 * triggered interrupts.
 *
 * @returns VBox status code
 * @param pVM The cross context VM structure
 * @param gsi The GSI number
 * @param pMSI The MSI that should be delivered when the interrupt fires
 */
VMMR3_INT_DECL(int) NEMR3KvmSplitIrqchipAddUpdateRTE(PVM pVM, uint16_t u16Gsi, PCMSIMSG pMsi);

/**
 *  Remove an Redirection Table entry indexed by the GSI number
 *
 *  @returns VBox status code
 *  @param pVM The cross context VM structure
 *  @param gsi The GSI number for what the Redirection Table Entry should be
 *  removed
 */
VMMR3_INT_DECL(int) NEMR3KvmSplitIrqchipRemoveRTE(PVM pVM, uint16_t u16Gsi);

/**
 * Returns an array of Hyper-V CPUID leaves supported by KVM.
 *
 * @returns VBox status code
 * @param pVM The cross context VM structure
 * @param outpCpuId The pointer where the CPUID leaves will be returned. Must be freed by the caller!
 * @param outcLeaves The pointer where the number of CPUID leaves will be returned.
 */
VMMR3_INT_DECL(int) NEMR3KvmGetHvCpuIdLeaves(PVM pVM, PCPUMCPUIDLEAF *outpCpuId, size_t *outcLeaves);

/**
 * Returns an array of CPUID leaves supported by KVM.
 *
 * @returns VBox status code
 * @param pVM The cross context VM structure
 * @param outpCpuId The pointer where the CPUID leaves will be returned. Must be freed by the caller!
 * @param outcLeaves The pointer where the number of CPUID leaves will be returned.
 */
VMMR3_INT_DECL(int) NEMR3KvmGetCpuIdLeaves(PVM pVM, PCPUMCPUIDLEAF *outpCpuId, size_t *outcLeaves);
#endif

#if defined(VBOX_WITH_KVM) && defined(IN_RING3)

#define KVM_SPLIT_IRQCHIP_NUM_INTR_PINS 24

/**
 * Execute state load operation. This sets the correct KVM MP state depending on
 * the VBox vCPUs state.
 * @param pVM The cross context VM structure
 */
VMMR3_INT_DECL(int) NEMR3LoadExec(PVM pVM);

/**
 * Retrieves the local APIC state from the in-kernel irqchip.
 * @param pVCpu The vCpu to retrieve the APIC state from
 * @param pXApicPage Pointer to the memory the APIC state is saved to. Must be
 *                   at least of size KVM_APIC_REG_SIZE.
 * @returns VBox status code
 */
VMMR3_INT_DECL(int) NEMR3KvmSplitIrqchipGetApicState(PVMCPU pVCpu, void* pXApicPage);

/**
 * Configures the local APIC state of the in-kernel irqchip.
 * @param pVCpu The vCpu for which to set the APIC state
 * @param pXApicPage Pointer to the memory containing APIC state. Must be at
 *                   least of size KVM_APIC_REG_SIZE.
 * @returns VBox status code
 */
VMMR3_INT_DECL(int) NEMR3KvmSplitIrqchipSetApicState(PVMCPU pVCpu, void* pXApicPage);

/**
 * Deliver a MSI via the in-kernel irqchip.
 *
 * @returns VBox status code
 * @param pVM The cross context VM structure
 * @param pMsi The MSI to inject into the guest
 */
VMMR3_INT_DECL(int) NEMR3KvmSplitIrqchipDeliverMsi(PVM pVM, PCMSIMSG pMsi);

/**
 * Add or update the Entry in the Redirection Table indexed by the GSI number.
 *
 * Interrupts configured via this interface will cause an EOI exit when the
 * guest acknowledges them. Typically, this is only necessary for level
 * triggered interrupts.
 *
 * @returns VBox status code
 * @param pVM The cross context VM structure
 * @param gsi The GSI number
 * @param pMSI The MSI that should be delivered when the interrupt fires
 */
VMMR3_INT_DECL(int) NEMR3KvmSplitIrqchipAddUpdateRTE(PVM pVM, uint16_t u16Gsi, PCMSIMSG pMsi);

/**
 *  Remove an Redirection Table entry indexed by the GSI number
 *
 *  @returns VBox status code
 *  @param pVM The cross context VM structure
 *  @param gsi The GSI number for what the Redirection Table Entry should be
 *  removed
 */
VMMR3_INT_DECL(int) NEMR3KvmSplitIrqchipRemoveRTE(PVM pVM, uint16_t u16Gsi);
#endif

/** @name Flags for NEMR3NotifyPhysRomRegisterEarly and NEMR3NotifyPhysRomRegisterLate.
 * @{ */
/** Set if the range is replacing RAM rather that unused space. */
#define NEM_NOTIFY_PHYS_ROM_F_REPLACE       RT_BIT(1)
/** Set if it's MMIO2 being mapped or unmapped. */
#define NEM_NOTIFY_PHYS_ROM_F_SHADOW        RT_BIT(2)
/** @} */

/**
 * Called when the A20 state changes.
 *
 * Windows: Hyper-V doesn't seem to offer a simple way of implementing the A20
 * line features of PCs.  So, we do a very minimal emulation of the HMA to make
 * DOS happy.
 *
 * @param   pVCpu           The CPU the A20 state changed on.
 * @param   fEnabled        Whether it was enabled (true) or disabled.
 */
VMMR3_INT_DECL(void) NEMR3NotifySetA20(PVMCPU pVCpu, bool fEnabled);
VMMR3_INT_DECL(void) NEMR3NotifyDebugEventChanged(PVM pVM);
VMMR3_INT_DECL(void) NEMR3NotifyDebugEventChangedPerCpu(PVM pVM, PVMCPU pVCpu);
/** @} */


/** @defgroup grp_nem_r0    The NEM ring-0 Context API
 * @{  */
VMMR0_INT_DECL(int)  NEMR0Init(void);
VMMR0_INT_DECL(void) NEMR0Term(void);
VMMR0_INT_DECL(int)  NEMR0InitVM(PGVM pGVM);
VMMR0_INT_DECL(int)  NEMR0InitVMPart2(PGVM pGVM);
VMMR0_INT_DECL(void) NEMR0CleanupVM(PGVM pGVM);
VMMR0_INT_DECL(int)  NEMR0MapPages(PGVM pGVM, VMCPUID idCpu);
VMMR0_INT_DECL(int)  NEMR0UnmapPages(PGVM pGVM, VMCPUID idCpu);
VMMR0_INT_DECL(int)  NEMR0ExportState(PGVM pGVM, VMCPUID idCpu);
VMMR0_INT_DECL(int)  NEMR0ImportState(PGVM pGVM, VMCPUID idCpu, uint64_t fWhat);
VMMR0_INT_DECL(int)  NEMR0QueryCpuTick(PGVM pGVM, VMCPUID idCpu);
VMMR0_INT_DECL(int)  NEMR0ResumeCpuTickOnAll(PGVM pGVM, VMCPUID idCpu, uint64_t uPausedTscValue);
VMMR0_INT_DECL(VBOXSTRICTRC) NEMR0RunGuestCode(PGVM pGVM, VMCPUID idCpu);
VMMR0_INT_DECL(int)  NEMR0UpdateStatistics(PGVM pGVM, VMCPUID idCpu);
VMMR0_INT_DECL(int)  NEMR0DoExperiment(PGVM pGVM, VMCPUID idCpu, uint64_t u64Arg);
#ifdef RT_OS_WINDOWS
VMMR0_INT_DECL(int)  NEMR0WinGetPartitionId(PGVM pGVM, uintptr_t uHandle);
#endif
/** @} */


/** @defgroup grp_nem_hc    The NEM Host Context API
 * @{
 */
VMM_INT_DECL(bool) NEMHCIsLongModeAllowed(PVMCC pVM);
VMM_INT_DECL(uint32_t) NEMHCGetFeatures(PVMCC pVM);
VMM_INT_DECL(int)  NEMImportStateOnDemand(PVMCPUCC pVCpu, uint64_t fWhat);

/** @name NEM_FEAT_F_XXX - Features supported by the NEM backend
 * @{ */
/** NEM backend uses nested paging for the guest. */
#define NEM_FEAT_F_NESTED_PAGING    RT_BIT(0)
/** NEM backend uses full (unrestricted) guest execution. */
#define NEM_FEAT_F_FULL_GST_EXEC    RT_BIT(1)
/** NEM backend offers an xsave/xrstor interface. */
#define NEM_FEAT_F_XSAVE_XRSTOR     RT_BIT(2)
/** @} */

VMM_INT_DECL(void) NEMHCNotifyHandlerPhysicalRegister(PVMCC pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb);
VMM_INT_DECL(void) NEMHCNotifyHandlerPhysicalDeregister(PVMCC pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb,
                                                        RTR3PTR pvMemR3, uint8_t *pu2State);
VMM_INT_DECL(void) NEMHCNotifyHandlerPhysicalModify(PVMCC pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhysOld,
                                                    RTGCPHYS GCPhysNew, RTGCPHYS cb, bool fRestoreAsRAM);

VMM_INT_DECL(int)  NEMHCNotifyPhysPageAllocated(PVMCC pVM, RTGCPHYS GCPhys, RTHCPHYS HCPhys, uint32_t fPageProt,
                                                PGMPAGETYPE enmType, uint8_t *pu2State);
VMM_INT_DECL(void) NEMHCNotifyPhysPageProtChanged(PVMCC pVM, RTGCPHYS GCPhys, RTHCPHYS HCPhys, RTR3PTR pvR3, uint32_t fPageProt,
                                                  PGMPAGETYPE enmType, uint8_t *pu2State);
VMM_INT_DECL(void) NEMHCNotifyPhysPageChanged(PVMCC pVM, RTGCPHYS GCPhys, RTHCPHYS HCPhysPrev, RTHCPHYS HCPhysNew,
                                              RTR3PTR pvNewR3, uint32_t fPageProt, PGMPAGETYPE enmType, uint8_t *pu2State);
/** @name NEM_PAGE_PROT_XXX - Page protection
 * @{ */
#define NEM_PAGE_PROT_NONE      UINT32_C(0)     /**< All access causes VM exits. */
#define NEM_PAGE_PROT_READ      RT_BIT(0)       /**< Read access. */
#define NEM_PAGE_PROT_EXECUTE   RT_BIT(1)       /**< Execute access. */
#define NEM_PAGE_PROT_WRITE     RT_BIT(2)       /**< write access. */
/** @} */

VMM_INT_DECL(int) NEMHCQueryCpuTick(PVMCPUCC pVCpu, uint64_t *pcTicks, uint32_t *puAux);
VMM_INT_DECL(int) NEMHCResumeCpuTickOnAll(PVMCC pVM, PVMCPUCC pVCpu, uint64_t uPausedTscValue);

/** @} */

/** @} */
RT_C_DECLS_END


#endif /* !VBOX_INCLUDED_vmm_nem_h */

