/** @file
 * EM - Execution Monitor.
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

#ifndef VBOX_INCLUDED_vmm_em_h
#define VBOX_INCLUDED_vmm_em_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#include <VBox/vmm/trpm.h>
#include <VBox/vmm/vmapi.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_em        The Execution Monitor / Manager API
 * @ingroup grp_vmm
 * @{
 */

/** Enable to allow V86 code to run in raw mode. */
#define VBOX_RAW_V86

/**
 * The Execution Manager State.
 *
 * @remarks This is used in the saved state!
 */
typedef enum EMSTATE
{
    /** Not yet started. */
    EMSTATE_NONE = 1,
    /** Raw-mode execution. */
    EMSTATE_RAW,
    /** Hardware accelerated raw-mode execution. */
    EMSTATE_HM,
    /** Executing in IEM. */
    EMSTATE_IEM,
    /** Recompiled mode execution. */
    EMSTATE_REM,
    /** Execution is halted. (waiting for interrupt) */
    EMSTATE_HALTED,
    /** Application processor execution is halted. (waiting for startup IPI (SIPI)) */
    EMSTATE_WAIT_SIPI,
    /** Execution is suspended. */
    EMSTATE_SUSPENDED,
    /** The VM is terminating. */
    EMSTATE_TERMINATING,
    /** Guest debug event from raw-mode is being processed. */
    EMSTATE_DEBUG_GUEST_RAW,
    /** Guest debug event from hardware accelerated mode is being processed. */
    EMSTATE_DEBUG_GUEST_HM,
    /** Guest debug event from interpreted execution mode is being processed. */
    EMSTATE_DEBUG_GUEST_IEM,
    /** Guest debug event from recompiled-mode is being processed. */
    EMSTATE_DEBUG_GUEST_REM,
    /** Hypervisor debug event being processed. */
    EMSTATE_DEBUG_HYPER,
    /** The VM has encountered a fatal error. (And everyone is panicing....) */
    EMSTATE_GURU_MEDITATION,
    /** Executing in IEM, falling back on REM if we cannot switch back to HM or
     * RAW after a short while. */
    EMSTATE_IEM_THEN_REM,
    /** Executing in native (API) execution monitor. */
    EMSTATE_NEM,
    /** Guest debug event from NEM mode is being processed. */
    EMSTATE_DEBUG_GUEST_NEM,
    /** Just a hack to ensure that we get a 32-bit integer. */
    EMSTATE_MAKE_32BIT_HACK = 0x7fffffff
} EMSTATE;


/**
 * EMInterpretInstructionCPU execution modes.
 */
typedef enum
{
    /** Only supervisor code (CPL=0). */
    EMCODETYPE_SUPERVISOR,
    /** User-level code only. */
    EMCODETYPE_USER,
    /** Supervisor and user-level code (use with great care!). */
    EMCODETYPE_ALL,
    /** Just a hack to ensure that we get a 32-bit integer. */
    EMCODETYPE_32BIT_HACK = 0x7fffffff
} EMCODETYPE;

VMM_INT_DECL(EMSTATE)           EMGetState(PVMCPU pVCpu);
VMM_INT_DECL(void)              EMSetState(PVMCPU pVCpu, EMSTATE enmNewState);

/** @name Callback handlers for instruction emulation functions.
 * These are placed here because IOM wants to use them as well.
 * @{
 */
typedef DECLCALLBACKTYPE(uint32_t, FNEMULATEPARAM2UINT32,(void *pvParam1, uint64_t val2));
typedef FNEMULATEPARAM2UINT32    *PFNEMULATEPARAM2UINT32;
typedef DECLCALLBACKTYPE(uint32_t, FNEMULATEPARAM2,(void *pvParam1, size_t val2));
typedef FNEMULATEPARAM2          *PFNEMULATEPARAM2;
typedef DECLCALLBACKTYPE(uint32_t, FNEMULATEPARAM3,(void *pvParam1, uint64_t val2, size_t val3));
typedef FNEMULATEPARAM3          *PFNEMULATEPARAM3;
typedef DECLCALLBACKTYPE(int, FNEMULATELOCKPARAM2,(void *pvParam1, uint64_t val2, RTGCUINTREG32 *pf));
typedef FNEMULATELOCKPARAM2 *PFNEMULATELOCKPARAM2;
typedef DECLCALLBACKTYPE(int, FNEMULATELOCKPARAM3,(void *pvParam1, uint64_t val2, size_t cb, RTGCUINTREG32 *pf));
typedef FNEMULATELOCKPARAM3 *PFNEMULATELOCKPARAM3;
/** @}  */

