/** @file
 * IPRT - Public Key Infrastructure APIs.
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

#ifndef IPRT_INCLUDED_crypto_pkix_h
#define IPRT_INCLUDED_crypto_pkix_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/asn1.h>


RT_C_DECLS_BEGIN

struct RTCRX509CERTIFICATE;
struct RTCRX509SUBJECTPUBLICKEYINFO;

/** @defgroup grp_rt_crpkix RTCrPkix - Public Key Infrastructure APIs
 * @ingroup grp_rt_crypto
 * @{
 */

/**
 * Verifies the signature (@a pSignatureValue) of the give data (@a pvData)
 * using the specfied public key (@a pPublicKey) and algorithm.
 *
 * @returns IPRT status code.
 * @param   pAlgorithm      The signature algorithm (digest w/ cipher).
 * @param   hPublicKey      The public key.
 * @param   pParameters     The signature parameters (not key, those are already
 *                          kept by hPublicKey).
 * @param   pSignatureValue The signature value.
 * @param   pvData          The signed data.
 * @param   cbData          The amount of signed data.
 * @param   pErrInfo        Where to return extended error info. Optional.
 *
 * @remarks Depending on the IPRT build configuration, the verficiation may be
 *          performed more than once using all available crypto implementations.
 */
RTDECL(int) RTCrPkixPubKeyVerifySignature(PCRTASN1OBJID pAlgorithm, RTCRKEY hPublicKey, PCRTASN1DYNTYPE pParameters,
                                          PCRTASN1BITSTRING pSignatureValue, const void *pvData, size_t cbData,
                                          PRTERRINFO pErrInfo);


/**
 * Verifies the signed digest (@a pvSignedDigest) against our digest (@a
 * hDigest) using the specfied public key (@a pPublicKey) and algorithm.
 *
 * @returns IPRT status code.
 * @param   pAlgorithm      The signature algorithm (digest w/ cipher).
 * @param   hPublicKey      The public key.
 * @param   pParameters     The signature parameters (not key, those are already
 *                          kept by hPublicKey).
 * @param   pvSignedDigest  The signed digest.
 * @param   cbSignedDigest  The signed digest size.
 * @param   hDigest         The digest of the data to compare @a pvSignedDigest
 *                          with.
 * @param   pErrInfo        Where to return extended error info. Optional.
 *
 * @remarks Depending on the IPRT build configuration, the verficiation may be
 *          performed more than once using all available crypto implementations.
 */
RTDECL(int) RTCrPkixPubKeyVerifySignedDigest(PCRTASN1OBJID pAlgorithm, RTCRKEY hPublicKey, PCRTASN1DYNTYPE pParameters,
                                             void const *pvSignedDigest, size_t cbSignedDigest,
                                             RTCRDIGEST hDigest, PRTERRINFO pErrInfo);

/**
 * Wrapper around RTCrPkixPubKeyVerifySignedDigest & RTCrKeyCreateFromAlgorithmAndBits.
 *
 * @note The public key info must include digest type for this to work.
 */
RTDECL(int) RTCrPkixPubKeyVerifySignedDigestByCertPubKeyInfo(struct RTCRX509SUBJECTPUBLICKEYINFO const *pCertPubKeyInfo,
                                                             void const *pvSignedDigest, size_t cbSignedDigest,
                                                             RTCRDIGEST hDigest, PRTERRINFO pErrInfo);

/**
 * Checks if the hash size can be handled by the given public key.
 */
RTDECL(bool) RTCrPkixPubKeyCanHandleDigestType(struct RTCRX509SUBJECTPUBLICKEYINFO const *pPublicKeyInfo,
                                               RTDIGESTTYPE enmDigestType, PRTERRINFO pErrInfo);

/**
 * Checks if the hash size can be handled by the given certificate's public key.
 */
RTDECL(bool) RTCrPkixCanCertHandleDigestType(struct RTCRX509CERTIFICATE const *pCertificate,
                                             RTDIGESTTYPE enmDigestType, PRTERRINFO pErrInfo);

