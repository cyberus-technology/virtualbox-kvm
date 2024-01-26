/* $Id: IEMInline.h $ */
/** @file
 * IEM - Interpreted Execution Manager - Inlined Functions.
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

#ifndef VMM_INCLUDED_SRC_include_IEMInline_h
#define VMM_INCLUDED_SRC_include_IEMInline_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif



/**
 * Makes status code addjustments (pass up from I/O and access handler)
 * as well as maintaining statistics.
 *
 * @returns Strict VBox status code to pass up.
 * @param   pVCpu       The cross context virtual CPU structure of the calling thread.
 * @param   rcStrict    The status from executing an instruction.
 */
DECL_FORCE_INLINE(VBOXSTRICTRC) iemExecStatusCodeFiddling(PVMCPUCC pVCpu, VBOXSTRICTRC rcStrict) RT_NOEXCEPT
{
    if (rcStrict != VINF_SUCCESS)
    {
        if (RT_SUCCESS(rcStrict))
        {
            AssertMsg(   (rcStrict >= VINF_EM_FIRST && rcStrict <= VINF_EM_LAST)
                      || rcStrict == VINF_IOM_R3_IOPORT_READ
                      || rcStrict == VINF_IOM_R3_IOPORT_WRITE
                      || rcStrict == VINF_IOM_R3_IOPORT_COMMIT_WRITE
                      || rcStrict == VINF_IOM_R3_MMIO_READ
                      || rcStrict == VINF_IOM_R3_MMIO_READ_WRITE
                      || rcStrict == VINF_IOM_R3_MMIO_WRITE
                      || rcStrict == VINF_IOM_R3_MMIO_COMMIT_WRITE
                      || rcStrict == VINF_CPUM_R3_MSR_READ
                      || rcStrict == VINF_CPUM_R3_MSR_WRITE
                      || rcStrict == VINF_EM_RAW_EMULATE_INSTR
                      || rcStrict == VINF_EM_RAW_TO_R3
                      || rcStrict == VINF_EM_TRIPLE_FAULT
                      || rcStrict == VINF_GIM_R3_HYPERCALL
                      /* raw-mode / virt handlers only: */
                      || rcStrict == VINF_EM_RAW_EMULATE_INSTR_GDT_FAULT
                      || rcStrict == VINF_EM_RAW_EMULATE_INSTR_TSS_FAULT
                      || rcStrict == VINF_EM_RAW_EMULATE_INSTR_LDT_FAULT
                      || rcStrict == VINF_EM_RAW_EMULATE_INSTR_IDT_FAULT
                      || rcStrict == VINF_SELM_SYNC_GDT
                      || rcStrict == VINF_CSAM_PENDING_ACTION
                      || rcStrict == VINF_PATM_CHECK_PATCH_PAGE
                      /* nested hw.virt codes: */
                      || rcStrict == VINF_VMX_VMEXIT
                      || rcStrict == VINF_VMX_INTERCEPT_NOT_ACTIVE
                      || rcStrict == VINF_VMX_MODIFIES_BEHAVIOR
                      || rcStrict == VINF_SVM_VMEXIT
                      , ("rcStrict=%Rrc\n", VBOXSTRICTRC_VAL(rcStrict)));
/** @todo adjust for VINF_EM_RAW_EMULATE_INSTR. */
            int32_t const rcPassUp = pVCpu->iem.s.rcPassUp;
#ifdef VBOX_WITH_NESTED_HWVIRT_VMX
            if (   rcStrict == VINF_VMX_VMEXIT
                && rcPassUp == VINF_SUCCESS)
                rcStrict = VINF_SUCCESS;
            else
#endif
#ifdef VBOX_WITH_NESTED_HWVIRT_SVM
            if (   rcStrict == VINF_SVM_VMEXIT
                && rcPassUp == VINF_SUCCESS)
                rcStrict = VINF_SUCCESS;
            else
#endif
            if (rcPassUp == VINF_SUCCESS)
                pVCpu->iem.s.cRetInfStatuses++;
            else if (   rcPassUp < VINF_EM_FIRST
                     || rcPassUp > VINF_EM_LAST
                     || rcPassUp < VBOXSTRICTRC_VAL(rcStrict))
            {
                Log(("IEM: rcPassUp=%Rrc! rcStrict=%Rrc\n", rcPassUp, VBOXSTRICTRC_VAL(rcStrict)));
                pVCpu->iem.s.cRetPassUpStatus++;
                rcStrict = rcPassUp;
            }
            else
            {
                Log(("IEM: rcPassUp=%Rrc  rcStrict=%Rrc!\n", rcPassUp, VBOXSTRICTRC_VAL(rcStrict)));
                pVCpu->iem.s.cRetInfStatuses++;
            }
        }
        else if (rcStrict == VERR_IEM_ASPECT_NOT_IMPLEMENTED)
            pVCpu->iem.s.cRetAspectNotImplemented++;
        else if (rcStrict == VERR_IEM_INSTR_NOT_IMPLEMENTED)
            pVCpu->iem.s.cRetInstrNotImplemented++;
        else
            pVCpu->iem.s.cRetErrStatuses++;
    }
    else if (pVCpu->iem.s.rcPassUp != VINF_SUCCESS)
    {
        pVCpu->iem.s.cRetPassUpStatus++;
        rcStrict = pVCpu->iem.s.rcPassUp;
    }

    return rcStrict;
}


/**
 * Sets the pass up status.
 *
 * @returns VINF_SUCCESS.
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 * @param   rcPassUp            The pass up status.  Must be informational.
 *                              VINF_SUCCESS is not allowed.
 */
DECLINLINE(int) iemSetPassUpStatus(PVMCPUCC pVCpu, VBOXSTRICTRC rcPassUp) RT_NOEXCEPT
{
    AssertRC(VBOXSTRICTRC_VAL(rcPassUp)); Assert(rcPassUp != VINF_SUCCESS);

    int32_t const rcOldPassUp = pVCpu->iem.s.rcPassUp;
    if (rcOldPassUp == VINF_SUCCESS)
        pVCpu->iem.s.rcPassUp = VBOXSTRICTRC_VAL(rcPassUp);
    /* If both are EM scheduling codes, use EM priority rules. */
    else if (   rcOldPassUp >= VINF_EM_FIRST && rcOldPassUp <= VINF_EM_LAST
             && rcPassUp    >= VINF_EM_FIRST && rcPassUp    <= VINF_EM_LAST)
    {
        if (rcPassUp < rcOldPassUp)
        {
            Log(("IEM: rcPassUp=%Rrc! rcOldPassUp=%Rrc\n", VBOXSTRICTRC_VAL(rcPassUp), rcOldPassUp));
            pVCpu->iem.s.rcPassUp = VBOXSTRICTRC_VAL(rcPassUp);
        }
        else
            Log(("IEM: rcPassUp=%Rrc  rcOldPassUp=%Rrc!\n", VBOXSTRICTRC_VAL(rcPassUp), rcOldPassUp));
    }
    /* Override EM scheduling with specific status code. */
    else if (rcOldPassUp >= VINF_EM_FIRST && rcOldPassUp <= VINF_EM_LAST)
    {
        Log(("IEM: rcPassUp=%Rrc! rcOldPassUp=%Rrc\n", VBOXSTRICTRC_VAL(rcPassUp), rcOldPassUp));
        pVCpu->iem.s.rcPassUp = VBOXSTRICTRC_VAL(rcPassUp);
    }
    /* Don't override specific status code, first come first served. */
    else
        Log(("IEM: rcPassUp=%Rrc  rcOldPassUp=%Rrc!\n", VBOXSTRICTRC_VAL(rcPassUp), rcOldPassUp));
    return VINF_SUCCESS;
}


/**
 * Calculates the CPU mode.
 *
 * This is mainly for updating IEMCPU::enmCpuMode.
 *
 * @returns CPU mode.
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 */
DECLINLINE(IEMMODE) iemCalcCpuMode(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    if (CPUMIsGuestIn64BitCodeEx(&pVCpu->cpum.GstCtx))
        return IEMMODE_64BIT;
    if (pVCpu->cpum.GstCtx.cs.Attr.n.u1DefBig) /** @todo check if this is correct... */
        return IEMMODE_32BIT;
    return IEMMODE_16BIT;
}


#if defined(VBOX_INCLUDED_vmm_dbgf_h) || defined(DOXYGEN_RUNNING) /* dbgf.ro.cEnabledHwBreakpoints */
/**
 * Initializes the execution state.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 * @param   fBypassHandlers     Whether to bypass access handlers.
 *
 * @remarks Callers of this must call iemUninitExec() to undo potentially fatal
 *          side-effects in strict builds.
 */
DECLINLINE(void) iemInitExec(PVMCPUCC pVCpu, bool fBypassHandlers) RT_NOEXCEPT
{
    IEM_CTX_ASSERT(pVCpu, IEM_CPUMCTX_EXTRN_EXEC_DECODED_NO_MEM_MASK);
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
    pVCpu->iem.s.enmCpuMode         = iemCalcCpuMode(pVCpu);
# ifdef VBOX_STRICT
    pVCpu->iem.s.enmDefAddrMode     = (IEMMODE)0xfe;
    pVCpu->iem.s.enmEffAddrMode     = (IEMMODE)0xfe;
    pVCpu->iem.s.enmDefOpSize       = (IEMMODE)0xfe;
    pVCpu->iem.s.enmEffOpSize       = (IEMMODE)0xfe;
    pVCpu->iem.s.fPrefixes          = 0xfeedbeef;
    pVCpu->iem.s.uRexReg            = 127;
    pVCpu->iem.s.uRexB              = 127;
    pVCpu->iem.s.offModRm           = 127;
    pVCpu->iem.s.uRexIndex          = 127;
    pVCpu->iem.s.iEffSeg            = 127;
    pVCpu->iem.s.idxPrefix          = 127;
    pVCpu->iem.s.uVex3rdReg         = 127;
    pVCpu->iem.s.uVexLength         = 127;
    pVCpu->iem.s.fEvexStuff         = 127;
    pVCpu->iem.s.uFpuOpcode         = UINT16_MAX;
#  ifdef IEM_WITH_CODE_TLB
    pVCpu->iem.s.offInstrNextByte   = UINT16_MAX;
    pVCpu->iem.s.pbInstrBuf         = NULL;
    pVCpu->iem.s.cbInstrBuf         = UINT16_MAX;
    pVCpu->iem.s.cbInstrBufTotal    = UINT16_MAX;
    pVCpu->iem.s.offCurInstrStart   = INT16_MAX;
    pVCpu->iem.s.uInstrBufPc        = UINT64_C(0xc0ffc0ffcff0c0ff);
#  else
    pVCpu->iem.s.offOpcode          = 127;
    pVCpu->iem.s.cbOpcode           = 127;
#  endif
# endif /* VBOX_STRICT */

    pVCpu->iem.s.cActiveMappings    = 0;
    pVCpu->iem.s.iNextMapping       = 0;
    pVCpu->iem.s.rcPassUp           = VINF_SUCCESS;
    pVCpu->iem.s.fBypassHandlers                = fBypassHandlers;
    pVCpu->iem.s.fDisregardLock                 = false;
    pVCpu->iem.s.fPendingInstructionBreakpoints = false;
    pVCpu->iem.s.fPendingDataBreakpoints        = false;
    pVCpu->iem.s.fPendingIoBreakpoints          = false;
    if (RT_LIKELY(   !(pVCpu->cpum.GstCtx.dr[7] & X86_DR7_ENABLED_MASK)
                  && pVCpu->CTX_SUFF(pVM)->dbgf.ro.cEnabledHwBreakpoints == 0))
    { /* likely */ }
    else
        iemInitPendingBreakpointsSlow(pVCpu);
}
#endif /* VBOX_INCLUDED_vmm_dbgf_h */


#if defined(VBOX_WITH_NESTED_HWVIRT_SVM) || defined(VBOX_WITH_NESTED_HWVIRT_VMX)
/**
 * Performs a minimal reinitialization of the execution state.
 *
 * This is intended to be used by VM-exits, SMM, LOADALL and other similar
 * 'world-switch' types operations on the CPU. Currently only nested
 * hardware-virtualization uses it.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 */
DECLINLINE(void) iemReInitExec(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    IEMMODE const enmMode = iemCalcCpuMode(pVCpu);
    uint8_t const uCpl    = CPUMGetGuestCPL(pVCpu);

    pVCpu->iem.s.uCpl             = uCpl;
    pVCpu->iem.s.enmCpuMode       = enmMode;
    pVCpu->iem.s.enmDefAddrMode   = enmMode;  /** @todo check if this is correct... */
    pVCpu->iem.s.enmEffAddrMode   = enmMode;
    if (enmMode != IEMMODE_64BIT)
    {
        pVCpu->iem.s.enmDefOpSize = enmMode;  /** @todo check if this is correct... */
        pVCpu->iem.s.enmEffOpSize = enmMode;
    }
    else
    {
        pVCpu->iem.s.enmDefOpSize = IEMMODE_32BIT;
        pVCpu->iem.s.enmEffOpSize = enmMode;
    }
    pVCpu->iem.s.iEffSeg          = X86_SREG_DS;
# ifndef IEM_WITH_CODE_TLB
    /** @todo Shouldn't we be doing this in IEMTlbInvalidateAll()? */
    pVCpu->iem.s.offOpcode        = 0;
    pVCpu->iem.s.cbOpcode         = 0;
# endif
    pVCpu->iem.s.rcPassUp         = VINF_SUCCESS;
}
#endif


/**
 * Counterpart to #iemInitExec that undoes evil strict-build stuff.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 */
