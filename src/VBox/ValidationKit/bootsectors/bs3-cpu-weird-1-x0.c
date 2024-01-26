/* $Id: bs3-cpu-weird-1-x0.c $ */
/** @file
 * BS3Kit - bs3-cpu-weird-2, C test driver code (16-bit).
 */

/*
 * Copyright (C) 2007-2023 Oracle and/or its affiliates.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define BS3_USE_X0_TEXT_SEG
#include <bs3kit.h>
#include <bs3-cmn-memory.h>
#include <iprt/asm.h>
#include <iprt/asm-amd64-x86.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#undef  CHECK_MEMBER
#define CHECK_MEMBER(a_szName, a_szFmt, a_Actual, a_Expected) \
    do \
    { \
        if ((a_Actual) == (a_Expected)) { /* likely */ } \
        else bs3CpuWeird1_FailedF(a_szName "=" a_szFmt " expected " a_szFmt, (a_Actual), (a_Expected)); \
    } while (0)


/*********************************************************************************************************************************
*   External Symbols                                                                                                             *
*********************************************************************************************************************************/
extern FNBS3FAR     bs3CpuWeird1_InhibitedInt80_c16;
extern FNBS3FAR     bs3CpuWeird1_InhibitedInt80_c32;
extern FNBS3FAR     bs3CpuWeird1_InhibitedInt80_c64;
extern FNBS3FAR     bs3CpuWeird1_InhibitedInt80_int80_c16;
extern FNBS3FAR     bs3CpuWeird1_InhibitedInt80_int80_c32;
extern FNBS3FAR     bs3CpuWeird1_InhibitedInt80_int80_c64;

extern FNBS3FAR     bs3CpuWeird1_InhibitedInt3_c16;
extern FNBS3FAR     bs3CpuWeird1_InhibitedInt3_c32;
extern FNBS3FAR     bs3CpuWeird1_InhibitedInt3_c64;
extern FNBS3FAR     bs3CpuWeird1_InhibitedInt3_int3_c16;
extern FNBS3FAR     bs3CpuWeird1_InhibitedInt3_int3_c32;
extern FNBS3FAR     bs3CpuWeird1_InhibitedInt3_int3_c64;

extern FNBS3FAR     bs3CpuWeird1_InhibitedBp_c16;
extern FNBS3FAR     bs3CpuWeird1_InhibitedBp_c32;
extern FNBS3FAR     bs3CpuWeird1_InhibitedBp_c64;
extern FNBS3FAR     bs3CpuWeird1_InhibitedBp_int3_c16;
extern FNBS3FAR     bs3CpuWeird1_InhibitedBp_int3_c32;
extern FNBS3FAR     bs3CpuWeird1_InhibitedBp_int3_c64;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static const char BS3_FAR  *g_pszTestMode = (const char *)1;
static BS3CPUVENDOR         g_enmCpuVendor = BS3CPUVENDOR_INTEL;
static bool                 g_fVME = false;
//static uint8_t              g_bTestMode = 1;
//static bool                 g_f16BitSys = 1;



/**
 * Sets globals according to the mode.
 *
 * @param   bTestMode   The test mode.
 */
static void bs3CpuWeird1_SetGlobals(uint8_t bTestMode)
{
//    g_bTestMode     = bTestMode;
    g_pszTestMode   = Bs3GetModeName(bTestMode);
//    g_f16BitSys     = BS3_MODE_IS_16BIT_SYS(bTestMode);
    g_usBs3TestStep = 0;
    g_enmCpuVendor  = Bs3GetCpuVendor();
    g_fVME = (g_uBs3CpuDetected & BS3CPU_TYPE_MASK) >= BS3CPU_80486
          && (Bs3RegGetCr4() & X86_CR4_VME);
}


/**
 * Wrapper around Bs3TestFailedF that prefixes the error with g_usBs3TestStep
 * and g_pszTestMode.
 */
static void bs3CpuWeird1_FailedF(const char *pszFormat, ...)
{
    va_list va;

    char szTmp[168];
    va_start(va, pszFormat);
    Bs3StrPrintfV(szTmp, sizeof(szTmp), pszFormat, va);
    va_end(va);

    Bs3TestFailedF("%u - %s: %s", g_usBs3TestStep, g_pszTestMode, szTmp);
}


/**
 * Compares interrupt stuff.
 */
static void bs3CpuWeird1_CompareDbgInhibitRingXfer(PCBS3TRAPFRAME pTrapCtx, PCBS3REGCTX pStartCtx, uint8_t bXcpt,
                                                   int8_t cbPcAdjust, int8_t cbSpAdjust, uint32_t uDr6Expected,
                                                   uint8_t cbIretFrame, uint64_t uHandlerRsp)
{
    uint32_t uDr6 = (g_uBs3CpuDetected & BS3CPU_TYPE_MASK) >= BS3CPU_80386 ? Bs3RegGetDr6() : X86_DR6_INIT_VAL;
    uint16_t const cErrorsBefore = Bs3TestSubErrorCount();
    CHECK_MEMBER("bXcpt",       "%#04x",    pTrapCtx->bXcpt,        bXcpt);
    CHECK_MEMBER("bErrCd",      "%#06RX64", pTrapCtx->uErrCd,       0);
    CHECK_MEMBER("cbIretFrame", "%#04x",    pTrapCtx->cbIretFrame,  cbIretFrame);
    CHECK_MEMBER("uHandlerRsp",  "%#06RX64", pTrapCtx->uHandlerRsp,  uHandlerRsp);
    if (uDr6 != uDr6Expected)
        bs3CpuWeird1_FailedF("dr6=%#010RX32 expected %#010RX32", uDr6, uDr6Expected);
    Bs3TestCheckRegCtxEx(&pTrapCtx->Ctx, pStartCtx, cbPcAdjust, cbSpAdjust, 0 /*fExtraEfl*/, g_pszTestMode, g_usBs3TestStep);
    if (Bs3TestSubErrorCount() != cErrorsBefore)
    {
        Bs3TrapPrintFrame(pTrapCtx);
        Bs3TestPrintf("DR6=%#RX32; Handler: CS=%04RX16 SS:ESP=%04RX16:%08RX64 EFL=%RX64 cbIret=%#x\n",
                      uDr6, pTrapCtx->uHandlerCs, pTrapCtx->uHandlerSs, pTrapCtx->uHandlerRsp,
                      pTrapCtx->fHandlerRfl, pTrapCtx->cbIretFrame);
#if 0
        Bs3TestPrintf("Halting in CompareIntCtx: bXcpt=%#x\n", bXcpt);
        ASMHalt();
#endif
    }
}

static uint64_t bs3CpuWeird1_GetTrapHandlerEIP(uint8_t bXcpt, uint8_t bMode, bool fV86)
{
    if (   BS3_MODE_IS_RM_SYS(bMode)
        || (fV86 && BS3_MODE_IS_V86(bMode)))
    {
        PRTFAR16 paIvt = (PRTFAR16)Bs3XptrFlatToCurrent(0);
        return paIvt[bXcpt].off;
    }
    if (BS3_MODE_IS_16BIT_SYS(bMode))
        return Bs3Idt16[bXcpt].Gate.u16OffsetLow;
    if (BS3_MODE_IS_32BIT_SYS(bMode))
        return RT_MAKE_U32(Bs3Idt32[bXcpt].Gate.u16OffsetLow, Bs3Idt32[bXcpt].Gate.u16OffsetHigh);
    return RT_MAKE_U64(RT_MAKE_U32(Bs3Idt64[bXcpt].Gate.u16OffsetLow, Bs3Idt32[bXcpt].Gate.u16OffsetHigh),
                       Bs3Idt64[bXcpt].Gate.u32OffsetTop);
}


