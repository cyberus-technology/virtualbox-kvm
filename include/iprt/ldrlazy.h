/** @file
 * IPRT - Lazy share library linking (2nd try).
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

#ifndef IPRT_INCLUDED_ldrlazy_h
#define IPRT_INCLUDED_ldrlazy_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/ldr.h>

/** @defgroup grp_rt_ldrlazy        RTLdrLazy - Lazy shared library linking.
 * @ingroup grp_rt
 *
 * This is a set of macros which will produce code for dynamically loading and
 * resolving symbols in shared libraries (DLLs).
 *
 * There is an assembly language alternative to this that only requires writing
 * a list of symbols in a format similar to what the microsoft linkers take as
 * input when producing DLLs and import libraries.  That is probably preferable
 * over this code.  See src/bldprog/VBoxDef2LazyLoad.cpp.
 *
 * @{
 */


/**
 * Defines a module for use in lazy resolving.
 *
 * @param   a_Mod       The module name (C name).
 * @param   a_pszFile   The file to tell RTLdrLoad to load.
 */
#define RTLDRLAZY_MODULE(a_Mod, a_pszFile) \
    RTLDRLAZY_MODULE_EX(a_Mod, a_pszFile, RTLdrLoad)

/**
 * Defines a module for use in lazy resolving.
 *
 * @param   a_Mod       The module name (C name).
 * @param   a_pszFile   The file to tell RTLdrLoad to load.
 * @param   a_pfnLoadIt Function to call for loading the DLL, replacing
 *                      RTLdrLoad.
 */
#define RTLDRLAZY_MODULE_EX(a_Mod, a_pszFile, a_pfnLoadIt) \
    static bool rtLdrLazy_##a_Mod##_Resolve(const char *pszName, void **ppvSymbol) \
    { \
        static RTLDRMOD volatile    s_hMod    = NIL_RTLDRMOD; \
        static bool volatile        s_fLoaded = false; \
        RTLDRMOD    hMod; \
        int         rc; \
        if (!s_fLoaded) \
        { \
            rc = a_pfnLoadIt(a_pszFile, &hMod); \
            s_hMod = RT_SUCCESS(rc) ? hMod : NIL_RTLDRMOD; \
            s_fLoaded = true; \
            if (RT_FAILURE(rc)) \
                return false; \
        } \
        hMod = s_hMod; \
        if (hMod == NIL_RTLDRMOD) \
            return false; \
        rc = RTLdrGetSymbol(hMod, pszName, ppvSymbol); \
        return RT_SUCCESS(rc); \
    }



/** Function name mangler for preventing collision with system prototypes. */
#define RTLDRLAZY_FUNC_NAME(a_Mod, a_Name) a_Mod##__##a_Name

/**
 * Defines a function that should be lazily resolved.
 */
#define RTLDRLAZY_FUNC(a_Mod, a_RetType, a_CallConv, a_Name, a_ParamDecl, a_ParamNames, a_ErrRet) \
    DECLINLINE(a_RetType) RTLDRLAZY_FUNC_NAME(a_Mod, a_Name) a_ParamDecl \
    { \
        static a_RetType (a_CallConv * s_pfn) a_ParamDecl; \
        if (!s_pfn) \
        { \
            if (!rtLdrLazy_##a_Mod##_Resolve(#a_Name, (void **)&s_pfn)) \
                return a_ErrRet; \
        } \
        return s_pfn a_ParamNames; \
    }


/** @} */

#endif /* !IPRT_INCLUDED_ldrlazy_h */

