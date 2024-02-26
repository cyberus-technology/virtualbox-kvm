/* $Id: NEMR3Native-linux.cpp $ */
/** @file
 * NEM - Native execution manager, native ring-3 Linux backend.
 */

/*
 * Copyright (C) 2021-2023 Oracle and/or its affiliates.
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
#define LOG_GROUP LOG_GROUP_NEM
#define VMCPU_INCL_CPUM_GST_CTX
#include <VBox/vmm/nem.h>
#include <VBox/vmm/iem.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/apic.h>
#include <VBox/vmm/pdm.h>
#include <VBox/vmm/trpm.h>
#include "NEMInternal.h"
#include "HMInternal.h"
#include "GIMInternal.h"
#include "GIMHvInternal.h"
#include <VBox/vmm/vmcc.h>

#include <iprt/alloca.h>
#include <iprt/mem.h>
#include <iprt/string.h>
#include <iprt/system.h>
#include <iprt/x86.h>

#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <linux/kvm.h>

#include <algorithm>
#include <vector>

/*
 * Supply stuff missing from the kvm.h on the build box.
 */
#ifndef KVM_INTERNAL_ERROR_UNEXPECTED_EXIT_REASON /* since 5.4 */
# define KVM_INTERNAL_ERROR_UNEXPECTED_EXIT_REASON 4
#endif

/**
 * The MMIO address of the TPR register of the LAPIC.
 */
static constexpr uint64_t XAPIC_TPR_ADDR {0xfee00080};

/**
 * The class priority shift for the TPR register.
 */
static constexpr uint64_t LAPIC_TPR_SHIFT {4};

#ifdef VBOX_WITH_KVM_IRQCHIP_FULL
static int kvmSetGsiRoutingFullIrqChip(PVM pVM);
#endif

/**
 * Worker for nemR3NativeInit that gets the hypervisor capabilities.
 *
 * @returns VBox status code.
 * @param   pVM                 The cross context VM structure.
 * @param   pErrInfo            Where to always return error info.
 */
static int nemR3LnxInitCheckCapabilities(PVM pVM, PRTERRINFO pErrInfo)
{
    AssertReturn(pVM->nem.s.fdVm != -1, RTErrInfoSet(pErrInfo, VERR_WRONG_ORDER, "Wrong initalization order"));

    /*
     * Capabilities.
     */
    static const struct
    {
        const char *pszName;
        int         iCap;
        uint32_t    offNem      : 24;
        uint32_t    cbNem       : 3;
        uint32_t    fReqNonZero : 1;
        uint32_t    uReserved   : 4;
    } s_aCaps[] =
    {
#define CAP_ENTRY__L(a_Define)           { #a_Define, a_Define,            UINT32_C(0x00ffffff), 0, 0, 0 }
#define CAP_ENTRY__S(a_Define, a_Member) { #a_Define, a_Define, RT_UOFFSETOF(NEM, a_Member), RT_SIZEOFMEMB(NEM, a_Member), 0, 0 }
#define CAP_ENTRY_MS(a_Define, a_Member) { #a_Define, a_Define, RT_UOFFSETOF(NEM, a_Member), RT_SIZEOFMEMB(NEM, a_Member), 1, 0 }
#define CAP_ENTRY__U(a_Number)           { "KVM_CAP_" #a_Number, a_Number, UINT32_C(0x00ffffff), 0, 0, 0 }
#define CAP_ENTRY_ML(a_Number)           { "KVM_CAP_" #a_Number, a_Number, UINT32_C(0x00ffffff), 0, 1, 0 }

        CAP_ENTRY__L(KVM_CAP_IRQCHIP),                       /* 0 */
        CAP_ENTRY_ML(KVM_CAP_HLT),
        CAP_ENTRY__L(KVM_CAP_MMU_SHADOW_CACHE_CONTROL),
        CAP_ENTRY_ML(KVM_CAP_USER_MEMORY),
        CAP_ENTRY__L(KVM_CAP_SET_TSS_ADDR),
        CAP_ENTRY__U(5),
        CAP_ENTRY__L(KVM_CAP_VAPIC),
        CAP_ENTRY__L(KVM_CAP_EXT_CPUID),
        CAP_ENTRY__L(KVM_CAP_CLOCKSOURCE),
        CAP_ENTRY__L(KVM_CAP_NR_VCPUS),
        CAP_ENTRY_MS(KVM_CAP_NR_MEMSLOTS, cMaxMemSlots),     /* 10 */
        CAP_ENTRY__L(KVM_CAP_PIT),
        CAP_ENTRY__L(KVM_CAP_NOP_IO_DELAY),
        CAP_ENTRY__L(KVM_CAP_PV_MMU),
        CAP_ENTRY__L(KVM_CAP_MP_STATE),
        CAP_ENTRY__L(KVM_CAP_COALESCED_MMIO),
        CAP_ENTRY__L(KVM_CAP_SYNC_MMU),
        CAP_ENTRY__U(17),
        CAP_ENTRY__L(KVM_CAP_IOMMU),
        CAP_ENTRY__U(19), /* Buggy KVM_CAP_JOIN_MEMORY_REGIONS? */
        CAP_ENTRY__U(20), /* Mon-working KVM_CAP_DESTROY_MEMORY_REGION? */
        CAP_ENTRY__L(KVM_CAP_DESTROY_MEMORY_REGION_WORKS),   /* 21 */
        CAP_ENTRY__L(KVM_CAP_USER_NMI),
#ifdef __KVM_HAVE_GUEST_DEBUG
        CAP_ENTRY__L(KVM_CAP_SET_GUEST_DEBUG),
#endif
#ifdef __KVM_HAVE_PIT
        CAP_ENTRY__L(KVM_CAP_REINJECT_CONTROL),
#endif
        CAP_ENTRY__L(KVM_CAP_IRQ_ROUTING),
        CAP_ENTRY__L(KVM_CAP_IRQ_INJECT_STATUS),
        CAP_ENTRY__U(27),
        CAP_ENTRY__U(28),
        CAP_ENTRY__L(KVM_CAP_ASSIGN_DEV_IRQ),
        CAP_ENTRY__L(KVM_CAP_JOIN_MEMORY_REGIONS_WORKS),     /* 30 */
#ifdef __KVM_HAVE_MCE
        CAP_ENTRY__L(KVM_CAP_MCE),
#endif
        CAP_ENTRY__L(KVM_CAP_IRQFD),
#ifdef __KVM_HAVE_PIT
        CAP_ENTRY__L(KVM_CAP_PIT2),
#endif
        CAP_ENTRY__L(KVM_CAP_SET_BOOT_CPU_ID),
#ifdef __KVM_HAVE_PIT_STATE2
        CAP_ENTRY__L(KVM_CAP_PIT_STATE2),
#endif
        CAP_ENTRY__L(KVM_CAP_IOEVENTFD),
        CAP_ENTRY__L(KVM_CAP_SET_IDENTITY_MAP_ADDR),
#ifdef __KVM_HAVE_XEN_HVM
        CAP_ENTRY__L(KVM_CAP_XEN_HVM),
#endif
        CAP_ENTRY_ML(KVM_CAP_ADJUST_CLOCK),
        CAP_ENTRY__L(KVM_CAP_INTERNAL_ERROR_DATA),           /* 40 */
#ifdef __KVM_HAVE_VCPU_EVENTS
        CAP_ENTRY_ML(KVM_CAP_VCPU_EVENTS),
#else
        CAP_ENTRY_MU(41),
#endif
        CAP_ENTRY__L(KVM_CAP_S390_PSW),
        CAP_ENTRY__L(KVM_CAP_PPC_SEGSTATE),
        CAP_ENTRY__L(KVM_CAP_HYPERV),
        CAP_ENTRY__L(KVM_CAP_HYPERV_VAPIC),
        CAP_ENTRY__L(KVM_CAP_HYPERV_SPIN),
        CAP_ENTRY__L(KVM_CAP_PCI_SEGMENT),
        CAP_ENTRY__L(KVM_CAP_PPC_PAIRED_SINGLES),
        CAP_ENTRY__L(KVM_CAP_INTR_SHADOW),
#ifdef __KVM_HAVE_DEBUGREGS
        CAP_ENTRY__L(KVM_CAP_DEBUGREGS),                     /* 50 */
#endif
        CAP_ENTRY__S(KVM_CAP_X86_ROBUST_SINGLESTEP, fRobustSingleStep),
        CAP_ENTRY__L(KVM_CAP_PPC_OSI),
        CAP_ENTRY__L(KVM_CAP_PPC_UNSET_IRQ),
        CAP_ENTRY__L(KVM_CAP_ENABLE_CAP),
#ifdef __KVM_HAVE_XSAVE
        CAP_ENTRY_ML(KVM_CAP_XSAVE),
#else
        CAP_ENTRY_MU(55),
#endif
#ifdef __KVM_HAVE_XCRS
        CAP_ENTRY_ML(KVM_CAP_XCRS),
#else
        CAP_ENTRY_MU(56),
#endif
        CAP_ENTRY__L(KVM_CAP_PPC_GET_PVINFO),
        CAP_ENTRY__L(KVM_CAP_PPC_IRQ_LEVEL),
        CAP_ENTRY__L(KVM_CAP_ASYNC_PF),
        CAP_ENTRY__L(KVM_CAP_TSC_CONTROL),                   /* 60 */
        CAP_ENTRY__L(KVM_CAP_GET_TSC_KHZ),
        CAP_ENTRY__L(KVM_CAP_PPC_BOOKE_SREGS),
        CAP_ENTRY__L(KVM_CAP_SPAPR_TCE),
        CAP_ENTRY__L(KVM_CAP_PPC_SMT),
        CAP_ENTRY__L(KVM_CAP_PPC_RMA),
        CAP_ENTRY__L(KVM_CAP_MAX_VCPUS),
        CAP_ENTRY__L(KVM_CAP_PPC_HIOR),
        CAP_ENTRY__L(KVM_CAP_PPC_PAPR),
        CAP_ENTRY__L(KVM_CAP_SW_TLB),
        CAP_ENTRY__L(KVM_CAP_ONE_REG),                       /* 70 */
        CAP_ENTRY__L(KVM_CAP_S390_GMAP),
        CAP_ENTRY__L(KVM_CAP_TSC_DEADLINE_TIMER),
        CAP_ENTRY__L(KVM_CAP_S390_UCONTROL),
        CAP_ENTRY__L(KVM_CAP_SYNC_REGS),
        CAP_ENTRY__L(KVM_CAP_PCI_2_3),
        CAP_ENTRY__L(KVM_CAP_KVMCLOCK_CTRL),
        CAP_ENTRY__L(KVM_CAP_SIGNAL_MSI),
        CAP_ENTRY__L(KVM_CAP_PPC_GET_SMMU_INFO),
        CAP_ENTRY__L(KVM_CAP_S390_COW),
        CAP_ENTRY__L(KVM_CAP_PPC_ALLOC_HTAB),                /* 80 */
        CAP_ENTRY__L(KVM_CAP_READONLY_MEM),
        CAP_ENTRY__L(KVM_CAP_IRQFD_RESAMPLE),
        CAP_ENTRY__L(KVM_CAP_PPC_BOOKE_WATCHDOG),
        CAP_ENTRY__L(KVM_CAP_PPC_HTAB_FD),
        CAP_ENTRY__L(KVM_CAP_S390_CSS_SUPPORT),
        CAP_ENTRY__L(KVM_CAP_PPC_EPR),
        CAP_ENTRY__L(KVM_CAP_ARM_PSCI),
        CAP_ENTRY__L(KVM_CAP_ARM_SET_DEVICE_ADDR),
        CAP_ENTRY__L(KVM_CAP_DEVICE_CTRL),
        CAP_ENTRY__L(KVM_CAP_IRQ_MPIC),                      /* 90 */
        CAP_ENTRY__L(KVM_CAP_PPC_RTAS),
        CAP_ENTRY__L(KVM_CAP_IRQ_XICS),
        CAP_ENTRY__L(KVM_CAP_ARM_EL1_32BIT),
        CAP_ENTRY__L(KVM_CAP_SPAPR_MULTITCE),
        CAP_ENTRY__L(KVM_CAP_EXT_EMUL_CPUID),
        CAP_ENTRY__L(KVM_CAP_HYPERV_TIME),
        CAP_ENTRY__L(KVM_CAP_IOAPIC_POLARITY_IGNORED),
        CAP_ENTRY__L(KVM_CAP_ENABLE_CAP_VM),
        CAP_ENTRY__L(KVM_CAP_S390_IRQCHIP),
        CAP_ENTRY__L(KVM_CAP_IOEVENTFD_NO_LENGTH),           /* 100 */
        CAP_ENTRY__L(KVM_CAP_VM_ATTRIBUTES),
        CAP_ENTRY__L(KVM_CAP_ARM_PSCI_0_2),
        CAP_ENTRY__L(KVM_CAP_PPC_FIXUP_HCALL),
        CAP_ENTRY__L(KVM_CAP_PPC_ENABLE_HCALL),
        CAP_ENTRY__L(KVM_CAP_CHECK_EXTENSION_VM),
        CAP_ENTRY__L(KVM_CAP_S390_USER_SIGP),
        CAP_ENTRY__L(KVM_CAP_S390_VECTOR_REGISTERS),
        CAP_ENTRY__L(KVM_CAP_S390_MEM_OP),
        CAP_ENTRY__L(KVM_CAP_S390_USER_STSI),
        CAP_ENTRY__L(KVM_CAP_S390_SKEYS),                    /* 110 */
        CAP_ENTRY__L(KVM_CAP_MIPS_FPU),
        CAP_ENTRY__L(KVM_CAP_MIPS_MSA),
        CAP_ENTRY__L(KVM_CAP_S390_INJECT_IRQ),
        CAP_ENTRY__L(KVM_CAP_S390_IRQ_STATE),
        CAP_ENTRY__L(KVM_CAP_PPC_HWRNG),
        CAP_ENTRY__L(KVM_CAP_DISABLE_QUIRKS),
        CAP_ENTRY__L(KVM_CAP_X86_SMM),
        CAP_ENTRY__L(KVM_CAP_MULTI_ADDRESS_SPACE),
        CAP_ENTRY__L(KVM_CAP_GUEST_DEBUG_HW_BPS),
        CAP_ENTRY__L(KVM_CAP_GUEST_DEBUG_HW_WPS),            /* 120 */
        CAP_ENTRY__L(KVM_CAP_SPLIT_IRQCHIP),
        CAP_ENTRY__L(KVM_CAP_IOEVENTFD_ANY_LENGTH),
        CAP_ENTRY__L(KVM_CAP_HYPERV_SYNIC),
        CAP_ENTRY__L(KVM_CAP_S390_RI),
        CAP_ENTRY__L(KVM_CAP_SPAPR_TCE_64),
        CAP_ENTRY__L(KVM_CAP_ARM_PMU_V3),
        CAP_ENTRY__L(KVM_CAP_VCPU_ATTRIBUTES),
        CAP_ENTRY__L(KVM_CAP_MAX_VCPU_ID),
        CAP_ENTRY__L(KVM_CAP_X2APIC_API),
        CAP_ENTRY__L(KVM_CAP_S390_USER_INSTR0),              /* 130 */
        CAP_ENTRY__L(KVM_CAP_MSI_DEVID),
        CAP_ENTRY__L(KVM_CAP_PPC_HTM),
        CAP_ENTRY__L(KVM_CAP_SPAPR_RESIZE_HPT),
        CAP_ENTRY__L(KVM_CAP_PPC_MMU_RADIX),
        CAP_ENTRY__L(KVM_CAP_PPC_MMU_HASH_V3),
        CAP_ENTRY__L(KVM_CAP_IMMEDIATE_EXIT),
        CAP_ENTRY__L(KVM_CAP_MIPS_VZ),
        CAP_ENTRY__L(KVM_CAP_MIPS_TE),
        CAP_ENTRY__L(KVM_CAP_MIPS_64BIT),
        CAP_ENTRY__L(KVM_CAP_S390_GS),                       /* 140 */
        CAP_ENTRY__L(KVM_CAP_S390_AIS),
        CAP_ENTRY__L(KVM_CAP_SPAPR_TCE_VFIO),
        CAP_ENTRY__L(KVM_CAP_X86_DISABLE_EXITS),
        CAP_ENTRY__L(KVM_CAP_ARM_USER_IRQ),
        CAP_ENTRY__L(KVM_CAP_S390_CMMA_MIGRATION),
        CAP_ENTRY__L(KVM_CAP_PPC_FWNMI),
        CAP_ENTRY__L(KVM_CAP_PPC_SMT_POSSIBLE),
        CAP_ENTRY__L(KVM_CAP_HYPERV_SYNIC2),
        CAP_ENTRY__L(KVM_CAP_HYPERV_VP_INDEX),
        CAP_ENTRY__L(KVM_CAP_S390_AIS_MIGRATION),            /* 150 */
        CAP_ENTRY__L(KVM_CAP_PPC_GET_CPU_CHAR),
        CAP_ENTRY__L(KVM_CAP_S390_BPB),
        CAP_ENTRY__L(KVM_CAP_GET_MSR_FEATURES),
        CAP_ENTRY__L(KVM_CAP_HYPERV_EVENTFD),
        CAP_ENTRY__L(KVM_CAP_HYPERV_TLBFLUSH),
        CAP_ENTRY__L(KVM_CAP_S390_HPAGE_1M),
        CAP_ENTRY__L(KVM_CAP_NESTED_STATE),
        CAP_ENTRY__L(KVM_CAP_ARM_INJECT_SERROR_ESR),
        CAP_ENTRY__L(KVM_CAP_MSR_PLATFORM_INFO),
        CAP_ENTRY__L(KVM_CAP_PPC_NESTED_HV),                 /* 160 */
        CAP_ENTRY__L(KVM_CAP_HYPERV_SEND_IPI),
        CAP_ENTRY__L(KVM_CAP_COALESCED_PIO),
        CAP_ENTRY__L(KVM_CAP_HYPERV_ENLIGHTENED_VMCS),
        CAP_ENTRY__L(KVM_CAP_EXCEPTION_PAYLOAD),
        CAP_ENTRY__L(KVM_CAP_ARM_VM_IPA_SIZE),
        CAP_ENTRY__L(KVM_CAP_MANUAL_DIRTY_LOG_PROTECT),
        CAP_ENTRY__L(KVM_CAP_HYPERV_CPUID),
        CAP_ENTRY__L(KVM_CAP_MANUAL_DIRTY_LOG_PROTECT2),
        CAP_ENTRY__L(KVM_CAP_PPC_IRQ_XIVE),
        CAP_ENTRY__L(KVM_CAP_ARM_SVE),                       /* 170 */
        CAP_ENTRY__L(KVM_CAP_ARM_PTRAUTH_ADDRESS),
        CAP_ENTRY__L(KVM_CAP_ARM_PTRAUTH_GENERIC),
        CAP_ENTRY__L(KVM_CAP_PMU_EVENT_FILTER),
        CAP_ENTRY__L(KVM_CAP_ARM_IRQ_LINE_LAYOUT_2),
        CAP_ENTRY__L(KVM_CAP_HYPERV_DIRECT_TLBFLUSH),
        CAP_ENTRY__L(KVM_CAP_PPC_GUEST_DEBUG_SSTEP),
        CAP_ENTRY__L(KVM_CAP_ARM_NISV_TO_USER),
        CAP_ENTRY__L(KVM_CAP_ARM_INJECT_EXT_DABT),
        CAP_ENTRY__L(KVM_CAP_S390_VCPU_RESETS),
        CAP_ENTRY__L(KVM_CAP_S390_PROTECTED),                /* 180 */
        CAP_ENTRY__L(KVM_CAP_PPC_SECURE_GUEST),
        CAP_ENTRY__L(KVM_CAP_HALT_POLL),
        CAP_ENTRY__L(KVM_CAP_ASYNC_PF_INT),
        CAP_ENTRY__L(KVM_CAP_LAST_CPU),
        CAP_ENTRY__L(KVM_CAP_SMALLER_MAXPHYADDR),
        CAP_ENTRY__L(KVM_CAP_S390_DIAG318),
        CAP_ENTRY__L(KVM_CAP_STEAL_TIME),
        CAP_ENTRY_ML(KVM_CAP_X86_USER_SPACE_MSR),            /* (since 5.10) */
        CAP_ENTRY_ML(KVM_CAP_X86_MSR_FILTER),
        CAP_ENTRY__L(KVM_CAP_ENFORCE_PV_FEATURE_CPUID),      /* 190 */
        CAP_ENTRY__L(KVM_CAP_SYS_HYPERV_CPUID),
        CAP_ENTRY__L(KVM_CAP_DIRTY_LOG_RING),
        CAP_ENTRY__L(KVM_CAP_X86_BUS_LOCK_EXIT),
        CAP_ENTRY__L(KVM_CAP_PPC_DAWR1),
        CAP_ENTRY__L(KVM_CAP_SET_GUEST_DEBUG2),
        CAP_ENTRY__L(KVM_CAP_SGX_ATTRIBUTE),
        CAP_ENTRY__L(KVM_CAP_VM_COPY_ENC_CONTEXT_FROM),
        CAP_ENTRY__L(KVM_CAP_PTP_KVM),
        CAP_ENTRY__U(199),
        CAP_ENTRY__U(200),
        CAP_ENTRY__U(201),
        CAP_ENTRY__U(202),
        CAP_ENTRY__U(203),
        CAP_ENTRY__U(204),
        CAP_ENTRY__U(205),
        CAP_ENTRY__U(206),
        CAP_ENTRY__U(207),
        CAP_ENTRY__U(208),
        CAP_ENTRY__U(209),
        CAP_ENTRY__U(210),
        CAP_ENTRY__U(211),
        CAP_ENTRY__U(212),
        CAP_ENTRY__U(213),
        CAP_ENTRY__U(214),
        CAP_ENTRY__U(215),
        CAP_ENTRY__U(216),
    };

    LogRel(("NEM: KVM capabilities (system):\n"));
    int rcRet = VINF_SUCCESS;
    for (unsigned i = 0; i < RT_ELEMENTS(s_aCaps); i++)
    {
        int rc = ioctl(pVM->nem.s.fdVm, KVM_CHECK_EXTENSION, s_aCaps[i].iCap);
        if (rc >= 10)
            LogRel(("NEM:   %36s: %#x (%d)\n", s_aCaps[i].pszName, rc, rc));
        else if (rc >= 0)
            LogRel(("NEM:   %36s: %d\n", s_aCaps[i].pszName, rc));
        else
            LogRel(("NEM:   %s failed: %d/%d\n", s_aCaps[i].pszName, rc, errno));
        switch (s_aCaps[i].cbNem)
        {
            case 0:
                break;
            case 1:
            {
                uint8_t *puValue = (uint8_t *)&pVM->nem.padding[s_aCaps[i].offNem];
                AssertReturn(s_aCaps[i].offNem <= sizeof(NEM) - sizeof(*puValue), VERR_NEM_IPE_0);
                *puValue = (uint8_t)rc;
                AssertLogRelMsg((int)*puValue == rc, ("%s: %#x\n", s_aCaps[i].pszName, rc));
                break;
            }
            case 2:
            {
                uint16_t *puValue = (uint16_t *)&pVM->nem.padding[s_aCaps[i].offNem];
                AssertReturn(s_aCaps[i].offNem <= sizeof(NEM) - sizeof(*puValue), VERR_NEM_IPE_0);
                *puValue = (uint16_t)rc;
                AssertLogRelMsg((int)*puValue == rc, ("%s: %#x\n", s_aCaps[i].pszName, rc));
                break;
            }
            case 4:
            {
                uint32_t *puValue = (uint32_t *)&pVM->nem.padding[s_aCaps[i].offNem];
                AssertReturn(s_aCaps[i].offNem <= sizeof(NEM) - sizeof(*puValue), VERR_NEM_IPE_0);
                *puValue = (uint32_t)rc;
                AssertLogRelMsg((int)*puValue == rc, ("%s: %#x\n", s_aCaps[i].pszName, rc));
                break;
            }
            default:
                rcRet = RTErrInfoSetF(pErrInfo, VERR_NEM_IPE_0, "s_aCaps[%u] is bad: cbNem=%#x - %s",
                                      i, s_aCaps[i].pszName, s_aCaps[i].cbNem);
                AssertFailedReturn(rcRet);
        }

        /*
         * Is a require non-zero entry zero or failing?
         */
        if (s_aCaps[i].fReqNonZero && rc <= 0)
            rcRet = RTERRINFO_LOG_REL_ADD_F(pErrInfo, VERR_NEM_MISSING_FEATURE,
                                            "Required capability '%s' is missing!", s_aCaps[i].pszName);
    }

    /*
     * Get per VCpu KVM_RUN MMAP area size.
     */
    int rc = ioctl(pVM->nem.s.fdKvm, KVM_GET_VCPU_MMAP_SIZE, 0UL);
    if ((unsigned)rc < _64M)
    {
        pVM->nem.s.cbVCpuMmap = (uint32_t)rc;
        LogRel(("NEM:   %36s: %#x (%d)\n", "KVM_GET_VCPU_MMAP_SIZE", rc, rc));
    }
    else if (rc < 0)
        rcRet = RTERRINFO_LOG_REL_ADD_F(pErrInfo, VERR_NEM_MISSING_FEATURE, "KVM_GET_VCPU_MMAP_SIZE failed: %d", errno);
    else
        rcRet = RTERRINFO_LOG_REL_ADD_F(pErrInfo, VERR_NEM_INIT_FAILED, "Odd KVM_GET_VCPU_MMAP_SIZE value: %#x (%d)", rc, rc);

    /*
     * Init the slot ID bitmap.
     */
    ASMBitSet(&pVM->nem.s.bmSlotIds[0], 0);         /* don't use slot 0 */
    if (pVM->nem.s.cMaxMemSlots < _32K)
        ASMBitSetRange(&pVM->nem.s.bmSlotIds[0], pVM->nem.s.cMaxMemSlots, _32K);
    ASMBitSet(&pVM->nem.s.bmSlotIds[0], _32K - 1);  /* don't use the last slot */

    return rcRet;
}


