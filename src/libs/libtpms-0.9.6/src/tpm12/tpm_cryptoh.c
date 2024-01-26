/********************************************************************************/
/*										*/
/*		High Level Platform Independent Cryptography			*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_cryptoh.c $		*/
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
#include <stdarg.h>

#include "tpm_admin.h"
#include "tpm_auth.h"
#include "tpm_crypto.h"
#include "tpm_debug.h"
#include "tpm_digest.h"
#include "tpm_error.h"
#include "tpm_io.h"
#include "tpm_memory.h"
#include "tpm_migration.h"
#include "tpm_nonce.h"
#include "tpm_key.h"
#include "tpm_pcr.h"
#include "tpm_process.h"
#include "tpm_store.h"
#include "tpm_ver.h"

#include "tpm_cryptoh.h"

/* local prototypes */

static TPM_RESULT TPM_SHA1_valist(TPM_DIGEST md, 
				  uint32_t length0, unsigned char *buffer0,
				  va_list ap);
static TPM_RESULT TPM_HMAC_Generatevalist(TPM_HMAC hmac,
					  const TPM_SECRET key,
					  va_list ap);

static TPM_RESULT TPM_SHA1CompleteCommon(TPM_DIGEST hashValue,
					 void **sha1_context,
					 TPM_SIZED_BUFFER *hashData);

/*
  TPM_SIGN_INFO
*/

/* TPM_SignInfo_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_SignInfo_Init(TPM_SIGN_INFO *tpm_sign_info)
{
    printf(" TPM_SignInfo_Init:\n");
    memset(tpm_sign_info->fixed, 0, TPM_SIGN_INFO_FIXED_SIZE); 
    TPM_Nonce_Init(tpm_sign_info->replay);
    TPM_SizedBuffer_Init(&(tpm_sign_info->data));
    return;
}

/* TPM_SignInfo_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_SignInfo_Store(TPM_STORE_BUFFER *sbuffer,
			      const TPM_SIGN_INFO *tpm_sign_info)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_SignInfo_Store:\n");
    /* store the tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_SIGNINFO);
    }
    /* store the fixed */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, tpm_sign_info->fixed, TPM_SIGN_INFO_FIXED_SIZE);
    }
    /* store the replay */
    if (rc == 0) {
	rc = TPM_Nonce_Store(sbuffer, tpm_sign_info->replay);
    }
    /* store the dataLen and data */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_sign_info->data));
    }
    if (rc == 0) {
	const unsigned char *buffer;
	uint32_t length;
	TPM_Sbuffer_Get(sbuffer, &buffer, &length);
	TPM_PrintAll("  TPM_SignInfo_Store: Buffer", buffer, length);
    }
    return rc;
}

/* TPM_SignInfo_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the sign_info
   sets pointers to NULL
   calls TPM_SignInfo_Init to set members back to default values
   The sign_info itself is not freed
*/   

void TPM_SignInfo_Delete(TPM_SIGN_INFO *tpm_sign_info)
{
    printf(" TPM_SignInfo_Delete:\n");
    if (tpm_sign_info != NULL) {
	TPM_SizedBuffer_Delete(&(tpm_sign_info->data));
	TPM_SignInfo_Init(tpm_sign_info);
    }
    return;
}

/*
  TPM_CERTIFY_INFO
*/

/* TPM_CertifyInfo_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_CertifyInfo_Init(TPM_CERTIFY_INFO *tpm_certify_info)
{
    printf(" TPM_CertifyInfo_Init:\n");
    TPM_StructVer_Init(&(tpm_certify_info->version));
    tpm_certify_info->keyUsage = TPM_KEY_UNINITIALIZED;
    tpm_certify_info->keyFlags = 0;
    tpm_certify_info->authDataUsage = TPM_AUTH_ALWAYS;
    TPM_KeyParms_Init(&(tpm_certify_info->algorithmParms));
    TPM_Digest_Init(tpm_certify_info->pubkeyDigest);
    TPM_Nonce_Init(tpm_certify_info->data);
    tpm_certify_info->parentPCRStatus = TRUE;
    TPM_SizedBuffer_Init(&(tpm_certify_info->pcrInfo));
    tpm_certify_info->tpm_pcr_info = NULL;
    return;
}

#if 0
/* TPM_CertifyInfo_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_CertifyInfo_Init()
   After use, call TPM_CertifyInfo_Delete() to free memory

   NOTE: Never called.
*/

TPM_RESULT TPM_CertifyInfo_Load(TPM_CERTIFY_INFO *tpm_certify_info,
				unsigned char **stream,
				uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_CertifyInfo_Load:\n");
    /* load version */
    if (rc == 0) {
	rc = TPM_StructVer_Load(&(tpm_certify_info->version), stream, stream_size);
    }
    /* check ver immediately to ease debugging */
    if (rc == 0) {
	rc = TPM_StructVer_CheckVer(&(tpm_certify_info->version));
    }
    /* load keyUsage */
    if (rc == 0) {
	rc = TPM_Load16(&(tpm_certify_info->keyUsage), stream, stream_size);
    }
    /* load keyFlags */
    if (rc == 0) {
	rc = TPM_KeyFlags_Load(&(tpm_certify_info->keyFlags), stream, stream_size);
    }
    /* load authDataUsage */
    if (rc == 0) {
	rc = TPM_Load8(&(tpm_certify_info->authDataUsage), stream, stream_size);
    }
    /* load algorithmParms */
    if (rc == 0) {
	rc = TPM_KeyParms_Load(&(tpm_certify_info->algorithmParms), stream, stream_size);
    }
    /* load pubkeyDigest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_certify_info->pubkeyDigest, stream, stream_size);
    }
    /* load data */
    if (rc == 0) {
	rc = TPM_Nonce_Load(tpm_certify_info->data, stream, stream_size);
    }
    /* load parentPCRStatus */
    if (rc == 0) {
	rc = TPM_LoadBool(&(tpm_certify_info->parentPCRStatus), stream, stream_size);
    }
    /* load pcrInfo */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_certify_info->pcrInfo), stream, stream_size);
    }
    /* set TPM_PCR_INFO tpm_pcr_info cache from pcrInfo */
    if (rc == 0) {
	rc = TPM_PCRInfo_CreateFromBuffer(&(tpm_certify_info->tpm_pcr_info),
					  &(tpm_certify_info->pcrInfo));
    }
    return rc;
}
#endif

/* TPM_CertifyInfo_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_CertifyInfo_Store(TPM_STORE_BUFFER *sbuffer,
				 TPM_CERTIFY_INFO *tpm_certify_info)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_CertifyInfo_Store:\n");
    /* store version */
    if (rc == 0) {
	rc = TPM_StructVer_Store(sbuffer, &(tpm_certify_info->version));
    }
    /* store keyUsage */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, tpm_certify_info->keyUsage);
    }
    /* store keyFlags */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_certify_info->keyFlags);
    }
    /* store authDataUsage */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_certify_info->authDataUsage),
				sizeof(TPM_AUTH_DATA_USAGE)); 
    }
    /* store algorithmParms */
    if (rc == 0) {
	rc = TPM_KeyParms_Store(sbuffer, &(tpm_certify_info->algorithmParms));
    }
    /* store pubkeyDigest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_certify_info->pubkeyDigest);
    }
    /* store data */
    if (rc == 0) {
	rc = TPM_Nonce_Store(sbuffer, tpm_certify_info->data);
    }
    /* store parentPCRStatus */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_certify_info->parentPCRStatus),
				sizeof(TPM_BOOL));
    }
    /* copy cache to pcrInfo */
    if (rc == 0) {
	rc = TPM_SizedBuffer_SetStructure(&(tpm_certify_info->pcrInfo),
					  tpm_certify_info->tpm_pcr_info,
					  (TPM_STORE_FUNCTION_T)TPM_PCRInfo_Store);
    }
    /* copy pcrInfo to sbuffer */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_certify_info->pcrInfo));
    }
    return rc;
}

/* TPM_CertifyInfo_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_CertifyInfo_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_CertifyInfo_Delete(TPM_CERTIFY_INFO *tpm_certify_info)
{
    printf(" TPM_CertifyInfo_Delete:\n");
    if (tpm_certify_info != NULL) {
	TPM_KeyParms_Delete(&(tpm_certify_info->algorithmParms));
	/* pcrInfo */
	TPM_SizedBuffer_Delete(&(tpm_certify_info->pcrInfo));
	/* pcr cache */
	TPM_PCRInfo_Delete(tpm_certify_info->tpm_pcr_info);
	free(tpm_certify_info->tpm_pcr_info);
	TPM_CertifyInfo_Init(tpm_certify_info);
    }
    return;
}

/* TPM_CertifyInfo_Set() fills in tpm_certify_info with the information from the key pointed to be
   tpm_key
*/

TPM_RESULT TPM_CertifyInfo_Set(TPM_CERTIFY_INFO *tpm_certify_info,
			       TPM_KEY *tpm_key)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_CertifyInfo_Set:\n");
    if (rc == 0) {
	tpm_certify_info->keyUsage = tpm_key->keyUsage;
	tpm_certify_info->keyFlags = tpm_key->keyFlags;
	tpm_certify_info->authDataUsage = tpm_key->authDataUsage;
	rc = TPM_KeyParms_Copy(&(tpm_certify_info->algorithmParms),
			       &(tpm_key->algorithmParms));
    }
    /* pubkeyDigest SHALL be a digest of the value TPM_KEY -> pubKey -> key in a TPM_KEY
       representation of the key to be certified */
    if (rc == 0) {
	rc = TPM_SHA1(tpm_certify_info->pubkeyDigest,
		      tpm_key->pubKey.size, tpm_key->pubKey.buffer,
		      0, NULL);
    }
    return rc;
}

/*
  TPM_CERTIFY_INFO2
*/

/* TPM_CertifyInfo2_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_CertifyInfo2_Init(TPM_CERTIFY_INFO2 *tpm_certify_info2)
{
    printf(" TPM_CertifyInfo2_Init:\n");
    tpm_certify_info2->fill = 0x00;
    tpm_certify_info2->payloadType = TPM_PT_ASYM;
    tpm_certify_info2->keyUsage = TPM_KEY_UNINITIALIZED;
    tpm_certify_info2->keyFlags = 0;
    tpm_certify_info2->authDataUsage = TPM_AUTH_ALWAYS;
    TPM_KeyParms_Init(&(tpm_certify_info2->algorithmParms));
    TPM_Digest_Init(tpm_certify_info2->pubkeyDigest);
    TPM_Nonce_Init(tpm_certify_info2->data);
    tpm_certify_info2->parentPCRStatus = TRUE;
    TPM_SizedBuffer_Init(&(tpm_certify_info2->pcrInfo));
    TPM_SizedBuffer_Init(&(tpm_certify_info2->migrationAuthority));
    tpm_certify_info2->tpm_pcr_info_short = NULL;
    return;
}

#if 0
/* TPM_CertifyInfo2_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_CertifyInfo2_Init()
   After use, call TPM_CertifyInfo2_Delete() to free memory
*/

TPM_RESULT TPM_CertifyInfo2_Load(TPM_CERTIFY_INFO2 *tpm_certify_info2,
				 unsigned char **stream,
				 uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_CertifyInfo2_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_CERTIFY_INFO2, stream, stream_size);
    }
    /* load fill */
    if (rc == 0) {
	rc = TPM_Load8(&(tpm_certify_info2->fill), stream, stream_size);
    }
    /* check fill immediately to ease debugging */
    if (rc == 0) {
	if (tpm_certify_info2->fill != 0x00) {
	    printf("TPM_CertifyInfo2_Load: Error checking fill %02x\n", tpm_certify_info2->fill);
	    rc = TPM_INVALID_STRUCTURE;
	}
    }
    /* load payloadType */
    if (rc == 0) {
	rc = TPM_Load8(&(tpm_certify_info2->payloadType), stream, stream_size);
    }
    /* load keyUsage */
    if (rc == 0) {
	rc = TPM_Load16(&(tpm_certify_info2->keyUsage), stream, stream_size);
    }
    /* load keyFlags */
    if (rc == 0) {
	rc = TPM_KeyFlags_Load(&(tpm_certify_info2->keyFlags), stream, stream_size);
    }
    /* load authDataUsage */
    if (rc == 0) {
	rc = TPM_Load8(&(tpm_certify_info2->authDataUsage), stream, stream_size);
    }
    /* load algorithmParms */
    if (rc == 0) {
	rc = TPM_KeyParms_Load(&(tpm_certify_info2->algorithmParms), stream, stream_size);
    }
    /* load pubkeyDigest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_certify_info2->pubkeyDigest, stream, stream_size);
    }
    /* load data */
    if (rc == 0) {
	rc = TPM_Nonce_Load(tpm_certify_info2->data, stream, stream_size);
    }
    /* load parentPCRStatus */
    if (rc == 0) {
	rc = TPM_LoadBool(&(tpm_certify_info2->parentPCRStatus), stream, stream_size);
    }
    /* load pcrInfo */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_certify_info2->pcrInfo), stream, stream_size);
    }
    /* set TPM_PCR_INFO2 tpm_pcr_info cache from pcrInfo */
    if (rc == 0) {
	rc = TPM_PCRInfoShort_CreateFromBuffer(&(tpm_certify_info2->tpm_pcr_info_short),
					       &(tpm_certify_info2->pcrInfo));
    }
    /* load migrationAuthority */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_certify_info2->migrationAuthority), stream, stream_size);
    }
    /* check migrationAuthority immediately to ease debugging */
    if (rc == 0) {
	if ((tpm_certify_info2->migrationAuthority.buffer != NULL) &&
	    (tpm_certify_info2->migrationAuthority.size != TPM_DIGEST_SIZE)) {
	    printf("TPM_CertifyInfo2_Load: Error checking migrationAuthority %p, %u\n",
		   tpm_certify_info2->migrationAuthority.buffer,
		   tpm_certify_info2->migrationAuthority.size);
	    rc = TPM_INVALID_STRUCTURE;
	}
    }
    return rc;
}
#endif

/* TPM_CertifyInfo2_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_CertifyInfo2_Store(TPM_STORE_BUFFER *sbuffer,
				 TPM_CERTIFY_INFO2 *tpm_certify_info2)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_CertifyInfo2_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_CERTIFY_INFO2);
    }
    /* store fill */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_certify_info2->fill), sizeof(BYTE));
    }
    /* store payloadType */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_certify_info2->payloadType),
				sizeof(TPM_PAYLOAD_TYPE));
    }
    /* store keyUsage */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, tpm_certify_info2->keyUsage);
    }
    /* store keyFlags */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_certify_info2->keyFlags);
    }
    /* store authDataUsage */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_certify_info2->authDataUsage),
				sizeof(TPM_AUTH_DATA_USAGE)); 
    }
    /* store algorithmParms */
    if (rc == 0) {
	rc = TPM_KeyParms_Store(sbuffer, &(tpm_certify_info2->algorithmParms));
    }
    /* store pubkeyDigest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_certify_info2->pubkeyDigest);
    }
    /* store data */
    if (rc == 0) {
	rc = TPM_Nonce_Store(sbuffer, tpm_certify_info2->data);
    }
    /* store parentPCRStatus */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_certify_info2->parentPCRStatus),
				sizeof(TPM_BOOL));
    }
    /* copy cache to pcrInfo */
    if (rc == 0) {
	rc = TPM_SizedBuffer_SetStructure(&(tpm_certify_info2->pcrInfo),
					  tpm_certify_info2->tpm_pcr_info_short,
					  (TPM_STORE_FUNCTION_T)TPM_PCRInfoShort_Store);
    }
    /* copy pcrInfo to sbuffer */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_certify_info2->pcrInfo));
    }
    /* store migrationAuthority */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_certify_info2->migrationAuthority));
    }
    return rc;
}

/* TPM_CertifyInfo2_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_CertifyInfo2_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_CertifyInfo2_Delete(TPM_CERTIFY_INFO2 *tpm_certify_info2)
{
    printf(" TPM_CertifyInfo2_Delete:\n");
    if (tpm_certify_info2 != NULL) {
	TPM_KeyParms_Delete(&(tpm_certify_info2->algorithmParms));
	/* pcrInfo */
	TPM_SizedBuffer_Delete(&(tpm_certify_info2->pcrInfo));
	/* pcr cache */
	TPM_PCRInfoShort_Delete(tpm_certify_info2->tpm_pcr_info_short);
	free(tpm_certify_info2->tpm_pcr_info_short);
	TPM_SizedBuffer_Delete(&(tpm_certify_info2->migrationAuthority));
	TPM_CertifyInfo2_Init(tpm_certify_info2);
    }
    return;
}

/* TPM_CertifyInfo2_Set() fills in tpm_certify_info2 with the information from the key pointed to by
   tpm_key.

*/

TPM_RESULT TPM_CertifyInfo2_Set(TPM_CERTIFY_INFO2 *tpm_certify_info2,
			       TPM_KEY *tpm_key)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_ASYMKEY	*tpm_store_asymkey;

    printf(" TPM_CertifyInfo_Set:\n");
    /* get the TPM_STORE_ASYMKEY object */
    if (rc == 0) {
	rc = TPM_Key_GetStoreAsymkey(&tpm_store_asymkey, tpm_key);
    }
    if (rc == 0) {
	tpm_certify_info2->payloadType = tpm_store_asymkey->payload;
	tpm_certify_info2->keyUsage = tpm_key->keyUsage;
	tpm_certify_info2->keyFlags = tpm_key->keyFlags;
	tpm_certify_info2->authDataUsage = tpm_key->authDataUsage;
	rc = TPM_Key_GetStoreAsymkey(&tpm_store_asymkey, tpm_key);
    }
    if (rc == 0) {
	rc = TPM_KeyParms_Copy(&(tpm_certify_info2->algorithmParms),
			       &(tpm_key->algorithmParms));
    }
    /* pubkeyDigest SHALL be a digest of the value TPM_KEY -> pubKey -> key in a TPM_KEY
       representation of the key to be certified */
    if (rc == 0) {
	rc = TPM_SHA1(tpm_certify_info2->pubkeyDigest,
		      tpm_key->pubKey.size, tpm_key->pubKey.buffer,
		      0, NULL);
    }
    return rc;
}

/*
  TPM_SYMMETRIC_KEY
*/

/* TPM_SymmetricKey_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_SymmetricKey_Init(TPM_SYMMETRIC_KEY *tpm_symmetric_key)
{
    printf(" TPM_SymmetricKey_Init:\n");
    tpm_symmetric_key->algId = 0;
    tpm_symmetric_key->encScheme = TPM_ES_NONE;
    tpm_symmetric_key->size = 0;
    tpm_symmetric_key->data = NULL;
    return;
}

/* TPM_SymmetricKey_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_SymmetricKey_Init()
   After use, call TPM_SymmetricKey_Delete() to free memory
*/

TPM_RESULT TPM_SymmetricKey_Load(TPM_SYMMETRIC_KEY *tpm_symmetric_key,
				 unsigned char **stream,
				 uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_SymmetricKey_Load:\n");
    /* load algId */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_symmetric_key->algId), stream, stream_size);
    }
    /* load encScheme */
    if (rc == 0) {
	rc = TPM_Load16(&(tpm_symmetric_key->encScheme), stream, stream_size);
    }
    /* load size */
    if (rc == 0) {
	rc = TPM_Load16(&(tpm_symmetric_key->size), stream, stream_size);
    }
    /* allocate memory for the data */
    if ((rc == 0) && (tpm_symmetric_key->size > 0)) {
	rc = TPM_Malloc(&(tpm_symmetric_key->data), tpm_symmetric_key->size);
    }
    /* load data */
    if ((rc == 0) && (tpm_symmetric_key->size > 0)) {
	rc = TPM_Loadn(tpm_symmetric_key->data, tpm_symmetric_key->size, stream, stream_size);
    }
    return rc;
}

/* TPM_SymmetricKey_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_SymmetricKey_Store(TPM_STORE_BUFFER *sbuffer,
				  const TPM_SYMMETRIC_KEY *tpm_symmetric_key)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_SymmetricKey_Store:\n");
    /* store algId */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_symmetric_key->algId);
    }
    /* store encScheme */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, tpm_symmetric_key->encScheme);
    }
    /* NOTE: Cannot use TPM_SizedBuffer_Store since the first parameter is a uint16_t */
    /* store size */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, tpm_symmetric_key->size);
    }
    /* store data */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, tpm_symmetric_key->data, tpm_symmetric_key->size);
    }
    return rc;
}

/* TPM_SymmetricKey_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_SymmetricKey_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_SymmetricKey_Delete(TPM_SYMMETRIC_KEY *tpm_symmetric_key)
{
    printf(" TPM_SymmetricKey_Delete:\n");
    if (tpm_symmetric_key != NULL) {
	free(tpm_symmetric_key->data);
	TPM_SymmetricKey_Init(tpm_symmetric_key);
    }
    return;
}

/* TPM_SymmetricKeyData_EncryptSbuffer() encrypts 'sbuffer' to 'encrypt_data'

   Padding is included, so the output may be larger than the input.

   'encrypt_data' must be free by the caller
*/

TPM_RESULT TPM_SymmetricKeyData_EncryptSbuffer(TPM_SIZED_BUFFER *encrypt_data,
					       TPM_STORE_BUFFER *sbuffer,
					       const TPM_SYMMETRIC_KEY_TOKEN tpm_symmetric_key_data)
{
    TPM_RESULT		rc = 0;
    const unsigned char *decrypt_data;		/* serialization buffer */
    uint32_t		decrypt_data_size;	/* serialization size */

    printf(" TPM_SymmetricKeyData_EncryptSbuffer:\n");
    if (rc == 0) {
	/* get the serialization results */
	TPM_Sbuffer_Get(sbuffer, &decrypt_data, &decrypt_data_size);
	/* platform dependent symmetric key encrypt */
	rc = TPM_SymmetricKeyData_Encrypt(&(encrypt_data->buffer),	/* output, caller frees */
					  &(encrypt_data->size),	/* output */
					  decrypt_data,			/* input */
					  decrypt_data_size,		/* input */
					  tpm_symmetric_key_data);
    }
    return rc;
}

