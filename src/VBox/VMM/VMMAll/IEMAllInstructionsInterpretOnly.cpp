/* $Id: IEMAllInstructionsInterpretOnly.cpp $ */
/** @file
 * IEM - Instruction Decoding and Emulation.
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
#ifndef LOG_GROUP /* defined when included by tstIEMCheckMc.cpp */
# define LOG_GROUP LOG_GROUP_IEM
#endif
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
#ifndef TST_IEM_CHECK_MC
# include "IEMInternal.h"
#endif
#include <VBox/vmm/vmcc.h>
#include <VBox/log.h>
#include <VBox/err.h>
#include <VBox/param.h>
#include <VBox/dis.h>
#include <VBox/disopcode.h>
#include <iprt/asm-math.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/x86.h>

#ifndef TST_IEM_CHECK_MC
# include "IEMInline.h"
# include "IEMOpHlp.h"
# include "IEMMc.h"
#endif


#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4702) /* Unreachable code like return in iemOp_Grp6_lldt. */
#endif


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
#ifndef TST_IEM_CHECK_MC
/** Function table for the ADD instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_add =
{
    iemAImpl_add_u8,  iemAImpl_add_u8_locked,
    iemAImpl_add_u16, iemAImpl_add_u16_locked,
    iemAImpl_add_u32, iemAImpl_add_u32_locked,
    iemAImpl_add_u64, iemAImpl_add_u64_locked
};

/** Function table for the ADC instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_adc =
{
    iemAImpl_adc_u8,  iemAImpl_adc_u8_locked,
    iemAImpl_adc_u16, iemAImpl_adc_u16_locked,
    iemAImpl_adc_u32, iemAImpl_adc_u32_locked,
    iemAImpl_adc_u64, iemAImpl_adc_u64_locked
};

/** Function table for the SUB instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_sub =
{
    iemAImpl_sub_u8,  iemAImpl_sub_u8_locked,
    iemAImpl_sub_u16, iemAImpl_sub_u16_locked,
    iemAImpl_sub_u32, iemAImpl_sub_u32_locked,
    iemAImpl_sub_u64, iemAImpl_sub_u64_locked
};

/** Function table for the SBB instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_sbb =
{
    iemAImpl_sbb_u8,  iemAImpl_sbb_u8_locked,
    iemAImpl_sbb_u16, iemAImpl_sbb_u16_locked,
    iemAImpl_sbb_u32, iemAImpl_sbb_u32_locked,
    iemAImpl_sbb_u64, iemAImpl_sbb_u64_locked
};

/** Function table for the OR instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_or =
{
    iemAImpl_or_u8,  iemAImpl_or_u8_locked,
    iemAImpl_or_u16, iemAImpl_or_u16_locked,
    iemAImpl_or_u32, iemAImpl_or_u32_locked,
    iemAImpl_or_u64, iemAImpl_or_u64_locked
};

/** Function table for the XOR instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_xor =
{
    iemAImpl_xor_u8,  iemAImpl_xor_u8_locked,
    iemAImpl_xor_u16, iemAImpl_xor_u16_locked,
    iemAImpl_xor_u32, iemAImpl_xor_u32_locked,
    iemAImpl_xor_u64, iemAImpl_xor_u64_locked
};

/** Function table for the AND instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_and =
{
    iemAImpl_and_u8,  iemAImpl_and_u8_locked,
    iemAImpl_and_u16, iemAImpl_and_u16_locked,
    iemAImpl_and_u32, iemAImpl_and_u32_locked,
    iemAImpl_and_u64, iemAImpl_and_u64_locked
};

/** Function table for the CMP instruction.
 * @remarks Making operand order ASSUMPTIONS.
 */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_cmp =
{
    iemAImpl_cmp_u8,  NULL,
    iemAImpl_cmp_u16, NULL,
    iemAImpl_cmp_u32, NULL,
    iemAImpl_cmp_u64, NULL
};

/** Function table for the TEST instruction.
 * @remarks Making operand order ASSUMPTIONS.
 */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_test =
{
    iemAImpl_test_u8,  NULL,
    iemAImpl_test_u16, NULL,
    iemAImpl_test_u32, NULL,
    iemAImpl_test_u64, NULL
};


/** Function table for the BT instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_bt =
{
    NULL,  NULL,
    iemAImpl_bt_u16, NULL,
    iemAImpl_bt_u32, NULL,
    iemAImpl_bt_u64, NULL
};

/** Function table for the BTC instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_btc =
{
    NULL,  NULL,
    iemAImpl_btc_u16, iemAImpl_btc_u16_locked,
    iemAImpl_btc_u32, iemAImpl_btc_u32_locked,
    iemAImpl_btc_u64, iemAImpl_btc_u64_locked
};

/** Function table for the BTR instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_btr =
{
    NULL,  NULL,
    iemAImpl_btr_u16, iemAImpl_btr_u16_locked,
    iemAImpl_btr_u32, iemAImpl_btr_u32_locked,
    iemAImpl_btr_u64, iemAImpl_btr_u64_locked
};

/** Function table for the BTS instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_bts =
{
    NULL,  NULL,
    iemAImpl_bts_u16, iemAImpl_bts_u16_locked,
    iemAImpl_bts_u32, iemAImpl_bts_u32_locked,
    iemAImpl_bts_u64, iemAImpl_bts_u64_locked
};

/** Function table for the BSF instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_bsf =
{
    NULL,  NULL,
    iemAImpl_bsf_u16, NULL,
    iemAImpl_bsf_u32, NULL,
    iemAImpl_bsf_u64, NULL
};

/** Function table for the BSF instruction, AMD EFLAGS variant. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_bsf_amd =
{
    NULL,  NULL,
    iemAImpl_bsf_u16_amd, NULL,
    iemAImpl_bsf_u32_amd, NULL,
    iemAImpl_bsf_u64_amd, NULL
};

/** Function table for the BSF instruction, Intel EFLAGS variant. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_bsf_intel =
{
    NULL,  NULL,
    iemAImpl_bsf_u16_intel, NULL,
    iemAImpl_bsf_u32_intel, NULL,
    iemAImpl_bsf_u64_intel, NULL
};

/** EFLAGS variation selection table for the BSF instruction. */
IEM_STATIC const IEMOPBINSIZES * const g_iemAImpl_bsf_eflags[] =
{
    &g_iemAImpl_bsf,
    &g_iemAImpl_bsf_intel,
    &g_iemAImpl_bsf_amd,
    &g_iemAImpl_bsf,
};

