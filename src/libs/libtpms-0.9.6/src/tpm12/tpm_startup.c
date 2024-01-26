/********************************************************************************/
/*										*/
/*			    TPM Admin Startup and State				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_startup.c $		*/
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

#include "tpm_debug.h"
#include "tpm_error.h"
#include "tpm_constants.h"
#include "tpm_commands.h"
#include "tpm_crypto.h"
#include "tpm_cryptoh.h"
#include "tpm_debug.h"
#include "tpm_error.h"
#include "tpm_digest.h"
#include "tpm_init.h"
#include "tpm_key.h"
#include "tpm_nonce.h"
#include "tpm_nvfile.h"
#include "tpm_nvfilename.h"
#include "tpm_nvram.h"
#include "tpm_pcr.h"
#include "tpm_process.h"
#include "tpm_session.h"

#include "tpm_startup.h"

/*
  Save State
*/

/* TPM_SaveState_Load() restores the TPM state from a stream created by TPM_SaveState_Store()

   The two functions must be kept in sync.
*/

TPM_RESULT TPM_SaveState_Load(tpm_state_t *tpm_state,
			      unsigned char **stream,
			      uint32_t *stream_size)
{
    TPM_RESULT			rc = 0;
    unsigned char		*stream_start = *stream;	/* copy for integrity check */
    uint32_t			stream_size_start = *stream_size;
    
    printf(" TPM_SaveState_Load:\n");
    if (rc == 0) {
	printf("  TPM_SaveState_Load: Loading PCR's\n");
    }
    /* 1. Store PCR contents except for */
    /* a. If the PCR attribute pcrReset is TRUE */
    /* b. Any platform identified debug PCR */
    /* NOTE Done by TPM_StclearData_Load() */
    /* 2. The auditDigest MUST be handled according to the audit requirements as reported by
       TPM_GetCapability */
    /* NOTE Moved to TPM_STCLEAR_DATA */
    /* 3. All values in TPM_STCLEAR_DATA MUST be preserved */
    if (rc == 0) {
	rc = TPM_StclearData_Load(&(tpm_state->tpm_stclear_data), stream, stream_size,
				  tpm_state->tpm_permanent_data.pcrAttrib);
    }
    /* 4. All values in TPM_STCLEAR_FLAGS MUST be preserved */
    if (rc == 0) {
	rc = TPM_StclearFlags_Load(&(tpm_state->tpm_stclear_flags), stream, stream_size);
    }
    /* 5. The contents of any key that is currently loaded SHOULD be preserved if the key's
       parentPCRStatus indicator is TRUE. */
    /* 6. The contents of any key that has TPM_KEY_CONTROL_OWNER_EVICT set MUST be preserved */
    /* 7. The contents of any key that is currently loaded MAY be preserved as reported by
       TPM_GetCapability */
    if (rc == 0) {
	rc = TPM_KeyHandleEntries_Load(tpm_state, stream, stream_size);
    }
    /* 8. The contents of sessions (authorization, transport etc.) MAY be preserved as reported by
       TPM_GetCapability */
    /* NOTE Done at TPM_StclearData_Load() */
    /* load the NV volatile flags */
    if (rc == 0) {
	rc = TPM_NVIndexEntries_LoadVolatile(&(tpm_state->tpm_nv_index_entries),
					     stream, stream_size);
    }
    /* sanity check the stream size */
    if (rc == 0) {
	if (*stream_size != TPM_DIGEST_SIZE) {
	    printf("TPM_SaveState_Load: Error (fatal) stream size %u not %u\n",
		   *stream_size, TPM_DIGEST_SIZE);
	    rc = TPM_FAIL;
	}
    }
    /* check the integrity digest */
    if (rc == 0) {
	printf("  TPM_SaveState_Load: Checking integrity digest\n");
	rc = TPM_SHA1_Check(*stream, 	/* currently points to integrity digest */
			    stream_size_start - TPM_DIGEST_SIZE, stream_start,
			    0, NULL);
    }
    /* remove the integrity digest from the stream */
    if (rc == 0) {
	*stream_size -= TPM_DIGEST_SIZE;
    }
    return rc;
}

/* TPM_SaveState_Store() stores the TPM state to a stream that can be restored through
   TPM_SaveState_Load().

   The two functions must be kept in sync.
*/

TPM_RESULT TPM_SaveState_Store(TPM_STORE_BUFFER *sbuffer,
			       tpm_state_t *tpm_state)
{
    TPM_RESULT			rc = 0;
    const unsigned char 	*buffer;	/* elements of sbuffer */
    uint32_t 			length;
    TPM_DIGEST			tpm_digest;

    printf(" TPM_SaveState_Store:\n");
    if (rc == 0) {
	printf("  TPM_SaveState_Store: Storing PCR's\n");
    }
    /* NOTE: Actions from TPM_SaveState */
    /* 1. Store TPM_STCLEAR_DATA -> PCR contents except for */
    /* a. If the PCR attribute pcrReset is TRUE */
    /* b. Any platform identified debug PCR */
    /* NOTE Done by TPM_StclearData_Store() */
    /* 2. The auditDigest MUST be handled according to the audit requirements as reported by
       TPM_GetCapability */
    /* NOTE Moved to TPM_STCLEAR_DATA */
    /* a. If the ordinalAuditStatus is TRUE for the TPM_SaveState ordinal and the auditDigest is
       being stored in the saved state, the saved auditDigest MUST include the TPM_SaveState input
       parameters and MUST NOT include the output parameters. */
    /* NOTE Done naturally because this function is called between input and output audit. */
    /* 3. All values in TPM_STCLEAR_DATA MUST be preserved */
    if (rc == 0) {
	rc = TPM_StclearData_Store(sbuffer, &(tpm_state->tpm_stclear_data),
				   tpm_state->tpm_permanent_data.pcrAttrib);
    }
    /* 4. All values in TPM_STCLEAR_FLAGS MUST be preserved */
    if (rc == 0) {
	rc = TPM_StclearFlags_Store(sbuffer, &(tpm_state->tpm_stclear_flags));
    }
    /* 5. The contents of any key that is currently loaded SHOULD be preserved if the key's
       parentPCRStatus indicator is TRUE. */
    /* 6. The contents of any key that has TPM_KEY_CONTROL_OWNER_EVICT set MUST be preserved */
    /* 7. The contents of any key that is currently loaded MAY be preserved as reported by
       TPM_GetCapability */
    /* NOTE This implementation saves all keys.	 Owner evict keys are not saved in the state blob,
       as they are already saved in the file system */
    if (rc == 0) {
	rc = TPM_KeyHandleEntries_Store(sbuffer, tpm_state);
    }
    /* 8. The contents of sessions (authorization, transport etc.) MAY be preserved as reported by
       TPM_GetCapability */
    /* NOTE Done by TPM_StclearData_Store() */
    /* store the  NV volatile flags */
    if (rc == 0) {
	rc = TPM_NVIndexEntries_StoreVolatile(sbuffer,
					      &(tpm_state->tpm_nv_index_entries));
    }
    if (rc == 0) {
	/* get the current serialized buffer and its length */
	TPM_Sbuffer_Get(sbuffer, &buffer, &length);
	/* generate the integrity digest */
	rc = TPM_SHA1(tpm_digest,
		      length, buffer,
		      0, NULL);
    }
    /* append the integrity digest to the stream */
    if (rc == 0) {
	printf(" TPM_SaveState_Store: Appending integrity digest\n");
	rc = TPM_Sbuffer_Append(sbuffer, tpm_digest, TPM_DIGEST_SIZE);
    }
    return rc;
}

