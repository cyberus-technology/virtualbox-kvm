/********************************************************************************/
/*										*/
/*			X509 Support						*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: X509_spt.c $		*/
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
/*  - The TCG rants to the user of the other parts of the specification 	*/
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


/* 10.2.26	X509_spt.c */
/* 10.2.26.1	Includes */
#include "Tpm.h"
#include "TpmAsn1.h"
#include "TpmAsn1_fp.h"
#define _X509_SPT_
#include "X509.h"
#include "X509_spt_fp.h"
#if ALG_RSA
#   include "X509_RSA_fp.h"
#endif // ALG_RSA
#if ALG_ECC
#   include "X509_ECC_fp.h"
#endif // ALG_ECC
#if ALG_SM2
//#   include "X509_SM2_fp.h"
#endif // ALG_RSA
/* 10.2.26.2	Unmarshaling Functions */
/* 10.2.26.2.1	X509FindExtensionByOID() */
/* This will search a list of X509 extensions to find an extension with the requested OID. If the
   extension is found, the output context (ctx) is set up to point to the OID in the extension. */
/*     Return Value	Meaning */
/*     TRUE(1)	success */
/*     FALSE(0)	failure (could be catastrophic) */
BOOL
X509FindExtensionByOID(
		       ASN1UnmarshalContext    *ctxIn,         // IN: the context to search
		       ASN1UnmarshalContext    *ctx,           // OUT: the extension context
		       const BYTE              *OID            // IN: oid to search for
		       )
{
    INT16                length;
    //
    pAssert(ctxIn != NULL);
    // Make the search non-destructive of the input if ctx provided. Otherwise, use
    // the provided context.
    if (ctx == NULL)
	ctx = ctxIn;
    // if the provided search context is different from the context of the extension,
    // then copy the search context to the search context.
    else if(ctx != ctxIn)
	*ctx = *ctxIn;
    // Now, search in the extension context
    for(;ctx->size > ctx->offset; ctx->offset += length)
	{
	    VERIFY((length = ASN1NextTag(ctx)) >= 0);
	    // If this is not a constructed sequence, then it doesn't belong
	    // in the extensions.
	    VERIFY(ctx->tag == ASN1_CONSTRUCTED_SEQUENCE);
	    // Make sure that this entry could hold the OID
	    if (length >= OID_SIZE(OID))
	        {
	            // See if this is a match for the provided object identifier.
	            if (MemoryEqual(OID, &(ctx->buffer[ctx->offset]), OID_SIZE(OID)))
			{
			    // Return with ' ctx' set to point to the start of the OID with the size
			    // set to be the size of the SEQUENCE
			    ctx->buffer += ctx->offset;
			    ctx->offset = 0;
			    ctx->size = length;
			    return TRUE;
			}
	        }
	}
    VERIFY(ctx->offset == ctx->size);
    return FALSE;
 Error:
    ctxIn->size = -1;
    ctx->size = -1;
    return FALSE;
}
/* 10.2.26.2.2 X509GetExtensionBits() */
/* This function will extract a bit field from an extension. If the extension doesn't contain a bit
   string, it will fail. */
/*     Return Value	Meaning */
/*     TRUE(1)	success */
/*     FALSE(0)	failure */
UINT32
X509GetExtensionBits(
		     ASN1UnmarshalContext            *ctx,
		     UINT32                          *value
		     )
{
    INT16                length;
    //
    while (((length = ASN1NextTag(ctx)) > 0) && (ctx->size > ctx->offset))
	{
	    // Since this is an extension, the extension value will be in an OCTET STRING
	    if (ctx->tag == ASN1_OCTET_STRING)
	        {
	            return ASN1GetBitStringValue(ctx, value);
	        }
	    ctx->offset += length;
	}
    ctx->size = -1;
    return FALSE;
}
/* 10.2.26.2.3	X509ProcessExtensions() */
/* This function is used to process the TPMA_OBJECT and KeyUsage() extensions. It is not in the
   CertifyX509.c code because it makes the code harder to follow. */
