/********************************************************************************/
/*										*/
/*				Key Handler					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_key.c $		*/
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
#include <string.h>
#include <stdlib.h>

#include "tpm_auth.h"
#include "tpm_commands.h"
#include "tpm_crypto.h"
#include "tpm_cryptoh.h"
#include "tpm_debug.h"
#include "tpm_digest.h"
#include "tpm_error.h"
#include "tpm_init.h"
#include "tpm_io.h"
#include "tpm_load.h"
#include "tpm_memory.h"
#include "tpm_nonce.h"
#include "tpm_nvfile.h"
#include "tpm_nvram.h"
#include "tpm_owner.h"
#include "tpm_pcr.h"
#include "tpm_sizedbuffer.h"
#include "tpm_store.h"
#include "tpm_structures.h"
#include "tpm_startup.h"
#include "tpm_permanent.h"
#include "tpm_process.h"
#include "tpm_ver.h"

#include "tpm_key.h"

/* The default RSA exponent */
unsigned char tpm_default_rsa_exponent[] = {0x01, 0x00, 0x01};

/* local prototypes */

static TPM_RESULT TPM_Key_CheckTag(TPM_KEY12 *tpm_key12);

/*
  TPM_KEY, TPM_KEY12

  These functions generally handle either a TPM_KEY or TPM_KEY12.  Where structure members differ,
  the function checks the version or tag and adapts the processing to the structure type.  This
  handling is opaque to the caller.
*/


/* TPM_Key_Init initializes a key structure.  The default is TPM_KEY.  Typically, a TPM_Key_Set() or
   TPM_Key_Load() will adjust to TPM_KEY or TPM_KEY12 */

void TPM_Key_Init(TPM_KEY *tpm_key)
{
    printf(" TPM_Key_Init:\n");
    TPM_StructVer_Init(&(tpm_key->ver));
    tpm_key->keyUsage = TPM_KEY_UNINITIALIZED;
    tpm_key->keyFlags = 0;
    tpm_key->authDataUsage = 0;
    TPM_KeyParms_Init(&(tpm_key->algorithmParms));
    TPM_SizedBuffer_Init(&(tpm_key->pcrInfo));
    TPM_SizedBuffer_Init(&(tpm_key->pubKey));
    TPM_SizedBuffer_Init(&(tpm_key->encData));
    tpm_key->tpm_pcr_info = NULL;
    tpm_key->tpm_pcr_info_long = NULL;
    tpm_key->tpm_store_asymkey = NULL;
    tpm_key->tpm_migrate_asymkey = NULL;
    return;
}

/* TPM_Key_InitTag12() alters the tag and fill from TPM_KEY to TPM_KEY12 */

void TPM_Key_InitTag12(TPM_KEY *tpm_key)
{
    printf(" TPM_Key_InitTag12:\n");
    ((TPM_KEY12 *)tpm_key)->tag = TPM_TAG_KEY12;
    ((TPM_KEY12 *)tpm_key)->fill = 0x0000;
    return;
}

/* TPM_Key_Set() sets a TPM_KEY structure to the specified values.

   The tpm_pcr_info digestAtCreation is calculated.

   It serializes the tpm_pcr_info or tpm_pcr_info_long cache to pcrInfo.  One or the other may be
   specified, but not both.  The tag/version is set correctly.

   If the parent_key is NULL, encData is set to the clear text serialization of the
   tpm_store_asymkey member.

   If parent_key is not NULL, encData is not set yet, since further processing may be done before
   encryption.

   Must call TPM_Key_Delete() to free
 */

TPM_RESULT TPM_Key_Set(TPM_KEY *tpm_key,		/* output created key */
		       tpm_state_t *tpm_state,
		       TPM_KEY *parent_key,		/* NULL for root keys */
		       TPM_DIGEST *tpm_pcrs,		/* points to the TPM PCR array */
		       int ver,				/* TPM_KEY or TPM_KEY12 */
		       TPM_KEY_USAGE keyUsage,				/* input */
		       TPM_KEY_FLAGS keyFlags,				/* input */
		       TPM_AUTH_DATA_USAGE authDataUsage,		/* input */
		       TPM_KEY_PARMS *tpm_key_parms,			/* input */
		       TPM_PCR_INFO *tpm_pcr_info,			/* must copy */
		       TPM_PCR_INFO_LONG *tpm_pcr_info_long,		/* must copy */
		       uint32_t keyLength,		/* public key length in bytes */
		       BYTE* publicKey,			/* public key byte array */
		       TPM_STORE_ASYMKEY *tpm_store_asymkey,	 /* cache TPM_STORE_ASYMKEY */
		       TPM_MIGRATE_ASYMKEY *tpm_migrate_asymkey) /* cache TPM_MIGRATE_ASYMKEY */
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	sbuffer;
     
    printf(" TPM_Key_Set:\n");
    TPM_Sbuffer_Init(&sbuffer);
    /* version must be TPM_KEY or TPM_KEY12 */
    if (rc == 0) {
	if ((ver != 1) && (ver != 2)) {
	    printf("TPM_Key_Set: Error (fatal), "
		   "TPM_KEY version %d is not 1 or 2\n", ver);
	    rc = TPM_FAIL;	/* should never occur */
	}
    }
    /* either tpm_pcr_info != NULL for TPM_KEY or tpm_pcr_info_long != NULL for TPM_KEY12, but not
       both */
    if (rc == 0) {
	if ((ver == 1) && (tpm_pcr_info_long != NULL)) {
	    printf("TPM_Key_Set: Error (fatal), "
		   "TPM_KEY and TPM_PCR_INFO_LONG both specified\n");
	    rc = TPM_FAIL;	/* should never occur */
	}
    }
    if (rc == 0) {
	if ((ver == 2) && (tpm_pcr_info != NULL)) {
	    printf("TPM_Key_Set: Error (fatal), "
		   "TPM_KEY12 and TPM_PCR_INFO both specified\n");
	    rc = TPM_FAIL;	/* should never occur */
	}
    }
    if (rc == 0) {
	TPM_Key_Init(tpm_key);
	if (ver == 2) {
	    TPM_Key_InitTag12(tpm_key);		/* change tag to TPM_KEY12 */
	}
	tpm_key->keyUsage = keyUsage;
	tpm_key->keyFlags = keyFlags;
	tpm_key->authDataUsage = authDataUsage;
	rc = TPM_KeyParms_Copy(&(tpm_key->algorithmParms),	/* freed by caller */
			       tpm_key_parms);
    }
    /* The pcrInfo serialization is deferred, since PCR data is be altered after the initial
       'set'. */
    if (rc == 0) {
	/* generate the TPM_PCR_INFO member cache, directly copying from the tpm_pcr_info */
	if (tpm_pcr_info != NULL) {	/* TPM_KEY */
	    rc = TPM_PCRInfo_CreateFromInfo(&(tpm_key->tpm_pcr_info), tpm_pcr_info);
	}
	/* generate the TPM_PCR_INFO_LONG member cache, directly copying from the
	   tpm_pcr_info_long */
	else if (tpm_pcr_info_long != NULL) {	/* TPM_KEY12 */
	    rc = TPM_PCRInfoLong_CreateFromInfoLong(&(tpm_key->tpm_pcr_info_long),
						    tpm_pcr_info_long);
	}
    }
    if (rc == 0) {
	/* if there are PCR's specified, set the digestAtCreation */
	if (tpm_pcr_info != NULL) {
	    rc = TPM_PCRInfo_SetDigestAtCreation(tpm_key->tpm_pcr_info, tpm_pcrs);
	}
	/* if there are PCR's specified, set the localityAtCreation, digestAtCreation */
	else if (tpm_pcr_info_long != NULL) {	/* TPM_KEY12 */
	    if (rc == 0) {
		rc = TPM_Locality_Set(&(tpm_key->tpm_pcr_info_long->localityAtCreation),
					 tpm_state->tpm_stany_flags.localityModifier);
	    }
	    if (rc == 0) {
		rc = TPM_PCRInfoLong_SetDigestAtCreation(tpm_key->tpm_pcr_info_long, tpm_pcrs);
	    }
	}
    }
    /* set TPM_SIZED_BUFFER pubKey */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Set(&(tpm_key->pubKey),
				 keyLength,	/* in bytes */
				 publicKey);
    }
    if (rc == 0) {
	if (tpm_store_asymkey == NULL) {
	    printf("TPM_Key_Set: Error (fatal), No TPM_STORE_ASYMKEY supplied\n");
	    rc = TPM_FAIL;	/* should never occur */
	}
    }
    /* sanity check, currently no need to set TPM_MIGRATE_ASYMKEY */
    if (rc == 0) {
	if (tpm_migrate_asymkey != NULL) {
	    printf("TPM_Key_Set: Error (fatal), TPM_MIGRATE_ASYMKEY supplied\n");
	    rc = TPM_FAIL;	/* should never occur */
	}
    }
    if (rc == 0) {
	/* root key, no parent, just serialize the TPM_STORE_ASYMKEY structure */
	if (parent_key == NULL) {
	    if (rc == 0) {
		rc = TPM_StoreAsymkey_Store(&sbuffer, FALSE, tpm_store_asymkey); /* freed @1 */
	    }
	    if (rc == 0) {
		rc = TPM_SizedBuffer_SetFromStore(&(tpm_key->encData), &sbuffer);
	    }
	}
    }
    if (rc == 0) {
	tpm_key->tpm_store_asymkey = tpm_store_asymkey;		/* cache TPM_STORE_ASYMKEY */
	tpm_key->tpm_migrate_asymkey = tpm_migrate_asymkey;	/* cache TPM_MIGRATE_ASYMKEY */
    }
    /* Generate the TPM_STORE_ASYMKEY -> pubDataDigest.	 Serializes pcrInfo as a side effect. */
    if (rc == 0) {
	rc = TPM_Key_GeneratePubDataDigest(tpm_key);
    }
    TPM_Sbuffer_Delete(&sbuffer);	/* @1 */
    return rc;
}

/* TPM_Key_Copy() copies the source TPM_KEY to the destination.

   The destination should be initialized before the call.
*/

TPM_RESULT TPM_Key_Copy(TPM_KEY *tpm_key_dest,
			TPM_KEY *tpm_key_src,
			TPM_BOOL copyEncData)
{
    TPM_RESULT	rc = 0;

    if (rc == 0) {
	TPM_StructVer_Copy(&(tpm_key_dest->ver), &(tpm_key_src->ver));	/* works for TPM_KEY12
									   also */
	tpm_key_dest->keyUsage	= tpm_key_src->keyUsage;
	tpm_key_dest->keyFlags	= tpm_key_src->keyFlags;
	tpm_key_dest->authDataUsage = tpm_key_src->authDataUsage;
	rc = TPM_KeyParms_Copy(&(tpm_key_dest->algorithmParms), &(tpm_key_src->algorithmParms));
    }
    if (rc == 0) {
	rc = TPM_SizedBuffer_Copy(&(tpm_key_dest->pcrInfo), &(tpm_key_src->pcrInfo));
    }
    /* copy TPM_PCR_INFO cache */
    if (rc == 0) {
	if (tpm_key_src->tpm_pcr_info != NULL) {		/* TPM_KEY */
	    rc = TPM_PCRInfo_CreateFromInfo(&(tpm_key_dest->tpm_pcr_info),
					    tpm_key_src->tpm_pcr_info);
	}
	else if (tpm_key_src->tpm_pcr_info_long != NULL) {	/* TPM_KEY12 */
	    rc = TPM_PCRInfoLong_CreateFromInfoLong(&(tpm_key_dest->tpm_pcr_info_long),
						    tpm_key_src->tpm_pcr_info_long);
	}
    }
    /* copy pubKey */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Copy(&(tpm_key_dest->pubKey), &(tpm_key_src->pubKey));
    }
    /* copy encData */
    if (rc == 0) {
	if (copyEncData) {
	    rc = TPM_SizedBuffer_Copy(&(tpm_key_dest->encData), &(tpm_key_src->encData));
	}
    }
    return rc;
}

/* TPM_Key_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes

   The TPM_PCR_INFO or TPM_PCR_INFO_LONG cache is set from the deserialized pcrInfo stream.

   After use, call TPM_Key_Delete() to free memory
*/


TPM_RESULT TPM_Key_Load(TPM_KEY *tpm_key,	/* result */
			unsigned char **stream, /* pointer to next parameter */
			uint32_t *stream_size)	/* stream size left */
{
    TPM_RESULT		rc = 0;
    
    printf(" TPM_Key_Load:\n");
    /* load public data, and create PCR cache */
    if (rc == 0) {
	rc = TPM_Key_LoadPubData(tpm_key, FALSE, stream, stream_size);
    }
    /* load encDataSize and encData */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_key->encData), stream, stream_size);
    }
    return rc;
}

/* TPM_Key_LoadClear() load a serialized key where the TPM_STORE_ASYMKEY structure is serialized in
   clear text.

   The TPM_PCR_INFO or TPM_PCR_INFO_LONG cache is set from the deserialized pcrInfo stream.

   This function is used to load internal keys (e.g. EK, SRK, owner evict keys) or keys saved as
   part of a save state.
*/

TPM_RESULT TPM_Key_LoadClear(TPM_KEY *tpm_key,		/* result */
			     TPM_BOOL isEK,		/* key being loaded is EK */
			     unsigned char **stream,	/* pointer to next parameter */
			     uint32_t *stream_size)	/* stream size left */
{
    TPM_RESULT		rc = 0;
    uint32_t		storeAsymkeySize;
    
    printf(" TPM_Key_LoadClear:\n");
    /* load public data */
    if (rc == 0) {
	rc = TPM_Key_LoadPubData(tpm_key, isEK, stream, stream_size);
    }
    /* load TPM_STORE_ASYMKEY size */
    if (rc == 0) {
	rc = TPM_Load32(&storeAsymkeySize, stream, stream_size);
    }
    /* The size might be 0 for an uninitialized internal key.  That case is not an error. */
    if ((rc == 0) && (storeAsymkeySize > 0)) {
	rc = TPM_Key_LoadStoreAsymKey(tpm_key, isEK, stream, stream_size);
    }			     
    return rc;
}

/* TPM_Key_LoadPubData() deserializes a TPM_KEY or TPM_KEY12 structure, excluding encData, to
   'tpm_key'.

   The TPM_PCR_INFO or TPM_PCR_INFO_LONG cache is set from the deserialized pcrInfo stream.
   If the pcrInfo stream is empty, the caches remain NULL.

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes

   After use, call TPM_Key_Delete() to free memory
*/

TPM_RESULT TPM_Key_LoadPubData(TPM_KEY *tpm_key,	/* result */
			       TPM_BOOL isEK,		/* key being loaded is EK */
			       unsigned char **stream,	/* pointer to next parameter */
			       uint32_t *stream_size)	/* stream size left */
{
    TPM_RESULT		rc = 0;
   
    printf(" TPM_Key_LoadPubData:\n");
    /* peek at the first byte */
    if (rc == 0) {
	/* TPM_KEY[0] is major (non zero) */
	if ((*stream)[0] != 0) {
	    /* load ver */
	    if (rc == 0) {
		rc = TPM_StructVer_Load(&(tpm_key->ver), stream, stream_size);
	    }
	    /* check ver immediately to ease debugging */
	    if (rc == 0) {
		rc = TPM_StructVer_CheckVer(&(tpm_key->ver));
	    }
	}
	else {
	    /* TPM_KEY12 is tag (zero) */
	    /* load tag */
	    if (rc == 0) {
		rc = TPM_Load16(&(((TPM_KEY12 *)tpm_key)->tag), stream, stream_size);
	    }
	    /* load fill */
	    if (rc == 0) {
		rc = TPM_Load16(&(((TPM_KEY12 *)tpm_key)->fill), stream, stream_size);
	    }
	    if (rc == 0) {
		rc = TPM_Key_CheckTag((TPM_KEY12 *)tpm_key);
	    }
	}
    }
    /* load keyUsage */
    if (rc == 0) {
	rc = TPM_Load16(&(tpm_key->keyUsage), stream, stream_size);
    }
    /* load keyFlags */
    if (rc == 0) {
	rc = TPM_KeyFlags_Load(&(tpm_key->keyFlags), stream, stream_size);
    }
    /* load authDataUsage */
    if (rc == 0) {
	rc = TPM_Load8(&(tpm_key->authDataUsage), stream, stream_size);
    }
    /* load algorithmParms */
    if (rc == 0) {
	rc = TPM_KeyParms_Load(&(tpm_key->algorithmParms), stream, stream_size);
    }
    /* load PCRInfo */
    if ((rc == 0) && !isEK) {
	rc = TPM_SizedBuffer_Load(&(tpm_key->pcrInfo), stream, stream_size);
    }
    /* set TPM_PCR_INFO tpm_pcr_info cache from PCRInfo stream.	 If the stream is empty, a NULL is
       returned.
    */
    if ((rc == 0) && !isEK) {
	if (((TPM_KEY12 *)tpm_key)->tag != TPM_TAG_KEY12) {	/* TPM_KEY */
	    rc = TPM_PCRInfo_CreateFromBuffer(&(tpm_key->tpm_pcr_info),
					      &(tpm_key->pcrInfo));
	}
	else {							/* TPM_KEY12 */
	    rc = TPM_PCRInfoLong_CreateFromBuffer(&(tpm_key->tpm_pcr_info_long),
						  &(tpm_key->pcrInfo));
	}
    }
    /* load pubKey */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_key->pubKey), stream, stream_size);
    }
    return rc;
}

/* TPM_Key_StorePubData() serializes a TPM_KEY or TPM_KEY12 structure, excluding encData, appending
   results to 'sbuffer'.

   As a side effect, it serializes the tpm_pcr_info cache to pcrInfo.
*/

TPM_RESULT TPM_Key_StorePubData(TPM_STORE_BUFFER *sbuffer,
				TPM_BOOL isEK,
				TPM_KEY *tpm_key)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_Key_StorePubData:\n");
    
    if (rc == 0) {
	/* store ver */
	if (((TPM_KEY12 *)tpm_key)->tag != TPM_TAG_KEY12) {	/* TPM_KEY */
	    rc = TPM_StructVer_Store(sbuffer, &(tpm_key->ver));
	}
	else {							/* TPM_KEY12 */
	    /* store tag */
	    if (rc == 0) {
		rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_KEY12);
	    }
	    /* store fill */
	    if (rc == 0) {
		rc = TPM_Sbuffer_Append16(sbuffer, 0x0000);
	    }
	}
    }
    /* store keyUsage */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, tpm_key->keyUsage); 
    }
    /* store keyFlags */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_key->keyFlags); 
    }
    /* store authDataUsage */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_key->authDataUsage), sizeof(TPM_AUTH_DATA_USAGE)); 
    }
    /* store algorithmParms */
    if (rc == 0) {
	rc = TPM_KeyParms_Store(sbuffer, &(tpm_key->algorithmParms)); 
    }
    /* store pcrInfo */
    if ((rc == 0) && !isEK) {
	/* copy cache to pcrInfo */
	if (((TPM_KEY12 *)tpm_key)->tag != TPM_TAG_KEY12) {	/* TPM_KEY */
	    rc = TPM_SizedBuffer_SetStructure(&(tpm_key->pcrInfo),
					      tpm_key->tpm_pcr_info,
					      (TPM_STORE_FUNCTION_T)TPM_PCRInfo_Store);
	}
	else {							/* TPM_KEY12 */
	    rc = TPM_SizedBuffer_SetStructure(&(tpm_key->pcrInfo),
					      tpm_key->tpm_pcr_info_long,
					      (TPM_STORE_FUNCTION_T)TPM_PCRInfoLong_Store);
	}
    }
    /* copy pcrInfo to sbuffer */
    if ((rc == 0) && !isEK) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_key->pcrInfo));
    }
    /* store pubKey */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_key->pubKey)); 
    }
    return rc;
}

/* TPM_Key_Store() serializes a TPM_KEY structure, appending results to 'sbuffer'

   As a side effect, it serializes the tpm_pcr_info cache to pcrInfo.
*/

TPM_RESULT TPM_Key_Store(TPM_STORE_BUFFER *sbuffer,
			 TPM_KEY *tpm_key)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_Key_Store:\n");
    /* store the pubData */
    if (rc == 0) {
	rc = TPM_Key_StorePubData(sbuffer, FALSE, tpm_key); 
    }
    /* store encDataSize and encData */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_key->encData)); 
    }
    return rc;
}

/* TPM_Key_StoreClear() serializes a TPM_KEY structure, appending results to 'sbuffer'

   TPM_Key_StoreClear() serializes the tpm_store_asymkey member as cleartext.  It is used for keys
   such as the SRK, which never leave the TPM.	It is also used for saving state, where the entire
   blob is encrypted.

   As a side effect, it serializes the tpm_pcr_info cache to pcrInfo.
*/

TPM_RESULT TPM_Key_StoreClear(TPM_STORE_BUFFER *sbuffer,
			      TPM_BOOL isEK,		/* key being stored is EK */
			      TPM_KEY *tpm_key)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	asymSbuffer;
    const unsigned char *asymBuffer;
    uint32_t		asymLength;
    
    printf(" TPM_Key_StoreClear:\n");
    TPM_Sbuffer_Init(&asymSbuffer);			/* freed @1 */
    /* store the pubData */
    if (rc == 0) {
	rc = TPM_Key_StorePubData(sbuffer, isEK, tpm_key); 
    }
    /* store TPM_STORE_ASYMKEY cache as cleartext */
    if (rc == 0) {
	/* if the TPM_STORE_ASYMKEY cache exists */
	if (tpm_key->tpm_store_asymkey != NULL) {
	    /* , serialize it */
	    if (rc == 0) {
		rc = TPM_StoreAsymkey_Store(&asymSbuffer, isEK, tpm_key->tpm_store_asymkey);
	    }
	    /* get the result */
	    TPM_Sbuffer_Get(&asymSbuffer, &asymBuffer, &asymLength);
	    /* store the result as a sized buffer */
	    if (rc == 0) {
		rc = TPM_Sbuffer_Append32(sbuffer, asymLength);
	    }
	    if (rc == 0) {
		rc = TPM_Sbuffer_Append(sbuffer, asymBuffer, asymLength);
	    }
	}
	/* If there is no TPM_STORE_ASYMKEY cache, mark it empty.  This can occur for an internal
	   key that has not been created yet.  */
	else {
	    rc = TPM_Sbuffer_Append32(sbuffer, 0);
	}
    }
    TPM_Sbuffer_Delete(&asymSbuffer);			/* @1 */
    return rc;
}

/* TPM_KEY_StorePubkey() gets (as a stream) the TPM_PUBKEY derived from a TPM_KEY

   There is no need to actually assemble the structure, since only the serialization of its two
   members are needed.
   
   The stream is returned as a TPM_STORE_BUFFER (that must be initialized and deleted by the
   caller), and it's components (buffer and size).
*/

TPM_RESULT TPM_Key_StorePubkey(TPM_STORE_BUFFER *pubkeyStream,			/* output */
			       const unsigned char **pubkeyStreamBuffer,	/* output */
			       uint32_t *pubkeyStreamLength,			/* output */
			       TPM_KEY *tpm_key)				/* input */
{
    TPM_RESULT	rc = 0;

    printf(" TPM_Key_StorePubkey:\n");
    /* the first part is a TPM_KEY_PARMS */
    if (rc == 0) {
	rc = TPM_KeyParms_Store(pubkeyStream, &(tpm_key->algorithmParms));
    }
    /* the second part is the TPM_SIZED_BUFFER pubKey */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(pubkeyStream, &(tpm_key->pubKey));
    }
    /* retrieve the resulting pubkey stream */
    if (rc == 0) {
	TPM_Sbuffer_Get(pubkeyStream, 
			pubkeyStreamBuffer,
			pubkeyStreamLength);
    }
    return rc;
}

/* TPM_Key_Delete() 

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_Key_Init to set members back to default values
   The TPM_KEY itself is not freed

   The key is not freed because it might be a local variable rather than a malloc'ed pointer.
*/   