DECLINLINE(void) iemUninitExec(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    /* Note! do not touch fInPatchCode here! (see iemUninitExecAndFiddleStatusAndMaybeReenter) */
#ifdef VBOX_STRICT
# ifdef IEM_WITH_CODE_TLB
    NOREF(pVCpu);
# else
    pVCpu->iem.s.cbOpcode = 0;
# endif
#else
    NOREF(pVCpu);
#endif
}


/**
 * Calls iemUninitExec, iemExecStatusCodeFiddling and iemRCRawMaybeReenter.
 *
 * Only calling iemRCRawMaybeReenter in raw-mode, obviously.
 *
 * @returns Fiddled strict vbox status code, ready to return to non-IEM caller.
 * @param   pVCpu       The cross context virtual CPU structure of the calling thread.
 * @param   rcStrict    The status code to fiddle.
 */
DECLINLINE(VBOXSTRICTRC) iemUninitExecAndFiddleStatusAndMaybeReenter(PVMCPUCC pVCpu, VBOXSTRICTRC rcStrict) RT_NOEXCEPT
{
    iemUninitExec(pVCpu);
    return iemExecStatusCodeFiddling(pVCpu, rcStrict);
}


/**
 * Macro used by the IEMExec* method to check the given instruction length.
 *
 * Will return on failure!
 *
 * @param   a_cbInstr   The given instruction length.
 * @param   a_cbMin     The minimum length.
 */
#define IEMEXEC_ASSERT_INSTR_LEN_RETURN(a_cbInstr, a_cbMin) \
    AssertMsgReturn((unsigned)(a_cbInstr) - (unsigned)(a_cbMin) <= (unsigned)15 - (unsigned)(a_cbMin), \
                    ("cbInstr=%u cbMin=%u\n", (a_cbInstr), (a_cbMin)), VERR_IEM_INVALID_INSTR_LENGTH)


#ifndef IEM_WITH_SETJMP

/**
 * Fetches the first opcode byte.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 * @param   pu8                 Where to return the opcode byte.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetFirstU8(PVMCPUCC pVCpu, uint8_t *pu8) RT_NOEXCEPT
{
    /*
     * Check for hardware instruction breakpoints.
     */
    if (RT_LIKELY(!pVCpu->iem.s.fPendingInstructionBreakpoints))
    { /* likely */ }
    else
    {
        VBOXSTRICTRC rcStrict = DBGFBpCheckInstruction(pVCpu->CTX_SUFF(pVM), pVCpu,
                                                       pVCpu->cpum.GstCtx.rip + pVCpu->cpum.GstCtx.cs.u64Base);
        if (RT_LIKELY(rcStrict == VINF_SUCCESS))
        { /* likely */ }
        else if (rcStrict == VINF_EM_RAW_GUEST_TRAP)
            return iemRaiseDebugException(pVCpu);
        else
            return rcStrict;
    }

    /*
     * Fetch the first opcode byte.
     */
    uintptr_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_LIKELY((uint8_t)offOpcode < pVCpu->iem.s.cbOpcode))
    {
        pVCpu->iem.s.offOpcode = (uint8_t)offOpcode + 1;
        *pu8 = pVCpu->iem.s.abOpcode[offOpcode];
        return VINF_SUCCESS;
    }
    return iemOpcodeGetNextU8Slow(pVCpu, pu8);
}

#else  /* IEM_WITH_SETJMP */

/**
 * Fetches the first opcode byte, longjmp on error.
 *
 * @returns The opcode byte.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECL_INLINE_THROW(uint8_t) iemOpcodeGetFirstU8Jmp(PVMCPUCC pVCpu) IEM_NOEXCEPT_MAY_LONGJMP
{
    /*
     * Check for hardware instruction breakpoints.
     */
    if (RT_LIKELY(!pVCpu->iem.s.fPendingInstructionBreakpoints))
    { /* likely */ }
    else
    {
        VBOXSTRICTRC rcStrict = DBGFBpCheckInstruction(pVCpu->CTX_SUFF(pVM), pVCpu,
                                                       pVCpu->cpum.GstCtx.rip + pVCpu->cpum.GstCtx.cs.u64Base);
        if (RT_LIKELY(rcStrict == VINF_SUCCESS))
        { /* likely */ }
        else
        {
            if (rcStrict == VINF_EM_RAW_GUEST_TRAP)
                rcStrict = iemRaiseDebugException(pVCpu);
            IEM_DO_LONGJMP(pVCpu, VBOXSTRICTRC_VAL(rcStrict));
        }
    }

    /*
     * Fetch the first opcode byte.
     */
# ifdef IEM_WITH_CODE_TLB
    uintptr_t       offBuf = pVCpu->iem.s.offInstrNextByte;
    uint8_t const  *pbBuf  = pVCpu->iem.s.pbInstrBuf;
    if (RT_LIKELY(   pbBuf != NULL
                  && offBuf < pVCpu->iem.s.cbInstrBuf))
    {
        pVCpu->iem.s.offInstrNextByte = (uint32_t)offBuf + 1;
        return pbBuf[offBuf];
    }
# else
    uintptr_t offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_LIKELY((uint8_t)offOpcode < pVCpu->iem.s.cbOpcode))
    {
        pVCpu->iem.s.offOpcode = (uint8_t)offOpcode + 1;
        return pVCpu->iem.s.abOpcode[offOpcode];
    }
# endif
    return iemOpcodeGetNextU8SlowJmp(pVCpu);
}

#endif /* IEM_WITH_SETJMP */

/**
 * Fetches the first opcode byte, returns/throws automatically on failure.
 *
 * @param   a_pu8               Where to return the opcode byte.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_FIRST_U8(a_pu8) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetFirstU8(pVCpu, (a_pu8)); \
        if (rcStrict2 == VINF_SUCCESS) \
        { /* likely */ } \
        else \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_FIRST_U8(a_pu8) (*(a_pu8) = iemOpcodeGetFirstU8Jmp(pVCpu))
#endif /* IEM_WITH_SETJMP */


#ifndef IEM_WITH_SETJMP

/**
 * Fetches the next opcode byte.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 * @param   pu8                 Where to return the opcode byte.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextU8(PVMCPUCC pVCpu, uint8_t *pu8) RT_NOEXCEPT
{
    uintptr_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_LIKELY((uint8_t)offOpcode < pVCpu->iem.s.cbOpcode))
    {
        pVCpu->iem.s.offOpcode = (uint8_t)offOpcode + 1;
        *pu8 = pVCpu->iem.s.abOpcode[offOpcode];
        return VINF_SUCCESS;
    }
    return iemOpcodeGetNextU8Slow(pVCpu, pu8);
}

#else  /* IEM_WITH_SETJMP */

/**
 * Fetches the next opcode byte, longjmp on error.
 *
 * @returns The opcode byte.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECL_INLINE_THROW(uint8_t) iemOpcodeGetNextU8Jmp(PVMCPUCC pVCpu) IEM_NOEXCEPT_MAY_LONGJMP
{
# ifdef IEM_WITH_CODE_TLB
    uintptr_t       offBuf = pVCpu->iem.s.offInstrNextByte;
    uint8_t const  *pbBuf  = pVCpu->iem.s.pbInstrBuf;
    if (RT_LIKELY(   pbBuf != NULL
                  && offBuf < pVCpu->iem.s.cbInstrBuf))
    {
        pVCpu->iem.s.offInstrNextByte = (uint32_t)offBuf + 1;
        return pbBuf[offBuf];
    }
# else
    uintptr_t offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_LIKELY((uint8_t)offOpcode < pVCpu->iem.s.cbOpcode))
    {
        pVCpu->iem.s.offOpcode = (uint8_t)offOpcode + 1;
        return pVCpu->iem.s.abOpcode[offOpcode];
    }
# endif
    return iemOpcodeGetNextU8SlowJmp(pVCpu);
}

#endif /* IEM_WITH_SETJMP */

/**
 * Fetches the next opcode byte, returns automatically on failure.
 *
 * @param   a_pu8               Where to return the opcode byte.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_U8(a_pu8) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextU8(pVCpu, (a_pu8)); \
        if (rcStrict2 == VINF_SUCCESS) \
        { /* likely */ } \
        else \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_U8(a_pu8) (*(a_pu8) = iemOpcodeGetNextU8Jmp(pVCpu))
#endif /* IEM_WITH_SETJMP */


#ifndef IEM_WITH_SETJMP
/**
 * Fetches the next signed byte from the opcode stream.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pi8                 Where to return the signed byte.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextS8(PVMCPUCC pVCpu, int8_t *pi8) RT_NOEXCEPT
{
    return iemOpcodeGetNextU8(pVCpu, (uint8_t *)pi8);
}
#endif /* !IEM_WITH_SETJMP */


/**
 * Fetches the next signed byte from the opcode stream, returning automatically
 * on failure.
 *
 * @param   a_pi8               Where to return the signed byte.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_S8(a_pi8) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextS8(pVCpu, (a_pi8)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else /* IEM_WITH_SETJMP */
# define IEM_OPCODE_GET_NEXT_S8(a_pi8) (*(a_pi8) = (int8_t)iemOpcodeGetNextU8Jmp(pVCpu))

#endif /* IEM_WITH_SETJMP */


#ifndef IEM_WITH_SETJMP
/**
 * Fetches the next signed byte from the opcode stream, extending it to
 * unsigned 16-bit.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu16                Where to return the unsigned word.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextS8SxU16(PVMCPUCC pVCpu, uint16_t *pu16) RT_NOEXCEPT
{
    uint8_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_UNLIKELY(offOpcode >= pVCpu->iem.s.cbOpcode))
        return iemOpcodeGetNextS8SxU16Slow(pVCpu, pu16);

    *pu16 = (int8_t)pVCpu->iem.s.abOpcode[offOpcode];
    pVCpu->iem.s.offOpcode = offOpcode + 1;
    return VINF_SUCCESS;
}
#endif /* !IEM_WITH_SETJMP */

/**
 * Fetches the next signed byte from the opcode stream and sign-extending it to
 * a word, returning automatically on failure.
 *
 * @param   a_pu16              Where to return the word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_S8_SX_U16(a_pu16) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextS8SxU16(pVCpu, (a_pu16)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_S8_SX_U16(a_pu16) (*(a_pu16) = (int8_t)iemOpcodeGetNextU8Jmp(pVCpu))
#endif

#ifndef IEM_WITH_SETJMP
/**
 * Fetches the next signed byte from the opcode stream, extending it to
 * unsigned 32-bit.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu32                Where to return the unsigned dword.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextS8SxU32(PVMCPUCC pVCpu, uint32_t *pu32) RT_NOEXCEPT
{
    uint8_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_UNLIKELY(offOpcode >= pVCpu->iem.s.cbOpcode))
        return iemOpcodeGetNextS8SxU32Slow(pVCpu, pu32);

    *pu32 = (int8_t)pVCpu->iem.s.abOpcode[offOpcode];
    pVCpu->iem.s.offOpcode = offOpcode + 1;
    return VINF_SUCCESS;
}
#endif /* !IEM_WITH_SETJMP */

/**
 * Fetches the next signed byte from the opcode stream and sign-extending it to
 * a word, returning automatically on failure.
 *
 * @param   a_pu32              Where to return the word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_S8_SX_U32(a_pu32) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextS8SxU32(pVCpu, (a_pu32)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_S8_SX_U32(a_pu32) (*(a_pu32) = (int8_t)iemOpcodeGetNextU8Jmp(pVCpu))
#endif


#ifndef IEM_WITH_SETJMP
/**
 * Fetches the next signed byte from the opcode stream, extending it to
 * unsigned 64-bit.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64                Where to return the unsigned qword.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextS8SxU64(PVMCPUCC pVCpu, uint64_t *pu64) RT_NOEXCEPT
{
    uint8_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_UNLIKELY(offOpcode >= pVCpu->iem.s.cbOpcode))
        return iemOpcodeGetNextS8SxU64Slow(pVCpu, pu64);

    *pu64 = (int8_t)pVCpu->iem.s.abOpcode[offOpcode];
    pVCpu->iem.s.offOpcode = offOpcode + 1;
    return VINF_SUCCESS;
}
#endif /* !IEM_WITH_SETJMP */

/**
 * Fetches the next signed byte from the opcode stream and sign-extending it to
 * a word, returning automatically on failure.
 *
 * @param   a_pu64              Where to return the word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_S8_SX_U64(a_pu64) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextS8SxU64(pVCpu, (a_pu64)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_S8_SX_U64(a_pu64) (*(a_pu64) = (int8_t)iemOpcodeGetNextU8Jmp(pVCpu))
#endif


#ifndef IEM_WITH_SETJMP
/**
 * Fetches the next opcode byte.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 * @param   pu8                 Where to return the opcode byte.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextRm(PVMCPUCC pVCpu, uint8_t *pu8) RT_NOEXCEPT
{
    uintptr_t const offOpcode = pVCpu->iem.s.offOpcode;
    pVCpu->iem.s.offModRm = offOpcode;
    if (RT_LIKELY((uint8_t)offOpcode < pVCpu->iem.s.cbOpcode))
    {
        pVCpu->iem.s.offOpcode = (uint8_t)offOpcode + 1;
        *pu8 = pVCpu->iem.s.abOpcode[offOpcode];
        return VINF_SUCCESS;
    }
    return iemOpcodeGetNextU8Slow(pVCpu, pu8);
}
#else  /* IEM_WITH_SETJMP */
/**
 * Fetches the next opcode byte, longjmp on error.
 *
 * @returns The opcode byte.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECL_INLINE_THROW(uint8_t) iemOpcodeGetNextRmJmp(PVMCPUCC pVCpu) IEM_NOEXCEPT_MAY_LONGJMP
{
# ifdef IEM_WITH_CODE_TLB
    uintptr_t       offBuf = pVCpu->iem.s.offInstrNextByte;
    pVCpu->iem.s.offModRm  = offBuf;
    uint8_t const  *pbBuf  = pVCpu->iem.s.pbInstrBuf;
    if (RT_LIKELY(   pbBuf != NULL
                  && offBuf < pVCpu->iem.s.cbInstrBuf))
    {
        pVCpu->iem.s.offInstrNextByte = (uint32_t)offBuf + 1;
        return pbBuf[offBuf];
    }
# else
    uintptr_t offOpcode   = pVCpu->iem.s.offOpcode;
    pVCpu->iem.s.offModRm = offOpcode;
    if (RT_LIKELY((uint8_t)offOpcode < pVCpu->iem.s.cbOpcode))
    {
        pVCpu->iem.s.offOpcode = (uint8_t)offOpcode + 1;
        return pVCpu->iem.s.abOpcode[offOpcode];
    }
# endif
    return iemOpcodeGetNextU8SlowJmp(pVCpu);
}
#endif /* IEM_WITH_SETJMP */

