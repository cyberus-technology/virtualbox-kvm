/********************************************************************************/
/*										*/
/*				Storage Functions				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_storage.c $		*/
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
#include "tpm_cryptoh.h"
#include "tpm_crypto.h"
#include "tpm_debug.h"
#include "tpm_digest.h"
#include "tpm_error.h"
#include "tpm_io.h"
#include "tpm_key.h"
#include "tpm_memory.h"
#include "tpm_nonce.h"
#include "tpm_pcr.h"
#include "tpm_process.h"
#include "tpm_secret.h"
#include "tpm_structures.h"
#include "tpm_ver.h"

#include "tpm_storage.h"

/* local function prototypes */

static TPM_RESULT TPM_SealCryptCommon(BYTE **o1,
				      TPM_ADIP_ENC_SCHEME adipEncScheme,
				      TPM_SIZED_BUFFER *inData,
				      TPM_AUTH_SESSION_DATA *auth_session_data,
				      TPM_NONCE nonceOdd);

static TPM_RESULT TPM_LoadKeyCommon(TPM_KEY_HANDLE	*inKeyHandle,
				    TPM_BOOL		*key_added,
				    TPM_SECRET		**hmacKey,
				    TPM_AUTH_SESSION_DATA	**auth_session_data,
				    tpm_state_t		*tpm_state,
				    TPM_TAG		tag,
				    TPM_COMMAND_CODE	ordinal,
				    TPM_KEY_HANDLE	parentHandle,
				    TPM_KEY		*inKey,
				    TPM_DIGEST		inParamDigest,
				    TPM_AUTHHANDLE	authHandle,
				    TPM_NONCE		nonceOdd,
				    TPM_BOOL		continueAuthSession,
				    TPM_AUTHDATA	parentAuth);

/*
  TPM_BOUND_DATA
*/

/* TPM_BoundData_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_BoundData_Init(TPM_BOUND_DATA *tpm_bound_data)
{
    printf(" TPM_BoundData_Init:\n");
    TPM_StructVer_Init(&(tpm_bound_data->ver));
    tpm_bound_data->payload = TPM_PT_BIND;
    tpm_bound_data->payloadDataSize = 0;
    tpm_bound_data->payloadData = NULL;
    return;
}

/* TPM_BoundData_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_BoundData_Init()
   After use, call TPM_BoundData_Delete() to free memory
*/

TPM_RESULT TPM_BoundData_Load(TPM_BOUND_DATA *tpm_bound_data,
			      unsigned char **stream,
			      uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_BoundData_Load:\n");
    if (rc == 0) {
	rc = TPM_StructVer_Load(&(tpm_bound_data->ver), stream, stream_size);
    }
    /* check ver immediately to ease debugging */
    if (rc == 0) {
	rc = TPM_StructVer_CheckVer(&(tpm_bound_data->ver));
    }
    if (rc == 0) {
	rc = TPM_Load8(&(tpm_bound_data->payload), stream, stream_size);
    }
    if ((rc == 0) && (*stream_size > 0)){
	/* There is no payloadData size in the serialized data.	 Assume it consumes the rest of the
	   stream */
	tpm_bound_data->payloadDataSize = *stream_size;
	rc = TPM_Malloc(&(tpm_bound_data->payloadData), tpm_bound_data->payloadDataSize);
    }
    if ((rc == 0) && (*stream_size > 0)){
	memcpy(tpm_bound_data->payloadData, *stream, tpm_bound_data->payloadDataSize);
	*stream += tpm_bound_data->payloadDataSize;
	*stream_size -= tpm_bound_data->payloadDataSize;
    }
    return rc;
}

#if 0
/* TPM_BoundData_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes

   This structure serialization assumes that the payloadDataSize member indicates the size of
   payloadData.
*/

TPM_RESULT TPM_BoundData_Store(TPM_STORE_BUFFER *sbuffer,
			       const TPM_BOUND_DATA *tpm_bound_data)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_BoundData_Store:\n");
    if (rc == 0) {
	rc = TPM_StructVer_Store(sbuffer, &(tpm_bound_data->ver));
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_bound_data->payload), sizeof(TPM_PAYLOAD_TYPE));
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, tpm_bound_data->payloadData,
				tpm_bound_data->payloadDataSize);
    }
    return rc;
}
#endif

/* TPM_BoundData_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the bound_data
   sets pointers to NULL
   calls TPM_BoundData_Init to set members back to default values
   The bound_data itself is not freed
*/   

void TPM_BoundData_Delete(TPM_BOUND_DATA *tpm_bound_data)
{
    printf(" TPM_BoundData_Delete:\n");
    if (tpm_bound_data != NULL) {
	free(tpm_bound_data->payloadData);
	TPM_BoundData_Init(tpm_bound_data);
    }
    return;
}

/*
  TPM_SEALED_DATA
*/

/* TPM_SealedData_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_SealedData_Init(TPM_SEALED_DATA *tpm_sealed_data)
{
    printf(" TPM_SealedData_Init:\n");
    tpm_sealed_data->payload = TPM_PT_SEAL;
    TPM_Secret_Init(tpm_sealed_data->authData);
    TPM_Secret_Init(tpm_sealed_data->tpmProof);
    TPM_Digest_Init(tpm_sealed_data->storedDigest);
    TPM_SizedBuffer_Init(&(tpm_sealed_data->data));
    return;
}

/* TPM_SealedData_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_SealedData_Init()
   After use, call TPM_SealedData_Delete() to free memory
*/

TPM_RESULT TPM_SealedData_Load(TPM_SEALED_DATA *tpm_sealed_data,
			       unsigned char **stream,
			       uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_SealedData_Load:\n");
    /* load payload */
    if (rc == 0) {
	rc = TPM_Load8(&(tpm_sealed_data->payload), stream, stream_size);
    }
    /* load authData */
    if (rc == 0) {
	rc = TPM_Secret_Load(tpm_sealed_data->authData, stream, stream_size);
    }
    /* load tpmProof */
    if (rc == 0) {
	rc = TPM_Secret_Load(tpm_sealed_data->tpmProof, stream, stream_size);
    }
    /* load storedDigest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_sealed_data->storedDigest, stream, stream_size);
    }
    /* load dataSize and data  */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_sealed_data->data), stream, stream_size);
    }
    return rc;
}

/* TPM_SealedData_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_SealedData_Store(TPM_STORE_BUFFER *sbuffer,
				const TPM_SEALED_DATA *tpm_sealed_data)
{
    TPM_RESULT		rc = 0;
    printf(" TPM_SealedData_Store:\n");
    /* store payload */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_sealed_data->payload), sizeof(TPM_PAYLOAD_TYPE));
    }
    /* store authData */
    if (rc == 0) {
	rc = TPM_Secret_Store(sbuffer, tpm_sealed_data->authData);
    }
    /* store tpmProof */
    if (rc == 0) {
	rc = TPM_Secret_Store(sbuffer, tpm_sealed_data->tpmProof);
    }
    /* store storedDigest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_sealed_data->storedDigest);
    }
    /* store dataSize and data	*/
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_sealed_data->data));
    }
    return rc;
}

/* TPM_SealedData_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_SealedData_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_SealedData_Delete(TPM_SEALED_DATA *tpm_sealed_data)
{
    printf(" TPM_SealedData_Delete:\n");
    if (tpm_sealed_data != NULL) {
	TPM_SizedBuffer_Delete(&(tpm_sealed_data->data));
	TPM_SealedData_Init(tpm_sealed_data);
    }
    return;
}

/* TPM_SealedData_GenerateEncData() generates an enc_data structure by serializing the
   TPM_SEALED_DATA structure and encrypting the result using the public key.
*/

TPM_RESULT TPM_SealedData_GenerateEncData(TPM_SIZED_BUFFER *enc_data,
					  const TPM_SEALED_DATA *tpm_sealed_data,
					  TPM_KEY *tpm_key)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	sbuffer;		/* TPM_SEALED_DATA serialization */

    printf(" TPM_SealedData_GenerateEncData\n");
    TPM_Sbuffer_Init(&sbuffer);			/* freed @1 */
    /* serialize the TPM_SEALED_DATA */
    if (rc == 0) {
	rc = TPM_SealedData_Store(&sbuffer, tpm_sealed_data);
    }
    /* encrypt the TPM_SEALED_DATA serialization buffer with the public key, and place
       the result in the encData members */
    if (rc == 0) {
	rc = TPM_RSAPublicEncryptSbuffer_Key(enc_data, &sbuffer, tpm_key);
    }
    TPM_Sbuffer_Delete(&sbuffer);	/* @1 */
    return rc;
}

/* TPM_SealedData_DecryptEncData() decrypts the enc_data using the private key.	 The
   result is deserialized and stored in the TPM_SEALED_DATA structure.

*/

TPM_RESULT TPM_SealedData_DecryptEncData(TPM_SEALED_DATA *tpm_sealed_data,	/* result */
					 TPM_SIZED_BUFFER *enc_data,	/* encrypted input */
					 TPM_KEY *tpm_key)		/* key for decrypting */
{
    TPM_RESULT		rc = 0;
    unsigned char	*decryptData = NULL;	/* freed @1 */
    uint32_t		decryptDataLength = 0;	/* actual valid data */
    unsigned char	*stream;
    uint32_t		stream_size;
    
    printf(" TPM_SealedData_DecryptEncData:\n");
    /* allocate space for the decrypted data */
    if (rc == 0) {
	rc = TPM_RSAPrivateDecryptMalloc(&decryptData,		/* decrypted data */
					 &decryptDataLength,	/* actual size of decrypted data */
					 enc_data->buffer,	/* encrypted data */
					 enc_data->size,	/* encrypted data size */
					 tpm_key);
    }
    /* load the TPM_SEALED_DATA structure from the decrypted data stream */
    if (rc == 0) {
	/* use temporary variables, because TPM_SealedData_Load() moves the stream */
	stream = decryptData;
	stream_size = decryptDataLength;
	rc = TPM_SealedData_Load(tpm_sealed_data, &stream, &stream_size);
    }
    free(decryptData);		/* @1 */
    return rc;
}


/*
  TPM_STORED_DATA
*/

/* TPM_StoredData_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_StoredData_Init(TPM_STORED_DATA *tpm_stored_data,
			 unsigned int version)
{
    printf(" TPM_StoredData_Init: v%u\n", version);
    if (version == 1) {
	TPM_StructVer_Init(&(tpm_stored_data->ver));
    }
    else {
	((TPM_STORED_DATA12 *)tpm_stored_data)->tag = TPM_TAG_STORED_DATA12;
	((TPM_STORED_DATA12 *)tpm_stored_data)->et = 0x0000;
    }
    TPM_SizedBuffer_Init(&(tpm_stored_data->sealInfo));
    TPM_SizedBuffer_Init(&(tpm_stored_data->encData));
    tpm_stored_data->tpm_seal_info = NULL;
    return;
}

/* TPM_StoredData_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_StoredData_Init()
   After use, call TPM_StoredData_Delete() to free memory

   This function handles both TPM_STORED_DATA and TPM_STORED_DATA12 and returns the 'version'.
*/

TPM_RESULT TPM_StoredData_Load(TPM_STORED_DATA *tpm_stored_data,
			       unsigned int *version,
			       unsigned char **stream,
			       uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    /* Peek at the first byte to guess the version number.  The data is verified later.
       TPM_STORED_DATA is 01,01,00,00 TPM_STORED_DATA12 is 00,16,00,00 */
    if ((rc == 0) && (*stream_size > 0)) {
	if (**stream == 0x01) {
	    *version = 1;
	}
	else {
	    *version = 2;
	}
	printf(" TPM_StoredData_Load: v%u\n", *version);
    }	 
    /* 1.1 load ver */
    if ((rc == 0) && (*version == 1)) {
	rc = TPM_StructVer_Load(&(tpm_stored_data->ver), stream, stream_size);
    }
    /* 1.2 load tag */
    if ((rc == 0) && (*version != 1)) {
	rc = TPM_Load16(&(((TPM_STORED_DATA12 *)tpm_stored_data)->tag), stream, stream_size);
    }
    /* 1.2 load et */
    if ((rc == 0) && (*version != 1)) {
	rc = TPM_Load16(&(((TPM_STORED_DATA12 *)tpm_stored_data)->et), stream, stream_size);
    }
    /* check the TPM_STORED_DATA structure version */
    if ((rc == 0) && (*version == 1)) {
	rc = TPM_StructVer_CheckVer(&(tpm_stored_data->ver));
    }
    /* check the TPM_STORED_DATA12 structure tag */
    if ((rc == 0) && (*version != 1)) {
	rc = TPM_StoredData_CheckTag((TPM_STORED_DATA12 *)tpm_stored_data);
    }
    /* load sealInfoSize and sealInfo */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_stored_data->sealInfo), stream, stream_size);
    }
    /* load the TPM_PCR_INFO or TPM_PCR_INFO_LONG cache */
    if (rc == 0) {
	if (*version == 1) {
	    rc = TPM_PCRInfo_CreateFromBuffer(&(tpm_stored_data->tpm_seal_info),
					      &(tpm_stored_data->sealInfo));
	}
	else {
	    rc = TPM_PCRInfoLong_CreateFromBuffer
		 (&(((TPM_STORED_DATA12 *)tpm_stored_data)->tpm_seal_info_long),
		  &(tpm_stored_data->sealInfo));
	}
    }
    /* load encDataSize and encData */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_stored_data->encData), stream, stream_size);
    }
    return rc;
}

