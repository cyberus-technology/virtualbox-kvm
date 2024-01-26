/** @file
 * IPRT - Named shared memory.
 */

/*
 * Copyright (C) 2018-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_shmem_h
#define IPRT_INCLUDED_shmem_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_shmem RTShMem - Shared memory.
 * @ingroup grp_rt
 * @{
 */

/** @name Open flags for RTShMemOpen().
 * @{
 */
/** Creates a new shared memory object or opens an already existing one. */
#define RTSHMEM_O_F_CREATE            RT_BIT_32(0)
/** Creates a new shared memory object failing if one with the same name exists already. */
#define RTSHMEM_O_F_CREATE_EXCL       (RTSHMEM_O_F_CREATE | RT_BIT_32(1))
/** Opens the shared memory object for read access. */
#define RTSHMEM_O_F_READ              RT_BIT_32(2)
/** Opens the shared memory object for write access. */
#define RTSHMEM_O_F_WRITE             RT_BIT_32(3)
/** Opens the shared memory object for read and write access. */
#define RTSHMEM_O_F_READWRITE         (RTSHMEM_O_F_READ | RTSHMEM_O_F_WRITE)
/** Truncates the shared memory object to 0 bytes on open. */
#define RTSHMEM_O_F_TRUNCATE          RT_BIT_32(4)
/** Mappings may be created with executable access right (required to be known on Windows beforehand). */
#define RTSHMEM_O_F_MAYBE_EXEC        RT_BIT_32(5)
/** Mask of all valid flags. */
#define RTSHMEM_O_F_VALID_MASK        UINT32_C(0x0000003f)
/** @} */

/**
 * Creates or opens a new shared memory object with the given name.
 *
 * @returns IPRT status code.
 * @retval VERR_OUT_OF_RANGE if the mapping hint count is too big.
 * @param   phShMem         Where to store the handle to the shared memory object on success.
 * @param   pszName         Name of the shared memory object to open or create.
 * @param   fFlags          Combination of RTSHMEM_O_F_* flags.
 * @param   cbMax           Maximum number of bytes to reserve for the shared memory object.
 *                          On some platforms this can be 0 and set to another value using RTShMemSetSize() afterwards.
 *                          Giving 0 on Windows results in an error as shared memory objects there do not support
 *                          changing the size afterwards.
 * @param   cMappingsHint   Hint about the possible number of mappings created later on, set to 0 for a default value.
 */
RTDECL(int) RTShMemOpen(PRTSHMEM phShMem, const char *pszName, uint32_t fFlags, size_t cbMax, uint32_t cMappingsHint);

/**
 * Closes the given shared memory object.
 *
 * @returns IPRT status code.
 * @retval  VERR_INVALID_STATE if there is still a mapping active for the given shared memory object.
 * @param   hShMem          The shared memory object handle.
 *
 * @note The shared memory object will be deleted if the creator closes it.
 */
RTDECL(int) RTShMemClose(RTSHMEM hShMem);

/**
 * Tries to delete a shared memory object with the given name.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SUPPORTED if the platform does not support deleting the shared memory object by name.
 * @param   pszName         Name of the shared memory object to delete.
 */
RTDECL(int) RTShMemDelete(const char *pszName);

/**
 * Returns the number of references (i.e. mappings) held for the given shared memory object.
 *
 * @returns Reference count or 0 on invalid handle.
 * @param   hShMem          The shared memory object handle.
 */
RTDECL(uint32_t) RTShMemRefCount(RTSHMEM hShMem);

/**
 * Sets the size of the given shared memory object.
 *
 * @returns IPRT status code.
 * @retval  VERR_INVALID_STATE if there are mappings active for the given shared memory object.
 * @retval  VERR_NOT_SUPPORTED on some hosts which do not support changing the size after creation.
 * @param   hShMem          The shared memory object handle.
 * @param   cbMem           Size of the memory object handle in bytes.
 */
RTDECL(int) RTShMemSetSize(RTSHMEM hShMem, size_t cbMem);

/**
 * Queries the current size of the shared memory object.
 *
 * @returns IPRT status code.
 * @param   hShMem          The shared memory object handle.
 * @param   pcbMem          Where to store the size of the shared memory object on success.
 */
RTDECL(int) RTShMemQuerySize(RTSHMEM hShMem, size_t *pcbMem);

/** @name Region mapping flags for RTShMemMapRegion().
 * @{
 */
/** Read access. */
#define RTSHMEM_MAP_F_READ            RT_BIT_32(0)
/** Write access. */
#define RTSHMEM_MAP_F_WRITE           RT_BIT_32(1)
/** Execute access. */
#define RTSHMEM_MAP_F_EXEC            RT_BIT_32(2)
/** Copy on write, any write creates a new page private to the callers address space and changes
 * in that area are not shared with other processes using the hsared memory object. */
#define RTSHMEM_MAP_F_COW             RT_BIT_32(3)
/** Mask of all valid flags. */
#define RTSHMEM_MAP_F_VALID_MASK      UINT32_C(0x0000000f)
/** @} */

/**
 * Maps a region of the given shared memory object into the callers address space.
 *
 * @returns IPRT status code.
 * @retval  VERR_SHMEM_MAXIMUM_MAPPINGS_REACHED if the maximum number of mappings was reached (host dependent).
 * @retval  VERR_ACCESS_DENIED if the requested memory access rights do not line up with the flags given when opening
 *          the memory object (requesting write access for a readonly shared memory object for example).
 * @param   hShMem          The shared memory object handle.
 * @param   offRegion       Offset into the shared memory object to start mapping at.
 * @param   cbRegion        Size of the region to map.
 * @param   fFlags          Desired properties of the mapped region, combination of RTSHMEM_MAP_F_* defines.
 * @param   ppv             Where to store the start address of the mapped region on success.
 */
RTDECL(int) RTShMemMapRegion(RTSHMEM hShMem, size_t offRegion, size_t cbRegion, uint32_t fFlags, void **ppv);

/**
 * Unmaps the given region of the shared memory object.
 *
 * @returns IPRT status code.
 * @param   hShMem          The shared memory object handle.
 * @param   pv              Pointer to the mapped region obtained with RTShMemMapRegion() earlier on.
 */
RTDECL(int) RTShMemUnmapRegion(RTSHMEM hShMem, void *pv);

/** @} */
RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_shmem_h */

