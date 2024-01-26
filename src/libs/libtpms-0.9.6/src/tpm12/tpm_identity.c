/********************************************************************************/
/*										*/
/*			TPM Identity Handling					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_identity.c $		*/
/*										*/
/* (c) Copyright IBM Corporation 2006, 2010.					*/
/*										*/
/* All rights reserved.								*/
/* 										*/
/* Redistribution and use in source and binary forms, with or without		*/
/* modification, are permitted provided that the following conditions are	*/
/* met:										*/
/* 										*/
/* Redistributions of source code must retain the above copyright notice,	*/
/* this list of conditions and the following disclaimer.			*/
/* 										*/
/* Redistributions in binary form must reproduce the above copyright		*/
/* notice, this list of conditions and the following disclaimer in the		*/
/* documentation and/or other materials provided with the distribution.		*/
/* 										*/
/* Neither the names of the IBM Corporation nor the names of its		*/
/* contributors may be used to endorse or promote products derived from		*/
/* this software without specific prior written permission.			*/
/* 										*/
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS		*/
/* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT		*/
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR	*/
/* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT		*/
/* HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,	*/
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT		*/
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,	*/
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY	*/
/* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT		*/
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE	*/
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.		*/
/********************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "tpm_auth.h"
#include "tpm_crypto.h"
#include "tpm_cryptoh.h"
#include "tpm_debug.h"
#include "tpm_digest.h"
#include "tpm_error.h"
#include "tpm_io.h"
#include "tpm_key.h"
#include "tpm_memory.h"
#include "tpm_pcr.h"
#include "tpm_process.h"
#include "tpm_secret.h"
#include "tpm_storage.h"
#include "tpm_ver.h"
#include "tpm_identity.h"

/*
  TPM_EK_BLOB
*/

/* TPM_EKBlob_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_EKBlob_Init(TPM_EK_BLOB *tpm_ek_blob)
{
    printf(" TPM_EKBlob_Init:\n");
    tpm_ek_blob->ekType = 0;
    TPM_SizedBuffer_Init(&(tpm_ek_blob->blob));
    return;
}

/* TPM_EKBlob_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_EKBlob_Init()
   After use, call TPM_EKBlob_Delete() to free memory
*/

TPM_RESULT TPM_EKBlob_Load(TPM_EK_BLOB *tpm_ek_blob,
			   unsigned char **stream,
			   uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_EKBlob_Load:\n");
    /* check the tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_EK_BLOB, stream, stream_size);
    }
    /* load ekType */
    if (rc == 0) {
	rc = TPM_Load16(&(tpm_ek_blob->ekType), stream, stream_size);
    }
    /* load blob */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_ek_blob->blob), stream, stream_size);
    }
    return rc;
}

#if 0
/* TPM_EKBlob_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_EKBlob_Store(TPM_STORE_BUFFER *sbuffer,
			    const TPM_EK_BLOB *tpm_ek_blob)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_EKBlob_Store:\n");
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_EK_BLOB); 
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, tpm_ek_blob->ekType); 
    }
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_ek_blob->blob)); 
    }
    return rc;
}
#endif

/* TPM_EKBlob_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_EKBlob_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_EKBlob_Delete(TPM_EK_BLOB *tpm_ek_blob)
{
    printf(" TPM_EKBlob_Delete:\n");
    if (tpm_ek_blob != NULL) {
	TPM_SizedBuffer_Delete(&(tpm_ek_blob->blob));
	TPM_EKBlob_Init(tpm_ek_blob);
    }
    return;
}

/*
  TPM_EK_BLOB_ACTIVATE
*/

/* TPM_EKBlobActivate_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_EKBlobActivate_Init(TPM_EK_BLOB_ACTIVATE *tpm_ek_blob_activate)
{
    printf(" TPM_EKBlobActivate_Init:\n");
    TPM_SymmetricKey_Init(&(tpm_ek_blob_activate->sessionKey));
    TPM_Digest_Init(tpm_ek_blob_activate->idDigest);
    TPM_PCRInfoShort_Init(&(tpm_ek_blob_activate->pcrInfo));
    return;
}

/* TPM_EKBlobActivate_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_EKBlobActivate_Init()
   After use, call TPM_EKBlobActivate_Delete() to free memory
*/

TPM_RESULT TPM_EKBlobActivate_Load(TPM_EK_BLOB_ACTIVATE *tpm_ek_blob_activate,
				   unsigned char **stream,
				   uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_EKBlobActivate_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_EK_BLOB_ACTIVATE, stream, stream_size);
    }
    /* load sessionKey */
    if (rc == 0) {
	rc = TPM_SymmetricKey_Load(&(tpm_ek_blob_activate->sessionKey), stream, stream_size);
    }
    /* load idDigest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_ek_blob_activate->idDigest, stream, stream_size);
    }
    /* load pcrInfo */
    if (rc == 0) {
	rc = TPM_PCRInfoShort_Load(&(tpm_ek_blob_activate->pcrInfo), stream, stream_size, FALSE);
    }
    return rc;
}

#if 0
/* TPM_EKBlobActivate_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_EKBlobActivate_Store(TPM_STORE_BUFFER *sbuffer,
				    const TPM_EK_BLOB_ACTIVATE *tpm_ek_blob_activate)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_EKBlobActivate_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_EK_BLOB_ACTIVATE); 
    }
    /* store sessionKey */
    if (rc == 0) {
	rc = TPM_SymmetricKey_Store(sbuffer, &(tpm_ek_blob_activate->sessionKey));
    }
    /* store idDigest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_ek_blob_activate->idDigest);
    }
    /* store pcrInfo */
    if (rc == 0) {
	rc = TPM_PCRInfoShort_Store(sbuffer, &(tpm_ek_blob_activate->pcrInfo), FALSE);
    }
    return rc;
}
#endif

