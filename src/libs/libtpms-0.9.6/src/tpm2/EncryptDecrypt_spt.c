/********************************************************************************/
/*										*/
/*			 	Encrypt Decrypt Support 			*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: EncryptDecrypt_spt.c $	*/
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

/* 7.7 Encrypt Decrypt Support (EncryptDecrypt_spt.c) */
#include "Tpm.h"
#include "EncryptDecrypt_fp.h"
#include "EncryptDecrypt_spt_fp.h"
#if CC_EncryptDecrypt2
/* Error Returns Meaning */
/* TPM_RC_KEY is not a symmetric decryption key with both public and private portions loaded */
/* TPM_RC_SIZE IvIn size is incompatible with the block cipher mode; or inData size is not an even
   multiple of the block size for CBC or ECB mode */
/* TPM_RC_VALUE keyHandle is restricted and the argument mode does not match the key's mode */
TPM_RC
EncryptDecryptShared(
		     TPMI_DH_OBJECT        keyHandleIn,
		     TPMI_YES_NO           decryptIn,
		     TPMI_ALG_SYM_MODE     modeIn,
		     TPM2B_IV              *ivIn,
		     TPM2B_MAX_BUFFER      *inData,
		     EncryptDecrypt_Out    *out
		     )
{
    OBJECT              *symKey;
    UINT16               keySize;
    UINT16               blockSize;
    BYTE                *key;
    TPM_ALG_ID           alg;
    TPM_ALG_ID           mode;
    TPM_RC               result;
    BOOL                 OK;
    // Input Validation
    symKey = HandleToObject(keyHandleIn);
    mode = symKey->publicArea.parameters.symDetail.sym.mode.sym;
    // The input key should be a symmetric key
    if(symKey->publicArea.type != TPM_ALG_SYMCIPHER)
	return TPM_RCS_KEY + RC_EncryptDecrypt_keyHandle;
    // The key must be unrestricted and allow the selected operation
    OK = !IS_ATTRIBUTE(symKey->publicArea.objectAttributes,
		       TPMA_OBJECT, restricted);
    if(YES == decryptIn)
	OK = OK && IS_ATTRIBUTE(symKey->publicArea.objectAttributes,
				TPMA_OBJECT, decrypt);
    else
	OK = OK && IS_ATTRIBUTE(symKey->publicArea.objectAttributes,
				TPMA_OBJECT, sign);
    if(!OK)
	return TPM_RCS_ATTRIBUTES + RC_EncryptDecrypt_keyHandle;
    // Make sure that key is an encrypt/decrypt key and not SMAC
    if(!CryptSymModeIsValid(mode, TRUE))
	return TPM_RCS_MODE + RC_EncryptDecrypt_keyHandle;
    // If the key mode is not TPM_ALG_NULL...
    // or TPM_ALG_NULL
    if(mode != TPM_ALG_NULL)
	{
	    // then the input mode has to be TPM_ALG_NULL or the same as the key
	    if((modeIn != TPM_ALG_NULL) && (modeIn != mode))
		return TPM_RCS_MODE + RC_EncryptDecrypt_mode;
	}
    else
	{
	    // if the key mode is null, then the input can't be null
	    if(modeIn == TPM_ALG_NULL)
		return TPM_RCS_MODE + RC_EncryptDecrypt_mode;
	    mode = modeIn;
	}
    // The input iv for ECB mode should be an Empty Buffer.  All the other modes
    // should have an iv size same as encryption block size
    keySize = symKey->publicArea.parameters.symDetail.sym.keyBits.sym;
    alg = symKey->publicArea.parameters.symDetail.sym.algorithm;
    blockSize = CryptGetSymmetricBlockSize(alg, keySize);
    // reverify the algorithm. This is mainly to keep static analysis tools happy
    if(blockSize == 0)
	return TPM_RCS_KEY + RC_EncryptDecrypt_keyHandle;
    if(((mode == TPM_ALG_ECB) && (ivIn->t.size != 0))
       || ((mode != TPM_ALG_ECB) && (ivIn->t.size != blockSize)))
	return TPM_RCS_SIZE + RC_EncryptDecrypt_ivIn;
    // The input data size of CBC mode or ECB mode must be an even multiple of
    // the symmetric algorithm's block size
    if(((mode == TPM_ALG_CBC) || (mode == TPM_ALG_ECB))
       && ((inData->t.size % blockSize) != 0))
	return TPM_RCS_SIZE + RC_EncryptDecrypt_inData;
    // Copy IV
    // Note: This is copied here so that the calls to the encrypt/decrypt functions
    // will modify the output buffer, not the input buffer
    out->ivOut = *ivIn;
    // Command Output
    key = symKey->sensitive.sensitive.sym.t.buffer;
    // For symmetric encryption, the cipher data size is the same as plain data
    // size.
    out->outData.t.size = inData->t.size;
    if(decryptIn == YES)
	{
	    // Decrypt data to output
	    result = CryptSymmetricDecrypt(out->outData.t.buffer, alg, keySize, key,
					   &(out->ivOut), mode, inData->t.size,
					   inData->t.buffer);
	}
    else
	{
	    // Encrypt data to output
	    result = CryptSymmetricEncrypt(out->outData.t.buffer, alg, keySize, key,
					   &(out->ivOut), mode, inData->t.size,
					   inData->t.buffer);
	}
    return result;
}
#endif // CC_EncryptDecrypt
