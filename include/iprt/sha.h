/** @file
 * IPRT - SHA1 digest creation
 */

/*
 * Copyright (C) 2009-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_sha_h
#define IPRT_INCLUDED_sha_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_sha   RTSha - SHA Family of Hash Functions
 * @ingroup grp_rt
 * @{
 */

/** The size of a SHA-1 hash. */
#define RTSHA1_HASH_SIZE    20
/** The length of a SHA-1 digest string. The terminator is not included. */
#define RTSHA1_DIGEST_LEN   40

/**
 * SHA-1 context.
 */
typedef union RTSHA1CONTEXT
{
    uint64_t                u64BetterAlignment;
    uint8_t                 abPadding[8 + (5 + 80) * 4 + 4];
#ifdef RT_SHA1_PRIVATE_CONTEXT
    SHA_CTX                 Private;
#endif
#ifdef RT_SHA1_PRIVATE_ALT_CONTEXT
    RTSHA1ALTPRIVATECTX     AltPrivate;
#endif
} RTSHA1CONTEXT;
/** Pointer to an SHA-1 context. */
typedef RTSHA1CONTEXT *PRTSHA1CONTEXT;

/**
 * Compute the SHA-1 hash of the data.
 *
 * @param   pvBuf       Pointer to the data.
 * @param   cbBuf       The amount of data (in bytes).
 * @param   pabHash     Where to store the hash. (What is passed is a pointer to
 *                      the caller's buffer.)
 */
RTDECL(void) RTSha1(const void *pvBuf, size_t cbBuf, uint8_t pabHash[RTSHA1_HASH_SIZE]);

/**
 * Computes the SHA-1 hash for the given data comparing it with the one given.
 *
 * @returns true on match, false on mismatch.
 * @param   pvBuf       Pointer to the data.
 * @param   cbBuf       The amount of data (in bytes).
 * @param   pabHash     The hash to verify. (What is passed is a pointer to the
 *                      caller's buffer.)
 */
RTDECL(bool) RTSha1Check(const void *pvBuf, size_t cbBuf, uint8_t const pabHash[RTSHA1_HASH_SIZE]);

/**
 * Initializes the SHA-1 context.
 *
 * @param   pCtx        Pointer to the SHA-1 context.
 */
RTDECL(void) RTSha1Init(PRTSHA1CONTEXT pCtx);

/**
 * Feed data into the SHA-1 computation.
 *
 * @param   pCtx        Pointer to the SHA-1 context.
 * @param   pvBuf       Pointer to the data.
 * @param   cbBuf       The length of the data (in bytes).
 */
RTDECL(void) RTSha1Update(PRTSHA1CONTEXT pCtx, const void *pvBuf, size_t cbBuf);

/**
 * Compute the SHA-1 hash of the data.
 *
 * @param   pCtx        Pointer to the SHA-1 context.
 * @param   pabHash     Where to store the hash. (What is passed is a pointer to
 *                      the caller's buffer.)
 */
RTDECL(void) RTSha1Final(PRTSHA1CONTEXT pCtx, uint8_t pabHash[RTSHA1_HASH_SIZE]);

/**
 * Converts a SHA-1 hash to a digest string.
 *
 * @returns IPRT status code.
 *
 * @param   pabHash     The binary digest returned by RTSha1Final or RTSha1.
 * @param   pszDigest   Where to return the stringified digest.
 * @param   cchDigest   The size of the output buffer. Should be at least
 *                      RTSHA1_DIGEST_LEN + 1 bytes.
 */
RTDECL(int) RTSha1ToString(uint8_t const pabHash[RTSHA1_HASH_SIZE], char *pszDigest, size_t cchDigest);

/**
 * Converts a SHA-1 hash to a digest string.
 *
 * @returns IPRT status code.
 *
 * @param   pszDigest   The stringified digest. Leading and trailing spaces are
 *                      ignored.
 * @param   pabHash     Where to store the hash. (What is passed is a pointer to
 *                      the caller's buffer.)
 */
RTDECL(int) RTSha1FromString(char const *pszDigest, uint8_t pabHash[RTSHA1_HASH_SIZE]);

/**
 * Creates a SHA1 digest for the given memory buffer.
 *
 * @returns iprt status code.
 *
 * @param   pvBuf                 Memory buffer to create a SHA1 digest for.
 * @param   cbBuf                 The amount of data (in bytes).
 * @param   ppszDigest            On success the SHA1 digest.
 * @param   pfnProgressCallback   optional callback for the progress indication
 * @param   pvUser                user defined pointer for the callback
 */
