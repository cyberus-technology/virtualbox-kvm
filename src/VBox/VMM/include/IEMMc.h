/* $Id: IEMMc.h $ */
/** @file
 * IEM - Interpreted Execution Manager - IEM_MC_XXX.
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

#ifndef VMM_INCLUDED_SRC_include_IEMMc_h
#define VMM_INCLUDED_SRC_include_IEMMc_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/** @name   "Microcode" macros.
 *
 * The idea is that we should be able to use the same code to interpret
 * instructions as well as recompiler instructions.  Thus this obfuscation.
 *
 * @{
 */
#define IEM_MC_BEGIN(a_cArgs, a_cLocals)                {
#define IEM_MC_END()                                    }

/** Internal macro. */
#define IEM_MC_RETURN_ON_FAILURE(a_Expr) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = a_Expr; \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)


/** Advances RIP, finishes the instruction and returns.
 * This may include raising debug exceptions and such. */
#define IEM_MC_ADVANCE_RIP_AND_FINISH()                 return iemRegAddToRipAndFinishingClearingRF(pVCpu, IEM_GET_INSTR_LEN(pVCpu))
/** Sets RIP (may trigger \#GP), finishes the instruction and returns. */
#define IEM_MC_REL_JMP_S8_AND_FINISH(a_i8) \
    return iemRegRipRelativeJumpS8AndFinishClearingRF(pVCpu, IEM_GET_INSTR_LEN(pVCpu), (a_i8), pVCpu->iem.s.enmEffOpSize)
/** Sets RIP (may trigger \#GP), finishes the instruction and returns.
 * @note only usable in 16-bit op size mode.  */
#define IEM_MC_REL_JMP_S16_AND_FINISH(a_i16) \
    return iemRegRipRelativeJumpS16AndFinishClearingRF(pVCpu, IEM_GET_INSTR_LEN(pVCpu), (a_i16))
/** Sets RIP (may trigger \#GP), finishes the instruction and returns. */
#define IEM_MC_REL_JMP_S32_AND_FINISH(a_i32) \
    return iemRegRipRelativeJumpS32AndFinishClearingRF(pVCpu, IEM_GET_INSTR_LEN(pVCpu), (a_i32), pVCpu->iem.s.enmEffOpSize)
/** Sets RIP (may trigger \#GP), finishes the instruction and returns. */
#define IEM_MC_SET_RIP_U16_AND_FINISH(a_u16NewIP)       return iemRegRipJumpU16AndFinishClearningRF((pVCpu), (a_u16NewIP))
/** Sets RIP (may trigger \#GP), finishes the instruction and returns. */
#define IEM_MC_SET_RIP_U32_AND_FINISH(a_u32NewIP)       return iemRegRipJumpU32AndFinishClearningRF((pVCpu), (a_u32NewIP))
/** Sets RIP (may trigger \#GP), finishes the instruction and returns. */
#define IEM_MC_SET_RIP_U64_AND_FINISH(a_u64NewIP)       return iemRegRipJumpU64AndFinishClearningRF((pVCpu), (a_u64NewIP))

