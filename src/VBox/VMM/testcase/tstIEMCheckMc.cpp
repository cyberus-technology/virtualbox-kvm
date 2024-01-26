/* $Id: tstIEMCheckMc.cpp $ */
/** @file
 * IEM Testcase - Check the "Microcode".
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
#define VMCPU_INCL_CPUM_GST_CTX
#include <iprt/assert.h>
#include <iprt/rand.h>
#include <iprt/test.h>

#include <VBox/types.h>
#include <VBox/err.h>
#include <VBox/log.h>
#define TST_IEM_CHECK_MC    /**< For hacks.  */
#define IN_TSTVMSTRUCT 1    /**< Ditto. */
#include "../include/IEMInternal.h"
#include <VBox/vmm/vm.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
bool volatile       g_fRandom;
uint8_t volatile    g_bRandom;
RTUINT128U          g_u128Zero;
X86XMMREG           g_XmmZero;



#define CHK_TYPE(a_ExpectedType, a_Param) \
    do { a_ExpectedType const * pCheckType = &(a_Param); NOREF(pCheckType); } while (0)
#define CHK_PTYPE(a_ExpectedType, a_Param) \
    do { a_ExpectedType pCheckType = (a_Param); NOREF(pCheckType); } while (0)

#define CHK_CONST(a_ExpectedType, a_Const) \
    do { \
        AssertCompile(((a_Const) >> 1) == ((a_Const) >> 1)); \
        AssertCompile((a_ExpectedType)(a_Const) == (a_Const)); \
    } while (0)

#define CHK_SINGLE_BIT(a_ExpectedType, a_fBitMask) \
    do { \
        CHK_CONST(a_ExpectedType, a_fBitMask); \
        AssertCompile(RT_IS_POWER_OF_TWO(a_fBitMask)); \
    } while (0)

#define CHK_GCPTR(a_EffAddr) \
    CHK_TYPE(RTGCPTR, a_EffAddr)

#define CHK_SEG_IDX(a_iSeg) \
    do { \
        uint8_t iMySeg = (a_iSeg); NOREF(iMySeg); /** @todo const or variable. grr. */ \
    } while (0)

#define CHK_GREG_IDX(a_iGReg) \
    do { \
        uint8_t const iMyGReg = (a_iGReg); NOREF(iMyGReg); \
    } while (0)

#define CHK_MREG_IDX(a_iMReg) \
    do { \
        uint8_t const iMyMReg = (a_iMReg); NOREF(iMyMReg); \
    } while (0)

#define CHK_XREG_IDX(a_iXReg) \
    do { \
        uint8_t const iMyXReg = (a_iXReg); NOREF(iMyXReg); \
    } while (0)

#define CHK_YREG_IDX(a_iYReg) \
    do { \
        uint8_t const iMyYReg = (a_iYReg); NOREF(iMyYReg); \
    } while (0)

#define CHK_CALL_ARG(a_Name, a_iArg) \
    do { RT_CONCAT3(iArgCheck_,a_iArg,a_Name) = 1; } while (0)


/** @name Other stubs.
 * @{   */

typedef VBOXSTRICTRC (* PFNIEMOP)(PVMCPU pVCpu);
#undef  FNIEMOP_DEF
#define FNIEMOP_DEF(a_Name) \
    static VBOXSTRICTRC a_Name(PVMCPU pVCpu) RT_NO_THROW_DEF
#undef FNIEMOP_DEF_1
#define FNIEMOP_DEF_1(a_Name, a_Type0, a_Name0) \
    static VBOXSTRICTRC a_Name(PVMCPU pVCpu, a_Type0 a_Name0) RT_NO_THROW_DEF
#undef FNIEMOP_DEF_2
#define FNIEMOP_DEF_2(a_Name, a_Type0, a_Name0, a_Type1, a_Name1) \
    static VBOXSTRICTRC a_Name(PVMCPU pVCpu, a_Type0 a_Name0, a_Type1 a_Name1) RT_NO_THROW_DEF

typedef VBOXSTRICTRC (* PFNIEMOPRM)(PVMCPU pVCpu, uint8_t bRm);
#undef FNIEMOPRM_DEF
#define FNIEMOPRM_DEF(a_Name) \
    static VBOXSTRICTRC a_Name(PVMCPU pVCpu, uint8_t bRm) RT_NO_THROW_DEF

#undef  IEM_NOT_REACHED_DEFAULT_CASE_RET
#define IEM_NOT_REACHED_DEFAULT_CASE_RET()                  default: return VERR_IPE_NOT_REACHED_DEFAULT_CASE
#undef  IEM_RETURN_ASPECT_NOT_IMPLEMENTED
#define IEM_RETURN_ASPECT_NOT_IMPLEMENTED()                 return IEM_RETURN_ASPECT_NOT_IMPLEMENTED
#undef  IEM_RETURN_ASPECT_NOT_IMPLEMENTED_LOG
#define IEM_RETURN_ASPECT_NOT_IMPLEMENTED_LOG(a_LoggerArgs) return IEM_RETURN_ASPECT_NOT_IMPLEMENTED


#define IEM_OPCODE_GET_NEXT_RM(a_pu8)                       do { *(a_pu8)  = g_bRandom; CHK_PTYPE(uint8_t  *, a_pu8);  } while (0)
#define IEM_OPCODE_GET_NEXT_U8(a_pu8)                       do { *(a_pu8)  = g_bRandom; CHK_PTYPE(uint8_t  *, a_pu8);  } while (0)
#define IEM_OPCODE_GET_NEXT_S8(a_pi8)                       do { *(a_pi8)  = g_bRandom; CHK_PTYPE(int8_t   *, a_pi8);  } while (0)
#define IEM_OPCODE_GET_NEXT_S8_SX_U16(a_pu16)               do { *(a_pu16) = g_bRandom; CHK_PTYPE(uint16_t *, a_pu16); } while (0)
#define IEM_OPCODE_GET_NEXT_S8_SX_U32(a_pu32)               do { *(a_pu32) = g_bRandom; CHK_PTYPE(uint32_t *, a_pu32); } while (0)
#define IEM_OPCODE_GET_NEXT_S8_SX_U64(a_pu64)               do { *(a_pu64) = g_bRandom; CHK_PTYPE(uint64_t *, a_pu64); } while (0)
#define IEM_OPCODE_GET_NEXT_U16(a_pu16)                     do { *(a_pu16) = g_bRandom; CHK_PTYPE(uint16_t *, a_pu16); } while (0)
#define IEM_OPCODE_GET_NEXT_U16_ZX_U32(a_pu32)              do { *(a_pu32) = g_bRandom; CHK_PTYPE(uint32_t *, a_pu32); } while (0)
#define IEM_OPCODE_GET_NEXT_U16_ZX_U64(a_pu64)              do { *(a_pu64) = g_bRandom; CHK_PTYPE(uint64_t *, a_pu64); } while (0)
#define IEM_OPCODE_GET_NEXT_S16(a_pi16)                     do { *(a_pi16) = g_bRandom; CHK_PTYPE(int16_t  *, a_pi16); } while (0)
#define IEM_OPCODE_GET_NEXT_U32(a_pu32)                     do { *(a_pu32) = g_bRandom; CHK_PTYPE(uint32_t *, a_pu32); } while (0)
#define IEM_OPCODE_GET_NEXT_U32_ZX_U64(a_pu64)              do { *(a_pu64) = g_bRandom; CHK_PTYPE(uint64_t *, a_pu64); } while (0)
#define IEM_OPCODE_GET_NEXT_S32(a_pi32)                     do { *(a_pi32) = g_bRandom; CHK_PTYPE(int32_t  *, a_pi32); } while (0)
#define IEM_OPCODE_GET_NEXT_S32_SX_U64(a_pu64)              do { *(a_pu64) = g_bRandom; CHK_PTYPE(uint64_t *, a_pu64); } while (0)
#define IEM_OPCODE_GET_NEXT_U64(a_pu64)                     do { *(a_pu64) = g_bRandom; CHK_PTYPE(uint64_t *, a_pu64); } while (0)
#define IEMOP_HLP_MIN_186()                                 do { } while (0)
#define IEMOP_HLP_MIN_286()                                 do { } while (0)
#define IEMOP_HLP_MIN_386()                                 do { } while (0)
#define IEMOP_HLP_MIN_386_EX(a_fTrue)                       do { } while (0)
#define IEMOP_HLP_MIN_486()                                 do { } while (0)
#define IEMOP_HLP_MIN_586()                                 do { } while (0)
#define IEMOP_HLP_MIN_686()                                 do { } while (0)
#define IEMOP_HLP_NO_REAL_OR_V86_MODE()                     do { } while (0)
#define IEMOP_HLP_NO_64BIT()                                do { } while (0)
#define IEMOP_HLP_ONLY_64BIT()                              do { } while (0)
#define IEMOP_HLP_64BIT_OP_SIZE()                           do { } while (0)
#define IEMOP_HLP_DEFAULT_64BIT_OP_SIZE()                   do { } while (0)
#define IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX()                          do { } while (0)
#define IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE(a_szPrf)      do { } while (0)
#define IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX()            do { } while (0)
#define IEMOP_HLP_DONE_VEX_DECODING()                       do { } while (0)
#define IEMOP_HLP_DONE_VEX_DECODING_EX(a_fFeature)          do { } while (0)
#define IEMOP_HLP_DONE_VEX_DECODING_L0_EX(a_fFeature)       do { } while (0)
#define IEMOP_HLP_DONE_VEX_DECODING_L0()                    do { } while (0)
#define IEMOP_HLP_DONE_VEX_DECODING_NO_VVVV()               do { } while (0)
#define IEMOP_HLP_DONE_VEX_DECODING_NO_VVVV_EX(a_fFeature)  do { } while (0)
#define IEMOP_HLP_DONE_VEX_DECODING_L0_AND_NO_VVVV()        do { } while (0)
#define IEMOP_HLP_DONE_DECODING_NO_LOCK_REPZ_OR_REPNZ_PREFIXES()                                    do { } while (0)
#define IEMOP_HLP_DONE_DECODING_NO_SIZE_OP_REPZ_OR_REPNZ_PREFIXES()                                 do { } while (0)
#ifdef VBOX_WITH_NESTED_HWVIRT_VMX
# define IEMOP_HLP_VMX_INSTR(a_szInstr, a_InsDiagPrefix)                                            do { } while (0)
# define IEMOP_HLP_IN_VMX_OPERATION(a_szInstr, a_InsDiagPrefix)                                     do { } while (0)
#endif


#define IEMOP_HLP_DONE_DECODING()                           do { } while (0)

#define IEMOP_HLP_DECODED_NL_1(a_uDisOpNo, a_fIemOpFlags, a_uDisParam0, a_fDisOpType)               do { } while (0)
#define IEMOP_HLP_DECODED_NL_2(a_uDisOpNo, a_fIemOpFlags, a_uDisParam0, a_uDisParam1, a_fDisOpType) do { } while (0)
#undef  IEMOP_RAISE_DIVIDE_ERROR
#define IEMOP_RAISE_DIVIDE_ERROR()                          VERR_TRPM_ACTIVE_TRAP
#undef  IEMOP_RAISE_INVALID_OPCODE
#define IEMOP_RAISE_INVALID_OPCODE()                        VERR_TRPM_ACTIVE_TRAP
#undef  IEMOP_RAISE_INVALID_LOCK_PREFIX
#define IEMOP_RAISE_INVALID_LOCK_PREFIX()                   VERR_TRPM_ACTIVE_TRAP
#define IEMOP_MNEMONIC(a_Stats, a_szMnemonic)               do { } while (0)
#define IEMOP_MNEMONIC0EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_fDisHints, a_fIemHints) do { } while (0)
#define IEMOP_MNEMONIC1EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_Op1, a_fDisHints, a_fIemHints) do { } while (0)
#define IEMOP_MNEMONIC2EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_fDisHints, a_fIemHints) do { } while (0)
#define IEMOP_MNEMONIC3EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_Op3, a_fDisHints, a_fIemHints) do { } while (0)
#define IEMOP_MNEMONIC4EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_Op3, a_Op4, a_fDisHints, a_fIemHints) do { } while (0)
#define IEMOP_MNEMONIC0(a_Form, a_Upper, a_Lower, a_fDisHints, a_fIemHints)                         do { } while (0)
#define IEMOP_MNEMONIC1(a_Form, a_Upper, a_Lower, a_Op1, a_fDisHints, a_fIemHints)                  do { } while (0)
#define IEMOP_MNEMONIC2(a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_fDisHints, a_fIemHints)           do { } while (0)
#define IEMOP_MNEMONIC3(a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_Op3, a_fDisHints, a_fIemHints)    do { } while (0)
#define IEMOP_MNEMONIC4(a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_Op3, a_Op4, a_fDisHints, a_fIemHints)    do { } while (0)
#define IEMOP_BITCH_ABOUT_STUB()                            do { } while (0)
#define FNIEMOP_STUB(a_Name) \
    FNIEMOP_DEF(a_Name) { return VERR_NOT_IMPLEMENTED; } \
    typedef int ignore_semicolon
#define FNIEMOP_STUB_1(a_Name, a_Type0, a_Name0) \
    FNIEMOP_DEF_1(a_Name, a_Type0, a_Name0) { return VERR_NOT_IMPLEMENTED; } \
    typedef int ignore_semicolon

#define FNIEMOP_UD_STUB(a_Name) \
    FNIEMOP_DEF(a_Name) { return IEMOP_RAISE_INVALID_OPCODE(); } \
    typedef int ignore_semicolon