static int bs3CpuWeird1_DbgInhibitRingXfer_Worker(uint8_t bTestMode, uint8_t bIntGate, uint8_t cbRingInstr, int8_t cbSpAdjust,
                                                  FPFNBS3FAR pfnTestCode, FPFNBS3FAR pfnTestLabel)
{
    BS3TRAPFRAME            TrapCtx;
    BS3TRAPFRAME            TrapExpect;
    BS3REGCTX               Ctx;
    uint8_t                 bSavedDpl;
    uint8_t const           offTestLabel    = BS3_FP_OFF(pfnTestLabel) - BS3_FP_OFF(pfnTestCode);
    //uint8_t const           cbIretFrameSame = BS3_MODE_IS_RM_SYS(bTestMode)    ? 6
    //                                        : BS3_MODE_IS_16BIT_SYS(bTestMode) ? 12
    //                                        : BS3_MODE_IS_64BIT_SYS(bTestMode) ? 40 : 12;
    uint8_t                 cbIretFrameInt;
    uint8_t                 cbIretFrameIntDb;
    uint8_t const           cbIretFrameSame = BS3_MODE_IS_16BIT_SYS(bTestMode) ? 6
                                            : BS3_MODE_IS_32BIT_SYS(bTestMode) ? 12 : 40;
    uint8_t const           cbSpAdjSame     = BS3_MODE_IS_64BIT_SYS(bTestMode) ? 48 : cbIretFrameSame;
    uint8_t                 bVmeMethod = 0;
    uint64_t                uHandlerRspInt;
    uint64_t                uHandlerRspIntDb;
    BS3_XPTR_AUTO(uint32_t, StackXptr);

    /* make sure they're allocated  */
    Bs3MemZero(&Ctx, sizeof(Ctx));
    Bs3MemZero(&TrapCtx, sizeof(TrapCtx));
    Bs3MemZero(&TrapExpect, sizeof(TrapExpect));

    /*
     * Make INT xx accessible from DPL 3 and create a ring-3 context that we can work with.
     */
    bSavedDpl = Bs3TrapSetDpl(bIntGate, 3);

    Bs3RegCtxSaveEx(&Ctx, bTestMode, 1024);
    Bs3RegCtxSetRipCsFromLnkPtr(&Ctx, pfnTestCode);
    if (BS3_MODE_IS_16BIT_SYS(bTestMode))
        g_uBs3TrapEipHint = Ctx.rip.u32;
    Ctx.rflags.u32 &= ~X86_EFL_RF;

    /* Raw-mode enablers. */
    Ctx.rflags.u32 |= X86_EFL_IF;
    if ((g_uBs3CpuDetected & BS3CPU_TYPE_MASK) >= BS3CPU_80486)
        Ctx.cr0.u32 |= X86_CR0_WP;

    /* We put the SS value on the stack so we can easily set breakpoints there. */
    Ctx.rsp.u32 -= 8;
    BS3_XPTR_SET_FLAT(uint32_t, StackXptr, Ctx.rsp.u32); /* ASSUMES SS.BASE == 0!! */

    /* ring-3 */
    if (!BS3_MODE_IS_RM_OR_V86(bTestMode))
        Bs3RegCtxConvertToRingX(&Ctx, 3);

    /* V8086: Set IOPL to 3. */
    if (BS3_MODE_IS_V86(bTestMode))
    {
        Ctx.rflags.u32 |= X86_EFL_IOPL;
        if (g_fVME)
        {
            Bs3RegSetTr(BS3_SEL_TSS32_IRB);
#if 0
            /* SDMv3b, 20.3.3 method 5: */
            ASMBitClear(&Bs3SharedIntRedirBm, bIntGate);
            bVmeMethod = 5;
#else
            /* SDMv3b, 20.3.3 method 4 (similar to non-VME): */
            ASMBitSet(&Bs3SharedIntRedirBm, bIntGate);
            bVmeMethod = 4;
        }
#endif
    }

    /*
     * Test #0: Test run.  Calc expected delayed #DB from it.
     */
    if ((g_uBs3CpuDetected & BS3CPU_TYPE_MASK) >= BS3CPU_80386)
    {
        Bs3RegSetDr7(0);
        Bs3RegSetDr6(X86_DR6_INIT_VAL);
    }
    *BS3_XPTR_GET(uint32_t, StackXptr) = Ctx.ss;
    Bs3TrapSetJmpAndRestore(&Ctx, &TrapExpect);
    if (TrapExpect.bXcpt != bIntGate)
    {

        Bs3TestFailedF("%u: bXcpt is %#x, expected %#x!\n", g_usBs3TestStep, TrapExpect.bXcpt, bIntGate);
        Bs3TrapPrintFrame(&TrapExpect);
        return 1;
    }

    cbIretFrameInt   = TrapExpect.cbIretFrame;
    cbIretFrameIntDb = cbIretFrameInt + cbIretFrameSame;
    uHandlerRspInt   = TrapExpect.uHandlerRsp;
    uHandlerRspIntDb = uHandlerRspInt - cbSpAdjSame;

    TrapExpect.Ctx.bCpl         = 0;
    TrapExpect.Ctx.cs           = TrapExpect.uHandlerCs;
    TrapExpect.Ctx.ss           = TrapExpect.uHandlerSs;
    TrapExpect.Ctx.rsp.u64      = TrapExpect.uHandlerRsp;
    TrapExpect.Ctx.rflags.u64   = TrapExpect.fHandlerRfl;
    if (BS3_MODE_IS_V86(bTestMode))
    {
        if (bVmeMethod >= 5)
        {
            TrapExpect.Ctx.rflags.u32 |= X86_EFL_VM;
            TrapExpect.Ctx.bCpl = 3;
            TrapExpect.Ctx.rip.u64 = bs3CpuWeird1_GetTrapHandlerEIP(bIntGate, bTestMode, true);
            cbIretFrameIntDb = 36;
            if (BS3_MODE_IS_16BIT_SYS(bTestMode))
                uHandlerRspIntDb = Bs3Tss16.sp0  - cbIretFrameIntDb;
            else
                uHandlerRspIntDb = Bs3Tss32.esp0 - cbIretFrameIntDb;
        }
        else
        {
            TrapExpect.Ctx.ds   = 0;
            TrapExpect.Ctx.es   = 0;
            TrapExpect.Ctx.fs   = 0;
            TrapExpect.Ctx.gs   = 0;
        }
    }

    /*
     * Test #1: Single stepping ring-3.  Ignored except for V8086 w/ VME.
     */
    g_usBs3TestStep++;
    if ((g_uBs3CpuDetected & BS3CPU_TYPE_MASK) >= BS3CPU_80386)
    {
        Bs3RegSetDr7(0);
        Bs3RegSetDr6(X86_DR6_INIT_VAL);
    }
    *BS3_XPTR_GET(uint32_t, StackXptr) = Ctx.ss;
    Ctx.rflags.u32 |= X86_EFL_TF;

    Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
    if (   !BS3_MODE_IS_V86(bTestMode)
        || bVmeMethod < 5)
        bs3CpuWeird1_CompareDbgInhibitRingXfer(&TrapCtx, &Ctx, bIntGate, offTestLabel + cbRingInstr, cbSpAdjust,
                                               X86_DR6_INIT_VAL, cbIretFrameInt, uHandlerRspInt);
    else
    {
        TrapExpect.Ctx.rflags.u32 |= X86_EFL_TF;
        bs3CpuWeird1_CompareDbgInhibitRingXfer(&TrapCtx, &TrapExpect.Ctx, X86_XCPT_DB, offTestLabel, -2,
                                               X86_DR6_INIT_VAL | X86_DR6_BS, cbIretFrameIntDb, uHandlerRspIntDb);
        TrapExpect.Ctx.rflags.u32 &= ~X86_EFL_TF;
    }

    Ctx.rflags.u32 &= ~X86_EFL_TF;
    if ((g_uBs3CpuDetected & BS3CPU_TYPE_MASK) >= BS3CPU_80386)
    {
        uint32_t uDr6Expect;

        /*
         * Test #2: Execution breakpoint on ring transition instruction.
         *          This hits on AMD-V (threadripper) but not on VT-x (skylake).
         */
        g_usBs3TestStep++;
        Bs3RegSetDr0(Bs3SelRealModeCodeToFlat(pfnTestLabel));
        Bs3RegSetDr7(X86_DR7_L0 | X86_DR7_G0 | X86_DR7_RW(0, X86_DR7_RW_EO) | X86_DR7_LEN(0, X86_DR7_LEN_BYTE));
        Bs3RegSetDr6(X86_DR6_INIT_VAL);
        *BS3_XPTR_GET(uint32_t, StackXptr) = Ctx.ss;

        Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
        Bs3RegSetDr7(0);
        if (g_enmCpuVendor == BS3CPUVENDOR_AMD || g_enmCpuVendor == BS3CPUVENDOR_HYGON)
            bs3CpuWeird1_CompareDbgInhibitRingXfer(&TrapCtx, &Ctx, X86_XCPT_DB, offTestLabel, cbSpAdjust,
                                                   X86_DR6_INIT_VAL | X86_DR6_B0, cbIretFrameInt, uHandlerRspInt);
        else
            bs3CpuWeird1_CompareDbgInhibitRingXfer(&TrapCtx, &Ctx, bIntGate, offTestLabel + cbRingInstr, cbSpAdjust,
                                                   X86_DR6_INIT_VAL, cbIretFrameInt, uHandlerRspInt);

        /*
         * Test #3: Same as above, but with the LE and GE flags set.
         */
        g_usBs3TestStep++;
        Bs3RegSetDr0(Bs3SelRealModeCodeToFlat(pfnTestLabel));
        Bs3RegSetDr7(X86_DR7_L0 | X86_DR7_G0 | X86_DR7_RW(0, X86_DR7_RW_EO) | X86_DR7_LEN(0, X86_DR7_LEN_BYTE) | X86_DR7_LE | X86_DR7_GE);
        Bs3RegSetDr6(X86_DR6_INIT_VAL);
        *BS3_XPTR_GET(uint32_t, StackXptr) = Ctx.ss;

        Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
        if (g_enmCpuVendor == BS3CPUVENDOR_AMD || g_enmCpuVendor ==  BS3CPUVENDOR_HYGON)
            bs3CpuWeird1_CompareDbgInhibitRingXfer(&TrapCtx, &Ctx, X86_XCPT_DB, offTestLabel, cbSpAdjust,
                                                   X86_DR6_INIT_VAL | X86_DR6_B0, cbIretFrameInt, uHandlerRspInt);
        else
            bs3CpuWeird1_CompareDbgInhibitRingXfer(&TrapCtx, &Ctx, bIntGate, offTestLabel + cbRingInstr, cbSpAdjust,
                                                   X86_DR6_INIT_VAL, cbIretFrameInt, uHandlerRspInt);

        /*
         * Test #4: Execution breakpoint on pop ss / mov ss.  Hits.
         * Note! In real mode AMD-V updates the stack pointer, or something else is busted. Totally weird!
         */
        g_usBs3TestStep++;
        Bs3RegSetDr0(Bs3SelRealModeCodeToFlat(pfnTestCode));
        Bs3RegSetDr7(X86_DR7_L0 | X86_DR7_G0 | X86_DR7_RW(0, X86_DR7_RW_EO) | X86_DR7_LEN(0, X86_DR7_LEN_BYTE));
        Bs3RegSetDr6(X86_DR6_INIT_VAL);
        *BS3_XPTR_GET(uint32_t, StackXptr) = Ctx.ss;

        Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
        bs3CpuWeird1_CompareDbgInhibitRingXfer(&TrapCtx, &Ctx, X86_XCPT_DB, 0, 0, X86_DR6_INIT_VAL | X86_DR6_B0,
                                               cbIretFrameInt,
                                               uHandlerRspInt - (BS3_MODE_IS_RM_SYS(bTestMode) ? 2 : 0) );

        /*
         * Test #5: Same as above, but with the LE and GE flags set.
         */
        g_usBs3TestStep++;
        Bs3RegSetDr0(Bs3SelRealModeCodeToFlat(pfnTestCode));
        Bs3RegSetDr7(X86_DR7_L0 | X86_DR7_G0 | X86_DR7_RW(0, X86_DR7_RW_EO) | X86_DR7_LEN(0, X86_DR7_LEN_BYTE) | X86_DR7_LE | X86_DR7_GE);
        Bs3RegSetDr6(X86_DR6_INIT_VAL);
        *BS3_XPTR_GET(uint32_t, StackXptr) = Ctx.ss;

        Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
        bs3CpuWeird1_CompareDbgInhibitRingXfer(&TrapCtx, &Ctx, X86_XCPT_DB, 0, 0, X86_DR6_INIT_VAL | X86_DR6_B0,
                                               cbIretFrameInt,
                                               uHandlerRspInt - (BS3_MODE_IS_RM_SYS(bTestMode) ? 2 : 0) );

        /*
         * Test #6: Data breakpoint on SS load.  The #DB is delivered after ring transition.  Weird!
         *
         * Note! Intel loses the B0 status, probably for reasons similar to Pentium Pro errata 3.  Similar
         *       erratum is seen with virtually every march since, e.g. skylake SKL009 & SKL111.
         *       Weirdly enougth, they seem to get this right in real mode.  Go figure.
         */
        g_usBs3TestStep++;
        *BS3_XPTR_GET(uint32_t, StackXptr) = Ctx.ss;
        Bs3RegSetDr0(BS3_XPTR_GET_FLAT(uint32_t, StackXptr));
        Bs3RegSetDr7(X86_DR7_L0 | X86_DR7_G0 | X86_DR7_RW(0, X86_DR7_RW_RW) | X86_DR7_LEN(0, X86_DR7_LEN_WORD));
        Bs3RegSetDr6(X86_DR6_INIT_VAL);

        Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
        TrapExpect.Ctx.rip = TrapCtx.Ctx.rip; /// @todo fixme
        Bs3RegSetDr7(0);
        uDr6Expect = X86_DR6_INIT_VAL | X86_DR6_B0;
        if (g_enmCpuVendor == BS3CPUVENDOR_INTEL && bTestMode != BS3_MODE_RM)
            uDr6Expect = X86_DR6_INIT_VAL;
        bs3CpuWeird1_CompareDbgInhibitRingXfer(&TrapCtx, &TrapExpect.Ctx, X86_XCPT_DB, 0, 0, uDr6Expect,
                                               cbIretFrameSame, uHandlerRspIntDb);

        /*
         * Test #7: Same as above, but with the LE and GE flags set.
         */
        g_usBs3TestStep++;
        *BS3_XPTR_GET(uint32_t, StackXptr) = Ctx.ss;
        Bs3RegSetDr0(BS3_XPTR_GET_FLAT(uint32_t, StackXptr));
        Bs3RegSetDr7(X86_DR7_L0 | X86_DR7_G0 | X86_DR7_RW(0, X86_DR7_RW_RW) | X86_DR7_LEN(0, X86_DR7_LEN_WORD) | X86_DR7_LE | X86_DR7_GE);
        Bs3RegSetDr6(X86_DR6_INIT_VAL);

        Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
        TrapExpect.Ctx.rip = TrapCtx.Ctx.rip; /// @todo fixme
        Bs3RegSetDr7(0);
        uDr6Expect = X86_DR6_INIT_VAL | X86_DR6_B0;
        if (g_enmCpuVendor == BS3CPUVENDOR_INTEL && bTestMode != BS3_MODE_RM)
            uDr6Expect = X86_DR6_INIT_VAL;
        bs3CpuWeird1_CompareDbgInhibitRingXfer(&TrapCtx, &TrapExpect.Ctx, X86_XCPT_DB, 0, 0, uDr6Expect,
                                               cbIretFrameSame, uHandlerRspIntDb);

        if (!BS3_MODE_IS_RM_OR_V86(bTestMode))
        {
            /*
             * Test #8: Data breakpoint on SS GDT entry.  Half weird!
             * Note! Intel loses the B1 status, see test #6.
             */
            g_usBs3TestStep++;
            *BS3_XPTR_GET(uint32_t, StackXptr) = (Ctx.ss & X86_SEL_RPL) | BS3_SEL_SPARE_00;
            Bs3GdteSpare00 = Bs3Gdt[Ctx.ss / sizeof(Bs3Gdt[0])];

            Bs3RegSetDr1(Bs3SelPtrToFlat(&Bs3GdteSpare00));
            Bs3RegSetDr7(X86_DR7_L1 | X86_DR7_G1 | X86_DR7_RW(1, X86_DR7_RW_RW) | X86_DR7_LEN(1, X86_DR7_LEN_DWORD));
            Bs3RegSetDr6(X86_DR6_INIT_VAL);

            Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
            TrapExpect.Ctx.rip = TrapCtx.Ctx.rip; /// @todo fixme
            Bs3RegSetDr7(0);
            uDr6Expect = g_enmCpuVendor == BS3CPUVENDOR_INTEL ? X86_DR6_INIT_VAL : X86_DR6_INIT_VAL | X86_DR6_B1;
            bs3CpuWeird1_CompareDbgInhibitRingXfer(&TrapCtx, &TrapExpect.Ctx, X86_XCPT_DB, 0, 0, uDr6Expect,
                                                   cbIretFrameSame, uHandlerRspIntDb);

            /*
             * Test #9: Same as above, but with the LE and GE flags set.
             */
            g_usBs3TestStep++;
            *BS3_XPTR_GET(uint32_t, StackXptr) = (Ctx.ss & X86_SEL_RPL) | BS3_SEL_SPARE_00;
            Bs3GdteSpare00 = Bs3Gdt[Ctx.ss / sizeof(Bs3Gdt[0])];

            Bs3RegSetDr1(Bs3SelPtrToFlat(&Bs3GdteSpare00));
            Bs3RegSetDr7(X86_DR7_L1 | X86_DR7_G1 | X86_DR7_RW(1, X86_DR7_RW_RW) | X86_DR7_LEN(1, X86_DR7_LEN_DWORD) | X86_DR7_LE | X86_DR7_GE);
            Bs3RegSetDr6(X86_DR6_INIT_VAL);

            Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
            TrapExpect.Ctx.rip = TrapCtx.Ctx.rip; /// @todo fixme
            Bs3RegSetDr7(0);
            uDr6Expect = g_enmCpuVendor == BS3CPUVENDOR_INTEL ? X86_DR6_INIT_VAL : X86_DR6_INIT_VAL | X86_DR6_B1;
            bs3CpuWeird1_CompareDbgInhibitRingXfer(&TrapCtx, &TrapExpect.Ctx, X86_XCPT_DB, 0, 0, uDr6Expect,
                                                   cbIretFrameSame, uHandlerRspIntDb);
        }

        /*
         * Cleanup.
         */
        Bs3RegSetDr0(0);
        Bs3RegSetDr1(0);
        Bs3RegSetDr2(0);
        Bs3RegSetDr3(0);
        Bs3RegSetDr6(X86_DR6_INIT_VAL);
        Bs3RegSetDr7(0);
    }
    Bs3TrapSetDpl(bIntGate, bSavedDpl);
    return 0;
}