#define IEM_MC_RAISE_DIVIDE_ERROR()                     return iemRaiseDivideError(pVCpu)
#define IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE()       \
    do { \
        if (pVCpu->cpum.GstCtx.cr0 & (X86_CR0_EM | X86_CR0_TS)) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_WAIT_DEVICE_NOT_AVAILABLE()  \
    do { \
        if ((pVCpu->cpum.GstCtx.cr0 & (X86_CR0_MP | X86_CR0_TS)) == (X86_CR0_MP | X86_CR0_TS)) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_FPU_XCPT() \
    do { \
        if (pVCpu->cpum.GstCtx.XState.x87.FSW & X86_FSW_ES) \
            return iemRaiseMathFault(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_AVX2_RELATED_XCPT() \
    do { \
        if (   (pVCpu->cpum.GstCtx.aXcr[0] & (XSAVE_C_YMM | XSAVE_C_SSE)) != (XSAVE_C_YMM | XSAVE_C_SSE) \
            || !(pVCpu->cpum.GstCtx.cr4 & X86_CR4_OSXSAVE) \
            || !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fAvx2) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (pVCpu->cpum.GstCtx.cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_AVX_RELATED_XCPT() \
    do { \
        if (   (pVCpu->cpum.GstCtx.aXcr[0] & (XSAVE_C_YMM | XSAVE_C_SSE)) != (XSAVE_C_YMM | XSAVE_C_SSE) \
            || !(pVCpu->cpum.GstCtx.cr4 & X86_CR4_OSXSAVE) \
            || !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fAvx) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (pVCpu->cpum.GstCtx.cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_AESNI_RELATED_XCPT() \
    do { \
        if (   (pVCpu->cpum.GstCtx.cr0 & X86_CR0_EM) \
            || !(pVCpu->cpum.GstCtx.cr4 & X86_CR4_OSFXSR) \
            || !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fAesNi) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (pVCpu->cpum.GstCtx.cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_SSE42_RELATED_XCPT() \
    do { \
        if (   (pVCpu->cpum.GstCtx.cr0 & X86_CR0_EM) \
            || !(pVCpu->cpum.GstCtx.cr4 & X86_CR4_OSFXSR) \
            || !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fSse42) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (pVCpu->cpum.GstCtx.cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_SSE41_RELATED_XCPT() \
    do { \
        if (   (pVCpu->cpum.GstCtx.cr0 & X86_CR0_EM) \
            || !(pVCpu->cpum.GstCtx.cr4 & X86_CR4_OSFXSR) \
            || !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fSse41) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (pVCpu->cpum.GstCtx.cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_SSSE3_RELATED_XCPT() \
    do { \
        if (   (pVCpu->cpum.GstCtx.cr0 & X86_CR0_EM) \
            || !(pVCpu->cpum.GstCtx.cr4 & X86_CR4_OSFXSR) \
            || !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fSsse3) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (pVCpu->cpum.GstCtx.cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_SSE3_RELATED_XCPT() \
    do { \
        if (   (pVCpu->cpum.GstCtx.cr0 & X86_CR0_EM) \
            || !(pVCpu->cpum.GstCtx.cr4 & X86_CR4_OSFXSR) \
            || !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fSse3) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (pVCpu->cpum.GstCtx.cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_SSE2_RELATED_XCPT() \
    do { \
        if (   (pVCpu->cpum.GstCtx.cr0 & X86_CR0_EM) \
            || !(pVCpu->cpum.GstCtx.cr4 & X86_CR4_OSFXSR) \
            || !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fSse2) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (pVCpu->cpum.GstCtx.cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_SSE_RELATED_XCPT() \
    do { \
        if (   (pVCpu->cpum.GstCtx.cr0 & X86_CR0_EM) \
            || !(pVCpu->cpum.GstCtx.cr4 & X86_CR4_OSFXSR) \
            || !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fSse) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (pVCpu->cpum.GstCtx.cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_MMX_RELATED_XCPT() \
    do { \
        if (   (pVCpu->cpum.GstCtx.cr0 & X86_CR0_EM) \
            || !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fMmx) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (pVCpu->cpum.GstCtx.cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
        if (pVCpu->cpum.GstCtx.XState.x87.FSW & X86_FSW_ES) \
            return iemRaiseMathFault(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_MMX_RELATED_XCPT_EX(a_fSupported) \
    do { \
        if (   (pVCpu->cpum.GstCtx.cr0 & X86_CR0_EM) \
            || !(a_fSupported)) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (pVCpu->cpum.GstCtx.cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
        if (pVCpu->cpum.GstCtx.XState.x87.FSW & X86_FSW_ES) \
            return iemRaiseMathFault(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_MMX_RELATED_XCPT_CHECK_SSE_OR_MMXEXT() \
    do { \
        if (   (pVCpu->cpum.GstCtx.cr0 & X86_CR0_EM) \
            || (   !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fSse \
                && !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fAmdMmxExts) ) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (pVCpu->cpum.GstCtx.cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
        if (pVCpu->cpum.GstCtx.XState.x87.FSW & X86_FSW_ES) \
            return iemRaiseMathFault(pVCpu); \
    } while (0)
#define IEM_MC_RAISE_GP0_IF_CPL_NOT_ZERO() \
    do { \
        if (pVCpu->iem.s.uCpl != 0) \
            return iemRaiseGeneralProtectionFault0(pVCpu); \
    } while (0)
#define IEM_MC_RAISE_GP0_IF_EFF_ADDR_UNALIGNED(a_EffAddr, a_cbAlign) \
    do { \
        if (!((a_EffAddr) & ((a_cbAlign) - 1))) { /* likely */ } \
        else return iemRaiseGeneralProtectionFault0(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_FSGSBASE_XCPT() \
    do { \
        if (   pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT \
            || !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fFsGsBase \
            || !(pVCpu->cpum.GstCtx.cr4 & X86_CR4_FSGSBASE)) \
            return iemRaiseUndefinedOpcode(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_NON_CANONICAL_ADDR_GP0(a_u64Addr) \
    do { \
        if (!IEM_IS_CANONICAL(a_u64Addr)) \
            return iemRaiseGeneralProtectionFault0(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_SSE_AVX_SIMD_FP_OR_UD_XCPT() \
    do { \
        if ((  ~((pVCpu->cpum.GstCtx.XState.x87.MXCSR & X86_MXCSR_XCPT_MASK) >> X86_MXCSR_XCPT_MASK_SHIFT) \
             & (pVCpu->cpum.GstCtx.XState.x87.MXCSR & X86_MXCSR_XCPT_FLAGS)) != 0) \
        { \
            if (pVCpu->cpum.GstCtx.cr4 & X86_CR4_OSXMMEEXCPT)\
                return iemRaiseSimdFpException(pVCpu); \
            else \
                return iemRaiseUndefinedOpcode(pVCpu); \
        } \
    } while (0)
#define IEM_MC_RAISE_SSE_AVX_SIMD_FP_OR_UD_XCPT() \
    do { \
        if (pVCpu->cpum.GstCtx.cr4 & X86_CR4_OSXMMEEXCPT)\
            return iemRaiseSimdFpException(pVCpu); \
        else \
            return iemRaiseUndefinedOpcode(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_PCLMUL_RELATED_XCPT() \
    do { \
        if (   (pVCpu->cpum.GstCtx.cr0 & X86_CR0_EM) \
            || !(pVCpu->cpum.GstCtx.cr4 & X86_CR4_OSFXSR) \
            || !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fPclMul) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (pVCpu->cpum.GstCtx.cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)


#define IEM_MC_LOCAL(a_Type, a_Name)                    a_Type a_Name
#define IEM_MC_LOCAL_CONST(a_Type, a_Name, a_Value)     a_Type const a_Name = (a_Value)
#define IEM_MC_REF_LOCAL(a_pRefArg, a_Local)            (a_pRefArg) = &(a_Local)
#define IEM_MC_ARG(a_Type, a_Name, a_iArg)              a_Type a_Name
#define IEM_MC_ARG_CONST(a_Type, a_Name, a_Value, a_iArg)       a_Type const a_Name = (a_Value)
#define IEM_MC_ARG_LOCAL_REF(a_Type, a_Name, a_Local, a_iArg)   a_Type const a_Name = &(a_Local)
#define IEM_MC_ARG_LOCAL_EFLAGS(a_pName, a_Name, a_iArg) \
    uint32_t a_Name; \
    uint32_t *a_pName = &a_Name
#define IEM_MC_COMMIT_EFLAGS(a_EFlags) \
   do { pVCpu->cpum.GstCtx.eflags.u = (a_EFlags); Assert(pVCpu->cpum.GstCtx.eflags.u & X86_EFL_1); } while (0)

#define IEM_MC_ASSIGN(a_VarOrArg, a_CVariableOrConst)   (a_VarOrArg) = (a_CVariableOrConst)
#define IEM_MC_ASSIGN_TO_SMALLER                        IEM_MC_ASSIGN

#define IEM_MC_FETCH_GREG_U8(a_u8Dst, a_iGReg)          (a_u8Dst)  = iemGRegFetchU8(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U8_ZX_U16(a_u16Dst, a_iGReg)  (a_u16Dst) = iemGRegFetchU8(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U8_ZX_U32(a_u32Dst, a_iGReg)  (a_u32Dst) = iemGRegFetchU8(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U8_ZX_U64(a_u64Dst, a_iGReg)  (a_u64Dst) = iemGRegFetchU8(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U8_SX_U16(a_u16Dst, a_iGReg)  (a_u16Dst) = (int8_t)iemGRegFetchU8(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U8_SX_U32(a_u32Dst, a_iGReg)  (a_u32Dst) = (int8_t)iemGRegFetchU8(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U8_SX_U64(a_u64Dst, a_iGReg)  (a_u64Dst) = (int8_t)iemGRegFetchU8(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U16(a_u16Dst, a_iGReg)        (a_u16Dst) = iemGRegFetchU16(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U16_ZX_U32(a_u32Dst, a_iGReg) (a_u32Dst) = iemGRegFetchU16(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U16_ZX_U64(a_u64Dst, a_iGReg) (a_u64Dst) = iemGRegFetchU16(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U16_SX_U32(a_u32Dst, a_iGReg) (a_u32Dst) = (int16_t)iemGRegFetchU16(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U16_SX_U64(a_u64Dst, a_iGReg) (a_u64Dst) = (int16_t)iemGRegFetchU16(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U32(a_u32Dst, a_iGReg)        (a_u32Dst) = iemGRegFetchU32(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U32_ZX_U64(a_u64Dst, a_iGReg) (a_u64Dst) = iemGRegFetchU32(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U32_SX_U64(a_u64Dst, a_iGReg) (a_u64Dst) = (int32_t)iemGRegFetchU32(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U64(a_u64Dst, a_iGReg)        (a_u64Dst) = iemGRegFetchU64(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U64_ZX_U64                    IEM_MC_FETCH_GREG_U64
#define IEM_MC_FETCH_SREG_U16(a_u16Dst, a_iSReg) do { \
        IEM_CTX_IMPORT_NORET(pVCpu, CPUMCTX_EXTRN_SREG_FROM_IDX(a_iSReg)); \
        (a_u16Dst) = iemSRegFetchU16(pVCpu, (a_iSReg)); \
    } while (0)
#define IEM_MC_FETCH_SREG_ZX_U32(a_u32Dst, a_iSReg) do { \
        IEM_CTX_IMPORT_NORET(pVCpu, CPUMCTX_EXTRN_SREG_FROM_IDX(a_iSReg)); \
        (a_u32Dst) = iemSRegFetchU16(pVCpu, (a_iSReg)); \
    } while (0)
#define IEM_MC_FETCH_SREG_ZX_U64(a_u64Dst, a_iSReg) do { \
        IEM_CTX_IMPORT_NORET(pVCpu, CPUMCTX_EXTRN_SREG_FROM_IDX(a_iSReg)); \
        (a_u64Dst) = iemSRegFetchU16(pVCpu, (a_iSReg)); \
    } while (0)
/** @todo IEM_MC_FETCH_SREG_BASE_U64 & IEM_MC_FETCH_SREG_BASE_U32 probably aren't worth it... */
#define IEM_MC_FETCH_SREG_BASE_U64(a_u64Dst, a_iSReg) do { \
        IEM_CTX_IMPORT_NORET(pVCpu, CPUMCTX_EXTRN_SREG_FROM_IDX(a_iSReg)); \
        (a_u64Dst) = iemSRegBaseFetchU64(pVCpu, (a_iSReg)); \
    } while (0)
#define IEM_MC_FETCH_SREG_BASE_U32(a_u32Dst, a_iSReg) do { \
        IEM_CTX_IMPORT_NORET(pVCpu, CPUMCTX_EXTRN_SREG_FROM_IDX(a_iSReg)); \
        (a_u32Dst) = iemSRegBaseFetchU64(pVCpu, (a_iSReg)); \
    } while (0)
/** @note Not for IOPL or IF testing or modification. */
#define IEM_MC_FETCH_EFLAGS(a_EFlags)                   (a_EFlags) = pVCpu->cpum.GstCtx.eflags.u
#define IEM_MC_FETCH_EFLAGS_U8(a_EFlags)                (a_EFlags) = (uint8_t)pVCpu->cpum.GstCtx.eflags.u
#define IEM_MC_FETCH_FSW(a_u16Fsw)                      (a_u16Fsw) = pVCpu->cpum.GstCtx.XState.x87.FSW
#define IEM_MC_FETCH_FCW(a_u16Fcw)                      (a_u16Fcw) = pVCpu->cpum.GstCtx.XState.x87.FCW

#define IEM_MC_STORE_GREG_U8(a_iGReg, a_u8Value)        *iemGRegRefU8( pVCpu, (a_iGReg)) = (a_u8Value)
#define IEM_MC_STORE_GREG_U16(a_iGReg, a_u16Value)      *iemGRegRefU16(pVCpu, (a_iGReg)) = (a_u16Value)
#define IEM_MC_STORE_GREG_U32(a_iGReg, a_u32Value)      *iemGRegRefU64(pVCpu, (a_iGReg)) = (uint32_t)(a_u32Value) /* clear high bits. */
#define IEM_MC_STORE_GREG_U64(a_iGReg, a_u64Value)      *iemGRegRefU64(pVCpu, (a_iGReg)) = (a_u64Value)
#define IEM_MC_STORE_GREG_I64(a_iGReg, a_i64Value)      *iemGRegRefI64(pVCpu, (a_iGReg)) = (a_i64Value)
#define IEM_MC_STORE_GREG_U8_CONST                      IEM_MC_STORE_GREG_U8
#define IEM_MC_STORE_GREG_U16_CONST                     IEM_MC_STORE_GREG_U16
#define IEM_MC_STORE_GREG_U32_CONST                     IEM_MC_STORE_GREG_U32
#define IEM_MC_STORE_GREG_U64_CONST                     IEM_MC_STORE_GREG_U64
#define IEM_MC_CLEAR_HIGH_GREG_U64(a_iGReg)             *iemGRegRefU64(pVCpu, (a_iGReg)) &= UINT32_MAX
#define IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(a_pu32Dst)    do { (a_pu32Dst)[1] = 0; } while (0)
/** @todo IEM_MC_STORE_SREG_BASE_U64 & IEM_MC_STORE_SREG_BASE_U32 aren't worth it... */
#define IEM_MC_STORE_SREG_BASE_U64(a_iSReg, a_u64Value) do { \
        IEM_CTX_IMPORT_NORET(pVCpu, CPUMCTX_EXTRN_SREG_FROM_IDX(a_iSReg)); \
        *iemSRegBaseRefU64(pVCpu, (a_iSReg)) = (a_u64Value); \
    } while (0)
#define IEM_MC_STORE_SREG_BASE_U32(a_iSReg, a_u32Value) do { \
        IEM_CTX_IMPORT_NORET(pVCpu, CPUMCTX_EXTRN_SREG_FROM_IDX(a_iSReg)); \
        *iemSRegBaseRefU64(pVCpu, (a_iSReg)) = (uint32_t)(a_u32Value); /* clear high bits. */ \
    } while (0)
#define IEM_MC_STORE_FPUREG_R80_SRC_REF(a_iSt, a_pr80Src) \
    do { pVCpu->cpum.GstCtx.XState.x87.aRegs[a_iSt].r80 = *(a_pr80Src); } while (0)


#define IEM_MC_REF_GREG_U8(a_pu8Dst, a_iGReg)           (a_pu8Dst)  = iemGRegRefU8( pVCpu, (a_iGReg))
#define IEM_MC_REF_GREG_U16(a_pu16Dst, a_iGReg)         (a_pu16Dst) = iemGRegRefU16(pVCpu, (a_iGReg))
/** @todo User of IEM_MC_REF_GREG_U32 needs to clear the high bits on commit.
 *        Use IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF! */
#define IEM_MC_REF_GREG_U32(a_pu32Dst, a_iGReg)         (a_pu32Dst) = iemGRegRefU32(pVCpu, (a_iGReg))
#define IEM_MC_REF_GREG_I32(a_pi32Dst, a_iGReg)         (a_pi32Dst) = (int32_t *)iemGRegRefU32(pVCpu, (a_iGReg))
#define IEM_MC_REF_GREG_I32_CONST(a_pi32Dst, a_iGReg)   (a_pi32Dst) = (int32_t const *)iemGRegRefU32(pVCpu, (a_iGReg))
#define IEM_MC_REF_GREG_U64(a_pu64Dst, a_iGReg)         (a_pu64Dst) = iemGRegRefU64(pVCpu, (a_iGReg))
#define IEM_MC_REF_GREG_I64(a_pi64Dst, a_iGReg)         (a_pi64Dst) = (int64_t *)iemGRegRefU64(pVCpu, (a_iGReg))
#define IEM_MC_REF_GREG_I64_CONST(a_pi64Dst, a_iGReg)   (a_pi64Dst) = (int64_t const *)iemGRegRefU64(pVCpu, (a_iGReg))
/** @note Not for IOPL or IF testing or modification.
 * @note Must preserve any undefined bits, see CPUMX86EFLAGS! */
#define IEM_MC_REF_EFLAGS(a_pEFlags)                    (a_pEFlags) = &pVCpu->cpum.GstCtx.eflags.uBoth
#define IEM_MC_REF_MXCSR(a_pfMxcsr)                     (a_pfMxcsr) = &pVCpu->cpum.GstCtx.XState.x87.MXCSR

#define IEM_MC_ADD_GREG_U8(a_iGReg, a_u8Value)          *iemGRegRefU8( pVCpu, (a_iGReg)) += (a_u8Value)
#define IEM_MC_ADD_GREG_U16(a_iGReg, a_u16Value)        *iemGRegRefU16(pVCpu, (a_iGReg)) += (a_u16Value)
#define IEM_MC_ADD_GREG_U32(a_iGReg, a_u32Value) \
    do { \
        uint32_t *pu32Reg = iemGRegRefU32(pVCpu, (a_iGReg)); \
        *pu32Reg += (a_u32Value); \
        pu32Reg[1] = 0; /* implicitly clear the high bit. */ \
    } while (0)
#define IEM_MC_ADD_GREG_U64(a_iGReg, a_u64Value)        *iemGRegRefU64(pVCpu, (a_iGReg)) += (a_u64Value)

#define IEM_MC_SUB_GREG_U8(a_iGReg,  a_u8Value)         *iemGRegRefU8( pVCpu, (a_iGReg)) -= (a_u8Value)
#define IEM_MC_SUB_GREG_U16(a_iGReg, a_u16Value)        *iemGRegRefU16(pVCpu, (a_iGReg)) -= (a_u16Value)
#define IEM_MC_SUB_GREG_U32(a_iGReg, a_u32Value) \
    do { \
        uint32_t *pu32Reg = iemGRegRefU32(pVCpu, (a_iGReg)); \
        *pu32Reg -= (a_u32Value); \
        pu32Reg[1] = 0; /* implicitly clear the high bit. */ \
    } while (0)
#define IEM_MC_SUB_GREG_U64(a_iGReg, a_u64Value)        *iemGRegRefU64(pVCpu, (a_iGReg)) -= (a_u64Value)
#define IEM_MC_SUB_LOCAL_U16(a_u16Value, a_u16Const)   do { (a_u16Value) -= a_u16Const; } while (0)

#define IEM_MC_ADD_GREG_U8_TO_LOCAL(a_u8Value, a_iGReg)    do { (a_u8Value)  += iemGRegFetchU8( pVCpu, (a_iGReg)); } while (0)
#define IEM_MC_ADD_GREG_U16_TO_LOCAL(a_u16Value, a_iGReg)  do { (a_u16Value) += iemGRegFetchU16(pVCpu, (a_iGReg)); } while (0)
#define IEM_MC_ADD_GREG_U32_TO_LOCAL(a_u32Value, a_iGReg)  do { (a_u32Value) += iemGRegFetchU32(pVCpu, (a_iGReg)); } while (0)
#define IEM_MC_ADD_GREG_U64_TO_LOCAL(a_u64Value, a_iGReg)  do { (a_u64Value) += iemGRegFetchU64(pVCpu, (a_iGReg)); } while (0)
#define IEM_MC_ADD_LOCAL_S16_TO_EFF_ADDR(a_EffAddr, a_i16) do { (a_EffAddr) += (a_i16); } while (0)
#define IEM_MC_ADD_LOCAL_S32_TO_EFF_ADDR(a_EffAddr, a_i32) do { (a_EffAddr) += (a_i32); } while (0)
#define IEM_MC_ADD_LOCAL_S64_TO_EFF_ADDR(a_EffAddr, a_i64) do { (a_EffAddr) += (a_i64); } while (0)

#define IEM_MC_AND_LOCAL_U8(a_u8Local, a_u8Mask)        do { (a_u8Local)  &= (a_u8Mask);  } while (0)
#define IEM_MC_AND_LOCAL_U16(a_u16Local, a_u16Mask)     do { (a_u16Local) &= (a_u16Mask); } while (0)
#define IEM_MC_AND_LOCAL_U32(a_u32Local, a_u32Mask)     do { (a_u32Local) &= (a_u32Mask); } while (0)
#define IEM_MC_AND_LOCAL_U64(a_u64Local, a_u64Mask)     do { (a_u64Local) &= (a_u64Mask); } while (0)

#define IEM_MC_AND_ARG_U16(a_u16Arg, a_u16Mask)         do { (a_u16Arg) &= (a_u16Mask); } while (0)
#define IEM_MC_AND_ARG_U32(a_u32Arg, a_u32Mask)         do { (a_u32Arg) &= (a_u32Mask); } while (0)
#define IEM_MC_AND_ARG_U64(a_u64Arg, a_u64Mask)         do { (a_u64Arg) &= (a_u64Mask); } while (0)

#define IEM_MC_OR_LOCAL_U8(a_u8Local, a_u8Mask)         do { (a_u8Local)  |= (a_u8Mask);  } while (0)
#define IEM_MC_OR_LOCAL_U16(a_u16Local, a_u16Mask)      do { (a_u16Local) |= (a_u16Mask); } while (0)
#define IEM_MC_OR_LOCAL_U32(a_u32Local, a_u32Mask)      do { (a_u32Local) |= (a_u32Mask); } while (0)

#define IEM_MC_SAR_LOCAL_S16(a_i16Local, a_cShift)      do { (a_i16Local) >>= (a_cShift);  } while (0)
#define IEM_MC_SAR_LOCAL_S32(a_i32Local, a_cShift)      do { (a_i32Local) >>= (a_cShift);  } while (0)
#define IEM_MC_SAR_LOCAL_S64(a_i64Local, a_cShift)      do { (a_i64Local) >>= (a_cShift);  } while (0)

#define IEM_MC_SHR_LOCAL_U8(a_u8Local, a_cShift)        do { (a_u8Local)  >>= (a_cShift);  } while (0)

#define IEM_MC_SHL_LOCAL_S16(a_i16Local, a_cShift)      do { (a_i16Local) <<= (a_cShift);  } while (0)
#define IEM_MC_SHL_LOCAL_S32(a_i32Local, a_cShift)      do { (a_i32Local) <<= (a_cShift);  } while (0)
#define IEM_MC_SHL_LOCAL_S64(a_i64Local, a_cShift)      do { (a_i64Local) <<= (a_cShift);  } while (0)

#define IEM_MC_AND_2LOCS_U32(a_u32Local, a_u32Mask)     do { (a_u32Local) &= (a_u32Mask); } while (0)

#define IEM_MC_OR_2LOCS_U32(a_u32Local, a_u32Mask)      do { (a_u32Local) |= (a_u32Mask); } while (0)

#define IEM_MC_AND_GREG_U8(a_iGReg, a_u8Value)          *iemGRegRefU8( pVCpu, (a_iGReg)) &= (a_u8Value)
#define IEM_MC_AND_GREG_U16(a_iGReg, a_u16Value)        *iemGRegRefU16(pVCpu, (a_iGReg)) &= (a_u16Value)
#define IEM_MC_AND_GREG_U32(a_iGReg, a_u32Value) \
    do { \
        uint32_t *pu32Reg = iemGRegRefU32(pVCpu, (a_iGReg)); \
        *pu32Reg &= (a_u32Value); \
        pu32Reg[1] = 0; /* implicitly clear the high bit. */ \
    } while (0)
#define IEM_MC_AND_GREG_U64(a_iGReg, a_u64Value)        *iemGRegRefU64(pVCpu, (a_iGReg)) &= (a_u64Value)

#define IEM_MC_OR_GREG_U8(a_iGReg, a_u8Value)           *iemGRegRefU8( pVCpu, (a_iGReg)) |= (a_u8Value)
#define IEM_MC_OR_GREG_U16(a_iGReg, a_u16Value)         *iemGRegRefU16(pVCpu, (a_iGReg)) |= (a_u16Value)
#define IEM_MC_OR_GREG_U32(a_iGReg, a_u32Value) \
    do { \
        uint32_t *pu32Reg = iemGRegRefU32(pVCpu, (a_iGReg)); \
        *pu32Reg |= (a_u32Value); \
        pu32Reg[1] = 0; /* implicitly clear the high bit. */ \
    } while (0)
#define IEM_MC_OR_GREG_U64(a_iGReg, a_u64Value)         *iemGRegRefU64(pVCpu, (a_iGReg)) |= (a_u64Value)

#define IEM_MC_BSWAP_LOCAL_U16(a_u16Local)              (a_u16Local) = RT_BSWAP_U16((a_u16Local));
#define IEM_MC_BSWAP_LOCAL_U32(a_u32Local)              (a_u32Local) = RT_BSWAP_U32((a_u32Local));
#define IEM_MC_BSWAP_LOCAL_U64(a_u64Local)              (a_u64Local) = RT_BSWAP_U64((a_u64Local));

/** @note Not for IOPL or IF modification. */
#define IEM_MC_SET_EFL_BIT(a_fBit)                      do { pVCpu->cpum.GstCtx.eflags.u |= (a_fBit); } while (0)
/** @note Not for IOPL or IF modification. */
#define IEM_MC_CLEAR_EFL_BIT(a_fBit)                    do { pVCpu->cpum.GstCtx.eflags.u &= ~(a_fBit); } while (0)
/** @note Not for IOPL or IF modification. */
#define IEM_MC_FLIP_EFL_BIT(a_fBit)                     do { pVCpu->cpum.GstCtx.eflags.u ^= (a_fBit); } while (0)

#define IEM_MC_CLEAR_FSW_EX()   do { pVCpu->cpum.GstCtx.XState.x87.FSW &= X86_FSW_C_MASK | X86_FSW_TOP_MASK; } while (0)

/** Switches the FPU state to MMX mode (FSW.TOS=0, FTW=0) if necessary. */
#define IEM_MC_FPU_TO_MMX_MODE() do { \
        iemFpuRotateStackSetTop(&pVCpu->cpum.GstCtx.XState.x87, 0); \
        pVCpu->cpum.GstCtx.XState.x87.FSW &= ~X86_FSW_TOP_MASK; \
        pVCpu->cpum.GstCtx.XState.x87.FTW  = 0xff; \
    } while (0)

/** Switches the FPU state from MMX mode (FSW.TOS=0, FTW=0xffff). */
#define IEM_MC_FPU_FROM_MMX_MODE() do { \
        iemFpuRotateStackSetTop(&pVCpu->cpum.GstCtx.XState.x87, 0); \
        pVCpu->cpum.GstCtx.XState.x87.FSW &= ~X86_FSW_TOP_MASK; \
        pVCpu->cpum.GstCtx.XState.x87.FTW  = 0; \
    } while (0)

#define IEM_MC_FETCH_MREG_U64(a_u64Value, a_iMReg) \
    do { (a_u64Value) = pVCpu->cpum.GstCtx.XState.x87.aRegs[(a_iMReg)].mmx; } while (0)
#define IEM_MC_FETCH_MREG_U32(a_u32Value, a_iMReg) \
    do { (a_u32Value) = pVCpu->cpum.GstCtx.XState.x87.aRegs[(a_iMReg)].au32[0]; } while (0)
#define IEM_MC_STORE_MREG_U64(a_iMReg, a_u64Value) do { \
        pVCpu->cpum.GstCtx.XState.x87.aRegs[(a_iMReg)].mmx = (a_u64Value); \
        pVCpu->cpum.GstCtx.XState.x87.aRegs[(a_iMReg)].au32[2] = 0xffff; \
    } while (0)
#define IEM_MC_STORE_MREG_U32_ZX_U64(a_iMReg, a_u32Value) do { \
        pVCpu->cpum.GstCtx.XState.x87.aRegs[(a_iMReg)].mmx = (uint32_t)(a_u32Value); \
        pVCpu->cpum.GstCtx.XState.x87.aRegs[(a_iMReg)].au32[2] = 0xffff; \
    } while (0)
#define IEM_MC_REF_MREG_U64(a_pu64Dst, a_iMReg) /** @todo need to set high word to 0xffff on commit (see IEM_MC_STORE_MREG_U64) */ \
        (a_pu64Dst) = (&pVCpu->cpum.GstCtx.XState.x87.aRegs[(a_iMReg)].mmx)
#define IEM_MC_REF_MREG_U64_CONST(a_pu64Dst, a_iMReg) \
        (a_pu64Dst) = ((uint64_t const *)&pVCpu->cpum.GstCtx.XState.x87.aRegs[(a_iMReg)].mmx)
#define IEM_MC_REF_MREG_U32_CONST(a_pu32Dst, a_iMReg) \
        (a_pu32Dst) = ((uint32_t const *)&pVCpu->cpum.GstCtx.XState.x87.aRegs[(a_iMReg)].mmx)
#define IEM_MC_MODIFIED_MREG(a_iMReg) \
    do { pVCpu->cpum.GstCtx.XState.x87.aRegs[(a_iMReg)].au32[2] = 0xffff; } while (0)
#define IEM_MC_MODIFIED_MREG_BY_REF(a_pu64Dst) \
    do { ((uint32_t *)(a_pu64Dst))[2] = 0xffff; } while (0)

#define IEM_MC_CLEAR_XREG_U32_MASK(a_iXReg, a_bMask) \
    do { if ((a_bMask) & (1 << 0)) pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au32[0] = 0; \
         if ((a_bMask) & (1 << 1)) pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au32[1] = 0; \
         if ((a_bMask) & (1 << 2)) pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au32[2] = 0; \
         if ((a_bMask) & (1 << 3)) pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au32[3] = 0; \
    } while (0)
#define IEM_MC_FETCH_XREG_U128(a_u128Value, a_iXReg) \
    do { (a_u128Value).au64[0] = pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au64[0]; \
         (a_u128Value).au64[1] = pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au64[1]; \
    } while (0)
#define IEM_MC_FETCH_XREG_XMM(a_XmmValue, a_iXReg) \
    do { (a_XmmValue).au64[0] = pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au64[0]; \
         (a_XmmValue).au64[1] = pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au64[1]; \
    } while (0)
#define IEM_MC_FETCH_XREG_U64(a_u64Value, a_iXReg, a_iQWord) \
    do { (a_u64Value) = pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au64[(a_iQWord)]; } while (0)
#define IEM_MC_FETCH_XREG_U32(a_u32Value, a_iXReg, a_iDWord) \
    do { (a_u32Value) = pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au32[(a_iDWord)]; } while (0)
#define IEM_MC_FETCH_XREG_U16(a_u16Value, a_iXReg, a_iWord) \
    do { (a_u16Value) = pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au16[(a_iWord)]; } while (0)
#define IEM_MC_FETCH_XREG_U8( a_u8Value,  a_iXReg, a_iByte) \
    do { (a_u8Value)  = pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au16[(a_iByte)]; } while (0)
#define IEM_MC_STORE_XREG_U128(a_iXReg, a_u128Value) \
    do { pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au64[0] = (a_u128Value).au64[0]; \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au64[1] = (a_u128Value).au64[1]; \
    } while (0)
#define IEM_MC_STORE_XREG_XMM(a_iXReg, a_XmmValue) \
    do { pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au64[0] = (a_XmmValue).au64[0]; \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au64[1] = (a_XmmValue).au64[1]; \
    } while (0)
#define IEM_MC_STORE_XREG_XMM_U32(a_iXReg, a_iDword, a_XmmValue) \
    do { pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au32[(a_iDword)] = (a_XmmValue).au32[(a_iDword)]; } while (0)
#define IEM_MC_STORE_XREG_XMM_U64(a_iXReg, a_iQword, a_XmmValue) \
    do { pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au64[(a_iQword)] = (a_XmmValue).au64[(a_iQword)]; } while (0)
#define IEM_MC_STORE_XREG_U64(a_iXReg, a_iQword, a_u64Value) \
    do { pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au64[(a_iQword)] = (a_u64Value); } while (0)
#define IEM_MC_STORE_XREG_U32(a_iXReg, a_iDword, a_u32Value) \
    do { pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au32[(a_iDword)] = (a_u32Value); } while (0)
#define IEM_MC_STORE_XREG_U16(a_iXReg, a_iWord, a_u16Value) \
    do { pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au32[(a_iWord)]  = (a_u16Value); } while (0)
#define IEM_MC_STORE_XREG_U8(a_iXReg,  a_iByte, a_u8Value) \
    do { pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au32[(a_iByte)]  = (a_u8Value);  } while (0)

#define IEM_MC_STORE_XREG_U64_ZX_U128(a_iXReg, a_u64Value) \
    do { pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au64[0] = (a_u64Value); \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au64[1] = 0; \
    } while (0)

#define IEM_MC_STORE_XREG_U32_U128(a_iXReg, a_iDwDst, a_u128Value, a_iDwSrc) \
    do { pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au32[(a_iDwDst)] = (a_u128Value).au32[(a_iDwSrc)]; } while (0)
#define IEM_MC_STORE_XREG_R32(a_iXReg, a_r32Value) \
    do { pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].ar32[0] = (a_r32Value); } while (0)
#define IEM_MC_STORE_XREG_R64(a_iXReg, a_r64Value) \
    do { pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].ar64[0] = (a_r64Value); } while (0)
#define IEM_MC_STORE_XREG_U32_ZX_U128(a_iXReg, a_u32Value) \
    do { pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au64[0] = (uint32_t)(a_u32Value); \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au64[1] = 0; \
    } while (0)
#define IEM_MC_STORE_XREG_HI_U64(a_iXReg, a_u64Value) \
    do { pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au64[1] = (a_u64Value); } while (0)
#define IEM_MC_REF_XREG_U128(a_pu128Dst, a_iXReg)       \
    (a_pu128Dst) = (&pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].uXmm)
#define IEM_MC_REF_XREG_U128_CONST(a_pu128Dst, a_iXReg) \
    (a_pu128Dst) = ((PCRTUINT128U)&pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].uXmm)
#define IEM_MC_REF_XREG_XMM_CONST(a_pXmmDst, a_iXReg) \
    (a_pXmmDst) = (&pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)])
#define IEM_MC_REF_XREG_U32_CONST(a_pu32Dst, a_iXReg) \
    (a_pu32Dst) = ((uint32_t const *)&pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au32[0])
#define IEM_MC_REF_XREG_U64_CONST(a_pu64Dst, a_iXReg) \
    (a_pu64Dst) = ((uint64_t const *)&pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].au64[0])
#define IEM_MC_REF_XREG_R32_CONST(a_pr32Dst, a_iXReg) \
    (a_pr32Dst) = ((RTFLOAT32U const *)&pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].ar32[0])
#define IEM_MC_REF_XREG_R64_CONST(a_pr64Dst, a_iXReg) \
    (a_pr64Dst) = ((RTFLOAT64U const *)&pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXReg)].ar64[0])
#define IEM_MC_COPY_XREG_U128(a_iXRegDst, a_iXRegSrc) \
    do { pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXRegDst)].au64[0] \
            = pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXRegSrc)].au64[0]; \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXRegDst)].au64[1] \
            = pVCpu->cpum.GstCtx.XState.x87.aXMM[(a_iXRegSrc)].au64[1]; \
    } while (0)

#define IEM_MC_FETCH_YREG_U32(a_u32Dst, a_iYRegSrc) \
    do { uintptr_t const iYRegSrcTmp    = (a_iYRegSrc); \
         (a_u32Dst) = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrcTmp].au32[0]; \
    } while (0)
#define IEM_MC_FETCH_YREG_U64(a_u64Dst, a_iYRegSrc) \
    do { uintptr_t const iYRegSrcTmp    = (a_iYRegSrc); \
         (a_u64Dst) = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrcTmp].au64[0]; \
    } while (0)
#define IEM_MC_FETCH_YREG_2ND_U64(a_u64Dst, a_iYRegSrc) \
    do { uintptr_t const iYRegSrcTmp    = (a_iYRegSrc); \
         (a_u64Dst) = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrcTmp].au64[1]; \
    } while (0)
#define IEM_MC_FETCH_YREG_U128(a_u128Dst, a_iYRegSrc) \
    do { uintptr_t const iYRegSrcTmp    = (a_iYRegSrc); \
         (a_u128Dst).au64[0] = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrcTmp].au64[0]; \
         (a_u128Dst).au64[1] = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrcTmp].au64[1]; \
    } while (0)
#define IEM_MC_FETCH_YREG_U256(a_u256Dst, a_iYRegSrc) \
    do { uintptr_t const iYRegSrcTmp    = (a_iYRegSrc); \
         (a_u256Dst).au64[0] = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrcTmp].au64[0]; \
         (a_u256Dst).au64[1] = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrcTmp].au64[1]; \
         (a_u256Dst).au64[2] = pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegSrcTmp].au64[0]; \
         (a_u256Dst).au64[3] = pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegSrcTmp].au64[1]; \
    } while (0)

#define IEM_MC_INT_CLEAR_ZMM_256_UP(a_iXRegDst) do { /* For AVX512 and AVX1024 support. */ } while (0)
#define IEM_MC_STORE_YREG_U32_ZX_VLMAX(a_iYRegDst, a_u32Src) \
    do { uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au32[0]       = (a_u32Src); \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au32[1]       = 0; \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[1]       = 0; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = 0; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(iYRegDstTmp); \
    } while (0)
#define IEM_MC_STORE_YREG_U64_ZX_VLMAX(a_iYRegDst, a_u64Src) \
    do { uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[0]       = (a_u64Src); \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[1]       = 0; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = 0; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(iYRegDstTmp); \
    } while (0)
#define IEM_MC_STORE_YREG_U128_ZX_VLMAX(a_iYRegDst, a_u128Src) \
    do { uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[0]       = (a_u128Src).au64[0]; \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[1]       = (a_u128Src).au64[1]; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = 0; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(iYRegDstTmp); \
    } while (0)
#define IEM_MC_STORE_YREG_U256_ZX_VLMAX(a_iYRegDst, a_u256Src) \
    do { uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[0]       = (a_u256Src).au64[0]; \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[1]       = (a_u256Src).au64[1]; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = (a_u256Src).au64[2]; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = (a_u256Src).au64[3]; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(iYRegDstTmp); \
    } while (0)

#define IEM_MC_REF_YREG_U128(a_pu128Dst, a_iYReg)       \
    (a_pu128Dst) = (&pVCpu->cpum.GstCtx.XState.x87.aYMM[(a_iYReg)].uXmm)
#define IEM_MC_REF_YREG_U128_CONST(a_pu128Dst, a_iYReg) \
    (a_pu128Dst) = ((PCRTUINT128U)&pVCpu->cpum.GstCtx.XState.x87.aYMM[(a_iYReg)].uXmm)
#define IEM_MC_REF_YREG_U64_CONST(a_pu64Dst, a_iYReg) \
    (a_pu64Dst) = ((uint64_t const *)&pVCpu->cpum.GstCtx.XState.x87.aYMM[(a_iYReg)].au64[0])
#define IEM_MC_CLEAR_YREG_128_UP(a_iYReg) \
    do { uintptr_t const iYRegTmp   = (a_iYReg); \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegTmp].au64[0] = 0; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(iYRegTmp); \
    } while (0)

#define IEM_MC_COPY_YREG_U256_ZX_VLMAX(a_iYRegDst, a_iYRegSrc) \
    do { uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         uintptr_t const iYRegSrcTmp    = (a_iYRegSrc); \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[0]       = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrcTmp].au64[0]; \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[1]       = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrcTmp].au64[1]; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegSrcTmp].au64[0]; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegSrcTmp].au64[1]; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(iYRegDstTmp); \
    } while (0)