/* TPM_SaveState_IsSaveKey() determines which keys are saved as part of the saved state.

   According to Ryan, all keys must be saved for this to be of use.
*/

void TPM_SaveState_IsSaveKey(TPM_BOOL *save, 
			     TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entry)
{
    *save = FALSE;
    /* 5. The contents of any key that is currently loaded SHOULD be preserved if the key's
       parentPCRStatus indicator is TRUE. */
    /* 6. The contents of any key that has TPM_KEY_CONTROL_OWNER_EVICT set MUST be preserved */
    /* 7. The contents of any key that is currently loaded MAY be preserved as reported by
       TPM_GetCapability */
    /* NOTE Owner evict keys are not saved in the state blob, as they are already saved in the file
       system */
    if (!(tpm_key_handle_entry->keyControl & TPM_KEY_CONTROL_OWNER_EVICT)) {
	*save = TRUE;
    }
    else {
	*save = FALSE;
    }
    if (*save) {
	printf(" TPM_SaveState_IsSaveKey: Save key handle %08x\n", tpm_key_handle_entry->handle);
    }
    return;
}

/* TPM_SaveState_NVLoad() deserializes the saved state data from the NV file TPM_SAVESTATE_NAME

   0 on success.
   Returns TPM_RETRY on non-existent file
   TPM_FAIL on failure to load (fatal), since it should never occur
*/

TPM_RESULT TPM_SaveState_NVLoad(tpm_state_t *tpm_state)
{
    TPM_RESULT		rc = 0;
    unsigned char	*stream = NULL;
    unsigned char	*stream_start = NULL;
    uint32_t		stream_size;
    
    printf(" TPM_SaveState_NVLoad:\n");
    if (rc == 0) {
	/* load from NVRAM.  Returns TPM_RETRY on non-existent file. */
	rc = TPM_NVRAM_LoadData(&stream,			/* freed @1 */
				&stream_size,
				tpm_state->tpm_number,
				TPM_SAVESTATE_NAME);
    }
    /* deserialize from stream */
    if (rc == 0) {
	stream_start = stream;			/* save starting point for free() */
	rc = TPM_SaveState_Load(tpm_state, &stream, &stream_size);
	if (rc != 0) {
	    printf("TPM_SaveState_NVLoad: Error (fatal) loading deserializing saved state\n");
	    rc = TPM_FAIL;
	}
    }
    free(stream_start); /* @1 */
    return rc;
}

/* TPM_SaveState_NVStore() serializes saved state data and stores it in the NV file
   TPM_SAVESTATE_NAME
*/

TPM_RESULT TPM_SaveState_NVStore(tpm_state_t *tpm_state)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	sbuffer;		/* safe buffer for storing binary data */
    const unsigned char *buffer;
    uint32_t		length;

    printf(" TPM_SaveState_NVStore:\n");
    TPM_Sbuffer_Init(&sbuffer);			/* freed @1 */
    /* serialize relevant data from tpm_state  to be written to NV */
    if (rc == 0) {
	rc = TPM_SaveState_Store(&sbuffer, tpm_state);
	/* get the serialized buffer and its length */
	TPM_Sbuffer_Get(&sbuffer, &buffer, &length);
    }
    /* validate the length of the stream */
    if (rc == 0) {
	printf("   TPM_SaveState_NVStore: Require %u bytes\n", length);
	if (length > TPM_MAX_SAVESTATE_SPACE) {
	    printf("TPM_SaveState_NVStore: Error, No space, need %u max %u\n",
		   length, TPM_MAX_SAVESTATE_SPACE);
	    rc = TPM_NOSPACE;
	}
    }
    if (rc == 0) {
	/* store the buffer in NVRAM */
	rc = TPM_NVRAM_StoreData(buffer,
				 length,
				 tpm_state->tpm_number,
				 TPM_SAVESTATE_NAME);
	tpm_state->tpm_stany_flags.stateSaved = TRUE;  /* mark the state as stored */
    }
    TPM_Sbuffer_Delete(&sbuffer);	/* @1 */
    return rc;
}

/* TPM_SaveState_NVDelete() deletes the NV file

   If mustExist is TRUE, returns an error if the file does not exist.
   If mustExist is FALSE, returns success if the file does not exist.
*/

TPM_RESULT TPM_SaveState_NVDelete(tpm_state_t *tpm_state,
				  TPM_BOOL mustExist)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_SaveState_NVDelete:\n");
    if (rc == 0) {
	/* remove the saved state */
	rc = TPM_NVRAM_DeleteName(tpm_state->tpm_number,
				  TPM_SAVESTATE_NAME,
				  mustExist);
	tpm_state->tpm_stany_flags.stateSaved = FALSE;	/* mark the state as deleted */
    }
    return rc;
}

/* Volatile state includes all the tpm_state structure volatile members.  It is a superset of Saved
   state, used when the entire TPM state must be saved and restored
*/