BS3_DECL_FAR(uint8_t) BS3_CMN_FAR_NM(bs3CpuWeird1_DbgInhibitRingXfer)(uint8_t bMode)
{
    if (BS3_MODE_IS_V86(bMode))
        switch (bMode)
        {
            /** @todo some busted stack stuff with the 16-bit guys.  Also, if VME is
             *        enabled, we're probably not able to do any sensible testing. */
            case BS3_MODE_PP16_V86:
            case BS3_MODE_PE16_V86:
            case BS3_MODE_PAE16_V86:
                return BS3TESTDOMODE_SKIPPED;
        }
    //if (bMode != BS3_MODE_PE16_V86) return BS3TESTDOMODE_SKIPPED;
    //if (bMode != BS3_MODE_PAEV86) return BS3TESTDOMODE_SKIPPED;

    bs3CpuWeird1_SetGlobals(bMode);

    /** @todo test sysenter and syscall too. */
    /** @todo test INTO. */
    /** @todo test all V8086 software INT delivery modes (currently only 4 and 1). */

    /* Note! Both ICEBP and BOUND has be checked cursorily and found not to be affected. */
    if (BS3_MODE_IS_16BIT_CODE(bMode))
    {
        bs3CpuWeird1_DbgInhibitRingXfer_Worker(bMode, 0x80, 2, 2, bs3CpuWeird1_InhibitedInt80_c16, bs3CpuWeird1_InhibitedInt80_int80_c16);
        if (!BS3_MODE_IS_V86(bMode) || !g_fVME)
        {
            /** @todo explain why these GURU     */
            bs3CpuWeird1_DbgInhibitRingXfer_Worker(bMode, 0x03, 2, 2, bs3CpuWeird1_InhibitedInt3_c16,  bs3CpuWeird1_InhibitedInt3_int3_c16);
            bs3CpuWeird1_DbgInhibitRingXfer_Worker(bMode, 0x03, 1, 2, bs3CpuWeird1_InhibitedBp_c16,    bs3CpuWeird1_InhibitedBp_int3_c16);
        }
    }
    else if (BS3_MODE_IS_32BIT_CODE(bMode))
    {
        bs3CpuWeird1_DbgInhibitRingXfer_Worker(bMode, 0x80, 2, 4, bs3CpuWeird1_InhibitedInt80_c32, bs3CpuWeird1_InhibitedInt80_int80_c32);
        bs3CpuWeird1_DbgInhibitRingXfer_Worker(bMode, 0x03, 2, 4, bs3CpuWeird1_InhibitedInt3_c32,  bs3CpuWeird1_InhibitedInt3_int3_c32);
        bs3CpuWeird1_DbgInhibitRingXfer_Worker(bMode, 0x03, 1, 4, bs3CpuWeird1_InhibitedBp_c32,    bs3CpuWeird1_InhibitedBp_int3_c32);
    }
    else
    {
        bs3CpuWeird1_DbgInhibitRingXfer_Worker(bMode, 0x80, 2, 0, bs3CpuWeird1_InhibitedInt80_c64, bs3CpuWeird1_InhibitedInt80_int80_c64);
        bs3CpuWeird1_DbgInhibitRingXfer_Worker(bMode, 0x03, 2, 0, bs3CpuWeird1_InhibitedInt3_c64,  bs3CpuWeird1_InhibitedInt3_int3_c64);
        bs3CpuWeird1_DbgInhibitRingXfer_Worker(bMode, 0x03, 1, 0, bs3CpuWeird1_InhibitedBp_c64,    bs3CpuWeird1_InhibitedBp_int3_c64);
    }

    return 0;
}