/**
 * Signs a digest (@a hDigest) using the specified private key (@a pPrivateKey) and algorithm.
 *
 * @returns IPRT status code.
 * @param   pAlgorithm      The signature algorithm (digest w/ cipher).
 * @param   hPrivateKey     Handle to the private key to use.
 * @param   pParameters     Parameter to the public key algorithm. Optional.
 * @param   hDigest         The digest of the data being signed.
 * @param   fFlags          Flags for future extensions, MBZ.
 * @param   pvSignature     The output signature buffer.  Pass NULL to query
 *                          the signature size.
 * @param   pcbSignature    On input the variable pointed to holds the size of
 *                          the buffer @a pvSignature points to.
 *                          On return the variable pointed to is set to the size
 *                          of the returned signature, or the required size in
 *                          case of VERR_BUFFER_OVERFLOW.
 * @param   pErrInfo        Where to return extended error info. Optional.
 *
 * @remarks Depending on the IPRT build configuration and the algorithm used, the
 *          signing may be performed more than once using all available crypto
 *          implementations.
 */
RTDECL(int) RTCrPkixPubKeySignDigest(PCRTASN1OBJID pAlgorithm, RTCRKEY hPrivateKey, PCRTASN1DYNTYPE pParameters,
                                     RTCRDIGEST hDigest, uint32_t fFlags,
                                     void *pvSignature, size_t *pcbSignature, PRTERRINFO pErrInfo);

/**
 * Gets the cipher OID matching the given signature algorithm.
 *
 * @returns Cipher OID string on success, NULL on failure.
 * @param   pAlgorithm          The signature algorithm (hash function w/ cipher).
 * @sa      RTCrX509AlgorithmIdentifier_GetEncryptionOid,
 *          RTCrX509AlgorithmIdentifier_GetEncryptionOidFromOid
 */
RTDECL(const char *) RTCrPkixGetCiperOidFromSignatureAlgorithm(PCRTASN1OBJID pAlgorithm);

/**
 * Gets the cipher OID matching the given signature algorithm OID.
 *
 * @returns Cipher OID string on success, NULL on failure.
 * @param   pszSignatureOid     The signature algorithm ID (hash function w/ cipher).
 * @sa      RTCrX509AlgorithmIdentifier_GetEncryptionOid,
 *          RTCrX509AlgorithmIdentifier_GetEncryptionOidFromOid
 */
RTDECL(const char *) RTCrPkixGetCiperOidFromSignatureAlgorithmOid(const char *pszSignatureOid);


/** @name PKCS-1 Object Identifiers (OIDs)
 * @{ */
#define RTCR_PKCS1_OID                              "1.2.840.113549.1.1"
#define RTCR_PKCS1_RSA_OID                          "1.2.840.113549.1.1.1"
#define RTCR_PKCS1_MD2_WITH_RSA_OID                 "1.2.840.113549.1.1.2"
#define RTCR_PKCS1_MD4_WITH_RSA_OID                 "1.2.840.113549.1.1.3"
#define RTCR_PKCS1_MD5_WITH_RSA_OID                 "1.2.840.113549.1.1.4"
#define RTCR_PKCS1_SHA1_WITH_RSA_OID                "1.2.840.113549.1.1.5"
#define RTCR_PKCS1_RSA_OAEP_ENCRYPTION_SET_OID      "1.2.840.113549.1.1.6"
#define RTCR_PKCS1_RSA_AES_OAEP_OID                 "1.2.840.113549.1.1.7"
#define RTCR_PKCS1_MSGF1_OID                        "1.2.840.113549.1.1.8"
#define RTCR_PKCS1_P_SPECIFIED_OID                  "1.2.840.113549.1.1.9"
#define RTCR_PKCS1_RSASSA_PSS_OID                   "1.2.840.113549.1.1.10"
#define RTCR_PKCS1_SHA256_WITH_RSA_OID              "1.2.840.113549.1.1.11"
#define RTCR_PKCS1_SHA384_WITH_RSA_OID              "1.2.840.113549.1.1.12"
#define RTCR_PKCS1_SHA512_WITH_RSA_OID              "1.2.840.113549.1.1.13"
#define RTCR_PKCS1_SHA224_WITH_RSA_OID              "1.2.840.113549.1.1.14"
#define RTCR_PKCS1_SHA512T224_WITH_RSA_OID          "1.2.840.113549.1.1.15"
#define RTCR_PKCS1_SHA512T256_WITH_RSA_OID          "1.2.840.113549.1.1.16"
/** @} */

