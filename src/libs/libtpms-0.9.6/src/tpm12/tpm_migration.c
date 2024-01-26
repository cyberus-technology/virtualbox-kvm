/********************************************************************************/
/*										*/
/*				TPM Migration					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_migration.c $		*/
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
#include <string.h>

#include "tpm_auth.h"
#include "tpm_crypto.h"
#include "tpm_cryptoh.h"
#include "tpm_debug.h"
#include "tpm_digest.h"
#include "tpm_error.h"
#include "tpm_io.h"
#include "tpm_key.h"
#include "tpm_memory.h"
#include "tpm_nonce.h"
#include "tpm_permanent.h"
#include "tpm_process.h"
#include "tpm_secret.h"

#include "tpm_migration.h"

/*
  TPM_MIGRATIONKEYAUTH
*/
  
/* TPM_Migrationkeyauth_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_Migrationkeyauth_Init(TPM_MIGRATIONKEYAUTH *tpm_migrationkeyauth)
{
    printf(" TPM_Migrationkeyauth_Init:\n");
    TPM_Pubkey_Init(&(tpm_migrationkeyauth->migrationKey));
    tpm_migrationkeyauth->migrationScheme = 0; 
    TPM_Digest_Init(tpm_migrationkeyauth->digest); 
    return;
}

/* TPM_Migrationkeyauth_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_Migrationkeyauth_Init()
   After use, call TPM_Migrationkeyauth_Delete() to free memory
*/

TPM_RESULT TPM_Migrationkeyauth_Load(TPM_MIGRATIONKEYAUTH *tpm_migrationkeyauth,
				     unsigned char **stream,
				     uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_Migrationkeyauth_Load:\n");
    /* load migrationKey */
    if (rc == 0) {
	rc = TPM_Pubkey_Load(&(tpm_migrationkeyauth->migrationKey), stream, stream_size);
    }
    /* load migrationScheme */
    if (rc == 0) {
	rc = TPM_Load16(&(tpm_migrationkeyauth->migrationScheme), stream, stream_size);
    }
    /* load digest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_migrationkeyauth->digest, stream, stream_size);
    }
    return rc;
}

/* TPM_Migrationkeyauth_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_Migrationkeyauth_Store(TPM_STORE_BUFFER *sbuffer,
				      TPM_MIGRATIONKEYAUTH *tpm_migrationkeyauth)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_Migrationkeyauth_Store:\n");
    /* store migrationKey */
    if (rc == 0) {
	rc = TPM_Pubkey_Store(sbuffer, &(tpm_migrationkeyauth->migrationKey));
    }
    /* store migrationScheme */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, tpm_migrationkeyauth->migrationScheme);
    }
    /* store digest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_migrationkeyauth->digest);
    }
    return rc;
}

/* TPM_Migrationkeyauth_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_Migrationkeyauth_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_Migrationkeyauth_Delete(TPM_MIGRATIONKEYAUTH *tpm_migrationkeyauth)
{
    printf(" TPM_Migrationkeyauth_Delete:\n");
    if (tpm_migrationkeyauth != NULL) {
	TPM_Pubkey_Delete(&(tpm_migrationkeyauth->migrationKey));
	TPM_Migrationkeyauth_Init(tpm_migrationkeyauth);
    }
    return;
}

/*
  TPM_MSA_COMPOSITE
*/

/* TPM_MsaComposite_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_MsaComposite_Init(TPM_MSA_COMPOSITE *tpm_msa_composite)
{
    printf(" TPM_MsaComposite_Init:\n");
    tpm_msa_composite->MSAlist = 0;
    tpm_msa_composite->migAuthDigest = NULL;
    return;
}

/* TPM_MsaComposite_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_MsaComposite_Init()
   After use, call TPM_MsaComposite_Delete() to free memory
*/

TPM_RESULT TPM_MsaComposite_Load(TPM_MSA_COMPOSITE *tpm_msa_composite,
				 unsigned char **stream,
				 uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;
    uint32_t		i;

    printf(" TPM_MsaComposite_Load:\n");
    /* load MSAlist */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_msa_composite->MSAlist), stream, stream_size);
    }
    /* MSAlist MUST be one (1) or greater. */
    if (rc == 0) {
	if (tpm_msa_composite->MSAlist == 0) {
	    printf("TPM_MsaComposite_Load: Error, MSAlist is zero\n");
	    rc = TPM_INVALID_STRUCTURE;
	}
    }
    /* FIXME add MSAlist limit */
    /* allocate memory for the migAuthDigest array */
    if (rc == 0) {
	rc = TPM_Malloc((unsigned char **)&(tpm_msa_composite->migAuthDigest),
			(tpm_msa_composite->MSAlist) * TPM_DIGEST_SIZE);
    }
    /* load migAuthDigest array */
    for (i = 0 ; (rc == 0) && (i < tpm_msa_composite->MSAlist) ; i++) {
	rc = TPM_Digest_Load(tpm_msa_composite->migAuthDigest[i], stream, stream_size);
    }
    return rc;
}

/* TPM_MsaComposite_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_MsaComposite_Store(TPM_STORE_BUFFER *sbuffer,
				  const TPM_MSA_COMPOSITE *tpm_msa_composite)
{
    TPM_RESULT		rc = 0;
    uint32_t		i;

    printf(" TPM_MsaComposite_Store:\n");
    /* store MSAlist */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_msa_composite->MSAlist);
    }
    /* store migAuthDigest array */
    for (i = 0 ; (rc == 0) && (i < tpm_msa_composite->MSAlist) ; i++) {
	rc = TPM_Digest_Store(sbuffer, tpm_msa_composite->migAuthDigest[i]);
    }
    return rc;
}

/* TPM_MsaComposite_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_MsaComposite_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_MsaComposite_Delete(TPM_MSA_COMPOSITE *tpm_msa_composite)
{
    printf(" TPM_MsaComposite_Delete:\n");
    if (tpm_msa_composite != NULL) {
	free(tpm_msa_composite->migAuthDigest);
	TPM_MsaComposite_Init(tpm_msa_composite);
    }
    return;
}

TPM_RESULT TPM_MsaComposite_CheckMigAuthDigest(TPM_DIGEST tpm_digest, /* value to check vs list */
					       TPM_MSA_COMPOSITE *tpm_msa_composite)
{
    TPM_RESULT		rc = 0;
    uint32_t		n;		/* count through msaList */
    TPM_BOOL		match;

    printf(" TPM_MsaComposite_CheckMigAuthDigest:\n");
    for (n = 0 , match = FALSE ; (n < tpm_msa_composite->MSAlist) && !match ; n++) {
	rc = TPM_Digest_Compare(tpm_digest, tpm_msa_composite->migAuthDigest[n]);
	if (rc == 0) {
	    match = TRUE;
	}
    }
    if (match) {
	rc = TPM_SUCCESS;
    }
    else {
	printf("TPM_MsaComposite_CheckMigAuthDigest: Error, no match to msaList\n");
	rc = TPM_MA_TICKET_SIGNATURE;
    }
    return rc;
}

/* TPM_MsaComposite_CheckSigTicket()

   i. Verify that for one of the n=1 to n=(msaList -> MSAlist) values of msaList ->
      migAuthDigest[n], sigTicket == HMAC (V1) using tpmProof as the secret where V1 is a
      TPM_CMK_SIGTICKET structure such that:

      (1) V1 -> verKeyDigest = msaList -> migAuthDigest[n]
      (2) V1 -> signedData = SHA1[restrictTicket]
*/

TPM_RESULT TPM_MsaComposite_CheckSigTicket(TPM_DIGEST sigTicket, /* expected HMAC */
					   TPM_SECRET tpmProof,	  /* HMAC key */
					   TPM_MSA_COMPOSITE *tpm_msa_composite,
					   TPM_CMK_SIGTICKET *tpm_cmk_sigticket)
{
    TPM_RESULT		rc = 0;
    uint32_t		n;		/* count through msaList */
    TPM_BOOL		match;
    TPM_STORE_BUFFER	sbuffer;
    const unsigned char *buffer;	
    uint32_t		length;
    
    printf(" TPM_MsaComposite_CheckSigTicket: TPM_MSA_COMPOSITE length %u\n",
	   tpm_msa_composite->MSAlist);
    TPM_Sbuffer_Init(&sbuffer);		/* freed @1 */
    for (n = 0 , match = FALSE ;
	 (rc == 0) && (n < tpm_msa_composite->MSAlist) && !match ; n++) {

	if (rc == 0) {
	    /* verKeyDigest = msaList -> migAuthDigest[n].  The rest of the structure is initialized
	       by the caller */
	    TPM_PrintFour("  TPM_MsaComposite_CheckSigTicket: Checking migAuthDigest: ",
			  tpm_msa_composite->migAuthDigest[n]);
	    TPM_Digest_Copy(tpm_cmk_sigticket->verKeyDigest, tpm_msa_composite->migAuthDigest[n]);
	    /* serialize the TPM_CMK_SIGTICKET structure */
	    TPM_Sbuffer_Clear(&sbuffer);	/* reset pointers without free */
	    rc = TPM_CmkSigticket_Store(&sbuffer, tpm_cmk_sigticket);
	    TPM_Sbuffer_Get(&sbuffer, &buffer, &length);
	}
	if (rc == 0) {
	    rc = TPM_HMAC_Check(&match,
				sigTicket,	/* expected */
				tpmProof,	/* HMAC key*/
				length, buffer, /* TPM_CMK_SIGTICKET */
				0, NULL);
	}
    }
    if (rc == 0) {
	    if (!match) {
	    printf("TPM_MsaComposite_CheckSigTicket: Error, no match to msaList\n");
	    rc = TPM_MA_TICKET_SIGNATURE;
	}
    }
    TPM_Sbuffer_Delete(&sbuffer);	/* @1 */
    return rc;
}

/*
  TPM_CMK_AUTH
*/

/* TPM_CmkAuth_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_CmkAuth_Init(TPM_CMK_AUTH *tpm_cmk_auth)
{
    printf(" TPM_CmkAuth_Init:\n");
    TPM_Digest_Init(tpm_cmk_auth->migrationAuthorityDigest);
    TPM_Digest_Init(tpm_cmk_auth->destinationKeyDigest);
    TPM_Digest_Init(tpm_cmk_auth->sourceKeyDigest);
    return;
}

/* TPM_CmkAuth_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_CmkAuth_Init()
   After use, call TPM_CmkAuth_Delete() to free memory
*/

TPM_RESULT TPM_CmkAuth_Load(TPM_CMK_AUTH *tpm_cmk_auth,
			    unsigned char **stream,
			    uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_CmkAuth_Load:\n");
    /* load migrationAuthorityDigest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_cmk_auth->migrationAuthorityDigest, stream, stream_size);
    }
    /* load destinationKeyDigest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_cmk_auth->destinationKeyDigest, stream, stream_size);
    }
    /* load sourceKeyDigest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_cmk_auth->sourceKeyDigest, stream, stream_size);
    }
    return rc;
}

/* TPM_CmkAuth_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_CmkAuth_Store(TPM_STORE_BUFFER *sbuffer,
			     const TPM_CMK_AUTH *tpm_cmk_auth)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_CmkAuth_Store:\n");
    /* store migrationAuthorityDigest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_cmk_auth->migrationAuthorityDigest);
    }
    /* store destinationKeyDigest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_cmk_auth->destinationKeyDigest);
    }
    /* store sourceKeyDigest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_cmk_auth->sourceKeyDigest);
    }
    return rc;
}

/* TPM_CmkAuth_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_CmkAuth_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_CmkAuth_Delete(TPM_CMK_AUTH *tpm_cmk_auth)
{
    printf(" TPM_CmkAuth_Delete:\n");
    if (tpm_cmk_auth != NULL) {
	TPM_CmkAuth_Init(tpm_cmk_auth);
    }
    return;
}

/*
  TPM_CMK_MIGAUTH
*/

/* TPM_CmkMigauth_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_CmkMigauth_Init(TPM_CMK_MIGAUTH *tpm_cmk_migauth)
{
    printf(" TPM_CmkMigauth_Init:\n");
    TPM_Digest_Init(tpm_cmk_migauth->msaDigest);
    TPM_Digest_Init(tpm_cmk_migauth->pubKeyDigest);
    return;
}

/* TPM_CmkMigauth_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_CmkMigauth_Init()
   After use, call TPM_CmkMigauth_Delete() to free memory
*/

TPM_RESULT TPM_CmkMigauth_Load(TPM_CMK_MIGAUTH *tpm_cmk_migauth,
			       unsigned char **stream,
			       uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_CmkMigauth_Load:\n");
    /* check tag */
    if (rc == 0) {	
	rc = TPM_CheckTag(TPM_TAG_CMK_MIGAUTH, stream, stream_size);
    }
    /* load msaDigest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_cmk_migauth->msaDigest , stream, stream_size);
    }
    /* load pubKeyDigest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_cmk_migauth->pubKeyDigest , stream, stream_size);
    }
    return rc;
}

/* TPM_CmkMigauth_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_CmkMigauth_Store(TPM_STORE_BUFFER *sbuffer,
				const TPM_CMK_MIGAUTH *tpm_cmk_migauth)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_CmkMigauth_Store:\n");
    /* store tag */
    if (rc == 0) {	
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_CMK_MIGAUTH); 
    }
    /* store msaDigest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_cmk_migauth->msaDigest);
    }
    /* store pubKeyDigest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_cmk_migauth->pubKeyDigest);
    }
    return rc;
}

/* TPM_CmkMigauth_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_CmkMigauth_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_CmkMigauth_Delete(TPM_CMK_MIGAUTH *tpm_cmk_migauth)
{
    printf(" TPM_CmkMigauth_Delete:\n");
    if (tpm_cmk_migauth != NULL) {
	TPM_CmkMigauth_Init(tpm_cmk_migauth);
    }
    return;
}

/* TPM_CmkMigauth_CheckHMAC() checks an HMAC of a TPM_CMK_MIGAUTH object.

   It serializes the structure and HMAC's the result.  The common function cannot be used because
   'tpm_hmac' is not part of the structure and cannot be NULL'ed.
*/

TPM_RESULT TPM_CmkMigauth_CheckHMAC(TPM_BOOL *valid,			/* result */
				    TPM_HMAC tpm_hmac,			/* expected */
				    TPM_SECRET tpm_hmac_key,		/* key */
				    TPM_CMK_MIGAUTH *tpm_cmk_migauth)	/* data */
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	sbuffer;	/* serialized TPM_CMK_MIGAUTH */

    printf(" TPM_CmkMigauth_CheckHMAC:\n");
    TPM_Sbuffer_Init(&sbuffer);				/* freed @1 */
    /* Serialize the TPM_CMK_MIGAUTH structure */
    if (rc == 0) {
	rc = TPM_CmkMigauth_Store(&sbuffer, tpm_cmk_migauth);
    }	 
    /* verify the HMAC of the serialized structure */
    if (rc == 0) {
	rc = TPM_HMAC_CheckSbuffer(valid,		/* result */
				   tpm_hmac,		/* expected */
				   tpm_hmac_key,	/* key */
				   &sbuffer);		/* data stream */
    }
    TPM_Sbuffer_Delete(&sbuffer);			/* @1 */
    return rc;
}

/*
  TPM_CMK_SIGTICKET
*/

/* TPM_CmkSigticket_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_CmkSigticket_Init(TPM_CMK_SIGTICKET *tpm_cmk_sigticket)
{
    printf(" TPM_CmkSigticket_Init:\n");
    TPM_Digest_Init(tpm_cmk_sigticket->verKeyDigest);
    TPM_Digest_Init(tpm_cmk_sigticket->signedData);
    return;
}

/* TPM_CmkSigticket_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_CmkSigticket_Init()
   After use, call TPM_CmkSigticket_Delete() to free memory
*/