void TPM_Key_Delete(TPM_KEY *tpm_key)
{
    if (tpm_key != NULL) {
	printf(" TPM_Key_Delete:\n");
	TPM_KeyParms_Delete(&(tpm_key->algorithmParms));
	/* pcrInfo */
	TPM_SizedBuffer_Delete(&(tpm_key->pcrInfo));
	/* pcr caches */
	TPM_PCRInfo_Delete(tpm_key->tpm_pcr_info);
	free(tpm_key->tpm_pcr_info);
	TPM_PCRInfoLong_Delete(tpm_key->tpm_pcr_info_long);
	free(tpm_key->tpm_pcr_info_long);

	TPM_SizedBuffer_Delete(&(tpm_key->pubKey));
	TPM_SizedBuffer_Delete(&(tpm_key->encData));
	TPM_StoreAsymkey_Delete(tpm_key->tpm_store_asymkey);
	free(tpm_key->tpm_store_asymkey);
	TPM_MigrateAsymkey_Delete(tpm_key->tpm_migrate_asymkey);
	free(tpm_key->tpm_migrate_asymkey);
	TPM_Key_Init(tpm_key);
    }
    return;
}

/* TPM_Key_CheckStruct() verifies that the 'tpm_key' has either a TPM_KEY -> ver of a TPM_KEY12 tag
   and fill
*/

TPM_RESULT TPM_Key_CheckStruct(int *ver, TPM_KEY *tpm_key)
{
    TPM_RESULT	rc = 0;

    /* The key can be either a TPM_KEY or TPM_KEY12 */
    if (*(unsigned char *)tpm_key == 0x01) {
	*ver = 1;
	rc = TPM_StructVer_CheckVer(&(tpm_key->ver));	/* check for TPM_KEY */
	if (rc == 0) {					/* if found TPM_KEY */
	    printf(" TPM_Key_CheckStruct: TPM_KEY version %u.%u\n",
		   tpm_key->ver.major, tpm_key->ver.minor);
	}
    }
    else {						/* else check for TPM_KEY12 */
	*ver = 2;
	rc = TPM_Key_CheckTag((TPM_KEY12 *)tpm_key);
	if (rc == 0) {
	    printf(" TPM_Key_CheckStruct: TPM_KEY12\n");
	}
	else {	/* not TPM_KEY or TPM_KEY12 */
	    printf("TPM_Key_CheckStruct: Error checking structure, bytes 0:3 %02x %02x %02x %02x\n",
		   tpm_key->ver.major, tpm_key->ver.minor,
		   tpm_key->ver.revMajor, tpm_key->ver.revMinor);
	    rc = TPM_BAD_KEY_PROPERTY;
	}
    }
    return rc;
}

/* TPM_Key_CheckTag() checks that the TPM_KEY12 tag is correct
 */

static TPM_RESULT TPM_Key_CheckTag(TPM_KEY12 *tpm_key12)
{
    TPM_RESULT	rc = 0;

    if (rc == 0) {
	if (tpm_key12->tag != TPM_TAG_KEY12) {
	    printf("TPM_Key_CheckTag: Error, TPM_KEY12 tag %04x should be TPM_TAG_KEY12\n",
		   tpm_key12->tag);
	    rc = TPM_BAD_KEY_PROPERTY;
	}
    }
    if (rc == 0) {
	if (tpm_key12->fill != 0x0000) {
	    printf("TPM_Key_CheckTag: Error, TPM_KEY12 fill %04x should be 0x0000\n",
		   tpm_key12->fill);
	    rc = TPM_BAD_KEY_PROPERTY;
	}
    }
    return rc;
}

/* TPM_Key_CheckProperties() checks that the TPM can generate a key of the type requested in
   'tpm_key'.

   if keyLength is non-zero, checks that the tpm_key specifies the correct key length.  If keyLength
   is 0, any tpm_key key length is accepted.

   Returns TPM_BAD_KEY_PROPERTY on error.
 */

TPM_RESULT TPM_Key_CheckProperties(int *ver,
				   TPM_KEY *tpm_key,
				   uint32_t keyLength,	/* in bits */
				   TPM_BOOL FIPS)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_Key_CheckProperties:\n");
    /* check the version */
    if (rc == 0) {
	rc = TPM_Key_CheckStruct(ver, tpm_key);
    }
    /* if FIPS */
    if ((rc == 0) && FIPS) {
	/* b.  If keyInfo -> authDataUsage specifies TPM_AUTH_NEVER return TPM_NOTFIPS */
	if (tpm_key->authDataUsage == TPM_AUTH_NEVER) {
	    printf("TPM_Key_CheckProperties: Error, FIPS authDataUsage TPM_AUTH_NEVER\n");
	    rc = TPM_NOTFIPS;
	}
    }
    /* most of the work is done by TPM_KeyParms_CheckProperties() */
    if (rc == 0) {
	printf("  TPM_Key_CheckProperties: authDataUsage %02x\n", tpm_key->authDataUsage);
	rc = TPM_KeyParms_CheckProperties(&(tpm_key->algorithmParms),
					  tpm_key->keyUsage,
					  keyLength,	/* in bits */
					  FIPS);
    }
    return rc;
}

/* TPM_Key_LoadStoreAsymKey() deserializes a stream to a TPM_STORE_ASYMKEY structure and stores it
   in the TPM_KEY cache.

   Call this function when a key is loaded, either from the host (stream is decrypted encData) or
   from permanent data or saved state (stream was clear text).
*/

TPM_RESULT TPM_Key_LoadStoreAsymKey(TPM_KEY *tpm_key,
				    TPM_BOOL isEK,
				    unsigned char **stream,	/* decrypted encData (clear text) */
				    uint32_t *stream_size)
{
    TPM_RESULT	rc = 0;
    
    /* This function should never be called when the TPM_STORE_ASYMKEY structure has already been
       loaded.	This indicates an internal error. */
    printf(" TPM_Key_LoadStoreAsymKey:\n");
    if (rc == 0) {
	if (tpm_key->tpm_store_asymkey != NULL) {
	    printf("TPM_Key_LoadStoreAsymKey: Error (fatal), TPM_STORE_ASYMKEY already loaded\n");
	    rc = TPM_FAIL;	/* should never occur */
	}
    }
    /* If the stream size is 0, there is an internal error. */
    if (rc == 0) {
	if (*stream_size == 0) {
	    printf("TPM_Key_LoadStoreAsymKey: Error (fatal), stream size is 0\n");
	    rc = TPM_FAIL;	/* should never occur */
	}
    }
    /* allocate memory for the structure */
    if (rc == 0) {
	rc = TPM_Malloc((unsigned char **)&(tpm_key->tpm_store_asymkey),
			sizeof(TPM_STORE_ASYMKEY));
    }
    if (rc == 0) {
	TPM_StoreAsymkey_Init(tpm_key->tpm_store_asymkey);
	rc = TPM_StoreAsymkey_Load(tpm_key->tpm_store_asymkey, isEK,
				   stream, stream_size,
				   &(tpm_key->algorithmParms), &(tpm_key->pubKey));
	TPM_PrintFour("  TPM_Key_LoadStoreAsymKey: usageAuth",
		      tpm_key->tpm_store_asymkey->usageAuth);
    }
    return rc;
}

/* TPM_Key_GetStoreAsymkey() gets the TPM_STORE_ASYMKEY from a TPM_KEY cache.
 */

TPM_RESULT TPM_Key_GetStoreAsymkey(TPM_STORE_ASYMKEY **tpm_store_asymkey,
				   TPM_KEY *tpm_key)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_Key_GetStoreAsymkey:\n");
    if (rc == 0) {
	/* return the cached structure */
	*tpm_store_asymkey = tpm_key->tpm_store_asymkey;
	if (tpm_key->tpm_store_asymkey == NULL) {
	    printf("TPM_Key_GetStoreAsymkey: Error (fatal), no cache\n");
	    rc = TPM_FAIL;	/* indicate no cache */
	}
    }
    return rc; 
}

/* TPM_Key_GetMigrateAsymkey() gets the TPM_MIGRATE_ASYMKEY from a TPM_KEY cache.
 */

TPM_RESULT TPM_Key_GetMigrateAsymkey(TPM_MIGRATE_ASYMKEY **tpm_migrate_asymkey,
				     TPM_KEY *tpm_key)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_Key_GetMigrateAsymkey:\n");
    if (rc == 0) {
	/* return the cached structure */
	*tpm_migrate_asymkey = tpm_key->tpm_migrate_asymkey;
	if (tpm_key->tpm_migrate_asymkey == NULL) {
	    printf("TPM_Key_GetMigrateAsymkey: Error (fatal), no cache\n");
	    rc = TPM_FAIL;	/* indicate no cache */
	}
    }
    return rc; 
}

/* TPM_Key_GetUsageAuth() gets the usageAuth from the TPM_STORE_ASYMKEY or TPM_MIGRATE_ASYMKEY
   contained in a TPM_KEY
*/

TPM_RESULT TPM_Key_GetUsageAuth(TPM_SECRET **usageAuth,
				TPM_KEY *tpm_key)
{
    TPM_RESULT	rc = 0;
    TPM_STORE_ASYMKEY *tpm_store_asymkey;
    TPM_MIGRATE_ASYMKEY *tpm_migrate_asymkey;
    
    printf(" TPM_Key_GetUsageAuth:\n");
    /* check that the TPM_KEY_USAGE indicates a valid key */ 
    if (rc == 0) {
	if ((tpm_key == NULL) ||
	    (tpm_key->keyUsage == TPM_KEY_UNINITIALIZED)) {
	    printf("TPM_Key_GetUsageAuth: Error, key not initialized\n");
	    rc = TPM_INVALID_KEYUSAGE;
	}
    }
    /* get the TPM_STORE_ASYMKEY object */
    if (rc == 0) {
	rc = TPM_Key_GetStoreAsymkey(&tpm_store_asymkey, tpm_key);

	/* found a TPM_STORE_ASYMKEY */
	if (rc == 0) {
	    *usageAuth = &(tpm_store_asymkey->usageAuth);
	}
	/* get the TPM_MIGRATE_ASYMKEY object */
	else {
	    rc = TPM_Key_GetMigrateAsymkey(&tpm_migrate_asymkey, tpm_key);
	    /* found a TPM_MIGRATE_ASYMKEY */
	    if (rc == 0) {
		*usageAuth = &(tpm_migrate_asymkey->usageAuth);
	    }
	}
    }
    if (rc != 0) {
	printf("TPM_Key_GetUsageAuth: Error (fatal), "
	       "could not get TPM_STORE_ASYMKEY or TPM_MIGRATE_ASYMKEY\n");
	rc = TPM_FAIL;	/* should never occur */
    }
    /* get the usageAuth element */
    if (rc == 0) {
	TPM_PrintFour("  TPM_Key_GetUsageAuth: Auth", **usageAuth);
    }
    return rc;
}

/* TPM_Key_GetPublicKey() gets the public key from the TPM_STORE_PUBKEY contained in a TPM_KEY
 */

TPM_RESULT TPM_Key_GetPublicKey(uint32_t	*nbytes,
				unsigned char	**narr,
				TPM_KEY		*tpm_key)
{
    TPM_RESULT	rc = 0;
    
    printf(" TPM_Key_GetPublicKey:\n");
    if (rc == 0) {
	*nbytes = tpm_key->pubKey.size;
	*narr = tpm_key->pubKey.buffer;
    }
    return rc;
}

/* TPM_Key_GetPrimeFactorP() gets the prime factor p from the TPM_STORE_ASYMKEY contained in a
   TPM_KEY
*/

TPM_RESULT TPM_Key_GetPrimeFactorP(uint32_t 		*pbytes,
				   unsigned char	**parr,
				   TPM_KEY		*tpm_key)
{
    TPM_RESULT	rc = 0;
    TPM_STORE_ASYMKEY	*tpm_store_asymkey;
    
    printf(" TPM_Key_GetPrimeFactorP:\n");
    if (rc == 0) {
	rc = TPM_Key_GetStoreAsymkey(&tpm_store_asymkey, tpm_key);
    }
    if (rc == 0) {
	*pbytes = tpm_store_asymkey->privKey.p_key.size;
	*parr = tpm_store_asymkey->privKey.p_key.buffer;
    }
    return rc;
}

/* TPM_Key_GetPrivateKey() gets the private key from the TPM_STORE_ASYMKEY contained in a TPM_KEY
 */

TPM_RESULT TPM_Key_GetPrivateKey(uint32_t	*dbytes,
				 unsigned char	**darr,
				 TPM_KEY	*tpm_key)
{
    TPM_RESULT	rc = 0;
    TPM_STORE_ASYMKEY	*tpm_store_asymkey;
    
    printf(" TPM_Key_GetPrivateKey:\n");
    if (rc == 0) {
	rc = TPM_Key_GetStoreAsymkey(&tpm_store_asymkey, tpm_key);
    }
    if (rc == 0) {
	*dbytes = tpm_store_asymkey->privKey.d_key.size;
	*darr = tpm_store_asymkey->privKey.d_key.buffer;
    }
    return rc;
}

/* TPM_Key_GetExponent() gets the exponent key from the TPM_RSA_KEY_PARMS contained in a TPM_KEY
 */

TPM_RESULT TPM_Key_GetExponent(uint32_t		*ebytes,
			       unsigned char	**earr,
			       TPM_KEY		*tpm_key)
{
    TPM_RESULT		rc = 0;
    
    printf(" TPM_Key_GetExponent:\n");
    if (rc == 0) {
	rc = TPM_KeyParms_GetExponent(ebytes, earr, &(tpm_key->algorithmParms));
    }
    return rc;
}

/* TPM_Key_GetPCRUsage() returns 'pcrUsage' TRUE if any bit is set in the pcrSelect bit mask.

   'start_pcr' indicates the starting byte index into pcrSelect[]
*/

TPM_RESULT TPM_Key_GetPCRUsage(TPM_BOOL *pcrUsage,
			       TPM_KEY *tpm_key,
			       size_t start_index)
{
    TPM_RESULT	rc = 0;
    
    printf(" TPM_Key_GetPCRUsage: Start %lu\n", (unsigned long)start_index);
    if (((TPM_KEY12 *)tpm_key)->tag != TPM_TAG_KEY12) { /* TPM_KEY */
	rc = TPM_PCRInfo_GetPCRUsage(pcrUsage, tpm_key->tpm_pcr_info, start_index);
    }
    else {						/* TPM_KEY12 */
	rc = TPM_PCRInfoLong_GetPCRUsage(pcrUsage, tpm_key->tpm_pcr_info_long, start_index);
    }
    return rc;
}

/* TPM_Key_GetLocalityAtRelease() the localityAtRelease for a TPM_PCR_INFO_LONG.
   For a TPM_PCR_INFO is returns TPM_LOC_ALL (all localities).
*/

TPM_RESULT TPM_Key_GetLocalityAtRelease(TPM_LOCALITY_SELECTION *localityAtRelease,
					TPM_KEY *tpm_key)
{
    TPM_RESULT	rc = 0;
    
    printf(" TPM_Key_GetLocalityAtRelease:\n");
    if (((TPM_KEY12 *)tpm_key)->tag != TPM_TAG_KEY12) { /* TPM_KEY */
	/* locality not used for TPM_PCR_INFO */
	*localityAtRelease = TPM_LOC_ALL;
    }
    /* TPM_KEY12 */
    else if (tpm_key->tpm_pcr_info_long == NULL) {
	/* locality not used if TPM_PCR_INFO_LONG was not specified */
	*localityAtRelease = TPM_LOC_ALL;
    }
    else {
	*localityAtRelease = tpm_key->tpm_pcr_info_long->localityAtRelease;
    }
    return rc;
}

/* TPM_Key_GenerateRSA() generates a TPM_KEY using TPM_KEY_PARMS.  The tag/version is set correctly.

   The TPM_STORE_ASYMKEY member cache is set.  pcrInfo is set as a serialized tpm_pcr_info or
   tpm_pcr_info_long.

   For exported keys, encData is not set yet.  It later becomes the encryption of TPM_STORE_ASYMKEY.

   For internal 'root' keys (endorsement key, srk), encData is stored as clear text.

   It returns the TPM_KEY object.

   Call tree:
	local - sets tpm_store_asymkey->privkey
	TPM_Key_Set - sets keyUsage, keyFlags, authDataUsage, algorithmParms
			tpm_pcr_info cache, digestAtCreation, pubKey,
	    TPM_Key_GeneratePubDataDigest - pubDataDigest
		TPM_Key_Store
		    TPM_Key_StorePubData - serializes tpm_pcr_info cache
*/

TPM_RESULT TPM_Key_GenerateRSA(TPM_KEY *tpm_key,		/* output created key */
			       tpm_state_t *tpm_state,
			       TPM_KEY *parent_key,		/* NULL for root keys */
			       TPM_DIGEST *tpm_pcrs,		/* PCR array from state */
			       int ver,				/* TPM_KEY or TPM_KEY12 */
			       TPM_KEY_USAGE keyUsage,			/* input */
			       TPM_KEY_FLAGS keyFlags,			/* input */
			       TPM_AUTH_DATA_USAGE authDataUsage,	/* input */
			       TPM_KEY_PARMS *tpm_key_parms,		/* input */
			       TPM_PCR_INFO *tpm_pcr_info,		/* input */
			       TPM_PCR_INFO_LONG *tpm_pcr_info_long)	/* input */
{
    TPM_RESULT		rc = 0;
    TPM_RSA_KEY_PARMS	*tpm_rsa_key_parms;
    unsigned char	*earr;		/* public exponent */
    uint32_t		ebytes;

	/* generated RSA key */
    unsigned char	*n = NULL;	/* public key */
    unsigned char	*p = NULL;	/* prime factor */
    unsigned char	*q = NULL;	/* prime factor */
    unsigned char	*d = NULL;	/* private key */
    
    printf(" TPM_Key_GenerateRSA:\n");
    /* extract the TPM_RSA_KEY_PARMS from TPM_KEY_PARMS */
    if (rc == 0) {
	rc = TPM_KeyParms_GetRSAKeyParms(&tpm_rsa_key_parms, tpm_key_parms);
    }
    /* get the public exponent, with conversion */
    if (rc == 0) {
	rc = TPM_RSAKeyParms_GetExponent(&ebytes, &earr, tpm_rsa_key_parms);
    }
    /* allocate storage for TPM_STORE_ASYMKEY.	The structure is not freed.  It is cached in the
       TPM_KEY->TPM_STORE_ASYMKEY member and freed when they are deleted. */
    if (rc == 0) {
	rc = TPM_Malloc((unsigned char **)&(tpm_key->tpm_store_asymkey),
			sizeof(TPM_STORE_ASYMKEY));
    }
    if (rc == 0) {
	TPM_StoreAsymkey_Init(tpm_key->tpm_store_asymkey);
    }
    /* generate the key pair */
    if (rc == 0) {
	rc = TPM_RSAGenerateKeyPair(&n,		/* public key (modulus) freed @3 */
				    &p,		/* private prime factor freed @4 */
				    &q,		/* private prime factor freed @5 */
				    &d,		/* private key (private exponent) freed @6 */
				    tpm_rsa_key_parms->keyLength,	/* key size in bits */
				    earr,	/* public exponent */
				    ebytes);
    }
    /* construct the TPM_STORE_ASYMKEY member */
    if (rc == 0) {
	TPM_PrintFour(" TPM_Key_GenerateRSA: Public key n", n);
	TPM_PrintAll(" TPM_Key_GenerateRSA: Exponent", earr, ebytes);
	TPM_PrintFour(" TPM_Key_GenerateRSA: Private prime p", p);
	TPM_PrintFour(" TPM_Key_GenerateRSA: Private prime q", q);
	TPM_PrintFour(" TPM_Key_GenerateRSA: Private key d", d);
	/* add the private primes and key to the TPM_STORE_ASYMKEY object */
	rc = TPM_SizedBuffer_Set(&(tpm_key->tpm_store_asymkey->privKey.d_key),
				 tpm_rsa_key_parms->keyLength/CHAR_BIT,
				 d);
    }
    if (rc == 0) {
	rc = TPM_SizedBuffer_Set(&(tpm_key->tpm_store_asymkey->privKey.p_key),
				 tpm_rsa_key_parms->keyLength/(CHAR_BIT * 2),
				 p);
    }
    if (rc == 0) {
	rc = TPM_SizedBuffer_Set(&(tpm_key->tpm_store_asymkey->privKey.q_key),
				 tpm_rsa_key_parms->keyLength/(CHAR_BIT * 2),
				 q);
    }
    if (rc == 0) {
	rc = TPM_Key_Set(tpm_key,
			 tpm_state,
			 parent_key,
			 tpm_pcrs,
			 ver,					/* TPM_KEY or TPM_KEY12 */
			 keyUsage,				/* TPM_KEY_USAGE */
			 keyFlags,				/* TPM_KEY_FLAGS */
			 authDataUsage,				/* TPM_AUTH_DATA_USAGE */
			 tpm_key_parms,				/* TPM_KEY_PARMS */
			 tpm_pcr_info,				/* TPM_PCR_INFO */
			 tpm_pcr_info_long,			/* TPM_PCR_INFO_LONG */
			 tpm_rsa_key_parms->keyLength/CHAR_BIT, /* TPM_STORE_PUBKEY.keyLength */
			 n,				/* TPM_STORE_PUBKEY.key (public key) */
			 /* FIXME redundant */
			 tpm_key->tpm_store_asymkey,	/* cache the TPM_STORE_ASYMKEY structure */
			 NULL);				/* TPM_MIGRATE_ASYMKEY */
    }
    free(n);					/* @3 */
    free(p);					/* @4 */
    free(q);					/* @5 */
    free(d);					/* @6 */
    return rc;
}

/* TPM_Key_GeneratePubkeyDigest() serializes a TPM_PUBKEY derived from the TPM_KEY and calculates
   its digest.
*/

TPM_RESULT TPM_Key_GeneratePubkeyDigest(TPM_DIGEST tpm_digest,
					TPM_KEY *tpm_key)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	pubkeyStream;		/* from tpm_key */
    const unsigned char *pubkeyStreamBuffer;	
    uint32_t		pubkeyStreamLength;

    printf(" TPM_Key_GeneratePubkeyDigest:\n");
    TPM_Sbuffer_Init(&pubkeyStream);		/* freed @1 */
    /* serialize a TPM_PUBKEY derived from the TPM_KEY */
    if (rc == 0) {
	rc = TPM_Key_StorePubkey(&pubkeyStream,		/* output */
				 &pubkeyStreamBuffer,	/* output */
				 &pubkeyStreamLength,	/* output */
				 tpm_key);		/* input */
    }
    if (rc == 0) {
	rc = TPM_SHA1(tpm_digest,
		      pubkeyStreamLength, pubkeyStreamBuffer,
		      0, NULL);
    }	
    TPM_Sbuffer_Delete(&pubkeyStream);		/* @1 */
    return rc;

}

/* TPM_Key_ComparePubkey() serializes and hashes the TPM_PUBKEY derived from a TPM_KEY and a
   TPM_PUBKEY and compares the results
*/

TPM_RESULT TPM_Key_ComparePubkey(TPM_KEY *tpm_key,
				 TPM_PUBKEY *tpm_pubkey)
{
    TPM_RESULT		rc = 0;
    TPM_DIGEST		key_digest;
    TPM_DIGEST		pubkey_digest;
    
    if (rc == 0) {
	rc = TPM_Key_GeneratePubkeyDigest(key_digest, tpm_key);
    }
    if (rc == 0) {
	rc = TPM_SHA1_GenerateStructure(pubkey_digest, tpm_pubkey,
					(TPM_STORE_FUNCTION_T)TPM_Pubkey_Store);
    }
    if (rc == 0) {
	rc = TPM_Digest_Compare(key_digest, pubkey_digest);
    }	 
    return rc;
}

/* TPM_Key_GeneratePubDataDigest() generates and stores a TPM_STORE_ASYMKEY -> pubDataDigest

   As a side effect, it serializes the tpm_pcr_info cache to pcrInfo.
*/

