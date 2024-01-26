/** @file
 * IPRT - Handle Tables.
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

#ifndef IPRT_INCLUDED_handletable_h
#define IPRT_INCLUDED_handletable_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_handletable    RTHandleTable - Handle Tables
 * @ingroup grp_rt
 * @{
 */

/**
 * Callback for retaining an object during the lookup and free calls.
 *
 * This callback is executed when a handle is being looked up in one
 * way or another from behind the handle table lock. This allows you
 * to increase the reference (or some equivalent thing) during the
 * handle lookup and thereby eliminate any race with anyone trying
 * to free the handle.
 *
 * Note that there is no counterpart to this callback, so if you make
 * use of this you'll have to release the object manually of course.
 *
 * Another use of this callback is to do some extra access checking.
 * Use the return code to indicate whether the lookup should fail
 * or not (no object is returned on faliure, naturally).
 *
 * @returns IPRT status code for the lookup (the caller won't see this).
 *
 * @param   hHandleTable    The handle table handle.
 * @param   pvObj           The object which has been looked up.
 * @param   pvCtx           The context argument if the handle table was created with the
 *                          RTHANDLETABLE_FLAGS_CONTEXT set. Otherwise NULL.
 * @param   pvUser          The user context argument specified when creating the table.
 */
typedef DECLCALLBACKTYPE(int, FNRTHANDLETABLERETAIN,(RTHANDLETABLE hHandleTable, void *pvObj, void *pvCtx, void *pvUser));
/** Pointer to a FNHANDLETABLERETAIN. */
typedef FNRTHANDLETABLERETAIN *PFNRTHANDLETABLERETAIN;

/**
 * Callback for deleting a left over object during RTHandleTableDestroy.
 *
 * @param   hHandleTable    The handle table handle.
 * @param   h               The handle.
 * @param   pvObj           The object.
 * @param   pvCtx           The context argument if the handle table was created with the
 *                          RTHANDLETABLE_FLAGS_CONTEXT set. Otherwise NULL.
 * @param   pvUser          The user context argument specified when creating the table.
 *
 */
typedef DECLCALLBACKTYPE(void, FNRTHANDLETABLEDELETE,(RTHANDLETABLE hHandleTable, uint32_t h, void *pvObj, void *pvCtx, void *pvUser));
/** Pointer to a FNRTHANDLETABLEDELETE. */
typedef FNRTHANDLETABLEDELETE *PFNRTHANDLETABLEDELETE;


/** @name RTHandleTableCreateEx flags
 * @{ */
/** Whether the handle table entries takes a context or not.
 *
 * This can be useful for associating a handle with for instance a process or
 * similar in order to prevent anyone but the owner from using the handle.
 *
 * Setting this means you will have to use the WithCtx functions to do the
 * handle management. */
#define RTHANDLETABLE_FLAGS_CONTEXT         RT_BIT_32(0)
/** Whether the handle table should take care of the serialization (IRQ unsafe).
 * If not specified the caller will have to take care of that. */
#define RTHANDLETABLE_FLAGS_LOCKED          RT_BIT_32(1)
/** Like RTHANDLETABLE_FLAGS_LOCKED, except it's IRQ safe.
 * A side-effect is that callbacks may be called with IRQs disabled.  */
#define RTHANDLETABLE_FLAGS_LOCKED_IRQ_SAFE RT_BIT_32(2)
/** The mask of valid flags. */
#define RTHANDLETABLE_FLAGS_MASK            UINT32_C(0x00000007)
/** @} */


/**
 * Creates a handle table.
 *
 * The handle table translates a 32-bit handle into an object pointer,
 * optionally calling you back so you can retain the object without
 * racing RTHandleTableFree.
 *
 * @returns IPRT status code and on success a handle table handle will be stored at the
 *          location phHandleTable points at.
 *
 * @param   phHandleTable   Where to store the handle table handle on success.
 * @param   fFlags          Flags, see RTHANDLETABLE_FLAGS_*.
 * @param   uBase           The handle base value. This is the value of the
 *                          first handle to be returned.
 * @param   cMax            The max number of handles. When exceeded the RTHandleTableAlloc
 *                          or RTHandleTableAllocWithCtx calls will fail. Note that this
 *                          number will be rounded up to a multiple of the sub-table size,
 *                          or if it's too close to UINT32_MAX it will be rounded down.
 * @param   pfnRetain       Optional retain callback that will be called from behind the
 *                          lock (if any) during lookup.
 * @param   pvUser          The user argument to the retain callback.
 */
RTDECL(int)     RTHandleTableCreateEx(PRTHANDLETABLE phHandleTable, uint32_t fFlags, uint32_t uBase, uint32_t cMax,
                                      PFNRTHANDLETABLERETAIN pfnRetain, void *pvUser);