RTR3DECL(int) RTSha1Digest(void* pvBuf, size_t cbBuf, char **ppszDigest, PFNRTPROGRESS pfnProgressCallback, void *pvUser);

/**
 * Creates a SHA1 digest for the given file.
 *
 * @returns iprt status code.
 *
 * @param   pszFile               Filename to create a SHA1 digest for.
 * @param   ppszDigest            On success the SHA1 digest.
 * @param   pfnProgressCallback   optional callback for the progress indication
 * @param   pvUser                user defined pointer for the callback
 */
RTR3DECL(int) RTSha1DigestFromFile(const char *pszFile, char **ppszDigest, PFNRTPROGRESS pfnProgressCallback, void *pvUser);



/** The size of a SHA-256 hash. */
#define RTSHA256_HASH_SIZE      32
/** The length of a SHA-256 digest string. The terminator is not included. */
#define RTSHA256_DIGEST_LEN     64

/**
 * SHA-256 context.
 */
typedef union RTSHA256CONTEXT
{
    uint64_t                u64BetterAlignment;
    uint8_t                 abPadding[8 + (8 + 80) * 4];
#ifdef RT_SHA256_PRIVATE_CONTEXT
    SHA256_CTX              Private;
#endif
#ifdef RT_SHA256_PRIVATE_ALT_CONTEXT
    RTSHA256ALTPRIVATECTX   AltPrivate;
#endif
} RTSHA256CONTEXT;
/** Pointer to an SHA-256 context. */
typedef RTSHA256CONTEXT *PRTSHA256CONTEXT;

/**
 * Compute the SHA-256 hash of the data.
 *
 * @param   pvBuf       Pointer to the data.
 * @param   cbBuf       The amount of data (in bytes).
 * @param   pabHash     Where to store the hash. (What is passed is a pointer to
 *                      the caller's buffer.)
 */
RTDECL(void) RTSha256(const void *pvBuf, size_t cbBuf, uint8_t pabHash[RTSHA256_HASH_SIZE]);

/**
 * Computes the SHA-256 hash for the given data comparing it with the one given.
 *
 * @returns true on match, false on mismatch.
 * @param   pvBuf       Pointer to the data.
 * @param   cbBuf       The amount of data (in bytes).
 * @param   pabHash     The hash to verify. (What is passed is a pointer to the
 *                      caller's buffer.)
 */
RTDECL(bool) RTSha256Check(const void *pvBuf, size_t cbBuf, uint8_t const pabHash[RTSHA256_HASH_SIZE]);

/**
 * Initializes the SHA-256 context.
 *
 * @param   pCtx        Pointer to the SHA-256 context.
 */
RTDECL(void) RTSha256Init(PRTSHA256CONTEXT pCtx);

/**
 * Feed data into the SHA-256 computation.
 *
 * @param   pCtx        Pointer to the SHA-256 context.
 * @param   pvBuf       Pointer to the data.
 * @param   cbBuf       The length of the data (in bytes).
 */
RTDECL(void) RTSha256Update(PRTSHA256CONTEXT pCtx, const void *pvBuf, size_t cbBuf);

/**
 * Compute the SHA-256 hash of the data.
 *
 * @param   pCtx        Pointer to the SHA-256 context.
 * @param   pabHash     Where to store the hash. (What is passed is a pointer to
 *                      the caller's buffer.)
 */
RTDECL(void) RTSha256Final(PRTSHA256CONTEXT pCtx, uint8_t pabHash[RTSHA256_HASH_SIZE]);

/**
 * Converts a SHA-256 hash to a digest string.
 *
 * @returns IPRT status code.
 *
 * @param   pabHash     The binary digest returned by RTSha256Final or RTSha256.
 * @param   pszDigest   Where to return the stringified digest.
 * @param   cchDigest   The size of the output buffer. Should be at least
 *                      RTSHA256_DIGEST_LEN + 1 bytes.
 */
RTDECL(int) RTSha256ToString(uint8_t const pabHash[RTSHA256_HASH_SIZE], char *pszDigest, size_t cchDigest);