/** @name ANSI X9.62 Object Identifiers (OIDs)
 * @{ */
#define RTCR_X962_ECDSA_OID                         "1.2.840.10045.2.1"
#define RTCR_X962_ECDSA_WITH_SHA1_OID               "1.2.840.10045.4.1"
#define RTCR_X962_ECDSA_WITH_SHA2_OID               "1.2.840.10045.4.3"
#define RTCR_X962_ECDSA_WITH_SHA224_OID             "1.2.840.10045.4.3.1"
#define RTCR_X962_ECDSA_WITH_SHA256_OID             "1.2.840.10045.4.3.2"
#define RTCR_X962_ECDSA_WITH_SHA384_OID             "1.2.840.10045.4.3.3"
#define RTCR_X962_ECDSA_WITH_SHA512_OID             "1.2.840.10045.4.3.4"
/** @}  */

/** @name NIST Object Identifiers (OIDs)
 * @{ */
#define RTCR_NIST_ALGORITHM_OID                     "2.16.840.1.101.3.4"
#define RTCR_NIST_HASH_ALGS_OID                     "2.16.840.1.101.3.4.2"
#define RTCR_NIST_SIG_ALGS_OID                      "2.16.840.1.101.3.4.3"
#define RTCR_NIST_SHA3_224_WITH_ECDSA_OID           "2.16.840.1.101.3.4.3.9"
#define RTCR_NIST_SHA3_256_WITH_ECDSA_OID           "2.16.840.1.101.3.4.3.10"
#define RTCR_NIST_SHA3_384_WITH_ECDSA_OID           "2.16.840.1.101.3.4.3.11"
#define RTCR_NIST_SHA3_512_WITH_ECDSA_OID           "2.16.840.1.101.3.4.3.12"
#define RTCR_NIST_SHA3_224_WITH_RSA_OID             "2.16.840.1.101.3.4.3.13"
#define RTCR_NIST_SHA3_256_WITH_RSA_OID             "2.16.840.1.101.3.4.3.14"
#define RTCR_NIST_SHA3_384_WITH_RSA_OID             "2.16.840.1.101.3.4.3.15"
#define RTCR_NIST_SHA3_512_WITH_RSA_OID             "2.16.840.1.101.3.4.3.16"
/** @}  */


/**
 * Public key signature scheme provider descriptor.
 */