/* TPM_EKBlobActivate_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_EKBlobActivate_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_EKBlobActivate_Delete(TPM_EK_BLOB_ACTIVATE *tpm_ek_blob_activate)
{
    printf(" TPM_EKBlobActivate_Delete:\n");
    if (tpm_ek_blob_activate != NULL) {
	TPM_SymmetricKey_Delete(&(tpm_ek_blob_activate->sessionKey));
	TPM_PCRInfoShort_Delete(&(tpm_ek_blob_activate->pcrInfo));
	TPM_EKBlobActivate_Init(tpm_ek_blob_activate);
    }
    return;
}

/*
  TPM_EK_BLOB_AUTH
*/

#if 0
/* TPM_EKBlobAuth_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_EKBlobAuth_Init(TPM_EK_BLOB_AUTH *tpm_ek_blob_auth)
{
    printf(" TPM_EKBlobAuth_Init:\n");
    TPM_Secret_Init(tpm_ek_blob_auth->authValue);
    return;
}

/* TPM_EKBlobAuth_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_EKBlobAuth_Init()
   After use, call TPM_EKBlobAuth_Delete() to free memory
*/

TPM_RESULT TPM_EKBlobAuth_Load(TPM_EK_BLOB_AUTH *tpm_ek_blob_auth,
			       unsigned char **stream,
			       uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_EKBlobAuth_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_EK_BLOB_AUTH, stream, stream_size);
    }
    /* load authValue */
    if (rc == 0) {
	rc = TPM_Secret_Load(tpm_ek_blob_auth->authValue, stream, stream_size);
    }
    return rc;
}

/* TPM_EKBlobAuth_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_EKBlobAuth_Store(TPM_STORE_BUFFER *sbuffer,
				const TPM_EK_BLOB_AUTH *tpm_ek_blob_auth)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_EKBlobAuth_Store:\n");
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_EK_BLOB_AUTH); 
    }
    if (rc == 0) {
	rc = TPM_Secret_Store(sbuffer, tpm_ek_blob_auth->authValue);
    }
    return rc;
}

/* TPM_EKBlobAuth_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_EKBlobAuth_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_EKBlobAuth_Delete(TPM_EK_BLOB_AUTH *tpm_ek_blob_auth)
{
    printf(" TPM_EKBlobAuth_Delete:\n");
    if (tpm_ek_blob_auth != NULL) {
	TPM_EKBlobAuth_Init(tpm_ek_blob_auth);
    }
    return;
}
#endif

/*
  TPM_IDENTITY_CONTENTS
*/

/* TPM_IdentityContents_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_IdentityContents_Init(TPM_IDENTITY_CONTENTS *tpm_identity_contents)
{
    printf(" TPM_IdentityContents_Init:\n");
    TPM_StructVer_Init(&(tpm_identity_contents->ver));
    tpm_identity_contents->ordinal = TPM_ORD_MakeIdentity;
    TPM_Digest_Init(tpm_identity_contents->labelPrivCADigest);
    TPM_Pubkey_Init(&(tpm_identity_contents->identityPubKey));
    return;
}

/* TPM_IdentityContents_Load()	

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_IdentityContents_Init()
   After use, call TPM_IdentityContents_Delete() to free memory

   NOTE: Never called.
*/
#if 0
TPM_RESULT TPM_IdentityContents_Load(TPM_IDENTITY_CONTENTS *tpm_identity_contents,
				     unsigned char **stream,
				     uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_IdentityContents_Load:\n");
    /* load ver */
    if (rc == 0) {
	rc = TPM_StructVer_Load(&(tpm_identity_contents->ver), stream, stream_size);
    }
    /* check ver immediately to ease debugging */
    if (rc == 0) {
	rc = TPM_StructVer_CheckVer(&(tpm_identity_contents->ver));
    }
    /* load ordinal */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_identity_contents->ordinal), stream, stream_size);
    }
    /* load labelPrivCADigest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_identity_contents->labelPrivCADigest, stream, stream_size);
    }
    /* load identityPubKey */
    if (rc == 0) {
	rc = TPM_Pubkey_Load(&(tpm_identity_contents->identityPubKey), stream, stream_size);
    }
    return rc;
}
#endif

/* TPM_IdentityContents_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_IdentityContents_Store(TPM_STORE_BUFFER *sbuffer,
				      TPM_IDENTITY_CONTENTS *tpm_identity_contents)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_IdentityContents_Store:\n");
    /* store ver */
    if (rc == 0) {
	rc = TPM_StructVer_Store(sbuffer, &(tpm_identity_contents->ver));
    }
    /* store ordinal */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_identity_contents->ordinal);
    }
    /* store labelPrivCADigest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_identity_contents->labelPrivCADigest);
    }
    /* store identityPubKey */
    if (rc == 0) {
	rc = TPM_Pubkey_Store(sbuffer, &(tpm_identity_contents->identityPubKey));
    }
    return rc;
}

/* TPM_IdentityContents_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_IdentityContents_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_IdentityContents_Delete(TPM_IDENTITY_CONTENTS *tpm_identity_contents)
{
    printf(" TPM_IdentityContents_Delete:\n");
    if (tpm_identity_contents != NULL) {
	TPM_Pubkey_Delete(&(tpm_identity_contents->identityPubKey));
	TPM_IdentityContents_Init(tpm_identity_contents);
    }
    return;
}

/*
  TPM_ASYM_CA_CONTENTS
*/