#define FNIEMOP_UD_STUB_1(a_Name, a_Type0, a_Name0) \
    FNIEMOP_DEF_1(a_Name, a_Type0, a_Name0) { return IEMOP_RAISE_INVALID_OPCODE(); } \
    typedef int ignore_semicolon


#define FNIEMOP_CALL(a_pfn)                                 (a_pfn)(pVCpu)
#define FNIEMOP_CALL_1(a_pfn, a0)                           (a_pfn)(pVCpu, a0)
#define FNIEMOP_CALL_2(a_pfn, a0, a1)                       (a_pfn)(pVCpu, a0, a1)

#undef  IEM_IS_REAL_OR_V86_MODE
#define IEM_IS_REAL_OR_V86_MODE(a_pVCpu)                    (g_fRandom)
#undef  IEM_IS_LONG_MODE
#define IEM_IS_LONG_MODE(a_pVCpu)                           (g_fRandom)
#undef  IEM_IS_REAL_MODE
#define IEM_IS_REAL_MODE(a_pVCpu)                           (g_fRandom)
#undef  IEM_IS_GUEST_CPU_AMD
#define IEM_IS_GUEST_CPU_AMD(a_pVCpu)                       (g_fRandom)
#undef  IEM_IS_GUEST_CPU_INTEL
#define IEM_IS_GUEST_CPU_INTEL(a_pVCpu)                     (g_fRandom)
#undef  IEM_GET_GUEST_CPU_FEATURES
#define IEM_GET_GUEST_CPU_FEATURES(a_pVCpu)                 ((PCCPUMFEATURES)(uintptr_t)42)
#undef  IEM_GET_HOST_CPU_FEATURES
#define IEM_GET_HOST_CPU_FEATURES(a_pVCpu)                  ((PCCPUMFEATURES)(uintptr_t)88)

#define iemRecalEffOpSize(a_pVCpu)                          do { } while (0)

IEMOPBINSIZES g_iemAImpl_add;
IEMOPBINSIZES g_iemAImpl_adc;
IEMOPBINSIZES g_iemAImpl_sub;
IEMOPBINSIZES g_iemAImpl_sbb;
IEMOPBINSIZES g_iemAImpl_or;
IEMOPBINSIZES g_iemAImpl_xor;
IEMOPBINSIZES g_iemAImpl_and;
IEMOPBINSIZES g_iemAImpl_cmp;
IEMOPBINSIZES g_iemAImpl_test;
IEMOPBINSIZES g_iemAImpl_bt;
IEMOPBINSIZES g_iemAImpl_btc;
IEMOPBINSIZES g_iemAImpl_btr;
IEMOPBINSIZES g_iemAImpl_bts;
IEMOPBINSIZES g_iemAImpl_bsf;
IEMOPBINSIZES g_iemAImpl_bsr;
PCIEMOPBINSIZES g_apIemImplGrp1[8];
IEMOPUNARYSIZES g_iemAImpl_inc;
IEMOPUNARYSIZES g_iemAImpl_dec;
IEMOPUNARYSIZES g_iemAImpl_neg;
IEMOPUNARYSIZES g_iemAImpl_not;

#undef  IEMTARGETCPU_EFL_BEHAVIOR_SELECT
#define IEMTARGETCPU_EFL_BEHAVIOR_SELECT(a_aArray)                  NULL
#undef  IEMTARGETCPU_EFL_BEHAVIOR_SELECT_NON_NATIVE
#define IEMTARGETCPU_EFL_BEHAVIOR_SELECT_NON_NATIVE(a_aArray)       NULL
#undef  IEMTARGETCPU_EFL_BEHAVIOR_SELECT_EX
#define IEMTARGETCPU_EFL_BEHAVIOR_SELECT_EX(a_aaArray, a_fNative)   NULL

#undef  IEM_SELECT_HOST_OR_FALLBACK
#define IEM_SELECT_HOST_OR_FALLBACK(a_fCpumFeatureMember, a_pfnNative, a_pfnFallback)   NULL

#define iemAImpl_fpu_r32_to_r80         NULL
#define iemAImpl_fcom_r80_by_r32        NULL
#define iemAImpl_fadd_r80_by_r32        NULL
#define iemAImpl_fmul_r80_by_r32        NULL
#define iemAImpl_fsub_r80_by_r32        NULL
#define iemAImpl_fsubr_r80_by_r32       NULL
#define iemAImpl_fdiv_r80_by_r32        NULL
#define iemAImpl_fdivr_r80_by_r32       NULL

#define iemAImpl_fpu_r64_to_r80         NULL
#define iemAImpl_fadd_r80_by_r64        NULL
#define iemAImpl_fmul_r80_by_r64        NULL
#define iemAImpl_fcom_r80_by_r64        NULL
#define iemAImpl_fsub_r80_by_r64        NULL
#define iemAImpl_fsubr_r80_by_r64       NULL
#define iemAImpl_fdiv_r80_by_r64        NULL
#define iemAImpl_fdivr_r80_by_r64       NULL

#define iemAImpl_fadd_r80_by_r80        NULL
#define iemAImpl_fmul_r80_by_r80        NULL
#define iemAImpl_fsub_r80_by_r80        NULL
#define iemAImpl_fsubr_r80_by_r80       NULL
#define iemAImpl_fdiv_r80_by_r80        NULL
#define iemAImpl_fdivr_r80_by_r80       NULL
#define iemAImpl_fprem_r80_by_r80       NULL
#define iemAImpl_fprem1_r80_by_r80      NULL
#define iemAImpl_fscale_r80_by_r80      NULL

#define iemAImpl_fpatan_r80_by_r80      NULL
#define iemAImpl_fyl2x_r80_by_r80       NULL
#define iemAImpl_fyl2xp1_r80_by_r80     NULL

#define iemAImpl_fcom_r80_by_r80        NULL
#define iemAImpl_fucom_r80_by_r80       NULL
#define iemAImpl_fabs_r80               NULL
#define iemAImpl_fchs_r80               NULL
#define iemAImpl_ftst_r80               NULL
#define iemAImpl_fxam_r80               NULL
#define iemAImpl_f2xm1_r80              NULL
#define iemAImpl_fsqrt_r80              NULL
#define iemAImpl_frndint_r80            NULL
#define iemAImpl_fsin_r80               NULL
#define iemAImpl_fcos_r80               NULL

#define iemAImpl_fld1                   NULL
#define iemAImpl_fldl2t                 NULL
#define iemAImpl_fldl2e                 NULL
#define iemAImpl_fldpi                  NULL
#define iemAImpl_fldlg2                 NULL
#define iemAImpl_fldln2                 NULL
#define iemAImpl_fldz                   NULL

#define iemAImpl_fptan_r80_r80          NULL
#define iemAImpl_fxtract_r80_r80        NULL
#define iemAImpl_fsincos_r80_r80        NULL

#define iemAImpl_fiadd_r80_by_i16       NULL
#define iemAImpl_fimul_r80_by_i16       NULL
#define iemAImpl_fisub_r80_by_i16       NULL
#define iemAImpl_fisubr_r80_by_i16      NULL
#define iemAImpl_fidiv_r80_by_i16       NULL
#define iemAImpl_fidivr_r80_by_i16      NULL

#define iemAImpl_fiadd_r80_by_i32       NULL
#define iemAImpl_fimul_r80_by_i32       NULL
#define iemAImpl_fisub_r80_by_i32       NULL
#define iemAImpl_fisubr_r80_by_i32      NULL
#define iemAImpl_fidiv_r80_by_i32       NULL
#define iemAImpl_fidivr_r80_by_i32      NULL

#define iemCImpl_callf                  NULL
#define iemCImpl_FarJmp                 NULL

#define iemAImpl_pshufhw_u128           NULL
#define iemAImpl_pshuflw_u128           NULL
#define iemAImpl_pshufd_u128            NULL
#define iemAImpl_punpcklbw_u64          NULL
#define iemAImpl_punpcklwd_u64          NULL
#define iemAImpl_punpckldq_u64          NULL
#define iemAImpl_punpckhbw_u64          NULL
#define iemAImpl_punpckhwd_u64          NULL
#define iemAImpl_punpckhdq_u64          NULL
#define iemAImpl_packsswb_u64           NULL
#define iemAImpl_packssdw_u64           NULL
#define iemAImpl_packuswb_u64           NULL

#define iemAImpl_punpcklbw_u128         NULL
#define iemAImpl_punpcklwd_u128         NULL
#define iemAImpl_punpckldq_u128         NULL
#define iemAImpl_punpcklqdq_u128        NULL
#define iemAImpl_punpckhbw_u128         NULL
#define iemAImpl_punpckhwd_u128         NULL
#define iemAImpl_punpckhdq_u128         NULL
#define iemAImpl_punpckhqdq_u128        NULL
#define iemAImpl_packsswb_u128          NULL
#define iemAImpl_packssdw_u128          NULL
#define iemAImpl_packuswb_u128          NULL
#define iemAImpl_packusdw_u128          NULL


#define iemAImpl_pand_u64               NULL
#define iemAImpl_pandn_u64              NULL
#define iemAImpl_por_u64                NULL
#define iemAImpl_pxor_u64               NULL
#define iemAImpl_pcmpeqb_u64            NULL
#define iemAImpl_pcmpeqw_u64            NULL
#define iemAImpl_pcmpeqd_u64            NULL
#define iemAImpl_pcmpgtb_u64            NULL
#define iemAImpl_pcmpgtw_u64            NULL
#define iemAImpl_pcmpgtd_u64            NULL
#define iemAImpl_paddb_u64              NULL
#define iemAImpl_paddw_u64              NULL
#define iemAImpl_paddd_u64              NULL
#define iemAImpl_paddq_u64              NULL
#define iemAImpl_psubb_u64              NULL
#define iemAImpl_psubw_u64              NULL
#define iemAImpl_psubd_u64              NULL
#define iemAImpl_psubq_u64              NULL

#define iemAImpl_pand_u128              NULL
#define iemAImpl_pandn_u128             NULL
#define iemAImpl_por_u128               NULL
#define iemAImpl_pxor_u128              NULL
#define iemAImpl_pcmpeqb_u128           NULL
#define iemAImpl_pcmpeqw_u128           NULL
#define iemAImpl_pcmpeqd_u128           NULL
#define iemAImpl_pcmpgtb_u128           NULL
#define iemAImpl_pcmpgtw_u128           NULL
#define iemAImpl_pcmpgtd_u128           NULL
#define iemAImpl_paddb_u128             NULL
#define iemAImpl_paddw_u128             NULL
#define iemAImpl_paddd_u128             NULL
#define iemAImpl_paddq_u128             NULL
#define iemAImpl_psubb_u128             NULL
#define iemAImpl_psubw_u128             NULL
#define iemAImpl_psubd_u128             NULL
#define iemAImpl_psubq_u128             NULL

#define iemAImpl_psllw_u64              NULL
#define iemAImpl_psrlw_u64              NULL
#define iemAImpl_psraw_u64              NULL
#define iemAImpl_pslld_u64              NULL
#define iemAImpl_psrld_u64              NULL
#define iemAImpl_psrad_u64              NULL
#define iemAImpl_psllq_u64              NULL
#define iemAImpl_psrlq_u64              NULL
#define iemAImpl_psraq_u64              NULL

#define iemAImpl_psllw_u128             NULL
#define iemAImpl_psrlw_u128             NULL
#define iemAImpl_psraw_u128             NULL
#define iemAImpl_pslld_u128             NULL
#define iemAImpl_psrld_u128             NULL
#define iemAImpl_psrad_u128             NULL
#define iemAImpl_psllq_u128             NULL
#define iemAImpl_psrlq_u128             NULL
#define iemAImpl_psraq_u128             NULL

#define iemAImpl_psllw_imm_u64          NULL
#define iemAImpl_psrlw_imm_u64          NULL
#define iemAImpl_psraw_imm_u64          NULL
#define iemAImpl_pslld_imm_u64          NULL
#define iemAImpl_psrld_imm_u64          NULL
#define iemAImpl_psrad_imm_u64          NULL
#define iemAImpl_psllq_imm_u64          NULL
#define iemAImpl_psrlq_imm_u64          NULL
#define iemAImpl_psraq_imm_u64          NULL

#define iemAImpl_psllw_imm_u128         NULL
#define iemAImpl_psrlw_imm_u128         NULL
#define iemAImpl_psraw_imm_u128         NULL
#define iemAImpl_pslld_imm_u128         NULL
#define iemAImpl_psrld_imm_u128         NULL
#define iemAImpl_psrad_imm_u128         NULL
#define iemAImpl_psllq_imm_u128         NULL
#define iemAImpl_psrlq_imm_u128         NULL
#define iemAImpl_psraq_imm_u128         NULL

#define iemAImpl_pslldq_imm_u128        NULL
#define iemAImpl_psrldq_imm_u128        NULL

#define iemAImpl_paddsb_u64             NULL
#define iemAImpl_paddusb_u64            NULL
#define iemAImpl_paddsw_u64             NULL
#define iemAImpl_paddusw_u64            NULL
#define iemAImpl_psubsb_u64             NULL
#define iemAImpl_psubusb_u64            NULL
#define iemAImpl_psubsw_u64             NULL
#define iemAImpl_psubusw_u64            NULL

#define iemAImpl_paddsb_u128            NULL
#define iemAImpl_paddusb_u128           NULL
#define iemAImpl_paddsw_u128            NULL
#define iemAImpl_paddusw_u128           NULL
#define iemAImpl_psubsb_u128            NULL
#define iemAImpl_psubusb_u128           NULL
#define iemAImpl_psubsw_u128            NULL
#define iemAImpl_psubusw_u128           NULL

