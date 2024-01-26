/** @file
 * IPRT - Random Numbers and Byte Streams.
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

#ifndef IPRT_INCLUDED_rand_h
#define IPRT_INCLUDED_rand_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_rand       RTRand - Random Numbers and Byte Streams
 * @ingroup grp_rt
 * @{
 */

/**
 * Fills a buffer with random bytes.
 *
 * @param   pv  Where to store the random bytes.
 * @param   cb  Number of bytes to generate.
 */
RTDECL(void) RTRandBytes(void *pv, size_t cb) RT_NO_THROW_PROTO;

/**
 * Generate a 32-bit signed random number in the set [i32First..i32Last].
 *
 * @returns The random number.
 * @param   i32First    First number in the set.
 * @param   i32Last     Last number in the set.
 */
RTDECL(int32_t) RTRandS32Ex(int32_t i32First, int32_t i32Last) RT_NO_THROW_PROTO;

/**
 * Generate a 32-bit signed random number.
 *
 * @returns The random number.
 */
RTDECL(int32_t) RTRandS32(void) RT_NO_THROW_PROTO;

/**
 * Generate a 32-bit unsigned random number in the set [u32First..u32Last].
 *
 * @returns The random number.
 * @param   u32First    First number in the set.
 * @param   u32Last     Last number in the set.
 */
RTDECL(uint32_t) RTRandU32Ex(uint32_t u32First, uint32_t u32Last) RT_NO_THROW_PROTO;

/**
 * Generate a 32-bit unsigned random number.
 *
 * @returns The random number.
 */
RTDECL(uint32_t) RTRandU32(void) RT_NO_THROW_PROTO;

/**
 * Generate a 64-bit signed random number in the set [i64First..i64Last].
 *
 * @returns The random number.
 * @param   i64First    First number in the set.
 * @param   i64Last     Last number in the set.
 */
RTDECL(int64_t) RTRandS64Ex(int64_t i64First, int64_t i64Last) RT_NO_THROW_PROTO;

/**
 * Generate a 64-bit signed random number.
 *
 * @returns The random number.
 */
RTDECL(int64_t) RTRandS64(void) RT_NO_THROW_PROTO;

/**
 * Generate a 64-bit unsigned random number in the set [u64First..u64Last].
 *
 * @returns The random number.
 * @param   u64First    First number in the set.
 * @param   u64Last     Last number in the set.
 */
RTDECL(uint64_t) RTRandU64Ex(uint64_t u64First, uint64_t u64Last) RT_NO_THROW_PROTO;

/**
 * Generate a 64-bit unsigned random number.
 *
 * @returns The random number.
 */
RTDECL(uint64_t) RTRandU64(void) RT_NO_THROW_PROTO;


/**
 * Create an instance of the default random number generator.
 *
 * @returns IPRT status code.
 * @param   phRand      Where to return the handle to the new random number
 *                      generator.
 */
RTDECL(int) RTRandAdvCreate(PRTRAND phRand) RT_NO_THROW_PROTO;

/**
 * Create an instance of the default pseudo random number generator.
 *
 * @returns IPRT status code.
 * @param   phRand      Where to store the handle to the generator.
 */
RTDECL(int) RTRandAdvCreatePseudo(PRTRAND phRand) RT_NO_THROW_PROTO;

/**
 * Create an instance of the Park-Miller pseudo random number generator.
 *
 * @returns IPRT status code.
 * @param   phRand      Where to store the handle to the generator.
 */
RTDECL(int) RTRandAdvCreateParkMiller(PRTRAND phRand) RT_NO_THROW_PROTO;

/**
 * Create an instance of the faster random number generator for the OS.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SUPPORTED on platforms which doesn't have this feature.
 * @retval  VERR_FILE_NOT_FOUND on system where the random generator hasn't
 *          been installed or configured correctly.
 * @retval  VERR_PATH_NOT_FOUND for the same reasons as VERR_FILE_NOT_FOUND.
 *
 * @param   phRand      Where to store the handle to the generator.
 *
 * @remarks Think /dev/urandom.
 */
RTDECL(int) RTRandAdvCreateSystemFaster(PRTRAND phRand) RT_NO_THROW_PROTO;

/**
 * Create an instance of the truer random number generator for the OS.
 *
 * Don't use this unless you seriously need good random numbers because most
 * systems will have will have problems producing sufficient entropy for this
 * and you'll end up blocking while it accumulates.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SUPPORTED on platforms which doesn't have this feature.
 * @retval  VERR_FILE_NOT_FOUND on system where the random generator hasn't
 *          been installed or configured correctly.
 * @retval  VERR_PATH_NOT_FOUND for the same reasons as VERR_FILE_NOT_FOUND.
 *
 * @param   phRand      Where to store the handle to the generator.
 *
 * @remarks Think /dev/random.
 */
RTDECL(int) RTRandAdvCreateSystemTruer(PRTRAND phRand) RT_NO_THROW_PROTO;

/**
 * Destroys a random number generator.
 *
 * @returns IPRT status code.
 * @param   hRand       Handle to the random number generator.
 */
