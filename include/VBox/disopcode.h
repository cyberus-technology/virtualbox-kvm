/** @file
 * Disassembler - Opcodes
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

#ifndef VBOX_INCLUDED_disopcode_h
#define VBOX_INCLUDED_disopcode_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/assert.h>

#define MODRM_MOD(a)    (a>>6)
#define MODRM_REG(a)    ((a>>3)&0x7)
#define MODRM_RM(a)     (a&0x7)
#define MAKE_MODRM(mod, reg, rm) (((mod&3) << 6) | ((reg&7) << 3) | (rm&7))

#define SIB_SCALE(a)    (a>>6)
#define SIB_INDEX(a)    ((a>>3)&0x7)
#define SIB_BASE(a)     (a&0x7)


/** @defgroup grp_dis_opcodes Opcodes (DISOPCODE::uOpCode)
 * @ingroup grp_dis
 * @{
 */
enum OPCODES
{
/** @name  Full Intel X86 opcode list
 * @{ */
    OP_INVALID = 0,
    OP_OPSIZE,
    OP_ADDRSIZE,
    OP_SEG,
    OP_REPNE,
    OP_REPE,
    OP_REX,
    OP_LOCK,
#ifndef IN_SLICKEDIT
    OP_LAST_PREFIX = OP_LOCK, /**< Last prefix for disassembler. */
#else
    OP_LAST_PREFIX = 7, /**< Last prefix for disassembler. */
#endif
    OP_AND,
    OP_OR,
    OP_DAA,
    OP_SUB,
    OP_DAS,
    OP_XOR,
    OP_AAA,
    OP_CMP,
    OP_IMM_GRP1,
    OP_AAS,
    OP_INC,
    OP_DEC,
    OP_PUSHA,
    OP_POPA,
    OP_BOUND,
    OP_ARPL,
    OP_PUSH,
    OP_POP,
    OP_IMUL,
    OP_INSB,
    OP_INSWD,
    OP_OUTSB,
    OP_OUTSWD,
    OP_JO,
    OP_JNO,
    OP_JC,
    OP_JNC,
    OP_JE,
    OP_JNE,
    OP_JBE,
    OP_JNBE,
    OP_JS,
    OP_JNS,
    OP_JP,
    OP_JNP,
    OP_JL,
    OP_JNL,
    OP_JLE,
    OP_JNLE,
    OP_ADD,
    OP_TEST,
    OP_XCHG,
    OP_MOV,
    OP_LEA,
    OP_NOP,
    OP_CBW,
    OP_CWD,
    OP_CALL,
    OP_WAIT,
    OP_PUSHF,
    OP_POPF,
    OP_SAHF,
    OP_LAHF,
    OP_MOVSB,
    OP_MOVSWD,
    OP_CMPSB,
    OP_CMPWD,
    OP_STOSB,
    OP_STOSWD,
    OP_LODSB,
    OP_LODSWD,
    OP_SCASB,
    OP_SCASWD,
    OP_SHIFT_GRP2,
    OP_RETN,
    OP_LES,
    OP_LDS,
    OP_ENTER,
    OP_LEAVE,
    OP_RETF,
    OP_INT1,
    OP_INT3,
    OP_INT,
    OP_INTO,
    OP_IRET,
    OP_AAM,
    OP_AAD,
    OP_SALC,
    OP_XLAT,
    OP_ESCF0,
    OP_ESCF1,
    OP_ESCF2,
    OP_ESCF3,
    OP_ESCF4,
    OP_ESCF5,
    OP_ESCF6,
    OP_ESCF7,
    OP_LOOPNE,
    OP_LOOPE,
    OP_LOOP,
    OP_JECXZ,
    OP_IN,
    OP_OUT,
    OP_JMP,
    OP_2B_ESC,
    OP_ADC,
    OP_SBB,
    OP_HLT,
    OP_CMC,
    OP_UNARY_GRP3,
    OP_CLC,
    OP_STC,
    OP_CLI,
    OP_STI,
    OP_CLD,
    OP_STD,
    OP_INC_GRP4,
    OP_IND_GRP5,
    OP_GRP6,
    OP_GRP7,
    OP_LAR,
    OP_LSL,
    OP_SYSCALL,
    OP_CLTS,
    OP_SYSRET,
    OP_INVD,
    OP_WBINVD,
    OP_ILLUD2,
    OP_FEMMS,
    OP_3DNOW,
    OP_MOVUPS,
    OP_MOVLPS,
    OP_MOVHLPS = OP_MOVLPS, /**< @todo OP_MOVHLPS */
    OP_UNPCKLPS,
    OP_MOVHPS,
    OP_MOVLHPS = OP_MOVHPS, /**< @todo OP_MOVLHPS */
    OP_UNPCKHPS,
    OP_PREFETCH_GRP16,
    OP_MOV_CR,
    OP_MOVAPS,
    OP_CVTPI2PS,
    OP_MOVNTPS,
    OP_CVTTPS2PI,
    OP_CVTPS2PI,
    OP_UCOMISS,
    OP_COMISS,
    OP_WRMSR,
    OP_RDTSC,
    OP_RDTSCP,
    OP_RDMSR,
    OP_RDPMC,
    OP_SYSENTER,
    OP_SYSEXIT,
    OP_GETSEC,
    OP_PAUSE,
    OP_CMOVO,
    OP_CMOVNO,
    OP_CMOVC,
    OP_CMOVNC,
    OP_CMOVZ,
    OP_CMOVNZ,
    OP_CMOVBE,
    OP_CMOVNBE,
    OP_CMOVS,
    OP_CMOVNS,
    OP_CMOVP,
    OP_CMOVNP,
    OP_CMOVL,
    OP_CMOVNL,
    OP_CMOVLE,
    OP_CMOVNLE,
    OP_MOVMSKPS,
    OP_SQRTPS,
    OP_RSQRTPS,
    OP_RCPPS,
    OP_ANDPS,
    OP_ANDNPS,
    OP_ORPS,
    OP_XORPS,
    OP_ADDPS,
    OP_MULPS,
    OP_CVTPS2PD,
    OP_CVTDQ2PS,
    OP_SUBPS,
    OP_MINPS,
    OP_DIVPS,
    OP_MAXPS,
    OP_PUNPCKLBW,
    OP_PUNPCKLWD,
    OP_PUNPCKLDQ,
    OP_PACKSSWB,
    OP_PCMPGTB,
    OP_PCMPGTW,
    OP_PCMPGTD,
    OP_PCMPGTQ,
    OP_PACKUSWB,
    OP_PUNPCKHBW,
    OP_PUNPCKHWD,
    OP_PUNPCKHDQ,
    OP_PACKSSDW,
    OP_MOVD,
    OP_MOVQ,
    OP_PSHUFW,
    OP_3B_ESC4,
    OP_3B_ESC5,
    OP_PCMPEQB,
    OP_PCMPEQW,
    OP_PCMPEQD,
    OP_PCMPEQQ,
    OP_SETO,
    OP_SETNO,
    OP_SETC,
    OP_SETNC,
    OP_SETE,
    OP_SETNE,
    OP_SETBE,
    OP_SETNBE,
    OP_SETS,
    OP_SETNS,
    OP_SETP,
    OP_SETNP,
    OP_SETL,
    OP_SETNL,
    OP_SETLE,
    OP_SETNLE,
    OP_CPUID,
    OP_BT,
    OP_SHLD,
    OP_RSM,
    OP_BTS,
    OP_SHRD,
    OP_GRP15,
    OP_CMPXCHG,
    OP_LSS,
    OP_BTR,
    OP_LFS,
    OP_LGS,
    OP_MOVZX,
    OP_GRP10_INV,
    OP_GRP8,
    OP_BTC,
    OP_BSF,
    OP_BSR,
    OP_MOVSX,
    OP_XADD,
    OP_CMPPS,
    OP_MOVNTI,
    OP_PINSRW,
    OP_PEXTRW,
    OP_SHUFPS,
    OP_GRP9,
    OP_BSWAP,
    OP_ADDSUBPS,
    OP_ADDSUBPD,
    OP_PSRLW,
    OP_PSRLD,
    OP_PSRLQ,
    OP_PADDQ,
    OP_PMULLW,
    OP_PMOVMSKB,
    OP_PSUBUSB,
    OP_PSUBUSW,
    OP_PMINUB,
    OP_PAND,
    OP_PADDUSB,
    OP_PADDUSW,
    OP_PMAXUB,
    OP_PANDN,
    OP_PAVGB,
    OP_PSRAW,
    OP_PSRAD,
    OP_PAVGW,
    OP_PMULHUW,
    OP_PMULHW,
    OP_MOVNTQ,
    OP_PSUBSB,
    OP_PSUBSW,
    OP_PMINSW,
    OP_POR,
    OP_PADDSB,
    OP_PADDSW,
    OP_PMAXSW,
    OP_PXOR,
    OP_LDDQU,
    OP_PSLLW,
    OP_PSLLD,
    OP_PSSQ,
    OP_PMULUDQ,
    OP_PMADDWD,
    OP_PSADBW,
    OP_MASKMOVQ,
    OP_PSUBB,
    OP_PSUBW,
    OP_PSUBD,
    OP_PSUBQ,
    OP_PADDB,
    OP_PADDW,
    OP_PADDD,
    OP_MOVUPD,
    OP_MOVLPD,
    OP_UNPCKLPD,
    OP_UNPCKHPD,
    OP_MOVHPD,
    OP_MOVAPD,
    OP_CVTPI2PD,
    OP_MOVNTPD,
    OP_CVTTPD2PI,
    OP_CVTPD2PI,
    OP_UCOMISD,
    OP_COMISD,
    OP_MOVMSKPD,
    OP_SQRTPD,
    OP_ANDPD,
    OP_ANDNPD,
    OP_ORPD,
    OP_XORPD,
    OP_ADDPD,
    OP_MULPD,
    OP_CVTPD2PS,
    OP_CVTPS2DQ,
    OP_SUBPD,
    OP_MINPD,
    OP_DIVPD,
    OP_MAXPD,
    OP_GRP12,
    OP_GRP13,
    OP_GRP14,
    OP_GRP17,
    OP_EMMS,
    OP_MMX_UD78,
    OP_MMX_UD79,
    OP_MMX_UD7A,
    OP_MMX_UD7B,
    OP_MMX_UD7C,
    OP_MMX_UD7D,
    OP_PUNPCKLQDQ,
    OP_PUNPCKHQDQ,
    OP_MOVDQA,
    OP_PSHUFD,
    OP_CMPPD,
    OP_SHUFPD,
    OP_CVTTPD2DQ,
    OP_MOVNTDQ,
    OP_MOVNTDQA,
    OP_PACKUSDW,
    OP_PSHUFB,
    OP_PHADDW,
    OP_PHADDD,
    OP_PHADDSW,
    OP_HADDPS,
    OP_HADDPD,
    OP_PMADDUBSW,
    OP_PHSUBW,
    OP_PHSUBD,
    OP_PHSUBSW,
    OP_HSUBPS,
    OP_HSUBPD,
    OP_PSIGNB,
    OP_PSIGNW,
    OP_PSIGND,
    OP_PMULHRSW,
    OP_PERMILPS,
    OP_PERMILPD,
    OP_TESTPS,
    OP_TESTPD,
    OP_PBLENDVB,
    OP_CVTPH2PS,
    OP_BLENDVPS,
    OP_BLENDVPD,
    OP_PERMPS,
    OP_PERMD,
    OP_PTEST,
    OP_BROADCASTSS,
    OP_BROADCASTSD,
    OP_BROADCASTF128,
    OP_PABSB,
    OP_PABSW,
    OP_PABSD,
    OP_PMOVSXBW,
    OP_PMOVSXBD,
    OP_PMOVSXBQ,
    OP_PMOVSXWD,
    OP_PMOVSXWQ,
    OP_PMOVSXDQ,
    OP_PMOVZXBW,
    OP_PMOVZXBD,
    OP_PMOVZXBQ,
    OP_PMOVZXWD,
    OP_PMOVZXWQ,
    OP_PMOVZXDQ,
    OP_PMULDQ,
    OP_PMINSB,
    OP_PMINSD,
    OP_PMINUW,
    OP_PMINUD,
    OP_PMAXSB,
    OP_PMAXSD,
    OP_PMAXUW,
    OP_PMAXUD,
    OP_PMULLD,
    OP_PHMINPOSUW,
    OP_PSRLVD,
    OP_PSRAVD,
    OP_PSLLVD,
    OP_PBROADCASTD,
    OP_PBROADCASTQ,
    OP_PBROADCASTI128,
    OP_PBROADCASTB,
    OP_PBROADCASTW,
    OP_PMASKMOVD,
    OP_GATHER,
    OP_FMADDSUB132PS,
    OP_FMSUBADD132PS,
    OP_FMADD132PS,
    OP_FMADD132SS,
    OP_FMSUB132PS,
    OP_FMSUB132SS,
    OP_FNMADD132PS,
    OP_FNMADD132SS,
    OP_FNMSUB132PS,
    OP_FNMSUB132SS,
    OP_FMADDSUB213PS,
    OP_FMSUBADD213PS,
    OP_FMADD213PS,
    OP_FMADD213SS,
    OP_FMSUB213PS,
    OP_FMSUB213SS,
    OP_FNMADD213PS,
    OP_FNMADD213SS,
    OP_FNMSUB213PS,
    OP_FNMSUB213SS,
    OP_FMADDSUB231PS,
    OP_FMSUBADD231PS,
    OP_FMADD231PS,
    OP_FMADD231SS,
    OP_FMSUB231PS,
    OP_FMSUB231SS,
    OP_FNMADD231PS,
    OP_FNMADD231SS,
    OP_FNMSUB231PS,
    OP_FNMSUB231SS,
    OP_AESIMC,
    OP_AESENC,
    OP_AESENCLAST,
    OP_AESDEC,
    OP_AESDECLAST,
    OP_MOVBEGM,
    OP_MOVBEMG,
    OP_CRC32,
    OP_POPCNT,
    OP_TZCNT,
    OP_LZCNT,
    OP_ADCX,
    OP_ADOX,
    OP_ANDN,
    OP_BZHI,
    OP_BEXTR,
    OP_BLSR,
    OP_BLSMSK,
    OP_BLSI,
    OP_PEXT,
    OP_PDEP,
    OP_SHLX,
    OP_SHRX,
    OP_SARX,
    OP_MULX,
    OP_MASKMOVDQU,
    OP_MASKMOVPS,
    OP_MASKMOVPD,
    OP_MOVSD,
    OP_CVTSI2SD,
    OP_CVTTSD2SI,
    OP_CVTSD2SI,
    OP_SQRTSD,
    OP_ADDSD,
    OP_MULSD,
    OP_CVTSD2SS,
    OP_SUBSD,
    OP_MINSD,
    OP_DIVSD,
    OP_MAXSD,
    OP_PSHUFLW,
    OP_CMPSD,
    OP_MOVDQ2Q,
    OP_CVTPD2DQ,
    OP_MOVSS,
    OP_MOVSLDUP,
    OP_MOVDDUP,
    OP_MOVSHDUP,
    OP_CVTSI2SS,
    OP_CVTTSS2SI,
    OP_CVTSS2SI,
    OP_CVTSS2SD,
    OP_SQRTSS,
    OP_RSQRTSS,
    OP_RCPSS,
    OP_ADDSS,
    OP_MULSS,
    OP_CVTTPS2DQ,
    OP_SUBSS,
    OP_MINSS,
    OP_DIVSS,
    OP_MAXSS,
    OP_MOVDQU,
    OP_PSHUFHW,
    OP_CMPSS,
    OP_MOVQ2DQ,
    OP_CVTDQ2PD,
    OP_PERMQ,
    OP_PERMPD,
    OP_PBLENDD,
    OP_PERM2F128,
    OP_ROUNDPS,
    OP_ROUNDPD,
    OP_ROUNDSS,
    OP_ROUNDSD,
    OP_BLENDPS,
    OP_BLENDPD,
    OP_PBLENDW,
    OP_PALIGNR,
    OP_PEXTRB,
    OP_PEXTRD,
    OP_PEXTRQ,
    OP_EXTRACTPS,
    OP_INSERTF128,
    OP_EXTRACTF128,
    OP_CVTPS2PH,
    OP_PINSRB,
    OP_PINSRD,
    OP_PINSRQ,
    OP_INSERTPS,
    OP_INSERTI128,
    OP_EXTRACTI128,
    OP_DPPS,
    OP_DPPD,
    OP_MPSADBW,
    OP_PCLMULQDQ,
    OP_PERM2I128,
    OP_PCMPESTRM,
    OP_PCMPESTRI,
    OP_PCMPISTRM,
    OP_PCMPISTRI,
    OP_AESKEYGEN,
    OP_RORX,
    OP_RDRAND,
    OP_RDSEED,
    OP_MOVBE,
    OP_VEX3B,
    OP_VEX2B,
/** @} */

/** @name Floating point ops
  * @{ */
    OP_FADD,
    OP_FMUL,
    OP_FCOM,
    OP_FCOMP,
    OP_FSUB,
    OP_FSUBR,
    OP_FDIV,
    OP_FDIVR,
    OP_FLD,
    OP_FST,
    OP_FSTP,
    OP_FLDENV,
    OP_FSTENV,
    OP_FSTCW,
    OP_FXCH,
    OP_FNOP,
    OP_FCHS,
    OP_FABS,
    OP_FLD1,
    OP_FLDL2T,
    OP_FLDL2E,
    OP_FLDPI,
    OP_FLDLG2,
    OP_FLDLN2,
    OP_FLDZ,
    OP_F2XM1,
    OP_FYL2X,
    OP_FPTAN,
    OP_FPATAN,
    OP_FXTRACT,
    OP_FREM1,
    OP_FDECSTP,
    OP_FINCSTP,
    OP_FPREM,
    OP_FYL2XP1,
    OP_FSQRT,
    OP_FSINCOS,
    OP_FRNDINT,
    OP_FSCALE,
    OP_FSIN,
    OP_FCOS,
    OP_FIADD,
    OP_FIMUL,
    OP_FISUB,
    OP_FISUBR,
    OP_FIDIV,
    OP_FIDIVR,
    OP_FCMOVB,
    OP_FCMOVE,
    OP_FCMOVBE,
    OP_FCMOVU,
    OP_FUCOMPP,
    OP_FILD,
    OP_FIST,
    OP_FISTP,
    OP_FCMOVNB,
    OP_FCMOVNE,
    OP_FCMOVNBE,
    OP_FCMOVNU,
    OP_FCLEX,
    OP_FINIT,
    OP_FUCOMI,
    OP_FCOMI,
    OP_FRSTOR,
    OP_FSAVE,
    OP_FNSTSW,
    OP_FFREE,
    OP_FUCOM,
    OP_FUCOMP,
    OP_FICOM,
    OP_FICOMP,
    OP_FADDP,
    OP_FMULP,
    OP_FCOMPP,
    OP_FSUBRP,
    OP_FSUBP,
    OP_FDIVRP,
    OP_FDIVP,
    OP_FBLD,
    OP_FBSTP,
    OP_FCOMIP,
    OP_FUCOMIP,
/** @} */

/** @name 3DNow!
 * @{ */
    OP_PI2FW,
    OP_PI2FD,
    OP_PF2IW,
    OP_PF2ID,
    OP_PFPNACC,
    OP_PFCMPGE,
    OP_PFMIN,
    OP_PFRCP,
    OP_PFRSQRT,
    OP_PFSUB,
    OP_PFADD,
    OP_PFCMPGT,
    OP_PFMAX,
    OP_PFRCPIT1,
    OP_PFRSQRTIT1,
    OP_PFSUBR,
    OP_PFACC,
    OP_PFCMPEQ,
    OP_PFMUL,
    OP_PFRCPIT2,
    OP_PFMULHRW,
    OP_PFSWAPD,
    OP_PAVGUSB,
    OP_PFNACC,
/** @}  */
    OP_ROL,
    OP_ROR,
    OP_RCL,
    OP_RCR,
    OP_SHL,
    OP_SHR,
    OP_SAR,
    OP_NOT,
    OP_NEG,
    OP_MUL,
    OP_DIV,
    OP_IDIV,
    OP_SLDT,
    OP_STR,
    OP_LLDT,
    OP_LTR,
    OP_VERR,
    OP_VERW,
    OP_SGDT,
    OP_LGDT,
    OP_SIDT,
    OP_LIDT,
    OP_SMSW,
    OP_LMSW,
    OP_INVLPG,
    OP_CMPXCHG8B,
    OP_PSLLQ,
    OP_PSRLDQ,
    OP_PSLLDQ,
    OP_FXSAVE,
    OP_FXRSTOR,
    OP_LDMXCSR,
    OP_STMXCSR,
    OP_XSAVE,
    OP_XSAVEOPT,
    OP_XRSTOR,
    OP_XGETBV,
    OP_XSETBV,
    OP_RDFSBASE,
    OP_RDGSBASE,
    OP_WRFSBASE,
    OP_WRGSBASE,
    OP_LFENCE,
    OP_MFENCE,
    OP_SFENCE,
    OP_PREFETCH,
    OP_MONITOR,
    OP_MWAIT,
    OP_CLFLUSH,
    OP_CLFLUSHOPT,
    OP_MOV_DR,
    OP_MOV_TR,
    OP_SWAPGS,
    OP_UD1,
    OP_UD2,
/** @name VT-x instructions
 * @{ */
    OP_VMREAD,
    OP_VMWRITE,
    OP_VMCALL,
    OP_VMXON,
    OP_VMXOFF,
    OP_VMCLEAR,
    OP_VMLAUNCH,
    OP_VMRESUME,
    OP_VMPTRLD,
    OP_VMPTRST,
    OP_INVEPT,
    OP_INVVPID,
    OP_INVPCID,
    OP_VMFUNC,
/** @}  */
/** @name AMD-V instructions
 * @{ */
    OP_VMMCALL,
    OP_VMRUN,
    OP_VMLOAD,
    OP_VMSAVE,
    OP_CLGI,
    OP_STGI,
    OP_INVLPGA,
    OP_SKINIT,
/** @}  */
/** @name 64 bits instruction
 * @{ */
    OP_MOVSXD,
/** @} */
/** @name AVX instructions
 * @{ */
    /* Manual */
    OP_VSTMXCSR,
    OP_VLDMXCSR,
    OP_VPACKUSDW,