typedef struct RTCRPKIXSIGNATUREDESC
{
    /** The signature scheme provider name. */
    const char         *pszName;
    /** The object ID string. */
    const char         *pszObjId;
    /** Pointer to a NULL terminated table of alias object IDs (optional). */
    const char * const *papszObjIdAliases;
    /** The size of the state. */
    uint32_t            cbState;
    /** Reserved for future / explicit padding. */
    uint32_t            uReserved;
    /** Provider specific field.  This generally indicates the kind of padding
     *  scheme to employ with the given OID. */
    uintptr_t           uProviderSpecific;

    /**
     * Initializes the state of the signature scheme provider.
     *
     * Optional, RT_BZERO will be used if NULL.
     *
     * @returns IPRT status code.
     * @param   pDesc           Pointer to this structure (for uProviderSpecific).
     * @param   pvState         The opaque provider state.
     * @param   pvOpaque        Opaque provider specific parameter.
     * @param   fSigning        Set if a signing operation is going to be performed,
     *                          clear if it is a verification.  This is a fixed
     *                          setting for the lifetime of the instance due to the
     *                          algorithm requiring different keys.
     * @param   hKey            The key handle.  Caller has retained it for the
     *                          lifetime of the state being initialize.
     * @param   pParams         Algorithm/key parameters, optional.  Will be NULL if
     *                          none.
     */
    DECLCALLBACKMEMBER(int, pfnInit,(struct RTCRPKIXSIGNATUREDESC const *pDesc, void *pvState, void *pvOpaque, bool fSigning,
                                     RTCRKEY hKey, PCRTASN1DYNTYPE pParams));

    /**
     * Resets the state before performing another signing or verification.
     *
     * Optional.  It is assumed that the provider does not have any state needing to
     * be re-initialized if this method is not implemented.
     *
     * @returns IPRT status code.
     * @param   pDesc           Pointer to this structure (for uProviderSpecific).
     * @param   pvState         The opaque provider state.
     * @param   fSigning        Exactly the same value as the init call.
     */
    DECLCALLBACKMEMBER(int, pfnReset,(struct RTCRPKIXSIGNATUREDESC const *pDesc, void *pvState, bool fSigning));

    /**
     * Deletes the provider state. Optional.
     *
     * The state will be securely wiped clean after the call, regardless of whether
     * the method is implemented or not.
     *
     * @param   pDesc           Pointer to this structure (for uProviderSpecific).
     * @param   pvState         The opaque provider state.
     * @param   fSigning        Exactly the same value as the init call.
     */
    DECLCALLBACKMEMBER(void, pfnDelete,(struct RTCRPKIXSIGNATUREDESC const *pDesc, void *pvState, bool fSigning));

    /**
     * Verifies a signed message digest (fSigning = false).
     *
     * @returns IPRT status code.
     * @retval  VINF_SUCCESS if the signature checked out correctly.
     * @retval  VINF_CR_DIGEST_DEPRECATED if the signature checked out correctly
     *          but the hash algorithm is deprecated.
     * @retval  VINF_CR_DIGEST_COMPROMISED if the signature checked out correctly
     *          but the hash algorithm is compromised.
     * @retval  VINF_CR_DIGEST_SEVERELY_COMPROMISED if the signature checked out
     *          correctly but the hash algorithm is severely compromised.
     * @retval  VERR_PKIX_KEY wrong key or some other key issue.
     *
     * @param   pDesc           Pointer to this structure (for uProviderSpecific).
     * @param   pvState         The opaque provider state.
     * @param   hKey            The key handle associated with the state at init.
     * @param   hDigest         The handle to the digest.  Calls RTCrDigestFinal to
     *                          complete and retreive the final hash value.
     * @param   pvSignature     The signature to validate.
     * @param   cbSignature     The size of the signature (in bytes).
     */
    DECLCALLBACKMEMBER(int, pfnVerify,(struct RTCRPKIXSIGNATUREDESC const *pDesc, void *pvState, RTCRKEY hKey,
                                       RTCRDIGEST hDigest, void const *pvSignature, size_t cbSignature));

    /**
     * Sign a message digest (fSigning = true).
     *
     * @returns IPRT status code.
     * @retval  VINF_SUCCESS on success.
     * @retval  VINF_CR_DIGEST_DEPRECATED on success but the hash algorithm is deprecated.
     * @retval  VINF_CR_DIGEST_COMPROMISED on success but the hash algorithm is compromised.
     * @retval  VINF_CR_DIGEST_SEVERELY_COMPROMISED on success but the hash algorithm
     *          is severely compromised.
     * @retval  VERR_PKIX_KEY wrong key or some other key issue.
     * @retval  VERR_BUFFER_OVERFLOW if the signature buffer is too small, the
     *          require buffer size will be available in @a *pcbSignature.
     *
     * @param   pDesc           Pointer to this structure (for uProviderSpecific).
     * @param   pvState         The opaque provider state.
     * @param   hKey            The key handle associated with the state at init.
     * @param   hDigest         The handle to the digest.  Calls RTCrDigestFinal to
     *                          complete and retreive the final hash value.
     * @param   pvSignature     The output signature buffer.
     * @param   pcbSignature    On input the variable pointed to holds the size of
     *                          the buffer @a pvSignature points to.
     *                          On return the variable pointed to is set to the size
     *                          of the returned signature, or the required size in
     *                          case of VERR_BUFFER_OVERFLOW.
     */
    DECLCALLBACKMEMBER(int, pfnSign,(struct RTCRPKIXSIGNATUREDESC const *pDesc, void *pvState, RTCRKEY hKey,
                                     RTCRDIGEST hDigest, void *pvSignature, size_t *pcbSignature));

} RTCRPKIXSIGNATUREDESC;
/** Pointer to a public key signature scheme provider descriptor. */
typedef RTCRPKIXSIGNATUREDESC const *PCRTCRPKIXSIGNATUREDESC;

