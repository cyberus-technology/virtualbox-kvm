/** @file
 * IPRT - Crypto - Trust Anchor Format (RFC-5914).
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

#ifndef IPRT_INCLUDED_crypto_taf_h
#define IPRT_INCLUDED_crypto_taf_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/asn1.h>
#include <iprt/crypto/x509.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_crtaf RTCrTaf - Trust Anchor Format (RFC-5914)
 * @ingroup grp_rt_crypto
 * @{
 */


/**
 * RFC-5914 CertPathControls (IPRT representation).
 */
typedef struct RTCRTAFCERTPATHCONTROLS
{
    /** Sequence core. */
    RTASN1SEQUENCECORE                  SeqCore;
    /** The trust anchor subject.  For use in path construction. */
    RTCRX509NAME                        TaName;
    /** Certificate, optional, implicit tag 0. */
    RTCRX509CERTIFICATE                 Certificate;
    /** Certificate policies, optional, implicit tag 1.
     * @remarks This is an ASN.1 SEQUENCE, not an ASN.1 SET as the name
     *          mistakenly might be taken to indicate. */
    RTCRX509CERTIFICATEPOLICIES         PolicySet;
    /** Policy flags, optional, implicit tag 2. */
    RTASN1BITSTRING                     PolicyFlags;
    /** Name constraints, optional, implicit tag 3. */
    RTCRX509NAMECONSTRAINTS             NameConstr;
    /** Path length constraints, optional, implicit tag 4. */
    RTASN1INTEGER                       PathLenConstraint;
} RTCRTAFCERTPATHCONTROLS;
/** Pointer to the IPRT representation of a RFC-5914 CertPathControls. */
typedef RTCRTAFCERTPATHCONTROLS *PRTCRTAFCERTPATHCONTROLS;
/** Pointer to the const IPRT representation of a RFC-5914 CertPathControls. */
typedef RTCRTAFCERTPATHCONTROLS const *PCRTCRTAFCERTPATHCONTROLS;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRTAFCERTPATHCONTROLS, RTDECL, RTCrTafCertPathControls, SeqCore.Asn1Core);

/** @name Bit definitions for RTCRTAFCERTPATHCONTROL::PolicyFlags
 * @{ */
#define RTCRTAFCERTPOLICYFLAGS_INHIBIT_POLICY_MAPPING   0
#define RTCRTAFCERTPOLICYFLAGS_REQUIRE_EXPLICIT_POLICY  1
#define RTCRTAFCERTPOLICYFLAGS_INHIBIT_ANY_POLICY       2
/** @} */


/**
 * RFC-5914 TrustAnchorInfo (IPRT representation).
 */
typedef struct RTCRTAFTRUSTANCHORINFO
{
    /** Sequence core. */
    RTASN1SEQUENCECORE                  SeqCore;
    /** The version number (defaults to v1). */
    RTASN1INTEGER                       Version;
    /** The public key of the trust anchor. */
    RTCRX509SUBJECTPUBLICKEYINFO        PubKey;
    /** Key identifier. */
    RTASN1OCTETSTRING                   KeyIdentifier;
    /** Trust anchor title, optional, size 1 to 64. */
    RTASN1STRING                        TaTitle;
    /** Certificate path controls, optional. */
    RTCRTAFCERTPATHCONTROLS             CertPath;
    /** Extensions, explicit optional, context tag 1.  */
    struct
    {
        /** Context tag 1. */
        RTASN1CONTEXTTAG1               CtxTag1;
        /** The extensions. */
        RTCRX509EXTENSIONS              Exts;
    } T1;
    /** Title language tag, implicit optional, context tag 2.
     * Defaults to "en". */
    RTASN1STRING                        TaTitleLangTag;
} RTCRTAFTRUSTANCHORINFO;
/** Pointer to the IPRT representation of a RFC-5914 TrustAnchorInfo. */
typedef RTCRTAFTRUSTANCHORINFO *PRTCRTAFTRUSTANCHORINFO;
/** Pointer to the const IPRT representation of a RFC-5914 TrustAnchorInfo. */
typedef RTCRTAFTRUSTANCHORINFO const *PCRTCRTAFTRUSTANCHORINFO;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRTAFTRUSTANCHORINFO, RTDECL, RTCrTafTrustAnchorInfo, SeqCore.Asn1Core);