#define iemAImpl_pmullw_u64             NULL
#define iemAImpl_pmulhw_u64             NULL
#define iemAImpl_pmulhuw_u64            NULL
#define iemAImpl_pmaddwd_u64            NULL

#define iemAImpl_pmullw_u128            NULL
#define iemAImpl_pmulhw_u128            NULL
#define iemAImpl_pmulhuw_u128           NULL
#define iemAImpl_pmaddwd_u128           NULL

#define iemAImpl_pmaxub_u64             NULL
#define iemAImpl_pmaxsw_u64             NULL
#define iemAImpl_pminub_u64             NULL
#define iemAImpl_pminsw_u64             NULL
#define iemAImpl_pavgb_u64              NULL
#define iemAImpl_pavgw_u64              NULL
#define iemAImpl_psadbw_u64             NULL
#define iemAImpl_pmuludq_u64            NULL

#define iemAImpl_pmaxub_u128            NULL
#define iemAImpl_pmaxsw_u128            NULL
#define iemAImpl_pminub_u128            NULL
#define iemAImpl_pminsw_u128            NULL
#define iemAImpl_pavgb_u128             NULL
#define iemAImpl_pavgw_u128             NULL
#define iemAImpl_psadbw_u128            NULL
#define iemAImpl_pmuludq_u128           NULL
#define iemAImpl_unpcklps_u128          NULL
#define iemAImpl_unpcklpd_u128          NULL
#define iemAImpl_unpckhps_u128          NULL
#define iemAImpl_unpckhpd_u128          NULL

#define iemAImpl_addps_u128             NULL
#define iemAImpl_addpd_u128             NULL
#define iemAImpl_mulps_u128             NULL
#define iemAImpl_mulpd_u128             NULL
#define iemAImpl_subps_u128             NULL
#define iemAImpl_subpd_u128             NULL
#define iemAImpl_minps_u128             NULL
#define iemAImpl_minpd_u128             NULL
#define iemAImpl_divps_u128             NULL
#define iemAImpl_divpd_u128             NULL
#define iemAImpl_maxps_u128             NULL
#define iemAImpl_maxpd_u128             NULL
#define iemAImpl_haddps_u128            NULL
#define iemAImpl_haddpd_u128            NULL
#define iemAImpl_hsubps_u128            NULL
#define iemAImpl_hsubpd_u128            NULL
#define iemAImpl_sqrtps_u128            NULL
#define iemAImpl_sqrtpd_u128            NULL
#define iemAImpl_rsqrtps_u128           NULL
#define iemAImpl_addsubps_u128          NULL
#define iemAImpl_addsubpd_u128          NULL
#define iemAImpl_cvtpd2ps_u128          NULL
#define iemAImpl_cvtps2pd_u128          NULL
#define iemAImpl_shufpd_u128            NULL
#define iemAImpl_shufps_u128            NULL
#define iemAImpl_roundps_u128           NULL
#define iemAImpl_roundpd_u128           NULL

#define iemAImpl_cvtdq2ps_u128          NULL
#define iemAImpl_cvtps2dq_u128          NULL
#define iemAImpl_cvttps2dq_u128         NULL
#define iemAImpl_cvttpd2dq_u128         NULL
#define iemAImpl_cvtdq2pd_u128          NULL
#define iemAImpl_cvtpd2dq_u128          NULL

#define iemAImpl_addss_u128_r32         NULL
#define iemAImpl_addsd_u128_r64         NULL
#define iemAImpl_mulss_u128_r32         NULL
#define iemAImpl_mulsd_u128_r64         NULL
#define iemAImpl_subss_u128_r32         NULL
#define iemAImpl_subsd_u128_r64         NULL
#define iemAImpl_minss_u128_r32         NULL
#define iemAImpl_minsd_u128_r64         NULL
#define iemAImpl_divss_u128_r32         NULL
#define iemAImpl_divsd_u128_r64         NULL
#define iemAImpl_maxss_u128_r32         NULL
#define iemAImpl_maxsd_u128_r64         NULL
#define iemAImpl_sqrtss_u128_r32        NULL
#define iemAImpl_sqrtsd_u128_r64        NULL
#define iemAImpl_roundss_u128_r32       NULL
#define iemAImpl_roundsd_u128_r64       NULL
#define iemAImpl_rsqrtss_u128_r32       NULL

#define iemAImpl_cvtss2sd_u128_r32      NULL
#define iemAImpl_cvtsd2ss_u128_r64      NULL

/** @}  */


#define IEM_REPEAT_0(a_Callback, a_User)    do { } while (0)
#define IEM_REPEAT_1(a_Callback, a_User)                                      a_Callback##_CALLBACK(0, a_User)
#define IEM_REPEAT_2(a_Callback, a_User)    IEM_REPEAT_1(a_Callback, a_User); a_Callback##_CALLBACK(1, a_User)
#define IEM_REPEAT_3(a_Callback, a_User)    IEM_REPEAT_2(a_Callback, a_User); a_Callback##_CALLBACK(2, a_User)
#define IEM_REPEAT_4(a_Callback, a_User)    IEM_REPEAT_3(a_Callback, a_User); a_Callback##_CALLBACK(3, a_User)
#define IEM_REPEAT_5(a_Callback, a_User)    IEM_REPEAT_4(a_Callback, a_User); a_Callback##_CALLBACK(4, a_User)
#define IEM_REPEAT_6(a_Callback, a_User)    IEM_REPEAT_5(a_Callback, a_User); a_Callback##_CALLBACK(5, a_User)
#define IEM_REPEAT_7(a_Callback, a_User)    IEM_REPEAT_6(a_Callback, a_User); a_Callback##_CALLBACK(6, a_User)
#define IEM_REPEAT_8(a_Callback, a_User)    IEM_REPEAT_7(a_Callback, a_User); a_Callback##_CALLBACK(7, a_User)
#define IEM_REPEAT_9(a_Callback, a_User)    IEM_REPEAT_8(a_Callback, a_User); a_Callback##_CALLBACK(8, a_User)
#define IEM_REPEAT(a_cTimes, a_Callback, a_User) RT_CONCAT(IEM_REPEAT_,a_cTimes)(a_Callback, a_User)



/** @name Microcode test stubs
 * @{  */

#define IEM_ARG_CHECK_CALLBACK(a_idx, a_User) int RT_CONCAT(iArgCheck_,a_idx); NOREF(RT_CONCAT(iArgCheck_,a_idx))
#define IEM_MC_BEGIN(a_cArgs, a_cLocals) \
    { \
        const uint8_t cArgs   = (a_cArgs); NOREF(cArgs); \
        const uint8_t cLocals = (a_cLocals); NOREF(cLocals); \
        const uint8_t fMcBegin = (a_cArgs) + (a_cLocals); \
        IEM_REPEAT(a_cArgs, IEM_ARG_CHECK, 0); \

#define IEM_MC_END() \
    }

#define IEM_MC_ADVANCE_RIP_AND_FINISH()                 do { (void)fMcBegin; return VINF_SUCCESS; } while (0)
#define IEM_MC_REL_JMP_S8_AND_FINISH(a_i8)              do { (void)fMcBegin; CHK_TYPE(int8_t, a_i8); return VINF_SUCCESS; } while (0)
#define IEM_MC_REL_JMP_S16_AND_FINISH(a_i16)            do { (void)fMcBegin; CHK_TYPE(int16_t, a_i16); return VINF_SUCCESS; } while (0)
#define IEM_MC_REL_JMP_S32_AND_FINISH(a_i32)            do { (void)fMcBegin; CHK_TYPE(int32_t, a_i32); return VINF_SUCCESS; } while (0)
#define IEM_MC_SET_RIP_U16_AND_FINISH(a_u16NewIP)       do { (void)fMcBegin; CHK_TYPE(uint16_t, a_u16NewIP); return VINF_SUCCESS; } while (0)
#define IEM_MC_SET_RIP_U32_AND_FINISH(a_u32NewIP)       do { (void)fMcBegin; CHK_TYPE(uint32_t, a_u32NewIP); return VINF_SUCCESS; } while (0)
#define IEM_MC_SET_RIP_U64_AND_FINISH(a_u64NewIP)       do { (void)fMcBegin; CHK_TYPE(uint64_t, a_u64NewIP); return VINF_SUCCESS; } while (0)
#define IEM_MC_RAISE_DIVIDE_ERROR()                     do { (void)fMcBegin; return VERR_TRPM_ACTIVE_TRAP; } while (0)
#define IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE()       do { (void)fMcBegin; } while (0)
#define IEM_MC_MAYBE_RAISE_WAIT_DEVICE_NOT_AVAILABLE()  do { (void)fMcBegin; } while (0)
#define IEM_MC_MAYBE_RAISE_FPU_XCPT()                   do { (void)fMcBegin; } while (0)
#define IEM_MC_MAYBE_RAISE_MMX_RELATED_XCPT()           do { (void)fMcBegin; } while (0)
#define IEM_MC_MAYBE_RAISE_MMX_RELATED_XCPT_EX(fSupported) do { (void)fMcBegin; } while (0)
#define IEM_MC_MAYBE_RAISE_MMX_RELATED_XCPT_CHECK_SSE_OR_MMXEXT() do { (void)fMcBegin; } while (0)
#define IEM_MC_MAYBE_RAISE_SSE_RELATED_XCPT()           do { (void)fMcBegin; } while (0)
#define IEM_MC_MAYBE_RAISE_SSE2_RELATED_XCPT()          do { (void)fMcBegin; } while (0)
#define IEM_MC_MAYBE_RAISE_SSE3_RELATED_XCPT()          do { (void)fMcBegin; } while (0)
#define IEM_MC_MAYBE_RAISE_SSSE3_RELATED_XCPT()         do { (void)fMcBegin; } while (0)
#define IEM_MC_MAYBE_RAISE_SSE41_RELATED_XCPT()         do { (void)fMcBegin; } while (0)
#define IEM_MC_MAYBE_RAISE_SSE42_RELATED_XCPT()         do { (void)fMcBegin; } while (0)
#define IEM_MC_MAYBE_RAISE_AESNI_RELATED_XCPT()         do { (void)fMcBegin; } while (0)
#define IEM_MC_MAYBE_RAISE_AVX_RELATED_XCPT()           do { (void)fMcBegin; } while (0)
#define IEM_MC_MAYBE_RAISE_AVX2_RELATED_XCPT()          do { (void)fMcBegin; } while (0)
#define IEM_MC_RAISE_GP0_IF_CPL_NOT_ZERO()              do { (void)fMcBegin; } while (0)
#define IEM_MC_RAISE_GP0_IF_EFF_ADDR_UNALIGNED(a_EffAddr, a_cbAlign) \
    do { (void)fMcBegin; AssertCompile(RT_IS_POWER_OF_TWO(a_cbAlign)); CHK_TYPE(RTGCPTR,  a_EffAddr); } while (0)
#define IEM_MC_MAYBE_RAISE_FSGSBASE_XCPT()              do { (void)fMcBegin; } while (0)
#define IEM_MC_MAYBE_RAISE_NON_CANONICAL_ADDR_GP0(a_u64Addr) do { (void)fMcBegin; } while (0)
#define IEM_MC_MAYBE_RAISE_SSE_AVX_SIMD_FP_OR_UD_XCPT() do { (void)fMcBegin; } while (0)
#define IEM_MC_RAISE_SSE_AVX_SIMD_FP_OR_UD_XCPT()       do { (void)fMcBegin; } while (0)
#define IEM_MC_MAYBE_RAISE_PCLMUL_RELATED_XCPT()        do { (void)fMcBegin; } while (0)

#define IEM_MC_LOCAL(a_Type, a_Name) (void)fMcBegin; \
    a_Type a_Name; NOREF(a_Name); (void)fMcBegin
#define IEM_MC_LOCAL_CONST(a_Type, a_Name, a_Value) (void)fMcBegin; \
    a_Type const a_Name = (a_Value); \
    NOREF(a_Name)
#define IEM_MC_REF_LOCAL(a_pRefArg, a_Local) (void)fMcBegin; \
    (a_pRefArg) = &(a_Local)

#define IEM_MC_ARG(a_Type, a_Name, a_iArg) (void)fMcBegin; \
    RT_CONCAT(iArgCheck_,a_iArg) = 1; NOREF(RT_CONCAT(iArgCheck_,a_iArg)); \
    int RT_CONCAT3(iArgCheck_,a_iArg,a_Name); NOREF(RT_CONCAT3(iArgCheck_,a_iArg,a_Name)); \
    AssertCompile((a_iArg) < cArgs); \
    a_Type a_Name; \
    NOREF(a_Name)
#define IEM_MC_ARG_CONST(a_Type, a_Name, a_Value, a_iArg) (void)fMcBegin; \
    RT_CONCAT(iArgCheck_, a_iArg) = 1; NOREF(RT_CONCAT(iArgCheck_,a_iArg)); \
    int RT_CONCAT3(iArgCheck_,a_iArg,a_Name); NOREF(RT_CONCAT3(iArgCheck_,a_iArg,a_Name)); \
    AssertCompile((a_iArg) < cArgs); \
    a_Type const a_Name = (a_Value); \
    NOREF(a_Name)
#define IEM_MC_ARG_XSTATE(a_Name, a_iArg) \
    IEM_MC_ARG_CONST(PX86XSAVEAREA, a_Name, NULL, a_iArg)