TPM_RESULT TPM_Key_GeneratePubDataDigest(TPM_KEY *tpm_key)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	sbuffer;	/* TPM_KEY serialization */
    TPM_STORE_ASYMKEY	*tpm_store_asymkey;
    
    printf(" TPM_Key_GeneratePubDataDigest:\n");
    TPM_Sbuffer_Init(&sbuffer);			/* freed @1 */
    /* serialize the TPM_KEY excluding the encData fields */
    if (rc == 0) {
	rc = TPM_Key_StorePubData(&sbuffer, FALSE, tpm_key);
    }
    /* get the TPM_STORE_ASYMKEY structure */
    if (rc == 0) {
	rc = TPM_Key_GetStoreAsymkey(&tpm_store_asymkey, tpm_key);
    }
    /* hash the serialized buffer to tpm_digest */
    if (rc == 0) {
	rc = TPM_SHA1Sbuffer(tpm_store_asymkey->pubDataDigest, &sbuffer);
    }
    TPM_Sbuffer_Delete(&sbuffer);	/* @1 */
    return rc;
}

/* TPM_Key_CheckPubDataDigest() generates a TPM_STORE_ASYMKEY -> pubDataDigest and compares it to
   the stored value.

   Returns:  Error id
 */

TPM_RESULT TPM_Key_CheckPubDataDigest(TPM_KEY *tpm_key)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	sbuffer;	/* TPM_KEY serialization */
    TPM_STORE_ASYMKEY	*tpm_store_asymkey;
    TPM_DIGEST		tpm_digest;	/* calculated pubDataDigest */
    
    printf(" TPM_Key_CheckPubDataDigest:\n");
    TPM_Sbuffer_Init(&sbuffer);			/* freed @1 */
    /* serialize the TPM_KEY excluding the encData fields */
    if (rc == 0) {
	rc = TPM_Key_StorePubData(&sbuffer, FALSE, tpm_key);
    }
    /* get the TPM_STORE_ASYMKEY structure */
    if (rc == 0) {
	rc = TPM_Key_GetStoreAsymkey(&tpm_store_asymkey, tpm_key);
    }
    if (rc == 0) {
	rc = TPM_SHA1Sbuffer(tpm_digest, &sbuffer);
    }
    if (rc == 0) {
	rc = TPM_Digest_Compare(tpm_store_asymkey->pubDataDigest, tpm_digest);
    }
    TPM_Sbuffer_Delete(&sbuffer);	/* @1 */
    return rc;
}

/* TPM_Key_GenerateEncData() generates an TPM_KEY -> encData structure member by serializing the
   cached TPM_KEY -> TPM_STORE_ASYMKEY member and encrypting the result using the parent_key public
   key.
*/

TPM_RESULT TPM_Key_GenerateEncData(TPM_KEY *tpm_key,
				   TPM_KEY *parent_key)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_ASYMKEY	*tpm_store_asymkey;

    printf(" TPM_Key_GenerateEncData;\n");
    /* get the TPM_STORE_ASYMKEY structure */
    if (rc == 0) {
	rc = TPM_Key_GetStoreAsymkey(&tpm_store_asymkey, tpm_key);
    }
    if (rc == 0) {
	rc = TPM_StoreAsymkey_GenerateEncData(&(tpm_key->encData),
					      tpm_store_asymkey,
					      parent_key);
    }
    return rc;
}


/* TPM_Key_DecryptEncData() decrypts the TPM_KEY -> encData using the parent private key.  The
   result is deserialized and stored in the TPM_KEY -> TPM_STORE_ASYMKEY cache.

*/

TPM_RESULT TPM_Key_DecryptEncData(TPM_KEY *tpm_key,	/* result */
				  TPM_KEY *parent_key)	/* parent for decrypting encData */
{
    TPM_RESULT		rc = 0;
    unsigned char	*decryptData = NULL;	/* freed @1 */
    uint32_t		decryptDataLength = 0;	/* actual valid data */
    unsigned char	*stream;
    uint32_t		stream_size;

    printf(" TPM_Key_DecryptEncData\n");
    /* allocate space for the decrypted data */
    if (rc == 0) {
	rc = TPM_RSAPrivateDecryptMalloc(&decryptData,			/* decrypted data */
					 &decryptDataLength,		/* actual size of decrypted
									   data */
					 tpm_key->encData.buffer,	/* encrypted data */
					 tpm_key->encData.size,		/* encrypted data size */
					 parent_key);
    }
    /* load the TPM_STORE_ASYMKEY cache from the 'encData' member stream */
    if (rc == 0) {
	stream = decryptData;
	stream_size = decryptDataLength;
	rc = TPM_Key_LoadStoreAsymKey(tpm_key, FALSE, &stream, &stream_size);
    }
    free(decryptData);		/* @1 */
    return rc;
}

/* TPM_Key_GeneratePCRDigest() generates a digest based on the current PCR state and the PCR's
   specified with the key.

   The key can be either TPM_KEY or TPM_KEY12.

   This function assumes that TPM_Key_GetPCRUsage() has determined that PCR's are in use, so
   a NULL PCR cache will return an error here.

   See Part 1 25.1 
*/

TPM_RESULT TPM_Key_CheckPCRDigest(TPM_KEY *tpm_key,
				  tpm_state_t *tpm_state)
{
    TPM_RESULT		rc = 0;
    
    printf(" TPM_Key_GeneratePCRDigest:\n");
    if (((TPM_KEY12 *)tpm_key)->tag != TPM_TAG_KEY12) { /* TPM_KEY */
	/* i. Calculate H1 a TPM_COMPOSITE_HASH of the PCR selected by LK -> pcrInfo ->
	   releasePCRSelection */
	/* ii. Compare H1 to LK -> pcrInfo -> digestAtRelease on mismatch return TPM_WRONGPCRVAL */
	if (rc == 0) {
	    rc = TPM_PCRInfo_CheckDigest(tpm_key->tpm_pcr_info,
					 tpm_state->tpm_stclear_data.PCRS);	/* array of PCR's */
	}
    }
    else {					/* TPM_KEY12 */
	/* i. Calculate H1 a TPM_COMPOSITE_HASH of the PCR selected by LK -> pcrInfo ->
	   releasePCRSelection */
	/* ii. Compare H1 to LK -> pcrInfo -> digestAtRelease on mismatch return TPM_WRONGPCRVAL */
	if (rc == 0) {
	    rc = TPM_PCRInfoLong_CheckDigest(tpm_key->tpm_pcr_info_long,
					     tpm_state->tpm_stclear_data.PCRS,	/* array of PCR's */
					     tpm_state->tpm_stany_flags.localityModifier);
	}
    }
    /* 4. Allow use of the key */
    if (rc != 0) {
	printf("TPM_Key_CheckPCRDigest: Error, wrong digestAtRelease value\n");
	rc = TPM_WRONGPCRVAL;
    }
    return rc;
}

/* TPM_Key_CheckRestrictDelegate() checks the restrictDelegate data against the TPM_KEY properties.
   It determines how the TPM responds to delegated requests to use a certified migration key.

   Called from TPM_AuthSessions_GetData() if it's a DSAP session using a key entity..

   TPM_PERMANENT_DATA -> restrictDelegate is used as follows:

   1. If the session type is TPM_PID_DSAP and TPM_KEY -> keyFlags -> migrateAuthority is TRUE
   a. If
     TPM_KEY_USAGE is TPM_KEY_SIGNING and restrictDelegate -> TPM_CMK_DELEGATE_SIGNING is TRUE, or
     TPM_KEY_USAGE is TPM_KEY_STORAGE and restrictDelegate -> TPM_CMK_DELEGATE_STORAGE is TRUE, or
     TPM_KEY_USAGE is TPM_KEY_BIND and restrictDelegate -> TPM_CMK_DELEGATE_BIND is TRUE, or
     TPM_KEY_USAGE is TPM_KEY_LEGACY and restrictDelegate -> TPM_CMK_DELEGATE_LEGACY is TRUE, or
     TPM_KEY_USAGE is TPM_KEY_MIGRATE and restrictDelegate -> TPM_CMK_DELEGATE_MIGRATE is TRUE
   then the key can be used.
   b. Else return TPM_INVALID_KEYUSAGE.

*/

TPM_RESULT TPM_Key_CheckRestrictDelegate(TPM_KEY *tpm_key,
					 TPM_CMK_DELEGATE restrictDelegate)
{
    TPM_RESULT	rc = 0;
    
    printf("TPM_Key_CheckRestrictDelegate:\n");
    if (rc == 0) {
	if (tpm_key == NULL) {
	    printf("TPM_Key_CheckRestrictDelegate: Error (fatal), key NULL\n");
	    rc = TPM_FAIL;	/* internal error, should never occur */
	}
    }
    /* if it's a certified migration key */
    if (rc == 0) {
	if (tpm_key->keyFlags & TPM_MIGRATEAUTHORITY) {
	    if (!(
		  ((restrictDelegate & TPM_CMK_DELEGATE_SIGNING) &&
		   (tpm_key->keyUsage == TPM_KEY_SIGNING)) ||

		  ((restrictDelegate & TPM_CMK_DELEGATE_STORAGE) &&
		   (tpm_key->keyUsage == TPM_KEY_STORAGE)) ||

		  ((restrictDelegate & TPM_CMK_DELEGATE_BIND) &&
		   (tpm_key->keyUsage == TPM_KEY_BIND)) ||

		  ((restrictDelegate & TPM_CMK_DELEGATE_LEGACY) &&
		   (tpm_key->keyUsage == TPM_KEY_LEGACY)) ||

		  ((restrictDelegate & TPM_CMK_DELEGATE_MIGRATE) &&
		   (tpm_key->keyUsage == TPM_KEY_MIGRATE))
		  )) {
		printf("TPM_Key_CheckRestrictDelegate: Error, "
		       "invalid keyUsage %04hx restrictDelegate %08x\n",
		       tpm_key->keyUsage, restrictDelegate);
		rc = TPM_INVALID_KEYUSAGE;
	    }
	}
    }
    return rc;
}

/*
  TPM_KEY_FLAGS
*/

/* TPM_KeyFlags_Load() deserializes a TPM_KEY_FLAGS value and checks for a legal value.
 */

TPM_RESULT TPM_KeyFlags_Load(TPM_KEY_FLAGS *tpm_key_flags,	/* result */
			     unsigned char **stream,		/* pointer to next parameter */
			     uint32_t *stream_size)		/* stream size left */
{
    TPM_RESULT		rc = 0;

    /* load keyFlags */
    if (rc == 0) {
	rc = TPM_Load32(tpm_key_flags, stream, stream_size);
    }
    /* check TPM_KEY_FLAGS validity, look for extra bits set */
    if (rc == 0) {
	if (*tpm_key_flags & ~TPM_KEY_FLAGS_MASK) {
	    printf("TPM_KeyFlags_Load: Error, illegal keyFlags value %08x\n",
		   *tpm_key_flags);
	    rc = TPM_BAD_KEY_PROPERTY;
	}
    }
    return rc;
}

/*
  TPM_KEY_PARMS
*/

void TPM_KeyParms_Init(TPM_KEY_PARMS *tpm_key_parms)
{
    printf(" TPM_KeyParms_Init:\n");
    tpm_key_parms->algorithmID = 0;
    tpm_key_parms->encScheme = TPM_ES_NONE;
    tpm_key_parms->sigScheme = TPM_SS_NONE;
    TPM_SizedBuffer_Init(&(tpm_key_parms->parms));
    tpm_key_parms->tpm_rsa_key_parms = NULL;
    return;
}

#if 0
/* TPM_KeyParms_SetRSA() is a 'Set' version specific to RSA keys */

TPM_RESULT TPM_KeyParms_SetRSA(TPM_KEY_PARMS *tpm_key_parms,
			       TPM_ALGORITHM_ID algorithmID,
			       TPM_ENC_SCHEME encScheme,
			       TPM_SIG_SCHEME sigScheme,
			       uint32_t keyLength,	/* in bits */
			       TPM_SIZED_BUFFER *exponent)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_KeyParms_SetRSA:\n");
    /* copy the TPM_KEY_PARMS members */
    if (rc == 0) {
	tpm_key_parms->algorithmID = algorithmID;
	tpm_key_parms->encScheme = encScheme;
	tpm_key_parms->sigScheme = sigScheme;
	/* construct the TPM_RSA_KEY_PARMS cache member object */
	rc = TPM_RSAKeyParms_New(&tpm_key_parms->tpm_rsa_key_parms);
    }
    if (rc == 0) {
	/* copy the TPM_RSA_KEY_PARMS members */
	tpm_key_parms->tpm_rsa_key_parms->keyLength = keyLength;
	tpm_key_parms->tpm_rsa_key_parms->numPrimes = 2;
	rc = TPM_SizedBuffer_Copy(&(tpm_key_parms->tpm_rsa_key_parms->exponent), exponent);
    }
    /* serialize the TPM_RSA_KEY_PARMS object back to TPM_KEY_PARMS */
    if (rc == 0) {
	rc = TPM_SizedBuffer_SetStructure(&(tpm_key_parms->parms),
					  tpm_key_parms->tpm_rsa_key_parms,
					  (TPM_STORE_FUNCTION_T)TPM_RSAKeyParms_Store);
    }
    return rc;
}
#endif


/* TPM_KeyParms_Copy() copies the source to the destination.

   If the algorithmID is TPM_ALG_RSA, the tpm_rsa_key_parms cache is allocated and copied.

   Must be freed by TPM_KeyParms_Delete() after use
*/

TPM_RESULT TPM_KeyParms_Copy(TPM_KEY_PARMS *tpm_key_parms_dest,
			     TPM_KEY_PARMS *tpm_key_parms_src)
{
    TPM_RESULT rc = 0;
    
    printf(" TPM_KeyParms_Copy:\n");
    if (rc == 0) {
	tpm_key_parms_dest->algorithmID = tpm_key_parms_src->algorithmID;
	tpm_key_parms_dest->encScheme	= tpm_key_parms_src->encScheme;
	tpm_key_parms_dest->sigScheme	= tpm_key_parms_src->sigScheme;
	rc = TPM_SizedBuffer_Copy(&(tpm_key_parms_dest->parms),
				  &(tpm_key_parms_src->parms));
    }
    /* if there is a destination TPM_RSA_KEY_PARMS cache */
    if ((rc == 0) && (tpm_key_parms_dest->algorithmID == TPM_ALG_RSA)) {
	/* construct the TPM_RSA_KEY_PARMS cache member object */
	if (rc == 0) {
	    rc = TPM_RSAKeyParms_New(&(tpm_key_parms_dest->tpm_rsa_key_parms));
	}
	/* copy the TPM_RSA_KEY_PARMS member */
	if (rc == 0) {
	    rc = TPM_RSAKeyParms_Copy(tpm_key_parms_dest->tpm_rsa_key_parms,
				      tpm_key_parms_src->tpm_rsa_key_parms);
	}
    }
    return rc;
}

/* TPM_KeyParms_Load deserializes a stream to a TPM_KEY_PARMS structure.

   Must be freed by TPM_KeyParms_Delete() after use
*/

TPM_RESULT TPM_KeyParms_Load(TPM_KEY_PARMS *tpm_key_parms,	/* result */
			     unsigned char **stream,		/* pointer to next parameter */
			     uint32_t *stream_size)		/* stream size left */
{
    TPM_RESULT		rc = 0;
    unsigned char	*parms_stream;
    uint32_t		parms_stream_size;
    
    printf(" TPM_KeyParms_Load:\n");
    /* load algorithmID */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_key_parms->algorithmID), stream, stream_size);
    }
    /* load encScheme */
    if (rc == 0) {
	rc = TPM_Load16(&(tpm_key_parms->encScheme), stream, stream_size);
    }
    /* load sigScheme */
    if (rc == 0) {
	rc = TPM_Load16(&(tpm_key_parms->sigScheme), stream, stream_size);
    }
    /* load parmSize and parms */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_key_parms->parms), stream, stream_size);
    }
    if (rc == 0) {
	switch (tpm_key_parms->algorithmID) {
	    /* Allow load of uninitialized structures */
	  case 0:
	    break;

	  case TPM_ALG_RSA:
	    /* load the TPM_RSA_KEY_PARMS cache if the algorithmID indicates an RSA key */
	    if (rc == 0) {
		rc = TPM_RSAKeyParms_New(&(tpm_key_parms->tpm_rsa_key_parms));
	    }
	    /* deserialize the parms stream, but don't move the pointer */
	    if (rc == 0) {
		parms_stream = tpm_key_parms->parms.buffer;
		parms_stream_size = tpm_key_parms->parms.size;
		rc = TPM_RSAKeyParms_Load(tpm_key_parms->tpm_rsa_key_parms,
					  &parms_stream, &parms_stream_size);
	    }
	    break;

	    /* NOTE Only handles TPM_RSA_KEY_PARMS, could handle TPM_SYMMETRIC_KEY_PARMS */
	  case TPM_ALG_AES128:
	  case TPM_ALG_AES192:
	  case TPM_ALG_AES256:
	  default:
	    printf("TPM_KeyParms_Load: Cannot handle algorithmID %08x\n",
		   tpm_key_parms->algorithmID);
	    rc = TPM_BAD_KEY_PROPERTY;
	    break;
	}
    }
    return rc;
}

TPM_RESULT TPM_KeyParms_GetExponent(uint32_t		*ebytes,
				    unsigned char	**earr,
				    TPM_KEY_PARMS	*tpm_key_parms)
{
    TPM_RESULT		rc = 0;
    TPM_RSA_KEY_PARMS	*tpm_rsa_key_parms;
    
    printf(" TPM_KeyParms_GetExponent:\n");
    if (rc == 0) {
	rc = TPM_KeyParms_GetRSAKeyParms(&tpm_rsa_key_parms, tpm_key_parms);
    }
    if (rc == 0) {
	rc = TPM_RSAKeyParms_GetExponent(ebytes, earr, tpm_rsa_key_parms);
    }
    return rc;
}
     

/* TPM_KeyParms_Store serializes a TPM_KEY_PARMS structure, appending results to 'sbuffer'
*/

TPM_RESULT TPM_KeyParms_Store(TPM_STORE_BUFFER *sbuffer,
			      TPM_KEY_PARMS *tpm_key_parms)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_KeyParms_Store:\n");
    /* store algorithmID */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_key_parms->algorithmID); 
    }
    /* store encScheme */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, tpm_key_parms->encScheme); 
    }
    /* store sigScheme */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, tpm_key_parms->sigScheme); 
    }
    /* copy cache to parms */
    if (rc == 0) {
	switch (tpm_key_parms->algorithmID) {
	    /* Allow store of uninitialized structures */
	  case 0:
	    break;
	  case TPM_ALG_RSA:
	    rc = TPM_SizedBuffer_SetStructure(&(tpm_key_parms->parms),
					      tpm_key_parms->tpm_rsa_key_parms,
					      (TPM_STORE_FUNCTION_T)TPM_RSAKeyParms_Store);
	    break;
	    /* NOTE Only handles TPM_RSA_KEY_PARMS, could handle TPM_SYMMETRIC_KEY_PARMS */
	  case TPM_ALG_AES128:
	  case TPM_ALG_AES192:
	  case TPM_ALG_AES256:
	  default:
	    printf("TPM_KeyParms_Store: Cannot handle algorithmID %08x\n",
		   tpm_key_parms->algorithmID);
	    rc = TPM_BAD_KEY_PROPERTY;
	    break;
	}
    }
    /* store parms */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_key_parms->parms));
    }
    return rc;
}

/* TPM_KeyParms_Delete frees any member allocated memory */
    
void TPM_KeyParms_Delete(TPM_KEY_PARMS *tpm_key_parms)
{
    printf(" TPM_KeyParms_Delete:\n");
    if (tpm_key_parms != NULL) {
	TPM_SizedBuffer_Delete(&(tpm_key_parms->parms));
	TPM_RSAKeyParms_Delete(tpm_key_parms->tpm_rsa_key_parms);
	free(tpm_key_parms->tpm_rsa_key_parms);
	TPM_KeyParms_Init(tpm_key_parms);
    }
    return;
}

/* TPM_KeyParms_GetRSAKeyParms() gets the TPM_RSA_KEY_PARMS from a TPM_KEY_PARMS cache.

   Returns an error if the cache is NULL, since the cache should always be set when the
   TPM_KEY_PARMS indicates an RSA key.
*/

TPM_RESULT TPM_KeyParms_GetRSAKeyParms(TPM_RSA_KEY_PARMS **tpm_rsa_key_parms,
				       TPM_KEY_PARMS *tpm_key_parms)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_KeyParms_GetRSAKeyParms:\n");
    /* algorithmID must be RSA */
    if (rc == 0) {
	if (tpm_key_parms->algorithmID != TPM_ALG_RSA) {
	    printf("TPM_KeyParms_GetRSAKeyParms: Error, incorrect algorithmID %08x\n",
		   tpm_key_parms->algorithmID);
	    rc = TPM_BAD_KEY_PROPERTY;
	}
    }
    /* if the TPM_RSA_KEY_PARMS structure has not been cached, deserialize it */
    if (rc == 0) {
	if (tpm_key_parms->tpm_rsa_key_parms == NULL) {
	    printf("TPM_KeyParms_GetRSAKeyParms: Error (fatal), cache is NULL\n");
	    /* This should never occur.	 The cache is loaded when the TPM_KEY_PARMS is loaded. */
	    rc = TPM_FAIL;
	}
    }
    /* return the cached structure */
    if (rc == 0) {
	*tpm_rsa_key_parms = tpm_key_parms->tpm_rsa_key_parms;
    }
    return rc; 
}

/* TPM_KeyParms_CheckProperties() checks that the TPM can generate a key of the type requested in
   'tpm_key_parms'

   if' keyLength' is non-zero, checks that the tpm_key specifies the correct key length.  If
   keyLength is 0, any tpm_key key length is accepted.
*/

