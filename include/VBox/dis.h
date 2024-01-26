/** @file
 * DIS - The VirtualBox Disassembler.
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

#ifndef VBOX_INCLUDED_dis_h
#define VBOX_INCLUDED_dis_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#include <VBox/disopcode.h>
#include <iprt/assert.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_dis   VBox Disassembler
 * @{ */

/** @name Prefix byte flags (DISSTATE::fPrefix).
 * @{
 */
#define DISPREFIX_NONE                  UINT8_C(0x00)
/** non-default address size. */
#define DISPREFIX_ADDRSIZE              UINT8_C(0x01)
/** non-default operand size. */
#define DISPREFIX_OPSIZE                UINT8_C(0x02)
/** lock prefix. */
#define DISPREFIX_LOCK                  UINT8_C(0x04)
/** segment prefix. */
#define DISPREFIX_SEG                   UINT8_C(0x08)
/** rep(e) prefix (not a prefix, but we'll treat is as one). */
#define DISPREFIX_REP                   UINT8_C(0x10)
/** rep(e) prefix (not a prefix, but we'll treat is as one). */
#define DISPREFIX_REPNE                 UINT8_C(0x20)
/** REX prefix (64 bits) */
#define DISPREFIX_REX                   UINT8_C(0x40)
/** @} */

/** @name VEX.Lvvvv prefix destination register flag.
 *  @{
 */
#define VEX_LEN256                      UINT8_C(0x01)
#define VEXREG_IS256B(x)                   ((x) & VEX_LEN256)
/* Convert second byte of VEX prefix to internal format */
#define VEX_2B2INT(x)                   ((((x) >> 2) & 0x1f))
#define VEX_HAS_REX_R(x)                  (!((x) & 0x80))

#define DISPREFIX_VEX_FLAG_W            UINT8_C(0x01)
 /** @} */

/** @name 64 bits prefix byte flags (DISSTATE::fRexPrefix).
 * Requires VBox/disopcode.h.
 * @{
 */
#define DISPREFIX_REX_OP_2_FLAGS(a)     (a - OP_PARM_REX_START)
/*#define DISPREFIX_REX_FLAGS             DISPREFIX_REX_OP_2_FLAGS(OP_PARM_REX) - 0, which is no flag */
#define DISPREFIX_REX_FLAGS_B           DISPREFIX_REX_OP_2_FLAGS(OP_PARM_REX_B)
#define DISPREFIX_REX_FLAGS_X           DISPREFIX_REX_OP_2_FLAGS(OP_PARM_REX_X)
#define DISPREFIX_REX_FLAGS_XB          DISPREFIX_REX_OP_2_FLAGS(OP_PARM_REX_XB)
#define DISPREFIX_REX_FLAGS_R           DISPREFIX_REX_OP_2_FLAGS(OP_PARM_REX_R)
#define DISPREFIX_REX_FLAGS_RB          DISPREFIX_REX_OP_2_FLAGS(OP_PARM_REX_RB)
#define DISPREFIX_REX_FLAGS_RX          DISPREFIX_REX_OP_2_FLAGS(OP_PARM_REX_RX)
#define DISPREFIX_REX_FLAGS_RXB         DISPREFIX_REX_OP_2_FLAGS(OP_PARM_REX_RXB)
#define DISPREFIX_REX_FLAGS_W           DISPREFIX_REX_OP_2_FLAGS(OP_PARM_REX_W)
#define DISPREFIX_REX_FLAGS_WB          DISPREFIX_REX_OP_2_FLAGS(OP_PARM_REX_WB)
#define DISPREFIX_REX_FLAGS_WX          DISPREFIX_REX_OP_2_FLAGS(OP_PARM_REX_WX)
#define DISPREFIX_REX_FLAGS_WXB         DISPREFIX_REX_OP_2_FLAGS(OP_PARM_REX_WXB)
#define DISPREFIX_REX_FLAGS_WR          DISPREFIX_REX_OP_2_FLAGS(OP_PARM_REX_WR)
#define DISPREFIX_REX_FLAGS_WRB         DISPREFIX_REX_OP_2_FLAGS(OP_PARM_REX_WRB)
#define DISPREFIX_REX_FLAGS_WRX         DISPREFIX_REX_OP_2_FLAGS(OP_PARM_REX_WRX)
#define DISPREFIX_REX_FLAGS_WRXB        DISPREFIX_REX_OP_2_FLAGS(OP_PARM_REX_WRXB)
/** @} */
AssertCompile(RT_IS_POWER_OF_TWO(DISPREFIX_REX_FLAGS_B));
AssertCompile(RT_IS_POWER_OF_TWO(DISPREFIX_REX_FLAGS_X));
AssertCompile(RT_IS_POWER_OF_TWO(DISPREFIX_REX_FLAGS_W));
AssertCompile(RT_IS_POWER_OF_TWO(DISPREFIX_REX_FLAGS_R));

/** @name Operand type (DISOPCODE::fOpType).
 * @{
 */