/* TPM_SymmetricKeyData_StreamCrypt() encrypts or decrypts 'data_in' to 'data_out '

   It assumes that the size of data_out and data_in are equal, and that a stream cipher mode is
   used.  For the supported stream ciphers, encrypt and decrypt are equivalent, so no direction flag
   is required.

   AES 128 with CTR or OFB modes are supported.	 For CTR mode, pad is the initial count.  For OFB
   mode, pad is the IV.
*/

TPM_RESULT TPM_SymmetricKeyData_StreamCrypt(unsigned char *data_out,		/* output */
					    const unsigned char *data_in,	/* input */
					    uint32_t data_size,			/* input */
					    TPM_ALGORITHM_ID algId,		/* algorithm */
					    TPM_ENC_SCHEME encScheme,		/* mode */
					    const unsigned char *symmetric_key, /* input */
					    uint32_t symmetric_key_size,	/* input */
					    unsigned char *pad_in,		/* input */
					    uint32_t pad_in_size)		/* input */
{
    TPM_RESULT		rc = 0;

    printf(" TPM_SymmetricKeyData_StreamCrypt:\n");
    switch (algId) {
      case TPM_ALG_AES128:
	switch (encScheme) {
	  case TPM_ES_SYM_CTR:
	    rc = TPM_SymmetricKeyData_CtrCrypt(data_out,
					       data_in,
					       data_size,
					       symmetric_key,
					       symmetric_key_size,
					       pad_in,
					       pad_in_size);
	    break;
	  case TPM_ES_SYM_OFB:
	    rc = TPM_SymmetricKeyData_OfbCrypt(data_out,
					       data_in,
					       data_size,
					       symmetric_key,
					       symmetric_key_size,
					       pad_in,
					       pad_in_size);
	    break;
	  default:
	    printf("TPM_SymmetricKeyData_StreamCrypt: Error, bad AES128 encScheme %04x\n",
		   encScheme);
	    rc = TPM_INAPPROPRIATE_ENC;
	    break;
	}
	break;
      default:
	printf("TPM_SymmetricKeyData_StreamCrypt: Error, bad algID %08x\n", algId);
	rc = TPM_INAPPROPRIATE_ENC;
	break;
    }
    return rc;
}

/* These functions perform high-level, platform independent functions.
   They call the lower level, platform dependent crypto functions in
   tpm_crypto.c
*/

/* TPM_SHA1Sbuffer() calculates the SHA-1 digest of a TPM_STORE_BUFFER.

   This is commonly used when calculating a digest on a serialized structure.  Structures are
   serialized to a TPM_STORE_BUFFER.

   The TPM_STORE_BUFFER is not deleted.
*/

TPM_RESULT TPM_SHA1Sbuffer(TPM_DIGEST tpm_digest,
			   TPM_STORE_BUFFER *sbuffer)
{
    TPM_RESULT		rc = 0;
    const unsigned char *buffer;	/* serialized buffer */
    uint32_t		length;		/* serialization length */

    printf(" TPM_SHA1Sbuffer:\n");
    if (rc == 0) {
	/* get the components of the TPM_STORE_BUFFER */
	TPM_Sbuffer_Get(sbuffer, &buffer, &length);
	TPM_PrintFour("  TPM_SHA1Sbuffer: input", buffer);
	/* hash the serialized buffer to tpm_digest */
	rc = TPM_SHA1(tpm_digest,
		      length, buffer,
		      0, NULL);
    }	 
    return rc;
}

/* TPM_SHA1_GenerateStructure() generates a SHA-1 digest of a structure.  It serializes the
   structure and hashes the result.

   tpmStructure is the structure to be serialized
   storeFunction is the serialization function for the structure
*/

TPM_RESULT TPM_SHA1_GenerateStructure(TPM_DIGEST tpm_digest,
				      void *tpmStructure,
				      TPM_STORE_FUNCTION_T storeFunction)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	sbuffer;	/* serialized tpmStructure */

    printf(" TPM_SHA1_GenerateStructure:\n");
    TPM_Sbuffer_Init(&sbuffer);				/* freed @1 */
    /* Serialize the structure */
    if (rc == 0) {
	rc = storeFunction(&sbuffer, tpmStructure);
    }	 
    /* hash the serialized buffer to tpm_hmac */
    if (rc == 0) {
	rc = TPM_SHA1Sbuffer(tpm_digest, &sbuffer);
    }
    TPM_Sbuffer_Delete(&sbuffer);			/* @1 */
    return rc;
}

/* TPM_SHA1_CheckStructure() generates a SHA-1 digest of a structure.  It serializes the structure
   and hashes the result.  It compares the result to 'expected_digest' and returns 'error' on
   mismatch.

   tpmStructure is the structure to be serialized
   storeFunction is the serialization function for the structure
*/



TPM_RESULT TPM_SHA1_CheckStructure(TPM_DIGEST expected_digest,
				   void *tpmStructure,
				   TPM_STORE_FUNCTION_T storeFunction,
				   TPM_RESULT error)
{
    TPM_RESULT		rc = 0;
    TPM_DIGEST		actual_digest;

    printf(" TPM_SHA1_CheckStructure:\n");
    /* hash the serialized buffer to tpm_digest */
    if (rc == 0) {
	rc = TPM_SHA1_GenerateStructure(actual_digest, tpmStructure, storeFunction);
    }
    /* check the digests */
    if (rc == 0) {
	rc = TPM_Digest_Compare(actual_digest, expected_digest);
	if (rc != 0) {
	    rc = error;
	}
    }
    return rc;
}

/* TPM_SHA1() can be called directly to hash a list of streams.

   The ... arguments to be hashed are a list of the form
	size_t length, unsigned char *buffer
   terminated by a 0 length
 */

TPM_RESULT TPM_SHA1(TPM_DIGEST md, ...)
{
    TPM_RESULT	rc = 0;
    va_list	ap;

    printf(" TPM_SHA1:\n");
    va_start(ap, md);
    rc = TPM_SHA1_valist(md, 0, NULL, ap);
    va_end(ap);
    return rc;
}

/* TPM_SHA1_Check() digests the list of streams and compares the result to 'digest_expect'
 */

TPM_RESULT TPM_SHA1_Check(TPM_DIGEST digest_expect, ...)
{
    TPM_RESULT	rc = 0;
    TPM_DIGEST	digest_actual;
    va_list	ap;

    printf(" TPM_SHA1_Check:\n");
    if (rc == 0) {
	va_start(ap, digest_expect);
	rc = TPM_SHA1_valist(digest_actual, 0, NULL, ap);
	va_end(ap);
    }
    if (rc == 0) {
	rc = TPM_Digest_Compare(digest_expect, digest_actual);
    }
    return rc;
}

/* TPM_SHA1_valist() is the internal function, called with the va_list already created.

   It is called from TPM_SHA1() to do a simple hash.  Typically length0==0 and buffer0==NULL.

   It can also be called from the HMAC function to hash the variable number of input parameters.  In
   that case, the va_list for the text is already formed.  length0 and buffer0 are used to input the
   padded key.
*/

static TPM_RESULT TPM_SHA1_valist(TPM_DIGEST md,
				  uint32_t length0, unsigned char *buffer0,
				  va_list ap)
{
    TPM_RESULT		rc = 0;
    uint32_t		length;
    unsigned char	*buffer;
    void		*context = NULL;	/* platform dependent context */
    TPM_BOOL		done = FALSE;
    
    printf(" TPM_SHA1_valist:\n");
    if (rc == 0) {
	rc = TPM_SHA1InitCmd(&context);
    }
    if (rc == 0) {	
	if (length0 !=0) {		/* optional first text block */
	    printf("  TPM_SHA1_valist: Digesting %u bytes\n", length0);
	    rc = TPM_SHA1UpdateCmd(context, buffer0, length0);	/* hash the buffer */
	}
    }
    while ((rc == 0) && !done) {
	length = va_arg(ap, uint32_t);		/* first vararg is the length */
	if (length != 0) {			/* loop until a zero length argument terminates */
	    buffer = va_arg(ap, unsigned char *);	/* second vararg is the array */
	    printf("  TPM_SHA1_valist: Digesting %u bytes\n", length);
	    rc = TPM_SHA1UpdateCmd(context, buffer, length);	/* hash the buffer */
	}
	else {
	    done = TRUE;
	}
    }
    if (rc == 0) {
	rc = TPM_SHA1FinalCmd(md, context);
    }
    if (rc == 0) {
	TPM_PrintFour("  TPM_SHA1_valist: Digest", md);
    }	 
    /* call TPM_SHA1Delete even if there was an error */
    TPM_SHA1Delete(&context);
    return rc;
}

/* TPM_HMAC_GenerateSbuffer() calculates the HMAC digest of a TPM_STORE_BUFFER.

   This is commonly used when calculating an HMAC on a serialized structure.  Structures are
   serialized to a TPM_STORE_BUFFER.

   The TPM_STORE_BUFFER is not deleted.
*/

TPM_RESULT TPM_HMAC_GenerateSbuffer(TPM_HMAC tpm_hmac,
				    const TPM_SECRET hmac_key,
				    TPM_STORE_BUFFER *sbuffer)
{
    TPM_RESULT		rc = 0;
    const unsigned char *buffer;	/* serialized buffer */
    uint32_t		length;		/* serialization length */

    printf(" TPM_HMAC_GenerateSbuffer:\n");
    if (rc == 0) {
	/* get the components of the TPM_STORE_BUFFER */
	TPM_Sbuffer_Get(sbuffer, &buffer, &length);
	/* HMAC the serialized buffer to tpm_hmac */
	rc = TPM_HMAC_Generate(tpm_hmac,
			       hmac_key,
			       length, buffer,
			       0, NULL);
    }	 
    return rc;
}

/* TPM_HMAC_GenerateStructure() generates an HMAC of a structure.  It serializes the structure and
   HMAC's the result.

   hmacKey is the HMAC key
   tpmStructure is the structure to be serialized
   storeFunction is the serialization function for the structure
*/

TPM_RESULT TPM_HMAC_GenerateStructure(TPM_HMAC tpm_hmac,
				      const TPM_SECRET hmac_key,
				      void *tpmStructure,
				      TPM_STORE_FUNCTION_T storeFunction)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	sbuffer;	/* serialized tpmStructure */

    printf(" TPM_HMAC_GenerateStructure:\n");
    TPM_Sbuffer_Init(&sbuffer);				/* freed @1 */
    /* Serialize the structure */
    if (rc == 0) {
	rc = storeFunction(&sbuffer, tpmStructure);
    }	 
    /* hash the serialized buffer to tpm_hmac */
    if (rc == 0) {
	rc = TPM_HMAC_GenerateSbuffer(tpm_hmac, hmac_key, &sbuffer);
    }
    TPM_Sbuffer_Delete(&sbuffer);			/* @1 */
    return rc;
}
    
/* TPM_HMAC_Generate() can be called directly to HMAC a list of streams.
   
   The ... arguments are a message list of the form
	size_t length, unsigned char *buffer
   terminated by a 0 length
*/

TPM_RESULT TPM_HMAC_Generate(TPM_HMAC tpm_hmac,
			     const TPM_SECRET hmac_key,
			     ...)
{
    TPM_RESULT		rc = 0;
    va_list		ap;
    
    printf(" TPM_HMAC_Generate:\n");
    va_start(ap, hmac_key);
    rc = TPM_HMAC_Generatevalist(tpm_hmac, hmac_key, ap);
    va_end(ap);
    return rc;
}

/* TPM_HMAC_Generatevalist() is the internal function, called with the va_list already created.

   It is called from TPM_HMAC_Generate() and TPM_HMAC_Check() with the va_list for the text already
   formed.
*/

#define TPM_HMAC_BLOCK_SIZE 64

static TPM_RESULT TPM_HMAC_Generatevalist(TPM_HMAC tpm_hmac,
					  const TPM_SECRET key,
					  va_list ap)
{
    TPM_RESULT		rc = 0;
    unsigned char	ipad[TPM_HMAC_BLOCK_SIZE];
    unsigned char	opad[TPM_HMAC_BLOCK_SIZE];
    size_t		i;
    TPM_DIGEST		inner_hash;

    printf(" TPM_HMAC_Generatevalist:\n");
    /* calculate key XOR ipad and key XOR opad */
    if (rc == 0) {
	/* first part, key XOR pad */
	for (i = 0 ; i < TPM_AUTHDATA_SIZE ; i++) {
	    ipad[i] = key[i] ^ 0x36;	/* magic numbers from RFC 2104 */
	    opad[i] = key[i] ^ 0x5c;
	}
	/* second part, 0x00 XOR pad */
	memset(ipad + TPM_AUTHDATA_SIZE, 0x36, TPM_HMAC_BLOCK_SIZE - TPM_AUTHDATA_SIZE);
	memset(opad + TPM_AUTHDATA_SIZE, 0x5c, TPM_HMAC_BLOCK_SIZE - TPM_AUTHDATA_SIZE);
	/* calculate the inner hash, hash the key XOR ipad and the text */
	rc = TPM_SHA1_valist(inner_hash,
			     TPM_HMAC_BLOCK_SIZE, ipad, ap);
    }
    /* hash the key XOR opad and the previous hash */
    if (rc == 0) {
	rc = TPM_SHA1(tpm_hmac,
		      TPM_HMAC_BLOCK_SIZE, opad,
		      TPM_DIGEST_SIZE, inner_hash,
		      0, NULL);
    }
    if (rc == 0) {
	TPM_PrintFour(" TPM_HMAC_Generatevalist: HMAC", tpm_hmac);
    }	 
    return rc;
}

/* TPM_HMAC_CheckSbuffer() checks the HMAC of a TPM_STORE_BUFFER.

   This is commonly used when checking an HMAC on a serialized structure.  Structures are serialized
   to a TPM_STORE_BUFFER.

   The TPM_STORE_BUFFER is not deleted.
*/

TPM_RESULT TPM_HMAC_CheckSbuffer(TPM_BOOL *valid,			/* result */
				 TPM_HMAC expect,		/* expected */
				 const TPM_SECRET hmac_key,	/* key */
				 TPM_STORE_BUFFER *sbuffer)	/* data stream */
{
    TPM_RESULT		rc = 0;
    const unsigned char *buffer;	/* serialized buffer */
    uint32_t		length;		/* serialization length */

    printf(" TPM_HMAC_CheckSbuffer:\n");
    if (rc == 0) {
	/* get the components of the TPM_STORE_BUFFER */
	TPM_Sbuffer_Get(sbuffer, &buffer, &length);
	/* HMAC the serialized buffer to tpm_hmac */
	rc = TPM_HMAC_Check(valid,
			    expect,
			    hmac_key,
			    length, buffer,
			    0, NULL);
    }	 
    return rc;
}

/* TPM_HMAC_Check() can be called directly to check the HMAC of a list of streams.
   
   The ... arguments are a list of the form
	   size_t length, unsigned char *buffer
   terminated by a 0 length

*/

TPM_RESULT TPM_HMAC_Check(TPM_BOOL *valid,
			  TPM_HMAC expect,
			  const TPM_SECRET key,
			  ...)
{
    TPM_RESULT		rc = 0;
    va_list		ap;
    TPM_HMAC		actual;
    int			result;

    printf(" TPM_HMAC_Check:\n");
    va_start(ap, key);
    if (rc == 0) {
	rc = TPM_HMAC_Generatevalist(actual, key, ap);
    }
    if (rc == 0) {
	TPM_PrintFour("  TPM_HMAC_Check: Calculated", actual);
	TPM_PrintFour("  TPM_HMAC_Check: Received  ", expect);
	result = memcmp(expect, actual, TPM_DIGEST_SIZE);
	if (result == 0) {
	    *valid = TRUE;
	}
	else {
	    *valid = FALSE;
	}
    }
    va_end(ap);
    return rc;
}

/* TPM_HMAC_CheckStructure() is a generic function that checks the integrity HMAC of a structure.

   hmacKey is the HMAC key
   tpmStructure is the structure to be serialized
   expect is the expected HMAC, a member of the structure
   storeFunction is the serialization function for the structure
   error is the failure return code

   The function saves a copy of the expected HMAC, and then NULL's the structure member.  It
   serializes the structure, generates an HMAC, and compares it to the expected value.

   As a side effect, the structure member is zeroed.
*/

TPM_RESULT TPM_HMAC_CheckStructure(const TPM_SECRET hmac_key,
				   void *tpmStructure,
				   TPM_HMAC expect,
				   TPM_STORE_FUNCTION_T storeFunction,
				   TPM_RESULT error)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	sbuffer;	/* serialized tpmStructure */
    TPM_HMAC		saveExpect;
    TPM_BOOL		valid;

    printf(" TPM_HMAC_CheckStructure:\n");
    TPM_Sbuffer_Init(&sbuffer);			/* freed @1 */
    if (rc == 0) {
	TPM_Digest_Copy(saveExpect, expect);	/* save the expected value */
	TPM_Digest_Init(expect);		/* set value in structure to NULL */
	rc = storeFunction(&sbuffer,
			   tpmStructure);
    }
    /* verify the HMAC of the serialized structure */
    if (rc == 0) {
	rc = TPM_HMAC_CheckSbuffer(&valid,		/* result */
				   saveExpect,		/* expected */
				   hmac_key,		/* key */
				   &sbuffer);		/* data stream */
    }
    if (rc == 0) {
	if (!valid) {
	    printf("TPM_HMAC_CheckStructure: Error checking HMAC\n");
	    rc = error;
	}
    }
    TPM_Sbuffer_Delete(&sbuffer);		/* @1 */
    return rc;
}

/* TPM_XOR XOR's 'in1' and 'in2' of 'length', putting the result in 'out'

*/

void TPM_XOR(unsigned char *out,
	     const unsigned char *in1,
	     const unsigned char *in2,
	     size_t length)
{
    size_t i;
    
    for (i = 0 ; i < length ; i++) {
	out[i] = in1[i] ^ in2[i];
    }
    return;
}

/* TPM_MGF1() generates an MGF1 'array' of length 'arrayLen' from 'seed' of length 'seedlen'

   The openSSL DLL doesn't export MGF1 in Windows or Linux 1.0.0, so this version is created from
   scratch.
   
   Algorithm and comments (not the code) from:

   PKCS #1: RSA Cryptography Specifications Version 2.1 B.2.1 MGF1

   Prototype designed to be compatible with openSSL

   MGF1 is a Mask Generation Function based on a hash function.
   
   MGF1 (mgfSeed, maskLen)

   Options:     

   Hash hash function (hLen denotes the length in octets of the hash 
   function output)

   Input:
   
   mgfSeed         seed from which mask is generated, an octet string
   maskLen         intended length in octets of the mask, at most 2^32(hLen)

   Output:      
   mask            mask, an octet string of length l; or "mask too long"

   Error:          "mask too long'
*/

TPM_RESULT TPM_MGF1(unsigned char       *mask,
                    uint32_t            maskLen,
                    const unsigned char *mgfSeed,
                    uint32_t		mgfSeedlen)
{
    TPM_RESULT 		rc = 0;
    unsigned char       counter[4];     /* 4 octets */
    uint32_t	        count;          /* counter as an integral type */
    uint32_t		outLen;
    TPM_DIGEST          lastDigest;     
    
    printf(" TPM_MGF1: Output length %u\n", maskLen);
    if (rc == 0) {
        /* this is possible with arrayLen on a 64 bit architecture, comment to quiet beam */
        if ((maskLen / TPM_DIGEST_SIZE) > 0xffffffff) {        /*constant condition*/
            printf(" TPM_MGF1: Error (fatal), Output length too large for 32 bit counter\n");
            rc = TPM_FAIL;              /* should never occur */
        }
    }
    /* 1.If l > 2^32(hLen), output "mask too long" and stop. */
    /* NOTE Checked by caller */
    /* 2. Let T be the empty octet string. */
    /* 3. For counter from 0 to [masklen/hLen] - 1, do the following: */
    for (count = 0, outLen = 0 ; (rc == 0) && (outLen < maskLen) ; count++) {
	/* a. Convert counter to an octet string C of length 4 octets - see Section 4.1 */
	/* C = I2OSP(counter, 4) NOTE Basically big endian */
        uint32_t count_n = htonl(count);
	memcpy(counter, &count_n, 4);
	/* b.Concatenate the hash of the seed mgfSeed and C to the octet string T: */
	/* T = T || Hash (mgfSeed || C) */
	/* If the entire digest is needed for the mask */
	if ((outLen + TPM_DIGEST_SIZE) < maskLen) {
	    rc = TPM_SHA1(mask + outLen,
			  mgfSeedlen, mgfSeed,
			  4, counter,
			  0, NULL);
	    outLen += TPM_DIGEST_SIZE;
	}
	/* if the mask is not modulo TPM_DIGEST_SIZE, only part of the final digest is needed */
	else {
	    /* hash to a temporary digest variable */
	    rc = TPM_SHA1(lastDigest,
			  mgfSeedlen, mgfSeed,
			  4, counter,
			  0, NULL);
	    /* copy what's needed */
	    memcpy(mask + outLen, lastDigest, maskLen - outLen);
	    outLen = maskLen;           /* outLen = outLen + maskLen - outLen */
	}
    }
    /* 4.Output the leading l octets of T as the octet string mask. */
    return rc;
}

/* TPM_MGF1_GenerateArray() generates an array of length arrayLen using the varargs as the seed.

   Since the seed is a known length, it is passed in rather that extracted from the varargs.  If the
   seed length turns out to be wrong once the varargs are parsed, TPM_FAIL is returned.

   'array' must be freed by the caller.
*/