    /* Generated from tables: */
    OP_VADDPD,
    OP_VADDPS,
    OP_VADDSD,
    OP_VADDSS,
    OP_VADDSUBPD,
    OP_VADDSUBPS,
    OP_VAESDEC,
    OP_VAESDECLAST,
    OP_VAESENC,
    OP_VAESENCLAST,
    OP_VAESIMC,
    OP_VAESKEYGEN,
    OP_VANDNPD,
    OP_VANDNPS,
    OP_VANDPD,
    OP_VANDPS,
    OP_VBLENDPD,
    OP_VBLENDPS,
    OP_VBLENDVPD,
    OP_VBLENDVPS,
    OP_VBROADCASTF128,
    OP_VBROADCASTSD,
    OP_VBROADCASTSS,
    OP_VCMPSD,
    OP_VCMPSS,
    OP_VCOMISD,
    OP_VCOMISS,
    OP_VCVTDQ2PD,
    OP_VCVTDQ2PS,
    OP_VCVTPD2DQ,
    OP_VCVTPD2PS,
    OP_VCVTPH2PS,
    OP_VCVTPS2DQ,
    OP_VCVTPS2PD,
    OP_VCVTPS2PH,
    OP_VCVTSD2SS,
    OP_VCVTSI2SS,
    OP_VCVTSS2SD,
    OP_VCVTSS2SI,
    OP_VCVTTPD2DQ,
    OP_VCVTTPS2DQ,
    OP_VCVTTSS2SI,
    OP_VDIVPD,
    OP_VDIVPS,
    OP_VDIVSD,
    OP_VDIVSS,
    OP_VDPPD,
    OP_VDPPS,
    OP_VEXTRACTF128,
    OP_VEXTRACTI128,
    OP_VEXTRACTPS,
    OP_VFMADD132PS,
    OP_VFMADD132SS,
    OP_VFMADD213PS,
    OP_VFMADD213SS,
    OP_VFMADD231PS,
    OP_VFMADD231SS,
    OP_VFMADDSUB132PS,
    OP_VFMADDSUB213PS,
    OP_VFMADDSUB231PS,
    OP_VFMSUB132PS,
    OP_VFMSUB132SS,
    OP_VFMSUB213PS,
    OP_VFMSUB213SS,
    OP_VFMSUB231PS,
    OP_VFMSUB231SS,
    OP_VFMSUBADD132PS,
    OP_VFMSUBADD213PS,
    OP_VFMSUBADD231PS,
    OP_VFNMADD132PS,
    OP_VFNMADD132SS,
    OP_VFNMADD213PS,
    OP_VFNMADD213SS,
    OP_VFNMADD231PS,
    OP_VFNMADD231SS,
    OP_VFNMSUB132PS,
    OP_VFNMSUB132SS,
    OP_VFNMSUB213PS,
    OP_VFNMSUB213SS,
    OP_VFNMSUB231PS,
    OP_VFNMSUB231SS,
    OP_VGATHER,
    OP_VHADDPD,
    OP_VHADDPS,
    OP_VHSUBPD,
    OP_VHSUBPS,
    OP_VINSERTF128,
    OP_VINSERTI128,
    OP_VINSERTPS,
    OP_VLDDQU,
    OP_VMASKMOVDQU,
    OP_VMASKMOVPD,
    OP_VMASKMOVPS,
    OP_VMAXPD,
    OP_VMAXPS,
    OP_VMAXSD,
    OP_VMAXSS,
    OP_VMINPD,
    OP_VMINPS,
    OP_VMINSD,
    OP_VMINSS,
    OP_VMOVAPD,
    OP_VMOVAPS,
    OP_VMOVD,
    OP_VMOVDDUP,
    OP_VMOVDQA,
    OP_VMOVDQU,
    OP_VMOVHPD,
    OP_VMOVHPS,
    OP_VMOVLHPS = OP_VMOVHPS, /**< @todo OP_VMOVHPS */
    OP_VMOVLPD,
    OP_VMOVLPS,
    OP_VMOVHLPS = OP_VMOVLPS, /**< @todo OP_VMOVLPS */
    OP_VMOVMSKPD,
    OP_VMOVMSKPS,
    OP_VMOVNTDQ,
    OP_VMOVNTDQA,
    OP_VMOVNTPD,
    OP_VMOVNTPS,
    OP_VMOVQ,
    OP_VMOVSD,
    OP_VMOVSHDUP,
    OP_VMOVSLDUP,
    OP_VMOVSS,
    OP_VMOVUPD,
    OP_VMOVUPS,
    OP_VMPSADBW,
    OP_VMULPD,
    OP_VMULPS,
    OP_VMULSD,
    OP_VMULSS,
    OP_VORPD,
    OP_VORPS,
    OP_VPABSB,
    OP_VPABSD,
    OP_VPABSW,
    OP_VPACKSSDW,
    OP_VPACKSSWB,
    OP_VPACKUSWB,
    OP_VPADDB,
    OP_VPADDD,
    OP_VPADDQ,
    OP_VPADDSB,
    OP_VPADDSW,
    OP_VPADDUSB,
    OP_VPADDUSW,
    OP_VPADDW,
    OP_VPALIGNR,
    OP_VPAND,
    OP_VPANDN,
    OP_VPAVGB,
    OP_VPAVGW,
    OP_VPBLENDD,
    OP_VPBLENDVB,
    OP_VPBLENDW,
    OP_VPBROADCASTB,
    OP_VPBROADCASTD,
    OP_VPBROADCASTI128,
    OP_VPBROADCASTQ,
    OP_VPBROADCASTW,
    OP_VPCLMULQDQ,
    OP_VPCMPEQB,
    OP_VPCMPEQD,
    OP_VPCMPEQQ,
    OP_VPCMPEQW,
    OP_VPCMPESTRI,
    OP_VPCMPESTRM,
    OP_VPCMPGTB,
    OP_VPCMPGTD,
    OP_VPCMPGTQ,
    OP_VPCMPGTW,
    OP_VPCMPISTRI,
    OP_VPCMPISTRM,
    OP_VPERM2F128,
    OP_VPERM2I128,
    OP_VPERMD,
    OP_VPERMILPD,
    OP_VPERMILPS,
    OP_VPERMPD,
    OP_VPERMPS,
    OP_VPERMQ,
    OP_VPEXTRB,
    OP_VPEXTRD,
    OP_VPEXTRW,
    OP_VPEXTRQ,
    OP_VPHADDD,
    OP_VPHADDSW,
    OP_VPHADDW,
    OP_VPHMINPOSUW,
    OP_VPHSUBD,
    OP_VPHSUBSW,
    OP_VPHSUBW,
    OP_VPINSRB,
    OP_VPINSRD,
    OP_VPINSRW,
    OP_VPINSRQ,
    OP_VPMADDUBSW,
    OP_VPMADDWD,
    OP_VPMASKMOVD,
    OP_VPMAXSB,
    OP_VPMAXSD,
    OP_VPMAXSW,
    OP_VPMAXUB,
    OP_VPMAXUD,
    OP_VPMAXUW,
    OP_VPMINSB,
    OP_VPMINSD,
    OP_VPMINSW,
    OP_VPMINUB,
    OP_VPMINUD,
    OP_VPMINUW,
    OP_VPMOVMSKB,
    OP_VPMOVSXBW,
    OP_VPMOVSXBD,
    OP_VPMOVSXBQ,
    OP_VPMOVSXWD,
    OP_VPMOVSXWQ,
    OP_VPMOVSXDQ,
    OP_VPMOVZXBW,
    OP_VPMOVZXBD,
    OP_VPMOVZXBQ,
    OP_VPMOVZXWD,
    OP_VPMOVZXWQ,
    OP_VPMOVZXDQ,
    OP_VPMULDQ,
    OP_VPMULHRSW,
    OP_VPMULHUW,
    OP_VPMULHW,
    OP_VPMULLD,
    OP_VPMULLW,
    OP_VPMULUDQ,
    OP_VPOR,
    OP_VPSADBW,
    OP_VPSHUFB,
    OP_VPSHUFD,
    OP_VPSHUFHW,
    OP_VPSHUFLW,
    OP_VPSIGNB,
    OP_VPSIGND,
    OP_VPSIGNW,
    OP_VPSLLD,
    OP_VPSLLQ,
    OP_VPSLLVD,
    OP_VPSLLW,
    OP_VPSRAD,
    OP_VPSRAVD,
    OP_VPSRAW,
    OP_VPSRLD,
    OP_VPSRLQ,
    OP_VPSRLVD,
    OP_VPSRLW,
    OP_VPSUBB,
    OP_VPSUBD,
    OP_VPSUBQ,
    OP_VPSUBSB,
    OP_VPSUBSW,
    OP_VPSUBUSB,
    OP_VPSUBUSW,
    OP_VPSUBW,
    OP_VPTEST,
    OP_VPUNPCKHBW,
    OP_VPUNPCKHDQ,
    OP_VPUNPCKHQDQ,
    OP_VPUNPCKHWD,
    OP_VPUNPCKLBW,
    OP_VPUNPCKLDQ,
    OP_VPUNPCKLQDQ,
    OP_VPUNPCKLWD,
    OP_VPXOR,
    OP_VRCPPS,
    OP_VRCPSS,
    OP_VROUNDPD,
    OP_VROUNDPS,
    OP_VROUNDSD,
    OP_VROUNDSS,
    OP_VRSQRTPS,
    OP_VRSQRTSS,
    OP_VSHUFPD,
    OP_VSHUFPS,
    OP_VSQRTPD,
    OP_VSQRTPS,
    OP_VSQRTSD,
    OP_VSQRTSS,
    OP_VSUBPD,
    OP_VSUBPS,
    OP_VSUBSD,
    OP_VSUBSS,
    OP_VTESTPD,
    OP_VTESTPS,
    OP_VUCOMISD,
    OP_VUCOMISS,
    OP_VUNPCKHPD,
    OP_VUNPCKHPS,
    OP_VUNPCKLPD,
    OP_VUNPCKLPS,
    OP_VVPACKUSDW,
    OP_VXORPD,
    OP_VXORPS,
    OP_VZEROALL,

/** @} */
    OP_END_OF_OPCODES
};
AssertCompile(OP_LOCK == 7);
#if 0
AssertCompile(OP_END_OF_OPCODES < 1024 /* see 15 byte DISOPCODE variant */);
#endif
/** @} */