/**
 * Converts a SHA-256 hash to a digest string.
 *
 * @returns IPRT status code.
 *
 * @param   pszDigest   The stringified digest. Leading and trailing spaces are
 *                      ignored.
 * @param   pabHash     Where to store the hash. (What is passed is a pointer to
 *                      the caller's buffer.)
 */
RTDECL(int) RTSha256FromString(char const *pszDigest, uint8_t pabHash[RTSHA256_HASH_SIZE]);

/**
 * Creates a SHA256 digest for the given memory buffer.
 *
 * @returns iprt status code.
 *
 * @param   pvBuf                 Memory buffer to create a
 *                                SHA256 digest for.
 * @param   cbBuf                 The amount of data (in bytes).
 * @param   ppszDigest            On success the SHA256 digest.
 * @param   pfnProgressCallback   optional callback for the progress indication
 * @param   pvUser                user defined pointer for the callback
 */
RTR3DECL(int) RTSha256Digest(void* pvBuf, size_t cbBuf, char **ppszDigest, PFNRTPROGRESS pfnProgressCallback, void *pvUser);

/**
 * Creates a SHA256 digest for the given file.
 *
 * @returns iprt status code.
 *
 * @param   pszFile               Filename to create a SHA256
 *                                digest for.
 * @param   ppszDigest            On success the SHA256 digest.
 * @param   pfnProgressCallback   optional callback for the progress indication
 * @param   pvUser                user defined pointer for the callback
 */
RTR3DECL(int) RTSha256DigestFromFile(const char *pszFile, char **ppszDigest, PFNRTPROGRESS pfnProgressCallback, void *pvUser);



/** The size of a SHA-224 hash. */
#define RTSHA224_HASH_SIZE      28
/** The length of a SHA-224 digest string. The terminator is not included. */
#define RTSHA224_DIGEST_LEN     56

/** SHA-224 context (same as for SHA-256). */
typedef RTSHA256CONTEXT RTSHA224CONTEXT;
/** Pointer to an SHA-224 context. */
typedef RTSHA256CONTEXT *PRTSHA224CONTEXT;

/**
 * Compute the SHA-224 hash of the data.
 *
 * @param   pvBuf       Pointer to the data.
 * @param   cbBuf       The amount of data (in bytes).
 * @param   pabHash     Where to store the hash. (What is passed is a pointer to
 *                      the caller's buffer.)
 */
RTDECL(void) RTSha224(const void *pvBuf, size_t cbBuf, uint8_t pabHash[RTSHA224_HASH_SIZE]);

/**
 * Computes the SHA-224 hash for the given data comparing it with the one given.
 *
 * @returns true on match, false on mismatch.
 * @param   pvBuf       Pointer to the data.
 * @param   cbBuf       The amount of data (in bytes).
 * @param   pabHash     The hash to verify. (What is passed is a pointer to the
 *                      caller's buffer.)
 */
RTDECL(bool) RTSha224Check(const void *pvBuf, size_t cbBuf, uint8_t const pabHash[RTSHA224_HASH_SIZE]);

/**
 * Initializes the SHA-224 context.
 *
 * @param   pCtx        Pointer to the SHA-224 context.
 */
RTDECL(void) RTSha224Init(PRTSHA224CONTEXT pCtx);

/**
 * Feed data into the SHA-224 computation.
 *
 * @param   pCtx        Pointer to the SHA-224 context.
 * @param   pvBuf       Pointer to the data.
 * @param   cbBuf       The length of the data (in bytes).
 */
RTDECL(void) RTSha224Update(PRTSHA224CONTEXT pCtx, const void *pvBuf, size_t cbBuf);

/**
 * Compute the SHA-224 hash of the data.
 *
 * @param   pCtx        Pointer to the SHA-224 context.
 * @param   pabHash     Where to store the hash. (What is passed is a pointer to
 *                      the caller's buffer.)
 */
RTDECL(void) RTSha224Final(PRTSHA224CONTEXT pCtx, uint8_t pabHash[RTSHA224_HASH_SIZE]);

/**
 * Converts a SHA-224 hash to a digest string.
 *
 * @returns IPRT status code.
 *
 * @param   pabHash     The binary digest returned by RTSha224Final or RTSha224.
 * @param   pszDigest   Where to return the stringified digest.
 * @param   cchDigest   The size of the output buffer. Should be at least
 *                      RTSHA224_DIGEST_LEN + 1 bytes.
 */
RTDECL(int) RTSha224ToString(uint8_t const pabHash[RTSHA224_HASH_SIZE], char *pszDigest, size_t cchDigest);

