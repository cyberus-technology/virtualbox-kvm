/* $Id: GIMHvOnKvm.cpp $ */
/** @file
 * GIM - Guest Interface Manager, Hyper-V implementation for the KVM-Backend.
 */

/*
 * Copyright (C) 2014-2023 Oracle and/or its affiliates.
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
#define LOG_GROUP LOG_GROUP_GIM
#include <VBox/vmm/gim.h>
#include <VBox/vmm/nem.h>
#include <VBox/vmm/ssm.h>
#include <VBox/vmm/hm.h>
#include "GIMInternal.h"
#include <VBox/vmm/vm.h>

#include <VBox/err.h>
#include <VBox/version.h>

#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/mem.h>

/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/**
 * GIM Hyper-V saved-state version.
 *
 * We use a number that is far away from the original GIMHv saved state version
 * to prevent future collisions.
 */
#define GIM_HV_SAVED_STATE_VERSION                      UINT32_C(0x1000)

#ifdef VBOX_WITH_STATISTICS
# define GIMHV_MSRRANGE(a_uFirst, a_uLast, a_szName) \
    { (a_uFirst), (a_uLast), kCpumMsrRdFn_Gim, kCpumMsrWrFn_Gim, 0, 0, 0, 0, 0, a_szName, { 0 }, { 0 }, { 0 }, { 0 } }
#else
# define GIMHV_MSRRANGE(a_uFirst, a_uLast, a_szName) \
    { (a_uFirst), (a_uLast), kCpumMsrRdFn_Gim, kCpumMsrWrFn_Gim, 0, 0, 0, 0, 0, a_szName }
#endif


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/**
 * Array of MSR ranges supported by Hyper-V.
 */
static CPUMMSRRANGE const g_aMsrRanges_HyperV[] =
{
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE0_FIRST,  MSR_GIM_HV_RANGE0_LAST,  "Hyper-V range 0"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE1_FIRST,  MSR_GIM_HV_RANGE1_LAST,  "Hyper-V range 1"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE2_FIRST,  MSR_GIM_HV_RANGE2_LAST,  "Hyper-V range 2"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE3_FIRST,  MSR_GIM_HV_RANGE3_LAST,  "Hyper-V range 3"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE4_FIRST,  MSR_GIM_HV_RANGE4_LAST,  "Hyper-V range 4"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE5_FIRST,  MSR_GIM_HV_RANGE5_LAST,  "Hyper-V range 5"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE6_FIRST,  MSR_GIM_HV_RANGE6_LAST,  "Hyper-V range 6"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE7_FIRST,  MSR_GIM_HV_RANGE7_LAST,  "Hyper-V range 7"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE8_FIRST,  MSR_GIM_HV_RANGE8_LAST,  "Hyper-V range 8"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE9_FIRST,  MSR_GIM_HV_RANGE9_LAST,  "Hyper-V range 9"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE10_FIRST, MSR_GIM_HV_RANGE10_LAST, "Hyper-V range 10"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE11_FIRST, MSR_GIM_HV_RANGE11_LAST, "Hyper-V range 11"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE12_FIRST, MSR_GIM_HV_RANGE12_LAST, "Hyper-V range 12")
};
#undef GIMHV_MSRRANGE

/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/

/**
 * Initializes the Hyper-V GIM provider.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pGimCfg     The GIM CFGM node.
 */