#define DISOPTYPE_INVALID                  RT_BIT_32(0)
#define DISOPTYPE_HARMLESS                 RT_BIT_32(1)
#define DISOPTYPE_CONTROLFLOW              RT_BIT_32(2)
#define DISOPTYPE_POTENTIALLY_DANGEROUS    RT_BIT_32(3)
#define DISOPTYPE_DANGEROUS                RT_BIT_32(4)
#define DISOPTYPE_PORTIO                   RT_BIT_32(5)
#define DISOPTYPE_PRIVILEGED               RT_BIT_32(6)
#define DISOPTYPE_PRIVILEGED_NOTRAP        RT_BIT_32(7)
#define DISOPTYPE_UNCOND_CONTROLFLOW       RT_BIT_32(8)
#define DISOPTYPE_RELATIVE_CONTROLFLOW     RT_BIT_32(9)
#define DISOPTYPE_COND_CONTROLFLOW         RT_BIT_32(10)
#define DISOPTYPE_INTERRUPT                RT_BIT_32(11)
#define DISOPTYPE_ILLEGAL                  RT_BIT_32(12)
#define DISOPTYPE_RRM_DANGEROUS            RT_BIT_32(14)  /**< Some additional dangerous ones when recompiling raw r0. */
#define DISOPTYPE_RRM_DANGEROUS_16         RT_BIT_32(15)  /**< Some additional dangerous ones when recompiling 16-bit raw r0. */
#define DISOPTYPE_RRM_MASK                 (DISOPTYPE_RRM_DANGEROUS | DISOPTYPE_RRM_DANGEROUS_16)
#define DISOPTYPE_INHIBIT_IRQS             RT_BIT_32(16)  /**< Will or can inhibit irqs (sti, pop ss, mov ss) */
#define DISOPTYPE_PORTIO_READ              RT_BIT_32(17)
#define DISOPTYPE_PORTIO_WRITE             RT_BIT_32(18)
#define DISOPTYPE_INVALID_64               RT_BIT_32(19)  /**< Invalid in 64 bits mode */
#define DISOPTYPE_ONLY_64                  RT_BIT_32(20)  /**< Only valid in 64 bits mode */
#define DISOPTYPE_DEFAULT_64_OP_SIZE       RT_BIT_32(21)  /**< Default 64 bits operand size */
#define DISOPTYPE_FORCED_64_OP_SIZE        RT_BIT_32(22)  /**< Forced 64 bits operand size; regardless of prefix bytes */
#define DISOPTYPE_REXB_EXTENDS_OPREG       RT_BIT_32(23)  /**< REX.B extends the register field in the opcode byte */
#define DISOPTYPE_MOD_FIXED_11             RT_BIT_32(24)  /**< modrm.mod is always 11b */
#define DISOPTYPE_FORCED_32_OP_SIZE_X86    RT_BIT_32(25)  /**< Forced 32 bits operand size; regardless of prefix bytes (only in 16 & 32 bits mode!) */
#define DISOPTYPE_AVX                      RT_BIT_32(28)  /**< AVX,AVX2,++ instruction. Not implemented yet! */
#define DISOPTYPE_SSE                      RT_BIT_32(29)  /**< SSE,SSE2,SSE3,SSE4,++ instruction. Not implemented yet! */
#define DISOPTYPE_MMX                      RT_BIT_32(30)  /**< MMX,MMXExt,3DNow,++ instruction. Not implemented yet! */
#define DISOPTYPE_FPU                      RT_BIT_32(31)  /**< FPU instruction. Not implemented yet! */
#define DISOPTYPE_ALL                      UINT32_C(0xffffffff)
/** @}  */

/** @name Parameter usage flags.
 * @{
 */
#define DISUSE_BASE                        RT_BIT_64(0)
#define DISUSE_INDEX                       RT_BIT_64(1)
#define DISUSE_SCALE                       RT_BIT_64(2)
#define DISUSE_REG_GEN8                    RT_BIT_64(3)
#define DISUSE_REG_GEN16                   RT_BIT_64(4)
#define DISUSE_REG_GEN32                   RT_BIT_64(5)
#define DISUSE_REG_GEN64                   RT_BIT_64(6)
#define DISUSE_REG_FP                      RT_BIT_64(7)
#define DISUSE_REG_MMX                     RT_BIT_64(8)
#define DISUSE_REG_XMM                     RT_BIT_64(9)
#define DISUSE_REG_YMM                     RT_BIT_64(10)
#define DISUSE_REG_CR                      RT_BIT_64(11)
#define DISUSE_REG_DBG                     RT_BIT_64(12)
#define DISUSE_REG_SEG                     RT_BIT_64(13)
#define DISUSE_REG_TEST                    RT_BIT_64(14)
#define DISUSE_DISPLACEMENT8               RT_BIT_64(15)
#define DISUSE_DISPLACEMENT16              RT_BIT_64(16)
#define DISUSE_DISPLACEMENT32              RT_BIT_64(17)
#define DISUSE_DISPLACEMENT64              RT_BIT_64(18)
#define DISUSE_RIPDISPLACEMENT32           RT_BIT_64(19)
#define DISUSE_IMMEDIATE8                  RT_BIT_64(20)
#define DISUSE_IMMEDIATE8_REL              RT_BIT_64(21)
#define DISUSE_IMMEDIATE16                 RT_BIT_64(22)
#define DISUSE_IMMEDIATE16_REL             RT_BIT_64(23)
#define DISUSE_IMMEDIATE32                 RT_BIT_64(24)
#define DISUSE_IMMEDIATE32_REL             RT_BIT_64(25)
#define DISUSE_IMMEDIATE64                 RT_BIT_64(26)
#define DISUSE_IMMEDIATE64_REL             RT_BIT_64(27)
#define DISUSE_IMMEDIATE_ADDR_0_32         RT_BIT_64(28)
#define DISUSE_IMMEDIATE_ADDR_16_32        RT_BIT_64(29)
#define DISUSE_IMMEDIATE_ADDR_0_16         RT_BIT_64(30)
#define DISUSE_IMMEDIATE_ADDR_16_16        RT_BIT_64(31)
/** DS:ESI */
#define DISUSE_POINTER_DS_BASED            RT_BIT_64(32)
/** ES:EDI */
#define DISUSE_POINTER_ES_BASED            RT_BIT_64(33)
#define DISUSE_IMMEDIATE16_SX8             RT_BIT_64(34)
#define DISUSE_IMMEDIATE32_SX8             RT_BIT_64(35)
#define DISUSE_IMMEDIATE64_SX8             RT_BIT_64(36)