/**
 * Converts a SHA-224 hash to a digest string.
 *
 * @returns IPRT status code.
 *
 * @param   pszDigest   The stringified digest. Leading and trailing spaces are
 *                      ignored.
 * @param   pabHash     Where to store the hash. (What is passed is a pointer to
 *                      the caller's buffer.)
 */
RTDECL(int) RTSha224FromString(char const *pszDigest, uint8_t pabHash[RTSHA224_HASH_SIZE]);

/**
 * Creates a SHA224 digest for the given memory buffer.
 *
 * @returns iprt status code.
 *
 * @param   pvBuf                 Memory buffer to create a SHA224 digest for.
 * @param   cbBuf                 The amount of data (in bytes).
 * @param   ppszDigest            On success the SHA224 digest.
 * @param   pfnProgressCallback   optional callback for the progress indication
 * @param   pvUser                user defined pointer for the callback
 */
RTR3DECL(int) RTSha224Digest(void* pvBuf, size_t cbBuf, char **ppszDigest, PFNRTPROGRESS pfnProgressCallback, void *pvUser);

/**
 * Creates a SHA224 digest for the given file.
 *
 * @returns iprt status code.
 *
 * @param   pszFile               Filename to create a SHA224 digest for.
 * @param   ppszDigest            On success the SHA224 digest.
 * @param   pfnProgressCallback   optional callback for the progress indication
 * @param   pvUser                user defined pointer for the callback
 */
RTR3DECL(int) RTSha224DigestFromFile(const char *pszFile, char **ppszDigest, PFNRTPROGRESS pfnProgressCallback, void *pvUser);



/** The size of a SHA-512 hash. */
#define RTSHA512_HASH_SIZE      64
/** The length of a SHA-512 digest string. The terminator is not included. */
#define RTSHA512_DIGEST_LEN     128

/**
 * SHA-512 context.
 */
typedef union RTSHA512CONTEXT
{
    uint64_t                u64BetterAlignment;
    uint8_t                 abPadding[16 + (80 + 8) * 8];
#ifdef RT_SHA512_PRIVATE_CONTEXT
    SHA512_CTX              Private;
#endif
#ifdef RT_SHA512_PRIVATE_ALT_CONTEXT
    RTSHA512ALTPRIVATECTX   AltPrivate;
#endif
} RTSHA512CONTEXT;
/** Pointer to an SHA-512 context. */
typedef RTSHA512CONTEXT *PRTSHA512CONTEXT;

/**
 * Compute the SHA-512 hash of the data.
 *
 * @param   pvBuf       Pointer to the data.
 * @param   cbBuf       The amount of data (in bytes).
 * @param   pabHash     Where to store the hash. (What is passed is a pointer to
 *                      the caller's buffer.)
 */
RTDECL(void) RTSha512(const void *pvBuf, size_t cbBuf, uint8_t pabHash[RTSHA512_HASH_SIZE]);

/**
 * Computes the SHA-512 hash for the given data comparing it with the one given.
 *
 * @returns true on match, false on mismatch.
 * @param   pvBuf       Pointer to the data.
 * @param   cbBuf       The amount of data (in bytes).
 * @param   pabHash     The hash to verify. (What is passed is a pointer to the
 *                      caller's buffer.)
 */
RTDECL(bool) RTSha512Check(const void *pvBuf, size_t cbBuf, uint8_t const pabHash[RTSHA512_HASH_SIZE]);

/**
 * Initializes the SHA-512 context.
 *
 * @param   pCtx        Pointer to the SHA-512 context.
 */
RTDECL(void) RTSha512Init(PRTSHA512CONTEXT pCtx);

/**
 * Feed data into the SHA-512 computation.
 *
 * @param   pCtx        Pointer to the SHA-512 context.
 * @param   pvBuf       Pointer to the data.
 * @param   cbBuf       The length of the data (in bytes).
 */
RTDECL(void) RTSha512Update(PRTSHA512CONTEXT pCtx, const void *pvBuf, size_t cbBuf);

/**
 * Compute the SHA-512 hash of the data.
 *
 * @param   pCtx        Pointer to the SHA-512 context.
 * @param   pabHash     Where to store the hash. (What is passed is a pointer to
 *                      the caller's buffer.)
 */