/** Function table for the BSR instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_bsr =
{
    NULL,  NULL,
    iemAImpl_bsr_u16, NULL,
    iemAImpl_bsr_u32, NULL,
    iemAImpl_bsr_u64, NULL
};

/** Function table for the BSR instruction, AMD EFLAGS variant. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_bsr_amd =
{
    NULL,  NULL,
    iemAImpl_bsr_u16_amd, NULL,
    iemAImpl_bsr_u32_amd, NULL,
    iemAImpl_bsr_u64_amd, NULL
};

/** Function table for the BSR instruction, Intel EFLAGS variant. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_bsr_intel =
{
    NULL,  NULL,
    iemAImpl_bsr_u16_intel, NULL,
    iemAImpl_bsr_u32_intel, NULL,
    iemAImpl_bsr_u64_intel, NULL
};

/** EFLAGS variation selection table for the BSR instruction. */
IEM_STATIC const IEMOPBINSIZES * const g_iemAImpl_bsr_eflags[] =
{
    &g_iemAImpl_bsr,
    &g_iemAImpl_bsr_intel,
    &g_iemAImpl_bsr_amd,
    &g_iemAImpl_bsr,
};

/** Function table for the IMUL instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_imul_two =
{
    NULL,  NULL,
    iemAImpl_imul_two_u16, NULL,
    iemAImpl_imul_two_u32, NULL,
    iemAImpl_imul_two_u64, NULL
};

/** Function table for the IMUL instruction, AMD EFLAGS variant. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_imul_two_amd =
{
    NULL,  NULL,
    iemAImpl_imul_two_u16_amd, NULL,
    iemAImpl_imul_two_u32_amd, NULL,
    iemAImpl_imul_two_u64_amd, NULL
};

/** Function table for the IMUL instruction, Intel EFLAGS variant. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_imul_two_intel =
{
    NULL,  NULL,
    iemAImpl_imul_two_u16_intel, NULL,
    iemAImpl_imul_two_u32_intel, NULL,
    iemAImpl_imul_two_u64_intel, NULL
};

/** EFLAGS variation selection table for the IMUL instruction. */
IEM_STATIC const IEMOPBINSIZES * const g_iemAImpl_imul_two_eflags[] =
{
    &g_iemAImpl_imul_two,
    &g_iemAImpl_imul_two_intel,
    &g_iemAImpl_imul_two_amd,
    &g_iemAImpl_imul_two,
};

/** EFLAGS variation selection table for the 16-bit IMUL instruction. */
IEM_STATIC PFNIEMAIMPLBINU16 const g_iemAImpl_imul_two_u16_eflags[] =
{
    iemAImpl_imul_two_u16,
    iemAImpl_imul_two_u16_intel,
    iemAImpl_imul_two_u16_amd,
    iemAImpl_imul_two_u16,
};

/** EFLAGS variation selection table for the 32-bit IMUL instruction. */
IEM_STATIC PFNIEMAIMPLBINU32 const g_iemAImpl_imul_two_u32_eflags[] =
{
    iemAImpl_imul_two_u32,
    iemAImpl_imul_two_u32_intel,
    iemAImpl_imul_two_u32_amd,
    iemAImpl_imul_two_u32,
};

/** EFLAGS variation selection table for the 64-bit IMUL instruction. */
IEM_STATIC PFNIEMAIMPLBINU64 const g_iemAImpl_imul_two_u64_eflags[] =
{
    iemAImpl_imul_two_u64,
    iemAImpl_imul_two_u64_intel,
    iemAImpl_imul_two_u64_amd,
    iemAImpl_imul_two_u64,
};

/** Group 1 /r lookup table. */
IEM_STATIC const PCIEMOPBINSIZES g_apIemImplGrp1[8] =
{
    &g_iemAImpl_add,
    &g_iemAImpl_or,
    &g_iemAImpl_adc,
    &g_iemAImpl_sbb,
    &g_iemAImpl_and,
    &g_iemAImpl_sub,
    &g_iemAImpl_xor,
    &g_iemAImpl_cmp
};

/** Function table for the INC instruction. */
IEM_STATIC const IEMOPUNARYSIZES g_iemAImpl_inc =
{
    iemAImpl_inc_u8,  iemAImpl_inc_u8_locked,
    iemAImpl_inc_u16, iemAImpl_inc_u16_locked,
    iemAImpl_inc_u32, iemAImpl_inc_u32_locked,
    iemAImpl_inc_u64, iemAImpl_inc_u64_locked
};

/** Function table for the DEC instruction. */
IEM_STATIC const IEMOPUNARYSIZES g_iemAImpl_dec =
{
    iemAImpl_dec_u8,  iemAImpl_dec_u8_locked,
    iemAImpl_dec_u16, iemAImpl_dec_u16_locked,
    iemAImpl_dec_u32, iemAImpl_dec_u32_locked,
    iemAImpl_dec_u64, iemAImpl_dec_u64_locked
};

/** Function table for the NEG instruction. */
IEM_STATIC const IEMOPUNARYSIZES g_iemAImpl_neg =
{
    iemAImpl_neg_u8,  iemAImpl_neg_u8_locked,
    iemAImpl_neg_u16, iemAImpl_neg_u16_locked,
    iemAImpl_neg_u32, iemAImpl_neg_u32_locked,
    iemAImpl_neg_u64, iemAImpl_neg_u64_locked
};

/** Function table for the NOT instruction. */
IEM_STATIC const IEMOPUNARYSIZES g_iemAImpl_not =
{
    iemAImpl_not_u8,  iemAImpl_not_u8_locked,
    iemAImpl_not_u16, iemAImpl_not_u16_locked,
    iemAImpl_not_u32, iemAImpl_not_u32_locked,
    iemAImpl_not_u64, iemAImpl_not_u64_locked
};


/** Function table for the ROL instruction. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_rol =
{
    iemAImpl_rol_u8,
    iemAImpl_rol_u16,
    iemAImpl_rol_u32,
    iemAImpl_rol_u64
};

/** Function table for the ROL instruction, AMD EFLAGS variant. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_rol_amd =
{
    iemAImpl_rol_u8_amd,
    iemAImpl_rol_u16_amd,
    iemAImpl_rol_u32_amd,
    iemAImpl_rol_u64_amd
};

/** Function table for the ROL instruction, Intel EFLAGS variant. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_rol_intel =
{
    iemAImpl_rol_u8_intel,
    iemAImpl_rol_u16_intel,
    iemAImpl_rol_u32_intel,
    iemAImpl_rol_u64_intel
};

/** EFLAGS variation selection table for the ROL instruction. */
IEM_STATIC const IEMOPSHIFTSIZES * const g_iemAImpl_rol_eflags[] =
{
    &g_iemAImpl_rol,
    &g_iemAImpl_rol_intel,
    &g_iemAImpl_rol_amd,
    &g_iemAImpl_rol,
};