/* TPM_VolatileAll_Load() restores the TPM state from a stream created by TPM_VolatileAll_Store()

   The two functions must be kept in sync.
*/

TPM_RESULT TPM_VolatileAll_Load(tpm_state_t *tpm_state,
				unsigned char **stream,
				uint32_t *stream_size)
{
    TPM_RESULT			rc = 0;
    TPM_PCR_ATTRIBUTES 		pcrAttrib[TPM_NUM_PCR];
    size_t			i;
    unsigned char		*stream_start = *stream;	/* copy for integrity check */
    uint32_t			stream_size_start = *stream_size;

    printf(" TPM_VolatileAll_Load:\n");
    /* check format tag */
    /* In the future, if multiple formats are supported, this check will be replaced by a 'switch'
       on the tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_VSTATE_V1, stream, stream_size);
    }
    /* compiled in TPM parameters */
    if (rc == 0) {
	rc = TPM_Parameters_Load(stream, stream_size);
    }
    /* V1 is the TCG standard returned by the getcap.  It's unlikely that this will change */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_STCLEAR_FLAGS_V1, stream, stream_size);
    }
    /* TPM_STCLEAR_FLAGS */
    if (rc == 0) {
	rc = TPM_StclearFlags_Load(&(tpm_state->tpm_stclear_flags), stream, stream_size);
    }
    /* TPM_STANY_FLAGS  */
    if (rc == 0) {
	rc = TPM_StanyFlags_Load(&(tpm_state->tpm_stany_flags), stream, stream_size);
    }
    /* TPM_STCLEAR_DATA  */
    /* normally, resettable PCRs are not restored.  "All" means to restore everything */
    for (i = 0 ; (rc == 0) && (i < TPM_NUM_PCR) ; i++) {
	pcrAttrib[i].pcrReset = FALSE;
    }
    /* TPM_STCLEAR_DATA */
    if (rc == 0) {
	rc = TPM_StclearData_Load(&(tpm_state->tpm_stclear_data), stream, stream_size,
				  (TPM_PCR_ATTRIBUTES *)&pcrAttrib);
    }
    /* TPM_STANY_DATA  */
    if (rc == 0) {
	rc = TPM_StanyData_Load(&(tpm_state->tpm_stany_data), stream, stream_size);
    }
    /* TPM_KEY_HANDLE_ENTRY */
    if (rc == 0) {
	rc = TPM_KeyHandleEntries_Load(tpm_state, stream, stream_size);
    }
    /* Context for SHA1 functions */
    if (rc == 0) {
	printf("  TPM_VolatileAll_Load: Loading SHA ordinal context\n");
	rc = TPM_Sha1Context_Load(&(tpm_state->sha1_context), stream, stream_size);
    }
    /* Context for TIS SHA1 functions */
    if (rc == 0) {
	printf("  TPM_VolatileAll_Load: Loading TIS context\n");
	rc = TPM_Sha1Context_Load(&(tpm_state->sha1_context_tis), stream, stream_size);
    }
    /* TPM_TRANSHANDLE */
    if (rc == 0) {
        rc = TPM_Load32(&(tpm_state->transportHandle), stream, stream_size);
    }
    /* testState */
    if (rc == 0) {
        rc = TPM_Load32(&(tpm_state->testState), stream, stream_size);
    }
    /* load the NV volatile flags */
    if (rc == 0) {
	rc = TPM_NVIndexEntries_LoadVolatile(&(tpm_state->tpm_nv_index_entries),
					     stream, stream_size);
    }
    /* sanity check the stream size */
    if (rc == 0) {
	if (*stream_size != TPM_DIGEST_SIZE) {
	    printf("TPM_VolatileAll_Load: Error (fatal) stream size %u not %u\n",
		   *stream_size, TPM_DIGEST_SIZE);
	    rc = TPM_FAIL;
	}
    }
    /* check the integrity digest */
    if (rc == 0) {
	printf("  TPM_VolatileAll_Load: Checking integrity digest\n");
	rc = TPM_SHA1_Check(*stream, 	/* currently points to integrity digest */
			    stream_size_start - TPM_DIGEST_SIZE, stream_start,
			    0, NULL);
    }
    /* remove the integrity digest from the stream */
    if (rc == 0) {
	*stream_size -= TPM_DIGEST_SIZE;
    }
    return rc;
}

/* TPM_VolatileAll_Store() stores the TPM state to a stream that can be restored through
   TPM_VolatileAll_Load().

   The two functions must be kept in sync.
*/

TPM_RESULT TPM_VolatileAll_Store(TPM_STORE_BUFFER *sbuffer,
				 tpm_state_t *tpm_state)
{
    TPM_RESULT			rc = 0;
    TPM_PCR_ATTRIBUTES 		pcrAttrib[TPM_NUM_PCR];
    size_t			i;
    const unsigned char 	*buffer;	/* elements of sbuffer */
    uint32_t 			length;
    TPM_DIGEST			tpm_digest;

    printf(" TPM_VolatileAll_Store:\n");
    /* overall format tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_VSTATE_V1);
    }
    /* compiled in TPM parameters */
    if (rc == 0) {
	rc = TPM_Parameters_Store(sbuffer);
    }
    /* V1 is the TCG standard returned by the getcap.  It's unlikely that this will change */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_STCLEAR_FLAGS_V1);
    }
    /* TPM_STCLEAR_FLAGS */
    if (rc == 0) {
	rc = TPM_StclearFlags_Store(sbuffer, &(tpm_state->tpm_stclear_flags));
    }
    /* TPM_STANY_FLAGS */
    if (rc == 0) {
	rc = TPM_StanyFlags_Store(sbuffer, &(tpm_state->tpm_stany_flags));
    }
    /* TPM_STCLEAR_DATA */
    /* normally, resettable PCRs are not restored.  "All" means to restore everything */
    for (i = 0 ; (rc == 0) && (i < TPM_NUM_PCR) ; i++) {
	pcrAttrib[i].pcrReset = FALSE;
    }
    /* TPM_STCLEAR_DATA */
    if (rc == 0) {
	rc = TPM_StclearData_Store(sbuffer, &(tpm_state->tpm_stclear_data),
				   (TPM_PCR_ATTRIBUTES *)&pcrAttrib);
    }
    /* TPM_STANY_DATA  */
    if (rc == 0) {
	rc = TPM_StanyData_Store(sbuffer, &(tpm_state->tpm_stany_data));
    }
    /* TPM_KEY_HANDLE_ENTRY */
    if (rc == 0) {
	rc = TPM_KeyHandleEntries_Store(sbuffer, tpm_state);
    }
    /* Context for SHA1 functions */
    if (rc == 0) {
	printf("  TPM_VolatileAll_Store: Storing SHA ordinal context\n");
	rc = TPM_Sha1Context_Store(sbuffer, tpm_state->sha1_context);
    }
    /* Context for TIS SHA1 functions */
    if (rc == 0) {
	printf("  TPM_VolatileAll_Store: Storing TIS context\n");
	rc = TPM_Sha1Context_Store(sbuffer, tpm_state->sha1_context_tis);
    }
    /* TPM_TRANSHANDLE */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_state->transportHandle);
    }
    /* testState */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_state->testState);
    }
    /* store the NV volatile flags */
    if (rc == 0) {
	rc = TPM_NVIndexEntries_StoreVolatile(sbuffer,
					      &(tpm_state->tpm_nv_index_entries));
    }
    if (rc == 0) {
	/* get the current serialized buffer and its length */
	TPM_Sbuffer_Get(sbuffer, &buffer, &length);
	/* generate the integrity digest */
	rc = TPM_SHA1(tpm_digest,
		      length, buffer,
		      0, NULL);
    }
    /* append the integrity digest to the stream */
    if (rc == 0) {
	printf(" TPM_VolatileAll_Store: Appending integrity digest\n");
	rc = TPM_Sbuffer_Append(sbuffer, tpm_digest, TPM_DIGEST_SIZE);
    }
    return rc;
}