#define IEM_MC_ARG_LOCAL_REF(a_Type, a_Name, a_Local, a_iArg) (void)fMcBegin; \
    RT_CONCAT(iArgCheck_, a_iArg) = 1; NOREF(RT_CONCAT(iArgCheck_,a_iArg)); \
    int RT_CONCAT3(iArgCheck_,a_iArg,a_Name); NOREF(RT_CONCAT3(iArgCheck_,a_iArg,a_Name)); \
    AssertCompile((a_iArg) < cArgs); \
    a_Type const a_Name = &(a_Local); \
    NOREF(a_Name)
#define IEM_MC_ARG_LOCAL_EFLAGS(a_pName, a_Name, a_iArg) (void)fMcBegin; \
    RT_CONCAT(iArgCheck_, a_iArg) = 1; NOREF(RT_CONCAT(iArgCheck_,a_iArg)); \
    int RT_CONCAT3(iArgCheck_,a_iArg,a_pName); NOREF(RT_CONCAT3(iArgCheck_,a_iArg,a_pName)); \
    AssertCompile((a_iArg) < cArgs); \
    uint32_t a_Name; \
    uint32_t *a_pName = &a_Name; \
    NOREF(a_pName)

#define IEM_MC_COMMIT_EFLAGS(a_EFlags)                  do { CHK_TYPE(uint32_t, a_EFlags); (void)fMcBegin; } while (0)
#define IEM_MC_ASSIGN(a_VarOrArg, a_CVariableOrConst)   do { (a_VarOrArg) = (0); (void)fMcBegin; } while (0)
#define IEM_MC_ASSIGN_TO_SMALLER                        IEM_MC_ASSIGN

#define IEM_MC_FETCH_GREG_U8(a_u8Dst, a_iGReg)          do { (a_u8Dst)  = 0; CHK_TYPE(uint8_t,  a_u8Dst);  CHK_GREG_IDX(a_iGReg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_GREG_U8_ZX_U16(a_u16Dst, a_iGReg)  do { (a_u16Dst) = 0; CHK_TYPE(uint16_t, a_u16Dst); CHK_GREG_IDX(a_iGReg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_GREG_U8_ZX_U32(a_u32Dst, a_iGReg)  do { (a_u32Dst) = 0; CHK_TYPE(uint32_t, a_u32Dst); CHK_GREG_IDX(a_iGReg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_GREG_U8_ZX_U64(a_u64Dst, a_iGReg)  do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); CHK_GREG_IDX(a_iGReg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_GREG_U8_SX_U16(a_u16Dst, a_iGReg)  do { (a_u16Dst) = 0; CHK_TYPE(uint16_t, a_u16Dst); CHK_GREG_IDX(a_iGReg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_GREG_U8_SX_U32(a_u32Dst, a_iGReg)  do { (a_u32Dst) = 0; CHK_TYPE(uint32_t, a_u32Dst); CHK_GREG_IDX(a_iGReg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_GREG_U8_SX_U64(a_u64Dst, a_iGReg)  do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); CHK_GREG_IDX(a_iGReg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_GREG_U16(a_u16Dst, a_iGReg)        do { (a_u16Dst) = 0; CHK_TYPE(uint16_t, a_u16Dst); CHK_GREG_IDX(a_iGReg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_GREG_U16_ZX_U32(a_u32Dst, a_iGReg) do { (a_u32Dst) = 0; CHK_TYPE(uint32_t, a_u32Dst); CHK_GREG_IDX(a_iGReg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_GREG_U16_ZX_U64(a_u64Dst, a_iGReg) do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); CHK_GREG_IDX(a_iGReg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_GREG_U16_SX_U32(a_u32Dst, a_iGReg) do { (a_u32Dst) = 0; CHK_TYPE(uint32_t, a_u32Dst); CHK_GREG_IDX(a_iGReg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_GREG_U16_SX_U64(a_u64Dst, a_iGReg) do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); CHK_GREG_IDX(a_iGReg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_GREG_U32(a_u32Dst, a_iGReg)        do { (a_u32Dst) = 0; CHK_TYPE(uint32_t, a_u32Dst); CHK_GREG_IDX(a_iGReg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_GREG_U32_ZX_U64(a_u64Dst, a_iGReg) do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); CHK_GREG_IDX(a_iGReg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_GREG_U32_SX_U64(a_u64Dst, a_iGReg) do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); CHK_GREG_IDX(a_iGReg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_GREG_U64(a_u64Dst, a_iGReg)        do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); CHK_GREG_IDX(a_iGReg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_GREG_U64_ZX_U64                    IEM_MC_FETCH_GREG_U64
#define IEM_MC_FETCH_SREG_U16(a_u16Dst, a_iSReg)        do { (a_u16Dst) = 0; CHK_TYPE(uint16_t, a_u16Dst); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_SREG_ZX_U32(a_u32Dst, a_iSReg)     do { (a_u32Dst) = 0; CHK_TYPE(uint32_t, a_u32Dst); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_SREG_ZX_U64(a_u64Dst, a_iSReg)     do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_SREG_BASE_U64(a_u64Dst, a_iSReg)   do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_SREG_BASE_U32(a_u32Dst, a_iSReg)   do { (a_u32Dst) = 0; CHK_TYPE(uint32_t, a_u32Dst); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_EFLAGS(a_EFlags)                   do { (a_EFlags) = 0; CHK_TYPE(uint32_t, a_EFlags); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_EFLAGS_U8(a_EFlags)                do { (a_EFlags) = 0; CHK_TYPE(uint8_t,  a_EFlags); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_FSW(a_u16Fsw)                      do { (a_u16Fsw) = 0; CHK_TYPE(uint16_t, a_u16Fsw); (void)fFpuRead; (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_FCW(a_u16Fcw)                      do { (a_u16Fcw) = 0; CHK_TYPE(uint16_t, a_u16Fcw); (void)fFpuRead; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_GREG_U8(a_iGReg, a_u8Value)        do { CHK_GREG_IDX(a_iGReg); CHK_TYPE(uint8_t, a_u8Value); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_GREG_U16(a_iGReg, a_u16Value)      do { CHK_GREG_IDX(a_iGReg); CHK_TYPE(uint16_t, a_u16Value); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_GREG_U32(a_iGReg, a_u32Value)      do { CHK_GREG_IDX(a_iGReg); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_GREG_U64(a_iGReg, a_u64Value)      do { CHK_GREG_IDX(a_iGReg); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_GREG_I64(a_iGReg, a_i64Value)      do { CHK_GREG_IDX(a_iGReg); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_GREG_U8_CONST(a_iGReg, a_u8C)      do { CHK_GREG_IDX(a_iGReg); AssertCompile((uint8_t )(a_u8C)  == (a_u8C) ); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_GREG_U16_CONST(a_iGReg, a_u16C)    do { CHK_GREG_IDX(a_iGReg); AssertCompile((uint16_t)(a_u16C) == (a_u16C)); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_GREG_U32_CONST(a_iGReg, a_u32C)    do { CHK_GREG_IDX(a_iGReg); AssertCompile((uint32_t)(a_u32C) == (a_u32C)); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_GREG_U64_CONST(a_iGReg, a_u64C)    do { CHK_GREG_IDX(a_iGReg); AssertCompile((uint64_t)(a_u64C) == (a_u64C)); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_FPUREG_R80_SRC_REF(a_iSt, a_pr80Src) do { CHK_PTYPE(PCRTFLOAT80U, a_pr80Src); Assert((a_iSt) < 8); (void)fMcBegin; } while (0)
#define IEM_MC_CLEAR_HIGH_GREG_U64(a_iGReg)             do { CHK_GREG_IDX(a_iGReg); (void)fMcBegin;  } while (0)
#define IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(a_pu32Dst)    do { CHK_PTYPE(uint32_t *, a_pu32Dst); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_SREG_BASE_U64(a_iSeg, a_u64Value)  do { (void)fMcBegin; CHK_SEG_IDX(a_iSeg); } while (0)
#define IEM_MC_STORE_SREG_BASE_U32(a_iSeg, a_u32Value)  do { (void)fMcBegin; CHK_SEG_IDX(a_iSeg); } while (0)
#define IEM_MC_REF_GREG_U8(a_pu8Dst, a_iGReg)           do { CHK_GREG_IDX(a_iGReg); (a_pu8Dst)  = (uint8_t  *)((uintptr_t)0); CHK_PTYPE(uint8_t  *, a_pu8Dst);  (void)fMcBegin; } while (0)
#define IEM_MC_REF_GREG_U16(a_pu16Dst, a_iGReg)         do { CHK_GREG_IDX(a_iGReg); (a_pu16Dst) = (uint16_t *)((uintptr_t)0); CHK_PTYPE(uint16_t *, a_pu16Dst); (void)fMcBegin; } while (0)
#define IEM_MC_REF_GREG_U32(a_pu32Dst, a_iGReg)         do { CHK_GREG_IDX(a_iGReg); (a_pu32Dst) = (uint32_t *)((uintptr_t)0); CHK_PTYPE(uint32_t *, a_pu32Dst); (void)fMcBegin; } while (0)
#define IEM_MC_REF_GREG_I32(a_pi32Dst, a_iGReg)         do { CHK_GREG_IDX(a_iGReg); (a_pi32Dst) = (int32_t *)((uintptr_t)0); CHK_PTYPE(int32_t *, a_pi32Dst); (void)fMcBegin; } while (0)
#define IEM_MC_REF_GREG_I32_CONST(a_pi32Dst, a_iGReg)   do { CHK_GREG_IDX(a_iGReg); (a_pi32Dst) = (int32_t const *)((uintptr_t)0); CHK_PTYPE(int32_t const *, a_pi32Dst); (void)fMcBegin; } while (0)
#define IEM_MC_REF_GREG_U64(a_pu64Dst, a_iGReg)         do { CHK_GREG_IDX(a_iGReg); (a_pu64Dst) = (uint64_t *)((uintptr_t)0); CHK_PTYPE(uint64_t *, a_pu64Dst); (void)fMcBegin; } while (0)
#define IEM_MC_REF_GREG_I64(a_pi64Dst, a_iGReg)         do { CHK_GREG_IDX(a_iGReg); (a_pi64Dst) = (int64_t *)((uintptr_t)0); CHK_PTYPE(int64_t *, a_pi64Dst); (void)fMcBegin; } while (0)
#define IEM_MC_REF_GREG_I64_CONST(a_pi64Dst, a_iGReg)   do { CHK_GREG_IDX(a_iGReg); (a_pi64Dst) = (int64_t const *)((uintptr_t)0); CHK_PTYPE(int64_t const *, a_pi64Dst); (void)fMcBegin; } while (0)
#define IEM_MC_REF_EFLAGS(a_pEFlags)                    do { (a_pEFlags) = (uint32_t *)((uintptr_t)0); CHK_PTYPE(uint32_t *, a_pEFlags); (void)fMcBegin; } while (0)
#define IEM_MC_REF_FPUREG(a_pr80Dst, a_iSt)             do { (a_pr80Dst) = (PRTFLOAT80U)((uintptr_t)0); CHK_PTYPE(PCRTFLOAT80U, a_pr80Dst); AssertCompile((a_iSt) < 8); (void)fMcBegin; } while (0)
#define IEM_MC_REF_MXCSR(a_pfMxcsr)                     do { (a_pfMxcsr) = (uint32_t *)((uintptr_t)0); CHK_PTYPE(uint32_t *, a_pfMxcsr); (void)fMcBegin; (void)fSseRead; } while (0)

#define IEM_MC_ADD_GREG_U8(a_iGReg, a_u8Value)          do { CHK_GREG_IDX(a_iGReg); CHK_CONST(uint8_t,  a_u8Value);  (void)fMcBegin; } while (0)
#define IEM_MC_ADD_GREG_U16(a_iGReg, a_u16Value)        do { CHK_GREG_IDX(a_iGReg); CHK_CONST(uint16_t, a_u16Value); (void)fMcBegin; } while (0)
#define IEM_MC_ADD_GREG_U32(a_iGReg, a_u32Value)        do { CHK_GREG_IDX(a_iGReg); CHK_CONST(uint32_t, a_u32Value); (void)fMcBegin; } while (0)
#define IEM_MC_ADD_GREG_U64(a_iGReg, a_u64Value)        do { CHK_GREG_IDX(a_iGReg); CHK_CONST(uint64_t, a_u64Value); (void)fMcBegin; } while (0)
#define IEM_MC_SUB_GREG_U8(a_iGReg,  a_u8Value)         do { CHK_GREG_IDX(a_iGReg); CHK_CONST(uint8_t,  a_u8Value);  (void)fMcBegin; } while (0)
#define IEM_MC_SUB_GREG_U16(a_iGReg, a_u16Value)        do { CHK_GREG_IDX(a_iGReg); CHK_CONST(uint16_t, a_u16Value); (void)fMcBegin; } while (0)
#define IEM_MC_SUB_GREG_U32(a_iGReg, a_u32Value)        do { CHK_GREG_IDX(a_iGReg); CHK_CONST(uint32_t, a_u32Value); (void)fMcBegin; } while (0)
#define IEM_MC_SUB_GREG_U64(a_iGReg, a_u64Value)        do { CHK_GREG_IDX(a_iGReg); CHK_CONST(uint64_t, a_u64Value); (void)fMcBegin; } while (0)
#define IEM_MC_SUB_LOCAL_U16(a_u16Value, a_u16Const)    do { CHK_CONST(uint16_t, a_u16Const); (void)fMcBegin; } while (0)