/* TPM_AsymCaContents_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_AsymCaContents_Init(TPM_ASYM_CA_CONTENTS *tpm_asym_ca_contents)
{
    printf(" TPM_AsymCaContents_Init:\n");
    TPM_SymmetricKey_Init(&(tpm_asym_ca_contents->sessionKey));
    TPM_Digest_Init(tpm_asym_ca_contents->idDigest);
    return;
}

/* TPM_AsymCaContents_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_AsymCaContents_Init()
   After use, call TPM_AsymCaContents_Delete() to free memory
*/

TPM_RESULT TPM_AsymCaContents_Load(TPM_ASYM_CA_CONTENTS *tpm_asym_ca_contents,
				   unsigned char **stream,
				   uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_AsymCaContents_Load:\n");
    if (rc == 0) {
	rc = TPM_SymmetricKey_Load(&(tpm_asym_ca_contents->sessionKey), stream, stream_size);
    }
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_asym_ca_contents->idDigest, stream, stream_size);
    }
    return rc;
}

#if 0
/* TPM_AsymCaContents_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_AsymCaContents_Store(TPM_STORE_BUFFER *sbuffer,
				    const TPM_ASYM_CA_CONTENTS *tpm_asym_ca_contents)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_AsymCaContents_Store:\n");
    if (rc == 0) {
	rc = TPM_SymmetricKey_Store(sbuffer, &(tpm_asym_ca_contents->sessionKey));
    }
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_asym_ca_contents->idDigest);
    }
    return rc;
}
#endif

/* TPM_AsymCaContents_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_AsymCaContents_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_AsymCaContents_Delete(TPM_ASYM_CA_CONTENTS *tpm_asym_ca_contents)
{
    printf(" TPM_AsymCaContents_Delete:\n");
    if (tpm_asym_ca_contents != NULL) {
	TPM_SymmetricKey_Delete(&(tpm_asym_ca_contents->sessionKey));
	TPM_AsymCaContents_Init(tpm_asym_ca_contents);
    }
    return;
}






/*
  Processing Functions
*/

/* 15.1 TPM_MakeIdentity rev 114

   Generate a new Attestation Identity Key (AIK)

   labelPrivCADigest identifies the privacy CA that the owner expects to be the target CA for the
   AIK.	 The selection is not enforced by the TPM.  It is advisory only.  It is included because the
   TSS cannot be trusted to send the AIK to the correct privacy CA.  The privacy CA can use this
   parameter to validate that it is the target privacy CA and label intended by the TPM owner at the
   time the key was created.  The label can be used to indicate an application purpose.

   The public key of the new TPM identity SHALL be identityPubKey. The private key of the new TPM
   identity SHALL be tpm_signature_key.

   Properties of the new identity

   TPM_PUBKEY identityPubKey This SHALL be the public key of a previously unused asymmetric key
   pair.

   TPM_STORE_ASYMKEY tpm_signature_key This SHALL be the private key that forms a pair with
   identityPubKey and SHALL be extant only in a TPM-shielded location.
   
   This capability also generates a TPM_KEY containing the tpm_signature_key.
   
   If identityPubKey is stored on a platform it SHALL exist only in storage to which access is
   controlled and is available to authorized entities.
*/