TPM_RESULT TPM_CmkSigticket_Load(TPM_CMK_SIGTICKET *tpm_cmk_sigticket,
				 unsigned char **stream,
				 uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_CmkSigticket_Load:\n");
    /* check tag */
    if (rc == 0) {	
	rc = TPM_CheckTag(TPM_TAG_CMK_SIGTICKET, stream, stream_size);
    }
    /* load verKeyDigest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_cmk_sigticket->verKeyDigest , stream, stream_size);
    }
    /* load signedData */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_cmk_sigticket->signedData , stream, stream_size);
    }
    return rc;
}

/* TPM_CmkSigticket_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_CmkSigticket_Store(TPM_STORE_BUFFER *sbuffer,
				  const TPM_CMK_SIGTICKET *tpm_cmk_sigticket)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_CmkSigticket_Store:\n");
    /* store tag */
    if (rc == 0) {	
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_CMK_SIGTICKET); 
    }
    /* store verKeyDigest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_cmk_sigticket->verKeyDigest);
    }
    /* store signedData */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_cmk_sigticket->signedData);
    }
    return rc;
}

/* TPM_CmkSigticket_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_CmkSigticket_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_CmkSigticket_Delete(TPM_CMK_SIGTICKET *tpm_cmk_sigticket)
{
    printf(" TPM_CmkSigticket_Delete:\n");
    if (tpm_cmk_sigticket != NULL) {
	TPM_CmkSigticket_Init(tpm_cmk_sigticket);
    }
    return;
}

/*
  TPM_CMK_MA_APPROVAL
*/

/* TPM_CmkMaApproval_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_CmkMaApproval_Init(TPM_CMK_MA_APPROVAL *tpm_cmk_ma_approval)
{
    printf(" TPM_CmkMaApproval_Init:\n");
    TPM_Digest_Init(tpm_cmk_ma_approval->migrationAuthorityDigest);
    return;
}

/* TPM_CmkMaApproval_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_CmkMaApproval_Init()
   After use, call TPM_CmkMaApproval_Delete() to free memory
*/

TPM_RESULT TPM_CmkMaApproval_Load(TPM_CMK_MA_APPROVAL *tpm_cmk_ma_approval,
				  unsigned char **stream,
				  uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_CmkMaApproval_Load:\n");
    /* check tag */
    if (rc == 0) {	
	rc = TPM_CheckTag(TPM_TAG_CMK_MA_APPROVAL, stream, stream_size);
    }
    /* load migrationAuthorityDigest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_cmk_ma_approval->migrationAuthorityDigest, stream, stream_size);
    }
    return rc;
}

/* TPM_CmkMaApproval_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_CmkMaApproval_Store(TPM_STORE_BUFFER *sbuffer,
				   const TPM_CMK_MA_APPROVAL *tpm_cmk_ma_approval)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_CmkMaApproval_Store:\n");
    /* store tag */
    if (rc == 0) {	
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_CMK_MA_APPROVAL); 
    }
    /* store migrationAuthorityDigest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_cmk_ma_approval->migrationAuthorityDigest);
    }
    return rc;
}

/* TPM_CmkMaApproval_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_CmkMaApproval_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_CmkMaApproval_Delete(TPM_CMK_MA_APPROVAL *tpm_cmk_ma_approval)
{
    printf(" TPM_CmkMaApproval_Delete:\n");
    if (tpm_cmk_ma_approval != NULL) {
	TPM_CmkMaApproval_Init(tpm_cmk_ma_approval);
    }
    return;
}

/* TPM_CmkMaApproval_CheckHMAC() generates an HMAC of a TPM_CMK_MIGAUTH object

   It serializes the structure and HMAC's the result.The common function cannot be used because
   'tpm_hmac' is not part of the structure and cannot be NULL'ed.
*/

TPM_RESULT TPM_CmkMaApproval_CheckHMAC(TPM_BOOL *valid,			/* result */
				       TPM_HMAC tpm_hmac,		/* expected */
				       TPM_SECRET tpm_hmac_key,		/* key */
				       TPM_CMK_MA_APPROVAL *tpm_cmk_ma_approval) /* data */
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	sbuffer;	/* serialized TPM_CMK_MA_APPROVAL */

    printf(" TPM_CmkMaApproval_CheckHMAC:\n");
    TPM_Sbuffer_Init(&sbuffer);				/* freed @1 */
    /* Serialize the TPM_CMK_MA_APPROVAL structure */
    if (rc == 0) {
	rc = TPM_CmkMaApproval_Store(&sbuffer, tpm_cmk_ma_approval);
    }	 
    /* verify the HMAC of the serialized structure */
    if (rc == 0) {
	rc = TPM_HMAC_CheckSbuffer(valid,		/* result */
				   tpm_hmac,		/* expected */
				   tpm_hmac_key,	/* key */
				   &sbuffer);		/* data stream */
    }
    TPM_Sbuffer_Delete(&sbuffer);			/* @1 */
    return rc;
}

/*
  Processing functions
*/

/* TPM_CreateBlobCommon() does the steps common to TPM_CreateMigrationBlob and
   TPM_CMK_CreateBlob
   
   It takes a TPM_STORE_ASYMKEY, and
	- splits the TPM_STORE_PRIVKEY into k1 (20) and k2 (112)
	- builds a TPM_MIGRATE_ASYMKEY using
		'payload_type'
		TPM_STORE_ASYMKEY usageAuth, pubDataDigest
		k2 as partPrivKey
	- serializes the TPM_MIGRATE_ASYMKEY
	- OAEP encode using
		'phash'
		k1 as seed
*/

TPM_RESULT TPM_CreateBlobCommon(TPM_SIZED_BUFFER *outData,	/* The modified, encrypted
								   entity. */
				TPM_STORE_ASYMKEY *d1AsymKey,
				TPM_DIGEST pHash,		/* for OAEP padding */
				TPM_PAYLOAD_TYPE payload_type,
				TPM_SIZED_BUFFER *random,	/* String used for xor encryption */
				TPM_PUBKEY *migrationKey)	/* public key of the migration
								   facility */
{
    TPM_RESULT		rc = 0;
    uint32_t		o1_size;
    BYTE		*o1;
    BYTE		*r1;
    BYTE		*x1;

    printf("TPM_CreateBlobCommon:\n");
    o1 = NULL;		/* freed @1 */
    r1 = NULL;		/* freed @2 */
    x1 = NULL;		/* freed @3 */
    if (rc == 0) {
	TPM_StoreAsymkey_GetO1Size(&o1_size, d1AsymKey);
    }
    if (rc == 0) {
	rc = TPM_Malloc(&o1, o1_size);
    }
    if (rc == 0) {
	rc = TPM_Malloc(&r1, o1_size);
    }
    if (rc == 0) {
	rc = TPM_Malloc(&x1, o1_size);
    }
    if (rc == 0) {
	rc = TPM_StoreAsymkey_StoreO1(o1,
				      o1_size,
				      d1AsymKey,
				      pHash,
				      payload_type,
				      d1AsymKey->usageAuth);
    }
    /* NOTE Comments from TPM_CreateMigrationBlob rev 81 */
    /* d. Create r1 a random value from the TPM RNG. The size of r1 MUST be the size of o1. Return
       r1 in the Random parameter. */
    if (rc == 0) {
	rc = TPM_Random(r1, o1_size);
    }
    /* e. Create x1 by XOR of o1 with r1 */
    if (rc == 0) {
	TPM_PrintFourLimit("TPM_CreateBlobCommon: r1 -", r1, o1_size);
	TPM_XOR(x1, o1, r1, o1_size);
	TPM_PrintFourLimit("TPM_CreateBlobCommon: x1 -", x1, o1_size);
	/* f. Copy r1 into the output field "random".*/
	rc = TPM_SizedBuffer_Set(random, o1_size, r1);
    }
    /* g. Encrypt x1 with the migration public key included in migrationKeyAuth. */
    if (rc == 0) {
	rc = TPM_RSAPublicEncrypt_Pubkey(outData,
					 x1,
					 o1_size,
					 migrationKey);
	TPM_PrintFour("TPM_CreateBlobCommon: outData", outData->buffer);
    }
    free(o1);		/* @1 */
    free(r1);		/* @2 */
    free(x1);		/* @3 */
    return rc;
}

/* 11.1 TPM_CreateMigrationBlob rev 109

   The TPM_CreateMigrationBlob command implements the first step in the process of moving a
   migratable key to a new parent or platform. Execution of this command requires knowledge of the
   migrationAuth field of the key to be migrated.

   Migrate mode is generally used to migrate keys from one TPM to another for backup, upgrade or to
   clone a key on another platform. To do this, the TPM needs to create a data blob that another TPM
   can deal with.  This is done by loading in a backup public key that will be used by the TPM to
   create a new data blob for a migratable key.

   The TPM Owner does the selection and authorization of migration public keys at any time prior to
   the execution of TPM_CreateMigrationBlob by performing the TPM_AuthorizeMigrationKey command.

   IReWrap mode is used to directly move the key to a new parent (either on this platform or
   another). The TPM simply re-encrypts the key using a new parent, and outputs a normal encrypted
   element that can be subsequently used by a TPM_LoadKey command.

   TPM_CreateMigrationBlob implicitly cannot be used to migrate a non-migratory key. No explicit
   check is required. Only the TPM knows tpmProof. Therefore it is impossible for the caller to
   submit an authorization value equal to tpmProof and migrate a non-migratory key.
*/