/** @defgroup grp_dis_opparam Opcode parameters (DISOPCODE::fParam1,
 *            DISOPCODE::fParam2, DISOPCODE::fParam3)
 * @ingroup grp_dis
 * @{
 */

/**
 * @remarks Register order is important for translations!!
 */
enum OP_PARM
{
    OP_PARM_NONE,

    OP_PARM_REG_EAX,
    OP_PARM_REG_GEN32_START = OP_PARM_REG_EAX,
    OP_PARM_REG_ECX,
    OP_PARM_REG_EDX,
    OP_PARM_REG_EBX,
    OP_PARM_REG_ESP,
    OP_PARM_REG_EBP,
    OP_PARM_REG_ESI,
    OP_PARM_REG_EDI,
    OP_PARM_REG_GEN32_END = OP_PARM_REG_EDI,

    OP_PARM_REG_ES,
    OP_PARM_REG_SEG_START = OP_PARM_REG_ES,
    OP_PARM_REG_CS,
    OP_PARM_REG_SS,
    OP_PARM_REG_DS,
    OP_PARM_REG_FS,
    OP_PARM_REG_GS,
    OP_PARM_REG_SEG_END = OP_PARM_REG_GS,

    OP_PARM_REG_AX,
    OP_PARM_REG_GEN16_START = OP_PARM_REG_AX,
    OP_PARM_REG_CX,
    OP_PARM_REG_DX,
    OP_PARM_REG_BX,
    OP_PARM_REG_SP,
    OP_PARM_REG_BP,
    OP_PARM_REG_SI,
    OP_PARM_REG_DI,
    OP_PARM_REG_GEN16_END = OP_PARM_REG_DI,