/** Mask of immediate use flags. */
#define DISUSE_IMMEDIATE                   ( DISUSE_IMMEDIATE8 \
                                           | DISUSE_IMMEDIATE16 \
                                           | DISUSE_IMMEDIATE32 \
                                           | DISUSE_IMMEDIATE64 \
                                           | DISUSE_IMMEDIATE8_REL \
                                           | DISUSE_IMMEDIATE16_REL \
                                           | DISUSE_IMMEDIATE32_REL \
                                           | DISUSE_IMMEDIATE64_REL \
                                           | DISUSE_IMMEDIATE_ADDR_0_32 \
                                           | DISUSE_IMMEDIATE_ADDR_16_32 \
                                           | DISUSE_IMMEDIATE_ADDR_0_16 \
                                           | DISUSE_IMMEDIATE_ADDR_16_16 \
                                           | DISUSE_IMMEDIATE16_SX8 \
                                           | DISUSE_IMMEDIATE32_SX8 \
                                           | DISUSE_IMMEDIATE64_SX8)
/** Check if the use flags indicates an effective address. */
#define DISUSE_IS_EFFECTIVE_ADDR(a_fUseFlags) (!!(  (a_fUseFlags) \
                                                  & (  DISUSE_BASE  \
                                                     | DISUSE_INDEX \
                                                     | DISUSE_DISPLACEMENT32 \
                                                     | DISUSE_DISPLACEMENT64 \
                                                     | DISUSE_DISPLACEMENT16 \
                                                     | DISUSE_DISPLACEMENT8 \
                                                     | DISUSE_RIPDISPLACEMENT32) ))
/** @} */

/** @name 64-bit general register indexes.
 * This matches the AMD64 register encoding.  It is found used in
 * DISOPPARAM::Base.idxGenReg and DISOPPARAM::Index.idxGenReg.
 * @note  Safe to assume same values as the 16-bit and 32-bit general registers.
 * @{
 */
#define DISGREG_RAX                     UINT8_C(0)
#define DISGREG_RCX                     UINT8_C(1)
#define DISGREG_RDX                     UINT8_C(2)
#define DISGREG_RBX                     UINT8_C(3)
#define DISGREG_RSP                     UINT8_C(4)
#define DISGREG_RBP                     UINT8_C(5)
#define DISGREG_RSI                     UINT8_C(6)
#define DISGREG_RDI                     UINT8_C(7)
#define DISGREG_R8                      UINT8_C(8)
#define DISGREG_R9                      UINT8_C(9)
#define DISGREG_R10                     UINT8_C(10)
#define DISGREG_R11                     UINT8_C(11)
#define DISGREG_R12                     UINT8_C(12)
#define DISGREG_R13                     UINT8_C(13)
#define DISGREG_R14                     UINT8_C(14)
#define DISGREG_R15                     UINT8_C(15)
/** @} */

/** @name 32-bit general register indexes.
 * This matches the AMD64 register encoding.  It is found used in
 * DISOPPARAM::Base.idxGenReg and DISOPPARAM::Index.idxGenReg.
 * @note  Safe to assume same values as the 16-bit and 64-bit general registers.
 * @{
 */
#define DISGREG_EAX                     UINT8_C(0)
#define DISGREG_ECX                     UINT8_C(1)
#define DISGREG_EDX                     UINT8_C(2)
#define DISGREG_EBX                     UINT8_C(3)
#define DISGREG_ESP                     UINT8_C(4)
#define DISGREG_EBP                     UINT8_C(5)
#define DISGREG_ESI                     UINT8_C(6)
#define DISGREG_EDI                     UINT8_C(7)
#define DISGREG_R8D                     UINT8_C(8)
#define DISGREG_R9D                     UINT8_C(9)
#define DISGREG_R10D                    UINT8_C(10)
#define DISGREG_R11D                    UINT8_C(11)
#define DISGREG_R12D                    UINT8_C(12)
#define DISGREG_R13D                    UINT8_C(13)
#define DISGREG_R14D                    UINT8_C(14)
#define DISGREG_R15D                    UINT8_C(15)
/** @} */

/** @name 16-bit general register indexes.
 * This matches the AMD64 register encoding.  It is found used in
 * DISOPPARAM::Base.idxGenReg and DISOPPARAM::Index.idxGenReg.
 * @note  Safe to assume same values as the 32-bit and 64-bit general registers.
 * @{
 */
#define DISGREG_AX                      UINT8_C(0)
#define DISGREG_CX                      UINT8_C(1)
#define DISGREG_DX                      UINT8_C(2)
#define DISGREG_BX                      UINT8_C(3)
#define DISGREG_SP                      UINT8_C(4)
#define DISGREG_BP                      UINT8_C(5)
#define DISGREG_SI                      UINT8_C(6)
#define DISGREG_DI                      UINT8_C(7)
#define DISGREG_R8W                     UINT8_C(8)
#define DISGREG_R9W                     UINT8_C(9)
#define DISGREG_R10W                    UINT8_C(10)
#define DISGREG_R11W                    UINT8_C(11)
#define DISGREG_R12W                    UINT8_C(12)
#define DISGREG_R13W                    UINT8_C(13)
#define DISGREG_R14W                    UINT8_C(14)
#define DISGREG_R15W                    UINT8_C(15)
/** @} */

/** @name 8-bit general register indexes.
 * This mostly (?) matches the AMD64 register encoding.  It is found used in
 * DISOPPARAM::Base.idxGenReg and DISOPPARAM::Index.idxGenReg.
 * @{
 */
#define DISGREG_AL                      UINT8_C(0)
#define DISGREG_CL                      UINT8_C(1)
#define DISGREG_DL                      UINT8_C(2)
#define DISGREG_BL                      UINT8_C(3)
#define DISGREG_AH                      UINT8_C(4)
#define DISGREG_CH                      UINT8_C(5)
#define DISGREG_DH                      UINT8_C(6)
#define DISGREG_BH                      UINT8_C(7)
#define DISGREG_R8B                     UINT8_C(8)
#define DISGREG_R9B                     UINT8_C(9)
#define DISGREG_R10B                    UINT8_C(10)
#define DISGREG_R11B                    UINT8_C(11)
#define DISGREG_R12B                    UINT8_C(12)
#define DISGREG_R13B                    UINT8_C(13)
#define DISGREG_R14B                    UINT8_C(14)
#define DISGREG_R15B                    UINT8_C(15)
#define DISGREG_SPL                     UINT8_C(16)
#define DISGREG_BPL                     UINT8_C(17)
#define DISGREG_SIL                     UINT8_C(18)
#define DISGREG_DIL                     UINT8_C(19)
/** @} */