/**
 * Does the early setup of a KVM VM.
 *
 * @returns VBox status code.
 * @param   pVM                 The cross context VM structure.
 * @param   pErrInfo            Where to always return error info.
 */
static int nemR3LnxInitSetupVm(PVM pVM, PRTERRINFO pErrInfo)
{
    AssertReturn(pVM->nem.s.fdVm != -1, RTErrInfoSet(pErrInfo, VERR_WRONG_ORDER, "Wrong initalization order"));

    /*
     * Enable user space MSRs and let us check everything KVM cannot handle.
     * We will set up filtering later when ring-3 init has completed.
     */
    struct kvm_enable_cap CapEn =
    {
        KVM_CAP_X86_USER_SPACE_MSR, 0,
        { KVM_MSR_EXIT_REASON_FILTER | KVM_MSR_EXIT_REASON_UNKNOWN | KVM_MSR_EXIT_REASON_INVAL, 0, 0, 0}
    };
    int rcLnx = ioctl(pVM->nem.s.fdVm, KVM_ENABLE_CAP, &CapEn);
    if (rcLnx == -1)
        return RTErrInfoSetF(pErrInfo, VERR_NEM_VM_CREATE_FAILED, "Failed to enable KVM_CAP_X86_USER_SPACE_MSR failed: %u", errno);

#ifdef VBOX_WITH_KVM_IRQCHIP_FULL
    rcLnx = ioctl(pVM->nem.s.fdVm, KVM_CREATE_IRQCHIP, 0);
    if (rcLnx == -1)
        return RTErrInfoSetF(pErrInfo, VERR_NEM_VM_CREATE_FAILED, "Failed to execute KVM_CREATE_VCPU: %u", errno);

    kvmSetGsiRoutingFullIrqChip(pVM);
#else
    struct kvm_enable_cap CapSplitIrqChip =
    {
        KVM_CAP_SPLIT_IRQCHIP, 0,
        { KVM_IRQCHIP_NUM_IOAPIC_INTR_PINS, 0, 0, 0}
    };
    rcLnx = ioctl(pVM->nem.s.fdVm, KVM_ENABLE_CAP, &CapSplitIrqChip);
    if (rcLnx == -1)
        return RTErrInfoSetF(pErrInfo, VERR_NEM_VM_CREATE_FAILED, "Failed to enable KVM_CAP_SPLIT_IRQCHIP: %u", errno);
#endif

    /*
     * Create the VCpus.
     */
    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
    {
        PVMCPU pVCpu = pVM->apCpusR3[idCpu];

        /* Create it. */
        pVCpu->nem.s.fdVCpu = ioctl(pVM->nem.s.fdVm, KVM_CREATE_VCPU, (unsigned long)idCpu);
        if (pVCpu->nem.s.fdVCpu < 0)
            return RTErrInfoSetF(pErrInfo, VERR_NEM_VM_CREATE_FAILED, "KVM_CREATE_VCPU failed for VCpu #%u: %d", idCpu, errno);

        /* Map the KVM_RUN area. */
        pVCpu->nem.s.pRun = (struct kvm_run *)mmap(NULL, pVM->nem.s.cbVCpuMmap, PROT_READ | PROT_WRITE, MAP_SHARED,
                                                   pVCpu->nem.s.fdVCpu, 0 /*offset*/);
        if ((void *)pVCpu->nem.s.pRun == MAP_FAILED)
            return RTErrInfoSetF(pErrInfo, VERR_NEM_VM_CREATE_FAILED, "mmap failed for VCpu #%u: %d", idCpu, errno);

        /* We want all x86 registers and events on each exit. */
        pVCpu->nem.s.pRun->kvm_valid_regs = KVM_SYNC_X86_REGS | KVM_SYNC_X86_SREGS | KVM_SYNC_X86_EVENTS;
    }

    pVM->nem.s.pARedirectionTable = std::make_unique<std::array<std::optional<MSIMSG>, KVM_IRQCHIP_NUM_IOAPIC_INTR_PINS>>();

    return VINF_SUCCESS;
}

static VBOXSTRICTRC nemR3LnxSetVCpuSignalMask(PVMCPU pVCpu, sigset_t *pSigset)
{
    /*
     * glibc and Linux/KVM do not agree on the size of sigset_t.
     */
    constexpr size_t kernel_sigset_size = 8;

    alignas(kvm_signal_mask) char backing[sizeof(kvm_signal_mask) + kernel_sigset_size];
    kvm_signal_mask *pKvmSignalMask = reinterpret_cast<kvm_signal_mask *>(backing);

    static_assert(sizeof(sigset_t) >= kernel_sigset_size);

    pKvmSignalMask->len = kernel_sigset_size;
    memcpy(pKvmSignalMask->sigset, pSigset, kernel_sigset_size);

    int rc = ioctl(pVCpu->nem.s.fdVCpu, KVM_SET_SIGNAL_MASK, pKvmSignalMask);
    AssertLogRelMsgReturn(rc == 0, ("Failed to set vCPU signal mask: %d", errno),
                          VERR_NEM_INIT_FAILED);

    return VINF_SUCCESS;
}

static void nemR3LnxConsumePokeSignal()
{
    int iPokeSignal = RTThreadPokeSignal();
    AssertReturnVoid(iPokeSignal >= 0);

    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, iPokeSignal);

    struct timespec timeout;

    /* Don't wait for a signal, just poll. */
    timeout.tv_sec = 0;
    timeout.tv_nsec = 0;

    int rc = sigtimedwait(&sigset, nullptr, &timeout);
    AssertLogRelMsg(rc >= 0 || errno == EAGAIN || errno == EINTR, ("Failed to consume signal: %d", errno));
}

/** @callback_method_impl{FNVMMEMTRENDEZVOUS}   */
static DECLCALLBACK(VBOXSTRICTRC) nemR3LnxFixThreadPoke(PVM pVM, PVMCPU pVCpu, void *pvUser)
{
    RT_NOREF(pVM, pvUser);

    int iPokeSignal = RTThreadPokeSignal();
    AssertReturn(iPokeSignal >= 0, VERR_NEM_INIT_FAILED);

    /* We disable the poke signal for the host. We never want that signal to be delivered. */
    int rc = RTThreadControlPokeSignal(pVCpu->hThread, false /*fEnable*/);
    AssertLogRelRC(rc);

    sigset_t sigset;

    /* Fetch the current signal mask. */
    int rcProcMask = pthread_sigmask(SIG_BLOCK /* ignored */, nullptr, &sigset);
    AssertLogRelMsgReturn(rcProcMask == 0, ("Failed to retrieve thread signal mask"), VERR_NEM_INIT_FAILED);

    sigdelset(&sigset, iPokeSignal);

    /* We enable the poke signal for the vCPU. Any poke will kick the vCPU out of guest execution. */
    VBOXSTRICTRC rcVcpuMask = nemR3LnxSetVCpuSignalMask(pVCpu, &sigset);
    AssertRCSuccessReturn(rcVcpuMask, rcVcpuMask);

    /* Create a timer that delivers the poke signal. */
    struct sigevent sev {};

    sev.sigev_notify = SIGEV_THREAD_ID;
    sev.sigev_signo = iPokeSignal;
    sev._sigev_un._tid = gettid();

    int rcTimer = timer_create(CLOCK_MONOTONIC, &sev, &pVCpu->nem.s.pTimer);
    AssertLogRelMsgReturn(rcTimer == 0, ("Failed to create timer: %d", errno), VERR_NEM_INIT_FAILED);

    return VINF_SUCCESS;
}

/**
 * Check common environment problems and inform the user about misconfigurations.
 */
int nemR3CheckEnvironment(void)
{
    static const char szSplitLockMitigationFile[] = "/proc/sys/kernel/split_lock_mitigate";

    char buf[64] {};
    int fd = open(szSplitLockMitigationFile, O_RDONLY | O_CLOEXEC);

    // Older kernels might not have this. A hard error feels unjustified here.
    AssertLogRelMsgReturn(fd >= 0, ("Failed to check %s (%d). Assuming there is no problem.\n", szSplitLockMitigationFile, fd),
                          VINF_SUCCESS);

    /* Leave one character to ensure that the string is zero-terminated. */
    ssize_t bytes = read(fd, buf, sizeof(buf) - 1);
    AssertLogRelMsgReturn(bytes >= 0, ("Failed to read %s (%zd)\n", szSplitLockMitigationFile, bytes),
                          VERR_NEM_INIT_FAILED);

    int mitigationStatus = atoi(buf);

    if (mitigationStatus != 0) {
        LogRel(("NEM: WARNING: %s is %d. This can cause VM hangs, unless you set split_lock_detect=off on the host kernel command line! Please set it to 0.\n",
                szSplitLockMitigationFile, mitigationStatus));
    }

    return VINF_SUCCESS;
}

/**
 * Try initialize the native API.
 *
 * This may only do part of the job, more can be done in
 * nemR3NativeInitAfterCPUM() and nemR3NativeInitCompleted().
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   fFallback       Whether we're in fallback mode or use-NEM mode. In
 *                          the latter we'll fail if we cannot initialize.
 * @param   fForced         Whether the HMForced flag is set and we should
 *                          fail if we cannot initialize.
 */
int nemR3NativeInit(PVM pVM, bool fFallback, bool fForced)
{
    RT_NOREF(pVM, fFallback, fForced);

    int rcCheck = nemR3CheckEnvironment();
    AssertLogRelMsgReturn(RT_SUCCESS(rcCheck), ("Failed to check environment\n"), VERR_NEM_INIT_FAILED);

    /*
     * Some state init.
     */
    pVM->nem.s.fdKvm = -1;
    pVM->nem.s.fdVm  = -1;
    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
    {
        PNEMCPU pNemCpu = &pVM->apCpusR3[idCpu]->nem.s;
        pNemCpu->fdVCpu = -1;
    }

    /*
     * Error state.
     * The error message will be non-empty on failure and 'rc' will be set too.
     */
    RTERRINFOSTATIC ErrInfo;
    PRTERRINFO pErrInfo = RTErrInfoInitStatic(&ErrInfo);

    /*
     * Open kvm subsystem so we can issue system ioctls.
     */
    int rc;
    int fdKvm = open("/dev/kvm", O_RDWR | O_CLOEXEC);
    if (fdKvm >= 0)
    {
        pVM->nem.s.fdKvm = fdKvm;

        /*
         * Create an empty VM since it is recommended we check capabilities on
         * the VM rather than the system descriptor.
         */
        int fdVm = ioctl(fdKvm, KVM_CREATE_VM, 0UL /* Type must be zero on x86 */);
        if (fdVm >= 0)
        {
            pVM->nem.s.fdVm = fdVm;

            /*
             * Check capabilities.
             */
            rc = nemR3LnxInitCheckCapabilities(pVM, pErrInfo);
            if (RT_SUCCESS(rc))
            {
                /*
                 * Set up the VM (more on this later).
                 */
                rc = nemR3LnxInitSetupVm(pVM, pErrInfo);
                if (RT_SUCCESS(rc))
                {
                    /*
                     * Set ourselves as the execution engine and make config adjustments.
                     */
                    VM_SET_MAIN_EXECUTION_ENGINE(pVM, VM_EXEC_ENGINE_NATIVE_API);
                    Log(("NEM: Marked active!\n"));
                    PGMR3EnableNemMode(pVM);

                    /*
                     * Register release statistics
                     */
                    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
                    {
                        PNEMCPU pNemCpu = &pVM->apCpusR3[idCpu]->nem.s;
                        STAMR3RegisterF(pVM, &pNemCpu->StatImportOnDemand,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "Number of on-demand state imports",      "/NEM/CPU%u/ImportOnDemand", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatImportOnReturn,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "Number of state imports on loop return", "/NEM/CPU%u/ImportOnReturn", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatImportOnReturnSkipped, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "Number of skipped state imports on loop return", "/NEM/CPU%u/ImportOnReturnSkipped", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatImportPendingInterrupt, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "Number of times an interrupt was pending when importing from KVM", "/NEM/CPU%u/ImportPendingInterrupt", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatExportPendingInterrupt, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "Number of times an interrupt was pending when exporting to KVM", "/NEM/CPU%u/ExportPendingInterrupt", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatFlushExitOnReturn,   STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "Number of times a KVM_EXIT_IO or KVM_EXIT_MMIO was flushed before returning to EM", "/NEM/CPU%u/FlushExitOnReturn", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatFlushExitOnReturn1Loop, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "Number of times a KVM_EXIT_IO or KVM_EXIT_MMIO was flushed before returning to EM", "/NEM/CPU%u/FlushExitOnReturn-01-loop", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatFlushExitOnReturn2Loops, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "Number of times a KVM_EXIT_IO or KVM_EXIT_MMIO was flushed before returning to EM", "/NEM/CPU%u/FlushExitOnReturn-02-loops", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatFlushExitOnReturn3Loops, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "Number of times a KVM_EXIT_IO or KVM_EXIT_MMIO was flushed before returning to EM", "/NEM/CPU%u/FlushExitOnReturn-03-loops", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatFlushExitOnReturn4PlusLoops, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "Number of times a KVM_EXIT_IO or KVM_EXIT_MMIO was flushed before returning to EM", "/NEM/CPU%u/FlushExitOnReturn-04-to-7-loops", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatQueryCpuTick,        STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "Number of TSC queries",                  "/NEM/CPU%u/QueryCpuTick", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatExitTotal,           STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "All exits",                  "/NEM/CPU%u/Exit", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatExitIo,              STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "KVM_EXIT_IO",                "/NEM/CPU%u/Exit/Io", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatExitMmio,            STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "KVM_EXIT_MMIO",              "/NEM/CPU%u/Exit/Mmio", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatExitSetTpr,          STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "KVM_EXIT_SET_TRP",           "/NEM/CPU%u/Exit/SetTpr", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatExitTprAccess,       STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "KVM_EXIT_TPR_ACCESS",        "/NEM/CPU%u/Exit/TprAccess", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatExitRdMsr,           STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "KVM_EXIT_RDMSR",             "/NEM/CPU%u/Exit/RdMsr", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatExitWrMsr,           STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "KVM_EXIT_WRMSR",             "/NEM/CPU%u/Exit/WrMsr", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatExitIrqWindowOpen,   STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "KVM_EXIT_IRQ_WINDOWS_OPEN",  "/NEM/CPU%u/Exit/IrqWindowOpen", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatExitHalt,            STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "KVM_EXIT_HLT",               "/NEM/CPU%u/Exit/Hlt", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatExitIntr,            STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "KVM_EXIT_INTR",              "/NEM/CPU%u/Exit/Intr", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatExitHypercall,       STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "KVM_EXIT_HYPERCALL",         "/NEM/CPU%u/Exit/Hypercall", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatExitDebug,           STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "KVM_EXIT_DEBUG",             "/NEM/CPU%u/Exit/Debug", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatExitBusLock,         STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "KVM_EXIT_BUS_LOCK",          "/NEM/CPU%u/Exit/BusLock", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatExitInternalErrorEmulation, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "KVM_EXIT_INTERNAL_ERROR/EMULATION", "/NEM/CPU%u/Exit/InternalErrorEmulation", idCpu);
                        STAMR3RegisterF(pVM, &pNemCpu->StatExitInternalErrorFatal,     STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "KVM_EXIT_INTERNAL_ERROR/*", "/NEM/CPU%u/Exit/InternalErrorFatal", idCpu);
                    }

                    /*
                     * Success.
                     */
                    return VINF_SUCCESS;
                }

                /*
                 * Bail out.
                 */
            }
            close(fdVm);
            pVM->nem.s.fdVm = -1;
        }
        else
            rc = RTErrInfoSetF(pErrInfo, VERR_NEM_VM_CREATE_FAILED, "KVM_CREATE_VM failed: %u", errno);
        close(fdKvm);
        pVM->nem.s.fdKvm = -1;
    }
    else if (errno == EACCES)
        rc = RTErrInfoSet(pErrInfo, VERR_ACCESS_DENIED, "Do not have access to open /dev/kvm for reading & writing.");
    else if (errno == ENOENT)
        rc = RTErrInfoSet(pErrInfo, VERR_NOT_SUPPORTED, "KVM is not availble (/dev/kvm does not exist)");
    else
        rc = RTErrInfoSetF(pErrInfo, RTErrConvertFromErrno(errno), "Failed to open '/dev/kvm': %u", errno);

    /*
     * We only fail if in forced mode, otherwise just log the complaint and return.
     */
    Assert(RTErrInfoIsSet(pErrInfo));
    if (   (fForced || !fFallback)
        && pVM->bMainExecutionEngine != VM_EXEC_ENGINE_NATIVE_API)
        return VMSetError(pVM, RT_SUCCESS_NP(rc) ? VERR_NEM_NOT_AVAILABLE : rc, RT_SRC_POS, "%s", pErrInfo->pszMsg);
    LogRel(("NEM: Not available: %s\n", pErrInfo->pszMsg));
    return VINF_SUCCESS;
}


/**
 * This is called after CPUMR3Init is done.
 *
 * @returns VBox status code.
 * @param   pVM                 The VM handle..
 */
int nemR3NativeInitAfterCPUM(PVM pVM)
{
    /*
     * Validate sanity.
     */
    AssertReturn(pVM->nem.s.fdKvm >= 0, VERR_WRONG_ORDER);
    AssertReturn(pVM->nem.s.fdVm >= 0, VERR_WRONG_ORDER);
    AssertReturn(pVM->bMainExecutionEngine == VM_EXEC_ENGINE_NATIVE_API, VERR_WRONG_ORDER);

    /** @todo */

    return VINF_SUCCESS;
}

static PCPUMCPUIDLEAF findKvmLeaf(PCPUMCPUIDLEAF paKvmSupportedLeaves,
                                  uint32_t cKvmSupportedLeaves,
                                  uint32_t leaf,
                                  uint32_t subleaf)
{
    for (uint32_t i = 0; i < cKvmSupportedLeaves; i++) {
        auto& kvmLeaf = paKvmSupportedLeaves[i];

        if (kvmLeaf.uLeaf == leaf && kvmLeaf.uSubLeaf == subleaf) {
            return &kvmLeaf;
        }
    }

    return nullptr;
}

static void maybeMaskUnsupportedKVMCpuidLeafValues(PCPUMCPUIDLEAF paKvmSupportedLeaves,
                                                   uint32_t cKvmSupportedLeaves,
                                                   uint32_t leaf,
                                                   uint32_t subleaf,
                                                   uint32_t& eax,
                                                   uint32_t& ebx,
                                                   uint32_t& ecx,
                                                   uint32_t& edx)
{
    static const uint32_t CPUID_FEATURE_INFORMATION_LEAF = 0x1;

    /*
     * A list of CPUID leaves that we want to mask with the KVM
     * supported values. For example, we want to make sure that FSGSBASE
     * support is supported by KVM before we offer it to the guest.
     * VirtualBox detects the features it wants to offer via CPUID,
     * which bypasses Linux/KVM.
     */
    const std::vector<uint32_t> leavesToMask = {
        CPUID_FEATURE_INFORMATION_LEAF,
        0x6,        // Thermal and power management
        0x7,        // Structured Extended Feature Flags Enumeration
        0x12,       // SGX capabilities
        0x14,       // Processor Trace
        0x19,       // AES Key Locker features
        0x24,       // AVX10 Features
        0x80000001, // Extended Processor Info and Feature Bits
        0x80000007, // Processor Power Management Information and RAS Capabilities
        0x80000008, // Virtual and Physical address Sizes
        0x8000000A, // Secure Virtual Machine features
        0x8000001F, // Encrypted Memory Capabilities
        0x80000021, // Extended Feature Identification 2
    };

    if (std::find(leavesToMask.begin(), leavesToMask.end(), leaf) == leavesToMask.end()) {
        return;
    }

    auto* paKvmSupportedLeaf = findKvmLeaf(paKvmSupportedLeaves, cKvmSupportedLeaves, leaf, subleaf);

    if (paKvmSupportedLeaf == nullptr) {
        return;
    }

    switch (leaf) {
    case CPUID_FEATURE_INFORMATION_LEAF:
        eax &= paKvmSupportedLeaf->uEax;
        // ebx reports APIC IDs which we would mask if we use the
        // KVM supported values.
        ecx &= paKvmSupportedLeaf->uEcx;
        ecx |= X86_CPUID_FEATURE_ECX_HVP; // The hypervisor bit is not enabled in the KVM values.
        edx &= paKvmSupportedLeaf->uEdx;
        break;
    default:
        eax &= paKvmSupportedLeaf->uEax;
        ebx &= paKvmSupportedLeaf->uEbx;
        ecx &= paKvmSupportedLeaf->uEcx;
        edx &= paKvmSupportedLeaf->uEdx;
        break;
    }
}

/**
 * Update the CPUID leaves for a VCPU.
 *
 * The KVM_SET_CPUID2 call replaces any previous leaves, so we have to redo
 * everything when there really just are single bit changes.  That said, it
 * looks like KVM update the XCR/XSAVE related stuff as well as the APIC enabled
 * bit(s), so it should suffice if we do this at startup, I hope.
 */
