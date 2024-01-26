/********************************************************************************/
/*										*/
/*			OID values						*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: OIDs.h $			*/
/*										*/
/*  Licenses and Notices							*/
/*										*/
/*  1. Copyright Licenses:							*/
/*										*/
/*  - Trusted Computing Group (TCG) grants to the user of the source code in	*/
/*    this specification (the "Source Code") a worldwide, irrevocable, 		*/
/*    nonexclusive, royalty free, copyright license to reproduce, create 	*/
/*    derivative works, distribute, display and perform the Source Code and	*/
/*    derivative works thereof, and to grant others the rights granted herein.	*/
/*										*/
/*  - The TCG grants to the user of the other parts of the specification 	*/
/*    (other than the Source Code) the rights to reproduce, distribute, 	*/
/*    display, and perform the specification solely for the purpose of 		*/
/*    developing products based on such documents.				*/
/*										*/
/*  2. Source Code Distribution Conditions:					*/
/*										*/
/*  - Redistributions of Source Code must retain the above copyright licenses, 	*/
/*    this list of conditions and the following disclaimers.			*/
/*										*/
/*  - Redistributions in binary form must reproduce the above copyright 	*/
/*    licenses, this list of conditions	and the following disclaimers in the 	*/
/*    documentation and/or other materials provided with the distribution.	*/
/*										*/
/*  3. Disclaimers:								*/
/*										*/
/*  - THE COPYRIGHT LICENSES SET FORTH ABOVE DO NOT REPRESENT ANY FORM OF	*/
/*  LICENSE OR WAIVER, EXPRESS OR IMPLIED, BY ESTOPPEL OR OTHERWISE, WITH	*/
/*  RESPECT TO PATENT RIGHTS HELD BY TCG MEMBERS (OR OTHER THIRD PARTIES)	*/
/*  THAT MAY BE NECESSARY TO IMPLEMENT THIS SPECIFICATION OR OTHERWISE.		*/
/*  Contact TCG Administration (admin@trustedcomputinggroup.org) for 		*/
/*  information on specification licensing rights available through TCG 	*/
/*  membership agreements.							*/
/*										*/
/*  - THIS SPECIFICATION IS PROVIDED "AS IS" WITH NO EXPRESS OR IMPLIED 	*/
/*    WARRANTIES WHATSOEVER, INCLUDING ANY WARRANTY OF MERCHANTABILITY OR 	*/
/*    FITNESS FOR A PARTICULAR PURPOSE, ACCURACY, COMPLETENESS, OR 		*/
/*    NONINFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS, OR ANY WARRANTY 		*/
/*    OTHERWISE ARISING OUT OF ANY PROPOSAL, SPECIFICATION OR SAMPLE.		*/
/*										*/
/*  - Without limitation, TCG and its members and licensors disclaim all 	*/
/*    liability, including liability for infringement of any proprietary 	*/
/*    rights, relating to use of information in this specification and to the	*/
/*    implementation of this specification, and TCG disclaims all liability for	*/
/*    cost of procurement of substitute goods or services, lost profits, loss 	*/
/*    of use, loss of data or any incidental, consequential, direct, indirect, 	*/
/*    or special damages, whether under contract, tort, warranty or otherwise, 	*/
/*    arising in any way out of use or reliance upon this specification or any 	*/
/*    information herein.							*/
/*										*/
/*  (c) Copyright IBM Corp. and others, 2019 - 2020				*/
/*										*/
/********************************************************************************/

// 10.1.16	OIDs.h

#include "Tpm.h"

#ifndef _OIDS_H_
#define _OIDS_H_

// All the OIDs in this file are defined as DER-encoded values with a leading tag 0x06
// (ASN1_OBJECT_IDENTIFIER), followed by a single length byte. This allows the OID size to be
// determined by looking at octet[1] of the OID (total size is OID[1] + 2).

// These macros allow OIDs to be defined (or not) depending on whether the associated hash
// algorithm is implemented.

// NOTE: When one of these macros is used, the NAME needs '_" on each side. The exception is when
// the macro is used for the hash OID when only a single _ is used.

