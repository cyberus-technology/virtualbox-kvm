/* $Id: DisasmFormatYasm.cpp $ */
/** @file
 * VBox Disassembler - Yasm(/Nasm) Style Formatter.
 */

/*
 * Copyright (C) 2008-2023 Oracle and/or its affiliates.
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
#include <VBox/dis.h>
#include "DisasmInternal.h"
#include <iprt/assert.h>
#include <iprt/ctype.h>
#include <iprt/err.h>
#include <iprt/string.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static const char g_szSpaces[] =
"                                                                               ";
static const char g_aszYasmRegGen8[20][5] =
{
    "al\0\0", "cl\0\0", "dl\0\0", "bl\0\0", "ah\0\0", "ch\0\0", "dh\0\0", "bh\0\0", "r8b\0",  "r9b\0",  "r10b",  "r11b",  "r12b",  "r13b",  "r14b",  "r15b",  "spl\0",  "bpl\0",  "sil\0",  "dil\0"
};
static const char g_aszYasmRegGen16[16][5] =
{
    "ax\0\0", "cx\0\0", "dx\0\0", "bx\0\0", "sp\0\0", "bp\0\0", "si\0\0", "di\0\0", "r8w\0",  "r9w\0",  "r10w",  "r11w",  "r12w",  "r13w",  "r14w",  "r15w"
};
#if 0 /* unused */
static const char g_aszYasmRegGen1616[8][6] =
{
    "bx+si", "bx+di", "bp+si", "bp+di", "si\0\0\0", "di\0\0\0", "bp\0\0\0", "bx\0\0\0"
};
#endif
static const char g_aszYasmRegGen32[16][5] =
{
    "eax\0",  "ecx\0",  "edx\0",  "ebx\0",  "esp\0",  "ebp\0",  "esi\0",  "edi\0",  "r8d\0",  "r9d\0",  "r10d",  "r11d",  "r12d",  "r13d",  "r14d",  "r15d"
};
static const char g_aszYasmRegGen64[16][4] =
{
    "rax",    "rcx",    "rdx",    "rbx",    "rsp",    "rbp",    "rsi",    "rdi",    "r8\0",   "r9\0",   "r10",   "r11",   "r12",   "r13",   "r14",   "r15"
};
static const char g_aszYasmRegSeg[6][3] =
{
    "es",     "cs",     "ss",      "ds",    "fs",     "gs"
};
static const char g_aszYasmRegFP[8][4] =
{
    "st0",    "st1",    "st2",    "st3",    "st4",    "st5",    "st6",    "st7"
};
static const char g_aszYasmRegMMX[8][4] =
{
    "mm0",    "mm1",    "mm2",    "mm3",    "mm4",    "mm5",    "mm6",    "mm7"
};
static const char g_aszYasmRegXMM[16][6] =
{
    "xmm0\0", "xmm1\0", "xmm2\0", "xmm3\0", "xmm4\0", "xmm5\0", "xmm6\0", "xmm7\0", "xmm8\0", "xmm9\0", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15"
};
static const char g_aszYasmRegYMM[16][6] =
{
    "ymm0\0", "ymm1\0", "ymm2\0", "ymm3\0", "ymm4\0", "ymm5\0", "ymm6\0", "ymm7\0", "ymm8\0", "ymm9\0", "ymm10", "ymm11", "ymm12", "ymm13", "ymm14", "ymm15"
};
static const char g_aszYasmRegCRx[16][5] =
{
    "cr0\0",  "cr1\0",  "cr2\0",  "cr3\0",  "cr4\0",  "cr5\0",  "cr6\0",  "cr7\0",  "cr8\0",  "cr9\0",  "cr10",  "cr11",  "cr12",  "cr13",  "cr14",  "cr15"
};
static const char g_aszYasmRegDRx[16][5] =
{
    "dr0\0",  "dr1\0",  "dr2\0",  "dr3\0",  "dr4\0",  "dr5\0",  "dr6\0",  "dr7\0",  "dr8\0",  "dr9\0",  "dr10",  "dr11",  "dr12",  "dr13",  "dr14",  "dr15"
};
static const char g_aszYasmRegTRx[16][5] =
{
    "tr0\0",  "tr1\0",  "tr2\0",  "tr3\0",  "tr4\0",  "tr5\0",  "tr6\0",  "tr7\0",  "tr8\0",  "tr9\0",  "tr10",  "tr11",  "tr12",  "tr13",  "tr14",  "tr15"
};



/**
 * Gets the base register name for the given parameter.
 *
 * @returns Pointer to the register name.
 * @param   pDis        The disassembler state.
 * @param   pParam      The parameter.
 * @param   pcchReg     Where to store the length of the name.
 */
static const char *disasmFormatYasmBaseReg(PCDISSTATE pDis, PCDISOPPARAM pParam, size_t *pcchReg)
{
    RT_NOREF_PV(pDis);

    switch (pParam->fUse & (  DISUSE_REG_GEN8 | DISUSE_REG_GEN16 | DISUSE_REG_GEN32 | DISUSE_REG_GEN64
                            | DISUSE_REG_FP   | DISUSE_REG_MMX   | DISUSE_REG_XMM   | DISUSE_REG_YMM
                            | DISUSE_REG_CR   | DISUSE_REG_DBG   | DISUSE_REG_SEG   | DISUSE_REG_TEST))

    {
        case DISUSE_REG_GEN8:
        {
            Assert(pParam->Base.idxGenReg < RT_ELEMENTS(g_aszYasmRegGen8));
            const char *psz = g_aszYasmRegGen8[pParam->Base.idxGenReg];
            *pcchReg = 2 + !!psz[2] + !!psz[3];
            return psz;
        }

        case DISUSE_REG_GEN16:
        {
            Assert(pParam->Base.idxGenReg < RT_ELEMENTS(g_aszYasmRegGen16));
            const char *psz = g_aszYasmRegGen16[pParam->Base.idxGenReg];
            *pcchReg = 2 + !!psz[2] + !!psz[3];
            return psz;
        }

        // VSIB
        case DISUSE_REG_XMM | DISUSE_REG_GEN32:
        case DISUSE_REG_YMM | DISUSE_REG_GEN32:
        case DISUSE_REG_GEN32:
        {
            Assert(pParam->Base.idxGenReg < RT_ELEMENTS(g_aszYasmRegGen32));
            const char *psz = g_aszYasmRegGen32[pParam->Base.idxGenReg];
            *pcchReg = 2 + !!psz[2] + !!psz[3];
            return psz;
        }

        // VSIB
        case DISUSE_REG_XMM | DISUSE_REG_GEN64:
        case DISUSE_REG_YMM | DISUSE_REG_GEN64:
        case DISUSE_REG_GEN64:
        {
            Assert(pParam->Base.idxGenReg < RT_ELEMENTS(g_aszYasmRegGen64));
            const char *psz = g_aszYasmRegGen64[pParam->Base.idxGenReg];
            *pcchReg = 2 + !!psz[2] + !!psz[3];
            return psz;
        }

        case DISUSE_REG_FP:
        {
            Assert(pParam->Base.idxFpuReg < RT_ELEMENTS(g_aszYasmRegFP));
            const char *psz = g_aszYasmRegFP[pParam->Base.idxFpuReg];
            *pcchReg = 3;
            return psz;
        }

        case DISUSE_REG_MMX:
        {
            Assert(pParam->Base.idxMmxReg < RT_ELEMENTS(g_aszYasmRegMMX));
            const char *psz = g_aszYasmRegMMX[pParam->Base.idxMmxReg];
            *pcchReg = 3;
            return psz;
        }

        case DISUSE_REG_XMM:
        {
            Assert(pParam->Base.idxXmmReg < RT_ELEMENTS(g_aszYasmRegXMM));
            const char *psz = g_aszYasmRegXMM[pParam->Base.idxXmmReg];
            *pcchReg = 4 + !!psz[4];
            return psz;
        }

        case DISUSE_REG_YMM:
        {
            Assert(pParam->Base.idxYmmReg < RT_ELEMENTS(g_aszYasmRegYMM));
            const char *psz = g_aszYasmRegYMM[pParam->Base.idxYmmReg];
            *pcchReg = 4 + !!psz[4];
            return psz;
        }

        case DISUSE_REG_CR:
        {
            Assert(pParam->Base.idxCtrlReg < RT_ELEMENTS(g_aszYasmRegCRx));
            const char *psz = g_aszYasmRegCRx[pParam->Base.idxCtrlReg];
            *pcchReg = 3;
            return psz;
        }

        case DISUSE_REG_DBG:
        {
            Assert(pParam->Base.idxDbgReg < RT_ELEMENTS(g_aszYasmRegDRx));
            const char *psz = g_aszYasmRegDRx[pParam->Base.idxDbgReg];
            *pcchReg = 3;
            return psz;
        }

        case DISUSE_REG_SEG:
        {
            Assert(pParam->Base.idxSegReg < RT_ELEMENTS(g_aszYasmRegCRx));
            const char *psz = g_aszYasmRegSeg[pParam->Base.idxSegReg];
            *pcchReg = 2;
            return psz;
        }

        case DISUSE_REG_TEST:
        {
            Assert(pParam->Base.idxTestReg < RT_ELEMENTS(g_aszYasmRegTRx));
            const char *psz = g_aszYasmRegTRx[pParam->Base.idxTestReg];
            *pcchReg = 3;
            return psz;
        }

        default:
            AssertMsgFailed(("%#x\n", pParam->fUse));
            *pcchReg = 3;
            return "r??";
    }
}