#define IEM_MC_AND_GREG_U8(a_iGReg, a_u8Value)          do { CHK_GREG_IDX(a_iGReg); CHK_CONST(uint8_t,  a_u8Value);  (void)fMcBegin; } while (0)
#define IEM_MC_AND_GREG_U16(a_iGReg, a_u16Value)        do { CHK_GREG_IDX(a_iGReg); CHK_CONST(uint16_t, a_u16Value); (void)fMcBegin; } while (0)
#define IEM_MC_AND_GREG_U32(a_iGReg, a_u32Value)        do { CHK_GREG_IDX(a_iGReg); CHK_CONST(uint32_t, a_u32Value); (void)fMcBegin; } while (0)
#define IEM_MC_AND_GREG_U64(a_iGReg, a_u64Value)        do { CHK_GREG_IDX(a_iGReg); CHK_CONST(uint64_t, a_u64Value); (void)fMcBegin; } while (0)
#define IEM_MC_OR_GREG_U8(a_iGReg,  a_u8Value)          do { CHK_GREG_IDX(a_iGReg); CHK_CONST(uint8_t,  a_u8Value);  (void)fMcBegin; } while (0)
#define IEM_MC_OR_GREG_U16(a_iGReg, a_u16Value)         do { CHK_GREG_IDX(a_iGReg); CHK_CONST(uint16_t, a_u16Value); (void)fMcBegin; } while (0)
#define IEM_MC_OR_GREG_U32(a_iGReg, a_u32Value)         do { CHK_GREG_IDX(a_iGReg); CHK_CONST(uint32_t, a_u32Value); (void)fMcBegin; } while (0)
#define IEM_MC_OR_GREG_U64(a_iGReg, a_u64Value)         do { CHK_GREG_IDX(a_iGReg); CHK_CONST(uint64_t, a_u64Value); (void)fMcBegin; } while (0)

#define IEM_MC_ADD_GREG_U8_TO_LOCAL(a_u16Value, a_iGReg)   do { CHK_GREG_IDX(a_iGReg); (a_u8Value)  += 1; CHK_TYPE(uint8_t,  a_u8Value);  (void)fMcBegin; } while (0)
#define IEM_MC_ADD_GREG_U16_TO_LOCAL(a_u16Value, a_iGReg)  do { CHK_GREG_IDX(a_iGReg); (a_u16Value) += 1; CHK_TYPE(uint16_t, a_u16Value); (void)fMcBegin; } while (0)
#define IEM_MC_ADD_GREG_U32_TO_LOCAL(a_u32Value, a_iGReg)  do { CHK_GREG_IDX(a_iGReg); (a_u32Value) += 1; CHK_TYPE(uint32_t, a_u32Value); (void)fMcBegin; } while (0)
#define IEM_MC_ADD_GREG_U64_TO_LOCAL(a_u64Value, a_iGReg)  do { CHK_GREG_IDX(a_iGReg); (a_u64Value) += 1; CHK_TYPE(uint64_t, a_u64Value); (void)fMcBegin; } while (0)
#define IEM_MC_ADD_LOCAL_S16_TO_EFF_ADDR(a_EffAddr, a_i16) do { (a_EffAddr) += (a_i16); CHK_GCPTR(a_EffAddr); (void)fMcBegin; } while (0)
#define IEM_MC_ADD_LOCAL_S32_TO_EFF_ADDR(a_EffAddr, a_i32) do { (a_EffAddr) += (a_i32); CHK_GCPTR(a_EffAddr); (void)fMcBegin; } while (0)
#define IEM_MC_ADD_LOCAL_S64_TO_EFF_ADDR(a_EffAddr, a_i64) do { (a_EffAddr) += (a_i64); CHK_GCPTR(a_EffAddr); (void)fMcBegin; } while (0)
#define IEM_MC_AND_LOCAL_U8(a_u8Local, a_u8Mask)        do { (a_u8Local)  &= (a_u8Mask);  CHK_TYPE(uint8_t,  a_u8Local);  CHK_CONST(uint8_t,  a_u8Mask);  (void)fMcBegin; } while (0)
#define IEM_MC_AND_LOCAL_U16(a_u16Local, a_u16Mask)     do { (a_u16Local) &= (a_u16Mask); CHK_TYPE(uint16_t, a_u16Local); CHK_CONST(uint16_t, a_u16Mask); (void)fMcBegin; } while (0)
#define IEM_MC_AND_LOCAL_U32(a_u32Local, a_u32Mask)     do { (a_u32Local) &= (a_u32Mask); CHK_TYPE(uint32_t, a_u32Local); CHK_CONST(uint32_t, a_u32Mask); (void)fMcBegin; } while (0)
#define IEM_MC_AND_LOCAL_U64(a_u64Local, a_u64Mask)     do { (a_u64Local) &= (a_u64Mask); CHK_TYPE(uint64_t, a_u64Local); CHK_CONST(uint64_t, a_u64Mask); (void)fMcBegin; } while (0)
#define IEM_MC_AND_ARG_U16(a_u16Arg, a_u16Mask)         do { (a_u16Arg)   &= (a_u16Mask); CHK_TYPE(uint16_t, a_u16Arg);   CHK_CONST(uint16_t, a_u16Mask); (void)fMcBegin; } while (0)
#define IEM_MC_AND_ARG_U32(a_u32Arg, a_u32Mask)         do { (a_u32Arg)   &= (a_u32Mask); CHK_TYPE(uint32_t, a_u32Arg);   CHK_CONST(uint32_t, a_u32Mask); (void)fMcBegin; } while (0)
#define IEM_MC_AND_ARG_U64(a_u64Arg, a_u64Mask)         do { (a_u64Arg)   &= (a_u64Mask); CHK_TYPE(uint64_t, a_u64Arg);   CHK_CONST(uint64_t, a_u64Mask); (void)fMcBegin; } while (0)
#define IEM_MC_OR_LOCAL_U8(a_u8Local, a_u8Mask)         do { (a_u8Local)  |= (a_u8Mask);  CHK_TYPE(uint8_t,  a_u8Local);  CHK_CONST(uint8_t,  a_u8Mask);  (void)fMcBegin; } while (0)
#define IEM_MC_OR_LOCAL_U16(a_u16Local, a_u16Mask)      do { (a_u16Local) |= (a_u16Mask); CHK_TYPE(uint16_t, a_u16Local); CHK_CONST(uint16_t, a_u16Mask); (void)fMcBegin; } while (0)
#define IEM_MC_OR_LOCAL_U32(a_u32Local, a_u32Mask)      do { (a_u32Local) |= (a_u32Mask); CHK_TYPE(uint32_t, a_u32Local); CHK_CONST(uint32_t, a_u32Mask); (void)fMcBegin; } while (0)
#define IEM_MC_SAR_LOCAL_S16(a_i16Local, a_cShift)      do { (a_i16Local) >>= (a_cShift); CHK_TYPE(int16_t, a_i16Local);  CHK_CONST(uint8_t,  a_cShift);  (void)fMcBegin; } while (0)
#define IEM_MC_SAR_LOCAL_S32(a_i32Local, a_cShift)      do { (a_i32Local) >>= (a_cShift); CHK_TYPE(int32_t, a_i32Local);  CHK_CONST(uint8_t,  a_cShift);  (void)fMcBegin; } while (0)
#define IEM_MC_SAR_LOCAL_S64(a_i64Local, a_cShift)      do { (a_i64Local) >>= (a_cShift); CHK_TYPE(int64_t, a_i64Local);  CHK_CONST(uint8_t,  a_cShift);  (void)fMcBegin; } while (0)
#define IEM_MC_SHR_LOCAL_U8(a_u8Local, a_cShift)        do { (a_u8Local)  >>= (a_cShift); CHK_TYPE(uint8_t, a_u8Local);   CHK_CONST(uint8_t,  a_cShift);  (void)fMcBegin; } while (0)
#define IEM_MC_SHL_LOCAL_S16(a_i16Local, a_cShift)      do { (a_i16Local) <<= (a_cShift); CHK_TYPE(int16_t, a_i16Local);  CHK_CONST(uint8_t,  a_cShift);  (void)fMcBegin; } while (0)
#define IEM_MC_SHL_LOCAL_S32(a_i32Local, a_cShift)      do { (a_i32Local) <<= (a_cShift); CHK_TYPE(int32_t, a_i32Local);  CHK_CONST(uint8_t,  a_cShift);  (void)fMcBegin; } while (0)
#define IEM_MC_SHL_LOCAL_S64(a_i64Local, a_cShift)      do { (a_i64Local) <<= (a_cShift); CHK_TYPE(int64_t, a_i64Local);  CHK_CONST(uint8_t,  a_cShift);  (void)fMcBegin; } while (0)
#define IEM_MC_AND_2LOCS_U32(a_u32Local, a_u32Mask)     do { (a_u32Local) &= (a_u32Mask); CHK_TYPE(uint32_t, a_u32Local); (void)fMcBegin; } while (0)
#define IEM_MC_OR_2LOCS_U32(a_u32Local, a_u32Mask)      do { (a_u32Local) |= (a_u32Mask); CHK_TYPE(uint32_t, a_u32Local); (void)fMcBegin; } while (0)
#define IEM_MC_SET_EFL_BIT(a_fBit)                      do { CHK_SINGLE_BIT(uint32_t, a_fBit); (void)fMcBegin; } while (0)
#define IEM_MC_CLEAR_EFL_BIT(a_fBit)                    do { CHK_SINGLE_BIT(uint32_t, a_fBit); (void)fMcBegin; } while (0)
#define IEM_MC_FLIP_EFL_BIT(a_fBit)                     do { CHK_SINGLE_BIT(uint32_t, a_fBit); (void)fMcBegin; } while (0)
#define IEM_MC_CLEAR_FSW_EX()                           do { (void)fMcBegin; } while (0)
#define IEM_MC_FPU_TO_MMX_MODE()                        do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_FPU_FROM_MMX_MODE()                      do { (void)fMcBegin; } while (0)

#define IEM_MC_BSWAP_LOCAL_U16(a_u16Local)              do { CHK_TYPE(uint16_t,  a_u16Local); (void)fMcBegin; } while (0)
#define IEM_MC_BSWAP_LOCAL_U32(a_u32Local)              do { CHK_TYPE(uint32_t,  a_u32Local);( void)fMcBegin; } while (0)
#define IEM_MC_BSWAP_LOCAL_U64(a_u64Local)              do { CHK_TYPE(uint64_t,  a_u64Local); (void)fMcBegin; } while (0)

#define IEM_MC_FETCH_MREG_U64(a_u64Value, a_iMReg)          do { CHK_MREG_IDX(a_iMReg); (a_u64Value) = 0; CHK_TYPE(uint64_t, a_u64Value); (void)fFpuRead; (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MREG_U32(a_u32Value, a_iMReg)          do { CHK_MREG_IDX(a_iMReg); (a_u32Value) = 0; CHK_TYPE(uint32_t, a_u32Value); (void)fFpuRead; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MREG_U64(a_iMReg, a_u64Value)          do { CHK_MREG_IDX(a_iMReg); CHK_TYPE(uint64_t, a_u64Value); (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MREG_U32_ZX_U64(a_iMReg, a_u32Value)   do { CHK_MREG_IDX(a_iMReg); CHK_TYPE(uint32_t, a_u32Value); (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_REF_MREG_U64(a_pu64Dst, a_iMReg)             do { CHK_MREG_IDX(a_iMReg); (a_pu64Dst) = (uint64_t *)((uintptr_t)0); CHK_PTYPE(uint64_t *, a_pu64Dst);             (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_REF_MREG_U64_CONST(a_pu64Dst, a_iMReg)       do { CHK_MREG_IDX(a_iMReg); (a_pu64Dst) = (uint64_t const *)((uintptr_t)0); CHK_PTYPE(uint64_t const *, a_pu64Dst); (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_REF_MREG_U32_CONST(a_pu32Dst, a_iMReg)       do { CHK_MREG_IDX(a_iMReg); (a_pu32Dst) = (uint32_t const *)((uintptr_t)0); CHK_PTYPE(uint32_t const *, a_pu32Dst); (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_MODIFIED_MREG(a_iMReg)                       do { CHK_MREG_IDX(a_iMReg); (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_MODIFIED_MREG_BY_REF(a_pu64Dst)              do { AssertCompile(sizeof(*a_pu64Dst) <= sizeof(uint64_t)); (void)fFpuWrite; (void)fMcBegin; } while (0)