/**
 * A simplified version of the RTHandleTableCreateEx API.
 *
 * It assumes a max of about 64K handles with 1 being the base. The table
 * access will serialized (RTHANDLETABLE_FLAGS_LOCKED).
 *
 * @returns IPRT status code and *phHandleTable.
 *
 * @param   phHandleTable   Where to store the handle table handle on success.
 */
RTDECL(int)     RTHandleTableCreate(PRTHANDLETABLE phHandleTable);

/**
 * Destroys a handle table.
 *
 * If any entries are still in used the pfnDelete callback will be invoked
 * on each of them (if specfied) to allow to you clean things up.
 *
 * @returns IPRT status code
 *
 * @param   hHandleTable    The handle to the handle table.
 * @param   pfnDelete       Function to be called back on each handle still in use. Optional.
 * @param   pvUser          The user argument to pfnDelete.
 */
RTDECL(int)     RTHandleTableDestroy(RTHANDLETABLE hHandleTable, PFNRTHANDLETABLEDELETE pfnDelete, void *pvUser);

/**
 * Allocates a handle from the handle table.
 *
 * @returns IPRT status code, almost any.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_NO_MEMORY if we failed to extend the handle table.
 * @retval  VERR_NO_MORE_HANDLES if we're out of handles.
 *
 * @param   hHandleTable    The handle to the handle table.
 * @param   pvObj           The object to associate with the new handle.
 *                          This must be aligned on a 4 byte boundary.
 * @param   ph              Where to return the handle on success.
 *
 * @remarks Do not call this if RTHANDLETABLE_FLAGS_CONTEXT was used during creation.
 */
RTDECL(int)     RTHandleTableAlloc(RTHANDLETABLE hHandleTable, void *pvObj, uint32_t *ph);

/**
 * Looks up a handle.
 *
 * @returns The object pointer on success. NULL on failure.
 *
 * @param   hHandleTable    The handle to the handle table.
 * @param   h               The handle to lookup.
 *
 * @remarks Do not call this if RTHANDLETABLE_FLAGS_CONTEXT was used during creation.
 */
RTDECL(void *)  RTHandleTableLookup(RTHANDLETABLE hHandleTable, uint32_t h);

/**
 * Looks up and frees a handle.
 *
 * @returns The object pointer on success. NULL on failure.
 *
 * @param   hHandleTable    The handle to the handle table.
 * @param   h               The handle to lookup.
 *
 * @remarks Do not call this if RTHANDLETABLE_FLAGS_CONTEXT was used during creation.
 */
RTDECL(void *)  RTHandleTableFree(RTHANDLETABLE hHandleTable, uint32_t h);

/**
 * Allocates a handle from the handle table.
 *
 * @returns IPRT status code, almost any.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_NO_MEMORY if we failed to extend the handle table.
 * @retval  VERR_NO_MORE_HANDLES if we're out of handles.
 *
 * @param   hHandleTable    The handle to the handle table.
 * @param   pvObj           The object to associate with the new handle.
 *                          This must be aligned on a 4 byte boundary.
 * @param   pvCtx           The context to associate with the new handle.
 * @param   ph              Where to return the handle on success.
 *
 * @remarks Call this if RTHANDLETABLE_FLAGS_CONTEXT was used during creation.
 */
RTDECL(int)     RTHandleTableAllocWithCtx(RTHANDLETABLE hHandleTable, void *pvObj, void *pvCtx, uint32_t *ph);

/**
 * Looks up a handle.
 *
 * @returns The object pointer on success. NULL on failure.
 *
 * @param   hHandleTable    The handle to the handle table.
 * @param   h               The handle to lookup.
 * @param   pvCtx           The handle context, this must match what was given on allocation.
 *
 * @remarks Call this if RTHANDLETABLE_FLAGS_CONTEXT was used during creation.
 */
RTDECL(void *)  RTHandleTableLookupWithCtx(RTHANDLETABLE hHandleTable, uint32_t h, void *pvCtx);

/**
 * Looks up and frees a handle.
 *
 * @returns The object pointer on success. NULL on failure.
 *
 * @param   hHandleTable    The handle to the handle table.
 * @param   h               The handle to lookup.
 * @param   pvCtx           The handle context, this must match what was given on allocation.
 *
 * @remarks Call this if RTHANDLETABLE_FLAGS_CONTEXT was used during creation.
 */
RTDECL(void *)  RTHandleTableFreeWithCtx(RTHANDLETABLE hHandleTable, uint32_t h, void *pvCtx);

/** @} */

RT_C_DECLS_END


#endif /* !IPRT_INCLUDED_handletable_h */

