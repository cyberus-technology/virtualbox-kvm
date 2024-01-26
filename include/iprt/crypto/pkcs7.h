/** @file
 * IPRT - PKCS \#7, Cryptographic Message Syntax Standard (aka CMS).
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

#ifndef IPRT_INCLUDED_crypto_pkcs7_h
#define IPRT_INCLUDED_crypto_pkcs7_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/asn1.h>
#include <iprt/crypto/x509.h>


RT_C_DECLS_BEGIN

struct RTCRPKCS7CONTENTINFO;


/** @defgroup grp_rt_crpkcs7 RTCrPkcs7 - PKCS \#7, Cryptographic Message Syntax Standard (aka CMS).
 * @ingroup grp_rt_crypto
 * @{
 */

/** PKCS \#7 data object ID.*/
#define RTCR_PKCS7_DATA_OID                         "1.2.840.113549.1.7.1"
/** PKCS \#7 signedData object ID. */
#define RTCR_PKCS7_SIGNED_DATA_OID                  "1.2.840.113549.1.7.2"
/** PKCS \#7 envelopedData object ID. */
#define RTCR_PKCS7_ENVELOPED_DATA_OID               "1.2.840.113549.1.7.3"
/** PKCS \#7 signedAndEnvelopedData object ID.  */
#define RTCR_PKCS7_SIGNED_AND_ENVELOPED_DATA_OID    "1.2.840.113549.1.7.4"
/** PKCS \#7 digestedData object ID. */
#define RTCR_PKCS7_DIGESTED_DATA_OID                "1.2.840.113549.1.7.5"
/** PKCS \#7 encryptedData object ID. */
#define RTCR_PKCS7_ENCRYPTED_DATA_OID               "1.2.840.113549.1.7.6"


/**
 * PKCS \#7 IssuerAndSerialNumber (IPRT representation).
 */
typedef struct RTCRPKCS7ISSUERANDSERIALNUMBER
{
    /** Sequence core. */
    RTASN1SEQUENCECORE                  SeqCore;
    /** The certificate name. */
    RTCRX509NAME                        Name;
    /** The certificate serial number. */
    RTASN1INTEGER                       SerialNumber;
} RTCRPKCS7ISSUERANDSERIALNUMBER;
/** Pointer to the IPRT representation of a PKCS \#7 IssuerAndSerialNumber. */
typedef RTCRPKCS7ISSUERANDSERIALNUMBER *PRTCRPKCS7ISSUERANDSERIALNUMBER;
/** Pointer to the const IPRT representation of a PKCS \#7
 * IssuerAndSerialNumber. */
typedef RTCRPKCS7ISSUERANDSERIALNUMBER const *PCRTCRPKCS7ISSUERANDSERIALNUMBER;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRPKCS7ISSUERANDSERIALNUMBER, RTDECL, RTCrPkcs7IssuerAndSerialNumber, SeqCore.Asn1Core);


/** Pointer to the IPRT representation of a PKCS \#7 SignerInfo. */
typedef struct RTCRPKCS7SIGNERINFO *PRTCRPKCS7SIGNERINFO;
/** Pointer to the const IPRT representation of a PKCS \#7 SignerInfo. */
typedef struct RTCRPKCS7SIGNERINFO const *PCRTCRPKCS7SIGNERINFO;
RTASN1_IMPL_GEN_SET_OF_TYPEDEFS_AND_PROTOS(RTCRPKCS7SIGNERINFOS, RTCRPKCS7SIGNERINFO, RTDECL, RTCrPkcs7SignerInfos);


/**
 * Attribute value type (for the union).
 */
typedef enum RTCRPKCS7ATTRIBUTETYPE
{
    /** Zero is invalid. */
    RTCRPKCS7ATTRIBUTETYPE_INVALID = 0,
    /** Not present, union is NULL. */
    RTCRPKCS7ATTRIBUTETYPE_NOT_PRESENT,
    /** Unknown values, pCores. */
    RTCRPKCS7ATTRIBUTETYPE_UNKNOWN,
    /** Object IDs, use pObjIds. */
    RTCRPKCS7ATTRIBUTETYPE_OBJ_IDS,
    /** Octet strings, use pOctetStrings. */
    RTCRPKCS7ATTRIBUTETYPE_OCTET_STRINGS,
    /** Counter signatures (PKCS \#9), use pCounterSignatures.
     * RTCR_PKCS9_ID_COUNTER_SIGNATURE_OID - 1.2.840.113549.1.9.6.  */
    RTCRPKCS7ATTRIBUTETYPE_COUNTER_SIGNATURES,
    /** Signing time (PKCS \#9), use pSigningTime.
     * RTCR_PKCS9_ID_SIGNING_TIME_OID - 1.2.840.113549.1.9.5.  */
    RTCRPKCS7ATTRIBUTETYPE_SIGNING_TIME,
    /** Microsoft timestamp info (RFC-3161) signed data, use pContentInfo.
     * RTCR_PKCS9_ID_MS_TIMESTAMP - 1.3.6.1.4.1.311.3.3.1. */
    RTCRPKCS7ATTRIBUTETYPE_MS_TIMESTAMP,
    /** Microsoft nested PKCS\#7 signature (signtool /as).
     * RTCR_PKCS9_ID_MS_NESTED_SIGNATURE  - 1.3.6.1.4.1.311.2.4.1. */
    RTCRPKCS7ATTRIBUTETYPE_MS_NESTED_SIGNATURE,
    /** Microsoft statement type, use pObjIdSeqs.
     * RTCR_PKCS9_ID_MS_STATEMENT_TYPE - 1.3.6.1.4.1.311.2.1.11. */
    RTCRPKCS7ATTRIBUTETYPE_MS_STATEMENT_TYPE,
    /** Apple plist with the all code directory digests, use pOctetStrings.
     * RTCR_PKCS9_ID_APPLE_MULTI_CD_PLIST - 1.2.840.113635.100.9.1.  */
    RTCRPKCS7ATTRIBUTETYPE_APPLE_MULTI_CD_PLIST,
    /** Blow the type up to 32-bits. */
    RTCRPKCS7ATTRIBUTETYPE_32BIT_HACK = 0x7fffffff
} RTCRPKCS7ATTRIBUTETYPE;