/** Function table for the ROR instruction. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_ror =
{
    iemAImpl_ror_u8,
    iemAImpl_ror_u16,
    iemAImpl_ror_u32,
    iemAImpl_ror_u64
};

/** Function table for the ROR instruction, AMD EFLAGS variant. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_ror_amd =
{
    iemAImpl_ror_u8_amd,
    iemAImpl_ror_u16_amd,
    iemAImpl_ror_u32_amd,
    iemAImpl_ror_u64_amd
};

/** Function table for the ROR instruction, Intel EFLAGS variant. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_ror_intel =
{
    iemAImpl_ror_u8_intel,
    iemAImpl_ror_u16_intel,
    iemAImpl_ror_u32_intel,
    iemAImpl_ror_u64_intel
};

/** EFLAGS variation selection table for the ROR instruction. */
IEM_STATIC const IEMOPSHIFTSIZES * const g_iemAImpl_ror_eflags[] =
{
    &g_iemAImpl_ror,
    &g_iemAImpl_ror_intel,
    &g_iemAImpl_ror_amd,
    &g_iemAImpl_ror,
};


/** Function table for the RCL instruction. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_rcl =
{
    iemAImpl_rcl_u8,
    iemAImpl_rcl_u16,
    iemAImpl_rcl_u32,
    iemAImpl_rcl_u64
};

/** Function table for the RCL instruction, AMD EFLAGS variant. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_rcl_amd =
{
    iemAImpl_rcl_u8_amd,
    iemAImpl_rcl_u16_amd,
    iemAImpl_rcl_u32_amd,
    iemAImpl_rcl_u64_amd
};

/** Function table for the RCL instruction, Intel EFLAGS variant. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_rcl_intel =
{
    iemAImpl_rcl_u8_intel,
    iemAImpl_rcl_u16_intel,
    iemAImpl_rcl_u32_intel,
    iemAImpl_rcl_u64_intel
};

/** EFLAGS variation selection table for the RCL instruction. */
IEM_STATIC const IEMOPSHIFTSIZES * const g_iemAImpl_rcl_eflags[] =
{
    &g_iemAImpl_rcl,
    &g_iemAImpl_rcl_intel,
    &g_iemAImpl_rcl_amd,
    &g_iemAImpl_rcl,
};


/** Function table for the RCR instruction. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_rcr =
{
    iemAImpl_rcr_u8,
    iemAImpl_rcr_u16,
    iemAImpl_rcr_u32,
    iemAImpl_rcr_u64
};

/** Function table for the RCR instruction, AMD EFLAGS variant. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_rcr_amd =
{
    iemAImpl_rcr_u8_amd,
    iemAImpl_rcr_u16_amd,
    iemAImpl_rcr_u32_amd,
    iemAImpl_rcr_u64_amd
};

/** Function table for the RCR instruction, Intel EFLAGS variant. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_rcr_intel =
{
    iemAImpl_rcr_u8_intel,
    iemAImpl_rcr_u16_intel,
    iemAImpl_rcr_u32_intel,
    iemAImpl_rcr_u64_intel
};

/** EFLAGS variation selection table for the RCR instruction. */
IEM_STATIC const IEMOPSHIFTSIZES * const g_iemAImpl_rcr_eflags[] =
{
    &g_iemAImpl_rcr,
    &g_iemAImpl_rcr_intel,
    &g_iemAImpl_rcr_amd,
    &g_iemAImpl_rcr,
};


/** Function table for the SHL instruction. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_shl =
{
    iemAImpl_shl_u8,
    iemAImpl_shl_u16,
    iemAImpl_shl_u32,
    iemAImpl_shl_u64
};

/** Function table for the SHL instruction, AMD EFLAGS variant. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_shl_amd =
{
    iemAImpl_shl_u8_amd,
    iemAImpl_shl_u16_amd,
    iemAImpl_shl_u32_amd,
    iemAImpl_shl_u64_amd
};

/** Function table for the SHL instruction, Intel EFLAGS variant. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_shl_intel =
{
    iemAImpl_shl_u8_intel,
    iemAImpl_shl_u16_intel,
    iemAImpl_shl_u32_intel,
    iemAImpl_shl_u64_intel
};

/** EFLAGS variation selection table for the SHL instruction. */
IEM_STATIC const IEMOPSHIFTSIZES * const g_iemAImpl_shl_eflags[] =
{
    &g_iemAImpl_shl,
    &g_iemAImpl_shl_intel,
    &g_iemAImpl_shl_amd,
    &g_iemAImpl_shl,
};


/** Function table for the SHR instruction. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_shr =
{
    iemAImpl_shr_u8,
    iemAImpl_shr_u16,
    iemAImpl_shr_u32,
    iemAImpl_shr_u64
};

/** Function table for the SHR instruction, AMD EFLAGS variant. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_shr_amd =
{
    iemAImpl_shr_u8_amd,
    iemAImpl_shr_u16_amd,
    iemAImpl_shr_u32_amd,
    iemAImpl_shr_u64_amd
};

/** Function table for the SHR instruction, Intel EFLAGS variant. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_shr_intel =
{
    iemAImpl_shr_u8_intel,
    iemAImpl_shr_u16_intel,
    iemAImpl_shr_u32_intel,
    iemAImpl_shr_u64_intel
};

/** EFLAGS variation selection table for the SHR instruction. */
IEM_STATIC const IEMOPSHIFTSIZES * const g_iemAImpl_shr_eflags[] =
{
    &g_iemAImpl_shr,
    &g_iemAImpl_shr_intel,
    &g_iemAImpl_shr_amd,
    &g_iemAImpl_shr,
};


