/** @file
 * IPRT - Crypto - RSA Public Key Cryptosystem .
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

#ifndef IPRT_INCLUDED_crypto_rsa_h
#define IPRT_INCLUDED_crypto_rsa_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/asn1.h>
#include <iprt/crypto/x509.h>
#include <iprt/crypto/pkcs7.h>
#include <iprt/md5.h>
#include <iprt/sha.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_cr_rsa  RTCrRsa - RSA Public Key Cryptosystem
 * @ingroup grp_rt_crypto
 * @{
 */

/**
 * RSA public key - ASN.1 IPRT representation.
 */
typedef struct RTCRRSAPUBLICKEY
{
    /** Sequence core for the structure. */
    RTASN1SEQUENCECORE          SeqCore;
    /** The modulus (n). */
    RTASN1INTEGER               Modulus;
    /** The public exponent (e). */
    RTASN1INTEGER               PublicExponent;
} RTCRRSAPUBLICKEY;
/** Pointer to the ASN.1 IPRT representation of an RSA public key. */
typedef RTCRRSAPUBLICKEY *PRTCRRSAPUBLICKEY;
/** Pointer to the const ASN.1 IPRT representation of an RSA public key. */
typedef RTCRRSAPUBLICKEY const *PCRTCRRSAPUBLICKEY;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRRSAPUBLICKEY, RTDECL, RTCrRsaPublicKey, SeqCore.Asn1Core);

RTDECL(bool) RTCrRsaPublicKey_CanHandleDigestType(PCRTCRRSAPUBLICKEY pRsaPublicKey, RTDIGESTTYPE enmDigestType,
                                                  PRTERRINFO pErrInfo);


/**
 * RSA other prime info (ASN.1 IPRT representation).
 */
typedef struct RTCRRSAOTHERPRIMEINFO
{
    /** Sequence core for the structure. */
    RTASN1SEQUENCECORE          SeqCore;
    /** The prime (ri). */
    RTASN1INTEGER               Prime;
    /** The exponent (di). */
    RTASN1INTEGER               Exponent;
    /** The coefficient (ti). */
    RTASN1INTEGER               Coefficient;
} RTCRRSAOTHERPRIMEINFO;
/** Pointer to the ASN.1 IPRT representation of RSA other prime info.  */
typedef RTCRRSAOTHERPRIMEINFO *PRTCRRSAOTHERPRIMEINFO;
/** Pointer to the const ASN.1 IPRT representation of RSA other prime info. */
typedef RTCRRSAOTHERPRIMEINFO const *PCRTCRRSAOTHERPRIMEINFO;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRRSAOTHERPRIMEINFO, RTDECL, RTCrRsaOtherPrimeInfo, SeqCore.Asn1Core);
RTASN1_IMPL_GEN_SEQ_OF_TYPEDEFS_AND_PROTOS(RTCRRSAOTHERPRIMEINFOS, RTCRRSAOTHERPRIMEINFO, RTDECL, RTCrRsaOtherPrimeInfos);

/**
 * RSA private key - ASN.1 IPRT representation.
 */
typedef struct RTCRRSAPRIVATEKEY
{
    /** Sequence core for the structure. */
    RTASN1SEQUENCECORE          SeqCore;
    /** Key version number. */
    RTASN1INTEGER               Version;
    /** The modulus (n). */
    RTASN1INTEGER               Modulus;
    /** The public exponent (e). */
    RTASN1INTEGER               PublicExponent;
    /** The private exponent (d). */
    RTASN1INTEGER               PrivateExponent;
    /** The first prime factor (p) of the modulus (n). */
    RTASN1INTEGER               Prime1;
    /** The second prime factor (q) of the modulus (n). */
    RTASN1INTEGER               Prime2;
    /** The first exponent (d mod (p-1)). */
    RTASN1INTEGER               Exponent1;
    /** The second exponent (d mod (q-1)). */
    RTASN1INTEGER               Exponent2;
    /** The coefficient ((inverse of q) mod p). */
    RTASN1INTEGER               Coefficient;
    /** Optional other prime information (version must be 'multi' if present). */
    RTCRRSAOTHERPRIMEINFOS      OtherPrimeInfos;
} RTCRRSAPRIVATEKEY;
/** Pointer to the ASN.1 IPRT representation of an RSA private key. */
typedef RTCRRSAPRIVATEKEY *PRTCRRSAPRIVATEKEY;
/** Pointer to the const ASN.1 IPRT representation of an RSA private key. */
typedef RTCRRSAPRIVATEKEY const *PCRTCRRSAPRIVATEKEY;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRRSAPRIVATEKEY, RTDECL, RTCrRsaPrivateKey, SeqCore.Asn1Core);

/** @name RSA Private Key Versions
 * @{ */
#define RTCRRSAPRIVATEKEY_VERSION_TWO_PRIME     0
#define RTCRRSAPRIVATEKEY_VERSION_MULTI         1
/** @}  */

RTDECL(bool) RTCrRsaPrivateKey_CanHandleDigestType(PCRTCRRSAPRIVATEKEY pRsaPrivateKey, RTDIGESTTYPE enmDigestType,
                                                   PRTERRINFO pErrInfo);


/**
 * RSA DigestInfo used by the EMSA-PKCS1-v1_5 encoding method.
 */
typedef struct RTCRRSADIGESTINFO
{
    /** Sequence core for the structure. */
    RTASN1SEQUENCECORE          SeqCore;
    /** The digest algorithm. */
    RTCRX509ALGORITHMIDENTIFIER DigestAlgorithm;
    /** The digest. */
    RTASN1OCTETSTRING           Digest;
} RTCRRSADIGESTINFO;
/** Pointer to the ASN.1 IPRT representation of RSA digest info. */
typedef RTCRRSADIGESTINFO *PRTCRRSADIGESTINFO;
/** Pointer to the const ASN.1 IPRT representation of RSA digest info. */
typedef RTCRRSADIGESTINFO const *PCRTCRRSADIGESTINFO;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRRSADIGESTINFO, RTDECL, RTCrRsaDigestInfo, SeqCore.Asn1Core);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_crypto_rsa_h */