TPM_RESULT TPM_Process_CreateMigrationBlob(tpm_state_t *tpm_state,
					   TPM_STORE_BUFFER *response,
					   TPM_TAG tag,
					   uint32_t paramSize,
					   TPM_COMMAND_CODE ordinal,
					   unsigned char *command,
					   TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;				/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_KEY_HANDLE parentHandle;	/* Handle of the parent key that can decrypt encData. */
    TPM_MIGRATE_SCHEME migrationType;	/* The migration type, either MIGRATE or REWRAP */
    TPM_MIGRATIONKEYAUTH migrationKeyAuth;	/* Migration public key and its authorization
						   digest. */
    TPM_SIZED_BUFFER encData;		/* The encrypted entity that is to be modified. */
    TPM_AUTHHANDLE parentAuthHandle;	/* The authorization handle used for the parent key. */
    TPM_NONCE nonceOdd;			/* Nonce generated by system associated with
					   parentAuthHandle */
    TPM_BOOL continueAuthSession;	/* Continue use flag for parent session */
    TPM_AUTHDATA parentAuth;		/* Authorization HMAC key: parentKey.usageAuth. */
    TPM_AUTHHANDLE entityAuthHandle;	/* The authorization handle used for the encrypted
					   entity. */
    TPM_NONCE entitynonceOdd;		/* Nonce generated by system associated with
					   entityAuthHandle */
    TPM_BOOL continueEntitySession = TRUE;	/* Continue use flag for entity session */
    TPM_AUTHDATA entityAuth;		/* Authorization HMAC key: entity.migrationAuth. */

    /* processing parameters */
    unsigned char *		inParamStart;	/* starting point of inParam's */
    unsigned char *		inParamEnd;	/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			parentAuthHandleValid = FALSE;
    TPM_BOOL			entityAuthHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*parent_auth_session_data = NULL;	/* session data for
									   parentAuthHandle */
    TPM_AUTH_SESSION_DATA	*entity_auth_session_data = NULL;	/* session data for
									   entityAuthHandle */
    TPM_SECRET			*hmacKey;
    TPM_SECRET			*entityHmacKey;
    TPM_KEY			*parentKey;
    TPM_BOOL			parentPCRStatus;
    TPM_SECRET			*parentUsageAuth;
    unsigned char		*d1Decrypt;	       	/* decryption of encData */
    uint32_t			d1DecryptLength = 0;   	/* actual valid data */
    unsigned char		*stream;		/* for deserializing decrypted encData */
    uint32_t			stream_size;
    TPM_STORE_ASYMKEY		d1AsymKey;		/* structure from decrypted encData */
    TPM_STORE_BUFFER		mka_sbuffer;		/* serialized migrationKeyAuth */
    const unsigned char		*mka_buffer;	
    uint32_t			mka_length;
    
    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_SIZED_BUFFER	random;		/* String used for xor encryption */
    TPM_SIZED_BUFFER	outData;	/* The modified, encrypted entity. */

    printf("TPM_Process_CreateMigrationBlob: Ordinal Entry\n");
    TPM_Migrationkeyauth_Init(&migrationKeyAuth);	/* freed @1 */
    TPM_SizedBuffer_Init(&encData);			/* freed @2 */
    TPM_SizedBuffer_Init(&random);			/* freed @3 */
    TPM_SizedBuffer_Init(&outData);			/* freed @4 */
    d1Decrypt = NULL;					/* freed @5 */
    TPM_StoreAsymkey_Init(&d1AsymKey);			/* freed @6 */
    TPM_Sbuffer_Init(&mka_sbuffer);			/* freed @7 */
    /*
      get inputs
    */
    /* get parentHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&parentHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get migrationType */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CreateMigrationBlob: parentHandle %08x\n", parentHandle); 
	returnCode = TPM_Load16(&migrationType, &command, &paramSize);
    }
    /* get migrationKeyAuth */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Migrationkeyauth_Load(&migrationKeyAuth, &command, &paramSize);
    }
    /* get encData */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&encData, &command, &paramSize);
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
	returnCode = TPM_AuthParams_Get(&parentAuthHandle,
					&parentAuthHandleValid,
					nonceOdd,
					&continueAuthSession,
					parentAuth,
					&command, &paramSize);
    }
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	printf("TPM_Process_CreateMigrationBlob: parentAuthHandle %08x\n", parentAuthHandle);
    }
    /* get the 'below the line' authorization parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthParams_Get(&entityAuthHandle,
					&entityAuthHandleValid,
					entitynonceOdd,
					&continueEntitySession,
					entityAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CreateMigrationBlob: entityAuthHandle %08x\n", entityAuthHandle); 
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_CreateMigrationBlob: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	parentAuthHandleValid = FALSE;
	entityAuthHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* The TPM does not check the PCR values when migrating values locked to a PCR. */
    /* get the key associated with parentHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&parentKey, &parentPCRStatus,
						 tpm_state, parentHandle,
						 FALSE,		/* read-only, do not check PCR's */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    /* get parentHandle -> usageAuth */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_Key_GetUsageAuth(&parentUsageAuth, parentKey);
    }	 
    /* get the session data */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_AuthSessions_GetData(&parent_auth_session_data,
					      &hmacKey,
					      tpm_state,
					      parentAuthHandle,
					      TPM_PID_NONE,
					      TPM_ET_KEYHANDLE,
					      ordinal,
					      parentKey,
					      parentUsageAuth,			/* OIAP */
					      parentKey->tpm_store_asymkey->pubDataDigest); /*OSAP*/
    }
    /* 1. Validate that parentAuth authorizes the use of the key pointed to by parentHandle. */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					parent_auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					parentAuth);		/* Authorization digest for input */
    }
    /* if there is no parent authorization, check that the parent authDataUsage is TPM_AUTH_NEVER */
    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_AUTH2_COMMAND)) {
	if (parentKey->authDataUsage != TPM_AUTH_NEVER) {
	    printf("TPM_Process_CreateMigrationBlob: Error, parent key authorization required\n");
	    returnCode = TPM_AUTHFAIL;
	}
    }
    /* 2. Validate that parentHandle -> keyUsage is TPM_KEY_STORAGE, if not return
       TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	if (parentKey->keyUsage != TPM_KEY_STORAGE) {
	    printf("TPM_Process_CreateMigrationBlob: Error, keyUsage %04hx is invalid\n",
		   parentKey->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 3. Create d1 a TPM_STORE_ASYMKEY structure by decrypting encData using the key pointed to by
       parentHandle. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CreateMigrationBlob: Decrypting encData\n");
	/* decrypt with the parent key to a stream */
	returnCode = TPM_RSAPrivateDecryptMalloc(&d1Decrypt,	       /* decrypted data */
						 &d1DecryptLength,     /* actual size of d1 data */
						 encData.buffer,/* encrypted data */
						 encData.size,	/* encrypted data size */
						 parentKey);
    }
    /* deserialize the stream to a TPM_STORE_ASYMKEY d1AsymKey */
    if (returnCode == TPM_SUCCESS) {
	stream = d1Decrypt;
	stream_size = d1DecryptLength;
	returnCode = TPM_StoreAsymkey_Load(&d1AsymKey, FALSE,
					   &stream, &stream_size,
					   NULL,	/* TPM_KEY_PARMS */
					   NULL);	/* TPM_SIZED_BUFFER pubKey */
    }	 
    /* a. Verify that d1 -> payload is TPM_PT_ASYM. */
    if (returnCode == TPM_SUCCESS) {
	if (d1AsymKey.payload != TPM_PT_ASYM) {
	    printf("TPM_Process_CreateMigrationBlob: Error, bad payload %02x\n",
		   d1AsymKey.payload);
	    returnCode = TPM_BAD_MIGRATION;
	}
    }
    /* 4. Validate that entityAuth authorizes the migration of d1. The validation MUST use d1 ->
       migrationAuth as the secret. */
    /* get the second session data */
    /* The second authorisation session (using entityAuth) MUST be OIAP because OSAP does not have a
       suitable entityType */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_GetData(&entity_auth_session_data,
					      &entityHmacKey,
					      tpm_state,
					      entityAuthHandle,
					      TPM_PID_OIAP,
					      TPM_ET_KEYHANDLE,
					      ordinal,
					      NULL,
					      &(d1AsymKey.migrationAuth),
					      NULL);
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Auth2data_Check(tpm_state,
					 *entityHmacKey,		/* HMAC key */
					 inParamDigest,
					 entity_auth_session_data,	/* authorization session */
					 entitynonceOdd,		/* Nonce generated by system
									   associated with authHandle */
					 continueEntitySession,
					 entityAuth);		/* Authorization digest for input */
    }
    /* 5.  Validate that migrationKeyAuth -> digest is the SHA-1 hash of (migrationKeyAuth ->
       migrationKey || migrationKeyAuth -> migrationScheme || TPM_PERMANENT_DATA -> tpmProof). */
    /* first serialize the TPM_PUBKEY migrationKeyAuth -> migrationKey */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CreateMigrationBlob: Verifying migrationKeyAuth\n");
	returnCode = TPM_Pubkey_Store(&mka_sbuffer, &(migrationKeyAuth.migrationKey));
    }
    if (returnCode == TPM_SUCCESS) {
	/* get the serialization result */
	TPM_Sbuffer_Get(&mka_sbuffer, &mka_buffer, &mka_length);
	/* compare to migrationKeyAuth -> digest */
	returnCode = TPM_SHA1_Check(migrationKeyAuth.digest,
				    mka_length, mka_buffer,	/* serialized migrationKey */
				    sizeof(TPM_MIGRATE_SCHEME), &(migrationKeyAuth.migrationScheme),
				    TPM_SECRET_SIZE, tpm_state->tpm_permanent_data.tpmProof,
				    0, NULL);
    }
    /* 6. If migrationType == TPM_MS_MIGRATE the TPM SHALL perform the following actions: */
    if ((returnCode == TPM_SUCCESS) && (migrationType == TPM_MS_MIGRATE)) {
	printf("TPM_Process_CreateMigrationBlob: migrationType TPM_MS_MIGRATE\n");
	/* a. Build two byte arrays, K1 and K2: */
	/* i. K1 = d1.privKey[0..19] (d1.privKey.keyLength + 16 bytes of d1.privKey.key), sizeof(K1)
	   = 20 */
	/* ii. K2 = d1.privKey[20..131] (position 16-127 of TPM_STORE_ASYMKEY. privKey.key)
	   (position 16-127 of d1 . privKey.key), sizeof(K2) = 112 */
	/* b. Build M1 a TPM_MIGRATE_ASYMKEY structure */
	/* i. TPM_MIGRATE_ASYMKEY.payload = TPM_PT_MIGRATE */
	/* ii. TPM_MIGRATE_ASYMKEY.usageAuth = d1.usageAuth */
	/* iii. TPM_MIGRATE_ASYMKEY.pubDataDigest = d1.pubDataDigest */
	/* iv. TPM_MIGRATE_ASYMKEY.partPrivKeyLen = 112 - 127. */
	/* v. TPM_MIGRATE_ASYMKEY.partPrivKey = K2 */
	/* c. Create o1 (which SHALL be 198 bytes for a 2048 bit RSA key) by performing the OAEP
	   encoding of m using OAEP parameters of */
	/* i. m = M1 the TPM_MIGRATE_ASYMKEY structure */
	/* ii. pHash = d1->migrationAuth */
	/* iii. seed = s1 = K1 */
	/* d. Create r1 a random value from the TPM RNG. The size of r1 MUST be the size of
	   o1. Return r1 in the Random parameter. */
	/* e. Create x1 by XOR of o1 with r1*/
	/* f. Copy r1 into the output field "random".*/
	/* g. Encrypt x1 with the migration public key included in migrationKeyAuth.*/
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_CreateBlobCommon(&outData,			/* output */
					      &d1AsymKey,		/* TPM_STORE_ASYMKEY */
					      d1AsymKey.migrationAuth,	/* pHash */
					      TPM_PT_MIGRATE,		/* payload type */
					      &random,		/* string for XOR encryption */
					      &(migrationKeyAuth.migrationKey)); /* TPM_PUBKEY */
	}
    }
    /* 7. If migrationType == TPM_MS_REWRAP the TPM SHALL perform the following actions: */
    else if ((returnCode == TPM_SUCCESS) && (migrationType == TPM_MS_REWRAP)) {
	printf("TPM_Process_CreateMigrationBlob: migrationType TPM_MS_REWRAP\n");
	/* a. Rewrap the key using the public key in migrationKeyAuth, keeping the existing contents
	   of that key. */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_RSAPublicEncrypt_Pubkey(&outData,
						     d1Decrypt,	/* decrypted encData parameter */
						     d1DecryptLength,
						     &(migrationKeyAuth.migrationKey));
	}
	/* b. Set randomSize to 0 in the output parameter array */
	/* NOTE Done by TPM_SizedBuffer_Init() */
    }
    /* 8. Else */
    /* a. Return TPM_BAD_PARAMETER */
    else if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CreateMigrationBlob: Error, illegal migrationType %04hx\n",
	       migrationType);
	returnCode = TPM_BAD_PARAMETER;
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_CreateMigrationBlob: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return random */
	    returnCode = TPM_SizedBuffer_Store(response, &random);
	}
	if (returnCode == TPM_SUCCESS) {
	    /* return outData */
	    returnCode = TPM_SizedBuffer_Store(response, &outData);
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
					    *hmacKey,		/* HMAC key */
					    parent_auth_session_data,
					    outParamDigest,
					    nonceOdd,
					    continueAuthSession);
	}
	/* calculate and set the below the line parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_AuthParams_Set(response,
					    *entityHmacKey,	/* HMAC key */
					    entity_auth_session_data,
					    outParamDigest,
					    entitynonceOdd,
					    continueEntitySession);
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
	 !continueAuthSession)
	&& parentAuthHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions,
					 parentAuthHandle);
    }
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueEntitySession) &&
	entityAuthHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions,
					 entityAuthHandle);
    }
    /*
      cleanup
    */
    TPM_Migrationkeyauth_Delete(&migrationKeyAuth);	/* @1 */
    TPM_SizedBuffer_Delete(&encData);			/* @2 */
    TPM_SizedBuffer_Delete(&random);			/* @3 */
    TPM_SizedBuffer_Delete(&outData);			/* @4 */
    free(d1Decrypt);					/* @5 */
    TPM_StoreAsymkey_Delete(&d1AsymKey);		/* @6 */
    TPM_Sbuffer_Delete(&mka_sbuffer);			/* @7 */
    return rcf;
}



/* 11.2 TPM_ConvertMigrationBlob rev 87

   This command takes a migration blob and creates a normal wrapped blob. The migrated blob must be
   loaded into the TPM using the normal TPM_LoadKey function.

   Note that the command migrates private keys, only. The migration of the associated public keys is
   not specified by TPM because they are not security sensitive. Migration of the associated public
   keys may be specified in a platform specific specification. A TPM_KEY structure must be recreated
   before the migrated key can be used by the target TPM in a LoadKey command.
*/

/* The relationship between Create and Convert parameters are:

   Create:	k1 || k2 = privKey
		m = TPM_MIGRATE_ASYMKEY, partPrivKey = k2
		o1 = OAEP (m), seed = k1
		x1 = o1 ^ r1
		out = pub (x1)
   Convert:
		d1 = priv (in)
		o1 = d1 ^ r1
		m1, seed = OAEP (o1)
		k1 = seed || partPrivKey
*/

TPM_RESULT TPM_Process_ConvertMigrationBlob(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE parentHandle;	/* Handle of a loaded key that can decrypt keys. */
    TPM_SIZED_BUFFER inData;		/* The XOR'd and encrypted key */
    TPM_SIZED_BUFFER random;		/* Random value used to hide key data. */
    TPM_AUTHHANDLE authHandle;		/* The authorization handle used for keyHandle. */
    TPM_NONCE nonceOdd;			/* Nonce generated by system associated with authHandle */
    TPM_BOOL continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   handle */
    TPM_AUTHDATA parentAuth;		/* The authorization digest that authorizes the inputs and
					   the migration of the key in parentHandle. HMAC key:
					   parentKey.usageAuth */

    /* processing parameters */
    unsigned char *		inParamStart;	/* starting point of inParam's */
    unsigned char *		inParamEnd;	/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_KEY			*parentKey = NULL;	/* the key specified by parentHandle */
    TPM_BOOL			parentPCRStatus;
    TPM_SECRET			*parentUsageAuth;
    unsigned char		*d1Decrypt;
    uint32_t			d1DecryptLength = 0;		/* actual valid data */
    BYTE			*o1Oaep;
    TPM_STORE_ASYMKEY		d2AsymKey;
    TPM_STORE_BUFFER		d2_sbuffer;

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_SIZED_BUFFER	outData;	/* The encrypted private key that can be loaded with
					   TPM_LoadKey */

    printf("TPM_Process_ConvertMigrationBlob: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&inData);		/* freed @1 */
    TPM_SizedBuffer_Init(&random);		/* freed @2 */
    TPM_SizedBuffer_Init(&outData);		/* freed @3 */
    d1Decrypt = NULL;				/* freed @4 */
    o1Oaep = NULL;				/* freed @5 */
    TPM_StoreAsymkey_Init(&d2AsymKey);		/* freed @6 */
    TPM_Sbuffer_Init(&d2_sbuffer);		/* freed @7 */
    /*
      get inputs
    */
    /* get parentHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&parentHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get inData */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_ConvertMigrationBlob: parentHandle %08x\n", parentHandle);
	returnCode = TPM_SizedBuffer_Load(&inData, &command, &paramSize);
    }
    /* get random */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&random, &command, &paramSize);
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
	returnCode = TPM_CheckRequestTag10(tag);
    }
    /* get the optional 'below the line' authorization parameters */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					parentAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_ConvertMigrationBlob: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* Verify that parentHandle points to a valid key.	Get the TPM_KEY associated with parentHandle
     */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&parentKey, &parentPCRStatus,
						 tpm_state, parentHandle,
						 FALSE,		/* not r/o, using to decrypt */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    /* check TPM_AUTH_DATA_USAGE authDataUsage */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND)) {
	if (parentKey->authDataUsage != TPM_AUTH_NEVER) {
	    printf("TPM_Process_ConvertMigrationBlob: Error, parent key authorization required\n");
	    returnCode = TPM_AUTHFAIL;
	}
    }
    /* get parentHandle -> usageAuth */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_Key_GetUsageAuth(&parentUsageAuth, parentKey);
    }	 
    /* get the session data */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_NONE,
					      TPM_ET_KEYHANDLE,
					      ordinal,
					      parentKey,
					      parentUsageAuth,			/* OIAP */
					      parentKey->tpm_store_asymkey->pubDataDigest); /*OSAP*/
    }
    /* 1. Validate the authorization to use the key in parentHandle */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					parentAuth);		/* Authorization digest for input */
    }
    /* 2. If the keyUsage field of the key referenced by parentHandle does not have the value
       TPM_KEY_STORAGE, the TPM must return the error code TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	if (parentKey->keyUsage != TPM_KEY_STORAGE) {
	    printf("TPM_Process_ConvertMigrationBlob: Error, "
		   "parentHandle -> keyUsage should be TPM_KEY_STORAGE, is %04x\n",
		   parentKey->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 3. Create d1 by decrypting the inData area using the key in parentHandle */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_ConvertMigrationBlob: Decrypting inData\n");
	TPM_PrintFourLimit("TPM_Process_ConvertMigrationBlob: inData", inData.buffer, inData.size);
	returnCode = TPM_RSAPrivateDecryptMalloc(&d1Decrypt,		/* decrypted data */
						 &d1DecryptLength,	/* actual size of d1 data */
						 inData.buffer,		/* encrypted data */
						 inData.size,
						 parentKey);
    }
    /* the random input parameter must be the same length as the decrypted data */
    if (returnCode == TPM_SUCCESS) {
	if (d1DecryptLength != random.size) {
	    printf("TPM_Process_ConvertMigrationBlob: Error "
		   "decrypt data length %u random size %u\n",
		   d1DecryptLength, random.size);
	    returnCode = TPM_BAD_PARAMETER;
	}
    }
    /* allocate memory for o1 */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Malloc(&o1Oaep, d1DecryptLength);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_ConvertMigrationBlob: d1 length %u\n", d1DecryptLength);
	TPM_PrintFourLimit("TPM_Process_ConvertMigrationBlob: d1 -", d1Decrypt, d1DecryptLength);
	/* 4. Create o1 by XOR d1 and random parameter */
	TPM_XOR(o1Oaep, d1Decrypt, random.buffer, d1DecryptLength);
	/* 5. Create m1 a TPM_MIGRATE_ASYMKEY structure, seed and pHash by OAEP decoding o1 */
	/* NOTE TPM_StoreAsymkey_LoadO1() extracts TPM_STORE_ASYMKEY from the OAEP encoded
	   TPM_MIGRATE_ASYMKEY. */
	returnCode = TPM_StoreAsymkey_LoadO1(&d2AsymKey, o1Oaep, d1DecryptLength);
    }
    /* 6. Create k1 by combining seed and the TPM_MIGRATE_ASYMKEY -> partPrivKey field */
    /* NOTE Done by TPM_StoreAsymkey_LoadO1 () */
    /* 7. Create d2 a TPM_STORE_ASYMKEY structure */
    if (returnCode == TPM_SUCCESS) {
	/* a. Verify that m1 -> payload == TPM_PT_MIGRATE */
	/* NOTE TPM_StoreAsymkey_LoadO1() copied TPM_MIGRATE_ASYMKEY -> payload to TPM_STORE_ASYMKEY
	   -> payload */
	if (d2AsymKey.payload != TPM_PT_MIGRATE) {
	    printf("TPM_Process_ConvertMigrationBlob: Error, invalid payload %02x\n",
		   d2AsymKey.payload);
	    returnCode = TPM_BAD_MIGRATION;
	}
    }
    if (returnCode == TPM_SUCCESS) {
	/* b. Set d2 -> payload = TPM_PT_ASYM */
	d2AsymKey.payload = TPM_PT_ASYM;  
	/* c. Set d2 -> usageAuth to m1 -> usageAuth */
	/* d. Set d2 -> migrationAuth to pHash */
	/* e. Set d2 -> pubDataDigest to m1 -> pubDataDigest */
	/* f. Set d2 -> privKey field to k1 */
	/* NOTE Done by TPM_StoreAsymkey_LoadO1() */
	/* 9. Create outData using the key in parentHandle to perform the encryption */
	/* serialize d2key  to d2 */
	returnCode = TPM_StoreAsymkey_Store(&d2_sbuffer, FALSE, &d2AsymKey);
    }
    /* encrypt d2 with parentKey */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_RSAPublicEncryptSbuffer_Key(&outData, &d2_sbuffer, parentKey);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_ConvertMigrationBlob: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return the outData */
	    returnCode = TPM_SizedBuffer_Store(response, &outData);
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
	if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	    returnCode = TPM_AuthParams_Set(response,
					    *hmacKey,		/* HMAC key */
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
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&inData);		/* @1 */
    TPM_SizedBuffer_Delete(&random);		/* @2 */
    TPM_SizedBuffer_Delete(&outData);		/* @3 */
    free(d1Decrypt);				/* @4 */
    free(o1Oaep);				/* @5 */
    TPM_StoreAsymkey_Delete(&d2AsymKey);	/* @6 */
    TPM_Sbuffer_Delete(&d2_sbuffer);		/* @7 */
    return rcf;
}