static int nemR3LnxUpdateCpuIdsLeaves(PVM pVM, PVMCPU pVCpu)
{
    uint32_t              cLeaves  = 0;
    PCCPUMCPUIDLEAF const paLeaves = CPUMR3CpuIdGetPtr(pVM, &cLeaves);
    struct kvm_cpuid2    *pReq = (struct kvm_cpuid2 *)alloca(RT_UOFFSETOF_DYN(struct kvm_cpuid2, entries[cLeaves + 2]));

    pReq->nent    = cLeaves;
    pReq->padding = 0;

    size_t cKvmSupportedLeaves = 0;
    PCPUMCPUIDLEAF paKvmSupportedLeaves = nullptr;
    int rc = NEMR3KvmGetCpuIdLeaves(pVM, &paKvmSupportedLeaves, &cKvmSupportedLeaves);
    AssertLogRelMsgReturn(RT_SUCCESS(rc), ("Could not retrieve supported CPUID leaves"), rc);


    for (uint32_t i = 0; i < cLeaves; i++)
    {
        CPUMGetGuestCpuId(pVCpu, paLeaves[i].uLeaf, paLeaves[i].uSubLeaf, -1 /*f64BitMode*/,
                          &pReq->entries[i].eax,
                          &pReq->entries[i].ebx,
                          &pReq->entries[i].ecx,
                          &pReq->entries[i].edx);

        maybeMaskUnsupportedKVMCpuidLeafValues(paKvmSupportedLeaves,
                                               cKvmSupportedLeaves,
                                               paLeaves[i].uLeaf,
                                               paLeaves[i].uSubLeaf,
                                               pReq->entries[i].eax,
                                               pReq->entries[i].ebx,
                                               pReq->entries[i].ecx,
                                               pReq->entries[i].edx);

        pReq->entries[i].function   = paLeaves[i].uLeaf;
        pReq->entries[i].index      = paLeaves[i].uSubLeaf;
        pReq->entries[i].flags      = !paLeaves[i].fSubLeafMask ? 0 : KVM_CPUID_FLAG_SIGNIFCANT_INDEX;
        pReq->entries[i].padding[0] = 0;
        pReq->entries[i].padding[1] = 0;
        pReq->entries[i].padding[2] = 0;
    }

    int rcLnx = ioctl(pVCpu->nem.s.fdVCpu, KVM_SET_CPUID2, pReq);
    AssertLogRelMsgReturn(rcLnx == 0, ("rcLnx=%d errno=%d cLeaves=%#x\n", rcLnx, errno, cLeaves), RTErrConvertFromErrno(errno));

    return VINF_SUCCESS;
}

static int nemR3LnxInitGuestInterface(PVM pVM)
{
    switch (pVM->gim.s.enmProviderId) {
    case GIMPROVIDERID_HYPERV:
        /*
          SynIC is currently disabled pending investigation of interrupt issues. See #19.

          Enabling this capability is not sufficient to enable SynNIC. The corresponding features in the Hyper-V CPUID
          leaves also have to be enabled. Look for SYNIC and STIMER in GIMHv.cpp.

          The CPUID implementation hints must also indicate deprecating AutoEOI to make APICv work.
         */
#if 0
        LogRel(("NEM: Enabling SYNIC.\n"));

        for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
        {
            PVMCPU pVCpu = pVM->apCpusR3[idCpu];

            struct kvm_enable_cap CapSynIC =
            {
                KVM_CAP_HYPERV_SYNIC2, 0, { 0, 0, 0, 0 }
            };

            int rcLnx = ioctl(pVCpu->nem.s.fdVCpu, KVM_ENABLE_CAP, &CapSynIC);
            AssertLogRelMsgReturn(rcLnx == 0, ("Failed to enable SYNIC: rcLnx=%d errno=%d\n", rcLnx, errno),
                                  RTErrConvertFromErrno(errno));
        }
#endif

        break;

    default:
        /* Other guest interfaces are not fully supported. */
        break;
    }

    return VINF_SUCCESS;
}

namespace
{

enum class KvmCpuIdIoctl : uint32_t
{
    CPUID = KVM_GET_SUPPORTED_CPUID,
    HV_CPUID = KVM_GET_SUPPORTED_HV_CPUID
};

int KvmGetCpuIdLeavesGeneric(PVM pVM, KvmCpuIdIoctl ioctlNum, PCPUMCPUIDLEAF *outpCpuId, size_t *outcLeaves)
{
    struct kvm_cpuid2 *pKvmCpuid;
    uint32_t cLeaves = 0;
    int rc;

    /* In case we exit due to errors. */
    *outpCpuId = nullptr;
    *outcLeaves = 0;

    /* There is no way to query how many leaves there are. We just try until we hit the right size. */
    do
    {
        cLeaves += 1;
        Log(("Querying for %u leaves\n", cLeaves));

        pKvmCpuid = static_cast<struct kvm_cpuid2 *>(alloca(RT_UOFFSETOF_DYN(struct kvm_cpuid2, entries[cLeaves])));

        pKvmCpuid->nent = cLeaves;
        pKvmCpuid->padding = 0;

        rc = ioctl(pVM->nem.s.fdKvm, static_cast<uint32_t>(ioctlNum), pKvmCpuid);
    } while (rc != 0 && errno == E2BIG);
    AssertLogRelMsgReturn(rc == 0, ("Failed to query supported CPUID leaves: errno=%d", errno), RTErrConvertFromErrno(errno));
    AssertFatal(cLeaves == pKvmCpuid->nent);

    PCPUMCPUIDLEAF pCpuId = static_cast<PCPUMCPUIDLEAF>(RTMemAllocZ(sizeof(*pCpuId) * cLeaves));

    for (uint32_t uLeaf = 0; uLeaf < cLeaves; uLeaf++)
    {
        pCpuId[uLeaf].uLeaf = pKvmCpuid->entries[uLeaf].function;
        pCpuId[uLeaf].uSubLeaf = pKvmCpuid->entries[uLeaf].index;

        pCpuId[uLeaf].uEax = pKvmCpuid->entries[uLeaf].eax;
        pCpuId[uLeaf].uEbx = pKvmCpuid->entries[uLeaf].ebx;
        pCpuId[uLeaf].uEcx = pKvmCpuid->entries[uLeaf].ecx;
        pCpuId[uLeaf].uEdx = pKvmCpuid->entries[uLeaf].edx;
    }

    *outpCpuId = pCpuId;
    *outcLeaves = cLeaves;

    return VINF_SUCCESS;
}

} // anonymous namespace

int NEMR3KvmGetHvCpuIdLeaves(PVM pVM, PCPUMCPUIDLEAF *outpCpuId, size_t *outcLeaves)
{
    return KvmGetCpuIdLeavesGeneric(pVM, KvmCpuIdIoctl::HV_CPUID, outpCpuId, outcLeaves);
}

int NEMR3KvmGetCpuIdLeaves(PVM pVM, PCPUMCPUIDLEAF *outpCpuId, size_t *outcLeaves)
{
    return KvmGetCpuIdLeavesGeneric(pVM, KvmCpuIdIoctl::CPUID, outpCpuId, outcLeaves);
}

int nemR3NativeInitCompleted(PVM pVM, VMINITCOMPLETED enmWhat)
{
    /*
     * Make RTThreadPoke work again (disabled for avoiding unnecessary
     * critical section issues in ring-0).
     */
    if (enmWhat == VMINITCOMPLETED_RING3)
        VMMR3EmtRendezvous(pVM, VMMEMTRENDEZVOUS_FLAGS_TYPE_ALL_AT_ONCE, nemR3LnxFixThreadPoke, NULL);

    /*
     * Configure CPUIDs after ring-3 init has been done.
     */
    if (enmWhat == VMINITCOMPLETED_RING3)
    {
        for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
        {
            int rc = nemR3LnxUpdateCpuIdsLeaves(pVM, pVM->apCpusR3[idCpu]);
            AssertRCReturn(rc, rc);
        }
    }

    if (enmWhat == VMINITCOMPLETED_RING3)
    {
        int rc = nemR3LnxInitGuestInterface(pVM);
        AssertRCReturn(rc, rc);
    }

    /*
     * Configure MSRs after ring-3 init is done.
     *
     * We only need to tell KVM which MSRs it can handle, as we already
     * requested KVM_MSR_EXIT_REASON_FILTER, KVM_MSR_EXIT_REASON_UNKNOWN
     * and KVM_MSR_EXIT_REASON_INVAL in nemR3LnxInitSetupVm, and here we
     * will use KVM_MSR_FILTER_DEFAULT_DENY.  So, all MSRs w/o a 1 in the
     * bitmaps should be deferred to ring-3.
     */
    if (enmWhat == VMINITCOMPLETED_RING3)
    {
        struct kvm_msr_filter MsrFilters = {0}; /* Structure with a couple of implicit paddings on 64-bit systems. */
        MsrFilters.flags = KVM_MSR_FILTER_DEFAULT_DENY;

        unsigned iRange = 0;
#define MSR_RANGE_BEGIN(a_uBase, a_uEnd, a_fFlags) \
        AssertCompile(0x3000 <= KVM_MSR_FILTER_MAX_BITMAP_SIZE * 8); \
        uint64_t RT_CONCAT(bm, a_uBase)[0x3000 / 64] = {0}; \
        do { \
            uint64_t * const pbm = RT_CONCAT(bm, a_uBase); \
            uint32_t   const uBase = UINT32_C(a_uBase); \
            uint32_t   const cMsrs = UINT32_C(a_uEnd) - UINT32_C(a_uBase); \
            MsrFilters.ranges[iRange].base   = UINT32_C(a_uBase); \
            MsrFilters.ranges[iRange].nmsrs  = cMsrs; \
            MsrFilters.ranges[iRange].flags  = (a_fFlags); \
            MsrFilters.ranges[iRange].bitmap = (uint8_t *)&RT_CONCAT(bm, a_uBase)[0]
#define MSR_RANGE_ADD(a_Msr) \
        do { Assert((uint32_t)(a_Msr) - uBase < cMsrs); ASMBitSet(pbm, (uint32_t)(a_Msr) - uBase); } while (0)
#define MSR_RANGE_ADD_CLOSED_IVL(first_Msr, last_Msr) \
        for (uint32_t uMsr = (first_Msr); uMsr <= last_Msr; uMsr++) { MSR_RANGE_ADD(uMsr); }
#define MSR_RANGE_END(a_cMinMsrs) \
            /* optimize the range size before closing: */ \
            uint32_t cBitmap = cMsrs / 64; \
            while (cBitmap > ((a_cMinMsrs) + 63 / 64) && pbm[cBitmap - 1] == 0) \
                cBitmap -= 1; \
            MsrFilters.ranges[iRange].nmsrs = cBitmap * 64; \
            iRange++; \
        } while (0)

        /* 1st Intel range: 0000_0000 to 0000_3000. */
        MSR_RANGE_BEGIN(0x00000000, 0x00003000, KVM_MSR_FILTER_READ | KVM_MSR_FILTER_WRITE);
        MSR_RANGE_ADD(MSR_IA32_TSC);
        MSR_RANGE_ADD(MSR_IA32_APICBASE);
        MSR_RANGE_ADD(MSR_IA32_SYSENTER_CS);
        MSR_RANGE_ADD(MSR_IA32_SYSENTER_ESP);
        MSR_RANGE_ADD(MSR_IA32_SYSENTER_EIP);
        MSR_RANGE_ADD(MSR_IA32_CR_PAT);
        /** @todo more? */
        MSR_RANGE_END(64);

        /* 1st AMD range: c000_0000 to c000_3000 */
        MSR_RANGE_BEGIN(0xc0000000, 0xc0003000, KVM_MSR_FILTER_READ | KVM_MSR_FILTER_WRITE);
        MSR_RANGE_ADD(MSR_K6_EFER);
        MSR_RANGE_ADD(MSR_K6_STAR);

        /*
         * If we don't allow direct access to FS_BASE, we clobber the FS base for the guest. This sounds like a bug in
         * our state synchronization with KVM.
         */
        MSR_RANGE_ADD(MSR_K8_FS_BASE);

        MSR_RANGE_ADD(MSR_K8_GS_BASE);
        MSR_RANGE_ADD(MSR_K8_KERNEL_GS_BASE);
        MSR_RANGE_ADD(MSR_K8_LSTAR);
        MSR_RANGE_ADD(MSR_K8_CSTAR);
        MSR_RANGE_ADD(MSR_K8_SF_MASK);
        MSR_RANGE_ADD(MSR_K8_TSC_AUX);
        /** @todo add more? */
        MSR_RANGE_END(64);

        if (pVM->gim.s.enmProviderId == GIMPROVIDERID_HYPERV)
        {
            MSR_RANGE_BEGIN(0x40000000, 0x40003000, KVM_MSR_FILTER_READ | KVM_MSR_FILTER_WRITE);

            MSR_RANGE_ADD_CLOSED_IVL(MSR_GIM_HV_RANGE0_FIRST, MSR_GIM_HV_RANGE0_LAST);
            MSR_RANGE_ADD_CLOSED_IVL(MSR_GIM_HV_RANGE1_FIRST, MSR_GIM_HV_RANGE1_LAST);
            MSR_RANGE_ADD_CLOSED_IVL(MSR_GIM_HV_RANGE2_FIRST, MSR_GIM_HV_RANGE2_LAST);
            MSR_RANGE_ADD_CLOSED_IVL(MSR_GIM_HV_RANGE3_FIRST, MSR_GIM_HV_RANGE3_LAST);

            /* SynIC / STimer */
            MSR_RANGE_ADD_CLOSED_IVL(MSR_GIM_HV_RANGE4_FIRST, MSR_GIM_HV_RANGE4_LAST);
            MSR_RANGE_ADD_CLOSED_IVL(MSR_GIM_HV_RANGE5_FIRST, MSR_GIM_HV_RANGE5_LAST);
            MSR_RANGE_ADD_CLOSED_IVL(MSR_GIM_HV_RANGE6_FIRST, MSR_GIM_HV_RANGE6_LAST);

            MSR_RANGE_ADD_CLOSED_IVL(MSR_GIM_HV_RANGE7_FIRST, MSR_GIM_HV_RANGE7_LAST);
            MSR_RANGE_ADD_CLOSED_IVL(MSR_GIM_HV_RANGE8_FIRST, MSR_GIM_HV_RANGE8_LAST);
            MSR_RANGE_ADD_CLOSED_IVL(MSR_GIM_HV_RANGE9_FIRST, MSR_GIM_HV_RANGE9_LAST);
            MSR_RANGE_ADD_CLOSED_IVL(MSR_GIM_HV_RANGE10_FIRST, MSR_GIM_HV_RANGE10_LAST);
            MSR_RANGE_ADD_CLOSED_IVL(MSR_GIM_HV_RANGE11_FIRST, MSR_GIM_HV_RANGE11_LAST);

            /*
             * Crash MSRs
             *
             * We deliberately don't add them here, so we can handle them instead of KVM. This allows us to log the
             * crash reason into VM log instead of it ending up in the kernel's log.
             */
            // MSR_RANGE_ADD_CLOSED_IVL(MSR_GIM_HV_RANGE12_FIRST, MSR_GIM_HV_RANGE12_LAST);

            /*
             * These should be available to the guest with feature bit 23 in the base features, which we don't
             * expose. But Windows touches them anyway?
             */
            MSR_RANGE_ADD(0x40000114 /* HV_X64_MSR_STIME_UNHALTED_TIMER_CONFIG */);
            MSR_RANGE_ADD(0x40000115 /* HV_X64_MSR_STIME_UNHALTED_TIMER_COUNT */);

            /*
             * These are available to the guest with feature bit 15 in the base features (undocumented).
             */
            MSR_RANGE_ADD(0x40000118 /* HV_X64_MSR_TSC_INVARIANT_CONTROL */);

            MSR_RANGE_END(64);
        }

        /** @todo Specify other ranges too? Like hyper-V and KVM to make sure we get
         *        the MSR requests instead of KVM. */

        int rcLnx = ioctl(pVM->nem.s.fdVm, KVM_X86_SET_MSR_FILTER, &MsrFilters);
        if (rcLnx == -1)
            return VMSetError(pVM, VERR_NEM_VM_CREATE_FAILED, RT_SRC_POS,
                              "Failed to enable KVM_X86_SET_MSR_FILTER failed: %u", errno);
    }

    return VINF_SUCCESS;
}


int nemR3NativeTerm(PVM pVM)
{
    /*
     * Per-cpu data
     */
    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
    {
        PVMCPU pVCpu = pVM->apCpusR3[idCpu];

        if (pVCpu->nem.s.fdVCpu != -1)
        {
            close(pVCpu->nem.s.fdVCpu);
            pVCpu->nem.s.fdVCpu = -1;
        }
        if (pVCpu->nem.s.pRun)
        {
            munmap(pVCpu->nem.s.pRun, pVM->nem.s.cbVCpuMmap);
            pVCpu->nem.s.pRun = NULL;
        }
    }

    /*
     * Global data.
     */
    if (pVM->nem.s.fdVm != -1)
    {
        close(pVM->nem.s.fdVm);
        pVM->nem.s.fdVm = -1;
    }

    if (pVM->nem.s.fdKvm != -1)
    {
        close(pVM->nem.s.fdKvm);
        pVM->nem.s.fdKvm = -1;
    }

    pVM->nem.s.pARedirectionTable.reset();

    return VINF_SUCCESS;
}


/**
 * VM reset notification.
 *
 * @param   pVM         The cross context VM structure.
 */
void nemR3NativeReset(PVM pVM)
{
    pVM->nem.s.pARedirectionTable->fill(std::nullopt);

    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
    {
        PVMCPU pVCpu = pVM->apCpusR3[idCpu];

        struct kvm_mp_state mp;
        mp.mp_state = pVCpu->idCpu == 0 ? KVM_MP_STATE_RUNNABLE : KVM_MP_STATE_UNINITIALIZED;

        int rcLnx = ioctl(pVCpu->nem.s.fdVCpu, KVM_SET_MP_STATE, &mp);
        AssertLogRelMsg(rcLnx == 0, ("nemR3NativeReset: Failed to set MP state. Error: %d, errno %d\n", rcLnx, errno));
    }
}


/**
 * Reset CPU due to INIT IPI or hot (un)plugging.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the CPU being
 *                      reset.
 * @param   fInitIpi    Whether this is the INIT IPI or hot (un)plugging case.
 */
void nemR3NativeResetCpu(PVMCPU pVCpu, bool fInitIpi)
{
    RT_NOREF(pVCpu, fInitIpi);
}


/*********************************************************************************************************************************
*   Memory management                                                                                                            *
*********************************************************************************************************************************/


/**
 * Allocates a memory slot ID.
 *
 * @returns Slot ID on success, UINT16_MAX on failure.
 */
static uint16_t nemR3LnxMemSlotIdAlloc(PVM pVM)
{
    /* Use the hint first. */
    uint16_t idHint = pVM->nem.s.idPrevSlot;
    if (idHint < _32K - 1)
    {
        int32_t idx = ASMBitNextClear(&pVM->nem.s.bmSlotIds, _32K, idHint);
        Assert(idx < _32K);
        if (idx > 0 && !ASMAtomicBitTestAndSet(&pVM->nem.s.bmSlotIds, idx))
            return pVM->nem.s.idPrevSlot = (uint16_t)idx;
    }

    /*
     * Search the whole map from the start.
     */
    int32_t idx = ASMBitFirstClear(&pVM->nem.s.bmSlotIds, _32K);
    Assert(idx < _32K);
    if (idx > 0 && !ASMAtomicBitTestAndSet(&pVM->nem.s.bmSlotIds, idx))
        return pVM->nem.s.idPrevSlot = (uint16_t)idx;

    Assert(idx < 0 /*shouldn't trigger unless there is a race */);
    return UINT16_MAX; /* caller is expected to assert. */
}


/**
 * Frees a memory slot ID
 */
static void nemR3LnxMemSlotIdFree(PVM pVM, uint16_t idSlot)
{
    if (RT_LIKELY(idSlot < _32K && ASMAtomicBitTestAndClear(&pVM->nem.s.bmSlotIds, idSlot)))
    { /*likely*/ }
    else
        AssertMsgFailed(("idSlot=%u (%#x)\n", idSlot, idSlot));
}



VMMR3_INT_DECL(int) NEMR3NotifyPhysRamRegister(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, void *pvR3,
                                               uint8_t *pu2State, uint32_t *puNemRange)
{
    uint16_t idSlot = nemR3LnxMemSlotIdAlloc(pVM);
    AssertLogRelReturn(idSlot < _32K, VERR_NEM_MAP_PAGES_FAILED);

    Log5(("NEMR3NotifyPhysRamRegister: %RGp LB %RGp, pvR3=%p pu2State=%p (%d) puNemRange=%p (%d) - idSlot=%#x\n",
          GCPhys, cb, pvR3, pu2State, pu2State, puNemRange, *puNemRange, idSlot));

    struct kvm_userspace_memory_region Region;
    Region.slot             = idSlot;
    Region.flags            = 0;
    Region.guest_phys_addr  = GCPhys;
    Region.memory_size      = cb;
    Region.userspace_addr   = (uintptr_t)pvR3;

    int rc = ioctl(pVM->nem.s.fdVm, KVM_SET_USER_MEMORY_REGION, &Region);
    if (rc == 0)
    {
        *pu2State   = 0;
        *puNemRange = idSlot;
        return VINF_SUCCESS;
    }

    LogRel(("NEMR3NotifyPhysRamRegister: %RGp LB %RGp, pvR3=%p, idSlot=%#x failed: %u/%u\n", GCPhys, cb, pvR3, idSlot, rc, errno));
    nemR3LnxMemSlotIdFree(pVM, idSlot);
    return VERR_NEM_MAP_PAGES_FAILED;
}


VMMR3_INT_DECL(bool) NEMR3IsMmio2DirtyPageTrackingSupported(PVM pVM)
{
    RT_NOREF(pVM);
    return true;
}


VMMR3_INT_DECL(int) NEMR3NotifyPhysMmioExMapEarly(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, uint32_t fFlags,
                                                  void *pvRam, void *pvMmio2, uint8_t *pu2State, uint32_t *puNemRange)
{
    Log5(("NEMR3NotifyPhysMmioExMapEarly: %RGp LB %RGp fFlags=%#x pvRam=%p pvMmio2=%p pu2State=%p (%d) puNemRange=%p (%#x)\n",
          GCPhys, cb, fFlags, pvRam, pvMmio2, pu2State, *pu2State, puNemRange, puNemRange ? *puNemRange : UINT32_MAX));
    RT_NOREF(pvRam);

    if (fFlags & NEM_NOTIFY_PHYS_MMIO_EX_F_REPLACE)
    {
        /** @todo implement splitting and whatnot of ranges if we want to be 100%
         *        conforming (just modify RAM registrations in MM.cpp to test). */
        AssertLogRelMsgFailedReturn(("%RGp LB %RGp fFlags=%#x pvRam=%p pvMmio2=%p\n", GCPhys, cb, fFlags, pvRam, pvMmio2),
                                    VERR_NEM_MAP_PAGES_FAILED);
    }

    /*
     * Register MMIO2.
     */
    if (fFlags & NEM_NOTIFY_PHYS_MMIO_EX_F_MMIO2)
    {
        AssertReturn(pvMmio2, VERR_NEM_MAP_PAGES_FAILED);
        AssertReturn(puNemRange, VERR_NEM_MAP_PAGES_FAILED);

        uint16_t idSlot = nemR3LnxMemSlotIdAlloc(pVM);
        AssertLogRelReturn(idSlot < _32K, VERR_NEM_MAP_PAGES_FAILED);

        struct kvm_userspace_memory_region Region;
        Region.slot             = idSlot;
        Region.flags            = fFlags & NEM_NOTIFY_PHYS_MMIO_EX_F_TRACK_DIRTY_PAGES ? KVM_MEM_LOG_DIRTY_PAGES : 0;
        Region.guest_phys_addr  = GCPhys;
        Region.memory_size      = cb;
        Region.userspace_addr   = (uintptr_t)pvMmio2;

        int rc = ioctl(pVM->nem.s.fdVm, KVM_SET_USER_MEMORY_REGION, &Region);
        if (rc == 0)
        {
            *pu2State   = 0;
            *puNemRange = idSlot;
            Log5(("NEMR3NotifyPhysMmioExMapEarly: %RGp LB %RGp fFlags=%#x pvMmio2=%p - idSlot=%#x\n",
                  GCPhys, cb, fFlags, pvMmio2, idSlot));
            return VINF_SUCCESS;
        }

        nemR3LnxMemSlotIdFree(pVM, idSlot);
        AssertLogRelMsgFailedReturn(("%RGp LB %RGp fFlags=%#x, pvMmio2=%p, idSlot=%#x failed: %u/%u\n",
                                     GCPhys, cb, fFlags, pvMmio2, idSlot, errno, rc),
                                    VERR_NEM_MAP_PAGES_FAILED);
    }

    /* MMIO, don't care. */
    *pu2State   = 0;
    *puNemRange = UINT32_MAX;
    return VINF_SUCCESS;
}