VMMDECL(void)                   EMSetHypercallInstructionsEnabled(PVMCPU pVCpu, bool fEnabled);
VMMDECL(bool)                   EMAreHypercallInstructionsEnabled(PVMCPU pVCpu);
VMM_INT_DECL(bool)              EMShouldContinueAfterHalt(PVMCPU pVCpu, PCPUMCTX pCtx);
VMM_INT_DECL(bool)              EMMonitorWaitShouldContinue(PVMCPU pVCpu, PCPUMCTX pCtx);
VMM_INT_DECL(int)               EMMonitorWaitPrepare(PVMCPU pVCpu, uint64_t rax, uint64_t rcx, uint64_t rdx, RTGCPHYS GCPhys);
VMM_INT_DECL(void)              EMMonitorWaitClear(PVMCPU pVCpu);
VMM_INT_DECL(bool)              EMMonitorIsArmed(PVMCPU pVCpu);
VMM_INT_DECL(unsigned)          EMMonitorWaitIsActive(PVMCPU pVCpu);
VMM_INT_DECL(int)               EMMonitorWaitPerform(PVMCPU pVCpu, uint64_t rax, uint64_t rcx);
VMM_INT_DECL(int)               EMUnhaltAndWakeUp(PVMCC pVM, PVMCPUCC pVCpuDst);
VMMRZ_INT_DECL(VBOXSTRICTRC)    EMRZSetPendingIoPortWrite(PVMCPU pVCpu, RTIOPORT uPort, uint8_t cbInstr, uint8_t cbValue, uint32_t uValue);
VMMRZ_INT_DECL(VBOXSTRICTRC)    EMRZSetPendingIoPortRead(PVMCPU pVCpu, RTIOPORT uPort, uint8_t cbInstr, uint8_t cbValue);

/**
 * Common defined exit types that EM knows what to do about.
 *
 * These should be used instead of the VT-x, SVM or NEM specific ones for exits
 * worth optimizing.
 */
typedef enum EMEXITTYPE
{
    EMEXITTYPE_INVALID = 0,
    EMEXITTYPE_IO_PORT_READ,
    EMEXITTYPE_IO_PORT_WRITE,
    EMEXITTYPE_IO_PORT_STR_READ,
    EMEXITTYPE_IO_PORT_STR_WRITE,
    EMEXITTYPE_MMIO,
    EMEXITTYPE_MMIO_READ,
    EMEXITTYPE_MMIO_WRITE,
    EMEXITTYPE_MSR_READ,
    EMEXITTYPE_MSR_WRITE,
    EMEXITTYPE_CPUID,
    EMEXITTYPE_RDTSC,
    EMEXITTYPE_MOV_CRX,
    EMEXITTYPE_MOV_DRX,
    EMEXITTYPE_VMREAD,
    EMEXITTYPE_VMWRITE,

    /** @name Raw-mode only (for now), keep at end.
     * @{  */
    EMEXITTYPE_INVLPG,
    EMEXITTYPE_LLDT,
    EMEXITTYPE_RDPMC,
    EMEXITTYPE_CLTS,
    EMEXITTYPE_STI,
    EMEXITTYPE_INT,
    EMEXITTYPE_SYSCALL,
    EMEXITTYPE_SYSENTER,
    EMEXITTYPE_HLT
    /** @} */
} EMEXITTYPE;
AssertCompileSize(EMEXITTYPE, 4);

/** @name EMEXIT_F_XXX - EM exit flags.
 *
 * The flags the exit type are combined to a 32-bit number using the
 * EMEXIT_MAKE_FT() macro.
 *
 * @{  */