TPM_RESULT TPM_Process_MakeIdentity(tpm_state_t *tpm_state,
				    TPM_STORE_BUFFER *response,
				    TPM_TAG tag,
				    uint32_t paramSize,
				    TPM_COMMAND_CODE ordinal,
				    unsigned char *command,
				    TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_ENCAUTH		identityAuth;	/* Encrypted usage authorization data for the new identity
					   */
    TPM_CHOSENID_HASH	labelPrivCADigest;	/* The digest of the identity label and privacy CA
						   chosen for the new TPM identity. */
    TPM_KEY		idKeyParams;	/* Structure containing all parameters of new identity
					   key. pubKey.keyLength & idKeyParams.encData are both 0
					   MAY be TPM_KEY12 */
    TPM_AUTHHANDLE	srkAuthHandle;	/* The authorization handle used for SRK authorization. */
    TPM_NONCE		srknonceOdd;	/* Nonce generated by system associated with srkAuthHandle
					   */
    TPM_BOOL	continueSrkSession = TRUE;	/* Ignored */
    TPM_AUTHDATA	srkAuth;	/* The authorization digest for the inputs and the SRK. HMAC
					   key: srk.usageAuth. */
    TPM_AUTHHANDLE	authHandle;	/* The authorization handle used for owner
					   authorization. Session type MUST be OSAP. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* Ignored */
    TPM_AUTHDATA	ownerAuth;	/* The authorization digest for inputs and owner. HMAC key:
					   ownerAuth. */
    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			srkAuthHandleValid = FALSE;
    TPM_BOOL			authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*srk_auth_session_data = NULL;	/* session data for authHandle */
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;  /* session data for dataAuthHandle */
    TPM_SECRET			*srkHmacKey;
    TPM_SECRET			*hmacKey;
    TPM_SECRET			a1Auth;
    TPM_STORE_ASYMKEY		*idKeyStoreAsymkey;
    TPM_IDENTITY_CONTENTS	idContents;
    TPM_DIGEST			h1Digest;	/* digest of TPM_IDENTITY_CONTENTS structure */
    int				ver;

    /* output parameters */
    uint32_t			outParamStart;	/* starting point of outParam's */
    uint32_t			outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST			outParamDigest;
    TPM_KEY			idKey;		/* The newly created identity key. MAY be TPM_KEY12
						   */
    TPM_SIZED_BUFFER		identityBinding;	/* Signature of TPM_IDENTITY_CONTENTS using
							   idKey.private. */
    printf("TPM_Process_MakeIdentity: Ordinal Entry\n");
    TPM_Key_Init(&idKeyParams);			/* freed @1 */
    TPM_Key_Init(&idKey);			/* freed @2 */
    TPM_SizedBuffer_Init(&identityBinding);	/* freed @3 */
    TPM_IdentityContents_Init(&idContents);	/* freed @4 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get identityAuth parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Authdata_Load(identityAuth, &command, &paramSize);
    }
    /* get labelPrivCADigest parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Digest_Load(labelPrivCADigest, &command, &paramSize);
    }
    /* get idKeyParams parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_Load(&idKeyParams, &command, &paramSize);
    }
    /* save the ending point of inParam's for authorization and auditing */
    inParamEnd = command;
    /* digest the input parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_GetInParamDigest(inParamDigest,	/* output */
					  &auditStatus,		/* output */
					  &transportEncrypt,	/* output */
					  tpm_state,
					  tag,
					  ordinal,
					  inParamStart,
					  inParamEnd,
					  transportInternal);
    }
    /* check state */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_ALL);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag21(tag);
    }
    /* get the optional 'below the line' authorization parameters */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_AuthParams_Get(&srkAuthHandle,
					&srkAuthHandleValid,
					srknonceOdd,
					&continueSrkSession,
					srkAuth,
					&command, &paramSize);
	printf("TPM_Process_MakeIdentity: srkAuthHandle %08x\n", srkAuthHandle);
    }
    /* get the 'below the line' authorization parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					ownerAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_MakeIdentity: authHandle %08x\n", authHandle); 
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_MakeIdentity: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	srkAuthHandleValid = FALSE;
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* 1. Validate the idKeyParams parameters for the key description */
    /* a. If the algorithm type is RSA the key length MUST be a minimum of 2048 and MUST use the
       default exponent. For interoperability the key length SHOULD be 2048 */
    /* b. If the algorithm type is other than RSA the strength provided by the key MUST be
       comparable to RSA 2048 */
    /* c. If the TPM is not designed to create a key of the requested type, return the error code
       TPM_BAD_KEY_PROPERTY */
    /* d. If TPM_PERMANENT_FLAGS -> FIPS is TRUE then */
    /* i. If authDataUsage specifies TPM_AUTH_NEVER return TPM_NOTFIPS */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_CheckProperties(&ver, &idKeyParams, 2048,
					     tpm_state->tpm_permanent_flags.FIPS);
	printf("TPM_Process_MakeIdentity: key parameters v = %d\n", ver);
    }
    /* 2. Use authHandle to verify that the Owner authorized all TPM_MakeIdentity input
       parameters. */
    /* get the session data */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_OSAP,
					      TPM_ET_OWNER,
					      ordinal,
					      NULL,
					      NULL,
					      tpm_state->tpm_permanent_data.ownerAuth);
    }
    /* Validate the authorization to use the key pointed to by keyHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Auth2data_Check(tpm_state,
					 *hmacKey,		/* HMAC key */
					 inParamDigest,
					 auth_session_data,	/* authorization session */
					 nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					 continueAuthSession,
					 ownerAuth);		/* Authorization digest for input */
    }
    /* 3. Use srkAuthHandle to verify that the SRK owner authorized all TPM_MakeIdentity input
       parameters. */
    /* get the session data */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_AuthSessions_GetData
		     (&srk_auth_session_data,
		      &srkHmacKey,
		      tpm_state,
		      srkAuthHandle,
		      TPM_PID_NONE,
		      TPM_ET_KEYHANDLE,
		      ordinal,
		      &(tpm_state->tpm_permanent_data.srk),
		      /* OIAP */
		      &(tpm_state->tpm_permanent_data.srk.tpm_store_asymkey->usageAuth),
		      /* OSAP */
		      tpm_state->tpm_permanent_data.srk.tpm_store_asymkey->pubDataDigest);
    }
    /* Validate the authorization to use the key pointed to by keyHandle */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*srkHmacKey,		/* HMAC key */
					inParamDigest,
					srk_auth_session_data, /* authorization session */
					srknonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueSrkSession,
					srkAuth);		/* Authorization digest for input */
    }
    /* if there is no SRK authorization, check that the SRK authDataUsage is TPM_AUTH_NEVER */
    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_AUTH2_COMMAND)) {
	if (tpm_state->tpm_permanent_data.srk.authDataUsage != TPM_AUTH_NEVER) {
	    printf("TPM_Process_MakeIdentity: Error, SRK authorization required\n");
	    returnCode = TPM_AUTHFAIL;
	}
    }
    /* 4. Verify that idKeyParams -> keyUsage is TPM_KEY_IDENTITY. If it is not, return
       TPM_INVALID_KEYUSAGE */
    /* NOTE: TPM_KEY_IDENTITY keys must use TPM_SS_RSASSAPKCS1v15_SHA1 */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_MakeIdentity: Checking key parameters\n");
	if (idKeyParams.keyUsage != TPM_KEY_IDENTITY) {
	    printf("TPM_Process_MakeIdentity: Error, "
		   "idKeyParams keyUsage %08x should be TPM_KEY_IDENTITY\n",
		   idKeyParams.keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 5. Verify that idKeyParams -> keyFlags -> migratable is FALSE. If it is not, return
       TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	if (idKeyParams.keyFlags & TPM_MIGRATABLE) {
	    printf("TPM_Process_MakeIdentity: Error, "
		   "idKeyParams keyFlags %08x cannot be migratable\n",
		   idKeyParams.keyFlags);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 6. Create a1 by decrypting identityAuth according to the ADIP indicated by authHandle. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessionData_Decrypt(a1Auth,
						 NULL,
						 identityAuth,
						 auth_session_data,
						 NULL,
						 NULL,
						 FALSE);	/* even and odd */
    }
    /* 7. Set continueAuthSession and continueSRKSession to FALSE. */
    if (returnCode == TPM_SUCCESS) {
	continueAuthSession = FALSE;
	continueSrkSession = FALSE;
	/* 8. Determine the structure version */
	/* a. If idKeyParams -> tag is TPM_TAG_KEY12 */
	/* i. Set V1 to 2 */
	/* ii. Create idKey a TPM_KEY12 structure using idKeyParams as the default values for the
	       structure */
	/* b. If idKeyParams -> ver is 1.1	*/
	/* i. Set V1 to 1 */
	/* ii. Create idKey a TPM_KEY structure using idKeyParams as the default values for the
	       structure */
	/* NOTE Done by TPM_Key_CheckProperties() */
	/* NOTE The creation determination is done by TPM_Key_GenerateRSA() */
    }
    /* 9. Set the digestAtCreation values for pcrInfo */
    /* NOTE Done as the key is generated */
    /* a. For PCR_INFO_LONG include the locality of the current command */
    /* 10. Create an asymmetric key pair (identityPubKey and tpm_signature_key) using a
       TPM-protected capability, in accordance with the algorithm specified in idKeyParams */
    if (returnCode == TPM_SUCCESS) {
	/* generate the key pair, create the tpm_store_asymkey cache, copy key parameters, create
	   tpm_pcr_info cache, copies pcr parameters, sets digestAtCreation, sets pubKey, serializes
	   pcrInfo
	   
	   does not set encData */
	printf("TPM_Process_MakeIdentity: Generating key\n");
	returnCode = TPM_Key_GenerateRSA(&idKey,
					 tpm_state,
					 &(tpm_state->tpm_permanent_data.srk),	/* parent key */
					 tpm_state->tpm_stclear_data.PCRS,	/* PCR array */
					 ver,
					 idKeyParams.keyUsage,
					 idKeyParams.keyFlags,
					 idKeyParams.authDataUsage,	/* TPM_AUTH_DATA_USAGE */
					 &(idKeyParams.algorithmParms), /* TPM_KEY_PARMS */
					 idKeyParams.tpm_pcr_info,	/* TPM_PCR_INFO */
					 idKeyParams.tpm_pcr_info_long);/* TPM_PCR_INFO_LONG */

    }
    /* 11. Ensure that the authorization information in A1 is properly stored in the idKey as
       usageAuth. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_GetStoreAsymkey(&idKeyStoreAsymkey,
					     &idKey);
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_Secret_Copy(idKeyStoreAsymkey->usageAuth, a1Auth);
	/* 12. Attach identityPubKey and tpm_signature_key to idKey */
	/* Note: Done as the key is generated */
	/* 13. Set idKey -> migrationAuth to TPM_PERMANENT_DATA -> tpmProof */
	TPM_Secret_Copy(idKeyStoreAsymkey->migrationAuth, tpm_state->tpm_permanent_data.tpmProof);
	/* 14. Ensure that all TPM_PAYLOAD_TYPE structures identity this key as TPM_PT_ASYM */
	/* NOTE Done as the key is generated */
    }	 
    /* 15. Encrypt the private portion of idKey using the SRK as the parent key */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_MakeIdentity: Encrypting key private part with SRK\n");
	returnCode = TPM_Key_GenerateEncData(&idKey, &(tpm_state->tpm_permanent_data.srk));
    }
    /* 16. Create a TPM_IDENTITY_CONTENTS structure named idContents using labelPrivCADigest and the
       information from idKey */
    if (returnCode == TPM_SUCCESS) {
	TPM_Digest_Copy(idContents.labelPrivCADigest, labelPrivCADigest);
	returnCode = TPM_Pubkey_Set(&(idContents.identityPubKey), &idKey);
    }
    /* 17. Sign idContents using tpm_signature_key and TPM_SS_RSASSAPKCS1v15_SHA1. Store the result
       in identityBinding. */
    /* NOTE: TPM_Key_CheckProperties() verified TPM_SS_RSASSAPKCS1v15_SHA1 */
    /* serialize tpm_identity_contents and hash the results*/
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SHA1_GenerateStructure(h1Digest,
						&idContents,
						(TPM_STORE_FUNCTION_T)TPM_IdentityContents_Store);
    }
    /* sign the digest */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_MakeIdentity: Signing digest of TPM_IDENTITY_CONTENTS\n");
	returnCode = TPM_RSASignToSizedBuffer(&identityBinding, h1Digest, TPM_DIGEST_SIZE, &idKey);
    }