VMMR3_INT_DECL(int) NEMR3NotifyPhysMmioExMapLate(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, uint32_t fFlags,
                                                 void *pvRam, void *pvMmio2, uint32_t *puNemRange)
{
    RT_NOREF(pVM, GCPhys, cb, fFlags, pvRam, pvMmio2, puNemRange);
    return VINF_SUCCESS;
}


VMMR3_INT_DECL(int) NEMR3NotifyPhysMmioExUnmap(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, uint32_t fFlags, void *pvRam,
                                               void *pvMmio2, uint8_t *pu2State, uint32_t *puNemRange)
{
    Log5(("NEMR3NotifyPhysMmioExUnmap: %RGp LB %RGp fFlags=%#x pvRam=%p pvMmio2=%p pu2State=%p puNemRange=%p (%#x)\n",
          GCPhys, cb, fFlags, pvRam, pvMmio2, pu2State, puNemRange, *puNemRange));
    RT_NOREF(pVM, GCPhys, cb, fFlags, pvRam, pvMmio2, pu2State);

    if (fFlags & NEM_NOTIFY_PHYS_MMIO_EX_F_REPLACE)
    {
        /** @todo implement splitting and whatnot of ranges if we want to be 100%
         *        conforming (just modify RAM registrations in MM.cpp to test). */
        AssertLogRelMsgFailedReturn(("%RGp LB %RGp fFlags=%#x pvRam=%p pvMmio2=%p\n", GCPhys, cb, fFlags, pvRam, pvMmio2),
                                    VERR_NEM_UNMAP_PAGES_FAILED);
    }

    if (fFlags & NEM_NOTIFY_PHYS_MMIO_EX_F_MMIO2)
    {
        uint32_t const idSlot = *puNemRange;
        AssertReturn(idSlot > 0 && idSlot < _32K, VERR_NEM_IPE_4);
        AssertReturn(ASMBitTest(pVM->nem.s.bmSlotIds, idSlot), VERR_NEM_IPE_4);

        struct kvm_userspace_memory_region Region;
        Region.slot             = idSlot;
        Region.flags            = 0;
        Region.guest_phys_addr  = GCPhys;
        Region.memory_size      = 0;    /* this deregisters it. */
        Region.userspace_addr   = (uintptr_t)pvMmio2;

        int rc = ioctl(pVM->nem.s.fdVm, KVM_SET_USER_MEMORY_REGION, &Region);
        if (rc == 0)
        {
            if (pu2State)
                *pu2State = 0;
            *puNemRange = UINT32_MAX;
            nemR3LnxMemSlotIdFree(pVM, idSlot);
            return VINF_SUCCESS;
        }

        AssertLogRelMsgFailedReturn(("%RGp LB %RGp fFlags=%#x, pvMmio2=%p, idSlot=%#x failed: %u/%u\n",
                                     GCPhys, cb, fFlags, pvMmio2, idSlot, errno, rc),
                                    VERR_NEM_UNMAP_PAGES_FAILED);
    }

    if (pu2State)
        *pu2State = UINT8_MAX;
    return VINF_SUCCESS;
}


VMMR3_INT_DECL(int) NEMR3PhysMmio2QueryAndResetDirtyBitmap(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, uint32_t uNemRange,
                                                           void *pvBitmap, size_t cbBitmap)
{
    AssertReturn(uNemRange > 0 && uNemRange < _32K, VERR_NEM_IPE_4);
    AssertReturn(ASMBitTest(pVM->nem.s.bmSlotIds, uNemRange), VERR_NEM_IPE_4);

    RT_NOREF(GCPhys, cbBitmap);

    struct kvm_dirty_log DirtyLog;
    DirtyLog.slot         = uNemRange;
    DirtyLog.padding1     = 0;
    DirtyLog.dirty_bitmap = pvBitmap;

    int rc = ioctl(pVM->nem.s.fdVm, KVM_GET_DIRTY_LOG, &DirtyLog);
    AssertLogRelMsgReturn(rc == 0, ("%RGp LB %RGp idSlot=%#x failed: %u/%u\n", GCPhys, cb, uNemRange, errno, rc),
                          VERR_NEM_QUERY_DIRTY_BITMAP_FAILED);

    return VINF_SUCCESS;
}


VMMR3_INT_DECL(int)  NEMR3NotifyPhysRomRegisterEarly(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, void *pvPages, uint32_t fFlags,
                                                     uint8_t *pu2State, uint32_t *puNemRange)
{
    Log5(("NEMR3NotifyPhysRomRegisterEarly: %RGp LB %RGp pvPages=%p fFlags=%#x\n", GCPhys, cb, pvPages, fFlags));
    *pu2State = UINT8_MAX;

    /* Don't support puttint ROM where there is already RAM.  For
       now just shuffle the registrations till it works... */
    AssertLogRelMsgReturn(!(fFlags & NEM_NOTIFY_PHYS_ROM_F_REPLACE), ("%RGp LB %RGp fFlags=%#x\n", GCPhys, cb, fFlags),
                          VERR_NEM_MAP_PAGES_FAILED);

    /** @todo figure out how to do shadow ROMs.   */

    /*
     * We only allocate a slot number here in case we need to use it to
     * fend of physical handler fun.
     */
    uint16_t idSlot = nemR3LnxMemSlotIdAlloc(pVM);
    AssertLogRelReturn(idSlot < _32K, VERR_NEM_MAP_PAGES_FAILED);

    *pu2State   = 0;
    *puNemRange = idSlot;
    Log5(("NEMR3NotifyPhysRomRegisterEarly: %RGp LB %RGp fFlags=%#x pvPages=%p - idSlot=%#x\n",
          GCPhys, cb, fFlags, pvPages, idSlot));
    RT_NOREF(GCPhys, cb, fFlags, pvPages);
    return VINF_SUCCESS;
}


VMMR3_INT_DECL(int)  NEMR3NotifyPhysRomRegisterLate(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, void *pvPages,
                                                    uint32_t fFlags, uint8_t *pu2State, uint32_t *puNemRange)
{
    Log5(("NEMR3NotifyPhysRomRegisterLate: %RGp LB %RGp pvPages=%p fFlags=%#x pu2State=%p (%d) puNemRange=%p (%#x)\n",
          GCPhys, cb, pvPages, fFlags, pu2State, *pu2State, puNemRange, *puNemRange));

    AssertPtrReturn(pvPages, VERR_NEM_IPE_5);

    uint32_t const idSlot = *puNemRange;
    AssertReturn(idSlot > 0 && idSlot < _32K, VERR_NEM_IPE_4);
    AssertReturn(ASMBitTest(pVM->nem.s.bmSlotIds, idSlot), VERR_NEM_IPE_4);

    *pu2State = UINT8_MAX;

    /*
     * Do the actual setting of the user pages here now that we've
     * got a valid pvPages (typically isn't available during the early
     * notification, unless we're replacing RAM).
     */
    struct kvm_userspace_memory_region Region;
    Region.slot             = idSlot;
    Region.flags            = 0;
    Region.guest_phys_addr  = GCPhys;
    Region.memory_size      = cb;
    Region.userspace_addr   = (uintptr_t)pvPages;

    int rc = ioctl(pVM->nem.s.fdVm, KVM_SET_USER_MEMORY_REGION, &Region);
    if (rc == 0)
    {
        *pu2State   = 0;
        Log5(("NEMR3NotifyPhysRomRegisterEarly: %RGp LB %RGp fFlags=%#x pvPages=%p - idSlot=%#x\n",
              GCPhys, cb, fFlags, pvPages, idSlot));
        return VINF_SUCCESS;
    }
    AssertLogRelMsgFailedReturn(("%RGp LB %RGp fFlags=%#x, pvPages=%p, idSlot=%#x failed: %u/%u\n",
                                 GCPhys, cb, fFlags, pvPages, idSlot, errno, rc),
                                VERR_NEM_MAP_PAGES_FAILED);
}


VMMR3_INT_DECL(int) NEMR3LoadExec(PVM pVM)
{
    // TODO: this code leaves a small window between the guest sending an INIT IPI
    // and a subsequent SIPI IPI. If that's the case, we need to set the MP state
    // `KVM_MP_STATE_INIT_RECEIVED` which requires some serious interaction
    // between the NEM and SSM. For now, we hope that noone suspends a VM during
    // VCPU bringup. See vbox-engineering#426.
    for (VMCPUID i = 0; i < pVM->cCpus; i++) {
        PVMCPU pVCpu = pVM->apCpusR3[i];
        auto state = VMCPU_GET_STATE(pVCpu);
        if (state == VMCPUSTATE_STARTED || state == VMCPUSTATE_STARTED_EXEC_NEM || state == VMCPUSTATE_STARTED_EXEC_NEM_WAIT )
        {
            struct kvm_mp_state mp;
            mp.mp_state = KVM_MP_STATE_RUNNABLE;
            int rcLnx = ioctl(pVCpu->nem.s.fdVCpu, KVM_SET_MP_STATE, &mp);
            AssertLogRelMsgReturn(rcLnx == 0, ("NEMR3Load: Failed to set MP state. Error: %d, errno %d\n", rcLnx, errno), VERR_NEM_IPE_5);
        }
    }
    return VINF_SUCCESS;
}

VMMR3_INT_DECL(int) NEMR3KvmGetMsr(PVMCPU pVCpu, uint64_t msr, uint64_t* val)
{
    alignas(struct kvm_msrs) char backing[sizeof(struct kvm_msrs) + sizeof(struct kvm_msr_entry)];
    struct kvm_msrs* msr_data {reinterpret_cast<struct kvm_msrs*>(&backing[0])};
    RT_ZERO(backing);

    msr_data->nmsrs = 1;
    msr_data->entries[0].index = msr;

    int rcLnx = ioctl(pVCpu->nem.s.fdVCpu, KVM_GET_MSRS, msr_data);
    AssertLogRelMsgReturn(rcLnx == 1, ("NEMR3KvmGetMsr: \
                Failed to get MSR data. Error: %d, errno %d\n", rcLnx, errno), VERR_NOT_SUPPORTED);

    AssertLogRelMsgReturn(val != nullptr, ("NEMR3KvmGetMsr: \
                Invalid buffer\n", rcLnx, errno), VERR_NEM_IPE_5);

    *val = msr_data->entries[0].data;

    return VINF_SUCCESS;
}

VMMR3_INT_DECL(int) NEMR3KvmSetMsr(PVMCPU pVCpu, uint64_t msr, uint64_t val)
{
    alignas(struct kvm_msrs) char backing[sizeof(struct kvm_msrs) + sizeof(struct kvm_msr_entry)];
    struct kvm_msrs* msr_data {reinterpret_cast<struct kvm_msrs*>(&backing[0])};
    RT_ZERO(backing);

    msr_data->nmsrs = 1;
    msr_data->entries[0].index = msr;
    msr_data->entries[0].data = val;

    int rcLnx = ioctl(pVCpu->nem.s.fdVCpu, KVM_SET_MSRS, msr_data);
    AssertLogRelMsgReturn(rcLnx == 1, ("NEMR3KvmSetMsr: \
                Failed to set MSR[%lx] data. Error: %d, errno %d\n", msr, rcLnx, errno), VERR_NOT_SUPPORTED);

    return VINF_SUCCESS;
}

VMMR3_INT_DECL(int) NEMR3KvmGetLapicState(PVMCPU pVCpu, void* pXApicPage)
{
    struct kvm_lapic_state state;

    int rcLnx = ioctl(pVCpu->nem.s.fdVCpu, KVM_GET_LAPIC, &state);
    AssertLogRelMsgReturn(rcLnx == 0, ("NEMR3KvmGetLapicState: \
                Failed to get APIC state. Error: %d, errno %d\n", rcLnx, errno), VERR_NEM_IPE_5);

    memcpy(pXApicPage, &state.regs[0], KVM_APIC_REG_SIZE);
    return VINF_SUCCESS;
}

VMMR3_INT_DECL(int) NEMR3KvmSetLapicState(PVMCPU pVCpu, void* pXApicPage)
{
    struct kvm_lapic_state state;

    memcpy(&state.regs[0], pXApicPage, KVM_APIC_REG_SIZE);

    int rcLnx = ioctl(pVCpu->nem.s.fdVCpu, KVM_SET_LAPIC, &state);
    AssertLogRelMsgReturn(rcLnx == 0, ("NEMR3KvmSetApicState: \
                Failed to set APIC state. Error %d, errno %d\n", rcLnx, errno), VERR_NEM_IPE_5);

    return VINF_SUCCESS;
}

VMMR3_INT_DECL(int) NEMR3KvmSetIrqLine(PVM pVM, uint16_t u16Gsi, int iLevel)
{
    struct kvm_irq_level irq;
    RT_ZERO(irq);

    irq.irq = u16Gsi;
    irq.level = iLevel;

    int rcLnx = ioctl(pVM->nem.s.fdVm, KVM_IRQ_LINE, &irq);
    AssertLogRelMsgReturn(rcLnx == 0, ("NEMR3KvmSetIrqLine: Failed to set irq line %d! error: %d, errno %d\n", u16Gsi, rcLnx, errno), VERR_NEM_IPE_5);

    return VINF_SUCCESS;
}

VMMR3_INT_DECL(int) NEMR3KvmSplitIrqchipDeliverMsi(PVM pVM, PCMSIMSG pMsi)
{
    AssertLogRelReturn(pVM != nullptr, VERR_INVALID_POINTER);
    AssertLogRelReturn(pMsi != nullptr, VERR_INVALID_POINTER);

    struct kvm_msi msi;
    RT_ZERO(msi);
    msi.address_lo = pMsi->Addr.au32[0];
    msi.address_hi = pMsi->Addr.au32[1];
    msi.data = pMsi->Data.u32;

    int rcLnx = ioctl(pVM->nem.s.fdVm, KVM_SIGNAL_MSI, &msi);
    AssertLogRelMsgReturn(rcLnx >= 0, ("NEMR3KvmSplitIrqchipDeliverMsi: Failed to deliver MSI! error: %d, errno %d\n", rcLnx, errno), VERR_NEM_IPE_5);

    return rcLnx == 0 ? VERR_APIC_INTR_DISCARDED : VINF_SUCCESS;
}

#ifdef VBOX_WITH_KVM_IRQCHIP_FULL
static int kvmSetGsiRoutingFullIrqChip(PVM pVM)
{
    alignas(kvm_irq_routing) char backing[ sizeof(struct kvm_irq_routing) +
        (KVM_IRQCHIP_NUM_IOAPIC_INTR_PINS + KVM_IRQCHIP_NUM_PIC_INTR_PINS) * sizeof(struct kvm_irq_routing_entry) ] {};
    kvm_irq_routing* routing = reinterpret_cast<kvm_irq_routing*>(backing);

    for (unsigned i = 0; i < KVM_IRQCHIP_NUM_PIC_INTR_PINS; ++i) {
        routing->entries[i].gsi = i;
        routing->entries[i].type = KVM_IRQ_ROUTING_IRQCHIP;
        routing->entries[i].u.irqchip.irqchip = (i < 8) ? KVM_IRQCHIP_PIC_MASTER : KVM_IRQCHIP_PIC_SLAVE;
        routing->entries[i].u.irqchip.pin = (i < 8) ? i : (i - 8);
    }

    for (unsigned i = 0; i < KVM_IRQCHIP_NUM_IOAPIC_INTR_PINS; ++i) {
        uint64_t arr_idx = i + KVM_IRQCHIP_NUM_PIC_INTR_PINS;
        routing->entries[arr_idx].gsi = i;
        routing->entries[arr_idx].type = KVM_IRQ_ROUTING_IRQCHIP;
        routing->entries[arr_idx].u.irqchip.irqchip = KVM_IRQCHIP_IOAPIC;
        if (i == 0) {
            routing->entries[arr_idx].u.irqchip.pin = 2;
        } else {
            routing->entries[arr_idx].u.irqchip.pin = i;
        }
    }
    routing->nr = KVM_IRQCHIP_NUM_PIC_INTR_PINS + KVM_IRQCHIP_NUM_IOAPIC_INTR_PINS;

    int rc = ioctl(pVM->nem.s.fdVm, KVM_SET_GSI_ROUTING, routing);

    AssertLogRelMsgReturn(rc >= 0, ("NEM/KVM: Unable to set GSI routing! rc: %d errno %d \n", rc, errno), VERR_INTERNAL_ERROR);

    return VINF_SUCCESS;
}

VMMR3_INT_DECL(int) NEMR3KvmGetPicState(PVM pVM, KVMIRQCHIP irqchip, KVMPICSTATE* state)
{
    struct kvm_irqchip irqchip_state;
    irqchip_state.chip_id = irqchip == KVMIRQCHIP::PIC_MASTER ? KVM_IRQCHIP_PIC_MASTER : KVM_IRQCHIP_PIC_SLAVE;

    if (state == nullptr) {
        return VERR_INVALID_POINTER;
    }

    int rcLnx = ioctl(pVM->nem.s.fdVm, KVM_GET_IRQCHIP, &irqchip_state);
    AssertLogRelMsgReturn(rcLnx == 0, ("NEMR3KvmGetPicState: \
                Failed to get PIC state. Error: %d, errno %d\n", rcLnx, errno), VERR_NEM_IPE_5);

    state->last_irr = irqchip_state.chip.pic.last_irr;
    state->irr = irqchip_state.chip.pic.irr;
    state->imr = irqchip_state.chip.pic.imr;
    state->isr = irqchip_state.chip.pic.isr;
    state->priority_add = irqchip_state.chip.pic.priority_add;
    state->irq_base = irqchip_state.chip.pic.irq_base;
    state->read_reg_select = irqchip_state.chip.pic.read_reg_select;
    state->poll = irqchip_state.chip.pic.poll;
    state->special_mask = irqchip_state.chip.pic.special_mask;
    state->init_state = irqchip_state.chip.pic.init_state;
    state->auto_eoi = irqchip_state.chip.pic.auto_eoi;
    state->rotate_on_auto_eoi = irqchip_state.chip.pic.rotate_on_auto_eoi;
    state->special_fully_nested_mode = irqchip_state.chip.pic.special_fully_nested_mode;
    state->init4 = irqchip_state.chip.pic.init4;
    state->elcr = irqchip_state.chip.pic.elcr;
    state->elcr_mask = irqchip_state.chip.pic.elcr_mask;

    return VINF_SUCCESS;
}

VMMR3_INT_DECL(int) NEMR3KvmSetPicState(PVM pVM, KVMIRQCHIP irqchip, KVMPICSTATE* state)
{
    struct kvm_irqchip irqchip_state;
    irqchip_state.chip_id = irqchip == KVMIRQCHIP::PIC_MASTER ? KVM_IRQCHIP_PIC_MASTER : KVM_IRQCHIP_PIC_SLAVE;

    if (state == nullptr) {
        return VERR_INVALID_POINTER;
    }

    irqchip_state.chip.pic.last_irr = state->last_irr;
    irqchip_state.chip.pic.irr = state->irr;
    irqchip_state.chip.pic.imr = state->imr;
    irqchip_state.chip.pic.isr = state->isr;
    irqchip_state.chip.pic.priority_add = state->priority_add;
    irqchip_state.chip.pic.irq_base = state->irq_base;
    irqchip_state.chip.pic.read_reg_select = state->read_reg_select;
    irqchip_state.chip.pic.poll = state->poll;
    irqchip_state.chip.pic.special_mask = state->special_mask;
    irqchip_state.chip.pic.init_state = state->init_state;
    irqchip_state.chip.pic.auto_eoi = state->auto_eoi;
    irqchip_state.chip.pic.rotate_on_auto_eoi = state->rotate_on_auto_eoi;
    irqchip_state.chip.pic.special_fully_nested_mode = state->special_fully_nested_mode;
    irqchip_state.chip.pic.init4 = state->init4;
    irqchip_state.chip.pic.elcr = state->elcr;
    irqchip_state.chip.pic.elcr_mask = state->elcr_mask;

    int rcLnx = ioctl(pVM->nem.s.fdVm, KVM_GET_IRQCHIP, &irqchip_state);
    AssertLogRelMsgReturn(rcLnx == 0, ("NEMR3KvmSetPicState: \
                Failed to get PIC state. Error: %d, errno %d\n", rcLnx, errno), VERR_NEM_IPE_5);

    return VINF_SUCCESS;
}

VMMR3_INT_DECL(int) NEMR3KvmGetIoApicState(PVM pVM, KVMIOAPICSTATE* state)
{
    struct kvm_irqchip irqchip_state;
    irqchip_state.chip_id = KVM_IRQCHIP_IOAPIC;

    if (state == nullptr) {
        return VERR_INVALID_POINTER;
    }

    int rcLnx = ioctl(pVM->nem.s.fdVm, KVM_GET_IRQCHIP, &irqchip_state);
    AssertLogRelMsgReturn(rcLnx == 0, ("NEMR3KvmGetIoApicState: \
                Failed to get IOAPIC state. Error: %d, errno %d\n", rcLnx, errno), VERR_NEM_IPE_5);

    state->base_address = irqchip_state.chip.ioapic.base_address;
    state->ioregsel = irqchip_state.chip.ioapic.ioregsel;
    state->id = irqchip_state.chip.ioapic.id;
    state->irr = irqchip_state.chip.ioapic.irr;

    for (unsigned i = 0; i < KVM_IRQCHIP_NUM_IOAPIC_INTR_PINS; ++i) {
        state->redirtbl[i] = irqchip_state.chip.ioapic.redirtbl[i].bits;
    }

    return VINF_SUCCESS;
}

VMMR3_INT_DECL(int) NEMR3KvmSetIoApicState(PVM pVM, KVMIOAPICSTATE* state)
{
    struct kvm_irqchip irqchip_state;
    irqchip_state.chip_id = KVM_IRQCHIP_IOAPIC;

    if (state == nullptr) {
        return VERR_INVALID_POINTER;
    }

    irqchip_state.chip.ioapic.base_address = state->base_address;
    irqchip_state.chip.ioapic.ioregsel = state->ioregsel;
    irqchip_state.chip.ioapic.id = state->id;
    irqchip_state.chip.ioapic.irr = state->irr;

    for (unsigned i = 0; i < KVM_IRQCHIP_NUM_IOAPIC_INTR_PINS; ++i) {
        irqchip_state.chip.ioapic.redirtbl[i].bits = state->redirtbl[i];
    }

    int rcLnx = ioctl(pVM->nem.s.fdVm, KVM_SET_IRQCHIP, &irqchip_state);
    AssertLogRelMsgReturn(rcLnx == 0, ("NEMR3KvmSetIoApicState: \
                Failed to set IOPIC state. Error: %d, errno %d\n", rcLnx, errno), VERR_NEM_IPE_5);

    return VINF_SUCCESS;
}
#endif