/**
 * Gets the index register name for the given parameter.
 *
 * @returns The index register name.
 * @param   pDis        The disassembler state.
 * @param   pParam      The parameter.
 * @param   pcchReg     Where to store the length of the name.
 */
static const char *disasmFormatYasmIndexReg(PCDISSTATE pDis, PCDISOPPARAM pParam, size_t *pcchReg)
{
    if (pParam->fUse & DISUSE_REG_XMM)
    {
        Assert(pParam->Index.idxXmmReg < RT_ELEMENTS(g_aszYasmRegXMM));
        const char *psz = g_aszYasmRegXMM[pParam->Index.idxXmmReg];
        *pcchReg = 4 + !!psz[4];
        return psz;
    }
    else if (pParam->fUse & DISUSE_REG_YMM)
    {
        Assert(pParam->Index.idxYmmReg < RT_ELEMENTS(g_aszYasmRegYMM));
        const char *psz = g_aszYasmRegYMM[pParam->Index.idxYmmReg];
        *pcchReg = 4 + !!psz[4];
        return psz;

    }
    else
    switch (pDis->uAddrMode)
    {
        case DISCPUMODE_16BIT:
        {
            Assert(pParam->Index.idxGenReg < RT_ELEMENTS(g_aszYasmRegGen16));
            const char *psz = g_aszYasmRegGen16[pParam->Index.idxGenReg];
            *pcchReg = 2 + !!psz[2] + !!psz[3];
            return psz;
        }

        case DISCPUMODE_32BIT:
        {
            Assert(pParam->Index.idxGenReg < RT_ELEMENTS(g_aszYasmRegGen32));
            const char *psz = g_aszYasmRegGen32[pParam->Index.idxGenReg];
            *pcchReg = 2 + !!psz[2] + !!psz[3];
            return psz;
        }

        case DISCPUMODE_64BIT:
        {
            Assert(pParam->Index.idxGenReg < RT_ELEMENTS(g_aszYasmRegGen64));
            const char *psz = g_aszYasmRegGen64[pParam->Index.idxGenReg];
            *pcchReg = 2 + !!psz[2] + !!psz[3];
            return psz;
        }

        default:
            AssertMsgFailed(("%#x %#x\n", pParam->fUse, pDis->uAddrMode));
            *pcchReg = 3;
            return "r??";
    }
}


/**
 * Formats the current instruction in Yasm (/ Nasm) style.
 *
 *
 * @returns The number of output characters. If this is >= cchBuf, then the content
 *          of pszBuf will be truncated.
 * @param   pDis            Pointer to the disassembler state.
 * @param   pszBuf          The output buffer.
 * @param   cchBuf          The size of the output buffer.
 * @param   fFlags          Format flags, see DIS_FORMAT_FLAGS_*.
 * @param   pfnGetSymbol    Get symbol name for a jmp or call target address. Optional.
 * @param   pvUser          User argument for pfnGetSymbol.
 */
