/* $Id: Disasm.cpp $ */
/** @file
 * VBox disassembler - Disassemble and optionally format.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DIS
#include <VBox/dis.h>
#include <VBox/disopcode.h>
#include <iprt/errcore.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include "DisasmInternal.h"


/**
 * Disassembles one instruction
 *
 * @returns VBox error code
 * @param   pvInstr         Pointer to the instruction to disassemble.
 * @param   enmCpuMode      The CPU state.
 * @param   pDis            The disassembler state (output).
 * @param   pcbInstr        Where to store the size of the instruction. NULL is
 *                          allowed.
 * @param   pszOutput       Storage for disassembled instruction
 * @param   cbOutput        Size of the output buffer.
 *
 * @todo    Define output callback.
 */
DISDECL(int) DISInstrToStr(void const *pvInstr, DISCPUMODE enmCpuMode, PDISSTATE pDis, uint32_t *pcbInstr,
                           char *pszOutput, size_t cbOutput)
{
    return DISInstrToStrEx((uintptr_t)pvInstr, enmCpuMode, NULL, NULL, DISOPTYPE_ALL,
                           pDis, pcbInstr, pszOutput, cbOutput);
}

/**
 * Disassembles one instruction with a byte fetcher caller.
 *
 * @returns VBox error code
 * @param   uInstrAddr      Pointer to the structure to disassemble.
 * @param   enmCpuMode      The CPU mode.
 * @param   pfnCallback     The byte fetcher callback.
 * @param   pvUser          The user argument (found in
 *                          DISSTATE::pvUser).
 * @param   pDis            The disassembler state (output).
 * @param   pcbInstr        Where to store the size of the instruction. NULL is
 *                          allowed.
 * @param   pszOutput       Storage for disassembled instruction.
 * @param   cbOutput        Size of the output buffer.
 *
 * @todo    Define output callback.
 */
DISDECL(int) DISInstrToStrWithReader(RTUINTPTR uInstrAddr, DISCPUMODE enmCpuMode, PFNDISREADBYTES pfnReadBytes, void *pvUser,
                                     PDISSTATE pDis, uint32_t *pcbInstr, char *pszOutput, size_t cbOutput)

{
    return DISInstrToStrEx(uInstrAddr, enmCpuMode, pfnReadBytes, pvUser, DISOPTYPE_ALL,
                           pDis, pcbInstr, pszOutput, cbOutput);
}

/**
 * Disassembles one instruction; only fully disassembly an instruction if it matches the filter criteria
 *
 * @returns VBox error code
 * @param   uInstrAddr      Pointer to the structure to disassemble.
 * @param   enmCpuMode      The CPU mode.
 * @param   pfnCallback     The byte fetcher callback.
 * @param   uFilter         Instruction filter.
 * @param   pDis            Where to return the disassembled instruction info.
 * @param   pcbInstr        Where to store the size of the instruction. NULL is
 *                          allowed.
 * @param   pszOutput       Storage for disassembled instruction.
 * @param   cbOutput        Size of the output buffer.
 *
 * @todo    Define output callback.
 */
DISDECL(int) DISInstrToStrEx(RTUINTPTR uInstrAddr, DISCPUMODE enmCpuMode,
                             PFNDISREADBYTES pfnReadBytes, void *pvUser, uint32_t uFilter,
                             PDISSTATE pDis, uint32_t *pcbInstr, char *pszOutput, size_t cbOutput)
{
    /* Don't filter if formatting is desired. */
    if (uFilter !=  DISOPTYPE_ALL && pszOutput && cbOutput)
        uFilter = DISOPTYPE_ALL;

    int rc = DISInstrEx(uInstrAddr, enmCpuMode, uFilter, pfnReadBytes, pvUser, pDis, pcbInstr);
    if (RT_SUCCESS(rc) && pszOutput && cbOutput)
    {
        size_t cch = DISFormatYasmEx(pDis, pszOutput, cbOutput,
                                     DIS_FMT_FLAGS_BYTES_LEFT | DIS_FMT_FLAGS_BYTES_BRACKETS | DIS_FMT_FLAGS_BYTES_SPACED
                                     | DIS_FMT_FLAGS_RELATIVE_BRANCH | DIS_FMT_FLAGS_ADDR_LEFT,
                                     NULL /*pfnGetSymbol*/, NULL /*pvUser*/);
        if (cch + 2 <= cbOutput)
        {
            pszOutput[cch++] = '\n';
            pszOutput[cch] = '\0';
        }
    }
    return rc;
}