/* TPM_StoredData_StoreClearData() serializes a TPM_STORED_DATA structure, excluding encData,
   appending results to 'sbuffer'.

   Before serializing, it serializes tpm_seal_info to sealInfoSize and sealInfo.

   This function handles both TPM_STORED_DATA and TPM_STORED_DATA12.
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_StoredData_StoreClearData(TPM_STORE_BUFFER *sbuffer,
					 TPM_STORED_DATA *tpm_stored_data,
					 unsigned int version)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_StoredData_StoreClearData: v%u\n", version);
    /* 1.1 store ver */
    if ((rc == 0) && (version == 1)) {
	rc = TPM_StructVer_Store(sbuffer, &(tpm_stored_data->ver));
    }
    /* 1.2 store tag */
    if ((rc == 0) && (version != 1)) {
	rc = TPM_Sbuffer_Append16(sbuffer, ((TPM_STORED_DATA12 *)tpm_stored_data)->tag);
    }
    /* 1.2 store et */
    if ((rc == 0) && (version != 1)) {
	rc = TPM_Sbuffer_Append16(sbuffer, ((TPM_STORED_DATA12 *)tpm_stored_data)->et);
    }
    /* store sealInfoSize and sealInfo */
    if (rc == 0) {
	/* copy cache to sealInfoSize and sealInfo */
	if (version == 1) {
	    rc = TPM_SizedBuffer_SetStructure(&(tpm_stored_data->sealInfo),
					      tpm_stored_data->tpm_seal_info,
					      (TPM_STORE_FUNCTION_T)TPM_PCRInfo_Store);
	}
	else {
	    rc = TPM_SizedBuffer_SetStructure(&(tpm_stored_data->sealInfo),
					      tpm_stored_data->tpm_seal_info,
					      (TPM_STORE_FUNCTION_T)TPM_PCRInfoLong_Store);
	}
    }
    /* copy sealInfoSize and sealInfo to sbuffer */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_stored_data->sealInfo));
    }
    return rc;
}

/* TPM_StoredData_Store()
   
   Before serializing, it serializes tpm_seal_info to sealInfoSize and sealInfo.

   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_StoredData_Store(TPM_STORE_BUFFER *sbuffer,
				TPM_STORED_DATA *tpm_stored_data,
				unsigned int version)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_StoredData_Store: v%u\n", version);
    if (rc == 0) {
	rc = TPM_StoredData_StoreClearData(sbuffer, tpm_stored_data, version);
    }
    /* store encDataSize and encData */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_stored_data->encData));
    }
    return rc;
}

/* TPM_StoredData_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_StoredData_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_StoredData_Delete(TPM_STORED_DATA *tpm_stored_data,
			   unsigned int version)
{
    printf(" TPM_StoredData_Delete: v%u\n", version);
    if (tpm_stored_data != NULL) {
	TPM_SizedBuffer_Delete(&(tpm_stored_data->sealInfo));
	TPM_SizedBuffer_Delete(&(tpm_stored_data->encData));
	if (version == 1) {
	    TPM_PCRInfo_Delete(tpm_stored_data->tpm_seal_info);
	    free(tpm_stored_data->tpm_seal_info);
	}
	else {
	    TPM_PCRInfoLong_Delete((TPM_PCR_INFO_LONG *)tpm_stored_data->tpm_seal_info);
	    free(tpm_stored_data->tpm_seal_info);
	}
	TPM_StoredData_Init(tpm_stored_data, version);
    }
    return;
}

/* TPM_StoredData_CheckTag() verifies the tag and et members of a TPM_STORED_DATA12 structure

 */

TPM_RESULT TPM_StoredData_CheckTag(TPM_STORED_DATA12 *tpm_stored_data12)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_StoredData_CheckTag:\n");
    if (rc == 0) {
	if (tpm_stored_data12->tag != TPM_TAG_STORED_DATA12) {
	    printf("TPM_StoredData_CheckTag: Error, tag expected %04x found %04hx\n",
		   TPM_TAG_STORED_DATA12, tpm_stored_data12->tag);
	    rc = TPM_BAD_VERSION;
	}
    }
    return rc;
}

/* TPM_StoredData_GenerateDigest() generates a TPM_DIGEST over the TPM_STORED_DATA structure
   excluding the encDataSize and encData members.
*/

TPM_RESULT TPM_StoredData_GenerateDigest(TPM_DIGEST tpm_digest,
					 TPM_STORED_DATA *tpm_stored_data,
					 unsigned int version)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	sbuffer;	/* TPM_STORED_DATA serialization */
    
    printf(" TPM_StoredData_GenerateDigest:\n");
    TPM_Sbuffer_Init(&sbuffer);			/* freed @1 */
    /* serialize the TPM_STORED_DATA excluding the encData fields */
    if (rc == 0) {
	rc = TPM_StoredData_StoreClearData(&sbuffer, tpm_stored_data, version);
    }
    if (rc == 0) {
	rc = TPM_SHA1Sbuffer(tpm_digest, &sbuffer);
    }
    TPM_Sbuffer_Delete(&sbuffer);	/* @1 */
    return rc;
}

/*
  Processing Functions
*/

/* TPM_SealCryptCommon() rev 98

   Handles the encrypt/decrypt actions common to TPM_Sealx and TPM_Unseal

   'encrypt TRUE for encryption, FALSE for decryption

   The output o1 must be freed by the caller.
*/

static TPM_RESULT TPM_SealCryptCommon(BYTE **o1,	/* freed by caller */
				      TPM_ADIP_ENC_SCHEME adipEncScheme,
				      TPM_SIZED_BUFFER *inData,
				      TPM_AUTH_SESSION_DATA *auth_session_data,
				      TPM_NONCE nonceOdd)
{
    TPM_RESULT		rc = 0;
    BYTE		*x1;			/* XOR string, MGF1 output */
    TPM_DIGEST		ctr;			/* symmetric key algorithm CTR */

    printf(" TPM_SealCryptCommon:\n");
    x1 = NULL;					/* freed @1 */

    /* allocate for the output o1 */
    if (rc == TPM_SUCCESS) {
	rc = TPM_Malloc(o1, inData->size);	/* freed by caller */
    }
    if (rc == TPM_SUCCESS) {
	TPM_PrintFourLimit("  TPM_SealCryptCommon: input data", inData->buffer, inData->size);
    }
    switch (adipEncScheme) {
      case TPM_ET_XOR:
	printf("  TPM_SealCryptCommon: TPM_ET_XOR\n");
	if (rc == TPM_SUCCESS) {
	    /* i. Use MGF1 to create string X1 of length sealedDataSize. The inputs to MGF1 are;
	       authLastnonceEven, nonceOdd, "XOR", and authHandle -> sharedSecret. The four
	       concatenated values form the Z value that is the seed for MFG1. */
	    rc = TPM_MGF1_GenerateArray(&x1,		/* MGF1 array */
					inData->size,	/* MGF1 array length */
					
					TPM_NONCE_SIZE +
					TPM_NONCE_SIZE +
					sizeof("XOR") -1 +
					TPM_DIGEST_SIZE, /* seed length */
					
					TPM_NONCE_SIZE, auth_session_data->nonceEven,
					TPM_NONCE_SIZE, nonceOdd,
					sizeof("XOR") -1, "XOR",
					TPM_DIGEST_SIZE, auth_session_data->sharedSecret,
					0, NULL);
	}
	/* ii. Create o1 by XOR of d1 -> data and X1 */
	if (rc == TPM_SUCCESS) {
	    TPM_PrintFour("  TPM_SealCryptCommon: XOR key", x1);
	    TPM_XOR(*o1, inData->buffer, x1, inData->size);
	}
	break;
      case TPM_ET_AES128_CTR:
	printf("  TPM_SealCryptCommon: TPM_ET_AES128_CTR\n");
	/* i. Create o1 by encrypting d1 -> data using the algorithm indicated by inData ->
	   et */
	/* ii. Key is from authHandle -> sharedSecret */
	/* iii. IV is SHA-1 of (authLastNonceEven || nonceOdd) */
	if (rc == TPM_SUCCESS) {
	    rc = TPM_SHA1(ctr,
			  TPM_NONCE_SIZE, auth_session_data->nonceEven,
			  TPM_NONCE_SIZE, nonceOdd,
			  0, NULL);
	}
	if (rc == TPM_SUCCESS) {
	    TPM_PrintFour("  TPM_SealCryptCommon: AES key", auth_session_data->sharedSecret);
	    TPM_PrintFour("  TPM_SealCryptCommon: CTR", ctr);
	    rc = TPM_SymmetricKeyData_CtrCrypt(*o1,				/* output data */
					       inData->buffer,			/* input data */
					       inData->size,			/* data size */
					       auth_session_data->sharedSecret, /* key */
					       TPM_SECRET_SIZE,			/* key size */
					       ctr,				/* CTR */
					       TPM_DIGEST_SIZE);		/* CTR size */
	}
	break;
      default:
	printf("TPM_SealCryptCommon: Error, unsupported adipEncScheme %02x\n", adipEncScheme);
	rc = TPM_INAPPROPRIATE_ENC;
	break;
    }
    if (rc == 0) {
	TPM_PrintFour("  TPM_SealCryptCommon: output data", *o1);
	
    }
    free(x1);				/* @1 */
    return rc;
}

/* 10.1 TPM_Seal rev 110

   The SEAL operation allows software to explicitly state the future "trusted" configuration that
   the platform must be in for the secret to be revealed. The SEAL operation also implicitly
   includes the relevant platform configuration (PCR-values) when the SEAL operation was
   performed. The SEAL operation uses the tpmProof value to BIND the blob to an individual TPM.

   TPM_Seal is used to encrypt private objects that can only be decrypted using TPM_Unseal.
*/