#define IEM_MC_CLEAR_XREG_U32_MASK(a_iXReg, a_bMask)        do { CHK_XREG_IDX(a_iXReg); CHK_TYPE(uint8_t, a_bMask); (void)fSseRead;  (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_XREG_U128(a_u128Value, a_iXReg)        do { CHK_XREG_IDX(a_iXReg); (a_u128Value) = g_u128Zero; CHK_TYPE(RTUINT128U, a_u128Value); (void)fSseRead;  (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_XREG_XMM(a_XmmValue, a_iXReg)          do { CHK_XREG_IDX(a_iXReg); (a_XmmValue) = g_XmmZero; CHK_TYPE(X86XMMREG, a_XmmValue); (void)fSseRead;  (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_XREG_U64(a_u64Value, a_iXReg, a_iQWord)    do { CHK_XREG_IDX(a_iXReg); (a_u64Value) = 0; CHK_TYPE(uint64_t, a_u64Value); (void)fSseRead; (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_XREG_U32(a_u32Value, a_iXReg, a_iDWord)    do { CHK_XREG_IDX(a_iXReg); (a_u32Value) = 0; CHK_TYPE(uint32_t, a_u32Value); (void)fSseRead; (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_XREG_U16(a_u16Value, a_iXReg, a_iWord )    do { CHK_XREG_IDX(a_iXReg); (a_u16Value) = 0; CHK_TYPE(uint16_t, a_u16Value); (void)fSseRead; (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_XREG_U8( a_u8Value,  a_iXReg, a_iByte)     do { CHK_XREG_IDX(a_iXReg); (a_u8Value)  = 0; CHK_TYPE(uint8_t,  a_u8Value);  (void)fSseRead; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_XREG_U32_U128(a_iXReg, a_iDwDst, a_u128Value, a_iDwSrc)    do { CHK_XREG_IDX(a_iXReg); CHK_TYPE(RTUINT128U, a_u128Value);  AssertCompile((a_iDwDst) < RT_ELEMENTS((a_u128Value).au32)); AssertCompile((a_iDwSrc) < RT_ELEMENTS((a_u128Value).au32)); (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_XREG_U128(a_iXReg, a_u128Value)        do { CHK_TYPE(RTUINT128U, a_u128Value); (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_XREG_XMM(a_iXReg, a_XmmValue)          do { CHK_TYPE(X86XMMREG, a_XmmValue); (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_XREG_XMM_U32(a_iXReg, a_iDword, a_XmmValue)      do { CHK_XREG_IDX(a_iXReg); CHK_TYPE(X86XMMREG, a_XmmValue);  AssertCompile((a_iDword) < RT_ELEMENTS((a_XmmValue).au32)); (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_XREG_XMM_U64(a_iXReg, a_iQword, a_XmmValue)      do { CHK_XREG_IDX(a_iXReg); CHK_TYPE(X86XMMREG, a_XmmValue);  AssertCompile((a_iQword) < RT_ELEMENTS((a_XmmValue).au64)); (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_XREG_U64(a_iXReg, a_iQword, a_u64Value)          do { CHK_XREG_IDX(a_iXReg); CHK_TYPE(uint64_t,  a_u64Value);  (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_XREG_U64_ZX_U128(a_iXReg, a_u64Value)  do { CHK_XREG_IDX(a_iXReg); CHK_TYPE(uint64_t,  a_u64Value);  (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_XREG_U32(a_iXReg, a_iDword, a_u32Value)    do { CHK_XREG_IDX(a_iXReg); CHK_TYPE(uint32_t,  a_u32Value); (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_XREG_U16(a_iXReg, a_iWord,  a_u16Value)    do { CHK_XREG_IDX(a_iXReg); CHK_TYPE(uint16_t,  a_u16Value); (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_XREG_U8( a_iXReg, a_iByte,  a_u8Value )    do { CHK_XREG_IDX(a_iXReg); CHK_TYPE(uint8_t,   a_u8Value ); (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_XREG_U32_ZX_U128(a_iXReg, a_u32Value)  do { CHK_XREG_IDX(a_iXReg); CHK_TYPE(uint32_t,  a_u32Value);  (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_XREG_HI_U64(a_iXReg, a_u64Value)       do { CHK_XREG_IDX(a_iXReg); CHK_TYPE(uint64_t,  a_u64Value);  (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_XREG_R32(a_iXReg, a_r32Value)          do { CHK_XREG_IDX(a_iXReg); CHK_TYPE(RTFLOAT32U,  a_r32Value);  (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_XREG_R64(a_iXReg, a_r64Value)          do { CHK_XREG_IDX(a_iXReg); CHK_TYPE(RTFLOAT64U,  a_r64Value);  (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_REF_XREG_U128(a_pu128Dst, a_iXReg)           do { CHK_XREG_IDX(a_iXReg); (a_pu128Dst) = (PRTUINT128U)((uintptr_t)0);        CHK_PTYPE(PRTUINT128U, a_pu128Dst);     (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_REF_XREG_U128_CONST(a_pu128Dst, a_iXReg)     do { CHK_XREG_IDX(a_iXReg); (a_pu128Dst) = (PCRTUINT128U)((uintptr_t)0);       CHK_PTYPE(PCRTUINT128U, a_pu128Dst);    (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_REF_XREG_U32_CONST(a_pu32Dst, a_iXReg)       do { CHK_XREG_IDX(a_iXReg); (a_pu32Dst)  = (uint32_t const *)((uintptr_t)0);   CHK_PTYPE(uint32_t const *, a_pu32Dst); (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_REF_XREG_U64_CONST(a_pu64Dst, a_iXReg)       do { CHK_XREG_IDX(a_iXReg); (a_pu64Dst)  = (uint64_t const *)((uintptr_t)0);   CHK_PTYPE(uint64_t const *, a_pu64Dst); (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_REF_XREG_R32_CONST(a_pr32Dst, a_iXReg)       do { CHK_XREG_IDX(a_iXReg); (a_pr32Dst)  = (RTFLOAT32U const *)((uintptr_t)0); CHK_PTYPE(RTFLOAT32U const *, a_pr32Dst); (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_REF_XREG_R64_CONST(a_pr64Dst, a_iXReg)       do { CHK_XREG_IDX(a_iXReg); (a_pr64Dst)  = (RTFLOAT64U const *)((uintptr_t)0); CHK_PTYPE(RTFLOAT64U const *, a_pr64Dst); (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_REF_XREG_XMM_CONST(a_pXmmDst, a_iXReg)       do { CHK_XREG_IDX(a_iXReg); (a_pXmmDst) = (PCX86XMMREG)((uintptr_t)0);         CHK_PTYPE(PCX86XMMREG, a_pXmmDst);      (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_COPY_XREG_U128(a_iXRegDst, a_iXRegSrc)       do { CHK_XREG_IDX(a_iXRegDst); CHK_XREG_IDX(a_iXRegSrc); (void)fSseWrite; (void)fMcBegin; } while (0)

#define IEM_MC_FETCH_YREG_U256(a_u256Value, a_iYRegSrc)           do { CHK_YREG_IDX(a_iYRegSrc); (a_u256Value).au64[0] = (a_u256Value).au64[1] = (a_u256Value).au64[2] = (a_u256Value).au64[3] = 0; CHK_TYPE(RTUINT256U, a_u256Value); (void)fAvxRead; (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_YREG_U128(a_u128Value, a_iYRegSrc)           do { CHK_YREG_IDX(a_iYRegSrc); (a_u128Value).au64[0] = (a_u128Value).au64[1] = 0; CHK_TYPE(RTUINT128U, a_u128Value); (void)fAvxRead; (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_YREG_U64(a_u64Value, a_iYRegSrc)             do { CHK_YREG_IDX(a_iYRegSrc); (a_u64Value) = UINT64_MAX; CHK_TYPE(uint64_t, a_u64Value); (void)fAvxRead; (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_YREG_2ND_U64(a_u64Value, a_iYRegSrc)         do { CHK_YREG_IDX(a_iYRegSrc); (a_u64Value) = UINT64_MAX; CHK_TYPE(uint64_t, a_u64Value); (void)fAvxRead; (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_YREG_U32(a_u32Value, a_iYRegSrc)             do { CHK_YREG_IDX(a_iYRegSrc); (a_u32Value) = UINT32_MAX; CHK_TYPE(uint32_t, a_u32Value); (void)fAvxRead; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_YREG_U32_ZX_VLMAX(a_iYRegDst, a_u32Value)    do { CHK_YREG_IDX(a_iYRegDst); CHK_TYPE(uint32_t, a_u32Value); (void)fAvxWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_YREG_U64_ZX_VLMAX(a_iYRegDst, a_u64Value)    do { CHK_YREG_IDX(a_iYRegDst); CHK_TYPE(uint64_t, a_u64Value); (void)fAvxWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_YREG_U128_ZX_VLMAX(a_iYRegDst, a_u128Value)  do { CHK_YREG_IDX(a_iYRegDst); CHK_TYPE(RTUINT128U, a_u128Value); (void)fAvxWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_YREG_U256_ZX_VLMAX(a_iYRegDst, a_u256Value)  do { CHK_YREG_IDX(a_iYRegDst); CHK_TYPE(RTUINT256U, a_u256Value); (void)fAvxWrite; (void)fMcBegin; } while (0)
#define IEM_MC_REF_YREG_U128(a_pu128Dst, a_iYReg)                 do { CHK_YREG_IDX(a_iYReg); (a_pu128Dst) = (PRTUINT128U)((uintptr_t)0);      CHK_PTYPE(PRTUINT128U, a_pu128Dst);       (void)fAvxWrite; (void)fMcBegin; } while (0)
#define IEM_MC_REF_YREG_U128_CONST(a_pu128Dst, a_iYReg)           do { CHK_YREG_IDX(a_iYReg); (a_pu128Dst) = (PCRTUINT128U)((uintptr_t)0);     CHK_PTYPE(PCRTUINT128U, a_pu128Dst);      (void)fAvxWrite; (void)fMcBegin; } while (0)
#define IEM_MC_REF_YREG_U64_CONST(a_pu64Dst, a_iYReg)             do { CHK_YREG_IDX(a_iYReg); (a_pu64Dst)  = (uint64_t const *)((uintptr_t)0); CHK_PTYPE(uint64_t const *, a_pu64Dst);   (void)fAvxWrite; (void)fMcBegin; } while (0)
#define IEM_MC_CLEAR_YREG_128_UP(a_iYReg)                         do { CHK_YREG_IDX(a_iYReg); (void)fAvxWrite; (void)fMcBegin; } while (0)
#define IEM_MC_COPY_YREG_U256_ZX_VLMAX(a_iYRegDst, a_iYRegSrc)    do { CHK_YREG_IDX(a_iYRegDst); CHK_YREG_IDX(a_iYRegSrc); (void)fAvxWrite; (void)fMcBegin; } while (0)
#define IEM_MC_COPY_YREG_U128_ZX_VLMAX(a_iYRegDst, a_iYRegSrc)    do { CHK_YREG_IDX(a_iYRegDst); CHK_YREG_IDX(a_iYRegSrc); (void)fAvxWrite; (void)fMcBegin; } while (0)
#define IEM_MC_COPY_YREG_U64_ZX_VLMAX(a_iYRegDst, a_iYRegSrc)     do { CHK_YREG_IDX(a_iYRegDst); CHK_YREG_IDX(a_iYRegSrc); (void)fAvxWrite; (void)fMcBegin; } while (0)
#define IEM_MC_MERGE_YREG_U32_U96_ZX_VLMAX(a_iYRegDst, a_iYRegSrc32, a_iYRegSrcHx)        do { CHK_YREG_IDX(a_iYRegDst); CHK_YREG_IDX(a_iYRegSrcHx); CHK_YREG_IDX(a_iYRegSrc32); (void)fAvxWrite; (void)fAvxRead; (void)fMcBegin; } while (0)
#define IEM_MC_MERGE_YREG_U64_U64_ZX_VLMAX(a_iYRegDst, a_iYRegSrc64, a_iYRegSrcHx)        do { CHK_YREG_IDX(a_iYRegDst); CHK_YREG_IDX(a_iYRegSrcHx); CHK_YREG_IDX(a_iYRegSrc64); (void)fAvxWrite; (void)fAvxRead; (void)fMcBegin; } while (0)
#define IEM_MC_MERGE_YREG_U64HI_U64HI_ZX_VLMAX(a_iYRegDst, a_iYRegSrc64, a_iYRegSrcHx)    do { CHK_YREG_IDX(a_iYRegDst); CHK_YREG_IDX(a_iYRegSrcHx); CHK_YREG_IDX(a_iYRegSrc64); (void)fAvxWrite; (void)fAvxRead; (void)fMcBegin; } while (0)
#define IEM_MC_MERGE_YREG_U64LO_U64LO_ZX_VLMAX(a_iYRegDst, a_iYRegSrc64, a_iYRegSrcHx)    do { CHK_YREG_IDX(a_iYRegDst); CHK_YREG_IDX(a_iYRegSrcHx); CHK_YREG_IDX(a_iYRegSrc64); (void)fAvxWrite; (void)fAvxRead; (void)fMcBegin; } while (0)
#define IEM_MC_MERGE_YREG_U64LO_U64LOCAL_ZX_VLMAX(a_iYRegDst, a_iYRegSrcHx, a_u64Local)   do { CHK_YREG_IDX(a_iYRegDst); CHK_YREG_IDX(a_iYRegSrcHx); (void)fAvxWrite; (void)fAvxRead; (void)fMcBegin; } while (0)
#define IEM_MC_MERGE_YREG_U64LOCAL_U64HI_ZX_VLMAX(a_iYRegDst, a_u64Local, a_iYRegSrcHx)   do { CHK_YREG_IDX(a_iYRegDst); CHK_YREG_IDX(a_iYRegSrcHx); (void)fAvxWrite; (void)fAvxRead; (void)fMcBegin; } while (0)

