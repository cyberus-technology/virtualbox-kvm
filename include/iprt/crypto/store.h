/** @file
 * IPRT - Cryptographic (Certificate) Store.
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

#ifndef IPRT_INCLUDED_crypto_store_h
#define IPRT_INCLUDED_crypto_store_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/crypto/x509.h>
#include <iprt/crypto/taf.h>
#include <iprt/sha.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_crstore    RTCrStore - Crypotgraphic (Certificate) Store.
 * @ingroup grp_rt_crypto
 * @{
 */


/**
 * A certificate store search.
 *
 * Used by the store provider to keep track of the current location of a
 * certificate search.
 */
typedef struct RTCRSTORECERTSEARCH
{
    /** Opaque provider specific storage.
     *
     * Provider restriction: The provider is only allowed to use the two first
     * entries for the find-all searches, because the front-end API may want the
     * last two for implementing specific searches on top of it. */
    uintptr_t   auOpaque[4];
} RTCRSTORECERTSEARCH;
/** Pointer to a certificate store search. */
typedef RTCRSTORECERTSEARCH *PRTCRSTORECERTSEARCH;


/**
 * Info about a wanted certificate.
 *
 * All the search criteria are optional, but for a safe and efficient search
 * it's recommended to specify all possible ones.  If none are given, the search
 * function will fail.
 *
 * For use with RTCrStoreCertAddFromFishingExpedition and others.
 */
typedef struct RTCRCERTWANTED
{
    /** The certificate subject name, optional.
     * The format is: "C=US, ST=California, L=Redwood Shores, O=Oracle Corporation" */
    const char *pszSubject;
    /** The size of the DER (ASN.1) encoded certificate, optional (0). */
    uint16_t    cbEncoded;
    /** Set if abSha1 contains a valid SHA-1 fingerprint. */
    bool        fSha1Fingerprint;
    /** Set if abSha512 contains a valid SHA-512 fingerprint. */
    bool        fSha512Fingerprint;
    /** The SHA-1 fingerprint (of the encoded data).   */
    uint8_t     abSha1[RTSHA1_HASH_SIZE];
    /** The SHA-512 fingerprint (of the encoded data).   */
    uint8_t     abSha512[RTSHA512_HASH_SIZE];
    /** User pointer for directly associating other data with the entry.
     * Subclassing the structure isn't possible because it's passed as an array. */
    void const *pvUser;
} RTCRCERTWANTED;
/** Pointer to a const certificat wanted structure. */
typedef RTCRCERTWANTED const *PCRTCRCERTWANTED;


/**
 * Standard store identifiers.
 *
 * This is a least common denominator approach to system specific certificate
 * stores, could be extended to include things other than certificates later if
 * we need it.
 *
 * Windows has lots of different stores, they'll be combined by the
 * implementation, possibly leading to duplicates.  The user stores on Windows
 * seems to be unioned with the system (machine) stores.
 *
 * Linux may have different stores depending on the distro/version/installation,
 * in which case we'll combine them, which will most likely lead to
 * duplicates just like on windows.  Haven't found any easily accessible
 * per-user certificate stores on linux yet, so they'll all be empty.
 *
 * Mac OS X seems a lot simpler, at least from the GUI point of view.  Each
 * keychains as a "Certificates" folder (the "My Certificates" folder seems to
 * only be a matching of "Keys" and "Certificates"). However, there are two
 * system keychains that we need to combine, "System" and "System Roots".  As
 * with Windows and Linux, there is a possibility for duplicates here.
 *
 * On solaris we have currently no idea where to look for a certificate store,
 * so that doesn't yet work.
 *
 * Because of the OS X setup, we do not provide any purpose specific
 */
typedef enum RTCRSTOREID
{
    /** Mandatory invalid zero value. */
    RTCRSTOREID_INVALID = 0,
    /** Open the certificate store of the current user containing trusted
     * CAs and certificates.
     * @remarks This may or may not include all the certificates in the system
     *          store, that's host dependent.  So, you better look in both. */
    RTCRSTOREID_USER_TRUSTED_CAS_AND_CERTIFICATES,
    /** Open the certificate store of the system containg trusted CAs
     * and certificates. */
    RTCRSTOREID_SYSTEM_TRUSTED_CAS_AND_CERTIFICATES,
    /** Open the certificate store of the current user containing intermediate CAs.
     * @remarks This may or may not include all the certificates in the system
     *          store, that's host dependent.  So, you better look in both. */
    RTCRSTOREID_USER_INTERMEDIATE_CAS,
    /** Open the certificate store of the system containg intermediate CAs. */
    RTCRSTOREID_SYSTEM_INTERMEDIATE_CAS,
    /** End of valid values. */
    RTCRSTOREID_END,
    /** Traditional enum type compression prevention hack. */
    RTCRSTOREID_32BIT_HACK = 0x7fffffff
} RTCRSTOREID;