VMMR3_INT_DECL(int) gimR3HvInit(PVM pVM, PCFGMNODE pGimCfg)
{
    AssertReturn(pVM, VERR_INVALID_PARAMETER);
    AssertReturn(pVM->gim.s.enmProviderId == GIMPROVIDERID_HYPERV, VERR_INTERNAL_ERROR_5);

    PGIMHV pHv = &pVM->gim.s.u.Hv;

    /*
     * Read configuration.
     */
    PCFGMNODE pCfgHv = CFGMR3GetChild(pGimCfg, "HyperV");
    if (pCfgHv)
    {
        /*
         * Validate the Hyper-V settings.
         */
        int rc2 = CFGMR3ValidateConfig(pCfgHv, "/HyperV/",
                                  "VendorID"
                                  "|VSInterface"
                                  "|HypercallDebugInterface"
                                  "|VirtioGPU",
                                  "" /* pszValidNodes */, "GIM/HyperV" /* pszWho */, 0 /* uInstance */);
        if (RT_FAILURE(rc2))
            return rc2;
    }

    /** @cfgm{/GIM/HyperV/VendorID, string, 'VBoxVBoxVBox'}
     * The Hyper-V vendor signature, must be 12 characters. */
    char szVendor[13];
    int rc = CFGMR3QueryStringDef(pCfgHv, "VendorID", szVendor, sizeof(szVendor), "VBoxVBoxVBox");
    AssertLogRelRCReturn(rc, rc);
    AssertLogRelMsgReturn(strlen(szVendor) == 12,
                          ("The VendorID config value must be exactly 12 chars, '%s' isn't!\n", szVendor),
                          VERR_INVALID_PARAMETER);

    LogRel(("GIM: HyperV: Reporting vendor as '%s'\n", szVendor));

    AssertReleaseMsg(!RTStrNCmp(szVendor, GIM_HV_VENDOR_VBOX, sizeof(GIM_HV_VENDOR_VBOX) - 1), (("GIM Vendors other than VBox are unsupported")));

    pHv->fIsInterfaceVs = false;
    pHv->fDbgHypercallInterface = false;

    uint32_t uKvmBaseFeat = 0;
    uint32_t uKvmPartFlags = 0;
    uint32_t uKvmPowMgmtFeat = 0;
    uint32_t uKvmMiscFeat = 0;
    uint32_t uKvmHyperHints = 0;

    {
        PCPUMCPUIDLEAF pKvmCpuidLeaves = nullptr;
        size_t cKvmCpuidLeaves = 0;

        rc = NEMR3KvmGetHvCpuIdLeaves(pVM, &pKvmCpuidLeaves, &cKvmCpuidLeaves);
        AssertLogRelRCReturn(rc, rc);

        for (size_t uLeaf = 0; uLeaf < cKvmCpuidLeaves; uLeaf++) {
            LogRel(("GIM: KVM CPUID[%08x] eax=%08x ebx=%08x ecx=%08x edx=%08x\n",
                    pKvmCpuidLeaves[uLeaf].uLeaf,
                    pKvmCpuidLeaves[uLeaf].uEax, pKvmCpuidLeaves[uLeaf].uEbx,
                    pKvmCpuidLeaves[uLeaf].uEcx, pKvmCpuidLeaves[uLeaf].uEdx));

            /*
              See this documentation for an overview of Hyper-V CPUID flags:
              https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/feature-discovery
             */

            switch (pKvmCpuidLeaves[uLeaf].uLeaf) {
            case 0x40000003: /* Features */
                uKvmBaseFeat = pKvmCpuidLeaves[uLeaf].uEax;
                uKvmPartFlags = pKvmCpuidLeaves[uLeaf].uEbx;
                uKvmPowMgmtFeat = pKvmCpuidLeaves[uLeaf].uEcx;
                uKvmMiscFeat = pKvmCpuidLeaves[uLeaf].uEdx;
                break;
            case 0x40000004: /* Implementation Recommendations */
                uKvmHyperHints = pKvmCpuidLeaves[uLeaf].uEax;
                break;
            default:
                // Ignore
                break;
            }
        }

        RTMemFree(pKvmCpuidLeaves);
    }

    /*
     * Determine interface capabilities based on the version.
     */
    if (!pVM->gim.s.u32Version)
    {
        /* Basic features. */
        pHv->uBaseFeat = 0
                       | GIM_HV_BASE_FEAT_VP_RUNTIME_MSR
                       | GIM_HV_BASE_FEAT_PART_TIME_REF_COUNT_MSR
                       | GIM_HV_BASE_FEAT_BASIC_SYNIC_MSRS
                       | GIM_HV_BASE_FEAT_STIMER_MSRS
                       | GIM_HV_BASE_FEAT_APIC_ACCESS_MSRS
                       | GIM_HV_BASE_FEAT_HYPERCALL_MSRS
                       | GIM_HV_BASE_FEAT_VP_ID_MSR
                       | GIM_HV_BASE_FEAT_VIRT_SYS_RESET_MSR
                     //| GIM_HV_BASE_FEAT_STAT_PAGES_MSR
                       | GIM_HV_BASE_FEAT_PART_REF_TSC_MSR
                     //| GIM_HV_BASE_FEAT_GUEST_IDLE_STATE_MSR
                       | GIM_HV_BASE_FEAT_TIMER_FREQ_MSRS
                     //| GIM_HV_BASE_FEAT_DEBUG_MSRS
                       ;

        /* Miscellaneous features. */
        pHv->uMiscFeat = 0
                       //| GIM_HV_MISC_FEAT_GUEST_DEBUGGING
                       //| GIM_HV_MISC_FEAT_XMM_HYPERCALL_INPUT
                         | GIM_HV_MISC_FEAT_TIMER_FREQ
                         | GIM_HV_MISC_FEAT_GUEST_CRASH_MSRS
                       //| GIM_HV_MISC_FEAT_DEBUG_MSRS
                         | GIM_HV_MISC_FEAT_USE_DIRECT_SYNTH_MSRS
                         ;

        /* Hypervisor recommendations to the guest. */
        pHv->uHyperHints = GIM_HV_HINT_RELAX_TIME_CHECKS
                         /* Causes assertion failures in interrupt injection. */
                       //| GIM_HV_HINT_MSR_FOR_APIC_ACCESS
                       //|GIM_HV_HINT_MSR_FOR_SYS_RESET
                         | GIM_HV_HINT_DEPRECATE_AUTO_EOI
                         /* Inform the guest whether the host has hyperthreading disabled. */
                         | (GIM_HV_HINT_NO_NONARCH_CORESHARING & uKvmHyperHints)
                         ;


        // We should not enable features and hints that KVM doesn't know about.
        AssertRelease((pHv->uHyperHints & ~uKvmHyperHints) == 0);
        AssertRelease((pHv->uBaseFeat & ~uKvmBaseFeat) == 0);
        AssertRelease((pHv->uMiscFeat & ~uKvmMiscFeat) == 0);
        AssertRelease((pHv->uPartFlags & ~uKvmPartFlags) == 0);
        AssertRelease((pHv->uPowMgmtFeat & ~uKvmPowMgmtFeat) == 0);
    }

    /*
     * Make sure the CPUID bits are in accordance with the Hyper-V
     * requirement and other paranoia checks.
     * See "Requirements for implementing the Microsoft hypervisor interface" spec.
     */
    AssertRelease(!(pHv->uPartFlags & (  GIM_HV_PART_FLAGS_CREATE_PART
                                        | GIM_HV_PART_FLAGS_ACCESS_MEMORY_POOL
                                        | GIM_HV_PART_FLAGS_ACCESS_PART_ID
                                        | GIM_HV_PART_FLAGS_ADJUST_MSG_BUFFERS
                                        | GIM_HV_PART_FLAGS_CREATE_PORT
                                        | GIM_HV_PART_FLAGS_ACCESS_STATS
                                        | GIM_HV_PART_FLAGS_CPU_MGMT
                                        | GIM_HV_PART_FLAGS_CPU_PROFILER)));

    AssertRelease((pHv->uBaseFeat & (GIM_HV_BASE_FEAT_HYPERCALL_MSRS | GIM_HV_BASE_FEAT_VP_ID_MSR))
            == (GIM_HV_BASE_FEAT_HYPERCALL_MSRS | GIM_HV_BASE_FEAT_VP_ID_MSR));

    /*
     * Expose HVP (Hypervisor Present) bit to the guest.
     */
    CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_HVP);

    /*
     * Modify the standard hypervisor leaves for Hyper-V.
     */
    CPUMCPUIDLEAF HyperLeaf;
    RT_ZERO(HyperLeaf);
    HyperLeaf.uLeaf = UINT32_C(0x40000000);
    HyperLeaf.uEax  = UINT32_C(0x40000006); /* Minimum value for Hyper-V default is 0x40000005. */
    /*
     * Don't report vendor as 'Microsoft Hv'[1] by default, see @bugref{7270#c152}.
     * [1]: ebx=0x7263694d ('rciM') ecx=0x666f736f ('foso') edx=0x76482074 ('vH t')
     */
    {
        uint32_t uVendorEbx;
        uint32_t uVendorEcx;
        uint32_t uVendorEdx;
        uVendorEbx = ((uint32_t)szVendor[ 3]) << 24 | ((uint32_t)szVendor[ 2]) << 16 | ((uint32_t)szVendor[1]) << 8
                    | (uint32_t)szVendor[ 0];
        uVendorEcx = ((uint32_t)szVendor[ 7]) << 24 | ((uint32_t)szVendor[ 6]) << 16 | ((uint32_t)szVendor[5]) << 8
                    | (uint32_t)szVendor[ 4];
        uVendorEdx = ((uint32_t)szVendor[11]) << 24 | ((uint32_t)szVendor[10]) << 16 | ((uint32_t)szVendor[9]) << 8
                    | (uint32_t)szVendor[ 8];
        HyperLeaf.uEbx         = uVendorEbx;
        HyperLeaf.uEcx         = uVendorEcx;
        HyperLeaf.uEdx         = uVendorEdx;
    }
    rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
    AssertLogRelRCReturn(rc, rc);

    HyperLeaf.uLeaf        = UINT32_C(0x40000001);
    HyperLeaf.uEax         = 0x31237648;           /* 'Hv#1' */
    HyperLeaf.uEbx         = 0;                    /* Reserved */
    HyperLeaf.uEcx         = 0;                    /* Reserved */
    HyperLeaf.uEdx         = 0;                    /* Reserved */
    rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
    AssertLogRelRCReturn(rc, rc);

    /*
     * Add Hyper-V specific leaves.
     */
    HyperLeaf.uLeaf        = UINT32_C(0x40000002); /* MBZ until MSR_GIM_HV_GUEST_OS_ID is set by the guest. */
    HyperLeaf.uEax         = 0;
    HyperLeaf.uEbx         = 0;
    HyperLeaf.uEcx         = 0;
    HyperLeaf.uEdx         = 0;
    rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
    AssertLogRelRCReturn(rc, rc);

    HyperLeaf.uLeaf        = UINT32_C(0x40000003);
    HyperLeaf.uEax         = pHv->uBaseFeat;
    HyperLeaf.uEbx         = pHv->uPartFlags;
    HyperLeaf.uEcx         = pHv->uPowMgmtFeat;
    HyperLeaf.uEdx         = pHv->uMiscFeat;
    rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
    AssertLogRelRCReturn(rc, rc);

    HyperLeaf.uLeaf        = UINT32_C(0x40000004);
    HyperLeaf.uEax         = pHv->uHyperHints;
    /* Recommended number of spinlock retries before notifying the Hypervisor. 0xffffffff means that the Hypervisor is never notified */
    HyperLeaf.uEbx         = 0xffffffff;
    HyperLeaf.uEcx         = 0;
    HyperLeaf.uEdx         = 0;
    rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
    AssertLogRelRCReturn(rc, rc);

    RT_ZERO(HyperLeaf);
    HyperLeaf.uLeaf        = UINT32_C(0x40000005);
    rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
    AssertLogRelRCReturn(rc, rc);

    /*
     * Insert all MSR ranges of Hyper-V.
     */
    for (unsigned i = 0; i < RT_ELEMENTS(g_aMsrRanges_HyperV); i++)
    {
        int rc2 = CPUMR3MsrRangesInsert(pVM, &g_aMsrRanges_HyperV[i]);
        AssertLogRelRCReturn(rc2, rc2);
    }

    /*
     * Setup non-zero MSRs.
     */
    if (pHv->uMiscFeat & GIM_HV_MISC_FEAT_GUEST_CRASH_MSRS)
        pHv->uCrashCtlMsr = MSR_GIM_HV_CRASH_CTL_NOTIFY;

    return VINF_SUCCESS;
}