DISDECL(size_t) DISFormatYasmEx(PCDISSTATE pDis, char *pszBuf, size_t cchBuf, uint32_t fFlags,
                                PFNDISGETSYMBOL pfnGetSymbol, void *pvUser)
{
/** @todo monitor and mwait aren't formatted correctly in 64-bit mode. */
    /*
     * Input validation and massaging.
     */
    AssertPtr(pDis);
    AssertPtrNull(pszBuf);
    Assert(pszBuf || !cchBuf);
    AssertPtrNull(pfnGetSymbol);
    AssertMsg(DIS_FMT_FLAGS_IS_VALID(fFlags), ("%#x\n", fFlags));
    if (fFlags & DIS_FMT_FLAGS_ADDR_COMMENT)
        fFlags = (fFlags & ~DIS_FMT_FLAGS_ADDR_LEFT) | DIS_FMT_FLAGS_ADDR_RIGHT;
    if (fFlags & DIS_FMT_FLAGS_BYTES_COMMENT)
        fFlags = (fFlags & ~DIS_FMT_FLAGS_BYTES_LEFT) | DIS_FMT_FLAGS_BYTES_RIGHT;

    PCDISOPCODE const pOp = pDis->pCurInstr;

    /*
     * Output macros
     */
    char           *pszDst = pszBuf;
    size_t          cchDst = cchBuf;
    size_t          cchOutput = 0;
#define PUT_C(ch)       \
            do { \
                cchOutput++; \
                if (cchDst > 1) \
                { \
                    cchDst--; \
                    *pszDst++ = (ch); \
                } \
            } while (0)
#define PUT_STR(pszSrc, cchSrc) \
            do { \
                cchOutput += (cchSrc); \
                if (cchDst > (cchSrc)) \
                { \
                    memcpy(pszDst, (pszSrc), (cchSrc)); \
                    pszDst += (cchSrc); \
                    cchDst -= (cchSrc); \
                } \
                else if (cchDst > 1) \
                { \
                    memcpy(pszDst, (pszSrc), cchDst - 1); \
                    pszDst += cchDst - 1; \
                    cchDst = 1; \
                } \
            } while (0)
#define PUT_SZ(sz) \
            PUT_STR((sz), sizeof(sz) - 1)
#define PUT_SZ_STRICT(szStrict, szRelaxed) \
            do { if (fFlags & DIS_FMT_FLAGS_STRICT) PUT_SZ(szStrict); else PUT_SZ(szRelaxed); } while (0)
#define PUT_PSZ(psz) \
            do { const size_t cchTmp = strlen(psz); PUT_STR((psz), cchTmp); } while (0)
#define PUT_NUM(cch, fmt, num) \
            do { \
                 cchOutput += (cch); \
                 if (cchDst > 1) \
                 { \
                    const size_t cchTmp = RTStrPrintf(pszDst, cchDst, fmt, (num)); \
                    pszDst += cchTmp; \
                    cchDst -= cchTmp; \
                    Assert(cchTmp == (cch) || cchDst == 1); \
                 } \
            } while (0)
/** @todo add two flags for choosing between %X / %x and h / 0x. */
#define PUT_NUM_8(num)  PUT_NUM(4,  "0%02xh", (uint8_t)(num))
#define PUT_NUM_16(num) PUT_NUM(6,  "0%04xh", (uint16_t)(num))
#define PUT_NUM_32(num) PUT_NUM(10, "0%08xh", (uint32_t)(num))
#define PUT_NUM_64(num) PUT_NUM(18, "0%016RX64h", (uint64_t)(num))

#define PUT_NUM_SIGN(cch, fmt, num, stype, utype) \
            do { \
                if ((stype)(num) >= 0) \
                { \
                    PUT_C('+'); \
                    PUT_NUM(cch, fmt, (utype)(num)); \
                } \
                else \
                { \
                    PUT_C('-'); \
                    PUT_NUM(cch, fmt, (utype)-(stype)(num)); \
                } \
            } while (0)
#define PUT_NUM_S8(num)  PUT_NUM_SIGN(4,  "0%02xh", num, int8_t,  uint8_t)
#define PUT_NUM_S16(num) PUT_NUM_SIGN(6,  "0%04xh", num, int16_t, uint16_t)
#define PUT_NUM_S32(num) PUT_NUM_SIGN(10, "0%08xh", num, int32_t, uint32_t)
#define PUT_NUM_S64(num) PUT_NUM_SIGN(18, "0%016RX64h", num, int64_t, uint64_t)

#define PUT_SYMBOL_TWO(a_rcSym, a_szStart, a_chEnd) \
        do { \
            if (RT_SUCCESS(a_rcSym)) \
            { \
                PUT_SZ(a_szStart); \
                PUT_PSZ(szSymbol); \
                if (off != 0) \
                { \
                    if ((int8_t)off == off) \
                        PUT_NUM_S8(off); \
                    else if ((int16_t)off == off) \
                        PUT_NUM_S16(off); \
                    else if ((int32_t)off == off) \
                        PUT_NUM_S32(off); \
                    else \
                        PUT_NUM_S64(off); \
                } \
                PUT_C(a_chEnd); \
            } \
        } while (0)

#define PUT_SYMBOL(a_uSeg, a_uAddr, a_szStart, a_chEnd) \
        do { \
            if (pfnGetSymbol) \
            { \
                int rcSym = pfnGetSymbol(pDis, a_uSeg, a_uAddr, szSymbol, sizeof(szSymbol), &off, pvUser); \
                PUT_SYMBOL_TWO(rcSym, a_szStart, a_chEnd); \
            } \
        } while (0)


    /*
     * The address?
     */
    if (fFlags & DIS_FMT_FLAGS_ADDR_LEFT)
    {
#if HC_ARCH_BITS == 64 || GC_ARCH_BITS == 64
        if (pDis->uInstrAddr >= _4G)
            PUT_NUM(9, "%08x`", (uint32_t)(pDis->uInstrAddr >> 32));
#endif
        PUT_NUM(8, "%08x", (uint32_t)pDis->uInstrAddr);
        PUT_C(' ');
    }

    /*
     * The opcode bytes?
     */
    if (fFlags & DIS_FMT_FLAGS_BYTES_LEFT)
    {
        size_t cchTmp = disFormatBytes(pDis, pszDst, cchDst, fFlags);
        cchOutput += cchTmp;
        if (cchDst > 1)
        {
            if (cchTmp <= cchDst)
            {
                cchDst -= cchTmp;
                pszDst += cchTmp;
            }
            else
            {
                pszDst += cchDst - 1;
                cchDst = 1;
            }
        }

        /* Some padding to align the instruction. */
        size_t cchPadding = (7 * (2 + !!(fFlags & DIS_FMT_FLAGS_BYTES_SPACED)))
                          + !!(fFlags & DIS_FMT_FLAGS_BYTES_BRACKETS) * 2
                          + 2;
        cchPadding = cchTmp + 1 >= cchPadding ? 1 : cchPadding - cchTmp;
        PUT_STR(g_szSpaces, cchPadding);
    }


    /*
     * Filter out invalid opcodes first as they need special
     * treatment. UD2 is an exception and should be handled normally.
     */
    size_t const offInstruction = cchOutput;
    if (    pOp->uOpcode == OP_INVALID
        ||  (   pOp->uOpcode == OP_ILLUD2
             && (pDis->fPrefix & DISPREFIX_LOCK)))
        PUT_SZ("Illegal opcode");
    else
    {
        /*
         * Prefixes
         */
        if (pDis->fPrefix & DISPREFIX_LOCK)
            PUT_SZ("lock ");
        if (pDis->fPrefix & DISPREFIX_REP)
            PUT_SZ("rep ");
        else if(pDis->fPrefix & DISPREFIX_REPNE)
            PUT_SZ("repne ");

        /*
         * Adjust the format string to the correct mnemonic
         * or to avoid things the assembler cannot handle correctly.
         */
        char szTmpFmt[48];
        const char *pszFmt = pOp->pszOpcode;
        bool fIgnoresOpSize = false;
        bool fMayNeedAddrSize = false;
        switch (pOp->uOpcode)
        {
            case OP_JECXZ:
                pszFmt = pDis->uOpMode == DISCPUMODE_16BIT ? "jcxz %Jb" : pDis->uOpMode == DISCPUMODE_32BIT ? "jecxz %Jb"   : "jrcxz %Jb";
                break;
            case OP_PUSHF:
                pszFmt = pDis->uOpMode == DISCPUMODE_16BIT ? "pushfw"   : pDis->uOpMode == DISCPUMODE_32BIT ? "pushfd"      : "pushfq";
                break;
            case OP_POPF:
                pszFmt = pDis->uOpMode == DISCPUMODE_16BIT ? "popfw"    : pDis->uOpMode == DISCPUMODE_32BIT ? "popfd"       : "popfq";
                break;
            case OP_PUSHA:
                pszFmt = pDis->uOpMode == DISCPUMODE_16BIT ? "pushaw"   : "pushad";
                break;
            case OP_POPA:
                pszFmt = pDis->uOpMode == DISCPUMODE_16BIT ? "popaw"    : "popad";
                break;
            case OP_INSB:
                pszFmt = "insb";
                fIgnoresOpSize = fMayNeedAddrSize = true;
                break;
            case OP_INSWD:
                pszFmt = pDis->uOpMode == DISCPUMODE_16BIT ? "insw"     : pDis->uOpMode == DISCPUMODE_32BIT ? "insd"  : "insq";
                fMayNeedAddrSize = true;
                break;
            case OP_OUTSB:
                pszFmt = "outsb";
                fIgnoresOpSize = fMayNeedAddrSize = true;
                break;
            case OP_OUTSWD:
                pszFmt = pDis->uOpMode == DISCPUMODE_16BIT ? "outsw"    : pDis->uOpMode == DISCPUMODE_32BIT ? "outsd" : "outsq";
                fMayNeedAddrSize = true;
                break;
            case OP_MOVSB:
                pszFmt = "movsb";
                fIgnoresOpSize = fMayNeedAddrSize = true;
                break;
            case OP_MOVSWD:
                pszFmt = pDis->uOpMode == DISCPUMODE_16BIT ? "movsw"    : pDis->uOpMode == DISCPUMODE_32BIT ? "movsd" : "movsq";
                fMayNeedAddrSize = true;
                break;
            case OP_CMPSB:
                pszFmt = "cmpsb";
                fIgnoresOpSize = fMayNeedAddrSize = true;
                break;
            case OP_CMPWD:
                pszFmt = pDis->uOpMode == DISCPUMODE_16BIT ? "cmpsw"    : pDis->uOpMode == DISCPUMODE_32BIT ? "cmpsd" : "cmpsq";
                fMayNeedAddrSize = true;
                break;
            case OP_SCASB:
                pszFmt = "scasb";
                fIgnoresOpSize = fMayNeedAddrSize = true;
                break;
            case OP_SCASWD:
                pszFmt = pDis->uOpMode == DISCPUMODE_16BIT ? "scasw"    : pDis->uOpMode == DISCPUMODE_32BIT ? "scasd" : "scasq";
                fMayNeedAddrSize = true;
                break;
            case OP_LODSB:
                pszFmt = "lodsb";
                fIgnoresOpSize = fMayNeedAddrSize = true;
                break;
            case OP_LODSWD:
                pszFmt = pDis->uOpMode == DISCPUMODE_16BIT ? "lodsw"    : pDis->uOpMode == DISCPUMODE_32BIT ? "lodsd" : "lodsq";
                fMayNeedAddrSize = true;
                break;
            case OP_STOSB:
                pszFmt = "stosb";
                fIgnoresOpSize = fMayNeedAddrSize = true;
                break;
            case OP_STOSWD:
                pszFmt = pDis->uOpMode == DISCPUMODE_16BIT ? "stosw"    : pDis->uOpMode == DISCPUMODE_32BIT ? "stosd" : "stosq";
                fMayNeedAddrSize = true;
                break;
            case OP_CBW:
                pszFmt = pDis->uOpMode == DISCPUMODE_16BIT ? "cbw"      : pDis->uOpMode == DISCPUMODE_32BIT ? "cwde"  : "cdqe";
                break;
            case OP_CWD:
                pszFmt = pDis->uOpMode == DISCPUMODE_16BIT ? "cwd"      : pDis->uOpMode == DISCPUMODE_32BIT ? "cdq"   : "cqo";
                break;
            case OP_SHL:
                Assert(pszFmt[3] == '/');
                pszFmt += 4;
                break;
            case OP_XLAT:
                pszFmt = "xlatb";
                break;
            case OP_INT3:
                pszFmt = "int3";
                break;

            /*
             * Don't know how to tell yasm to generate complicated nop stuff, so 'db' it.
             */
            case OP_NOP:
                if (pDis->bOpCode == 0x90)
                    /* fine, fine */;
                else if (pszFmt[sizeof("nop %Ev") - 1] == '/' && pszFmt[sizeof("nop %Ev")] == 'p')
                    pszFmt = "prefetch %Eb";
                else if (pDis->bOpCode == 0x1f)
                {
                    Assert(pDis->cbInstr >= 3);
                    PUT_SZ("db 00fh, 01fh,");
                    PUT_NUM_8(MAKE_MODRM(pDis->ModRM.Bits.Mod, pDis->ModRM.Bits.Reg, pDis->ModRM.Bits.Rm));
                    for (unsigned i = 3; i < pDis->cbInstr; i++)
                    {
                        PUT_C(',');
                        PUT_NUM_8(0x90); /// @todo fixme.
                    }
                    pszFmt = "";
                }
                break;

            default:
                /* ST(X) -> stX  (floating point) */
                if (*pszFmt == 'f' && strchr(pszFmt, '('))
                {
                    char *pszFmtDst = szTmpFmt;
                    char ch;
                    do
                    {
                        ch = *pszFmt++;
                        if (ch == 'S' && pszFmt[0] == 'T' && pszFmt[1] == '(')
                        {
                            *pszFmtDst++ = 's';
                            *pszFmtDst++ = 't';
                            pszFmt += 2;
                            ch = *pszFmt;
                            Assert(pszFmt[1] == ')');
                            pszFmt += 2;
                            *pszFmtDst++ = ch;
                        }
                        else
                            *pszFmtDst++ = ch;
                    } while (ch != '\0');
                    pszFmt = szTmpFmt;
                }
                if (strchr("#@&", *pszFmt))
                {
                    const char *pszDelim = strchr(pszFmt, '/');
                    const char *pszSpace = (pszDelim ? strchr(pszDelim, ' ') : NULL);
                    if (pszDelim != NULL)
                    {
                        char *pszFmtDst = szTmpFmt;
                        if (pszSpace == NULL) pszSpace = strchr(pszDelim, 0);
                        if (   (*pszFmt == '#' && !pDis->bVexWFlag) /** @todo check this*/
                            || (*pszFmt == '@' && !VEXREG_IS256B(pDis->bVexDestReg))
                            || (*pszFmt == '&' && (   DISUSE_IS_EFFECTIVE_ADDR(pDis->Param1.fUse)
                                                   || DISUSE_IS_EFFECTIVE_ADDR(pDis->Param2.fUse)
                                                   || DISUSE_IS_EFFECTIVE_ADDR(pDis->Param3.fUse)
                                                   || DISUSE_IS_EFFECTIVE_ADDR(pDis->Param4.fUse))))
                        {
                            strncpy(pszFmtDst, pszFmt + 1, pszDelim - pszFmt - 1);
                            pszFmtDst += pszDelim - pszFmt - 1;
                        }
                        else
                        {
                            strncpy(pszFmtDst, pszDelim + 1, pszSpace - pszDelim - 1);
                            pszFmtDst += pszSpace - pszDelim - 1;
                        }
                        strcpy (pszFmtDst, pszSpace);
                        pszFmt = szTmpFmt;
                    }
                }
                break;

            /*
             * Horrible hacks.
             */
            case OP_FLD:
                if (pDis->bOpCode == 0xdb) /* m80fp workaround. */
                    *(int *)&pDis->Param1.fParam &= ~0x1f; /* make it pure OP_PARM_M */
                break;
            case OP_LAR: /* hack w -> v, probably not correct. */
                *(int *)&pDis->Param2.fParam &= ~0x1f;
                *(int *)&pDis->Param2.fParam |= OP_PARM_v;
                break;
        }

        /*
         * Add operand size and address prefixes for outsb, movsb, etc.
         */
        if (pDis->fPrefix & (DISPREFIX_OPSIZE | DISPREFIX_ADDRSIZE))
        {
            if (fIgnoresOpSize && (pDis->fPrefix & DISPREFIX_OPSIZE) )
            {
                if (pDis->uCpuMode == DISCPUMODE_16BIT)
                    PUT_SZ("o32 ");
                else
                    PUT_SZ("o16 ");
            }
            if (fMayNeedAddrSize && (pDis->fPrefix & DISPREFIX_ADDRSIZE) )
            {
                if (pDis->uCpuMode == DISCPUMODE_16BIT)
                    PUT_SZ("a32 ");
                else
                    PUT_SZ("a16 ");
            }
        }

        /*
         * Formatting context and associated macros.
         */
        PCDISOPPARAM pParam = &pDis->Param1;
        int iParam = 1;

#define PUT_FAR() \
            do { \
                if (    OP_PARM_VSUBTYPE(pParam->fParam) == OP_PARM_p \
                    &&  pOp->uOpcode != OP_LDS /* table bugs? */ \
                    &&  pOp->uOpcode != OP_LES \
                    &&  pOp->uOpcode != OP_LFS \
                    &&  pOp->uOpcode != OP_LGS \
                    &&  pOp->uOpcode != OP_LSS ) \
                    PUT_SZ("far "); \
            } while (0)
        /** @todo mov ah,ch ends up with a byte 'override'... - check if this wasn't fixed. */
        /** @todo drop the work/dword/qword override when the src/dst is a register (except for movsx/movzx). */
#define PUT_SIZE_OVERRIDE() \
            do { \
                switch (OP_PARM_VSUBTYPE(pParam->fParam)) \
                { \
                    case OP_PARM_v: \
                    case OP_PARM_y: \
                        switch (pDis->uOpMode) \
                        { \
                            case DISCPUMODE_16BIT: if (OP_PARM_VSUBTYPE(pParam->fParam) != OP_PARM_y) PUT_SZ("word "); break; \
                            case DISCPUMODE_32BIT: \
                                if (pDis->pCurInstr->uOpcode != OP_GATHER || pDis->bVexWFlag) { PUT_SZ("dword "); break; } \
                                RT_FALL_THRU(); \
                            case DISCPUMODE_64BIT: PUT_SZ("qword "); break; \
                            default: break; \
                        } \
                        break; \
                    case OP_PARM_b: PUT_SZ("byte "); break; \
                    case OP_PARM_w: \
                        if (   OP_PARM_VTYPE(pParam->fParam) == OP_PARM_W \
                            || OP_PARM_VTYPE(pParam->fParam) == OP_PARM_M) \
                        { \
                            if (VEXREG_IS256B(pDis->bVexDestReg)) PUT_SZ("dword "); \
                            else PUT_SZ("word "); \
                        } \
                        break; \
                    case OP_PARM_d: \
                        if (   OP_PARM_VTYPE(pParam->fParam) == OP_PARM_W \
                            || OP_PARM_VTYPE(pParam->fParam) == OP_PARM_M) \
                        { \
                            if (VEXREG_IS256B(pDis->bVexDestReg)) PUT_SZ("qword "); \
                            else PUT_SZ("dword "); \
                        } \
                        break; \
                    case OP_PARM_q: \
                        if (   OP_PARM_VTYPE(pParam->fParam) == OP_PARM_W \
                            || OP_PARM_VTYPE(pParam->fParam) == OP_PARM_M) \
                        { \
                            if (VEXREG_IS256B(pDis->bVexDestReg)) PUT_SZ("oword "); \
                            else PUT_SZ("qword "); \
                        } \
                       break; \
                    case OP_PARM_ps: \
                    case OP_PARM_pd: \
                    case OP_PARM_x: if (VEXREG_IS256B(pDis->bVexDestReg)) { PUT_SZ("yword "); break; } RT_FALL_THRU(); \
                    case OP_PARM_ss: \
                    case OP_PARM_sd: \
                    case OP_PARM_dq: PUT_SZ("oword "); break; \
                    case OP_PARM_qq: PUT_SZ("yword "); break; \
                    case OP_PARM_p: break; /* see PUT_FAR */ \
                    case OP_PARM_s: if (pParam->fUse & DISUSE_REG_FP) PUT_SZ("tword "); break; /* ?? */ \
                    case OP_PARM_z: break; \
                    case OP_PARM_NONE: \
                        if (    OP_PARM_VTYPE(pParam->fParam) == OP_PARM_M \
                            &&  ((pParam->fUse & DISUSE_REG_FP) || pOp->uOpcode == OP_FLD)) \
                            PUT_SZ("tword "); \
                        break; \
                    default:        break; /*no pointer type specified/necessary*/ \
                } \
            } while (0)
        static const char s_szSegPrefix[6][4] = { "es:", "cs:", "ss:", "ds:", "fs:", "gs:" };
#define PUT_SEGMENT_OVERRIDE() \
        do { \
            if (pDis->fPrefix & DISPREFIX_SEG) \
                PUT_STR(s_szSegPrefix[pDis->idxSegPrefix], 3); \
        } while (0)


        /*
         * Segment prefixing for instructions that doesn't do memory access.
         */
        if (    (pDis->fPrefix & DISPREFIX_SEG)
            &&  !DISUSE_IS_EFFECTIVE_ADDR(pDis->Param1.fUse)
            &&  !DISUSE_IS_EFFECTIVE_ADDR(pDis->Param2.fUse)
            &&  !DISUSE_IS_EFFECTIVE_ADDR(pDis->Param3.fUse))
        {
            PUT_STR(s_szSegPrefix[pDis->idxSegPrefix], 2);
            PUT_C(' ');
        }


        /*
         * The formatting loop.
         */
        RTINTPTR off;
        char szSymbol[128];
        char ch;
        while ((ch = *pszFmt++) != '\0')
        {
            if (ch == '%')
            {
                ch = *pszFmt++;
                switch (ch)
                {
                    /*
                     * ModRM - Register only / VEX.vvvv.
                     */
                    case 'C': /* Control register (ParseModRM / UseModRM). */
                    case 'D': /* Debug register (ParseModRM / UseModRM). */
                    case 'G': /* ModRM selects general register (ParseModRM / UseModRM). */
                    case 'S': /* ModRM byte selects a segment register (ParseModRM / UseModRM). */
                    case 'T': /* ModRM byte selects a test register (ParseModRM / UseModRM). */
                    case 'V': /* ModRM byte selects an XMM/SSE register (ParseModRM / UseModRM). */
                    case 'P': /* ModRM byte selects MMX register (ParseModRM / UseModRM). */
                    case 'H': /* The VEX.vvvv field of the VEX prefix selects a XMM/YMM register. */
                    case 'B': /* The VEX.vvvv field of the VEX prefix selects a general register (ParseVexDest). */
                    case 'L': /* The upper 4 bits of the 8-bit immediate selects a XMM/YMM register. */
                    {
                        pszFmt += RT_C_IS_ALPHA(pszFmt[0]) ? RT_C_IS_ALPHA(pszFmt[1]) ? 2 : 1 : 0;
                        Assert(!(pParam->fUse & (DISUSE_INDEX | DISUSE_SCALE) /* No SIB here... */));
                        Assert(!(pParam->fUse & (DISUSE_DISPLACEMENT8 | DISUSE_DISPLACEMENT16 | DISUSE_DISPLACEMENT32 | DISUSE_DISPLACEMENT64 | DISUSE_RIPDISPLACEMENT32)));

                        size_t cchReg;
                        const char *pszReg = disasmFormatYasmBaseReg(pDis, pParam, &cchReg);
                        PUT_STR(pszReg, cchReg);
                        break;
                    }

                    /*
                     * ModRM - Register or memory.
                     */
                    case 'E': /* ModRM specifies parameter (ParseModRM / UseModRM / UseSIB). */
                    case 'Q': /* ModRM byte selects MMX register or memory address (ParseModRM / UseModRM). */
                    case 'R': /* ModRM byte may only refer to a general register (ParseModRM / UseModRM). */
                    case 'W': /* ModRM byte selects an XMM/SSE register or a memory address (ParseModRM / UseModRM). */
                    case 'U': /* ModRM byte may only refer to a XMM/SSE register (ParseModRM / UseModRM). */
                    case 'M': /* ModRM byte may only refer to memory (ParseModRM / UseModRM). */
                    {
                        pszFmt += RT_C_IS_ALPHA(pszFmt[0]) ? RT_C_IS_ALPHA(pszFmt[1]) ? 2 : 1 : 0;

                        PUT_FAR();
                        uint32_t const fUse = pParam->fUse;
                        if (DISUSE_IS_EFFECTIVE_ADDR(fUse))
                        {
                            /* Work around mov seg,[mem16]  and mov [mem16],seg as these always make a 16-bit mem
                               while the register variants deals with 16, 32 & 64 in the normal fashion. */
                            if (    pParam->fParam != OP_PARM_Ev
                                ||  pOp->uOpcode != OP_MOV
                                ||  (   pOp->fParam1 != OP_PARM_Sw
                                     && pOp->fParam2 != OP_PARM_Sw))
                                PUT_SIZE_OVERRIDE();
                            PUT_C('[');
                        }
                        if (    (fFlags & DIS_FMT_FLAGS_STRICT)
                            &&  (fUse & (DISUSE_DISPLACEMENT8 | DISUSE_DISPLACEMENT16 | DISUSE_DISPLACEMENT32 | DISUSE_DISPLACEMENT64 | DISUSE_RIPDISPLACEMENT32)))
                        {
                            if (   (fUse & DISUSE_DISPLACEMENT8)
                                && !pParam->uDisp.i8)
                                PUT_SZ("byte ");
                            else if (   (fUse & DISUSE_DISPLACEMENT16)
                                     && (int8_t)pParam->uDisp.i16 == (int16_t)pParam->uDisp.i16)
                                PUT_SZ("word ");
                            else if (   (fUse & DISUSE_DISPLACEMENT32)
                                     && (int16_t)pParam->uDisp.i32 == (int32_t)pParam->uDisp.i32) //??
                                PUT_SZ("dword ");
                            else if (   (fUse & DISUSE_DISPLACEMENT64)
                                     && (pDis->SIB.Bits.Base != 5 || pDis->ModRM.Bits.Mod != 0)
                                     && (int32_t)pParam->uDisp.i64 == (int64_t)pParam->uDisp.i64) //??
                                PUT_SZ("qword ");
                        }
                        if (DISUSE_IS_EFFECTIVE_ADDR(fUse))
                            PUT_SEGMENT_OVERRIDE();

                        bool fBase =  (fUse & DISUSE_BASE) /* When exactly is DISUSE_BASE supposed to be set? disasmModRMReg doesn't set it. */
                                   || (   (fUse & (  DISUSE_REG_GEN8
                                                   | DISUSE_REG_GEN16
                                                   | DISUSE_REG_GEN32
                                                   | DISUSE_REG_GEN64
                                                   | DISUSE_REG_FP
                                                   | DISUSE_REG_MMX
                                                   | DISUSE_REG_XMM
                                                   | DISUSE_REG_YMM
                                                   | DISUSE_REG_CR
                                                   | DISUSE_REG_DBG
                                                   | DISUSE_REG_SEG
                                                   | DISUSE_REG_TEST ))
                                       && !DISUSE_IS_EFFECTIVE_ADDR(fUse));
                        if (fBase)
                        {
                            size_t cchReg;
                            const char *pszReg = disasmFormatYasmBaseReg(pDis, pParam, &cchReg);
                            PUT_STR(pszReg, cchReg);
                        }

                        if (fUse & DISUSE_INDEX)
                        {
                            if (fBase)
                                PUT_C('+');

                            size_t cchReg;
                            const char *pszReg = disasmFormatYasmIndexReg(pDis, pParam, &cchReg);
                            PUT_STR(pszReg, cchReg);

                            if (fUse & DISUSE_SCALE)
                            {
                                PUT_C('*');
                                PUT_C('0' + pParam->uScale);
                            }
                        }
                        else
                            Assert(!(fUse & DISUSE_SCALE));

                        int64_t off2 = 0;
                        if (fUse & (DISUSE_DISPLACEMENT8 | DISUSE_DISPLACEMENT16 | DISUSE_DISPLACEMENT32 | DISUSE_DISPLACEMENT64 | DISUSE_RIPDISPLACEMENT32))
                        {
                            if (fUse & DISUSE_DISPLACEMENT8)
                                off2 = pParam->uDisp.i8;
                            else if (fUse & DISUSE_DISPLACEMENT16)
                                off2 = pParam->uDisp.i16;
                            else if (fUse & (DISUSE_DISPLACEMENT32 | DISUSE_RIPDISPLACEMENT32))
                                off2 = pParam->uDisp.i32;
                            else if (fUse & DISUSE_DISPLACEMENT64)
                                off2 = pParam->uDisp.i64;
                            else
                            {
                                AssertFailed();
                                off2 = 0;
                            }

                            int64_t off3 = off2;
                            if (fBase || (fUse & (DISUSE_INDEX | DISUSE_RIPDISPLACEMENT32)))
                            {
                                PUT_C(off3 >= 0 ? '+' : '-');
                                if (off3 < 0)
                                    off3 = -off3;
                            }
                            if (fUse & DISUSE_DISPLACEMENT8)
                                PUT_NUM_8( off3);
                            else if (fUse & DISUSE_DISPLACEMENT16)
                                PUT_NUM_16(off3);
                            else if (fUse & DISUSE_DISPLACEMENT32)
                                PUT_NUM_32(off3);
                            else if (fUse & DISUSE_DISPLACEMENT64)
                                PUT_NUM_64(off3);
                            else
                            {
                                PUT_NUM_32(off3);
                                PUT_SZ(" wrt rip (");
                                off2 += pDis->uInstrAddr + pDis->cbInstr;
                                PUT_NUM_64(off2);
                                if (pfnGetSymbol)
                                    PUT_SYMBOL((pDis->fPrefix & DISPREFIX_SEG)
                                               ? DIS_FMT_SEL_FROM_REG(pDis->idxSegPrefix)
                                               : DIS_FMT_SEL_FROM_REG(DISSELREG_DS),
                                               pDis->uAddrMode == DISCPUMODE_64BIT
                                               ? (uint64_t)off2
                                               : pDis->uAddrMode == DISCPUMODE_32BIT
                                               ? (uint32_t)off2
                                               : (uint16_t)off2,
                                               " = ",
                                               ')');
                                else
                                    PUT_C(')');
                            }
                        }

                        if (DISUSE_IS_EFFECTIVE_ADDR(fUse))
                        {
                            if (pfnGetSymbol && !fBase && !(fUse & (DISUSE_INDEX | DISUSE_RIPDISPLACEMENT32)) && off2 != 0)
                                PUT_SYMBOL((pDis->fPrefix & DISPREFIX_SEG)
                                           ? DIS_FMT_SEL_FROM_REG(pDis->idxSegPrefix)
                                           : DIS_FMT_SEL_FROM_REG(DISSELREG_DS),
                                           pDis->uAddrMode == DISCPUMODE_64BIT
                                           ? (uint64_t)off2
                                           : pDis->uAddrMode == DISCPUMODE_32BIT
                                           ? (uint32_t)off2
                                           : (uint16_t)off2,
                                           " (=",
                                           ')');
                            PUT_C(']');
                        }
                        break;
                    }

                    case 'F': /* Eflags register (0 - popf/pushf only, avoided in adjustments above). */
                        AssertFailed();
                        break;

                    case 'I': /* Immediate data (ParseImmByte, ParseImmByteSX, ParseImmV, ParseImmUshort, ParseImmZ). */
                        Assert(*pszFmt == 'b' || *pszFmt == 'v' || *pszFmt == 'w' || *pszFmt == 'z'); pszFmt++;
                        switch (pParam->fUse & (  DISUSE_IMMEDIATE8 | DISUSE_IMMEDIATE16 | DISUSE_IMMEDIATE32 | DISUSE_IMMEDIATE64
                                                | DISUSE_IMMEDIATE16_SX8 | DISUSE_IMMEDIATE32_SX8 | DISUSE_IMMEDIATE64_SX8))
                        {
                            case DISUSE_IMMEDIATE8:
                                if (    (fFlags & DIS_FMT_FLAGS_STRICT)
                                    &&  (   (pOp->fParam1 >= OP_PARM_REG_GEN8_START && pOp->fParam1 <= OP_PARM_REG_GEN8_END)
                                         || (pOp->fParam2 >= OP_PARM_REG_GEN8_START && pOp->fParam2 <= OP_PARM_REG_GEN8_END))
                                   )
                                    PUT_SZ("strict byte ");
                                PUT_NUM_8(pParam->uValue);
                                break;

                            case DISUSE_IMMEDIATE16:
                                if (    pDis->uCpuMode != pDis->uOpMode
                                    ||  (   (fFlags & DIS_FMT_FLAGS_STRICT)
                                         && (   (int8_t)pParam->uValue == (int16_t)pParam->uValue
                                             || (pOp->fParam1 >= OP_PARM_REG_GEN16_START && pOp->fParam1 <= OP_PARM_REG_GEN16_END)
                                             || (pOp->fParam2 >= OP_PARM_REG_GEN16_START && pOp->fParam2 <= OP_PARM_REG_GEN16_END))
                                        )
                                   )
                                {
                                    if (OP_PARM_VSUBTYPE(pParam->fParam) == OP_PARM_b)
                                        PUT_SZ_STRICT("strict byte ", "byte ");
                                    else if (   OP_PARM_VSUBTYPE(pParam->fParam) == OP_PARM_v
                                             || OP_PARM_VSUBTYPE(pParam->fParam) == OP_PARM_z)
                                        PUT_SZ_STRICT("strict word ", "word ");
                                }
                                PUT_NUM_16(pParam->uValue);
                                break;

                            case DISUSE_IMMEDIATE16_SX8:
                                if (   !(pDis->fPrefix & DISPREFIX_OPSIZE)
                                    || pDis->pCurInstr->uOpcode != OP_PUSH)
                                    PUT_SZ_STRICT("strict byte ", "byte ");
                                else
                                    PUT_SZ("word ");
                                PUT_NUM_16(pParam->uValue);
                                break;

                            case DISUSE_IMMEDIATE32:
                                if (    pDis->uOpMode != (pDis->uCpuMode == DISCPUMODE_16BIT ? DISCPUMODE_16BIT : DISCPUMODE_32BIT) /* not perfect */
                                    ||  (   (fFlags & DIS_FMT_FLAGS_STRICT)
                                         && (   (int8_t)pParam->uValue == (int32_t)pParam->uValue
                                             || (pOp->fParam1 >= OP_PARM_REG_GEN32_START && pOp->fParam1 <= OP_PARM_REG_GEN32_END)
                                             || (pOp->fParam2 >= OP_PARM_REG_GEN32_START && pOp->fParam2 <= OP_PARM_REG_GEN32_END))
                                        )
                                    )
                                {
                                    if (OP_PARM_VSUBTYPE(pParam->fParam) == OP_PARM_b)
                                        PUT_SZ_STRICT("strict byte ", "byte ");
                                    else if (   OP_PARM_VSUBTYPE(pParam->fParam) == OP_PARM_v
                                             || OP_PARM_VSUBTYPE(pParam->fParam) == OP_PARM_z)
                                        PUT_SZ_STRICT("strict dword ", "dword ");
                                }
                                PUT_NUM_32(pParam->uValue);
                                if (pDis->uCpuMode == DISCPUMODE_32BIT)
                                    PUT_SYMBOL(DIS_FMT_SEL_FROM_REG(DISSELREG_CS), pParam->uValue, " (=", ')');
                                break;

                            case DISUSE_IMMEDIATE32_SX8:
                                if (   !(pDis->fPrefix & DISPREFIX_OPSIZE)
                                    || pDis->pCurInstr->uOpcode != OP_PUSH)
                                    PUT_SZ_STRICT("strict byte ", "byte ");
                                else
                                    PUT_SZ("dword ");
                                PUT_NUM_32(pParam->uValue);
                                break;

                            case DISUSE_IMMEDIATE64_SX8:
                                if (   !(pDis->fPrefix & DISPREFIX_OPSIZE)
                                    || pDis->pCurInstr->uOpcode != OP_PUSH)
                                    PUT_SZ_STRICT("strict byte ", "byte ");
                                else
                                    PUT_SZ("qword ");
                                PUT_NUM_64(pParam->uValue);
                                break;

                            case DISUSE_IMMEDIATE64:
                                PUT_NUM_64(pParam->uValue);
                                break;

                            default:
                                AssertFailed();
                                break;
                        }
                        break;

                    case 'J': /* Relative jump offset (ParseImmBRel + ParseImmVRel). */
                    {
                        int32_t offDisplacement;
                        Assert(iParam == 1);
                        bool fPrefix = (fFlags & DIS_FMT_FLAGS_STRICT)
                                    && pOp->uOpcode != OP_CALL
                                    && pOp->uOpcode != OP_LOOP
                                    && pOp->uOpcode != OP_LOOPE
                                    && pOp->uOpcode != OP_LOOPNE
                                    && pOp->uOpcode != OP_JECXZ;
                        if (pOp->uOpcode == OP_CALL)
                            fFlags &= ~DIS_FMT_FLAGS_RELATIVE_BRANCH;

                        if (pParam->fUse & DISUSE_IMMEDIATE8_REL)
                        {
                            if (fPrefix)
                                PUT_SZ("short ");
                            offDisplacement = (int8_t)pParam->uValue;
                            Assert(*pszFmt == 'b'); pszFmt++;

                            if (fFlags & DIS_FMT_FLAGS_RELATIVE_BRANCH)
                                PUT_NUM_S8(offDisplacement);
                        }
                        else if (pParam->fUse & DISUSE_IMMEDIATE16_REL)
                        {
                            if (fPrefix)
                                PUT_SZ("near ");
                            offDisplacement = (int16_t)pParam->uValue;
                            Assert(*pszFmt == 'v'); pszFmt++;

                            if (fFlags & DIS_FMT_FLAGS_RELATIVE_BRANCH)
                                PUT_NUM_S16(offDisplacement);
                        }
                        else
                        {
                            if (fPrefix)
                                PUT_SZ("near ");
                            offDisplacement = (int32_t)pParam->uValue;
                            Assert(pParam->fUse & (DISUSE_IMMEDIATE32_REL | DISUSE_IMMEDIATE64_REL));
                            Assert(*pszFmt == 'v'); pszFmt++;

                            if (fFlags & DIS_FMT_FLAGS_RELATIVE_BRANCH)
                                PUT_NUM_S32(offDisplacement);
                        }
                        if (fFlags & DIS_FMT_FLAGS_RELATIVE_BRANCH)
                            PUT_SZ(" (");

                        RTUINTPTR uTrgAddr = pDis->uInstrAddr + pDis->cbInstr + offDisplacement;
                        if (pDis->uCpuMode == DISCPUMODE_16BIT)
                            PUT_NUM_16(uTrgAddr);
                        else if (pDis->uCpuMode == DISCPUMODE_32BIT)
                            PUT_NUM_32(uTrgAddr);
                        else
                            PUT_NUM_64(uTrgAddr);

                        if (fFlags & DIS_FMT_FLAGS_RELATIVE_BRANCH)
                        {
                            PUT_SYMBOL(DIS_FMT_SEL_FROM_REG(DISSELREG_CS), uTrgAddr, " = ", ' ');
                            PUT_C(')');
                        }
                        else
                            PUT_SYMBOL(DIS_FMT_SEL_FROM_REG(DISSELREG_CS), uTrgAddr, " (", ')');
                        break;
                    }

                    case 'A': /* Direct (jump/call) address (ParseImmAddr). */
                    {
                        Assert(*pszFmt == 'p'); pszFmt++;
                        PUT_FAR();
                        PUT_SIZE_OVERRIDE();
                        PUT_SEGMENT_OVERRIDE();
                        off = 0;
                        int rc = VERR_SYMBOL_NOT_FOUND;
                        switch (pParam->fUse & (DISUSE_IMMEDIATE_ADDR_16_16 | DISUSE_IMMEDIATE_ADDR_16_32 | DISUSE_DISPLACEMENT64 | DISUSE_DISPLACEMENT32 | DISUSE_DISPLACEMENT16))
                        {
                            case DISUSE_IMMEDIATE_ADDR_16_16:
                                PUT_NUM_16(pParam->uValue >> 16);
                                PUT_C(':');
                                PUT_NUM_16(pParam->uValue);
                                if (pfnGetSymbol)
                                    rc = pfnGetSymbol(pDis, DIS_FMT_SEL_FROM_VALUE(pParam->uValue >> 16), (uint16_t)pParam->uValue, szSymbol, sizeof(szSymbol), &off, pvUser);
                                break;
                            case DISUSE_IMMEDIATE_ADDR_16_32:
                                PUT_NUM_16(pParam->uValue >> 32);
                                PUT_C(':');
                                PUT_NUM_32(pParam->uValue);
                                if (pfnGetSymbol)
                                    rc = pfnGetSymbol(pDis, DIS_FMT_SEL_FROM_VALUE(pParam->uValue >> 16), (uint32_t)pParam->uValue, szSymbol, sizeof(szSymbol), &off, pvUser);
                                break;
                            case DISUSE_DISPLACEMENT16:
                                PUT_NUM_16(pParam->uValue);
                                if (pfnGetSymbol)
                                    rc = pfnGetSymbol(pDis, DIS_FMT_SEL_FROM_REG(DISSELREG_CS), (uint16_t)pParam->uValue, szSymbol, sizeof(szSymbol), &off, pvUser);
                                break;
                            case DISUSE_DISPLACEMENT32:
                                PUT_NUM_32(pParam->uValue);
                                if (pfnGetSymbol)
                                    rc = pfnGetSymbol(pDis, DIS_FMT_SEL_FROM_REG(DISSELREG_CS), (uint32_t)pParam->uValue, szSymbol, sizeof(szSymbol), &off, pvUser);
                                break;
                            case DISUSE_DISPLACEMENT64:
                                PUT_NUM_64(pParam->uValue);
                                if (pfnGetSymbol)
                                    rc = pfnGetSymbol(pDis, DIS_FMT_SEL_FROM_REG(DISSELREG_CS), (uint64_t)pParam->uValue, szSymbol, sizeof(szSymbol), &off, pvUser);
                                break;
                            default:
                                AssertFailed();
                                break;
                        }

                        PUT_SYMBOL_TWO(rc, " [", ']');
                        break;
                    }

                    case 'O': /* No ModRM byte (ParseImmAddr). */
                    {
                        Assert(*pszFmt == 'b' || *pszFmt == 'v'); pszFmt++;
                        PUT_FAR();
                        PUT_SIZE_OVERRIDE();
                        PUT_C('[');
                        PUT_SEGMENT_OVERRIDE();
                        off = 0;
                        int rc = VERR_SYMBOL_NOT_FOUND;
                        switch (pParam->fUse & (DISUSE_IMMEDIATE_ADDR_16_16 | DISUSE_IMMEDIATE_ADDR_16_32 | DISUSE_DISPLACEMENT64 | DISUSE_DISPLACEMENT32 | DISUSE_DISPLACEMENT16))
                        {
                            case DISUSE_IMMEDIATE_ADDR_16_16:
                                PUT_NUM_16(pParam->uValue >> 16);
                                PUT_C(':');
                                PUT_NUM_16(pParam->uValue);
                                if (pfnGetSymbol)
                                    rc = pfnGetSymbol(pDis, DIS_FMT_SEL_FROM_VALUE(pParam->uValue >> 16), (uint16_t)pParam->uValue, szSymbol, sizeof(szSymbol), &off, pvUser);
                                break;
                            case DISUSE_IMMEDIATE_ADDR_16_32:
                                PUT_NUM_16(pParam->uValue >> 32);
                                PUT_C(':');
                                PUT_NUM_32(pParam->uValue);
                                if (pfnGetSymbol)
                                    rc = pfnGetSymbol(pDis, DIS_FMT_SEL_FROM_VALUE(pParam->uValue >> 16), (uint32_t)pParam->uValue, szSymbol, sizeof(szSymbol), &off, pvUser);
                                break;
                            case DISUSE_DISPLACEMENT16:
                                PUT_NUM_16(pParam->uDisp.i16);
                                if (pfnGetSymbol)
                                    rc = pfnGetSymbol(pDis, DIS_FMT_SEL_FROM_REG(DISSELREG_CS), pParam->uDisp.u16, szSymbol, sizeof(szSymbol), &off, pvUser);
                                break;
                            case DISUSE_DISPLACEMENT32:
                                PUT_NUM_32(pParam->uDisp.i32);
                                if (pfnGetSymbol)
                                    rc = pfnGetSymbol(pDis, DIS_FMT_SEL_FROM_REG(DISSELREG_CS), pParam->uDisp.u32, szSymbol, sizeof(szSymbol), &off, pvUser);
                                break;
                            case DISUSE_DISPLACEMENT64:
                                PUT_NUM_64(pParam->uDisp.i64);
                                if (pfnGetSymbol)
                                    rc = pfnGetSymbol(pDis, DIS_FMT_SEL_FROM_REG(DISSELREG_CS), pParam->uDisp.u64, szSymbol, sizeof(szSymbol), &off, pvUser);
                                break;
                            default:
                                AssertFailed();
                                break;
                        }
                        PUT_C(']');

                        PUT_SYMBOL_TWO(rc, " (", ')');
                        break;
                    }

                    case 'X': /* DS:SI (ParseXb, ParseXv). */
                    case 'Y': /* ES:DI (ParseYb, ParseYv). */
                    {
                        Assert(*pszFmt == 'b' || *pszFmt == 'v'); pszFmt++;
                        PUT_FAR();
                        PUT_SIZE_OVERRIDE();
                        PUT_C('[');
                        if (pParam->fUse & DISUSE_POINTER_DS_BASED)
                            PUT_SZ("ds:");
                        else
                            PUT_SZ("es:");

                        size_t cchReg;
                        const char *pszReg = disasmFormatYasmBaseReg(pDis, pParam, &cchReg);
                        PUT_STR(pszReg, cchReg);
                        PUT_C(']');
                        break;
                    }

                    case 'e': /* Register based on operand size (e.g. %eAX, %eAH) (ParseFixedReg). */
                    {
                        Assert(RT_C_IS_ALPHA(pszFmt[0]) && RT_C_IS_ALPHA(pszFmt[1]) && !RT_C_IS_ALPHA(pszFmt[2]));
                        pszFmt += 2;
                        size_t cchReg;
                        const char *pszReg = disasmFormatYasmBaseReg(pDis, pParam, &cchReg);
                        PUT_STR(pszReg, cchReg);
                        break;
                    }

                    default:
                        AssertMsgFailed(("%c%s!\n", ch, pszFmt));
                        break;
                }
                AssertMsg(*pszFmt == ',' || *pszFmt == '\0', ("%c%s\n", ch, pszFmt));
            }
            else
            {
                PUT_C(ch);
                if (ch == ',')
                {
                    Assert(*pszFmt != ' ');
                    PUT_C(' ');
                    switch (++iParam)
                    {
                        case 2: pParam = &pDis->Param2; break;
                        case 3: pParam = &pDis->Param3; break;
                        case 4: pParam = &pDis->Param4; break;
                        default: pParam = NULL; break;
                    }
                }
            }
        } /* while more to format */
    }

    /*
     * Any additional output to the right of the instruction?
     */
    if (fFlags & (DIS_FMT_FLAGS_BYTES_RIGHT | DIS_FMT_FLAGS_ADDR_RIGHT))
    {
        /* some up front padding. */
        size_t cchPadding = cchOutput - offInstruction;
        cchPadding = cchPadding + 1 >= 42 ? 1 : 42 - cchPadding;
        PUT_STR(g_szSpaces, cchPadding);

        /* comment? */
        if (fFlags & (DIS_FMT_FLAGS_BYTES_RIGHT | DIS_FMT_FLAGS_ADDR_RIGHT))
            PUT_SZ(";");

        /*
         * The address?
         */
        if (fFlags & DIS_FMT_FLAGS_ADDR_RIGHT)
        {
            PUT_C(' ');
#if HC_ARCH_BITS == 64 || GC_ARCH_BITS == 64
            if (pDis->uInstrAddr >= _4G)
                PUT_NUM(9, "%08x`", (uint32_t)(pDis->uInstrAddr >> 32));
#endif
            PUT_NUM(8, "%08x", (uint32_t)pDis->uInstrAddr);
        }

        /*
         * Opcode bytes?
         */
        if (fFlags & DIS_FMT_FLAGS_BYTES_RIGHT)
        {
            PUT_C(' ');
            size_t cchTmp = disFormatBytes(pDis, pszDst, cchDst, fFlags);
            cchOutput += cchTmp;
            if (cchTmp >= cchDst)
                cchTmp = cchDst - (cchDst != 0);
            cchDst -= cchTmp;
            pszDst += cchTmp;
        }
    }

    /*
     * Terminate it - on overflow we'll have reserved one byte for this.
     */
    if (cchDst > 0)
        *pszDst = '\0';
    else
        Assert(!cchBuf);

    /* clean up macros */
#undef PUT_PSZ
#undef PUT_SZ
#undef PUT_STR
#undef PUT_C
    return cchOutput;
}