/* Error Returns	Meaning */
/* TPM_RCS_ATTRIBUTES the attributes of object are not consistent with the extension setting */
/* TPM_RC_VALUE	problem parsing the extensions */
TPM_RC
X509ProcessExtensions(
		      OBJECT              *object,        // IN: The object with the attributes to
		      //      check
		      stringRef           *extension      // IN: The start and length of the extensions
		      )
{
    ASN1UnmarshalContext     ctx;
    ASN1UnmarshalContext     extensionCtx;
    INT16                    length;
    UINT32                   value;
    TPMA_OBJECT              attributes = object->publicArea.objectAttributes;
    //
    if(!ASN1UnmarshalContextInitialize(&ctx, extension->len, extension->buf)
       || ((length = ASN1NextTag(&ctx)) < 0)
       || (ctx.tag != X509_EXTENSIONS))
	return TPM_RCS_VALUE;
    if( ((length = ASN1NextTag(&ctx)) < 0)
	|| (ctx.tag != (ASN1_CONSTRUCTED_SEQUENCE)))
	return TPM_RCS_VALUE;
    
    // Get the extension for the TPMA_OBJECT if there is one
    if(X509FindExtensionByOID(&ctx, &extensionCtx, OID_TCG_TPMA_OBJECT) &&
       X509GetExtensionBits(&extensionCtx, &value))
	{
	    // If an keyAttributes extension was found, it must be exactly the same as the
	    // attributes of the object.
	    // NOTE: MemoryEqual() is used rather than a simple UINT32 compare to avoid
	    // type-punned pointer warning/error.
	    if(!MemoryEqual(&value, &attributes, sizeof(value)))
		return TPM_RCS_ATTRIBUTES;
	}
    // Make sure the failure to find the value wasn't because of a fatal error
    else if(extensionCtx.size < 0)
	return TPM_RCS_VALUE;

    // Get the keyUsage extension. This one is required
    if(X509FindExtensionByOID(&ctx, &extensionCtx, OID_KEY_USAGE_EXTENSION) &&
        X509GetExtensionBits(&extensionCtx, &value))
    {
        x509KeyUsageUnion   keyUsage;
	BOOL                badSign;
	BOOL                badDecrypt;
	BOOL                badFixedTPM;
	BOOL                badRestricted;

        keyUsage.integer = value;

	// For KeyUsage:
	// 1) 'sign' is SET if Key Usage includes signing
	badSign = ((KEY_USAGE_SIGN.integer & keyUsage.integer) != 0)
		  && !IS_ATTRIBUTE(attributes, TPMA_OBJECT, sign);
	// 2) 'decrypt' is SET if Key Usage includes decryption uses
	badDecrypt = ((KEY_USAGE_DECRYPT.integer & keyUsage.integer) != 0)
		     && !IS_ATTRIBUTE(attributes, TPMA_OBJECT, decrypt);
	// 3) 'fixedTPM' is SET if Key Usage is non-repudiation
	badFixedTPM = IS_ATTRIBUTE(keyUsage.x509, TPMA_X509_KEY_USAGE,
				   nonrepudiation)
		      && !IS_ATTRIBUTE(attributes, TPMA_OBJECT, fixedTPM);
	// 4)'restricted' is SET if Key Usage is for key agreement.
	badRestricted = IS_ATTRIBUTE(keyUsage.x509, TPMA_X509_KEY_USAGE, keyAgreement)
			&& !IS_ATTRIBUTE(attributes, TPMA_OBJECT, restricted);
	if(badSign || badDecrypt || badFixedTPM || badRestricted)
	    return TPM_RCS_VALUE;
   }
    else
	// The KeyUsage extension is required
	return TPM_RCS_VALUE;

    return TPM_RC_SUCCESS;
}
/* 10.2.26.3	Marshaling Functions */
/* 10.2.26.3.1	X509AddSigningAlgorithm() */
/* This creates the singing algorithm data. */
/* Return Value	Meaning */
/* > 0	number of octets added */
/* <= 0	failure */
INT16
X509AddSigningAlgorithm(
			ASN1MarshalContext  *ctx,
			OBJECT              *signKey,
			TPMT_SIG_SCHEME     *scheme
			)
{
    switch(signKey->publicArea.type)
	{
#if ALG_RSA
	  case TPM_ALG_RSA:
	    return X509AddSigningAlgorithmRSA(signKey, scheme, ctx);
#endif // ALG_RSA
#if ALG_ECC
	  case TPM_ALG_ECC:
	    return X509AddSigningAlgorithmECC(signKey, scheme, ctx);
#endif // ALG_ECC
#if ALG_SM2
	  case TPM_ALG_SM2:
	    break;  // no signing algorithm for SM2 yet
	    //            return X509AddSigningAlgorithmSM2(signKey, scheme, ctx);
#endif // ALG_SM2
	  default:
	    break;
	}
    return 0;
}
/* 10.2.26.3.2	X509AddPublicKey() */
/* This function will add the publicKey description to the DER data. If fillPtr is NULL, then no
   data is transferred and this function will indicate if the TPM has the values for DER-encoding of
   the public key. */
/*     Return Value	Meaning */
/*     > 0	number of octets added */
/*     == 0	failure */
INT16
X509AddPublicKey(
		 ASN1MarshalContext  *ctx,
		 OBJECT              *object
		 )
{
    switch(object->publicArea.type)
	{
#if ALG_RSA
	  case TPM_ALG_RSA:
	    return X509AddPublicRSA(object, ctx);
#endif
#if ALG_ECC
	  case TPM_ALG_ECC:
	    return X509AddPublicECC(object, ctx);
#endif
#if ALG_SM2
	  case TPM_ALG_SM2:
	    break;
#endif
	  default:
	    break;
	}
    return FALSE;
}
/* 10.2.26.3.3	X509PushAlgorithmIdentifierSequence() */
/* The function adds the algorithm identifier sequence. */
/* Return Value	Meaning */
/* > 0	number of bytes added */
/* == 0	failure */
INT16
X509PushAlgorithmIdentifierSequence(
				    ASN1MarshalContext          *ctx,
				    const BYTE                  *OID
				    )
{
    ASN1StartMarshalContext(ctx);   // hash algorithm
    ASN1PushNull(ctx);
    ASN1PushOID(ctx, OID);
    return ASN1EndEncapsulation(ctx, ASN1_CONSTRUCTED_SEQUENCE);
}

