/********************************************************************************/
/*										*/
/*			     	Symmetric Commands				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: SymmetricCommands.c $	*/
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
/*  (c) Copyright IBM Corp. and others, 2016 - 2021				*/
/*										*/
/********************************************************************************/

#include "Tpm.h"
#include "EncryptDecrypt_fp.h"
#if CC_EncryptDecrypt2
#include  "EncryptDecrypt_spt_fp.h"
#endif
#if CC_EncryptDecrypt  // Conditional expansion of this file
TPM_RC
TPM2_EncryptDecrypt(
		    EncryptDecrypt_In   *in,            // IN: input parameter list
		    EncryptDecrypt_Out  *out            // OUT: output parameter list
		    )
{
#if CC_EncryptDecrypt2
    return EncryptDecryptShared(in->keyHandle, in->decrypt, in->mode,
				&in->ivIn, &in->inData, out);
#else
    OBJECT              *symKey;
    UINT16               keySize;
    UINT16               blockSize;
    BYTE                *key;
    TPM_ALG_ID           alg;
    TPM_ALG_ID           mode;
    TPM_RC               result;
    BOOL                 OK;
    TPMA_OBJECT          attributes;
    // Input Validation
    symKey = HandleToObject(in->keyHandle);
    mode = symKey->publicArea.parameters.symDetail.sym.mode.sym;
    attributes = symKey->publicArea.objectAttributes;
    // The input key should be a symmetric key
    if(symKey->publicArea.type != TPM_ALG_SYMCIPHER)
	return TPM_RCS_KEY + RC_EncryptDecrypt_keyHandle;	
    // The key must be unrestricted and allow the selected operation
    OK = IS_ATTRIBUTE(attributes, TPMA_OBJECT, restricted)
     if(YES == in->decrypt)
	 OK = OK && IS_ATTRIBUTE(attributes, TPMA_OBJECT, decrypt);
     else
	 OK = OK && IS_ATTRIBUTE(attributes, TPMA_OBJECT, sign);
    if(!OK)
	return TPM_RCS_ATTRIBUTES + RC_EncryptDecrypt_keyHandle;
    // If the key mode is not TPM_ALG_NULL...
    // or TPM_ALG_NULL
    if(mode != TPM_ALG_NULL)
	{
	    // then the input mode has to be TPM_ALG_NULL or the same as the key
	    if((in->mode != TPM_ALG_NULL) && (in->mode != mode))
		return TPM_RCS_MODE + RC_EncryptDecrypt_mode;
	}
    else
	{
	    // if the key mode is null, then the input can't be null
	    if(in->mode == TPM_ALG_NULL)
		return TPM_RCS_MODE + RC_EncryptDecrypt_mode;
	    mode = in->mode;
	}
    // The input iv for ECB mode should be an Empty Buffer.  All the other modes
    // should have an iv size same as encryption block size
    keySize = symKey->publicArea.parameters.symDetail.sym.keyBits.sym;
    alg = symKey->publicArea.parameters.symDetail.sym.algorithm;
    blockSize = CryptGetSymmetricBlockSize(alg, keySize);
    // reverify the algorithm. This is mainly to keep static analysis tools happy
    if(blockSize == 0)
	return TPM_RCS_KEY + RC_EncryptDecrypt_keyHandle;
    // Note: When an algorithm is not supported by a TPM, the TPM_ALG_xxx for that
    // algorithm is not defined. However, it is assumed that the TPM_ALG_xxx for
    // the algorithm is always defined. Both have the same numeric value.
    // TPM_ALG_xxx is used here so that the code does not get cluttered with
    // #ifdef's. Having this check does not mean that the algorithm is supported.
    // If it was not supported the unmarshaling code would have rejected it before
    // this function were called. This means that, depending on the implementation,
    // the check could be redundant but it doesn't hurt.
    if(((mode == TPM_ALG_ECB) && (in->ivIn.t.size != 0))
       || ((mode != TPM_ALG_ECB) && (in->ivIn.t.size != blockSize)))
	return TPM_RCS_SIZE + RC_EncryptDecrypt_ivIn;
    // The input data size of CBC mode or ECB mode must be an even multiple of
    // the symmetric algorithm's block size
    if(((mode == TPM_ALG_CBC) || (mode == TPM_ALG_ECB))
       && ((in->inData.t.size % blockSize) != 0))
	return TPM_RCS_SIZE + RC_EncryptDecrypt_inData;
    // Copy IV
    // Note: This is copied here so that the calls to the encrypt/decrypt functions
    // will modify the output buffer, not the input buffer
    out->ivOut = in->ivIn;
    // Command Output
    key = symKey->sensitive.sensitive.sym.t.buffer;
    // For symmetric encryption, the cipher data size is the same as plain data
    // size.
    out->outData.t.size = in->inData.t.size;
    if(in->decrypt == YES)
	{
	    // Decrypt data to output
	    result = CryptSymmetricDecrypt(out->outData.t.buffer, alg, keySize, key,
					   &(out->ivOut), mode, in->inData.t.size,
					   in->inData.t.buffer);
	}
    else
	{
	    // Encrypt data to output
	    result = CryptSymmetricEncrypt(out->outData.t.buffer, alg, keySize, key,
					   &(out->ivOut), mode, in->inData.t.size,
					   in->inData.t.buffer);
	}
    return result;
#endif // CC_EncryptDecrypt2
}
#endif // CC_EncryptDecrypt
#include "Tpm.h"
#include "EncryptDecrypt2_fp.h"
#include "EncryptDecrypt_spt_fp.h"
#if CC_EncryptDecrypt2  // Conditional expansion of this file
TPM_RC
TPM2_EncryptDecrypt2(
		     EncryptDecrypt2_In   *in,            // IN: input parameter list
		     EncryptDecrypt2_Out  *out            // OUT: output parameter list
		     )
{
    TPM_RC                result;
    // EncryptDecyrptShared() performs the operations as shown in
    // TPM2_EncrypDecrypt
    result = EncryptDecryptShared(in->keyHandle, in->decrypt, in->mode,
				  &in->ivIn, &in->inData,
				  (EncryptDecrypt_Out *)out);
    // Handle response code swizzle.
    switch(result)
	{
	  case TPM_RCS_MODE + RC_EncryptDecrypt_mode:
	    result = TPM_RCS_MODE + RC_EncryptDecrypt2_mode;
	    break;
	  case TPM_RCS_SIZE + RC_EncryptDecrypt_ivIn:
	    result = TPM_RCS_SIZE + RC_EncryptDecrypt2_ivIn;
	    break;
	  case TPM_RCS_SIZE + RC_EncryptDecrypt_inData:
	    result = TPM_RCS_SIZE + RC_EncryptDecrypt2_inData;
	    break;
	  default:
	    break;
	}
    return result;
}
#endif // CC_EncryptDecrypt2
#include "Tpm.h"
#include "Hash_fp.h"
#if CC_Hash  // Conditional expansion of this file
TPM_RC
TPM2_Hash(
	  Hash_In         *in,            // IN: input parameter list
	  Hash_Out        *out            // OUT: output parameter list
	  )
{
    HASH_STATE          hashState;
    // Command Output
    // Output hash
    // Start hash stack
    out->outHash.t.size = CryptHashStart(&hashState, in->hashAlg);
    // Adding hash data
    CryptDigestUpdate2B(&hashState, &in->data.b);
    // Complete hash
    CryptHashEnd2B(&hashState, &out->outHash.b);
    // Output ticket
    out->validation.tag = TPM_ST_HASHCHECK;
    out->validation.hierarchy = in->hierarchy;
    if(in->hierarchy == TPM_RH_NULL)
	{
	    // Ticket is not required
	    out->validation.hierarchy = TPM_RH_NULL;
	    out->validation.digest.t.size = 0;
	}
    else if(in->data.t.size >= sizeof(TPM_GENERATED_VALUE)
	    && !TicketIsSafe(&in->data.b))
	{
	    // Ticket is not safe
	    out->validation.hierarchy = TPM_RH_NULL;
	    out->validation.digest.t.size = 0;
	}
    else
	{
	    // Compute ticket
	    TicketComputeHashCheck(in->hierarchy, in->hashAlg,
				   &out->outHash, &out->validation);
	}
    return TPM_RC_SUCCESS;
}
#endif // CC_Hash
#include "Tpm.h"
#include "HMAC_fp.h"
#if CC_HMAC  // Conditional expansion of this file
TPM_RC
TPM2_HMAC(
	  HMAC_In         *in,            // IN: input parameter list
	  HMAC_Out        *out            // OUT: output parameter list
	  )
{
    HMAC_STATE               hmacState;
    OBJECT                  *hmacObject;
    TPMI_ALG_HASH            hashAlg;
    TPMT_PUBLIC             *publicArea;
    // Input Validation
    // Get HMAC key object and public area pointers
    hmacObject = HandleToObject(in->handle);
    publicArea = &hmacObject->publicArea;
    // Make sure that the key is an HMAC key
    if(publicArea->type != TPM_ALG_KEYEDHASH)
	return TPM_RCS_TYPE + RC_HMAC_handle;
    // and that it is unrestricted
    if (IS_ATTRIBUTE(publicArea->objectAttributes, TPMA_OBJECT, restricted))
	return TPM_RCS_ATTRIBUTES + RC_HMAC_handle;
    // and that it is a signing key
    if (!IS_ATTRIBUTE(publicArea->objectAttributes, TPMA_OBJECT, sign))
	return TPM_RCS_KEY + RC_HMAC_handle;
    // See if the key has a default
    if(publicArea->parameters.keyedHashDetail.scheme.scheme == TPM_ALG_NULL)
	// it doesn't so use the input value
	hashAlg = in->hashAlg;
    else
	{
	    // key has a default so use it
	    hashAlg
		= publicArea->parameters.keyedHashDetail.scheme.details.hmac.hashAlg;
	    // and verify that the input was either the  TPM_ALG_NULL or the default
	    if(in->hashAlg != TPM_ALG_NULL && in->hashAlg != hashAlg)
		hashAlg = TPM_ALG_NULL;
	}
    // if we ended up without a hash algorithm then return an error
    if(hashAlg == TPM_ALG_NULL)
	return TPM_RCS_VALUE + RC_HMAC_hashAlg;
    // Command Output
    // Start HMAC stack
    out->outHMAC.t.size = CryptHmacStart2B(&hmacState, hashAlg,
					   &hmacObject->sensitive.sensitive.bits.b);
    // Adding HMAC data
    CryptDigestUpdate2B(&hmacState.hashState, &in->buffer.b);
    // Complete HMAC
    CryptHmacEnd2B(&hmacState, &out->outHMAC.b);
    return TPM_RC_SUCCESS;
}
#endif // CC_HMAC