#define EMEXIT_F_TYPE_MASK          UINT32_C(0x00000fff)    /**< The exit type mask. */
#define EMEXIT_F_KIND_EM            UINT32_C(0x00000000)    /**< EMEXITTYPE */
#define EMEXIT_F_KIND_VMX           UINT32_C(0x00001000)    /**< VT-x exit codes. */
#define EMEXIT_F_KIND_SVM           UINT32_C(0x00002000)    /**< SVM exit codes. */
#define EMEXIT_F_KIND_NEM           UINT32_C(0x00003000)    /**< NEMEXITTYPE */
#define EMEXIT_F_KIND_XCPT          UINT32_C(0x00004000)    /**< Exception numbers (raw-mode). */
#define EMEXIT_F_KIND_MASK          UINT32_C(0x00007000)
#define EMEXIT_F_CS_EIP             UINT32_C(0x00010000)    /**< The PC is EIP in the low dword and CS in the high. */
#define EMEXIT_F_UNFLATTENED_PC     UINT32_C(0x00020000)    /**< The PC hasn't had CS.BASE added to it. */
/** HM is calling (from ring-0).  Preemption is currently disabled or we're using preemption hooks. */
#define EMEXIT_F_HM                 UINT32_C(0x00040000)
/** Combines flags and exit type into EMHistoryAddExit() input. */
#define EMEXIT_MAKE_FT(a_fFlags, a_uType)   ((a_fFlags) | (uint32_t)(a_uType))
/** @} */

typedef enum EMEXITACTION
{
    /** The record is free. */
    EMEXITACTION_FREE_RECORD = 0,
    /** Take normal action on the exit. */
    EMEXITACTION_NORMAL,
    /** Take normal action on the exit, already probed and found nothing. */
    EMEXITACTION_NORMAL_PROBED,
    /** Do a probe execution. */
    EMEXITACTION_EXEC_PROBE,
    /** Execute using EMEXITREC::cMaxInstructionsWithoutExit. */
    EMEXITACTION_EXEC_WITH_MAX
} EMEXITACTION;
AssertCompileSize(EMEXITACTION, 4);

/**
 * Accumulative exit record.
 *
 * This could perhaps be squeezed down a bit, but there isn't too much point.
 * We'll probably need more data as time goes by.
 */
typedef struct EMEXITREC
{
    /** The flat PC of the exit. */
    uint64_t            uFlatPC;
    /** Flags and type, see EMEXIT_MAKE_FT. */
    uint32_t            uFlagsAndType;
    /** The action to take (EMEXITACTION). */
    uint8_t             enmAction;
    uint8_t             bUnused;
    /** Maximum number of instructions to execute without hitting an exit. */
    uint16_t            cMaxInstructionsWithoutExit;
    /** The exit number (EMCPU::iNextExit) at which it was last updated. */
    uint64_t            uLastExitNo;
    /** Number of hits. */
    uint64_t            cHits;
} EMEXITREC;
AssertCompileSize(EMEXITREC, 32);
/** Pointer to an accumulative exit record. */
typedef EMEXITREC *PEMEXITREC;
/** Pointer to a const accumulative exit record. */
typedef EMEXITREC const *PCEMEXITREC;

VMM_INT_DECL(PCEMEXITREC)       EMHistoryAddExit(PVMCPUCC pVCpu, uint32_t uFlagsAndType, uint64_t uFlatPC, uint64_t uTimestamp);
#ifdef IN_RC
VMMRC_INT_DECL(void)            EMRCHistoryAddExitCsEip(PVMCPU pVCpu, uint32_t uFlagsAndType, uint16_t uCs, uint32_t uEip,
                                                        uint64_t uTimestamp);
#endif
VMM_INT_DECL(void)              EMHistoryUpdatePC(PVMCPUCC pVCpu, uint64_t uFlatPC, bool fFlattened);
VMM_INT_DECL(PCEMEXITREC)       EMHistoryUpdateFlagsAndType(PVMCPUCC pVCpu, uint32_t uFlagsAndType);
VMM_INT_DECL(PCEMEXITREC)       EMHistoryUpdateFlagsAndTypeAndPC(PVMCPUCC pVCpu, uint32_t uFlagsAndType, uint64_t uFlatPC);
VMM_INT_DECL(VBOXSTRICTRC)      EMHistoryExec(PVMCPUCC pVCpu, PCEMEXITREC pExitRec, uint32_t fWillExit);