TPM_RESULT TPM_Process_Seal(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE	keyHandle;	/* Handle of a loaded key that can perform seal
					   operations. */
    TPM_ENCAUTH		encAuth;	/* The encrypted authorization data for the sealed data. */
    TPM_SIZED_BUFFER	pcrInfo;	/* The PCR selection information. The caller MAY use
					   TPM_PCR_INFO_LONG. */
    TPM_SIZED_BUFFER	inData;		/* The data to be sealed to the platform and any specified
					   PCRs */
    TPM_AUTHHANDLE	authHandle;	/* The authorization handle used for keyHandle
					   authorization. Must be an OS_AP session for this
					   command. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL		continueAuthSession = TRUE;	/* Ignored */
    TPM_AUTHDATA	pubAuth;	/* The authorization digest for inputs and keyHandle. HMAC
					   key: key.usageAuth. */

    /* processing */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_KEY			*key = NULL;		/* the key specified by keyHandle */
    TPM_SECRET			*keyUsageAuth;
    TPM_BOOL			parentPCRStatus;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    unsigned int		v1PcrVersion = 1;		/* pcrInfo version */
    TPM_STORED_DATA12		*s1_12;
    TPM_PCR_INFO		tpm_pcr_info;		/* deserialized pcrInfo v1 */
    TPM_PCR_INFO_LONG		tpm_pcr_info_long;	/* deserialized pcrInfo v2 */
    unsigned char		*stream;
    uint32_t			stream_size;
    TPM_DIGEST			a1Auth;
    TPM_SEALED_DATA		s2SealedData;
    
    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_STORED_DATA	s1StoredData;	/* Encrypted, integrity-protected data object that is the
					   result of the TPM_Seal operation. Returned as
					   SealedData */
    
    printf("TPM_Process_Seal: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&pcrInfo);			/* freed @1 */
    TPM_SizedBuffer_Init(&inData);			/* freed @2 */
    TPM_StoredData_Init(&s1StoredData, v1PcrVersion);	/* freed @3, default is v1 */
    TPM_PCRInfo_Init(&tpm_pcr_info);			/* freed @4 */
    TPM_PCRInfoLong_Init(&tpm_pcr_info_long);		/* freed @5 */
    TPM_SealedData_Init(&s2SealedData);			/* freed @6 */
    s1_12 = (TPM_STORED_DATA12 *)&s1StoredData;		/* to avoid casts */
    /*
      get inputs
    */
    /* get keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&keyHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get encAuth parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_Seal: keyHandle %08x\n", keyHandle);
	returnCode = TPM_Authdata_Load(encAuth, &command, &paramSize);
    }
    /* get pcrInfo parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&pcrInfo, &command, &paramSize);
    }	
    /* get inData parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&inData, &command, &paramSize);
    }	
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_Seal: Sealing %u bytes\n", inData.size);
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
	    printf("TPM_Process_Seal: Error, command has %u extra bytes\n",
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
    /* get the key corresponding to the keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&key, &parentPCRStatus, tpm_state, keyHandle,
						 FALSE,		/* not r/o, using to encrypt */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    /* get keyHandle -> usageAuth */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_GetUsageAuth(&keyUsageAuth, key);
    }	 
    /* get the session data */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_OSAP,
					      TPM_ET_KEYHANDLE,
					      ordinal,
					      key,
					      NULL,			/* OIAP */
					      key->tpm_store_asymkey->pubDataDigest);	/* OSAP */
    }
    /* 1. Validate the authorization to use the key pointed to by keyHandle */
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
    /* 2. If the inDataSize is 0 the TPM returns TPM_BAD_PARAMETER */
    if (returnCode == TPM_SUCCESS) {
	if (inData.size == 0) {
	    printf("TPM_Process_Seal: Error, inDataSize is 0\n");
	    returnCode = TPM_BAD_PARAMETER;
	}
    }
    /* 3. If the keyUsage field of the key indicated by keyHandle does not have the value
       TPM_KEY_STORAGE, the TPM must return the error code TPM_INVALID_KEYUSAGE. */
    if (returnCode == TPM_SUCCESS) {
	if (key->keyUsage != TPM_KEY_STORAGE) {
	    printf("TPM_Process_Seal: Error, key keyUsage %04hx must be TPM_KEY_STORAGE\n",
		   key->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 4. If the keyHandle points to a migratable key then the TPM MUST return the error code
       TPM_INVALID_KEY_USAGE. */
    if (returnCode == TPM_SUCCESS) {
	if (key->keyFlags & TPM_MIGRATABLE) {
	    printf("TPM_Process_Seal: Error, key keyFlags %08x indicates migratable\n",
		   key->keyFlags);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 5. Determine the version of pcrInfo */
    if (returnCode == TPM_SUCCESS) {
	/* a. If pcrInfoSize is 0 */
	if (pcrInfo.size == 0) {
	    v1PcrVersion = 1;			/* i. set V1 to 1  */
	}
	else {				/* b. Else */
	    /* i. Point X1 as TPM_PCR_INFO_LONG structure to pcrInfo  */
	    /* ii. If X1 -> tag is TPM_TAG_PCR_INFO_LONG  */
	    if (htons(*(uint16_t *)(pcrInfo.buffer)) == TPM_TAG_PCR_INFO_LONG) {
		v1PcrVersion = 2;			/* (1) Set V1 to 2  */
	    }
	    else {			/* iii. Else */
		v1PcrVersion = 1;			/* (1) Set V1 to 1 */
	    }
	}
	/* 6. If V1 is 1 then */
	/* a. Create S1 a TPM_STORED_DATA structure  */
	/* 7. else  */
	/* a. Create S1 a TPM_STORED_DATA12 structure  */
	/* b. Set S1 -> et to 0 */
	/* 8. Set S1 -> encDataSize to 0 */
	/* 9. Set S1 -> encData to all zeros */
	printf("TPM_Process_Seal: V%u\n", v1PcrVersion);
	TPM_StoredData_Init(&s1StoredData, v1PcrVersion);
	/* 10. Set S1 -> sealInfoSize to pcrInfoSize */
	/* NOTE This step is unnecessary.  If pcrInfoSize is 0, sealInfoSize is already initialized
	   to 0.  If pcrInfoSize is non-zero, sealInfoSize is the result of serialization of the
	   tpm_seal_info member, which is either a TPM_PCR_INFO or a TPM_PCR_INFO_LONG */
    }
    /* 11. If pcrInfoSize is not 0 then */
    if ((returnCode == TPM_SUCCESS) && (pcrInfo.size != 0)) {
	printf("TPM_Process_Seal: Creating PCR digest\n");
	/* assign the stream, so pcrInfo is not altered */
	stream = pcrInfo.buffer;
	stream_size = pcrInfo.size;
	/* a. if V1 is 1 then */
	if (v1PcrVersion == 1) {
	    /* i. Validate pcrInfo as a valid TPM_PCR_INFO structure, return TPM_BADINDEX on
	       error */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_PCRInfo_Load(&tpm_pcr_info, &stream, &stream_size);
		if (returnCode != 0) {
		    returnCode = TPM_BADINDEX;
		}
	    }
	    /* build the TPM_STORED_DATA S1 structure */
	    if (returnCode == TPM_SUCCESS) {
		/* ii. Set S1 -> sealInfo -> pcrSelection to pcrInfo -> pcrSelection */
		returnCode = TPM_PCRInfo_CreateFromBuffer(&(s1StoredData.tpm_seal_info), &pcrInfo);
	    }
	    /* iii. Create h1 the composite hash of the PCR selected by pcrInfo -> pcrSelection */
	    /* iv. Set S1 -> sealInfo -> digestAtCreation to h1 */
	    /* NOTE hash directly to destination.  */
	    if (returnCode == TPM_SUCCESS) {
		returnCode =
		    TPM_PCRSelection_GenerateDigest(s1StoredData.tpm_seal_info->digestAtCreation,
						    &(tpm_pcr_info.pcrSelection),
						    tpm_state->tpm_stclear_data.PCRS);
	    }
	    /* v. Set S1 -> sealInfo -> digestAtRelease to pcrInfo -> digestAtRelease */
	    /* NOTE digestAtRelease copied during TPM_PCRInfo_CreateFromBuffer() */
	}
	/* b. else (v1 is 2) */
	else {
	    /* i. Validate pcrInfo as a valid TPM_PCR_INFO_LONG structure, return TPM_BADINDEX
	       on error */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_PCRInfoLong_Load(&tpm_pcr_info_long, &stream, &stream_size);
		if (returnCode != 0) {
		    returnCode = TPM_BADINDEX;
		}
	    }
	    /* build the TPM_STORED_DATA S1 structure */
	    if (returnCode == TPM_SUCCESS) {
		/* ii. Set S1 -> sealInfo -> creationPCRSelection to pcrInfo -> creationPCRSelection
		   */
		/* iii. Set S1 -> sealInfo -> releasePCRSelection to pcrInfo -> releasePCRSelection
		   */
		/* iv. Set S1 -> sealInfo -> digestAtRelease to pcrInfo -> digestAtRelease */
		/* v. Set S1 -> sealInfo -> localityAtRelease to pcrInfo -> localityAtRelease */
		/* NOTE copied during TPM_PCRInfoLong_CreateFromBuffer() */
		returnCode = TPM_PCRInfoLong_CreateFromBuffer(&(s1_12->tpm_seal_info_long),
							      &pcrInfo);
	    }
	    if (returnCode == TPM_SUCCESS) {
		/* vi. Create h2 the composite hash of the PCR selected by pcrInfo ->
		   creationPCRSelection */
		/* vii. Set S1 -> sealInfo -> digestAtCreation to h2 */
		/* NOTE hash directly to destination. */
		returnCode =
		    TPM_PCRSelection_GenerateDigest(s1_12->tpm_seal_info_long->digestAtCreation,
						    &(tpm_pcr_info_long.creationPCRSelection),
						    tpm_state->tpm_stclear_data.PCRS);
	    }
	    /* viii. Set S1 -> sealInfo -> localityAtCreation to TPM_STANY_FLAGS ->
	       localityModifier */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_Locality_Set(&(s1_12->tpm_seal_info_long->localityAtCreation),
					      tpm_state->tpm_stany_flags.localityModifier);
	    }
	}
    }
    /* 12. Create a1 by decrypting encAuth according to the ADIP indicated by authHandle. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessionData_Decrypt(a1Auth,
						 NULL,
						 encAuth,
						 auth_session_data,
						 NULL,
						 NULL,
						 FALSE);	/* even and odd */
    }
    /* 13. The TPM provides NO validation of a1. Well-known values (like all zeros) are valid and
       possible. */
    /* 14. Create S2 a TPM_SEALED_DATA structure */
    if (returnCode == TPM_SUCCESS) {
	/* a. Set S2 -> payload to TPM_PT_SEAL */
	/* NOTE: Done at TPM_SealedData_Init() */
	/* b. Set S2 -> tpmProof to TPM_PERMANENT_DATA -> tpmProof */
	TPM_Secret_Copy(s2SealedData.tpmProof, tpm_state->tpm_permanent_data.tpmProof);
	/* c. Create h3 the SHA-1 of S1 */
	/* d. Set S2 -> storedDigest to h3 */
	returnCode = TPM_StoredData_GenerateDigest(s2SealedData.storedDigest,
						   &s1StoredData, v1PcrVersion);
    }
    if (returnCode == TPM_SUCCESS) {
	/* e. Set S2 -> authData to a1 */
	TPM_Secret_Copy(s2SealedData.authData, a1Auth);
	/* f. Set S2 -> dataSize to inDataSize */
	/* g. Set S2 -> data to inData */
	returnCode = TPM_SizedBuffer_Copy(&(s2SealedData.data), &inData);
    }
    /* 15. Validate that the size of S2 can be encrypted by the key pointed to by keyHandle, return
       TPM_BAD_DATASIZE on error */
    /* 16. Create s3 the encryption of S2 using the key pointed to by keyHandle */
    /* 17. Set continueAuthSession to FALSE */
    if (returnCode == TPM_SUCCESS) {
	continueAuthSession = FALSE;
    }
    /* 18. Set S1 -> encDataSize to the size of s3 */
    /* 19. Set S1 -> encData to s3 */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SealedData_GenerateEncData(&(s1StoredData.encData), &s2SealedData, key);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_Seal: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* 20. Return S1 as sealedData */
	    returnCode = TPM_StoredData_Store(response, &s1StoredData, v1PcrVersion);
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
					       outParamEnd - outParamStart);		/* length */
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
    TPM_SizedBuffer_Delete(&pcrInfo);			/* @1 */
    TPM_SizedBuffer_Delete(&inData);			/* @2 */
    TPM_StoredData_Delete(&s1StoredData, v1PcrVersion);	/* @3 */
    TPM_PCRInfo_Delete(&tpm_pcr_info);			/* @4 */
    TPM_PCRInfoLong_Delete(&tpm_pcr_info_long); 	/* @5 */
    TPM_SealedData_Delete(&s2SealedData);		/* @6 */
    return rcf;
}

/* 10.7 TPM_Sealx rev 110
     
   The TPM_Sealx command works exactly like the TPM_Seal command with the additional requirement of
   encryption for the inData parameter. This command also places in the sealed blob the information
   that the TPM_Unseal also requires encryption.

   TPM_Sealx requires the use of 1.2 data structures. The actions are the same as TPM_Seal without
   the checks for 1.1 data structure usage.
*/