#include "Tpm.h"
#include "MAC_fp.h"
#if CC_MAC  // Conditional expansion of this file
/* Error Returns Meaning */
/* TPM_RC_ATTRIBUTES key referenced by handle is a restricted key */
/* TPM_RC_KEY handle does not reference a signing key */
/* TPM_RC_TYPE key referenced by handle is not an HMAC key */
/* TPM_RC_VALUE hashAlg is not compatible with the hash algorithm of the scheme of the object
   referenced by handle */
TPM_RC
TPM2_MAC(
	 MAC_In         *in,            // IN: input parameter list
	 MAC_Out        *out            // OUT: output parameter list
	 )
{
    OBJECT                  *keyObject;
    HMAC_STATE               state;
    TPMT_PUBLIC             *publicArea;
    TPM_RC                   result;
    // Input Validation
    // Get MAC key object and public area pointers
    keyObject = HandleToObject(in->handle);
    publicArea = &keyObject->publicArea;
    // If the key is not able to do a MAC, indicate that the handle selects an
    // object that can't do a MAC
    result = CryptSelectMac(publicArea, &in->inScheme);
    if(result == TPM_RCS_TYPE)
	return TPM_RCS_TYPE + RC_MAC_handle;
    // If there is another error type, indicate that the scheme and key are not
    // compatible
    if(result != TPM_RC_SUCCESS)
	return RcSafeAddToResult(result, RC_MAC_inScheme);
    // Make sure that the key is not restricted
    if(IS_ATTRIBUTE(publicArea->objectAttributes, TPMA_OBJECT, restricted))
	return TPM_RCS_ATTRIBUTES + RC_MAC_handle;
    // and that it is a signing key
    if(!IS_ATTRIBUTE(publicArea->objectAttributes, TPMA_OBJECT, sign))
	return TPM_RCS_KEY + RC_MAC_handle;
    // Command Output
    out->outMAC.t.size = CryptMacStart(&state, &publicArea->parameters,
				       in->inScheme,
				       &keyObject->sensitive.sensitive.any.b);
    // If the mac can't start, treat it as a fatal error
    if(out->outMAC.t.size == 0)
	return TPM_RC_FAILURE;
    CryptDigestUpdate2B(&state.hashState, &in->buffer.b);
    // If the MAC result is not what was expected, it is a fatal error
    if(CryptHmacEnd2B(&state, &out->outMAC.b) != out->outMAC.t.size)
	return TPM_RC_FAILURE;
    return TPM_RC_SUCCESS;
}
#endif // CC_MAC