/** Function table for the SAR instruction. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_sar =
{
    iemAImpl_sar_u8,
    iemAImpl_sar_u16,
    iemAImpl_sar_u32,
    iemAImpl_sar_u64
};

/** Function table for the SAR instruction, AMD EFLAGS variant. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_sar_amd =
{
    iemAImpl_sar_u8_amd,
    iemAImpl_sar_u16_amd,
    iemAImpl_sar_u32_amd,
    iemAImpl_sar_u64_amd
};

/** Function table for the SAR instruction, Intel EFLAGS variant. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_sar_intel =
{
    iemAImpl_sar_u8_intel,
    iemAImpl_sar_u16_intel,
    iemAImpl_sar_u32_intel,
    iemAImpl_sar_u64_intel
};

/** EFLAGS variation selection table for the SAR instruction. */
IEM_STATIC const IEMOPSHIFTSIZES * const g_iemAImpl_sar_eflags[] =
{
    &g_iemAImpl_sar,
    &g_iemAImpl_sar_intel,
    &g_iemAImpl_sar_amd,
    &g_iemAImpl_sar,
};


/** Function table for the MUL instruction. */
IEM_STATIC const IEMOPMULDIVSIZES g_iemAImpl_mul =
{
    iemAImpl_mul_u8,
    iemAImpl_mul_u16,
    iemAImpl_mul_u32,
    iemAImpl_mul_u64
};

/** Function table for the MUL instruction, AMD EFLAGS variation. */
IEM_STATIC const IEMOPMULDIVSIZES g_iemAImpl_mul_amd =
{
    iemAImpl_mul_u8_amd,
    iemAImpl_mul_u16_amd,
    iemAImpl_mul_u32_amd,
    iemAImpl_mul_u64_amd
};

/** Function table for the MUL instruction, Intel EFLAGS variation. */
IEM_STATIC const IEMOPMULDIVSIZES g_iemAImpl_mul_intel =
{
    iemAImpl_mul_u8_intel,
    iemAImpl_mul_u16_intel,
    iemAImpl_mul_u32_intel,
    iemAImpl_mul_u64_intel
};

/** EFLAGS variation selection table for the MUL instruction. */
IEM_STATIC const IEMOPMULDIVSIZES * const g_iemAImpl_mul_eflags[] =
{
    &g_iemAImpl_mul,
    &g_iemAImpl_mul_intel,
    &g_iemAImpl_mul_amd,
    &g_iemAImpl_mul,
};

/** EFLAGS variation selection table for the 8-bit MUL instruction. */
IEM_STATIC PFNIEMAIMPLMULDIVU8 const g_iemAImpl_mul_u8_eflags[] =
{
    iemAImpl_mul_u8,
    iemAImpl_mul_u8_intel,
    iemAImpl_mul_u8_amd,
    iemAImpl_mul_u8
};


/** Function table for the IMUL instruction working implicitly on rAX. */
IEM_STATIC const IEMOPMULDIVSIZES g_iemAImpl_imul =
{
    iemAImpl_imul_u8,
    iemAImpl_imul_u16,
    iemAImpl_imul_u32,
    iemAImpl_imul_u64
};

/** Function table for the IMUL instruction working implicitly on rAX, AMD EFLAGS variation. */
IEM_STATIC const IEMOPMULDIVSIZES g_iemAImpl_imul_amd =
{
    iemAImpl_imul_u8_amd,
    iemAImpl_imul_u16_amd,
    iemAImpl_imul_u32_amd,
    iemAImpl_imul_u64_amd
};

/** Function table for the IMUL instruction working implicitly on rAX, Intel EFLAGS variation. */
IEM_STATIC const IEMOPMULDIVSIZES g_iemAImpl_imul_intel =
{
    iemAImpl_imul_u8_intel,
    iemAImpl_imul_u16_intel,
    iemAImpl_imul_u32_intel,
    iemAImpl_imul_u64_intel
};

/** EFLAGS variation selection table for the IMUL instruction. */
IEM_STATIC const IEMOPMULDIVSIZES * const g_iemAImpl_imul_eflags[] =
{
    &g_iemAImpl_imul,
    &g_iemAImpl_imul_intel,
    &g_iemAImpl_imul_amd,
    &g_iemAImpl_imul,
};

/** EFLAGS variation selection table for the 8-bit IMUL instruction. */
IEM_STATIC PFNIEMAIMPLMULDIVU8 const g_iemAImpl_imul_u8_eflags[] =
{
    iemAImpl_imul_u8,
    iemAImpl_imul_u8_intel,
    iemAImpl_imul_u8_amd,
    iemAImpl_imul_u8
};


/** Function table for the DIV instruction. */
IEM_STATIC const IEMOPMULDIVSIZES g_iemAImpl_div =
{
    iemAImpl_div_u8,
    iemAImpl_div_u16,
    iemAImpl_div_u32,
    iemAImpl_div_u64
};

/** Function table for the DIV instruction, AMD EFLAGS variation. */
IEM_STATIC const IEMOPMULDIVSIZES g_iemAImpl_div_amd =
{
    iemAImpl_div_u8_amd,
    iemAImpl_div_u16_amd,
    iemAImpl_div_u32_amd,
    iemAImpl_div_u64_amd
};

/** Function table for the DIV instruction, Intel EFLAGS variation. */
IEM_STATIC const IEMOPMULDIVSIZES g_iemAImpl_div_intel =
{
    iemAImpl_div_u8_intel,
    iemAImpl_div_u16_intel,
    iemAImpl_div_u32_intel,
    iemAImpl_div_u64_intel
};

/** EFLAGS variation selection table for the DIV instruction. */
IEM_STATIC const IEMOPMULDIVSIZES * const g_iemAImpl_div_eflags[] =
{
    &g_iemAImpl_div,
    &g_iemAImpl_div_intel,
    &g_iemAImpl_div_amd,
    &g_iemAImpl_div,
};

/** EFLAGS variation selection table for the 8-bit DIV instruction. */
IEM_STATIC PFNIEMAIMPLMULDIVU8 const g_iemAImpl_div_u8_eflags[] =
{
    iemAImpl_div_u8,
    iemAImpl_div_u8_intel,
    iemAImpl_div_u8_amd,
    iemAImpl_div_u8
};


/** Function table for the IDIV instruction. */
IEM_STATIC const IEMOPMULDIVSIZES g_iemAImpl_idiv =
{
    iemAImpl_idiv_u8,
    iemAImpl_idiv_u16,
    iemAImpl_idiv_u32,
    iemAImpl_idiv_u64
};

/** Function table for the IDIV instruction, AMD EFLAGS variation. */
IEM_STATIC const IEMOPMULDIVSIZES g_iemAImpl_idiv_amd =
{
    iemAImpl_idiv_u8_amd,
    iemAImpl_idiv_u16_amd,
    iemAImpl_idiv_u32_amd,
    iemAImpl_idiv_u64_amd
};