/** TrustAnchorInfo version 1.  */
#define RTCRTAFTRUSTANCHORINFO_V1       1


/** Indicates what kind of value a TrustAnchorChoice structure contains. */
typedef enum RTCRTAFTRUSTANCHORCHOICEVAL
{
    /** Invalid zero value. */
    RTCRTAFTRUSTANCHORCHOICEVAL_INVALID = 0,
    /** RTCRTAFTRUSTANCHORCHOICE::u.pCertificate. */
    RTCRTAFTRUSTANCHORCHOICEVAL_CERTIFICATE,
    /** RTCRTAFTRUSTANCHORCHOICE::u.pT1. */
    RTCRTAFTRUSTANCHORCHOICEVAL_TBS_CERTIFICATE,
    /** RTCRTAFTRUSTANCHORCHOICE::u.pT2. */
    RTCRTAFTRUSTANCHORCHOICEVAL_TRUST_ANCHOR_INFO,
    /** End of valid choices. */
    RTCRTAFTRUSTANCHORCHOICEVAL_END,
    /** Make sure it's (at least) 32-bit wide. */
    RTCRTAFTRUSTANCHORCHOICEVAL_32BIT_HACK = 0x7fffffff
} RTCRTAFTRUSTANCHORCHOICEVAL;


/**
 * RFC-5914 TrustAnchorChoice (IPRT representation).
 */
typedef struct RTCRTAFTRUSTANCHORCHOICE
{
    /** Dummy object for simplifying everything.   */
    RTASN1DUMMY                         Dummy;
    /** Allocation for the valid member (to optimize space usage). */
    RTASN1ALLOCATION                    Allocation;
    /** Indicates which of the pointers are valid. */
    RTCRTAFTRUSTANCHORCHOICEVAL         enmChoice;
    /** Choice union. */
    union
    {
        /** Generic ASN.1 core pointer for the choice.  */
        PRTASN1CORE                     pAsn1Core;
        /** Choice 0: X509 certificate.  */
        PRTCRX509CERTIFICATE            pCertificate;
        /** Choice 1: To-be-signed certificate part.  This may differ from the
         * TBSCertificate member of the original certificate. */
        struct
        {
            /** Explicit context tag. */
            RTASN1CONTEXTTAG1           CtxTag1;
            /** Pointer to the TBS certificate structure. */
            RTCRX509TBSCERTIFICATE      TbsCert;
        } *pT1;

        /** Choice 2: To-be-signed certificate part.  This may differ from the
         * TBSCertificate member of the original certificate. */
        struct
        {
            /** Explicit context tag. */
            RTASN1CONTEXTTAG2           CtxTag2;
            /** Pointer to the trust anchor infomration structure. */
            RTCRTAFTRUSTANCHORINFO      TaInfo;
        } *pT2;
    } u;
} RTCRTAFTRUSTANCHORCHOICE;
/** Pointer to the IPRT representation of a RFC-5914 TrustAnchorChoice. */
typedef RTCRTAFTRUSTANCHORCHOICE *PRTCRTAFTRUSTANCHORCHOICE;
/** Pointer to the const IPRT representation of a RFC-5914 TrustAnchorChoice. */
typedef RTCRTAFTRUSTANCHORCHOICE const *PCRTCRTAFTRUSTANCHORCHOICE;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRTAFTRUSTANCHORCHOICE, RTDECL, RTCrTafTrustAnchorChoice, Dummy.Asn1Core);

/*
 * RFC-5914 TrustAnchorList (IPRT representation).
 */
RTASN1_IMPL_GEN_SEQ_OF_TYPEDEFS_AND_PROTOS(RTCRTAFTRUSTANCHORLIST, RTCRTAFTRUSTANCHORCHOICE, RTDECL, RTCrTafTrustAnchorList);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_crypto_taf_h */