RTDECL(void) RTSha512Final(PRTSHA512CONTEXT pCtx, uint8_t pabHash[RTSHA512_HASH_SIZE]);

/**
 * Converts a SHA-512 hash to a digest string.
 *
 * @returns IPRT status code.
 *
 * @param   pabHash     The binary digest returned by RTSha512Final or RTSha512.
 * @param   pszDigest   Where to return the stringified digest.
 * @param   cchDigest   The size of the output buffer. Should be at least
 *                      RTSHA512_DIGEST_LEN + 1 bytes.
 */
RTDECL(int) RTSha512ToString(uint8_t const pabHash[RTSHA512_HASH_SIZE], char *pszDigest, size_t cchDigest);

/**
 * Converts a SHA-512 hash to a digest string.
 *
 * @returns IPRT status code.
 *
 * @param   pszDigest   The stringified digest. Leading and trailing spaces are
 *                      ignored.
 * @param   pabHash     Where to store the hash. (What is passed is a pointer to
 *                      the caller's buffer.)
 */
RTDECL(int) RTSha512FromString(char const *pszDigest, uint8_t pabHash[RTSHA512_HASH_SIZE]);


/** Macro for declaring the interface for a SHA-512 variation.
 * @internal */
#define RTSHA512_DECLARE_VARIANT(a_Name, a_UName) \
    typedef RTSHA512CONTEXT RT_CONCAT3(RTSHA,a_UName,CONTEXT); \
    typedef RTSHA512CONTEXT *RT_CONCAT3(PRTSHA,a_UName,CONTEXT); \
    RTDECL(void) RT_CONCAT(RTSha,a_Name)(const void *pvBuf, size_t cbBuf, uint8_t pabHash[RT_CONCAT3(RTSHA,a_UName,_HASH_SIZE)]); \
    RTDECL(bool) RT_CONCAT3(RTSha,a_Name,Check)(const void *pvBuf, size_t cbBuf, uint8_t const pabHash[RT_CONCAT3(RTSHA,a_UName,_HASH_SIZE)]); \
    RTDECL(void) RT_CONCAT3(RTSha,a_Name,Init)(RT_CONCAT3(PRTSHA,a_UName,CONTEXT) pCtx); \
    RTDECL(void) RT_CONCAT3(RTSha,a_Name,Update)(RT_CONCAT3(PRTSHA,a_UName,CONTEXT) pCtx, const void *pvBuf, size_t cbBuf); \
    RTDECL(void) RT_CONCAT3(RTSha,a_Name,Final)(RT_CONCAT3(PRTSHA,a_UName,CONTEXT) pCtx, uint8_t pabHash[RT_CONCAT3(RTSHA,a_UName,_HASH_SIZE)]); \
    RTDECL(int)  RT_CONCAT3(RTSha,a_Name,ToString)(uint8_t const pabHash[RT_CONCAT3(RTSHA,a_UName,_HASH_SIZE)], char *pszDigest, size_t cchDigest); \
    RTDECL(int)  RT_CONCAT3(RTSha,a_Name,FromString)(char const *pszDigest, uint8_t pabHash[RT_CONCAT3(RTSHA,a_UName,_HASH_SIZE)])


/** The size of a SHA-384 hash. */
#define RTSHA384_HASH_SIZE      48
/** The length of a SHA-384 digest string. The terminator is not included. */
#define RTSHA384_DIGEST_LEN     96
RTSHA512_DECLARE_VARIANT(384,384);

/** The size of a SHA-512/224 hash. */
#define RTSHA512T224_HASH_SIZE  28
/** The length of a SHA-512/224 digest string. The terminator is not
 *  included. */
#define RTSHA512T224_DIGEST_LEN 56
RTSHA512_DECLARE_VARIANT(512t224,512T224);

/** The size of a SHA-512/256 hash. */
#define RTSHA512T256_HASH_SIZE  32
/** The length of a SHA-512/256 digest string. The terminator is not
 *  included. */
#define RTSHA512T256_DIGEST_LEN 64
RTSHA512_DECLARE_VARIANT(512t256,512T256);


/**
 * SHA3 context.
 */
typedef union RTSHA3CONTEXT
{
    uint64_t                a64Padding[26];
    uint8_t                 abPadding[208];
#ifdef RT_SHA3_PRIVATE_CONTEXT
    RTSHA3PRIVATECTX        Private;
#endif
#ifdef RT_SHA3_PRIVATE_ALT_CONTEXT
    RTSHA3ALTPRIVATECTX     AltPrivate;
#endif
} RTSHA3CONTEXT;
/** Pointer to an SHA3 context. */
typedef RTSHA3CONTEXT *PRTSHA3CONTEXT;