/**
 * PKCS \#7 Attribute (IPRT representation).
 */
typedef struct RTCRPKCS7ATTRIBUTE
{
    /** Sequence core. */
    RTASN1SEQUENCECORE                  SeqCore;
    /** The attribute type (object ID). */
    RTASN1OBJID                         Type;
    /** The type of data found in the values union. */
    RTCRPKCS7ATTRIBUTETYPE              enmType;
    /** Value allocation. */
    RTASN1ALLOCATION                    Allocation;
    /** Values.  */
    union
    {
        /** ASN.1 cores (RTCRPKCS7ATTRIBUTETYPE_UNKNOWN). */
        PRTASN1SETOFCORES               pCores;
        /** ASN.1 object identifiers (RTCRPKCS7ATTRIBUTETYPE_OBJ_IDS). */
        PRTASN1SETOFOBJIDS              pObjIds;
        /** Sequence of ASN.1 object identifiers (RTCRPKCS7ATTRIBUTETYPE_MS_STATEMENT_TYPE). */
        PRTASN1SETOFOBJIDSEQS           pObjIdSeqs;
        /** ASN.1 octet strings (RTCRPKCS7ATTRIBUTETYPE_OCTET_STRINGS). */
        PRTASN1SETOFOCTETSTRINGS        pOctetStrings;
        /** Counter signatures RTCRPKCS7ATTRIBUTETYPE_COUNTER_SIGNATURES(). */
        PRTCRPKCS7SIGNERINFOS           pCounterSignatures;
        /** Signing time(s) (RTCRPKCS7ATTRIBUTETYPE_SIGNING_TIME). */
        PRTASN1SETOFTIMES               pSigningTime;
        /** Microsoft timestamp (RFC-3161 signed data, RTCRPKCS7ATTRIBUTETYPE_MS_TIMESTAMP),
         * Microsoft nested signature (RTCRPKCS7ATTRIBUTETYPE_MS_NESTED_SIGNATURE). */
        struct RTCRPKCS7SETOFCONTENTINFOS *pContentInfos;
    } uValues;
} RTCRPKCS7ATTRIBUTE;
/** Pointer to the IPRT representation of a PKCS \#7 Attribute. */
typedef RTCRPKCS7ATTRIBUTE *PRTCRPKCS7ATTRIBUTE;
/** Pointer to the const IPRT representation of a PKCS \#7 Attribute. */
typedef RTCRPKCS7ATTRIBUTE const *PCRTCRPKCS7ATTRIBUTE;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRPKCS7ATTRIBUTE, RTDECL, RTCrPkcs7Attribute, SeqCore.Asn1Core);