/**
 * Initializes remaining bits of the Hyper-V provider.
 *
 * This is called after initializing HM and almost all other VMM components.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 */
VMMR3_INT_DECL(int) gimR3HvInitCompleted(PVM pVM)
{
    PGIMHV pHv = &pVM->gim.s.u.Hv;
    pHv->cTscTicksPerSecond = TMCpuTicksPerSecond(pVM);

    /*
     * Determine interface capabilities based on the version.
     */
    if (!pVM->gim.s.u32Version)
    {
        /* Hypervisor capabilities; features used by the hypervisor. */
        pHv->uHyperCaps  = HMIsNestedPagingActive(pVM) ? GIM_HV_HOST_FEAT_NESTED_PAGING : 0;
        pHv->uHyperCaps |= HMIsMsrBitmapActive(pVM)    ? GIM_HV_HOST_FEAT_MSR_BITMAP    : 0;
    }

    CPUMCPUIDLEAF HyperLeaf;
    RT_ZERO(HyperLeaf);
    HyperLeaf.uLeaf        = UINT32_C(0x40000006);
    HyperLeaf.uEax         = pHv->uHyperCaps;
    HyperLeaf.uEbx         = 0;
    HyperLeaf.uEcx         = 0;
    HyperLeaf.uEdx         = 0;
    int rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
    AssertLogRelRCReturn(rc, rc);

    return rc;
}