/*********************************************************************************************************************************
*   IP / EIP  Wrapping                                                                                                           *
*********************************************************************************************************************************/
#define PROTO_ALL(a_Template) \
    FNBS3FAR a_Template ## _c16, a_Template ## _c16_EndProc, \
             a_Template ## _c32, a_Template ## _c32_EndProc, \
             a_Template ## _c64, a_Template ## _c64_EndProc
PROTO_ALL(bs3CpuWeird1_PcWrapBenign1);
PROTO_ALL(bs3CpuWeird1_PcWrapBenign2);
PROTO_ALL(bs3CpuWeird1_PcWrapCpuId);
PROTO_ALL(bs3CpuWeird1_PcWrapIn80);
PROTO_ALL(bs3CpuWeird1_PcWrapOut80);
PROTO_ALL(bs3CpuWeird1_PcWrapSmsw);
PROTO_ALL(bs3CpuWeird1_PcWrapRdCr0);
PROTO_ALL(bs3CpuWeird1_PcWrapRdDr0);
PROTO_ALL(bs3CpuWeird1_PcWrapWrDr0);
#undef PROTO_ALL

typedef enum { kPcWrapSetup_None, kPcWrapSetup_ZeroRax } PCWRAPSETUP;

/**
 * Compares pc wraparound result.
 */