#define IEM_MC_COPY_YREG_U128_ZX_VLMAX(a_iYRegDst, a_iYRegSrc) \
    do { uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         uintptr_t const iYRegSrcTmp    = (a_iYRegSrc); \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[0]       = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrcTmp].au64[0]; \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[1]       = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrcTmp].au64[1]; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = 0; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(iYRegDstTmp); \
    } while (0)
#define IEM_MC_COPY_YREG_U64_ZX_VLMAX(a_iYRegDst, a_iYRegSrc) \
    do { uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         uintptr_t const iYRegSrcTmp    = (a_iYRegSrc); \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[0]       = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrcTmp].au64[0]; \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[1]       = 0; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = 0; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(iYRegDstTmp); \
    } while (0)

#define IEM_MC_MERGE_YREG_U32_U96_ZX_VLMAX(a_iYRegDst, a_iYRegSrc32, a_iYRegSrcHx) \
    do { uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         uintptr_t const iYRegSrc32Tmp  = (a_iYRegSrc32); \
         uintptr_t const iYRegSrcHxTmp  = (a_iYRegSrcHx); \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au32[0]       = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrc32Tmp].au32[0]; \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au32[1]       = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrcHxTmp].au32[1]; \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[1]       = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrcHxTmp].au64[1]; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = 0; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(iYRegDstTmp); \
    } while (0)