TPM_RESULT TPM_Process_Sealx(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE	keyHandle;	/* Handle of a loaded key that can perform seal
					   operations. */
    TPM_ENCAUTH		encAuth;	/* The encrypted authorization data for the sealed data */
    TPM_SIZED_BUFFER	pcrInfo;	/* If 0 there are no PCR registers in use.  pcrInfo MUST use
					   TPM_PCR_INFO_LONG */
    TPM_SIZED_BUFFER	inData;		/* The data to be sealed to the platform and any specified
					   PCRs */

    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle used for keyHandle
					   authorization.  Must be an OSAP session for this command.
					   */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL		continueAuthSession = TRUE;	/* Ignored */
    TPM_AUTHDATA	pubAuth;	/* The authorization digest for inputs and keyHandle. HMAC
					   key: key.usageAuth. */

    /* processing */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;			/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_KEY			*key = NULL;			/* the key specified by keyHandle */
    TPM_SECRET			*keyUsageAuth;
    TPM_BOOL			parentPCRStatus;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */

    /* output parameters */
    uint32_t			outParamStart;	/* starting point of outParam's */
    uint32_t			outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST			outParamDigest;
    TPM_STORED_DATA12		s1StoredData;	/* Encrypted, integrity-protected data object that
						   is the result of the TPM_Seal operation. Returned
						   as SealedData */
    TPM_STORED_DATA		*s1_11;		/* 1.1 version to avoid casts */
    TPM_SEALED_DATA		s2SealedData;
    TPM_DIGEST			a1Auth;
    BYTE			*o1DecryptedData;
    
    printf("TPM_Process_Sealx: Ordinal Entry\n");
    s1_11 = (TPM_STORED_DATA *)&s1StoredData;	/* 1.1 version to avoid casts */
    TPM_SizedBuffer_Init(&pcrInfo);		/* freed @1 */
    TPM_SizedBuffer_Init(&inData);		/* freed @2 */
    TPM_StoredData_Init(s1_11, 2);		/* freed @3 */
    TPM_SealedData_Init(&s2SealedData); 	/* freed @4 */
    o1DecryptedData = NULL;			/* freed @5 */
    /*
      get inputs
    */
    /*	get keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&keyHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get encAuth parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Authdata_Load(encAuth, &command, &paramSize);
    }
    /* get pcrInfo parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&pcrInfo, &command, &paramSize);
    }	
    /* get inData parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&inData, &command, &paramSize);
    }	
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_Sealx: Sealing %u bytes\n", inData.size);
	TPM_PrintFourLimit("TPM_Process_Sealx: Sealing data", inData.buffer, inData.size);
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
	    printf("TPM_Process_Sealx: Error, command has %u extra bytes\n",
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
    /* get the key corresponding to the keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&key, &parentPCRStatus, tpm_state, keyHandle,
						 FALSE,		/* not r/o, using to encrypt */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    /* get keyHandle -> usageAuth */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_GetUsageAuth(&keyUsageAuth, key);
    }	 
    /* get the session data */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_OSAP,
					      TPM_ET_KEYHANDLE,
					      ordinal,
					      key,
					      NULL,			/* OIAP */
					      key->tpm_store_asymkey->pubDataDigest);	/* OSAP */
    }
    /* 1. Validate the authorization to use the key pointed to by keyHandle */
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
    /* 2. If the inDataSize is 0 the TPM returns TPM_BAD_PARAMETER */
    if (returnCode == TPM_SUCCESS) {
	if (inData.size == 0) {
	    printf("TPM_Process_Sealx: Error, inDataSize is 0\n");
	    returnCode = TPM_BAD_PARAMETER;
	}
    }
    /* 3. If the keyUsage field of the key indicated by keyHandle does not have the value
       TPM_KEY_STORAGE, the TPM must return the error code TPM_INVALID_KEYUSAGE. */
    if (returnCode == TPM_SUCCESS) {
	if (key->keyUsage != TPM_KEY_STORAGE) {
	    printf("TPM_Process_Sealx: Error, key keyUsage %04hx must be TPM_KEY_STORAGE\n",
		   key->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 4. If the keyHandle points to a migratable key then the TPM MUST return the error code
       TPM_INVALID_KEY_USAGE. */
    if (returnCode == TPM_SUCCESS) {
	if (key->keyFlags & TPM_MIGRATABLE) {
	    printf("TPM_Process_Sealx: Error, key keyFlags %08x indicates migratable\n",
		   key->keyFlags);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 5. Create S1 a TPM_STORED_DATA12 structure */
    /* 6. Set S1 -> encDataSize to 0 */
    /* 7. Set S1 -> encData to all zeros */
    /* NOTE: Done by TPM_StoredData_Init() */
    /* 8. Set S1 -> sealInfoSize to pcrInfoSize */
    /* NOTE This step is unnecessary.  If pcrInfoSize is 0, sealInfoSize is already initialized
       to 0.  If pcrInfoSize is non-zero, sealInfoSize is the result of serialization of the
       tpm_seal_info member, which is a TPM_PCR_INFO_LONG */
    /* 9. If pcrInfoSize is not 0 then */
    if ((returnCode == TPM_SUCCESS) && (pcrInfo.size != 0)) {
	printf("TPM_Process_Sealx: Setting sealInfo to pcrInfo\n");
	/* initializing the s -> TPM_PCR_INFO_LONG cache to the contents of pcrInfo */
	/* a. Validate pcrInfo as a valid TPM_PCR_INFO_LONG structure, return TPM_BADINDEX on
	   error */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_PCRInfoLong_CreateFromBuffer(&(s1StoredData.tpm_seal_info_long),
							  &pcrInfo);
	    if (returnCode != TPM_SUCCESS) {
		returnCode = TPM_BADINDEX;
	    }
	}
	/* b. Set S1 -> sealInfo -> creationPCRSelection to pcrInfo -> creationPCRSelection */
	/* c. Set S1 -> sealInfo -> releasePCRSelection to pcrInfo -> releasePCRSelection */
	/* d. Set S1 -> sealInfo -> digestAtRelease to pcrInfo -> digestAtRelease */
	/* e. Set S1 -> sealInfo -> localityAtRelease to pcrInfo -> localityAtRelease  */
	/* NOTE copied during TPM_PCRInfoLong_CreateFromBuffer() */
	/* f. Create h2 the composite hash of the PCR selected by pcrInfo -> creationPCRSelection */
	/* g. Set S1 -> sealInfo -> digestAtCreation to h2 */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_PCRSelection_GenerateDigest
			 (s1StoredData.tpm_seal_info_long->digestAtCreation,
			  &(s1StoredData.tpm_seal_info_long->creationPCRSelection),
			  tpm_state->tpm_stclear_data.PCRS);
	}
	/* h. Set S1 -> sealInfo -> localityAtCreation to TPM_STANY_DATA -> localityModifier */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Locality_Set(&(s1StoredData.tpm_seal_info_long->localityAtCreation),
					  tpm_state->tpm_stany_flags.localityModifier);
	}
    }
    /* 10. Create S2 a TPM_SEALED_DATA structure */
    /* NOTE: Done at TPM_SealedData_Init() */
    /* 11.Create a1 by decrypting encAuth according to the ADIP indicated by authHandle. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_Sealx: Decrypting encAuth\n");
	returnCode = TPM_AuthSessionData_Decrypt(a1Auth,	/* a1 even */
						 NULL,		/* a1 odd (2nd encAuth) */
						 encAuth,	/* encAuthEven */
						 auth_session_data,
						 NULL,		/* nonceOdd */
						 NULL,		/* encAuthOdd */
						 FALSE);	/* even and odd */
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_Sealx: Decrypted Auth", a1Auth);
	/* a. If authHandle indicates XOR encryption for the AuthData secrets */
	if (auth_session_data->adipEncScheme == TPM_ET_XOR) {
	    /* i. Set S1 -> et to TPM_ET_XOR || TPM_ET_KEY */
	    /* (1) TPM_ET_KEY is added because TPM_Unseal uses zero as a special value indicating no
	       encryption. */
	    s1StoredData.et = TPM_ET_XOR | TPM_ET_KEY;
	}
	/* b. Else */
	else {
	    /* i. Set S1 -> et to algorithm indicated by authHandle */
	    s1StoredData.et = auth_session_data->adipEncScheme << 8;
	}
    }
    /* 12. The TPM provides NO validation of a1. Well-known values (like all zeros) are valid and
       possible. */
    /* 13. If authHandle indicates XOR encryption */
    /* a. Use MGF1 to create string X2 of length inDataSize. The inputs to MGF1 are;
       authLastNonceEven, nonceOdd, "XOR", and authHandle -> sharedSecret. The four concatenated
       values form the Z value that is the seed for MFG1. */
    /* b. Create o1 by XOR of inData and x2 */
    /* 14. Else */
    /* a. Create o1 by decrypting inData using the algorithm indicated by authHandle */
    /* b. Key is from authHandle -> sharedSecret */
    /* c. CTR is SHA-1 of (authLastNonceEven || nonceOdd) */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_Sealx: decrypting inData\n");
	returnCode = TPM_SealCryptCommon(&o1DecryptedData,		/* freed by caller */
					 auth_session_data->adipEncScheme,
					 &inData,
					 auth_session_data,
					 nonceOdd);

   }
    /* 15. Create S2 a TPM_SEALED_DATA structure */
    if (returnCode == TPM_SUCCESS) {
	/* a. Set S2 -> payload to TPM_PT_SEAL */
	/* NOTE: Done at TPM_SealedData_Init() */
	/* b. Set S2 -> tpmProof to TPM_PERMANENT_DATA -> tpmProof */
	TPM_Secret_Copy(s2SealedData.tpmProof, tpm_state->tpm_permanent_data.tpmProof);
	/* c. Create h3 the SHA-1 of S1 */
	/* d. Set S2 -> storedDigest to h3 */
	returnCode = TPM_StoredData_GenerateDigest(s2SealedData.storedDigest, s1_11, 2);
    }
    /* e. Set S2 -> authData to a1 */
    if (returnCode == TPM_SUCCESS) {
	TPM_Secret_Copy(s2SealedData.authData, a1Auth);
	/* f. Set S2 -> dataSize to inDataSize */
	/* g. Set S2 -> data to o1 */
	returnCode = TPM_SizedBuffer_Set(&(s2SealedData.data), inData.size, o1DecryptedData);
    }
    /* 16. Validate that the size of S2 can be encrypted by the key pointed to by keyHandle, return
       */
    /* TPM_BAD_DATASIZE on error */
    /* 17. Create s3 the encryption of S2 using the key pointed to by keyHandle */
    /* 18. Set continueAuthSession to FALSE */
    if (returnCode == TPM_SUCCESS) {
	continueAuthSession = FALSE;
    }
    /* 19. Set S1 -> encDataSize to the size of s3 */
    /* 20. Set S1 -> encData to s3 */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_Sealx: Encrypting sealed data\n");
	returnCode = TPM_SealedData_GenerateEncData(&(s1StoredData.encData), &s2SealedData, key);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_Sealx: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* 21. Return S1 as sealedData */
	    returnCode = TPM_StoredData_Store(response, s1_11, 2);
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
    TPM_SizedBuffer_Delete(&pcrInfo);		/* @1 */
    TPM_SizedBuffer_Delete(&inData);		/* @2 */
    TPM_StoredData_Delete(s1_11, 2);		/* @3 */
    TPM_SealedData_Delete(&s2SealedData);	/* @4 */
    free(o1DecryptedData);			/* @5 */
    return rcf;
}

/* 10.2 TPM_Unseal rev 110

   The TPM_Unseal operation will reveal TPM_Sealed data only if it was encrypted on this platform
   and the current configuration (as defined by the named PCR contents) is the one named as
   qualified to decrypt it.  Internally, TPM_Unseal accepts a data blob generated by a TPM_Seal
   operation. TPM_Unseal decrypts the structure internally, checks the integrity of the resulting
   data, and checks that the PCR named has the value named during TPM_Seal.  Additionally, the
   caller must supply appropriate authorization data for blob and for the key that was used to seal
   that data.
   
   If the integrity, platform configuration and authorization checks succeed, the sealed data is
   returned to the caller; otherwise, an error is generated.
*/