TPM_RESULT TPM_MGF1_GenerateArray(unsigned char **array,
				  uint32_t arrayLen,
				  uint32_t seedLen,
				  ...)
{
    TPM_RESULT		rc = 0;
    va_list		ap;
    unsigned char	*seed;		/* constructed MGF1 seed */
    size_t		vaLength;	/* next seed segment length */
    unsigned char	*vaBuffer;	/* next seed segment buffer */
    uint32_t		seedLeft;	/* remaining seed bytes required */
    unsigned char	*seedBuffer;	/* running pointer to the seed array */
    TPM_BOOL		done = FALSE;	/* done when a vaLength == 0 is reached */

    printf(" TPM_MGF1_GenerateArray: arrayLen %u seedLen %u\n", arrayLen, seedLen);
    seed = NULL;		/* freed @1 */
    *array = NULL;		/* freed by caller */
    va_start(ap, seedLen);
    /* allocate temporary memory for the seed */
    if (rc == 0) {
	rc = TPM_Malloc(&seed, seedLen);
	seedBuffer = seed;
	seedLeft = seedLen;
    }
    /* construct the seed */
    while ((rc == 0) && !done) {
	vaLength = (size_t)va_arg(ap, uint32_t);		/* first vararg is the length */
	if (vaLength != 0) {			/* loop until a zero length argument terminates */
	    if (rc == 0) {
		printf("  TPM_MGF1_GenerateArray: Appending %lu bytes\n", (unsigned long)vaLength);
		if (vaLength > seedLeft) {
		    printf("TPM_MGF1_GenerateArray: Error (fatal), seedLen too small\n");
		    rc = TPM_FAIL;	/* internal error, should never occur */
		}
	    }
	    if (rc == 0) {
		vaBuffer = va_arg(ap, unsigned char *); /* second vararg is the array */
		memcpy(seedBuffer, vaBuffer, vaLength);
		seedBuffer += vaLength;
		seedLeft-= vaLength;
	    }
	}
	else {
	    done = TRUE;
	    if (seedLeft != 0) {
		printf("TPM_MGF1_GenerateArray: Error (fatal), seedLen too large by %u\n",
		       seedLeft);
		rc = TPM_FAIL;	/* internal error, should never occur */
	    }
	}
    }
    /* allocate memory for the array */
    if (rc == 0) {
	rc = TPM_Malloc(array, arrayLen);
    }
    /* generate the MGF1 array */
    if (rc == 0) {
	TPM_MGF1(*array,
		 arrayLen,
		 seed,
		 seedLen);
	TPM_PrintFour("  TPM_MGF1_GenerateArray: MGF1", *array);
    }
    va_end(ap);
    free(seed);		/* @1 */
    return rc;
}

/* TPM_bn2binMalloc() allocates a buffer 'bin' and loads it from 'bn'.
   'bytes' is set to the allocated size of 'bin'.

   If padBytes is non-zero, 'bin' is padded with leading zeros if necessary, so that 'bytes' will
   equal 'padBytes'.  This is used when TPM data structures expect a fixed length while the crypto
   library 'bn to bin' function might truncates leading zeros.

   '*bin' must be freed by the caller
*/

TPM_RESULT TPM_bn2binMalloc(unsigned char **bin,	/* freed by caller */
			    unsigned int *bytes,
			    TPM_BIGNUM bn,
			    uint32_t padBytes)
{
    TPM_RESULT  rc = 0;

    printf("   TPM_bn2binMalloc: padBytes %u\n", padBytes);
    /* number of bytes required in the bin array */
    if (rc == 0) {
        rc = TPM_BN_num_bytes(bytes, bn);
    }
    /* calculate the array size to malloc */
    if (rc == 0) {
        /* padBytes 0 says that no padding is required */
        if (padBytes == 0) {
            padBytes = *bytes;  /* setting equal yields no padding */
        }       
	/* if the array with padding is still less than the number of bytes required by the bignum,
	   this function fails */
        if (padBytes < *bytes) {
            printf("TPM_bn2binMalloc: Error, "
                   "padBytes %u less than BN bytes %u\n", padBytes, *bytes);
            rc = TPM_SIZE;
        }
	/* log if padding is occurring */
        if (padBytes != *bytes) {
            printf("   TPM_bn2binMalloc: padBytes %u bytes %u\n", padBytes, *bytes);
        }
    }
    /* allocate for the padded array */
    if (rc == 0) {
        rc = TPM_Malloc(bin, padBytes);
	*bytes = padBytes;
    }
    /* call the bignum to bin conversion */
    if (rc == 0) {
	rc = TPM_bn2binArray(*bin, padBytes, bn);
    }
    return rc;
}

/* TPM_bn2binArray() loads the array 'bin' of size 'bytes' from 'bn'

   The data from 'bn' is right justified and zero padded.
*/

TPM_RESULT TPM_bn2binArray(unsigned char *bin,
			   unsigned int bytes,
			   TPM_BIGNUM bn)
{
    TPM_RESULT		rc = 0;
    unsigned int	numBytes;

    printf("   TPM_bn2binArray: size %u\n", bytes);
    if (rc == 0) {
	/* zero pad */
	memset(bin, 0, bytes);
	/* bytes required for the bignum */
	rc = TPM_BN_num_bytes(&numBytes, bn);
    }
    /* if the array is less than the number of bytes required by the bignum, this function fails */
    if (rc == 0) {
	printf("   TPM_bn2binArray: numBytes in bignum %u\n", numBytes);
	if (numBytes > bytes) {
            printf("TPM_bn2binArray: Error, "
                   "BN bytes %u greater than array bytes %u\n", numBytes, bytes);
            rc = TPM_SIZE;
	}
    }
    if (rc == 0) {
	/* if there are bytes in the bignum (it is not zero) */
	if (numBytes  > 0) {
	    rc = TPM_bn2bin(bin + bytes - numBytes,	/* store right justified */
			    bn);
	}
    }
    return rc;
}

/* TPM_2bin2bn() converts two byte arrays to a positive BIGNUM.

   The two byte arrays are concatenated.  The concatenation is used to create the BIGNUM.

   bignum must be freed by the caller.
*/

TPM_RESULT TPM_2bin2bn(TPM_BIGNUM *bignum_in,         /* freed by caller */
                       const unsigned char *bin0, uint32_t size0,
                       const unsigned char *bin1, uint32_t size1)
{
    TPM_RESULT          rc = 0;         /* TPM return code */
    TPM_STORE_BUFFER    sBuffer;        /* used of >1 element or first element is negative */
    const unsigned char *buffer;
    uint32_t		size;
    
    printf("  TPM_bin2bn:\n");
    TPM_Sbuffer_Init(&sBuffer);         /* freed @1 */
    /* append the first element */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(&sBuffer, bin0, size0);
    }
    /* append the next element */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(&sBuffer, bin1, size1);
    }
    /* create the BIGNUM from the array */
    if (rc == 0) {
        TPM_Sbuffer_Get(&sBuffer, &buffer, &size);
        /* create the BIGNUM */
        rc = TPM_bin2bn(bignum_in, buffer, size);  /* freed by caller */
    }
    TPM_Sbuffer_Delete(&sBuffer);               /* @1 */
    return rc;
}

/* TPM_RSAPrivateDecryptMalloc() allocates a buffer 'decrypt_data' of size 'decrypt_data_size'
   and then calls TPM_RSAPrivateDecryptH().
*/

TPM_RESULT TPM_RSAPrivateDecryptMalloc(unsigned char **decrypt_data,	/* decrypted data */
				       uint32_t *decrypt_data_length,	/* length of data put into
									   decrypt_data */
				       unsigned char *encrypt_data,
				       uint32_t encrypt_data_size,
				       TPM_KEY *tpm_key)
{
    TPM_RESULT		rc = 0;

    /* allocate space for the decrypted blob */
    printf(" TPM_RSAPrivateDecryptMalloc: Return max data size %u bytes\n",
	   tpm_key->pubKey.size);
    if (rc == 0) {
	rc = TPM_Malloc(decrypt_data, tpm_key->pubKey.size);
    }
    if (rc == 0) {
	rc = TPM_RSAPrivateDecryptH(*decrypt_data,
				    decrypt_data_length,
				    tpm_key->pubKey.size,
				    encrypt_data,
				    encrypt_data_size,
				    tpm_key);
    }
    return rc;
}

/* TPM_RSAPrivateDecryptH() decrypts 'encrypt_data' using the private key in
   'tpm_key' and 'decrypt_data_length' bytes are moved to 'decrypt_data'.

   'decrypt_data_length' is at most 'decrypt_data_size'.
*/

TPM_RESULT TPM_RSAPrivateDecryptH(unsigned char *decrypt_data,	/* decrypted data */
				  uint32_t *decrypt_data_length,	/* length of data put into
									   decrypt_data */
				  uint32_t decrypt_data_size,	/* size of decrypt_data buffer */
				  unsigned char *encrypt_data,
				  uint32_t encrypt_data_size,
				  TPM_KEY *tpm_key)
{
    TPM_RESULT		rc = 0;
    unsigned char	*narr;		/* public modulus */
    uint32_t		nbytes;
    unsigned char	*earr;		/* public exponent */
    uint32_t		ebytes;
    unsigned char	*darr;		/* private exponent */
    uint32_t		dbytes;

    printf(" TPM_RSAPrivateDecryptH: Data size %u bytes\n", encrypt_data_size);
    TPM_PrintFourLimit("  TPM_RSAPrivateDecryptH: Encrypt data", encrypt_data, encrypt_data_size);
    if (rc == 0) {
	if (tpm_key == NULL) {
	    printf("TPM_RSAPrivateDecryptH: Error, NULL key\n");
	    rc = TPM_DECRYPT_ERROR;
	}
    }
    /* extract the public key from TPM_KEY */
    if (rc == 0) {
	rc = TPM_Key_GetPublicKey(&nbytes, &narr, tpm_key);
    }	
    /* extract the private key from TPM_KEY */
    if (rc == 0) {
	rc = TPM_Key_GetPrivateKey(&dbytes, &darr, tpm_key);
    }
    /* extract the exponent from TPM_KEY */
    if (rc == 0) {
	rc = TPM_Key_GetExponent(&ebytes, &earr, tpm_key);
    }
    /* check the key size vs the data size */
    if (rc == 0) {
	if (encrypt_data_size > nbytes) {
	    printf("TPM_RSAPrivateDecryptH: Error, data size too long for key size %u bytes\n",
		   nbytes);
	    rc = TPM_BAD_DATASIZE;
	}
    }
    if (rc == 0) {
	/* debug printing */
	printf("  TPM_RSAPrivateDecryptH: Public key length %u\n", nbytes);
	printf("  TPM_RSAPrivateDecryptH: Private key length %u\n", dbytes);
	TPM_PrintFour("  TPM_RSAPrivateDecryptH: Public key", narr);
	printf("  TPM_RSAPrivateDecryptH: Exponent %02x %02x %02x\n", earr[0], earr[1], earr[2]);
	TPM_PrintFour("  TPM_RSAPrivateDecryptH: Private key", darr);
	/* decrypt with private key */
	rc = TPM_RSAPrivateDecrypt(decrypt_data,	/* decrypted data */
				   decrypt_data_length, /* length of data put into decrypt_data */
				   decrypt_data_size,	/* size of decrypt_data buffer */
				   tpm_key->algorithmParms.encScheme,	/* encryption scheme */
				   encrypt_data,	/* encrypted data */
				   encrypt_data_size,
				   narr,		/* public modulus */
				   nbytes,
				   earr,		/* public exponent */
				   ebytes,
				   darr,		/* private exponent */
				   dbytes);
    }
    if (rc == 0) {
	TPM_PrintFourLimit(" TPM_RSAPrivateDecryptH: Decrypt data", decrypt_data, *decrypt_data_length);
    }
    return rc;
}

/* TPM_RSAPublicEncryptSbuffer_Key() encrypts 'sbuffer' using the public key in 'tpm_key' and
   puts the results in 'encData'
*/

TPM_RESULT TPM_RSAPublicEncryptSbuffer_Key(TPM_SIZED_BUFFER *enc_data,
					   TPM_STORE_BUFFER *sbuffer,
					   TPM_KEY *tpm_key)
{
    TPM_RESULT		rc = 0;
    const unsigned char *decrypt_data;		/* serialization buffer */
    uint32_t		decrypt_data_size;	/* serialization size */

    printf(" TPM_RSAPublicEncryptSbuffer_Key:\n");
    /* get the serialization results */
    TPM_Sbuffer_Get(sbuffer, &decrypt_data, &decrypt_data_size);
    /* encrypt the serialization buffer with the public key, and place
       the result in the enc_data buffer */
    rc = TPM_RSAPublicEncrypt_Key(enc_data,
				  decrypt_data,
				  decrypt_data_size,
				  tpm_key);
    return rc;
}

/* TPM_RSAPublicEncrypt_Key() encrypts 'buffer' of 'length' using the public key in 'tpm_key' and
   puts the results in 'encData'
*/

TPM_RESULT TPM_RSAPublicEncrypt_Key(TPM_SIZED_BUFFER *enc_data,
				    const unsigned char *decrypt_data,
				    size_t decrypt_data_size,
				    TPM_KEY *tpm_key)
{
    TPM_RESULT		rc = 0;
    unsigned char	*narr;		 /* public modulus */
    uint32_t		nbytes;
    unsigned char	*earr;		 /* public exponent */
    uint32_t		ebytes;
    
    printf(" TPM_RSAPublicEncrypt_Key: Data size %lu bytes\n", (unsigned long)decrypt_data_size);
    if (rc == 0) {
	if (tpm_key == NULL) {
	    printf("TPM_RSAPublicEncrypt_Key: Error, NULL key\n");
	    rc = TPM_ENCRYPT_ERROR;
	}
    }
    /* extract the public key from TPM_KEY */
    if (rc == 0) {
	rc = TPM_Key_GetPublicKey(&nbytes, &narr, tpm_key);
    }
    /* extract the exponent from TPM_KEY */
    if (rc == 0) {
	rc = TPM_Key_GetExponent(&ebytes, &earr, tpm_key);
    }
    if (rc == 0) {
	rc = TPM_RSAPublicEncrypt_Common(enc_data,
					 decrypt_data,
					 decrypt_data_size,
					 tpm_key->algorithmParms.encScheme, /* encryption scheme */
					 narr,
					 nbytes,
					 earr,
					 ebytes);
    }
    return rc;
}

/* TPM_RSAPublicEncrypt_Key() encrypts 'buffer' of 'length' using the public key in 'tpm_pubkey' and
   puts the results in 'encData'
*/

TPM_RESULT TPM_RSAPublicEncrypt_Pubkey(TPM_SIZED_BUFFER *enc_data,
				       const unsigned char *decrypt_data,
				       size_t decrypt_data_size,
				       TPM_PUBKEY *tpm_pubkey)
{
    TPM_RESULT		rc = 0;
    unsigned char	*narr;		 /* public modulus */
    uint32_t		nbytes;
    unsigned char	*earr;		 /* public exponent */
    uint32_t		ebytes;
    
    printf(" TPM_RSAPublicEncrypt_Pubkey: Data size %lu bytes\n", (unsigned long)decrypt_data_size);
    if (rc == 0) {
	if (tpm_pubkey == NULL) {
	    printf("TPM_RSAPublicEncrypt_Pubkey: Error, NULL key\n");
	    rc = TPM_ENCRYPT_ERROR;
	}
    }
    /* extract the public key from TPM_PUBKEY */
    if (rc == 0) {
	rc = TPM_Pubkey_GetPublicKey(&nbytes, &narr, tpm_pubkey);
    }
    /* extract the exponent from TPM_KEY */
    if (rc == 0) {
	rc = TPM_Pubkey_GetExponent(&ebytes, &earr, tpm_pubkey);
    }
    if (rc == 0) {
	rc = TPM_RSAPublicEncrypt_Common(enc_data,
					 decrypt_data,
					 decrypt_data_size,
					 tpm_pubkey->algorithmParms.encScheme,
					 narr,
					 nbytes,
					 earr,
					 ebytes);
    }
    return rc;
}

/* TPM_RSAPublicEncrypt_Key() encrypts 'buffer' of 'length' using the public key modulus and
   exponent, and puts the results in 'encData'
*/

TPM_RESULT TPM_RSAPublicEncrypt_Common(TPM_SIZED_BUFFER *enc_data,
				       const unsigned char *decrypt_data,
				       size_t decrypt_data_size,
				       TPM_ENC_SCHEME encScheme,
				       unsigned char	*narr,		 /* public modulus */
				       uint32_t 	nbytes,
				       unsigned char	*earr,		 /* public exponent */
				       uint32_t 	ebytes)

{
    TPM_RESULT		rc = 0;
    unsigned char	*encrypt_data = NULL;
    
    printf(" TPM_RSAPublicEncrypt_Common: Data size %lu bytes\n", (unsigned long)decrypt_data_size);
    TPM_PrintFourLimit(" TPM_RSAPublicEncrypt_Common: Decrypt data", decrypt_data, decrypt_data_size);
    /* check the key size vs the data size */
    if (rc == 0) {
	if (decrypt_data_size > nbytes) {
	    printf("TPM_RSAPublicEncrypt_Common: Error, data size too long for key size %u bytes\n",
		   nbytes);
	    rc = TPM_BAD_DATASIZE;
	}
    }
    /* allocate an array for the encrypted data */
    if (rc == 0) {
	rc = TPM_Malloc(&encrypt_data, nbytes);
    }
    /* pad and encrypt the data */
    if (rc == 0) {
	TPM_PrintFour(" TPM_RSAPublicEncrypt_Common: Public key", narr);
	printf(" TPM_RSAPublicEncrypt_Common: Exponent %02x %02x %02x\n",
	       earr[0], earr[1], earr[2]);
	rc = TPM_RSAPublicEncrypt(encrypt_data,		/* encrypted data */
				  nbytes,		/* encrypted data size */
				  encScheme,		/* encryption scheme */
				  decrypt_data,		/* decrypted data */
				  decrypt_data_size,
				  narr,			/* public modulus */
				  nbytes,
				  earr,			/* public exponent */
				  ebytes);
    }
    /* copy the result to the sized buffer */
    if (rc == 0) {
	printf("  TPM_RSAPublicEncrypt_Common: Encrypt data size %u\n", nbytes);
	TPM_PrintFour(" TPM_RSAPublicEncrypt_Common: Encrypt data", encrypt_data);
	rc = TPM_SizedBuffer_Set(enc_data, nbytes, encrypt_data);
    }
    free(encrypt_data); /* @1 */
    return rc;
}

/*
  Signing Functions

  These commands show the TPM command and the allowed signature schemes:

				SHA	DER	INFO
  TPM_GetAuditDigestSigned	y	n	y
  TPM_CertifyKey		y	n	y
  TPM_CertifyKey2		y	n	y
  TPM_CertifySelfTest		y	n	y
  TPM_Quote			y	n	y
  TPM_Quote2			y	n	y
  TPM_Sign			y	y	y
  TPM_MakeIdentity		y	n	y
  TPM_GetCapabilitySigned	y	n	y
*/

/* TPM_RSASignToSizedBuffer() signs 'message' using the private key in 'tpm_key' and places the
   result in 'signature'

   'signature' should be initialized and deleted by the caller
*/

TPM_RESULT TPM_RSASignToSizedBuffer(TPM_SIZED_BUFFER *signature,
				    const unsigned char *message,	/* input */
				    size_t message_size,		/* input */
				    TPM_KEY *tpm_key)		/* input, signing key */
{
    TPM_RESULT		rc = 0;
    TPM_RSA_KEY_PARMS	*rsa_key_parms;
    unsigned int	signature_length;
    
    printf(" TPM_RSASignToSizedBuffer: Message size %lu bytes\n", (unsigned long)message_size);
    if (rc == 0) {
	rc = TPM_KeyParms_GetRSAKeyParms(&rsa_key_parms,
					 &(tpm_key->algorithmParms));
    }
    /* allocating space for the signature */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Allocate(signature, (rsa_key_parms->keyLength)/CHAR_BIT);
    }
    /* sign */
    if (rc == 0) {
	rc = TPM_RSASignH(signature->buffer,	/* output signature */
			  &signature_length,	/* output, size of signature */
			  signature->size,	/* input, size of signature buffer */
			  message,		/* message */
			  message_size,		/* message size */
			  tpm_key);		/* input, signing key */
    }
    /* sanity check on signature */
    if (rc == 0) {
	if (signature_length != signature->size) {
	    printf("TPM_RSASignToSizedBuffer: Error (fatal) signature_length %u sigSize %u\n",
		   signature_length, signature->size);
	    rc = TPM_FAIL;	/* internal error, should never occur */
	}
    }
    return rc;
}
    

/* TPM_RSASignH() signs 'message' using the private key in 'tpm_key'.  'signature_length' bytes are
   moved to 'signature'.

   'signature_length' is at most 'signature_size'.
*/