/**
 * Fetches the next opcode byte, which is a ModR/M byte, returns automatically
 * on failure.
 *
 * Will note down the position of the ModR/M byte for VT-x exits.
 *
 * @param   a_pbRm              Where to return the RM opcode byte.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_RM(a_pbRm) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextRm(pVCpu, (a_pbRm)); \
        if (rcStrict2 == VINF_SUCCESS) \
        { /* likely */ } \
        else \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_RM(a_pbRm) (*(a_pbRm) = iemOpcodeGetNextRmJmp(pVCpu))
#endif /* IEM_WITH_SETJMP */


#ifndef IEM_WITH_SETJMP

/**
 * Fetches the next opcode word.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu16                Where to return the opcode word.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextU16(PVMCPUCC pVCpu, uint16_t *pu16) RT_NOEXCEPT
{
    uintptr_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_LIKELY((uint8_t)offOpcode + 2 <= pVCpu->iem.s.cbOpcode))
    {
        pVCpu->iem.s.offOpcode = (uint8_t)offOpcode + 2;
# ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        *pu16 = *(uint16_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
# else
        *pu16 = RT_MAKE_U16(pVCpu->iem.s.abOpcode[offOpcode], pVCpu->iem.s.abOpcode[offOpcode + 1]);
# endif
        return VINF_SUCCESS;
    }
    return iemOpcodeGetNextU16Slow(pVCpu, pu16);
}

#else  /* IEM_WITH_SETJMP */

/**
 * Fetches the next opcode word, longjmp on error.
 *
 * @returns The opcode word.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECL_INLINE_THROW(uint16_t) iemOpcodeGetNextU16Jmp(PVMCPUCC pVCpu) IEM_NOEXCEPT_MAY_LONGJMP
{
# ifdef IEM_WITH_CODE_TLB
    uintptr_t       offBuf = pVCpu->iem.s.offInstrNextByte;
    uint8_t const  *pbBuf  = pVCpu->iem.s.pbInstrBuf;
    if (RT_LIKELY(   pbBuf != NULL
                  && offBuf + 2 <= pVCpu->iem.s.cbInstrBuf))
    {
        pVCpu->iem.s.offInstrNextByte = (uint32_t)offBuf + 2;
#  ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        return *(uint16_t const *)&pbBuf[offBuf];
#  else
        return RT_MAKE_U16(pbBuf[offBuf], pbBuf[offBuf + 1]);
#  endif
    }
# else /* !IEM_WITH_CODE_TLB */
    uintptr_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_LIKELY((uint8_t)offOpcode + 2 <= pVCpu->iem.s.cbOpcode))
    {
        pVCpu->iem.s.offOpcode = (uint8_t)offOpcode + 2;
#  ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        return *(uint16_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
#  else
        return RT_MAKE_U16(pVCpu->iem.s.abOpcode[offOpcode], pVCpu->iem.s.abOpcode[offOpcode + 1]);
#  endif
    }
# endif /* !IEM_WITH_CODE_TLB */
    return iemOpcodeGetNextU16SlowJmp(pVCpu);
}

#endif /* IEM_WITH_SETJMP */

/**
 * Fetches the next opcode word, returns automatically on failure.
 *
 * @param   a_pu16              Where to return the opcode word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_U16(a_pu16) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextU16(pVCpu, (a_pu16)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_U16(a_pu16) (*(a_pu16) = iemOpcodeGetNextU16Jmp(pVCpu))
#endif

#ifndef IEM_WITH_SETJMP
/**
 * Fetches the next opcode word, zero extending it to a double word.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu32                Where to return the opcode double word.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextU16ZxU32(PVMCPUCC pVCpu, uint32_t *pu32) RT_NOEXCEPT
{
    uint8_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_UNLIKELY(offOpcode + 2 > pVCpu->iem.s.cbOpcode))
        return iemOpcodeGetNextU16ZxU32Slow(pVCpu, pu32);

    *pu32 = RT_MAKE_U16(pVCpu->iem.s.abOpcode[offOpcode], pVCpu->iem.s.abOpcode[offOpcode + 1]);
    pVCpu->iem.s.offOpcode = offOpcode + 2;
    return VINF_SUCCESS;
}
#endif /* !IEM_WITH_SETJMP */

/**
 * Fetches the next opcode word and zero extends it to a double word, returns
 * automatically on failure.
 *
 * @param   a_pu32              Where to return the opcode double word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_U16_ZX_U32(a_pu32) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextU16ZxU32(pVCpu, (a_pu32)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_U16_ZX_U32(a_pu32) (*(a_pu32) = iemOpcodeGetNextU16Jmp(pVCpu))
#endif

#ifndef IEM_WITH_SETJMP
/**
 * Fetches the next opcode word, zero extending it to a quad word.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64                Where to return the opcode quad word.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextU16ZxU64(PVMCPUCC pVCpu, uint64_t *pu64) RT_NOEXCEPT
{
    uint8_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_UNLIKELY(offOpcode + 2 > pVCpu->iem.s.cbOpcode))
        return iemOpcodeGetNextU16ZxU64Slow(pVCpu, pu64);

    *pu64 = RT_MAKE_U16(pVCpu->iem.s.abOpcode[offOpcode], pVCpu->iem.s.abOpcode[offOpcode + 1]);
    pVCpu->iem.s.offOpcode = offOpcode + 2;
    return VINF_SUCCESS;
}
#endif /* !IEM_WITH_SETJMP */

/**
 * Fetches the next opcode word and zero extends it to a quad word, returns
 * automatically on failure.
 *
 * @param   a_pu64              Where to return the opcode quad word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_U16_ZX_U64(a_pu64) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextU16ZxU64(pVCpu, (a_pu64)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_U16_ZX_U64(a_pu64)  (*(a_pu64) = iemOpcodeGetNextU16Jmp(pVCpu))
#endif


#ifndef IEM_WITH_SETJMP
/**
 * Fetches the next signed word from the opcode stream.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pi16                Where to return the signed word.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextS16(PVMCPUCC pVCpu, int16_t *pi16) RT_NOEXCEPT
{
    return iemOpcodeGetNextU16(pVCpu, (uint16_t *)pi16);
}
#endif /* !IEM_WITH_SETJMP */


/**
 * Fetches the next signed word from the opcode stream, returning automatically
 * on failure.
 *
 * @param   a_pi16              Where to return the signed word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_S16(a_pi16) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextS16(pVCpu, (a_pi16)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_S16(a_pi16) (*(a_pi16) = (int16_t)iemOpcodeGetNextU16Jmp(pVCpu))
#endif

#ifndef IEM_WITH_SETJMP

/**
 * Fetches the next opcode dword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu32                Where to return the opcode double word.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextU32(PVMCPUCC pVCpu, uint32_t *pu32) RT_NOEXCEPT
{
    uintptr_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_LIKELY((uint8_t)offOpcode + 4 <= pVCpu->iem.s.cbOpcode))
    {
        pVCpu->iem.s.offOpcode = (uint8_t)offOpcode + 4;
# ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        *pu32 = *(uint32_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
# else
        *pu32 = RT_MAKE_U32_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                    pVCpu->iem.s.abOpcode[offOpcode + 1],
                                    pVCpu->iem.s.abOpcode[offOpcode + 2],
                                    pVCpu->iem.s.abOpcode[offOpcode + 3]);
# endif
        return VINF_SUCCESS;
    }
    return iemOpcodeGetNextU32Slow(pVCpu, pu32);
}

#else  /* IEM_WITH_SETJMP */

/**
 * Fetches the next opcode dword, longjmp on error.
 *
 * @returns The opcode dword.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECL_INLINE_THROW(uint32_t) iemOpcodeGetNextU32Jmp(PVMCPUCC pVCpu) IEM_NOEXCEPT_MAY_LONGJMP
{
# ifdef IEM_WITH_CODE_TLB
    uintptr_t       offBuf = pVCpu->iem.s.offInstrNextByte;
    uint8_t const  *pbBuf  = pVCpu->iem.s.pbInstrBuf;
    if (RT_LIKELY(   pbBuf != NULL
                  && offBuf + 4 <= pVCpu->iem.s.cbInstrBuf))
    {
        pVCpu->iem.s.offInstrNextByte = (uint32_t)offBuf + 4;
#  ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        return *(uint32_t const *)&pbBuf[offBuf];
#  else
        return RT_MAKE_U32_FROM_U8(pbBuf[offBuf],
                                   pbBuf[offBuf + 1],
                                   pbBuf[offBuf + 2],
                                   pbBuf[offBuf + 3]);
#  endif
    }
# else
    uintptr_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_LIKELY((uint8_t)offOpcode + 4 <= pVCpu->iem.s.cbOpcode))
    {
        pVCpu->iem.s.offOpcode = (uint8_t)offOpcode + 4;
#  ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        return *(uint32_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
#  else
        return RT_MAKE_U32_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                   pVCpu->iem.s.abOpcode[offOpcode + 1],
                                   pVCpu->iem.s.abOpcode[offOpcode + 2],
                                   pVCpu->iem.s.abOpcode[offOpcode + 3]);
#  endif
    }
# endif
    return iemOpcodeGetNextU32SlowJmp(pVCpu);
}

#endif /* IEM_WITH_SETJMP */

/**
 * Fetches the next opcode dword, returns automatically on failure.
 *
 * @param   a_pu32              Where to return the opcode dword.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_U32(a_pu32) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextU32(pVCpu, (a_pu32)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_U32(a_pu32) (*(a_pu32) = iemOpcodeGetNextU32Jmp(pVCpu))
#endif

#ifndef IEM_WITH_SETJMP
/**
 * Fetches the next opcode dword, zero extending it to a quad word.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64                Where to return the opcode quad word.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextU32ZxU64(PVMCPUCC pVCpu, uint64_t *pu64) RT_NOEXCEPT
{
    uint8_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_UNLIKELY(offOpcode + 4 > pVCpu->iem.s.cbOpcode))
        return iemOpcodeGetNextU32ZxU64Slow(pVCpu, pu64);

    *pu64 = RT_MAKE_U32_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                pVCpu->iem.s.abOpcode[offOpcode + 1],
                                pVCpu->iem.s.abOpcode[offOpcode + 2],
                                pVCpu->iem.s.abOpcode[offOpcode + 3]);
    pVCpu->iem.s.offOpcode = offOpcode + 4;
    return VINF_SUCCESS;
}
#endif /* !IEM_WITH_SETJMP */

/**
 * Fetches the next opcode dword and zero extends it to a quad word, returns
 * automatically on failure.
 *
 * @param   a_pu64              Where to return the opcode quad word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_U32_ZX_U64(a_pu64) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextU32ZxU64(pVCpu, (a_pu64)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_U32_ZX_U64(a_pu64) (*(a_pu64) = iemOpcodeGetNextU32Jmp(pVCpu))
#endif


#ifndef IEM_WITH_SETJMP
/**
 * Fetches the next signed double word from the opcode stream.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pi32                Where to return the signed double word.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextS32(PVMCPUCC pVCpu, int32_t *pi32) RT_NOEXCEPT
{
    return iemOpcodeGetNextU32(pVCpu, (uint32_t *)pi32);
}
#endif

/**
 * Fetches the next signed double word from the opcode stream, returning
 * automatically on failure.
 *
 * @param   a_pi32              Where to return the signed double word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_S32(a_pi32) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextS32(pVCpu, (a_pi32)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_S32(a_pi32)    (*(a_pi32) = (int32_t)iemOpcodeGetNextU32Jmp(pVCpu))
#endif

#ifndef IEM_WITH_SETJMP
/**
 * Fetches the next opcode dword, sign extending it into a quad word.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64                Where to return the opcode quad word.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextS32SxU64(PVMCPUCC pVCpu, uint64_t *pu64) RT_NOEXCEPT
{
    uint8_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_UNLIKELY(offOpcode + 4 > pVCpu->iem.s.cbOpcode))
        return iemOpcodeGetNextS32SxU64Slow(pVCpu, pu64);

    int32_t i32 = RT_MAKE_U32_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                      pVCpu->iem.s.abOpcode[offOpcode + 1],
                                      pVCpu->iem.s.abOpcode[offOpcode + 2],
                                      pVCpu->iem.s.abOpcode[offOpcode + 3]);
    *pu64 = i32;
    pVCpu->iem.s.offOpcode = offOpcode + 4;
    return VINF_SUCCESS;
}
#endif /* !IEM_WITH_SETJMP */