static uint8_t bs3CpuWeird1_ComparePcWrap(PCBS3TRAPFRAME pTrapCtx, PCBS3TRAPFRAME pTrapExpect)
{
    uint16_t const cErrorsBefore = Bs3TestSubErrorCount();
    CHECK_MEMBER("bXcpt",       "%#04x",    pTrapCtx->bXcpt,        pTrapExpect->bXcpt);
    CHECK_MEMBER("bErrCd",      "%#06RX64", pTrapCtx->uErrCd,       pTrapExpect->uErrCd);
    Bs3TestCheckRegCtxEx(&pTrapCtx->Ctx, &pTrapExpect->Ctx, 0 /*cbPcAdjust*/, 0 /*cbSpAdjust*/, 0 /*fExtraEfl*/,
                         g_pszTestMode, g_usBs3TestStep);
    if (Bs3TestSubErrorCount() != cErrorsBefore)
    {
        Bs3TrapPrintFrame(pTrapCtx);
        Bs3TestPrintf("CS=%04RX16 SS:ESP=%04RX16:%08RX64 EFL=%RX64 cbIret=%#x\n",
                      pTrapCtx->uHandlerCs, pTrapCtx->uHandlerSs, pTrapCtx->uHandlerRsp,
                      pTrapCtx->fHandlerRfl, pTrapCtx->cbIretFrame);
#if 0
        Bs3TestPrintf("Halting in ComparePcWrap: bXcpt=%#x\n", pTrapCtx->bXcpt);
        ASMHalt();
#endif
        return 1;
    }
    return 0;
}


static uint8_t bs3CpuWeird1_PcWrapping_Worker16(uint8_t bMode, RTSEL SelCode, uint8_t BS3_FAR *pbHead,
                                                uint8_t BS3_FAR *pbTail, uint8_t BS3_FAR *pbAfter,
                                                void const BS3_FAR *pvTemplate, size_t cbTemplate, PCWRAPSETUP enmSetup)
{
    BS3TRAPFRAME            TrapCtx;
    BS3TRAPFRAME            TrapExpect;
    BS3REGCTX               Ctx;
    uint8_t                 bXcpt;

    /* make sure they're allocated  */
    Bs3MemZero(&Ctx, sizeof(Ctx));
    Bs3MemZero(&TrapCtx, sizeof(TrapCtx));
    Bs3MemZero(&TrapExpect, sizeof(TrapExpect));

    /*
     * Create the expected result by first placing the code template
     * at the start of the buffer and giving it a quick run.
     *
     * I cannot think of any instruction always causing #GP(0) right now, so
     * we generate a ud2 and modify it instead.
     */
    Bs3MemCpy(pbHead, pvTemplate, cbTemplate);
    if ((g_uBs3CpuDetected & BS3CPU_TYPE_MASK) <= BS3CPU_80286)
    {
        pbHead[cbTemplate] = 0xcc;      /* int3 */
        bXcpt = X86_XCPT_BP;
    }
    else
    {
        pbHead[cbTemplate]     = 0x0f;  /* ud2 */
        pbHead[cbTemplate + 1] = 0x0b;
        bXcpt = X86_XCPT_UD;
    }

    Bs3RegCtxSaveEx(&Ctx, bMode, 1024);

    Ctx.cs    = SelCode;
    Ctx.rip.u = 0;
    switch (enmSetup)
    {
        case kPcWrapSetup_None:
            break;
        case kPcWrapSetup_ZeroRax:
            Ctx.rax.u = 0;
            break;
    }

    /* V8086: Set IOPL to 3. */
    if (BS3_MODE_IS_V86(bMode))
        Ctx.rflags.u32 |= X86_EFL_IOPL;

    Bs3TrapSetJmpAndRestore(&Ctx, &TrapExpect);
    if (TrapExpect.bXcpt != bXcpt)
    {

        Bs3TestFailedF("%u: Setup: bXcpt is %#x, expected %#x!\n", g_usBs3TestStep, TrapExpect.bXcpt, bXcpt);
        Bs3TrapPrintFrame(&TrapExpect);
        return 1;
    }

    /*
     * Adjust the contexts for the real test.
     */
    Ctx.cs    = SelCode;
    Ctx.rip.u = (uint32_t)_64K - cbTemplate;

    if ((g_uBs3CpuDetected & BS3CPU_TYPE_MASK) <= BS3CPU_80286)
        TrapExpect.Ctx.rip.u = 1;
    else
    {
        if (BS3_MODE_IS_16BIT_SYS(bMode))
            TrapExpect.Ctx.rip.u = 0;
        else
            TrapExpect.Ctx.rip.u = UINT32_C(0x10000);
        TrapExpect.bXcpt  = X86_XCPT_GP;
        TrapExpect.uErrCd = 0;
    }

    /*
     * Prepare the buffer for 16-bit wrap around.
     */
    Bs3MemSet(pbHead, 0xcc, 64);        /* int3 */
    if (bXcpt == X86_XCPT_UD)
    {
        pbHead[0] = 0x0f;               /* ud2 */
        pbHead[1] = 0x0b;
    }
    Bs3MemCpy(&pbTail[_4K - cbTemplate], pvTemplate, cbTemplate);
    Bs3MemSet(pbAfter, 0xf1, 64);       /* icebp / int1 */

    /*
     * Do a test run.
     */
    Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
    if (!bs3CpuWeird1_ComparePcWrap(&TrapCtx, &TrapExpect))
    {
#if 0 /* needs more work */
        /*
         * Slide the instruction template across the boundrary byte-by-byte and
         * check that it triggers #GP on the initial instruction on 386+.
         */
        unsigned cbTail;
        unsigned cbHead;
        g_usBs3TestStep++;
        for (cbTail = cbTemplate - 1, cbHead = 1; cbTail > 0; cbTail--, cbHead++, g_usBs3TestStep++)
        {
            pbTail[X86_PAGE_SIZE - cbTail - 1] = 0xcc;
            Bs3MemCpy(&pbTail[X86_PAGE_SIZE - cbTail], pvTemplate, cbTail);
            Bs3MemCpy(pbHead, &((uint8_t const *)pvTemplate)[cbTail], cbHead);
            if (bXcpt == X86_XCPT_BP)
                pbHead[cbHead]     = 0xcc; /* int3 */
            else
            {
                pbHead[cbHead]     = 0x0f; /* ud2 */
                pbHead[cbHead + 1] = 0x0b;
            }

            Ctx.rip.u = (uint32_t)_64K - cbTail;
            if ((g_uBs3CpuDetected & BS3CPU_TYPE_MASK) <= BS3CPU_80286)
                TrapExpect.Ctx.rip.u = cbHead + 1;
            else
            {
                TrapExpect.Ctx.rip.u = Ctx.rip.u;
            }

            Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
            if (bs3CpuWeird1_ComparePcWrap(&TrapCtx, &TrapExpect))
                return 1;
        }
#endif
    }
    return 0;
}


