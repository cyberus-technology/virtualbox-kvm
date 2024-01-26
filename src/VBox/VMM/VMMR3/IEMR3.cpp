/* $Id: IEMR3.cpp $ */
/** @file
 * IEM - Interpreted Execution Manager.
 */

/*
 * Copyright (C) 2011-2023 Oracle and/or its affiliates.
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
#define LOG_GROUP LOG_GROUP_EM
#include <VBox/vmm/iem.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/mm.h>
#include "IEMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/vmapi.h>
#include <VBox/err.h>
#ifdef VBOX_WITH_DEBUGGER
# include <VBox/dbg.h>
#endif

#include <iprt/assert.h>
#include <iprt/getopt.h>
#include <iprt/string.h>


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static FNDBGFINFOARGVINT iemR3InfoITlb;
static FNDBGFINFOARGVINT iemR3InfoDTlb;
#ifdef VBOX_WITH_DEBUGGER
static void iemR3RegisterDebuggerCommands(void);
#endif


static const char *iemGetTargetCpuName(uint32_t enmTargetCpu)
{
    switch (enmTargetCpu)
    {
#define CASE_RET_STR(enmValue) case enmValue: return #enmValue + (sizeof("IEMTARGETCPU_") - 1)
        CASE_RET_STR(IEMTARGETCPU_8086);
        CASE_RET_STR(IEMTARGETCPU_V20);
        CASE_RET_STR(IEMTARGETCPU_186);
        CASE_RET_STR(IEMTARGETCPU_286);
        CASE_RET_STR(IEMTARGETCPU_386);
        CASE_RET_STR(IEMTARGETCPU_486);
        CASE_RET_STR(IEMTARGETCPU_PENTIUM);
        CASE_RET_STR(IEMTARGETCPU_PPRO);
        CASE_RET_STR(IEMTARGETCPU_CURRENT);
#undef CASE_RET_STR
        default: return "Unknown";
    }
}


/**
 * Initializes the interpreted execution manager.
 *
 * This must be called after CPUM as we're quering information from CPUM about
 * the guest and host CPUs.
 *
 * @returns VBox status code.
 * @param   pVM                The cross context VM structure.
 */