/**
 * Fetches the next opcode double word and sign extends it to a quad word,
 * returns automatically on failure.
 *
 * @param   a_pu64              Where to return the opcode quad word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_S32_SX_U64(a_pu64) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextS32SxU64(pVCpu, (a_pu64)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_S32_SX_U64(a_pu64) (*(a_pu64) = (int32_t)iemOpcodeGetNextU32Jmp(pVCpu))
#endif

#ifndef IEM_WITH_SETJMP

/**
 * Fetches the next opcode qword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64                Where to return the opcode qword.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextU64(PVMCPUCC pVCpu, uint64_t *pu64) RT_NOEXCEPT
{
    uintptr_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_LIKELY((uint8_t)offOpcode + 8 <= pVCpu->iem.s.cbOpcode))
    {
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
        pVCpu->iem.s.offOpcode = (uint8_t)offOpcode + 8;
        return VINF_SUCCESS;
    }
    return iemOpcodeGetNextU64Slow(pVCpu, pu64);
}

#else  /* IEM_WITH_SETJMP */

/**
 * Fetches the next opcode qword, longjmp on error.
 *
 * @returns The opcode qword.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECL_INLINE_THROW(uint64_t) iemOpcodeGetNextU64Jmp(PVMCPUCC pVCpu) IEM_NOEXCEPT_MAY_LONGJMP
{
# ifdef IEM_WITH_CODE_TLB
    uintptr_t       offBuf = pVCpu->iem.s.offInstrNextByte;
    uint8_t const  *pbBuf  = pVCpu->iem.s.pbInstrBuf;
    if (RT_LIKELY(   pbBuf != NULL
                  && offBuf + 8 <= pVCpu->iem.s.cbInstrBuf))
    {
        pVCpu->iem.s.offInstrNextByte = (uint32_t)offBuf + 8;
#  ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        return *(uint64_t const *)&pbBuf[offBuf];
#  else
        return RT_MAKE_U64_FROM_U8(pbBuf[offBuf],
                                   pbBuf[offBuf + 1],
                                   pbBuf[offBuf + 2],
                                   pbBuf[offBuf + 3],
                                   pbBuf[offBuf + 4],
                                   pbBuf[offBuf + 5],
                                   pbBuf[offBuf + 6],
                                   pbBuf[offBuf + 7]);
#  endif
    }
# else
    uintptr_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_LIKELY((uint8_t)offOpcode + 8 <= pVCpu->iem.s.cbOpcode))
    {
        pVCpu->iem.s.offOpcode = (uint8_t)offOpcode + 8;
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
# endif
    return iemOpcodeGetNextU64SlowJmp(pVCpu);
}

#endif /* IEM_WITH_SETJMP */

/**
 * Fetches the next opcode quad word, returns automatically on failure.
 *
 * @param   a_pu64              Where to return the opcode quad word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_U64(a_pu64) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextU64(pVCpu, (a_pu64)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_U64(a_pu64)    ( *(a_pu64) = iemOpcodeGetNextU64Jmp(pVCpu) )
#endif


/** @name  Misc Worker Functions.
 * @{
 */

/**
 * Gets the correct EFLAGS regardless of whether PATM stores parts of them or
 * not (kind of obsolete now).
 *
 * @param   a_pVCpu The cross context virtual CPU structure of the calling thread.
 */
#define IEMMISC_GET_EFL(a_pVCpu)            ( (a_pVCpu)->cpum.GstCtx.eflags.u  )

/**
 * Updates the EFLAGS in the correct manner wrt. PATM (kind of obsolete).
 *
 * @param   a_pVCpu The cross context virtual CPU structure of the calling thread.
 * @param   a_fEfl  The new EFLAGS.
 */
#define IEMMISC_SET_EFL(a_pVCpu, a_fEfl)    do { (a_pVCpu)->cpum.GstCtx.eflags.u = (a_fEfl); } while (0)


/**
 * Loads a NULL data selector into a selector register, both the hidden and
 * visible parts, in protected mode.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pSReg               Pointer to the segment register.
 * @param   uRpl                The RPL.
 */
DECLINLINE(void) iemHlpLoadNullDataSelectorProt(PVMCPUCC pVCpu, PCPUMSELREG pSReg, RTSEL uRpl) RT_NOEXCEPT
{
    /** @todo Testcase: write a testcase checking what happends when loading a NULL
     *        data selector in protected mode. */
    pSReg->Sel      = uRpl;
    pSReg->ValidSel = uRpl;
    pSReg->fFlags   = CPUMSELREG_FLAGS_VALID;
    if (IEM_IS_GUEST_CPU_INTEL(pVCpu))
    {
        /* VT-x (Intel 3960x) observed doing something like this. */
        pSReg->Attr.u   = X86DESCATTR_UNUSABLE | X86DESCATTR_G | X86DESCATTR_D | (pVCpu->iem.s.uCpl << X86DESCATTR_DPL_SHIFT);
        pSReg->u32Limit = UINT32_MAX;
        pSReg->u64Base  = 0;
    }
    else
    {
        pSReg->Attr.u   = X86DESCATTR_UNUSABLE;
        pSReg->u32Limit = 0;
        pSReg->u64Base  = 0;
    }
}

/** @} */


/*
 *
 * Helpers routines.
 * Helpers routines.
 * Helpers routines.
 *
 */

/**
 * Recalculates the effective operand size.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemRecalEffOpSize(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    switch (pVCpu->iem.s.enmCpuMode)
    {
        case IEMMODE_16BIT:
            pVCpu->iem.s.enmEffOpSize = pVCpu->iem.s.fPrefixes & IEM_OP_PRF_SIZE_OP ? IEMMODE_32BIT : IEMMODE_16BIT;
            break;
        case IEMMODE_32BIT:
            pVCpu->iem.s.enmEffOpSize = pVCpu->iem.s.fPrefixes & IEM_OP_PRF_SIZE_OP ? IEMMODE_16BIT : IEMMODE_32BIT;
            break;
        case IEMMODE_64BIT:
            switch (pVCpu->iem.s.fPrefixes & (IEM_OP_PRF_SIZE_REX_W | IEM_OP_PRF_SIZE_OP))
            {
                case 0:
                    pVCpu->iem.s.enmEffOpSize = pVCpu->iem.s.enmDefOpSize;
                    break;
                case IEM_OP_PRF_SIZE_OP:
                    pVCpu->iem.s.enmEffOpSize = IEMMODE_16BIT;
                    break;
                case IEM_OP_PRF_SIZE_REX_W:
                case IEM_OP_PRF_SIZE_REX_W | IEM_OP_PRF_SIZE_OP:
                    pVCpu->iem.s.enmEffOpSize = IEMMODE_64BIT;
                    break;
            }
            break;
        default:
            AssertFailed();
    }
}


/**
 * Sets the default operand size to 64-bit and recalculates the effective
 * operand size.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemRecalEffOpSize64Default(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    Assert(pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT);
    pVCpu->iem.s.enmDefOpSize = IEMMODE_64BIT;
    if ((pVCpu->iem.s.fPrefixes & (IEM_OP_PRF_SIZE_REX_W | IEM_OP_PRF_SIZE_OP)) != IEM_OP_PRF_SIZE_OP)
        pVCpu->iem.s.enmEffOpSize = IEMMODE_64BIT;
    else
        pVCpu->iem.s.enmEffOpSize = IEMMODE_16BIT;
}


/**
 * Sets the default operand size to 64-bit and recalculates the effective
 * operand size, with intel ignoring any operand size prefix (AMD respects it).
 *
 * This is for the relative jumps.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemRecalEffOpSize64DefaultAndIntelIgnoresOpSizePrefix(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    Assert(pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT);
    pVCpu->iem.s.enmDefOpSize = IEMMODE_64BIT;
    if (   (pVCpu->iem.s.fPrefixes & (IEM_OP_PRF_SIZE_REX_W | IEM_OP_PRF_SIZE_OP)) != IEM_OP_PRF_SIZE_OP
        || pVCpu->iem.s.enmCpuVendor == CPUMCPUVENDOR_INTEL)
        pVCpu->iem.s.enmEffOpSize = IEMMODE_64BIT;
    else
        pVCpu->iem.s.enmEffOpSize = IEMMODE_16BIT;
}




/** @name   Register Access.
 * @{
 */

/**
 * Gets a reference (pointer) to the specified hidden segment register.
 *
 * @returns Hidden register reference.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The segment register.
 */
DECLINLINE(PCPUMSELREG) iemSRegGetHid(PVMCPUCC pVCpu, uint8_t iSegReg) RT_NOEXCEPT
{
    Assert(iSegReg < X86_SREG_COUNT);
    IEM_CTX_ASSERT(pVCpu, CPUMCTX_EXTRN_SREG_FROM_IDX(iSegReg));
    PCPUMSELREG pSReg = &pVCpu->cpum.GstCtx.aSRegs[iSegReg];

    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSReg));
    return pSReg;
}


/**
 * Ensures that the given hidden segment register is up to date.
 *
 * @returns Hidden register reference.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pSReg               The segment register.
 */
DECLINLINE(PCPUMSELREG) iemSRegUpdateHid(PVMCPUCC pVCpu, PCPUMSELREG pSReg) RT_NOEXCEPT
{
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSReg));
    NOREF(pVCpu);
    return pSReg;
}


/**
 * Gets a reference (pointer) to the specified segment register (the selector
 * value).
 *
 * @returns Pointer to the selector variable.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The segment register.
 */
DECLINLINE(uint16_t *) iemSRegRef(PVMCPUCC pVCpu, uint8_t iSegReg) RT_NOEXCEPT
{
    Assert(iSegReg < X86_SREG_COUNT);
    IEM_CTX_ASSERT(pVCpu, CPUMCTX_EXTRN_SREG_FROM_IDX(iSegReg));
    return &pVCpu->cpum.GstCtx.aSRegs[iSegReg].Sel;
}


/**
 * Fetches the selector value of a segment register.
 *
 * @returns The selector value.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The segment register.
 */
DECLINLINE(uint16_t) iemSRegFetchU16(PVMCPUCC pVCpu, uint8_t iSegReg) RT_NOEXCEPT
{
    Assert(iSegReg < X86_SREG_COUNT);
    IEM_CTX_ASSERT(pVCpu, CPUMCTX_EXTRN_SREG_FROM_IDX(iSegReg));
    return pVCpu->cpum.GstCtx.aSRegs[iSegReg].Sel;
}


/**
 * Fetches the base address value of a segment register.
 *
 * @returns The selector value.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The segment register.
 */
DECLINLINE(uint64_t) iemSRegBaseFetchU64(PVMCPUCC pVCpu, uint8_t iSegReg) RT_NOEXCEPT
{
    Assert(iSegReg < X86_SREG_COUNT);
    IEM_CTX_ASSERT(pVCpu, CPUMCTX_EXTRN_SREG_FROM_IDX(iSegReg));
    return pVCpu->cpum.GstCtx.aSRegs[iSegReg].u64Base;
}


/**
 * Gets a reference (pointer) to the specified general purpose register.
 *
 * @returns Register reference.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iReg                The general purpose register.
 */
DECLINLINE(void *) iemGRegRef(PVMCPUCC pVCpu, uint8_t iReg) RT_NOEXCEPT
{
    Assert(iReg < 16);
    return &pVCpu->cpum.GstCtx.aGRegs[iReg];
}


/**
 * Gets a reference (pointer) to the specified 8-bit general purpose register.
 *
 * Because of AH, CH, DH and BH we cannot use iemGRegRef directly here.
 *
 * @returns Register reference.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iReg                The register.
 */
DECLINLINE(uint8_t *) iemGRegRefU8(PVMCPUCC pVCpu, uint8_t iReg) RT_NOEXCEPT
{
    if (iReg < 4 || (pVCpu->iem.s.fPrefixes & IEM_OP_PRF_REX))
    {
        Assert(iReg < 16);
        return &pVCpu->cpum.GstCtx.aGRegs[iReg].u8;
    }
    /* high 8-bit register. */
    Assert(iReg < 8);
    return &pVCpu->cpum.GstCtx.aGRegs[iReg & 3].bHi;
}


/**
 * Gets a reference (pointer) to the specified 16-bit general purpose register.
 *
 * @returns Register reference.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iReg                The register.
 */
DECLINLINE(uint16_t *) iemGRegRefU16(PVMCPUCC pVCpu, uint8_t iReg) RT_NOEXCEPT
{
    Assert(iReg < 16);
    return &pVCpu->cpum.GstCtx.aGRegs[iReg].u16;
}


/**
 * Gets a reference (pointer) to the specified 32-bit general purpose register.
 *
 * @returns Register reference.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iReg                The register.
 */
DECLINLINE(uint32_t *) iemGRegRefU32(PVMCPUCC pVCpu, uint8_t iReg) RT_NOEXCEPT
{
    Assert(iReg < 16);
    return &pVCpu->cpum.GstCtx.aGRegs[iReg].u32;
}


/**
 * Gets a reference (pointer) to the specified signed 32-bit general purpose register.
 *
 * @returns Register reference.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iReg                The register.
 */
DECLINLINE(int32_t *) iemGRegRefI32(PVMCPUCC pVCpu, uint8_t iReg) RT_NOEXCEPT
{
    Assert(iReg < 16);
    return (int32_t *)&pVCpu->cpum.GstCtx.aGRegs[iReg].u32;
}


/**
 * Gets a reference (pointer) to the specified 64-bit general purpose register.
 *
 * @returns Register reference.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iReg                The register.
 */
DECLINLINE(uint64_t *) iemGRegRefU64(PVMCPUCC pVCpu, uint8_t iReg) RT_NOEXCEPT
{
    Assert(iReg < 64);
    return &pVCpu->cpum.GstCtx.aGRegs[iReg].u64;
}


/**
 * Gets a reference (pointer) to the specified signed 64-bit general purpose register.
 *
 * @returns Register reference.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iReg                The register.
 */