/**
 * Locates a signature schema provider descriptor by object ID string.
 * @returns Pointer to descriptor on success, NULL on if not found.
 * @param   pszObjId    The ID of the signature to search for.
 * @param   ppvOpaque   Where to store an opaque schema parameter. Optional.
 */
PCRTCRPKIXSIGNATUREDESC RTCrPkixSignatureFindByObjIdString(const char *pszObjId, void **ppvOpaque);

/**
 * Locates a signature schema provider descriptor by ASN.1 object ID.
 * @returns Pointer to descriptor on success, NULL on if not found.
 * @param   pObjId      The ID of the signature to search for.
 * @param   ppvOpaque   Where to store an opaque schema parameter. Optional.
 */
PCRTCRPKIXSIGNATUREDESC RTCrPkixSignatureFindByObjId(PCRTASN1OBJID pObjId, void **ppvOpaque);

/**
 * Create a signature schema provier instance.
 *
 * @returns IPRT status code.
 * @param   phSignature Where to return the handle to the created instance.
 * @param   pDesc       The signature schema provider descriptor.  Use
 *                      RTCrPkixSignatureFindByObjIdString() or RTCrPkixSignatureFindByObjId()
 *                      to get this.
 * @param   pvOpaque    The opaque schema parameter returned by the find functions.
 * @param   fSigning    Set if the intention is to sign stuff, clear if verification only.
 * @param   hKey        The key handle.  A referenced will be retained.
 * @param   pParams     Algorithm/key parameters, optional.
 */
RTDECL(int) RTCrPkixSignatureCreate(PRTCRPKIXSIGNATURE phSignature, PCRTCRPKIXSIGNATUREDESC pDesc, void *pvOpaque,
                                    bool fSigning, RTCRKEY hKey, PCRTASN1DYNTYPE pParams);
/** Convenience wrapper function for RTCrPkixSignatureCreate(). */
RTDECL(int) RTCrPkixSignatureCreateByObjIdString(PRTCRPKIXSIGNATURE phSignature, const char *pszObjId,
                                                 RTCRKEY hKey, PCRTASN1DYNTYPE pParams, bool fSigning);
/** Convenience wrapper function for RTCrPkixSignatureCreate(). */
RTDECL(int) RTCrPkixSignatureCreateByObjId(PRTCRPKIXSIGNATURE phSignature, PCRTASN1OBJID pObjId, RTCRKEY hKey,
                                           PCRTASN1DYNTYPE pParams, bool fSigning);

/**
 * Retains a reference to the signature schema provider instance.
 *
 * @returns New reference count on success, UINT32_MAX if invalid handle.
 * @param   hSignature      The signature schema provider handle.
 */
RTDECL(uint32_t) RTCrPkixSignatureRetain(RTCRPKIXSIGNATURE hSignature);

/**
 * Releases a reference to the signature schema provider instance.
 *
 * @returns New reference count on success, UINT32_MAX if invalid handle.
 * @param   hSignature      The signature schema provider handle.  NIL is ignored.
 */
RTDECL(uint32_t) RTCrPkixSignatureRelease(RTCRPKIXSIGNATURE hSignature);

