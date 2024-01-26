/** @file
 * Symbols from dbghelp.dll, allowing us to select which one to load.
 */

/*
 * Copyright (C) 2013-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_win_lazy_dbghelp_h
#define IPRT_INCLUDED_win_lazy_dbghelp_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/ldrlazy.h>
#include <iprt/path.h>
#include <iprt/env.h>
#include <iprt/errcore.h>


/**
 * Custom loader callback.
 * @returns Module handle or NIL_RTLDRMOD.
 */
static int rtLdrLazyLoadDbgHelp(const char *pszFile, PRTLDRMOD phMod)
{
    static const struct
    {
        const char *pszEnv;
        const char *pszSubDir;
    } s_aLocations[] =
    {
#ifdef RT_ARCH_AMD64
        { "ProgramFiles(x86)",  "Windows Kits\\8.1\\Debuggers\\x64\\dbghelp.dll" },
        { "ProgramFiles(x86)",  "Windows Kits\\8.0\\Debuggers\\x64\\dbghelp.dll" },
        { "ProgramFiles",       "Debugging Tools for Windows (x64)\\dbghelp.dll" },
#else
        { "ProgramFiles",       "Windows Kits\\8.1\\Debuggers\\x86\\dbghelp.dll" },
        { "ProgramFiles",       "Windows Kits\\8.0\\Debuggers\\x86\\dbghelp.dll" },
        { "ProgramFiles",       "Debugging Tools for Windows (x86)\\dbghelp.dll" },
#endif /** @todo More places we should look? */
    };
    uint32_t i;
    for (i = 0; i < RT_ELEMENTS(s_aLocations); i++)
    {
        char   szPath[RTPATH_MAX];
        size_t cchPath;
        int rc = RTEnvGetEx(RTENV_DEFAULT, s_aLocations[i].pszEnv, szPath, sizeof(szPath), &cchPath);
        if (RT_SUCCESS(rc))
        {
            rc = RTPathAppend(szPath, sizeof(szPath), s_aLocations[i].pszSubDir);
            if (RT_SUCCESS(rc))
            {
                rc = RTLdrLoad(szPath, phMod);
                if (RT_SUCCESS(rc))
                    return rc;
            }
        }
    }

    /* Fall back on the system one, if present. */
    return RTLdrLoadSystem(pszFile, true /*fNoUnload*/, phMod);
}

RTLDRLAZY_MODULE_EX(dbghelp, "dbghelp.dll", rtLdrLazyLoadDbgHelp);

RTLDRLAZY_FUNC(dbghelp, BOOL, WINAPI, SymInitialize, (HANDLE a1, PCWSTR a2, BOOL a3), (a1, a2, a3), FALSE);
#undef SymInitialize
#define SymInitialize RTLDRLAZY_FUNC_NAME(dbghelp, SymInitialize)

RTLDRLAZY_FUNC(dbghelp, BOOL, WINAPI, SymCleanup, (HANDLE a1), (a1), FALSE);
#undef SymCleanup
#define SymCleanup RTLDRLAZY_FUNC_NAME(dbghelp, SymCleanup)

RTLDRLAZY_FUNC(dbghelp, DWORD, WINAPI, SymGetOptions, (VOID), (), 0);
#undef SymGetOptions
#define SymGetOptions RTLDRLAZY_FUNC_NAME(dbghelp, SymGetOptions)

RTLDRLAZY_FUNC(dbghelp, DWORD, WINAPI, SymSetOptions, (DWORD a1), (a1), 0);
#undef SymSetOptions
#define SymSetOptions RTLDRLAZY_FUNC_NAME(dbghelp, SymSetOptions)

RTLDRLAZY_FUNC(dbghelp, BOOL, WINAPI, SymRegisterCallback64, (HANDLE a1, PSYMBOL_REGISTERED_CALLBACK64 a2, ULONG64 a3),
               (a1, a2, a3), FALSE);
#undef SymRegisterCallback64
#define SymRegisterCallback64 RTLDRLAZY_FUNC_NAME(dbghelp, SymRegisterCallback64)

RTLDRLAZY_FUNC(dbghelp, DWORD64, WINAPI, SymLoadModuleEx,
               (HANDLE a1, HANDLE a2, PCSTR a3, PCSTR a4, DWORD64 a5, DWORD a6, PMODLOAD_DATA a7, DWORD a8),
               (a1, a2, a3, a4, a5, a6, a7, a8), 0);
#undef SymLoadModuleEx
#define SymLoadModuleEx RTLDRLAZY_FUNC_NAME(dbghelp, SymLoadModuleEx)

RTLDRLAZY_FUNC(dbghelp, DWORD64, WINAPI, SymLoadModuleExW,
               (HANDLE a1, HANDLE a2, PCWSTR a3, PCWSTR a4, DWORD64 a5, DWORD a6, PMODLOAD_DATA a7, DWORD a8),
               (a1, a2, a3, a4, a5, a6, a7, a8), 0);
#undef SymLoadModuleExW
#define SymLoadModuleExW RTLDRLAZY_FUNC_NAME(dbghelp, SymLoadModuleExW)

RTLDRLAZY_FUNC(dbghelp, DWORD64, WINAPI, SymUnloadModule64, (HANDLE a1, DWORD64 a2), (a1, a2), 0);
#undef SymUnloadModule64
#define SymUnloadModule64 RTLDRLAZY_FUNC_NAME(dbghelp, SymUnloadModule64)

RTLDRLAZY_FUNC(dbghelp, BOOL, WINAPI, SymEnumSymbols,
               (HANDLE a1, ULONG64 a2, PCSTR a3, PSYM_ENUMERATESYMBOLS_CALLBACK a4, PVOID a5),
               (a1, a2, a3, a4, a5), FALSE);
#undef SymEnumSymbols
#define SymEnumSymbols RTLDRLAZY_FUNC_NAME(dbghelp, SymEnumSymbols)

RTLDRLAZY_FUNC(dbghelp, BOOL, WINAPI, SymEnumLinesW,
               (HANDLE a1, ULONG64 a2, PCWSTR a3, PCWSTR a4, PSYM_ENUMLINES_CALLBACKW a5, PVOID a6),
               (a1, a2, a3, a4, a5, a6), FALSE);
#undef SymEnumLinesW
#define SymEnumLinesW RTLDRLAZY_FUNC_NAME(dbghelp, SymEnumLinesW)

RTLDRLAZY_FUNC(dbghelp, BOOL, WINAPI, SymGetModuleInfo64, (HANDLE a1, DWORD64 a2, PIMAGEHLP_MODULE64 a3), (a1, a2, a3), FALSE);
#undef SymGetModuleInfo64
#define SymGetModuleInfo64 RTLDRLAZY_FUNC_NAME(dbghelp, SymGetModuleInfo64)




#endif /* !IPRT_INCLUDED_win_lazy_dbghelp_h */

