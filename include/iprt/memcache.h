/** @file
 * IPRT - Memory Object Allocation Cache.
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

#ifndef IPRT_INCLUDED_memcache_h
#define IPRT_INCLUDED_memcache_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


#include <iprt/cdefs.h>
#include <iprt/types.h>

RT_C_DECLS_BEGIN


/** @defgroup grp_rt_memcache   RTMemCache - Memory Object Allocation Cache
 * @ingroup grp_rt
 *
 * Optimized allocation, initialization, freeing and destruction of memory
 * objects of the same kind and size.  Objects are constructed once, then
 * allocated and freed one or more times, until finally destructed together with
 * the cache (RTMemCacheDestroy).  It's expected behavior, even when pfnCtor is
 * NULL, that the user will be store information that should be persistent
 * across RTMemCacheFree calls.
 *
 * The objects are zeroed prior to calling pfnCtor. For obvious reasons, the
 * objects are not touched by the cache after that, so that RTMemCacheAlloc will
 * return the object in the same state as when it as handed to RTMemCacheFree.
 *
 * @todo A callback for the reuse (at alloc time) might be of interest.
 *
 * @{
 */

/** A memory cache handle. */
typedef R3R0PTRTYPE(struct RTMEMCACHEINT *)     RTMEMCACHE;
/** Pointer to a memory cache handle. */
typedef RTMEMCACHE                             *PRTMEMCACHE;
/** Nil memory cache handle. */
#define NIL_RTMEMCACHE                          ((RTMEMCACHE)0)


/**
 * Object constructor.
 *
 * This is called for when an element is allocated for the first time.
 *
 * @returns IPRT status code.
 * @param   hMemCache           The cache handle.
 * @param   pvObj               The memory object that should be initialized.
 * @param   pvUser              The user argument.
 *
 * @remarks No serialization is performed.
 */
typedef DECLCALLBACKTYPE(int, FNMEMCACHECTOR,(RTMEMCACHE hMemCache, void *pvObj, void *pvUser));
/** Pointer to an object constructor for the memory cache. */
typedef FNMEMCACHECTOR *PFNMEMCACHECTOR;

/**
 * Object destructor.
 *
 * This is called when we're shrinking or destroying the cache.
 *
 * @param   hMemCache           The cache handle.
 * @param   pvObj               The memory object that should be initialized.
 * @param   pvUser              The user argument.
 *
 * @remarks No serialization is performed.
 */
typedef DECLCALLBACKTYPE(void, FNMEMCACHEDTOR,(RTMEMCACHE hMemCache, void *pvObj, void *pvUser));
/** Pointer to an object destructor for the memory cache. */
typedef FNMEMCACHEDTOR *PFNMEMCACHEDTOR;


/**
 * Create an allocation cache for fixed size memory objects.
 *
 * @returns IPRT status code.
 * @param   phMemCache          Where to return the cache handle.
 * @param   cbObject            The size of one memory object.
 * @param   cbAlignment         The object alignment.  This must be a power of
 *                              two.  The higest alignment is 64.  If set to 0,
 *                              a sensible alignment value will be derived from
 *                              the object size.
 * @param   cMaxObjects         The maximum cache size.  Pass UINT32_MAX if unsure.
 * @param   pfnCtor             Object constructor callback.  Optional.
 * @param   pfnDtor             Object destructor callback.  Optional.
 * @param   pvUser              User argument for the two callbacks.
 * @param   fFlags              Flags reserved for future use.  Must be zero.
 */
RTDECL(int)     RTMemCacheCreate(PRTMEMCACHE phMemCache, size_t cbObject, size_t cbAlignment, uint32_t cMaxObjects,
                                 PFNMEMCACHECTOR pfnCtor, PFNMEMCACHEDTOR pfnDtor, void *pvUser, uint32_t fFlags);

/**
 * Destroy a cache destroying and freeing allocated memory.
 *
 * @returns IPRT status code.
 * @param   hMemCache       The cache handle.  NIL is quietly (VINF_SUCCESS)
 *                          ignored.
 */
RTDECL(int)     RTMemCacheDestroy(RTMEMCACHE hMemCache);

/**
 * Allocate an object.
 *
 * @returns Pointer to the allocated cache object.
 * @param   hMemCache           The cache handle.
 */
RTDECL(void *)  RTMemCacheAlloc(RTMEMCACHE hMemCache);

/**
 * Allocate an object and return a proper status code.
 *
 * @returns IPRT status code.
 * @retval  VERR_MEM_CACHE_MAX_SIZE if we've reached maximum size (see
 *          RTMemCacheCreate).
 * @retval  VERR_NO_MEMORY if we failed to allocate more memory for the cache.
 *
 * @param   hMemCache           The cache handle.
 * @param   ppvObj              Where to return the object.
 */
RTDECL(int)     RTMemCacheAllocEx(RTMEMCACHE hMemCache, void **ppvObj);

/**
 * Free an object previously returned by RTMemCacheAlloc or RTMemCacheAllocEx.
 *
 * @param   hMemCache           The cache handle.
 * @param   pvObj               The object to free.  NULL is fine.
 */
RTDECL(void)    RTMemCacheFree(RTMEMCACHE hMemCache, void *pvObj);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_memcache_h */

