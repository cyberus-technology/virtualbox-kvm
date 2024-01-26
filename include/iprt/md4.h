/** @file
 * IPRT - Message-Digest Algorithm 4.
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

#ifndef IPRT_INCLUDED_md4_h
#define IPRT_INCLUDED_md4_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_md4    RTMd4 - Message-Digest algorithm 4
 * @ingroup grp_rt
 *
 * @note This is just for backwards compatibility and completeness.
 *
 * @{
 */

/** Size of a MD4 hash. */
#define RTMD4_HASH_SIZE     16
/** The length of a MD4 digest string. The terminator is not included. */
#define RTMD4_DIGEST_LEN    32

/**
 * MD4 hash algorithm context.
 */
typedef union RTMD4CONTEXT
{
    uint64_t            u64BetterAlignment;
    uint8_t             abPadding[22*4 + 64 + 8];
#ifdef RT_MD4_PRIVATE_CONTEXT
    MD4_CTX             Private;
#endif
#ifdef RT_MD4_PRIVATE_ALT_CONTEXT
    RTMD4ALTPRIVATECTX  AltPrivate;
#endif
} RTMD4CONTEXT;

/** Pointer to MD4 hash algorithm context. */
typedef RTMD4CONTEXT *PRTMD4CONTEXT;


/**
 * Compute the MD4 hash of the data.
 *
 * @param   pvBuf       Pointer to data.
 * @param   cbBuf       Length of data (in bytes).
 * @param   pabDigest   Where to store the hash.
 *                      (What's passed is a pointer to the caller's buffer.)
 */
RTDECL(void) RTMd4(const void *pvBuf, size_t cbBuf, uint8_t pabDigest[RTMD4_HASH_SIZE]);

/**
 * Initialize MD4 context.
 *
 * @param   pCtx        Pointer to the MD4 context to initialize.
 */
RTDECL(void) RTMd4Init(PRTMD4CONTEXT pCtx);

/**
 * Feed data into the MD4 computation.
 *
 * @param   pCtx        Pointer to the MD4 context.
 * @param   pvBuf       Pointer to data.
 * @param   cbBuf       Length of data (in bytes).
 */
RTDECL(void) RTMd4Update(PRTMD4CONTEXT pCtx, const void *pvBuf, size_t cbBuf);

/**
 * Compute the MD4 hash of the data.
 *
 * @param   pCtx        Pointer to the MD4 context.
 * @param   pabDigest   Where to store the hash. (What's passed is a pointer to
 *                      the caller's buffer.)
 */
RTDECL(void) RTMd4Final(PRTMD4CONTEXT pCtx, uint8_t pabDigest[RTMD4_HASH_SIZE]);

/**
 * Converts a MD4 hash to a digest string.
 *
 * @returns IPRT status code.
 *
 * @param   pabDigest   The binary digest returned by RTMd4Final or RTMd4.
 * @param   pszDigest   Where to return the stringified digest.
 * @param   cchDigest   The size of the output buffer. Should be at least
 *                      RTMD4_STRING_LEN + 1 bytes.
 */
RTDECL(int) RTMd4ToString(uint8_t const pabDigest[RTMD4_HASH_SIZE], char *pszDigest, size_t cchDigest);

/**
 * Converts a MD4 hash to a digest string.
 *
 * @returns IPRT status code.
 *
 * @param   pszDigest   The stringified digest. Leading and trailing spaces are
 *                      ignored.
 * @param   pabDigest   Where to store the hash. (What is passed is a pointer to
 *                      the caller's buffer.)
 */
RTDECL(int) RTMd4FromString(char const *pszDigest, uint8_t pabDigest[RTMD4_HASH_SIZE]);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_md4_h */