/** @name Segment registerindexes.
 * This matches the AMD64 register encoding.  It is found used in
 * DISOPPARAM::Base.idxSegReg.
 * @{
 */
typedef enum
{
    DISSELREG_ES = 0,
    DISSELREG_CS = 1,
    DISSELREG_SS = 2,
    DISSELREG_DS = 3,
    DISSELREG_FS = 4,
    DISSELREG_GS = 5,
    /** End of the valid register index values. */
    DISSELREG_END,
    /** The usual 32-bit paranoia. */
    DIS_SEGREG_32BIT_HACK = 0x7fffffff
} DISSELREG;
/** @} */

/** @name FPU register indexes.
 * This matches the AMD64 register encoding.  It is found used in
 * DISOPPARAM::Base.idxFpuReg.
 * @{
 */
#define DISFPREG_ST0                    UINT8_C(0)
#define DISFPREG_ST1                    UINT8_C(1)
#define DISFPREG_ST2                    UINT8_C(2)
#define DISFPREG_ST3                    UINT8_C(3)
#define DISFPREG_ST4                    UINT8_C(4)
#define DISFPREG_ST5                    UINT8_C(5)
#define DISFPREG_ST6                    UINT8_C(6)
#define DISFPREG_ST7                    UINT8_C(7)
/** @}  */

/** @name Control register indexes.
 * This matches the AMD64 register encoding.  It is found used in
 * DISOPPARAM::Base.idxCtrlReg.
 * @{
 */
#define DISCREG_CR0                     UINT8_C(0)
#define DISCREG_CR1                     UINT8_C(1)
#define DISCREG_CR2                     UINT8_C(2)
#define DISCREG_CR3                     UINT8_C(3)
#define DISCREG_CR4                     UINT8_C(4)
#define DISCREG_CR8                     UINT8_C(8)
/** @}  */

/** @name Debug register indexes.
 * This matches the AMD64 register encoding.  It is found used in
 * DISOPPARAM::Base.idxDbgReg.
 * @{
 */
#define DISDREG_DR0                     UINT8_C(0)
#define DISDREG_DR1                     UINT8_C(1)
#define DISDREG_DR2                     UINT8_C(2)
#define DISDREG_DR3                     UINT8_C(3)
#define DISDREG_DR4                     UINT8_C(4)
#define DISDREG_DR5                     UINT8_C(5)
#define DISDREG_DR6                     UINT8_C(6)
#define DISDREG_DR7                     UINT8_C(7)
/** @}  */

/** @name MMX register indexes.
 * This matches the AMD64 register encoding.  It is found used in
 * DISOPPARAM::Base.idxMmxReg.
 * @{
 */
#define DISMREG_MMX0                    UINT8_C(0)
#define DISMREG_MMX1                    UINT8_C(1)
#define DISMREG_MMX2                    UINT8_C(2)
#define DISMREG_MMX3                    UINT8_C(3)
#define DISMREG_MMX4                    UINT8_C(4)
#define DISMREG_MMX5                    UINT8_C(5)
#define DISMREG_MMX6                    UINT8_C(6)
#define DISMREG_MMX7                    UINT8_C(7)
/** @}  */

/** @name SSE register indexes.
 * This matches the AMD64 register encoding.  It is found used in
 * DISOPPARAM::Base.idxXmmReg.
 * @{
 */
#define DISXREG_XMM0                    UINT8_C(0)
#define DISXREG_XMM1                    UINT8_C(1)
#define DISXREG_XMM2                    UINT8_C(2)
#define DISXREG_XMM3                    UINT8_C(3)
#define DISXREG_XMM4                    UINT8_C(4)
#define DISXREG_XMM5                    UINT8_C(5)
#define DISXREG_XMM6                    UINT8_C(6)
#define DISXREG_XMM7                    UINT8_C(7)
/** @} */


/**
 * Opcode parameter (operand) details.
 */
