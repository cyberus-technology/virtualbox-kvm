/* $Id: bs3-cpu-weird-1.c $ */
/** @file
 * BS3Kit - bs3-cpu-weird-1, 16-bit C code.
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
FNBS3TESTDOMODE bs3CpuWeird1_DbgInhibitRingXfer_f16;
FNBS3TESTDOMODE bs3CpuWeird1_PcWrapping_f16;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static const BS3TESTMODEBYONEENTRY g_aModeByOneTests[] =
{
    { "dbg+inhibit+ringxfer", bs3CpuWeird1_DbgInhibitRingXfer_f16, 0 },
    { "pc wrapping", bs3CpuWeird1_PcWrapping_f16, 0 },
};


BS3_DECL(void) Main_rm()
{
    Bs3InitAll_rm();
    Bs3TestInit("bs3-cpu-weird-1");
    Bs3TestPrintf("g_uBs3CpuDetected=%#x\n", g_uBs3CpuDetected);

    /*
     * Do tests driven from 16-bit code.
     */
    Bs3TestDoModesByOne_rm(g_aModeByOneTests, RT_ELEMENTS(g_aModeByOneTests), 0);

    Bs3TestTerm();
    Bs3Shutdown();
}