/* 11.3 TPM_AuthorizeMigrationKey rev 114

   This command creates an authorization blob, to allow the TPM owner to specify which migration
   facility they will use and allow users to migrate information without further involvement with
   the TPM owner.

   It is the responsibility of the TPM Owner to determine whether migrationKey is appropriate for
   migration. The TPM checks just the cryptographic strength of migrationKey.
*/

TPM_RESULT TPM_Process_AuthorizeMigrationKey(tpm_state_t *tpm_state,
					     TPM_STORE_BUFFER *response,
					     TPM_TAG tag,
					     uint32_t paramSize,
					     TPM_COMMAND_CODE ordinal,
					     unsigned char *command,
					     TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;				/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_MIGRATE_SCHEME migrationScheme; /* Type of migration operation that is to be permitted for
					   this key. */
    TPM_PUBKEY migrationKey;		/* The public key to be authorized. */
    TPM_AUTHHANDLE authHandle;		/* The authorization handle used for owner authorization. */
    TPM_NONCE nonceOdd;			/* Nonce generated by system associated with authHandle */
    TPM_BOOL continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   handle */
    TPM_AUTHDATA ownerAuth;		/* The authorization digest for inputs and owner
					   authorization. HMAC key: ownerAuth. */

    /* processing parameters */
    unsigned char *		inParamStart;	/* starting point of inParam's */
    unsigned char *		inParamEnd;	/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_RSA_KEY_PARMS		*rsa_key_parms;			/* for migrationKey */
    TPM_STORE_BUFFER		sbuffer;
    const unsigned char		*buffer;
    uint32_t			length;
    
    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_MIGRATIONKEYAUTH outData;	/* (f1) Returned public key and authorization digest. */

    printf("TPM_Process_AuthorizeMigrationKey: Ordinal Entry\n");
    TPM_Pubkey_Init(&migrationKey);		/* freed @1 */
    TPM_Migrationkeyauth_Init(&outData);	/* freed @2 */
    TPM_Sbuffer_Init(&sbuffer);			/* freed @3 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get migrationScheme */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load16(&migrationScheme, &command, &paramSize);
    }
    /* get migrationKey */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Pubkey_Load(&migrationKey, &command, &paramSize);
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
	returnCode = TPM_CheckRequestTag1(tag);
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
	if (paramSize != 0) {
	    printf("TPM_Process_AuthorizeMigrationKey: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* 1. Check that the cryptographic strength of migrationKey is at least that of a 2048 bit RSA
       key. If migrationKey is an RSA key, this means that migrationKey MUST be 2048 bits or greater
       and MUST use the default exponent. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyParms_GetRSAKeyParms(&rsa_key_parms,
						 &(migrationKey.algorithmParms));
    }
    if (returnCode == TPM_SUCCESS) {
	if (rsa_key_parms->keyLength < 2048) {
	    printf("TPM_Process_AuthorizeMigrationKey: Error, "
		   "migrationKey length %u less than 2048\n",
		   rsa_key_parms->keyLength);
	    returnCode = TPM_BAD_KEY_PROPERTY;
	}
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyParams_CheckDefaultExponent(&(rsa_key_parms->exponent));
    }
    /* 2. Validate the AuthData to use the TPM by the TPM Owner */
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
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* owner HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					ownerAuth);		/* Authorization digest for input */
    }
    /* 3. Create a f1 a TPM_MIGRATIONKEYAUTH  structure */
    /* NOTE: This is outData */
    /* 4. Verify that migrationKey-> algorithmParms -> encScheme is TPM_ES_RSAESOAEP_SHA1_MGF1, and
       return the error code TPM_INAPPROPRIATE_ENC if it is not */
    if (returnCode == TPM_SUCCESS) {
	if (migrationKey.algorithmParms.encScheme != TPM_ES_RSAESOAEP_SHA1_MGF1) {
	    printf("TPM_Process_AuthorizeMigrationKey: Error, "
		   "migrationKey encScheme %04hx must be TPM_ES_RSAESOAEP_SHA1_MGF1\n",
		   migrationKey.algorithmParms.encScheme);
	    returnCode = TPM_INAPPROPRIATE_ENC;
	}
    }
    /* 5. Set f1 -> migrationKey to the input migrationKey */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Pubkey_Copy(&(outData.migrationKey), &(migrationKey));
    }
    if (returnCode == TPM_SUCCESS) {
	/* 6. Set f1 -> migrationScheme to the input migrationScheme */
	outData.migrationScheme = migrationScheme;
	/* 7. Create v1 by concatenating (migrationKey || migrationScheme || TPM_PERMANENT_DATA ->
	   tpmProof) */
	/* 8. Create h1 by performing a SHA-1 hash of v1 */
	/* first serialize the TPM_PUBKEY migrationKey */
	returnCode = TPM_Pubkey_Store(&sbuffer, &migrationKey);
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_Sbuffer_Get(&sbuffer, &buffer, &length);
	/* 9. Set f1 -> digest to h1 */
	returnCode = TPM_SHA1(outData.digest,
			      length, buffer,		/* serialized migrationKey */
			      sizeof(TPM_MIGRATE_SCHEME), &(migrationScheme),
			      TPM_SECRET_SIZE, tpm_state->tpm_permanent_data.tpmProof,
			      0, NULL);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_AuthorizeMigrationKey: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* 10. Return f1 as outData */
	    returnCode = TPM_Migrationkeyauth_Store(response, &outData);
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
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_AuthParams_Set(response,
					    *hmacKey,		/* owner HMAC key */
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
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    TPM_Pubkey_Delete(&migrationKey);		/* @1 */
    TPM_Migrationkeyauth_Delete(&outData);	/* @2 */
    TPM_Sbuffer_Delete(&sbuffer);		/* @3 */
    return rcf;
}

/* 11.4 TPM_MigrateKey rev 87

   The TPM_MigrateKey command performs the function of a migration authority.

   The command is relatively simple; it just decrypts the input packet (coming from
   TPM_CreateMigrationBlob or TPM_CMK_CreateBlob) and then re-encrypts it with the input public
   key. The output of this command would then be sent to TPM_ConvertMigrationBlob or
   TPM_CMK_ConvertMigration on the target TPM.
   
   TPM_MigrateKey does not make ANY assumptions about the contents of the encrypted blob. Since it
   does not have the XOR string, it cannot actually determine much about the key that is being
   migrated.
  
   This command exists to permit the TPM to be a migration authority. If used in this way, it is
   expected that the physical security of the system containing the TPM and the AuthData value for
   the MA key would be tightly controlled.

   To prevent the execution of this command using any other key as a parent key, this command works
   only if keyUsage for maKeyHandle is TPM_KEY_MIGRATE.
*/

TPM_RESULT TPM_Process_MigrateKey(tpm_state_t *tpm_state,
				  TPM_STORE_BUFFER *response,
				  TPM_TAG tag,
				  uint32_t paramSize,
				  TPM_COMMAND_CODE ordinal,
				  unsigned char *command,
				  TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;				/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_KEY_HANDLE maKeyHandle;		/* Handle of the key to be used to migrate the key. */
    TPM_PUBKEY pubKey;			/* Public key to which the blob is to be migrated */
    TPM_SIZED_BUFFER inData;		/* The input blob */

    TPM_AUTHHANDLE maAuthHandle;	/* The authorization session handle used for maKeyHandle. */
    TPM_NONCE nonceOdd;		/* Nonce generated by system associated with certAuthHandle */
    TPM_BOOL continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   session handle */
    TPM_AUTHDATA keyAuth;	/* The authorization session digest for the inputs and key to be
				   signed. HMAC key: maKeyHandle.usageAuth. */

    /* processing parameters */
    unsigned char *		inParamStart;	/* starting point of inParam's */
    unsigned char *		inParamEnd;	/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			maAuthHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_KEY			*maKey = NULL;		/* the key specified by maKeyHandle */
    TPM_RSA_KEY_PARMS		*tpm_rsa_key_parms;	/* for maKey */
    TPM_SECRET			*maKeyUsageAuth;
    TPM_BOOL			maPCRStatus;
    uint32_t			decrypt_data_size;	/* resulting decrypted data size */
    BYTE			*decrypt_data = NULL;	/* The resulting decrypted data. */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_SIZED_BUFFER	outData;	/* The re-encrypted blob */

    printf("TPM_Process_MigrateKey: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&inData);	/* freed @1 */
    TPM_SizedBuffer_Init(&outData);	/* freed @2 */
    TPM_Pubkey_Init(&pubKey);		/* freed @4 */
    /*
      get inputs
    */
    /* get maKeyHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&maKeyHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get pubKey */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_MigrateKey: maKeyHandle %08x\n", maKeyHandle); 
	returnCode = TPM_Pubkey_Load(&pubKey, &command, &paramSize);
    }
    /* get encData */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&inData, &command, &paramSize);
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
	returnCode = TPM_CheckRequestTag10(tag);
    }
    /* get the optional 'below the line' authorization parameters */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_AuthParams_Get(&maAuthHandle,
					&maAuthHandleValid,
					nonceOdd,
					&continueAuthSession,
					keyAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_MigrateKey: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	maAuthHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* 1. Validate that keyAuth authorizes the use of the key pointed to by maKeyHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&maKey, &maPCRStatus, tpm_state, maKeyHandle,
						 FALSE,		/* not read-only */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    /* get maKeyHandle -> usageAuth */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_Key_GetUsageAuth(&maKeyUsageAuth, maKey);
    }	 
    /* get the session data */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	printf("TPM_Process_MigrateKey: maAuthHandle %08x\n", maAuthHandle); 
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      maAuthHandle,
					      TPM_PID_NONE,
					      TPM_ET_KEYHANDLE,
					      ordinal,
					      maKey,
					      maKeyUsageAuth,		/* OIAP */
					      maKey->tpm_store_asymkey->pubDataDigest); /* OSAP */
    }
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					keyAuth);		/* Authorization digest for input */
    }
    /* check TPM_AUTH_DATA_USAGE authDataUsage */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND)) {
	if (maKey->authDataUsage != TPM_AUTH_NEVER) {
	    printf("TPM_Process_MigrateKey: Error, authorization required\n");
	    returnCode = TPM_AUTHFAIL;
	}
    }
    /* 2. The TPM validates that the key pointed to by maKeyHandle has a key usage value of
       TPM_KEY_MIGRATE, and that the allowed encryption scheme is TPM_ES_RSAESOAEP_SHA1_MGF1. */
    if (returnCode == TPM_SUCCESS) {
	if (maKey->keyUsage != TPM_KEY_MIGRATE) {
	    printf("TPM_Process_MigrateKey: Error, keyUsage %04hx not TPM_KEY_MIGRATE\n",
		   maKey->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
	else if (maKey->algorithmParms.encScheme != TPM_ES_RSAESOAEP_SHA1_MGF1) {
	    printf("TPM_Process_MigrateKey: Error, encScheme %04hx not TPM_ES_RSAESOAEP_SHA_MGF1\n",
		   maKey->algorithmParms.encScheme);
	    returnCode = TPM_BAD_KEY_PROPERTY;
	}
    }
    /* 3. The TPM validates that pubKey is of a size supported by the TPM and that its size is
       consistent with the input blob and maKeyHandle. */
    /* NOTE: Let the encryption step do this step */
    /* 4. The TPM decrypts inData and re-encrypts it using pubKey. */
    /* get the TPM_RSA_KEY_PARMS associated with maKey */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyParms_GetRSAKeyParms(&tpm_rsa_key_parms, &(maKey->algorithmParms));
    }	     
    /* decrypt using maKey */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_MigrateKey: Decrypt using maKey\n");
	returnCode =
	    TPM_RSAPrivateDecryptMalloc(&decrypt_data,		/* decrypted data, freed @3 */
					&decrypt_data_size,	/* actual size of decrypt data */
					inData.buffer,
					inData.size,
					maKey);
	
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_MigrateKey: Encrypt using pubKey\n");
	returnCode = TPM_RSAPublicEncrypt_Pubkey(&outData,	/* encrypted data */
						 decrypt_data,
						 decrypt_data_size,
						 &pubKey);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_MigrateKey: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return outData */
	    returnCode = TPM_SizedBuffer_Store(response, &outData);
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
	if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	    returnCode = TPM_AuthParams_Set(response,
					    *hmacKey,		/* HMAC key */
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
	 !continueAuthSession) &&
	maAuthHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, maAuthHandle);
    }
    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&inData);	/* @1 */
    TPM_SizedBuffer_Delete(&outData);	/* @2 */
    free(decrypt_data);			/* @3 */
    TPM_Pubkey_Delete(&pubKey);		/* @4 */
    return rcf;
}
     
/* 11.7 TPM_CMK_CreateKey rev 114

   The TPM_CMK_CreateKey command both generates and creates a secure storage bundle for asymmetric
   keys whose migration is controlled by a migration authority.

   TPM_CMK_CreateKey is very similar to TPM_CreateWrapKey, but: (1) the resultant key must be a
   migratable key and can be migrated only by TPM_CMK_CreateBlob; (2) the command is Owner
   authorized via a ticket.

   TPM_CMK_CreateKey creates an otherwise normal migratable key except that (1) migrationAuth is an
   HMAC of the migration authority and the new key's public key, signed by tpmProof (instead of
   being tpmProof); (2) the migrationAuthority bit is set TRUE; (3) the payload type is
   TPM_PT_MIGRATE_RESTRICTED.
     
   The migration-selection/migration authority is specified by passing in a public key (actually the
   digests of one or more public keys, so more than one migration authority can be specified).
*/