TPM_RESULT TPM_Process_Unseal(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE	parentHandle;	/* Handle of a loaded key that can unseal the data. */
    TPM_STORED_DATA	inData;		/* The encrypted data generated by TPM_Seal. */
    TPM_AUTHHANDLE	authHandle;	/* The authorization handle used for parentHandle. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   handle */
    TPM_AUTHDATA	parentAuth;	/* The authorization digest for inputs and
					   parentHandle. HMAC key: parentKey.usageAuth. */
    TPM_AUTHHANDLE	dataAuthHandle; /* The authorization handle used to authorize inData. */
    TPM_NONCE		datanonceOdd;	/* Nonce generated by system associated with
					   entityAuthHandle */
    TPM_BOOL	continueDataSession = TRUE;	/* Continue usage flag for dataAuthHandle. */
    TPM_AUTHDATA	dataAuth;	/* The authorization digest for the encrypted entity. HMAC
					   key: entity.usageAuth.  */

    /* processing */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_BOOL			dataAuthHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_AUTH_SESSION_DATA	*data_auth_session_data = NULL; /* session data for dataAuthHandle
								   */
    TPM_SECRET			*hmacKey;
    TPM_SECRET			*dataHmacKey;
    unsigned int		v1StoredDataVersion = 1;	/* version of TPM_STORED_DATA
								   inData */
    TPM_KEY			*parentKey;
    TPM_BOOL			parentPCRStatus;
    TPM_SECRET			*parentUsageAuth;
    TPM_SEALED_DATA		d1SealedData;
    TPM_DIGEST			h1StoredDataDigest;
    TPM_STORED_DATA12		*s2StoredData;
    BYTE			*o1Encrypted;			/* For ADIP encryption */
    TPM_ADIP_ENC_SCHEME		adipEncScheme;	 

    /* output parameters */
    uint32_t			outParamStart;	/* starting point of outParam's */
    uint32_t			outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST			outParamDigest;
    uint32_t			secretSize = 0; /* Decrypted data that had been sealed */
    BYTE			*secret = NULL;

    printf("TPM_Process_Unseal: Ordinal Entry\n");
    TPM_StoredData_Init(&inData, v1StoredDataVersion);	/* freed @1, default is v1 */
    TPM_SealedData_Init(&d1SealedData); 		/* freed @2 */
    o1Encrypted = NULL;					/* freed @3 */
    s2StoredData = (TPM_STORED_DATA12 *)&inData;	/* inData when it's a TPM_STORED_DATA12
							   structure */
    /*
      get inputs
    */
    /* get parentHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&parentHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get inData parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_Unseal: parentHandle %08x\n", parentHandle);
	returnCode = TPM_StoredData_Load(&inData, &v1StoredDataVersion, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_Unseal: inData is v%u\n", v1StoredDataVersion);
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
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					parentAuth,
					&command, &paramSize);
    }
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	printf("TPM_Process_Unseal: authHandle %08x\n", authHandle);
    }
    /* get the 'below the line' authorization parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthParams_Get(&dataAuthHandle,
					&dataAuthHandleValid,
					datanonceOdd,
					&continueDataSession,
					dataAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_Unseal: dataAuthHandle %08x\n", dataAuthHandle); 
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_Unseal: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	authHandleValid = FALSE;
	dataAuthHandleValid = FALSE;
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
    /* get parentHandle -> usageAuth */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_Key_GetUsageAuth(&parentUsageAuth, parentKey);
    }	 
    /* get the first session data */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
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
    /* 1. The TPM MUST validate that parentAuth authorizes the use of the key in parentHandle, on
       error return TPM_AUTHFAIL */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					parentAuth);		/* Authorization digest for input */
    }
    /* if there are no parent auth parameters */
    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_AUTH2_COMMAND)) {
	if (parentKey->authDataUsage != TPM_AUTH_NEVER) {
	    printf("TPM_Process_Unseal: Error, parent key authorization required\n");
	    returnCode = TPM_AUTHFAIL;
	}
    }
    /* 2. If the keyUsage field of the key indicated by parentHandle does not have the value
       TPM_KEY_STORAGE, the TPM MUST return the error code TPM_INVALID_KEYUSAGE. */
    if (returnCode == TPM_SUCCESS) {
	if (parentKey->keyUsage != TPM_KEY_STORAGE) {
	    printf("TPM_Process_Unseal: Error, key keyUsage %04hx must be TPM_KEY_STORAGE\n",
		   parentKey->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 3. The TPM MUST check that the TPM_KEY_FLAGS -> Migratable flag has the value FALSE in the
       key indicated by parentKeyHandle. If not, the TPM MUST return the error code
       TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	if (parentKey->keyFlags & TPM_MIGRATABLE) {
	    printf("TPM_Process_Unseal: Error, key keyFlags %08x indicates migratable\n",
		   parentKey->keyFlags);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 4. Determine the version of inData */
    /* a. If inData -> tag = TPM_TAG_STORED_DATA12 */
    /* i. Set V1 to 2 */
    /* ii. Map S2 a TPM_STORED_DATA12 structure to inData */
    /* b. Else If inData -> ver = 1.1 */
    /* i. Set V1 to 1 */
    /* ii. Map S2 a TPM_STORED_DATA structure to inData */
    /* c. Else */
    /* i. Return TPM_BAD_VERSION */
    /* NOTE: Done during TPM_StoredData_Load() */
    /* The extra indent of error checking is required because the next steps all return
       TPM_NOTSEALED_BLOB on error */
    if (returnCode == TPM_SUCCESS) {
	/* 5. Create d1 by decrypting S2 -> encData using the key pointed to by parentHandle */
	printf("TPM_Process_Unseal: Decrypting encData\n");
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_SealedData_DecryptEncData(&d1SealedData,	/* TPM_SEALED_DATA */
						       &(inData.encData),
						       parentKey);	
	}
	/* 6. Validate d1 */
	/* a. d1 MUST be a TPM_SEALED_DATA structure */
	/* NOTE Done during TPM_SealedData_DecryptEncData() */
	/* b. d1 -> tpmProof MUST match TPM_PERMANENT_DATA -> tpmProof */
	if (returnCode == TPM_SUCCESS) {
	    printf("TPM_Process_Unseal: Sealed data size %u\n", d1SealedData.data.size);
	    TPM_PrintFour("TPM_Process_Unseal: Sealed data", d1SealedData.data.buffer);
	    printf("TPM_Process_Unseal: Checking tpmProof\n");
	    returnCode = TPM_Secret_Compare(d1SealedData.tpmProof,
					    tpm_state->tpm_permanent_data.tpmProof);
	}
	if (returnCode == TPM_SUCCESS) {
	    /* c. Set S2 -> encDataSize to 0 */
	    /* d. Set S2 -> encData to all zeros */
	    /* NOTE: This would be done at cleanup */
	    TPM_SizedBuffer_Delete(&(inData.encData));
	    /* e. Create h1 the SHA-1 of S2 */
	    returnCode = TPM_StoredData_GenerateDigest(h1StoredDataDigest,
						       &inData, v1StoredDataVersion);
	}
	/* f. d1 -> storedDigest MUST match h1 */
	if (returnCode == TPM_SUCCESS) {
	    printf("TPM_Process_Unseal: Checking storedDigest\n");
	    returnCode = TPM_Digest_Compare(d1SealedData.storedDigest, h1StoredDataDigest);
	}
	/* g. d1 -> payload MUST be TPM_PT_SEAL */
	if (returnCode == TPM_SUCCESS) {
	    if (d1SealedData.payload != TPM_PT_SEAL) {
		printf("TPM_Process_Unseal: Error, payload %02x not TPM_PT_SEAL\n",
		       d1SealedData.payload);
		returnCode = TPM_NOTSEALED_BLOB;
	    }
	}
	/* h. Any failure MUST return TPM_NOTSEALED_BLOB */
	if (returnCode != TPM_SUCCESS) {
	    returnCode = TPM_NOTSEALED_BLOB;
	}
    }
    /* 7. If S2 -> sealInfo is not 0 then */
    /* NOTE: Done by _CheckDigest() */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_Unseal: Checking PCR digest\n");
	/* a. If V1 is 1 then */
	if (v1StoredDataVersion == 1) {
	    /* i. Validate that S2 -> pcrInfo is a valid TPM_PCR_INFO structure */
	    /* NOTE: Done during TPM_StoredData_Load() */
	    /* ii. Create h2 the composite hash of the PCR selected by S2 -> pcrInfo -> pcrSelection
	     */
	    /* c. Compare h2 with S2 -> pcrInfo -> digestAtRelease, on mismatch return
	       TPM_WRONGPCRVALUE */
	    returnCode = TPM_PCRInfo_CheckDigest(inData.tpm_seal_info,
						 tpm_state->tpm_stclear_data.PCRS); /* PCR array */
	}
	/* b. If V1 is 2 then */
	else {
	    /* i. Validate that S2 -> pcrInfo is a valid TPM_PCR_INFO_LONG structure */
	    /* NOTE: Done during TPM_StoredData_Load() */
	    /* ii. Create h2 the composite hash of the PCR selected by S2 -> pcrInfo ->
	       releasePCRSelection */
	    /* iii. Check that S2 -> pcrInfo -> localityAtRelease for TPM_STANY_DATA ->
	       localityModifier is TRUE */
	    /* (1) For example if TPM_STANY_DATA -> localityModifier was 2 then S2 -> pcrInfo ->
	       localityAtRelease -> TPM_LOC_TWO would have to be TRUE */
	    /* c. Compare h2 with S2 -> pcrInfo -> digestAtRelease, on mismatch return
	       TPM_WRONGPCRVALUE */
	    returnCode =
		TPM_PCRInfoLong_CheckDigest(s2StoredData->tpm_seal_info_long,
					    tpm_state->tpm_stclear_data.PCRS,	/* PCR array */
					    tpm_state->tpm_stany_flags.localityModifier);
	}
    }
    /* 8. The TPM MUST validate authorization to use d1 by checking that the HMAC calculation
       using d1 -> authData as the shared secret matches the dataAuth. Return TPM_AUTHFAIL on
       mismatch. */
    /* get the second session data */
    /* NOTE: While OSAP isn't specifically excluded, there is currently no way to set up an OSAP
       session using TPM_SEALED_DATA as the entity */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_GetData(&data_auth_session_data,
					      &dataHmacKey,
					      tpm_state,
					      dataAuthHandle,
					      TPM_PID_OIAP,	/* currently require OIAP */
					      0,		/* OSAP entity type */
					      ordinal,
					      NULL,
					      &(d1SealedData.authData), /* OIAP */
					      NULL);			/* OSAP */
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Auth2data_Check(tpm_state,
					 *dataHmacKey,		/* HMAC key */
					 inParamDigest,
					 data_auth_session_data, /* authorization session */
					 datanonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					 continueDataSession,
					 dataAuth);		/* Authorization digest for input */
    }
    if (returnCode == TPM_SUCCESS) {
	/* 9. If V1 is 2 and S2 -> et specifies encryption (i.e. is not all zeros) then */
	if ((v1StoredDataVersion == 2) && (s2StoredData->et != 0x0000)) {
	    /* a. If tag is not TPM_TAG_RQU_AUTH2_COMMAND, return TPM_AUTHFAIL */
	    if (returnCode == TPM_SUCCESS) {
		if (tag != TPM_TAG_RQU_AUTH2_COMMAND) {
		    printf("TPM_Process_Unseal: Error, sealed with encryption but auth-1\n");
		    returnCode = TPM_AUTHFAIL;
		}
	    }
	    /* b. Verify that the authHandle session type is TPM_PID_OSAP or TPM_PID_DSAP, return
	       TPM_BAD_MODE on error. */
	    if (returnCode == TPM_SUCCESS) {
		if ((auth_session_data->protocolID != TPM_PID_OSAP) &&
		    (auth_session_data->protocolID != TPM_PID_DSAP)) {
		    printf("TPM_Process_Unseal: Error, sealed with encryption but OIAP\n");
		    returnCode = TPM_BAD_MODE;
		}
	    }	    
	    /* c. If MSB of S2 -> et is TPM_ET_XOR */
	    /* i. Use MGF1 to create string X1 of length sealedDataSize. The inputs to MGF1 are;
	       authLastnonceEven, nonceOdd, "XOR", and authHandle -> sharedSecret. The four
	       concatenated values form the Z value that is the seed for MFG1. */
	    /* d. Else */
	    /* i. Create o1 by encrypting d1 -> data using the algorithm indicated by inData ->
	       et */
	    /* ii. Key is from authHandle -> sharedSecret */
	    /* iii. IV is SHA-1 of (authLastNonceEven || nonceOdd) */
	    if (returnCode == TPM_SUCCESS) {
		/* entity type MSB is ADIP encScheme */
		adipEncScheme = (s2StoredData->et >> 8) & 0x00ff;
		printf("TPM_Process_Unseal: Encrypting the output, encScheme %02x\n",
		       adipEncScheme);
		returnCode = TPM_SealCryptCommon(&o1Encrypted,
						 adipEncScheme,
						 &(d1SealedData.data),
						 auth_session_data,
						 nonceOdd);
		secretSize = d1SealedData.data.size;
		secret = o1Encrypted;
	    }
	    /* e. Set continueAuthSession to FALSE */
	    continueAuthSession = FALSE;
	}
	/* 10. else */
	else {
	    printf("TPM_Process_Unseal: No output encryption\n");
	    /* a. Set o1 to d1 -> data */
	    secretSize = d1SealedData.data.size;
	    secret = d1SealedData.data.buffer;
	}
    }
    /* 11. Set the return secret as o1 */
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_Unseal: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return secretSize */
	    returnCode = TPM_Sbuffer_Append32(response, secretSize);
	}
	if (returnCode == TPM_SUCCESS) {
	    /* return secret */
	    returnCode = TPM_Sbuffer_Append(response, secret, secretSize);
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
					    auth_session_data,
					    outParamDigest,
					    nonceOdd,
					    continueAuthSession);
	}
	/* calculate and set the below the line parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_AuthParams_Set(response,
					    *dataHmacKey,	/* HMAC key */
					    data_auth_session_data,
					    outParamDigest,
					    datanonceOdd,
					    continueDataSession);
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
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueDataSession) &&
	dataAuthHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, dataAuthHandle);
    }
    /*
      cleanup
    */
    TPM_StoredData_Delete(&inData, v1StoredDataVersion);	/* @1 */
    TPM_SealedData_Delete(&d1SealedData);			/* @2 */
    free(o1Encrypted);						/* @3 */
    return rcf;
}
	    
/* 10.3 TPM_UnBind rev 87

   TPM_UnBind takes the data blob that is the result of a Tspi_Data_Bind command and decrypts it
   for export to the User. The caller must authorize the use of the key that will decrypt the
   incoming blob.

   UnBind operates on a block-by-block basis, and has no notion of any relation between one block
   and another.

   UnBind SHALL operate on a single block only. 
*/

