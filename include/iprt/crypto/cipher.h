/** @file
 * IPRT - Crypto - Symmetric Ciphers.
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

#ifndef IPRT_INCLUDED_crypto_cipher_h
#define IPRT_INCLUDED_crypto_cipher_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/asn1.h>


RT_C_DECLS_BEGIN

struct RTCRX509SUBJECTPUBLICKEYINFO;

/** @defgroup grp_rt_crcipher RTCrCipher - Symmetric Ciphers
 * @ingroup grp_rt_crypto
 * @{
 */

/**
 * A symmetric cipher handle.
 *
 * @remarks In OpenSSL terms this corresponds to a EVP_CIPHER, while in Microsoft
 *          terms it is an algorithm handle.  The latter is why a handle was
 *          choosen rather than constant descriptor structure pointer. */
typedef struct RTCRCIPHERINT   *RTCRCIPHER;
/** Pointer to a symmetric cipher handle. */
typedef RTCRCIPHER             *PRTCRCIPHER;
/** Nil symmetric cipher handle. */
#define NIL_RTCRCIPHER          ((RTCRCIPHER)0)
/** Symmetric cipher context */
typedef struct RTCRCIPHERCTXINT *RTCRCIPHERCTX;
/** Pointer to a symmetric cipher context */
typedef RTCRCIPHERCTX          *PRTCRCIPHERCTX;
/** Nil symmetric cipher context */
#define NIL_RTCRCIPHERCTX       ((RTCRCIPHERCTX)0)

/**
 * Symmetric cipher types.
 *
 * @note Only add new types at the end, existing values must be stable.
 */
typedef enum RTCRCIPHERTYPE
{
    /** Invalid zero value. */
    RTCRCIPHERTYPE_INVALID = 0,
    /** XTS-AES-128 (NIST SP 800-38E). */
    RTCRCIPHERTYPE_XTS_AES_128,
    /** XTS-AES-256 (NIST SP 800-38E). */
    RTCRCIPHERTYPE_XTS_AES_256,
    /** GCM-AES-128. */
    RTCRCIPHERTYPE_GCM_AES_128,
    /** GCM-AES-256. */
    RTCRCIPHERTYPE_GCM_AES_256,
    /* CTR-AES-128 */
    RTCRCIPHERTYPE_CTR_AES_128,
    /* CTR-AES-256 */
    RTCRCIPHERTYPE_CTR_AES_256,
    /** End of valid symmetric cipher types. */
    RTCRCIPHERTYPE_END,
    /** Make sure the type is a 32-bit one. */
    RTCRCIPHERTYPE_32BIT_HACK = 0x7fffffff
} RTCRCIPHERTYPE;


RTDECL(int) RTCrCipherOpenByType(PRTCRCIPHER phCipher, RTCRCIPHERTYPE enmType, uint32_t fFlags);
RTDECL(uint32_t) RTCrCipherRetain(RTCRCIPHER hCipher);
RTDECL(uint32_t) RTCrCipherRelease(RTCRCIPHER hCipher);
RTDECL(uint32_t) RTCrCipherGetKeyLength(RTCRCIPHER hCipher);
RTDECL(uint32_t) RTCrCipherGetInitializationVectorLength(RTCRCIPHER hCipher);
RTDECL(uint32_t) RTCrCipherGetBlockSize(RTCRCIPHER hCipher);

RTDECL(int) RTCrCipherCtxFree(RTCRCIPHERCTX phCipherCtx);

RTDECL(int) RTCrCipherCtxEncryptInit(RTCRCIPHER hCipher, void const *pvKey, size_t cbKey,
                                     void const *pvInitVector, size_t cbInitVector,
                                     void const *pvAuthData, size_t cbAuthData,
                                     PRTCRCIPHERCTX phCipherCtx);
RTDECL(int) RTCrCipherCtxEncryptProcess(RTCRCIPHERCTX hCipherCtx, void const *pvPlainText, size_t cbPlainText,
                                        void *pvEncrypted, size_t cbEncrypted, size_t *pcbEncrypted);
RTDECL(int) RTCrCipherCtxEncryptFinish(RTCRCIPHERCTX hCipherCtx,
                                       void *pvEncrypted, size_t *pcbEncrypted,
                                       void *pvTag, size_t cbTag, size_t *pcbTag);

RTDECL(int) RTCrCipherCtxDecryptInit(RTCRCIPHER hCipher, void const *pvKey, size_t cbKey,
                                     void const *pvInitVector, size_t cbInitVector,
                                     void const *pvAuthData, size_t cbAuthData,
                                     void *pvTag, size_t cbTag, PRTCRCIPHERCTX phCipherCtx);
RTDECL(int) RTCrCipherCtxDecryptProcess(RTCRCIPHERCTX hCipherCtx,
                                        void const *pvEncrypted, size_t cbEncrypted,
                                        void *pvPlainText, size_t cbPlainText, size_t *pcbPlainText);
RTDECL(int) RTCrCipherCtxDecryptFinish(RTCRCIPHERCTX hCipherCtx,
                                       void *pvPlainText, size_t *pcbPlainText);


RTDECL(int) RTCrCipherEncrypt(RTCRCIPHER hCipher, void const *pvKey, size_t cbKey,
                              void const *pvInitVector, size_t cbInitVector,
                              void const *pvPlainText, size_t cbPlainText,
                              void *pvEncrypted, size_t cbEncrypted, size_t *pcbEncrypted);
RTDECL(int) RTCrCipherDecrypt(RTCRCIPHER hCipher, void const *pvKey, size_t cbKey,
                              void const *pvInitVector, size_t cbInitVector,
                              void const *pvEncrypted, size_t cbEncrypted,
                              void *pvPlainText, size_t cbPlainText, size_t *pcbPlainText);
RTDECL(int) RTCrCipherEncryptEx(RTCRCIPHER hCipher, void const *pvKey, size_t cbKey,
                                void const *pvInitVector, size_t cbInitVector,
                                void const *pvAuthData, size_t cbAuthData,
                                void const *pvPlainText, size_t cbPlainText,
                                void *pvEncrypted, size_t cbEncrypted, size_t *pcbEncrypted,
                                void *pvTag, size_t cbTag, size_t *pcbTag);
RTDECL(int) RTCrCipherDecryptEx(RTCRCIPHER hCipher, void const *pvKey, size_t cbKey,
                                void const *pvInitVector, size_t cbInitVector,
                                void const *pvAuthData, size_t cbAuthData,
                                void *pvTag, size_t cbTag,
                                void const *pvEncrypted, size_t cbEncrypted,
                                void *pvPlainText, size_t cbPlainText, size_t *pcbPlainText);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_crypto_cipher_h */