#ifndef ALG_SHA1
#   define ALG_SHA1 NO
#endif
#if ALG_SHA1
#define SHA1_OID(NAME)    MAKE_OID(NAME##SHA1)
#else
#define SHA1_OID(NAME)
#endif
#ifndef ALG_SHA256
#   define ALG_SHA256 NO
#endif
#if ALG_SHA256
#define SHA256_OID(NAME)  MAKE_OID(NAME##SHA256)
#else
#define SHA256_OID(NAME)
#endif
#ifndef ALG_SHA384
#   define ALG_SHA384 NO
#endif
#if ALG_SHA384
#define SHA384_OID(NAME)  MAKE_OID(NAME##SHA384)
#else
#define SHA384_OID(NAME)
#endif
#ifndef ALG_SHA512
#   define ALG_SHA512 NO
#endif
#if ALG_SHA512
#define SHA512_OID(NAME)  MAKE_OID(NAME##SHA512)
#else
#define SHA512_OID(NAME)
#endif
#ifndef ALG_SM3_256
#   define ALG_SM3_256 NO
#endif
#if ALG_SM3_256
#define SM3_256_OID(NAME) MAKE_OID(NAME##SM3_256)
#else
#define SM3_256_OID(NAME)
#endif
#ifndef ALG_SHA3_256
#   define ALG_SHA3_256 NO
#endif
#if ALG_SHA3_256
#define SHA3_256_OID(NAME) MAKE_OID(NAME##SHA3_256)
#else
#define SHA3_256_OID(NAME)
#endif
#ifndef ALG_SHA3_384
#   define ALG_SHA3_384 NO
#endif
#if ALG_SHA3_384
#define SHA3_384_OID(NAME) MAKE_OID(NAME##SHA3_384)
#else
#define SHA3_384_OID(NAME)
#endif
#ifndef ALG_SHA3_512
#   define ALG_SHA3_512 NO
#endif
#if ALG_SHA3_512
#define SHA3_512_OID(NAME) MAKE_OID(NAME##SHA3_512)
#else
#define SHA3_512_OID(NAME)
#endif
// These are encoded to take one additional byte of algorithm selector
#define NIST_HASH       0x06, 0x09, 0x60, 0x86, 0x48, 1, 101, 3, 4, 2
#define NIST_SIG        0x06, 0x09, 0x60, 0x86, 0x48, 1, 101, 3, 4, 3

// These hash OIDs used in a lot of places.
#define OID_SHA1_VALUE              0x06, 0x05, 0x2B, 0x0E, 0x03, 0x02, 0x1A
SHA1_OID(_);        // Expands to:
//     MAKE_OID(_SHA1)
// which expands to:
//     EXTERN  const BYTE OID_SHA1[] INITIALIZER({OID_SHA1_VALUE})
// which, depending on the setting of EXTERN and
// INITIALIZER, expands to either:
//      extern const BYTE    OID_SHA1[]
// or
//      const BYTE           OID_SHA1[] = {OID_SHA1_VALUE}
// which is:
//      const BYTE           OID_SHA1[] = {0x06, 0x05, 0x2B, 0x0E,
//                                         0x03, 0x02, 0x1A}
#define OID_SHA256_VALUE            NIST_HASH, 1
SHA256_OID(_);
#define OID_SHA384_VALUE            NIST_HASH, 2
SHA384_OID(_);
#define OID_SHA512_VALUE            NIST_HASH, 3
SHA512_OID(_);
#define OID_SM3_256_VALUE           0x06, 0x08, 0x2A, 0x81, 0x1C, 0xCF, 0x55, 0x01, \
	0x83, 0x11
SM3_256_OID(_);         // (1.2.156.10197.1.401)
#define OID_SHA3_256_VALUE          NIST_HASH, 8
SHA3_256_OID(_);
#define OID_SHA3_384_VALUE          NIST_HASH, 9
SHA3_384_OID(_);
#define OID_SHA3_512_VALUE          NIST_HASH, 10
SHA3_512_OID(_);
// These are used for RSA-PSS
#if ALG_RSA
#define OID_MGF1_VALUE              0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, \
	0x01, 0x01, 0x08
MAKE_OID(_MGF1);
#define OID_RSAPSS_VALUE            0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, \
	0x01, 0x01, 0x0A
MAKE_OID(_RSAPSS);
// This is the OID to designate the public part of an RSA key.
#define OID_PKCS1_PUB_VALUE         0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, \
	0x01, 0x01, 0x01
MAKE_OID(_PKCS1_PUB);
// These are used for RSA PKCS1 signature Algorithms
#define OID_PKCS1_SHA1_VALUE        0x06,0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, \
	0x0D, 0x01, 0x01, 0x05
SHA1_OID(_PKCS1_);      // (1.2.840.113549.1.1.5)
#define OID_PKCS1_SHA256_VALUE      0x06,0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, \
	0x0D, 0x01, 0x01, 0x0B
SHA256_OID(_PKCS1_);    // (1.2.840.113549.1.1.11)
#define OID_PKCS1_SHA384_VALUE      0x06,0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, \
	0x0D, 0x01, 0x01, 0x0C
SHA384_OID(_PKCS1_);    // (1.2.840.113549.1.1.12)
#define OID_PKCS1_SHA512_VALUE      0x06,0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, \
	0x0D, 0x01, 0x01, 0x0D
SHA512_OID(_PKCS1_);    //(1.2.840.113549.1.1.13)
#define OID_PKCS1_SM3_256_VALUE     0x06, 0x08, 0x2A, 0x81, 0x1C, 0xCF, 0x55, \
	0x01, 0x83, 0x78