TPM_RESULT TPM_Process_UnBind(tpm_state_t *tpm_state,
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
					   UnBind operations. */
    TPM_SIZED_BUFFER	inData;		/* Encrypted blob to be decrypted */
    TPM_AUTHHANDLE	authHandle;	/* The handle used for keyHandle authorization */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   handle */
    TPM_AUTHDATA	privAuth;	/* The authorization digest that authorizes the inputs and
					   use of keyHandle. HMAC key: key.usageAuth. */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_KEY			*key = NULL;			/* the key specified by keyHandle */
    TPM_SECRET			*keyUsageAuth;
    TPM_RSA_KEY_PARMS		*tpm_rsa_key_parms;		/* for key */
    TPM_BOOL			parentPCRStatus;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    uint32_t			decrypt_data_size;		/* resulting decrypted data size */
    BYTE			*decrypt_data = NULL;		/* The resulting decrypted data. */
    unsigned char		*stream;
    uint32_t			stream_size;
    TPM_BOUND_DATA		tpm_bound_data;

    /* output parameters */
    uint32_t		outParamStart;		/* starting point of outParam's */
    uint32_t		outParamEnd;		/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    size_t		outDataSize = 0;	/* The length of the returned decrypted data */
    BYTE		*outData = NULL;	/* The resulting decrypted data. */
    
    printf("TPM_Process_UnBind: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&inData);		/* freed @1 */
    TPM_BoundData_Init(&tpm_bound_data);	/* freed @3 */
    /*
      get inputs
    */
    /*	get keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&keyHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get areaToSignSize and areaToSign parameters */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_UnBind: keyHandle %08x\n", keyHandle);
	returnCode = TPM_SizedBuffer_Load(&inData, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_UnBind: UnBinding %u bytes\n", inData.size);
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
	    printf("TPM_Process_UnBind: Error, command has %u extra bytes\n",
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
    /* 1. If the inDataSize is 0 the TPM returns TPM_BAD_PARAMETER */
    if (returnCode == TPM_SUCCESS) {
	if (inData.size == 0) {
	    printf("TPM_Process_UnBind: Error, inDataSize is 0\n");
	    returnCode = TPM_BAD_PARAMETER;
	}
    }
    /* get the key corresponding to the keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&key, &parentPCRStatus, tpm_state, keyHandle,
						 FALSE,		/* not read-only */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND)){
	if (key->authDataUsage != TPM_AUTH_NEVER) {
	    printf("TPM_Process_UnBind: Error, authorization required\n");
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
    /* 2. Validate the authorization to use the key pointed to by keyHandle */
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
    /* 3. If the keyUsage field of the key referenced by keyHandle does not have the value
       TPM_KEY_BIND or TPM_KEY_LEGACY, the TPM must return the error code TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	if ((key->keyUsage != TPM_KEY_BIND) && (key->keyUsage != TPM_KEY_LEGACY)) {
	    printf("TPM_Process_UnBind: Error, invalid keyUsage %04hx\n", (key->keyUsage));
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* Get the TPM_RSA_KEY_PARMS associated with key */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyParms_GetRSAKeyParms(&tpm_rsa_key_parms, &(key->algorithmParms));
    }	     
    /* 4. Decrypt the inData using the key pointed to by keyHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode =
	    TPM_RSAPrivateDecryptMalloc(&decrypt_data,		/* decrypted data, freed @2 */
					&decrypt_data_size,	/* actual size of decrypted data
								   data */
					inData.buffer,
					inData.size,
					key);
    }
    if (returnCode == TPM_SUCCESS) {
	/* 5. if (keyHandle -> encScheme does not equal TPM_ES_RSAESOAEP_SHA1_MGF1) and (keyHandle
	   -> keyUsage equals TPM_KEY_LEGACY), */
	if ((key->algorithmParms.encScheme != TPM_ES_RSAESOAEP_SHA1_MGF1) &&
	    (key->keyUsage == TPM_KEY_LEGACY)) {
	    printf("TPM_Process_UnBind: Legacy key\n");
	    /* a. The payload does not have TPM specific markers to validate, so no consistency
	       check can be performed. */
	    /* b. Set the output parameter outData to the value of the decrypted value of
	       inData. (Padding associated with the encryption wrapping of inData SHALL NOT be
	       returned.) */
	    outData = decrypt_data;
	    /* c. Set the output parameter outDataSize to the size of outData, as deduced from the
	       decryption process. */
	    outDataSize = decrypt_data_size;
	}
	/* 6. else */
	else {
	    printf("TPM_Process_UnBind: Payload is TPM_BOUND_DATA structure\n");
	    /* a. Interpret the decrypted data under the assumption that it is a TPM_BOUND_DATA
	       structure, and validate that the payload type is TPM_PT_BIND */
	    if (returnCode == TPM_SUCCESS) {
		stream = decrypt_data;
		stream_size = decrypt_data_size;
		returnCode = TPM_BoundData_Load(&tpm_bound_data,
						&stream,
						&stream_size);
	    }
	    if (returnCode == TPM_SUCCESS) {
		if (tpm_bound_data.payload != TPM_PT_BIND) {
		    printf("TPM_Process_UnBind: Error, "
			   "TPM_BOUND_DATA->payload %02x not TPM_PT_BIND\n",
			   tpm_bound_data.payload);
		    returnCode = TPM_INVALID_STRUCTURE;
		}
	    }
	    /* b. Set the output parameter outData to the value of TPM_BOUND_DATA ->
	       payloadData. (Other parameters of TPM_BOUND_DATA SHALL NOT be returned. Padding
	       associated with the encryption wrapping of inData SHALL NOT be returned.) */
	    if (returnCode == TPM_SUCCESS) {
		outData = tpm_bound_data.payloadData;
		/* c. Set the output parameter outDataSize to the size of outData, as deduced from
		   the decryption process and the interpretation of TPM_BOUND_DATA. */
		outDataSize = tpm_bound_data.payloadDataSize;
	    }
	}
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_UnBind: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* 10. Return the computed outData */
	    /* append outDataSize */
	    returnCode = TPM_Sbuffer_Append32(response, outDataSize);
	}
	if (returnCode == TPM_SUCCESS) {
	    /* append outData */
	    returnCode = TPM_Sbuffer_Append(response, outData, outDataSize);
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
    free(decrypt_data);				/* @2 */
    TPM_BoundData_Delete(&tpm_bound_data);	/* @3 */
    return rcf;
}

/* 10.4 TPM_CreateWrapKey rev 114

   The TPM_CreateWrapKey command both generates and creates a secure storage bundle for asymmetric
   keys.

   The newly created key can be locked to a specific PCR value by specifying a set of PCR registers.
*/

TPM_RESULT TPM_Process_CreateWrapKey(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE	parentHandle;	/* Handle of a loaded key that can perform key wrapping. */
    TPM_ENCAUTH		dataUsageAuth;	/* Encrypted usage authorization data for the key. */
    TPM_ENCAUTH		dataMigrationAuth;	/* Encrypted migration authorization data for the
						   key.*/
    TPM_KEY		keyInfo;	/* Information about key to be created, pubkey.keyLength and
					   keyInfo.encData elements are 0. MAY be TPM_KEY12 */
    TPM_AUTHHANDLE	authHandle;	/* The authorization handle used for parent key
					   authorization. Must be an OSAP session.  */
    TPM_NONCE	nonceOdd;		/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* Ignored */
    TPM_AUTHDATA pubAuth;		/* The authorization digest that authorizes the use of the
					   public key in parentHandle. HMAC key:
					   parentKey.usageAuth. */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_SECRET			*hmacKey;
    TPM_KEY			*parentKey = NULL;	/* the key specified by parentHandle */
    TPM_BOOL			parentPCRStatus;
    TPM_RSA_KEY_PARMS		*keyInfoRSAParms = NULL;	/* substructure of keyInfo */
    TPM_SECRET			du1UsageAuth;
    TPM_SECRET			dm1MigrationAuth;
    TPM_STORE_ASYMKEY		*wrappedStoreAsymkey;		/* substructure of wrappedKey */
    TPM_PCR_INFO		wrappedPCRInfo;
    int				ver;				/* TPM_KEY or TPM_KEY12 */

    /* output parameters */
    TPM_KEY		wrappedKey;	/* The TPM_KEY structure which includes the public and
					   encrypted private key. MAY be TPM_KEY12 */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    
    printf("TPM_Process_CreateWrapKey: Ordinal Entry\n");
    TPM_Key_Init(&keyInfo);
    TPM_Key_Init(&wrappedKey);
    TPM_PCRInfo_Init(&wrappedPCRInfo);
    /*
      get inputs
    */
    /* get parentHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&parentHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get dataUsageAuth parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CreateWrapKey: parentHandle %08x\n", parentHandle);
	returnCode = TPM_Authdata_Load(dataUsageAuth, &command, &paramSize);
    }
    /* get dataMigrationAuth parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Authdata_Load(dataMigrationAuth, &command, &paramSize);
    }
    /* get keyInfo parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_Load(&keyInfo, &command, &paramSize);	/* freed @1 */
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
	    printf("TPM_Process_CreateWrapKey: Error, command has %u extra bytes\n",
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
    /* get the key corresponding to the parentHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&parentKey, &parentPCRStatus, tpm_state,
						 parentHandle,
						 FALSE, /* not r/o, using to encrypt w/public key */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    /* get the session data */
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
    /* 1. Validate the authorization to use the key pointed to by parentHandle. Return TPM_AUTHFAIL
       on any error. */
    /* 2. Validate the session type for parentHandle is OSAP. */
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_CreateWrapKey: sharedSecret", auth_session_data->sharedSecret);
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle
								*/
					continueAuthSession,
					pubAuth);	/* Authorization digest for input */
    }
    /* 3. If the TPM is not designed to create a key of the type requested in keyInfo, return the
       error code TPM_BAD_KEY_PROPERTY */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CreateWrapKey: Checking key properties\n");
	returnCode = TPM_Key_CheckProperties(&ver, &keyInfo, 0,
					     tpm_state->tpm_permanent_flags.FIPS);
    }	     
    /* Get the TPM_RSA_KEY_PARMS associated with keyInfo */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CreateWrapKey: key parameters v = %d\n", ver);
	returnCode = TPM_KeyParms_GetRSAKeyParms(&keyInfoRSAParms, &(keyInfo.algorithmParms));
    }	     
    /* 4. Verify that parentHandle->keyUsage equals TPM_KEY_STORAGE */
    if (returnCode == TPM_SUCCESS) {
	if (parentKey->keyUsage != TPM_KEY_STORAGE) {
	    printf("TPM_Process_CreateWrapKey: Error, parent keyUsage not TPM_KEY_STORAGE\n");
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }	 
    /* 5. If parentHandle -> keyFlags -> migratable is TRUE and keyInfo -> keyFlags -> migratable is
       FALSE then return TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	if ((parentKey->keyFlags & TPM_MIGRATABLE) && !(keyInfo.keyFlags & TPM_MIGRATABLE)) {
	    printf("TPM_Process_CreateWrapKey: Error, parent not migratable\n");
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }	 
    /* 6. Validate key parameters */
    /* a. keyInfo -> keyUsage MUST NOT be TPM_KEY_IDENTITY or TPM_KEY_AUTHCHANGE. If it is, return
       TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	if ((keyInfo.keyUsage == TPM_KEY_IDENTITY) ||
	    (keyInfo.keyUsage == TPM_KEY_AUTHCHANGE)) {
	    printf("TPM_Process_CreateWrapKey: Error, Invalid key usage %04x\n",
		   keyInfo.keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }	 
    /* b. If keyInfo -> keyFlags -> migrateAuthority is TRUE then return TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	if (keyInfo.keyFlags & TPM_MIGRATEAUTHORITY) {
	    printf("TPM_Process_CreateWrapKey: Error, Invalid key flags %08x\n",
		   keyInfo.keyFlags);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }	 
    /* 7.  If TPM_PERMANENT_FLAGS -> FIPS is TRUE then
       a.  If keyInfo -> keySize is less than 1024 return TPM_NOTFIPS
       b.  If keyInfo -> authDataUsage specifies TPM_AUTH_NEVER return TPM_NOTFIPS
       c.  If keyInfo -> keyUsage specifies TPM_KEY_LEGACY return TPM_NOTFIPS
       NOTE Done in step 3 TPM_Key_CheckProperties()
    */
    /* 8. If keyInfo -> keyUsage equals TPM_KEY_STORAGE	 or TPM_KEY_MIGRATE
       i. algorithmID MUST be TPM_ALG_RSA
       ii. encScheme MUST be TPM_ES_RSAESOAEP_SHA1_MGF1
       iii. sigScheme MUST be TPM_SS_NONE
       iv. key size MUST be 2048
       v. exponentSize MUST be 0
       NOTE Done in step 3 TPM_Key_CheckProperties()
    */
    /* 9. Determine the version of key
       a.If keyInfo -> ver is 1.1
       i. Set V1 to 1
       ii. Map wrappedKey to a TPM_KEY structure
       iii. Validate all remaining TPM_KEY structures
       b. Else if keyInfo -> tag is TPM_TAG_KEY12
       i. Set V1 to 2
       ii. Map wrappedKey to a TPM_KEY12 structure
       iii. Validate all remaining TPM_KEY12 structures
       NOTE Check done by TPM_Key_CheckProperties()
       NOTE Map done by TPM_Key_GenerateRSA()
    */
    /* 10..Create DU1 by decrypting dataUsageAuth according to the ADIP indicated by authHandle */
    /* 11. Create DM1 by decrypting dataMigrationAuth according to the ADIP indicated by
	   authHandle */
    if (returnCode == TPM_SUCCESS) {
	TPM_AuthSessionData_Decrypt(du1UsageAuth, 
				    dm1MigrationAuth,
				    dataUsageAuth,	/* even encAuth */
				    auth_session_data,
				    nonceOdd,
				    dataMigrationAuth,	/* odd encAuth */
				    TRUE);
    }
    /* 12. Set continueAuthSession to FALSE */
    if (returnCode == TPM_SUCCESS) {
	continueAuthSession = FALSE;
    }
    /* 13. Generate asymmetric key according to algorithm information in keyInfo */
    /* 14. Fill in the wrappedKey structure with information from the newly generated key. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CreateWrapKey: Generating key\n");
	returnCode = TPM_Key_GenerateRSA(&wrappedKey,
					 tpm_state,
					 parentKey,
					 tpm_state->tpm_stclear_data.PCRS,	/* PCR array */
					 ver,
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
	TPM_Secret_Copy(wrappedStoreAsymkey->usageAuth, du1UsageAuth);
	/* b. If the KeyFlags -> migratable bit is set to 1, the wrappedKey -> encData ->
	   migrationAuth SHALL contain the decrypted value from dataMigrationAuth. */
	if (wrappedKey.keyFlags & TPM_MIGRATABLE) {
	    TPM_Secret_Copy(wrappedStoreAsymkey->migrationAuth, dm1MigrationAuth);
	}
	/* c. If the KeyFlags -> migratable bit is set to 0, the wrappedKey -> encData ->
	   migrationAuth SHALL be set to the value tpmProof */
	else {
	    TPM_Secret_Copy(wrappedStoreAsymkey->migrationAuth,
			    tpm_state->tpm_permanent_data.tpmProof);
	}
	printf("TPM_Process_CreateWrapKey: wrappedKey.PCRInfoSize %d\n", wrappedKey.pcrInfo.size);
    }	 
    /* 15. If keyInfo->PCRInfoSize is non-zero. */
    /* a. If V1 is 1 */
    /* i. Set wrappedKey -> pcrInfo to a TPM_PCR_INFO structure using the pcrSelection to
       indicate the PCR's in use */
    /* b. Else */
    /* i. Set wrappedKey -> pcrInfo to a TPM_PCR_INFO_LONG structure */
    /* c. Set wrappedKey -> pcrInfo to keyInfo -> pcrInfo */
    /* d. Set wrappedKey -> digestAtCreation to the TPM_COMPOSITE_HASH indicated by
       creationPCRSelection */
    /* e. If V1 is 2 set wrappedKey -> localityAtCreation to TPM_STANY_DATA -> locality */
    /* NOTE Done by TPM_Key_GenerateRSA() */
    /* 16. Encrypt the private portions of the wrappedKey structure using the key in parentHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_GenerateEncData(&wrappedKey, parentKey);
    }	 
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_CreateWrapKey: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* 17. Return the newly generated key in the wrappedKey parameter */
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
    /* cleanup */
    TPM_Key_Delete(&keyInfo);			/* @1 */
    TPM_Key_Delete(&wrappedKey);		/* @2 */
    TPM_PCRInfo_Delete(&wrappedPCRInfo);	/* @3 */
    return rcf;
}