    OP_PARM_REG_AL,
    OP_PARM_REG_GEN8_START = OP_PARM_REG_AL,
    OP_PARM_REG_CL,
    OP_PARM_REG_DL,
    OP_PARM_REG_BL,
    OP_PARM_REG_AH,
    OP_PARM_REG_CH,
    OP_PARM_REG_DH,
    OP_PARM_REG_BH,
    OP_PARM_REG_GEN8_END = OP_PARM_REG_BH,

    OP_PARM_REGFP_0,
    OP_PARM_REG_FP_START = OP_PARM_REGFP_0,
    OP_PARM_REGFP_1,
    OP_PARM_REGFP_2,
    OP_PARM_REGFP_3,
    OP_PARM_REGFP_4,
    OP_PARM_REGFP_5,
    OP_PARM_REGFP_6,
    OP_PARM_REGFP_7,
    OP_PARM_REG_FP_END = OP_PARM_REGFP_7,

    OP_PARM_NTA,
    OP_PARM_T0,
    OP_PARM_T1,
    OP_PARM_T2,
    OP_PARM_1,

    OP_PARM_REX,
    OP_PARM_REX_START = OP_PARM_REX,
    OP_PARM_REX_B,
    OP_PARM_REX_X,
    OP_PARM_REX_XB,
    OP_PARM_REX_R,
    OP_PARM_REX_RB,
    OP_PARM_REX_RX,
    OP_PARM_REX_RXB,
    OP_PARM_REX_W,
    OP_PARM_REX_WB,
    OP_PARM_REX_WX,
    OP_PARM_REX_WXB,
    OP_PARM_REX_WR,
    OP_PARM_REX_WRB,
    OP_PARM_REX_WRX,
    OP_PARM_REX_WRXB,