TPM_RESULT TPM_Process_CMK_CreateKey(tpm_state_t *tpm_state,
				     TPM_STORE_BUFFER *response,
				     TPM_TAG tag,
				     uint32_t paramSize,
				     TPM_COMMAND_CODE ordinal,
				     unsigned char *command,
				     TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;				/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_KEY_HANDLE parentHandle;	/* Handle of a loaded key that can perform key wrapping. */
    TPM_ENCAUTH dataUsageAuth;		/* Encrypted usage authorization data for the key. */
    TPM_KEY keyInfo;			/* Information about key to be created, pubkey.keyLength and
					   keyInfo.encData elements are 0. MUST be TPM_KEY12 */
    TPM_HMAC migrationAuthorityApproval;/* A ticket, created by the TPM Owner using
					   TPM_CMK_ApproveMA, approving a TPM_MSA_COMPOSITE
					   structure */
    TPM_DIGEST migrationAuthorityDigest;/* The digest of a TPM_MSA_COMPOSITE structure */

    TPM_AUTHHANDLE authHandle;		/* The authorization handle used for parent key
					   authorization. Must be an OSAP session. */
    TPM_NONCE nonceOdd;			/* Nonce generated by system associated with authHandle */
    TPM_BOOL continueAuthSession = TRUE;	/* Ignored */
    TPM_AUTHDATA pubAuth;		/* The authorization session digest that authorizes the use
					   of the public key in parentHandle. HMAC key:
					   parentKey.usageAuth.*/

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for entityAuthHandle
								 */
    TPM_SECRET			*hmacKey;
    TPM_KEY			*parentKey = NULL;	/* the key specified by parentHandle */
    TPM_BOOL			parentPCRStatus;
    TPM_BOOL			hmacValid;			/* for migrationAuthorityApproval */
    TPM_SECRET			du1DecryptAuth;
    TPM_STORE_ASYMKEY		*wrappedStoreAsymkey;		/* substructure of wrappedKey */
    TPM_CMK_MA_APPROVAL		m1CmkMaApproval;
    TPM_CMK_MIGAUTH		m2CmkMigauth;
    int				ver;

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_KEY		wrappedKey;	/* The TPM_KEY structure which includes the public and
					   encrypted private key. MUST be TPM_KEY12 */

    printf("TPM_Process_CMK_CreateKey: Ordinal Entry\n");
    TPM_Key_Init(&keyInfo);			/* freed @1 */
    TPM_Key_Init(&wrappedKey);			/* freed @2 */
    TPM_CmkMaApproval_Init(&m1CmkMaApproval);	/* freed @3 */
    TPM_CmkMigauth_Init(&m2CmkMigauth);		/* freed @4 */
    /*
      get inputs
    */
    /* get parentHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&parentHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get dataUsageAuth */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CMK_CreateKey: parentHandle %08x\n", parentHandle);
	returnCode = TPM_Authdata_Load(dataUsageAuth, &command, &paramSize);
    }
    /* get keyInfo */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_Load(&keyInfo, &command, &paramSize);
    }
    /* get migrationAuthorityApproval */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Digest_Load(migrationAuthorityApproval, &command, &paramSize);
    }
    /* get migrationAuthorityDigest */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Digest_Load(migrationAuthorityDigest, &command, &paramSize);
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
	returnCode = TPM_CheckRequestTag1(tag);
    }
    /* get the 'below the line' authorization parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					pubAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CMK_CreateKey: authHandle %08x\n", authHandle); 
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_CMK_CreateKey: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* 1. Validate the authorization to use the key pointed to by parentHandle. Return TPM_AUTHFAIL
       on any error. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&parentKey, &parentPCRStatus,
						 tpm_state, parentHandle,
						 FALSE,		/* not r/o, using to encrypt */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    /* get the session data */
    /* 2. Validate the session type for parentHandle is OSAP. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_OSAP,
					      TPM_ET_KEYHANDLE,
					      ordinal,
					      parentKey,
					      NULL,			/* OIAP */
					      parentKey->tpm_store_asymkey->pubDataDigest); /*OSAP*/
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					pubAuth);		/* Authorization digest for input */
    }
    /* 3. If the TPM is not designed to create a key of the type requested in keyInfo, return the
       error code TPM_BAD_KEY_PROPERTY */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_CheckProperties(&ver, &keyInfo, 0,
					     tpm_state->tpm_permanent_flags.FIPS);
	printf("TPM_Process_CMK_CreateKey: key parameters v = %d\n", ver);
    }
    /* 4. Verify that parentHandle->keyUsage equals TPM_KEY_STORAGE */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CMK_CreateKey: Checking parent key\n");
	if (parentKey->keyUsage != TPM_KEY_STORAGE) {
	    printf("TPM_Process_CMK_CreateKey: Error, parent keyUsage not TPM_KEY_STORAGE\n");
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }	 
    /* 5. Verify that parentHandle-> keyFlags-> migratable == FALSE */
    if (returnCode == TPM_SUCCESS) {
	if (parentKey->keyFlags & TPM_MIGRATABLE) {
	    printf("TPM_Process_CMK_CreateKey: Error, parent migratable\n");
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 6. If keyInfo -> keyFlags -> migratable is FALSE then return TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CMK_CreateKey: Checking key flags\n");
	if (!(keyInfo.keyFlags & TPM_MIGRATABLE)) {
	    printf("TPM_Process_CMK_CreateKey: Error, keyInfo migratable is FALSE\n");
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 7. If keyInfo -> keyFlags -> migrateAuthority is FALSE , return TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	if (!(keyInfo.keyFlags & TPM_MIGRATEAUTHORITY)) {
	    printf("TPM_Process_CMK_CreateKey: Error, keyInfo migrateauthority is FALSE\n");
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 8. Verify that the migration authority is authorized */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CMK_CreateKey: Checking migration authority authorization\n");
	/* a. Create M1 a TPM_CMK_MA_APPROVAL structure */
	/* NOTE Done by TPM_CmkMaApproval_Init() */
	/* i. Set M1 ->migrationAuthorityDigest to migrationAuthorityDigest */
	TPM_Digest_Copy(m1CmkMaApproval.migrationAuthorityDigest, migrationAuthorityDigest);
	/* b. Verify that migrationAuthorityApproval == HMAC(M1) using tpmProof as the secret and
	   return error TPM_MA_AUTHORITY on mismatch */
	returnCode =
	    TPM_CmkMaApproval_CheckHMAC(&hmacValid,
					migrationAuthorityApproval,		/* expect */
					tpm_state->tpm_permanent_data.tpmProof, /* HMAC key */
					&m1CmkMaApproval);
	if (!hmacValid) {
	    printf("TPM_Process_CMK_CreateKey: Error, Invalid migrationAuthorityApproval\n");
	    returnCode = TPM_MA_AUTHORITY;
	}
    }
    /* 9. Validate key parameters */
    /* a. keyInfo -> keyUsage MUST NOT be TPM_KEY_IDENTITY or TPM_KEY_AUTHCHANGE. If it is, return
       TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CMK_CreateKey: Checking key usage\n");
	if ((keyInfo.keyUsage == TPM_KEY_IDENTITY) ||
	    (keyInfo.keyUsage == TPM_KEY_AUTHCHANGE)) {
	    printf("TPM_Process_CMK_CreateKey: Error, invalid keyInfo -> keyUsage %04hx\n",
		   keyInfo.keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 10. If TPM_PERMANENT_FLAGS -> FIPS is TRUE then */
    /* a. If keyInfo -> keySize is less than 1024 return TPM_NOTFIPS */
    /* b. If keyInfo -> authDataUsage specifies TPM_AUTH_NEVER return TPM_NOTFIPS */
    /* c. If keyInfo -> keyUsage specifies TPM_KEY_LEGACY return TPM_NOTFIPS */
    /* NOTE Done by TPM_Key_CheckProperties() */
    /* 11. If keyInfo -> keyUsage equals TPM_KEY_STORAGE or TPM_KEY_MIGRATE */
    /* a. algorithmID MUST be TPM_ALG_RSA */
    /* b. encScheme MUST be TPM_ES_RSAESOAEP_SHA1_MGF1 */
    /* c. sigScheme MUST be TPM_SS_NONE */
    /* d. key size MUST be 2048 */
    /* e. exponentSize MUST be 0 */
    /* NOTE Done by TPM_Key_CheckProperties() */
    /* 12. If keyInfo -> tag is NOT TPM_TAG_KEY12 return TPM_INVALID_STRUCTURE */
    if (returnCode == TPM_SUCCESS) {
	if (ver != 2) {
	    printf("TPM_Process_CMK_CreateKey: Error, keyInfo must be TPM_TAG_KEY12\n");
	    returnCode = TPM_INVALID_STRUCTURE;
	}
    }
    /* 13. Map wrappedKey to a TPM_KEY12 structure */
    /* NOTE: Not required.  TPM_KEY functions handle TPM_KEY12 subclass */
    /* 14. Create DU1 by decrypting dataUsageAuth according to the ADIP indicated by authHandle. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessionData_Decrypt(du1DecryptAuth,
						 NULL,
						 dataUsageAuth,
						 auth_session_data,
						 NULL,
						 NULL,
						 FALSE);	/* even and odd */
    }
    if (returnCode == TPM_SUCCESS) {
	/* 15. Set continueAuthSession to FALSE */
	continueAuthSession = FALSE;
	/* 16. Generate asymmetric key according to algorithm information in keyInfo */
	/* 17. Fill in the wrappedKey structure with information from the newly generated key.	*/
	printf("TPM_Process_CMK_CreateKey: Generating key\n");
	returnCode = TPM_Key_GenerateRSA(&wrappedKey,
					 tpm_state,
					 parentKey,
					 tpm_state->tpm_stclear_data.PCRS,	/* PCR array */
					 ver,				/* TPM_KEY12 */
					 keyInfo.keyUsage,
					 keyInfo.keyFlags,
					 keyInfo.authDataUsage,		/* TPM_AUTH_DATA_USAGE */
					 &(keyInfo.algorithmParms),	/* TPM_KEY_PARMS */
					 keyInfo.tpm_pcr_info,		/* TPM_PCR_INFO */
					 keyInfo.tpm_pcr_info_long);	/* TPM_PCR_INFO_LONG */
    }	 
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_GetStoreAsymkey(&wrappedStoreAsymkey,
					     &wrappedKey);
    }	 
    if (returnCode == TPM_SUCCESS) {
	/* a. Set wrappedKey -> encData -> usageAuth to DU1 */
	TPM_Secret_Copy(wrappedStoreAsymkey->usageAuth, du1DecryptAuth);
	/* b. Set wrappedKey -> encData -> payload to TPM_PT_MIGRATE_RESTRICTED */
	wrappedStoreAsymkey->payload = TPM_PT_MIGRATE_RESTRICTED;
	/* c. Create thisPubKey, a TPM_PUBKEY structure containing wrappedKey's public key. */
	/* NOTE All that is really needed is its digest, which is calculated directly */
    }
    if (returnCode == TPM_SUCCESS) {
	/* d. Create M2 a TPM_CMK_MIGAUTH structure */
	/* NOTE Done by TPM_CmkMigauth_Init() */
	/* i. Set M2 -> msaDigest to migrationAuthorityDigest */
	TPM_Digest_Copy(m2CmkMigauth.msaDigest, migrationAuthorityDigest);
	/* ii. Set M2 -> pubKeyDigest to SHA-1 (thisPubKey) */
	returnCode = TPM_Key_GeneratePubkeyDigest(m2CmkMigauth.pubKeyDigest, &wrappedKey);
	/* e. Set wrappedKey -> encData -> migrationAuth equal to HMAC(M2), using tpmProof as the
	   shared secret */
	returnCode = TPM_HMAC_GenerateStructure
		     (wrappedStoreAsymkey->migrationAuth,	/* HMAC */
		      tpm_state->tpm_permanent_data.tpmProof,	/* HMAC key */
		      &m2CmkMigauth,				/* structure */
		      (TPM_STORE_FUNCTION_T)TPM_CmkMigauth_Store);	/* store function */
    }
    /* 18. If keyInfo->PCRInfoSize is non-zero */
    /* a. Set wrappedKey -> pcrInfo to a TPM_PCR_INFO_LONG structure */
    /* b. Set wrappedKey -> pcrInfo to keyInfo -> pcrInfo  */
    /* b. Set wrappedKey -> digestAtCreation to the TPM_COMPOSITE_HASH indicated by
       creationPCRSelection */
    /* c. Set wrappedKey -> localityAtCreation to TPM_STANY_FLAGS -> localityModifier */
    /* NOTE This is done during TPM_Key_GenerateRSA() */
    /* 19. Encrypt the private portions of the wrappedKey structure using the key in parentHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_GenerateEncData(&wrappedKey, parentKey);
    }	 
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_CMK_CreateKey: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* 20. Return the newly generated key in the wrappedKey parameter */
	    returnCode = TPM_Key_Store(response, &wrappedKey);
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
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_AuthParams_Set(response,
					    *hmacKey,	/* HMAC key */
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
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    TPM_Key_Delete(&keyInfo);			/* @1 */
    TPM_Key_Delete(&wrappedKey);		/* @2 */
    TPM_CmkMaApproval_Delete(&m1CmkMaApproval); /* @3 */
    TPM_CmkMigauth_Delete(&m2CmkMigauth);	/* @4 */
    return rcf;
}


/* 11.5 TPM_CMK_CreateTicket rev 101

   The TPM_verifySignature command uses a public key to verify the signature over a digest.

   TPM_verifySignature returns a ticket that can be used to prove to the same TPM that signature
   verification with a particular public key was successful.
*/

TPM_RESULT TPM_Process_CMK_CreateTicket(tpm_state_t *tpm_state,
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
    TPM_PUBKEY verificationKey;		/* The public key to be used to check signatureValue */
    TPM_DIGEST signedData;		/* The data to be verified */
    TPM_SIZED_BUFFER signatureValue;	/* The signatureValue to be verified */
    TPM_AUTHHANDLE authHandle;		/* The authorization handle used for owner authorization. */
    TPM_NONCE nonceOdd;			/* Nonce generated by system associated with authHandle */
    TPM_BOOL continueAuthSession = TRUE;	/* Ignored */
    TPM_AUTHDATA pubAuth;		/* The authorization digest for inputs and owner. HMAC key:
					   ownerAuth. */

    /* processing parameters */
    unsigned char *		inParamStart;	/* starting point of inParam's */
    unsigned char *		inParamEnd;	/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_CMK_SIGTICKET		m2CmkSigticket;
    
    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_HMAC		sigTicket;	/* Ticket that proves digest created on this TPM */

    printf("TPM_Process_CMK_CreateTicket: Ordinal Entry\n");
    TPM_Pubkey_Init(&verificationKey);		/* freed @1 */
    TPM_SizedBuffer_Init(&signatureValue);	/* freed @2 */
    TPM_CmkSigticket_Init(&m2CmkSigticket);	/* freed @3 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get verificationKey */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Pubkey_Load(&verificationKey, &command, &paramSize);
    }
    /* get signedData */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Digest_Load(signedData, &command, &paramSize);
    }
    /* get signatureValue */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&signatureValue, &command, &paramSize);
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
	returnCode = TPM_CheckRequestTag1(tag);
    }
    /* get the 'below the line' authorization parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					pubAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_CMK_CreateTicket: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* 1. Validate the TPM Owner authorization to use the command */
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
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* owner HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					pubAuth);		/* Authorization digest for input */
    }
    /* 2. Validate that the key type and algorithm are correct */
    /* a. Validate that verificationKey -> algorithmParms -> algorithmID == TPM_ALG_RSA */
    if (returnCode == TPM_SUCCESS) {
	if (verificationKey.algorithmParms.algorithmID != TPM_ALG_RSA) {
	    printf("TPM_Process_CMK_CreateTicket: Error, incorrect algorithmID %08x\n",
		   verificationKey.algorithmParms.algorithmID);
	    returnCode = TPM_BAD_KEY_PROPERTY;
	}
    }
    /* b. Validate that verificationKey -> algorithmParms ->encScheme == TPM_ES_NONE */
    if (returnCode == TPM_SUCCESS) {
	if (verificationKey.algorithmParms.encScheme != TPM_ES_NONE) {
	    printf("TPM_Process_CMK_CreateTicket: Error, incorrect encScheme %04hx\n",
		   verificationKey.algorithmParms.encScheme);
	    returnCode = TPM_INAPPROPRIATE_ENC;
	}
    }
    /* c. Validate that verificationKey ->algorithmParms ->sigScheme is
       TPM_SS_RSASSAPKCS1v15_SHA1 or TPM_SS_RSASSAPKCS1v15_INFO */
    if (returnCode == TPM_SUCCESS) {
	if ((verificationKey.algorithmParms.sigScheme != TPM_SS_RSASSAPKCS1v15_SHA1) &&
	    (verificationKey.algorithmParms.sigScheme != TPM_SS_RSASSAPKCS1v15_INFO)) {
	    printf("TPM_Process_CMK_CreateTicket: Error, incorrect sigScheme %04hx\n",
		   verificationKey.algorithmParms.sigScheme);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 3. Use verificationKey to verify that signatureValue is a valid signature on signedData, and
       return error TPM_BAD_SIGNATURE on mismatch */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CMK_CreateTicket: Verifying signature\n");
	returnCode = TPM_RSAVerifyH(&signatureValue,		/* signature */
				    signedData,			/* data that was signed */
				    TPM_DIGEST_SIZE,		/* size of signed data */
				    &verificationKey);		/* TPM_PUBKEY public key */
	if (returnCode != TPM_SUCCESS) {
	    printf("TPM_Process_CMK_CreateTicket: Error verifying signature\n");
	}
    }
    /* 4. Create M2 a TPM_CMK_SIGTICKET */
    /* NOTE Done by TPM_CmkSigticket_Init() */
    /* a. Set M2 -> verKeyDigest to the SHA-1 (verificationKey) */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SHA1_GenerateStructure(m2CmkSigticket.verKeyDigest, &verificationKey,
						(TPM_STORE_FUNCTION_T)TPM_Pubkey_Store);
    }
    if (returnCode == TPM_SUCCESS) {
	/* b. Set M2 -> signedData to signedData */
	TPM_Digest_Copy(m2CmkSigticket.signedData, signedData);
	/* 5. Set sigTicket = HMAC(M2) signed by using tpmProof as the secret */
	returnCode = TPM_HMAC_GenerateStructure
		     (sigTicket,				/* HMAC */
		      tpm_state->tpm_permanent_data.tpmProof,	/* HMAC key */
		      &m2CmkSigticket,				/* structure */
		      (TPM_STORE_FUNCTION_T)TPM_CmkSigticket_Store);	/* store function */
    }
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_CMK_CreateTicket: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return sigTicket */
	    returnCode = TPM_Digest_Store(response, sigTicket);
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
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_AuthParams_Set(response,
					    *hmacKey,		/* owner HMAC key */
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
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    TPM_Pubkey_Delete(&verificationKey);	/* @1 */
    TPM_SizedBuffer_Delete(&signatureValue);	/* @2 */
    TPM_CmkSigticket_Delete(&m2CmkSigticket);	/* @3 */
    return rcf;
}


