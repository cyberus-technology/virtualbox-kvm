/** @file
 * IPRT - Memory Allocate for Sensitive Data.
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

#ifndef IPRT_INCLUDED_memsafer_h
#define IPRT_INCLUDED_memsafer_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/mem.h> /* RTMEM_TAG */

RT_C_DECLS_BEGIN


/** @defgroup grp_rt_memsafer   RTMemSafer - Memory Allocator for Sensitive Data
 * @ingroup grp_rt
 *
 * This API doesn't provide 100% secure storage, it only provider more secure
 * and safer storage.  Thus the API isn't called RTMemSafe because you cannot
 * assume the data is safe against all kinds of extraction methods.
 *
 * The API guarantee that the memory won't be returned to the system containing
 * any of the information you put there.  It will be repeatedly wiped after use.
 *
 * The API tries to isolate your data from other information stored in the
 * process/system.  How well this is done depends on the implementation.  The
 * more complicated implementations will provide protection against heartbleed
 * like bugs where pieces of the heap is copied onto the wire.
 *
 * The more hardened implementations of the API will also do their best to
 * prevent the memory from ending up in process dumps or being readable by
 * debuggers.
 *
 * Finally, two functions are provided for scrambling the sensitive memory while
 * it's not in use.
 *
 * @{
 */

/** @name RTMEMSAFER_F_XXX
 * @{ */
/** Require the memory to not hit the page file.
 * @remarks Makes not guarantees with regards to hibernation /
 *          suspend-to-disk. */
#define RTMEMSAFER_F_REQUIRE_NOT_PAGABLE    RT_BIT_32(0)
/** Mask of valid bits.  */
#define RTMEMSAFER_F_VALID_MASK             UINT32_C(0x00000001)
/** @} */

/**
 * Scrambles memory allocated by RTMemSaferAllocZEx and associates after use.
 *
 * Call this when the sensitive data isn't actively being used.  It will at a
 * minimum make sure the data is slightly scrambled, how hard it is to unbutton
 * is dependent on which implementation is used and available host support.
 *
 * The user must synchronize calls to RTMemSaferScramble and
 * RTMemSaferUnscramble, this memory allocator provides no help and keeps no
 * state information around.
 *
 * @returns IPRT status code.
 * @param   pv          The pointer returned by the allocation function.
 * @param   cb          The exact size given to the allocation function.
 */
RTDECL(int) RTMemSaferScramble(void *pv, size_t cb);

/**
 * Unscrambles memory allocated by RTMemSaferAllocZEx and associates before use.
 *
 * This undoes the effect of RTMemSaferScramble.
 *
 * @returns IPRT status code.
 * @param   pv          The pointer returned by the allocation function.
 * @param   cb          The exact size given to the allocation function.
 */
RTDECL(int) RTMemSaferUnscramble(void *pv, size_t cb);

/**
 * Allocates memory for sensitive data.
 *
 * Some effort will be taken to isolate the data from other memory allocation.
 * Memory is always zeroed.
 *
 * @returns IPRT status code.
 * @param   ppvNew      Where to return the pointer to the memory.
 * @param   cb          Number of bytes to allocate.
 * @param   fFlags      Flags for controlling the allocation, see
 *                      RTMEMSAFER_F_XXX.
 * @param   pszTag      Allocation tag used for statistics and such.
 */
RTDECL(int) RTMemSaferAllocZExTag(void **ppvNew, size_t cb, uint32_t fFlags, const char *pszTag) RT_NO_THROW_PROTO;

/**
 * Allocates memory for sensitive data.
 *
 * Some effort will be taken to isolate the data from other memory allocation.
 * Memory is always zeroed.
 *
 * @returns IPRT status code.
 * @param   a_ppvNew    Where to return the pointer to the memory.
 * @param   a_cb        Number of bytes to allocate.
 * @param   a_fFlags    Flags for controlling the allocation, see
 *                      RTMEMSAFER_F_XXX.
 */
#define RTMemSaferAllocZEx(a_ppvNew, a_cb, a_fFlags) RTMemSaferAllocZExTag(a_ppvNew, a_cb, a_fFlags, RTMEM_TAG)

/**
 * Allocates memory for sensitive data.
 *
 * Some effort will be taken to isolate the data from other memory allocation.
 * Memory is always zeroed.
 *
 * @returns Pointer to the allocated memory.
 * @param   cb          Number of bytes to allocate.
 * @param   pszTag      Allocation tag used for statistics and such.
 */
RTDECL(void *) RTMemSaferAllocZTag(size_t cb, const char *pszTag) RT_NO_THROW_PROTO;

/**
 * Allocates memory for sensitive data.
 *
 * Some effort will be taken to isolate the data from other memory allocation.
 * Memory is always zeroed.
 *
 * @returns Pointer to the allocated memory.
 * @param   a_cb        Number of bytes to allocate.
 */