#if 0	/* NOTE Debug code to reverse the signature */
    if (returnCode == TPM_SUCCESS) {
	unsigned char 	*message = NULL;
	unsigned char 	*narr = NULL;
	uint32_t 	nbytes;
	unsigned char 	*earr = NULL;
	uint32_t 	ebytes;
	if (returnCode == 0) {
	    returnCode = TPM_Malloc(&message, identityBinding.size); /* freed @10 */
	}
	if (returnCode == 0) {
	    returnCode = TPM_Key_GetPublicKey(&nbytes, &narr, &idKey);
	}
	if (returnCode == 0) {
	    returnCode = TPM_Key_GetExponent(&ebytes, &earr, &idKey);
	}
	if (returnCode == 0) {
	    returnCode = TPM_RSAPublicEncryptRaw(message,			/* output */
						 identityBinding.size,	
						 identityBinding.buffer,	/* input */
						 identityBinding.size,	
						 narr,			/* public modulus */
						 nbytes,
						 earr,			/* public exponent */
						 ebytes);
	}
	free(message);	/* @10 */
    }
#endif
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_MakeIdentity: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return idKey */
	    returnCode = TPM_Key_Store(response, &idKey);
	}
	if (returnCode == TPM_SUCCESS) {
	    /* return identityBinding */
	    returnCode = TPM_SizedBuffer_Store(response, &identityBinding);
	    /* checkpoint the end of the outParam's */
	    outParamEnd = response->buffer_current - response->buffer;
	}
	/* digest the above the line output parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_GetOutParamDigest(outParamDigest,	/* output */
					       auditStatus,	/* input audit status */
					       transportEncrypt,
					       tag,			
					       returnCode,
					       ordinal,		/* command ordinal */
					       response->buffer + outParamStart,	/* start */
					       outParamEnd - outParamStart);	/* length */
	}
	/* calculate and set the below the line parameters */
	if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	    returnCode = TPM_AuthParams_Set(response,
					    *srkHmacKey,	/* owner HMAC key */
					    srk_auth_session_data,
					    outParamDigest,
					    srknonceOdd,
					    continueSrkSession);
	}
	/* calculate and set the below the line parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_AuthParams_Set(response,
					    *hmacKey,	/* owner HMAC key */
					    auth_session_data,
					    outParamDigest,
					    nonceOdd,
					    continueAuthSession);
	}
	/* audit if required */
	if ((returnCode == TPM_SUCCESS) && auditStatus) {
	    returnCode = TPM_ProcessAudit(tpm_state,
					  transportEncrypt,
					  inParamDigest,
					  outParamDigest,
					  ordinal);
	}
	/* adjust the initial response */
	rcf = TPM_Sbuffer_StoreFinalResponse(response, returnCode, tpm_state);
    }
    /* if there was an error, or continueSrkSession is FALSE, terminate the session */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueSrkSession) &&
	srkAuthHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, srkAuthHandle);
    }
    /* if there was an error, or continueAuthSession is FALSE, terminate the session */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    TPM_Key_Delete(&idKeyParams);		/* freed @1 */
    TPM_Key_Delete(&idKey);			/* freed @2 */
    TPM_SizedBuffer_Delete(&identityBinding);	/* freed @3 */
    TPM_IdentityContents_Delete(&idContents);	/* freed @4 */
    return rcf;
}