typedef struct DISOPPARAM
{
    /** A combination of DISUSE_XXX. */
    uint64_t        fUse;
    /** Immediate value or address, applicable if any of the flags included in
     * DISUSE_IMMEDIATE are set in fUse. */
    uint64_t        uValue;
    /** Disposition.  */
    union
    {
        /** 64-bit displacement, applicable if DISUSE_DISPLACEMENT64 is set in fUse.  */
        int64_t     i64;
        uint64_t    u64;
        /** 32-bit displacement, applicable if DISUSE_DISPLACEMENT32 or
         * DISUSE_RIPDISPLACEMENT32  is set in fUse. */
        int32_t     i32;
        uint32_t    u32;
        /** 16-bit displacement, applicable if DISUSE_DISPLACEMENT16 is set in fUse.  */
        int32_t     i16;
        uint32_t    u16;
        /** 8-bit displacement, applicable if DISUSE_DISPLACEMENT8 is set in fUse.  */
        int32_t     i8;
        uint32_t    u8;
    } uDisp;
    /** The base register from ModR/M or SIB, applicable if DISUSE_BASE is
     * set in fUse. */
    union
    {
        /** General register index (DISGREG_XXX), applicable if DISUSE_REG_GEN8,
         * DISUSE_REG_GEN16, DISUSE_REG_GEN32 or DISUSE_REG_GEN64 is set in fUse. */
        uint8_t     idxGenReg;
        /** FPU stack register index (DISFPREG_XXX), applicable if DISUSE_REG_FP is
         * set in fUse.  1:1 indexes. */
        uint8_t     idxFpuReg;
        /** MMX register index (DISMREG_XXX), applicable if DISUSE_REG_MMX is
         * set in fUse.  1:1 indexes. */
        uint8_t     idxMmxReg;
        /** SSE register index (DISXREG_XXX), applicable if DISUSE_REG_XMM is
         * set in fUse.  1:1 indexes. */
        uint8_t     idxXmmReg;
        /** SSE2 register index (DISYREG_XXX), applicable if DISUSE_REG_YMM is
         * set in fUse.  1:1 indexes. */
        uint8_t     idxYmmReg;
        /** Segment register index (DISSELREG_XXX), applicable if DISUSE_REG_SEG is
         * set in fUse. */
        uint8_t     idxSegReg;
        /** Test register, TR0-TR7, present on early IA32 CPUs, applicable if
         * DISUSE_REG_TEST is set in fUse.  No index defines for these. */
        uint8_t     idxTestReg;
        /** Control register index (DISCREG_XXX), applicable if DISUSE_REG_CR is
         * set in fUse.  1:1 indexes. */
        uint8_t     idxCtrlReg;
        /** Debug register index (DISDREG_XXX), applicable if DISUSE_REG_DBG is
         * set in fUse.  1:1 indexes. */
        uint8_t     idxDbgReg;
    } Base;
    /** The SIB index register meaning, applicable if DISUSE_INDEX is
     * set in fUse. */
    union
    {
        /** General register index (DISGREG_XXX), applicable if DISUSE_REG_GEN8,
         * DISUSE_REG_GEN16, DISUSE_REG_GEN32 or DISUSE_REG_GEN64 is set in fUse. */
        uint8_t     idxGenReg;
        /** XMM register index (DISXREG_XXX), applicable if DISUSE_REG_XMM
         *  is set in fUse. */
        uint8_t     idxXmmReg;
        /** YMM register index (DISXREG_XXX), applicable if DISUSE_REG_YMM
         *  is set in fUse. */
        uint8_t     idxYmmReg;
    } Index;
    /** 2, 4 or 8, if DISUSE_SCALE is set in fUse. */
    uint8_t         uScale;
    /** Parameter size. */
    uint8_t         cb;
    /** Copy of the corresponding DISOPCODE::fParam1 / DISOPCODE::fParam2 /
     * DISOPCODE::fParam3. */
    uint32_t        fParam;
} DISOPPARAM;
AssertCompileSize(DISOPPARAM, 32);
/** Pointer to opcode parameter. */
typedef DISOPPARAM *PDISOPPARAM;
/** Pointer to opcode parameter. */
typedef const DISOPPARAM *PCDISOPPARAM;


#if (defined(RT_ARCH_X86) || defined(RT_ARCH_AMD64)) && defined(DIS_CORE_ONLY)
# define DISOPCODE_BITFIELD(a_cBits) : a_cBits
#else
# define DISOPCODE_BITFIELD(a_cBits)
#endif

/**
 * Opcode descriptor.
 */
#if !defined(DIS_CORE_ONLY) || defined(DOXYGEN_RUNNING)
typedef struct DISOPCODE
{
# define DISOPCODE_FORMAT  0
    /** Mnemonic and operand formatting. */
    const char  *pszOpcode;
    /** Parameter \#1 parser index. */
    uint8_t     idxParse1;
    /** Parameter \#2 parser index. */
    uint8_t     idxParse2;
    /** Parameter \#3 parser index. */
    uint8_t     idxParse3;
    /** Parameter \#4 parser index.  */
    uint8_t     idxParse4;
    /** The opcode identifier. This DIS specific, @see grp_dis_opcodes and
     * VBox/disopcode.h. */
    uint16_t    uOpcode;
    /** Parameter \#1 info, @see grp_dis_opparam. */
    uint16_t    fParam1;
    /** Parameter \#2 info, @see grp_dis_opparam. */
    uint16_t    fParam2;
    /** Parameter \#3 info, @see grp_dis_opparam. */
    uint16_t    fParam3;
    /** Parameter \#4 info, @see grp_dis_opparam. */
    uint16_t    fParam4;
    /** padding unused */
    uint16_t    uPadding;
    /** Operand type flags, DISOPTYPE_XXX. */
    uint32_t    fOpType;
} DISOPCODE;
#else
# pragma pack(1)
typedef struct DISOPCODE
{
#if 1 /*!defined(RT_ARCH_X86) && !defined(RT_ARCH_AMD64) - probably not worth it for ~4K, costs 2-3% speed. */
    /* 16 bytes (trick is to make sure the bitfields doesn't cross dwords): */
# define DISOPCODE_FORMAT  16
    uint32_t    fOpType;
    uint16_t    uOpcode;
    uint8_t     idxParse1;
    uint8_t     idxParse2;
    uint32_t    fParam1   : 12; /* 1st dword: 12+12+8 = 0x20 (32) */
    uint32_t    fParam2   : 12;
    uint32_t    idxParse3 : 8;
    uint32_t    fParam3   : 12; /* 2nd dword: 12+12+8 = 0x20 (32) */
    uint32_t    fParam4   : 12;
    uint32_t    idxParse4 : 8;
#else /* 15 bytes: */
# define DISOPCODE_FORMAT  15
    uint64_t    uOpcode   : 10; /* 1st qword: 10+12+12+12+6+6+6 = 0x40 (64) */
    uint64_t    idxParse1 : 6;
    uint64_t    idxParse2 : 6;
    uint64_t    idxParse3 : 6;
    uint64_t    fParam1   : 12;
    uint64_t    fParam2   : 12;
    uint64_t    fParam3   : 12;
    uint32_t    fOpType;
    uint16_t    fParam4;
    uint8_t     idxParse4;
#endif
} DISOPCODE;
# pragma pack()
AssertCompile(sizeof(DISOPCODE) == DISOPCODE_FORMAT);
#endif
AssertCompile(DISOPCODE_FORMAT != 15); /* Needs fixing before use as disopcode.h now has more than 1024 opcode values. */
/** Pointer to const opcode. */
typedef const struct DISOPCODE *PCDISOPCODE;


