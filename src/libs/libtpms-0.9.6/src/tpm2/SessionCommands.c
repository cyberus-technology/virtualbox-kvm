/********************************************************************************/
/*										*/
/*			     	Session Commands				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: SessionCommands.c $	*/
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
#include "StartAuthSession_fp.h"
#if CC_StartAuthSession  // Conditional expansion of this file
TPM_RC
TPM2_StartAuthSession(
		      StartAuthSession_In     *in,            // IN: input parameter buffer
		      StartAuthSession_Out    *out            // OUT: output parameter buffer
		      )
{
    TPM_RC                   result = TPM_RC_SUCCESS;
    OBJECT                  *tpmKey;                // TPM key for decrypt salt
    TPM2B_DATA               salt;
    // Input Validation
    // Check input nonce size.  IT should be at least 16 bytes but not larger
    // than the digest size of session hash.
    if(in->nonceCaller.t.size < 16
       || in->nonceCaller.t.size > CryptHashGetDigestSize(in->authHash))
	return TPM_RCS_SIZE + RC_StartAuthSession_nonceCaller;
    // If an decrypt key is passed in, check its validation
    if(in->tpmKey != TPM_RH_NULL)
	{
	    // Get pointer to loaded decrypt key
	    tpmKey = HandleToObject(in->tpmKey);
	    // key must be asymmetric with its sensitive area loaded. Since this
	    // command does not require authorization, the presence of the sensitive
	    // area was not already checked as it is with most other commands that
	    // use the sensitive are so check it here
	    if(!CryptIsAsymAlgorithm(tpmKey->publicArea.type))
		return TPM_RCS_KEY + RC_StartAuthSession_tpmKey;
	    // secret size cannot be 0
	    if(in->encryptedSalt.t.size == 0)
		return TPM_RCS_VALUE + RC_StartAuthSession_encryptedSalt;
	    // Decrypting salt requires accessing the private portion of a key.
	    // Therefore, tmpKey can not be a key with only public portion loaded
	    if(tpmKey->attributes.publicOnly)
		return TPM_RCS_HANDLE + RC_StartAuthSession_tpmKey;
	    // HMAC session input handle check.
	    // tpmKey should be a decryption key
	    if(!IS_ATTRIBUTE(tpmKey->publicArea.objectAttributes, TPMA_OBJECT, decrypt))
		return TPM_RCS_ATTRIBUTES + RC_StartAuthSession_tpmKey;
	    // Secret Decryption.  A TPM_RC_VALUE, TPM_RC_KEY or Unmarshal errors
	    // may be returned at this point
	    result = CryptSecretDecrypt(tpmKey, &in->nonceCaller, SECRET_KEY,
					&in->encryptedSalt, &salt);
	    if(result != TPM_RC_SUCCESS)
		return TPM_RCS_VALUE + RC_StartAuthSession_encryptedSalt;
	}
    else
	{
	    // secret size must be 0
	    if(in->encryptedSalt.t.size != 0)
		return TPM_RCS_VALUE + RC_StartAuthSession_encryptedSalt;
	    salt.t.size = 0;
	}
    switch(HandleGetType(in->bind))
	{
	  case TPM_HT_TRANSIENT:
	      {
		  OBJECT      *object = HandleToObject(in->bind);
		  // If the bind handle references a transient object, make sure that we
		  // can get to the authorization value. Also, make sure that the object
		  // has a proper Name (nameAlg != TPM_ALG_NULL). If it doesn't, then
		  // it might be possible to bind to an object where the authValue is
		  // known. This does not create a real issue in that, if you know the
		  // authorization value, you can actually bind to the object. However,
		  // there is a potential
		  if(object->attributes.publicOnly == SET)
		      return TPM_RCS_HANDLE + RC_StartAuthSession_bind;
		  break;
	      }
	  case TPM_HT_NV_INDEX:
	    // a PIN index can't be a bind object
	      {
		  NV_INDEX       *nvIndex = NvGetIndexInfo(in->bind, NULL);
		  if(IsNvPinPassIndex(nvIndex->publicArea.attributes)
		     || IsNvPinFailIndex(nvIndex->publicArea.attributes))
		      return TPM_RCS_HANDLE + RC_StartAuthSession_bind;
		  break;
	      }
	  default:
	    break;
	}
    // If 'symmetric' is a symmetric block cipher (not TPM_ALG_NULL or TPM_ALG_XOR)
    // then the mode must be CFB.
    if(in->symmetric.algorithm != TPM_ALG_NULL
       && in->symmetric.algorithm != TPM_ALG_XOR
       && in->symmetric.mode.sym != TPM_ALG_CFB)
	return TPM_RCS_MODE + RC_StartAuthSession_symmetric;
    // Internal Data Update and command output
    // Create internal session structure.  TPM_RC_CONTEXT_GAP, TPM_RC_NO_HANDLES
    // or TPM_RC_SESSION_MEMORY errors may be returned at this point.
    //
    // The detailed actions for creating the session context are not shown here
    // as the details are implementation dependent
    // SessionCreate sets the output handle and nonceTPM
    result = SessionCreate(in->sessionType, in->authHash, &in->nonceCaller,
			   &in->symmetric, in->bind, &salt, &out->sessionHandle,
			   &out->nonceTPM);
    return result;
}
#endif // CC_StartAuthSession
#include "Tpm.h"
#include "PolicyRestart_fp.h"
#if CC_PolicyRestart  // Conditional expansion of this file
TPM_RC
TPM2_PolicyRestart(
		   PolicyRestart_In    *in             // IN: input parameter list
		   )
{
    // Initialize policy session data
    SessionResetPolicyData(SessionGet(in->sessionHandle));
    return TPM_RC_SUCCESS;
}
#endif // CC_PolicyRestart
