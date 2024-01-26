/********************************************************************************/
/*										*/
/*	Macro and Structure Definitions for the X509 Commands and Functions.	*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: X509.h $			*/
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
/*  (c) Copyright IBM Corp. and others, 2019 - 2021				*/
/*										*/
/********************************************************************************/

// 10.1.16	X509.h
// 10.1.16.1	Introduction
// This file contains the macro and structure definitions for the X509 commands and functions.
#ifndef _X509_H_
#define _X509_H_
// 10.1.16.2	Includes
#include "Tpm.h"
#include "TpmAsn1.h"
// 10.1.16.3	Defined Constants
// 10.1.16.3.1	X509 Application-specific types
#define X509_SELECTION          0xA0
#define X509_ISSUER_UNIQUE_ID   0xA1
#define X509_SUBJECT_UNIQUE_ID  0xA2
#define X509_EXTENSIONS         0xA3
// These defines give the order in which values appear in the TBScertificate of an x.509
// certificate. These values are used to index into an array of
#define ENCODED_SIZE_REF        0
#define VERSION_REF             (ENCODED_SIZE_REF + 1)
#define SERIAL_NUMBER_REF       (VERSION_REF + 1)
#define SIGNATURE_REF           (SERIAL_NUMBER_REF + 1)
#define ISSUER_REF              (SIGNATURE_REF + 1)
#define VALIDITY_REF            (ISSUER_REF + 1)
#define SUBJECT_KEY_REF         (VALIDITY_REF + 1)
#define SUBJECT_PUBLIC_KEY_REF  (SUBJECT_KEY_REF + 1)
#define EXTENSIONS_REF          (SUBJECT_PUBLIC_KEY_REF + 1)
#define REF_COUNT               (EXTENSIONS_REF + 1)

// 10.1.16.4 Structures Used to access the fields of a TBSsignature some of which are in the
// in_CertifyX509 structure and some of which are in the out_CertifyX509 structure.
typedef struct stringRef
{
    BYTE        *buf;
    INT16        len;
} stringRef;
// This is defined to avoid bit by bit comparisons within a UINT32
typedef union x509KeyUsageUnion {
    TPMA_X509_KEY_USAGE     x509;
    UINT32                  integer;
} x509KeyUsageUnion;

// 10.1.16.5	Global X509 Constants

// These values are instanced by X509_spt.c and referenced by other X509-related files. This is the
// DER-encoded value for the Key Usage OID (2.5.29.15). This is the full OID, not just the numeric
// value

#define OID_KEY_USAGE_EXTENSION_VALUE  0x06, 0x03, 0x55, 0x1D, 0x0F
MAKE_OID(_KEY_USAGE_EXTENSION);

// This is the DER-encoded value for the TCG-defined TPMA_OBJECT OID (2.23.133.10.1.1.1)

#define OID_TCG_TPMA_OBJECT_VALUE       0x06, 0x07, 0x67, 0x81, 0x05, 0x0a, 0x01, \
	0x01, 0x01
MAKE_OID(_TCG_TPMA_OBJECT);

#ifdef _X509_SPT_

// If a bit is SET in KEY_USAGE_SIGN is also SET in keyUsage then the associated key has to have
// sign SET.

const x509KeyUsageUnion KEY_USAGE_SIGN =
    {TPMA_X509_KEY_USAGE_INITIALIZER(
				    /* bits_at_0        */ 0, /* decipheronly    */ 0,  /* encipheronly   */ 0,
				    /* crlsign          */ 1, /* keycertsign     */ 1,  /* keyagreement   */ 0,
				    /* dataencipherment */ 0, /* keyencipherment */ 0,  /* nonrepudiation */ 0,
				    /* digitalsignature */ 1)};

// If a bit is SET in KEY_USAGE_DECRYPT is also SET in keyUsage then the associated key has to have decrypt SET.

const x509KeyUsageUnion KEY_USAGE_DECRYPT =
    {TPMA_X509_KEY_USAGE_INITIALIZER(
				    /* bits_at_0        */ 0, /* decipheronly    */ 1,  /* encipheronly   */ 1,
				    /* crlsign          */ 0, /* keycertsign     */ 0,  /* keyagreement   */ 1,
				    /* dataencipherment */ 1, /* keyencipherment */ 1,  /* nonrepudiation */ 0,
				    /* digitalsignature */ 0)};
#else
extern x509KeyUsageUnion KEY_USAGE_SIGN;
extern x509KeyUsageUnion KEY_USAGE_DECRYPT;
#endif

#endif // _X509_H_