static uint8_t bs3CpuWeird1_PcWrapping_Worker32(uint8_t bMode, RTSEL SelCode, uint8_t BS3_FAR *pbPage1,
                                                uint8_t BS3_FAR *pbPage2, uint32_t uFlatPage2,
                                                void const BS3_FAR *pvTemplate, size_t cbTemplate, PCWRAPSETUP enmSetup)
{
    BS3TRAPFRAME            TrapCtx;
    BS3TRAPFRAME            TrapExpect;
    BS3REGCTX               Ctx;
    unsigned                cbPage1;
    unsigned                cbPage2;

    /* make sure they're allocated  */
    Bs3MemZero(&Ctx, sizeof(Ctx));
    Bs3MemZero(&TrapCtx, sizeof(TrapCtx));
    Bs3MemZero(&TrapExpect, sizeof(TrapExpect));

    //Bs3TestPrintf("SelCode=%#x pbPage1=%p pbPage2=%p uFlatPage2=%RX32 pvTemplate=%p cbTemplate\n",
    //              SelCode, pbPage1, pbPage2, uFlatPage2, pvTemplate, cbTemplate);

    /*
     * Create the expected result by first placing the code template
     * at the start of the buffer and giving it a quick run.
     */
    Bs3MemSet(pbPage1, 0xcc, _4K);
    Bs3MemSet(pbPage2, 0xcc, _4K);
    Bs3MemCpy(&pbPage1[_4K - cbTemplate], pvTemplate, cbTemplate);
    pbPage2[0] = 0x0f;      /* ud2 */
    pbPage2[1] = 0x0b;

    Bs3RegCtxSaveEx(&Ctx, bMode, 1024);

    Ctx.cs    = BS3_SEL_R0_CS32;
    Ctx.rip.u = uFlatPage2 - cbTemplate;
    switch (enmSetup)
    {
        case kPcWrapSetup_None:
            break;
        case kPcWrapSetup_ZeroRax:
            Ctx.rax.u = 0;
            break;
    }

    Bs3TrapSetJmpAndRestore(&Ctx, &TrapExpect);
    if (TrapExpect.bXcpt != X86_XCPT_UD)
    {

        Bs3TestFailedF("%u: Setup: bXcpt is %#x, expected %#x!\n", g_usBs3TestStep, TrapExpect.bXcpt, X86_XCPT_UD);
        Bs3TrapPrintFrame(&TrapExpect);
        return 1;
    }

    /*
     * The real test uses the special CS selector.
     */
    Ctx.cs            = SelCode;
    TrapExpect.Ctx.cs = SelCode;

    /*
     * Unlike 16-bit mode, the instruction may cross the wraparound boundary,
     * so we test by advancing the template across byte-by-byte.
     */
    for (cbPage1 = cbTemplate, cbPage2 = 0; cbPage1 > 0; cbPage1--, cbPage2++, g_usBs3TestStep++)
    {
        pbPage1[X86_PAGE_SIZE - cbPage1 - 1] = 0xcc;
        Bs3MemCpy(&pbPage1[X86_PAGE_SIZE - cbPage1], pvTemplate, cbPage1);
        Bs3MemCpy(pbPage2, &((uint8_t const *)pvTemplate)[cbPage1], cbPage2);
        pbPage2[cbPage2]     = 0x0f;    /* ud2 */
        pbPage2[cbPage2 + 1] = 0x0b;

        Ctx.rip.u = UINT32_MAX - cbPage1 + 1;
        TrapExpect.Ctx.rip.u = cbPage2;

        Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
        if (bs3CpuWeird1_ComparePcWrap(&TrapCtx, &TrapExpect))
            return 1;
    }
    return 0;
}