/**
 * Verifies a signed message digest.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS if the signature checked out correctly.
 * @retval  VINF_CR_DIGEST_DEPRECATED if the signature checked out correctly
 *          but the hash algorithm is deprecated.
 * @retval  VINF_CR_DIGEST_COMPROMISED if the signature checked out correctly
 *          but the hash algorithm is compromised.
 * @retval  VINF_CR_DIGEST_SEVERELY_COMPROMISED if the signature checked out
 *          correctly but the hash algorithm is severely compromised.
 * @retval  VERR_PKIX_KEY wrong key or some other key issue.
 *
 * @param   hSignature      The signature schema provider handle.
 * @param   hDigest         The handle to the digest.  All that must have been
 *                          feed to it via RTCrDigestUpdate() and friends prior
 *                          to calling this function.  The function will itself
 *                          call RTCrDigestFinal() to complete and retreive the
 *                          final hash value.
 * @param   pvSignature     The signature to validate.
 * @param   cbSignature     The size of the signature (in bytes).
 */
RTDECL(int) RTCrPkixSignatureVerify(RTCRPKIXSIGNATURE hSignature, RTCRDIGEST hDigest, void const *pvSignature, size_t cbSignature);
/** Convenience wrapper function for RTCrPkixSignatureVerify(). */
RTDECL(int) RTCrPkixSignatureVerifyBitString(RTCRPKIXSIGNATURE hSignature, RTCRDIGEST hDigest, PCRTASN1BITSTRING pSignature);
/** Convenience wrapper function for RTCrPkixSignatureVerify(). */
RTDECL(int) RTCrPkixSignatureVerifyOctetString(RTCRPKIXSIGNATURE hSignature, RTCRDIGEST hDigest, PCRTASN1OCTETSTRING pSignature);

/**
 * Sign a message digest.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VINF_CR_DIGEST_DEPRECATED on success but the hash algorithm is deprecated.
 * @retval  VINF_CR_DIGEST_COMPROMISED on success but the hash algorithm is compromised.
 * @retval  VINF_CR_DIGEST_SEVERELY_COMPROMISED on success but the hash algorithm
 *          is severely compromised.
 * @retval  VERR_PKIX_KEY wrong key or some other key issue.
 * @retval  VERR_BUFFER_OVERFLOW if the signature buffer is too small, the
 *          require buffer size will be available in @a *pcbSignature.
 *
 * @param   hSignature      The signature schema provider handle.
 * @param   hDigest         The handle to the digest.  All that must have been
 *                          feed to it via RTCrDigestUpdate() and friends prior
 *                          to calling this function.  The function will itself
 *                          call RTCrDigestFinal() to complete and retreive the
 *                          final hash value.
 * @param   pvSignature     The output signature buffer.
 * @param   pcbSignature    On input the variable pointed to holds the size of
 *                          the buffer @a pvSignature points to.
 *                          On return the variable pointed to is set to the size
 *                          of the returned signature, or the required size in
 *                          case of VERR_BUFFER_OVERFLOW.
 */
RTDECL(int) RTCrPkixSignatureSign(RTCRPKIXSIGNATURE hSignature, RTCRDIGEST hDigest, void *pvSignature, size_t *pcbSignature);


/**
 * Public key encryption scheme provider descriptor.
 *
 * @todo This is just a sketch left over from when the signature code was
 *       chiseled out.
 */
