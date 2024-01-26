/** @file
 *
 * VBox HDD container test utility - scripting engine.
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

#ifndef VBOX_INCLUDED_SRC_testcase_VDScript_h
#define VBOX_INCLUDED_SRC_testcase_VDScript_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/** Handle to the scripting context. */
typedef struct VDSCRIPTCTXINT *VDSCRIPTCTX;
/** Pointer to a scripting context handle. */
typedef VDSCRIPTCTX *PVDSCRIPTCTX;

/**
 * Supprted primitive types in the scripting engine.
 */
typedef enum VDSCRIPTTYPE
{
    /** Invalid type, do not use. */
    VDSCRIPTTYPE_INVALID = 0,
    /** void type, used for no return value of methods. */
    VDSCRIPTTYPE_VOID,
    /** unsigned 8bit integer. */
    VDSCRIPTTYPE_UINT8,
    VDSCRIPTTYPE_INT8,
    VDSCRIPTTYPE_UINT16,
    VDSCRIPTTYPE_INT16,
    VDSCRIPTTYPE_UINT32,
    VDSCRIPTTYPE_INT32,
    VDSCRIPTTYPE_UINT64,
    VDSCRIPTTYPE_INT64,
    VDSCRIPTTYPE_STRING,
    VDSCRIPTTYPE_BOOL,
    VDSCRIPTTYPE_POINTER,
    /** As usual, the 32bit blowup hack. */
    VDSCRIPTTYPE_32BIT_HACK = 0x7fffffff
} VDSCRIPTTYPE;
/** Pointer to a type. */
typedef VDSCRIPTTYPE *PVDSCRIPTTYPE;
/** Pointer to a const type. */
typedef const VDSCRIPTTYPE *PCVDSCRIPTTYPE;

/**
 * Script argument.
 */
typedef struct VDSCRIPTARG
{
    /** Type of the argument. */
    VDSCRIPTTYPE    enmType;
    /** Value */
    union
    {
        uint8_t     u8;
        int8_t      i8;
        uint16_t    u16;
        int16_t     i16;
        uint32_t    u32;
        int32_t     i32;
        uint64_t    u64;
        int64_t     i64;
        const char *psz;
        bool        f;
        void       *p;
    };
} VDSCRIPTARG;
/** Pointer to an argument. */
typedef VDSCRIPTARG *PVDSCRIPTARG;

/** Script callback. */
typedef DECLCALLBACKTYPE(int, FNVDSCRIPTCALLBACK,(PVDSCRIPTARG paScriptArgs, void *pvUser));
/** Pointer to a script callback. */
typedef FNVDSCRIPTCALLBACK *PFNVDSCRIPTCALLBACK;

/**
 * Callback registration structure.
 */
typedef struct VDSCRIPTCALLBACK
{
    /** The function name. */
    const char            *pszFnName;
    /** The return type of the function. */
    VDSCRIPTTYPE           enmTypeReturn;
    /** Pointer to the array of argument types. */
    PCVDSCRIPTTYPE         paArgs;
    /** Number of arguments this method takes. */
    unsigned               cArgs;
    /** The callback handler. */
    PFNVDSCRIPTCALLBACK    pfnCallback;
} VDSCRIPTCALLBACK;
/** Pointer to a callback register entry. */
typedef VDSCRIPTCALLBACK *PVDSCRIPTCALLBACK;
/** Pointer to a const callback register entry. */
typedef const VDSCRIPTCALLBACK *PCVDSCRIPTCALLBACK;

/**
 * @{
 */
/** The address space stays assigned to a variable
 * even if the pointer is casted to another type.
 */
#define VDSCRIPT_AS_FLAGS_TRANSITIVE RT_BIT(0)
/** @} */

/**
 * Address space read callback
 *
 * @returns VBox status code.
 * @param   pvUser         Opaque user data given on registration.
 * @param   Address        The address to read from, address is stored in the member for
 *                         base type given on registration.
 * @param   pvBuf          Where to store the read bits.
 * @param   cbRead         How much to read.
 */
typedef DECLCALLBACKTYPE(int, FNVDSCRIPTASREAD,(void *pvUser, VDSCRIPTARG Address, void *pvBuf, size_t cbRead));
/** Pointer to a read callback. */
typedef FNVDSCRIPTASREAD *PFNVDSCRIPTASREAD;