    OP_PARM_REG_RAX,
    OP_PARM_REG_GEN64_START = OP_PARM_REG_RAX,
    OP_PARM_REG_RCX,
    OP_PARM_REG_RDX,
    OP_PARM_REG_RBX,
    OP_PARM_REG_RSP,
    OP_PARM_REG_RBP,
    OP_PARM_REG_RSI,
    OP_PARM_REG_RDI,
    OP_PARM_REG_R8,
    OP_PARM_REG_R9,
    OP_PARM_REG_R10,
    OP_PARM_REG_R11,
    OP_PARM_REG_R12,
    OP_PARM_REG_R13,
    OP_PARM_REG_R14,
    OP_PARM_REG_R15,
    OP_PARM_REG_GEN64_END = OP_PARM_REG_R15
};


/* 8-bit GRP aliases (for IEM). */
#define OP_PARM_AL OP_PARM_REG_AL

/* GPR aliases for op-size specified register sizes (for IEM). */
#define OP_PARM_rAX OP_PARM_REG_EAX
#define OP_PARM_rCX OP_PARM_REG_ECX
#define OP_PARM_rDX OP_PARM_REG_EDX
#define OP_PARM_rBX OP_PARM_REG_EBX
#define OP_PARM_rSP OP_PARM_REG_ESP
#define OP_PARM_rBP OP_PARM_REG_EBP
#define OP_PARM_rSI OP_PARM_REG_ESI
#define OP_PARM_rDI OP_PARM_REG_EDI