static int kvmSetGsiRouting(PVM pVM)
{
    alignas(kvm_irq_routing) char backing[ sizeof(struct kvm_irq_routing) + KVM_IRQCHIP_NUM_IOAPIC_INTR_PINS * sizeof(struct kvm_irq_routing_entry) ] {};
    kvm_irq_routing* routing = reinterpret_cast<kvm_irq_routing*>(backing);

    unsigned routingCount {0};

    for(unsigned i {0}; i < KVM_IRQCHIP_NUM_IOAPIC_INTR_PINS; ++i)
    {
        if (pVM->nem.s.pARedirectionTable->at(i).has_value())
        {
            PMSIMSG msi = &(pVM->nem.s.pARedirectionTable->at(i).value());
            routing->entries[routingCount].gsi = i;
            routing->entries[routingCount].type = KVM_IRQ_ROUTING_MSI;
            routing->entries[routingCount].u.msi.address_lo = msi->Addr.au32[0];
            routing->entries[routingCount].u.msi.address_hi = msi->Addr.au32[1];
            routing->entries[routingCount].u.msi.data = msi->Data.u32;
            routingCount++;
        }
    }

    routing->nr = routingCount;

    int rc = ioctl(pVM->nem.s.fdVm, KVM_SET_GSI_ROUTING, routing);

    AssertLogRelMsgReturn(rc >= 0, ("NEM/KVM: Unable to set GSI routing! rc: %d errno %d \n", rc, errno), VERR_INTERNAL_ERROR);

    return VINF_SUCCESS;
}


VMMR3_INT_DECL(int) NEMR3KvmSplitIrqchipAddUpdateRTE(PVM pVM, uint16_t u16Gsi, PCMSIMSG pMsi)
{
    AssertRelease(pVM->nem.s.pARedirectionTable != nullptr);
    AssertRelease(u16Gsi < KVM_IRQCHIP_NUM_IOAPIC_INTR_PINS);

    pVM->nem.s.pARedirectionTable->at(u16Gsi) = *pMsi;

    return kvmSetGsiRouting(pVM);
}


VMMR3_INT_DECL(int) NEMR3KvmSplitIrqchipRemoveRTE(PVM pVM, uint16_t u16Gsi)
{
    AssertRelease(pVM->nem.s.pARedirectionTable != nullptr);
    AssertRelease(u16Gsi < KVM_IRQCHIP_NUM_IOAPIC_INTR_PINS);

    pVM->nem.s.pARedirectionTable->at(u16Gsi) = std::nullopt;

    return kvmSetGsiRouting(pVM);
}


VMMR3_INT_DECL(void) NEMR3NotifySetA20(PVMCPU pVCpu, bool fEnabled)
{
    Log(("nemR3NativeNotifySetA20: fEnabled=%RTbool\n", fEnabled));
    Assert(VM_IS_NEM_ENABLED(pVCpu->CTX_SUFF(pVM)));
    RT_NOREF(pVCpu, fEnabled);
}


VMM_INT_DECL(void) NEMHCNotifyHandlerPhysicalDeregister(PVMCC pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb,
                                                        RTR3PTR pvMemR3, uint8_t *pu2State)
{
    Log5(("NEMHCNotifyHandlerPhysicalDeregister: %RGp LB %RGp enmKind=%d pvMemR3=%p pu2State=%p (%d)\n",
          GCPhys, cb, enmKind, pvMemR3, pu2State, *pu2State));

    *pu2State = UINT8_MAX;
    RT_NOREF(pVM, enmKind, GCPhys, cb, pvMemR3);
}


void nemHCNativeNotifyHandlerPhysicalRegister(PVMCC pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb)
{
    Log5(("nemHCNativeNotifyHandlerPhysicalRegister: %RGp LB %RGp enmKind=%d\n", GCPhys, cb, enmKind));
    RT_NOREF(pVM, enmKind, GCPhys, cb);
}


void nemHCNativeNotifyHandlerPhysicalModify(PVMCC pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhysOld,
                                            RTGCPHYS GCPhysNew, RTGCPHYS cb, bool fRestoreAsRAM)
{
    Log5(("nemHCNativeNotifyHandlerPhysicalModify: %RGp LB %RGp -> %RGp enmKind=%d fRestoreAsRAM=%d\n",
          GCPhysOld, cb, GCPhysNew, enmKind, fRestoreAsRAM));
    RT_NOREF(pVM, enmKind, GCPhysOld, GCPhysNew, cb, fRestoreAsRAM);
}


int nemHCNativeNotifyPhysPageAllocated(PVMCC pVM, RTGCPHYS GCPhys, RTHCPHYS HCPhys, uint32_t fPageProt,
                                       PGMPAGETYPE enmType, uint8_t *pu2State)
{
    Log5(("nemHCNativeNotifyPhysPageAllocated: %RGp HCPhys=%RHp fPageProt=%#x enmType=%d *pu2State=%d\n",
          GCPhys, HCPhys, fPageProt, enmType, *pu2State));
    RT_NOREF(pVM, GCPhys, HCPhys, fPageProt, enmType, pu2State);
    return VINF_SUCCESS;
}


VMM_INT_DECL(void) NEMHCNotifyPhysPageProtChanged(PVMCC pVM, RTGCPHYS GCPhys, RTHCPHYS HCPhys, RTR3PTR pvR3, uint32_t fPageProt,
                                                  PGMPAGETYPE enmType, uint8_t *pu2State)
{
    Log5(("NEMHCNotifyPhysPageProtChanged: %RGp HCPhys=%RHp fPageProt=%#x enmType=%d *pu2State=%d\n",
          GCPhys, HCPhys, fPageProt, enmType, *pu2State));
    Assert(VM_IS_NEM_ENABLED(pVM));
    RT_NOREF(pVM, GCPhys, HCPhys, pvR3, fPageProt, enmType, pu2State);

}


VMM_INT_DECL(void) NEMHCNotifyPhysPageChanged(PVMCC pVM, RTGCPHYS GCPhys, RTHCPHYS HCPhysPrev, RTHCPHYS HCPhysNew,
                                              RTR3PTR pvNewR3, uint32_t fPageProt, PGMPAGETYPE enmType, uint8_t *pu2State)
{
    Log5(("nemHCNativeNotifyPhysPageChanged: %RGp HCPhys=%RHp->%RHp pvNewR3=%p fPageProt=%#x enmType=%d *pu2State=%d\n",
          GCPhys, HCPhysPrev, HCPhysNew, pvNewR3, fPageProt, enmType, *pu2State));
    Assert(VM_IS_NEM_ENABLED(pVM));
    RT_NOREF(pVM, GCPhys, HCPhysPrev, HCPhysNew, pvNewR3, fPageProt, enmType, pu2State);
}


/*********************************************************************************************************************************
*   CPU State                                                                                                                    *
*********************************************************************************************************************************/

/**
 * Worker that imports selected state from KVM.
 */
static int nemHCLnxImportState(PVMCPUCC pVCpu, uint64_t fWhat, PCPUMCTX pCtx, struct kvm_run *pRun)
{
    fWhat &= pVCpu->cpum.GstCtx.fExtrn;
    if (!fWhat)
        return VINF_SUCCESS;

    /*
     * Stuff that goes into kvm_run::s.regs.regs:
     */
    if (fWhat & (CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_RFLAGS | CPUMCTX_EXTRN_GPRS_MASK))
    {
        if (fWhat & CPUMCTX_EXTRN_RIP)
            pCtx->rip       = pRun->s.regs.regs.rip;
        if (fWhat & CPUMCTX_EXTRN_RFLAGS)
            pCtx->rflags.u  = pRun->s.regs.regs.rflags;

        if (fWhat & CPUMCTX_EXTRN_RAX)
            pCtx->rax       = pRun->s.regs.regs.rax;
        if (fWhat & CPUMCTX_EXTRN_RCX)
            pCtx->rcx       = pRun->s.regs.regs.rcx;
        if (fWhat & CPUMCTX_EXTRN_RDX)
            pCtx->rdx       = pRun->s.regs.regs.rdx;
        if (fWhat & CPUMCTX_EXTRN_RBX)
            pCtx->rbx       = pRun->s.regs.regs.rbx;
        if (fWhat & CPUMCTX_EXTRN_RSP)
            pCtx->rsp       = pRun->s.regs.regs.rsp;
        if (fWhat & CPUMCTX_EXTRN_RBP)
            pCtx->rbp       = pRun->s.regs.regs.rbp;
        if (fWhat & CPUMCTX_EXTRN_RSI)
            pCtx->rsi       = pRun->s.regs.regs.rsi;
        if (fWhat & CPUMCTX_EXTRN_RDI)
            pCtx->rdi       = pRun->s.regs.regs.rdi;
        if (fWhat & CPUMCTX_EXTRN_R8_R15)
        {
            pCtx->r8        = pRun->s.regs.regs.r8;
            pCtx->r9        = pRun->s.regs.regs.r9;
            pCtx->r10       = pRun->s.regs.regs.r10;
            pCtx->r11       = pRun->s.regs.regs.r11;
            pCtx->r12       = pRun->s.regs.regs.r12;
            pCtx->r13       = pRun->s.regs.regs.r13;
            pCtx->r14       = pRun->s.regs.regs.r14;
            pCtx->r15       = pRun->s.regs.regs.r15;
        }
    }

    /*
     * Stuff that goes into kvm_run::s.regs.sregs.
     *
     * Note! The apic_base can be ignored because we gets all MSR writes to it
     *       and VBox always keeps the correct value.
     */
    bool fMaybeChangedMode = false;
    bool fUpdateCr3        = false;
    if (fWhat & (  CPUMCTX_EXTRN_SREG_MASK | CPUMCTX_EXTRN_TABLE_MASK | CPUMCTX_EXTRN_CR_MASK
                 | CPUMCTX_EXTRN_EFER      | CPUMCTX_EXTRN_APIC_TPR))
    {
        /** @todo what about Attr.n.u4LimitHigh?   */
#define NEM_LNX_IMPORT_SEG(a_CtxSeg, a_KvmSeg) do { \
            (a_CtxSeg).u64Base              = (a_KvmSeg).base; \
            (a_CtxSeg).u32Limit             = (a_KvmSeg).limit; \
            (a_CtxSeg).ValidSel = (a_CtxSeg).Sel = (a_KvmSeg).selector; \
            (a_CtxSeg).Attr.n.u4Type        = (a_KvmSeg).type; \
            (a_CtxSeg).Attr.n.u1DescType    = (a_KvmSeg).s; \
            (a_CtxSeg).Attr.n.u2Dpl         = (a_KvmSeg).dpl; \
            (a_CtxSeg).Attr.n.u1Present     = (a_KvmSeg).present; \
            (a_CtxSeg).Attr.n.u1Available   = (a_KvmSeg).avl; \
            (a_CtxSeg).Attr.n.u1Long        = (a_KvmSeg).l; \
            (a_CtxSeg).Attr.n.u1DefBig      = (a_KvmSeg).db; \
            (a_CtxSeg).Attr.n.u1Granularity = (a_KvmSeg).g; \
            (a_CtxSeg).Attr.n.u1Unusable    = (a_KvmSeg).unusable; \
            (a_CtxSeg).fFlags               = CPUMSELREG_FLAGS_VALID; \
            CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &(a_CtxSeg)); \
        } while (0)

        if (fWhat & CPUMCTX_EXTRN_SREG_MASK)
        {
            if (fWhat & CPUMCTX_EXTRN_ES)
                NEM_LNX_IMPORT_SEG(pCtx->es, pRun->s.regs.sregs.es);
            if (fWhat & CPUMCTX_EXTRN_CS)
                NEM_LNX_IMPORT_SEG(pCtx->cs, pRun->s.regs.sregs.cs);
            if (fWhat & CPUMCTX_EXTRN_SS)
                NEM_LNX_IMPORT_SEG(pCtx->ss, pRun->s.regs.sregs.ss);
            if (fWhat & CPUMCTX_EXTRN_DS)
                NEM_LNX_IMPORT_SEG(pCtx->ds, pRun->s.regs.sregs.ds);
            if (fWhat & CPUMCTX_EXTRN_FS)
                NEM_LNX_IMPORT_SEG(pCtx->fs, pRun->s.regs.sregs.fs);
            if (fWhat & CPUMCTX_EXTRN_GS)
                NEM_LNX_IMPORT_SEG(pCtx->gs, pRun->s.regs.sregs.gs);
        }
        if (fWhat & CPUMCTX_EXTRN_TABLE_MASK)
        {
            if (fWhat & CPUMCTX_EXTRN_GDTR)
            {
                pCtx->gdtr.pGdt     = pRun->s.regs.sregs.gdt.base;
                pCtx->gdtr.cbGdt    = pRun->s.regs.sregs.gdt.limit;
            }
            if (fWhat & CPUMCTX_EXTRN_IDTR)
            {
                pCtx->idtr.pIdt     = pRun->s.regs.sregs.idt.base;
                pCtx->idtr.cbIdt    = pRun->s.regs.sregs.idt.limit;
            }
            if (fWhat & CPUMCTX_EXTRN_LDTR)
                NEM_LNX_IMPORT_SEG(pCtx->ldtr, pRun->s.regs.sregs.ldt);
            if (fWhat & CPUMCTX_EXTRN_TR)
                NEM_LNX_IMPORT_SEG(pCtx->tr, pRun->s.regs.sregs.tr);
        }
        if (fWhat & CPUMCTX_EXTRN_CR_MASK)
        {
            if (fWhat & CPUMCTX_EXTRN_CR0)
            {
                if (pVCpu->cpum.GstCtx.cr0 != pRun->s.regs.sregs.cr0)
                {
                    CPUMSetGuestCR0(pVCpu, pRun->s.regs.sregs.cr0);
                    fMaybeChangedMode = true;
                }
            }
            if (fWhat & CPUMCTX_EXTRN_CR2)
                pCtx->cr2              = pRun->s.regs.sregs.cr2;
            if (fWhat & CPUMCTX_EXTRN_CR3)
            {
                if (pCtx->cr3 != pRun->s.regs.sregs.cr3)
                {
                    CPUMSetGuestCR3(pVCpu, pRun->s.regs.sregs.cr3);
                    fUpdateCr3 = true;
                }
            }
            if (fWhat & CPUMCTX_EXTRN_CR4)
            {
                if (pCtx->cr4 != pRun->s.regs.sregs.cr4)
                {
                    CPUMSetGuestCR4(pVCpu, pRun->s.regs.sregs.cr4);
                    fMaybeChangedMode = true;
                }
            }
        }

        if (fWhat & CPUMCTX_EXTRN_EFER)
        {
            if (pCtx->msrEFER != pRun->s.regs.sregs.efer)
            {
                Log7(("NEM/%u: MSR EFER changed %RX64 -> %RX64\n", pVCpu->idCpu,  pVCpu->cpum.GstCtx.msrEFER, pRun->s.regs.sregs.efer));
                if ((pRun->s.regs.sregs.efer ^ pVCpu->cpum.GstCtx.msrEFER) & MSR_K6_EFER_NXE)
                    PGMNotifyNxeChanged(pVCpu, RT_BOOL(pRun->s.regs.sregs.efer & MSR_K6_EFER_NXE));
                pCtx->msrEFER = pRun->s.regs.sregs.efer;
                fMaybeChangedMode = true;
            }
        }
#undef NEM_LNX_IMPORT_SEG
    }

    /*
     * Debug registers.
     */
    if (fWhat & CPUMCTX_EXTRN_DR_MASK)
    {
        struct kvm_debugregs DbgRegs = {{0}};
        int rc = ioctl(pVCpu->nem.s.fdVCpu, KVM_GET_DEBUGREGS, &DbgRegs);
        AssertMsgReturn(rc == 0, ("rc=%d errno=%d\n", rc, errno), VERR_NEM_IPE_3);

        if (fWhat & CPUMCTX_EXTRN_DR0_DR3)
        {
            pCtx->dr[0] = DbgRegs.db[0];
            pCtx->dr[1] = DbgRegs.db[1];
            pCtx->dr[2] = DbgRegs.db[2];
            pCtx->dr[3] = DbgRegs.db[3];
        }
        if (fWhat & CPUMCTX_EXTRN_DR6)
            pCtx->dr[6] = DbgRegs.dr6;
        if (fWhat & CPUMCTX_EXTRN_DR7)
            pCtx->dr[7] = DbgRegs.dr7;
    }

    /*
     * FPU, SSE, AVX, ++.
     */
    if (fWhat & (CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX | CPUMCTX_EXTRN_OTHER_XSAVE | CPUMCTX_EXTRN_XCRx))
    {
        if (fWhat & (CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX | CPUMCTX_EXTRN_OTHER_XSAVE))
        {
            fWhat |= CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX | CPUMCTX_EXTRN_OTHER_XSAVE; /* we do all or nothing at all */

            AssertCompile(sizeof(pCtx->XState) >= sizeof(struct kvm_xsave));
            int rc = ioctl(pVCpu->nem.s.fdVCpu, KVM_GET_XSAVE, &pCtx->XState);
            AssertMsgReturn(rc == 0, ("rc=%d errno=%d\n", rc, errno), VERR_NEM_IPE_3);
        }

        if (fWhat & CPUMCTX_EXTRN_XCRx)
        {
            struct kvm_xcrs Xcrs =
            {   /*.nr_xcrs = */ 2,
                /*.flags = */   0,
                /*.xcrs= */ {
                    { /*.xcr =*/ 0, /*.reserved=*/ 0, /*.value=*/ pCtx->aXcr[0] },
                    { /*.xcr =*/ 1, /*.reserved=*/ 0, /*.value=*/ pCtx->aXcr[1] },
                }
            };

            int rc = ioctl(pVCpu->nem.s.fdVCpu, KVM_GET_XCRS, &Xcrs);
            AssertMsgReturn(rc == 0, ("rc=%d errno=%d\n", rc, errno), VERR_NEM_IPE_3);

            pCtx->aXcr[0] = Xcrs.xcrs[0].value;
            pCtx->aXcr[1] = Xcrs.xcrs[1].value;
            pCtx->fXStateMask = Xcrs.xcrs[0].value;
        }
    }

    /*
     * MSRs.
     */
    if (fWhat & (  CPUMCTX_EXTRN_KERNEL_GS_BASE | CPUMCTX_EXTRN_SYSCALL_MSRS | CPUMCTX_EXTRN_SYSENTER_MSRS
                 | CPUMCTX_EXTRN_TSC_AUX        | CPUMCTX_EXTRN_OTHER_MSRS))
    {
        union
        {
            struct kvm_msrs Core;
            uint64_t padding[2 + sizeof(struct kvm_msr_entry) * 32];
        }                   uBuf;
        uint64_t           *pauDsts[32];
        uint32_t            iMsr        = 0;
        PCPUMCTXMSRS const  pCtxMsrs    = CPUMQueryGuestCtxMsrsPtr(pVCpu);

#define ADD_MSR(a_Msr, a_uValue) do { \
            Assert(iMsr < 32); \
            uBuf.Core.entries[iMsr].index    = (a_Msr); \
            uBuf.Core.entries[iMsr].reserved = 0; \
            uBuf.Core.entries[iMsr].data     = UINT64_MAX; \
            pauDsts[iMsr] = &(a_uValue); \
            iMsr += 1; \
        } while (0)

        if (fWhat & CPUMCTX_EXTRN_KERNEL_GS_BASE)
            ADD_MSR(MSR_K8_KERNEL_GS_BASE, pCtx->msrKERNELGSBASE);
        if (fWhat & CPUMCTX_EXTRN_SYSCALL_MSRS)
        {
            ADD_MSR(MSR_K6_STAR,    pCtx->msrSTAR);
            ADD_MSR(MSR_K8_LSTAR,   pCtx->msrLSTAR);
            ADD_MSR(MSR_K8_CSTAR,   pCtx->msrCSTAR);
            ADD_MSR(MSR_K8_SF_MASK, pCtx->msrSFMASK);
        }
        if (fWhat & CPUMCTX_EXTRN_SYSENTER_MSRS)
        {
            ADD_MSR(MSR_IA32_SYSENTER_CS,  pCtx->SysEnter.cs);
            ADD_MSR(MSR_IA32_SYSENTER_EIP, pCtx->SysEnter.eip);
            ADD_MSR(MSR_IA32_SYSENTER_ESP, pCtx->SysEnter.esp);
        }
        if (fWhat & CPUMCTX_EXTRN_TSC_AUX)
            ADD_MSR(MSR_K8_TSC_AUX, pCtxMsrs->msr.TscAux);
        if (fWhat & CPUMCTX_EXTRN_OTHER_MSRS)
        {
            ADD_MSR(MSR_IA32_CR_PAT, pCtx->msrPAT);
            /** @todo What do we _have_ to add here?
             * We also have: Mttr*, MiscEnable, FeatureControl. */
        }

        uBuf.Core.pad   = 0;
        uBuf.Core.nmsrs = iMsr;
        int rc = ioctl(pVCpu->nem.s.fdVCpu, KVM_GET_MSRS, &uBuf);
        AssertMsgReturn(rc == (int)iMsr,
                        ("rc=%d iMsr=%d (->%#x) errno=%d\n",
                         rc, iMsr, (uint32_t)rc < iMsr ? uBuf.Core.entries[rc].index : 0, errno),
                        VERR_NEM_IPE_3);

        while (iMsr-- > 0)
            *pauDsts[iMsr] = uBuf.Core.entries[iMsr].data;
#undef ADD_MSR
    }

    /*
     * Interruptibility state and pending interrupts.
     */
    if (fWhat & (CPUMCTX_EXTRN_INHIBIT_INT | CPUMCTX_EXTRN_INHIBIT_NMI))
    {
        fWhat |= CPUMCTX_EXTRN_INHIBIT_INT | CPUMCTX_EXTRN_INHIBIT_NMI; /* always do both, see export and interrupt FF handling */

        struct kvm_vcpu_events KvmEvents = {0};
        int rcLnx = ioctl(pVCpu->nem.s.fdVCpu, KVM_GET_VCPU_EVENTS, &KvmEvents);
        AssertLogRelMsgReturn(rcLnx == 0, ("rcLnx=%d errno=%d\n", rcLnx, errno), VERR_NEM_IPE_3);

        if (pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_RIP)
            pVCpu->cpum.GstCtx.rip = pRun->s.regs.regs.rip;

        CPUMUpdateInterruptShadowSsStiEx(&pVCpu->cpum.GstCtx,
                                         RT_BOOL(KvmEvents.interrupt.shadow & KVM_X86_SHADOW_INT_MOV_SS),
                                         RT_BOOL(KvmEvents.interrupt.shadow & KVM_X86_SHADOW_INT_STI),
                                         pVCpu->cpum.GstCtx.rip);
        CPUMUpdateInterruptInhibitingByNmi(&pVCpu->cpum.GstCtx, KvmEvents.nmi.masked != 0);

        Assert(KvmEvents.nmi.injected == 0);
        Assert(KvmEvents.nmi.pending  == 0);
    }

    /*
     * Update the external mask.
     */
    pCtx->fExtrn &= ~fWhat;
    pVCpu->cpum.GstCtx.fExtrn &= ~fWhat;
    if (!(pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_ALL))
        pVCpu->cpum.GstCtx.fExtrn = 0;

    /*
     * We sometimes need to update PGM on the guest status.
     */
    if (!fMaybeChangedMode && !fUpdateCr3)
    { /* likely */ }
    else
    {
        /*
         * Make sure we got all the state PGM might need.
         */
        Log7(("nemHCLnxImportState: fMaybeChangedMode=%d fUpdateCr3=%d fExtrnNeeded=%#RX64\n", fMaybeChangedMode, fUpdateCr3,
              pVCpu->cpum.GstCtx.fExtrn & (CPUMCTX_EXTRN_CR0 | CPUMCTX_EXTRN_CR4 | CPUMCTX_EXTRN_CR3 | CPUMCTX_EXTRN_EFER) ));
        if (pVCpu->cpum.GstCtx.fExtrn & (CPUMCTX_EXTRN_CR0 | CPUMCTX_EXTRN_CR4 | CPUMCTX_EXTRN_CR3 | CPUMCTX_EXTRN_EFER))
        {
            if (pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_CR0)
            {
                if (pVCpu->cpum.GstCtx.cr0 != pRun->s.regs.sregs.cr0)
                {
                    CPUMSetGuestCR0(pVCpu, pRun->s.regs.sregs.cr0);
                    fMaybeChangedMode = true;
                }
            }
            if (pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_CR3)
            {
                if (pCtx->cr3 != pRun->s.regs.sregs.cr3)
                {
                    CPUMSetGuestCR3(pVCpu, pRun->s.regs.sregs.cr3);
                    fUpdateCr3 = true;
                }
            }
            if (pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_CR4)
            {
                if (pCtx->cr4 != pRun->s.regs.sregs.cr4)
                {
                    CPUMSetGuestCR4(pVCpu, pRun->s.regs.sregs.cr4);
                    fMaybeChangedMode = true;
                }
            }
            if (fWhat & CPUMCTX_EXTRN_EFER)
            {
                if (pCtx->msrEFER != pRun->s.regs.sregs.efer)
                {
                    Log7(("NEM/%u: MSR EFER changed %RX64 -> %RX64\n", pVCpu->idCpu,  pVCpu->cpum.GstCtx.msrEFER, pRun->s.regs.sregs.efer));
                    if ((pRun->s.regs.sregs.efer ^ pVCpu->cpum.GstCtx.msrEFER) & MSR_K6_EFER_NXE)
                        PGMNotifyNxeChanged(pVCpu, RT_BOOL(pRun->s.regs.sregs.efer & MSR_K6_EFER_NXE));
                    pCtx->msrEFER = pRun->s.regs.sregs.efer;
                    fMaybeChangedMode = true;
                }
            }

            pVCpu->cpum.GstCtx.fExtrn &= ~(CPUMCTX_EXTRN_CR0 | CPUMCTX_EXTRN_CR4 | CPUMCTX_EXTRN_CR3 | CPUMCTX_EXTRN_EFER);
            if (!(pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_ALL))
                pVCpu->cpum.GstCtx.fExtrn = 0;
        }

        /*
         * Notify PGM about the changes.
         */
        if (fMaybeChangedMode)
        {
            int rc = PGMChangeMode(pVCpu, pVCpu->cpum.GstCtx.cr0, pVCpu->cpum.GstCtx.cr4,
                                   pVCpu->cpum.GstCtx.msrEFER, false /*fForce*/);
            AssertMsgReturn(rc == VINF_SUCCESS, ("rc=%Rrc\n", rc), RT_FAILURE_NP(rc) ? rc : VERR_NEM_IPE_1);
        }

        if (fUpdateCr3)
        {
            int rc = PGMUpdateCR3(pVCpu, pVCpu->cpum.GstCtx.cr3);
            if (rc == VINF_SUCCESS)
            { /* likely */ }
            else
                AssertMsgFailedReturn(("rc=%Rrc\n", rc), RT_FAILURE_NP(rc) ? rc : VERR_NEM_IPE_2);
        }
    }

    return VINF_SUCCESS;
}