/**
 * Formats the current instruction in Yasm (/ Nasm) style.
 *
 * This is a simplified version of DISFormatYasmEx() provided for your convenience.
 *
 *
 * @returns The number of output characters. If this is >= cchBuf, then the content
 *          of pszBuf will be truncated.
 * @param   pDis    Pointer to the disassembler state.
 * @param   pszBuf  The output buffer.
 * @param   cchBuf  The size of the output buffer.
 */
DISDECL(size_t) DISFormatYasm(PCDISSTATE pDis, char *pszBuf, size_t cchBuf)
{
    return DISFormatYasmEx(pDis, pszBuf, cchBuf, 0 /* fFlags */, NULL /* pfnGetSymbol */, NULL /* pvUser */);
}


/**
 * Checks if the encoding of the given disassembled instruction is something we
 * can never get YASM to produce.
 *
 * @returns true if it's odd, false if it isn't.
 * @param   pDis        The disassembler output.  The byte fetcher callback will
 *                      be used if present as we might need to fetch opcode
 *                      bytes.
 */
DISDECL(bool) DISFormatYasmIsOddEncoding(PDISSTATE pDis)
{
    /*
     * Mod rm + SIB: Check for duplicate EBP encodings that yasm won't use for very good reasons.
     */
    if (    pDis->uAddrMode != DISCPUMODE_16BIT /// @todo correct?
        &&  pDis->ModRM.Bits.Rm == 4
        &&  pDis->ModRM.Bits.Mod != 3)
    {
        /* No scaled index SIB (index=4), except for ESP. */
        if (    pDis->SIB.Bits.Index == 4
            &&  pDis->SIB.Bits.Base != 4)
            return true;

        /* EBP + displacement */
        if (    pDis->ModRM.Bits.Mod != 0
             && pDis->SIB.Bits.Base == 5
             && pDis->SIB.Bits.Scale == 0)
            return true;
    }

    /*
     * Seems to be an instruction alias here, but I cannot find any docs on it... hrmpf!
     */
    if (    pDis->pCurInstr->uOpcode == OP_SHL
        &&  pDis->ModRM.Bits.Reg == 6)
        return true;

    /*
     * Check for multiple prefixes of the same kind.
     */
    uint8_t  off1stSeg = UINT8_MAX;
    uint8_t  offOpSize = UINT8_MAX;
    uint8_t  offAddrSize = UINT8_MAX;
    uint32_t fPrefixes = 0;
    for (uint32_t offOpcode = 0; offOpcode < RT_ELEMENTS(pDis->abInstr); offOpcode++)
    {
        uint32_t f;
        switch (pDis->abInstr[offOpcode])
        {
            case 0xf0:
                f = DISPREFIX_LOCK;
                break;

            case 0xf2:
            case 0xf3:
                f = DISPREFIX_REP; /* yes, both */
                break;

            case 0x2e:
            case 0x3e:
            case 0x26:
            case 0x36:
            case 0x64:
            case 0x65:
                if (off1stSeg == UINT8_MAX)
                    off1stSeg = offOpcode;
                f = DISPREFIX_SEG;
                break;

            case 0x66:
                if (offOpSize == UINT8_MAX)
                    offOpSize = offOpcode;
                f = DISPREFIX_OPSIZE;
                break;

            case 0x67:
                if (offAddrSize == UINT8_MAX)
                    offAddrSize = offOpcode;
                f = DISPREFIX_ADDRSIZE;
                break;

            case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
            case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
                f = pDis->uCpuMode == DISCPUMODE_64BIT ? DISPREFIX_REX : 0;
                break;

            default:
                f = 0;
                break;
        }
        if (!f)
            break; /* done */
        if (fPrefixes & f)
            return true;
        fPrefixes |= f;
    }

    /* segment overrides are fun */
    if (fPrefixes & DISPREFIX_SEG)
    {
        /* no effective address which it may apply to. */
        Assert((pDis->fPrefix & DISPREFIX_SEG) || pDis->uCpuMode == DISCPUMODE_64BIT);
        if (    !DISUSE_IS_EFFECTIVE_ADDR(pDis->Param1.fUse)
            &&  !DISUSE_IS_EFFECTIVE_ADDR(pDis->Param2.fUse)
            &&  !DISUSE_IS_EFFECTIVE_ADDR(pDis->Param3.fUse))
            return true;

        /* Yasm puts the segment prefixes before the operand prefix with no
           way of overriding it. */
        if (offOpSize < off1stSeg)
            return true;
    }

    /* fixed register + addr override doesn't go down all that well. */
    if (fPrefixes & DISPREFIX_ADDRSIZE)
    {
        Assert(pDis->fPrefix & DISPREFIX_ADDRSIZE);
        if (    pDis->pCurInstr->fParam3 == OP_PARM_NONE
            &&  pDis->pCurInstr->fParam2 == OP_PARM_NONE
            &&  (   pDis->pCurInstr->fParam1 >= OP_PARM_REG_GEN32_START
                 && pDis->pCurInstr->fParam1 <= OP_PARM_REG_GEN32_END))
            return true;
    }

    /* Almost all prefixes are bad for jumps. */
    if (fPrefixes)
    {
        switch (pDis->pCurInstr->uOpcode)
        {
            /* nop w/ prefix(es). */
            case OP_NOP:
                return true;

            case OP_JMP:
                if (    pDis->pCurInstr->fParam1 != OP_PARM_Jb
                    &&  pDis->pCurInstr->fParam1 != OP_PARM_Jv)
                    break;
                RT_FALL_THRU();
            case OP_JO:
            case OP_JNO:
            case OP_JC:
            case OP_JNC:
            case OP_JE:
            case OP_JNE:
            case OP_JBE:
            case OP_JNBE:
            case OP_JS:
            case OP_JNS:
            case OP_JP:
            case OP_JNP:
            case OP_JL:
            case OP_JNL:
            case OP_JLE:
            case OP_JNLE:
                /** @todo branch hinting 0x2e/0x3e... */
                return true;
        }

    }

    /* All but the segment prefix is bad news for push/pop. */
    if (fPrefixes & ~DISPREFIX_SEG)
    {
        switch (pDis->pCurInstr->uOpcode)
        {
            case OP_POP:
            case OP_PUSH:
                if (    pDis->pCurInstr->fParam1 >= OP_PARM_REG_SEG_START
                    &&  pDis->pCurInstr->fParam1 <= OP_PARM_REG_SEG_END)
                    return true;
                if (    (fPrefixes & ~DISPREFIX_OPSIZE)
                    &&  pDis->pCurInstr->fParam1 >= OP_PARM_REG_GEN32_START
                    &&  pDis->pCurInstr->fParam1 <= OP_PARM_REG_GEN32_END)
                    return true;
                break;

            case OP_POPA:
            case OP_POPF:
            case OP_PUSHA:
            case OP_PUSHF:
                if (fPrefixes & ~DISPREFIX_OPSIZE)
                    return true;
                break;
        }
    }

    /* Implicit 8-bit register instructions doesn't mix with operand size. */
    if (    (fPrefixes & DISPREFIX_OPSIZE)
        &&  (   (   pDis->pCurInstr->fParam1 == OP_PARM_Gb /* r8 */
                 && pDis->pCurInstr->fParam2 == OP_PARM_Eb /* r8/mem8 */)
             || (   pDis->pCurInstr->fParam2 == OP_PARM_Gb /* r8 */
                 && pDis->pCurInstr->fParam1 == OP_PARM_Eb /* r8/mem8 */))
       )
    {
        switch (pDis->pCurInstr->uOpcode)
        {
            case OP_ADD:
            case OP_OR:
            case OP_ADC:
            case OP_SBB:
            case OP_AND:
            case OP_SUB:
            case OP_XOR:
            case OP_CMP:
                return true;
            default:
                break;
        }
    }

    /* Instructions taking no address or operand which thus may be annoyingly
       difficult to format for yasm. */
    if (fPrefixes)
    {
        switch (pDis->pCurInstr->uOpcode)
        {
            case OP_STI:
            case OP_STC:
            case OP_CLI:
            case OP_CLD:
            case OP_CLC:
            case OP_INT:
            case OP_INT3:
            case OP_INTO:
            case OP_HLT:
            /** @todo Many more to can be added here. */
                return true;
            default:
                break;
        }
    }

    /* FPU and other instructions that ignores operand size override. */
    if (fPrefixes & DISPREFIX_OPSIZE)
    {
        switch (pDis->pCurInstr->uOpcode)
        {
            /* FPU: */
            case OP_FIADD:
            case OP_FIMUL:
            case OP_FISUB:
            case OP_FISUBR:
            case OP_FIDIV:
            case OP_FIDIVR:
            /** @todo there are many more. */
                return true;

            case OP_MOV:
                /** @todo could be that we're not disassembling these correctly. */
                if (pDis->pCurInstr->fParam1 == OP_PARM_Sw)
                    return true;
                /** @todo what about the other way? */
                break;

            default:
                break;
        }
    }


    /*
     * Check for the version of xyz reg,reg instruction that the assembler doesn't use.
     *
     * For example:
     *    expected: 1aee   sbb ch, dh     ; SBB r8, r/m8
     *        yasm: 18F5   sbb ch, dh     ; SBB r/m8, r8
     */
    if (pDis->ModRM.Bits.Mod == 3 /* reg,reg */)
    {
        switch (pDis->pCurInstr->uOpcode)
        {
            case OP_ADD:
            case OP_OR:
            case OP_ADC:
            case OP_SBB:
            case OP_AND:
            case OP_SUB:
            case OP_XOR:
            case OP_CMP:
                if (    (    pDis->pCurInstr->fParam1 == OP_PARM_Gb /* r8 */
                         && pDis->pCurInstr->fParam2 == OP_PARM_Eb /* r8/mem8 */)
                    ||  (    pDis->pCurInstr->fParam1 == OP_PARM_Gv /* rX */
                         && pDis->pCurInstr->fParam2 == OP_PARM_Ev /* rX/memX */))
                    return true;

                /* 82 (see table A-6). */
                if (pDis->bOpCode == 0x82)
                    return true;
                break;

            /* ff /0, fe /0, ff /1, fe /0 */
            case OP_DEC:
            case OP_INC:
                return true;

            case OP_POP:
            case OP_PUSH:
                Assert(pDis->bOpCode == 0x8f);
                return true;

            case OP_MOV:
                if (   pDis->bOpCode == 0x8a
                    || pDis->bOpCode == 0x8b)
                    return true;
                break;

            default:
                break;
        }
    }

    /* shl eax,1 will be assembled to the form without the immediate byte. */
    if (    pDis->pCurInstr->fParam2 == OP_PARM_Ib
        &&  (uint8_t)pDis->Param2.uValue == 1)
    {
        switch (pDis->pCurInstr->uOpcode)
        {
            case OP_SHL:
            case OP_SHR:
            case OP_SAR:
            case OP_RCL:
            case OP_RCR:
            case OP_ROL:
            case OP_ROR:
                return true;
        }
    }

    /* And some more - see table A-6. */
    if (pDis->bOpCode == 0x82)
    {
        switch (pDis->pCurInstr->uOpcode)
        {
            case OP_ADD:
            case OP_OR:
            case OP_ADC:
            case OP_SBB:
            case OP_AND:
            case OP_SUB:
            case OP_XOR:
            case OP_CMP:
                return true;
                break;
        }
    }


    /* check for REX.X = 1 without SIB. */

    /* Yasm encodes setnbe al with /2 instead of /0 like the AMD manual
       says (intel doesn't appear to care). */
    switch (pDis->pCurInstr->uOpcode)
    {
        case OP_SETO:
        case OP_SETNO:
        case OP_SETC:
        case OP_SETNC:
        case OP_SETE:
        case OP_SETNE:
        case OP_SETBE:
        case OP_SETNBE:
        case OP_SETS:
        case OP_SETNS:
        case OP_SETP:
        case OP_SETNP:
        case OP_SETL:
        case OP_SETNL:
        case OP_SETLE:
        case OP_SETNLE:
            AssertMsg(pDis->bOpCode >= 0x90 && pDis->bOpCode <= 0x9f, ("%#x\n", pDis->bOpCode));
            if (pDis->ModRM.Bits.Reg != 2)
                return true;
            break;
    }

    /*
     * The MOVZX reg32,mem16 instruction without an operand size prefix
     * doesn't quite make sense...
     */
    if (    pDis->pCurInstr->uOpcode == OP_MOVZX
        &&  pDis->bOpCode == 0xB7
        &&  (pDis->uCpuMode == DISCPUMODE_16BIT) != !!(fPrefixes & DISPREFIX_OPSIZE))
        return true;

    /*
     * YASM doesn't do ICEBP/INT1/INT01, unlike NASM.
     */
    if (pDis->bOpCode == 0xF1)
        return true;

    return false;
}