#define IEM_MC_MERGE_YREG_U64_U64_ZX_VLMAX(a_iYRegDst, a_iYRegSrc64, a_iYRegSrcHx) \
    do { uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         uintptr_t const iYRegSrc64Tmp  = (a_iYRegSrc64); \
         uintptr_t const iYRegSrcHxTmp  = (a_iYRegSrcHx); \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[0]       = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrc64Tmp].au64[0]; \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[1]       = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrcHxTmp].au64[1]; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = 0; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(iYRegDstTmp); \
    } while (0)
#define IEM_MC_MERGE_YREG_U64LO_U64LO_ZX_VLMAX(a_iYRegDst, a_iYRegSrc64, a_iYRegSrcHx) /* for vmovhlps */ \
    do { uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         uintptr_t const iYRegSrc64Tmp  = (a_iYRegSrc64); \
         uintptr_t const iYRegSrcHxTmp  = (a_iYRegSrcHx); \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[0]       = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrc64Tmp].au64[0]; \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[1]       = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrcHxTmp].au64[0]; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = 0; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(iYRegDstTmp); \
    } while (0)
#define IEM_MC_MERGE_YREG_U64HI_U64HI_ZX_VLMAX(a_iYRegDst, a_iYRegSrc64, a_iYRegSrcHx) /* for vmovhlps */ \
    do { uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         uintptr_t const iYRegSrc64Tmp  = (a_iYRegSrc64); \
         uintptr_t const iYRegSrcHxTmp  = (a_iYRegSrcHx); \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[0]       = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrc64Tmp].au64[1]; \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[1]       = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrcHxTmp].au64[1]; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = 0; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(iYRegDstTmp); \
    } while (0)
#define IEM_MC_MERGE_YREG_U64LO_U64LOCAL_ZX_VLMAX(a_iYRegDst, a_iYRegSrcHx, a_u64Local) \
    do { uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         uintptr_t const iYRegSrcHxTmp  = (a_iYRegSrcHx); \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[0]       = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrcHxTmp].au64[0]; \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[1]       = (a_u64Local); \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = 0; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(iYRegDstTmp); \
    } while (0)
#define IEM_MC_MERGE_YREG_U64LOCAL_U64HI_ZX_VLMAX(a_iYRegDst, a_u64Local, a_iYRegSrcHx) \
    do { uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         uintptr_t const iYRegSrcHxTmp  = (a_iYRegSrcHx); \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[0]       = (a_u64Local); \
         pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegDstTmp].au64[1]       = pVCpu->cpum.GstCtx.XState.x87.aXMM[iYRegSrcHxTmp].au64[1]; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = 0; \
         pVCpu->cpum.GstCtx.XState.u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(iYRegDstTmp); \
    } while (0)

#ifndef IEM_WITH_SETJMP
# define IEM_MC_FETCH_MEM_U8(a_u8Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU8(pVCpu, &(a_u8Dst), (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM16_U8(a_u8Dst, a_iSeg, a_GCPtrMem16) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU8(pVCpu, &(a_u8Dst), (a_iSeg), (a_GCPtrMem16)))
# define IEM_MC_FETCH_MEM32_U8(a_u8Dst, a_iSeg, a_GCPtrMem32) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU8(pVCpu, &(a_u8Dst), (a_iSeg), (a_GCPtrMem32)))
#else
# define IEM_MC_FETCH_MEM_U8(a_u8Dst, a_iSeg, a_GCPtrMem) \
    ((a_u8Dst) = iemMemFetchDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM16_U8(a_u8Dst, a_iSeg, a_GCPtrMem16) \
    ((a_u8Dst) = iemMemFetchDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem16)))
# define IEM_MC_FETCH_MEM32_U8(a_u8Dst, a_iSeg, a_GCPtrMem32) \
    ((a_u8Dst) = iemMemFetchDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem32)))
#endif