/* 11.9 TPM_CMK_CreateBlob rev 114

   TPM_CMK_CreateBlob command is very similar to TPM_CreateMigrationBlob, except that it: (1) uses
   an extra ticket (restrictedKeyAuth) instead of a migrationAuth authorization session; (2) uses
   the migration options TPM_MS_RESTRICT_MIGRATE or TPM_MS_RESTRICT_APPROVE; (3) produces a wrapped
   key blob whose migrationAuth is independent of tpmProof.

   If the destination (parent) public key is the MA, migration is implicitly permitted. Further
   checks are required if the MA is not the destination (parent) public key, and merely selects a
   migration destination: (1) sigTicket must prove that restrictTicket was signed by the MA; (2)
   restrictTicket must vouch that the target public key is approved for migration to the destination
   (parent) public key. (Obviously, this more complex method may also be used by an MA to approve
   migration to that MA.) In both cases, the MA must be one of the MAs implicitly listed in the
   migrationAuth of the target key-to-be-migrated.
   
   When the migrationType is TPM_MS_RESTRICT_MIGRATE, restrictTicket and sigTicket are unused.	The
   TPM may test that the corresponding sizes are zero, so the caller should set them to zero for
   interoperability.
*/

TPM_RESULT TPM_Process_CMK_CreateBlob(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE parentHandle;	/* Handle of the parent key that can decrypt encData. */
    TPM_MIGRATE_SCHEME migrationType;	/* The migration type, either TPM_MS_RESTRICT_MIGRATE or
					   TPM_MS_RESTRICT_APPROVE
					   NOTE Never used */
    TPM_MIGRATIONKEYAUTH migrationKeyAuth;	/* Migration public key and its authorization
						   session digest. */
    TPM_DIGEST pubSourceKeyDigest;	/* The digest of the TPM_PUBKEY of the entity to be migrated
					   */
    TPM_SIZED_BUFFER msaListBuffer;	/* One or more digests of public keys belonging to migration
					   authorities */
    TPM_SIZED_BUFFER restrictTicketBuffer;	/* If migrationType is TPM_MS_RESTRICT_APPROVE, a
						   TPM_CMK_AUTH structure, containing the digests of
						   the public keys belonging to the Migration
						   Authority, the destination parent key and the
						   key-to-be-migrated. */
    TPM_SIZED_BUFFER sigTicketBuffer;	/* If migrationType is TPM_MS_RESTRICT_APPROVE, a TPM_HMAC
					   structure, generated by the TPM, signaling a valid
					   signature over restrictTicket */
    TPM_SIZED_BUFFER encData;		/* The encrypted entity that is to be modified. */
    TPM_AUTHHANDLE parentAuthHandle;	/* The authorization handle used for the parent key. */
    TPM_NONCE nonceOdd;			/* Nonce generated by system associated with
					   parentAuthHandle */
    TPM_BOOL continueAuthSession;	/* Continue use flag for parent session */
    TPM_AUTHDATA parentAuth;		/* The authorization digest for inputs and
					   parentHandle. HMAC key: parentKey.usageAuth. */

    /* processing parameters */
    unsigned char *		inParamStart;	/* starting point of inParam's */
    unsigned char *		inParamEnd;	/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_KEY			*parentKey;
    TPM_BOOL			parentPCRStatus;
    TPM_SECRET			*parentUsageAuth;
    unsigned char		*d1Decrypt;		/* decryption of encData */
    uint32_t			d1DecryptLength = 0;	/* actual valid data */
    TPM_STORE_ASYMKEY		d1AsymKey;	/* structure from decrypted encData */
    unsigned char		*stream;	/* for deserializing structures */
    uint32_t			stream_size;
    TPM_STORE_BUFFER		mka_sbuffer;	/* serialized migrationKeyAuth.migrationKey */
    const unsigned char		*mka_buffer;	
    uint32_t			mka_length;
    TPM_DIGEST			migrationKeyDigest;  /* digest of migrationKeyAuth.migrationKey */
    TPM_DIGEST			pHash;
    TPM_CMK_MIGAUTH		m2CmkMigauth;
    TPM_BOOL			valid;
    TPM_MSA_COMPOSITE		msaList;
    TPM_DIGEST			sigTicket;
    TPM_CMK_AUTH		restrictTicket;
    TPM_CMK_SIGTICKET		v1CmkSigticket;

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_SIZED_BUFFER	random;		/* String used for xor encryption */
    TPM_SIZED_BUFFER	outData;	/* The modified, encrypted entity. */

    printf("TPM_Process_CMK_CreateBlob: Ordinal Entry\n");
    d1Decrypt = NULL;					/* freed @1 */
    TPM_Migrationkeyauth_Init(&migrationKeyAuth);	/* freed @2 */
    TPM_SizedBuffer_Init(&msaListBuffer);		/* freed @3 */
    TPM_SizedBuffer_Init(&restrictTicketBuffer);	/* freed @4 */
    TPM_SizedBuffer_Init(&sigTicketBuffer);		/* freed @5 */
    TPM_SizedBuffer_Init(&encData);			/* freed @6 */
    TPM_SizedBuffer_Init(&random);			/* freed @7 */
    TPM_SizedBuffer_Init(&outData);			/* freed @8 */
    TPM_Sbuffer_Init(&mka_sbuffer);			/* freed @9 */
    TPM_StoreAsymkey_Init(&d1AsymKey);			/* freed @10 */
    TPM_MsaComposite_Init(&msaList);			/* freed @11 */
    TPM_CmkAuth_Init(&restrictTicket);			/* freed @12 */
    TPM_CmkMigauth_Init(&m2CmkMigauth);			/* freed @13 */
    TPM_CmkSigticket_Init(&v1CmkSigticket);		/* freed @14 */
    /*
      get inputs
    */
    /* get parentHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&parentHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get migrationType */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load16(&migrationType, &command, &paramSize);
    }
    /* get migrationKeyAuth */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Migrationkeyauth_Load(&migrationKeyAuth, &command, &paramSize);
    }
    /* get pubSourceKeyDigest */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Digest_Load(pubSourceKeyDigest, &command, &paramSize);
    }
    /* get msaListBuffer */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&msaListBuffer, &command, &paramSize);
    }
    /* deserialize to msaList */
    if (returnCode == TPM_SUCCESS) {
	stream = msaListBuffer.buffer;
	stream_size = msaListBuffer.size;
	returnCode = TPM_MsaComposite_Load(&msaList, &stream, &stream_size);
    }
    /* get restrictTicket */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&restrictTicketBuffer, &command, &paramSize);
    }
    /* get sigTicket */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&sigTicketBuffer, &command, &paramSize);
    }
    /* get encData */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&encData, &command, &paramSize);
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
	returnCode = TPM_CheckRequestTag1(tag);
    }
    /* get the 'below the line' authorization parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthParams_Get(&parentAuthHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					parentAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_CMK_CreateBlob: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    /*
      The TPM does not check the PCR values when migrating values locked to a PCR. */
    /* get the key associated with parentHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&parentKey, &parentPCRStatus,
						 tpm_state, parentHandle,
						 FALSE,		/* do not check PCR's */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    /* get parentHandle -> usageAuth */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_GetUsageAuth(&parentUsageAuth, parentKey);
    }	 
    /* get the session data */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      parentAuthHandle,
					      TPM_PID_NONE,
					      TPM_ET_KEYHANDLE,
					      ordinal,
					      parentKey,
					      parentUsageAuth,			/* OIAP */
					      parentKey->tpm_store_asymkey->pubDataDigest); /*OSAP*/
    }
    /* 1. Validate that parentAuth authorizes the use of the key pointed to by parentHandle. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,	
					parentAuth);		/* Authorization digest for input */
   }
    /* 2.The TPM MAY verify that migrationType == migrationKeyAuth -> migrationScheme and return
	 TPM_BAD_MODE on error.
       a.The TPM MAY ignore migrationType. */
    /* 3. Verify that parentHandle-> keyFlags-> migratable == FALSE */
    if (returnCode == TPM_SUCCESS) {
	if (parentKey->keyFlags & TPM_MIGRATABLE) {
	    printf("TPM_Process_CMK_CreateBlob: Error, parent migratable\n");
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* Validate that parentHandle -> keyUsage is TPM_KEY_STORAGE, if not return the error code
       TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	if (parentKey->keyUsage != TPM_KEY_STORAGE) {
	    printf("TPM_Process_CMK_CreateBlob: Error, keyUsage %04hx is invalid\n",
		   parentKey->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 4. Create d1 by decrypting encData using the key pointed to by parentHandle. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CMK_CreateBlob: Decrypting encData\n");
	/* decrypt with the parent key to a stream */
	returnCode = TPM_RSAPrivateDecryptMalloc(&d1Decrypt,	/* decrypted data, freed @1 */
						 &d1DecryptLength,	/* actual size of d1 data */
						 encData.buffer,/* encrypted data */
						 encData.size,	/* encrypted data size */
						 parentKey);
    }
    /* deserialize the stream to a TPM_STORE_ASYMKEY d1AsymKey */
    if (returnCode == TPM_SUCCESS) {
	stream = d1Decrypt;
	stream_size = d1DecryptLength;
	returnCode = TPM_StoreAsymkey_Load(&d1AsymKey, FALSE,
					   &stream, &stream_size,
					   NULL,	/* TPM_KEY_PARMS */
					   NULL);	/* TPM_SIZED_BUFFER pubKey */
    }	 
    /* 5. Verify that the digest within migrationKeyAuth is legal for this TPM and public key */
    /* NOTE Presumably, this reverses the steps from TPM_AuthorizeMigrationKey */
    /* create h1 by concatenating (migrationKey || migrationScheme || TPM_PERMANENT_DATA ->
       tpmProof) */
    /* first serialize the TPM_PUBKEY migrationKeyAuth -> migrationKey */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CMK_CreateBlob: Verifying migrationKeyAuth\n");
	returnCode = TPM_Pubkey_Store(&mka_sbuffer, &(migrationKeyAuth.migrationKey));
    }
    if (returnCode == TPM_SUCCESS) {
	/* get the serialization result */
	TPM_Sbuffer_Get(&mka_sbuffer, &mka_buffer, &mka_length);
	/* then create the hash.  tpmProof indicates that the input knew ownerAuth in
	   TPM_AuthorizeMigrationKey */
	/* compare to migrationKeyAuth -> digest */	
	returnCode = TPM_SHA1_Check(migrationKeyAuth.digest,
				    mka_length, mka_buffer,	/* serialized migrationKey */
				    sizeof(TPM_MIGRATE_SCHEME), &(migrationKeyAuth.migrationScheme),
				    TPM_SECRET_SIZE, tpm_state->tpm_permanent_data.tpmProof,
				    0, NULL);
    }	
    /* 6. Verify that d1 -> payload == TPM_PT_MIGRATE_RESTRICTED or TPM_PT_MIGRATE_EXTERNAL */
    if (returnCode == TPM_SUCCESS) {
	if ((d1AsymKey.payload != TPM_PT_MIGRATE_RESTRICTED) &&
	    (d1AsymKey.payload != TPM_PT_MIGRATE_EXTERNAL)) {
	    printf("TPM_Process_CMK_CreateBlob: Error, invalid payload %02x\n", d1AsymKey.payload);
	    returnCode = TPM_INVALID_STRUCTURE;
	}
    }
    /* 7. Verify that the migration authorities in msaList are authorized to migrate this key */
    /* a. Create M2 a TPM_CMK_MIGAUTH structure */
    /* NOTE Done by TPM_CmkMigauth_Init() */
    /* i. Set M2 -> msaDigest to SHA-1[msaList] */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SHA1_GenerateStructure(m2CmkMigauth.msaDigest, &msaList,
						(TPM_STORE_FUNCTION_T)TPM_MsaComposite_Store);
    }
    if (returnCode == TPM_SUCCESS) {
	/* ii. Set M2 -> pubKeyDigest to pubSourceKeyDigest */
	TPM_Digest_Copy(m2CmkMigauth.pubKeyDigest, pubSourceKeyDigest);
	/* b. Verify that d1 -> migrationAuth == HMAC(M2) using tpmProof as the secret and return
	   error TPM_MA_AUTHORITY on mismatch */
	returnCode = TPM_CmkMigauth_CheckHMAC(&valid,
					      d1AsymKey.migrationAuth,		/* expected */
					      tpm_state->tpm_permanent_data.tpmProof, /* HMAC key*/
					      &m2CmkMigauth);
	if (!valid) {
	    printf("TPM_Process_CMK_CreateBlob: Error validating migrationAuth\n");
	    returnCode = TPM_MA_AUTHORITY;
	}	    
    }
    /* SHA-1[migrationKeyAuth -> migrationKey] is required below */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SHA1(migrationKeyDigest,
			      mka_length, mka_buffer,	/* serialized migrationKey */
			      0, NULL);
    }
    /* 8. If migrationKeyAuth -> migrationScheme == TPM_MS_RESTRICT_MIGRATE */
    if ((returnCode == TPM_SUCCESS) &&
	(migrationKeyAuth.migrationScheme == TPM_MS_RESTRICT_MIGRATE)) {
	/* a. Verify that intended migration destination is an MA: */
	if (returnCode == TPM_SUCCESS) {
	    printf("TPM_Process_CMK_CreateBlob: migrationScheme is TPM_MS_RESTRICT_MIGRATE\n");
	    /* i. For one of n=1 to n=(msaList -> MSAlist), verify that SHA-1[migrationKeyAuth ->
	       migrationKey] == msaList -> migAuthDigest[n] */
	    returnCode = TPM_MsaComposite_CheckMigAuthDigest(migrationKeyDigest, &msaList);
	}
	/* b. Validate that the MA key is the correct type */
	/* i. Validate that migrationKeyAuth -> migrationKey -> algorithmParms -> algorithmID ==
	   TPM_ALG_RSA */
	if (returnCode == TPM_SUCCESS) {
	    if (migrationKeyAuth.migrationKey.algorithmParms.algorithmID != TPM_ALG_RSA) {
		printf("TPM_Process_CMK_CreateBlob: Error, algorithmID %08x not TPM_ALG_RSA\n",
		       migrationKeyAuth.migrationKey.algorithmParms.algorithmID);
		returnCode = TPM_BAD_KEY_PROPERTY;
	    }
	}
	/* ii. Validate that migrationKeyAuth -> migrationKey -> algorithmParms -> encScheme is an
	   encryption scheme supported by the TPM */
	if (returnCode == TPM_SUCCESS) {
	    if (migrationKeyAuth.migrationKey.algorithmParms.encScheme !=
		TPM_ES_RSAESOAEP_SHA1_MGF1) {

		printf("TPM_Process_CMK_CreateBlob: Error, "
		       "encScheme %04hx not TPM_ES_RSAESOAEP_SHA1_MGF1\n",
		       migrationKeyAuth.migrationKey.algorithmParms.encScheme );
		returnCode = TPM_INAPPROPRIATE_ENC;
	    }
	}
	/* iii. Validate that migrationKeyAuth -> migrationKey ->algorithmParms -> sigScheme is
	   TPM_SS_NONE */
	if (returnCode == TPM_SUCCESS) {
	    if (migrationKeyAuth.migrationKey.algorithmParms.sigScheme != TPM_SS_NONE) {
		printf("TPM_Process_CMK_CreateBlob: Error, sigScheme %04hx not TPM_SS_NONE\n",
		       migrationKeyAuth.migrationKey.algorithmParms.sigScheme);
		returnCode = TPM_INVALID_KEYUSAGE;
	    }
	}
	/* c. The TPM MAY validate that restrictTicketSize is zero. */
	if (returnCode == TPM_SUCCESS) {
	    if (restrictTicketBuffer.size != 0) {
		printf("TPM_Process_CMK_CreateBlob: Error, "
		       "TPM_MS_RESTRICT_MIGRATE and restrictTicketSize %u not zero\n",
		       restrictTicketBuffer.size);
		returnCode = TPM_BAD_PARAMETER;
	    }
	}
	/* d. The TPM MAY validate that sigTicketSize is zero. */
	if (returnCode == TPM_SUCCESS) {
	    if (sigTicketBuffer.size != 0) {
		printf("TPM_Process_CMK_CreateBlob: Error, "
		       "TPM_MS_RESTRICT_MIGRATE and sigTicketSize %u not zero\n",
		       sigTicketBuffer.size);
		returnCode = TPM_BAD_PARAMETER;
	    }
	}
    }
    /* 9. If migrationKeyAuth -> migrationScheme == TPM_MS_RESTRICT_APPROVE */
    else if ((returnCode == TPM_SUCCESS) &&
	     (migrationKeyAuth.migrationScheme == TPM_MS_RESTRICT_APPROVE)) {
	/* a. Verify that the intended migration destination has been approved by the MSA: */
	/* i. Verify that for one of the n=1 to n=(msaList -> MSAlist) values of msaList ->
	   migAuthDigest[n], sigTicket == HMAC (V1) using tpmProof as the secret where V1 is a
	   TPM_CMK_SIGTICKET structure such that: */
	/* (1) V1 -> verKeyDigest = msaList -> migAuthDigest[n] */
	/* (2) V1 -> signedData = SHA-1[restrictTicket] */
	printf("TPM_Process_CMK_CreateBlob: migrationScheme is TPM_MS_RESTRICT_APPROVE_DOUBLE\n");
	/* deserialize the sigTicket TPM_HMAC */
	if (returnCode == TPM_SUCCESS) {
	    stream = sigTicketBuffer.buffer;
	    stream_size = sigTicketBuffer.size;
	    returnCode = TPM_Digest_Load(sigTicket, &stream, &stream_size);
	}
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_SHA1(v1CmkSigticket.signedData,
				  restrictTicketBuffer.size, restrictTicketBuffer.buffer,
				  0, NULL);
	}
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_MsaComposite_CheckSigTicket(sigTicket,
							 tpm_state->tpm_permanent_data.tpmProof,
							 &msaList,
							 &v1CmkSigticket);
	}
	/* ii. If [restrictTicket -> destinationKeyDigest] != SHA-1[migrationKeyAuth ->
	   migrationKey], return error TPM_MA_DESTINATION */
	/* deserialize the restrictTicket structure */
	if (returnCode == TPM_SUCCESS) {
	    stream = restrictTicketBuffer.buffer;
	    stream_size = restrictTicketBuffer.size;
	    returnCode = TPM_CmkAuth_Load(&restrictTicket, &stream, &stream_size);
	}
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Digest_Compare(migrationKeyDigest,
					    restrictTicket.destinationKeyDigest);
	    if (returnCode != TPM_SUCCESS) {
		printf("TPM_Process_CMK_CreateBlob: Error, no match to destinationKeyDigest\n");
		returnCode = TPM_MA_DESTINATION;
	    }
	}
	/* iii. If [restrictTicket -> sourceKeyDigest] != pubSourceKeyDigest, return error
	   TPM_MA_SOURCE */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Digest_Compare(pubSourceKeyDigest, restrictTicket.sourceKeyDigest);
	    if (returnCode != TPM_SUCCESS) {
		printf("TPM_Process_CMK_CreateBlob: Error, no match to sourceKeyDigest\n");
		returnCode = TPM_MA_SOURCE;
	    }
	}
    }
    /* 10. Else return with error TPM_BAD_PARAMETER. */
    else if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CMK_CreateBlob: Error, Illegal migrationScheme %04hx\n",
	       migrationKeyAuth.migrationScheme);
	returnCode = TPM_BAD_PARAMETER;
    }
    /* 11. Build two bytes array, K1 and K2, using d1: */
    /* a. K1 = TPM_STORE_ASYMKEY.privKey[0..19] (TPM_STORE_ASYMKEY.privKey.keyLength + 16 bytes of
       TPM_STORE_ASYMKEY.privKey.key), sizeof(K1) = 20 */
    /* b. K2 = TPM_STORE_ASYMKEY.privKey[20..131] (position 16-127 of
       TPM_STORE_ASYMKEY.privKey.key), sizeof(K2) = 112 */
    /* 12. Build M1 a TPM_MIGRATE_ASYMKEY structure */
    /* a. TPM_MIGRATE_ASYMKEY.payload = TPM_PT_CMK_MIGRATE */
    /* b. TPM_MIGRATE_ASYMKEY.usageAuth = TPM_STORE_ASYMKEY.usageAuth */
    /* c. TPM_MIGRATE_ASYMKEY.pubDataDigest = TPM_STORE_ASYMKEY.pubDataDigest */
    /* d. TPM_MIGRATE_ASYMKEY.partPrivKeyLen = 112 - 127.  */
    /* e. TPM_MIGRATE_ASYMKEY.partPrivKey = K2 */
    /* 13. Create o1 (which SHALL be 198 bytes for a 2048 bit RSA key) by performing the OAEP
       encoding of m using OAEP parameters m, pHash, and seed */
    /* a. m is the previously created M1 */
    /* b. pHash = SHA-1( SHA-1[msaList] || pubSourceKeyDigest) */
    /* c. seed = s1 = the previously created K1 */
    /* 14. Create r1 a random value from the TPM RNG. The size of r1 MUST be the size of o1. Return
       r1 in the */
    /* random parameter */
    /* 15. Create x1 by XOR of o1 with r1 */
    /* 16. Copy r1 into the output field "random" */
    /* 17. Encrypt x1 with the migrationKeyAuth-> migrationKey */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SHA1(pHash,
			      TPM_DIGEST_SIZE, m2CmkMigauth.msaDigest,
			      TPM_DIGEST_SIZE, pubSourceKeyDigest,
			      0, NULL);
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CreateBlobCommon(&outData,
					  &d1AsymKey,
					  pHash,
					  TPM_PT_CMK_MIGRATE,
					  &random,
					  &(migrationKeyAuth.migrationKey));
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_CMK_CreateBlob: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return random */
	    returnCode = TPM_SizedBuffer_Store(response, &random);
	}
	if (returnCode == TPM_SUCCESS) {
	    /* return outData */
	    returnCode = TPM_SizedBuffer_Store(response, &outData);
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
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_AuthParams_Set(response,
					    *hmacKey,		/* owner HMAC key */
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
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions,
					 parentAuthHandle);
    }
    /*
      cleanup
    */
    free(d1Decrypt);					/* @1 */
    TPM_Migrationkeyauth_Delete(&migrationKeyAuth);	/* @2 */
    TPM_SizedBuffer_Delete(&msaListBuffer);		/* @3 */
    TPM_SizedBuffer_Delete(&restrictTicketBuffer);	/* @4 */
    TPM_SizedBuffer_Delete(&sigTicketBuffer);		/* @5 */
    TPM_SizedBuffer_Delete(&encData);			/* @6 */
    TPM_SizedBuffer_Delete(&random);			/* @7 */
    TPM_SizedBuffer_Delete(&outData);			/* @8 */
    TPM_Sbuffer_Delete(&mka_sbuffer);			/* @9 */
    TPM_StoreAsymkey_Delete(&d1AsymKey);		/* @10 */
    TPM_MsaComposite_Delete(&msaList);			/* @11 */
    TPM_CmkAuth_Delete(&restrictTicket);		/* @12 */
    TPM_CmkMigauth_Delete(&m2CmkMigauth);		/* @13 */
    TPM_CmkSigticket_Delete(&v1CmkSigticket);		/* @14 */
    return rcf;
}