/**
 * Creates a snapshot of a standard store.
 *
 * This will return an in-memory store containing all data from the given store.
 * There will be no duplicates in this one.
 *
 * @returns IPRT status code.
 * @param   phStore             Where to return the store handle. Use
 *                              RTCrStoreRelease to release it.
 * @param   enmStoreId          The store to snapshot.
 * @param   pErrInfo            Where to return additional error/warning info.
 *                              Optional.
 */
RTDECL(int) RTCrStoreCreateSnapshotById(PRTCRSTORE phStore, RTCRSTOREID enmStoreId, PRTERRINFO pErrInfo);

RTDECL(int) RTCrStoreCreateSnapshotOfUserAndSystemTrustedCAsAndCerts(PRTCRSTORE phStore, PRTERRINFO pErrInfo);

RTDECL(int) RTCrStoreCreateInMem(PRTCRSTORE phStore, uint32_t cSizeHint);
RTDECL(int) RTCrStoreCreateInMemEx(PRTCRSTORE phStore, uint32_t cSizeHint, RTCRSTORE hParentStore);

RTDECL(uint32_t) RTCrStoreRetain(RTCRSTORE hStore);
RTDECL(uint32_t) RTCrStoreRelease(RTCRSTORE hStore);
RTDECL(PCRTCRCERTCTX) RTCrStoreCertByIssuerAndSerialNo(RTCRSTORE hStore, PCRTCRX509NAME pIssuer, PCRTASN1INTEGER pSerialNo);

/**
 * Add a certificate to the store.
 *
 * @returns IPRT status code.
 * @retval  VWRN_ALREADY_EXISTS if the certificate is already present and
 *          RTCRCERTCTX_F_ADD_IF_NOT_FOUND was specified.
 * @retval  VERR_WRITE_PROTECT if the store doesn't support adding.
 * @param   hStore              The store to add the certificate to.
 * @param   fFlags              RTCRCERTCTX_F_XXX. Encoding must be specified.
 *                              RTCRCERTCTX_F_ADD_IF_NOT_FOUND is supported.
 * @param   pvSrc               The encoded certificate bytes.
 * @param   cbSrc               The size of the encoded certificate.
 * @param   pErrInfo            Where to return additional error/warning info.
 *                              Optional.
 */
RTDECL(int) RTCrStoreCertAddEncoded(RTCRSTORE hStore, uint32_t fFlags, void const *pvSrc, size_t cbSrc, PRTERRINFO pErrInfo);

/**
 * Add an X.509 packaged certificate to the store.
 *
 * @returns IPRT status code.
 * @retval  VWRN_ALREADY_EXISTS if the certificate is already present and
 *          RTCRCERTCTX_F_ADD_IF_NOT_FOUND was specified.
 * @retval  VERR_WRITE_PROTECT if the store doesn't support adding.
 * @param   hStore              The store to add the certificate to.
 * @param   fFlags              RTCRCERTCTX_F_XXX. Encoding must is optional,
 *                              but must be RTCRCERTCTX_F_ENC_X509_DER if given.
 *                              RTCRCERTCTX_F_ADD_IF_NOT_FOUND is supported.
 * @param   pCertificate        The certificate to add.  We may have to encode
 *                              it, thus not const.
 * @param   pErrInfo            Where to return additional error/warning info.
 *                              Optional.
 */
RTDECL(int) RTCrStoreCertAddX509(RTCRSTORE hStore, uint32_t fFlags, PRTCRX509CERTIFICATE pCertificate, PRTERRINFO pErrInfo);

/**
 * Adds certificates from files in the specified directory.
 *
 * @returns IPRT status code.  Even when RTCRCERTCTX_F_ADD_CONTINUE_ON_ERROR is
 *          used, an error is returned as an error (and not a warning).
 *
 * @param   hStore              The store to add the certificate(s) to.
 * @param   fFlags              RTCRCERTCTX_F_ADD_IF_NOT_FOUND and/or
 *                              RTCRCERTCTX_F_ADD_CONTINUE_ON_ERROR.
 * @param   pszDir              The path to the directory.
 * @param   paSuffixes          List of suffixes of files to process.
 * @param   cSuffixes           Number of suffixes.  If this is 0, all files are
 *                              processed.
 * @param   pErrInfo            Where to return additional error/warning info.
 *                              Optional.
 */
RTDECL(int) RTCrStoreCertAddFromDir(RTCRSTORE hStore, uint32_t fFlags, const char *pszDir,
                                    PCRTSTRTUPLE paSuffixes, size_t cSuffixes, PRTERRINFO pErrInfo);