/**
 * Callback for reading instruction bytes.
 *
 * @returns VBox status code, bytes in DISSTATE::abInstr and byte count in
 *          DISSTATE::cbCachedInstr.
 * @param   pDis            Pointer to the disassembler state.  The user
 *                          argument can be found in DISSTATE::pvUser if needed.
 * @param   offInstr        The offset relative to the start of the instruction.
 *
 *                          To get the source address, add this to
 *                          DISSTATE::uInstrAddr.
 *
 *                          To calculate the destination buffer address, use it
 *                          as an index into DISSTATE::abInstr.
 *
 * @param   cbMinRead       The minimum number of bytes to read.
 * @param   cbMaxRead       The maximum number of bytes that may be read.
 */
typedef DECLCALLBACKTYPE(int, FNDISREADBYTES,(PDISSTATE pDis, uint8_t offInstr, uint8_t cbMinRead, uint8_t cbMaxRead));
/** Pointer to a opcode byte reader. */
typedef FNDISREADBYTES *PFNDISREADBYTES;

/** Parser callback.
 * @remark no DECLCALLBACK() here because it's considered to be internal and
 *         there is no point in enforcing CDECL. */
typedef size_t FNDISPARSE(size_t offInstr, PCDISOPCODE pOp, PDISSTATE pDis, PDISOPPARAM pParam);
/** Pointer to a disassembler parser function. */
typedef FNDISPARSE *PFNDISPARSE;
/** Pointer to a const disassembler parser function pointer. */
typedef PFNDISPARSE const *PCPFNDISPARSE;

/**
 * The diassembler state and result.
 */
typedef struct DISSTATE
{
    /** The number of valid bytes in abInstr. */
    uint8_t         cbCachedInstr;
    /** SIB fields. */
    union
    {
        /** Bitfield view */
        struct
        {
            uint8_t     Base;
            uint8_t     Index;
            uint8_t     Scale;
        } Bits;
    } SIB;
    /** ModRM fields. */
    union
    {
        /** Bitfield view */
        struct
        {
            uint8_t     Rm;
            uint8_t     Reg;
            uint8_t     Mod;
        } Bits;
    } ModRM;
    /** The CPU mode (DISCPUMODE). */
    uint8_t         uCpuMode;
    /** The addressing mode (DISCPUMODE). */
    uint8_t         uAddrMode;
    /** The operand mode (DISCPUMODE). */
    uint8_t         uOpMode;
    /** Per instruction prefix settings. */
    uint8_t         fPrefix;
    /** REX prefix value (64 bits only). */
    uint8_t         fRexPrefix;
    /** Segment prefix value (DISSELREG). */
    uint8_t         idxSegPrefix;
    /** Last prefix byte (for SSE2 extension tables). */
    uint8_t         bLastPrefix;
    /** Last significant opcode byte of instruction. */
    uint8_t         bOpCode;
    /** The size of the prefix bytes. */
    uint8_t         cbPrefix;
    /** The instruction size. */
    uint8_t         cbInstr;
    /** VEX presence flag, destination register and size
     * @todo r=bird: There is no VEX presence flage here, just ~vvvv and L.  */
    uint8_t         bVexDestReg;
    /** VEX.W flag */
    uint8_t         bVexWFlag;
    /** Unused bytes. */
    uint8_t         abUnused[1];
    /** Internal: instruction filter */
    uint32_t        fFilter;
    /** Internal: pointer to disassembly function table */
    PCPFNDISPARSE   pfnDisasmFnTable;
#if ARCH_BITS == 32
    uint32_t        uPtrPadding1;
#endif
    /** Pointer to the current instruction. */
    PCDISOPCODE     pCurInstr;
#if ARCH_BITS == 32
    uint32_t        uPtrPadding2;
#endif
    /** The instruction bytes. */
    uint8_t         abInstr[16];
    /** SIB displacment. */
    int32_t         i32SibDisp;

    /** Return code set by a worker function like the opcode bytes readers. */
    int32_t         rc;
    /** The address of the instruction. */
    RTUINTPTR       uInstrAddr;
    /** Optional read function */
    PFNDISREADBYTES pfnReadBytes;
#if ARCH_BITS == 32
    uint32_t        uPadding3;
#endif
    /** User data supplied as an argument to the APIs. */
    void           *pvUser;
#if ARCH_BITS == 32
    uint32_t        uPadding4;
#endif
    /** Parameters.  */
    DISOPPARAM      Param1;
    DISOPPARAM      Param2;
    DISOPPARAM      Param3;
    DISOPPARAM      Param4;
} DISSTATE;
AssertCompileSize(DISSTATE, 0xd8);

/** @deprecated  Use DISSTATE and change Cpu and DisState to Dis. */
typedef DISSTATE DISCPUSTATE;



DISDECL(int) DISInstrToStr(void const *pvInstr, DISCPUMODE enmCpuMode,
                           PDISSTATE pDis, uint32_t *pcbInstr, char *pszOutput, size_t cbOutput);
DISDECL(int) DISInstrToStrWithReader(RTUINTPTR uInstrAddr, DISCPUMODE enmCpuMode, PFNDISREADBYTES pfnReadBytes, void *pvUser,
                                     PDISSTATE pDis, uint32_t *pcbInstr, char *pszOutput, size_t cbOutput);
DISDECL(int) DISInstrToStrEx(RTUINTPTR uInstrAddr, DISCPUMODE enmCpuMode,
                             PFNDISREADBYTES pfnReadBytes, void *pvUser, uint32_t uFilter,
                             PDISSTATE pDis, uint32_t *pcbInstr, char *pszOutput, size_t cbOutput);

DISDECL(int) DISInstr(void const *pvInstr, DISCPUMODE enmCpuMode, PDISSTATE pDis, uint32_t *pcbInstr);
DISDECL(int) DISInstrWithReader(RTUINTPTR uInstrAddr, DISCPUMODE enmCpuMode, PFNDISREADBYTES pfnReadBytes, void *pvUser,
                                PDISSTATE pDis, uint32_t *pcbInstr);
DISDECL(int) DISInstrEx(RTUINTPTR uInstrAddr, DISCPUMODE enmCpuMode, uint32_t uFilter,
                        PFNDISREADBYTES pfnReadBytes, void *pvUser,
                        PDISSTATE pDis, uint32_t *pcbInstr);
