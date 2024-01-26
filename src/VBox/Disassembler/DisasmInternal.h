/* $Id: DisasmInternal.h $ */
/** @file
 * VBox disassembler - Internal header.
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
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef VBOX_INCLUDED_SRC_DisasmInternal_h
#define VBOX_INCLUDED_SRC_DisasmInternal_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#include <VBox/dis.h>


/** @defgroup grp_dis_int Internals.
 * @ingroup grp_dis
 * @{
 */

/** @name Index into g_apfnCalcSize and g_apfnFullDisasm.
 * @{ */
enum IDX_Parse
{
  IDX_ParseNop = 0,
  IDX_ParseModRM,
  IDX_UseModRM,
  IDX_ParseImmByte,
  IDX_ParseImmBRel,
  IDX_ParseImmUshort,
  IDX_ParseImmV,
  IDX_ParseImmVRel,
  IDX_ParseImmAddr,
  IDX_ParseFixedReg,
  IDX_ParseImmUlong,
  IDX_ParseImmQword,
  IDX_ParseTwoByteEsc,
  IDX_ParseGrp1,
  IDX_ParseShiftGrp2,
  IDX_ParseGrp3,
  IDX_ParseGrp4,
  IDX_ParseGrp5,
  IDX_Parse3DNow,
  IDX_ParseGrp6,
  IDX_ParseGrp7,
  IDX_ParseGrp8,
  IDX_ParseGrp9,
  IDX_ParseGrp10,
  IDX_ParseGrp12,
  IDX_ParseGrp13,
  IDX_ParseGrp14,
  IDX_ParseGrp15,
  IDX_ParseGrp16,
  IDX_ParseGrp17,
  IDX_ParseModFence,
  IDX_ParseYv,
  IDX_ParseYb,
  IDX_ParseXv,
  IDX_ParseXb,
  IDX_ParseEscFP,
  IDX_ParseNopPause,
  IDX_ParseImmByteSX,
  IDX_ParseImmZ,
  IDX_ParseThreeByteEsc4,
  IDX_ParseThreeByteEsc5,
  IDX_ParseImmAddrF,
  IDX_ParseInvOpModRM,
  IDX_ParseVex2b,
  IDX_ParseVex3b,
  IDX_ParseVexDest,
  IDX_ParseMax
};
AssertCompile(IDX_ParseMax < 64 /* Packed DISOPCODE assumption. */);
/** @}  */

/**
 * Opcode map descriptor.
 *
 * This is used a number of places to save storage space where there are lots of
 * invalid instructions and the beginning or end of the map.
 */
typedef struct DISOPMAPDESC
{
    /** Pointer to the opcodes described by this structure. */
    PCDISOPCODE     papOpcodes;
#if ARCH_BITS <= 32
    uint16_t
#else
    uint32_t
#endif
    /** The map index corresponding to the first papOpcodes entry. */
                    idxFirst,
    /** Number of opcodes in the map. */
                    cOpcodes;
} DISOPMAPDESC;
/** Pointer to a const opcode map descriptor. */
typedef DISOPMAPDESC const *PCDISOPMAPDESC;

/** @name Opcode maps.
 * @{ */
extern const DISOPCODE g_InvalidOpcode[1];

extern const DISOPCODE g_aOneByteMapX86[256];
extern const DISOPCODE g_aOneByteMapX64[256];
extern const DISOPCODE g_aTwoByteMapX86[256];

/** Two byte opcode map with prefix 0x66 */
extern const DISOPCODE g_aTwoByteMapX86_PF66[256];

/** Two byte opcode map with prefix 0xF2 */
extern const DISOPCODE g_aTwoByteMapX86_PFF2[256];

/** Two byte opcode map with prefix 0xF3 */
extern const DISOPCODE g_aTwoByteMapX86_PFF3[256];

/** Three byte opcode map (0xF 0x38) */
extern PCDISOPCODE const g_apThreeByteMapX86_0F38[16];

/** Three byte opcode map with prefix 0x66 (0xF 0x38) */
extern PCDISOPCODE const g_apThreeByteMapX86_660F38[16];

/** Three byte opcode map with prefix 0xF2 (0xF 0x38) */
extern PCDISOPCODE const g_apThreeByteMapX86_F20F38[16];

/** Three byte opcode map with prefix 0xF3 (0xF 0x38) */
extern PCDISOPCODE const g_apThreeByteMapX86_F30F38[16];

extern PCDISOPCODE const g_apThreeByteMapX86_0F3A[16];

/** Three byte opcode map with prefix 0x66 (0xF 0x3A) */
extern PCDISOPCODE const g_apThreeByteMapX86_660F3A[16];

/** Three byte opcode map with prefixes 0x66 0xF2 (0xF 0x38) */
extern PCDISOPCODE const g_apThreeByteMapX86_66F20F38[16];

/** VEX opcodes table defined by [VEX.m-mmmm - 1].
  * 0Fh, 0F38h, 0F3Ah correspondingly, VEX.pp = 00b */
extern PCDISOPMAPDESC const g_apVexOpcodesMapRanges_None[3];

/** VEX opcodes table defined by [VEX.m-mmmm - 1].
  * 0Fh, 0F38h, 0F3Ah correspondingly, VEX.pp = 01b (66h) */
extern PCDISOPMAPDESC const g_apVexOpcodesMapRanges_66H[3];

/** 0Fh, 0F38h, 0F3Ah correspondingly, VEX.pp = 10b (F3h) */
extern PCDISOPMAPDESC const g_apVexOpcodesMapRanges_F3H[3];

/** 0Fh, 0F38h, 0F3Ah correspondingly, VEX.pp = 11b (F2h) */
extern PCDISOPMAPDESC const g_apVexOpcodesMapRanges_F2H[3];