/* SREG aliases (for IEM). */
#define OP_PARM_ES  OP_PARM_REG_ES
#define OP_PARM_CS  OP_PARM_REG_CS
#define OP_PARM_SS  OP_PARM_REG_SS
#define OP_PARM_DS  OP_PARM_REG_DS
#define OP_PARM_FS  OP_PARM_REG_FS
#define OP_PARM_GS  OP_PARM_REG_GS

/*
 * Note! We don't document anything here if we can help it, because it we love
 *       wasting other peoples time figuring out crypting crap.  The new VEX
 *       stuff of course uphelds this vexing tradition.  Aaaaaaaaaaaaaaaaaaarg!
 */

#define OP_PARM_VTYPE(a)        ((unsigned)a & 0xFE0)
#define OP_PARM_VSUBTYPE(a)     ((unsigned)a & 0x01F)

#define OP_PARM_A               0x100
#define OP_PARM_VARIABLE        OP_PARM_A
#define OP_PARM_E               0x120
#define OP_PARM_F               0x140
#define OP_PARM_G               0x160
#define OP_PARM_I               0x180
#define OP_PARM_J               0x1A0
#define OP_PARM_M               0x1C0
#define OP_PARM_O               0x1E0
#define OP_PARM_R               0x200
#define OP_PARM_X               0x220
#define OP_PARM_Y               0x240

/* Grouped rare parameters for optimization purposes */
#define IS_OP_PARM_RARE(a)      ((a & 0xF00) >= 0x300)
#define OP_PARM_C               0x300       /* control register */
#define OP_PARM_D               0x320       /* debug register */
#define OP_PARM_S               0x340       /* segment register */
#define OP_PARM_T               0x360       /* test register */
#define OP_PARM_Q               0x380
#define OP_PARM_P               0x3A0       /* mmx register */
#define OP_PARM_W               0x3C0       /* xmm register */
#define OP_PARM_V               0x3E0
#define OP_PARM_U               0x400       /* The R/M field of the ModR/M byte selects XMM/YMM register. */
#define OP_PARM_B               0x420       /* VEX.vvvv field select general purpose register. */
#define OP_PARM_H               0x440
#define OP_PARM_L               0x460

#define OP_PARM_NONE            0
#define OP_PARM_a               0x1     /**< Operand to bound instruction. */
#define OP_PARM_b               0x2     /**< Byte (always). */
#define OP_PARM_d               0x3     /**< Double word (always).  */
#define OP_PARM_dq              0x4     /**< Double quad word (always). */
#define OP_PARM_p               0x5     /**< Far pointer (subject to opsize). */
#define OP_PARM_pd              0x6     /**< 128-bit or 256-bit double precision floating point data. */
#define OP_PARM_pi              0x7     /**< Quad word MMX register. */
#define OP_PARM_ps              0x8     /**< 128-bit or 256-bit single precision floating point data. */
#define OP_PARM_q               0xA     /**< Quad word (always). */
#define OP_PARM_s               0xB     /**< Descriptor table size (SIDT/LIDT/SGDT/LGDT). */
#define OP_PARM_sd              0xC     /**< Scalar element of 128-bit double precision floating point data. */
#define OP_PARM_ss              0xD     /**< Scalar element of 128-bit single precision floating point data. */
#define OP_PARM_v               0xE     /**< Word, double word, or quad word depending on opsize. */
#define OP_PARM_w               0xF     /**< Word (always). */
#define OP_PARM_x               0x10    /**< Double quad word (dq) or quad quad word (qq) depending on opsize. */
#define OP_PARM_y               0x11    /**< Double word or quad word depending on opsize. */
#define OP_PARM_z               0x12    /**< Word (16-bit opsize) or double word (32-bit/64-bit opsize). */
#define OP_PARM_qq              0x13    /**< Quad quad word. */