static uint8_t bs3CpuWeird1_PcWrapping_Worker64(uint8_t bMode, uint8_t BS3_FAR *pbBuf, uint32_t uFlatBuf,
                                                void const BS3_FAR *pvTemplate, size_t cbTemplate, PCWRAPSETUP enmSetup)
{
    uint8_t BS3_FAR * const pbPage1 = pbBuf;                 /* mapped at 0, 4G and 8G */
    uint8_t BS3_FAR * const pbPage2 = &pbBuf[X86_PAGE_SIZE]; /* mapped at -4K, 4G-4K and 8G-4K. */
    BS3TRAPFRAME            TrapCtx;
    BS3TRAPFRAME            TrapExpect;
    BS3REGCTX               Ctx;
    unsigned                cbStart;
    unsigned                cbEnd;

    /* make sure they're allocated  */
    Bs3MemZero(&Ctx, sizeof(Ctx));
    Bs3MemZero(&TrapCtx, sizeof(TrapCtx));
    Bs3MemZero(&TrapExpect, sizeof(TrapExpect));

    /*
     * Create the expected result by first placing the code template
     * at the start of the buffer and giving it a quick run.
     */
    Bs3MemCpy(pbPage1, pvTemplate, cbTemplate);
    pbPage1[cbTemplate]     = 0x0f; /* ud2 */
    pbPage1[cbTemplate + 1] = 0x0b;

    Bs3RegCtxSaveEx(&Ctx, bMode, 1024);

    Ctx.rip.u = uFlatBuf;
    switch (enmSetup)
    {
        case kPcWrapSetup_None:
            break;
        case kPcWrapSetup_ZeroRax:
            Ctx.rax.u = 0;
            break;
    }

    Bs3TrapSetJmpAndRestore(&Ctx, &TrapExpect);
    if (TrapExpect.bXcpt != X86_XCPT_UD)
    {

        Bs3TestFailedF("%u: Setup: bXcpt is %#x, expected %#x!\n", g_usBs3TestStep, TrapExpect.bXcpt, X86_XCPT_UD);
        Bs3TrapPrintFrame(&TrapExpect);
        return 1;
    }

    /*
     * Unlike 16-bit mode, the instruction may cross the wraparound boundary,
     * so we test by advancing the template across byte-by-byte.
     *
     * Page #1 is mapped at address zero and Page #2 as the last one.
     */
    Bs3MemSet(pbBuf, 0xf1, X86_PAGE_SIZE * 2);
    for (cbStart = cbTemplate, cbEnd = 0; cbStart > 0; cbStart--, cbEnd++)
    {
        pbPage2[X86_PAGE_SIZE - cbStart - 1] = 0xf1;
        Bs3MemCpy(&pbPage2[X86_PAGE_SIZE - cbStart], pvTemplate, cbStart);
        Bs3MemCpy(pbPage1, &((uint8_t const *)pvTemplate)[cbStart], cbEnd);
        pbPage1[cbEnd]     = 0x0f;    /* ud2 */
        pbPage1[cbEnd + 1] = 0x0b;

        Ctx.rip.u            = UINT64_MAX - cbStart + 1;
        TrapExpect.Ctx.rip.u = cbEnd;

        Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
        if (bs3CpuWeird1_ComparePcWrap(&TrapCtx, &TrapExpect))
            return 1;
        g_usBs3TestStep++;

        /* Also check that crossing 4G isn't buggered up in our code by
           32-bit and 16-bit mode support.*/
        Ctx.rip.u            = _4G - cbStart;
        TrapExpect.Ctx.rip.u = _4G + cbEnd;
        Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
        if (bs3CpuWeird1_ComparePcWrap(&TrapCtx, &TrapExpect))
            return 1;
        g_usBs3TestStep++;

        Ctx.rip.u            = _4G*2 - cbStart;
        TrapExpect.Ctx.rip.u = _4G*2 + cbEnd;
        Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
        if (bs3CpuWeird1_ComparePcWrap(&TrapCtx, &TrapExpect))
            return 1;
        g_usBs3TestStep += 2;
    }
    return 0;
}