RTDECL(int) RTCrStoreCertAddWantedFromDir(RTCRSTORE hStore, uint32_t fFlags,
                                          const char *pszDir, PCRTSTRTUPLE paSuffixes, size_t cSuffixes,
                                          PCRTCRCERTWANTED paWanted, size_t cWanted, bool *pafFound, PRTERRINFO pErrInfo);

/**
 * Adds certificates from the specified file.
 *
 * The supported file formats are:
 *      - PEM (base 64 blobs wrapped in -----BEGIN / END----).  Support multiple
 *        certificates in one file.
 *      - Binary DER ASN.1 certificate. Only one per file.
 *      - Java key store version 2.
 *
 * @returns IPRT status code.  Even when RTCRCERTCTX_F_ADD_CONTINUE_ON_ERROR is
 *          used, an error is returned as an error (and not a warning).
 *
 * @param   hStore              The store to add the certificate(s) to.
 * @param   fFlags              RTCRCERTCTX_F_ADD_IF_NOT_FOUND and/or
 *                              RTCRCERTCTX_F_ADD_CONTINUE_ON_ERROR.
 * @param   pszFilename         The filename.
 * @param   pErrInfo            Where to return additional error/warning info.
 *                              Optional.
 */
RTDECL(int) RTCrStoreCertAddFromFile(RTCRSTORE hStore, uint32_t fFlags, const char *pszFilename, PRTERRINFO pErrInfo);

RTDECL(int) RTCrStoreCertAddWantedFromFile(RTCRSTORE hStore, uint32_t fFlags, const char *pszFilename,
                                           PCRTCRCERTWANTED paWanted, size_t cWanted, bool *pafFound, PRTERRINFO pErrInfo);

/**
 * Adds certificates from the specified java key store file.
 *
 * @returns IPRT status code.  Even when RTCRCERTCTX_F_ADD_CONTINUE_ON_ERROR is
 *          used, an error is returned as an error (and not a warning).
 *
 * @param   hStore              The store to add the certificate(s) to.
 * @param   fFlags              RTCRCERTCTX_F_ADD_IF_NOT_FOUND and/or
 *                              RTCRCERTCTX_F_ADD_CONTINUE_ON_ERROR.
 * @param   pszFilename         The path to the JKS file.
 * @param   pErrInfo            Where to return additional error/warning info.
 *                              Optional.
 */
RTDECL(int) RTCrStoreCertAddFromJavaKeyStore(RTCRSTORE hStore, uint32_t fFlags, const char *pszFilename, PRTERRINFO pErrInfo);

/**
 * Adds certificates from an in-memory java key store.
 *
 * @returns IPRT status code.  Even when RTCRCERTCTX_F_ADD_CONTINUE_ON_ERROR is
 *          used, an error is returned as an error (and not a warning).
 *
 * @param   hStore              The store to add the certificate(s) to.
 * @param   fFlags              RTCRCERTCTX_F_ADD_IF_NOT_FOUND and/or
 *                              RTCRCERTCTX_F_ADD_CONTINUE_ON_ERROR.
 * @param   pvContent           Pointer to the key store bytes.
 * @param   cbContent           The size of the key store.
 * @param   pszErrorName        The file name or whatever helpful indicator the
 *                              caller want in the error messages.
 * @param   pErrInfo            Where to return additional error/warning info.
 *                              Optional.
 */
RTDECL(int) RTCrStoreCertAddFromJavaKeyStoreInMem(RTCRSTORE hStore, uint32_t fFlags, void const *pvContent, size_t cbContent,
                                                  const char *pszErrorName, PRTERRINFO pErrInfo);

/**
 * Adds all certificates from @a hStoreSrc into @a hStore.
 *
 * @returns IPRT status code.  Even when RTCRCERTCTX_F_ADD_CONTINUE_ON_ERROR is
 *          used, an error is returned as an error (and not a warning).
 *
 * @param   hStore              The destination store.
 * @param   fFlags              RTCRCERTCTX_F_ADD_IF_NOT_FOUND and/or
 *                              RTCRCERTCTX_F_ADD_CONTINUE_ON_ERROR.
 * @param   hStoreSrc           The source store.
 */
RTDECL(int) RTCrStoreCertAddFromStore(RTCRSTORE hStore, uint32_t fFlags, RTCRSTORE hStoreSrc);

RTDECL(int) RTCrStoreCertAddWantedFromStore(RTCRSTORE hStore, uint32_t fFlags, RTCRSTORE hSrcStore,
                                            PCRTCRCERTWANTED paWanted, size_t cWanted, bool *pafFound);

RTDECL(int) RTCrStoreCertCheckWanted(RTCRSTORE hStore, PCRTCRCERTWANTED paWanted, size_t cWanted, bool *pafFound);