#define OP_PARM_Ap              (OP_PARM_A+OP_PARM_p)
#define OP_PARM_By              (OP_PARM_B+OP_PARM_y)
#define OP_PARM_Cd              (OP_PARM_C+OP_PARM_d)
#define OP_PARM_Dd              (OP_PARM_D+OP_PARM_d)
#define OP_PARM_Eb              (OP_PARM_E+OP_PARM_b)
#define OP_PARM_Ed              (OP_PARM_E+OP_PARM_d)
#define OP_PARM_Ep              (OP_PARM_E+OP_PARM_p)
#define OP_PARM_Ev              (OP_PARM_E+OP_PARM_v)
#define OP_PARM_Ew              (OP_PARM_E+OP_PARM_w)
#define OP_PARM_Ey              (OP_PARM_E+OP_PARM_y)
#define OP_PARM_Fv              (OP_PARM_F+OP_PARM_v)
#define OP_PARM_Gb              (OP_PARM_G+OP_PARM_b)
#define OP_PARM_Gd              (OP_PARM_G+OP_PARM_d)
#define OP_PARM_Gv              (OP_PARM_G+OP_PARM_v)
#define OP_PARM_Gw              (OP_PARM_G+OP_PARM_w)
#define OP_PARM_Gy              (OP_PARM_G+OP_PARM_y)
#define OP_PARM_Hq              (OP_PARM_H+OP_PARM_q)
#define OP_PARM_Hps             (OP_PARM_H+OP_PARM_ps)
#define OP_PARM_Hpd             (OP_PARM_H+OP_PARM_pd)
#define OP_PARM_Hdq             (OP_PARM_H+OP_PARM_dq)
#define OP_PARM_Hqq             (OP_PARM_H+OP_PARM_qq)
#define OP_PARM_Hsd             (OP_PARM_H+OP_PARM_sd)
#define OP_PARM_Hss             (OP_PARM_H+OP_PARM_ss)
#define OP_PARM_Hx              (OP_PARM_H+OP_PARM_x)
#define OP_PARM_Ib              (OP_PARM_I+OP_PARM_b)
#define OP_PARM_Id              (OP_PARM_I+OP_PARM_d)
#define OP_PARM_Iq              (OP_PARM_I+OP_PARM_q)
#define OP_PARM_Iw              (OP_PARM_I+OP_PARM_w)
#define OP_PARM_Iv              (OP_PARM_I+OP_PARM_v)
#define OP_PARM_Iz              (OP_PARM_I+OP_PARM_z)
#define OP_PARM_Jb              (OP_PARM_J+OP_PARM_b)
#define OP_PARM_Jv              (OP_PARM_J+OP_PARM_v)
#define OP_PARM_Ma              (OP_PARM_M+OP_PARM_a)
#define OP_PARM_Mb              (OP_PARM_M+OP_PARM_b)
#define OP_PARM_Mw              (OP_PARM_M+OP_PARM_w)
#define OP_PARM_Md              (OP_PARM_M+OP_PARM_d)
#define OP_PARM_Mp              (OP_PARM_M+OP_PARM_p)
#define OP_PARM_Mq              (OP_PARM_M+OP_PARM_q)
#define OP_PARM_Mdq             (OP_PARM_M+OP_PARM_dq)
#define OP_PARM_Ms              (OP_PARM_M+OP_PARM_s)
#define OP_PARM_Mx              (OP_PARM_M+OP_PARM_x)
#define OP_PARM_My              (OP_PARM_M+OP_PARM_y)
#define OP_PARM_Mps             (OP_PARM_M+OP_PARM_ps)
#define OP_PARM_Mpd             (OP_PARM_M+OP_PARM_pd)
#define OP_PARM_Ob              (OP_PARM_O+OP_PARM_b)
#define OP_PARM_Ov              (OP_PARM_O+OP_PARM_v)
#define OP_PARM_Pq              (OP_PARM_P+OP_PARM_q)
#define OP_PARM_Pd              (OP_PARM_P+OP_PARM_d)
#define OP_PARM_Qd              (OP_PARM_Q+OP_PARM_d)
#define OP_PARM_Qq              (OP_PARM_Q+OP_PARM_q)
#define OP_PARM_Rd              (OP_PARM_R+OP_PARM_d)
#define OP_PARM_Rw              (OP_PARM_R+OP_PARM_w)
#define OP_PARM_Ry              (OP_PARM_R+OP_PARM_y)
#define OP_PARM_Sw              (OP_PARM_S+OP_PARM_w)
#define OP_PARM_Td              (OP_PARM_T+OP_PARM_d)
#define OP_PARM_Ux              (OP_PARM_U+OP_PARM_x)
#define OP_PARM_Vq              (OP_PARM_V+OP_PARM_q)
#define OP_PARM_Vx              (OP_PARM_V+OP_PARM_x)
#define OP_PARM_Vy              (OP_PARM_V+OP_PARM_y)
#define OP_PARM_Wq              (OP_PARM_W+OP_PARM_q)
/*#define OP_PARM_Ws              (OP_PARM_W+OP_PARM_s) - wtf? Same as lgdt (OP_PARM_Ms)?*/
#define OP_PARM_Wx              (OP_PARM_W+OP_PARM_x)
#define OP_PARM_Xb              (OP_PARM_X+OP_PARM_b)
#define OP_PARM_Xv              (OP_PARM_X+OP_PARM_v)
#define OP_PARM_Yb              (OP_PARM_Y+OP_PARM_b)
#define OP_PARM_Yv              (OP_PARM_Y+OP_PARM_v)