BS3_DECL_FAR(uint8_t) BS3_CMN_FAR_NM(bs3CpuWeird1_PcWrapping)(uint8_t bMode)
{
    uint8_t bRet = 1;
    size_t  i;

    bs3CpuWeird1_SetGlobals(bMode);

    if (BS3_MODE_IS_16BIT_CODE(bMode))
    {
        /*
         * For 16-bit testing, we need a 68 KB buffer.
         *
         * This is a little annoying to work with from 16-bit bit, so we use
         * separate pointers to each interesting bit of it.
         */
        /** @todo add api for doing this, so we don't need to include bs3-cmn-memory.h. */
        uint8_t BS3_FAR *pbBuf = (uint8_t BS3_FAR *)Bs3SlabAllocEx(&g_Bs3Mem4KLow.Core, 17 /*cPages*/, 0 /*fFlags*/);
        if (pbBuf != NULL)
        {
            uint32_t const   uFlatBuf = Bs3SelPtrToFlat(pbBuf);
            uint8_t BS3_FAR *pbTail   = Bs3XptrFlatToCurrent(uFlatBuf + 0x0f000);
            uint8_t BS3_FAR *pbAfter  = Bs3XptrFlatToCurrent(uFlatBuf + UINT32_C(0x10000));
            RTSEL            SelCode;
            uint32_t         off;
            static struct { FPFNBS3FAR pfnStart, pfnEnd;  PCWRAPSETUP enmSetup; unsigned fNoV86 : 1; }
            const s_aTemplates16[] =
            {
#define ENTRY16(a_Template, a_enmSetup, a_fNoV86)     { a_Template ## _c16, a_Template ## _c16_EndProc, a_enmSetup, a_fNoV86 }
                ENTRY16(bs3CpuWeird1_PcWrapBenign1, kPcWrapSetup_None,    0),
                ENTRY16(bs3CpuWeird1_PcWrapBenign2, kPcWrapSetup_None,    0),
                ENTRY16(bs3CpuWeird1_PcWrapCpuId,   kPcWrapSetup_ZeroRax, 0),
                ENTRY16(bs3CpuWeird1_PcWrapIn80,    kPcWrapSetup_None,    0),
                ENTRY16(bs3CpuWeird1_PcWrapOut80,   kPcWrapSetup_None,    0),
                ENTRY16(bs3CpuWeird1_PcWrapSmsw,    kPcWrapSetup_None,    0),
                ENTRY16(bs3CpuWeird1_PcWrapRdCr0,   kPcWrapSetup_None,    1),
                ENTRY16(bs3CpuWeird1_PcWrapRdDr0,   kPcWrapSetup_None,    1),
                ENTRY16(bs3CpuWeird1_PcWrapWrDr0,   kPcWrapSetup_ZeroRax, 1),
#undef ENTRY16
            };

            /* Fill the buffer with int1 instructions: */
            for (off = 0; off < UINT32_C(0x11000); off += _4K)
            {
                uint8_t BS3_FAR *pbPage = Bs3XptrFlatToCurrent(uFlatBuf + off);
                Bs3MemSet(pbPage, 0xf1, _4K);
            }

            /* Setup the CS for it. */
            SelCode = (uint16_t)(uFlatBuf >> 4);
            if (!BS3_MODE_IS_RM_OR_V86(bMode))
            {
                Bs3SelSetup16BitCode(&Bs3GdteSpare00, uFlatBuf, 0);
                SelCode = BS3_SEL_SPARE_00;
            }

            /* Allow IN and OUT to port 80h from V8086 mode. */
            if (BS3_MODE_IS_V86(bMode))
            {
                Bs3RegSetTr(BS3_SEL_TSS32_IOBP_IRB);
                ASMBitClear(Bs3SharedIobp, 0x80);
            }

            for (i = 0; i < RT_ELEMENTS(s_aTemplates16); i++)
            {
                if (!s_aTemplates16[i].fNoV86 || !BS3_MODE_IS_V86(bMode))
                    bs3CpuWeird1_PcWrapping_Worker16(bMode, SelCode, pbBuf, pbTail, pbAfter, s_aTemplates16[i].pfnStart,
                                                     (uintptr_t)s_aTemplates16[i].pfnEnd - (uintptr_t)s_aTemplates16[i].pfnStart,
                                                     s_aTemplates16[i].enmSetup);
                g_usBs3TestStep = i * 256;
            }

            if (BS3_MODE_IS_V86(bMode))
                ASMBitSet(Bs3SharedIobp, 0x80);

            Bs3SlabFree(&g_Bs3Mem4KLow.Core, uFlatBuf, 17);

            bRet = 0;
        }
        else
            Bs3TestFailed("Failed to allocate 17 pages (68KB)");
    }
    else
    {
        /*
         * For 32-bit and 64-bit mode we just need two pages.
         */
        size_t const     cbBuf = X86_PAGE_SIZE * 2;
        uint8_t BS3_FAR *pbBuf = (uint8_t BS3_FAR *)Bs3MemAlloc(BS3MEMKIND_TILED, cbBuf);
        if (pbBuf)
        {
            uint32_t const uFlatBuf = Bs3SelPtrToFlat(pbBuf);
            Bs3MemSet(pbBuf, 0xf1, cbBuf);

            /*
             * For 32-bit we set up a CS that starts with the 2nd page and
             * ends with the first.
             */
            if (BS3_MODE_IS_32BIT_CODE(bMode))
            {
                static struct { FPFNBS3FAR pfnStart, pfnEnd; PCWRAPSETUP enmSetup; } const s_aTemplates32[] =
                {
#define ENTRY32(a_Template, a_enmSetup)  { a_Template ## _c32, a_Template ## _c32_EndProc, a_enmSetup }
                    ENTRY32(bs3CpuWeird1_PcWrapBenign1, kPcWrapSetup_None),
                    ENTRY32(bs3CpuWeird1_PcWrapBenign2, kPcWrapSetup_None),
                    ENTRY32(bs3CpuWeird1_PcWrapCpuId,   kPcWrapSetup_ZeroRax),
                    ENTRY32(bs3CpuWeird1_PcWrapIn80,    kPcWrapSetup_None),
                    ENTRY32(bs3CpuWeird1_PcWrapOut80,   kPcWrapSetup_None),
                    ENTRY32(bs3CpuWeird1_PcWrapSmsw,    kPcWrapSetup_None),
                    ENTRY32(bs3CpuWeird1_PcWrapRdCr0,   kPcWrapSetup_None),
                    ENTRY32(bs3CpuWeird1_PcWrapRdDr0,   kPcWrapSetup_None),
                    ENTRY32(bs3CpuWeird1_PcWrapWrDr0,   kPcWrapSetup_ZeroRax),
#undef ENTRY32
                };

                Bs3SelSetup32BitCode(&Bs3GdteSpare00, uFlatBuf + X86_PAGE_SIZE, UINT32_MAX, 0);

                for (i = 0; i < RT_ELEMENTS(s_aTemplates32); i++)
                {
                    //Bs3TestPrintf("pfnStart=%p pfnEnd=%p\n", s_aTemplates32[i].pfnStart, s_aTemplates32[i].pfnEnd);
                    bs3CpuWeird1_PcWrapping_Worker32(bMode, BS3_SEL_SPARE_00, pbBuf, &pbBuf[X86_PAGE_SIZE],
                                                     uFlatBuf + X86_PAGE_SIZE, Bs3SelLnkPtrToCurPtr(s_aTemplates32[i].pfnStart),
                                                     (uintptr_t)s_aTemplates32[i].pfnEnd - (uintptr_t)s_aTemplates32[i].pfnStart,
                                                     s_aTemplates32[i].enmSetup);
                    g_usBs3TestStep = i * 256;
                }

                bRet = 0;
            }
            /*
             * For 64-bit we have to alias the two buffer pages to the first and
             * last page in the address space. To test that the 32-bit 4G rollover
             * isn't incorrectly applied to LM64, we repeat this mappingfor the 4G
             * and 8G boundaries too.
             *
             * This ASSUMES there is nothing important in page 0 when in LM64.
             */
            else
            {
                static const struct { uint64_t uDst; uint16_t off; } s_aMappings[] =
                {
                    { UINT64_MAX - X86_PAGE_SIZE + 1, X86_PAGE_SIZE * 1 },
                    { UINT64_C(0),                    X86_PAGE_SIZE * 0 },
#if 1 /* technically not required as we just repeat the same 4G address space in long mode: */
                    { _4G - X86_PAGE_SIZE,            X86_PAGE_SIZE * 1 },
                    { _4G,                            X86_PAGE_SIZE * 0 },
                    { _4G*2 - X86_PAGE_SIZE,          X86_PAGE_SIZE * 1 },
                    { _4G*2,                          X86_PAGE_SIZE * 0 },
#endif
                };
                int      rc = VINF_SUCCESS;
                unsigned iMap;
                BS3_ASSERT(bMode == BS3_MODE_LM64);
                for (iMap = 0; iMap < RT_ELEMENTS(s_aMappings) && RT_SUCCESS(rc); iMap++)
                {
                    rc = Bs3PagingAlias(s_aMappings[iMap].uDst, uFlatBuf + s_aMappings[iMap].off, X86_PAGE_SIZE,
                                        X86_PTE_P | X86_PTE_A | X86_PTE_D | X86_PTE_RW);
                    if (RT_FAILURE(rc))
                        Bs3TestFailedF("Bs3PagingAlias(%#RX64,...) failed: %d", s_aMappings[iMap].uDst, rc);
                }

                if (RT_SUCCESS(rc))
                {
                    static struct { FPFNBS3FAR pfnStart, pfnEnd; PCWRAPSETUP enmSetup; } const s_aTemplates64[] =
                    {
#define ENTRY64(a_Template, a_enmSetup) { a_Template ## _c64, a_Template ## _c64_EndProc, a_enmSetup }
                        ENTRY64(bs3CpuWeird1_PcWrapBenign1, kPcWrapSetup_None),
                        ENTRY64(bs3CpuWeird1_PcWrapBenign2, kPcWrapSetup_None),
                        ENTRY64(bs3CpuWeird1_PcWrapCpuId,   kPcWrapSetup_ZeroRax),
                        ENTRY64(bs3CpuWeird1_PcWrapIn80,    kPcWrapSetup_None),
                        ENTRY64(bs3CpuWeird1_PcWrapOut80,   kPcWrapSetup_None),
                        ENTRY64(bs3CpuWeird1_PcWrapSmsw,    kPcWrapSetup_None),
                        ENTRY64(bs3CpuWeird1_PcWrapRdCr0,   kPcWrapSetup_None),
                        ENTRY64(bs3CpuWeird1_PcWrapRdDr0,   kPcWrapSetup_None),
                        ENTRY64(bs3CpuWeird1_PcWrapWrDr0,   kPcWrapSetup_ZeroRax),
#undef ENTRY64
                    };

                    for (i = 0; i < RT_ELEMENTS(s_aTemplates64); i++)
                    {
                        bs3CpuWeird1_PcWrapping_Worker64(bMode, pbBuf, uFlatBuf,
                                                         Bs3SelLnkPtrToCurPtr(s_aTemplates64[i].pfnStart),
                                                            (uintptr_t)s_aTemplates64[i].pfnEnd
                                                          - (uintptr_t)s_aTemplates64[i].pfnStart,
                                                         s_aTemplates64[i].enmSetup);
                        g_usBs3TestStep = i * 256;
                    }

                    bRet = 0;

                    Bs3PagingUnalias(UINT64_C(0), X86_PAGE_SIZE);
                }

                while (iMap-- > 0)
                    Bs3PagingUnalias(s_aMappings[iMap].uDst, X86_PAGE_SIZE);
            }
            Bs3MemFree(pbBuf, cbBuf);
        }
        else
            Bs3TestFailed("Failed to allocate 2-3 pages for tests.");
    }

    return bRet;
}