TPM_RESULT TPM_KeyParms_CheckProperties(TPM_KEY_PARMS *tpm_key_parms,
					TPM_KEY_USAGE tpm_key_usage,
					uint32_t keyLength,	/* in bits */
					TPM_BOOL FIPS)
{
    TPM_RESULT	rc = 0;
    TPM_RSA_KEY_PARMS *tpm_rsa_key_parms = NULL;/* used if algorithmID indicates RSA */

    printf("  TPM_KeyParms_CheckProperties: keyUsage %04hx\n", tpm_key_usage);
    printf("  TPM_KeyParms_CheckProperties: sigScheme %04hx\n", tpm_key_parms->sigScheme);
    printf("  TPM_KeyParms_CheckProperties: encScheme %04hx\n", tpm_key_parms->encScheme);
    if (rc == 0) {
	/* the code currently only supports RSA */
	if (tpm_key_parms->algorithmID != TPM_ALG_RSA) {
	    printf("TPM_KeyParms_CheckProperties: Error, algorithmID not TPM_ALG_RSA\n");
	    rc = TPM_BAD_KEY_PROPERTY;
	}
    }
    /* get the TPM_RSA_KEY_PARMS structure from the TPM_KEY_PARMS structure */
    /* NOTE: for now only support RSA keys */
    if (rc == 0) {
	rc = TPM_KeyParms_GetRSAKeyParms(&tpm_rsa_key_parms, tpm_key_parms);
    }
    /* check key length if specified as input parameter */
    if ((rc == 0) && (keyLength != 0)) {
	if (tpm_rsa_key_parms->keyLength != keyLength) {	/* in bits */
	    printf("TPM_KeyParms_CheckProperties: Error, Bad keyLength should be %u, was %u\n",
		   keyLength, tpm_rsa_key_parms->keyLength);
	    rc = TPM_BAD_KEY_PROPERTY;
	}
    }
    if (rc == 0) {
	if (tpm_rsa_key_parms->keyLength > TPM_RSA_KEY_LENGTH_MAX) {	/* in bits */
	    printf("TPM_KeyParms_CheckProperties: Error, Bad keyLength max %u, was %u\n",
		   TPM_RSA_KEY_LENGTH_MAX, tpm_rsa_key_parms->keyLength);
	    rc = TPM_BAD_KEY_PROPERTY;
	}
	
    }
    /* kgold - Support only 2 primes */
    if (rc == 0) {
	if (tpm_rsa_key_parms->numPrimes != 2) {
	    printf("TPM_KeyParms_CheckProperties: Error, numPrimes %u should be 2\n",
		   tpm_rsa_key_parms->numPrimes);
	    rc = TPM_BAD_KEY_PROPERTY;
	}
    }
    /* if FIPS */
    if ((rc == 0) && FIPS) {
	/* a.  If keyInfo -> keySize is less than 1024 return TPM_NOTFIPS */
	if (tpm_rsa_key_parms->keyLength < 1024) {
	    printf("TPM_KeyParms_CheckProperties: Error, Invalid FIPS key length %u\n",
		   tpm_rsa_key_parms->keyLength);
	    rc = TPM_NOTFIPS;
	}
	/* c.  If keyInfo -> keyUsage specifies TPM_KEY_LEGACY return TPM_NOTFIPS */
	else if (tpm_key_usage == TPM_KEY_LEGACY) {
	    printf("TPM_KeyParms_CheckProperties: Error, FIPS authDataUsage TPM_AUTH_NEVER\n");
	    rc = TPM_NOTFIPS;
	}
    }
    /* From Part 2 5.7.1 Mandatory Key Usage Schemes  and TPM_CreateWrapKey, TPM_LoadKey */
    if (rc == 0) {
	switch (tpm_key_usage) {
	  case TPM_KEY_UNINITIALIZED:
	    printf("TPM_KeyParms_CheckProperties: Error, keyUsage TPM_KEY_UNINITIALIZED\n");
	    rc = TPM_BAD_KEY_PROPERTY;
	    break;
	  case TPM_KEY_SIGNING:
	    if (tpm_key_parms->encScheme != TPM_ES_NONE) {
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Signing encScheme %04hx is not TPM_ES_NONE\n",
		       tpm_key_parms->encScheme);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
#ifdef TPM_V12
	    else if ((tpm_key_parms->sigScheme != TPM_SS_RSASSAPKCS1v15_SHA1) &&
		     (tpm_key_parms->sigScheme != TPM_SS_RSASSAPKCS1v15_DER) &&
		     (tpm_key_parms->sigScheme != TPM_SS_RSASSAPKCS1v15_INFO)) {
#else	/* TPM 1.1 */
	    else if (tpm_key_parms->sigScheme != TPM_SS_RSASSAPKCS1v15_SHA1) {

#endif	
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Signing sigScheme %04hx is not DER, SHA1, INFO\n",
		       tpm_key_parms->sigScheme);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	    break;
	  case TPM_KEY_STORAGE:
	    if (tpm_key_parms->encScheme != TPM_ES_RSAESOAEP_SHA1_MGF1) {
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Storage encScheme %04hx is not TPM_ES_RSAESOAEP_SHA1_MGF1\n",
		       tpm_key_parms->encScheme);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	    else if (tpm_key_parms->sigScheme != TPM_SS_NONE) {
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Storage sigScheme %04hx is not TPM_SS_NONE\n",
		       tpm_key_parms->sigScheme);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	    else if (tpm_key_parms->algorithmID != TPM_ALG_RSA) { /*constant condition*/
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Storage algorithmID %08x is not TPM_ALG_RSA\n",
		       tpm_key_parms->algorithmID);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	    /* interoperable TPM only supports 2048 */
	    else if (tpm_rsa_key_parms->keyLength < 2048) {
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Storage keyLength %d is less than 2048\n",
		       tpm_rsa_key_parms->keyLength);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	    else {
		rc = TPM_KeyParams_CheckDefaultExponent(&(tpm_rsa_key_parms->exponent));
	    }
	    break;
	  case TPM_KEY_IDENTITY:
	    if (tpm_key_parms->encScheme != TPM_ES_NONE) {
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Identity encScheme %04hx is not TPM_ES_NONE\n",
		       tpm_key_parms->encScheme);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	    else if (tpm_key_parms->sigScheme != TPM_SS_RSASSAPKCS1v15_SHA1) {
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Identity sigScheme %04hx is not %04x\n",
		       tpm_key_parms->sigScheme, TPM_SS_RSASSAPKCS1v15_SHA1);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	    else if (tpm_key_parms->algorithmID != TPM_ALG_RSA) { /*constant condition*/
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Identity algorithmID %08x is not TPM_ALG_RSA\n",
		       tpm_key_parms->algorithmID);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	    /* interoperable TPM only supports 2048 */
	    else if (tpm_rsa_key_parms->keyLength < 2048) {
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Identity keyLength %d is less than 2048\n",
		       tpm_rsa_key_parms->keyLength);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	    else {
		rc = TPM_KeyParams_CheckDefaultExponent(&(tpm_rsa_key_parms->exponent));
	    }
	    break;
	  case TPM_KEY_AUTHCHANGE:
	    if (tpm_key_parms->encScheme != TPM_ES_RSAESOAEP_SHA1_MGF1) {
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Authchange encScheme %04hx is not TPM_ES_RSAESOAEP_SHA1_MGF1\n",
		       tpm_key_parms->encScheme);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	    else if (tpm_key_parms->sigScheme != TPM_SS_NONE) {
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Authchange sigScheme %04hx is not TPM_SS_NONE\n",
		       tpm_key_parms->sigScheme);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	    else if (tpm_rsa_key_parms->keyLength < 512) {
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Authchange keyLength %d is less than 512\n",
		       tpm_rsa_key_parms->keyLength);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	    break;
	  case TPM_KEY_BIND:
	    if ((tpm_key_parms->encScheme != TPM_ES_RSAESOAEP_SHA1_MGF1) &&
		(tpm_key_parms->encScheme != TPM_ES_RSAESPKCSv15)) {
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Bind encScheme %04hx is not %04x or %04x\n",
		       tpm_key_parms->encScheme,
		       TPM_ES_RSAESOAEP_SHA1_MGF1, TPM_ES_RSAESPKCSv15);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	    else if (tpm_key_parms->sigScheme != TPM_SS_NONE) {
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Bind sigScheme %04hx is not TPM_SS_NONE\n",
		       tpm_key_parms->sigScheme);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	    break;
	  case TPM_KEY_LEGACY:
	    if ((tpm_key_parms->encScheme != TPM_ES_RSAESOAEP_SHA1_MGF1) &&
		(tpm_key_parms->encScheme != TPM_ES_RSAESPKCSv15)) {
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Legacy encScheme %04hx is not %04x or %04x\n",
		       tpm_key_parms->encScheme,
		       TPM_ES_RSAESOAEP_SHA1_MGF1, TPM_ES_RSAESPKCSv15);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	    else if ((tpm_key_parms->sigScheme != TPM_SS_RSASSAPKCS1v15_SHA1) &&
		     (tpm_key_parms->sigScheme != TPM_SS_RSASSAPKCS1v15_DER)) {
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Legacy sigScheme %04hx is not %04x or %04x\n",
		       tpm_key_parms->sigScheme,
		       TPM_SS_RSASSAPKCS1v15_SHA1, TPM_SS_RSASSAPKCS1v15_DER);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	    break;
	  case TPM_KEY_MIGRATE:
	    if (tpm_key_parms->encScheme != TPM_ES_RSAESOAEP_SHA1_MGF1) {
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Migrate encScheme %04hx is not TPM_ES_RSAESOAEP_SHA1_MGF1\n",
		       tpm_key_parms->encScheme);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	    else if (tpm_key_parms->sigScheme != TPM_SS_NONE) {
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Migrate sigScheme %04hx is not TPM_SS_NONE\n",
		       tpm_key_parms->sigScheme);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	    else if (tpm_key_parms->algorithmID != TPM_ALG_RSA) { /*constant condition*/
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Migrate algorithmID %08x is not TPM_ALG_RSA\n",
		       tpm_key_parms->algorithmID);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	    /* interoperable TPM only supports 2048 */
	    else if (tpm_rsa_key_parms->keyLength < 2048) {
		printf("TPM_KeyParms_CheckProperties: Error, "
		       "Migrate keyLength %d is less than 2048\n",
		       tpm_rsa_key_parms->keyLength);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	    else {
		rc = TPM_KeyParams_CheckDefaultExponent(&(tpm_rsa_key_parms->exponent));
	    }
	    break;
	  default:
	    printf("TPM_KeyParms_CheckProperties: Error, Unknown keyUsage %04hx\n", tpm_key_usage);
	    rc = TPM_BAD_KEY_PROPERTY;
	    break;
	}
    }
    return rc; 
}

TPM_RESULT TPM_KeyParams_CheckDefaultExponent(TPM_SIZED_BUFFER *exponent)
{
    TPM_RESULT	rc = 0;
    uint32_t	i;
    
    if ((rc == 0) && (exponent->size != 0)) {	 /* 0 is the default */
	printf("  TPM_KeyParams_CheckDefaultExponent: exponent size %u\n", exponent->size);
	if (rc == 0) {
	    if (exponent->size < 3) {		 
		printf("TPM_KeyParams_CheckDefaultExponent: Error, exponent size is %u\n",
		       exponent->size);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	}
	if (rc == 0) {
	    for (i = 3 ; i < exponent->size ; i++) {
		if (exponent->buffer[i] != 0) {
		    printf("TPM_KeyParams_CheckDefaultExponent: Error, exponent[%u] is %02x\n",
			   i, exponent->buffer[i]);
		    rc = TPM_BAD_KEY_PROPERTY;
		}
	    }
	}
	if (rc == 0) {
	    if ((exponent->buffer[0] != tpm_default_rsa_exponent[0]) ||
		(exponent->buffer[1] != tpm_default_rsa_exponent[1]) ||
		(exponent->buffer[2] != tpm_default_rsa_exponent[2])) {
		printf("TPM_KeyParams_CheckDefaultExponent: Error, exponent is %02x %02x %02x\n",
		       exponent->buffer[2], exponent->buffer[1], exponent->buffer[0]);
		rc = TPM_BAD_KEY_PROPERTY;
	    }
	}
    }
    return rc; 
}

/*
  TPM_STORE_ASYMKEY
*/

void TPM_StoreAsymkey_Init(TPM_STORE_ASYMKEY *tpm_store_asymkey)
{
    printf(" TPM_StoreAsymkey_Init:\n");
    tpm_store_asymkey->payload = TPM_PT_ASYM;
    TPM_Secret_Init(tpm_store_asymkey->usageAuth);
    TPM_Secret_Init(tpm_store_asymkey->migrationAuth);
    TPM_Digest_Init(tpm_store_asymkey->pubDataDigest);
    TPM_StorePrivkey_Init(&(tpm_store_asymkey->privKey));
    return;
}

/* TPM_StoreAsymkey_Load() deserializes the TPM_STORE_ASYMKEY structure.

   The serialized structure contains the private factor p.  Normally, 'tpm_key_parms' and
   tpm_store_pubkey are not NULL and the private key d is derived from p and the public key n and
   exponent e.

   In some cases, a TPM_STORE_ASYMKEY is being manipulated without the rest of the TPM_KEY
   structure.  When 'tpm_key' is NULL, p is left intact, and the resulting structure cannot be used
   as a private key.
*/

TPM_RESULT TPM_StoreAsymkey_Load(TPM_STORE_ASYMKEY *tpm_store_asymkey,
				 TPM_BOOL isEK,
				 unsigned char **stream,	
				 uint32_t *stream_size,
				 TPM_KEY_PARMS *tpm_key_parms,
				 TPM_SIZED_BUFFER *pubKey)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_StoreAsymkey_Load:\n");
    /* load payload */
    if ((rc == 0) && !isEK) {
	rc = TPM_Load8(&(tpm_store_asymkey->payload), stream, stream_size);
    }
    /* check payload value to ease debugging */
    if ((rc == 0) && !isEK) {
	if (
	    /* normal key */
	    (tpm_store_asymkey->payload != TPM_PT_ASYM) &&
	    /* TPM_CMK_CreateKey payload */
	    (tpm_store_asymkey->payload != TPM_PT_MIGRATE_RESTRICTED) &&
	    /* TPM_CMK_ConvertMigration payload */
	    (tpm_store_asymkey->payload != TPM_PT_MIGRATE_EXTERNAL)
	    ) {
	    printf("TPM_StoreAsymkey_Load: Error, invalid payload %02x\n",
		   tpm_store_asymkey->payload);
	    rc = TPM_INVALID_STRUCTURE;
	}
    }
    /* load usageAuth */
    if ((rc == 0) && !isEK) {
	rc = TPM_Secret_Load(tpm_store_asymkey->usageAuth, stream, stream_size);
    }
    /* load migrationAuth */
    if ((rc == 0) && !isEK) {
	rc = TPM_Secret_Load(tpm_store_asymkey->migrationAuth, stream, stream_size);
    }
    /* load pubDataDigest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_store_asymkey->pubDataDigest, stream, stream_size);
    }
    /* load privKey - actually prime factor p */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load((&(tpm_store_asymkey->privKey.p_key)),
				  stream, stream_size);
    }
    /* convert prime factor p to the private key */
    if ((rc == 0) && (tpm_key_parms != NULL) && (pubKey != NULL)) {
	rc = TPM_StorePrivkey_Convert(tpm_store_asymkey,
				      tpm_key_parms, pubKey);
    }
    return rc;
}

#if 0
static TPM_RESULT TPM_StoreAsymkey_LoadTest(TPM_KEY *tpm_key)
{
    TPM_RESULT	rc = 0;
    int		irc;

    /* actual */
    unsigned char	*narr;
    unsigned char	*earr;
    unsigned char	*parr;		
    unsigned char	*qarr;
    unsigned char	*darr;

    uint32_t		nbytes;
    uint32_t		ebytes;
    uint32_t		pbytes;
    uint32_t		qbytes;
    uint32_t		dbytes;

    /* computed */
    unsigned char	*q1arr = NULL;
    unsigned char	*d1arr = NULL;

    uint32_t		q1bytes;
    uint32_t		d1bytes;

    printf(" TPM_StoreAsymkey_LoadTest:\n");
    /* actual data */
    if (rc == 0) {
	narr = tpm_key->pubKey.key;
	darr = tpm_key->tpm_store_asymkey->privKey.d_key;
	parr = tpm_key->tpm_store_asymkey->privKey.p_key;
	qarr = tpm_key->tpm_store_asymkey->privKey.q_key;

	nbytes = tpm_key->pubKey.keyLength;
	dbytes = tpm_key->tpm_store_asymkey->privKey.d_keyLength;
	pbytes = tpm_key->tpm_store_asymkey->privKey.p_keyLength;
	qbytes = tpm_key->tpm_store_asymkey->privKey.q_keyLength;
	
	rc = TPM_Key_GetPublicKey(&nbytes, &narr, tpm_key);
    }
    if (rc == 0) {
	rc = TPM_Key_GetExponent(&ebytes, &earr, tpm_key);
    }
    if (rc == 0) {
	rc = TPM_Key_GetPrimeFactorP(&pbytes, &parr, tpm_key);
    }
    /* computed data */
    if (rc == 0) {
	rc = TPM_RSAGetPrivateKey(&q1bytes, &q1arr,	/* freed @1 */
				  &d1bytes, &d1arr,	/* freed @2 */
				  nbytes, narr,
				  ebytes, earr,
				  pbytes, parr);
    }
    /* compare q */
    if (rc == 0) {
	if (qbytes != q1bytes) {
	    printf("TPM_StoreAsymkey_LoadTest: Error (fatal), qbytes %u q1bytes %u\n",
		   qbytes, q1bytes);
	    rc = TPM_FAIL;
	}
    }
    if (rc == 0) {
	irc = memcmp(qarr, q1arr, qbytes);
	if (irc != 0) {
	    printf("TPM_StoreAsymkey_LoadTest: Error (fatal), qarr mismatch\n");
	    rc = TPM_FAIL;
	}
    }
    /* compare d */
    if (rc == 0) {
	if (dbytes != d1bytes) {
	    printf("TPM_StoreAsymkey_LoadTest: Error (fatal), dbytes %u d1bytes %u\n",
		   dbytes, d1bytes);
	    rc = TPM_FAIL;
	}
    }
    if (rc == 0) {
	irc = memcmp(darr, d1arr, dbytes);
	if (irc != 0) {
	    printf("TPM_StoreAsymkey_LoadTest: Error (fatal), darr mismatch\n");
	    rc = TPM_FAIL;
	}
    }
    free(q1arr);	/* @1 */
    free(d1arr);	/* @2 */
    return rc;
}
#endif

TPM_RESULT TPM_StoreAsymkey_Store(TPM_STORE_BUFFER *sbuffer,
				  TPM_BOOL isEK,
				  const TPM_STORE_ASYMKEY *tpm_store_asymkey)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_StoreAsymkey_Store:\n");
    /* store payload */
    if ((rc == 0) && !isEK) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_store_asymkey->payload), sizeof(TPM_PAYLOAD_TYPE));
    }
    /* store usageAuth */
    if ((rc == 0) && !isEK) {
	rc = TPM_Secret_Store(sbuffer, tpm_store_asymkey->usageAuth);
    }
    /* store migrationAuth */
    if ((rc == 0) && !isEK) {
	rc = TPM_Secret_Store(sbuffer, tpm_store_asymkey->migrationAuth);
    }
    /* store pubDataDigest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_store_asymkey->pubDataDigest);
    }
    /* store privKey */
    if (rc == 0) {
	rc = TPM_StorePrivkey_Store(sbuffer, &(tpm_store_asymkey->privKey));
    }
    return rc;
}

void TPM_StoreAsymkey_Delete(TPM_STORE_ASYMKEY *tpm_store_asymkey)
{
    printf(" TPM_StoreAsymkey_Delete:\n");
    if (tpm_store_asymkey != NULL) {
	TPM_Secret_Delete(tpm_store_asymkey->usageAuth);
	TPM_Secret_Delete(tpm_store_asymkey->migrationAuth);
	TPM_StorePrivkey_Delete(&(tpm_store_asymkey->privKey));
	TPM_StoreAsymkey_Init(tpm_store_asymkey);
    }
    return;
}

TPM_RESULT TPM_StoreAsymkey_GenerateEncData(TPM_SIZED_BUFFER *encData,
					    TPM_STORE_ASYMKEY *tpm_store_asymkey,
					    TPM_KEY *parent_key)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	sbuffer;		/* TPM_STORE_ASYMKEY serialization */

    printf(" TPM_StoreAsymkey_GenerateEncData;\n");
    TPM_Sbuffer_Init(&sbuffer);			/* freed @1 */
    /* serialize the TPM_STORE_ASYMKEY member */
    if (rc == 0) {
	rc = TPM_StoreAsymkey_Store(&sbuffer, FALSE, tpm_store_asymkey);
    }
    if (rc == 0) {
	rc = TPM_RSAPublicEncryptSbuffer_Key(encData, &sbuffer, parent_key);
    }
    TPM_Sbuffer_Delete(&sbuffer);	/* @1 */
    return rc;
}

/* TPM_StoreAsymkey_GetPrimeFactorP() gets the prime factor p from the TPM_STORE_ASYMKEY
*/

TPM_RESULT TPM_StoreAsymkey_GetPrimeFactorP(uint32_t 		*pbytes,
					    unsigned char	**parr,
					    TPM_STORE_ASYMKEY	*tpm_store_asymkey)
{
    TPM_RESULT	rc = 0;
    
    printf(" TPM_StoreAsymkey_GetPrimeFactorP:\n");
    if (rc == 0) {
	*pbytes = tpm_store_asymkey->privKey.p_key.size;
	*parr = tpm_store_asymkey->privKey.p_key.buffer;
	TPM_PrintFour("  TPM_StoreAsymkey_GetPrimeFactorP:", *parr);
    }
    return rc;
}

/* TPM_StoreAsymkey_GetO1Size() calculates the destination o1 size for a TPM_STORE_ASYMKEY

   Used for creating a migration blob, TPM_STORE_ASYMKEY -> TPM_MIGRATE_ASYMKEY.
 */

void TPM_StoreAsymkey_GetO1Size(uint32_t		*o1_size,
				TPM_STORE_ASYMKEY	*tpm_store_asymkey)
{
    *o1_size = tpm_store_asymkey->privKey.p_key.size +	/* private key */
	       sizeof(uint32_t) -		/* private key length */
	       TPM_DIGEST_SIZE +		/* - k1 -> k2 TPM_MIGRATE_ASYMKEY -> partPrivKey */
	       sizeof(uint32_t) +		/* TPM_MIGRATE_ASYMKEY -> partPrivKeyLen */
	       sizeof(TPM_PAYLOAD_TYPE) +	/* TPM_MIGRATE_ASYMKEY -> payload */
	       TPM_SECRET_SIZE +		/* TPM_MIGRATE_ASYMKEY -> usageAuth */
	       TPM_DIGEST_SIZE +		/* TPM_MIGRATE_ASYMKEY -> pubDataDigest */
	       TPM_DIGEST_SIZE +		/* OAEP pHash */
	       TPM_DIGEST_SIZE +		/* OAEP seed */
	       1;				/* OAEP 0x01 byte */
    printf(" TPM_StoreAsymkey_GetO1Size: key size %u o1 size %u\n",
	   tpm_store_asymkey->privKey.p_key.size, *o1_size);
}

/* TPM_StoreAsymkey_CheckO1Size() verifies the destination o1_size against the source k1k2 array
   length

   This is a currently just a sanity check on the TPM_StoreAsymkey_GetO1Size() function.
*/

TPM_RESULT TPM_StoreAsymkey_CheckO1Size(uint32_t o1_size,
					uint32_t k1k2_length)
{
    TPM_RESULT rc = 0;
    
    /* sanity check the TPM_MIGRATE_ASYMKEY size against the requested o1 size */
    /* K1 K2 are the length and value of the private key, 4 + 128 bytes for a 2048-bit key */
    if (o1_size <
	(k1k2_length - TPM_DIGEST_SIZE + /* k1 k2, the first 20 bytes are used as the OAEP seed */
	 sizeof(TPM_PAYLOAD_TYPE) +	/* TPM_MIGRATE_ASYMKEY -> payload */
	 TPM_SECRET_SIZE +		/* TPM_MIGRATE_ASYMKEY -> usageAuth */
	 TPM_DIGEST_SIZE +		/* TPM_MIGRATE_ASYMKEY -> pubDataDigest */
	 sizeof(uint32_t) +		/* TPM_MIGRATE_ASYMKEY -> partPrivKeyLen */
	 TPM_DIGEST_SIZE +		/* OAEP pHash */
	 TPM_DIGEST_SIZE +				/* OAEP seed */
	 1)) {				/* OAEP 0x01 byte */
	printf("  TPM_StoreAsymkey_CheckO1Size: Error (fatal) k1k2_length %d too large for o1 %u\n",
	       k1k2_length, o1_size);
	rc = TPM_FAIL;
    }
    return rc;
}

/* TPM_StoreAsymkey_StoreO1() creates an OAEP encoded TPM_MIGRATE_ASYMKEY from a
   TPM_STORE_ASYMKEY.

   It does the common steps of constructing the TPM_MIGRATE_ASYMKEY, serializing it, and OAEP
   padding.
*/

