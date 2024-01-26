/* $Id: bs3-cpu-basic-2.c $ */
/** @file
 * BS3Kit - bs3-cpu-basic-2, 16-bit C code.
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
#include <bs3kit.h>
#include <iprt/asm-amd64-x86.h>


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
BS3TESTMODE_PROTOTYPES_MODE(bs3CpuBasic2_TssGateEsp);
BS3TESTMODE_PROTOTYPES_MODE(bs3CpuBasic2_RaiseXcpt1);

FNBS3TESTDOMODE             bs3CpuBasic2_RaiseXcpt11_f16;
FNBS3TESTDOMODE             bs3CpuBasic2_sidt_f16;
FNBS3TESTDOMODE             bs3CpuBasic2_sgdt_f16;
FNBS3TESTDOMODE             bs3CpuBasic2_lidt_f16;
FNBS3TESTDOMODE             bs3CpuBasic2_lgdt_f16;
FNBS3TESTDOMODE             bs3CpuBasic2_iret_f16;
FNBS3TESTDOMODE             bs3CpuBasic2_jmp_call_f16;
FNBS3TESTDOMODE             bs3CpuBasic2_far_jmp_call_f16;
FNBS3TESTDOMODE             bs3CpuBasic2_near_ret_f16;
FNBS3TESTDOMODE             bs3CpuBasic2_far_ret_f16;
FNBS3TESTDOMODE             bs3CpuBasic2_instr_len_f16;

BS3_DECL_CALLBACK(void)     bs3CpuBasic2_Do32BitTests_pe32();


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static const BS3TESTMODEENTRY g_aModeTest[] =
{
    BS3TESTMODEENTRY_MODE("tss / gate / esp", bs3CpuBasic2_TssGateEsp),
#if 0 /** @todo The 'raise xcpt \#1' test doesn't work in IEM! */
    BS3TESTMODEENTRY_MODE("raise xcpt #1", bs3CpuBasic2_RaiseXcpt1),
#endif
};

static const BS3TESTMODEBYONEENTRY g_aModeByOneTests[] =
{
#if 1
    { "#ac",  bs3CpuBasic2_RaiseXcpt11_f16, 0 },
#endif
#if 1
    { "iret", bs3CpuBasic2_iret_f16, 0 },
    { "near jmp+call jb / jv / ind",  bs3CpuBasic2_jmp_call_f16, 0 },
    { "far jmp+call",  bs3CpuBasic2_far_jmp_call_f16, 0 },
    { "near ret",  bs3CpuBasic2_near_ret_f16, 0 },
    { "far ret",   bs3CpuBasic2_far_ret_f16, 0 },
#endif
#if 1
    { "sidt", bs3CpuBasic2_sidt_f16, 0 },
    { "sgdt", bs3CpuBasic2_sgdt_f16, 0 },
    { "lidt", bs3CpuBasic2_lidt_f16, 0 },
    { "lgdt", bs3CpuBasic2_lgdt_f16, 0 },
#endif
#if 1
    { "instr length", bs3CpuBasic2_instr_len_f16, 0 },
#endif
};


BS3_DECL(void) Main_rm()
{
    Bs3InitAll_rm();
    Bs3TestInit("bs3-cpu-basic-2");
    Bs3TestPrintf("g_uBs3CpuDetected=%#x\n", g_uBs3CpuDetected);

    /*
     * Do tests driven from 16-bit code.
     */
    NOREF(g_aModeTest); NOREF(g_aModeByOneTests); /* for when commenting out bits */
#if 1
    Bs3TestDoModes_rm(g_aModeTest, RT_ELEMENTS(g_aModeTest));
#endif
    Bs3TestDoModesByOne_rm(g_aModeByOneTests, RT_ELEMENTS(g_aModeByOneTests), 0);

#if 0 /** @todo The '\#PF' test doesn't work right in IEM! */
    /*
     * Do tests driven from 32-bit code (bs3-cpu-basic-2-32.c32 via assembly).
     */
    Bs3SwitchTo32BitAndCallC_rm(bs3CpuBasic2_Do32BitTests_pe32, 0);
#endif

    Bs3TestTerm();
    Bs3Shutdown();
for (;;) { ASMHalt(); }
}