typedef struct RTCRPKIXENCRYPTIONDESC
{
    /** The encryption scheme provider name. */
    const char         *pszName;
    /** The object ID string. */
    const char         *pszObjId;
    /** Pointer to a NULL terminated table of alias object IDs (optional). */
    const char * const *papszObjIdAliases;
    /** The size of the state. */
    uint32_t            cbState;
    /** Reserved for future use / padding.  */
    uint32_t            uReserved;
    /** Provider specific field. */
    uintptr_t           uProviderSpecific;

    /**
     * Initializes the state for this encryption scheme.
     *
     * Optional, RT_BZERO will be used if NULL.
     *
     * @returns IPRT status code.
     * @param   pDesc           Pointer to this structure (so uProviderSpecific can
     *                          be read).
     * @param   pvState         The opaque provider state.
     * @param   pvOpaque        Opaque provider specific parameter.
     * @param   fEncrypt        Set if the instance will be encrypting, clear if it
     *                          will be decrypting.  This aspect of the instance is
     *                          immutable due to the algorithm requiring different
     *                          keys for each of the operations.
     * @param   pKey            The key to use (whether private or public depends on
     *                          the operation type).
     * @param   pParams         Algorithm/key parameters, optional.  Will be NULL if
     *                          none.
     */
    DECLCALLBACKMEMBER(int, pfnInit,(struct RTCRPKIXENCRYPTIONDESC const *pDesc, void *pvState, void *pvOpaque, bool fEncrypt,
                                     PCRTASN1BITSTRING pKey, PCRTASN1DYNTYPE pParams));

    /**
     * Re-initializes the provider state.
     *
     * Optional.  It is assumed that the provider does not have any state needing
     * to be re-initialized if this method is not implemented.  (Do not assume that
     * a final encrypt/decrypt call has been made prior to this call.)
     *
     * @returns IPRT status code.
     * @param   pDesc           Pointer to this structure (so uProviderSpecific can
     *                          be read).
     * @param   pvState         The opaque provider state.
     * @param   enmOperation    Same as for the earlier pfnInit call.
     */
    DECLCALLBACKMEMBER(int, pfnReset,(struct RTCRPKIXENCRYPTIONDESC const *pDesc, void *pvState, bool fEncrypt));

    /**
     * Deletes the provider state. Optional.
     *
     * The state will be securely wiped clean after the call, regardless of whether
     * the method is implemented or not.
     *
     * @param   pDesc           Pointer to this structure (so uProviderSpecific can
     *                          be read).
     * @param   pvState         The opaque provider state.
     * @param   enmOperation    Same as for the earlier pfnInit call.
     */
    DECLCALLBACKMEMBER(void, pfnDelete,(struct RTCRPKIXENCRYPTIONDESC const *pDesc, void *pvState, bool fEncrypt));

    /**
     * Encrypt using the public key (fEncrypt = true).
     *
     * @returns IPRT status code.
     * @retval  VINF_SUCCESS on success.
     * @retval  VERR_PKIX_KEY wrong key or some other key issue.
     * @retval  VERR_BUFFER_OVERFLOW if the output buffer is too small, the require
     *          buffer size will be available in @a *pcbCiphertext.  The caller can
     *          should retry the call with a larger buffer.
     *
     * @param   pDesc           Pointer to this structure (so uProviderSpecific can
     *                          be read).
     * @param   pvState         The opaque provider state.
     * @param   pvPlaintext     The plaintext to encrypt.
     * @param   cbPlaintext     The number of bytes of plaintext.
     * @param   pvCiphertext    Where to return the ciphertext (if any).
     * @param   cbMaxCiphertext The size of the buffer @a pvCiphertext points to.
     * @param   pcbCiphertext   Where to return the actual number of bytes of
     *                          ciphertext returned.
     * @param   fFinal          Whether this is the final call.
     */
    DECLCALLBACKMEMBER(int, pfnEncrypt,(struct RTCRPKIXENCRYPTIONDESC const *pDesc, void *pvState,
                                        void const *pvPlaintext, size_t cbPlaintext,
                                        void *pvCiphertext, size_t cbMaxCiphertext, size_t *pcbCiphertext, bool fFinal));

    /**
     * Calculate the output buffer size for the next pfnEncrypt call.
     *
     * @returns IPRT status code.
     * @param   pDesc           Pointer to this structure (so uProviderSpecific can
     *                          be read).
     * @param   pvState         The opaque provider state.
     * @param   cbPlaintext     The number of bytes of plaintext.
     * @param   pcbCiphertext   Where to return the minimum buffer size.  This may
     *                          be larger than the actual number of bytes return.
     * @param   fFinal          Whether this is the final call.
     */
    DECLCALLBACKMEMBER(int, pfnEncryptLength,(struct RTCRPKIXENCRYPTIONDESC const *pDesc, void *pvState,
                                              size_t cbPlaintext, size_t *pcbCiphertext, bool fFinal));

    /**
     * Decrypt using the private key (fEncrypt = false).
     *
     * @returns IPRT status code.
     * @retval  VINF_SUCCESS on success.
     * @retval  VERR_PKIX_KEY wrong key or some other key issue.
     * @retval  VERR_BUFFER_OVERFLOW if the output buffer is too small, the require
     *          buffer size will be available in @a *pcbCiphertext.  The caller can
     *          should retry the call with a larger buffer.
     *
     * @param   pDesc           Pointer to this structure (so uProviderSpecific can
     *                          be read).
     * @param   pvState         The opaque provider state.
     * @param   pvCiphertext    The ciphertext to decrypt.
     * @param   cbCiphertext    The number of bytes of ciphertext.
     * @param   pvPlaintext     Where to return the plaintext (if any).
     * @param   cbMaxPlaintext  The size of the buffer @a pvPlaintext points to.
     * @param   pcbPlaintext    Where to return the actual number of bytes of
     *                          plaintext returned.
     * @param   fFinal          Whether this is the final call.
     */
    DECLCALLBACKMEMBER(int, pfnDecrypt,(struct RTCRPKIXENCRYPTIONDESC const *pDesc, void *pvState,
                                        void const *pvCiphertext, size_t cbCiphertext,
                                        void *pvPlaintext, size_t cbMaxPlaintext, size_t *pcbPlaintext, bool fFinal));

    /**
     * Calculate the output buffer size for the next pfnDecrypt call.
     *
     * @returns IPRT status code.
     * @param   pDesc           Pointer to this structure (so uProviderSpecific can
     *                          be read).
     * @param   pvState         The opaque provider state.
     * @param   cbCiphertext    The number of bytes of ciphertext.
     * @param   pcbPlaintext    Where to return the minimum buffer size.  This may
     *                          be larger than the actual number of bytes return.
     * @param   fFinal          Whether this is the final call.
     */
    DECLCALLBACKMEMBER(int, pfnDecryptLength,(struct RTCRPKIXENCRYPTIONDESC const *pDesc, void *pvState,
                                              size_t cbCiphertext, size_t *pcbPlaintext, bool fFinal));
} RTCRPKIXENCRYPTIONDESC;
/** Pointer to a public key encryption schema provider descriptor. */
typedef RTCRPKIXENCRYPTIONDESC const *PCRTCRPKIXENCRYPTIONDESC;


