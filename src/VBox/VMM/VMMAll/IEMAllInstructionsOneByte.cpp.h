/* $Id: IEMAllInstructionsOneByte.cpp.h $ */
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


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
extern const PFNIEMOP g_apfnOneByteMap[256]; /* not static since we need to forward declare it. */

/* Instruction group definitions: */

/** @defgroup og_gen    General
 * @{ */
 /** @defgroup og_gen_arith     Arithmetic
  * @{  */
  /** @defgroup og_gen_arith_bin    Binary numbers */
  /** @defgroup og_gen_arith_dec    Decimal numbers */
 /** @} */
/** @} */

/** @defgroup og_stack Stack
 * @{ */
 /** @defgroup og_stack_sreg    Segment registers */
/** @} */

/** @defgroup og_prefix     Prefixes */
/** @defgroup og_escapes    Escape bytes */



/** @name One byte opcodes.
 * @{
 */

/* Instruction specification format - work in progress:  */

/**
 * @opcode      0x00
 * @opmnemonic  add
 * @op1         rm:Eb
 * @op2         reg:Gb
 * @opmaps      one
 * @openc       ModR/M
 * @opflmodify  cf,pf,af,zf,sf,of
 * @ophints     harmless ignores_op_sizes
 * @opstats     add_Eb_Gb
 * @opgroup     og_gen_arith_bin
 * @optest              op1=1   op2=1   -> op1=2   efl&|=nc,pe,na,nz,pl,nv
 * @optest      efl|=cf op1=1   op2=2   -> op1=3   efl&|=nc,po,na,nz,pl,nv
 * @optest              op1=254 op2=1   -> op1=255 efl&|=nc,po,na,nz,ng,nv
 * @optest              op1=128 op2=128 -> op1=0   efl&|=ov,pl,zf,na,po,cf
 */
FNIEMOP_DEF(iemOp_add_Eb_Gb)
{
    IEMOP_MNEMONIC2(MR, ADD, add, Eb, Gb, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES | IEMOPHINT_LOCK_ALLOWED);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_r8, &g_iemAImpl_add);
}


/**
 * @opcode      0x01
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @optest               op1=1  op2=1  -> op1=2  efl&|=nc,pe,na,nz,pl,nv
 * @optest      efl|=cf  op1=2  op2=2  -> op1=4  efl&|=nc,pe,na,nz,pl,nv
 * @optest      efl&~=cf op1=-1 op2=1  -> op1=0  efl&|=cf,po,af,zf,pl,nv
 * @optest               op1=-1 op2=-1 -> op1=-2 efl&|=cf,pe,af,nz,ng,nv
 */
FNIEMOP_DEF(iemOp_add_Ev_Gv)
{
    IEMOP_MNEMONIC2(MR, ADD, add, Ev, Gv, DISOPTYPE_HARMLESS, IEMOPHINT_LOCK_ALLOWED);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_rv, &g_iemAImpl_add);
}


/**
 * @opcode      0x02
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opcopytests iemOp_add_Eb_Gb
 */
FNIEMOP_DEF(iemOp_add_Gb_Eb)
{
    IEMOP_MNEMONIC2(RM, ADD, add, Gb, Eb, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_r8_rm, &g_iemAImpl_add);
}


/**
 * @opcode      0x03
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opcopytests iemOp_add_Ev_Gv
 */
FNIEMOP_DEF(iemOp_add_Gv_Ev)
{
    IEMOP_MNEMONIC2(RM, ADD, add, Gv, Ev, DISOPTYPE_HARMLESS, 0);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rv_rm, &g_iemAImpl_add);
}


/**
 * @opcode      0x04
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opcopytests iemOp_add_Eb_Gb
 */
FNIEMOP_DEF(iemOp_add_Al_Ib)
{
    IEMOP_MNEMONIC2(FIXED, ADD, add, AL, Ib, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_AL_Ib, &g_iemAImpl_add);
}


/**
 * @opcode      0x05
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @optest      op1=1 op2=1 -> op1=2 efl&|=nv,pl,nz,na,pe
 * @optest      efl|=cf  op1=2  op2=2  -> op1=4  efl&|=nc,pe,na,nz,pl,nv
 * @optest      efl&~=cf op1=-1 op2=1  -> op1=0  efl&|=cf,po,af,zf,pl,nv
 * @optest               op1=-1 op2=-1 -> op1=-2 efl&|=cf,pe,af,nz,ng,nv
 */
FNIEMOP_DEF(iemOp_add_eAX_Iz)
{
    IEMOP_MNEMONIC2(FIXED, ADD, add, rAX, Iz, DISOPTYPE_HARMLESS, 0);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rAX_Iz, &g_iemAImpl_add);
}


/**
 * @opcode      0x06
 * @opgroup     og_stack_sreg
 */
FNIEMOP_DEF(iemOp_push_ES)
{
    IEMOP_MNEMONIC1(FIXED, PUSH, push, ES, DISOPTYPE_HARMLESS | DISOPTYPE_INVALID_64, 0);
    IEMOP_HLP_NO_64BIT();
    return FNIEMOP_CALL_1(iemOpCommonPushSReg, X86_SREG_ES);
}


/**
 * @opcode      0x07
 * @opgroup     og_stack_sreg
 */
FNIEMOP_DEF(iemOp_pop_ES)
{
    IEMOP_MNEMONIC1(FIXED, POP, pop, ES, DISOPTYPE_HARMLESS | DISOPTYPE_INVALID_64, 0);
    IEMOP_HLP_NO_64BIT();
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_pop_Sreg, X86_SREG_ES, pVCpu->iem.s.enmEffOpSize);
}


/**
 * @opcode      0x08
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   af
 * @opflclear   of,cf
 * @optest                  op1=7 op2=12 -> op1=15   efl&|=nc,po,na,nz,pl,nv
 * @optest      efl|=of,cf  op1=0 op2=0  -> op1=0    efl&|=nc,po,na,zf,pl,nv
 * @optest            op1=0xee op2=0x11  -> op1=0xff efl&|=nc,po,na,nz,ng,nv
 * @optest            op1=0xff op2=0xff  -> op1=0xff efl&|=nc,po,na,nz,ng,nv
 */
FNIEMOP_DEF(iemOp_or_Eb_Gb)
{
    IEMOP_MNEMONIC2(MR, OR, or, Eb, Gb, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES | IEMOPHINT_LOCK_ALLOWED);
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_r8, &g_iemAImpl_or);
}


/*
 * @opcode      0x09
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   af
 * @opflclear   of,cf
 * @optest      efl|=of,cf  op1=12 op2=7 -> op1=15   efl&|=nc,po,na,nz,pl,nv
 * @optest      efl|=of,cf  op1=0 op2=0  -> op1=0    efl&|=nc,po,na,zf,pl,nv
 * @optest      op1=-2 op2=1  -> op1=-1 efl&|=nc,po,na,nz,ng,nv
 * @optest      o16 / op1=0x5a5a             op2=0xa5a5             -> op1=-1 efl&|=nc,po,na,nz,ng,nv
 * @optest      o32 / op1=0x5a5a5a5a         op2=0xa5a5a5a5         -> op1=-1 efl&|=nc,po,na,nz,ng,nv
 * @optest      o64 / op1=0x5a5a5a5a5a5a5a5a op2=0xa5a5a5a5a5a5a5a5 -> op1=-1 efl&|=nc,po,na,nz,ng,nv
 */
FNIEMOP_DEF(iemOp_or_Ev_Gv)
{
    IEMOP_MNEMONIC2(MR, OR, or, Ev, Gv, DISOPTYPE_HARMLESS, IEMOPHINT_LOCK_ALLOWED);
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_rv, &g_iemAImpl_or);
}


/**
 * @opcode      0x0a
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   af
 * @opflclear   of,cf
 * @opcopytests iemOp_or_Eb_Gb
 */
FNIEMOP_DEF(iemOp_or_Gb_Eb)
{
    IEMOP_MNEMONIC2(RM, OR, or, Gb, Eb, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES | IEMOPHINT_LOCK_ALLOWED);
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_r8_rm, &g_iemAImpl_or);
}


/**
 * @opcode      0x0b
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   af
 * @opflclear   of,cf
 * @opcopytests iemOp_or_Ev_Gv
 */
FNIEMOP_DEF(iemOp_or_Gv_Ev)
{
    IEMOP_MNEMONIC2(RM, OR, or, Gv, Ev, DISOPTYPE_HARMLESS, 0);
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rv_rm, &g_iemAImpl_or);
}


/**
 * @opcode      0x0c
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   af
 * @opflclear   of,cf
 * @opcopytests iemOp_or_Eb_Gb
 */
FNIEMOP_DEF(iemOp_or_Al_Ib)
{
    IEMOP_MNEMONIC2(FIXED, OR, or, AL, Ib, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES);
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_AL_Ib, &g_iemAImpl_or);
}


/**
 * @opcode      0x0d
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   af
 * @opflclear   of,cf
 * @optest      efl|=of,cf  op1=12 op2=7 -> op1=15   efl&|=nc,po,na,nz,pl,nv
 * @optest      efl|=of,cf  op1=0 op2=0  -> op1=0    efl&|=nc,po,na,zf,pl,nv
 * @optest      op1=-2 op2=1  -> op1=-1 efl&|=nc,po,na,nz,ng,nv
 * @optest      o16 / op1=0x5a5a             op2=0xa5a5     -> op1=-1 efl&|=nc,po,na,nz,ng,nv
 * @optest      o32 / op1=0x5a5a5a5a         op2=0xa5a5a5a5 -> op1=-1 efl&|=nc,po,na,nz,ng,nv
 * @optest      o64 / op1=0x5a5a5a5a5a5a5a5a op2=0xa5a5a5a5 -> op1=-1 efl&|=nc,po,na,nz,ng,nv
 * @optest      o64 / op1=0x5a5a5a5aa5a5a5a5 op2=0x5a5a5a5a -> op1=0x5a5a5a5affffffff efl&|=nc,po,na,nz,pl,nv
 */
FNIEMOP_DEF(iemOp_or_eAX_Iz)
{
    IEMOP_MNEMONIC2(FIXED, OR, or, rAX, Iz, DISOPTYPE_HARMLESS, 0);
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rAX_Iz, &g_iemAImpl_or);
}


/**
 * @opcode      0x0e
 * @opgroup     og_stack_sreg
 */
FNIEMOP_DEF(iemOp_push_CS)
{
    IEMOP_MNEMONIC1(FIXED, PUSH, push, CS, DISOPTYPE_HARMLESS | DISOPTYPE_POTENTIALLY_DANGEROUS | DISOPTYPE_INVALID_64, 0);
    IEMOP_HLP_NO_64BIT();
    return FNIEMOP_CALL_1(iemOpCommonPushSReg, X86_SREG_CS);
}


/**
 * @opcode      0x0f
 * @opmnemonic  EscTwo0f
 * @openc       two0f
 * @opdisenum   OP_2B_ESC
 * @ophints     harmless
 * @opgroup     og_escapes
 */
FNIEMOP_DEF(iemOp_2byteEscape)
{
#ifdef VBOX_STRICT
    /* Sanity check the table the first time around. */
    static bool s_fTested = false;
    if (RT_LIKELY(s_fTested)) { /* likely */  }
    else
    {
        s_fTested = true;
        Assert(g_apfnTwoByteMap[0xbc * 4 + 0] == iemOp_bsf_Gv_Ev);
        Assert(g_apfnTwoByteMap[0xbc * 4 + 1] == iemOp_bsf_Gv_Ev);
        Assert(g_apfnTwoByteMap[0xbc * 4 + 2] == iemOp_tzcnt_Gv_Ev);
        Assert(g_apfnTwoByteMap[0xbc * 4 + 3] == iemOp_bsf_Gv_Ev);
    }
#endif

    if (RT_LIKELY(IEM_GET_TARGET_CPU(pVCpu) >= IEMTARGETCPU_286))
    {
        uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
        IEMOP_HLP_MIN_286();
        return FNIEMOP_CALL(g_apfnTwoByteMap[(uintptr_t)b * 4 + pVCpu->iem.s.idxPrefix]);
    }
    /* @opdone */

    /*
     * On the 8086 this is a POP CS instruction.
     * For the time being we don't specify this this.
     */
    IEMOP_MNEMONIC1(FIXED, POP, pop, CS, DISOPTYPE_HARMLESS | DISOPTYPE_POTENTIALLY_DANGEROUS | DISOPTYPE_INVALID_64, IEMOPHINT_SKIP_PYTHON);
    IEMOP_HLP_NO_64BIT();
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_pop_Sreg, X86_SREG_ES, pVCpu->iem.s.enmEffOpSize);
}

/**
 * @opcode      0x10
 * @opgroup     og_gen_arith_bin
 * @opfltest    cf
 * @opflmodify  cf,pf,af,zf,sf,of
 * @optest      op1=1 op2=1 efl&~=cf -> op1=2 efl&|=nc,pe,na,nz,pl,nv
 * @optest      op1=1 op2=1 efl|=cf  -> op1=3 efl&|=nc,po,na,nz,pl,nv
 * @optest      op1=0xff op2=0 efl|=cf -> op1=0 efl&|=cf,po,af,zf,pl,nv
 * @optest      op1=0  op2=0 efl|=cf -> op1=1 efl&|=nc,pe,na,nz,pl,nv
 * @optest      op1=0  op2=0 efl&~=cf -> op1=0 efl&|=nc,po,na,zf,pl,nv
 */
FNIEMOP_DEF(iemOp_adc_Eb_Gb)
{
    IEMOP_MNEMONIC2(MR, ADC, adc, Eb, Gb, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES | IEMOPHINT_LOCK_ALLOWED);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_r8, &g_iemAImpl_adc);
}


/**
 * @opcode      0x11
 * @opgroup     og_gen_arith_bin
 * @opfltest    cf
 * @opflmodify  cf,pf,af,zf,sf,of
 * @optest      op1=1 op2=1 efl&~=cf -> op1=2 efl&|=nc,pe,na,nz,pl,nv
 * @optest      op1=1 op2=1 efl|=cf  -> op1=3 efl&|=nc,po,na,nz,pl,nv
 * @optest      op1=-1 op2=0 efl|=cf -> op1=0 efl&|=cf,po,af,zf,pl,nv
 * @optest      op1=0  op2=0 efl|=cf -> op1=1 efl&|=nc,pe,na,nz,pl,nv
 * @optest      op1=0  op2=0 efl&~=cf -> op1=0 efl&|=nc,po,na,zf,pl,nv
 */
FNIEMOP_DEF(iemOp_adc_Ev_Gv)
{
    IEMOP_MNEMONIC2(MR, ADC, adc, Ev, Gv, DISOPTYPE_HARMLESS, IEMOPHINT_LOCK_ALLOWED);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_rv, &g_iemAImpl_adc);
}


/**
 * @opcode      0x12
 * @opgroup     og_gen_arith_bin
 * @opfltest    cf
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opcopytests iemOp_adc_Eb_Gb
 */
FNIEMOP_DEF(iemOp_adc_Gb_Eb)
{
    IEMOP_MNEMONIC2(RM, ADC, adc, Gb, Eb, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_r8_rm, &g_iemAImpl_adc);
}


/**
 * @opcode      0x13
 * @opgroup     og_gen_arith_bin
 * @opfltest    cf
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opcopytests iemOp_adc_Ev_Gv
 */
FNIEMOP_DEF(iemOp_adc_Gv_Ev)
{
    IEMOP_MNEMONIC2(RM, ADC, adc, Gv, Ev, DISOPTYPE_HARMLESS, 0);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rv_rm, &g_iemAImpl_adc);
}


/**
 * @opcode      0x14
 * @opgroup     og_gen_arith_bin
 * @opfltest    cf
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opcopytests iemOp_adc_Eb_Gb
 */
FNIEMOP_DEF(iemOp_adc_Al_Ib)
{
    IEMOP_MNEMONIC2(FIXED, ADC, adc, AL, Ib, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_AL_Ib, &g_iemAImpl_adc);
}


/**
 * @opcode      0x15
 * @opgroup     og_gen_arith_bin
 * @opfltest    cf
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opcopytests iemOp_adc_Ev_Gv
 */
FNIEMOP_DEF(iemOp_adc_eAX_Iz)
{
    IEMOP_MNEMONIC2(FIXED, ADC, adc, rAX, Iz, DISOPTYPE_HARMLESS, 0);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rAX_Iz, &g_iemAImpl_adc);
}


/**
 * @opcode      0x16
 */
FNIEMOP_DEF(iemOp_push_SS)
{
    IEMOP_MNEMONIC1(FIXED, PUSH, push, SS, DISOPTYPE_HARMLESS | DISOPTYPE_INVALID_64 | DISOPTYPE_RRM_DANGEROUS, 0);
    IEMOP_HLP_NO_64BIT();
    return FNIEMOP_CALL_1(iemOpCommonPushSReg, X86_SREG_SS);
}


/**
 * @opcode      0x17
 * @opgroup     og_gen_arith_bin
 * @opfltest    cf
 * @opflmodify  cf,pf,af,zf,sf,of
 */
FNIEMOP_DEF(iemOp_pop_SS)
{
    IEMOP_MNEMONIC1(FIXED, POP, pop, SS, DISOPTYPE_HARMLESS | DISOPTYPE_INHIBIT_IRQS | DISOPTYPE_INVALID_64 | DISOPTYPE_RRM_DANGEROUS , 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_NO_64BIT();
    return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_pop_Sreg, X86_SREG_SS, pVCpu->iem.s.enmEffOpSize);
}


/**
 * @opcode      0x18
 * @opgroup     og_gen_arith_bin
 * @opfltest    cf
 * @opflmodify  cf,pf,af,zf,sf,of
 */
FNIEMOP_DEF(iemOp_sbb_Eb_Gb)
{
    IEMOP_MNEMONIC2(MR, SBB, sbb, Eb, Gb, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES | IEMOPHINT_LOCK_ALLOWED);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_r8, &g_iemAImpl_sbb);
}


/**
 * @opcode      0x19
 * @opgroup     og_gen_arith_bin
 * @opfltest    cf
 * @opflmodify  cf,pf,af,zf,sf,of
 */
FNIEMOP_DEF(iemOp_sbb_Ev_Gv)
{
    IEMOP_MNEMONIC2(MR, SBB, sbb, Ev, Gv, DISOPTYPE_HARMLESS, IEMOPHINT_LOCK_ALLOWED);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_rv, &g_iemAImpl_sbb);
}


/**
 * @opcode      0x1a
 * @opgroup     og_gen_arith_bin
 * @opfltest    cf
 * @opflmodify  cf,pf,af,zf,sf,of
 */
FNIEMOP_DEF(iemOp_sbb_Gb_Eb)
{
    IEMOP_MNEMONIC2(RM, SBB, sbb, Gb, Eb, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_r8_rm, &g_iemAImpl_sbb);
}


/**
 * @opcode      0x1b
 * @opgroup     og_gen_arith_bin
 * @opfltest    cf
 * @opflmodify  cf,pf,af,zf,sf,of
 */
FNIEMOP_DEF(iemOp_sbb_Gv_Ev)
{
    IEMOP_MNEMONIC2(RM, SBB, sbb, Gv, Ev, DISOPTYPE_HARMLESS, 0);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rv_rm, &g_iemAImpl_sbb);
}


/**
 * @opcode      0x1c
 * @opgroup     og_gen_arith_bin
 * @opfltest    cf
 * @opflmodify  cf,pf,af,zf,sf,of
 */
FNIEMOP_DEF(iemOp_sbb_Al_Ib)
{
    IEMOP_MNEMONIC2(FIXED, SBB, sbb, AL, Ib, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_AL_Ib, &g_iemAImpl_sbb);
}


/**
 * @opcode      0x1d
 * @opgroup     og_gen_arith_bin
 * @opfltest    cf
 * @opflmodify  cf,pf,af,zf,sf,of
 */
FNIEMOP_DEF(iemOp_sbb_eAX_Iz)
{
    IEMOP_MNEMONIC2(FIXED, SBB, sbb, rAX, Iz, DISOPTYPE_HARMLESS, 0);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rAX_Iz, &g_iemAImpl_sbb);
}


/**
 * @opcode      0x1e
 * @opgroup     og_stack_sreg
 */
FNIEMOP_DEF(iemOp_push_DS)
{
    IEMOP_MNEMONIC1(FIXED, PUSH, push, DS, DISOPTYPE_HARMLESS | DISOPTYPE_INVALID_64, 0);
    IEMOP_HLP_NO_64BIT();
    return FNIEMOP_CALL_1(iemOpCommonPushSReg, X86_SREG_DS);
}


/**
 * @opcode      0x1f
 * @opgroup     og_stack_sreg
 */
FNIEMOP_DEF(iemOp_pop_DS)
{
    IEMOP_MNEMONIC1(FIXED, POP, pop, DS, DISOPTYPE_HARMLESS | DISOPTYPE_INVALID_64 | DISOPTYPE_RRM_DANGEROUS, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_NO_64BIT();
    return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_pop_Sreg, X86_SREG_DS, pVCpu->iem.s.enmEffOpSize);
}


/**
 * @opcode      0x20
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   af
 * @opflclear   of,cf
 */
FNIEMOP_DEF(iemOp_and_Eb_Gb)
{
    IEMOP_MNEMONIC2(MR, AND, and, Eb, Gb, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES | IEMOPHINT_LOCK_ALLOWED);
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_r8, &g_iemAImpl_and);
}


/**
 * @opcode      0x21
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   af
 * @opflclear   of,cf
 */
FNIEMOP_DEF(iemOp_and_Ev_Gv)
{
    IEMOP_MNEMONIC2(MR, AND, and, Ev, Gv, DISOPTYPE_HARMLESS, IEMOPHINT_LOCK_ALLOWED);
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_rv, &g_iemAImpl_and);
}


/**
 * @opcode      0x22
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   af
 * @opflclear   of,cf
 */
FNIEMOP_DEF(iemOp_and_Gb_Eb)
{
    IEMOP_MNEMONIC2(RM, AND, and, Gb, Eb, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES);
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_r8_rm, &g_iemAImpl_and);
}


/**
 * @opcode      0x23
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   af
 * @opflclear   of,cf
 */
FNIEMOP_DEF(iemOp_and_Gv_Ev)
{
    IEMOP_MNEMONIC2(RM, AND, and, Gv, Ev, DISOPTYPE_HARMLESS, 0);
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rv_rm, &g_iemAImpl_and);
}


/**
 * @opcode      0x24
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   af
 * @opflclear   of,cf
 */
FNIEMOP_DEF(iemOp_and_Al_Ib)
{
    IEMOP_MNEMONIC2(FIXED, AND, and, AL, Ib, DISOPTYPE_HARMLESS, 0);
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_AL_Ib, &g_iemAImpl_and);
}


/**
 * @opcode      0x25
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   af
 * @opflclear   of,cf
 */
FNIEMOP_DEF(iemOp_and_eAX_Iz)
{
    IEMOP_MNEMONIC2(FIXED, AND, and, rAX, Iz, DISOPTYPE_HARMLESS, 0);
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rAX_Iz, &g_iemAImpl_and);
}


/**
 * @opcode      0x26
 * @opmnemonic  SEG
 * @op1         ES
 * @opgroup     og_prefix
 * @openc       prefix
 * @opdisenum   OP_SEG
 * @ophints     harmless
 */
FNIEMOP_DEF(iemOp_seg_ES)
{
    IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("seg es");
    pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_SEG_ES;
    pVCpu->iem.s.iEffSeg    = X86_SREG_ES;

    uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/**
 * @opcode      0x27
 * @opfltest    af,cf
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   of
 */
FNIEMOP_DEF(iemOp_daa)
{
    IEMOP_MNEMONIC0(FIXED, DAA, daa, DISOPTYPE_HARMLESS | DISOPTYPE_INVALID_64, 0); /* express implicit AL register use */
    IEMOP_HLP_NO_64BIT();
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_OF);
    return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_daa);
}


/**
 * @opcode      0x28
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 */
FNIEMOP_DEF(iemOp_sub_Eb_Gb)
{
    IEMOP_MNEMONIC2(MR, SUB, sub, Eb, Gb, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES | IEMOPHINT_LOCK_ALLOWED);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_r8, &g_iemAImpl_sub);
}


/**
 * @opcode      0x29
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 */
FNIEMOP_DEF(iemOp_sub_Ev_Gv)
{
    IEMOP_MNEMONIC2(MR, SUB, sub, Ev, Gv, DISOPTYPE_HARMLESS, IEMOPHINT_LOCK_ALLOWED);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_rv, &g_iemAImpl_sub);
}


/**
 * @opcode      0x2a
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 */
FNIEMOP_DEF(iemOp_sub_Gb_Eb)
{
    IEMOP_MNEMONIC2(RM, SUB, sub, Gb, Eb, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_r8_rm, &g_iemAImpl_sub);
}


/**
 * @opcode      0x2b
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 */
FNIEMOP_DEF(iemOp_sub_Gv_Ev)
{
    IEMOP_MNEMONIC2(RM, SUB, sub, Gv, Ev, DISOPTYPE_HARMLESS, 0);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rv_rm, &g_iemAImpl_sub);
}


/**
 * @opcode      0x2c
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 */
FNIEMOP_DEF(iemOp_sub_Al_Ib)
{
    IEMOP_MNEMONIC2(FIXED, SUB, sub, AL, Ib, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_AL_Ib, &g_iemAImpl_sub);
}


/**
 * @opcode      0x2d
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 */
FNIEMOP_DEF(iemOp_sub_eAX_Iz)
{
    IEMOP_MNEMONIC2(FIXED, SUB, sub, rAX, Iz, DISOPTYPE_HARMLESS, 0);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rAX_Iz, &g_iemAImpl_sub);
}


/**
 * @opcode      0x2e
 * @opmnemonic  SEG
 * @op1         CS
 * @opgroup     og_prefix
 * @openc       prefix
 * @opdisenum   OP_SEG
 * @ophints     harmless
 */
FNIEMOP_DEF(iemOp_seg_CS)
{
    IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("seg cs");
    pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_SEG_CS;
    pVCpu->iem.s.iEffSeg    = X86_SREG_CS;

    uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/**
 * @opcode      0x2f
 * @opfltest    af,cf
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   of
 */
FNIEMOP_DEF(iemOp_das)
{
    IEMOP_MNEMONIC0(FIXED, DAS, das, DISOPTYPE_HARMLESS | DISOPTYPE_INVALID_64, 0); /* express implicit AL register use */
    IEMOP_HLP_NO_64BIT();
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_OF);
    return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_das);
}


/**
 * @opcode      0x30
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   af
 * @opflclear   of,cf
 */
FNIEMOP_DEF(iemOp_xor_Eb_Gb)
{
    IEMOP_MNEMONIC2(MR, XOR, xor, Eb, Gb, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES | IEMOPHINT_LOCK_ALLOWED);
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_r8, &g_iemAImpl_xor);
}


/**
 * @opcode      0x31
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   af
 * @opflclear   of,cf
 */
FNIEMOP_DEF(iemOp_xor_Ev_Gv)
{
    IEMOP_MNEMONIC2(MR, XOR, xor, Ev, Gv, DISOPTYPE_HARMLESS, IEMOPHINT_LOCK_ALLOWED);
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_rv, &g_iemAImpl_xor);
}


/**
 * @opcode      0x32
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   af
 * @opflclear   of,cf
 */
FNIEMOP_DEF(iemOp_xor_Gb_Eb)
{
    IEMOP_MNEMONIC2(RM, XOR, xor, Gb, Eb, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES);
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_r8_rm, &g_iemAImpl_xor);
}


/**
 * @opcode      0x33
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   af
 * @opflclear   of,cf
 */
FNIEMOP_DEF(iemOp_xor_Gv_Ev)
{
    IEMOP_MNEMONIC2(RM, XOR, xor, Gv, Ev, DISOPTYPE_HARMLESS, 0);
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rv_rm, &g_iemAImpl_xor);
}


/**
 * @opcode      0x34
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   af
 * @opflclear   of,cf
 */
FNIEMOP_DEF(iemOp_xor_Al_Ib)
{
    IEMOP_MNEMONIC2(FIXED, XOR, xor, AL, Ib, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES);
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_AL_Ib, &g_iemAImpl_xor);
}


/**
 * @opcode      0x35
 * @opgroup     og_gen_arith_bin
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   af
 * @opflclear   of,cf
 */
FNIEMOP_DEF(iemOp_xor_eAX_Iz)
{
    IEMOP_MNEMONIC2(FIXED, XOR, xor, rAX, Iz, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES);
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rAX_Iz, &g_iemAImpl_xor);
}


/**
 * @opcode      0x36
 * @opmnemonic  SEG
 * @op1         SS
 * @opgroup     og_prefix
 * @openc       prefix
 * @opdisenum   OP_SEG
 * @ophints     harmless
 */
FNIEMOP_DEF(iemOp_seg_SS)
{
    IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("seg ss");
    pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_SEG_SS;
    pVCpu->iem.s.iEffSeg    = X86_SREG_SS;

    uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/**
 * @opcode      0x37
 * @opfltest    af,cf
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   pf,zf,sf,of
 * @opgroup     og_gen_arith_dec
 * @optest              efl&~=af ax=9      -> efl&|=nc,po,na,nz,pl,nv
 * @optest              efl&~=af ax=0      -> efl&|=nc,po,na,zf,pl,nv
 * @optest      intel / efl&~=af ax=0x00f0 -> ax=0x0000 efl&|=nc,po,na,zf,pl,nv
 * @optest      amd   / efl&~=af ax=0x00f0 -> ax=0x0000 efl&|=nc,po,na,nz,pl,nv
 * @optest              efl&~=af ax=0x00f9 -> ax=0x0009 efl&|=nc,po,na,nz,pl,nv
 * @optest              efl|=af  ax=0      -> ax=0x0106 efl&|=cf,po,af,nz,pl,nv
 * @optest              efl|=af  ax=0x0100 -> ax=0x0206 efl&|=cf,po,af,nz,pl,nv
 * @optest      intel / efl|=af  ax=0x000a -> ax=0x0100 efl&|=cf,po,af,zf,pl,nv
 * @optest      amd   / efl|=af  ax=0x000a -> ax=0x0100 efl&|=cf,pe,af,nz,pl,nv
 * @optest      intel / efl|=af  ax=0x010a -> ax=0x0200 efl&|=cf,po,af,zf,pl,nv
 * @optest      amd   / efl|=af  ax=0x010a -> ax=0x0200 efl&|=cf,pe,af,nz,pl,nv
 * @optest      intel / efl|=af  ax=0x0f0a -> ax=0x1000 efl&|=cf,po,af,zf,pl,nv
 * @optest      amd   / efl|=af  ax=0x0f0a -> ax=0x1000 efl&|=cf,pe,af,nz,pl,nv
 * @optest      intel / efl|=af  ax=0x7f0a -> ax=0x8000 efl&|=cf,po,af,zf,pl,nv
 * @optest      amd   / efl|=af  ax=0x7f0a -> ax=0x8000 efl&|=cf,pe,af,nz,ng,ov
 * @optest      intel / efl|=af  ax=0xff0a -> ax=0x0000 efl&|=cf,po,af,zf,pl,nv
 * @optest      amd   / efl|=af  ax=0xff0a -> ax=0x0000 efl&|=cf,pe,af,nz,pl,nv
 * @optest      intel / efl&~=af ax=0xff0a -> ax=0x0000 efl&|=cf,po,af,zf,pl,nv
 * @optest      amd   / efl&~=af ax=0xff0a -> ax=0x0000 efl&|=cf,pe,af,nz,pl,nv
 * @optest      intel / efl&~=af ax=0x000b -> ax=0x0101 efl&|=cf,pe,af,nz,pl,nv
 * @optest      amd   / efl&~=af ax=0x000b -> ax=0x0101 efl&|=cf,po,af,nz,pl,nv
 * @optest      intel / efl&~=af ax=0x000c -> ax=0x0102 efl&|=cf,pe,af,nz,pl,nv
 * @optest      amd   / efl&~=af ax=0x000c -> ax=0x0102 efl&|=cf,po,af,nz,pl,nv
 * @optest      intel / efl&~=af ax=0x000d -> ax=0x0103 efl&|=cf,po,af,nz,pl,nv
 * @optest      amd   / efl&~=af ax=0x000d -> ax=0x0103 efl&|=cf,pe,af,nz,pl,nv
 * @optest      intel / efl&~=af ax=0x000e -> ax=0x0104 efl&|=cf,pe,af,nz,pl,nv
 * @optest      amd   / efl&~=af ax=0x000e -> ax=0x0104 efl&|=cf,po,af,nz,pl,nv
 * @optest      intel / efl&~=af ax=0x000f -> ax=0x0105 efl&|=cf,po,af,nz,pl,nv
 * @optest      amd   / efl&~=af ax=0x000f -> ax=0x0105 efl&|=cf,pe,af,nz,pl,nv
 * @optest      intel / efl&~=af ax=0x020f -> ax=0x0305 efl&|=cf,po,af,nz,pl,nv
 * @optest      amd   / efl&~=af ax=0x020f -> ax=0x0305 efl&|=cf,pe,af,nz,pl,nv
 */
FNIEMOP_DEF(iemOp_aaa)
{
    IEMOP_MNEMONIC0(FIXED, AAA, aaa, DISOPTYPE_HARMLESS | DISOPTYPE_INVALID_64, 0); /* express implicit AL/AX register use */
    IEMOP_HLP_NO_64BIT();
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_OF);

    return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_aaa);
}


/**
 * @opcode      0x38
 */
FNIEMOP_DEF(iemOp_cmp_Eb_Gb)
{
    IEMOP_MNEMONIC(cmp_Eb_Gb, "cmp Eb,Gb");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_r8, &g_iemAImpl_cmp);
}


/**
 * @opcode      0x39
 */
FNIEMOP_DEF(iemOp_cmp_Ev_Gv)
{
    IEMOP_MNEMONIC(cmp_Ev_Gv, "cmp Ev,Gv");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_rv, &g_iemAImpl_cmp);
}


/**
 * @opcode      0x3a
 */
FNIEMOP_DEF(iemOp_cmp_Gb_Eb)
{
    IEMOP_MNEMONIC(cmp_Gb_Eb, "cmp Gb,Eb");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_r8_rm, &g_iemAImpl_cmp);
}


/**
 * @opcode      0x3b
 */
FNIEMOP_DEF(iemOp_cmp_Gv_Ev)
{
    IEMOP_MNEMONIC(cmp_Gv_Ev, "cmp Gv,Ev");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rv_rm, &g_iemAImpl_cmp);
}


/**
 * @opcode      0x3c
 */
FNIEMOP_DEF(iemOp_cmp_Al_Ib)
{
    IEMOP_MNEMONIC(cmp_al_Ib, "cmp al,Ib");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_AL_Ib, &g_iemAImpl_cmp);
}


/**
 * @opcode      0x3d
 */
FNIEMOP_DEF(iemOp_cmp_eAX_Iz)
{
    IEMOP_MNEMONIC(cmp_rAX_Iz, "cmp rAX,Iz");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rAX_Iz, &g_iemAImpl_cmp);
}


/**
 * @opcode      0x3e
 */
FNIEMOP_DEF(iemOp_seg_DS)
{
    IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("seg ds");
    pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_SEG_DS;
    pVCpu->iem.s.iEffSeg    = X86_SREG_DS;

    uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/**
 * @opcode      0x3f
 * @opfltest    af,cf
 * @opflmodify  cf,pf,af,zf,sf,of
 * @opflundef   pf,zf,sf,of
 * @opgroup     og_gen_arith_dec
 * @optest            / efl&~=af ax=0x0009 -> efl&|=nc,po,na,nz,pl,nv
 * @optest            / efl&~=af ax=0x0000 -> efl&|=nc,po,na,zf,pl,nv
 * @optest      intel / efl&~=af ax=0x00f0 -> ax=0x0000 efl&|=nc,po,na,zf,pl,nv
 * @optest      amd   / efl&~=af ax=0x00f0 -> ax=0x0000 efl&|=nc,po,na,nz,pl,nv
 * @optest            / efl&~=af ax=0x00f9 -> ax=0x0009 efl&|=nc,po,na,nz,pl,nv
 * @optest      intel / efl|=af  ax=0x0000 -> ax=0xfe0a efl&|=cf,po,af,nz,pl,nv
 * @optest      amd   / efl|=af  ax=0x0000 -> ax=0xfe0a efl&|=cf,po,af,nz,ng,nv
 * @optest      intel / efl|=af  ax=0x0100 -> ax=0xff0a efl&|=cf,po,af,nz,pl,nv
 * @optest      amd   / efl|=af  ax=0x0100 -> ax=0xff0a efl&|=cf,po,af,nz,ng,nv
 * @optest      intel / efl|=af  ax=0x000a -> ax=0xff04 efl&|=cf,pe,af,nz,pl,nv
 * @optest      amd   / efl|=af  ax=0x000a -> ax=0xff04 efl&|=cf,pe,af,nz,ng,nv
 * @optest            / efl|=af  ax=0x010a -> ax=0x0004 efl&|=cf,pe,af,nz,pl,nv
 * @optest            / efl|=af  ax=0x020a -> ax=0x0104 efl&|=cf,pe,af,nz,pl,nv
 * @optest            / efl|=af  ax=0x0f0a -> ax=0x0e04 efl&|=cf,pe,af,nz,pl,nv
 * @optest            / efl|=af  ax=0x7f0a -> ax=0x7e04 efl&|=cf,pe,af,nz,pl,nv
 * @optest      intel / efl|=af  ax=0xff0a -> ax=0xfe04 efl&|=cf,pe,af,nz,pl,nv
 * @optest      amd   / efl|=af  ax=0xff0a -> ax=0xfe04 efl&|=cf,pe,af,nz,ng,nv
 * @optest      intel / efl&~=af ax=0xff0a -> ax=0xfe04 efl&|=cf,pe,af,nz,pl,nv
 * @optest      amd   / efl&~=af ax=0xff0a -> ax=0xfe04 efl&|=cf,pe,af,nz,ng,nv
 * @optest      intel / efl&~=af ax=0xff09 -> ax=0xff09 efl&|=nc,po,na,nz,pl,nv
 * @optest      amd   / efl&~=af ax=0xff09 -> ax=0xff09 efl&|=nc,po,na,nz,ng,nv
 * @optest      intel / efl&~=af ax=0x000b -> ax=0xff05 efl&|=cf,po,af,nz,pl,nv
 * @optest      amd   / efl&~=af ax=0x000b -> ax=0xff05 efl&|=cf,po,af,nz,ng,nv
 * @optest      intel / efl&~=af ax=0x000c -> ax=0xff06 efl&|=cf,po,af,nz,pl,nv
 * @optest      amd   / efl&~=af ax=0x000c -> ax=0xff06 efl&|=cf,po,af,nz,ng,nv
 * @optest      intel / efl&~=af ax=0x000d -> ax=0xff07 efl&|=cf,pe,af,nz,pl,nv
 * @optest      amd   / efl&~=af ax=0x000d -> ax=0xff07 efl&|=cf,pe,af,nz,ng,nv
 * @optest      intel / efl&~=af ax=0x000e -> ax=0xff08 efl&|=cf,pe,af,nz,pl,nv
 * @optest      amd   / efl&~=af ax=0x000e -> ax=0xff08 efl&|=cf,pe,af,nz,ng,nv
 * @optest      intel / efl&~=af ax=0x000f -> ax=0xff09 efl&|=cf,po,af,nz,pl,nv
 * @optest      amd   / efl&~=af ax=0x000f -> ax=0xff09 efl&|=cf,po,af,nz,ng,nv
 * @optest      intel / efl&~=af ax=0x00fa -> ax=0xff04 efl&|=cf,pe,af,nz,pl,nv
 * @optest      amd   / efl&~=af ax=0x00fa -> ax=0xff04 efl&|=cf,pe,af,nz,ng,nv
 * @optest      intel / efl&~=af ax=0xfffa -> ax=0xfe04 efl&|=cf,pe,af,nz,pl,nv
 * @optest      amd   / efl&~=af ax=0xfffa -> ax=0xfe04 efl&|=cf,pe,af,nz,ng,nv
 */
FNIEMOP_DEF(iemOp_aas)
{
    IEMOP_MNEMONIC0(FIXED, AAS, aas, DISOPTYPE_HARMLESS | DISOPTYPE_INVALID_64, 0); /* express implicit AL/AX register use */
    IEMOP_HLP_NO_64BIT();
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_OF | X86_EFL_OF);

    return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_aas);
}


/**
 * Common 'inc/dec/not/neg register' helper.
 */
FNIEMOP_DEF_2(iemOpCommonUnaryGReg, PCIEMOPUNARYSIZES, pImpl, uint8_t, iReg)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(2, 0);
            IEM_MC_ARG(uint16_t *,  pu16Dst, 0);
            IEM_MC_ARG(uint32_t *,  pEFlags, 1);
            IEM_MC_REF_GREG_U16(pu16Dst, iReg);
            IEM_MC_REF_EFLAGS(pEFlags);
            IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnNormalU16, pu16Dst, pEFlags);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(2, 0);
            IEM_MC_ARG(uint32_t *,  pu32Dst, 0);
            IEM_MC_ARG(uint32_t *,  pEFlags, 1);
            IEM_MC_REF_GREG_U32(pu32Dst, iReg);
            IEM_MC_REF_EFLAGS(pEFlags);
            IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnNormalU32, pu32Dst, pEFlags);
            IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(pu32Dst);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(2, 0);
            IEM_MC_ARG(uint64_t *,  pu64Dst, 0);
            IEM_MC_ARG(uint32_t *,  pEFlags, 1);
            IEM_MC_REF_GREG_U64(pu64Dst, iReg);
            IEM_MC_REF_EFLAGS(pEFlags);
            IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnNormalU64, pu64Dst, pEFlags);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            return VINF_SUCCESS;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0x40
 */
FNIEMOP_DEF(iemOp_inc_eAX)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("rex");
        pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_REX;

        uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC(inc_eAX, "inc eAX");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_inc, X86_GREG_xAX);
}


/**
 * @opcode      0x41
 */
FNIEMOP_DEF(iemOp_inc_eCX)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("rex.b");
        pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_B;
        pVCpu->iem.s.uRexB     = 1 << 3;

        uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC(inc_eCX, "inc eCX");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_inc, X86_GREG_xCX);
}


/**
 * @opcode      0x42
 */
FNIEMOP_DEF(iemOp_inc_eDX)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("rex.x");
        pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_X;
        pVCpu->iem.s.uRexIndex = 1 << 3;

        uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC(inc_eDX, "inc eDX");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_inc, X86_GREG_xDX);
}



/**
 * @opcode      0x43
 */
FNIEMOP_DEF(iemOp_inc_eBX)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("rex.bx");
        pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_B | IEM_OP_PRF_REX_X;
        pVCpu->iem.s.uRexB     = 1 << 3;
        pVCpu->iem.s.uRexIndex = 1 << 3;

        uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC(inc_eBX, "inc eBX");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_inc, X86_GREG_xBX);
}


/**
 * @opcode      0x44
 */
FNIEMOP_DEF(iemOp_inc_eSP)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("rex.r");
        pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_R;
        pVCpu->iem.s.uRexReg   = 1 << 3;

        uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC(inc_eSP, "inc eSP");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_inc, X86_GREG_xSP);
}


/**
 * @opcode      0x45
 */
FNIEMOP_DEF(iemOp_inc_eBP)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("rex.rb");
        pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_R | IEM_OP_PRF_REX_B;
        pVCpu->iem.s.uRexReg   = 1 << 3;
        pVCpu->iem.s.uRexB     = 1 << 3;

        uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC(inc_eBP, "inc eBP");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_inc, X86_GREG_xBP);
}


/**
 * @opcode      0x46
 */
FNIEMOP_DEF(iemOp_inc_eSI)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("rex.rx");
        pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_R | IEM_OP_PRF_REX_X;
        pVCpu->iem.s.uRexReg   = 1 << 3;
        pVCpu->iem.s.uRexIndex = 1 << 3;

        uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC(inc_eSI, "inc eSI");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_inc, X86_GREG_xSI);
}


/**
 * @opcode      0x47
 */
FNIEMOP_DEF(iemOp_inc_eDI)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("rex.rbx");
        pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_R | IEM_OP_PRF_REX_B | IEM_OP_PRF_REX_X;
        pVCpu->iem.s.uRexReg   = 1 << 3;
        pVCpu->iem.s.uRexB     = 1 << 3;
        pVCpu->iem.s.uRexIndex = 1 << 3;

        uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC(inc_eDI, "inc eDI");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_inc, X86_GREG_xDI);
}


/**
 * @opcode      0x48
 */
FNIEMOP_DEF(iemOp_dec_eAX)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("rex.w");
        pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_SIZE_REX_W;
        iemRecalEffOpSize(pVCpu);

        uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC(dec_eAX, "dec eAX");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_dec, X86_GREG_xAX);
}


/**
 * @opcode      0x49
 */
FNIEMOP_DEF(iemOp_dec_eCX)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("rex.bw");
        pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_B | IEM_OP_PRF_SIZE_REX_W;
        pVCpu->iem.s.uRexB     = 1 << 3;
        iemRecalEffOpSize(pVCpu);

        uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC(dec_eCX, "dec eCX");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_dec, X86_GREG_xCX);
}


/**
 * @opcode      0x4a
 */
FNIEMOP_DEF(iemOp_dec_eDX)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("rex.xw");
        pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_X | IEM_OP_PRF_SIZE_REX_W;
        pVCpu->iem.s.uRexIndex = 1 << 3;
        iemRecalEffOpSize(pVCpu);

        uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC(dec_eDX, "dec eDX");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_dec, X86_GREG_xDX);
}


/**
 * @opcode      0x4b
 */
FNIEMOP_DEF(iemOp_dec_eBX)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("rex.bxw");
        pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_B | IEM_OP_PRF_REX_X | IEM_OP_PRF_SIZE_REX_W;
        pVCpu->iem.s.uRexB     = 1 << 3;
        pVCpu->iem.s.uRexIndex = 1 << 3;
        iemRecalEffOpSize(pVCpu);

        uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC(dec_eBX, "dec eBX");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_dec, X86_GREG_xBX);
}


/**
 * @opcode      0x4c
 */
FNIEMOP_DEF(iemOp_dec_eSP)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("rex.rw");
        pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_R | IEM_OP_PRF_SIZE_REX_W;
        pVCpu->iem.s.uRexReg   = 1 << 3;
        iemRecalEffOpSize(pVCpu);

        uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC(dec_eSP, "dec eSP");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_dec, X86_GREG_xSP);
}


/**
 * @opcode      0x4d
 */
FNIEMOP_DEF(iemOp_dec_eBP)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("rex.rbw");
        pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_R | IEM_OP_PRF_REX_B | IEM_OP_PRF_SIZE_REX_W;
        pVCpu->iem.s.uRexReg   = 1 << 3;
        pVCpu->iem.s.uRexB     = 1 << 3;
        iemRecalEffOpSize(pVCpu);

        uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC(dec_eBP, "dec eBP");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_dec, X86_GREG_xBP);
}


/**
 * @opcode      0x4e
 */
FNIEMOP_DEF(iemOp_dec_eSI)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("rex.rxw");
        pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_R | IEM_OP_PRF_REX_X | IEM_OP_PRF_SIZE_REX_W;
        pVCpu->iem.s.uRexReg   = 1 << 3;
        pVCpu->iem.s.uRexIndex = 1 << 3;
        iemRecalEffOpSize(pVCpu);

        uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC(dec_eSI, "dec eSI");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_dec, X86_GREG_xSI);
}


/**
 * @opcode      0x4f
 */
FNIEMOP_DEF(iemOp_dec_eDI)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("rex.rbxw");
        pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_R | IEM_OP_PRF_REX_B | IEM_OP_PRF_REX_X | IEM_OP_PRF_SIZE_REX_W;
        pVCpu->iem.s.uRexReg   = 1 << 3;
        pVCpu->iem.s.uRexB     = 1 << 3;
        pVCpu->iem.s.uRexIndex = 1 << 3;
        iemRecalEffOpSize(pVCpu);

        uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC(dec_eDI, "dec eDI");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_dec, X86_GREG_xDI);
}


/**
 * Common 'push register' helper.
 */
FNIEMOP_DEF_1(iemOpCommonPushGReg, uint8_t, iReg)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        iReg |= pVCpu->iem.s.uRexB;
        pVCpu->iem.s.enmDefOpSize = IEMMODE_64BIT;
        pVCpu->iem.s.enmEffOpSize = !(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_SIZE_OP) ? IEMMODE_64BIT : IEMMODE_16BIT;
    }

    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint16_t, u16Value);
            IEM_MC_FETCH_GREG_U16(u16Value, iReg);
            IEM_MC_PUSH_U16(u16Value);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint32_t, u32Value);
            IEM_MC_FETCH_GREG_U32(u32Value, iReg);
            IEM_MC_PUSH_U32(u32Value);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint64_t, u64Value);
            IEM_MC_FETCH_GREG_U64(u64Value, iReg);
            IEM_MC_PUSH_U64(u64Value);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0x50
 */
FNIEMOP_DEF(iemOp_push_eAX)
{
    IEMOP_MNEMONIC(push_rAX, "push rAX");
    return FNIEMOP_CALL_1(iemOpCommonPushGReg, X86_GREG_xAX);
}


/**
 * @opcode      0x51
 */
FNIEMOP_DEF(iemOp_push_eCX)
{
    IEMOP_MNEMONIC(push_rCX, "push rCX");
    return FNIEMOP_CALL_1(iemOpCommonPushGReg, X86_GREG_xCX);
}


/**
 * @opcode      0x52
 */
FNIEMOP_DEF(iemOp_push_eDX)
{
    IEMOP_MNEMONIC(push_rDX, "push rDX");
    return FNIEMOP_CALL_1(iemOpCommonPushGReg, X86_GREG_xDX);
}


/**
 * @opcode      0x53
 */
FNIEMOP_DEF(iemOp_push_eBX)
{
    IEMOP_MNEMONIC(push_rBX, "push rBX");
    return FNIEMOP_CALL_1(iemOpCommonPushGReg, X86_GREG_xBX);
}


/**
 * @opcode      0x54
 */
FNIEMOP_DEF(iemOp_push_eSP)
{
    IEMOP_MNEMONIC(push_rSP, "push rSP");
    if (IEM_GET_TARGET_CPU(pVCpu) == IEMTARGETCPU_8086)
    {
        IEM_MC_BEGIN(0, 1);
        IEM_MC_LOCAL(uint16_t, u16Value);
        IEM_MC_FETCH_GREG_U16(u16Value, X86_GREG_xSP);
        IEM_MC_SUB_LOCAL_U16(u16Value, 2);
        IEM_MC_PUSH_U16(u16Value);
        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
    return FNIEMOP_CALL_1(iemOpCommonPushGReg, X86_GREG_xSP);
}


/**
 * @opcode      0x55
 */
FNIEMOP_DEF(iemOp_push_eBP)
{
    IEMOP_MNEMONIC(push_rBP, "push rBP");
    return FNIEMOP_CALL_1(iemOpCommonPushGReg, X86_GREG_xBP);
}


/**
 * @opcode      0x56
 */
FNIEMOP_DEF(iemOp_push_eSI)
{
    IEMOP_MNEMONIC(push_rSI, "push rSI");
    return FNIEMOP_CALL_1(iemOpCommonPushGReg, X86_GREG_xSI);
}


/**
 * @opcode      0x57
 */
FNIEMOP_DEF(iemOp_push_eDI)
{
    IEMOP_MNEMONIC(push_rDI, "push rDI");
    return FNIEMOP_CALL_1(iemOpCommonPushGReg, X86_GREG_xDI);
}


/**
 * Common 'pop register' helper.
 */
FNIEMOP_DEF_1(iemOpCommonPopGReg, uint8_t, iReg)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        iReg |= pVCpu->iem.s.uRexB;
        pVCpu->iem.s.enmDefOpSize = IEMMODE_64BIT;
        pVCpu->iem.s.enmEffOpSize = !(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_SIZE_OP) ? IEMMODE_64BIT : IEMMODE_16BIT;
    }

    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint16_t *, pu16Dst);
            IEM_MC_REF_GREG_U16(pu16Dst, iReg);
            IEM_MC_POP_U16(pu16Dst);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint32_t *, pu32Dst);
            IEM_MC_REF_GREG_U32(pu32Dst, iReg);
            IEM_MC_POP_U32(pu32Dst);
            IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(pu32Dst); /** @todo testcase*/
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint64_t *, pu64Dst);
            IEM_MC_REF_GREG_U64(pu64Dst, iReg);
            IEM_MC_POP_U64(pu64Dst);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0x58
 */
FNIEMOP_DEF(iemOp_pop_eAX)
{
    IEMOP_MNEMONIC(pop_rAX, "pop rAX");
    return FNIEMOP_CALL_1(iemOpCommonPopGReg, X86_GREG_xAX);
}


/**
 * @opcode      0x59
 */
FNIEMOP_DEF(iemOp_pop_eCX)
{
    IEMOP_MNEMONIC(pop_rCX, "pop rCX");
    return FNIEMOP_CALL_1(iemOpCommonPopGReg, X86_GREG_xCX);
}


/**
 * @opcode      0x5a
 */
FNIEMOP_DEF(iemOp_pop_eDX)
{
    IEMOP_MNEMONIC(pop_rDX, "pop rDX");
    return FNIEMOP_CALL_1(iemOpCommonPopGReg, X86_GREG_xDX);
}


/**
 * @opcode      0x5b
 */
FNIEMOP_DEF(iemOp_pop_eBX)
{
    IEMOP_MNEMONIC(pop_rBX, "pop rBX");
    return FNIEMOP_CALL_1(iemOpCommonPopGReg, X86_GREG_xBX);
}


/**
 * @opcode      0x5c
 */
FNIEMOP_DEF(iemOp_pop_eSP)
{
    IEMOP_MNEMONIC(pop_rSP, "pop rSP");
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        if (pVCpu->iem.s.uRexB)
            return FNIEMOP_CALL_1(iemOpCommonPopGReg, X86_GREG_xSP);
        pVCpu->iem.s.enmDefOpSize = IEMMODE_64BIT;
        pVCpu->iem.s.enmEffOpSize = !(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_SIZE_OP) ? IEMMODE_64BIT : IEMMODE_16BIT;
    }

    IEMOP_HLP_DECODED_NL_1(OP_POP, IEMOPFORM_FIXED, OP_PARM_REG_ESP,
                           DISOPTYPE_HARMLESS | DISOPTYPE_DEFAULT_64_OP_SIZE | DISOPTYPE_REXB_EXTENDS_OPREG);
    /** @todo add testcase for this instruction. */
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint16_t, u16Dst);
            IEM_MC_POP_U16(&u16Dst); /** @todo not correct MC, fix later. */
            IEM_MC_STORE_GREG_U16(X86_GREG_xSP, u16Dst);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint32_t, u32Dst);
            IEM_MC_POP_U32(&u32Dst);
            IEM_MC_STORE_GREG_U32(X86_GREG_xSP, u32Dst);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint64_t, u64Dst);
            IEM_MC_POP_U64(&u64Dst);
            IEM_MC_STORE_GREG_U64(X86_GREG_xSP, u64Dst);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0x5d
 */
FNIEMOP_DEF(iemOp_pop_eBP)
{
    IEMOP_MNEMONIC(pop_rBP, "pop rBP");
    return FNIEMOP_CALL_1(iemOpCommonPopGReg, X86_GREG_xBP);
}


/**
 * @opcode      0x5e
 */
FNIEMOP_DEF(iemOp_pop_eSI)
{
    IEMOP_MNEMONIC(pop_rSI, "pop rSI");
    return FNIEMOP_CALL_1(iemOpCommonPopGReg, X86_GREG_xSI);
}


/**
 * @opcode      0x5f
 */
FNIEMOP_DEF(iemOp_pop_eDI)
{
    IEMOP_MNEMONIC(pop_rDI, "pop rDI");
    return FNIEMOP_CALL_1(iemOpCommonPopGReg, X86_GREG_xDI);
}


/**
 * @opcode      0x60
 */
FNIEMOP_DEF(iemOp_pusha)
{
    IEMOP_MNEMONIC(pusha, "pusha");
    IEMOP_HLP_MIN_186();
    IEMOP_HLP_NO_64BIT();
    if (pVCpu->iem.s.enmEffOpSize == IEMMODE_16BIT)
        return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_pusha_16);
    Assert(pVCpu->iem.s.enmEffOpSize == IEMMODE_32BIT);
    return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_pusha_32);
}


/**
 * @opcode      0x61
 */
FNIEMOP_DEF(iemOp_popa__mvex)
{
    if (pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT)
    {
        IEMOP_MNEMONIC(popa, "popa");
        IEMOP_HLP_MIN_186();
        IEMOP_HLP_NO_64BIT();
        if (pVCpu->iem.s.enmEffOpSize == IEMMODE_16BIT)
            return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_popa_16);
        Assert(pVCpu->iem.s.enmEffOpSize == IEMMODE_32BIT);
        return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_popa_32);
    }
    IEMOP_MNEMONIC(mvex, "mvex");
    Log(("mvex prefix is not supported!\n"));
    return IEMOP_RAISE_INVALID_OPCODE();
}


/**
 * @opcode      0x62
 * @opmnemonic  bound
 * @op1         Gv_RO
 * @op2         Ma
 * @opmincpu    80186
 * @ophints     harmless invalid_64
 * @optest      op1=0 op2=0 ->
 * @optest      op1=1 op2=0 -> value.xcpt=5
 * @optest      o16 / op1=0xffff op2=0x0000fffe ->
 * @optest      o16 / op1=0xfffe op2=0x0000fffe ->
 * @optest      o16 / op1=0x7fff op2=0x0000fffe -> value.xcpt=5
 * @optest      o16 / op1=0x7fff op2=0x7ffffffe ->
 * @optest      o16 / op1=0x7fff op2=0xfffe8000 -> value.xcpt=5
 * @optest      o16 / op1=0x8000 op2=0xfffe8000 ->
 * @optest      o16 / op1=0xffff op2=0xfffe8000 -> value.xcpt=5
 * @optest      o16 / op1=0xfffe op2=0xfffe8000 ->
 * @optest      o16 / op1=0xfffe op2=0x8000fffe -> value.xcpt=5
 * @optest      o16 / op1=0x8000 op2=0x8000fffe -> value.xcpt=5
 * @optest      o16 / op1=0x0000 op2=0x8000fffe -> value.xcpt=5
 * @optest      o16 / op1=0x0001 op2=0x8000fffe -> value.xcpt=5
 * @optest      o16 / op1=0xffff op2=0x0001000f -> value.xcpt=5
 * @optest      o16 / op1=0x0000 op2=0x0001000f -> value.xcpt=5
 * @optest      o16 / op1=0x0001 op2=0x0001000f -> value.xcpt=5
 * @optest      o16 / op1=0x0002 op2=0x0001000f -> value.xcpt=5
 * @optest      o16 / op1=0x0003 op2=0x0001000f -> value.xcpt=5
 * @optest      o16 / op1=0x0004 op2=0x0001000f -> value.xcpt=5
 * @optest      o16 / op1=0x000e op2=0x0001000f -> value.xcpt=5
 * @optest      o16 / op1=0x000f op2=0x0001000f -> value.xcpt=5
 * @optest      o16 / op1=0x0010 op2=0x0001000f -> value.xcpt=5
 * @optest      o16 / op1=0x0011 op2=0x0001000f -> value.xcpt=5
 * @optest      o32 / op1=0xffffffff op2=0x00000000fffffffe ->
 * @optest      o32 / op1=0xfffffffe op2=0x00000000fffffffe ->
 * @optest      o32 / op1=0x7fffffff op2=0x00000000fffffffe -> value.xcpt=5
 * @optest      o32 / op1=0x7fffffff op2=0x7ffffffffffffffe ->
 * @optest      o32 / op1=0x7fffffff op2=0xfffffffe80000000 -> value.xcpt=5
 * @optest      o32 / op1=0x80000000 op2=0xfffffffe80000000 ->
 * @optest      o32 / op1=0xffffffff op2=0xfffffffe80000000 -> value.xcpt=5
 * @optest      o32 / op1=0xfffffffe op2=0xfffffffe80000000 ->
 * @optest      o32 / op1=0xfffffffe op2=0x80000000fffffffe -> value.xcpt=5
 * @optest      o32 / op1=0x80000000 op2=0x80000000fffffffe -> value.xcpt=5
 * @optest      o32 / op1=0x00000000 op2=0x80000000fffffffe -> value.xcpt=5
 * @optest      o32 / op1=0x00000002 op2=0x80000000fffffffe -> value.xcpt=5
 * @optest      o32 / op1=0x00000001 op2=0x0000000100000003 -> value.xcpt=5
 * @optest      o32 / op1=0x00000002 op2=0x0000000100000003 -> value.xcpt=5
 * @optest      o32 / op1=0x00000003 op2=0x0000000100000003 -> value.xcpt=5
 * @optest      o32 / op1=0x00000004 op2=0x0000000100000003 -> value.xcpt=5
 * @optest      o32 / op1=0x00000005 op2=0x0000000100000003 -> value.xcpt=5
 * @optest      o32 / op1=0x0000000e op2=0x0000000100000003 -> value.xcpt=5
 * @optest      o32 / op1=0x0000000f op2=0x0000000100000003 -> value.xcpt=5
 * @optest      o32 / op1=0x00000010 op2=0x0000000100000003 -> value.xcpt=5
 */
FNIEMOP_DEF(iemOp_bound_Gv_Ma__evex)
{
    /* The BOUND instruction is invalid 64-bit mode. In legacy and
       compatability mode it is invalid with MOD=3.

       In 32-bit mode, the EVEX prefix works by having the top two bits (MOD)
       both be set.  In the Intel EVEX documentation (sdm vol 2) these are simply
       given as R and X without an exact description, so we assume it builds on
       the VEX one and means they are inverted wrt REX.R and REX.X.  Thus, just
       like with the 3-byte VEX, 32-bit code is restrict wrt addressable registers. */
    uint8_t bRm;
    if (pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT)
    {
        IEMOP_MNEMONIC2(RM_MEM, BOUND, bound, Gv_RO, Ma, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES);
        IEMOP_HLP_MIN_186();
        IEM_OPCODE_GET_NEXT_U8(&bRm);
        if (IEM_IS_MODRM_MEM_MODE(bRm))
        {
            /** @todo testcase: check that there are two memory accesses involved.  Check
             *        whether they're both read before the \#BR triggers. */
            if (pVCpu->iem.s.enmEffOpSize == IEMMODE_16BIT)
            {
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint16_t,    u16Index,       0); /* Note! All operands are actually signed. Lazy unsigned bird. */
                IEM_MC_ARG(uint16_t,    u16LowerBounds, 1);
                IEM_MC_ARG(uint16_t,    u16UpperBounds, 2);
                IEM_MC_LOCAL(RTGCPTR,   GCPtrEffSrc);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

                IEM_MC_FETCH_GREG_U16(u16Index, IEM_GET_MODRM_REG_8(bRm));
                IEM_MC_FETCH_MEM_U16(u16LowerBounds, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
                IEM_MC_FETCH_MEM_U16_DISP(u16UpperBounds, pVCpu->iem.s.iEffSeg, GCPtrEffSrc, 2);

                IEM_MC_CALL_CIMPL_3(iemCImpl_bound_16, u16Index, u16LowerBounds, u16UpperBounds); /* returns */
                IEM_MC_END();
            }
            else /* 32-bit operands */
            {
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint32_t,    u32Index,       0); /* Note! All operands are actually signed. Lazy unsigned bird. */
                IEM_MC_ARG(uint32_t,    u32LowerBounds, 1);
                IEM_MC_ARG(uint32_t,    u32UpperBounds, 2);
                IEM_MC_LOCAL(RTGCPTR,   GCPtrEffSrc);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

                IEM_MC_FETCH_GREG_U32(u32Index, IEM_GET_MODRM_REG_8(bRm));
                IEM_MC_FETCH_MEM_U32(u32LowerBounds, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
                IEM_MC_FETCH_MEM_U32_DISP(u32UpperBounds, pVCpu->iem.s.iEffSeg, GCPtrEffSrc, 4);

                IEM_MC_CALL_CIMPL_3(iemCImpl_bound_32, u32Index, u32LowerBounds, u32UpperBounds); /* returns */
                IEM_MC_END();
            }
        }

        /*
         * @opdone
         */
        if (!IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fAvx512Foundation)
        {
            /* Note that there is no need for the CPU to fetch further bytes
               here because MODRM.MOD == 3. */
            Log(("evex not supported by the guest CPU!\n"));
            return IEMOP_RAISE_INVALID_OPCODE();
        }
    }
    else
    {
        /** @todo check how this is decoded in 64-bit mode w/o EVEX. Intel probably
         *        does modr/m read, whereas AMD probably doesn't... */
        if (!IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fAvx512Foundation)
        {
            Log(("evex not supported by the guest CPU!\n"));
            return FNIEMOP_CALL(iemOp_InvalidAllNeedRM);
        }
        IEM_OPCODE_GET_NEXT_U8(&bRm);
    }

    IEMOP_MNEMONIC(evex, "evex");
    uint8_t bP2; IEM_OPCODE_GET_NEXT_U8(&bP2);
    uint8_t bP3; IEM_OPCODE_GET_NEXT_U8(&bP3);
    Log(("evex prefix is not implemented!\n"));
    return VERR_IEM_INSTR_NOT_IMPLEMENTED;
}


/** Opcode 0x63 - non-64-bit modes. */
FNIEMOP_DEF(iemOp_arpl_Ew_Gw)
{
    IEMOP_MNEMONIC(arpl_Ew_Gw, "arpl Ew,Gw");
    IEMOP_HLP_MIN_286();
    IEMOP_HLP_NO_REAL_OR_V86_MODE();
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);

    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        /* Register */
        IEMOP_HLP_DECODED_NL_2(OP_ARPL, IEMOPFORM_MR_REG, OP_PARM_Ew, OP_PARM_Gw, DISOPTYPE_HARMLESS);
        IEM_MC_BEGIN(3, 0);
        IEM_MC_ARG(uint16_t *,      pu16Dst,    0);
        IEM_MC_ARG(uint16_t,        u16Src,     1);
        IEM_MC_ARG(uint32_t *,      pEFlags,    2);

        IEM_MC_FETCH_GREG_U16(u16Src, IEM_GET_MODRM_REG_8(bRm));
        IEM_MC_REF_GREG_U16(pu16Dst,  IEM_GET_MODRM_RM_8(bRm));
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_arpl, pu16Dst, u16Src, pEFlags);

        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
    else
    {
        /* Memory */
        IEM_MC_BEGIN(3, 2);
        IEM_MC_ARG(uint16_t *, pu16Dst,          0);
        IEM_MC_ARG(uint16_t,   u16Src,           1);
        IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
        IEMOP_HLP_DECODED_NL_2(OP_ARPL, IEMOPFORM_MR_REG, OP_PARM_Ew, OP_PARM_Gw, DISOPTYPE_HARMLESS);
        IEM_MC_MEM_MAP(pu16Dst, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
        IEM_MC_FETCH_GREG_U16(u16Src, IEM_GET_MODRM_REG_8(bRm));
        IEM_MC_FETCH_EFLAGS(EFlags);
        IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_arpl, pu16Dst, u16Src, pEFlags);

        IEM_MC_MEM_COMMIT_AND_UNMAP(pu16Dst, IEM_ACCESS_DATA_RW);
        IEM_MC_COMMIT_EFLAGS(EFlags);
        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
}


/**
 * @opcode 0x63
 *
 * @note This is a weird one. It works like a regular move instruction if
 *       REX.W isn't set, at least according to AMD docs (rev 3.15, 2009-11).
 * @todo This definitely needs a testcase to verify the odd cases.  */
FNIEMOP_DEF(iemOp_movsxd_Gv_Ev)
{
    Assert(pVCpu->iem.s.enmEffOpSize == IEMMODE_64BIT); /* Caller branched already . */

    IEMOP_MNEMONIC(movsxd_Gv_Ev, "movsxd Gv,Ev");
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);

    if (pVCpu->iem.s.fPrefixes & IEM_OP_PRF_SIZE_REX_W)
    {
        if (IEM_IS_MODRM_REG_MODE(bRm))
        {
            /*
             * Register to register.
             */
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint64_t, u64Value);
            IEM_MC_FETCH_GREG_U32_SX_U64(u64Value, IEM_GET_MODRM_RM(pVCpu, bRm));
            IEM_MC_STORE_GREG_U64(IEM_GET_MODRM_REG(pVCpu, bRm), u64Value);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
        }
        else
        {
            /*
             * We're loading a register from memory.
             */
            IEM_MC_BEGIN(0, 2);
            IEM_MC_LOCAL(uint64_t, u64Value);
            IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
            IEM_MC_FETCH_MEM_U32_SX_U64(u64Value, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
            IEM_MC_STORE_GREG_U64(IEM_GET_MODRM_REG(pVCpu, bRm), u64Value);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
        }
    }
    else
        AssertFailedReturn(VERR_IEM_INSTR_NOT_IMPLEMENTED);
}


/**
 * @opcode      0x64
 * @opmnemonic  segfs
 * @opmincpu    80386
 * @opgroup     og_prefixes
 */
FNIEMOP_DEF(iemOp_seg_FS)
{
    IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("seg fs");
    IEMOP_HLP_MIN_386();

    pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_SEG_FS;
    pVCpu->iem.s.iEffSeg    = X86_SREG_FS;

    uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/**
 * @opcode      0x65
 * @opmnemonic  seggs
 * @opmincpu    80386
 * @opgroup     og_prefixes
 */
FNIEMOP_DEF(iemOp_seg_GS)
{
    IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("seg gs");
    IEMOP_HLP_MIN_386();

    pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_SEG_GS;
    pVCpu->iem.s.iEffSeg    = X86_SREG_GS;

    uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/**
 * @opcode      0x66
 * @opmnemonic  opsize
 * @openc       prefix
 * @opmincpu    80386
 * @ophints     harmless
 * @opgroup     og_prefixes
 */
FNIEMOP_DEF(iemOp_op_size)
{
    IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("op size");
    IEMOP_HLP_MIN_386();

    pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_SIZE_OP;
    iemRecalEffOpSize(pVCpu);

    /* For the 4 entry opcode tables, the operand prefix doesn't not count
       when REPZ or REPNZ are present. */
    if (pVCpu->iem.s.idxPrefix == 0)
        pVCpu->iem.s.idxPrefix = 1;

    uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/**
 * @opcode      0x67
 * @opmnemonic  addrsize
 * @openc       prefix
 * @opmincpu    80386
 * @ophints     harmless
 * @opgroup     og_prefixes
 */
FNIEMOP_DEF(iemOp_addr_size)
{
    IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("addr size");
    IEMOP_HLP_MIN_386();

    pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_SIZE_ADDR;
    switch (pVCpu->iem.s.enmDefAddrMode)
    {
        case IEMMODE_16BIT: pVCpu->iem.s.enmEffAddrMode = IEMMODE_32BIT; break;
        case IEMMODE_32BIT: pVCpu->iem.s.enmEffAddrMode = IEMMODE_16BIT; break;
        case IEMMODE_64BIT: pVCpu->iem.s.enmEffAddrMode = IEMMODE_32BIT; break;
        default: AssertFailed();
    }

    uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/**
 * @opcode      0x68
 */
FNIEMOP_DEF(iemOp_push_Iz)
{
    IEMOP_MNEMONIC(push_Iz, "push Iz");
    IEMOP_HLP_MIN_186();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
        {
            uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(&u16Imm);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
            IEM_MC_BEGIN(0,0);
            IEM_MC_PUSH_U16(u16Imm);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            return VINF_SUCCESS;
        }

        case IEMMODE_32BIT:
        {
            uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(&u32Imm);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
            IEM_MC_BEGIN(0,0);
            IEM_MC_PUSH_U32(u32Imm);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            return VINF_SUCCESS;
        }

        case IEMMODE_64BIT:
        {
            uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S32_SX_U64(&u64Imm);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
            IEM_MC_BEGIN(0,0);
            IEM_MC_PUSH_U64(u64Imm);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            return VINF_SUCCESS;
        }

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0x69
 */
FNIEMOP_DEF(iemOp_imul_Gv_Ev_Iz)
{
    IEMOP_MNEMONIC(imul_Gv_Ev_Iz, "imul Gv,Ev,Iz"); /* Gv = Ev * Iz; */
    IEMOP_HLP_MIN_186();
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF);

    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
        {
            if (IEM_IS_MODRM_REG_MODE(bRm))
            {
                /* register operand */
                uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(&u16Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint16_t *,      pu16Dst,            0);
                IEM_MC_ARG_CONST(uint16_t,  u16Src,/*=*/ u16Imm,1);
                IEM_MC_ARG(uint32_t *,      pEFlags,            2);
                IEM_MC_LOCAL(uint16_t,      u16Tmp);

                IEM_MC_FETCH_GREG_U16(u16Tmp, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_LOCAL(pu16Dst, u16Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_imul_two_u16_eflags),
                                         pu16Dst, u16Src, pEFlags);
                IEM_MC_STORE_GREG_U16(IEM_GET_MODRM_REG(pVCpu, bRm), u16Tmp);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
            }
            else
            {
                /* memory operand */
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint16_t *,      pu16Dst,            0);
                IEM_MC_ARG(uint16_t,        u16Src,             1);
                IEM_MC_ARG(uint32_t *,      pEFlags,            2);
                IEM_MC_LOCAL(uint16_t,      u16Tmp);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 2);
                uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(&u16Imm);
                IEM_MC_ASSIGN(u16Src, u16Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U16(u16Tmp, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
                IEM_MC_REF_LOCAL(pu16Dst, u16Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_imul_two_u16_eflags),
                                         pu16Dst, u16Src, pEFlags);
                IEM_MC_STORE_GREG_U16(IEM_GET_MODRM_REG(pVCpu, bRm), u16Tmp);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
            }
            return VINF_SUCCESS;
        }

        case IEMMODE_32BIT:
        {
            if (IEM_IS_MODRM_REG_MODE(bRm))
            {
                /* register operand */
                uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(&u32Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint32_t *,      pu32Dst,            0);
                IEM_MC_ARG_CONST(uint32_t,  u32Src,/*=*/ u32Imm,1);
                IEM_MC_ARG(uint32_t *,      pEFlags,            2);
                IEM_MC_LOCAL(uint32_t,      u32Tmp);

                IEM_MC_FETCH_GREG_U32(u32Tmp, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_LOCAL(pu32Dst, u32Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_imul_two_u32_eflags),
                                         pu32Dst, u32Src, pEFlags);
                IEM_MC_STORE_GREG_U32(IEM_GET_MODRM_REG(pVCpu, bRm), u32Tmp);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
            }
            else
            {
                /* memory operand */
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint32_t *,      pu32Dst,            0);
                IEM_MC_ARG(uint32_t,        u32Src,             1);
                IEM_MC_ARG(uint32_t *,      pEFlags,            2);
                IEM_MC_LOCAL(uint32_t,      u32Tmp);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 4);
                uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(&u32Imm);
                IEM_MC_ASSIGN(u32Src, u32Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U32(u32Tmp, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
                IEM_MC_REF_LOCAL(pu32Dst, u32Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_imul_two_u32_eflags),
                                         pu32Dst, u32Src, pEFlags);
                IEM_MC_STORE_GREG_U32(IEM_GET_MODRM_REG(pVCpu, bRm), u32Tmp);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
            }
            return VINF_SUCCESS;
        }

        case IEMMODE_64BIT:
        {
            if (IEM_IS_MODRM_REG_MODE(bRm))
            {
                /* register operand */
                uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S32_SX_U64(&u64Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint64_t *,      pu64Dst,            0);
                IEM_MC_ARG_CONST(uint64_t,  u64Src,/*=*/ u64Imm,1);
                IEM_MC_ARG(uint32_t *,      pEFlags,            2);
                IEM_MC_LOCAL(uint64_t,      u64Tmp);

                IEM_MC_FETCH_GREG_U64(u64Tmp, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_LOCAL(pu64Dst, u64Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_imul_two_u64_eflags),
                                         pu64Dst, u64Src, pEFlags);
                IEM_MC_STORE_GREG_U64(IEM_GET_MODRM_REG(pVCpu, bRm), u64Tmp);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
            }
            else
            {
                /* memory operand */
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint64_t *,      pu64Dst,            0);
                IEM_MC_ARG(uint64_t,        u64Src,             1);
                IEM_MC_ARG(uint32_t *,      pEFlags,            2);
                IEM_MC_LOCAL(uint64_t,      u64Tmp);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 4);
                uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S32_SX_U64(&u64Imm);
                IEM_MC_ASSIGN(u64Src, u64Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U64(u64Tmp, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
                IEM_MC_REF_LOCAL(pu64Dst, u64Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_imul_two_u64_eflags),
                                         pu64Dst, u64Src, pEFlags);
                IEM_MC_STORE_GREG_U64(IEM_GET_MODRM_REG(pVCpu, bRm), u64Tmp);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
            }
            return VINF_SUCCESS;
        }

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0x6a
 */
FNIEMOP_DEF(iemOp_push_Ib)
{
    IEMOP_MNEMONIC(push_Ib, "push Ib");
    IEMOP_HLP_MIN_186();
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    IEM_MC_BEGIN(0,0);
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_PUSH_U16(i8Imm);
            break;
        case IEMMODE_32BIT:
            IEM_MC_PUSH_U32(i8Imm);
            break;
        case IEMMODE_64BIT:
            IEM_MC_PUSH_U64(i8Imm);
            break;
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/**
 * @opcode      0x6b
 */
FNIEMOP_DEF(iemOp_imul_Gv_Ev_Ib)
{
    IEMOP_MNEMONIC(imul_Gv_Ev_Ib, "imul Gv,Ev,Ib"); /* Gv = Ev * Iz; */
    IEMOP_HLP_MIN_186();
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF);

    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            if (IEM_IS_MODRM_REG_MODE(bRm))
            {
                /* register operand */
                uint8_t u8Imm; IEM_OPCODE_GET_NEXT_U8(&u8Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint16_t *,      pu16Dst,                    0);
                IEM_MC_ARG_CONST(uint16_t,  u16Src,/*=*/ (int8_t)u8Imm, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                    2);
                IEM_MC_LOCAL(uint16_t,      u16Tmp);

                IEM_MC_FETCH_GREG_U16(u16Tmp, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_LOCAL(pu16Dst, u16Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_imul_two_u16_eflags),
                                         pu16Dst, u16Src, pEFlags);
                IEM_MC_STORE_GREG_U16(IEM_GET_MODRM_REG(pVCpu, bRm), u16Tmp);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
            }
            else
            {
                /* memory operand */
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint16_t *,      pu16Dst,                    0);
                IEM_MC_ARG(uint16_t,        u16Src,                     1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                    2);
                IEM_MC_LOCAL(uint16_t,      u16Tmp);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 1);
                uint16_t u16Imm; IEM_OPCODE_GET_NEXT_S8_SX_U16(&u16Imm);
                IEM_MC_ASSIGN(u16Src, u16Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U16(u16Tmp, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
                IEM_MC_REF_LOCAL(pu16Dst, u16Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_imul_two_u16_eflags),
                                         pu16Dst, u16Src, pEFlags);
                IEM_MC_STORE_GREG_U16(IEM_GET_MODRM_REG(pVCpu, bRm), u16Tmp);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
            }
            return VINF_SUCCESS;

        case IEMMODE_32BIT:
            if (IEM_IS_MODRM_REG_MODE(bRm))
            {
                /* register operand */
                uint8_t u8Imm; IEM_OPCODE_GET_NEXT_U8(&u8Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint32_t *,      pu32Dst,                    0);
                IEM_MC_ARG_CONST(uint32_t,  u32Src,/*=*/ (int8_t)u8Imm, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                    2);
                IEM_MC_LOCAL(uint32_t,      u32Tmp);

                IEM_MC_FETCH_GREG_U32(u32Tmp, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_LOCAL(pu32Dst, u32Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_imul_two_u32_eflags),
                                         pu32Dst, u32Src, pEFlags);
                IEM_MC_STORE_GREG_U32(IEM_GET_MODRM_REG(pVCpu, bRm), u32Tmp);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
            }
            else
            {
                /* memory operand */
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint32_t *,      pu32Dst,                    0);
                IEM_MC_ARG(uint32_t,        u32Src,                     1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                    2);
                IEM_MC_LOCAL(uint32_t,      u32Tmp);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 1);
                uint32_t u32Imm; IEM_OPCODE_GET_NEXT_S8_SX_U32(&u32Imm);
                IEM_MC_ASSIGN(u32Src, u32Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U32(u32Tmp, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
                IEM_MC_REF_LOCAL(pu32Dst, u32Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_imul_two_u32_eflags),
                                         pu32Dst, u32Src, pEFlags);
                IEM_MC_STORE_GREG_U32(IEM_GET_MODRM_REG(pVCpu, bRm), u32Tmp);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
            }
            return VINF_SUCCESS;

        case IEMMODE_64BIT:
            if (IEM_IS_MODRM_REG_MODE(bRm))
            {
                /* register operand */
                uint8_t u8Imm; IEM_OPCODE_GET_NEXT_U8(&u8Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint64_t *,      pu64Dst,                    0);
                IEM_MC_ARG_CONST(uint64_t,  u64Src,/*=*/ (int8_t)u8Imm, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                    2);
                IEM_MC_LOCAL(uint64_t,      u64Tmp);

                IEM_MC_FETCH_GREG_U64(u64Tmp, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_LOCAL(pu64Dst, u64Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_imul_two_u64_eflags),
                                         pu64Dst, u64Src, pEFlags);
                IEM_MC_STORE_GREG_U64(IEM_GET_MODRM_REG(pVCpu, bRm), u64Tmp);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
            }
            else
            {
                /* memory operand */
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint64_t *,      pu64Dst,                    0);
                IEM_MC_ARG(uint64_t,        u64Src,                     1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                    2);
                IEM_MC_LOCAL(uint64_t,      u64Tmp);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 1);
                uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S8_SX_U64(&u64Imm);
                IEM_MC_ASSIGN(u64Src, u64Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U64(u64Tmp, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
                IEM_MC_REF_LOCAL(pu64Dst, u64Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_imul_two_u64_eflags),
                                         pu64Dst, u64Src, pEFlags);
                IEM_MC_STORE_GREG_U64(IEM_GET_MODRM_REG(pVCpu, bRm), u64Tmp);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
            }
            return VINF_SUCCESS;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
    AssertFailedReturn(VERR_IEM_IPE_8);
}


/**
 * @opcode      0x6c
 */
FNIEMOP_DEF(iemOp_insb_Yb_DX)
{
    IEMOP_HLP_MIN_186();
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    if (pVCpu->iem.s.fPrefixes & (IEM_OP_PRF_REPNZ | IEM_OP_PRF_REPZ))
    {
        IEMOP_MNEMONIC(rep_insb_Yb_DX, "rep ins Yb,DX");
        switch (pVCpu->iem.s.enmEffAddrMode)
        {
            case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_ins_op8_addr16, false);
            case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_ins_op8_addr32, false);
            case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_ins_op8_addr64, false);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        IEMOP_MNEMONIC(ins_Yb_DX, "ins Yb,DX");
        switch (pVCpu->iem.s.enmEffAddrMode)
        {
            case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_ins_op8_addr16, false);
            case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_ins_op8_addr32, false);
            case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_ins_op8_addr64, false);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/**
 * @opcode      0x6d
 */
FNIEMOP_DEF(iemOp_inswd_Yv_DX)
{
    IEMOP_HLP_MIN_186();
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    if (pVCpu->iem.s.fPrefixes & (IEM_OP_PRF_REPZ | IEM_OP_PRF_REPNZ))
    {
        IEMOP_MNEMONIC(rep_ins_Yv_DX, "rep ins Yv,DX");
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_ins_op16_addr16, false);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_ins_op16_addr32, false);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_ins_op16_addr64, false);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
                break;
            case IEMMODE_64BIT:
            case IEMMODE_32BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_ins_op32_addr16, false);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_ins_op32_addr32, false);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_ins_op32_addr64, false);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
                break;
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        IEMOP_MNEMONIC(ins_Yv_DX, "ins Yv,DX");
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_ins_op16_addr16, false);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_ins_op16_addr32, false);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_ins_op16_addr64, false);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
                break;
            case IEMMODE_64BIT:
            case IEMMODE_32BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_ins_op32_addr16, false);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_ins_op32_addr32, false);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_ins_op32_addr64, false);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
                break;
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/**
 * @opcode      0x6e
 */
FNIEMOP_DEF(iemOp_outsb_Yb_DX)
{
    IEMOP_HLP_MIN_186();
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    if (pVCpu->iem.s.fPrefixes & (IEM_OP_PRF_REPNZ | IEM_OP_PRF_REPZ))
    {
        IEMOP_MNEMONIC(rep_outsb_DX_Yb, "rep outs DX,Yb");
        switch (pVCpu->iem.s.enmEffAddrMode)
        {
            case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_rep_outs_op8_addr16, pVCpu->iem.s.iEffSeg, false);
            case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_rep_outs_op8_addr32, pVCpu->iem.s.iEffSeg, false);
            case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_rep_outs_op8_addr64, pVCpu->iem.s.iEffSeg, false);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        IEMOP_MNEMONIC(outs_DX_Yb, "outs DX,Yb");
        switch (pVCpu->iem.s.enmEffAddrMode)
        {
            case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_outs_op8_addr16, pVCpu->iem.s.iEffSeg, false);
            case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_outs_op8_addr32, pVCpu->iem.s.iEffSeg, false);
            case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_outs_op8_addr64, pVCpu->iem.s.iEffSeg, false);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/**
 * @opcode      0x6f
 */
FNIEMOP_DEF(iemOp_outswd_Yv_DX)
{
    IEMOP_HLP_MIN_186();
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    if (pVCpu->iem.s.fPrefixes & (IEM_OP_PRF_REPZ | IEM_OP_PRF_REPNZ))
    {
        IEMOP_MNEMONIC(rep_outs_DX_Yv, "rep outs DX,Yv");
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_rep_outs_op16_addr16, pVCpu->iem.s.iEffSeg, false);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_rep_outs_op16_addr32, pVCpu->iem.s.iEffSeg, false);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_rep_outs_op16_addr64, pVCpu->iem.s.iEffSeg, false);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
                break;
            case IEMMODE_64BIT:
            case IEMMODE_32BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_rep_outs_op32_addr16, pVCpu->iem.s.iEffSeg, false);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_rep_outs_op32_addr32, pVCpu->iem.s.iEffSeg, false);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_rep_outs_op32_addr64, pVCpu->iem.s.iEffSeg, false);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
                break;
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        IEMOP_MNEMONIC(outs_DX_Yv, "outs DX,Yv");
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_outs_op16_addr16, pVCpu->iem.s.iEffSeg, false);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_outs_op16_addr32, pVCpu->iem.s.iEffSeg, false);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_outs_op16_addr64, pVCpu->iem.s.iEffSeg, false);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
                break;
            case IEMMODE_64BIT:
            case IEMMODE_32BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_outs_op32_addr16, pVCpu->iem.s.iEffSeg, false);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_outs_op32_addr32, pVCpu->iem.s.iEffSeg, false);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_outs_op32_addr64, pVCpu->iem.s.iEffSeg, false);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
                break;
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/**
 * @opcode      0x70
 */
FNIEMOP_DEF(iemOp_jo_Jb)
{
    IEMOP_MNEMONIC(jo_Jb, "jo  Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_OF) {
        IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
    } IEM_MC_ELSE() {
        IEM_MC_ADVANCE_RIP_AND_FINISH();
    } IEM_MC_ENDIF();
    IEM_MC_END();
}


/**
 * @opcode      0x71
 */
FNIEMOP_DEF(iemOp_jno_Jb)
{
    IEMOP_MNEMONIC(jno_Jb, "jno Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_OF) {
        IEM_MC_ADVANCE_RIP_AND_FINISH();
    } IEM_MC_ELSE() {
        IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
    } IEM_MC_ENDIF();
    IEM_MC_END();
}

/**
 * @opcode      0x72
 */
FNIEMOP_DEF(iemOp_jc_Jb)
{
    IEMOP_MNEMONIC(jc_Jb, "jc/jnae Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_CF) {
        IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
    } IEM_MC_ELSE() {
        IEM_MC_ADVANCE_RIP_AND_FINISH();
    } IEM_MC_ENDIF();
    IEM_MC_END();
}


/**
 * @opcode      0x73
 */
FNIEMOP_DEF(iemOp_jnc_Jb)
{
    IEMOP_MNEMONIC(jnc_Jb, "jnc/jnb Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_CF) {
        IEM_MC_ADVANCE_RIP_AND_FINISH();
    } IEM_MC_ELSE() {
        IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
    } IEM_MC_ENDIF();
    IEM_MC_END();
}


/**
 * @opcode      0x74
 */
FNIEMOP_DEF(iemOp_je_Jb)
{
    IEMOP_MNEMONIC(je_Jb, "je/jz   Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_ZF) {
        IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
    } IEM_MC_ELSE() {
        IEM_MC_ADVANCE_RIP_AND_FINISH();
    } IEM_MC_ENDIF();
    IEM_MC_END();
}


/**
 * @opcode      0x75
 */
FNIEMOP_DEF(iemOp_jne_Jb)
{
    IEMOP_MNEMONIC(jne_Jb, "jne/jnz Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_ZF) {
        IEM_MC_ADVANCE_RIP_AND_FINISH();
    } IEM_MC_ELSE() {
        IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
    } IEM_MC_ENDIF();
    IEM_MC_END();
}


/**
 * @opcode      0x76
 */
FNIEMOP_DEF(iemOp_jbe_Jb)
{
    IEMOP_MNEMONIC(jbe_Jb, "jbe/jna Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_ANY_BITS_SET(X86_EFL_CF | X86_EFL_ZF) {
        IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
    } IEM_MC_ELSE() {
        IEM_MC_ADVANCE_RIP_AND_FINISH();
    } IEM_MC_ENDIF();
    IEM_MC_END();
}


/**
 * @opcode      0x77
 */
FNIEMOP_DEF(iemOp_jnbe_Jb)
{
    IEMOP_MNEMONIC(ja_Jb, "ja/jnbe Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_ANY_BITS_SET(X86_EFL_CF | X86_EFL_ZF) {
        IEM_MC_ADVANCE_RIP_AND_FINISH();
    } IEM_MC_ELSE() {
        IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
    } IEM_MC_ENDIF();
    IEM_MC_END();
}


/**
 * @opcode      0x78
 */
FNIEMOP_DEF(iemOp_js_Jb)
{
    IEMOP_MNEMONIC(js_Jb, "js  Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_SF) {
        IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
    } IEM_MC_ELSE() {
        IEM_MC_ADVANCE_RIP_AND_FINISH();
    } IEM_MC_ENDIF();
    IEM_MC_END();
}


/**
 * @opcode      0x79
 */
FNIEMOP_DEF(iemOp_jns_Jb)
{
    IEMOP_MNEMONIC(jns_Jb, "jns Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_SF) {
        IEM_MC_ADVANCE_RIP_AND_FINISH();
    } IEM_MC_ELSE() {
        IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
    } IEM_MC_ENDIF();
    IEM_MC_END();
}


/**
 * @opcode      0x7a
 */
FNIEMOP_DEF(iemOp_jp_Jb)
{
    IEMOP_MNEMONIC(jp_Jb, "jp  Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_PF) {
        IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
    } IEM_MC_ELSE() {
        IEM_MC_ADVANCE_RIP_AND_FINISH();
    } IEM_MC_ENDIF();
    IEM_MC_END();
}


/**
 * @opcode      0x7b
 */
FNIEMOP_DEF(iemOp_jnp_Jb)
{
    IEMOP_MNEMONIC(jnp_Jb, "jnp Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_PF) {
        IEM_MC_ADVANCE_RIP_AND_FINISH();
    } IEM_MC_ELSE() {
        IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
    } IEM_MC_ENDIF();
    IEM_MC_END();
}


/**
 * @opcode      0x7c
 */
FNIEMOP_DEF(iemOp_jl_Jb)
{
    IEMOP_MNEMONIC(jl_Jb, "jl/jnge Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BITS_NE(X86_EFL_SF, X86_EFL_OF) {
        IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
    } IEM_MC_ELSE() {
        IEM_MC_ADVANCE_RIP_AND_FINISH();
    } IEM_MC_ENDIF();
    IEM_MC_END();
}


/**
 * @opcode      0x7d
 */
FNIEMOP_DEF(iemOp_jnl_Jb)
{
    IEMOP_MNEMONIC(jge_Jb, "jnl/jge Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BITS_NE(X86_EFL_SF, X86_EFL_OF) {
        IEM_MC_ADVANCE_RIP_AND_FINISH();
    } IEM_MC_ELSE() {
        IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
    } IEM_MC_ENDIF();
    IEM_MC_END();
}


/**
 * @opcode      0x7e
 */
FNIEMOP_DEF(iemOp_jle_Jb)
{
    IEMOP_MNEMONIC(jle_Jb, "jle/jng Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET_OR_BITS_NE(X86_EFL_ZF, X86_EFL_SF, X86_EFL_OF) {
        IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
    } IEM_MC_ELSE() {
        IEM_MC_ADVANCE_RIP_AND_FINISH();
    } IEM_MC_ENDIF();
    IEM_MC_END();
}


/**
 * @opcode      0x7f
 */
FNIEMOP_DEF(iemOp_jnle_Jb)
{
    IEMOP_MNEMONIC(jg_Jb, "jnle/jg Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET_OR_BITS_NE(X86_EFL_ZF, X86_EFL_SF, X86_EFL_OF) {
        IEM_MC_ADVANCE_RIP_AND_FINISH();
    } IEM_MC_ELSE() {
        IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
    } IEM_MC_ENDIF();
    IEM_MC_END();
}


/**
 * @opcode      0x80
 */
FNIEMOP_DEF(iemOp_Grp1_Eb_Ib_80)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    switch (IEM_GET_MODRM_REG_8(bRm))
    {
        case 0: IEMOP_MNEMONIC(add_Eb_Ib, "add Eb,Ib"); break;
        case 1: IEMOP_MNEMONIC(or_Eb_Ib,  "or  Eb,Ib"); break;
        case 2: IEMOP_MNEMONIC(adc_Eb_Ib, "adc Eb,Ib"); break;
        case 3: IEMOP_MNEMONIC(sbb_Eb_Ib, "sbb Eb,Ib"); break;
        case 4: IEMOP_MNEMONIC(and_Eb_Ib, "and Eb,Ib"); break;
        case 5: IEMOP_MNEMONIC(sub_Eb_Ib, "sub Eb,Ib"); break;
        case 6: IEMOP_MNEMONIC(xor_Eb_Ib, "xor Eb,Ib"); break;
        case 7: IEMOP_MNEMONIC(cmp_Eb_Ib, "cmp Eb,Ib"); break;
    }
    PCIEMOPBINSIZES pImpl = g_apIemImplGrp1[IEM_GET_MODRM_REG_8(bRm)];

    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        /* register target */
        uint8_t u8Imm; IEM_OPCODE_GET_NEXT_U8(&u8Imm);
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_BEGIN(3, 0);
        IEM_MC_ARG(uint8_t *,       pu8Dst,                 0);
        IEM_MC_ARG_CONST(uint8_t,   u8Src, /*=*/ u8Imm,     1);
        IEM_MC_ARG(uint32_t *,      pEFlags,                2);

        IEM_MC_REF_GREG_U8(pu8Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, u8Src, pEFlags);

        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
    else
    {
        /* memory target */
        uint32_t fAccess;
        if (pImpl->pfnLockedU8)
            fAccess = IEM_ACCESS_DATA_RW;
        else /* CMP */
            fAccess = IEM_ACCESS_DATA_R;
        IEM_MC_BEGIN(3, 2);
        IEM_MC_ARG(uint8_t *,       pu8Dst,                 0);
        IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,        2);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 1);
        uint8_t u8Imm; IEM_OPCODE_GET_NEXT_U8(&u8Imm);
        IEM_MC_ARG_CONST(uint8_t,   u8Src, /*=*/ u8Imm,     1);
        if (pImpl->pfnLockedU8)
            IEMOP_HLP_DONE_DECODING();
        else
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

        IEM_MC_MEM_MAP(pu8Dst, fAccess, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
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
 * @opcode      0x81
 */
FNIEMOP_DEF(iemOp_Grp1_Ev_Iz)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    switch (IEM_GET_MODRM_REG_8(bRm))
    {
        case 0: IEMOP_MNEMONIC(add_Ev_Iz, "add Ev,Iz"); break;
        case 1: IEMOP_MNEMONIC(or_Ev_Iz,  "or  Ev,Iz"); break;
        case 2: IEMOP_MNEMONIC(adc_Ev_Iz, "adc Ev,Iz"); break;
        case 3: IEMOP_MNEMONIC(sbb_Ev_Iz, "sbb Ev,Iz"); break;
        case 4: IEMOP_MNEMONIC(and_Ev_Iz, "and Ev,Iz"); break;
        case 5: IEMOP_MNEMONIC(sub_Ev_Iz, "sub Ev,Iz"); break;
        case 6: IEMOP_MNEMONIC(xor_Ev_Iz, "xor Ev,Iz"); break;
        case 7: IEMOP_MNEMONIC(cmp_Ev_Iz, "cmp Ev,Iz"); break;
    }
    PCIEMOPBINSIZES pImpl = g_apIemImplGrp1[IEM_GET_MODRM_REG_8(bRm)];

    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
        {
            if (IEM_IS_MODRM_REG_MODE(bRm))
            {
                /* register target */
                uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(&u16Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint16_t *,      pu16Dst,                0);
                IEM_MC_ARG_CONST(uint16_t,  u16Src, /*=*/ u16Imm,   1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                2);

                IEM_MC_REF_GREG_U16(pu16Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
            }
            else
            {
                /* memory target */
                uint32_t fAccess;
                if (pImpl->pfnLockedU16)
                    fAccess = IEM_ACCESS_DATA_RW;
                else /* CMP, TEST */
                    fAccess = IEM_ACCESS_DATA_R;
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint16_t *,      pu16Dst,                0);
                IEM_MC_ARG(uint16_t,        u16Src,                 1);
                IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,        2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 2);
                uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(&u16Imm);
                IEM_MC_ASSIGN(u16Src, u16Imm);
                if (pImpl->pfnLockedU16)
                    IEMOP_HLP_DONE_DECODING();
                else
                    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_MEM_MAP(pu16Dst, fAccess, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_LOCK))
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);
                else
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU16, pu16Dst, u16Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu16Dst, fAccess);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
            }
            break;
        }

        case IEMMODE_32BIT:
        {
            if (IEM_IS_MODRM_REG_MODE(bRm))
            {
                /* register target */
                uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(&u32Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint32_t *,      pu32Dst,                0);
                IEM_MC_ARG_CONST(uint32_t,  u32Src, /*=*/ u32Imm,   1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                2);

                IEM_MC_REF_GREG_U32(pu32Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);
                if (pImpl != &g_iemAImpl_cmp)   /* TEST won't get here, no need to check for it. */
                    IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(pu32Dst);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
            }
            else
            {
                /* memory target */
                uint32_t fAccess;
                if (pImpl->pfnLockedU32)
                    fAccess = IEM_ACCESS_DATA_RW;
                else /* CMP, TEST */
                    fAccess = IEM_ACCESS_DATA_R;
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint32_t *,      pu32Dst,                0);
                IEM_MC_ARG(uint32_t,        u32Src,                 1);
                IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,        2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 4);
                uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(&u32Imm);
                IEM_MC_ASSIGN(u32Src, u32Imm);
                if (pImpl->pfnLockedU32)
                    IEMOP_HLP_DONE_DECODING();
                else
                    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_MEM_MAP(pu32Dst, fAccess, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_LOCK))
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);
                else
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU32, pu32Dst, u32Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu32Dst, fAccess);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
            }
            break;
        }

        case IEMMODE_64BIT:
        {
            if (IEM_IS_MODRM_REG_MODE(bRm))
            {
                /* register target */
                uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S32_SX_U64(&u64Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint64_t *,      pu64Dst,                0);
                IEM_MC_ARG_CONST(uint64_t,  u64Src, /*=*/ u64Imm,   1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                2);

                IEM_MC_REF_GREG_U64(pu64Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
            }
            else
            {
                /* memory target */
                uint32_t fAccess;
                if (pImpl->pfnLockedU64)
                    fAccess = IEM_ACCESS_DATA_RW;
                else /* CMP */
                    fAccess = IEM_ACCESS_DATA_R;
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint64_t *,      pu64Dst,                0);
                IEM_MC_ARG(uint64_t,        u64Src,                 1);
                IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,        2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 4);
                uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S32_SX_U64(&u64Imm);
                if (pImpl->pfnLockedU64)
                    IEMOP_HLP_DONE_DECODING();
                else
                    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_ASSIGN(u64Src, u64Imm);
                IEM_MC_MEM_MAP(pu64Dst, fAccess, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_LOCK))
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);
                else
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU64, pu64Dst, u64Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu64Dst, fAccess);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
            }
            break;
        }

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0x82
 * @opmnemonic  grp1_82
 * @opgroup     og_groups
 */
FNIEMOP_DEF(iemOp_Grp1_Eb_Ib_82)
{
    IEMOP_HLP_NO_64BIT(); /** @todo do we need to decode the whole instruction or is this ok? */
    return FNIEMOP_CALL(iemOp_Grp1_Eb_Ib_80);
}


/**
 * @opcode      0x83
 */
FNIEMOP_DEF(iemOp_Grp1_Ev_Ib)
{
    uint8_t bRm;   IEM_OPCODE_GET_NEXT_U8(&bRm);
    switch (IEM_GET_MODRM_REG_8(bRm))
    {
        case 0: IEMOP_MNEMONIC(add_Ev_Ib, "add Ev,Ib"); break;
        case 1: IEMOP_MNEMONIC(or_Ev_Ib,  "or  Ev,Ib"); break;
        case 2: IEMOP_MNEMONIC(adc_Ev_Ib, "adc Ev,Ib"); break;
        case 3: IEMOP_MNEMONIC(sbb_Ev_Ib, "sbb Ev,Ib"); break;
        case 4: IEMOP_MNEMONIC(and_Ev_Ib, "and Ev,Ib"); break;
        case 5: IEMOP_MNEMONIC(sub_Ev_Ib, "sub Ev,Ib"); break;
        case 6: IEMOP_MNEMONIC(xor_Ev_Ib, "xor Ev,Ib"); break;
        case 7: IEMOP_MNEMONIC(cmp_Ev_Ib, "cmp Ev,Ib"); break;
    }
    /* Note! Seems the OR, AND, and XOR instructions are present on CPUs prior
             to the 386 even if absent in the intel reference manuals and some
             3rd party opcode listings. */
    PCIEMOPBINSIZES pImpl = g_apIemImplGrp1[IEM_GET_MODRM_REG_8(bRm)];

    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        /*
         * Register target
         */
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        uint8_t u8Imm; IEM_OPCODE_GET_NEXT_U8(&u8Imm);
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
            {
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint16_t *,      pu16Dst,                    0);
                IEM_MC_ARG_CONST(uint16_t,  u16Src, /*=*/ (int8_t)u8Imm,1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                    2);

                IEM_MC_REF_GREG_U16(pu16Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;
            }

            case IEMMODE_32BIT:
            {
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint32_t *,      pu32Dst,                    0);
                IEM_MC_ARG_CONST(uint32_t,  u32Src, /*=*/ (int8_t)u8Imm,1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                    2);

                IEM_MC_REF_GREG_U32(pu32Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);
                if (pImpl != &g_iemAImpl_cmp)   /* TEST won't get here, no need to check for it. */
                    IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(pu32Dst);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;
            }

            case IEMMODE_64BIT:
            {
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint64_t *,      pu64Dst,                    0);
                IEM_MC_ARG_CONST(uint64_t,  u64Src, /*=*/ (int8_t)u8Imm,1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                    2);

                IEM_MC_REF_GREG_U64(pu64Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;
            }

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /*
         * Memory target.
         */
        uint32_t fAccess;
        if (pImpl->pfnLockedU16)
            fAccess = IEM_ACCESS_DATA_RW;
        else /* CMP */
            fAccess = IEM_ACCESS_DATA_R;

        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
            {
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint16_t *,      pu16Dst,                    0);
                IEM_MC_ARG(uint16_t,        u16Src,                     1);
                IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,            2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 1);
                uint8_t u8Imm; IEM_OPCODE_GET_NEXT_U8(&u8Imm);
                IEM_MC_ASSIGN(u16Src, (int8_t)u8Imm);
                if (pImpl->pfnLockedU16)
                    IEMOP_HLP_DONE_DECODING();
                else
                    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_MEM_MAP(pu16Dst, fAccess, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
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
            }

            case IEMMODE_32BIT:
            {
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint32_t *,      pu32Dst,                    0);
                IEM_MC_ARG(uint32_t,        u32Src,                     1);
                IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,            2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 1);
                uint8_t u8Imm; IEM_OPCODE_GET_NEXT_U8(&u8Imm);
                IEM_MC_ASSIGN(u32Src, (int8_t)u8Imm);
                if (pImpl->pfnLockedU32)
                    IEMOP_HLP_DONE_DECODING();
                else
                    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_MEM_MAP(pu32Dst, fAccess, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
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
            }

            case IEMMODE_64BIT:
            {
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint64_t *,      pu64Dst,                    0);
                IEM_MC_ARG(uint64_t,        u64Src,                     1);
                IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,            2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 1);
                uint8_t u8Imm; IEM_OPCODE_GET_NEXT_U8(&u8Imm);
                IEM_MC_ASSIGN(u64Src, (int8_t)u8Imm);
                if (pImpl->pfnLockedU64)
                    IEMOP_HLP_DONE_DECODING();
                else
                    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_MEM_MAP(pu64Dst, fAccess, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
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
            }

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/**
 * @opcode      0x84
 */
FNIEMOP_DEF(iemOp_test_Eb_Gb)
{
    IEMOP_MNEMONIC(test_Eb_Gb, "test Eb,Gb");
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_r8, &g_iemAImpl_test);
}


/**
 * @opcode      0x85
 */
FNIEMOP_DEF(iemOp_test_Ev_Gv)
{
    IEMOP_MNEMONIC(test_Ev_Gv, "test Ev,Gv");
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_rv, &g_iemAImpl_test);
}


/**
 * @opcode      0x86
 */
FNIEMOP_DEF(iemOp_xchg_Eb_Gb)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    IEMOP_MNEMONIC(xchg_Eb_Gb, "xchg Eb,Gb");

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 2);
        IEM_MC_LOCAL(uint8_t, uTmp1);
        IEM_MC_LOCAL(uint8_t, uTmp2);

        IEM_MC_FETCH_GREG_U8(uTmp1, IEM_GET_MODRM_REG(pVCpu, bRm));
        IEM_MC_FETCH_GREG_U8(uTmp2, IEM_GET_MODRM_RM(pVCpu, bRm));
        IEM_MC_STORE_GREG_U8(IEM_GET_MODRM_RM(pVCpu, bRm),                              uTmp1);
        IEM_MC_STORE_GREG_U8(IEM_GET_MODRM_REG(pVCpu, bRm), uTmp2);

        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
    else
    {
        /*
         * We're accessing memory.
         */
/** @todo the register must be committed separately! */
        IEM_MC_BEGIN(2, 2);
        IEM_MC_ARG(uint8_t *,  pu8Mem,           0);
        IEM_MC_ARG(uint8_t *,  pu8Reg,           1);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
        IEM_MC_MEM_MAP(pu8Mem, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
        IEM_MC_REF_GREG_U8(pu8Reg, IEM_GET_MODRM_REG(pVCpu, bRm));
        if (!pVCpu->iem.s.fDisregardLock)
            IEM_MC_CALL_VOID_AIMPL_2(iemAImpl_xchg_u8_locked, pu8Mem, pu8Reg);
        else
            IEM_MC_CALL_VOID_AIMPL_2(iemAImpl_xchg_u8_unlocked, pu8Mem, pu8Reg);
        IEM_MC_MEM_COMMIT_AND_UNMAP(pu8Mem, IEM_ACCESS_DATA_RW);

        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
}


/**
 * @opcode      0x87
 */
FNIEMOP_DEF(iemOp_xchg_Ev_Gv)
{
    IEMOP_MNEMONIC(xchg_Ev_Gv, "xchg Ev,Gv");
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
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint16_t, uTmp1);
                IEM_MC_LOCAL(uint16_t, uTmp2);

                IEM_MC_FETCH_GREG_U16(uTmp1, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_FETCH_GREG_U16(uTmp2, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_STORE_GREG_U16(IEM_GET_MODRM_RM(pVCpu, bRm),                              uTmp1);
                IEM_MC_STORE_GREG_U16(IEM_GET_MODRM_REG(pVCpu, bRm), uTmp2);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                return VINF_SUCCESS;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint32_t, uTmp1);
                IEM_MC_LOCAL(uint32_t, uTmp2);

                IEM_MC_FETCH_GREG_U32(uTmp1, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_FETCH_GREG_U32(uTmp2, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_STORE_GREG_U32(IEM_GET_MODRM_RM(pVCpu, bRm),                              uTmp1);
                IEM_MC_STORE_GREG_U32(IEM_GET_MODRM_REG(pVCpu, bRm), uTmp2);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                return VINF_SUCCESS;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint64_t, uTmp1);
                IEM_MC_LOCAL(uint64_t, uTmp2);

                IEM_MC_FETCH_GREG_U64(uTmp1, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_FETCH_GREG_U64(uTmp2, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_STORE_GREG_U64(IEM_GET_MODRM_RM(pVCpu, bRm),                              uTmp1);
                IEM_MC_STORE_GREG_U64(IEM_GET_MODRM_REG(pVCpu, bRm), uTmp2);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                return VINF_SUCCESS;

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
/** @todo the register must be committed separately! */
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(2, 2);
                IEM_MC_ARG(uint16_t *,  pu16Mem, 0);
                IEM_MC_ARG(uint16_t *,  pu16Reg, 1);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEM_MC_MEM_MAP(pu16Mem, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_REF_GREG_U16(pu16Reg, IEM_GET_MODRM_REG(pVCpu, bRm));
                if (!pVCpu->iem.s.fDisregardLock)
                    IEM_MC_CALL_VOID_AIMPL_2(iemAImpl_xchg_u16_locked, pu16Mem, pu16Reg);
                else
                    IEM_MC_CALL_VOID_AIMPL_2(iemAImpl_xchg_u16_unlocked, pu16Mem, pu16Reg);
                IEM_MC_MEM_COMMIT_AND_UNMAP(pu16Mem, IEM_ACCESS_DATA_RW);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                return VINF_SUCCESS;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(2, 2);
                IEM_MC_ARG(uint32_t *,  pu32Mem, 0);
                IEM_MC_ARG(uint32_t *,  pu32Reg, 1);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEM_MC_MEM_MAP(pu32Mem, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_REF_GREG_U32(pu32Reg, IEM_GET_MODRM_REG(pVCpu, bRm));
                if (!pVCpu->iem.s.fDisregardLock)
                    IEM_MC_CALL_VOID_AIMPL_2(iemAImpl_xchg_u32_locked, pu32Mem, pu32Reg);
                else
                    IEM_MC_CALL_VOID_AIMPL_2(iemAImpl_xchg_u32_unlocked, pu32Mem, pu32Reg);
                IEM_MC_MEM_COMMIT_AND_UNMAP(pu32Mem, IEM_ACCESS_DATA_RW);

                IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(pu32Reg);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                return VINF_SUCCESS;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(2, 2);
                IEM_MC_ARG(uint64_t *,  pu64Mem, 0);
                IEM_MC_ARG(uint64_t *,  pu64Reg, 1);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEM_MC_MEM_MAP(pu64Mem, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_REF_GREG_U64(pu64Reg, IEM_GET_MODRM_REG(pVCpu, bRm));
                if (!pVCpu->iem.s.fDisregardLock)
                    IEM_MC_CALL_VOID_AIMPL_2(iemAImpl_xchg_u64_locked, pu64Mem, pu64Reg);
                else
                    IEM_MC_CALL_VOID_AIMPL_2(iemAImpl_xchg_u64_unlocked, pu64Mem, pu64Reg);
                IEM_MC_MEM_COMMIT_AND_UNMAP(pu64Mem, IEM_ACCESS_DATA_RW);

                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                return VINF_SUCCESS;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/**
 * @opcode      0x88
 */
FNIEMOP_DEF(iemOp_mov_Eb_Gb)
{
    IEMOP_MNEMONIC(mov_Eb_Gb, "mov Eb,Gb");

    uint8_t bRm;
    IEM_OPCODE_GET_NEXT_U8(&bRm);

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_BEGIN(0, 1);
        IEM_MC_LOCAL(uint8_t, u8Value);
        IEM_MC_FETCH_GREG_U8(u8Value, IEM_GET_MODRM_REG(pVCpu, bRm));
        IEM_MC_STORE_GREG_U8(IEM_GET_MODRM_RM(pVCpu, bRm), u8Value);
        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
    else
    {
        /*
         * We're writing a register to memory.
         */
        IEM_MC_BEGIN(0, 2);
        IEM_MC_LOCAL(uint8_t, u8Value);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_FETCH_GREG_U8(u8Value, IEM_GET_MODRM_REG(pVCpu, bRm));
        IEM_MC_STORE_MEM_U8(pVCpu->iem.s.iEffSeg, GCPtrEffDst, u8Value);
        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
}


/**
 * @opcode      0x89
 */
FNIEMOP_DEF(iemOp_mov_Ev_Gv)
{
    IEMOP_MNEMONIC(mov_Ev_Gv, "mov Ev,Gv");

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
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint16_t, u16Value);
                IEM_MC_FETCH_GREG_U16(u16Value, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_STORE_GREG_U16(IEM_GET_MODRM_RM(pVCpu, bRm), u16Value);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint32_t, u32Value);
                IEM_MC_FETCH_GREG_U32(u32Value, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_STORE_GREG_U32(IEM_GET_MODRM_RM(pVCpu, bRm), u32Value);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint64_t, u64Value);
                IEM_MC_FETCH_GREG_U64(u64Value, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_STORE_GREG_U64(IEM_GET_MODRM_RM(pVCpu, bRm), u64Value);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /*
         * We're writing a register to memory.
         */
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint16_t, u16Value);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_GREG_U16(u16Value, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_STORE_MEM_U16(pVCpu->iem.s.iEffSeg, GCPtrEffDst, u16Value);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint32_t, u32Value);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_GREG_U32(u32Value, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_STORE_MEM_U32(pVCpu->iem.s.iEffSeg, GCPtrEffDst, u32Value);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint64_t, u64Value);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_GREG_U64(u64Value, IEM_GET_MODRM_REG(pVCpu, bRm));
                IEM_MC_STORE_MEM_U64(pVCpu->iem.s.iEffSeg, GCPtrEffDst, u64Value);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/**
 * @opcode      0x8a
 */
FNIEMOP_DEF(iemOp_mov_Gb_Eb)
{
    IEMOP_MNEMONIC(mov_Gb_Eb, "mov Gb,Eb");

    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_BEGIN(0, 1);
        IEM_MC_LOCAL(uint8_t, u8Value);
        IEM_MC_FETCH_GREG_U8(u8Value, IEM_GET_MODRM_RM(pVCpu, bRm));
        IEM_MC_STORE_GREG_U8(IEM_GET_MODRM_REG(pVCpu, bRm), u8Value);
        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
    else
    {
        /*
         * We're loading a register from memory.
         */
        IEM_MC_BEGIN(0, 2);
        IEM_MC_LOCAL(uint8_t, u8Value);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_FETCH_MEM_U8(u8Value, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
        IEM_MC_STORE_GREG_U8(IEM_GET_MODRM_REG(pVCpu, bRm), u8Value);
        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
}


/**
 * @opcode      0x8b
 */
FNIEMOP_DEF(iemOp_mov_Gv_Ev)
{
    IEMOP_MNEMONIC(mov_Gv_Ev, "mov Gv,Ev");

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
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint16_t, u16Value);
                IEM_MC_FETCH_GREG_U16(u16Value, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_STORE_GREG_U16(IEM_GET_MODRM_REG(pVCpu, bRm), u16Value);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint32_t, u32Value);
                IEM_MC_FETCH_GREG_U32(u32Value, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_STORE_GREG_U32(IEM_GET_MODRM_REG(pVCpu, bRm), u32Value);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint64_t, u64Value);
                IEM_MC_FETCH_GREG_U64(u64Value, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_STORE_GREG_U64(IEM_GET_MODRM_REG(pVCpu, bRm), u64Value);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /*
         * We're loading a register from memory.
         */
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint16_t, u16Value);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U16(u16Value, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
                IEM_MC_STORE_GREG_U16(IEM_GET_MODRM_REG(pVCpu, bRm), u16Value);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint32_t, u32Value);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U32(u32Value, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
                IEM_MC_STORE_GREG_U32(IEM_GET_MODRM_REG(pVCpu, bRm), u32Value);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint64_t, u64Value);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U64(u64Value, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
                IEM_MC_STORE_GREG_U64(IEM_GET_MODRM_REG(pVCpu, bRm), u64Value);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/**
 * opcode      0x63
 * @todo Table fixme
 */
FNIEMOP_DEF(iemOp_arpl_Ew_Gw_movsx_Gv_Ev)
{
    if (pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT)
        return FNIEMOP_CALL(iemOp_arpl_Ew_Gw);
    if (pVCpu->iem.s.enmEffOpSize != IEMMODE_64BIT)
        return FNIEMOP_CALL(iemOp_mov_Gv_Ev);
    return FNIEMOP_CALL(iemOp_movsxd_Gv_Ev);
}


/**
 * @opcode      0x8c
 */
FNIEMOP_DEF(iemOp_mov_Ev_Sw)
{
    IEMOP_MNEMONIC(mov_Ev_Sw, "mov Ev,Sw");

    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);

    /*
     * Check that the destination register exists. The REX.R prefix is ignored.
     */
    uint8_t const iSegReg = IEM_GET_MODRM_REG_8(bRm);
    if (   iSegReg > X86_SREG_GS)
        return IEMOP_RAISE_INVALID_OPCODE(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */

    /*
     * If rm is denoting a register, no more instruction bytes.
     * In that case, the operand size is respected and the upper bits are
     * cleared (starting with some pentium).
     */
    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint16_t, u16Value);
                IEM_MC_FETCH_SREG_U16(u16Value, iSegReg);
                IEM_MC_STORE_GREG_U16(IEM_GET_MODRM_RM(pVCpu, bRm), u16Value);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint32_t, u32Value);
                IEM_MC_FETCH_SREG_ZX_U32(u32Value, iSegReg);
                IEM_MC_STORE_GREG_U32(IEM_GET_MODRM_RM(pVCpu, bRm), u32Value);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint64_t, u64Value);
                IEM_MC_FETCH_SREG_ZX_U64(u64Value, iSegReg);
                IEM_MC_STORE_GREG_U64(IEM_GET_MODRM_RM(pVCpu, bRm), u64Value);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /*
         * We're saving the register to memory.  The access is word sized
         * regardless of operand size prefixes.
         */
#if 0 /* not necessary */
        pVCpu->iem.s.enmEffOpSize = pVCpu->iem.s.enmDefOpSize = IEMMODE_16BIT;
#endif
        IEM_MC_BEGIN(0, 2);
        IEM_MC_LOCAL(uint16_t,  u16Value);
        IEM_MC_LOCAL(RTGCPTR,   GCPtrEffDst);
        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_FETCH_SREG_U16(u16Value, iSegReg);
        IEM_MC_STORE_MEM_U16(pVCpu->iem.s.iEffSeg, GCPtrEffDst, u16Value);
        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
}




/**
 * @opcode      0x8d
 */
FNIEMOP_DEF(iemOp_lea_Gv_M)
{
    IEMOP_MNEMONIC(lea_Gv_M, "lea Gv,M");
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    if (IEM_IS_MODRM_REG_MODE(bRm))
        return IEMOP_RAISE_INVALID_OPCODE(); /* no register form */

    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0, 2);
            IEM_MC_LOCAL(RTGCPTR,  GCPtrEffSrc);
            IEM_MC_LOCAL(uint16_t, u16Cast);
            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
            IEM_MC_ASSIGN_TO_SMALLER(u16Cast, GCPtrEffSrc);
            IEM_MC_STORE_GREG_U16(IEM_GET_MODRM_REG(pVCpu, bRm), u16Cast);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0, 2);
            IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
            IEM_MC_LOCAL(uint32_t, u32Cast);
            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
            IEM_MC_ASSIGN_TO_SMALLER(u32Cast, GCPtrEffSrc);
            IEM_MC_STORE_GREG_U32(IEM_GET_MODRM_REG(pVCpu, bRm), u32Cast);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
            IEM_MC_STORE_GREG_U64(IEM_GET_MODRM_REG(pVCpu, bRm), GCPtrEffSrc);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0x8e
 */
FNIEMOP_DEF(iemOp_mov_Sw_Ev)
{
    IEMOP_MNEMONIC(mov_Sw_Ev, "mov Sw,Ev");

    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);

    /*
     * The practical operand size is 16-bit.
     */
#if 0 /* not necessary */
    pVCpu->iem.s.enmEffOpSize = pVCpu->iem.s.enmDefOpSize = IEMMODE_16BIT;
#endif

    /*
     * Check that the destination register exists and can be used with this
     * instruction.  The REX.R prefix is ignored.
     */
    uint8_t const iSegReg = IEM_GET_MODRM_REG_8(bRm);
    if (   iSegReg == X86_SREG_CS
        || iSegReg > X86_SREG_GS)
        return IEMOP_RAISE_INVALID_OPCODE(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_BEGIN(2, 0);
        IEM_MC_ARG_CONST(uint8_t, iSRegArg, iSegReg, 0);
        IEM_MC_ARG(uint16_t,      u16Value,          1);
        IEM_MC_FETCH_GREG_U16(u16Value, IEM_GET_MODRM_RM(pVCpu, bRm));
        IEM_MC_CALL_CIMPL_2(iemCImpl_load_SReg, iSRegArg, u16Value);
        IEM_MC_END();
    }
    else
    {
        /*
         * We're loading the register from memory.  The access is word sized
         * regardless of operand size prefixes.
         */
        IEM_MC_BEGIN(2, 1);
        IEM_MC_ARG_CONST(uint8_t, iSRegArg, iSegReg, 0);
        IEM_MC_ARG(uint16_t,      u16Value,          1);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_FETCH_MEM_U16(u16Value, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
        IEM_MC_CALL_CIMPL_2(iemCImpl_load_SReg, iSRegArg, u16Value);
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0x8f /0. */
FNIEMOP_DEF_1(iemOp_pop_Ev, uint8_t, bRm)
{
    /* This bugger is rather annoying as it requires rSP to be updated before
       doing the effective address calculations.  Will eventually require a
       split between the R/M+SIB decoding and the effective address
       calculation - which is something that is required for any attempt at
       reusing this code for a recompiler.  It may also be good to have if we
       need to delay #UD exception caused by invalid lock prefixes.

       For now, we'll do a mostly safe interpreter-only implementation here. */
    /** @todo What's the deal with the 'reg' field and pop Ev?  Ignorning it for
     *        now until tests show it's checked.. */
    IEMOP_MNEMONIC(pop_Ev, "pop Ev");

    /* Register access is relatively easy and can share code. */
    if (IEM_IS_MODRM_REG_MODE(bRm))
        return FNIEMOP_CALL_1(iemOpCommonPopGReg, IEM_GET_MODRM_RM(pVCpu, bRm));

    /*
     * Memory target.
     *
     * Intel says that RSP is incremented before it's used in any effective
     * address calcuations.  This means some serious extra annoyance here since
     * we decode and calculate the effective address in one step and like to
     * delay committing registers till everything is done.
     *
     * So, we'll decode and calculate the effective address twice.  This will
     * require some recoding if turned into a recompiler.
     */
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE(); /* The common code does this differently. */

#ifndef TST_IEM_CHECK_MC
    /* Calc effective address with modified ESP. */
/** @todo testcase */
    RTGCPTR         GCPtrEff;
    VBOXSTRICTRC    rcStrict;
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT: rcStrict = iemOpHlpCalcRmEffAddrEx(pVCpu, bRm, 0, &GCPtrEff, 2); break;
        case IEMMODE_32BIT: rcStrict = iemOpHlpCalcRmEffAddrEx(pVCpu, bRm, 0, &GCPtrEff, 4); break;
        case IEMMODE_64BIT: rcStrict = iemOpHlpCalcRmEffAddrEx(pVCpu, bRm, 0, &GCPtrEff, 8); break;
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    /* Perform the operation - this should be CImpl. */
    RTUINT64U TmpRsp;
    TmpRsp.u = pVCpu->cpum.GstCtx.rsp;
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
        {
            uint16_t u16Value;
            rcStrict = iemMemStackPopU16Ex(pVCpu, &u16Value, &TmpRsp);
            if (rcStrict == VINF_SUCCESS)
                rcStrict = iemMemStoreDataU16(pVCpu, pVCpu->iem.s.iEffSeg, GCPtrEff, u16Value);
            break;
        }

        case IEMMODE_32BIT:
        {
            uint32_t u32Value;
            rcStrict = iemMemStackPopU32Ex(pVCpu, &u32Value, &TmpRsp);
            if (rcStrict == VINF_SUCCESS)
                rcStrict = iemMemStoreDataU32(pVCpu, pVCpu->iem.s.iEffSeg, GCPtrEff, u32Value);
            break;
        }

        case IEMMODE_64BIT:
        {
            uint64_t u64Value;
            rcStrict = iemMemStackPopU64Ex(pVCpu, &u64Value, &TmpRsp);
            if (rcStrict == VINF_SUCCESS)
                rcStrict = iemMemStoreDataU64(pVCpu, pVCpu->iem.s.iEffSeg, GCPtrEff, u64Value);
            break;
        }

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
    if (rcStrict == VINF_SUCCESS)
    {
        pVCpu->cpum.GstCtx.rsp = TmpRsp.u;
        return iemRegUpdateRipAndFinishClearingRF(pVCpu);
    }
    return rcStrict;

#else
    return VERR_IEM_IPE_2;
#endif
}


/**
 * @opcode      0x8f
 */
FNIEMOP_DEF(iemOp_Grp1A__xop)
{
    /*
     * AMD has defined /1 thru /7 as XOP prefix.  The prefix is similar to the
     * three byte VEX prefix, except that the mmmmm field cannot have the values
     * 0 thru 7, because it would then be confused with pop Ev (modrm.reg == 0).
     */
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    if ((bRm & X86_MODRM_REG_MASK) == (0 << X86_MODRM_REG_SHIFT)) /* /0 */
        return FNIEMOP_CALL_1(iemOp_pop_Ev, bRm);

    IEMOP_MNEMONIC(xop, "xop");
    if (IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fXop)
    {
        /** @todo Test when exctly the XOP conformance checks kick in during
         * instruction decoding and fetching (using \#PF). */
        uint8_t bXop2;   IEM_OPCODE_GET_NEXT_U8(&bXop2);
        uint8_t bOpcode; IEM_OPCODE_GET_NEXT_U8(&bOpcode);
        if (   (  pVCpu->iem.s.fPrefixes
                & (IEM_OP_PRF_SIZE_OP | IEM_OP_PRF_REPZ | IEM_OP_PRF_REPNZ | IEM_OP_PRF_LOCK | IEM_OP_PRF_REX))
            == 0)
        {
            pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_XOP;
            if ((bXop2 & 0x80 /* XOP.W */) && pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
                pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_SIZE_REX_W;
            pVCpu->iem.s.uRexReg    = (~bRm >> (7 - 3)) & 0x8;
            pVCpu->iem.s.uRexIndex  = (~bRm >> (6 - 3)) & 0x8;
            pVCpu->iem.s.uRexB      = (~bRm >> (5 - 3)) & 0x8;
            pVCpu->iem.s.uVex3rdReg = (~bXop2 >> 3) & 0xf;
            pVCpu->iem.s.uVexLength = (bXop2 >> 2) & 1;
            pVCpu->iem.s.idxPrefix  = bXop2 & 0x3;

            /** @todo XOP: Just use new tables and decoders. */
            switch (bRm & 0x1f)
            {
                case 8: /* xop opcode map 8. */
                    IEMOP_BITCH_ABOUT_STUB();
                    return VERR_IEM_INSTR_NOT_IMPLEMENTED;

                case 9: /* xop opcode map 9. */
                    IEMOP_BITCH_ABOUT_STUB();
                    return VERR_IEM_INSTR_NOT_IMPLEMENTED;

                case 10: /* xop opcode map 10. */
                    IEMOP_BITCH_ABOUT_STUB();
                    return VERR_IEM_INSTR_NOT_IMPLEMENTED;

                default:
                    Log(("XOP: Invalid vvvv value: %#x!\n", bRm & 0x1f));
                    return IEMOP_RAISE_INVALID_OPCODE();
            }
        }
        else
            Log(("XOP: Invalid prefix mix!\n"));
    }
    else
        Log(("XOP: XOP support disabled!\n"));
    return IEMOP_RAISE_INVALID_OPCODE();
}


/**
 * Common 'xchg reg,rAX' helper.
 */
FNIEMOP_DEF_1(iemOpCommonXchgGRegRax, uint8_t, iReg)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    iReg |= pVCpu->iem.s.uRexB;
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0, 2);
            IEM_MC_LOCAL(uint16_t, u16Tmp1);
            IEM_MC_LOCAL(uint16_t, u16Tmp2);
            IEM_MC_FETCH_GREG_U16(u16Tmp1, iReg);
            IEM_MC_FETCH_GREG_U16(u16Tmp2, X86_GREG_xAX);
            IEM_MC_STORE_GREG_U16(X86_GREG_xAX, u16Tmp1);
            IEM_MC_STORE_GREG_U16(iReg,         u16Tmp2);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0, 2);
            IEM_MC_LOCAL(uint32_t, u32Tmp1);
            IEM_MC_LOCAL(uint32_t, u32Tmp2);
            IEM_MC_FETCH_GREG_U32(u32Tmp1, iReg);
            IEM_MC_FETCH_GREG_U32(u32Tmp2, X86_GREG_xAX);
            IEM_MC_STORE_GREG_U32(X86_GREG_xAX, u32Tmp1);
            IEM_MC_STORE_GREG_U32(iReg,         u32Tmp2);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0, 2);
            IEM_MC_LOCAL(uint64_t, u64Tmp1);
            IEM_MC_LOCAL(uint64_t, u64Tmp2);
            IEM_MC_FETCH_GREG_U64(u64Tmp1, iReg);
            IEM_MC_FETCH_GREG_U64(u64Tmp2, X86_GREG_xAX);
            IEM_MC_STORE_GREG_U64(X86_GREG_xAX, u64Tmp1);
            IEM_MC_STORE_GREG_U64(iReg,         u64Tmp2);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0x90
 */
FNIEMOP_DEF(iemOp_nop)
{
    /* R8/R8D and RAX/EAX can be exchanged. */
    if (pVCpu->iem.s.fPrefixes & IEM_OP_PRF_REX_B)
    {
        IEMOP_MNEMONIC(xchg_r8_rAX, "xchg r8,rAX");
        return FNIEMOP_CALL_1(iemOpCommonXchgGRegRax, X86_GREG_xAX);
    }

    if (pVCpu->iem.s.fPrefixes & IEM_OP_PRF_LOCK)
    {
        IEMOP_MNEMONIC(pause, "pause");
#ifdef VBOX_WITH_NESTED_HWVIRT_VMX
        if (IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fVmx)
            return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_vmx_pause);
#endif
#ifdef VBOX_WITH_NESTED_HWVIRT_SVM
        if (IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fSvm)
            return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_svm_pause);
#endif
    }
    else
        IEMOP_MNEMONIC(nop, "nop");
    IEM_MC_BEGIN(0, 0);
    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/**
 * @opcode      0x91
 */
FNIEMOP_DEF(iemOp_xchg_eCX_eAX)
{
    IEMOP_MNEMONIC(xchg_rCX_rAX, "xchg rCX,rAX");
    return FNIEMOP_CALL_1(iemOpCommonXchgGRegRax, X86_GREG_xCX);
}


/**
 * @opcode      0x92
 */
FNIEMOP_DEF(iemOp_xchg_eDX_eAX)
{
    IEMOP_MNEMONIC(xchg_rDX_rAX, "xchg rDX,rAX");
    return FNIEMOP_CALL_1(iemOpCommonXchgGRegRax, X86_GREG_xDX);
}


/**
 * @opcode      0x93
 */
FNIEMOP_DEF(iemOp_xchg_eBX_eAX)
{
    IEMOP_MNEMONIC(xchg_rBX_rAX, "xchg rBX,rAX");
    return FNIEMOP_CALL_1(iemOpCommonXchgGRegRax, X86_GREG_xBX);
}


/**
 * @opcode      0x94
 */
FNIEMOP_DEF(iemOp_xchg_eSP_eAX)
{
    IEMOP_MNEMONIC(xchg_rSX_rAX, "xchg rSX,rAX");
    return FNIEMOP_CALL_1(iemOpCommonXchgGRegRax, X86_GREG_xSP);
}


/**
 * @opcode      0x95
 */
FNIEMOP_DEF(iemOp_xchg_eBP_eAX)
{
    IEMOP_MNEMONIC(xchg_rBP_rAX, "xchg rBP,rAX");
    return FNIEMOP_CALL_1(iemOpCommonXchgGRegRax, X86_GREG_xBP);
}


/**
 * @opcode      0x96
 */
FNIEMOP_DEF(iemOp_xchg_eSI_eAX)
{
    IEMOP_MNEMONIC(xchg_rSI_rAX, "xchg rSI,rAX");
    return FNIEMOP_CALL_1(iemOpCommonXchgGRegRax, X86_GREG_xSI);
}


/**
 * @opcode      0x97
 */
FNIEMOP_DEF(iemOp_xchg_eDI_eAX)
{
    IEMOP_MNEMONIC(xchg_rDI_rAX, "xchg rDI,rAX");
    return FNIEMOP_CALL_1(iemOpCommonXchgGRegRax, X86_GREG_xDI);
}


/**
 * @opcode      0x98
 */
FNIEMOP_DEF(iemOp_cbw)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEMOP_MNEMONIC(cbw, "cbw");
            IEM_MC_BEGIN(0, 1);
            IEM_MC_IF_GREG_BIT_SET(X86_GREG_xAX, 7) {
                IEM_MC_OR_GREG_U16(X86_GREG_xAX, UINT16_C(0xff00));
            } IEM_MC_ELSE() {
                IEM_MC_AND_GREG_U16(X86_GREG_xAX, UINT16_C(0x00ff));
            } IEM_MC_ENDIF();
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_32BIT:
            IEMOP_MNEMONIC(cwde, "cwde");
            IEM_MC_BEGIN(0, 1);
            IEM_MC_IF_GREG_BIT_SET(X86_GREG_xAX, 15) {
                IEM_MC_OR_GREG_U32(X86_GREG_xAX, UINT32_C(0xffff0000));
            } IEM_MC_ELSE() {
                IEM_MC_AND_GREG_U32(X86_GREG_xAX, UINT32_C(0x0000ffff));
            } IEM_MC_ENDIF();
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_64BIT:
            IEMOP_MNEMONIC(cdqe, "cdqe");
            IEM_MC_BEGIN(0, 1);
            IEM_MC_IF_GREG_BIT_SET(X86_GREG_xAX, 31) {
                IEM_MC_OR_GREG_U64(X86_GREG_xAX, UINT64_C(0xffffffff00000000));
            } IEM_MC_ELSE() {
                IEM_MC_AND_GREG_U64(X86_GREG_xAX, UINT64_C(0x00000000ffffffff));
            } IEM_MC_ENDIF();
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0x99
 */
FNIEMOP_DEF(iemOp_cwd)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEMOP_MNEMONIC(cwd, "cwd");
            IEM_MC_BEGIN(0, 1);
            IEM_MC_IF_GREG_BIT_SET(X86_GREG_xAX, 15) {
                IEM_MC_STORE_GREG_U16_CONST(X86_GREG_xDX, UINT16_C(0xffff));
            } IEM_MC_ELSE() {
                IEM_MC_STORE_GREG_U16_CONST(X86_GREG_xDX, 0);
            } IEM_MC_ENDIF();
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_32BIT:
            IEMOP_MNEMONIC(cdq, "cdq");
            IEM_MC_BEGIN(0, 1);
            IEM_MC_IF_GREG_BIT_SET(X86_GREG_xAX, 31) {
                IEM_MC_STORE_GREG_U32_CONST(X86_GREG_xDX, UINT32_C(0xffffffff));
            } IEM_MC_ELSE() {
                IEM_MC_STORE_GREG_U32_CONST(X86_GREG_xDX, 0);
            } IEM_MC_ENDIF();
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_64BIT:
            IEMOP_MNEMONIC(cqo, "cqo");
            IEM_MC_BEGIN(0, 1);
            IEM_MC_IF_GREG_BIT_SET(X86_GREG_xAX, 63) {
                IEM_MC_STORE_GREG_U64_CONST(X86_GREG_xDX, UINT64_C(0xffffffffffffffff));
            } IEM_MC_ELSE() {
                IEM_MC_STORE_GREG_U64_CONST(X86_GREG_xDX, 0);
            } IEM_MC_ENDIF();
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0x9a
 */
FNIEMOP_DEF(iemOp_call_Ap)
{
    IEMOP_MNEMONIC(call_Ap, "call Ap");
    IEMOP_HLP_NO_64BIT();

    /* Decode the far pointer address and pass it on to the far call C implementation. */
    uint32_t offSeg;
    if (pVCpu->iem.s.enmEffOpSize != IEMMODE_16BIT)
        IEM_OPCODE_GET_NEXT_U32(&offSeg);
    else
        IEM_OPCODE_GET_NEXT_U16_ZX_U32(&offSeg);
    uint16_t uSel;  IEM_OPCODE_GET_NEXT_U16(&uSel);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_3(iemCImpl_callf, uSel, offSeg, pVCpu->iem.s.enmEffOpSize);
}


/** Opcode 0x9b. (aka fwait) */
FNIEMOP_DEF(iemOp_wait)
{
    IEMOP_MNEMONIC(wait, "wait");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_MAYBE_RAISE_WAIT_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/**
 * @opcode      0x9c
 */
FNIEMOP_DEF(iemOp_pushf_Fv)
{
    IEMOP_MNEMONIC(pushf_Fv, "pushf Fv");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_pushf, pVCpu->iem.s.enmEffOpSize);
}


/**
 * @opcode      0x9d
 */
FNIEMOP_DEF(iemOp_popf_Fv)
{
    IEMOP_MNEMONIC(popf_Fv, "popf Fv");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_popf, pVCpu->iem.s.enmEffOpSize);
}


/**
 * @opcode      0x9e
 */
FNIEMOP_DEF(iemOp_sahf)
{
    IEMOP_MNEMONIC(sahf, "sahf");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    if (   pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT
        && !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fLahfSahf)
        return IEMOP_RAISE_INVALID_OPCODE();
    IEM_MC_BEGIN(0, 2);
    IEM_MC_LOCAL(uint32_t, u32Flags);
    IEM_MC_LOCAL(uint32_t, EFlags);
    IEM_MC_FETCH_EFLAGS(EFlags);
    IEM_MC_FETCH_GREG_U8_ZX_U32(u32Flags, X86_GREG_xSP/*=AH*/);
    IEM_MC_AND_LOCAL_U32(u32Flags, X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF);
    IEM_MC_AND_LOCAL_U32(EFlags, UINT32_C(0xffffff00));
    IEM_MC_OR_LOCAL_U32(u32Flags, X86_EFL_1);
    IEM_MC_OR_2LOCS_U32(EFlags, u32Flags);
    IEM_MC_COMMIT_EFLAGS(EFlags);
    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/**
 * @opcode      0x9f
 */
FNIEMOP_DEF(iemOp_lahf)
{
    IEMOP_MNEMONIC(lahf, "lahf");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    if (   pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT
        && !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fLahfSahf)
        return IEMOP_RAISE_INVALID_OPCODE();
    IEM_MC_BEGIN(0, 1);
    IEM_MC_LOCAL(uint8_t, u8Flags);
    IEM_MC_FETCH_EFLAGS_U8(u8Flags);
    IEM_MC_STORE_GREG_U8(X86_GREG_xSP/*=AH*/, u8Flags);
    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/**
 * Macro used by iemOp_mov_AL_Ob, iemOp_mov_rAX_Ov, iemOp_mov_Ob_AL and
 * iemOp_mov_Ov_rAX to fetch the moffsXX bit of the opcode and fend off lock
 * prefixes.  Will return on failures.
 * @param   a_GCPtrMemOff   The variable to store the offset in.
 */
#define IEMOP_FETCH_MOFFS_XX(a_GCPtrMemOff) \
    do \
    { \
        switch (pVCpu->iem.s.enmEffAddrMode) \
        { \
            case IEMMODE_16BIT: \
                IEM_OPCODE_GET_NEXT_U16_ZX_U64(&(a_GCPtrMemOff)); \
                break; \
            case IEMMODE_32BIT: \
                IEM_OPCODE_GET_NEXT_U32_ZX_U64(&(a_GCPtrMemOff)); \
                break; \
            case IEMMODE_64BIT: \
                IEM_OPCODE_GET_NEXT_U64(&(a_GCPtrMemOff)); \
                break; \
            IEM_NOT_REACHED_DEFAULT_CASE_RET(); \
        } \
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX(); \
    } while (0)

/**
 * @opcode      0xa0
 */
FNIEMOP_DEF(iemOp_mov_AL_Ob)
{
    /*
     * Get the offset and fend off lock prefixes.
     */
    IEMOP_MNEMONIC(mov_AL_Ob, "mov AL,Ob");
    RTGCPTR GCPtrMemOff;
    IEMOP_FETCH_MOFFS_XX(GCPtrMemOff);

    /*
     * Fetch AL.
     */
    IEM_MC_BEGIN(0,1);
    IEM_MC_LOCAL(uint8_t, u8Tmp);
    IEM_MC_FETCH_MEM_U8(u8Tmp, pVCpu->iem.s.iEffSeg, GCPtrMemOff);
    IEM_MC_STORE_GREG_U8(X86_GREG_xAX, u8Tmp);
    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/**
 * @opcode      0xa1
 */
FNIEMOP_DEF(iemOp_mov_rAX_Ov)
{
    /*
     * Get the offset and fend off lock prefixes.
     */
    IEMOP_MNEMONIC(mov_rAX_Ov, "mov rAX,Ov");
    RTGCPTR GCPtrMemOff;
    IEMOP_FETCH_MOFFS_XX(GCPtrMemOff);

    /*
     * Fetch rAX.
     */
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0,1);
            IEM_MC_LOCAL(uint16_t, u16Tmp);
            IEM_MC_FETCH_MEM_U16(u16Tmp, pVCpu->iem.s.iEffSeg, GCPtrMemOff);
            IEM_MC_STORE_GREG_U16(X86_GREG_xAX, u16Tmp);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0,1);
            IEM_MC_LOCAL(uint32_t, u32Tmp);
            IEM_MC_FETCH_MEM_U32(u32Tmp, pVCpu->iem.s.iEffSeg, GCPtrMemOff);
            IEM_MC_STORE_GREG_U32(X86_GREG_xAX, u32Tmp);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0,1);
            IEM_MC_LOCAL(uint64_t, u64Tmp);
            IEM_MC_FETCH_MEM_U64(u64Tmp, pVCpu->iem.s.iEffSeg, GCPtrMemOff);
            IEM_MC_STORE_GREG_U64(X86_GREG_xAX, u64Tmp);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0xa2
 */
FNIEMOP_DEF(iemOp_mov_Ob_AL)
{
    /*
     * Get the offset and fend off lock prefixes.
     */
    IEMOP_MNEMONIC(mov_Ob_AL, "mov Ob,AL");
    RTGCPTR GCPtrMemOff;
    IEMOP_FETCH_MOFFS_XX(GCPtrMemOff);

    /*
     * Store AL.
     */
    IEM_MC_BEGIN(0,1);
    IEM_MC_LOCAL(uint8_t, u8Tmp);
    IEM_MC_FETCH_GREG_U8(u8Tmp, X86_GREG_xAX);
    IEM_MC_STORE_MEM_U8(pVCpu->iem.s.iEffSeg, GCPtrMemOff, u8Tmp);
    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/**
 * @opcode      0xa3
 */
FNIEMOP_DEF(iemOp_mov_Ov_rAX)
{
    /*
     * Get the offset and fend off lock prefixes.
     */
    IEMOP_MNEMONIC(mov_Ov_rAX, "mov Ov,rAX");
    RTGCPTR GCPtrMemOff;
    IEMOP_FETCH_MOFFS_XX(GCPtrMemOff);

    /*
     * Store rAX.
     */
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0,1);
            IEM_MC_LOCAL(uint16_t, u16Tmp);
            IEM_MC_FETCH_GREG_U16(u16Tmp, X86_GREG_xAX);
            IEM_MC_STORE_MEM_U16(pVCpu->iem.s.iEffSeg, GCPtrMemOff, u16Tmp);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0,1);
            IEM_MC_LOCAL(uint32_t, u32Tmp);
            IEM_MC_FETCH_GREG_U32(u32Tmp, X86_GREG_xAX);
            IEM_MC_STORE_MEM_U32(pVCpu->iem.s.iEffSeg, GCPtrMemOff, u32Tmp);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0,1);
            IEM_MC_LOCAL(uint64_t, u64Tmp);
            IEM_MC_FETCH_GREG_U64(u64Tmp, X86_GREG_xAX);
            IEM_MC_STORE_MEM_U64(pVCpu->iem.s.iEffSeg, GCPtrMemOff, u64Tmp);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}

/** Macro used by iemOp_movsb_Xb_Yb and iemOp_movswd_Xv_Yv */
#define IEM_MOVS_CASE(ValBits, AddrBits) \
        IEM_MC_BEGIN(0, 2); \
        IEM_MC_LOCAL(uint##ValBits##_t, uValue); \
        IEM_MC_LOCAL(RTGCPTR,           uAddr); \
        IEM_MC_FETCH_GREG_U##AddrBits##_ZX_U64(uAddr, X86_GREG_xSI); \
        IEM_MC_FETCH_MEM_U##ValBits(uValue, pVCpu->iem.s.iEffSeg, uAddr); \
        IEM_MC_FETCH_GREG_U##AddrBits##_ZX_U64(uAddr, X86_GREG_xDI); \
        IEM_MC_STORE_MEM_U##ValBits(X86_SREG_ES, uAddr, uValue); \
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_DF) { \
            IEM_MC_SUB_GREG_U##AddrBits(X86_GREG_xDI, ValBits / 8); \
            IEM_MC_SUB_GREG_U##AddrBits(X86_GREG_xSI, ValBits / 8); \
        } IEM_MC_ELSE() { \
            IEM_MC_ADD_GREG_U##AddrBits(X86_GREG_xDI, ValBits / 8); \
            IEM_MC_ADD_GREG_U##AddrBits(X86_GREG_xSI, ValBits / 8); \
        } IEM_MC_ENDIF(); \
        IEM_MC_ADVANCE_RIP_AND_FINISH(); \
        IEM_MC_END()

/**
 * @opcode      0xa4
 */
FNIEMOP_DEF(iemOp_movsb_Xb_Yb)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    /*
     * Use the C implementation if a repeat prefix is encountered.
     */
    if (pVCpu->iem.s.fPrefixes & (IEM_OP_PRF_REPNZ | IEM_OP_PRF_REPZ))
    {
        IEMOP_MNEMONIC(rep_movsb_Xb_Yb, "rep movsb Xb,Yb");
        switch (pVCpu->iem.s.enmEffAddrMode)
        {
            case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op8_addr16, pVCpu->iem.s.iEffSeg);
            case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op8_addr32, pVCpu->iem.s.iEffSeg);
            case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op8_addr64, pVCpu->iem.s.iEffSeg);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    IEMOP_MNEMONIC(movsb_Xb_Yb, "movsb Xb,Yb");

    /*
     * Sharing case implementation with movs[wdq] below.
     */
    switch (pVCpu->iem.s.enmEffAddrMode)
    {
        case IEMMODE_16BIT: IEM_MOVS_CASE(8, 16); break;
        case IEMMODE_32BIT: IEM_MOVS_CASE(8, 32); break;
        case IEMMODE_64BIT: IEM_MOVS_CASE(8, 64); break;
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0xa5
 */
FNIEMOP_DEF(iemOp_movswd_Xv_Yv)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    /*
     * Use the C implementation if a repeat prefix is encountered.
     */
    if (pVCpu->iem.s.fPrefixes & (IEM_OP_PRF_REPNZ | IEM_OP_PRF_REPZ))
    {
        IEMOP_MNEMONIC(rep_movs_Xv_Yv, "rep movs Xv,Yv");
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op16_addr16, pVCpu->iem.s.iEffSeg);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op16_addr32, pVCpu->iem.s.iEffSeg);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op16_addr64, pVCpu->iem.s.iEffSeg);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
                break;
            case IEMMODE_32BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op32_addr16, pVCpu->iem.s.iEffSeg);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op32_addr32, pVCpu->iem.s.iEffSeg);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op32_addr64, pVCpu->iem.s.iEffSeg);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
            case IEMMODE_64BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: AssertFailedReturn(VERR_IEM_IPE_6);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op64_addr32, pVCpu->iem.s.iEffSeg);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op64_addr64, pVCpu->iem.s.iEffSeg);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    IEMOP_MNEMONIC(movs_Xv_Yv, "movs Xv,Yv");

    /*
     * Annoying double switch here.
     * Using ugly macro for implementing the cases, sharing it with movsb.
     */
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            switch (pVCpu->iem.s.enmEffAddrMode)
            {
                case IEMMODE_16BIT: IEM_MOVS_CASE(16, 16); break;
                case IEMMODE_32BIT: IEM_MOVS_CASE(16, 32); break;
                case IEMMODE_64BIT: IEM_MOVS_CASE(16, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;

        case IEMMODE_32BIT:
            switch (pVCpu->iem.s.enmEffAddrMode)
            {
                case IEMMODE_16BIT: IEM_MOVS_CASE(32, 16); break;
                case IEMMODE_32BIT: IEM_MOVS_CASE(32, 32); break;
                case IEMMODE_64BIT: IEM_MOVS_CASE(32, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;

        case IEMMODE_64BIT:
            switch (pVCpu->iem.s.enmEffAddrMode)
            {
                case IEMMODE_16BIT: AssertFailedReturn(VERR_IEM_IPE_1); /* cannot be encoded */ break;
                case IEMMODE_32BIT: IEM_MOVS_CASE(64, 32); break;
                case IEMMODE_64BIT: IEM_MOVS_CASE(64, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}

#undef IEM_MOVS_CASE

/** Macro used by iemOp_cmpsb_Xb_Yb and iemOp_cmpswd_Xv_Yv */
#define IEM_CMPS_CASE(ValBits, AddrBits) \
        IEM_MC_BEGIN(3, 3); \
        IEM_MC_ARG(uint##ValBits##_t *, puValue1, 0); \
        IEM_MC_ARG(uint##ValBits##_t,   uValue2,  1); \
        IEM_MC_ARG(uint32_t *,          pEFlags,  2); \
        IEM_MC_LOCAL(uint##ValBits##_t, uValue1); \
        IEM_MC_LOCAL(RTGCPTR,           uAddr); \
        \
        IEM_MC_FETCH_GREG_U##AddrBits##_ZX_U64(uAddr, X86_GREG_xSI); \
        IEM_MC_FETCH_MEM_U##ValBits(uValue1, pVCpu->iem.s.iEffSeg, uAddr); \
        IEM_MC_FETCH_GREG_U##AddrBits##_ZX_U64(uAddr, X86_GREG_xDI); \
        IEM_MC_FETCH_MEM_U##ValBits(uValue2, X86_SREG_ES, uAddr); \
        IEM_MC_REF_LOCAL(puValue1, uValue1); \
        IEM_MC_REF_EFLAGS(pEFlags); \
        IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_cmp_u##ValBits, puValue1, uValue2, pEFlags); \
        \
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_DF) { \
            IEM_MC_SUB_GREG_U##AddrBits(X86_GREG_xDI, ValBits / 8); \
            IEM_MC_SUB_GREG_U##AddrBits(X86_GREG_xSI, ValBits / 8); \
        } IEM_MC_ELSE() { \
            IEM_MC_ADD_GREG_U##AddrBits(X86_GREG_xDI, ValBits / 8); \
            IEM_MC_ADD_GREG_U##AddrBits(X86_GREG_xSI, ValBits / 8); \
        } IEM_MC_ENDIF(); \
        IEM_MC_ADVANCE_RIP_AND_FINISH(); \
        IEM_MC_END()

/**
 * @opcode      0xa6
 */
FNIEMOP_DEF(iemOp_cmpsb_Xb_Yb)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    /*
     * Use the C implementation if a repeat prefix is encountered.
     */
    if (pVCpu->iem.s.fPrefixes & IEM_OP_PRF_REPZ)
    {
        IEMOP_MNEMONIC(repz_cmps_Xb_Yb, "repz cmps Xb,Yb");
        switch (pVCpu->iem.s.enmEffAddrMode)
        {
            case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repe_cmps_op8_addr16, pVCpu->iem.s.iEffSeg);
            case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repe_cmps_op8_addr32, pVCpu->iem.s.iEffSeg);
            case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repe_cmps_op8_addr64, pVCpu->iem.s.iEffSeg);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    if (pVCpu->iem.s.fPrefixes & IEM_OP_PRF_REPNZ)
    {
        IEMOP_MNEMONIC(repnz_cmps_Xb_Yb, "repnz cmps Xb,Yb");
        switch (pVCpu->iem.s.enmEffAddrMode)
        {
            case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repne_cmps_op8_addr16, pVCpu->iem.s.iEffSeg);
            case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repne_cmps_op8_addr32, pVCpu->iem.s.iEffSeg);
            case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repne_cmps_op8_addr64, pVCpu->iem.s.iEffSeg);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    IEMOP_MNEMONIC(cmps_Xb_Yb, "cmps Xb,Yb");

    /*
     * Sharing case implementation with cmps[wdq] below.
     */
    switch (pVCpu->iem.s.enmEffAddrMode)
    {
        case IEMMODE_16BIT: IEM_CMPS_CASE(8, 16); break;
        case IEMMODE_32BIT: IEM_CMPS_CASE(8, 32); break;
        case IEMMODE_64BIT: IEM_CMPS_CASE(8, 64); break;
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0xa7
 */
FNIEMOP_DEF(iemOp_cmpswd_Xv_Yv)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    /*
     * Use the C implementation if a repeat prefix is encountered.
     */
    if (pVCpu->iem.s.fPrefixes & IEM_OP_PRF_REPZ)
    {
        IEMOP_MNEMONIC(repe_cmps_Xv_Yv, "repe cmps Xv,Yv");
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repe_cmps_op16_addr16, pVCpu->iem.s.iEffSeg);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repe_cmps_op16_addr32, pVCpu->iem.s.iEffSeg);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repe_cmps_op16_addr64, pVCpu->iem.s.iEffSeg);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
                break;
            case IEMMODE_32BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repe_cmps_op32_addr16, pVCpu->iem.s.iEffSeg);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repe_cmps_op32_addr32, pVCpu->iem.s.iEffSeg);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repe_cmps_op32_addr64, pVCpu->iem.s.iEffSeg);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
            case IEMMODE_64BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: AssertFailedReturn(VERR_IEM_IPE_4);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repe_cmps_op64_addr32, pVCpu->iem.s.iEffSeg);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repe_cmps_op64_addr64, pVCpu->iem.s.iEffSeg);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }

    if (pVCpu->iem.s.fPrefixes & IEM_OP_PRF_REPNZ)
    {
        IEMOP_MNEMONIC(repne_cmps_Xv_Yv, "repne cmps Xv,Yv");
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repne_cmps_op16_addr16, pVCpu->iem.s.iEffSeg);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repne_cmps_op16_addr32, pVCpu->iem.s.iEffSeg);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repne_cmps_op16_addr64, pVCpu->iem.s.iEffSeg);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
                break;
            case IEMMODE_32BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repne_cmps_op32_addr16, pVCpu->iem.s.iEffSeg);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repne_cmps_op32_addr32, pVCpu->iem.s.iEffSeg);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repne_cmps_op32_addr64, pVCpu->iem.s.iEffSeg);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
            case IEMMODE_64BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: AssertFailedReturn(VERR_IEM_IPE_2);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repne_cmps_op64_addr32, pVCpu->iem.s.iEffSeg);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_repne_cmps_op64_addr64, pVCpu->iem.s.iEffSeg);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }

    IEMOP_MNEMONIC(cmps_Xv_Yv, "cmps Xv,Yv");

    /*
     * Annoying double switch here.
     * Using ugly macro for implementing the cases, sharing it with cmpsb.
     */
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            switch (pVCpu->iem.s.enmEffAddrMode)
            {
                case IEMMODE_16BIT: IEM_CMPS_CASE(16, 16); break;
                case IEMMODE_32BIT: IEM_CMPS_CASE(16, 32); break;
                case IEMMODE_64BIT: IEM_CMPS_CASE(16, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;

        case IEMMODE_32BIT:
            switch (pVCpu->iem.s.enmEffAddrMode)
            {
                case IEMMODE_16BIT: IEM_CMPS_CASE(32, 16); break;
                case IEMMODE_32BIT: IEM_CMPS_CASE(32, 32); break;
                case IEMMODE_64BIT: IEM_CMPS_CASE(32, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;

        case IEMMODE_64BIT:
            switch (pVCpu->iem.s.enmEffAddrMode)
            {
                case IEMMODE_16BIT: AssertFailedReturn(VERR_IEM_IPE_1); /* cannot be encoded */ break;
                case IEMMODE_32BIT: IEM_CMPS_CASE(64, 32); break;
                case IEMMODE_64BIT: IEM_CMPS_CASE(64, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}

#undef IEM_CMPS_CASE

/**
 * @opcode      0xa8
 */
FNIEMOP_DEF(iemOp_test_AL_Ib)
{
    IEMOP_MNEMONIC(test_al_Ib, "test al,Ib");
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_AL_Ib, &g_iemAImpl_test);
}


/**
 * @opcode      0xa9
 */
FNIEMOP_DEF(iemOp_test_eAX_Iz)
{
    IEMOP_MNEMONIC(test_rAX_Iz, "test rAX,Iz");
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rAX_Iz, &g_iemAImpl_test);
}


/** Macro used by iemOp_stosb_Yb_AL and iemOp_stoswd_Yv_eAX */
#define IEM_STOS_CASE(ValBits, AddrBits) \
        IEM_MC_BEGIN(0, 2); \
        IEM_MC_LOCAL(uint##ValBits##_t, uValue); \
        IEM_MC_LOCAL(RTGCPTR, uAddr); \
        IEM_MC_FETCH_GREG_U##ValBits(uValue, X86_GREG_xAX); \
        IEM_MC_FETCH_GREG_U##AddrBits##_ZX_U64(uAddr,  X86_GREG_xDI); \
        IEM_MC_STORE_MEM_U##ValBits(X86_SREG_ES, uAddr, uValue); \
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_DF) { \
            IEM_MC_SUB_GREG_U##AddrBits(X86_GREG_xDI, ValBits / 8); \
        } IEM_MC_ELSE() { \
            IEM_MC_ADD_GREG_U##AddrBits(X86_GREG_xDI, ValBits / 8); \
        } IEM_MC_ENDIF(); \
        IEM_MC_ADVANCE_RIP_AND_FINISH(); \
        IEM_MC_END()

/**
 * @opcode      0xaa
 */
FNIEMOP_DEF(iemOp_stosb_Yb_AL)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    /*
     * Use the C implementation if a repeat prefix is encountered.
     */
    if (pVCpu->iem.s.fPrefixes & (IEM_OP_PRF_REPNZ | IEM_OP_PRF_REPZ))
    {
        IEMOP_MNEMONIC(rep_stos_Yb_al, "rep stos Yb,al");
        switch (pVCpu->iem.s.enmEffAddrMode)
        {
            case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_al_m16);
            case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_al_m32);
            case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_al_m64);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    IEMOP_MNEMONIC(stos_Yb_al, "stos Yb,al");

    /*
     * Sharing case implementation with stos[wdq] below.
     */
    switch (pVCpu->iem.s.enmEffAddrMode)
    {
        case IEMMODE_16BIT: IEM_STOS_CASE(8, 16); break;
        case IEMMODE_32BIT: IEM_STOS_CASE(8, 32); break;
        case IEMMODE_64BIT: IEM_STOS_CASE(8, 64); break;
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0xab
 */
FNIEMOP_DEF(iemOp_stoswd_Yv_eAX)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    /*
     * Use the C implementation if a repeat prefix is encountered.
     */
    if (pVCpu->iem.s.fPrefixes & (IEM_OP_PRF_REPNZ | IEM_OP_PRF_REPZ))
    {
        IEMOP_MNEMONIC(rep_stos_Yv_rAX, "rep stos Yv,rAX");
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_ax_m16);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_ax_m32);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_ax_m64);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
                break;
            case IEMMODE_32BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_eax_m16);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_eax_m32);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_eax_m64);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
            case IEMMODE_64BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: AssertFailedReturn(VERR_IEM_IPE_9);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_rax_m32);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_rax_m64);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    IEMOP_MNEMONIC(stos_Yv_rAX, "stos Yv,rAX");

    /*
     * Annoying double switch here.
     * Using ugly macro for implementing the cases, sharing it with stosb.
     */
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            switch (pVCpu->iem.s.enmEffAddrMode)
            {
                case IEMMODE_16BIT: IEM_STOS_CASE(16, 16); break;
                case IEMMODE_32BIT: IEM_STOS_CASE(16, 32); break;
                case IEMMODE_64BIT: IEM_STOS_CASE(16, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;

        case IEMMODE_32BIT:
            switch (pVCpu->iem.s.enmEffAddrMode)
            {
                case IEMMODE_16BIT: IEM_STOS_CASE(32, 16); break;
                case IEMMODE_32BIT: IEM_STOS_CASE(32, 32); break;
                case IEMMODE_64BIT: IEM_STOS_CASE(32, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;

        case IEMMODE_64BIT:
            switch (pVCpu->iem.s.enmEffAddrMode)
            {
                case IEMMODE_16BIT: AssertFailedReturn(VERR_IEM_IPE_1); /* cannot be encoded */ break;
                case IEMMODE_32BIT: IEM_STOS_CASE(64, 32); break;
                case IEMMODE_64BIT: IEM_STOS_CASE(64, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}

#undef IEM_STOS_CASE

/** Macro used by iemOp_lodsb_AL_Xb and iemOp_lodswd_eAX_Xv */
#define IEM_LODS_CASE(ValBits, AddrBits) \
        IEM_MC_BEGIN(0, 2); \
        IEM_MC_LOCAL(uint##ValBits##_t, uValue); \
        IEM_MC_LOCAL(RTGCPTR, uAddr); \
        IEM_MC_FETCH_GREG_U##AddrBits##_ZX_U64(uAddr, X86_GREG_xSI); \
        IEM_MC_FETCH_MEM_U##ValBits(uValue, pVCpu->iem.s.iEffSeg, uAddr); \
        IEM_MC_STORE_GREG_U##ValBits(X86_GREG_xAX, uValue); \
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_DF) { \
            IEM_MC_SUB_GREG_U##AddrBits(X86_GREG_xSI, ValBits / 8); \
        } IEM_MC_ELSE() { \
            IEM_MC_ADD_GREG_U##AddrBits(X86_GREG_xSI, ValBits / 8); \
        } IEM_MC_ENDIF(); \
        IEM_MC_ADVANCE_RIP_AND_FINISH(); \
        IEM_MC_END()

/**
 * @opcode      0xac
 */
FNIEMOP_DEF(iemOp_lodsb_AL_Xb)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    /*
     * Use the C implementation if a repeat prefix is encountered.
     */
    if (pVCpu->iem.s.fPrefixes & (IEM_OP_PRF_REPNZ | IEM_OP_PRF_REPZ))
    {
        IEMOP_MNEMONIC(rep_lodsb_AL_Xb, "rep lodsb AL,Xb");
        switch (pVCpu->iem.s.enmEffAddrMode)
        {
            case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_lods_al_m16, pVCpu->iem.s.iEffSeg);
            case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_lods_al_m32, pVCpu->iem.s.iEffSeg);
            case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_lods_al_m64, pVCpu->iem.s.iEffSeg);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    IEMOP_MNEMONIC(lodsb_AL_Xb, "lodsb AL,Xb");

    /*
     * Sharing case implementation with stos[wdq] below.
     */
    switch (pVCpu->iem.s.enmEffAddrMode)
    {
        case IEMMODE_16BIT: IEM_LODS_CASE(8, 16); break;
        case IEMMODE_32BIT: IEM_LODS_CASE(8, 32); break;
        case IEMMODE_64BIT: IEM_LODS_CASE(8, 64); break;
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0xad
 */
FNIEMOP_DEF(iemOp_lodswd_eAX_Xv)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    /*
     * Use the C implementation if a repeat prefix is encountered.
     */
    if (pVCpu->iem.s.fPrefixes & (IEM_OP_PRF_REPNZ | IEM_OP_PRF_REPZ))
    {
        IEMOP_MNEMONIC(rep_lods_rAX_Xv, "rep lods rAX,Xv");
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_lods_ax_m16, pVCpu->iem.s.iEffSeg);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_lods_ax_m32, pVCpu->iem.s.iEffSeg);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_lods_ax_m64, pVCpu->iem.s.iEffSeg);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
                break;
            case IEMMODE_32BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_lods_eax_m16, pVCpu->iem.s.iEffSeg);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_lods_eax_m32, pVCpu->iem.s.iEffSeg);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_lods_eax_m64, pVCpu->iem.s.iEffSeg);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
            case IEMMODE_64BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: AssertFailedReturn(VERR_IEM_IPE_7);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_lods_rax_m32, pVCpu->iem.s.iEffSeg);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_lods_rax_m64, pVCpu->iem.s.iEffSeg);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    IEMOP_MNEMONIC(lods_rAX_Xv, "lods rAX,Xv");

    /*
     * Annoying double switch here.
     * Using ugly macro for implementing the cases, sharing it with lodsb.
     */
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            switch (pVCpu->iem.s.enmEffAddrMode)
            {
                case IEMMODE_16BIT: IEM_LODS_CASE(16, 16); break;
                case IEMMODE_32BIT: IEM_LODS_CASE(16, 32); break;
                case IEMMODE_64BIT: IEM_LODS_CASE(16, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;

        case IEMMODE_32BIT:
            switch (pVCpu->iem.s.enmEffAddrMode)
            {
                case IEMMODE_16BIT: IEM_LODS_CASE(32, 16); break;
                case IEMMODE_32BIT: IEM_LODS_CASE(32, 32); break;
                case IEMMODE_64BIT: IEM_LODS_CASE(32, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;

        case IEMMODE_64BIT:
            switch (pVCpu->iem.s.enmEffAddrMode)
            {
                case IEMMODE_16BIT: AssertFailedReturn(VERR_IEM_IPE_1); /* cannot be encoded */ break;
                case IEMMODE_32BIT: IEM_LODS_CASE(64, 32); break;
                case IEMMODE_64BIT: IEM_LODS_CASE(64, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}

#undef IEM_LODS_CASE

/** Macro used by iemOp_scasb_AL_Xb and iemOp_scaswd_eAX_Xv */
#define IEM_SCAS_CASE(ValBits, AddrBits) \
        IEM_MC_BEGIN(3, 2); \
        IEM_MC_ARG(uint##ValBits##_t *, puRax,   0); \
        IEM_MC_ARG(uint##ValBits##_t,   uValue,  1); \
        IEM_MC_ARG(uint32_t *,          pEFlags, 2); \
        IEM_MC_LOCAL(RTGCPTR,           uAddr); \
        \
        IEM_MC_FETCH_GREG_U##AddrBits##_ZX_U64(uAddr, X86_GREG_xDI); \
        IEM_MC_FETCH_MEM_U##ValBits(uValue, X86_SREG_ES, uAddr); \
        IEM_MC_REF_GREG_U##ValBits(puRax, X86_GREG_xAX); \
        IEM_MC_REF_EFLAGS(pEFlags); \
        IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_cmp_u##ValBits, puRax, uValue, pEFlags); \
        \
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_DF) { \
            IEM_MC_SUB_GREG_U##AddrBits(X86_GREG_xDI, ValBits / 8); \
        } IEM_MC_ELSE() { \
            IEM_MC_ADD_GREG_U##AddrBits(X86_GREG_xDI, ValBits / 8); \
        } IEM_MC_ENDIF(); \
        IEM_MC_ADVANCE_RIP_AND_FINISH(); \
        IEM_MC_END()

/**
 * @opcode      0xae
 */
FNIEMOP_DEF(iemOp_scasb_AL_Xb)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    /*
     * Use the C implementation if a repeat prefix is encountered.
     */
    if (pVCpu->iem.s.fPrefixes & IEM_OP_PRF_REPZ)
    {
        IEMOP_MNEMONIC(repe_scasb_AL_Xb, "repe scasb AL,Xb");
        switch (pVCpu->iem.s.enmEffAddrMode)
        {
            case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repe_scas_al_m16);
            case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repe_scas_al_m32);
            case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repe_scas_al_m64);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    if (pVCpu->iem.s.fPrefixes & IEM_OP_PRF_REPNZ)
    {
        IEMOP_MNEMONIC(repone_scasb_AL_Xb, "repne scasb AL,Xb");
        switch (pVCpu->iem.s.enmEffAddrMode)
        {
            case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repne_scas_al_m16);
            case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repne_scas_al_m32);
            case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repne_scas_al_m64);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    IEMOP_MNEMONIC(scasb_AL_Xb, "scasb AL,Xb");

    /*
     * Sharing case implementation with stos[wdq] below.
     */
    switch (pVCpu->iem.s.enmEffAddrMode)
    {
        case IEMMODE_16BIT: IEM_SCAS_CASE(8, 16); break;
        case IEMMODE_32BIT: IEM_SCAS_CASE(8, 32); break;
        case IEMMODE_64BIT: IEM_SCAS_CASE(8, 64); break;
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0xaf
 */
FNIEMOP_DEF(iemOp_scaswd_eAX_Xv)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    /*
     * Use the C implementation if a repeat prefix is encountered.
     */
    if (pVCpu->iem.s.fPrefixes & IEM_OP_PRF_REPZ)
    {
        IEMOP_MNEMONIC(repe_scas_rAX_Xv, "repe scas rAX,Xv");
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repe_scas_ax_m16);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repe_scas_ax_m32);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repe_scas_ax_m64);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
                break;
            case IEMMODE_32BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repe_scas_eax_m16);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repe_scas_eax_m32);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repe_scas_eax_m64);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
            case IEMMODE_64BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: AssertFailedReturn(VERR_IEM_IPE_6); /** @todo It's this wrong, we can do 16-bit addressing in 64-bit mode, but not 32-bit. right? */
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repe_scas_rax_m32);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repe_scas_rax_m64);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    if (pVCpu->iem.s.fPrefixes & IEM_OP_PRF_REPNZ)
    {
        IEMOP_MNEMONIC(repne_scas_rAX_Xv, "repne scas rAX,Xv");
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repne_scas_ax_m16);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repne_scas_ax_m32);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repne_scas_ax_m64);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
                break;
            case IEMMODE_32BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repne_scas_eax_m16);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repne_scas_eax_m32);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repne_scas_eax_m64);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
            case IEMMODE_64BIT:
                switch (pVCpu->iem.s.enmEffAddrMode)
                {
                    case IEMMODE_16BIT: AssertFailedReturn(VERR_IEM_IPE_5);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repne_scas_rax_m32);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_repne_scas_rax_m64);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    IEMOP_MNEMONIC(scas_rAX_Xv, "scas rAX,Xv");

    /*
     * Annoying double switch here.
     * Using ugly macro for implementing the cases, sharing it with scasb.
     */
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            switch (pVCpu->iem.s.enmEffAddrMode)
            {
                case IEMMODE_16BIT: IEM_SCAS_CASE(16, 16); break;
                case IEMMODE_32BIT: IEM_SCAS_CASE(16, 32); break;
                case IEMMODE_64BIT: IEM_SCAS_CASE(16, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;

        case IEMMODE_32BIT:
            switch (pVCpu->iem.s.enmEffAddrMode)
            {
                case IEMMODE_16BIT: IEM_SCAS_CASE(32, 16); break;
                case IEMMODE_32BIT: IEM_SCAS_CASE(32, 32); break;
                case IEMMODE_64BIT: IEM_SCAS_CASE(32, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;

        case IEMMODE_64BIT:
            switch (pVCpu->iem.s.enmEffAddrMode)
            {
                case IEMMODE_16BIT: AssertFailedReturn(VERR_IEM_IPE_1); /* cannot be encoded */ break;
                case IEMMODE_32BIT: IEM_SCAS_CASE(64, 32); break;
                case IEMMODE_64BIT: IEM_SCAS_CASE(64, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}

#undef IEM_SCAS_CASE

/**
 * Common 'mov r8, imm8' helper.
 */
FNIEMOP_DEF_1(iemOpCommonMov_r8_Ib, uint8_t, iReg)
{
    uint8_t u8Imm; IEM_OPCODE_GET_NEXT_U8(&u8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(0, 1);
    IEM_MC_LOCAL_CONST(uint8_t, u8Value,/*=*/ u8Imm);
    IEM_MC_STORE_GREG_U8(iReg, u8Value);
    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/**
 * @opcode      0xb0
 */
FNIEMOP_DEF(iemOp_mov_AL_Ib)
{
    IEMOP_MNEMONIC(mov_AL_Ib, "mov AL,Ib");
    return FNIEMOP_CALL_1(iemOpCommonMov_r8_Ib, X86_GREG_xAX | pVCpu->iem.s.uRexB);
}


/**
 * @opcode      0xb1
 */
FNIEMOP_DEF(iemOp_CL_Ib)
{
    IEMOP_MNEMONIC(mov_CL_Ib, "mov CL,Ib");
    return FNIEMOP_CALL_1(iemOpCommonMov_r8_Ib, X86_GREG_xCX | pVCpu->iem.s.uRexB);
}


/**
 * @opcode      0xb2
 */
FNIEMOP_DEF(iemOp_DL_Ib)
{
    IEMOP_MNEMONIC(mov_DL_Ib, "mov DL,Ib");
    return FNIEMOP_CALL_1(iemOpCommonMov_r8_Ib, X86_GREG_xDX | pVCpu->iem.s.uRexB);
}


/**
 * @opcode      0xb3
 */
FNIEMOP_DEF(iemOp_BL_Ib)
{
    IEMOP_MNEMONIC(mov_BL_Ib, "mov BL,Ib");
    return FNIEMOP_CALL_1(iemOpCommonMov_r8_Ib, X86_GREG_xBX | pVCpu->iem.s.uRexB);
}


/**
 * @opcode      0xb4
 */
FNIEMOP_DEF(iemOp_mov_AH_Ib)
{
    IEMOP_MNEMONIC(mov_AH_Ib, "mov AH,Ib");
    return FNIEMOP_CALL_1(iemOpCommonMov_r8_Ib, X86_GREG_xSP | pVCpu->iem.s.uRexB);
}


/**
 * @opcode      0xb5
 */
FNIEMOP_DEF(iemOp_CH_Ib)
{
    IEMOP_MNEMONIC(mov_CH_Ib, "mov CH,Ib");
    return FNIEMOP_CALL_1(iemOpCommonMov_r8_Ib, X86_GREG_xBP | pVCpu->iem.s.uRexB);
}


/**
 * @opcode      0xb6
 */
FNIEMOP_DEF(iemOp_DH_Ib)
{
    IEMOP_MNEMONIC(mov_DH_Ib, "mov DH,Ib");
    return FNIEMOP_CALL_1(iemOpCommonMov_r8_Ib, X86_GREG_xSI | pVCpu->iem.s.uRexB);
}


/**
 * @opcode      0xb7
 */
FNIEMOP_DEF(iemOp_BH_Ib)
{
    IEMOP_MNEMONIC(mov_BH_Ib, "mov BH,Ib");
    return FNIEMOP_CALL_1(iemOpCommonMov_r8_Ib, X86_GREG_xDI | pVCpu->iem.s.uRexB);
}


/**
 * Common 'mov regX,immX' helper.
 */
FNIEMOP_DEF_1(iemOpCommonMov_Rv_Iv, uint8_t, iReg)
{
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
        {
            uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(&u16Imm);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL_CONST(uint16_t, u16Value,/*=*/ u16Imm);
            IEM_MC_STORE_GREG_U16(iReg, u16Value);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;
        }

        case IEMMODE_32BIT:
        {
            uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(&u32Imm);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL_CONST(uint32_t, u32Value,/*=*/ u32Imm);
            IEM_MC_STORE_GREG_U32(iReg, u32Value);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;
        }
        case IEMMODE_64BIT:
        {
            uint64_t u64Imm; IEM_OPCODE_GET_NEXT_U64(&u64Imm); /* 64-bit immediate! */
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL_CONST(uint64_t, u64Value,/*=*/ u64Imm);
            IEM_MC_STORE_GREG_U64(iReg, u64Value);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;
        }
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0xb8
 */
FNIEMOP_DEF(iemOp_eAX_Iv)
{
    IEMOP_MNEMONIC(mov_rAX_IV, "mov rAX,IV");
    return FNIEMOP_CALL_1(iemOpCommonMov_Rv_Iv, X86_GREG_xAX | pVCpu->iem.s.uRexB);
}


/**
 * @opcode      0xb9
 */
FNIEMOP_DEF(iemOp_eCX_Iv)
{
    IEMOP_MNEMONIC(mov_rCX_IV, "mov rCX,IV");
    return FNIEMOP_CALL_1(iemOpCommonMov_Rv_Iv, X86_GREG_xCX | pVCpu->iem.s.uRexB);
}


/**
 * @opcode      0xba
 */
FNIEMOP_DEF(iemOp_eDX_Iv)
{
    IEMOP_MNEMONIC(mov_rDX_IV, "mov rDX,IV");
    return FNIEMOP_CALL_1(iemOpCommonMov_Rv_Iv, X86_GREG_xDX | pVCpu->iem.s.uRexB);
}


/**
 * @opcode      0xbb
 */
FNIEMOP_DEF(iemOp_eBX_Iv)
{
    IEMOP_MNEMONIC(mov_rBX_IV, "mov rBX,IV");
    return FNIEMOP_CALL_1(iemOpCommonMov_Rv_Iv, X86_GREG_xBX | pVCpu->iem.s.uRexB);
}


/**
 * @opcode      0xbc
 */
FNIEMOP_DEF(iemOp_eSP_Iv)
{
    IEMOP_MNEMONIC(mov_rSP_IV, "mov rSP,IV");
    return FNIEMOP_CALL_1(iemOpCommonMov_Rv_Iv, X86_GREG_xSP | pVCpu->iem.s.uRexB);
}


/**
 * @opcode      0xbd
 */
FNIEMOP_DEF(iemOp_eBP_Iv)
{
    IEMOP_MNEMONIC(mov_rBP_IV, "mov rBP,IV");
    return FNIEMOP_CALL_1(iemOpCommonMov_Rv_Iv, X86_GREG_xBP | pVCpu->iem.s.uRexB);
}


/**
 * @opcode      0xbe
 */
FNIEMOP_DEF(iemOp_eSI_Iv)
{
    IEMOP_MNEMONIC(mov_rSI_IV, "mov rSI,IV");
    return FNIEMOP_CALL_1(iemOpCommonMov_Rv_Iv, X86_GREG_xSI | pVCpu->iem.s.uRexB);
}


/**
 * @opcode      0xbf
 */
FNIEMOP_DEF(iemOp_eDI_Iv)
{
    IEMOP_MNEMONIC(mov_rDI_IV, "mov rDI,IV");
    return FNIEMOP_CALL_1(iemOpCommonMov_Rv_Iv, X86_GREG_xDI | pVCpu->iem.s.uRexB);
}


/**
 * @opcode      0xc0
 */
FNIEMOP_DEF(iemOp_Grp2_Eb_Ib)
{
    IEMOP_HLP_MIN_186();
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    PCIEMOPSHIFTSIZES pImpl;
    switch (IEM_GET_MODRM_REG_8(bRm))
    {
        case 0: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_rol_eflags); IEMOP_MNEMONIC(rol_Eb_Ib, "rol Eb,Ib"); break;
        case 1: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_ror_eflags); IEMOP_MNEMONIC(ror_Eb_Ib, "ror Eb,Ib"); break;
        case 2: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_rcl_eflags); IEMOP_MNEMONIC(rcl_Eb_Ib, "rcl Eb,Ib"); break;
        case 3: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_rcr_eflags); IEMOP_MNEMONIC(rcr_Eb_Ib, "rcr Eb,Ib"); break;
        case 4: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_shl_eflags); IEMOP_MNEMONIC(shl_Eb_Ib, "shl Eb,Ib"); break;
        case 5: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_shr_eflags); IEMOP_MNEMONIC(shr_Eb_Ib, "shr Eb,Ib"); break;
        case 7: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_sar_eflags); IEMOP_MNEMONIC(sar_Eb_Ib, "sar Eb,Ib"); break;
        case 6: return IEMOP_RAISE_INVALID_OPCODE();
        IEM_NOT_REACHED_DEFAULT_CASE_RET(); /* gcc maybe stupid */
    }
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_OF | X86_EFL_AF);

    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        /* register */
        uint8_t cShift; IEM_OPCODE_GET_NEXT_U8(&cShift);
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_BEGIN(3, 0);
        IEM_MC_ARG(uint8_t *,       pu8Dst,            0);
        IEM_MC_ARG_CONST(uint8_t,   cShiftArg, cShift, 1);
        IEM_MC_ARG(uint32_t *,      pEFlags,           2);
        IEM_MC_REF_GREG_U8(pu8Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, cShiftArg, pEFlags);
        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
    else
    {
        /* memory */
        IEM_MC_BEGIN(3, 2);
        IEM_MC_ARG(uint8_t *,   pu8Dst,    0);
        IEM_MC_ARG(uint8_t,     cShiftArg,  1);
        IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 1);
        uint8_t cShift; IEM_OPCODE_GET_NEXT_U8(&cShift);
        IEM_MC_ASSIGN(cShiftArg, cShift);
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_MEM_MAP(pu8Dst, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
        IEM_MC_FETCH_EFLAGS(EFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, cShiftArg, pEFlags);

        IEM_MC_MEM_COMMIT_AND_UNMAP(pu8Dst, IEM_ACCESS_DATA_RW);
        IEM_MC_COMMIT_EFLAGS(EFlags);
        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
}


/**
 * @opcode      0xc1
 */
FNIEMOP_DEF(iemOp_Grp2_Ev_Ib)
{
    IEMOP_HLP_MIN_186();
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    PCIEMOPSHIFTSIZES pImpl;
    switch (IEM_GET_MODRM_REG_8(bRm))
    {
        case 0: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_rol_eflags); IEMOP_MNEMONIC(rol_Ev_Ib, "rol Ev,Ib"); break;
        case 1: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_ror_eflags); IEMOP_MNEMONIC(ror_Ev_Ib, "ror Ev,Ib"); break;
        case 2: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_rcl_eflags); IEMOP_MNEMONIC(rcl_Ev_Ib, "rcl Ev,Ib"); break;
        case 3: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_rcr_eflags); IEMOP_MNEMONIC(rcr_Ev_Ib, "rcr Ev,Ib"); break;
        case 4: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_shl_eflags); IEMOP_MNEMONIC(shl_Ev_Ib, "shl Ev,Ib"); break;
        case 5: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_shr_eflags); IEMOP_MNEMONIC(shr_Ev_Ib, "shr Ev,Ib"); break;
        case 7: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_sar_eflags); IEMOP_MNEMONIC(sar_Ev_Ib, "sar Ev,Ib"); break;
        case 6: return IEMOP_RAISE_INVALID_OPCODE();
        IEM_NOT_REACHED_DEFAULT_CASE_RET(); /* gcc maybe stupid */
    }
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_OF | X86_EFL_AF);

    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        /* register */
        uint8_t cShift; IEM_OPCODE_GET_NEXT_U8(&cShift);
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint16_t *,      pu16Dst,           0);
                IEM_MC_ARG_CONST(uint8_t,   cShiftArg, cShift, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,           2);
                IEM_MC_REF_GREG_U16(pu16Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, cShiftArg, pEFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint32_t *,      pu32Dst,           0);
                IEM_MC_ARG_CONST(uint8_t,   cShiftArg, cShift, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,           2);
                IEM_MC_REF_GREG_U32(pu32Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, cShiftArg, pEFlags);
                IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(pu32Dst);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint64_t *,      pu64Dst,           0);
                IEM_MC_ARG_CONST(uint8_t,   cShiftArg, cShift, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,           2);
                IEM_MC_REF_GREG_U64(pu64Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, cShiftArg, pEFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /* memory */
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint16_t *,  pu16Dst,    0);
                IEM_MC_ARG(uint8_t,     cShiftArg,  1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 1);
                uint8_t cShift; IEM_OPCODE_GET_NEXT_U8(&cShift);
                IEM_MC_ASSIGN(cShiftArg, cShift);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_MEM_MAP(pu16Dst, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, cShiftArg, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu16Dst, IEM_ACCESS_DATA_RW);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint32_t *,  pu32Dst,    0);
                IEM_MC_ARG(uint8_t,     cShiftArg,  1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 1);
                uint8_t cShift; IEM_OPCODE_GET_NEXT_U8(&cShift);
                IEM_MC_ASSIGN(cShiftArg, cShift);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_MEM_MAP(pu32Dst, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, cShiftArg, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu32Dst, IEM_ACCESS_DATA_RW);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint64_t *,  pu64Dst,    0);
                IEM_MC_ARG(uint8_t,     cShiftArg,  1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 1);
                uint8_t cShift; IEM_OPCODE_GET_NEXT_U8(&cShift);
                IEM_MC_ASSIGN(cShiftArg, cShift);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_MEM_MAP(pu64Dst, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, cShiftArg, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu64Dst, IEM_ACCESS_DATA_RW);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/**
 * @opcode      0xc2
 */
FNIEMOP_DEF(iemOp_retn_Iw)
{
    IEMOP_MNEMONIC(retn_Iw, "retn Iw");
    uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(&u16Imm);
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_retn_iw_16, u16Imm);
        case IEMMODE_32BIT:
            return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_retn_iw_32, u16Imm);
        case IEMMODE_64BIT:
            return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_retn_iw_64, u16Imm);
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0xc3
 */
FNIEMOP_DEF(iemOp_retn)
{
    IEMOP_MNEMONIC(retn, "retn");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_retn_16);
        case IEMMODE_32BIT:
            return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_retn_32);
        case IEMMODE_64BIT:
            return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_retn_64);
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0xc4
 */
FNIEMOP_DEF(iemOp_les_Gv_Mp__vex3)
{
    /* The LDS instruction is invalid 64-bit mode. In legacy and
       compatability mode it is invalid with MOD=3.
       The use as a VEX prefix is made possible by assigning the inverted
       REX.R and REX.X to the two MOD bits, since the REX bits are ignored
       outside of 64-bit mode.  VEX is not available in real or v86 mode. */
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    if (   pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT
        || IEM_IS_MODRM_REG_MODE(bRm) )
    {
        IEMOP_MNEMONIC(vex3_prefix, "vex3");
        if (IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fAvx)
        {
            /* Note! The real mode, v8086 mode and invalid prefix checks are done once
                     the instruction is fully decoded.  Even when XCR0=3 and CR4.OSXSAVE=0. */
            uint8_t bVex2;   IEM_OPCODE_GET_NEXT_U8(&bVex2);
            uint8_t bOpcode; IEM_OPCODE_GET_NEXT_U8(&bOpcode);
            pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_VEX;
            if ((bVex2 & 0x80 /* VEX.W */) && pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
                pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_SIZE_REX_W;
            pVCpu->iem.s.uRexReg    = (~bRm >> (7 - 3)) & 0x8;
            pVCpu->iem.s.uRexIndex  = (~bRm >> (6 - 3)) & 0x8;
            pVCpu->iem.s.uRexB      = (~bRm >> (5 - 3)) & 0x8;
            pVCpu->iem.s.uVex3rdReg = (~bVex2 >> 3) & 0xf;
            pVCpu->iem.s.uVexLength = (bVex2 >> 2) & 1;
            pVCpu->iem.s.idxPrefix  = bVex2 & 0x3;

            switch (bRm & 0x1f)
            {
                case 1: /* 0x0f lead opcode byte. */
#ifdef IEM_WITH_VEX
                    return FNIEMOP_CALL(g_apfnVexMap1[(uintptr_t)bOpcode * 4 + pVCpu->iem.s.idxPrefix]);
#else
                    IEMOP_BITCH_ABOUT_STUB();
                    return VERR_IEM_INSTR_NOT_IMPLEMENTED;
#endif

                case 2: /* 0x0f 0x38 lead opcode bytes. */
#ifdef IEM_WITH_VEX
                    return FNIEMOP_CALL(g_apfnVexMap2[(uintptr_t)bOpcode * 4 + pVCpu->iem.s.idxPrefix]);
#else
                    IEMOP_BITCH_ABOUT_STUB();
                    return VERR_IEM_INSTR_NOT_IMPLEMENTED;
#endif

                case 3: /* 0x0f 0x3a lead opcode bytes. */
#ifdef IEM_WITH_VEX
                    return FNIEMOP_CALL(g_apfnVexMap3[(uintptr_t)bOpcode * 4 + pVCpu->iem.s.idxPrefix]);
#else
                    IEMOP_BITCH_ABOUT_STUB();
                    return VERR_IEM_INSTR_NOT_IMPLEMENTED;
#endif

                default:
                    Log(("VEX3: Invalid vvvv value: %#x!\n", bRm & 0x1f));
                    return IEMOP_RAISE_INVALID_OPCODE();
            }
        }
        Log(("VEX3: AVX support disabled!\n"));
        return IEMOP_RAISE_INVALID_OPCODE();
    }

    IEMOP_MNEMONIC(les_Gv_Mp, "les Gv,Mp");
    return FNIEMOP_CALL_2(iemOpCommonLoadSRegAndGreg, X86_SREG_ES, bRm);
}


/**
 * @opcode      0xc5
 */
FNIEMOP_DEF(iemOp_lds_Gv_Mp__vex2)
{
    /* The LES instruction is invalid 64-bit mode. In legacy and
       compatability mode it is invalid with MOD=3.
       The use as a VEX prefix is made possible by assigning the inverted
       REX.R to the top MOD bit, and the top bit in the inverted register
       specifier to the bottom MOD bit, thereby effectively limiting 32-bit
       to accessing registers 0..7 in this VEX form. */
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    if (   pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT
        || IEM_IS_MODRM_REG_MODE(bRm))
    {
        IEMOP_MNEMONIC(vex2_prefix, "vex2");
        if (IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fAvx)
        {
            /* Note! The real mode, v8086 mode and invalid prefix checks are done once
                     the instruction is fully decoded.  Even when XCR0=3 and CR4.OSXSAVE=0. */
            uint8_t bOpcode; IEM_OPCODE_GET_NEXT_U8(&bOpcode);
            pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_VEX;
            pVCpu->iem.s.uRexReg    = (~bRm >> (7 - 3)) & 0x8;
            pVCpu->iem.s.uVex3rdReg = (~bRm >> 3) & 0xf;
            pVCpu->iem.s.uVexLength = (bRm >> 2) & 1;
            pVCpu->iem.s.idxPrefix  = bRm & 0x3;

#ifdef IEM_WITH_VEX
            return FNIEMOP_CALL(g_apfnVexMap1[(uintptr_t)bOpcode * 4 + pVCpu->iem.s.idxPrefix]);
#else
            IEMOP_BITCH_ABOUT_STUB();
            return VERR_IEM_INSTR_NOT_IMPLEMENTED;
#endif
        }

        /** @todo does intel completely decode the sequence with SIB/disp before \#UD? */
        Log(("VEX2: AVX support disabled!\n"));
        return IEMOP_RAISE_INVALID_OPCODE();
    }

    IEMOP_MNEMONIC(lds_Gv_Mp, "lds Gv,Mp");
    return FNIEMOP_CALL_2(iemOpCommonLoadSRegAndGreg, X86_SREG_DS, bRm);
}


/**
 * @opcode      0xc6
 */
FNIEMOP_DEF(iemOp_Grp11_Eb_Ib)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    if ((bRm & X86_MODRM_REG_MASK) != (0 << X86_MODRM_REG_SHIFT)) /* only mov Eb,Ib in this group. */
        return IEMOP_RAISE_INVALID_OPCODE();
    IEMOP_MNEMONIC(mov_Eb_Ib, "mov Eb,Ib");

    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        /* register access */
        uint8_t u8Imm; IEM_OPCODE_GET_NEXT_U8(&u8Imm);
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_BEGIN(0, 0);
        IEM_MC_STORE_GREG_U8(IEM_GET_MODRM_RM(pVCpu, bRm), u8Imm);
        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
    else
    {
        /* memory access. */
        IEM_MC_BEGIN(0, 1);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 1);
        uint8_t u8Imm; IEM_OPCODE_GET_NEXT_U8(&u8Imm);
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_STORE_MEM_U8(pVCpu->iem.s.iEffSeg, GCPtrEffDst, u8Imm);
        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
}


/**
 * @opcode      0xc7
 */
FNIEMOP_DEF(iemOp_Grp11_Ev_Iz)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    if ((bRm & X86_MODRM_REG_MASK) != (0 << X86_MODRM_REG_SHIFT)) /* only mov Eb,Ib in this group. */
        return IEMOP_RAISE_INVALID_OPCODE();
    IEMOP_MNEMONIC(mov_Ev_Iz, "mov Ev,Iz");

    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        /* register access */
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(0, 0);
                uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(&u16Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_STORE_GREG_U16(IEM_GET_MODRM_RM(pVCpu, bRm), u16Imm);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(0, 0);
                uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(&u32Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_STORE_GREG_U32(IEM_GET_MODRM_RM(pVCpu, bRm), u32Imm);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(0, 0);
                uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S32_SX_U64(&u64Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_STORE_GREG_U64(IEM_GET_MODRM_RM(pVCpu, bRm), u64Imm);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /* memory access. */
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 2);
                uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(&u16Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_STORE_MEM_U16(pVCpu->iem.s.iEffSeg, GCPtrEffDst, u16Imm);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 4);
                uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(&u32Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_STORE_MEM_U32(pVCpu->iem.s.iEffSeg, GCPtrEffDst, u32Imm);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 4);
                uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S32_SX_U64(&u64Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_STORE_MEM_U64(pVCpu->iem.s.iEffSeg, GCPtrEffDst, u64Imm);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}




/**
 * @opcode      0xc8
 */
FNIEMOP_DEF(iemOp_enter_Iw_Ib)
{
    IEMOP_MNEMONIC(enter_Iw_Ib, "enter Iw,Ib");
    IEMOP_HLP_MIN_186();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    uint16_t cbFrame;        IEM_OPCODE_GET_NEXT_U16(&cbFrame);
    uint8_t  u8NestingLevel; IEM_OPCODE_GET_NEXT_U8(&u8NestingLevel);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_3(iemCImpl_enter, pVCpu->iem.s.enmEffOpSize, cbFrame, u8NestingLevel);
}


/**
 * @opcode      0xc9
 */
FNIEMOP_DEF(iemOp_leave)
{
    IEMOP_MNEMONIC(leave, "leave");
    IEMOP_HLP_MIN_186();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_leave, pVCpu->iem.s.enmEffOpSize);
}


/**
 * @opcode      0xca
 */
FNIEMOP_DEF(iemOp_retf_Iw)
{
    IEMOP_MNEMONIC(retf_Iw, "retf Iw");
    uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(&u16Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_retf, pVCpu->iem.s.enmEffOpSize, u16Imm);
}


/**
 * @opcode      0xcb
 */
FNIEMOP_DEF(iemOp_retf)
{
    IEMOP_MNEMONIC(retf, "retf");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_retf, pVCpu->iem.s.enmEffOpSize, 0);
}


/**
 * @opcode      0xcc
 */
FNIEMOP_DEF(iemOp_int3)
{
    IEMOP_MNEMONIC(int3, "int3");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_int, X86_XCPT_BP, IEMINT_INT3);
}


/**
 * @opcode      0xcd
 */
FNIEMOP_DEF(iemOp_int_Ib)
{
    IEMOP_MNEMONIC(int_Ib, "int Ib");
    uint8_t u8Int; IEM_OPCODE_GET_NEXT_U8(&u8Int);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_int, u8Int, IEMINT_INTN);
}


/**
 * @opcode      0xce
 */
FNIEMOP_DEF(iemOp_into)
{
    IEMOP_MNEMONIC(into, "into");
    IEMOP_HLP_NO_64BIT();

    IEM_MC_BEGIN(2, 0);
    IEM_MC_ARG_CONST(uint8_t,   u8Int,  /*=*/ X86_XCPT_OF, 0);
    IEM_MC_ARG_CONST(IEMINT,    enmInt, /*=*/ IEMINT_INTO, 1);
    IEM_MC_CALL_CIMPL_2(iemCImpl_int, u8Int, enmInt);
    IEM_MC_END();
    return VINF_SUCCESS;
}


/**
 * @opcode      0xcf
 */
FNIEMOP_DEF(iemOp_iret)
{
    IEMOP_MNEMONIC(iret, "iret");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_iret, pVCpu->iem.s.enmEffOpSize);
}


/**
 * @opcode      0xd0
 */
FNIEMOP_DEF(iemOp_Grp2_Eb_1)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    PCIEMOPSHIFTSIZES pImpl;
    switch (IEM_GET_MODRM_REG_8(bRm))
    {
        case 0: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_rol_eflags); IEMOP_MNEMONIC(rol_Eb_1, "rol Eb,1"); break;
        case 1: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_ror_eflags); IEMOP_MNEMONIC(ror_Eb_1, "ror Eb,1"); break;
        case 2: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_rcl_eflags); IEMOP_MNEMONIC(rcl_Eb_1, "rcl Eb,1"); break;
        case 3: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_rcr_eflags); IEMOP_MNEMONIC(rcr_Eb_1, "rcr Eb,1"); break;
        case 4: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_shl_eflags); IEMOP_MNEMONIC(shl_Eb_1, "shl Eb,1"); break;
        case 5: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_shr_eflags); IEMOP_MNEMONIC(shr_Eb_1, "shr Eb,1"); break;
        case 7: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_sar_eflags); IEMOP_MNEMONIC(sar_Eb_1, "sar Eb,1"); break;
        case 6: return IEMOP_RAISE_INVALID_OPCODE();
        IEM_NOT_REACHED_DEFAULT_CASE_RET(); /* gcc maybe, well... */
    }
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_OF | X86_EFL_AF);

    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        /* register */
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_BEGIN(3, 0);
        IEM_MC_ARG(uint8_t *,       pu8Dst,             0);
        IEM_MC_ARG_CONST(uint8_t,   cShiftArg,/*=*/1,   1);
        IEM_MC_ARG(uint32_t *,      pEFlags,            2);
        IEM_MC_REF_GREG_U8(pu8Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, cShiftArg, pEFlags);
        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
    else
    {
        /* memory */
        IEM_MC_BEGIN(3, 2);
        IEM_MC_ARG(uint8_t *,       pu8Dst,             0);
        IEM_MC_ARG_CONST(uint8_t,   cShiftArg,/*=*/1,   1);
        IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags,        2);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_MEM_MAP(pu8Dst, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
        IEM_MC_FETCH_EFLAGS(EFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, cShiftArg, pEFlags);

        IEM_MC_MEM_COMMIT_AND_UNMAP(pu8Dst, IEM_ACCESS_DATA_RW);
        IEM_MC_COMMIT_EFLAGS(EFlags);
        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
}



/**
 * @opcode      0xd1
 */
FNIEMOP_DEF(iemOp_Grp2_Ev_1)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    PCIEMOPSHIFTSIZES pImpl;
    switch (IEM_GET_MODRM_REG_8(bRm))
    {
        case 0: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_rol_eflags); IEMOP_MNEMONIC(rol_Ev_1, "rol Ev,1"); break;
        case 1: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_ror_eflags); IEMOP_MNEMONIC(ror_Ev_1, "ror Ev,1"); break;
        case 2: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_rcl_eflags); IEMOP_MNEMONIC(rcl_Ev_1, "rcl Ev,1"); break;
        case 3: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_rcr_eflags); IEMOP_MNEMONIC(rcr_Ev_1, "rcr Ev,1"); break;
        case 4: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_shl_eflags); IEMOP_MNEMONIC(shl_Ev_1, "shl Ev,1"); break;
        case 5: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_shr_eflags); IEMOP_MNEMONIC(shr_Ev_1, "shr Ev,1"); break;
        case 7: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_sar_eflags); IEMOP_MNEMONIC(sar_Ev_1, "sar Ev,1"); break;
        case 6: return IEMOP_RAISE_INVALID_OPCODE();
        IEM_NOT_REACHED_DEFAULT_CASE_RET(); /* gcc maybe, well... */
    }
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_OF | X86_EFL_AF);

    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        /* register */
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint16_t *,      pu16Dst,           0);
                IEM_MC_ARG_CONST(uint8_t,   cShiftArg,/*=1*/1, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,           2);
                IEM_MC_REF_GREG_U16(pu16Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, cShiftArg, pEFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint32_t *,      pu32Dst,           0);
                IEM_MC_ARG_CONST(uint8_t,   cShiftArg,/*=1*/1, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,           2);
                IEM_MC_REF_GREG_U32(pu32Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, cShiftArg, pEFlags);
                IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(pu32Dst);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint64_t *,      pu64Dst,           0);
                IEM_MC_ARG_CONST(uint8_t,   cShiftArg,/*=1*/1, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,           2);
                IEM_MC_REF_GREG_U64(pu64Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, cShiftArg, pEFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /* memory */
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint16_t *,      pu16Dst,            0);
                IEM_MC_ARG_CONST(uint8_t,   cShiftArg,/*=1*/1,  1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags,        2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_MEM_MAP(pu16Dst, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, cShiftArg, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu16Dst, IEM_ACCESS_DATA_RW);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint32_t *,      pu32Dst,            0);
                IEM_MC_ARG_CONST(uint8_t,   cShiftArg,/*=1*/1,  1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags,        2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_MEM_MAP(pu32Dst, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, cShiftArg, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu32Dst, IEM_ACCESS_DATA_RW);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint64_t *,      pu64Dst,            0);
                IEM_MC_ARG_CONST(uint8_t,   cShiftArg,/*=1*/1,  1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags,        2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_MEM_MAP(pu64Dst, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, cShiftArg, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu64Dst, IEM_ACCESS_DATA_RW);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/**
 * @opcode      0xd2
 */
FNIEMOP_DEF(iemOp_Grp2_Eb_CL)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    PCIEMOPSHIFTSIZES pImpl;
    switch (IEM_GET_MODRM_REG_8(bRm))
    {
        case 0: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_rol_eflags); IEMOP_MNEMONIC(rol_Eb_CL, "rol Eb,CL"); break;
        case 1: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_ror_eflags); IEMOP_MNEMONIC(ror_Eb_CL, "ror Eb,CL"); break;
        case 2: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_rcl_eflags); IEMOP_MNEMONIC(rcl_Eb_CL, "rcl Eb,CL"); break;
        case 3: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_rcr_eflags); IEMOP_MNEMONIC(rcr_Eb_CL, "rcr Eb,CL"); break;
        case 4: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_shl_eflags); IEMOP_MNEMONIC(shl_Eb_CL, "shl Eb,CL"); break;
        case 5: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_shr_eflags); IEMOP_MNEMONIC(shr_Eb_CL, "shr Eb,CL"); break;
        case 7: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_sar_eflags); IEMOP_MNEMONIC(sar_Eb_CL, "sar Eb,CL"); break;
        case 6: return IEMOP_RAISE_INVALID_OPCODE();
        IEM_NOT_REACHED_DEFAULT_CASE_RET(); /* gcc, grr. */
    }
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_OF | X86_EFL_AF);

    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        /* register */
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_BEGIN(3, 0);
        IEM_MC_ARG(uint8_t *,   pu8Dst,     0);
        IEM_MC_ARG(uint8_t,     cShiftArg,  1);
        IEM_MC_ARG(uint32_t *,  pEFlags,    2);
        IEM_MC_REF_GREG_U8(pu8Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
        IEM_MC_FETCH_GREG_U8(cShiftArg, X86_GREG_xCX);
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, cShiftArg, pEFlags);
        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
    else
    {
        /* memory */
        IEM_MC_BEGIN(3, 2);
        IEM_MC_ARG(uint8_t *,   pu8Dst,          0);
        IEM_MC_ARG(uint8_t,     cShiftArg,       1);
        IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_FETCH_GREG_U8(cShiftArg, X86_GREG_xCX);
        IEM_MC_MEM_MAP(pu8Dst, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
        IEM_MC_FETCH_EFLAGS(EFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, cShiftArg, pEFlags);

        IEM_MC_MEM_COMMIT_AND_UNMAP(pu8Dst, IEM_ACCESS_DATA_RW);
        IEM_MC_COMMIT_EFLAGS(EFlags);
        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
}


/**
 * @opcode      0xd3
 */
FNIEMOP_DEF(iemOp_Grp2_Ev_CL)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    PCIEMOPSHIFTSIZES pImpl;
    switch (IEM_GET_MODRM_REG_8(bRm))
    {
        case 0: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_rol_eflags); IEMOP_MNEMONIC(rol_Ev_CL, "rol Ev,CL"); break;
        case 1: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_ror_eflags); IEMOP_MNEMONIC(ror_Ev_CL, "ror Ev,CL"); break;
        case 2: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_rcl_eflags); IEMOP_MNEMONIC(rcl_Ev_CL, "rcl Ev,CL"); break;
        case 3: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_rcr_eflags); IEMOP_MNEMONIC(rcr_Ev_CL, "rcr Ev,CL"); break;
        case 4: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_shl_eflags); IEMOP_MNEMONIC(shl_Ev_CL, "shl Ev,CL"); break;
        case 5: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_shr_eflags); IEMOP_MNEMONIC(shr_Ev_CL, "shr Ev,CL"); break;
        case 7: pImpl = IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_sar_eflags); IEMOP_MNEMONIC(sar_Ev_CL, "sar Ev,CL"); break;
        case 6: return IEMOP_RAISE_INVALID_OPCODE();
        IEM_NOT_REACHED_DEFAULT_CASE_RET(); /* gcc maybe stupid */
    }
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_OF | X86_EFL_AF);

    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        /* register */
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint16_t *,      pu16Dst,    0);
                IEM_MC_ARG(uint8_t,         cShiftArg,  1);
                IEM_MC_ARG(uint32_t *,      pEFlags,    2);
                IEM_MC_REF_GREG_U16(pu16Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_FETCH_GREG_U8(cShiftArg, X86_GREG_xCX);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, cShiftArg, pEFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint32_t *,      pu32Dst,    0);
                IEM_MC_ARG(uint8_t,         cShiftArg,  1);
                IEM_MC_ARG(uint32_t *,      pEFlags,    2);
                IEM_MC_REF_GREG_U32(pu32Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_FETCH_GREG_U8(cShiftArg, X86_GREG_xCX);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, cShiftArg, pEFlags);
                IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(pu32Dst);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint64_t *,      pu64Dst,    0);
                IEM_MC_ARG(uint8_t,         cShiftArg,  1);
                IEM_MC_ARG(uint32_t *,      pEFlags,    2);
                IEM_MC_REF_GREG_U64(pu64Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_FETCH_GREG_U8(cShiftArg, X86_GREG_xCX);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, cShiftArg, pEFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /* memory */
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint16_t *,  pu16Dst,    0);
                IEM_MC_ARG(uint8_t,     cShiftArg,  1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_GREG_U8(cShiftArg, X86_GREG_xCX);
                IEM_MC_MEM_MAP(pu16Dst, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, cShiftArg, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu16Dst, IEM_ACCESS_DATA_RW);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint32_t *,  pu32Dst,    0);
                IEM_MC_ARG(uint8_t,     cShiftArg,  1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_GREG_U8(cShiftArg, X86_GREG_xCX);
                IEM_MC_MEM_MAP(pu32Dst, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, cShiftArg, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu32Dst, IEM_ACCESS_DATA_RW);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint64_t *,  pu64Dst,    0);
                IEM_MC_ARG(uint8_t,     cShiftArg,  1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_GREG_U8(cShiftArg, X86_GREG_xCX);
                IEM_MC_MEM_MAP(pu64Dst, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, cShiftArg, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu64Dst, IEM_ACCESS_DATA_RW);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}

/**
 * @opcode      0xd4
 */
FNIEMOP_DEF(iemOp_aam_Ib)
{
    IEMOP_MNEMONIC(aam_Ib, "aam Ib");
    uint8_t bImm; IEM_OPCODE_GET_NEXT_U8(&bImm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_NO_64BIT();
    if (!bImm)
        return IEMOP_RAISE_DIVIDE_ERROR();
    return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_aam, bImm);
}


/**
 * @opcode      0xd5
 */
FNIEMOP_DEF(iemOp_aad_Ib)
{
    IEMOP_MNEMONIC(aad_Ib, "aad Ib");
    uint8_t bImm; IEM_OPCODE_GET_NEXT_U8(&bImm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_NO_64BIT();
    return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_aad, bImm);
}


/**
 * @opcode      0xd6
 */
FNIEMOP_DEF(iemOp_salc)
{
    IEMOP_MNEMONIC(salc, "salc");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_NO_64BIT();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_CF) {
        IEM_MC_STORE_GREG_U8_CONST(X86_GREG_xAX, 0xff);
    } IEM_MC_ELSE() {
        IEM_MC_STORE_GREG_U8_CONST(X86_GREG_xAX, 0x00);
    } IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/**
 * @opcode      0xd7
 */
FNIEMOP_DEF(iemOp_xlat)
{
    IEMOP_MNEMONIC(xlat, "xlat");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    switch (pVCpu->iem.s.enmEffAddrMode)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(2, 0);
            IEM_MC_LOCAL(uint8_t,  u8Tmp);
            IEM_MC_LOCAL(uint16_t, u16Addr);
            IEM_MC_FETCH_GREG_U8_ZX_U16(u16Addr, X86_GREG_xAX);
            IEM_MC_ADD_GREG_U16_TO_LOCAL(u16Addr, X86_GREG_xBX);
            IEM_MC_FETCH_MEM16_U8(u8Tmp, pVCpu->iem.s.iEffSeg, u16Addr);
            IEM_MC_STORE_GREG_U8(X86_GREG_xAX, u8Tmp);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(2, 0);
            IEM_MC_LOCAL(uint8_t,  u8Tmp);
            IEM_MC_LOCAL(uint32_t, u32Addr);
            IEM_MC_FETCH_GREG_U8_ZX_U32(u32Addr, X86_GREG_xAX);
            IEM_MC_ADD_GREG_U32_TO_LOCAL(u32Addr, X86_GREG_xBX);
            IEM_MC_FETCH_MEM32_U8(u8Tmp, pVCpu->iem.s.iEffSeg, u32Addr);
            IEM_MC_STORE_GREG_U8(X86_GREG_xAX, u8Tmp);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(2, 0);
            IEM_MC_LOCAL(uint8_t,  u8Tmp);
            IEM_MC_LOCAL(uint64_t, u64Addr);
            IEM_MC_FETCH_GREG_U8_ZX_U64(u64Addr, X86_GREG_xAX);
            IEM_MC_ADD_GREG_U64_TO_LOCAL(u64Addr, X86_GREG_xBX);
            IEM_MC_FETCH_MEM_U8(u8Tmp, pVCpu->iem.s.iEffSeg, u64Addr);
            IEM_MC_STORE_GREG_U8(X86_GREG_xAX, u8Tmp);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

         IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * Common worker for FPU instructions working on ST0 and STn, and storing the
 * result in ST0.
 *
 * @param   bRm         Mod R/M byte.
 * @param   pfnAImpl    Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_2(iemOpHlpFpu_st0_stN, uint8_t, bRm, PFNIEMAIMPLFPUR80, pfnAImpl)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(3, 1);
    IEM_MC_LOCAL(IEMFPURESULT,          FpuRes);
    IEM_MC_ARG_LOCAL_REF(PIEMFPURESULT, pFpuRes,        FpuRes,     0);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value1,                 1);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value2,                 2);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80(pr80Value1, 0, pr80Value2, IEM_GET_MODRM_RM_8(bRm))
        IEM_MC_CALL_FPU_AIMPL_3(pfnAImpl, pFpuRes, pr80Value1, pr80Value2);
        IEM_MC_STORE_FPU_RESULT(FpuRes, 0);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW(0);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/**
 * Common worker for FPU instructions working on ST0 and STn, and only affecting
 * flags.
 *
 * @param   bRm         Mod R/M byte.
 * @param   pfnAImpl    Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_2(iemOpHlpFpuNoStore_st0_stN, uint8_t, bRm, PFNIEMAIMPLFPUR80FSW, pfnAImpl)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(3, 1);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,        u16Fsw,     0);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value1,                 1);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value2,                 2);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80(pr80Value1, 0, pr80Value2, IEM_GET_MODRM_RM_8(bRm))
        IEM_MC_CALL_FPU_AIMPL_3(pfnAImpl, pu16Fsw, pr80Value1, pr80Value2);
        IEM_MC_UPDATE_FSW(u16Fsw);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW(UINT8_MAX);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/**
 * Common worker for FPU instructions working on ST0 and STn, only affecting
 * flags, and popping when done.
 *
 * @param   bRm         Mod R/M byte.
 * @param   pfnAImpl    Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_2(iemOpHlpFpuNoStore_st0_stN_pop, uint8_t, bRm, PFNIEMAIMPLFPUR80FSW, pfnAImpl)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(3, 1);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,        u16Fsw,     0);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value1,                 1);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value2,                 2);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80(pr80Value1, 0, pr80Value2, IEM_GET_MODRM_RM_8(bRm))
        IEM_MC_CALL_FPU_AIMPL_3(pfnAImpl, pu16Fsw, pr80Value1, pr80Value2);
        IEM_MC_UPDATE_FSW_THEN_POP(u16Fsw);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW_THEN_POP(UINT8_MAX);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xd8 11/0. */
FNIEMOP_DEF_1(iemOp_fadd_stN,   uint8_t, bRm)
{
    IEMOP_MNEMONIC(fadd_st0_stN, "fadd st0,stN");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_stN, bRm, iemAImpl_fadd_r80_by_r80);
}


/** Opcode 0xd8 11/1. */
FNIEMOP_DEF_1(iemOp_fmul_stN,   uint8_t, bRm)
{
    IEMOP_MNEMONIC(fmul_st0_stN, "fmul st0,stN");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_stN, bRm, iemAImpl_fmul_r80_by_r80);
}


/** Opcode 0xd8 11/2. */
FNIEMOP_DEF_1(iemOp_fcom_stN,   uint8_t, bRm)
{
    IEMOP_MNEMONIC(fcom_st0_stN, "fcom st0,stN");
    return FNIEMOP_CALL_2(iemOpHlpFpuNoStore_st0_stN, bRm, iemAImpl_fcom_r80_by_r80);
}


/** Opcode 0xd8 11/3. */
FNIEMOP_DEF_1(iemOp_fcomp_stN,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fcomp_st0_stN, "fcomp st0,stN");
    return FNIEMOP_CALL_2(iemOpHlpFpuNoStore_st0_stN_pop, bRm, iemAImpl_fcom_r80_by_r80);
}


/** Opcode 0xd8 11/4. */
FNIEMOP_DEF_1(iemOp_fsub_stN,   uint8_t, bRm)
{
    IEMOP_MNEMONIC(fsub_st0_stN, "fsub st0,stN");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_stN, bRm, iemAImpl_fsub_r80_by_r80);
}


/** Opcode 0xd8 11/5. */
FNIEMOP_DEF_1(iemOp_fsubr_stN,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fsubr_st0_stN, "fsubr st0,stN");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_stN, bRm, iemAImpl_fsubr_r80_by_r80);
}


/** Opcode 0xd8 11/6. */
FNIEMOP_DEF_1(iemOp_fdiv_stN,   uint8_t, bRm)
{
    IEMOP_MNEMONIC(fdiv_st0_stN, "fdiv st0,stN");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_stN, bRm, iemAImpl_fdiv_r80_by_r80);
}


/** Opcode 0xd8 11/7. */
FNIEMOP_DEF_1(iemOp_fdivr_stN,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fdivr_st0_stN, "fdivr st0,stN");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_stN, bRm, iemAImpl_fdivr_r80_by_r80);
}


/**
 * Common worker for FPU instructions working on ST0 and an m32r, and storing
 * the result in ST0.
 *
 * @param   bRm         Mod R/M byte.
 * @param   pfnAImpl    Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_2(iemOpHlpFpu_st0_m32r, uint8_t, bRm, PFNIEMAIMPLFPUR32, pfnAImpl)
{
    IEM_MC_BEGIN(3, 3);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffSrc);
    IEM_MC_LOCAL(IEMFPURESULT,          FpuRes);
    IEM_MC_LOCAL(RTFLOAT32U,            r32Val2);
    IEM_MC_ARG_LOCAL_REF(PIEMFPURESULT, pFpuRes,        FpuRes,     0);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value1,                 1);
    IEM_MC_ARG_LOCAL_REF(PCRTFLOAT32U,  pr32Val2,       r32Val2,    2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_FETCH_MEM_R32(r32Val2, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value1, 0)
        IEM_MC_CALL_FPU_AIMPL_3(pfnAImpl, pFpuRes, pr80Value1, pr32Val2);
        IEM_MC_STORE_FPU_RESULT(FpuRes, 0);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW(0);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xd8 !11/0. */
FNIEMOP_DEF_1(iemOp_fadd_m32r,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fadd_st0_m32r, "fadd st0,m32r");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_m32r, bRm, iemAImpl_fadd_r80_by_r32);
}


/** Opcode 0xd8 !11/1. */
FNIEMOP_DEF_1(iemOp_fmul_m32r,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fmul_st0_m32r, "fmul st0,m32r");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_m32r, bRm, iemAImpl_fmul_r80_by_r32);
}


/** Opcode 0xd8 !11/2. */
FNIEMOP_DEF_1(iemOp_fcom_m32r,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fcom_st0_m32r, "fcom st0,m32r");

    IEM_MC_BEGIN(3, 3);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffSrc);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_LOCAL(RTFLOAT32U,            r32Val2);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,        u16Fsw,     0);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value1,                 1);
    IEM_MC_ARG_LOCAL_REF(PCRTFLOAT32U,  pr32Val2,       r32Val2,    2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_FETCH_MEM_R32(r32Val2, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value1, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_fcom_r80_by_r32, pu16Fsw, pr80Value1, pr32Val2);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xd8 !11/3. */
FNIEMOP_DEF_1(iemOp_fcomp_m32r, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fcomp_st0_m32r, "fcomp st0,m32r");

    IEM_MC_BEGIN(3, 3);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffSrc);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_LOCAL(RTFLOAT32U,            r32Val2);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,        u16Fsw,     0);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value1,                 1);
    IEM_MC_ARG_LOCAL_REF(PCRTFLOAT32U,  pr32Val2,       r32Val2,    2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_FETCH_MEM_R32(r32Val2, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value1, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_fcom_r80_by_r32, pu16Fsw, pr80Value1, pr32Val2);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP_THEN_POP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP_THEN_POP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xd8 !11/4. */
FNIEMOP_DEF_1(iemOp_fsub_m32r,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fsub_st0_m32r, "fsub st0,m32r");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_m32r, bRm, iemAImpl_fsub_r80_by_r32);
}


/** Opcode 0xd8 !11/5. */
FNIEMOP_DEF_1(iemOp_fsubr_m32r, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fsubr_st0_m32r, "fsubr st0,m32r");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_m32r, bRm, iemAImpl_fsubr_r80_by_r32);
}


/** Opcode 0xd8 !11/6. */
FNIEMOP_DEF_1(iemOp_fdiv_m32r,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fdiv_st0_m32r, "fdiv st0,m32r");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_m32r, bRm, iemAImpl_fdiv_r80_by_r32);
}


/** Opcode 0xd8 !11/7. */
FNIEMOP_DEF_1(iemOp_fdivr_m32r, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fdivr_st0_m32r, "fdivr st0,m32r");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_m32r, bRm, iemAImpl_fdivr_r80_by_r32);
}


/**
 * @opcode      0xd8
 */
FNIEMOP_DEF(iemOp_EscF0)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    pVCpu->iem.s.uFpuOpcode = RT_MAKE_U16(bRm, 0xd8 & 0x7);

    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        switch (IEM_GET_MODRM_REG_8(bRm))
        {
            case 0: return FNIEMOP_CALL_1(iemOp_fadd_stN,  bRm);
            case 1: return FNIEMOP_CALL_1(iemOp_fmul_stN,  bRm);
            case 2: return FNIEMOP_CALL_1(iemOp_fcom_stN,  bRm);
            case 3: return FNIEMOP_CALL_1(iemOp_fcomp_stN, bRm);
            case 4: return FNIEMOP_CALL_1(iemOp_fsub_stN,  bRm);
            case 5: return FNIEMOP_CALL_1(iemOp_fsubr_stN, bRm);
            case 6: return FNIEMOP_CALL_1(iemOp_fdiv_stN,  bRm);
            case 7: return FNIEMOP_CALL_1(iemOp_fdivr_stN, bRm);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        switch (IEM_GET_MODRM_REG_8(bRm))
        {
            case 0: return FNIEMOP_CALL_1(iemOp_fadd_m32r,  bRm);
            case 1: return FNIEMOP_CALL_1(iemOp_fmul_m32r,  bRm);
            case 2: return FNIEMOP_CALL_1(iemOp_fcom_m32r,  bRm);
            case 3: return FNIEMOP_CALL_1(iemOp_fcomp_m32r, bRm);
            case 4: return FNIEMOP_CALL_1(iemOp_fsub_m32r,  bRm);
            case 5: return FNIEMOP_CALL_1(iemOp_fsubr_m32r, bRm);
            case 6: return FNIEMOP_CALL_1(iemOp_fdiv_m32r,  bRm);
            case 7: return FNIEMOP_CALL_1(iemOp_fdivr_m32r, bRm);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/** Opcode 0xd9 /0 mem32real
 * @sa  iemOp_fld_m64r */
FNIEMOP_DEF_1(iemOp_fld_m32r, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fld_m32r, "fld m32r");

    IEM_MC_BEGIN(2, 3);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffSrc);
    IEM_MC_LOCAL(IEMFPURESULT,          FpuRes);
    IEM_MC_LOCAL(RTFLOAT32U,            r32Val);
    IEM_MC_ARG_LOCAL_REF(PIEMFPURESULT, pFpuRes,    FpuRes, 0);
    IEM_MC_ARG_LOCAL_REF(PCRTFLOAT32U,  pr32Val,    r32Val, 1);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_FETCH_MEM_R32(r32Val, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_IS_EMPTY(7)
        IEM_MC_CALL_FPU_AIMPL_2(iemAImpl_fld_r80_from_r32, pFpuRes, pr32Val);
        IEM_MC_PUSH_FPU_RESULT_MEM_OP(FpuRes, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_PUSH_OVERFLOW_MEM_OP(pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xd9 !11/2 mem32real */
FNIEMOP_DEF_1(iemOp_fst_m32r, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fst_m32r, "fst m32r");
    IEM_MC_BEGIN(3, 2);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffDst);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,    u16Fsw, 0);
    IEM_MC_ARG(PRTFLOAT32U,             pr32Dst,            1);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value,          2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_MEM_MAP(pr32Dst, IEM_ACCESS_DATA_W, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 1 /*arg*/);
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_fst_r80_to_r32, pu16Fsw, pr32Dst, pr80Value);
        IEM_MC_MEM_COMMIT_AND_UNMAP_FOR_FPU_STORE(pr32Dst, IEM_ACCESS_DATA_W, u16Fsw);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ELSE()
        IEM_MC_IF_FCW_IM()
            IEM_MC_STORE_MEM_NEG_QNAN_R32_BY_REF(pr32Dst);
            IEM_MC_MEM_COMMIT_AND_UNMAP(pr32Dst, IEM_ACCESS_DATA_W);
        IEM_MC_ENDIF();
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xd9 !11/3 */
FNIEMOP_DEF_1(iemOp_fstp_m32r, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fstp_m32r, "fstp m32r");
    IEM_MC_BEGIN(3, 2);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffDst);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,    u16Fsw, 0);
    IEM_MC_ARG(PRTFLOAT32U,             pr32Dst,            1);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value,          2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_MEM_MAP(pr32Dst, IEM_ACCESS_DATA_W, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 1 /*arg*/);
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_fst_r80_to_r32, pu16Fsw, pr32Dst, pr80Value);
        IEM_MC_MEM_COMMIT_AND_UNMAP_FOR_FPU_STORE(pr32Dst, IEM_ACCESS_DATA_W, u16Fsw);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP_THEN_POP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ELSE()
        IEM_MC_IF_FCW_IM()
            IEM_MC_STORE_MEM_NEG_QNAN_R32_BY_REF(pr32Dst);
            IEM_MC_MEM_COMMIT_AND_UNMAP(pr32Dst, IEM_ACCESS_DATA_W);
        IEM_MC_ENDIF();
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP_THEN_POP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xd9 !11/4 */
FNIEMOP_DEF_1(iemOp_fldenv, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fldenv, "fldenv m14/28byte");
    IEM_MC_BEGIN(3, 0);
    IEM_MC_ARG_CONST(IEMMODE,           enmEffOpSize, /*=*/ pVCpu->iem.s.enmEffOpSize,  0);
    IEM_MC_ARG(uint8_t,                 iEffSeg,                                    1);
    IEM_MC_ARG(RTGCPTR,                 GCPtrEffSrc,                                2);
    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_ACTUALIZE_FPU_STATE_FOR_CHANGE();
    IEM_MC_ASSIGN(iEffSeg, pVCpu->iem.s.iEffSeg);
    IEM_MC_CALL_CIMPL_3(iemCImpl_fldenv, enmEffOpSize, iEffSeg, GCPtrEffSrc);
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0xd9 !11/5 */
FNIEMOP_DEF_1(iemOp_fldcw, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fldcw_m2byte, "fldcw m2byte");
    IEM_MC_BEGIN(1, 1);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffSrc);
    IEM_MC_ARG(uint16_t,                u16Fsw,                                     0);
    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_ACTUALIZE_FPU_STATE_FOR_CHANGE();
    IEM_MC_FETCH_MEM_U16(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_CALL_CIMPL_1(iemCImpl_fldcw, u16Fsw);
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0xd9 !11/6 */
FNIEMOP_DEF_1(iemOp_fnstenv, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fstenv, "fstenv m14/m28byte");
    IEM_MC_BEGIN(3, 0);
    IEM_MC_ARG_CONST(IEMMODE,           enmEffOpSize, /*=*/ pVCpu->iem.s.enmEffOpSize,  0);
    IEM_MC_ARG(uint8_t,                 iEffSeg,                                    1);
    IEM_MC_ARG(RTGCPTR,                 GCPtrEffDst,                                2);
    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_ACTUALIZE_FPU_STATE_FOR_READ();
    IEM_MC_ASSIGN(iEffSeg, pVCpu->iem.s.iEffSeg);
    IEM_MC_CALL_CIMPL_3(iemCImpl_fnstenv, enmEffOpSize, iEffSeg, GCPtrEffDst);
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0xd9 !11/7 */
FNIEMOP_DEF_1(iemOp_fnstcw, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fnstcw_m2byte, "fnstcw m2byte");
    IEM_MC_BEGIN(2, 0);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffDst);
    IEM_MC_LOCAL(uint16_t,              u16Fcw);
    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_ACTUALIZE_FPU_STATE_FOR_READ();
    IEM_MC_FETCH_FCW(u16Fcw);
    IEM_MC_STORE_MEM_U16(pVCpu->iem.s.iEffSeg, GCPtrEffDst, u16Fcw);
    IEM_MC_ADVANCE_RIP_AND_FINISH(); /* C0-C3 are documented as undefined, we leave them unmodified. */
    IEM_MC_END();
}


/** Opcode 0xd9 0xd0, 0xd9 0xd8-0xdf, ++?.  */
FNIEMOP_DEF(iemOp_fnop)
{
    IEMOP_MNEMONIC(fnop, "fnop");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_ACTUALIZE_FPU_STATE_FOR_CHANGE();
    /** @todo Testcase: looks like FNOP leaves FOP alone but updates FPUIP. Could be
     *        intel optimizations. Investigate. */
    IEM_MC_UPDATE_FPU_OPCODE_IP();
    IEM_MC_ADVANCE_RIP_AND_FINISH(); /* C0-C3 are documented as undefined, we leave them unmodified. */
    IEM_MC_END();
}


/** Opcode 0xd9 11/0 stN */
FNIEMOP_DEF_1(iemOp_fld_stN, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fld_stN, "fld stN");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    /** @todo Testcase: Check if this raises \#MF?  Intel mentioned it not. AMD
     *        indicates that it does. */
    IEM_MC_BEGIN(0, 2);
    IEM_MC_LOCAL(PCRTFLOAT80U,          pr80Value);
    IEM_MC_LOCAL(IEMFPURESULT,          FpuRes);
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value, IEM_GET_MODRM_RM_8(bRm))
        IEM_MC_SET_FPU_RESULT(FpuRes, 0 /*FSW*/, pr80Value);
        IEM_MC_PUSH_FPU_RESULT(FpuRes);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_PUSH_UNDERFLOW();
    IEM_MC_ENDIF();

    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/** Opcode 0xd9 11/3 stN */
FNIEMOP_DEF_1(iemOp_fxch_stN, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fxch_stN, "fxch stN");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    /** @todo Testcase: Check if this raises \#MF?  Intel mentioned it not. AMD
     *        indicates that it does. */
    IEM_MC_BEGIN(1, 3);
    IEM_MC_LOCAL(PCRTFLOAT80U,          pr80Value1);
    IEM_MC_LOCAL(PCRTFLOAT80U,          pr80Value2);
    IEM_MC_LOCAL(IEMFPURESULT,          FpuRes);
    IEM_MC_ARG_CONST(uint8_t,           iStReg, /*=*/ IEM_GET_MODRM_RM_8(bRm), 0);
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80(pr80Value1, 0, pr80Value2, IEM_GET_MODRM_RM_8(bRm))
        IEM_MC_SET_FPU_RESULT(FpuRes, X86_FSW_C1, pr80Value2);
        IEM_MC_STORE_FPUREG_R80_SRC_REF(IEM_GET_MODRM_RM_8(bRm), pr80Value1);
        IEM_MC_STORE_FPU_RESULT(FpuRes, 0);
    IEM_MC_ELSE()
        IEM_MC_CALL_CIMPL_1(iemCImpl_fxch_underflow, iStReg);
    IEM_MC_ENDIF();

    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/** Opcode 0xd9 11/4, 0xdd 11/2. */
FNIEMOP_DEF_1(iemOp_fstp_stN, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fstp_st0_stN, "fstp st0,stN");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    /* fstp st0, st0 is frequently used as an official 'ffreep st0' sequence. */
    uint8_t const iDstReg = IEM_GET_MODRM_RM_8(bRm);
    if (!iDstReg)
    {
        IEM_MC_BEGIN(0, 1);
        IEM_MC_LOCAL_CONST(uint16_t,        u16Fsw, /*=*/ 0);
        IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
        IEM_MC_MAYBE_RAISE_FPU_XCPT();

        IEM_MC_PREPARE_FPU_USAGE();
        IEM_MC_IF_FPUREG_NOT_EMPTY(0)
            IEM_MC_UPDATE_FSW_THEN_POP(u16Fsw);
        IEM_MC_ELSE()
            IEM_MC_FPU_STACK_UNDERFLOW_THEN_POP(0);
        IEM_MC_ENDIF();

        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
    else
    {
        IEM_MC_BEGIN(0, 2);
        IEM_MC_LOCAL(PCRTFLOAT80U,          pr80Value);
        IEM_MC_LOCAL(IEMFPURESULT,          FpuRes);
        IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
        IEM_MC_MAYBE_RAISE_FPU_XCPT();

        IEM_MC_PREPARE_FPU_USAGE();
        IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value, 0)
            IEM_MC_SET_FPU_RESULT(FpuRes, 0 /*FSW*/, pr80Value);
            IEM_MC_STORE_FPU_RESULT_THEN_POP(FpuRes, iDstReg);
        IEM_MC_ELSE()
            IEM_MC_FPU_STACK_UNDERFLOW_THEN_POP(iDstReg);
        IEM_MC_ENDIF();

        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
}


/**
 * Common worker for FPU instructions working on ST0 and replaces it with the
 * result, i.e. unary operators.
 *
 * @param   pfnAImpl    Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpFpu_st0, PFNIEMAIMPLFPUR80UNARY, pfnAImpl)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(2, 1);
    IEM_MC_LOCAL(IEMFPURESULT,          FpuRes);
    IEM_MC_ARG_LOCAL_REF(PIEMFPURESULT, pFpuRes,    FpuRes, 0);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value,          1);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value, 0)
        IEM_MC_CALL_FPU_AIMPL_2(pfnAImpl, pFpuRes, pr80Value);
        IEM_MC_STORE_FPU_RESULT(FpuRes, 0);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW(0);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xd9 0xe0. */
FNIEMOP_DEF(iemOp_fchs)
{
    IEMOP_MNEMONIC(fchs_st0, "fchs st0");
    return FNIEMOP_CALL_1(iemOpHlpFpu_st0, iemAImpl_fchs_r80);
}


/** Opcode 0xd9 0xe1. */
FNIEMOP_DEF(iemOp_fabs)
{
    IEMOP_MNEMONIC(fabs_st0, "fabs st0");
    return FNIEMOP_CALL_1(iemOpHlpFpu_st0, iemAImpl_fabs_r80);
}


/** Opcode 0xd9 0xe4. */
FNIEMOP_DEF(iemOp_ftst)
{
    IEMOP_MNEMONIC(ftst_st0, "ftst st0");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(2, 1);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,    u16Fsw, 0);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value,          1);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value, 0)
        IEM_MC_CALL_FPU_AIMPL_2(iemAImpl_ftst_r80, pu16Fsw, pr80Value);
        IEM_MC_UPDATE_FSW(u16Fsw);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW(UINT8_MAX);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xd9 0xe5. */
FNIEMOP_DEF(iemOp_fxam)
{
    IEMOP_MNEMONIC(fxam_st0, "fxam st0");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(2, 1);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,    u16Fsw, 0);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value,          1);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_REF_FPUREG(pr80Value, 0);
    IEM_MC_CALL_FPU_AIMPL_2(iemAImpl_fxam_r80, pu16Fsw, pr80Value);
    IEM_MC_UPDATE_FSW(u16Fsw);
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/**
 * Common worker for FPU instructions pushing a constant onto the FPU stack.
 *
 * @param   pfnAImpl    Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpFpuPushConstant, PFNIEMAIMPLFPUR80LDCONST, pfnAImpl)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(1, 1);
    IEM_MC_LOCAL(IEMFPURESULT,          FpuRes);
    IEM_MC_ARG_LOCAL_REF(PIEMFPURESULT, pFpuRes,    FpuRes, 0);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_IS_EMPTY(7)
        IEM_MC_CALL_FPU_AIMPL_1(pfnAImpl, pFpuRes);
        IEM_MC_PUSH_FPU_RESULT(FpuRes);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_PUSH_OVERFLOW();
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xd9 0xe8. */
FNIEMOP_DEF(iemOp_fld1)
{
    IEMOP_MNEMONIC(fld1, "fld1");
    return FNIEMOP_CALL_1(iemOpHlpFpuPushConstant, iemAImpl_fld1);
}


/** Opcode 0xd9 0xe9. */
FNIEMOP_DEF(iemOp_fldl2t)
{
    IEMOP_MNEMONIC(fldl2t, "fldl2t");
    return FNIEMOP_CALL_1(iemOpHlpFpuPushConstant, iemAImpl_fldl2t);
}


/** Opcode 0xd9 0xea. */
FNIEMOP_DEF(iemOp_fldl2e)
{
    IEMOP_MNEMONIC(fldl2e, "fldl2e");
    return FNIEMOP_CALL_1(iemOpHlpFpuPushConstant, iemAImpl_fldl2e);
}

/** Opcode 0xd9 0xeb. */
FNIEMOP_DEF(iemOp_fldpi)
{
    IEMOP_MNEMONIC(fldpi, "fldpi");
    return FNIEMOP_CALL_1(iemOpHlpFpuPushConstant, iemAImpl_fldpi);
}


/** Opcode 0xd9 0xec. */
FNIEMOP_DEF(iemOp_fldlg2)
{
    IEMOP_MNEMONIC(fldlg2, "fldlg2");
    return FNIEMOP_CALL_1(iemOpHlpFpuPushConstant, iemAImpl_fldlg2);
}

/** Opcode 0xd9 0xed. */
FNIEMOP_DEF(iemOp_fldln2)
{
    IEMOP_MNEMONIC(fldln2, "fldln2");
    return FNIEMOP_CALL_1(iemOpHlpFpuPushConstant, iemAImpl_fldln2);
}


/** Opcode 0xd9 0xee. */
FNIEMOP_DEF(iemOp_fldz)
{
    IEMOP_MNEMONIC(fldz, "fldz");
    return FNIEMOP_CALL_1(iemOpHlpFpuPushConstant, iemAImpl_fldz);
}


/** Opcode 0xd9 0xf0.
 *
 * The f2xm1 instruction works on values +1.0 thru -1.0, currently (the range on
 * 287 & 8087 was +0.5 thru 0.0 according to docs).  In addition is does appear
 * to produce proper results for +Inf and -Inf.
 *
 * This is probably usful in the implementation pow() and similar.
 */
FNIEMOP_DEF(iemOp_f2xm1)
{
    IEMOP_MNEMONIC(f2xm1_st0, "f2xm1 st0");
    return FNIEMOP_CALL_1(iemOpHlpFpu_st0, iemAImpl_f2xm1_r80);
}


/**
 * Common worker for FPU instructions working on STn and ST0, storing the result
 * in STn, and popping the stack unless IE, DE or ZE was raised.
 *
 * @param   bRm         Mod R/M byte.
 * @param   pfnAImpl    Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_2(iemOpHlpFpu_stN_st0_pop, uint8_t, bRm, PFNIEMAIMPLFPUR80, pfnAImpl)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(3, 1);
    IEM_MC_LOCAL(IEMFPURESULT,          FpuRes);
    IEM_MC_ARG_LOCAL_REF(PIEMFPURESULT, pFpuRes,        FpuRes,     0);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value1,                 1);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value2,                 2);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80(pr80Value1, IEM_GET_MODRM_RM_8(bRm), pr80Value2, 0)
        IEM_MC_CALL_FPU_AIMPL_3(pfnAImpl, pFpuRes, pr80Value1, pr80Value2);
        IEM_MC_STORE_FPU_RESULT_THEN_POP(FpuRes, IEM_GET_MODRM_RM_8(bRm));
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW_THEN_POP(IEM_GET_MODRM_RM_8(bRm));
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xd9 0xf1. */
FNIEMOP_DEF(iemOp_fyl2x)
{
    IEMOP_MNEMONIC(fyl2x_st0, "fyl2x st1,st0");
    return FNIEMOP_CALL_2(iemOpHlpFpu_stN_st0_pop, 1, iemAImpl_fyl2x_r80_by_r80);
}


/**
 * Common worker for FPU instructions working on ST0 and having two outputs, one
 * replacing ST0 and one pushed onto the stack.
 *
 * @param   pfnAImpl    Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpFpuReplace_st0_push, PFNIEMAIMPLFPUR80UNARYTWO, pfnAImpl)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(2, 1);
    IEM_MC_LOCAL(IEMFPURESULTTWO,           FpuResTwo);
    IEM_MC_ARG_LOCAL_REF(PIEMFPURESULTTWO,  pFpuResTwo, FpuResTwo,  0);
    IEM_MC_ARG(PCRTFLOAT80U,                pr80Value,              1);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value, 0)
        IEM_MC_CALL_FPU_AIMPL_2(pfnAImpl, pFpuResTwo, pr80Value);
        IEM_MC_PUSH_FPU_RESULT_TWO(FpuResTwo);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_PUSH_UNDERFLOW_TWO();
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xd9 0xf2. */
FNIEMOP_DEF(iemOp_fptan)
{
    IEMOP_MNEMONIC(fptan_st0, "fptan st0");
    return FNIEMOP_CALL_1(iemOpHlpFpuReplace_st0_push, iemAImpl_fptan_r80_r80);
}


/** Opcode 0xd9 0xf3. */
FNIEMOP_DEF(iemOp_fpatan)
{
    IEMOP_MNEMONIC(fpatan_st1_st0, "fpatan st1,st0");
    return FNIEMOP_CALL_2(iemOpHlpFpu_stN_st0_pop, 1, iemAImpl_fpatan_r80_by_r80);
}


/** Opcode 0xd9 0xf4. */
FNIEMOP_DEF(iemOp_fxtract)
{
    IEMOP_MNEMONIC(fxtract_st0, "fxtract st0");
    return FNIEMOP_CALL_1(iemOpHlpFpuReplace_st0_push, iemAImpl_fxtract_r80_r80);
}


/** Opcode 0xd9 0xf5. */
FNIEMOP_DEF(iemOp_fprem1)
{
    IEMOP_MNEMONIC(fprem1_st0_st1, "fprem1 st0,st1");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_stN, 1, iemAImpl_fprem1_r80_by_r80);
}


/** Opcode 0xd9 0xf6. */
FNIEMOP_DEF(iemOp_fdecstp)
{
    IEMOP_MNEMONIC(fdecstp, "fdecstp");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    /* Note! C0, C2 and C3 are documented as undefined, we clear them. */
    /** @todo Testcase: Check whether FOP, FPUIP and FPUCS are affected by
     *        FINCSTP and FDECSTP. */

    IEM_MC_BEGIN(0,0);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_ACTUALIZE_FPU_STATE_FOR_CHANGE();
    IEM_MC_FPU_STACK_DEC_TOP();
    IEM_MC_UPDATE_FSW_CONST(0);

    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/** Opcode 0xd9 0xf7. */
FNIEMOP_DEF(iemOp_fincstp)
{
    IEMOP_MNEMONIC(fincstp, "fincstp");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    /* Note! C0, C2 and C3 are documented as undefined, we clear them. */
    /** @todo Testcase: Check whether FOP, FPUIP and FPUCS are affected by
     *        FINCSTP and FDECSTP. */

    IEM_MC_BEGIN(0,0);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_ACTUALIZE_FPU_STATE_FOR_CHANGE();
    IEM_MC_FPU_STACK_INC_TOP();
    IEM_MC_UPDATE_FSW_CONST(0);

    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/** Opcode 0xd9 0xf8. */
FNIEMOP_DEF(iemOp_fprem)
{
    IEMOP_MNEMONIC(fprem_st0_st1, "fprem st0,st1");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_stN, 1, iemAImpl_fprem_r80_by_r80);
}


/** Opcode 0xd9 0xf9. */
FNIEMOP_DEF(iemOp_fyl2xp1)
{
    IEMOP_MNEMONIC(fyl2xp1_st1_st0, "fyl2xp1 st1,st0");
    return FNIEMOP_CALL_2(iemOpHlpFpu_stN_st0_pop, 1, iemAImpl_fyl2xp1_r80_by_r80);
}


/** Opcode 0xd9 0xfa. */
FNIEMOP_DEF(iemOp_fsqrt)
{
    IEMOP_MNEMONIC(fsqrt_st0, "fsqrt st0");
    return FNIEMOP_CALL_1(iemOpHlpFpu_st0, iemAImpl_fsqrt_r80);
}


/** Opcode 0xd9 0xfb. */
FNIEMOP_DEF(iemOp_fsincos)
{
    IEMOP_MNEMONIC(fsincos_st0, "fsincos st0");
    return FNIEMOP_CALL_1(iemOpHlpFpuReplace_st0_push, iemAImpl_fsincos_r80_r80);
}


/** Opcode 0xd9 0xfc. */
FNIEMOP_DEF(iemOp_frndint)
{
    IEMOP_MNEMONIC(frndint_st0, "frndint st0");
    return FNIEMOP_CALL_1(iemOpHlpFpu_st0, iemAImpl_frndint_r80);
}


/** Opcode 0xd9 0xfd. */
FNIEMOP_DEF(iemOp_fscale)
{
    IEMOP_MNEMONIC(fscale_st0_st1, "fscale st0,st1");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_stN, 1, iemAImpl_fscale_r80_by_r80);
}


/** Opcode 0xd9 0xfe. */
FNIEMOP_DEF(iemOp_fsin)
{
    IEMOP_MNEMONIC(fsin_st0, "fsin st0");
    return FNIEMOP_CALL_1(iemOpHlpFpu_st0, iemAImpl_fsin_r80);
}


/** Opcode 0xd9 0xff. */
FNIEMOP_DEF(iemOp_fcos)
{
    IEMOP_MNEMONIC(fcos_st0, "fcos st0");
    return FNIEMOP_CALL_1(iemOpHlpFpu_st0, iemAImpl_fcos_r80);
}


/** Used by iemOp_EscF1. */
IEM_STATIC const PFNIEMOP g_apfnEscF1_E0toFF[32] =
{
    /* 0xe0 */  iemOp_fchs,
    /* 0xe1 */  iemOp_fabs,
    /* 0xe2 */  iemOp_Invalid,
    /* 0xe3 */  iemOp_Invalid,
    /* 0xe4 */  iemOp_ftst,
    /* 0xe5 */  iemOp_fxam,
    /* 0xe6 */  iemOp_Invalid,
    /* 0xe7 */  iemOp_Invalid,
    /* 0xe8 */  iemOp_fld1,
    /* 0xe9 */  iemOp_fldl2t,
    /* 0xea */  iemOp_fldl2e,
    /* 0xeb */  iemOp_fldpi,
    /* 0xec */  iemOp_fldlg2,
    /* 0xed */  iemOp_fldln2,
    /* 0xee */  iemOp_fldz,
    /* 0xef */  iemOp_Invalid,
    /* 0xf0 */  iemOp_f2xm1,
    /* 0xf1 */  iemOp_fyl2x,
    /* 0xf2 */  iemOp_fptan,
    /* 0xf3 */  iemOp_fpatan,
    /* 0xf4 */  iemOp_fxtract,
    /* 0xf5 */  iemOp_fprem1,
    /* 0xf6 */  iemOp_fdecstp,
    /* 0xf7 */  iemOp_fincstp,
    /* 0xf8 */  iemOp_fprem,
    /* 0xf9 */  iemOp_fyl2xp1,
    /* 0xfa */  iemOp_fsqrt,
    /* 0xfb */  iemOp_fsincos,
    /* 0xfc */  iemOp_frndint,
    /* 0xfd */  iemOp_fscale,
    /* 0xfe */  iemOp_fsin,
    /* 0xff */  iemOp_fcos
};


/**
 * @opcode      0xd9
 */
FNIEMOP_DEF(iemOp_EscF1)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    pVCpu->iem.s.uFpuOpcode = RT_MAKE_U16(bRm, 0xd9 & 0x7);

    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        switch (IEM_GET_MODRM_REG_8(bRm))
        {
            case 0: return FNIEMOP_CALL_1(iemOp_fld_stN, bRm);
            case 1: return FNIEMOP_CALL_1(iemOp_fxch_stN, bRm);
            case 2:
                if (bRm == 0xd0)
                    return FNIEMOP_CALL(iemOp_fnop);
                return IEMOP_RAISE_INVALID_OPCODE();
            case 3: return FNIEMOP_CALL_1(iemOp_fstp_stN, bRm); /* Reserved. Intel behavior seems to be FSTP ST(i) though. */
            case 4:
            case 5:
            case 6:
            case 7:
                Assert((unsigned)bRm - 0xe0U < RT_ELEMENTS(g_apfnEscF1_E0toFF));
                return FNIEMOP_CALL(g_apfnEscF1_E0toFF[bRm - 0xe0]);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        switch (IEM_GET_MODRM_REG_8(bRm))
        {
            case 0: return FNIEMOP_CALL_1(iemOp_fld_m32r,  bRm);
            case 1: return IEMOP_RAISE_INVALID_OPCODE();
            case 2: return FNIEMOP_CALL_1(iemOp_fst_m32r,  bRm);
            case 3: return FNIEMOP_CALL_1(iemOp_fstp_m32r, bRm);
            case 4: return FNIEMOP_CALL_1(iemOp_fldenv,    bRm);
            case 5: return FNIEMOP_CALL_1(iemOp_fldcw,     bRm);
            case 6: return FNIEMOP_CALL_1(iemOp_fnstenv,    bRm);
            case 7: return FNIEMOP_CALL_1(iemOp_fnstcw,     bRm);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/** Opcode 0xda 11/0. */
FNIEMOP_DEF_1(iemOp_fcmovb_stN,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fcmovb_st0_stN, "fcmovb st0,stN");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(0, 1);
    IEM_MC_LOCAL(PCRTFLOAT80U,      pr80ValueN);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80_FIRST(pr80ValueN, IEM_GET_MODRM_RM_8(bRm), 0)
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_CF)
            IEM_MC_STORE_FPUREG_R80_SRC_REF(0, pr80ValueN);
        IEM_MC_ENDIF();
        IEM_MC_UPDATE_FPU_OPCODE_IP();
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW(0);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xda 11/1. */
FNIEMOP_DEF_1(iemOp_fcmove_stN,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fcmove_st0_stN, "fcmove st0,stN");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(0, 1);
    IEM_MC_LOCAL(PCRTFLOAT80U,      pr80ValueN);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80_FIRST(pr80ValueN, IEM_GET_MODRM_RM_8(bRm), 0)
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_ZF)
            IEM_MC_STORE_FPUREG_R80_SRC_REF(0, pr80ValueN);
        IEM_MC_ENDIF();
        IEM_MC_UPDATE_FPU_OPCODE_IP();
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW(0);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xda 11/2. */
FNIEMOP_DEF_1(iemOp_fcmovbe_stN, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fcmovbe_st0_stN, "fcmovbe st0,stN");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(0, 1);
    IEM_MC_LOCAL(PCRTFLOAT80U,      pr80ValueN);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80_FIRST(pr80ValueN, IEM_GET_MODRM_RM_8(bRm), 0)
        IEM_MC_IF_EFL_ANY_BITS_SET(X86_EFL_CF | X86_EFL_ZF)
            IEM_MC_STORE_FPUREG_R80_SRC_REF(0, pr80ValueN);
        IEM_MC_ENDIF();
        IEM_MC_UPDATE_FPU_OPCODE_IP();
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW(0);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xda 11/3. */
FNIEMOP_DEF_1(iemOp_fcmovu_stN,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fcmovu_st0_stN, "fcmovu st0,stN");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(0, 1);
    IEM_MC_LOCAL(PCRTFLOAT80U,      pr80ValueN);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80_FIRST(pr80ValueN, IEM_GET_MODRM_RM_8(bRm), 0)
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_PF)
            IEM_MC_STORE_FPUREG_R80_SRC_REF(0, pr80ValueN);
        IEM_MC_ENDIF();
        IEM_MC_UPDATE_FPU_OPCODE_IP();
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW(0);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/**
 * Common worker for FPU instructions working on ST0 and ST1, only affecting
 * flags, and popping twice when done.
 *
 * @param   pfnAImpl    Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpFpuNoStore_st0_st1_pop_pop, PFNIEMAIMPLFPUR80FSW, pfnAImpl)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(3, 1);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,        u16Fsw,     0);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value1,                 1);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value2,                 2);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80(pr80Value1, 0, pr80Value2, 1)
        IEM_MC_CALL_FPU_AIMPL_3(pfnAImpl, pu16Fsw, pr80Value1, pr80Value2);
        IEM_MC_UPDATE_FSW_THEN_POP_POP(u16Fsw);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW_THEN_POP_POP();
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xda 0xe9. */
FNIEMOP_DEF(iemOp_fucompp)
{
    IEMOP_MNEMONIC(fucompp, "fucompp");
    return FNIEMOP_CALL_1(iemOpHlpFpuNoStore_st0_st1_pop_pop, iemAImpl_fucom_r80_by_r80);
}


/**
 * Common worker for FPU instructions working on ST0 and an m32i, and storing
 * the result in ST0.
 *
 * @param   bRm         Mod R/M byte.
 * @param   pfnAImpl    Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_2(iemOpHlpFpu_st0_m32i, uint8_t, bRm, PFNIEMAIMPLFPUI32, pfnAImpl)
{
    IEM_MC_BEGIN(3, 3);
    IEM_MC_LOCAL(RTGCPTR,                   GCPtrEffSrc);
    IEM_MC_LOCAL(IEMFPURESULT,              FpuRes);
    IEM_MC_LOCAL(int32_t,                   i32Val2);
    IEM_MC_ARG_LOCAL_REF(PIEMFPURESULT,     pFpuRes,        FpuRes,     0);
    IEM_MC_ARG(PCRTFLOAT80U,                pr80Value1,                 1);
    IEM_MC_ARG_LOCAL_REF(int32_t const *,   pi32Val2,       i32Val2,    2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_FETCH_MEM_I32(i32Val2, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value1, 0)
        IEM_MC_CALL_FPU_AIMPL_3(pfnAImpl, pFpuRes, pr80Value1, pi32Val2);
        IEM_MC_STORE_FPU_RESULT(FpuRes, 0);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW(0);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xda !11/0. */
FNIEMOP_DEF_1(iemOp_fiadd_m32i,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fiadd_m32i, "fiadd m32i");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_m32i, bRm, iemAImpl_fiadd_r80_by_i32);
}


/** Opcode 0xda !11/1. */
FNIEMOP_DEF_1(iemOp_fimul_m32i,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fimul_m32i, "fimul m32i");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_m32i, bRm, iemAImpl_fimul_r80_by_i32);
}


/** Opcode 0xda !11/2. */
FNIEMOP_DEF_1(iemOp_ficom_m32i,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(ficom_st0_m32i, "ficom st0,m32i");

    IEM_MC_BEGIN(3, 3);
    IEM_MC_LOCAL(RTGCPTR,                   GCPtrEffSrc);
    IEM_MC_LOCAL(uint16_t,                  u16Fsw);
    IEM_MC_LOCAL(int32_t,                   i32Val2);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,        pu16Fsw,        u16Fsw,     0);
    IEM_MC_ARG(PCRTFLOAT80U,                pr80Value1,                 1);
    IEM_MC_ARG_LOCAL_REF(int32_t const *,   pi32Val2,       i32Val2,    2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_FETCH_MEM_I32(i32Val2, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value1, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_ficom_r80_by_i32, pu16Fsw, pr80Value1, pi32Val2);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xda !11/3. */
FNIEMOP_DEF_1(iemOp_ficomp_m32i, uint8_t, bRm)
{
    IEMOP_MNEMONIC(ficomp_st0_m32i, "ficomp st0,m32i");

    IEM_MC_BEGIN(3, 3);
    IEM_MC_LOCAL(RTGCPTR,                   GCPtrEffSrc);
    IEM_MC_LOCAL(uint16_t,                  u16Fsw);
    IEM_MC_LOCAL(int32_t,                   i32Val2);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,        pu16Fsw,        u16Fsw,     0);
    IEM_MC_ARG(PCRTFLOAT80U,                pr80Value1,                 1);
    IEM_MC_ARG_LOCAL_REF(int32_t const *,   pi32Val2,       i32Val2,    2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_FETCH_MEM_I32(i32Val2, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value1, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_ficom_r80_by_i32, pu16Fsw, pr80Value1, pi32Val2);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP_THEN_POP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP_THEN_POP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xda !11/4. */
FNIEMOP_DEF_1(iemOp_fisub_m32i,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fisub_m32i, "fisub m32i");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_m32i, bRm, iemAImpl_fisub_r80_by_i32);
}


/** Opcode 0xda !11/5. */
FNIEMOP_DEF_1(iemOp_fisubr_m32i, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fisubr_m32i, "fisubr m32i");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_m32i, bRm, iemAImpl_fisubr_r80_by_i32);
}


/** Opcode 0xda !11/6. */
FNIEMOP_DEF_1(iemOp_fidiv_m32i,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fidiv_m32i, "fidiv m32i");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_m32i, bRm, iemAImpl_fidiv_r80_by_i32);
}


/** Opcode 0xda !11/7. */
FNIEMOP_DEF_1(iemOp_fidivr_m32i, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fidivr_m32i, "fidivr m32i");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_m32i, bRm, iemAImpl_fidivr_r80_by_i32);
}


/**
 * @opcode      0xda
 */
FNIEMOP_DEF(iemOp_EscF2)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    pVCpu->iem.s.uFpuOpcode = RT_MAKE_U16(bRm, 0xda & 0x7);
    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        switch (IEM_GET_MODRM_REG_8(bRm))
        {
            case 0: return FNIEMOP_CALL_1(iemOp_fcmovb_stN, bRm);
            case 1: return FNIEMOP_CALL_1(iemOp_fcmove_stN, bRm);
            case 2: return FNIEMOP_CALL_1(iemOp_fcmovbe_stN, bRm);
            case 3: return FNIEMOP_CALL_1(iemOp_fcmovu_stN, bRm);
            case 4: return IEMOP_RAISE_INVALID_OPCODE();
            case 5:
                if (bRm == 0xe9)
                    return FNIEMOP_CALL(iemOp_fucompp);
                return IEMOP_RAISE_INVALID_OPCODE();
            case 6: return IEMOP_RAISE_INVALID_OPCODE();
            case 7: return IEMOP_RAISE_INVALID_OPCODE();
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        switch (IEM_GET_MODRM_REG_8(bRm))
        {
            case 0: return FNIEMOP_CALL_1(iemOp_fiadd_m32i,  bRm);
            case 1: return FNIEMOP_CALL_1(iemOp_fimul_m32i,  bRm);
            case 2: return FNIEMOP_CALL_1(iemOp_ficom_m32i,  bRm);
            case 3: return FNIEMOP_CALL_1(iemOp_ficomp_m32i, bRm);
            case 4: return FNIEMOP_CALL_1(iemOp_fisub_m32i,  bRm);
            case 5: return FNIEMOP_CALL_1(iemOp_fisubr_m32i, bRm);
            case 6: return FNIEMOP_CALL_1(iemOp_fidiv_m32i,  bRm);
            case 7: return FNIEMOP_CALL_1(iemOp_fidivr_m32i, bRm);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/** Opcode 0xdb !11/0. */
FNIEMOP_DEF_1(iemOp_fild_m32i, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fild_m32i, "fild m32i");

    IEM_MC_BEGIN(2, 3);
    IEM_MC_LOCAL(RTGCPTR,                   GCPtrEffSrc);
    IEM_MC_LOCAL(IEMFPURESULT,              FpuRes);
    IEM_MC_LOCAL(int32_t,                   i32Val);
    IEM_MC_ARG_LOCAL_REF(PIEMFPURESULT,     pFpuRes,    FpuRes, 0);
    IEM_MC_ARG_LOCAL_REF(int32_t const *,   pi32Val,    i32Val, 1);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_FETCH_MEM_I32(i32Val, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_IS_EMPTY(7)
        IEM_MC_CALL_FPU_AIMPL_2(iemAImpl_fild_r80_from_i32, pFpuRes, pi32Val);
        IEM_MC_PUSH_FPU_RESULT_MEM_OP(FpuRes, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_PUSH_OVERFLOW_MEM_OP(pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdb !11/1. */
FNIEMOP_DEF_1(iemOp_fisttp_m32i, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fisttp_m32i, "fisttp m32i");
    IEM_MC_BEGIN(3, 2);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffDst);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,    u16Fsw, 0);
    IEM_MC_ARG(int32_t *,               pi32Dst,            1);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value,          2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_MEM_MAP(pi32Dst, IEM_ACCESS_DATA_W, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 1 /*arg*/);
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_fistt_r80_to_i32, pu16Fsw, pi32Dst, pr80Value);
        IEM_MC_MEM_COMMIT_AND_UNMAP_FOR_FPU_STORE(pi32Dst, IEM_ACCESS_DATA_W, u16Fsw);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP_THEN_POP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ELSE()
        IEM_MC_IF_FCW_IM()
            IEM_MC_STORE_MEM_I32_CONST_BY_REF(pi32Dst, INT32_MIN /* (integer indefinite) */);
            IEM_MC_MEM_COMMIT_AND_UNMAP(pi32Dst, IEM_ACCESS_DATA_W);
        IEM_MC_ENDIF();
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP_THEN_POP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdb !11/2. */
FNIEMOP_DEF_1(iemOp_fist_m32i, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fist_m32i, "fist m32i");
    IEM_MC_BEGIN(3, 2);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffDst);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,    u16Fsw, 0);
    IEM_MC_ARG(int32_t *,               pi32Dst,            1);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value,          2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_MEM_MAP(pi32Dst, IEM_ACCESS_DATA_W, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 1 /*arg*/);
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_fist_r80_to_i32, pu16Fsw, pi32Dst, pr80Value);
        IEM_MC_MEM_COMMIT_AND_UNMAP_FOR_FPU_STORE(pi32Dst, IEM_ACCESS_DATA_W, u16Fsw);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ELSE()
        IEM_MC_IF_FCW_IM()
            IEM_MC_STORE_MEM_I32_CONST_BY_REF(pi32Dst, INT32_MIN /* (integer indefinite) */);
            IEM_MC_MEM_COMMIT_AND_UNMAP(pi32Dst, IEM_ACCESS_DATA_W);
        IEM_MC_ENDIF();
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdb !11/3. */
FNIEMOP_DEF_1(iemOp_fistp_m32i, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fistp_m32i, "fistp m32i");
    IEM_MC_BEGIN(3, 2);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffDst);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,    u16Fsw, 0);
    IEM_MC_ARG(int32_t *,               pi32Dst,            1);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value,          2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_MEM_MAP(pi32Dst, IEM_ACCESS_DATA_W, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 1 /*arg*/);
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_fist_r80_to_i32, pu16Fsw, pi32Dst, pr80Value);
        IEM_MC_MEM_COMMIT_AND_UNMAP_FOR_FPU_STORE(pi32Dst, IEM_ACCESS_DATA_W, u16Fsw);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP_THEN_POP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ELSE()
        IEM_MC_IF_FCW_IM()
            IEM_MC_STORE_MEM_I32_CONST_BY_REF(pi32Dst, INT32_MIN /* (integer indefinite) */);
            IEM_MC_MEM_COMMIT_AND_UNMAP(pi32Dst, IEM_ACCESS_DATA_W);
        IEM_MC_ENDIF();
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP_THEN_POP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdb !11/5. */
FNIEMOP_DEF_1(iemOp_fld_m80r, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fld_m80r, "fld m80r");

    IEM_MC_BEGIN(2, 3);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffSrc);
    IEM_MC_LOCAL(IEMFPURESULT,          FpuRes);
    IEM_MC_LOCAL(RTFLOAT80U,            r80Val);
    IEM_MC_ARG_LOCAL_REF(PIEMFPURESULT, pFpuRes,    FpuRes, 0);
    IEM_MC_ARG_LOCAL_REF(PCRTFLOAT80U,  pr80Val,    r80Val, 1);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_FETCH_MEM_R80(r80Val, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_IS_EMPTY(7)
        IEM_MC_CALL_FPU_AIMPL_2(iemAImpl_fld_r80_from_r80, pFpuRes, pr80Val);
        IEM_MC_PUSH_FPU_RESULT_MEM_OP(FpuRes, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_PUSH_OVERFLOW_MEM_OP(pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdb !11/7. */
FNIEMOP_DEF_1(iemOp_fstp_m80r, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fstp_m80r, "fstp m80r");
    IEM_MC_BEGIN(3, 2);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffDst);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,    u16Fsw, 0);
    IEM_MC_ARG(PRTFLOAT80U,             pr80Dst,            1);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value,          2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_MEM_MAP_EX(pr80Dst, IEM_ACCESS_DATA_W, sizeof(*pr80Dst), pVCpu->iem.s.iEffSeg, GCPtrEffDst, 7 /*cbAlign*/, 1 /*arg*/);
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_fst_r80_to_r80, pu16Fsw, pr80Dst, pr80Value);
        IEM_MC_MEM_COMMIT_AND_UNMAP_FOR_FPU_STORE(pr80Dst, IEM_ACCESS_DATA_W, u16Fsw);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP_THEN_POP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ELSE()
        IEM_MC_IF_FCW_IM()
            IEM_MC_STORE_MEM_NEG_QNAN_R80_BY_REF(pr80Dst);
            IEM_MC_MEM_COMMIT_AND_UNMAP(pr80Dst, IEM_ACCESS_DATA_W);
        IEM_MC_ENDIF();
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP_THEN_POP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdb 11/0. */
FNIEMOP_DEF_1(iemOp_fcmovnb_stN,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fcmovnb_st0_stN, "fcmovnb st0,stN");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(0, 1);
    IEM_MC_LOCAL(PCRTFLOAT80U,      pr80ValueN);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80_FIRST(pr80ValueN, IEM_GET_MODRM_RM_8(bRm), 0)
        IEM_MC_IF_EFL_BIT_NOT_SET(X86_EFL_CF)
            IEM_MC_STORE_FPUREG_R80_SRC_REF(0, pr80ValueN);
        IEM_MC_ENDIF();
        IEM_MC_UPDATE_FPU_OPCODE_IP();
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW(0);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdb 11/1. */
FNIEMOP_DEF_1(iemOp_fcmovne_stN,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fcmovne_st0_stN, "fcmovne st0,stN");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(0, 1);
    IEM_MC_LOCAL(PCRTFLOAT80U,      pr80ValueN);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80_FIRST(pr80ValueN, IEM_GET_MODRM_RM_8(bRm), 0)
        IEM_MC_IF_EFL_BIT_NOT_SET(X86_EFL_ZF)
            IEM_MC_STORE_FPUREG_R80_SRC_REF(0, pr80ValueN);
        IEM_MC_ENDIF();
        IEM_MC_UPDATE_FPU_OPCODE_IP();
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW(0);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdb 11/2. */
FNIEMOP_DEF_1(iemOp_fcmovnbe_stN, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fcmovnbe_st0_stN, "fcmovnbe st0,stN");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(0, 1);
    IEM_MC_LOCAL(PCRTFLOAT80U,      pr80ValueN);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80_FIRST(pr80ValueN, IEM_GET_MODRM_RM_8(bRm), 0)
        IEM_MC_IF_EFL_NO_BITS_SET(X86_EFL_CF | X86_EFL_ZF)
            IEM_MC_STORE_FPUREG_R80_SRC_REF(0, pr80ValueN);
        IEM_MC_ENDIF();
        IEM_MC_UPDATE_FPU_OPCODE_IP();
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW(0);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdb 11/3. */
FNIEMOP_DEF_1(iemOp_fcmovnnu_stN, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fcmovnnu_st0_stN, "fcmovnnu st0,stN");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(0, 1);
    IEM_MC_LOCAL(PCRTFLOAT80U,      pr80ValueN);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80_FIRST(pr80ValueN, IEM_GET_MODRM_RM_8(bRm), 0)
        IEM_MC_IF_EFL_BIT_NOT_SET(X86_EFL_PF)
            IEM_MC_STORE_FPUREG_R80_SRC_REF(0, pr80ValueN);
        IEM_MC_ENDIF();
        IEM_MC_UPDATE_FPU_OPCODE_IP();
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW(0);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdb 0xe0. */
FNIEMOP_DEF(iemOp_fneni)
{
    IEMOP_MNEMONIC(fneni, "fneni (8087/ign)");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_BEGIN(0,0);
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/** Opcode 0xdb 0xe1. */
FNIEMOP_DEF(iemOp_fndisi)
{
    IEMOP_MNEMONIC(fndisi, "fndisi (8087/ign)");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_BEGIN(0,0);
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/** Opcode 0xdb 0xe2. */
FNIEMOP_DEF(iemOp_fnclex)
{
    IEMOP_MNEMONIC(fnclex, "fnclex");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(0,0);
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_ACTUALIZE_FPU_STATE_FOR_CHANGE();
    IEM_MC_CLEAR_FSW_EX();
    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/** Opcode 0xdb 0xe3. */
FNIEMOP_DEF(iemOp_fninit)
{
    IEMOP_MNEMONIC(fninit, "fninit");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_finit, false /*fCheckXcpts*/);
}


/** Opcode 0xdb 0xe4. */
FNIEMOP_DEF(iemOp_fnsetpm)
{
    IEMOP_MNEMONIC(fnsetpm, "fnsetpm (80287/ign)");   /* set protected mode on fpu. */
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_BEGIN(0,0);
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/** Opcode 0xdb 0xe5. */
FNIEMOP_DEF(iemOp_frstpm)
{
    IEMOP_MNEMONIC(frstpm, "frstpm (80287XL/ign)"); /* reset pm, back to real mode. */
#if 0 /* #UDs on newer CPUs */
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_BEGIN(0,0);
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
    return VINF_SUCCESS;
#else
    return IEMOP_RAISE_INVALID_OPCODE();
#endif
}


/** Opcode 0xdb 11/5. */
FNIEMOP_DEF_1(iemOp_fucomi_stN, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fucomi_st0_stN, "fucomi st0,stN");
    return IEM_MC_DEFER_TO_CIMPL_3(iemCImpl_fcomi_fucomi, IEM_GET_MODRM_RM_8(bRm), iemAImpl_fucomi_r80_by_r80, false /*fPop*/);
}


/** Opcode 0xdb 11/6. */
FNIEMOP_DEF_1(iemOp_fcomi_stN,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fcomi_st0_stN, "fcomi st0,stN");
    return IEM_MC_DEFER_TO_CIMPL_3(iemCImpl_fcomi_fucomi, IEM_GET_MODRM_RM_8(bRm), iemAImpl_fcomi_r80_by_r80, false /*fPop*/);
}


/**
 * @opcode      0xdb
 */
FNIEMOP_DEF(iemOp_EscF3)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    pVCpu->iem.s.uFpuOpcode = RT_MAKE_U16(bRm, 0xdb & 0x7);
    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        switch (IEM_GET_MODRM_REG_8(bRm))
        {
            case 0: return FNIEMOP_CALL_1(iemOp_fcmovnb_stN,  bRm);
            case 1: return FNIEMOP_CALL_1(iemOp_fcmovne_stN,  bRm);
            case 2: return FNIEMOP_CALL_1(iemOp_fcmovnbe_stN, bRm);
            case 3: return FNIEMOP_CALL_1(iemOp_fcmovnnu_stN, bRm);
            case 4:
                switch (bRm)
                {
                    case 0xe0:  return FNIEMOP_CALL(iemOp_fneni);
                    case 0xe1:  return FNIEMOP_CALL(iemOp_fndisi);
                    case 0xe2:  return FNIEMOP_CALL(iemOp_fnclex);
                    case 0xe3:  return FNIEMOP_CALL(iemOp_fninit);
                    case 0xe4:  return FNIEMOP_CALL(iemOp_fnsetpm);
                    case 0xe5:  return FNIEMOP_CALL(iemOp_frstpm);
                    case 0xe6:  return IEMOP_RAISE_INVALID_OPCODE();
                    case 0xe7:  return IEMOP_RAISE_INVALID_OPCODE();
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
                break;
            case 5: return FNIEMOP_CALL_1(iemOp_fucomi_stN, bRm);
            case 6: return FNIEMOP_CALL_1(iemOp_fcomi_stN,  bRm);
            case 7: return IEMOP_RAISE_INVALID_OPCODE();
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        switch (IEM_GET_MODRM_REG_8(bRm))
        {
            case 0: return FNIEMOP_CALL_1(iemOp_fild_m32i,  bRm);
            case 1: return FNIEMOP_CALL_1(iemOp_fisttp_m32i,bRm);
            case 2: return FNIEMOP_CALL_1(iemOp_fist_m32i,  bRm);
            case 3: return FNIEMOP_CALL_1(iemOp_fistp_m32i, bRm);
            case 4: return IEMOP_RAISE_INVALID_OPCODE();
            case 5: return FNIEMOP_CALL_1(iemOp_fld_m80r,   bRm);
            case 6: return IEMOP_RAISE_INVALID_OPCODE();
            case 7: return FNIEMOP_CALL_1(iemOp_fstp_m80r,  bRm);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/**
 * Common worker for FPU instructions working on STn and ST0, and storing the
 * result in STn unless IE, DE or ZE was raised.
 *
 * @param   bRm         Mod R/M byte.
 * @param   pfnAImpl    Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_2(iemOpHlpFpu_stN_st0, uint8_t, bRm, PFNIEMAIMPLFPUR80, pfnAImpl)
{
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(3, 1);
    IEM_MC_LOCAL(IEMFPURESULT,          FpuRes);
    IEM_MC_ARG_LOCAL_REF(PIEMFPURESULT, pFpuRes,        FpuRes,     0);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value1,                 1);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value2,                 2);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80(pr80Value1, IEM_GET_MODRM_RM_8(bRm), pr80Value2, 0)
        IEM_MC_CALL_FPU_AIMPL_3(pfnAImpl, pFpuRes, pr80Value1, pr80Value2);
        IEM_MC_STORE_FPU_RESULT(FpuRes, IEM_GET_MODRM_RM_8(bRm));
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW(IEM_GET_MODRM_RM_8(bRm));
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdc 11/0. */
FNIEMOP_DEF_1(iemOp_fadd_stN_st0,   uint8_t, bRm)
{
    IEMOP_MNEMONIC(fadd_stN_st0, "fadd stN,st0");
    return FNIEMOP_CALL_2(iemOpHlpFpu_stN_st0, bRm, iemAImpl_fadd_r80_by_r80);
}


/** Opcode 0xdc 11/1. */
FNIEMOP_DEF_1(iemOp_fmul_stN_st0,   uint8_t, bRm)
{
    IEMOP_MNEMONIC(fmul_stN_st0, "fmul stN,st0");
    return FNIEMOP_CALL_2(iemOpHlpFpu_stN_st0, bRm, iemAImpl_fmul_r80_by_r80);
}


/** Opcode 0xdc 11/4. */
FNIEMOP_DEF_1(iemOp_fsubr_stN_st0,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fsubr_stN_st0, "fsubr stN,st0");
    return FNIEMOP_CALL_2(iemOpHlpFpu_stN_st0, bRm, iemAImpl_fsubr_r80_by_r80);
}


/** Opcode 0xdc 11/5. */
FNIEMOP_DEF_1(iemOp_fsub_stN_st0,   uint8_t, bRm)
{
    IEMOP_MNEMONIC(fsub_stN_st0, "fsub stN,st0");
    return FNIEMOP_CALL_2(iemOpHlpFpu_stN_st0, bRm, iemAImpl_fsub_r80_by_r80);
}


/** Opcode 0xdc 11/6. */
FNIEMOP_DEF_1(iemOp_fdivr_stN_st0,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fdivr_stN_st0, "fdivr stN,st0");
    return FNIEMOP_CALL_2(iemOpHlpFpu_stN_st0, bRm, iemAImpl_fdivr_r80_by_r80);
}


/** Opcode 0xdc 11/7. */
FNIEMOP_DEF_1(iemOp_fdiv_stN_st0,   uint8_t, bRm)
{
    IEMOP_MNEMONIC(fdiv_stN_st0, "fdiv stN,st0");
    return FNIEMOP_CALL_2(iemOpHlpFpu_stN_st0, bRm, iemAImpl_fdiv_r80_by_r80);
}


/**
 * Common worker for FPU instructions working on ST0 and a 64-bit floating point
 * memory operand, and storing the result in ST0.
 *
 * @param   bRm         Mod R/M byte.
 * @param   pfnImpl     Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_2(iemOpHlpFpu_ST0_m64r, uint8_t, bRm, PFNIEMAIMPLFPUR64, pfnImpl)
{
    IEM_MC_BEGIN(3, 3);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffSrc);
    IEM_MC_LOCAL(IEMFPURESULT,          FpuRes);
    IEM_MC_LOCAL(RTFLOAT64U,            r64Factor2);
    IEM_MC_ARG_LOCAL_REF(PIEMFPURESULT, pFpuRes,        FpuRes,     0);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Factor1,                1);
    IEM_MC_ARG_LOCAL_REF(PRTFLOAT64U,   pr64Factor2,    r64Factor2, 2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_FETCH_MEM_R64(r64Factor2, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Factor1, 0)
        IEM_MC_CALL_FPU_AIMPL_3(pfnImpl, pFpuRes, pr80Factor1, pr64Factor2);
        IEM_MC_STORE_FPU_RESULT_MEM_OP(FpuRes, 0, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP(0, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdc !11/0. */
FNIEMOP_DEF_1(iemOp_fadd_m64r,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fadd_m64r, "fadd m64r");
    return FNIEMOP_CALL_2(iemOpHlpFpu_ST0_m64r, bRm, iemAImpl_fadd_r80_by_r64);
}


/** Opcode 0xdc !11/1. */
FNIEMOP_DEF_1(iemOp_fmul_m64r,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fmul_m64r, "fmul m64r");
    return FNIEMOP_CALL_2(iemOpHlpFpu_ST0_m64r, bRm, iemAImpl_fmul_r80_by_r64);
}


/** Opcode 0xdc !11/2. */
FNIEMOP_DEF_1(iemOp_fcom_m64r,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fcom_st0_m64r, "fcom st0,m64r");

    IEM_MC_BEGIN(3, 3);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffSrc);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_LOCAL(RTFLOAT64U,            r64Val2);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,        u16Fsw,     0);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value1,                 1);
    IEM_MC_ARG_LOCAL_REF(PCRTFLOAT64U,  pr64Val2,       r64Val2,    2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_FETCH_MEM_R64(r64Val2, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value1, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_fcom_r80_by_r64, pu16Fsw, pr80Value1, pr64Val2);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdc !11/3. */
FNIEMOP_DEF_1(iemOp_fcomp_m64r, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fcomp_st0_m64r, "fcomp st0,m64r");

    IEM_MC_BEGIN(3, 3);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffSrc);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_LOCAL(RTFLOAT64U,            r64Val2);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,        u16Fsw,     0);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value1,                 1);
    IEM_MC_ARG_LOCAL_REF(PCRTFLOAT64U,  pr64Val2,       r64Val2,    2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_FETCH_MEM_R64(r64Val2, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value1, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_fcom_r80_by_r64, pu16Fsw, pr80Value1, pr64Val2);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP_THEN_POP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP_THEN_POP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdc !11/4. */
FNIEMOP_DEF_1(iemOp_fsub_m64r,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fsub_m64r, "fsub m64r");
    return FNIEMOP_CALL_2(iemOpHlpFpu_ST0_m64r, bRm, iemAImpl_fsub_r80_by_r64);
}


/** Opcode 0xdc !11/5. */
FNIEMOP_DEF_1(iemOp_fsubr_m64r, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fsubr_m64r, "fsubr m64r");
    return FNIEMOP_CALL_2(iemOpHlpFpu_ST0_m64r, bRm, iemAImpl_fsubr_r80_by_r64);
}


/** Opcode 0xdc !11/6. */
FNIEMOP_DEF_1(iemOp_fdiv_m64r,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fdiv_m64r, "fdiv m64r");
    return FNIEMOP_CALL_2(iemOpHlpFpu_ST0_m64r, bRm, iemAImpl_fdiv_r80_by_r64);
}


/** Opcode 0xdc !11/7. */
FNIEMOP_DEF_1(iemOp_fdivr_m64r, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fdivr_m64r, "fdivr m64r");
    return FNIEMOP_CALL_2(iemOpHlpFpu_ST0_m64r, bRm, iemAImpl_fdivr_r80_by_r64);
}


/**
 * @opcode      0xdc
 */
FNIEMOP_DEF(iemOp_EscF4)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    pVCpu->iem.s.uFpuOpcode = RT_MAKE_U16(bRm, 0xdc & 0x7);
    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        switch (IEM_GET_MODRM_REG_8(bRm))
        {
            case 0: return FNIEMOP_CALL_1(iemOp_fadd_stN_st0,  bRm);
            case 1: return FNIEMOP_CALL_1(iemOp_fmul_stN_st0,  bRm);
            case 2: return FNIEMOP_CALL_1(iemOp_fcom_stN,      bRm); /* Marked reserved, intel behavior is that of FCOM ST(i). */
            case 3: return FNIEMOP_CALL_1(iemOp_fcomp_stN,     bRm); /* Marked reserved, intel behavior is that of FCOMP ST(i). */
            case 4: return FNIEMOP_CALL_1(iemOp_fsubr_stN_st0, bRm);
            case 5: return FNIEMOP_CALL_1(iemOp_fsub_stN_st0,  bRm);
            case 6: return FNIEMOP_CALL_1(iemOp_fdivr_stN_st0, bRm);
            case 7: return FNIEMOP_CALL_1(iemOp_fdiv_stN_st0,  bRm);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        switch (IEM_GET_MODRM_REG_8(bRm))
        {
            case 0: return FNIEMOP_CALL_1(iemOp_fadd_m64r,  bRm);
            case 1: return FNIEMOP_CALL_1(iemOp_fmul_m64r,  bRm);
            case 2: return FNIEMOP_CALL_1(iemOp_fcom_m64r,  bRm);
            case 3: return FNIEMOP_CALL_1(iemOp_fcomp_m64r, bRm);
            case 4: return FNIEMOP_CALL_1(iemOp_fsub_m64r,  bRm);
            case 5: return FNIEMOP_CALL_1(iemOp_fsubr_m64r, bRm);
            case 6: return FNIEMOP_CALL_1(iemOp_fdiv_m64r,  bRm);
            case 7: return FNIEMOP_CALL_1(iemOp_fdivr_m64r, bRm);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/** Opcode 0xdd !11/0.
 * @sa iemOp_fld_m32r */
FNIEMOP_DEF_1(iemOp_fld_m64r,    uint8_t, bRm)
{
    IEMOP_MNEMONIC(fld_m64r, "fld m64r");

    IEM_MC_BEGIN(2, 3);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffSrc);
    IEM_MC_LOCAL(IEMFPURESULT,          FpuRes);
    IEM_MC_LOCAL(RTFLOAT64U,            r64Val);
    IEM_MC_ARG_LOCAL_REF(PIEMFPURESULT, pFpuRes,    FpuRes, 0);
    IEM_MC_ARG_LOCAL_REF(PCRTFLOAT64U,  pr64Val,    r64Val, 1);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_FETCH_MEM_R64(r64Val, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_IS_EMPTY(7)
        IEM_MC_CALL_FPU_AIMPL_2(iemAImpl_fld_r80_from_r64, pFpuRes, pr64Val);
        IEM_MC_PUSH_FPU_RESULT_MEM_OP(FpuRes, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_PUSH_OVERFLOW_MEM_OP(pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdd !11/0. */
FNIEMOP_DEF_1(iemOp_fisttp_m64i, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fisttp_m64i, "fisttp m64i");
    IEM_MC_BEGIN(3, 2);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffDst);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,    u16Fsw, 0);
    IEM_MC_ARG(int64_t *,               pi64Dst,            1);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value,          2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_MEM_MAP(pi64Dst, IEM_ACCESS_DATA_W, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 1 /*arg*/);
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_fistt_r80_to_i64, pu16Fsw, pi64Dst, pr80Value);
        IEM_MC_MEM_COMMIT_AND_UNMAP_FOR_FPU_STORE(pi64Dst, IEM_ACCESS_DATA_W, u16Fsw);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP_THEN_POP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ELSE()
        IEM_MC_IF_FCW_IM()
            IEM_MC_STORE_MEM_I64_CONST_BY_REF(pi64Dst, INT64_MIN /* (integer indefinite) */);
            IEM_MC_MEM_COMMIT_AND_UNMAP(pi64Dst, IEM_ACCESS_DATA_W);
        IEM_MC_ENDIF();
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP_THEN_POP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdd !11/0. */
FNIEMOP_DEF_1(iemOp_fst_m64r,    uint8_t, bRm)
{
    IEMOP_MNEMONIC(fst_m64r, "fst m64r");
    IEM_MC_BEGIN(3, 2);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffDst);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,    u16Fsw, 0);
    IEM_MC_ARG(PRTFLOAT64U,             pr64Dst,            1);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value,          2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_MEM_MAP(pr64Dst, IEM_ACCESS_DATA_W, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 1 /*arg*/);
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_fst_r80_to_r64, pu16Fsw, pr64Dst, pr80Value);
        IEM_MC_MEM_COMMIT_AND_UNMAP_FOR_FPU_STORE(pr64Dst, IEM_ACCESS_DATA_W, u16Fsw);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ELSE()
        IEM_MC_IF_FCW_IM()
            IEM_MC_STORE_MEM_NEG_QNAN_R64_BY_REF(pr64Dst);
            IEM_MC_MEM_COMMIT_AND_UNMAP(pr64Dst, IEM_ACCESS_DATA_W);
        IEM_MC_ENDIF();
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}




/** Opcode 0xdd !11/0. */
FNIEMOP_DEF_1(iemOp_fstp_m64r,   uint8_t, bRm)
{
    IEMOP_MNEMONIC(fstp_m64r, "fstp m64r");
    IEM_MC_BEGIN(3, 2);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffDst);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,    u16Fsw, 0);
    IEM_MC_ARG(PRTFLOAT64U,             pr64Dst,            1);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value,          2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_MEM_MAP(pr64Dst, IEM_ACCESS_DATA_W, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 1 /*arg*/);
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_fst_r80_to_r64, pu16Fsw, pr64Dst, pr80Value);
        IEM_MC_MEM_COMMIT_AND_UNMAP_FOR_FPU_STORE(pr64Dst, IEM_ACCESS_DATA_W, u16Fsw);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP_THEN_POP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ELSE()
        IEM_MC_IF_FCW_IM()
            IEM_MC_STORE_MEM_NEG_QNAN_R64_BY_REF(pr64Dst);
            IEM_MC_MEM_COMMIT_AND_UNMAP(pr64Dst, IEM_ACCESS_DATA_W);
        IEM_MC_ENDIF();
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP_THEN_POP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdd !11/0. */
FNIEMOP_DEF_1(iemOp_frstor,      uint8_t, bRm)
{
    IEMOP_MNEMONIC(frstor, "frstor m94/108byte");
    IEM_MC_BEGIN(3, 0);
    IEM_MC_ARG_CONST(IEMMODE,           enmEffOpSize, /*=*/ pVCpu->iem.s.enmEffOpSize,  0);
    IEM_MC_ARG(uint8_t,                 iEffSeg,                                    1);
    IEM_MC_ARG(RTGCPTR,                 GCPtrEffSrc,                                2);
    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_ACTUALIZE_FPU_STATE_FOR_CHANGE();
    IEM_MC_ASSIGN(iEffSeg, pVCpu->iem.s.iEffSeg);
    IEM_MC_CALL_CIMPL_3(iemCImpl_frstor, enmEffOpSize, iEffSeg, GCPtrEffSrc);
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0xdd !11/0. */
FNIEMOP_DEF_1(iemOp_fnsave,      uint8_t, bRm)
{
    IEMOP_MNEMONIC(fnsave, "fnsave m94/108byte");
    IEM_MC_BEGIN(3, 0);
    IEM_MC_ARG_CONST(IEMMODE,           enmEffOpSize, /*=*/ pVCpu->iem.s.enmEffOpSize,  0);
    IEM_MC_ARG(uint8_t,                 iEffSeg,                                    1);
    IEM_MC_ARG(RTGCPTR,                 GCPtrEffDst,                                2);
    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_ACTUALIZE_FPU_STATE_FOR_CHANGE(); /* Note! Implicit fninit after the save, do not use FOR_READ here! */
    IEM_MC_ASSIGN(iEffSeg, pVCpu->iem.s.iEffSeg);
    IEM_MC_CALL_CIMPL_3(iemCImpl_fnsave, enmEffOpSize, iEffSeg, GCPtrEffDst);
    IEM_MC_END();
    return VINF_SUCCESS;

}

/** Opcode 0xdd !11/0. */
FNIEMOP_DEF_1(iemOp_fnstsw,      uint8_t, bRm)
{
    IEMOP_MNEMONIC(fnstsw_m16, "fnstsw m16");

    IEM_MC_BEGIN(0, 2);
    IEM_MC_LOCAL(uint16_t, u16Tmp);
    IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();

    IEM_MC_ACTUALIZE_FPU_STATE_FOR_READ();
    IEM_MC_FETCH_FSW(u16Tmp);
    IEM_MC_STORE_MEM_U16(pVCpu->iem.s.iEffSeg, GCPtrEffDst, u16Tmp);
    IEM_MC_ADVANCE_RIP_AND_FINISH();

/** @todo Debug / drop a hint to the verifier that things may differ
 * from REM. Seen 0x4020 (iem) vs 0x4000 (rem) at 0008:801c6b88 booting
 * NT4SP1. (X86_FSW_PE) */
    IEM_MC_END();
}


/** Opcode 0xdd 11/0. */
FNIEMOP_DEF_1(iemOp_ffree_stN,   uint8_t, bRm)
{
    IEMOP_MNEMONIC(ffree_stN, "ffree stN");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    /* Note! C0, C1, C2 and C3 are documented as undefined, we leave the
             unmodified. */

    IEM_MC_BEGIN(0, 0);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_ACTUALIZE_FPU_STATE_FOR_CHANGE();
    IEM_MC_FPU_STACK_FREE(IEM_GET_MODRM_RM_8(bRm));
    IEM_MC_UPDATE_FPU_OPCODE_IP();

    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/** Opcode 0xdd 11/1. */
FNIEMOP_DEF_1(iemOp_fst_stN,     uint8_t, bRm)
{
    IEMOP_MNEMONIC(fst_st0_stN, "fst st0,stN");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(0, 2);
    IEM_MC_LOCAL(PCRTFLOAT80U,          pr80Value);
    IEM_MC_LOCAL(IEMFPURESULT,          FpuRes);
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value, 0)
        IEM_MC_SET_FPU_RESULT(FpuRes, 0 /*FSW*/, pr80Value);
        IEM_MC_STORE_FPU_RESULT(FpuRes, IEM_GET_MODRM_RM_8(bRm));
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW(IEM_GET_MODRM_RM_8(bRm));
    IEM_MC_ENDIF();

    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/** Opcode 0xdd 11/3. */
FNIEMOP_DEF_1(iemOp_fucom_stN_st0, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fucom_st0_stN, "fucom st0,stN");
    return FNIEMOP_CALL_2(iemOpHlpFpuNoStore_st0_stN, bRm, iemAImpl_fucom_r80_by_r80);
}


/** Opcode 0xdd 11/4. */
FNIEMOP_DEF_1(iemOp_fucomp_stN,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fucomp_st0_stN, "fucomp st0,stN");
    return FNIEMOP_CALL_2(iemOpHlpFpuNoStore_st0_stN_pop, bRm, iemAImpl_fucom_r80_by_r80);
}


/**
 * @opcode      0xdd
 */
FNIEMOP_DEF(iemOp_EscF5)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    pVCpu->iem.s.uFpuOpcode = RT_MAKE_U16(bRm, 0xdd & 0x7);
    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        switch (IEM_GET_MODRM_REG_8(bRm))
        {
            case 0: return FNIEMOP_CALL_1(iemOp_ffree_stN,   bRm);
            case 1: return FNIEMOP_CALL_1(iemOp_fxch_stN,    bRm); /* Reserved, intel behavior is that of XCHG ST(i). */
            case 2: return FNIEMOP_CALL_1(iemOp_fst_stN,     bRm);
            case 3: return FNIEMOP_CALL_1(iemOp_fstp_stN,    bRm);
            case 4: return FNIEMOP_CALL_1(iemOp_fucom_stN_st0,bRm);
            case 5: return FNIEMOP_CALL_1(iemOp_fucomp_stN,  bRm);
            case 6: return IEMOP_RAISE_INVALID_OPCODE();
            case 7: return IEMOP_RAISE_INVALID_OPCODE();
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        switch (IEM_GET_MODRM_REG_8(bRm))
        {
            case 0: return FNIEMOP_CALL_1(iemOp_fld_m64r,    bRm);
            case 1: return FNIEMOP_CALL_1(iemOp_fisttp_m64i, bRm);
            case 2: return FNIEMOP_CALL_1(iemOp_fst_m64r,    bRm);
            case 3: return FNIEMOP_CALL_1(iemOp_fstp_m64r,   bRm);
            case 4: return FNIEMOP_CALL_1(iemOp_frstor,      bRm);
            case 5: return IEMOP_RAISE_INVALID_OPCODE();
            case 6: return FNIEMOP_CALL_1(iemOp_fnsave,      bRm);
            case 7: return FNIEMOP_CALL_1(iemOp_fnstsw,      bRm);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/** Opcode 0xde 11/0. */
FNIEMOP_DEF_1(iemOp_faddp_stN_st0, uint8_t, bRm)
{
    IEMOP_MNEMONIC(faddp_stN_st0, "faddp stN,st0");
    return FNIEMOP_CALL_2(iemOpHlpFpu_stN_st0_pop, bRm, iemAImpl_fadd_r80_by_r80);
}


/** Opcode 0xde 11/0. */
FNIEMOP_DEF_1(iemOp_fmulp_stN_st0, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fmulp_stN_st0, "fmulp stN,st0");
    return FNIEMOP_CALL_2(iemOpHlpFpu_stN_st0_pop, bRm, iemAImpl_fmul_r80_by_r80);
}


/** Opcode 0xde 0xd9. */
FNIEMOP_DEF(iemOp_fcompp)
{
    IEMOP_MNEMONIC(fcompp, "fcompp");
    return FNIEMOP_CALL_1(iemOpHlpFpuNoStore_st0_st1_pop_pop, iemAImpl_fcom_r80_by_r80);
}


/** Opcode 0xde 11/4. */
FNIEMOP_DEF_1(iemOp_fsubrp_stN_st0, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fsubrp_stN_st0, "fsubrp stN,st0");
    return FNIEMOP_CALL_2(iemOpHlpFpu_stN_st0_pop, bRm, iemAImpl_fsubr_r80_by_r80);
}


/** Opcode 0xde 11/5. */
FNIEMOP_DEF_1(iemOp_fsubp_stN_st0, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fsubp_stN_st0, "fsubp stN,st0");
    return FNIEMOP_CALL_2(iemOpHlpFpu_stN_st0_pop, bRm, iemAImpl_fsub_r80_by_r80);
}


/** Opcode 0xde 11/6. */
FNIEMOP_DEF_1(iemOp_fdivrp_stN_st0, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fdivrp_stN_st0, "fdivrp stN,st0");
    return FNIEMOP_CALL_2(iemOpHlpFpu_stN_st0_pop, bRm, iemAImpl_fdivr_r80_by_r80);
}


/** Opcode 0xde 11/7. */
FNIEMOP_DEF_1(iemOp_fdivp_stN_st0, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fdivp_stN_st0, "fdivp stN,st0");
    return FNIEMOP_CALL_2(iemOpHlpFpu_stN_st0_pop, bRm, iemAImpl_fdiv_r80_by_r80);
}


/**
 * Common worker for FPU instructions working on ST0 and an m16i, and storing
 * the result in ST0.
 *
 * @param   bRm         Mod R/M byte.
 * @param   pfnAImpl    Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_2(iemOpHlpFpu_st0_m16i, uint8_t, bRm, PFNIEMAIMPLFPUI16, pfnAImpl)
{
    IEM_MC_BEGIN(3, 3);
    IEM_MC_LOCAL(RTGCPTR,                   GCPtrEffSrc);
    IEM_MC_LOCAL(IEMFPURESULT,              FpuRes);
    IEM_MC_LOCAL(int16_t,                   i16Val2);
    IEM_MC_ARG_LOCAL_REF(PIEMFPURESULT,     pFpuRes,        FpuRes,     0);
    IEM_MC_ARG(PCRTFLOAT80U,                pr80Value1,                 1);
    IEM_MC_ARG_LOCAL_REF(int16_t const *,   pi16Val2,       i16Val2,    2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_FETCH_MEM_I16(i16Val2, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value1, 0)
        IEM_MC_CALL_FPU_AIMPL_3(pfnAImpl, pFpuRes, pr80Value1, pi16Val2);
        IEM_MC_STORE_FPU_RESULT(FpuRes, 0);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW(0);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xde !11/0. */
FNIEMOP_DEF_1(iemOp_fiadd_m16i,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fiadd_m16i, "fiadd m16i");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_m16i, bRm, iemAImpl_fiadd_r80_by_i16);
}


/** Opcode 0xde !11/1. */
FNIEMOP_DEF_1(iemOp_fimul_m16i,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fimul_m16i, "fimul m16i");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_m16i, bRm, iemAImpl_fimul_r80_by_i16);
}


/** Opcode 0xde !11/2. */
FNIEMOP_DEF_1(iemOp_ficom_m16i,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(ficom_st0_m16i, "ficom st0,m16i");

    IEM_MC_BEGIN(3, 3);
    IEM_MC_LOCAL(RTGCPTR,                   GCPtrEffSrc);
    IEM_MC_LOCAL(uint16_t,                  u16Fsw);
    IEM_MC_LOCAL(int16_t,                   i16Val2);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,        pu16Fsw,        u16Fsw,     0);
    IEM_MC_ARG(PCRTFLOAT80U,                pr80Value1,                 1);
    IEM_MC_ARG_LOCAL_REF(int16_t const *,   pi16Val2,       i16Val2,    2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_FETCH_MEM_I16(i16Val2, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value1, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_ficom_r80_by_i16, pu16Fsw, pr80Value1, pi16Val2);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xde !11/3. */
FNIEMOP_DEF_1(iemOp_ficomp_m16i, uint8_t, bRm)
{
    IEMOP_MNEMONIC(ficomp_st0_m16i, "ficomp st0,m16i");

    IEM_MC_BEGIN(3, 3);
    IEM_MC_LOCAL(RTGCPTR,                   GCPtrEffSrc);
    IEM_MC_LOCAL(uint16_t,                  u16Fsw);
    IEM_MC_LOCAL(int16_t,                   i16Val2);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,        pu16Fsw,        u16Fsw,     0);
    IEM_MC_ARG(PCRTFLOAT80U,                pr80Value1,                 1);
    IEM_MC_ARG_LOCAL_REF(int16_t const *,   pi16Val2,       i16Val2,    2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_FETCH_MEM_I16(i16Val2, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value1, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_ficom_r80_by_i16, pu16Fsw, pr80Value1, pi16Val2);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP_THEN_POP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP_THEN_POP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xde !11/4. */
FNIEMOP_DEF_1(iemOp_fisub_m16i,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fisub_m16i, "fisub m16i");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_m16i, bRm, iemAImpl_fisub_r80_by_i16);
}


/** Opcode 0xde !11/5. */
FNIEMOP_DEF_1(iemOp_fisubr_m16i, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fisubr_m16i, "fisubr m16i");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_m16i, bRm, iemAImpl_fisubr_r80_by_i16);
}


/** Opcode 0xde !11/6. */
FNIEMOP_DEF_1(iemOp_fidiv_m16i,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fidiv_m16i, "fidiv m16i");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_m16i, bRm, iemAImpl_fidiv_r80_by_i16);
}


/** Opcode 0xde !11/7. */
FNIEMOP_DEF_1(iemOp_fidivr_m16i, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fidivr_m16i, "fidivr m16i");
    return FNIEMOP_CALL_2(iemOpHlpFpu_st0_m16i, bRm, iemAImpl_fidivr_r80_by_i16);
}


/**
 * @opcode      0xde
 */
FNIEMOP_DEF(iemOp_EscF6)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    pVCpu->iem.s.uFpuOpcode = RT_MAKE_U16(bRm, 0xde & 0x7);
    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        switch (IEM_GET_MODRM_REG_8(bRm))
        {
            case 0: return FNIEMOP_CALL_1(iemOp_faddp_stN_st0, bRm);
            case 1: return FNIEMOP_CALL_1(iemOp_fmulp_stN_st0, bRm);
            case 2: return FNIEMOP_CALL_1(iemOp_fcomp_stN, bRm);
            case 3: if (bRm == 0xd9)
                        return FNIEMOP_CALL(iemOp_fcompp);
                    return IEMOP_RAISE_INVALID_OPCODE();
            case 4: return FNIEMOP_CALL_1(iemOp_fsubrp_stN_st0, bRm);
            case 5: return FNIEMOP_CALL_1(iemOp_fsubp_stN_st0, bRm);
            case 6: return FNIEMOP_CALL_1(iemOp_fdivrp_stN_st0, bRm);
            case 7: return FNIEMOP_CALL_1(iemOp_fdivp_stN_st0, bRm);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        switch (IEM_GET_MODRM_REG_8(bRm))
        {
            case 0: return FNIEMOP_CALL_1(iemOp_fiadd_m16i,  bRm);
            case 1: return FNIEMOP_CALL_1(iemOp_fimul_m16i,  bRm);
            case 2: return FNIEMOP_CALL_1(iemOp_ficom_m16i,  bRm);
            case 3: return FNIEMOP_CALL_1(iemOp_ficomp_m16i, bRm);
            case 4: return FNIEMOP_CALL_1(iemOp_fisub_m16i,  bRm);
            case 5: return FNIEMOP_CALL_1(iemOp_fisubr_m16i, bRm);
            case 6: return FNIEMOP_CALL_1(iemOp_fidiv_m16i,  bRm);
            case 7: return FNIEMOP_CALL_1(iemOp_fidivr_m16i, bRm);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/** Opcode 0xdf 11/0.
 * Undocument instruction, assumed to work like ffree + fincstp.  */
FNIEMOP_DEF_1(iemOp_ffreep_stN, uint8_t, bRm)
{
    IEMOP_MNEMONIC(ffreep_stN, "ffreep stN");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(0, 0);

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_ACTUALIZE_FPU_STATE_FOR_CHANGE();
    IEM_MC_FPU_STACK_FREE(IEM_GET_MODRM_RM_8(bRm));
    IEM_MC_FPU_STACK_INC_TOP();
    IEM_MC_UPDATE_FPU_OPCODE_IP();

    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/** Opcode 0xdf 0xe0. */
FNIEMOP_DEF(iemOp_fnstsw_ax)
{
    IEMOP_MNEMONIC(fnstsw_ax, "fnstsw ax");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(0, 1);
    IEM_MC_LOCAL(uint16_t, u16Tmp);
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_ACTUALIZE_FPU_STATE_FOR_READ();
    IEM_MC_FETCH_FSW(u16Tmp);
    IEM_MC_STORE_GREG_U16(X86_GREG_xAX, u16Tmp);
    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/** Opcode 0xdf 11/5. */
FNIEMOP_DEF_1(iemOp_fucomip_st0_stN, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fucomip_st0_stN, "fucomip st0,stN");
    return IEM_MC_DEFER_TO_CIMPL_3(iemCImpl_fcomi_fucomi, IEM_GET_MODRM_RM_8(bRm), iemAImpl_fcomi_r80_by_r80, true /*fPop*/);
}


/** Opcode 0xdf 11/6. */
FNIEMOP_DEF_1(iemOp_fcomip_st0_stN,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fcomip_st0_stN, "fcomip st0,stN");
    return IEM_MC_DEFER_TO_CIMPL_3(iemCImpl_fcomi_fucomi, IEM_GET_MODRM_RM_8(bRm), iemAImpl_fcomi_r80_by_r80, true /*fPop*/);
}


/** Opcode 0xdf !11/0. */
FNIEMOP_DEF_1(iemOp_fild_m16i, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fild_m16i, "fild m16i");

    IEM_MC_BEGIN(2, 3);
    IEM_MC_LOCAL(RTGCPTR,                   GCPtrEffSrc);
    IEM_MC_LOCAL(IEMFPURESULT,              FpuRes);
    IEM_MC_LOCAL(int16_t,                   i16Val);
    IEM_MC_ARG_LOCAL_REF(PIEMFPURESULT,     pFpuRes,    FpuRes, 0);
    IEM_MC_ARG_LOCAL_REF(int16_t const *,   pi16Val,    i16Val, 1);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_FETCH_MEM_I16(i16Val, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_IS_EMPTY(7)
        IEM_MC_CALL_FPU_AIMPL_2(iemAImpl_fild_r80_from_i16, pFpuRes, pi16Val);
        IEM_MC_PUSH_FPU_RESULT_MEM_OP(FpuRes, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_PUSH_OVERFLOW_MEM_OP(pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdf !11/1. */
FNIEMOP_DEF_1(iemOp_fisttp_m16i, uint8_t, bRm)
{
    IEMOP_MNEMONIC(fisttp_m16i, "fisttp m16i");
    IEM_MC_BEGIN(3, 2);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffDst);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,    u16Fsw, 0);
    IEM_MC_ARG(int16_t *,               pi16Dst,            1);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value,          2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_MEM_MAP(pi16Dst, IEM_ACCESS_DATA_W, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 1 /*arg*/);
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_fistt_r80_to_i16, pu16Fsw, pi16Dst, pr80Value);
        IEM_MC_MEM_COMMIT_AND_UNMAP_FOR_FPU_STORE(pi16Dst, IEM_ACCESS_DATA_W, u16Fsw);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP_THEN_POP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ELSE()
        IEM_MC_IF_FCW_IM()
            IEM_MC_STORE_MEM_I16_CONST_BY_REF(pi16Dst, INT16_MIN /* (integer indefinite) */);
            IEM_MC_MEM_COMMIT_AND_UNMAP(pi16Dst, IEM_ACCESS_DATA_W);
        IEM_MC_ENDIF();
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP_THEN_POP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdf !11/2. */
FNIEMOP_DEF_1(iemOp_fist_m16i,   uint8_t, bRm)
{
    IEMOP_MNEMONIC(fist_m16i, "fist m16i");
    IEM_MC_BEGIN(3, 2);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffDst);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,    u16Fsw, 0);
    IEM_MC_ARG(int16_t *,               pi16Dst,            1);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value,          2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_MEM_MAP(pi16Dst, IEM_ACCESS_DATA_W, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 1 /*arg*/);
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_fist_r80_to_i16, pu16Fsw, pi16Dst, pr80Value);
        IEM_MC_MEM_COMMIT_AND_UNMAP_FOR_FPU_STORE(pi16Dst, IEM_ACCESS_DATA_W, u16Fsw);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ELSE()
        IEM_MC_IF_FCW_IM()
            IEM_MC_STORE_MEM_I16_CONST_BY_REF(pi16Dst, INT16_MIN /* (integer indefinite) */);
            IEM_MC_MEM_COMMIT_AND_UNMAP(pi16Dst, IEM_ACCESS_DATA_W);
        IEM_MC_ENDIF();
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdf !11/3. */
FNIEMOP_DEF_1(iemOp_fistp_m16i,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fistp_m16i, "fistp m16i");
    IEM_MC_BEGIN(3, 2);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffDst);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,    u16Fsw, 0);
    IEM_MC_ARG(int16_t *,               pi16Dst,            1);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value,          2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_MEM_MAP(pi16Dst, IEM_ACCESS_DATA_W, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 1 /*arg*/);
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_fist_r80_to_i16, pu16Fsw, pi16Dst, pr80Value);
        IEM_MC_MEM_COMMIT_AND_UNMAP_FOR_FPU_STORE(pi16Dst, IEM_ACCESS_DATA_W, u16Fsw);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP_THEN_POP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ELSE()
        IEM_MC_IF_FCW_IM()
            IEM_MC_STORE_MEM_I16_CONST_BY_REF(pi16Dst, INT16_MIN /* (integer indefinite) */);
            IEM_MC_MEM_COMMIT_AND_UNMAP(pi16Dst, IEM_ACCESS_DATA_W);
        IEM_MC_ENDIF();
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP_THEN_POP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdf !11/4. */
FNIEMOP_DEF_1(iemOp_fbld_m80d,   uint8_t, bRm)
{
    IEMOP_MNEMONIC(fbld_m80d, "fbld m80d");

    IEM_MC_BEGIN(2, 3);
    IEM_MC_LOCAL(RTGCPTR,                   GCPtrEffSrc);
    IEM_MC_LOCAL(IEMFPURESULT,              FpuRes);
    IEM_MC_LOCAL(RTPBCD80U,                 d80Val);
    IEM_MC_ARG_LOCAL_REF(PIEMFPURESULT,     pFpuRes,    FpuRes,     0);
    IEM_MC_ARG_LOCAL_REF(PCRTPBCD80U,       pd80Val,    d80Val,     1);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_FETCH_MEM_D80(d80Val, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_IS_EMPTY(7)
        IEM_MC_CALL_FPU_AIMPL_2(iemAImpl_fld_r80_from_d80, pFpuRes, pd80Val);
        IEM_MC_PUSH_FPU_RESULT_MEM_OP(FpuRes, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_PUSH_OVERFLOW_MEM_OP(pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdf !11/5. */
FNIEMOP_DEF_1(iemOp_fild_m64i,   uint8_t, bRm)
{
    IEMOP_MNEMONIC(fild_m64i, "fild m64i");

    IEM_MC_BEGIN(2, 3);
    IEM_MC_LOCAL(RTGCPTR,                   GCPtrEffSrc);
    IEM_MC_LOCAL(IEMFPURESULT,              FpuRes);
    IEM_MC_LOCAL(int64_t,                   i64Val);
    IEM_MC_ARG_LOCAL_REF(PIEMFPURESULT,     pFpuRes,    FpuRes, 0);
    IEM_MC_ARG_LOCAL_REF(int64_t const *,   pi64Val,    i64Val, 1);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();
    IEM_MC_FETCH_MEM_I64(i64Val, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);

    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_IS_EMPTY(7)
        IEM_MC_CALL_FPU_AIMPL_2(iemAImpl_fild_r80_from_i64, pFpuRes, pi64Val);
        IEM_MC_PUSH_FPU_RESULT_MEM_OP(FpuRes, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ELSE()
        IEM_MC_FPU_STACK_PUSH_OVERFLOW_MEM_OP(pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdf !11/6. */
FNIEMOP_DEF_1(iemOp_fbstp_m80d,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fbstp_m80d, "fbstp m80d");
    IEM_MC_BEGIN(3, 2);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffDst);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,    u16Fsw, 0);
    IEM_MC_ARG(PRTPBCD80U,              pd80Dst,            1);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value,          2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_MEM_MAP_EX(pd80Dst, IEM_ACCESS_DATA_W, sizeof(*pd80Dst), pVCpu->iem.s.iEffSeg, GCPtrEffDst, 7 /*cbAlign*/, 1 /*arg*/);
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_fst_r80_to_d80, pu16Fsw, pd80Dst, pr80Value);
        IEM_MC_MEM_COMMIT_AND_UNMAP_FOR_FPU_STORE(pd80Dst, IEM_ACCESS_DATA_W, u16Fsw);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP_THEN_POP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ELSE()
        IEM_MC_IF_FCW_IM()
            IEM_MC_STORE_MEM_INDEF_D80_BY_REF(pd80Dst);
            IEM_MC_MEM_COMMIT_AND_UNMAP(pd80Dst, IEM_ACCESS_DATA_W);
        IEM_MC_ENDIF();
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP_THEN_POP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/** Opcode 0xdf !11/7. */
FNIEMOP_DEF_1(iemOp_fistp_m64i,  uint8_t, bRm)
{
    IEMOP_MNEMONIC(fistp_m64i, "fistp m64i");
    IEM_MC_BEGIN(3, 2);
    IEM_MC_LOCAL(RTGCPTR,               GCPtrEffDst);
    IEM_MC_LOCAL(uint16_t,              u16Fsw);
    IEM_MC_ARG_LOCAL_REF(uint16_t *,    pu16Fsw,    u16Fsw, 0);
    IEM_MC_ARG(int64_t *,               pi64Dst,            1);
    IEM_MC_ARG(PCRTFLOAT80U,            pr80Value,          2);

    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE();
    IEM_MC_MAYBE_RAISE_FPU_XCPT();

    IEM_MC_MEM_MAP(pi64Dst, IEM_ACCESS_DATA_W, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 1 /*arg*/);
    IEM_MC_PREPARE_FPU_USAGE();
    IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(pr80Value, 0)
        IEM_MC_CALL_FPU_AIMPL_3(iemAImpl_fist_r80_to_i64, pu16Fsw, pi64Dst, pr80Value);
        IEM_MC_MEM_COMMIT_AND_UNMAP_FOR_FPU_STORE(pi64Dst, IEM_ACCESS_DATA_W, u16Fsw);
        IEM_MC_UPDATE_FSW_WITH_MEM_OP_THEN_POP(u16Fsw, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ELSE()
        IEM_MC_IF_FCW_IM()
            IEM_MC_STORE_MEM_I64_CONST_BY_REF(pi64Dst, INT64_MIN /* (integer indefinite) */);
            IEM_MC_MEM_COMMIT_AND_UNMAP(pi64Dst, IEM_ACCESS_DATA_W);
        IEM_MC_ENDIF();
        IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP_THEN_POP(UINT8_MAX, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
    IEM_MC_ENDIF();
    IEM_MC_ADVANCE_RIP_AND_FINISH();

    IEM_MC_END();
}


/**
 * @opcode      0xdf
 */
FNIEMOP_DEF(iemOp_EscF7)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        switch (IEM_GET_MODRM_REG_8(bRm))
        {
            case 0: return FNIEMOP_CALL_1(iemOp_ffreep_stN, bRm); /* ffree + pop afterwards, since forever according to AMD. */
            case 1: return FNIEMOP_CALL_1(iemOp_fxch_stN,   bRm); /* Reserved, behaves like FXCH ST(i) on intel. */
            case 2: return FNIEMOP_CALL_1(iemOp_fstp_stN,   bRm); /* Reserved, behaves like FSTP ST(i) on intel. */
            case 3: return FNIEMOP_CALL_1(iemOp_fstp_stN,   bRm); /* Reserved, behaves like FSTP ST(i) on intel. */
            case 4: if (bRm == 0xe0)
                        return FNIEMOP_CALL(iemOp_fnstsw_ax);
                    return IEMOP_RAISE_INVALID_OPCODE();
            case 5: return FNIEMOP_CALL_1(iemOp_fucomip_st0_stN, bRm);
            case 6: return FNIEMOP_CALL_1(iemOp_fcomip_st0_stN,  bRm);
            case 7: return IEMOP_RAISE_INVALID_OPCODE();
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        switch (IEM_GET_MODRM_REG_8(bRm))
        {
            case 0: return FNIEMOP_CALL_1(iemOp_fild_m16i,   bRm);
            case 1: return FNIEMOP_CALL_1(iemOp_fisttp_m16i, bRm);
            case 2: return FNIEMOP_CALL_1(iemOp_fist_m16i,   bRm);
            case 3: return FNIEMOP_CALL_1(iemOp_fistp_m16i,  bRm);
            case 4: return FNIEMOP_CALL_1(iemOp_fbld_m80d,   bRm);
            case 5: return FNIEMOP_CALL_1(iemOp_fild_m64i,   bRm);
            case 6: return FNIEMOP_CALL_1(iemOp_fbstp_m80d,  bRm);
            case 7: return FNIEMOP_CALL_1(iemOp_fistp_m64i,  bRm);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/**
 * @opcode      0xe0
 */
FNIEMOP_DEF(iemOp_loopne_Jb)
{
    IEMOP_MNEMONIC(loopne_Jb, "loopne Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    switch (pVCpu->iem.s.enmEffAddrMode)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_SUB_GREG_U16(X86_GREG_xCX, 1);
            IEM_MC_IF_CX_IS_NZ_AND_EFL_BIT_NOT_SET(X86_EFL_ZF) {
                IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP_AND_FINISH();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            break;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_SUB_GREG_U32(X86_GREG_xCX, 1);
            IEM_MC_IF_ECX_IS_NZ_AND_EFL_BIT_NOT_SET(X86_EFL_ZF) {
                IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP_AND_FINISH();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            break;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_SUB_GREG_U64(X86_GREG_xCX, 1);
            IEM_MC_IF_RCX_IS_NZ_AND_EFL_BIT_NOT_SET(X86_EFL_ZF) {
                IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP_AND_FINISH();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            break;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0xe1
 */
FNIEMOP_DEF(iemOp_loope_Jb)
{
    IEMOP_MNEMONIC(loope_Jb, "loope Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    switch (pVCpu->iem.s.enmEffAddrMode)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_SUB_GREG_U16(X86_GREG_xCX, 1);
            IEM_MC_IF_CX_IS_NZ_AND_EFL_BIT_SET(X86_EFL_ZF) {
                IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP_AND_FINISH();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            break;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_SUB_GREG_U32(X86_GREG_xCX, 1);
            IEM_MC_IF_ECX_IS_NZ_AND_EFL_BIT_SET(X86_EFL_ZF) {
                IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP_AND_FINISH();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            break;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_SUB_GREG_U64(X86_GREG_xCX, 1);
            IEM_MC_IF_RCX_IS_NZ_AND_EFL_BIT_SET(X86_EFL_ZF) {
                IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP_AND_FINISH();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            break;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0xe2
 */
FNIEMOP_DEF(iemOp_loop_Jb)
{
    IEMOP_MNEMONIC(loop_Jb, "loop Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    /** @todo Check out the \#GP case if EIP < CS.Base or EIP > CS.Limit when
     * using the 32-bit operand size override.  How can that be restarted?  See
     * weird pseudo code in intel manual. */

    /* NB: At least Windows for Workgroups 3.11 (NDIS.386) and Windows 95 (NDIS.VXD, IOS)
     * use LOOP $-2 to implement NdisStallExecution and other CPU stall APIs. Shortcutting
     * the loop causes guest crashes, but when logging it's nice to skip a few million
     * lines of useless output. */
#if defined(LOG_ENABLED)
    if ((LogIs3Enabled() || LogIs4Enabled()) && (-(int8_t)IEM_GET_INSTR_LEN(pVCpu) == i8Imm))
        switch (pVCpu->iem.s.enmEffAddrMode)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(0,0);
                IEM_MC_STORE_GREG_U16_CONST(X86_GREG_xCX, 0);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(0,0);
                IEM_MC_STORE_GREG_U32_CONST(X86_GREG_xCX, 0);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(0,0);
                IEM_MC_STORE_GREG_U64_CONST(X86_GREG_xCX, 0);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
#endif

    switch (pVCpu->iem.s.enmEffAddrMode)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0,0);

            IEM_MC_SUB_GREG_U16(X86_GREG_xCX, 1);
            IEM_MC_IF_CX_IS_NZ() {
                IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP_AND_FINISH();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            break;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_SUB_GREG_U32(X86_GREG_xCX, 1);
            IEM_MC_IF_ECX_IS_NZ() {
                IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP_AND_FINISH();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            break;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_SUB_GREG_U64(X86_GREG_xCX, 1);
            IEM_MC_IF_RCX_IS_NZ() {
                IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP_AND_FINISH();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            break;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0xe3
 */
FNIEMOP_DEF(iemOp_jecxz_Jb)
{
    IEMOP_MNEMONIC(jecxz_Jb, "jecxz Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    switch (pVCpu->iem.s.enmEffAddrMode)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_IF_CX_IS_NZ() {
                IEM_MC_ADVANCE_RIP_AND_FINISH();
            } IEM_MC_ELSE() {
                IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
            } IEM_MC_ENDIF();
            IEM_MC_END();
            break;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_IF_ECX_IS_NZ() {
                IEM_MC_ADVANCE_RIP_AND_FINISH();
            } IEM_MC_ELSE() {
                IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
            } IEM_MC_ENDIF();
            IEM_MC_END();
            break;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_IF_RCX_IS_NZ() {
                IEM_MC_ADVANCE_RIP_AND_FINISH();
            } IEM_MC_ELSE() {
                IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
            } IEM_MC_ENDIF();
            IEM_MC_END();
            break;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/** Opcode 0xe4 */
FNIEMOP_DEF(iemOp_in_AL_Ib)
{
    IEMOP_MNEMONIC(in_AL_Ib, "in AL,Ib");
    uint8_t u8Imm; IEM_OPCODE_GET_NEXT_U8(&u8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_3(iemCImpl_in, u8Imm, true /* fImm */, 1);
}


/** Opcode 0xe5 */
FNIEMOP_DEF(iemOp_in_eAX_Ib)
{
    IEMOP_MNEMONIC(in_eAX_Ib, "in eAX,Ib");
    uint8_t u8Imm; IEM_OPCODE_GET_NEXT_U8(&u8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_3(iemCImpl_in, u8Imm, true /* fImm */, pVCpu->iem.s.enmEffOpSize == IEMMODE_16BIT ? 2 : 4);
}


/** Opcode 0xe6 */
FNIEMOP_DEF(iemOp_out_Ib_AL)
{
    IEMOP_MNEMONIC(out_Ib_AL, "out Ib,AL");
    uint8_t u8Imm; IEM_OPCODE_GET_NEXT_U8(&u8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_3(iemCImpl_out, u8Imm, true /* fImm */, 1);
}


/** Opcode 0xe7 */
FNIEMOP_DEF(iemOp_out_Ib_eAX)
{
    IEMOP_MNEMONIC(out_Ib_eAX, "out Ib,eAX");
    uint8_t u8Imm; IEM_OPCODE_GET_NEXT_U8(&u8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_3(iemCImpl_out, u8Imm, true /* fImm */, pVCpu->iem.s.enmEffOpSize == IEMMODE_16BIT ? 2 : 4);
}


/**
 * @opcode      0xe8
 */
FNIEMOP_DEF(iemOp_call_Jv)
{
    IEMOP_MNEMONIC(call_Jv, "call Jv");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
        {
            uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(&u16Imm);
            return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_call_rel_16, (int16_t)u16Imm);
        }

        case IEMMODE_32BIT:
        {
            uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(&u32Imm);
            return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_call_rel_32, (int32_t)u32Imm);
        }

        case IEMMODE_64BIT:
        {
            uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S32_SX_U64(&u64Imm);
            return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_call_rel_64, u64Imm);
        }

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0xe9
 */
FNIEMOP_DEF(iemOp_jmp_Jv)
{
    IEMOP_MNEMONIC(jmp_Jv, "jmp Jv");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
        {
            int16_t i16Imm; IEM_OPCODE_GET_NEXT_S16(&i16Imm);
            IEM_MC_BEGIN(0, 0);
            IEM_MC_REL_JMP_S16_AND_FINISH(i16Imm);
            IEM_MC_END();
            return VINF_SUCCESS;
        }

        case IEMMODE_64BIT:
        case IEMMODE_32BIT:
        {
            int32_t i32Imm; IEM_OPCODE_GET_NEXT_S32(&i32Imm);
            IEM_MC_BEGIN(0, 0);
            IEM_MC_REL_JMP_S32_AND_FINISH(i32Imm);
            IEM_MC_END();
            return VINF_SUCCESS;
        }

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0xea
 */
FNIEMOP_DEF(iemOp_jmp_Ap)
{
    IEMOP_MNEMONIC(jmp_Ap, "jmp Ap");
    IEMOP_HLP_NO_64BIT();

    /* Decode the far pointer address and pass it on to the far call C implementation. */
    uint32_t offSeg;
    if (pVCpu->iem.s.enmEffOpSize != IEMMODE_16BIT)
        IEM_OPCODE_GET_NEXT_U32(&offSeg);
    else
        IEM_OPCODE_GET_NEXT_U16_ZX_U32(&offSeg);
    uint16_t uSel;  IEM_OPCODE_GET_NEXT_U16(&uSel);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_3(iemCImpl_FarJmp, uSel, offSeg, pVCpu->iem.s.enmEffOpSize);
}


/**
 * @opcode      0xeb
 */
FNIEMOP_DEF(iemOp_jmp_Jb)
{
    IEMOP_MNEMONIC(jmp_Jb, "jmp Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(&i8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_REL_JMP_S8_AND_FINISH(i8Imm);
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0xec */
FNIEMOP_DEF(iemOp_in_AL_DX)
{
    IEMOP_MNEMONIC(in_AL_DX, "in  AL,DX");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_in_eAX_DX, 1);
}


/** Opcode 0xed */
FNIEMOP_DEF(iemOp_in_eAX_DX)
{
    IEMOP_MNEMONIC(in_eAX_DX, "in  eAX,DX");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_in_eAX_DX, pVCpu->iem.s.enmEffOpSize == IEMMODE_16BIT ? 2 : 4);
}


/** Opcode 0xee */
FNIEMOP_DEF(iemOp_out_DX_AL)
{
    IEMOP_MNEMONIC(out_DX_AL, "out DX,AL");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_out_DX_eAX, 1);
}


/** Opcode 0xef */
FNIEMOP_DEF(iemOp_out_DX_eAX)
{
    IEMOP_MNEMONIC(out_DX_eAX, "out DX,eAX");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_out_DX_eAX, pVCpu->iem.s.enmEffOpSize == IEMMODE_16BIT ? 2 : 4);
}


/**
 * @opcode      0xf0
 */
FNIEMOP_DEF(iemOp_lock)
{
    IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("lock");
    if (!pVCpu->iem.s.fDisregardLock)
        pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_LOCK;

    uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/**
 * @opcode      0xf1
 */
FNIEMOP_DEF(iemOp_int1)
{
    IEMOP_MNEMONIC(int1, "int1"); /* icebp */
    /** @todo Does not generate \#UD on 286, or so they say...  Was allegedly a
     * prefix byte on 8086 and/or/maybe 80286 without meaning according to the 286
     * LOADALL memo.  Needs some testing. */
    IEMOP_HLP_MIN_386();
    /** @todo testcase! */
    return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_int, X86_XCPT_DB, IEMINT_INT1);
}


/**
 * @opcode      0xf2
 */
FNIEMOP_DEF(iemOp_repne)
{
    /* This overrides any previous REPE prefix. */
    pVCpu->iem.s.fPrefixes &= ~IEM_OP_PRF_REPZ;
    IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("repne");
    pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_REPNZ;

    /* For the 4 entry opcode tables, REPNZ overrides any previous
       REPZ and operand size prefixes. */
    pVCpu->iem.s.idxPrefix = 3;

    uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/**
 * @opcode      0xf3
 */
FNIEMOP_DEF(iemOp_repe)
{
    /* This overrides any previous REPNE prefix. */
    pVCpu->iem.s.fPrefixes &= ~IEM_OP_PRF_REPNZ;
    IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE("repe");
    pVCpu->iem.s.fPrefixes |= IEM_OP_PRF_REPZ;

    /* For the 4 entry opcode tables, REPNZ overrides any previous
       REPNZ and operand size prefixes. */
    pVCpu->iem.s.idxPrefix = 2;

    uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/**
 * @opcode      0xf4
 */
FNIEMOP_DEF(iemOp_hlt)
{
    IEMOP_MNEMONIC(hlt, "hlt");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_hlt);
}


/**
 * @opcode      0xf5
 */
FNIEMOP_DEF(iemOp_cmc)
{
    IEMOP_MNEMONIC(cmc, "cmc");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_BEGIN(0, 0);
    IEM_MC_FLIP_EFL_BIT(X86_EFL_CF);
    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/**
 * Common implementation of 'inc/dec/not/neg Eb'.
 *
 * @param   bRm             The RM byte.
 * @param   pImpl           The instruction implementation.
 */
FNIEMOP_DEF_2(iemOpCommonUnaryEb, uint8_t, bRm, PCIEMOPUNARYSIZES, pImpl)
{
    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        /* register access */
        IEM_MC_BEGIN(2, 0);
        IEM_MC_ARG(uint8_t *,   pu8Dst, 0);
        IEM_MC_ARG(uint32_t *,  pEFlags, 1);
        IEM_MC_REF_GREG_U8(pu8Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnNormalU8, pu8Dst, pEFlags);
        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
    else
    {
        /* memory access. */
        IEM_MC_BEGIN(2, 2);
        IEM_MC_ARG(uint8_t *,       pu8Dst,          0);
        IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags, 1);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
        IEM_MC_MEM_MAP(pu8Dst, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
        IEM_MC_FETCH_EFLAGS(EFlags);
        if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_LOCK))
            IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnNormalU8, pu8Dst, pEFlags);
        else
            IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnLockedU8, pu8Dst, pEFlags);

        IEM_MC_MEM_COMMIT_AND_UNMAP(pu8Dst, IEM_ACCESS_DATA_RW);
        IEM_MC_COMMIT_EFLAGS(EFlags);
        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
}


/**
 * Common implementation of 'inc/dec/not/neg Ev'.
 *
 * @param   bRm             The RM byte.
 * @param   pImpl           The instruction implementation.
 */
FNIEMOP_DEF_2(iemOpCommonUnaryEv, uint8_t, bRm, PCIEMOPUNARYSIZES, pImpl)
{
    /* Registers are handled by a common worker. */
    if (IEM_IS_MODRM_REG_MODE(bRm))
        return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, pImpl, IEM_GET_MODRM_RM(pVCpu, bRm));

    /* Memory we do here. */
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(2, 2);
            IEM_MC_ARG(uint16_t *,      pu16Dst,         0);
            IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags, 1);
            IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
            IEM_MC_MEM_MAP(pu16Dst, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
            IEM_MC_FETCH_EFLAGS(EFlags);
            if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_LOCK))
                IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnNormalU16, pu16Dst, pEFlags);
            else
                IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnLockedU16, pu16Dst, pEFlags);

            IEM_MC_MEM_COMMIT_AND_UNMAP(pu16Dst, IEM_ACCESS_DATA_RW);
            IEM_MC_COMMIT_EFLAGS(EFlags);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(2, 2);
            IEM_MC_ARG(uint32_t *,      pu32Dst,         0);
            IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags, 1);
            IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
            IEM_MC_MEM_MAP(pu32Dst, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
            IEM_MC_FETCH_EFLAGS(EFlags);
            if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_LOCK))
                IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnNormalU32, pu32Dst, pEFlags);
            else
                IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnLockedU32, pu32Dst, pEFlags);

            IEM_MC_MEM_COMMIT_AND_UNMAP(pu32Dst, IEM_ACCESS_DATA_RW);
            IEM_MC_COMMIT_EFLAGS(EFlags);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(2, 2);
            IEM_MC_ARG(uint64_t *,      pu64Dst,         0);
            IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags, 1);
            IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
            IEM_MC_MEM_MAP(pu64Dst, IEM_ACCESS_DATA_RW, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
            IEM_MC_FETCH_EFLAGS(EFlags);
            if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_LOCK))
                IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnNormalU64, pu64Dst, pEFlags);
            else
                IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnLockedU64, pu64Dst, pEFlags);

            IEM_MC_MEM_COMMIT_AND_UNMAP(pu64Dst, IEM_ACCESS_DATA_RW);
            IEM_MC_COMMIT_EFLAGS(EFlags);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/** Opcode 0xf6 /0. */
FNIEMOP_DEF_1(iemOp_grp3_test_Eb, uint8_t, bRm)
{
    IEMOP_MNEMONIC(test_Eb_Ib, "test Eb,Ib");
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);

    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        /* register access */
        uint8_t u8Imm; IEM_OPCODE_GET_NEXT_U8(&u8Imm);
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(3, 0);
        IEM_MC_ARG(uint8_t *,       pu8Dst,             0);
        IEM_MC_ARG_CONST(uint8_t,   u8Src,/*=*/u8Imm,   1);
        IEM_MC_ARG(uint32_t *,      pEFlags,            2);
        IEM_MC_REF_GREG_U8(pu8Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_test_u8, pu8Dst, u8Src, pEFlags);
        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
    else
    {
        /* memory access. */
        IEM_MC_BEGIN(3, 2);
        IEM_MC_ARG(uint8_t *,       pu8Dst,             0);
        IEM_MC_ARG(uint8_t,         u8Src,              1);
        IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,    2);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 1);
        uint8_t u8Imm; IEM_OPCODE_GET_NEXT_U8(&u8Imm);
        IEM_MC_ASSIGN(u8Src, u8Imm);
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_MEM_MAP(pu8Dst, IEM_ACCESS_DATA_R, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
        IEM_MC_FETCH_EFLAGS(EFlags);
        IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_test_u8, pu8Dst, u8Src, pEFlags);

        IEM_MC_MEM_COMMIT_AND_UNMAP(pu8Dst, IEM_ACCESS_DATA_R);
        IEM_MC_COMMIT_EFLAGS(EFlags);
        IEM_MC_ADVANCE_RIP_AND_FINISH();
        IEM_MC_END();
    }
}


/** Opcode 0xf7 /0. */
FNIEMOP_DEF_1(iemOp_grp3_test_Ev, uint8_t, bRm)
{
    IEMOP_MNEMONIC(test_Ev_Iv, "test Ev,Iv");
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_AF);

    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        /* register access */
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
            {
                uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(&u16Imm);
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint16_t *,      pu16Dst,                0);
                IEM_MC_ARG_CONST(uint16_t,  u16Src,/*=*/u16Imm,     1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                2);
                IEM_MC_REF_GREG_U16(pu16Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_test_u16, pu16Dst, u16Src, pEFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;
            }

            case IEMMODE_32BIT:
            {
                uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(&u32Imm);
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint32_t *,      pu32Dst,                0);
                IEM_MC_ARG_CONST(uint32_t,  u32Src,/*=*/u32Imm,     1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                2);
                IEM_MC_REF_GREG_U32(pu32Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_test_u32, pu32Dst, u32Src, pEFlags);
                /* No clearing the high dword here - test doesn't write back the result. */
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;
            }

            case IEMMODE_64BIT:
            {
                uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S32_SX_U64(&u64Imm);
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint64_t *,      pu64Dst,                0);
                IEM_MC_ARG_CONST(uint64_t,  u64Src,/*=*/u64Imm,     1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                2);
                IEM_MC_REF_GREG_U64(pu64Dst, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_test_u64, pu64Dst, u64Src, pEFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;
            }

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /* memory access. */
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
            {
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint16_t *,      pu16Dst,            0);
                IEM_MC_ARG(uint16_t,        u16Src,             1);
                IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,    2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 2);
                uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(&u16Imm);
                IEM_MC_ASSIGN(u16Src, u16Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_MEM_MAP(pu16Dst, IEM_ACCESS_DATA_R, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_test_u16, pu16Dst, u16Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu16Dst, IEM_ACCESS_DATA_R);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;
            }

            case IEMMODE_32BIT:
            {
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint32_t *,      pu32Dst,            0);
                IEM_MC_ARG(uint32_t,        u32Src,             1);
                IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,    2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 4);
                uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(&u32Imm);
                IEM_MC_ASSIGN(u32Src, u32Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_MEM_MAP(pu32Dst, IEM_ACCESS_DATA_R, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_test_u32, pu32Dst, u32Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu32Dst, IEM_ACCESS_DATA_R);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;
            }

            case IEMMODE_64BIT:
            {
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint64_t *,      pu64Dst,            0);
                IEM_MC_ARG(uint64_t,        u64Src,             1);
                IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,    2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 4);
                uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S32_SX_U64(&u64Imm);
                IEM_MC_ASSIGN(u64Src, u64Imm);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_MEM_MAP(pu64Dst, IEM_ACCESS_DATA_R, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_test_u64, pu64Dst, u64Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu64Dst, IEM_ACCESS_DATA_R);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP_AND_FINISH();
                IEM_MC_END();
                break;
            }

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/** Opcode 0xf6 /4, /5, /6 and /7. */
FNIEMOP_DEF_2(iemOpCommonGrp3MulDivEb, uint8_t, bRm, PFNIEMAIMPLMULDIVU8, pfnU8)
{
    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        /* register access */
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_BEGIN(3, 1);
        IEM_MC_ARG(uint16_t *,      pu16AX,     0);
        IEM_MC_ARG(uint8_t,         u8Value,    1);
        IEM_MC_ARG(uint32_t *,      pEFlags,    2);
        IEM_MC_LOCAL(int32_t,       rc);

        IEM_MC_FETCH_GREG_U8(u8Value, IEM_GET_MODRM_RM(pVCpu, bRm));
        IEM_MC_REF_GREG_U16(pu16AX, X86_GREG_xAX);
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_AIMPL_3(rc, pfnU8, pu16AX, u8Value, pEFlags);
        IEM_MC_IF_LOCAL_IS_Z(rc) {
            IEM_MC_ADVANCE_RIP_AND_FINISH();
        } IEM_MC_ELSE() {
            IEM_MC_RAISE_DIVIDE_ERROR();
        } IEM_MC_ENDIF();

        IEM_MC_END();
    }
    else
    {
        /* memory access. */
        IEM_MC_BEGIN(3, 2);
        IEM_MC_ARG(uint16_t *,      pu16AX,     0);
        IEM_MC_ARG(uint8_t,         u8Value,    1);
        IEM_MC_ARG(uint32_t *,      pEFlags,    2);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
        IEM_MC_LOCAL(int32_t,       rc);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_FETCH_MEM_U8(u8Value, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
        IEM_MC_REF_GREG_U16(pu16AX, X86_GREG_xAX);
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_AIMPL_3(rc, pfnU8, pu16AX, u8Value, pEFlags);
        IEM_MC_IF_LOCAL_IS_Z(rc) {
            IEM_MC_ADVANCE_RIP_AND_FINISH();
        } IEM_MC_ELSE() {
            IEM_MC_RAISE_DIVIDE_ERROR();
        } IEM_MC_ENDIF();

        IEM_MC_END();
    }
}


/** Opcode 0xf7 /4, /5, /6 and /7. */
FNIEMOP_DEF_2(iemOpCommonGrp3MulDivEv, uint8_t, bRm, PCIEMOPMULDIVSIZES, pImpl)
{
    IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF);

    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        /* register access */
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
            {
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_BEGIN(4, 1);
                IEM_MC_ARG(uint16_t *,      pu16AX,     0);
                IEM_MC_ARG(uint16_t *,      pu16DX,     1);
                IEM_MC_ARG(uint16_t,        u16Value,   2);
                IEM_MC_ARG(uint32_t *,      pEFlags,    3);
                IEM_MC_LOCAL(int32_t,       rc);

                IEM_MC_FETCH_GREG_U16(u16Value, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_GREG_U16(pu16AX, X86_GREG_xAX);
                IEM_MC_REF_GREG_U16(pu16DX, X86_GREG_xDX);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_AIMPL_4(rc, pImpl->pfnU16, pu16AX, pu16DX, u16Value, pEFlags);
                IEM_MC_IF_LOCAL_IS_Z(rc) {
                    IEM_MC_ADVANCE_RIP_AND_FINISH();
                } IEM_MC_ELSE() {
                    IEM_MC_RAISE_DIVIDE_ERROR();
                } IEM_MC_ENDIF();

                IEM_MC_END();
                break;
            }

            case IEMMODE_32BIT:
            {
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_BEGIN(4, 1);
                IEM_MC_ARG(uint32_t *,      pu32AX,     0);
                IEM_MC_ARG(uint32_t *,      pu32DX,     1);
                IEM_MC_ARG(uint32_t,        u32Value,   2);
                IEM_MC_ARG(uint32_t *,      pEFlags,    3);
                IEM_MC_LOCAL(int32_t,       rc);

                IEM_MC_FETCH_GREG_U32(u32Value, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_GREG_U32(pu32AX, X86_GREG_xAX);
                IEM_MC_REF_GREG_U32(pu32DX, X86_GREG_xDX);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_AIMPL_4(rc, pImpl->pfnU32, pu32AX, pu32DX, u32Value, pEFlags);
                IEM_MC_IF_LOCAL_IS_Z(rc) {
                    IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(pu32AX);
                    IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(pu32DX);
                    IEM_MC_ADVANCE_RIP_AND_FINISH();
                } IEM_MC_ELSE() {
                    IEM_MC_RAISE_DIVIDE_ERROR();
                } IEM_MC_ENDIF();

                IEM_MC_END();
                break;
            }

            case IEMMODE_64BIT:
            {
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_BEGIN(4, 1);
                IEM_MC_ARG(uint64_t *,      pu64AX,     0);
                IEM_MC_ARG(uint64_t *,      pu64DX,     1);
                IEM_MC_ARG(uint64_t,        u64Value,   2);
                IEM_MC_ARG(uint32_t *,      pEFlags,    3);
                IEM_MC_LOCAL(int32_t,       rc);

                IEM_MC_FETCH_GREG_U64(u64Value, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_REF_GREG_U64(pu64AX, X86_GREG_xAX);
                IEM_MC_REF_GREG_U64(pu64DX, X86_GREG_xDX);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_AIMPL_4(rc, pImpl->pfnU64, pu64AX, pu64DX, u64Value, pEFlags);
                IEM_MC_IF_LOCAL_IS_Z(rc) {
                    IEM_MC_ADVANCE_RIP_AND_FINISH();
                } IEM_MC_ELSE() {
                    IEM_MC_RAISE_DIVIDE_ERROR();
                } IEM_MC_ENDIF();

                IEM_MC_END();
                break;
            }

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /* memory access. */
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
            {
                IEM_MC_BEGIN(4, 2);
                IEM_MC_ARG(uint16_t *,      pu16AX,     0);
                IEM_MC_ARG(uint16_t *,      pu16DX,     1);
                IEM_MC_ARG(uint16_t,        u16Value,   2);
                IEM_MC_ARG(uint32_t *,      pEFlags,    3);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_LOCAL(int32_t,       rc);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U16(u16Value, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
                IEM_MC_REF_GREG_U16(pu16AX, X86_GREG_xAX);
                IEM_MC_REF_GREG_U16(pu16DX, X86_GREG_xDX);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_AIMPL_4(rc, pImpl->pfnU16, pu16AX, pu16DX, u16Value, pEFlags);
                IEM_MC_IF_LOCAL_IS_Z(rc) {
                    IEM_MC_ADVANCE_RIP_AND_FINISH();
                } IEM_MC_ELSE() {
                    IEM_MC_RAISE_DIVIDE_ERROR();
                } IEM_MC_ENDIF();

                IEM_MC_END();
                break;
            }

            case IEMMODE_32BIT:
            {
                IEM_MC_BEGIN(4, 2);
                IEM_MC_ARG(uint32_t *,      pu32AX,     0);
                IEM_MC_ARG(uint32_t *,      pu32DX,     1);
                IEM_MC_ARG(uint32_t,        u32Value,   2);
                IEM_MC_ARG(uint32_t *,      pEFlags,    3);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_LOCAL(int32_t,       rc);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U32(u32Value, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
                IEM_MC_REF_GREG_U32(pu32AX, X86_GREG_xAX);
                IEM_MC_REF_GREG_U32(pu32DX, X86_GREG_xDX);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_AIMPL_4(rc, pImpl->pfnU32, pu32AX, pu32DX, u32Value, pEFlags);
                IEM_MC_IF_LOCAL_IS_Z(rc) {
                    IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(pu32AX);
                    IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(pu32DX);
                    IEM_MC_ADVANCE_RIP_AND_FINISH();
                } IEM_MC_ELSE() {
                    IEM_MC_RAISE_DIVIDE_ERROR();
                } IEM_MC_ENDIF();

                IEM_MC_END();
                break;
            }

            case IEMMODE_64BIT:
            {
                IEM_MC_BEGIN(4, 2);
                IEM_MC_ARG(uint64_t *,      pu64AX,     0);
                IEM_MC_ARG(uint64_t *,      pu64DX,     1);
                IEM_MC_ARG(uint64_t,        u64Value,   2);
                IEM_MC_ARG(uint32_t *,      pEFlags,    3);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_LOCAL(int32_t,       rc);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U64(u64Value, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
                IEM_MC_REF_GREG_U64(pu64AX, X86_GREG_xAX);
                IEM_MC_REF_GREG_U64(pu64DX, X86_GREG_xDX);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_AIMPL_4(rc, pImpl->pfnU64, pu64AX, pu64DX, u64Value, pEFlags);
                IEM_MC_IF_LOCAL_IS_Z(rc) {
                    IEM_MC_ADVANCE_RIP_AND_FINISH();
                } IEM_MC_ELSE() {
                    IEM_MC_RAISE_DIVIDE_ERROR();
                } IEM_MC_ENDIF();

                IEM_MC_END();
                break;
            }

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}

/**
 * @opcode      0xf6
 */
FNIEMOP_DEF(iemOp_Grp3_Eb)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    switch (IEM_GET_MODRM_REG_8(bRm))
    {
        case 0:
            return FNIEMOP_CALL_1(iemOp_grp3_test_Eb, bRm);
        case 1:
/** @todo testcase: Present on <=386, most 486 (not early), Pentiums, and current CPUs too. CPUUNDOC.EXE */
            return IEMOP_RAISE_INVALID_OPCODE();
        case 2:
            IEMOP_MNEMONIC(not_Eb, "not Eb");
            return FNIEMOP_CALL_2(iemOpCommonUnaryEb, bRm, &g_iemAImpl_not);
        case 3:
            IEMOP_MNEMONIC(neg_Eb, "neg Eb");
            return FNIEMOP_CALL_2(iemOpCommonUnaryEb, bRm, &g_iemAImpl_neg);
        case 4:
            IEMOP_MNEMONIC(mul_Eb, "mul Eb");
            IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF);
            return FNIEMOP_CALL_2(iemOpCommonGrp3MulDivEb, bRm, IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_mul_u8_eflags));
        case 5:
            IEMOP_MNEMONIC(imul_Eb, "imul Eb");
            IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF);
            return FNIEMOP_CALL_2(iemOpCommonGrp3MulDivEb, bRm, IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_imul_u8_eflags));
        case 6:
            IEMOP_MNEMONIC(div_Eb, "div Eb");
            IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_OF | X86_EFL_CF);
            return FNIEMOP_CALL_2(iemOpCommonGrp3MulDivEb, bRm, IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_div_u8_eflags));
        case 7:
            IEMOP_MNEMONIC(idiv_Eb, "idiv Eb");
            IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_OF | X86_EFL_CF);
            return FNIEMOP_CALL_2(iemOpCommonGrp3MulDivEb, bRm, IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_idiv_u8_eflags));
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0xf7
 */
FNIEMOP_DEF(iemOp_Grp3_Ev)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    switch (IEM_GET_MODRM_REG_8(bRm))
    {
        case 0:
            return FNIEMOP_CALL_1(iemOp_grp3_test_Ev, bRm);
        case 1:
/** @todo testcase: Present on <=386, most 486 (not early), Pentiums, and current CPUs too. CPUUNDOC.EXE */
            return IEMOP_RAISE_INVALID_OPCODE();
        case 2:
            IEMOP_MNEMONIC(not_Ev, "not Ev");
            return FNIEMOP_CALL_2(iemOpCommonUnaryEv, bRm, &g_iemAImpl_not);
        case 3:
            IEMOP_MNEMONIC(neg_Ev, "neg Ev");
            return FNIEMOP_CALL_2(iemOpCommonUnaryEv, bRm, &g_iemAImpl_neg);
        case 4:
            IEMOP_MNEMONIC(mul_Ev, "mul Ev");
            IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF);
            return FNIEMOP_CALL_2(iemOpCommonGrp3MulDivEv, bRm, IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_mul_eflags));
        case 5:
            IEMOP_MNEMONIC(imul_Ev, "imul Ev");
            IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF);
            return FNIEMOP_CALL_2(iemOpCommonGrp3MulDivEv, bRm, IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_imul_eflags));
        case 6:
            IEMOP_MNEMONIC(div_Ev, "div Ev");
            IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_OF | X86_EFL_CF);
            return FNIEMOP_CALL_2(iemOpCommonGrp3MulDivEv, bRm, IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_div_eflags));
        case 7:
            IEMOP_MNEMONIC(idiv_Ev, "idiv Ev");
            IEMOP_VERIFICATION_UNDEFINED_EFLAGS(X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_OF | X86_EFL_CF);
            return FNIEMOP_CALL_2(iemOpCommonGrp3MulDivEv, bRm, IEMTARGETCPU_EFL_BEHAVIOR_SELECT(g_iemAImpl_idiv_eflags));
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0xf8
 */
FNIEMOP_DEF(iemOp_clc)
{
    IEMOP_MNEMONIC(clc, "clc");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_BEGIN(0, 0);
    IEM_MC_CLEAR_EFL_BIT(X86_EFL_CF);
    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/**
 * @opcode      0xf9
 */
FNIEMOP_DEF(iemOp_stc)
{
    IEMOP_MNEMONIC(stc, "stc");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_BEGIN(0, 0);
    IEM_MC_SET_EFL_BIT(X86_EFL_CF);
    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/**
 * @opcode      0xfa
 */
FNIEMOP_DEF(iemOp_cli)
{
    IEMOP_MNEMONIC(cli, "cli");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_cli);
}


FNIEMOP_DEF(iemOp_sti)
{
    IEMOP_MNEMONIC(sti, "sti");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_sti);
}


/**
 * @opcode      0xfc
 */
FNIEMOP_DEF(iemOp_cld)
{
    IEMOP_MNEMONIC(cld, "cld");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_BEGIN(0, 0);
    IEM_MC_CLEAR_EFL_BIT(X86_EFL_DF);
    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/**
 * @opcode      0xfd
 */
FNIEMOP_DEF(iemOp_std)
{
    IEMOP_MNEMONIC(std, "std");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    IEM_MC_BEGIN(0, 0);
    IEM_MC_SET_EFL_BIT(X86_EFL_DF);
    IEM_MC_ADVANCE_RIP_AND_FINISH();
    IEM_MC_END();
}


/**
 * @opcode      0xfe
 */
FNIEMOP_DEF(iemOp_Grp4)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    switch (IEM_GET_MODRM_REG_8(bRm))
    {
        case 0:
            IEMOP_MNEMONIC(inc_Eb, "inc Eb");
            return FNIEMOP_CALL_2(iemOpCommonUnaryEb, bRm, &g_iemAImpl_inc);
        case 1:
            IEMOP_MNEMONIC(dec_Eb, "dec Eb");
            return FNIEMOP_CALL_2(iemOpCommonUnaryEb, bRm, &g_iemAImpl_dec);
        default:
            IEMOP_MNEMONIC(grp4_ud, "grp4-ud");
            return IEMOP_RAISE_INVALID_OPCODE();
    }
}


/**
 * Opcode 0xff /2.
 * @param   bRm             The RM byte.
 */
FNIEMOP_DEF_1(iemOp_Grp5_calln_Ev, uint8_t, bRm)
{
    IEMOP_MNEMONIC(calln_Ev, "calln Ev");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();

    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        /* The new RIP is taken from a register. */
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(1, 0);
                IEM_MC_ARG(uint16_t, u16Target, 0);
                IEM_MC_FETCH_GREG_U16(u16Target, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_CALL_CIMPL_1(iemCImpl_call_16, u16Target);
                IEM_MC_END()
                return VINF_SUCCESS;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(1, 0);
                IEM_MC_ARG(uint32_t, u32Target, 0);
                IEM_MC_FETCH_GREG_U32(u32Target, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_CALL_CIMPL_1(iemCImpl_call_32, u32Target);
                IEM_MC_END()
                return VINF_SUCCESS;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(1, 0);
                IEM_MC_ARG(uint64_t, u64Target, 0);
                IEM_MC_FETCH_GREG_U64(u64Target, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_CALL_CIMPL_1(iemCImpl_call_64, u64Target);
                IEM_MC_END()
                return VINF_SUCCESS;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /* The new RIP is taken from a register. */
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(1, 1);
                IEM_MC_ARG(uint16_t,  u16Target, 0);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U16(u16Target, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
                IEM_MC_CALL_CIMPL_1(iemCImpl_call_16, u16Target);
                IEM_MC_END()
                return VINF_SUCCESS;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(1, 1);
                IEM_MC_ARG(uint32_t,  u32Target, 0);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U32(u32Target, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
                IEM_MC_CALL_CIMPL_1(iemCImpl_call_32, u32Target);
                IEM_MC_END()
                return VINF_SUCCESS;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(1, 1);
                IEM_MC_ARG(uint64_t,  u64Target, 0);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U64(u64Target, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
                IEM_MC_CALL_CIMPL_1(iemCImpl_call_64, u64Target);
                IEM_MC_END()
                return VINF_SUCCESS;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}

typedef IEM_CIMPL_DECL_TYPE_3(FNIEMCIMPLFARBRANCH, uint16_t, uSel, uint64_t, offSeg, IEMMODE, enmOpSize);

FNIEMOP_DEF_2(iemOpHlp_Grp5_far_Ep, uint8_t, bRm, FNIEMCIMPLFARBRANCH *, pfnCImpl)
{
    /* Registers? How?? */
    if (RT_LIKELY(IEM_IS_MODRM_MEM_MODE(bRm)))
    { /* likely */ }
    else
        return IEMOP_RAISE_INVALID_OPCODE(); /* callf eax is not legal */

    /* 64-bit mode: Default is 32-bit, but only intel respects a REX.W prefix. */
    /** @todo what does VIA do? */
    if (pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT || pVCpu->iem.s.enmEffOpSize != IEMMODE_64BIT || IEM_IS_GUEST_CPU_INTEL(pVCpu))
    { /* likely */ }
    else
        pVCpu->iem.s.enmEffOpSize = IEMMODE_32BIT;

    /* Far pointer loaded from memory. */
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(3, 1);
            IEM_MC_ARG(uint16_t,        u16Sel,                         0);
            IEM_MC_ARG(uint16_t,        offSeg,                         1);
            IEM_MC_ARG_CONST(IEMMODE,   enmEffOpSize, IEMMODE_16BIT,    2);
            IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
            IEM_MC_FETCH_MEM_U16(offSeg, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
            IEM_MC_FETCH_MEM_U16_DISP(u16Sel, pVCpu->iem.s.iEffSeg, GCPtrEffSrc, 2);
            IEM_MC_CALL_CIMPL_3(pfnCImpl, u16Sel, offSeg, enmEffOpSize);
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(3, 1);
            IEM_MC_ARG(uint16_t,        u16Sel,                         0);
            IEM_MC_ARG(uint32_t,        offSeg,                         1);
            IEM_MC_ARG_CONST(IEMMODE,   enmEffOpSize, IEMMODE_32BIT,    2);
            IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
            IEM_MC_FETCH_MEM_U32(offSeg, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
            IEM_MC_FETCH_MEM_U16_DISP(u16Sel, pVCpu->iem.s.iEffSeg, GCPtrEffSrc, 4);
            IEM_MC_CALL_CIMPL_3(pfnCImpl, u16Sel, offSeg, enmEffOpSize);
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_64BIT:
            Assert(!IEM_IS_GUEST_CPU_AMD(pVCpu));
            IEM_MC_BEGIN(3, 1);
            IEM_MC_ARG(uint16_t,        u16Sel,                         0);
            IEM_MC_ARG(uint64_t,        offSeg,                         1);
            IEM_MC_ARG_CONST(IEMMODE,   enmEffOpSize, IEMMODE_64BIT,    2);
            IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
            IEM_MC_FETCH_MEM_U64(offSeg, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
            IEM_MC_FETCH_MEM_U16_DISP(u16Sel, pVCpu->iem.s.iEffSeg, GCPtrEffSrc, 8);
            IEM_MC_CALL_CIMPL_3(pfnCImpl, u16Sel, offSeg, enmEffOpSize);
            IEM_MC_END();
            return VINF_SUCCESS;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * Opcode 0xff /3.
 * @param   bRm             The RM byte.
 */
FNIEMOP_DEF_1(iemOp_Grp5_callf_Ep, uint8_t, bRm)
{
    IEMOP_MNEMONIC(callf_Ep, "callf Ep");
    return FNIEMOP_CALL_2(iemOpHlp_Grp5_far_Ep, bRm, iemCImpl_callf);
}


/**
 * Opcode 0xff /4.
 * @param   bRm             The RM byte.
 */
FNIEMOP_DEF_1(iemOp_Grp5_jmpn_Ev, uint8_t, bRm)
{
    IEMOP_MNEMONIC(jmpn_Ev, "jmpn Ev");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE_AND_INTEL_IGNORES_OP_SIZE_PREFIX();

    if (IEM_IS_MODRM_REG_MODE(bRm))
    {
        /* The new RIP is taken from a register. */
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint16_t, u16Target);
                IEM_MC_FETCH_GREG_U16(u16Target, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_SET_RIP_U16_AND_FINISH(u16Target);
                IEM_MC_END()
                return VINF_SUCCESS;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint32_t, u32Target);
                IEM_MC_FETCH_GREG_U32(u32Target, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_SET_RIP_U32_AND_FINISH(u32Target);
                IEM_MC_END()
                return VINF_SUCCESS;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint64_t, u64Target);
                IEM_MC_FETCH_GREG_U64(u64Target, IEM_GET_MODRM_RM(pVCpu, bRm));
                IEM_MC_SET_RIP_U64_AND_FINISH(u64Target);
                IEM_MC_END()
                return VINF_SUCCESS;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /* The new RIP is taken from a memory location. */
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint16_t, u16Target);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U16(u16Target, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
                IEM_MC_SET_RIP_U16_AND_FINISH(u16Target);
                IEM_MC_END()
                return VINF_SUCCESS;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint32_t, u32Target);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U32(u32Target, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
                IEM_MC_SET_RIP_U32_AND_FINISH(u32Target);
                IEM_MC_END()
                return VINF_SUCCESS;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint64_t, u64Target);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U64(u64Target, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
                IEM_MC_SET_RIP_U64_AND_FINISH(u64Target);
                IEM_MC_END()
                return VINF_SUCCESS;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/**
 * Opcode 0xff /5.
 * @param   bRm             The RM byte.
 */
FNIEMOP_DEF_1(iemOp_Grp5_jmpf_Ep, uint8_t, bRm)
{
    IEMOP_MNEMONIC(jmpf_Ep, "jmpf Ep");
    return FNIEMOP_CALL_2(iemOpHlp_Grp5_far_Ep, bRm, iemCImpl_FarJmp);
}


/**
 * Opcode 0xff /6.
 * @param   bRm             The RM byte.
 */
FNIEMOP_DEF_1(iemOp_Grp5_push_Ev, uint8_t, bRm)
{
    IEMOP_MNEMONIC(push_Ev, "push Ev");

    /* Registers are handled by a common worker. */
    if (IEM_IS_MODRM_REG_MODE(bRm))
        return FNIEMOP_CALL_1(iemOpCommonPushGReg, IEM_GET_MODRM_RM(pVCpu, bRm));

    /* Memory we do here. */
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0, 2);
            IEM_MC_LOCAL(uint16_t,  u16Src);
            IEM_MC_LOCAL(RTGCPTR,   GCPtrEffSrc);
            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
            IEM_MC_FETCH_MEM_U16(u16Src, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
            IEM_MC_PUSH_U16(u16Src);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0, 2);
            IEM_MC_LOCAL(uint32_t,  u32Src);
            IEM_MC_LOCAL(RTGCPTR,   GCPtrEffSrc);
            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
            IEM_MC_FETCH_MEM_U32(u32Src, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
            IEM_MC_PUSH_U32(u32Src);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0, 2);
            IEM_MC_LOCAL(uint64_t,  u64Src);
            IEM_MC_LOCAL(RTGCPTR,   GCPtrEffSrc);
            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
            IEM_MC_FETCH_MEM_U64(u64Src, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
            IEM_MC_PUSH_U64(u64Src);
            IEM_MC_ADVANCE_RIP_AND_FINISH();
            IEM_MC_END();
            break;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * @opcode      0xff
 */
FNIEMOP_DEF(iemOp_Grp5)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    switch (IEM_GET_MODRM_REG_8(bRm))
    {
        case 0:
            IEMOP_MNEMONIC(inc_Ev, "inc Ev");
            return FNIEMOP_CALL_2(iemOpCommonUnaryEv, bRm, &g_iemAImpl_inc);
        case 1:
            IEMOP_MNEMONIC(dec_Ev, "dec Ev");
            return FNIEMOP_CALL_2(iemOpCommonUnaryEv, bRm, &g_iemAImpl_dec);
        case 2:
            return FNIEMOP_CALL_1(iemOp_Grp5_calln_Ev, bRm);
        case 3:
            return FNIEMOP_CALL_1(iemOp_Grp5_callf_Ep, bRm);
        case 4:
            return FNIEMOP_CALL_1(iemOp_Grp5_jmpn_Ev, bRm);
        case 5:
            return FNIEMOP_CALL_1(iemOp_Grp5_jmpf_Ep, bRm);
        case 6:
            return FNIEMOP_CALL_1(iemOp_Grp5_push_Ev, bRm);
        case 7:
            IEMOP_MNEMONIC(grp5_ud, "grp5-ud");
            return IEMOP_RAISE_INVALID_OPCODE();
    }
    AssertFailedReturn(VERR_IEM_IPE_3);
}



const PFNIEMOP g_apfnOneByteMap[256] =
{
    /* 0x00 */  iemOp_add_Eb_Gb,        iemOp_add_Ev_Gv,        iemOp_add_Gb_Eb,        iemOp_add_Gv_Ev,
    /* 0x04 */  iemOp_add_Al_Ib,        iemOp_add_eAX_Iz,       iemOp_push_ES,          iemOp_pop_ES,
    /* 0x08 */  iemOp_or_Eb_Gb,         iemOp_or_Ev_Gv,         iemOp_or_Gb_Eb,         iemOp_or_Gv_Ev,
    /* 0x0c */  iemOp_or_Al_Ib,         iemOp_or_eAX_Iz,        iemOp_push_CS,          iemOp_2byteEscape,
    /* 0x10 */  iemOp_adc_Eb_Gb,        iemOp_adc_Ev_Gv,        iemOp_adc_Gb_Eb,        iemOp_adc_Gv_Ev,
    /* 0x14 */  iemOp_adc_Al_Ib,        iemOp_adc_eAX_Iz,       iemOp_push_SS,          iemOp_pop_SS,
    /* 0x18 */  iemOp_sbb_Eb_Gb,        iemOp_sbb_Ev_Gv,        iemOp_sbb_Gb_Eb,        iemOp_sbb_Gv_Ev,
    /* 0x1c */  iemOp_sbb_Al_Ib,        iemOp_sbb_eAX_Iz,       iemOp_push_DS,          iemOp_pop_DS,
    /* 0x20 */  iemOp_and_Eb_Gb,        iemOp_and_Ev_Gv,        iemOp_and_Gb_Eb,        iemOp_and_Gv_Ev,
    /* 0x24 */  iemOp_and_Al_Ib,        iemOp_and_eAX_Iz,       iemOp_seg_ES,           iemOp_daa,
    /* 0x28 */  iemOp_sub_Eb_Gb,        iemOp_sub_Ev_Gv,        iemOp_sub_Gb_Eb,        iemOp_sub_Gv_Ev,
    /* 0x2c */  iemOp_sub_Al_Ib,        iemOp_sub_eAX_Iz,       iemOp_seg_CS,           iemOp_das,
    /* 0x30 */  iemOp_xor_Eb_Gb,        iemOp_xor_Ev_Gv,        iemOp_xor_Gb_Eb,        iemOp_xor_Gv_Ev,
    /* 0x34 */  iemOp_xor_Al_Ib,        iemOp_xor_eAX_Iz,       iemOp_seg_SS,           iemOp_aaa,
    /* 0x38 */  iemOp_cmp_Eb_Gb,        iemOp_cmp_Ev_Gv,        iemOp_cmp_Gb_Eb,        iemOp_cmp_Gv_Ev,
    /* 0x3c */  iemOp_cmp_Al_Ib,        iemOp_cmp_eAX_Iz,       iemOp_seg_DS,           iemOp_aas,
    /* 0x40 */  iemOp_inc_eAX,          iemOp_inc_eCX,          iemOp_inc_eDX,          iemOp_inc_eBX,
    /* 0x44 */  iemOp_inc_eSP,          iemOp_inc_eBP,          iemOp_inc_eSI,          iemOp_inc_eDI,
    /* 0x48 */  iemOp_dec_eAX,          iemOp_dec_eCX,          iemOp_dec_eDX,          iemOp_dec_eBX,
    /* 0x4c */  iemOp_dec_eSP,          iemOp_dec_eBP,          iemOp_dec_eSI,          iemOp_dec_eDI,
    /* 0x50 */  iemOp_push_eAX,         iemOp_push_eCX,         iemOp_push_eDX,         iemOp_push_eBX,
    /* 0x54 */  iemOp_push_eSP,         iemOp_push_eBP,         iemOp_push_eSI,         iemOp_push_eDI,
    /* 0x58 */  iemOp_pop_eAX,          iemOp_pop_eCX,          iemOp_pop_eDX,          iemOp_pop_eBX,
    /* 0x5c */  iemOp_pop_eSP,          iemOp_pop_eBP,          iemOp_pop_eSI,          iemOp_pop_eDI,
    /* 0x60 */  iemOp_pusha,            iemOp_popa__mvex,       iemOp_bound_Gv_Ma__evex, iemOp_arpl_Ew_Gw_movsx_Gv_Ev,
    /* 0x64 */  iemOp_seg_FS,           iemOp_seg_GS,           iemOp_op_size,          iemOp_addr_size,
    /* 0x68 */  iemOp_push_Iz,          iemOp_imul_Gv_Ev_Iz,    iemOp_push_Ib,          iemOp_imul_Gv_Ev_Ib,
    /* 0x6c */  iemOp_insb_Yb_DX,       iemOp_inswd_Yv_DX,      iemOp_outsb_Yb_DX,      iemOp_outswd_Yv_DX,
    /* 0x70 */  iemOp_jo_Jb,            iemOp_jno_Jb,           iemOp_jc_Jb,            iemOp_jnc_Jb,
    /* 0x74 */  iemOp_je_Jb,            iemOp_jne_Jb,           iemOp_jbe_Jb,           iemOp_jnbe_Jb,
    /* 0x78 */  iemOp_js_Jb,            iemOp_jns_Jb,           iemOp_jp_Jb,            iemOp_jnp_Jb,
    /* 0x7c */  iemOp_jl_Jb,            iemOp_jnl_Jb,           iemOp_jle_Jb,           iemOp_jnle_Jb,
    /* 0x80 */  iemOp_Grp1_Eb_Ib_80,    iemOp_Grp1_Ev_Iz,       iemOp_Grp1_Eb_Ib_82,    iemOp_Grp1_Ev_Ib,
    /* 0x84 */  iemOp_test_Eb_Gb,       iemOp_test_Ev_Gv,       iemOp_xchg_Eb_Gb,       iemOp_xchg_Ev_Gv,
    /* 0x88 */  iemOp_mov_Eb_Gb,        iemOp_mov_Ev_Gv,        iemOp_mov_Gb_Eb,        iemOp_mov_Gv_Ev,
    /* 0x8c */  iemOp_mov_Ev_Sw,        iemOp_lea_Gv_M,         iemOp_mov_Sw_Ev,        iemOp_Grp1A__xop,
    /* 0x90 */  iemOp_nop,              iemOp_xchg_eCX_eAX,     iemOp_xchg_eDX_eAX,     iemOp_xchg_eBX_eAX,
    /* 0x94 */  iemOp_xchg_eSP_eAX,     iemOp_xchg_eBP_eAX,     iemOp_xchg_eSI_eAX,     iemOp_xchg_eDI_eAX,
    /* 0x98 */  iemOp_cbw,              iemOp_cwd,              iemOp_call_Ap,          iemOp_wait,
    /* 0x9c */  iemOp_pushf_Fv,         iemOp_popf_Fv,          iemOp_sahf,             iemOp_lahf,
    /* 0xa0 */  iemOp_mov_AL_Ob,        iemOp_mov_rAX_Ov,       iemOp_mov_Ob_AL,        iemOp_mov_Ov_rAX,
    /* 0xa4 */  iemOp_movsb_Xb_Yb,      iemOp_movswd_Xv_Yv,     iemOp_cmpsb_Xb_Yb,      iemOp_cmpswd_Xv_Yv,
    /* 0xa8 */  iemOp_test_AL_Ib,       iemOp_test_eAX_Iz,      iemOp_stosb_Yb_AL,      iemOp_stoswd_Yv_eAX,
    /* 0xac */  iemOp_lodsb_AL_Xb,      iemOp_lodswd_eAX_Xv,    iemOp_scasb_AL_Xb,      iemOp_scaswd_eAX_Xv,
    /* 0xb0 */  iemOp_mov_AL_Ib,        iemOp_CL_Ib,            iemOp_DL_Ib,            iemOp_BL_Ib,
    /* 0xb4 */  iemOp_mov_AH_Ib,        iemOp_CH_Ib,            iemOp_DH_Ib,            iemOp_BH_Ib,
    /* 0xb8 */  iemOp_eAX_Iv,           iemOp_eCX_Iv,           iemOp_eDX_Iv,           iemOp_eBX_Iv,
    /* 0xbc */  iemOp_eSP_Iv,           iemOp_eBP_Iv,           iemOp_eSI_Iv,           iemOp_eDI_Iv,
    /* 0xc0 */  iemOp_Grp2_Eb_Ib,       iemOp_Grp2_Ev_Ib,       iemOp_retn_Iw,          iemOp_retn,
    /* 0xc4 */  iemOp_les_Gv_Mp__vex3,  iemOp_lds_Gv_Mp__vex2,  iemOp_Grp11_Eb_Ib,      iemOp_Grp11_Ev_Iz,
    /* 0xc8 */  iemOp_enter_Iw_Ib,      iemOp_leave,            iemOp_retf_Iw,          iemOp_retf,
    /* 0xcc */  iemOp_int3,             iemOp_int_Ib,           iemOp_into,             iemOp_iret,
    /* 0xd0 */  iemOp_Grp2_Eb_1,        iemOp_Grp2_Ev_1,        iemOp_Grp2_Eb_CL,       iemOp_Grp2_Ev_CL,
    /* 0xd4 */  iemOp_aam_Ib,           iemOp_aad_Ib,           iemOp_salc,             iemOp_xlat,
    /* 0xd8 */  iemOp_EscF0,            iemOp_EscF1,            iemOp_EscF2,            iemOp_EscF3,
    /* 0xdc */  iemOp_EscF4,            iemOp_EscF5,            iemOp_EscF6,            iemOp_EscF7,
    /* 0xe0 */  iemOp_loopne_Jb,        iemOp_loope_Jb,         iemOp_loop_Jb,          iemOp_jecxz_Jb,
    /* 0xe4 */  iemOp_in_AL_Ib,         iemOp_in_eAX_Ib,        iemOp_out_Ib_AL,        iemOp_out_Ib_eAX,
    /* 0xe8 */  iemOp_call_Jv,          iemOp_jmp_Jv,           iemOp_jmp_Ap,           iemOp_jmp_Jb,
    /* 0xec */  iemOp_in_AL_DX,         iemOp_in_eAX_DX,        iemOp_out_DX_AL,        iemOp_out_DX_eAX,
    /* 0xf0 */  iemOp_lock,             iemOp_int1,             iemOp_repne,            iemOp_repe,
    /* 0xf4 */  iemOp_hlt,              iemOp_cmc,              iemOp_Grp3_Eb,          iemOp_Grp3_Ev,
    /* 0xf8 */  iemOp_clc,              iemOp_stc,              iemOp_cli,              iemOp_sti,
    /* 0xfc */  iemOp_cld,              iemOp_std,              iemOp_Grp4,             iemOp_Grp5,
};


/** @} */