/* 27.8 TPM_LoadKey rev 114

   Version 1.2 deprecates LoadKey due to the HMAC of the new keyhandle on return. The wrapping makes
   use of the handle difficult in an environment where the TSS, or other management entity, is
   changing the TPM handle to a virtual handle.
   
   Software using loadKey on a 1.2 TPM can have a collision with the returned handle as the 1.2 TPM
   uses random values in the lower three bytes of the handle. All new software must use LoadKey2 to
   allow management software the ability to manage the key handle.
   
   Before the TPM can use a key to either wrap, unwrap, bind, unbind, seal, unseal, sign or perform
   any other action, it needs to be present in the TPM.	 The TPM_LoadKey function loads the key into
   the TPM for further use.
*/

TPM_RESULT TPM_Process_LoadKey(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE	parentHandle;	/* TPM handle of parent key. */
    TPM_KEY		*inKey;		/* Incoming key structure, both encrypted private and clear
					   public portions.  MAY be TPM_KEY12 */
    TPM_AUTHHANDLE	authHandle;	/* The authorization handle used for parentHandle
					   authorization. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = FALSE;	/* The continue use flag for the authorization
						   handle */
    TPM_AUTHDATA	parentAuth;	/* The authorization digest for inputs and
					   parentHandle. HMAC key: parentKey.usageAuth. */
    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_BOOL			key_added = FALSE;	/* key has been added to handle list */
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    
    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_KEY_HANDLE	inKeyHandle;	/* Internal TPM handle where decrypted key was loaded. */

    printf("TPM_Process_LoadKey: Ordinal Entry\n");
    inKey = NULL;			/* freed @1 */
    /*
      get inputs
    */
    /*	get parentHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&parentHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* Allocate space for inKey.  The key cannot be a local variable, since it persists in key
       storage after the command completes. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadKey: parentHandle %08x\n", parentHandle);
	returnCode = TPM_Malloc((unsigned char **)&inKey, sizeof(TPM_KEY));	/* freed @1 */
    }
    /* get inKey parameter */
    if (returnCode == TPM_SUCCESS) {
	TPM_Key_Init(inKey);					/* freed @2 */
	returnCode = TPM_Key_Load(inKey, &command, &paramSize); /* freed @2 */
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_LoadKey: inKey n", inKey->pubKey.buffer);
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
	    printf("TPM_Process_LoadKey: Error, command has %u extra bytes\n",
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
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_LoadKeyCommon(&inKeyHandle,	/* output */
				       &key_added,	/* output */
				       &hmacKey,	/* output */
				       &auth_session_data, /* output */
				       tpm_state,
				       tag,
				       ordinal,
				       parentHandle,
				       inKey,
				       inParamDigest,
				       authHandle,	/*uninitialized*/
				       nonceOdd,
				       continueAuthSession,
				       parentAuth);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_LoadKey: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return the key handle */
	    returnCode = TPM_Sbuffer_Append32(response, inKeyHandle);
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
    /* if there was a failure, delete inKey */
    if ((rcf != 0) || (returnCode != TPM_SUCCESS)) {
	TPM_Key_Delete(inKey);	/* @2 */
	free(inKey);		/* @1 */
	if (key_added) {
	    /* if there was a failure and inKey was stored in the handle list, free the handle.
	       Ignore errors, since only one error code can be returned. */
	    TPM_KeyHandleEntries_DeleteHandle(tpm_state->tpm_key_handle_entries, inKeyHandle);
	}	
    }
    return rcf;
}
	    
/* 10.5 TPM_LoadKey2 rev 107

   Before the TPM can use a key to either wrap, unwrap, unbind, seal, unseal, sign or perform any
   other action, it needs to be present in the TPM.  The TPM_LoadKey2 function loads the key into
   the TPM for further use.

   The TPM assigns the key handle. The TPM always locates a loaded key by use of the handle. The
   assumption is that the handle may change due to key management operations. It is the
   responsibility of upper level software to maintain the mapping between handle and any label used
   by external software.

   This command has the responsibility of enforcing restrictions on the use of keys. For example,
   when attempting to load a STORAGE key it will be checked for the restrictions on a storage key
   (2048 size etc.).

   The load command must maintain a record of whether any previous key in the key hierarchy was
   bound to a PCR using parentPCRStatus.
   
   The flag parentPCRStatus enables the possibility of checking that a platform passed through some
   particular state or states before finishing in the current state. A grandparent key could be
   linked to state-1, a parent key could linked to state-2, and a child key could be linked to
   state-3, for example. The use of the child key then indicates that the platform passed through
   states 1 and 2 and is currently in state 3, in this example.	 TPM_Startup with stType ==
   TPM_ST_CLEAR indicates that the platform has been reset, so the platform has not passed through
   the previous states. Hence keys with parentPCRStatus==TRUE must be unloaded if TPM_Startup is
   issued with stType == TPM_ST_CLEAR.
   
   If a TPM_KEY structure has been decrypted AND the integrity test using "pubDataDigest" has passed
   AND the key is non-migratory, the key must have been created by the TPM. So there is every reason
   to believe that the key poses no security threat to the TPM. While there is no known attack from
   a rogue migratory key, there is a desire to verify that a loaded migratory key is a real key,
   arising from a general sense of unease about execution of arbitrary data as a key. Ideally a
   consistency check would consist of an encrypt/decrypt cycle, but this may be expensive. For RSA
   keys, it is therefore suggested that the consistency test consists of dividing the supposed RSA
   product by the supposed RSA prime, and checking that there is no remainder.
*/

TPM_RESULT TPM_Process_LoadKey2(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE	parentHandle;	/* TPM handle of parent key. */
    TPM_KEY		*inKey;		/* Incoming key structure, both encrypted private and clear
					   public portions.  MAY be TPM_KEY12 */
    TPM_AUTHHANDLE	authHandle;	/* The authorization handle used for parentHandle
					   authorization. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = FALSE;	/* The continue use flag for the authorization
						   handle */
    TPM_AUTHDATA	parentAuth;	/* The authorization digest for inputs and
					   parentHandle. HMAC key: parentKey.usageAuth. */
    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_BOOL			key_added = FALSE;	/* key has been added to handle list */
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    
    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_KEY_HANDLE	inKeyHandle;	/* Internal TPM handle where decrypted key was loaded. */

    printf("TPM_Process_LoadKey2: Ordinal Entry\n");
    inKey = NULL;			/* freed @1 */
    /*
      get inputs
    */
    /*	get parentHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&parentHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* Allocate space for inKey.  The key cannot be a local variable, since it persists in key
       storage after the command completes. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadKey2: parentHandle %08x\n", parentHandle);
	returnCode = TPM_Malloc((unsigned char **)&inKey, sizeof(TPM_KEY));	/* freed @1 */
    }
    /* get inKey parameter */
    if (returnCode == TPM_SUCCESS) {
	TPM_Key_Init(inKey);					/* freed @2 */
	returnCode = TPM_Key_Load(inKey, &command, &paramSize); /* freed @2 */
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_LoadKey2: inKey n", inKey->pubKey.buffer);
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
	    printf("TPM_Process_LoadKey2: Error, command has %u extra bytes\n",
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
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_LoadKeyCommon(&inKeyHandle,		/* output */
				       &key_added,		/* output */
				       &hmacKey,		/* output */
				       &auth_session_data,	/* output */
				       tpm_state,
				       tag,
				       ordinal,
				       parentHandle,
				       inKey,
				       inParamDigest,
				       authHandle,		/* uninitialized */
				       nonceOdd,
				       continueAuthSession,
				       parentAuth);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_LoadKey2: Ordinal returnCode %08x %u\n",
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
	    /* In TPM_LoadKey2, the inKeyHandle is not part of the output HMAC */
	    /* return the key handle */
	    returnCode = TPM_Sbuffer_Append32(response, inKeyHandle);
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
    /* if there was a failure, delete inKey */
    if ((rcf != 0) || (returnCode != TPM_SUCCESS)) {
	TPM_Key_Delete(inKey);	/* @2 */
	free(inKey);		/* @1 */
	if (key_added) {
	    /* if there was a failure and inKey was stored in the handle list, free the handle.
	       Ignore errors, since only one error code can be returned. */
	    TPM_KeyHandleEntries_DeleteHandle(tpm_state->tpm_key_handle_entries, inKeyHandle);
	}	
    }
    return rcf;
}

/* TPM_LoadKeyCommon rev 114

   Code common to TPM_LoadKey and TPM_LoadKey2.	 They differ only in whether the key handle is
   included in the response HMAC calculation.
*/

static TPM_RESULT TPM_LoadKeyCommon(TPM_KEY_HANDLE	*inKeyHandle,	/* output */
				    TPM_BOOL		*key_added,	/* output */
				    TPM_SECRET		**hmacKey,	/* output */
				    TPM_AUTH_SESSION_DATA	**auth_session_data, /* output */
				    tpm_state_t		*tpm_state,
				    TPM_TAG		tag,
				    TPM_COMMAND_CODE	ordinal,
				    TPM_KEY_HANDLE	parentHandle,
				    TPM_KEY		*inKey,
				    TPM_DIGEST		inParamDigest,
				    TPM_AUTHHANDLE	authHandle,
				    TPM_NONCE		nonceOdd,
				    TPM_BOOL		continueAuthSession,
				    TPM_AUTHDATA	parentAuth)
{
    TPM_RESULT			rc = 0;
    TPM_KEY			*parentKey;		/* the key specified by parentHandle */
    TPM_SECRET			*parentUsageAuth;
    TPM_BOOL			parentPCRStatus;
    TPM_BOOL			parentPCRUsage;
    int				ver;

    printf("TPM_LoadKeyCommon:\n");
    *key_added = FALSE; /* key has been added to handle list */
    /* Verify that parentHandle points to a valid key.	Get the TPM_KEY associated with parentHandle
     */
    if (rc == TPM_SUCCESS) {
	rc = TPM_KeyHandleEntries_GetKey(&parentKey, &parentPCRStatus,
					 tpm_state, parentHandle,
					 FALSE,		/* not r/o, using to decrypt */
					 FALSE,		/* do not ignore PCRs */
					 FALSE);	/* cannot use EK */
    }
    /* check TPM_AUTH_DATA_USAGE authDataUsage */
    if ((rc == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND)) {
	if (parentKey->authDataUsage != TPM_AUTH_NEVER) {
	    printf("TPM_LoadKeyCommon: Error, authorization required\n");
	    rc = TPM_AUTHFAIL;
	}
    }
    /* get parentHandle -> usageAuth */
    if ((rc == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	rc = TPM_Key_GetUsageAuth(&parentUsageAuth, parentKey);
    }	 
    /* get the session data */
    if ((rc == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	rc = TPM_AuthSessions_GetData(auth_session_data,
				      hmacKey,
				      tpm_state,
				      authHandle,
				      TPM_PID_NONE,
				      TPM_ET_KEYHANDLE,
				      ordinal,
				      parentKey,
				      parentUsageAuth,			/* OIAP */
				      parentKey->tpm_store_asymkey->pubDataDigest);	/* OSAP */
    }
    /* 1. Validate the command and the parameters using parentAuth and parentHandle -> usageAuth */
    if ((rc == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	rc = TPM_Authdata_Check(tpm_state,
				**hmacKey,		/* HMAC key */
				inParamDigest,
				*auth_session_data,	/* authorization session */
				nonceOdd,		/* Nonce generated by system
							   associated with authHandle */
				continueAuthSession,
				parentAuth);		/* Authorization digest for input */
    }
    /* 2. If parentHandle -> keyUsage is NOT TPM_KEY_STORAGE return TPM_INVALID_KEYUSAGE */
    if (rc == TPM_SUCCESS) {
	if (parentKey->keyUsage != TPM_KEY_STORAGE) {
	    printf("TPM_LoadKeyCommon: Error, "
		   "parentHandle -> keyUsage should be TPM_KEY_STORAGE, is %04x\n",
		   parentKey->keyUsage);
	    rc = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 3. If the TPM is not designed to operate on a key of the type specified by inKey, return the
       error code TPM_BAD_KEY_PROPERTY.	 */
    if (rc == TPM_SUCCESS) {
	rc = TPM_Key_CheckProperties(&ver, inKey, 0, tpm_state->tpm_permanent_flags.FIPS);
	printf("TPM_LoadKeyCommon: key parameters v = %d\n", ver);
    }
    /* 4. The TPM MUST handle both TPM_KEY and TPM_KEY12 structures.
       This step is done at TPM_Key_Load()
    */
    /* 5. Decrypt the inKey -> privkey to obtain TPM_STORE_ASYMKEY structure using the key in
       parentHandle.
    */
    if (rc == TPM_SUCCESS) {
	rc = TPM_Key_DecryptEncData(inKey, parentKey);
    }
    /* 6. Validate the integrity of inKey and decrypted TPM_STORE_ASYMKEY
       a. Reproduce inKey -> TPM_STORE_ASYMKEY -> pubDataDigest using the fields of inKey, and check
       that the reproduced value is the same as pubDataDigest
    */
    if (rc == TPM_SUCCESS) {
	rc = TPM_Key_CheckPubDataDigest(inKey);
    }
    /* 7. Validate the consistency of the key and it's key usage. */
    /* a. If inKey -> keyFlags -> migratable is TRUE, the TPM SHALL verify consistency of the public
       and private components of the asymmetric key pair. If inKey -> keyFlags -> migratable is
       FALSE, the TPM MAY verify consistency of the public and private components of the asymmetric
       key pair. The consistency of an RSA key pair MAY be verified by dividing the supposed (P*Q)
       product by a supposed prime and checking that there is no remainder.

       This step is done at TPM_Key_Load()
    */
    /* b.  If inKey -> keyUsage is TPM_KEY_IDENTITY, verify that inKey->keyFlags->migratable is
       FALSE. If it is not, return TPM_INVALID_KEYUSAGE
    */
    if (rc == TPM_SUCCESS) {
	if ((inKey->keyUsage == TPM_KEY_IDENTITY) &&
	    (inKey->keyFlags & TPM_MIGRATABLE)) {
	    printf("TPM_LoadKeyCommon: Error, identity key is migratable\n");
	    rc = TPM_INVALID_KEYUSAGE;
	}
    }
    /* c.  If inKey -> keyUsage is TPM_KEY_AUTHCHANGE, return TPM_INVALID_KEYUSAGE */
    if (rc == TPM_SUCCESS) {
	if (inKey->keyUsage == TPM_KEY_AUTHCHANGE) {
	    printf("TPM_LoadKeyCommon: Error, keyUsage is TPM_KEY_AUTHCHANGE\n");
	    rc = TPM_INVALID_KEYUSAGE;
	}
    }
    /* d.  If inKey -> keyFlags -> migratable equals 0 then verify that TPM_STORE_ASYMKEY ->
       migrationAuth equals TPM_PERMANENT_DATA -> tpmProof */
    if (rc == TPM_SUCCESS) {
	if (!(inKey->keyFlags & TPM_MIGRATABLE)) {
	    rc = TPM_Secret_Compare(tpm_state->tpm_permanent_data.tpmProof,
				   inKey->tpm_store_asymkey->migrationAuth);
	    if (rc != 0) {
		printf("TPM_LoadKeyCommon: Error, tpmProof mismatch\n");
		rc = TPM_INVALID_KEYUSAGE;
	    }
	}
    }
    /*	 e.   Validate the mix of encryption and signature schemes
	 f.   If TPM_PERMANENT_FLAGS -> FIPS is TRUE then
	 i.   If keyInfo -> keySize is less than 1024 return TPM_NOTFIPS
	 ii.  If keyInfo -> authDataUsage specifies TPM_AUTH_NEVER return
	 TPM_NOTFIPS
	 iii.  If  keyInfo  ->	keyUsage specifies TPM_KEY_LEGACY  return
	 TPM_NOTFIPS
	 g.   If inKey -> keyUsage is TPM_KEY_STORAGE or TPM_KEY_MIGRATE
	 i.   algorithmID MUST be TPM_ALG_RSA
	 ii.  Key size MUST be 2048
	 iii. exponentSize MUST be 0
	 iv. sigScheme MUST be TPM_SS_NONE
	 h.   If inKey -> keyUsage is TPM_KEY_IDENTITY
	 i.   algorithmID MUST be TPM_ALG_RSA
	 ii.  Key size MUST be 2048
	 iv. exponentSize MUST be 0
	 iii. encScheme MUST be TPM_ES_NONE
	 NOTE Done in step 3.  
    */
    if (rc == TPM_SUCCESS) {
	/* i. If the decrypted inKey -> pcrInfo is NULL, */
	/* i. The TPM MUST set the internal indicator to indicate that the key is not using any PCR
	   registers. */
	/* j.  Else */
	/* i. The TPM MUST store pcrInfo in a manner that allows the TPM to calculate a composite
	   hash whenever the key will be in use */
	/* ii. The TPM MUST handle both version 1.1 TPM_PCR_INFO and 1.2 TPM_PCR_INFO_LONG
	   structures according to the type of TPM_KEY structure */
	/* (1) The TPM MUST validate the TPM_PCR_INFO or TPM_PCR_INFO_LONG structures for legal
	       values.	However, the digestAtRelease and localityAtRelease are not validated for
	       authorization until use time.*/
	/* NOTE TPM_Key_Load() loads the TPM_PCR_INFO or TPM_PCR_INFO_LONG cache */
    }
    /* 8.  Perform any processing necessary to make TPM_STORE_ASYMKEY key available for
       operations. */
    /* NOTE Done at TPM_Key_Load() */
    /* 9. Load key and key information into internal memory of the TPM. If insufficient memory
       exists return error TPM_NOSPACE. */
    /* 10. Assign inKeyHandle according to internal TPM rules. */
    /* 11. Set InKeyHandle -> parentPCRStatus to parentHandle -> parentPCRStatus. */
    if (rc == TPM_SUCCESS) {
	*inKeyHandle = 0;	/* no preferred value */
	rc = TPM_KeyHandleEntries_AddKeyEntry(inKeyHandle,			/* output */
					      tpm_state->tpm_key_handle_entries, /* input */
					      inKey,				/* input */
					      parentPCRStatus,
					      0);			/* keyControl */
    }
    if (rc == TPM_SUCCESS) {
	printf(" TPM_LoadKeyCommon: Loaded key handle %08x\n", *inKeyHandle);
	/* remember that the handle has been added to handle list, so it can be deleted on error */
	*key_added = TRUE;
	
    }
    /* 12. If parentHandle indicates it is using PCR registers then set inKeyHandle ->
       parentPCRStatus to TRUE. */
    if (rc == TPM_SUCCESS) {
	rc = TPM_Key_GetPCRUsage(&parentPCRUsage, parentKey, 0);
    }
    if (rc == TPM_SUCCESS) {
	if (parentPCRUsage) {
	    rc = TPM_KeyHandleEntries_SetParentPCRStatus(tpm_state->tpm_key_handle_entries,
							 *inKeyHandle, TRUE);
	}
    }	
    return rc;
}

/* 10.6 TPM_GetPubKey rev 102

   The owner of a key may wish to obtain the public key value from a loaded key. This information
   may have privacy concerns so the command must have authorization from the key owner.
*/

TPM_RESULT TPM_Process_GetPubKey(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE	keyHandle;	/* TPM handle of key. */
    TPM_AUTHHANDLE	authHandle;	/* The authorization handle used for keyHandle
					   authorization. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/*The continue use flag for the authorization
						  handle */
    TPM_AUTHDATA	keyAuth;	/* Authorization HMAC key: key.usageAuth. */
    
    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_KEY			*key = NULL;		/* the key specified by keyHandle */
    TPM_BOOL			parentPCRStatus;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_SECRET			*keyUsageAuth;
    TPM_STORE_BUFFER		pubkeyStream;

    /* output parameters */
    uint32_t			outParamStart;		/* starting point of outParam's */
    uint32_t			outParamEnd;		/* ending point of outParam's */
    TPM_DIGEST			outParamDigest;
    const unsigned char		*pubkeyStreamBuffer;	/* output */
    uint32_t			pubkeyStreamLength;

    printf("TPM_Process_GetPubKey: Ordinal Entry\n");
    TPM_Sbuffer_Init(&pubkeyStream);		/* freed @1 */
    /*
      get inputs
    */
    /* get keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&keyHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_GetPubKey: keyHandle %08x\n", keyHandle);
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
					keyAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_GetPubKey: Error, command has %u extra bytes\n",
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
    /* get the key corresponding to the keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_GetPubKey: Key handle %08x\n", keyHandle);
	returnCode = TPM_KeyHandleEntries_GetKey(&key, &parentPCRStatus, tpm_state, keyHandle,
						 TRUE,		/* read-only */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    /* 1. If tag = TPM_TAG_RQU_AUTH1_COMMAND then */
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
					      key->tpm_store_asymkey->pubDataDigest);	/* OSAP */
    }


    /* a. Validate the command parameters using keyHandle -> usageAuth, on error return
       TPM_AUTHFAIL */
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
    /* 2. Else	*/
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND)){
	/* a. Verify that keyHandle -> authDataUsage is TPM_NO_READ_PUBKEY_AUTH or TPM_AUTH_NEVER,
	   on error return TPM_AUTHFAIL */
#ifdef TPM_V12
	if ((key->authDataUsage != TPM_NO_READ_PUBKEY_AUTH) &&
	    (key->authDataUsage != TPM_AUTH_NEVER)) {
	    printf("TPM_Process_GetPubKey: Error, authorization required\n");
	    returnCode = TPM_AUTHFAIL;
	}
#else	/* TPM 1.1 does not have TPM_NO_READ_PUBKEY_AUTH */
	if (key->authDataUsage != TPM_AUTH_NEVER) {
	    printf("TPM_Process_GetPubKey: Error, authorization required\n");
	    returnCode = TPM_AUTHFAIL;
	}
#endif
    }
#ifdef TPM_V12	/* TPM 1.1 does not have readSRKPub  */
    if (returnCode == TPM_SUCCESS) {
	/* 3. If keyHandle == TPM_KH_SRK then  */
	if ((keyHandle == TPM_KH_SRK) &&
	    /* a. If TPM_PERMANENT_FLAGS -> readSRKPub is FALSE then return TPM_INVALID_KEYHANDLE */
	    !tpm_state->tpm_permanent_flags.readSRKPub) {
	    printf("TPM_Process_GetPubKey: "
		   "Error, keyHandle is TPM_KH_SRK and readSRKPub is FALSE\n");
	    returnCode = TPM_INVALID_KEYHANDLE;
	}
    }
#endif
    /* 4. If keyHandle -> pcrInfoSize is not 0 */
    /* a. If keyHandle -> keyFlags has pcrIgnoredOnRead set to FALSE */
    /* i. Create a digestAtRelease according to the specified PCR registers and compare
       to keyHandle -> digestAtRelease and if a mismatch return TPM_WRONGPCRVAL */
    /* ii. If specified validate any locality requests */
    /* NOTE: Done at TPM_KeyHandleEntries_GetKey() */
    /* 5. Create a TPM_PUBKEY structure and return */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_StorePubkey(&pubkeyStream,		/* output */
					 &pubkeyStreamBuffer,	/* output */
					 &pubkeyStreamLength,	/* output */
					 key);			/* input */
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_GetPubKey: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* TPM_PUBKEY structure  */
	    returnCode = TPM_Sbuffer_Append(response, pubkeyStreamBuffer, pubkeyStreamLength);
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
    TPM_Sbuffer_Delete(&pubkeyStream);		/* @1 */
    return rcf;
}