PCRTCRPKIXENCRYPTIONDESC RTCrPkixEncryptionFindByObjIdString(const char *pszObjId, void *ppvOpaque);
PCRTCRPKIXENCRYPTIONDESC RTCrPkixEncryptionFindByObjId(PCRTASN1OBJID pObjId, void *ppvOpaque);
RTDECL(int) RTCrPkixEncryptionCreateByObjIdString(PRTCRPKIXENCRYPTION phEncryption, const char *pszObjId,
                                                  bool fEncrypt, RTCRKEY hKey, PCRTASN1DYNTYPE pParams);
RTDECL(int) RTCrPkixEncryptionCreateByObjId(PRTCRPKIXENCRYPTION phEncryption, PCRTASN1OBJID pObjId, bool fEncrypt,
                                            RTCRKEY hKey, PCRTASN1DYNTYPE pParams);


RTDECL(int) RTCrPkixEncryptionCreate(PRTCRPKIXENCRYPTION phEncryption, PCRTCRPKIXENCRYPTIONDESC pDesc, void *pvOpaque,
                                     bool fEncrypt, PCRTASN1BITSTRING pKey, PCRTASN1DYNTYPE pParams);
RTDECL(int) RTCrPkixEncryptionReset(RTCRPKIXENCRYPTION hEncryption);
RTDECL(uint32_t) RTCrPkixEncryptionRetain(RTCRPKIXENCRYPTION hEncryption);
RTDECL(uint32_t) RTCrPkixEncryptionRelease(RTCRPKIXENCRYPTION hEncryption);


/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_crypto_pkix_h */

