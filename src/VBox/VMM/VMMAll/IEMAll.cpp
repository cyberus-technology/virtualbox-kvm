/* $Id: IEMAll.cpp $ */
/** @file
 * IEM - Interpreted Execution Manager - All Contexts.
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


/** @page pg_iem    IEM - Interpreted Execution Manager
 *
 * The interpreted exeuction manager (IEM) is for executing short guest code
 * sequences that are causing too many exits / virtualization traps.  It will
 * also be used to interpret single instructions, thus replacing the selective
 * interpreters in EM and IOM.
 *
 * Design goals:
 *      - Relatively small footprint, although we favour speed and correctness
 *        over size.
 *      - Reasonably fast.
 *      - Correctly handle lock prefixed instructions.
 *      - Complete instruction set - eventually.
 *      - Refactorable into a recompiler, maybe.
 *      - Replace EMInterpret*.
 *
 * Using the existing disassembler has been considered, however this is thought
 * to conflict with speed as the disassembler chews things a bit too much while
 * leaving us with a somewhat complicated state to interpret afterwards.
 *
 *
 * The current code is very much work in progress. You've been warned!
 *
 *
 * @section sec_iem_fpu_instr   FPU Instructions
 *
 * On x86 and AMD64 hosts, the FPU instructions are implemented by executing the
 * same or equivalent instructions on the host FPU.  To make life easy, we also
 * let the FPU prioritize the unmasked exceptions for us.  This however, only
 * works reliably when CR0.NE is set, i.e. when using \#MF instead the IRQ 13
 * for FPU exception delivery, because with CR0.NE=0 there is a window where we
 * can trigger spurious FPU exceptions.
 *
 * The guest FPU state is not loaded into the host CPU and kept there till we
 * leave IEM because the calling conventions have declared an all year open
 * season on much of the FPU state.  For instance an innocent looking call to
 * memcpy might end up using a whole bunch of XMM or MM registers if the
 * particular implementation finds it worthwhile.
 *
 *
 * @section sec_iem_logging     Logging
 *
 * The IEM code uses the \"IEM\" log group for the main logging. The different
 * logging levels/flags are generally used for the following purposes:
 *      - Level 1  (Log)  : Errors, exceptions, interrupts and such major events.
 *      - Flow  (LogFlow) : Basic enter/exit IEM state info.
 *      - Level 2  (Log2) : ?
 *      - Level 3  (Log3) : More detailed enter/exit IEM state info.
 *      - Level 4  (Log4) : Decoding mnemonics w/ EIP.
 *      - Level 5  (Log5) : Decoding details.
 *      - Level 6  (Log6) : Enables/disables the lockstep comparison with REM.
 *      - Level 7  (Log7) : iret++ execution logging.
 *      - Level 8  (Log8) : Memory writes.
 *      - Level 9  (Log9) : Memory reads.
 *      - Level 10 (Log10): TLBs.
 *      - Level 11 (Log11): Unmasked FPU exceptions.
 *
 * The SVM (AMD-V) and VMX (VT-x) code has the following assignments:
 *      - Level 1  (Log)  : Errors and other major events.
 *      - Flow (LogFlow)  : Misc flow stuff (cleanup?)
 *      - Level 2  (Log2) : VM exits.
 */

/* Disabled warning C4505: 'iemRaisePageFaultJmp' : unreferenced local function has been removed */
#ifdef _MSC_VER
# pragma warning(disable:4505)
#endif


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP   LOG_GROUP_IEM
#define VMCPU_INCL_CPUM_GST_CTX
#include <VBox/vmm/iem.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/apic.h>
#include <VBox/vmm/pdm.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/iom.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/nem.h>
#include <VBox/vmm/gim.h>
#ifdef VBOX_WITH_NESTED_HWVIRT_SVM
# include <VBox/vmm/em.h>
# include <VBox/vmm/hm_svm.h>
#endif
#ifdef VBOX_WITH_NESTED_HWVIRT_VMX
# include <VBox/vmm/hmvmxinline.h>
#endif
#include <VBox/vmm/tm.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/dbgftrace.h>
#include "IEMInternal.h"
#include <VBox/vmm/vmcc.h>
#include <VBox/log.h>
#include <VBox/err.h>
#include <VBox/param.h>
#include <VBox/dis.h>
#include <VBox/disopcode.h>
#include <iprt/asm-math.h>
#if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
# include <iprt/asm-amd64-x86.h>
#elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
# include <iprt/asm-arm.h>
#endif
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/x86.h>

#include "IEMInline.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * CPU exception classes.
 */
typedef enum IEMXCPTCLASS
{
    IEMXCPTCLASS_BENIGN,
    IEMXCPTCLASS_CONTRIBUTORY,
    IEMXCPTCLASS_PAGE_FAULT,
    IEMXCPTCLASS_DOUBLE_FAULT
} IEMXCPTCLASS;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
#if defined(IEM_LOG_MEMORY_WRITES)
/** What IEM just wrote. */
uint8_t g_abIemWrote[256];
/** How much IEM just wrote. */
size_t g_cbIemWrote;
#endif


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static VBOXSTRICTRC    iemMemFetchSelDescWithErr(PVMCPUCC pVCpu, PIEMSELDESC pDesc, uint16_t uSel,
                                                 uint8_t uXcpt, uint16_t uErrorCode) RT_NOEXCEPT;


/**
 * Slow path of iemInitDecoder() and iemInitExec() that checks what kind of
 * breakpoints are enabled.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 */
void iemInitPendingBreakpointsSlow(PVMCPUCC pVCpu)
{
    /*
     * Process guest breakpoints.
     */
#define PROCESS_ONE_BP(a_fDr7, a_iBp) do { \
        if (a_fDr7 & X86_DR7_L_G(a_iBp)) \
        { \
            switch (X86_DR7_GET_RW(a_fDr7, a_iBp)) \
            { \
                case X86_DR7_RW_EO: \
                    pVCpu->iem.s.fPendingInstructionBreakpoints = true; \
                    break; \
                case X86_DR7_RW_WO: \
                case X86_DR7_RW_RW: \
                    pVCpu->iem.s.fPendingDataBreakpoints = true; \
                    break; \
                case X86_DR7_RW_IO: \
                    pVCpu->iem.s.fPendingIoBreakpoints = true; \
                    break; \
            } \
        } \
    } while (0)
    uint32_t const fGstDr7 = (uint32_t)pVCpu->cpum.GstCtx.dr[7];
    if (fGstDr7 & X86_DR7_ENABLED_MASK)
    {
        PROCESS_ONE_BP(fGstDr7, 0);
        PROCESS_ONE_BP(fGstDr7, 1);
        PROCESS_ONE_BP(fGstDr7, 2);
        PROCESS_ONE_BP(fGstDr7, 3);
    }

    /*
     * Process hypervisor breakpoints.
     */
    uint32_t const fHyperDr7 = DBGFBpGetDR7(pVCpu->CTX_SUFF(pVM));
    if (fHyperDr7 & X86_DR7_ENABLED_MASK)
    {
        PROCESS_ONE_BP(fHyperDr7, 0);
        PROCESS_ONE_BP(fHyperDr7, 1);
        PROCESS_ONE_BP(fHyperDr7, 2);
        PROCESS_ONE_BP(fHyperDr7, 3);
    }
}


/**
 * Initializes the decoder state.
 *
 * iemReInitDecoder is mostly a copy of this function.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 * @param   fBypassHandlers     Whether to bypass access handlers.
 * @param   fDisregardLock      Whether to disregard the LOCK prefix.
 */
DECLINLINE(void) iemInitDecoder(PVMCPUCC pVCpu, bool fBypassHandlers, bool fDisregardLock)
{
    IEM_CTX_ASSERT(pVCpu, IEM_CPUMCTX_EXTRN_MUST_MASK);
    Assert(!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_IEM));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.cs));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.ss));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.es));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.ds));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.fs));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.gs));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.ldtr));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.tr));

    pVCpu->iem.s.uCpl               = CPUMGetGuestCPL(pVCpu);
    IEMMODE enmMode = iemCalcCpuMode(pVCpu);
    pVCpu->iem.s.enmCpuMode         = enmMode;
    pVCpu->iem.s.enmDefAddrMode     = enmMode;  /** @todo check if this is correct... */
    pVCpu->iem.s.enmEffAddrMode     = enmMode;
    if (enmMode != IEMMODE_64BIT)
    {
        pVCpu->iem.s.enmDefOpSize   = enmMode;  /** @todo check if this is correct... */
        pVCpu->iem.s.enmEffOpSize   = enmMode;
    }
    else
    {
        pVCpu->iem.s.enmDefOpSize   = IEMMODE_32BIT;
        pVCpu->iem.s.enmEffOpSize   = IEMMODE_32BIT;
    }
    pVCpu->iem.s.fPrefixes          = 0;
    pVCpu->iem.s.uRexReg            = 0;
    pVCpu->iem.s.uRexB              = 0;
    pVCpu->iem.s.uRexIndex          = 0;
    pVCpu->iem.s.idxPrefix          = 0;
    pVCpu->iem.s.uVex3rdReg         = 0;
    pVCpu->iem.s.uVexLength         = 0;
    pVCpu->iem.s.fEvexStuff         = 0;
    pVCpu->iem.s.iEffSeg            = X86_SREG_DS;
#ifdef IEM_WITH_CODE_TLB
    pVCpu->iem.s.pbInstrBuf         = NULL;
    pVCpu->iem.s.offInstrNextByte   = 0;
    pVCpu->iem.s.offCurInstrStart   = 0;
# ifdef VBOX_STRICT
    pVCpu->iem.s.cbInstrBuf         = UINT16_MAX;
    pVCpu->iem.s.cbInstrBufTotal    = UINT16_MAX;
    pVCpu->iem.s.uInstrBufPc        = UINT64_C(0xc0ffc0ffcff0c0ff);
# endif
#else
    pVCpu->iem.s.offOpcode          = 0;
    pVCpu->iem.s.cbOpcode           = 0;
#endif
    pVCpu->iem.s.offModRm           = 0;
    pVCpu->iem.s.cActiveMappings    = 0;
    pVCpu->iem.s.iNextMapping       = 0;
    pVCpu->iem.s.rcPassUp           = VINF_SUCCESS;
    pVCpu->iem.s.fBypassHandlers    = fBypassHandlers;
    pVCpu->iem.s.fDisregardLock     = fDisregardLock;
    pVCpu->iem.s.fPendingInstructionBreakpoints = false;
    pVCpu->iem.s.fPendingDataBreakpoints        = false;
    pVCpu->iem.s.fPendingIoBreakpoints          = false;
    if (RT_LIKELY(   !(pVCpu->cpum.GstCtx.dr[7] & X86_DR7_ENABLED_MASK)
                  && pVCpu->CTX_SUFF(pVM)->dbgf.ro.cEnabledHwBreakpoints == 0))
    { /* likely */ }
    else
        iemInitPendingBreakpointsSlow(pVCpu);

#ifdef DBGFTRACE_ENABLED
    switch (enmMode)
    {
        case IEMMODE_64BIT:
            RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "I64/%u %08llx", pVCpu->iem.s.uCpl, pVCpu->cpum.GstCtx.rip);
            break;
        case IEMMODE_32BIT:
            RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "I32/%u %04x:%08x", pVCpu->iem.s.uCpl, pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.eip);
            break;
        case IEMMODE_16BIT:
            RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "I16/%u %04x:%04x", pVCpu->iem.s.uCpl, pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.eip);
            break;
    }
#endif
}


/**
 * Reinitializes the decoder state 2nd+ loop of IEMExecLots.
 *
 * This is mostly a copy of iemInitDecoder.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 */
DECLINLINE(void) iemReInitDecoder(PVMCPUCC pVCpu)
{
    Assert(!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_IEM));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.cs));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.ss));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.es));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.ds));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.fs));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.gs));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.ldtr));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.tr));

    pVCpu->iem.s.uCpl               = CPUMGetGuestCPL(pVCpu);   /** @todo this should be updated during execution! */
    IEMMODE enmMode = iemCalcCpuMode(pVCpu);
    pVCpu->iem.s.enmCpuMode         = enmMode;                  /** @todo this should be updated during execution! */
    pVCpu->iem.s.enmDefAddrMode     = enmMode;  /** @todo check if this is correct... */
    pVCpu->iem.s.enmEffAddrMode     = enmMode;
    if (enmMode != IEMMODE_64BIT)
    {
        pVCpu->iem.s.enmDefOpSize   = enmMode;  /** @todo check if this is correct... */
        pVCpu->iem.s.enmEffOpSize   = enmMode;
    }
    else
    {
        pVCpu->iem.s.enmDefOpSize   = IEMMODE_32BIT;
        pVCpu->iem.s.enmEffOpSize   = IEMMODE_32BIT;
    }
    pVCpu->iem.s.fPrefixes          = 0;
    pVCpu->iem.s.uRexReg            = 0;
    pVCpu->iem.s.uRexB              = 0;
    pVCpu->iem.s.uRexIndex          = 0;
    pVCpu->iem.s.idxPrefix          = 0;
    pVCpu->iem.s.uVex3rdReg         = 0;
    pVCpu->iem.s.uVexLength         = 0;
    pVCpu->iem.s.fEvexStuff         = 0;
    pVCpu->iem.s.iEffSeg            = X86_SREG_DS;
#ifdef IEM_WITH_CODE_TLB
    if (pVCpu->iem.s.pbInstrBuf)
    {
        uint64_t off = (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT
                        ? pVCpu->cpum.GstCtx.rip
                        : pVCpu->cpum.GstCtx.eip + (uint32_t)pVCpu->cpum.GstCtx.cs.u64Base)
                     - pVCpu->iem.s.uInstrBufPc;
        if (off < pVCpu->iem.s.cbInstrBufTotal)
        {
            pVCpu->iem.s.offInstrNextByte = (uint32_t)off;
            pVCpu->iem.s.offCurInstrStart = (uint16_t)off;
            if ((uint16_t)off + 15 <= pVCpu->iem.s.cbInstrBufTotal)
                pVCpu->iem.s.cbInstrBuf = (uint16_t)off + 15;
            else
                pVCpu->iem.s.cbInstrBuf = pVCpu->iem.s.cbInstrBufTotal;
        }
        else
        {
            pVCpu->iem.s.pbInstrBuf       = NULL;
            pVCpu->iem.s.offInstrNextByte = 0;
            pVCpu->iem.s.offCurInstrStart = 0;
            pVCpu->iem.s.cbInstrBuf       = 0;
            pVCpu->iem.s.cbInstrBufTotal  = 0;
        }
    }
    else
    {
        pVCpu->iem.s.offInstrNextByte = 0;
        pVCpu->iem.s.offCurInstrStart = 0;
        pVCpu->iem.s.cbInstrBuf       = 0;
        pVCpu->iem.s.cbInstrBufTotal  = 0;
    }
#else
    pVCpu->iem.s.cbOpcode           = 0;
    pVCpu->iem.s.offOpcode          = 0;
#endif
    pVCpu->iem.s.offModRm           = 0;
    Assert(pVCpu->iem.s.cActiveMappings == 0);
    pVCpu->iem.s.iNextMapping       = 0;
    Assert(pVCpu->iem.s.rcPassUp   == VINF_SUCCESS);
    Assert(pVCpu->iem.s.fBypassHandlers == false);

#ifdef DBGFTRACE_ENABLED
    switch (enmMode)
    {
        case IEMMODE_64BIT:
            RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "I64/%u %08llx", pVCpu->iem.s.uCpl, pVCpu->cpum.GstCtx.rip);
            break;
        case IEMMODE_32BIT:
            RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "I32/%u %04x:%08x", pVCpu->iem.s.uCpl, pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.eip);
            break;
        case IEMMODE_16BIT:
            RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "I16/%u %04x:%04x", pVCpu->iem.s.uCpl, pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.eip);
            break;
    }
#endif
}



/**
 * Prefetch opcodes the first time when starting executing.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 * @param   fBypassHandlers     Whether to bypass access handlers.
 * @param   fDisregardLock      Whether to disregard LOCK prefixes.
 *
 * @todo    Combine fDisregardLock and fBypassHandlers into a flag parameter and
 *          store them as such.
 */
static VBOXSTRICTRC iemInitDecoderAndPrefetchOpcodes(PVMCPUCC pVCpu, bool fBypassHandlers, bool fDisregardLock) RT_NOEXCEPT
{
    iemInitDecoder(pVCpu, fBypassHandlers, fDisregardLock);

#ifndef IEM_WITH_CODE_TLB
    /*
     * What we're doing here is very similar to iemMemMap/iemMemBounceBufferMap.
     *
     * First translate CS:rIP to a physical address.
     *
     * Note! The iemOpcodeFetchMoreBytes code depends on this here code to fetch
     *       all relevant bytes from the first page, as it ASSUMES it's only ever
     *       called for dealing with CS.LIM, page crossing and instructions that
     *       are too long.
     */
    uint32_t    cbToTryRead;
    RTGCPTR     GCPtrPC;
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        cbToTryRead = GUEST_PAGE_SIZE;
        GCPtrPC     = pVCpu->cpum.GstCtx.rip;
        if (IEM_IS_CANONICAL(GCPtrPC))
            cbToTryRead = GUEST_PAGE_SIZE - (GCPtrPC & GUEST_PAGE_OFFSET_MASK);
        else
            return iemRaiseGeneralProtectionFault0(pVCpu);
    }
    else
    {
        uint32_t GCPtrPC32 = pVCpu->cpum.GstCtx.eip;
        AssertMsg(!(GCPtrPC32 & ~(uint32_t)UINT16_MAX) || pVCpu->iem.s.enmCpuMode == IEMMODE_32BIT, ("%04x:%RX64\n", pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip));
        if (GCPtrPC32 <= pVCpu->cpum.GstCtx.cs.u32Limit)
            cbToTryRead = pVCpu->cpum.GstCtx.cs.u32Limit - GCPtrPC32 + 1;
        else
            return iemRaiseSelectorBounds(pVCpu, X86_SREG_CS, IEM_ACCESS_INSTRUCTION);
        if (cbToTryRead) { /* likely */ }
        else /* overflowed */
        {
            Assert(GCPtrPC32 == 0); Assert(pVCpu->cpum.GstCtx.cs.u32Limit == UINT32_MAX);
            cbToTryRead = UINT32_MAX;
        }
        GCPtrPC = (uint32_t)pVCpu->cpum.GstCtx.cs.u64Base + GCPtrPC32;
        Assert(GCPtrPC <= UINT32_MAX);
    }

    PGMPTWALK Walk;
    int rc = PGMGstGetPage(pVCpu, GCPtrPC, &Walk);
    if (RT_SUCCESS(rc))
        Assert(Walk.fSucceeded); /* probable. */
    else
    {
        Log(("iemInitDecoderAndPrefetchOpcodes: %RGv - rc=%Rrc\n", GCPtrPC, rc));
# ifdef VBOX_WITH_NESTED_HWVIRT_VMX_EPT
        if (Walk.fFailed & PGM_WALKFAIL_EPT)
            IEM_VMX_VMEXIT_EPT_RET(pVCpu, &Walk, IEM_ACCESS_INSTRUCTION, IEM_SLAT_FAIL_LINEAR_TO_PHYS_ADDR, 0 /* cbInstr */);
# endif
        return iemRaisePageFault(pVCpu, GCPtrPC, 1, IEM_ACCESS_INSTRUCTION, rc);
    }
    if ((Walk.fEffective & X86_PTE_US) || pVCpu->iem.s.uCpl != 3) { /* likely */ }
    else
    {
        Log(("iemInitDecoderAndPrefetchOpcodes: %RGv - supervisor page\n", GCPtrPC));
# ifdef VBOX_WITH_NESTED_HWVIRT_VMX_EPT
        if (Walk.fFailed & PGM_WALKFAIL_EPT)
            IEM_VMX_VMEXIT_EPT_RET(pVCpu, &Walk, IEM_ACCESS_INSTRUCTION, IEM_SLAT_FAIL_LINEAR_TO_PAGE_TABLE, 0 /* cbInstr */);
# endif
        return iemRaisePageFault(pVCpu, GCPtrPC, 1, IEM_ACCESS_INSTRUCTION, VERR_ACCESS_DENIED);
    }
    if (!(Walk.fEffective & X86_PTE_PAE_NX) || !(pVCpu->cpum.GstCtx.msrEFER & MSR_K6_EFER_NXE)) { /* likely */ }
    else
    {
        Log(("iemInitDecoderAndPrefetchOpcodes: %RGv - NX\n", GCPtrPC));
# ifdef VBOX_WITH_NESTED_HWVIRT_VMX_EPT
        if (Walk.fFailed & PGM_WALKFAIL_EPT)
            IEM_VMX_VMEXIT_EPT_RET(pVCpu, &Walk, IEM_ACCESS_INSTRUCTION, IEM_SLAT_FAIL_LINEAR_TO_PAGE_TABLE, 0 /* cbInstr */);
# endif
        return iemRaisePageFault(pVCpu, GCPtrPC, 1, IEM_ACCESS_INSTRUCTION, VERR_ACCESS_DENIED);
    }
    RTGCPHYS const GCPhys = Walk.GCPhys | (GCPtrPC & GUEST_PAGE_OFFSET_MASK);
    /** @todo Check reserved bits and such stuff. PGM is better at doing
     *        that, so do it when implementing the guest virtual address
     *        TLB... */

    /*
     * Read the bytes at this address.
     */
    uint32_t cbLeftOnPage = GUEST_PAGE_SIZE - (GCPtrPC & GUEST_PAGE_OFFSET_MASK);
    if (cbToTryRead > cbLeftOnPage)
        cbToTryRead = cbLeftOnPage;
    if (cbToTryRead > sizeof(pVCpu->iem.s.abOpcode))
        cbToTryRead = sizeof(pVCpu->iem.s.abOpcode);

    if (!pVCpu->iem.s.fBypassHandlers)
    {
        VBOXSTRICTRC rcStrict = PGMPhysRead(pVCpu->CTX_SUFF(pVM), GCPhys, pVCpu->iem.s.abOpcode, cbToTryRead, PGMACCESSORIGIN_IEM);
        if (RT_LIKELY(rcStrict == VINF_SUCCESS))
        { /* likely */ }
        else if (PGM_PHYS_RW_IS_SUCCESS(rcStrict))
        {
            Log(("iemInitDecoderAndPrefetchOpcodes: %RGv/%RGp LB %#x - read status -  rcStrict=%Rrc\n",
                 GCPtrPC, GCPhys, VBOXSTRICTRC_VAL(rcStrict), cbToTryRead));
            rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
        }
        else
        {
            Log((RT_SUCCESS(rcStrict)
                 ? "iemInitDecoderAndPrefetchOpcodes: %RGv/%RGp LB %#x - read status - rcStrict=%Rrc\n"
                 : "iemInitDecoderAndPrefetchOpcodes: %RGv/%RGp LB %#x - read error - rcStrict=%Rrc (!!)\n",
                 GCPtrPC, GCPhys, VBOXSTRICTRC_VAL(rcStrict), cbToTryRead));
            return rcStrict;
        }
    }
    else
    {
        rc = PGMPhysSimpleReadGCPhys(pVCpu->CTX_SUFF(pVM), pVCpu->iem.s.abOpcode, GCPhys, cbToTryRead);
        if (RT_SUCCESS(rc))
        { /* likely */ }
        else
        {
            Log(("iemInitDecoderAndPrefetchOpcodes: %RGv/%RGp LB %#x - read error - rc=%Rrc (!!)\n",
                 GCPtrPC, GCPhys, rc, cbToTryRead));
            return rc;
        }
    }
    pVCpu->iem.s.cbOpcode = cbToTryRead;
#endif /* !IEM_WITH_CODE_TLB */
    return VINF_SUCCESS;
}


/**
 * Invalidates the IEM TLBs.
 *
 * This is called internally as well as by PGM when moving GC mappings.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling
 *                      thread.
 */
VMM_INT_DECL(void) IEMTlbInvalidateAll(PVMCPUCC pVCpu)
{
#if defined(IEM_WITH_CODE_TLB) || defined(IEM_WITH_DATA_TLB)
    Log10(("IEMTlbInvalidateAll\n"));
# ifdef IEM_WITH_CODE_TLB
    pVCpu->iem.s.cbInstrBufTotal = 0;
    pVCpu->iem.s.CodeTlb.uTlbRevision += IEMTLB_REVISION_INCR;
    if (pVCpu->iem.s.CodeTlb.uTlbRevision != 0)
    { /* very likely */ }
    else
    {
        pVCpu->iem.s.CodeTlb.uTlbRevision = IEMTLB_REVISION_INCR;
        unsigned i = RT_ELEMENTS(pVCpu->iem.s.CodeTlb.aEntries);
        while (i-- > 0)
            pVCpu->iem.s.CodeTlb.aEntries[i].uTag = 0;
    }
# endif

# ifdef IEM_WITH_DATA_TLB
    pVCpu->iem.s.DataTlb.uTlbRevision += IEMTLB_REVISION_INCR;
    if (pVCpu->iem.s.DataTlb.uTlbRevision != 0)
    { /* very likely */ }
    else
    {
        pVCpu->iem.s.DataTlb.uTlbRevision = IEMTLB_REVISION_INCR;
        unsigned i = RT_ELEMENTS(pVCpu->iem.s.DataTlb.aEntries);
        while (i-- > 0)
            pVCpu->iem.s.DataTlb.aEntries[i].uTag = 0;
    }
# endif
#else
    RT_NOREF(pVCpu);
#endif
}


/**
 * Invalidates a page in the TLBs.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling
 *                      thread.
 * @param   GCPtr       The address of the page to invalidate
 * @thread EMT(pVCpu)
 */
VMM_INT_DECL(void) IEMTlbInvalidatePage(PVMCPUCC pVCpu, RTGCPTR GCPtr)
{
#if defined(IEM_WITH_CODE_TLB) || defined(IEM_WITH_DATA_TLB)
    Log10(("IEMTlbInvalidatePage: GCPtr=%RGv\n", GCPtr));
    GCPtr = IEMTLB_CALC_TAG_NO_REV(GCPtr);
    Assert(!(GCPtr >> (48 - X86_PAGE_SHIFT)));
    uintptr_t const idx = IEMTLB_TAG_TO_INDEX(GCPtr);

# ifdef IEM_WITH_CODE_TLB
    if (pVCpu->iem.s.CodeTlb.aEntries[idx].uTag == (GCPtr | pVCpu->iem.s.CodeTlb.uTlbRevision))
    {
        pVCpu->iem.s.CodeTlb.aEntries[idx].uTag = 0;
        if (GCPtr == IEMTLB_CALC_TAG_NO_REV(pVCpu->iem.s.uInstrBufPc))
            pVCpu->iem.s.cbInstrBufTotal = 0;
    }
# endif

# ifdef IEM_WITH_DATA_TLB
    if (pVCpu->iem.s.DataTlb.aEntries[idx].uTag == (GCPtr | pVCpu->iem.s.DataTlb.uTlbRevision))
        pVCpu->iem.s.DataTlb.aEntries[idx].uTag = 0;
# endif
#else
    NOREF(pVCpu); NOREF(GCPtr);
#endif
}


#if defined(IEM_WITH_CODE_TLB) || defined(IEM_WITH_DATA_TLB)
/**
 * Invalid both TLBs slow fashion following a rollover.
 *
 * Worker for IEMTlbInvalidateAllPhysical,
 * IEMTlbInvalidateAllPhysicalAllCpus, iemOpcodeFetchBytesJmp, iemMemMap,
 * iemMemMapJmp and others.
 *
 * @thread EMT(pVCpu)
 */
static void IEMTlbInvalidateAllPhysicalSlow(PVMCPUCC pVCpu)
{
    Log10(("IEMTlbInvalidateAllPhysicalSlow\n"));
    ASMAtomicWriteU64(&pVCpu->iem.s.CodeTlb.uTlbPhysRev, IEMTLB_PHYS_REV_INCR * 2);
    ASMAtomicWriteU64(&pVCpu->iem.s.DataTlb.uTlbPhysRev, IEMTLB_PHYS_REV_INCR * 2);

    unsigned i;
# ifdef IEM_WITH_CODE_TLB
    i = RT_ELEMENTS(pVCpu->iem.s.CodeTlb.aEntries);
    while (i-- > 0)
    {
        pVCpu->iem.s.CodeTlb.aEntries[i].pbMappingR3       = NULL;
        pVCpu->iem.s.CodeTlb.aEntries[i].fFlagsAndPhysRev &= ~(  IEMTLBE_F_PG_NO_WRITE   | IEMTLBE_F_PG_NO_READ
                                                               | IEMTLBE_F_PG_UNASSIGNED | IEMTLBE_F_PHYS_REV);
    }
# endif
# ifdef IEM_WITH_DATA_TLB
    i = RT_ELEMENTS(pVCpu->iem.s.DataTlb.aEntries);
    while (i-- > 0)
    {
        pVCpu->iem.s.DataTlb.aEntries[i].pbMappingR3       = NULL;
        pVCpu->iem.s.DataTlb.aEntries[i].fFlagsAndPhysRev &= ~(  IEMTLBE_F_PG_NO_WRITE   | IEMTLBE_F_PG_NO_READ
                                                               | IEMTLBE_F_PG_UNASSIGNED | IEMTLBE_F_PHYS_REV);
    }
# endif

}
#endif


/**
 * Invalidates the host physical aspects of the IEM TLBs.
 *
 * This is called internally as well as by PGM when moving GC mappings.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling
 *                      thread.
 * @note    Currently not used.
 */
VMM_INT_DECL(void) IEMTlbInvalidateAllPhysical(PVMCPUCC pVCpu)
{
#if defined(IEM_WITH_CODE_TLB) || defined(IEM_WITH_DATA_TLB)
    /* Note! This probably won't end up looking exactly like this, but it give an idea... */
    Log10(("IEMTlbInvalidateAllPhysical\n"));

# ifdef IEM_WITH_CODE_TLB
    pVCpu->iem.s.cbInstrBufTotal = 0;
# endif
    uint64_t uTlbPhysRev = pVCpu->iem.s.CodeTlb.uTlbPhysRev + IEMTLB_PHYS_REV_INCR;
    if (RT_LIKELY(uTlbPhysRev > IEMTLB_PHYS_REV_INCR * 2))
    {
        pVCpu->iem.s.CodeTlb.uTlbPhysRev = uTlbPhysRev;
        pVCpu->iem.s.DataTlb.uTlbPhysRev = uTlbPhysRev;
    }
    else
        IEMTlbInvalidateAllPhysicalSlow(pVCpu);
#else
    NOREF(pVCpu);
#endif
}


/**
 * Invalidates the host physical aspects of the IEM TLBs.
 *
 * This is called internally as well as by PGM when moving GC mappings.
 *
 * @param   pVM         The cross context VM structure.
 * @param   idCpuCaller The ID of the calling EMT if available to the caller,
 *                      otherwise NIL_VMCPUID.
 *
 * @remarks Caller holds the PGM lock.
 */
VMM_INT_DECL(void) IEMTlbInvalidateAllPhysicalAllCpus(PVMCC pVM, VMCPUID idCpuCaller)
{
#if defined(IEM_WITH_CODE_TLB) || defined(IEM_WITH_DATA_TLB)
    PVMCPUCC const pVCpuCaller = idCpuCaller >= pVM->cCpus ? VMMGetCpu(pVM) : VMMGetCpuById(pVM, idCpuCaller);
    if (pVCpuCaller)
        VMCPU_ASSERT_EMT(pVCpuCaller);
    Log10(("IEMTlbInvalidateAllPhysicalAllCpus\n"));

    VMCC_FOR_EACH_VMCPU(pVM)
    {
# ifdef IEM_WITH_CODE_TLB
        if (pVCpuCaller == pVCpu)
            pVCpu->iem.s.cbInstrBufTotal = 0;
# endif

        uint64_t const uTlbPhysRevPrev = ASMAtomicUoReadU64(&pVCpu->iem.s.CodeTlb.uTlbPhysRev);
        uint64_t       uTlbPhysRevNew  = uTlbPhysRevPrev + IEMTLB_PHYS_REV_INCR;
        if (RT_LIKELY(uTlbPhysRevNew > IEMTLB_PHYS_REV_INCR * 2))
        { /* likely */}
        else if (pVCpuCaller == pVCpu)
            uTlbPhysRevNew = IEMTLB_PHYS_REV_INCR;
        else
        {
            IEMTlbInvalidateAllPhysicalSlow(pVCpu);
            continue;
        }
        ASMAtomicCmpXchgU64(&pVCpu->iem.s.CodeTlb.uTlbPhysRev, uTlbPhysRevNew, uTlbPhysRevPrev);
        ASMAtomicCmpXchgU64(&pVCpu->iem.s.DataTlb.uTlbPhysRev, uTlbPhysRevNew, uTlbPhysRevPrev);
    }
    VMCC_FOR_EACH_VMCPU_END(pVM);

#else
    RT_NOREF(pVM, idCpuCaller);
#endif
}

#ifdef IEM_WITH_CODE_TLB

/**
 * Tries to fetches @a cbDst opcode bytes, raise the appropriate exception on
 * failure and jumps.
 *
 * We end up here for a number of reasons:
 *      - pbInstrBuf isn't yet initialized.
 *      - Advancing beyond the buffer boundrary (e.g. cross page).
 *      - Advancing beyond the CS segment limit.
 *      - Fetching from non-mappable page (e.g. MMIO).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 * @param   pvDst               Where to return the bytes.
 * @param   cbDst               Number of bytes to read.
 *
 * @todo    Make cbDst = 0 a way of initializing pbInstrBuf?
 */
void iemOpcodeFetchBytesJmp(PVMCPUCC pVCpu, size_t cbDst, void *pvDst) IEM_NOEXCEPT_MAY_LONGJMP
{
# ifdef IN_RING3
    for (;;)
    {
        Assert(cbDst <= 8);
        uint32_t offBuf = pVCpu->iem.s.offInstrNextByte;

        /*
         * We might have a partial buffer match, deal with that first to make the
         * rest simpler.  This is the first part of the cross page/buffer case.
         */
        if (pVCpu->iem.s.pbInstrBuf != NULL)
        {
            if (offBuf < pVCpu->iem.s.cbInstrBuf)
            {
                Assert(offBuf + cbDst > pVCpu->iem.s.cbInstrBuf);
                uint32_t const cbCopy = pVCpu->iem.s.cbInstrBuf - pVCpu->iem.s.offInstrNextByte;
                memcpy(pvDst, &pVCpu->iem.s.pbInstrBuf[offBuf], cbCopy);

                cbDst  -= cbCopy;
                pvDst   = (uint8_t *)pvDst + cbCopy;
                offBuf += cbCopy;
                pVCpu->iem.s.offInstrNextByte += offBuf;
            }
        }

        /*
         * Check segment limit, figuring how much we're allowed to access at this point.
         *
         * We will fault immediately if RIP is past the segment limit / in non-canonical
         * territory.  If we do continue, there are one or more bytes to read before we
         * end up in trouble and we need to do that first before faulting.
         */
        RTGCPTR  GCPtrFirst;
        uint32_t cbMaxRead;
        if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        {
            GCPtrFirst = pVCpu->cpum.GstCtx.rip + (offBuf - (uint32_t)(int32_t)pVCpu->iem.s.offCurInstrStart);
            if (RT_LIKELY(IEM_IS_CANONICAL(GCPtrFirst)))
            { /* likely */ }
            else
                iemRaiseGeneralProtectionFault0Jmp(pVCpu);
            cbMaxRead = X86_PAGE_SIZE - ((uint32_t)GCPtrFirst & X86_PAGE_OFFSET_MASK);
        }
        else
        {
            GCPtrFirst = pVCpu->cpum.GstCtx.eip + (offBuf - (uint32_t)(int32_t)pVCpu->iem.s.offCurInstrStart);
            /* Assert(!(GCPtrFirst & ~(uint32_t)UINT16_MAX) || pVCpu->iem.s.enmCpuMode == IEMMODE_32BIT); - this is allowed */
            if (RT_LIKELY((uint32_t)GCPtrFirst <= pVCpu->cpum.GstCtx.cs.u32Limit))
            { /* likely */ }
            else /** @todo For CPUs older than the 386, we should not necessarily generate \#GP here but wrap around! */
                iemRaiseSelectorBoundsJmp(pVCpu, X86_SREG_CS, IEM_ACCESS_INSTRUCTION);
            cbMaxRead = pVCpu->cpum.GstCtx.cs.u32Limit - (uint32_t)GCPtrFirst + 1;
            if (cbMaxRead != 0)
            { /* likely */ }
            else
            {
                /* Overflowed because address is 0 and limit is max. */
                Assert(GCPtrFirst == 0); Assert(pVCpu->cpum.GstCtx.cs.u32Limit == UINT32_MAX);
                cbMaxRead = X86_PAGE_SIZE;
            }
            GCPtrFirst = (uint32_t)GCPtrFirst + (uint32_t)pVCpu->cpum.GstCtx.cs.u64Base;
            uint32_t cbMaxRead2 = X86_PAGE_SIZE - ((uint32_t)GCPtrFirst & X86_PAGE_OFFSET_MASK);
            if (cbMaxRead2 < cbMaxRead)
                cbMaxRead = cbMaxRead2;
            /** @todo testcase: unreal modes, both huge 16-bit and 32-bit. */
        }

        /*
         * Get the TLB entry for this piece of code.
         */
        uint64_t const     uTag  = IEMTLB_CALC_TAG(    &pVCpu->iem.s.CodeTlb, GCPtrFirst);
        PIEMTLBENTRY const pTlbe = IEMTLB_TAG_TO_ENTRY(&pVCpu->iem.s.CodeTlb, uTag);
        if (pTlbe->uTag == uTag)
        {
            /* likely when executing lots of code, otherwise unlikely */
# ifdef VBOX_WITH_STATISTICS
            pVCpu->iem.s.CodeTlb.cTlbHits++;
# endif
        }
        else
        {
            pVCpu->iem.s.CodeTlb.cTlbMisses++;
            PGMPTWALK Walk;
            int rc = PGMGstGetPage(pVCpu, GCPtrFirst, &Walk);
            if (RT_FAILURE(rc))
            {
#ifdef VBOX_WITH_NESTED_HWVIRT_VMX_EPT
                /** @todo Nested VMX: Need to handle EPT violation/misconfig here?  */
                Assert(!(Walk.fFailed & PGM_WALKFAIL_EPT));
#endif
                Log(("iemOpcodeFetchMoreBytes: %RGv - rc=%Rrc\n", GCPtrFirst, rc));
                iemRaisePageFaultJmp(pVCpu, GCPtrFirst, 1, IEM_ACCESS_INSTRUCTION, rc);
            }

            AssertCompile(IEMTLBE_F_PT_NO_EXEC == 1);
            Assert(Walk.fSucceeded);
            pTlbe->uTag             = uTag;
            pTlbe->fFlagsAndPhysRev = (~Walk.fEffective & (X86_PTE_US | X86_PTE_RW | X86_PTE_D | X86_PTE_A))
                                    | (Walk.fEffective >> X86_PTE_PAE_BIT_NX);
            pTlbe->GCPhys           = Walk.GCPhys;
            pTlbe->pbMappingR3      = NULL;
        }

        /*
         * Check TLB page table level access flags.
         */
        if (pTlbe->fFlagsAndPhysRev & (IEMTLBE_F_PT_NO_USER | IEMTLBE_F_PT_NO_EXEC))
        {
            if ((pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PT_NO_USER) && pVCpu->iem.s.uCpl == 3)
            {
                Log(("iemOpcodeFetchBytesJmp: %RGv - supervisor page\n", GCPtrFirst));
                iemRaisePageFaultJmp(pVCpu, GCPtrFirst, 1, IEM_ACCESS_INSTRUCTION, VERR_ACCESS_DENIED);
            }
            if ((pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PT_NO_EXEC) && (pVCpu->cpum.GstCtx.msrEFER & MSR_K6_EFER_NXE))
            {
                Log(("iemOpcodeFetchMoreBytes: %RGv - NX\n", GCPtrFirst));
                iemRaisePageFaultJmp(pVCpu, GCPtrFirst, 1, IEM_ACCESS_INSTRUCTION, VERR_ACCESS_DENIED);
            }
        }

        /*
         * Look up the physical page info if necessary.
         */
        if ((pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PHYS_REV) == pVCpu->iem.s.CodeTlb.uTlbPhysRev)
        { /* not necessary */ }
        else
        {
            AssertCompile(PGMIEMGCPHYS2PTR_F_NO_WRITE     == IEMTLBE_F_PG_NO_WRITE);
            AssertCompile(PGMIEMGCPHYS2PTR_F_NO_READ      == IEMTLBE_F_PG_NO_READ);
            AssertCompile(PGMIEMGCPHYS2PTR_F_NO_MAPPINGR3 == IEMTLBE_F_NO_MAPPINGR3);
            AssertCompile(PGMIEMGCPHYS2PTR_F_UNASSIGNED   == IEMTLBE_F_PG_UNASSIGNED);
            if (RT_LIKELY(pVCpu->iem.s.CodeTlb.uTlbPhysRev > IEMTLB_PHYS_REV_INCR))
            { /* likely */ }
            else
                IEMTlbInvalidateAllPhysicalSlow(pVCpu);
            pTlbe->fFlagsAndPhysRev &= ~(  IEMTLBE_F_PHYS_REV
                                         | IEMTLBE_F_NO_MAPPINGR3 | IEMTLBE_F_PG_NO_READ | IEMTLBE_F_PG_NO_WRITE | IEMTLBE_F_PG_UNASSIGNED);
            int rc = PGMPhysIemGCPhys2PtrNoLock(pVCpu->CTX_SUFF(pVM), pVCpu, pTlbe->GCPhys, &pVCpu->iem.s.CodeTlb.uTlbPhysRev,
                                                &pTlbe->pbMappingR3, &pTlbe->fFlagsAndPhysRev);
            AssertRCStmt(rc, IEM_DO_LONGJMP(pVCpu, rc));
        }

# if defined(IN_RING3) || defined(IN_RING0) /** @todo fixme */
        /*
         * Try do a direct read using the pbMappingR3 pointer.
         */
        if (    (pTlbe->fFlagsAndPhysRev & (IEMTLBE_F_PHYS_REV | IEMTLBE_F_NO_MAPPINGR3 | IEMTLBE_F_PG_NO_READ))
             == pVCpu->iem.s.CodeTlb.uTlbPhysRev)
        {
            uint32_t const offPg = (GCPtrFirst & X86_PAGE_OFFSET_MASK);
            pVCpu->iem.s.cbInstrBufTotal = offPg + cbMaxRead;
            if (offBuf == (uint32_t)(int32_t)pVCpu->iem.s.offCurInstrStart)
            {
                pVCpu->iem.s.cbInstrBuf       = offPg + RT_MIN(15, cbMaxRead);
                pVCpu->iem.s.offCurInstrStart = (int16_t)offPg;
            }
            else
            {
                uint32_t const cbInstr = offBuf - (uint32_t)(int32_t)pVCpu->iem.s.offCurInstrStart;
                if (cbInstr + (uint32_t)cbDst <= 15)
                {
                    pVCpu->iem.s.cbInstrBuf       = offPg + RT_MIN(cbMaxRead + cbInstr, 15) - cbInstr;
                    pVCpu->iem.s.offCurInstrStart = (int16_t)(offPg - cbInstr);
                }
                else
                {
                    Log(("iemOpcodeFetchMoreBytes: %04x:%08RX64 LB %#x + %#zx -> #GP(0)\n",
                         pVCpu->cpum.GstCtx.cs, pVCpu->cpum.GstCtx.rip, cbInstr, cbDst));
                    iemRaiseGeneralProtectionFault0Jmp(pVCpu);
                }
            }
            if (cbDst <= cbMaxRead)
            {
                pVCpu->iem.s.offInstrNextByte = offPg + (uint32_t)cbDst;
                pVCpu->iem.s.uInstrBufPc      = GCPtrFirst & ~(RTGCPTR)X86_PAGE_OFFSET_MASK;
                pVCpu->iem.s.pbInstrBuf       = pTlbe->pbMappingR3;
                memcpy(pvDst, &pTlbe->pbMappingR3[offPg], cbDst);
                return;
            }
            pVCpu->iem.s.pbInstrBuf = NULL;

            memcpy(pvDst, &pTlbe->pbMappingR3[offPg], cbMaxRead);
            pVCpu->iem.s.offInstrNextByte = offPg + cbMaxRead;
        }
# else
#  error "refactor as needed"
        /*
         * If there is no special read handling, so we can read a bit more and
         * put it in the prefetch buffer.
         */
        if (   cbDst < cbMaxRead
            && (pTlbe->fFlagsAndPhysRev & (IEMTLBE_F_PHYS_REV | IEMTLBE_F_PG_NO_READ)) == pVCpu->iem.s.CodeTlb.uTlbPhysRev)
        {
            VBOXSTRICTRC rcStrict = PGMPhysRead(pVCpu->CTX_SUFF(pVM), pTlbe->GCPhys,
                                                &pVCpu->iem.s.abOpcode[0], cbToTryRead, PGMACCESSORIGIN_IEM);
            if (RT_LIKELY(rcStrict == VINF_SUCCESS))
            { /* likely */ }
            else if (PGM_PHYS_RW_IS_SUCCESS(rcStrict))
            {
                Log(("iemOpcodeFetchMoreBytes: %RGv/%RGp LB %#x - read status -  rcStrict=%Rrc\n",
                     GCPtrNext, GCPhys, VBOXSTRICTRC_VAL(rcStrict), cbToTryRead));
                rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                AssertStmt(rcStrict == VINF_SUCCESS, IEM_DO_LONGJMP(pVCpu, VBOXSTRICRC_VAL(rcStrict)));
            }
            else
            {
                Log((RT_SUCCESS(rcStrict)
                     ? "iemOpcodeFetchMoreBytes: %RGv/%RGp LB %#x - read status - rcStrict=%Rrc\n"
                     : "iemOpcodeFetchMoreBytes: %RGv/%RGp LB %#x - read error - rcStrict=%Rrc (!!)\n",
                     GCPtrNext, GCPhys, VBOXSTRICTRC_VAL(rcStrict), cbToTryRead));
                IEM_DO_LONGJMP(pVCpu, VBOXSTRICTRC_VAL(rcStrict));
            }
        }
# endif
        /*
         * Special read handling, so only read exactly what's needed.
         * This is a highly unlikely scenario.
         */
        else
        {
            pVCpu->iem.s.CodeTlb.cTlbSlowReadPath++;

            /* Check instruction length. */
            uint32_t const cbInstr = offBuf - (uint32_t)(int32_t)pVCpu->iem.s.offCurInstrStart;
            if (RT_LIKELY(cbInstr + cbDst <= 15))
            { /* likely */ }
            else
            {
                Log(("iemOpcodeFetchMoreBytes: %04x:%08RX64 LB %#x + %#zx -> #GP(0) [slow]\n",
                     pVCpu->cpum.GstCtx.cs, pVCpu->cpum.GstCtx.rip, cbInstr, cbDst));
                iemRaiseGeneralProtectionFault0Jmp(pVCpu);
            }

            /* Do the reading. */
            uint32_t const cbToRead = RT_MIN((uint32_t)cbDst, cbMaxRead);
            VBOXSTRICTRC rcStrict = PGMPhysRead(pVCpu->CTX_SUFF(pVM), pTlbe->GCPhys + (GCPtrFirst & X86_PAGE_OFFSET_MASK),
                                                pvDst, cbToRead, PGMACCESSORIGIN_IEM);
            if (RT_LIKELY(rcStrict == VINF_SUCCESS))
            { /* likely */ }
            else if (PGM_PHYS_RW_IS_SUCCESS(rcStrict))
            {
                Log(("iemOpcodeFetchMoreBytes: %RGv/%RGp LB %#x - read status -  rcStrict=%Rrc\n",
                     GCPtrFirst, pTlbe->GCPhys + (GCPtrFirst & X86_PAGE_OFFSET_MASK), VBOXSTRICTRC_VAL(rcStrict), cbToRead));
                rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                AssertStmt(rcStrict == VINF_SUCCESS, IEM_DO_LONGJMP(pVCpu, VBOXSTRICTRC_VAL(rcStrict)));
            }
            else
            {
                Log((RT_SUCCESS(rcStrict)
                     ? "iemOpcodeFetchMoreBytes: %RGv/%RGp LB %#x - read status - rcStrict=%Rrc\n"
                     : "iemOpcodeFetchMoreBytes: %RGv/%RGp LB %#x - read error - rcStrict=%Rrc (!!)\n",
                     GCPtrFirst, pTlbe->GCPhys + (GCPtrFirst & X86_PAGE_OFFSET_MASK), VBOXSTRICTRC_VAL(rcStrict), cbToRead));
                IEM_DO_LONGJMP(pVCpu, VBOXSTRICTRC_VAL(rcStrict));
            }
            pVCpu->iem.s.offInstrNextByte = offBuf + cbToRead;
            if (cbToRead == cbDst)
                return;
        }

        /*
         * More to read, loop.
         */
        cbDst -= cbMaxRead;
        pvDst  = (uint8_t *)pvDst + cbMaxRead;
    }
# else  /* !IN_RING3 */
    RT_NOREF(pvDst, cbDst);
    if (pvDst || cbDst)
        IEM_DO_LONGJMP(pVCpu, VERR_INTERNAL_ERROR);
# endif /* !IN_RING3 */
}

#else

/**
 * Try fetch at least @a cbMin bytes more opcodes, raise the appropriate
 * exception if it fails.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 * @param   cbMin               The minimum number of bytes relative offOpcode
 *                              that must be read.
 */
VBOXSTRICTRC iemOpcodeFetchMoreBytes(PVMCPUCC pVCpu, size_t cbMin) RT_NOEXCEPT
{
    /*
     * What we're doing here is very similar to iemMemMap/iemMemBounceBufferMap.
     *
     * First translate CS:rIP to a physical address.
     */
    uint8_t const   cbOpcode  = pVCpu->iem.s.cbOpcode;
    uint8_t const   offOpcode = pVCpu->iem.s.offOpcode;
    uint8_t const   cbLeft    = cbOpcode - offOpcode;
    Assert(cbLeft < cbMin);
    Assert(cbOpcode <= sizeof(pVCpu->iem.s.abOpcode));

    uint32_t        cbToTryRead;
    RTGCPTR         GCPtrNext;
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        GCPtrNext   = pVCpu->cpum.GstCtx.rip + cbOpcode;
        if (!IEM_IS_CANONICAL(GCPtrNext))
            return iemRaiseGeneralProtectionFault0(pVCpu);
        cbToTryRead = GUEST_PAGE_SIZE - (GCPtrNext & GUEST_PAGE_OFFSET_MASK);
    }
    else
    {
        uint32_t GCPtrNext32 = pVCpu->cpum.GstCtx.eip;
        /* Assert(!(GCPtrNext32 & ~(uint32_t)UINT16_MAX) || pVCpu->iem.s.enmCpuMode == IEMMODE_32BIT); - this is allowed */
        GCPtrNext32 += cbOpcode;
        if (GCPtrNext32 > pVCpu->cpum.GstCtx.cs.u32Limit)
            /** @todo For CPUs older than the 386, we should not generate \#GP here but wrap around! */
            return iemRaiseSelectorBounds(pVCpu, X86_SREG_CS, IEM_ACCESS_INSTRUCTION);
        cbToTryRead = pVCpu->cpum.GstCtx.cs.u32Limit - GCPtrNext32 + 1;
        if (!cbToTryRead) /* overflowed */
        {
            Assert(GCPtrNext32 == 0); Assert(pVCpu->cpum.GstCtx.cs.u32Limit == UINT32_MAX);
            cbToTryRead = UINT32_MAX;
            /** @todo check out wrapping around the code segment.  */
        }
        if (cbToTryRead < cbMin - cbLeft)
            return iemRaiseSelectorBounds(pVCpu, X86_SREG_CS, IEM_ACCESS_INSTRUCTION);
        GCPtrNext = (uint32_t)pVCpu->cpum.GstCtx.cs.u64Base + GCPtrNext32;

        uint32_t cbLeftOnPage = GUEST_PAGE_SIZE - (GCPtrNext & GUEST_PAGE_OFFSET_MASK);
        if (cbToTryRead > cbLeftOnPage)
            cbToTryRead = cbLeftOnPage;
    }

    /* Restrict to opcode buffer space.

       We're making ASSUMPTIONS here based on work done previously in
       iemInitDecoderAndPrefetchOpcodes, where bytes from the first page will
       be fetched in case of an instruction crossing two pages. */
    if (cbToTryRead > sizeof(pVCpu->iem.s.abOpcode) - cbOpcode)
        cbToTryRead = sizeof(pVCpu->iem.s.abOpcode) - cbOpcode;
    if (RT_LIKELY(cbToTryRead + cbLeft >= cbMin))
    { /* likely */ }
    else
    {
        Log(("iemOpcodeFetchMoreBytes: %04x:%08RX64 LB %#x + %#zx -> #GP(0)\n",
             pVCpu->cpum.GstCtx.cs, pVCpu->cpum.GstCtx.rip, offOpcode, cbMin));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    PGMPTWALK Walk;
    int rc = PGMGstGetPage(pVCpu, GCPtrNext, &Walk);
    if (RT_FAILURE(rc))
    {
        Log(("iemOpcodeFetchMoreBytes: %RGv - rc=%Rrc\n", GCPtrNext, rc));
#ifdef VBOX_WITH_NESTED_HWVIRT_VMX_EPT
        if (Walk.fFailed & PGM_WALKFAIL_EPT)
            IEM_VMX_VMEXIT_EPT_RET(pVCpu, &Walk, IEM_ACCESS_INSTRUCTION, IEM_SLAT_FAIL_LINEAR_TO_PHYS_ADDR, 0 /* cbInstr */);
#endif
        return iemRaisePageFault(pVCpu, GCPtrNext, 1, IEM_ACCESS_INSTRUCTION, rc);
    }
    if (!(Walk.fEffective & X86_PTE_US) && pVCpu->iem.s.uCpl == 3)
    {
        Log(("iemOpcodeFetchMoreBytes: %RGv - supervisor page\n", GCPtrNext));
#ifdef VBOX_WITH_NESTED_HWVIRT_VMX_EPT
        if (Walk.fFailed & PGM_WALKFAIL_EPT)
            IEM_VMX_VMEXIT_EPT_RET(pVCpu, &Walk, IEM_ACCESS_INSTRUCTION, IEM_SLAT_FAIL_LINEAR_TO_PAGE_TABLE, 0 /* cbInstr */);
#endif
        return iemRaisePageFault(pVCpu, GCPtrNext, 1, IEM_ACCESS_INSTRUCTION, VERR_ACCESS_DENIED);
    }
    if ((Walk.fEffective & X86_PTE_PAE_NX) && (pVCpu->cpum.GstCtx.msrEFER & MSR_K6_EFER_NXE))
    {
        Log(("iemOpcodeFetchMoreBytes: %RGv - NX\n", GCPtrNext));
#ifdef VBOX_WITH_NESTED_HWVIRT_VMX_EPT
        if (Walk.fFailed & PGM_WALKFAIL_EPT)
            IEM_VMX_VMEXIT_EPT_RET(pVCpu, &Walk, IEM_ACCESS_INSTRUCTION, IEM_SLAT_FAIL_LINEAR_TO_PAGE_TABLE, 0 /* cbInstr */);
#endif
        return iemRaisePageFault(pVCpu, GCPtrNext, 1, IEM_ACCESS_INSTRUCTION, VERR_ACCESS_DENIED);
    }
    RTGCPHYS const GCPhys = Walk.GCPhys | (GCPtrNext & GUEST_PAGE_OFFSET_MASK);
    Log5(("GCPtrNext=%RGv GCPhys=%RGp cbOpcodes=%#x\n",  GCPtrNext,  GCPhys,  cbOpcode));
    /** @todo Check reserved bits and such stuff. PGM is better at doing
     *        that, so do it when implementing the guest virtual address
     *        TLB... */

    /*
     * Read the bytes at this address.
     *
     * We read all unpatched bytes in iemInitDecoderAndPrefetchOpcodes already,
     * and since PATM should only patch the start of an instruction there
     * should be no need to check again here.
     */
    if (!pVCpu->iem.s.fBypassHandlers)
    {
        VBOXSTRICTRC rcStrict = PGMPhysRead(pVCpu->CTX_SUFF(pVM), GCPhys, &pVCpu->iem.s.abOpcode[cbOpcode],
                                            cbToTryRead, PGMACCESSORIGIN_IEM);
        if (RT_LIKELY(rcStrict == VINF_SUCCESS))
        { /* likely */ }
        else if (PGM_PHYS_RW_IS_SUCCESS(rcStrict))
        {
            Log(("iemOpcodeFetchMoreBytes: %RGv/%RGp LB %#x - read status -  rcStrict=%Rrc\n",
                 GCPtrNext, GCPhys, VBOXSTRICTRC_VAL(rcStrict), cbToTryRead));
            rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
        }
        else
        {
            Log((RT_SUCCESS(rcStrict)
                 ? "iemOpcodeFetchMoreBytes: %RGv/%RGp LB %#x - read status - rcStrict=%Rrc\n"
                 : "iemOpcodeFetchMoreBytes: %RGv/%RGp LB %#x - read error - rcStrict=%Rrc (!!)\n",
                 GCPtrNext, GCPhys, VBOXSTRICTRC_VAL(rcStrict), cbToTryRead));
            return rcStrict;
        }
    }
    else
    {
        rc = PGMPhysSimpleReadGCPhys(pVCpu->CTX_SUFF(pVM), &pVCpu->iem.s.abOpcode[cbOpcode], GCPhys, cbToTryRead);
        if (RT_SUCCESS(rc))
        { /* likely */ }
        else
        {
            Log(("iemOpcodeFetchMoreBytes: %RGv - read error - rc=%Rrc (!!)\n", GCPtrNext, rc));
            return rc;
        }
    }
    pVCpu->iem.s.cbOpcode = cbOpcode + cbToTryRead;
    Log5(("%.*Rhxs\n", pVCpu->iem.s.cbOpcode, pVCpu->iem.s.abOpcode));

    return VINF_SUCCESS;
}

#endif /* !IEM_WITH_CODE_TLB */
#ifndef IEM_WITH_SETJMP

/**
 * Deals with the problematic cases that iemOpcodeGetNextU8 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 * @param   pb                  Where to return the opcode byte.
 */
VBOXSTRICTRC iemOpcodeGetNextU8Slow(PVMCPUCC pVCpu, uint8_t *pb) RT_NOEXCEPT
{
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 1);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
        *pb = pVCpu->iem.s.abOpcode[offOpcode];
        pVCpu->iem.s.offOpcode = offOpcode + 1;
    }
    else
        *pb = 0;
    return rcStrict;
}

#else  /* IEM_WITH_SETJMP */

/**
 * Deals with the problematic cases that iemOpcodeGetNextU8Jmp doesn't like, longjmp on error.
 *
 * @returns The opcode byte.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
uint8_t iemOpcodeGetNextU8SlowJmp(PVMCPUCC pVCpu) IEM_NOEXCEPT_MAY_LONGJMP
{
# ifdef IEM_WITH_CODE_TLB
    uint8_t u8;
    iemOpcodeFetchBytesJmp(pVCpu, sizeof(u8), &u8);
    return u8;
# else
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 1);
    if (rcStrict == VINF_SUCCESS)
        return pVCpu->iem.s.abOpcode[pVCpu->iem.s.offOpcode++];
    IEM_DO_LONGJMP(pVCpu, VBOXSTRICTRC_VAL(rcStrict));
# endif
}

#endif /* IEM_WITH_SETJMP */

#ifndef IEM_WITH_SETJMP

/**
 * Deals with the problematic cases that iemOpcodeGetNextS8SxU16 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu16                Where to return the opcode dword.
 */
VBOXSTRICTRC iemOpcodeGetNextS8SxU16Slow(PVMCPUCC pVCpu, uint16_t *pu16) RT_NOEXCEPT
{
    uint8_t      u8;
    VBOXSTRICTRC rcStrict = iemOpcodeGetNextU8Slow(pVCpu, &u8);
    if (rcStrict == VINF_SUCCESS)
        *pu16 = (int8_t)u8;
    return rcStrict;
}


/**
 * Deals with the problematic cases that iemOpcodeGetNextS8SxU32 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu32                Where to return the opcode dword.
 */
VBOXSTRICTRC iemOpcodeGetNextS8SxU32Slow(PVMCPUCC pVCpu, uint32_t *pu32) RT_NOEXCEPT
{
    uint8_t      u8;
    VBOXSTRICTRC rcStrict = iemOpcodeGetNextU8Slow(pVCpu, &u8);
    if (rcStrict == VINF_SUCCESS)
        *pu32 = (int8_t)u8;
    return rcStrict;
}


/**
 * Deals with the problematic cases that iemOpcodeGetNextS8SxU64 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64                Where to return the opcode qword.
 */
VBOXSTRICTRC iemOpcodeGetNextS8SxU64Slow(PVMCPUCC pVCpu, uint64_t *pu64) RT_NOEXCEPT
{
    uint8_t      u8;
    VBOXSTRICTRC rcStrict = iemOpcodeGetNextU8Slow(pVCpu, &u8);
    if (rcStrict == VINF_SUCCESS)
        *pu64 = (int8_t)u8;
    return rcStrict;
}

#endif /* !IEM_WITH_SETJMP */


#ifndef IEM_WITH_SETJMP

/**
 * Deals with the problematic cases that iemOpcodeGetNextU16 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu16                Where to return the opcode word.
 */
VBOXSTRICTRC iemOpcodeGetNextU16Slow(PVMCPUCC pVCpu, uint16_t *pu16) RT_NOEXCEPT
{
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 2);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
# ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        *pu16 = *(uint16_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
# else
        *pu16 = RT_MAKE_U16(pVCpu->iem.s.abOpcode[offOpcode], pVCpu->iem.s.abOpcode[offOpcode + 1]);
# endif
        pVCpu->iem.s.offOpcode = offOpcode + 2;
    }
    else
        *pu16 = 0;
    return rcStrict;
}

#else  /* IEM_WITH_SETJMP */

/**
 * Deals with the problematic cases that iemOpcodeGetNextU16Jmp doesn't like, longjmp on error
 *
 * @returns The opcode word.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
uint16_t iemOpcodeGetNextU16SlowJmp(PVMCPUCC pVCpu) IEM_NOEXCEPT_MAY_LONGJMP
{
# ifdef IEM_WITH_CODE_TLB
    uint16_t u16;
    iemOpcodeFetchBytesJmp(pVCpu, sizeof(u16), &u16);
    return u16;
# else
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 2);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
        pVCpu->iem.s.offOpcode += 2;
#  ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        return *(uint16_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
#  else
        return RT_MAKE_U16(pVCpu->iem.s.abOpcode[offOpcode], pVCpu->iem.s.abOpcode[offOpcode + 1]);
#  endif
    }
    IEM_DO_LONGJMP(pVCpu, VBOXSTRICTRC_VAL(rcStrict));
# endif
}

#endif /* IEM_WITH_SETJMP */

#ifndef IEM_WITH_SETJMP

/**
 * Deals with the problematic cases that iemOpcodeGetNextU16ZxU32 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu32                Where to return the opcode double word.
 */
VBOXSTRICTRC iemOpcodeGetNextU16ZxU32Slow(PVMCPUCC pVCpu, uint32_t *pu32) RT_NOEXCEPT
{
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 2);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
        *pu32 = RT_MAKE_U16(pVCpu->iem.s.abOpcode[offOpcode], pVCpu->iem.s.abOpcode[offOpcode + 1]);
        pVCpu->iem.s.offOpcode = offOpcode + 2;
    }
    else
        *pu32 = 0;
    return rcStrict;
}


/**
 * Deals with the problematic cases that iemOpcodeGetNextU16ZxU64 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64                Where to return the opcode quad word.
 */
VBOXSTRICTRC iemOpcodeGetNextU16ZxU64Slow(PVMCPUCC pVCpu, uint64_t *pu64) RT_NOEXCEPT
{
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 2);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
        *pu64 = RT_MAKE_U16(pVCpu->iem.s.abOpcode[offOpcode], pVCpu->iem.s.abOpcode[offOpcode + 1]);
        pVCpu->iem.s.offOpcode = offOpcode + 2;
    }
    else
        *pu64 = 0;
    return rcStrict;
}

#endif /* !IEM_WITH_SETJMP */

#ifndef IEM_WITH_SETJMP

/**
 * Deals with the problematic cases that iemOpcodeGetNextU32 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu32                Where to return the opcode dword.
 */
VBOXSTRICTRC iemOpcodeGetNextU32Slow(PVMCPUCC pVCpu, uint32_t *pu32) RT_NOEXCEPT
{
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 4);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
# ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        *pu32 = *(uint32_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
# else
        *pu32 = RT_MAKE_U32_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                    pVCpu->iem.s.abOpcode[offOpcode + 1],
                                    pVCpu->iem.s.abOpcode[offOpcode + 2],
                                    pVCpu->iem.s.abOpcode[offOpcode + 3]);
# endif
        pVCpu->iem.s.offOpcode = offOpcode + 4;
    }
    else
        *pu32 = 0;
    return rcStrict;
}

#else  /* IEM_WITH_SETJMP */

/**
 * Deals with the problematic cases that iemOpcodeGetNextU32Jmp doesn't like, longjmp on error.
 *
 * @returns The opcode dword.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
uint32_t iemOpcodeGetNextU32SlowJmp(PVMCPUCC pVCpu) IEM_NOEXCEPT_MAY_LONGJMP
{
# ifdef IEM_WITH_CODE_TLB
    uint32_t u32;
    iemOpcodeFetchBytesJmp(pVCpu, sizeof(u32), &u32);
    return u32;
# else
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 4);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
        pVCpu->iem.s.offOpcode = offOpcode + 4;
#  ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        return *(uint32_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
#  else
        return RT_MAKE_U32_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                   pVCpu->iem.s.abOpcode[offOpcode + 1],
                                   pVCpu->iem.s.abOpcode[offOpcode + 2],
                                   pVCpu->iem.s.abOpcode[offOpcode + 3]);
#  endif
    }
    IEM_DO_LONGJMP(pVCpu, VBOXSTRICTRC_VAL(rcStrict));
# endif
}

#endif /* IEM_WITH_SETJMP */

#ifndef IEM_WITH_SETJMP

/**
 * Deals with the problematic cases that iemOpcodeGetNextU32ZxU64 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64                Where to return the opcode dword.
 */
VBOXSTRICTRC iemOpcodeGetNextU32ZxU64Slow(PVMCPUCC pVCpu, uint64_t *pu64) RT_NOEXCEPT
{
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 4);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
        *pu64 = RT_MAKE_U32_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                    pVCpu->iem.s.abOpcode[offOpcode + 1],
                                    pVCpu->iem.s.abOpcode[offOpcode + 2],
                                    pVCpu->iem.s.abOpcode[offOpcode + 3]);
        pVCpu->iem.s.offOpcode = offOpcode + 4;
    }
    else
        *pu64 = 0;
    return rcStrict;
}


/**
 * Deals with the problematic cases that iemOpcodeGetNextS32SxU64 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64                Where to return the opcode qword.
 */
VBOXSTRICTRC iemOpcodeGetNextS32SxU64Slow(PVMCPUCC pVCpu, uint64_t *pu64) RT_NOEXCEPT
{
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 4);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
        *pu64 = (int32_t)RT_MAKE_U32_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                             pVCpu->iem.s.abOpcode[offOpcode + 1],
                                             pVCpu->iem.s.abOpcode[offOpcode + 2],
                                             pVCpu->iem.s.abOpcode[offOpcode + 3]);
        pVCpu->iem.s.offOpcode = offOpcode + 4;
    }
    else
        *pu64 = 0;
    return rcStrict;
}

#endif /* !IEM_WITH_SETJMP */

#ifndef IEM_WITH_SETJMP

/**
 * Deals with the problematic cases that iemOpcodeGetNextU64 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64                Where to return the opcode qword.
 */
VBOXSTRICTRC iemOpcodeGetNextU64Slow(PVMCPUCC pVCpu, uint64_t *pu64) RT_NOEXCEPT
{
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 8);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
# ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        *pu64 = *(uint64_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
# else
        *pu64 = RT_MAKE_U64_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                    pVCpu->iem.s.abOpcode[offOpcode + 1],
                                    pVCpu->iem.s.abOpcode[offOpcode + 2],
                                    pVCpu->iem.s.abOpcode[offOpcode + 3],
                                    pVCpu->iem.s.abOpcode[offOpcode + 4],
                                    pVCpu->iem.s.abOpcode[offOpcode + 5],
                                    pVCpu->iem.s.abOpcode[offOpcode + 6],
                                    pVCpu->iem.s.abOpcode[offOpcode + 7]);
# endif
        pVCpu->iem.s.offOpcode = offOpcode + 8;
    }
    else
        *pu64 = 0;
    return rcStrict;
}

#else  /* IEM_WITH_SETJMP */

/**
 * Deals with the problematic cases that iemOpcodeGetNextU64Jmp doesn't like, longjmp on error.
 *
 * @returns The opcode qword.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
uint64_t iemOpcodeGetNextU64SlowJmp(PVMCPUCC pVCpu) IEM_NOEXCEPT_MAY_LONGJMP
{
# ifdef IEM_WITH_CODE_TLB
    uint64_t u64;
    iemOpcodeFetchBytesJmp(pVCpu, sizeof(u64), &u64);
    return u64;
# else
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 8);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
        pVCpu->iem.s.offOpcode = offOpcode + 8;
#  ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        return *(uint64_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
#  else
        return RT_MAKE_U64_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                   pVCpu->iem.s.abOpcode[offOpcode + 1],
                                   pVCpu->iem.s.abOpcode[offOpcode + 2],
                                   pVCpu->iem.s.abOpcode[offOpcode + 3],
                                   pVCpu->iem.s.abOpcode[offOpcode + 4],
                                   pVCpu->iem.s.abOpcode[offOpcode + 5],
                                   pVCpu->iem.s.abOpcode[offOpcode + 6],
                                   pVCpu->iem.s.abOpcode[offOpcode + 7]);
#  endif
    }
    IEM_DO_LONGJMP(pVCpu, VBOXSTRICTRC_VAL(rcStrict));
# endif
}

#endif /* IEM_WITH_SETJMP */



/** @name  Misc Worker Functions.
 * @{
 */

/**
 * Gets the exception class for the specified exception vector.
 *
 * @returns The class of the specified exception.
 * @param   uVector       The exception vector.
 */
static IEMXCPTCLASS iemGetXcptClass(uint8_t uVector) RT_NOEXCEPT
{
    Assert(uVector <= X86_XCPT_LAST);
    switch (uVector)
    {
        case X86_XCPT_DE:
        case X86_XCPT_TS:
        case X86_XCPT_NP:
        case X86_XCPT_SS:
        case X86_XCPT_GP:
        case X86_XCPT_SX:   /* AMD only */
            return IEMXCPTCLASS_CONTRIBUTORY;

        case X86_XCPT_PF:
        case X86_XCPT_VE:   /* Intel only */
            return IEMXCPTCLASS_PAGE_FAULT;

        case X86_XCPT_DF:
            return IEMXCPTCLASS_DOUBLE_FAULT;
    }
    return IEMXCPTCLASS_BENIGN;
}


/**
 * Evaluates how to handle an exception caused during delivery of another event
 * (exception / interrupt).
 *
 * @returns How to handle the recursive exception.
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 * @param   fPrevFlags          The flags of the previous event.
 * @param   uPrevVector         The vector of the previous event.
 * @param   fCurFlags           The flags of the current exception.
 * @param   uCurVector          The vector of the current exception.
 * @param   pfXcptRaiseInfo     Where to store additional information about the
 *                              exception condition. Optional.
 */
VMM_INT_DECL(IEMXCPTRAISE) IEMEvaluateRecursiveXcpt(PVMCPUCC pVCpu, uint32_t fPrevFlags, uint8_t uPrevVector, uint32_t fCurFlags,
                                                    uint8_t uCurVector, PIEMXCPTRAISEINFO pfXcptRaiseInfo)
{
    /*
     * Only CPU exceptions can be raised while delivering other events, software interrupt
     * (INTn/INT3/INTO/ICEBP) generated exceptions cannot occur as the current (second) exception.
     */
    AssertReturn(fCurFlags & IEM_XCPT_FLAGS_T_CPU_XCPT, IEMXCPTRAISE_INVALID);
    Assert(pVCpu); RT_NOREF(pVCpu);
    Log2(("IEMEvaluateRecursiveXcpt: uPrevVector=%#x uCurVector=%#x\n", uPrevVector, uCurVector));

    IEMXCPTRAISE     enmRaise   = IEMXCPTRAISE_CURRENT_XCPT;
    IEMXCPTRAISEINFO fRaiseInfo = IEMXCPTRAISEINFO_NONE;
    if (fPrevFlags & IEM_XCPT_FLAGS_T_CPU_XCPT)
    {
        IEMXCPTCLASS enmPrevXcptClass = iemGetXcptClass(uPrevVector);
        if (enmPrevXcptClass != IEMXCPTCLASS_BENIGN)
        {
            IEMXCPTCLASS enmCurXcptClass = iemGetXcptClass(uCurVector);
            if (   enmPrevXcptClass == IEMXCPTCLASS_PAGE_FAULT
                && (   enmCurXcptClass == IEMXCPTCLASS_PAGE_FAULT
                    || enmCurXcptClass == IEMXCPTCLASS_CONTRIBUTORY))
            {
                enmRaise = IEMXCPTRAISE_DOUBLE_FAULT;
                fRaiseInfo = enmCurXcptClass == IEMXCPTCLASS_PAGE_FAULT ? IEMXCPTRAISEINFO_PF_PF
                                                                        : IEMXCPTRAISEINFO_PF_CONTRIBUTORY_XCPT;
                Log2(("IEMEvaluateRecursiveXcpt: Vectoring page fault. uPrevVector=%#x uCurVector=%#x uCr2=%#RX64\n", uPrevVector,
                      uCurVector, pVCpu->cpum.GstCtx.cr2));
            }
            else if (   enmPrevXcptClass == IEMXCPTCLASS_CONTRIBUTORY
                     && enmCurXcptClass  == IEMXCPTCLASS_CONTRIBUTORY)
            {
                enmRaise = IEMXCPTRAISE_DOUBLE_FAULT;
                Log2(("IEMEvaluateRecursiveXcpt: uPrevVector=%#x uCurVector=%#x -> #DF\n", uPrevVector, uCurVector));
            }
            else if (   enmPrevXcptClass == IEMXCPTCLASS_DOUBLE_FAULT
                     && (   enmCurXcptClass == IEMXCPTCLASS_CONTRIBUTORY
                         || enmCurXcptClass == IEMXCPTCLASS_PAGE_FAULT))
            {
                enmRaise = IEMXCPTRAISE_TRIPLE_FAULT;
                Log2(("IEMEvaluateRecursiveXcpt: #DF handler raised a %#x exception -> triple fault\n", uCurVector));
            }
        }
        else
        {
            if (uPrevVector == X86_XCPT_NMI)
            {
                fRaiseInfo = IEMXCPTRAISEINFO_NMI_XCPT;
                if (uCurVector == X86_XCPT_PF)
                {
                    fRaiseInfo |= IEMXCPTRAISEINFO_NMI_PF;
                    Log2(("IEMEvaluateRecursiveXcpt: NMI delivery caused a page fault\n"));
                }
            }
            else if (   uPrevVector == X86_XCPT_AC
                     && uCurVector  == X86_XCPT_AC)
            {
                enmRaise   = IEMXCPTRAISE_CPU_HANG;
                fRaiseInfo = IEMXCPTRAISEINFO_AC_AC;
                Log2(("IEMEvaluateRecursiveXcpt: Recursive #AC - Bad guest\n"));
            }
        }
    }
    else if (fPrevFlags & IEM_XCPT_FLAGS_T_EXT_INT)
    {
        fRaiseInfo = IEMXCPTRAISEINFO_EXT_INT_XCPT;
        if (uCurVector == X86_XCPT_PF)
            fRaiseInfo |= IEMXCPTRAISEINFO_EXT_INT_PF;
    }
    else
    {
        Assert(fPrevFlags & IEM_XCPT_FLAGS_T_SOFT_INT);
        fRaiseInfo = IEMXCPTRAISEINFO_SOFT_INT_XCPT;
    }

    if (pfXcptRaiseInfo)
        *pfXcptRaiseInfo = fRaiseInfo;
    return enmRaise;
}


/**
 * Enters the CPU shutdown state initiated by a triple fault or other
 * unrecoverable conditions.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu           The cross context virtual CPU structure of the
 *                          calling thread.
 */
static VBOXSTRICTRC iemInitiateCpuShutdown(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    if (IEM_VMX_IS_NON_ROOT_MODE(pVCpu))
        IEM_VMX_VMEXIT_TRIPLE_FAULT_RET(pVCpu, VMX_EXIT_TRIPLE_FAULT, 0 /* u64ExitQual */);

    if (IEM_SVM_IS_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_SHUTDOWN))
    {
        Log2(("shutdown: Guest intercept -> #VMEXIT\n"));
        IEM_SVM_VMEXIT_RET(pVCpu, SVM_EXIT_SHUTDOWN, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    RT_NOREF(pVCpu);
    return VINF_EM_TRIPLE_FAULT;
}


/**
 * Validates a new SS segment.
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context virtual CPU structure of the
 *                          calling thread.
 * @param   NewSS           The new SS selctor.
 * @param   uCpl            The CPL to load the stack for.
 * @param   pDesc           Where to return the descriptor.
 */
static VBOXSTRICTRC iemMiscValidateNewSS(PVMCPUCC pVCpu, RTSEL NewSS, uint8_t uCpl, PIEMSELDESC pDesc) RT_NOEXCEPT
{
    /* Null selectors are not allowed (we're not called for dispatching
       interrupts with SS=0 in long mode). */
    if (!(NewSS & X86_SEL_MASK_OFF_RPL))
    {
        Log(("iemMiscValidateNewSSandRsp: %#x - null selector -> #TS(0)\n", NewSS));
        return iemRaiseTaskSwitchFault0(pVCpu);
    }

    /** @todo testcase: check that the TSS.ssX RPL is checked.  Also check when. */
    if ((NewSS & X86_SEL_RPL) != uCpl)
    {
        Log(("iemMiscValidateNewSSandRsp: %#x - RPL and CPL (%d) differs -> #TS\n", NewSS, uCpl));
        return iemRaiseTaskSwitchFaultBySelector(pVCpu, NewSS);
    }

    /*
     * Read the descriptor.
     */
    VBOXSTRICTRC rcStrict = iemMemFetchSelDesc(pVCpu, pDesc, NewSS, X86_XCPT_TS);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /*
     * Perform the descriptor validation documented for LSS, POP SS and MOV SS.
     */
    if (!pDesc->Legacy.Gen.u1DescType)
    {
        Log(("iemMiscValidateNewSSandRsp: %#x - system selector (%#x) -> #TS\n", NewSS, pDesc->Legacy.Gen.u4Type));
        return iemRaiseTaskSwitchFaultBySelector(pVCpu, NewSS);
    }

    if (    (pDesc->Legacy.Gen.u4Type & X86_SEL_TYPE_CODE)
        || !(pDesc->Legacy.Gen.u4Type & X86_SEL_TYPE_WRITE) )
    {
        Log(("iemMiscValidateNewSSandRsp: %#x - code or read only (%#x) -> #TS\n", NewSS, pDesc->Legacy.Gen.u4Type));
        return iemRaiseTaskSwitchFaultBySelector(pVCpu, NewSS);
    }
    if (pDesc->Legacy.Gen.u2Dpl != uCpl)
    {
        Log(("iemMiscValidateNewSSandRsp: %#x - DPL (%d) and CPL (%d) differs -> #TS\n", NewSS, pDesc->Legacy.Gen.u2Dpl, uCpl));
        return iemRaiseTaskSwitchFaultBySelector(pVCpu, NewSS);
    }

    /* Is it there? */
    /** @todo testcase: Is this checked before the canonical / limit check below? */
    if (!pDesc->Legacy.Gen.u1Present)
    {
        Log(("iemMiscValidateNewSSandRsp: %#x - segment not present -> #NP\n", NewSS));
        return iemRaiseSelectorNotPresentBySelector(pVCpu, NewSS);
    }

    return VINF_SUCCESS;
}

/** @} */


/** @name  Raising Exceptions.
 *
 * @{
 */


/**
 * Loads the specified stack far pointer from the TSS.
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   uCpl            The CPL to load the stack for.
 * @param   pSelSS          Where to return the new stack segment.
 * @param   puEsp           Where to return the new stack pointer.
 */
static VBOXSTRICTRC iemRaiseLoadStackFromTss32Or16(PVMCPUCC pVCpu, uint8_t uCpl, PRTSEL pSelSS, uint32_t *puEsp) RT_NOEXCEPT
{
    VBOXSTRICTRC rcStrict;
    Assert(uCpl < 4);

    IEM_CTX_IMPORT_RET(pVCpu, CPUMCTX_EXTRN_TR | CPUMCTX_EXTRN_GDTR | CPUMCTX_EXTRN_LDTR);
    switch (pVCpu->cpum.GstCtx.tr.Attr.n.u4Type)
    {
        /*
         * 16-bit TSS (X86TSS16).
         */
        case X86_SEL_TYPE_SYS_286_TSS_AVAIL: AssertFailed(); RT_FALL_THRU();
        case X86_SEL_TYPE_SYS_286_TSS_BUSY:
        {
            uint32_t off = uCpl * 4 + 2;
            if (off + 4 <= pVCpu->cpum.GstCtx.tr.u32Limit)
            {
                /** @todo check actual access pattern here. */
                uint32_t u32Tmp = 0; /* gcc maybe... */
                rcStrict = iemMemFetchSysU32(pVCpu, &u32Tmp, UINT8_MAX, pVCpu->cpum.GstCtx.tr.u64Base + off);
                if (rcStrict == VINF_SUCCESS)
                {
                    *puEsp  = RT_LOWORD(u32Tmp);
                    *pSelSS = RT_HIWORD(u32Tmp);
                    return VINF_SUCCESS;
                }
            }
            else
            {
                Log(("LoadStackFromTss32Or16: out of bounds! uCpl=%d, u32Limit=%#x TSS16\n", uCpl, pVCpu->cpum.GstCtx.tr.u32Limit));
                rcStrict = iemRaiseTaskSwitchFaultCurrentTSS(pVCpu);
            }
            break;
        }

        /*
         * 32-bit TSS (X86TSS32).
         */
        case X86_SEL_TYPE_SYS_386_TSS_AVAIL: AssertFailed(); RT_FALL_THRU();
        case X86_SEL_TYPE_SYS_386_TSS_BUSY:
        {
            uint32_t off = uCpl * 8 + 4;
            if (off + 7 <= pVCpu->cpum.GstCtx.tr.u32Limit)
            {
/** @todo check actual access pattern here. */
                uint64_t u64Tmp;
                rcStrict = iemMemFetchSysU64(pVCpu, &u64Tmp, UINT8_MAX, pVCpu->cpum.GstCtx.tr.u64Base + off);
                if (rcStrict == VINF_SUCCESS)
                {
                    *puEsp  = u64Tmp & UINT32_MAX;
                    *pSelSS = (RTSEL)(u64Tmp >> 32);
                    return VINF_SUCCESS;
                }
            }
            else
            {
                Log(("LoadStackFromTss32Or16: out of bounds! uCpl=%d, u32Limit=%#x TSS16\n", uCpl, pVCpu->cpum.GstCtx.tr.u32Limit));
                rcStrict = iemRaiseTaskSwitchFaultCurrentTSS(pVCpu);
            }
            break;
        }

        default:
            AssertFailed();
            rcStrict = VERR_IEM_IPE_4;
            break;
    }

    *puEsp  = 0; /* make gcc happy */
    *pSelSS = 0; /* make gcc happy */
    return rcStrict;
}


/**
 * Loads the specified stack pointer from the 64-bit TSS.
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   uCpl            The CPL to load the stack for.
 * @param   uIst            The interrupt stack table index, 0 if to use uCpl.
 * @param   puRsp           Where to return the new stack pointer.
 */
static VBOXSTRICTRC iemRaiseLoadStackFromTss64(PVMCPUCC pVCpu, uint8_t uCpl, uint8_t uIst, uint64_t *puRsp) RT_NOEXCEPT
{
    Assert(uCpl < 4);
    Assert(uIst < 8);
    *puRsp  = 0; /* make gcc happy */

    IEM_CTX_IMPORT_RET(pVCpu, CPUMCTX_EXTRN_TR | CPUMCTX_EXTRN_GDTR | CPUMCTX_EXTRN_LDTR);
    AssertReturn(pVCpu->cpum.GstCtx.tr.Attr.n.u4Type == AMD64_SEL_TYPE_SYS_TSS_BUSY, VERR_IEM_IPE_5);

    uint32_t off;
    if (uIst)
        off = (uIst - 1) * sizeof(uint64_t) + RT_UOFFSETOF(X86TSS64, ist1);
    else
        off = uCpl * sizeof(uint64_t) + RT_UOFFSETOF(X86TSS64, rsp0);
    if (off + sizeof(uint64_t) > pVCpu->cpum.GstCtx.tr.u32Limit)
    {
        Log(("iemRaiseLoadStackFromTss64: out of bounds! uCpl=%d uIst=%d, u32Limit=%#x\n", uCpl, uIst, pVCpu->cpum.GstCtx.tr.u32Limit));
        return iemRaiseTaskSwitchFaultCurrentTSS(pVCpu);
    }

    return iemMemFetchSysU64(pVCpu, puRsp, UINT8_MAX, pVCpu->cpum.GstCtx.tr.u64Base + off);
}


/**
 * Adjust the CPU state according to the exception being raised.
 *
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   u8Vector        The exception that has been raised.
 */
DECLINLINE(void) iemRaiseXcptAdjustState(PVMCPUCC pVCpu, uint8_t u8Vector)
{
    switch (u8Vector)
    {
        case X86_XCPT_DB:
            IEM_CTX_ASSERT(pVCpu, CPUMCTX_EXTRN_DR7);
            pVCpu->cpum.GstCtx.dr[7] &= ~X86_DR7_GD;
            break;
        /** @todo Read the AMD and Intel exception reference... */
    }
}


/**
 * Implements exceptions and interrupts for real mode.
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   cbInstr         The number of bytes to offset rIP by in the return
 *                          address.
 * @param   u8Vector        The interrupt / exception vector number.
 * @param   fFlags          The flags.
 * @param   uErr            The error value if IEM_XCPT_FLAGS_ERR is set.
 * @param   uCr2            The CR2 value if IEM_XCPT_FLAGS_CR2 is set.
 */
static VBOXSTRICTRC
iemRaiseXcptOrIntInRealMode(PVMCPUCC      pVCpu,
                            uint8_t     cbInstr,
                            uint8_t     u8Vector,
                            uint32_t    fFlags,
                            uint16_t    uErr,
                            uint64_t    uCr2) RT_NOEXCEPT
{
    NOREF(uErr); NOREF(uCr2);
    IEM_CTX_ASSERT(pVCpu, IEM_CPUMCTX_EXTRN_XCPT_MASK);

    /*
     * Read the IDT entry.
     */
    if (pVCpu->cpum.GstCtx.idtr.cbIdt < UINT32_C(4) * u8Vector + 3)
    {
        Log(("RaiseXcptOrIntInRealMode: %#x is out of bounds (%#x)\n", u8Vector, pVCpu->cpum.GstCtx.idtr.cbIdt));
        return iemRaiseGeneralProtectionFault(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
    }
    RTFAR16 Idte;
    VBOXSTRICTRC rcStrict = iemMemFetchDataU32(pVCpu, (uint32_t *)&Idte, UINT8_MAX, pVCpu->cpum.GstCtx.idtr.pIdt + UINT32_C(4) * u8Vector);
    if (RT_UNLIKELY(rcStrict != VINF_SUCCESS))
    {
        Log(("iemRaiseXcptOrIntInRealMode: failed to fetch IDT entry! vec=%#x rc=%Rrc\n", u8Vector, VBOXSTRICTRC_VAL(rcStrict)));
        return rcStrict;
    }

    /*
     * Push the stack frame.
     */
    uint16_t *pu16Frame;
    uint64_t  uNewRsp;
    rcStrict = iemMemStackPushBeginSpecial(pVCpu, 6, 3, (void **)&pu16Frame, &uNewRsp);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    uint32_t fEfl = IEMMISC_GET_EFL(pVCpu);
#if IEM_CFG_TARGET_CPU == IEMTARGETCPU_DYNAMIC
    AssertCompile(IEMTARGETCPU_8086 <= IEMTARGETCPU_186 && IEMTARGETCPU_V20 <= IEMTARGETCPU_186 && IEMTARGETCPU_286 > IEMTARGETCPU_186);
    if (pVCpu->iem.s.uTargetCpu <= IEMTARGETCPU_186)
        fEfl |= UINT16_C(0xf000);
#endif
    pu16Frame[2] = (uint16_t)fEfl;
    pu16Frame[1] = (uint16_t)pVCpu->cpum.GstCtx.cs.Sel;
    pu16Frame[0] = (fFlags & IEM_XCPT_FLAGS_T_SOFT_INT) ? pVCpu->cpum.GstCtx.ip + cbInstr : pVCpu->cpum.GstCtx.ip;
    rcStrict = iemMemStackPushCommitSpecial(pVCpu, pu16Frame, uNewRsp);
    if (RT_UNLIKELY(rcStrict != VINF_SUCCESS))
        return rcStrict;

    /*
     * Load the vector address into cs:ip and make exception specific state
     * adjustments.
     */
    pVCpu->cpum.GstCtx.cs.Sel           = Idte.sel;
    pVCpu->cpum.GstCtx.cs.ValidSel      = Idte.sel;
    pVCpu->cpum.GstCtx.cs.fFlags        = CPUMSELREG_FLAGS_VALID;
    pVCpu->cpum.GstCtx.cs.u64Base       = (uint32_t)Idte.sel << 4;
    /** @todo do we load attribs and limit as well? Should we check against limit like far jump? */
    pVCpu->cpum.GstCtx.rip              = Idte.off;
    fEfl &= ~(X86_EFL_IF | X86_EFL_TF | X86_EFL_AC);
    IEMMISC_SET_EFL(pVCpu, fEfl);

    /** @todo do we actually do this in real mode? */
    if (fFlags & IEM_XCPT_FLAGS_T_CPU_XCPT)
        iemRaiseXcptAdjustState(pVCpu, u8Vector);

    return fFlags & IEM_XCPT_FLAGS_T_CPU_XCPT ? VINF_IEM_RAISED_XCPT : VINF_SUCCESS;
}


/**
 * Loads a NULL data selector into when coming from V8086 mode.
 *
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   pSReg           Pointer to the segment register.
 */
DECLINLINE(void) iemHlpLoadNullDataSelectorOnV86Xcpt(PVMCPUCC pVCpu, PCPUMSELREG pSReg)
{
    pSReg->Sel      = 0;
    pSReg->ValidSel = 0;
    if (IEM_IS_GUEST_CPU_INTEL(pVCpu))
    {
        /* VT-x (Intel 3960x) doesn't change the base and limit, clears and sets the following attributes */
        pSReg->Attr.u &= X86DESCATTR_DT | X86DESCATTR_TYPE | X86DESCATTR_DPL | X86DESCATTR_G | X86DESCATTR_D;
        pSReg->Attr.u |= X86DESCATTR_UNUSABLE;
    }
    else
    {
        pSReg->fFlags   = CPUMSELREG_FLAGS_VALID;
        /** @todo check this on AMD-V */
        pSReg->u64Base  = 0;
        pSReg->u32Limit = 0;
    }
}


/**
 * Loads a segment selector during a task switch in V8086 mode.
 *
 * @param   pSReg           Pointer to the segment register.
 * @param   uSel            The selector value to load.
 */
DECLINLINE(void) iemHlpLoadSelectorInV86Mode(PCPUMSELREG pSReg, uint16_t uSel)
{
    /* See Intel spec. 26.3.1.2 "Checks on Guest Segment Registers". */
    pSReg->Sel      = uSel;
    pSReg->ValidSel = uSel;
    pSReg->fFlags   = CPUMSELREG_FLAGS_VALID;
    pSReg->u64Base  = uSel << 4;
    pSReg->u32Limit = 0xffff;
    pSReg->Attr.u   = 0xf3;
}


/**
 * Loads a segment selector during a task switch in protected mode.
 *
 * In this task switch scenario, we would throw \#TS exceptions rather than
 * \#GPs.
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   pSReg           Pointer to the segment register.
 * @param   uSel            The new selector value.
 *
 * @remarks This does _not_ handle CS or SS.
 * @remarks This expects pVCpu->iem.s.uCpl to be up to date.
 */
static VBOXSTRICTRC iemHlpTaskSwitchLoadDataSelectorInProtMode(PVMCPUCC pVCpu, PCPUMSELREG pSReg, uint16_t uSel) RT_NOEXCEPT
{
    Assert(pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT);

    /* Null data selector. */
    if (!(uSel & X86_SEL_MASK_OFF_RPL))
    {
        iemHlpLoadNullDataSelectorProt(pVCpu, pSReg, uSel);
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSReg));
        CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_HIDDEN_SEL_REGS);
        return VINF_SUCCESS;
    }

    /* Fetch the descriptor. */
    IEMSELDESC Desc;
    VBOXSTRICTRC rcStrict = iemMemFetchSelDesc(pVCpu, &Desc, uSel, X86_XCPT_TS);
    if (rcStrict != VINF_SUCCESS)
    {
        Log(("iemHlpTaskSwitchLoadDataSelectorInProtMode: failed to fetch selector. uSel=%u rc=%Rrc\n", uSel,
             VBOXSTRICTRC_VAL(rcStrict)));
        return rcStrict;
    }

    /* Must be a data segment or readable code segment. */
    if (   !Desc.Legacy.Gen.u1DescType
        || (Desc.Legacy.Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_READ)) == X86_SEL_TYPE_CODE)
    {
        Log(("iemHlpTaskSwitchLoadDataSelectorInProtMode: invalid segment type. uSel=%u Desc.u4Type=%#x\n", uSel,
             Desc.Legacy.Gen.u4Type));
        return iemRaiseTaskSwitchFaultWithErr(pVCpu, uSel & X86_SEL_MASK_OFF_RPL);
    }

    /* Check privileges for data segments and non-conforming code segments. */
    if (   (Desc.Legacy.Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_CONF))
        != (X86_SEL_TYPE_CODE | X86_SEL_TYPE_CONF))
    {
        /* The RPL and the new CPL must be less than or equal to the DPL. */
        if (   (unsigned)(uSel & X86_SEL_RPL) > Desc.Legacy.Gen.u2Dpl
            || (pVCpu->iem.s.uCpl > Desc.Legacy.Gen.u2Dpl))
        {
            Log(("iemHlpTaskSwitchLoadDataSelectorInProtMode: Invalid priv. uSel=%u uSel.RPL=%u DPL=%u CPL=%u\n",
                 uSel, (uSel & X86_SEL_RPL), Desc.Legacy.Gen.u2Dpl, pVCpu->iem.s.uCpl));
            return iemRaiseTaskSwitchFaultWithErr(pVCpu, uSel & X86_SEL_MASK_OFF_RPL);
        }
    }

    /* Is it there? */
    if (!Desc.Legacy.Gen.u1Present)
    {
        Log(("iemHlpTaskSwitchLoadDataSelectorInProtMode: Segment not present. uSel=%u\n", uSel));
        return iemRaiseSelectorNotPresentWithErr(pVCpu, uSel & X86_SEL_MASK_OFF_RPL);
    }

    /* The base and limit. */
    uint32_t cbLimit = X86DESC_LIMIT_G(&Desc.Legacy);
    uint64_t u64Base = X86DESC_BASE(&Desc.Legacy);

    /*
     * Ok, everything checked out fine. Now set the accessed bit before
     * committing the result into the registers.
     */
    if (!(Desc.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
    {
        rcStrict = iemMemMarkSelDescAccessed(pVCpu, uSel);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        Desc.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
    }

    /* Commit */
    pSReg->Sel      = uSel;
    pSReg->Attr.u   = X86DESC_GET_HID_ATTR(&Desc.Legacy);
    pSReg->u32Limit = cbLimit;
    pSReg->u64Base  = u64Base;  /** @todo testcase/investigate: seen claims that the upper half of the base remains unchanged... */
    pSReg->ValidSel = uSel;
    pSReg->fFlags   = CPUMSELREG_FLAGS_VALID;
    if (IEM_IS_GUEST_CPU_INTEL(pVCpu))
        pSReg->Attr.u &= ~X86DESCATTR_UNUSABLE;

    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSReg));
    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_HIDDEN_SEL_REGS);
    return VINF_SUCCESS;
}


/**
 * Performs a task switch.
 *
 * If the task switch is the result of a JMP, CALL or IRET instruction, the
 * caller is responsible for performing the necessary checks (like DPL, TSS
 * present etc.) which are specific to JMP/CALL/IRET. See Intel Instruction
 * reference for JMP, CALL, IRET.
 *
 * If the task switch is the due to a software interrupt or hardware exception,
 * the caller is responsible for validating the TSS selector and descriptor. See
 * Intel Instruction reference for INT n.
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   enmTaskSwitch   The cause of the task switch.
 * @param   uNextEip        The EIP effective after the task switch.
 * @param   fFlags          The flags, see IEM_XCPT_FLAGS_XXX.
 * @param   uErr            The error value if IEM_XCPT_FLAGS_ERR is set.
 * @param   uCr2            The CR2 value if IEM_XCPT_FLAGS_CR2 is set.
 * @param   SelTSS          The TSS selector of the new task.
 * @param   pNewDescTSS     Pointer to the new TSS descriptor.
 */
VBOXSTRICTRC
iemTaskSwitch(PVMCPUCC        pVCpu,
              IEMTASKSWITCH   enmTaskSwitch,
              uint32_t        uNextEip,
              uint32_t        fFlags,
              uint16_t        uErr,
              uint64_t        uCr2,
              RTSEL           SelTSS,
              PIEMSELDESC     pNewDescTSS) RT_NOEXCEPT
{
    Assert(!IEM_IS_REAL_MODE(pVCpu));
    Assert(pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT);
    IEM_CTX_ASSERT(pVCpu, IEM_CPUMCTX_EXTRN_XCPT_MASK);

    uint32_t const uNewTSSType = pNewDescTSS->Legacy.Gate.u4Type;
    Assert(   uNewTSSType == X86_SEL_TYPE_SYS_286_TSS_AVAIL
           || uNewTSSType == X86_SEL_TYPE_SYS_286_TSS_BUSY
           || uNewTSSType == X86_SEL_TYPE_SYS_386_TSS_AVAIL
           || uNewTSSType == X86_SEL_TYPE_SYS_386_TSS_BUSY);

    bool const fIsNewTSS386 = (   uNewTSSType == X86_SEL_TYPE_SYS_386_TSS_AVAIL
                               || uNewTSSType == X86_SEL_TYPE_SYS_386_TSS_BUSY);

    Log(("iemTaskSwitch: enmTaskSwitch=%u NewTSS=%#x fIsNewTSS386=%RTbool EIP=%#RX32 uNextEip=%#RX32\n", enmTaskSwitch, SelTSS,
         fIsNewTSS386, pVCpu->cpum.GstCtx.eip, uNextEip));

    /* Update CR2 in case it's a page-fault. */
    /** @todo This should probably be done much earlier in IEM/PGM. See
     *        @bugref{5653#c49}. */
    if (fFlags & IEM_XCPT_FLAGS_CR2)
        pVCpu->cpum.GstCtx.cr2 = uCr2;

    /*
     * Check the new TSS limit. See Intel spec. 6.15 "Exception and Interrupt Reference"
     * subsection "Interrupt 10 - Invalid TSS Exception (#TS)".
     */
    uint32_t const uNewTSSLimit    = pNewDescTSS->Legacy.Gen.u16LimitLow | (pNewDescTSS->Legacy.Gen.u4LimitHigh << 16);
    uint32_t const uNewTSSLimitMin = fIsNewTSS386 ? X86_SEL_TYPE_SYS_386_TSS_LIMIT_MIN : X86_SEL_TYPE_SYS_286_TSS_LIMIT_MIN;
    if (uNewTSSLimit < uNewTSSLimitMin)
    {
        Log(("iemTaskSwitch: Invalid new TSS limit. enmTaskSwitch=%u uNewTSSLimit=%#x uNewTSSLimitMin=%#x -> #TS\n",
             enmTaskSwitch, uNewTSSLimit, uNewTSSLimitMin));
        return iemRaiseTaskSwitchFaultWithErr(pVCpu, SelTSS & X86_SEL_MASK_OFF_RPL);
    }

    /*
     * Task switches in VMX non-root mode always cause task switches.
     * The new TSS must have been read and validated (DPL, limits etc.) before a
     * task-switch VM-exit commences.
     *
     * See Intel spec. 25.4.2 "Treatment of Task Switches".
     */
    if (IEM_VMX_IS_NON_ROOT_MODE(pVCpu))
    {
        Log(("iemTaskSwitch: Guest intercept (source=%u, sel=%#x) -> VM-exit.\n", enmTaskSwitch, SelTSS));
        IEM_VMX_VMEXIT_TASK_SWITCH_RET(pVCpu, enmTaskSwitch, SelTSS, uNextEip - pVCpu->cpum.GstCtx.eip);
    }

    /*
     * The SVM nested-guest intercept for task-switch takes priority over all exceptions
     * after validating the incoming (new) TSS, see AMD spec. 15.14.1 "Task Switch Intercept".
     */
    if (IEM_SVM_IS_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_TASK_SWITCH))
    {
        uint32_t const uExitInfo1 = SelTSS;
        uint32_t       uExitInfo2 = uErr;
        switch (enmTaskSwitch)
        {
            case IEMTASKSWITCH_JUMP: uExitInfo2 |= SVM_EXIT2_TASK_SWITCH_JUMP; break;
            case IEMTASKSWITCH_IRET: uExitInfo2 |= SVM_EXIT2_TASK_SWITCH_IRET; break;
            default: break;
        }
        if (fFlags & IEM_XCPT_FLAGS_ERR)
            uExitInfo2 |= SVM_EXIT2_TASK_SWITCH_HAS_ERROR_CODE;
        if (pVCpu->cpum.GstCtx.eflags.Bits.u1RF)
            uExitInfo2 |= SVM_EXIT2_TASK_SWITCH_EFLAGS_RF;

        Log(("iemTaskSwitch: Guest intercept -> #VMEXIT. uExitInfo1=%#RX64 uExitInfo2=%#RX64\n", uExitInfo1, uExitInfo2));
        IEM_SVM_VMEXIT_RET(pVCpu, SVM_EXIT_TASK_SWITCH, uExitInfo1, uExitInfo2);
        RT_NOREF2(uExitInfo1, uExitInfo2);
    }

    /*
     * Check the current TSS limit. The last written byte to the current TSS during the
     * task switch will be 2 bytes at offset 0x5C (32-bit) and 1 byte at offset 0x28 (16-bit).
     * See Intel spec. 7.2.1 "Task-State Segment (TSS)" for static and dynamic fields.
     *
     * The AMD docs doesn't mention anything about limit checks with LTR which suggests you can
     * end up with smaller than "legal" TSS limits.
     */
    uint32_t const uCurTSSLimit    = pVCpu->cpum.GstCtx.tr.u32Limit;
    uint32_t const uCurTSSLimitMin = fIsNewTSS386 ? 0x5F : 0x29;
    if (uCurTSSLimit < uCurTSSLimitMin)
    {
        Log(("iemTaskSwitch: Invalid current TSS limit. enmTaskSwitch=%u uCurTSSLimit=%#x uCurTSSLimitMin=%#x -> #TS\n",
             enmTaskSwitch, uCurTSSLimit, uCurTSSLimitMin));
        return iemRaiseTaskSwitchFaultWithErr(pVCpu, SelTSS & X86_SEL_MASK_OFF_RPL);
    }

    /*
     * Verify that the new TSS can be accessed and map it. Map only the required contents
     * and not the entire TSS.
     */
    void           *pvNewTSS;
    uint32_t  const cbNewTSS    = uNewTSSLimitMin + 1;
    RTGCPTR   const GCPtrNewTSS = X86DESC_BASE(&pNewDescTSS->Legacy);
    AssertCompile(sizeof(X86TSS32) == X86_SEL_TYPE_SYS_386_TSS_LIMIT_MIN + 1);
    /** @todo Handle if the TSS crosses a page boundary. Intel specifies that it may
     *        not perform correct translation if this happens. See Intel spec. 7.2.1
     *        "Task-State Segment". */
    VBOXSTRICTRC rcStrict = iemMemMap(pVCpu, &pvNewTSS, cbNewTSS, UINT8_MAX, GCPtrNewTSS, IEM_ACCESS_SYS_RW, 0);
    if (rcStrict != VINF_SUCCESS)
    {
        Log(("iemTaskSwitch: Failed to read new TSS. enmTaskSwitch=%u cbNewTSS=%u uNewTSSLimit=%u rc=%Rrc\n", enmTaskSwitch,
             cbNewTSS, uNewTSSLimit, VBOXSTRICTRC_VAL(rcStrict)));
        return rcStrict;
    }

    /*
     * Clear the busy bit in current task's TSS descriptor if it's a task switch due to JMP/IRET.
     */
    uint32_t fEFlags = pVCpu->cpum.GstCtx.eflags.u;
    if (   enmTaskSwitch == IEMTASKSWITCH_JUMP
        || enmTaskSwitch == IEMTASKSWITCH_IRET)
    {
        PX86DESC pDescCurTSS;
        rcStrict = iemMemMap(pVCpu, (void **)&pDescCurTSS, sizeof(*pDescCurTSS), UINT8_MAX,
                             pVCpu->cpum.GstCtx.gdtr.pGdt + (pVCpu->cpum.GstCtx.tr.Sel & X86_SEL_MASK), IEM_ACCESS_SYS_RW, 0);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: Failed to read new TSS descriptor in GDT. enmTaskSwitch=%u pGdt=%#RX64 rc=%Rrc\n",
                 enmTaskSwitch, pVCpu->cpum.GstCtx.gdtr.pGdt, VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }

        pDescCurTSS->Gate.u4Type &= ~X86_SEL_TYPE_SYS_TSS_BUSY_MASK;
        rcStrict = iemMemCommitAndUnmap(pVCpu, pDescCurTSS, IEM_ACCESS_SYS_RW);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: Failed to commit new TSS descriptor in GDT. enmTaskSwitch=%u pGdt=%#RX64 rc=%Rrc\n",
                 enmTaskSwitch, pVCpu->cpum.GstCtx.gdtr.pGdt, VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }

        /* Clear EFLAGS.NT (Nested Task) in the eflags memory image, if it's a task switch due to an IRET. */
        if (enmTaskSwitch == IEMTASKSWITCH_IRET)
        {
            Assert(   uNewTSSType == X86_SEL_TYPE_SYS_286_TSS_BUSY
                   || uNewTSSType == X86_SEL_TYPE_SYS_386_TSS_BUSY);
            fEFlags &= ~X86_EFL_NT;
        }
    }

    /*
     * Save the CPU state into the current TSS.
     */
    RTGCPTR const GCPtrCurTSS = pVCpu->cpum.GstCtx.tr.u64Base;
    if (GCPtrNewTSS == GCPtrCurTSS)
    {
        Log(("iemTaskSwitch: Switching to the same TSS! enmTaskSwitch=%u GCPtr[Cur|New]TSS=%#RGv\n", enmTaskSwitch, GCPtrCurTSS));
        Log(("uCurCr3=%#x uCurEip=%#x uCurEflags=%#x uCurEax=%#x uCurEsp=%#x uCurEbp=%#x uCurCS=%#04x uCurSS=%#04x uCurLdt=%#x\n",
             pVCpu->cpum.GstCtx.cr3, pVCpu->cpum.GstCtx.eip, pVCpu->cpum.GstCtx.eflags.u, pVCpu->cpum.GstCtx.eax,
             pVCpu->cpum.GstCtx.esp, pVCpu->cpum.GstCtx.ebp, pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.ss.Sel,
             pVCpu->cpum.GstCtx.ldtr.Sel));
    }
    if (fIsNewTSS386)
    {
        /*
         * Verify that the current TSS (32-bit) can be accessed, only the minimum required size.
         * See Intel spec. 7.2.1 "Task-State Segment (TSS)" for static and dynamic fields.
         */
        void          *pvCurTSS32;
        uint32_t const offCurTSS = RT_UOFFSETOF(X86TSS32, eip);
        uint32_t const cbCurTSS  = RT_UOFFSETOF(X86TSS32, selLdt) - RT_UOFFSETOF(X86TSS32, eip);
        AssertCompile(RTASSERT_OFFSET_OF(X86TSS32, selLdt) - RTASSERT_OFFSET_OF(X86TSS32, eip) == 64);
        rcStrict = iemMemMap(pVCpu, &pvCurTSS32, cbCurTSS, UINT8_MAX, GCPtrCurTSS + offCurTSS, IEM_ACCESS_SYS_RW, 0);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: Failed to read current 32-bit TSS. enmTaskSwitch=%u GCPtrCurTSS=%#RGv cb=%u rc=%Rrc\n",
                 enmTaskSwitch, GCPtrCurTSS, cbCurTSS, VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }

        /* !! WARNING !! Access -only- the members (dynamic fields) that are mapped, i.e interval [offCurTSS..cbCurTSS). */
        PX86TSS32 pCurTSS32 = (PX86TSS32)((uintptr_t)pvCurTSS32 - offCurTSS);
        pCurTSS32->eip    = uNextEip;
        pCurTSS32->eflags = fEFlags;
        pCurTSS32->eax    = pVCpu->cpum.GstCtx.eax;
        pCurTSS32->ecx    = pVCpu->cpum.GstCtx.ecx;
        pCurTSS32->edx    = pVCpu->cpum.GstCtx.edx;
        pCurTSS32->ebx    = pVCpu->cpum.GstCtx.ebx;
        pCurTSS32->esp    = pVCpu->cpum.GstCtx.esp;
        pCurTSS32->ebp    = pVCpu->cpum.GstCtx.ebp;
        pCurTSS32->esi    = pVCpu->cpum.GstCtx.esi;
        pCurTSS32->edi    = pVCpu->cpum.GstCtx.edi;
        pCurTSS32->es     = pVCpu->cpum.GstCtx.es.Sel;
        pCurTSS32->cs     = pVCpu->cpum.GstCtx.cs.Sel;
        pCurTSS32->ss     = pVCpu->cpum.GstCtx.ss.Sel;
        pCurTSS32->ds     = pVCpu->cpum.GstCtx.ds.Sel;
        pCurTSS32->fs     = pVCpu->cpum.GstCtx.fs.Sel;
        pCurTSS32->gs     = pVCpu->cpum.GstCtx.gs.Sel;

        rcStrict = iemMemCommitAndUnmap(pVCpu, pvCurTSS32, IEM_ACCESS_SYS_RW);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: Failed to commit current 32-bit TSS. enmTaskSwitch=%u rc=%Rrc\n", enmTaskSwitch,
                 VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }
    }
    else
    {
        /*
         * Verify that the current TSS (16-bit) can be accessed. Again, only the minimum required size.
         */
        void          *pvCurTSS16;
        uint32_t const offCurTSS = RT_UOFFSETOF(X86TSS16, ip);
        uint32_t const cbCurTSS  = RT_UOFFSETOF(X86TSS16, selLdt) - RT_UOFFSETOF(X86TSS16, ip);
        AssertCompile(RTASSERT_OFFSET_OF(X86TSS16, selLdt) - RTASSERT_OFFSET_OF(X86TSS16, ip) == 28);
        rcStrict = iemMemMap(pVCpu, &pvCurTSS16, cbCurTSS, UINT8_MAX, GCPtrCurTSS + offCurTSS, IEM_ACCESS_SYS_RW, 0);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: Failed to read current 16-bit TSS. enmTaskSwitch=%u GCPtrCurTSS=%#RGv cb=%u rc=%Rrc\n",
                 enmTaskSwitch, GCPtrCurTSS, cbCurTSS, VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }

        /* !! WARNING !! Access -only- the members (dynamic fields) that are mapped, i.e interval [offCurTSS..cbCurTSS). */
        PX86TSS16 pCurTSS16 = (PX86TSS16)((uintptr_t)pvCurTSS16 - offCurTSS);
        pCurTSS16->ip    = uNextEip;
        pCurTSS16->flags = (uint16_t)fEFlags;
        pCurTSS16->ax    = pVCpu->cpum.GstCtx.ax;
        pCurTSS16->cx    = pVCpu->cpum.GstCtx.cx;
        pCurTSS16->dx    = pVCpu->cpum.GstCtx.dx;
        pCurTSS16->bx    = pVCpu->cpum.GstCtx.bx;
        pCurTSS16->sp    = pVCpu->cpum.GstCtx.sp;
        pCurTSS16->bp    = pVCpu->cpum.GstCtx.bp;
        pCurTSS16->si    = pVCpu->cpum.GstCtx.si;
        pCurTSS16->di    = pVCpu->cpum.GstCtx.di;
        pCurTSS16->es    = pVCpu->cpum.GstCtx.es.Sel;
        pCurTSS16->cs    = pVCpu->cpum.GstCtx.cs.Sel;
        pCurTSS16->ss    = pVCpu->cpum.GstCtx.ss.Sel;
        pCurTSS16->ds    = pVCpu->cpum.GstCtx.ds.Sel;

        rcStrict = iemMemCommitAndUnmap(pVCpu, pvCurTSS16, IEM_ACCESS_SYS_RW);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: Failed to commit current 16-bit TSS. enmTaskSwitch=%u rc=%Rrc\n", enmTaskSwitch,
                 VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }
    }

    /*
     * Update the previous task link field for the new TSS, if the task switch is due to a CALL/INT_XCPT.
     */
    if (   enmTaskSwitch == IEMTASKSWITCH_CALL
        || enmTaskSwitch == IEMTASKSWITCH_INT_XCPT)
    {
        /* 16 or 32-bit TSS doesn't matter, we only access the first, common 16-bit field (selPrev) here. */
        PX86TSS32 pNewTSS = (PX86TSS32)pvNewTSS;
        pNewTSS->selPrev  = pVCpu->cpum.GstCtx.tr.Sel;
    }

    /*
     * Read the state from the new TSS into temporaries. Setting it immediately as the new CPU state is tricky,
     * it's done further below with error handling (e.g. CR3 changes will go through PGM).
     */
    uint32_t uNewCr3, uNewEip, uNewEflags, uNewEax, uNewEcx, uNewEdx, uNewEbx, uNewEsp, uNewEbp, uNewEsi, uNewEdi;
    uint16_t uNewES,  uNewCS, uNewSS, uNewDS, uNewFS, uNewGS, uNewLdt;
    bool     fNewDebugTrap;
    if (fIsNewTSS386)
    {
        PCX86TSS32 pNewTSS32 = (PCX86TSS32)pvNewTSS;
        uNewCr3       = (pVCpu->cpum.GstCtx.cr0 & X86_CR0_PG) ? pNewTSS32->cr3 : 0;
        uNewEip       = pNewTSS32->eip;
        uNewEflags    = pNewTSS32->eflags;
        uNewEax       = pNewTSS32->eax;
        uNewEcx       = pNewTSS32->ecx;
        uNewEdx       = pNewTSS32->edx;
        uNewEbx       = pNewTSS32->ebx;
        uNewEsp       = pNewTSS32->esp;
        uNewEbp       = pNewTSS32->ebp;
        uNewEsi       = pNewTSS32->esi;
        uNewEdi       = pNewTSS32->edi;
        uNewES        = pNewTSS32->es;
        uNewCS        = pNewTSS32->cs;
        uNewSS        = pNewTSS32->ss;
        uNewDS        = pNewTSS32->ds;
        uNewFS        = pNewTSS32->fs;
        uNewGS        = pNewTSS32->gs;
        uNewLdt       = pNewTSS32->selLdt;
        fNewDebugTrap = RT_BOOL(pNewTSS32->fDebugTrap);
    }
    else
    {
        PCX86TSS16 pNewTSS16 = (PCX86TSS16)pvNewTSS;
        uNewCr3       = 0;
        uNewEip       = pNewTSS16->ip;
        uNewEflags    = pNewTSS16->flags;
        uNewEax       = UINT32_C(0xffff0000) | pNewTSS16->ax;
        uNewEcx       = UINT32_C(0xffff0000) | pNewTSS16->cx;
        uNewEdx       = UINT32_C(0xffff0000) | pNewTSS16->dx;
        uNewEbx       = UINT32_C(0xffff0000) | pNewTSS16->bx;
        uNewEsp       = UINT32_C(0xffff0000) | pNewTSS16->sp;
        uNewEbp       = UINT32_C(0xffff0000) | pNewTSS16->bp;
        uNewEsi       = UINT32_C(0xffff0000) | pNewTSS16->si;
        uNewEdi       = UINT32_C(0xffff0000) | pNewTSS16->di;
        uNewES        = pNewTSS16->es;
        uNewCS        = pNewTSS16->cs;
        uNewSS        = pNewTSS16->ss;
        uNewDS        = pNewTSS16->ds;
        uNewFS        = 0;
        uNewGS        = 0;
        uNewLdt       = pNewTSS16->selLdt;
        fNewDebugTrap = false;
    }

    if (GCPtrNewTSS == GCPtrCurTSS)
        Log(("uNewCr3=%#x uNewEip=%#x uNewEflags=%#x uNewEax=%#x uNewEsp=%#x uNewEbp=%#x uNewCS=%#04x uNewSS=%#04x uNewLdt=%#x\n",
             uNewCr3, uNewEip, uNewEflags, uNewEax, uNewEsp, uNewEbp, uNewCS, uNewSS, uNewLdt));

    /*
     * We're done accessing the new TSS.
     */
    rcStrict = iemMemCommitAndUnmap(pVCpu, pvNewTSS, IEM_ACCESS_SYS_RW);
    if (rcStrict != VINF_SUCCESS)
    {
        Log(("iemTaskSwitch: Failed to commit new TSS. enmTaskSwitch=%u rc=%Rrc\n", enmTaskSwitch, VBOXSTRICTRC_VAL(rcStrict)));
        return rcStrict;
    }

    /*
     * Set the busy bit in the new TSS descriptor, if the task switch is a JMP/CALL/INT_XCPT.
     */
    if (enmTaskSwitch != IEMTASKSWITCH_IRET)
    {
        rcStrict = iemMemMap(pVCpu, (void **)&pNewDescTSS, sizeof(*pNewDescTSS), UINT8_MAX,
                             pVCpu->cpum.GstCtx.gdtr.pGdt + (SelTSS & X86_SEL_MASK), IEM_ACCESS_SYS_RW, 0);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: Failed to read new TSS descriptor in GDT (2). enmTaskSwitch=%u pGdt=%#RX64 rc=%Rrc\n",
                 enmTaskSwitch, pVCpu->cpum.GstCtx.gdtr.pGdt, VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }

        /* Check that the descriptor indicates the new TSS is available (not busy). */
        AssertMsg(   pNewDescTSS->Legacy.Gate.u4Type == X86_SEL_TYPE_SYS_286_TSS_AVAIL
                  || pNewDescTSS->Legacy.Gate.u4Type == X86_SEL_TYPE_SYS_386_TSS_AVAIL,
                     ("Invalid TSS descriptor type=%#x", pNewDescTSS->Legacy.Gate.u4Type));

        pNewDescTSS->Legacy.Gate.u4Type |= X86_SEL_TYPE_SYS_TSS_BUSY_MASK;
        rcStrict = iemMemCommitAndUnmap(pVCpu, pNewDescTSS, IEM_ACCESS_SYS_RW);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: Failed to commit new TSS descriptor in GDT (2). enmTaskSwitch=%u pGdt=%#RX64 rc=%Rrc\n",
                 enmTaskSwitch, pVCpu->cpum.GstCtx.gdtr.pGdt, VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }
    }

    /*
     * From this point on, we're technically in the new task. We will defer exceptions
     * until the completion of the task switch but before executing any instructions in the new task.
     */
    pVCpu->cpum.GstCtx.tr.Sel      = SelTSS;
    pVCpu->cpum.GstCtx.tr.ValidSel = SelTSS;
    pVCpu->cpum.GstCtx.tr.fFlags   = CPUMSELREG_FLAGS_VALID;
    pVCpu->cpum.GstCtx.tr.Attr.u   = X86DESC_GET_HID_ATTR(&pNewDescTSS->Legacy);
    pVCpu->cpum.GstCtx.tr.u32Limit = X86DESC_LIMIT_G(&pNewDescTSS->Legacy);
    pVCpu->cpum.GstCtx.tr.u64Base  = X86DESC_BASE(&pNewDescTSS->Legacy);
    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_TR);

    /* Set the busy bit in TR. */
    pVCpu->cpum.GstCtx.tr.Attr.n.u4Type |= X86_SEL_TYPE_SYS_TSS_BUSY_MASK;

    /* Set EFLAGS.NT (Nested Task) in the eflags loaded from the new TSS, if it's a task switch due to a CALL/INT_XCPT. */
    if (   enmTaskSwitch == IEMTASKSWITCH_CALL
        || enmTaskSwitch == IEMTASKSWITCH_INT_XCPT)
    {
        uNewEflags |= X86_EFL_NT;
    }

    pVCpu->cpum.GstCtx.dr[7] &= ~X86_DR7_LE_ALL;     /** @todo Should we clear DR7.LE bit too? */
    pVCpu->cpum.GstCtx.cr0   |= X86_CR0_TS;
    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_CR0);

    pVCpu->cpum.GstCtx.eip    = uNewEip;
    pVCpu->cpum.GstCtx.eax    = uNewEax;
    pVCpu->cpum.GstCtx.ecx    = uNewEcx;
    pVCpu->cpum.GstCtx.edx    = uNewEdx;
    pVCpu->cpum.GstCtx.ebx    = uNewEbx;
    pVCpu->cpum.GstCtx.esp    = uNewEsp;
    pVCpu->cpum.GstCtx.ebp    = uNewEbp;
    pVCpu->cpum.GstCtx.esi    = uNewEsi;
    pVCpu->cpum.GstCtx.edi    = uNewEdi;

    uNewEflags &= X86_EFL_LIVE_MASK;
    uNewEflags |= X86_EFL_RA1_MASK;
    IEMMISC_SET_EFL(pVCpu, uNewEflags);

    /*
     * Switch the selectors here and do the segment checks later. If we throw exceptions, the selectors
     * will be valid in the exception handler. We cannot update the hidden parts until we've switched CR3
     * due to the hidden part data originating from the guest LDT/GDT which is accessed through paging.
     */
    pVCpu->cpum.GstCtx.es.Sel       = uNewES;
    pVCpu->cpum.GstCtx.es.Attr.u   &= ~X86DESCATTR_P;

    pVCpu->cpum.GstCtx.cs.Sel       = uNewCS;
    pVCpu->cpum.GstCtx.cs.Attr.u   &= ~X86DESCATTR_P;

    pVCpu->cpum.GstCtx.ss.Sel       = uNewSS;
    pVCpu->cpum.GstCtx.ss.Attr.u   &= ~X86DESCATTR_P;

    pVCpu->cpum.GstCtx.ds.Sel       = uNewDS;
    pVCpu->cpum.GstCtx.ds.Attr.u   &= ~X86DESCATTR_P;

    pVCpu->cpum.GstCtx.fs.Sel       = uNewFS;
    pVCpu->cpum.GstCtx.fs.Attr.u   &= ~X86DESCATTR_P;

    pVCpu->cpum.GstCtx.gs.Sel       = uNewGS;
    pVCpu->cpum.GstCtx.gs.Attr.u   &= ~X86DESCATTR_P;
    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_HIDDEN_SEL_REGS);

    pVCpu->cpum.GstCtx.ldtr.Sel     = uNewLdt;
    pVCpu->cpum.GstCtx.ldtr.fFlags  = CPUMSELREG_FLAGS_STALE;
    pVCpu->cpum.GstCtx.ldtr.Attr.u &= ~X86DESCATTR_P;
    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_LDTR);

    if (IEM_IS_GUEST_CPU_INTEL(pVCpu))
    {
        pVCpu->cpum.GstCtx.es.Attr.u   |= X86DESCATTR_UNUSABLE;
        pVCpu->cpum.GstCtx.cs.Attr.u   |= X86DESCATTR_UNUSABLE;
        pVCpu->cpum.GstCtx.ss.Attr.u   |= X86DESCATTR_UNUSABLE;
        pVCpu->cpum.GstCtx.ds.Attr.u   |= X86DESCATTR_UNUSABLE;
        pVCpu->cpum.GstCtx.fs.Attr.u   |= X86DESCATTR_UNUSABLE;
        pVCpu->cpum.GstCtx.gs.Attr.u   |= X86DESCATTR_UNUSABLE;
        pVCpu->cpum.GstCtx.ldtr.Attr.u |= X86DESCATTR_UNUSABLE;
    }

    /*
     * Switch CR3 for the new task.
     */
    if (   fIsNewTSS386
        && (pVCpu->cpum.GstCtx.cr0 & X86_CR0_PG))
    {
        /** @todo Should we update and flush TLBs only if CR3 value actually changes? */
        int rc = CPUMSetGuestCR3(pVCpu, uNewCr3);
        AssertRCSuccessReturn(rc, rc);

        /* Inform PGM. */
        /** @todo Should we raise \#GP(0) here when PAE PDPEs are invalid? */
        rc = PGMFlushTLB(pVCpu, pVCpu->cpum.GstCtx.cr3, !(pVCpu->cpum.GstCtx.cr4 & X86_CR4_PGE));
        AssertRCReturn(rc, rc);
        /* ignore informational status codes */

        CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_CR3);
    }

    /*
     * Switch LDTR for the new task.
     */
    if (!(uNewLdt & X86_SEL_MASK_OFF_RPL))
        iemHlpLoadNullDataSelectorProt(pVCpu, &pVCpu->cpum.GstCtx.ldtr, uNewLdt);
    else
    {
        Assert(!pVCpu->cpum.GstCtx.ldtr.Attr.n.u1Present);   /* Ensures that LDT.TI check passes in iemMemFetchSelDesc() below. */

        IEMSELDESC DescNewLdt;
        rcStrict = iemMemFetchSelDesc(pVCpu, &DescNewLdt, uNewLdt, X86_XCPT_TS);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: fetching LDT failed. enmTaskSwitch=%u uNewLdt=%u cbGdt=%u rc=%Rrc\n", enmTaskSwitch,
                 uNewLdt, pVCpu->cpum.GstCtx.gdtr.cbGdt, VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }
        if (   !DescNewLdt.Legacy.Gen.u1Present
            ||  DescNewLdt.Legacy.Gen.u1DescType
            ||  DescNewLdt.Legacy.Gen.u4Type != X86_SEL_TYPE_SYS_LDT)
        {
            Log(("iemTaskSwitch: Invalid LDT. enmTaskSwitch=%u uNewLdt=%u DescNewLdt.Legacy.u=%#RX64 -> #TS\n", enmTaskSwitch,
                 uNewLdt, DescNewLdt.Legacy.u));
            return iemRaiseTaskSwitchFaultWithErr(pVCpu, uNewLdt & X86_SEL_MASK_OFF_RPL);
        }

        pVCpu->cpum.GstCtx.ldtr.ValidSel = uNewLdt;
        pVCpu->cpum.GstCtx.ldtr.fFlags   = CPUMSELREG_FLAGS_VALID;
        pVCpu->cpum.GstCtx.ldtr.u64Base  = X86DESC_BASE(&DescNewLdt.Legacy);
        pVCpu->cpum.GstCtx.ldtr.u32Limit = X86DESC_LIMIT_G(&DescNewLdt.Legacy);
        pVCpu->cpum.GstCtx.ldtr.Attr.u   = X86DESC_GET_HID_ATTR(&DescNewLdt.Legacy);
        if (IEM_IS_GUEST_CPU_INTEL(pVCpu))
            pVCpu->cpum.GstCtx.ldtr.Attr.u &= ~X86DESCATTR_UNUSABLE;
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.ldtr));
    }

    IEMSELDESC DescSS;
    if (IEM_IS_V86_MODE(pVCpu))
    {
        pVCpu->iem.s.uCpl = 3;
        iemHlpLoadSelectorInV86Mode(&pVCpu->cpum.GstCtx.es, uNewES);
        iemHlpLoadSelectorInV86Mode(&pVCpu->cpum.GstCtx.cs, uNewCS);
        iemHlpLoadSelectorInV86Mode(&pVCpu->cpum.GstCtx.ss, uNewSS);
        iemHlpLoadSelectorInV86Mode(&pVCpu->cpum.GstCtx.ds, uNewDS);
        iemHlpLoadSelectorInV86Mode(&pVCpu->cpum.GstCtx.fs, uNewFS);
        iemHlpLoadSelectorInV86Mode(&pVCpu->cpum.GstCtx.gs, uNewGS);

        /* Quick fix: fake DescSS. */ /** @todo fix the code further down? */
        DescSS.Legacy.u = 0;
        DescSS.Legacy.Gen.u16LimitLow = (uint16_t)pVCpu->cpum.GstCtx.ss.u32Limit;
        DescSS.Legacy.Gen.u4LimitHigh = pVCpu->cpum.GstCtx.ss.u32Limit >> 16;
        DescSS.Legacy.Gen.u16BaseLow  = (uint16_t)pVCpu->cpum.GstCtx.ss.u64Base;
        DescSS.Legacy.Gen.u8BaseHigh1 = (uint8_t)(pVCpu->cpum.GstCtx.ss.u64Base >> 16);
        DescSS.Legacy.Gen.u8BaseHigh2 = (uint8_t)(pVCpu->cpum.GstCtx.ss.u64Base >> 24);
        DescSS.Legacy.Gen.u4Type      = X86_SEL_TYPE_RW_ACC;
        DescSS.Legacy.Gen.u2Dpl       = 3;
    }
    else
    {
        uint8_t const uNewCpl = (uNewCS & X86_SEL_RPL);

        /*
         * Load the stack segment for the new task.
         */
        if (!(uNewSS & X86_SEL_MASK_OFF_RPL))
        {
            Log(("iemTaskSwitch: Null stack segment. enmTaskSwitch=%u uNewSS=%#x -> #TS\n", enmTaskSwitch, uNewSS));
            return iemRaiseTaskSwitchFaultWithErr(pVCpu, uNewSS & X86_SEL_MASK_OFF_RPL);
        }

        /* Fetch the descriptor. */
        rcStrict = iemMemFetchSelDesc(pVCpu, &DescSS, uNewSS, X86_XCPT_TS);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: failed to fetch SS. uNewSS=%#x rc=%Rrc\n", uNewSS,
                 VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }

        /* SS must be a data segment and writable. */
        if (    !DescSS.Legacy.Gen.u1DescType
            ||  (DescSS.Legacy.Gen.u4Type & X86_SEL_TYPE_CODE)
            || !(DescSS.Legacy.Gen.u4Type & X86_SEL_TYPE_WRITE))
        {
            Log(("iemTaskSwitch: SS invalid descriptor type. uNewSS=%#x u1DescType=%u u4Type=%#x\n",
                 uNewSS, DescSS.Legacy.Gen.u1DescType, DescSS.Legacy.Gen.u4Type));
            return iemRaiseTaskSwitchFaultWithErr(pVCpu, uNewSS & X86_SEL_MASK_OFF_RPL);
        }

        /* The SS.RPL, SS.DPL, CS.RPL (CPL) must be equal. */
        if (   (uNewSS & X86_SEL_RPL) != uNewCpl
            || DescSS.Legacy.Gen.u2Dpl != uNewCpl)
        {
            Log(("iemTaskSwitch: Invalid priv. for SS. uNewSS=%#x SS.DPL=%u uNewCpl=%u -> #TS\n", uNewSS, DescSS.Legacy.Gen.u2Dpl,
                 uNewCpl));
            return iemRaiseTaskSwitchFaultWithErr(pVCpu, uNewSS & X86_SEL_MASK_OFF_RPL);
        }

        /* Is it there? */
        if (!DescSS.Legacy.Gen.u1Present)
        {
            Log(("iemTaskSwitch: SS not present. uNewSS=%#x -> #NP\n", uNewSS));
            return iemRaiseSelectorNotPresentWithErr(pVCpu, uNewSS & X86_SEL_MASK_OFF_RPL);
        }

        uint32_t cbLimit = X86DESC_LIMIT_G(&DescSS.Legacy);
        uint64_t u64Base = X86DESC_BASE(&DescSS.Legacy);

        /* Set the accessed bit before committing the result into SS. */
        if (!(DescSS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
        {
            rcStrict = iemMemMarkSelDescAccessed(pVCpu, uNewSS);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            DescSS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
        }

        /* Commit SS. */
        pVCpu->cpum.GstCtx.ss.Sel      = uNewSS;
        pVCpu->cpum.GstCtx.ss.ValidSel = uNewSS;
        pVCpu->cpum.GstCtx.ss.Attr.u   = X86DESC_GET_HID_ATTR(&DescSS.Legacy);
        pVCpu->cpum.GstCtx.ss.u32Limit = cbLimit;
        pVCpu->cpum.GstCtx.ss.u64Base  = u64Base;
        pVCpu->cpum.GstCtx.ss.fFlags   = CPUMSELREG_FLAGS_VALID;
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.ss));

        /* CPL has changed, update IEM before loading rest of segments. */
        pVCpu->iem.s.uCpl = uNewCpl;

        /*
         * Load the data segments for the new task.
         */
        rcStrict = iemHlpTaskSwitchLoadDataSelectorInProtMode(pVCpu, &pVCpu->cpum.GstCtx.es, uNewES);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        rcStrict = iemHlpTaskSwitchLoadDataSelectorInProtMode(pVCpu, &pVCpu->cpum.GstCtx.ds, uNewDS);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        rcStrict = iemHlpTaskSwitchLoadDataSelectorInProtMode(pVCpu, &pVCpu->cpum.GstCtx.fs, uNewFS);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        rcStrict = iemHlpTaskSwitchLoadDataSelectorInProtMode(pVCpu, &pVCpu->cpum.GstCtx.gs, uNewGS);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;

        /*
         * Load the code segment for the new task.
         */
        if (!(uNewCS & X86_SEL_MASK_OFF_RPL))
        {
            Log(("iemTaskSwitch #TS: Null code segment. enmTaskSwitch=%u uNewCS=%#x\n", enmTaskSwitch, uNewCS));
            return iemRaiseTaskSwitchFaultWithErr(pVCpu, uNewCS & X86_SEL_MASK_OFF_RPL);
        }

        /* Fetch the descriptor. */
        IEMSELDESC DescCS;
        rcStrict = iemMemFetchSelDesc(pVCpu, &DescCS, uNewCS, X86_XCPT_TS);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: failed to fetch CS. uNewCS=%u rc=%Rrc\n", uNewCS, VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }

        /* CS must be a code segment. */
        if (   !DescCS.Legacy.Gen.u1DescType
            || !(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_CODE))
        {
            Log(("iemTaskSwitch: CS invalid descriptor type. uNewCS=%#x u1DescType=%u u4Type=%#x -> #TS\n", uNewCS,
                 DescCS.Legacy.Gen.u1DescType, DescCS.Legacy.Gen.u4Type));
            return iemRaiseTaskSwitchFaultWithErr(pVCpu, uNewCS & X86_SEL_MASK_OFF_RPL);
        }

        /* For conforming CS, DPL must be less than or equal to the RPL. */
        if (   (DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_CONF)
            && DescCS.Legacy.Gen.u2Dpl > (uNewCS & X86_SEL_RPL))
        {
            Log(("iemTaskSwitch: confirming CS DPL > RPL. uNewCS=%#x u4Type=%#x DPL=%u -> #TS\n", uNewCS, DescCS.Legacy.Gen.u4Type,
                 DescCS.Legacy.Gen.u2Dpl));
            return iemRaiseTaskSwitchFaultWithErr(pVCpu, uNewCS & X86_SEL_MASK_OFF_RPL);
        }

        /* For non-conforming CS, DPL must match RPL. */
        if (   !(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_CONF)
            && DescCS.Legacy.Gen.u2Dpl != (uNewCS & X86_SEL_RPL))
        {
            Log(("iemTaskSwitch: non-confirming CS DPL RPL mismatch. uNewCS=%#x u4Type=%#x DPL=%u -> #TS\n", uNewCS,
                 DescCS.Legacy.Gen.u4Type, DescCS.Legacy.Gen.u2Dpl));
            return iemRaiseTaskSwitchFaultWithErr(pVCpu, uNewCS & X86_SEL_MASK_OFF_RPL);
        }

        /* Is it there? */
        if (!DescCS.Legacy.Gen.u1Present)
        {
            Log(("iemTaskSwitch: CS not present. uNewCS=%#x -> #NP\n", uNewCS));
            return iemRaiseSelectorNotPresentWithErr(pVCpu, uNewCS & X86_SEL_MASK_OFF_RPL);
        }

        cbLimit = X86DESC_LIMIT_G(&DescCS.Legacy);
        u64Base = X86DESC_BASE(&DescCS.Legacy);

        /* Set the accessed bit before committing the result into CS. */
        if (!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
        {
            rcStrict = iemMemMarkSelDescAccessed(pVCpu, uNewCS);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            DescCS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
        }

        /* Commit CS. */
        pVCpu->cpum.GstCtx.cs.Sel      = uNewCS;
        pVCpu->cpum.GstCtx.cs.ValidSel = uNewCS;
        pVCpu->cpum.GstCtx.cs.Attr.u   = X86DESC_GET_HID_ATTR(&DescCS.Legacy);
        pVCpu->cpum.GstCtx.cs.u32Limit = cbLimit;
        pVCpu->cpum.GstCtx.cs.u64Base  = u64Base;
        pVCpu->cpum.GstCtx.cs.fFlags   = CPUMSELREG_FLAGS_VALID;
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.cs));
    }

    /** @todo Debug trap. */
    if (fIsNewTSS386 && fNewDebugTrap)
        Log(("iemTaskSwitch: Debug Trap set in new TSS. Not implemented!\n"));

    /*
     * Construct the error code masks based on what caused this task switch.
     * See Intel Instruction reference for INT.
     */
    uint16_t uExt;
    if (   enmTaskSwitch == IEMTASKSWITCH_INT_XCPT
        && (   !(fFlags & IEM_XCPT_FLAGS_T_SOFT_INT)
            ||  (fFlags & IEM_XCPT_FLAGS_ICEBP_INSTR)))
    {
        uExt = 1;
    }
    else
        uExt = 0;

    /*
     * Push any error code on to the new stack.
     */
    if (fFlags & IEM_XCPT_FLAGS_ERR)
    {
        Assert(enmTaskSwitch == IEMTASKSWITCH_INT_XCPT);
        uint32_t      cbLimitSS    = X86DESC_LIMIT_G(&DescSS.Legacy);
        uint8_t const cbStackFrame = fIsNewTSS386 ? 4 : 2;

        /* Check that there is sufficient space on the stack. */
        /** @todo Factor out segment limit checking for normal/expand down segments
         *        into a separate function. */
        if (!(DescSS.Legacy.Gen.u4Type & X86_SEL_TYPE_DOWN))
        {
            if (   pVCpu->cpum.GstCtx.esp - 1 > cbLimitSS
                || pVCpu->cpum.GstCtx.esp < cbStackFrame)
            {
                /** @todo Intel says \#SS(EXT) for INT/XCPT, I couldn't figure out AMD yet. */
                Log(("iemTaskSwitch: SS=%#x ESP=%#x cbStackFrame=%#x is out of bounds -> #SS\n",
                     pVCpu->cpum.GstCtx.ss.Sel, pVCpu->cpum.GstCtx.esp, cbStackFrame));
                return iemRaiseStackSelectorNotPresentWithErr(pVCpu, uExt);
            }
        }
        else
        {
            if (   pVCpu->cpum.GstCtx.esp - 1 > (DescSS.Legacy.Gen.u1DefBig ? UINT32_MAX : UINT32_C(0xffff))
                || pVCpu->cpum.GstCtx.esp - cbStackFrame < cbLimitSS + UINT32_C(1))
            {
                Log(("iemTaskSwitch: SS=%#x ESP=%#x cbStackFrame=%#x (expand down) is out of bounds -> #SS\n",
                     pVCpu->cpum.GstCtx.ss.Sel, pVCpu->cpum.GstCtx.esp, cbStackFrame));
                return iemRaiseStackSelectorNotPresentWithErr(pVCpu, uExt);
            }
        }


        if (fIsNewTSS386)
            rcStrict = iemMemStackPushU32(pVCpu, uErr);
        else
            rcStrict = iemMemStackPushU16(pVCpu, uErr);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: Can't push error code to new task's stack. %s-bit TSS. rc=%Rrc\n",
                 fIsNewTSS386 ? "32" : "16", VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }
    }

    /* Check the new EIP against the new CS limit. */
    if (pVCpu->cpum.GstCtx.eip > pVCpu->cpum.GstCtx.cs.u32Limit)
    {
        Log(("iemHlpTaskSwitchLoadDataSelectorInProtMode: New EIP exceeds CS limit. uNewEIP=%#RX32 CS limit=%u -> #GP(0)\n",
             pVCpu->cpum.GstCtx.eip, pVCpu->cpum.GstCtx.cs.u32Limit));
        /** @todo Intel says \#GP(EXT) for INT/XCPT, I couldn't figure out AMD yet. */
        return iemRaiseGeneralProtectionFault(pVCpu, uExt);
    }

    Log(("iemTaskSwitch: Success! New CS:EIP=%#04x:%#x SS=%#04x\n", pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.eip,
         pVCpu->cpum.GstCtx.ss.Sel));
    return fFlags & IEM_XCPT_FLAGS_T_CPU_XCPT ? VINF_IEM_RAISED_XCPT : VINF_SUCCESS;
}


/**
 * Implements exceptions and interrupts for protected mode.
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   cbInstr         The number of bytes to offset rIP by in the return
 *                          address.
 * @param   u8Vector        The interrupt / exception vector number.
 * @param   fFlags          The flags.
 * @param   uErr            The error value if IEM_XCPT_FLAGS_ERR is set.
 * @param   uCr2            The CR2 value if IEM_XCPT_FLAGS_CR2 is set.
 */
static VBOXSTRICTRC
iemRaiseXcptOrIntInProtMode(PVMCPUCC    pVCpu,
                            uint8_t     cbInstr,
                            uint8_t     u8Vector,
                            uint32_t    fFlags,
                            uint16_t    uErr,
                            uint64_t    uCr2) RT_NOEXCEPT
{
    IEM_CTX_ASSERT(pVCpu, IEM_CPUMCTX_EXTRN_XCPT_MASK);

    /*
     * Read the IDT entry.
     */
    if (pVCpu->cpum.GstCtx.idtr.cbIdt < UINT32_C(8) * u8Vector + 7)
    {
        Log(("RaiseXcptOrIntInProtMode: %#x is out of bounds (%#x)\n", u8Vector, pVCpu->cpum.GstCtx.idtr.cbIdt));
        return iemRaiseGeneralProtectionFault(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
    }
    X86DESC Idte;
    VBOXSTRICTRC rcStrict = iemMemFetchSysU64(pVCpu, &Idte.u, UINT8_MAX,
                                              pVCpu->cpum.GstCtx.idtr.pIdt + UINT32_C(8) * u8Vector);
    if (RT_UNLIKELY(rcStrict != VINF_SUCCESS))
    {
        Log(("iemRaiseXcptOrIntInProtMode: failed to fetch IDT entry! vec=%#x rc=%Rrc\n", u8Vector, VBOXSTRICTRC_VAL(rcStrict)));
        return rcStrict;
    }
    Log(("iemRaiseXcptOrIntInProtMode: vec=%#x P=%u DPL=%u DT=%u:%u A=%u %04x:%04x%04x\n",
         u8Vector, Idte.Gate.u1Present, Idte.Gate.u2Dpl, Idte.Gate.u1DescType, Idte.Gate.u4Type,
         Idte.Gate.u5ParmCount, Idte.Gate.u16Sel, Idte.Gate.u16OffsetHigh, Idte.Gate.u16OffsetLow));

    /*
     * Check the descriptor type, DPL and such.
     * ASSUMES this is done in the same order as described for call-gate calls.
     */
    if (Idte.Gate.u1DescType)
    {
        Log(("RaiseXcptOrIntInProtMode %#x - not system selector (%#x) -> #GP\n", u8Vector, Idte.Gate.u4Type));
        return iemRaiseGeneralProtectionFault(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
    }
    bool     fTaskGate   = false;
    uint8_t  f32BitGate  = true;
    uint32_t fEflToClear = X86_EFL_TF | X86_EFL_NT | X86_EFL_RF | X86_EFL_VM;
    switch (Idte.Gate.u4Type)
    {
        case X86_SEL_TYPE_SYS_UNDEFINED:
        case X86_SEL_TYPE_SYS_286_TSS_AVAIL:
        case X86_SEL_TYPE_SYS_LDT:
        case X86_SEL_TYPE_SYS_286_TSS_BUSY:
        case X86_SEL_TYPE_SYS_286_CALL_GATE:
        case X86_SEL_TYPE_SYS_UNDEFINED2:
        case X86_SEL_TYPE_SYS_386_TSS_AVAIL:
        case X86_SEL_TYPE_SYS_UNDEFINED3:
        case X86_SEL_TYPE_SYS_386_TSS_BUSY:
        case X86_SEL_TYPE_SYS_386_CALL_GATE:
        case X86_SEL_TYPE_SYS_UNDEFINED4:
        {
            /** @todo check what actually happens when the type is wrong...
             *        esp. call gates. */
            Log(("RaiseXcptOrIntInProtMode %#x - invalid type (%#x) -> #GP\n", u8Vector, Idte.Gate.u4Type));
            return iemRaiseGeneralProtectionFault(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
        }

        case X86_SEL_TYPE_SYS_286_INT_GATE:
            f32BitGate = false;
            RT_FALL_THRU();
        case X86_SEL_TYPE_SYS_386_INT_GATE:
            fEflToClear |= X86_EFL_IF;
            break;

        case X86_SEL_TYPE_SYS_TASK_GATE:
            fTaskGate = true;
#ifndef IEM_IMPLEMENTS_TASKSWITCH
            IEM_RETURN_ASPECT_NOT_IMPLEMENTED_LOG(("Task gates\n"));
#endif
            break;

        case X86_SEL_TYPE_SYS_286_TRAP_GATE:
            f32BitGate = false;
        case X86_SEL_TYPE_SYS_386_TRAP_GATE:
            break;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }

    /* Check DPL against CPL if applicable. */
    if ((fFlags & (IEM_XCPT_FLAGS_T_SOFT_INT | IEM_XCPT_FLAGS_ICEBP_INSTR)) == IEM_XCPT_FLAGS_T_SOFT_INT)
    {
        if (pVCpu->iem.s.uCpl > Idte.Gate.u2Dpl)
        {
            Log(("RaiseXcptOrIntInProtMode %#x - CPL (%d) > DPL (%d) -> #GP\n", u8Vector, pVCpu->iem.s.uCpl, Idte.Gate.u2Dpl));
            return iemRaiseGeneralProtectionFault(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
        }
    }

    /* Is it there? */
    if (!Idte.Gate.u1Present)
    {
        Log(("RaiseXcptOrIntInProtMode %#x - not present -> #NP\n", u8Vector));
        return iemRaiseSelectorNotPresentWithErr(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
    }

    /* Is it a task-gate? */
    if (fTaskGate)
    {
        /*
         * Construct the error code masks based on what caused this task switch.
         * See Intel Instruction reference for INT.
         */
        uint16_t const uExt     = (    (fFlags & IEM_XCPT_FLAGS_T_SOFT_INT)
                                   && !(fFlags & IEM_XCPT_FLAGS_ICEBP_INSTR)) ? 0 : 1;
        uint16_t const uSelMask = X86_SEL_MASK_OFF_RPL;
        RTSEL          SelTSS   = Idte.Gate.u16Sel;

        /*
         * Fetch the TSS descriptor in the GDT.
         */
        IEMSELDESC DescTSS;
        rcStrict = iemMemFetchSelDescWithErr(pVCpu, &DescTSS, SelTSS, X86_XCPT_GP, (SelTSS & uSelMask) | uExt);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("RaiseXcptOrIntInProtMode %#x - failed to fetch TSS selector %#x, rc=%Rrc\n", u8Vector, SelTSS,
                 VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }

        /* The TSS descriptor must be a system segment and be available (not busy). */
        if (   DescTSS.Legacy.Gen.u1DescType
            || (   DescTSS.Legacy.Gen.u4Type != X86_SEL_TYPE_SYS_286_TSS_AVAIL
                && DescTSS.Legacy.Gen.u4Type != X86_SEL_TYPE_SYS_386_TSS_AVAIL))
        {
            Log(("RaiseXcptOrIntInProtMode %#x - TSS selector %#x of task gate not a system descriptor or not available %#RX64\n",
                 u8Vector, SelTSS, DescTSS.Legacy.au64));
            return iemRaiseGeneralProtectionFault(pVCpu, (SelTSS & uSelMask) | uExt);
        }

        /* The TSS must be present. */
        if (!DescTSS.Legacy.Gen.u1Present)
        {
            Log(("RaiseXcptOrIntInProtMode %#x - TSS selector %#x not present %#RX64\n", u8Vector, SelTSS, DescTSS.Legacy.au64));
            return iemRaiseSelectorNotPresentWithErr(pVCpu, (SelTSS & uSelMask) | uExt);
        }

        /* Do the actual task switch. */
        return iemTaskSwitch(pVCpu, IEMTASKSWITCH_INT_XCPT,
                             (fFlags & IEM_XCPT_FLAGS_T_SOFT_INT) ? pVCpu->cpum.GstCtx.eip + cbInstr : pVCpu->cpum.GstCtx.eip,
                             fFlags, uErr, uCr2, SelTSS, &DescTSS);
    }

    /* A null CS is bad. */
    RTSEL NewCS = Idte.Gate.u16Sel;
    if (!(NewCS & X86_SEL_MASK_OFF_RPL))
    {
        Log(("RaiseXcptOrIntInProtMode %#x - CS=%#x -> #GP\n", u8Vector, NewCS));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /* Fetch the descriptor for the new CS. */
    IEMSELDESC DescCS;
    rcStrict = iemMemFetchSelDesc(pVCpu, &DescCS, NewCS, X86_XCPT_GP); /** @todo correct exception? */
    if (rcStrict != VINF_SUCCESS)
    {
        Log(("RaiseXcptOrIntInProtMode %#x - CS=%#x - rc=%Rrc\n", u8Vector, NewCS, VBOXSTRICTRC_VAL(rcStrict)));
        return rcStrict;
    }

    /* Must be a code segment. */
    if (!DescCS.Legacy.Gen.u1DescType)
    {
        Log(("RaiseXcptOrIntInProtMode %#x - CS=%#x - system selector (%#x) -> #GP\n", u8Vector, NewCS, DescCS.Legacy.Gen.u4Type));
        return iemRaiseGeneralProtectionFault(pVCpu, NewCS & X86_SEL_MASK_OFF_RPL);
    }
    if (!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_CODE))
    {
        Log(("RaiseXcptOrIntInProtMode %#x - CS=%#x - data selector (%#x) -> #GP\n", u8Vector, NewCS, DescCS.Legacy.Gen.u4Type));
        return iemRaiseGeneralProtectionFault(pVCpu, NewCS & X86_SEL_MASK_OFF_RPL);
    }

    /* Don't allow lowering the privilege level. */
    /** @todo Does the lowering of privileges apply to software interrupts
     *        only?  This has bearings on the more-privileged or
     *        same-privilege stack behavior further down.  A testcase would
     *        be nice. */
    if (DescCS.Legacy.Gen.u2Dpl > pVCpu->iem.s.uCpl)
    {
        Log(("RaiseXcptOrIntInProtMode %#x - CS=%#x - DPL (%d) > CPL (%d) -> #GP\n",
             u8Vector, NewCS, DescCS.Legacy.Gen.u2Dpl, pVCpu->iem.s.uCpl));
        return iemRaiseGeneralProtectionFault(pVCpu, NewCS & X86_SEL_MASK_OFF_RPL);
    }

    /* Make sure the selector is present. */
    if (!DescCS.Legacy.Gen.u1Present)
    {
        Log(("RaiseXcptOrIntInProtMode %#x - CS=%#x - segment not present -> #NP\n", u8Vector, NewCS));
        return iemRaiseSelectorNotPresentBySelector(pVCpu, NewCS);
    }

    /* Check the new EIP against the new CS limit. */
    uint32_t const uNewEip =    Idte.Gate.u4Type == X86_SEL_TYPE_SYS_286_INT_GATE
                             || Idte.Gate.u4Type == X86_SEL_TYPE_SYS_286_TRAP_GATE
                           ? Idte.Gate.u16OffsetLow
                           : Idte.Gate.u16OffsetLow | ((uint32_t)Idte.Gate.u16OffsetHigh << 16);
    uint32_t cbLimitCS = X86DESC_LIMIT_G(&DescCS.Legacy);
    if (uNewEip > cbLimitCS)
    {
        Log(("RaiseXcptOrIntInProtMode %#x - EIP=%#x > cbLimitCS=%#x (CS=%#x) -> #GP(0)\n",
             u8Vector, uNewEip, cbLimitCS, NewCS));
        return iemRaiseGeneralProtectionFault(pVCpu, 0);
    }
    Log7(("iemRaiseXcptOrIntInProtMode: new EIP=%#x CS=%#x\n", uNewEip, NewCS));

    /* Calc the flag image to push. */
    uint32_t        fEfl    = IEMMISC_GET_EFL(pVCpu);
    if (fFlags & (IEM_XCPT_FLAGS_DRx_INSTR_BP | IEM_XCPT_FLAGS_T_SOFT_INT))
        fEfl &= ~X86_EFL_RF;
    else
        fEfl |= X86_EFL_RF; /* Vagueness is all I've found on this so far... */ /** @todo Automatically pushing EFLAGS.RF. */

    /* From V8086 mode only go to CPL 0. */
    uint8_t const   uNewCpl = DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_CONF
                            ? pVCpu->iem.s.uCpl : DescCS.Legacy.Gen.u2Dpl;
    if ((fEfl & X86_EFL_VM) && uNewCpl != 0) /** @todo When exactly is this raised? */
    {
        Log(("RaiseXcptOrIntInProtMode %#x - CS=%#x - New CPL (%d) != 0 w/ VM=1 -> #GP\n", u8Vector, NewCS, uNewCpl));
        return iemRaiseGeneralProtectionFault(pVCpu, 0);
    }

    /*
     * If the privilege level changes, we need to get a new stack from the TSS.
     * This in turns means validating the new SS and ESP...
     */
    if (uNewCpl != pVCpu->iem.s.uCpl)
    {
        RTSEL    NewSS;
        uint32_t uNewEsp;
        rcStrict = iemRaiseLoadStackFromTss32Or16(pVCpu, uNewCpl, &NewSS, &uNewEsp);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;

        IEMSELDESC DescSS;
        rcStrict = iemMiscValidateNewSS(pVCpu, NewSS, uNewCpl, &DescSS);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        /* If the new SS is 16-bit, we are only going to use SP, not ESP. */
        if (!DescSS.Legacy.Gen.u1DefBig)
        {
            Log(("iemRaiseXcptOrIntInProtMode: Forcing ESP=%#x to 16 bits\n", uNewEsp));
            uNewEsp = (uint16_t)uNewEsp;
        }

        Log7(("iemRaiseXcptOrIntInProtMode: New SS=%#x ESP=%#x (from TSS); current SS=%#x ESP=%#x\n", NewSS, uNewEsp, pVCpu->cpum.GstCtx.ss.Sel, pVCpu->cpum.GstCtx.esp));

        /* Check that there is sufficient space for the stack frame. */
        uint32_t cbLimitSS = X86DESC_LIMIT_G(&DescSS.Legacy);
        uint8_t const cbStackFrame = !(fEfl & X86_EFL_VM)
                                   ? (fFlags & IEM_XCPT_FLAGS_ERR ? 12 : 10) << f32BitGate
                                   : (fFlags & IEM_XCPT_FLAGS_ERR ? 20 : 18) << f32BitGate;

        if (!(DescSS.Legacy.Gen.u4Type & X86_SEL_TYPE_DOWN))
        {
            if (   uNewEsp - 1 > cbLimitSS
                || uNewEsp < cbStackFrame)
            {
                Log(("RaiseXcptOrIntInProtMode: %#x - SS=%#x ESP=%#x cbStackFrame=%#x is out of bounds -> #GP\n",
                     u8Vector, NewSS, uNewEsp, cbStackFrame));
                return iemRaiseSelectorBoundsBySelector(pVCpu, NewSS);
            }
        }
        else
        {
            if (   uNewEsp - 1 > (DescSS.Legacy.Gen.u1DefBig ? UINT32_MAX : UINT16_MAX)
                || uNewEsp - cbStackFrame < cbLimitSS + UINT32_C(1))
            {
                Log(("RaiseXcptOrIntInProtMode: %#x - SS=%#x ESP=%#x cbStackFrame=%#x (expand down) is out of bounds -> #GP\n",
                     u8Vector, NewSS, uNewEsp, cbStackFrame));
                return iemRaiseSelectorBoundsBySelector(pVCpu, NewSS);
            }
        }

        /*
         * Start making changes.
         */

        /* Set the new CPL so that stack accesses use it. */
        uint8_t const uOldCpl = pVCpu->iem.s.uCpl;
        pVCpu->iem.s.uCpl = uNewCpl;

        /* Create the stack frame. */
        RTPTRUNION uStackFrame;
        rcStrict = iemMemMap(pVCpu, &uStackFrame.pv, cbStackFrame, UINT8_MAX,
                             uNewEsp - cbStackFrame + X86DESC_BASE(&DescSS.Legacy),
                             IEM_ACCESS_STACK_W | IEM_ACCESS_WHAT_SYS, 0); /* _SYS is a hack ... */
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        void * const pvStackFrame = uStackFrame.pv;
        if (f32BitGate)
        {
            if (fFlags & IEM_XCPT_FLAGS_ERR)
                *uStackFrame.pu32++ = uErr;
            uStackFrame.pu32[0] = (fFlags & IEM_XCPT_FLAGS_T_SOFT_INT) ? pVCpu->cpum.GstCtx.eip + cbInstr : pVCpu->cpum.GstCtx.eip;
            uStackFrame.pu32[1] = (pVCpu->cpum.GstCtx.cs.Sel & ~X86_SEL_RPL) | uOldCpl;
            uStackFrame.pu32[2] = fEfl;
            uStackFrame.pu32[3] = pVCpu->cpum.GstCtx.esp;
            uStackFrame.pu32[4] = pVCpu->cpum.GstCtx.ss.Sel;
            Log7(("iemRaiseXcptOrIntInProtMode: 32-bit push SS=%#x ESP=%#x\n", pVCpu->cpum.GstCtx.ss.Sel, pVCpu->cpum.GstCtx.esp));
            if (fEfl & X86_EFL_VM)
            {
                uStackFrame.pu32[1] = pVCpu->cpum.GstCtx.cs.Sel;
                uStackFrame.pu32[5] = pVCpu->cpum.GstCtx.es.Sel;
                uStackFrame.pu32[6] = pVCpu->cpum.GstCtx.ds.Sel;
                uStackFrame.pu32[7] = pVCpu->cpum.GstCtx.fs.Sel;
                uStackFrame.pu32[8] = pVCpu->cpum.GstCtx.gs.Sel;
            }
        }
        else
        {
            if (fFlags & IEM_XCPT_FLAGS_ERR)
                *uStackFrame.pu16++ = uErr;
            uStackFrame.pu16[0] = (fFlags & IEM_XCPT_FLAGS_T_SOFT_INT) ? pVCpu->cpum.GstCtx.ip + cbInstr : pVCpu->cpum.GstCtx.ip;
            uStackFrame.pu16[1] = (pVCpu->cpum.GstCtx.cs.Sel & ~X86_SEL_RPL) | uOldCpl;
            uStackFrame.pu16[2] = fEfl;
            uStackFrame.pu16[3] = pVCpu->cpum.GstCtx.sp;
            uStackFrame.pu16[4] = pVCpu->cpum.GstCtx.ss.Sel;
            Log7(("iemRaiseXcptOrIntInProtMode: 16-bit push SS=%#x SP=%#x\n", pVCpu->cpum.GstCtx.ss.Sel, pVCpu->cpum.GstCtx.sp));
            if (fEfl & X86_EFL_VM)
            {
                uStackFrame.pu16[1] = pVCpu->cpum.GstCtx.cs.Sel;
                uStackFrame.pu16[5] = pVCpu->cpum.GstCtx.es.Sel;
                uStackFrame.pu16[6] = pVCpu->cpum.GstCtx.ds.Sel;
                uStackFrame.pu16[7] = pVCpu->cpum.GstCtx.fs.Sel;
                uStackFrame.pu16[8] = pVCpu->cpum.GstCtx.gs.Sel;
            }
        }
        rcStrict = iemMemCommitAndUnmap(pVCpu, pvStackFrame, IEM_ACCESS_STACK_W | IEM_ACCESS_WHAT_SYS);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;

        /* Mark the selectors 'accessed' (hope this is the correct time). */
        /** @todo testcase: excatly _when_ are the accessed bits set - before or
         *        after pushing the stack frame? (Write protect the gdt + stack to
         *        find out.) */
        if (!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
        {
            rcStrict = iemMemMarkSelDescAccessed(pVCpu, NewCS);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            DescCS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
        }

        if (!(DescSS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
        {
            rcStrict = iemMemMarkSelDescAccessed(pVCpu, NewSS);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            DescSS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
        }

        /*
         * Start comitting the register changes (joins with the DPL=CPL branch).
         */
        pVCpu->cpum.GstCtx.ss.Sel            = NewSS;
        pVCpu->cpum.GstCtx.ss.ValidSel       = NewSS;
        pVCpu->cpum.GstCtx.ss.fFlags         = CPUMSELREG_FLAGS_VALID;
        pVCpu->cpum.GstCtx.ss.u32Limit       = cbLimitSS;
        pVCpu->cpum.GstCtx.ss.u64Base        = X86DESC_BASE(&DescSS.Legacy);
        pVCpu->cpum.GstCtx.ss.Attr.u         = X86DESC_GET_HID_ATTR(&DescSS.Legacy);
        /** @todo When coming from 32-bit code and operating with a 16-bit TSS and
         *        16-bit handler, the high word of ESP remains unchanged (i.e. only
         *        SP is loaded).
         *  Need to check the other combinations too:
         *      - 16-bit TSS, 32-bit handler
         *      - 32-bit TSS, 16-bit handler */
        if (!pVCpu->cpum.GstCtx.ss.Attr.n.u1DefBig)
            pVCpu->cpum.GstCtx.sp            = (uint16_t)(uNewEsp - cbStackFrame);
        else
            pVCpu->cpum.GstCtx.rsp           = uNewEsp - cbStackFrame;

        if (fEfl & X86_EFL_VM)
        {
            iemHlpLoadNullDataSelectorOnV86Xcpt(pVCpu, &pVCpu->cpum.GstCtx.gs);
            iemHlpLoadNullDataSelectorOnV86Xcpt(pVCpu, &pVCpu->cpum.GstCtx.fs);
            iemHlpLoadNullDataSelectorOnV86Xcpt(pVCpu, &pVCpu->cpum.GstCtx.es);
            iemHlpLoadNullDataSelectorOnV86Xcpt(pVCpu, &pVCpu->cpum.GstCtx.ds);
        }
    }
    /*
     * Same privilege, no stack change and smaller stack frame.
     */
    else
    {
        uint64_t        uNewRsp;
        RTPTRUNION      uStackFrame;
        uint8_t const   cbStackFrame = (fFlags & IEM_XCPT_FLAGS_ERR ? 8 : 6) << f32BitGate;
        rcStrict = iemMemStackPushBeginSpecial(pVCpu, cbStackFrame, f32BitGate ? 3 : 1, &uStackFrame.pv, &uNewRsp);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        void * const pvStackFrame = uStackFrame.pv;

        if (f32BitGate)
        {
            if (fFlags & IEM_XCPT_FLAGS_ERR)
                *uStackFrame.pu32++ = uErr;
            uStackFrame.pu32[0] = fFlags & IEM_XCPT_FLAGS_T_SOFT_INT ? pVCpu->cpum.GstCtx.eip + cbInstr : pVCpu->cpum.GstCtx.eip;
            uStackFrame.pu32[1] = (pVCpu->cpum.GstCtx.cs.Sel & ~X86_SEL_RPL) | pVCpu->iem.s.uCpl;
            uStackFrame.pu32[2] = fEfl;
        }
        else
        {
            if (fFlags & IEM_XCPT_FLAGS_ERR)
                *uStackFrame.pu16++ = uErr;
            uStackFrame.pu16[0] = fFlags & IEM_XCPT_FLAGS_T_SOFT_INT ? pVCpu->cpum.GstCtx.eip + cbInstr : pVCpu->cpum.GstCtx.eip;
            uStackFrame.pu16[1] = (pVCpu->cpum.GstCtx.cs.Sel & ~X86_SEL_RPL) | pVCpu->iem.s.uCpl;
            uStackFrame.pu16[2] = fEfl;
        }
        rcStrict = iemMemCommitAndUnmap(pVCpu, pvStackFrame, IEM_ACCESS_STACK_W); /* don't use the commit here */
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;

        /* Mark the CS selector as 'accessed'. */
        if (!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
        {
            rcStrict = iemMemMarkSelDescAccessed(pVCpu, NewCS);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            DescCS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
        }

        /*
         * Start committing the register changes (joins with the other branch).
         */
        pVCpu->cpum.GstCtx.rsp = uNewRsp;
    }

    /* ... register committing continues. */
    pVCpu->cpum.GstCtx.cs.Sel            = (NewCS & ~X86_SEL_RPL) | uNewCpl;
    pVCpu->cpum.GstCtx.cs.ValidSel       = (NewCS & ~X86_SEL_RPL) | uNewCpl;
    pVCpu->cpum.GstCtx.cs.fFlags         = CPUMSELREG_FLAGS_VALID;
    pVCpu->cpum.GstCtx.cs.u32Limit       = cbLimitCS;
    pVCpu->cpum.GstCtx.cs.u64Base        = X86DESC_BASE(&DescCS.Legacy);
    pVCpu->cpum.GstCtx.cs.Attr.u         = X86DESC_GET_HID_ATTR(&DescCS.Legacy);

    pVCpu->cpum.GstCtx.rip               = uNewEip;  /* (The entire register is modified, see pe16_32 bs3kit tests.) */
    fEfl &= ~fEflToClear;
    IEMMISC_SET_EFL(pVCpu, fEfl);

    if (fFlags & IEM_XCPT_FLAGS_CR2)
        pVCpu->cpum.GstCtx.cr2 = uCr2;

    if (fFlags & IEM_XCPT_FLAGS_T_CPU_XCPT)
        iemRaiseXcptAdjustState(pVCpu, u8Vector);

    return fFlags & IEM_XCPT_FLAGS_T_CPU_XCPT ? VINF_IEM_RAISED_XCPT : VINF_SUCCESS;
}


/**
 * Implements exceptions and interrupts for long mode.
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   cbInstr         The number of bytes to offset rIP by in the return
 *                          address.
 * @param   u8Vector        The interrupt / exception vector number.
 * @param   fFlags          The flags.
 * @param   uErr            The error value if IEM_XCPT_FLAGS_ERR is set.
 * @param   uCr2            The CR2 value if IEM_XCPT_FLAGS_CR2 is set.
 */
static VBOXSTRICTRC
iemRaiseXcptOrIntInLongMode(PVMCPUCC    pVCpu,
                            uint8_t     cbInstr,
                            uint8_t     u8Vector,
                            uint32_t    fFlags,
                            uint16_t    uErr,
                            uint64_t    uCr2) RT_NOEXCEPT
{
    IEM_CTX_ASSERT(pVCpu, IEM_CPUMCTX_EXTRN_XCPT_MASK);

    /*
     * Read the IDT entry.
     */
    uint16_t offIdt = (uint16_t)u8Vector << 4;
    if (pVCpu->cpum.GstCtx.idtr.cbIdt < offIdt + 7)
    {
        Log(("iemRaiseXcptOrIntInLongMode: %#x is out of bounds (%#x)\n", u8Vector, pVCpu->cpum.GstCtx.idtr.cbIdt));
        return iemRaiseGeneralProtectionFault(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
    }
    X86DESC64 Idte;
#ifdef _MSC_VER /* Shut up silly compiler warning. */
    Idte.au64[0] = 0;
    Idte.au64[1] = 0;
#endif
    VBOXSTRICTRC rcStrict = iemMemFetchSysU64(pVCpu, &Idte.au64[0], UINT8_MAX, pVCpu->cpum.GstCtx.idtr.pIdt + offIdt);
    if (RT_LIKELY(rcStrict == VINF_SUCCESS))
        rcStrict = iemMemFetchSysU64(pVCpu, &Idte.au64[1], UINT8_MAX, pVCpu->cpum.GstCtx.idtr.pIdt + offIdt + 8);
    if (RT_UNLIKELY(rcStrict != VINF_SUCCESS))
    {
        Log(("iemRaiseXcptOrIntInLongMode: failed to fetch IDT entry! vec=%#x rc=%Rrc\n", u8Vector, VBOXSTRICTRC_VAL(rcStrict)));
        return rcStrict;
    }
    Log(("iemRaiseXcptOrIntInLongMode: vec=%#x P=%u DPL=%u DT=%u:%u IST=%u %04x:%08x%04x%04x\n",
         u8Vector, Idte.Gate.u1Present, Idte.Gate.u2Dpl, Idte.Gate.u1DescType, Idte.Gate.u4Type,
         Idte.Gate.u3IST, Idte.Gate.u16Sel, Idte.Gate.u32OffsetTop, Idte.Gate.u16OffsetHigh, Idte.Gate.u16OffsetLow));

    /*
     * Check the descriptor type, DPL and such.
     * ASSUMES this is done in the same order as described for call-gate calls.
     */
    if (Idte.Gate.u1DescType)
    {
        Log(("iemRaiseXcptOrIntInLongMode %#x - not system selector (%#x) -> #GP\n", u8Vector, Idte.Gate.u4Type));
        return iemRaiseGeneralProtectionFault(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
    }
    uint32_t fEflToClear = X86_EFL_TF | X86_EFL_NT | X86_EFL_RF | X86_EFL_VM;
    switch (Idte.Gate.u4Type)
    {
        case AMD64_SEL_TYPE_SYS_INT_GATE:
            fEflToClear |= X86_EFL_IF;
            break;
        case AMD64_SEL_TYPE_SYS_TRAP_GATE:
            break;

        default:
            Log(("iemRaiseXcptOrIntInLongMode %#x - invalid type (%#x) -> #GP\n", u8Vector, Idte.Gate.u4Type));
            return iemRaiseGeneralProtectionFault(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
    }

    /* Check DPL against CPL if applicable. */
    if ((fFlags & (IEM_XCPT_FLAGS_T_SOFT_INT | IEM_XCPT_FLAGS_ICEBP_INSTR)) == IEM_XCPT_FLAGS_T_SOFT_INT)
    {
        if (pVCpu->iem.s.uCpl > Idte.Gate.u2Dpl)
        {
            Log(("iemRaiseXcptOrIntInLongMode %#x - CPL (%d) > DPL (%d) -> #GP\n", u8Vector, pVCpu->iem.s.uCpl, Idte.Gate.u2Dpl));
            return iemRaiseGeneralProtectionFault(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
        }
    }

    /* Is it there? */
    if (!Idte.Gate.u1Present)
    {
        Log(("iemRaiseXcptOrIntInLongMode %#x - not present -> #NP\n", u8Vector));
        return iemRaiseSelectorNotPresentWithErr(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
    }

    /* A null CS is bad. */
    RTSEL NewCS = Idte.Gate.u16Sel;
    if (!(NewCS & X86_SEL_MASK_OFF_RPL))
    {
        Log(("iemRaiseXcptOrIntInLongMode %#x - CS=%#x -> #GP\n", u8Vector, NewCS));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /* Fetch the descriptor for the new CS. */
    IEMSELDESC DescCS;
    rcStrict = iemMemFetchSelDesc(pVCpu, &DescCS, NewCS, X86_XCPT_GP);
    if (rcStrict != VINF_SUCCESS)
    {
        Log(("iemRaiseXcptOrIntInLongMode %#x - CS=%#x - rc=%Rrc\n", u8Vector, NewCS, VBOXSTRICTRC_VAL(rcStrict)));
        return rcStrict;
    }

    /* Must be a 64-bit code segment. */
    if (!DescCS.Long.Gen.u1DescType)
    {
        Log(("iemRaiseXcptOrIntInLongMode %#x - CS=%#x - system selector (%#x) -> #GP\n", u8Vector, NewCS, DescCS.Legacy.Gen.u4Type));
        return iemRaiseGeneralProtectionFault(pVCpu, NewCS & X86_SEL_MASK_OFF_RPL);
    }
    if (   !DescCS.Long.Gen.u1Long
        || DescCS.Long.Gen.u1DefBig
        || !(DescCS.Long.Gen.u4Type & X86_SEL_TYPE_CODE) )
    {
        Log(("iemRaiseXcptOrIntInLongMode %#x - CS=%#x - not 64-bit code selector (%#x, L=%u, D=%u) -> #GP\n",
             u8Vector, NewCS, DescCS.Legacy.Gen.u4Type, DescCS.Long.Gen.u1Long, DescCS.Long.Gen.u1DefBig));
        return iemRaiseGeneralProtectionFault(pVCpu, NewCS & X86_SEL_MASK_OFF_RPL);
    }

    /* Don't allow lowering the privilege level.  For non-conforming CS
       selectors, the CS.DPL sets the privilege level the trap/interrupt
       handler runs at.  For conforming CS selectors, the CPL remains
       unchanged, but the CS.DPL must be <= CPL. */
    /** @todo Testcase: Interrupt handler with CS.DPL=1, interrupt dispatched
     *        when CPU in Ring-0. Result \#GP?  */
    if (DescCS.Legacy.Gen.u2Dpl > pVCpu->iem.s.uCpl)
    {
        Log(("iemRaiseXcptOrIntInLongMode %#x - CS=%#x - DPL (%d) > CPL (%d) -> #GP\n",
             u8Vector, NewCS, DescCS.Legacy.Gen.u2Dpl, pVCpu->iem.s.uCpl));
        return iemRaiseGeneralProtectionFault(pVCpu, NewCS & X86_SEL_MASK_OFF_RPL);
    }


    /* Make sure the selector is present. */
    if (!DescCS.Legacy.Gen.u1Present)
    {
        Log(("iemRaiseXcptOrIntInLongMode %#x - CS=%#x - segment not present -> #NP\n", u8Vector, NewCS));
        return iemRaiseSelectorNotPresentBySelector(pVCpu, NewCS);
    }

    /* Check that the new RIP is canonical. */
    uint64_t const uNewRip = Idte.Gate.u16OffsetLow
                           | ((uint32_t)Idte.Gate.u16OffsetHigh << 16)
                           | ((uint64_t)Idte.Gate.u32OffsetTop  << 32);
    if (!IEM_IS_CANONICAL(uNewRip))
    {
        Log(("iemRaiseXcptOrIntInLongMode %#x - RIP=%#RX64 - Not canonical -> #GP(0)\n", u8Vector, uNewRip));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /*
     * If the privilege level changes or if the IST isn't zero, we need to get
     * a new stack from the TSS.
     */
    uint64_t        uNewRsp;
    uint8_t const   uNewCpl = DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_CONF
                            ? pVCpu->iem.s.uCpl : DescCS.Legacy.Gen.u2Dpl;
    if (   uNewCpl != pVCpu->iem.s.uCpl
        || Idte.Gate.u3IST != 0)
    {
        rcStrict = iemRaiseLoadStackFromTss64(pVCpu, uNewCpl, Idte.Gate.u3IST, &uNewRsp);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
    }
    else
        uNewRsp = pVCpu->cpum.GstCtx.rsp;
    uNewRsp &= ~(uint64_t)0xf;

    /*
     * Calc the flag image to push.
     */
    uint32_t        fEfl    = IEMMISC_GET_EFL(pVCpu);
    if (fFlags & (IEM_XCPT_FLAGS_DRx_INSTR_BP | IEM_XCPT_FLAGS_T_SOFT_INT))
        fEfl &= ~X86_EFL_RF;
    else
        fEfl |= X86_EFL_RF; /* Vagueness is all I've found on this so far... */ /** @todo Automatically pushing EFLAGS.RF. */

    /*
     * Start making changes.
     */
    /* Set the new CPL so that stack accesses use it. */
    uint8_t const uOldCpl = pVCpu->iem.s.uCpl;
    pVCpu->iem.s.uCpl = uNewCpl;

    /* Create the stack frame. */
    uint32_t   cbStackFrame = sizeof(uint64_t) * (5 + !!(fFlags & IEM_XCPT_FLAGS_ERR));
    RTPTRUNION uStackFrame;
    rcStrict = iemMemMap(pVCpu, &uStackFrame.pv, cbStackFrame, UINT8_MAX,
                         uNewRsp - cbStackFrame, IEM_ACCESS_STACK_W | IEM_ACCESS_WHAT_SYS, 0); /* _SYS is a hack ... */
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;
    void * const pvStackFrame = uStackFrame.pv;

    if (fFlags & IEM_XCPT_FLAGS_ERR)
        *uStackFrame.pu64++ = uErr;
    uStackFrame.pu64[0] = fFlags & IEM_XCPT_FLAGS_T_SOFT_INT ? pVCpu->cpum.GstCtx.rip + cbInstr : pVCpu->cpum.GstCtx.rip;
    uStackFrame.pu64[1] = (pVCpu->cpum.GstCtx.cs.Sel & ~X86_SEL_RPL) | uOldCpl; /* CPL paranoia */
    uStackFrame.pu64[2] = fEfl;
    uStackFrame.pu64[3] = pVCpu->cpum.GstCtx.rsp;
    uStackFrame.pu64[4] = pVCpu->cpum.GstCtx.ss.Sel;
    rcStrict = iemMemCommitAndUnmap(pVCpu, pvStackFrame, IEM_ACCESS_STACK_W | IEM_ACCESS_WHAT_SYS);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /* Mark the CS selectors 'accessed' (hope this is the correct time). */
    /** @todo testcase: excatly _when_ are the accessed bits set - before or
     *        after pushing the stack frame? (Write protect the gdt + stack to
     *        find out.) */
    if (!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
    {
        rcStrict = iemMemMarkSelDescAccessed(pVCpu, NewCS);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        DescCS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
    }

    /*
     * Start comitting the register changes.
     */
    /** @todo research/testcase: Figure out what VT-x and AMD-V loads into the
     *        hidden registers when interrupting 32-bit or 16-bit code! */
    if (uNewCpl != uOldCpl)
    {
        pVCpu->cpum.GstCtx.ss.Sel        = 0 | uNewCpl;
        pVCpu->cpum.GstCtx.ss.ValidSel   = 0 | uNewCpl;
        pVCpu->cpum.GstCtx.ss.fFlags     = CPUMSELREG_FLAGS_VALID;
        pVCpu->cpum.GstCtx.ss.u32Limit   = UINT32_MAX;
        pVCpu->cpum.GstCtx.ss.u64Base    = 0;
        pVCpu->cpum.GstCtx.ss.Attr.u     = (uNewCpl << X86DESCATTR_DPL_SHIFT) | X86DESCATTR_UNUSABLE;
    }
    pVCpu->cpum.GstCtx.rsp           = uNewRsp - cbStackFrame;
    pVCpu->cpum.GstCtx.cs.Sel        = (NewCS & ~X86_SEL_RPL) | uNewCpl;
    pVCpu->cpum.GstCtx.cs.ValidSel   = (NewCS & ~X86_SEL_RPL) | uNewCpl;
    pVCpu->cpum.GstCtx.cs.fFlags     = CPUMSELREG_FLAGS_VALID;
    pVCpu->cpum.GstCtx.cs.u32Limit   = X86DESC_LIMIT_G(&DescCS.Legacy);
    pVCpu->cpum.GstCtx.cs.u64Base    = X86DESC_BASE(&DescCS.Legacy);
    pVCpu->cpum.GstCtx.cs.Attr.u     = X86DESC_GET_HID_ATTR(&DescCS.Legacy);
    pVCpu->cpum.GstCtx.rip           = uNewRip;

    fEfl &= ~fEflToClear;
    IEMMISC_SET_EFL(pVCpu, fEfl);

    if (fFlags & IEM_XCPT_FLAGS_CR2)
        pVCpu->cpum.GstCtx.cr2 = uCr2;

    if (fFlags & IEM_XCPT_FLAGS_T_CPU_XCPT)
        iemRaiseXcptAdjustState(pVCpu, u8Vector);

    return fFlags & IEM_XCPT_FLAGS_T_CPU_XCPT ? VINF_IEM_RAISED_XCPT : VINF_SUCCESS;
}


/**
 * Implements exceptions and interrupts.
 *
 * All exceptions and interrupts goes thru this function!
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   cbInstr         The number of bytes to offset rIP by in the return
 *                          address.
 * @param   u8Vector        The interrupt / exception vector number.
 * @param   fFlags          The flags.
 * @param   uErr            The error value if IEM_XCPT_FLAGS_ERR is set.
 * @param   uCr2            The CR2 value if IEM_XCPT_FLAGS_CR2 is set.
 */
VBOXSTRICTRC
iemRaiseXcptOrInt(PVMCPUCC    pVCpu,
                  uint8_t     cbInstr,
                  uint8_t     u8Vector,
                  uint32_t    fFlags,
                  uint16_t    uErr,
                  uint64_t    uCr2) RT_NOEXCEPT
{
    /*
     * Get all the state that we might need here.
     */
    IEM_CTX_IMPORT_RET(pVCpu, IEM_CPUMCTX_EXTRN_XCPT_MASK);
    IEM_CTX_ASSERT(pVCpu, IEM_CPUMCTX_EXTRN_XCPT_MASK);

#ifndef IEM_WITH_CODE_TLB /** @todo we're doing it afterwards too, that should suffice... */
    /*
     * Flush prefetch buffer
     */
    pVCpu->iem.s.cbOpcode = pVCpu->iem.s.offOpcode;
#endif

    /*
     * Perform the V8086 IOPL check and upgrade the fault without nesting.
     */
    if (   pVCpu->cpum.GstCtx.eflags.Bits.u1VM
        && pVCpu->cpum.GstCtx.eflags.Bits.u2IOPL != 3
        && (fFlags & (  IEM_XCPT_FLAGS_T_SOFT_INT
                      | IEM_XCPT_FLAGS_BP_INSTR
                      | IEM_XCPT_FLAGS_ICEBP_INSTR
                      | IEM_XCPT_FLAGS_OF_INSTR)) == IEM_XCPT_FLAGS_T_SOFT_INT
        && (pVCpu->cpum.GstCtx.cr0 & X86_CR0_PE) )
    {
        Log(("iemRaiseXcptOrInt: V8086 IOPL check failed for int %#x -> #GP(0)\n", u8Vector));
        fFlags   = IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR;
        u8Vector = X86_XCPT_GP;
        uErr     = 0;
    }
#ifdef DBGFTRACE_ENABLED
    RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "Xcpt/%u: %02x %u %x %x %llx %04x:%04llx %04x:%04llx",
                      pVCpu->iem.s.cXcptRecursions, u8Vector, cbInstr, fFlags, uErr, uCr2,
                      pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, pVCpu->cpum.GstCtx.ss.Sel, pVCpu->cpum.GstCtx.rsp);
#endif

    /*
     * Evaluate whether NMI blocking should be in effect.
     * Normally, NMI blocking is in effect whenever we inject an NMI.
     */
    bool fBlockNmi = u8Vector == X86_XCPT_NMI
                  && (fFlags & IEM_XCPT_FLAGS_T_CPU_XCPT);

#ifdef VBOX_WITH_NESTED_HWVIRT_VMX
    if (IEM_VMX_IS_NON_ROOT_MODE(pVCpu))
    {
        VBOXSTRICTRC rcStrict0 = iemVmxVmexitEvent(pVCpu, u8Vector, fFlags, uErr, uCr2, cbInstr);
        if (rcStrict0 != VINF_VMX_INTERCEPT_NOT_ACTIVE)
            return rcStrict0;

        /* If virtual-NMI blocking is in effect for the nested-guest, guest NMIs are not blocked. */
        if (pVCpu->cpum.GstCtx.hwvirt.vmx.fVirtNmiBlocking)
        {
            Assert(CPUMIsGuestVmxPinCtlsSet(&pVCpu->cpum.GstCtx, VMX_PIN_CTLS_VIRT_NMI));
            fBlockNmi = false;
        }
    }
#endif

#ifdef VBOX_WITH_NESTED_HWVIRT_SVM
    if (CPUMIsGuestInSvmNestedHwVirtMode(IEM_GET_CTX(pVCpu)))
    {
        /*
         * If the event is being injected as part of VMRUN, it isn't subject to event
         * intercepts in the nested-guest. However, secondary exceptions that occur
         * during injection of any event -are- subject to exception intercepts.
         *
         * See AMD spec. 15.20 "Event Injection".
         */
        if (!pVCpu->cpum.GstCtx.hwvirt.svm.fInterceptEvents)
            pVCpu->cpum.GstCtx.hwvirt.svm.fInterceptEvents = true;
        else
        {
            /*
             * Check and handle if the event being raised is intercepted.
             */
            VBOXSTRICTRC rcStrict0 = iemHandleSvmEventIntercept(pVCpu, u8Vector, fFlags, uErr, uCr2);
            if (rcStrict0 != VINF_SVM_INTERCEPT_NOT_ACTIVE)
                return rcStrict0;
        }
    }
#endif

    /*
     * Set NMI blocking if necessary.
     */
    if (fBlockNmi)
        CPUMSetInterruptInhibitingByNmi(&pVCpu->cpum.GstCtx);

    /*
     * Do recursion accounting.
     */
    uint8_t const  uPrevXcpt = pVCpu->iem.s.uCurXcpt;
    uint32_t const fPrevXcpt = pVCpu->iem.s.fCurXcpt;
    if (pVCpu->iem.s.cXcptRecursions == 0)
        Log(("iemRaiseXcptOrInt: %#x at %04x:%RGv cbInstr=%#x fFlags=%#x uErr=%#x uCr2=%llx\n",
             u8Vector, pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, cbInstr, fFlags, uErr, uCr2));
    else
    {
        Log(("iemRaiseXcptOrInt: %#x at %04x:%RGv cbInstr=%#x fFlags=%#x uErr=%#x uCr2=%llx; prev=%#x depth=%d flags=%#x\n",
             u8Vector, pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, cbInstr, fFlags, uErr, uCr2, pVCpu->iem.s.uCurXcpt,
             pVCpu->iem.s.cXcptRecursions + 1, fPrevXcpt));

        if (pVCpu->iem.s.cXcptRecursions >= 4)
        {
#ifdef DEBUG_bird
            AssertFailed();
#endif
            IEM_RETURN_ASPECT_NOT_IMPLEMENTED_LOG(("Too many fault nestings.\n"));
        }

        /*
         * Evaluate the sequence of recurring events.
         */
        IEMXCPTRAISE enmRaise = IEMEvaluateRecursiveXcpt(pVCpu, fPrevXcpt, uPrevXcpt, fFlags, u8Vector,
                                                         NULL /* pXcptRaiseInfo */);
        if (enmRaise == IEMXCPTRAISE_CURRENT_XCPT)
        { /* likely */ }
        else if (enmRaise == IEMXCPTRAISE_DOUBLE_FAULT)
        {
            Log2(("iemRaiseXcptOrInt: Raising double fault. uPrevXcpt=%#x\n", uPrevXcpt));
            fFlags   = IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR;
            u8Vector = X86_XCPT_DF;
            uErr     = 0;
#ifdef VBOX_WITH_NESTED_HWVIRT_VMX
            /* VMX nested-guest #DF intercept needs to be checked here. */
            if (IEM_VMX_IS_NON_ROOT_MODE(pVCpu))
            {
                VBOXSTRICTRC rcStrict0 = iemVmxVmexitEventDoubleFault(pVCpu);
                if (rcStrict0 != VINF_VMX_INTERCEPT_NOT_ACTIVE)
                    return rcStrict0;
            }
#endif
            /* SVM nested-guest #DF intercepts need to be checked now. See AMD spec. 15.12 "Exception Intercepts". */
            if (IEM_SVM_IS_XCPT_INTERCEPT_SET(pVCpu, X86_XCPT_DF))
                IEM_SVM_VMEXIT_RET(pVCpu, SVM_EXIT_XCPT_DF, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
        }
        else if (enmRaise == IEMXCPTRAISE_TRIPLE_FAULT)
        {
            Log2(("iemRaiseXcptOrInt: Raising triple fault. uPrevXcpt=%#x\n", uPrevXcpt));
            return iemInitiateCpuShutdown(pVCpu);
        }
        else if (enmRaise == IEMXCPTRAISE_CPU_HANG)
        {
            /* If a nested-guest enters an endless CPU loop condition, we'll emulate it; otherwise guru. */
            Log2(("iemRaiseXcptOrInt: CPU hang condition detected\n"));
            if (   !CPUMIsGuestInSvmNestedHwVirtMode(IEM_GET_CTX(pVCpu))
                && !CPUMIsGuestInVmxNonRootMode(IEM_GET_CTX(pVCpu)))
                return VERR_EM_GUEST_CPU_HANG;
        }
        else
        {
            AssertMsgFailed(("Unexpected condition! enmRaise=%#x uPrevXcpt=%#x fPrevXcpt=%#x, u8Vector=%#x fFlags=%#x\n",
                             enmRaise, uPrevXcpt, fPrevXcpt, u8Vector, fFlags));
            return VERR_IEM_IPE_9;
        }

        /*
         * The 'EXT' bit is set when an exception occurs during deliver of an external
         * event (such as an interrupt or earlier exception)[1]. Privileged software
         * exception (INT1) also sets the EXT bit[2]. Exceptions generated by software
         * interrupts and INTO, INT3 instructions, the 'EXT' bit will not be set.
         *
         * [1] - Intel spec. 6.13 "Error Code"
         * [2] - Intel spec. 26.5.1.1 "Details of Vectored-Event Injection".
         * [3] - Intel Instruction reference for INT n.
         */
        if (   (fPrevXcpt & (IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_T_EXT_INT | IEM_XCPT_FLAGS_ICEBP_INSTR))
            && (fFlags & IEM_XCPT_FLAGS_ERR)
            && u8Vector != X86_XCPT_PF
            && u8Vector != X86_XCPT_DF)
        {
            uErr |= X86_TRAP_ERR_EXTERNAL;
        }
    }

    pVCpu->iem.s.cXcptRecursions++;
    pVCpu->iem.s.uCurXcpt    = u8Vector;
    pVCpu->iem.s.fCurXcpt    = fFlags;
    pVCpu->iem.s.uCurXcptErr = uErr;
    pVCpu->iem.s.uCurXcptCr2 = uCr2;

    /*
     * Extensive logging.
     */
#if defined(LOG_ENABLED) && defined(IN_RING3)
    if (LogIs3Enabled())
    {
        IEM_CTX_IMPORT_RET(pVCpu, CPUMCTX_EXTRN_DR_MASK);
        PVM     pVM = pVCpu->CTX_SUFF(pVM);
        char    szRegs[4096];
        DBGFR3RegPrintf(pVM->pUVM, pVCpu->idCpu, &szRegs[0], sizeof(szRegs),
                        "rax=%016VR{rax} rbx=%016VR{rbx} rcx=%016VR{rcx} rdx=%016VR{rdx}\n"
                        "rsi=%016VR{rsi} rdi=%016VR{rdi} r8 =%016VR{r8} r9 =%016VR{r9}\n"
                        "r10=%016VR{r10} r11=%016VR{r11} r12=%016VR{r12} r13=%016VR{r13}\n"
                        "r14=%016VR{r14} r15=%016VR{r15} %VRF{rflags}\n"
                        "rip=%016VR{rip} rsp=%016VR{rsp} rbp=%016VR{rbp}\n"
                        "cs={%04VR{cs} base=%016VR{cs_base} limit=%08VR{cs_lim} flags=%04VR{cs_attr}} cr0=%016VR{cr0}\n"
                        "ds={%04VR{ds} base=%016VR{ds_base} limit=%08VR{ds_lim} flags=%04VR{ds_attr}} cr2=%016VR{cr2}\n"
                        "es={%04VR{es} base=%016VR{es_base} limit=%08VR{es_lim} flags=%04VR{es_attr}} cr3=%016VR{cr3}\n"
                        "fs={%04VR{fs} base=%016VR{fs_base} limit=%08VR{fs_lim} flags=%04VR{fs_attr}} cr4=%016VR{cr4}\n"
                        "gs={%04VR{gs} base=%016VR{gs_base} limit=%08VR{gs_lim} flags=%04VR{gs_attr}} cr8=%016VR{cr8}\n"
                        "ss={%04VR{ss} base=%016VR{ss_base} limit=%08VR{ss_lim} flags=%04VR{ss_attr}}\n"
                        "dr0=%016VR{dr0} dr1=%016VR{dr1} dr2=%016VR{dr2} dr3=%016VR{dr3}\n"
                        "dr6=%016VR{dr6} dr7=%016VR{dr7}\n"
                        "gdtr=%016VR{gdtr_base}:%04VR{gdtr_lim}  idtr=%016VR{idtr_base}:%04VR{idtr_lim}  rflags=%08VR{rflags}\n"
                        "ldtr={%04VR{ldtr} base=%016VR{ldtr_base} limit=%08VR{ldtr_lim} flags=%08VR{ldtr_attr}}\n"
                        "tr  ={%04VR{tr} base=%016VR{tr_base} limit=%08VR{tr_lim} flags=%08VR{tr_attr}}\n"
                        "    sysenter={cs=%04VR{sysenter_cs} eip=%08VR{sysenter_eip} esp=%08VR{sysenter_esp}}\n"
                        "        efer=%016VR{efer}\n"
                        "         pat=%016VR{pat}\n"
                        "     sf_mask=%016VR{sf_mask}\n"
                        "krnl_gs_base=%016VR{krnl_gs_base}\n"
                        "       lstar=%016VR{lstar}\n"
                        "        star=%016VR{star} cstar=%016VR{cstar}\n"
                        "fcw=%04VR{fcw} fsw=%04VR{fsw} ftw=%04VR{ftw} mxcsr=%04VR{mxcsr} mxcsr_mask=%04VR{mxcsr_mask}\n"
                        );

        char szInstr[256];
        DBGFR3DisasInstrEx(pVM->pUVM, pVCpu->idCpu, 0, 0,
                           DBGF_DISAS_FLAGS_CURRENT_GUEST | DBGF_DISAS_FLAGS_DEFAULT_MODE,
                           szInstr, sizeof(szInstr), NULL);
        Log3(("%s%s\n", szRegs, szInstr));
    }
#endif /* LOG_ENABLED */

    /*
     * Stats.
     */
    if (!(fFlags & IEM_XCPT_FLAGS_T_CPU_XCPT))
        STAM_REL_STATS({ pVCpu->iem.s.aStatInts[u8Vector] += 1; });
    else if (u8Vector <= X86_XCPT_LAST)
    {
        STAM_REL_COUNTER_INC(&pVCpu->iem.s.aStatXcpts[u8Vector]);
        EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_XCPT, u8Vector),
                         pVCpu->cpum.GstCtx.rip + pVCpu->cpum.GstCtx.cs.u64Base, ASMReadTSC());
    }

    /*
     * #PF's implies a INVLPG for the CR2 value (see 4.10.1.1 in Intel SDM Vol 3)
     * to ensure that a stale TLB or paging cache entry will only cause one
     * spurious #PF.
     */
    if (    u8Vector == X86_XCPT_PF
        && (fFlags & (IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_CR2)) == (IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_CR2))
        IEMTlbInvalidatePage(pVCpu, uCr2);

    /*
     * Call the mode specific worker function.
     */
    VBOXSTRICTRC    rcStrict;
    if (!(pVCpu->cpum.GstCtx.cr0 & X86_CR0_PE))
        rcStrict = iemRaiseXcptOrIntInRealMode(pVCpu, cbInstr, u8Vector, fFlags, uErr, uCr2);
    else if (pVCpu->cpum.GstCtx.msrEFER & MSR_K6_EFER_LMA)
        rcStrict = iemRaiseXcptOrIntInLongMode(pVCpu, cbInstr, u8Vector, fFlags, uErr, uCr2);
    else
        rcStrict = iemRaiseXcptOrIntInProtMode(pVCpu, cbInstr, u8Vector, fFlags, uErr, uCr2);

    /* Flush the prefetch buffer. */
#ifdef IEM_WITH_CODE_TLB
    pVCpu->iem.s.pbInstrBuf = NULL;
#else
    pVCpu->iem.s.cbOpcode = IEM_GET_INSTR_LEN(pVCpu);
#endif

    /*
     * Unwind.
     */
    pVCpu->iem.s.cXcptRecursions--;
    pVCpu->iem.s.uCurXcpt = uPrevXcpt;
    pVCpu->iem.s.fCurXcpt = fPrevXcpt;
    Log(("iemRaiseXcptOrInt: returns %Rrc (vec=%#x); cs:rip=%04x:%RGv ss:rsp=%04x:%RGv cpl=%u depth=%d\n",
         VBOXSTRICTRC_VAL(rcStrict), u8Vector, pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, pVCpu->cpum.GstCtx.ss.Sel, pVCpu->cpum.GstCtx.esp, pVCpu->iem.s.uCpl,
         pVCpu->iem.s.cXcptRecursions + 1));
    return rcStrict;
}

#ifdef IEM_WITH_SETJMP
/**
 * See iemRaiseXcptOrInt.  Will not return.
 */
DECL_NO_RETURN(void)
iemRaiseXcptOrIntJmp(PVMCPUCC    pVCpu,
                     uint8_t     cbInstr,
                     uint8_t     u8Vector,
                     uint32_t    fFlags,
                     uint16_t    uErr,
                     uint64_t    uCr2) IEM_NOEXCEPT_MAY_LONGJMP
{
    VBOXSTRICTRC rcStrict = iemRaiseXcptOrInt(pVCpu, cbInstr, u8Vector, fFlags, uErr, uCr2);
    IEM_DO_LONGJMP(pVCpu, VBOXSTRICTRC_VAL(rcStrict));
}
#endif


/** \#DE - 00.  */
VBOXSTRICTRC iemRaiseDivideError(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_DE, IEM_XCPT_FLAGS_T_CPU_XCPT, 0, 0);
}


/** \#DB - 01.
 * @note This automatically clear DR7.GD.  */
VBOXSTRICTRC iemRaiseDebugException(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    /* This always clears RF (via IEM_XCPT_FLAGS_DRx_INSTR_BP). */
    pVCpu->cpum.GstCtx.dr[7] &= ~X86_DR7_GD;
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_DB, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_DRx_INSTR_BP, 0, 0);
}


/** \#BR - 05.  */
VBOXSTRICTRC iemRaiseBoundRangeExceeded(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_BR, IEM_XCPT_FLAGS_T_CPU_XCPT, 0, 0);
}


/** \#UD - 06.  */
VBOXSTRICTRC iemRaiseUndefinedOpcode(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_UD, IEM_XCPT_FLAGS_T_CPU_XCPT, 0, 0);
}


/** \#NM - 07.  */
VBOXSTRICTRC iemRaiseDeviceNotAvailable(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_NM, IEM_XCPT_FLAGS_T_CPU_XCPT, 0, 0);
}


/** \#TS(err) - 0a.  */
VBOXSTRICTRC iemRaiseTaskSwitchFaultWithErr(PVMCPUCC pVCpu, uint16_t uErr) RT_NOEXCEPT
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_TS, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, uErr, 0);
}


/** \#TS(tr) - 0a.  */
VBOXSTRICTRC iemRaiseTaskSwitchFaultCurrentTSS(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_TS, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR,
                             pVCpu->cpum.GstCtx.tr.Sel, 0);
}


/** \#TS(0) - 0a.  */
VBOXSTRICTRC iemRaiseTaskSwitchFault0(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_TS, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR,
                             0, 0);
}


/** \#TS(err) - 0a.  */
VBOXSTRICTRC iemRaiseTaskSwitchFaultBySelector(PVMCPUCC pVCpu, uint16_t uSel) RT_NOEXCEPT
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_TS, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR,
                             uSel & X86_SEL_MASK_OFF_RPL, 0);
}


/** \#NP(err) - 0b.  */
VBOXSTRICTRC iemRaiseSelectorNotPresentWithErr(PVMCPUCC pVCpu, uint16_t uErr) RT_NOEXCEPT
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_NP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, uErr, 0);
}


/** \#NP(sel) - 0b.  */
VBOXSTRICTRC iemRaiseSelectorNotPresentBySelector(PVMCPUCC pVCpu, uint16_t uSel) RT_NOEXCEPT
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_NP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR,
                             uSel & ~X86_SEL_RPL, 0);
}


/** \#SS(seg) - 0c.  */
VBOXSTRICTRC iemRaiseStackSelectorNotPresentBySelector(PVMCPUCC pVCpu, uint16_t uSel) RT_NOEXCEPT
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_SS, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR,
                             uSel & ~X86_SEL_RPL, 0);
}


/** \#SS(err) - 0c.  */
VBOXSTRICTRC iemRaiseStackSelectorNotPresentWithErr(PVMCPUCC pVCpu, uint16_t uErr) RT_NOEXCEPT
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_SS, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, uErr, 0);
}


/** \#GP(n) - 0d.  */
VBOXSTRICTRC iemRaiseGeneralProtectionFault(PVMCPUCC pVCpu, uint16_t uErr) RT_NOEXCEPT
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_GP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, uErr, 0);
}


/** \#GP(0) - 0d.  */
VBOXSTRICTRC iemRaiseGeneralProtectionFault0(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_GP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, 0, 0);
}

#ifdef IEM_WITH_SETJMP
/** \#GP(0) - 0d.  */
DECL_NO_RETURN(void) iemRaiseGeneralProtectionFault0Jmp(PVMCPUCC pVCpu) IEM_NOEXCEPT_MAY_LONGJMP
{
    iemRaiseXcptOrIntJmp(pVCpu, 0, X86_XCPT_GP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, 0, 0);
}
#endif


/** \#GP(sel) - 0d.  */
VBOXSTRICTRC iemRaiseGeneralProtectionFaultBySelector(PVMCPUCC pVCpu, RTSEL Sel) RT_NOEXCEPT
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_GP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR,
                             Sel & ~X86_SEL_RPL, 0);
}


/** \#GP(0) - 0d.  */
VBOXSTRICTRC iemRaiseNotCanonical(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_GP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, 0, 0);
}


/** \#GP(sel) - 0d.  */
VBOXSTRICTRC iemRaiseSelectorBounds(PVMCPUCC pVCpu, uint32_t iSegReg, uint32_t fAccess) RT_NOEXCEPT
{
    NOREF(iSegReg); NOREF(fAccess);
    return iemRaiseXcptOrInt(pVCpu, 0, iSegReg == X86_SREG_SS ? X86_XCPT_SS : X86_XCPT_GP,
                             IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, 0, 0);
}

#ifdef IEM_WITH_SETJMP
/** \#GP(sel) - 0d, longjmp.  */
DECL_NO_RETURN(void) iemRaiseSelectorBoundsJmp(PVMCPUCC pVCpu, uint32_t iSegReg, uint32_t fAccess) IEM_NOEXCEPT_MAY_LONGJMP
{
    NOREF(iSegReg); NOREF(fAccess);
    iemRaiseXcptOrIntJmp(pVCpu, 0, iSegReg == X86_SREG_SS ? X86_XCPT_SS : X86_XCPT_GP,
                         IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, 0, 0);
}
#endif

/** \#GP(sel) - 0d.  */
VBOXSTRICTRC iemRaiseSelectorBoundsBySelector(PVMCPUCC pVCpu, RTSEL Sel) RT_NOEXCEPT
{
    NOREF(Sel);
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_GP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, 0, 0);
}

#ifdef IEM_WITH_SETJMP
/** \#GP(sel) - 0d, longjmp.  */
DECL_NO_RETURN(void) iemRaiseSelectorBoundsBySelectorJmp(PVMCPUCC pVCpu, RTSEL Sel) IEM_NOEXCEPT_MAY_LONGJMP
{
    NOREF(Sel);
    iemRaiseXcptOrIntJmp(pVCpu, 0, X86_XCPT_GP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, 0, 0);
}
#endif


/** \#GP(sel) - 0d.  */
VBOXSTRICTRC iemRaiseSelectorInvalidAccess(PVMCPUCC pVCpu, uint32_t iSegReg, uint32_t fAccess) RT_NOEXCEPT
{
    NOREF(iSegReg); NOREF(fAccess);
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_GP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, 0, 0);
}

#ifdef IEM_WITH_SETJMP
/** \#GP(sel) - 0d, longjmp.  */
DECL_NO_RETURN(void) iemRaiseSelectorInvalidAccessJmp(PVMCPUCC pVCpu, uint32_t iSegReg, uint32_t fAccess) IEM_NOEXCEPT_MAY_LONGJMP
{
    NOREF(iSegReg); NOREF(fAccess);
    iemRaiseXcptOrIntJmp(pVCpu, 0, X86_XCPT_GP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, 0, 0);
}
#endif


/** \#PF(n) - 0e.  */
VBOXSTRICTRC iemRaisePageFault(PVMCPUCC pVCpu, RTGCPTR GCPtrWhere, uint32_t cbAccess, uint32_t fAccess, int rc) RT_NOEXCEPT
{
    uint16_t uErr;
    switch (rc)
    {
        case VERR_PAGE_NOT_PRESENT:
        case VERR_PAGE_TABLE_NOT_PRESENT:
        case VERR_PAGE_DIRECTORY_PTR_NOT_PRESENT:
        case VERR_PAGE_MAP_LEVEL4_NOT_PRESENT:
            uErr = 0;
            break;

        default:
            AssertMsgFailed(("%Rrc\n", rc));
            RT_FALL_THRU();
        case VERR_ACCESS_DENIED:
            uErr = X86_TRAP_PF_P;
            break;

        /** @todo reserved  */
    }

    if (pVCpu->iem.s.uCpl == 3)
        uErr |= X86_TRAP_PF_US;

    if (   (fAccess & IEM_ACCESS_WHAT_MASK) == IEM_ACCESS_WHAT_CODE
        && (   (pVCpu->cpum.GstCtx.cr4 & X86_CR4_PAE)
            && (pVCpu->cpum.GstCtx.msrEFER & MSR_K6_EFER_NXE) ) )
        uErr |= X86_TRAP_PF_ID;

#if 0 /* This is so much non-sense, really.  Why was it done like that? */
    /* Note! RW access callers reporting a WRITE protection fault, will clear
             the READ flag before calling.  So, read-modify-write accesses (RW)
             can safely be reported as READ faults. */
    if ((fAccess & (IEM_ACCESS_TYPE_WRITE | IEM_ACCESS_TYPE_READ)) == IEM_ACCESS_TYPE_WRITE)
        uErr |= X86_TRAP_PF_RW;
#else
    if (fAccess & IEM_ACCESS_TYPE_WRITE)
    {
        /// @todo r=bird: bs3-cpu-basic-2 wants X86_TRAP_PF_RW for xchg and cmpxchg
        /// (regardless of outcome of the comparison in the latter case).
        //if (!(fAccess & IEM_ACCESS_TYPE_READ))
            uErr |= X86_TRAP_PF_RW;
    }
#endif

    /* For FXSAVE and FRSTOR the #PF is typically reported at the max address
       of the memory operand rather than at the start of it. (Not sure what
       happens if it crosses a page boundrary.)  The current heuristics for
       this is to report the #PF for the last byte if the access is more than
       64 bytes. This is probably not correct, but we can work that out later,
       main objective now is to get FXSAVE to work like for real hardware and
       make bs3-cpu-basic2 work. */
    if (cbAccess <= 64)
    { /* likely*/ }
    else
        GCPtrWhere += cbAccess - 1;

    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_PF, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR | IEM_XCPT_FLAGS_CR2,
                             uErr, GCPtrWhere);
}

#ifdef IEM_WITH_SETJMP
/** \#PF(n) - 0e, longjmp.  */
DECL_NO_RETURN(void) iemRaisePageFaultJmp(PVMCPUCC pVCpu, RTGCPTR GCPtrWhere, uint32_t cbAccess,
                                          uint32_t fAccess, int rc) IEM_NOEXCEPT_MAY_LONGJMP
{
    IEM_DO_LONGJMP(pVCpu, VBOXSTRICTRC_VAL(iemRaisePageFault(pVCpu, GCPtrWhere, cbAccess, fAccess, rc)));
}
#endif


/** \#MF(0) - 10.  */
VBOXSTRICTRC iemRaiseMathFault(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    if (pVCpu->cpum.GstCtx.cr0 & X86_CR0_NE)
        return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_MF, IEM_XCPT_FLAGS_T_CPU_XCPT, 0, 0);

    /* Convert a #MF into a FERR -> IRQ 13. See @bugref{6117}. */
    PDMIsaSetIrq(pVCpu->CTX_SUFF(pVM), 13 /* u8Irq */, 1 /* u8Level */, 0 /* uTagSrc */);
    return iemRegUpdateRipAndFinishClearingRF(pVCpu);
}


/** \#AC(0) - 11.  */
VBOXSTRICTRC iemRaiseAlignmentCheckException(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_AC, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, 0, 0);
}

#ifdef IEM_WITH_SETJMP
/** \#AC(0) - 11, longjmp.  */
DECL_NO_RETURN(void) iemRaiseAlignmentCheckExceptionJmp(PVMCPUCC pVCpu) IEM_NOEXCEPT_MAY_LONGJMP
{
    IEM_DO_LONGJMP(pVCpu, VBOXSTRICTRC_VAL(iemRaiseAlignmentCheckException(pVCpu)));
}
#endif


/** \#XF(0)/\#XM(0) - 19.   */
VBOXSTRICTRC iemRaiseSimdFpException(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_XF, IEM_XCPT_FLAGS_T_CPU_XCPT, 0, 0);
}


/** Accessed via IEMOP_RAISE_DIVIDE_ERROR.   */
IEM_CIMPL_DEF_0(iemCImplRaiseDivideError)
{
    NOREF(cbInstr);
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_DE, IEM_XCPT_FLAGS_T_CPU_XCPT, 0, 0);
}


/** Accessed via IEMOP_RAISE_INVALID_LOCK_PREFIX. */
IEM_CIMPL_DEF_0(iemCImplRaiseInvalidLockPrefix)
{
    NOREF(cbInstr);
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_UD, IEM_XCPT_FLAGS_T_CPU_XCPT, 0, 0);
}


/** Accessed via IEMOP_RAISE_INVALID_OPCODE. */
IEM_CIMPL_DEF_0(iemCImplRaiseInvalidOpcode)
{
    NOREF(cbInstr);
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_UD, IEM_XCPT_FLAGS_T_CPU_XCPT, 0, 0);
}


/** @}  */

/** @name  Common opcode decoders.
 * @{
 */
//#include <iprt/mem.h>

/**
 * Used to add extra details about a stub case.
 * @param   pVCpu       The cross context virtual CPU structure of the calling thread.
 */
void iemOpStubMsg2(PVMCPUCC pVCpu) RT_NOEXCEPT
{
#if defined(LOG_ENABLED) && defined(IN_RING3)
    PVM  pVM = pVCpu->CTX_SUFF(pVM);
    char szRegs[4096];
    DBGFR3RegPrintf(pVM->pUVM, pVCpu->idCpu, &szRegs[0], sizeof(szRegs),
                    "rax=%016VR{rax} rbx=%016VR{rbx} rcx=%016VR{rcx} rdx=%016VR{rdx}\n"
                    "rsi=%016VR{rsi} rdi=%016VR{rdi} r8 =%016VR{r8} r9 =%016VR{r9}\n"
                    "r10=%016VR{r10} r11=%016VR{r11} r12=%016VR{r12} r13=%016VR{r13}\n"
                    "r14=%016VR{r14} r15=%016VR{r15} %VRF{rflags}\n"
                    "rip=%016VR{rip} rsp=%016VR{rsp} rbp=%016VR{rbp}\n"
                    "cs={%04VR{cs} base=%016VR{cs_base} limit=%08VR{cs_lim} flags=%04VR{cs_attr}} cr0=%016VR{cr0}\n"
                    "ds={%04VR{ds} base=%016VR{ds_base} limit=%08VR{ds_lim} flags=%04VR{ds_attr}} cr2=%016VR{cr2}\n"
                    "es={%04VR{es} base=%016VR{es_base} limit=%08VR{es_lim} flags=%04VR{es_attr}} cr3=%016VR{cr3}\n"
                    "fs={%04VR{fs} base=%016VR{fs_base} limit=%08VR{fs_lim} flags=%04VR{fs_attr}} cr4=%016VR{cr4}\n"
                    "gs={%04VR{gs} base=%016VR{gs_base} limit=%08VR{gs_lim} flags=%04VR{gs_attr}} cr8=%016VR{cr8}\n"
                    "ss={%04VR{ss} base=%016VR{ss_base} limit=%08VR{ss_lim} flags=%04VR{ss_attr}}\n"
                    "dr0=%016VR{dr0} dr1=%016VR{dr1} dr2=%016VR{dr2} dr3=%016VR{dr3}\n"
                    "dr6=%016VR{dr6} dr7=%016VR{dr7}\n"
                    "gdtr=%016VR{gdtr_base}:%04VR{gdtr_lim}  idtr=%016VR{idtr_base}:%04VR{idtr_lim}  rflags=%08VR{rflags}\n"
                    "ldtr={%04VR{ldtr} base=%016VR{ldtr_base} limit=%08VR{ldtr_lim} flags=%08VR{ldtr_attr}}\n"
                    "tr  ={%04VR{tr} base=%016VR{tr_base} limit=%08VR{tr_lim} flags=%08VR{tr_attr}}\n"
                    "    sysenter={cs=%04VR{sysenter_cs} eip=%08VR{sysenter_eip} esp=%08VR{sysenter_esp}}\n"
                    "        efer=%016VR{efer}\n"
                    "         pat=%016VR{pat}\n"
                    "     sf_mask=%016VR{sf_mask}\n"
                    "krnl_gs_base=%016VR{krnl_gs_base}\n"
                    "       lstar=%016VR{lstar}\n"
                    "        star=%016VR{star} cstar=%016VR{cstar}\n"
                    "fcw=%04VR{fcw} fsw=%04VR{fsw} ftw=%04VR{ftw} mxcsr=%04VR{mxcsr} mxcsr_mask=%04VR{mxcsr_mask}\n"
                    );

    char szInstr[256];
    DBGFR3DisasInstrEx(pVM->pUVM, pVCpu->idCpu, 0, 0,
                       DBGF_DISAS_FLAGS_CURRENT_GUEST | DBGF_DISAS_FLAGS_DEFAULT_MODE,
                       szInstr, sizeof(szInstr), NULL);

    RTAssertMsg2Weak("%s%s\n", szRegs, szInstr);
#else
    RTAssertMsg2Weak("cs:rip=%04x:%RX64\n", pVCpu->cpum.GstCtx.cs, pVCpu->cpum.GstCtx.rip);
#endif
}

/** @} */



/** @name   Register Access.
 * @{
 */

/**
 * Adds a 8-bit signed jump offset to RIP/EIP/IP.
 *
 * May raise a \#GP(0) if the new RIP is non-canonical or outside the code
 * segment limit.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   cbInstr             Instruction size.
 * @param   offNextInstr        The offset of the next instruction.
 * @param   enmEffOpSize        Effective operand size.
 */
VBOXSTRICTRC iemRegRipRelativeJumpS8AndFinishClearingRF(PVMCPUCC pVCpu, uint8_t cbInstr, int8_t offNextInstr,
                                                        IEMMODE enmEffOpSize) RT_NOEXCEPT
{
    switch (enmEffOpSize)
    {
        case IEMMODE_16BIT:
        {
            uint16_t const uNewIp = pVCpu->cpum.GstCtx.ip + cbInstr + (int16_t)offNextInstr;
            if (RT_LIKELY(   uNewIp <= pVCpu->cpum.GstCtx.cs.u32Limit
                          || pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT /* no CS limit checks in 64-bit mode */))
                pVCpu->cpum.GstCtx.rip = uNewIp;
            else
                return iemRaiseGeneralProtectionFault0(pVCpu);
            break;
        }

        case IEMMODE_32BIT:
        {
            Assert(pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT);
            Assert(pVCpu->cpum.GstCtx.rip <= UINT32_MAX);

            uint32_t const uNewEip = pVCpu->cpum.GstCtx.eip + cbInstr + (int32_t)offNextInstr;
            if (RT_LIKELY(uNewEip <= pVCpu->cpum.GstCtx.cs.u32Limit))
                pVCpu->cpum.GstCtx.rip = uNewEip;
            else
                return iemRaiseGeneralProtectionFault0(pVCpu);
            break;
        }

        case IEMMODE_64BIT:
        {
            Assert(pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT);

            uint64_t const uNewRip = pVCpu->cpum.GstCtx.rip + cbInstr + (int64_t)offNextInstr;
            if (RT_LIKELY(IEM_IS_CANONICAL(uNewRip)))
                pVCpu->cpum.GstCtx.rip = uNewRip;
            else
                return iemRaiseGeneralProtectionFault0(pVCpu);
            break;
        }

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }

#ifndef IEM_WITH_CODE_TLB
    /* Flush the prefetch buffer. */
    pVCpu->iem.s.cbOpcode = cbInstr;
#endif

    /*
     * Clear RF and finish the instruction (maybe raise #DB).
     */
    return iemRegFinishClearingRF(pVCpu);
}


/**
 * Adds a 16-bit signed jump offset to RIP/EIP/IP.
 *
 * May raise a \#GP(0) if the new RIP is non-canonical or outside the code
 * segment limit.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   cbInstr             Instruction size.
 * @param   offNextInstr        The offset of the next instruction.
 */
VBOXSTRICTRC iemRegRipRelativeJumpS16AndFinishClearingRF(PVMCPUCC pVCpu, uint8_t cbInstr, int16_t offNextInstr) RT_NOEXCEPT
{
    Assert(pVCpu->iem.s.enmEffOpSize == IEMMODE_16BIT);

    uint16_t const uNewIp = pVCpu->cpum.GstCtx.ip + cbInstr + offNextInstr;
    if (RT_LIKELY(   uNewIp <= pVCpu->cpum.GstCtx.cs.u32Limit
                  || pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT /* no limit checking in 64-bit mode */))
        pVCpu->cpum.GstCtx.rip = uNewIp;
    else
        return iemRaiseGeneralProtectionFault0(pVCpu);

#ifndef IEM_WITH_CODE_TLB
    /* Flush the prefetch buffer. */
    pVCpu->iem.s.cbOpcode = IEM_GET_INSTR_LEN(pVCpu);
#endif

    /*
     * Clear RF and finish the instruction (maybe raise #DB).
     */
    return iemRegFinishClearingRF(pVCpu);
}


/**
 * Adds a 32-bit signed jump offset to RIP/EIP/IP.
 *
 * May raise a \#GP(0) if the new RIP is non-canonical or outside the code
 * segment limit.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   cbInstr             Instruction size.
 * @param   offNextInstr        The offset of the next instruction.
 * @param   enmEffOpSize        Effective operand size.
 */
VBOXSTRICTRC iemRegRipRelativeJumpS32AndFinishClearingRF(PVMCPUCC pVCpu, uint8_t cbInstr, int32_t offNextInstr,
                                                         IEMMODE enmEffOpSize) RT_NOEXCEPT
{
    if (enmEffOpSize == IEMMODE_32BIT)
    {
        Assert(pVCpu->cpum.GstCtx.rip <= UINT32_MAX); Assert(pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT);

        uint32_t const uNewEip = pVCpu->cpum.GstCtx.eip + cbInstr + offNextInstr;
        if (RT_LIKELY(uNewEip <= pVCpu->cpum.GstCtx.cs.u32Limit))
            pVCpu->cpum.GstCtx.rip = uNewEip;
        else
            return iemRaiseGeneralProtectionFault0(pVCpu);
    }
    else
    {
        Assert(enmEffOpSize == IEMMODE_64BIT);

        uint64_t const uNewRip = pVCpu->cpum.GstCtx.rip + cbInstr + (int64_t)offNextInstr;
        if (RT_LIKELY(IEM_IS_CANONICAL(uNewRip)))
            pVCpu->cpum.GstCtx.rip = uNewRip;
        else
            return iemRaiseGeneralProtectionFault0(pVCpu);
    }

#ifndef IEM_WITH_CODE_TLB
    /* Flush the prefetch buffer. */
    pVCpu->iem.s.cbOpcode = IEM_GET_INSTR_LEN(pVCpu);
#endif

    /*
     * Clear RF and finish the instruction (maybe raise #DB).
     */
    return iemRegFinishClearingRF(pVCpu);
}


/**
 * Performs a near jump to the specified address.
 *
 * May raise a \#GP(0) if the new IP outside the code segment limit.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   uNewIp              The new IP value.
 */
VBOXSTRICTRC iemRegRipJumpU16AndFinishClearningRF(PVMCPUCC pVCpu, uint16_t uNewIp) RT_NOEXCEPT
{
    if (RT_LIKELY(   uNewIp <= pVCpu->cpum.GstCtx.cs.u32Limit
                  || pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT /* no limit checks in 64-bit mode */))
        pVCpu->cpum.GstCtx.rip = uNewIp;
    else
        return iemRaiseGeneralProtectionFault0(pVCpu);
    /** @todo Test 16-bit jump in 64-bit mode.  */

#ifndef IEM_WITH_CODE_TLB
    /* Flush the prefetch buffer. */
    pVCpu->iem.s.cbOpcode = IEM_GET_INSTR_LEN(pVCpu);
#endif

    /*
     * Clear RF and finish the instruction (maybe raise #DB).
     */
    return iemRegFinishClearingRF(pVCpu);
}


/**
 * Performs a near jump to the specified address.
 *
 * May raise a \#GP(0) if the new RIP is outside the code segment limit.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   uNewEip             The new EIP value.
 */
VBOXSTRICTRC iemRegRipJumpU32AndFinishClearningRF(PVMCPUCC pVCpu, uint32_t uNewEip) RT_NOEXCEPT
{
    Assert(pVCpu->cpum.GstCtx.rip <= UINT32_MAX);
    Assert(pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT);

    if (RT_LIKELY(uNewEip <= pVCpu->cpum.GstCtx.cs.u32Limit))
        pVCpu->cpum.GstCtx.rip = uNewEip;
    else
        return iemRaiseGeneralProtectionFault0(pVCpu);

#ifndef IEM_WITH_CODE_TLB
    /* Flush the prefetch buffer. */
    pVCpu->iem.s.cbOpcode = IEM_GET_INSTR_LEN(pVCpu);
#endif

    /*
     * Clear RF and finish the instruction (maybe raise #DB).
     */
    return iemRegFinishClearingRF(pVCpu);
}


/**
 * Performs a near jump to the specified address.
 *
 * May raise a \#GP(0) if the new RIP is non-canonical or outside the code
 * segment limit.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   uNewRip             The new RIP value.
 */
VBOXSTRICTRC iemRegRipJumpU64AndFinishClearningRF(PVMCPUCC pVCpu, uint64_t uNewRip) RT_NOEXCEPT
{
    Assert(pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT);

    if (RT_LIKELY(IEM_IS_CANONICAL(uNewRip)))
        pVCpu->cpum.GstCtx.rip = uNewRip;
    else
        return iemRaiseGeneralProtectionFault0(pVCpu);

#ifndef IEM_WITH_CODE_TLB
    /* Flush the prefetch buffer. */
    pVCpu->iem.s.cbOpcode = IEM_GET_INSTR_LEN(pVCpu);
#endif

    /*
     * Clear RF and finish the instruction (maybe raise #DB).
     */
    return iemRegFinishClearingRF(pVCpu);
}

/** @}  */


/** @name   FPU access and helpers.
 *
 * @{
 */

/**
 * Updates the x87.DS and FPUDP registers.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pFpuCtx             The FPU context.
 * @param   iEffSeg             The effective segment register.
 * @param   GCPtrEff            The effective address relative to @a iEffSeg.
 */
DECLINLINE(void) iemFpuUpdateDP(PVMCPUCC pVCpu, PX86FXSTATE pFpuCtx, uint8_t iEffSeg, RTGCPTR GCPtrEff)
{
    RTSEL sel;
    switch (iEffSeg)
    {
        case X86_SREG_DS: sel = pVCpu->cpum.GstCtx.ds.Sel; break;
        case X86_SREG_SS: sel = pVCpu->cpum.GstCtx.ss.Sel; break;
        case X86_SREG_CS: sel = pVCpu->cpum.GstCtx.cs.Sel; break;
        case X86_SREG_ES: sel = pVCpu->cpum.GstCtx.es.Sel; break;
        case X86_SREG_FS: sel = pVCpu->cpum.GstCtx.fs.Sel; break;
        case X86_SREG_GS: sel = pVCpu->cpum.GstCtx.gs.Sel; break;
        default:
            AssertMsgFailed(("%d\n", iEffSeg));
            sel = pVCpu->cpum.GstCtx.ds.Sel;
    }
    /** @todo pFpuCtx->DS and FPUDP needs to be kept seperately. */
    if (IEM_IS_REAL_OR_V86_MODE(pVCpu))
    {
        pFpuCtx->DS    = 0;
        pFpuCtx->FPUDP = (uint32_t)GCPtrEff + ((uint32_t)sel << 4);
    }
    else if (!IEM_IS_LONG_MODE(pVCpu))
    {
        pFpuCtx->DS    = sel;
        pFpuCtx->FPUDP = GCPtrEff;
    }
    else
        *(uint64_t *)&pFpuCtx->FPUDP = GCPtrEff;
}


/**
 * Rotates the stack registers in the push direction.
 *
 * @param   pFpuCtx             The FPU context.
 * @remarks This is a complete waste of time, but fxsave stores the registers in
 *          stack order.
 */
DECLINLINE(void) iemFpuRotateStackPush(PX86FXSTATE pFpuCtx)
{
    RTFLOAT80U r80Tmp = pFpuCtx->aRegs[7].r80;
    pFpuCtx->aRegs[7].r80 = pFpuCtx->aRegs[6].r80;
    pFpuCtx->aRegs[6].r80 = pFpuCtx->aRegs[5].r80;
    pFpuCtx->aRegs[5].r80 = pFpuCtx->aRegs[4].r80;
    pFpuCtx->aRegs[4].r80 = pFpuCtx->aRegs[3].r80;
    pFpuCtx->aRegs[3].r80 = pFpuCtx->aRegs[2].r80;
    pFpuCtx->aRegs[2].r80 = pFpuCtx->aRegs[1].r80;
    pFpuCtx->aRegs[1].r80 = pFpuCtx->aRegs[0].r80;
    pFpuCtx->aRegs[0].r80 = r80Tmp;
}


/**
 * Rotates the stack registers in the pop direction.
 *
 * @param   pFpuCtx             The FPU context.
 * @remarks This is a complete waste of time, but fxsave stores the registers in
 *          stack order.
 */
DECLINLINE(void) iemFpuRotateStackPop(PX86FXSTATE pFpuCtx)
{
    RTFLOAT80U r80Tmp = pFpuCtx->aRegs[0].r80;
    pFpuCtx->aRegs[0].r80 = pFpuCtx->aRegs[1].r80;
    pFpuCtx->aRegs[1].r80 = pFpuCtx->aRegs[2].r80;
    pFpuCtx->aRegs[2].r80 = pFpuCtx->aRegs[3].r80;
    pFpuCtx->aRegs[3].r80 = pFpuCtx->aRegs[4].r80;
    pFpuCtx->aRegs[4].r80 = pFpuCtx->aRegs[5].r80;
    pFpuCtx->aRegs[5].r80 = pFpuCtx->aRegs[6].r80;
    pFpuCtx->aRegs[6].r80 = pFpuCtx->aRegs[7].r80;
    pFpuCtx->aRegs[7].r80 = r80Tmp;
}


/**
 * Updates FSW and pushes a FPU result onto the FPU stack if no pending
 * exception prevents it.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pResult             The FPU operation result to push.
 * @param   pFpuCtx             The FPU context.
 */
static void iemFpuMaybePushResult(PVMCPU pVCpu, PIEMFPURESULT pResult, PX86FXSTATE pFpuCtx) RT_NOEXCEPT
{
    /* Update FSW and bail if there are pending exceptions afterwards. */
    uint16_t fFsw = pFpuCtx->FSW & ~X86_FSW_C_MASK;
    fFsw |= pResult->FSW & ~X86_FSW_TOP_MASK;
    if (   (fFsw         & (X86_FSW_IE | X86_FSW_ZE | X86_FSW_DE))
        & ~(pFpuCtx->FCW & (X86_FCW_IM | X86_FCW_ZM | X86_FCW_DM)))
    {
        if ((fFsw & X86_FSW_ES) && !(pFpuCtx->FCW & X86_FSW_ES))
            Log11(("iemFpuMaybePushResult: %04x:%08RX64: FSW %#x -> %#x\n",
                   pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, pFpuCtx->FSW, fFsw));
        pFpuCtx->FSW = fFsw;
        return;
    }

    uint16_t iNewTop = (X86_FSW_TOP_GET(fFsw) + 7) & X86_FSW_TOP_SMASK;
    if (!(pFpuCtx->FTW & RT_BIT(iNewTop)))
    {
        /* All is fine, push the actual value. */
        pFpuCtx->FTW |= RT_BIT(iNewTop);
        pFpuCtx->aRegs[7].r80 = pResult->r80Result;
    }
    else if (pFpuCtx->FCW & X86_FCW_IM)
    {
        /* Masked stack overflow, push QNaN. */
        fFsw |= X86_FSW_IE | X86_FSW_SF | X86_FSW_C1;
        iemFpuStoreQNan(&pFpuCtx->aRegs[7].r80);
    }
    else
    {
        /* Raise stack overflow, don't push anything. */
        pFpuCtx->FSW |= pResult->FSW & ~X86_FSW_C_MASK;
        pFpuCtx->FSW |= X86_FSW_IE | X86_FSW_SF | X86_FSW_C1 | X86_FSW_B | X86_FSW_ES;
        Log11(("iemFpuMaybePushResult: %04x:%08RX64: stack overflow (FSW=%#x)\n",
               pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, pFpuCtx->FSW));
        return;
    }

    fFsw &= ~X86_FSW_TOP_MASK;
    fFsw |= iNewTop << X86_FSW_TOP_SHIFT;
    pFpuCtx->FSW = fFsw;

    iemFpuRotateStackPush(pFpuCtx);
    RT_NOREF(pVCpu);
}


/**
 * Stores a result in a FPU register and updates the FSW and FTW.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pFpuCtx             The FPU context.
 * @param   pResult             The result to store.
 * @param   iStReg              Which FPU register to store it in.
 */
static void iemFpuStoreResultOnly(PVMCPU pVCpu, PX86FXSTATE pFpuCtx, PIEMFPURESULT pResult, uint8_t iStReg) RT_NOEXCEPT
{
    Assert(iStReg < 8);
    uint16_t       fNewFsw = pFpuCtx->FSW;
    uint16_t const iReg    = (X86_FSW_TOP_GET(fNewFsw) + iStReg) & X86_FSW_TOP_SMASK;
    fNewFsw &= ~X86_FSW_C_MASK;
    fNewFsw |= pResult->FSW & ~X86_FSW_TOP_MASK;
    if ((fNewFsw & X86_FSW_ES) && !(pFpuCtx->FSW & X86_FSW_ES))
        Log11(("iemFpuStoreResultOnly: %04x:%08RX64: FSW %#x -> %#x\n",
               pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, pFpuCtx->FSW, fNewFsw));
    pFpuCtx->FSW  = fNewFsw;
    pFpuCtx->FTW |= RT_BIT(iReg);
    pFpuCtx->aRegs[iStReg].r80 = pResult->r80Result;
    RT_NOREF(pVCpu);
}


/**
 * Only updates the FPU status word (FSW) with the result of the current
 * instruction.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pFpuCtx             The FPU context.
 * @param   u16FSW              The FSW output of the current instruction.
 */
static void iemFpuUpdateFSWOnly(PVMCPU pVCpu, PX86FXSTATE pFpuCtx, uint16_t u16FSW) RT_NOEXCEPT
{
    uint16_t fNewFsw = pFpuCtx->FSW;
    fNewFsw &= ~X86_FSW_C_MASK;
    fNewFsw |= u16FSW & ~X86_FSW_TOP_MASK;
    if ((fNewFsw & X86_FSW_ES) && !(pFpuCtx->FSW & X86_FSW_ES))
        Log11(("iemFpuStoreResultOnly: %04x:%08RX64: FSW %#x -> %#x\n",
               pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, pFpuCtx->FSW, fNewFsw));
    pFpuCtx->FSW = fNewFsw;
    RT_NOREF(pVCpu);
}


/**
 * Pops one item off the FPU stack if no pending exception prevents it.
 *
 * @param   pFpuCtx             The FPU context.
 */
static void iemFpuMaybePopOne(PX86FXSTATE pFpuCtx) RT_NOEXCEPT
{
    /* Check pending exceptions. */
    uint16_t uFSW = pFpuCtx->FSW;
    if (   (pFpuCtx->FSW & (X86_FSW_IE | X86_FSW_ZE | X86_FSW_DE))
        & ~(pFpuCtx->FCW & (X86_FCW_IM | X86_FCW_ZM | X86_FCW_DM)))
        return;

    /* TOP--. */
    uint16_t iOldTop = uFSW & X86_FSW_TOP_MASK;
    uFSW &= ~X86_FSW_TOP_MASK;
    uFSW |= (iOldTop + (UINT16_C(9) << X86_FSW_TOP_SHIFT)) & X86_FSW_TOP_MASK;
    pFpuCtx->FSW = uFSW;

    /* Mark the previous ST0 as empty. */
    iOldTop >>= X86_FSW_TOP_SHIFT;
    pFpuCtx->FTW &= ~RT_BIT(iOldTop);

    /* Rotate the registers. */
    iemFpuRotateStackPop(pFpuCtx);
}


/**
 * Pushes a FPU result onto the FPU stack if no pending exception prevents it.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pResult             The FPU operation result to push.
 */
void iemFpuPushResult(PVMCPUCC pVCpu, PIEMFPURESULT pResult) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);
    iemFpuMaybePushResult(pVCpu, pResult, pFpuCtx);
}


/**
 * Pushes a FPU result onto the FPU stack if no pending exception prevents it,
 * and sets FPUDP and FPUDS.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pResult             The FPU operation result to push.
 * @param   iEffSeg             The effective segment register.
 * @param   GCPtrEff            The effective address relative to @a iEffSeg.
 */
void iemFpuPushResultWithMemOp(PVMCPUCC pVCpu, PIEMFPURESULT pResult, uint8_t iEffSeg, RTGCPTR GCPtrEff) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateDP(pVCpu, pFpuCtx, iEffSeg, GCPtrEff);
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);
    iemFpuMaybePushResult(pVCpu, pResult, pFpuCtx);
}


/**
 * Replace ST0 with the first value and push the second onto the FPU stack,
 * unless a pending exception prevents it.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pResult             The FPU operation result to store and push.
 */
void iemFpuPushResultTwo(PVMCPUCC pVCpu, PIEMFPURESULTTWO pResult) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);

    /* Update FSW and bail if there are pending exceptions afterwards. */
    uint16_t fFsw = pFpuCtx->FSW & ~X86_FSW_C_MASK;
    fFsw |= pResult->FSW & ~X86_FSW_TOP_MASK;
    if (   (fFsw         & (X86_FSW_IE | X86_FSW_ZE | X86_FSW_DE))
        & ~(pFpuCtx->FCW & (X86_FCW_IM | X86_FCW_ZM | X86_FCW_DM)))
    {
        if ((fFsw & X86_FSW_ES) && !(pFpuCtx->FSW & X86_FSW_ES))
            Log11(("iemFpuPushResultTwo: %04x:%08RX64: FSW %#x -> %#x\n",
                   pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, pFpuCtx->FSW, fFsw));
        pFpuCtx->FSW = fFsw;
        return;
    }

    uint16_t iNewTop = (X86_FSW_TOP_GET(fFsw) + 7) & X86_FSW_TOP_SMASK;
    if (!(pFpuCtx->FTW & RT_BIT(iNewTop)))
    {
        /* All is fine, push the actual value. */
        pFpuCtx->FTW |= RT_BIT(iNewTop);
        pFpuCtx->aRegs[0].r80 = pResult->r80Result1;
        pFpuCtx->aRegs[7].r80 = pResult->r80Result2;
    }
    else if (pFpuCtx->FCW & X86_FCW_IM)
    {
        /* Masked stack overflow, push QNaN. */
        fFsw |= X86_FSW_IE | X86_FSW_SF | X86_FSW_C1;
        iemFpuStoreQNan(&pFpuCtx->aRegs[0].r80);
        iemFpuStoreQNan(&pFpuCtx->aRegs[7].r80);
    }
    else
    {
        /* Raise stack overflow, don't push anything. */
        pFpuCtx->FSW |= pResult->FSW & ~X86_FSW_C_MASK;
        pFpuCtx->FSW |= X86_FSW_IE | X86_FSW_SF | X86_FSW_C1 | X86_FSW_B | X86_FSW_ES;
        Log11(("iemFpuPushResultTwo: %04x:%08RX64: stack overflow (FSW=%#x)\n",
               pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, pFpuCtx->FSW));
        return;
    }

    fFsw &= ~X86_FSW_TOP_MASK;
    fFsw |= iNewTop << X86_FSW_TOP_SHIFT;
    pFpuCtx->FSW = fFsw;

    iemFpuRotateStackPush(pFpuCtx);
}


/**
 * Stores a result in a FPU register, updates the FSW, FTW, FPUIP, FPUCS, and
 * FOP.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pResult             The result to store.
 * @param   iStReg              Which FPU register to store it in.
 */
void iemFpuStoreResult(PVMCPUCC pVCpu, PIEMFPURESULT pResult, uint8_t iStReg) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);
    iemFpuStoreResultOnly(pVCpu, pFpuCtx, pResult, iStReg);
}


/**
 * Stores a result in a FPU register, updates the FSW, FTW, FPUIP, FPUCS, and
 * FOP, and then pops the stack.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pResult             The result to store.
 * @param   iStReg              Which FPU register to store it in.
 */
void iemFpuStoreResultThenPop(PVMCPUCC pVCpu, PIEMFPURESULT pResult, uint8_t iStReg) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);
    iemFpuStoreResultOnly(pVCpu, pFpuCtx, pResult, iStReg);
    iemFpuMaybePopOne(pFpuCtx);
}


/**
 * Stores a result in a FPU register, updates the FSW, FTW, FPUIP, FPUCS, FOP,
 * FPUDP, and FPUDS.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pResult             The result to store.
 * @param   iStReg              Which FPU register to store it in.
 * @param   iEffSeg             The effective memory operand selector register.
 * @param   GCPtrEff            The effective memory operand offset.
 */
void iemFpuStoreResultWithMemOp(PVMCPUCC pVCpu, PIEMFPURESULT pResult, uint8_t iStReg,
                                uint8_t iEffSeg, RTGCPTR GCPtrEff) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateDP(pVCpu, pFpuCtx, iEffSeg, GCPtrEff);
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);
    iemFpuStoreResultOnly(pVCpu, pFpuCtx, pResult, iStReg);
}


/**
 * Stores a result in a FPU register, updates the FSW, FTW, FPUIP, FPUCS, FOP,
 * FPUDP, and FPUDS, and then pops the stack.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pResult             The result to store.
 * @param   iStReg              Which FPU register to store it in.
 * @param   iEffSeg             The effective memory operand selector register.
 * @param   GCPtrEff            The effective memory operand offset.
 */
void iemFpuStoreResultWithMemOpThenPop(PVMCPUCC pVCpu, PIEMFPURESULT pResult,
                                       uint8_t iStReg, uint8_t iEffSeg, RTGCPTR GCPtrEff) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateDP(pVCpu, pFpuCtx, iEffSeg, GCPtrEff);
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);
    iemFpuStoreResultOnly(pVCpu, pFpuCtx, pResult, iStReg);
    iemFpuMaybePopOne(pFpuCtx);
}


/**
 * Updates the FOP, FPUIP, and FPUCS.  For FNOP.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
void iemFpuUpdateOpcodeAndIp(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);
}


/**
 * Updates the FSW, FOP, FPUIP, and FPUCS.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u16FSW              The FSW from the current instruction.
 */
void iemFpuUpdateFSW(PVMCPUCC pVCpu, uint16_t u16FSW) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);
    iemFpuUpdateFSWOnly(pVCpu, pFpuCtx, u16FSW);
}


/**
 * Updates the FSW, FOP, FPUIP, and FPUCS, then pops the stack.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u16FSW              The FSW from the current instruction.
 */
void iemFpuUpdateFSWThenPop(PVMCPUCC pVCpu, uint16_t u16FSW) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);
    iemFpuUpdateFSWOnly(pVCpu, pFpuCtx, u16FSW);
    iemFpuMaybePopOne(pFpuCtx);
}


/**
 * Updates the FSW, FOP, FPUIP, FPUCS, FPUDP, and FPUDS.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u16FSW              The FSW from the current instruction.
 * @param   iEffSeg             The effective memory operand selector register.
 * @param   GCPtrEff            The effective memory operand offset.
 */
void iemFpuUpdateFSWWithMemOp(PVMCPUCC pVCpu, uint16_t u16FSW, uint8_t iEffSeg, RTGCPTR GCPtrEff) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateDP(pVCpu, pFpuCtx, iEffSeg, GCPtrEff);
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);
    iemFpuUpdateFSWOnly(pVCpu, pFpuCtx, u16FSW);
}


/**
 * Updates the FSW, FOP, FPUIP, and FPUCS, then pops the stack twice.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u16FSW              The FSW from the current instruction.
 */
void iemFpuUpdateFSWThenPopPop(PVMCPUCC pVCpu, uint16_t u16FSW) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);
    iemFpuUpdateFSWOnly(pVCpu, pFpuCtx, u16FSW);
    iemFpuMaybePopOne(pFpuCtx);
    iemFpuMaybePopOne(pFpuCtx);
}


/**
 * Updates the FSW, FOP, FPUIP, FPUCS, FPUDP, and FPUDS, then pops the stack.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u16FSW              The FSW from the current instruction.
 * @param   iEffSeg             The effective memory operand selector register.
 * @param   GCPtrEff            The effective memory operand offset.
 */
void iemFpuUpdateFSWWithMemOpThenPop(PVMCPUCC pVCpu, uint16_t u16FSW, uint8_t iEffSeg, RTGCPTR GCPtrEff) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateDP(pVCpu, pFpuCtx, iEffSeg, GCPtrEff);
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);
    iemFpuUpdateFSWOnly(pVCpu, pFpuCtx, u16FSW);
    iemFpuMaybePopOne(pFpuCtx);
}


/**
 * Worker routine for raising an FPU stack underflow exception.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pFpuCtx             The FPU context.
 * @param   iStReg              The stack register being accessed.
 */
static void iemFpuStackUnderflowOnly(PVMCPU pVCpu, PX86FXSTATE pFpuCtx, uint8_t iStReg)
{
    Assert(iStReg < 8 || iStReg == UINT8_MAX);
    if (pFpuCtx->FCW & X86_FCW_IM)
    {
        /* Masked underflow. */
        pFpuCtx->FSW &= ~X86_FSW_C_MASK;
        pFpuCtx->FSW |= X86_FSW_IE | X86_FSW_SF;
        uint16_t iReg = (X86_FSW_TOP_GET(pFpuCtx->FSW) + iStReg) & X86_FSW_TOP_SMASK;
        if (iStReg != UINT8_MAX)
        {
            pFpuCtx->FTW |= RT_BIT(iReg);
            iemFpuStoreQNan(&pFpuCtx->aRegs[iStReg].r80);
        }
    }
    else
    {
        pFpuCtx->FSW &= ~X86_FSW_C_MASK;
        pFpuCtx->FSW |= X86_FSW_IE | X86_FSW_SF | X86_FSW_ES | X86_FSW_B;
        Log11(("iemFpuStackUnderflowOnly: %04x:%08RX64: underflow (FSW=%#x)\n",
               pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, pFpuCtx->FSW));
    }
    RT_NOREF(pVCpu);
}


/**
 * Raises a FPU stack underflow exception.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iStReg              The destination register that should be loaded
 *                              with QNaN if \#IS is not masked. Specify
 *                              UINT8_MAX if none (like for fcom).
 */
void iemFpuStackUnderflow(PVMCPUCC pVCpu, uint8_t iStReg) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);
    iemFpuStackUnderflowOnly(pVCpu, pFpuCtx, iStReg);
}


void iemFpuStackUnderflowWithMemOp(PVMCPUCC pVCpu, uint8_t iStReg, uint8_t iEffSeg, RTGCPTR GCPtrEff) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateDP(pVCpu, pFpuCtx, iEffSeg, GCPtrEff);
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);
    iemFpuStackUnderflowOnly(pVCpu, pFpuCtx, iStReg);
}


void iemFpuStackUnderflowThenPop(PVMCPUCC pVCpu, uint8_t iStReg) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);
    iemFpuStackUnderflowOnly(pVCpu, pFpuCtx, iStReg);
    iemFpuMaybePopOne(pFpuCtx);
}


void iemFpuStackUnderflowWithMemOpThenPop(PVMCPUCC pVCpu, uint8_t iStReg, uint8_t iEffSeg, RTGCPTR GCPtrEff) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateDP(pVCpu, pFpuCtx, iEffSeg, GCPtrEff);
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);
    iemFpuStackUnderflowOnly(pVCpu, pFpuCtx, iStReg);
    iemFpuMaybePopOne(pFpuCtx);
}


void iemFpuStackUnderflowThenPopPop(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);
    iemFpuStackUnderflowOnly(pVCpu, pFpuCtx, UINT8_MAX);
    iemFpuMaybePopOne(pFpuCtx);
    iemFpuMaybePopOne(pFpuCtx);
}


void iemFpuStackPushUnderflow(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);

    if (pFpuCtx->FCW & X86_FCW_IM)
    {
        /* Masked overflow - Push QNaN. */
        uint16_t iNewTop = (X86_FSW_TOP_GET(pFpuCtx->FSW) + 7) & X86_FSW_TOP_SMASK;
        pFpuCtx->FSW &= ~(X86_FSW_TOP_MASK | X86_FSW_C_MASK);
        pFpuCtx->FSW |= X86_FSW_IE | X86_FSW_SF;
        pFpuCtx->FSW |= iNewTop << X86_FSW_TOP_SHIFT;
        pFpuCtx->FTW |= RT_BIT(iNewTop);
        iemFpuStoreQNan(&pFpuCtx->aRegs[7].r80);
        iemFpuRotateStackPush(pFpuCtx);
    }
    else
    {
        /* Exception pending - don't change TOP or the register stack. */
        pFpuCtx->FSW &= ~X86_FSW_C_MASK;
        pFpuCtx->FSW |= X86_FSW_IE | X86_FSW_SF | X86_FSW_ES | X86_FSW_B;
        Log11(("iemFpuStackPushUnderflow: %04x:%08RX64: underflow (FSW=%#x)\n",
               pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, pFpuCtx->FSW));
    }
}


void iemFpuStackPushUnderflowTwo(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);

    if (pFpuCtx->FCW & X86_FCW_IM)
    {
        /* Masked overflow - Push QNaN. */
        uint16_t iNewTop = (X86_FSW_TOP_GET(pFpuCtx->FSW) + 7) & X86_FSW_TOP_SMASK;
        pFpuCtx->FSW &= ~(X86_FSW_TOP_MASK | X86_FSW_C_MASK);
        pFpuCtx->FSW |= X86_FSW_IE | X86_FSW_SF;
        pFpuCtx->FSW |= iNewTop << X86_FSW_TOP_SHIFT;
        pFpuCtx->FTW |= RT_BIT(iNewTop);
        iemFpuStoreQNan(&pFpuCtx->aRegs[0].r80);
        iemFpuStoreQNan(&pFpuCtx->aRegs[7].r80);
        iemFpuRotateStackPush(pFpuCtx);
    }
    else
    {
        /* Exception pending - don't change TOP or the register stack. */
        pFpuCtx->FSW &= ~X86_FSW_C_MASK;
        pFpuCtx->FSW |= X86_FSW_IE | X86_FSW_SF | X86_FSW_ES | X86_FSW_B;
        Log11(("iemFpuStackPushUnderflowTwo: %04x:%08RX64: underflow (FSW=%#x)\n",
               pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, pFpuCtx->FSW));
    }
}


/**
 * Worker routine for raising an FPU stack overflow exception on a push.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pFpuCtx             The FPU context.
 */
static void iemFpuStackPushOverflowOnly(PVMCPU pVCpu, PX86FXSTATE pFpuCtx) RT_NOEXCEPT
{
    if (pFpuCtx->FCW & X86_FCW_IM)
    {
        /* Masked overflow. */
        uint16_t iNewTop = (X86_FSW_TOP_GET(pFpuCtx->FSW) + 7) & X86_FSW_TOP_SMASK;
        pFpuCtx->FSW &= ~(X86_FSW_TOP_MASK | X86_FSW_C_MASK);
        pFpuCtx->FSW |= X86_FSW_C1 | X86_FSW_IE | X86_FSW_SF;
        pFpuCtx->FSW |= iNewTop << X86_FSW_TOP_SHIFT;
        pFpuCtx->FTW |= RT_BIT(iNewTop);
        iemFpuStoreQNan(&pFpuCtx->aRegs[7].r80);
        iemFpuRotateStackPush(pFpuCtx);
    }
    else
    {
        /* Exception pending - don't change TOP or the register stack. */
        pFpuCtx->FSW &= ~X86_FSW_C_MASK;
        pFpuCtx->FSW |= X86_FSW_C1 | X86_FSW_IE | X86_FSW_SF | X86_FSW_ES | X86_FSW_B;
        Log11(("iemFpuStackPushOverflowOnly: %04x:%08RX64: overflow (FSW=%#x)\n",
               pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, pFpuCtx->FSW));
    }
    RT_NOREF(pVCpu);
}


/**
 * Raises a FPU stack overflow exception on a push.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
void iemFpuStackPushOverflow(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);
    iemFpuStackPushOverflowOnly(pVCpu, pFpuCtx);
}


/**
 * Raises a FPU stack overflow exception on a push with a memory operand.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iEffSeg             The effective memory operand selector register.
 * @param   GCPtrEff            The effective memory operand offset.
 */
void iemFpuStackPushOverflowWithMemOp(PVMCPUCC pVCpu, uint8_t iEffSeg, RTGCPTR GCPtrEff) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    iemFpuUpdateDP(pVCpu, pFpuCtx, iEffSeg, GCPtrEff);
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pFpuCtx);
    iemFpuStackPushOverflowOnly(pVCpu, pFpuCtx);
}

/** @}  */


/** @name   SSE+AVX SIMD access and helpers.
 *
 * @{
 */
/**
 * Stores a result in a SIMD XMM register, updates the MXCSR.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pResult             The result to store.
 * @param   iXmmReg             Which SIMD XMM register to store the result in.
 */
void iemSseStoreResult(PVMCPUCC pVCpu, PCIEMSSERESULT pResult, uint8_t iXmmReg) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    pFpuCtx->MXCSR |= pResult->MXCSR & X86_MXCSR_XCPT_FLAGS;

    /* The result is only updated if there is no unmasked exception pending. */
    if ((  ~((pFpuCtx->MXCSR & X86_MXCSR_XCPT_MASK) >> X86_MXCSR_XCPT_MASK_SHIFT)
         & (pFpuCtx->MXCSR & X86_MXCSR_XCPT_FLAGS)) == 0)
        pVCpu->cpum.GstCtx.XState.x87.aXMM[iXmmReg] = pResult->uResult;
}


/**
 * Updates the MXCSR.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   fMxcsr              The new MXCSR value.
 */
void iemSseUpdateMxcsr(PVMCPUCC pVCpu, uint32_t fMxcsr) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    pFpuCtx->MXCSR |= fMxcsr & X86_MXCSR_XCPT_FLAGS;
}
/** @}  */


/** @name   Memory access.
 *
 * @{
 */


/**
 * Updates the IEMCPU::cbWritten counter if applicable.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   fAccess             The access being accounted for.
 * @param   cbMem               The access size.
 */
DECL_FORCE_INLINE(void) iemMemUpdateWrittenCounter(PVMCPUCC pVCpu, uint32_t fAccess, size_t cbMem)
{
    if (   (fAccess & (IEM_ACCESS_WHAT_MASK | IEM_ACCESS_TYPE_WRITE)) == (IEM_ACCESS_WHAT_STACK | IEM_ACCESS_TYPE_WRITE)
        || (fAccess & (IEM_ACCESS_WHAT_MASK | IEM_ACCESS_TYPE_WRITE)) == (IEM_ACCESS_WHAT_DATA  | IEM_ACCESS_TYPE_WRITE) )
        pVCpu->iem.s.cbWritten += (uint32_t)cbMem;
}


/**
 * Applies the segment limit, base and attributes.
 *
 * This may raise a \#GP or \#SS.
 *
 * @returns VBox strict status code.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   fAccess             The kind of access which is being performed.
 * @param   iSegReg             The index of the segment register to apply.
 *                              This is UINT8_MAX if none (for IDT, GDT, LDT,
 *                              TSS, ++).
 * @param   cbMem               The access size.
 * @param   pGCPtrMem           Pointer to the guest memory address to apply
 *                              segmentation to.  Input and output parameter.
 */
VBOXSTRICTRC iemMemApplySegment(PVMCPUCC pVCpu, uint32_t fAccess, uint8_t iSegReg, size_t cbMem, PRTGCPTR pGCPtrMem) RT_NOEXCEPT
{
    if (iSegReg == UINT8_MAX)
        return VINF_SUCCESS;

    IEM_CTX_IMPORT_RET(pVCpu, CPUMCTX_EXTRN_SREG_FROM_IDX(iSegReg));
    PCPUMSELREGHID pSel = iemSRegGetHid(pVCpu, iSegReg);
    switch (pVCpu->iem.s.enmCpuMode)
    {
        case IEMMODE_16BIT:
        case IEMMODE_32BIT:
        {
            RTGCPTR32 GCPtrFirst32 = (RTGCPTR32)*pGCPtrMem;
            RTGCPTR32 GCPtrLast32  = GCPtrFirst32 + (uint32_t)cbMem - 1;

            if (   pSel->Attr.n.u1Present
                && !pSel->Attr.n.u1Unusable)
            {
                Assert(pSel->Attr.n.u1DescType);
                if (!(pSel->Attr.n.u4Type & X86_SEL_TYPE_CODE))
                {
                    if (   (fAccess & IEM_ACCESS_TYPE_WRITE)
                        && !(pSel->Attr.n.u4Type & X86_SEL_TYPE_WRITE) )
                        return iemRaiseSelectorInvalidAccess(pVCpu, iSegReg, fAccess);

                    if (!IEM_IS_REAL_OR_V86_MODE(pVCpu))
                    {
                        /** @todo CPL check. */
                    }

                    /*
                     * There are two kinds of data selectors, normal and expand down.
                     */
                    if (!(pSel->Attr.n.u4Type & X86_SEL_TYPE_DOWN))
                    {
                        if (   GCPtrFirst32 > pSel->u32Limit
                            || GCPtrLast32  > pSel->u32Limit) /* yes, in real mode too (since 80286). */
                            return iemRaiseSelectorBounds(pVCpu, iSegReg, fAccess);
                    }
                    else
                    {
                       /*
                        * The upper boundary is defined by the B bit, not the G bit!
                        */
                       if (   GCPtrFirst32 < pSel->u32Limit + UINT32_C(1)
                           || GCPtrLast32  > (pSel->Attr.n.u1DefBig ? UINT32_MAX : UINT32_C(0xffff)))
                          return iemRaiseSelectorBounds(pVCpu, iSegReg, fAccess);
                    }
                    *pGCPtrMem = GCPtrFirst32 += (uint32_t)pSel->u64Base;
                }
                else
                {
                    /*
                     * Code selector and usually be used to read thru, writing is
                     * only permitted in real and V8086 mode.
                     */
                    if (   (   (fAccess & IEM_ACCESS_TYPE_WRITE)
                            || (   (fAccess & IEM_ACCESS_TYPE_READ)
                               && !(pSel->Attr.n.u4Type & X86_SEL_TYPE_READ)) )
                        && !IEM_IS_REAL_OR_V86_MODE(pVCpu) )
                        return iemRaiseSelectorInvalidAccess(pVCpu, iSegReg, fAccess);

                    if (   GCPtrFirst32 > pSel->u32Limit
                        || GCPtrLast32  > pSel->u32Limit) /* yes, in real mode too (since 80286). */
                        return iemRaiseSelectorBounds(pVCpu, iSegReg, fAccess);

                    if (!IEM_IS_REAL_OR_V86_MODE(pVCpu))
                    {
                        /** @todo CPL check. */
                    }

                    *pGCPtrMem  = GCPtrFirst32 += (uint32_t)pSel->u64Base;
                }
            }
            else
                return iemRaiseGeneralProtectionFault0(pVCpu);
            return VINF_SUCCESS;
        }

        case IEMMODE_64BIT:
        {
            RTGCPTR GCPtrMem = *pGCPtrMem;
            if (iSegReg == X86_SREG_GS || iSegReg == X86_SREG_FS)
                *pGCPtrMem = GCPtrMem + pSel->u64Base;

            Assert(cbMem >= 1);
            if (RT_LIKELY(X86_IS_CANONICAL(GCPtrMem) && X86_IS_CANONICAL(GCPtrMem + cbMem - 1)))
                return VINF_SUCCESS;
            /** @todo We should probably raise \#SS(0) here if segment is SS; see AMD spec.
             *        4.12.2 "Data Limit Checks in 64-bit Mode". */
            return iemRaiseGeneralProtectionFault0(pVCpu);
        }

        default:
            AssertFailedReturn(VERR_IEM_IPE_7);
    }
}


/**
 * Translates a virtual address to a physical physical address and checks if we
 * can access the page as specified.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   GCPtrMem            The virtual address.
 * @param   cbAccess            The access size, for raising \#PF correctly for
 *                              FXSAVE and such.
 * @param   fAccess             The intended access.
 * @param   pGCPhysMem          Where to return the physical address.
 */
VBOXSTRICTRC iemMemPageTranslateAndCheckAccess(PVMCPUCC pVCpu, RTGCPTR GCPtrMem, uint32_t cbAccess,
                                               uint32_t fAccess, PRTGCPHYS pGCPhysMem) RT_NOEXCEPT
{
    /** @todo Need a different PGM interface here.  We're currently using
     *        generic / REM interfaces. this won't cut it for R0. */
    /** @todo If/when PGM handles paged real-mode, we can remove the hack in
     *        iemSvmWorldSwitch/iemVmxWorldSwitch to work around raising a page-fault
     *        here. */
    PGMPTWALK Walk;
    int rc = PGMGstGetPage(pVCpu, GCPtrMem, &Walk);
    if (RT_FAILURE(rc))
    {
        Log(("iemMemPageTranslateAndCheckAccess: GCPtrMem=%RGv - failed to fetch page -> #PF\n", GCPtrMem));
        /** @todo Check unassigned memory in unpaged mode. */
        /** @todo Reserved bits in page tables. Requires new PGM interface. */
#ifdef VBOX_WITH_NESTED_HWVIRT_VMX_EPT
        if (Walk.fFailed & PGM_WALKFAIL_EPT)
            IEM_VMX_VMEXIT_EPT_RET(pVCpu, &Walk, fAccess, IEM_SLAT_FAIL_LINEAR_TO_PHYS_ADDR, 0 /* cbInstr */);
#endif
        *pGCPhysMem = NIL_RTGCPHYS;
        return iemRaisePageFault(pVCpu, GCPtrMem, cbAccess, fAccess, rc);
    }

    /* If the page is writable and does not have the no-exec bit set, all
       access is allowed.  Otherwise we'll have to check more carefully... */
    if ((Walk.fEffective & (X86_PTE_RW | X86_PTE_US | X86_PTE_PAE_NX)) != (X86_PTE_RW | X86_PTE_US))
    {
        /* Write to read only memory? */
        if (   (fAccess & IEM_ACCESS_TYPE_WRITE)
            && !(Walk.fEffective & X86_PTE_RW)
            && (   (    pVCpu->iem.s.uCpl == 3
                    && !(fAccess & IEM_ACCESS_WHAT_SYS))
                || (pVCpu->cpum.GstCtx.cr0 & X86_CR0_WP)))
        {
            Log(("iemMemPageTranslateAndCheckAccess: GCPtrMem=%RGv - read-only page -> #PF\n", GCPtrMem));
            *pGCPhysMem = NIL_RTGCPHYS;
#ifdef VBOX_WITH_NESTED_HWVIRT_VMX_EPT
            if (Walk.fFailed & PGM_WALKFAIL_EPT)
                IEM_VMX_VMEXIT_EPT_RET(pVCpu, &Walk, fAccess, IEM_SLAT_FAIL_LINEAR_TO_PAGE_TABLE, 0 /* cbInstr */);
#endif
            return iemRaisePageFault(pVCpu, GCPtrMem, cbAccess, fAccess & ~IEM_ACCESS_TYPE_READ, VERR_ACCESS_DENIED);
        }

        /* Kernel memory accessed by userland? */
        if (   !(Walk.fEffective & X86_PTE_US)
            && pVCpu->iem.s.uCpl == 3
            && !(fAccess & IEM_ACCESS_WHAT_SYS))
        {
            Log(("iemMemPageTranslateAndCheckAccess: GCPtrMem=%RGv - user access to kernel page -> #PF\n", GCPtrMem));
            *pGCPhysMem = NIL_RTGCPHYS;
#ifdef VBOX_WITH_NESTED_HWVIRT_VMX_EPT
            if (Walk.fFailed & PGM_WALKFAIL_EPT)
                IEM_VMX_VMEXIT_EPT_RET(pVCpu, &Walk, fAccess, IEM_SLAT_FAIL_LINEAR_TO_PAGE_TABLE, 0 /* cbInstr */);
#endif
            return iemRaisePageFault(pVCpu, GCPtrMem, cbAccess, fAccess, VERR_ACCESS_DENIED);
        }

        /* Executing non-executable memory? */
        if (   (fAccess & IEM_ACCESS_TYPE_EXEC)
            && (Walk.fEffective & X86_PTE_PAE_NX)
            && (pVCpu->cpum.GstCtx.msrEFER & MSR_K6_EFER_NXE) )
        {
            Log(("iemMemPageTranslateAndCheckAccess: GCPtrMem=%RGv - NX -> #PF\n", GCPtrMem));
            *pGCPhysMem = NIL_RTGCPHYS;
#ifdef VBOX_WITH_NESTED_HWVIRT_VMX_EPT
            if (Walk.fFailed & PGM_WALKFAIL_EPT)
                IEM_VMX_VMEXIT_EPT_RET(pVCpu, &Walk, fAccess, IEM_SLAT_FAIL_LINEAR_TO_PAGE_TABLE, 0 /* cbInstr */);
#endif
            return iemRaisePageFault(pVCpu, GCPtrMem, cbAccess, fAccess & ~(IEM_ACCESS_TYPE_READ | IEM_ACCESS_TYPE_WRITE),
                                     VERR_ACCESS_DENIED);
        }
    }

    /*
     * Set the dirty / access flags.
     * ASSUMES this is set when the address is translated rather than on committ...
     */
    /** @todo testcase: check when A and D bits are actually set by the CPU.  */
    uint32_t fAccessedDirty = fAccess & IEM_ACCESS_TYPE_WRITE ? X86_PTE_D | X86_PTE_A : X86_PTE_A;
    if ((Walk.fEffective & fAccessedDirty) != fAccessedDirty)
    {
        int rc2 = PGMGstModifyPage(pVCpu, GCPtrMem, 1, fAccessedDirty, ~(uint64_t)fAccessedDirty);
        AssertRC(rc2);
        /** @todo Nested VMX: Accessed/dirty bit currently not supported, asserted below. */
        Assert(!(CPUMGetGuestIa32VmxEptVpidCap(pVCpu) & VMX_BF_EPT_VPID_CAP_ACCESS_DIRTY_MASK));
    }

    RTGCPHYS const GCPhys = Walk.GCPhys | (GCPtrMem & GUEST_PAGE_OFFSET_MASK);
    *pGCPhysMem = GCPhys;
    return VINF_SUCCESS;
}


/**
 * Looks up a memory mapping entry.
 *
 * @returns The mapping index (positive) or VERR_NOT_FOUND (negative).
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   pvMem           The memory address.
 * @param   fAccess         The access to.
 */
DECLINLINE(int) iemMapLookup(PVMCPUCC pVCpu, void *pvMem, uint32_t fAccess)
{
    Assert(pVCpu->iem.s.cActiveMappings <= RT_ELEMENTS(pVCpu->iem.s.aMemMappings));
    fAccess &= IEM_ACCESS_WHAT_MASK | IEM_ACCESS_TYPE_MASK;
    if (   pVCpu->iem.s.aMemMappings[0].pv == pvMem
        && (pVCpu->iem.s.aMemMappings[0].fAccess & (IEM_ACCESS_WHAT_MASK | IEM_ACCESS_TYPE_MASK)) == fAccess)
        return 0;
    if (   pVCpu->iem.s.aMemMappings[1].pv == pvMem
        && (pVCpu->iem.s.aMemMappings[1].fAccess & (IEM_ACCESS_WHAT_MASK | IEM_ACCESS_TYPE_MASK)) == fAccess)
        return 1;
    if (   pVCpu->iem.s.aMemMappings[2].pv == pvMem
        && (pVCpu->iem.s.aMemMappings[2].fAccess & (IEM_ACCESS_WHAT_MASK | IEM_ACCESS_TYPE_MASK)) == fAccess)
        return 2;
    return VERR_NOT_FOUND;
}


/**
 * Finds a free memmap entry when using iNextMapping doesn't work.
 *
 * @returns Memory mapping index, 1024 on failure.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
static unsigned iemMemMapFindFree(PVMCPUCC pVCpu)
{
    /*
     * The easy case.
     */
    if (pVCpu->iem.s.cActiveMappings == 0)
    {
        pVCpu->iem.s.iNextMapping = 1;
        return 0;
    }

    /* There should be enough mappings for all instructions. */
    AssertReturn(pVCpu->iem.s.cActiveMappings < RT_ELEMENTS(pVCpu->iem.s.aMemMappings), 1024);

    for (unsigned i = 0; i < RT_ELEMENTS(pVCpu->iem.s.aMemMappings); i++)
        if (pVCpu->iem.s.aMemMappings[i].fAccess == IEM_ACCESS_INVALID)
            return i;

    AssertFailedReturn(1024);
}


/**
 * Commits a bounce buffer that needs writing back and unmaps it.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   iMemMap         The index of the buffer to commit.
 * @param   fPostponeFail   Whether we can postpone writer failures to ring-3.
 *                          Always false in ring-3, obviously.
 */
static VBOXSTRICTRC iemMemBounceBufferCommitAndUnmap(PVMCPUCC pVCpu, unsigned iMemMap, bool fPostponeFail)
{
    Assert(pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_BOUNCE_BUFFERED);
    Assert(pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_TYPE_WRITE);
#ifdef IN_RING3
    Assert(!fPostponeFail);
    RT_NOREF_PV(fPostponeFail);
#endif

    /*
     * Do the writing.
     */
    PVMCC pVM = pVCpu->CTX_SUFF(pVM);
    if (!pVCpu->iem.s.aMemBbMappings[iMemMap].fUnassigned)
    {
        uint16_t const  cbFirst  = pVCpu->iem.s.aMemBbMappings[iMemMap].cbFirst;
        uint16_t const  cbSecond = pVCpu->iem.s.aMemBbMappings[iMemMap].cbSecond;
        uint8_t const  *pbBuf    = &pVCpu->iem.s.aBounceBuffers[iMemMap].ab[0];
        if (!pVCpu->iem.s.fBypassHandlers)
        {
            /*
             * Carefully and efficiently dealing with access handler return
             * codes make this a little bloated.
             */
            VBOXSTRICTRC rcStrict = PGMPhysWrite(pVM,
                                                 pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst,
                                                 pbBuf,
                                                 cbFirst,
                                                 PGMACCESSORIGIN_IEM);
            if (rcStrict == VINF_SUCCESS)
            {
                if (cbSecond)
                {
                    rcStrict = PGMPhysWrite(pVM,
                                            pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond,
                                            pbBuf + cbFirst,
                                            cbSecond,
                                            PGMACCESSORIGIN_IEM);
                    if (rcStrict == VINF_SUCCESS)
                    { /* nothing */ }
                    else if (PGM_PHYS_RW_IS_SUCCESS(rcStrict))
                    {
                        Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysWrite GCPhysFirst=%RGp/%#x GCPhysSecond=%RGp/%#x %Rrc\n",
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst,
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond, VBOXSTRICTRC_VAL(rcStrict) ));
                        rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                    }
#ifndef IN_RING3
                    else if (fPostponeFail)
                    {
                        Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysWrite GCPhysFirst=%RGp/%#x GCPhysSecond=%RGp/%#x %Rrc (postponed)\n",
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst,
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond, VBOXSTRICTRC_VAL(rcStrict) ));
                        pVCpu->iem.s.aMemMappings[iMemMap].fAccess |= IEM_ACCESS_PENDING_R3_WRITE_2ND;
                        VMCPU_FF_SET(pVCpu, VMCPU_FF_IEM);
                        return iemSetPassUpStatus(pVCpu, rcStrict);
                    }
#endif
                    else
                    {
                        Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysWrite GCPhysFirst=%RGp/%#x GCPhysSecond=%RGp/%#x %Rrc (!!)\n",
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst,
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond, VBOXSTRICTRC_VAL(rcStrict) ));
                        return rcStrict;
                    }
                }
            }
            else if (PGM_PHYS_RW_IS_SUCCESS(rcStrict))
            {
                if (!cbSecond)
                {
                    Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysWrite GCPhysFirst=%RGp/%#x %Rrc\n",
                         pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst, VBOXSTRICTRC_VAL(rcStrict) ));
                    rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                }
                else
                {
                    VBOXSTRICTRC rcStrict2 = PGMPhysWrite(pVM,
                                                          pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond,
                                                          pbBuf + cbFirst,
                                                          cbSecond,
                                                          PGMACCESSORIGIN_IEM);
                    if (rcStrict2 == VINF_SUCCESS)
                    {
                        Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysWrite GCPhysFirst=%RGp/%#x %Rrc GCPhysSecond=%RGp/%#x\n",
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst, VBOXSTRICTRC_VAL(rcStrict),
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond));
                        rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                    }
                    else if (PGM_PHYS_RW_IS_SUCCESS(rcStrict2))
                    {
                        Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysWrite GCPhysFirst=%RGp/%#x %Rrc GCPhysSecond=%RGp/%#x %Rrc\n",
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst, VBOXSTRICTRC_VAL(rcStrict),
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond, VBOXSTRICTRC_VAL(rcStrict2) ));
                        PGM_PHYS_RW_DO_UPDATE_STRICT_RC(rcStrict, rcStrict2);
                        rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                    }
#ifndef IN_RING3
                    else if (fPostponeFail)
                    {
                        Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysWrite GCPhysFirst=%RGp/%#x GCPhysSecond=%RGp/%#x %Rrc (postponed)\n",
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst,
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond, VBOXSTRICTRC_VAL(rcStrict) ));
                        pVCpu->iem.s.aMemMappings[iMemMap].fAccess |= IEM_ACCESS_PENDING_R3_WRITE_2ND;
                        VMCPU_FF_SET(pVCpu, VMCPU_FF_IEM);
                        return iemSetPassUpStatus(pVCpu, rcStrict);
                    }
#endif
                    else
                    {
                        Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysWrite GCPhysFirst=%RGp/%#x %Rrc GCPhysSecond=%RGp/%#x %Rrc (!!)\n",
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst, VBOXSTRICTRC_VAL(rcStrict),
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond, VBOXSTRICTRC_VAL(rcStrict2) ));
                        return rcStrict2;
                    }
                }
            }
#ifndef IN_RING3
            else if (fPostponeFail)
            {
                Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysWrite GCPhysFirst=%RGp/%#x GCPhysSecond=%RGp/%#x %Rrc (postponed)\n",
                     pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst,
                     pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond, VBOXSTRICTRC_VAL(rcStrict) ));
                if (!cbSecond)
                    pVCpu->iem.s.aMemMappings[iMemMap].fAccess |= IEM_ACCESS_PENDING_R3_WRITE_1ST;
                else
                    pVCpu->iem.s.aMemMappings[iMemMap].fAccess |= IEM_ACCESS_PENDING_R3_WRITE_1ST | IEM_ACCESS_PENDING_R3_WRITE_2ND;
                VMCPU_FF_SET(pVCpu, VMCPU_FF_IEM);
                return iemSetPassUpStatus(pVCpu, rcStrict);
            }
#endif
            else
            {
                Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysWrite GCPhysFirst=%RGp/%#x %Rrc [GCPhysSecond=%RGp/%#x] (!!)\n",
                     pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst, VBOXSTRICTRC_VAL(rcStrict),
                     pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond));
                return rcStrict;
            }
        }
        else
        {
            /*
             * No access handlers, much simpler.
             */
            int rc = PGMPhysSimpleWriteGCPhys(pVM, pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, pbBuf, cbFirst);
            if (RT_SUCCESS(rc))
            {
                if (cbSecond)
                {
                    rc = PGMPhysSimpleWriteGCPhys(pVM, pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, pbBuf + cbFirst, cbSecond);
                    if (RT_SUCCESS(rc))
                    { /* likely */ }
                    else
                    {
                        Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysSimpleWriteGCPhys GCPhysFirst=%RGp/%#x GCPhysSecond=%RGp/%#x %Rrc (!!)\n",
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst,
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond, rc));
                        return rc;
                    }
                }
            }
            else
            {
                Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysSimpleWriteGCPhys GCPhysFirst=%RGp/%#x %Rrc [GCPhysSecond=%RGp/%#x] (!!)\n",
                     pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst, rc,
                     pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond));
                return rc;
            }
        }
    }

#if defined(IEM_LOG_MEMORY_WRITES)
    Log(("IEM Wrote %RGp: %.*Rhxs\n", pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst,
         RT_MAX(RT_MIN(pVCpu->iem.s.aMemBbMappings[iMemMap].cbFirst, 64), 1), &pVCpu->iem.s.aBounceBuffers[iMemMap].ab[0]));
    if (pVCpu->iem.s.aMemBbMappings[iMemMap].cbSecond)
        Log(("IEM Wrote %RGp: %.*Rhxs [2nd page]\n", pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond,
             RT_MIN(pVCpu->iem.s.aMemBbMappings[iMemMap].cbSecond, 64),
             &pVCpu->iem.s.aBounceBuffers[iMemMap].ab[pVCpu->iem.s.aMemBbMappings[iMemMap].cbFirst]));

    size_t cbWrote = pVCpu->iem.s.aMemBbMappings[iMemMap].cbFirst + pVCpu->iem.s.aMemBbMappings[iMemMap].cbSecond;
    g_cbIemWrote = cbWrote;
    memcpy(g_abIemWrote, &pVCpu->iem.s.aBounceBuffers[iMemMap].ab[0], RT_MIN(cbWrote, sizeof(g_abIemWrote)));
#endif

    /*
     * Free the mapping entry.
     */
    pVCpu->iem.s.aMemMappings[iMemMap].fAccess = IEM_ACCESS_INVALID;
    Assert(pVCpu->iem.s.cActiveMappings != 0);
    pVCpu->iem.s.cActiveMappings--;
    return VINF_SUCCESS;
}


/**
 * iemMemMap worker that deals with a request crossing pages.
 */
static VBOXSTRICTRC
iemMemBounceBufferMapCrossPage(PVMCPUCC pVCpu, int iMemMap, void **ppvMem, size_t cbMem, RTGCPTR GCPtrFirst, uint32_t fAccess)
{
    Assert(cbMem <= GUEST_PAGE_SIZE);

    /*
     * Do the address translations.
     */
    uint32_t const cbFirstPage  = GUEST_PAGE_SIZE - (uint32_t)(GCPtrFirst & GUEST_PAGE_OFFSET_MASK);
    RTGCPHYS GCPhysFirst;
    VBOXSTRICTRC rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, GCPtrFirst, cbFirstPage, fAccess, &GCPhysFirst);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;
    Assert((GCPhysFirst & GUEST_PAGE_OFFSET_MASK) == (GCPtrFirst & GUEST_PAGE_OFFSET_MASK));

    uint32_t const cbSecondPage = (uint32_t)cbMem - cbFirstPage;
    RTGCPHYS GCPhysSecond;
    rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, (GCPtrFirst + (cbMem - 1)) & ~(RTGCPTR)GUEST_PAGE_OFFSET_MASK,
                                                 cbSecondPage, fAccess, &GCPhysSecond);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;
    Assert((GCPhysSecond & GUEST_PAGE_OFFSET_MASK) == 0);
    GCPhysSecond &= ~(RTGCPHYS)GUEST_PAGE_OFFSET_MASK; /** @todo why? */

    PVMCC pVM = pVCpu->CTX_SUFF(pVM);

    /*
     * Read in the current memory content if it's a read, execute or partial
     * write access.
     */
    uint8_t * const pbBuf = &pVCpu->iem.s.aBounceBuffers[iMemMap].ab[0];

    if (fAccess & (IEM_ACCESS_TYPE_READ | IEM_ACCESS_TYPE_EXEC | IEM_ACCESS_PARTIAL_WRITE))
    {
        if (!pVCpu->iem.s.fBypassHandlers)
        {
            /*
             * Must carefully deal with access handler status codes here,
             * makes the code a bit bloated.
             */
            rcStrict = PGMPhysRead(pVM, GCPhysFirst, pbBuf, cbFirstPage, PGMACCESSORIGIN_IEM);
            if (rcStrict == VINF_SUCCESS)
            {
                rcStrict = PGMPhysRead(pVM, GCPhysSecond, pbBuf + cbFirstPage, cbSecondPage, PGMACCESSORIGIN_IEM);
                if (rcStrict == VINF_SUCCESS)
                { /*likely */ }
                else if (PGM_PHYS_RW_IS_SUCCESS(rcStrict))
                    rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                else
                {
                    Log(("iemMemBounceBufferMapPhys: PGMPhysRead GCPhysSecond=%RGp rcStrict2=%Rrc (!!)\n",
                         GCPhysSecond, VBOXSTRICTRC_VAL(rcStrict) ));
                    return rcStrict;
                }
            }
            else if (PGM_PHYS_RW_IS_SUCCESS(rcStrict))
            {
                VBOXSTRICTRC rcStrict2 = PGMPhysRead(pVM, GCPhysSecond, pbBuf + cbFirstPage, cbSecondPage, PGMACCESSORIGIN_IEM);
                if (PGM_PHYS_RW_IS_SUCCESS(rcStrict2))
                {
                    PGM_PHYS_RW_DO_UPDATE_STRICT_RC(rcStrict, rcStrict2);
                    rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                }
                else
                {
                    Log(("iemMemBounceBufferMapPhys: PGMPhysRead GCPhysSecond=%RGp rcStrict2=%Rrc (rcStrict=%Rrc) (!!)\n",
                         GCPhysSecond, VBOXSTRICTRC_VAL(rcStrict2), VBOXSTRICTRC_VAL(rcStrict2) ));
                    return rcStrict2;
                }
            }
            else
            {
                Log(("iemMemBounceBufferMapPhys: PGMPhysRead GCPhysFirst=%RGp rcStrict=%Rrc (!!)\n",
                     GCPhysFirst, VBOXSTRICTRC_VAL(rcStrict) ));
                return rcStrict;
            }
        }
        else
        {
            /*
             * No informational status codes here, much more straight forward.
             */
            int rc = PGMPhysSimpleReadGCPhys(pVM, pbBuf, GCPhysFirst, cbFirstPage);
            if (RT_SUCCESS(rc))
            {
                Assert(rc == VINF_SUCCESS);
                rc = PGMPhysSimpleReadGCPhys(pVM, pbBuf + cbFirstPage, GCPhysSecond, cbSecondPage);
                if (RT_SUCCESS(rc))
                    Assert(rc == VINF_SUCCESS);
                else
                {
                    Log(("iemMemBounceBufferMapPhys: PGMPhysSimpleReadGCPhys GCPhysSecond=%RGp rc=%Rrc (!!)\n", GCPhysSecond, rc));
                    return rc;
                }
            }
            else
            {
                Log(("iemMemBounceBufferMapPhys: PGMPhysSimpleReadGCPhys GCPhysFirst=%RGp rc=%Rrc (!!)\n", GCPhysFirst, rc));
                return rc;
            }
        }
    }
#ifdef VBOX_STRICT
    else
        memset(pbBuf, 0xcc, cbMem);
    if (cbMem < sizeof(pVCpu->iem.s.aBounceBuffers[iMemMap].ab))
        memset(pbBuf + cbMem, 0xaa, sizeof(pVCpu->iem.s.aBounceBuffers[iMemMap].ab) - cbMem);
#endif
    AssertCompileMemberAlignment(VMCPU, iem.s.aBounceBuffers, 64);

    /*
     * Commit the bounce buffer entry.
     */
    pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst    = GCPhysFirst;
    pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond   = GCPhysSecond;
    pVCpu->iem.s.aMemBbMappings[iMemMap].cbFirst        = (uint16_t)cbFirstPage;
    pVCpu->iem.s.aMemBbMappings[iMemMap].cbSecond       = (uint16_t)cbSecondPage;
    pVCpu->iem.s.aMemBbMappings[iMemMap].fUnassigned    = false;
    pVCpu->iem.s.aMemMappings[iMemMap].pv               = pbBuf;
    pVCpu->iem.s.aMemMappings[iMemMap].fAccess          = fAccess | IEM_ACCESS_BOUNCE_BUFFERED;
    pVCpu->iem.s.iNextMapping = iMemMap + 1;
    pVCpu->iem.s.cActiveMappings++;

    iemMemUpdateWrittenCounter(pVCpu, fAccess, cbMem);
    *ppvMem = pbBuf;
    return VINF_SUCCESS;
}


/**
 * iemMemMap woker that deals with iemMemPageMap failures.
 */
static VBOXSTRICTRC iemMemBounceBufferMapPhys(PVMCPUCC pVCpu, unsigned iMemMap, void **ppvMem, size_t cbMem,
                                              RTGCPHYS GCPhysFirst, uint32_t fAccess, VBOXSTRICTRC rcMap)
{
    /*
     * Filter out conditions we can handle and the ones which shouldn't happen.
     */
    if (   rcMap != VERR_PGM_PHYS_TLB_CATCH_WRITE
        && rcMap != VERR_PGM_PHYS_TLB_CATCH_ALL
        && rcMap != VERR_PGM_PHYS_TLB_UNASSIGNED)
    {
        AssertReturn(RT_FAILURE_NP(rcMap), VERR_IEM_IPE_8);
        return rcMap;
    }
    pVCpu->iem.s.cPotentialExits++;

    /*
     * Read in the current memory content if it's a read, execute or partial
     * write access.
     */
    uint8_t *pbBuf = &pVCpu->iem.s.aBounceBuffers[iMemMap].ab[0];
    if (fAccess & (IEM_ACCESS_TYPE_READ | IEM_ACCESS_TYPE_EXEC | IEM_ACCESS_PARTIAL_WRITE))
    {
        if (rcMap == VERR_PGM_PHYS_TLB_UNASSIGNED)
            memset(pbBuf, 0xff, cbMem);
        else
        {
            int rc;
            if (!pVCpu->iem.s.fBypassHandlers)
            {
                VBOXSTRICTRC rcStrict = PGMPhysRead(pVCpu->CTX_SUFF(pVM), GCPhysFirst, pbBuf, cbMem, PGMACCESSORIGIN_IEM);
                if (rcStrict == VINF_SUCCESS)
                { /* nothing */ }
                else if (PGM_PHYS_RW_IS_SUCCESS(rcStrict))
                    rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                else
                {
                    Log(("iemMemBounceBufferMapPhys: PGMPhysRead GCPhysFirst=%RGp rcStrict=%Rrc (!!)\n",
                         GCPhysFirst, VBOXSTRICTRC_VAL(rcStrict) ));
                    return rcStrict;
                }
            }
            else
            {
                rc = PGMPhysSimpleReadGCPhys(pVCpu->CTX_SUFF(pVM), pbBuf, GCPhysFirst, cbMem);
                if (RT_SUCCESS(rc))
                { /* likely */ }
                else
                {
                    Log(("iemMemBounceBufferMapPhys: PGMPhysSimpleReadGCPhys GCPhysFirst=%RGp rcStrict=%Rrc (!!)\n",
                         GCPhysFirst, rc));
                    return rc;
                }
            }
        }
    }
#ifdef VBOX_STRICT
    else
        memset(pbBuf, 0xcc, cbMem);
#endif
#ifdef VBOX_STRICT
    if (cbMem < sizeof(pVCpu->iem.s.aBounceBuffers[iMemMap].ab))
        memset(pbBuf + cbMem, 0xaa, sizeof(pVCpu->iem.s.aBounceBuffers[iMemMap].ab) - cbMem);
#endif

    /*
     * Commit the bounce buffer entry.
     */
    pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst    = GCPhysFirst;
    pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond   = NIL_RTGCPHYS;
    pVCpu->iem.s.aMemBbMappings[iMemMap].cbFirst        = (uint16_t)cbMem;
    pVCpu->iem.s.aMemBbMappings[iMemMap].cbSecond       = 0;
    pVCpu->iem.s.aMemBbMappings[iMemMap].fUnassigned    = rcMap == VERR_PGM_PHYS_TLB_UNASSIGNED;
    pVCpu->iem.s.aMemMappings[iMemMap].pv               = pbBuf;
    pVCpu->iem.s.aMemMappings[iMemMap].fAccess          = fAccess | IEM_ACCESS_BOUNCE_BUFFERED;
    pVCpu->iem.s.iNextMapping = iMemMap + 1;
    pVCpu->iem.s.cActiveMappings++;

    iemMemUpdateWrittenCounter(pVCpu, fAccess, cbMem);
    *ppvMem = pbBuf;
    return VINF_SUCCESS;
}



/**
 * Maps the specified guest memory for the given kind of access.
 *
 * This may be using bounce buffering of the memory if it's crossing a page
 * boundary or if there is an access handler installed for any of it.  Because
 * of lock prefix guarantees, we're in for some extra clutter when this
 * happens.
 *
 * This may raise a \#GP, \#SS, \#PF or \#AC.
 *
 * @returns VBox strict status code.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling thread.
 * @param   ppvMem      Where to return the pointer to the mapped memory.
 * @param   cbMem       The number of bytes to map.  This is usually 1, 2, 4, 6,
 *                      8, 12, 16, 32 or 512.  When used by string operations
 *                      it can be up to a page.
 * @param   iSegReg     The index of the segment register to use for this
 *                      access.  The base and limits are checked. Use UINT8_MAX
 *                      to indicate that no segmentation is required (for IDT,
 *                      GDT and LDT accesses).
 * @param   GCPtrMem    The address of the guest memory.
 * @param   fAccess     How the memory is being accessed.  The
 *                      IEM_ACCESS_TYPE_XXX bit is used to figure out how to map
 *                      the memory, while the IEM_ACCESS_WHAT_XXX bit is used
 *                      when raising exceptions.
 * @param   uAlignCtl   Alignment control:
 *                          - Bits 15:0 is the alignment mask.
 *                          - Bits 31:16 for flags like IEM_MEMMAP_F_ALIGN_GP,
 *                            IEM_MEMMAP_F_ALIGN_SSE, and
 *                            IEM_MEMMAP_F_ALIGN_GP_OR_AC.
 *                      Pass zero to skip alignment.
 */
VBOXSTRICTRC iemMemMap(PVMCPUCC pVCpu, void **ppvMem, size_t cbMem, uint8_t iSegReg, RTGCPTR GCPtrMem,
                       uint32_t fAccess, uint32_t uAlignCtl) RT_NOEXCEPT
{
    /*
     * Check the input and figure out which mapping entry to use.
     */
    Assert(cbMem <= sizeof(pVCpu->iem.s.aBounceBuffers[0]));
    Assert(   cbMem <= 64 || cbMem == 512 || cbMem == 256 || cbMem == 108 || cbMem == 104 || cbMem == 102 || cbMem == 94
           || (iSegReg == UINT8_MAX && uAlignCtl == 0 && fAccess == IEM_ACCESS_DATA_R /* for the CPUID logging interface */) );
    Assert(~(fAccess & ~(IEM_ACCESS_TYPE_MASK | IEM_ACCESS_WHAT_MASK)));
    Assert(pVCpu->iem.s.cActiveMappings < RT_ELEMENTS(pVCpu->iem.s.aMemMappings));

    unsigned iMemMap = pVCpu->iem.s.iNextMapping;
    if (   iMemMap >= RT_ELEMENTS(pVCpu->iem.s.aMemMappings)
        || pVCpu->iem.s.aMemMappings[iMemMap].fAccess != IEM_ACCESS_INVALID)
    {
        iMemMap = iemMemMapFindFree(pVCpu);
        AssertLogRelMsgReturn(iMemMap < RT_ELEMENTS(pVCpu->iem.s.aMemMappings),
                              ("active=%d fAccess[0] = {%#x, %#x, %#x}\n", pVCpu->iem.s.cActiveMappings,
                               pVCpu->iem.s.aMemMappings[0].fAccess, pVCpu->iem.s.aMemMappings[1].fAccess,
                               pVCpu->iem.s.aMemMappings[2].fAccess),
                              VERR_IEM_IPE_9);
    }

    /*
     * Map the memory, checking that we can actually access it.  If something
     * slightly complicated happens, fall back on bounce buffering.
     */
    VBOXSTRICTRC rcStrict = iemMemApplySegment(pVCpu, fAccess, iSegReg, cbMem, &GCPtrMem);
    if (rcStrict == VINF_SUCCESS)
    { /* likely */ }
    else
        return rcStrict;

    if ((GCPtrMem & GUEST_PAGE_OFFSET_MASK) + cbMem <= GUEST_PAGE_SIZE) /* Crossing a page boundary? */
    { /* likely */ }
    else
        return iemMemBounceBufferMapCrossPage(pVCpu, iMemMap, ppvMem, cbMem, GCPtrMem, fAccess);

    /*
     * Alignment check.
     */
    if ( (GCPtrMem & (uAlignCtl & UINT16_MAX)) == 0 )
    { /* likelyish */ }
    else
    {
        /* Misaligned access. */
        if ((fAccess & IEM_ACCESS_WHAT_MASK) != IEM_ACCESS_WHAT_SYS)
        {
            if (   !(uAlignCtl & IEM_MEMMAP_F_ALIGN_GP)
                || (   (uAlignCtl & IEM_MEMMAP_F_ALIGN_SSE)
                    && (pVCpu->cpum.GstCtx.XState.x87.MXCSR & X86_MXCSR_MM)) )
            {
                AssertCompile(X86_CR0_AM == X86_EFL_AC);

                if (iemMemAreAlignmentChecksEnabled(pVCpu))
                    return iemRaiseAlignmentCheckException(pVCpu);
            }
            else if (   (uAlignCtl & IEM_MEMMAP_F_ALIGN_GP_OR_AC)
                     && (GCPtrMem & 3) /* The value 4 matches 10980xe's FXSAVE and helps make bs3-cpu-basic2 work. */
                    /** @todo may only apply to 2, 4 or 8 byte misalignments depending on the CPU
                     * implementation. See FXSAVE/FRSTOR/XSAVE/XRSTOR/++.  Using 4 for now as
                     * that's what FXSAVE does on a 10980xe. */
                     && iemMemAreAlignmentChecksEnabled(pVCpu))
                return iemRaiseAlignmentCheckException(pVCpu);
            else
                return iemRaiseGeneralProtectionFault0(pVCpu);
        }
    }

#ifdef IEM_WITH_DATA_TLB
    Assert(!(fAccess & IEM_ACCESS_TYPE_EXEC));

    /*
     * Get the TLB entry for this page.
     */
    uint64_t const     uTag  = IEMTLB_CALC_TAG(    &pVCpu->iem.s.DataTlb, GCPtrMem);
    PIEMTLBENTRY const pTlbe = IEMTLB_TAG_TO_ENTRY(&pVCpu->iem.s.DataTlb, uTag);
    if (pTlbe->uTag == uTag)
    {
# ifdef VBOX_WITH_STATISTICS
        pVCpu->iem.s.DataTlb.cTlbHits++;
# endif
    }
    else
    {
        pVCpu->iem.s.DataTlb.cTlbMisses++;
        PGMPTWALK Walk;
        int rc = PGMGstGetPage(pVCpu, GCPtrMem, &Walk);
        if (RT_FAILURE(rc))
        {
            Log(("iemMemMap: GCPtrMem=%RGv - failed to fetch page -> #PF\n", GCPtrMem));
# ifdef VBOX_WITH_NESTED_HWVIRT_VMX_EPT
            if (Walk.fFailed & PGM_WALKFAIL_EPT)
                IEM_VMX_VMEXIT_EPT_RET(pVCpu, &Walk, fAccess, IEM_SLAT_FAIL_LINEAR_TO_PHYS_ADDR, 0 /* cbInstr */);
# endif
            return iemRaisePageFault(pVCpu, GCPtrMem, (uint32_t)cbMem, fAccess, rc);
        }

        Assert(Walk.fSucceeded);
        pTlbe->uTag             = uTag;
        pTlbe->fFlagsAndPhysRev = ~Walk.fEffective & (X86_PTE_US | X86_PTE_RW | X86_PTE_D | X86_PTE_A); /* skipping NX */
        pTlbe->GCPhys           = Walk.GCPhys;
        pTlbe->pbMappingR3      = NULL;
    }

    /*
     * Check TLB page table level access flags.
     */
    /* If the page is either supervisor only or non-writable, we need to do
       more careful access checks. */
    if (pTlbe->fFlagsAndPhysRev & (IEMTLBE_F_PT_NO_USER | IEMTLBE_F_PT_NO_WRITE))
    {
        /* Write to read only memory? */
        if (   (pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PT_NO_WRITE)
            && (fAccess & IEM_ACCESS_TYPE_WRITE)
            && (   (    pVCpu->iem.s.uCpl == 3
                    && !(fAccess & IEM_ACCESS_WHAT_SYS))
                || (pVCpu->cpum.GstCtx.cr0 & X86_CR0_WP)))
        {
            Log(("iemMemMap: GCPtrMem=%RGv - read-only page -> #PF\n", GCPtrMem));
# ifdef VBOX_WITH_NESTED_HWVIRT_VMX_EPT
            if (Walk.fFailed & PGM_WALKFAIL_EPT)
                IEM_VMX_VMEXIT_EPT_RET(pVCpu, &Walk, fAccess, IEM_SLAT_FAIL_LINEAR_TO_PAGE_TABLE, 0 /* cbInstr */);
# endif
            return iemRaisePageFault(pVCpu, GCPtrMem, (uint32_t)cbMem, fAccess & ~IEM_ACCESS_TYPE_READ, VERR_ACCESS_DENIED);
        }

        /* Kernel memory accessed by userland? */
        if (   (pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PT_NO_USER)
            && pVCpu->iem.s.uCpl == 3
            && !(fAccess & IEM_ACCESS_WHAT_SYS))
        {
            Log(("iemMemMap: GCPtrMem=%RGv - user access to kernel page -> #PF\n", GCPtrMem));
# ifdef VBOX_WITH_NESTED_HWVIRT_VMX_EPT
            if (Walk.fFailed & PGM_WALKFAIL_EPT)
                IEM_VMX_VMEXIT_EPT_RET(pVCpu, &Walk, fAccess, IEM_SLAT_FAIL_LINEAR_TO_PAGE_TABLE, 0 /* cbInstr */);
# endif
            return iemRaisePageFault(pVCpu, GCPtrMem, (uint32_t)cbMem, fAccess, VERR_ACCESS_DENIED);
        }
    }

    /*
     * Set the dirty / access flags.
     * ASSUMES this is set when the address is translated rather than on commit...
     */
    /** @todo testcase: check when A and D bits are actually set by the CPU.  */
    uint64_t const fTlbAccessedDirty = (fAccess & IEM_ACCESS_TYPE_WRITE ? IEMTLBE_F_PT_NO_DIRTY : 0) | IEMTLBE_F_PT_NO_ACCESSED;
    if (pTlbe->fFlagsAndPhysRev & fTlbAccessedDirty)
    {
        uint32_t const fAccessedDirty = fAccess & IEM_ACCESS_TYPE_WRITE ? X86_PTE_D | X86_PTE_A : X86_PTE_A;
        int rc2 = PGMGstModifyPage(pVCpu, GCPtrMem, 1, fAccessedDirty, ~(uint64_t)fAccessedDirty);
        AssertRC(rc2);
        /** @todo Nested VMX: Accessed/dirty bit currently not supported, asserted below. */
        Assert(!(CPUMGetGuestIa32VmxEptVpidCap(pVCpu) & VMX_BF_EPT_VPID_CAP_ACCESS_DIRTY_MASK));
        pTlbe->fFlagsAndPhysRev &= ~fTlbAccessedDirty;
    }

    /*
     * Look up the physical page info if necessary.
     */
    uint8_t *pbMem = NULL;
    if ((pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PHYS_REV) == pVCpu->iem.s.DataTlb.uTlbPhysRev)
# ifdef IN_RING3
        pbMem = pTlbe->pbMappingR3;
# else
        pbMem = NULL;
# endif
    else
    {
        AssertCompile(PGMIEMGCPHYS2PTR_F_NO_WRITE     == IEMTLBE_F_PG_NO_WRITE);
        AssertCompile(PGMIEMGCPHYS2PTR_F_NO_READ      == IEMTLBE_F_PG_NO_READ);
        AssertCompile(PGMIEMGCPHYS2PTR_F_NO_MAPPINGR3 == IEMTLBE_F_NO_MAPPINGR3);
        AssertCompile(PGMIEMGCPHYS2PTR_F_UNASSIGNED   == IEMTLBE_F_PG_UNASSIGNED);
        if (RT_LIKELY(pVCpu->iem.s.CodeTlb.uTlbPhysRev > IEMTLB_PHYS_REV_INCR))
        { /* likely */ }
        else
            IEMTlbInvalidateAllPhysicalSlow(pVCpu);
        pTlbe->pbMappingR3       = NULL;
        pTlbe->fFlagsAndPhysRev &= ~(  IEMTLBE_F_PHYS_REV
                                     | IEMTLBE_F_NO_MAPPINGR3 | IEMTLBE_F_PG_NO_READ | IEMTLBE_F_PG_NO_WRITE | IEMTLBE_F_PG_UNASSIGNED);
        int rc = PGMPhysIemGCPhys2PtrNoLock(pVCpu->CTX_SUFF(pVM), pVCpu, pTlbe->GCPhys, &pVCpu->iem.s.DataTlb.uTlbPhysRev,
                                            &pbMem, &pTlbe->fFlagsAndPhysRev);
        AssertRCReturn(rc, rc);
# ifdef IN_RING3
        pTlbe->pbMappingR3 = pbMem;
# endif
    }

    /*
     * Check the physical page level access and mapping.
     */
    if (   !(pTlbe->fFlagsAndPhysRev & (IEMTLBE_F_PG_NO_WRITE | IEMTLBE_F_PG_NO_READ))
        || !(pTlbe->fFlagsAndPhysRev & (  (fAccess & IEM_ACCESS_TYPE_WRITE ? IEMTLBE_F_PG_NO_WRITE : 0)
                                        | (fAccess & IEM_ACCESS_TYPE_READ  ? IEMTLBE_F_PG_NO_READ  : 0))) )
    { /* probably likely */ }
    else
        return iemMemBounceBufferMapPhys(pVCpu, iMemMap, ppvMem, cbMem,
                                         pTlbe->GCPhys | (GCPtrMem & GUEST_PAGE_OFFSET_MASK), fAccess,
                                           pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PG_UNASSIGNED ? VERR_PGM_PHYS_TLB_UNASSIGNED
                                         : pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PG_NO_READ    ? VERR_PGM_PHYS_TLB_CATCH_ALL
                                                                                             : VERR_PGM_PHYS_TLB_CATCH_WRITE);
    Assert(!(pTlbe->fFlagsAndPhysRev & IEMTLBE_F_NO_MAPPINGR3)); /* ASSUMPTIONS about PGMPhysIemGCPhys2PtrNoLock behaviour. */

    if (pbMem)
    {
        Assert(!((uintptr_t)pbMem & GUEST_PAGE_OFFSET_MASK));
        pbMem    = pbMem + (GCPtrMem & GUEST_PAGE_OFFSET_MASK);
        fAccess |= IEM_ACCESS_NOT_LOCKED;
    }
    else
    {
        Assert(!(fAccess & IEM_ACCESS_NOT_LOCKED));
        RTGCPHYS const GCPhysFirst = pTlbe->GCPhys | (GCPtrMem & GUEST_PAGE_OFFSET_MASK);
        rcStrict = iemMemPageMap(pVCpu, GCPhysFirst, fAccess, (void **)&pbMem, &pVCpu->iem.s.aMemMappingLocks[iMemMap].Lock);
        if (rcStrict != VINF_SUCCESS)
            return iemMemBounceBufferMapPhys(pVCpu, iMemMap, ppvMem, cbMem, GCPhysFirst, fAccess, rcStrict);
    }

    void * const pvMem = pbMem;

    if (fAccess & IEM_ACCESS_TYPE_WRITE)
        Log8(("IEM WR %RGv (%RGp) LB %#zx\n", GCPtrMem, pTlbe->GCPhys | (GCPtrMem & GUEST_PAGE_OFFSET_MASK), cbMem));
    if (fAccess & IEM_ACCESS_TYPE_READ)
        Log9(("IEM RD %RGv (%RGp) LB %#zx\n", GCPtrMem, pTlbe->GCPhys | (GCPtrMem & GUEST_PAGE_OFFSET_MASK), cbMem));

#else  /* !IEM_WITH_DATA_TLB */

    RTGCPHYS GCPhysFirst;
    rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, GCPtrMem, (uint32_t)cbMem, fAccess, &GCPhysFirst);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    if (fAccess & IEM_ACCESS_TYPE_WRITE)
        Log8(("IEM WR %RGv (%RGp) LB %#zx\n", GCPtrMem, GCPhysFirst, cbMem));
    if (fAccess & IEM_ACCESS_TYPE_READ)
        Log9(("IEM RD %RGv (%RGp) LB %#zx\n", GCPtrMem, GCPhysFirst, cbMem));

    void *pvMem;
    rcStrict = iemMemPageMap(pVCpu, GCPhysFirst, fAccess, &pvMem, &pVCpu->iem.s.aMemMappingLocks[iMemMap].Lock);
    if (rcStrict != VINF_SUCCESS)
        return iemMemBounceBufferMapPhys(pVCpu, iMemMap, ppvMem, cbMem, GCPhysFirst, fAccess, rcStrict);

#endif /* !IEM_WITH_DATA_TLB */

    /*
     * Fill in the mapping table entry.
     */
    pVCpu->iem.s.aMemMappings[iMemMap].pv      = pvMem;
    pVCpu->iem.s.aMemMappings[iMemMap].fAccess = fAccess;
    pVCpu->iem.s.iNextMapping     = iMemMap + 1;
    pVCpu->iem.s.cActiveMappings += 1;

    iemMemUpdateWrittenCounter(pVCpu, fAccess, cbMem);
    *ppvMem = pvMem;

    return VINF_SUCCESS;
}


/**
 * Commits the guest memory if bounce buffered and unmaps it.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pvMem               The mapping.
 * @param   fAccess             The kind of access.
 */
VBOXSTRICTRC iemMemCommitAndUnmap(PVMCPUCC pVCpu, void *pvMem, uint32_t fAccess) RT_NOEXCEPT
{
    int iMemMap = iemMapLookup(pVCpu, pvMem, fAccess);
    AssertReturn(iMemMap >= 0, iMemMap);

    /* If it's bounce buffered, we may need to write back the buffer. */
    if (pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_BOUNCE_BUFFERED)
    {
        if (pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_TYPE_WRITE)
            return iemMemBounceBufferCommitAndUnmap(pVCpu, iMemMap, false /*fPostponeFail*/);
    }
    /* Otherwise unlock it. */
    else if (!(pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_NOT_LOCKED))
        PGMPhysReleasePageMappingLock(pVCpu->CTX_SUFF(pVM), &pVCpu->iem.s.aMemMappingLocks[iMemMap].Lock);

    /* Free the entry. */
    pVCpu->iem.s.aMemMappings[iMemMap].fAccess = IEM_ACCESS_INVALID;
    Assert(pVCpu->iem.s.cActiveMappings != 0);
    pVCpu->iem.s.cActiveMappings--;
    return VINF_SUCCESS;
}

#ifdef IEM_WITH_SETJMP

/**
 * Maps the specified guest memory for the given kind of access, longjmp on
 * error.
 *
 * This may be using bounce buffering of the memory if it's crossing a page
 * boundary or if there is an access handler installed for any of it.  Because
 * of lock prefix guarantees, we're in for some extra clutter when this
 * happens.
 *
 * This may raise a \#GP, \#SS, \#PF or \#AC.
 *
 * @returns Pointer to the mapped memory.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling thread.
 * @param   cbMem       The number of bytes to map.  This is usually 1,
 *                      2, 4, 6, 8, 12, 16, 32 or 512.  When used by
 *                      string operations it can be up to a page.
 * @param   iSegReg     The index of the segment register to use for
 *                      this access.  The base and limits are checked.
 *                      Use UINT8_MAX to indicate that no segmentation
 *                      is required (for IDT, GDT and LDT accesses).
 * @param   GCPtrMem    The address of the guest memory.
 * @param   fAccess     How the memory is being accessed.  The
 *                      IEM_ACCESS_TYPE_XXX bit is used to figure out
 *                      how to map the memory, while the
 *                      IEM_ACCESS_WHAT_XXX bit is used when raising
 *                      exceptions.
 * @param   uAlignCtl   Alignment control:
 *                          - Bits 15:0 is the alignment mask.
 *                          - Bits 31:16 for flags like IEM_MEMMAP_F_ALIGN_GP,
 *                            IEM_MEMMAP_F_ALIGN_SSE, and
 *                            IEM_MEMMAP_F_ALIGN_GP_OR_AC.
 *                      Pass zero to skip alignment.
 */
void *iemMemMapJmp(PVMCPUCC pVCpu, size_t cbMem, uint8_t iSegReg, RTGCPTR GCPtrMem, uint32_t fAccess,
                   uint32_t uAlignCtl) IEM_NOEXCEPT_MAY_LONGJMP
{
    /*
     * Check the input, check segment access and adjust address
     * with segment base.
     */
    Assert(cbMem <= 64 || cbMem == 512 || cbMem == 108 || cbMem == 104 || cbMem == 94); /* 512 is the max! */
    Assert(~(fAccess & ~(IEM_ACCESS_TYPE_MASK | IEM_ACCESS_WHAT_MASK)));
    Assert(pVCpu->iem.s.cActiveMappings < RT_ELEMENTS(pVCpu->iem.s.aMemMappings));

    VBOXSTRICTRC rcStrict = iemMemApplySegment(pVCpu, fAccess, iSegReg, cbMem, &GCPtrMem);
    if (rcStrict == VINF_SUCCESS) { /*likely*/ }
    else IEM_DO_LONGJMP(pVCpu, VBOXSTRICTRC_VAL(rcStrict));

    /*
     * Alignment check.
     */
    if ( (GCPtrMem & (uAlignCtl & UINT16_MAX)) == 0 )
    { /* likelyish */ }
    else
    {
        /* Misaligned access. */
        if ((fAccess & IEM_ACCESS_WHAT_MASK) != IEM_ACCESS_WHAT_SYS)
        {
            if (   !(uAlignCtl & IEM_MEMMAP_F_ALIGN_GP)
                || (   (uAlignCtl & IEM_MEMMAP_F_ALIGN_SSE)
                    && (pVCpu->cpum.GstCtx.XState.x87.MXCSR & X86_MXCSR_MM)) )
            {
                AssertCompile(X86_CR0_AM == X86_EFL_AC);

                if (iemMemAreAlignmentChecksEnabled(pVCpu))
                    iemRaiseAlignmentCheckExceptionJmp(pVCpu);
            }
            else if (   (uAlignCtl & IEM_MEMMAP_F_ALIGN_GP_OR_AC)
                     && (GCPtrMem & 3) /* The value 4 matches 10980xe's FXSAVE and helps make bs3-cpu-basic2 work. */
                    /** @todo may only apply to 2, 4 or 8 byte misalignments depending on the CPU
                     * implementation. See FXSAVE/FRSTOR/XSAVE/XRSTOR/++.  Using 4 for now as
                     * that's what FXSAVE does on a 10980xe. */
                     && iemMemAreAlignmentChecksEnabled(pVCpu))
                iemRaiseAlignmentCheckExceptionJmp(pVCpu);
            else
                iemRaiseGeneralProtectionFault0Jmp(pVCpu);
        }
    }

    /*
     * Figure out which mapping entry to use.
     */
    unsigned iMemMap = pVCpu->iem.s.iNextMapping;
    if (   iMemMap >= RT_ELEMENTS(pVCpu->iem.s.aMemMappings)
        || pVCpu->iem.s.aMemMappings[iMemMap].fAccess != IEM_ACCESS_INVALID)
    {
        iMemMap = iemMemMapFindFree(pVCpu);
        AssertLogRelMsgStmt(iMemMap < RT_ELEMENTS(pVCpu->iem.s.aMemMappings),
                            ("active=%d fAccess[0] = {%#x, %#x, %#x}\n", pVCpu->iem.s.cActiveMappings,
                             pVCpu->iem.s.aMemMappings[0].fAccess, pVCpu->iem.s.aMemMappings[1].fAccess,
                             pVCpu->iem.s.aMemMappings[2].fAccess),
                            IEM_DO_LONGJMP(pVCpu, VERR_IEM_IPE_9));
    }

    /*
     * Crossing a page boundary?
     */
    if ((GCPtrMem & GUEST_PAGE_OFFSET_MASK) + cbMem <= GUEST_PAGE_SIZE)
    { /* No (likely). */ }
    else
    {
        void *pvMem;
        rcStrict = iemMemBounceBufferMapCrossPage(pVCpu, iMemMap, &pvMem, cbMem, GCPtrMem, fAccess);
        if (rcStrict == VINF_SUCCESS)
            return pvMem;
        IEM_DO_LONGJMP(pVCpu, VBOXSTRICTRC_VAL(rcStrict));
    }

#ifdef IEM_WITH_DATA_TLB
    Assert(!(fAccess & IEM_ACCESS_TYPE_EXEC));

    /*
     * Get the TLB entry for this page.
     */
    uint64_t const     uTag  = IEMTLB_CALC_TAG(    &pVCpu->iem.s.DataTlb, GCPtrMem);
    PIEMTLBENTRY const pTlbe = IEMTLB_TAG_TO_ENTRY(&pVCpu->iem.s.DataTlb, uTag);
    if (pTlbe->uTag == uTag)
        STAM_STATS({pVCpu->iem.s.DataTlb.cTlbHits++;});
    else
    {
        pVCpu->iem.s.DataTlb.cTlbMisses++;
        PGMPTWALK Walk;
        int rc = PGMGstGetPage(pVCpu, GCPtrMem, &Walk);
        if (RT_FAILURE(rc))
        {
            Log(("iemMemMap: GCPtrMem=%RGv - failed to fetch page -> #PF\n", GCPtrMem));
# ifdef VBOX_WITH_NESTED_HWVIRT_VMX_EPT
            if (Walk.fFailed & PGM_WALKFAIL_EPT)
                IEM_VMX_VMEXIT_EPT_RET(pVCpu, &Walk, fAccess, IEM_SLAT_FAIL_LINEAR_TO_PHYS_ADDR, 0 /* cbInstr */);
# endif
            iemRaisePageFaultJmp(pVCpu, GCPtrMem, (uint32_t)cbMem, fAccess, rc);
        }

        Assert(Walk.fSucceeded);
        pTlbe->uTag             = uTag;
        pTlbe->fFlagsAndPhysRev = ~Walk.fEffective & (X86_PTE_US | X86_PTE_RW | X86_PTE_D | X86_PTE_A); /* skipping NX */
        pTlbe->GCPhys           = Walk.GCPhys;
        pTlbe->pbMappingR3      = NULL;
    }

    /*
     * Check the flags and physical revision.
     */
    /** @todo make the caller pass these in with fAccess. */
    uint64_t const fNoUser          = (fAccess & IEM_ACCESS_WHAT_MASK) != IEM_ACCESS_WHAT_SYS && pVCpu->iem.s.uCpl == 3
                                    ? IEMTLBE_F_PT_NO_USER : 0;
    uint64_t const fNoWriteNoDirty  = fAccess & IEM_ACCESS_TYPE_WRITE
                                    ? IEMTLBE_F_PG_NO_WRITE | IEMTLBE_F_PT_NO_DIRTY
                                      | (   (pVCpu->cpum.GstCtx.cr0 & X86_CR0_WP)
                                         || (pVCpu->iem.s.uCpl == 3 && (fAccess & IEM_ACCESS_WHAT_MASK) != IEM_ACCESS_WHAT_SYS)
                                         ? IEMTLBE_F_PT_NO_WRITE : 0)
                                    : 0;
    uint64_t const fNoRead          = fAccess & IEM_ACCESS_TYPE_READ ? IEMTLBE_F_PG_NO_READ : 0;
    uint8_t       *pbMem            = NULL;
    if (   (pTlbe->fFlagsAndPhysRev & (IEMTLBE_F_PHYS_REV | IEMTLBE_F_PT_NO_ACCESSED | fNoRead | fNoWriteNoDirty | fNoUser))
        == pVCpu->iem.s.DataTlb.uTlbPhysRev)
# ifdef IN_RING3
        pbMem = pTlbe->pbMappingR3;
# else
        pbMem = NULL;
# endif
    else
    {
        /*
         * Okay, something isn't quite right or needs refreshing.
         */
        /* Write to read only memory? */
        if (pTlbe->fFlagsAndPhysRev & fNoWriteNoDirty & IEMTLBE_F_PT_NO_WRITE)
        {
            Log(("iemMemMapJmp: GCPtrMem=%RGv - read-only page -> #PF\n", GCPtrMem));
# ifdef VBOX_WITH_NESTED_HWVIRT_VMX_EPT
            if (Walk.fFailed & PGM_WALKFAIL_EPT)
                IEM_VMX_VMEXIT_EPT_RET(pVCpu, &Walk, fAccess, IEM_SLAT_FAIL_LINEAR_TO_PAGE_TABLE, 0 /* cbInstr */);
# endif
            iemRaisePageFaultJmp(pVCpu, GCPtrMem, (uint32_t)cbMem, fAccess & ~IEM_ACCESS_TYPE_READ, VERR_ACCESS_DENIED);
        }

        /* Kernel memory accessed by userland? */
        if (pTlbe->fFlagsAndPhysRev & fNoUser & IEMTLBE_F_PT_NO_USER)
        {
            Log(("iemMemMapJmp: GCPtrMem=%RGv - user access to kernel page -> #PF\n", GCPtrMem));
# ifdef VBOX_WITH_NESTED_HWVIRT_VMX_EPT
            if (Walk.fFailed & PGM_WALKFAIL_EPT)
                IEM_VMX_VMEXIT_EPT_RET(pVCpu, &Walk, fAccess, IEM_SLAT_FAIL_LINEAR_TO_PAGE_TABLE, 0 /* cbInstr */);
# endif
            iemRaisePageFaultJmp(pVCpu, GCPtrMem, (uint32_t)cbMem, fAccess, VERR_ACCESS_DENIED);
        }

        /* Set the dirty / access flags.
           ASSUMES this is set when the address is translated rather than on commit... */
        /** @todo testcase: check when A and D bits are actually set by the CPU.  */
        if (pTlbe->fFlagsAndPhysRev & ((fNoWriteNoDirty & IEMTLBE_F_PT_NO_DIRTY) | IEMTLBE_F_PT_NO_ACCESSED))
        {
            uint32_t const fAccessedDirty = fAccess & IEM_ACCESS_TYPE_WRITE ? X86_PTE_D | X86_PTE_A : X86_PTE_A;
            int rc2 = PGMGstModifyPage(pVCpu, GCPtrMem, 1, fAccessedDirty, ~(uint64_t)fAccessedDirty);
            AssertRC(rc2);
            /** @todo Nested VMX: Accessed/dirty bit currently not supported, asserted below. */
            Assert(!(CPUMGetGuestIa32VmxEptVpidCap(pVCpu) & VMX_BF_EPT_VPID_CAP_ACCESS_DIRTY_MASK));
            pTlbe->fFlagsAndPhysRev &= ~((fNoWriteNoDirty & IEMTLBE_F_PT_NO_DIRTY) | IEMTLBE_F_PT_NO_ACCESSED);
        }

        /*
         * Check if the physical page info needs updating.
         */
        if ((pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PHYS_REV) == pVCpu->iem.s.DataTlb.uTlbPhysRev)
# ifdef IN_RING3
            pbMem = pTlbe->pbMappingR3;
# else
            pbMem = NULL;
# endif
        else
        {
            AssertCompile(PGMIEMGCPHYS2PTR_F_NO_WRITE     == IEMTLBE_F_PG_NO_WRITE);
            AssertCompile(PGMIEMGCPHYS2PTR_F_NO_READ      == IEMTLBE_F_PG_NO_READ);
            AssertCompile(PGMIEMGCPHYS2PTR_F_NO_MAPPINGR3 == IEMTLBE_F_NO_MAPPINGR3);
            AssertCompile(PGMIEMGCPHYS2PTR_F_UNASSIGNED   == IEMTLBE_F_PG_UNASSIGNED);
            pTlbe->pbMappingR3       = NULL;
            pTlbe->fFlagsAndPhysRev &= ~(  IEMTLBE_F_PHYS_REV
                                         | IEMTLBE_F_NO_MAPPINGR3 | IEMTLBE_F_PG_NO_READ | IEMTLBE_F_PG_NO_WRITE | IEMTLBE_F_PG_UNASSIGNED);
            int rc = PGMPhysIemGCPhys2PtrNoLock(pVCpu->CTX_SUFF(pVM), pVCpu, pTlbe->GCPhys, &pVCpu->iem.s.DataTlb.uTlbPhysRev,
                                                &pbMem, &pTlbe->fFlagsAndPhysRev);
            AssertRCStmt(rc, IEM_DO_LONGJMP(pVCpu, rc));
# ifdef IN_RING3
            pTlbe->pbMappingR3 = pbMem;
# endif
        }

        /*
         * Check the physical page level access and mapping.
         */
        if (!(pTlbe->fFlagsAndPhysRev & ((fNoWriteNoDirty | fNoRead) & (IEMTLBE_F_PG_NO_WRITE | IEMTLBE_F_PG_NO_READ))))
        { /* probably likely */ }
        else
        {
            rcStrict = iemMemBounceBufferMapPhys(pVCpu, iMemMap, (void **)&pbMem, cbMem,
                                                 pTlbe->GCPhys | (GCPtrMem & GUEST_PAGE_OFFSET_MASK), fAccess,
                                                   pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PG_UNASSIGNED ? VERR_PGM_PHYS_TLB_UNASSIGNED
                                                 : pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PG_NO_READ    ? VERR_PGM_PHYS_TLB_CATCH_ALL
                                                                                                     : VERR_PGM_PHYS_TLB_CATCH_WRITE);
            if (rcStrict == VINF_SUCCESS)
                return pbMem;
            IEM_DO_LONGJMP(pVCpu, VBOXSTRICTRC_VAL(rcStrict));
        }
    }
    Assert(!(pTlbe->fFlagsAndPhysRev & IEMTLBE_F_NO_MAPPINGR3)); /* ASSUMPTIONS about PGMPhysIemGCPhys2PtrNoLock behaviour. */

    if (pbMem)
    {
        Assert(!((uintptr_t)pbMem & GUEST_PAGE_OFFSET_MASK));
        pbMem    = pbMem + (GCPtrMem & GUEST_PAGE_OFFSET_MASK);
        fAccess |= IEM_ACCESS_NOT_LOCKED;
    }
    else
    {
        Assert(!(fAccess & IEM_ACCESS_NOT_LOCKED));
        RTGCPHYS const GCPhysFirst = pTlbe->GCPhys | (GCPtrMem & GUEST_PAGE_OFFSET_MASK);
        rcStrict = iemMemPageMap(pVCpu, GCPhysFirst, fAccess, (void **)&pbMem, &pVCpu->iem.s.aMemMappingLocks[iMemMap].Lock);
        if (rcStrict == VINF_SUCCESS)
            return pbMem;
        IEM_DO_LONGJMP(pVCpu, VBOXSTRICTRC_VAL(rcStrict));
    }

    void * const pvMem = pbMem;

    if (fAccess & IEM_ACCESS_TYPE_WRITE)
        Log8(("IEM WR %RGv (%RGp) LB %#zx\n", GCPtrMem, pTlbe->GCPhys | (GCPtrMem & GUEST_PAGE_OFFSET_MASK), cbMem));
    if (fAccess & IEM_ACCESS_TYPE_READ)
        Log9(("IEM RD %RGv (%RGp) LB %#zx\n", GCPtrMem, pTlbe->GCPhys | (GCPtrMem & GUEST_PAGE_OFFSET_MASK), cbMem));

#else  /* !IEM_WITH_DATA_TLB */


    RTGCPHYS GCPhysFirst;
    rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, GCPtrMem, (uint32_t)cbMem, fAccess, &GCPhysFirst);
    if (rcStrict == VINF_SUCCESS) { /*likely*/ }
    else IEM_DO_LONGJMP(pVCpu, VBOXSTRICTRC_VAL(rcStrict));

    if (fAccess & IEM_ACCESS_TYPE_WRITE)
        Log8(("IEM WR %RGv (%RGp) LB %#zx\n", GCPtrMem, GCPhysFirst, cbMem));
    if (fAccess & IEM_ACCESS_TYPE_READ)
        Log9(("IEM RD %RGv (%RGp) LB %#zx\n", GCPtrMem, GCPhysFirst, cbMem));

    void *pvMem;
    rcStrict = iemMemPageMap(pVCpu, GCPhysFirst, fAccess, &pvMem, &pVCpu->iem.s.aMemMappingLocks[iMemMap].Lock);
    if (rcStrict == VINF_SUCCESS)
    { /* likely */ }
    else
    {
        rcStrict = iemMemBounceBufferMapPhys(pVCpu, iMemMap, &pvMem, cbMem, GCPhysFirst, fAccess, rcStrict);
        if (rcStrict == VINF_SUCCESS)
            return pvMem;
        IEM_DO_LONGJMP(pVCpu, VBOXSTRICTRC_VAL(rcStrict));
    }

#endif /* !IEM_WITH_DATA_TLB */

    /*
     * Fill in the mapping table entry.
     */
    pVCpu->iem.s.aMemMappings[iMemMap].pv      = pvMem;
    pVCpu->iem.s.aMemMappings[iMemMap].fAccess = fAccess;
    pVCpu->iem.s.iNextMapping = iMemMap + 1;
    pVCpu->iem.s.cActiveMappings++;

    iemMemUpdateWrittenCounter(pVCpu, fAccess, cbMem);
    return pvMem;
}


/**
 * Commits the guest memory if bounce buffered and unmaps it, longjmp on error.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pvMem               The mapping.
 * @param   fAccess             The kind of access.
 */
void iemMemCommitAndUnmapJmp(PVMCPUCC pVCpu, void *pvMem, uint32_t fAccess) IEM_NOEXCEPT_MAY_LONGJMP
{
    int iMemMap = iemMapLookup(pVCpu, pvMem, fAccess);
    AssertStmt(iMemMap >= 0, IEM_DO_LONGJMP(pVCpu, iMemMap));

    /* If it's bounce buffered, we may need to write back the buffer. */
    if (pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_BOUNCE_BUFFERED)
    {
        if (pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_TYPE_WRITE)
        {
            VBOXSTRICTRC rcStrict = iemMemBounceBufferCommitAndUnmap(pVCpu, iMemMap, false /*fPostponeFail*/);
            if (rcStrict == VINF_SUCCESS)
                return;
            IEM_DO_LONGJMP(pVCpu, VBOXSTRICTRC_VAL(rcStrict));
        }
    }
    /* Otherwise unlock it. */
    else if (!(pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_NOT_LOCKED))
        PGMPhysReleasePageMappingLock(pVCpu->CTX_SUFF(pVM), &pVCpu->iem.s.aMemMappingLocks[iMemMap].Lock);

    /* Free the entry. */
    pVCpu->iem.s.aMemMappings[iMemMap].fAccess = IEM_ACCESS_INVALID;
    Assert(pVCpu->iem.s.cActiveMappings != 0);
    pVCpu->iem.s.cActiveMappings--;
}

#endif /* IEM_WITH_SETJMP */

#ifndef IN_RING3
/**
 * Commits the guest memory if bounce buffered and unmaps it, if any bounce
 * buffer part shows trouble it will be postponed to ring-3 (sets FF and stuff).
 *
 * Allows the instruction to be completed and retired, while the IEM user will
 * return to ring-3 immediately afterwards and do the postponed writes there.
 *
 * @returns VBox status code (no strict statuses).  Caller must check
 *          VMCPU_FF_IEM before repeating string instructions and similar stuff.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pvMem               The mapping.
 * @param   fAccess             The kind of access.
 */
VBOXSTRICTRC iemMemCommitAndUnmapPostponeTroubleToR3(PVMCPUCC pVCpu, void *pvMem, uint32_t fAccess) RT_NOEXCEPT
{
    int iMemMap = iemMapLookup(pVCpu, pvMem, fAccess);
    AssertReturn(iMemMap >= 0, iMemMap);

    /* If it's bounce buffered, we may need to write back the buffer. */
    if (pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_BOUNCE_BUFFERED)
    {
        if (pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_TYPE_WRITE)
            return iemMemBounceBufferCommitAndUnmap(pVCpu, iMemMap, true /*fPostponeFail*/);
    }
    /* Otherwise unlock it. */
    else if (!(pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_NOT_LOCKED))
        PGMPhysReleasePageMappingLock(pVCpu->CTX_SUFF(pVM), &pVCpu->iem.s.aMemMappingLocks[iMemMap].Lock);

    /* Free the entry. */
    pVCpu->iem.s.aMemMappings[iMemMap].fAccess = IEM_ACCESS_INVALID;
    Assert(pVCpu->iem.s.cActiveMappings != 0);
    pVCpu->iem.s.cActiveMappings--;
    return VINF_SUCCESS;
}
#endif


/**
 * Rollbacks mappings, releasing page locks and such.
 *
 * The caller shall only call this after checking cActiveMappings.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling thread.
 */
void iemMemRollback(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    Assert(pVCpu->iem.s.cActiveMappings > 0);

    uint32_t iMemMap = RT_ELEMENTS(pVCpu->iem.s.aMemMappings);
    while (iMemMap-- > 0)
    {
        uint32_t const fAccess = pVCpu->iem.s.aMemMappings[iMemMap].fAccess;
        if (fAccess != IEM_ACCESS_INVALID)
        {
            AssertMsg(!(fAccess & ~IEM_ACCESS_VALID_MASK) && fAccess != 0, ("%#x\n", fAccess));
            pVCpu->iem.s.aMemMappings[iMemMap].fAccess = IEM_ACCESS_INVALID;
            if (!(fAccess & (IEM_ACCESS_BOUNCE_BUFFERED | IEM_ACCESS_NOT_LOCKED)))
                PGMPhysReleasePageMappingLock(pVCpu->CTX_SUFF(pVM), &pVCpu->iem.s.aMemMappingLocks[iMemMap].Lock);
            AssertMsg(pVCpu->iem.s.cActiveMappings > 0,
                      ("iMemMap=%u fAccess=%#x pv=%p GCPhysFirst=%RGp GCPhysSecond=%RGp\n",
                       iMemMap, fAccess, pVCpu->iem.s.aMemMappings[iMemMap].pv,
                       pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond));
            pVCpu->iem.s.cActiveMappings--;
        }
    }
}


/**
 * Fetches a data byte.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu8Dst              Where to return the byte.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
VBOXSTRICTRC iemMemFetchDataU8(PVMCPUCC pVCpu, uint8_t *pu8Dst, uint8_t iSegReg, RTGCPTR GCPtrMem) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    uint8_t const *pu8Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu8Src, sizeof(*pu8Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R, 0);
    if (rc == VINF_SUCCESS)
    {
        *pu8Dst = *pu8Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu8Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Fetches a data byte, longjmp on error.
 *
 * @returns The byte.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
uint8_t iemMemFetchDataU8Jmp(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem) IEM_NOEXCEPT_MAY_LONGJMP
{
    /* The lazy approach for now... */
    uint8_t const *pu8Src = (uint8_t const *)iemMemMapJmp(pVCpu, sizeof(*pu8Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R, 0);
    uint8_t const  bRet   = *pu8Src;
    iemMemCommitAndUnmapJmp(pVCpu, (void *)pu8Src, IEM_ACCESS_DATA_R);
    return bRet;
}
#endif /* IEM_WITH_SETJMP */


/**
 * Fetches a data word.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu16Dst             Where to return the word.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
VBOXSTRICTRC iemMemFetchDataU16(PVMCPUCC pVCpu, uint16_t *pu16Dst, uint8_t iSegReg, RTGCPTR GCPtrMem) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    uint16_t const *pu16Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu16Src, sizeof(*pu16Src), iSegReg, GCPtrMem,
                                IEM_ACCESS_DATA_R, sizeof(*pu16Src) - 1);
    if (rc == VINF_SUCCESS)
    {
        *pu16Dst = *pu16Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu16Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Fetches a data word, longjmp on error.
 *
 * @returns The word
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
uint16_t iemMemFetchDataU16Jmp(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem) IEM_NOEXCEPT_MAY_LONGJMP
{
    /* The lazy approach for now... */
    uint16_t const *pu16Src = (uint16_t const *)iemMemMapJmp(pVCpu, sizeof(*pu16Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R,
                                                             sizeof(*pu16Src) - 1);
    uint16_t const u16Ret = *pu16Src;
    iemMemCommitAndUnmapJmp(pVCpu, (void *)pu16Src, IEM_ACCESS_DATA_R);
    return u16Ret;
}
#endif


/**
 * Fetches a data dword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu32Dst             Where to return the dword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
VBOXSTRICTRC iemMemFetchDataU32(PVMCPUCC pVCpu, uint32_t *pu32Dst, uint8_t iSegReg, RTGCPTR GCPtrMem) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    uint32_t const *pu32Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu32Src, sizeof(*pu32Src), iSegReg, GCPtrMem,
                                IEM_ACCESS_DATA_R, sizeof(*pu32Src) - 1);
    if (rc == VINF_SUCCESS)
    {
        *pu32Dst = *pu32Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu32Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


/**
 * Fetches a data dword and zero extends it to a qword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64Dst             Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
VBOXSTRICTRC iemMemFetchDataU32_ZX_U64(PVMCPUCC pVCpu, uint64_t *pu64Dst, uint8_t iSegReg, RTGCPTR GCPtrMem) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    uint32_t const *pu32Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu32Src, sizeof(*pu32Src), iSegReg, GCPtrMem,
                                IEM_ACCESS_DATA_R, sizeof(*pu32Src) - 1);
    if (rc == VINF_SUCCESS)
    {
        *pu64Dst = *pu32Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu32Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP

/**
 * Fetches a data dword, longjmp on error, fallback/safe version.
 *
 * @returns The dword
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
uint32_t iemMemFetchDataU32SafeJmp(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem) IEM_NOEXCEPT_MAY_LONGJMP
{
    uint32_t const *pu32Src = (uint32_t const *)iemMemMapJmp(pVCpu, sizeof(*pu32Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R,
                                                             sizeof(*pu32Src) - 1);
    uint32_t const  u32Ret  = *pu32Src;
    iemMemCommitAndUnmapJmp(pVCpu, (void *)pu32Src, IEM_ACCESS_DATA_R);
    return u32Ret;
}


/**
 * Fetches a data dword, longjmp on error.
 *
 * @returns The dword
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
uint32_t iemMemFetchDataU32Jmp(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem) IEM_NOEXCEPT_MAY_LONGJMP
{
# if defined(IEM_WITH_DATA_TLB) && defined(IN_RING3)
    /*
     * Convert from segmented to flat address and check that it doesn't cross a page boundrary.
     */
    RTGCPTR GCPtrEff = iemMemApplySegmentToReadJmp(pVCpu, iSegReg, sizeof(uint32_t), GCPtrMem);
    if (RT_LIKELY((GCPtrEff & GUEST_PAGE_OFFSET_MASK) <= GUEST_PAGE_SIZE - sizeof(uint32_t)))
    {
        /*
         * TLB lookup.
         */
        uint64_t const uTag  = IEMTLB_CALC_TAG(    &pVCpu->iem.s.DataTlb, GCPtrEff);
        PIEMTLBENTRY   pTlbe = IEMTLB_TAG_TO_ENTRY(&pVCpu->iem.s.DataTlb, uTag);
        if (pTlbe->uTag == uTag)
        {
            /*
             * Check TLB page table level access flags.
             */
            uint64_t const fNoUser = pVCpu->iem.s.uCpl == 3 ? IEMTLBE_F_PT_NO_USER : 0;
            if (   (pTlbe->fFlagsAndPhysRev & (  IEMTLBE_F_PHYS_REV       | IEMTLBE_F_PG_UNASSIGNED | IEMTLBE_F_PG_NO_READ
                                               | IEMTLBE_F_PT_NO_ACCESSED | IEMTLBE_F_NO_MAPPINGR3  | fNoUser))
                == pVCpu->iem.s.DataTlb.uTlbPhysRev)
            {
                STAM_STATS({pVCpu->iem.s.DataTlb.cTlbHits++;});

                /*
                 * Alignment check:
                 */
                /** @todo check priority \#AC vs \#PF */
                if (   !(GCPtrEff & (sizeof(uint32_t) - 1))
                    || !(pVCpu->cpum.GstCtx.cr0 & X86_CR0_AM)
                    || !pVCpu->cpum.GstCtx.eflags.Bits.u1AC
                    || pVCpu->iem.s.uCpl != 3)
                {
                    /*
                     * Fetch and return the dword
                     */
                    Assert(pTlbe->pbMappingR3); /* (Only ever cleared by the owning EMT.) */
                    Assert(!((uintptr_t)pTlbe->pbMappingR3 & GUEST_PAGE_OFFSET_MASK));
                    return *(uint32_t const *)&pTlbe->pbMappingR3[GCPtrEff & GUEST_PAGE_OFFSET_MASK];
                }
                Log10(("iemMemFetchDataU32Jmp: Raising #AC for %RGv\n", GCPtrEff));
                iemRaiseAlignmentCheckExceptionJmp(pVCpu);
            }
        }
    }

    /* Fall back on the slow careful approach in case of TLB miss, MMIO, exception
       outdated page pointer, or other troubles. */
    Log10(("iemMemFetchDataU32Jmp: %u:%RGv fallback\n", iSegReg, GCPtrMem));
    return iemMemFetchDataU32SafeJmp(pVCpu, iSegReg, GCPtrMem);

# else
    uint32_t const *pu32Src = (uint32_t const *)iemMemMapJmp(pVCpu, sizeof(*pu32Src), iSegReg, GCPtrMem,
                                                             IEM_ACCESS_DATA_R, sizeof(*pu32Src) - 1);
    uint32_t const  u32Ret  = *pu32Src;
    iemMemCommitAndUnmapJmp(pVCpu, (void *)pu32Src, IEM_ACCESS_DATA_R);
    return u32Ret;
# endif
}
#endif


#ifdef SOME_UNUSED_FUNCTION
/**
 * Fetches a data dword and sign extends it to a qword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64Dst             Where to return the sign extended value.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
VBOXSTRICTRC iemMemFetchDataS32SxU64(PVMCPUCC pVCpu, uint64_t *pu64Dst, uint8_t iSegReg, RTGCPTR GCPtrMem) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    int32_t const *pi32Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pi32Src, sizeof(*pi32Src), iSegReg, GCPtrMem,
                                IEM_ACCESS_DATA_R, sizeof(*pi32Src) - 1);
    if (rc == VINF_SUCCESS)
    {
        *pu64Dst = *pi32Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pi32Src, IEM_ACCESS_DATA_R);
    }
#ifdef __GNUC__ /* warning: GCC may be a royal pain */
    else
        *pu64Dst = 0;
#endif
    return rc;
}
#endif


/**
 * Fetches a data qword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64Dst             Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
VBOXSTRICTRC iemMemFetchDataU64(PVMCPUCC pVCpu, uint64_t *pu64Dst, uint8_t iSegReg, RTGCPTR GCPtrMem) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    uint64_t const *pu64Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu64Src, sizeof(*pu64Src), iSegReg, GCPtrMem,
                                IEM_ACCESS_DATA_R, sizeof(*pu64Src) - 1);
    if (rc == VINF_SUCCESS)
    {
        *pu64Dst = *pu64Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu64Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Fetches a data qword, longjmp on error.
 *
 * @returns The qword.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
uint64_t iemMemFetchDataU64Jmp(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem) IEM_NOEXCEPT_MAY_LONGJMP
{
    /* The lazy approach for now... */
    uint64_t const *pu64Src = (uint64_t const *)iemMemMapJmp(pVCpu, sizeof(*pu64Src), iSegReg, GCPtrMem,
                                                             IEM_ACCESS_DATA_R, sizeof(*pu64Src) - 1);
    uint64_t const u64Ret = *pu64Src;
    iemMemCommitAndUnmapJmp(pVCpu, (void *)pu64Src, IEM_ACCESS_DATA_R);
    return u64Ret;
}
#endif


/**
 * Fetches a data qword, aligned at a 16 byte boundrary (for SSE).
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64Dst             Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
VBOXSTRICTRC iemMemFetchDataU64AlignedU128(PVMCPUCC pVCpu, uint64_t *pu64Dst, uint8_t iSegReg, RTGCPTR GCPtrMem) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    uint64_t const *pu64Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu64Src, sizeof(*pu64Src), iSegReg, GCPtrMem,
                                IEM_ACCESS_DATA_R, 15 | IEM_MEMMAP_F_ALIGN_GP | IEM_MEMMAP_F_ALIGN_SSE);
    if (rc == VINF_SUCCESS)
    {
        *pu64Dst = *pu64Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu64Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Fetches a data qword, longjmp on error.
 *
 * @returns The qword.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
uint64_t iemMemFetchDataU64AlignedU128Jmp(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem) IEM_NOEXCEPT_MAY_LONGJMP
{
    /* The lazy approach for now... */
    uint64_t const *pu64Src = (uint64_t const *)iemMemMapJmp(pVCpu, sizeof(*pu64Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R,
                                                             15 | IEM_MEMMAP_F_ALIGN_GP | IEM_MEMMAP_F_ALIGN_SSE);
    uint64_t const u64Ret = *pu64Src;
    iemMemCommitAndUnmapJmp(pVCpu, (void *)pu64Src, IEM_ACCESS_DATA_R);
    return u64Ret;
}
#endif


/**
 * Fetches a data tword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pr80Dst             Where to return the tword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
VBOXSTRICTRC iemMemFetchDataR80(PVMCPUCC pVCpu, PRTFLOAT80U pr80Dst, uint8_t iSegReg, RTGCPTR GCPtrMem) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    PCRTFLOAT80U pr80Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pr80Src, sizeof(*pr80Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R, 7);
    if (rc == VINF_SUCCESS)
    {
        *pr80Dst = *pr80Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pr80Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Fetches a data tword, longjmp on error.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pr80Dst             Where to return the tword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
void iemMemFetchDataR80Jmp(PVMCPUCC pVCpu, PRTFLOAT80U pr80Dst, uint8_t iSegReg, RTGCPTR GCPtrMem) IEM_NOEXCEPT_MAY_LONGJMP
{
    /* The lazy approach for now... */
    PCRTFLOAT80U pr80Src = (PCRTFLOAT80U)iemMemMapJmp(pVCpu, sizeof(*pr80Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R, 7);
    *pr80Dst = *pr80Src;
    iemMemCommitAndUnmapJmp(pVCpu, (void *)pr80Src, IEM_ACCESS_DATA_R);
}
#endif


/**
 * Fetches a data decimal tword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pd80Dst             Where to return the tword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
VBOXSTRICTRC iemMemFetchDataD80(PVMCPUCC pVCpu, PRTPBCD80U pd80Dst, uint8_t iSegReg, RTGCPTR GCPtrMem) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    PCRTPBCD80U pd80Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pd80Src, sizeof(*pd80Src), iSegReg, GCPtrMem,
                                IEM_ACCESS_DATA_R, 7 /** @todo FBLD alignment check */);
    if (rc == VINF_SUCCESS)
    {
        *pd80Dst = *pd80Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pd80Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Fetches a data decimal tword, longjmp on error.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pd80Dst             Where to return the tword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
void iemMemFetchDataD80Jmp(PVMCPUCC pVCpu, PRTPBCD80U pd80Dst, uint8_t iSegReg, RTGCPTR GCPtrMem) IEM_NOEXCEPT_MAY_LONGJMP
{
    /* The lazy approach for now... */
    PCRTPBCD80U pd80Src = (PCRTPBCD80U)iemMemMapJmp(pVCpu, sizeof(*pd80Src), iSegReg, GCPtrMem,
                                                    IEM_ACCESS_DATA_R, 7 /** @todo FBSTP alignment check */);
    *pd80Dst = *pd80Src;
    iemMemCommitAndUnmapJmp(pVCpu, (void *)pd80Src, IEM_ACCESS_DATA_R);
}
#endif


/**
 * Fetches a data dqword (double qword), generally SSE related.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu128Dst            Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
VBOXSTRICTRC iemMemFetchDataU128(PVMCPUCC pVCpu, PRTUINT128U pu128Dst, uint8_t iSegReg, RTGCPTR GCPtrMem) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    PCRTUINT128U pu128Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu128Src, sizeof(*pu128Src), iSegReg, GCPtrMem,
                                IEM_ACCESS_DATA_R, 0 /* NO_AC variant */);
    if (rc == VINF_SUCCESS)
    {
        pu128Dst->au64[0] = pu128Src->au64[0];
        pu128Dst->au64[1] = pu128Src->au64[1];
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu128Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Fetches a data dqword (double qword), generally SSE related.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu128Dst            Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
void iemMemFetchDataU128Jmp(PVMCPUCC pVCpu, PRTUINT128U pu128Dst, uint8_t iSegReg, RTGCPTR GCPtrMem) IEM_NOEXCEPT_MAY_LONGJMP
{
    /* The lazy approach for now... */
    PCRTUINT128U pu128Src = (PCRTUINT128U)iemMemMapJmp(pVCpu, sizeof(*pu128Src), iSegReg, GCPtrMem,
                                                       IEM_ACCESS_DATA_R, 0 /* NO_AC variant */);
    pu128Dst->au64[0] = pu128Src->au64[0];
    pu128Dst->au64[1] = pu128Src->au64[1];
    iemMemCommitAndUnmapJmp(pVCpu, (void *)pu128Src, IEM_ACCESS_DATA_R);
}
#endif


/**
 * Fetches a data dqword (double qword) at an aligned address, generally SSE
 * related.
 *
 * Raises \#GP(0) if not aligned.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu128Dst            Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
VBOXSTRICTRC iemMemFetchDataU128AlignedSse(PVMCPUCC pVCpu, PRTUINT128U pu128Dst, uint8_t iSegReg, RTGCPTR GCPtrMem) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    PCRTUINT128U pu128Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu128Src, sizeof(*pu128Src), iSegReg, GCPtrMem,
                                IEM_ACCESS_DATA_R, (sizeof(*pu128Src) - 1) | IEM_MEMMAP_F_ALIGN_GP | IEM_MEMMAP_F_ALIGN_SSE);
    if (rc == VINF_SUCCESS)
    {
        pu128Dst->au64[0] = pu128Src->au64[0];
        pu128Dst->au64[1] = pu128Src->au64[1];
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu128Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Fetches a data dqword (double qword) at an aligned address, generally SSE
 * related, longjmp on error.
 *
 * Raises \#GP(0) if not aligned.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu128Dst            Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
void iemMemFetchDataU128AlignedSseJmp(PVMCPUCC pVCpu, PRTUINT128U pu128Dst, uint8_t iSegReg,
                                      RTGCPTR GCPtrMem) IEM_NOEXCEPT_MAY_LONGJMP
{
    /* The lazy approach for now... */
    PCRTUINT128U pu128Src = (PCRTUINT128U)iemMemMapJmp(pVCpu, sizeof(*pu128Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R,
                                                       (sizeof(*pu128Src) - 1) | IEM_MEMMAP_F_ALIGN_GP | IEM_MEMMAP_F_ALIGN_SSE);
    pu128Dst->au64[0] = pu128Src->au64[0];
    pu128Dst->au64[1] = pu128Src->au64[1];
    iemMemCommitAndUnmapJmp(pVCpu, (void *)pu128Src, IEM_ACCESS_DATA_R);
}
#endif


/**
 * Fetches a data oword (octo word), generally AVX related.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu256Dst            Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
VBOXSTRICTRC iemMemFetchDataU256(PVMCPUCC pVCpu, PRTUINT256U pu256Dst, uint8_t iSegReg, RTGCPTR GCPtrMem) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    PCRTUINT256U pu256Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu256Src, sizeof(*pu256Src), iSegReg, GCPtrMem,
                                IEM_ACCESS_DATA_R, 0 /* NO_AC variant */);
    if (rc == VINF_SUCCESS)
    {
        pu256Dst->au64[0] = pu256Src->au64[0];
        pu256Dst->au64[1] = pu256Src->au64[1];
        pu256Dst->au64[2] = pu256Src->au64[2];
        pu256Dst->au64[3] = pu256Src->au64[3];
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu256Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Fetches a data oword (octo word), generally AVX related.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu256Dst            Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
void iemMemFetchDataU256Jmp(PVMCPUCC pVCpu, PRTUINT256U pu256Dst, uint8_t iSegReg, RTGCPTR GCPtrMem) IEM_NOEXCEPT_MAY_LONGJMP
{
    /* The lazy approach for now... */
    PCRTUINT256U pu256Src = (PCRTUINT256U)iemMemMapJmp(pVCpu, sizeof(*pu256Src), iSegReg, GCPtrMem,
                                                       IEM_ACCESS_DATA_R, 0 /* NO_AC variant */);
    pu256Dst->au64[0] = pu256Src->au64[0];
    pu256Dst->au64[1] = pu256Src->au64[1];
    pu256Dst->au64[2] = pu256Src->au64[2];
    pu256Dst->au64[3] = pu256Src->au64[3];
    iemMemCommitAndUnmapJmp(pVCpu, (void *)pu256Src, IEM_ACCESS_DATA_R);
}
#endif


/**
 * Fetches a data oword (octo word) at an aligned address, generally AVX
 * related.
 *
 * Raises \#GP(0) if not aligned.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu256Dst            Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
VBOXSTRICTRC iemMemFetchDataU256AlignedSse(PVMCPUCC pVCpu, PRTUINT256U pu256Dst, uint8_t iSegReg, RTGCPTR GCPtrMem) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    PCRTUINT256U pu256Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu256Src, sizeof(*pu256Src), iSegReg, GCPtrMem,
                                IEM_ACCESS_DATA_R, (sizeof(*pu256Src) - 1) | IEM_MEMMAP_F_ALIGN_GP | IEM_MEMMAP_F_ALIGN_SSE);
    if (rc == VINF_SUCCESS)
    {
        pu256Dst->au64[0] = pu256Src->au64[0];
        pu256Dst->au64[1] = pu256Src->au64[1];
        pu256Dst->au64[2] = pu256Src->au64[2];
        pu256Dst->au64[3] = pu256Src->au64[3];
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu256Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Fetches a data oword (octo word) at an aligned address, generally AVX
 * related, longjmp on error.
 *
 * Raises \#GP(0) if not aligned.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu256Dst            Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
void iemMemFetchDataU256AlignedSseJmp(PVMCPUCC pVCpu, PRTUINT256U pu256Dst, uint8_t iSegReg,
                                      RTGCPTR GCPtrMem) IEM_NOEXCEPT_MAY_LONGJMP
{
    /* The lazy approach for now... */
    PCRTUINT256U pu256Src = (PCRTUINT256U)iemMemMapJmp(pVCpu, sizeof(*pu256Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R,
                                                       (sizeof(*pu256Src) - 1) | IEM_MEMMAP_F_ALIGN_GP | IEM_MEMMAP_F_ALIGN_SSE);
    pu256Dst->au64[0] = pu256Src->au64[0];
    pu256Dst->au64[1] = pu256Src->au64[1];
    pu256Dst->au64[2] = pu256Src->au64[2];
    pu256Dst->au64[3] = pu256Src->au64[3];
    iemMemCommitAndUnmapJmp(pVCpu, (void *)pu256Src, IEM_ACCESS_DATA_R);
}
#endif



/**
 * Fetches a descriptor register (lgdt, lidt).
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pcbLimit            Where to return the limit.
 * @param   pGCPtrBase          Where to return the base.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   enmOpSize           The effective operand size.
 */
VBOXSTRICTRC iemMemFetchDataXdtr(PVMCPUCC pVCpu, uint16_t *pcbLimit, PRTGCPTR pGCPtrBase, uint8_t iSegReg,
                                 RTGCPTR GCPtrMem, IEMMODE enmOpSize) RT_NOEXCEPT
{
    /*
     * Just like SIDT and SGDT, the LIDT and LGDT instructions are a
     * little special:
     *      - The two reads are done separately.
     *      - Operand size override works in 16-bit and 32-bit code, but 64-bit.
     *      - We suspect the 386 to actually commit the limit before the base in
     *        some cases (search for 386 in  bs3CpuBasic2_lidt_lgdt_One).  We
     *        don't try emulate this eccentric behavior, because it's not well
     *        enough understood and rather hard to trigger.
     *      - The 486 seems to do a dword limit read when the operand size is 32-bit.
     */
    VBOXSTRICTRC rcStrict;
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        rcStrict = iemMemFetchDataU16(pVCpu, pcbLimit, iSegReg, GCPtrMem);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemFetchDataU64(pVCpu, pGCPtrBase, iSegReg, GCPtrMem + 2);
    }
    else
    {
        uint32_t uTmp = 0; /* (Visual C++ maybe used uninitialized) */
        if (enmOpSize == IEMMODE_32BIT)
        {
            if (IEM_GET_TARGET_CPU(pVCpu) != IEMTARGETCPU_486)
            {
                rcStrict = iemMemFetchDataU16(pVCpu, pcbLimit, iSegReg, GCPtrMem);
                if (rcStrict == VINF_SUCCESS)
                    rcStrict = iemMemFetchDataU32(pVCpu, &uTmp, iSegReg, GCPtrMem + 2);
            }
            else
            {
                rcStrict = iemMemFetchDataU32(pVCpu, &uTmp, iSegReg, GCPtrMem);
                if (rcStrict == VINF_SUCCESS)
                {
                    *pcbLimit = (uint16_t)uTmp;
                    rcStrict = iemMemFetchDataU32(pVCpu, &uTmp, iSegReg, GCPtrMem + 2);
                }
            }
            if (rcStrict == VINF_SUCCESS)
                *pGCPtrBase = uTmp;
        }
        else
        {
            rcStrict = iemMemFetchDataU16(pVCpu, pcbLimit, iSegReg, GCPtrMem);
            if (rcStrict == VINF_SUCCESS)
            {
                rcStrict = iemMemFetchDataU32(pVCpu, &uTmp, iSegReg, GCPtrMem + 2);
                if (rcStrict == VINF_SUCCESS)
                    *pGCPtrBase = uTmp & UINT32_C(0x00ffffff);
            }
        }
    }
    return rcStrict;
}



/**
 * Stores a data byte.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u8Value             The value to store.
 */
VBOXSTRICTRC iemMemStoreDataU8(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, uint8_t u8Value) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    uint8_t *pu8Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu8Dst, sizeof(*pu8Dst), iSegReg, GCPtrMem, IEM_ACCESS_DATA_W, 0);
    if (rc == VINF_SUCCESS)
    {
        *pu8Dst = u8Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu8Dst, IEM_ACCESS_DATA_W);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Stores a data byte, longjmp on error.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u8Value             The value to store.
 */
void iemMemStoreDataU8Jmp(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, uint8_t u8Value) IEM_NOEXCEPT_MAY_LONGJMP
{
    /* The lazy approach for now... */
    uint8_t *pu8Dst = (uint8_t *)iemMemMapJmp(pVCpu, sizeof(*pu8Dst), iSegReg, GCPtrMem, IEM_ACCESS_DATA_W, 0);
    *pu8Dst = u8Value;
    iemMemCommitAndUnmapJmp(pVCpu, pu8Dst, IEM_ACCESS_DATA_W);
}
#endif


/**
 * Stores a data word.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u16Value            The value to store.
 */
VBOXSTRICTRC iemMemStoreDataU16(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, uint16_t u16Value) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    uint16_t *pu16Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu16Dst, sizeof(*pu16Dst), iSegReg, GCPtrMem,
                                IEM_ACCESS_DATA_W, sizeof(*pu16Dst) - 1);
    if (rc == VINF_SUCCESS)
    {
        *pu16Dst = u16Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu16Dst, IEM_ACCESS_DATA_W);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Stores a data word, longjmp on error.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u16Value            The value to store.
 */
void iemMemStoreDataU16Jmp(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, uint16_t u16Value) IEM_NOEXCEPT_MAY_LONGJMP
{
    /* The lazy approach for now... */
    uint16_t *pu16Dst = (uint16_t *)iemMemMapJmp(pVCpu, sizeof(*pu16Dst), iSegReg, GCPtrMem,
                                                 IEM_ACCESS_DATA_W, sizeof(*pu16Dst) - 1);
    *pu16Dst = u16Value;
    iemMemCommitAndUnmapJmp(pVCpu, pu16Dst, IEM_ACCESS_DATA_W);
}
#endif


/**
 * Stores a data dword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u32Value            The value to store.
 */
VBOXSTRICTRC iemMemStoreDataU32(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, uint32_t u32Value) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    uint32_t *pu32Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu32Dst, sizeof(*pu32Dst), iSegReg, GCPtrMem,
                                IEM_ACCESS_DATA_W, sizeof(*pu32Dst) - 1);
    if (rc == VINF_SUCCESS)
    {
        *pu32Dst = u32Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu32Dst, IEM_ACCESS_DATA_W);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Stores a data dword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u32Value            The value to store.
 */
void iemMemStoreDataU32Jmp(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, uint32_t u32Value) IEM_NOEXCEPT_MAY_LONGJMP
{
    /* The lazy approach for now... */
    uint32_t *pu32Dst = (uint32_t *)iemMemMapJmp(pVCpu, sizeof(*pu32Dst), iSegReg, GCPtrMem,
                                                 IEM_ACCESS_DATA_W, sizeof(*pu32Dst) - 1);
    *pu32Dst = u32Value;
    iemMemCommitAndUnmapJmp(pVCpu, pu32Dst, IEM_ACCESS_DATA_W);
}
#endif


/**
 * Stores a data qword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u64Value            The value to store.
 */
VBOXSTRICTRC iemMemStoreDataU64(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, uint64_t u64Value) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    uint64_t *pu64Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu64Dst, sizeof(*pu64Dst), iSegReg, GCPtrMem,
                                IEM_ACCESS_DATA_W, sizeof(*pu64Dst) - 1);
    if (rc == VINF_SUCCESS)
    {
        *pu64Dst = u64Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu64Dst, IEM_ACCESS_DATA_W);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Stores a data qword, longjmp on error.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u64Value            The value to store.
 */
void iemMemStoreDataU64Jmp(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, uint64_t u64Value) IEM_NOEXCEPT_MAY_LONGJMP
{
    /* The lazy approach for now... */
    uint64_t *pu64Dst = (uint64_t *)iemMemMapJmp(pVCpu, sizeof(*pu64Dst), iSegReg, GCPtrMem,
                                                 IEM_ACCESS_DATA_W, sizeof(*pu64Dst) - 1);
    *pu64Dst = u64Value;
    iemMemCommitAndUnmapJmp(pVCpu, pu64Dst, IEM_ACCESS_DATA_W);
}
#endif


/**
 * Stores a data dqword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u128Value            The value to store.
 */
VBOXSTRICTRC iemMemStoreDataU128(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, RTUINT128U u128Value) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    PRTUINT128U pu128Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu128Dst, sizeof(*pu128Dst), iSegReg, GCPtrMem,
                                IEM_ACCESS_DATA_W, 0 /* NO_AC variant */);
    if (rc == VINF_SUCCESS)
    {
        pu128Dst->au64[0] = u128Value.au64[0];
        pu128Dst->au64[1] = u128Value.au64[1];
        rc = iemMemCommitAndUnmap(pVCpu, pu128Dst, IEM_ACCESS_DATA_W);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Stores a data dqword, longjmp on error.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u128Value            The value to store.
 */
void iemMemStoreDataU128Jmp(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, RTUINT128U u128Value) IEM_NOEXCEPT_MAY_LONGJMP
{
    /* The lazy approach for now... */
    PRTUINT128U pu128Dst = (PRTUINT128U)iemMemMapJmp(pVCpu, sizeof(*pu128Dst), iSegReg, GCPtrMem,
                                                     IEM_ACCESS_DATA_W, 0 /* NO_AC variant */);
    pu128Dst->au64[0] = u128Value.au64[0];
    pu128Dst->au64[1] = u128Value.au64[1];
    iemMemCommitAndUnmapJmp(pVCpu, pu128Dst, IEM_ACCESS_DATA_W);
}
#endif


/**
 * Stores a data dqword, SSE aligned.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u128Value           The value to store.
 */
VBOXSTRICTRC iemMemStoreDataU128AlignedSse(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, RTUINT128U u128Value) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    PRTUINT128U pu128Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu128Dst, sizeof(*pu128Dst), iSegReg, GCPtrMem, IEM_ACCESS_DATA_W,
                                (sizeof(*pu128Dst) - 1) | IEM_MEMMAP_F_ALIGN_GP | IEM_MEMMAP_F_ALIGN_SSE);
    if (rc == VINF_SUCCESS)
    {
        pu128Dst->au64[0] = u128Value.au64[0];
        pu128Dst->au64[1] = u128Value.au64[1];
        rc = iemMemCommitAndUnmap(pVCpu, pu128Dst, IEM_ACCESS_DATA_W);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Stores a data dqword, SSE aligned.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u128Value           The value to store.
 */
void iemMemStoreDataU128AlignedSseJmp(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem,
                                      RTUINT128U u128Value) IEM_NOEXCEPT_MAY_LONGJMP
{
    /* The lazy approach for now... */
    PRTUINT128U pu128Dst = (PRTUINT128U)iemMemMapJmp(pVCpu, sizeof(*pu128Dst), iSegReg, GCPtrMem, IEM_ACCESS_DATA_W,
                                                     (sizeof(*pu128Dst) - 1) | IEM_MEMMAP_F_ALIGN_GP | IEM_MEMMAP_F_ALIGN_SSE);
    pu128Dst->au64[0] = u128Value.au64[0];
    pu128Dst->au64[1] = u128Value.au64[1];
    iemMemCommitAndUnmapJmp(pVCpu, pu128Dst, IEM_ACCESS_DATA_W);
}
#endif


/**
 * Stores a data dqword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   pu256Value          Pointer to the value to store.
 */
VBOXSTRICTRC iemMemStoreDataU256(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, PCRTUINT256U pu256Value) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    PRTUINT256U pu256Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu256Dst, sizeof(*pu256Dst), iSegReg, GCPtrMem,
                                IEM_ACCESS_DATA_W, 0 /* NO_AC variant */);
    if (rc == VINF_SUCCESS)
    {
        pu256Dst->au64[0] = pu256Value->au64[0];
        pu256Dst->au64[1] = pu256Value->au64[1];
        pu256Dst->au64[2] = pu256Value->au64[2];
        pu256Dst->au64[3] = pu256Value->au64[3];
        rc = iemMemCommitAndUnmap(pVCpu, pu256Dst, IEM_ACCESS_DATA_W);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Stores a data dqword, longjmp on error.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   pu256Value          Pointer to the value to store.
 */
void iemMemStoreDataU256Jmp(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, PCRTUINT256U pu256Value) IEM_NOEXCEPT_MAY_LONGJMP
{
    /* The lazy approach for now... */
    PRTUINT256U pu256Dst = (PRTUINT256U)iemMemMapJmp(pVCpu, sizeof(*pu256Dst), iSegReg, GCPtrMem,
                                                     IEM_ACCESS_DATA_W, 0 /* NO_AC variant */);
    pu256Dst->au64[0] = pu256Value->au64[0];
    pu256Dst->au64[1] = pu256Value->au64[1];
    pu256Dst->au64[2] = pu256Value->au64[2];
    pu256Dst->au64[3] = pu256Value->au64[3];
    iemMemCommitAndUnmapJmp(pVCpu, pu256Dst, IEM_ACCESS_DATA_W);
}
#endif


/**
 * Stores a data dqword, AVX \#GP(0) aligned.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   pu256Value          Pointer to the value to store.
 */
VBOXSTRICTRC iemMemStoreDataU256AlignedAvx(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, PCRTUINT256U pu256Value) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    PRTUINT256U pu256Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu256Dst, sizeof(*pu256Dst), iSegReg, GCPtrMem,
                                IEM_ACCESS_DATA_W, (sizeof(*pu256Dst) - 1) | IEM_MEMMAP_F_ALIGN_GP);
    if (rc == VINF_SUCCESS)
    {
        pu256Dst->au64[0] = pu256Value->au64[0];
        pu256Dst->au64[1] = pu256Value->au64[1];
        pu256Dst->au64[2] = pu256Value->au64[2];
        pu256Dst->au64[3] = pu256Value->au64[3];
        rc = iemMemCommitAndUnmap(pVCpu, pu256Dst, IEM_ACCESS_DATA_W);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Stores a data dqword, AVX aligned.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   pu256Value          Pointer to the value to store.
 */
void iemMemStoreDataU256AlignedAvxJmp(PVMCPUCC pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem,
                                      PCRTUINT256U pu256Value) IEM_NOEXCEPT_MAY_LONGJMP
{
    /* The lazy approach for now... */
    PRTUINT256U pu256Dst = (PRTUINT256U)iemMemMapJmp(pVCpu, sizeof(*pu256Dst), iSegReg, GCPtrMem,
                                                     IEM_ACCESS_DATA_W, (sizeof(*pu256Dst) - 1) | IEM_MEMMAP_F_ALIGN_GP);
    pu256Dst->au64[0] = pu256Value->au64[0];
    pu256Dst->au64[1] = pu256Value->au64[1];
    pu256Dst->au64[2] = pu256Value->au64[2];
    pu256Dst->au64[3] = pu256Value->au64[3];
    iemMemCommitAndUnmapJmp(pVCpu, pu256Dst, IEM_ACCESS_DATA_W);
}
#endif


/**
 * Stores a descriptor register (sgdt, sidt).
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   cbLimit             The limit.
 * @param   GCPtrBase           The base address.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
VBOXSTRICTRC iemMemStoreDataXdtr(PVMCPUCC pVCpu, uint16_t cbLimit, RTGCPTR GCPtrBase, uint8_t iSegReg, RTGCPTR GCPtrMem) RT_NOEXCEPT
{
    /*
     * The SIDT and SGDT instructions actually stores the data using two
     * independent writes (see bs3CpuBasic2_sidt_sgdt_One).  The instructions
     * does not respond to opsize prefixes.
     */
    VBOXSTRICTRC rcStrict = iemMemStoreDataU16(pVCpu, iSegReg, GCPtrMem, cbLimit);
    if (rcStrict == VINF_SUCCESS)
    {
        if (pVCpu->iem.s.enmCpuMode == IEMMODE_16BIT)
            rcStrict = iemMemStoreDataU32(pVCpu, iSegReg, GCPtrMem + 2,
                                          IEM_GET_TARGET_CPU(pVCpu) <= IEMTARGETCPU_286
                                          ? (uint32_t)GCPtrBase | UINT32_C(0xff000000) : (uint32_t)GCPtrBase);
        else if (pVCpu->iem.s.enmCpuMode == IEMMODE_32BIT)
            rcStrict = iemMemStoreDataU32(pVCpu, iSegReg, GCPtrMem + 2, (uint32_t)GCPtrBase);
        else
            rcStrict = iemMemStoreDataU64(pVCpu, iSegReg, GCPtrMem + 2, GCPtrBase);
    }
    return rcStrict;
}


/**
 * Pushes a word onto the stack.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u16Value            The value to push.
 */
VBOXSTRICTRC iemMemStackPushU16(PVMCPUCC pVCpu, uint16_t u16Value) RT_NOEXCEPT
{
    /* Increment the stack pointer. */
    uint64_t    uNewRsp;
    RTGCPTR     GCPtrTop = iemRegGetRspForPush(pVCpu, 2, &uNewRsp);

    /* Write the word the lazy way. */
    uint16_t *pu16Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu16Dst, sizeof(*pu16Dst), X86_SREG_SS, GCPtrTop,
                                IEM_ACCESS_STACK_W, sizeof(*pu16Dst) - 1);
    if (rc == VINF_SUCCESS)
    {
        *pu16Dst = u16Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu16Dst, IEM_ACCESS_STACK_W);
    }

    /* Commit the new RSP value unless we an access handler made trouble. */
    if (rc == VINF_SUCCESS)
        pVCpu->cpum.GstCtx.rsp = uNewRsp;

    return rc;
}


/**
 * Pushes a dword onto the stack.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u32Value            The value to push.
 */
VBOXSTRICTRC iemMemStackPushU32(PVMCPUCC pVCpu, uint32_t u32Value) RT_NOEXCEPT
{
    /* Increment the stack pointer. */
    uint64_t    uNewRsp;
    RTGCPTR     GCPtrTop = iemRegGetRspForPush(pVCpu, 4, &uNewRsp);

    /* Write the dword the lazy way. */
    uint32_t *pu32Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu32Dst, sizeof(*pu32Dst), X86_SREG_SS, GCPtrTop,
                                IEM_ACCESS_STACK_W, sizeof(*pu32Dst) - 1);
    if (rc == VINF_SUCCESS)
    {
        *pu32Dst = u32Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu32Dst, IEM_ACCESS_STACK_W);
    }

    /* Commit the new RSP value unless we an access handler made trouble. */
    if (rc == VINF_SUCCESS)
        pVCpu->cpum.GstCtx.rsp = uNewRsp;

    return rc;
}


/**
 * Pushes a dword segment register value onto the stack.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u32Value            The value to push.
 */
VBOXSTRICTRC iemMemStackPushU32SReg(PVMCPUCC pVCpu, uint32_t u32Value) RT_NOEXCEPT
{
    /* Increment the stack pointer. */
    uint64_t    uNewRsp;
    RTGCPTR     GCPtrTop = iemRegGetRspForPush(pVCpu, 4, &uNewRsp);

    /* The intel docs talks about zero extending the selector register
       value.  My actual intel CPU here might be zero extending the value
       but it still only writes the lower word... */
    /** @todo Test this on new HW and on AMD and in 64-bit mode.  Also test what
     * happens when crossing an electric page boundrary, is the high word checked
     * for write accessibility or not? Probably it is.  What about segment limits?
     * It appears this behavior is also shared with trap error codes.
     *
     * Docs indicate the behavior changed maybe in Pentium or Pentium Pro. Check
     * ancient hardware when it actually did change. */
    uint16_t *pu16Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu16Dst, sizeof(uint32_t), X86_SREG_SS, GCPtrTop,
                                IEM_ACCESS_STACK_RW, sizeof(*pu16Dst) - 1); /** @todo 2 or 4 alignment check for PUSH SS? */
    if (rc == VINF_SUCCESS)
    {
        *pu16Dst = (uint16_t)u32Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu16Dst, IEM_ACCESS_STACK_RW);
    }

    /* Commit the new RSP value unless we an access handler made trouble. */
    if (rc == VINF_SUCCESS)
        pVCpu->cpum.GstCtx.rsp = uNewRsp;

    return rc;
}


/**
 * Pushes a qword onto the stack.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u64Value            The value to push.
 */
VBOXSTRICTRC iemMemStackPushU64(PVMCPUCC pVCpu, uint64_t u64Value) RT_NOEXCEPT
{
    /* Increment the stack pointer. */
    uint64_t    uNewRsp;
    RTGCPTR     GCPtrTop = iemRegGetRspForPush(pVCpu, 8, &uNewRsp);

    /* Write the word the lazy way. */
    uint64_t *pu64Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu64Dst, sizeof(*pu64Dst), X86_SREG_SS, GCPtrTop,
                                IEM_ACCESS_STACK_W, sizeof(*pu64Dst) - 1);
    if (rc == VINF_SUCCESS)
    {
        *pu64Dst = u64Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu64Dst, IEM_ACCESS_STACK_W);
    }

    /* Commit the new RSP value unless we an access handler made trouble. */
    if (rc == VINF_SUCCESS)
        pVCpu->cpum.GstCtx.rsp = uNewRsp;

    return rc;
}


/**
 * Pops a word from the stack.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu16Value           Where to store the popped value.
 */
VBOXSTRICTRC iemMemStackPopU16(PVMCPUCC pVCpu, uint16_t *pu16Value) RT_NOEXCEPT
{
    /* Increment the stack pointer. */
    uint64_t    uNewRsp;
    RTGCPTR     GCPtrTop = iemRegGetRspForPop(pVCpu, 2, &uNewRsp);

    /* Write the word the lazy way. */
    uint16_t const *pu16Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu16Src, sizeof(*pu16Src), X86_SREG_SS, GCPtrTop,
                                IEM_ACCESS_STACK_R, sizeof(*pu16Src) - 1);
    if (rc == VINF_SUCCESS)
    {
        *pu16Value = *pu16Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu16Src, IEM_ACCESS_STACK_R);

        /* Commit the new RSP value. */
        if (rc == VINF_SUCCESS)
            pVCpu->cpum.GstCtx.rsp = uNewRsp;
    }

    return rc;
}


/**
 * Pops a dword from the stack.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu32Value           Where to store the popped value.
 */
VBOXSTRICTRC iemMemStackPopU32(PVMCPUCC pVCpu, uint32_t *pu32Value) RT_NOEXCEPT
{
    /* Increment the stack pointer. */
    uint64_t    uNewRsp;
    RTGCPTR     GCPtrTop = iemRegGetRspForPop(pVCpu, 4, &uNewRsp);

    /* Write the word the lazy way. */
    uint32_t const *pu32Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu32Src, sizeof(*pu32Src), X86_SREG_SS, GCPtrTop,
                                IEM_ACCESS_STACK_R, sizeof(*pu32Src) - 1);
    if (rc == VINF_SUCCESS)
    {
        *pu32Value = *pu32Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu32Src, IEM_ACCESS_STACK_R);

        /* Commit the new RSP value. */
        if (rc == VINF_SUCCESS)
            pVCpu->cpum.GstCtx.rsp = uNewRsp;
    }

    return rc;
}


/**
 * Pops a qword from the stack.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64Value           Where to store the popped value.
 */
VBOXSTRICTRC iemMemStackPopU64(PVMCPUCC pVCpu, uint64_t *pu64Value) RT_NOEXCEPT
{
    /* Increment the stack pointer. */
    uint64_t    uNewRsp;
    RTGCPTR     GCPtrTop = iemRegGetRspForPop(pVCpu, 8, &uNewRsp);

    /* Write the word the lazy way. */
    uint64_t const *pu64Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu64Src, sizeof(*pu64Src), X86_SREG_SS, GCPtrTop,
                                IEM_ACCESS_STACK_R, sizeof(*pu64Src) - 1);
    if (rc == VINF_SUCCESS)
    {
        *pu64Value = *pu64Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu64Src, IEM_ACCESS_STACK_R);

        /* Commit the new RSP value. */
        if (rc == VINF_SUCCESS)
            pVCpu->cpum.GstCtx.rsp = uNewRsp;
    }

    return rc;
}


/**
 * Pushes a word onto the stack, using a temporary stack pointer.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u16Value            The value to push.
 * @param   pTmpRsp             Pointer to the temporary stack pointer.
 */
VBOXSTRICTRC iemMemStackPushU16Ex(PVMCPUCC pVCpu, uint16_t u16Value, PRTUINT64U pTmpRsp) RT_NOEXCEPT
{
    /* Increment the stack pointer. */
    RTUINT64U   NewRsp = *pTmpRsp;
    RTGCPTR     GCPtrTop = iemRegGetRspForPushEx(pVCpu, &NewRsp, 2);

    /* Write the word the lazy way. */
    uint16_t *pu16Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu16Dst, sizeof(*pu16Dst), X86_SREG_SS, GCPtrTop,
                                IEM_ACCESS_STACK_W, sizeof(*pu16Dst) - 1);
    if (rc == VINF_SUCCESS)
    {
        *pu16Dst = u16Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu16Dst, IEM_ACCESS_STACK_W);
    }

    /* Commit the new RSP value unless we an access handler made trouble. */
    if (rc == VINF_SUCCESS)
        *pTmpRsp = NewRsp;

    return rc;
}


/**
 * Pushes a dword onto the stack, using a temporary stack pointer.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u32Value            The value to push.
 * @param   pTmpRsp             Pointer to the temporary stack pointer.
 */
VBOXSTRICTRC iemMemStackPushU32Ex(PVMCPUCC pVCpu, uint32_t u32Value, PRTUINT64U pTmpRsp) RT_NOEXCEPT
{
    /* Increment the stack pointer. */
    RTUINT64U   NewRsp = *pTmpRsp;
    RTGCPTR     GCPtrTop = iemRegGetRspForPushEx(pVCpu, &NewRsp, 4);

    /* Write the word the lazy way. */
    uint32_t *pu32Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu32Dst, sizeof(*pu32Dst), X86_SREG_SS, GCPtrTop,
                                IEM_ACCESS_STACK_W, sizeof(*pu32Dst) - 1);
    if (rc == VINF_SUCCESS)
    {
        *pu32Dst = u32Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu32Dst, IEM_ACCESS_STACK_W);
    }

    /* Commit the new RSP value unless we an access handler made trouble. */
    if (rc == VINF_SUCCESS)
        *pTmpRsp = NewRsp;

    return rc;
}


/**
 * Pushes a dword onto the stack, using a temporary stack pointer.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u64Value            The value to push.
 * @param   pTmpRsp             Pointer to the temporary stack pointer.
 */
VBOXSTRICTRC iemMemStackPushU64Ex(PVMCPUCC pVCpu, uint64_t u64Value, PRTUINT64U pTmpRsp) RT_NOEXCEPT
{
    /* Increment the stack pointer. */
    RTUINT64U   NewRsp = *pTmpRsp;
    RTGCPTR     GCPtrTop = iemRegGetRspForPushEx(pVCpu, &NewRsp, 8);

    /* Write the word the lazy way. */
    uint64_t *pu64Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu64Dst, sizeof(*pu64Dst), X86_SREG_SS, GCPtrTop,
                                IEM_ACCESS_STACK_W, sizeof(*pu64Dst) - 1);
    if (rc == VINF_SUCCESS)
    {
        *pu64Dst = u64Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu64Dst, IEM_ACCESS_STACK_W);
    }

    /* Commit the new RSP value unless we an access handler made trouble. */
    if (rc == VINF_SUCCESS)
        *pTmpRsp = NewRsp;

    return rc;
}


/**
 * Pops a word from the stack, using a temporary stack pointer.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu16Value           Where to store the popped value.
 * @param   pTmpRsp             Pointer to the temporary stack pointer.
 */
VBOXSTRICTRC iemMemStackPopU16Ex(PVMCPUCC pVCpu, uint16_t *pu16Value, PRTUINT64U pTmpRsp) RT_NOEXCEPT
{
    /* Increment the stack pointer. */
    RTUINT64U   NewRsp = *pTmpRsp;
    RTGCPTR     GCPtrTop = iemRegGetRspForPopEx(pVCpu, &NewRsp, 2);

    /* Write the word the lazy way. */
    uint16_t const *pu16Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu16Src, sizeof(*pu16Src), X86_SREG_SS, GCPtrTop,
                                IEM_ACCESS_STACK_R, sizeof(*pu16Src) - 1);
    if (rc == VINF_SUCCESS)
    {
        *pu16Value = *pu16Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu16Src, IEM_ACCESS_STACK_R);

        /* Commit the new RSP value. */
        if (rc == VINF_SUCCESS)
            *pTmpRsp = NewRsp;
    }

    return rc;
}


/**
 * Pops a dword from the stack, using a temporary stack pointer.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu32Value           Where to store the popped value.
 * @param   pTmpRsp             Pointer to the temporary stack pointer.
 */
VBOXSTRICTRC iemMemStackPopU32Ex(PVMCPUCC pVCpu, uint32_t *pu32Value, PRTUINT64U pTmpRsp) RT_NOEXCEPT
{
    /* Increment the stack pointer. */
    RTUINT64U   NewRsp = *pTmpRsp;
    RTGCPTR     GCPtrTop = iemRegGetRspForPopEx(pVCpu, &NewRsp, 4);

    /* Write the word the lazy way. */
    uint32_t const *pu32Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu32Src, sizeof(*pu32Src), X86_SREG_SS, GCPtrTop,
                                IEM_ACCESS_STACK_R, sizeof(*pu32Src) - 1);
    if (rc == VINF_SUCCESS)
    {
        *pu32Value = *pu32Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu32Src, IEM_ACCESS_STACK_R);

        /* Commit the new RSP value. */
        if (rc == VINF_SUCCESS)
            *pTmpRsp = NewRsp;
    }

    return rc;
}


/**
 * Pops a qword from the stack, using a temporary stack pointer.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64Value           Where to store the popped value.
 * @param   pTmpRsp             Pointer to the temporary stack pointer.
 */
VBOXSTRICTRC iemMemStackPopU64Ex(PVMCPUCC pVCpu, uint64_t *pu64Value, PRTUINT64U pTmpRsp) RT_NOEXCEPT
{
    /* Increment the stack pointer. */
    RTUINT64U   NewRsp = *pTmpRsp;
    RTGCPTR     GCPtrTop = iemRegGetRspForPopEx(pVCpu, &NewRsp, 8);

    /* Write the word the lazy way. */
    uint64_t const *pu64Src;
    VBOXSTRICTRC rcStrict = iemMemMap(pVCpu, (void **)&pu64Src, sizeof(*pu64Src), X86_SREG_SS, GCPtrTop,
                                      IEM_ACCESS_STACK_R, sizeof(*pu64Src) - 1);
    if (rcStrict == VINF_SUCCESS)
    {
        *pu64Value = *pu64Src;
        rcStrict = iemMemCommitAndUnmap(pVCpu, (void *)pu64Src, IEM_ACCESS_STACK_R);

        /* Commit the new RSP value. */
        if (rcStrict == VINF_SUCCESS)
            *pTmpRsp = NewRsp;
    }

    return rcStrict;
}


/**
 * Begin a special stack push (used by interrupt, exceptions and such).
 *
 * This will raise \#SS or \#PF if appropriate.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   cbMem               The number of bytes to push onto the stack.
 * @param   cbAlign             The alignment mask (7, 3, 1).
 * @param   ppvMem              Where to return the pointer to the stack memory.
 *                              As with the other memory functions this could be
 *                              direct access or bounce buffered access, so
 *                              don't commit register until the commit call
 *                              succeeds.
 * @param   puNewRsp            Where to return the new RSP value.  This must be
 *                              passed unchanged to
 *                              iemMemStackPushCommitSpecial().
 */
VBOXSTRICTRC iemMemStackPushBeginSpecial(PVMCPUCC pVCpu, size_t cbMem, uint32_t cbAlign,
                                         void **ppvMem, uint64_t *puNewRsp) RT_NOEXCEPT
{
    Assert(cbMem < UINT8_MAX);
    RTGCPTR     GCPtrTop = iemRegGetRspForPush(pVCpu, (uint8_t)cbMem, puNewRsp);
    return iemMemMap(pVCpu, ppvMem, cbMem, X86_SREG_SS, GCPtrTop,
                     IEM_ACCESS_STACK_W, cbAlign);
}


/**
 * Commits a special stack push (started by iemMemStackPushBeginSpecial).
 *
 * This will update the rSP.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pvMem               The pointer returned by
 *                              iemMemStackPushBeginSpecial().
 * @param   uNewRsp             The new RSP value returned by
 *                              iemMemStackPushBeginSpecial().
 */
VBOXSTRICTRC iemMemStackPushCommitSpecial(PVMCPUCC pVCpu, void *pvMem, uint64_t uNewRsp) RT_NOEXCEPT
{
    VBOXSTRICTRC rcStrict = iemMemCommitAndUnmap(pVCpu, pvMem, IEM_ACCESS_STACK_W);
    if (rcStrict == VINF_SUCCESS)
        pVCpu->cpum.GstCtx.rsp = uNewRsp;
    return rcStrict;
}


/**
 * Begin a special stack pop (used by iret, retf and such).
 *
 * This will raise \#SS or \#PF if appropriate.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   cbMem               The number of bytes to pop from the stack.
 * @param   cbAlign             The alignment mask (7, 3, 1).
 * @param   ppvMem              Where to return the pointer to the stack memory.
 * @param   puNewRsp            Where to return the new RSP value.  This must be
 *                              assigned to CPUMCTX::rsp manually some time
 *                              after iemMemStackPopDoneSpecial() has been
 *                              called.
 */
VBOXSTRICTRC iemMemStackPopBeginSpecial(PVMCPUCC pVCpu, size_t cbMem, uint32_t cbAlign,
                                        void const **ppvMem, uint64_t *puNewRsp) RT_NOEXCEPT
{
    Assert(cbMem < UINT8_MAX);
    RTGCPTR     GCPtrTop = iemRegGetRspForPop(pVCpu, (uint8_t)cbMem, puNewRsp);
    return iemMemMap(pVCpu, (void **)ppvMem, cbMem, X86_SREG_SS, GCPtrTop, IEM_ACCESS_STACK_R, cbAlign);
}


/**
 * Continue a special stack pop (used by iret and retf), for the purpose of
 * retrieving a new stack pointer.
 *
 * This will raise \#SS or \#PF if appropriate.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   off                 Offset from the top of the stack. This is zero
 *                              except in the retf case.
 * @param   cbMem               The number of bytes to pop from the stack.
 * @param   ppvMem              Where to return the pointer to the stack memory.
 * @param   uCurNewRsp          The current uncommitted RSP value.  (No need to
 *                              return this because all use of this function is
 *                              to retrieve a new value and anything we return
 *                              here would be discarded.)
 */
VBOXSTRICTRC iemMemStackPopContinueSpecial(PVMCPUCC pVCpu, size_t off, size_t cbMem,
                                           void const **ppvMem, uint64_t uCurNewRsp) RT_NOEXCEPT
{
    Assert(cbMem < UINT8_MAX);

    /* The essense of iemRegGetRspForPopEx and friends: */ /** @todo put this into a inlined function? */
    RTGCPTR GCPtrTop;
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        GCPtrTop = uCurNewRsp;
    else if (pVCpu->cpum.GstCtx.ss.Attr.n.u1DefBig)
        GCPtrTop = (uint32_t)uCurNewRsp;
    else
        GCPtrTop = (uint16_t)uCurNewRsp;

    return iemMemMap(pVCpu, (void **)ppvMem, cbMem, X86_SREG_SS, GCPtrTop + off, IEM_ACCESS_STACK_R,
                     0 /* checked in iemMemStackPopBeginSpecial */);
}


/**
 * Done with a special stack pop (started by iemMemStackPopBeginSpecial or
 * iemMemStackPopContinueSpecial).
 *
 * The caller will manually commit the rSP.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pvMem               The pointer returned by
 *                              iemMemStackPopBeginSpecial() or
 *                              iemMemStackPopContinueSpecial().
 */
VBOXSTRICTRC iemMemStackPopDoneSpecial(PVMCPUCC pVCpu, void const *pvMem) RT_NOEXCEPT
{
    return iemMemCommitAndUnmap(pVCpu, (void *)pvMem, IEM_ACCESS_STACK_R);
}


/**
 * Fetches a system table byte.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pbDst               Where to return the byte.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
VBOXSTRICTRC iemMemFetchSysU8(PVMCPUCC pVCpu, uint8_t *pbDst, uint8_t iSegReg, RTGCPTR GCPtrMem) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    uint8_t const *pbSrc;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pbSrc, sizeof(*pbSrc), iSegReg, GCPtrMem, IEM_ACCESS_SYS_R, 0);
    if (rc == VINF_SUCCESS)
    {
        *pbDst = *pbSrc;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pbSrc, IEM_ACCESS_SYS_R);
    }
    return rc;
}


/**
 * Fetches a system table word.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu16Dst             Where to return the word.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
VBOXSTRICTRC iemMemFetchSysU16(PVMCPUCC pVCpu, uint16_t *pu16Dst, uint8_t iSegReg, RTGCPTR GCPtrMem) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    uint16_t const *pu16Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu16Src, sizeof(*pu16Src), iSegReg, GCPtrMem, IEM_ACCESS_SYS_R, 0);
    if (rc == VINF_SUCCESS)
    {
        *pu16Dst = *pu16Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu16Src, IEM_ACCESS_SYS_R);
    }
    return rc;
}


/**
 * Fetches a system table dword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu32Dst             Where to return the dword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
VBOXSTRICTRC iemMemFetchSysU32(PVMCPUCC pVCpu, uint32_t *pu32Dst, uint8_t iSegReg, RTGCPTR GCPtrMem) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    uint32_t const *pu32Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu32Src, sizeof(*pu32Src), iSegReg, GCPtrMem, IEM_ACCESS_SYS_R, 0);
    if (rc == VINF_SUCCESS)
    {
        *pu32Dst = *pu32Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu32Src, IEM_ACCESS_SYS_R);
    }
    return rc;
}


/**
 * Fetches a system table qword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64Dst             Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
VBOXSTRICTRC iemMemFetchSysU64(PVMCPUCC pVCpu, uint64_t *pu64Dst, uint8_t iSegReg, RTGCPTR GCPtrMem) RT_NOEXCEPT
{
    /* The lazy approach for now... */
    uint64_t const *pu64Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu64Src, sizeof(*pu64Src), iSegReg, GCPtrMem, IEM_ACCESS_SYS_R, 0);
    if (rc == VINF_SUCCESS)
    {
        *pu64Dst = *pu64Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu64Src, IEM_ACCESS_SYS_R);
    }
    return rc;
}


/**
 * Fetches a descriptor table entry with caller specified error code.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pDesc               Where to return the descriptor table entry.
 * @param   uSel                The selector which table entry to fetch.
 * @param   uXcpt               The exception to raise on table lookup error.
 * @param   uErrorCode          The error code associated with the exception.
 */
static VBOXSTRICTRC iemMemFetchSelDescWithErr(PVMCPUCC pVCpu, PIEMSELDESC pDesc, uint16_t uSel,
                                              uint8_t uXcpt, uint16_t uErrorCode)  RT_NOEXCEPT
{
    AssertPtr(pDesc);
    IEM_CTX_IMPORT_RET(pVCpu, CPUMCTX_EXTRN_GDTR | CPUMCTX_EXTRN_LDTR);

    /** @todo did the 286 require all 8 bytes to be accessible? */
    /*
     * Get the selector table base and check bounds.
     */
    RTGCPTR GCPtrBase;
    if (uSel & X86_SEL_LDT)
    {
        if (   !pVCpu->cpum.GstCtx.ldtr.Attr.n.u1Present
            || (uSel | X86_SEL_RPL_LDT) > pVCpu->cpum.GstCtx.ldtr.u32Limit )
        {
            Log(("iemMemFetchSelDesc: LDT selector %#x is out of bounds (%3x) or ldtr is NP (%#x)\n",
                 uSel, pVCpu->cpum.GstCtx.ldtr.u32Limit, pVCpu->cpum.GstCtx.ldtr.Sel));
            return iemRaiseXcptOrInt(pVCpu, 0, uXcpt, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR,
                                     uErrorCode, 0);
        }

        Assert(pVCpu->cpum.GstCtx.ldtr.Attr.n.u1Present);
        GCPtrBase = pVCpu->cpum.GstCtx.ldtr.u64Base;
    }
    else
    {
        if ((uSel | X86_SEL_RPL_LDT) > pVCpu->cpum.GstCtx.gdtr.cbGdt)
        {
            Log(("iemMemFetchSelDesc: GDT selector %#x is out of bounds (%3x)\n", uSel, pVCpu->cpum.GstCtx.gdtr.cbGdt));
            return iemRaiseXcptOrInt(pVCpu, 0, uXcpt, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR,
                                     uErrorCode, 0);
        }
        GCPtrBase = pVCpu->cpum.GstCtx.gdtr.pGdt;
    }

    /*
     * Read the legacy descriptor and maybe the long mode extensions if
     * required.
     */
    VBOXSTRICTRC rcStrict;
    if (IEM_GET_TARGET_CPU(pVCpu) > IEMTARGETCPU_286)
        rcStrict = iemMemFetchSysU64(pVCpu, &pDesc->Legacy.u, UINT8_MAX, GCPtrBase + (uSel & X86_SEL_MASK));
    else
    {
        rcStrict     = iemMemFetchSysU16(pVCpu, &pDesc->Legacy.au16[0], UINT8_MAX, GCPtrBase + (uSel & X86_SEL_MASK) + 0);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemFetchSysU16(pVCpu, &pDesc->Legacy.au16[1], UINT8_MAX, GCPtrBase + (uSel & X86_SEL_MASK) + 2);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemFetchSysU16(pVCpu, &pDesc->Legacy.au16[2], UINT8_MAX, GCPtrBase + (uSel & X86_SEL_MASK) + 4);
        if (rcStrict == VINF_SUCCESS)
            pDesc->Legacy.au16[3] = 0;
        else
            return rcStrict;
    }

    if (rcStrict == VINF_SUCCESS)
    {
        if (   !IEM_IS_LONG_MODE(pVCpu)
            || pDesc->Legacy.Gen.u1DescType)
            pDesc->Long.au64[1] = 0;
        else if (   (uint32_t)(uSel | X86_SEL_RPL_LDT) + 8
                 <= (uSel & X86_SEL_LDT ? pVCpu->cpum.GstCtx.ldtr.u32Limit : pVCpu->cpum.GstCtx.gdtr.cbGdt))
            rcStrict = iemMemFetchSysU64(pVCpu, &pDesc->Long.au64[1], UINT8_MAX, GCPtrBase + (uSel | X86_SEL_RPL_LDT) + 1);
        else
        {
            Log(("iemMemFetchSelDesc: system selector %#x is out of bounds\n", uSel));
            /** @todo is this the right exception? */
            return iemRaiseXcptOrInt(pVCpu, 0, uXcpt, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, uErrorCode, 0);
        }
    }
    return rcStrict;
}


/**
 * Fetches a descriptor table entry.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pDesc               Where to return the descriptor table entry.
 * @param   uSel                The selector which table entry to fetch.
 * @param   uXcpt               The exception to raise on table lookup error.
 */
VBOXSTRICTRC iemMemFetchSelDesc(PVMCPUCC pVCpu, PIEMSELDESC pDesc, uint16_t uSel, uint8_t uXcpt) RT_NOEXCEPT
{
    return iemMemFetchSelDescWithErr(pVCpu, pDesc, uSel, uXcpt, uSel & X86_SEL_MASK_OFF_RPL);
}


/**
 * Marks the selector descriptor as accessed (only non-system descriptors).
 *
 * This function ASSUMES that iemMemFetchSelDesc has be called previously and
 * will therefore skip the limit checks.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   uSel                The selector.
 */
VBOXSTRICTRC iemMemMarkSelDescAccessed(PVMCPUCC pVCpu, uint16_t uSel) RT_NOEXCEPT
{
    /*
     * Get the selector table base and calculate the entry address.
     */
    RTGCPTR GCPtr = uSel & X86_SEL_LDT
                  ? pVCpu->cpum.GstCtx.ldtr.u64Base
                  : pVCpu->cpum.GstCtx.gdtr.pGdt;
    GCPtr += uSel & X86_SEL_MASK;

    /*
     * ASMAtomicBitSet will assert if the address is misaligned, so do some
     * ugly stuff to avoid this.  This will make sure it's an atomic access
     * as well more or less remove any question about 8-bit or 32-bit accesss.
     */
    VBOXSTRICTRC        rcStrict;
    uint32_t volatile  *pu32;
    if ((GCPtr & 3) == 0)
    {
        /* The normal case, map the 32-bit bits around the accessed bit (40). */
        GCPtr += 2 + 2;
        rcStrict = iemMemMap(pVCpu, (void **)&pu32, 4, UINT8_MAX, GCPtr, IEM_ACCESS_SYS_RW, 0);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        ASMAtomicBitSet(pu32, 8); /* X86_SEL_TYPE_ACCESSED is 1, but it is preceeded by u8BaseHigh1. */
    }
    else
    {
        /* The misaligned GDT/LDT case, map the whole thing. */
        rcStrict = iemMemMap(pVCpu, (void **)&pu32, 8, UINT8_MAX, GCPtr, IEM_ACCESS_SYS_RW, 0);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        switch ((uintptr_t)pu32 & 3)
        {
            case 0: ASMAtomicBitSet(pu32,                         40 + 0 -  0); break;
            case 1: ASMAtomicBitSet((uint8_t volatile *)pu32 + 3, 40 + 0 - 24); break;
            case 2: ASMAtomicBitSet((uint8_t volatile *)pu32 + 2, 40 + 0 - 16); break;
            case 3: ASMAtomicBitSet((uint8_t volatile *)pu32 + 1, 40 + 0 -  8); break;
        }
    }

    return iemMemCommitAndUnmap(pVCpu, (void *)pu32, IEM_ACCESS_SYS_RW);
}

/** @} */

/** @name   Opcode Helpers.
 * @{
 */

/**
 * Calculates the effective address of a ModR/M memory operand.
 *
 * Meant to be used via IEM_MC_CALC_RM_EFF_ADDR.
 *
 * @return  Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   bRm                 The ModRM byte.
 * @param   cbImm               The size of any immediate following the
 *                              effective address opcode bytes. Important for
 *                              RIP relative addressing.
 * @param   pGCPtrEff           Where to return the effective address.
 */
VBOXSTRICTRC iemOpHlpCalcRmEffAddr(PVMCPUCC pVCpu, uint8_t bRm, uint8_t cbImm, PRTGCPTR pGCPtrEff) RT_NOEXCEPT
{
    Log5(("iemOpHlpCalcRmEffAddr: bRm=%#x\n", bRm));
# define SET_SS_DEF() \
    do \
    { \
        if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_SEG_MASK)) \
            pVCpu->iem.s.iEffSeg = X86_SREG_SS; \
    } while (0)

    if (pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT)
    {
/** @todo Check the effective address size crap! */
        if (pVCpu->iem.s.enmEffAddrMode == IEMMODE_16BIT)
        {
            uint16_t u16EffAddr;

            /* Handle the disp16 form with no registers first. */
            if ((bRm & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 6)
                IEM_OPCODE_GET_NEXT_U16(&u16EffAddr);
            else
            {
                /* Get the displacment. */
                switch ((bRm >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
                {
                    case 0:  u16EffAddr = 0;                             break;
                    case 1:  IEM_OPCODE_GET_NEXT_S8_SX_U16(&u16EffAddr); break;
                    case 2:  IEM_OPCODE_GET_NEXT_U16(&u16EffAddr);       break;
                    default: AssertFailedReturn(VERR_IEM_IPE_1); /* (caller checked for these) */
                }

                /* Add the base and index registers to the disp. */
                switch (bRm & X86_MODRM_RM_MASK)
                {
                    case 0: u16EffAddr += pVCpu->cpum.GstCtx.bx + pVCpu->cpum.GstCtx.si; break;
                    case 1: u16EffAddr += pVCpu->cpum.GstCtx.bx + pVCpu->cpum.GstCtx.di; break;
                    case 2: u16EffAddr += pVCpu->cpum.GstCtx.bp + pVCpu->cpum.GstCtx.si; SET_SS_DEF(); break;
                    case 3: u16EffAddr += pVCpu->cpum.GstCtx.bp + pVCpu->cpum.GstCtx.di; SET_SS_DEF(); break;
                    case 4: u16EffAddr += pVCpu->cpum.GstCtx.si;            break;
                    case 5: u16EffAddr += pVCpu->cpum.GstCtx.di;            break;
                    case 6: u16EffAddr += pVCpu->cpum.GstCtx.bp;            SET_SS_DEF(); break;
                    case 7: u16EffAddr += pVCpu->cpum.GstCtx.bx;            break;
                }
            }

            *pGCPtrEff = u16EffAddr;
        }
        else
        {
            Assert(pVCpu->iem.s.enmEffAddrMode == IEMMODE_32BIT);
            uint32_t u32EffAddr;

            /* Handle the disp32 form with no registers first. */
            if ((bRm & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 5)
                IEM_OPCODE_GET_NEXT_U32(&u32EffAddr);
            else
            {
                /* Get the register (or SIB) value. */
                switch ((bRm & X86_MODRM_RM_MASK))
                {
                    case 0: u32EffAddr = pVCpu->cpum.GstCtx.eax; break;
                    case 1: u32EffAddr = pVCpu->cpum.GstCtx.ecx; break;
                    case 2: u32EffAddr = pVCpu->cpum.GstCtx.edx; break;
                    case 3: u32EffAddr = pVCpu->cpum.GstCtx.ebx; break;
                    case 4: /* SIB */
                    {
                        uint8_t bSib; IEM_OPCODE_GET_NEXT_U8(&bSib);

                        /* Get the index and scale it. */
                        switch ((bSib >> X86_SIB_INDEX_SHIFT) & X86_SIB_INDEX_SMASK)
                        {
                            case 0: u32EffAddr = pVCpu->cpum.GstCtx.eax; break;
                            case 1: u32EffAddr = pVCpu->cpum.GstCtx.ecx; break;
                            case 2: u32EffAddr = pVCpu->cpum.GstCtx.edx; break;
                            case 3: u32EffAddr = pVCpu->cpum.GstCtx.ebx; break;
                            case 4: u32EffAddr = 0; /*none */ break;
                            case 5: u32EffAddr = pVCpu->cpum.GstCtx.ebp; break;
                            case 6: u32EffAddr = pVCpu->cpum.GstCtx.esi; break;
                            case 7: u32EffAddr = pVCpu->cpum.GstCtx.edi; break;
                            IEM_NOT_REACHED_DEFAULT_CASE_RET();
                        }
                        u32EffAddr <<= (bSib >> X86_SIB_SCALE_SHIFT) & X86_SIB_SCALE_SMASK;

                        /* add base */
                        switch (bSib & X86_SIB_BASE_MASK)
                        {
                            case 0: u32EffAddr += pVCpu->cpum.GstCtx.eax; break;
                            case 1: u32EffAddr += pVCpu->cpum.GstCtx.ecx; break;
                            case 2: u32EffAddr += pVCpu->cpum.GstCtx.edx; break;
                            case 3: u32EffAddr += pVCpu->cpum.GstCtx.ebx; break;
                            case 4: u32EffAddr += pVCpu->cpum.GstCtx.esp; SET_SS_DEF(); break;
                            case 5:
                                if ((bRm & X86_MODRM_MOD_MASK) != 0)
                                {
                                    u32EffAddr += pVCpu->cpum.GstCtx.ebp;
                                    SET_SS_DEF();
                                }
                                else
                                {
                                    uint32_t u32Disp;
                                    IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                                    u32EffAddr += u32Disp;
                                }
                                break;
                            case 6: u32EffAddr += pVCpu->cpum.GstCtx.esi; break;
                            case 7: u32EffAddr += pVCpu->cpum.GstCtx.edi; break;
                            IEM_NOT_REACHED_DEFAULT_CASE_RET();
                        }
                        break;
                    }
                    case 5: u32EffAddr = pVCpu->cpum.GstCtx.ebp; SET_SS_DEF(); break;
                    case 6: u32EffAddr = pVCpu->cpum.GstCtx.esi; break;
                    case 7: u32EffAddr = pVCpu->cpum.GstCtx.edi; break;
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }

                /* Get and add the displacement. */
                switch ((bRm >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
                {
                    case 0:
                        break;
                    case 1:
                    {
                        int8_t i8Disp; IEM_OPCODE_GET_NEXT_S8(&i8Disp);
                        u32EffAddr += i8Disp;
                        break;
                    }
                    case 2:
                    {
                        uint32_t u32Disp; IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                        u32EffAddr += u32Disp;
                        break;
                    }
                    default:
                        AssertFailedReturn(VERR_IEM_IPE_2); /* (caller checked for these) */
                }

            }
            if (pVCpu->iem.s.enmEffAddrMode == IEMMODE_32BIT)
                *pGCPtrEff = u32EffAddr;
            else
            {
                Assert(pVCpu->iem.s.enmEffAddrMode == IEMMODE_16BIT);
                *pGCPtrEff = u32EffAddr & UINT16_MAX;
            }
        }
    }
    else
    {
        uint64_t u64EffAddr;

        /* Handle the rip+disp32 form with no registers first. */
        if ((bRm & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 5)
        {
            IEM_OPCODE_GET_NEXT_S32_SX_U64(&u64EffAddr);
            u64EffAddr += pVCpu->cpum.GstCtx.rip + IEM_GET_INSTR_LEN(pVCpu) + cbImm;
        }
        else
        {
            /* Get the register (or SIB) value. */
            switch ((bRm & X86_MODRM_RM_MASK) | pVCpu->iem.s.uRexB)
            {
                case  0: u64EffAddr = pVCpu->cpum.GstCtx.rax; break;
                case  1: u64EffAddr = pVCpu->cpum.GstCtx.rcx; break;
                case  2: u64EffAddr = pVCpu->cpum.GstCtx.rdx; break;
                case  3: u64EffAddr = pVCpu->cpum.GstCtx.rbx; break;
                case  5: u64EffAddr = pVCpu->cpum.GstCtx.rbp; SET_SS_DEF(); break;
                case  6: u64EffAddr = pVCpu->cpum.GstCtx.rsi; break;
                case  7: u64EffAddr = pVCpu->cpum.GstCtx.rdi; break;
                case  8: u64EffAddr = pVCpu->cpum.GstCtx.r8;  break;
                case  9: u64EffAddr = pVCpu->cpum.GstCtx.r9;  break;
                case 10: u64EffAddr = pVCpu->cpum.GstCtx.r10; break;
                case 11: u64EffAddr = pVCpu->cpum.GstCtx.r11; break;
                case 13: u64EffAddr = pVCpu->cpum.GstCtx.r13; break;
                case 14: u64EffAddr = pVCpu->cpum.GstCtx.r14; break;
                case 15: u64EffAddr = pVCpu->cpum.GstCtx.r15; break;
                /* SIB */
                case 4:
                case 12:
                {
                    uint8_t bSib; IEM_OPCODE_GET_NEXT_U8(&bSib);

                    /* Get the index and scale it. */
                    switch (((bSib >> X86_SIB_INDEX_SHIFT) & X86_SIB_INDEX_SMASK) | pVCpu->iem.s.uRexIndex)
                    {
                        case  0: u64EffAddr = pVCpu->cpum.GstCtx.rax; break;
                        case  1: u64EffAddr = pVCpu->cpum.GstCtx.rcx; break;
                        case  2: u64EffAddr = pVCpu->cpum.GstCtx.rdx; break;
                        case  3: u64EffAddr = pVCpu->cpum.GstCtx.rbx; break;
                        case  4: u64EffAddr = 0; /*none */ break;
                        case  5: u64EffAddr = pVCpu->cpum.GstCtx.rbp; break;
                        case  6: u64EffAddr = pVCpu->cpum.GstCtx.rsi; break;
                        case  7: u64EffAddr = pVCpu->cpum.GstCtx.rdi; break;
                        case  8: u64EffAddr = pVCpu->cpum.GstCtx.r8;  break;
                        case  9: u64EffAddr = pVCpu->cpum.GstCtx.r9;  break;
                        case 10: u64EffAddr = pVCpu->cpum.GstCtx.r10; break;
                        case 11: u64EffAddr = pVCpu->cpum.GstCtx.r11; break;
                        case 12: u64EffAddr = pVCpu->cpum.GstCtx.r12; break;
                        case 13: u64EffAddr = pVCpu->cpum.GstCtx.r13; break;
                        case 14: u64EffAddr = pVCpu->cpum.GstCtx.r14; break;
                        case 15: u64EffAddr = pVCpu->cpum.GstCtx.r15; break;
                        IEM_NOT_REACHED_DEFAULT_CASE_RET();
                    }
                    u64EffAddr <<= (bSib >> X86_SIB_SCALE_SHIFT) & X86_SIB_SCALE_SMASK;

                    /* add base */
                    switch ((bSib & X86_SIB_BASE_MASK) | pVCpu->iem.s.uRexB)
                    {
                        case  0: u64EffAddr += pVCpu->cpum.GstCtx.rax; break;
                        case  1: u64EffAddr += pVCpu->cpum.GstCtx.rcx; break;
                        case  2: u64EffAddr += pVCpu->cpum.GstCtx.rdx; break;
                        case  3: u64EffAddr += pVCpu->cpum.GstCtx.rbx; break;
                        case  4: u64EffAddr += pVCpu->cpum.GstCtx.rsp; SET_SS_DEF(); break;
                        case  6: u64EffAddr += pVCpu->cpum.GstCtx.rsi; break;
                        case  7: u64EffAddr += pVCpu->cpum.GstCtx.rdi; break;
                        case  8: u64EffAddr += pVCpu->cpum.GstCtx.r8;  break;
                        case  9: u64EffAddr += pVCpu->cpum.GstCtx.r9;  break;
                        case 10: u64EffAddr += pVCpu->cpum.GstCtx.r10; break;
                        case 11: u64EffAddr += pVCpu->cpum.GstCtx.r11; break;
                        case 12: u64EffAddr += pVCpu->cpum.GstCtx.r12; break;
                        case 14: u64EffAddr += pVCpu->cpum.GstCtx.r14; break;
                        case 15: u64EffAddr += pVCpu->cpum.GstCtx.r15; break;
                        /* complicated encodings */
                        case 5:
                        case 13:
                            if ((bRm & X86_MODRM_MOD_MASK) != 0)
                            {
                                if (!pVCpu->iem.s.uRexB)
                                {
                                    u64EffAddr += pVCpu->cpum.GstCtx.rbp;
                                    SET_SS_DEF();
                                }
                                else
                                    u64EffAddr += pVCpu->cpum.GstCtx.r13;
                            }
                            else
                            {
                                uint32_t u32Disp;
                                IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                                u64EffAddr += (int32_t)u32Disp;
                            }
                            break;
                        IEM_NOT_REACHED_DEFAULT_CASE_RET();
                    }
                    break;
                }
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }

            /* Get and add the displacement. */
            switch ((bRm >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
            {
                case 0:
                    break;
                case 1:
                {
                    int8_t i8Disp;
                    IEM_OPCODE_GET_NEXT_S8(&i8Disp);
                    u64EffAddr += i8Disp;
                    break;
                }
                case 2:
                {
                    uint32_t u32Disp;
                    IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                    u64EffAddr += (int32_t)u32Disp;
                    break;
                }
                IEM_NOT_REACHED_DEFAULT_CASE_RET(); /* (caller checked for these) */
            }

        }

        if (pVCpu->iem.s.enmEffAddrMode == IEMMODE_64BIT)
            *pGCPtrEff = u64EffAddr;
        else
        {
            Assert(pVCpu->iem.s.enmEffAddrMode == IEMMODE_32BIT);
            *pGCPtrEff = u64EffAddr & UINT32_MAX;
        }
    }

    Log5(("iemOpHlpCalcRmEffAddr: EffAddr=%#010RGv\n", *pGCPtrEff));
    return VINF_SUCCESS;
}


/**
 * Calculates the effective address of a ModR/M memory operand.
 *
 * Meant to be used via IEM_MC_CALC_RM_EFF_ADDR.
 *
 * @return  Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   bRm                 The ModRM byte.
 * @param   cbImm               The size of any immediate following the
 *                              effective address opcode bytes. Important for
 *                              RIP relative addressing.
 * @param   pGCPtrEff           Where to return the effective address.
 * @param   offRsp              RSP displacement.
 */
VBOXSTRICTRC iemOpHlpCalcRmEffAddrEx(PVMCPUCC pVCpu, uint8_t bRm, uint8_t cbImm, PRTGCPTR pGCPtrEff, int8_t offRsp) RT_NOEXCEPT
{
    Log5(("iemOpHlpCalcRmEffAddr: bRm=%#x\n", bRm));
# define SET_SS_DEF() \
    do \
    { \
        if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_SEG_MASK)) \
            pVCpu->iem.s.iEffSeg = X86_SREG_SS; \
    } while (0)

    if (pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT)
    {
/** @todo Check the effective address size crap! */
        if (pVCpu->iem.s.enmEffAddrMode == IEMMODE_16BIT)
        {
            uint16_t u16EffAddr;

            /* Handle the disp16 form with no registers first. */
            if ((bRm & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 6)
                IEM_OPCODE_GET_NEXT_U16(&u16EffAddr);
            else
            {
                /* Get the displacment. */
                switch ((bRm >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
                {
                    case 0:  u16EffAddr = 0;                             break;
                    case 1:  IEM_OPCODE_GET_NEXT_S8_SX_U16(&u16EffAddr); break;
                    case 2:  IEM_OPCODE_GET_NEXT_U16(&u16EffAddr);       break;
                    default: AssertFailedReturn(VERR_IEM_IPE_1); /* (caller checked for these) */
                }

                /* Add the base and index registers to the disp. */
                switch (bRm & X86_MODRM_RM_MASK)
                {
                    case 0: u16EffAddr += pVCpu->cpum.GstCtx.bx + pVCpu->cpum.GstCtx.si; break;
                    case 1: u16EffAddr += pVCpu->cpum.GstCtx.bx + pVCpu->cpum.GstCtx.di; break;
                    case 2: u16EffAddr += pVCpu->cpum.GstCtx.bp + pVCpu->cpum.GstCtx.si; SET_SS_DEF(); break;
                    case 3: u16EffAddr += pVCpu->cpum.GstCtx.bp + pVCpu->cpum.GstCtx.di; SET_SS_DEF(); break;
                    case 4: u16EffAddr += pVCpu->cpum.GstCtx.si;            break;
                    case 5: u16EffAddr += pVCpu->cpum.GstCtx.di;            break;
                    case 6: u16EffAddr += pVCpu->cpum.GstCtx.bp;            SET_SS_DEF(); break;
                    case 7: u16EffAddr += pVCpu->cpum.GstCtx.bx;            break;
                }
            }

            *pGCPtrEff = u16EffAddr;
        }
        else
        {
            Assert(pVCpu->iem.s.enmEffAddrMode == IEMMODE_32BIT);
            uint32_t u32EffAddr;

            /* Handle the disp32 form with no registers first. */
            if ((bRm & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 5)
                IEM_OPCODE_GET_NEXT_U32(&u32EffAddr);
            else
            {
                /* Get the register (or SIB) value. */
                switch ((bRm & X86_MODRM_RM_MASK))
                {
                    case 0: u32EffAddr = pVCpu->cpum.GstCtx.eax; break;
                    case 1: u32EffAddr = pVCpu->cpum.GstCtx.ecx; break;
                    case 2: u32EffAddr = pVCpu->cpum.GstCtx.edx; break;
                    case 3: u32EffAddr = pVCpu->cpum.GstCtx.ebx; break;
                    case 4: /* SIB */
                    {
                        uint8_t bSib; IEM_OPCODE_GET_NEXT_U8(&bSib);

                        /* Get the index and scale it. */
                        switch ((bSib >> X86_SIB_INDEX_SHIFT) & X86_SIB_INDEX_SMASK)
                        {
                            case 0: u32EffAddr = pVCpu->cpum.GstCtx.eax; break;
                            case 1: u32EffAddr = pVCpu->cpum.GstCtx.ecx; break;
                            case 2: u32EffAddr = pVCpu->cpum.GstCtx.edx; break;
                            case 3: u32EffAddr = pVCpu->cpum.GstCtx.ebx; break;
                            case 4: u32EffAddr = 0; /*none */ break;
                            case 5: u32EffAddr = pVCpu->cpum.GstCtx.ebp; break;
                            case 6: u32EffAddr = pVCpu->cpum.GstCtx.esi; break;
                            case 7: u32EffAddr = pVCpu->cpum.GstCtx.edi; break;
                            IEM_NOT_REACHED_DEFAULT_CASE_RET();
                        }
                        u32EffAddr <<= (bSib >> X86_SIB_SCALE_SHIFT) & X86_SIB_SCALE_SMASK;

                        /* add base */
                        switch (bSib & X86_SIB_BASE_MASK)
                        {
                            case 0: u32EffAddr += pVCpu->cpum.GstCtx.eax; break;
                            case 1: u32EffAddr += pVCpu->cpum.GstCtx.ecx; break;
                            case 2: u32EffAddr += pVCpu->cpum.GstCtx.edx; break;
                            case 3: u32EffAddr += pVCpu->cpum.GstCtx.ebx; break;
                            case 4:
                                u32EffAddr += pVCpu->cpum.GstCtx.esp + offRsp;
                                SET_SS_DEF();
                                break;
                            case 5:
                                if ((bRm & X86_MODRM_MOD_MASK) != 0)
                                {
                                    u32EffAddr += pVCpu->cpum.GstCtx.ebp;
                                    SET_SS_DEF();
                                }
                                else
                                {
                                    uint32_t u32Disp;
                                    IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                                    u32EffAddr += u32Disp;
                                }
                                break;
                            case 6: u32EffAddr += pVCpu->cpum.GstCtx.esi; break;
                            case 7: u32EffAddr += pVCpu->cpum.GstCtx.edi; break;
                            IEM_NOT_REACHED_DEFAULT_CASE_RET();
                        }
                        break;
                    }
                    case 5: u32EffAddr = pVCpu->cpum.GstCtx.ebp; SET_SS_DEF(); break;
                    case 6: u32EffAddr = pVCpu->cpum.GstCtx.esi; break;
                    case 7: u32EffAddr = pVCpu->cpum.GstCtx.edi; break;
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }

                /* Get and add the displacement. */
                switch ((bRm >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
                {
                    case 0:
                        break;
                    case 1:
                    {
                        int8_t i8Disp; IEM_OPCODE_GET_NEXT_S8(&i8Disp);
                        u32EffAddr += i8Disp;
                        break;
                    }
                    case 2:
                    {
                        uint32_t u32Disp; IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                        u32EffAddr += u32Disp;
                        break;
                    }
                    default:
                        AssertFailedReturn(VERR_IEM_IPE_2); /* (caller checked for these) */
                }

            }
            if (pVCpu->iem.s.enmEffAddrMode == IEMMODE_32BIT)
                *pGCPtrEff = u32EffAddr;
            else
            {
                Assert(pVCpu->iem.s.enmEffAddrMode == IEMMODE_16BIT);
                *pGCPtrEff = u32EffAddr & UINT16_MAX;
            }
        }
    }
    else
    {
        uint64_t u64EffAddr;

        /* Handle the rip+disp32 form with no registers first. */
        if ((bRm & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 5)
        {
            IEM_OPCODE_GET_NEXT_S32_SX_U64(&u64EffAddr);
            u64EffAddr += pVCpu->cpum.GstCtx.rip + IEM_GET_INSTR_LEN(pVCpu) + cbImm;
        }
        else
        {
            /* Get the register (or SIB) value. */
            switch ((bRm & X86_MODRM_RM_MASK) | pVCpu->iem.s.uRexB)
            {
                case  0: u64EffAddr = pVCpu->cpum.GstCtx.rax; break;
                case  1: u64EffAddr = pVCpu->cpum.GstCtx.rcx; break;
                case  2: u64EffAddr = pVCpu->cpum.GstCtx.rdx; break;
                case  3: u64EffAddr = pVCpu->cpum.GstCtx.rbx; break;
                case  5: u64EffAddr = pVCpu->cpum.GstCtx.rbp; SET_SS_DEF(); break;
                case  6: u64EffAddr = pVCpu->cpum.GstCtx.rsi; break;
                case  7: u64EffAddr = pVCpu->cpum.GstCtx.rdi; break;
                case  8: u64EffAddr = pVCpu->cpum.GstCtx.r8;  break;
                case  9: u64EffAddr = pVCpu->cpum.GstCtx.r9;  break;
                case 10: u64EffAddr = pVCpu->cpum.GstCtx.r10; break;
                case 11: u64EffAddr = pVCpu->cpum.GstCtx.r11; break;
                case 13: u64EffAddr = pVCpu->cpum.GstCtx.r13; break;
                case 14: u64EffAddr = pVCpu->cpum.GstCtx.r14; break;
                case 15: u64EffAddr = pVCpu->cpum.GstCtx.r15; break;
                /* SIB */
                case 4:
                case 12:
                {
                    uint8_t bSib; IEM_OPCODE_GET_NEXT_U8(&bSib);

                    /* Get the index and scale it. */
                    switch (((bSib >> X86_SIB_INDEX_SHIFT) & X86_SIB_INDEX_SMASK) | pVCpu->iem.s.uRexIndex)
                    {
                        case  0: u64EffAddr = pVCpu->cpum.GstCtx.rax; break;
                        case  1: u64EffAddr = pVCpu->cpum.GstCtx.rcx; break;
                        case  2: u64EffAddr = pVCpu->cpum.GstCtx.rdx; break;
                        case  3: u64EffAddr = pVCpu->cpum.GstCtx.rbx; break;
                        case  4: u64EffAddr = 0; /*none */ break;
                        case  5: u64EffAddr = pVCpu->cpum.GstCtx.rbp; break;
                        case  6: u64EffAddr = pVCpu->cpum.GstCtx.rsi; break;
                        case  7: u64EffAddr = pVCpu->cpum.GstCtx.rdi; break;
                        case  8: u64EffAddr = pVCpu->cpum.GstCtx.r8;  break;
                        case  9: u64EffAddr = pVCpu->cpum.GstCtx.r9;  break;
                        case 10: u64EffAddr = pVCpu->cpum.GstCtx.r10; break;
                        case 11: u64EffAddr = pVCpu->cpum.GstCtx.r11; break;
                        case 12: u64EffAddr = pVCpu->cpum.GstCtx.r12; break;
                        case 13: u64EffAddr = pVCpu->cpum.GstCtx.r13; break;
                        case 14: u64EffAddr = pVCpu->cpum.GstCtx.r14; break;
                        case 15: u64EffAddr = pVCpu->cpum.GstCtx.r15; break;
                        IEM_NOT_REACHED_DEFAULT_CASE_RET();
                    }
                    u64EffAddr <<= (bSib >> X86_SIB_SCALE_SHIFT) & X86_SIB_SCALE_SMASK;

                    /* add base */
                    switch ((bSib & X86_SIB_BASE_MASK) | pVCpu->iem.s.uRexB)
                    {
                        case  0: u64EffAddr += pVCpu->cpum.GstCtx.rax; break;
                        case  1: u64EffAddr += pVCpu->cpum.GstCtx.rcx; break;
                        case  2: u64EffAddr += pVCpu->cpum.GstCtx.rdx; break;
                        case  3: u64EffAddr += pVCpu->cpum.GstCtx.rbx; break;
                        case  4: u64EffAddr += pVCpu->cpum.GstCtx.rsp + offRsp; SET_SS_DEF(); break;
                        case  6: u64EffAddr += pVCpu->cpum.GstCtx.rsi; break;
                        case  7: u64EffAddr += pVCpu->cpum.GstCtx.rdi; break;
                        case  8: u64EffAddr += pVCpu->cpum.GstCtx.r8;  break;
                        case  9: u64EffAddr += pVCpu->cpum.GstCtx.r9;  break;
                        case 10: u64EffAddr += pVCpu->cpum.GstCtx.r10; break;
                        case 11: u64EffAddr += pVCpu->cpum.GstCtx.r11; break;
                        case 12: u64EffAddr += pVCpu->cpum.GstCtx.r12; break;
                        case 14: u64EffAddr += pVCpu->cpum.GstCtx.r14; break;
                        case 15: u64EffAddr += pVCpu->cpum.GstCtx.r15; break;
                        /* complicated encodings */
                        case 5:
                        case 13:
                            if ((bRm & X86_MODRM_MOD_MASK) != 0)
                            {
                                if (!pVCpu->iem.s.uRexB)
                                {
                                    u64EffAddr += pVCpu->cpum.GstCtx.rbp;
                                    SET_SS_DEF();
                                }
                                else
                                    u64EffAddr += pVCpu->cpum.GstCtx.r13;
                            }
                            else
                            {
                                uint32_t u32Disp;
                                IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                                u64EffAddr += (int32_t)u32Disp;
                            }
                            break;
                        IEM_NOT_REACHED_DEFAULT_CASE_RET();
                    }
                    break;
                }
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }

            /* Get and add the displacement. */
            switch ((bRm >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
            {
                case 0:
                    break;
                case 1:
                {
                    int8_t i8Disp;
                    IEM_OPCODE_GET_NEXT_S8(&i8Disp);
                    u64EffAddr += i8Disp;
                    break;
                }
                case 2:
                {
                    uint32_t u32Disp;
                    IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                    u64EffAddr += (int32_t)u32Disp;
                    break;
                }
                IEM_NOT_REACHED_DEFAULT_CASE_RET(); /* (caller checked for these) */
            }

        }

        if (pVCpu->iem.s.enmEffAddrMode == IEMMODE_64BIT)
            *pGCPtrEff = u64EffAddr;
        else
        {
            Assert(pVCpu->iem.s.enmEffAddrMode == IEMMODE_32BIT);
            *pGCPtrEff = u64EffAddr & UINT32_MAX;
        }
    }

    Log5(("iemOpHlpCalcRmEffAddr: EffAddr=%#010RGv\n", *pGCPtrEff));
    return VINF_SUCCESS;
}


#ifdef IEM_WITH_SETJMP
/**
 * Calculates the effective address of a ModR/M memory operand.
 *
 * Meant to be used via IEM_MC_CALC_RM_EFF_ADDR.
 *
 * May longjmp on internal error.
 *
 * @return  The effective address.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   bRm                 The ModRM byte.
 * @param   cbImm               The size of any immediate following the
 *                              effective address opcode bytes. Important for
 *                              RIP relative addressing.
 */
RTGCPTR iemOpHlpCalcRmEffAddrJmp(PVMCPUCC pVCpu, uint8_t bRm, uint8_t cbImm) IEM_NOEXCEPT_MAY_LONGJMP
{
    Log5(("iemOpHlpCalcRmEffAddrJmp: bRm=%#x\n", bRm));
# define SET_SS_DEF() \
    do \
    { \
        if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_SEG_MASK)) \
            pVCpu->iem.s.iEffSeg = X86_SREG_SS; \
    } while (0)

    if (pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT)
    {
/** @todo Check the effective address size crap! */
        if (pVCpu->iem.s.enmEffAddrMode == IEMMODE_16BIT)
        {
            uint16_t u16EffAddr;

            /* Handle the disp16 form with no registers first. */
            if ((bRm & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 6)
                IEM_OPCODE_GET_NEXT_U16(&u16EffAddr);
            else
            {
                /* Get the displacment. */
                switch ((bRm >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
                {
                    case 0:  u16EffAddr = 0;                             break;
                    case 1:  IEM_OPCODE_GET_NEXT_S8_SX_U16(&u16EffAddr); break;
                    case 2:  IEM_OPCODE_GET_NEXT_U16(&u16EffAddr);       break;
                    default: AssertFailedStmt(IEM_DO_LONGJMP(pVCpu, VERR_IEM_IPE_1)); /* (caller checked for these) */
                }

                /* Add the base and index registers to the disp. */
                switch (bRm & X86_MODRM_RM_MASK)
                {
                    case 0: u16EffAddr += pVCpu->cpum.GstCtx.bx + pVCpu->cpum.GstCtx.si; break;
                    case 1: u16EffAddr += pVCpu->cpum.GstCtx.bx + pVCpu->cpum.GstCtx.di; break;
                    case 2: u16EffAddr += pVCpu->cpum.GstCtx.bp + pVCpu->cpum.GstCtx.si; SET_SS_DEF(); break;
                    case 3: u16EffAddr += pVCpu->cpum.GstCtx.bp + pVCpu->cpum.GstCtx.di; SET_SS_DEF(); break;
                    case 4: u16EffAddr += pVCpu->cpum.GstCtx.si;            break;
                    case 5: u16EffAddr += pVCpu->cpum.GstCtx.di;            break;
                    case 6: u16EffAddr += pVCpu->cpum.GstCtx.bp;            SET_SS_DEF(); break;
                    case 7: u16EffAddr += pVCpu->cpum.GstCtx.bx;            break;
                }
            }

            Log5(("iemOpHlpCalcRmEffAddrJmp: EffAddr=%#06RX16\n", u16EffAddr));
            return u16EffAddr;
        }

        Assert(pVCpu->iem.s.enmEffAddrMode == IEMMODE_32BIT);
        uint32_t u32EffAddr;

        /* Handle the disp32 form with no registers first. */
        if ((bRm & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 5)
            IEM_OPCODE_GET_NEXT_U32(&u32EffAddr);
        else
        {
            /* Get the register (or SIB) value. */
            switch ((bRm & X86_MODRM_RM_MASK))
            {
                case 0: u32EffAddr = pVCpu->cpum.GstCtx.eax; break;
                case 1: u32EffAddr = pVCpu->cpum.GstCtx.ecx; break;
                case 2: u32EffAddr = pVCpu->cpum.GstCtx.edx; break;
                case 3: u32EffAddr = pVCpu->cpum.GstCtx.ebx; break;
                case 4: /* SIB */
                {
                    uint8_t bSib; IEM_OPCODE_GET_NEXT_U8(&bSib);

                    /* Get the index and scale it. */
                    switch ((bSib >> X86_SIB_INDEX_SHIFT) & X86_SIB_INDEX_SMASK)
                    {
                        case 0: u32EffAddr = pVCpu->cpum.GstCtx.eax; break;
                        case 1: u32EffAddr = pVCpu->cpum.GstCtx.ecx; break;
                        case 2: u32EffAddr = pVCpu->cpum.GstCtx.edx; break;
                        case 3: u32EffAddr = pVCpu->cpum.GstCtx.ebx; break;
                        case 4: u32EffAddr = 0; /*none */ break;
                        case 5: u32EffAddr = pVCpu->cpum.GstCtx.ebp; break;
                        case 6: u32EffAddr = pVCpu->cpum.GstCtx.esi; break;
                        case 7: u32EffAddr = pVCpu->cpum.GstCtx.edi; break;
                        IEM_NOT_REACHED_DEFAULT_CASE_RET2(RTGCPTR_MAX);
                    }
                    u32EffAddr <<= (bSib >> X86_SIB_SCALE_SHIFT) & X86_SIB_SCALE_SMASK;

                    /* add base */
                    switch (bSib & X86_SIB_BASE_MASK)
                    {
                        case 0: u32EffAddr += pVCpu->cpum.GstCtx.eax; break;
                        case 1: u32EffAddr += pVCpu->cpum.GstCtx.ecx; break;
                        case 2: u32EffAddr += pVCpu->cpum.GstCtx.edx; break;
                        case 3: u32EffAddr += pVCpu->cpum.GstCtx.ebx; break;
                        case 4: u32EffAddr += pVCpu->cpum.GstCtx.esp; SET_SS_DEF(); break;
                        case 5:
                            if ((bRm & X86_MODRM_MOD_MASK) != 0)
                            {
                                u32EffAddr += pVCpu->cpum.GstCtx.ebp;
                                SET_SS_DEF();
                            }
                            else
                            {
                                uint32_t u32Disp;
                                IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                                u32EffAddr += u32Disp;
                            }
                            break;
                        case 6: u32EffAddr += pVCpu->cpum.GstCtx.esi; break;
                        case 7: u32EffAddr += pVCpu->cpum.GstCtx.edi; break;
                        IEM_NOT_REACHED_DEFAULT_CASE_RET2(RTGCPTR_MAX);
                    }
                    break;
                }
                case 5: u32EffAddr = pVCpu->cpum.GstCtx.ebp; SET_SS_DEF(); break;
                case 6: u32EffAddr = pVCpu->cpum.GstCtx.esi; break;
                case 7: u32EffAddr = pVCpu->cpum.GstCtx.edi; break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET2(RTGCPTR_MAX);
            }

            /* Get and add the displacement. */
            switch ((bRm >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
            {
                case 0:
                    break;
                case 1:
                {
                    int8_t i8Disp; IEM_OPCODE_GET_NEXT_S8(&i8Disp);
                    u32EffAddr += i8Disp;
                    break;
                }
                case 2:
                {
                    uint32_t u32Disp; IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                    u32EffAddr += u32Disp;
                    break;
                }
                default:
                    AssertFailedStmt(IEM_DO_LONGJMP(pVCpu, VERR_IEM_IPE_2)); /* (caller checked for these) */
            }
        }

        if (pVCpu->iem.s.enmEffAddrMode == IEMMODE_32BIT)
        {
            Log5(("iemOpHlpCalcRmEffAddrJmp: EffAddr=%#010RX32\n", u32EffAddr));
            return u32EffAddr;
        }
        Assert(pVCpu->iem.s.enmEffAddrMode == IEMMODE_16BIT);
        Log5(("iemOpHlpCalcRmEffAddrJmp: EffAddr=%#06RX32\n", u32EffAddr & UINT16_MAX));
        return u32EffAddr & UINT16_MAX;
    }

    uint64_t u64EffAddr;

    /* Handle the rip+disp32 form with no registers first. */
    if ((bRm & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 5)
    {
        IEM_OPCODE_GET_NEXT_S32_SX_U64(&u64EffAddr);
        u64EffAddr += pVCpu->cpum.GstCtx.rip + IEM_GET_INSTR_LEN(pVCpu) + cbImm;
    }
    else
    {
        /* Get the register (or SIB) value. */
        switch ((bRm & X86_MODRM_RM_MASK) | pVCpu->iem.s.uRexB)
        {
            case  0: u64EffAddr = pVCpu->cpum.GstCtx.rax; break;
            case  1: u64EffAddr = pVCpu->cpum.GstCtx.rcx; break;
            case  2: u64EffAddr = pVCpu->cpum.GstCtx.rdx; break;
            case  3: u64EffAddr = pVCpu->cpum.GstCtx.rbx; break;
            case  5: u64EffAddr = pVCpu->cpum.GstCtx.rbp; SET_SS_DEF(); break;
            case  6: u64EffAddr = pVCpu->cpum.GstCtx.rsi; break;
            case  7: u64EffAddr = pVCpu->cpum.GstCtx.rdi; break;
            case  8: u64EffAddr = pVCpu->cpum.GstCtx.r8;  break;
            case  9: u64EffAddr = pVCpu->cpum.GstCtx.r9;  break;
            case 10: u64EffAddr = pVCpu->cpum.GstCtx.r10; break;
            case 11: u64EffAddr = pVCpu->cpum.GstCtx.r11; break;
            case 13: u64EffAddr = pVCpu->cpum.GstCtx.r13; break;
            case 14: u64EffAddr = pVCpu->cpum.GstCtx.r14; break;
            case 15: u64EffAddr = pVCpu->cpum.GstCtx.r15; break;
            /* SIB */
            case 4:
            case 12:
            {
                uint8_t bSib; IEM_OPCODE_GET_NEXT_U8(&bSib);

                /* Get the index and scale it. */
                switch (((bSib >> X86_SIB_INDEX_SHIFT) & X86_SIB_INDEX_SMASK) | pVCpu->iem.s.uRexIndex)
                {
                    case  0: u64EffAddr = pVCpu->cpum.GstCtx.rax; break;
                    case  1: u64EffAddr = pVCpu->cpum.GstCtx.rcx; break;
                    case  2: u64EffAddr = pVCpu->cpum.GstCtx.rdx; break;
                    case  3: u64EffAddr = pVCpu->cpum.GstCtx.rbx; break;
                    case  4: u64EffAddr = 0; /*none */ break;
                    case  5: u64EffAddr = pVCpu->cpum.GstCtx.rbp; break;
                    case  6: u64EffAddr = pVCpu->cpum.GstCtx.rsi; break;
                    case  7: u64EffAddr = pVCpu->cpum.GstCtx.rdi; break;
                    case  8: u64EffAddr = pVCpu->cpum.GstCtx.r8;  break;
                    case  9: u64EffAddr = pVCpu->cpum.GstCtx.r9;  break;
                    case 10: u64EffAddr = pVCpu->cpum.GstCtx.r10; break;
                    case 11: u64EffAddr = pVCpu->cpum.GstCtx.r11; break;
                    case 12: u64EffAddr = pVCpu->cpum.GstCtx.r12; break;
                    case 13: u64EffAddr = pVCpu->cpum.GstCtx.r13; break;
                    case 14: u64EffAddr = pVCpu->cpum.GstCtx.r14; break;
                    case 15: u64EffAddr = pVCpu->cpum.GstCtx.r15; break;
                    IEM_NOT_REACHED_DEFAULT_CASE_RET2(RTGCPTR_MAX);
                }
                u64EffAddr <<= (bSib >> X86_SIB_SCALE_SHIFT) & X86_SIB_SCALE_SMASK;

                /* add base */
                switch ((bSib & X86_SIB_BASE_MASK) | pVCpu->iem.s.uRexB)
                {
                    case  0: u64EffAddr += pVCpu->cpum.GstCtx.rax; break;
                    case  1: u64EffAddr += pVCpu->cpum.GstCtx.rcx; break;
                    case  2: u64EffAddr += pVCpu->cpum.GstCtx.rdx; break;
                    case  3: u64EffAddr += pVCpu->cpum.GstCtx.rbx; break;
                    case  4: u64EffAddr += pVCpu->cpum.GstCtx.rsp; SET_SS_DEF(); break;
                    case  6: u64EffAddr += pVCpu->cpum.GstCtx.rsi; break;
                    case  7: u64EffAddr += pVCpu->cpum.GstCtx.rdi; break;
                    case  8: u64EffAddr += pVCpu->cpum.GstCtx.r8;  break;
                    case  9: u64EffAddr += pVCpu->cpum.GstCtx.r9;  break;
                    case 10: u64EffAddr += pVCpu->cpum.GstCtx.r10; break;
                    case 11: u64EffAddr += pVCpu->cpum.GstCtx.r11; break;
                    case 12: u64EffAddr += pVCpu->cpum.GstCtx.r12; break;
                    case 14: u64EffAddr += pVCpu->cpum.GstCtx.r14; break;
                    case 15: u64EffAddr += pVCpu->cpum.GstCtx.r15; break;
                    /* complicated encodings */
                    case 5:
                    case 13:
                        if ((bRm & X86_MODRM_MOD_MASK) != 0)
                        {
                            if (!pVCpu->iem.s.uRexB)
                            {
                                u64EffAddr += pVCpu->cpum.GstCtx.rbp;
                                SET_SS_DEF();
                            }
                            else
                                u64EffAddr += pVCpu->cpum.GstCtx.r13;
                        }
                        else
                        {
                            uint32_t u32Disp;
                            IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                            u64EffAddr += (int32_t)u32Disp;
                        }
                        break;
                    IEM_NOT_REACHED_DEFAULT_CASE_RET2(RTGCPTR_MAX);
                }
                break;
            }
            IEM_NOT_REACHED_DEFAULT_CASE_RET2(RTGCPTR_MAX);
        }

        /* Get and add the displacement. */
        switch ((bRm >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
        {
            case 0:
                break;
            case 1:
            {
                int8_t i8Disp;
                IEM_OPCODE_GET_NEXT_S8(&i8Disp);
                u64EffAddr += i8Disp;
                break;
            }
            case 2:
            {
                uint32_t u32Disp;
                IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                u64EffAddr += (int32_t)u32Disp;
                break;
            }
            IEM_NOT_REACHED_DEFAULT_CASE_RET2(RTGCPTR_MAX); /* (caller checked for these) */
        }

    }

    if (pVCpu->iem.s.enmEffAddrMode == IEMMODE_64BIT)
    {
        Log5(("iemOpHlpCalcRmEffAddrJmp: EffAddr=%#010RGv\n", u64EffAddr));
        return u64EffAddr;
    }
    Assert(pVCpu->iem.s.enmEffAddrMode == IEMMODE_32BIT);
    Log5(("iemOpHlpCalcRmEffAddrJmp: EffAddr=%#010RGv\n", u64EffAddr & UINT32_MAX));
    return u64EffAddr & UINT32_MAX;
}
#endif /* IEM_WITH_SETJMP */

/** @}  */


#ifdef LOG_ENABLED
/**
 * Logs the current instruction.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   fSameCtx    Set if we have the same context information as the VMM,
 *                      clear if we may have already executed an instruction in
 *                      our debug context. When clear, we assume IEMCPU holds
 *                      valid CPU mode info.
 *
 *                      The @a fSameCtx parameter is now misleading and obsolete.
 * @param   pszFunction The IEM function doing the execution.
 */
static void iemLogCurInstr(PVMCPUCC pVCpu, bool fSameCtx, const char *pszFunction) RT_NOEXCEPT
{
# ifdef IN_RING3
    if (LogIs2Enabled())
    {
        char     szInstr[256];
        uint32_t cbInstr = 0;
        if (fSameCtx)
            DBGFR3DisasInstrEx(pVCpu->pVMR3->pUVM, pVCpu->idCpu, 0, 0,
                               DBGF_DISAS_FLAGS_CURRENT_GUEST | DBGF_DISAS_FLAGS_DEFAULT_MODE,
                               szInstr, sizeof(szInstr), &cbInstr);
        else
        {
            uint32_t fFlags = 0;
            switch (pVCpu->iem.s.enmCpuMode)
            {
                case IEMMODE_64BIT: fFlags |= DBGF_DISAS_FLAGS_64BIT_MODE; break;
                case IEMMODE_32BIT: fFlags |= DBGF_DISAS_FLAGS_32BIT_MODE; break;
                case IEMMODE_16BIT:
                    if (!(pVCpu->cpum.GstCtx.cr0 & X86_CR0_PE) || pVCpu->cpum.GstCtx.eflags.Bits.u1VM)
                        fFlags |= DBGF_DISAS_FLAGS_16BIT_REAL_MODE;
                    else
                        fFlags |= DBGF_DISAS_FLAGS_16BIT_MODE;
                    break;
            }
            DBGFR3DisasInstrEx(pVCpu->pVMR3->pUVM, pVCpu->idCpu, pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, fFlags,
                               szInstr, sizeof(szInstr), &cbInstr);
        }

        PCX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
        Log2(("**** %s\n"
              " eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x\n"
              " eip=%08x esp=%08x ebp=%08x iopl=%d tr=%04x\n"
              " cs=%04x ss=%04x ds=%04x es=%04x fs=%04x gs=%04x efl=%08x\n"
              " fsw=%04x fcw=%04x ftw=%02x mxcsr=%04x/%04x\n"
              " %s\n"
              , pszFunction,
              pVCpu->cpum.GstCtx.eax, pVCpu->cpum.GstCtx.ebx, pVCpu->cpum.GstCtx.ecx, pVCpu->cpum.GstCtx.edx, pVCpu->cpum.GstCtx.esi, pVCpu->cpum.GstCtx.edi,
              pVCpu->cpum.GstCtx.eip, pVCpu->cpum.GstCtx.esp, pVCpu->cpum.GstCtx.ebp, pVCpu->cpum.GstCtx.eflags.Bits.u2IOPL, pVCpu->cpum.GstCtx.tr.Sel,
              pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.ss.Sel, pVCpu->cpum.GstCtx.ds.Sel, pVCpu->cpum.GstCtx.es.Sel,
              pVCpu->cpum.GstCtx.fs.Sel, pVCpu->cpum.GstCtx.gs.Sel, pVCpu->cpum.GstCtx.eflags.u,
              pFpuCtx->FSW, pFpuCtx->FCW, pFpuCtx->FTW, pFpuCtx->MXCSR, pFpuCtx->MXCSR_MASK,
              szInstr));

        if (LogIs3Enabled())
            DBGFR3InfoEx(pVCpu->pVMR3->pUVM, pVCpu->idCpu, "cpumguest", "verbose", NULL);
    }
    else
# endif
        LogFlow(("%s: cs:rip=%04x:%08RX64 ss:rsp=%04x:%08RX64 EFL=%06x\n", pszFunction, pVCpu->cpum.GstCtx.cs.Sel,
                 pVCpu->cpum.GstCtx.rip, pVCpu->cpum.GstCtx.ss.Sel, pVCpu->cpum.GstCtx.rsp, pVCpu->cpum.GstCtx.eflags.u));
    RT_NOREF_PV(pVCpu); RT_NOREF_PV(fSameCtx);
}
#endif /* LOG_ENABLED */


#ifdef VBOX_WITH_NESTED_HWVIRT_VMX
/**
 * Deals with VMCPU_FF_VMX_APIC_WRITE, VMCPU_FF_VMX_MTF, VMCPU_FF_VMX_NMI_WINDOW,
 * VMCPU_FF_VMX_PREEMPT_TIMER and VMCPU_FF_VMX_INT_WINDOW.
 *
 * @returns Modified rcStrict.
 * @param   pVCpu       The cross context virtual CPU structure of the calling thread.
 * @param   rcStrict    The instruction execution status.
 */
static VBOXSTRICTRC iemHandleNestedInstructionBoundaryFFs(PVMCPUCC pVCpu, VBOXSTRICTRC rcStrict) RT_NOEXCEPT
{
    Assert(CPUMIsGuestInVmxNonRootMode(IEM_GET_CTX(pVCpu)));
    if (!VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_VMX_APIC_WRITE | VMCPU_FF_VMX_MTF))
    {
        /* VMX preemption timer takes priority over NMI-window exits. */
        if (VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_VMX_PREEMPT_TIMER))
        {
            rcStrict = iemVmxVmexitPreemptTimer(pVCpu);
            Assert(!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_VMX_PREEMPT_TIMER));
        }
        /*
         * Check remaining intercepts.
         *
         * NMI-window and Interrupt-window VM-exits.
         * Interrupt shadow (block-by-STI and Mov SS) inhibits interrupts and may also block NMIs.
         * Event injection during VM-entry takes priority over NMI-window and interrupt-window VM-exits.
         *
         * See Intel spec. 26.7.6 "NMI-Window Exiting".
         * See Intel spec. 26.7.5 "Interrupt-Window Exiting and Virtual-Interrupt Delivery".
         */
        else if (   VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_VMX_NMI_WINDOW | VMCPU_FF_VMX_INT_WINDOW)
                 && !CPUMIsInInterruptShadow(&pVCpu->cpum.GstCtx)
                 && !TRPMHasTrap(pVCpu))
        {
            Assert(CPUMIsGuestVmxInterceptEvents(&pVCpu->cpum.GstCtx));
            if (   VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_VMX_NMI_WINDOW)
                && CPUMIsGuestVmxVirtNmiBlocking(&pVCpu->cpum.GstCtx))
            {
                rcStrict = iemVmxVmexit(pVCpu, VMX_EXIT_NMI_WINDOW, 0 /* u64ExitQual */);
                Assert(!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_VMX_NMI_WINDOW));
            }
            else if (   VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_VMX_INT_WINDOW)
                     && CPUMIsGuestVmxVirtIntrEnabled(&pVCpu->cpum.GstCtx))
            {
                rcStrict = iemVmxVmexit(pVCpu, VMX_EXIT_INT_WINDOW, 0 /* u64ExitQual */);
                Assert(!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_VMX_INT_WINDOW));
            }
        }
    }
    /* TPR-below threshold/APIC write has the highest priority. */
    else  if (VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_VMX_APIC_WRITE))
    {
        rcStrict = iemVmxApicWriteEmulation(pVCpu);
        Assert(!CPUMIsInInterruptShadow(&pVCpu->cpum.GstCtx));
        Assert(!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_VMX_APIC_WRITE));
    }
    /* MTF takes priority over VMX-preemption timer. */
    else
    {
        rcStrict = iemVmxVmexit(pVCpu, VMX_EXIT_MTF, 0 /* u64ExitQual */);
        Assert(!CPUMIsInInterruptShadow(&pVCpu->cpum.GstCtx));
        Assert(!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_VMX_MTF));
    }
    return rcStrict;
}
#endif /* VBOX_WITH_NESTED_HWVIRT_VMX */


/** @def IEM_TRY_SETJMP
 * Wrapper around setjmp / try, hiding all the ugly differences.
 *
 * @note Use with extreme care as this is a fragile macro.
 * @param   a_pVCpu     The cross context virtual CPU structure of the calling EMT.
 * @param   a_rcTarget  The variable that should receive the status code in case
 *                      of a longjmp/throw.
 */
/** @def IEM_TRY_SETJMP_AGAIN
 * For when setjmp / try is used again in the same variable scope as a previous
 * IEM_TRY_SETJMP invocation.
 */
/** @def IEM_CATCH_LONGJMP_BEGIN
 * Start wrapper for catch / setjmp-else.
 *
 * This will set up a scope.
 *
 * @note Use with extreme care as this is a fragile macro.
 * @param   a_pVCpu     The cross context virtual CPU structure of the calling EMT.
 * @param   a_rcTarget  The variable that should receive the status code in case
 *                      of a longjmp/throw.
 */
/** @def IEM_CATCH_LONGJMP_END
 * End wrapper for catch / setjmp-else.
 *
 * This will close the scope set up by IEM_CATCH_LONGJMP_BEGIN and clean up the
 * state.
 *
 * @note Use with extreme care as this is a fragile macro.
 * @param   a_pVCpu     The cross context virtual CPU structure of the calling EMT.
 */
#if defined(IEM_WITH_SETJMP) || defined(DOXYGEN_RUNNING)
# ifdef IEM_WITH_THROW_CATCH
#  define IEM_TRY_SETJMP(a_pVCpu, a_rcTarget) \
        a_rcTarget = VINF_SUCCESS; \
        try
#  define IEM_TRY_SETJMP_AGAIN(a_pVCpu, a_rcTarget) \
        IEM_TRY_SETJMP(a_pVCpu, a_rcTarget)
#  define IEM_CATCH_LONGJMP_BEGIN(a_pVCpu, a_rcTarget) \
        catch (int rcThrown) \
        { \
            a_rcTarget = rcThrown
#  define IEM_CATCH_LONGJMP_END(a_pVCpu) \
        } \
        ((void)0)
# else  /* !IEM_WITH_THROW_CATCH */
#  define IEM_TRY_SETJMP(a_pVCpu, a_rcTarget) \
        jmp_buf  JmpBuf; \
        jmp_buf * volatile pSavedJmpBuf = pVCpu->iem.s.CTX_SUFF(pJmpBuf); \
        pVCpu->iem.s.CTX_SUFF(pJmpBuf) = &JmpBuf; \
        if ((rcStrict = setjmp(JmpBuf)) == 0)
#  define IEM_TRY_SETJMP_AGAIN(a_pVCpu, a_rcTarget) \
        pSavedJmpBuf = pVCpu->iem.s.CTX_SUFF(pJmpBuf); \
        pVCpu->iem.s.CTX_SUFF(pJmpBuf) = &JmpBuf; \
        if ((rcStrict = setjmp(JmpBuf)) == 0)
#  define IEM_CATCH_LONGJMP_BEGIN(a_pVCpu, a_rcTarget) \
        else \
        { \
            ((void)0)
#  define IEM_CATCH_LONGJMP_END(a_pVCpu) \
        } \
        (a_pVCpu)->iem.s.CTX_SUFF(pJmpBuf) = pSavedJmpBuf
# endif /* !IEM_WITH_THROW_CATCH */
#endif  /* IEM_WITH_SETJMP */


/**
 * The actual code execution bits of IEMExecOne, IEMExecOneEx, and
 * IEMExecOneWithPrefetchedByPC.
 *
 * Similar code is found in IEMExecLots.
 *
 * @return  Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   fExecuteInhibit     If set, execute the instruction following CLI,
 *                      POP SS and MOV SS,GR.
 * @param   pszFunction The calling function name.
 */
DECLINLINE(VBOXSTRICTRC) iemExecOneInner(PVMCPUCC pVCpu, bool fExecuteInhibit, const char *pszFunction)
{
    AssertMsg(pVCpu->iem.s.aMemMappings[0].fAccess == IEM_ACCESS_INVALID, ("0: %#x %RGp\n", pVCpu->iem.s.aMemMappings[0].fAccess, pVCpu->iem.s.aMemBbMappings[0].GCPhysFirst));
    AssertMsg(pVCpu->iem.s.aMemMappings[1].fAccess == IEM_ACCESS_INVALID, ("1: %#x %RGp\n", pVCpu->iem.s.aMemMappings[1].fAccess, pVCpu->iem.s.aMemBbMappings[1].GCPhysFirst));
    AssertMsg(pVCpu->iem.s.aMemMappings[2].fAccess == IEM_ACCESS_INVALID, ("2: %#x %RGp\n", pVCpu->iem.s.aMemMappings[2].fAccess, pVCpu->iem.s.aMemBbMappings[2].GCPhysFirst));
    RT_NOREF_PV(pszFunction);

#ifdef IEM_WITH_SETJMP
    VBOXSTRICTRC rcStrict;
    IEM_TRY_SETJMP(pVCpu, rcStrict)
    {
        uint8_t b; IEM_OPCODE_GET_FIRST_U8(&b);
        rcStrict = FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }
    IEM_CATCH_LONGJMP_BEGIN(pVCpu, rcStrict);
    {
        pVCpu->iem.s.cLongJumps++;
    }
    IEM_CATCH_LONGJMP_END(pVCpu);
#else
    uint8_t b; IEM_OPCODE_GET_FIRST_U8(&b);
    VBOXSTRICTRC rcStrict = FNIEMOP_CALL(g_apfnOneByteMap[b]);
#endif
    if (rcStrict == VINF_SUCCESS)
        pVCpu->iem.s.cInstructions++;
    if (pVCpu->iem.s.cActiveMappings > 0)
    {
        Assert(rcStrict != VINF_SUCCESS);
        iemMemRollback(pVCpu);
    }
    AssertMsg(pVCpu->iem.s.aMemMappings[0].fAccess == IEM_ACCESS_INVALID, ("0: %#x %RGp\n", pVCpu->iem.s.aMemMappings[0].fAccess, pVCpu->iem.s.aMemBbMappings[0].GCPhysFirst));
    AssertMsg(pVCpu->iem.s.aMemMappings[1].fAccess == IEM_ACCESS_INVALID, ("1: %#x %RGp\n", pVCpu->iem.s.aMemMappings[1].fAccess, pVCpu->iem.s.aMemBbMappings[1].GCPhysFirst));
    AssertMsg(pVCpu->iem.s.aMemMappings[2].fAccess == IEM_ACCESS_INVALID, ("2: %#x %RGp\n", pVCpu->iem.s.aMemMappings[2].fAccess, pVCpu->iem.s.aMemBbMappings[2].GCPhysFirst));

//#ifdef DEBUG
//    AssertMsg(IEM_GET_INSTR_LEN(pVCpu) == cbInstr || rcStrict != VINF_SUCCESS, ("%u %u\n", IEM_GET_INSTR_LEN(pVCpu), cbInstr));
//#endif

#ifdef VBOX_WITH_NESTED_HWVIRT_VMX
    /*
     * Perform any VMX nested-guest instruction boundary actions.
     *
     * If any of these causes a VM-exit, we must skip executing the next
     * instruction (would run into stale page tables). A VM-exit makes sure
     * there is no interrupt-inhibition, so that should ensure we don't go
     * to try execute the next instruction. Clearing fExecuteInhibit is
     * problematic because of the setjmp/longjmp clobbering above.
     */
    if (   !VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_VMX_APIC_WRITE | VMCPU_FF_VMX_MTF | VMCPU_FF_VMX_PREEMPT_TIMER
                                     | VMCPU_FF_VMX_INT_WINDOW | VMCPU_FF_VMX_NMI_WINDOW)
        || rcStrict != VINF_SUCCESS)
    { /* likely */ }
    else
        rcStrict = iemHandleNestedInstructionBoundaryFFs(pVCpu, rcStrict);
#endif

    /* Execute the next instruction as well if a cli, pop ss or
       mov ss, Gr has just completed successfully. */
    if (   fExecuteInhibit
        && rcStrict == VINF_SUCCESS
        && CPUMIsInInterruptShadow(&pVCpu->cpum.GstCtx))
    {
        rcStrict = iemInitDecoderAndPrefetchOpcodes(pVCpu, pVCpu->iem.s.fBypassHandlers, pVCpu->iem.s.fDisregardLock);
        if (rcStrict == VINF_SUCCESS)
        {
#ifdef LOG_ENABLED
            iemLogCurInstr(pVCpu, false, pszFunction);
#endif
#ifdef IEM_WITH_SETJMP
            IEM_TRY_SETJMP_AGAIN(pVCpu, rcStrict)
            {
                uint8_t b; IEM_OPCODE_GET_FIRST_U8(&b);
                rcStrict = FNIEMOP_CALL(g_apfnOneByteMap[b]);
            }
            IEM_CATCH_LONGJMP_BEGIN(pVCpu, rcStrict);
            {
                pVCpu->iem.s.cLongJumps++;
            }
            IEM_CATCH_LONGJMP_END(pVCpu);
#else
            IEM_OPCODE_GET_FIRST_U8(&b);
            rcStrict = FNIEMOP_CALL(g_apfnOneByteMap[b]);
#endif
            if (rcStrict == VINF_SUCCESS)
            {
                pVCpu->iem.s.cInstructions++;
#ifdef VBOX_WITH_NESTED_HWVIRT_VMX
                if (!VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_VMX_APIC_WRITE | VMCPU_FF_VMX_MTF | VMCPU_FF_VMX_PREEMPT_TIMER
                                              | VMCPU_FF_VMX_INT_WINDOW | VMCPU_FF_VMX_NMI_WINDOW))
                { /* likely */ }
                else
                    rcStrict = iemHandleNestedInstructionBoundaryFFs(pVCpu, rcStrict);
#endif
            }
            if (pVCpu->iem.s.cActiveMappings > 0)
            {
                Assert(rcStrict != VINF_SUCCESS);
                iemMemRollback(pVCpu);
            }
            AssertMsg(pVCpu->iem.s.aMemMappings[0].fAccess == IEM_ACCESS_INVALID, ("0: %#x %RGp\n", pVCpu->iem.s.aMemMappings[0].fAccess, pVCpu->iem.s.aMemBbMappings[0].GCPhysFirst));
            AssertMsg(pVCpu->iem.s.aMemMappings[1].fAccess == IEM_ACCESS_INVALID, ("1: %#x %RGp\n", pVCpu->iem.s.aMemMappings[1].fAccess, pVCpu->iem.s.aMemBbMappings[1].GCPhysFirst));
            AssertMsg(pVCpu->iem.s.aMemMappings[2].fAccess == IEM_ACCESS_INVALID, ("2: %#x %RGp\n", pVCpu->iem.s.aMemMappings[2].fAccess, pVCpu->iem.s.aMemBbMappings[2].GCPhysFirst));
        }
        else if (pVCpu->iem.s.cActiveMappings > 0)
            iemMemRollback(pVCpu);
        /** @todo drop this after we bake this change into RIP advancing. */
        CPUMClearInterruptShadow(&pVCpu->cpum.GstCtx); /* hope this is correct for all exceptional cases... */
    }

    /*
     * Return value fiddling, statistics and sanity assertions.
     */
    rcStrict = iemExecStatusCodeFiddling(pVCpu, rcStrict);

    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.cs));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.ss));
    return rcStrict;
}


/**
 * Execute one instruction.
 *
 * @return  Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 */
VMMDECL(VBOXSTRICTRC) IEMExecOne(PVMCPUCC pVCpu)
{
    AssertCompile(sizeof(pVCpu->iem.s) <= sizeof(pVCpu->iem.padding)); /* (tstVMStruct can't do it's job w/o instruction stats) */
#ifdef LOG_ENABLED
    iemLogCurInstr(pVCpu, true, "IEMExecOne");
#endif

    /*
     * Do the decoding and emulation.
     */
    VBOXSTRICTRC rcStrict = iemInitDecoderAndPrefetchOpcodes(pVCpu, false, false);
    if (rcStrict == VINF_SUCCESS)
        rcStrict = iemExecOneInner(pVCpu, true, "IEMExecOne");
    else if (pVCpu->iem.s.cActiveMappings > 0)
        iemMemRollback(pVCpu);

    if (rcStrict != VINF_SUCCESS)
        LogFlow(("IEMExecOne: cs:rip=%04x:%08RX64 ss:rsp=%04x:%08RX64 EFL=%06x - rcStrict=%Rrc\n",
                 pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, pVCpu->cpum.GstCtx.ss.Sel, pVCpu->cpum.GstCtx.rsp, pVCpu->cpum.GstCtx.eflags.u, VBOXSTRICTRC_VAL(rcStrict)));
    return rcStrict;
}


VMMDECL(VBOXSTRICTRC) IEMExecOneEx(PVMCPUCC pVCpu, uint32_t *pcbWritten)
{
    uint32_t const cbOldWritten = pVCpu->iem.s.cbWritten;
    VBOXSTRICTRC rcStrict = iemInitDecoderAndPrefetchOpcodes(pVCpu, false, false);
    if (rcStrict == VINF_SUCCESS)
    {
        rcStrict = iemExecOneInner(pVCpu, true, "IEMExecOneEx");
        if (pcbWritten)
            *pcbWritten = pVCpu->iem.s.cbWritten - cbOldWritten;
    }
    else if (pVCpu->iem.s.cActiveMappings > 0)
        iemMemRollback(pVCpu);

    return rcStrict;
}


VMMDECL(VBOXSTRICTRC) IEMExecOneWithPrefetchedByPC(PVMCPUCC pVCpu, uint64_t OpcodeBytesPC,
                                                   const void *pvOpcodeBytes, size_t cbOpcodeBytes)
{
    VBOXSTRICTRC rcStrict;
    if (   cbOpcodeBytes
        && pVCpu->cpum.GstCtx.rip == OpcodeBytesPC)
    {
        iemInitDecoder(pVCpu, false, false);
#ifdef IEM_WITH_CODE_TLB
        pVCpu->iem.s.uInstrBufPc      = OpcodeBytesPC;
        pVCpu->iem.s.pbInstrBuf       = (uint8_t const *)pvOpcodeBytes;
        pVCpu->iem.s.cbInstrBufTotal  = (uint16_t)RT_MIN(X86_PAGE_SIZE, cbOpcodeBytes);
        pVCpu->iem.s.offCurInstrStart = 0;
        pVCpu->iem.s.offInstrNextByte = 0;
#else
        pVCpu->iem.s.cbOpcode = (uint8_t)RT_MIN(cbOpcodeBytes, sizeof(pVCpu->iem.s.abOpcode));
        memcpy(pVCpu->iem.s.abOpcode, pvOpcodeBytes, pVCpu->iem.s.cbOpcode);
#endif
        rcStrict = VINF_SUCCESS;
    }
    else
        rcStrict = iemInitDecoderAndPrefetchOpcodes(pVCpu, false, false);
    if (rcStrict == VINF_SUCCESS)
        rcStrict = iemExecOneInner(pVCpu, true, "IEMExecOneWithPrefetchedByPC");
    else if (pVCpu->iem.s.cActiveMappings > 0)
        iemMemRollback(pVCpu);

    return rcStrict;
}


VMMDECL(VBOXSTRICTRC) IEMExecOneBypassEx(PVMCPUCC pVCpu, uint32_t *pcbWritten)
{
    uint32_t const cbOldWritten = pVCpu->iem.s.cbWritten;
    VBOXSTRICTRC rcStrict = iemInitDecoderAndPrefetchOpcodes(pVCpu, true, false);
    if (rcStrict == VINF_SUCCESS)
    {
        rcStrict = iemExecOneInner(pVCpu, false, "IEMExecOneBypassEx");
        if (pcbWritten)
            *pcbWritten = pVCpu->iem.s.cbWritten - cbOldWritten;
    }
    else if (pVCpu->iem.s.cActiveMappings > 0)
        iemMemRollback(pVCpu);

    return rcStrict;
}


VMMDECL(VBOXSTRICTRC) IEMExecOneBypassWithPrefetchedByPC(PVMCPUCC pVCpu, uint64_t OpcodeBytesPC,
                                                         const void *pvOpcodeBytes, size_t cbOpcodeBytes)
{
    VBOXSTRICTRC rcStrict;
    if (   cbOpcodeBytes
        && pVCpu->cpum.GstCtx.rip == OpcodeBytesPC)
    {
        iemInitDecoder(pVCpu, true, false);
#ifdef IEM_WITH_CODE_TLB
        pVCpu->iem.s.uInstrBufPc      = OpcodeBytesPC;
        pVCpu->iem.s.pbInstrBuf       = (uint8_t const *)pvOpcodeBytes;
        pVCpu->iem.s.cbInstrBufTotal  = (uint16_t)RT_MIN(X86_PAGE_SIZE, cbOpcodeBytes);
        pVCpu->iem.s.offCurInstrStart = 0;
        pVCpu->iem.s.offInstrNextByte = 0;
#else
        pVCpu->iem.s.cbOpcode = (uint8_t)RT_MIN(cbOpcodeBytes, sizeof(pVCpu->iem.s.abOpcode));
        memcpy(pVCpu->iem.s.abOpcode, pvOpcodeBytes, pVCpu->iem.s.cbOpcode);
#endif
        rcStrict = VINF_SUCCESS;
    }
    else
        rcStrict = iemInitDecoderAndPrefetchOpcodes(pVCpu, true, false);
    if (rcStrict == VINF_SUCCESS)
        rcStrict = iemExecOneInner(pVCpu, false, "IEMExecOneBypassWithPrefetchedByPC");
    else if (pVCpu->iem.s.cActiveMappings > 0)
        iemMemRollback(pVCpu);

    return rcStrict;
}


/**
 * For handling split cacheline lock operations when the host has split-lock
 * detection enabled.
 *
 * This will cause the interpreter to disregard the lock prefix and implicit
 * locking (xchg).
 *
 * @returns Strict VBox status code.
 * @param   pVCpu   The cross context virtual CPU structure of the calling EMT.
 */
VMMDECL(VBOXSTRICTRC) IEMExecOneIgnoreLock(PVMCPUCC pVCpu)
{
    /*
     * Do the decoding and emulation.
     */
    VBOXSTRICTRC rcStrict = iemInitDecoderAndPrefetchOpcodes(pVCpu, false, true /*fDisregardLock*/);
    if (rcStrict == VINF_SUCCESS)
        rcStrict = iemExecOneInner(pVCpu, true, "IEMExecOneIgnoreLock");
    else if (pVCpu->iem.s.cActiveMappings > 0)
        iemMemRollback(pVCpu);

    if (rcStrict != VINF_SUCCESS)
        LogFlow(("IEMExecOneIgnoreLock: cs:rip=%04x:%08RX64 ss:rsp=%04x:%08RX64 EFL=%06x - rcStrict=%Rrc\n",
                 pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, pVCpu->cpum.GstCtx.ss.Sel, pVCpu->cpum.GstCtx.rsp, pVCpu->cpum.GstCtx.eflags.u, VBOXSTRICTRC_VAL(rcStrict)));
    return rcStrict;
}


VMMDECL(VBOXSTRICTRC) IEMExecLots(PVMCPUCC pVCpu, uint32_t cMaxInstructions, uint32_t cPollRate, uint32_t *pcInstructions)
{
    uint32_t const cInstructionsAtStart = pVCpu->iem.s.cInstructions;
    AssertMsg(RT_IS_POWER_OF_TWO(cPollRate + 1), ("%#x\n", cPollRate));

    /*
     * See if there is an interrupt pending in TRPM, inject it if we can.
     */
    /** @todo What if we are injecting an exception and not an interrupt? Is that
     *        possible here? For now we assert it is indeed only an interrupt. */
    if (!TRPMHasTrap(pVCpu))
    { /* likely */ }
    else
    {
        if (   !CPUMIsInInterruptShadow(&pVCpu->cpum.GstCtx)
            && !CPUMAreInterruptsInhibitedByNmi(&pVCpu->cpum.GstCtx))
        {
            /** @todo Can we centralize this under CPUMCanInjectInterrupt()? */
#if defined(VBOX_WITH_NESTED_HWVIRT_SVM) || defined(VBOX_WITH_NESTED_HWVIRT_VMX)
            bool fIntrEnabled = CPUMGetGuestGif(&pVCpu->cpum.GstCtx);
            if (fIntrEnabled)
            {
                if (!CPUMIsGuestInNestedHwvirtMode(IEM_GET_CTX(pVCpu)))
                    fIntrEnabled = pVCpu->cpum.GstCtx.eflags.Bits.u1IF;
                else if (CPUMIsGuestInVmxNonRootMode(IEM_GET_CTX(pVCpu)))
                    fIntrEnabled = CPUMIsGuestVmxPhysIntrEnabled(IEM_GET_CTX(pVCpu));
                else
                {
                    Assert(CPUMIsGuestInSvmNestedHwVirtMode(IEM_GET_CTX(pVCpu)));
                    fIntrEnabled = CPUMIsGuestSvmPhysIntrEnabled(pVCpu, IEM_GET_CTX(pVCpu));
                }
            }
#else
            bool fIntrEnabled = pVCpu->cpum.GstCtx.eflags.Bits.u1IF;
#endif
            if (fIntrEnabled)
            {
                uint8_t     u8TrapNo;
                TRPMEVENT   enmType;
                uint32_t    uErrCode;
                RTGCPTR     uCr2;
                int rc2 = TRPMQueryTrapAll(pVCpu, &u8TrapNo, &enmType, &uErrCode, &uCr2, NULL /*pu8InstLen*/, NULL /*fIcebp*/);
                AssertRC(rc2);
                Assert(enmType == TRPM_HARDWARE_INT);
                VBOXSTRICTRC rcStrict = IEMInjectTrap(pVCpu, u8TrapNo, enmType, (uint16_t)uErrCode, uCr2, 0 /*cbInstr*/);

                TRPMResetTrap(pVCpu);

#if defined(VBOX_WITH_NESTED_HWVIRT_SVM) || defined(VBOX_WITH_NESTED_HWVIRT_VMX)
                /* Injecting an event may cause a VM-exit. */
                if (   rcStrict != VINF_SUCCESS
                    && rcStrict != VINF_IEM_RAISED_XCPT)
                    return iemExecStatusCodeFiddling(pVCpu, rcStrict);
#else
                NOREF(rcStrict);
#endif
            }
        }
    }

    /*
     * Initial decoder init w/ prefetch, then setup setjmp.
     */
    VBOXSTRICTRC rcStrict = iemInitDecoderAndPrefetchOpcodes(pVCpu, false, false);
    if (rcStrict == VINF_SUCCESS)
    {
#ifdef IEM_WITH_SETJMP
        pVCpu->iem.s.cActiveMappings = 0; /** @todo wtf? */
        IEM_TRY_SETJMP(pVCpu, rcStrict)
#endif
        {
            /*
             * The run loop.  We limit ourselves to 4096 instructions right now.
             */
            uint32_t cMaxInstructionsGccStupidity = cMaxInstructions;
            PVMCC pVM = pVCpu->CTX_SUFF(pVM);
            for (;;)
            {
                /*
                 * Log the state.
                 */
#ifdef LOG_ENABLED
                iemLogCurInstr(pVCpu, true, "IEMExecLots");
#endif

                /*
                 * Do the decoding and emulation.
                 */
                uint8_t b; IEM_OPCODE_GET_FIRST_U8(&b);
                rcStrict = FNIEMOP_CALL(g_apfnOneByteMap[b]);
#ifdef VBOX_STRICT
                CPUMAssertGuestRFlagsCookie(pVM, pVCpu);
#endif
                if (RT_LIKELY(rcStrict == VINF_SUCCESS))
                {
                    Assert(pVCpu->iem.s.cActiveMappings == 0);
                    pVCpu->iem.s.cInstructions++;

#ifdef VBOX_WITH_NESTED_HWVIRT_VMX
                    /* Perform any VMX nested-guest instruction boundary actions. */
                    uint64_t fCpu = pVCpu->fLocalForcedActions;
                    if (!(fCpu & (  VMCPU_FF_VMX_APIC_WRITE | VMCPU_FF_VMX_MTF | VMCPU_FF_VMX_PREEMPT_TIMER
                                  | VMCPU_FF_VMX_INT_WINDOW | VMCPU_FF_VMX_NMI_WINDOW)))
                    { /* likely */ }
                    else
                    {
                        rcStrict = iemHandleNestedInstructionBoundaryFFs(pVCpu, rcStrict);
                        if (RT_LIKELY(rcStrict == VINF_SUCCESS))
                            fCpu = pVCpu->fLocalForcedActions;
                        else
                        {
                            rcStrict = iemExecStatusCodeFiddling(pVCpu, rcStrict);
                            break;
                        }
                    }
#endif
                    if (RT_LIKELY(pVCpu->iem.s.rcPassUp == VINF_SUCCESS))
                    {
#ifndef VBOX_WITH_NESTED_HWVIRT_VMX
                        uint64_t fCpu = pVCpu->fLocalForcedActions;
#endif
                        fCpu &= VMCPU_FF_ALL_MASK & ~(  VMCPU_FF_PGM_SYNC_CR3
                                                      | VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL
                                                      | VMCPU_FF_TLB_FLUSH
                                                      | VMCPU_FF_UNHALT );

                        if (RT_LIKELY(   (   !fCpu
                                          || (   !(fCpu & ~(VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC))
                                              && !pVCpu->cpum.GstCtx.rflags.Bits.u1IF) )
                                      && !VM_FF_IS_ANY_SET(pVM, VM_FF_ALL_MASK) ))
                        {
                            if (cMaxInstructionsGccStupidity-- > 0)
                            {
                                /* Poll timers every now an then according to the caller's specs. */
                                if (   (cMaxInstructionsGccStupidity & cPollRate) != 0
                                    || !TMTimerPollBool(pVM, pVCpu))
                                {
                                    Assert(pVCpu->iem.s.cActiveMappings == 0);
                                    iemReInitDecoder(pVCpu);
                                    continue;
                                }
                            }
                        }
                    }
                    Assert(pVCpu->iem.s.cActiveMappings == 0);
                }
                else if (pVCpu->iem.s.cActiveMappings > 0)
                    iemMemRollback(pVCpu);
                rcStrict = iemExecStatusCodeFiddling(pVCpu, rcStrict);
                break;
            }
        }
#ifdef IEM_WITH_SETJMP
        IEM_CATCH_LONGJMP_BEGIN(pVCpu, rcStrict);
        {
            if (pVCpu->iem.s.cActiveMappings > 0)
                iemMemRollback(pVCpu);
# if defined(VBOX_WITH_NESTED_HWVIRT_SVM) || defined(VBOX_WITH_NESTED_HWVIRT_VMX)
            rcStrict = iemExecStatusCodeFiddling(pVCpu, rcStrict);
# endif
            pVCpu->iem.s.cLongJumps++;
        }
        IEM_CATCH_LONGJMP_END(pVCpu);
#endif

        /*
         * Assert hidden register sanity (also done in iemInitDecoder and iemReInitDecoder).
         */
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.cs));
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.ss));
    }
    else
    {
        if (pVCpu->iem.s.cActiveMappings > 0)
            iemMemRollback(pVCpu);

#if defined(VBOX_WITH_NESTED_HWVIRT_SVM) || defined(VBOX_WITH_NESTED_HWVIRT_VMX)
        /*
         * When a nested-guest causes an exception intercept (e.g. #PF) when fetching
         * code as part of instruction execution, we need this to fix-up VINF_SVM_VMEXIT.
         */
        rcStrict = iemExecStatusCodeFiddling(pVCpu, rcStrict);
#endif
    }

    /*
     * Maybe re-enter raw-mode and log.
     */
    if (rcStrict != VINF_SUCCESS)
        LogFlow(("IEMExecLots: cs:rip=%04x:%08RX64 ss:rsp=%04x:%08RX64 EFL=%06x - rcStrict=%Rrc\n",
                 pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, pVCpu->cpum.GstCtx.ss.Sel, pVCpu->cpum.GstCtx.rsp, pVCpu->cpum.GstCtx.eflags.u, VBOXSTRICTRC_VAL(rcStrict)));
    if (pcInstructions)
        *pcInstructions = pVCpu->iem.s.cInstructions - cInstructionsAtStart;
    return rcStrict;
}


/**
 * Interface used by EMExecuteExec, does exit statistics and limits.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   fWillExit           To be defined.
 * @param   cMinInstructions    Minimum number of instructions to execute before checking for FFs.
 * @param   cMaxInstructions    Maximum number of instructions to execute.
 * @param   cMaxInstructionsWithoutExits
 *                              The max number of instructions without exits.
 * @param   pStats              Where to return statistics.
 */
VMMDECL(VBOXSTRICTRC) IEMExecForExits(PVMCPUCC pVCpu, uint32_t fWillExit, uint32_t cMinInstructions, uint32_t cMaxInstructions,
                                      uint32_t cMaxInstructionsWithoutExits, PIEMEXECFOREXITSTATS pStats)
{
    NOREF(fWillExit); /** @todo define flexible exit crits */

    /*
     * Initialize return stats.
     */
    pStats->cInstructions    = 0;
    pStats->cExits           = 0;
    pStats->cMaxExitDistance = 0;
    pStats->cReserved        = 0;

    /*
     * Initial decoder init w/ prefetch, then setup setjmp.
     */
    VBOXSTRICTRC rcStrict = iemInitDecoderAndPrefetchOpcodes(pVCpu, false, false);
    if (rcStrict == VINF_SUCCESS)
    {
#ifdef IEM_WITH_SETJMP
        pVCpu->iem.s.cActiveMappings     = 0; /** @todo wtf?!? */
        IEM_TRY_SETJMP(pVCpu, rcStrict)
#endif
        {
#ifdef IN_RING0
            bool const fCheckPreemptionPending   = !RTThreadPreemptIsPossible() || !RTThreadPreemptIsEnabled(NIL_RTTHREAD);
#endif
            uint32_t   cInstructionSinceLastExit = 0;

            /*
             * The run loop.  We limit ourselves to 4096 instructions right now.
             */
            PVM pVM = pVCpu->CTX_SUFF(pVM);
            for (;;)
            {
                /*
                 * Log the state.
                 */
#ifdef LOG_ENABLED
                iemLogCurInstr(pVCpu, true, "IEMExecForExits");
#endif

                /*
                 * Do the decoding and emulation.
                 */
                uint32_t const cPotentialExits = pVCpu->iem.s.cPotentialExits;

                uint8_t b; IEM_OPCODE_GET_FIRST_U8(&b);
                rcStrict = FNIEMOP_CALL(g_apfnOneByteMap[b]);

                if (   cPotentialExits != pVCpu->iem.s.cPotentialExits
                    && cInstructionSinceLastExit > 0 /* don't count the first */ )
                {
                    pStats->cExits += 1;
                    if (cInstructionSinceLastExit > pStats->cMaxExitDistance)
                        pStats->cMaxExitDistance = cInstructionSinceLastExit;
                    cInstructionSinceLastExit = 0;
                }

                if (RT_LIKELY(rcStrict == VINF_SUCCESS))
                {
                    Assert(pVCpu->iem.s.cActiveMappings == 0);
                    pVCpu->iem.s.cInstructions++;
                    pStats->cInstructions++;
                    cInstructionSinceLastExit++;

#ifdef VBOX_WITH_NESTED_HWVIRT_VMX
                    /* Perform any VMX nested-guest instruction boundary actions. */
                    uint64_t fCpu = pVCpu->fLocalForcedActions;
                    if (!(fCpu & (  VMCPU_FF_VMX_APIC_WRITE | VMCPU_FF_VMX_MTF | VMCPU_FF_VMX_PREEMPT_TIMER
                                  | VMCPU_FF_VMX_INT_WINDOW | VMCPU_FF_VMX_NMI_WINDOW)))
                    { /* likely */ }
                    else
                    {
                        rcStrict = iemHandleNestedInstructionBoundaryFFs(pVCpu, rcStrict);
                        if (RT_LIKELY(rcStrict == VINF_SUCCESS))
                            fCpu = pVCpu->fLocalForcedActions;
                        else
                        {
                            rcStrict = iemExecStatusCodeFiddling(pVCpu, rcStrict);
                            break;
                        }
                    }
#endif
                    if (RT_LIKELY(pVCpu->iem.s.rcPassUp == VINF_SUCCESS))
                    {
#ifndef VBOX_WITH_NESTED_HWVIRT_VMX
                        uint64_t fCpu = pVCpu->fLocalForcedActions;
#endif
                        fCpu &= VMCPU_FF_ALL_MASK & ~(  VMCPU_FF_PGM_SYNC_CR3
                                                      | VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL
                                                      | VMCPU_FF_TLB_FLUSH
                                                      | VMCPU_FF_UNHALT );
                        if (RT_LIKELY(   (   (   !fCpu
                                              || (   !(fCpu & ~(VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC))
                                                  && !pVCpu->cpum.GstCtx.rflags.Bits.u1IF))
                                          && !VM_FF_IS_ANY_SET(pVM, VM_FF_ALL_MASK) )
                                      || pStats->cInstructions < cMinInstructions))
                        {
                            if (pStats->cInstructions < cMaxInstructions)
                            {
                                if (cInstructionSinceLastExit <= cMaxInstructionsWithoutExits)
                                {
#ifdef IN_RING0
                                    if (   !fCheckPreemptionPending
                                        || !RTThreadPreemptIsPending(NIL_RTTHREAD))
#endif
                                    {
                                        Assert(pVCpu->iem.s.cActiveMappings == 0);
                                        iemReInitDecoder(pVCpu);
                                        continue;
                                    }
#ifdef IN_RING0
                                    rcStrict = VINF_EM_RAW_INTERRUPT;
                                    break;
#endif
                                }
                            }
                        }
                        Assert(!(fCpu & VMCPU_FF_IEM));
                    }
                    Assert(pVCpu->iem.s.cActiveMappings == 0);
                }
                else if (pVCpu->iem.s.cActiveMappings > 0)
                        iemMemRollback(pVCpu);
                rcStrict = iemExecStatusCodeFiddling(pVCpu, rcStrict);
                break;
            }
        }
#ifdef IEM_WITH_SETJMP
        IEM_CATCH_LONGJMP_BEGIN(pVCpu, rcStrict);
        {
            if (pVCpu->iem.s.cActiveMappings > 0)
                iemMemRollback(pVCpu);
            pVCpu->iem.s.cLongJumps++;
        }
        IEM_CATCH_LONGJMP_END(pVCpu);
#endif

        /*
         * Assert hidden register sanity (also done in iemInitDecoder and iemReInitDecoder).
         */
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.cs));
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.GstCtx.ss));
    }
    else
    {
        if (pVCpu->iem.s.cActiveMappings > 0)
            iemMemRollback(pVCpu);

#if defined(VBOX_WITH_NESTED_HWVIRT_SVM) || defined(VBOX_WITH_NESTED_HWVIRT_VMX)
        /*
         * When a nested-guest causes an exception intercept (e.g. #PF) when fetching
         * code as part of instruction execution, we need this to fix-up VINF_SVM_VMEXIT.
         */
        rcStrict = iemExecStatusCodeFiddling(pVCpu, rcStrict);
#endif
    }

    /*
     * Maybe re-enter raw-mode and log.
     */
    if (rcStrict != VINF_SUCCESS)
        LogFlow(("IEMExecForExits: cs:rip=%04x:%08RX64 ss:rsp=%04x:%08RX64 EFL=%06x - rcStrict=%Rrc; ins=%u exits=%u maxdist=%u\n",
                 pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, pVCpu->cpum.GstCtx.ss.Sel, pVCpu->cpum.GstCtx.rsp,
                 pVCpu->cpum.GstCtx.eflags.u, VBOXSTRICTRC_VAL(rcStrict), pStats->cInstructions, pStats->cExits, pStats->cMaxExitDistance));
    return rcStrict;
}


/**
 * Injects a trap, fault, abort, software interrupt or external interrupt.
 *
 * The parameter list matches TRPMQueryTrapAll pretty closely.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 * @param   u8TrapNo            The trap number.
 * @param   enmType             What type is it (trap/fault/abort), software
 *                              interrupt or hardware interrupt.
 * @param   uErrCode            The error code if applicable.
 * @param   uCr2                The CR2 value if applicable.
 * @param   cbInstr             The instruction length (only relevant for
 *                              software interrupts).
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMInjectTrap(PVMCPUCC pVCpu, uint8_t u8TrapNo, TRPMEVENT enmType, uint16_t uErrCode, RTGCPTR uCr2,
                                         uint8_t cbInstr)
{
    iemInitDecoder(pVCpu, false, false);
#ifdef DBGFTRACE_ENABLED
    RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "IEMInjectTrap: %x %d %x %llx",
                      u8TrapNo, enmType, uErrCode, uCr2);
#endif

    uint32_t fFlags;
    switch (enmType)
    {
        case TRPM_HARDWARE_INT:
            Log(("IEMInjectTrap: %#4x ext\n", u8TrapNo));
            fFlags = IEM_XCPT_FLAGS_T_EXT_INT;
            uErrCode = uCr2 = 0;
            break;

        case TRPM_SOFTWARE_INT:
            Log(("IEMInjectTrap: %#4x soft\n", u8TrapNo));
            fFlags = IEM_XCPT_FLAGS_T_SOFT_INT;
            uErrCode = uCr2 = 0;
            break;

        case TRPM_TRAP:
            Log(("IEMInjectTrap: %#4x trap err=%#x cr2=%#RGv\n", u8TrapNo, uErrCode, uCr2));
            fFlags = IEM_XCPT_FLAGS_T_CPU_XCPT;
            if (u8TrapNo == X86_XCPT_PF)
                fFlags |= IEM_XCPT_FLAGS_CR2;
            switch (u8TrapNo)
            {
                case X86_XCPT_DF:
                case X86_XCPT_TS:
                case X86_XCPT_NP:
                case X86_XCPT_SS:
                case X86_XCPT_PF:
                case X86_XCPT_AC:
                case X86_XCPT_GP:
                    fFlags |= IEM_XCPT_FLAGS_ERR;
                    break;
            }
            break;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }

    VBOXSTRICTRC rcStrict = iemRaiseXcptOrInt(pVCpu, cbInstr, u8TrapNo, fFlags, uErrCode, uCr2);

    if (pVCpu->iem.s.cActiveMappings > 0)
        iemMemRollback(pVCpu);

    return rcStrict;
}


/**
 * Injects the active TRPM event.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure.
 */
VMMDECL(VBOXSTRICTRC) IEMInjectTrpmEvent(PVMCPUCC pVCpu)
{
#ifndef IEM_IMPLEMENTS_TASKSWITCH
    IEM_RETURN_ASPECT_NOT_IMPLEMENTED_LOG(("Event injection\n"));
#else
    uint8_t     u8TrapNo;
    TRPMEVENT   enmType;
    uint32_t    uErrCode;
    RTGCUINTPTR uCr2;
    uint8_t     cbInstr;
    int rc = TRPMQueryTrapAll(pVCpu, &u8TrapNo, &enmType, &uErrCode, &uCr2, &cbInstr, NULL /* fIcebp */);
    if (RT_FAILURE(rc))
        return rc;

    /** @todo r=ramshankar: Pass ICEBP info. to IEMInjectTrap() below and handle
     *        ICEBP \#DB injection as a special case. */
    VBOXSTRICTRC rcStrict = IEMInjectTrap(pVCpu, u8TrapNo, enmType, uErrCode, uCr2, cbInstr);
#ifdef VBOX_WITH_NESTED_HWVIRT_SVM
    if (rcStrict == VINF_SVM_VMEXIT)
        rcStrict = VINF_SUCCESS;
#endif
#ifdef VBOX_WITH_NESTED_HWVIRT_VMX
    if (rcStrict == VINF_VMX_VMEXIT)
        rcStrict = VINF_SUCCESS;
#endif
    /** @todo Are there any other codes that imply the event was successfully
     *        delivered to the guest? See @bugref{6607}.  */
    if (   rcStrict == VINF_SUCCESS
        || rcStrict == VINF_IEM_RAISED_XCPT)
        TRPMResetTrap(pVCpu);

    return rcStrict;
#endif
}


VMM_INT_DECL(int) IEMBreakpointSet(PVM pVM, RTGCPTR GCPtrBp)
{
    RT_NOREF_PV(pVM); RT_NOREF_PV(GCPtrBp);
    return VERR_NOT_IMPLEMENTED;
}


VMM_INT_DECL(int) IEMBreakpointClear(PVM pVM, RTGCPTR GCPtrBp)
{
    RT_NOREF_PV(pVM); RT_NOREF_PV(GCPtrBp);
    return VERR_NOT_IMPLEMENTED;
}


/**
 * Interface for HM and EM for executing string I/O OUT (write) instructions.
 *
 * This API ASSUMES that the caller has already verified that the guest code is
 * allowed to access the I/O port.  (The I/O port is in the DX register in the
 * guest state.)
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   cbValue             The size of the I/O port access (1, 2, or 4).
 * @param   enmAddrMode         The addressing mode.
 * @param   fRepPrefix          Indicates whether a repeat prefix is used
 *                              (doesn't matter which for this instruction).
 * @param   cbInstr             The instruction length in bytes.
 * @param   iEffSeg             The effective segment address.
 * @param   fIoChecked          Whether the access to the I/O port has been
 *                              checked or not.  It's typically checked in the
 *                              HM scenario.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecStringIoWrite(PVMCPUCC pVCpu, uint8_t cbValue, IEMMODE enmAddrMode,
                                                bool fRepPrefix, uint8_t cbInstr, uint8_t iEffSeg, bool fIoChecked)
{
    AssertMsgReturn(iEffSeg < X86_SREG_COUNT, ("%#x\n", iEffSeg), VERR_IEM_INVALID_EFF_SEG);
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 1);

    /*
     * State init.
     */
    iemInitExec(pVCpu, false /*fBypassHandlers*/);

    /*
     * Switch orgy for getting to the right handler.
     */
    VBOXSTRICTRC rcStrict;
    if (fRepPrefix)
    {
        switch (enmAddrMode)
        {
            case IEMMODE_16BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_rep_outs_op8_addr16(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_rep_outs_op16_addr16(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_rep_outs_op32_addr16(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            case IEMMODE_32BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_rep_outs_op8_addr32(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_rep_outs_op16_addr32(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_rep_outs_op32_addr32(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            case IEMMODE_64BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_rep_outs_op8_addr64(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_rep_outs_op16_addr64(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_rep_outs_op32_addr64(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            default:
                AssertMsgFailedReturn(("enmAddrMode=%d\n", enmAddrMode), VERR_IEM_INVALID_ADDRESS_MODE);
        }
    }
    else
    {
        switch (enmAddrMode)
        {
            case IEMMODE_16BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_outs_op8_addr16(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_outs_op16_addr16(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_outs_op32_addr16(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            case IEMMODE_32BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_outs_op8_addr32(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_outs_op16_addr32(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_outs_op32_addr32(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            case IEMMODE_64BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_outs_op8_addr64(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_outs_op16_addr64(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_outs_op32_addr64(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            default:
                AssertMsgFailedReturn(("enmAddrMode=%d\n", enmAddrMode), VERR_IEM_INVALID_ADDRESS_MODE);
        }
    }

    if (pVCpu->iem.s.cActiveMappings)
        iemMemRollback(pVCpu);

    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM for executing string I/O IN (read) instructions.
 *
 * This API ASSUMES that the caller has already verified that the guest code is
 * allowed to access the I/O port.  (The I/O port is in the DX register in the
 * guest state.)
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   cbValue             The size of the I/O port access (1, 2, or 4).
 * @param   enmAddrMode         The addressing mode.
 * @param   fRepPrefix          Indicates whether a repeat prefix is used
 *                              (doesn't matter which for this instruction).
 * @param   cbInstr             The instruction length in bytes.
 * @param   fIoChecked          Whether the access to the I/O port has been
 *                              checked or not.  It's typically checked in the
 *                              HM scenario.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecStringIoRead(PVMCPUCC pVCpu, uint8_t cbValue, IEMMODE enmAddrMode,
                                               bool fRepPrefix, uint8_t cbInstr, bool fIoChecked)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 1);

    /*
     * State init.
     */
    iemInitExec(pVCpu, false /*fBypassHandlers*/);

    /*
     * Switch orgy for getting to the right handler.
     */
    VBOXSTRICTRC rcStrict;
    if (fRepPrefix)
    {
        switch (enmAddrMode)
        {
            case IEMMODE_16BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_rep_ins_op8_addr16(pVCpu, cbInstr, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_rep_ins_op16_addr16(pVCpu, cbInstr, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_rep_ins_op32_addr16(pVCpu, cbInstr, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            case IEMMODE_32BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_rep_ins_op8_addr32(pVCpu, cbInstr, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_rep_ins_op16_addr32(pVCpu, cbInstr, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_rep_ins_op32_addr32(pVCpu, cbInstr, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            case IEMMODE_64BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_rep_ins_op8_addr64(pVCpu, cbInstr, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_rep_ins_op16_addr64(pVCpu, cbInstr, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_rep_ins_op32_addr64(pVCpu, cbInstr, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            default:
                AssertMsgFailedReturn(("enmAddrMode=%d\n", enmAddrMode), VERR_IEM_INVALID_ADDRESS_MODE);
        }
    }
    else
    {
        switch (enmAddrMode)
        {
            case IEMMODE_16BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_ins_op8_addr16(pVCpu, cbInstr, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_ins_op16_addr16(pVCpu, cbInstr, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_ins_op32_addr16(pVCpu, cbInstr, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            case IEMMODE_32BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_ins_op8_addr32(pVCpu, cbInstr, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_ins_op16_addr32(pVCpu, cbInstr, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_ins_op32_addr32(pVCpu, cbInstr, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            case IEMMODE_64BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_ins_op8_addr64(pVCpu, cbInstr, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_ins_op16_addr64(pVCpu, cbInstr, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_ins_op32_addr64(pVCpu, cbInstr, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            default:
                AssertMsgFailedReturn(("enmAddrMode=%d\n", enmAddrMode), VERR_IEM_INVALID_ADDRESS_MODE);
        }
    }

    if (   pVCpu->iem.s.cActiveMappings == 0
        || VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_IEM))
    { /* likely */ }
    else
    {
        AssertMsg(!IOM_SUCCESS(rcStrict), ("%#x\n", VBOXSTRICTRC_VAL(rcStrict)));
        iemMemRollback(pVCpu);
    }
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for rawmode to write execute an OUT instruction.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cbInstr     The instruction length in bytes.
 * @param   u16Port     The port to read.
 * @param   fImm        Whether the port is specified using an immediate operand or
 *                      using the implicit DX register.
 * @param   cbReg       The register size.
 *
 * @remarks In ring-0 not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedOut(PVMCPUCC pVCpu, uint8_t cbInstr, uint16_t u16Port, bool fImm, uint8_t cbReg)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 1);
    Assert(cbReg <= 4 && cbReg != 3);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_3(iemCImpl_out, u16Port, fImm, cbReg);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for rawmode to write execute an IN instruction.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cbInstr     The instruction length in bytes.
 * @param   u16Port     The port to read.
 * @param   fImm        Whether the port is specified using an immediate operand or
 *                      using the implicit DX.
 * @param   cbReg       The register size.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedIn(PVMCPUCC pVCpu, uint8_t cbInstr, uint16_t u16Port, bool fImm, uint8_t cbReg)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 1);
    Assert(cbReg <= 4 && cbReg != 3);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_3(iemCImpl_in, u16Port, fImm, cbReg);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to write to a CRx register.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cbInstr     The instruction length in bytes.
 * @param   iCrReg      The control register number (destination).
 * @param   iGReg       The general purpose register number (source).
 *
 * @remarks In ring-0 not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedMovCRxWrite(PVMCPUCC pVCpu, uint8_t cbInstr, uint8_t iCrReg, uint8_t iGReg)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 2);
    Assert(iCrReg < 16);
    Assert(iGReg < 16);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_2(iemCImpl_mov_Cd_Rd, iCrReg, iGReg);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to read from a CRx register.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cbInstr     The instruction length in bytes.
 * @param   iGReg       The general purpose register number (destination).
 * @param   iCrReg      The control register number (source).
 *
 * @remarks In ring-0 not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedMovCRxRead(PVMCPUCC pVCpu, uint8_t cbInstr, uint8_t iGReg, uint8_t iCrReg)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 2);
    IEM_CTX_ASSERT(pVCpu, IEM_CPUMCTX_EXTRN_EXEC_DECODED_NO_MEM_MASK | CPUMCTX_EXTRN_CR3 | CPUMCTX_EXTRN_CR4
                        | CPUMCTX_EXTRN_APIC_TPR);
    Assert(iCrReg < 16);
    Assert(iGReg < 16);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_2(iemCImpl_mov_Rd_Cd, iGReg, iCrReg);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to write to a DRx register.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cbInstr     The instruction length in bytes.
 * @param   iDrReg      The debug register number (destination).
 * @param   iGReg       The general purpose register number (source).
 *
 * @remarks In ring-0 not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedMovDRxWrite(PVMCPUCC pVCpu, uint8_t cbInstr, uint8_t iDrReg, uint8_t iGReg)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 2);
    IEM_CTX_ASSERT(pVCpu, IEM_CPUMCTX_EXTRN_EXEC_DECODED_NO_MEM_MASK | CPUMCTX_EXTRN_DR7);
    Assert(iDrReg < 8);
    Assert(iGReg < 16);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_2(iemCImpl_mov_Dd_Rd, iDrReg, iGReg);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to read from a DRx register.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cbInstr     The instruction length in bytes.
 * @param   iGReg       The general purpose register number (destination).
 * @param   iDrReg      The debug register number (source).
 *
 * @remarks In ring-0 not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedMovDRxRead(PVMCPUCC pVCpu, uint8_t cbInstr, uint8_t iGReg, uint8_t iDrReg)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 2);
    IEM_CTX_ASSERT(pVCpu, IEM_CPUMCTX_EXTRN_EXEC_DECODED_NO_MEM_MASK | CPUMCTX_EXTRN_DR7);
    Assert(iDrReg < 8);
    Assert(iGReg < 16);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_2(iemCImpl_mov_Rd_Dd, iGReg, iDrReg);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to clear the CR0[TS] bit.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cbInstr     The instruction length in bytes.
 *
 * @remarks In ring-0 not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedClts(PVMCPUCC pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 2);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_0(iemCImpl_clts);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the LMSW instruction (loads CR0).
 *
 * @returns Strict VBox status code.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   cbInstr         The instruction length in bytes.
 * @param   uValue          The value to load into CR0.
 * @param   GCPtrEffDst     The guest-linear address if the LMSW instruction has a
 *                          memory operand. Otherwise pass NIL_RTGCPTR.
 *
 * @remarks In ring-0 not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedLmsw(PVMCPUCC pVCpu, uint8_t cbInstr, uint16_t uValue, RTGCPTR GCPtrEffDst)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 3);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_2(iemCImpl_lmsw, uValue, GCPtrEffDst);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the XSETBV instruction (loads XCRx).
 *
 * Takes input values in ecx and edx:eax of the CPU context of the calling EMT.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   cbInstr     The instruction length in bytes.
 * @remarks In ring-0 not all of the state needs to be synced in.
 * @thread  EMT(pVCpu)
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedXsetbv(PVMCPUCC pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 3);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_0(iemCImpl_xsetbv);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the WBINVD instruction.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cbInstr     The instruction length in bytes.
 *
 * @remarks In ring-0 not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedWbinvd(PVMCPUCC pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 2);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_0(iemCImpl_wbinvd);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the INVD instruction.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cbInstr     The instruction length in bytes.
 *
 * @remarks In ring-0 not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedInvd(PVMCPUCC pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 2);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_0(iemCImpl_invd);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the INVLPG instruction.
 *
 * @returns Strict VBox status code.
 * @retval  VINF_PGM_SYNC_CR3
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cbInstr     The instruction length in bytes.
 * @param   GCPtrPage   The effective address of the page to invalidate.
 *
 * @remarks In ring-0 not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedInvlpg(PVMCPUCC pVCpu, uint8_t cbInstr, RTGCPTR GCPtrPage)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 3);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_1(iemCImpl_invlpg, GCPtrPage);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the INVPCID instruction.
 *
 * @returns Strict VBox status code.
 * @retval  VINF_PGM_SYNC_CR3
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cbInstr     The instruction length in bytes.
 * @param   iEffSeg     The effective segment register.
 * @param   GCPtrDesc   The effective address of the INVPCID descriptor.
 * @param   uType       The invalidation type.
 *
 * @remarks In ring-0 not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedInvpcid(PVMCPUCC pVCpu, uint8_t cbInstr, uint8_t iEffSeg, RTGCPTR GCPtrDesc,
                                                 uint64_t uType)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 4);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_3(iemCImpl_invpcid, iEffSeg, GCPtrDesc, uType);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the CPUID instruction.
 *
 * @returns Strict VBox status code.
 *
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   cbInstr             The instruction length in bytes.
 *
 * @remarks Not all of the state needs to be synced in, the usual pluss RAX and RCX.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedCpuid(PVMCPUCC pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 2);
    IEM_CTX_ASSERT(pVCpu, IEM_CPUMCTX_EXTRN_EXEC_DECODED_NO_MEM_MASK | CPUMCTX_EXTRN_RAX | CPUMCTX_EXTRN_RCX);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_0(iemCImpl_cpuid);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the RDPMC instruction.
 *
 * @returns Strict VBox status code.
 *
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   cbInstr             The instruction length in bytes.
 *
 * @remarks Not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedRdpmc(PVMCPUCC pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 2);
    IEM_CTX_ASSERT(pVCpu, IEM_CPUMCTX_EXTRN_EXEC_DECODED_NO_MEM_MASK | CPUMCTX_EXTRN_CR4);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_0(iemCImpl_rdpmc);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the RDTSC instruction.
 *
 * @returns Strict VBox status code.
 * @retval  VINF_IEM_RAISED_XCPT (VINF_EM_RESCHEDULE) if exception is raised.
 *
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   cbInstr             The instruction length in bytes.
 *
 * @remarks Not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedRdtsc(PVMCPUCC pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 2);
    IEM_CTX_ASSERT(pVCpu, IEM_CPUMCTX_EXTRN_EXEC_DECODED_NO_MEM_MASK | CPUMCTX_EXTRN_CR4);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_0(iemCImpl_rdtsc);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the RDTSCP instruction.
 *
 * @returns Strict VBox status code.
 * @retval  VINF_IEM_RAISED_XCPT (VINF_EM_RESCHEDULE) if exception is raised.
 *
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   cbInstr             The instruction length in bytes.
 *
 * @remarks Not all of the state needs to be synced in.  Recommended
 *          to include CPUMCTX_EXTRN_TSC_AUX, to avoid extra fetch call.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedRdtscp(PVMCPUCC pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 3);
    IEM_CTX_ASSERT(pVCpu, IEM_CPUMCTX_EXTRN_EXEC_DECODED_NO_MEM_MASK | CPUMCTX_EXTRN_CR4 | CPUMCTX_EXTRN_TSC_AUX);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_0(iemCImpl_rdtscp);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the RDMSR instruction.
 *
 * @returns Strict VBox status code.
 * @retval  VINF_IEM_RAISED_XCPT (VINF_EM_RESCHEDULE) if exception is raised.
 *
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   cbInstr             The instruction length in bytes.
 *
 * @remarks Not all of the state needs to be synced in.  Requires RCX and
 *          (currently) all MSRs.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedRdmsr(PVMCPUCC pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 2);
    IEM_CTX_ASSERT(pVCpu, IEM_CPUMCTX_EXTRN_EXEC_DECODED_NO_MEM_MASK | CPUMCTX_EXTRN_RCX | CPUMCTX_EXTRN_ALL_MSRS);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_0(iemCImpl_rdmsr);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the WRMSR instruction.
 *
 * @returns Strict VBox status code.
 * @retval  VINF_IEM_RAISED_XCPT (VINF_EM_RESCHEDULE) if exception is raised.
 *
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   cbInstr             The instruction length in bytes.
 *
 * @remarks Not all of the state needs to be synced in.  Requires RCX, RAX, RDX,
 *          and (currently) all MSRs.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedWrmsr(PVMCPUCC pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 2);
    IEM_CTX_ASSERT(pVCpu, IEM_CPUMCTX_EXTRN_EXEC_DECODED_NO_MEM_MASK
                        | CPUMCTX_EXTRN_RCX | CPUMCTX_EXTRN_RAX | CPUMCTX_EXTRN_RDX | CPUMCTX_EXTRN_ALL_MSRS);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_0(iemCImpl_wrmsr);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the MONITOR instruction.
 *
 * @returns Strict VBox status code.
 * @retval  VINF_IEM_RAISED_XCPT (VINF_EM_RESCHEDULE) if exception is raised.
 *
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   cbInstr             The instruction length in bytes.
 *
 * @remarks Not all of the state needs to be synced in.
 * @remarks ASSUMES the default segment of DS and no segment override prefixes
 *          are used.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedMonitor(PVMCPUCC pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 3);
    IEM_CTX_ASSERT(pVCpu, IEM_CPUMCTX_EXTRN_EXEC_DECODED_MEM_MASK | CPUMCTX_EXTRN_DS);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_1(iemCImpl_monitor, X86_SREG_DS);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the MWAIT instruction.
 *
 * @returns Strict VBox status code.
 * @retval  VINF_IEM_RAISED_XCPT (VINF_EM_RESCHEDULE) if exception is raised.
 *
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   cbInstr             The instruction length in bytes.
 *
 * @remarks Not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedMwait(PVMCPUCC pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 3);
    IEM_CTX_ASSERT(pVCpu, IEM_CPUMCTX_EXTRN_EXEC_DECODED_NO_MEM_MASK | CPUMCTX_EXTRN_RCX | CPUMCTX_EXTRN_RAX);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_0(iemCImpl_mwait);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the HLT instruction.
 *
 * @returns Strict VBox status code.
 * @retval  VINF_IEM_RAISED_XCPT (VINF_EM_RESCHEDULE) if exception is raised.
 *
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   cbInstr             The instruction length in bytes.
 *
 * @remarks Not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedHlt(PVMCPUCC pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 1);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_0(iemCImpl_hlt);
    Assert(!pVCpu->iem.s.cActiveMappings);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Checks if IEM is in the process of delivering an event (interrupt or
 * exception).
 *
 * @returns true if we're in the process of raising an interrupt or exception,
 *          false otherwise.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   puVector        Where to store the vector associated with the
 *                          currently delivered event, optional.
 * @param   pfFlags         Where to store th event delivery flags (see
 *                          IEM_XCPT_FLAGS_XXX), optional.
 * @param   puErr           Where to store the error code associated with the
 *                          event, optional.
 * @param   puCr2           Where to store the CR2 associated with the event,
 *                          optional.
 * @remarks The caller should check the flags to determine if the error code and
 *          CR2 are valid for the event.
 */
VMM_INT_DECL(bool) IEMGetCurrentXcpt(PVMCPUCC pVCpu, uint8_t *puVector, uint32_t *pfFlags, uint32_t *puErr, uint64_t *puCr2)
{
    bool const fRaisingXcpt = pVCpu->iem.s.cXcptRecursions > 0;
    if (fRaisingXcpt)
    {
        if (puVector)
            *puVector = pVCpu->iem.s.uCurXcpt;
        if (pfFlags)
            *pfFlags = pVCpu->iem.s.fCurXcpt;
        if (puErr)
            *puErr = pVCpu->iem.s.uCurXcptErr;
        if (puCr2)
            *puCr2 = pVCpu->iem.s.uCurXcptCr2;
    }
    return fRaisingXcpt;
}

#ifdef IN_RING3

/**
 * Handles the unlikely and probably fatal merge cases.
 *
 * @returns Merged status code.
 * @param   rcStrict        Current EM status code.
 * @param   rcStrictCommit  The IOM I/O or MMIO write commit status to merge
 *                          with @a rcStrict.
 * @param   iMemMap         The memory mapping index. For error reporting only.
 * @param   pVCpu           The cross context virtual CPU structure of the calling
 *                          thread, for error reporting only.
 */
DECL_NO_INLINE(static, VBOXSTRICTRC) iemR3MergeStatusSlow(VBOXSTRICTRC rcStrict, VBOXSTRICTRC rcStrictCommit,
                                                          unsigned iMemMap, PVMCPUCC pVCpu)
{
    if (RT_FAILURE_NP(rcStrict))
        return rcStrict;

    if (RT_FAILURE_NP(rcStrictCommit))
        return rcStrictCommit;

    if (rcStrict == rcStrictCommit)
        return rcStrictCommit;

    AssertLogRelMsgFailed(("rcStrictCommit=%Rrc rcStrict=%Rrc iMemMap=%u fAccess=%#x FirstPg=%RGp LB %u SecondPg=%RGp LB %u\n",
                           VBOXSTRICTRC_VAL(rcStrictCommit), VBOXSTRICTRC_VAL(rcStrict), iMemMap,
                           pVCpu->iem.s.aMemMappings[iMemMap].fAccess,
                           pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, pVCpu->iem.s.aMemBbMappings[iMemMap].cbFirst,
                           pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, pVCpu->iem.s.aMemBbMappings[iMemMap].cbSecond));
    return VERR_IOM_FF_STATUS_IPE;
}


/**
 * Helper for IOMR3ProcessForceFlag.
 *
 * @returns Merged status code.
 * @param   rcStrict        Current EM status code.
 * @param   rcStrictCommit  The IOM I/O or MMIO write commit status to merge
 *                          with @a rcStrict.
 * @param   iMemMap         The memory mapping index. For error reporting only.
 * @param   pVCpu           The cross context virtual CPU structure of the calling
 *                          thread, for error reporting only.
 */
DECLINLINE(VBOXSTRICTRC) iemR3MergeStatus(VBOXSTRICTRC rcStrict, VBOXSTRICTRC rcStrictCommit, unsigned iMemMap, PVMCPUCC pVCpu)
{
    /* Simple. */
    if (RT_LIKELY(rcStrict == VINF_SUCCESS || rcStrict == VINF_EM_RAW_TO_R3))
        return rcStrictCommit;

    if (RT_LIKELY(rcStrictCommit == VINF_SUCCESS))
        return rcStrict;

    /* EM scheduling status codes. */
    if (RT_LIKELY(   rcStrict >= VINF_EM_FIRST
                  && rcStrict <= VINF_EM_LAST))
    {
        if (RT_LIKELY(   rcStrictCommit >= VINF_EM_FIRST
                      && rcStrictCommit <= VINF_EM_LAST))
            return rcStrict < rcStrictCommit ? rcStrict : rcStrictCommit;
    }

    /* Unlikely */
    return iemR3MergeStatusSlow(rcStrict, rcStrictCommit, iMemMap, pVCpu);
}


/**
 * Called by force-flag handling code when VMCPU_FF_IEM is set.
 *
 * @returns Merge between @a rcStrict and what the commit operation returned.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   rcStrict    The status code returned by ring-0 or raw-mode.
 */
VMMR3_INT_DECL(VBOXSTRICTRC) IEMR3ProcessForceFlag(PVM pVM, PVMCPUCC pVCpu, VBOXSTRICTRC rcStrict)
{
    /*
     * Reset the pending commit.
     */
    AssertMsg(  (pVCpu->iem.s.aMemMappings[0].fAccess | pVCpu->iem.s.aMemMappings[1].fAccess | pVCpu->iem.s.aMemMappings[2].fAccess)
              & (IEM_ACCESS_PENDING_R3_WRITE_1ST | IEM_ACCESS_PENDING_R3_WRITE_2ND),
              ("%#x %#x %#x\n",
               pVCpu->iem.s.aMemMappings[0].fAccess, pVCpu->iem.s.aMemMappings[1].fAccess, pVCpu->iem.s.aMemMappings[2].fAccess));
    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_IEM);

    /*
     * Commit the pending bounce buffers (usually just one).
     */
    unsigned cBufs = 0;
    unsigned iMemMap = RT_ELEMENTS(pVCpu->iem.s.aMemMappings);
    while (iMemMap-- > 0)
        if (pVCpu->iem.s.aMemMappings[iMemMap].fAccess & (IEM_ACCESS_PENDING_R3_WRITE_1ST | IEM_ACCESS_PENDING_R3_WRITE_2ND))
        {
            Assert(pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_TYPE_WRITE);
            Assert(pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_BOUNCE_BUFFERED);
            Assert(!pVCpu->iem.s.aMemBbMappings[iMemMap].fUnassigned);

            uint16_t const  cbFirst  = pVCpu->iem.s.aMemBbMappings[iMemMap].cbFirst;
            uint16_t const  cbSecond = pVCpu->iem.s.aMemBbMappings[iMemMap].cbSecond;
            uint8_t const  *pbBuf    = &pVCpu->iem.s.aBounceBuffers[iMemMap].ab[0];

            if (pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_PENDING_R3_WRITE_1ST)
            {
                VBOXSTRICTRC rcStrictCommit1 = PGMPhysWrite(pVM,
                                                            pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst,
                                                            pbBuf,
                                                            cbFirst,
                                                            PGMACCESSORIGIN_IEM);
                rcStrict = iemR3MergeStatus(rcStrict, rcStrictCommit1, iMemMap, pVCpu);
                Log(("IEMR3ProcessForceFlag: iMemMap=%u GCPhysFirst=%RGp LB %#x %Rrc => %Rrc\n",
                     iMemMap, pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst,
                     VBOXSTRICTRC_VAL(rcStrictCommit1), VBOXSTRICTRC_VAL(rcStrict)));
            }

            if (pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_PENDING_R3_WRITE_2ND)
            {
                VBOXSTRICTRC rcStrictCommit2 = PGMPhysWrite(pVM,
                                                            pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond,
                                                            pbBuf + cbFirst,
                                                            cbSecond,
                                                            PGMACCESSORIGIN_IEM);
                rcStrict = iemR3MergeStatus(rcStrict, rcStrictCommit2, iMemMap, pVCpu);
                Log(("IEMR3ProcessForceFlag: iMemMap=%u GCPhysSecond=%RGp LB %#x %Rrc => %Rrc\n",
                     iMemMap, pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond,
                     VBOXSTRICTRC_VAL(rcStrictCommit2), VBOXSTRICTRC_VAL(rcStrict)));
            }
            cBufs++;
            pVCpu->iem.s.aMemMappings[iMemMap].fAccess = IEM_ACCESS_INVALID;
        }

    AssertMsg(cBufs > 0 && cBufs == pVCpu->iem.s.cActiveMappings,
              ("cBufs=%u cActiveMappings=%u - %#x %#x %#x\n", cBufs, pVCpu->iem.s.cActiveMappings,
               pVCpu->iem.s.aMemMappings[0].fAccess, pVCpu->iem.s.aMemMappings[1].fAccess, pVCpu->iem.s.aMemMappings[2].fAccess));
    pVCpu->iem.s.cActiveMappings = 0;
    return rcStrict;
}

#endif /* IN_RING3 */