TPM_RESULT TPM_RSASignH(unsigned char *signature,		/* output */
			unsigned int *signature_length,		/* output, size of signature */
			unsigned int signature_size,	/* input, size of signature buffer */
			const unsigned char *message,	/* input */
			size_t message_size,		/* input */
			TPM_KEY *tpm_key)		/* input, signing key */
{
    TPM_RESULT		rc = 0;
    unsigned char	*narr;		 /* public modulus */
    uint32_t		nbytes;
    unsigned char	*earr;		 /* public exponent */
    uint32_t		ebytes;
    unsigned char	*darr;		/* private exponent */
    uint32_t		dbytes;
    
    printf(" TPM_RSASignH: Message size %lu bytes\n", (unsigned long)message_size);
    TPM_PrintFourLimit("  TPM_RSASignH: Message", message, message_size);
    /* extract the public key from TPM_KEY */
    if (rc == 0) {
	rc = TPM_Key_GetPublicKey(&nbytes, &narr, tpm_key);
    }	
    /* extract the private key from TPM_KEY */
    if (rc == 0) {
	rc = TPM_Key_GetPrivateKey(&dbytes, &darr, tpm_key);
    }
    /* extract the exponent from TPM_KEY */
    if (rc == 0) {
	rc = TPM_Key_GetExponent(&ebytes, &earr, tpm_key);
    }
    if (rc == 0) {
	/* debug printing */
	TPM_PrintFour("  TPM_RSASignH: Public key", narr);
	printf("  TPM_RSASignH: Exponent %02x %02x %02x\n", earr[0], earr[1], earr[2]);
	TPM_PrintFour("  TPM_RSASignH: Private key", darr);
	/* sign with private key */
	rc = TPM_RSASign(signature,		/* output */
			 signature_length,	/* output, size of signature */
			 signature_size,	/* input, size of signature buffer */
			 tpm_key->algorithmParms.sigScheme,	/* input, type of signature */
			 message,	/* input */
			 message_size,	/* input */
			 narr,		/* public modulus */
			 nbytes,
			 earr,		/* public exponent */
			 ebytes,
			 darr,		/* private exponent */
			 dbytes);
    }
    if (rc == 0) {
	TPM_PrintFour("  TPM_RSASignH: Signature", signature);
    }
    return rc;
}

/* TPM_RSAVerifyH() verifies 'message' using the TPM format public key in 'tpm_pubkey'
*/

TPM_RESULT TPM_RSAVerifyH(TPM_SIZED_BUFFER *signature,	/* input */
			  const unsigned char *message, /* input */
			  uint32_t message_size,	/* input */
			  TPM_PUBKEY *tpm_pubkey)	/* input, verify key */
{
    TPM_RESULT		rc = 0;
    unsigned char	*narr;		 /* public modulus */
    uint32_t		nbytes;
    unsigned char	*earr;		 /* public exponent */
    uint32_t		ebytes;

    printf(" TPM_RSAVerifyH: Message size %u bytes\n", message_size);
    /* extract the public key from TPM_PUBKEY */
    if (rc == 0) {
	rc = TPM_Pubkey_GetPublicKey(&nbytes, &narr, tpm_pubkey);
    }
    /* extract the exponent from TPM_PUBKEY */
    if (rc == 0) {
	rc = TPM_Pubkey_GetExponent(&ebytes, &earr, tpm_pubkey);
    }
    if (rc == 0) {
	/* debug printing */
	TPM_PrintFour("  TPM_RSAVerifyH: Public key", narr);
        TPM_PrintAll("  TPM_RSAVerifyH: Public exponent", earr, ebytes);
	/* verify with public key */
	rc = TPM_RSAVerify(signature->buffer,	/* input signature buffer */
			   signature->size,	/* input, size of signature buffer */
			   tpm_pubkey->algorithmParms.sigScheme, /* input, type of signature */
			   message,		/* message */
			   message_size,	/* message size */
			   narr,		/* public modulus */
			   nbytes,
			   earr,		/* public exponent */
			   ebytes);
    }
    return rc;
}

/* TPM_RSAVerify() verifies the 'signature' of size 'signature_size' on the 'message' of size
   'message_size' using the public key n,e and the signature scheme 'sigScheme' as specified in PKCS
   #1 v2.0.
*/

TPM_RESULT TPM_RSAVerify(unsigned char *signature,      /* input */
                         unsigned int 	signature_size, /* input, size of signature buffer */
                         TPM_SIG_SCHEME sigScheme,      /* input, type of signature */
                         const unsigned char *message,  /* input */
                         uint32_t 	message_size,   /* input */
                         unsigned char *narr,           /* public modulus */
                         uint32_t 	nbytes,
                         unsigned char *earr,           /* public exponent */
                         uint32_t 	ebytes)
{
    TPM_RESULT  rc = 0;
    
    printf(" TPM_RSAVerify:\n");
    /* determine the signature scheme for the key */
    if (rc == 0) {
        switch(sigScheme) {
          case TPM_SS_NONE:
            printf("TPM_RSAVerify: Error, sigScheme TPM_SS_NONE\n");
            rc = TPM_INVALID_KEYUSAGE;
            break;
          case TPM_SS_RSASSAPKCS1v15_SHA1:
          case TPM_SS_RSASSAPKCS1v15_INFO:
            rc = TPM_RSAVerifySHA1(signature,
                                   signature_size,
                                   message,
                                   message_size,
				   narr,		/* public modulus */
				   nbytes,
				   earr,		/* public exponent */
				   ebytes);
            break;
          case TPM_SS_RSASSAPKCS1v15_DER:
            printf("TPM_RSAVerify: Error, sigScheme %04hx unsupported\n", sigScheme);
            rc = TPM_INVALID_KEYUSAGE;
            break;
          default:
            printf("TPM_RSAVerify: Error, sigScheme %04hx unknown\n", sigScheme);
            rc = TPM_INVALID_KEYUSAGE;
            break;
        }
    }
    return rc;
}

/*
  OAEP Padding 
*/

/* TPM_RSA_padding_add_PKCS1_OAEP() is a variation of the the openSSL function

   int RSA_padding_add_PKCS1_OAEP(unsigned char *to, int tlen,
   unsigned char *f, int fl, unsigned char *p, int pl);

   It is used for TPM migration.  The "encoding parameter" pl is replaced by pHash and the generated
   random seed is replaced by a seed parameter.

   This function was independently written from the PKCS1 specification "9.1.1.1 Encoding
   Operation", intended to be unencumbered by any license.


   | seed | pHash |	   PS	     | 01 |	     Message	      |

   SHA1	  SHA1					    flen

   | <-			  emLen					   -> |
   | db
   | maskDb
   |		    dbMask				       |
   | seedMask
   | maskSeed
*/

TPM_RESULT TPM_RSA_padding_add_PKCS1_OAEP(unsigned char *em, uint32_t emLen,
					  const unsigned char *from, uint32_t fLen,
					  const unsigned char *pHash,		/* input 20 bytes */
					  const unsigned char *seed)		/* input 20 bytes */
{	
    TPM_RESULT	rc = 0;
    unsigned char *dbMask;
    unsigned char *db;
    unsigned char *maskedDb;
    unsigned char *seedMask;
    unsigned char *maskedSeed;

    printf(" TPM_RSA_padding_add_PKCS1_OAEP: fLen %d emLen %d\n", fLen, emLen);
    TPM_PrintFourLimit("  TPM_RSA_padding_add_PKCS1_OAEP: from", from, fLen);
    TPM_PrintFour("  TPM_RSA_padding_add_PKCS1_OAEP: pHash", pHash);
    TPM_PrintFour("  TPM_RSA_padding_add_PKCS1_OAEP: seed", seed);
    
    dbMask = NULL;			/* freed @1 */

    /* 1. If the length of P is greater than the input limitation for */
    /* the hash function (2^61-1 octets for SHA-1) then output "parameter */
    /* string too long" and stop. */
    /* NOTE Not done, pHash is input directly */
    /* 2. If ||M|| > emLen-2hLen-1 then output "message too long" and stop. */
    if (rc == 0) {
	if (emLen < ((2 * TPM_DIGEST_SIZE) + 1 + fLen)) {
	    printf("TPM_RSA_padding_add_PKCS1_OAEP: Error, "
		   "message length %u too large for encoded length %u\n", fLen, emLen);
	    rc = TPM_ENCRYPT_ERROR;
	}
    }
    /* 3. Generate an octet string PS consisting of emLen-||M||-2hLen-1 zero octets. The length of
       PS may be 0. */
    /* NOTE Created directly in DB (step 5) */

    /* 4. Let pHash = Hash(P), an octet string of length hLen. */
    /* NOTE pHash is input directly */

    /* 5. Concatenate pHash, PS, the message M, and other padding to form a data block DB as: DB =
       pHash || PS || 01 || M */
    if (rc == 0) {
	/* NOTE Since db is eventually maskedDb, part of em, create directly in em */
	db = em + TPM_DIGEST_SIZE;
	memcpy(db, pHash, TPM_DIGEST_SIZE);				/* pHash */
	memset(db + TPM_DIGEST_SIZE, 0,					/* PS */
	       emLen - fLen - (2 * TPM_DIGEST_SIZE) - 1);
	/* PSlen = emlen - flen - (2 * TPM_DIGEST_SIZE) - 1
	   0x01 index = TPM_DIGEST_SIZE + PSlen
	   = TPM_DIGEST_SIZE + emlen - flen - (2 * TPM_DIGEST_SIZE) - 1
	   = emlen - fLen - TPM_DIGEST_SIZE - 1 */
	db[emLen - fLen - TPM_DIGEST_SIZE - 1] = 0x01;
	memcpy(db + emLen - fLen - TPM_DIGEST_SIZE, from, fLen);	/* M */

	/* 6. Generate a random octet string seed of length hLen. */
	/* NOTE seed is input directly */

	/* 7. Let dbMask = MGF(seed, emLen-hLen). */
	rc = TPM_Malloc(&dbMask, emLen - TPM_DIGEST_SIZE);
    }
    if (rc == 0) {
	rc = TPM_MGF1(dbMask, emLen - TPM_DIGEST_SIZE, seed, TPM_DIGEST_SIZE);
    }
    if (rc == 0) {
	/* 8. Let maskedDB = DB \xor dbMask. */
	/* NOTE Since maskedDB is eventually em, XOR directly to em */
	maskedDb = em + TPM_DIGEST_SIZE;
	TPM_XOR(maskedDb, db, dbMask, emLen - TPM_DIGEST_SIZE);

	/* 9. Let seedMask = MGF(maskedDB, hLen). */
	/* NOTE Since seedMask is eventually em, create directly to em */
	seedMask = em;
	rc = TPM_MGF1(seedMask, TPM_DIGEST_SIZE, maskedDb, emLen - TPM_DIGEST_SIZE);
    }
    if (rc == 0) {
	/* 10. Let maskedSeed = seed \xor seedMask. */
	/* NOTE Since maskedSeed is eventually em, create directly to em */
	maskedSeed = em;
	TPM_XOR(maskedSeed, seed, seedMask, TPM_DIGEST_SIZE);

	/* 11. Let EM = maskedSeed || maskedDB. */
	/* NOTE Created directly in em */
	
	/* 12. Output EM. */
	TPM_PrintFourLimit("  TPM_RSA_padding_add_PKCS1_OAEP: em", em, emLen);
    }
    free(dbMask);		/* @1 */
    return rc;
}

/* TPM_RSA_padding_check_PKCS1_OAEP() is a variation of the openSSL function

   int RSA_padding_check_PKCS1_OAEP(unsigned char *to, int tlen,
   unsigned char *f, int fl, int rsa_len, unsigned char *p, int pl);
   
   It is used for TPM key migration.  In addition to the message 'to' and message length 'tlen', the
   seed and 'pHash are returned.

   This function was independently written from the PKCS1 specification "9.1.1.2 Decoding
   Operation", intended to be unencumbered by the any license.

   |  seed  |  pHash  |		PS	     | 01 |    Message	      |
   SHA1	    SHA1
   | <-			  emLen					   -> |
   
   | maskedSeed
   | seedMask
   | maskedDB
   | db
   | <-		 dbMask					  -> |
   
*/

TPM_RESULT TPM_RSA_padding_check_PKCS1_OAEP(unsigned char *to, uint32_t *tLen, uint32_t tSize, 
					    const unsigned char *em, uint32_t emLen,
					    unsigned char *pHash,	/* output 20 bytes */
					    unsigned char *seed)	/* output 20 bytes */
{
    TPM_RESULT		rc = 0;
    const unsigned char *maskedSeed;
    const unsigned char *maskedDB;
    uint32_t		dbLen;
    unsigned char	*dbMask;
    unsigned char	*seedMask;
    unsigned char	*db;
    size_t		i;

    printf(" TPM_RSA_padding_check_PKCS1_OAEP: emLen %d tSize %d\n", emLen, tSize);
    TPM_PrintFourLimit("  TPM_RSA_padding_check_PKCS1_OAEP: em", em, emLen);

    dbMask = NULL;			/* freed @1 */
    
    /* 1. If the length of P is greater than the input limitation for the hash function (2^61-1
       octets for SHA-1) then output "parameter string too long" and stop. */
    /* NOTE There is no P input.  pHash is calculated for the output, but no comparison is
       performed. */

    /* 2. If ||EM|| < 2hLen+1, then output "decoding error" and stop. */
    if (rc == 0) {
	if (emLen < (2 * TPM_DIGEST_SIZE) + 1) {
	    printf("TPM_RSA_padding_check_PKCS1_OAEP: Error, encoded length %u too small\n", emLen);
	    rc = TPM_DECRYPT_ERROR;
	}
    }
    if (rc == 0) {
	/* 3. Let maskedSeed be the first hLen octets of EM and let maskedDB be the remaining ||EM||
	   - hLen octets. */
	maskedSeed = em;
	maskedDB = em + TPM_DIGEST_SIZE;
	dbLen = emLen - TPM_DIGEST_SIZE;
	/* 4. Let seedMask = MGF(maskedDB, hLen). */
	/* NOTE Created directly in seed */ 
	seedMask = seed;
	rc = TPM_MGF1(seedMask, TPM_DIGEST_SIZE, maskedDB, dbLen);
    }
    if (rc == 0) {
	/* 5. Let seed = maskedSeed \xor seedMask. */
	TPM_XOR(seed, maskedSeed, seedMask, TPM_DIGEST_SIZE);
	/* 6. Let dbMask = MGF(seed, ||EM|| - hLen). */
	rc = TPM_Malloc(&dbMask, dbLen);
    }
    if (rc == 0) {
	rc = TPM_MGF1(dbMask, dbLen, seed, TPM_DIGEST_SIZE);
    }
    if (rc == 0) {
	/* 7. Let DB = maskedDB \xor dbMask. */
	/* NOTE XOR back to dbMask, since dbMask no longer needed */
	db = dbMask;
	TPM_XOR(db, maskedDB, dbMask, dbLen);
	/* 8. Let pHash = Hash(P), an octet string of length hLen. */
	/* NOTE pHash is input directly */
	/* 9. Separate DB into an octet string pHash' consisting of the first hLen octets of DB,
	   ... */
	memcpy(pHash, db, TPM_DIGEST_SIZE);
	/* ... a (possibly empty) octet string PS consisting of consecutive zero octets following
	   pHash', and a message M as: DB = pHash' || PS || 01 || M */
	for (i = TPM_DIGEST_SIZE; i < dbLen; i++) {
	    if (db[i] != 0x00) {
		break;	/* skip the PS segment */
	    }
	}
	/* If there is no 01 octet to separate PS from M, output "decoding error" and stop. */
	if (i == dbLen) {
	    printf("TPM_RSA_padding_check_PKCS1_OAEP: Error, missing 0x01\n");
	    rc = TPM_DECRYPT_ERROR;
	}
    }
    if (rc == 0) {
	if (db[i] != 0x01) {
	    printf("TPM_RSA_padding_check_PKCS1_OAEP: Error, missing 0x01\n");
	    rc = TPM_DECRYPT_ERROR;
	}
    }
    /* 10. If pHash' does not equal pHash, output "decoding error" and stop. */
    /* NOTE No pHash input to compare */
    /* 11. Output M. */
    if (rc == 0) {
	i++;	/* skip the 0x01 to the beginning of the message M */
	*tLen = dbLen - i;
	if (*tLen > tSize) {
	    printf("TPM_RSA_padding_check_PKCS1_OAEP: Error, tSize %u too small for message %u\n",
		   tSize, *tLen);
	    rc = TPM_DECRYPT_ERROR;
	}
    }
    if (rc == 0) {
	memcpy(to, db + i, *tLen);
	printf("  TPM_RSA_padding_check_PKCS1_OAEP: tLen %d \n", *tLen);
	TPM_PrintFourLimit("  TPM_RSA_padding_check_PKCS1_OAEP: to", to, *tLen);
	TPM_PrintFour("  TPM_RSA_padding_check_PKCS1_OAEP: pHash", pHash);
	TPM_PrintFour("  TPM_RSA_padding_check_PKCS1_OAEP: seed", seed);
    }
    free(dbMask);		/* @1 */
    return rc;
}

/* TPM_RSA_exponent_verify() validates the public exponent against a list of legal values.  Some
   values (e.g. even numbers) will have the key generator.
*/

TPM_RESULT TPM_RSA_exponent_verify(unsigned long exponent)
{
    TPM_RESULT rc = 0;
    size_t i;
    int found;

    static const unsigned long legalExponent[] = { 3,5,7,17,257,65537 };
    
    for (i = 0, found = FALSE ;
	 !found && (i < (sizeof(legalExponent) / sizeof (unsigned long))) ;
	 i++) {

	if (exponent == legalExponent[i]) {
	    found = TRUE;
	}
    }
    if (!found) {
	printf("TPM_RSA_exponent_verify: Error, public exponent %lu is illegal\n", exponent );
	rc = TPM_BAD_KEY_PROPERTY;
    }
    return rc;
}

/* SHA1 and HMAC test driver

   Returns TPM_FAILEDSELFTEST on error
*/

