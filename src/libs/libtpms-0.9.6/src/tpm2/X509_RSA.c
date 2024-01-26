/********************************************************************************/
/*										*/
/*			     TPM X509 RSA					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: X509_RSA.c $		*/
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

/* 10.2.25	X509_RSA.c */
/* 10.2.25.1	Includes */
#include "Tpm.h"
#include "X509.h"
#include "TpmAsn1_fp.h"
#include "X509_RSA_fp.h"
#include "X509_spt_fp.h"
#include "CryptHash_fp.h"
#include "CryptRsa_fp.h"

/* 10.2.25.2	Functions */
#if ALG_RSA
/* 10.2.25.2.1	X509AddSigningAlgorithmRSA() */
/* This creates the singing algorithm data. */
/* Return Value	Meaning */
/* > 0	number of bytes added */
/* == 0	failure */
INT16
X509AddSigningAlgorithmRSA(
			   OBJECT              *signKey,
			   TPMT_SIG_SCHEME     *scheme,
			   ASN1MarshalContext  *ctx
			   )
{
    TPM_ALG_ID           hashAlg = scheme->details.any.hashAlg;
    PHASH_DEF            hashDef = CryptGetHashDef(hashAlg);
    //
    NOT_REFERENCED(signKey);
    // return failure if hash isn't implemented
    if(hashDef->hashAlg != hashAlg)
	return 0;
    switch(scheme->scheme)
    {
        case TPM_ALG_RSASSA:
        {
	    // if the hash is implemented but there is no PKCS1 OID defined
	    // then this is not a valid signing combination.
	    if(hashDef->PKCS1[0] != ASN1_OBJECT_IDENTIFIER)
		break;
	    if(ctx == NULL)
		return 1;
	    return X509PushAlgorithmIdentifierSequence(ctx, hashDef->PKCS1);
        }
        case TPM_ALG_RSAPSS:
            // leave if this is just an implementation check
            if(ctx == NULL)
                return 1;
            // In the case of SHA1, everything is default and RFC4055 says that 
            // implementations that do signature generation MUST omit the parameter
            // when defaults are used. )-:
            if(hashDef->hashAlg == TPM_ALG_SHA1)
            {
                return X509PushAlgorithmIdentifierSequence(ctx, OID_RSAPSS);
            }
            else
            {
                // Going to build something that looks like:
                //  SEQUENCE (2 elem)
                //     OBJECT IDENTIFIER 1.2.840.113549.1.1.10 rsaPSS (PKCS #1)
                //     SEQUENCE (3 elem)
                //       [0] (1 elem)
                //         SEQUENCE (2 elem)
                //           OBJECT IDENTIFIER 2.16.840.1.101.3.4.2.1 sha-256 
                //           NULL
                //       [1] (1 elem)
                //         SEQUENCE (2 elem)
                //           OBJECT IDENTIFIER 1.2.840.113549.1.1.8 pkcs1-MGF
                //           SEQUENCE (2 elem)
                //             OBJECT IDENTIFIER 2.16.840.1.101.3.4.2.1 sha-256
                //             NULL
                //       [2] (1 elem)  salt length
                //         INTEGER 32

                // The indentation is just to keep track of where we are in the 
                // structure
                ASN1StartMarshalContext(ctx); // SEQUENCE (2 elements)
                {
                    ASN1StartMarshalContext(ctx);   // SEQUENCE (3 elements)
                    {
                        // [2] (1 elem)  salt length
                        //    INTEGER 32
                        ASN1StartMarshalContext(ctx);
                        {
                            INT16       saltSize =
                                CryptRsaPssSaltSize((INT16)hashDef->digestSize,
                                (INT16)signKey->publicArea.unique.rsa.t.size);
                            ASN1PushUINT(ctx, saltSize);
                        }
                        ASN1EndEncapsulation(ctx, ASN1_APPLICAIION_SPECIFIC + 2);

                        // Add the mask generation algorithm
                        // [1] (1 elem)
                        //    SEQUENCE (2 elem) 1st
                        //      OBJECT IDENTIFIER 1.2.840.113549.1.1.8 pkcs1-MGF
                        //      SEQUENCE (2 elem) 2nd  
                        //        OBJECT IDENTIFIER 2.16.840.1.101.3.4.2.1 sha-256
                        //        NULL
                        ASN1StartMarshalContext(ctx);   // mask context [1] (1 elem)
                        {
                            ASN1StartMarshalContext(ctx);   // SEQUENCE (2 elem) 1st
                            // Handle the 2nd Sequence (sequence (object, null))
                            {
                                // This adds a NULL, then an OID and a SEQUENCE
                                // wrapper.
                                X509PushAlgorithmIdentifierSequence(ctx,
                                    hashDef->OID);
                                // add the pkcs1-MGF OID 
                                ASN1PushOID(ctx, OID_MGF1);
                            }
                            // End outer sequence
                            ASN1EndEncapsulation(ctx, ASN1_CONSTRUCTED_SEQUENCE);
                        }
                        // End the [1] 
                        ASN1EndEncapsulation(ctx, ASN1_APPLICAIION_SPECIFIC + 1);

                        // Add the hash algorithm
                        // [0] (1 elem)
                        //   SEQUENCE (2 elem) (done by 
                        //              X509PushAlgorithmIdentifierSequence)
                        //     OBJECT IDENTIFIER 2.16.840.1.101.3.4.2.1 sha-256 (NIST)
                        //     NULL
                        ASN1StartMarshalContext(ctx); // [0] (1 elem)
                        {
                            X509PushAlgorithmIdentifierSequence(ctx, hashDef->OID);
                        }
                        ASN1EndEncapsulation(ctx, (ASN1_APPLICAIION_SPECIFIC + 0));
                    }
                    //  SEQUENCE (3 elements) end
                    ASN1EndEncapsulation(ctx, ASN1_CONSTRUCTED_SEQUENCE);

                    // RSA PSS OID
                    // OBJECT IDENTIFIER 1.2.840.113549.1.1.10 rsaPSS (PKCS #1)
                    ASN1PushOID(ctx, OID_RSAPSS);
                }
                // End Sequence (2 elements)
                return ASN1EndEncapsulation(ctx, ASN1_CONSTRUCTED_SEQUENCE);
            }
        default:
            break;
    }
    return 0;
}
/* 10.2.25.2.2	X509AddPublicRSA() */
/* This function will add the publicKey description to the DER data. If fillPtr is NULL, then no
   data is transferred and this function will indicate if the TPM has the values for DER-encoding of
   the public key. */
