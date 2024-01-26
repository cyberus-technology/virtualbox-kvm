/** @file
 *
 * VBox HDD container test utility - scripting engine, internal script structures.
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
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef VBOX_INCLUDED_SRC_testcase_VDScriptInternal_h
#define VBOX_INCLUDED_SRC_testcase_VDScriptInternal_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/list.h>
#include <iprt/string.h>

#include "VDScript.h"

/**
 * Script function which can be called.
 */
typedef struct VDSCRIPTFN
{
    /** String space core. */
    RTSTRSPACECORE               Core;
    /** Flag whether function is defined in the source or was
     * registered from the outside. */
    bool                         fExternal;
    /** Flag dependent data. */
    union
    {
        /** Data for functions defined in the source. */
        struct
        {
            /** Pointer to the AST defining the function. */
            PVDSCRIPTASTFN       pAstFn;
        } Internal;
        /** Data for external defined functions. */
        struct
        {
            /** Callback function. */
            PFNVDSCRIPTCALLBACK  pfnCallback;
            /** Opaque user data. */
            void                *pvUser;
        } External;
    } Type;
    /** Return type of the function. */
    VDSCRIPTTYPE                 enmTypeRetn;
    /** Number of arguments the function takes. */
    unsigned                     cArgs;
    /** Variable sized array of argument types. */
    VDSCRIPTTYPE                 aenmArgTypes[1];
} VDSCRIPTFN;
/** Pointer to a script function registration structure. */
typedef VDSCRIPTFN *PVDSCRIPTFN;

/** Pointer to a tokenize state. */
typedef struct VDTOKENIZER *PVDTOKENIZER;

/**
 * Script context.
 */
typedef struct VDSCRIPTCTXINT
{
    /** String space of external registered and source defined functions. */
    RTSTRSPACE        hStrSpaceFn;
    /** List of ASTs for functions - VDSCRIPTASTFN. */
    RTLISTANCHOR      ListAst;
    /** Pointer to the current tokenizer state. */
    PVDTOKENIZER      pTokenizer;
} VDSCRIPTCTXINT;
/** Pointer to a script context. */
typedef VDSCRIPTCTXINT *PVDSCRIPTCTXINT;

/**
 * Check the context for type correctness.
 *
 * @returns VBox status code.
 * @param   pThis    The script context.
 */
DECLHIDDEN(int) vdScriptCtxCheck(PVDSCRIPTCTXINT pThis);

/**
 * Interprete a given function AST. The executed functions
 * must be type correct, otherwise the behavior is undefined
 * (Will assert in debug builds).
 *
 * @returns VBox status code.
 * @param   pThis    The script context.
 * @param   pszFn    The function name to interprete.
 * @param   paArgs   Arguments to pass to the function.
 * @param   cArgs    Number of arguments.
 * @param   pRet     Where to store the return value on success.
 *
 * @note: The AST is not modified in any way during the interpretation process.
 */
DECLHIDDEN(int) vdScriptCtxInterprete(PVDSCRIPTCTXINT pThis, const char *pszFn,
                                      PVDSCRIPTARG paArgs, unsigned cArgs,
                                      PVDSCRIPTARG pRet);

#endif /* !VBOX_INCLUDED_SRC_testcase_VDScriptInternal_h */