/* 11.7 TPM_CMK_SetRestrictions rev 96

   This command is used by the Owner to dictate the usage of a certified-migration key with
   delegated authorisation (authorisation other than actual Owner authorisation).

   This command is provided for privacy reasons and must not itself be delegated, because a
   certified-migration-key may involve a contractual relationship between the Owner and an external
   entity.

   Since restrictions are validated at DSAP session use, there is no need to invalidate DSAP
   sessions when the restriction value changes.
*/

TPM_RESULT TPM_Process_CMK_SetRestrictions(tpm_state_t *tpm_state,
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
    TPM_CMK_DELEGATE restriction;	/* The bit mask of how to set the restrictions on CMK keys
					   */
    TPM_AUTHHANDLE authHandle;		/* The authorization handle TPM Owner authorization */
    TPM_NONCE nonceOdd;			/* Nonce generated by system associated with authHandle */
    TPM_BOOL continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   handle */
    TPM_AUTHDATA ownerAuth;		/* The authorization digest. HMAC key: TPM Owner Auth */

    /* processing parameters */
    unsigned char *		inParamStart;	/* starting point of inParam's */
    unsigned char *		inParamEnd;	/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_CMK_SetRestrictions: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get restriction */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&restriction, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CMK_SetRestrictions: restriction %08x\n", restriction);
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
	returnCode = TPM_CheckRequestTag1(tag);
    }
    /* get the 'below the line' authorization parameters  */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					ownerAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_CMK_SetRestrictions: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* 1. Validate the ordinal and parameters using TPM Owner authorization, return TPM_AUTHFAIL on
	  error */
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
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_CMK_SetRestrictions: ownerAuth secret", *hmacKey);
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,	/* owner HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,	/* Nonce generated by system
							   associated with authHandle */
					continueAuthSession,
					ownerAuth);	/* Authorization digest for input */
    }
    /*	2. Set TPM_PERMANENT_DATA -> TPM_CMK_DELEGATE -> restrictDelegate = restriction */
    if (returnCode == TPM_SUCCESS) {
	/* only update NVRAM if the value is changing */
	if (tpm_state->tpm_permanent_data.restrictDelegate != restriction) {
	    tpm_state->tpm_permanent_data.restrictDelegate = restriction;
	    /* Store the permanent data back to NVRAM */
	    printf("TPM_Process_CMK_SetRestrictions: Storing permanent data\n");
	    returnCode = TPM_PermanentAll_NVStore(tpm_state,
						  TRUE,	/* write NV */
						  0);	/* no roll back */
	}
	else {
	    printf("TPM_Process_CMK_SetRestrictions: No change to value\n");
	}
    }
    /*	3. Return TPM_SUCCESS */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_CMK_SetRestrictions: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
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
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_AuthParams_Set(response,
					    *hmacKey,		/* owner HMAC key */
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
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    return rcf;
}

/* 11.6 TPM_CMK_ApproveMA 87

  This command creates an authorization ticket, to allow the TPM owner to specify which Migration
  Authorities they approve and allow users to create certified-migration-keys without further
  involvement with the TPM owner.

  It is the responsibility of the TPM Owner to determine whether a particular Migration Authority is
  suitable to control migration.
*/
  