TPM_RESULT TPM_CryptoTest(void)
{
    TPM_RESULT	rc = 0;
    int		not_equal;
    TPM_BOOL	valid;
    
    /* SHA1 */
    unsigned char buffer1[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    unsigned char expect1[] = {0x84,0x98,0x3E,0x44,0x1C,
			       0x3B,0xD2,0x6E,0xBA,0xAE,
			       0x4A,0xA1,0xF9,0x51,0x29,
			       0xE5,0xE5,0x46,0x70,0xF1};
    TPM_DIGEST	actual;
    uint32_t	actual_size;

    /* HMAC */
    unsigned char key2[] = {0xaa,0xaa,0xaa,0xaa,0xaa, 0xaa,0xaa,0xaa,0xaa,0xaa,
			    0xaa,0xaa,0xaa,0xaa,0xaa, 0xaa,0xaa,0xaa,0xaa,0xaa};
    unsigned char expect2[] = {0x12,0x5d,0x73,0x42,0xb9,0xac,0x11,0xcd,0x91,0xa3,
			       0x9a,0xf4,0x8a,0xa1,0x7b,0x4f,0x63,0xf1,0x75,0xd3};
    /* data  0xdd repeated 50 times */
    unsigned char data2[50];

    /* oaep tests */
    const unsigned char oaep_pad_str[] = { 'T', 'C', 'P', 'A' };
    unsigned char pHash_in[TPM_DIGEST_SIZE];
    unsigned char pHash_out[TPM_DIGEST_SIZE];
    unsigned char seed_in[TPM_DIGEST_SIZE] = {0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,
					      0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff,
					      0xf0,0xf1,0xf2,0xf3};
    unsigned char seed_out[TPM_DIGEST_SIZE];
    unsigned char oaep_in[8] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};
    unsigned char oaep_pad[256];
    unsigned char oaep_out[8];
    uint32_t	  oeap_length;

    /* symmetric key with pad */
    TPM_SYMMETRIC_KEY_TOKEN tpm_symmetric_key_data = NULL;	/* opaque structure, freed @7 */
    unsigned char	clrStream[64];	/* expected */
    unsigned char	*encStream;	/* encrypted */
    uint32_t		encSize;
    unsigned char	*decStream;	/* actual */
    uint32_t		decSize;

    /* symmetric key ctr and ofb mode */
    TPM_SECRET		symKey;
    TPM_NONCE		pad;		/* CTR or IV */
    TPM_ENCAUTH		symClear;
    TPM_ENCAUTH		symEnc;
    TPM_ENCAUTH		symDec;
    
    /* RSA encrypt and decrypt, sign and verify */
    unsigned char *n;		/* public key - modulus */
    unsigned char *p;		/* private key prime */
    unsigned char *q;		/* private key prime */
    unsigned char *d;		/* private key (private exponent) */
    unsigned char encrypt_data[2048/8];		/* encrypted data */
    unsigned char signature[2048/8];		/* signature; libtpms added */

    printf(" TPM_CryptoTest:\n");
    encStream = NULL;		/* freed @1 */
    decStream = NULL;		/* freed @2 */
    n = NULL;			/* freed @3 */
    p = NULL;			/* freed @4 */
    q = NULL;			/* freed @5 */
    d = NULL;			/* freed @6 */
    
    if (rc == 0) {
	printf(" TPM_CryptoTest: Test 1 - SHA1 one part\n");
	rc = TPM_SHA1(actual,
		      sizeof(buffer1) - 1, buffer1,
		      0, NULL);
    }
    if (rc == 0) {
	not_equal = memcmp(expect1, actual, TPM_DIGEST_SIZE);
	if (not_equal) {
	    printf("TPM_CryptoTest: Error in test 1\n");
	    TPM_PrintFour("\texpect", expect1);
	    TPM_PrintFour("\tactual", actual);
	    rc = TPM_FAILEDSELFTEST;
	}
    }
    if (rc == 0) {
	printf(" TPM_CryptoTest: Test 2 - SHA1 two parts\n");
	rc = TPM_SHA1(actual,
		      16, buffer1,	/* first 16 */
		      sizeof(buffer1) - 17, buffer1 + 16,	/* rest */
		      0, NULL);
    }
    if (rc == 0) {
	not_equal = memcmp(expect1, actual, TPM_DIGEST_SIZE);
	if (not_equal) {
	    printf("TPM_CryptoTest: Error in test 2\n");
	    TPM_PrintFour("\texpect", expect1);
	    TPM_PrintFour("\tactual", actual);
	    rc = TPM_FAILEDSELFTEST;
	}
    }
    if (rc == 0) {
	printf(" TPM_CryptoTest: Test 3 - HMAC generate - one part\n");
	memset(data2, 0xdd, 50);
	rc = TPM_HMAC_Generate(actual,
			       key2,
			       50, data2,
			       0, NULL);
    }
    if (rc == 0) {
	not_equal = memcmp(expect2, actual, TPM_DIGEST_SIZE);
	if (not_equal) {
	    printf("TPM_CryptoTest: Error in test 3\n");
	    TPM_PrintFour("\texpect", expect1);
	    TPM_PrintFour("\tactual", actual);
	    rc = TPM_FAILEDSELFTEST;
	}
    }
    if (rc == 0) {
	printf(" TPM_CryptoTest: Test 4 - HMAC generate - two parts\n");
	memset(data2, 0xdd, 50);
	rc = TPM_HMAC_Generate(actual,
			       key2,
			       20, data2,
			       30, data2 + 20,
			       0, NULL);
    }
    if (rc == 0) {
	not_equal = memcmp(expect2, actual, TPM_DIGEST_SIZE);
	if (not_equal) {
	    printf("TPM_CryptoTest: Error in test 3\n");
	    TPM_PrintFour("\texpect", expect2);
	    TPM_PrintFour("\tactual", actual);
	    rc = TPM_FAILEDSELFTEST;
	}
    }
    if (rc == 0) {
	printf(" TPM_CryptoTest: Test 4 - HMAC check - two parts\n");
	memset(data2, 0xdd, 50);
	rc = TPM_HMAC_Check(&valid,
			    expect2,
			    key2,
			    20, data2,
			    30, data2 + 20,
			    0, NULL);
    }
    if (rc == 0) {
	if (!valid) {
	    printf("TPM_CryptoTest: Error in test 4\n");
	    TPM_PrintFour("\texpect", expect1);
	    TPM_PrintFour("\tactual", actual);
	    rc = TPM_FAILEDSELFTEST;
	}
    }
    if (rc == 0) {
	printf(" TPM_CryptoTest: Test 5 - OAEP add and check\n");
	rc = TPM_SHA1(pHash_in,
		      sizeof(oaep_pad_str), oaep_pad_str,
		      0, NULL);
    }
    if (rc == 0) {
	rc = TPM_RSA_padding_add_PKCS1_OAEP(oaep_pad, sizeof(oaep_pad),
					    oaep_in, sizeof(oaep_in),
					    pHash_in, seed_in);
    }
    if (rc == 0) {
	rc = TPM_RSA_padding_check_PKCS1_OAEP(oaep_out, &oeap_length, sizeof(oaep_out),
					      oaep_pad, sizeof(oaep_pad),
					      pHash_out,
					      seed_out);
    }
    if (rc == 0) {
	if (oeap_length != sizeof(oaep_out)) {
	    printf("TPM_CryptoTest: Error in test 5, expect length %lu, actual length %u\n",
		   (unsigned long)sizeof(oaep_out), oeap_length);
	    rc = TPM_FAILEDSELFTEST;
	}
    }
    if (rc == 0) {
	not_equal = memcmp(oaep_in, oaep_out, sizeof(oaep_out));
	if (not_equal) {
	    printf("TPM_CryptoTest: Error in test 5 oaep\n");
	    TPM_PrintFour("\tin ", oaep_in);
	    TPM_PrintFour("\tout", oaep_out);
	    rc = TPM_FAILEDSELFTEST;
	}
    }
    if (rc == 0) {
	not_equal = memcmp(pHash_in, pHash_out, sizeof(pHash_in));
	if (not_equal) {
	    printf("TPM_CryptoTest: Error in test 5 pHash\n");
	    TPM_PrintFour("\tpHash_in ", pHash_in);
	    TPM_PrintFour("\tpHash_out", pHash_out);
	    rc = TPM_FAILEDSELFTEST;
	}
    }
    if (rc == 0) {
	not_equal = memcmp(seed_in, seed_out, sizeof(seed_in));
	if (not_equal) {
	    printf("TPM_CryptoTest: Error in test 5 seed\n");
	    TPM_PrintFour("\tseed_in ", seed_in);
	    TPM_PrintFour("\tseed_out", seed_out);
	    rc = TPM_FAILEDSELFTEST;
	}
    }
    if (rc == 0) {
	printf(" TPM_CryptoTest: Test 6 - Symmetric key with PKCS pad test\n");
	/* allocate memory for the key token */
	rc = TPM_SymmetricKeyData_New(&tpm_symmetric_key_data);	/* freed @7 */
    }
    /* generate a key */
    if (rc == 0) {
	rc = TPM_SymmetricKeyData_GenerateKey(tpm_symmetric_key_data);
    }
    /* generate clear text */
    if (rc == 0) {
	rc = TPM_Random(clrStream, sizeof(clrStream));
    }
    /* symmetric encrypt */
    if (rc == 0) {
	rc = TPM_SymmetricKeyData_Encrypt(&encStream,		/* output, freed @1 */
					  &encSize,			/* output */
					  clrStream,			/* input */
					  sizeof(clrStream),		/* input */
					  tpm_symmetric_key_data);	/* key */
    }
    /* symmetric decrypt */
    if (rc == 0) {
	rc = TPM_SymmetricKeyData_Decrypt(&decStream,		/* output, freed by caller */
					  &decSize,			/* output */
					  encStream,			/* input */
					  encSize,			/* input */
					  tpm_symmetric_key_data);	/* key */
    }
    /* symmetric compare */
    if (rc == 0) {
	if (sizeof(clrStream) != decSize) {
	    printf("TPM_CryptoTest: Error in test 6, in %lu, out %u\n",
		   (unsigned long)sizeof(clrStream), decSize);
	    rc = TPM_FAILEDSELFTEST;
	}
    }
    if (rc == 0) {
	not_equal = memcmp(clrStream, decStream, sizeof(clrStream));
	if (not_equal) {
	    printf("TPM_CryptoTest: Error in test 6\n");
	    TPM_PrintFour("\tclear stream  in", clrStream);
	    TPM_PrintFour("\tdecrypted stream", decStream);
	    rc = TPM_FAILEDSELFTEST;
	}
    }
    if (rc == 0) {
	printf(" TPM_CryptoTest: Test 7 - Symmetric key with CTR mode\n");
	/* generate a key */
	rc = TPM_Random(symKey, TPM_SECRET_SIZE);
    }
    /* generate CTR */
    if (rc == 0) {
	rc = TPM_Random(pad, TPM_NONCE_SIZE);
    }
    /* generate clear text */
    if (rc == 0) {
	rc = TPM_Random(symClear, TPM_AUTHDATA_SIZE);
    }
    if (rc == 0) {
	rc = TPM_SymmetricKeyData_CtrCrypt(symEnc,			/* output */
					   symClear,			/* input */
					   TPM_AUTHDATA_SIZE,		/* input */
					   symKey,			/* in */
					   TPM_SECRET_SIZE,		/* in */
					   pad,				/* input */
					   TPM_NONCE_SIZE);		/* input */
    }
    if (rc == 0) {
	rc = TPM_SymmetricKeyData_CtrCrypt(symDec,			/* output */
					   symEnc,			/* input */
					   TPM_AUTHDATA_SIZE,		/* input */
					   symKey,			/* in */
					   TPM_SECRET_SIZE,		/* in */
					   pad,				/* input */
					   TPM_NONCE_SIZE);		/* input */
    }
    /* symmetric compare */
    if (rc == 0) {
	rc = TPM_Secret_Compare(symDec, symClear);
	if (rc != 0) {
	    printf("TPM_CryptoTest: Error in test 8\n");
	    TPM_PrintFour("\tclear stream  in", symClear);
	    TPM_PrintFour("\tdecrypted stream", symDec);
	}
    }
    if (rc == 0) {
	printf(" TPM_CryptoTest: Test 8 - Symmetric key with OFB mode\n");
	/* generate a key */
	rc = TPM_Random(symKey, TPM_SECRET_SIZE);
    }
    /* generate IV */
    if (rc == 0) {
	rc = TPM_Random(pad, TPM_NONCE_SIZE);
    }
    /* generate clear text */
    if (rc == 0) {
	rc = TPM_Random(symClear, TPM_AUTHDATA_SIZE);
    }
    if (rc == 0) {
	rc = TPM_SymmetricKeyData_OfbCrypt(symEnc,			/* output */
					   symClear,			/* input */
					   TPM_AUTHDATA_SIZE,		/* input */
					   symKey,			/* in */
					   TPM_SECRET_SIZE,		/* in */
					   pad,				/* input */
					   TPM_NONCE_SIZE);		/* input */
    }
    if (rc == 0) {
	rc = TPM_SymmetricKeyData_OfbCrypt(symDec,			/* output */
					   symEnc,			/* input */
					   TPM_AUTHDATA_SIZE,		/* input */
					   symKey,			/* in */
					   TPM_SECRET_SIZE,		/* in */
					   pad,				/* input */
					   TPM_NONCE_SIZE);		/* input */
    }
    /* symmetric compare */
    if (rc == 0) {
	rc = TPM_Secret_Compare(symDec, symClear);
	if (rc != 0) {
	    printf("TPM_CryptoTest: Error in test 8\n");
	    TPM_PrintFour("\tclear stream  in", symClear);
	    TPM_PrintFour("\tdecrypted stream", symDec);
	}
    }
    /* RSA OAEP encrypt and decrypt */
    if (rc == 0) {
	printf(" TPM_CryptoTest: Test 9 - RSA encrypt with OAEP padding\n");
	/* generate a key */
	rc = TPM_RSAGenerateKeyPair(&n,				/* public key - modulus */
				    &p,				/* private key prime */
				    &q,				/* private key prime */
				    &d,				/* private key (private exponent) */
				    2048,			/* key size in bits */
				    tpm_default_rsa_exponent,	/* public exponent as an array */
				    3);
    }
    /* encrypt */
    if (rc == 0) {
	rc = TPM_RSAPublicEncrypt(encrypt_data,			/* encrypted data */
				  sizeof(encrypt_data),		/* size of encrypted data buffer */
				  TPM_ES_RSAESOAEP_SHA1_MGF1,	/* TPM_ENC_SCHEME */
				  expect1,			/* decrypted data */
				  sizeof(expect1),
				  n,				/* public modulus */
				  2048/8,
				  tpm_default_rsa_exponent,	/* public exponent */
				  3);
    }
    if (rc == 0) {
	rc = TPM_RSAPrivateDecrypt(actual,			/* decrypted data */
				   &actual_size,		/* length of data put into
								   decrypt_data */
				   TPM_DIGEST_SIZE,		/* size of decrypt_data buffer */
				   TPM_ES_RSAESOAEP_SHA1_MGF1,	/* TPM_ENC_SCHEME */
				   encrypt_data,		/* encrypted data */
				   sizeof(encrypt_data),
				   n,				/* public modulus */
				   2048/8,
				   tpm_default_rsa_exponent,	/* public exponent */
				   3,
				   d,				/* private exponent */
				   2048/8);
    }
    if (rc == 0) {
	if (actual_size != TPM_DIGEST_SIZE) {
	    printf("TPM_CryptoTest: Error in test 9, expect length %u, actual length %u\n",
		   TPM_DIGEST_SIZE, actual_size);
	    rc = TPM_FAILEDSELFTEST;
	}
    }
    if (rc == 0) {
	not_equal = memcmp(expect1, actual, TPM_DIGEST_SIZE);
	if (not_equal) {
	    printf("TPM_CryptoTest: Error in test 9\n");
	    TPM_PrintFour("\tin ", expect1);
	    TPM_PrintFour("\tout", actual);
	    rc = TPM_FAILEDSELFTEST;
	}
    }
    /* RSA PKCS1 pad, encrypt and decrypt */
    if (rc == 0) {
	printf(" TPM_CryptoTest: Test 10 - RSA encrypt with PKCS padding\n");
	/* encrypt */
	rc = TPM_RSAPublicEncrypt(encrypt_data,			/* encrypted data */
				  sizeof(encrypt_data),		/* size of encrypted data buffer */
				  TPM_ES_RSAESPKCSv15,		/* TPM_ENC_SCHEME */
				  expect1,			/* decrypted data */
				  sizeof(expect1),
				  n,				/* public modulus */
				  2048/8,
				  tpm_default_rsa_exponent,	/* public exponent */
				  3);
    }
    /* decrypt */
    if (rc == 0) {
	rc = TPM_RSAPrivateDecrypt(actual,			/* decrypted data */
				   &actual_size,		/* length of data put into
								   decrypt_data */
				   TPM_DIGEST_SIZE,		/* size of decrypt_data buffer */
				   TPM_ES_RSAESPKCSv15,		/* TPM_ENC_SCHEME */
				   encrypt_data,		/* encrypted data */
				   sizeof(encrypt_data),
				   n,				/* public modulus */
				   2048/8,
				   tpm_default_rsa_exponent,	/* public exponent */
				   3,
				   d,				/* private exponent */
				   2048/8);
    }
    /* check length after padding removed */
    if (rc == 0) {
	if (actual_size != TPM_DIGEST_SIZE) {
	    printf("TPM_CryptoTest: Error in test 10, expect length %u, actual length %u\n",
		   TPM_DIGEST_SIZE, actual_size);
	    rc = TPM_FAILEDSELFTEST;
	}
    }
    /* check data */
    if (rc == 0) {
	not_equal = memcmp(expect1, actual, TPM_DIGEST_SIZE);
	if (not_equal) {
	    printf("TPM_CryptoTest: Error in test 10\n");
	    TPM_PrintFour("\tin ", expect1);
	    TPM_PrintFour("\tout", actual);
	    rc = TPM_FAILEDSELFTEST;
	}
    }

// libtpms added begin

    if (rc == 0) {
	printf(" TPM_CryptoTest: Test 11a - RSA sign with PKCS1v15 padding\n");
	rc = TPM_RSASign(signature,
			 &actual_size,
			 sizeof(signature),
			 TPM_SS_RSASSAPKCS1v15_SHA1,
			 expect1,
			 sizeof(expect1),
			 n,				/* public modulus */
			 2048/8,
			 tpm_default_rsa_exponent,	/* public exponent */
			 3,
			 d,				/* private exponent */
			 2048/8);
    }
    if (rc == 0) {
	rc = TPM_RSAVerify(signature,		/* input signature buffer */
			   sizeof(signature),	/* input, size of signature buffer */
			   TPM_SS_RSASSAPKCS1v15_SHA1, /* input, type of signature */
			   expect1,		/* message */
			   sizeof(expect1),	/* message size */
			   n,			/* public modulus */
			   2048/8,
			   tpm_default_rsa_exponent,/* public exponent */
			   3);
    }

#if 0
    /* Verification with TPM_SS_RSASSAPKCS1v15_DER is not supported */
    if (rc == 0) {
	printf(" TPM_CryptoTest: Test 11b - RSA sign with PKCS1v15_DER padding\n");
	rc = TPM_RSASign(signature,
			 &actual_size,
			 sizeof(signature),
			 TPM_SS_RSASSAPKCS1v15_DER,
			 expect1,
			 sizeof(expect1),
			 n,				/* public modulus */
			 2048/8,
			 tpm_default_rsa_exponent,	/* public exponent */
			 3,
			 d,				/* private exponent */
			 2048/8);
    }
    if (rc == 0) {
	rc = TPM_RSAVerify(signature,		/* input signature buffer */
			   sizeof(signature),	/* input, size of signature buffer */
			   TPM_SS_RSASSAPKCS1v15_DER, /* input, type of signature */
			   expect1,		/* message */
			   sizeof(expect1),	/* message size */
			   n,			/* public modulus */
			   2048/8,
			   tpm_default_rsa_exponent,/* public exponent */
			   3);
    }
#endif // libtpms added end

    /* run library specific self tests as required */
    if (rc == 0) {
	rc = TPM_Crypto_TestSpecific();
    }
    if (rc != 0) {
	rc = TPM_FAILEDSELFTEST;
    }
    free(encStream);					/* @1 */
    free(decStream);					/* @2 */
    free(n);						/* @3 */
    free(p);						/* @4 */
    free(q);						/* @5 */
    free(d);						/* @6 */
    TPM_SymmetricKeyData_Free(&tpm_symmetric_key_data);	/* @7 */
    return rc;
}

/* 13.5 TPM_Sign rev 111

   The Sign command signs data and returns the resulting digital signature.

   The TPM does not allow TPM_Sign with a TPM_KEY_IDENTITY (AIK) because TPM_Sign can sign arbitrary
   data and could be used to fake a quote.  (This could have been relaxed to allow TPM_Sign with an
   AIK if the signature scheme is _INFO For an _INFO key, the metadata prevents TPM_Sign from faking
   a quote.)

   The TPM MUST support all values of areaToSignSize that are legal for the defined signature scheme
   and key size. The maximum value of areaToSignSize is determined by the defined signature scheme
   and key size.

   In the case of PKCS1v15_SHA1 the areaToSignSize MUST be TPM_DIGEST (the hash size of a sha1
   operation - see 8.5.1 TPM_SS_RSASSAPKCS1v15_SHA1). In the case of PKCS1v15_DER the maximum size
   of areaToSign is k - 11 octets, where k is limited by the key size (see 8.5.2
   TPM_SS_RSASSAPKCS1v15_DER).
*/

TPM_RESULT TPM_Process_Sign(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE	keyHandle;	/* The keyHandle identifier of a loaded key that can perform
					   digital signatures. */
    TPM_SIZED_BUFFER	areaToSign;	/* The value to sign */
    TPM_AUTHHANDLE	authHandle;	/* The authorization handle used for keyHandle authorization
					 */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization handle */
    TPM_AUTHDATA	privAuth;	/* The authorization digest that authorizes the use of
					   keyHandle. HMAC key: key.usageAuth */
    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus	 ;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_KEY			*key = NULL;			/* the key specified by keyHandle */
    TPM_RSA_KEY_PARMS		*rsa_key_parms;			/* for key */
    TPM_BOOL			parentPCRStatus;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_SECRET			*keyUsageAuth;
    TPM_SIGN_INFO		tpm_sign_info;
    const unsigned char		*S1_data;			/* data to be signed */
    uint32_t			S1_size;
    TPM_DIGEST			infoDigest;			/* TPM_SIGN_INFO structure digest */
    TPM_STORE_BUFFER		sbuffer;
    
    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_SIZED_BUFFER	sig;		/* The resulting digital signature. */

    printf("TPM_Process_Sign: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&areaToSign);	/* freed @1 */
    TPM_SignInfo_Init(&tpm_sign_info);	/* freed @2 */
    TPM_Sbuffer_Init(&sbuffer);		/* freed @3 */
    TPM_SizedBuffer_Init(&sig);		/* freed @4 */
    /*
      get inputs
    */
    /*	get keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&keyHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_Sign: keyHandle %08x\n", keyHandle);
	/* get areaToSignSize and areaToSign parameters */
	returnCode = TPM_SizedBuffer_Load(&areaToSign, &command, &paramSize);	/* freed @1 */
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_Sign: Signing %u bytes\n", areaToSign.size);
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
					privAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_Sign: Error, command has %u extra bytes\n", paramSize);
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
    /* get the key corresponding to the keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&key, &parentPCRStatus, tpm_state, keyHandle,
						 FALSE,		/* not r/o, used to sign */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    /* check TPM_AUTH_DATA_USAGE authDataUsage */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND)) {
	if (key->authDataUsage != TPM_AUTH_NEVER) {
	    printf("TPM_Process_Sign: Error, authorization required\n");
	    returnCode = TPM_AUTHFAIL;
	}
    }
    /* get keyHandle -> usageAuth */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_Key_GetUsageAuth(&keyUsageAuth, key);
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
					      key,
					      keyUsageAuth,		/* OIAP */
					      key->tpm_store_asymkey->pubDataDigest); /* OSAP */
    }
    /* 1. The TPM validates the AuthData to use the key pointed to by keyHandle. */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					privAuth);	/* Authorization digest for input */
    }
    /* 2. If the areaToSignSize is 0 the TPM returns TPM_BAD_PARAMETER. */
    if (returnCode == TPM_SUCCESS) {
	if (areaToSign.size == 0) {
	    printf("TPM_Process_Sign: Error, areaToSignSize is 0\n");
	    returnCode = TPM_BAD_PARAMETER;
	}
    }
    /* 3. Validate that keyHandle -> keyUsage is TPM_KEY_SIGNING or TPM_KEY_LEGACY, if not return
       the error code TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	if ((key->keyUsage != TPM_KEY_SIGNING) && ((key->keyUsage) != TPM_KEY_LEGACY)) {
	    printf("TPM_Process_Sign: Error, keyUsage %04hx is invalid\n", key->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 4. The TPM verifies that the signature scheme and key size can properly sign the areaToSign
       parameter.  NOTE Done in 5. - 7.*/
    /* get key -> TPM_RSA_KEY_PARMS */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyParms_GetRSAKeyParms(&rsa_key_parms,
						 &(key->algorithmParms));
    }
    if (returnCode == TPM_SUCCESS) {
	/* 5. If signature scheme is TPM_SS_RSASSAPKCS1v15_SHA1 then */
	if (key->algorithmParms.sigScheme == TPM_SS_RSASSAPKCS1v15_SHA1) {
	    printf("TPM_Process_Sign: sigScheme is TPM_SS_RSASSAPKCS1v15_SHA1\n");
	    /* a. Validate that areaToSignSize is 20 return TPM_BAD_PARAMETER on error */
	    if (returnCode == TPM_SUCCESS) {
		if (areaToSign.size != TPM_DIGEST_SIZE) {
		    printf("TPM_Process_Sign: Error, areaToSignSize %d should be %u\n",
			   areaToSign.size, TPM_DIGEST_SIZE);
		    returnCode = TPM_BAD_PARAMETER;
		}
	    }
	    /* b. Set S1 to areaToSign */
	    if (returnCode == TPM_SUCCESS) {
		S1_size = areaToSign.size;
		S1_data = areaToSign.buffer;
	    }
	}
	/* 6. Else if signature scheme is TPM_SS_RSASSAPKCS1v15_DER then */
	else if (key->algorithmParms.sigScheme == TPM_SS_RSASSAPKCS1v15_DER) {
	    printf("TPM_Process_Sign: sigScheme is TPM_SS_RSASSAPKCS1v15_DER\n");
	    /* a. Validate that areaToSignSize is at least 11 bytes less than the key size, return
	       TPM_BAD_PARAMETER on error */
	    if (returnCode == TPM_SUCCESS) {
		if (areaToSign.size > (((rsa_key_parms->keyLength)/CHAR_BIT) - 11)) {
		    printf("TPM_Process_Sign: Error, areaToSignSize %d should be 11-%u\n",
			   areaToSign.size, ((rsa_key_parms->keyLength)/CHAR_BIT));
		    returnCode = TPM_BAD_PARAMETER;
		}
	    }	
	    /* b. Set S1 to areaToSign */
	    if (returnCode == TPM_SUCCESS) {
		S1_size = areaToSign.size;
		S1_data = areaToSign.buffer;
	    }
	}
	/* 7. else if signature scheme is TPM_SS_RSASSAPKCS1v15_INFO then */
	else if (key->algorithmParms.sigScheme == TPM_SS_RSASSAPKCS1v15_INFO) {
	    printf("TPM_Process_Sign: sigScheme is TPM_SS_RSASSAPKCS1v15_INFO\n");
	    if (returnCode == TPM_SUCCESS) {
		/* a. Create S2 a TPM_SIGN_INFO structure */
		/* NOTE: Done by TPM_SignInfo_Init() */
		/* b. Set S2 -> fixed to "SIGN" */
		memcpy(tpm_sign_info.fixed, "SIGN", TPM_SIGN_INFO_FIXED_SIZE);
		/* c.i. If nonceOdd is not present due to an unauthorized command return
		   TPM_BAD_PARAMETER */
		if (tag == TPM_TAG_RQU_COMMAND) {
		    printf("TPM_Process_Sign: Error, TPM_SS_RSASSAPKCS1v15_INFO and no auth\n");
		    returnCode = TPM_BAD_PARAMETER;
		}
	    }
	    if (returnCode == TPM_SUCCESS) {
		/* c. Set S2 -> replay to nonceOdd */
		TPM_Nonce_Copy(tpm_sign_info.replay, nonceOdd);
		/* d. Set S2 -> dataLen to areaToSignSize */
		/* e. Set S2 -> data to areaToSign */
		returnCode = TPM_SizedBuffer_Copy(&(tpm_sign_info.data), &areaToSign);
	    }
	    /* f. Set S1 to the SHA-1(S2) */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_SHA1_GenerateStructure(infoDigest,
							&tpm_sign_info,
							(TPM_STORE_FUNCTION_T)TPM_SignInfo_Store);
		S1_size = TPM_DIGEST_SIZE;
		S1_data = infoDigest;
	    }
	}
	/* 8. Else return TPM_INVALID_KEYUSAGE */
	else {
	    printf("TPM_Process_Sign: Error, sigScheme %04hx\n", key->algorithmParms.sigScheme);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 9. The TPM computes the signature, sig, using the key referenced by keyHandle using S1 as the
       value to sign */
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintAll("TPM_Process_Sign: Digest to sign", S1_data, S1_size);
	returnCode = TPM_RSASignToSizedBuffer(&sig,		/* signature */
					      S1_data,		/* message */
					      S1_size,		/* message size */
					      key);		/* input, signing key */
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_Sign: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* 10. Return the computed signature in Sig */
	    returnCode = TPM_SizedBuffer_Store(response, &sig);
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
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&areaToSign);	/* @1 */
    TPM_SignInfo_Delete(&tpm_sign_info);	/* @2 */
    TPM_Sbuffer_Delete(&sbuffer);		/* @3 */
    TPM_SizedBuffer_Delete(&sig);		/* @4 */
    return rcf;
}