RTDECL(int) RTCrStoreCertAddWantedFromFishingExpedition(RTCRSTORE hStore, uint32_t fFlags,
                                                        PCRTCRCERTWANTED paWanted, size_t cWanted,
                                                        bool *pafFound, PRTERRINFO pErrInfo);

/**
 * Exports the certificates in the store to a PEM file
 *
 * @returns IPRT status code.
 * @param   hStore              The store which certificates should be exported.
 * @param   fFlags              Reserved for the future, MBZ.
 * @param   pszFilename         The name of the destination PEM file.  This will
 *                              be truncated.
 */
RTDECL(int) RTCrStoreCertExportAsPem(RTCRSTORE hStore, uint32_t fFlags, const char *pszFilename);

/**
 * Counts the number of certificates in the store.
 *
 * @returns Certificate count on success, UINT32_MAX on failure.
 * @param   hStore              The store which certificates should be counted.
 */
RTDECL(uint32_t) RTCrStoreCertCount(RTCRSTORE hStore);

RTDECL(int) RTCrStoreCertFindAll(RTCRSTORE hStore, PRTCRSTORECERTSEARCH pSearch);
RTDECL(int) RTCrStoreCertFindBySubjectOrAltSubjectByRfc5280(RTCRSTORE hStore, PCRTCRX509NAME pSubject,
                                                            PRTCRSTORECERTSEARCH pSearch);
RTDECL(PCRTCRCERTCTX) RTCrStoreCertSearchNext(RTCRSTORE hStore, PRTCRSTORECERTSEARCH pSearch);
RTDECL(int) RTCrStoreCertSearchDestroy(RTCRSTORE hStore, PRTCRSTORECERTSEARCH pSearch);

RTDECL(int) RTCrStoreConvertToOpenSslCertStore(RTCRSTORE hStore, uint32_t fFlags, void **ppvOpenSslStore, PRTERRINFO pErrInfo);
RTDECL(int) RTCrStoreConvertToOpenSslCertStack(RTCRSTORE hStore, uint32_t fFlags, void **ppvOpenSslStack, PRTERRINFO pErrInfo);


/** @} */


/** @defgroup grp_rt_crcertctx  RTCrCertCtx - (Store) Certificate Context.
 * @{  */


/**
 * Certificate context.
 *
 * This is returned by the certificate store APIs and is part of a larger
 * reference counted structure.  All the data is read only.
 */
typedef struct RTCRCERTCTX
{
    /** Flags, RTCRCERTCTX_F_XXX.  */
    uint32_t                    fFlags;
    /** The size of the (DER) encoded certificate. */
    uint32_t                    cbEncoded;
    /** Pointer to the (DER) encoded certificate. */
    uint8_t const              *pabEncoded;
    /** Pointer to the decoded X.509 representation of the certificate.
     * This can be NULL when pTaInfo is present.  */
    PCRTCRX509CERTIFICATE       pCert;
    /** Pointer to the decoded TrustAnchorInfo for the certificate.  This can be
     * NULL, even for trust anchors, as long as pCert isn't. */
    PCRTCRTAFTRUSTANCHORINFO    pTaInfo;
    /** Reserved for future use. */
    void                       *paReserved[2];
} RTCRCERTCTX;

/** @name RTCRCERTCTX_F_XXX.
 * @{ */
/** Encoding mask. */
#define RTCRCERTCTX_F_ENC_MASK         UINT32_C(0x000000ff)
/** X.509 certificate, DER encoded. */
#define RTCRCERTCTX_F_ENC_X509_DER     UINT32_C(0x00000000)
/** RTF-5914 trust anchor info, DER encoded. */
#define RTCRCERTCTX_F_ENC_TAF_DER      UINT32_C(0x00000001)
#if 0
/** Extended certificate, DER encoded. */
#define RTCRCERTCTX_F_ENC_PKCS6_DER    UINT32_C(0x00000002)
#endif
/** Mask containing the flags that ends up in the certificate context. */
#define RTCRCERTCTX_F_MASK             UINT32_C(0x000000ff)

/** Add APIs: Add the certificate if not found. */
#define RTCRCERTCTX_F_ADD_IF_NOT_FOUND          UINT32_C(0x00010000)
/** Add APIs: Continue on error when possible. */
#define RTCRCERTCTX_F_ADD_CONTINUE_ON_ERROR     UINT32_C(0x00020000)
/** @} */


RTDECL(uint32_t) RTCrCertCtxRetain(PCRTCRCERTCTX pCertCtx);
RTDECL(uint32_t) RTCrCertCtxRelease(PCRTCRCERTCTX pCertCtx);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_crypto_store_h */