/* 15.2 TPM_ActivateIdentity rev 107

  The purpose of TPM_ActivateIdentity is to twofold. The first purpose is to obtain assurance that
  the credential in the TPM_SYM_CA_ATTESTATION is for this TPM. The second purpose is to obtain the
  session key used to encrypt the TPM_IDENTITY_CREDENTIAL.

  The command TPM_ActivateIdentity activates a TPM identity created using the command
  TPM_MakeIdentity.
  
  The command assumes the availability of the private key associated with the identity. The command
  will verify the association between the keys during the process.
  
  The command will decrypt the input blob and extract the session key and verify the connection
  between the public and private keys. The input blob can be in 1.1 or 1.2 format.
*/

TPM_RESULT TPM_Process_ActivateIdentity(tpm_state_t *tpm_state,
					TPM_STORE_BUFFER *response,
					TPM_TAG tag,
					uint32_t paramSize,
					TPM_COMMAND_CODE ordinal,
					unsigned char *command,
					TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_KEY_HANDLE	idKeyHandle;	/* handle of identity key to be activated */
    TPM_SIZED_BUFFER	blob;		/* The encrypted ASYM_CA_CONTENTS or TPM_EK_BLOB */
    TPM_AUTHHANDLE	idKeyAuthHandle;	/* The authorization handle used for ID key
						   authorization. */
    TPM_NONCE		idKeynonceOdd;	/* Nonce generated by system associated with idKeyAuthHandle
					   */
    TPM_BOOL	continueIdKeySession = TRUE;	/* Continue usage flag for idKeyAuthHandle. */
    TPM_AUTHDATA	idKeyAuth;	/* The authorization digest for the inputs and ID key. HMAC
					   key: idKey.usageAuth. */
    TPM_AUTHHANDLE	authHandle;	/* The authorization handle used for owner authorization. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   handle */
    TPM_AUTHDATA		ownerAuth;	/* The authorization digest for inputs and
						   owner. HMAC key: ownerAuth. */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			idKeyAuthHandleValid = FALSE;
    TPM_BOOL			authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*id_key_auth_session_data = NULL; /* session data for authHandle */
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL; /* session data for dataAuthHandle */
    TPM_SECRET			*idKeyHmacKey;
    TPM_SECRET			*hmacKey;
    TPM_KEY			*idKey;			/* Identity key to be activated */
    TPM_SECRET			*idKeyUsageAuth;
    TPM_BOOL			idPCRStatus;
    TPM_DIGEST			h1Digest;		/* digest of public key in idKey */
    unsigned char		*b1Blob = NULL;		/* decrypted blob */
    uint32_t			b1BlobLength = 0;	/* actual valid data */
    TPM_STRUCTURE_TAG		hTag;			/* b1 tag in host byte order */
    int				vers = 0;		/* version of blob */
    unsigned char		*stream;
    uint32_t			stream_size;
    TPM_EK_BLOB			b1EkBlob;
    TPM_ASYM_CA_CONTENTS	b1AsymCaContents;
    TPM_SYMMETRIC_KEY		*k1 = NULL;
    TPM_EK_BLOB_ACTIVATE	a1;

    /* output parameters */
    uint32_t			outParamStart;	/* starting point of outParam's */
    uint32_t			outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST			outParamDigest;
    TPM_SYMMETRIC_KEY		symmetricKey;	/* The decrypted symmetric key. */

    printf("TPM_Process_ActivateIdentity: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&blob);		/* freed @1 */
    TPM_SymmetricKey_Init(&symmetricKey);	/* freed @2 */
    TPM_AsymCaContents_Init(&b1AsymCaContents); /* freed @4 */
    TPM_EKBlob_Init(&b1EkBlob);			/* freed @5 */
    TPM_EKBlobActivate_Init(&a1);		/* freed @6 */
    /*
      get inputs
    */
    /* get idKey parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&idKeyHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get blob parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&blob, &command, &paramSize);
    }
    /* save the ending point of inParam's for authorization and auditing */
    inParamEnd = command;
    /* digest the input parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_GetInParamDigest(inParamDigest,	/* output */
					  &auditStatus,		/* output */
					  &transportEncrypt,	/* output */
					  tpm_state,
					  tag,
					  ordinal,
					  inParamStart,
					  inParamEnd,
					  transportInternal);
    }
    /* check state */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_ALL);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag21(tag);
    }
    /* get the optional 'below the line' authorization parameters */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_AuthParams_Get(&idKeyAuthHandle,
					&idKeyAuthHandleValid,
					idKeynonceOdd,
					&continueIdKeySession,
					idKeyAuth,
					&command, &paramSize);
	printf("TPM_Process_ActivateIdentity: idKeyAuthHandle %08x\n", idKeyAuthHandle);
    }
    /* get the 'below the line' authorization parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					ownerAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_ActivateIdentity: authHandle %08x\n", authHandle); 
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_ActivateIdentity: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	idKeyAuthHandleValid = FALSE;
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* 1. Using the authHandle field, validate the owner's authorization to execute the command and
       all of the incoming parameters. */
    /* get the session data */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_NONE,
					      TPM_ET_OWNER,
					      ordinal,
					      NULL,
					      &(tpm_state->tpm_permanent_data.ownerAuth), /* OIAP */
					      tpm_state->tpm_permanent_data.ownerAuth);	  /* OSAP */
    }
    /* Validate the authorization to use the key pointed to by keyHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					ownerAuth);		/* Authorization digest for input */
    }
    /* 2. Using the idKeyAuthHandle, validate the authorization to execute command and all of the
       incoming parameters */
    /* get the idKey */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&idKey, &idPCRStatus, tpm_state, idKeyHandle,
						 FALSE,		/* not r/o, using to authenticate */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    /* get keyHandle -> usageAuth */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_GetUsageAuth(&idKeyUsageAuth, idKey);
    }	 
    /* get the session data */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_AuthSessions_GetData(&id_key_auth_session_data,
					      &idKeyHmacKey,
					      tpm_state,
					      idKeyAuthHandle,
					      TPM_PID_NONE,
					      TPM_ET_KEYHANDLE,
					      ordinal,
					      idKey,
					      idKeyUsageAuth,		/* OIAP */
					      idKey->tpm_store_asymkey->pubDataDigest); /* OSAP */
    }
    /* Validate the authorization to use the key pointed to by keyHandle */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_Auth2data_Check(tpm_state,
					 *idKeyHmacKey,		/* HMAC key */
					 inParamDigest,
					 id_key_auth_session_data,	/* authorization session */
					 idKeynonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					 continueIdKeySession,
					 idKeyAuth);		/* Authorization digest for input */
    }
    /* if there is no idKey authorization, check that the idKey -> authDataUsage is TPM_AUTH_NEVER
       */
    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_AUTH2_COMMAND)) {
	if (idKey->authDataUsage != TPM_AUTH_NEVER) {
	    printf("TPM_Process_ActivateIdentity: Error, ID key authorization required\n");
	    returnCode = TPM_AUTHFAIL;
	}
    }
    /* 3. Validate that the idKey is the public key of a valid TPM identity by checking that
       idKeyHandle -> keyUsage is TPM_KEY_IDENTITY. Return TPM_BAD_PARAMETER on mismatch */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_ActivateIdentity: Checking for identity key\n");
	if (idKey->keyUsage != TPM_KEY_IDENTITY) {
	    printf("TPM_Process_ActivateIdentity: Error, keyUsage %04hx must be TPM_KEY_IDENTITY\n",
		   idKey->keyUsage);
	    returnCode = TPM_BAD_PARAMETER;
	}
    }
    /* 4. Create H1 the digest of a TPM_PUBKEY derived from idKey */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_GeneratePubkeyDigest(h1Digest, idKey);
    }
    /* 5. Decrypt blob creating B1 using PRIVEK as the decryption key */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_ActivateIdentity: Decrypting blob with EK\n");
	returnCode = TPM_RSAPrivateDecryptMalloc(&b1Blob,	/* decrypted data */
						 &b1BlobLength,	/* actual size of b1 data */
						 blob.buffer,
						 blob.size,
						 &(tpm_state->tpm_permanent_data.endorsementKey));
    }
    /* 6. Determine the type and version of B1 */
    if (returnCode == TPM_SUCCESS) {
	stream = b1Blob;		/* b1 must be preserved for the free */
	stream_size = b1BlobLength;
	/* convert possible tag to uint16_t */
	hTag = ntohs(*(TPM_STRUCTURE_TAG *)b1Blob);
	/* a. If B1 -> tag is TPM_TAG_EK_BLOB then */
	if (hTag == TPM_TAG_EK_BLOB) {
	    /* i. B1 is a TPM_EK_BLOB */
	    printf("TPM_Process_ActivateIdentity: b1 is TPM_EK_BLOB\n");
	    vers = 2;
	    returnCode = TPM_EKBlob_Load(&b1EkBlob, &stream, &stream_size);
	}
	/* b. Else */
	else {
	    /* i. B1 is a TPM_ASYM_CA_CONTENTS. As there is no tag for this structure it is possible
	       for the TPM to make a mistake here but other sections of the structure undergo
	       validation */
	    printf("TPM_Process_ActivateIdentity: b1 is TPM_ASYM_CA_CONTENTS\n");
	    vers = 1;
	    returnCode = TPM_AsymCaContents_Load(&b1AsymCaContents, &stream, &stream_size);
	}
    }
    /* 7. If B1 is a version 1.1 TPM_ASYM_CA_CONTENTS then */
    if ((returnCode == TPM_SUCCESS) && (vers == 1)) {
	/* a. Compare H1 to B1 -> idDigest on mismatch return TPM_BAD_PARAMETER */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Digest_Compare(h1Digest, b1AsymCaContents.idDigest);
	    if (returnCode != 0) {
		printf("TPM_Process_ActivateIdentity: Error "
		       "comparing TPM_ASYM_CA_CONTENTS idDigest\n");
		returnCode = TPM_BAD_PARAMETER;
	    }
	}
	/* b. Set K1 to B1 -> sessionKey */
	if (returnCode == TPM_SUCCESS) {
	    k1 = &(b1AsymCaContents.sessionKey);
	}
    }
    /* 8. If B1 is a TPM_EK_BLOB then */
    if ((returnCode == TPM_SUCCESS) && (vers == 2)) {
	/* a. Validate that B1 -> ekType is TPM_EK_TYPE_ACTIVATE, return TPM_BAD_TYPE if not. */
	if (returnCode == TPM_SUCCESS) {
	    if (b1EkBlob.ekType != TPM_EK_TYPE_ACTIVATE) {
		printf("TPM_Process_ActivateIdentity: Error, "
		       "TPM_EK_BLOB not ekType TPM_EK_TYPE_ACTIVATE\n");
		returnCode = TPM_BAD_TYPE;
	    }
	}
	/* b. Assign A1 as a TPM_EK_BLOB_ACTIVATE structure from B1 -> blob */
	if (returnCode == TPM_SUCCESS) {
	    stream = b1EkBlob.blob.buffer;
	    stream_size =  b1EkBlob.blob.size;
	    returnCode = TPM_EKBlobActivate_Load(&a1, &stream, &stream_size);
	}
	/* c. Compare H1 to A1 -> idDigest on mismatch return TPM_BAD_PARAMETER */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Digest_Compare(h1Digest, a1.idDigest);
	    if (returnCode != 0) {
		printf("TPM_Process_ActivateIdentity: Error "
		       "comparing TPM_EK_TYPE_ACTIVATE idDigest\n");
		returnCode = TPM_BAD_PARAMETER;
	    }
	}
	/* d. If A1 -> pcrSelection is not NULL */
	/* i. Compute a composite hash C1 using the PCR selection A1 -> pcrSelection */
	/* ii. Compare C1 to A1 -> pcrInfo -> digestAtRelease and return TPM_WRONGPCRVAL on a
	   mismatch */
	/* e. If A1 -> pcrInfo specifies a locality ensure that the appropriate locality has been
	   asserted, return TPM_BAD_LOCALITY on error */
	if (returnCode == TPM_SUCCESS) {
	    if (returnCode == TPM_SUCCESS) {
		returnCode =
		    TPM_PCRInfoShort_CheckDigest(&(a1.pcrInfo),
						 tpm_state->tpm_stclear_data.PCRS, /* PCR array */
						 tpm_state->tpm_stany_flags.localityModifier);
	    }
	}
	/* f. Set K1 to A1 -> symmetricKey */
	if (returnCode == TPM_SUCCESS) {
	    k1 = &(a1.sessionKey);
	}
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_ActivateIdentity: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* 9. Return K1 */
	    returnCode = TPM_SymmetricKey_Store(response, k1);
	    /* checkpoint the end of the outParam's */
	    outParamEnd = response->buffer_current - response->buffer;
	}
	/* digest the above the line output parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_GetOutParamDigest(outParamDigest,	/* output */
					       auditStatus,	/* input audit status */
					       transportEncrypt,
					       tag,			
					       returnCode,
					       ordinal,		/* command ordinal */
					       response->buffer + outParamStart,	/* start */
					       outParamEnd - outParamStart);	/* length */
	}
	/* calculate and set the below the line parameters */
	if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	    returnCode = TPM_AuthParams_Set(response,
					    *idKeyHmacKey,	/* owner HMAC key */
					    id_key_auth_session_data,
					    outParamDigest,
					    idKeynonceOdd,
					    continueIdKeySession);
	}
	/* calculate and set the below the line parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_AuthParams_Set(response,
					    *hmacKey,	/* owner HMAC key */
					    auth_session_data,
					    outParamDigest,
					    nonceOdd,
					    continueAuthSession);
	}
	/* audit if required */
	if ((returnCode == TPM_SUCCESS) && auditStatus) {
	    returnCode = TPM_ProcessAudit(tpm_state,
					  transportEncrypt,
					  inParamDigest,
					  outParamDigest,
					  ordinal);
	}
	/* adjust the initial response */
	rcf = TPM_Sbuffer_StoreFinalResponse(response, returnCode, tpm_state);
    }
    /* if there was an error, or continueAuthSession is FALSE, terminate the session */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueIdKeySession) &&
	idKeyAuthHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, idKeyAuthHandle);
    }
    /* if there was an error, or continueAuthSession is FALSE, terminate the session */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
	    

    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&blob);			/* @1 */
    TPM_SymmetricKey_Delete(&symmetricKey);		/* @2 */
    free(b1Blob);					/* @3 */
    TPM_AsymCaContents_Delete(&b1AsymCaContents);	/* @4 */
    TPM_EKBlob_Delete(&b1EkBlob);			/* @5 */
    TPM_EKBlobActivate_Delete(&a1);			/* @6 */
    return rcf;
}
