/** @file
 * IPRT - Cryptographic Keys
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

#ifndef IPRT_INCLUDED_crypto_key_h
#define IPRT_INCLUDED_crypto_key_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/crypto/x509.h>
#include <iprt/crypto/taf.h>
#include <iprt/sha.h>


RT_C_DECLS_BEGIN

struct RTCRPEMSECTION;
struct RTCRX509SUBJECTPUBLICKEYINFO;

/** @defgroup grp_rt_crkey      RTCrKey - Crypotgraphic Keys.
 * @ingroup grp_rt_crypto
 * @{
 */

/**
 * Key types.
 */
typedef enum RTCRKEYTYPE
{
    /** Invalid zero value. */
    RTCRKEYTYPE_INVALID = 0,
    /** RSA private key. */
    RTCRKEYTYPE_RSA_PRIVATE,
    /** RSA public key. */
    RTCRKEYTYPE_RSA_PUBLIC,
    /** ECDSA private key. */
    RTCRKEYTYPE_ECDSA_PRIVATE,
    /** ECDSA public key. */
    RTCRKEYTYPE_ECDSA_PUBLIC,
    /** End of key types. */
    RTCRKEYTYPE_END,
    /** The usual type size hack. */
    RTCRKEYTYPE_32BIT_HACK = 0x7fffffff
} RTCRKEYTYPE;


RTDECL(int)             RTCrKeyCreateFromSubjectPublicKeyInfo(PRTCRKEY phKey, struct RTCRX509SUBJECTPUBLICKEYINFO const *pSrc,
                                                              PRTERRINFO pErrInfo, const char *pszErrorTag);
RTDECL(int)             RTCrKeyCreateFromPublicAlgorithmAndBits(PRTCRKEY phKey, PCRTASN1OBJID pAlgorithm,
                                                                PCRTASN1DYNTYPE pParameters, PCRTASN1BITSTRING pPublicKey,
                                                                PRTERRINFO pErrInfo, const char *pszErrorTag);
RTDECL(int)             RTCrKeyCreateFromPemSection(PRTCRKEY phKey, uint32_t fFlags, struct RTCRPEMSECTION const *pSection,
                                                    const char *pszPassword, PRTERRINFO pErrInfo, const char *pszErrorTag);
RTDECL(int)             RTCrKeyCreateFromBuffer(PRTCRKEY phKey, uint32_t fFlags, void const *pvSrc, size_t cbSrc,
                                                const char *pszPassword, PRTERRINFO pErrInfo, const char *pszErrorTag);
RTDECL(int)             RTCrKeyCreateFromFile(PRTCRKEY phKey, uint32_t fFlags, const char *pszFilename,
                                              const char *pszPassword, PRTERRINFO pErrInfo);
/** @todo add support for decrypting private keys.  */
/** @name RTCRKEYFROM_F_XXX
 * @{ */
/** Only PEM sections, no binary fallback.
 * @sa RTCRPEMREADFILE_F_ONLY_PEM */
#define RTCRKEYFROM_F_ONLY_PEM                      RT_BIT(1)
/** Valid flags.   */
#define RTCRKEYFROM_F_VALID_MASK                    UINT32_C(0x00000002)
/** @} */

RTDECL(int)             RTCrKeyCreateNewRsa(PRTCRKEY phKey, uint32_t cBits, uint32_t uPubExp, uint32_t fFlags);


RTDECL(uint32_t)        RTCrKeyRetain(RTCRKEY hKey);
RTDECL(uint32_t)        RTCrKeyRelease(RTCRKEY hKey);
RTDECL(RTCRKEYTYPE)     RTCrKeyGetType(RTCRKEY hKey);
RTDECL(bool)            RTCrKeyHasPrivatePart(RTCRKEY hKey);
RTDECL(bool)            RTCrKeyHasPublicPart(RTCRKEY hKey);
RTDECL(uint32_t)        RTCrKeyGetBitCount(RTCRKEY hKey);
RTDECL(int)             RTCrKeyQueryRsaModulus(RTCRKEY hKey, PRTBIGNUM pModulus);
RTDECL(int)             RTCrKeyQueryRsaPrivateExponent(RTCRKEY hKey, PRTBIGNUM pPrivateExponent);
RTDECL(int)             RTCrKeyVerifyParameterCompatibility(RTCRKEY hKey, PCRTASN1DYNTYPE pParameters, bool fForSignature,
                                                            PCRTASN1OBJID pAlgorithm, PRTERRINFO pErrInfo);


/** Public key markers. */
extern RT_DECL_DATA_CONST(RTCRPEMMARKER const)  g_aRTCrKeyPublicMarkers[];
/** Number of entries in g_aRTCrKeyPublicMarkers. */
extern RT_DECL_DATA_CONST(uint32_t const)       g_cRTCrKeyPublicMarkers;
/** Private key markers. */
extern RT_DECL_DATA_CONST(RTCRPEMMARKER const)  g_aRTCrKeyPrivateMarkers[];
/** Number of entries in g_aRTCrKeyPrivateMarkers. */
extern RT_DECL_DATA_CONST(uint32_t const)       g_cRTCrKeyPrivateMarkers;
/** Private and public key markers. */
extern RT_DECL_DATA_CONST(RTCRPEMMARKER const)  g_aRTCrKeyAllMarkers[];
/** Number of entries in g_aRTCrKeyAllMarkers. */
extern RT_DECL_DATA_CONST(uint32_t const)       g_cRTCrKeyAllMarkers;

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_crypto_key_h */