RTDECL(int) RTRandAdvDestroy(RTRAND hRand) RT_NO_THROW_PROTO;

/**
 * Generic method for seeding of a random number generator.
 *
 * The different generators may have specialized methods for
 * seeding, use one of those if you desire better control
 * over the result.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SUPPORTED if it isn't a pseudo generator.
 *
 * @param   hRand       Handle to the random number generator.
 * @param   u64Seed     Seed.
 */
RTDECL(int) RTRandAdvSeed(RTRAND hRand, uint64_t u64Seed) RT_NO_THROW_PROTO;

/**
 * Save the current state of a pseudo generator.
 *
 * This can be use to save the state so it can later be resumed at the same
 * position.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success. *pcbState contains the length of the
 *          returned string and pszState contains the state string.
 * @retval  VERR_BUFFER_OVERFLOW if the supplied buffer is too small. *pcbState
 *          will contain the necessary buffer size.
 * @retval  VERR_NOT_SUPPORTED by non-psuedo generators.
 *
 * @param   hRand       Handle to the random number generator.
 * @param   pszState    Where to store the state. The returned string will be
 *                      null terminated and printable.
 * @param   pcbState    The size of the buffer pszState points to on input, the
 *                      size required / used on return (including the
 *                      terminator, thus the 'cb' instead of 'cch').
 */
RTDECL(int) RTRandAdvSaveState(RTRAND hRand, char *pszState, size_t *pcbState) RT_NO_THROW_PROTO;

/**
 * Restores the state of a pseudo generator.
 *
 * The state must have been obtained using RTRandAdvGetState.
 *
 * @returns IPRT status code.
 * @retval  VERR_PARSE_ERROR if the state string is malformed.
 * @retval  VERR_NOT_SUPPORTED by non-psuedo generators.
 *
 * @param   hRand       Handle to the random number generator.
 * @param   pszState    The state to load.
 */
RTDECL(int) RTRandAdvRestoreState(RTRAND hRand, char const *pszState) RT_NO_THROW_PROTO;

/**
 * Fills a buffer with random bytes.
 *
 * @param   hRand       Handle to the random number generator.
 * @param   pv  Where to store the random bytes.
 * @param   cb  Number of bytes to generate.
 */
RTDECL(void) RTRandAdvBytes(RTRAND hRand, void *pv, size_t cb) RT_NO_THROW_PROTO;

/**
 * Generate a 32-bit signed random number in the set [i32First..i32Last].
 *
 * @returns The random number.
 * @param   hRand       Handle to the random number generator.
 * @param   i32First    First number in the set.
 * @param   i32Last     Last number in the set.
 */
RTDECL(int32_t) RTRandAdvS32Ex(RTRAND hRand, int32_t i32First, int32_t i32Last) RT_NO_THROW_PROTO;

/**
 * Generate a 32-bit signed random number.
 *
 * @returns The random number.
 * @param   hRand       Handle to the random number generator.
 */
RTDECL(int32_t) RTRandAdvS32(RTRAND hRand) RT_NO_THROW_PROTO;

/**
 * Generate a 32-bit unsigned random number in the set [u32First..u32Last].
 *
 * @returns The random number.
 * @param   hRand       Handle to the random number generator.
 * @param   u32First    First number in the set.
 * @param   u32Last     Last number in the set.
 */
RTDECL(uint32_t) RTRandAdvU32Ex(RTRAND hRand, uint32_t u32First, uint32_t u32Last) RT_NO_THROW_PROTO;

/**
 * Generate a 32-bit unsigned random number.
 *
 * @returns The random number.
 * @param   hRand       Handle to the random number generator.
 */
RTDECL(uint32_t) RTRandAdvU32(RTRAND hRand) RT_NO_THROW_PROTO;

/**
 * Generate a 64-bit signed random number in the set [i64First..i64Last].
 *
 * @returns The random number.
 * @param   hRand       Handle to the random number generator.
 * @param   i64First    First number in the set.
 * @param   i64Last     Last number in the set.
 */
RTDECL(int64_t) RTRandAdvS64Ex(RTRAND hRand, int64_t i64First, int64_t i64Last) RT_NO_THROW_PROTO;

/**
 * Generate a 64-bit signed random number.
 *
 * @returns The random number.
 */
RTDECL(int64_t) RTRandAdvS64(RTRAND hRand) RT_NO_THROW_PROTO;

/**
 * Generate a 64-bit unsigned random number in the set [u64First..u64Last].
 *
 * @returns The random number.
 * @param   hRand       Handle to the random number generator.
 * @param   u64First    First number in the set.
 * @param   u64Last     Last number in the set.
 */
RTDECL(uint64_t) RTRandAdvU64Ex(RTRAND hRand, uint64_t u64First, uint64_t u64Last) RT_NO_THROW_PROTO;

/**
 * Generate a 64-bit unsigned random number.
 *
 * @returns The random number.
 * @param   hRand       Handle to the random number generator.
 */
RTDECL(uint64_t) RTRandAdvU64(RTRAND hRand) RT_NO_THROW_PROTO;


/** @} */

RT_C_DECLS_END


#endif /* !IPRT_INCLUDED_rand_h */