#ifndef IEM_WITH_SETJMP
# define IEM_MC_FETCH_MEM_U16(a_u16Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU16(pVCpu, &(a_u16Dst), (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U16_DISP(a_u16Dst, a_iSeg, a_GCPtrMem, a_offDisp) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU16(pVCpu, &(a_u16Dst), (a_iSeg), (a_GCPtrMem) + (a_offDisp)))
# define IEM_MC_FETCH_MEM_I16(a_i16Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU16(pVCpu, (uint16_t *)&(a_i16Dst), (a_iSeg), (a_GCPtrMem)))
#else
# define IEM_MC_FETCH_MEM_U16(a_u16Dst, a_iSeg, a_GCPtrMem) \
    ((a_u16Dst) = iemMemFetchDataU16Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U16_DISP(a_u16Dst, a_iSeg, a_GCPtrMem, a_offDisp) \
    ((a_u16Dst) = iemMemFetchDataU16Jmp(pVCpu, (a_iSeg), (a_GCPtrMem) + (a_offDisp)))
# define IEM_MC_FETCH_MEM_I16(a_i16Dst, a_iSeg, a_GCPtrMem) \
    ((a_i16Dst) = (int16_t)iemMemFetchDataU16Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
#endif

#ifndef IEM_WITH_SETJMP
# define IEM_MC_FETCH_MEM_U32(a_u32Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU32(pVCpu, &(a_u32Dst), (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U32_DISP(a_u32Dst, a_iSeg, a_GCPtrMem, a_offDisp) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU32(pVCpu, &(a_u32Dst), (a_iSeg), (a_GCPtrMem) + (a_offDisp)))
# define IEM_MC_FETCH_MEM_I32(a_i32Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU32(pVCpu, (uint32_t *)&(a_i32Dst), (a_iSeg), (a_GCPtrMem)))
#else
# define IEM_MC_FETCH_MEM_U32(a_u32Dst, a_iSeg, a_GCPtrMem) \
    ((a_u32Dst) = iemMemFetchDataU32Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U32_DISP(a_u32Dst, a_iSeg, a_GCPtrMem, a_offDisp) \
    ((a_u32Dst) = iemMemFetchDataU32Jmp(pVCpu, (a_iSeg), (a_GCPtrMem) + (a_offDisp)))
# define IEM_MC_FETCH_MEM_I32(a_i32Dst, a_iSeg, a_GCPtrMem) \
    ((a_i32Dst) = (int32_t)iemMemFetchDataU32Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
#endif

#ifdef SOME_UNUSED_FUNCTION
# define IEM_MC_FETCH_MEM_S32_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataS32SxU64(pVCpu, &(a_u64Dst), (a_iSeg), (a_GCPtrMem)))
#endif

#ifndef IEM_WITH_SETJMP
# define IEM_MC_FETCH_MEM_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU64(pVCpu, &(a_u64Dst), (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U64_DISP(a_u64Dst, a_iSeg, a_GCPtrMem, a_offDisp) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU64(pVCpu, &(a_u64Dst), (a_iSeg), (a_GCPtrMem) + (a_offDisp)))
# define IEM_MC_FETCH_MEM_U64_ALIGN_U128(a_u64Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU64AlignedU128(pVCpu, &(a_u64Dst), (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_I64(a_i64Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU64(pVCpu, (uint64_t *)&(a_i64Dst), (a_iSeg), (a_GCPtrMem)))
#else
# define IEM_MC_FETCH_MEM_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    ((a_u64Dst) = iemMemFetchDataU64Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U64_DISP(a_u64Dst, a_iSeg, a_GCPtrMem, a_offDisp) \
    ((a_u64Dst) = iemMemFetchDataU64Jmp(pVCpu, (a_iSeg), (a_GCPtrMem) + (a_offDisp)))
# define IEM_MC_FETCH_MEM_U64_ALIGN_U128(a_u64Dst, a_iSeg, a_GCPtrMem) \
    ((a_u64Dst) = iemMemFetchDataU64AlignedU128Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_I64(a_i64Dst, a_iSeg, a_GCPtrMem) \
    ((a_i64Dst) = (int64_t)iemMemFetchDataU64Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
#endif

#ifndef IEM_WITH_SETJMP
# define IEM_MC_FETCH_MEM_R32(a_r32Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU32(pVCpu, &(a_r32Dst).u, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_R64(a_r64Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU64(pVCpu, &(a_r64Dst).u, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_R80(a_r80Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataR80(pVCpu, &(a_r80Dst), (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_D80(a_d80Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataD80(pVCpu, &(a_d80Dst), (a_iSeg), (a_GCPtrMem)))
#else
# define IEM_MC_FETCH_MEM_R32(a_r32Dst, a_iSeg, a_GCPtrMem) \
    ((a_r32Dst).u = iemMemFetchDataU32Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_R64(a_r64Dst, a_iSeg, a_GCPtrMem) \
    ((a_r64Dst).u = iemMemFetchDataU64Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_R80(a_r80Dst, a_iSeg, a_GCPtrMem) \
    iemMemFetchDataR80Jmp(pVCpu, &(a_r80Dst), (a_iSeg), (a_GCPtrMem))
# define IEM_MC_FETCH_MEM_D80(a_d80Dst, a_iSeg, a_GCPtrMem) \
    iemMemFetchDataD80Jmp(pVCpu, &(a_d80Dst), (a_iSeg), (a_GCPtrMem))
#endif

#ifndef IEM_WITH_SETJMP
# define IEM_MC_FETCH_MEM_U128(a_u128Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU128(pVCpu, &(a_u128Dst), (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U128_NO_AC(a_u128Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU128(pVCpu, &(a_u128Dst), (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U128_ALIGN_SSE(a_u128Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU128AlignedSse(pVCpu, &(a_u128Dst), (a_iSeg), (a_GCPtrMem)))

# define IEM_MC_FETCH_MEM_XMM(a_XmmDst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU128(pVCpu, &(a_XmmDst).uXmm, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_XMM_NO_AC(a_XmmDst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU128(pVCpu, &(a_XmmDst).uXmm, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_XMM_ALIGN_SSE(a_XmmDst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU128AlignedSse(pVCpu, &(a_XmmDst).uXmm, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_XMM_U32(a_XmmDst, a_iDWord, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU32(pVCpu, &(a_XmmDst).au32[(a_iDWord)], (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_XMM_U64(a_XmmDst, a_iQWord, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU64(pVCpu, &(a_XmmDst).au64[(a_iQWord)], (a_iSeg), (a_GCPtrMem)))
#else
# define IEM_MC_FETCH_MEM_U128(a_u128Dst, a_iSeg, a_GCPtrMem) \
    iemMemFetchDataU128Jmp(pVCpu, &(a_u128Dst), (a_iSeg), (a_GCPtrMem))
# define IEM_MC_FETCH_MEM_U128_NO_AC(a_u128Dst, a_iSeg, a_GCPtrMem) \
    iemMemFetchDataU128Jmp(pVCpu, &(a_u128Dst), (a_iSeg), (a_GCPtrMem))
# define IEM_MC_FETCH_MEM_U128_ALIGN_SSE(a_u128Dst, a_iSeg, a_GCPtrMem) \
    iemMemFetchDataU128AlignedSseJmp(pVCpu, &(a_u128Dst), (a_iSeg), (a_GCPtrMem))

# define IEM_MC_FETCH_MEM_XMM(a_XmmDst, a_iSeg, a_GCPtrMem) \
    iemMemFetchDataU128Jmp(pVCpu, &(a_XmmDst).uXmm, (a_iSeg), (a_GCPtrMem))
# define IEM_MC_FETCH_MEM_XMM_NO_AC(a_XmmDst, a_iSeg, a_GCPtrMem) \
    iemMemFetchDataU128Jmp(pVCpu, &(a_XmmDst).uXmm, (a_iSeg), (a_GCPtrMem))
# define IEM_MC_FETCH_MEM_XMM_ALIGN_SSE(a_XmmDst, a_iSeg, a_GCPtrMem) \
    iemMemFetchDataU128AlignedSseJmp(pVCpu, &(a_XmmDst).uXmm, (a_iSeg), (a_GCPtrMem))
# define IEM_MC_FETCH_MEM_XMM_U32(a_XmmDst, a_iDWord, a_iSeg, a_GCPtrMem) \
    (a_XmmDst).au32[(a_iDWord)] = iemMemFetchDataU32Jmp(pVCpu, (a_iSeg), (a_GCPtrMem))
# define IEM_MC_FETCH_MEM_XMM_U64(a_XmmDst, a_iQWord, a_iSeg, a_GCPtrMem) \
    (a_XmmDst).au64[(a_iQWord)] = iemMemFetchDataU64Jmp(pVCpu, (a_iSeg), (a_GCPtrMem))
#endif

#ifndef IEM_WITH_SETJMP
# define IEM_MC_FETCH_MEM_U256(a_u256Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU256(pVCpu, &(a_u256Dst), (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U256_NO_AC(a_u256Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU256(pVCpu, &(a_u256Dst), (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U256_ALIGN_AVX(a_u256Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU256AlignedSse(pVCpu, &(a_u256Dst), (a_iSeg), (a_GCPtrMem)))

# define IEM_MC_FETCH_MEM_YMM(a_YmmDst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU256(pVCpu, &(a_YmmDst).ymm, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_YMM_NO_AC(a_YmmDst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU256(pVCpu, &(a_YmmDst).ymm, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_YMM_ALIGN_AVX(a_YmmDst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU256AlignedSse(pVCpu, &(a_YmmDst).ymm, (a_iSeg), (a_GCPtrMem)))
#else
# define IEM_MC_FETCH_MEM_U256(a_u256Dst, a_iSeg, a_GCPtrMem) \
    iemMemFetchDataU256Jmp(pVCpu, &(a_u256Dst), (a_iSeg), (a_GCPtrMem))
# define IEM_MC_FETCH_MEM_U256_NO_AC(a_u256Dst, a_iSeg, a_GCPtrMem) \
    iemMemFetchDataU256Jmp(pVCpu, &(a_u256Dst), (a_iSeg), (a_GCPtrMem))
# define IEM_MC_FETCH_MEM_U256_ALIGN_AVX(a_u256Dst, a_iSeg, a_GCPtrMem) \
    iemMemFetchDataU256AlignedSseJmp(pVCpu, &(a_u256Dst), (a_iSeg), (a_GCPtrMem))

# define IEM_MC_FETCH_MEM_YMM(a_YmmDst, a_iSeg, a_GCPtrMem) \
    iemMemFetchDataU256Jmp(pVCpu, &(a_YmmDst).ymm, (a_iSeg), (a_GCPtrMem))
# define IEM_MC_FETCH_MEM_YMM_NO_AC(a_YmmDst, a_iSeg, a_GCPtrMem) \
    iemMemFetchDataU256Jmp(pVCpu, &(a_YmmDst).ymm, (a_iSeg), (a_GCPtrMem))
# define IEM_MC_FETCH_MEM_YMM_ALIGN_AVX(a_YmmDst, a_iSeg, a_GCPtrMem) \
    iemMemFetchDataU256AlignedSseJmp(pVCpu, &(a_YmmDst).ymm, (a_iSeg), (a_GCPtrMem))
#endif



#ifndef IEM_WITH_SETJMP
# define IEM_MC_FETCH_MEM_U8_ZX_U16(a_u16Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint8_t u8Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU8(pVCpu, &u8Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u16Dst) = u8Tmp; \
    } while (0)
# define IEM_MC_FETCH_MEM_U8_ZX_U32(a_u32Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint8_t u8Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU8(pVCpu, &u8Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u32Dst) = u8Tmp; \
    } while (0)
# define IEM_MC_FETCH_MEM_U8_ZX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint8_t u8Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU8(pVCpu, &u8Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u64Dst) = u8Tmp; \
    } while (0)
# define IEM_MC_FETCH_MEM_U16_ZX_U32(a_u32Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint16_t u16Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU16(pVCpu, &u16Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u32Dst) = u16Tmp; \
    } while (0)
# define IEM_MC_FETCH_MEM_U16_ZX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint16_t u16Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU16(pVCpu, &u16Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u64Dst) = u16Tmp; \
    } while (0)
# define IEM_MC_FETCH_MEM_U32_ZX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint32_t u32Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU32(pVCpu, &u32Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u64Dst) = u32Tmp; \
    } while (0)
#else  /* IEM_WITH_SETJMP */
# define IEM_MC_FETCH_MEM_U8_ZX_U16(a_u16Dst, a_iSeg, a_GCPtrMem) \
    ((a_u16Dst) = iemMemFetchDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U8_ZX_U32(a_u32Dst, a_iSeg, a_GCPtrMem) \
    ((a_u32Dst) = iemMemFetchDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U8_ZX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    ((a_u64Dst) = iemMemFetchDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U16_ZX_U32(a_u32Dst, a_iSeg, a_GCPtrMem) \
    ((a_u32Dst) = iemMemFetchDataU16Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U16_ZX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    ((a_u64Dst) = iemMemFetchDataU16Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U32_ZX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    ((a_u64Dst) = iemMemFetchDataU32Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
#endif /* IEM_WITH_SETJMP */

#ifndef IEM_WITH_SETJMP
# define IEM_MC_FETCH_MEM_U8_SX_U16(a_u16Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint8_t u8Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU8(pVCpu, &u8Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u16Dst) = (int8_t)u8Tmp; \
    } while (0)
# define IEM_MC_FETCH_MEM_U8_SX_U32(a_u32Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint8_t u8Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU8(pVCpu, &u8Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u32Dst) = (int8_t)u8Tmp; \
    } while (0)
# define IEM_MC_FETCH_MEM_U8_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint8_t u8Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU8(pVCpu, &u8Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u64Dst) = (int8_t)u8Tmp; \
    } while (0)
# define IEM_MC_FETCH_MEM_U16_SX_U32(a_u32Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint16_t u16Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU16(pVCpu, &u16Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u32Dst) = (int16_t)u16Tmp; \
    } while (0)
# define IEM_MC_FETCH_MEM_U16_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint16_t u16Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU16(pVCpu, &u16Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u64Dst) = (int16_t)u16Tmp; \
    } while (0)
# define IEM_MC_FETCH_MEM_U32_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint32_t u32Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU32(pVCpu, &u32Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u64Dst) = (int32_t)u32Tmp; \
    } while (0)
#else  /* IEM_WITH_SETJMP */
# define IEM_MC_FETCH_MEM_U8_SX_U16(a_u16Dst, a_iSeg, a_GCPtrMem) \
    ((a_u16Dst) = (int8_t)iemMemFetchDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U8_SX_U32(a_u32Dst, a_iSeg, a_GCPtrMem) \
    ((a_u32Dst) = (int8_t)iemMemFetchDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U8_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    ((a_u64Dst) = (int8_t)iemMemFetchDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U16_SX_U32(a_u32Dst, a_iSeg, a_GCPtrMem) \
    ((a_u32Dst) = (int16_t)iemMemFetchDataU16Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U16_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    ((a_u64Dst) = (int16_t)iemMemFetchDataU16Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U32_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    ((a_u64Dst) = (int32_t)iemMemFetchDataU32Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
#endif /* IEM_WITH_SETJMP */

#ifndef IEM_WITH_SETJMP
# define IEM_MC_STORE_MEM_U8(a_iSeg, a_GCPtrMem, a_u8Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU8(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u8Value)))
# define IEM_MC_STORE_MEM_U16(a_iSeg, a_GCPtrMem, a_u16Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU16(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u16Value)))
# define IEM_MC_STORE_MEM_U32(a_iSeg, a_GCPtrMem, a_u32Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU32(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u32Value)))
# define IEM_MC_STORE_MEM_U64(a_iSeg, a_GCPtrMem, a_u64Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU64(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u64Value)))
#else
# define IEM_MC_STORE_MEM_U8(a_iSeg, a_GCPtrMem, a_u8Value) \
    iemMemStoreDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u8Value))
# define IEM_MC_STORE_MEM_U16(a_iSeg, a_GCPtrMem, a_u16Value) \
    iemMemStoreDataU16Jmp(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u16Value))
# define IEM_MC_STORE_MEM_U32(a_iSeg, a_GCPtrMem, a_u32Value) \
    iemMemStoreDataU32Jmp(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u32Value))
# define IEM_MC_STORE_MEM_U64(a_iSeg, a_GCPtrMem, a_u64Value) \
    iemMemStoreDataU64Jmp(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u64Value))
#endif

#ifndef IEM_WITH_SETJMP
# define IEM_MC_STORE_MEM_U8_CONST(a_iSeg, a_GCPtrMem, a_u8C) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU8(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u8C)))
# define IEM_MC_STORE_MEM_U16_CONST(a_iSeg, a_GCPtrMem, a_u16C) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU16(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u16C)))
# define IEM_MC_STORE_MEM_U32_CONST(a_iSeg, a_GCPtrMem, a_u32C) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU32(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u32C)))
# define IEM_MC_STORE_MEM_U64_CONST(a_iSeg, a_GCPtrMem, a_u64C) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU64(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u64C)))
#else
# define IEM_MC_STORE_MEM_U8_CONST(a_iSeg, a_GCPtrMem, a_u8C) \
    iemMemStoreDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u8C))
# define IEM_MC_STORE_MEM_U16_CONST(a_iSeg, a_GCPtrMem, a_u16C) \
    iemMemStoreDataU16Jmp(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u16C))