/* TPM_VolatileAll_NVLoad() deserializes the entire volatile state data from the NV file
   TPM_VOLATILESTATE_NAME.

   If the file does not exist (a normal startup), returns success.

   0 on success or non-existent file
   TPM_FAIL on failure to load (fatal), since it should never occur
*/

TPM_RESULT TPM_VolatileAll_NVLoad(tpm_state_t *tpm_state)
{
    TPM_RESULT		rc = 0;
    TPM_BOOL		done = FALSE;
    unsigned char	*stream = NULL;
    unsigned char	*stream_start = NULL;
    uint32_t		stream_size;
    
    printf(" TPM_VolatileAll_NVLoad:\n");
    if (rc == 0) {
	/* load from NVRAM.  Returns TPM_RETRY on non-existent file. */
	rc = TPM_NVRAM_LoadData(&stream,			/* freed @1 */
				&stream_size,
				tpm_state->tpm_number,
				TPM_VOLATILESTATE_NAME);
	/* if the file does not exist, leave the volatile state initial values */
	if (rc == TPM_RETRY) {
	    done = TRUE;
	    rc = 0;
	}
	else if (rc != 0) {
	    printf("TPM_VolatileAll_NVLoad: Error (fatal) loading %s\n", TPM_VOLATILESTATE_NAME);
	    rc = TPM_FAIL;
	}
    }
    /* deserialize from stream */
    if ((rc == 0) && !done) {
	stream_start = stream;			/* save starting point for free() */
	rc = TPM_VolatileAll_Load(tpm_state, &stream, &stream_size);
	if (rc != 0) {
	    printf("TPM_VolatileAll_NVLoad: Error (fatal) loading deserializing state\n");
	    rc = TPM_FAIL;
	}
    }
    if (rc != 0) {
	printf("  TPM_VolatileAll_NVLoad: Set testState to %u \n", TPM_TEST_STATE_FAILURE);
	tpm_state->testState = TPM_TEST_STATE_FAILURE;
	
    }
    free(stream_start); /* @1 */
    return rc;
}

/* TPM_VolatileAll_NVStore() serializes the entire volatile state data and stores it in the NV file
   TPM_VOLATILESTATE_NAME
*/

TPM_RESULT TPM_VolatileAll_NVStore(tpm_state_t *tpm_state)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	sbuffer;		/* safe buffer for storing binary data */
    const unsigned char *buffer;
    uint32_t		length;

    printf(" TPM_VolatileAll_NVStore:\n");
    TPM_Sbuffer_Init(&sbuffer);			/* freed @1 */
    /* serialize relevant data from tpm_state  to be written to NV */
    if (rc == 0) {
	rc = TPM_VolatileAll_Store(&sbuffer, tpm_state);
	/* get the serialized buffer and its length */
	TPM_Sbuffer_Get(&sbuffer, &buffer, &length);
    }
    /* validate the length of the stream */
    if (rc == 0) {
	printf("   TPM_VolatileAll_NVStore: Require %u bytes\n", length);
	if (length > TPM_MAX_VOLATILESTATE_SPACE) {
	    printf("TPM_VolatileAll_NVStore: Error, No space, need %u max %u\n",
		   length, TPM_MAX_VOLATILESTATE_SPACE);
	    rc = TPM_NOSPACE;
	}
    }
    if (rc == 0) {
	/* store the buffer in NVRAM */
	rc = TPM_NVRAM_StoreData(buffer,
				 length,
				 tpm_state->tpm_number,
				 TPM_VOLATILESTATE_NAME);
    }
    TPM_Sbuffer_Delete(&sbuffer);	/* @1 */
    return rc;
}

/*
  Compiled in TPM Parameters
*/