/*     Return Value	Meaning */
/*     > 0	number of bytes added */
/*     == 0	failure */
INT16
X509AddPublicRSA(
		 OBJECT                  *object,
		 ASN1MarshalContext    *ctx
		 )
{
    UINT32          exp = object->publicArea.parameters.rsaDetail.exponent;
    //
    // If this is a check to see if the key can be encoded, it can.
    // Need to mark the end sequence
    if(ctx == NULL)
	return 1;
    ASN1StartMarshalContext(ctx); // SEQUENCE (2 elem) 1st
    ASN1StartMarshalContext(ctx); // BIT STRING
    ASN1StartMarshalContext(ctx); // SEQUENCE *(2 elem) 3rd
    
    // Get public exponent in big-endian byte order.
    if(exp == 0)
	exp = RSA_DEFAULT_PUBLIC_EXPONENT;
    
    // Push a 4 byte integer. This might get reduced if there are leading zeros or
    // extended if the high order byte is negative.
    ASN1PushUINT(ctx, exp);
    // Push the public key as an integer
    ASN1PushInteger(ctx, object->publicArea.unique.rsa.t.size,
		    object->publicArea.unique.rsa.t.buffer);
    // Embed this in a SEQUENCE tag and length in for the key, exponent sequence
    ASN1EndEncapsulation(ctx, ASN1_CONSTRUCTED_SEQUENCE); // SEQUENCE (3rd)
    
    // Embed this in a BIT STRING
    ASN1EndEncapsulation(ctx, ASN1_BITSTRING);
    
    // Now add the formatted SEQUENCE for the RSA public key OID. This is a
    // fully constructed value so it doesn't need to have a context started
    X509PushAlgorithmIdentifierSequence(ctx, OID_PKCS1_PUB);
    
    return ASN1EndEncapsulation(ctx, ASN1_CONSTRUCTED_SEQUENCE);
}
#endif // ALG_RSA