/* 13.1 TPM_SHA1Start rev 96

   This capability starts the process of calculating a SHA-1 digest.

   The exposure of the SHA-1 processing is a convenience to platforms in a mode that do not have
   sufficient memory to perform SHA-1 themselves. As such the use of SHA-1 is restrictive on the
   TPM.

   The TPM may not allow any other types of processing during the execution of a SHA-1
   session. There is only one SHA-1 session active on a TPM.  The exclusivity of a SHA-1 
   context is due to the relatively large volatile buffer it requires in order to hold the 
   intermediate results between the SHA-1 context commands.  This buffer can be in 
   contradiction to other command needs.

   After the execution of SHA1Start, and prior to SHA1End, the receipt of any command other than
   SHA1Update will cause the invalidation of the SHA-1 session.
*/

TPM_RESULT TPM_Process_SHA1Start(tpm_state_t *tpm_state,
				 TPM_STORE_BUFFER *response,
				 TPM_TAG tag,
				 uint32_t paramSize,
				 TPM_COMMAND_CODE ordinal,
				 unsigned char *command,
				 TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters	 - none */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */

    /* output parameters */
    uint32_t		outParamStart;			/* starting point of outParam's */
    uint32_t		outParamEnd;			/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    uint32_t maxNumBytes = TPM_SHA1_MAXNUMBYTES;	/* Maximum number of bytes that can be sent
							   to TPM_SHA1Update. Must be a multiple of
							   64 bytes. */

    printf("TPM_Process_SHA1Start: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
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
	returnCode = TPM_CheckState(tpm_state, tag, (TPM_CHECK_NOT_SHUTDOWN |
						     TPM_CHECK_NO_LOCKOUT));
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_SHA1Start: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* This capability prepares the TPM for a subsequent TPM_SHA1Update, TPM_SHA1Complete or
       TPM_SHA1CompleteExtend command. The capability SHALL open a thread that calculates a SHA-1
       digest.
    */
    if (returnCode == TPM_SUCCESS) {
	if (transportInternal == NULL) {
	    tpm_state->transportHandle = 0;	/* SHA-1 thread not within transport */
	}
	else {
	    tpm_state->transportHandle = transportInternal->transHandle; /* SHA-1 thread within
									    transport */
	}
	returnCode = TPM_SHA1InitCmd(&(tpm_state->sha1_context));
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_SHA1Start: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* append maxNumBytes */
	    returnCode = TPM_Sbuffer_Append32(response, maxNumBytes);
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
    return rcf;
}

/* 13.2 TPM_SHA1Update rev 114

   This capability inputs complete blocks of data into a pending SHA-1 digest. At the end of the
   process, the digest remains pending.
*/

TPM_RESULT TPM_Process_SHA1Update(tpm_state_t *tpm_state,
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
    TPM_SIZED_BUFFER	hashData;	/* Bytes to be hashed */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_SHA1Update: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&hashData);	/* freed @1 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* load hashData */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&hashData, &command, &paramSize);
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
	returnCode = TPM_CheckState(tpm_state, tag, (TPM_CHECK_NOT_SHUTDOWN |
						     TPM_CHECK_NO_LOCKOUT));
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_SHA1Update: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* This command SHALL incorporate complete blocks of data into the digest of an existing SHA-1
       thread. Only integral numbers of complete blocks (64 bytes each) can be processed.
    */
    /* 1. If there is no existing SHA-1 thread, return TPM_SHA_THREAD */
    if (returnCode == TPM_SUCCESS) {
	if (tpm_state->sha1_context == NULL) {
	    printf("TPM_Process_SHA1Update: Error, no existing SHA1 thread\n");
	    returnCode = TPM_SHA_THREAD;
	}
    }
    /* 2. If numBytes is not a multiple of 64 */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_SHA1Update: numBytes %u bytes\n", hashData.size);
	if ((hashData.size % 64) != 0) {
	    printf("TPM_Process_SHA1Update: Error, numBytes not integral number of blocks\n");
	    /* a. Return TPM_SHA_ERROR */
	    returnCode = TPM_SHA_ERROR;
	    /* b. The TPM MAY terminate the SHA-1 thread */
	    TPM_SHA1Delete(&(tpm_state->sha1_context));
	}
    }
    /* 3. If numBytes is greater than maxNumBytes returned by TPM_SHA1Start */
    if (returnCode == TPM_SUCCESS) {
	if (hashData.size > TPM_SHA1_MAXNUMBYTES) {
	    /* a. Return TPM_SHA_ERROR */
	    returnCode = TPM_SHA_ERROR;
	    /* b. The TPM MAY terminate the SHA-1 thread */
	    TPM_SHA1Delete(&(tpm_state->sha1_context));
	}
    }
    /* 4. Incorporate hashData into the digest of the existing SHA-1 thread. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SHA1UpdateCmd(tpm_state->sha1_context, hashData.buffer, hashData.size);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_SHA1Update: Ordinal returnCode %08x %u\n",
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
    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&hashData);	/* @1 */
    return rcf;
}

/* 13.3 TPM_SHA1Complete rev 87

   This capability terminates a pending SHA-1 calculation.
*/

TPM_RESULT TPM_Process_SHA1Complete(tpm_state_t *tpm_state,
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
    TPM_SIZED_BUFFER	hashData;	/* Final bytes to be hashed */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_DIGEST		hashValue;	/* The output of the SHA-1 hash. */
    TPM_SizedBuffer_Init(&hashData);	/* freed @1 */

    printf("TPM_Process_SHA1Complete: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* load hashData */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&hashData, &command, &paramSize);
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
	returnCode = TPM_CheckState(tpm_state, tag, (TPM_CHECK_NOT_SHUTDOWN |
						     TPM_CHECK_NO_LOCKOUT));
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_SHA1Complete: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* This command SHALL incorporate a partial or complete block of data into the digest of an
       existing SHA-1 thread, and terminate that thread. hashDataSize MAY have values in the range
       of 0 through 64, inclusive.  If the SHA-1 thread has received no bytes the TPM SHALL
       calculate the SHA-1 of the empty buffer.
    */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SHA1CompleteCommon(hashValue,
					    &(tpm_state->sha1_context),
					    &hashData);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_SHA1Complete: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* append hashValue */
	    returnCode = TPM_Digest_Store(response, hashValue);
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
    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&hashData);	/* @1 */
    return rcf;
}

/* 13.4 TPM_SHA1CompleteExtend rev 109

   This capability terminates a pending SHA-1 calculation and EXTENDS the result into a Platform
   Configuration Register using a SHA-1 hash process.

   This command is designed to complete a hash sequence and extend a PCR in memory-less
   environments.

   This command SHALL incorporate a partial or complete block of data into the digest of an existing
   SHA-1 thread, EXTEND the resultant digest into a PCR, and terminate the thread. hashDataSize MAY
   have values in the range of 0 through 64, inclusive.
*/

TPM_RESULT TPM_Process_SHA1CompleteExtend(tpm_state_t *tpm_state,
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
    TPM_PCRINDEX	pcrNum;		/* Index of the PCR to be modified */
    TPM_SIZED_BUFFER	hashData;	/* Final bytes to be hashed */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus; 		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_DIGEST		h1HashValue;	/* The output of the SHA-1 hash. */
    TPM_PCRVALUE	outDigest;	/* The PCR value after execution of the command. */
    
    printf("TPM_Process_SHA1CompleteExtend: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&hashData);	/* freed @1 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get pcrNum */ 
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&pcrNum, &command, &paramSize);
    }
    /* get hashData */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_SHA1CompleteExtend: pcrNum %u\n", pcrNum);
	returnCode = TPM_SizedBuffer_Load(&hashData, &command, &paramSize);
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
	returnCode = TPM_CheckState(tpm_state, tag, (TPM_CHECK_NOT_SHUTDOWN |
						     TPM_CHECK_NO_LOCKOUT));
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_SHA1CompleteExtend: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. 1.Validate that pcrNum represents a legal PCR number. On error, return TPM_BADINDEX. */
    /* 2. Map V1 to TPM_STANY_DATA */
    /* 3. Map L1 to V1 -> localityModifier */
    /* 4. If the current locality, held in L1, is not selected in TPM_PERMANENT_DATA -> pcrAttrib
	  [PCRIndex]. pcrExtendLocal, return TPM_BAD_LOCALITY */
    /* NOTE Done in TPM_ExtendCommon() */
    /* 5. Create H1 the TPM_DIGEST of the SHA-1 session ensuring that hashData, if any, is  */
    /* added to the SHA-1 session */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SHA1CompleteCommon(h1HashValue,
					    &(tpm_state->sha1_context),
					    &hashData);
    }
    /* 6. Perform the actions of TPM_Extend using H1 as the data and pcrNum as the PCR to extend */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_ExtendCommon(outDigest, tpm_state, ordinal, pcrNum, h1HashValue);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_SHA1CompleteExtend: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* append hashValue */
	    returnCode = TPM_Digest_Store(response, h1HashValue);
	}
	/* append outDigest */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Digest_Store(response, outDigest);
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
    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&hashData);	/* @1 */
    return rcf;
}

/* TPM_SHA1CompleteCommon() is common code for TPM_Process_SHA1Complete() and
   TPM_Process_SHA1CompleteExtend()
*/

TPM_RESULT TPM_SHA1CompleteCommon(TPM_DIGEST hashValue,		/* output: digest */
				  void **sha1_context,		/* IO: SHA1 context */
				  TPM_SIZED_BUFFER *hashData)	/* final data to be hashed */
{
    TPM_RESULT	rc = 0;

    /* The TPM specification says that the last data chunk must be 0-64 bytes */
    printf("TPM_SHA1CompleteCommon: %u bytes\n", hashData->size);
    if (rc == 0) {
	if (hashData->size > 64) {
	    printf("TPM_SHA1CompleteCommon: Error, hashDataSize %u not 0-64\n",
		   hashData->size);
	    rc = TPM_SHA_ERROR;
	}
    }
    /* cannot call SHA1Complete() before SHA1Start() */
    if (rc == 0) {
	if (*sha1_context == NULL) {
	    printf("TPM_SHA1CompleteCommon: Error, no existing SHA1 thread\n");
	    rc = TPM_SHA_THREAD;
	}
    }
    if ((rc == 0) && (hashData->size != 0)) {
	rc = TPM_SHA1UpdateCmd(*sha1_context, hashData->buffer, hashData->size);
    }
    if (rc == 0) {
	rc = TPM_SHA1FinalCmd(hashValue, *sha1_context);
    }
    /* the SHA1 thread should be terminated even if there is an error */
    TPM_SHA1Delete(sha1_context);
    return rc;
}

/* 13.6 TPM_GetRandom rev 87

   GetRandom returns the next bytesRequested bytes from the random number generator to the caller.

   It is recommended that a TPM implement the RNG in a manner that would allow it to return RNG
   bytes such that the frequency of bytesRequested being more than the number of bytes available is
   an infrequent occurrence.
*/

TPM_RESULT TPM_Process_GetRandom(tpm_state_t *tpm_state,
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
    uint32_t	  bytesRequested;	/* Number of bytes to return */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_SIZED_BUFFER	randomBytes;	/* The returned bytes */

    printf("TPM_Process_GetRandom: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&randomBytes); /* freed @1 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get bytesRequested parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&bytesRequested, &command, &paramSize);
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
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_ALLOW_NO_OWNER);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_GetRandom: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. The TPM determines if amount bytesRequested is available from the TPM. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_GetRandom: bytesRequested %u\n", bytesRequested);
	if (bytesRequested > TPM_RANDOM_MAX) {
	    bytesRequested = TPM_RANDOM_MAX;
	    printf("TPM_Process_GetRandom: bytes available %u\n", bytesRequested);
	}
    }
    /* 2. Set randomBytesSize to the number of bytes available from the RNG. This number MAY be less
       than bytesRequested. */
    if ((returnCode == TPM_SUCCESS) && (bytesRequested > 0)) {
	returnCode = TPM_SizedBuffer_Allocate(&randomBytes, bytesRequested);
    }
    /* 3. Set randomBytes to the next randomBytesSize bytes from the RNG */
    if ((returnCode == TPM_SUCCESS) && (bytesRequested > 0)) {
	returnCode = TPM_Random(randomBytes.buffer, bytesRequested);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_GetRandom: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* append randomBytes */
	    returnCode = TPM_SizedBuffer_Store(response, &randomBytes);
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
    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&randomBytes);	/* freed @1 */
    return rcf;
}

/* 13.7 TPM_StirRandom rev 109

   StirRandom adds entropy to the RNG state.
*/

TPM_RESULT TPM_Process_StirRandom(tpm_state_t *tpm_state,
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
    TPM_SIZED_BUFFER	inData;		/* Data to add entropy to RNG state,
					   Number of bytes of input */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_StirRandom: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&inData);	/* freed @1 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get inData parameter */
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
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_ALLOW_NO_OWNER);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_StirRandom: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. If dataSize is not less than 256 bytes, the TPM MAY return TPM_BAD_PARAMETER. */
    /* The TPM updates the state of the current RNG using the appropriate mixing function. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_StirRandomCmd(&inData);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_StirRandom: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
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
    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&inData);	/* @1 */
    return rcf;
}

/* 13.8 TPM_CertifyKey rev 107

   The TPM_CertifyKey operation allows one key to certify the public portion of another key.  A TPM
   identity key may be used to certify non-migratable keys but is not permitted to certify migratory
   keys or certified migration keys. As such, it allows the TPM to make the statement "this key is
   held in a TPM-shielded location, and it will never be revealed." For this statement to have
   veracity, the Challenger must trust the policies used by the entity that issued the identity and
   the maintenance policy of the TPM manufacturer.
   
   Signing and legacy keys may be used to certify both migratable and non-migratable keys. Then the
   usefulness of a certificate depends on the trust in the certifying key by the recipient of the
   certificate.
   
   The key to be certified must be loaded before TPM_CertifyKey is called.
   
   The determination to use the TPM_CERTIFY_INFO or TPM_CERTIFY_INFO2 on the output is based on
   which PCRs and what localities the certified key is restricted to. A key to be certified that
   does not have locality restrictions and which uses no PCRs greater than PCR #15 will cause this
   command to return and sign a TPM_CERTIFY_INFO structure, which provides compatibility with V1.1
   TPMs.
   
   When this command is run to certify all other keys (those that use PCR #16 or higher, as well as
   those limited by locality in any way), it will return and sign a TPM_CERTIFY_INFO2 structure.
   
   TPM_CertifyKey does not support the case where (a) the certifying key requires a usage
   authorization to be provided but (b) the key-to-be-certified does not. In such cases,
   TPM_CertifyKey2 must be used.
   
   If a command tag (in the parameter array) specifies only one authorisation session, then the TPM
   convention is that the first session listed is ignored (authDataUsage must be TPM_AUTH_NEVER for
   this key) and the incoming session data is used for the second auth session in the list. In
   TPM_CertifyKey, the first session is the certifying key and the second session is the
   key-to-be-certified. In TPM_CertifyKey2, the first session is the key-to-be-certified and the
   second session is the certifying key.
*/

