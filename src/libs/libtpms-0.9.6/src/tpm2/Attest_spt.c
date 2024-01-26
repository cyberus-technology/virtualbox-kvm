/********************************************************************************/
/*										*/
/*			     				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: Attest_spt.c $		*/
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
/*  (c) Copyright IBM Corp. and others, 2016 - 2018				*/
/*										*/
/********************************************************************************/

#include "Tpm.h"
#include "Attest_spt_fp.h"
/* 7.2.2 Functions */
/* 7.2.2.1 FillInAttestInfo() */
/* Fill in common fields of TPMS_ATTEST structure. */
void
FillInAttestInfo(
		 TPMI_DH_OBJECT       signHandle,    // IN: handle of signing object
		 TPMT_SIG_SCHEME     *scheme,        // IN/OUT: scheme to be used for signing
		 TPM2B_DATA          *data,          // IN: qualifying data
		 TPMS_ATTEST         *attest         // OUT: attest structure
		 )
{
    OBJECT              *signObject = HandleToObject(signHandle);
    // Magic number
    attest->magic = TPM_GENERATED_VALUE;
    if(signObject == NULL)
	{
	    // The name for a null handle is TPM_RH_NULL
	    // This is defined because UINT32_TO_BYTE_ARRAY does a cast. If the
	    // size of the cast is smaller than a constant, the compiler warns
	    // about the truncation of a constant value.
	    TPM_HANDLE      nullHandle = TPM_RH_NULL;
	    attest->qualifiedSigner.t.size = sizeof(TPM_HANDLE);
	    UINT32_TO_BYTE_ARRAY(nullHandle, attest->qualifiedSigner.t.name);
	}
    else
	{
	    // Certifying object qualified name
	    // if the scheme is anonymous, this is an empty buffer
	    if(CryptIsSchemeAnonymous(scheme->scheme))
		attest->qualifiedSigner.t.size = 0;
	    else
		attest->qualifiedSigner = signObject->qualifiedName;
	}
    // current clock in plain text
    TimeFillInfo(&attest->clockInfo);
    // Firmware version in plain text
    attest->firmwareVersion = ((UINT64)gp.firmwareV1 << (sizeof(UINT32) * 8));
    attest->firmwareVersion += gp.firmwareV2;
    // Check the hierarchy of sign object.  For NULL sign handle, the hierarchy
    // will be TPM_RH_NULL
    if((signObject == NULL)
       || (!signObject->attributes.epsHierarchy
	   && !signObject->attributes.ppsHierarchy))
	{
	    // For signing key that is not in platform or endorsement hierarchy,
	    // obfuscate the reset, restart and firmware version information
	    UINT64          obfuscation[2];
	    CryptKDFa(CONTEXT_INTEGRITY_HASH_ALG, &gp.shProof.b, OBFUSCATE_STRING,
		      &attest->qualifiedSigner.b, NULL, 128,
		      (BYTE *)&obfuscation[0], NULL, FALSE);
	    // Obfuscate data
	    attest->firmwareVersion += obfuscation[0];
	    attest->clockInfo.resetCount += (UINT32)(obfuscation[1] >> 32);
	    attest->clockInfo.restartCount += (UINT32)obfuscation[1];
	}
    // External data
    if(CryptIsSchemeAnonymous(scheme->scheme))
	attest->extraData.t.size = 0;
    else
	{
	    // If we move the data to the attestation structure, then it is not
	    // used in the signing operation except as part of the signed data
	    attest->extraData = *data;
	    data->t.size = 0;
	}
}
/* 7.2.2.2 SignAttestInfo() */
/* Sign a TPMS_ATTEST structure. If signHandle is TPM_RH_NULL, a null signature is returned. */
/* Error Returns Meaning */
/* TPM_RC_ATTRIBUTES signHandle references not a signing key */
/* TPM_RC_SCHEME scheme is not compatible with signHandle type */
/* TPM_RC_VALUE digest generated for the given scheme is greater than the modulus of signHandle (for
   an RSA key); invalid commit status or failed to generate r value (for an ECC key) */
TPM_RC
SignAttestInfo(
	       OBJECT              *signKey,           // IN: sign object
	       TPMT_SIG_SCHEME     *scheme,            // IN: sign scheme
	       TPMS_ATTEST         *certifyInfo,       // IN: the data to be signed
	       TPM2B_DATA          *qualifyingData,    // IN: extra data for the signing
	       //     process
	       TPM2B_ATTEST        *attest,            // OUT: marshaled attest blob to be
	       //     signed
	       TPMT_SIGNATURE      *signature          // OUT: signature
	       )
{
    BYTE                    *buffer;
    HASH_STATE              hashState;
    TPM2B_DIGEST            digest;
    TPM_RC                  result;
    // Marshal TPMS_ATTEST structure for hash
    buffer = attest->t.attestationData;
    attest->t.size = TPMS_ATTEST_Marshal(certifyInfo, &buffer, NULL);
    if(signKey == NULL)
	{
	    signature->sigAlg = TPM_ALG_NULL;
	    result = TPM_RC_SUCCESS;
	}
    else
	{
	    TPMI_ALG_HASH           hashAlg;
	    // Compute hash
	    hashAlg = scheme->details.any.hashAlg;
	    // need to set the receive buffer to get something put in it
	    digest.t.size = sizeof(digest.t.buffer);
	    digest.t.size = CryptHashBlock(hashAlg, attest->t.size,
					   attest->t.attestationData,
					   digest.t.size, digest.t.buffer);
	    // If there is qualifying data, need to rehash the data
	    // hash(qualifyingData || hash(attestationData))
	    if(qualifyingData->t.size != 0)
		{
		    CryptHashStart(&hashState, hashAlg);
		    CryptDigestUpdate2B(&hashState, &qualifyingData->b);
		    CryptDigestUpdate2B(&hashState, &digest.b);
		    CryptHashEnd2B(&hashState, &digest.b);
		}
	    // Sign the hash. A TPM_RC_VALUE, TPM_RC_SCHEME, or
	    // TPM_RC_ATTRIBUTES error may be returned at this point
	    result = CryptSign(signKey, scheme, &digest, signature);
	    // Since the clock is used in an attestation, the state in NV is no longer
	    // "orderly" with respect to the data in RAM if the signature is valid
	    if(result == TPM_RC_SUCCESS)
		{
		    // Command uses the clock so need to clear the orderly state if it is
		    // set.
		    result = NvClearOrderly();
		}
	}
    return result;
}
/* 7.2.2.3 IsSigningObject() */
/* Checks to see if the object is OK for signing. This is here rather than in Object_spt.c because
   all the attestation commands use this file but not Object_spt.c. */
/* Return Values Meaning */
/* TRUE object may sign */
/* FALSE object may not sign */
BOOL
IsSigningObject(
		OBJECT          *object         // IN:
		)
{
    return ((object == NULL)
	    || ((IS_ATTRIBUTE(object->publicArea.objectAttributes, TPMA_OBJECT, sign)
		 && object->publicArea.type != TPM_ALG_SYMCIPHER)));
}