#define OP_PARM_Vps             (OP_PARM_V+OP_PARM_ps)
#define OP_PARM_Vss             (OP_PARM_V+OP_PARM_ss)
#define OP_PARM_Vpd             (OP_PARM_V+OP_PARM_pd)
#define OP_PARM_Vdq             (OP_PARM_V+OP_PARM_dq)
#define OP_PARM_Wps             (OP_PARM_W+OP_PARM_ps)
#define OP_PARM_Wpd             (OP_PARM_W+OP_PARM_pd)
#define OP_PARM_Wss             (OP_PARM_W+OP_PARM_ss)
#define OP_PARM_Ww              (OP_PARM_W+OP_PARM_w)
#define OP_PARM_Wd              (OP_PARM_W+OP_PARM_d)
#define OP_PARM_Wq              (OP_PARM_W+OP_PARM_q)
#define OP_PARM_Wdq             (OP_PARM_W+OP_PARM_dq)
#define OP_PARM_Wqq             (OP_PARM_W+OP_PARM_qq)
#define OP_PARM_Ppi             (OP_PARM_P+OP_PARM_pi)
#define OP_PARM_Qpi             (OP_PARM_Q+OP_PARM_pi)
#define OP_PARM_Qdq             (OP_PARM_Q+OP_PARM_dq)
#define OP_PARM_Vsd             (OP_PARM_V+OP_PARM_sd)
#define OP_PARM_Wsd             (OP_PARM_W+OP_PARM_sd)
#define OP_PARM_Vqq             (OP_PARM_V+OP_PARM_qq)
#define OP_PARM_Pdq             (OP_PARM_P+OP_PARM_dq)
#define OP_PARM_Ups             (OP_PARM_U+OP_PARM_ps)
#define OP_PARM_Upd             (OP_PARM_U+OP_PARM_pd)
#define OP_PARM_Udq             (OP_PARM_U+OP_PARM_dq)
#define OP_PARM_Lx              (OP_PARM_L+OP_PARM_x)

/* For making IEM / bs3-cpu-generated-1 happy: */
#define OP_PARM_Ed_WO           OP_PARM_Ed              /**< Annotates write only operand. */
#define OP_PARM_Eq              (OP_PARM_E+OP_PARM_q)
#define OP_PARM_Eq_WO           OP_PARM_Eq              /**< Annotates write only operand. */
#define OP_PARM_Gv_RO           OP_PARM_Gv              /**< Annotates read only first operand (default is readwrite). */
#define OP_PARM_HssHi           OP_PARM_Hx              /**< Register referenced by VEX.vvvv, bits [127:32]. */
#define OP_PARM_HsdHi           OP_PARM_Hx              /**< Register referenced by VEX.vvvv, bits [127:64]. */
#define OP_PARM_HqHi            OP_PARM_Hx              /**< Register referenced by VEX.vvvv, bits [127:64]. */
#define OP_PARM_M_RO            OP_PARM_M               /**< Annotates read only memory of variable operand size (xrstor). */
#define OP_PARM_M_RW            OP_PARM_M               /**< Annotates read-write memory of variable operand size (xsave). */
#define OP_PARM_Mb_RO           OP_PARM_Mb              /**< Annotates read only memory byte operand. */
#define OP_PARM_Md_RO           OP_PARM_Md              /**< Annotates read only memory operand. */
#define OP_PARM_Md_WO           OP_PARM_Md              /**< Annotates write only memory operand. */
#define OP_PARM_Mdq_WO          OP_PARM_Mdq             /**< Annotates write only memory operand. */
#define OP_PARM_Mq_WO           OP_PARM_Mq              /**< Annotates write only memory quad word operand. */
#define OP_PARM_Mps_WO          OP_PARM_Mps             /**< Annotates write only memory operand. */
#define OP_PARM_Mpd_WO          OP_PARM_Mpd             /**< Annotates write only memory operand. */
#define OP_PARM_Mx_WO           OP_PARM_Mx             /**< Annotates write only memory operand. */
#define OP_PARM_PdZx_WO         OP_PARM_Pd              /**< Annotates write only operand and zero extends to 64-bit. */
#define OP_PARM_Pq_WO           OP_PARM_Pq              /**< Annotates write only operand. */
#define OP_PARM_Qq_WO           OP_PARM_Qq              /**< Annotates write only operand. */
#define OP_PARM_Nq              OP_PARM_Qq              /**< Missing 'N' class (MMX reg selected by modrm.mem) in disasm. */
#define OP_PARM_Uq              (OP_PARM_U+OP_PARM_q)
#define OP_PARM_UqHi            (OP_PARM_U+OP_PARM_dq)
#define OP_PARM_Uss             (OP_PARM_U+OP_PARM_ss)
#define OP_PARM_Uss_WO          OP_PARM_Uss             /**< Annotates write only operand. */
#define OP_PARM_Usd             (OP_PARM_U+OP_PARM_sd)
#define OP_PARM_Usd_WO          OP_PARM_Usd             /**< Annotates write only operand. */
#define OP_PARM_Vd              (OP_PARM_V+OP_PARM_d)
#define OP_PARM_Vd_WO           OP_PARM_Vd              /**< Annotates write only operand. */
#define OP_PARM_VdZx_WO         OP_PARM_Vd              /**< Annotates that the registers get their upper bits cleared */
#define OP_PARM_Vdq_WO          OP_PARM_Vdq             /**< Annotates that only YMM/XMM[127:64] are accessed. */
#define OP_PARM_Vpd_WO          OP_PARM_Vpd             /**< Annotates write only operand. */
#define OP_PARM_Vps_WO          OP_PARM_Vps             /**< Annotates write only operand. */
#define OP_PARM_Vq_WO           OP_PARM_Vq              /**< Annotates write only operand. */
#define OP_PARM_VqHi            OP_PARM_Vdq             /**< Annotates that only YMM/XMM[127:64] are accessed. */
#define OP_PARM_VqHi_WO         OP_PARM_Vdq             /**< Annotates that only YMM/XMM[127:64] are written. */
#define OP_PARM_VqZx_WO         OP_PARM_Vq              /**< Annotates that the registers get their upper bits cleared */
#define OP_PARM_VsdZx_WO        OP_PARM_Vsd             /**< Annotates that the registers get their upper bits cleared. */
#define OP_PARM_VssZx_WO        OP_PARM_Vss             /**< Annotates that the registers get their upper bits cleared. */
#define OP_PARM_Vss_WO          OP_PARM_Vss             /**< Annotates write only operand. */
#define OP_PARM_Vsd_WO          OP_PARM_Vsd             /**< Annotates write only operand. */
#define OP_PARM_Vx_WO           OP_PARM_Vx              /**< Annotates write only operand. */
#define OP_PARM_Wpd_WO          OP_PARM_Wpd             /**< Annotates write only operand. */
#define OP_PARM_Wps_WO          OP_PARM_Wps             /**< Annotates write only operand. */
#define OP_PARM_Wq_WO           OP_PARM_Wq              /**< Annotates write only operand. */
#define OP_PARM_WqZxReg_WO      OP_PARM_Wq              /**< Annotates that register targets get their upper bits cleared. */
#define OP_PARM_Wss_WO          OP_PARM_Wss             /**< Annotates write only operand. */
#define OP_PARM_Wsd_WO          OP_PARM_Wsd             /**< Annotates write only operand. */
#define OP_PARM_Wx_WO           OP_PARM_Wx              /**< Annotates write only operand. */

/** @} */

#endif /* !VBOX_INCLUDED_disopcode_h */