/** Two dimmentional map descriptor array: first index is by VEX.pp (prefix),
 * second by the VEX.mmmm (map).
 * The latter has to be bounced checked as we only have the first 4 maps. */
extern PCDISOPMAPDESC const g_aapVexOpcodesMapRanges[4][4];
/** @} */

/** @name Opcode extensions (Group tables)
 * @{ */
extern const DISOPCODE g_aMapX86_Group1[8*4];
extern const DISOPCODE g_aMapX86_Group2[8*6];
extern const DISOPCODE g_aMapX86_Group3[8*2];
extern const DISOPCODE g_aMapX86_Group4[8];
extern const DISOPCODE g_aMapX86_Group5[8];
extern const DISOPCODE g_aMapX86_Group6[8];
extern const DISOPCODE g_aMapX86_Group7_mem[8];
extern const DISOPCODE g_aMapX86_Group7_mod11_rm000[8];
extern const DISOPCODE g_aMapX86_Group7_mod11_rm001[8];
extern const DISOPCODE g_aMapX86_Group8[8];
extern const DISOPCODE g_aMapX86_Group9[8];
extern const DISOPCODE g_aMapX86_Group10[8];
extern const DISOPCODE g_aMapX86_Group11[8*2];
extern const DISOPCODE g_aMapX86_Group12[8*2];
extern const DISOPCODE g_aMapX86_Group13[8*2];
extern const DISOPCODE g_aMapX86_Group14[8*2];
extern const DISOPCODE g_aMapX86_Group15_mem[8];
extern const DISOPCODE g_aMapX86_Group15_mod11_rm000[8];
extern const DISOPCODE g_aMapX86_Group16[8];
extern const DISOPCODE g_aMapX86_Group17[8*2];
extern const DISOPCODE g_aMapX86_NopPause[2];
/** @} */

/** 3DNow! map (0x0F 0x0F prefix) */
extern const DISOPCODE g_aTwoByteMapX86_3DNow[256];

/** Floating point opcodes starting with escape byte 0xDF
 * @{ */
extern const DISOPCODE g_aMapX86_EscF0_Low[8];
extern const DISOPCODE g_aMapX86_EscF0_High[16*4];
extern const DISOPCODE g_aMapX86_EscF1_Low[8];
extern const DISOPCODE g_aMapX86_EscF1_High[16*4];
extern const DISOPCODE g_aMapX86_EscF2_Low[8];
extern const DISOPCODE g_aMapX86_EscF2_High[16*4];
extern const DISOPCODE g_aMapX86_EscF3_Low[8];
extern const DISOPCODE g_aMapX86_EscF3_High[16*4];
extern const DISOPCODE g_aMapX86_EscF4_Low[8];
extern const DISOPCODE g_aMapX86_EscF4_High[16*4];
extern const DISOPCODE g_aMapX86_EscF5_Low[8];
extern const DISOPCODE g_aMapX86_EscF5_High[16*4];
extern const DISOPCODE g_aMapX86_EscF6_Low[8];
extern const DISOPCODE g_aMapX86_EscF6_High[16*4];
extern const DISOPCODE g_aMapX86_EscF7_Low[8];
extern const DISOPCODE g_aMapX86_EscF7_High[16*4];

extern const PCDISOPCODE g_apMapX86_FP_Low[8];
extern const PCDISOPCODE g_apMapX86_FP_High[8];
/** @} */

/** @def OP
 * Wrapper which initializes an DISOPCODE.
 * We must use this so that we can exclude unused fields in order
 * to save precious bytes in the GC version.
 *
 * @internal
 */
#if DISOPCODE_FORMAT == 0
# define OP(pszOpcode, idxParse1, idxParse2, idxParse3, opcode, param1, param2, param3, optype) \
    { pszOpcode, idxParse1, idxParse2, idxParse3, 0, opcode, param1, param2, param3, 0, 0, optype }
# define OPVEX(pszOpcode, idxParse1, idxParse2, idxParse3, idxParse4, opcode, param1, param2, param3, param4, optype) \
    { pszOpcode, idxParse1, idxParse2, idxParse3, idxParse4, opcode, param1, param2, param3, param4, 0, optype | DISOPTYPE_SSE }

#elif DISOPCODE_FORMAT == 16
# define OP(pszOpcode, idxParse1, idxParse2, idxParse3, opcode, param1, param2, param3, optype) \
    { optype,                 opcode, idxParse1, idxParse2, param1, param2, idxParse3, param3, 0,      0         }
# define OPVEX(pszOpcode, idxParse1, idxParse2, idxParse3, idxParse4, opcode, param1, param2, param3, param4, optype) \
    { optype | DISOPTYPE_SSE, opcode, idxParse1, idxParse2, param1, param2, idxParse3, param3, param4, idxParse4 }

#elif DISOPCODE_FORMAT == 15
# define OP(pszOpcode, idxParse1, idxParse2, idxParse3, opcode, param1, param2, param3, optype) \
    { opcode, idxParse1, idxParse2, idxParse3, param1, param2, param3, optype,                 0,      0         }
# define OPVEX(pszOpcode, idxParse1, idxParse2, idxParse3, idxParse4, opcode, param1, param2, param3, param4, optype) \
    { opcode, idxParse1, idxParse2, idxParse3, param1, param2, param3, optype | DISOPTYPE_SSE, param4, idxParse4 }
#else
# error Unsupported DISOPCODE_FORMAT value
#endif


size_t disFormatBytes(PCDISSTATE pDis, char *pszDst, size_t cchDst, uint32_t fFlags);

/** @} */
#endif /* !VBOX_INCLUDED_SRC_DisasmInternal_h */