# define IEM_MC_STORE_MEM_U32_CONST(a_iSeg, a_GCPtrMem, a_u32C) \
    iemMemStoreDataU32Jmp(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u32C))
# define IEM_MC_STORE_MEM_U64_CONST(a_iSeg, a_GCPtrMem, a_u64C) \
    iemMemStoreDataU64Jmp(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u64C))
#endif

#define IEM_MC_STORE_MEM_I8_CONST_BY_REF( a_pi8Dst,  a_i8C)     *(a_pi8Dst)  = (a_i8C)
#define IEM_MC_STORE_MEM_I16_CONST_BY_REF(a_pi16Dst, a_i16C)    *(a_pi16Dst) = (a_i16C)
#define IEM_MC_STORE_MEM_I32_CONST_BY_REF(a_pi32Dst, a_i32C)    *(a_pi32Dst) = (a_i32C)
#define IEM_MC_STORE_MEM_I64_CONST_BY_REF(a_pi64Dst, a_i64C)    *(a_pi64Dst) = (a_i64C)
#define IEM_MC_STORE_MEM_NEG_QNAN_R32_BY_REF(a_pr32Dst)         (a_pr32Dst)->u = UINT32_C(0xffc00000)
#define IEM_MC_STORE_MEM_NEG_QNAN_R64_BY_REF(a_pr64Dst)         (a_pr64Dst)->u = UINT64_C(0xfff8000000000000)
#define IEM_MC_STORE_MEM_NEG_QNAN_R80_BY_REF(a_pr80Dst) \
    do { \
        (a_pr80Dst)->au64[0] = UINT64_C(0xc000000000000000); \
        (a_pr80Dst)->au16[4] = UINT16_C(0xffff); \
    } while (0)
#define IEM_MC_STORE_MEM_INDEF_D80_BY_REF(a_pd80Dst) \
    do { \
        (a_pd80Dst)->au64[0] = UINT64_C(0xc000000000000000); \
        (a_pd80Dst)->au16[4] = UINT16_C(0xffff); \
    } while (0)

#ifndef IEM_WITH_SETJMP
# define IEM_MC_STORE_MEM_U128(a_iSeg, a_GCPtrMem, a_u128Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU128(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u128Value)))
# define IEM_MC_STORE_MEM_U128_ALIGN_SSE(a_iSeg, a_GCPtrMem, a_u128Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU128AlignedSse(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u128Value)))
#else
# define IEM_MC_STORE_MEM_U128(a_iSeg, a_GCPtrMem, a_u128Value) \
    iemMemStoreDataU128Jmp(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u128Value))
# define IEM_MC_STORE_MEM_U128_ALIGN_SSE(a_iSeg, a_GCPtrMem, a_u128Value) \
    iemMemStoreDataU128AlignedSseJmp(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u128Value))
#endif

#ifndef IEM_WITH_SETJMP
# define IEM_MC_STORE_MEM_U256(a_iSeg, a_GCPtrMem, a_u256Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU256(pVCpu, (a_iSeg), (a_GCPtrMem), &(a_u256Value)))
# define IEM_MC_STORE_MEM_U256_ALIGN_AVX(a_iSeg, a_GCPtrMem, a_u256Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU256AlignedAvx(pVCpu, (a_iSeg), (a_GCPtrMem), &(a_u256Value)))
#else
# define IEM_MC_STORE_MEM_U256(a_iSeg, a_GCPtrMem, a_u256Value) \
    iemMemStoreDataU256Jmp(pVCpu, (a_iSeg), (a_GCPtrMem), &(a_u256Value))
# define IEM_MC_STORE_MEM_U256_ALIGN_AVX(a_iSeg, a_GCPtrMem, a_u256Value) \
    iemMemStoreDataU256AlignedAvxJmp(pVCpu, (a_iSeg), (a_GCPtrMem), &(a_u256Value))
#endif


#define IEM_MC_PUSH_U16(a_u16Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStackPushU16(pVCpu, (a_u16Value)))
#define IEM_MC_PUSH_U32(a_u32Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStackPushU32(pVCpu, (a_u32Value)))
#define IEM_MC_PUSH_U32_SREG(a_u32Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStackPushU32SReg(pVCpu, (a_u32Value)))
#define IEM_MC_PUSH_U64(a_u64Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStackPushU64(pVCpu, (a_u64Value)))

#define IEM_MC_POP_U16(a_pu16Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStackPopU16(pVCpu, (a_pu16Value)))
#define IEM_MC_POP_U32(a_pu32Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStackPopU32(pVCpu, (a_pu32Value)))
#define IEM_MC_POP_U64(a_pu64Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStackPopU64(pVCpu, (a_pu64Value)))

/** Maps guest memory for direct or bounce buffered access.
 * The purpose is to pass it to an operand implementation, thus the a_iArg.
 * @remarks     May return.
 */
#define IEM_MC_MEM_MAP(a_pMem, a_fAccess, a_iSeg, a_GCPtrMem, a_iArg) \
    IEM_MC_RETURN_ON_FAILURE(iemMemMap(pVCpu, (void **)&(a_pMem), sizeof(*(a_pMem)), (a_iSeg), \
                                       (a_GCPtrMem), (a_fAccess), sizeof(*(a_pMem)) - 1))

/** Maps guest memory for direct or bounce buffered access.
 * The purpose is to pass it to an operand implementation, thus the a_iArg.
 * @remarks     May return.
 */
#define IEM_MC_MEM_MAP_EX(a_pvMem, a_fAccess, a_cbMem, a_iSeg, a_GCPtrMem, a_cbAlign, a_iArg) \
    IEM_MC_RETURN_ON_FAILURE(iemMemMap(pVCpu, (void **)&(a_pvMem), (a_cbMem), (a_iSeg), \
                                       (a_GCPtrMem), (a_fAccess), (a_cbAlign)))

/** Commits the memory and unmaps the guest memory.
 * @remarks     May return.
 */
#define IEM_MC_MEM_COMMIT_AND_UNMAP(a_pvMem, a_fAccess) \
    IEM_MC_RETURN_ON_FAILURE(iemMemCommitAndUnmap(pVCpu, (a_pvMem), (a_fAccess)))

/** Commits the memory and unmaps the guest memory unless the FPU status word
 * indicates (@a a_u16FSW) and FPU control word indicates a pending exception
 * that would cause FLD not to store.
 *
 * The current understanding is that \#O, \#U, \#IA and \#IS will prevent a
 * store, while \#P will not.
 *
 * @remarks     May in theory return - for now.
 */
#define IEM_MC_MEM_COMMIT_AND_UNMAP_FOR_FPU_STORE(a_pvMem, a_fAccess, a_u16FSW) \
    do { \
        if (   !(a_u16FSW & X86_FSW_ES) \
            || !(  (a_u16FSW & (X86_FSW_UE | X86_FSW_OE | X86_FSW_IE)) \
                 & ~(pVCpu->cpum.GstCtx.XState.x87.FCW & X86_FCW_MASK_ALL) ) ) \
            IEM_MC_RETURN_ON_FAILURE(iemMemCommitAndUnmap(pVCpu, (a_pvMem), (a_fAccess))); \
    } while (0)

/** Calculate efficient address from R/M. */
#ifndef IEM_WITH_SETJMP
# define IEM_MC_CALC_RM_EFF_ADDR(a_GCPtrEff, bRm, cbImm) \
    IEM_MC_RETURN_ON_FAILURE(iemOpHlpCalcRmEffAddr(pVCpu, (bRm), (cbImm), &(a_GCPtrEff)))
#else
# define IEM_MC_CALC_RM_EFF_ADDR(a_GCPtrEff, bRm, cbImm) \
    ((a_GCPtrEff) = iemOpHlpCalcRmEffAddrJmp(pVCpu, (bRm), (cbImm)))
#endif

#define IEM_MC_CALL_VOID_AIMPL_0(a_pfn)                   (a_pfn)()
#define IEM_MC_CALL_VOID_AIMPL_1(a_pfn, a0)               (a_pfn)((a0))
#define IEM_MC_CALL_VOID_AIMPL_2(a_pfn, a0, a1)           (a_pfn)((a0), (a1))
#define IEM_MC_CALL_VOID_AIMPL_3(a_pfn, a0, a1, a2)       (a_pfn)((a0), (a1), (a2))
#define IEM_MC_CALL_VOID_AIMPL_4(a_pfn, a0, a1, a2, a3)   (a_pfn)((a0), (a1), (a2), (a3))
#define IEM_MC_CALL_AIMPL_3(a_rc, a_pfn, a0, a1, a2)      (a_rc) = (a_pfn)((a0), (a1), (a2))
#define IEM_MC_CALL_AIMPL_4(a_rc, a_pfn, a0, a1, a2, a3)  (a_rc) = (a_pfn)((a0), (a1), (a2), (a3))

/**
 * Defers the rest of the instruction emulation to a C implementation routine
 * and returns, only taking the standard parameters.
 *
 * @param   a_pfnCImpl      The pointer to the C routine.
 * @sa      IEM_DECL_IMPL_C_TYPE_0 and IEM_CIMPL_DEF_0.
 */
#define IEM_MC_CALL_CIMPL_0(a_pfnCImpl)                 return (a_pfnCImpl)(pVCpu, IEM_GET_INSTR_LEN(pVCpu))

/**
 * Defers the rest of instruction emulation to a C implementation routine and
 * returns, taking one argument in addition to the standard ones.
 *
 * @param   a_pfnCImpl      The pointer to the C routine.
 * @param   a0              The argument.
 */
#define IEM_MC_CALL_CIMPL_1(a_pfnCImpl, a0)             return (a_pfnCImpl)(pVCpu, IEM_GET_INSTR_LEN(pVCpu), a0)

/**
 * Defers the rest of the instruction emulation to a C implementation routine
 * and returns, taking two arguments in addition to the standard ones.
 *
 * @param   a_pfnCImpl      The pointer to the C routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 */