DECLINLINE(int64_t *) iemGRegRefI64(PVMCPUCC pVCpu, uint8_t iReg) RT_NOEXCEPT
{
    Assert(iReg < 16);
    return (int64_t *)&pVCpu->cpum.GstCtx.aGRegs[iReg].u64;
}


/**
 * Gets a reference (pointer) to the specified segment register's base address.
 *
 * @returns Segment register base address reference.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The segment selector.
 */
DECLINLINE(uint64_t *) iemSRegBaseRefU64(PVMCPUCC pVCpu, uint8_t iSegReg) RT_NOEXCEPT
{
    Assert(iSegReg < X86_SREG_COUNT);
    IEM_CTX_ASSERT(pVCpu, CPUMCTX_EXTRN_SREG_FROM_IDX(iSegReg));
    return &pVCpu->cpum.GstCtx.aSRegs[iSegReg].u64Base;
}


/**
 * Fetches the value of a 8-bit general purpose register.
 *
 * @returns The register value.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iReg                The register.
 */
DECLINLINE(uint8_t) iemGRegFetchU8(PVMCPUCC pVCpu, uint8_t iReg) RT_NOEXCEPT
{
    return *iemGRegRefU8(pVCpu, iReg);
}


/**
 * Fetches the value of a 16-bit general purpose register.
 *
 * @returns The register value.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iReg                The register.
 */
DECLINLINE(uint16_t) iemGRegFetchU16(PVMCPUCC pVCpu, uint8_t iReg) RT_NOEXCEPT
{
    Assert(iReg < 16);
    return pVCpu->cpum.GstCtx.aGRegs[iReg].u16;
}


/**
 * Fetches the value of a 32-bit general purpose register.
 *
 * @returns The register value.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iReg                The register.
 */
DECLINLINE(uint32_t) iemGRegFetchU32(PVMCPUCC pVCpu, uint8_t iReg) RT_NOEXCEPT
{
    Assert(iReg < 16);
    return pVCpu->cpum.GstCtx.aGRegs[iReg].u32;
}


/**
 * Fetches the value of a 64-bit general purpose register.
 *
 * @returns The register value.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iReg                The register.
 */
DECLINLINE(uint64_t) iemGRegFetchU64(PVMCPUCC pVCpu, uint8_t iReg) RT_NOEXCEPT
{
    Assert(iReg < 16);
    return pVCpu->cpum.GstCtx.aGRegs[iReg].u64;
}


/**
 * Get the address of the top of the stack.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(RTGCPTR) iemRegGetEffRsp(PCVMCPU pVCpu) RT_NOEXCEPT
{
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        return pVCpu->cpum.GstCtx.rsp;
    if (pVCpu->cpum.GstCtx.ss.Attr.n.u1DefBig)
        return pVCpu->cpum.GstCtx.esp;
    return pVCpu->cpum.GstCtx.sp;
}


/**
 * Updates the RIP/EIP/IP to point to the next instruction.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   cbInstr             The number of bytes to add.
 */
DECL_FORCE_INLINE(void) iemRegAddToRip(PVMCPUCC pVCpu, uint8_t cbInstr) RT_NOEXCEPT
{
    /*
     * Advance RIP.
     *
     * When we're targetting 8086/8, 80186/8 or 80286 mode the updates are 16-bit,
     * while in all other modes except LM64 the updates are 32-bit.  This means
     * we need to watch for both 32-bit and 16-bit "carry" situations, i.e.
     * 4GB and 64KB rollovers, and decide whether anything needs masking.
     *
     * See PC wrap around tests in bs3-cpu-weird-1.
     */
    uint64_t const uRipPrev = pVCpu->cpum.GstCtx.rip;
    uint64_t const uRipNext = uRipPrev + cbInstr;
    if (RT_LIKELY(   !((uRipNext ^ uRipPrev) & (RT_BIT_64(32) | RT_BIT_64(16)))
                  || pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT))
        pVCpu->cpum.GstCtx.rip = uRipNext;
    else if (IEM_GET_TARGET_CPU(pVCpu) >= IEMTARGETCPU_386)
        pVCpu->cpum.GstCtx.rip = (uint32_t)uRipNext;
    else
        pVCpu->cpum.GstCtx.rip = (uint16_t)uRipNext;
}


/**
 * Called by iemRegAddToRipAndFinishingClearingRF and others when any of the
 * following EFLAGS bits are set:
 *      - X86_EFL_RF - clear it.
 *      - CPUMCTX_INHIBIT_SHADOW (_SS/_STI) - clear them.
 *      - X86_EFL_TF - generate single step \#DB trap.
 *      - CPUMCTX_DBG_HIT_DR0/1/2/3 - generate \#DB trap (data or I/O, not
 *        instruction).
 *
 * According to @sdmv3{077,200,Table 6-2,Priority Among Concurrent Events},
 * a \#DB due to TF (single stepping) or a DRx non-instruction breakpoint
 * takes priority over both NMIs and hardware interrupts.  So, neither is
 * considered here.  (The RESET, \#MC, SMI, INIT, STOPCLK and FLUSH events are
 * either unsupported will be triggered on-top of any \#DB raised here.)
 *
 * The RF flag only needs to be cleared here as it only suppresses instruction
 * breakpoints which are not raised here (happens synchronously during
 * instruction fetching).
 *
 * The CPUMCTX_INHIBIT_SHADOW_SS flag will be cleared by this function, so its
 * status has no bearing on whether \#DB exceptions are raised.
 *
 * @note This must *NOT* be called by the two instructions setting the
 *       CPUMCTX_INHIBIT_SHADOW_SS flag.
 *
 * @see  @sdmv3{077,200,Table 6-2,Priority Among Concurrent Events}
 * @see  @sdmv3{077,200,6.8.3,Masking Exceptions and Interrupts When Switching
 *              Stacks}
 */
static VBOXSTRICTRC iemFinishInstructionWithFlagsSet(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    /*
     * Normally we're just here to clear RF and/or interrupt shadow bits.
     */
    if (RT_LIKELY((pVCpu->cpum.GstCtx.eflags.uBoth & (X86_EFL_TF | CPUMCTX_DBG_HIT_DRX_MASK | CPUMCTX_DBG_DBGF_MASK)) == 0))
        pVCpu->cpum.GstCtx.eflags.uBoth &= ~(X86_EFL_RF | CPUMCTX_INHIBIT_SHADOW);
    else
    {
        /*
         * Raise a #DB or/and DBGF event.
         */
        VBOXSTRICTRC rcStrict;
        if (pVCpu->cpum.GstCtx.eflags.uBoth & (X86_EFL_TF | CPUMCTX_DBG_HIT_DRX_MASK))
        {
            IEM_CTX_IMPORT_RET(pVCpu, CPUMCTX_EXTRN_DR6);
            pVCpu->cpum.GstCtx.dr[6] &= ~X86_DR6_B_MASK;
            if (pVCpu->cpum.GstCtx.eflags.uBoth & X86_EFL_TF)
                pVCpu->cpum.GstCtx.dr[6] |= X86_DR6_BS;
            pVCpu->cpum.GstCtx.dr[6] |= (pVCpu->cpum.GstCtx.eflags.uBoth & CPUMCTX_DBG_HIT_DRX_MASK) >> CPUMCTX_DBG_HIT_DRX_SHIFT;
            LogFlowFunc(("Guest #DB fired at %04X:%016llX: DR6=%08X, RFLAGS=%16RX64\n",
                         pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, (unsigned)pVCpu->cpum.GstCtx.dr[6],
                         pVCpu->cpum.GstCtx.rflags.uBoth));

            pVCpu->cpum.GstCtx.eflags.uBoth &= ~(X86_EFL_RF | CPUMCTX_INHIBIT_SHADOW | CPUMCTX_DBG_HIT_DRX_MASK);
            rcStrict = iemRaiseDebugException(pVCpu);

            /* A DBGF event/breakpoint trumps the iemRaiseDebugException informational status code. */
            if ((pVCpu->cpum.GstCtx.eflags.uBoth & CPUMCTX_DBG_DBGF_MASK) && RT_FAILURE(rcStrict))
            {
                rcStrict = pVCpu->cpum.GstCtx.eflags.uBoth & CPUMCTX_DBG_DBGF_BP ? VINF_EM_DBG_BREAKPOINT : VINF_EM_DBG_EVENT;
                LogFlowFunc(("dbgf at %04X:%016llX: %Rrc\n", pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, VBOXSTRICTRC_VAL(rcStrict)));
            }
        }
        else
        {
            Assert(pVCpu->cpum.GstCtx.eflags.uBoth & CPUMCTX_DBG_DBGF_MASK);
            rcStrict = pVCpu->cpum.GstCtx.eflags.uBoth & CPUMCTX_DBG_DBGF_BP ? VINF_EM_DBG_BREAKPOINT : VINF_EM_DBG_EVENT;
            LogFlowFunc(("dbgf at %04X:%016llX: %Rrc\n", pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, VBOXSTRICTRC_VAL(rcStrict)));
        }
        pVCpu->cpum.GstCtx.eflags.uBoth &= ~CPUMCTX_DBG_DBGF_MASK;
        return rcStrict;
    }
    return VINF_SUCCESS;
}


/**
 * Clears the RF and CPUMCTX_INHIBIT_SHADOW, triggering \#DB if pending.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECL_FORCE_INLINE(VBOXSTRICTRC) iemRegFinishClearingRF(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    /*
     * We assume that most of the time nothing actually needs doing here.
     */
    AssertCompile(CPUMCTX_INHIBIT_SHADOW < UINT32_MAX);
    if (RT_LIKELY(!(  pVCpu->cpum.GstCtx.eflags.uBoth
                    & (X86_EFL_TF | X86_EFL_RF | CPUMCTX_INHIBIT_SHADOW | CPUMCTX_DBG_HIT_DRX_MASK | CPUMCTX_DBG_DBGF_MASK)) ))
        return VINF_SUCCESS;
    return iemFinishInstructionWithFlagsSet(pVCpu);
}


/**
 * Updates the RIP/EIP/IP to point to the next instruction and clears EFLAGS.RF
 * and CPUMCTX_INHIBIT_SHADOW.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   cbInstr             The number of bytes to add.
 */
DECLINLINE(VBOXSTRICTRC) iemRegAddToRipAndFinishingClearingRF(PVMCPUCC pVCpu, uint8_t cbInstr) RT_NOEXCEPT
{
    iemRegAddToRip(pVCpu, cbInstr);
    return iemRegFinishClearingRF(pVCpu);
}


/**
 * Extended version of iemFinishInstructionWithFlagsSet that goes with
 * iemRegAddToRipAndFinishingClearingRfEx.
 *
 * See iemFinishInstructionWithFlagsSet() for details.
 */
static VBOXSTRICTRC iemFinishInstructionWithTfSet(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    /*
     * Raise a #DB.
     */
    IEM_CTX_IMPORT_RET(pVCpu, CPUMCTX_EXTRN_DR6);
    pVCpu->cpum.GstCtx.dr[6] &= ~X86_DR6_B_MASK;
    pVCpu->cpum.GstCtx.dr[6] |= X86_DR6_BS
                             |  (pVCpu->cpum.GstCtx.eflags.uBoth & CPUMCTX_DBG_HIT_DRX_MASK) >> CPUMCTX_DBG_HIT_DRX_SHIFT;
    /** @todo Do we set all pending \#DB events, or just one? */
    LogFlowFunc(("Guest #DB fired at %04X:%016llX: DR6=%08X, RFLAGS=%16RX64 (popf)\n",
                 pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, (unsigned)pVCpu->cpum.GstCtx.dr[6],
                 pVCpu->cpum.GstCtx.rflags.uBoth));
    pVCpu->cpum.GstCtx.eflags.uBoth &= ~(X86_EFL_RF | CPUMCTX_INHIBIT_SHADOW | CPUMCTX_DBG_HIT_DRX_MASK | CPUMCTX_DBG_DBGF_MASK);
    return iemRaiseDebugException(pVCpu);
}


/**
 * Extended version of iemRegAddToRipAndFinishingClearingRF for use by POPF and
 * others potentially updating EFLAGS.TF.
 *
 * The single step event must be generated using the TF value at the start of
 * the instruction, not the new value set by it.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   cbInstr             The number of bytes to add.
 * @param   fEflOld             The EFLAGS at the start of the instruction
 *                              execution.
 */
DECLINLINE(VBOXSTRICTRC) iemRegAddToRipAndFinishingClearingRfEx(PVMCPUCC pVCpu, uint8_t cbInstr, uint32_t fEflOld) RT_NOEXCEPT
{
    iemRegAddToRip(pVCpu, cbInstr);
    if (!(fEflOld & X86_EFL_TF))
        return iemRegFinishClearingRF(pVCpu);
    return iemFinishInstructionWithTfSet(pVCpu);
}


/**
 * Updates the RIP/EIP/IP to point to the next instruction and clears EFLAGS.RF.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(VBOXSTRICTRC) iemRegUpdateRipAndFinishClearingRF(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    return iemRegAddToRipAndFinishingClearingRF(pVCpu, IEM_GET_INSTR_LEN(pVCpu));
}


/**
 * Adds to the stack pointer.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   cbToAdd             The number of bytes to add (8-bit!).
 */
DECLINLINE(void) iemRegAddToRsp(PVMCPUCC pVCpu, uint8_t cbToAdd) RT_NOEXCEPT
{
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        pVCpu->cpum.GstCtx.rsp += cbToAdd;
    else if (pVCpu->cpum.GstCtx.ss.Attr.n.u1DefBig)
        pVCpu->cpum.GstCtx.esp += cbToAdd;
    else
        pVCpu->cpum.GstCtx.sp  += cbToAdd;
}


/**
 * Subtracts from the stack pointer.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   cbToSub             The number of bytes to subtract (8-bit!).
 */