DISDECL(int) DISInstrWithPrefetchedBytes(RTUINTPTR uInstrAddr, DISCPUMODE enmCpuMode, uint32_t fFilter,
                                         void const *pvPrefetched, size_t cbPretched,
                                         PFNDISREADBYTES pfnReadBytes, void *pvUser,
                                         PDISSTATE pDis, uint32_t *pcbInstr);

DISDECL(uint8_t)    DISGetParamSize(PCDISSTATE pDis, PCDISOPPARAM pParam);
#if 0 /* unused */
DISDECL(DISSELREG)  DISDetectSegReg(PCDISSTATE pDis, PCDISOPPARAM pParam);
DISDECL(uint8_t)    DISQuerySegPrefixByte(PCDISSTATE pDis);
#endif

#if 0 /* Needs refactoring if we want to use this again, CPUMCTXCORE is history.  */
/** @name Flags returned by DISQueryParamVal (DISQPVPARAMVAL::flags).
 * @{
 */
#define DISQPV_FLAG_8                   UINT8_C(0x01)
#define DISQPV_FLAG_16                  UINT8_C(0x02)
#define DISQPV_FLAG_32                  UINT8_C(0x04)
#define DISQPV_FLAG_64                  UINT8_C(0x08)
#define DISQPV_FLAG_FARPTR16            UINT8_C(0x10)
#define DISQPV_FLAG_FARPTR32            UINT8_C(0x20)
/** @}  */

/** @name Types returned by DISQueryParamVal (DISQPVPARAMVAL::flags).
 * @{ */
#define DISQPV_TYPE_REGISTER            UINT8_C(1)
#define DISQPV_TYPE_ADDRESS             UINT8_C(2)
#define DISQPV_TYPE_IMMEDIATE           UINT8_C(3)
/** @}  */

typedef struct
{
    union
    {
        uint8_t     val8;
        uint16_t    val16;
        uint32_t    val32;
        uint64_t    val64;

        int8_t      i8;
        int16_t     i16;
        int32_t     i32;
        int64_t     i64;

        struct
        {
            uint16_t sel;
            uint32_t offset;
        } farptr;
    } val;

    uint8_t         type;
    uint8_t         size;
    uint8_t         flags;
} DISQPVPARAMVAL;
/** Pointer to opcode parameter value. */
typedef DISQPVPARAMVAL *PDISQPVPARAMVAL;

/** Indicates which parameter DISQueryParamVal should operate on. */
typedef enum DISQPVWHICH
{
    DISQPVWHICH_DST = 1,
    DISQPVWHICH_SRC,
    DISQPVWHAT_32_BIT_HACK = 0x7fffffff
} DISQPVWHICH;
DISDECL(int) DISQueryParamVal(PCPUMCTXCORE pCtx, PCDISSTATE pDis, PCDISOPPARAM pParam, PDISQPVPARAMVAL pParamVal, DISQPVWHICH parmtype);
DISDECL(int) DISQueryParamRegPtr(PCPUMCTXCORE pCtx, PCDISSTATE pDis, PCDISOPPARAM pParam, void **ppReg, size_t *pcbSize);

DISDECL(int) DISFetchReg8(PCCPUMCTXCORE pCtx, unsigned reg8, uint8_t *pVal);
DISDECL(int) DISFetchReg16(PCCPUMCTXCORE pCtx, unsigned reg16, uint16_t *pVal);
DISDECL(int) DISFetchReg32(PCCPUMCTXCORE pCtx, unsigned reg32, uint32_t *pVal);
DISDECL(int) DISFetchReg64(PCCPUMCTXCORE pCtx, unsigned reg64, uint64_t *pVal);
DISDECL(int) DISFetchRegSeg(PCCPUMCTXCORE pCtx, DISSELREG sel, RTSEL *pVal);
DISDECL(int) DISWriteReg8(PCPUMCTXCORE pRegFrame, unsigned reg8, uint8_t val8);
DISDECL(int) DISWriteReg16(PCPUMCTXCORE pRegFrame, unsigned reg32, uint16_t val16);
DISDECL(int) DISWriteReg32(PCPUMCTXCORE pRegFrame, unsigned reg32, uint32_t val32);
DISDECL(int) DISWriteReg64(PCPUMCTXCORE pRegFrame, unsigned reg64, uint64_t val64);
DISDECL(int) DISWriteRegSeg(PCPUMCTXCORE pCtx, DISSELREG sel, RTSEL val);
DISDECL(int) DISPtrReg8(PCPUMCTXCORE pCtx, unsigned reg8, uint8_t **ppReg);
DISDECL(int) DISPtrReg16(PCPUMCTXCORE pCtx, unsigned reg16, uint16_t **ppReg);
DISDECL(int) DISPtrReg32(PCPUMCTXCORE pCtx, unsigned reg32, uint32_t **ppReg);
DISDECL(int) DISPtrReg64(PCPUMCTXCORE pCtx, unsigned reg64, uint64_t **ppReg);
#endif /* obsolete */


/**
 * Try resolve an address into a symbol name.
 *
 * For use with DISFormatYasmEx(), DISFormatMasmEx() and DISFormatGasEx().
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success, pszBuf contains the full symbol name.
 * @retval  VINF_BUFFER_OVERFLOW if pszBuf is too small the symbol name. The
 *          content of pszBuf is truncated and zero terminated.
 * @retval  VERR_SYMBOL_NOT_FOUND if no matching symbol was found for the address.
 *
 * @param   pDis        Pointer to the disassembler CPU state.
 * @param   u32Sel      The selector value. Use DIS_FMT_SEL_IS_REG, DIS_FMT_SEL_GET_VALUE,
 *                      DIS_FMT_SEL_GET_REG to access this.
 * @param   uAddress    The segment address.
 * @param   pszBuf      Where to store the symbol name
 * @param   cchBuf      The size of the buffer.
 * @param   poff        If not a perfect match, then this is where the offset from the return
 *                      symbol to the specified address is returned.
 * @param   pvUser      The user argument.
 */