TPM_RESULT TPM_StoreAsymkey_StoreO1(BYTE		*o1,
				    uint32_t		o1_size,
				    TPM_STORE_ASYMKEY	*tpm_store_asymkey,
				    TPM_DIGEST		pHash,
				    TPM_PAYLOAD_TYPE	payload_type,
				    TPM_SECRET		usageAuth)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	k1k2_sbuffer;	/* serialization of TPM_STORE_ASYMKEY -> privKey -> key */
    const unsigned char *k1k2;		/* serialization results */
    uint32_t		k1k2_length;
    TPM_MIGRATE_ASYMKEY tpm_migrate_asymkey;
    TPM_STORE_BUFFER	tpm_migrate_asymkey_sbuffer;	/* serialized tpm_migrate_asymkey */
    const unsigned char *tpm_migrate_asymkey_buffer;	
    uint32_t		tpm_migrate_asymkey_length;
    
    printf(" TPM_StoreAsymkey_StoreO1:\n");
    TPM_Sbuffer_Init(&k1k2_sbuffer);			/* freed @1 */
    TPM_MigrateAsymkey_Init(&tpm_migrate_asymkey);	/* freed @2 */
    TPM_Sbuffer_Init(&tpm_migrate_asymkey_sbuffer);	/* freed @3 */

    /* NOTE Comments from TPM_CreateMigrationBlob rev 81 */
    /* a. Build two byte arrays, K1 and K2: */
    /* i. K1 = TPM_STORE_ASYMKEY.privKey[0..19] (TPM_STORE_ASYMKEY.privKey.keyLength + 16 bytes of
       TPM_STORE_ASYMKEY.privKey.key), sizeof(K1) = 20 */
    /* ii. K2 = TPM_STORE_ASYMKEY.privKey[20..131] (position 16-127 of
       TPM_STORE_ASYMKEY. privKey.key), sizeof(K2) = 112 */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(&k1k2_sbuffer, &(tpm_store_asymkey->privKey.p_key));
    }
    if (rc == 0) {
	TPM_Sbuffer_Get(&k1k2_sbuffer, &k1k2, &k1k2_length);
	/* sanity check the TPM_STORE_ASYMKEY -> privKey -> key size against the requested o1
	   size */
	rc = TPM_StoreAsymkey_CheckO1Size(o1_size, k1k2_length);
    }
    /* b. Build M1 a TPM_MIGRATE_ASYMKEY structure */
    /* i. TPM_MIGRATE_ASYMKEY.payload = TPM_PT_MIGRATE */
    /* ii. TPM_MIGRATE_ASYMKEY.usageAuth = TPM_STORE_ASYMKEY.usageAuth */
    /* iii. TPM_MIGRATE_ASYMKEY.pubDataDigest = TPM_STORE_ASYMKEY. pubDataDigest */
    /* iv. TPM_MIGRATE_ASYMKEY.partPrivKeyLen = 112 - 127. */
    /* v. TPM_MIGRATE_ASYMKEY.partPrivKey = K2 */
    if (rc == 0) {
	tpm_migrate_asymkey.payload = payload_type;
	TPM_Secret_Copy(tpm_migrate_asymkey.usageAuth, usageAuth);
	TPM_Digest_Copy(tpm_migrate_asymkey.pubDataDigest, tpm_store_asymkey->pubDataDigest);
	TPM_PrintFour("  TPM_StoreAsymkey_StoreO1: k1 -", k1k2);
	TPM_PrintFour("  TPM_StoreAsymkey_StoreO1: k2 -", k1k2 + TPM_DIGEST_SIZE);
	rc = TPM_SizedBuffer_Set(&(tpm_migrate_asymkey.partPrivKey),
				 k1k2_length - TPM_DIGEST_SIZE,	/* k2 length 112 for 2048 bit key */
				 k1k2 + TPM_DIGEST_SIZE);	/* k2 */
    }
    /* c. Create o1 (which SHALL be 198 bytes for a 2048 bit RSA key) by performing the OAEP
       encoding of m using OAEP parameters of */
    /* i. m = M1 the TPM_MIGRATE_ASYMKEY structure */
    /* ii. pHash = d1->migrationAuth */
    /* iii. seed = s1 = K1 */
    if (rc == 0) {
	/* serialize TPM_MIGRATE_ASYMKEY m */
	rc = TPM_MigrateAsymkey_Store(&tpm_migrate_asymkey_sbuffer, &tpm_migrate_asymkey);
    }
    if (rc == 0) {
	/* get the serialization result */
	TPM_Sbuffer_Get(&tpm_migrate_asymkey_sbuffer,
			&tpm_migrate_asymkey_buffer, &tpm_migrate_asymkey_length);
	TPM_PrintFour("  TPM_StoreAsymkey_StoreO1: pHash -", pHash);
	rc = TPM_RSA_padding_add_PKCS1_OAEP(o1,				/* output */
					    o1_size,
					    tpm_migrate_asymkey_buffer, /* message */
					    tpm_migrate_asymkey_length,
					    pHash, 
					    k1k2);			/* k1, seed */
	TPM_PrintFour("  TPM_StoreAsymkey_StoreO1: o1 -", o1);
    }
    TPM_Sbuffer_Delete(&k1k2_sbuffer);			/* @1 */
    TPM_MigrateAsymkey_Delete(&tpm_migrate_asymkey);	/* @2 */
    TPM_Sbuffer_Delete(&tpm_migrate_asymkey_sbuffer);	/* @3 */
    return rc;
}

/* TPM_StoreAsymkey_LoadO1() extracts TPM_STORE_ASYMKEY from the OAEP encoded TPM_MIGRATE_ASYMKEY.

   It does the common steps OAEP depadding, deserializing the TPM_MIGRATE_ASYMKEY, and
   reconstructing the TPM_STORE_ASYMKEY.

   It sets these, which may or may not be correct at a higher level
   
   TPM_STORE_ASYMKEY -> payload	      = TPM_MIGRATE_ASYMKEY -> payload
   TPM_STORE_ASYMKEY -> usageAuth     = TPM_MIGRATE_ASYMKEY -> usageAuth
   TPM_STORE_ASYMKEY -> migrationAuth = pHash
   TPM_STORE_ASYMKEY -> pubDataDigest = TPM_MIGRATE_ASYMKEY -> pubDataDigest
   TPM_STORE_ASYMKEY -> privKey	      = seed + TPM_MIGRATE_ASYMKEY -> partPrivKey
*/

TPM_RESULT TPM_StoreAsymkey_LoadO1(TPM_STORE_ASYMKEY	*tpm_store_asymkey,	/* output */
				   BYTE			*o1,			/* input */
				   uint32_t		o1_size)		/* input */
{
    TPM_RESULT			rc = 0;
    BYTE			*tpm_migrate_asymkey_buffer;
    uint32_t			tpm_migrate_asymkey_length;
    TPM_DIGEST			seed;
    TPM_DIGEST			pHash;
    unsigned char		*stream;	/* for deserializing structures */
    uint32_t			stream_size;
    TPM_MIGRATE_ASYMKEY		tpm_migrate_asymkey;
    TPM_STORE_BUFFER		k1k2_sbuffer;
    const unsigned char		*k1k2_buffer;
    uint32_t			k1k2_length;
    
    printf(" TPM_StoreAsymkey_LoadO1:\n");
    TPM_MigrateAsymkey_Init(&tpm_migrate_asymkey);	/* freed @1 */
    TPM_Sbuffer_Init(&k1k2_sbuffer);			/* freed @2 */
    tpm_migrate_asymkey_buffer = NULL;			/* freed @3 */
    /* allocate memory for TPM_MIGRATE_ASYMKEY after removing OAEP pad from o1 */
    if (rc == 0) {
	rc = TPM_Malloc(&tpm_migrate_asymkey_buffer, o1_size);
    }
    if (rc == 0) {
	TPM_PrintFour("  TPM_StoreAsymkey_LoadO1: o1 -", o1);
	/* 5. Create m1, seed and pHash by OAEP decoding o1 */
	printf("  TPM_StoreAsymkey_LoadO1: Depadding\n");
	rc = TPM_RSA_padding_check_PKCS1_OAEP(tpm_migrate_asymkey_buffer,	/* out: to */
					      &tpm_migrate_asymkey_length,	/* out: to length */
					      o1_size,				/* to size */
					      o1, o1_size,		/* from, from length  */
					      pHash,
					      seed);	
	TPM_PrintFour("  TPM_StoreAsymkey_LoadO1: tpm_migrate_asymkey_buffer -",
		      tpm_migrate_asymkey_buffer);
	printf("  TPM_StoreAsymkey_LoadO1: tpm_migrate_asymkey_length %u\n",
	       tpm_migrate_asymkey_length);
	TPM_PrintFour("  TPM_StoreAsymkey_LoadO1: - pHash", pHash);
	TPM_PrintFour("  TPM_StoreAsymkey_LoadO1: - seed", seed);
    }
    /* deserialize the buffer back to a TPM_MIGRATE_ASYMKEY */
    if (rc == 0) {
	stream = tpm_migrate_asymkey_buffer;
	stream_size = tpm_migrate_asymkey_length;
	rc = TPM_MigrateAsymkey_Load(&tpm_migrate_asymkey, &stream, &stream_size);
	printf("  TPM_StoreAsymkey_LoadO1: partPrivKey length %u\n",
	       tpm_migrate_asymkey.partPrivKey.size);
	TPM_PrintFourLimit("  TPM_StoreAsymkey_LoadO1: partPrivKey -",
		      tpm_migrate_asymkey.partPrivKey.buffer,
		      tpm_migrate_asymkey.partPrivKey.size);
    }
    /* create k1k2 by combining seed (k1) and TPM_MIGRATE_ASYMKEY.partPrivKey (k2) field */
    if (rc == 0) {
	rc = TPM_Digest_Store(&k1k2_sbuffer, seed);
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(&k1k2_sbuffer,
				tpm_migrate_asymkey.partPrivKey.buffer,
				tpm_migrate_asymkey.partPrivKey.size);
    }
    /* assemble the TPM_STORE_ASYMKEY structure */
    if (rc == 0) {
	tpm_store_asymkey->payload = tpm_migrate_asymkey.payload;
	TPM_Digest_Copy(tpm_store_asymkey->usageAuth, tpm_migrate_asymkey.usageAuth);
	TPM_Digest_Copy(tpm_store_asymkey->migrationAuth, pHash);
	TPM_Digest_Copy(tpm_store_asymkey->pubDataDigest, tpm_migrate_asymkey.pubDataDigest);
	TPM_Sbuffer_Get(&k1k2_sbuffer, &k1k2_buffer, &k1k2_length);
	printf("  TPM_StoreAsymkey_LoadO1: k1k2 length %u\n", k1k2_length);
	TPM_PrintFourLimit("  TPM_StoreAsymkey_LoadO1: k1k2", k1k2_buffer, k1k2_length);
	rc = TPM_SizedBuffer_Load(&(tpm_store_asymkey->privKey.p_key),
				  (unsigned char **)&k1k2_buffer, &k1k2_length);
    }
    TPM_MigrateAsymkey_Delete(&tpm_migrate_asymkey);	/* @1 */
    TPM_Sbuffer_Delete(&k1k2_sbuffer);			/* @2 */
    free(tpm_migrate_asymkey_buffer);			/* @3 */
    return rc;
}


/*
  TPM_MIGRATE_ASYMKEY
*/

/* TPM_MigrateAsymkey_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_MigrateAsymkey_Init(TPM_MIGRATE_ASYMKEY *tpm_migrate_asymkey)
{
    printf(" TPM_MigrateAsymkey_Init:\n");
    tpm_migrate_asymkey->payload = TPM_PT_MIGRATE;
    TPM_Secret_Init(tpm_migrate_asymkey->usageAuth);
    TPM_Digest_Init(tpm_migrate_asymkey->pubDataDigest);
    TPM_SizedBuffer_Init(&(tpm_migrate_asymkey->partPrivKey));
    return;
}

/* TPM_MigrateAsymkey_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_MigrateAsymkey_Init()
   After use, call TPM_MigrateAsymkey_Delete() to free memory
*/

TPM_RESULT TPM_MigrateAsymkey_Load(TPM_MIGRATE_ASYMKEY *tpm_migrate_asymkey,
				   unsigned char **stream,
				   uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_MigrateAsymkey_Load:\n");
    /* load payload */
    if (rc == 0) {
	rc = TPM_Load8(&(tpm_migrate_asymkey->payload), stream, stream_size);
    }
    /* check payload value to ease debugging */
    if (rc == 0) {
	if ((tpm_migrate_asymkey->payload != TPM_PT_MIGRATE) &&
	    (tpm_migrate_asymkey->payload != TPM_PT_MAINT) &&
	    (tpm_migrate_asymkey->payload != TPM_PT_CMK_MIGRATE)) {
	    printf("TPM_MigrateAsymkey_Load: Error illegal payload %02x\n",
		   tpm_migrate_asymkey->payload);
	    rc = TPM_INVALID_STRUCTURE;
	}
    }
    /* load usageAuth */
    if (rc == 0) {
	rc = TPM_Secret_Load(tpm_migrate_asymkey->usageAuth, stream, stream_size);
    }
    /* load pubDataDigest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_migrate_asymkey->pubDataDigest, stream, stream_size);
    }
    /* load partPrivKey */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_migrate_asymkey->partPrivKey), stream, stream_size);
    }
    return rc;
}

/* TPM_MigrateAsymkey_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_MigrateAsymkey_Store(TPM_STORE_BUFFER *sbuffer,
				    const TPM_MIGRATE_ASYMKEY *tpm_migrate_asymkey)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_MigrateAsymkey_Store:\n");
    /* store payload */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_migrate_asymkey->payload), sizeof(TPM_PAYLOAD_TYPE));
    }
    /* store usageAuth */
    if (rc == 0) {
	rc = TPM_Secret_Store(sbuffer, tpm_migrate_asymkey->usageAuth);
    }
    /* store pubDataDigest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_migrate_asymkey->pubDataDigest);
    }
    /* store partPrivKey */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_migrate_asymkey->partPrivKey));
    }
    return rc;
}

/* TPM_MigrateAsymkey_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_MigrateAsymkey_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_MigrateAsymkey_Delete(TPM_MIGRATE_ASYMKEY *tpm_migrate_asymkey)
{
    printf(" TPM_MigrateAsymkey_Delete:\n");
    if (tpm_migrate_asymkey != NULL) {
	TPM_Secret_Delete(tpm_migrate_asymkey->usageAuth);
	TPM_SizedBuffer_Zero(&(tpm_migrate_asymkey->partPrivKey));
	TPM_SizedBuffer_Delete(&(tpm_migrate_asymkey->partPrivKey));
	TPM_MigrateAsymkey_Init(tpm_migrate_asymkey);
    }
    return;
}

/*
  TPM_STORE_PRIVKEY
*/

void TPM_StorePrivkey_Init(TPM_STORE_PRIVKEY *tpm_store_privkey)
{
    printf(" TPM_StorePrivkey_Init:\n");
    TPM_SizedBuffer_Init(&(tpm_store_privkey->d_key));
    TPM_SizedBuffer_Init(&(tpm_store_privkey->p_key));
    TPM_SizedBuffer_Init(&(tpm_store_privkey->q_key));
    return;
}

/* TPM_StorePrivkey_Convert() sets the prime factor q and private key d based on the prime factor p
   and the public key and exponent.
*/

TPM_RESULT TPM_StorePrivkey_Convert(TPM_STORE_ASYMKEY *tpm_store_asymkey,	/* I/O result */
				    TPM_KEY_PARMS *tpm_key_parms,	/* to get exponent */
				    TPM_SIZED_BUFFER *pubKey)		/* to get public key */
{
    TPM_RESULT	rc = 0;
    /* computed data */
    unsigned char	*narr;
    unsigned char	*earr;
    unsigned char	*parr;		
    unsigned char	*qarr = NULL;
    unsigned char	*darr = NULL;
    uint32_t		nbytes;
    uint32_t		ebytes;
    uint32_t		pbytes;
    uint32_t		qbytes;
    uint32_t		dbytes;

    
    printf(" TPM_StorePrivkey_Convert:\n");
    if (rc == 0) {
	TPM_PrintFour("  TPM_StorePrivkey_Convert: p",	tpm_store_asymkey->privKey.p_key.buffer);
	nbytes = pubKey->size;
	narr = pubKey->buffer;
	rc = TPM_KeyParms_GetExponent(&ebytes, &earr, tpm_key_parms);
    }
    if (rc == 0) {
	rc = TPM_StoreAsymkey_GetPrimeFactorP(&pbytes, &parr, tpm_store_asymkey);
    }
    if (rc == 0) {
	rc = TPM_RSAGetPrivateKey(&qbytes, &qarr,	/* freed @1 */
				  &dbytes, &darr,	/* freed @2 */
				  nbytes, narr,
				  ebytes, earr,
				  pbytes, parr);
    }
    if (rc == 0) {
	TPM_PrintFour("  TPM_StorePrivkey_Convert: q", qarr);
	TPM_PrintFour("  TPM_StorePrivkey_Convert: d", darr);
	rc = TPM_SizedBuffer_Set((&(tpm_store_asymkey->privKey.q_key)), qbytes, qarr);
    }
    if (rc == 0) {
	rc = TPM_SizedBuffer_Set((&(tpm_store_asymkey->privKey.d_key)), dbytes, darr);
    }
    free(qarr); /* @1 */
    free(darr); /* @2 */
    return rc;
}

/* TPM_StorePrivkey_Store serializes a TPM_STORE_PRIVKEY structure, appending results to 'sbuffer'

   Only the prime factor p is stored.  The other prime factor q and the private key d are
   recalculated after a load.
 */

TPM_RESULT TPM_StorePrivkey_Store(TPM_STORE_BUFFER *sbuffer,
				  const TPM_STORE_PRIVKEY *tpm_store_privkey)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_StorePrivkey_Store:\n");
    if (rc == 0) {
	TPM_PrintFour("  TPM_StorePrivkey_Store: p",  tpm_store_privkey->p_key.buffer);
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_store_privkey->p_key));
    }
    return rc;
}

void TPM_StorePrivkey_Delete(TPM_STORE_PRIVKEY *tpm_store_privkey)
{
    printf(" TPM_StorePrivkey_Delete:\n");
    if (tpm_store_privkey != NULL) {
	TPM_SizedBuffer_Zero(&(tpm_store_privkey->d_key));
	TPM_SizedBuffer_Zero(&(tpm_store_privkey->p_key));
	TPM_SizedBuffer_Zero(&(tpm_store_privkey->q_key));
	
	TPM_SizedBuffer_Delete(&(tpm_store_privkey->d_key));
	TPM_SizedBuffer_Delete(&(tpm_store_privkey->p_key));
	TPM_SizedBuffer_Delete(&(tpm_store_privkey->q_key));
	TPM_StorePrivkey_Init(tpm_store_privkey);
    }
    return;
}

/*
  TPM_PUBKEY
*/

void TPM_Pubkey_Init(TPM_PUBKEY *tpm_pubkey)
{
    printf(" TPM_Pubkey_Init:\n");
    TPM_KeyParms_Init(&(tpm_pubkey->algorithmParms));
    TPM_SizedBuffer_Init(&(tpm_pubkey->pubKey));
    return;
}

TPM_RESULT TPM_Pubkey_Load(TPM_PUBKEY *tpm_pubkey,	/* result */
			   unsigned char **stream,	/* pointer to next parameter */
			   uint32_t *stream_size)		/* stream size left */
{
    TPM_RESULT	rc = 0;

    printf(" TPM_Pubkey_Load:\n");
    /* load algorithmParms */
    if (rc == 0) {
	rc = TPM_KeyParms_Load(&(tpm_pubkey->algorithmParms), stream, stream_size);
    }
    /* load pubKey */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_pubkey->pubKey), stream, stream_size);
    }
    return rc;
}

/* TPM_Pubkey_Store serializes a TPM_PUBKEY structure, appending results to 'sbuffer'
*/

TPM_RESULT TPM_Pubkey_Store(TPM_STORE_BUFFER *sbuffer,
			    TPM_PUBKEY *tpm_pubkey)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_Pubkey_Store:\n");
    if (rc == 0) {
	rc = TPM_KeyParms_Store(sbuffer, &(tpm_pubkey->algorithmParms));
    }
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_pubkey->pubKey));
    }
    return rc;
}

void TPM_Pubkey_Delete(TPM_PUBKEY *tpm_pubkey)
{
    printf(" TPM_Pubkey_Delete:\n");
    if (tpm_pubkey != NULL) {
	TPM_KeyParms_Delete(&(tpm_pubkey->algorithmParms));
	TPM_SizedBuffer_Delete(&(tpm_pubkey->pubKey));
	TPM_Pubkey_Init(tpm_pubkey);
    }
    return;
}

TPM_RESULT TPM_Pubkey_Set(TPM_PUBKEY *tpm_pubkey,
			  TPM_KEY *tpm_key)
{
    TPM_RESULT	rc = 0;
    
    printf(" TPM_Pubkey_Set:\n");
    if (rc == 0) {
	/* add TPM_KEY_PARMS algorithmParms */
	rc = TPM_KeyParms_Copy(&(tpm_pubkey->algorithmParms),
			       &(tpm_key->algorithmParms));
    }
    if (rc == 0) {
	/* add TPM_SIZED_BUFFER pubKey */
	rc = TPM_SizedBuffer_Copy(&(tpm_pubkey->pubKey),
				  &(tpm_key->pubKey));
    }				
    return rc;
}

TPM_RESULT TPM_Pubkey_Copy(TPM_PUBKEY *dest_tpm_pubkey,
			   TPM_PUBKEY *src_tpm_pubkey)
{
    TPM_RESULT	rc = 0;
    
    printf(" TPM_Pubkey_Copy:\n");
    /* copy TPM_KEY_PARMS algorithmParms */
    if (rc == 0) {
	rc = TPM_KeyParms_Copy(&(dest_tpm_pubkey->algorithmParms),
			       &(src_tpm_pubkey->algorithmParms));
    }
    /* copy TPM_SIZED_BUFFER pubKey */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Copy(&(dest_tpm_pubkey->pubKey),
				  &(src_tpm_pubkey->pubKey));
    }				
    return rc;
   
}

/* TPM_Pubkey_GetExponent() gets the exponent key from the TPM_RSA_KEY_PARMS contained in a
   TPM_PUBKEY
*/

TPM_RESULT TPM_Pubkey_GetExponent(uint32_t	*ebytes,
				  unsigned char **earr,
				  TPM_PUBKEY	*tpm_pubkey)
{
    TPM_RESULT		rc = 0;
    
    printf(" TPM_Pubkey_GetExponent:\n");
    if (rc == 0) {
	rc = TPM_KeyParms_GetExponent(ebytes, earr, &(tpm_pubkey->algorithmParms));
    }
    return rc;
}

/* TPM_Pubkey_GetPublicKey() gets the public key from the TPM_PUBKEY
 */

TPM_RESULT TPM_Pubkey_GetPublicKey(uint32_t		*nbytes,
				   unsigned char	**narr,
				   TPM_PUBKEY		*tpm_pubkey)
{
    TPM_RESULT	rc = 0;
    
    printf(" TPM_Pubkey_GetPublicKey:\n");
    if (rc == 0) {
	*nbytes = tpm_pubkey->pubKey.size;
	*narr = tpm_pubkey->pubKey.buffer;
    }
    return rc;
}

/*
  TPM_RSA_KEY_PARMS
*/
  

/* Allocates and loads a TPM_RSA_KEY_PARMS structure

   Must be delete'd and freed by the caller.
*/

void TPM_RSAKeyParms_Init(TPM_RSA_KEY_PARMS *tpm_rsa_key_parms)
{
    printf(" TPM_RSAKeyParms_Init:\n");
    tpm_rsa_key_parms->keyLength = 0;
    tpm_rsa_key_parms->numPrimes = 0;
    TPM_SizedBuffer_Init(&(tpm_rsa_key_parms->exponent));
    return;
}

/* TPM_RSAKeyParms_Load() sets members from stream, and shifts the stream past the bytes consumed.

   Must call TPM_RSAKeyParms_Delete() to free
*/

TPM_RESULT TPM_RSAKeyParms_Load(TPM_RSA_KEY_PARMS *tpm_rsa_key_parms,	/* result */
				unsigned char **stream,		/* pointer to next parameter */ 
				uint32_t *stream_size)		/* stream size left */
{
    TPM_RESULT	rc = 0;

    printf(" TPM_RSAKeyParms_Load:\n");
    /* load keyLength */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_rsa_key_parms->keyLength), stream, stream_size);
    }
    /* load numPrimes */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_rsa_key_parms->numPrimes), stream, stream_size);
    }
    /* load exponent */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_rsa_key_parms->exponent), stream, stream_size); 
    }
    return rc;
}

/* TPM_RSAKeyParms_Store serializes a TPM_RSA_KEY_PARMS structure, appending results to 'sbuffer'
*/

TPM_RESULT TPM_RSAKeyParms_Store(TPM_STORE_BUFFER *sbuffer,
				 const TPM_RSA_KEY_PARMS *tpm_rsa_key_parms)
{
    TPM_RESULT	rc = 0;
    
    printf(" TPM_RSAKeyParms_Store:\n");
    /* store keyLength */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_rsa_key_parms->keyLength); 
    }
    /* store numPrimes */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_rsa_key_parms->numPrimes); 
    }
    /* store exponent */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_rsa_key_parms->exponent)); 
    }
    return rc;
}

/* TPM_RSAKeyParms_Delete frees any member allocated memory

   If 'tpm_rsa_key_parms' is NULL, this is a no-op.
 */

void TPM_RSAKeyParms_Delete(TPM_RSA_KEY_PARMS *tpm_rsa_key_parms)
{
    printf(" TPM_RSAKeyParms_Delete:\n");
    if (tpm_rsa_key_parms != NULL) {
	TPM_SizedBuffer_Delete(&(tpm_rsa_key_parms->exponent));
	TPM_RSAKeyParms_Init(tpm_rsa_key_parms);
    }
    return;
}

