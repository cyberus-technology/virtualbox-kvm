/********************************************************************************/
/*										*/
/*			  Asymmetric Commands   				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: AsymmetricCommands.c $	*/
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
/*  (c) Copyright IBM Corp. and others, 2016 - 2019				*/
/*										*/
/********************************************************************************/

#include "Tpm.h"
#include "RSA_Encrypt_fp.h"
#if CC_RSA_Encrypt  // Conditional expansion of this file
TPM_RC
TPM2_RSA_Encrypt(
		 RSA_Encrypt_In      *in,            // IN: input parameter list
		 RSA_Encrypt_Out     *out            // OUT: output parameter list
		 )
{
    TPM_RC                  result;
    OBJECT                  *rsaKey;
    TPMT_RSA_DECRYPT        *scheme;
    // Input Validation
    rsaKey = HandleToObject(in->keyHandle);
    // selected key must be an RSA key
    if(rsaKey->publicArea.type != TPM_ALG_RSA)
	return TPM_RCS_KEY + RC_RSA_Encrypt_keyHandle;
    // selected key must have the decryption attribute
    if(!IS_ATTRIBUTE(rsaKey->publicArea.objectAttributes, TPMA_OBJECT, decrypt))
	return TPM_RCS_ATTRIBUTES + RC_RSA_Encrypt_keyHandle;
    // Is there a label?
    if(!IsLabelProperlyFormatted(&in->label.b))
	return TPM_RCS_VALUE + RC_RSA_Encrypt_label;
    // Command Output
    // Select a scheme for encryption
    scheme = CryptRsaSelectScheme(in->keyHandle, &in->inScheme);
    if(scheme == NULL)
	return TPM_RCS_SCHEME + RC_RSA_Encrypt_inScheme;
    // Encryption.  TPM_RC_VALUE, or TPM_RC_SCHEME errors my be returned buy
    // CryptEncyptRSA.
    out->outData.t.size = sizeof(out->outData.t.buffer);
    result = CryptRsaEncrypt(&out->outData, &in->message.b, rsaKey, scheme,
			     &in->label.b, NULL);
    return result;
}
#endif // CC_RSA_Encrypt
#include "Tpm.h"
#include "RSA_Decrypt_fp.h"
#if CC_RSA_Decrypt  // Conditional expansion of this file
TPM_RC
TPM2_RSA_Decrypt(
		 RSA_Decrypt_In      *in,            // IN: input parameter list
		 RSA_Decrypt_Out     *out            // OUT: output parameter list
		 )
{
    TPM_RC                       result;
    OBJECT                      *rsaKey;
    TPMT_RSA_DECRYPT            *scheme;
    // Input Validation
    rsaKey = HandleToObject(in->keyHandle);
    // The selected key must be an RSA key
    if(rsaKey->publicArea.type != TPM_ALG_RSA)
	return TPM_RCS_KEY + RC_RSA_Decrypt_keyHandle;
    // The selected key must be an unrestricted decryption key
    if(IS_ATTRIBUTE(rsaKey->publicArea.objectAttributes, TPMA_OBJECT, restricted)
       || !IS_ATTRIBUTE(rsaKey->publicArea.objectAttributes, TPMA_OBJECT, decrypt))
	return TPM_RCS_ATTRIBUTES + RC_RSA_Decrypt_keyHandle;
    // NOTE: Proper operation of this command requires that the sensitive area
    // of the key is loaded. This is assured because authorization is required
    // to use the sensitive area of the key. In order to check the authorization,
    // the sensitive area has to be loaded, even if authorization is with policy.
    // If label is present, make sure that it is a NULL-terminated string
    if(!IsLabelProperlyFormatted(&in->label.b))
	return TPM_RCS_VALUE + RC_RSA_Decrypt_label;
    // Command Output
    // Select a scheme for decrypt.
    scheme = CryptRsaSelectScheme(in->keyHandle, &in->inScheme);
    if(scheme == NULL)
	return TPM_RCS_SCHEME + RC_RSA_Decrypt_inScheme;
    // Decryption.  TPM_RC_VALUE, TPM_RC_SIZE, and TPM_RC_KEY error may be
    // returned by CryptRsaDecrypt.
    // NOTE: CryptRsaDecrypt can also return TPM_RC_ATTRIBUTES or TPM_RC_BINDING
    // when the key is not a decryption key but that was checked above.
    out->message.t.size = sizeof(out->message.t.buffer);
    result = CryptRsaDecrypt(&out->message.b, &in->cipherText.b, rsaKey,
			     scheme, &in->label.b);
    return result;
}
#endif // CC_RSA_Decrypt
#include "Tpm.h"
#include "ECDH_KeyGen_fp.h"
#if CC_ECDH_KeyGen  // Conditional expansion of this file
TPM_RC
TPM2_ECDH_KeyGen(
		 ECDH_KeyGen_In      *in,            // IN: input parameter list
		 ECDH_KeyGen_Out     *out            // OUT: output parameter list
		 )
{
    OBJECT                  *eccKey;
    TPM2B_ECC_PARAMETER      sensitive;
    TPM_RC                   result;
    // Input Validation
    eccKey = HandleToObject(in->keyHandle);
    // Referenced key must be an ECC key
    if(eccKey->publicArea.type != TPM_ALG_ECC)
	return TPM_RCS_KEY + RC_ECDH_KeyGen_keyHandle;
    // Command Output
    do
	{
	    TPMT_PUBLIC         *keyPublic = &eccKey->publicArea;
	    // Create ephemeral ECC key
	    result = CryptEccNewKeyPair(&out->pubPoint.point, &sensitive,
					keyPublic->parameters.eccDetail.curveID);
	    if(result == TPM_RC_SUCCESS)
	        {
	            // Compute Z
	            result = CryptEccPointMultiply(&out->zPoint.point,
	                                           keyPublic->parameters.eccDetail.curveID,
	                                           &keyPublic->unique.ecc,
	                                           &sensitive,
	                                           NULL, NULL);
		    // The point in the key is not on the curve. Indicate
		    // that the key is bad.
	            if(result == TPM_RC_ECC_POINT)
	                return TPM_RCS_KEY + RC_ECDH_KeyGen_keyHandle;
		    // The other possible error from CryptEccPointMultiply is
		    // TPM_RC_NO_RESULT indicating that the multiplication resulted in
		    // the point at infinity, so get a new random key and start over
		    // BTW, this never happens.
	        }
	} while(result == TPM_RC_NO_RESULT);
    return result;
}
#endif // CC_ECDH_KeyGen
#include "Tpm.h"
#include "ECDH_ZGen_fp.h"
#if CC_ECDH_ZGen  // Conditional expansion of this file
TPM_RC
TPM2_ECDH_ZGen(
	       ECDH_ZGen_In    *in,            // IN: input parameter list
	       ECDH_ZGen_Out   *out            // OUT: output parameter list
	       )
{
    TPM_RC                   result;
    OBJECT                  *eccKey;
    // Input Validation
    eccKey = HandleToObject(in->keyHandle);
    // Selected key must be a non-restricted, decrypt ECC key
    if(eccKey->publicArea.type != TPM_ALG_ECC)
	return TPM_RCS_KEY + RC_ECDH_ZGen_keyHandle;
    // Selected key needs to be unrestricted with the 'decrypt' attribute
    if(IS_ATTRIBUTE(eccKey->publicArea.objectAttributes, TPMA_OBJECT, restricted)
       || !IS_ATTRIBUTE(eccKey->publicArea.objectAttributes, TPMA_OBJECT, decrypt))
	return TPM_RCS_ATTRIBUTES + RC_ECDH_ZGen_keyHandle;
    // Make sure the scheme allows this use
    if(eccKey->publicArea.parameters.eccDetail.scheme.scheme != TPM_ALG_ECDH
       &&  eccKey->publicArea.parameters.eccDetail.scheme.scheme != TPM_ALG_NULL)
	return TPM_RCS_SCHEME + RC_ECDH_ZGen_keyHandle;
    // Command Output
    // Compute Z. TPM_RC_ECC_POINT or TPM_RC_NO_RESULT may be returned here.
    result = CryptEccPointMultiply(&out->outPoint.point,
				   eccKey->publicArea.parameters.eccDetail.curveID,
				   &in->inPoint.point,
				   &eccKey->sensitive.sensitive.ecc,
				   NULL, NULL);
    if(result != TPM_RC_SUCCESS)
	return RcSafeAddToResult(result, RC_ECDH_ZGen_inPoint);
    return result;
}
#endif // CC_ECDH_ZGen
#include "Tpm.h"
#include "ECC_Parameters_fp.h"
#if CC_ECC_Parameters  // Conditional expansion of this file
TPM_RC
TPM2_ECC_Parameters(
		    ECC_Parameters_In   *in,            // IN: input parameter list
		    ECC_Parameters_Out  *out            // OUT: output parameter list
		    )
{
    // Command Output
    // Get ECC curve parameters
    if(CryptEccGetParameters(in->curveID, &out->parameters))
	return TPM_RC_SUCCESS;
    else
	return TPM_RCS_VALUE + RC_ECC_Parameters_curveID;
}
#endif // CC_ECC_Parameters
#include "Tpm.h"
#include "ZGen_2Phase_fp.h"
#if CC_ZGen_2Phase  // Conditional expansion of this file
TPM_RC
TPM2_ZGen_2Phase(
		 ZGen_2Phase_In      *in,            // IN: input parameter list
		 ZGen_2Phase_Out     *out            // OUT: output parameter list
		 )
{
    TPM_RC                   result;
    OBJECT                  *eccKey;
    TPM2B_ECC_PARAMETER      r;
    TPM_ALG_ID               scheme;
    // Input Validation
    eccKey = HandleToObject(in->keyA);
    // keyA must be an ECC key
    if(eccKey->publicArea.type != TPM_ALG_ECC)
	return TPM_RCS_KEY + RC_ZGen_2Phase_keyA;
    // keyA must not be restricted and must be a decrypt key
    if(IS_ATTRIBUTE(eccKey->publicArea.objectAttributes, TPMA_OBJECT, restricted)
       || !IS_ATTRIBUTE(eccKey->publicArea.objectAttributes, TPMA_OBJECT, decrypt))
	return TPM_RCS_ATTRIBUTES + RC_ZGen_2Phase_keyA;
    // if the scheme of keyA is TPM_ALG_NULL, then use the input scheme; otherwise
    // the input scheme must be the same as the scheme of keyA
    scheme = eccKey->publicArea.parameters.asymDetail.scheme.scheme;
    if(scheme != TPM_ALG_NULL)
	{
	    if(scheme != in->inScheme)
		return TPM_RCS_SCHEME + RC_ZGen_2Phase_inScheme;
	}
    else
	scheme = in->inScheme;
    if(scheme == TPM_ALG_NULL)
	return TPM_RCS_SCHEME + RC_ZGen_2Phase_inScheme;
    // Input points must be on the curve of keyA
    if(!CryptEccIsPointOnCurve(eccKey->publicArea.parameters.eccDetail.curveID,
			       &in->inQsB.point))
	return TPM_RCS_ECC_POINT + RC_ZGen_2Phase_inQsB;
    if(!CryptEccIsPointOnCurve(eccKey->publicArea.parameters.eccDetail.curveID,
			       &in->inQeB.point))
	return TPM_RCS_ECC_POINT + RC_ZGen_2Phase_inQeB;
    if(!CryptGenerateR(&r, &in->counter,
		       eccKey->publicArea.parameters.eccDetail.curveID,
		       NULL))
	return TPM_RCS_VALUE + RC_ZGen_2Phase_counter;
    // Command Output
    result = CryptEcc2PhaseKeyExchange(&out->outZ1.point,
				       &out->outZ2.point,
				       eccKey->publicArea.parameters.eccDetail.curveID,
				       scheme,
				       &eccKey->sensitive.sensitive.ecc,
				       &r,
				       &in->inQsB.point,
				       &in->inQeB.point);
    if(result == TPM_RC_SCHEME)
	return TPM_RCS_SCHEME + RC_ZGen_2Phase_inScheme;
    if(result == TPM_RC_SUCCESS)
	CryptEndCommit(in->counter);
    return result;
}
#endif // CC_ZGen_2Phase