TPM_RESULT TPM_Process_CMK_ApproveMA(tpm_state_t *tpm_state,
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
    TPM_DIGEST migrationAuthorityDigest;	/* A digest of a TPM_MSA_COMPOSITE structure (itself
						   one or more digests of public keys belonging to
						   migration authorities) */
    TPM_AUTHHANDLE authHandle;		/* The authorization session handle used for owner
					   authentication. */
    TPM_NONCE nonceOdd;			/* Nonce generated by system associated with authHandle */
    TPM_BOOL continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   session handle */
    TPM_AUTHDATA ownerAuth;		/* Authorization HMAC, key: ownerAuth. */

    /* processing parameters */
    unsigned char *		inParamStart;	/* starting point of inParam's */
    unsigned char *		inParamEnd;	/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_CMK_MA_APPROVAL		m2CmkMaApproval;

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_HMAC		outData;	/* HMAC of migrationAuthorityDigest */

    printf("TPM_Process_CMK_ApproveMA: Ordinal Entry\n");
    TPM_CmkMaApproval_Init(&m2CmkMaApproval);	/* freed @1 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get migrationAuthorityDigest */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Digest_Load(migrationAuthorityDigest, &command, &paramSize);
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
	returnCode = TPM_CheckRequestTag1(tag);
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
	if (paramSize != 0) {
	    printf("TPM_Process_CMK_ApproveMA: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* 1. Validate the AuthData to use the TPM by the TPM Owner */
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
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* owner HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					ownerAuth);		/* Authorization digest for input */
    }
    if (returnCode == TPM_SUCCESS) {
	/* 2. Create M2 a TPM_CMK_MA_APPROVAL structure */
	/* NOTE Done by TPM_CmkMaApproval_Init() */
	/* a. Set M2 ->migrationAuthorityDigest to migrationAuthorityDigest */
	TPM_Digest_Copy(m2CmkMaApproval.migrationAuthorityDigest, migrationAuthorityDigest);
	/* 3. Set outData = HMAC(M2) using tpmProof as the secret */
	returnCode = TPM_HMAC_GenerateStructure
		     (outData,					/* HMAC */
		      tpm_state->tpm_permanent_data.tpmProof,	/* HMAC key */
		      &m2CmkMaApproval,				/* structure */
		      (TPM_STORE_FUNCTION_T)TPM_CmkMaApproval_Store);	/* store function */
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_CMK_ApproveMA: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return the outData */
	    returnCode = TPM_Digest_Store(response, outData);
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
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_AuthParams_Set(response,
					    *hmacKey,		/* HMAC key */
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
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    TPM_CmkMaApproval_Delete(&m2CmkMaApproval); /* @1 */
    return rcf;
}

/* 11.10 TPM_CMK_ConvertMigration rev 106

   TPM_CMK_ConvertMigration completes the migration of certified migration blobs.

   This command takes a certified migration blob and creates a normal wrapped blob with payload type
   TPM_PT_MIGRATE_EXTERNAL. The migrated blob must be loaded into the TPM using the normal
   TPM_LoadKey function.

   Note that the command migrates private keys, only. The migration of the associated public keys is
   not specified by TPM because they are not security sensitive. Migration of the associated public
   keys may be specified in a platform specific specification. A TPM_KEY structure must be recreated
   before the migrated key can be used by the target TPM in a LoadKey command.

   TPM_CMK_ConvertMigration checks that one of the MAs implicitly listed in the migrationAuth of the
   target key has approved migration of the target key to the destination (parent) key, and that the
   settings (flags etc.) in the target key are those of a CMK.
*/

TPM_RESULT TPM_Process_CMK_ConvertMigration(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE	parentHandle;	/* Handle of a loaded key that can decrypt keys. */
    TPM_CMK_AUTH	restrictTicket; /* The digests of public keys belonging to the Migration
					   Authority, the destination parent key and the
					   key-to-be-migrated. */
    TPM_HMAC		sigTicket;	/* A signature ticket, generated by the TPM, signaling a
					   valid signature over restrictTicket */
    TPM_KEY		migratedKey;	/* The public key of the key-to-be-migrated. The private
					   portion MUST be TPM_MIGRATE_ASYMKEY properly XOR'd */
    TPM_SIZED_BUFFER	msaListBuffer;	/* One or more digests of public keys belonging to migration
					   authorities */
    TPM_SIZED_BUFFER	random;		/* Random value used to hide key data. */
    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle used for keyHandle. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession;	/* The continue use flag for the authorization session
					   handle */
    TPM_AUTHDATA	parentAuth;	/* Authorization HMAC: parentKey.usageAuth */

    /* processing parameters */
    unsigned char *		inParamStart;	/* starting point of inParam's */
    unsigned char *		inParamEnd;	/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_KEY			*parentKey;
    TPM_BOOL			parentPCRStatus;
    TPM_SECRET			*parentUsageAuth;
    unsigned char		*d1Decrypt;
    uint32_t			d1DecryptLength = 0;	/* actual valid data */
    BYTE			*o1Oaep;
    unsigned char		*stream;		/* for deserializing structures */
    uint32_t			stream_size;
    TPM_MSA_COMPOSITE		msaList;
    TPM_DIGEST			msaListDigest;
    TPM_DIGEST			migratedPubKeyDigest;
    TPM_STORE_ASYMKEY		d2AsymKey;
    TPM_STORE_BUFFER		d2_sbuffer;
    TPM_DIGEST			parentPubKeyDigest;
    TPM_CMK_SIGTICKET		v1CmkSigticket;
    TPM_CMK_MIGAUTH		m2CmkMigauth;

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_SIZED_BUFFER	outData;	/* The encrypted private key that can be loaded with
					   TPM_LoadKey */

    printf("TPM_Process_CMK_ConvertMigration: Ordinal Entry\n");
    TPM_CmkAuth_Init(&restrictTicket);		/* freed @1 */
    TPM_Key_Init(&migratedKey);			/* freed @2 */
    TPM_SizedBuffer_Init(&msaListBuffer);	/* freed @3 */
    TPM_SizedBuffer_Init(&random);		/* freed @4 */
    TPM_SizedBuffer_Init(&outData);		/* freed @5 */
    d1Decrypt = NULL;				/* freed @6 */
    TPM_MsaComposite_Init(&msaList);		/* freed @7 */
    TPM_StoreAsymkey_Init(&d2AsymKey);		/* freed @8 */
    TPM_Sbuffer_Init(&d2_sbuffer);		/* freed @9 */
    TPM_CmkSigticket_Init(&v1CmkSigticket);	/* freed @10 */
    o1Oaep = NULL;				/* freed @11 */
    TPM_CmkMigauth_Init(&m2CmkMigauth);		/* freed @12 */
    /*
      get inputs
    */
    /* get parentHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&parentHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get restrictTicket */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CMK_ConvertMigration: parentHandle %08x\n", parentHandle);
	returnCode = TPM_CmkAuth_Load(&restrictTicket, &command, &paramSize);
    }
    /* get sigTicket */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Digest_Load(sigTicket, &command, &paramSize);
    }
    /* get migratedKey */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_Load(&migratedKey, &command, &paramSize);
    }
    /* get msaListBuffer */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&msaListBuffer, &command, &paramSize);
    }
    /* get random */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&random, &command, &paramSize);
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
	returnCode = TPM_CheckRequestTag1(tag);
    }
    /* get the 'below the line' authorization parameters  */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					parentAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_CMK_ConvertMigration: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* 1. Validate the AuthData to use the key in parentHandle */
    /* get the key associated with parentHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&parentKey, &parentPCRStatus,
						 tpm_state, parentHandle,
						 FALSE,		/* not r/o, using private key */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    /* get parentHandle -> usageAuth */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_GetUsageAuth(&parentUsageAuth, parentKey);
    }	 
    /* get the session data */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_NONE,
					      TPM_ET_KEYHANDLE,
					      ordinal,
					      parentKey,
					      parentUsageAuth,			/* OIAP */
					      parentKey->tpm_store_asymkey->pubDataDigest); /*OSAP*/
    }
    /* 1. Validate the authorization to use the key in parentHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					parentAuth);		/* Authorization digest for input */
    }
    /* 2. If the keyUsage field of the key referenced by parentHandle does not have the value
       TPM_KEY_STORAGE, the TPM must return the error code TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	if (parentKey->keyUsage != TPM_KEY_STORAGE) {
	    printf("TPM_Process_CMK_ConvertMigration: Error, "
		   "parentHandle -> keyUsage should be TPM_KEY_STORAGE, is %04x\n",
		   parentKey->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 3. Create d1 by decrypting the migratedKey -> encData area using the key in parentHandle */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CMK_ConvertMigration: Decrypting encData\n");
	TPM_PrintFour("TPM_Process_CMK_ConvertMigration: encData", migratedKey.encData.buffer);
	returnCode = TPM_RSAPrivateDecryptMalloc(&d1Decrypt,		/* decrypted data */
						 &d1DecryptLength,	/* actual size of d1 data */
						 migratedKey.encData.buffer,	/* encrypted data */
						 migratedKey.encData.size,
						 parentKey);
    }
    /* the random input parameter must be the same length as the decrypted data */
    if (returnCode == TPM_SUCCESS) {
	if (d1DecryptLength != random.size) {
	    printf("TPM_Process_CMK_ConvertMigration: Error "
		   "decrypt data length %u random size %u\n",
		   d1DecryptLength, random.size);
	    returnCode = TPM_BAD_PARAMETER;
	}
    }
    /* allocate memory for o1 */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Malloc(&o1Oaep, d1DecryptLength);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CMK_ConvertMigration: d1 length %u\n", d1DecryptLength);
	TPM_PrintFour("TPM_Process_CMK_ConvertMigration: d1 -", d1Decrypt);
	/* 4. Create o1 by XOR d1 and random parameter */
	TPM_XOR(o1Oaep, d1Decrypt, random.buffer, d1DecryptLength);
	/* 5. Create m1 a TPM_MIGRATE_ASYMKEY, seed and pHash by OAEP decoding o1 */
	/* 7. Create k1 by combining seed and the TPM_MIGRATE_ASYMKEY -> partPrivKey */
	/* 8. Create d2 a TPM_STORE_ASYMKEY structure */
	/* a. Set the TPM_STORE_ASYMKEY -> privKey field to k1 */
	/* b. Set d2 -> usageAuth to m1 -> usageAuth */
	/* c. Set d2 -> pubDataDigest to m1 -> pubDataDigest */
	returnCode = TPM_StoreAsymkey_LoadO1(&d2AsymKey, o1Oaep, d1DecryptLength);
    }
    if (returnCode == TPM_SUCCESS) {	
	printf("TPM_Process_CMK_ConvertMigration: Checking pHash\n");
	/* 6. Create migratedPubKey a TPM_PUBKEY structure corresponding to migratedKey */
	/* NOTE this function goes directly to the SHA1 digest */
	returnCode = TPM_Key_GeneratePubkeyDigest(migratedPubKeyDigest, &migratedKey);
    }
    /* 6.a. Verify that pHash == SHA-1( SHA-1[msaList] || SHA-1(migratedPubKey ) */
    /* deserialize to msaListBuffer to msaList */
    if (returnCode == TPM_SUCCESS) {
	stream = msaListBuffer.buffer;
	stream_size = msaListBuffer.size;
	returnCode = TPM_MsaComposite_Load(&msaList, &stream, &stream_size);
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SHA1_GenerateStructure(msaListDigest, &msaList,
						(TPM_STORE_FUNCTION_T)TPM_MsaComposite_Store);
    }
    /* pHash is returned in TPM_STORE_ASYMKEY -> migrationAuth */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SHA1_Check(d2AsymKey.migrationAuth,
				    TPM_DIGEST_SIZE, msaListDigest,
				    TPM_DIGEST_SIZE, migratedPubKeyDigest,
				    0, NULL);
    }
    /* 9. Verify that parentHandle-> keyFlags -> migratable == FALSE and parentHandle-> encData ->
       migrationAuth == tpmProof */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CMK_ConvertMigration: Checking parent key\n");
	if (parentKey->keyFlags & TPM_MIGRATABLE) {
	    printf("TPM_Process_CMK_ConvertMigration: Error, parent migratable\n");
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 10. Verify that m1 -> payload == TPM_PT_CMK_MIGRATE, then set d2-> payload =
       TPM_PT_MIGRATE_EXTERNAL */
    /* NOTE TPM_StoreAsymkey_LoadO1() copied TPM_MIGRATE_ASYMKEY -> payload to TPM_STORE_ASYMKEY ->
       payload */
    if (returnCode == TPM_SUCCESS) {
	if (d2AsymKey.payload != TPM_PT_CMK_MIGRATE) {
	    printf("TPM_Process_CMK_ConvertMigration: Error, invalid payload %02x\n",
		   d2AsymKey.payload);
	    returnCode = TPM_BAD_MIGRATION;
	}
	else {
	    d2AsymKey.payload = TPM_PT_MIGRATE_EXTERNAL;
	}
    }
    /* 11. Verify that for one of the n=1 to n=(msaList -> MSAlist) values of msaList ->
       migAuthDigest[n], sigTicket == HMAC (V1) using tpmProof as the secret where V1 is a
       TPM_CMK_SIGTICKET structure such that: */
    /* a. V1 -> verKeyDigest = msaList -> migAuthDigest[n] */
    /* b. V1 -> signedData = SHA-1[restrictTicket] */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CMK_ConvertMigration: Checking sigTicket\n");
	/* generate SHA1[restrictTicket] */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_SHA1_GenerateStructure(v1CmkSigticket.signedData, &restrictTicket,
						    (TPM_STORE_FUNCTION_T)TPM_CmkAuth_Store);
	}
	if (returnCode == TPM_SUCCESS) {
	    TPM_PrintFour(" TPM_Process_CMK_ConvertMigration: TPM_CMK_SIGTICKET -> sigTicket",
			  v1CmkSigticket.signedData);
	    returnCode = TPM_MsaComposite_CheckSigTicket(sigTicket,
							 tpm_state->tpm_permanent_data.tpmProof,
							 &msaList,
							 &v1CmkSigticket);
	}
    }
    /* 12. Create parentPubKey, a TPM_PUBKEY structure corresponding to parenthandle */
    if (returnCode == TPM_SUCCESS) {
	/* NOTE this function goes directly to the SHA1 digest */
	returnCode = TPM_Key_GeneratePubkeyDigest(parentPubKeyDigest, parentKey);
    }
    /* 13. If [restrictTicket -> destinationKeyDigest] != SHA-1(parentPubKey), return error
       TPM_MA_DESTINATION */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Digest_Compare(restrictTicket.destinationKeyDigest,
					parentPubKeyDigest);
	if (returnCode != TPM_SUCCESS) {
	    printf("TPM_Process_CMK_ConvertMigration: Error checking destinationKeyDigest\n");
	    returnCode = TPM_MA_DESTINATION;
	}	    
    }
    /* 14. Verify that migratedKey is corresponding to d2 */
    /* NOTE check the private key against the public key */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_StorePrivkey_Convert(&d2AsymKey,
					      &(migratedKey.algorithmParms),
					      &(migratedKey.pubKey));
    }
    /* 15. If migratedKey -> keyFlags -> migratable is FALSE, and return error TPM_INVALID_KEYUSAGE
       */
    if (returnCode == TPM_SUCCESS) {
	if (!(migratedKey.keyFlags & TPM_MIGRATABLE)) {
	    printf("TPM_Process_CMK_ConvertMigration: Error, migratedKey migratable is FALSE\n");
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 16. If migratedKey -> keyFlags -> migrateAuthority is FALSE, return error
       TPM_INVALID_KEYUSAGE
       */
    if (returnCode == TPM_SUCCESS) {
	if (!(migratedKey.keyFlags & TPM_MIGRATEAUTHORITY)) {
	    printf("TPM_Process_CMK_ConvertMigration: Error, "
		   "migratedKey migrateauthority is FALSE\n");
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 17. If [restrictTicket -> sourceKeyDigest] != SHA-1(migratedPubKey), return error
       TPM_MA_SOURCE */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Digest_Compare(restrictTicket.sourceKeyDigest, migratedPubKeyDigest);
	if (returnCode != TPM_SUCCESS) {
	    printf("TPM_Process_CMK_ConvertMigration: Error checking sourceKeyDigest\n");
	    returnCode = TPM_MA_SOURCE;
	}
    }
    /* 18. Create M2 a TPM_CMK_MIGAUTH structure */
    /* NOTE Done by TPM_CmkMigauth_Init() */
    /* a. Set M2 -> msaDigest to SHA-1[msaList] */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SHA1_GenerateStructure(m2CmkMigauth.msaDigest, &msaList,
						(TPM_STORE_FUNCTION_T)TPM_MsaComposite_Store);
    }
    if (returnCode == TPM_SUCCESS) {
	/* b. Set M2 -> pubKeyDigest to SHA-1[migratedPubKey] */
	TPM_Digest_Copy(m2CmkMigauth.pubKeyDigest, migratedPubKeyDigest);
	/* 19. Set d2 -> migrationAuth = HMAC(M2) using tpmProof as the secret */
	returnCode = TPM_HMAC_GenerateStructure
		     (d2AsymKey.migrationAuth,	/* HMAC */
		      tpm_state->tpm_permanent_data.tpmProof,	/* HMAC key */
		      &m2CmkMigauth,				/* structure */
		      (TPM_STORE_FUNCTION_T)TPM_CmkMigauth_Store);	/* store function */
   }
    /* 21. Create outData using the key in parentHandle to perform the encryption */
    if (returnCode == TPM_SUCCESS) {
	/* serialize d2Asymkey	to d2_sbuffer */
	returnCode = TPM_StoreAsymkey_Store(&d2_sbuffer, FALSE, &d2AsymKey);
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_RSAPublicEncryptSbuffer_Key(&outData, &d2_sbuffer, parentKey);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_CMK_ConvertMigration: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return the outData */
	    returnCode = TPM_SizedBuffer_Store(response, &outData);
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
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_AuthParams_Set(response,
					    *hmacKey,		/* HMAC key */
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
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    TPM_CmkAuth_Delete(&restrictTicket);	/* @1 */
    TPM_Key_Delete(&migratedKey);		/* @2 */
    TPM_SizedBuffer_Delete(&msaListBuffer);	/* @3 */
    TPM_SizedBuffer_Delete(&random);		/* @4 */
    TPM_SizedBuffer_Delete(&outData);		/* @5 */
    free(d1Decrypt);				/* @6 */
    TPM_MsaComposite_Delete(&msaList);		/* @7 */
    TPM_StoreAsymkey_Delete(&d2AsymKey);	/* @8 */
    TPM_Sbuffer_Delete(&d2_sbuffer);		/* @9 */
    TPM_CmkSigticket_Delete(&v1CmkSigticket);	/* @10 */
    free(o1Oaep);				/* @11 */
    TPM_CmkMigauth_Delete(&m2CmkMigauth);	/* @12 */
    return rcf;
}