VMMR3DECL(int)      IEMR3Init(PVM pVM)
{
    int rc;

    /*
     * Read configuration.
     */
    PCFGMNODE pIem = CFGMR3GetChild(CFGMR3GetRoot(pVM), "IEM");

#ifndef VBOX_WITHOUT_CPUID_HOST_CALL
    /** @cfgm{/IEM/CpuIdHostCall, boolean, false}
     * Controls whether the custom VBox specific CPUID host call interface is
     * enabled or not. */
# ifdef DEBUG_bird
    rc = CFGMR3QueryBoolDef(pIem, "CpuIdHostCall", &pVM->iem.s.fCpuIdHostCall, true);
# else
    rc = CFGMR3QueryBoolDef(pIem, "CpuIdHostCall", &pVM->iem.s.fCpuIdHostCall, false);
# endif
    AssertLogRelRCReturn(rc, rc);
#endif

    /*
     * Initialize per-CPU data and register statistics.
     */
    uint64_t const uInitialTlbRevision = UINT64_C(0) - (IEMTLB_REVISION_INCR * 200U);
    uint64_t const uInitialTlbPhysRev  = UINT64_C(0) - (IEMTLB_PHYS_REV_INCR * 100U);

    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
    {
        PVMCPU pVCpu = pVM->apCpusR3[idCpu];
        AssertCompile(sizeof(pVCpu->iem.s) <= sizeof(pVCpu->iem.padding)); /* (tstVMStruct can't do it's job w/o instruction stats) */

        pVCpu->iem.s.CodeTlb.uTlbRevision = pVCpu->iem.s.DataTlb.uTlbRevision = uInitialTlbRevision;
        pVCpu->iem.s.CodeTlb.uTlbPhysRev  = pVCpu->iem.s.DataTlb.uTlbPhysRev  = uInitialTlbPhysRev;

        STAMR3RegisterF(pVM, &pVCpu->iem.s.cInstructions,               STAMTYPE_U32,       STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                        "Instructions interpreted",                     "/IEM/CPU%u/cInstructions", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.cLongJumps,                  STAMTYPE_U32,       STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES,
                        "Number of longjmp calls",                      "/IEM/CPU%u/cLongJumps", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.cPotentialExits,             STAMTYPE_U32,       STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                        "Potential exits",                              "/IEM/CPU%u/cPotentialExits", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.cRetAspectNotImplemented,    STAMTYPE_U32_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                        "VERR_IEM_ASPECT_NOT_IMPLEMENTED",              "/IEM/CPU%u/cRetAspectNotImplemented", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.cRetInstrNotImplemented,     STAMTYPE_U32_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                        "VERR_IEM_INSTR_NOT_IMPLEMENTED",               "/IEM/CPU%u/cRetInstrNotImplemented", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.cRetInfStatuses,             STAMTYPE_U32_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                        "Informational statuses returned",              "/IEM/CPU%u/cRetInfStatuses", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.cRetErrStatuses,             STAMTYPE_U32_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                        "Error statuses returned",                      "/IEM/CPU%u/cRetErrStatuses", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.cbWritten,                   STAMTYPE_U32,       STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES,
                        "Approx bytes written",                         "/IEM/CPU%u/cbWritten", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.cPendingCommit,              STAMTYPE_U32,       STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES,
                        "Times RC/R0 had to postpone instruction committing to ring-3", "/IEM/CPU%u/cPendingCommit", idCpu);

#ifdef VBOX_WITH_STATISTICS
        STAMR3RegisterF(pVM, &pVCpu->iem.s.CodeTlb.cTlbHits,            STAMTYPE_U64_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                        "Code TLB hits",                            "/IEM/CPU%u/CodeTlb-Hits", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.DataTlb.cTlbHits,            STAMTYPE_U64_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                        "Data TLB hits",                            "/IEM/CPU%u/DataTlb-Hits", idCpu);
#endif
        STAMR3RegisterF(pVM, &pVCpu->iem.s.CodeTlb.cTlbMisses,          STAMTYPE_U32_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                        "Code TLB misses",                          "/IEM/CPU%u/CodeTlb-Misses", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.CodeTlb.uTlbRevision,        STAMTYPE_X64,       STAMVISIBILITY_ALWAYS, STAMUNIT_NONE,
                        "Code TLB revision",                        "/IEM/CPU%u/CodeTlb-Revision", idCpu);
        STAMR3RegisterF(pVM, (void *)&pVCpu->iem.s.CodeTlb.uTlbPhysRev, STAMTYPE_X64,       STAMVISIBILITY_ALWAYS, STAMUNIT_NONE,
                        "Code TLB physical revision",               "/IEM/CPU%u/CodeTlb-PhysRev", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.CodeTlb.cTlbSlowReadPath,    STAMTYPE_U32_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_NONE,
                        "Code TLB slow read path",                  "/IEM/CPU%u/CodeTlb-SlowReads", idCpu);

        STAMR3RegisterF(pVM, &pVCpu->iem.s.DataTlb.cTlbMisses,          STAMTYPE_U32_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                        "Data TLB misses",                          "/IEM/CPU%u/DataTlb-Misses", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.DataTlb.uTlbRevision,        STAMTYPE_X64,       STAMVISIBILITY_ALWAYS, STAMUNIT_NONE,
                        "Data TLB revision",                        "/IEM/CPU%u/DataTlb-Revision", idCpu);
        STAMR3RegisterF(pVM, (void *)&pVCpu->iem.s.DataTlb.uTlbPhysRev, STAMTYPE_X64,       STAMVISIBILITY_ALWAYS, STAMUNIT_NONE,
                        "Data TLB physical revision",               "/IEM/CPU%u/DataTlb-PhysRev", idCpu);

        for (uint32_t i = 0; i < RT_ELEMENTS(pVCpu->iem.s.aStatXcpts); i++)
            STAMR3RegisterF(pVM, &pVCpu->iem.s.aStatXcpts[i], STAMTYPE_COUNTER, STAMVISIBILITY_USED, STAMUNIT_OCCURENCES,
                            "", "/IEM/CPU%u/Exceptions/%02x", idCpu, i);
        for (uint32_t i = 0; i < RT_ELEMENTS(pVCpu->iem.s.aStatInts); i++)
            STAMR3RegisterF(pVM, &pVCpu->iem.s.aStatInts[i], STAMTYPE_U32_RESET, STAMVISIBILITY_USED, STAMUNIT_OCCURENCES,
                            "", "/IEM/CPU%u/Interrupts/%02x", idCpu, i);

#if defined(VBOX_WITH_STATISTICS) && !defined(DOXYGEN_RUNNING)
        /* Instruction statistics: */
# define IEM_DO_INSTR_STAT(a_Name, a_szDesc) \
            STAMR3RegisterF(pVM, &pVCpu->iem.s.StatsRZ.a_Name, STAMTYPE_U32_RESET, STAMVISIBILITY_USED, \
                            STAMUNIT_COUNT, a_szDesc, "/IEM/CPU%u/instr-RZ/" #a_Name, idCpu); \
            STAMR3RegisterF(pVM, &pVCpu->iem.s.StatsR3.a_Name, STAMTYPE_U32_RESET, STAMVISIBILITY_USED, \
                            STAMUNIT_COUNT, a_szDesc, "/IEM/CPU%u/instr-R3/" #a_Name, idCpu);
# include "IEMInstructionStatisticsTmpl.h"
# undef IEM_DO_INSTR_STAT
#endif

        /*
         * Host and guest CPU information.
         */
        if (idCpu == 0)
        {
            pVCpu->iem.s.enmCpuVendor                     = CPUMGetGuestCpuVendor(pVM);
            pVCpu->iem.s.enmHostCpuVendor                 = CPUMGetHostCpuVendor(pVM);
            pVCpu->iem.s.aidxTargetCpuEflFlavour[0]       =    pVCpu->iem.s.enmCpuVendor == CPUMCPUVENDOR_INTEL
                                                            || pVCpu->iem.s.enmCpuVendor == CPUMCPUVENDOR_VIA /*??*/
                                                          ? IEMTARGETCPU_EFL_BEHAVIOR_INTEL : IEMTARGETCPU_EFL_BEHAVIOR_AMD;
#if defined(RT_ARCH_X86) || defined(RT_ARCH_AMD64)
            if (pVCpu->iem.s.enmCpuVendor == pVCpu->iem.s.enmHostCpuVendor)
                pVCpu->iem.s.aidxTargetCpuEflFlavour[1]   = IEMTARGETCPU_EFL_BEHAVIOR_NATIVE;
            else
#endif
                pVCpu->iem.s.aidxTargetCpuEflFlavour[1]   = pVCpu->iem.s.aidxTargetCpuEflFlavour[0];

#if IEM_CFG_TARGET_CPU == IEMTARGETCPU_DYNAMIC
            switch (pVM->cpum.ro.GuestFeatures.enmMicroarch)
            {
                case kCpumMicroarch_Intel_8086:     pVCpu->iem.s.uTargetCpu = IEMTARGETCPU_8086; break;
                case kCpumMicroarch_Intel_80186:    pVCpu->iem.s.uTargetCpu = IEMTARGETCPU_186; break;
                case kCpumMicroarch_Intel_80286:    pVCpu->iem.s.uTargetCpu = IEMTARGETCPU_286; break;
                case kCpumMicroarch_Intel_80386:    pVCpu->iem.s.uTargetCpu = IEMTARGETCPU_386; break;
                case kCpumMicroarch_Intel_80486:    pVCpu->iem.s.uTargetCpu = IEMTARGETCPU_486; break;
                case kCpumMicroarch_Intel_P5:       pVCpu->iem.s.uTargetCpu = IEMTARGETCPU_PENTIUM; break;
                case kCpumMicroarch_Intel_P6:       pVCpu->iem.s.uTargetCpu = IEMTARGETCPU_PPRO; break;
                case kCpumMicroarch_NEC_V20:        pVCpu->iem.s.uTargetCpu = IEMTARGETCPU_V20; break;
                case kCpumMicroarch_NEC_V30:        pVCpu->iem.s.uTargetCpu = IEMTARGETCPU_V20; break;
                default:                            pVCpu->iem.s.uTargetCpu = IEMTARGETCPU_CURRENT; break;
            }
            LogRel(("IEM: TargetCpu=%s, Microarch=%s aidxTargetCpuEflFlavour={%d,%d}\n",
                    iemGetTargetCpuName(pVCpu->iem.s.uTargetCpu), CPUMMicroarchName(pVM->cpum.ro.GuestFeatures.enmMicroarch),
                    pVCpu->iem.s.aidxTargetCpuEflFlavour[0], pVCpu->iem.s.aidxTargetCpuEflFlavour[1]));
#else
            LogRel(("IEM: Microarch=%s aidxTargetCpuEflFlavour={%d,%d}\n",
                    CPUMMicroarchName(pVM->cpum.ro.GuestFeatures.enmMicroarch),
                    pVCpu->iem.s.aidxTargetCpuEflFlavour[0], pVCpu->iem.s.aidxTargetCpuEflFlavour[1]));
#endif
        }
        else
        {
            pVCpu->iem.s.enmCpuVendor                     = pVM->apCpusR3[0]->iem.s.enmCpuVendor;
            pVCpu->iem.s.enmHostCpuVendor                 = pVM->apCpusR3[0]->iem.s.enmHostCpuVendor;
            pVCpu->iem.s.aidxTargetCpuEflFlavour[0]       = pVM->apCpusR3[0]->iem.s.aidxTargetCpuEflFlavour[0];
            pVCpu->iem.s.aidxTargetCpuEflFlavour[1]       = pVM->apCpusR3[0]->iem.s.aidxTargetCpuEflFlavour[1];
#if IEM_CFG_TARGET_CPU == IEMTARGETCPU_DYNAMIC
            pVCpu->iem.s.uTargetCpu                       = pVM->apCpusR3[0]->iem.s.uTargetCpu;
#endif
        }

        /*
         * Mark all buffers free.
         */
        uint32_t iMemMap = RT_ELEMENTS(pVCpu->iem.s.aMemMappings);
        while (iMemMap-- > 0)
            pVCpu->iem.s.aMemMappings[iMemMap].fAccess = IEM_ACCESS_INVALID;
    }

#ifdef VBOX_WITH_NESTED_HWVIRT_VMX
    /*
     * Register the per-VM VMX APIC-access page handler type.
     */
    if (pVM->cpum.ro.GuestFeatures.fVmx)
    {
        rc = PGMR3HandlerPhysicalTypeRegister(pVM, PGMPHYSHANDLERKIND_ALL, PGMPHYSHANDLER_F_NOT_IN_HM,
                                              iemVmxApicAccessPageHandler,
                                              "VMX APIC-access page", &pVM->iem.s.hVmxApicAccessPage);
        AssertLogRelRCReturn(rc, rc);
    }
#endif

    DBGFR3InfoRegisterInternalArgv(pVM, "itlb", "IEM instruction TLB", iemR3InfoITlb, DBGFINFO_FLAGS_RUN_ON_EMT);
    DBGFR3InfoRegisterInternalArgv(pVM, "dtlb", "IEM instruction TLB", iemR3InfoDTlb, DBGFINFO_FLAGS_RUN_ON_EMT);
#ifdef VBOX_WITH_DEBUGGER
    iemR3RegisterDebuggerCommands();
#endif

    return VINF_SUCCESS;
}


VMMR3DECL(int)      IEMR3Term(PVM pVM)
{
    NOREF(pVM);
    return VINF_SUCCESS;
}


VMMR3DECL(void)     IEMR3Relocate(PVM pVM)
{
    RT_NOREF(pVM);
}


/** Worker for iemR3InfoTlbPrintSlots and iemR3InfoTlbPrintAddress. */
static void iemR3InfoTlbPrintHeader(PVMCPU pVCpu, PCDBGFINFOHLP pHlp, IEMTLB const *pTlb, bool *pfHeader)
{
    if (*pfHeader)
        return;
    pHlp->pfnPrintf(pHlp, "%cTLB for CPU %u:\n", &pVCpu->iem.s.CodeTlb == pTlb ? 'I' : 'D', pVCpu->idCpu);
    *pfHeader = true;
}


/** Worker for iemR3InfoTlbPrintSlots and iemR3InfoTlbPrintAddress. */
static void iemR3InfoTlbPrintSlot(PCDBGFINFOHLP pHlp, IEMTLB const *pTlb, IEMTLBENTRY const *pTlbe, uint32_t uSlot)
{
    pHlp->pfnPrintf(pHlp, "%02x: %s %#018RX64 -> %RGp / %p / %#05x %s%s%s%s/%s%s%s/%s %s\n",
                    uSlot,
                    (pTlbe->uTag & IEMTLB_REVISION_MASK) == pTlb->uTlbRevision ? "valid  "
                    : (pTlbe->uTag & IEMTLB_REVISION_MASK) == 0                ? "empty  "
                                                                               : "expired",
                    (pTlbe->uTag & ~IEMTLB_REVISION_MASK) << X86_PAGE_SHIFT,
                    pTlbe->GCPhys, pTlbe->pbMappingR3,
                    (uint32_t)(pTlbe->fFlagsAndPhysRev & ~IEMTLBE_F_PHYS_REV),
                    pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PT_NO_EXEC      ? "NX" : " X",
                    pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PT_NO_WRITE     ? "RO" : "RW",
                    pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PT_NO_ACCESSED  ? "-"  : "A",
                    pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PT_NO_DIRTY     ? "-"  : "D",
                    pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PG_NO_WRITE     ? "-"  : "w",
                    pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PG_NO_READ      ? "-"  : "r",
                    pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PG_UNASSIGNED   ? "U"  : "-",
                    pTlbe->fFlagsAndPhysRev & IEMTLBE_F_NO_MAPPINGR3    ? "S"  : "M",
                    (pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PHYS_REV) == pTlb->uTlbPhysRev ? "phys-valid"
                    : (pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PHYS_REV) == 0 ? "phys-empty" : "phys-expired");
}


/** Displays one or more TLB slots. */
static void iemR3InfoTlbPrintSlots(PVMCPU pVCpu, PCDBGFINFOHLP pHlp, IEMTLB const *pTlb,
                                   uint32_t uSlot, uint32_t cSlots, bool *pfHeader)
{
    if (uSlot < RT_ELEMENTS(pTlb->aEntries))
    {
        if (cSlots > RT_ELEMENTS(pTlb->aEntries))
        {
            pHlp->pfnPrintf(pHlp, "error: Too many slots given: %u, adjusting it down to the max (%u)\n",
                            cSlots, RT_ELEMENTS(pTlb->aEntries));
            cSlots = RT_ELEMENTS(pTlb->aEntries);
        }

        iemR3InfoTlbPrintHeader(pVCpu, pHlp, pTlb, pfHeader);
        while (cSlots-- > 0)
        {
            IEMTLBENTRY const Tlbe = pTlb->aEntries[uSlot];
            iemR3InfoTlbPrintSlot(pHlp, pTlb, &Tlbe, uSlot);
            uSlot = (uSlot + 1) % RT_ELEMENTS(pTlb->aEntries);
        }
    }
    else
        pHlp->pfnPrintf(pHlp, "error: TLB slot is out of range: %u (%#x), max %u (%#x)\n",
                        uSlot, uSlot, RT_ELEMENTS(pTlb->aEntries) - 1, RT_ELEMENTS(pTlb->aEntries) - 1);
}


/** Displays the TLB slot for the given address. */
static void iemR3InfoTlbPrintAddress(PVMCPU pVCpu, PCDBGFINFOHLP pHlp, IEMTLB const *pTlb,
                                     uint64_t uAddress, bool *pfHeader)
{
    iemR3InfoTlbPrintHeader(pVCpu, pHlp, pTlb, pfHeader);

    uint64_t const    uTag  = (uAddress << 16) >> (X86_PAGE_SHIFT + 16);
    uint32_t const    uSlot = (uint8_t)uTag;
    IEMTLBENTRY const Tlbe  = pTlb->aEntries[uSlot];
    pHlp->pfnPrintf(pHlp, "Address %#RX64 -> slot %#x - %s\n", uAddress, uSlot,
                    Tlbe.uTag == (uTag | pTlb->uTlbRevision)  ? "match"
                    : (Tlbe.uTag & ~IEMTLB_REVISION_MASK) == uTag ? "expired" : "mismatch");
    iemR3InfoTlbPrintSlot(pHlp, pTlb, &Tlbe, uSlot);
}


/** Common worker for iemR3InfoDTlb and iemR3InfoITlb. */
static void iemR3InfoTlbCommon(PVM pVM, PCDBGFINFOHLP pHlp, int cArgs, char **papszArgs, bool fITlb)
{
    /*
     * This is entirely argument driven.
     */
    static RTGETOPTDEF const s_aOptions[] =
    {
        { "--cpu",     'c', RTGETOPT_REQ_UINT32                          },
        { "--vcpu",    'c', RTGETOPT_REQ_UINT32                          },
        { "all",       'A', RTGETOPT_REQ_NOTHING                         },
        { "--all",     'A', RTGETOPT_REQ_NOTHING                         },
        { "--address", 'a', RTGETOPT_REQ_UINT64      | RTGETOPT_FLAG_HEX },
        { "--range",   'r', RTGETOPT_REQ_UINT32_PAIR | RTGETOPT_FLAG_HEX },
        { "--slot",    's', RTGETOPT_REQ_UINT32      | RTGETOPT_FLAG_HEX },
    };

    char  szDefault[] = "-A";
    char *papszDefaults[2] = { szDefault, NULL };
    if (cArgs == 0)
    {
        cArgs     = 1;
        papszArgs = papszDefaults;
    }

    RTGETOPTSTATE State;
    int rc = RTGetOptInit(&State, cArgs, papszArgs, s_aOptions, RT_ELEMENTS(s_aOptions), 0 /*iFirst*/, 0 /*fFlags*/);
    AssertRCReturnVoid(rc);

    bool            fNeedHeader  = true;
    bool            fAddressMode = true;
    PVMCPU          pVCpu        = VMMGetCpu(pVM);
    if (!pVCpu)
        pVCpu = VMMGetCpuById(pVM, 0);

    RTGETOPTUNION   ValueUnion;
    while ((rc = RTGetOpt(&State, &ValueUnion)) != 0)
    {
        switch (rc)
        {
            case 'c':
                if (ValueUnion.u32 >= pVM->cCpus)
                    pHlp->pfnPrintf(pHlp, "error: Invalid CPU ID: %u\n", ValueUnion.u32);
                else if (!pVCpu || pVCpu->idCpu != ValueUnion.u32)
                {
                    pVCpu = VMMGetCpuById(pVM, ValueUnion.u32);
                    fNeedHeader = true;
                }
                break;

            case 'a':
                iemR3InfoTlbPrintAddress(pVCpu, pHlp, fITlb ? &pVCpu->iem.s.CodeTlb : &pVCpu->iem.s.DataTlb,
                                         ValueUnion.u64, &fNeedHeader);
                fAddressMode = true;
                break;

            case 'A':
                iemR3InfoTlbPrintSlots(pVCpu, pHlp, fITlb ? &pVCpu->iem.s.CodeTlb : &pVCpu->iem.s.DataTlb,
                                       0, RT_ELEMENTS(pVCpu->iem.s.CodeTlb.aEntries), &fNeedHeader);
                break;

            case 'r':
                iemR3InfoTlbPrintSlots(pVCpu, pHlp, fITlb ? &pVCpu->iem.s.CodeTlb : &pVCpu->iem.s.DataTlb,
                                       ValueUnion.PairU32.uFirst, ValueUnion.PairU32.uSecond, &fNeedHeader);
                fAddressMode = false;
                break;

            case 's':
                iemR3InfoTlbPrintSlots(pVCpu, pHlp, fITlb ? &pVCpu->iem.s.CodeTlb : &pVCpu->iem.s.DataTlb,
                                       ValueUnion.u32, 1, &fNeedHeader);
                fAddressMode = false;
                break;

            case VINF_GETOPT_NOT_OPTION:
                if (fAddressMode)
                {
                    uint64_t uAddr;
                    rc = RTStrToUInt64Full(ValueUnion.psz, 16, &uAddr);
                    if (RT_SUCCESS(rc) && rc != VWRN_NUMBER_TOO_BIG)
                        iemR3InfoTlbPrintAddress(pVCpu, pHlp, fITlb ? &pVCpu->iem.s.CodeTlb : &pVCpu->iem.s.DataTlb,
                                                 uAddr, &fNeedHeader);
                    else
                        pHlp->pfnPrintf(pHlp, "error: Invalid or malformed guest address '%s': %Rrc\n", ValueUnion.psz, rc);
                }
                else
                {
                    uint32_t uSlot;
                    rc = RTStrToUInt32Full(ValueUnion.psz, 16, &uSlot);
                    if (RT_SUCCESS(rc) && rc != VWRN_NUMBER_TOO_BIG)
                        iemR3InfoTlbPrintSlots(pVCpu, pHlp, fITlb ? &pVCpu->iem.s.CodeTlb : &pVCpu->iem.s.DataTlb,
                                               uSlot, 1, &fNeedHeader);
                    else
                        pHlp->pfnPrintf(pHlp, "error: Invalid or malformed TLB slot number '%s': %Rrc\n", ValueUnion.psz, rc);
                }
                break;

            case 'h':
                pHlp->pfnPrintf(pHlp,
                                "Usage: info %ctlb [options]\n"
                                "\n"
                                "Options:\n"
                                "  -c<n>, --cpu=<n>, --vcpu=<n>\n"
                                "    Selects the CPU which TLBs we're looking at. Default: Caller / 0\n"
                                "  -A, --all, all\n"
                                "    Display all the TLB entries (default if no other args).\n"
                                "  -a<virt>, --address=<virt>\n"
                                "    Shows the TLB entry for the specified guest virtual address.\n"
                                "  -r<slot:count>, --range=<slot:count>\n"
                                "    Shows the TLB entries for the specified slot range.\n"
                                "  -s<slot>,--slot=<slot>\n"
                                "    Shows the given TLB slot.\n"
                                "\n"
                                "Non-options are interpreted according to the last -a, -r or -s option,\n"
                                "defaulting to addresses if not preceeded by any of those options.\n"
                                , fITlb ? 'i' : 'd');
                return;

            default:
                pHlp->pfnGetOptError(pHlp, rc, &ValueUnion, &State);
                return;
        }
    }
}


/**
 * @callback_method_impl{FNDBGFINFOARGVINT, itlb}
 */
static DECLCALLBACK(void) iemR3InfoITlb(PVM pVM, PCDBGFINFOHLP pHlp, int cArgs, char **papszArgs)
{
    return iemR3InfoTlbCommon(pVM, pHlp, cArgs, papszArgs, true /*fITlb*/);
}


/**
 * @callback_method_impl{FNDBGFINFOARGVINT, dtlb}
 */
static DECLCALLBACK(void) iemR3InfoDTlb(PVM pVM, PCDBGFINFOHLP pHlp, int cArgs, char **papszArgs)
{
    return iemR3InfoTlbCommon(pVM, pHlp, cArgs, papszArgs, false /*fITlb*/);
}


#ifdef VBOX_WITH_DEBUGGER

/** @callback_method_impl{FNDBGCCMD,
 * Implements the '.alliem' command. }
 */
static DECLCALLBACK(int) iemR3DbgFlushTlbs(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PUVM pUVM, PCDBGCVAR paArgs, unsigned cArgs)
{
    VMCPUID idCpu = DBGCCmdHlpGetCurrentCpu(pCmdHlp);
    PVMCPU pVCpu = VMMR3GetCpuByIdU(pUVM, idCpu);
    if (pVCpu)
    {
        VMR3ReqPriorityCallVoidWaitU(pUVM, idCpu, (PFNRT)IEMTlbInvalidateAll, 1, pVCpu);
        return VINF_SUCCESS;
    }
    RT_NOREF(paArgs, cArgs);
    return DBGCCmdHlpFail(pCmdHlp, pCmd, "failed to get the PVMCPU for the current CPU");
}


/**
 * Called by IEMR3Init to register debugger commands.
 */
static void iemR3RegisterDebuggerCommands(void)
{
    /*
     * Register debugger commands.
     */
    static DBGCCMD const s_aCmds[] =
    {
        {
            /* .pszCmd = */         "iemflushtlb",
            /* .cArgsMin = */       0,
            /* .cArgsMax = */       0,
            /* .paArgDescs = */     NULL,
            /* .cArgDescs = */      0,
            /* .fFlags = */         0,
            /* .pfnHandler = */     iemR3DbgFlushTlbs,
            /* .pszSyntax = */      "",
            /* .pszDescription = */ "Flushed the code and data TLBs"
        },
    };

    int rc = DBGCRegisterCommands(&s_aCmds[0], RT_ELEMENTS(s_aCmds));
    AssertLogRelRC(rc);
}

#endif