#define IEM_MC_CALL_CIMPL_2(a_pfnCImpl, a0, a1)         return (a_pfnCImpl)(pVCpu, IEM_GET_INSTR_LEN(pVCpu), a0, a1)

/**
 * Defers the rest of the instruction emulation to a C implementation routine
 * and returns, taking three arguments in addition to the standard ones.
 *
 * @param   a_pfnCImpl      The pointer to the C routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 * @param   a2              The third extra argument.
 */
#define IEM_MC_CALL_CIMPL_3(a_pfnCImpl, a0, a1, a2)     return (a_pfnCImpl)(pVCpu, IEM_GET_INSTR_LEN(pVCpu), a0, a1, a2)

/**
 * Defers the rest of the instruction emulation to a C implementation routine
 * and returns, taking four arguments in addition to the standard ones.
 *
 * @param   a_pfnCImpl      The pointer to the C routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 * @param   a2              The third extra argument.
 * @param   a3              The fourth extra argument.
 */
#define IEM_MC_CALL_CIMPL_4(a_pfnCImpl, a0, a1, a2, a3)     return (a_pfnCImpl)(pVCpu, IEM_GET_INSTR_LEN(pVCpu), a0, a1, a2, a3)

/**
 * Defers the rest of the instruction emulation to a C implementation routine
 * and returns, taking two arguments in addition to the standard ones.
 *
 * @param   a_pfnCImpl      The pointer to the C routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 * @param   a2              The third extra argument.
 * @param   a3              The fourth extra argument.
 * @param   a4              The fifth extra argument.
 */
#define IEM_MC_CALL_CIMPL_5(a_pfnCImpl, a0, a1, a2, a3, a4) return (a_pfnCImpl)(pVCpu, IEM_GET_INSTR_LEN(pVCpu), a0, a1, a2, a3, a4)

/**
 * Defers the entire instruction emulation to a C implementation routine and
 * returns, only taking the standard parameters.
 *
 * This shall be used without any IEM_MC_BEGIN or IEM_END macro surrounding it.
 *
 * @param   a_pfnCImpl      The pointer to the C routine.
 * @sa      IEM_DECL_IMPL_C_TYPE_0 and IEM_CIMPL_DEF_0.
 */
#define IEM_MC_DEFER_TO_CIMPL_0(a_pfnCImpl)             (a_pfnCImpl)(pVCpu, IEM_GET_INSTR_LEN(pVCpu))

/**
 * Defers the entire instruction emulation to a C implementation routine and
 * returns, taking one argument in addition to the standard ones.
 *
 * This shall be used without any IEM_MC_BEGIN or IEM_END macro surrounding it.
 *
 * @param   a_pfnCImpl      The pointer to the C routine.
 * @param   a0              The argument.
 */
#define IEM_MC_DEFER_TO_CIMPL_1(a_pfnCImpl, a0)         (a_pfnCImpl)(pVCpu, IEM_GET_INSTR_LEN(pVCpu), a0)

/**
 * Defers the entire instruction emulation to a C implementation routine and
 * returns, taking two arguments in addition to the standard ones.
 *
 * This shall be used without any IEM_MC_BEGIN or IEM_END macro surrounding it.
 *
 * @param   a_pfnCImpl      The pointer to the C routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 */
#define IEM_MC_DEFER_TO_CIMPL_2(a_pfnCImpl, a0, a1)     (a_pfnCImpl)(pVCpu, IEM_GET_INSTR_LEN(pVCpu), a0, a1)

/**
 * Defers the entire instruction emulation to a C implementation routine and
 * returns, taking three arguments in addition to the standard ones.
 *
 * This shall be used without any IEM_MC_BEGIN or IEM_END macro surrounding it.
 *
 * @param   a_pfnCImpl      The pointer to the C routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 * @param   a2              The third extra argument.
 */
#define IEM_MC_DEFER_TO_CIMPL_3(a_pfnCImpl, a0, a1, a2) (a_pfnCImpl)(pVCpu, IEM_GET_INSTR_LEN(pVCpu), a0, a1, a2)

/**
 * Calls a FPU assembly implementation taking one visible argument.
 *
 * @param   a_pfnAImpl      Pointer to the assembly FPU routine.
 * @param   a0              The first extra argument.
 */
#define IEM_MC_CALL_FPU_AIMPL_1(a_pfnAImpl, a0) \
    do { \
        a_pfnAImpl(&pVCpu->cpum.GstCtx.XState.x87, (a0)); \
    } while (0)

/**
 * Calls a FPU assembly implementation taking two visible arguments.
 *
 * @param   a_pfnAImpl      Pointer to the assembly FPU routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 */
#define IEM_MC_CALL_FPU_AIMPL_2(a_pfnAImpl, a0, a1) \
    do { \
        a_pfnAImpl(&pVCpu->cpum.GstCtx.XState.x87, (a0), (a1)); \
    } while (0)

/**
 * Calls a FPU assembly implementation taking three visible arguments.
 *
 * @param   a_pfnAImpl      Pointer to the assembly FPU routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 * @param   a2              The third extra argument.
 */
#define IEM_MC_CALL_FPU_AIMPL_3(a_pfnAImpl, a0, a1, a2) \
    do { \
        a_pfnAImpl(&pVCpu->cpum.GstCtx.XState.x87, (a0), (a1), (a2)); \
    } while (0)

#define IEM_MC_SET_FPU_RESULT(a_FpuData, a_FSW, a_pr80Value) \
    do { \
        (a_FpuData).FSW       = (a_FSW); \
        (a_FpuData).r80Result = *(a_pr80Value); \
    } while (0)

/** Pushes FPU result onto the stack. */
#define IEM_MC_PUSH_FPU_RESULT(a_FpuData) \
    iemFpuPushResult(pVCpu, &a_FpuData)
/** Pushes FPU result onto the stack and sets the FPUDP. */
#define IEM_MC_PUSH_FPU_RESULT_MEM_OP(a_FpuData, a_iEffSeg, a_GCPtrEff) \
    iemFpuPushResultWithMemOp(pVCpu, &a_FpuData, a_iEffSeg, a_GCPtrEff)

/** Replaces ST0 with value one and pushes value 2 onto the FPU stack. */
#define IEM_MC_PUSH_FPU_RESULT_TWO(a_FpuDataTwo) \
    iemFpuPushResultTwo(pVCpu, &a_FpuDataTwo)

/** Stores FPU result in a stack register. */
#define IEM_MC_STORE_FPU_RESULT(a_FpuData, a_iStReg) \
    iemFpuStoreResult(pVCpu, &a_FpuData, a_iStReg)
/** Stores FPU result in a stack register and pops the stack. */
#define IEM_MC_STORE_FPU_RESULT_THEN_POP(a_FpuData, a_iStReg) \
    iemFpuStoreResultThenPop(pVCpu, &a_FpuData, a_iStReg)
/** Stores FPU result in a stack register and sets the FPUDP. */
#define IEM_MC_STORE_FPU_RESULT_MEM_OP(a_FpuData, a_iStReg, a_iEffSeg, a_GCPtrEff) \
    iemFpuStoreResultWithMemOp(pVCpu, &a_FpuData, a_iStReg, a_iEffSeg, a_GCPtrEff)
/** Stores FPU result in a stack register, sets the FPUDP, and pops the
 *  stack. */
#define IEM_MC_STORE_FPU_RESULT_WITH_MEM_OP_THEN_POP(a_FpuData, a_iStReg, a_iEffSeg, a_GCPtrEff) \
    iemFpuStoreResultWithMemOpThenPop(pVCpu, &a_FpuData, a_iStReg, a_iEffSeg, a_GCPtrEff)

/** Only update the FOP, FPUIP, and FPUCS. (For FNOP.) */
#define IEM_MC_UPDATE_FPU_OPCODE_IP() \
    iemFpuUpdateOpcodeAndIp(pVCpu)
/** Free a stack register (for FFREE and FFREEP). */
#define IEM_MC_FPU_STACK_FREE(a_iStReg) \
    iemFpuStackFree(pVCpu, a_iStReg)
/** Increment the FPU stack pointer. */
#define IEM_MC_FPU_STACK_INC_TOP() \
    iemFpuStackIncTop(pVCpu)
/** Decrement the FPU stack pointer. */
#define IEM_MC_FPU_STACK_DEC_TOP() \
    iemFpuStackDecTop(pVCpu)

/** Updates the FSW, FOP, FPUIP, and FPUCS. */
#define IEM_MC_UPDATE_FSW(a_u16FSW) \
    iemFpuUpdateFSW(pVCpu, a_u16FSW)
/** Updates the FSW with a constant value as well as FOP, FPUIP, and FPUCS. */
#define IEM_MC_UPDATE_FSW_CONST(a_u16FSW) \
    iemFpuUpdateFSW(pVCpu, a_u16FSW)
/** Updates the FSW, FOP, FPUIP, FPUCS, FPUDP, and FPUDS. */
#define IEM_MC_UPDATE_FSW_WITH_MEM_OP(a_u16FSW, a_iEffSeg, a_GCPtrEff) \
    iemFpuUpdateFSWWithMemOp(pVCpu, a_u16FSW, a_iEffSeg, a_GCPtrEff)
/** Updates the FSW, FOP, FPUIP, and FPUCS, and then pops the stack. */
#define IEM_MC_UPDATE_FSW_THEN_POP(a_u16FSW) \
    iemFpuUpdateFSWThenPop(pVCpu, a_u16FSW)
/** Updates the FSW, FOP, FPUIP, FPUCS, FPUDP and FPUDS, and then pops the
 *  stack. */
#define IEM_MC_UPDATE_FSW_WITH_MEM_OP_THEN_POP(a_u16FSW, a_iEffSeg, a_GCPtrEff) \
    iemFpuUpdateFSWWithMemOpThenPop(pVCpu, a_u16FSW, a_iEffSeg, a_GCPtrEff)
/** Updates the FSW, FOP, FPUIP, and FPUCS, and then pops the stack twice. */
#define IEM_MC_UPDATE_FSW_THEN_POP_POP(a_u16FSW) \
    iemFpuUpdateFSWThenPopPop(pVCpu, a_u16FSW)

/** Raises a FPU stack underflow exception.  Sets FPUIP, FPUCS and FOP. */
#define IEM_MC_FPU_STACK_UNDERFLOW(a_iStDst) \
    iemFpuStackUnderflow(pVCpu, a_iStDst)
/** Raises a FPU stack underflow exception.  Sets FPUIP, FPUCS and FOP. Pops
 *  stack. */
#define IEM_MC_FPU_STACK_UNDERFLOW_THEN_POP(a_iStDst) \
    iemFpuStackUnderflowThenPop(pVCpu, a_iStDst)
/** Raises a FPU stack underflow exception.  Sets FPUIP, FPUCS, FOP, FPUDP and
 *  FPUDS. */
#define IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP(a_iStDst, a_iEffSeg, a_GCPtrEff) \
    iemFpuStackUnderflowWithMemOp(pVCpu, a_iStDst, a_iEffSeg, a_GCPtrEff)
/** Raises a FPU stack underflow exception.  Sets FPUIP, FPUCS, FOP, FPUDP and
 *  FPUDS. Pops stack. */
#define IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP_THEN_POP(a_iStDst, a_iEffSeg, a_GCPtrEff) \
    iemFpuStackUnderflowWithMemOpThenPop(pVCpu, a_iStDst, a_iEffSeg, a_GCPtrEff)
/** Raises a FPU stack underflow exception.  Sets FPUIP, FPUCS and FOP. Pops
 *  stack twice. */
#define IEM_MC_FPU_STACK_UNDERFLOW_THEN_POP_POP() \
    iemFpuStackUnderflowThenPopPop(pVCpu)
/** Raises a FPU stack underflow exception for an instruction pushing a result
 *  value onto the stack. Sets FPUIP, FPUCS and FOP. */
#define IEM_MC_FPU_STACK_PUSH_UNDERFLOW() \
    iemFpuStackPushUnderflow(pVCpu)
/** Raises a FPU stack underflow exception for an instruction pushing a result
 *  value onto the stack and replacing ST0. Sets FPUIP, FPUCS and FOP. */
#define IEM_MC_FPU_STACK_PUSH_UNDERFLOW_TWO() \
    iemFpuStackPushUnderflowTwo(pVCpu)

/** Raises a FPU stack overflow exception as part of a push attempt.  Sets
 *  FPUIP, FPUCS and FOP. */
#define IEM_MC_FPU_STACK_PUSH_OVERFLOW() \
    iemFpuStackPushOverflow(pVCpu)
/** Raises a FPU stack overflow exception as part of a push attempt.  Sets
 *  FPUIP, FPUCS, FOP, FPUDP and FPUDS. */
#define IEM_MC_FPU_STACK_PUSH_OVERFLOW_MEM_OP(a_iEffSeg, a_GCPtrEff) \
    iemFpuStackPushOverflowWithMemOp(pVCpu, a_iEffSeg, a_GCPtrEff)