/* TPM_RSAKeyParms_Copy() does a copy of the source to the destination.

   The destination must be initialized first.
*/

TPM_RESULT TPM_RSAKeyParms_Copy(TPM_RSA_KEY_PARMS *tpm_rsa_key_parms_dest,
				TPM_RSA_KEY_PARMS *tpm_rsa_key_parms_src)
{
    TPM_RESULT rc = 0;
    
    printf(" TPM_RSAKeyParms_Copy:\n");
    if (rc == 0) {
	tpm_rsa_key_parms_dest->keyLength = tpm_rsa_key_parms_src->keyLength;
	tpm_rsa_key_parms_dest->numPrimes = tpm_rsa_key_parms_src->numPrimes;
	rc = TPM_SizedBuffer_Copy(&(tpm_rsa_key_parms_dest->exponent),
				  &(tpm_rsa_key_parms_src->exponent));
    }
    return rc;
}

/* TPM_RSAKeyParms_New() allocates memory for a TPM_RSA_KEY_PARMS and initializes the structure */

TPM_RESULT TPM_RSAKeyParms_New(TPM_RSA_KEY_PARMS **tpm_rsa_key_parms)
{
    TPM_RESULT rc = 0;

    printf(" TPM_RSAKeyParms_New:\n");
    if (rc == 0) {
	rc = TPM_Malloc((unsigned char **)tpm_rsa_key_parms, sizeof(TPM_RSA_KEY_PARMS));
    }	 
    if (rc == 0) {
	TPM_RSAKeyParms_Init(*tpm_rsa_key_parms);
    }	 
    return rc;
}

/* TPM_RSAKeyParms_GetExponent() gets the exponent array and size from tpm_rsa_key_parms.

   If the structure exponent.size is zero, the default RSA exponent is returned.
*/

TPM_RESULT TPM_RSAKeyParms_GetExponent(uint32_t		*ebytes,
				       unsigned char	**earr,
				       TPM_RSA_KEY_PARMS *tpm_rsa_key_parms)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_RSAKeyParms_GetExponent:\n");
    if (tpm_rsa_key_parms->exponent.size != 0) {
	*ebytes = tpm_rsa_key_parms->exponent.size;
	*earr = tpm_rsa_key_parms->exponent.buffer;
    }
    else {
	*ebytes = 3;
	*earr = tpm_default_rsa_exponent;
    }
    return rc;
}

/*
  A Key Handle Entry
*/

/* TPM_KeyHandleEntry_Init() removes an entry from the list.  It DOES NOT delete the
   TPM_KEY object. */

void TPM_KeyHandleEntry_Init(TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entry)
{
    tpm_key_handle_entry->handle = 0;
    tpm_key_handle_entry->key = NULL;
    tpm_key_handle_entry->parentPCRStatus = TRUE;
    tpm_key_handle_entry->keyControl = 0;
    return;
}

/* TPM_KeyHandleEntry_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_KeyHandleEntry_Init()
   After use, call TPM_KeyHandleEntry_Delete() to free memory
*/

TPM_RESULT TPM_KeyHandleEntry_Load(TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entry,
				   unsigned char **stream,
				   uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_KeyHandleEntry_Load:\n");
    /* load handle */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_key_handle_entry->handle), stream, stream_size); 
    }
    /* malloc space for the key member */
    if (rc == 0) {
	rc = TPM_Malloc((unsigned char **)&(tpm_key_handle_entry->key), sizeof(TPM_KEY));
    }
    /* load key */
    if (rc == 0) {
	TPM_Key_Init(tpm_key_handle_entry->key);
	rc = TPM_Key_LoadClear(tpm_key_handle_entry->key,
			       FALSE,			/* not EK */
			       stream, stream_size);
    }
    /* load parentPCRStatus */
    if (rc == 0) {
	rc = TPM_LoadBool(&(tpm_key_handle_entry->parentPCRStatus), stream, stream_size); 
    }
    /* load keyControl */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_key_handle_entry->keyControl), stream, stream_size); 
    }
    return rc;
}

/* TPM_KeyHandleEntry_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_KeyHandleEntry_Store(TPM_STORE_BUFFER *sbuffer,
				    const TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entry)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_KeyHandleEntry_Store:\n");
    /* store handle */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_key_handle_entry->handle); 
    }
    /* store key with private data appended in clear text */
    if (rc == 0) {
	rc = TPM_Key_StoreClear(sbuffer,
				FALSE,		/* not EK */
				tpm_key_handle_entry->key);
    }
    /* store parentPCRStatus */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer,
				&(tpm_key_handle_entry->parentPCRStatus), sizeof(TPM_BOOL)); 
    }
    /* store keyControl */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_key_handle_entry->keyControl); 
    }
    return rc;
}

/* TPM_KeyHandleEntry_Delete() deletes an entry from the list, deletes the TPM_KEY object, and
   free's the TPM_KEY.
*/

void TPM_KeyHandleEntry_Delete(TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entry)
{
    if (tpm_key_handle_entry != NULL) {
	if (tpm_key_handle_entry->handle != 0) {
	    printf(" TPM_KeyHandleEntry_Delete: Deleting %08x\n", tpm_key_handle_entry->handle);
	    TPM_Key_Delete(tpm_key_handle_entry->key);
	    free(tpm_key_handle_entry->key);
	}
	TPM_KeyHandleEntry_Init(tpm_key_handle_entry);
    }
    return;
}

/* TPM_KeyHandleEntry_FlushSpecific() flushes a key handle according to the rules of
   TPM_FlushSpecific()
*/

TPM_RESULT TPM_KeyHandleEntry_FlushSpecific(tpm_state_t *tpm_state,
					    TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entry)
{
    TPM_RESULT		rc = 0;
    TPM_AUTHHANDLE	authHandle = 0;		/* dummy parameter */
    TPM_BOOL		continueAuthSession;	/* dummy parameter */
    
    printf(" TPM_KeyHandleEntry_FlushSpecific:\n");
    if (rc == 0) {
	/* Internal error, should never happen */
	if (tpm_key_handle_entry->key == NULL) {
	    printf("TPM_KeyHandleEntry_FlushSpecific: Error (fatal), key is NULL\n");
	    rc = TPM_FAIL;
	}
    }
    /* terminate OSAP and DSAP sessions associated with the key */
    if (rc == 0) {
	/* The dummy parameters are not used.  The session, if any, associated with this function
	   is handled elsewhere. */
	TPM_AuthSessions_TerminateEntity(&continueAuthSession,
					 authHandle,
					 tpm_state->tpm_stclear_data.authSessions,
					 TPM_ET_KEYHANDLE,		/* TPM_ENTITY_TYPE */
					 &(tpm_key_handle_entry->key->
					   tpm_store_asymkey->pubDataDigest)); /* entityDigest */
	printf(" TPM_KeyHandleEntry_FlushSpecific: Flushing key handle %08x\n",
	       tpm_key_handle_entry->handle);
	/* free the TPM_KEY resources, free the key itself, and remove entry from the key handle
	   entries list */
	TPM_KeyHandleEntry_Delete(tpm_key_handle_entry);
    }
    return rc;
}

/*
  Key Handle Entries
*/

/* TPM_KeyHandleEntries_Init() initializes the fixed TPM_KEY_HANDLE_ENTRY array.  All entries are
   emptied.  The keys are not deleted.
*/

void TPM_KeyHandleEntries_Init(TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries)
{
    size_t i;
    
    printf(" TPM_KeyHandleEntries_Init:\n");
    for (i = 0 ; i < TPM_KEY_HANDLES ; i++) {
	TPM_KeyHandleEntry_Init(&(tpm_key_handle_entries[i]));
    }
    return;
}

/* TPM_KeyHandleEntries_Delete() deletes and freed all TPM_KEY's stored in entries, and the entry

*/

void TPM_KeyHandleEntries_Delete(TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries)
{
    size_t i;
    
    printf(" TPM_KeyHandleEntries_Delete:\n");
    for (i = 0 ; i < TPM_KEY_HANDLES ; i++) {
	TPM_KeyHandleEntry_Delete(&(tpm_key_handle_entries[i]));
    }
    return;
}

/* TPM_KeyHandleEntries_Load() loads the key handle entries from a stream created by
   TPM_KeyHandleEntries_Store()

   The two functions must be kept in sync.
*/

TPM_RESULT TPM_KeyHandleEntries_Load(tpm_state_t *tpm_state,
				     unsigned char **stream,
				     uint32_t *stream_size)
{
    TPM_RESULT			rc = 0;
    uint32_t			keyCount = 0;			/* keys to be saved */
    size_t			i;
    TPM_KEY_HANDLE_ENTRY	tpm_key_handle_entry;

    /* check format tag */
    /* In the future, if multiple formats are supported, this check will be replaced by a 'switch'
       on the tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_KEY_HANDLE_ENTRIES_V1, stream, stream_size);
    }
    /* get the count of keys in the stream */
    if (rc == 0) {
	rc = TPM_Load32(&keyCount, stream, stream_size);
	printf("  TPM_KeyHandleEntries_Load: %u keys to be loaded\n", keyCount);
    }
    /* sanity check that keyCount not greater than key slots */
    if (rc == 0) {
	if (keyCount > TPM_KEY_HANDLES) {
	    printf("TPM_KeyHandleEntries_Load: Error (fatal)"
		   " key handles in stream %u greater than %d\n",
		   keyCount, TPM_KEY_HANDLES);
	    rc = TPM_FAIL;
	}
    }    
    /* for each key handle entry */
    for (i = 0 ; (rc == 0) && (i < keyCount) ; i++) {
	/* deserialize the key handle entry and its key member */
	if (rc == 0) {
	    TPM_KeyHandleEntry_Init(&tpm_key_handle_entry);	/* freed @2 on error */
	    rc = TPM_KeyHandleEntry_Load(&tpm_key_handle_entry, stream, stream_size);
	}
	if (rc == 0) {
	    printf("  TPM_KeyHandleEntries_Load: Loading key handle %08x\n",
		   tpm_key_handle_entry.handle);
	    /* Add the entry to the list.  Keep the handle.  If the suggested value could not be
	       accepted, this is a "should never happen" fatal error.  It means that the save key
	       handle was saved twice.	*/
	    rc = TPM_KeyHandleEntries_AddEntry(&(tpm_key_handle_entry.handle), 	/* suggested */
					       TRUE,				/* keep handle */
					       tpm_state->tpm_key_handle_entries,
					       &tpm_key_handle_entry);
	}
	/* if there was an error copying the entry to the array, the entry must be delete'd to
	   prevent a memory leak, since a key has been loaded to the entry */
	if (rc != 0) {
	    TPM_KeyHandleEntry_Delete(&tpm_key_handle_entry);	/* @2 on error */
	}	
    }
    return rc;
}

/* TPM_KeyHandleEntries_Store() stores the key handle entries to a stream that can be restored
   through TPM_KeyHandleEntries_Load().

   The two functions must be kept in sync.
*/

TPM_RESULT TPM_KeyHandleEntries_Store(TPM_STORE_BUFFER *sbuffer,
				      tpm_state_t *tpm_state)
{
    TPM_RESULT			rc = 0;
    size_t			start;		/* iterator though key handle entries */
    size_t			current;	/* iterator though key handle entries */
    uint32_t			keyCount;	/* keys to be saved */
    TPM_BOOL 			save;		/* should key be saved */
    TPM_KEY_HANDLE_ENTRY	*tpm_key_handle_entry;
    
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_KEY_HANDLE_ENTRIES_V1);
    }
    /* first count up the keys */
    if (rc == 0) {
	start = 0;
	keyCount = 0;
	printf("  TPM_KeyHandleEntries_Store: Counting keys to be stored\n");
    }
    while ((rc == 0) &&
	   /* returns TPM_RETRY when at the end of the table, terminates loop */
	   (TPM_KeyHandleEntries_GetNextEntry(&tpm_key_handle_entry,
					      &current,
					      tpm_state->tpm_key_handle_entries,
					      start)) == 0) {
	TPM_SaveState_IsSaveKey(&save, tpm_key_handle_entry);
	if (save) {
	    keyCount++;
	}
	start = current + 1;
    }
    /* store the number of entries to save */
    if (rc == 0) {
	printf("  TPM_KeyHandleEntries_Store: %u keys to be stored\n", keyCount);
	rc = TPM_Sbuffer_Append32(sbuffer, keyCount);
    }
    /* for each key handle entry */
    if (rc == 0) {
	printf("  TPM_KeyHandleEntries_Store: Storing keys\n");
	start = 0;
    }
    while ((rc == 0) &&
	   /* returns TPM_RETRY when at the end of the table, terminates loop */
	   (TPM_KeyHandleEntries_GetNextEntry(&tpm_key_handle_entry,
					      &current,
					      tpm_state->tpm_key_handle_entries,
					      start)) == 0) {
	TPM_SaveState_IsSaveKey(&save, tpm_key_handle_entry);
	if (save) {
	    /* store the key handle entry and its associated key */
	    rc = TPM_KeyHandleEntry_Store(sbuffer, tpm_key_handle_entry);
	}
	start = current + 1;
    }
    return rc;
}



/* TPM_KeyHandleEntries_StoreHandles() stores only the two members which are part of the
   specification.

   - the number of loaded keys
   - a list of key handles

   A TPM_KEY_HANDLE_LIST structure that enumerates all key handles loaded on the TPM. The list only
   contains the number of handles that an external manager can operate with and does not include the
   EK or SRK.  This is command is available for backwards compatibility. It is the same as
   TPM_CAP_HANDLE with a resource type of keys.
*/

TPM_RESULT TPM_KeyHandleEntries_StoreHandles(TPM_STORE_BUFFER *sbuffer,
					     const TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries)
{
    TPM_RESULT	rc = 0;
    uint16_t	i, loadedCount;
    
    printf(" TPM_KeyHandleEntries_StoreHandles:\n");
    if (rc == 0) {
	loadedCount = 0;
	/* count the number of loaded handles */
	for (i = 0 ; i < TPM_KEY_HANDLES ; i++) {
	    if (tpm_key_handle_entries[i].key != NULL) {
		loadedCount++;
	    }
	}
	/* store 'loaded' handle count */
	rc = TPM_Sbuffer_Append16(sbuffer, loadedCount); 
    }
    for (i = 0 ; (rc == 0) && (i < TPM_KEY_HANDLES) ; i++) {
	if (tpm_key_handle_entries[i].key != NULL) {	/* if the index is loaded */
	    rc = TPM_Sbuffer_Append32(sbuffer, tpm_key_handle_entries[i].handle); /* store it */
	}
    }
    return rc;
}

/* TPM_KeyHandleEntries_DeleteHandle() removes a handle from the list.

   The TPM_KEY object must be _Delete'd and possibly free'd separately, because it might not be in
   the table.
*/

TPM_RESULT TPM_KeyHandleEntries_DeleteHandle(TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries,
					     TPM_KEY_HANDLE tpm_key_handle)
{
    TPM_RESULT	rc = 0;
    TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entry;
    
    printf(" TPM_KeyHandleEntries_DeleteHandle: %08x\n", tpm_key_handle);
    /* search for the handle */
    if (rc == 0) {
	rc = TPM_KeyHandleEntries_GetEntry(&tpm_key_handle_entry,
					   tpm_key_handle_entries,
					   tpm_key_handle);
	if (rc != 0) {
	    printf("TPM_KeyHandleEntries_DeleteHandle: Error, key handle %08x not found\n",
		   tpm_key_handle);
	}
    }
    /* delete the entry */
    if (rc == 0) {
	TPM_KeyHandleEntry_Init(tpm_key_handle_entry);
    }
    return rc;
}

/* TPM_KeyHandleEntries_IsSpace() returns 'isSpace' TRUE if an entry is available, FALSE if not.

   If TRUE, 'index' holds the first free position.
*/

void TPM_KeyHandleEntries_IsSpace(TPM_BOOL *isSpace,
				  uint32_t *index,
				  const TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries)
{
    printf(" TPM_KeyHandleEntries_IsSpace:\n");
    for (*index = 0, *isSpace = FALSE ; *index < TPM_KEY_HANDLES ; (*index)++) {
	if (tpm_key_handle_entries[*index].key == NULL) {	/* if the index is empty */
	    printf("  TPM_KeyHandleEntries_IsSpace: Found space at %u\n", *index);
	    *isSpace = TRUE;
	    break;
	}
    }
    return;
}

/* TPM_KeyHandleEntries_GetSpace() returns the number of unused key handle entries.

*/

void TPM_KeyHandleEntries_GetSpace(uint32_t *space,
				   const TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries)
{
    uint32_t i;

    printf(" TPM_KeyHandleEntries_GetSpace:\n");
    for (*space = 0 , i = 0 ; i < TPM_KEY_HANDLES ; i++) {
	if (tpm_key_handle_entries[i].key == NULL) {	/* if the index is empty */
	    (*space)++;
	}	    
    }
    return;
}

/* TPM_KeyHandleEntries_IsEvictSpace() returns 'isSpace' TRUE if there are at least 'minSpace'
   entries that do not have the ownerEvict bit set, FALSE if not.
*/

void TPM_KeyHandleEntries_IsEvictSpace(TPM_BOOL *isSpace,
				       const TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries,
				       uint32_t minSpace)
{
    uint32_t evictSpace;
    uint32_t i;

    for (i = 0,	 evictSpace = 0 ; i < TPM_KEY_HANDLES ; i++) {
	if (tpm_key_handle_entries[i].key == NULL) {	/* if the index is empty */
	    evictSpace++;
	}
	else {							/* is index is used */
	    if (!(tpm_key_handle_entries[i].keyControl & TPM_KEY_CONTROL_OWNER_EVICT)) {
		evictSpace++;	/* space that can be evicted */
	    }
	}
    }
    printf(" TPM_KeyHandleEntries_IsEvictSpace: evictable space, minimum %u free %u\n",
	   minSpace, evictSpace);
    if (evictSpace >= minSpace) {
	*isSpace = TRUE;
    }
    else {
	*isSpace = FALSE;
    }
    return;
}

/* TPM_KeyHandleEntries_AddKeyEntry() adds a TPM_KEY object to the list.

   If *tpm_key_handle == 0, a value is assigned.  If *tpm_key_handle != 0,
   that value is used if it it not currently in use.

   The handle is returned in tpm_key_handle.
*/

TPM_RESULT TPM_KeyHandleEntries_AddKeyEntry(TPM_KEY_HANDLE *tpm_key_handle,		/* i/o */
					    TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries, /* in */
					    TPM_KEY *tpm_key,
					    TPM_BOOL parentPCRStatus,
					    TPM_KEY_CONTROL keyControl)
{
    TPM_RESULT			rc = 0;
    TPM_KEY_HANDLE_ENTRY	tpm_key_handle_entry;

    printf(" TPM_KeyHandleEntries_AddKeyEntry:\n");
    tpm_key_handle_entry.key = tpm_key;
    tpm_key_handle_entry.parentPCRStatus = parentPCRStatus;
    tpm_key_handle_entry.keyControl = keyControl;
    rc = TPM_KeyHandleEntries_AddEntry(tpm_key_handle,
				       FALSE,			/* don't have to keep handle */
				       tpm_key_handle_entries,
				       &tpm_key_handle_entry);
    return rc;
}

/* TPM_KeyHandleEntries_AddEntry() adds (copies) the TPM_KEY_HANDLE_ENTRY object to the list.

   If *tpm_key_handle == 0:
	a value is assigned.

   If *tpm_key_handle != 0:

	If keepHandle is TRUE, the handle must be used.	 An error is returned if the handle is
	already in use.

	If keepHandle is FALSE, if the handle is already in use, a new value is assigned.

   The handle is returned in tpm_key_handle.
*/

TPM_RESULT TPM_KeyHandleEntries_AddEntry(TPM_KEY_HANDLE *tpm_key_handle,		/* i/o */
					 TPM_BOOL keepHandle,				/* input */
					 TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries,	/* input */
					 TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entry)	/* input */
					 
{
    TPM_RESULT			rc = 0;
    uint32_t			index;
    TPM_BOOL			isSpace;
    
    printf(" TPM_KeyHandleEntries_AddEntry: handle %08x, keepHandle %u\n",
	   *tpm_key_handle, keepHandle);
    /* check for valid TPM_KEY */
    if (rc == 0) {
	if (tpm_key_handle_entry->key == NULL) {	/* should never occur */
	    printf("TPM_KeyHandleEntries_AddEntry: Error (fatal), NULL TPM_KEY\n");
	    rc = TPM_FAIL;
	}
    }
    /* is there an empty entry, get the location index */
    if (rc == 0) {
	TPM_KeyHandleEntries_IsSpace(&isSpace, &index, tpm_key_handle_entries);
	if (!isSpace) {
	    printf("TPM_KeyHandleEntries_AddEntry: Error, key handle entries full\n");
	    rc = TPM_NOSPACE;
	}
    }
    if (rc == 0) {
	rc = TPM_Handle_GenerateHandle(tpm_key_handle,			/* I/O */
				       tpm_key_handle_entries,		/* handle array */
				       keepHandle,
				       TRUE,				/* isKeyHandle */
				       (TPM_GETENTRY_FUNCTION_T)TPM_KeyHandleEntries_GetEntry);
    }
    if (rc == 0) {
	tpm_key_handle_entries[index].handle = *tpm_key_handle;
	tpm_key_handle_entries[index].key = tpm_key_handle_entry->key;
	tpm_key_handle_entries[index].keyControl = tpm_key_handle_entry->keyControl;
	tpm_key_handle_entries[index].parentPCRStatus = tpm_key_handle_entry->parentPCRStatus;
	printf("  TPM_KeyHandleEntries_AddEntry: Index %u key handle %08x key pointer %p\n",
	       index, tpm_key_handle_entries[index].handle, tpm_key_handle_entries[index].key);
    }
    return rc;
}

/* TPM_KeyHandleEntries_GetEntry() searches all entries for the entry matching the handle, and
   returns that entry */

TPM_RESULT TPM_KeyHandleEntries_GetEntry(TPM_KEY_HANDLE_ENTRY **tpm_key_handle_entry,
					 TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries,
					 TPM_KEY_HANDLE tpm_key_handle)
{
    TPM_RESULT	rc = 0;
    size_t	i;
    TPM_BOOL	found;

    printf(" TPM_KeyHandleEntries_GetEntry: Get entry for handle %08x\n", tpm_key_handle);
    for (i = 0, found = FALSE ; (i < TPM_KEY_HANDLES) && !found ; i++) {
	/* first test for matching handle.  Then check for non-NULL to insure that entry is valid */
	if ((tpm_key_handle_entries[i].handle == tpm_key_handle) &&
	    tpm_key_handle_entries[i].key != NULL) {	/* found */
	    found = TRUE;
	    *tpm_key_handle_entry = &(tpm_key_handle_entries[i]);
	}
    }
    if (!found) {
	printf("  TPM_KeyHandleEntries_GetEntry: key handle %08x not found\n", tpm_key_handle);
	rc = TPM_INVALID_KEYHANDLE;
    }
    else {
	printf("  TPM_KeyHandleEntries_GetEntry: key handle %08x found\n", tpm_key_handle);
    }
    return rc;
}

/* TPM_KeyHandleEntries_GetNextEntry() gets the next valid TPM_KEY_HANDLE_ENTRY at or after the
   'start' index.

   The current position is returned in 'current'.  For iteration, the next 'start' should be
   'current' + 1.

   Returns

   0 on success.
   Returns TPM_RETRY when no more valid entries are found.
 */

TPM_RESULT TPM_KeyHandleEntries_GetNextEntry(TPM_KEY_HANDLE_ENTRY **tpm_key_handle_entry,
					     size_t *current,
					     TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries,
					     size_t start)
{
    TPM_RESULT	rc = TPM_RETRY;

    printf(" TPM_KeyHandleEntries_GetNextEntry: Start %lu\n", (unsigned long)start);
    for (*current = start ; *current < TPM_KEY_HANDLES ; (*current)++) {
	if (tpm_key_handle_entries[*current].key != NULL) {
	    *tpm_key_handle_entry = &(tpm_key_handle_entries[*current]);
	    rc = 0;	/* found an entry */
	    break;
	}
    }
    return rc;
}