#define IEM_MC_FETCH_MEM_U8(a_u8Dst, a_iSeg, a_GCPtrMem)                do { CHK_GCPTR(a_GCPtrMem); AssertCompile(sizeof(a_u8Dst) == (sizeof(uint8_t))); CHK_SEG_IDX(a_iSeg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM16_U8(a_u8Dst, a_iSeg, a_GCPtrMem16)            do { CHK_TYPE(uint16_t, a_GCPtrMem16); AssertCompile(sizeof(a_u8Dst) == (sizeof(uint8_t))); CHK_SEG_IDX(a_iSeg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM32_U8(a_u8Dst, a_iSeg, a_GCPtrMem32)            do { CHK_TYPE(uint32_t, a_GCPtrMem32); AssertCompile(sizeof(a_u8Dst) == (sizeof(uint8_t))); CHK_SEG_IDX(a_iSeg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U16(a_u16Dst, a_iSeg, a_GCPtrMem)              do { CHK_GCPTR(a_GCPtrMem); AssertCompile(sizeof(a_u16Dst) == (sizeof(uint16_t))); CHK_SEG_IDX(a_iSeg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_I16(a_i16Dst, a_iSeg, a_GCPtrMem)              do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(int16_t, a_i16Dst); CHK_SEG_IDX(a_iSeg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U32(a_u32Dst, a_iSeg, a_GCPtrMem)              do { CHK_GCPTR(a_GCPtrMem); AssertCompile(sizeof(a_u32Dst) == (sizeof(uint32_t))); CHK_SEG_IDX(a_iSeg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_I32(a_i32Dst, a_iSeg, a_GCPtrMem)              do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(int32_t, a_i32Dst); CHK_SEG_IDX(a_iSeg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_S32_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem)       do { CHK_GCPTR(a_GCPtrMem); AssertCompile(sizeof(a_u64Dst) == (sizeof(uint64_t))); CHK_SEG_IDX(a_iSeg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U64(a_u64Dst, a_iSeg, a_GCPtrMem)              do { CHK_GCPTR(a_GCPtrMem); AssertCompile(sizeof(a_u64Dst) == (sizeof(uint64_t))); CHK_SEG_IDX(a_iSeg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U64_ALIGN_U128(a_u64Dst, a_iSeg, a_GCPtrMem)   do { CHK_GCPTR(a_GCPtrMem); AssertCompile(sizeof(a_u64Dst) == (sizeof(uint64_t))); CHK_SEG_IDX(a_iSeg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_I64(a_i64Dst, a_iSeg, a_GCPtrMem)              do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(int64_t, a_i64Dst); CHK_SEG_IDX(a_iSeg); (void)fMcBegin; } while (0)

#define IEM_MC_FETCH_MEM_U8_DISP(a_u8Dst, a_iSeg, a_GCPtrMem, a_offDisp) \
    do { CHK_GCPTR(a_GCPtrMem); CHK_CONST(uint8_t, a_offDisp); CHK_TYPE(uint8_t, a_u8Dst); CHK_SEG_IDX(a_iSeg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U16_DISP(a_u16Dst, a_iSeg, a_GCPtrMem, a_offDisp) \
    do { CHK_GCPTR(a_GCPtrMem); CHK_CONST(uint8_t, a_offDisp); CHK_TYPE(uint16_t, a_u16Dst); CHK_SEG_IDX(a_iSeg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U32_DISP(a_u32Dst, a_iSeg, a_GCPtrMem, a_offDisp) \
    do { CHK_GCPTR(a_GCPtrMem); CHK_CONST(uint8_t, a_offDisp); CHK_TYPE(uint32_t, a_u32Dst); CHK_SEG_IDX(a_iSeg); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U64_DISP(a_u64Dst, a_iSeg, a_GCPtrMem, a_offDisp) \
    do { CHK_GCPTR(a_GCPtrMem); CHK_CONST(uint8_t, a_offDisp); CHK_TYPE(uint64_t, a_u64Dst); CHK_SEG_IDX(a_iSeg); (void)fMcBegin; } while (0)

#define IEM_MC_FETCH_MEM_U8_ZX_U16(a_u16Dst, a_iSeg, a_GCPtrMem)        do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); AssertCompile(sizeof(a_u16Dst) == (sizeof(uint16_t))); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U8_ZX_U32(a_u32Dst, a_iSeg, a_GCPtrMem)        do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); AssertCompile(sizeof(a_u32Dst) == (sizeof(uint32_t))); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U8_ZX_U64(a_u64Dst, a_iSeg, a_GCPtrMem)        do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); AssertCompile(sizeof(a_u64Dst) == (sizeof(uint64_t))); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U16_ZX_U32(a_u32Dst, a_iSeg, a_GCPtrMem)       do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); AssertCompile(sizeof(a_u32Dst) == (sizeof(uint32_t))); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U16_ZX_U64(a_u64Dst, a_iSeg, a_GCPtrMem)       do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); AssertCompile(sizeof(a_u64Dst) == (sizeof(uint64_t))); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U32_ZX_U64(a_u64Dst, a_iSeg, a_GCPtrMem)       do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); AssertCompile(sizeof(a_u64Dst) == (sizeof(uint64_t))); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U8_SX_U16(a_u16Dst, a_iSeg, a_GCPtrMem)        do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); AssertCompile(sizeof(a_u16Dst) == (sizeof(uint16_t))); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U8_SX_U32(a_u32Dst, a_iSeg, a_GCPtrMem)        do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); AssertCompile(sizeof(a_u32Dst) == (sizeof(uint32_t))); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U8_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem)        do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); AssertCompile(sizeof(a_u64Dst) == (sizeof(uint64_t))); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U16_SX_U32(a_u32Dst, a_iSeg, a_GCPtrMem)       do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); AssertCompile(sizeof(a_u32Dst) == (sizeof(uint32_t))); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U16_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem)       do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); AssertCompile(sizeof(a_u64Dst) == (sizeof(uint64_t))); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U32_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem)       do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); AssertCompile(sizeof(a_u64Dst) == (sizeof(uint64_t))); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_R32(a_r32Dst, a_iSeg, a_GCPtrMem)              do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTFLOAT32U, a_r32Dst); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_R64(a_r64Dst, a_iSeg, a_GCPtrMem)              do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTFLOAT64U, a_r64Dst); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_R80(a_r80Dst, a_iSeg, a_GCPtrMem)              do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTFLOAT80U, a_r80Dst); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_D80(a_d80Dst, a_iSeg, a_GCPtrMem)              do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTPBCD80U,  a_d80Dst); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U128(a_u128Dst, a_iSeg, a_GCPtrMem)            do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTUINT128U, a_u128Dst); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U128_NO_AC(a_u128Dst, a_iSeg, a_GCPtrMem)      do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTUINT128U, a_u128Dst); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U128_ALIGN_SSE(a_u128Dst, a_iSeg, a_GCPtrMem)  do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTUINT128U, a_u128Dst); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_XMM(a_XmmDst, a_iSeg, a_GCPtrMem)              do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(X86XMMREG, a_XmmDst); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_XMM_NO_AC(a_XmmDst, a_iSeg, a_GCPtrMem)        do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(X86XMMREG, a_XmmDst); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_XMM_ALIGN_SSE(a_XmmDst, a_iSeg, a_GCPtrMem)    do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(X86XMMREG, a_XmmDst); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_XMM_U32(a_XmmDst, a_iDWord, a_iSeg, a_GCPtrMem) do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(X86XMMREG, a_XmmDst); AssertCompile((a_iDWord) < RT_ELEMENTS((a_XmmDst).au32)); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_XMM_U64(a_XmmDst, a_iQWord, a_iSeg, a_GCPtrMem) do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(X86XMMREG, a_XmmDst); AssertCompile((a_iQWord) < RT_ELEMENTS((a_XmmDst).au64)); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U256(a_u256Dst, a_iSeg, a_GCPtrMem)            do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTUINT256U, a_u256Dst); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U256_NO_AC(a_u256Dst, a_iSeg, a_GCPtrMem)      do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTUINT256U, a_u256Dst); (void)fMcBegin; } while (0)
#define IEM_MC_FETCH_MEM_U256_ALIGN_AVX(a_u256Dst, a_iSeg, a_GCPtrMem)  do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTUINT256U, a_u256Dst); (void)fMcBegin; } while (0)

#define IEM_MC_STORE_MEM_U8(a_iSeg, a_GCPtrMem, a_u8Value)              do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(uint8_t,  a_u8Value); CHK_SEG_IDX(a_iSeg); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MEM_U16(a_iSeg, a_GCPtrMem, a_u16Value)            do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(uint16_t, a_u16Value);      (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MEM_U32(a_iSeg, a_GCPtrMem, a_u32Value)            do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(uint32_t, a_u32Value);      (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MEM_U64(a_iSeg, a_GCPtrMem, a_u64Value)            do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(uint64_t, a_u64Value);      (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MEM_U8_CONST(a_iSeg, a_GCPtrMem, a_u8C)            do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_CONST(uint8_t,  a_u8C);          (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MEM_U16_CONST(a_iSeg, a_GCPtrMem, a_u16C)          do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_CONST(uint16_t, a_u16C);         (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MEM_U32_CONST(a_iSeg, a_GCPtrMem, a_u32C)          do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_CONST(uint32_t, a_u32C);         (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MEM_U64_CONST(a_iSeg, a_GCPtrMem, a_u64C)          do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_CONST(uint64_t, a_u64C);         (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MEM_I8_CONST_BY_REF( a_pi8Dst,  a_i8C)             do { CHK_TYPE(int8_t *,  a_pi8Dst);  CHK_CONST(int8_t,  a_i8C);  (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MEM_I16_CONST_BY_REF(a_pi16Dst, a_i16C)            do { CHK_TYPE(int16_t *, a_pi16Dst); CHK_CONST(int16_t, a_i16C); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MEM_I32_CONST_BY_REF(a_pi32Dst, a_i32C)            do { CHK_TYPE(int32_t *, a_pi32Dst); CHK_CONST(int32_t, a_i32C); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MEM_I64_CONST_BY_REF(a_pi64Dst, a_i64C)            do { CHK_TYPE(int64_t *, a_pi64Dst); CHK_CONST(int64_t, a_i64C); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MEM_NEG_QNAN_R32_BY_REF(a_pr32Dst)                 do { CHK_TYPE(PRTFLOAT32U, a_pr32Dst); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MEM_NEG_QNAN_R64_BY_REF(a_pr64Dst)                 do { CHK_TYPE(PRTFLOAT64U, a_pr64Dst); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MEM_NEG_QNAN_R80_BY_REF(a_pr80Dst)                 do { CHK_TYPE(PRTFLOAT80U, a_pr80Dst); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MEM_INDEF_D80_BY_REF(a_pd80Dst)                    do { CHK_TYPE(PRTPBCD80U, a_pd80Dst); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MEM_U128(a_iSeg, a_GCPtrMem, a_u128Src)            do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTUINT128U, a_u128Src); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MEM_U128_ALIGN_SSE(a_iSeg, a_GCPtrMem, a_u128Src)  do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTUINT128U, a_u128Src); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MEM_U256(a_iSeg, a_GCPtrMem, a_u256Src)            do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTUINT256U, a_u256Src); (void)fMcBegin; } while (0)
#define IEM_MC_STORE_MEM_U256_ALIGN_AVX(a_iSeg, a_GCPtrMem, a_u256Src)  do { CHK_SEG_IDX(a_iSeg); CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTUINT256U, a_u256Src); (void)fMcBegin; } while (0)

#define IEM_MC_PUSH_U16(a_u16Value)                                     do { (void)fMcBegin; } while (0)
#define IEM_MC_PUSH_U32(a_u32Value)                                     do { (void)fMcBegin; } while (0)
#define IEM_MC_PUSH_U32_SREG(a_u32Value)                                do { (void)fMcBegin; } while (0)
#define IEM_MC_PUSH_U64(a_u64Value)                                     do { (void)fMcBegin; } while (0)
#define IEM_MC_POP_U16(a_pu16Value)                                     do { (void)fMcBegin; } while (0)
#define IEM_MC_POP_U32(a_pu32Value)                                     do { (void)fMcBegin; } while (0)
#define IEM_MC_POP_U64(a_pu64Value)                                     do { (void)fMcBegin; } while (0)
#define IEM_MC_MEM_MAP(a_pMem, a_fAccess, a_iSeg, a_GCPtrMem, a_iArg)   do { CHK_SEG_IDX(a_iSeg); (void)fMcBegin; } while (0)
#define IEM_MC_MEM_MAP_EX(a_pvMem, a_fAccess, a_cbMem, a_iSeg, a_GCPtrMem, a_cbAlign, a_iArg)  do { CHK_SEG_IDX(a_iSeg); (void)fMcBegin; AssertCompile((a_cbAlign) <= (a_cbMem)); } while (0)
#define IEM_MC_MEM_COMMIT_AND_UNMAP(a_pvMem, a_fAccess)                             do { (void)fMcBegin; } while (0)
#define IEM_MC_MEM_COMMIT_AND_UNMAP_FOR_FPU_STORE(a_pvMem, a_fAccess, a_u16FSW)     do { (void)fMcBegin; } while (0)
#define IEM_MC_CALC_RM_EFF_ADDR(a_GCPtrEff, bRm, cbImm)                 do { (a_GCPtrEff) = 0; CHK_GCPTR(a_GCPtrEff); (void)fMcBegin; } while (0)
#define IEM_MC_CALL_VOID_AIMPL_0(a_pfn)                                 do { (void)fMcBegin; } while (0)
#define IEM_MC_CALL_VOID_AIMPL_1(a_pfn, a0) \
    do { CHK_CALL_ARG(a0, 0); (void)fMcBegin; } while (0)
#define IEM_MC_CALL_VOID_AIMPL_2(a_pfn, a0, a1) \
    do { CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); (void)fMcBegin; } while (0)
#define IEM_MC_CALL_VOID_AIMPL_3(a_pfn, a0, a1, a2) \
    do { CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); (void)fMcBegin; } while (0)
#define IEM_MC_CALL_VOID_AIMPL_4(a_pfn, a0, a1, a2, a3) \
    do { CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); CHK_CALL_ARG(a3, 3); (void)fMcBegin; } while (0)
#define IEM_MC_CALL_AIMPL_3(a_rc, a_pfn, a0, a1, a2) \
    do { CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2);  (a_rc) = VINF_SUCCESS; (void)fMcBegin; } while (0)
#define IEM_MC_CALL_AIMPL_4(a_rc, a_pfn, a0, a1, a2, a3) \
    do { CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); CHK_CALL_ARG(a3, 3);  (a_rc) = VINF_SUCCESS; (void)fMcBegin; } while (0)