/**
 * Terminates the Hyper-V GIM provider.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3_INT_DECL(int) gimR3HvTerm(PVM pVM)
{
    gimR3HvReset(pVM);

    return VINF_SUCCESS;
}


/**
 * Applies relocations to data and code managed by this
 * component. This function will be called at init and
 * whenever the VMM need to relocate it self inside the GC.
 *
 * @param   pVM         The cross context VM structure.
 * @param   offDelta    Relocation delta relative to old location.
 */
VMMR3_INT_DECL(void) gimR3HvRelocate(PVM pVM, RTGCINTPTR offDelta)
{
    RT_NOREF(pVM, offDelta);
}


static bool isSynICAllowed(PGIMHV pHv)
{
    return pHv->uBaseFeat & GIM_HV_BASE_FEAT_BASIC_SYNIC_MSRS;
}

/**
 * This resets Hyper-V provider MSRs and unmaps whatever Hyper-V regions that
 * the guest may have mapped.
 *
 * This is called when the VM is being reset.
 *
 * @param   pVM     The cross context VM structure.
 *
 * @thread  EMT(0)
 */
VMMR3_INT_DECL(void) gimR3HvReset(PVM pVM)
{
    VM_ASSERT_EMT0(pVM);

    /*
     * Unmap MMIO2 pages that the guest may have setup.
     */
    LogRel(("GIM: HyperV: Resetting MMIO2 regions and MSRs\n"));
    PGIMHV pHv = &pVM->gim.s.u.Hv;

    /*
     * Reset MSRs.
     */
    pHv->u64GuestOsIdMsr      = 0;
    pHv->u64HypercallMsr      = 0;
    pHv->u64TscPageMsr        = 0;
    pHv->uCrashP0Msr          = 0;
    pHv->uCrashP1Msr          = 0;
    pHv->uCrashP2Msr          = 0;
    pHv->uCrashP3Msr          = 0;
    pHv->uCrashP4Msr          = 0;
    pHv->uDbgStatusMsr        = 0;
    pHv->uDbgPendingBufferMsr = 0;
    pHv->uDbgSendBufferMsr    = 0;
    pHv->uDbgRecvBufferMsr    = 0;

    PVMCPU pVCpuBsp = pVM->apCpusR3[0];
    NEMR3KvmSetMsr(pVCpuBsp, MSR_GIM_HV_GUEST_OS_ID, pHv->u64GuestOsIdMsr);
    NEMR3KvmSetMsr(pVCpuBsp, MSR_GIM_HV_HYPERCALL, pHv->u64HypercallMsr);
    NEMR3KvmSetMsr(pVCpuBsp, MSR_GIM_HV_REF_TSC, pHv->u64TscPageMsr);
    NEMR3KvmSetMsr(pVCpuBsp, MSR_GIM_HV_SYNTH_DEBUG_STATUS, pHv->uDbgStatusMsr);
    NEMR3KvmSetMsr(pVCpuBsp, MSR_GIM_HV_SYNTH_DEBUG_PENDING_BUFFER, pHv->uDbgPendingBufferMsr);
    NEMR3KvmSetMsr(pVCpuBsp, MSR_GIM_HV_SYNTH_DEBUG_SEND_BUFFER, pHv->uDbgSendBufferMsr);
    NEMR3KvmSetMsr(pVCpuBsp, MSR_GIM_HV_SYNTH_DEBUG_RECEIVE_BUFFER, pHv->uDbgRecvBufferMsr);

    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
    {
        PGIMHVCPU pHvCpu = &pVM->apCpusR3[idCpu]->gim.s.u.HvCpu;
        PVMCPU pVCpu = pVM->apCpusR3[idCpu];

        pHvCpu->uSControlMsr = 0;
        pHvCpu->uSimpMsr  = 0;
        pHvCpu->uSiefpMsr = 0;
        pHvCpu->uApicAssistPageMsr = 0;

        NEMR3KvmSetMsr(pVCpu, MSR_GIM_HV_SCONTROL, pHvCpu->uSControlMsr);
        NEMR3KvmSetMsr(pVCpu, MSR_GIM_HV_SIMP, pHvCpu->uSimpMsr);
        NEMR3KvmSetMsr(pVCpu, MSR_GIM_HV_SIEFP, pHvCpu->uSiefpMsr);
        NEMR3KvmSetMsr(pVCpu, MSR_GIM_HV_APIC_ASSIST_PAGE, pHvCpu->uApicAssistPageMsr);

        for (uint8_t idxSint = 0; idxSint < RT_ELEMENTS(pHvCpu->auSintMsrs); idxSint++) {
            pHvCpu->auSintMsrs[idxSint] = MSR_GIM_HV_SINT_MASKED;
            if (isSynICAllowed(pHv)) {
                NEMR3KvmSetMsr(pVCpu, MSR_GIM_HV_SINT0 + idxSint, pHvCpu->auSintMsrs[idxSint]);
            }
        }

        for (uint8_t idxStimer = 0; idxStimer < RT_ELEMENTS(pHvCpu->aStimers); idxStimer++)
        {
            PGIMHVSTIMER pHvStimer = &pHvCpu->aStimers[idxStimer];
            pHvStimer->uStimerConfigMsr = 0;
            pHvStimer->uStimerCountMsr  = 0;
            NEMR3KvmSetMsr(pVCpu, MSR_GIM_HV_STIMER0_CONFIG + idxStimer, pHvStimer->uStimerConfigMsr);
            NEMR3KvmSetMsr(pVCpu, MSR_GIM_HV_STIMER0_COUNT + idxStimer, pHvStimer->uStimerCountMsr);
        }
    }
}