/* TPM_KeyHandleEntries_GetKey() gets the TPM_KEY associated with the handle.

   If the key has PCR usage (size is non-zero and one or more mask bits are set), PCR's have been
   specified.  It computes a PCR digest based on the TPM PCR's and verifies it against the key
   digestAtRelease.
   
   Exceptions: readOnly is TRUE when the caller is indicating that only the public key is being read
   (e.g. TPM_GetPubKey).  In this case, if keyFlags TPM_PCRIGNOREDONREAD is also TRUE, the PCR
   digest and locality must not be checked.

   If ignorePCRs is TRUE, the PCR digest is also ignored.  A typical case is during OSAP and DSAP
   session setup.
 */

TPM_RESULT TPM_KeyHandleEntries_GetKey(TPM_KEY **tpm_key,
				       TPM_BOOL *parentPCRStatus,
				       tpm_state_t *tpm_state,
				       TPM_KEY_HANDLE tpm_key_handle,
				       TPM_BOOL readOnly,
				       TPM_BOOL ignorePCRs,
				       TPM_BOOL allowEK)
{
    TPM_RESULT		rc = 0;
    TPM_BOOL		found = FALSE;	/* found a special handle key */
    TPM_BOOL		validatePcrs = TRUE;
    TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entry;

    printf(" TPM_KeyHandleEntries_GetKey: For handle %08x\n", tpm_key_handle);
    /* If it's one of the special handles, return the TPM_KEY */
    if (rc == 0) {
	switch (tpm_key_handle) {
	  case TPM_KH_SRK:	/* The handle points to the SRK */
	    if (tpm_state->tpm_permanent_data.ownerInstalled) {
		*tpm_key = &(tpm_state->tpm_permanent_data.srk);
		*parentPCRStatus = FALSE;	/* storage root key (SRK) has no parent */
		found = TRUE;
	    }
	    else {
		printf(" TPM_KeyHandleEntries_GetKey: Error, SRK handle with no owner\n");
		rc = TPM_KEYNOTFOUND;
	    }
	    break;
	  case TPM_KH_EK:	/* The handle points to the PUBEK, only usable with
				   TPM_OwnerReadInternalPub */
	    if (rc == 0) {
		if (!allowEK) {
		    printf(" TPM_KeyHandleEntries_GetKey: Error, EK handle not allowed\n");
		    rc = TPM_KEYNOTFOUND;
		}
	    }
	    if (rc == 0) {
		if (tpm_state->tpm_permanent_data.endorsementKey.keyUsage ==
		    TPM_KEY_UNINITIALIZED) {
		    printf(" TPM_KeyHandleEntries_GetKey: Error, EK handle but no EK\n");
		    rc = TPM_KEYNOTFOUND;
		}
	    }
	    if (rc == 0) {
		*tpm_key = &(tpm_state->tpm_permanent_data.endorsementKey);
		*parentPCRStatus = FALSE;	/* endorsement key (EK) has no parent */
		found = TRUE;
	    }
	    break;
	  case TPM_KH_OWNER:	/* handle points to the TPM Owner */
	  case TPM_KH_REVOKE:	/* handle points to the RevokeTrust value */
	  case TPM_KH_TRANSPORT: /* handle points to the EstablishTransport static authorization */
	  case TPM_KH_OPERATOR: /* handle points to the Operator auth */
	  case TPM_KH_ADMIN:	/* handle points to the delegation administration auth */
	    printf("TPM_KeyHandleEntries_GetKey: Error, Unsupported key handle %08x\n",
		   tpm_key_handle);
	    rc = TPM_INVALID_RESOURCE;
	    break;
	  default:
	    /* continue searching */
	    break;
	}
    }
    /* If not one of the special key handles, search for the handle in the list */
    if ((rc == 0) && !found) {
	rc = TPM_KeyHandleEntries_GetEntry(&tpm_key_handle_entry,
					   tpm_state->tpm_key_handle_entries,
					   tpm_key_handle);
	if (rc != 0) {
	    printf("TPM_KeyHandleEntries_GetKey: Error, key handle %08x not found\n",
		   tpm_key_handle);
	}
    }
    /* Part 1 25.1 Validate Key for use 
       2. Set LK to the loaded key that is being used */
    /* NOTE:  For special handle keys, this was already done.  Just do here for keys in table */
    if ((rc == 0) && !found) {
	*tpm_key = tpm_key_handle_entry->key;
	*parentPCRStatus = tpm_key_handle_entry->parentPCRStatus;
    }
    /* 3. If LK -> pcrInfoSize is not 0 - if the key specifies PCR's */
    /* NOTE Done by TPM_Key_CheckPCRDigest() */
    /* a. If LK -> pcrInfo -> releasePCRSelection identifies the use of one or more PCR */
    if (rc == 0) {
#ifdef TPM_V12
	validatePcrs = !ignorePCRs &&
		       !(readOnly && ((*tpm_key)->keyFlags & TPM_PCRIGNOREDONREAD));
#else
	validatePcrs = !ignorePCRs && !readOnly;
#endif
    }
    if ((rc == 0) && validatePcrs) {
	if (rc == 0) {
	    rc = TPM_Key_CheckPCRDigest(*tpm_key, tpm_state);
	}
    }
    return rc;
}

/* TPM_KeyHandleEntries_SetParentPCRStatus() updates the parentPCRStatus member of the
   TPM_KEY_HANDLE_ENTRY */

TPM_RESULT TPM_KeyHandleEntries_SetParentPCRStatus(TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries,
						   TPM_KEY_HANDLE tpm_key_handle,
						   TPM_BOOL parentPCRStatus)
{
    TPM_RESULT	rc = 0;
    TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entry;

    printf(" TPM_KeyHandleEntries_SetParentPCRStatus: Handle %08x\n", tpm_key_handle);
    /* get the entry for the handle from the table */
    if (rc == 0) {
	rc = TPM_KeyHandleEntries_GetEntry(&tpm_key_handle_entry,
					   tpm_key_handle_entries,
					   tpm_key_handle);
	if (rc != 0) {
	    printf("TPM_KeyHandleEntries_SetParentPCRStatus: Error, key handle %08x not found\n",
		   tpm_key_handle);
	}
    }
    if (rc == 0) {
	tpm_key_handle_entry->parentPCRStatus = parentPCRStatus;
    }
    return rc;
}

/* TPM_KeyHandleEntries_OwnerEvictLoad() loads all owner evict keys from the stream into the key
   handle entries table.
*/

TPM_RESULT TPM_KeyHandleEntries_OwnerEvictLoad(TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries,
					       unsigned char **stream,
					       uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;
    uint16_t 		keyCount;
    uint16_t		i;		/* the uint16_t corresponds to the standard getcap */
    TPM_KEY_HANDLE_ENTRY tpm_key_handle_entry;	/* each entry as read from the stream */
    TPM_TAG		ownerEvictVersion;

    printf(" TPM_KeyHandleEntries_OwnerEvictLoad:\n");
    /* get the owner evict version number */
    if (rc == 0) {
	rc = TPM_Load16(&ownerEvictVersion, stream, stream_size); 
    }
    if (rc == 0) {
	if (ownerEvictVersion != TPM_TAG_NVSTATE_OE_V1) {
	    printf("TPM_KeyHandleEntries_OwnerEvictLoad: "
		   "Error (fatal) unsupported version tag %04x\n",
		   ownerEvictVersion);
	    rc = TPM_FAIL;
	}
    }
    /* get the count of owner evict keys in the stream */
    if (rc == 0) {
	rc = TPM_Load16(&keyCount, stream, stream_size); 
    }
    /* sanity check that keyCount not greater than key slots */
    if (rc == 0) {
	if (keyCount > TPM_OWNER_EVICT_KEY_HANDLES) {
	    printf("TPM_KeyHandleEntries_OwnerEvictLoad: Error (fatal)"
		   " key handles in stream %u greater than %d\n",
		   keyCount, TPM_OWNER_EVICT_KEY_HANDLES);
	    rc = TPM_FAIL;
	}
    }    
    if (rc == 0) {
	printf("  TPM_KeyHandleEntries_OwnerEvictLoad: Count %hu\n", keyCount);
    }
    for (i = 0 ; (rc == 0) && (i < keyCount) ; i++) {
	/* Must init each time through.  This just resets the structure members.  It does not free
	   the key that is in the structure after the first time through.  That key has been added
	   (copied) to the key handle entries array. */
	printf("  TPM_KeyHandleEntries_OwnerEvictLoad: Loading key %hu\n", i);
	TPM_KeyHandleEntry_Init(&tpm_key_handle_entry);	/* freed @2 on error */
	if (rc == 0) {
	    rc = TPM_KeyHandleEntry_Load(&tpm_key_handle_entry, stream, stream_size);
	}
	/* add the entry to the list */
	if (rc == 0) {
	    rc = TPM_KeyHandleEntries_AddEntry(&(tpm_key_handle_entry.handle),	/* suggested */
					       TRUE,				/* keep handle */
					       tpm_key_handle_entries,
					       &tpm_key_handle_entry);
	}
	/* if there was an error copying the entry to the array, the entry must be delete'd to
	   prevent a memory leak, since a key has been loaded to the entry */
	if (rc != 0) {
	    TPM_KeyHandleEntry_Delete(&tpm_key_handle_entry);	/* @2 on error */
	}	
    }
    return rc;
}
    
/* TPM_KeyHandleEntries_OwnerEvictStore() stores all owner evict keys from the key handle entries
   table to the stream.

   It is used to serialize to NVRAM.
*/

TPM_RESULT TPM_KeyHandleEntries_OwnerEvictStore(TPM_STORE_BUFFER *sbuffer,
						const TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries)
{
    TPM_RESULT	rc = 0;
    uint16_t 	count;
    uint16_t	i;		/* the uint16_t corresponds to the standard getcap */

    printf(" TPM_KeyHandleEntries_OwnerEvictStore:\n");
    /* append the owner evict version number to the stream */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_NVSTATE_OE_V1); 
    }
    /* count the number of owner evict keys */
    if (rc == 0) {
	rc = TPM_KeyHandleEntries_OwnerEvictGetCount(&count, tpm_key_handle_entries);
    }
    /* append the count to the stream */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, count); 
    }
    for (i = 0 ; (rc == 0) && (i < TPM_KEY_HANDLES) ; i++) {
	/* if the slot is occupied */
	if (tpm_key_handle_entries[i].key != NULL) {
	    /* if the key is owner evict */
	    if ((tpm_key_handle_entries[i].keyControl & TPM_KEY_CONTROL_OWNER_EVICT)) {
		/* store it */
		rc = TPM_KeyHandleEntry_Store(sbuffer, &(tpm_key_handle_entries[i]));
	    }
	}
    }
    return rc;
}

/* TPM_KeyHandleEntries_OwnerEvictGetCount returns the number of owner evict key entries
 */

TPM_RESULT
TPM_KeyHandleEntries_OwnerEvictGetCount(uint16_t *count,
					const TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries)
{
    TPM_RESULT	rc = 0;
    uint16_t	i;		/* the uint16_t corresponds to the standard getcap */

    printf(" TPM_KeyHandleEntries_OwnerEvictGetCount:\n");
    /* count the number of loaded owner evict handles */
    if (rc == 0) {
	for (i = 0 , *count = 0 ; i < TPM_KEY_HANDLES ; i++) {
	    /* if the slot is occupied */
	    if (tpm_key_handle_entries[i].key != NULL) {
		/* if the key is owner evict */
		if ((tpm_key_handle_entries[i].keyControl & TPM_KEY_CONTROL_OWNER_EVICT)) {
		    (*count)++;		/* count it */
		}
	    }
	}
	printf("  TPM_KeyHandleEntries_OwnerEvictGetCount: Count %hu\n", *count);
    }
    /* sanity check */
    if (rc == 0) {
	if (*count > TPM_OWNER_EVICT_KEY_HANDLES) {
	    printf("TPM_KeyHandleEntries_OwnerEvictGetCount: Error (fatal), "
		   "count greater that max %u\n", TPM_OWNER_EVICT_KEY_HANDLES);
	    rc = TPM_FAIL;	/* should never occur */
	}
    }
    return rc;
}

/* TPM_KeyHandleEntries_OwnerEvictDelete() flushes owner evict keys.  It does NOT write to NV.

*/

void TPM_KeyHandleEntries_OwnerEvictDelete(TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries)
{
    uint16_t	i;		/* the uint16_t corresponds to the standard getcap */

    for (i = 0 ; i < TPM_KEY_HANDLES ; i++) {
	/* if the slot is occupied */
	if (tpm_key_handle_entries[i].key != NULL) {
	    /* if the key is owner evict */
	    if ((tpm_key_handle_entries[i].keyControl & TPM_KEY_CONTROL_OWNER_EVICT)) {
		TPM_KeyHandleEntry_Delete(&(tpm_key_handle_entries[i]));
	    }
	}
    }
    return;
}

/*
  Processing Functions
*/

/* 14.4 TPM_ReadPubek rev 99

   Return the endorsement key public portion. This value should have controls placed upon access as
   it is a privacy sensitive value

   The readPubek flag is set to FALSE by TPM_TakeOwnership and set to TRUE by TPM_OwnerClear, thus
   mirroring if a TPM Owner is present.
*/

TPM_RESULT TPM_Process_ReadPubek(tpm_state_t *tpm_state,
				 TPM_STORE_BUFFER *response,
				 TPM_TAG tag,
				 uint32_t paramSize,		/* of remaining parameters*/
				 TPM_COMMAND_CODE ordinal,
				 unsigned char *command,
				 TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_NONCE		antiReplay;

    /* processing */
    const unsigned char *pubEndorsementKeyStreamBuffer;
    uint32_t		pubEndorsementKeyStreamLength;

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */

    /* output parameters */
    uint32_t		outParamStart;			/* starting point of outParam's */
    uint32_t		outParamEnd;			/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_STORE_BUFFER	pubEndorsementKeyStream;
    TPM_DIGEST		checksum;
    
    printf("TPM_Process_ReadPubek: Ordinal Entry\n");
    TPM_Sbuffer_Init(&pubEndorsementKeyStream);		/* freed @1 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get antiReplay parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Nonce_Load(antiReplay, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour(" TPM_Process_ReadPubek: antiReplay", antiReplay);
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
	    printf("TPM_Process_ReadPubek: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. If TPM_PERMANENT_FLAGS -> readPubek is FALSE return TPM_DISABLED_CMD. */
    if (returnCode == TPM_SUCCESS) {
	printf(" TPM_Process_ReadPubek: readPubek %02x\n",
	       tpm_state->tpm_permanent_flags.readPubek);
	if (!tpm_state->tpm_permanent_flags.readPubek) {
	    printf("TPM_Process_ReadPubek: Error, readPubek is FALSE\n");
	    returnCode = TPM_DISABLED_CMD;
	}
    }
    /* 2. If no EK is present the TPM MUST return TPM_NO_ENDORSEMENT */
    if (returnCode == TPM_SUCCESS) {
	if (tpm_state->tpm_permanent_data.endorsementKey.keyUsage == TPM_KEY_UNINITIALIZED) {
	    printf("TPM_Process_ReadPubek: Error, no EK is present\n");
	    returnCode = TPM_NO_ENDORSEMENT;
	}
    }
    /* 3. Create checksum by performing SHA-1 on the concatenation of (pubEndorsementKey ||
	  antiReplay). */
    if (returnCode == TPM_SUCCESS) {
	/* serialize the TPM_PUBKEY components of the EK */
	returnCode =
	    TPM_Key_StorePubkey(&pubEndorsementKeyStream,			/* output */
				&pubEndorsementKeyStreamBuffer,			/* output */
				&pubEndorsementKeyStreamLength,			/* output */
				&(tpm_state->tpm_permanent_data.endorsementKey));	/* input */
    }
    if (returnCode == TPM_SUCCESS) {
	printf(" TPM_Process_ReadPubek: pubEndorsementKey length %u\n",
	       pubEndorsementKeyStreamLength);
	/* create the checksum */
	returnCode = TPM_SHA1(checksum,
#if 0		/* The old Atmel chip and the LTC test code assume this, but it is incorrect */
			      tpm_state->tpm_permanent_data.endorsementKey.pubKey.keyLength,
			      tpm_state->tpm_permanent_data.endorsementKey.pubKey.key,
#else		/* this meets the TPM 1.2 standard */
			      pubEndorsementKeyStreamLength, pubEndorsementKeyStreamBuffer, 
#endif
			      sizeof(TPM_NONCE), antiReplay,
			      0, NULL);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_ReadPubek: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	/* 4. Export the PUBEK and checksum. */
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* append pubEndorsementKey */
	    returnCode = TPM_Sbuffer_Append(response,
					    pubEndorsementKeyStreamBuffer,
					    pubEndorsementKeyStreamLength);
	}
	/* append checksum */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Digest_Store(response, checksum);
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
    TPM_Sbuffer_Delete(&pubEndorsementKeyStream);	/* @1 */
    return rcf;
}

/* 14.2 TPM_CreateRevocableEK rev 98

   This command creates the TPM endorsement key. It returns a failure code if an endorsement key
   already exists. The TPM vendor may have a separate mechanism to create the EK and "squirt" the
   value into the TPM.
*/

TPM_RESULT TPM_Process_CreateRevocableEK(tpm_state_t *tpm_state,
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
    TPM_NONCE		antiReplay;	/* Arbitrary data */
    TPM_KEY_PARMS	keyInfo;	/* Information about key to be created, this includes all
					   algorithm parameters */
    TPM_BOOL		generateReset = FALSE;	/* If TRUE use TPM RNG to generate EKreset. If FALSE
						   use the passed value inputEKreset */
    TPM_NONCE		inputEKreset;	/* The authorization value to be used with TPM_RevokeTrust
					   if generateReset==FALSE, else the parameter is present
					   but unused */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus = FALSE;	/* audit the ordinal */
    TPM_BOOL			transportEncrypt = FALSE; 	/* wrapped in encrypted transport
								   session */
    TPM_KEY			*endorsementKey;	/* EK object from permanent store */
    TPM_BOOL			writeAllNV1 = FALSE;	/* flags to write back NV */
    TPM_BOOL			writeAllNV2 = FALSE;	/* flags to write back NV */

    /* output parameters */
    uint32_t		outParamStart;		/* starting point of outParam's */
    uint32_t		outParamEnd;		/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_PUBKEY		pubEndorsementKey;	/* The public endorsement key */
    TPM_DIGEST		checksum;		/* Hash of pubEndorsementKey and antiReplay */

    printf("TPM_Process_CreateRevocableEK: Ordinal Entry\n");
    /* get pointers */
    endorsementKey = &(tpm_state->tpm_permanent_data.endorsementKey);
    /* so that Delete's are safe */
    TPM_KeyParms_Init(&keyInfo);		/* freed @1 */
    TPM_Pubkey_Init(&pubEndorsementKey);	/* freed @2 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get antiReplay parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Nonce_Load(antiReplay, &command, &paramSize);
    }
    /* get keyInfo parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyParms_Load(&keyInfo, &command, &paramSize); /* freed @1 */
    }	    
    /* get generateReset parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode  = TPM_LoadBool(&generateReset, &command, &paramSize);
    }
    /* get inputEKreset parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CreateRevocableEK: generateReset %02x\n", generateReset);
	/* an email clarification says that this parameter is still present (but ignored) if
	   generateReset is TRUE */
	returnCode = TPM_Nonce_Load(inputEKreset, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_CreateRevocableEK: inputEKreset", inputEKreset);
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
	    printf("TPM_Process_CreateRevocableEK: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. If an EK already exists, return TPM_DISABLED_CMD */
    /* 2. Perform the actions of TPM_CreateEndorsementKeyPair, if any errors return with error */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CreateEndorsementKeyPair_Common(endorsementKey,
							 &pubEndorsementKey,
							 checksum,
							 &writeAllNV1,
							 tpm_state,
							 &keyInfo,
							 antiReplay);
    }
    if (returnCode == TPM_SUCCESS) {
	/* 3. Set TPM_PERMANENT_FLAGS -> enableRevokeEK to TRUE */
	TPM_SetCapability_Flag(&writeAllNV1,					/* altered */
			       &(tpm_state->tpm_permanent_flags.enableRevokeEK),	/* flag */
			       TRUE);							/* value */
	/* a. If generateReset is TRUE then */
	if (generateReset) {
	    /* i. Set TPM_PERMANENT_DATA -> EKreset to the next value from the TPM RNG */
	    returnCode = TPM_Nonce_Generate(tpm_state->tpm_permanent_data.EKReset);
	}
	/* b. Else  */
	else {
	    /* i. Set TPM_PERMANENT_DATA -> EKreset to inputEkreset  */
	    TPM_Nonce_Copy(tpm_state->tpm_permanent_data.EKReset, inputEKreset);
	}
    }
    /* save the permanent data and flags structure sto NVRAM */
    returnCode = TPM_PermanentAll_NVStore(tpm_state,
					  (TPM_BOOL)(writeAllNV1 || writeAllNV2),
					  returnCode);
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_CreateRevocableEK: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* 4. Return PUBEK, checksum and Ekreset */
	    /* append pubEndorsementKey. */
	    returnCode = TPM_Pubkey_Store(response, &pubEndorsementKey);
	}
	/* append checksum */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Digest_Store(response, checksum);
	}
	/* append outputEKreset */
	/* 5. The outputEKreset authorization is sent in the clear. There is no uniqueness on the
	   TPM available to actually perform encryption or use an encrypted channel. The assumption
	   is that this operation is occurring in a controlled environment and sending the value in
	   the clear is acceptable.
	*/
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Nonce_Store(response, tpm_state->tpm_permanent_data.EKReset);
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
    TPM_KeyParms_Delete(&keyInfo);		/* @1 */
    TPM_Pubkey_Delete(&pubEndorsementKey);	/* @2 */
    return rcf;
}

/* 14.1 TPM_CreateEndorsementKeyPair rev 104

   This command creates the TPM endorsement key. It returns a failure code if an endorsement key
   already exists.
*/

TPM_RESULT TPM_Process_CreateEndorsementKeyPair(tpm_state_t *tpm_state,
						TPM_STORE_BUFFER *response,
						TPM_TAG tag,
						uint32_t paramSize,	/* of remaining parameters*/
						TPM_COMMAND_CODE ordinal,
						unsigned char *command,
						TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_NONCE		antiReplay;	/* Arbitrary data */
    TPM_KEY_PARMS	keyInfo;	/* Information about key to be created, this includes all
					   algorithm parameters */

    /* processing parameters */
    unsigned char *	inParamStart;		/* starting point of inParam's */
    unsigned char *	inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST		inParamDigest;
    TPM_BOOL		auditStatus = FALSE;		/* audit the ordinal */
    TPM_BOOL		transportEncrypt = FALSE;	/* wrapped in encrypted transport session */
    TPM_KEY		*endorsementKey = FALSE;	/* EK object from permanent store */
    TPM_BOOL		writeAllNV1 = FALSE;	/* flags to write back data */
    TPM_BOOL		writeAllNV2 = FALSE;	/* flags to write back flags */

    /* output parameters */
    uint32_t		outParamStart;		/* starting point of outParam's */
    uint32_t		outParamEnd;		/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_PUBKEY		pubEndorsementKey;	/* The public endorsement key */
    TPM_DIGEST		checksum;		/* Hash of pubEndorsementKey and antiReplay */
    
    printf("TPM_Process_CreateEndorsementKeyPair: Ordinal Entry\n");
    /* get pointers */
    endorsementKey = &(tpm_state->tpm_permanent_data.endorsementKey);
    /* so that Delete's are safe */
    TPM_KeyParms_Init(&keyInfo);		/* freed @1 */
    TPM_Pubkey_Init(&pubEndorsementKey);	/* freed @2 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get antiReplay parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Nonce_Load(antiReplay, &command, &paramSize);
    }
    /* get keyInfo parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyParms_Load(&keyInfo, &command, &paramSize); /* freed @1 */
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
	    printf("TPM_Process_CreateEndorsementKeyPair: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CreateEndorsementKeyPair_Common(endorsementKey,
							 &pubEndorsementKey,
							 checksum,
							 &writeAllNV1,
							 tpm_state,
							 &keyInfo,
							 antiReplay);
    }
    /* 10. Set TPM_PERMANENT_FLAGS -> enableRevokeEK to FALSE */
    if (returnCode == TPM_SUCCESS) {
	TPM_SetCapability_Flag(&writeAllNV2,					/* altered */
			       &(tpm_state->tpm_permanent_flags.enableRevokeEK),	/* flag */
			       FALSE);						/* value */
    }
    /* save the permanent data and flags structures to NVRAM */
    returnCode = TPM_PermanentAll_NVStore(tpm_state,
					  (TPM_BOOL)(writeAllNV1 || writeAllNV2),
					  returnCode);
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_CreateEndorsementKeyPair: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	/* append pubEndorsementKey.  */
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    returnCode = TPM_Pubkey_Store(response, &pubEndorsementKey);
	}
	/* append checksum */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Digest_Store(response, checksum);
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
    TPM_KeyParms_Delete(&keyInfo);		/* @1 */
    TPM_Pubkey_Delete(&pubEndorsementKey);	/* @2 */
    return rcf;
}