TPM_RESULT TPM_Parameters_Load(unsigned char **stream,
			       uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_Parameters_Load:\n");
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_TPM_PARAMETERS_V1,
			  stream, stream_size);
    }
    if (rc == 0) {
	rc = TPM_Parameters_Check8(TPM_MAJOR, "TPM_MAJOR",
				   stream, stream_size);
    }
    if (rc == 0) {
	rc = TPM_Parameters_Check8(TPM_MINOR, "TPM_MINOR",
				   stream, stream_size);
    }
    if (rc == 0) {
	rc = TPM_Parameters_Check16(TPM_PCCLIENT, "TPM_PCCLIENT",
				    stream, stream_size);
    }
    if (rc == 0) {
	rc = TPM_Parameters_Check16(TPM_NUM_PCR, "TPM_NUM_PCR",
				    stream, stream_size);
    }
    if (rc == 0) {
	rc = TPM_Parameters_Check16(TPM_RSA_KEY_LENGTH_MAX, "TPM_RSA_KEY_LENGTH_MAX",
				    stream, stream_size);
    }
    if (rc == 0) {
	rc = TPM_Parameters_Check16(TPM_KEY_HANDLES, "TPM_KEY_HANDLES",
				    stream, stream_size);
    }
    if (rc == 0) {
	rc = TPM_Parameters_Check16(TPM_OWNER_EVICT_KEY_HANDLES, "TPM_OWNER_EVICT_KEY_HANDLES",
				    stream, stream_size);
    }
    if (rc == 0) {
	rc = TPM_Parameters_Check16(TPM_NUM_FAMILY_TABLE_ENTRY_MIN,
				    "TPM_NUM_FAMILY_TABLE_ENTRY_MIN",
				    stream, stream_size);
    }
    if (rc == 0) {
	rc = TPM_Parameters_Check16(TPM_NUM_DELEGATE_TABLE_ENTRY_MIN,
				    "TPM_NUM_DELEGATE_TABLE_ENTRY_MIN",
				    stream, stream_size);
    }
    if (rc == 0) {
	rc = TPM_Parameters_Check16(TPM_MIN_AUTH_SESSIONS, "TPM_MIN_AUTH_SESSIONS",
				    stream, stream_size);
    }
    if (rc == 0) {
	rc = TPM_Parameters_Check16(TPM_MIN_TRANS_SESSIONS, "TPM_MIN_TRANS_SESSIONS",
				    stream, stream_size);
    }
    if (rc == 0) {
	rc = TPM_Parameters_Check16(TPM_MIN_DAA_SESSIONS, "TPM_MIN_DAA_SESSIONS",
				    stream, stream_size);
    }
    if (rc == 0) {
	rc = TPM_Parameters_Check16(TPM_MIN_COUNTERS, "TPM_MIN_COUNTERS",
				    stream, stream_size);
    }
    if (rc == 0) {
	rc = TPM_Parameters_Check16(TPM_MIN_SESSION_LIST, "TPM_MIN_SESSION_LIST",
				    stream, stream_size);
    }
    if (rc == 0) {
	rc = TPM_Parameters_Check32(TPM_MAX_NV_SPACE, "TPM_MAX_NV_SPACE",
				    stream, stream_size);
    }
    return rc;
}

TPM_RESULT TPM_Parameters_Check8(uint8_t expected,
				 const char *parameter,
				 unsigned char **stream,
				 uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;
    uint8_t		tmp8;

    if (rc == 0) {
	rc = TPM_Load8(&tmp8, stream, stream_size);
	if (tmp8 != expected) {
	    printf("TPM_Parameters_Check8: Error (fatal) %s received %u expect %u\n",
		   parameter, tmp8, expected);
	    rc = TPM_FAIL;
	}
    }
    return rc;
}

TPM_RESULT TPM_Parameters_Check16(uint16_t expected,
				  const char *parameter,
				  unsigned char **stream,
				  uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;
    uint16_t		tmp16;

    if (rc == 0) {
	rc = TPM_Load16(&tmp16, stream, stream_size);
	if (tmp16 != expected) {
	    printf("TPM_Parameters_Check16: Error (fatal) %s received %u expect %u\n",
		   parameter, tmp16, expected);
	    rc = TPM_FAIL;
	}
    }
    return rc;
}

TPM_RESULT TPM_Parameters_Check32(uint32_t expected,
				  const char *parameter,
				  unsigned char **stream,
				  uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;
    uint32_t		tmp32;

    if (rc == 0) {
	rc = TPM_Load32(&tmp32, stream, stream_size);
	if (tmp32 != expected) {
	    printf("TPM_Parameters_Check32: Error (fatal) %s received %u expect %u\n",
		   parameter, tmp32, expected);
	    rc = TPM_FAIL;
	}
    }
    return rc;
}

TPM_RESULT TPM_Parameters_Store(TPM_STORE_BUFFER *sbuffer)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_Parameters_Store:\n");
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_TPM_PARAMETERS_V1);
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append8(sbuffer, TPM_MAJOR);
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append8(sbuffer, TPM_MINOR);
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_PCCLIENT);
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_NUM_PCR);
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_RSA_KEY_LENGTH_MAX);
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_KEY_HANDLES);
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_OWNER_EVICT_KEY_HANDLES);
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_NUM_FAMILY_TABLE_ENTRY_MIN);
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_NUM_DELEGATE_TABLE_ENTRY_MIN);
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_MIN_AUTH_SESSIONS);
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_MIN_TRANS_SESSIONS);
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_MIN_DAA_SESSIONS);
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_MIN_COUNTERS);
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_MIN_SESSION_LIST);
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, TPM_MAX_NV_SPACE);
    }
    return rc;
}


/* 27.5 TPM_Reset rev 105

   Releases all resources associated with existing authorization sessions. This is useful if a TSS
   driver has lost track of the state in the TPM.

   This is a deprecated command in V1.2. This command in 1.1 only referenced authorization sessions
   and is not upgraded to affect any other TPM entity in 1.2
*/