/**
 * Hyper-V state-load operation, final pass.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pSSM            The saved state handle.
 */
VMMR3_INT_DECL(int) gimR3HvLoad(PVM pVM, PSSMHANDLE pSSM)
{
    uint32_t uHvSavedStateVersion;
    int rc = SSMR3GetU32(pSSM, &uHvSavedStateVersion);
    AssertRCReturn(rc, rc);

    if (uHvSavedStateVersion != GIM_HV_SAVED_STATE_VERSION) {
        return SSMR3SetLoadError(pSSM, VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION, RT_SRC_POS,
                                 N_("Unsupported Hyper-V saved-state version %u (current %u)!"),
                                 uHvSavedStateVersion, GIM_HV_SAVED_STATE_VERSION);
    }

    for (unsigned i = 0; i < RT_ELEMENTS(g_aMsrRanges_HyperV); i++) {
        for (unsigned msr {g_aMsrRanges_HyperV[i].uFirst}; msr <= g_aMsrRanges_HyperV[i].uLast; ++msr) {

            // See gimR3HvSave to understand why we skip this MSR.
            if (msr == MSR_GIM_HV_EOI) {
                continue;
            }

            uint64_t val {0};
            PVMCPU pVCpu = pVM->apCpusR3[0];

            SSMR3GetU64(pSSM, &val);

            rc = NEMR3KvmSetMsr(pVCpu, msr, val);
            if (rc != VINF_SUCCESS) {
                // Some MSRs can only be written when HYPERV_SYINC2 has been enabled.
                // We don't actually care here because if we unable to write the MSR,
                // the guest couldn't have read/written it either.
                LogRel2(("Unable to read HV MSR: 0x%x\n", msr));
            }
        }
    }

    return VINF_SUCCESS;
}