/** Prepares for using the FPU state.
 * Ensures that we can use the host FPU in the current context (RC+R0.
 * Ensures the guest FPU state in the CPUMCTX is up to date. */
#define IEM_MC_PREPARE_FPU_USAGE()              iemFpuPrepareUsage(pVCpu)
/** Actualizes the guest FPU state so it can be accessed read-only fashion. */
#define IEM_MC_ACTUALIZE_FPU_STATE_FOR_READ()   iemFpuActualizeStateForRead(pVCpu)
/** Actualizes the guest FPU state so it can be accessed and modified. */
#define IEM_MC_ACTUALIZE_FPU_STATE_FOR_CHANGE() iemFpuActualizeStateForChange(pVCpu)

/** Stores SSE SIMD result updating MXCSR. */
#define IEM_MC_STORE_SSE_RESULT(a_SseData, a_iXmmReg) \
    iemSseStoreResult(pVCpu, &a_SseData, a_iXmmReg)
/** Updates MXCSR. */
#define IEM_MC_SSE_UPDATE_MXCSR(a_fMxcsr) \
    iemSseUpdateMxcsr(pVCpu, a_fMxcsr)

/** Prepares for using the SSE state.
 * Ensures that we can use the host SSE/FPU in the current context (RC+R0.
 * Ensures the guest SSE state in the CPUMCTX is up to date. */
#define IEM_MC_PREPARE_SSE_USAGE()              iemFpuPrepareUsageSse(pVCpu)
/** Actualizes the guest XMM0..15 and MXCSR register state for read-only access. */
#define IEM_MC_ACTUALIZE_SSE_STATE_FOR_READ()   iemFpuActualizeSseStateForRead(pVCpu)
/** Actualizes the guest XMM0..15 and MXCSR register state for read-write access. */
#define IEM_MC_ACTUALIZE_SSE_STATE_FOR_CHANGE() iemFpuActualizeSseStateForChange(pVCpu)

/** Prepares for using the AVX state.
 * Ensures that we can use the host AVX/FPU in the current context (RC+R0.
 * Ensures the guest AVX state in the CPUMCTX is up to date.
 * @note This will include the AVX512 state too when support for it is added
 *       due to the zero extending feature of VEX instruction. */
#define IEM_MC_PREPARE_AVX_USAGE()              iemFpuPrepareUsageAvx(pVCpu)
/** Actualizes the guest XMM0..15 and MXCSR register state for read-only access. */
#define IEM_MC_ACTUALIZE_AVX_STATE_FOR_READ()   iemFpuActualizeAvxStateForRead(pVCpu)
/** Actualizes the guest YMM0..15 and MXCSR register state for read-write access. */
#define IEM_MC_ACTUALIZE_AVX_STATE_FOR_CHANGE() iemFpuActualizeAvxStateForChange(pVCpu)

/**
 * Calls a MMX assembly implementation taking two visible arguments.
 *
 * @param   a_pfnAImpl      Pointer to the assembly MMX routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 */
#define IEM_MC_CALL_MMX_AIMPL_2(a_pfnAImpl, a0, a1) \
    do { \
        IEM_MC_PREPARE_FPU_USAGE(); \
        a_pfnAImpl(&pVCpu->cpum.GstCtx.XState.x87, (a0), (a1)); \
    } while (0)

/**
 * Calls a MMX assembly implementation taking three visible arguments.
 *
 * @param   a_pfnAImpl      Pointer to the assembly MMX routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 * @param   a2              The third extra argument.
 */
#define IEM_MC_CALL_MMX_AIMPL_3(a_pfnAImpl, a0, a1, a2) \
    do { \
        IEM_MC_PREPARE_FPU_USAGE(); \
        a_pfnAImpl(&pVCpu->cpum.GstCtx.XState.x87, (a0), (a1), (a2)); \
    } while (0)


/**
 * Calls a SSE assembly implementation taking two visible arguments.
 *
 * @param   a_pfnAImpl      Pointer to the assembly SSE routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 */
#define IEM_MC_CALL_SSE_AIMPL_2(a_pfnAImpl, a0, a1) \
    do { \
        IEM_MC_PREPARE_SSE_USAGE(); \
        a_pfnAImpl(&pVCpu->cpum.GstCtx.XState.x87, (a0), (a1)); \
    } while (0)

/**
 * Calls a SSE assembly implementation taking three visible arguments.
 *
 * @param   a_pfnAImpl      Pointer to the assembly SSE routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 * @param   a2              The third extra argument.
 */
#define IEM_MC_CALL_SSE_AIMPL_3(a_pfnAImpl, a0, a1, a2) \
    do { \
        IEM_MC_PREPARE_SSE_USAGE(); \
        a_pfnAImpl(&pVCpu->cpum.GstCtx.XState.x87, (a0), (a1), (a2)); \
    } while (0)


/** Declares implicit arguments for IEM_MC_CALL_AVX_AIMPL_2,
 *  IEM_MC_CALL_AVX_AIMPL_3, IEM_MC_CALL_AVX_AIMPL_4, ... */
#define IEM_MC_IMPLICIT_AVX_AIMPL_ARGS() \
    IEM_MC_ARG_CONST(PX86XSAVEAREA, pXState, &pVCpu->cpum.GstCtx.XState, 0)

/**
 * Calls a AVX assembly implementation taking two visible arguments.
 *
 * There is one implicit zero'th argument, a pointer to the extended state.
 *
 * @param   a_pfnAImpl      Pointer to the assembly AVX routine.
 * @param   a1              The first extra argument.
 * @param   a2              The second extra argument.
 */
#define IEM_MC_CALL_AVX_AIMPL_2(a_pfnAImpl, a1, a2) \
    do { \
        IEM_MC_PREPARE_AVX_USAGE(); \
        a_pfnAImpl(pXState, (a1), (a2)); \
    } while (0)

/**
 * Calls a AVX assembly implementation taking three visible arguments.
 *
 * There is one implicit zero'th argument, a pointer to the extended state.
 *
 * @param   a_pfnAImpl      Pointer to the assembly AVX routine.
 * @param   a1              The first extra argument.
 * @param   a2              The second extra argument.
 * @param   a3              The third extra argument.
 */
#define IEM_MC_CALL_AVX_AIMPL_3(a_pfnAImpl, a1, a2, a3) \
    do { \
        IEM_MC_PREPARE_AVX_USAGE(); \
        a_pfnAImpl(pXState, (a1), (a2), (a3)); \
    } while (0)

/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_EFL_BIT_SET(a_fBit)                   if (pVCpu->cpum.GstCtx.eflags.u & (a_fBit)) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_EFL_BIT_NOT_SET(a_fBit)               if (!(pVCpu->cpum.GstCtx.eflags.u & (a_fBit))) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_EFL_ANY_BITS_SET(a_fBits)             if (pVCpu->cpum.GstCtx.eflags.u & (a_fBits)) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_EFL_NO_BITS_SET(a_fBits)              if (!(pVCpu->cpum.GstCtx.eflags.u & (a_fBits))) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_EFL_BITS_NE(a_fBit1, a_fBit2)         \
    if (   !!(pVCpu->cpum.GstCtx.eflags.u & (a_fBit1)) \
        != !!(pVCpu->cpum.GstCtx.eflags.u & (a_fBit2)) ) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_EFL_BITS_EQ(a_fBit1, a_fBit2)         \
    if (   !!(pVCpu->cpum.GstCtx.eflags.u & (a_fBit1)) \
        == !!(pVCpu->cpum.GstCtx.eflags.u & (a_fBit2)) ) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_EFL_BIT_SET_OR_BITS_NE(a_fBit, a_fBit1, a_fBit2) \
    if (   (pVCpu->cpum.GstCtx.eflags.u & (a_fBit)) \
        ||    !!(pVCpu->cpum.GstCtx.eflags.u & (a_fBit1)) \
           != !!(pVCpu->cpum.GstCtx.eflags.u & (a_fBit2)) ) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_EFL_BIT_NOT_SET_AND_BITS_EQ(a_fBit, a_fBit1, a_fBit2) \
    if (   !(pVCpu->cpum.GstCtx.eflags.u & (a_fBit)) \
        &&    !!(pVCpu->cpum.GstCtx.eflags.u & (a_fBit1)) \
           == !!(pVCpu->cpum.GstCtx.eflags.u & (a_fBit2)) ) {
#define IEM_MC_IF_CX_IS_NZ()                            if (pVCpu->cpum.GstCtx.cx != 0) {
#define IEM_MC_IF_ECX_IS_NZ()                           if (pVCpu->cpum.GstCtx.ecx != 0) {
#define IEM_MC_IF_RCX_IS_NZ()                           if (pVCpu->cpum.GstCtx.rcx != 0) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_CX_IS_NZ_AND_EFL_BIT_SET(a_fBit) \
        if (   pVCpu->cpum.GstCtx.cx != 0 \
            && (pVCpu->cpum.GstCtx.eflags.u & a_fBit)) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_ECX_IS_NZ_AND_EFL_BIT_SET(a_fBit) \
        if (   pVCpu->cpum.GstCtx.ecx != 0 \
            && (pVCpu->cpum.GstCtx.eflags.u & a_fBit)) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_RCX_IS_NZ_AND_EFL_BIT_SET(a_fBit) \
        if (   pVCpu->cpum.GstCtx.rcx != 0 \
            && (pVCpu->cpum.GstCtx.eflags.u & a_fBit)) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_CX_IS_NZ_AND_EFL_BIT_NOT_SET(a_fBit) \
        if (   pVCpu->cpum.GstCtx.cx != 0 \
            && !(pVCpu->cpum.GstCtx.eflags.u & a_fBit)) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_ECX_IS_NZ_AND_EFL_BIT_NOT_SET(a_fBit) \
        if (   pVCpu->cpum.GstCtx.ecx != 0 \
            && !(pVCpu->cpum.GstCtx.eflags.u & a_fBit)) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_RCX_IS_NZ_AND_EFL_BIT_NOT_SET(a_fBit) \
        if (   pVCpu->cpum.GstCtx.rcx != 0 \
            && !(pVCpu->cpum.GstCtx.eflags.u & a_fBit)) {
#define IEM_MC_IF_LOCAL_IS_Z(a_Local)                   if ((a_Local) == 0) {
#define IEM_MC_IF_GREG_BIT_SET(a_iGReg, a_iBitNo)       if (iemGRegFetchU64(pVCpu, (a_iGReg)) & RT_BIT_64(a_iBitNo)) {

#define IEM_MC_REF_FPUREG(a_pr80Dst, a_iSt) \
    do { (a_pr80Dst) = &pVCpu->cpum.GstCtx.XState.x87.aRegs[a_iSt].r80; } while (0)
#define IEM_MC_IF_FPUREG_IS_EMPTY(a_iSt) \
    if (iemFpuStRegNotEmpty(pVCpu, (a_iSt)) != VINF_SUCCESS) {
#define IEM_MC_IF_FPUREG_NOT_EMPTY(a_iSt) \
    if (iemFpuStRegNotEmpty(pVCpu, (a_iSt)) == VINF_SUCCESS) {
#define IEM_MC_IF_FPUREG_IS_EMPTY(a_iSt) \
    if (iemFpuStRegNotEmpty(pVCpu, (a_iSt)) != VINF_SUCCESS) {
#define IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(a_pr80Dst, a_iSt) \
    if (iemFpuStRegNotEmptyRef(pVCpu, (a_iSt), &(a_pr80Dst)) == VINF_SUCCESS) {
#define IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80(a_pr80Dst0, a_iSt0, a_pr80Dst1, a_iSt1) \
    if (iemFpu2StRegsNotEmptyRef(pVCpu, (a_iSt0), &(a_pr80Dst0), (a_iSt1), &(a_pr80Dst1)) == VINF_SUCCESS) {
#define IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80_FIRST(a_pr80Dst0, a_iSt0, a_iSt1) \
    if (iemFpu2StRegsNotEmptyRefFirst(pVCpu, (a_iSt0), &(a_pr80Dst0), (a_iSt1)) == VINF_SUCCESS) {
#define IEM_MC_IF_FCW_IM() \
    if (pVCpu->cpum.GstCtx.XState.x87.FCW & X86_FCW_IM) {
#define IEM_MC_IF_MXCSR_XCPT_PENDING() \
        if ((  ~((pVCpu->cpum.GstCtx.XState.x87.MXCSR & X86_MXCSR_XCPT_MASK) >> X86_MXCSR_XCPT_MASK_SHIFT) \
             & (pVCpu->cpum.GstCtx.XState.x87.MXCSR & X86_MXCSR_XCPT_FLAGS)) != 0) {

#define IEM_MC_ELSE()                                   } else {
#define IEM_MC_ENDIF()                                  } do {} while (0)

/** @}  */

#endif /* !VMM_INCLUDED_SRC_include_IEMMc_h */