SM3_256_OID(_PKCS1_);   // 1.2.156.10197.1.504
#define OID_PKCS1_SHA3_256_VALUE    NIST_SIG, 14
SHA3_256_OID(_PKCS1_);
#define OID_PKCS1_SHA3_384_VALUE    NIST_SIG, 15
SHA3_384_OID(_PKCS1_);
#define OID_PKCS1_SHA3_512_VALUE    NIST_SIG, 16
SHA3_512_OID(_PKCS1_);
#endif // ALG_RSA
#if ALG_ECDSA
#define OID_ECDSA_SHA1_VALUE        0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, \
	0x01
SHA1_OID(_ECDSA_);      // (1.2.840.10045.4.1) SHA1 digest signed by an ECDSA key.
#define OID_ECDSA_SHA256_VALUE      0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, \
	0x03, 0x02
SHA256_OID(_ECDSA_);    // (1.2.840.10045.4.3.2) SHA256 digest signed by an ECDSA key.
#define OID_ECDSA_SHA384_VALUE      0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, \
	0x03, 0x03
SHA384_OID(_ECDSA_);    // (1.2.840.10045.4.3.3) SHA384 digest signed by an ECDSA key.
#define OID_ECDSA_SHA512_VALUE      0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, \
	0x03, 0x04
SHA512_OID(_ECDSA_);    // (1.2.840.10045.4.3.4) SHA512 digest signed by an ECDSA key.
#define OID_ECDSA_SM3_256_VALUE     0x06, 0x08, 0x2A, 0x81, 0x1C, 0xCF, 0x55, 0x01, \
	0x83, 0x75
SM3_256_OID(_ECDSA_);   // 1.2.156.10197.1.501
#define OID_ECDSA_SHA3_256_VALUE    NIST_SIG, 10
SHA3_256_OID(_ECDSA_);
#define OID_ECDSA_SHA3_384_VALUE    NIST_SIG, 11
SHA3_384_OID(_ECDSA_);
#define OID_ECDSA_SHA3_512_VALUE    NIST_SIG, 12
SHA3_512_OID(_ECDSA_);
#endif // ALG_ECDSA
#if ALG_ECC
#define OID_ECC_PUBLIC_VALUE        0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, \
	0x01
MAKE_OID(_ECC_PUBLIC);
#define OID_ECC_NIST_P192_VALUE     0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, \
	0x01, 0x01
#if ECC_NIST_P192
MAKE_OID(_ECC_NIST_P192);   // (1.2.840.10045.3.1.1) 'nistP192'
#endif // ECC_NIST_P192
#define OID_ECC_NIST_P224_VALUE     0x06, 0x05, 0x2B, 0x81, 0x04, 0x00, 0x21
#if ECC_NIST_P224
MAKE_OID(_ECC_NIST_P224);   // (1.3.132.0.33)        'nistP224'
#endif // ECC_NIST_P224
#define OID_ECC_NIST_P256_VALUE     0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, \
	0x01, 0x07
#if ECC_NIST_P256
MAKE_OID(_ECC_NIST_P256);   // (1.2.840.10045.3.1.7)  'nistP256'
#endif // ECC_NIST_P256
#define OID_ECC_NIST_P384_VALUE     0x06, 0x05, 0x2B, 0x81, 0x04, 0x00, 0x22
#if ECC_NIST_P384
MAKE_OID(_ECC_NIST_P384);   // (1.3.132.0.34)         'nistP384'
#endif // ECC_NIST_P384
#define OID_ECC_NIST_P521_VALUE     0x06, 0x05, 0x2B, 0x81, 0x04, 0x00, 0x23
#if ECC_NIST_P521
MAKE_OID(_ECC_NIST_P521);   // (1.3.132.0.35)         'nistP521'
#endif // ECC_NIST_P521
// No OIDs defined for these anonymous curves
#define OID_ECC_BN_P256_VALUE       0x00
#if ECC_BN_P256
MAKE_OID(_ECC_BN_P256);
#endif // ECC_BN_P256
#define OID_ECC_BN_P638_VALUE       0x00
#if ECC_BN_P638
MAKE_OID(_ECC_BN_P638);
#endif // ECC_BN_P638
#define OID_ECC_SM2_P256_VALUE      0x06, 0x08, 0x2A, 0x81, 0x1C, 0xCF, 0x55, 0x01, \
	0x82, 0x2D
#if ECC_SM2_P256
MAKE_OID(_ECC_SM2_P256);    // Don't know where I found this OID. It needs checking
#endif // ECC_SM2_P256
#if ECC_BN_P256
#define OID_ECC_BN_P256     NULL
#endif // ECC_BN_P256
#endif // ALG_ECC
// #undef MAKE_OID
#define OID_SIZE(OID)   (OID[1] + 2)
#endif // !_OIDS_H_