DECLINLINE(void) iemRegSubFromRsp(PVMCPUCC pVCpu, uint8_t cbToSub) RT_NOEXCEPT
{
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        pVCpu->cpum.GstCtx.rsp -= cbToSub;
    else if (pVCpu->cpum.GstCtx.ss.Attr.n.u1DefBig)
        pVCpu->cpum.GstCtx.esp -= cbToSub;
    else
        pVCpu->cpum.GstCtx.sp  -= cbToSub;
}


/**
 * Adds to the temporary stack pointer.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pTmpRsp             The temporary SP/ESP/RSP to update.
 * @param   cbToAdd             The number of bytes to add (16-bit).
 */
DECLINLINE(void) iemRegAddToRspEx(PCVMCPU pVCpu, PRTUINT64U pTmpRsp, uint16_t cbToAdd) RT_NOEXCEPT
{
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        pTmpRsp->u           += cbToAdd;
    else if (pVCpu->cpum.GstCtx.ss.Attr.n.u1DefBig)
        pTmpRsp->DWords.dw0  += cbToAdd;
    else
        pTmpRsp->Words.w0    += cbToAdd;
}


/**
 * Subtracts from the temporary stack pointer.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pTmpRsp             The temporary SP/ESP/RSP to update.
 * @param   cbToSub             The number of bytes to subtract.
 * @remarks The @a cbToSub argument *MUST* be 16-bit, iemCImpl_enter is
 *          expecting that.
 */
DECLINLINE(void) iemRegSubFromRspEx(PCVMCPU pVCpu, PRTUINT64U pTmpRsp, uint16_t cbToSub) RT_NOEXCEPT
{
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        pTmpRsp->u          -= cbToSub;
    else if (pVCpu->cpum.GstCtx.ss.Attr.n.u1DefBig)
        pTmpRsp->DWords.dw0 -= cbToSub;
    else
        pTmpRsp->Words.w0   -= cbToSub;
}


/**
 * Calculates the effective stack address for a push of the specified size as
 * well as the new RSP value (upper bits may be masked).
 *
 * @returns Effective stack addressf for the push.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   cbItem              The size of the stack item to pop.
 * @param   puNewRsp            Where to return the new RSP value.
 */
DECLINLINE(RTGCPTR) iemRegGetRspForPush(PCVMCPU pVCpu, uint8_t cbItem, uint64_t *puNewRsp) RT_NOEXCEPT
{
    RTUINT64U   uTmpRsp;
    RTGCPTR     GCPtrTop;
    uTmpRsp.u = pVCpu->cpum.GstCtx.rsp;

    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        GCPtrTop = uTmpRsp.u            -= cbItem;
    else if (pVCpu->cpum.GstCtx.ss.Attr.n.u1DefBig)
        GCPtrTop = uTmpRsp.DWords.dw0   -= cbItem;
    else
        GCPtrTop = uTmpRsp.Words.w0     -= cbItem;
    *puNewRsp = uTmpRsp.u;
    return GCPtrTop;
}


/**
 * Gets the current stack pointer and calculates the value after a pop of the
 * specified size.
 *
 * @returns Current stack pointer.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   cbItem              The size of the stack item to pop.
 * @param   puNewRsp            Where to return the new RSP value.
 */
DECLINLINE(RTGCPTR) iemRegGetRspForPop(PCVMCPU pVCpu, uint8_t cbItem, uint64_t *puNewRsp) RT_NOEXCEPT
{
    RTUINT64U   uTmpRsp;
    RTGCPTR     GCPtrTop;
    uTmpRsp.u = pVCpu->cpum.GstCtx.rsp;

    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        GCPtrTop = uTmpRsp.u;
        uTmpRsp.u += cbItem;
    }
    else if (pVCpu->cpum.GstCtx.ss.Attr.n.u1DefBig)
    {
        GCPtrTop = uTmpRsp.DWords.dw0;
        uTmpRsp.DWords.dw0 += cbItem;
    }
    else
    {
        GCPtrTop = uTmpRsp.Words.w0;
        uTmpRsp.Words.w0 += cbItem;
    }
    *puNewRsp = uTmpRsp.u;
    return GCPtrTop;
}


/**
 * Calculates the effective stack address for a push of the specified size as
 * well as the new temporary RSP value (upper bits may be masked).
 *
 * @returns Effective stack addressf for the push.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pTmpRsp             The temporary stack pointer.  This is updated.
 * @param   cbItem              The size of the stack item to pop.
 */
DECLINLINE(RTGCPTR) iemRegGetRspForPushEx(PCVMCPU pVCpu, PRTUINT64U pTmpRsp, uint8_t cbItem) RT_NOEXCEPT
{
    RTGCPTR GCPtrTop;

    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        GCPtrTop = pTmpRsp->u          -= cbItem;
    else if (pVCpu->cpum.GstCtx.ss.Attr.n.u1DefBig)
        GCPtrTop = pTmpRsp->DWords.dw0 -= cbItem;
    else
        GCPtrTop = pTmpRsp->Words.w0   -= cbItem;
    return GCPtrTop;
}


/**
 * Gets the effective stack address for a pop of the specified size and
 * calculates and updates the temporary RSP.
 *
 * @returns Current stack pointer.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pTmpRsp             The temporary stack pointer.  This is updated.
 * @param   cbItem              The size of the stack item to pop.
 */
DECLINLINE(RTGCPTR) iemRegGetRspForPopEx(PCVMCPU pVCpu, PRTUINT64U pTmpRsp, uint8_t cbItem) RT_NOEXCEPT
{
    RTGCPTR GCPtrTop;
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        GCPtrTop = pTmpRsp->u;
        pTmpRsp->u          += cbItem;
    }
    else if (pVCpu->cpum.GstCtx.ss.Attr.n.u1DefBig)
    {
        GCPtrTop = pTmpRsp->DWords.dw0;
        pTmpRsp->DWords.dw0 += cbItem;
    }
    else
    {
        GCPtrTop = pTmpRsp->Words.w0;
        pTmpRsp->Words.w0   += cbItem;
    }
    return GCPtrTop;
}

/** @}  */


/** @name   FPU access and helpers.
 *
 * @{
 */


/**
 * Hook for preparing to use the host FPU.
 *
 * This is necessary in ring-0 and raw-mode context (nop in ring-3).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemFpuPrepareUsage(PVMCPUCC pVCpu) RT_NOEXCEPT
{
#ifdef IN_RING3
    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_FPU_REM);
#else
    CPUMRZFpuStatePrepareHostCpuForUse(pVCpu);
#endif
    IEM_CTX_IMPORT_NORET(pVCpu, CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX | CPUMCTX_EXTRN_OTHER_XSAVE | CPUMCTX_EXTRN_XCRx);
}


/**
 * Hook for preparing to use the host FPU for SSE.
 *
 * This is necessary in ring-0 and raw-mode context (nop in ring-3).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemFpuPrepareUsageSse(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    iemFpuPrepareUsage(pVCpu);
}


/**
 * Hook for preparing to use the host FPU for AVX.
 *
 * This is necessary in ring-0 and raw-mode context (nop in ring-3).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemFpuPrepareUsageAvx(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    iemFpuPrepareUsage(pVCpu);
}


/**
 * Hook for actualizing the guest FPU state before the interpreter reads it.
 *
 * This is necessary in ring-0 and raw-mode context (nop in ring-3).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemFpuActualizeStateForRead(PVMCPUCC pVCpu) RT_NOEXCEPT
{
#ifdef IN_RING3
    NOREF(pVCpu);
#else
    CPUMRZFpuStateActualizeForRead(pVCpu);
#endif
    IEM_CTX_IMPORT_NORET(pVCpu, CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX | CPUMCTX_EXTRN_OTHER_XSAVE | CPUMCTX_EXTRN_XCRx);
}


/**
 * Hook for actualizing the guest FPU state before the interpreter changes it.
 *
 * This is necessary in ring-0 and raw-mode context (nop in ring-3).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemFpuActualizeStateForChange(PVMCPUCC pVCpu) RT_NOEXCEPT
{
#ifdef IN_RING3
    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_FPU_REM);
#else
    CPUMRZFpuStateActualizeForChange(pVCpu);
#endif
    IEM_CTX_IMPORT_NORET(pVCpu, CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX | CPUMCTX_EXTRN_OTHER_XSAVE | CPUMCTX_EXTRN_XCRx);
}


/**
 * Hook for actualizing the guest XMM0..15 and MXCSR register state for read
 * only.
 *
 * This is necessary in ring-0 and raw-mode context (nop in ring-3).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemFpuActualizeSseStateForRead(PVMCPUCC pVCpu) RT_NOEXCEPT
{
#if defined(IN_RING3) || defined(VBOX_WITH_KERNEL_USING_XMM)
    NOREF(pVCpu);
#else
    CPUMRZFpuStateActualizeSseForRead(pVCpu);
#endif
    IEM_CTX_IMPORT_NORET(pVCpu, CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX | CPUMCTX_EXTRN_OTHER_XSAVE | CPUMCTX_EXTRN_XCRx);
}


/**
 * Hook for actualizing the guest XMM0..15 and MXCSR register state for
 * read+write.
 *
 * This is necessary in ring-0 and raw-mode context (nop in ring-3).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemFpuActualizeSseStateForChange(PVMCPUCC pVCpu) RT_NOEXCEPT
{
#if defined(IN_RING3) || defined(VBOX_WITH_KERNEL_USING_XMM)
    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_FPU_REM);
#else
    CPUMRZFpuStateActualizeForChange(pVCpu);
#endif
    IEM_CTX_IMPORT_NORET(pVCpu, CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX | CPUMCTX_EXTRN_OTHER_XSAVE | CPUMCTX_EXTRN_XCRx);

    /* Make sure any changes are loaded the next time around. */
    pVCpu->cpum.GstCtx.XState.Hdr.bmXState |= XSAVE_C_SSE;
}


/**
 * Hook for actualizing the guest YMM0..15 and MXCSR register state for read
 * only.
 *
 * This is necessary in ring-0 and raw-mode context (nop in ring-3).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemFpuActualizeAvxStateForRead(PVMCPUCC pVCpu) RT_NOEXCEPT
{
#ifdef IN_RING3
    NOREF(pVCpu);
#else
    CPUMRZFpuStateActualizeAvxForRead(pVCpu);
#endif
    IEM_CTX_IMPORT_NORET(pVCpu, CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX | CPUMCTX_EXTRN_OTHER_XSAVE | CPUMCTX_EXTRN_XCRx);
}


/**
 * Hook for actualizing the guest YMM0..15 and MXCSR register state for
 * read+write.
 *
 * This is necessary in ring-0 and raw-mode context (nop in ring-3).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemFpuActualizeAvxStateForChange(PVMCPUCC pVCpu) RT_NOEXCEPT
{
#ifdef IN_RING3
    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_FPU_REM);
#else
    CPUMRZFpuStateActualizeForChange(pVCpu);
#endif
    IEM_CTX_IMPORT_NORET(pVCpu, CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX | CPUMCTX_EXTRN_OTHER_XSAVE | CPUMCTX_EXTRN_XCRx);

    /* Just assume we're going to make changes to the SSE and YMM_HI parts. */
    pVCpu->cpum.GstCtx.XState.Hdr.bmXState |= XSAVE_C_YMM | XSAVE_C_SSE;
}


/**
 * Stores a QNaN value into a FPU register.
 *
 * @param   pReg                Pointer to the register.
 */
DECLINLINE(void) iemFpuStoreQNan(PRTFLOAT80U pReg) RT_NOEXCEPT
{
    pReg->au32[0] = UINT32_C(0x00000000);
    pReg->au32[1] = UINT32_C(0xc0000000);
    pReg->au16[4] = UINT16_C(0xffff);
}


/**
 * Updates the FOP, FPU.CS and FPUIP registers.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pFpuCtx             The FPU context.
 */
DECLINLINE(void) iemFpuUpdateOpcodeAndIpWorker(PVMCPUCC pVCpu, PX86FXSTATE pFpuCtx) RT_NOEXCEPT
{
    Assert(pVCpu->iem.s.uFpuOpcode != UINT16_MAX);
    pFpuCtx->FOP = pVCpu->iem.s.uFpuOpcode;
    /** @todo x87.CS and FPUIP needs to be kept seperately. */
    if (IEM_IS_REAL_OR_V86_MODE(pVCpu))
    {
        /** @todo Testcase: making assumptions about how FPUIP and FPUDP are handled
         *        happens in real mode here based on the fnsave and fnstenv images. */
        pFpuCtx->CS    = 0;
        pFpuCtx->FPUIP = pVCpu->cpum.GstCtx.eip | ((uint32_t)pVCpu->cpum.GstCtx.cs.Sel << 4);
    }
    else if (!IEM_IS_LONG_MODE(pVCpu))
    {
        pFpuCtx->CS    = pVCpu->cpum.GstCtx.cs.Sel;
        pFpuCtx->FPUIP = pVCpu->cpum.GstCtx.rip;
    }
    else
        *(uint64_t *)&pFpuCtx->FPUIP = pVCpu->cpum.GstCtx.rip;
}





/**
 * Marks the specified stack register as free (for FFREE).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iStReg              The register to free.
 */
DECLINLINE(void) iemFpuStackFree(PVMCPUCC pVCpu, uint8_t iStReg) RT_NOEXCEPT
{
    Assert(iStReg < 8);
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    uint8_t     iReg    = (X86_FSW_TOP_GET(pFpuCtx->FSW) + iStReg) & X86_FSW_TOP_SMASK;
    pFpuCtx->FTW &= ~RT_BIT(iReg);
}