/**
 * Hyper-V load-done callback.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pSSM            The saved state handle.
 */
VMMR3_INT_DECL(int) gimR3HvLoadDone(PVM pVM, PSSMHANDLE pSSM)
{
    NOREF(pVM); NOREF(pSSM);
    return VINF_SUCCESS;
}

/**
 * Hyper-V state-save operation.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 * @param   pSSM    The saved state handle.
 */
VMMR3_INT_DECL(int) gimR3HvSave(PVM pVM, PSSMHANDLE pSSM)
{
    /*
     * Save the Hyper-V SSM version.
     */
    SSMR3PutU32(pSSM, GIM_HV_SAVED_STATE_VERSION);

    for (unsigned i = 0; i < RT_ELEMENTS(g_aMsrRanges_HyperV); i++) {
        for (unsigned msr {g_aMsrRanges_HyperV[i].uFirst}; msr <= g_aMsrRanges_HyperV[i].uLast; ++msr) {

            // This register is wirte-only for the guest and the last value written isn't interesting at all.
            // Thus, there is no need save it here.
            if (msr == MSR_GIM_HV_EOI) {
                continue;
            }

            uint64_t val {0};
            PVMCPU pVCpu = pVM->apCpusR3[0];

            int rc {NEMR3KvmGetMsr(pVCpu, msr, &val)};
            if (rc != VINF_SUCCESS) {
                // Some MSRs can only be read when HYPERV_SYINC2 has been enabled.
                // We don't actually care here because if we unable to read the MSR,
                // the guest couldn't have read/written it either. Simply save it as
                // zero and call it good.
                LogRel2(("Unable to read HV MSR: 0x%x\n", msr));
            }

            SSMR3PutU64(pSSM, val);
        }
    }

    return VINF_SUCCESS;
}

/**
 * Get Hyper-V debug setup parameters.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pDbgSetup   Where to store the debug setup details.
 */
VMMR3_INT_DECL(int) gimR3HvGetDebugSetup(PVM pVM, PGIMDEBUGSETUP pDbgSetup)
{
    NOREF(pVM); NOREF(pDbgSetup);
    return VERR_GIM_NO_DEBUG_CONNECTION;
}