/**
 * Interface for importing state on demand (used by IEM).
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context CPU structure.
 * @param   fWhat       What to import, CPUMCTX_EXTRN_XXX.
 */
VMM_INT_DECL(int) NEMImportStateOnDemand(PVMCPUCC pVCpu, uint64_t fWhat)
{
    STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatImportOnDemand);
    return nemHCLnxImportState(pVCpu, fWhat, &pVCpu->cpum.GstCtx, pVCpu->nem.s.pRun);
}


/**
 * Exports state to KVM.
 */
static int nemHCLnxExportState(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, struct kvm_run *pRun)
{
    uint64_t const fExtrn = ~pCtx->fExtrn & CPUMCTX_EXTRN_ALL;
    Assert((~fExtrn & CPUMCTX_EXTRN_ALL) != CPUMCTX_EXTRN_ALL);

    /*
     * Stuff that goes into kvm_run::s.regs.regs:
     */
    if (fExtrn & (CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_RFLAGS | CPUMCTX_EXTRN_GPRS_MASK))
    {
        if (fExtrn & CPUMCTX_EXTRN_RIP)
            pRun->s.regs.regs.rip    = pCtx->rip;
        if (fExtrn & CPUMCTX_EXTRN_RFLAGS)
            pRun->s.regs.regs.rflags = pCtx->rflags.u;

        if (fExtrn & CPUMCTX_EXTRN_RAX)
            pRun->s.regs.regs.rax    = pCtx->rax;
        if (fExtrn & CPUMCTX_EXTRN_RCX)
            pRun->s.regs.regs.rcx    = pCtx->rcx;
        if (fExtrn & CPUMCTX_EXTRN_RDX)
            pRun->s.regs.regs.rdx    = pCtx->rdx;
        if (fExtrn & CPUMCTX_EXTRN_RBX)
            pRun->s.regs.regs.rbx    = pCtx->rbx;
        if (fExtrn & CPUMCTX_EXTRN_RSP)
            pRun->s.regs.regs.rsp    = pCtx->rsp;
        if (fExtrn & CPUMCTX_EXTRN_RBP)
            pRun->s.regs.regs.rbp    = pCtx->rbp;
        if (fExtrn & CPUMCTX_EXTRN_RSI)
            pRun->s.regs.regs.rsi    = pCtx->rsi;
        if (fExtrn & CPUMCTX_EXTRN_RDI)
            pRun->s.regs.regs.rdi    = pCtx->rdi;
        if (fExtrn & CPUMCTX_EXTRN_R8_R15)
        {
            pRun->s.regs.regs.r8     = pCtx->r8;
            pRun->s.regs.regs.r9     = pCtx->r9;
            pRun->s.regs.regs.r10    = pCtx->r10;
            pRun->s.regs.regs.r11    = pCtx->r11;
            pRun->s.regs.regs.r12    = pCtx->r12;
            pRun->s.regs.regs.r13    = pCtx->r13;
            pRun->s.regs.regs.r14    = pCtx->r14;
            pRun->s.regs.regs.r15    = pCtx->r15;
        }
        pRun->kvm_dirty_regs |= KVM_SYNC_X86_REGS;
    }

    /*
     * Stuff that goes into kvm_run::s.regs.sregs:
     *
     * The APIC base register updating is a little suboptimal... But at least
     * VBox always has the right base register value, so it's one directional.
     */
    uint64_t const uApicBase = APICGetBaseMsrNoCheck(pVCpu);
    if (   (fExtrn & (  CPUMCTX_EXTRN_SREG_MASK | CPUMCTX_EXTRN_TABLE_MASK | CPUMCTX_EXTRN_CR_MASK
                      | CPUMCTX_EXTRN_EFER      | CPUMCTX_EXTRN_APIC_TPR))
        || uApicBase != pVCpu->nem.s.uKvmApicBase)
    {
        if ((pVCpu->nem.s.uKvmApicBase ^ uApicBase) & MSR_IA32_APICBASE_EN)
            Log(("NEM/%u: APICBASE_EN changed %#010RX64 -> %#010RX64\n", pVCpu->idCpu, pVCpu->nem.s.uKvmApicBase, uApicBase));
        pRun->s.regs.sregs.apic_base = uApicBase;
        pVCpu->nem.s.uKvmApicBase    = uApicBase;

#define NEM_LNX_EXPORT_SEG(a_KvmSeg, a_CtxSeg) do { \
            (a_KvmSeg).base     = (a_CtxSeg).u64Base; \
            (a_KvmSeg).limit    = (a_CtxSeg).u32Limit; \
            (a_KvmSeg).selector = (a_CtxSeg).Sel; \
            (a_KvmSeg).type     = (a_CtxSeg).Attr.n.u4Type; \
            (a_KvmSeg).s        = (a_CtxSeg).Attr.n.u1DescType; \
            (a_KvmSeg).dpl      = (a_CtxSeg).Attr.n.u2Dpl; \
            (a_KvmSeg).present  = (a_CtxSeg).Attr.n.u1Present; \
            (a_KvmSeg).avl      = (a_CtxSeg).Attr.n.u1Available; \
            (a_KvmSeg).l        = (a_CtxSeg).Attr.n.u1Long; \
            (a_KvmSeg).db       = (a_CtxSeg).Attr.n.u1DefBig; \
            (a_KvmSeg).g        = (a_CtxSeg).Attr.n.u1Granularity; \
            (a_KvmSeg).unusable = (a_CtxSeg).Attr.n.u1Unusable; \
            (a_KvmSeg).padding  = 0; \
        } while (0)

        if (fExtrn & CPUMCTX_EXTRN_SREG_MASK)
        {
            if (fExtrn & CPUMCTX_EXTRN_ES)
                NEM_LNX_EXPORT_SEG(pRun->s.regs.sregs.es, pCtx->es);
            if (fExtrn & CPUMCTX_EXTRN_CS)
                NEM_LNX_EXPORT_SEG(pRun->s.regs.sregs.cs, pCtx->cs);
            if (fExtrn & CPUMCTX_EXTRN_SS)
                NEM_LNX_EXPORT_SEG(pRun->s.regs.sregs.ss, pCtx->ss);
            if (fExtrn & CPUMCTX_EXTRN_DS)
                NEM_LNX_EXPORT_SEG(pRun->s.regs.sregs.ds, pCtx->ds);
            if (fExtrn & CPUMCTX_EXTRN_FS)
                NEM_LNX_EXPORT_SEG(pRun->s.regs.sregs.fs, pCtx->fs);
            if (fExtrn & CPUMCTX_EXTRN_GS)
                NEM_LNX_EXPORT_SEG(pRun->s.regs.sregs.gs, pCtx->gs);
        }
        if (fExtrn & CPUMCTX_EXTRN_TABLE_MASK)
        {
            if (fExtrn & CPUMCTX_EXTRN_GDTR)
            {
                pRun->s.regs.sregs.gdt.base  = pCtx->gdtr.pGdt;
                pRun->s.regs.sregs.gdt.limit = pCtx->gdtr.cbGdt;
                pRun->s.regs.sregs.gdt.padding[0] = 0;
                pRun->s.regs.sregs.gdt.padding[1] = 0;
                pRun->s.regs.sregs.gdt.padding[2] = 0;
            }
            if (fExtrn & CPUMCTX_EXTRN_IDTR)
            {
                pRun->s.regs.sregs.idt.base  = pCtx->idtr.pIdt;
                pRun->s.regs.sregs.idt.limit = pCtx->idtr.cbIdt;
                pRun->s.regs.sregs.idt.padding[0] = 0;
                pRun->s.regs.sregs.idt.padding[1] = 0;
                pRun->s.regs.sregs.idt.padding[2] = 0;
            }
            if (fExtrn & CPUMCTX_EXTRN_LDTR)
                NEM_LNX_EXPORT_SEG(pRun->s.regs.sregs.ldt, pCtx->ldtr);
            if (fExtrn & CPUMCTX_EXTRN_TR)
                NEM_LNX_EXPORT_SEG(pRun->s.regs.sregs.tr, pCtx->tr);
        }
        if (fExtrn & CPUMCTX_EXTRN_CR_MASK)
        {
            if (fExtrn & CPUMCTX_EXTRN_CR0)
                pRun->s.regs.sregs.cr0   = pCtx->cr0;
            if (fExtrn & CPUMCTX_EXTRN_CR2)
                pRun->s.regs.sregs.cr2   = pCtx->cr2;
            if (fExtrn & CPUMCTX_EXTRN_CR3)
                pRun->s.regs.sregs.cr3   = pCtx->cr3;
            if (fExtrn & CPUMCTX_EXTRN_CR4)
                pRun->s.regs.sregs.cr4   = pCtx->cr4;
        }
        if (fExtrn & CPUMCTX_EXTRN_EFER)
            pRun->s.regs.sregs.efer   = pCtx->msrEFER;

        RT_ZERO(pRun->s.regs.sregs.interrupt_bitmap); /* this is an alternative interrupt injection interface */

        pRun->kvm_dirty_regs |= KVM_SYNC_X86_SREGS;
    }

    /*
     * Debug registers.
     */
    if (fExtrn & CPUMCTX_EXTRN_DR_MASK)
    {
        struct kvm_debugregs DbgRegs = {{0}};

        if ((fExtrn & CPUMCTX_EXTRN_DR_MASK) != CPUMCTX_EXTRN_DR_MASK)
        {
            /* Partial debug state, we must get DbgRegs first so we can merge: */
            int rc = ioctl(pVCpu->nem.s.fdVCpu, KVM_GET_DEBUGREGS, &DbgRegs);
            AssertMsgReturn(rc == 0, ("rc=%d errno=%d\n", rc, errno), VERR_NEM_IPE_3);
        }

        if (fExtrn & CPUMCTX_EXTRN_DR0_DR3)
        {
            DbgRegs.db[0] = pCtx->dr[0];
            DbgRegs.db[1] = pCtx->dr[1];
            DbgRegs.db[2] = pCtx->dr[2];
            DbgRegs.db[3] = pCtx->dr[3];
        }
        if (fExtrn & CPUMCTX_EXTRN_DR6)
            DbgRegs.dr6 = pCtx->dr[6];
        if (fExtrn & CPUMCTX_EXTRN_DR7)
            DbgRegs.dr7 = pCtx->dr[7];

        int rc = ioctl(pVCpu->nem.s.fdVCpu, KVM_SET_DEBUGREGS, &DbgRegs);
        AssertMsgReturn(rc == 0, ("rc=%d errno=%d\n", rc, errno), VERR_NEM_IPE_3);
    }

    /*
     * FPU, SSE, AVX, ++.
     */
    if (fExtrn & (CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX | CPUMCTX_EXTRN_OTHER_XSAVE | CPUMCTX_EXTRN_XCRx))
    {
        if (fExtrn & (CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX | CPUMCTX_EXTRN_OTHER_XSAVE))
        {
            /** @todo could IEM just grab state partial control in some situations? */
            Assert(   (fExtrn & (CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX | CPUMCTX_EXTRN_OTHER_XSAVE))
                   ==           (CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX | CPUMCTX_EXTRN_OTHER_XSAVE)); /* no partial states */

            AssertCompile(sizeof(pCtx->XState) >= sizeof(struct kvm_xsave));
            int rc = ioctl(pVCpu->nem.s.fdVCpu, KVM_SET_XSAVE, &pCtx->XState);
            AssertMsgReturn(rc == 0, ("rc=%d errno=%d\n", rc, errno), VERR_NEM_IPE_3);
        }

        if (fExtrn & CPUMCTX_EXTRN_XCRx)
        {
            struct kvm_xcrs Xcrs =
            {   /*.nr_xcrs = */ 2,
                /*.flags = */   0,
                /*.xcrs= */ {
                    { /*.xcr =*/ 0, /*.reserved=*/ 0, /*.value=*/ pCtx->aXcr[0] },
                    { /*.xcr =*/ 1, /*.reserved=*/ 0, /*.value=*/ pCtx->aXcr[1] },
                }
            };

            int rc = ioctl(pVCpu->nem.s.fdVCpu, KVM_SET_XCRS, &Xcrs);
            AssertMsgReturn(rc == 0, ("rc=%d errno=%d\n", rc, errno), VERR_NEM_IPE_3);
        }
    }

    /*
     * MSRs.
     */
    if (fExtrn & (  CPUMCTX_EXTRN_KERNEL_GS_BASE | CPUMCTX_EXTRN_SYSCALL_MSRS | CPUMCTX_EXTRN_SYSENTER_MSRS
                  | CPUMCTX_EXTRN_TSC_AUX        | CPUMCTX_EXTRN_OTHER_MSRS))
    {
        union
        {
            struct kvm_msrs Core;
            uint64_t padding[2 + sizeof(struct kvm_msr_entry) * 32];
        }                   uBuf;
        uint32_t            iMsr     = 0;
        PCPUMCTXMSRS const  pCtxMsrs = CPUMQueryGuestCtxMsrsPtr(pVCpu);

#define ADD_MSR(a_Msr, a_uValue) do { \
            Assert(iMsr < 32); \
            uBuf.Core.entries[iMsr].index    = (a_Msr); \
            uBuf.Core.entries[iMsr].reserved = 0; \
            uBuf.Core.entries[iMsr].data     = (a_uValue); \
            iMsr += 1; \
        } while (0)

        if (fExtrn & CPUMCTX_EXTRN_KERNEL_GS_BASE)
            ADD_MSR(MSR_K8_KERNEL_GS_BASE, pCtx->msrKERNELGSBASE);
        if (fExtrn & CPUMCTX_EXTRN_SYSCALL_MSRS)
        {
            ADD_MSR(MSR_K6_STAR,    pCtx->msrSTAR);
            ADD_MSR(MSR_K8_LSTAR,   pCtx->msrLSTAR);
            ADD_MSR(MSR_K8_CSTAR,   pCtx->msrCSTAR);
            ADD_MSR(MSR_K8_SF_MASK, pCtx->msrSFMASK);
        }
        if (fExtrn & CPUMCTX_EXTRN_SYSENTER_MSRS)
        {
            ADD_MSR(MSR_IA32_SYSENTER_CS,  pCtx->SysEnter.cs);
            ADD_MSR(MSR_IA32_SYSENTER_EIP, pCtx->SysEnter.eip);
            ADD_MSR(MSR_IA32_SYSENTER_ESP, pCtx->SysEnter.esp);
        }
        if (fExtrn & CPUMCTX_EXTRN_TSC_AUX)
            ADD_MSR(MSR_K8_TSC_AUX, pCtxMsrs->msr.TscAux);
        if (fExtrn & CPUMCTX_EXTRN_OTHER_MSRS)
        {
            ADD_MSR(MSR_IA32_CR_PAT, pCtx->msrPAT);
            /** @todo What do we _have_ to add here?
             * We also have: Mttr*, MiscEnable, FeatureControl. */
        }

        uBuf.Core.pad   = 0;
        uBuf.Core.nmsrs = iMsr;
        int rc = ioctl(pVCpu->nem.s.fdVCpu, KVM_SET_MSRS, &uBuf);
        AssertMsgReturn(rc == (int)iMsr,
                        ("rc=%d iMsr=%d (->%#x) errno=%d\n",
                         rc, iMsr, (uint32_t)rc < iMsr ? uBuf.Core.entries[rc].index : 0, errno),
                        VERR_NEM_IPE_3);
    }

    /*
     * Interruptibility state.
     *
     * Note! This I/O control function sets most fields passed in, so when
     *       raising an interrupt, NMI, SMI or exception, this must be done
     *       by the code doing the rasing or we'll overwrite it here.
     */
    if (fExtrn & (CPUMCTX_EXTRN_INHIBIT_INT | CPUMCTX_EXTRN_INHIBIT_NMI))
    {
        Assert(   (fExtrn & (CPUMCTX_EXTRN_INHIBIT_INT | CPUMCTX_EXTRN_INHIBIT_NMI))
               ==           (CPUMCTX_EXTRN_INHIBIT_INT | CPUMCTX_EXTRN_INHIBIT_NMI));

        struct kvm_vcpu_events KvmEvents = {0};
        int rcLnx = ioctl(pVCpu->nem.s.fdVCpu, KVM_GET_VCPU_EVENTS, &KvmEvents);
        AssertLogRelMsgReturn(rcLnx == 0, ("rcLnx=%d errno=%d\n", rcLnx, errno), VERR_NEM_IPE_5);

        KvmEvents.flags = KVM_VCPUEVENT_VALID_SHADOW;
        if (!CPUMIsInInterruptShadowWithUpdate(&pVCpu->cpum.GstCtx))
        { /* probably likely */ }
        else
            KvmEvents.interrupt.shadow = (CPUMIsInInterruptShadowAfterSs(&pVCpu->cpum.GstCtx)  ? KVM_X86_SHADOW_INT_MOV_SS : 0)
                                       | (CPUMIsInInterruptShadowAfterSti(&pVCpu->cpum.GstCtx) ? KVM_X86_SHADOW_INT_STI    : 0);

        /* No flag - this is updated unconditionally. */
        KvmEvents.nmi.masked = CPUMAreInterruptsInhibitedByNmi(&pVCpu->cpum.GstCtx);

        rcLnx = ioctl(pVCpu->nem.s.fdVCpu, KVM_SET_VCPU_EVENTS, &KvmEvents);
        AssertLogRelMsgReturn(rcLnx == 0, ("rcLnx=%d errno=%d\n", rcLnx, errno), VERR_NEM_IPE_3);
    }

    /*
     * KVM now owns all the state.
     */
    pCtx->fExtrn = CPUMCTX_EXTRN_KEEPER_NEM | CPUMCTX_EXTRN_ALL;

    RT_NOREF(pVM);
    return VINF_SUCCESS;
}


/**
 * Query the CPU tick counter and optionally the TSC_AUX MSR value.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context CPU structure.
 * @param   pcTicks     Where to return the CPU tick count.
 * @param   puAux       Where to return the TSC_AUX register value.
 */
VMM_INT_DECL(int) NEMHCQueryCpuTick(PVMCPUCC pVCpu, uint64_t *pcTicks, uint32_t *puAux)
{
    STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatQueryCpuTick);

    // This function is called when the VM is paused or
    // suspended. It's called for all vCPUs.

    const size_t NMSRS = 2;

    size_t szReq = RT_UOFFSETOF_DYN(struct kvm_msrs, entries[NMSRS]);
    struct kvm_msrs *pReq = static_cast<kvm_msrs *>(alloca(szReq));
    memset(pReq, 0, szReq);

    pReq->nmsrs = NMSRS;
    pReq->entries[0].index = MSR_IA32_TSC;
    pReq->entries[1].index = MSR_K8_TSC_AUX;

    int rcLnx = ioctl(pVCpu->nem.s.fdVCpu, KVM_GET_MSRS, pReq);
    AssertLogRelMsgReturn(rcLnx == NMSRS, ("rcLnx=%d errno=%d\n", rcLnx, errno), VERR_NEM_IPE_5);

    if (pcTicks) {
      *pcTicks = pReq->entries[0].data;
    }

    if (puAux) {
      *puAux = static_cast<uint32_t>(pReq->entries[1].data);
    }

    return VINF_SUCCESS;
}


/**
 * Resumes CPU clock (TSC) on all virtual CPUs.
 *
 * This is called by TM when the VM is started, restored, resumed or similar.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context CPU structure of the calling EMT.
 * @param   uPausedTscValue The TSC value at the time of pausing.
 */
VMM_INT_DECL(int) NEMHCResumeCpuTickOnAll(PVMCC pVM, PVMCPUCC pVCpu, uint64_t uPausedTscValue)
{
    RT_NOREF(pVCpu);

    // This function is called once during unpause or resume. Despite
    // the pVCpu parameter it is _not_ called for all vCPUs.

    const size_t NMSRS = 1;

    size_t szReq = RT_UOFFSETOF_DYN(struct kvm_msrs, entries[NMSRS]);
    struct kvm_msrs *pReq = static_cast<kvm_msrs *>(alloca(szReq));
    memset(pReq, 0, szReq);

    pReq->nmsrs = NMSRS;
    pReq->entries[0].index = MSR_IA32_TSC;
    pReq->entries[0].data = uPausedTscValue;

    // Setting the individual TSC values of all CPUs is fundamentally
    // flawed, because the TSCs keep ticking while we set them. That
    // means that we never really end up with synchronized TSC values
    // unless KVM's built-in TSC synchronization magic fixes things up
    // for us. But the interface doesn't leave us a lot of choice here
    // for now.
    //
    // A better approach would be to use KVM_GET_CLOCK/KVM_SET_CLOCK
    // and restore TSC_ADJUST values. We should validate whether this
    // does the right thing though first.
    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
    {
        PVMCPU pVCpuCur = pVM->apCpusR3[idCpu];

        int rcLnx = ioctl(pVCpuCur->nem.s.fdVCpu, KVM_SET_MSRS, pReq);
        AssertLogRelMsgReturn(rcLnx == NMSRS, ("rcLnx=%d errno=%d\n", rcLnx, errno), VERR_NEM_IPE_5);
    }

    return VINF_SUCCESS;
}