/** Function table for the IDIV instruction, Intel EFLAGS variation. */
IEM_STATIC const IEMOPMULDIVSIZES g_iemAImpl_idiv_intel =
{
    iemAImpl_idiv_u8_intel,
    iemAImpl_idiv_u16_intel,
    iemAImpl_idiv_u32_intel,
    iemAImpl_idiv_u64_intel
};

/** EFLAGS variation selection table for the IDIV instruction. */
IEM_STATIC const IEMOPMULDIVSIZES * const g_iemAImpl_idiv_eflags[] =
{
    &g_iemAImpl_idiv,
    &g_iemAImpl_idiv_intel,
    &g_iemAImpl_idiv_amd,
    &g_iemAImpl_idiv,
};

/** EFLAGS variation selection table for the 8-bit IDIV instruction. */
IEM_STATIC PFNIEMAIMPLMULDIVU8 const g_iemAImpl_idiv_u8_eflags[] =
{
    iemAImpl_idiv_u8,
    iemAImpl_idiv_u8_intel,
    iemAImpl_idiv_u8_amd,
    iemAImpl_idiv_u8
};


/** Function table for the SHLD instruction. */
IEM_STATIC const IEMOPSHIFTDBLSIZES g_iemAImpl_shld =
{
    iemAImpl_shld_u16,
    iemAImpl_shld_u32,
    iemAImpl_shld_u64,
};

/** Function table for the SHLD instruction, AMD EFLAGS variation. */
IEM_STATIC const IEMOPSHIFTDBLSIZES g_iemAImpl_shld_amd =
{
    iemAImpl_shld_u16_amd,
    iemAImpl_shld_u32_amd,
    iemAImpl_shld_u64_amd
};

/** Function table for the SHLD instruction, Intel EFLAGS variation. */
IEM_STATIC const IEMOPSHIFTDBLSIZES g_iemAImpl_shld_intel =
{
    iemAImpl_shld_u16_intel,
    iemAImpl_shld_u32_intel,
    iemAImpl_shld_u64_intel
};

/** EFLAGS variation selection table for the SHLD instruction. */
IEM_STATIC const IEMOPSHIFTDBLSIZES * const g_iemAImpl_shld_eflags[] =
{
    &g_iemAImpl_shld,
    &g_iemAImpl_shld_intel,
    &g_iemAImpl_shld_amd,
    &g_iemAImpl_shld
};

/** Function table for the SHRD instruction. */
IEM_STATIC const IEMOPSHIFTDBLSIZES g_iemAImpl_shrd =
{
    iemAImpl_shrd_u16,
    iemAImpl_shrd_u32,
    iemAImpl_shrd_u64
};

/** Function table for the SHRD instruction, AMD EFLAGS variation. */
IEM_STATIC const IEMOPSHIFTDBLSIZES g_iemAImpl_shrd_amd =
{
    iemAImpl_shrd_u16_amd,
    iemAImpl_shrd_u32_amd,
    iemAImpl_shrd_u64_amd
};

/** Function table for the SHRD instruction, Intel EFLAGS variation. */
IEM_STATIC const IEMOPSHIFTDBLSIZES g_iemAImpl_shrd_intel =
{
    iemAImpl_shrd_u16_intel,
    iemAImpl_shrd_u32_intel,
    iemAImpl_shrd_u64_intel
};

/** EFLAGS variation selection table for the SHRD instruction. */
IEM_STATIC const IEMOPSHIFTDBLSIZES * const g_iemAImpl_shrd_eflags[] =
{
    &g_iemAImpl_shrd,
    &g_iemAImpl_shrd_intel,
    &g_iemAImpl_shrd_amd,
    &g_iemAImpl_shrd
};


# ifndef IEM_WITHOUT_ASSEMBLY
/** Function table for the VPXOR instruction */
IEM_STATIC const IEMOPMEDIAF3 g_iemAImpl_vpand          = { iemAImpl_vpand_u128,   iemAImpl_vpand_u256 };
/** Function table for the VPXORN instruction */
IEM_STATIC const IEMOPMEDIAF3 g_iemAImpl_vpandn         = { iemAImpl_vpandn_u128,  iemAImpl_vpandn_u256 };
/** Function table for the VPOR instruction */
IEM_STATIC const IEMOPMEDIAF3 g_iemAImpl_vpor           = { iemAImpl_vpor_u128,    iemAImpl_vpor_u256 };
/** Function table for the VPXOR instruction */
IEM_STATIC const IEMOPMEDIAF3 g_iemAImpl_vpxor          = { iemAImpl_vpxor_u128,   iemAImpl_vpxor_u256 };
# endif

/** Function table for the VPAND instruction, software fallback. */
IEM_STATIC const IEMOPMEDIAF3 g_iemAImpl_vpand_fallback = { iemAImpl_vpand_u128_fallback,  iemAImpl_vpand_u256_fallback };
/** Function table for the VPANDN instruction, software fallback. */
IEM_STATIC const IEMOPMEDIAF3 g_iemAImpl_vpandn_fallback= { iemAImpl_vpandn_u128_fallback, iemAImpl_vpandn_u256_fallback };
/** Function table for the VPOR instruction, software fallback. */
IEM_STATIC const IEMOPMEDIAF3 g_iemAImpl_vpor_fallback  = { iemAImpl_vpor_u128_fallback,   iemAImpl_vpor_u256_fallback };
/** Function table for the VPXOR instruction, software fallback. */
IEM_STATIC const IEMOPMEDIAF3 g_iemAImpl_vpxor_fallback = { iemAImpl_vpxor_u128_fallback,  iemAImpl_vpxor_u256_fallback };

#endif /* !TST_IEM_CHECK_MC */