TPM_RESULT TPM_Process_CertifyKey(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE	certHandle;	/* Handle of the key to be used to certify the key. */
    TPM_KEY_HANDLE	keyHandle;	/* Handle of the key to be certified. */
    TPM_NONCE		antiReplay;	/* 160 bits of externally supplied data (typically a nonce
					   provided to prevent replay-attacks) */
    TPM_AUTHHANDLE	certAuthHandle; /* The authorization session handle used for certHandle. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with certAuthHandle
					   */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization session
						   handle */
    TPM_AUTHDATA	certAuth;	/* The authorization session digest for inputs and
					   certHandle. HMAC key: certKey.auth. */
    TPM_AUTHHANDLE	keyAuthHandle;	/* The authorization session handle used for the key to be
					   signed. */
    TPM_NONCE		keynonceOdd;	/* Nonce generated by system associated with keyAuthHandle
					   */
    TPM_BOOL	continueKeySession = TRUE;	/* The continue use flag for the authorization session
						   handle */
    TPM_AUTHDATA	keyAuth;	/* The authorization session digest for the inputs and key
					   to be signed. HMAC key: key.usageAuth. */
						     
    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_AUTH_SESSION_DATA	*cert_auth_session_data = NULL; /* session data for authHandle */
    TPM_AUTH_SESSION_DATA	*target_auth_session_data = NULL; /* session data for authHandle */
    TPM_BOOL			certAuthHandleValid = FALSE;
    TPM_BOOL			keyAuthHandleValid = FALSE;
    TPM_SECRET			*certHmacKey;
    TPM_SECRET			*targetHmacKey;
    TPM_BOOL			certPCRStatus;
    TPM_BOOL			targetPCRStatus;
    TPM_KEY			*certKey = NULL;	/* the key specified by certHandle */
    TPM_KEY			*targetKey = NULL;	/* the key specified by keyHandle */
    TPM_SECRET			*certKeyUsageAuth;
    TPM_SECRET			*targetKeyUsageAuth;
    TPM_BOOL			pcrUsage;
    TPM_LOCALITY_SELECTION	localityAtRelease;
    int				v1Version;		/* TPM 1.1 or TPM 1.2 */
    int				certifyType = 0; 	/* TPM_CERTIFY_INFO or TPM_CERTIFY_INFO2 */
    TPM_DIGEST			m1Digest;		/* digest of certifyInfo */

    /* output parameters */
    uint32_t			outParamStart;	/* starting point of outParam's */
    uint32_t			outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST			outParamDigest;
    TPM_CERTIFY_INFO		certifyInfo;	/* TPM_CERTIFY_INFO or TPM_CERTIFY_INFO2 structure
						   that provides information relative to keyhandle
						   NOTE This is c1 in the Actions. */
    TPM_CERTIFY_INFO2		certifyInfo2;
    TPM_SIZED_BUFFER		outData;	/* The signature of certifyInfo */

    printf("TPM_Process_CertifyKey: Ordinal Entry\n");
    TPM_CertifyInfo_Init(&certifyInfo);		/* freed @1 */
    TPM_CertifyInfo2_Init(&certifyInfo2);	/* freed @2 */
    TPM_SizedBuffer_Init(&outData);		/* freed @3 */
    /*
      get inputs
    */
    /* get certHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&certHandle, &command, &paramSize);
    }
    /* get keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CertifyKey: certHandle %08x\n", certHandle);
	returnCode = TPM_Load32(&keyHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get antiReplay parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CertifyKey: keyHandle %08x\n", keyHandle);
	returnCode = TPM_Nonce_Load(antiReplay, &command, &paramSize);
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
	returnCode = TPM_CheckRequestTag210(tag);
    }
    /* get the optional 'below the line' authorization parameters */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_AuthParams_Get(&certAuthHandle,
					&certAuthHandleValid,
					nonceOdd,
					&continueAuthSession,
					certAuth,
					&command, &paramSize);
    }
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	printf("TPM_Process_CertifyKey: certAuthHandle %08x\n", certAuthHandle);
    }
    /* get the optional 'below the line' authorization parameters */
    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_COMMAND)) {
	returnCode = TPM_AuthParams_Get(&keyAuthHandle,
					&keyAuthHandleValid,
					keynonceOdd,
					&continueKeySession,
					keyAuth,
					&command, &paramSize);
    }
    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_COMMAND)) {
	printf("TPM_Process_CertifyKey: keyAuthHandle %08x\n", keyAuthHandle); 
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_CertifyKey: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	certAuthHandleValid = FALSE;
	keyAuthHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* get the key corresponding to the certHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&certKey, &certPCRStatus, tpm_state, certHandle,
						 FALSE,		/* not read-only */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    /* get the key corresponding to the keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&targetKey, &targetPCRStatus, tpm_state, keyHandle,
						 FALSE,		/* not read-only */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    /* 1. The TPM validates that the key pointed to by certHandle has a signature scheme of
       TPM_SS_RSASSAPKCS1v15_SHA1 or TPM_SS_RSASSAPKCS1v15_INFO */
    if (returnCode == TPM_SUCCESS) {
	if ((certKey->algorithmParms.sigScheme != TPM_SS_RSASSAPKCS1v15_SHA1) &&
	    (certKey->algorithmParms.sigScheme != TPM_SS_RSASSAPKCS1v15_INFO)) {
	    printf("TPM_Process_CertifyKey: Error, invalid certKey sigScheme %04hx\n",
		   certKey->algorithmParms.sigScheme);
	    returnCode = TPM_BAD_KEY_PROPERTY;
	}
    }
    /* 2. Verify command and key AuthData values */
    /* a. If tag is TPM_TAG_RQU_AUTH2_COMMAND */
    /* i. The TPM verifies the AuthData in certAuthHandle provides authorization to use the key
       pointed to by certHandle, return TPM_AUTHFAIL on error */
    /* ii. The TPM verifies the AuthData in keyAuthHandle provides authorization to use the key
       pointed to by keyHandle, return TPM_AUTH2FAIL on error */
    /* b. else if tag is TPM_TAG_RQU_AUTH1_COMMAND */
    /* i. Verify that authDataUsage is TPM_AUTH_NEVER for the key referenced by certHandle, return
       TPM_AUTHFAIL on error. */
    /* ii. The TPM verifies the AuthData in keyAuthHandle provides authorization to use the key
       pointed to by keyHandle, return TPM_AUTHFAIL on error */
    /* c. else if tag is TPM_TAG_RQU_COMMAND */
    /* i. Verify that authDataUsage is TPM_AUTH_NEVER for the key referenced by certHandle, return
       TPM_AUTHFAIL on error. */
    /* ii. Verify that authDataUsage is TPM_AUTH_NEVER or TPM_NO_READ_PUBKEY_AUTH for the key
       referenced by keyHandle, return TPM_AUTHFAIL on error. */

    /* NOTE: Simplified the above logic as follows */
    /* If tag is TPM_TAG_RQU_AUTH2_COMMAND, process the first set of authorization data */
    /* get certHandle -> usageAuth */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_Key_GetUsageAuth(&certKeyUsageAuth, certKey);
    }	 
    /* get the first session data */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_AuthSessions_GetData(&cert_auth_session_data,
					      &certHmacKey,
					      tpm_state,
					      certAuthHandle,
					      TPM_PID_NONE,
					      TPM_ET_KEYHANDLE,
					      ordinal,
					      certKey,
					      certKeyUsageAuth,			/* OIAP */
					      certKey->tpm_store_asymkey->pubDataDigest); /* OSAP */
    }
    /* The TPM verifies the AuthData in certAuthHandle provides authorization to use the key
       pointed to by certHandle, return TPM_AUTHFAIL on error */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*certHmacKey,		/* HMAC key */
					inParamDigest,
					cert_auth_session_data, /* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					certAuth);		/* Authorization digest for input */
    }
    /* If tag is not TPM_TAG_RQU_AUTH2_COMMAND */
    /* Verify that authDataUsage is TPM_AUTH_NEVER for the key referenced by certHandle, return
       TPM_AUTHFAIL on error. */
    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_AUTH2_COMMAND)) {
	if (certKey->authDataUsage != TPM_AUTH_NEVER) {
	    printf("TPM_Process_CertifyKey: Error, cert key authorization required\n");
	    returnCode = TPM_AUTHFAIL;
	}
    }
    /* If tag is TPM_TAG_RQU_AUTH2_COMMAND or TPM_TAG_RQU_AUTH1_COMMAND process the second set of
       authorization data */
    /* get keyHandle -> usageAuth */
    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_COMMAND)) {
	returnCode = TPM_Key_GetUsageAuth(&targetKeyUsageAuth, targetKey);
    }
    /* get the second session data */
    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_COMMAND)) {
	returnCode = TPM_AuthSessions_GetData(&target_auth_session_data,
					      &targetHmacKey,
					      tpm_state,
					      keyAuthHandle,
					      TPM_PID_NONE,
					      TPM_ET_KEYHANDLE,
					      ordinal,
					      targetKey,
					      targetKeyUsageAuth,		/* OIAP */
					      targetKey->tpm_store_asymkey->pubDataDigest); /*OSAP*/
    }
    /* The TPM verifies the AuthData in keyAuthHandle provides authorization to use the key
       pointed to by keyHandle, return TPM_AUTH2FAIL on error */
    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_COMMAND)) {
	returnCode = TPM_Auth2data_Check(tpm_state,
					 *targetHmacKey,		/* HMAC key */
					 inParamDigest,
					 target_auth_session_data,	/* authorization session */
					 keynonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					 continueKeySession,
					 keyAuth);		/* Authorization digest for input */
    }
    /* Verify that authDataUsage is TPM_AUTH_NEVER or TPM_NO_READ_PUBKEY_AUTH for the key referenced
       by keyHandle, return TPM_AUTHFAIL on error. */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND)) {
	if (targetKey->authDataUsage == TPM_AUTH_ALWAYS) {
	    printf("TPM_Process_CertifyKey: Error, target key authorization required\n");
	    returnCode = TPM_AUTHFAIL;
	}
    }
    /* 3. If keyHandle -> payload is not TPM_PT_ASYM, return TPM_INVALID_KEYUSAGE. */
    if (returnCode == TPM_SUCCESS) {
	if (targetKey->tpm_store_asymkey->payload != TPM_PT_ASYM) {
	    printf("TPM_Process_CertifyKey: Error, target key invalid payload %02x\n",
		   targetKey->tpm_store_asymkey->payload);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }	 
    /* 4. If the key pointed to by certHandle is an identity key (certHandle -> keyUsage is
       TPM_KEY_IDENTITY) */
    if ((returnCode == TPM_SUCCESS) && (certKey->keyUsage == TPM_KEY_IDENTITY)) {
	/* a. If keyHandle -> keyflags -> keyInfo -> migratable is TRUE return TPM_MIGRATEFAIL */
	if (targetKey->keyFlags & TPM_MIGRATABLE) {
	    printf("TPM_Process_CertifyKey: Error, target key is migratable\n");
	    returnCode = TPM_MIGRATEFAIL;
	}
    }
    /* 5. Validate that certHandle -> keyUsage is TPM_KEY_SIGN, TPM_KEY_IDENTITY or TPM_KEY_LEGACY,
       if not return TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CertifyKey: certHandle -> keyUsage %04hx\n", certKey->keyUsage);
	if ((certKey->keyUsage != TPM_KEY_SIGNING) &&
	    ((certKey->keyUsage) != TPM_KEY_IDENTITY) &&
	    ((certKey->keyUsage) != TPM_KEY_LEGACY)) {
	    printf("TPM_Process_CertifyKey: Error, certHandle -> keyUsage %04hx is invalid\n",
		   certKey->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 6. Validate that keyHandle -> keyUsage is TPM_KEY_SIGN, TPM_KEY_STORAGE, TPM_KEY_IDENTITY,
       TPM_KEY_BIND or TPM_KEY_LEGACY, if not return the error code TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CertifyKey: keyHandle -> keyUsage %04hx\n", targetKey->keyUsage);
	if ((targetKey->keyUsage != TPM_KEY_SIGNING) &&
	    ((targetKey->keyUsage) != TPM_KEY_STORAGE) &&
	    ((targetKey->keyUsage) != TPM_KEY_IDENTITY) &&
	    ((targetKey->keyUsage) != TPM_KEY_BIND) &&
	    ((targetKey->keyUsage) != TPM_KEY_LEGACY)) {
	    printf("TPM_Process_CertifyKey: Error, keyHandle -> keyUsage %04hx is invalid\n",
		   targetKey->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 7. If keyHandle -> digestAtRelease requires the use of PCRs 16 or higher to calculate or if
       keyHandle -> localityAtRelease is not 0x1F */
    /* get PCR usage 16 and higher */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_GetPCRUsage(&pcrUsage, targetKey, 2);
    }
    /* get localityAtRelease */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_GetLocalityAtRelease(&localityAtRelease, targetKey);
    }
    if (returnCode == TPM_SUCCESS) {
	if (pcrUsage || (localityAtRelease != TPM_LOC_ALL)) {
	    /* a. Set V1 to 1.2 */
	    v1Version = 2;	/* locality or >2 PCR's */
	}
	/* 8. Else */
	else {
	    /* a. Set V1 to 1.1 */
	    v1Version = 1;	/* no locality and <= 2 PCR's */
	}
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CertifyKey: V1 %d\n", v1Version);
	/* 9. If keyHandle -> pcrInfoSize is not 0 */
	if (targetKey->pcrInfo.size != 0) {
	    printf("TPM_Process_CertifyKey: Setting PCR info from key\n");
	    /* a. If keyHandle -> keyFlags has pcrIgnoredOnRead set to FALSE */
	    /* i. Create a digestAtRelease according to the specified PCR registers and
	       compare to keyHandle -> digestAtRelease and if a mismatch return
	       TPM_WRONGPCRVAL */
	    /* ii. If specified validate any locality requests on error TPM_BAD_LOCALITY */
	    /* NOTE: Done by TPM_KeyHandleEntries_GetKey() */
	    /* b. If V1 is 1.1 */
	    if (v1Version == 1) {
		certifyType = 1;
		/* i. Create C1 a TPM_CERTIFY_INFO structure */
		/* NOTE: Done by TPM_CertifyInfo_Init() */
		/* ii. Fill in C1 with the information from the key pointed to by keyHandle */
		/* NOTE: Done in common _Set() code below */
		/* iii. The TPM MUST set c1 -> pcrInfoSize to 44. */
		/* iv. The TPM MUST set c1 -> pcrInfo to a TPM_PCR_INFO structure properly filled
		   out using the information from keyHandle. */
		/* This function actually creates the cache, which is serialized later */
		if (returnCode == TPM_SUCCESS) {
		    returnCode = TPM_PCRInfo_CreateFromKey(&(certifyInfo.tpm_pcr_info),
							   targetKey);
		}
		/* v. The TPM MUST set c1 -> digestAtCreation to 20 bytes of 0x00. */
		if (returnCode == TPM_SUCCESS) {
		    TPM_Digest_Init(certifyInfo.tpm_pcr_info->digestAtCreation);
		}
	    }
	    /* c. Else */
	    else {
		certifyType = 2;
		/* i. Create C1 a TPM_CERTIFY_INFO2 structure */
		/* NOTE: Done by TPM_CertifyInfo2_Init() */
		/* ii. Fill in C1 with the information from the key pointed to by keyHandle */
		/* NOTE: Done in common _Set() code below */
		/* iii. Set C1 -> pcrInfoSize to the size of an appropriate TPM_PCR_INFO_SHORT
		   structure. */
		/* iv. Set C1 -> pcrInfo to a properly filled out TPM_PCR_INFO_SHORT structure,
		   using the information from keyHandle. */
		/* This function actually creates the cache, which is serialized later */
		if (returnCode == TPM_SUCCESS) {
		    returnCode = TPM_PCRInfoShort_CreateFromKey(&(certifyInfo2.tpm_pcr_info_short), 
								targetKey);
		}
		/* v. Set C1 -> migrationAuthoritySize to 0 */
		/* NOTE: Done by TPM_CertifyInfo2_Init() */
	    }
	}
	/* 10. Else */
	else {
	    certifyType = 1;
	    /* a. Create C1 a TPM_CERTIFY_INFO structure */
	    /* NOTE: Done by TPM_CertifyInfo_Init() */
	    /* b. Fill in C1 with the information from the key pointed to be keyHandle */
	    /* NOTE: Done in common _Set() code below */
	    /* c. The TPM MUST set c1 -> pcrInfoSize to 0 */
	    /* NOTE: Done by TPM_CertifyInfo_Init() */
	}
    }
    /* 11. Create TPM_DIGEST H1 which is the SHA-1 hash of keyHandle -> pubKey -> key. Note that
       <key> is the actual public modulus, and does not include any structure formatting. */
    /* 12. Set C1 -> pubKeyDigest to H1 */
    /* NOTE: Done by TPM_CertifyInfo_Set() or TPM_CertifyInfo2_Set() */
    /* 13. The TPM copies the antiReplay parameter to c1 -> data. */
    /* Set C1 -> parentPCRStatus to the value from keyHandle NOTE: Implied in specification */
    /* Fill in C1 with the information from the key pointed to by keyHandle */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CertifyKey: Setting certifyInfo from target key\n");
	if (certifyType == 1) {
	    TPM_Digest_Copy(certifyInfo.data, antiReplay);
	    certifyInfo.parentPCRStatus = targetPCRStatus;
	    returnCode = TPM_CertifyInfo_Set(&certifyInfo, targetKey);
	}	
	else {
	    TPM_Digest_Copy(certifyInfo2.data, antiReplay);
	    certifyInfo2.parentPCRStatus = targetPCRStatus;
	    returnCode = TPM_CertifyInfo2_Set(&certifyInfo2, targetKey);
	}
    }
    /* 14. The TPM sets certifyInfo to C1. */
    /* NOTE Created as certifyInfo or certifyInfo2 */
    /* 15. The TPM creates m1, a message digest formed by taking the SHA-1 of c1. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CertifyKey: Digesting certifyInfo\n");
	if (certifyType == 1) {
	    returnCode = TPM_SHA1_GenerateStructure(m1Digest, &certifyInfo,
						    (TPM_STORE_FUNCTION_T)TPM_CertifyInfo_Store);
	}	
	else {
	    returnCode = TPM_SHA1_GenerateStructure(m1Digest, &certifyInfo2,
						    (TPM_STORE_FUNCTION_T)TPM_CertifyInfo2_Store);
	}
    }
    /* a. The TPM then computes a signature using certHandle -> sigScheme. The resulting signed blob
       is returned in outData. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CertifyKey: Signing certifyInfo digest with certifying key\n");
	returnCode = TPM_RSASignToSizedBuffer(&outData,		/* signature */
					      m1Digest,		/* message */
					      TPM_DIGEST_SIZE,	/* message size */
					      certKey);		/* input, signing key */
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_CertifyKey: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* Return certifyInfo */
	    if (certifyType == 1) {
		returnCode = TPM_CertifyInfo_Store(response, &certifyInfo);
	    }	
	    else {
		returnCode = TPM_CertifyInfo2_Store(response, &certifyInfo2);
	    }
	}
	if (returnCode == TPM_SUCCESS) {
	    /* Return outData */
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
					    *certHmacKey,		/* HMAC key */
					    cert_auth_session_data,
					    outParamDigest,
					    nonceOdd,
					    continueAuthSession);
	}
	/* calculate and set the below the line parameters */
	if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_COMMAND)) {
	    returnCode = TPM_AuthParams_Set(response,
					    *targetHmacKey,		/* HMAC key */
					    target_auth_session_data,
					    outParamDigest,
					    keynonceOdd,
					    continueKeySession);
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
	 !continueKeySession) &&
	keyAuthHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, keyAuthHandle);
    }
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	certAuthHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, certAuthHandle);
    }
    /*
      cleanup
    */
    TPM_CertifyInfo_Delete(&certifyInfo);	/* @1 */
    TPM_CertifyInfo2_Delete(&certifyInfo2);	/* @2 */
    TPM_SizedBuffer_Delete(&outData);		/* @3 */
    return rcf;
}

/* 13.9 TPM_CertifyKey2 rev 107

   This command is based on TPM_CertifyKey, but includes the ability to certify a Certifiable
   Migration Key (CMK), which requires extra input parameters.

   TPM_CertifyKey2 always produces a TPM_CERTIFY_INFO2 structure.

   TPM_CertifyKey2 does not support the case where (a) the key-to-be-certified requires a usage
   authorization to be provided but (b) the certifying key does not.

   If a command tag (in the parameter array) specifies only one authorisation session, then the TPM
   convention is that the first session listed is ignored (authDataUsage must be
   TPM_NO_READ_PUBKEY_AUTH or TPM_AUTH_NEVER for this key) and the incoming session data is used for
   the second auth session in the list. In TPM_CertifyKey2, the first session is the key to be
   certified and the second session is the certifying key.
*/