/**
 * Address space write callback
 *
 * @returns VBox status code.
 * @param   pvUser         Opaque user data given on registration.
 * @param   Address        The address to write to, address is stored in the member for
 *                         base type given on registration.
 * @param   pvBuf          Data to write.
 * @param   cbWrite        How much to write.
 */
typedef DECLCALLBACKTYPE(int, FNVDSCRIPTASWRITE,(void *pvUser, VDSCRIPTARG Address, const void *pvBuf, size_t cbWrite));
/** Pointer to a write callback. */
typedef FNVDSCRIPTASWRITE *PFNVDSCRIPTASWRITE;

/**
 * Create a new scripting context.
 *
 * @returns VBox status code.
 * @param   phScriptCtx    Where to store the scripting context on success.
 */
DECLHIDDEN(int) VDScriptCtxCreate(PVDSCRIPTCTX phScriptCtx);

/**
 * Destroys the given scripting context.
 *
 * @param   hScriptCtx     The script context to destroy.
 */
DECLHIDDEN(void) VDScriptCtxDestroy(VDSCRIPTCTX hScriptCtx);

/**
 * Register callbacks for the scripting context.
 *
 * @returns VBox status code.
 * @param   hScriptCtx     The script context handle.
 * @param   paCallbacks    Pointer to the callbacks to register.
 * @param   cCallbacks     Number of callbacks in the array.
 * @param   pvUser         Opaque user data to pass on the callback invocation.
 */
DECLHIDDEN(int) VDScriptCtxCallbacksRegister(VDSCRIPTCTX hScriptCtx, PCVDSCRIPTCALLBACK paCallbacks,
                                             unsigned cCallbacks, void *pvUser);

/**
 * Load a given script into the context.
 *
 * @returns VBox status code.
 * @param   hScriptCtx     The script context handle.
 * @param   pszScript      Pointer to the char buffer containing the script.
 */
DECLHIDDEN(int) VDScriptCtxLoadScript(VDSCRIPTCTX hScriptCtx, const char *pszScript);

/**
 * Execute a given method in the script context.
 *
 * @returns VBox status code.
 * @param   hScriptCtx     The script context handle.
 * @param   pszFnCall      The method to call.
 * @param   paArgs         Pointer to arguments to pass.
 * @param   cArgs          Number of arguments.
 */
DECLHIDDEN(int) VDScriptCtxCallFn(VDSCRIPTCTX hScriptCtx, const char *pszFnCall,
                                  PVDSCRIPTARG paArgs, unsigned cArgs);

/**
 * Registers a new address space provider.
 *
 * @returns VBox status code.
 * @param   hScriptCtx     The script context handle.
 * @param   pszType        The type string.
 * @param   enmBaseType    The base integer type to use for the address space.
 *                         Bool and String are not supported of course.
 * @param   pfnRead        The read callback for the registered address space.
 * @param   pfnWrite       The write callback for the registered address space.
 * @param   pvUser         Opaque user data to pass to the read and write callbacks.
 * @param   fFlags         Flags, see VDSCRIPT_AS_FLAGS_*.
 *
 * @note This will automatically register a new type with the identifier given in pszType
 *       used for the pointer. Every variable with this type is automatically treated as a pointer.
 *
 * @note If the transitive flag is set the address space stays assigned even if the pointer value
 *       is casted to another pointer type.
 *       In the following example the pointer pStruct will use the registered address space for RTGCPHYS
 *       and dereferencing the pointer causes the read/write callbacks to be triggered.
 *
 *       ...
 *       Struct *pStruct = (Struct *)(RTGCPHYS)0x12345678;
 *       pStruct->count++;
 *       ...
 */
DECLHIDDEN(int) VDScriptCtxAsRegister(VDSCRIPTCTX hScriptCtx, const char *pszType, VDSCRIPTTYPE enmBaseType,
                                      PFNVDSCRIPTASREAD pfnRead, PFNVDSCRIPTASWRITE pfnWrite, void *pvUser,
                                      uint32_t fFlags);

#endif /* !VBOX_INCLUDED_SRC_testcase_VDScript_h */