/**
 * Common worker for instructions like ADD, AND, OR, ++ with a byte
 * memory/register as the destination.
 *
 * @param   pImpl       Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpBinaryOperator_rm_r8, PCIEMOPBINSIZES, pImpl)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(3, 0);
        IEM_MC_ARG(uint8_t *,  pu8Dst,  0);
        IEM_MC_ARG(uint8_t,    u8Src,   1);
        IEM_MC_ARG(uint32_t *, pEFlags, 2);

        IEM_MC_FETCH_GREG_U8(u8Src, IEM_GET_MODRM_REG(pVCpu, bRm));
        IEM_MC_REF_GREG_U8(pu8Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, u8Src, pEFlags);

        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
    else
    {
        /*
         * We're accessing memory.
         * Note! We're putting the eflags on the stack here so we can commit them
         *       after the memory.
         */
        uint32_t const fAccess = pImpl->pfnLockedU8 ? IEM_ACCESS_DATA_RW : IEM_ACCESS_DATA_R; /* CMP,TEST */
        IEM_MC_BEGIN(3, 2);
        IEM_MC_ARG(uint8_t *,  pu8Dst,           0);
        IEM_MC_ARG(uint8_t,    u8Src,            1);
        IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
        if (!pImpl->pfnLockedU8)
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_MEM_MAP(pu8Dst, fAccess, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
        IEM_MC_FETCH_GREG_U8(u8Src, IEM_GET_MODRM_REG(pVCpu, bRm));
        IEM_MC_FETCH_EFLAGS(EFlags);
        if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_LOCK))
            IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, u8Src, pEFlags);
        else
            IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU8, pu8Dst, u8Src, pEFlags);

        IEM_MC_MEM_COMMIT_AND_UNMAP(pu8Dst, fAccess);
        IEM_MC_COMMIT_EFLAGS(EFlags);
        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
}


/**
 * Common worker for word/dword/qword instructions like ADD, AND, OR, ++ with
 * memory/register as the destination.
 *
 * @param   pImpl       Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpBinaryOperator_rm_rv, PCIEMOPBINSIZES, pImpl)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint16_t *, pu16Dst, 0);
                IEM_MC_ARG(uint16_t,   u16Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);

                IEM_MC_FETCH_GREG_U16(u16Src, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_REF_GREG_U16(pu16Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint32_t *, pu32Dst, 0);
                IEM_MC_ARG(uint32_t,   u32Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);

                IEM_MC_FETCH_GREG_U32(u32Src, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_REF_GREG_U32(pu32Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);

                if ((pImpl != &g_iemAImpl_test) && (pImpl != &g_iemAImpl_cmp))
                    IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(pu32Dst);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint64_t *, pu64Dst, 0);
                IEM_MC_ARG(uint64_t,   u64Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);

                IEM_MC_FETCH_GREG_U64(u64Src, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_REF_GREG_U64(pu64Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /*
         * We're accessing memory.
         * Note! We're putting the eflags on the stack here so we can commit them
         *       after the memory.
         */
        uint32_t const fAccess = pImpl->pfnLockedU8 ? IEM_ACCESS_DATA_RW : IEM_ACCESS_DATA_R /* CMP,TEST */;
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint16_t *, pu16Dst,          0);
                IEM_MC_ARG(uint16_t,   u16Src,           1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                if (!pImpl->pfnLockedU16)
                    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_MEM_MAP(pu16Dst, fAccess, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_GREG_U16(u16Src, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_FETCH_EFLAGS(EFlags);
                if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_LOCK))
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);
                else
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU16, pu16Dst, u16Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu16Dst, fAccess);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint32_t *, pu32Dst,          0);
                IEM_MC_ARG(uint32_t,   u32Src,           1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                if (!pImpl->pfnLockedU32)
                    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_MEM_MAP(pu32Dst, fAccess, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_GREG_U32(u32Src, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_FETCH_EFLAGS(EFlags);
                if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_LOCK))
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);
                else
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU32, pu32Dst, u32Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu32Dst, fAccess);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint64_t *, pu64Dst,          0);
                IEM_MC_ARG(uint64_t,   u64Src,           1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                if (!pImpl->pfnLockedU64)
                    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_MEM_MAP(pu64Dst, fAccess, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_GREG_U64(u64Src, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_FETCH_EFLAGS(EFlags);
                if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_LOCK))
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);
                else
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU64, pu64Dst, u64Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu64Dst, fAccess);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/**
 * Common worker for byte instructions like ADD, AND, OR, ++ with a register as
 * the destination.
 *
 * @param   pImpl       Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpBinaryOperator_r8_rm, PCIEMOPBINSIZES, pImpl)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_BEGIN(3, 0);
        IEM_MC_ARG(uint8_t *,  pu8Dst,  0);
        IEM_MC_ARG(uint8_t,    u8Src,   1);
        IEM_MC_ARG(uint32_t *, pEFlags, 2);

        IEM_MC_FETCH_GREG_U8(u8Src, IEM_GET_MODRM_RM(pVCpu, bRm));
        IEM_MC_REF_GREG_U8(pu8Dst, IEM_GET_MODRM_REG(pVCpu, bRm));
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, u8Src, pEFlags);

        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
    else
    {
        /*
         * We're accessing memory.
         */
        IEM_MC_BEGIN(3, 1);
        IEM_MC_ARG(uint8_t *,  pu8Dst,  0);
        IEM_MC_ARG(uint8_t,    u8Src,   1);
        IEM_MC_ARG(uint32_t *, pEFlags, 2);
        IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_FETCH_MEM_U8(u8Src, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
        IEM_MC_REF_GREG_U8(pu8Dst, IEM_GET_MODRM_REG(pVCpu, bRm));
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, u8Src, pEFlags);

        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
}