RTDECL(int) RTCrPkcs7Attribute_SetAppleMultiCdPlist(PRTCRPKCS7ATTRIBUTE pThis, PCRTASN1SETOFOCTETSTRINGS pToClone,
                                                    PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTCrPkcs7Attribute_SetContentType(PRTCRPKCS7ATTRIBUTE pThis, PCRTASN1SETOFOBJIDS pToClone,
                                              PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTCrPkcs7Attribute_SetCounterSignatures(PRTCRPKCS7ATTRIBUTE pThis, PCRTCRPKCS7SIGNERINFOS pToClone,
                                                    PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTCrPkcs7Attribute_SetMessageDigest(PRTCRPKCS7ATTRIBUTE pThis, PCRTASN1SETOFOCTETSTRINGS pToClone,
                                                PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTCrPkcs7Attribute_SetMsStatementType(PRTCRPKCS7ATTRIBUTE pThis, PCRTASN1SETOFOBJIDSEQS pToClone,
                                                  PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTCrPkcs7Attribute_SetMsNestedSignature(PRTCRPKCS7ATTRIBUTE pThis, struct RTCRPKCS7SETOFCONTENTINFOS const *pToClone,
                                                    PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTCrPkcs7Attribute_SetMsTimestamp(PRTCRPKCS7ATTRIBUTE pThis, struct RTCRPKCS7SETOFCONTENTINFOS const *pToClone,
                                              PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTCrPkcs7Attribute_SetSigningTime(PRTCRPKCS7ATTRIBUTE pThis, PCRTASN1SETOFTIMES pToClone,
                                              PCRTASN1ALLOCATORVTABLE pAllocator);

RTASN1_IMPL_GEN_SET_OF_TYPEDEFS_AND_PROTOS(RTCRPKCS7ATTRIBUTES, RTCRPKCS7ATTRIBUTE, RTDECL, RTCrPkcs7Attributes);

RTDECL(int) RTCrPkcs7Attributes_HashAttributes(PRTCRPKCS7ATTRIBUTES pAttributes, RTCRDIGEST hDigest, PRTERRINFO pErrInfo);


/**
 * One PKCS \#7 SignerInfo (IPRT representation).
 */
typedef struct RTCRPKCS7SIGNERINFO
{
    /** Sequence core. */
    RTASN1SEQUENCECORE                  SeqCore;
    /** The structure version (RTCRPKCS7SIGNERINFO_V1). */
    RTASN1INTEGER                       Version;
    /** The issuer and serial number of the certificate used to produce the
     * encrypted digest below. */
    RTCRPKCS7ISSUERANDSERIALNUMBER      IssuerAndSerialNumber;
    /** The digest algorithm use to digest the signed content. */
    RTCRX509ALGORITHMIDENTIFIER         DigestAlgorithm;
    /** Authenticated attributes, optional [0].
     * @todo Check how other producers formats this. The microsoft one does not
     *       have explicit tags, but combines it with the SET OF. */
    RTCRPKCS7ATTRIBUTES                 AuthenticatedAttributes;
    /** The digest encryption algorithm use to encrypt the digest of the signed
     * content. */
    RTCRX509ALGORITHMIDENTIFIER         DigestEncryptionAlgorithm;
    /** The encrypted digest. */
    RTASN1OCTETSTRING                   EncryptedDigest;
    /** Unauthenticated attributes, optional [1].
     * @todo Check how other producers formats this. The microsoft one does not
     *       have explicit tags, but combines it with the SET OF. */
    RTCRPKCS7ATTRIBUTES                 UnauthenticatedAttributes;
} RTCRPKCS7SIGNERINFO;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRPKCS7SIGNERINFO, RTDECL, RTCrPkcs7SignerInfo, SeqCore.Asn1Core);

RTDECL(int) RTCrPkcs7SignerInfo_SetAuthenticatedAttributes(PRTCRPKCS7SIGNERINFO pThis, PCRTCRPKCS7ATTRIBUTES pAttributes,
                                                           PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTCrPkcs7SignerInfo_SetUnauthenticatedAttributes(PRTCRPKCS7SIGNERINFO pThis, PCRTCRPKCS7ATTRIBUTES pAttributes,
                                                             PCRTASN1ALLOCATORVTABLE pAllocator);

/** RTCRPKCS7SIGNERINFO::Version value.  */
#define RTCRPKCS7SIGNERINFO_V1    1

/** @name PKCS \#9 Attribute IDs
 * @{ */
/** Content type (RFC-2630 11.1).
 * Value: Object Identifier */
#define RTCR_PKCS9_ID_CONTENT_TYPE_OID      "1.2.840.113549.1.9.3"
/** Message digest (RFC-2630 11.2).
 * Value: Octet string. */
#define RTCR_PKCS9_ID_MESSAGE_DIGEST_OID    "1.2.840.113549.1.9.4"
/** Signing time (RFC-2630 11.3).
 * Value: Octet string. */
#define RTCR_PKCS9_ID_SIGNING_TIME_OID      "1.2.840.113549.1.9.5"
/** Counter signature (RFC-2630 11.4).
 * Value: SignerInfo. */
#define RTCR_PKCS9_ID_COUNTER_SIGNATURE_OID "1.2.840.113549.1.9.6"
/** Microsoft timestamp (RTF-3161) counter signature (SignedData).
 * @remarks This isn't defined by PKCS \#9, but lumped in here for convenience. It's actually listed as SPC by MS. */
#define RTCR_PKCS9_ID_MS_TIMESTAMP          "1.3.6.1.4.1.311.3.3.1"
/** Microsoft nested PKCS\#7 signature.
 * @remarks This isn't defined by PKCS \#9, but lumped in here for convenience. */
#define RTCR_PKCS9_ID_MS_NESTED_SIGNATURE   "1.3.6.1.4.1.311.2.4.1"
/** Microsoft statement type.
 * @remarks This isn't defined by PKCS \#9, but lumped in here for convenience. It's actually listed as SPC by MS. */
#define RTCR_PKCS9_ID_MS_STATEMENT_TYPE     "1.3.6.1.4.1.311.2.1.11"
/** Microsoft opus info.
 * @remarks This isn't defined by PKCS \#9, but lumped in here for convenience. It's actually listed as SPC by MS. */
#define RTCR_PKCS9_ID_MS_SP_OPUS_INFO       "1.3.6.1.4.1.311.2.1.12"
/** Apple code signing multi-code-directory plist.
 * @remarks This isn't defined by PKCS \#9, but lumped in here for convenience. */
#define RTCR_PKCS9_ID_APPLE_MULTI_CD_PLIST  "1.2.840.113635.100.9.1"
/** @} */


/**
 * Get the (next) signing time attribute from the specfied SignerInfo or one of
 * the immediate counter signatures.
 *
 * @returns Pointer to the signing time if found, NULL if not.
 * @param   pThis               The SignerInfo to search.
 * @param   ppSignerInfo        Pointer to variable keeping track of the
 *                              enumeration, optional.
 *
 *                              If specified the input value is taken to the be
 *                              SignerInfo of the previously returned signing
 *                              time.  The value pointed to is NULL, the
 *                              search/enum restarts.
 *
 *                              On successful return this is set to the
 *                              SignerInfo which we found the signing time in.
 */
RTDECL(PCRTASN1TIME) RTCrPkcs7SignerInfo_GetSigningTime(PCRTCRPKCS7SIGNERINFO pThis, PCRTCRPKCS7SIGNERINFO *ppSignerInfo);


/**
 * Get the (first) timestamp from within a Microsoft timestamp server counter
 * signature.
 *
 * @returns Pointer to the signing time if found, NULL if not.
 * @param   pThis               The SignerInfo to search.
 * @param   ppContentInfoRet    Where to return the pointer to the counter
 *                              signature, optional.
 */
RTDECL(PCRTASN1TIME) RTCrPkcs7SignerInfo_GetMsTimestamp(PCRTCRPKCS7SIGNERINFO pThis,
                                                        struct RTCRPKCS7CONTENTINFO const **ppContentInfoRet);



/**
 * PKCS \#7 ContentInfo (IPRT representation).
 */
typedef struct RTCRPKCS7CONTENTINFO
{
    /** Sequence core. */
    RTASN1SEQUENCECORE                  SeqCore;
    /** Object ID identifying the content below. */
    RTASN1OBJID                         ContentType;
    /** Content, optional, explicit tag 0.
     *
     * Hack alert! This should've been an explict context tag 0 structure with a
     * type selected according to ContentType.  However, it's simpler to replace the
     * explicit context with an OCTET STRING with implict tag 0.  Then we can tag
     * along on the encapsulation logic RTASN1OCTETSTRING provides for the dynamic
     * inner type.  The default decoder code will detect known structures as
     * outlined in the union below, and decode the octet string content as an
     * anonymous RTASN1CORE if not known.
     *
     * If the user want to decode the octet string content differently, it can do so
     * by destroying and freeing the current encapsulated pointer, replacing it with
     * it's own.  (Of course following the RTASN1OCTETSTRING rules.)  Just remember
     * to also update the value in the union.
     *
     * @remarks What's signed and verified is Content.pEncapsulated->uData.pv.
     */
    RTASN1OCTETSTRING                   Content;
    /** Pointer to the CMS octet string that's inside the Content, NULL if PKCS \#7.
     *
     * Hack alert! When transitioning from PKCS \#7 to CMS, the designers decided to
     * change things and add another wrapper.  This time we're talking about a real
     * octet string, not like the one above which is really an explicit content tag.
     * When constructing or decoding CMS content, this will be the same pointer as
     * Content.pEncapsulated, while the union below will be holding the same pointer
     * as pCmsContent->pEncapsulated.
     */
    PRTASN1OCTETSTRING                  pCmsContent;
    /** Same as Content.pEncapsulated, except a choice of known types. */
    union
    {
        /** ContentType is RTCRPKCS7SIGNEDDATA_OID. */
        struct RTCRPKCS7SIGNEDDATA         *pSignedData;
        /** ContentType is RTCRSPCINDIRECTDATACONTENT_OID. */
        struct RTCRSPCINDIRECTDATACONTENT  *pIndirectDataContent;
        /** ContentType is RTCRTSPTSTINFO_OID. */
        struct RTCRTSPTSTINFO              *pTstInfo;
        /** Generic / Unknown / User. */
        PRTASN1CORE                         pCore;
    } u;
} RTCRPKCS7CONTENTINFO;
/** Pointer to the IPRT representation of a PKCS \#7 ContentInfo. */
typedef RTCRPKCS7CONTENTINFO *PRTCRPKCS7CONTENTINFO;
/** Pointer to the const IPRT representation of a PKCS \#7 ContentInfo. */
typedef RTCRPKCS7CONTENTINFO const *PCRTCRPKCS7CONTENTINFO;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRPKCS7CONTENTINFO, RTDECL, RTCrPkcs7ContentInfo, SeqCore.Asn1Core);
RTASN1_IMPL_GEN_SET_OF_TYPEDEFS_AND_PROTOS(RTCRPKCS7SETOFCONTENTINFOS, RTCRPKCS7CONTENTINFO, RTDECL, RTCrPkcs7SetOfContentInfos);

RTDECL(bool) RTCrPkcs7ContentInfo_IsSignedData(PCRTCRPKCS7CONTENTINFO pThis);


/**
 * PKCS \#7 Certificate choice.
 */
typedef enum RTCRPKCS7CERTCHOICE
{
    RTCRPKCS7CERTCHOICE_INVALID = 0,
    RTCRPKCS7CERTCHOICE_X509,
    RTCRPKCS7CERTCHOICE_EXTENDED_PKCS6,
    RTCRPKCS7CERTCHOICE_AC_V1,
    RTCRPKCS7CERTCHOICE_AC_V2,
    RTCRPKCS7CERTCHOICE_OTHER,
    RTCRPKCS7CERTCHOICE_END,
    RTCRPKCS7CERTCHOICE_32BIT_HACK = 0x7fffffff
} RTCRPKCS7CERTCHOICE;


/**
 * Common representation for PKCS \#7 ExtendedCertificateOrCertificate and the
 * CMS CertificateChoices types.
 */
typedef struct RTCRPKCS7CERT
{
    /** Dummy ASN.1 record, not encoded. */
    RTASN1DUMMY                         Dummy;
    /** The value allocation. */
    RTASN1ALLOCATION                    Allocation;
    /** The choice of value.   */
    RTCRPKCS7CERTCHOICE                 enmChoice;
    /** The value union. */
    union
    {
        /** Standard X.509 certificate (RTCRCMSCERTIFICATECHOICE_X509). */
        PRTCRX509CERTIFICATE            pX509Cert;
        /** Extended PKCS \#6 certificate (RTCRCMSCERTIFICATECHOICE_EXTENDED_PKCS6). */
        PRTASN1CORE                     pExtendedCert;
        /** Attribute certificate version 1 (RTCRCMSCERTIFICATECHOICE_AC_V1). */
        PRTASN1CORE                     pAcV1;
        /** Attribute certificate version 2 (RTCRCMSCERTIFICATECHOICE_AC_V2). */
        PRTASN1CORE                     pAcV2;
        /** Other certificate (RTCRCMSCERTIFICATECHOICE_OTHER). */
        PRTASN1CORE                     pOtherCert;
    } u;
} RTCRPKCS7CERT;
/** Pointer to the IPRT representation of PKCS \#7 or CMS certificate. */
typedef RTCRPKCS7CERT *PRTCRPKCS7CERT;
/** Pointer to the const IPRT representation of PKCS \#7 or CMS certificate. */
typedef RTCRPKCS7CERT const *PCRTCRPKCS7CERT;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRPKCS7CERT, RTDECL, RTCrPkcs7Cert, Dummy.Asn1Core);
RTASN1_IMPL_GEN_SET_OF_TYPEDEFS_AND_PROTOS(RTCRPKCS7SETOFCERTS, RTCRPKCS7CERT, RTDECL, RTCrPkcs7SetOfCerts);

RTDECL(int) RTCrPkcs7Cert_SetX509Cert(PRTCRPKCS7CERT pThis, PCRTCRX509CERTIFICATE pToClone, PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTCrPkcs7Cert_SetExtendedCert(PRTCRPKCS7CERT pThis, PCRTASN1CORE pToClone, PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTCrPkcs7Cert_SetAcV1(PRTCRPKCS7CERT pThis, PCRTASN1CORE pToClone, PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTCrPkcs7Cert_SetAcV2(PRTCRPKCS7CERT pThis, PCRTASN1CORE pToClone, PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTCrPkcs7Cert_SetOtherCert(PRTCRPKCS7CERT pThis, PCRTASN1CORE pToClone, PCRTASN1ALLOCATORVTABLE pAllocator);

RTDECL(PCRTCRX509CERTIFICATE) RTCrPkcs7SetOfCerts_FindX509ByIssuerAndSerialNumber(PCRTCRPKCS7SETOFCERTS pCertificates,
                                                                                  PCRTCRX509NAME pIssuer,
                                                                                  PCRTASN1INTEGER pSerialNumber);


/**
 * PKCS \#7 SignedData (IPRT representation).
 */
typedef struct RTCRPKCS7SIGNEDDATA
{
    /** Sequence core. */
    RTASN1SEQUENCECORE                  SeqCore;
    /** The structure version value (1). */
    RTASN1INTEGER                       Version;
    /** The digest algorithms that are used to signed the content (ContentInfo). */
    RTCRX509ALGORITHMIDENTIFIERS        DigestAlgorithms;
    /** The content that's being signed. */
    RTCRPKCS7CONTENTINFO                ContentInfo;
    /** Certificates, optional, implicit tag 0. (Required by Authenticode.) */
    RTCRPKCS7SETOFCERTS                 Certificates;
    /** Certificate revocation lists, optional, implicit tag 1.
     * Not used by Authenticode, so currently stubbed. */
    RTASN1CORE                          Crls;
    /** Signer infos. */
    RTCRPKCS7SIGNERINFOS                SignerInfos;
} RTCRPKCS7SIGNEDDATA;
/** Pointer to the IPRT representation of a PKCS \#7 SignedData. */
typedef RTCRPKCS7SIGNEDDATA *PRTCRPKCS7SIGNEDDATA;
/** Pointer to the const IPRT representation of a PKCS \#7 SignedData. */
typedef RTCRPKCS7SIGNEDDATA const *PCRTCRPKCS7SIGNEDDATA;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRPKCS7SIGNEDDATA, RTDECL, RTCrPkcs7SignedData, SeqCore.Asn1Core);
RTASN1_IMPL_GEN_SET_OF_TYPEDEFS_AND_PROTOS(RTCRPKCS7SETOFSIGNEDDATA, RTCRPKCS7SIGNEDDATA, RTDECL, RTCrPkcs7SetOfSignedData);

/** PKCS \#7 SignedData object ID.  */
#define RTCRPKCS7SIGNEDDATA_OID   RTCR_PKCS7_SIGNED_DATA_OID

/** PKCS \#7 SignedData version number 1.  */
#define RTCRPKCS7SIGNEDDATA_V1    1
/* No version 2 seems to exist. */
/** CMS SignedData version number 3.
 * This should only be used if there are version 1 attribute certificates
 * present, or if there are version 3 SignerInfo items present, or if
 * enmcCountInfo is not id-data (RFC-5652, section 5.1). */
#define RTCRPKCS7SIGNEDDATA_V3    3
/** CMS SignedData version number 4.
 * This should only be used if there are version 2 attribute certificates
 * present (RFC-5652, section 5.1). */
#define RTCRPKCS7SIGNEDDATA_V4    4
/** CMS SignedData version number 5.
 * This should only be used if there are certificates or/and CRLs of the
 * OTHER type present (RFC-5652, section 5.1). */
#define RTCRPKCS7SIGNEDDATA_V5    5

RTDECL(int) RTCrPkcs7SignedData_SetCertificates(PRTCRPKCS7SIGNEDDATA pThis, PCRTCRPKCS7SETOFCERTS pCerts, PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTCrPkcs7SignedData_SetCrls(PRTCRPKCS7SIGNEDDATA pThis, PCRTASN1CORE pCerts, PCRTASN1ALLOCATORVTABLE pAllocator);

/** @name RTCRPKCS7SIGNEDDATA_SANITY_F_XXX - Flags for RTPkcs7SignedDataCheckSantiy.
 * @{ */
/** Check for authenticode restrictions. */
#define RTCRPKCS7SIGNEDDATA_SANITY_F_AUTHENTICODE         RT_BIT_32(0)
/** Check that all the hash algorithms are known to IPRT. */
#define RTCRPKCS7SIGNEDDATA_SANITY_F_ONLY_KNOWN_HASH      RT_BIT_32(1)
/** Require signing certificate to be present. */
#define RTCRPKCS7SIGNEDDATA_SANITY_F_SIGNING_CERT_PRESENT RT_BIT_32(2)
/** @} */

/** PKCS\#7/CMS (content info) markers. */
extern RTDATADECL(RTCRPEMMARKER const) g_aRTCrPkcs7Markers[];
/** Number of entries in g_aRTCrPkcs7Markers. */
extern RTDATADECL(uint32_t const)      g_cRTCrPkcs7Markers;

/** @name Flags for RTCrPkcs7ContentInfo_ReadFromBuffer
 * @{ */
/** Only allow PEM certificates, not binary ones.
 * @sa RTCRPEMREADFILE_F_ONLY_PEM  */
#define RTCRPKCS7_READ_F_PEM_ONLY        RT_BIT(1)
/** @} */

RTDECL(int) RTCrPkcs7_ReadFromBuffer(PRTCRPKCS7CONTENTINFO pContentInfo, const void *pvBuf, size_t cbBuf,
                                     uint32_t fFlags, PCRTASN1ALLOCATORVTABLE pAllocator,
                                     bool *pfCmsLabeled, PRTERRINFO pErrInfo, const char *pszErrorTag);


/**
 * PKCS \#7 DigestInfo (IPRT representation).
 */
typedef struct RTCRPKCS7DIGESTINFO
{
    /** Sequence core. */
    RTASN1SEQUENCECORE                  SeqCore;
    /** The digest algorithm use to digest the signed content. */
    RTCRX509ALGORITHMIDENTIFIER         DigestAlgorithm;
    /** The digest. */
    RTASN1OCTETSTRING                   Digest;
} RTCRPKCS7DIGESTINFO;
/** Pointer to the IPRT representation of a PKCS \#7 DigestInfo object. */
typedef RTCRPKCS7DIGESTINFO *PRTCRPKCS7DIGESTINFO;
/** Pointer to the const IPRT representation of a PKCS \#7 DigestInfo object. */
typedef RTCRPKCS7DIGESTINFO const *PCRTCRPKCS7DIGESTINFO;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRPKCS7DIGESTINFO, RTDECL, RTCrPkcs7DigestInfo, SeqCore.Asn1Core);


/**
 * Callback function for use with RTCrPkcs7VerifySignedData.
 *
 * @returns IPRT status code.
 * @param   pCert               The certificate to verify.
 * @param   hCertPaths          Unless the certificate is trusted directly, this
 *                              is a reference to the certificate path builder
 *                              and verifier instance that we used to establish
 *                              at least valid trusted path to @a pCert.  The
 *                              callback can use this to enforce additional
 *                              certificate lineage requirements, effective
 *                              policy checks and whatnot.
 *                              This is NIL_RTCRX509CERTPATHS if the certificate
 *                              is directly trusted.
 * @param   fFlags              Mix of the RTCRPKCS7VCC_F_XXX flags.
 * @param   pvUser              The user argument.
 * @param   pErrInfo            Optional error info buffer.
 */
typedef DECLCALLBACKTYPE(int, FNRTCRPKCS7VERIFYCERTCALLBACK,(PCRTCRX509CERTIFICATE pCert, RTCRX509CERTPATHS hCertPaths,
                                                             uint32_t fFlags, void *pvUser, PRTERRINFO pErrInfo));
/** Pointer to a FNRTCRPKCS7VERIFYCERTCALLBACK callback. */
typedef FNRTCRPKCS7VERIFYCERTCALLBACK *PFNRTCRPKCS7VERIFYCERTCALLBACK;

/** @name RTCRPKCS7VCC_F_XXX - Flags for FNRTCRPKCS7VERIFYCERTCALLBACK.
 * @{ */
/** Normal callback for a direct signatory of the signed data. */
#define RTCRPKCS7VCC_F_SIGNED_DATA                      RT_BIT_32(0)
/** Check that the signatory can be trusted for timestamps. */
#define RTCRPKCS7VCC_F_TIMESTAMP                        RT_BIT_32(1)
/** @} */

/**
 * @callback_method_impl{FNRTCRPKCS7VERIFYCERTCALLBACK,
 *  Default implementation that checks for the DigitalSignature KeyUsage bit.}
 */
RTDECL(int) RTCrPkcs7VerifyCertCallbackDefault(PCRTCRX509CERTIFICATE pCert, RTCRX509CERTPATHS hCertPaths, uint32_t fFlags,
                                               void *pvUser, PRTERRINFO pErrInfo);

/**
 * @callback_method_impl{FNRTCRPKCS7VERIFYCERTCALLBACK,
 * Standard code signing.  Use this for Microsoft SPC.}
 */
RTDECL(int) RTCrPkcs7VerifyCertCallbackCodeSigning(PCRTCRX509CERTIFICATE pCert, RTCRX509CERTPATHS hCertPaths, uint32_t fFlags,
                                                   void *pvUser, PRTERRINFO pErrInfo);

/**
 * Verifies PKCS \#7 SignedData.
 *
 * For compatability with alternative crypto providers, the user must work on
 * the top level PKCS \#7 structure instead directly on the SignedData.
 *
 * @returns IPRT status code.
 * @param   pContentInfo        PKCS \#7 content info structure.
 * @param   fFlags              RTCRPKCS7VERIFY_SD_F_XXX.
 * @param   hAdditionalCerts    Store containing additional certificates to
 *                              supplement those mentioned in the signed data.
 * @param   hTrustedCerts       Store containing trusted certificates.
 * @param   pValidationTime     The time we're supposed to validate the
 *                              certificates chains at.  Ignored for signatures
 *                              with valid signing time attributes.
 *                              When RTCRPKCS7VERIFY_SD_F_UPDATE_VALIDATION_TIME
 *                              is set, this is updated to the actual validation
 *                              time used.
 * @param   pfnVerifyCert       Callback for checking that a certificate used
 *                              for signing the data is suitable.
 * @param   pvUser              User argument for the callback.
 * @param   pErrInfo            Optional error info buffer.
 * @sa      RTCrPkcs7VerifySignedDataWithExternalData
 */
RTDECL(int) RTCrPkcs7VerifySignedData(PCRTCRPKCS7CONTENTINFO pContentInfo, uint32_t fFlags,
                                      RTCRSTORE hAdditionalCerts, RTCRSTORE hTrustedCerts,
                                      PCRTTIMESPEC pValidationTime, PFNRTCRPKCS7VERIFYCERTCALLBACK pfnVerifyCert, void *pvUser,
                                      PRTERRINFO pErrInfo);


/**
 * Verifies PKCS \#7 SignedData with external data.
 *
 * For compatability with alternative crypto providers, the user must work on
 * the top level PKCS \#7 structure instead directly on the SignedData.
 *
 * @returns IPRT status code.
 * @param   pContentInfo        PKCS \#7 content info structure.
 * @param   fFlags              RTCRPKCS7VERIFY_SD_F_XXX.
 * @param   hAdditionalCerts    Store containing additional certificates to
 *                              supplement those mentioned in the signed data.
 * @param   hTrustedCerts       Store containing trusted certificates.
 * @param   pValidationTime     The time we're supposed to validate the
 *                              certificates chains at.  Ignored for signatures
 *                              with valid signing time attributes.
 *                              When RTCRPKCS7VERIFY_SD_F_UPDATE_VALIDATION_TIME
 *                              is set, this is updated to the actual validation
 *                              time used.
 * @param   pfnVerifyCert       Callback for checking that a certificate used
 *                              for signing the data is suitable.
 * @param   pvUser              User argument for the callback.
 * @param   pvData              The signed external data.
 * @param   cbData              The size of the signed external data.
 * @param   pErrInfo            Optional error info buffer.
 * @sa      RTCrPkcs7VerifySignedData
 */
RTDECL(int) RTCrPkcs7VerifySignedDataWithExternalData(PCRTCRPKCS7CONTENTINFO pContentInfo, uint32_t fFlags,
                                                      RTCRSTORE hAdditionalCerts, RTCRSTORE hTrustedCerts,
                                                      PCRTTIMESPEC pValidationTime,
                                                      PFNRTCRPKCS7VERIFYCERTCALLBACK pfnVerifyCert, void *pvUser,
                                                      void const *pvData, size_t cbData, PRTERRINFO pErrInfo);

/** @name RTCRPKCS7VERIFY_SD_F_XXX - Flags for RTCrPkcs7VerifySignedData and
 *                                   RTCrPkcs7VerifySignedDataWithExternalData
 * @{ */
/** Always use the signing time attribute if present, requiring it to be
 * verified as valid.  The default behavior is to ignore unverifiable
 * signing time attributes and use the @a pValidationTime instead. */
#define RTCRPKCS7VERIFY_SD_F_ALWAYS_USE_SIGNING_TIME_IF_PRESENT     RT_BIT_32(0)
/** Same as RTCRPKCS7VERIFY_SD_F_ALWAYS_USE_SIGNING_TIME_IF_PRESENT for the MS
 *  timestamp counter signatures. */
#define RTCRPKCS7VERIFY_SD_F_ALWAYS_USE_MS_TIMESTAMP_IF_PRESENT     RT_BIT_32(1)
/** Only use signing time attributes from counter signatures. */
#define RTCRPKCS7VERIFY_SD_F_COUNTER_SIGNATURE_SIGNING_TIME_ONLY    RT_BIT_32(2)
/** Don't validate the counter signature containing the signing time, just use
 * it unverified.  This is useful if we don't necessarily have the root
 * certificates for the timestamp server handy, but use with great care.
 * @sa RTCRPKCS7VERIFY_SD_F_USE_MS_TIMESTAMP_UNVERIFIED */
#define RTCRPKCS7VERIFY_SD_F_USE_SIGNING_TIME_UNVERIFIED            RT_BIT_32(3)
/** Don't validate the MS counter signature containing the signing timestamp.
 * @sa RTCRPKCS7VERIFY_SD_F_USE_SIGNING_TIME_UNVERIFIED */
#define RTCRPKCS7VERIFY_SD_F_USE_MS_TIMESTAMP_UNVERIFIED            RT_BIT_32(4)
/** Do not consider timestamps in microsoft counter signatures. */
#define RTCRPKCS7VERIFY_SD_F_IGNORE_MS_TIMESTAMP                    RT_BIT_32(5)
/** The signed data requires certificates to have the timestamp extended
 * usage bit present.  This is used for recursivly verifying MS timestamp
 * signatures. */
#define RTCRPKCS7VERIFY_SD_F_USAGE_TIMESTAMPING                     RT_BIT_32(6)
/** Skip the verification of the certificate trust paths, taking all
 * certificates to be trustworthy. */
#define RTCRPKCS7VERIFY_SD_F_TRUST_ALL_CERTS                        RT_BIT_32(7)
/** Update @a pValidationTime with the actual validation time used.
 * This requires RTCRPKCS7VERIFY_SD_F_HAS_SIGNER_INDEX to get a consistent
 * result.  And yeah, it unconst the parameter, which is patently ugly. */
#define RTCRPKCS7VERIFY_SD_F_UPDATE_VALIDATION_TIME                 RT_BIT_32(8)
/** Check trust anchors (@sa RTCrX509CertPathsSetTrustAnchorChecks). */
#define RTCRPKCS7VERIFY_SD_F_CHECK_TRUST_ANCHORS                    RT_BIT_32(9)

/** This can be used to only verify one given signer info.
 * Max index value is 15.  */
#define RTCRPKCS7VERIFY_SD_F_SIGNER_INDEX(a_idxSignerInfo) \
    (  RTCRPKCS7VERIFY_SD_F_HAS_SIGNER_INDEX \
     | (((a_idxSignerInfo) & RTCRPKCS7VERIFY_SD_F_SIGNER_INDEX_MAX) << RTCRPKCS7VERIFY_SD_F_SIGNER_INDEX_SHIFT) )
/** Has a valid value in RTCRPKCS7VERIFY_SD_F_SIGNER_INDEX_MASK. */
#define RTCRPKCS7VERIFY_SD_F_HAS_SIGNER_INDEX                       RT_BIT_32(23)
/** Signer index shift value. */
#define RTCRPKCS7VERIFY_SD_F_SIGNER_INDEX_SHIFT                     24
/** Signer index mask. */
#define RTCRPKCS7VERIFY_SD_F_SIGNER_INDEX_MASK                      UINT32_C(0x0f000000)
/** Max signer index value (inclusive). */
#define RTCRPKCS7VERIFY_SD_F_SIGNER_INDEX_MAX \
    (RTCRPKCS7VERIFY_SD_F_SIGNER_INDEX_MASK >> RTCRPKCS7VERIFY_SD_F_SIGNER_INDEX_SHIFT)

/** Indicates internally that we're validating a counter signature and should
 * use different rules when checking out the authenticated attributes.
 * @internal  */
#define RTCRPKCS7VERIFY_SD_F_COUNTER_SIGNATURE                      RT_BIT_32(31)
/** @} */


RTDECL(int) RTCrPkcs7SimpleSignSignedData(uint32_t fFlags, PCRTCRX509CERTIFICATE pSigner, RTCRKEY hPrivateKey,
                                          void const *pvData, size_t cbData, RTDIGESTTYPE enmDigestType,
                                          RTCRSTORE hAdditionalCerts, PCRTCRPKCS7ATTRIBUTES pAdditionalAuthenticatedAttribs,
                                          void *pvResult, size_t *pcbResult, PRTERRINFO pErrInfo);

/** @name RTCRPKCS7SIGN_SD_F_XXX - Flags for RTCrPkcs7SimpleSign.
 * @{ */
/** Detached data. */
#define RTCRPKCS7SIGN_SD_F_DEATCHED         RT_BIT_32(0)
/** No SMIME capabilities attribute. */
#define RTCRPKCS7SIGN_SD_F_NO_SMIME_CAP     RT_BIT_32(1)
/** Produce version 1 output (PKCS\#7), rather than version 3 (CMS). */
#define RTCRPKCS7SIGN_SD_F_USE_V1           RT_BIT_32(2)
/** Avoid extra OCTET STRING encapsulation around the data blob.
 * This is needed for Authenticode signatures.  This requires that the
 * content type is supplied via the additional authenticated attributes.
 * @note Currently only works with RTCRPKCS7SIGN_SD_F_USE_V1.  */
#define RTCRPKCS7SIGN_SD_F_NO_DATA_ENCAP    RT_BIT_32(3)
/** Valid flag mask.   */
#define RTCRPKCS7SIGN_SD_F_VALID_MASK       UINT32_C(0x0000000f)
/** @} */

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_crypto_pkcs7_h */