/**
 * Increments FSW.TOP, i.e. pops an item off the stack without freeing it.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemFpuStackIncTop(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    uint16_t    uFsw    = pFpuCtx->FSW;
    uint16_t    uTop    = uFsw & X86_FSW_TOP_MASK;
    uTop  = (uTop + (1 << X86_FSW_TOP_SHIFT)) & X86_FSW_TOP_MASK;
    uFsw &= ~X86_FSW_TOP_MASK;
    uFsw |= uTop;
    pFpuCtx->FSW = uFsw;
}


/**
 * Decrements FSW.TOP, i.e. push an item off the stack without storing anything.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemFpuStackDecTop(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    uint16_t    uFsw    = pFpuCtx->FSW;
    uint16_t    uTop    = uFsw & X86_FSW_TOP_MASK;
    uTop  = (uTop + (7 << X86_FSW_TOP_SHIFT)) & X86_FSW_TOP_MASK;
    uFsw &= ~X86_FSW_TOP_MASK;
    uFsw |= uTop;
    pFpuCtx->FSW = uFsw;
}




DECLINLINE(int) iemFpuStRegNotEmpty(PVMCPUCC pVCpu, uint8_t iStReg) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    uint16_t    iReg    = (X86_FSW_TOP_GET(pFpuCtx->FSW) + iStReg) & X86_FSW_TOP_SMASK;
    if (pFpuCtx->FTW & RT_BIT(iReg))
        return VINF_SUCCESS;
    return VERR_NOT_FOUND;
}


DECLINLINE(int) iemFpuStRegNotEmptyRef(PVMCPUCC pVCpu, uint8_t iStReg, PCRTFLOAT80U *ppRef) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    uint16_t    iReg    = (X86_FSW_TOP_GET(pFpuCtx->FSW) + iStReg) & X86_FSW_TOP_SMASK;
    if (pFpuCtx->FTW & RT_BIT(iReg))
    {
        *ppRef = &pFpuCtx->aRegs[iStReg].r80;
        return VINF_SUCCESS;
    }
    return VERR_NOT_FOUND;
}


DECLINLINE(int) iemFpu2StRegsNotEmptyRef(PVMCPUCC pVCpu, uint8_t iStReg0, PCRTFLOAT80U *ppRef0,
                                        uint8_t iStReg1, PCRTFLOAT80U *ppRef1) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    uint16_t    iTop    = X86_FSW_TOP_GET(pFpuCtx->FSW);
    uint16_t    iReg0   = (iTop + iStReg0) & X86_FSW_TOP_SMASK;
    uint16_t    iReg1   = (iTop + iStReg1) & X86_FSW_TOP_SMASK;
    if ((pFpuCtx->FTW & (RT_BIT(iReg0) | RT_BIT(iReg1))) == (RT_BIT(iReg0) | RT_BIT(iReg1)))
    {
        *ppRef0 = &pFpuCtx->aRegs[iStReg0].r80;
        *ppRef1 = &pFpuCtx->aRegs[iStReg1].r80;
        return VINF_SUCCESS;
    }
    return VERR_NOT_FOUND;
}


DECLINLINE(int) iemFpu2StRegsNotEmptyRefFirst(PVMCPUCC pVCpu, uint8_t iStReg0, PCRTFLOAT80U *ppRef0, uint8_t iStReg1) RT_NOEXCEPT
{
    PX86FXSTATE pFpuCtx = &pVCpu->cpum.GstCtx.XState.x87;
    uint16_t    iTop    = X86_FSW_TOP_GET(pFpuCtx->FSW);
    uint16_t    iReg0   = (iTop + iStReg0) & X86_FSW_TOP_SMASK;
    uint16_t    iReg1   = (iTop + iStReg1) & X86_FSW_TOP_SMASK;
    if ((pFpuCtx->FTW & (RT_BIT(iReg0) | RT_BIT(iReg1))) == (RT_BIT(iReg0) | RT_BIT(iReg1)))
    {
        *ppRef0 = &pFpuCtx->aRegs[iStReg0].r80;
        return VINF_SUCCESS;
    }
    return VERR_NOT_FOUND;
}


/**
 * Rotates the stack registers when setting new TOS.
 *
 * @param   pFpuCtx             The FPU context.
 * @param   iNewTop             New TOS value.
 * @remarks We only do this to speed up fxsave/fxrstor which
 *          arrange the FP registers in stack order.
 *          MUST be done before writing the new TOS (FSW).
 */
DECLINLINE(void) iemFpuRotateStackSetTop(PX86FXSTATE pFpuCtx, uint16_t iNewTop) RT_NOEXCEPT
{
    uint16_t iOldTop = X86_FSW_TOP_GET(pFpuCtx->FSW);
    RTFLOAT80U ar80Temp[8];

    if (iOldTop == iNewTop)
        return;

    /* Unscrew the stack and get it into 'native' order. */
    ar80Temp[0] = pFpuCtx->aRegs[(8 - iOldTop + 0) & X86_FSW_TOP_SMASK].r80;
    ar80Temp[1] = pFpuCtx->aRegs[(8 - iOldTop + 1) & X86_FSW_TOP_SMASK].r80;
    ar80Temp[2] = pFpuCtx->aRegs[(8 - iOldTop + 2) & X86_FSW_TOP_SMASK].r80;
    ar80Temp[3] = pFpuCtx->aRegs[(8 - iOldTop + 3) & X86_FSW_TOP_SMASK].r80;
    ar80Temp[4] = pFpuCtx->aRegs[(8 - iOldTop + 4) & X86_FSW_TOP_SMASK].r80;
    ar80Temp[5] = pFpuCtx->aRegs[(8 - iOldTop + 5) & X86_FSW_TOP_SMASK].r80;
    ar80Temp[6] = pFpuCtx->aRegs[(8 - iOldTop + 6) & X86_FSW_TOP_SMASK].r80;
    ar80Temp[7] = pFpuCtx->aRegs[(8 - iOldTop + 7) & X86_FSW_TOP_SMASK].r80;

    /* Now rotate the stack to the new position. */
    pFpuCtx->aRegs[0].r80 = ar80Temp[(iNewTop + 0) & X86_FSW_TOP_SMASK];
    pFpuCtx->aRegs[1].r80 = ar80Temp[(iNewTop + 1) & X86_FSW_TOP_SMASK];
    pFpuCtx->aRegs[2].r80 = ar80Temp[(iNewTop + 2) & X86_FSW_TOP_SMASK];
    pFpuCtx->aRegs[3].r80 = ar80Temp[(iNewTop + 3) & X86_FSW_TOP_SMASK];
    pFpuCtx->aRegs[4].r80 = ar80Temp[(iNewTop + 4) & X86_FSW_TOP_SMASK];
    pFpuCtx->aRegs[5].r80 = ar80Temp[(iNewTop + 5) & X86_FSW_TOP_SMASK];
    pFpuCtx->aRegs[6].r80 = ar80Temp[(iNewTop + 6) & X86_FSW_TOP_SMASK];
    pFpuCtx->aRegs[7].r80 = ar80Temp[(iNewTop + 7) & X86_FSW_TOP_SMASK];
}


/**
 * Updates the FPU exception status after FCW is changed.
 *
 * @param   pFpuCtx             The FPU context.
 */
DECLINLINE(void) iemFpuRecalcExceptionStatus(PX86FXSTATE pFpuCtx) RT_NOEXCEPT
{
    uint16_t u16Fsw = pFpuCtx->FSW;
    if ((u16Fsw & X86_FSW_XCPT_MASK) & ~(pFpuCtx->FCW & X86_FCW_XCPT_MASK))
        u16Fsw |= X86_FSW_ES | X86_FSW_B;
    else
        u16Fsw &= ~(X86_FSW_ES | X86_FSW_B);
    pFpuCtx->FSW = u16Fsw;
}


/**
 * Calculates the full FTW (FPU tag word) for use in FNSTENV and FNSAVE.
 *
 * @returns The full FTW.
 * @param   pFpuCtx             The FPU context.
 */
DECLINLINE(uint16_t) iemFpuCalcFullFtw(PCX86FXSTATE pFpuCtx) RT_NOEXCEPT
{
    uint8_t const   u8Ftw  = (uint8_t)pFpuCtx->FTW;
    uint16_t        u16Ftw = 0;
    unsigned const  iTop   = X86_FSW_TOP_GET(pFpuCtx->FSW);
    for (unsigned iSt = 0; iSt < 8; iSt++)
    {
        unsigned const iReg = (iSt + iTop) & 7;
        if (!(u8Ftw & RT_BIT(iReg)))
            u16Ftw |= 3 << (iReg * 2); /* empty */
        else
        {
            uint16_t uTag;
            PCRTFLOAT80U const pr80Reg = &pFpuCtx->aRegs[iSt].r80;
            if (pr80Reg->s.uExponent == 0x7fff)
                uTag = 2; /* Exponent is all 1's => Special. */
            else if (pr80Reg->s.uExponent == 0x0000)
            {
                if (pr80Reg->s.uMantissa == 0x0000)
                    uTag = 1; /* All bits are zero => Zero. */
                else
                    uTag = 2; /* Must be special. */
            }
            else if (pr80Reg->s.uMantissa & RT_BIT_64(63)) /* The J bit. */
                uTag = 0; /* Valid. */
            else
                uTag = 2; /* Must be special. */

            u16Ftw |= uTag << (iReg * 2);
        }
    }

    return u16Ftw;
}


/**
 * Converts a full FTW to a compressed one (for use in FLDENV and FRSTOR).
 *
 * @returns The compressed FTW.
 * @param   u16FullFtw      The full FTW to convert.
 */
DECLINLINE(uint16_t) iemFpuCompressFtw(uint16_t u16FullFtw) RT_NOEXCEPT
{
    uint8_t u8Ftw = 0;
    for (unsigned i = 0; i < 8; i++)
    {
        if ((u16FullFtw & 3) != 3 /*empty*/)
            u8Ftw |= RT_BIT(i);
        u16FullFtw >>= 2;
    }

    return u8Ftw;
}

/** @}  */


/** @name   Memory access.
 *
 * @{
 */


/**
 * Checks whether alignment checks are enabled or not.
 *
 * @returns true if enabled, false if not.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(bool) iemMemAreAlignmentChecksEnabled(PVMCPUCC pVCpu) RT_NOEXCEPT
{
    AssertCompile(X86_CR0_AM == X86_EFL_AC);
    return pVCpu->iem.s.uCpl == 3
        && (((uint32_t)pVCpu->cpum.GstCtx.cr0 & pVCpu->cpum.GstCtx.eflags.u) & X86_CR0_AM);
}

/**
 * Checks if the given segment can be written to, raise the appropriate
 * exception if not.
 *
 * @returns VBox strict status code.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pHid                Pointer to the hidden register.
 * @param   iSegReg             The register number.
 * @param   pu64BaseAddr        Where to return the base address to use for the
 *                              segment. (In 64-bit code it may differ from the
 *                              base in the hidden segment.)
 */
DECLINLINE(VBOXSTRICTRC) iemMemSegCheckWriteAccessEx(PVMCPUCC pVCpu, PCCPUMSELREGHID pHid,
                                                     uint8_t iSegReg, uint64_t *pu64BaseAddr) RT_NOEXCEPT
{
    IEM_CTX_ASSERT(pVCpu, CPUMCTX_EXTRN_SREG_FROM_IDX(iSegReg));

    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        *pu64BaseAddr = iSegReg < X86_SREG_FS ? 0 : pHid->u64Base;
    else
    {
        if (!pHid->Attr.n.u1Present)
        {
            uint16_t    uSel = iemSRegFetchU16(pVCpu, iSegReg);
            AssertRelease(uSel == 0);
            Log(("iemMemSegCheckWriteAccessEx: %#x (index %u) - bad selector -> #GP\n", uSel, iSegReg));
            return iemRaiseGeneralProtectionFault0(pVCpu);
        }

        if (   (   (pHid->Attr.n.u4Type & X86_SEL_TYPE_CODE)
                || !(pHid->Attr.n.u4Type & X86_SEL_TYPE_WRITE) )
            &&  pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT )
            return iemRaiseSelectorInvalidAccess(pVCpu, iSegReg, IEM_ACCESS_DATA_W);
        *pu64BaseAddr = pHid->u64Base;
    }
    return VINF_SUCCESS;
}


/**
 * Checks if the given segment can be read from, raise the appropriate
 * exception if not.
 *
 * @returns VBox strict status code.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pHid                Pointer to the hidden register.
 * @param   iSegReg             The register number.
 * @param   pu64BaseAddr        Where to return the base address to use for the
 *                              segment. (In 64-bit code it may differ from the
 *                              base in the hidden segment.)
 */
DECLINLINE(VBOXSTRICTRC) iemMemSegCheckReadAccessEx(PVMCPUCC pVCpu, PCCPUMSELREGHID pHid,
                                                    uint8_t iSegReg, uint64_t *pu64BaseAddr) RT_NOEXCEPT
{
    IEM_CTX_ASSERT(pVCpu, CPUMCTX_EXTRN_SREG_FROM_IDX(iSegReg));

    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        *pu64BaseAddr = iSegReg < X86_SREG_FS ? 0 : pHid->u64Base;
    else
    {
        if (!pHid->Attr.n.u1Present)
        {
            uint16_t    uSel = iemSRegFetchU16(pVCpu, iSegReg);
            AssertRelease(uSel == 0);
            Log(("iemMemSegCheckReadAccessEx: %#x (index %u) - bad selector -> #GP\n", uSel, iSegReg));
            return iemRaiseGeneralProtectionFault0(pVCpu);
        }

        if ((pHid->Attr.n.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_READ)) == X86_SEL_TYPE_CODE)
            return iemRaiseSelectorInvalidAccess(pVCpu, iSegReg, IEM_ACCESS_DATA_R);
        *pu64BaseAddr = pHid->u64Base;
    }
    return VINF_SUCCESS;
}


