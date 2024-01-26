/** @file
 * IPRT - Kernel module.
 */

/*
 * Copyright (C) 2017-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_krnlmod_h
#define IPRT_INCLUDED_krnlmod_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_kmod RTKrnlMod - Kernel module/driver userspace side API.
 * @ingroup grp_rt
 * @{
 */

/**
 * Checks whether the given kernel module was loaded.
 *
 * @returns IPRT status code.
 * @param   pszName          The driver name to check.
 * @param   pfLoaded         Where to store the flag whether the module is loaded on success.
 */
RTDECL(int) RTKrnlModQueryLoaded(const char *pszName, bool *pfLoaded);

/**
 * Returns the kernel module information handle for the given loaded kernel module.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_FOUND if the kernel driver is not loaded.
 * @param   pszName          The driver name.
 * @param   phKrnlModInfo    Where to store the handle to the kernel module information record.
 */
RTDECL(int) RTKrnlModLoadedQueryInfo(const char *pszName, PRTKRNLMODINFO phKrnlModInfo);

/**
 * Returns the number of kernel modules loaded on the host system.
 *
 * @returns Number of kernel modules loaded.
 */
RTDECL(uint32_t) RTKrnlModLoadedGetCount(void);

/**
 * Returns all loaded kernel modules on the host.
 *
 * @returns IPRT status code.
 * @retval  VERR_BUFFER_OVERFLOW if there are not enough entries in the passed handle array.
 *                               The required number of entries will be returned in pcEntries.
 * @param   pahKrnlModInfo   Where to store the handles to the kernel module information records
 *                           on success.
 * @param   cEntriesMax      Maximum number of entries fitting in the given array.
 * @param   pcEntries        Where to store the number of entries used/required.
 */
RTDECL(int) RTKrnlModLoadedQueryInfoAll(PRTKRNLMODINFO pahKrnlModInfo, uint32_t cEntriesMax,
                                        uint32_t *pcEntries);

/**
 * Retains the given kernel module information record handle.
 *
 * @returns New reference count.
 * @param   hKrnlModInfo     The kernel module information record handle.
 */
RTDECL(uint32_t) RTKrnlModInfoRetain(RTKRNLMODINFO hKrnlModInfo);

/**
 * Releases the given kernel module information record handle.
 *
 * @returns New reference count, on 0 the handle is destroyed.
 * @param   hKrnlModInfo     The kernel module information record handle.
 */
RTDECL(uint32_t) RTKrnlModInfoRelease(RTKRNLMODINFO hKrnlModInfo);

/**
 * Returns the number of references held onto the kernel module by other
 * drivers or userspace clients.
 *
 * @returns Number of references held on the kernel module.
 * @param   hKrnlModInfo     The kernel module information record handle.
 */
RTDECL(uint32_t) RTKrnlModInfoGetRefCnt(RTKRNLMODINFO hKrnlModInfo);

/**
 * Returns the name of the kernel module.
 *
 * @returns Pointer to the kernel module name.
 * @param   hKrnlModInfo     The kernel module information record handle.
 */
RTDECL(const char *) RTKrnlModInfoGetName(RTKRNLMODINFO hKrnlModInfo);

/**
 * Returns the filepath of the kernel module.
 *
 * @returns Pointer to the kernel module path.
 * @param   hKrnlModInfo     The kernel module information record handle.
 */
RTDECL(const char *) RTKrnlModInfoGetFilePath(RTKRNLMODINFO hKrnlModInfo);

/**
 * Returns the size of the kernel module.
 *
 * @returns Size of the kernel module in bytes.
 * @param   hKrnlModInfo     The kernel module information record handle.
 */
RTDECL(size_t) RTKrnlModInfoGetSize(RTKRNLMODINFO hKrnlModInfo);

/**
 * Returns the load address of the kernel module.
 *
 * @returns Load address of the kernel module.
 * @param   hKrnlModInfo     The kernel module information record handle.
 */
RTDECL(RTR0UINTPTR) RTKrnlModInfoGetLoadAddr(RTKRNLMODINFO hKrnlModInfo);

/**
 * Query the kernel information record for a referencing kernel module of the
 * given record.
 *
 * @returns IPRT status code.
 * @param   hKrnlModInfo     The kernel module information record handle.
 * @param   idx              Referencing kernel module index (< reference count
 *                           as retrieved by RTKrnlModInfoGetRefCnt() ).
 * @param   phKrnlModInfoRef Where to store the handle to the referencing kernel module
 *                           information record.
 */
RTDECL(int) RTKrnlModInfoQueryRefModInfo(RTKRNLMODINFO hKrnlModInfo, uint32_t idx,
                                         PRTKRNLMODINFO phKrnlModInfoRef);

/**
 * Tries to load a kernel module by the given name.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SUPPORTED if not supported by or implemented for the platform.
 * @param   pszName         The name of the kernel module. This is highly platform
 *                          dependent.
 *
 * @note On macOS for example the name is the bundle ID.
 */
RTDECL(int) RTKrnlModLoadByName(const char *pszName);

/**
 * Tries to load a kernel module by the given file path.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SUPPORTED if not supported by or implemented for the platform.
 * @param   pszPath         The path of the kernel module.
 */
RTDECL(int) RTKrnlModLoadByPath(const char *pszPath);

/**
 * Tries to unload a kernel module by the given name.
 *
 * @returns IPRT status code.
 * @param   pszName         The name of the kernel module. This is highly platform
 *                          dependent and should be queried with RTKrnlModInfoGetName()
 *                          when checking whether the module was actually loaded.
 *
 * @note On macOS for example the name is the bundle ID.
 */
RTDECL(int) RTKrnlModUnloadByName(const char *pszName);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_krnlmod_h */