/** @name Deprecated interpretation related APIs (use IEM).
 * @{ */
VMM_INT_DECL(int)               EMInterpretDisasCurrent(PVMCPUCC pVCpu, PDISCPUSTATE pCpu, unsigned *pcbInstr);
VMM_INT_DECL(int)               EMInterpretDisasOneEx(PVMCPUCC pVCpu, RTGCUINTPTR GCPtrInstr,
                                                      PDISCPUSTATE pDISState, unsigned *pcbInstr);
VMM_INT_DECL(VBOXSTRICTRC)      EMInterpretInstruction(PVMCPUCC pVCpu);
VMM_INT_DECL(VBOXSTRICTRC)      EMInterpretInstructionDisasState(PVMCPUCC pVCpu, PDISCPUSTATE pDis, uint64_t rip);
/** @} */


/** @name EM_ONE_INS_FLAGS_XXX - flags for EMR3HmSingleInstruction (et al).
 * @{ */
/** Return when CS:RIP changes or some other important event happens.
 * This means running whole REP and LOOP $ sequences for instance. */
#define EM_ONE_INS_FLAGS_RIP_CHANGE     RT_BIT_32(0)
/** Mask of valid flags. */
#define EM_ONE_INS_FLAGS_MASK           UINT32_C(0x00000001)
/** @} */


#ifdef IN_RING0
/** @defgroup grp_em_r0     The EM Host Context Ring-0 API
 * @{ */
VMMR0_INT_DECL(int)             EMR0InitVM(PGVM pGVM);
/** @} */
#endif


#ifdef IN_RING3
/** @defgroup grp_em_r3     The EM Host Context Ring-3 API
 * @{
 */

/**
 * Command argument for EMR3RawSetMode().
 *
 * It's possible to extend this interface to change several
 * execution modes at once should the need arise.
 */
typedef enum EMEXECPOLICY
{
    /** The customary invalid zero entry. */
    EMEXECPOLICY_INVALID = 0,
    /** Whether to recompile ring-0 code or execute it in raw/hm. */
    EMEXECPOLICY_RECOMPILE_RING0,
    /** Whether to recompile ring-3 code or execute it in raw/hm. */
    EMEXECPOLICY_RECOMPILE_RING3,
    /** Whether to only use IEM for execution. */
    EMEXECPOLICY_IEM_ALL,
    /** End of valid value (not included). */
    EMEXECPOLICY_END,
    /** The customary 32-bit type blowup. */
    EMEXECPOLICY_32BIT_HACK = 0x7fffffff
} EMEXECPOLICY;
VMMR3DECL(int)                  EMR3SetExecutionPolicy(PUVM pUVM, EMEXECPOLICY enmPolicy, bool fEnforce);
VMMR3DECL(int)                  EMR3QueryExecutionPolicy(PUVM pUVM, EMEXECPOLICY enmPolicy, bool *pfEnforced);
VMMR3DECL(int)                  EMR3QueryMainExecutionEngine(PUVM pUVM, uint8_t *pbMainExecutionEngine);

VMMR3_INT_DECL(int)             EMR3Init(PVM pVM);
VMMR3_INT_DECL(int)             EMR3InitCompleted(PVM pVM, VMINITCOMPLETED enmWhat);
VMMR3_INT_DECL(void)            EMR3Relocate(PVM pVM);
VMMR3_INT_DECL(void)            EMR3ResetCpu(PVMCPU pVCpu);
VMMR3_INT_DECL(void)            EMR3Reset(PVM pVM);
VMMR3_INT_DECL(int)             EMR3Term(PVM pVM);
VMMR3DECL(DECL_NO_RETURN(void)) EMR3FatalError(PVMCPU pVCpu, int rc);
VMMR3_INT_DECL(int)             EMR3ExecuteVM(PVM pVM, PVMCPU pVCpu);
VMMR3_INT_DECL(int)             EMR3CheckRawForcedActions(PVM pVM, PVMCPU pVCpu);
VMMR3_INT_DECL(VBOXSTRICTRC)    EMR3HmSingleInstruction(PVM pVM, PVMCPU pVCpu, uint32_t fFlags);

/** @} */
#endif /* IN_RING3 */

/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_em_h */