/**
 * Common worker for word/dword/qword instructions like ADD, AND, OR, ++ with a
 * register as the destination.
 *
 * @param   pImpl       Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpBinaryOperator_rv_rm, PCIEMOPBINSIZES, pImpl)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint16_t *, pu16Dst, 0);
                IEM_MC_ARG(uint16_t,   u16Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);

                IEM_MC_FETCH_GREG_U16(u16Src, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_GREG_U16(pu16Dst, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint32_t *, pu32Dst, 0);
                IEM_MC_ARG(uint32_t,   u32Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);

                IEM_MC_FETCH_GREG_U32(u32Src, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_GREG_U32(pu32Dst, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);

                if (pImpl != &g_iemAImpl_cmp)   /* Not used with TEST. */
                    IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(pu32Dst);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint64_t *, pu64Dst, 0);
                IEM_MC_ARG(uint64_t,   u64Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);

                IEM_MC_FETCH_GREG_U64(u64Src, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_GREG_U64(pu64Dst, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /*
         * We're accessing memory.
         */
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint16_t *, pu16Dst, 0);
                IEM_MC_ARG(uint16_t,   u16Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U16(u16Src, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
                IEM_MC_REF_GREG_U16(pu16Dst, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint32_t *, pu32Dst, 0);
                IEM_MC_ARG(uint32_t,   u32Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U32(u32Src, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
                IEM_MC_REF_GREG_U32(pu32Dst, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);

                if (pImpl != &g_iemAImpl_cmp)
                    IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(pu32Dst);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint64_t *, pu64Dst, 0);
                IEM_MC_ARG(uint64_t,   u64Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U64(u64Src, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
                IEM_MC_REF_GREG_U64(pu64Dst, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/**
 * Common worker for instructions like ADD, AND, OR, ++ with working on AL with
 * a byte immediate.
 *
 * @param   pImpl       Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpBinaryOperator_AL_Ib, PCIEMOPBINSIZES, pImpl)
{
    uint8_t u8Imm; IEM_OPCODE_GET_NEXT_U8(&u8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(3, 0);
    IEM_MC_ARG(uint8_t *,       pu8Dst,             0);
    IEM_MC_ARG_CONST(uint8_t,   u8Src,/*=*/ u8Imm,  1);
    IEM_MC_ARG(uint32_t *,      pEFlags,            2);

    IEM_MC_REF_GREG_U8(pu8Dst, X86_GREG_xAX);
    IEM_MC_REF_EFLAGS(pEFlags);
    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, u8Src, pEFlags);

    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/**
 * Common worker for instructions like ADD, AND, OR, ++ with working on
 * AX/EAX/RAX with a word/dword immediate.
 *
 * @param   pImpl       Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpBinaryOperator_rAX_Iz, PCIEMOPBINSIZES, pImpl)
{
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
        {
            uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(&u16Imm);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

            IEM_MC_BEGIN(3, 0);
            IEM_MC_ARG(uint16_t *,      pu16Dst,                0);
            IEM_MC_ARG_CONST(uint16_t,  u16Src,/*=*/ u16Imm,    1);
            IEM_MC_ARG(uint32_t *,      pEFlags,                2);

            IEM_MC_REF_GREG_U16(pu16Dst, X86_GREG_xAX);
            IEM_MC_REF_EFLAGS(pEFlags);
            IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);

            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            return VINF_SUCCESS;
        }

        case IEMMODE_32BIT:
        {
            uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(&u32Imm);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

            IEM_MC_BEGIN(3, 0);
            IEM_MC_ARG(uint32_t *,      pu32Dst,                0);
            IEM_MC_ARG_CONST(uint32_t,  u32Src,/*=*/ u32Imm,    1);
            IEM_MC_ARG(uint32_t *,      pEFlags,                2);

            IEM_MC_REF_GREG_U32(pu32Dst, X86_GREG_xAX);
            IEM_MC_REF_EFLAGS(pEFlags);
            IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);

            if ((pImpl != &g_iemAImpl_test) && (pImpl != &g_iemAImpl_cmp))
                IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(pu32Dst);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            return VINF_SUCCESS;
        }

        case IEMMODE_64BIT:
        {
            uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S32_SX_U64(&u64Imm);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

            IEM_MC_BEGIN(3, 0);
            IEM_MC_ARG(uint64_t *,      pu64Dst,                0);
            IEM_MC_ARG_CONST(uint64_t,  u64Src,/*=*/ u64Imm,    1);
            IEM_MC_ARG(uint32_t *,      pEFlags,                2);

            IEM_MC_REF_GREG_U64(pu64Dst, X86_GREG_xAX);
            IEM_MC_REF_EFLAGS(pEFlags);
            IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);

            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            return VINF_SUCCESS;
        }

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/** Opcodes 0xf1, 0xd6. */
FNIEMOP_DEF(iemOp_Invalid)
{
    IEMOP_MNEMONIC(Invalid, "Invalid");
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Invalid with RM byte . */
FNIEMOPRM_DEF(iemOp_InvalidWithRM)
{
    RT_NOREF_PV(bRm);
    IEMOP_MNEMONIC(InvalidWithRm, "InvalidWithRM");
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Invalid with RM byte where intel decodes any additional address encoding
 *  bytes. */
FNIEMOPRM_DEF(iemOp_InvalidWithRMNeedDecode)
{
    IEMOP_MNEMONIC(InvalidWithRMNeedDecode, "InvalidWithRMNeedDecode");
    if (pVCpu->iem.s.enmCpuVendor == CPUMCPUVENDOR_INTEL)
    {
#ifndef TST_IEM_CHECK_MC
        if (IEM_IS_MODRM_MEM_MODE(bRm))
        {
            RTGCPTR      GCPtrEff;
            VBOXSTRICTRC rcStrict = iemOpHlpCalcRmEffAddr(pVCpu, bRm, 0, &GCPtrEff);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
        }
#endif
    }
    IEMOP_HLP_DONE_DECODING();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Invalid with RM byte where both AMD and Intel decodes any additional
 *  address encoding bytes. */
FNIEMOPRM_DEF(iemOp_InvalidWithRMAllNeeded)
{
    IEMOP_MNEMONIC(InvalidWithRMAllNeeded, "InvalidWithRMAllNeeded");
#ifndef TST_IEM_CHECK_MC
    if (IEM_IS_MODRM_MEM_MODE(bRm))
    {
        RTGCPTR      GCPtrEff;
        VBOXSTRICTRC rcStrict = iemOpHlpCalcRmEffAddr(pVCpu, bRm, 0, &GCPtrEff);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
    }
#endif
    IEMOP_HLP_DONE_DECODING();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Invalid with RM byte where intel requires 8-byte immediate.
 * Intel will also need SIB and displacement if bRm indicates memory. */
FNIEMOPRM_DEF(iemOp_InvalidWithRMNeedImm8)
{
    IEMOP_MNEMONIC(InvalidWithRMNeedImm8, "InvalidWithRMNeedImm8");
    if (pVCpu->iem.s.enmCpuVendor == CPUMCPUVENDOR_INTEL)
    {
#ifndef TST_IEM_CHECK_MC
        if (IEM_IS_MODRM_MEM_MODE(bRm))
        {
            RTGCPTR      GCPtrEff;
            VBOXSTRICTRC rcStrict = iemOpHlpCalcRmEffAddr(pVCpu, bRm, 0, &GCPtrEff);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
        }
#endif
        uint8_t bImm8;  IEM_OPCODE_GET_NEXT_U8(&bImm8);  RT_NOREF(bRm);
    }
    IEMOP_HLP_DONE_DECODING();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Invalid with RM byte where intel requires 8-byte immediate.
 * Both AMD and Intel also needs SIB and displacement according to bRm. */
FNIEMOPRM_DEF(iemOp_InvalidWithRMAllNeedImm8)
{
    IEMOP_MNEMONIC(InvalidWithRMAllNeedImm8, "InvalidWithRMAllNeedImm8");
#ifndef TST_IEM_CHECK_MC
    if (IEM_IS_MODRM_MEM_MODE(bRm))
    {
        RTGCPTR      GCPtrEff;
        VBOXSTRICTRC rcStrict = iemOpHlpCalcRmEffAddr(pVCpu, bRm, 0, &GCPtrEff);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
    }
#endif
    uint8_t bImm8;  IEM_OPCODE_GET_NEXT_U8(&bImm8);  RT_NOREF(bRm);
    IEMOP_HLP_DONE_DECODING();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Invalid opcode where intel requires Mod R/M sequence. */
FNIEMOP_DEF(iemOp_InvalidNeedRM)
{
    IEMOP_MNEMONIC(InvalidNeedRM, "InvalidNeedRM");
    if (pVCpu->iem.s.enmCpuVendor == CPUMCPUVENDOR_INTEL)
    {
        uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm); RT_NOREF(bRm);
#ifndef TST_IEM_CHECK_MC
        if (IEM_IS_MODRM_MEM_MODE(bRm))
        {
            RTGCPTR      GCPtrEff;
            VBOXSTRICTRC rcStrict = iemOpHlpCalcRmEffAddr(pVCpu, bRm, 0, &GCPtrEff);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
        }
#endif
    }
    IEMOP_HLP_DONE_DECODING();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Invalid opcode where both AMD and Intel requires Mod R/M sequence. */
FNIEMOP_DEF(iemOp_InvalidAllNeedRM)
{
    IEMOP_MNEMONIC(InvalidAllNeedRM, "InvalidAllNeedRM");
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm); RT_NOREF(bRm);
#ifndef TST_IEM_CHECK_MC
    if (IEM_IS_MODRM_MEM_MODE(bRm))
    {
        RTGCPTR      GCPtrEff;
        VBOXSTRICTRC rcStrict = iemOpHlpCalcRmEffAddr(pVCpu, bRm, 0, &GCPtrEff);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
    }
#endif
    IEMOP_HLP_DONE_DECODING();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Invalid opcode where intel requires Mod R/M sequence and 8-byte
 *  immediate. */
FNIEMOP_DEF(iemOp_InvalidNeedRMImm8)
{
    IEMOP_MNEMONIC(InvalidNeedRMImm8, "InvalidNeedRMImm8");
    if (pVCpu->iem.s.enmCpuVendor == CPUMCPUVENDOR_INTEL)
    {
        uint8_t bRm;  IEM_OPCODE_GET_NEXT_U8(&bRm);  RT_NOREF(bRm);
#ifndef TST_IEM_CHECK_MC
        if (IEM_IS_MODRM_MEM_MODE(bRm))
        {
            RTGCPTR      GCPtrEff;
            VBOXSTRICTRC rcStrict = iemOpHlpCalcRmEffAddr(pVCpu, bRm, 0, &GCPtrEff);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
        }
#endif
        uint8_t bImm; IEM_OPCODE_GET_NEXT_U8(&bImm); RT_NOREF(bImm);
    }
    IEMOP_HLP_DONE_DECODING();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Invalid opcode where intel requires a 3rd escape byte and a Mod R/M
 *  sequence. */
FNIEMOP_DEF(iemOp_InvalidNeed3ByteEscRM)
{
    IEMOP_MNEMONIC(InvalidNeed3ByteEscRM, "InvalidNeed3ByteEscRM");
    if (pVCpu->iem.s.enmCpuVendor == CPUMCPUVENDOR_INTEL)
    {
        uint8_t b3rd; IEM_OPCODE_GET_NEXT_U8(&b3rd); RT_NOREF(b3rd);
        uint8_t bRm;  IEM_OPCODE_GET_NEXT_U8(&bRm);  RT_NOREF(bRm);
#ifndef TST_IEM_CHECK_MC
        if (IEM_IS_MODRM_MEM_MODE(bRm))
        {
            RTGCPTR      GCPtrEff;
            VBOXSTRICTRC rcStrict = iemOpHlpCalcRmEffAddr(pVCpu, bRm, 0, &GCPtrEff);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
        }
#endif
    }
    IEMOP_HLP_DONE_DECODING();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Invalid opcode where intel requires a 3rd escape byte, Mod R/M sequence, and
 *  a 8-byte immediate. */
FNIEMOP_DEF(iemOp_InvalidNeed3ByteEscRMImm8)
{
    IEMOP_MNEMONIC(InvalidNeed3ByteEscRMImm8, "InvalidNeed3ByteEscRMImm8");
    if (pVCpu->iem.s.enmCpuVendor == CPUMCPUVENDOR_INTEL)
    {
        uint8_t b3rd; IEM_OPCODE_GET_NEXT_U8(&b3rd); RT_NOREF(b3rd);
        uint8_t bRm;  IEM_OPCODE_GET_NEXT_U8(&bRm);  RT_NOREF(bRm);
#ifndef TST_IEM_CHECK_MC
        if (IEM_IS_MODRM_MEM_MODE(bRm))
        {
            RTGCPTR      GCPtrEff;
            VBOXSTRICTRC rcStrict = iemOpHlpCalcRmEffAddr(pVCpu, bRm, 1, &GCPtrEff);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
        }
#endif
        uint8_t bImm; IEM_OPCODE_GET_NEXT_U8(&bImm); RT_NOREF(bImm);
        IEMOP_HLP_DONE_DECODING();
    }
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Repeats a_fn four times.  For decoding tables. */
#define IEMOP_X4(a_fn) a_fn, a_fn, a_fn, a_fn

/*
 * Include the tables.
 */
#ifdef IEM_WITH_3DNOW
# include "IEMAllInstructions3DNow.cpp.h"
#endif
#ifdef IEM_WITH_THREE_0F_38
# include "IEMAllInstructionsThree0f38.cpp.h"
#endif
#ifdef IEM_WITH_THREE_0F_3A
# include "IEMAllInstructionsThree0f3a.cpp.h"
#endif
#include "IEMAllInstructionsTwoByte0f.cpp.h"
#ifdef IEM_WITH_VEX
# include "IEMAllInstructionsVexMap1.cpp.h"
# include "IEMAllInstructionsVexMap2.cpp.h"
# include "IEMAllInstructionsVexMap3.cpp.h"
#endif
#include "IEMAllInstructionsOneByte.cpp.h"


#ifdef _MSC_VER
# pragma warning(pop)
#endif