/** Macro for declaring the interface for a SHA3 variation.
 *
 * @note The interface differes slightly from the older checksums:
 *          - Must call Final and/or Cleanup method.
 *          - Must use Clone instead of memcpy'ing the context.
 *          - Status codes are returned, Init may really fail.
 *
 * @internal */
#define RTSHA3_DECLARE_VARIANT(a_Bits) \
    typedef struct RT_CONCAT3(RTSHA3T,a_Bits,CONTEXT) { RTSHA3CONTEXT Sha3; } RT_CONCAT3(RTSHA3T,a_Bits,CONTEXT); \
    typedef RT_CONCAT3(RTSHA3T,a_Bits,CONTEXT) *RT_CONCAT3(PRTSHA3T,a_Bits,CONTEXT); \
    RTDECL(int)  RT_CONCAT(RTSha3t,a_Bits)(const void *pvBuf, size_t cbBuf, uint8_t pabHash[RT_CONCAT3(RTSHA3_,a_Bits,_HASH_SIZE)]); \
    RTDECL(bool) RT_CONCAT3(RTSha3t,a_Bits,Check)(const void *pvBuf, size_t cbBuf, uint8_t const pabHash[RT_CONCAT3(RTSHA3_,a_Bits,_HASH_SIZE)]); \
    RTDECL(int)  RT_CONCAT3(RTSha3t,a_Bits,Init)(RT_CONCAT3(PRTSHA3T,a_Bits,CONTEXT) pCtx); \
    RTDECL(int)  RT_CONCAT3(RTSha3t,a_Bits,Update)(RT_CONCAT3(PRTSHA3T,a_Bits,CONTEXT) pCtx, const void *pvBuf, size_t cbBuf); \
    RTDECL(int)  RT_CONCAT3(RTSha3t,a_Bits,Final)(RT_CONCAT3(PRTSHA3T,a_Bits,CONTEXT) pCtx, uint8_t pabHash[RT_CONCAT3(RTSHA3_,a_Bits,_HASH_SIZE)]); \
    RTDECL(int)  RT_CONCAT3(RTSha3t,a_Bits,Cleanup)(RT_CONCAT3(PRTSHA3T,a_Bits,CONTEXT) pCtx); \
    RTDECL(int)  RT_CONCAT3(RTSha3t,a_Bits,Clone)(RT_CONCAT3(PRTSHA3T,a_Bits,CONTEXT) pCtx, RT_CONCAT3(RTSHA3T,a_Bits,CONTEXT) const *pCtxSrc); \
    RTDECL(int)  RT_CONCAT3(RTSha3t,a_Bits,ToString)(uint8_t const pabHash[RT_CONCAT3(RTSHA3_,a_Bits,_HASH_SIZE)], char *pszDigest, size_t cchDigest); \
    RTDECL(int)  RT_CONCAT3(RTSha3t,a_Bits,FromString)(char const *pszDigest, uint8_t pabHash[RT_CONCAT3(RTSHA3_,a_Bits,_HASH_SIZE)])

/** The size of a SHA-224 hash. */
#define RTSHA3_224_HASH_SIZE     28
/** The length of a SHA-224 digest string. The terminator is not included. */
#define RTSHA3_224_DIGEST_LEN    56
RTSHA3_DECLARE_VARIANT(224);

/** The size of a SHA-256 hash. */
#define RTSHA3_256_HASH_SIZE     32
/** The length of a SHA-256 digest string. The terminator is not included. */
#define RTSHA3_256_DIGEST_LEN    64
RTSHA3_DECLARE_VARIANT(256);

/** The size of a SHA-384 hash. */
#define RTSHA3_384_HASH_SIZE     48
/** The length of a SHA-384 digest string. The terminator is not included. */
#define RTSHA3_384_DIGEST_LEN    96
RTSHA3_DECLARE_VARIANT(384);

/** The size of a SHA-512 hash. */
#define RTSHA3_512_HASH_SIZE     64
/** The length of a SHA-512 digest string. The terminator is not included. */
#define RTSHA3_512_DIGEST_LEN    128
RTSHA3_DECLARE_VARIANT(512);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_sha_h */

