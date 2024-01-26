/********************************************************************************/
/*										*/
/*			     TPM X509 ECC					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: X509_ECC.c $		*/
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

/* 10.2.24	X509_ECC.c */
/* 10.2.24.1	Includes */
#include "Tpm.h"
#include "X509.h"
#include "OIDs.h"
#include "TpmAsn1_fp.h"
#include "X509_ECC_fp.h"
#include "X509_spt_fp.h"
#include "CryptHash_fp.h"

/* 10.2.24.2	Functions */
/* 10.2.24.2.1	X509PushPoint() */
/* This seems like it might be used more than once so... */
/* Return Value	Meaning */
/* > 0	number of bytes added */
/* == 0	failure */
INT16
X509PushPoint(
	      ASN1MarshalContext      *ctx,
	      TPMS_ECC_POINT          *p
	      )
{
    // Push a bit string containing the public key. For now, push the x, and y
    // coordinates of the public point, bottom up
    ASN1StartMarshalContext(ctx); // BIT STRING
    {
	ASN1PushBytes(ctx, p->y.t.size, p->y.t.buffer);
	ASN1PushBytes(ctx, p->x.t.size, p->x.t.buffer);
	ASN1PushByte(ctx, 0x04);
    }
    return ASN1EndEncapsulation(ctx, ASN1_BITSTRING); // Ends BIT STRING
}
/* 10.2.24.2.2	X509AddSigningAlgorithmECC() */
/* This creates the singing algorithm data. */
/* Return Value	Meaning */
/* > 0	number of bytes added */
/* == 0	failure */
INT16
X509AddSigningAlgorithmECC(
			   OBJECT              *signKey,
			   TPMT_SIG_SCHEME     *scheme,
			   ASN1MarshalContext  *ctx
			   )
{
    PHASH_DEF            hashDef = CryptGetHashDef(scheme->details.any.hashAlg);
    //
    NOT_REFERENCED(signKey);
    // If the desired hashAlg definition wasn't found...
    if(hashDef->hashAlg != scheme->details.any.hashAlg)
	return 0;

    switch(scheme->scheme)
	{
#if ALG_ECDSA
	  case TPM_ALG_ECDSA:
	    // Make sure that we have an OID for this hash and ECC
	    if((hashDef->ECDSA)[0] != ASN1_OBJECT_IDENTIFIER)
		break;
	    // if this is just an implementation check, indicate that this
	    // combination is supported
	    if(!ctx)
		return 1;
	    ASN1StartMarshalContext(ctx);
	    ASN1PushOID(ctx, hashDef->ECDSA);
	    return ASN1EndEncapsulation(ctx, ASN1_CONSTRUCTED_SEQUENCE);
#endif	// ALG_ECDSA
	  default:
	    break;
	}
    return 0;
}
/* 10.2.24.2.3	X509AddPublicECC() */
/* This function will add the publicKey description to the DER data. If ctx is NULL, then no data is
   transferred and this function will indicate if the TPM has the values for DER-encoding of the
   public key. */
/*     Return Value	Meaning */
/*     > 0	number of bytes added */
/*     == 0	failure */
INT16
X509AddPublicECC(
		 OBJECT                *object,
		 ASN1MarshalContext    *ctx
		 )
{
    const BYTE      *curveOid =
	CryptEccGetOID(object->publicArea.parameters.eccDetail.curveID);
    if((curveOid == NULL) || (*curveOid != ASN1_OBJECT_IDENTIFIER))
	return 0;
    //
    //
    //  SEQUENCE (2 elem) 1st
    //    SEQUENCE (2 elem) 2nd
    //      OBJECT IDENTIFIER 1.2.840.10045.2.1 ecPublicKey (ANSI X9.62 public key type)
    //      OBJECT IDENTIFIER 1.2.840.10045.3.1.7 prime256v1 (ANSI X9.62 named curve)
    //    BIT STRING (520 bit) 000001001010000111010101010111001001101101000100000010...
    //
    // If this is a check to see if the key can be encoded, it can.
    // Need to mark the end sequence
    if(ctx == NULL)
	return 1;
    ASN1StartMarshalContext(ctx); // SEQUENCE (2 elem) 1st
    {
	X509PushPoint(ctx, &object->publicArea.unique.ecc); // BIT STRING
	ASN1StartMarshalContext(ctx); // SEQUENCE (2 elem) 2nd
	{
	    ASN1PushOID(ctx, curveOid); // curve dependent
	    ASN1PushOID(ctx, OID_ECC_PUBLIC); // (1.2.840.10045.2.1)
	}
	ASN1EndEncapsulation(ctx, ASN1_CONSTRUCTED_SEQUENCE); // Ends SEQUENCE 2nd
    }
    return ASN1EndEncapsulation(ctx, ASN1_CONSTRUCTED_SEQUENCE); // Ends SEQUENCE 1st
}