TPM_RESULT TPM_Process_Reset(tpm_state_t *tpm_state,
			     TPM_STORE_BUFFER *response,
			     TPM_TAG tag,
			     uint32_t paramSize,
			     TPM_COMMAND_CODE ordinal,
			     unsigned char *command,
			     TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* processing parameters */
    unsigned char *	inParamStart;		/* starting point of inParam's */
    unsigned char *	inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST		inParamDigest;
    TPM_BOOL		auditStatus;		/* audit the ordinal */
    TPM_BOOL		transportEncrypt;	/* wrapped in encrypted transport session */

    /* output parameters */
    uint32_t		outParamStart;		/* starting point of outParam's */
    uint32_t		outParamEnd;		/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_Reset: Ordinal Entry\n");
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
	    printf("TPM_Process_Reset: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. The TPM invalidates all resources allocated to authorization sessions as per version 1.1
       extant in the TPM */
    /* a. This includes structures created by TPM_SaveAuthContext and TPM_SaveKeyContext */
    /* b.The TPM MUST invalidate OSAP sessions */
    /* c.The TPM MAY invalidate DSAP sessions */
    /* d. The TPM MUST NOT invalidate structures created by TPM_SaveContext */
    if (returnCode == TPM_SUCCESS) {
	TPM_StclearData_AuthSessionDelete(&(tpm_state->tpm_stclear_data));
    }
    /* 2. The TPM does not reset any PCR or DIR values. */
    /* 3. The TPM does not reset any flags in the TPM_STCLEAR_FLAGS structure. */
    /* 4. The TPM does not reset or invalidate any keys */
    /*
      response
    */
    if (rcf == 0) {
	printf("TPM_Process_Reset: Ordinal returnCode %08x %u\n",
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
    return rcf;
}

/* 3.2 TPM_Startup rev 101
   
   TPM_Startup is always preceded by TPM_Init, which is the physical indication (a system-wide
   reset) that TPM initialization is necessary.

   There are many events on a platform that can cause a reset and the response to these events can
   require different operations to occur on the TPM. The mere reset indication does not contain
   sufficient information to inform the TPM as to what type of reset is occurring. Additional
   information known by the platform initialization code needs transmitting to the TPM. The
   TPM_Startup command provides the mechanism to transmit the information.
   
   The TPM can startup in three different modes:

   A "clear" start where all variables go back to their default or non-volatile set state

   A "save" start where the TPM recovers appropriate information and restores various values based
   on a prior TPM_SaveState. This recovery requires an invocation of TPM_Init to be successful.

   A failing "save" start must shut down the TPM. The CRTM cannot leave the TPM in a state where an
   untrusted upper software layer could issue a "clear" and then extend PCR's and thus mimic the
   CRTM.

   A "deactivated" start where the TPM turns itself off and requires another TPM_Init before the TPM
   will execute in a fully operational state.  The TPM can startup in three different modes:
*/

TPM_RESULT TPM_Process_Startup(tpm_state_t *tpm_state,
			       TPM_STORE_BUFFER *response,
			       TPM_TAG tag,
			       uint32_t paramSize,
			       TPM_COMMAND_CODE ordinal,
			       unsigned char *command,
			       TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT		rcf = 0;			/* fatal error precluding response */
    TPM_RESULT		returnCode = TPM_SUCCESS;	/* command return code */
    TPM_RESULT		returnCode1 = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_STARTUP_TYPE	startupType;

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_Startup: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get startupType parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load16(&startupType, &command, &paramSize);
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
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_Startup: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* TPM_CheckState() can check for the normal case where postInitialise TRUE is an error.  This
       is the only command where FALSE is the error. */
    if (returnCode == TPM_SUCCESS) {
	/* 1. If TPM_STANY_FLAGS -> postInitialise is FALSE,  */
	if (!(tpm_state->tpm_stany_flags.postInitialise)) {
	    /* a. Then the TPM MUST return TPM_INVALID_POSTINIT, and exit this capability */
	    printf("TPM_Process_Startup: Error, postInitialise is FALSE\n");
	    returnCode = TPM_INVALID_POSTINIT;
	}
    }
    if (returnCode == TPM_SUCCESS) {
	/* 1. If the TPM is in failure mode */
	if (tpm_state->testState == TPM_TEST_STATE_FAILURE) {
	    /* a. TPM_STANY_FLAGS -> postInitialize is still set to FALSE */
	    tpm_state->tpm_stany_flags.postInitialise = FALSE;
	    printf("TPM_Process_Startup: Error, shutdown is TRUE\n");
	    /* b. The TPM returns TPM_FAILEDSELFTEST */
	    returnCode = TPM_FAILEDSELFTEST;
	}
    }
    /*
      Processing
    */
    if (returnCode == TPM_SUCCESS) {
	switch (startupType) {
	  case TPM_ST_CLEAR:		/* The TPM is starting up from a clean state */
	    returnCode = TPM_Startup_Clear(tpm_state);
	    break;
	  case TPM_ST_STATE:		/* The TPM is starting up from a saved state */
	    returnCode = TPM_Startup_State(tpm_state);
	    break;
	  case TPM_ST_DEACTIVATED:	/* The TPM is to startup and set the deactivated flag to
					   TRUE */
	    returnCode = TPM_Startup_Deactivated(tpm_state);
	    break;
	  default:
	    returnCode = TPM_BAD_PARAMETER;
	    break;
	}
    }
    /* TPM_STANY_FLAGS MUST reset on TPM_Startup(any) */
    if (returnCode == TPM_SUCCESS) {
	TPM_StanyFlags_Init(&(tpm_state->tpm_stany_flags));
    }
    /* 5. The TPM MUST ensure that state associated with TPM_SaveState is invalidated */
    returnCode1 = TPM_SaveState_NVDelete(tpm_state,
					 FALSE);	/* Ignore errors if the state does not
							   exist. */
    if (returnCode == TPM_SUCCESS) {			/* previous error takes precedence */
	returnCode = returnCode1;
    }
    /* 6. The TPM MUST set TPM_STANY_FLAGS -> postInitialise to FALSE */
    tpm_state->tpm_stany_flags.postInitialise = FALSE;
    /*
      response
    */
    if (rcf == 0) {
	printf("TPM_Process_Startup: Ordinal returnCode %08x %u\n",
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
    return rcf;
}

/* 3.2 TPM_Startup(TPM_ST_CLEAR) rev 99
*/

TPM_RESULT TPM_Startup_Clear(tpm_state_t *tpm_state)
{	
    TPM_RESULT		returnCode = TPM_SUCCESS;	
    
    printf("TPM_Startup_Clear:\n");	
    /* 2. If stType = TPM_ST_CLEAR */	
    if (returnCode == TPM_SUCCESS) {	
	/* a. Ensure that sessions associated with resources TPM_RT_CONTEXT, TPM_RT_AUTH,
	   TPM_RT_DAA_TPM, and TPM_RT_TRANS are invalidated */
	/* NOTE TPM_RT_CONTEXT -
	   contextNonceKey cleared by TPM_Global_Init() -> TPM_StanyData_Init()
	   contextNonceSession cleared by TPM_Global_Init() -> TPM_StanyData_Init()
	*/
	/* NOTE TPM_RT_AUTH - TPM_AuthSessions_Init() called by TPM_Global_Init() ->
	   TPM_StanyData_Init() */
	/* TPM_RT_TRANS - TPM_TransportSessions_Init() called by TPM_Global_Init() ->
	   TPM_StanyData_Init()*/
	/* TPM_RT_DAA_TPM - TPM_DaaSessions_Init() called by TPM_Global_Init() ->
	   TPM_StanyData_Init()*/
	/* b. Reset PCR values to each correct default value */
	/* i. pcrReset is FALSE, set to 0x00..00 */
	/* ii. pcrReset is TRUE, set to 0xFF..FF */
	/* NOTE done by TPM_MainInit() -> TPM_Global_Init() */
	/* c. Set the following TPM_STCLEAR_FLAGS to their default state 
	   i. PhysicalPresence 
	   ii. PhysicalPresenceLock 
	   iii. disableForceClear
	*/
	/* NOTE Done by TPM_Global_Init() -> TPM_StclearFlags_Init() */
	/* d. The TPM MAY initialize auditDigest to all zeros 
	   i. If not initialized to all zeros the TPM SHALL ensure that auditDigest contains a valid
	   value
	   ii. If initialization fails the TPM SHALL set auditDigest to all zeros and SHALL set the
	   internal TPM state so that the TPM returns TPM_FAILEDSELFTEST to all subsequent
	   commands.
	*/
	/* NOTE Done by TPM_Global_Init() ->TPM_StanyData_Init() */
	/* e.  The TPM SHALL set TPM_STCLEAR_FLAGS -> deactivated to the same state as
	   TPM_PERMANENT_FLAGS -> deactivated
	*/
	tpm_state->tpm_stclear_flags.deactivated = tpm_state->tpm_permanent_flags.deactivated;
	/* f. The TPM MUST set the TPM_STANY_DATA fields to: */
	/* i. TPM_STANY_DATA->contextNonceSession is set to all zeros */
	/* ii. TPM_STANY_DATA->contextCount is set to 0 */
	/* iii. TPM_STANY_DATA->contextList is set to 0 */
	/* NOTE Done by TPM_Global_Init() ->TPM_StanyData_Init() */
	/* g. The TPM MUST set TPM_STCLEAR_DATA fields to: */
	/* i. Invalidate contextNonceKey */
	/* ii. countID to zero */
	/* iii. OwnerReference to TPM_KH_OWNER */
	/* NOTE Done by TPM_Global_Init() -> TPM_StclearData_Init() */
	/* h. The TPM MUST set the following TPM_STCLEAR_FLAGS to */
	/* i. bGlobalLock to FALSE */
	/* NOTE Done by TPM_Global_Init() -> TPM_StclearFlags_Init() */
	/* i. Determine which keys should remain in the TPM */
	/* i. For each key that has a valid preserved value in the TPM */
	/* (1) if parentPCRStatus is TRUE then call TPM_FlushSpecific(keyHandle) */
	/* (2) if isVolatile is TRUE then call TPM_FlushSpecific(keyHandle) */
	/* NOTE Since TPM_Global_Init() calls TPM_KeyHandleEntries_Init(), there are no keys
	   remaining.  Since this TPM implementation loads keys into volatile memory, not NVRAM, no
	   keys are preserved at ST_CLEAR. */
	/* ii. Keys under control of the OwnerEvict flag MUST stay resident in the TPM */
	/* NOTE Done by TPM_PermanentAll_NVLoad() */
	/* bReadSTClear and bWriteSTClear are volatile, in that they are set FALSE at
	   TPM_Startup(ST_Clear) */
	TPM_NVIndexEntries_StClear(&(tpm_state->tpm_nv_index_entries));
    }
    return returnCode;
}

/*  3.2 TPM_Startup(TPM_ST_STATE) rev 100
 */
	    
TPM_RESULT TPM_Startup_State(tpm_state_t *tpm_state)
{
    TPM_RESULT		returnCode = TPM_SUCCESS;
    
    printf("TPM_Startup_State:\n");
    if (returnCode == TPM_SUCCESS) {
	/* a. If the TPM has no state to restore the TPM MUST set the internal state such that it
	   returns TPM_FAILEDSELFTEST to all subsequent commands */
	/* b. The TPM MAY determine for each session type (authorization, transport, DAA, ...) to
	   release or maintain the session information. The TPM reports how it manages sessions in
	   the TPM_GetCapability command. */
	/* c.  The TPM SHALL take all necessary actions to ensure that all PCRs contain valid
	   preserved values. If the TPM is unable to successfully complete these actions, it SHALL
	   enter the TPM failure mode. */
	/* i. For resettable PCR the TPM MUST set the value of TPM_STCLEAR_DATA -> PCR[] to the
	   resettable PCR default value.  The TPM MUST NOT restore a resettable PCR to a preserved
	   value */
	/* d. The TPM MAY initialize auditDigest to all zeros */
	/* i. Otherwise, the TPM SHALL take all actions necessary to ensure that auditDigest
	   contains a valid value. If the TPM is unable to successfully complete these actions, the
	   TPM SHALL initialize auditDigest to all zeros and SHALL set the internal state such that
	   the TPM returns TPM_FAILEDSELFTEST to all subsequent commands. */
	/* e. The TPM MUST restore the following flags to their preserved states: */
	/* i. All values in TPM_STCLEAR_FLAGS */
	/* ii. All values in TPM_STCLEAR_DATA  */
	/* f. The TPM MUST restore all keys that have a valid preserved value */
	/* NOTE Owner evict keys are loaded at TPM_PermanentAll_NVLoad() */
	returnCode = TPM_SaveState_NVLoad(tpm_state);	/* returns TPM_RETRY on non-existent file */
    }
    /* g. The TPM resumes normal operation. If the TPM is unable to resume normal operation, it
       SHALL enter the TPM failure mode. */
    if (returnCode != TPM_SUCCESS) {
	printf("TPM_Startup_State: Error restoring state\n");
	returnCode = TPM_FAILEDSELFTEST;
	printf("  TPM_Startup_State: Set testState to %u \n", TPM_TEST_STATE_FAILURE);
	tpm_state->testState = TPM_TEST_STATE_FAILURE;
    }
    return returnCode;
}

/*  3.2 TPM_Startup(TPM_ST_DEACTIVATED) rev 97
 */
	    
TPM_RESULT TPM_Startup_Deactivated(tpm_state_t *tpm_state)
{
    TPM_RESULT		returnCode = TPM_SUCCESS;
    
    printf("TPM_Startup_Deactivated:\n");
    if (returnCode == TPM_SUCCESS) {
	/* a. Invalidate sessions */
	/* i. Ensure that all resources associated with saved and active sessions are invalidated */
	/* NOTE Done at TPM_MainInit() */
	/* b. The TPM MUST set TPM_STCLEAR_FLAGS -> deactivated to TRUE */
	tpm_state->tpm_stclear_flags.deactivated = TRUE;
    }
    return returnCode;
}

#if 0
/* TPM_Startup_Any() rev 96

   Handles Actions common to all TPM_Startup options.
*/

TPM_RESULT TPM_Startup_Any(tpm_state_t *tpm_state)
{
    TPM_RESULT		returnCode = TPM_SUCCESS;
    
    printf("TPM_Startup_Any:\n");	
    /* TPM_STANY_FLAGS MUST reset on TPM_Startup(any) */
    TPM_StanyFlags_Init(&(tpm_state->tpm_stany_flags));
    /* 5. The TPM MUST ensure that state associated with TPM_SaveState is invalidated */
    returnCode = TPM_SaveState_NVDelete(tpm_state,
					FALSE);	   /* Ignore errors if the state does not exist. */
    /* 6. The TPM MUST set TPM_STANY_FLAGS -> postInitialise to FALSE */
    tpm_state->tpm_stany_flags.postInitialise = FALSE;
    return returnCode;
}
#endif

/* 3.3 TPM_SaveState rev 111

   This warns a TPM to save some state information.

   If the relevant shielded storage is non-volatile, this command need have no effect.

   If the relevant shielded storage is volatile and the TPM alone is unable to detect the loss of
   external power in time to move data to non-volatile memory, this command should be presented
   before the TPM enters a low or no power state.

   Resettable PCRs are tied to platform state that does not survive a sleep state.  If the PCRs did
   not reset, they would falsely indicate that the platform state was already present when it came
   out of sleep.  Since some setup is required first, there would be a gap where PCRs indicated the
   wrong state.	 Therefore, the PCRs must be recreated.
*/

TPM_RESULT TPM_Process_SaveState(tpm_state_t *tpm_state,
				 TPM_STORE_BUFFER *response,
				 TPM_TAG tag,
				 uint32_t paramSize,
				 TPM_COMMAND_CODE ordinal,
				 unsigned char *command,
				 TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT		rcf = 0;			/* fatal error precluding response */
    TPM_RESULT		returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */

    /* processing parameters */
    unsigned char *	inParamStart;		/* starting point of inParam's */
    unsigned char *	inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST		inParamDigest;
    TPM_BOOL		auditStatus;		/* audit the ordinal */
    TPM_BOOL		transportEncrypt;	/* wrapped in encrypted transport session */

    /* output parameters */
    uint32_t		outParamStart;		/* starting point of outParam's */
    uint32_t		outParamEnd;		/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_SaveState: Ordinal Entry\n");
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
	    printf("TPM_Process_SaveState: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. Preserved values MUST be non-volatile. */
    /* 2. If data is never stored in a volatile medium, that data MAY be used as preserved data. In
       such cases, no explicit action may be required to preserve that data. */
    /* 3. If an explicit action is required to preserve data, it MUST be possible for the TPM to
       determine whether preserved data is valid. */
    /* 4. If the parameter mirrored by a preserved value is altered, all preserved values MUST be
       declared invalid. */
    if (returnCode == TPM_SUCCESS) {
	/* Determine if TPM_SaveState was called from within a transport session.  The TPM MAY save
	   transport sessions as part of the saved state.  Since this TPM implements that option,
	   there's no point in saving the state, because it would be immediately invalidated during
	   the transport response.  Return an error to indicate that the state was not saved. */
	if (transportInternal != NULL) {
	    printf("TPM_Process_SaveState: Error, called from transport session\n");
	    returnCode = TPM_NO_WRAP_TRANSPORT;
	}
    }
    /* Audit Generation Corner cases 3.a. TPM_SaveState: Only the input parameters are audited, and
       the audit occurs before the state is saved.  If an error occurs while or after the state is
       saved, the audit still occurs.
    */
    if ((returnCode == TPM_SUCCESS) && auditStatus) {
	returnCode = TPM_ProcessAudit(tpm_state,
				      transportEncrypt,
				      inParamDigest,
				      outParamDigest,
				      ordinal);
    }
    /* 5. The TPM MAY declare all preserved value is invalid in response to any command other that
	  TPM_Init. */
    /* NOTE Done by TPM_GetInParamDigest(), which is called by all ordinals */
    /* 1. Store TPM_STCLEAR_DATA -> PCR contents except for */
    /* a. If the PCR attribute pcrReset is TRUE */
    /* b. Any platform identified debug PCR */
    /* 2. The auditDigest MUST be handled according to the audit requirements as reported by
       TPM_GetCapability */
    /* a. If the ordinalAuditStatus is TRUE for the TPM_SaveState ordinal and the auditDigest is
       being stored in the saved state, the saved auditDigest MUST include the TPM_SaveState input
       parameters and MUST NOT include the output parameters. */
    /* 3. All values in TPM_STCLEAR_DATA MUST be preserved */
    /* 4. All values in TPM_STCLEAR_FLAGS MUST be preserved */
    /* 5. The contents of any key that is currently loaded SHOULD be preserved if the key's
       parentPCRStatus indicator is TRUE. */
    /* 6. The contents of any key that has TPM_KEY_CONTROL_OWNER_EVICT set MUST be preserved */
    /* 7. The contents of any key that is currently loaded MAY be preserved */
    /* 8. The contents of sessions (authorization, transport, DAA etc.) MAY be preserved as reported
       by TPM_GetCapability */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SaveState_NVStore(tpm_state);
    }
    /* store the state in NVRAM */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_SaveState: Ordinal returnCode %08x %u\n",
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
	/* Special case, no output parameter audit */
	/* adjust the initial response */
	rcf = TPM_Sbuffer_StoreFinalResponse(response, returnCode, tpm_state);
    }
    /*
      cleanup
    */
    return rcf;
}