VMM_INT_DECL(uint32_t) NEMHCGetFeatures(PVMCC pVM)
{
    RT_NOREF(pVM);
    return NEM_FEAT_F_NESTED_PAGING
         | NEM_FEAT_F_FULL_GST_EXEC
         | NEM_FEAT_F_XSAVE_XRSTOR;
}



/*********************************************************************************************************************************
*   Execution                                                                                                                    *
*********************************************************************************************************************************/


VMMR3_INT_DECL(bool) NEMR3CanExecuteGuest(PVM pVM, PVMCPU pVCpu)
{
#ifndef VBOX_WITH_KVM_IRQCHIP_FULL
    /*
     * Only execute when the A20 gate is enabled as I cannot immediately
     * spot any A20 support in KVM.
     */
    RT_NOREF(pVM);
    Assert(VM_IS_NEM_ENABLED(pVM));
    return PGMPhysIsA20Enabled(pVCpu);
#else
    /*
     * In full-irqchip mode, we always need to execute via KVM because we
     * have no other way to inject interrupt into the guest (because the PIC is
     * in the kernel!). Otherwise, we will break non-UEFI boot. This will
     * break DOS support.
     */
    return true;
#endif
}


bool nemR3NativeSetSingleInstruction(PVM pVM, PVMCPU pVCpu, bool fEnable)
{
    NOREF(pVM); NOREF(pVCpu); NOREF(fEnable);
    return false;
}


void nemR3NativeNotifyFF(PVM pVM, PVMCPU pVCpu, uint32_t fFlags)
{
    if (pVCpu->hThread == RTThreadSelf()) {
        // RTThreadPoke doesn't like poking the current thread. We can
        // safely return here because the vCPU thread is currently handling
        // an exit and will will check all conditions again when we re-enter
        // the run-loop.
        return;
    }

    int rc = RTThreadPoke(pVCpu->hThread);
    LogFlow(("nemR3NativeNotifyFF: #%u -> %Rrc\n", pVCpu->idCpu, rc));
    AssertRC(rc);
    RT_NOREF(pVM, fFlags);
}


DECLHIDDEN(bool) nemR3NativeNotifyDebugEventChanged(PVM pVM, bool fUseDebugLoop)
{
    RT_NOREF(pVM, fUseDebugLoop);
    return false;
}


DECLHIDDEN(bool) nemR3NativeNotifyDebugEventChangedPerCpu(PVM pVM, PVMCPU pVCpu, bool fUseDebugLoop)
{
    RT_NOREF(pVM, pVCpu, fUseDebugLoop);
    return false;
}


/**
 * Deals with pending interrupt FFs prior to executing guest code.
 */
static VBOXSTRICTRC nemHCLnxHandleInterruptFF(PVM pVM, PVMCPU pVCpu, struct kvm_run *pRun)
{
    RT_NOREF_PV(pVM);

    /*
     * Do not doing anything if TRPM has something pending already as we can
     * only inject one event per KVM_RUN call.  This can only happend if we
     * can directly from the loop in EM, so the inhibit bits must be internal.
     */
    if (TRPMHasTrap(pVCpu))
    {
        Log8(("nemHCLnxHandleInterruptFF: TRPM has an pending event already\n"));

        return VINF_SUCCESS;
    }

    /*
     * First update APIC.  We ASSUME this won't need TPR/CR8.
     */
    if (VMCPU_FF_TEST_AND_CLEAR(pVCpu, VMCPU_FF_UPDATE_APIC))
    {
        AssertLogRelMsgReturn(false, ("VMCPU_FF_UPDATE_APIC is set"), VERR_NEM_IPE_5);
    }

    if (!VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_INTERRUPT_PIC | VMCPU_FF_INTERRUPT_NMI  | VMCPU_FF_INTERRUPT_SMI))
        return VINF_SUCCESS;

    /*
     * We don't currently implement SMIs.
     */
    AssertReturn(!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INTERRUPT_SMI), VERR_NEM_IPE_0);

    /*
     * In KVM the CPUMCTX_EXTRN_INHIBIT_INT and CPUMCTX_EXTRN_INHIBIT_NMI states
     * are tied together with interrupt and NMI delivery, so we must get and
     * synchronize these all in one go and set both CPUMCTX_EXTRN_INHIBIT_XXX flags.
     * If we don't we may lose the interrupt/NMI we marked pending here when the
     * state is exported again before execution.
     */
    struct kvm_vcpu_events KvmEvents = {0};
    int rcLnx = ioctl(pVCpu->nem.s.fdVCpu, KVM_GET_VCPU_EVENTS, &KvmEvents);
    AssertLogRelMsgReturn(rcLnx == 0, ("rcLnx=%d errno=%d\n", rcLnx, errno), VERR_NEM_IPE_5);

    if (!(pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_RIP))
        pRun->s.regs.regs.rip = pVCpu->cpum.GstCtx.rip;

    KvmEvents.flags |= KVM_VCPUEVENT_VALID_SHADOW;
    if (!(pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_INHIBIT_INT))
        KvmEvents.interrupt.shadow = !CPUMIsInInterruptShadowWithUpdate(&pVCpu->cpum.GstCtx) ? 0
                                   :   (CPUMIsInInterruptShadowAfterSs(&pVCpu->cpum.GstCtx)  ? KVM_X86_SHADOW_INT_MOV_SS : 0)
                                     | (CPUMIsInInterruptShadowAfterSti(&pVCpu->cpum.GstCtx) ? KVM_X86_SHADOW_INT_STI    : 0);
    else
        CPUMUpdateInterruptShadowSsStiEx(&pVCpu->cpum.GstCtx,
                                         RT_BOOL(KvmEvents.interrupt.shadow & KVM_X86_SHADOW_INT_MOV_SS),
                                         RT_BOOL(KvmEvents.interrupt.shadow & KVM_X86_SHADOW_INT_STI),
                                         pRun->s.regs.regs.rip);

    if (!(pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_INHIBIT_NMI))
        KvmEvents.nmi.masked = CPUMAreInterruptsInhibitedByNmi(&pVCpu->cpum.GstCtx);
    else
        CPUMUpdateInterruptInhibitingByNmi(&pVCpu->cpum.GstCtx, KvmEvents.nmi.masked != 0);

    /* KVM will own the INT + NMI inhibit state soon: */
    pVCpu->cpum.GstCtx.fExtrn = (pVCpu->cpum.GstCtx.fExtrn & ~CPUMCTX_EXTRN_KEEPER_MASK)
                              | CPUMCTX_EXTRN_KEEPER_NEM | CPUMCTX_EXTRN_INHIBIT_INT | CPUMCTX_EXTRN_INHIBIT_NMI;

    /*
     * NMI? Try deliver it first.
     */
    if (VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INTERRUPT_NMI))
    {
#if 0
        int rcLnx = ioctl(pVCpu->nem.s.fdVm, KVM_NMI, 0UL);
        AssertLogRelMsgReturn(rcLnx == 0, ("rcLnx=%d errno=%d\n", rcLnx, errno), VERR_NEM_IPE_5);
#else
        KvmEvents.flags      |= KVM_VCPUEVENT_VALID_NMI_PENDING;
        KvmEvents.nmi.pending = 1;
#endif
        VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INTERRUPT_NMI);
        Log8(("Queuing NMI on %u\n", pVCpu->idCpu));
    }

#ifdef VBOX_WITH_KVM_IRQCHIP_FULL
    AssertLogRelMsg(!VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_INTERRUPT_PIC), ("PDM has pic interrupt but full irqchip is enabled"));
#else
    /*
     * PIC interrupt?
     */
    if (VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_INTERRUPT_PIC))
    {
        if (pRun->s.regs.regs.rflags & X86_EFL_IF)
        {
            if (pRun->ready_for_interrupt_injection)
            {
                uint8_t bInterrupt;
                int rc = PDMGetInterrupt(pVCpu, &bInterrupt);
                if (RT_SUCCESS(rc))
                {
                    TRPMAssertTrap(pVCpu, bInterrupt, TRPM_HARDWARE_INT);

                    Log8(("Queuing interrupt %#x on %u: %04x:%08RX64 efl=%#x\n", bInterrupt, pVCpu->idCpu,
                          pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, pVCpu->cpum.GstCtx.eflags.u));
                }
                else if (rc == VERR_APIC_INTR_MASKED_BY_TPR) /** @todo this isn't extremely efficient if we get a lot of exits... */
                    Log8(("VERR_APIC_INTR_MASKED_BY_TPR\n")); /* We'll get a TRP exit - no interrupt window needed. */
                else
                    Log8(("PDMGetInterrupt failed -> %Rrc\n", rc));
            }
            else
            {
                pRun->request_interrupt_window = 1;
                Log8(("Interrupt window pending on %u (#2)\n", pVCpu->idCpu));
            }
        }
        else
        {
            pRun->request_interrupt_window = 1;
            Log8(("Interrupt window pending on %u (#1)\n", pVCpu->idCpu));
        }
    }
#endif
    /*
     * Now, update the state.
     */
    /** @todo skip when possible...   */
    rcLnx = ioctl(pVCpu->nem.s.fdVCpu, KVM_SET_VCPU_EVENTS, &KvmEvents);
    AssertLogRelMsgReturn(rcLnx == 0, ("rcLnx=%d errno=%d\n", rcLnx, errno), VERR_NEM_IPE_5);

    return VINF_SUCCESS;
}


/**
 * Handles KVM_EXIT_INTERNAL_ERROR.
 */
static VBOXSTRICTRC nemR3LnxHandleInternalError(PVMCPU pVCpu, struct kvm_run *pRun)
{
    Log(("NEM: KVM_EXIT_INTERNAL_ERROR! suberror=%#x (%d) ndata=%u data=%.*Rhxs\n", pRun->internal.suberror,
         pRun->internal.suberror, pRun->internal.ndata, sizeof(pRun->internal.data), &pRun->internal.data[0]));

    /*
     * Deal with each suberror, returning if we don't want IEM to handle it.
     */
    switch (pRun->internal.suberror)
    {
        case KVM_INTERNAL_ERROR_EMULATION:
        {
            EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_INTERNAL_ERROR_EMULATION),
                             pRun->s.regs.regs.rip + pRun->s.regs.sregs.cs.base, ASMReadTSC());
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitInternalErrorEmulation);
            break;
        }

        case KVM_INTERNAL_ERROR_SIMUL_EX:
        case KVM_INTERNAL_ERROR_DELIVERY_EV:
        case KVM_INTERNAL_ERROR_UNEXPECTED_EXIT_REASON:
        default:
        {
            EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_INTERNAL_ERROR_FATAL),
                             pRun->s.regs.regs.rip + pRun->s.regs.sregs.cs.base, ASMReadTSC());
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitInternalErrorFatal);
            const char *pszName;
            switch (pRun->internal.suberror)
            {
                case KVM_INTERNAL_ERROR_EMULATION:              pszName = "KVM_INTERNAL_ERROR_EMULATION"; break;
                case KVM_INTERNAL_ERROR_SIMUL_EX:               pszName = "KVM_INTERNAL_ERROR_SIMUL_EX"; break;
                case KVM_INTERNAL_ERROR_DELIVERY_EV:            pszName = "KVM_INTERNAL_ERROR_DELIVERY_EV"; break;
                case KVM_INTERNAL_ERROR_UNEXPECTED_EXIT_REASON: pszName = "KVM_INTERNAL_ERROR_UNEXPECTED_EXIT_REASON"; break;
                default:                                        pszName = "unknown"; break;
            }
            LogRel(("NEM: KVM_EXIT_INTERNAL_ERROR! suberror=%#x (%s) ndata=%u data=%.*Rhxs\n", pRun->internal.suberror, pszName,
                    pRun->internal.ndata, sizeof(pRun->internal.data), &pRun->internal.data[0]));
            return VERR_NEM_IPE_0;
        }
    }

    /*
     * Execute instruction in IEM and try get on with it.
     */
    Log2(("nemR3LnxHandleInternalError: Executing instruction at %04x:%08RX64 in IEM\n",
          pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip));
    VBOXSTRICTRC rcStrict = nemHCLnxImportState(pVCpu,
                                                IEM_CPUMCTX_EXTRN_MUST_MASK | CPUMCTX_EXTRN_INHIBIT_INT
                                                 | CPUMCTX_EXTRN_INHIBIT_NMI,
                                                &pVCpu->cpum.GstCtx, pRun);
    if (RT_SUCCESS(rcStrict))
        rcStrict = IEMExecOne(pVCpu);
    return rcStrict;
}


/**
 * Handles KVM_EXIT_IO.
 */
static VBOXSTRICTRC nemHCLnxHandleExitIo(PVMCC pVM, PVMCPUCC pVCpu, struct kvm_run *pRun)
{
    /*
     * Input validation.
     */
    Assert(pRun->io.count > 0);
    Assert(pRun->io.size == 1 || pRun->io.size == 2 || pRun->io.size == 4);
    Assert(pRun->io.direction == KVM_EXIT_IO_IN || pRun->io.direction == KVM_EXIT_IO_OUT);
    Assert(pRun->io.data_offset < pVM->nem.s.cbVCpuMmap);
    Assert(pRun->io.data_offset + pRun->io.size * pRun->io.count <= pVM->nem.s.cbVCpuMmap);

    /*
     * We cannot easily act on the exit history here, because the I/O port
     * exit is stateful and the instruction will be completed in the next
     * KVM_RUN call.  There seems no way to avoid this.
     */
    EMHistoryAddExit(pVCpu,
                     pRun->io.count == 1
                     ? (  pRun->io.direction == KVM_EXIT_IO_IN
                        ? EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_IO_PORT_READ)
                        : EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_IO_PORT_WRITE))
                     : (  pRun->io.direction == KVM_EXIT_IO_IN
                        ? EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_IO_PORT_STR_READ)
                        : EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_IO_PORT_STR_WRITE)),
                     pRun->s.regs.regs.rip + pRun->s.regs.sregs.cs.base, ASMReadTSC());

    /*
     * Do the requested job.
     */
    VBOXSTRICTRC    rcStrict;
    RTPTRUNION      uPtrData;
    uPtrData.pu8 = (uint8_t *)pRun + pRun->io.data_offset;
    if (pRun->io.count == 1)
    {
        if (pRun->io.direction == KVM_EXIT_IO_IN)
        {
            uint32_t uValue = 0;
            rcStrict = IOMIOPortRead(pVM, pVCpu, pRun->io.port, &uValue, pRun->io.size);
            Log4(("IOExit/%u: %04x:%08RX64: IN %#x LB %u -> %#x, rcStrict=%Rrc\n",
                  pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip,
                  pRun->io.port, pRun->io.size, uValue, VBOXSTRICTRC_VAL(rcStrict) ));
            if (IOM_SUCCESS(rcStrict))
            {
                if (pRun->io.size == 4)
                    *uPtrData.pu32 = uValue;
                else if (pRun->io.size == 2)
                    *uPtrData.pu16 = (uint16_t)uValue;
                else
                    *uPtrData.pu8  = (uint8_t)uValue;
            }
        }
        else
        {
            uint32_t const uValue = pRun->io.size == 4 ? *uPtrData.pu32
                                  : pRun->io.size == 2 ? *uPtrData.pu16
                                  :                      *uPtrData.pu8;
            rcStrict = IOMIOPortWrite(pVM, pVCpu, pRun->io.port, uValue, pRun->io.size);
            Log4(("IOExit/%u: %04x:%08RX64: OUT %#x, %#x LB %u rcStrict=%Rrc\n",
                  pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip,
                  pRun->io.port, uValue, pRun->io.size, VBOXSTRICTRC_VAL(rcStrict) ));
        }
    }
    else
    {
        uint32_t cTransfers = pRun->io.count;
        if (pRun->io.direction == KVM_EXIT_IO_IN)
        {
            rcStrict = IOMIOPortReadString(pVM, pVCpu, pRun->io.port, uPtrData.pv, &cTransfers, pRun->io.size);
            Log4(("IOExit/%u: %04x:%08RX64: REP INS %#x LB %u * %#x times -> rcStrict=%Rrc cTransfers=%d\n",
                  pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip,
                  pRun->io.port, pRun->io.size, pRun->io.count, VBOXSTRICTRC_VAL(rcStrict), cTransfers ));
        }
        else
        {
            rcStrict = IOMIOPortWriteString(pVM, pVCpu, pRun->io.port, uPtrData.pv, &cTransfers, pRun->io.size);
            Log4(("IOExit/%u: %04x:%08RX64: REP OUTS %#x LB %u * %#x times -> rcStrict=%Rrc cTransfers=%d\n",
                  pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip,
                  pRun->io.port, pRun->io.size, pRun->io.count, VBOXSTRICTRC_VAL(rcStrict), cTransfers ));
        }
        Assert(cTransfers == 0);
    }
    return rcStrict;
}


/**
 * Handles KVM_EXIT_MMIO.
 */
static VBOXSTRICTRC nemHCLnxHandleExitMmio(PVMCC pVM, PVMCPUCC pVCpu, struct kvm_run *pRun)
{
    /*
     * Input validation.
     */
    Assert(pRun->mmio.len <= sizeof(pRun->mmio.data));
    Assert(pRun->mmio.is_write <= 1);

    /*
     * We cannot easily act on the exit history here, because the MMIO port
     * exit is stateful and the instruction will be completed in the next
     * KVM_RUN call.  There seems no way to circumvent this.
     */
    EMHistoryAddExit(pVCpu,
                     pRun->mmio.is_write
                     ? EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_MMIO_WRITE)
                     : EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_MMIO_READ),
                     pRun->s.regs.regs.rip + pRun->s.regs.sregs.cs.base, ASMReadTSC());

    /*
     * Do the requested job.
     */
    VBOXSTRICTRC rcStrict;
    if (pRun->mmio.is_write)
    {
        /*
         * Sync LAPIC TPR register with cr8 from KVM. This is required as long
         * as we don't use KVM's IRQCHIP feature.
         *
         * This doesn't cover the X2APIC mode. But the whole cr8-code will be
         * gone very soon anyway as we will use KVM's split-irqchip.
         */
        if (pRun->mmio.phys_addr == XAPIC_TPR_ADDR) {
            pRun->cr8 = *pRun->mmio.data >> LAPIC_TPR_SHIFT;
        }
        rcStrict = PGMPhysWrite(pVM, pRun->mmio.phys_addr, pRun->mmio.data, pRun->mmio.len, PGMACCESSORIGIN_HM);
        Log4(("MmioExit/%u: %04x:%08RX64: WRITE %#x LB %u, %.*Rhxs -> rcStrict=%Rrc\n",
              pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip,
              pRun->mmio.phys_addr, pRun->mmio.len, pRun->mmio.len, pRun->mmio.data, VBOXSTRICTRC_VAL(rcStrict) ));
    }
    else
    {
        rcStrict = PGMPhysRead(pVM, pRun->mmio.phys_addr, pRun->mmio.data, pRun->mmio.len, PGMACCESSORIGIN_HM);
        Log4(("MmioExit/%u: %04x:%08RX64: READ %#x LB %u -> %.*Rhxs rcStrict=%Rrc\n",
              pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip,
              pRun->mmio.phys_addr, pRun->mmio.len, pRun->mmio.len, pRun->mmio.data, VBOXSTRICTRC_VAL(rcStrict) ));
    }
    return rcStrict;
}


/**
 * Handles KVM_EXIT_RDMSR
 */
static VBOXSTRICTRC nemHCLnxHandleExitRdMsr(PVMCPUCC pVCpu, struct kvm_run *pRun)
{
    /*
     * Input validation.
     */
    Assert(   pRun->msr.reason == KVM_MSR_EXIT_REASON_INVAL
           || pRun->msr.reason == KVM_MSR_EXIT_REASON_UNKNOWN
           || pRun->msr.reason == KVM_MSR_EXIT_REASON_FILTER);

    /*
     * We cannot easily act on the exit history here, because the MSR exit is
     * stateful and the instruction will be completed in the next KVM_RUN call.
     * There seems no way to circumvent this.
     */
    EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_MSR_READ),
                     pRun->s.regs.regs.rip + pRun->s.regs.sregs.cs.base, ASMReadTSC());

    /*
     * Do the requested job.
     */
    uint64_t uValue = 0;
    VBOXSTRICTRC rcStrict = CPUMQueryGuestMsr(pVCpu, pRun->msr.index, &uValue);
    pRun->msr.data = uValue;
    if (rcStrict != VERR_CPUM_RAISE_GP_0)
    {
        Log3(("MsrRead/%u: %04x:%08RX64: msr=%#010x (reason=%#x) -> %#RX64 rcStrict=%Rrc\n", pVCpu->idCpu,
              pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip, pRun->msr.index, pRun->msr.reason, uValue, VBOXSTRICTRC_VAL(rcStrict) ));
        pRun->msr.error = 0;
    }
    else
    {
        Log3(("MsrRead/%u: %04x:%08RX64: msr=%#010x (reason%#x)-> %#RX64 rcStrict=#GP!\n", pVCpu->idCpu,
              pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip, pRun->msr.index, pRun->msr.reason, uValue));
        pRun->msr.error = 1;
        rcStrict = VINF_SUCCESS;
    }
    return rcStrict;
}


/**
 * Handles KVM_EXIT_WRMSR
 */
static VBOXSTRICTRC nemHCLnxHandleExitWrMsr(PVMCPUCC pVCpu, struct kvm_run *pRun)
{
    /*
     * Input validation.
     */
    Assert(   pRun->msr.reason == KVM_MSR_EXIT_REASON_INVAL
           || pRun->msr.reason == KVM_MSR_EXIT_REASON_UNKNOWN
           || pRun->msr.reason == KVM_MSR_EXIT_REASON_FILTER);

    /*
     * We cannot easily act on the exit history here, because the MSR exit is
     * stateful and the instruction will be completed in the next KVM_RUN call.
     * There seems no way to circumvent this.
     */
    EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_MSR_WRITE),
                     pRun->s.regs.regs.rip + pRun->s.regs.sregs.cs.base, ASMReadTSC());

    /*
     * Do the requested job.
     */
    VBOXSTRICTRC rcStrict = CPUMSetGuestMsr(pVCpu, pRun->msr.index, pRun->msr.data);
    if (rcStrict != VERR_CPUM_RAISE_GP_0)
    {
        Log3(("MsrWrite/%u: %04x:%08RX64: msr=%#010x := %#RX64 (reason=%#x) -> rcStrict=%Rrc\n", pVCpu->idCpu,
              pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip, pRun->msr.index, pRun->msr.data, pRun->msr.reason, VBOXSTRICTRC_VAL(rcStrict) ));
        pRun->msr.error = 0;
    }
    else
    {
        Log3(("MsrWrite/%u: %04x:%08RX64: msr=%#010x := %#RX64 (reason%#x)-> rcStrict=#GP!\n", pVCpu->idCpu,
              pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip, pRun->msr.index, pRun->msr.data, pRun->msr.reason));
        pRun->msr.error = 1;
        rcStrict = VINF_SUCCESS;
    }
    return rcStrict;
}