/**
 * Maps a physical page.
 *
 * @returns VBox status code (see PGMR3PhysTlbGCPhys2Ptr).
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   GCPhysMem           The physical address.
 * @param   fAccess             The intended access.
 * @param   ppvMem              Where to return the mapping address.
 * @param   pLock               The PGM lock.
 */
DECLINLINE(int) iemMemPageMap(PVMCPUCC pVCpu, RTGCPHYS GCPhysMem, uint32_t fAccess,
                              void **ppvMem, PPGMPAGEMAPLOCK pLock) RT_NOEXCEPT
{
#ifdef IEM_LOG_MEMORY_WRITES
    if (fAccess & IEM_ACCESS_TYPE_WRITE)
        return VERR_PGM_PHYS_TLB_CATCH_ALL;
#endif

    /** @todo This API may require some improving later.  A private deal with PGM
     *        regarding locking and unlocking needs to be struct.  A couple of TLBs
     *        living in PGM, but with publicly accessible inlined access methods
     *        could perhaps be an even better solution. */
    int rc = PGMPhysIemGCPhys2Ptr(pVCpu->CTX_SUFF(pVM), pVCpu,
                                  GCPhysMem,
                                  RT_BOOL(fAccess & IEM_ACCESS_TYPE_WRITE),
                                  pVCpu->iem.s.fBypassHandlers,
                                  ppvMem,
                                  pLock);
    /*Log(("PGMPhysIemGCPhys2Ptr %Rrc pLock=%.*Rhxs\n", rc, sizeof(*pLock), pLock));*/
    AssertMsg(rc == VINF_SUCCESS || RT_FAILURE_NP(rc), ("%Rrc\n", rc));

    return rc;
}


/**
 * Unmap a page previously mapped by iemMemPageMap.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   GCPhysMem           The physical address.
 * @param   fAccess             The intended access.
 * @param   pvMem               What iemMemPageMap returned.
 * @param   pLock               The PGM lock.
 */
DECLINLINE(void) iemMemPageUnmap(PVMCPUCC pVCpu, RTGCPHYS GCPhysMem, uint32_t fAccess,
                                 const void *pvMem, PPGMPAGEMAPLOCK pLock) RT_NOEXCEPT
{
    NOREF(pVCpu);
    NOREF(GCPhysMem);
    NOREF(fAccess);
    NOREF(pvMem);
    PGMPhysReleasePageMappingLock(pVCpu->CTX_SUFF(pVM), pLock);
}

#ifdef IEM_WITH_SETJMP

/** @todo slim this down   */
DECL_INLINE_THROW(RTGCPTR) iemMemApplySegmentToReadJmp(PVMCPUCC pVCpu, uint8_t iSegReg,
                                                       size_t cbMem, RTGCPTR GCPtrMem) IEM_NOEXCEPT_MAY_LONGJMP
{
    Assert(cbMem >= 1);
    Assert(iSegReg < X86_SREG_COUNT);

    /*
     * 64-bit mode is simpler.
     */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        if (iSegReg >= X86_SREG_FS && iSegReg != UINT8_MAX)
        {
            IEM_CTX_IMPORT_JMP(pVCpu, CPUMCTX_EXTRN_SREG_FROM_IDX(iSegReg));
            PCPUMSELREGHID const pSel = iemSRegGetHid(pVCpu, iSegReg);
            GCPtrMem += pSel->u64Base;
        }

        if (RT_LIKELY(X86_IS_CANONICAL(GCPtrMem) && X86_IS_CANONICAL(GCPtrMem + cbMem - 1)))
            return GCPtrMem;
        iemRaiseGeneralProtectionFault0Jmp(pVCpu);
    }
    /*
     * 16-bit and 32-bit segmentation.
     */
    else if (iSegReg != UINT8_MAX)
    {
        /** @todo Does this apply to segments with 4G-1 limit? */
        uint32_t const GCPtrLast32 = (uint32_t)GCPtrMem + (uint32_t)cbMem - 1;
        if (RT_LIKELY(GCPtrLast32 >= (uint32_t)GCPtrMem))
        {
            IEM_CTX_IMPORT_JMP(pVCpu, CPUMCTX_EXTRN_SREG_FROM_IDX(iSegReg));
            PCPUMSELREGHID const pSel = iemSRegGetHid(pVCpu, iSegReg);
            switch (pSel->Attr.u & (  X86DESCATTR_P     | X86DESCATTR_UNUSABLE
                                    | X86_SEL_TYPE_READ | X86_SEL_TYPE_WRITE /* same as read */
                                    | X86_SEL_TYPE_DOWN | X86_SEL_TYPE_CONF  /* same as down */
                                    | X86_SEL_TYPE_CODE))
            {
                case X86DESCATTR_P:                                         /* readonly data, expand up */
                case X86DESCATTR_P | X86_SEL_TYPE_WRITE:                    /* writable data, expand up */
                case X86DESCATTR_P | X86_SEL_TYPE_CODE | X86_SEL_TYPE_READ: /* code, read-only */
                case X86DESCATTR_P | X86_SEL_TYPE_CODE | X86_SEL_TYPE_READ | X86_SEL_TYPE_CONF: /* conforming code, read-only */
                    /* expand up */
                    if (RT_LIKELY(GCPtrLast32 <= pSel->u32Limit))
                        return (uint32_t)GCPtrMem + (uint32_t)pSel->u64Base;
                    Log10(("iemMemApplySegmentToReadJmp: out of bounds %#x..%#x vs %#x\n",
                           (uint32_t)GCPtrMem, GCPtrLast32, pSel->u32Limit));
                    break;

                case X86DESCATTR_P | X86_SEL_TYPE_DOWN:                         /* readonly data, expand down */
                case X86DESCATTR_P | X86_SEL_TYPE_DOWN | X86_SEL_TYPE_WRITE:    /* writable data, expand down */
                    /* expand down */
                    if (RT_LIKELY(   (uint32_t)GCPtrMem > pSel->u32Limit
                                  && (   pSel->Attr.n.u1DefBig
                                      || GCPtrLast32 <= UINT32_C(0xffff)) ))
                        return (uint32_t)GCPtrMem + (uint32_t)pSel->u64Base;
                    Log10(("iemMemApplySegmentToReadJmp: expand down out of bounds %#x..%#x vs %#x..%#x\n",
                           (uint32_t)GCPtrMem, GCPtrLast32, pSel->u32Limit, pSel->Attr.n.u1DefBig ? UINT32_MAX : UINT16_MAX));
                    break;

                default:
                    Log10(("iemMemApplySegmentToReadJmp: bad selector %#x\n", pSel->Attr.u));
                    iemRaiseSelectorInvalidAccessJmp(pVCpu, iSegReg, IEM_ACCESS_DATA_R);
                    break;
            }
        }
        Log10(("iemMemApplySegmentToReadJmp: out of bounds %#x..%#x\n",(uint32_t)GCPtrMem, GCPtrLast32));
        iemRaiseSelectorBoundsJmp(pVCpu, iSegReg, IEM_ACCESS_DATA_R);
    }
    /*
     * 32-bit flat address.
     */
    else
        return GCPtrMem;
}


/** @todo slim this down   */
DECL_INLINE_THROW(RTGCPTR) iemMemApplySegmentToWriteJmp(PVMCPUCC pVCpu, uint8_t iSegReg, size_t cbMem,
                                                        RTGCPTR GCPtrMem) IEM_NOEXCEPT_MAY_LONGJMP
{
    Assert(cbMem >= 1);
    Assert(iSegReg < X86_SREG_COUNT);

    /*
     * 64-bit mode is simpler.
     */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        if (iSegReg >= X86_SREG_FS)
        {
            IEM_CTX_IMPORT_JMP(pVCpu, CPUMCTX_EXTRN_SREG_FROM_IDX(iSegReg));
            PCPUMSELREGHID pSel = iemSRegGetHid(pVCpu, iSegReg);
            GCPtrMem += pSel->u64Base;
        }

        if (RT_LIKELY(X86_IS_CANONICAL(GCPtrMem) && X86_IS_CANONICAL(GCPtrMem + cbMem - 1)))
            return GCPtrMem;
    }
    /*
     * 16-bit and 32-bit segmentation.
     */
    else
    {
        IEM_CTX_IMPORT_JMP(pVCpu, CPUMCTX_EXTRN_SREG_FROM_IDX(iSegReg));
        PCPUMSELREGHID pSel           = iemSRegGetHid(pVCpu, iSegReg);
        uint32_t const fRelevantAttrs = pSel->Attr.u & (  X86DESCATTR_P     | X86DESCATTR_UNUSABLE
                                                        | X86_SEL_TYPE_CODE | X86_SEL_TYPE_WRITE | X86_SEL_TYPE_DOWN);
        if (fRelevantAttrs == (X86DESCATTR_P | X86_SEL_TYPE_WRITE)) /* data, expand up */
        {
            /* expand up */
            uint32_t GCPtrLast32 = (uint32_t)GCPtrMem + (uint32_t)cbMem;
            if (RT_LIKELY(   GCPtrLast32 > pSel->u32Limit
                          && GCPtrLast32 > (uint32_t)GCPtrMem))
                return (uint32_t)GCPtrMem + (uint32_t)pSel->u64Base;
        }
        else if (fRelevantAttrs == (X86DESCATTR_P | X86_SEL_TYPE_WRITE | X86_SEL_TYPE_DOWN)) /* data, expand up */
        {
            /* expand down */
            uint32_t GCPtrLast32 = (uint32_t)GCPtrMem + (uint32_t)cbMem;
            if (RT_LIKELY(   (uint32_t)GCPtrMem >  pSel->u32Limit
                          && GCPtrLast32        <= (pSel->Attr.n.u1DefBig ? UINT32_MAX : UINT32_C(0xffff))
                          && GCPtrLast32 > (uint32_t)GCPtrMem))
                return (uint32_t)GCPtrMem + (uint32_t)pSel->u64Base;
        }
        else
            iemRaiseSelectorInvalidAccessJmp(pVCpu, iSegReg, IEM_ACCESS_DATA_W);
        iemRaiseSelectorBoundsJmp(pVCpu, iSegReg, IEM_ACCESS_DATA_W);
    }
    iemRaiseGeneralProtectionFault0Jmp(pVCpu);
}

#endif /* IEM_WITH_SETJMP */

/**
 * Fakes a long mode stack selector for SS = 0.
 *
 * @param   pDescSs             Where to return the fake stack descriptor.
 * @param   uDpl                The DPL we want.
 */
DECLINLINE(void) iemMemFakeStackSelDesc(PIEMSELDESC pDescSs, uint32_t uDpl) RT_NOEXCEPT
{
    pDescSs->Long.au64[0] = 0;
    pDescSs->Long.au64[1] = 0;
    pDescSs->Long.Gen.u4Type     = X86_SEL_TYPE_RW_ACC;
    pDescSs->Long.Gen.u1DescType = 1; /* 1 = code / data, 0 = system. */
    pDescSs->Long.Gen.u2Dpl      = uDpl;
    pDescSs->Long.Gen.u1Present  = 1;
    pDescSs->Long.Gen.u1Long     = 1;
}

/** @} */


#ifdef VBOX_WITH_NESTED_HWVIRT_VMX

/**
 * Gets CR0 fixed-0 bits in VMX operation.
 *
 * We do this rather than fetching what we report to the guest (in
 * IA32_VMX_CR0_FIXED0 MSR) because real hardware (and so do we) report the same
 * values regardless of whether unrestricted-guest feature is available on the CPU.
 *
 * @returns CR0 fixed-0 bits.
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   fVmxNonRootMode     Whether the CR0 fixed-0 bits for VMX non-root mode
 *                              must be returned. When @c false, the CR0 fixed-0
 *                              bits for VMX root mode is returned.
 *
 */
DECLINLINE(uint64_t) iemVmxGetCr0Fixed0(PCVMCPUCC pVCpu, bool fVmxNonRootMode) RT_NOEXCEPT
{
    Assert(IEM_VMX_IS_ROOT_MODE(pVCpu));

    PCVMXMSRS  pMsrs = &pVCpu->cpum.GstCtx.hwvirt.vmx.Msrs;
    if (    fVmxNonRootMode
        && (pMsrs->ProcCtls2.n.allowed1 & VMX_PROC_CTLS2_UNRESTRICTED_GUEST))
        return VMX_V_CR0_FIXED0_UX;
    return VMX_V_CR0_FIXED0;
}


/**
 * Sets virtual-APIC write emulation as pending.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   offApic     The offset in the virtual-APIC page that was written.
 */
DECLINLINE(void) iemVmxVirtApicSetPendingWrite(PVMCPUCC pVCpu, uint16_t offApic) RT_NOEXCEPT
{
    Assert(offApic < XAPIC_OFF_END + 4);

    /*
     * Record the currently updated APIC offset, as we need this later for figuring
     * out whether to perform TPR, EOI or self-IPI virtualization as well as well
     * as for supplying the exit qualification when causing an APIC-write VM-exit.
     */
    pVCpu->cpum.GstCtx.hwvirt.vmx.offVirtApicWrite = offApic;

    /*
     * Flag that we need to perform virtual-APIC write emulation (TPR/PPR/EOI/Self-IPI
     * virtualization or APIC-write emulation).
     */
    if (!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_VMX_APIC_WRITE))
        VMCPU_FF_SET(pVCpu, VMCPU_FF_VMX_APIC_WRITE);
}

#endif /* VBOX_WITH_NESTED_HWVIRT_VMX */

#endif /* !VMM_INCLUDED_SRC_include_IEMInline_h */