/* TPM_CreateEndorsementKeyPair_Common rev 104

   Actions common to TPM_CreateEndorsementKeyPair and TPM_CreateRevocableEK

   'endorsementKey' points to TPM_PERMANENT_DATA -> endorsementKey
*/

TPM_RESULT TPM_CreateEndorsementKeyPair_Common(TPM_KEY *endorsementKey,		/* output */
					       TPM_PUBKEY *pubEndorsementKey,	/* output */
					       TPM_DIGEST checksum,		/* output */
					       TPM_BOOL *writePermanentData,	/* output */
					       tpm_state_t *tpm_state,		/* input */
					       TPM_KEY_PARMS *keyInfo,		/* input */
					       TPM_NONCE antiReplay)		/* input */
{
    TPM_RESULT		returnCode = TPM_SUCCESS;
    TPM_RSA_KEY_PARMS	*tpm_rsa_key_parms;		/* from keyInfo */
    TPM_STORE_BUFFER	pubEndorsementKeySerial;	/* serialization for checksum calculation */
    const unsigned char *pubEndorsementKeyBuffer;
    uint32_t		pubEndorsementKeyLength;

    printf("TPM_CreateEndorsementKeyPair_Common:\n");
    TPM_Sbuffer_Init(&pubEndorsementKeySerial);		/* freed @1 */
    /* 1. If an EK already exists, return TPM_DISABLED_CMD */
    if (returnCode == TPM_SUCCESS) {
	if (endorsementKey->keyUsage != TPM_KEY_UNINITIALIZED) {
	    printf("TPM_CreateEndorsementKeyPair_Common: Error, key already initialized\n");
	    returnCode = TPM_DISABLED_CMD;
	}
    }
    /* 2. Validate the keyInfo parameters for the key description */
    if (returnCode == TPM_SUCCESS) {
	/*
	  RSA
	*/
	/* a. If the algorithm type is RSA the key length MUST be a minimum of
	   2048. For interoperability the key length SHOULD be 2048 */
	if (keyInfo->algorithmID == TPM_ALG_RSA) {
	    if (returnCode == TPM_SUCCESS) {
		/* get the keyInfo TPM_RSA_KEY_PARMS structure */
		returnCode = TPM_KeyParms_GetRSAKeyParms(&tpm_rsa_key_parms,
							 keyInfo);
	    }
	    if (returnCode == TPM_SUCCESS) {
		if (tpm_rsa_key_parms->keyLength != TPM_KEY_RSA_NUMBITS) {	/* in bits */
		    printf("TPM_CreateEndorsementKeyPair_Common: Error, "
			   "Bad keyLength should be %u, was %u\n",
			   TPM_KEY_RSA_NUMBITS, tpm_rsa_key_parms->keyLength);
		    returnCode = TPM_BAD_KEY_PROPERTY;
		}
	    }
	    /* kgold - Support only 2 primes */
	    if (returnCode == TPM_SUCCESS) {
		if (tpm_rsa_key_parms->numPrimes != 2) {
		    printf("TPM_CreateEndorsementKeyPair_Common: Error, "
			   "Bad numPrimes should be 2, was %u\n",
			   tpm_rsa_key_parms->numPrimes);
		    returnCode = TPM_BAD_KEY_PROPERTY;
		}
	    }
	}
	/*
	  not RSA
	*/
	/* b. If the algorithm type is other than RSA the strength provided by
	   the key MUST be comparable to RSA 2048 */
	else {
	    if (returnCode == TPM_SUCCESS) {
		printf("TPM_CreateEndorsementKeyPair_Common: Error, "
		       "algorithmID %08x not supported\n",
		       keyInfo->algorithmID);
		returnCode = TPM_BAD_KEY_PROPERTY;
	    }
	}
    }
    /* c. The other parameters of keyInfo (encScheme, sigScheme, etc.) are ignored.
     */
    /* 3. Create a key pair called the "endorsement key pair" using a TPM-protected capability. The
       type and size of key are that indicated by keyInfo.  Set encScheme to
       TPM_ES_RSAESOAEP_SHA1_MGF1.

       Save the endorsement key in permanent structure.	 Save the endorsement private key 'd' in the
       TPM_KEY structure as encData */
    /* Certain HW TPMs do not ignore the encScheme parameter, and expect it to be
       TPM_ES_RSAESOAEP_SHA1_MGF1.  Test the value here to detect an application program that will
       fail with that TPM. */

    if (returnCode == TPM_SUCCESS) {
	if (keyInfo->encScheme != TPM_ES_RSAESOAEP_SHA1_MGF1) {
	    returnCode = TPM_BAD_KEY_PROPERTY;
	    printf("TPM_CreateEndorsementKeyPair_Common: Error, "
		   "encScheme %08x must be TPM_ES_RSAESOAEP_SHA1_MGF1\n",
		   keyInfo->encScheme);
	}
    }
    if (returnCode == TPM_SUCCESS) {
	keyInfo->sigScheme = TPM_ES_NONE;
	returnCode = TPM_Key_GenerateRSA(endorsementKey,
					 tpm_state,
					 NULL,			/* parent key, indicate root key */
					 tpm_state->tpm_stclear_data.PCRS,	/* PCR array */
					 1,			/* TPM_KEY */
					 TPM_KEY_STORAGE,	/* keyUsage */
					 0,			/* keyFlags */
					 TPM_AUTH_ALWAYS,	/* authDataUsage */
					 keyInfo,
					 NULL,			/* no PCR's */
					 NULL);			/* no PCR's */
	*writePermanentData = TRUE;
    }
    /* Assemble the TPM_PUBKEY pubEndorsementKey for the response */
    if (returnCode == TPM_SUCCESS) {
	/* add TPM_KEY_PARMS algorithmParms */
	returnCode = TPM_KeyParms_Copy(&(pubEndorsementKey->algorithmParms),
				       keyInfo);
    }
    if (returnCode == TPM_SUCCESS) {
	/* add TPM_SIZED_BUFFER pubKey */
	returnCode = TPM_SizedBuffer_Set(&(pubEndorsementKey->pubKey),
					 endorsementKey->pubKey.size,
					 endorsementKey->pubKey.buffer);
    }				
    /* 4. Create checksum by performing SHA-1 on the concatenation of (PUBEK
       || antiReplay) */
    if (returnCode == TPM_SUCCESS) {
	/* serialize the pubEndorsementKey */
	returnCode = TPM_Pubkey_Store(&pubEndorsementKeySerial,
				      pubEndorsementKey);
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_Sbuffer_Get(&pubEndorsementKeySerial,
			&pubEndorsementKeyBuffer, &pubEndorsementKeyLength);
	/* create the checksum */
	returnCode = TPM_SHA1(checksum,
			      pubEndorsementKeyLength, pubEndorsementKeyBuffer,
			      sizeof(TPM_NONCE), antiReplay,
			      0, NULL);
    }
    /* 5. Store the PRIVEK */
    /* NOTE Created in TPM_PERMANENT_DATA, call should save to NVRAM */
    /* 6. Create TPM_PERMANENT_DATA -> tpmDAASeed from the TPM RNG */
    /* 7. Create TPM_PERMANENT_DATA -> daaProof from the TPM RNG */
    /* 8. Create TPM_PERMANENT_DATA -> daaBlobKey from the TPM RNG */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_PermanentData_InitDaa(&(tpm_state->tpm_permanent_data));
    }
    /* 9. Set TPM_PERMANENT_FLAGS -> CEKPUsed to TRUE */
    if (returnCode == TPM_SUCCESS) {
	tpm_state->tpm_permanent_flags.CEKPUsed = TRUE;
    }
    /*
      cleanup
    */
    TPM_Sbuffer_Delete(&pubEndorsementKeySerial);	/* @1 */
    return returnCode;
}

/* 14.3 TPM_RevokeTrust rev 98

  This command clears the EK and sets the TPM back to a pure default state. The generation of the
  AuthData value occurs during the generation of the EK. It is the responsibility of the EK
  generator to properly protect and disseminate the RevokeTrust AuthData.
*/

TPM_RESULT TPM_Process_RevokeTrust(tpm_state_t *tpm_state,
				   TPM_STORE_BUFFER *response,
				   TPM_TAG tag,
				   uint32_t paramSize,		/* of remaining parameters*/
				   TPM_COMMAND_CODE ordinal,
				   unsigned char *command,
				   TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_NONCE EKReset;			/* The value that will be matched to EK Reset */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus = FALSE;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt = FALSE;	/* wrapped in encrypted transport
								   session */
    TPM_BOOL			writeAllNV1 = FALSE;	/* flags to write back data */
    TPM_BOOL			writeAllNV2 = FALSE;	/* flags to write back flags */
    TPM_BOOL			writeAllNV3 = FALSE;	/* flags to write back flags */
    TPM_BOOL			physicalPresence;

    /* output parameters */
    uint32_t		outParamStart;		/* starting point of outParam's */
    uint32_t		outParamEnd;		/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_RevokeTrust: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get EKReset parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Nonce_Load(EKReset, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour(" TPM_Process_RevokeTrust: EKReset", EKReset);
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
	    printf("TPM_Process_RevokeTrust: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. The TPM MUST validate that TPM_PERMANENT_FLAGS -> enableRevokeEK is TRUE, return
       TPM_PERMANENTEK on error */
    if (returnCode == TPM_SUCCESS) {
	if (!tpm_state->tpm_permanent_flags.enableRevokeEK) {
	    printf("TPM_Process_RevokeTrust: Error, enableRevokeEK is FALSE\n");
	    returnCode = TPM_PERMANENTEK;
	}  
    }
    /* 2. The TPM MUST validate that the EKReset matches TPM_PERMANENT_DATA -> EKReset, return
       TPM_AUTHFAIL on error. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Nonce_Compare(tpm_state->tpm_permanent_data.EKReset, EKReset);
	if (returnCode != 0) {
	    printf("TPM_Process_RevokeTrust: Error, EKReset mismatch\n");
	    returnCode = TPM_AUTHFAIL;
	}
    }
    /* 3. Ensure that physical presence is being asserted */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Global_GetPhysicalPresence(&physicalPresence, tpm_state);
    }
    if (returnCode == TPM_SUCCESS) {
	if (!physicalPresence) {
	    printf("TPM_Process_RevokeTrust: Error, physicalPresence is FALSE\n");
	    returnCode = TPM_BAD_PRESENCE;
	}
    }
    /* 4. Perform the actions of TPM_OwnerClear (excepting the command authentication) */
    /* a. NV items with the pubInfo -> nvIndex D value set MUST be deleted. This changes the
       TPM_OwnerClear handling of the same NV areas */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_OwnerClearCommon(tpm_state,
					  TRUE);	/* delete all NVRAM */
	writeAllNV1 = TRUE;
    }
    if (returnCode == TPM_SUCCESS) {
	/* b. Set TPM_PERMANENT_FLAGS -> nvLocked to FALSE */
	TPM_SetCapability_Flag(&writeAllNV2,				/* altered (dummy) */
			       &(tpm_state->tpm_permanent_flags.nvLocked),	/* flag */
			       FALSE);						/* value */
	/* 5. Invalidate TPM_PERMANENT_DATA -> tpmDAASeed */
	/* 6. Invalidate TPM_PERMANENT_DATA -> daaProof */
	/* 7. Invalidate TPM_PERMANENT_DATA -> daaBlobKey */
	returnCode = TPM_PermanentData_InitDaa(&(tpm_state->tpm_permanent_data));
    }
    if (returnCode == TPM_SUCCESS) {
	/* 8. Invalidate the EK and any internal state associated with the EK */
	printf("TPM_Process_RevokeTrust: Deleting endorsement key\n");
	TPM_Key_Delete(&(tpm_state->tpm_permanent_data.endorsementKey));
	TPM_SetCapability_Flag(&writeAllNV3,				/* altered  (dummy) */
			       &(tpm_state->tpm_permanent_flags.CEKPUsed),	/* flag */
			       FALSE);						/* value */
    }
    /* Store the permanent data and flags back to NVRAM */
    returnCode = TPM_PermanentAll_NVStore(tpm_state,
					  writeAllNV1,
					  returnCode);
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_RevokeTrust: Ordinal returnCode %08x %u\n",
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
    return rcf;
}

/* 27.7 TPM_DisablePubekRead rev 94

   The TPM Owner may wish to prevent any entity from reading the PUBEK. This command sets the
   non-volatile flag so that the TPM_ReadPubek command always returns TPM_DISABLED_CMD.

   This commands has in essence been deprecated as TPM_TakeOwnership now sets the value to false.
   The commands remains at this time for backward compatibility.
*/

TPM_RESULT TPM_Process_DisablePubekRead(tpm_state_t *tpm_state,
					TPM_STORE_BUFFER *response,
					TPM_TAG tag,
					uint32_t paramSize,		/* of remaining parameters*/
					TPM_COMMAND_CODE ordinal,
					unsigned char *command,
					TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_AUTHHANDLE	authHandle;	/* The authorization handle used for owner authorization. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   handle */
    TPM_AUTHDATA	ownerAuth;	/* The authorization digest for inputs and owner
					   authorization. HMAC key: ownerAuth. */

    /* processing */
    unsigned char *	inParamStart;		/* starting point of inParam's */
    unsigned char *	inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST		inParamDigest;
    TPM_BOOL		auditStatus;		/* audit the ordinal */
    TPM_BOOL		transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL		authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA *auth_session_data;		/* session data for authHandle */
    TPM_SECRET		*hmacKey;
    TPM_BOOL		writeAllNV = FALSE;	/* flag to write back NV */

    /* output parameters */
    uint32_t		outParamStart;		/* starting point of outParam's */
    uint32_t		outParamEnd;		/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_DisablePubekRead: Ordinal Entry\n");
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
	    printf("TPM_Process_DisablePubekRead: Error, command has %u extra bytes\n",
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
    /* Verify that the TPM Owner authorizes the command and all of the input, on error return
       TPM_AUTHFAIL. */
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
    /* 1. This capability sets the TPM_PERMANENT_FLAGS -> readPubek flag to FALSE. */
    if (returnCode == TPM_SUCCESS) {
	TPM_SetCapability_Flag(&writeAllNV,					/* altered */
			       &(tpm_state->tpm_permanent_flags.readPubek),	/* flag */
			       FALSE);						/* value */
	printf("TPM_Process_DisablePubekRead: readPubek now %02x\n",
	       tpm_state->tpm_permanent_flags.readPubek);
	/* save the permanent flags structure to NVRAM */
	returnCode = TPM_PermanentAll_NVStore(tpm_state,
					      writeAllNV,
					      returnCode);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_DisablePubekRead: Ordinal returnCode %08x %u\n",
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
	    /* no outParam's, set authorization response data */
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
    /* if there was an error, terminate the session. */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    return rcf;
}

/* 27.6 TPM_OwnerReadPubek rev 94

   Return the endorsement key public portion. This is authorized by the TPM Owner.
*/

TPM_RESULT TPM_Process_OwnerReadPubek(tpm_state_t *tpm_state,
				      TPM_STORE_BUFFER *response,
				      TPM_TAG tag,
				      uint32_t paramSize,		/* of remaining parameters*/
				      TPM_COMMAND_CODE ordinal,
				      unsigned char *command,
				      TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_AUTHHANDLE	authHandle;	/* The authorization handle used for owner authorization. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   handle */
    TPM_AUTHDATA	ownerAuth;	/* The authorization digest for inputs and owner
					   authorization. HMAC key: ownerAuth. */
  
    /* processing parameters */
    unsigned char *	inParamStart;		/* starting point of inParam's */
    unsigned char *	inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST		inParamDigest;
    TPM_BOOL		auditStatus;		/* audit the ordinal */
    TPM_BOOL		transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL		authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA *auth_session_data;	/* session data for authHandle */
    TPM_SECRET		*hmacKey;
    const unsigned char *pubEndorsementKeyStreamBuffer;
    uint32_t		pubEndorsementKeyStreamLength;

    /* output parameters */
    uint32_t		outParamStart;			/* starting point of outParam's */
    uint32_t		outParamEnd;			/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_STORE_BUFFER	pubEndorsementKeyStream;	/* The public endorsement key */

    printf("TPM_Process_OwnerReadPubek: Ordinal Entry\n");
    TPM_Sbuffer_Init(&pubEndorsementKeyStream); /* freed @1 */
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
	    printf("TPM_Process_OwnerReadPubek: Error, command has %u extra bytes\n",
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
    /* 1. Validate the TPM Owner authorization to execute this command */
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
    /* serialize the TPM_PUBKEY components of the EK */
    if (returnCode == TPM_SUCCESS) {
	returnCode =
	    TPM_Key_StorePubkey(&pubEndorsementKeyStream,		/* output */
				&pubEndorsementKeyStreamBuffer,		/* output */
				&pubEndorsementKeyStreamLength,		/* output */
				&(tpm_state->tpm_permanent_data.endorsementKey));	/* input */
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_OwnerReadPubek: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* 2. Export the PUBEK */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_Sbuffer_Append(response,
						pubEndorsementKeyStreamBuffer,
						pubEndorsementKeyStreamLength);
	    }
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
	    /* no outParam's, set authorization response data */
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
    /* if there was an error, terminate the session.  */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    TPM_Sbuffer_Delete(&pubEndorsementKeyStream);	/* @1 */
    return rcf;
}

/* 27.1.1 TPM_EvictKey rev 87

   The key commands are deprecated as the new way to handle keys is to use the standard context
   commands.  So TPM_EvictKey is now handled by TPM_FlushSpecific, TPM_TerminateHandle by
   TPM_FlushSpecific.

   The TPM will invalidate the key stored in the specified handle and return the space to the
   available internal pool for subsequent query by TPM_GetCapability and usage by TPM_LoadKey. If
   the specified key handle does not correspond to a valid key, an error will be returned.
*/

TPM_RESULT TPM_Process_EvictKey(tpm_state_t *tpm_state,
				TPM_STORE_BUFFER *response,
				TPM_TAG tag,
				uint32_t paramSize,		/* of remaining parameters*/
				TPM_COMMAND_CODE ordinal,
				unsigned char *command,
				TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_KEY_HANDLE		evictHandle;	/* The handle of the key to be evicted. */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_KEY_HANDLE_ENTRY	*tpm_key_handle_entry;	/* table entry for the evictHandle */

    /* output parameters */
    uint32_t		outParamStart;		/* starting point of outParam's */
    uint32_t		outParamEnd;		/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_EvictKey: Ordinal Entry\n");
    /*
      get inputs
    */
    /* get evictHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&evictHandle, &command, &paramSize);
    }
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
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_ALL);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_EvictKey: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* New 1.2 functionality
       The command must check the status of the ownerEvict flag for the key and if the flag is TRUE
       return TPM_KEY_CONTROL_OWNER
    */
    /* evict the key stored in the specified handle */
    /* get the TPM_KEY_HANDLE_ENTRY */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_EvictKey: Evicting handle %08x\n", evictHandle);
	returnCode = TPM_KeyHandleEntries_GetEntry(&tpm_key_handle_entry,
						   tpm_state->tpm_key_handle_entries,
						   evictHandle);
	if (returnCode != TPM_SUCCESS) {
	    printf("TPM_Process_EvictKey: Error, key handle %08x not found\n",
		   evictHandle);
	}
    }
    /* If tpm_key_handle_entry -> ownerEvict is TRUE return TPM_KEY_OWNER_CONTROL */
    if (returnCode == TPM_SUCCESS) {
	if (tpm_key_handle_entry->keyControl & TPM_KEY_CONTROL_OWNER_EVICT) {
	    printf("TPM_Process_EvictKey: Error, keyHandle specifies owner evict\n");
	    returnCode = TPM_KEY_OWNER_CONTROL;
	}
    }
    /* delete the entry, delete the key structure, and free the key */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntry_FlushSpecific(tpm_state, tpm_key_handle_entry);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_EvictKey: Ordinal returnCode %08x %u\n",
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
    return rcf;
}

/* 14.5 TPM_OwnerReadInternalPub rev 87

   A TPM Owner authorized command that returns the public portion of the EK or SRK.

   The keyHandle parameter is included in the incoming session authorization to prevent 
   alteration of the value, causing a different key to be read.	 Unlike most key handles, which 
   can be mapped by higher layer software, this key handle has only two fixed values.

*/

TPM_RESULT TPM_Process_OwnerReadInternalPub(tpm_state_t *tpm_state,
					    TPM_STORE_BUFFER *response,
					    TPM_TAG tag,
					    uint32_t paramSize,	     /* of remaining parameters */
					    TPM_COMMAND_CODE ordinal,
					    unsigned char *command,
					    TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_KEY_HANDLE keyHandle;	/* Handle for either PUBEK or SRK */
    TPM_AUTHHANDLE authHandle;	/* The authorization session handle used for owner
				   authentication. */
    TPM_NONCE nonceOdd;		/* Nonce generated by system associated with authHandle */
    TPM_BOOL continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   session handle */
    TPM_AUTHDATA ownerAuth;	/* The authorization session digest for inputs and owner
				   authentication.  HMAC key: ownerAuth. */

    /* processing parameters */
    unsigned char *	inParamStart;		/* starting point of inParam's */
    unsigned char *	inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST		inParamDigest;
    TPM_BOOL		auditStatus;		/* audit the ordinal */
    TPM_BOOL		transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL		authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA *auth_session_data;	/* session data for authHandle */
    TPM_SECRET		*hmacKey;
    TPM_KEY		*readKey = NULL;	/* key to be read back */
    const unsigned char *stream;
    uint32_t		stream_size;

    /* output parameters */
    uint32_t		outParamStart;		/* starting point of outParam's */
    uint32_t		outParamEnd;		/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_OwnerReadInternalPub: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    /* NOTE: This is a special case, where the keyHandle is part of the HMAC calculation to
       avoid a man-in-the-middle privacy attack that replaces the SRK handle with the EK
       handle. */
    inParamStart = command;
    /*	get keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&keyHandle, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_OwnerReadInternalPub: keyHandle %08x\n", keyHandle);
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
	    printf("TPM_Process_OwnerReadInternalPub: Error, command has %u extra bytes\n",
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
    /* 1. Validate the parameters and TPM Owner AuthData for this command */
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
	/* 2. If keyHandle is TPM_KH_EK */
	if (keyHandle == TPM_KH_EK) {
	    /* a. Set publicPortion to PUBEK */
	    printf("TPM_Process_OwnerReadInternalPub: Reading EK\n");
	    readKey = &(tpm_state->tpm_permanent_data.endorsementKey);
	}
	/* 3. Else If keyHandle is TPM_KH_SRK */
	else if (keyHandle == TPM_KH_SRK) {
	    /* a. Set publicPortion to the TPM_PUBKEY of the SRK */
	    printf("TPM_Process_OwnerReadInternalPub: Reading SRK\n");
	    readKey = &(tpm_state->tpm_permanent_data.srk);
	}
	/* 4. Else return TPM_BAD_PARAMETER */
	else {
	    printf("TPM_Process_OwnerReadInternalPub: Error, invalid keyHandle %08x\n",
		   keyHandle);
	    returnCode = TPM_BAD_PARAMETER;
	}
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_OwnerReadInternalPub: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* 5. Export the public key of the referenced key */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_Key_StorePubkey(response, &stream, &stream_size, readKey);
	    }
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
	    /* no outParam's, set authorization response data */
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
    /* if there was an error, terminate the session. */
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