typedef DECLCALLBACKTYPE(int, FNDISGETSYMBOL,(PCDISSTATE pDis, uint32_t u32Sel, RTUINTPTR uAddress, char *pszBuf, size_t cchBuf,
                                              RTINTPTR *poff, void *pvUser));
/** Pointer to a FNDISGETSYMBOL(). */
typedef FNDISGETSYMBOL *PFNDISGETSYMBOL;

/**
 * Checks if the FNDISGETSYMBOL argument u32Sel is a register or not.
 */
#define DIS_FMT_SEL_IS_REG(u32Sel)          ( !!((u32Sel) & RT_BIT(31)) )

/**
 * Extracts the selector value from the FNDISGETSYMBOL argument u32Sel.
 * @returns Selector value.
 */
#define DIS_FMT_SEL_GET_VALUE(u32Sel)       ( (RTSEL)(u32Sel) )

/**
 * Extracts the register number from the FNDISGETSYMBOL argument u32Sel.
 * @returns USE_REG_CS, USE_REG_SS, USE_REG_DS, USE_REG_ES, USE_REG_FS or USE_REG_FS.
 */
#define DIS_FMT_SEL_GET_REG(u32Sel)         ( ((u32Sel) >> 16) & 0xf )

/** @internal */
#define DIS_FMT_SEL_FROM_REG(uReg)          ( ((uReg) << 16) | RT_BIT(31) | 0xffff )
/** @internal */
#define DIS_FMT_SEL_FROM_VALUE(Sel)         ( (Sel) & 0xffff )


/** @name Flags for use with DISFormatYasmEx(), DISFormatMasmEx() and DISFormatGasEx().
 * @{
 */
/** Put the address to the right. */
#define DIS_FMT_FLAGS_ADDR_RIGHT            RT_BIT_32(0)
/** Put the address to the left. */
#define DIS_FMT_FLAGS_ADDR_LEFT             RT_BIT_32(1)
/** Put the address in comments.
 * For some assemblers this implies placing it to the right. */
#define DIS_FMT_FLAGS_ADDR_COMMENT          RT_BIT_32(2)
/** Put the instruction bytes to the right of the disassembly. */
#define DIS_FMT_FLAGS_BYTES_RIGHT           RT_BIT_32(3)
/** Put the instruction bytes to the left of the disassembly. */
#define DIS_FMT_FLAGS_BYTES_LEFT            RT_BIT_32(4)
/** Put the instruction bytes in comments.
 * For some assemblers this implies placing the bytes to the right. */
#define DIS_FMT_FLAGS_BYTES_COMMENT         RT_BIT_32(5)
/** Put the bytes in square brackets. */
#define DIS_FMT_FLAGS_BYTES_BRACKETS        RT_BIT_32(6)
/** Put spaces between the bytes. */
#define DIS_FMT_FLAGS_BYTES_SPACED          RT_BIT_32(7)
/** Display the relative +/- offset of branch instructions that uses relative addresses,
 * and put the target address in parenthesis. */
#define DIS_FMT_FLAGS_RELATIVE_BRANCH       RT_BIT_32(8)
/** Strict assembly. The assembly should, when ever possible, make the
 * assembler reproduce the exact same binary. (Refers to the yasm
 * strict keyword.) */
#define DIS_FMT_FLAGS_STRICT                RT_BIT_32(9)
/** Checks if the given flags are a valid combination. */
#define DIS_FMT_FLAGS_IS_VALID(fFlags) \
    (   !((fFlags) & ~UINT32_C(0x000003ff)) \
     && ((fFlags) & (DIS_FMT_FLAGS_ADDR_RIGHT  | DIS_FMT_FLAGS_ADDR_LEFT))  != (DIS_FMT_FLAGS_ADDR_RIGHT  | DIS_FMT_FLAGS_ADDR_LEFT) \
     && (   !((fFlags) & DIS_FMT_FLAGS_ADDR_COMMENT) \
         || (fFlags & (DIS_FMT_FLAGS_ADDR_RIGHT | DIS_FMT_FLAGS_ADDR_LEFT)) ) \
     && ((fFlags) & (DIS_FMT_FLAGS_BYTES_RIGHT | DIS_FMT_FLAGS_BYTES_LEFT)) != (DIS_FMT_FLAGS_BYTES_RIGHT | DIS_FMT_FLAGS_BYTES_LEFT) \
     && (   !((fFlags) & (DIS_FMT_FLAGS_BYTES_COMMENT | DIS_FMT_FLAGS_BYTES_BRACKETS)) \
         || (fFlags & (DIS_FMT_FLAGS_BYTES_RIGHT | DIS_FMT_FLAGS_BYTES_LEFT)) ) \
    )
/** @} */

DISDECL(size_t) DISFormatYasm(  PCDISSTATE pDis, char *pszBuf, size_t cchBuf);
DISDECL(size_t) DISFormatYasmEx(PCDISSTATE pDis, char *pszBuf, size_t cchBuf, uint32_t fFlags, PFNDISGETSYMBOL pfnGetSymbol, void *pvUser);
DISDECL(size_t) DISFormatMasm(  PCDISSTATE pDis, char *pszBuf, size_t cchBuf);
DISDECL(size_t) DISFormatMasmEx(PCDISSTATE pDis, char *pszBuf, size_t cchBuf, uint32_t fFlags, PFNDISGETSYMBOL pfnGetSymbol, void *pvUser);
DISDECL(size_t) DISFormatGas(   PCDISSTATE pDis, char *pszBuf, size_t cchBuf);
DISDECL(size_t) DISFormatGasEx( PCDISSTATE pDis, char *pszBuf, size_t cchBuf, uint32_t fFlags, PFNDISGETSYMBOL pfnGetSymbol, void *pvUser);

/** @todo DISAnnotate(PCDISSTATE pDis, char *pszBuf, size_t cchBuf, register
 *        reader, memory reader); */

DISDECL(bool)   DISFormatYasmIsOddEncoding(PDISSTATE pDis);

/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_dis_h */