#define IEM_MC_CALL_CIMPL_0(a_pfnCImpl)                                 do { (void)fMcBegin; } while (0)
#define IEM_MC_CALL_CIMPL_1(a_pfnCImpl, a0) \
    do { CHK_CALL_ARG(a0, 0);  (void)fMcBegin; } while (0)
#define IEM_MC_CALL_CIMPL_2(a_pfnCImpl, a0, a1) \
    do { CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); (void)fMcBegin; } while (0)
#define IEM_MC_CALL_CIMPL_3(a_pfnCImpl, a0, a1, a2) \
    do { CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); (void)fMcBegin; } while (0)
#define IEM_MC_CALL_CIMPL_4(a_pfnCImpl, a0, a1, a2, a3) \
    do { CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); CHK_CALL_ARG(a3, 3); (void)fMcBegin; } while (0)
#define IEM_MC_CALL_CIMPL_5(a_pfnCImpl, a0, a1, a2, a3, a4) \
    do { CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); CHK_CALL_ARG(a3, 3); CHK_CALL_ARG(a4, 4); (void)fMcBegin; } while (0)
#define IEM_MC_DEFER_TO_CIMPL_0(a_pfnCImpl)                             (VINF_SUCCESS)
#define IEM_MC_DEFER_TO_CIMPL_1(a_pfnCImpl, a0)                         (VINF_SUCCESS)
#define IEM_MC_DEFER_TO_CIMPL_2(a_pfnCImpl, a0, a1)                     (VINF_SUCCESS)
#define IEM_MC_DEFER_TO_CIMPL_3(a_pfnCImpl, a0, a1, a2)                 (VINF_SUCCESS)

#define IEM_MC_CALL_FPU_AIMPL_1(a_pfnAImpl, a0) \
    do { (void)fFpuHost; (void)fFpuWrite; CHK_CALL_ARG(a0, 0);  (void)fMcBegin; } while (0)
#define IEM_MC_CALL_FPU_AIMPL_2(a_pfnAImpl, a0, a1) \
    do { (void)fFpuHost; (void)fFpuWrite; CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); (void)fMcBegin; } while (0)
#define IEM_MC_CALL_FPU_AIMPL_3(a_pfnAImpl, a0, a1, a2) \
    do { (void)fFpuHost; (void)fFpuWrite; CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); (void)fMcBegin; } while (0)
#define IEM_MC_SET_FPU_RESULT(a_FpuData, a_FSW, a_pr80Value)            do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_PUSH_FPU_RESULT(a_FpuData)                               do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_PUSH_FPU_RESULT_MEM_OP(a_FpuData, a_iEffSeg, a_GCPtrEff) do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_PUSH_FPU_RESULT_TWO(a_FpuDataTwo)                        do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_FPU_RESULT(a_FpuData, a_iStReg)                    do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_FPU_RESULT_THEN_POP(a_FpuData, a_iStReg)           do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_FPU_RESULT_MEM_OP(a_FpuData, a_iStReg, a_iEffSeg, a_GCPtrEff)              do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_STORE_FPU_RESULT_MEM_OP_THEN_POP(a_FpuData, a_iStReg, a_iEffSeg, a_GCPtrEff)     do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_FPU_STACK_UNDERFLOW(a_iStReg)                                                    do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP(a_iStReg, a_iEffSeg, a_GCPtrEff)                      do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_FPU_STACK_UNDERFLOW_THEN_POP(a_iStReg)                                           do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP_THEN_POP(a_iStReg, a_iEffSeg, a_GCPtrEff)             do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_FPU_STACK_UNDERFLOW_THEN_POP_POP()                                               do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_FPU_STACK_PUSH_UNDERFLOW()                                                       do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_FPU_STACK_PUSH_UNDERFLOW_TWO()                                                   do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_FPU_STACK_PUSH_OVERFLOW()                                                        do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_FPU_STACK_PUSH_OVERFLOW_MEM_OP(a_iEffSeg, a_GCPtrEff)                            do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_UPDATE_FPU_OPCODE_IP()                                                           do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_FPU_STACK_DEC_TOP()                                                              do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_FPU_STACK_INC_TOP()                                                              do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_FPU_STACK_FREE(a_iStReg)                                                         do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_UPDATE_FSW(a_u16FSW)                                                             do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_UPDATE_FSW_CONST(a_u16FSW)                                                       do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_UPDATE_FSW_WITH_MEM_OP(a_u16FSW, a_iEffSeg, a_GCPtrEff)                          do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_UPDATE_FSW_THEN_POP(a_u16FSW)                                                    do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_UPDATE_FSW_WITH_MEM_OP_THEN_POP(a_u16FSW, a_iEffSeg, a_GCPtrEff)                 do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_UPDATE_FSW_THEN_POP_POP(a_u16FSW)                                                do { (void)fFpuWrite; (void)fMcBegin; } while (0)
#define IEM_MC_PREPARE_FPU_USAGE() (void)fMcBegin; \
    const int fFpuRead = 1, fFpuWrite = 1, fFpuHost = 1, fSseRead = 1, fSseWrite = 1, fSseHost = 1, fAvxRead = 1, fAvxWrite = 1, fAvxHost = 1
#define IEM_MC_ACTUALIZE_FPU_STATE_FOR_READ()   (void)fMcBegin; const int fFpuRead = 1, fSseRead = 1
#define IEM_MC_ACTUALIZE_FPU_STATE_FOR_CHANGE() (void)fMcBegin; const int fFpuRead = 1, fFpuWrite = 1, fSseRead = 1, fSseWrite = 1
#define IEM_MC_STORE_SSE_RESULT(a_SseData, a_iXmmReg)                                           do { (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_SSE_UPDATE_MXCSR(a_fMxcsr)                                                       do { (void)fSseWrite; (void)fMcBegin; } while (0)
#define IEM_MC_PREPARE_SSE_USAGE()              (void)fMcBegin; const int fSseRead = 1, fSseWrite = 1, fSseHost = 1
#define IEM_MC_ACTUALIZE_SSE_STATE_FOR_READ()   (void)fMcBegin; const int fSseRead = 1
#define IEM_MC_ACTUALIZE_SSE_STATE_FOR_CHANGE() (void)fMcBegin; const int fSseRead = 1, fSseWrite = 1
#define IEM_MC_PREPARE_AVX_USAGE()              (void)fMcBegin; const int fAvxRead = 1, fAvxWrite = 1, fAvxHost = 1, fSseRead = 1, fSseWrite = 1, fSseHost = 1
#define IEM_MC_ACTUALIZE_AVX_STATE_FOR_READ()   (void)fMcBegin; const int fAvxRead = 1, fSseRead = 1
#define IEM_MC_ACTUALIZE_AVX_STATE_FOR_CHANGE() (void)fMcBegin; const int fAvxRead = 1, fAvxWrite = 1, fSseRead = 1, fSseWrite = 1

#define IEM_MC_CALL_MMX_AIMPL_2(a_pfnAImpl, a0, a1) \
    do { (void)fFpuHost; (void)fFpuWrite; CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); (void)fMcBegin; } while (0)
#define IEM_MC_CALL_MMX_AIMPL_3(a_pfnAImpl, a0, a1, a2) \
    do { (void)fFpuHost; (void)fFpuWrite; CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); (void)fMcBegin; } while (0)
#define IEM_MC_CALL_SSE_AIMPL_2(a_pfnAImpl, a0, a1) \
    do { (void)fSseHost; (void)fSseWrite; CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); (void)fMcBegin; } while (0)
#define IEM_MC_CALL_SSE_AIMPL_3(a_pfnAImpl, a0, a1, a2) \
    do { (void)fSseHost; (void)fSseWrite; CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); (void)fMcBegin; } while (0)
#define IEM_MC_IMPLICIT_AVX_AIMPL_ARGS() do { IEM_MC_ARG_CONST(PX86XSAVEAREA, pXState, &pVCpu->cpum.GstCtx.XState, 0); (void)fMcBegin; } while (0)
#define IEM_MC_CALL_AVX_AIMPL_2(a_pfnAImpl, a1, a2) \
    do { (void)fAvxHost; (void)fAvxWrite; CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); (void)fMcBegin; } while (0)
#define IEM_MC_CALL_AVX_AIMPL_3(a_pfnAImpl, a1, a2, a3) \
    do { (void)fAvxHost; (void)fAvxWrite; CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); CHK_CALL_ARG(a3, 3); (void)fMcBegin; } while (0)
#define IEM_MC_CALL_AVX_AIMPL_4(a_pfnAImpl, a1, a2, a3, a4) \
    do { (void)fAvxHost; (void)fAvxWrite; CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); CHK_CALL_ARG(a3, 3); CHK_CALL_ARG(a4, 4); (void)fMcBegin; } while (0)

#define IEM_MC_IF_EFL_BIT_SET(a_fBit)                                   (void)fMcBegin; if (g_fRandom) {
#define IEM_MC_IF_EFL_BIT_NOT_SET(a_fBit)                               (void)fMcBegin; if (g_fRandom) {
#define IEM_MC_IF_EFL_ANY_BITS_SET(a_fBits)                             (void)fMcBegin; if (g_fRandom) {
#define IEM_MC_IF_EFL_NO_BITS_SET(a_fBits)                              (void)fMcBegin; if (g_fRandom) {
#define IEM_MC_IF_EFL_BITS_NE(a_fBit1, a_fBit2)                         (void)fMcBegin; if (g_fRandom) {
#define IEM_MC_IF_EFL_BITS_EQ(a_fBit1, a_fBit2)                         (void)fMcBegin; if (g_fRandom) {
#define IEM_MC_IF_EFL_BIT_SET_OR_BITS_NE(a_fBit, a_fBit1, a_fBit2)      (void)fMcBegin; if (g_fRandom) {
#define IEM_MC_IF_EFL_BIT_NOT_SET_AND_BITS_EQ(a_fBit, a_fBit1, a_fBit2) (void)fMcBegin; if (g_fRandom) {
#define IEM_MC_IF_CX_IS_NZ()                                            (void)fMcBegin; if (g_fRandom) {
#define IEM_MC_IF_ECX_IS_NZ()                                           (void)fMcBegin; if (g_fRandom) {
#define IEM_MC_IF_RCX_IS_NZ()                                           (void)fMcBegin; if (g_fRandom) {
#define IEM_MC_IF_CX_IS_NZ_AND_EFL_BIT_SET(a_fBit)                      (void)fMcBegin; if (g_fRandom) {
#define IEM_MC_IF_ECX_IS_NZ_AND_EFL_BIT_SET(a_fBit)                     (void)fMcBegin; if (g_fRandom) {
#define IEM_MC_IF_RCX_IS_NZ_AND_EFL_BIT_SET(a_fBit)                     (void)fMcBegin; if (g_fRandom) {
#define IEM_MC_IF_CX_IS_NZ_AND_EFL_BIT_NOT_SET(a_fBit)                  (void)fMcBegin; if (g_fRandom) {
#define IEM_MC_IF_ECX_IS_NZ_AND_EFL_BIT_NOT_SET(a_fBit)                 (void)fMcBegin; if (g_fRandom) {
#define IEM_MC_IF_RCX_IS_NZ_AND_EFL_BIT_NOT_SET(a_fBit)                 (void)fMcBegin; if (g_fRandom) {
#define IEM_MC_IF_LOCAL_IS_Z(a_Local)                                   (void)fMcBegin; if ((a_Local) == 0) {
#define IEM_MC_IF_GREG_BIT_SET(a_iGReg, a_iBitNo)                       (void)fMcBegin; CHK_GREG_IDX(a_iGReg); if (g_fRandom) {
#define IEM_MC_IF_FPUREG_NOT_EMPTY(a_iSt)                               (void)fMcBegin; if (g_fRandom != fFpuRead) {
#define IEM_MC_IF_FPUREG_IS_EMPTY(a_iSt)                                (void)fMcBegin; if (g_fRandom != fFpuRead) {
#define IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(a_pr80Dst, a_iSt) (void)fMcBegin; \
    a_pr80Dst = NULL; \
    if (g_fRandom != fFpuRead) {
#define IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80(p0, i0, p1, i1) (void)fMcBegin; \
    p0 = NULL; \
    p1 = NULL; \
    if (g_fRandom != fFpuRead) {
#define IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80_FIRST(p0, i0, i1) (void)fMcBegin; \
    p0 = NULL; \
    if (g_fRandom != fFpuRead) {
#define IEM_MC_IF_FCW_IM()                                              (void)fMcBegin; if (g_fRandom != fFpuRead) {
#define IEM_MC_IF_MXCSR_XCPT_PENDING()                                  (void)fMcBegin; if (g_fRandom != fSseRead) {
#define IEM_MC_ELSE()                                                   } else {
#define IEM_MC_ENDIF()                                                  } do { (void)fMcBegin; } while (0)

/** @}  */

#include "../VMMAll/IEMAllInstructionsInterpretOnly.cpp"



/**
 * Formalities...
 */
int main()
{
    RTTEST hTest;
    RTEXITCODE rcExit = RTTestInitAndCreate("tstIEMCheckMc", &hTest);
    if (rcExit == RTEXITCODE_SUCCESS)
    {
        RTTestBanner(hTest);
        RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "(this is only a compile test.)");
        rcExit = RTTestSummaryAndDestroy(hTest);
    }
    return rcExit;
}