static VBOXSTRICTRC nemHCLnxHandleExit(PVMCC pVM, PVMCPUCC pVCpu, struct kvm_run *pRun, bool *pfStatefulExit)
{
    STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitTotal);
    switch (pRun->exit_reason)
    {
        case KVM_EXIT_EXCEPTION:
            AssertFailed();
            break;

        case KVM_EXIT_IO:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitIo);
            *pfStatefulExit = true;
            return nemHCLnxHandleExitIo(pVM, pVCpu, pRun);

        case KVM_EXIT_MMIO:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitMmio);
            *pfStatefulExit = true;
            return nemHCLnxHandleExitMmio(pVM, pVCpu, pRun);

        case KVM_EXIT_IRQ_WINDOW_OPEN:
            EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_INTTERRUPT_WINDOW),
                             pRun->s.regs.regs.rip + pRun->s.regs.sregs.cs.base, ASMReadTSC());
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitIrqWindowOpen);
            Log5(("IrqWinOpen/%u: %d\n", pVCpu->idCpu, pRun->request_interrupt_window));
            pRun->request_interrupt_window = 0;
            return VINF_SUCCESS;

        case KVM_EXIT_SET_TPR:
            AssertFailed();
            break;

        case KVM_EXIT_TPR_ACCESS:
            AssertFailed();
            break;

        case KVM_EXIT_X86_RDMSR:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitRdMsr);
            *pfStatefulExit = true;
            return nemHCLnxHandleExitRdMsr(pVCpu, pRun);

        case KVM_EXIT_X86_WRMSR:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitWrMsr);
            *pfStatefulExit = true;
            return nemHCLnxHandleExitWrMsr(pVCpu, pRun);

        case KVM_EXIT_HLT:
            EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_HALT),
                             pRun->s.regs.regs.rip + pRun->s.regs.sregs.cs.base, ASMReadTSC());
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitHalt);
            Log5(("Halt/%u\n", pVCpu->idCpu));
            return VINF_EM_HALT;

        case KVM_EXIT_INTR: /* EINTR */
            EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_INTERRUPTED),
                             pRun->s.regs.regs.rip + pRun->s.regs.sregs.cs.base, ASMReadTSC());
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitIntr);
            Log5(("Intr/%u\n", pVCpu->idCpu));

            /* If we don't consume the poke signal, subsequent KVM_RUN invocations will immediately return EINTR again. */
            nemR3LnxConsumePokeSignal();

            return VINF_SUCCESS;

        case KVM_EXIT_HYPERCALL:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitHypercall);
            AssertFailed();
            break;

        case KVM_EXIT_DEBUG:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitDebug);
            AssertFailed();
            break;

        case KVM_EXIT_SYSTEM_EVENT:
            AssertFailed();
            break;
        case KVM_EXIT_IOAPIC_EOI:
            PDMIoApicBroadcastEoi(pVM, pRun->eoi.vector);
            return VINF_SUCCESS;
        case KVM_EXIT_HYPERV:
            Assert(pVM->gim.s.enmProviderId == GIMPROVIDERID_HYPERV);

            switch (pRun->hyperv.type)
            {
            case KVM_EXIT_HYPERV_SYNDBG:
                /* The synthetic debugger is not enabled and we should not get these exits. */
                AssertFailed();
                break;
            case KVM_EXIT_HYPERV_HCALL:
                LogRel2(("Hyper-V hcall input:%lx p0:%lx p1:%lx\n", pRun->hyperv.u.hcall.input, pRun->hyperv.u.hcall.params[0], pRun->hyperv.u.hcall.params[1]));

                /* TODO KVM handles the performance-critical hypercalls on its own. We get mostly extended hypercalls
                   here. We would need to forward them to gimHvHypercall. None of these features are enabled right now,
                   so we can just deny the hypercall right away. */

                pRun->hyperv.u.hcall.result = GIM_HV_STATUS_ACCESS_DENIED;
                break;
            case KVM_EXIT_HYPERV_SYNIC:
                LogRel2(("HyperV synic msr:%lx control:%lx evt_page:%lx msg_page:%lx\n",
                         pRun->hyperv.u.synic.msr,
                         pRun->hyperv.u.synic.control,
                         pRun->hyperv.u.synic.evt_page,
                         pRun->hyperv.u.synic.msg_page));

                switch (pRun->hyperv.u.synic.msr)
                {
                case MSR_GIM_HV_SCONTROL:
                    gimHvWriteMsr(pVCpu, MSR_GIM_HV_SCONTROL, 0, pRun->hyperv.u.synic.control);
                    break;
                case MSR_GIM_HV_SIMP:
                    gimHvWriteMsr(pVCpu, MSR_GIM_HV_SIMP, 0, pRun->hyperv.u.synic.msg_page);
                    break;
                case MSR_GIM_HV_SIEFP:
                    gimHvWriteMsr(pVCpu, MSR_GIM_HV_SIEFP, 0, pRun->hyperv.u.synic.evt_page);
                    break;
                default:
                    AssertReleaseFailed();
                }
                break;
            default:
                AssertReleaseFailed();
            }

            return VINF_SUCCESS;

        case KVM_EXIT_DIRTY_RING_FULL:
            AssertFailed();
            break;
        case KVM_EXIT_AP_RESET_HOLD:
            AssertFailed();
            break;
        case KVM_EXIT_X86_BUS_LOCK:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitBusLock);
            AssertFailed();
            break;


        case KVM_EXIT_SHUTDOWN:
            AssertFailed();
            break;

        case KVM_EXIT_FAIL_ENTRY:
            LogRel(("NEM: KVM_EXIT_FAIL_ENTRY! hardware_entry_failure_reason=%#x cpu=%#x\n",
                    pRun->fail_entry.hardware_entry_failure_reason, pRun->fail_entry.cpu));
            EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_FAILED_ENTRY),
                             pRun->s.regs.regs.rip + pRun->s.regs.sregs.cs.base, ASMReadTSC());
            return VERR_NEM_IPE_1;

        case KVM_EXIT_INTERNAL_ERROR:
            /* we're counting sub-reasons inside the function. */
            return nemR3LnxHandleInternalError(pVCpu, pRun);

        /*
         * Foreign and unknowns.
         */
        case KVM_EXIT_NMI:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_NMI on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_EPR:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_EPR on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_WATCHDOG:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_WATCHDOG on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_ARM_NISV:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_ARM_NISV on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_S390_STSI:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_S390_STSI on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_S390_TSCH:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_S390_TSCH on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_OSI:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_OSI on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_PAPR_HCALL:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_PAPR_HCALL on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_S390_UCONTROL:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_S390_UCONTROL on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_DCR:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_DCR on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_S390_SIEIC:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_S390_SIEIC on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_S390_RESET:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_S390_RESET on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_UNKNOWN:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_UNKNOWN on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        case KVM_EXIT_XEN:
            AssertLogRelMsgFailedReturn(("KVM_EXIT_XEN on VCpu #%u at %04x:%RX64!\n", pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
        default:
            AssertLogRelMsgFailedReturn(("Unknown exit reason %u on VCpu #%u at %04x:%RX64!\n", pRun->exit_reason, pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip), VERR_NEM_IPE_1);
    }

    RT_NOREF(pVM, pVCpu, pRun);
    return VERR_NOT_IMPLEMENTED;
}

static VBOXSTRICTRC nemHCLnxHandleTimers(PVMCC pVM, PVMCPUCC pVCpu)
{
    uint64_t nsAbsNextTimerEvt;
    uint64_t uTscNow;
    uint64_t nsDelta = TMVirtualSyncGetNsToDeadline(pVM, &nsAbsNextTimerEvt, &uTscNow);

    [[maybe_unused]] uint64_t const nsAbsOldTimerEvt = pVCpu->nem.s.nsAbsNextTimerEvt;

    pVCpu->nem.s.nsAbsNextTimerEvt = nsAbsNextTimerEvt;

    /*
     * With this optimization we only program timers once when something changes. We can enable this when we are
     * confident that everything works correctly.
     */
#ifdef VBOX_KVM_DONT_REPROGRAM_TIMERS
    if (nsAbsOldTimerEvt == nsAbsNextTimerEvt) {
        return VINF_SUCCESS;
    }
#endif

    if (nsDelta == 0) {
        /* If there is no timeout, program a catch-all timer instead. */
        nsDelta = RT_NS_1MS_64;
    } else if (nsDelta >= RT_NS_1SEC_64) {
        /* We need to exit at least once every 4 seconds. */
        nsDelta = RT_NS_1SEC_64;
    }

    struct itimerspec timeout {};

    /*
     * It would be nice to program absolute timeouts here instead for better accuracy, but VBox times do not correlate
     * to any Linux timer.
     */
    timeout.it_value.tv_sec = nsDelta / RT_NS_1SEC_64;
    timeout.it_value.tv_nsec = nsDelta % RT_NS_1SEC_64;

    int rcTimer = timer_settime(pVCpu->nem.s.pTimer, 0 /* relative timeout */,
                                    &timeout, nullptr);
    AssertLogRel(rcTimer == 0);

    return VINF_SUCCESS;
}

static VBOXSTRICTRC nemHCLnxCheckAndInjectInterrupts(PVMCPUCC pVCpu)
{
#ifdef VBOX_WITH_KVM_IRQCHIP_FULL
    NOREF(pVCpu);
    AssertLogRelMsg(!TRPMHasTrap(pVCpu), ("TRPM has trap but full irqchip is enabled"));
    return VINF_SUCCESS;
#else
    if (TRPMHasTrap(pVCpu))
    {
        TRPMEVENT enmType = TRPM_32BIT_HACK;
        uint8_t   bTrapNo = 0;
        TRPMQueryTrap(pVCpu, &bTrapNo, &enmType);
        Log(("nemHCLnxCheckAndInjectInterrupts: Pending trap: bTrapNo=%#x enmType=%d\n", bTrapNo, enmType));
        if (enmType == TRPM_HARDWARE_INT)
        {
            struct kvm_interrupt kvm_int;
            RT_ZERO(kvm_int);
            kvm_int.irq = bTrapNo;
            int rcLnx = ioctl(pVCpu->nem.s.fdVCpu, KVM_INTERRUPT, &kvm_int);
            AssertLogRelMsgReturn(rcLnx == 0, ("rcLnx=%d errno=%d\n", rcLnx, errno), VERR_NEM_IPE_5);

            TRPMResetTrap(pVCpu);
        }
        else
        {
            return VERR_NOT_SUPPORTED;
        }

    }
    return VINF_SUCCESS;
#endif
}


VBOXSTRICTRC nemR3NativeRunGC(PVM pVM, PVMCPU pVCpu)
{
    /*
     * Try switch to NEM runloop state.
     */
    if (VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC_NEM, VMCPUSTATE_STARTED))
    { /* likely */ }
    else
    {
        VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC_NEM, VMCPUSTATE_STARTED_EXEC_NEM_CANCELED);
        LogFlow(("NEM/%u: returning immediately because canceled\n", pVCpu->idCpu));
        return VINF_SUCCESS;
    }

    /*
     * The first time we come here, we have to apply Spectre mitigations. The prctl interface only allows us to set
     * these only for the current thread.
     */
    if (!pVCpu->nem.s.fMitigationsApplied) {
        Log(("NEM/%u: applying mitigations\n", pVCpu->idCpu));
        if (pVM->hm.s.fIbpbOnVmEntry || pVM->hm.s.fIbpbOnVmExit) {
            int rcLnx = prctl(PR_SET_SPECULATION_CTRL, PR_SPEC_INDIRECT_BRANCH, PR_SPEC_FORCE_DISABLE, 0, 0);

            if (rcLnx != 0 && errno == EPERM) {
                LogRel(("WARNING: requested IBPB, but kernel API is not activated! Boot Linux with spectre_v2_user=prctl.\n", pVCpu->idCpu));
            } else {
                AssertLogRelMsgReturn(rcLnx == 0,
                                      ("rcLnx=%d errno=%d\n", rcLnx, errno),
                                      VERR_NEM_MISSING_KERNEL_API_1);
                Log(("NEM/%u: enabled IBPB\n", pVCpu->idCpu));
            }
        }

        pVCpu->nem.s.fMitigationsApplied = true;
    }

    /*
     * The run loop.
     */
    struct kvm_run * const  pRun                = pVCpu->nem.s.pRun;
    const bool              fSingleStepping     = DBGFIsStepping(pVCpu);
    VBOXSTRICTRC            rcStrict            = VINF_SUCCESS;
    bool                    fStatefulExit       = false;  /* For MMIO and IO exits. */
    for (unsigned iLoop = 0;; iLoop++)
    {
        /*
         * Pending interrupts or such?  Need to check and deal with this prior
         * to the state syncing.
         */
        if (VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_UPDATE_APIC | VMCPU_FF_INTERRUPT_PIC
                                     | VMCPU_FF_INTERRUPT_NMI  | VMCPU_FF_INTERRUPT_SMI))
        {
            /* Try inject interrupt. */
            rcStrict = nemHCLnxHandleInterruptFF(pVM, pVCpu, pRun);
            if (rcStrict == VINF_SUCCESS)
            { /* likely */ }
            else
            {
                LogFlow(("NEM/%u: breaking: nemHCLnxHandleInterruptFF -> %Rrc\n", pVCpu->idCpu, VBOXSTRICTRC_VAL(rcStrict) ));
                STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatBreakOnStatus);
                break;
            }
        }

    // See NEMR3CanExecuteGuest for details why we ignore A20 at this point.
#ifndef VBOX_WITH_KVM_IRQCHIP_FULL
        /*
         * Do not execute in KVM if the A20 isn't enabled.
         */
        if (PGMPhysIsA20Enabled(pVCpu))
        { /* likely */ }
        else
        {
            rcStrict = VINF_EM_RESCHEDULE_REM;
            LogFlow(("NEM/%u: breaking: A20 disabled\n", pVCpu->idCpu));
            break;
        }
#endif

        /*
         * Ensure KVM has the whole state.
         */
        if ((pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_ALL) != CPUMCTX_EXTRN_ALL)
        {
            int rc2 = nemHCLnxExportState(pVM, pVCpu, &pVCpu->cpum.GstCtx, pRun);
            AssertRCReturn(rc2, rc2);
        }

        /* Poll timers and run for a bit. */
        nemHCLnxHandleTimers(pVM, pVCpu);

        if (   !VM_FF_IS_ANY_SET(pVM, VM_FF_EMT_RENDEZVOUS | VM_FF_TM_VIRTUAL_SYNC)
            && !VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_HM_TO_R3_MASK))
        {
            if (VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC_NEM_WAIT, VMCPUSTATE_STARTED_EXEC_NEM))
            {
                LogFlow(("NEM/%u: Entry @ %04x:%08RX64 IF=%d EFL=%#RX64 SS:RSP=%04x:%08RX64 cr0=%RX64\n",
                         pVCpu->idCpu, pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip,
                         !!(pRun->s.regs.regs.rflags & X86_EFL_IF), pRun->s.regs.regs.rflags,
                         pRun->s.regs.sregs.ss.selector, pRun->s.regs.regs.rsp, pRun->s.regs.sregs.cr0));

                VBOXSTRICTRC rc2 = nemHCLnxCheckAndInjectInterrupts(pVCpu);
                AssertLogRelMsg(RT_SUCCESS(rc2), ("Failed to inject interrupt"));

                TMNotifyStartOfExecution(pVM, pVCpu);

                uint64_t const uApicBase = APICGetBaseMsrNoCheck(pVCpu);
                pRun->apic_base = uApicBase;
                int rcLnx = ioctl(pVCpu->nem.s.fdVCpu, KVM_RUN, 0UL);
                int errno_ = errno;

                VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC_NEM, VMCPUSTATE_STARTED_EXEC_NEM_WAIT);
                TMNotifyEndOfExecution(pVM, pVCpu, ASMReadTSC());

                pVCpu->nem.s.pRun->immediate_exit = 0;

#ifdef LOG_ENABLED
                if (LogIsFlowEnabled())
                {
                    struct kvm_mp_state MpState = {UINT32_MAX};
                    ioctl(pVCpu->nem.s.fdVCpu, KVM_GET_MP_STATE, &MpState);
                    LogFlow(("NEM/%u: Exit  @ %04x:%08RX64 IF=%d EFL=%#RX64 CR8=%#x Reason=%#x IrqReady=%d Flags=%#x %#lx\n", pVCpu->idCpu,
                             pRun->s.regs.sregs.cs.selector, pRun->s.regs.regs.rip, pRun->if_flag,
                             pRun->s.regs.regs.rflags, pRun->s.regs.sregs.cr8, pRun->exit_reason,
                             pRun->ready_for_interrupt_injection, pRun->flags, MpState.mp_state));
                }
#endif
                fStatefulExit = false;
                if (RT_LIKELY(rcLnx == 0 || errno_ == EINTR))
                {
                    /*
                     * Deal with the exit.
                     */
                    rcStrict = nemHCLnxHandleExit(pVM, pVCpu, pRun, &fStatefulExit);
                    if (rcStrict == VINF_SUCCESS)
                    { /* hopefully likely */ }
                    else
                    {
                        LogFlow(("NEM/%u: breaking: nemHCLnxHandleExit -> %Rrc\n", pVCpu->idCpu, VBOXSTRICTRC_VAL(rcStrict) ));
                        STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatBreakOnStatus);
                        break;
                    }
                }
                else if (errno_ == EAGAIN) {
                    /*
                    * We might drop out of KVM_RUN if the vCPU is still in an
                    * uninitialized state (e.g. WAIT_FOR_INIT) and some spurious
                    * wakeup event is received. In this case, simply do nothing
                    * and let the run loop enter KVM_RUN again.
                    * See https://elixir.bootlin.com/linux/v6.6/source/arch/x86/kvm/x86.c#L11138
                    */
                }
                else
                {
                    rc2 = RTErrConvertFromErrno(errno_);
                    AssertLogRelMsgFailedReturn(("KVM_RUN failed: rcLnx=%d errno=%u rc=%Rrc\n", rcLnx, errno_, rc2), rc2);
                }

                /*
                 * If no relevant FFs are pending, loop.
                 */
                if (   !VM_FF_IS_ANY_SET(   pVM,   !fSingleStepping ? VM_FF_HP_R0_PRE_HM_MASK    : VM_FF_HP_R0_PRE_HM_STEP_MASK)
                    && !VMCPU_FF_IS_ANY_SET(pVCpu, !fSingleStepping ? VMCPU_FF_HP_R0_PRE_HM_MASK : VMCPU_FF_HP_R0_PRE_HM_STEP_MASK) )
                { /* likely */ }
                else
                {

                    /** @todo Try handle pending flags, not just return to EM loops.  Take care
                     *        not to set important RCs here unless we've handled an exit. */
                    LogFlow(("NEM/%u: breaking: pending FF (%#x / %#RX64)\n",
                             pVCpu->idCpu, pVM->fGlobalForcedActions, (uint64_t)pVCpu->fLocalForcedActions));
                    STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatBreakOnFFPost);
                    break;
                }
            }
            else
            {
                LogFlow(("NEM/%u: breaking: canceled %d (pre exec)\n", pVCpu->idCpu, VMCPU_GET_STATE(pVCpu) ));
                STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatBreakOnCancel);
                break;
            }
        }
        else
        {
            LogFlow(("NEM/%u: breaking: pending FF (pre exec)\n", pVCpu->idCpu));
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatBreakOnFFPre);
            break;
        }
    } /* the run loop */


    /*
     * If the last exit was stateful, commit the state we provided before
     * returning to the EM loop so we have a consistent state and can safely
     * be rescheduled and whatnot.  This may require us to make multiple runs
     * for larger MMIO and I/O operations. Sigh^3.
     *
     * Note! There is no 'ing way to reset the kernel side completion callback
     *       for these stateful i/o exits.  Very annoying interface.
     */
    /** @todo check how this works with string I/O and string MMIO. */
    if (fStatefulExit && RT_SUCCESS(rcStrict))
    {
        STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatFlushExitOnReturn);
        uint32_t const uOrgExit = pRun->exit_reason;
        for (uint32_t i = 0; ; i++)
        {
            pRun->immediate_exit = 1;
            int rcLnx = ioctl(pVCpu->nem.s.fdVCpu, KVM_RUN, 0UL);
            Log(("NEM/%u: Flushed stateful exit -> %d/%d exit_reason=%d\n", pVCpu->idCpu, rcLnx, errno, pRun->exit_reason));
            if (rcLnx == -1 && errno == EINTR)
            {
                switch (i)
                {
                    case 0: STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatFlushExitOnReturn1Loop); break;
                    case 1: STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatFlushExitOnReturn2Loops); break;
                    case 2: STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatFlushExitOnReturn3Loops); break;
                    default: STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatFlushExitOnReturn4PlusLoops); break;
                }
                break;
            }
            AssertLogRelMsgBreakStmt(rcLnx == 0 && pRun->exit_reason == uOrgExit,
                                     ("rcLnx=%d errno=%d exit_reason=%d uOrgExit=%d\n", rcLnx, errno, pRun->exit_reason, uOrgExit),
                                     rcStrict = VERR_NEM_IPE_6);
            VBOXSTRICTRC rcStrict2 = nemHCLnxHandleExit(pVM, pVCpu, pRun, &fStatefulExit);
            if (rcStrict2 == VINF_SUCCESS || rcStrict2 == rcStrict)
            { /* likely */ }
            else if (RT_FAILURE(rcStrict2))
            {
                rcStrict = rcStrict2;
                break;
            }
            else
            {
                AssertLogRelMsgBreakStmt(rcStrict == VINF_SUCCESS,
                                         ("rcStrict=%Rrc rcStrict2=%Rrc\n", VBOXSTRICTRC_VAL(rcStrict), VBOXSTRICTRC_VAL(rcStrict2)),
                                         rcStrict = VERR_NEM_IPE_7);
                rcStrict = rcStrict2;
            }
        }
        pRun->immediate_exit = 0;
    }

    /*
     * If the CPU is running, make sure to stop it before we try sync back the
     * state and return to EM.  We don't sync back the whole state if we can help it.
     */
    if (!VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED, VMCPUSTATE_STARTED_EXEC_NEM))
        VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED, VMCPUSTATE_STARTED_EXEC_NEM_CANCELED);

    if (pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_ALL)
    {
        /* Try anticipate what we might need. */
        uint64_t fImport = CPUMCTX_EXTRN_INHIBIT_INT | CPUMCTX_EXTRN_INHIBIT_NMI /* Required for processing APIC,PIC,NMI & SMI FFs. */
                         | IEM_CPUMCTX_EXTRN_MUST_MASK /*?*/;
        if (   (rcStrict >= VINF_EM_FIRST && rcStrict <= VINF_EM_LAST)
            || RT_FAILURE(rcStrict))
            fImport = CPUMCTX_EXTRN_ALL;
# ifdef IN_RING0 /* Ring-3 I/O port access optimizations: */
        else if (   rcStrict == VINF_IOM_R3_IOPORT_COMMIT_WRITE
                 || rcStrict == VINF_EM_PENDING_R3_IOPORT_WRITE)
            fImport = CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_CS | CPUMCTX_EXTRN_RFLAGS;
        else if (rcStrict == VINF_EM_PENDING_R3_IOPORT_READ)
            fImport = CPUMCTX_EXTRN_RAX | CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_CS | CPUMCTX_EXTRN_RFLAGS;
# endif
        else if (VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_INTERRUPT_PIC | VMCPU_FF_INTERRUPT_APIC
                                          | VMCPU_FF_INTERRUPT_NMI | VMCPU_FF_INTERRUPT_SMI))
            fImport |= IEM_CPUMCTX_EXTRN_XCPT_MASK;

        if (pVCpu->cpum.GstCtx.fExtrn & fImport)
        {
            int rc2 = nemHCLnxImportState(pVCpu, fImport, &pVCpu->cpum.GstCtx, pRun);
            if (RT_SUCCESS(rc2))
                pVCpu->cpum.GstCtx.fExtrn &= ~fImport;
            else if (RT_SUCCESS(rcStrict))
                rcStrict = rc2;
            if (!(pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_ALL))
                pVCpu->cpum.GstCtx.fExtrn = 0;
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatImportOnReturn);
        }
        else
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatImportOnReturnSkipped);
    }
    else
    {
        pVCpu->cpum.GstCtx.fExtrn = 0;
        STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatImportOnReturnSkipped);
    }

    LogFlow(("NEM/%u: %04x:%08RX64 efl=%#08RX64 => %Rrc\n", pVCpu->idCpu, pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip,
             pVCpu->cpum.GstCtx.rflags.u, VBOXSTRICTRC_VAL(rcStrict) ));
    return rcStrict;
}


/** @page pg_nem_linux NEM/linux - Native Execution Manager, Linux.
 *
 * This is using KVM.
 *
 */