TPM_RESULT TPM_Process_CertifyKey2(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE	keyHandle;	/* Handle of the key to be certified. */
    TPM_KEY_HANDLE	certHandle;	/* Handle of the key to be used to certify the key. */
    TPM_DIGEST		migrationPubDigest;	/* The digest of a TPM_MSA_COMPOSITE structure,
						   containing at least one public key of a Migration
						   Authority */
    TPM_NONCE		antiReplay;	/* 160 bits of externally supplied data (typically a nonce
					   provided to prevent replay-attacks) */
    TPM_AUTHHANDLE	keyAuthHandle;	/* The authorization session handle used for the key to be
					   signed.  */
    TPM_NONCE		keynonceOdd;	/* Nonce generated by system associated with keyAuthHandle
					   */
    TPM_BOOL	continueKeySession;	/* The continue use flag for the authorization session
					   handle */
    TPM_AUTHDATA	keyAuth;	/* The authorization session digest for the inputs and key
					   to be signed. HMAC key: key.usageAuth. */
    TPM_AUTHHANDLE	certAuthHandle; /* The authorization session handle used for certHandle.  */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with certAuthHandle
					   */
    TPM_BOOL	continueAuthSession;	/* The continue use flag for the authorization session
					   handle */
    TPM_AUTHDATA	 certAuth;	/* Authorization HMAC key: certKey.auth. */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_AUTH_SESSION_DATA	*cert_auth_session_data = NULL; /* session data for authHandle */
    TPM_AUTH_SESSION_DATA	*target_auth_session_data = NULL; /* session data for authHandle */
    TPM_BOOL			certAuthHandleValid = FALSE;
    TPM_BOOL			keyAuthHandleValid = FALSE;
    TPM_SECRET			*certHmacKey;
    TPM_SECRET			*targetHmacKey;
    TPM_BOOL			certPCRStatus;
    TPM_BOOL			targetPCRStatus;
    TPM_KEY			*certKey = NULL;	/* the key specified by certHandle */
    TPM_KEY			*targetKey = NULL;	/* the key specified by keyHandle */
    TPM_SECRET			*certKeyUsageAuth;
    TPM_SECRET			*targetKeyUsageAuth;
    TPM_STORE_ASYMKEY		*targetStoreAsymkey;
    TPM_CMK_MIGAUTH		m2CmkMigauth;
    TPM_BOOL			hmacValid;			
    TPM_DIGEST			migrationAuthority;
    TPM_DIGEST			m1Digest;		/* digest of certifyInfo */

    /* output parameters */
    uint32_t			outParamStart;	/* starting point of outParam's */
    uint32_t			outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST			outParamDigest;
    TPM_CERTIFY_INFO2		certifyInfo2;	/* TPM_CERTIFY_INFO2 relative to keyHandle */
    TPM_SIZED_BUFFER		outData;	/* The signed public key. */

    printf("TPM_Process_CertifyKey2: Ordinal Entry\n");
    TPM_CertifyInfo2_Init(&certifyInfo2);	/* freed @1 */
    TPM_SizedBuffer_Init(&outData);		/* freed @2 */
    TPM_CmkMigauth_Init(&m2CmkMigauth);		/* freed @3 */
    /*
      get inputs
    */
    /* get keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&keyHandle, &command, &paramSize);
    }
    /* get certHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CertifyKey2: keyHandle %08x\n", keyHandle);
	returnCode = TPM_Load32(&certHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get antiReplay parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CertifyKey2: certHandle %08x\n", certHandle);
	/* get the migrationPubDigest parameter */
	returnCode = TPM_Digest_Load(migrationPubDigest, &command, &paramSize);
    }
    /* get the antiReplay parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Nonce_Load(antiReplay, &command, &paramSize);
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
	returnCode = TPM_CheckRequestTag210(tag);
    }
    /* get the optional 'below the line' authorization parameters */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_AuthParams_Get(&keyAuthHandle,
					&keyAuthHandleValid,
					keynonceOdd,
					&continueKeySession,
					keyAuth,
					&command, &paramSize);
    }
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	printf("TPM_Process_CertifyKey2: keyAuthHandle %08x\n", keyAuthHandle);
    }
    /* get the optional 'below the line' authorization parameters */
    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_COMMAND)) {
	returnCode = TPM_AuthParams_Get(&certAuthHandle,
					&certAuthHandleValid,
					nonceOdd,
					&continueAuthSession,
					certAuth,
					&command, &paramSize);
    }
    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_COMMAND)) {
	printf("TPM_Process_CertifyKey2: certAuthHandle %08x\n", certAuthHandle); 
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_CertifyKey2: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	certAuthHandleValid = FALSE;
	keyAuthHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* get the keys corresponding to the certHandle and keyHandle parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&targetKey, &targetPCRStatus, tpm_state, keyHandle,
						 FALSE,		/* not read-only */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&certKey, &certPCRStatus, tpm_state, certHandle,
						 FALSE,		/* not read-only */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    /* get the TPM_STORE_ASYMKEY cache for the target TPM_KEY */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_GetStoreAsymkey(&targetStoreAsymkey, targetKey);
    }	 
    /* 1. The TPM validates that the key pointed to by certHandle has a signature scheme of
       TPM_SS_RSASSAPKCS1v15_SHA1 or TPM_SS_RSASSAPKCS1v15_INFO */
    if (returnCode == TPM_SUCCESS) {
	if ((certKey->algorithmParms.sigScheme != TPM_SS_RSASSAPKCS1v15_SHA1) &&
	    (certKey->algorithmParms.sigScheme != TPM_SS_RSASSAPKCS1v15_INFO)) {
	    printf("TPM_Process_CertifyKey2: Error, invalid certKey sigScheme %04hx\n",
		   certKey->algorithmParms.sigScheme);
	    returnCode = TPM_BAD_KEY_PROPERTY;
	}
    }
    /* 2. Verify command and key AuthData values: */
    /* a. If tag is TPM_TAG_RQU_AUTH2_COMMAND */
    /* i. The TPM verifies the AuthData in keyAuthHandle provides authorization to use the key
       pointed to by keyHandle, return TPM_AUTHFAIL on error */
    /* ii. The TPM verifies the AuthData in certAuthHandle provides authorization to use the key
       pointed to by certHandle, return TPM_AUTH2FAIL on error */
    /* b. else if tag is TPM_TAG_RQU_AUTH1_COMMAND */
    /* i. Verify that authDataUsage is TPM_AUTH_NEVER or TPM_NO_READ_PUBKEY_AUTH for the key
       referenced by keyHandle, return TPM_AUTHFAIL on error */
    /* ii. The TPM verifies the AuthData in certAuthHandle provides authorization to use the key
       pointed to by certHandle, return TPM_AUTHFAIL on error */
    /* c. else if tag is TPM_TAG_RQU_COMMAND */
    /* i. Verify that authDataUsage is TPM_AUTH_NEVER or TPM_NO_READ_PUBKEY_AUTH for the key
       referenced by keyHandle, return TPM_AUTHFAIL on error */
    /* ii. Verify that authDataUsage is TPM_AUTH_NEVER for the key referenced by certHandle, return
       TPM_AUTHFAIL on error. */
    /* NOTE: Simplified the above logic as follows */
    /* If tag is TPM_TAG_RQU_AUTH2_COMMAND, process the first set of authorization data */
    /* get keyHandle -> usageAuth */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_Key_GetUsageAuth(&targetKeyUsageAuth, targetKey);
    }	 
    /* get the first session data */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_AuthSessions_GetData(&target_auth_session_data,
					      &targetHmacKey,
					      tpm_state,
					      keyAuthHandle,
					      TPM_PID_NONE,
					      TPM_ET_KEYHANDLE,
					      ordinal,
					      targetKey,
					      targetKeyUsageAuth,			/* OIAP */
					      targetKey->tpm_store_asymkey->pubDataDigest); /*OSAP*/
    }
    /* The TPM verifies the AuthData in keyAuthHandle provides authorization to use the key
       pointed to by keyHandle, return TPM_AUTHFAIL on error */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*targetHmacKey,		/* HMAC key */
					inParamDigest,
					target_auth_session_data,	/* authorization session */
					keynonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueKeySession,
					keyAuth);		/* Authorization digest for input */
    }
    /* If tag is not TPM_TAG_RQU_AUTH2_COMMAND */
    /* Verify that authDataUsage is TPM_AUTH_NEVER or TPM_NO_READ_PUBKEY_AUTH for the key referenced
       by keyHandle, return TPM_AUTHFAIL on error. */
    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_AUTH2_COMMAND)) {
	if (targetKey->authDataUsage == TPM_AUTH_ALWAYS) {
	    printf("TPM_Process_CertifyKey2: Error, target key authorization required\n");
	    returnCode = TPM_AUTHFAIL;
	}
    }
    /* If tag is TPM_TAG_RQU_AUTH2_COMMAND or TPM_TAG_RQU_AUTH1_COMMAND process the second set of
       authorization data */
    /* get certHandle -> usageAuth */
    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_COMMAND)) {
	returnCode = TPM_Key_GetUsageAuth(&certKeyUsageAuth, certKey);
    }
    /* get the second session data */
    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_COMMAND)) {
	returnCode = TPM_AuthSessions_GetData(&cert_auth_session_data,
					      &certHmacKey,
					      tpm_state,
					      certAuthHandle,
					      TPM_PID_NONE,
					      TPM_ET_KEYHANDLE,
					      ordinal,
					      certKey,
					      certKeyUsageAuth,			/* OIAP */
					      certKey->tpm_store_asymkey->pubDataDigest); /* OSAP */
    }
    /* The TPM verifies the AuthData in certAuthHandle provides authorization to use the key
       pointed to by certHandle, return TPM_AUTH2FAIL on error */
    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_COMMAND)) {
	returnCode = TPM_Auth2data_Check(tpm_state,
					 *certHmacKey,			/* HMAC key */
					 inParamDigest,
					 cert_auth_session_data,	/* authorization session */
					 nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					 continueAuthSession,
					 certAuth);		/* Authorization digest for input */
    }
    /* If the command is TPM_TAG_RQU_COMMAND */
    /* Verify that authDataUsage is TPM_AUTH_NEVER for the key referenced by certHandle, return
       TPM_AUTHFAIL on error. */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND)) {
	if (certKey->authDataUsage != TPM_AUTH_NEVER) {
	    printf("TPM_Process_CertifyKey2: Error, cert key authorization required\n");
	    returnCode = TPM_AUTHFAIL;
	}
    }
    /* 3. If the key pointed to by certHandle is an identity key (certHandle -> keyUsage is
       TPM_KEY_IDENTITY) */
    if ((returnCode == TPM_SUCCESS) && (certKey->keyUsage == TPM_KEY_IDENTITY)) {
	/* a. If keyHandle -> keyFlags -> migratable is TRUE and
	   [keyHandle -> keyFlags-> migrateAuthority is FALSE or
	    (keyHandle -> payload != TPM_PT_MIGRATE_RESTRICTED and
	    keyHandle -> payload != TPM_PT_MIGRATE_EXTERNAL)]
	    return TPM_MIGRATEFAIL */
	if ((targetKey->keyFlags & TPM_MIGRATABLE) &&
	    (!(targetKey->keyFlags & TPM_MIGRATEAUTHORITY) ||
	     ((targetStoreAsymkey->payload != TPM_PT_MIGRATE_RESTRICTED) &&
	      (targetStoreAsymkey->payload != TPM_PT_MIGRATE_EXTERNAL)))) {
	    printf("TPM_Process_CertifyKey2: Error, target key migrate fail\n");
	    returnCode = TPM_MIGRATEFAIL;
	}
    }
    /* 4. Validate that certHandle -> keyUsage is TPM_KEY_SIGNING, TPM_KEY_IDENTITY or
       TPM_KEY_LEGACY, if not return the error code TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CertifyKey2: certHandle ->keyUsage %04hx\n", certKey->keyUsage);
	if ((certKey->keyUsage != TPM_KEY_SIGNING) &&
	    ((certKey->keyUsage) != TPM_KEY_IDENTITY) &&
	    ((certKey->keyUsage) != TPM_KEY_LEGACY)) {
	    printf("TPM_Process_CertifyKey2: Error, keyUsage %04hx is invalid\n",
		   certKey->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 5. Validate that keyHandle -> keyUsage is TPM_KEY_SIGNING, TPM_KEY_STORAGE, TPM_KEY_IDENTITY,
       TPM_KEY_BIND or TPM_KEY_LEGACY, if not return the error code TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CertifyKey2: keyHandle -> keyUsage %04hx\n", targetKey->keyUsage);
	if ((targetKey->keyUsage != TPM_KEY_SIGNING) &&
	    ((targetKey->keyUsage) != TPM_KEY_STORAGE) &&
	    ((targetKey->keyUsage) != TPM_KEY_IDENTITY) &&
	    ((targetKey->keyUsage) != TPM_KEY_BIND) &&
	    ((targetKey->keyUsage) != TPM_KEY_LEGACY)) {
	    printf("TPM_Process_CertifyKey2: Error, keyHandle -> keyUsage %04hx is invalid\n",
		   targetKey->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 6. The TPM SHALL create a c1 a TPM_CERTIFY_INFO2 structure from the key pointed to by
       keyHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CertifyInfo2_Set(&certifyInfo2, targetKey);
    }
    /* 7. Create TPM_DIGEST H1 which is the SHA-1 hash of keyHandle -> pubKey -> key. Note that
       <key> is the actual public modulus, and does not include any structure formatting. */
    /* 8. Set C1 -> pubKeyDigest to H1 */
    /* NOTE: Done by TPM_CertifyInfo2_Set() */
    if (returnCode == TPM_SUCCESS) {
	/* 9. Copy the antiReplay parameter to c1 -> data */
	TPM_Digest_Copy(certifyInfo2.data, antiReplay);
	/* 10. Copy other keyHandle parameters into C1 */
	certifyInfo2.parentPCRStatus = targetPCRStatus;
	/* 11. If keyHandle -> payload == TPM_PT_MIGRATE_RESTRICTED or TPM_PT_MIGRATE_EXTERNAL */
	if ((targetStoreAsymkey->payload == TPM_PT_MIGRATE_RESTRICTED) ||
	    (targetStoreAsymkey->payload == TPM_PT_MIGRATE_EXTERNAL)) {
	    printf("TPM_Process_CertifyKey2: "
		   "TPM_PT_MIGRATE_RESTRICTED or TPM_PT_MIGRATE_RESTRICTED\n");
	    /* a. create thisPubKey, a TPM_PUBKEY structure containing the public key, algorithm and
	       parameters corresponding to keyHandle */
	    /* NOTE Not required.  Digest is created directly below */
	    /* b. Verify that the migration authorization is valid for this key */
	    /* i. Create M2 a TPM_CMK_MIGAUTH structure */
	    /* NOTE Done by TPM_CmkMigauth_Init() */
	    if (returnCode == TPM_SUCCESS) {
		/* ii. Set M2 -> msaDigest to migrationPubDigest */
		TPM_Digest_Copy(m2CmkMigauth.msaDigest, migrationPubDigest );
		/* iii. Set M2 -> pubKeyDigest to SHA-1[thisPubKey] */
		returnCode = TPM_Key_GeneratePubkeyDigest(m2CmkMigauth.pubKeyDigest, targetKey);
	    }
	    /* iv. Verify that [keyHandle -> migrationAuth] == HMAC(M2) signed by using tpmProof as
	       the secret and return error TPM_MA_SOURCE on mismatch */
	    if (returnCode == TPM_SUCCESS) {
		printf("TPM_Process_CertifyKey2: Check migrationAuth\n");
		returnCode =
		    TPM_CmkMigauth_CheckHMAC(&hmacValid,				/* result */
					     targetStoreAsymkey->migrationAuth,		/* expect */
					     tpm_state->tpm_permanent_data.tpmProof, /* HMAC key */
					     &m2CmkMigauth);
	    }
	    if (returnCode == TPM_SUCCESS) {
		if (!hmacValid) {
		    printf("TPM_Process_CertifyKey2: Error, Invalid migrationAuth\n");
		    returnCode = TPM_MA_SOURCE;
		}
	    }
	    /* c. Set C1 -> migrationAuthority = SHA-1(migrationPubDigest || keyHandle -> payload)
	       */
	    if (returnCode == TPM_SUCCESS) {
		printf("TPM_Process_CertifyKey2: Set migrationAuthority\n");
		returnCode = TPM_SHA1(migrationAuthority,
				      TPM_DIGEST_SIZE, migrationPubDigest,
				      sizeof(TPM_PAYLOAD_TYPE), &(targetStoreAsymkey->payload),
				      0, NULL);
	    }
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_SizedBuffer_Set(&(certifyInfo2.migrationAuthority),
						 TPM_DIGEST_SIZE, migrationAuthority);
	    }	    
	    /* d. if keyHandle -> payload == TPM_PT_MIGRATE_RESTRICTED */
	    /* i. Set C1 -> payloadType = TPM_PT_MIGRATE_RESTRICTED */
	    /* e. if keyHandle -> payload == TPM_PT_MIGRATE_EXTERNAL */
	    /* i. Set C1 -> payloadType = TPM_PT_MIGRATE_EXTERNAL */
	    /* NOTE: Done by TPM_CertifyInfo2_Set() */
	}
	/* 12. Else */
	else {
	    printf("TPM_Process_CertifyKey2: "
		   " Not TPM_PT_MIGRATE_RESTRICTED or TPM_PT_MIGRATE_RESTRICTED\n");
	    /* a. set C1 -> migrationAuthority = NULL */
	    /* b. set C1 -> migrationAuthoritySize = 0 */
	    /* NOTE: Done by TPM_CertifyInfo2_Init() */
	    /* c. Set C1 -> payloadType = TPM_PT_ASYM */
	    certifyInfo2.payloadType = TPM_PT_ASYM;
	}
    }
    if (returnCode == TPM_SUCCESS) {
	/* 13. If keyHandle -> pcrInfoSize is not 0 */
	if (targetKey->pcrInfo.size != 0) {
	    printf("TPM_Process_CertifyKey2: Setting PCR info from key\n");
	    /* a. The TPM MUST set c1 -> pcrInfoSize to match the pcrInfoSize from the keyHandle
	       key. */
	    /* b. The TPM MUST set c1 -> pcrInfo to match the pcrInfo from the keyHandle key */
	    /* This function actually creates the cache, which is serialized later */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_PCRInfoShort_CreateFromKey(&(certifyInfo2.tpm_pcr_info_short),
							    targetKey);
	    }
	    /* c. If keyHandle -> keyFlags has pcrIgnoredOnRead set to FALSE */
	    /* i. Create a digestAtRelease according to the specified PCR registers and compare to
	       keyHandle -> digestAtRelease and if a mismatch return TPM_WRONGPCRVAL */
	    /* ii. If specified validate any locality requests on error TPM_BAD_LOCALITY */
	    /* NOTE: Done by TPM_KeyHandleEntries_GetKey() */
	}
	/* 14. Else */
	/* a. The TPM MUST set c1 -> pcrInfoSize to 0 */
	/* NOTE: Done by TPM_CertifyInfo2_Init() */
    }
    /* 15. The TPM creates m1, a message digest formed by taking the SHA-1 of c1 */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CertifyKey2: Digesting certifyInfo\n");
	returnCode = TPM_SHA1_GenerateStructure(m1Digest, &certifyInfo2,
						(TPM_STORE_FUNCTION_T)TPM_CertifyInfo2_Store);
    }
    /* a. The TPM then computes a signature using certHandle -> sigScheme. The resulting signed blob
       is returned in outData */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CertifyKey2: Signing certifyInfo digest\n");
	returnCode = TPM_RSASignToSizedBuffer(&outData,		/* signature */
					      m1Digest,		/* message */
					      TPM_DIGEST_SIZE,	/* message size */
					      certKey);		/* input, signing key */
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_CertifyKey2: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* Return certifyInfo */
	    returnCode = TPM_CertifyInfo2_Store(response, &certifyInfo2);
	}
	if (returnCode == TPM_SUCCESS) {
	    /* Return outData */
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
					    *targetHmacKey,		/* HMAC key */
					    target_auth_session_data,
					    outParamDigest,
					    keynonceOdd,
					    continueKeySession);
	}
	/* calculate and set the below the line parameters */
	if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_COMMAND)) {
	    returnCode = TPM_AuthParams_Set(response,
					    *certHmacKey,		/* HMAC key */
					    cert_auth_session_data,
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
	 !continueKeySession) &&
	keyAuthHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, keyAuthHandle);
    }
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	certAuthHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, certAuthHandle);
    }
    /*
      cleanup
    */
    TPM_CertifyInfo2_Delete(&certifyInfo2);	/* @1 */
    TPM_SizedBuffer_Delete(&outData);		/* @2 */
    TPM_CmkMigauth_Delete(&m2CmkMigauth);	/* @3 */
    return rcf;
}

/* 28.3 TPM_CertifySelfTest rev 94

  CertifySelfTest causes the TPM to perform a full self-test and return an authenticated value if
  the test passes.

  If a caller itself requires proof, it is sufficient to use any signing key for which only the TPM
  and the caller have AuthData.

  If a caller requires proof for a third party, the signing key must be one whose signature is
  trusted by the third party. A TPM-identity key may be suitable.

  Information returned by TPM_CertifySelfTest MUST NOT aid identification of an individual TPM.
*/

TPM_RESULT TPM_Process_CertifySelfTest(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE	keyHandle;	/* The keyHandle identifier of a loaded key that can perform
					   digital signatures. */
    TPM_NONCE		antiReplay;	/* AntiReplay nonce to prevent replay of messages */
    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle used for keyHandle
					   authorization */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization session
						   handle */
    TPM_AUTHDATA	privAuth;	/* The authorization session digest that authorizes the
					   inputs and use of keyHandle. HMAC key: key.usageAuth */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_KEY			*sigKey;			/* from keyHandle */
    TPM_BOOL			sigKeyPCRStatus;
    TPM_SECRET			*sigKeyUsageAuth;
    TPM_COMMAND_CODE		nOrdinal;			/* ordinal in nbo */
    TPM_DIGEST			m2Digest;			/* message to sign */

    /* output parameters */
    uint32_t			outParamStart;	/* starting point of outParam's */
    uint32_t			outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST			outParamDigest;
    TPM_SIZED_BUFFER		sig;		/* The resulting digital signature. */

    printf("TPM_Process_CertifySelfTest: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&sig);			/* freed @1 */
    /*
      get inputs
    */
    /* get keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&keyHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get the antiReplay parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CertifySelfTest: keyHandle %08x\n", keyHandle);
	returnCode = TPM_Nonce_Load(antiReplay, &command, &paramSize);
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
					privAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_CertifySelfTest: Error, command has %u extra bytes\n",
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
    /* 1. The TPM SHALL perform TPM_SelfTestFull. If the test fails the TPM returns the appropriate
       error code. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CertifySelfTest: Running self test\n");
	returnCode = TPM_SelfTestFullCmd(tpm_state);
    }
    /* 2. After successful completion of the self-test the TPM then validates the authorization to
       use the key pointed to by keyHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&sigKey, &sigKeyPCRStatus, tpm_state, keyHandle,
						 FALSE,		/* not read-only */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND)){
	if (sigKey->authDataUsage != TPM_AUTH_NEVER) {	
	    printf("TPM_Process_CertifySelfTest: Error, authorization required\n");
	    returnCode = TPM_AUTHFAIL;
	}
    }
    /* get keyHandle -> usageAuth */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_Key_GetUsageAuth(&sigKeyUsageAuth, sigKey);
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
					      sigKey,
					      sigKeyUsageAuth,		/* OIAP */
					      sigKey->tpm_store_asymkey->pubDataDigest); /* OSAP */
    }
    /* Validate the command parameters using privAuth */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					privAuth);		/* Authorization digest for input */
    }
    /* a. If the key pointed to by keyHandle has a signature scheme that is not
       TPM_SS_RSASSAPKCS1v15_SHA1, the TPM may either return TPM_BAD_SCHEME or may return
       TPM_SUCCESS and a vendor specific signature. */
    if (returnCode == TPM_SUCCESS) {
	if (sigKey->algorithmParms.sigScheme != TPM_SS_RSASSAPKCS1v15_SHA1) {
	    printf("TPM_Process_CertifySelfTest: Error, invalid sigKey sigScheme %04hx\n",
		   sigKey->algorithmParms.sigScheme);
	    returnCode = TPM_BAD_SCHEME;
	}
    }
    /* The key in keyHandle MUST have a KEYUSAGE value of type TPM_KEY_SIGNING or TPM_KEY_LEGACY or
       TPM_KEY_IDENTITY. */
    if (returnCode == TPM_SUCCESS) {
	if ((sigKey->keyUsage != TPM_KEY_SIGNING) &&
	    (sigKey->keyUsage != TPM_KEY_LEGACY) &&
	    (sigKey->keyUsage != TPM_KEY_IDENTITY)) {
	    printf("TPM_Process_CertifySelfTest: Error, Illegal keyUsage %04hx\n",
		   sigKey->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }	 
    /* 3. Create t1 the NOT null terminated string of "Test Passed" */
    /* 4. The TPM creates m2 the message to sign by concatenating t1 || AntiReplay || ordinal. */
    if (returnCode == TPM_SUCCESS) {
	nOrdinal = htonl(ordinal);
	returnCode = TPM_SHA1(m2Digest,
			      sizeof("Test Passed") - 1, "Test Passed",
			      TPM_NONCE_SIZE, antiReplay,
			      sizeof(TPM_COMMAND_CODE), &nOrdinal,
			      0, NULL);
    }
    /* 5. The TPM signs the SHA-1 of m2 using the key identified by keyHandle, and returns the
       signature as sig. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CertifySelfTest: Signing certifyInfo digest\n");
	returnCode = TPM_RSASignToSizedBuffer(&sig,		/* signature */
					      m2Digest,		/* message */
					      TPM_DIGEST_SIZE,	/* message size */
					      sigKey);		/* input, signing key */
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_CertifySelfTest: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return sig */
	    returnCode = TPM_SizedBuffer_Store(response, &sig);
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
    TPM_SizedBuffer_Delete(&sig);			/* @1 */
    return rcf;
}
