/* $Id: bs3-cpu-instr-2.c $ */
/** @file
 * BS3Kit - bs3-cpu-instr-2, 16-bit C code.
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


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_mul);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_imul);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_div);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_idiv);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_bsf_tzcnt);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_bsr_lzcnt);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_andn);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_bextr);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_blsr);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_blsmsk);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_blsi);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_bzhi);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_pdep);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_pext);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_rorx);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_shlx);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_sarx);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_shrx);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_mulx);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_popcnt);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_crc32);
BS3TESTMODE_PROTOTYPES_CMN_64(bs3CpuInstr2_cmpxchg16b);
BS3TESTMODE_PROTOTYPES_CMN_64(bs3CpuInstr2_wrfsbase);
BS3TESTMODE_PROTOTYPES_CMN_64(bs3CpuInstr2_wrgsbase);
BS3TESTMODE_PROTOTYPES_CMN_64(bs3CpuInstr2_rdfsbase);
BS3TESTMODE_PROTOTYPES_CMN_64(bs3CpuInstr2_rdgsbase);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static const BS3TESTMODEENTRY g_aModeTests[] =
{
#if 1
    BS3TESTMODEENTRY_CMN("mul", bs3CpuInstr2_mul),
    BS3TESTMODEENTRY_CMN("imul", bs3CpuInstr2_imul),
    BS3TESTMODEENTRY_CMN("div", bs3CpuInstr2_div),
    BS3TESTMODEENTRY_CMN("idiv", bs3CpuInstr2_idiv),
#endif
#if 1 /* BSF/BSR (386+) & TZCNT/LZCNT (BMI1,ABM) */
    BS3TESTMODEENTRY_CMN("bsf/tzcnt",  bs3CpuInstr2_bsf_tzcnt),
    BS3TESTMODEENTRY_CMN("bsr/lzcnt",  bs3CpuInstr2_bsr_lzcnt),
#endif
#if 1 /* BMI1 */
    BS3TESTMODEENTRY_CMN("andn",    bs3CpuInstr2_andn),
    BS3TESTMODEENTRY_CMN("bextr",   bs3CpuInstr2_bextr),
    BS3TESTMODEENTRY_CMN("blsr",    bs3CpuInstr2_blsr),
    BS3TESTMODEENTRY_CMN("blsmsk",  bs3CpuInstr2_blsmsk),
    BS3TESTMODEENTRY_CMN("blsi",    bs3CpuInstr2_blsi),
#endif
#if 1 /* BMI2 */
    BS3TESTMODEENTRY_CMN("bzhi",  bs3CpuInstr2_bzhi),
    BS3TESTMODEENTRY_CMN("pdep",  bs3CpuInstr2_pdep),
    BS3TESTMODEENTRY_CMN("pext",  bs3CpuInstr2_pext),
    BS3TESTMODEENTRY_CMN("rorx",  bs3CpuInstr2_rorx),
    BS3TESTMODEENTRY_CMN("shlx",  bs3CpuInstr2_shlx),
    BS3TESTMODEENTRY_CMN("sarx",  bs3CpuInstr2_sarx),
    BS3TESTMODEENTRY_CMN("shrx",  bs3CpuInstr2_shrx),
    BS3TESTMODEENTRY_CMN("mulx",  bs3CpuInstr2_mulx),
#endif
#if 1
    BS3TESTMODEENTRY_CMN("popcnt",  bs3CpuInstr2_popcnt), /* Intel: POPCNT; AMD: ABM */
    BS3TESTMODEENTRY_CMN("crc32",  bs3CpuInstr2_crc32),   /* SSE4.2 */
#endif
#if 1
    BS3TESTMODEENTRY_CMN_64("cmpxchg16b", bs3CpuInstr2_cmpxchg16b),
    BS3TESTMODEENTRY_CMN_64("wrfsbase", bs3CpuInstr2_wrfsbase),
    BS3TESTMODEENTRY_CMN_64("wrgsbase", bs3CpuInstr2_wrgsbase),
    BS3TESTMODEENTRY_CMN_64("rdfsbase", bs3CpuInstr2_rdfsbase),
    BS3TESTMODEENTRY_CMN_64("rdgsbase", bs3CpuInstr2_rdgsbase),
#endif
};


BS3_DECL(void) Main_rm()
{
    Bs3InitAll_rm();
    Bs3TestInit("bs3-cpu-instr-2");

    Bs3TestDoModes_rm(g_aModeTests, RT_ELEMENTS(g_aModeTests));

    Bs3TestTerm();
}