#define RTMemSaferAllocZ(a_cb) RTMemSaferAllocZTag(a_cb, RTMEM_TAG)


/**
 * Reallocates memory allocated by RTMemSaferAllocZEx, RTMemSaferAllocZ,
 * RTMemSaferAllocZExTag, or RTMemSaferAllocZTag.
 *
 * When extending the allocation, the new memory will be zeroed.  When shrinking
 * the allocation the left over memory will be wiped clean using
 * RTMemWipeThorougly.
 *
 * The function follows the standard realloc behavior.
 *
 * @returns IPRT status code.
 * @param   cbOld       The current allocation size.
 * @param   pvOld       The current allocation.
 * @param   cbNew       The size of the new allocation.
 * @param   ppvNew      Where to return the pointer to the new memory.
 * @param   fFlags      Flags for controlling the allocation, see
 *                      RTMEMSAFER_F_XXX.  It is not permitted to drop saftely
 *                      requirments after the initial allocation.
 * @param   pszTag      Allocation tag used for statistics and such.
 */
RTDECL(int) RTMemSaferReallocZExTag(size_t cbOld, void *pvOld, size_t cbNew, void **ppvNew, uint32_t fFlags, const char *pszTag) RT_NO_THROW_PROTO;

/**
 * Reallocates memory allocated by RTMemSaferAllocZEx, RTMemSaferAllocZ,
 * RTMemSaferAllocZExTag, or RTMemSaferAllocZTag.
 *
 * When extending the allocation, the new memory will be zeroed.  When shrinking
 * the allocation the left over memory will be wiped clean using
 * RTMemWipeThorougly.
 *
 * The function follows the standard realloc behavior.
 *
 * @returns IPRT status code.
 * @param   a_cbOld     The current allocation size.
 * @param   a_pvOld     The current allocation.
 * @param   a_cbNew     The size of the new allocation.
 * @param   a_ppvNew    Where to return the pointer to the new memory.
 * @param   a_fFlags    Flags for controlling the allocation. See RTMEMSAFER_ALLOC_EX_FLAGS_* defines,
 *                      this takes only effect when allocating completely new memory, for extending or
 *                      shrinking existing allocations the flags of the allocation take precedence.
 */
#define RTMemSaferReallocZEx(a_cbOld, a_pvOld, a_cbNew, a_ppvNew, a_fFlags) \
    RTMemSaferReallocZExTag(a_cbOld, a_pvOld, a_cbNew, a_ppvNew, a_fFlags, RTMEM_TAG)

/**
 * Reallocates memory allocated by RTMemSaferAllocZ or RTMemSaferAllocZTag.
 *
 * When extending the allocation, the new memory will be zeroed.  When shrinking
 * the allocation the left over memory will be wiped clean using
 * RTMemWipeThorougly.
 *
 * The function follows the standard realloc behavior.
 *
 * @returns Pointer to the allocated memory.
 * @param   cbOld       The current allocation size.
 * @param   pvOld       The current allocation.
 * @param   cbNew       The size of the new allocation.
 * @param   pszTag      Allocation tag used for statistics and such.
 */
RTDECL(void *) RTMemSaferReallocZTag(size_t cbOld, void *pvOld, size_t cbNew, const char *pszTag) RT_NO_THROW_PROTO;

/**
 * Reallocates memory allocated by RTMemSaferAllocZ or RTMemSaferAllocZTag.
 *
 * When extending the allocation, the new memory will be zeroed.  When shrinking
 * the allocation the left over memory will be wiped clean using
 * RTMemWipeThorougly.
 *
 * The function follows the standard realloc behavior.
 *
 * @returns Pointer to the allocated memory.
 * @param   a_cbOld     The current allocation size.
 * @param   a_pvOld     The current allocation.
 * @param   a_cbNew     The size of the new allocation.
 */
#define RTMemSaferReallocZ(a_cbOld, a_pvOld, a_cbNew) RTMemSaferReallocZTag(a_cbOld, a_pvOld, a_cbNew, RTMEM_TAG)


/**
 * Frees memory allocated by RTMemSaferAllocZ* or RTMemSaferReallocZ*.
 *
 * Before freeing the allocated memory, it will be wiped clean using
 * RTMemWipeThorougly.
 *
 * @param   pv          The allocation.
 * @param   cb          The allocation size.
 */
RTDECL(void) RTMemSaferFree(void *pv, size_t cb) RT_NO_THROW_PROTO;

/**
 * Gets the amount of memory allocated at @a pv.
 *
 * This can be used to check if the allocation was made using an RTMemSafer API.
 *
 * @returns Allocation size in bytes, 0 if not a RTMemSafer allocation.
 * @param   pv          The alleged RTMemSafer allocation.
 *
 * @note    Not supported in all contexts and implementations of the API.
 */
RTDECL(size_t) RTMemSaferGetSize(void *pv) RT_NO_THROW_PROTO;


/** @}  */
RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_memsafer_h */

