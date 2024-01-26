/********************************************************************************/
/*										*/
/*				DAA Handler					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_daa.c $		*/
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
#include "tpm_crypto.h"
#include "tpm_cryptoh.h"
#include "tpm_debug.h"
#include "tpm_error.h"
#include "tpm_init.h"
#include "tpm_key.h"
#include "tpm_memory.h"
#include "tpm_nonce.h"
#include "tpm_process.h"
#include "tpm_sizedbuffer.h"

#include "tpm_daa.h"

/*
  TPM_DAA_SESSION_DATA	(the entire array)
*/

void TPM_DaaSessions_Init(TPM_DAA_SESSION_DATA *daaSessions)
{
    size_t i;
    
    printf(" TPM_DaaSessions_Init:\n");
    for (i = 0 ; i < TPM_MIN_DAA_SESSIONS ; i++) {
	TPM_DaaSessionData_Init(&(daaSessions[i]));
    }
    return;
}

/* TPM_DaaSessions_Load() reads a count of the number of stored sessions and then loads those
   sessions.

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_DaaSessions_Init()
*/

TPM_RESULT TPM_DaaSessions_Load(TPM_DAA_SESSION_DATA *daaSessions,
				unsigned char **stream,
				uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;
    size_t		i;
    uint32_t		activeCount;

    printf(" TPM_DaaSessions_Load:\n");
    /* load active count */
    if (rc == 0) {
	rc = TPM_Load32(&activeCount, stream, stream_size);
    }
    if (rc == 0) {
	if (activeCount > TPM_MIN_DAA_SESSIONS) {
	    printf("TPM_DaaSessions_Load: Error (fatal) %u sessions, %u slots\n",
		   activeCount, TPM_MIN_DAA_SESSIONS);
	    rc = TPM_FAIL;
	}
    }    
    if (rc == 0) {
	printf(" TPM_DaaSessions_Load: Loading %u sessions\n", activeCount);
    }
    /* load DAA sessions */
    for (i = 0 ; (rc == 0) && (i < activeCount) ; i++) {
	rc = TPM_DaaSessionData_Load(&(daaSessions[i]), stream, stream_size);
    }
    return rc;
}

/* TPM_DaaSessions_Store() stores a count of the active sessions, followed by the sessions.
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_DaaSessions_Store(TPM_STORE_BUFFER *sbuffer,
				 TPM_DAA_SESSION_DATA *daaSessions)
{
    TPM_RESULT		rc = 0;
    size_t		i;
    uint32_t		space;
    uint32_t		activeCount;
    
    /* store active count */
    if (rc == 0) {
	TPM_DaaSessions_GetSpace(&space, daaSessions);
	activeCount = TPM_MIN_DAA_SESSIONS - space;
	printf(" TPM_DaaSessions_Store: Storing %u sessions\n", activeCount);
	rc = TPM_Sbuffer_Append32(sbuffer, activeCount);
    }
    /* store DAA sessions */
    for (i = 0 ; (rc == 0) && (i < TPM_MIN_DAA_SESSIONS) ; i++) {
	if ((daaSessions[i]).valid) {  /* if the session is active */
	    rc = TPM_DaaSessionData_Store(sbuffer, &(daaSessions[i]));
	}
    }
    return rc;
}

/* TPM_DaaSessions_Delete() terminates all loaded DAA sessions

*/

void TPM_DaaSessions_Delete(TPM_DAA_SESSION_DATA *daaSessions)
{
    size_t i;
    
    printf(" TPM_DaaSessions_Delete:\n");
    for (i = 0 ; i < TPM_MIN_DAA_SESSIONS ; i++) {
	TPM_DaaSessionData_Delete(&(daaSessions[i]));
    }
    return;
}

/* TPM_DaaSessions_IsSpace() returns 'isSpace' TRUE if an entry is available, FALSE if not.

   If TRUE, 'index' holds the first free position.
*/

void TPM_DaaSessions_IsSpace(TPM_BOOL *isSpace,
			     uint32_t *index,
			     TPM_DAA_SESSION_DATA *daaSessions)
{
    printf(" TPM_DaaSessions_IsSpace:\n");
    for (*index = 0, *isSpace = FALSE ; *index < TPM_MIN_DAA_SESSIONS ; (*index)++) {
	if (!((daaSessions[*index]).valid)) {
	    printf("  TPM_DaaSessions_IsSpace: Found space at %u\n", *index);
	    *isSpace = TRUE;
	    break;
	}	    
    }
    return;
}

/* TPM_DaaSessions_GetSpace() returns the number of unused daaHandle's.

*/

void TPM_DaaSessions_GetSpace(uint32_t *space,
			      TPM_DAA_SESSION_DATA *daaSessions)
{
    uint32_t i;

    printf(" TPM_DaaSessions_GetSpace:\n");
    for (*space = 0 , i = 0 ; i < TPM_MIN_DAA_SESSIONS ; i++) {
	if (!((daaSessions[i]).valid)) {
	    (*space)++;
	}	    
    }
    return;
}

/* TPM_DaaSessions_StoreHandles() stores

   - the number of loaded sessions
   - a list of session handles
*/

TPM_RESULT TPM_DaaSessions_StoreHandles(TPM_STORE_BUFFER *sbuffer,
					TPM_DAA_SESSION_DATA *daaSessions)
{
    TPM_RESULT	rc = 0;
    uint16_t	i;
    uint32_t	space;
    
    printf(" TPM_DaaSessions_StoreHandles:\n");
    /* get the number of loaded handles */
    if (rc == 0) {
	TPM_DaaSessions_GetSpace(&space, daaSessions);
	/* store loaded handle count.  Safe case because of TPM_MIN_DAA_SESSIONS value */
	rc = TPM_Sbuffer_Append16(sbuffer, (uint16_t)(TPM_MIN_DAA_SESSIONS - space)); 
    }
    for (i = 0 ; (rc == 0) && (i < TPM_MIN_DAA_SESSIONS) ; i++) {
	if ((daaSessions[i]).valid) {		       /* if the index is loaded */
	    rc = TPM_Sbuffer_Append32(sbuffer, (daaSessions[i]).daaHandle);	/* store it */
	}
    }
    return rc;
}

/* TPM_DaaSessions_GetNewHandle() checks for space in the DAA sessions table.

   If there is space, it returns a TPM_DAA_SESSION_DATA entry in 'tpm_daa_session_data' and its
   handle in 'daaHandle'.  The entry is marked 'valid'.

   If *daaHandle non-zero, the suggested value is tried first.

   Returns TPM_RESOURCES if there is no space in the sessions table.
*/

TPM_RESULT TPM_DaaSessions_GetNewHandle(TPM_DAA_SESSION_DATA **tpm_daa_session_data, /* entry */
					TPM_HANDLE *daaHandle,
					TPM_BOOL *daaHandleValid,
					TPM_DAA_SESSION_DATA *daaSessions)	/* array */
{
    TPM_RESULT			rc = 0;
    uint32_t			index;
    TPM_BOOL			isSpace;
    
    printf(" TPM_DaaSessions_GetNewHandle:\n");
    *daaHandle = FALSE;
    /* is there an empty entry, get the location index */
    if (rc == 0) {
	TPM_DaaSessions_IsSpace(&isSpace,	/* TRUE if space available */
				&index,		/* if space available, index into array */
				daaSessions);	/* array */
	if (!isSpace) {
	    printf("TPM_DaaSessions_GetNewHandle: Error, no space in daaSessions table\n");
	    rc = TPM_RESOURCES;
	}
    }
    if (rc == 0) {
	rc = TPM_Handle_GenerateHandle(daaHandle,		/* I/O, pointer to handle */
				       daaSessions,		/* handle array */
				       FALSE,			/* keepHandle */
				       FALSE,			/* isKeyHandle */
				       (TPM_GETENTRY_FUNCTION_T)TPM_DaaSessions_GetEntry);
    }
    if (rc == 0) {
	printf("  TPM_DaaSessions_GetNewHandle: Assigned handle %08x\n", *daaHandle);
	*tpm_daa_session_data = &(daaSessions[index]);
	TPM_DaaSessionData_Init(*tpm_daa_session_data); /* should be redundant since
								      terminate should have done
								      this */
	(*tpm_daa_session_data)->daaHandle = *daaHandle;
	(*tpm_daa_session_data)->valid = TRUE;
	*daaHandleValid = TRUE;
    }
    return rc;
}

/* TPM_DaaSessions_GetEntry() searches all entries for the entry matching the handle, and
   returns the TPM_DAA_SESSION_DATA entry associated with the handle.

   Returns
   0 for success
   TPM_BAD_HANDLE if the handle is not found
*/

TPM_RESULT TPM_DaaSessions_GetEntry(TPM_DAA_SESSION_DATA **tpm_daa_session_data, /* session for
										    daaHandle */
				    TPM_DAA_SESSION_DATA *daaSessions, /* points to first session in
									  array */
				    TPM_HANDLE daaHandle)	/* input */
{
    TPM_RESULT	rc = 0;
    size_t	i;
    TPM_BOOL	found;
    
    printf(" TPM_DaaSessions_GetEntry: daaHandle %08x\n", daaHandle);
    for (i = 0, found = FALSE ; (i < TPM_MIN_DAA_SESSIONS) && !found ; i++) {
	if ((daaSessions[i].valid) &&		   
	    (daaSessions[i].daaHandle == daaHandle)) {	  /* found */
	    found = TRUE;
	    *tpm_daa_session_data = &(daaSessions[i]);
	}
    }
    if (!found) {
	printf("  TPM_DaaSessions_GetEntry: session handle %08x not found\n",
	       daaHandle);
	rc = TPM_BAD_HANDLE;
    }
    return rc;
}

/* TPM_DaaSessions_AddEntry() adds an TPM_DAA_SESSION_DATA object to the list.

   If *tpm_handle == 0, a value is assigned.  If *tpm_handle != 0, that value is used if it it not
   currently in use.

   The handle is returned in tpm_handle.
*/

TPM_RESULT TPM_DaaSessions_AddEntry(TPM_HANDLE *tpm_handle,			/* i/o */
				    TPM_BOOL keepHandle,			/* input */
				    TPM_DAA_SESSION_DATA *daaSessions,		/* input */
				    TPM_DAA_SESSION_DATA *tpm_daa_session_data) /* input */
{
    TPM_RESULT			rc = 0;
    uint32_t			index;
    TPM_BOOL			isSpace;
    
    printf(" TPM_DaaSessions_AddEntry:\n");
    /* check for valid TPM_DAA_SESSION_DATA */
    if (rc == 0) {
	if (tpm_daa_session_data == NULL) {	/* NOTE: should never occur */
	    printf("TPM_DaaSessions_AddEntry: Error (fatal), NULL TPM_DAA_SESSION_DATA\n");
	    rc = TPM_FAIL;
	}
    }
    /* is there an empty entry, get the location index */
    if (rc == 0) {
	TPM_DaaSessions_IsSpace(&isSpace, &index, daaSessions);
	if (!isSpace) {
	    printf("TPM_DaaSessions_AddEntry: Error, session entries full\n");
	    rc = TPM_RESOURCES;
	}
    }
    if (rc == 0) {
	rc = TPM_Handle_GenerateHandle(tpm_handle,		/* I/O */
				       daaSessions,		/* handle array */
				       keepHandle,		/* keepHandle */
				       FALSE,			/* isKeyHandle */
				       (TPM_GETENTRY_FUNCTION_T)TPM_DaaSessions_GetEntry);
    }
    if (rc == 0) {
	TPM_DaaSessionData_Copy(&(daaSessions[index]), *tpm_handle, tpm_daa_session_data);
	daaSessions[index].valid = TRUE;
	printf("  TPM_DaaSessions_AddEntry: Index %u handle %08x\n",
	       index, daaSessions[index].daaHandle);
    }
    return rc;
}

/* TPM_DaaSessions_TerminateHandle() terminates the session associated with 'daaHandle'.

*/

TPM_RESULT TPM_DaaSessions_TerminateHandle(TPM_DAA_SESSION_DATA *daaSessions,
					   TPM_HANDLE daaHandle)
{
    TPM_RESULT	rc = 0;
    TPM_DAA_SESSION_DATA *tpm_daa_session_data;

    printf(" TPM_DaaSessions_TerminateHandle: daaHandle %08x\n", daaHandle);
    /* get the TPM_DAA_SESSION_DATA associated with the TPM_HANDLE */
    if (rc == 0) {
	rc = TPM_DaaSessions_GetEntry(&tpm_daa_session_data,	/* returns entry in array */
				      daaSessions,		/* array */
				      daaHandle);
    }
    /* invalidate the valid handle */
    if (rc == 0) {
	TPM_DaaSessionData_Delete(tpm_daa_session_data);
    }
    return rc;
}

/*
  TPM_DAA_SESSION_DATA (one element of the array)
*/

/* TPM_DaaSessionData_Init() initializes the DAA session.

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_DaaSessionData_Init(TPM_DAA_SESSION_DATA *tpm_daa_session_data)
{
    printf(" TPM_DaaSessionData_Init:\n");
    TPM_DAAIssuer_Init(&(tpm_daa_session_data->DAA_issuerSettings));
    TPM_DAATpm_Init(&(tpm_daa_session_data->DAA_tpmSpecific));
    TPM_DAAContext_Init(&(tpm_daa_session_data->DAA_session));
    TPM_DAAJoindata_Init(&(tpm_daa_session_data->DAA_joinSession)); 
    tpm_daa_session_data->daaHandle = 0;
    tpm_daa_session_data->valid = FALSE;
    return;
}

/* TPM_DaaSessionData_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_DaaSessionData_Init()
   After use, call TPM_DaaSessionData_Delete() to free memory
*/

TPM_RESULT TPM_DaaSessionData_Load(TPM_DAA_SESSION_DATA *tpm_daa_session_data,
				   unsigned char **stream,
				   uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_DaaSessionData_Load:\n");
    /* load DAA_issuerSettings */
    if (rc == 0) {
	rc = TPM_DAAIssuer_Load(&(tpm_daa_session_data->DAA_issuerSettings), stream, stream_size);
    }
    /* load DAA_tpmSpecific */
    if (rc == 0) {
	rc = TPM_DAATpm_Load(&(tpm_daa_session_data->DAA_tpmSpecific), stream, stream_size);
    }
    /* load DAA_session */
    if (rc == 0) {
	rc = TPM_DAAContext_Load(&(tpm_daa_session_data->DAA_session),stream, stream_size);
    }
    /* load DAA_joinSession */
    if (rc == 0) {
	rc = TPM_DAAJoindata_Load(&(tpm_daa_session_data->DAA_joinSession), stream, stream_size);
    }
    /* load daaHandle */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_daa_session_data->daaHandle), stream, stream_size);
    }
    /* set valid */
    if (rc == 0) {
	tpm_daa_session_data->valid = TRUE;
    }
    return rc;
}

/* TPM_DaaSessionData_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_DaaSessionData_Store(TPM_STORE_BUFFER *sbuffer,
				    const TPM_DAA_SESSION_DATA *tpm_daa_session_data)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_DaaSessionData_Store:\n");
    /* store DAA_issuerSettings */
    if (rc == 0) {
	rc = TPM_DAAIssuer_Store(sbuffer, &(tpm_daa_session_data->DAA_issuerSettings));
    }
    /* store DAA_tpmSpecific */
    if (rc == 0) {
	rc = TPM_DAATpm_Store(sbuffer, &(tpm_daa_session_data->DAA_tpmSpecific));
    }
    /* store DAA_session */
    if (rc == 0) {
	rc = TPM_DAAContext_Store(sbuffer, &(tpm_daa_session_data->DAA_session));
    }
    /* store DAA_joinSession */
    if (rc == 0) {
	rc = TPM_DAAJoindata_Store(sbuffer, &(tpm_daa_session_data->DAA_joinSession));
    }
    /* store daaHandle */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_daa_session_data->daaHandle);
    }
    return rc;
}

/* TPM_DaaSessionData_Delete() terminates the DAA session.

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_DaaSessionData_Init to set members back to default values
   The object itself is not freed
*/

void TPM_DaaSessionData_Delete(TPM_DAA_SESSION_DATA *tpm_daa_session_data)
{
    printf(" TPM_DaaSessionData_Delete:\n");
    if (tpm_daa_session_data != NULL) {
	TPM_DAAIssuer_Delete(&(tpm_daa_session_data->DAA_issuerSettings));
	TPM_DAATpm_Delete(&(tpm_daa_session_data->DAA_tpmSpecific));
	TPM_DAAContext_Delete(&(tpm_daa_session_data->DAA_session));
	TPM_DAAJoindata_Delete(&(tpm_daa_session_data->DAA_joinSession)); 
	TPM_DaaSessionData_Init(tpm_daa_session_data);
    }
    return;
}

/* TPM_DaaSessionData_Copy() copies the source to the destination.  The source handle is ignored,
   since it might already be used.
*/

void TPM_DaaSessionData_Copy(TPM_DAA_SESSION_DATA *dest_daa_session_data,
			     TPM_HANDLE tpm_handle,
			     TPM_DAA_SESSION_DATA *src_daa_session_data)
{
    dest_daa_session_data->daaHandle = tpm_handle;
    TPM_DAAIssuer_Copy(&(dest_daa_session_data->DAA_issuerSettings),
		       &(src_daa_session_data->DAA_issuerSettings));
    TPM_DAATpm_Copy(&(dest_daa_session_data->DAA_tpmSpecific),
		    &(src_daa_session_data->DAA_tpmSpecific));
    TPM_DAAContext_Copy(&(dest_daa_session_data->DAA_session),
			&(src_daa_session_data->DAA_session));
    TPM_DAAJoindata_Copy(&(dest_daa_session_data->DAA_joinSession),
			 &(src_daa_session_data->DAA_joinSession));
    dest_daa_session_data->valid= src_daa_session_data->valid;
    return;
}

/* TPM_DaaSessionData_CheckStage() verifies that the actual command processing stage is consistent
   with the stage expected by the TPM state.
*/

TPM_RESULT TPM_DaaSessionData_CheckStage(TPM_DAA_SESSION_DATA *tpm_daa_session_data,
					 BYTE stage)
{
    TPM_RESULT		rc = 0;
    
    printf(" TPM_DaaSessionData_CheckStage:\n");
    if (tpm_daa_session_data->DAA_session.DAA_stage != stage) {
	printf("TPM_DaaSessionData_CheckStage: Error, stage expected %u actual %u\n",
	       tpm_daa_session_data->DAA_session.DAA_stage, stage);
	rc = TPM_DAA_STAGE;
    }
    return rc;
}

/*
  TPM_DAA_ISSUER
*/

/* TPM_DAAIssuer_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_DAAIssuer_Init(TPM_DAA_ISSUER *tpm_daa_issuer)
{
    printf(" TPM_DAAIssuer_Init:\n");
    
    TPM_Digest_Init(tpm_daa_issuer->DAA_digest_R0);
    TPM_Digest_Init(tpm_daa_issuer->DAA_digest_R1);
    TPM_Digest_Init(tpm_daa_issuer->DAA_digest_S0);
    TPM_Digest_Init(tpm_daa_issuer->DAA_digest_S1);
    TPM_Digest_Init(tpm_daa_issuer->DAA_digest_n);
    TPM_Digest_Init(tpm_daa_issuer->DAA_digest_gamma);
    memset(tpm_daa_issuer->DAA_generic_q, 0, sizeof(tpm_daa_issuer->DAA_generic_q));
    return;
}

/* TPM_DAAIssuer_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_DAAIssuer_Init()
   After use, call TPM_DAAIssuer_Delete() to free memory
*/

TPM_RESULT TPM_DAAIssuer_Load(TPM_DAA_ISSUER *tpm_daa_issuer,
			      unsigned char **stream,
			      uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_DAAIssuer_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_DAA_ISSUER, stream, stream_size);
    }
    /* load DAA_digest_R0 */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_daa_issuer->DAA_digest_R0, stream, stream_size);
    }
    /* load DAA_digest_R1 */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_daa_issuer->DAA_digest_R1, stream, stream_size);
    }
    /* load DAA_digest_S0 */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_daa_issuer->DAA_digest_S0, stream, stream_size);
    }
    /* load DAA_digest_S1 */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_daa_issuer->DAA_digest_S1, stream, stream_size);
    }
    /* load DAA_digest_n */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_daa_issuer->DAA_digest_n, stream, stream_size);
    }
    /* load DAA_digest_gamma */
    if (rc == 0) {
	rc = TPM_Digest_Load (tpm_daa_issuer->DAA_digest_gamma, stream, stream_size);
    }
    /* load DAA_generic_q */
    if (rc == 0) {
	rc = TPM_Loadn(tpm_daa_issuer->DAA_generic_q, sizeof(tpm_daa_issuer->DAA_generic_q),
		       stream, stream_size);
    }
    return rc;
}

/* TPM_DAAIssuer_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_DAAIssuer_Store(TPM_STORE_BUFFER *sbuffer,
			       const TPM_DAA_ISSUER *tpm_daa_issuer)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_DAAIssuer_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_DAA_ISSUER);
    }
    /* store DAA_digest_R0 */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_daa_issuer->DAA_digest_R0);
    }
    /* store DAA_digest_R1 */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_daa_issuer->DAA_digest_R1);
    }
    /* store DAA_digest_S0 */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_daa_issuer->DAA_digest_S0);
    }
    /* store DAA_digest_S1 */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_daa_issuer->DAA_digest_S1);
    }
    /* store DAA_digest_n */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_daa_issuer->DAA_digest_n);
    }
    /* store DAA_digest_gamma */
    if (rc == 0) {
	rc = TPM_Digest_Store (sbuffer, tpm_daa_issuer->DAA_digest_gamma);
    }
    /* store DAA_generic_q */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer,
				tpm_daa_issuer->DAA_generic_q,
				sizeof(tpm_daa_issuer->DAA_generic_q));
    }
    return rc;
}

/* TPM_DAAIssuer_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_DAAIssuer_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_DAAIssuer_Delete(TPM_DAA_ISSUER *tpm_daa_issuer)
{
    printf(" TPM_DAAIssuer_Delete:\n");
    if (tpm_daa_issuer != NULL) {
	TPM_DAAIssuer_Init(tpm_daa_issuer);
    }
    return;
}

/* TPM_DAAIssuer_Copy() copies the source to the destination

*/

void TPM_DAAIssuer_Copy(TPM_DAA_ISSUER *dest_daa_issuer,
			TPM_DAA_ISSUER *src_daa_issuer)
{
    printf(" TPM_DAAIssuer_Copy:\n");
    
    TPM_Digest_Copy(dest_daa_issuer->DAA_digest_R0, src_daa_issuer->DAA_digest_R0);
    TPM_Digest_Copy(dest_daa_issuer->DAA_digest_R1, src_daa_issuer->DAA_digest_R1);
    TPM_Digest_Copy(dest_daa_issuer->DAA_digest_S0, src_daa_issuer->DAA_digest_S0);
    TPM_Digest_Copy(dest_daa_issuer->DAA_digest_S1, src_daa_issuer->DAA_digest_S1);
    TPM_Digest_Copy(dest_daa_issuer->DAA_digest_n, src_daa_issuer->DAA_digest_n);
    TPM_Digest_Copy(dest_daa_issuer->DAA_digest_gamma, src_daa_issuer->DAA_digest_gamma);
    memcpy(dest_daa_issuer->DAA_generic_q, src_daa_issuer->DAA_generic_q,
	   sizeof(src_daa_issuer->DAA_generic_q));
    return;
}

/*
  TPM_DAA_TPM
*/

/* TPM_DAATpm_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_DAATpm_Init(TPM_DAA_TPM *tpm_daa_tpm)
{
    printf(" TPM_DAATpm_Init:\n");
    TPM_Digest_Init(tpm_daa_tpm->DAA_digestIssuer);
    TPM_Digest_Init(tpm_daa_tpm->DAA_digest_v0);
    TPM_Digest_Init(tpm_daa_tpm->DAA_digest_v1);
    TPM_Digest_Init(tpm_daa_tpm->DAA_rekey);
    tpm_daa_tpm->DAA_count = 0;
    return;
}

/* TPM_DAATpm_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_DAATpm_Init()
   After use, call TPM_DAATpm_Delete() to free memory
*/

TPM_RESULT TPM_DAATpm_Load(TPM_DAA_TPM *tpm_daa_tpm,
			   unsigned char **stream,
			   uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_DAATpm_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_DAA_TPM, stream, stream_size);
    }
    /* load DAA_digestIssuer */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_daa_tpm->DAA_digestIssuer, stream, stream_size);
    }
    /* load DAA_digest_v0 */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_daa_tpm->DAA_digest_v0, stream, stream_size);
    }
    /* load DAA_digest_v1 */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_daa_tpm->DAA_digest_v1, stream, stream_size);
    }
    /* load DAA_rekey */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_daa_tpm->DAA_rekey, stream, stream_size);
    }
    /* load DAA_count */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_daa_tpm->DAA_count), stream, stream_size);
    }
    return rc;
}

/* TPM_DAATpm_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_DAATpm_Store(TPM_STORE_BUFFER *sbuffer,
			    const TPM_DAA_TPM *tpm_daa_tpm)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_DAATpm_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_DAA_TPM);
    }
    /* store DAA_digestIssuer */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_daa_tpm->DAA_digestIssuer);
    }
    /* store DAA_digest_v0 */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_daa_tpm->DAA_digest_v0);
    }
    /* store DAA_digest_v1 */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_daa_tpm->DAA_digest_v1);
    }
    /* store DAA_rekey */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_daa_tpm->DAA_rekey);
    }
    /* store DAA_count */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_daa_tpm->DAA_count);
    }
    return rc;
}

/* TPM_DAATpm_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_DAATpm_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_DAATpm_Delete(TPM_DAA_TPM *tpm_daa_tpm)
{
    printf(" TPM_DAATpm_Delete:\n");
    if (tpm_daa_tpm != NULL) {
	TPM_DAATpm_Init(tpm_daa_tpm);
    }
    return;
}

/* TPM_DAATpm_Copy() copies the source to the destination

*/

void TPM_DAATpm_Copy(TPM_DAA_TPM *dest_daa_tpm, TPM_DAA_TPM *src_daa_tpm)
{
    printf(" TPM_DAATpm_Copy:\n");
    TPM_Digest_Copy(dest_daa_tpm->DAA_digestIssuer, src_daa_tpm->DAA_digestIssuer);
    TPM_Digest_Copy(dest_daa_tpm->DAA_digest_v0, src_daa_tpm->DAA_digest_v0);
    TPM_Digest_Copy(dest_daa_tpm->DAA_digest_v1, src_daa_tpm->DAA_digest_v1);
    TPM_Digest_Copy(dest_daa_tpm->DAA_rekey, src_daa_tpm->DAA_rekey);
    dest_daa_tpm->DAA_count = src_daa_tpm->DAA_count;
    return;
}

/*
  TPM_DAA_CONTEXT
*/

/* TPM_DAAContext_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_DAAContext_Init(TPM_DAA_CONTEXT *tpm_daa_context)
{
    printf(" TPM_DAAContext_Init:\n");
    TPM_Digest_Init(tpm_daa_context->DAA_digestContext);
    TPM_Digest_Init(tpm_daa_context->DAA_digest);
    TPM_Nonce_Init(tpm_daa_context->DAA_contextSeed);
    memset(tpm_daa_context->DAA_scratch, 0, sizeof(tpm_daa_context->DAA_scratch));
    tpm_daa_context->DAA_stage = 0;
    tpm_daa_context->DAA_scratch_null = TRUE;
    return;
}

/* TPM_DAAContext_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_DAAContext_Init()
   After use, call TPM_DAAContext_Delete() to free memory
*/

TPM_RESULT TPM_DAAContext_Load(TPM_DAA_CONTEXT *tpm_daa_context,
			       unsigned char **stream,
			       uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_DAAContext_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_DAA_CONTEXT, stream, stream_size);
    }
    /* load DAA_digestContext */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_daa_context->DAA_digestContext, stream, stream_size);
    }
    /* load DAA_digest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_daa_context->DAA_digest, stream, stream_size);
    }
    /* load DAA_contextSeed */
    if (rc == 0) {
	rc = TPM_Nonce_Load(tpm_daa_context->DAA_contextSeed, stream, stream_size);
    }
    /* load DAA_scratch */
    if (rc == 0) {
	rc = TPM_Loadn(tpm_daa_context->DAA_scratch, sizeof(tpm_daa_context->DAA_scratch),
		  stream, stream_size);
    }
    /* load DAA_stage  */
    if (rc == 0) {
	rc = TPM_Load8(&(tpm_daa_context->DAA_stage), stream, stream_size);
    }
    /* load DAA_scratch_null  */
    if (rc == 0) {
	rc = TPM_LoadBool(&(tpm_daa_context->DAA_scratch_null), stream, stream_size);
    }
    return rc;
}

/* TPM_DAAContext_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_DAAContext_Store(TPM_STORE_BUFFER *sbuffer,
				const TPM_DAA_CONTEXT *tpm_daa_context)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_DAAContext_Store:\n");
    /* store tag  */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_DAA_CONTEXT);
    }
    /* store DAA_digestContext */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_daa_context->DAA_digestContext);
    }
    /* store DAA_digest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_daa_context->DAA_digest);
    }
    /* store DAA_contextSeed */
    if (rc == 0) {
	rc = TPM_Nonce_Store(sbuffer, tpm_daa_context->DAA_contextSeed);
    }
    /* store DAA_scratch */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, tpm_daa_context->DAA_scratch,
				sizeof(tpm_daa_context->DAA_scratch));
    }
    /* store DAA_stage	*/
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_daa_context->DAA_stage), sizeof(BYTE));
    }
    /* store DAA_scratch_null  */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_daa_context->DAA_scratch_null), sizeof(TPM_BOOL));
    }
    return rc;
}

/* TPM_DAAContext_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_DAAContext_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_DAAContext_Delete(TPM_DAA_CONTEXT *tpm_daa_context)
{
    printf(" TPM_DAAContext_Delete:\n");
    if (tpm_daa_context != NULL) {
	TPM_DAAContext_Init(tpm_daa_context);
    }
    return;
}

/* TPM_DAAContext_Copy() copies the source to the destination

*/

void TPM_DAAContext_Copy(TPM_DAA_CONTEXT *dest_daa_context, TPM_DAA_CONTEXT *src_daa_context)
{
    printf(" TPM_DAAContext_Copy:\n");
    TPM_Digest_Copy(dest_daa_context->DAA_digestContext, src_daa_context->DAA_digestContext);
    TPM_Digest_Copy(dest_daa_context->DAA_digest, src_daa_context->DAA_digest);
    TPM_Nonce_Copy(dest_daa_context->DAA_contextSeed, src_daa_context->DAA_contextSeed);
    memcpy(dest_daa_context->DAA_scratch, src_daa_context->DAA_scratch,
	   sizeof(src_daa_context->DAA_scratch));
    dest_daa_context->DAA_stage = src_daa_context->DAA_stage;
    dest_daa_context->DAA_scratch_null = src_daa_context->DAA_scratch_null;
    return;
}

/*
  TPM_DAA_JOINDATA
*/

/* TPM_DAAJoindata_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_DAAJoindata_Init(TPM_DAA_JOINDATA *tpm_daa_joindata)
{
    printf(" TPM_DAAJoindata_Init:\n");
    memset(tpm_daa_joindata->DAA_join_u0, 0, sizeof(tpm_daa_joindata->DAA_join_u0));
    memset(tpm_daa_joindata->DAA_join_u1, 0, sizeof(tpm_daa_joindata->DAA_join_u1));
    TPM_Digest_Init(tpm_daa_joindata->DAA_digest_n0);
    return;
}

/* TPM_DAAJoindata_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_DAAJoindata_Init()
   After use, call TPM_DAAJoindata_Delete() to free memory
*/

TPM_RESULT TPM_DAAJoindata_Load(TPM_DAA_JOINDATA *tpm_daa_joindata,
			   unsigned char **stream,
			   uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_DAAJoindata_Load:\n");
    /* load DAA_join_u0 */
    if (rc == 0) {
	rc = TPM_Loadn(tpm_daa_joindata->DAA_join_u0,
		       sizeof(tpm_daa_joindata->DAA_join_u0),
		       stream, stream_size);
    }
    /* load DAA_join_u1 */
    if (rc == 0) {
	rc = TPM_Loadn(tpm_daa_joindata->DAA_join_u1,
		       sizeof(tpm_daa_joindata->DAA_join_u1),
		       stream, stream_size);
    }
    /* load DAA_digest_n0 */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_daa_joindata->DAA_digest_n0, stream, stream_size);
    }
    return rc;
}

/* TPM_DAAJoindata_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_DAAJoindata_Store(TPM_STORE_BUFFER *sbuffer,
				 const TPM_DAA_JOINDATA *tpm_daa_joindata)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_DAAJoindata_Store:\n");
    /* store DAA_join_u0 */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, tpm_daa_joindata->DAA_join_u0,
				sizeof(tpm_daa_joindata->DAA_join_u0));
    }
    /* store DAA_join_u1 */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, tpm_daa_joindata->DAA_join_u1,
				sizeof(tpm_daa_joindata->DAA_join_u1));
    }
    /* store DAA_digest_n0 */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_daa_joindata->DAA_digest_n0);
    }
    return rc;
}

/* TPM_DAAJoindata_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_DAAJoindata_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_DAAJoindata_Delete(TPM_DAA_JOINDATA *tpm_daa_joindata)
{
    printf(" TPM_DAAJoindata_Delete:\n");
    if (tpm_daa_joindata != NULL) {
	TPM_DAAJoindata_Init(tpm_daa_joindata);
    }
    return;
}

/* TPM_DAAJoindata_Copy() copies the source to the destination

*/

void TPM_DAAJoindata_Copy(TPM_DAA_JOINDATA *dest_daa_joindata, TPM_DAA_JOINDATA *src_daa_joindata)
{
    printf(" TPM_DAAJoindata_Copy:\n");
    memcpy(dest_daa_joindata->DAA_join_u0, src_daa_joindata->DAA_join_u0,
	   sizeof(src_daa_joindata->DAA_join_u0));
    memcpy(dest_daa_joindata->DAA_join_u1, src_daa_joindata->DAA_join_u1,
	   sizeof(src_daa_joindata->DAA_join_u1));
    TPM_Digest_Copy(dest_daa_joindata->DAA_digest_n0, src_daa_joindata->DAA_digest_n0);
    return;
}

/*
  TPM_DAA_BLOB
*/

/* TPM_DAABlob_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_DAABlob_Init(TPM_DAA_BLOB *tpm_daa_blob)
{
    printf(" TPM_DAABlob_Init:\n");
    tpm_daa_blob->resourceType = 0; 
    memset(tpm_daa_blob->label, 0, sizeof(tpm_daa_blob->label));
    TPM_Digest_Init(tpm_daa_blob->blobIntegrity);
    TPM_SizedBuffer_Init(&(tpm_daa_blob->additionalData));
    TPM_SizedBuffer_Init(&(tpm_daa_blob->sensitiveData));
    return;
}

/* TPM_DAABlob_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_DAABlob_Init()
   After use, call TPM_DAABlob_Delete() to free memory
*/

TPM_RESULT TPM_DAABlob_Load(TPM_DAA_BLOB *tpm_daa_blob,
			    unsigned char **stream,
			    uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_DAABlob_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_DAA_BLOB, stream, stream_size);
    }
    /* load resourceType  */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_daa_blob->resourceType), stream, stream_size);
    }
    /* load label */
    if (rc == 0) {
	rc = TPM_Loadn(tpm_daa_blob->label, sizeof(tpm_daa_blob->label), stream, stream_size);
    }
    /* load blobIntegrity */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_daa_blob->blobIntegrity, stream, stream_size);
    }
    /* load additionalData */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_daa_blob->additionalData), stream, stream_size);
    }
    /* load sensitiveData */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_daa_blob->sensitiveData), stream, stream_size);
    }
    return rc;
}

/* TPM_DAABlob_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_DAABlob_Store(TPM_STORE_BUFFER *sbuffer,
			   const TPM_DAA_BLOB *tpm_daa_blob)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_DAABlob_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_DAA_BLOB);
    }
    /* store resourceType  */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_daa_blob->resourceType);
    }
    /* store label */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, tpm_daa_blob->label, sizeof(tpm_daa_blob->label));
    }
    /* store blobIntegrity */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_daa_blob->blobIntegrity);
    }
    /* store additionalData */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_daa_blob->additionalData));
    }
    /* store sensitiveData */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_daa_blob->sensitiveData));
    }
    return rc;
}

/* TPM_DAABlob_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_DAABlob_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_DAABlob_Delete(TPM_DAA_BLOB *tpm_daa_blob)
{
    printf(" TPM_DAABlob_Delete:\n");
    if (tpm_daa_blob != NULL) {
	TPM_SizedBuffer_Delete(&(tpm_daa_blob->additionalData));
	TPM_SizedBuffer_Delete(&(tpm_daa_blob->sensitiveData));
	TPM_DAABlob_Init(tpm_daa_blob);
    }
    return;
}

/*
  TPM_DAA_SENSITIVE
*/

/* TPM_DAASensitive_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_DAASensitive_Init(TPM_DAA_SENSITIVE *tpm_daa_sensitive)
{
    printf(" TPM_DAASensitive_Init:\n");
    TPM_SizedBuffer_Init(&(tpm_daa_sensitive->internalData));
    return;
}

/* TPM_DAASensitive_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_DAASensitive_Init()
   After use, call TPM_DAASensitive_Delete() to free memory
*/

TPM_RESULT TPM_DAASensitive_Load(TPM_DAA_SENSITIVE *tpm_daa_sensitive,
				 unsigned char **stream,
				 uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_DAASensitive_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_DAA_SENSITIVE, stream, stream_size);
    }
    /* load internalData */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_daa_sensitive->internalData), stream, stream_size);
    }
    return rc;
}

/* TPM_DAASensitive_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_DAASensitive_Store(TPM_STORE_BUFFER *sbuffer,
				  const TPM_DAA_SENSITIVE *tpm_daa_sensitive)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_DAASensitive_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_DAA_SENSITIVE);
    }
    /* store internalData */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_daa_sensitive->internalData));
    }
    return rc;
}

/* TPM_DAASensitive_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_DAASensitive_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_DAASensitive_Delete(TPM_DAA_SENSITIVE *tpm_daa_sensitive)
{
    printf(" TPM_DAASensitive_Delete:\n");
    if (tpm_daa_sensitive != NULL) {
	TPM_SizedBuffer_Delete(&(tpm_daa_sensitive->internalData));
	TPM_DAASensitive_Init(tpm_daa_sensitive);
    }
    return;
}


/*
  Processing Common Stage Functions
*/

TPM_RESULT TPM_DAAJoin_Stage00(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA **tpm_daa_session_data,
			       TPM_BOOL *daaHandleValid,
			       TPM_SIZED_BUFFER *outputData,
			       TPM_SIZED_BUFFER *inputData0)
{
    TPM_RESULT		rc = 0;
    unsigned char	*stream;
    uint32_t		stream_size;
    uint32_t		count;
    TPM_HANDLE		daaHandle = 0;		/* no preassigned handle */
    
    printf("TPM_DAAJoin_Stage00:\n");
    if (rc == 0) {
	/* a. Determine that sufficient resources are available to perform a TPM_DAA_Join. */
	/* i. The TPM MUST support sufficient resources to perform one (1)
	   TPM_DAA_Join/TPM_DAA_Sign. The TPM MAY support additional TPM_DAA_Join/TPM_DAA_Sign
	   sessions. */
	/* ii. The TPM may share internal resources between the DAA operations and other variable
	   resource requirements: */
	/* iii. If there are insufficient resources within the stored key pool (and one or more keys
	   need to be removed to permit the DAA operation to execute) return TPM_NOSPACE */
	/* iv. If there are insufficient resources within the stored session pool (and one or more
	   authorization or transport sessions need to be removed to permit the DAA operation to
	   execute), return TPM_RESOURCES. */
	rc = TPM_DaaSessions_GetNewHandle(tpm_daa_session_data,
					  &daaHandle,		/* output */
					  daaHandleValid,	/* output */
					  tpm_state->tpm_stclear_data.daaSessions); /* array */
    }
    if (rc == 0) {
	/* b. Set all fields in DAA_issuerSettings = NULL */
	/* c. set all fields in DAA_tpmSpecific = NULL */
	/* d. set all fields in DAA_session = NULL */
	/* e. Set all fields in DAA_joinSession = NULL */
	/* NOTE Done by TPM_DaaSessions_GetNewHandle() */
	/* f. Verify that sizeOf(inputData0) == sizeof(DAA_tpmSpecific -> DAA_count) and return
	   error TPM_DAA_INPUT_DATA0 on mismatch */
	if (inputData0->size != sizeof((*tpm_daa_session_data)->DAA_tpmSpecific.DAA_count)) {
	    printf("TPM_DAAJoin_Stage00: Error, inputData0 size %u should be %lu\n",
		   inputData0->size,
		   (unsigned long)sizeof((*tpm_daa_session_data)->DAA_tpmSpecific.DAA_count));
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    if (rc == 0) {
	/* g. Verify that inputData0 > 0, and return error TPM_DAA_INPUT_DATA0 on mismatch */
	stream = inputData0->buffer;
	stream_size = inputData0->size;
	rc = TPM_Load32(&count, &stream, &stream_size);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage00: count %u\n", count);
	if (count == 0) {
	    printf("TPM_DAAJoin_Stage00: Error, count is zero\n");
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    if (rc == 0) {
	/* h. Set DAA_tpmSpecific -> DAA_count = inputData0 */
	(*tpm_daa_session_data)->DAA_tpmSpecific.DAA_count = count;
	/* i. set DAA_session -> DAA_digestContext = SHA-1(DAA_tpmSpecific || DAA_joinSession) */
	rc = TPM_DAADigestContext_GenerateDigestJoin
	     ((*tpm_daa_session_data)->DAA_session.DAA_digestContext,
	      (*tpm_daa_session_data));
    }
    if (rc == 0) {
	/* j. set DAA_session -> DAA_stage = 1 */
	(*tpm_daa_session_data)->DAA_session.DAA_stage = 1;
	/* k. Assign session handle for TPM_DAA_Join */
	/* NOTE Done by TPM_DaaSessions_GetNewHandle() */
	printf("TPM_DAAJoin_Stage00: handle %08x\n", (*tpm_daa_session_data)->daaHandle);
	/* l. set outputData = new session handle */
	/* i. The handle in outputData is included the output HMAC. */
	rc = TPM_SizedBuffer_Append32(outputData, (*tpm_daa_session_data)->daaHandle);
    }
    /* m. return TPM_SUCCESS */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage01(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData,
			       TPM_SIZED_BUFFER *inputData0,
			       TPM_SIZED_BUFFER *inputData1)
{
    TPM_RESULT		rc = 0;
    TPM_DIGEST		signedDataDigest;

    printf("TPM_DAAJoin_Stage01:\n");
    outputData = outputData;			/* not used */
    /* a. Verify that DAA_session ->DAA_stage==1. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that sizeOf(inputData0) == DAA_SIZE_issuerModulus and return error
       TPM_DAA_INPUT_DATA0 on mismatch */
    if (rc == 0) {
	if (inputData0->size != DAA_SIZE_issuerModulus) {
	    printf("TPM_DAAJoin_Stage01: Error, bad input0 size %u\n", inputData0->size);
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    if (rc == 0) {
	/* d. If DAA_session -> DAA_scratch == NULL: */
	if (tpm_daa_session_data->DAA_session.DAA_scratch_null) {
	    printf("TPM_DAAJoin_Stage01: DAA_scratch null\n");
	    if (rc == 0) {
		/* i. Set DAA_session -> DAA_scratch = inputData0 */
		tpm_daa_session_data->DAA_session.DAA_scratch_null = FALSE;
		memcpy(tpm_daa_session_data->DAA_session.DAA_scratch,
		       inputData0->buffer, DAA_SIZE_issuerModulus);
		/* ii. set DAA_joinSession -> DAA_digest_n0 = SHA-1(DAA_session -> DAA_scratch) */
		rc =
		    TPM_SHA1(tpm_daa_session_data->DAA_joinSession.DAA_digest_n0,
			     sizeof(tpm_daa_session_data->DAA_session.DAA_scratch),
			     tpm_daa_session_data->DAA_session.DAA_scratch,
			     0, NULL);
	    }
	    /* iii. set DAA_tpmSpecific -> DAA_rekey = SHA-1(tpmDAASeed || DAA_joinSession ->
	       DAA_digest_n0)  */
	    if (rc == 0) {
		rc = TPM_SHA1(tpm_daa_session_data->DAA_tpmSpecific.DAA_rekey,
			      TPM_NONCE_SIZE, tpm_state->tpm_permanent_data.tpmDAASeed, 
			      TPM_DIGEST_SIZE, tpm_daa_session_data->DAA_joinSession.DAA_digest_n0, 
			      0, NULL);
	    }
	}
	/* e. Else (If DAA_session -> DAA_scratch != NULL): */
	else {
	    printf("TPM_DAAJoin_Stage01: DAA_scratch not null\n");
	    /* i. Set signedData = inputData0 */
	    /* ii. Verify that sizeOf(inputData1) == DAA_SIZE_issuerModulus and return error
	       TPM_DAA_INPUT_DATA1 on mismatch */
	    if (rc == 0) {
		if (inputData1->size != DAA_SIZE_issuerModulus) {
		    printf("TPM_DAAJoin_Stage01: Error, bad input1 size %u\n", inputData1->size);
		    rc = TPM_DAA_INPUT_DATA1;
		}
	    }
	    /* iii. Set signatureValue = inputData1 */
	    /* iv. Use the RSA key == [DAA_session -> DAA_scratch] to verify that signatureValue is
	       a signature on signedData using TPM_SS_RSASSAPKCS1v15_SHA1 (RSA PKCS1.5 with SHA-1),
	       and return error TPM_DAA_ISSUER_VALIDITY on mismatch */
	    if (rc == 0) {
		printf("TPM_DAAJoin_Stage01: Digesting signedData\n");
		rc = TPM_SHA1(signedDataDigest,
			      inputData0->size, inputData0->buffer,
			      0, NULL);
	    }
	    if (rc == 0) {
		printf("TPM_DAAJoin_Stage01: Verifying signature\n");
		rc = TPM_RSAVerify(inputData1->buffer,			/* signature */
				   inputData1->size,
				   TPM_SS_RSASSAPKCS1v15_INFO,		/* signature scheme */
				   signedDataDigest,			/* signed data */
				   TPM_DIGEST_SIZE,
				   tpm_daa_session_data->DAA_session.DAA_scratch, /* pub modulus */
				   sizeof(tpm_daa_session_data->DAA_session.DAA_scratch),
				   tpm_default_rsa_exponent,		/* public exponent */
				   3);
		if (rc != 0) {
		    printf("TPM_DAAJoin_Stage01: Error, bad signature\n");
		    rc = TPM_DAA_ISSUER_VALIDITY;
		}
	    }
	    /* v. Set DAA_session -> DAA_scratch = signedData */
	    if (rc == 0) {
		memcpy(tpm_daa_session_data->DAA_session.DAA_scratch,
		       inputData0->buffer, inputData1->size);
	    }
	}
    }
    if (rc == 0) {
	/* f. Decrement DAA_tpmSpecific -> DAA_count by 1 (unity) */
	tpm_daa_session_data->DAA_tpmSpecific.DAA_count--;
	/* g. If DAA_tpmSpecific -> DAA_count ==0: */
	if (tpm_daa_session_data->DAA_tpmSpecific.DAA_count == 0) {
	    /* h. increment DAA_session -> DAA_Stage by 1 */
	    tpm_daa_session_data->DAA_session.DAA_stage++;
	}
	/* i. set DAA_session -> DAA_digestContext = SHA-1(DAA_tpmSpecific || DAA_joinSession) */
	rc = TPM_DAADigestContext_GenerateDigestJoin
	     (tpm_daa_session_data->DAA_session.DAA_digestContext, tpm_daa_session_data);
    }
    /* j. set outputData = NULL */
    /* NOTE Done by caller */
    /* k. return TPM_SUCCESS */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage02(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData,
			       TPM_SIZED_BUFFER *inputData0,
			       TPM_SIZED_BUFFER *inputData1)
{
    TPM_RESULT		rc = 0;
    unsigned char	*stream;
    uint32_t		stream_size;
    TPM_STORE_BUFFER	signedDataSbuffer;
    TPM_DIGEST		signedDataDigest;

    printf("TPM_DAAJoin_Stage02:\n");
    outputData = outputData;			/* not used */
    tpm_state = tpm_state;			/* not used */
    TPM_Sbuffer_Init(&signedDataSbuffer);	/* freed @1*/
    /* a. Verify that DAA_session ->DAA_stage==2. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that sizeOf(inputData0) == sizeOf(TPM_DAA_ISSUER) and return error
       TPM_DAA_INPUT_DATA0 on mismatch */
    /* NOTE cannot use sizeof because packing may not be exact */
    /* d. Set DAA_issuerSettings = inputData0. Verify that all fields in DAA_issuerSettings are
       present and return error TPM_DAA_INPUT_DATA0 if not. */
    if (rc == 0) {
	stream = inputData0->buffer;
	stream_size = inputData0->size;
	rc = TPM_DAAIssuer_Load(&(tpm_daa_session_data->DAA_issuerSettings), &stream, &stream_size);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    if (rc == 0) {
	if (stream_size != 0) {
	    printf("TPM_DAAJoin_Stage02: Error, bad input0 size %u\n", inputData0->size);
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* e. Verify that sizeOf(inputData1) == DAA_SIZE_issuerModulus and return error
       TPM_DAA_INPUT_DATA1 on mismatch */
    if (rc == 0) {
	if (inputData1->size != DAA_SIZE_issuerModulus) {
	    printf("TPM_DAAJoin_Stage02: Error, bad input1 size %u\n", inputData1->size);
	    rc = TPM_DAA_INPUT_DATA1;
	}
    }
    /* f. Set signatureValue = inputData1 */
    /* g. Set signedData = (DAA_joinSession -> DAA_digest_n0 || DAA_issuerSettings) */
    if (rc == 0) {
	rc = TPM_Digest_Store(&signedDataSbuffer,
			      tpm_daa_session_data->DAA_joinSession.DAA_digest_n0);
    }
    if (rc == 0) {
	rc = TPM_DAAIssuer_Store(&signedDataSbuffer, &(tpm_daa_session_data->DAA_issuerSettings));
    }
    /* h. Use the RSA key [DAA_session -> DAA_scratch] to verify that signatureValue is a  */
    /* signature on signedData using TPM_SS_RSASSAPKCS1v15_SHA1 (RSA PKCS1.5 with SHA-1), and return
       error TPM_DAA_ISSUER_VALIDITY on mismatch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage02: Digesting signedData\n");
	rc = TPM_SHA1Sbuffer(signedDataDigest, &signedDataSbuffer);
    }
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage02: Verifying signature\n");
	rc = TPM_RSAVerify(inputData1->buffer,			/* signature */
			   inputData1->size,
			   TPM_SS_RSASSAPKCS1v15_INFO,		/* signature scheme */
			   signedDataDigest,			/* signed data */
			   TPM_DIGEST_SIZE,
			   tpm_daa_session_data->DAA_session.DAA_scratch,	/* public modulus */
			   sizeof(tpm_daa_session_data->DAA_session.DAA_scratch),
			   tpm_default_rsa_exponent,		/* public exponent */
			   3);
	if (rc != 0) {
	    printf("TPM_DAAJoin_Stage02: Error, bad signature\n");
	    rc = TPM_DAA_ISSUER_VALIDITY;
	}
    }
    /* i. Set DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings)	*/
    if (rc == 0) {
	rc = TPM_SHA1_GenerateStructure(tpm_daa_session_data->DAA_tpmSpecific.DAA_digestIssuer,
					&(tpm_daa_session_data->DAA_issuerSettings),
					(TPM_STORE_FUNCTION_T)TPM_DAAIssuer_Store);
    }
    /* j. set DAA_session -> DAA_digestContext = SHA-1(DAA_tpmSpecific || DAA_joinSession) */
    if (rc == 0) {
	rc = TPM_DAADigestContext_GenerateDigestJoin
	     (tpm_daa_session_data->DAA_session.DAA_digestContext, tpm_daa_session_data);
    }
    if (rc == 0) {
	/* k. Set DAA_session -> DAA_scratch = NULL */
	tpm_daa_session_data->DAA_session.DAA_scratch_null = TRUE;
	/* l. increment DAA_session -> DAA_stage by 1 */
	/* NOTE Done by common code */
    }
    /* m. return TPM_SUCCESS */
    TPM_Sbuffer_Delete(&signedDataSbuffer);	/* @1*/
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage03(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData,
			       TPM_SIZED_BUFFER *inputData0)
{
    TPM_RESULT		rc = 0;
    unsigned char	*stream;
    uint32_t		stream_size;

    printf("TPM_DAAJoin_Stage03:\n");
    tpm_state = tpm_state;			/* not used */
    outputData = outputData;			/* not used */
    /* a. Verify that DAA_session ->DAA_stage==3. Return TPM_DAA_STAGE and flush handle on
       mismatch	 */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Verify that sizeOf(inputData0) == sizeOf(DAA_tpmSpecific -> DAA_count) and return error
       TPM_DAA_INPUT_DATA0 on mismatch */
    if (rc == 0) {
	if (inputData0->size != sizeof(tpm_daa_session_data->DAA_tpmSpecific.DAA_count)) {
	    printf("TPM_DAAJoin_Stage03: Error, inputData0 size %u should be %lu\n",
		   inputData0->size,
		   (unsigned long)sizeof(tpm_daa_session_data->DAA_tpmSpecific.DAA_count));
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
      /* e. Set DAA_tpmSpecific -> DAA_count = inputData0 */
    if (rc == 0) {
	stream = inputData0->buffer;
	stream_size = inputData0->size;
	rc = TPM_Load32(&(tpm_daa_session_data->DAA_tpmSpecific.DAA_count), &stream, &stream_size);
    }
    /* f. Obtain random data from the RNG and store it as DAA_joinSession -> DAA_join_u0 */
    if (rc == 0) {
	rc = TPM_Random(tpm_daa_session_data->DAA_joinSession.DAA_join_u0,
			sizeof(tpm_daa_session_data->DAA_joinSession.DAA_join_u0));
    }
    /* g. Obtain random data from the RNG and store it as DAA_joinSession -> DAA_join_u1 */
    if (rc == 0) {
	rc = TPM_Random(tpm_daa_session_data->DAA_joinSession.DAA_join_u1,
			sizeof(tpm_daa_session_data->DAA_joinSession.DAA_join_u1));
    }
    /* h. set outputData = NULL */
    /* NOTE Done by caller */
    /* i. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* j. set DAA_session -> DAA_digestContext = SHA-1(DAA_tpmSpecific || DAA_joinSession)  */
    if (rc == 0) {
	rc = TPM_DAADigestContext_GenerateDigestJoin
	     (tpm_daa_session_data->DAA_session.DAA_digestContext, tpm_daa_session_data);
    }
    /* k. return TPM_SUCCESS */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage04(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData,
			       TPM_SIZED_BUFFER *inputData0,
			       TPM_SIZED_BUFFER *inputData1)
{
    TPM_RESULT		rc = 0;
    TPM_BIGNUM		xBignum = NULL;	/* freed @1 */
    TPM_BIGNUM		nBignum = NULL;	/* freed @2 */
    TPM_BIGNUM		fBignum = NULL;	/* freed @3 */
    TPM_BIGNUM		rBignum = NULL;	/* freed @4 */
		
    printf("TPM_DAAJoin_Stage04:\n");
    tpm_state = tpm_state;			/* not used */
    outputData = outputData;			/* not used */
    /* a. Verify that DAA_session ->DAA_stage==4. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Set DAA_generic_R0 = inputData0 */
    /* e. Verify that SHA-1(DAA_generic_R0) == DAA_issuerSettings -> DAA_digest_R0 and return error
       TPM_DAA_INPUT_DATA0 on mismatch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage04: Checking DAA_generic_R0\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_R0,	/* expect */
			    inputData0->size, inputData0->buffer,	/* DAA_generic_R0 */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* f. Set DAA_generic_n = inputData1 */
    /* g. Verify that SHA-1(DAA_generic_n) == DAA_issuerSettings -> DAA_digest_n and return error
       TPM_DAA_INPUT_DATA1 on mismatch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage04: Checking DAA_digest_n\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_n,	/* expect */
			    inputData1->size, inputData1->buffer,	/* DAA_generic_n */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA1;
	}
    }
    /* h. Set X = DAA_generic_R0 */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage04: Creating X\n");
	rc = TPM_bin2bn(&xBignum, inputData0->buffer, inputData0->size);
    }
    /* i. Set n = DAA_generic_n */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage04: Creating n\n");
	rc = TPM_bin2bn(&nBignum, inputData1->buffer, inputData1->size);
    }
    /* j. Set f = SHA-1(DAA_tpmSpecific -> DAA_rekey || DAA_tpmSpecific -> DAA_count || 0) ||
       SHA-1(DAA_tpmSpecific -> DAA_rekey || DAA_tpmSpecific -> DAA_count || 1 ) mod
       DAA_issuerSettings -> DAA_generic_q */
    if (rc == 0) {	
	rc = TPM_ComputeF(&fBignum, tpm_daa_session_data);		/* freed @3 */
    }
    /* k. Set f0 = f mod 2^DAA_power0 (erase all but the lowest DAA_power0 bits of f) */
    if (rc == 0) {
	rc = TPM_BN_mask_bits(fBignum, DAA_power0);	/* f becomes f0 */
    }
    /* l. Set DAA_session -> DAA_scratch = (X^f0) mod n */
    if (rc == 0) {
	rc = TPM_ComputeAexpPmodn(tpm_daa_session_data->DAA_session.DAA_scratch,
				  sizeof(tpm_daa_session_data->DAA_session.DAA_scratch),
				  &rBignum,	/* R */
				  xBignum,	/* A */
				  fBignum,	/* P */
				  nBignum);	/* n */
    }
    /* m. set outputData = NULL */
    /* NOTE Done by caller */
    /* n. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* o. return TPM_SUCCESS */
    TPM_BN_free(xBignum);	/* @1 */
    TPM_BN_free(nBignum);	/* @2 */
    TPM_BN_free(fBignum);	/* @3 */
    TPM_BN_free(rBignum);	/* @4 */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage05(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData,
			       TPM_SIZED_BUFFER *inputData0,
			       TPM_SIZED_BUFFER *inputData1)
{
    TPM_RESULT		rc = 0;
    TPM_BIGNUM		xBignum = NULL;		/* freed @1 */
    TPM_BIGNUM		nBignum = NULL;		/* freed @2 */
    TPM_BIGNUM		fBignum = NULL;		/* freed @3 */
    TPM_BIGNUM		f1Bignum = NULL;	/* freed @4 */
    TPM_BIGNUM		zBignum = NULL;		/* freed @5 */

    printf("TPM_DAAJoin_Stage05:\n");
    tpm_state = tpm_state;			/* not used */
    outputData = outputData;			/* not used */
    /* a. Verify that DAA_session ->DAA_stage==5. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Set DAA_generic_R1 = inputData0 */
    /* e. Verify that SHA-1(DAA_generic_R1) == DAA_issuerSettings -> DAA_digest_R1 and return error
       TPM_DAA_INPUT_DATA0 on mismatch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage05: Checking DAA_generic_R1\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_R1,	/* expect */
			    inputData0->size, inputData0->buffer,	/* DAA_generic_R1 */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* f. Set DAA_generic_n = inputData1 */
    /* g. Verify that SHA-1(DAA_generic_n) == DAA_issuerSettings -> DAA_digest_n and return error
       TPM_DAA_INPUT_DATA1 on mismatch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage05: Checking DAA_digest_n\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_n,	/* expect */
			    inputData1->size, inputData1->buffer,	/* DAA_generic_n */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA1;
	}
    }
    /* h. Set X = DAA_generic_R1 */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage05: Creating X\n");
	rc = TPM_bin2bn(&xBignum, inputData0->buffer, inputData0->size);
    }
    /* i. Set n = DAA_generic_n */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage05: Creating n\n");
	rc = TPM_bin2bn(&nBignum, inputData1->buffer, inputData1->size);
    }
    /* j. Set f = SHA-1(DAA_tpmSpecific -> DAA_rekey || DAA_tpmSpecific -> DAA_count || 0) ||
       SHA-1(DAA_tpmSpecific -> DAA_rekey || DAA_tpmSpecific -> DAA_count || 1 ) mod
       DAA_issuerSettings -> DAA_generic_q. */
    if (rc == 0) {	
	rc = TPM_ComputeF(&fBignum, tpm_daa_session_data);	/* freed @3 */
    }
    /* k. Shift f right by DAA_power0 bits (discard the lowest DAA_power0 bits) and label the result
       f1 */
    if (rc == 0) {
	rc = TPM_BN_rshift(&f1Bignum, fBignum, DAA_power0);	/* f becomes f1 */
    }
    /* l. Set Z = DAA_session -> DAA_scratch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage05: Creating Z\n");
	rc = TPM_bin2bn(&zBignum,
			tpm_daa_session_data->DAA_session.DAA_scratch,
			sizeof(tpm_daa_session_data->DAA_session.DAA_scratch));
    }
    /* m. Set DAA_session -> DAA_scratch = Z*(X^f1) mod n */
    if (rc == 0) {
	rc = TPM_ComputeZxAexpPmodn(tpm_daa_session_data->DAA_session.DAA_scratch,
				    sizeof(tpm_daa_session_data->DAA_session.DAA_scratch),
				    zBignum,	/* Z */
				    xBignum,	/* A */
				    f1Bignum,	/* P */
				    nBignum);	/* N */
    }
    /* n. set outputData = NULL */
    /* NOTE Done by caller */
    /* o. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* p. return TPM_SUCCESS */
    TPM_BN_free(xBignum);	/* @1 */
    TPM_BN_free(nBignum);	/* @2 */
    TPM_BN_free(fBignum);	/* @3 */
    TPM_BN_free(f1Bignum);	/* @4 */
    TPM_BN_free(zBignum);	/* @5 */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage06(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData,
			       TPM_SIZED_BUFFER *inputData0,
			       TPM_SIZED_BUFFER *inputData1)
{
    TPM_RESULT		rc = 0;
    TPM_BIGNUM		xBignum = NULL;	/* freed @1 */
    TPM_BIGNUM		nBignum = NULL;	/* freed @2 */
    TPM_BIGNUM		zBignum = NULL;	/* freed @3 */
    TPM_BIGNUM		yBignum = NULL;	/* freed @4 */

    printf("TPM_DAAJoin_Stage06:\n");
    tpm_state = tpm_state;			/* not used */
    outputData = outputData;			/* not used */
    /* a. Verify that DAA_session ->DAA_stage==6. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Set DAA_generic_S0 = inputData0 */
    /* e. Verify that SHA-1(DAA_generic_S0) == DAA_issuerSettings -> DAA_digest_S0 and return error
       TPM_DAA_INPUT_DATA0 on mismatch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage06: Checking DAA_generic_S0\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_S0,	/* expect */
			    inputData0->size, inputData0->buffer,	/* DAA_generic_S0 */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* f. Set DAA_generic_n = inputData1 */
    /* g. Verify that SHA-1(DAA_generic_n) == DAA_issuerSettings -> DAA_digest_n and return error
       TPM_DAA_INPUT_DATA1 on mismatch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage06: Checking DAA_digest_n\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_n,	/* expect */
			    inputData1->size, inputData1->buffer,	/* DAA_generic_n */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA1;
	}
    }
    /* h. Set X = DAA_generic_S0 */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage06: Creating X\n");
	rc = TPM_bin2bn(&xBignum, inputData0->buffer, inputData0->size);
    }
    /* i. Set n = DAA_generic_n */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage06: Creating n\n");
	rc = TPM_bin2bn(&nBignum, inputData1->buffer, inputData1->size);
    }
    /* j. Set Z = DAA_session -> DAA_scratch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage06: Creating Z\n");
	rc = TPM_bin2bn(&zBignum,
			tpm_daa_session_data->DAA_session.DAA_scratch,
			sizeof(tpm_daa_session_data->DAA_session.DAA_scratch));
    }
    /* k. Set Y = DAA_joinSession -> DAA_join_u0 */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage06: Creating Y\n");
	rc = TPM_bin2bn(&yBignum,
			tpm_daa_session_data->DAA_joinSession.DAA_join_u0,
			sizeof(tpm_daa_session_data->DAA_joinSession.DAA_join_u0));
    }
    /* l. Set DAA_session -> DAA_scratch = Z*(X^Y) mod n */
    if (rc == 0) {
	rc = TPM_ComputeZxAexpPmodn(tpm_daa_session_data->DAA_session.DAA_scratch,
				    sizeof(tpm_daa_session_data->DAA_session.DAA_scratch),
				    zBignum,	/* Z */
				    xBignum,	/* A */
				    yBignum,	/* P */
				    nBignum);	/* N */
    }
    /* m. set outputData = NULL */
    /* NOTE Done by caller */
    /* n. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* o. return TPM_SUCCESS */
    TPM_BN_free(xBignum);	/* @1 */
    TPM_BN_free(nBignum);	/* @2 */
    TPM_BN_free(zBignum);	/* @3 */
    TPM_BN_free(yBignum);	/* @4 */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage07(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData,
			       TPM_SIZED_BUFFER *inputData0,
			       TPM_SIZED_BUFFER *inputData1)
{
    TPM_RESULT		rc = 0;
    uint32_t		nCount;		/* DAA_count in nbo */
    TPM_BIGNUM		xBignum = NULL;	/* freed @1 */
    TPM_BIGNUM		nBignum = NULL;	/* freed @2 */
    TPM_BIGNUM		yBignum = NULL;	/* freed @3 */
    TPM_BIGNUM		zBignum = NULL;	/* freed @4 */

    printf("TPM_DAAJoin_Stage07:\n");
    tpm_state = tpm_state;			/* not used */
    /* a. Verify that DAA_session ->DAA_stage==7. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Set DAA_generic_S1 = inputData0 */
    /* e. Verify that SHA-1(DAA_generic_S1) == DAA_issuerSettings -> DAA_digest_S1 and return error
       TPM_DAA_INPUT_DATA0 on mismatch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage07: Checking DAA_generic_S1\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_S1,	/* expect */
			    inputData0->size, inputData0->buffer,	/* DAA_generic_S1 */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* f. Set DAA_generic_n = inputData1 */
    /* g. Verify that SHA-1(DAA_generic_n) == DAA_issuerSettings -> DAA_digest_n and return error
       TPM_DAA_INPUT_DATA1 on mismatch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage07: Checking DAA_digest_n\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_n,	/* expect */
			    inputData1->size, inputData1->buffer,	/* DAA_generic_n */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA1;
	}
    }
    /* h. Set X = DAA_generic_S1 */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage07: Creating X\n");
	rc = TPM_bin2bn(&xBignum, inputData0->buffer, inputData0->size);
    }
    /* i. Set n = DAA_generic_n */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage07: Creating n\n");
	rc = TPM_bin2bn(&nBignum, inputData1->buffer, inputData1->size);
    }
    /* j. Set Y = DAA_joinSession -> DAA_join_u1 */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage07: Creating Y\n");
	rc = TPM_bin2bn(&yBignum,
			tpm_daa_session_data->DAA_joinSession.DAA_join_u1,
			sizeof(tpm_daa_session_data->DAA_joinSession.DAA_join_u1));
    }
    /* k. Set Z = DAA_session -> DAA_scratch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage07: Creating Z\n");
	rc = TPM_bin2bn(&zBignum,
			tpm_daa_session_data->DAA_session.DAA_scratch,
			sizeof(tpm_daa_session_data->DAA_session.DAA_scratch));
    }
    /* l. Set DAA_session -> DAA_scratch = Z*(X^Y) mod n */
    if (rc == 0) {
	rc = TPM_ComputeZxAexpPmodn(tpm_daa_session_data->DAA_session.DAA_scratch,
				    sizeof(tpm_daa_session_data->DAA_session.DAA_scratch),
				    zBignum,	/* Z */
				    xBignum,	/* A */
				    yBignum,	/* P */
				    nBignum);	/* N */
    }
    /* m. Set DAA_session -> DAA_digest to the SHA-1 (DAA_session -> DAA_scratch || DAA_tpmSpecific
       -> DAA_count || DAA_joinSession -> DAA_digest_n0) */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage07: Computing DAA_digest\n");
	nCount = htonl(tpm_daa_session_data->DAA_tpmSpecific.DAA_count);
	rc = TPM_SHA1(tpm_daa_session_data->DAA_session.DAA_digest, 
		      sizeof(tpm_daa_session_data->DAA_session.DAA_scratch),
		      tpm_daa_session_data->DAA_session.DAA_scratch,
		      sizeof(uint32_t), &nCount,
		      TPM_DIGEST_SIZE, tpm_daa_session_data->DAA_joinSession.DAA_digest_n0,
		      0, NULL);
    }
    /* n. set outputData = DAA_session -> DAA_scratch */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Set(outputData,
				 sizeof(tpm_daa_session_data->DAA_session.DAA_scratch),
				 tpm_daa_session_data->DAA_session.DAA_scratch);
    }
    /* o. set DAA_session -> DAA_scratch = NULL */
    if (rc == 0) {
	tpm_daa_session_data->DAA_session.DAA_scratch_null = TRUE;
    }
    /* p. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* q. return TPM_SUCCESS */
    TPM_BN_free(xBignum);	/* @1 */
    TPM_BN_free(nBignum);	/* @2 */
    TPM_BN_free(yBignum);	/* @3 */
    TPM_BN_free(zBignum);	/* @4 */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage08(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData,
			       TPM_SIZED_BUFFER *inputData0)
{
    TPM_RESULT		rc = 0;
    unsigned char	*NE = NULL;			/* freed @1 */
    uint32_t		NELength;
    TPM_DIGEST		outDigest;
    
    printf("TPM_DAAJoin_Stage08:\n");
    /* a. Verify that DAA_session ->DAA_stage==8. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Verify inputSize0 == DAA_SIZE_NE and return error TPM_DAA_INPUT_DATA0 on mismatch */
    if (rc == 0) {
	if (inputData0->size != DAA_SIZE_NE) {
	    printf("TPM_DAAJoin_Stage08: Error, inputData0 size %u should be %u\n",
		   inputData0->size, DAA_SIZE_NE);
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* e. Set NE = decrypt(inputData0, privEK)	*/
    if (rc == 0) {
	rc = TPM_RSAPrivateDecryptMalloc(&NE,		/* decrypted data */
					 &NELength,	/* length of data put into decrypt_data */
					 inputData0->buffer,	/* encrypted data */
					 inputData0->size,	/* encrypted data size */
					 &(tpm_state->tpm_permanent_data.endorsementKey));
    }
    /* f. set outputData = SHA-1(DAA_session -> DAA_digest || NE)  */
    if (rc == 0) {
	rc = TPM_SHA1(outDigest,
		      TPM_DIGEST_SIZE, tpm_daa_session_data->DAA_session.DAA_digest, 
		      NELength, NE,
		      0, NULL);
    } 
    if (rc == 0) {
	rc = TPM_SizedBuffer_Set(outputData, TPM_DIGEST_SIZE, outDigest);
    }
    /* g. set DAA_session -> DAA_digest = NULL */
    if (rc == 0) {
	TPM_Digest_Init(tpm_daa_session_data->DAA_session.DAA_digest);
    }
    /* h. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* i. return TPM_SUCCESS */
    free(NE);			/* @1 */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage09_Sign_Stage2(tpm_state_t *tpm_state,
					   TPM_DAA_SESSION_DATA *tpm_daa_session_data,
					   TPM_SIZED_BUFFER *outputData,
					   TPM_SIZED_BUFFER *inputData0,
					   TPM_SIZED_BUFFER *inputData1)
{
    TPM_RESULT		rc = 0;
    unsigned char	*Y = NULL;	/* freed @1 */
    TPM_BIGNUM		yBignum = NULL;	/* freed @2 */
    TPM_BIGNUM		xBignum = NULL;	/* freed @3 */
    TPM_BIGNUM		nBignum = NULL;	/* freed @4 */
    TPM_BIGNUM		rBignum = NULL;	/* freed @5 */

    printf("TPM_DAAJoin_Stage09_Sign_Stage2:\n");
    tpm_state = tpm_state;			/* not used */
    outputData = outputData;			/* not used */
    /* a. Verify that DAA_session ->DAA_stage==9. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific ||DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Set DAA_generic_R0 = inputData0 */
    /* e. Verify that SHA-1(DAA_generic_R0) == DAA_issuerSettings -> DAA_digest_R0 and return error
       TPM_DAA_INPUT_DATA0 on mismatch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage09_Sign_Stage2: Checking DAA_generic_R0\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_R0,	/* expect */
			    inputData0->size, inputData0->buffer,	/* DAA_generic_R0 */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* f. Set DAA_generic_n = inputData1 */
    /* g. Verify that SHA-1(DAA_generic_n) == DAA_issuerSettings -> DAA_digest_n and return error
       TPM_DAA_INPUT_DATA1 on mismatch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage09_Sign_Stage2: Checking DAA_digest_n\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_n,	/* expect */
			    inputData1->size, inputData1->buffer,	/* DAA_generic_n */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA1;
	}
    }
    /* h. Obtain random data from the RNG and store it as DAA_session -> DAA_contextSeed */
    if (rc == 0) {
	rc = TPM_Nonce_Generate(tpm_daa_session_data->DAA_session.DAA_contextSeed);
    }
    /* i. Obtain DAA_SIZE_r0 bytes using the MGF1 function and label them Y.  "r0" || DAA_session ->
       DAA_contextSeed is the Z seed. */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage09_Sign_Stage2: Creating Y\n");
	rc = TPM_MGF1_GenerateArray(&Y,			/* returned MGF1 array */
				    DAA_SIZE_r0,		/* size of Y */
				    /* length of the entire seed */
				    sizeof("r0") -1 +
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    sizeof("r0") -1, "r0",
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    tpm_daa_session_data->DAA_session.DAA_contextSeed,
				    0, NULL);
    }
    if (rc == 0) {
	rc = TPM_bin2bn(&yBignum, Y, DAA_SIZE_r0);
    }
    /* j. Set X = DAA_generic_R0 */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage09_Sign_Stage2: Creating X\n");
	rc = TPM_bin2bn(&xBignum, inputData0->buffer, inputData0->size);
    }
    /* k. Set n = DAA_generic_n */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage09_Sign_Stage2: Creating n\n");
	rc = TPM_bin2bn(&nBignum, inputData1->buffer, inputData1->size);
    }
    /* l. Set DAA_session -> DAA_scratch = (X^Y) mod n */
    if (rc == 0) {
	rc = TPM_ComputeAexpPmodn(tpm_daa_session_data->DAA_session.DAA_scratch,
				  sizeof(tpm_daa_session_data->DAA_session.DAA_scratch),
				  &rBignum,	/* R */
				  xBignum,	/* A */
				  yBignum,	/* P */
				  nBignum);	/* n */

    }
    /* m. set outputData = NULL */
    /* NOTE Done by caller */
    /* n. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* o. return TPM_SUCCESS */
    free(Y);			/* @1 */
    TPM_BN_free(yBignum);	/* @2 */
    TPM_BN_free(xBignum);	/* @3 */
    TPM_BN_free(nBignum);	/* @4 */
    TPM_BN_free(rBignum);	/* @5 */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage10_Sign_Stage3(tpm_state_t *tpm_state,
					   TPM_DAA_SESSION_DATA *tpm_daa_session_data,
					   TPM_SIZED_BUFFER *outputData,
					   TPM_SIZED_BUFFER *inputData0,
					   TPM_SIZED_BUFFER *inputData1)
{
    TPM_RESULT		rc = 0;
    unsigned char	*Y= NULL;	/* freed @1 */
    TPM_BIGNUM		xBignum = NULL;	/* freed @2 */
    TPM_BIGNUM		nBignum = NULL;	/* freed @3 */
    TPM_BIGNUM		zBignum = NULL;	/* freed @4 */
    TPM_BIGNUM		yBignum = NULL;	/* freed @5*/

    printf("TPM_DAAJoin_Stage10_Sign_Stage3:\n");
    tpm_state = tpm_state;			/* not used */
    outputData = outputData;			/* not used */
    /* a. Verify that DAA_session ->DAA_stage==10. Return TPM_DAA_STAGE and flush handle on mismatch
       h */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Set DAA_generic_R1 = inputData0 */
    /* e. Verify that SHA-1(DAA_generic_R1) == DAA_issuerSettings -> DAA_digest_R1 and return error
       TPM_DAA_INPUT_DATA0 on mismatch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage10_Sign_Stage3: Checking DAA_generic_R1\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_R1,	/* expect */
			    inputData0->size, inputData0->buffer,	/* DAA_generic_R1 */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* f. Set DAA_generic_n = inputData1 */
    /* g. Verify that SHA-1(DAA_generic_n) == DAA_issuerSettings -> DAA_digest_n and return error
       TPM_DAA_INPUT_DATA1 on mismatch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage10_Sign_Stage3: Checking DAA_digest_n\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_n,	/* expect */
			    inputData1->size, inputData1->buffer,	/* DAA_generic_n */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA1;
	}
    }
    /* h. Obtain DAA_SIZE_r1 bytes using the MGF1 function and label them Y.  "r1" || DAA_session ->
       DAA_contextSeed is the Z seed. */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage10_Sign_Stage3: Creating Y\n");
	rc = TPM_MGF1_GenerateArray(&Y,			/* returned MGF1 array */
				    DAA_SIZE_r1,		/* size of Y */
				    /* length of the entire seed */
				    sizeof("r1") -1 +
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    sizeof("r1") -1, "r1",
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    tpm_daa_session_data->DAA_session.DAA_contextSeed,
				    0, NULL);
    }
    if (rc == 0) {
	rc = TPM_bin2bn(&yBignum, Y, DAA_SIZE_r1);
    }
    /* i. Set X = DAA_generic_R1 */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage10_Sign_Stage3: Creating X\n");
	rc = TPM_bin2bn(&xBignum, inputData0->buffer, inputData0->size);
    }
    /* j. Set n = DAA_generic_n */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage10_Sign_Stage3: Creating n\n");
	rc = TPM_bin2bn(&nBignum, inputData1->buffer, inputData1->size);
    }
    /* k. Set Z = DAA_session -> DAA_scratch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage10_Sign_Stage3: Creating Z\n");
	rc = TPM_bin2bn(&zBignum,
			tpm_daa_session_data->DAA_session.DAA_scratch,
			sizeof(tpm_daa_session_data->DAA_session.DAA_scratch));
    }
    /* l. Set DAA_session -> DAA_scratch = Z*(X^Y) mod n */
    if (rc == 0) {
	rc = TPM_ComputeZxAexpPmodn(tpm_daa_session_data->DAA_session.DAA_scratch,
				    sizeof(tpm_daa_session_data->DAA_session.DAA_scratch),
				    zBignum,	/* Z */
				    xBignum,	/* A */
				    yBignum,	/* P */
				    nBignum);	/* N */
    }
    /* m. set outputData = NULL */
    /* NOTE Done by caller */
    /* n. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* o. return TPM_SUCCESS */
    free(Y);			/* @1 */
    TPM_BN_free(xBignum);	/* @2 */
    TPM_BN_free(nBignum);	/* @3 */
    TPM_BN_free(zBignum);	/* @4 */
    TPM_BN_free(yBignum);	/* @5 */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage11_Sign_Stage4(tpm_state_t *tpm_state,
					   TPM_DAA_SESSION_DATA *tpm_daa_session_data,
					   TPM_SIZED_BUFFER *outputData,
					   TPM_SIZED_BUFFER *inputData0,
					   TPM_SIZED_BUFFER *inputData1)
{
    TPM_RESULT		rc = 0;
    unsigned char	*Y= NULL;	/* freed @1 */
    TPM_BIGNUM		yBignum = NULL;	/* freed @2 */
    TPM_BIGNUM		xBignum = NULL;	/* freed @3 */
    TPM_BIGNUM		nBignum = NULL;	/* freed @4 */
    TPM_BIGNUM		zBignum = NULL;	/* freed @5 */

    printf("TPM_DAAJoin_Stage11_Sign_Stage4:\n");
    tpm_state = tpm_state;			/* not used */
    outputData = outputData;			/* not used */
    /* a. Verify that DAA_session ->DAA_stage==11. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Set DAA_generic_S0 = inputData0 */
    /* e. Verify that SHA-1(DAA_generic_S0) == DAA_issuerSettings -> DAA_digest_S0 and return error
       TPM_DAA_INPUT_DATA0 on mismatch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage11_Sign_Stage4: Checking DAA_generic_S0\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_S0,	/* expect */
			    inputData0->size, inputData0->buffer,	/* DAA_generic_S0 */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* f. Set DAA_generic_n = inputData1 */
    /* g. Verify that SHA-1(DAA_generic_n) == DAA_issuerSettings -> DAA_digest_n and return error
       TPM_DAA_INPUT_DATA1 on mismatch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage11_Sign_Stage4: Checking DAA_digest_n\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_n,	/* expect */
			    inputData1->size, inputData1->buffer,	/* DAA_generic_n */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA1;
	}
    }
    /* h. Obtain DAA_SIZE_r2 bytes using the MGF1 function and label them Y.  "r2" || DAA_session ->
       DAA_contextSeed is the Z seed. */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage11_Sign_Stage4: Creating Y\n");
	rc = TPM_MGF1_GenerateArray(&Y,			/* returned MGF1 array */
				    DAA_SIZE_r2,		/* size of Y */
				    /* length of the entire seed */
				    sizeof("r2") -1 +
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    sizeof("r2") -1, "r2",
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    tpm_daa_session_data->DAA_session.DAA_contextSeed,
				    0, NULL);
    }
    if (rc == 0) {
	rc = TPM_bin2bn(&yBignum, Y, DAA_SIZE_r2);
    }
    /* i. Set X = DAA_generic_S0 */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage11_Sign_Stage4: Creating X\n");
	rc = TPM_bin2bn(&xBignum, inputData0->buffer, inputData0->size);
    }
    /* j. Set n = DAA_generic_n */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage11_Sign_Stage4: Creating n\n");
	rc = TPM_bin2bn(&nBignum, inputData1->buffer, inputData1->size);
    }
    /* k. Set Z = DAA_session -> DAA_scratch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage11_Sign_Stage4: Creating Z\n");
	rc = TPM_bin2bn(&zBignum,
			tpm_daa_session_data->DAA_session.DAA_scratch,
			sizeof(tpm_daa_session_data->DAA_session.DAA_scratch));
    }
    /* l. Set DAA_session -> DAA_scratch = Z*(X^Y) mod n */
    if (rc == 0) {
	rc = TPM_ComputeZxAexpPmodn(tpm_daa_session_data->DAA_session.DAA_scratch,
				    sizeof(tpm_daa_session_data->DAA_session.DAA_scratch),
				    zBignum,	/* Z */
				    xBignum,	/* A */
				    yBignum,	/* P */
				    nBignum);	/* N */
    }
    /* m. set outputData = NULL */
    /* NOTE Done by caller */
    /* n. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* o. return TPM_SUCCESS */
    free(Y);			/* @1 */
    TPM_BN_free(yBignum);	/* @2 */
    TPM_BN_free(xBignum);	/* @3 */
    TPM_BN_free(nBignum);	/* @4 */
    TPM_BN_free(zBignum);	/* @5 */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage12(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData,
			       TPM_SIZED_BUFFER *inputData0,
			       TPM_SIZED_BUFFER *inputData1)
{
    TPM_RESULT		rc = 0;
    unsigned char	*Y = NULL;	/* freed @1 */
    TPM_BIGNUM		yBignum = NULL;	/* freed @2 */
    TPM_BIGNUM		xBignum = NULL;	/* freed @3 */
    TPM_BIGNUM		nBignum = NULL;	/* freed @4 */
    TPM_BIGNUM		zBignum = NULL;	/* freed @5 */

    printf("TPM_DAAJoin_Stage12:\n");
    tpm_state = tpm_state;			/* not used */
    /* a. Verify that DAA_session ->DAA_stage==12. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings ) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Set DAA_generic_S1 = inputData0 */
    /* e. Verify that SHA-1(DAA_generic_S1) == DAA_issuerSettings -> DAA_digest_S1 and return error
       TPM_DAA_INPUT_DATA0 on mismatch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage12: Checking DAA_generic_S1\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_S1,	/* expect */
			    inputData0->size, inputData0->buffer,	/* DAA_generic_S1 */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* f. Set DAA_generic_n = inputData1 */
    /* g. Verify that SHA-1(DAA_generic_n) == DAA_issuerSettings -> DAA_digest_n and return error
       TPM_DAA_INPUT_DATA1 on mismatch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage12: Checking DAA_digest_n\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_n,	/* expect */
			    inputData1->size, inputData1->buffer,	/* DAA_generic_n */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA1;
	}
    }
    /* h. Obtain DAA_SIZE_r3 bytes using the MGF1 function and label them Y.  "r3" || DAA_session ->
       DAA_contextSeed) is the Z seed. */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage12: Creating Y\n");
	rc = TPM_MGF1_GenerateArray(&Y,			/* returned MGF1 array */
				    DAA_SIZE_r3,		/* size of Y */
				    /* length of the entire seed */
				    sizeof("r3") -1 +
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    sizeof("r3") -1, "r3",
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    tpm_daa_session_data->DAA_session.DAA_contextSeed,
				    0, NULL);
    }
    if (rc == 0) {
	rc = TPM_bin2bn(&yBignum, Y, DAA_SIZE_r3);
    }
    /* i. Set X = DAA_generic_S1 */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage12: Creating X\n");
	rc = TPM_bin2bn(&xBignum, inputData0->buffer, inputData0->size);
    }
    /* j. Set n = DAA_generic_n */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage12: Creating n\n");
	rc = TPM_bin2bn(&nBignum, inputData1->buffer, inputData1->size);
    }
    /* k. Set Z = DAA_session -> DAA_scratch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage12: Creating Z\n");
	rc = TPM_bin2bn(&zBignum,
			tpm_daa_session_data->DAA_session.DAA_scratch,
			sizeof(tpm_daa_session_data->DAA_session.DAA_scratch));
    }
    /* l. Set DAA_session -> DAA_scratch = Z*(X^Y) mod n */
    if (rc == 0) {
	rc = TPM_ComputeZxAexpPmodn(tpm_daa_session_data->DAA_session.DAA_scratch,
				    sizeof(tpm_daa_session_data->DAA_session.DAA_scratch),
				    zBignum,	/* Z */
				    xBignum,	/* A */
				    yBignum,	/* P */
				    nBignum);	/* N */
    }
    /* m. set outputData = DAA_session -> DAA_scratch */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Set(outputData,
				 sizeof(tpm_daa_session_data->DAA_session.DAA_scratch),
				 tpm_daa_session_data->DAA_session.DAA_scratch);
    }
    /* n. Set DAA_session -> DAA_scratch = NULL */
    if (rc == 0) {
	tpm_daa_session_data->DAA_session.DAA_scratch_null = TRUE;
    }
    /* o. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* p. return TPM_SUCCESS */
    free(Y);			/* @1 */
    TPM_BN_free(yBignum);	/* @2 */
    TPM_BN_free(xBignum);	/* @3 */
    TPM_BN_free(nBignum);	/* @4 */
    TPM_BN_free(zBignum);	/* @5 */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage13_Sign_Stage6(tpm_state_t *tpm_state,
					   TPM_DAA_SESSION_DATA *tpm_daa_session_data,
					   TPM_SIZED_BUFFER *outputData,
					   TPM_SIZED_BUFFER *inputData0,
					   TPM_SIZED_BUFFER *inputData1)
{
    TPM_RESULT		rc = 0;
    TPM_BIGNUM		wBignum = NULL;		/* freed @1 */
    TPM_BIGNUM		qBignum = NULL;		/* freed @2 */
    TPM_BIGNUM		nBignum = NULL;		/* freed @3 */
    TPM_BIGNUM		w1Bignum = NULL;	/* freed @4 */

    printf("TPM_DAAJoin_Stage13_Sign_Stage6:\n");
    tpm_state = tpm_state;			/* not used */
    outputData = outputData;			/* not used */
    /* a. Verify that DAA_session->DAA_stage==13. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Set DAA_generic_gamma = inputData0 */
    /* e. Verify that SHA-1(DAA_generic_gamma) == DAA_issuerSettings -> DAA_digest_gamma and return
       error TPM_DAA_INPUT_DATA0 on mismatch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage13_Sign_Stage6: Checking DAA_generic_gamma\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_gamma,	/* expect */
			    inputData0->size, inputData0->buffer,	/* DAA_generic_gamma */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* f. Verify that inputSize1 == DAA_SIZE_w and return error TPM_DAA_INPUT_DATA1 on mismatch */
    if (rc == 0) {
	if (inputData1->size !=	 DAA_SIZE_w) {
	    printf("TPM_DAAJoin_Stage13_Sign_Stage6: Error, inputData1 size %u should be %u\n",
		   inputData0->size, DAA_SIZE_w);
	    rc = TPM_DAA_INPUT_DATA1;
	}
    }
    /* g. Set w = inputData1 */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage13_Sign_Stage6: Creating w\n");
	rc = TPM_bin2bn(&wBignum, inputData1->buffer, inputData1->size);
    }
    /* FIXME added Set q = DAA_issuerSettings -> DAA_generic_q */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage13_Sign_Stage6: Creating q from DAA_generic_q\n");
	rc = TPM_bin2bn(&qBignum,
			tpm_daa_session_data->DAA_issuerSettings.DAA_generic_q,
			sizeof(tpm_daa_session_data->DAA_issuerSettings.DAA_generic_q));
    }
    /* FIXME Set n = DAA_generic_gamma */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage13_Sign_Stage6: Creating n\n");
	rc = TPM_bin2bn(&nBignum, inputData0->buffer, inputData0->size);
    }
    /* h. Set w1 = w^( DAA_issuerSettings -> DAA_generic_q) mod (DAA_generic_gamma) */
    /* FIXME w1 = (w^q) mod n */
    if (rc == 0) {
	rc = TPM_ComputeAexpPmodn(NULL,
				  0,
				  &w1Bignum,	/* R */
				  wBignum,	/* A */
				  qBignum,	/* P */
				  nBignum);	/* n */
    }
    /* i. If w1 != 1 (unity), return error TPM_DAA_WRONG_W */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage13_Sign_Stage6: Testing w1\n");
	rc = TPM_BN_is_one(w1Bignum);
    }
    /* j. Set DAA_session -> DAA_scratch = w */
    if (rc == 0) {
	rc = TPM_ComputeDAAScratch(tpm_daa_session_data->DAA_session.DAA_scratch,
				   sizeof(tpm_daa_session_data->DAA_session.DAA_scratch),
				   wBignum);
    }
    /* k. set outputData = NULL */
    /* NOTE Done by caller */
    /* l. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* m. return TPM_SUCCESS. */
    TPM_BN_free(wBignum);	/* @1 */
    TPM_BN_free(qBignum);	/* @2 */
    TPM_BN_free(nBignum);	/* @3 */
    TPM_BN_free(w1Bignum);	/* @4 */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage14_Sign_Stage7(tpm_state_t *tpm_state,
					   TPM_DAA_SESSION_DATA *tpm_daa_session_data,
					   TPM_SIZED_BUFFER *outputData,
					   TPM_SIZED_BUFFER *inputData0)
{
    TPM_RESULT		rc = 0;
    TPM_BIGNUM		fBignum = NULL;	/* freed @1 */
    TPM_BIGNUM		wBignum = NULL;	/* freed @2 */
    TPM_BIGNUM		nBignum = NULL;	/* freed @3 */
    TPM_BIGNUM		eBignum = NULL;	/* freed @4 */

    unsigned int	numBytes;	/* for debug */

    printf("TPM_DAAJoin_Stage14_Sign_Stage7:\n");
    tpm_state = tpm_state;			/* not used */
    /* a. Verify that DAA_session ->DAA_stage==14. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings ) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Set DAA_generic_gamma = inputData0 */
    /* e. Verify that SHA-1(DAA_generic_gamma) == DAA_issuerSettings -> DAA_digest_gamma and return
       error TPM_DAA_INPUT_DATA0 on mismatch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage14_Sign_Stage7: Checking DAA_generic_gamma\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_gamma,	/* expect */
			    inputData0->size, inputData0->buffer,	/* DAA_generic_gamma */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* f. Set f = SHA-1(DAA_tpmSpecific -> DAA_rekey || DAA_tpmSpecific -> DAA_count || 0) ||
       SHA-1(DAA_tpmSpecific -> DAA_rekey || DAA_tpmSpecific -> DAA_count || 1 ) mod
       DAA_issuerSettings -> DAA_generic_q. */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage14_Sign_Stage7: Creating f\n");
	rc = TPM_ComputeF(&fBignum, tpm_daa_session_data);	/* freed @1 */
    }
    if (rc == 0) {
	rc = TPM_BN_num_bytes(&numBytes, fBignum);
	printf("TPM_DAAJoin_Stage14_Sign_Stage7: f. f size %u\n", numBytes);
    }
    /* FIXME Set W = DAA_session -> DAA_scratch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage14_Sign_Stage7: Creating W\n");
	rc = TPM_bin2bn(&wBignum,
			tpm_daa_session_data->DAA_session.DAA_scratch,
			sizeof(tpm_daa_session_data->DAA_session.DAA_scratch));
    }
    if (rc == 0) {
	rc = TPM_BN_num_bytes(&numBytes, wBignum);
	printf("TPM_DAAJoin_Stage14_Sign_Stage7: W size %u\n", numBytes);
    }
    /* FIXME Set n = DAA_generic_gamma */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage14_Sign_Stage7: Creating n\n");
	rc = TPM_bin2bn(&nBignum, inputData0->buffer, inputData0->size);
    }
    if (rc == 0) {
	rc = TPM_BN_num_bytes(&numBytes, nBignum);
	printf("TPM_DAAJoin_Stage14_Sign_Stage7: n size %u\n", numBytes);
    }
    /* g. Set E = ((DAA_session -> DAA_scratch)^f) mod (DAA_generic_gamma). */
    /* FIXME E = (w^f) mod n */
    if (rc == 0) {
	rc = TPM_ComputeAexpPmodn(NULL,
				  0,
				  &eBignum,	/* R */
				  wBignum,	/* A */
				  fBignum,	/* P */
				  nBignum);	/* n */
    }
    /* h. Set outputData = E */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage14_Sign_Stage7: Output E\n");
	rc = TPM_bn2binMalloc(&(outputData->buffer),
			      &(outputData->size),
			      eBignum, 0);
    }
    /* i. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* j. return TPM_SUCCESS. */
    TPM_BN_free(fBignum);	/* @1 */
    TPM_BN_free(wBignum);	/* @2 */
    TPM_BN_free(nBignum);	/* @3 */
    TPM_BN_free(eBignum);	/* @4 */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage15_Sign_Stage8(tpm_state_t *tpm_state,
					   TPM_DAA_SESSION_DATA *tpm_daa_session_data,
					   TPM_SIZED_BUFFER *outputData,
					   TPM_SIZED_BUFFER *inputData0)
{
    TPM_RESULT		rc = 0;
    unsigned char	*r0 = NULL;		/* freed @1 */
    unsigned char	*r1 = NULL;		/* freed @2 */
    TPM_BIGNUM		r0Bignum = NULL;	/* freed @3 */
    TPM_BIGNUM		r1Bignum = NULL;	/* freed @4 */
    TPM_BIGNUM		r1sBignum = NULL;	/* freed @5 */
    TPM_BIGNUM		rBignum = NULL;		/* freed @6 */
    TPM_BIGNUM		e1Bignum = NULL;	/* freed @7 */
    TPM_BIGNUM		qBignum = NULL;		/* freed @8 */
    TPM_BIGNUM		nBignum = NULL;		/* freed @9 */
    TPM_BIGNUM		wBignum = NULL;		/* freed @10 */

    printf("TPM_DAAJoin_Stage15_Sign_Stage8:\n");
    tpm_state = tpm_state;			/* not used */
    /* a. Verify that DAA_session ->DAA_stage==15. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Set DAA_generic_gamma = inputData0 */
    /* e. Verify that SHA-1(DAA_generic_gamma) == DAA_issuerSettings -> DAA_digest_gamma and return
       error TPM_DAA_INPUT_DATA0 on mismatch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage15_Sign_Stage8: Checking DAA_generic_gamma\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_gamma,	/* expect */
			    inputData0->size, inputData0->buffer,	/* DAA_generic_gamma */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* f. Obtain DAA_SIZE_r0 bytes using the MGF1 function and label them r0.  "r0" || DAA_session
       -> DAA_contextSeed) is the Z seed. */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage15_Sign_Stage8: Creating r0\n");
	rc = TPM_MGF1_GenerateArray(&r0,			/* returned MGF1 array */
				    DAA_SIZE_r0,		/* size of Y */
				    /* length of the entire seed */
				    sizeof("r0") -1 +
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    sizeof("r0") -1, "r0",
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    tpm_daa_session_data->DAA_session.DAA_contextSeed,
				    0, NULL);
    }
    if (rc == 0) {
	rc = TPM_bin2bn(&r0Bignum, r0, DAA_SIZE_r0);
    }
    /* g. Obtain DAA_SIZE_r1 bytes using the MGF1 function and label them r1.  "r1" || DAA_session
       -> DAA_contextSeedis the Z seed. */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage15_Sign_Stage8: Creating r1\n");
	rc = TPM_MGF1_GenerateArray(&r1,			/* returned MGF1 array */
				    DAA_SIZE_r1,		/* size of Y */
				    /* length of the entire seed */
				    sizeof("r1") -1 +
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    sizeof("r1") -1, "r1",
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    tpm_daa_session_data->DAA_session.DAA_contextSeed,
				    0, NULL);
    }
    if (rc == 0) {
	rc = TPM_bin2bn(&r1Bignum, r1, DAA_SIZE_r1);
    }
    /* FIXME Set q = DAA_generic_q */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage15_Sign_Stage8: Creating n from DAA_generic_q\n");
	rc = TPM_bin2bn(&qBignum,
			tpm_daa_session_data->DAA_issuerSettings.DAA_generic_q,
			sizeof(tpm_daa_session_data->DAA_issuerSettings.DAA_generic_q));
    }
    /* h. set r = r0 + 2^DAA_power0 * r1 mod (DAA_issuerSettings -> DAA_generic_q). */
    /* FIXME added parentheses
       h. set r = (r0 + (2^DAA_power0 * r1)) mod (DAA_issuerSettings -> DAA_generic_q).
       h. set r = (r0 + (2^DAA_power0 * r1)) mod q */
    if (rc == 0) {
	rc = TPM_BN_lshift(&r1sBignum,	/* result, freed @5 */
			   r1Bignum,	/* input */
			   DAA_power0); /* n */
    }
    if (rc == 0) {
	rc = TPM_ComputeApBmodn(&rBignum,	/* result, freed @6 */
				r0Bignum,	/* A */
				r1sBignum,	/* B */
				qBignum);	/* n */
    }
    /* FIXME Set n = DAA_generic_gamma */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage15_Sign_Stage8: Creating n1 from DAA_generic_gamma\n");
	rc = TPM_bin2bn(&nBignum, inputData0->buffer, inputData0->size);
    }
    /* FIXME Set w = DAA_session -> DAA_scratch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage15_Sign_Stage8: Creating w from DAA_scratch\n");
	rc = TPM_bin2bn(&wBignum,
			tpm_daa_session_data->DAA_session.DAA_scratch,
			sizeof(tpm_daa_session_data->DAA_session.DAA_scratch));
    }
    /* i. set E1 = ((DAA_session -> DAA_scratch)^r) mod (DAA_generic_gamma). */
    /* (w ^ r) mod n */
    if (rc == 0) {
	rc = TPM_ComputeAexpPmodn(NULL,
				  0,
				  &e1Bignum,	/* R */
				  wBignum,	/* A */
				  rBignum,	/* P */
				  nBignum);	/* n */
    }
    /* j. Set DAA_session -> DAA_scratch = NULL */
    if (rc == 0) {
	tpm_daa_session_data->DAA_session.DAA_scratch_null = TRUE;
    }
    /* k. Set outputData = E1 */
    if (rc == 0) {
	rc = TPM_bn2binMalloc(&(outputData->buffer),
			      &(outputData->size),
			      e1Bignum, 0);
    }
    /* l. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* m. return TPM_SUCCESS. */
    free(r0);			/* @1 */
    free(r1);			/* @2 */
    TPM_BN_free(r0Bignum);	/* @3 */
    TPM_BN_free(r1Bignum);	/* @4 */
    TPM_BN_free(r1sBignum);	/* @5 */
    TPM_BN_free(rBignum);	/* @6 */
    TPM_BN_free(e1Bignum);	/* @7 */
    TPM_BN_free(qBignum);	/* @8 */
    TPM_BN_free(nBignum);	/* @9 */
    TPM_BN_free(wBignum);	/* @10 */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage16_Sign_Stage9(tpm_state_t *tpm_state,
					   TPM_DAA_SESSION_DATA *tpm_daa_session_data,
					   TPM_SIZED_BUFFER *outputData,
					   TPM_SIZED_BUFFER *inputData0)
{
    TPM_RESULT		rc = 0;
    unsigned char	*nt = NULL;		/* freed @1 */

    printf("TPM_DAAJoin_Stage16_Sign_Stage9:\n");
    tpm_state = tpm_state;			/* not used */
    /* a. Verify that DAA_session ->DAA_stage==16. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Verify that inputSize0 == sizeOf(TPM_DIGEST) and return error TPM_DAA_INPUT_DATA0 on
       mismatch */
    if (rc == 0) {
	if (inputData0->size != TPM_DIGEST_SIZE) {
	    printf("TPM_DAAJoin_Stage16_Sign_Stage9: Error, inputData0 size %u should be %u\n",
		   inputData0->size, TPM_DIGEST_SIZE);
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* e. Set DAA_session -> DAA_digest = inputData0 */
    if (rc == 0) {
	/* e. Set DAA_session -> DAA_digest = inputData0 */
	/* NOTE: This step is unnecessary, since the value is overridden in g. */
	/* f. Obtain DAA_SIZE_NT bytes from the RNG and label them NT */
	rc = TPM_Malloc(&nt, DAA_SIZE_NT);
    }
    if (rc == 0) {
	rc = TPM_Random(nt, DAA_SIZE_NT);
    }
    /* g. Set DAA_session -> DAA_digest to the SHA-1 ( DAA_session -> DAA_digest || NT ) */
    if (rc == 0) {
	rc = TPM_SHA1(tpm_daa_session_data->DAA_session.DAA_digest,
		      inputData0->size, inputData0->buffer,	/* e. DAA_session -> DAA_digest */
		      DAA_SIZE_NT, nt,
		      0, NULL);
    }
    /* h. Set outputData = NT  */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Set(outputData, DAA_SIZE_NT, nt);
    }
    /* i. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* j. return TPM_SUCCESS. */
    free(nt);		/* @1 */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage17_Sign_Stage11(tpm_state_t *tpm_state,
					    TPM_DAA_SESSION_DATA *tpm_daa_session_data,
					    TPM_SIZED_BUFFER *outputData)
{
    TPM_RESULT		rc = 0;
    unsigned char	*r0 = NULL;		/* freed @1 */
    TPM_BIGNUM		r0Bignum = NULL;	/* freed @2 */
    TPM_BIGNUM		fBignum = NULL;		/* freed @3 */
    TPM_BIGNUM		s0Bignum = NULL;	/* freed @4 */
    TPM_BIGNUM		cBignum = NULL;		/* freed @5 */

    printf("TPM_DAAJoin_Stage17_Sign_Stage11:\n");
    tpm_state = tpm_state;			/* not used */
    /* a. Verify that DAA_session ->DAA_stage==17. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Obtain DAA_SIZE_r0 bytes using the MGF1 function and label them r0.  "r0" || DAA_session
       -> DAA_contextSeed is the Z seed. */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage17_Sign_Stage11: Creating r0\n");
	rc = TPM_MGF1_GenerateArray(&r0,			/* returned MGF1 array */
				    DAA_SIZE_r0,		/* size of Y */
				    /* length of the entire seed */
				    sizeof("r0") -1 +
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    sizeof("r0") -1, "r0",
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    tpm_daa_session_data->DAA_session.DAA_contextSeed,
				    0, NULL);
    }
    if (rc == 0) {
	rc = TPM_bin2bn(&r0Bignum, r0, DAA_SIZE_r0);
    }
    /* e. Set f = SHA-1(DAA_tpmSpecific -> DAA_rekey || DAA_tpmSpecific -> DAA_count || 0) ||
       SHA-1(DAA_tpmSpecific -> DAA_rekey || DAA_tpmSpecific -> DAA_count || 1 ) mod
       DAA_issuerSettings -> DAA_generic_q. */
    if (rc == 0) {
	rc = TPM_ComputeF(&fBignum, tpm_daa_session_data);	/* freed @3 */
    }
    /* f. Set f0 = f mod 2^DAA_power0 (erase all but the lowest DAA_power0 bits of f) */
    if (rc == 0) {
	rc = TPM_BN_mask_bits(fBignum, DAA_power0);	/* f becomes f0 */
    }
    /* FIXME Set c = DAA_session -> DAA_digest */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage17_Sign_Stage11: Creating c from DAA_session -> DAA_digest\n");
	rc = TPM_bin2bn(&cBignum, tpm_daa_session_data->DAA_session.DAA_digest, TPM_DIGEST_SIZE);
    }
    /* g. Set s0 = r0 + (DAA_session -> DAA_digest) * f0 in Z. Compute over the integers.  The
       computation is not reduced with a modulus. */
    /* s0 = r0 + (c * f0) */
    if (rc == 0) {
	rc =  TPM_ComputeApBxC(&s0Bignum,	/* result */
			       r0Bignum,	/* A */
			       cBignum,		/* B */
			       fBignum);	/* C */
    }
    /* h. set outputData = s0 */
    if (rc == 0) {
	rc = TPM_bn2binMalloc(&(outputData->buffer),
			      &(outputData->size),
			      s0Bignum, 0);
    }
    /* i. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* j. return TPM_SUCCESS */
    free(r0);			/* @1 */
    TPM_BN_free(r0Bignum);	/* @2 */
    TPM_BN_free(fBignum);	/* @3 */
    TPM_BN_free(s0Bignum);	/* @4 */
    TPM_BN_free(cBignum);	/* @5 */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage18_Sign_Stage12(tpm_state_t *tpm_state,
					    TPM_DAA_SESSION_DATA *tpm_daa_session_data,
					    TPM_SIZED_BUFFER *outputData)
{
    TPM_RESULT		rc = 0;
    unsigned char	*r1 = NULL;		/* freed @1 */
    TPM_BIGNUM		r1Bignum = NULL;	/* freed @2 */
    TPM_BIGNUM		fBignum = NULL;		/* freed @3 */
    TPM_BIGNUM		f1Bignum = NULL;	/* freed @4 */
    TPM_BIGNUM		s1Bignum = NULL;	/* freed @5 */
    TPM_BIGNUM		cBignum = NULL;		/* freed @6 */

    printf("TPM_DAAJoin_Stage18_Sign_Stage12:\n");
    tpm_state = tpm_state;			/* not used */
    /* a. Verify that DAA_session ->DAA_stage==18. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Obtain DAA_SIZE_r1 bytes using the MGF1 function and label them r1.  "r1" || DAA_session
       -> DAA_contextSeed is the Z seed. */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage18_Sign_Stage12: Creating r1\n");
	rc = TPM_MGF1_GenerateArray(&r1,			/* returned MGF1 array */
				    DAA_SIZE_r1,		/* size of Y */
				    /* length of the entire seed */
				    sizeof("r1") -1 +
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    sizeof("r1") -1, "r1",
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    tpm_daa_session_data->DAA_session.DAA_contextSeed,
				    0, NULL);
    }
    if (rc == 0) {
	rc = TPM_bin2bn(&r1Bignum, r1, DAA_SIZE_r1);
    }
    /* e. Set f = SHA-1(DAA_tpmSpecific -> DAA_rekey || DAA_tpmSpecific -> DAA_count || 0) ||
       SHA-1(DAA_tpmSpecific -> DAA_rekey || DAA_tpmSpecific -> DAA_count || 1 ) mod
       DAA_issuerSettings -> DAA_generic_q. */
    if (rc == 0) {
	rc = TPM_ComputeF(&fBignum, tpm_daa_session_data);	/* freed @3 */
    }
    /* f. Shift f right by DAA_power0 bits (discard the lowest DAA_power0 bits) and label the result
       f1 */
    if (rc == 0) {
	rc = TPM_BN_rshift(&f1Bignum, fBignum, DAA_power0);	/* f becomes f1 */
    }
    /* FIXME Set c = DAA_session -> DAA_digest */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage18_Sign_Stage12: Creating c from DAA_session -> DAA_digest\n");
	rc = TPM_bin2bn(&cBignum, tpm_daa_session_data->DAA_session.DAA_digest, TPM_DIGEST_SIZE);
    }
    /* g. Set s1 = r1 + (DAA_session -> DAA_digest)* f1 in Z. Compute over the integers.  The
       computation is not reduced with a modulus. */
    /* s1 = r1 + (c * f1) */
    if (rc == 0) {
	rc =  TPM_ComputeApBxC(&s1Bignum,	/* result */
			       r1Bignum,	/* A */
			       cBignum,		/* B */
			       f1Bignum);	/* C */
    }
    /* h. set outputData = s1 */
    if (rc == 0) {
	rc = TPM_bn2binMalloc(&(outputData->buffer),
			      &(outputData->size),
			      s1Bignum, 0);
    }
    /* i. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* j. return TPM_SUCCESS */
    free(r1);			/* @1 */
    TPM_BN_free(r1Bignum);	/* @2 */
    TPM_BN_free(fBignum);	/* @3 */
    TPM_BN_free(f1Bignum);	/* @4 */
    TPM_BN_free(s1Bignum);	/* @5 */
    TPM_BN_free(cBignum);	/* @6 */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage19(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData)
{
    TPM_RESULT		rc = 0;
    unsigned char	*r2 = NULL;		/* freed @1 */
    TPM_BIGNUM		r2Bignum = NULL;	/* freed @2 */
    TPM_BIGNUM		s2Bignum = NULL;	/* freed @3 */
    TPM_BIGNUM		cBignum = NULL;		/* freed @4 */
    TPM_BIGNUM		u0Bignum = NULL;	/* freed @5 */

    printf("TPM_DAAJoin_Stage19:\n");
    tpm_state = tpm_state;			/* not used */
    /* a. Verify that DAA_session ->DAA_stage==19. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Obtain DAA_SIZE_r2 bytes using the MGF1 function and label them r2. "r2" || DAA_session ->
       DAA_contextSeed is the Z seed. */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage19: Creating r2\n");
	rc = TPM_MGF1_GenerateArray(&r2,			/* returned MGF1 array */
				    DAA_SIZE_r2,		/* size of Y */
				    /* length of the entire seed */
				    sizeof("r2") -1 +
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    sizeof("r2") -1, "r2",
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    tpm_daa_session_data->DAA_session.DAA_contextSeed,
				    0, NULL);
    }
    if (rc == 0) {
	rc = TPM_bin2bn(&r2Bignum, r2, DAA_SIZE_r2);
    }
    /* e. Set s2 = r2 + (DAA_session -> DAA_digest)*( DAA_joinSession -> DAA_join_u0) mod
       2^DAA_power1 (Erase all but the lowest DAA_power1 bits of s2) */
    /* FIXME Set c = DAA_session -> DAA_digest */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage19: Creating c from DAA_session -> DAA_digest\n");
	rc = TPM_bin2bn(&cBignum, tpm_daa_session_data->DAA_session.DAA_digest, TPM_DIGEST_SIZE);
    }
    /* FIXME Set u0 = DAA_joinSession -> DAA_join_u0 */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage19: Creating u0 from DAA_joinSession -> DAA_join_u0\n");
	rc = TPM_bin2bn(&u0Bignum,
			tpm_daa_session_data->DAA_joinSession.DAA_join_u0,
			sizeof(tpm_daa_session_data->DAA_joinSession.DAA_join_u0));
    }
    /* s2 = (r2 + c * u0) mod_pow */
    if (rc == 0) {
	rc = TPM_ComputeApBxC(&s2Bignum,	/* result */
			      r2Bignum,		/* A */
			      cBignum,		/* B */
			      u0Bignum);	/* C */
    }
    if (rc == 0) {
	rc = TPM_BN_mask_bits(s2Bignum, DAA_power1);
    }
    /* f. set outputData = s2 */
    if (rc == 0) {
	rc = TPM_bn2binMalloc(&(outputData->buffer),
			      &(outputData->size),
			      s2Bignum, 0);
    }
    /* insure that outputData is DAA_power1 bits */
    if (rc == 0) {
	rc = TPM_SizedBuffer_ComputeEnlarge(outputData, DAA_power1 / 8);
    }
    /* g. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* h. return TPM_SUCCESS */
    free(r2);			/* @1 */
    TPM_BN_free(r2Bignum);	/* @2 */
    TPM_BN_free(s2Bignum);	/* @3 */
    TPM_BN_free(cBignum);	/* @4 */
    TPM_BN_free(u0Bignum);	/* @5 */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage20(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData)
{
    TPM_RESULT		rc = 0;
    unsigned char	*r2 = NULL;		/* freed @1 */
    TPM_BIGNUM		r2Bignum = NULL;	/* freed @2 */
    TPM_BIGNUM		s12Bignum = NULL;	/* freed @3 */
    TPM_BIGNUM		s12sBignum = NULL;	/* freed @4 */
    TPM_BIGNUM		cBignum = NULL;		/* freed @5 */
    TPM_BIGNUM		u0Bignum = NULL;	/* freed @6 */

    unsigned int	numBytes;	/* just for debug */

    printf("TPM_DAAJoin_Stage20:\n");
    tpm_state = tpm_state;			/* not used */
    /* a. Verify that DAA_session ->DAA_stage==20. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Obtain DAA_SIZE_r2 bytes using the MGF1 function and label them r2.  "r2" || DAA_session
       -> DAA_contextSeed is the Z seed. */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage20: Creating r2\n");
	rc = TPM_MGF1_GenerateArray(&r2,			/* returned MGF1 array */
				    DAA_SIZE_r2,		/* size of Y */
				    /* length of the entire seed */
				    sizeof("r2") -1 +
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    sizeof("r2") -1, "r2",
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    tpm_daa_session_data->DAA_session.DAA_contextSeed,
				    0, NULL);
    }
    if (rc == 0) {
	rc = TPM_bin2bn(&r2Bignum, r2, DAA_SIZE_r2);
    }
    /* e. Set s12 = r2 + (DAA_session -> DAA_digest)*( DAA_joinSession -> DAA_join_u0)	*/
    /* FIXME Set c = DAA_session -> DAA_digest */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage20: Creating c from DAA_session -> DAA_digest\n");
	rc = TPM_bin2bn(&cBignum, tpm_daa_session_data->DAA_session.DAA_digest, TPM_DIGEST_SIZE);
    }
    /* FIXME Set u0 = DAA_joinSession -> DAA_join_u0 */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage20: Creating u0 from DAA_joinSession -> DAA_join_u0\n");
	rc = TPM_bin2bn(&u0Bignum,
			tpm_daa_session_data->DAA_joinSession.DAA_join_u0,
			sizeof(tpm_daa_session_data->DAA_joinSession.DAA_join_u0));
    }
    /* s12 = (r2 + c * u0) mod_pow */
    if (rc == 0) {
	rc =  TPM_ComputeApBxC(&s12Bignum,	/* result */
			       r2Bignum,	/* A */
			       cBignum,		/* B */
			       u0Bignum);	/* C */
    }
    /* FIXME for debug */
    if (rc == 0) {
	rc = TPM_BN_num_bytes(&numBytes, s12Bignum);
	printf("TPM_DAAJoin_Stage20: e. s12 size %u\n", numBytes);
    }
    /* f. Shift s12 right by DAA_power1 bit (discard the lowest DAA_power1 bits). */
    if (rc == 0) {
	rc = TPM_BN_rshift(&s12sBignum, s12Bignum, DAA_power1); /* s12 becomes s12s */
    }
    if (rc == 0) {
	rc = TPM_BN_num_bytes(&numBytes, s12sBignum);
	printf("TPM_DAAJoin_Stage20: f. s12 size %u\n", numBytes);
    }
    /* g. Set DAA_session -> DAA_scratch = s12 */
    if (rc == 0) {
	rc = TPM_ComputeDAAScratch(tpm_daa_session_data->DAA_session.DAA_scratch,
				   sizeof(tpm_daa_session_data->DAA_session.DAA_scratch),
				   s12sBignum);
    }
    /* h. Set outputData = DAA_session -> DAA_digest */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Set(outputData,
				 TPM_DIGEST_SIZE, tpm_daa_session_data->DAA_session.DAA_digest);
    }
    /* i. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* j. return TPM_SUCCESS */
    free(r2);			/* @1 */
    TPM_BN_free(r2Bignum);	/* @2 */
    TPM_BN_free(s12Bignum);	/* @3 */
    TPM_BN_free(s12sBignum);	/* @4 */
    TPM_BN_free(cBignum);	/* @5 */
    TPM_BN_free(u0Bignum);	/* @6 */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage21(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData)
{
    TPM_RESULT		rc = 0;
    unsigned char	*r3 = NULL;		/* freed @1 */
    TPM_BIGNUM		r3Bignum = NULL;	/* freed @2 */
    TPM_BIGNUM		s3Bignum = NULL;	/* freed @3 */
    TPM_BIGNUM		cBignum = NULL;		/* freed @4 */
    TPM_BIGNUM		u1Bignum = NULL;	/* freed @5 */
    TPM_BIGNUM		s12Bignum = NULL;	/* freed @6 */

    unsigned int	numBytes;	/* just for debug */

    printf("TPM_DAAJoin_Stage21:\n");
    tpm_state = tpm_state;			/* not used */
    /* a. Verify that DAA_session ->DAA_stage==21. Return TPM_DAA_STAGE and flush handle on
       mismatch	 */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Obtain DAA_SIZE_r3 bytes using the MGF1 function and label them r3.  "r3" || DAA_session
       -> DAA_contextSeed) is the Z seed. */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage21: Creating r3\n");
	rc = TPM_MGF1_GenerateArray(&r3,			/* returned MGF1 array */
				    DAA_SIZE_r3,		/* size of r3 */
				    /* length of the entire seed */
				    sizeof("r3") -1 +
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    sizeof("r3") -1, "r3",
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    tpm_daa_session_data->DAA_session.DAA_contextSeed,
				    0, NULL);
    }
    if (rc == 0) {
	rc = TPM_bin2bn(&r3Bignum, r3, DAA_SIZE_r3);
    }
    /* just for debug */
    if (rc == 0) {
	rc = TPM_BN_num_bytes(&numBytes, r3Bignum);
	printf("TPM_DAAJoin_Stage21: r3 size %u\n", numBytes);
    }
    /* e. Set s3 = r3 + (DAA_session -> DAA_digest)*( DAA_joinSession -> DAA_join_u1) + (DAA_session
       -> DAA_scratch). */
    /* FIXME Set c = DAA_session -> DAA_digest */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage21: Creating c from DAA_session -> DAA_digest\n");
	rc = TPM_bin2bn(&cBignum, tpm_daa_session_data->DAA_session.DAA_digest, TPM_DIGEST_SIZE);
    }
    /* just for debug */
    if (rc == 0) {
	rc = TPM_BN_num_bytes(&numBytes, cBignum);
	printf("TPM_DAAJoin_Stage21: c size %u\n", numBytes);
    }
    /* FIXME Set u1 = DAA_joinSession -> DAA_join_u1 */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage21: Creating u1 from DAA_joinSession -> DAA_join_u1\n");
	rc = TPM_bin2bn(&u1Bignum,
			tpm_daa_session_data->DAA_joinSession.DAA_join_u1,
			sizeof(tpm_daa_session_data->DAA_joinSession.DAA_join_u1));
    }
    /* just for debug */
    if (rc == 0) {
	rc = TPM_BN_num_bytes(&numBytes, u1Bignum);
	printf("TPM_DAAJoin_Stage21: u1 size %u\n", numBytes);
    }
    /* FIXME Set s12 = DAA_session -> DAA_scratch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage21: Creating s12 from DAA_session -> DAA_scratch\n");
	rc = TPM_bin2bn(&s12Bignum,
			tpm_daa_session_data->DAA_session.DAA_scratch,
			sizeof(tpm_daa_session_data->DAA_session.DAA_scratch));
    }
    if (rc == 0) {
	rc = TPM_BN_num_bytes(&numBytes, s12Bignum);
	printf("TPM_DAAJoin_Stage21: s12 size %u\n", numBytes);
    }
    /* s3 = r3 + c * u1 + s12 */
    if (rc == 0) {
	rc = TPM_ComputeApBxCpD(&s3Bignum,	/* freed by caller */
				r3Bignum,	/* A */
				cBignum,	/* B */
				u1Bignum,	/* C */
				s12Bignum);	/* D */
    }	 
    if (rc == 0) {
	rc = TPM_BN_num_bytes(&numBytes, s3Bignum);
	printf("TPM_DAAJoin_Stage21: s3 size %u\n", numBytes);
    }
    /* f. Set DAA_session -> DAA_scratch = NULL */
    if (rc == 0) {
	tpm_daa_session_data->DAA_session.DAA_scratch_null = TRUE;
    }
    /* g. set outputData = s3 */
    if (rc == 0) {
	rc = TPM_bn2binMalloc(&(outputData->buffer),
			      &(outputData->size),
			      s3Bignum, 0);
    }
    /* h. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* i. return TPM_SUCCESS */
    free(r3);			/* @1 */
    TPM_BN_free(r3Bignum);	/* @2 */
    TPM_BN_free(s3Bignum);	/* @3 */
    TPM_BN_free(cBignum);	/* @4 */
    TPM_BN_free(u1Bignum);	/* @5 */
    TPM_BN_free(s12Bignum);	/* @6 */
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage22(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData,
			       TPM_SIZED_BUFFER *inputData0)
{
    TPM_RESULT		rc = 0;
    TPM_BIGNUM		v10Bignum = NULL;	/* freed @1 */
    TPM_BIGNUM		v10sBignum = NULL;	/* freed @2 */
    TPM_BIGNUM		u0Bignum = NULL;	/* freed @3 */
    TPM_BIGNUM		u2Bignum = NULL;	/* freed @4 */
    TPM_BIGNUM		v0Bignum = NULL;	/* freed @5 */
    TPM_DAA_SENSITIVE	tpm_daa_sensitive;
    
    unsigned int	numBytes;	/* just for debug */

    printf("TPM_DAAJoin_Stage22:\n");
    TPM_DAASensitive_Init(&tpm_daa_sensitive);	/* freed @6 */
    /* a. Verify that DAA_session ->DAA_stage==22. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Verify inputSize0 == DAA_SIZE_v0 and return error TPM_DAA_INPUT_DATA0 on mismatch */
    if (rc == 0) {
	if (inputData0->size != DAA_SIZE_v0) {
	    printf("TPM_DAAJoin_Stage22: Error, inputData0 size %u should be %u\n",
		   inputData0->size, DAA_SIZE_v0);
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* e. Set u2 = inputData0 */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage22: Creating u2\n");
	rc = TPM_bin2bn(&u2Bignum, inputData0->buffer, inputData0->size);
    }
    /* f. Set v0 = u2 + (DAA_joinSession -> DAA_join_u0) mod 2^DAA_power1 (Erase all but the lowest
       DAA_power1 bits of v0). */
    /* FIXME Set u0 = DAA_joinSession -> DAA_join_u0 */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage22: Creating u0 from DAA_joinSession -> DAA_join_u0\n");
	rc = TPM_bin2bn(&u0Bignum,
			tpm_daa_session_data->DAA_joinSession.DAA_join_u0,
			sizeof(tpm_daa_session_data->DAA_joinSession.DAA_join_u0));
    }
    /* FIXME factor this? */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage22: Calculate v0\n");
	rc = TPM_BN_new(&v0Bignum);
    }
    /* v0 = u2 + u0 */
    if (rc == 0) {
	rc = TPM_BN_add(v0Bignum, u2Bignum, u0Bignum);
    }
    if (rc == 0) {
	rc = TPM_BN_num_bytes(&numBytes, v0Bignum);
	printf("TPM_DAAJoin_Stage22: f. v0 size before mask %u\n", numBytes);
    }
    /* v0 = v0 mod 2^DAA_power1 */
    if (rc == 0) {
	rc = TPM_BN_mask_bits(v0Bignum, DAA_power1);
    }
    if (rc == 0) {
	rc = TPM_BN_num_bytes(&numBytes, v0Bignum);
	printf("TPM_DAAJoin_Stage22: f. v0 size after mask %u\n", numBytes);
    }
    /* g. Set DAA_tpmSpecific -> DAA_digest_v0 = SHA-1(v0) */
    if (rc == 0) {
	rc = TPM_SHA1_BignumGenerate(tpm_daa_session_data->DAA_tpmSpecific.DAA_digest_v0,
				     v0Bignum,
				     (DAA_power1 + 7) / 8);	/* canonicalize the number of
								   bytes */
    }
    /* h. Set v10 = u2 + (DAA_joinSession -> DAA_join_u0) in Z. Compute over the integers.
       The computation is not reduced with a modulus. */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage22: Calculate v10\n");
	rc = TPM_BN_new(&v10Bignum);
    }
    /* v0 = u2 + u0 */
    if (rc == 0) {
	rc = TPM_BN_add(v10Bignum, u2Bignum, u0Bignum);
    }
    /* i. Shift v10 right by DAA_power1 bits (erase the lowest DAA_power1 bits). */
    if (rc == 0) {
	rc = TPM_BN_rshift(&v10sBignum, v10Bignum, DAA_power1);
    }
    /* j. Set DAA_session -> DAA_scratch = v10 */
    if (rc == 0) {
	rc = TPM_ComputeDAAScratch(tpm_daa_session_data->DAA_session.DAA_scratch,
				   sizeof(tpm_daa_session_data->DAA_session.DAA_scratch),
				   v10sBignum);
    }
    /* k. Set outputData */
    /* i. Fill in TPM_DAA_BLOB with a type of TPM_RT_DAA_V0 and encrypt the v0 parameters using
       TPM_PERMANENT_DATA -> daaBlobKey */
    /* Create a TPM_DAA_SENSITIVE structure */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage22: Create TPM_DAA_SENSITIVE\n");
	/* Set TPM_DAA_SENSITIVE -> internalData to v0Bignum */
	rc = TPM_bn2binMalloc(&(tpm_daa_sensitive.internalData.buffer),
			      &(tpm_daa_sensitive.internalData.size),
			      v0Bignum, 0);
    }
    if (rc == 0) {
	rc = TPM_ComputeEncrypt(outputData,
				tpm_state,
				&tpm_daa_sensitive,
				TPM_RT_DAA_V0);
    }
    /* l. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* m. set DAA_session -> DAA_digestContext = SHA-1(DAA_tpmSpecific || DAA_joinSession)  */
    if (rc == 0) {
	rc = TPM_DAADigestContext_GenerateDigestJoin
	     (tpm_daa_session_data->DAA_session.DAA_digestContext, tpm_daa_session_data);
    }
    /* n. return TPM_SUCCESS */
    TPM_BN_free(v10Bignum);				/* @1 */
    TPM_BN_free(v10sBignum);				/* @2 */
    TPM_BN_free(u0Bignum);				/* @3 */
    TPM_BN_free(u2Bignum);				/* @4 */
    TPM_BN_free(v0Bignum);				/* @5 */
    TPM_DAASensitive_Delete(&tpm_daa_sensitive);	/* @6 */ 
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage23(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData,
			       TPM_SIZED_BUFFER *inputData0)
{
    TPM_RESULT		rc = 0;
    TPM_BIGNUM		u1Bignum = NULL;	/* freed @1 */
    TPM_BIGNUM		u3Bignum = NULL;	/* freed @2 */
    TPM_BIGNUM		v1Bignum = NULL;	/* freed @3 */
    TPM_BIGNUM		v10Bignum = NULL;	/* freed @4 */
    TPM_DAA_SENSITIVE	tpm_daa_sensitive;

    printf("TPM_DAAJoin_Stage23:\n");
    TPM_DAASensitive_Init(&tpm_daa_sensitive);	/* freed @5 */
    /* a. Verify that DAA_session ->DAA_stage==23. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Verify inputSize0 == DAA_SIZE_v1 and return error TPM_DAA_INPUT_DATA0 on  */
    /* mismatch */
    if (rc == 0) {
	if (inputData0->size != DAA_SIZE_v1) {
	    printf("TPM_DAAJoin_Stage23: Error, inputData0 size %u should be %u\n",
		   inputData0->size, DAA_SIZE_v1);
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* e. Set u3 = inputData0 */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage23: Creating u3\n");
	rc = TPM_bin2bn(&u3Bignum, inputData0->buffer, inputData0->size);
    }
    /* f. Set v1 = u3 + DAA_joinSession -> DAA_join_u1 + DAA_session -> DAA_scratch */
    /* FIXME Set u1 = DAA_joinSession -> DAA_join_u1 */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage23: Creating u1 from DAA_joinSession -> DAA_join_u1\n");
	rc = TPM_bin2bn(&u1Bignum,
			tpm_daa_session_data->DAA_joinSession.DAA_join_u1,
			sizeof(tpm_daa_session_data->DAA_joinSession.DAA_join_u1));
    }
    /* FIXME Set v10 = DAA_session -> DAA_scratch */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage23: Creating v10\n");
	rc = TPM_bin2bn(&v10Bignum,
			tpm_daa_session_data->DAA_session.DAA_scratch,
			sizeof(tpm_daa_session_data->DAA_session.DAA_scratch));
    }
    if (rc == 0) {
	rc = TPM_BN_new(&v1Bignum);
    }
    /* f. Set v1 = u3 + u1 + v10 */
    if (rc == 0) {
	rc = TPM_BN_add(v1Bignum, u3Bignum, u1Bignum);
    }
    if (rc == 0) {
	rc = TPM_BN_add(v1Bignum, v1Bignum,v10Bignum);
    }
    /* g. Set DAA_tpmSpecific -> DAA_digest_v1 = SHA-1(v1) */
    if (rc == 0) {
	rc = TPM_SHA1_BignumGenerate(tpm_daa_session_data->DAA_tpmSpecific.DAA_digest_v1,
				     v1Bignum,
				     DAA_SIZE_v1);	/* canonicalize the number of bytes */
    }
    /* h. Set outputData */
    /* i. Fill in TPM_DAA_BLOB with a type of TPM_RT_DAA_V1 and encrypt the v1 parameters using
       TPM_PERMANENT_DATA -> daaBlobKey */
    /* Create a TPM_DAA_SENSITIVE structure */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage23: Create TPM_DAA_SENSITIVE\n");
	/* Set TPM_DAA_SENSITIVE -> internalData to v1Bignum */
	rc = TPM_bn2binMalloc(&(tpm_daa_sensitive.internalData.buffer),
			      &(tpm_daa_sensitive.internalData.size),
			      v1Bignum, 0);
    }
    if (rc == 0) {
	rc = TPM_ComputeEncrypt(outputData,
				tpm_state,
				&tpm_daa_sensitive,
				TPM_RT_DAA_V1);
    }
    
    
    /* i. Set DAA_session -> DAA_scratch = NULL */
    if (rc == 0) {
	tpm_daa_session_data->DAA_session.DAA_scratch_null = TRUE;
    }
    /* j. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* k. set DAA_session -> DAA_digestContext = SHA-1(DAA_tpmSpecific || DAA_joinSession)  */
    if (rc == 0) {
	rc = TPM_DAADigestContext_GenerateDigestJoin
	     (tpm_daa_session_data->DAA_session.DAA_digestContext, tpm_daa_session_data);
    }
    /* l. return TPM_SUCCESS */
    TPM_BN_free(u1Bignum);				/* @1 */
    TPM_BN_free(u3Bignum);				/* @2 */
    TPM_BN_free(v1Bignum);				/* @3 */
    TPM_BN_free(v10Bignum);				/* @4 */
    TPM_DAASensitive_Delete(&tpm_daa_sensitive);	/* @5 */ 
    return rc;
}

TPM_RESULT TPM_DAAJoin_Stage24(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData)
{
    TPM_RESULT		rc = 0;
    TPM_DAA_SENSITIVE	tpm_daa_sensitive;

    printf("TPM_DAAJoin_Stage24:\n");
    TPM_DAASensitive_Init(&tpm_daa_sensitive);	/* freed @1 */
    /* a. Verify that DAA_session ->DAA_stage==24. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession)
       and return error TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. set outputData = enc(DAA_tpmSpecific) using TPM_PERMANENT_DATA -> daaBlobKey */
    /* Create a TPM_DAA_SENSITIVE structure */
    if (rc == 0) {
	printf("TPM_DAAJoin_Stage24 Create TPM_DAA_SENSITIVE\n");
	/* Set TPM_DAA_SENSITIVE -> internalData to DAA_tpmSpecific */
	rc = TPM_SizedBuffer_SetStructure(&(tpm_daa_sensitive.internalData),
					  &(tpm_daa_session_data->DAA_tpmSpecific),
					  (TPM_STORE_FUNCTION_T)TPM_DAATpm_Store);
    }
    if (rc == 0) {
	rc = TPM_ComputeEncrypt(outputData,
				tpm_state,
				&tpm_daa_sensitive,
				TPM_RT_DAA_TPM);
    }
    /* e. return TPM_SUCCESS */
    TPM_DAASensitive_Delete(&tpm_daa_sensitive);	/* @2 */ 
    return rc;
}

TPM_RESULT TPM_DAASign_Stage00(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA **tpm_daa_session_data,	/* returns entry in
										   array */
			       TPM_BOOL *daaHandleValid,
			       TPM_SIZED_BUFFER *outputData,
			       TPM_SIZED_BUFFER *inputData0)
{
    TPM_RESULT		rc = 0;
    unsigned char	*stream;
    uint32_t		stream_size;
    TPM_HANDLE		daaHandle = 0;		/* no preassigned handle */
    
    printf("TPM_DAASign_Stage00:\n");
    /* a. Determine that sufficient resources are available to perform a TPM_DAA_Sign. */
    /* i. The TPM MUST support sufficient resources to perform one (1)
       TPM_DAA_Join/TPM_DAA_Sign. The TPM MAY support addition TPM_DAA_Join/ TPM_DAA_Sign
       sessions. */
    /* ii. The TPM may share internal resources between the DAA operations and other variable
       resource requirements: */
    /* iii. If there are insufficient resources within the stored key pool (and one or more keys
       need to be removed to permit the DAA operation to execute) return TPM_NOSPACE */
    /* iv. If there are insufficient resources within the stored session pool (and one or more
       authorization or transport sessions need to be removed to permit the DAA operation to
       execute), return TPM_RESOURCES. */
    if (rc == 0) {
	rc = TPM_DaaSessions_GetNewHandle(tpm_daa_session_data, /* returns entry in array */
					  &daaHandle,		/* output */
					  daaHandleValid,	/* output */
					  tpm_state->tpm_stclear_data.daaSessions); /* array */
    }
    /* b. Set DAA_issuerSettings = inputData0 */
    if (rc == 0) {
	stream = inputData0->buffer;
	stream_size = inputData0->size;
	rc = TPM_DAAIssuer_Load(&((*tpm_daa_session_data)->DAA_issuerSettings),
				&stream, &stream_size);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* c. Verify that all fields in DAA_issuerSettings are present and return error
       TPM_DAA_INPUT_DATA0 if not. */
    if (rc == 0) {
	if (stream_size != 0) {
	    printf("TPM_DAASign_Stage00: Error, bad input0 size %u\n", inputData0->size);
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    if (rc == 0) {
	/* d. set all fields in DAA_session = NULL */
	/* e. Assign new handle for session */
	/* NOTE Done by TPM_DaaSessions_GetNewHandle() */
	printf("TPM_DAASign_Stage00: handle %08x\n", (*tpm_daa_session_data)->daaHandle);
	/* f. Set outputData to new handle */
	/* i. The handle in outputData is included the output HMAC. */
	rc = TPM_SizedBuffer_Append32(outputData, (*tpm_daa_session_data)->daaHandle);
    }
    /* g. set DAA_session -> DAA_stage = 1 */
    /* NOTE Done by common code */
    /* h. return TPM_SUCCESS */
    return rc;
}
			       
TPM_RESULT TPM_DAASign_Stage01(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData,
			       TPM_SIZED_BUFFER *inputData0)
{
    TPM_RESULT		rc = 0;
    TPM_DAA_SENSITIVE	tpm_daa_sensitive;
    unsigned char	*stream;
    uint32_t		stream_size;

    printf("TPM_DAASign_Stage01:\n");
    outputData = outputData;			/* not used */
    TPM_DAASensitive_Init(&tpm_daa_sensitive);	/* freed @1 */
    /* a. Verify that DAA_session ->DAA_stage==1. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Set DAA_tpmSpecific = unwrap(inputData0) using TPM_PERMANENT_DATA -> daaBlobKey */
    if (rc == 0) {
	rc = TPM_ComputeDecrypt(&tpm_daa_sensitive,	/* output */
				tpm_state,		/* decryption and HMAC keys */
				inputData0,		/* encrypted stream */
				TPM_RT_DAA_TPM);	/* resourceType expected */
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    if (rc == 0) {
	stream = tpm_daa_sensitive.internalData.buffer;
	stream_size = tpm_daa_sensitive.internalData.size;
	rc = TPM_DAATpm_Load(&(tpm_daa_session_data->DAA_tpmSpecific), &stream, &stream_size);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* c. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    
    /* d. set DAA_session -> DAA_digestContext = SHA-1(DAA_tpmSpecific)	 */
    if (rc == 0) {
	rc = TPM_SHA1_GenerateStructure(tpm_daa_session_data->DAA_session.DAA_digestContext,
					&(tpm_daa_session_data->DAA_tpmSpecific),
					(TPM_STORE_FUNCTION_T)TPM_DAATpm_Store);
    }
    /* e set outputData = NULL */
    /* NOTE Done by caller */
    /* f. set DAA_session -> DAA_stage =2 */
    /* NOTE Done by common code */
    /* g. return TPM_SUCCESS */
    TPM_DAASensitive_Delete(&tpm_daa_sensitive);	/* @1 */
    return rc;
}
			       
TPM_RESULT TPM_DAASign_Stage05(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData,
			       TPM_SIZED_BUFFER *inputData0,
			       TPM_SIZED_BUFFER *inputData1)
{
    TPM_RESULT		rc = 0;
    unsigned char	*Y = NULL;	/* freed @1 */
    TPM_BIGNUM		yBignum = NULL;	/* freed @2 */
    TPM_BIGNUM		xBignum = NULL;	/* freed @3 */
    TPM_BIGNUM		nBignum = NULL;	/* freed @4 */
    TPM_BIGNUM		zBignum = NULL;	/* freed @5 */

    printf("TPM_DAASign_Stage05:\n");
    tpm_state = tpm_state;		/* not used */
    /* a. Verify that DAA_session ->DAA_stage==5. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific) and return error
       TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Set DAA_generic_S1 = inputData0 */
    /* e. Verify that SHA-1(DAA_generic_S1) == DAA_issuerSettings -> DAA_digest_S1 and return error
       TPM_DAA_INPUT_DATA0 on mismatch */
    if (rc == 0) {
	printf("TPM_DAASign_Stage05: Checking DAA_generic_S1\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_S1,	/* expect */
			    inputData0->size, inputData0->buffer,	/* DAA_generic_S1 */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* f. Set DAA_generic_n = inputData1 */
    /* g. Verify that SHA-1(DAA_generic_n) == DAA_issuerSettings -> DAA_digest_n and return error
       TPM_DAA_INPUT_DATA1 on mismatch */
    if (rc == 0) {
	printf("TPM_DAASign_Stage05: Checking DAA_digest_n\n");
	rc = TPM_SHA1_Check(tpm_daa_session_data->DAA_issuerSettings.DAA_digest_n,	/* expect */
			    inputData1->size, inputData1->buffer,	/* DAA_generic_n */
			    0, NULL);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA1;
	}
    }
    /* h. Obtain DAA_SIZE_r4 bytes using the MGF1 function and label them Y.  "r4" || DAA_session ->
       DAA_contextSeed is the Z seed. */
    if (rc == 0) {
	printf("TPM_DAASign_Stage05: Creating Y\n");
	rc = TPM_MGF1_GenerateArray(&Y,			/* returned MGF1 array */
				    DAA_SIZE_r4,		/* size of Y */
				    /* length of the entire seed */
				    sizeof("r4") -1 +
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    sizeof("r4") -1, "r4",
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    tpm_daa_session_data->DAA_session.DAA_contextSeed,
				    0, NULL);
    }
    if (rc == 0) {
	rc = TPM_bin2bn(&yBignum, Y, DAA_SIZE_r4);
    }
    /* i. Set X = DAA_generic_S1 */
    if (rc == 0) {
	printf("TPM_DAASign_Stage05 Creating X\n");
	rc = TPM_bin2bn(&xBignum, inputData0->buffer, inputData0->size);
    }
    /* j. Set n = DAA_generic_n */
    if (rc == 0) {
	printf("TPM_DAASign_Stage05: Creating n\n");
	rc = TPM_bin2bn(&nBignum, inputData1->buffer, inputData1->size);
    }
    /* k. Set Z = DAA_session -> DAA_scratch */
    if (rc == 0) {
	printf("TPM_DAASign_Stage05: Creating Z\n");
	rc = TPM_bin2bn(&zBignum,
			tpm_daa_session_data->DAA_session.DAA_scratch,
			sizeof(tpm_daa_session_data->DAA_session.DAA_scratch));
    }
    /* l. Set DAA_session -> DAA_scratch = Z*(X^Y) mod n */
    if (rc == 0) {
	rc = TPM_ComputeZxAexpPmodn(tpm_daa_session_data->DAA_session.DAA_scratch,
				    sizeof(tpm_daa_session_data->DAA_session.DAA_scratch),
				    zBignum,	/* Z */
				    xBignum,	/* A */
				    yBignum,	/* P */
				    nBignum);	/* N */
    }
    /* m. set outputData = DAA_session -> DAA_scratch */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Set(outputData,
				 sizeof(tpm_daa_session_data->DAA_session.DAA_scratch),
				 tpm_daa_session_data->DAA_session.DAA_scratch);
    }
    /* n. set DAA_session -> DAA_scratch = NULL */
    if (rc == 0) {
	tpm_daa_session_data->DAA_session.DAA_scratch_null = TRUE;
    }
    /* o. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* p. return TPM_SUCCESS */
    free(Y);			/* @1 */
    TPM_BN_free(yBignum);	/* @2 */
    TPM_BN_free(xBignum);	/* @3 */
    TPM_BN_free(nBignum);	/* @4 */
    TPM_BN_free(zBignum);	/* @5 */
    return rc;
}
			       
TPM_RESULT TPM_DAASign_Stage10(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData,
			       TPM_SIZED_BUFFER *inputData0,
			       TPM_SIZED_BUFFER *inputData1)
{
    TPM_RESULT		rc = 0;
    unsigned char	*stream;
    uint32_t		stream_size;
    uint8_t		selector;
    TPM_BOOL		parentPCRStatus;
    TPM_KEY_HANDLE	keyHandle;
    TPM_KEY		*identityKey = NULL;		/* the key specified by keyHandle */

    printf("TPM_DAASign_Stage10:\n");
    /* a. Verify that DAA_session ->DAA_stage==10. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific) and return error
       TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Verify that inputSize0 == sizeOf(BYTE), and return error TPM_DAA_INPUT_DATA0 on
       mismatch */
    /* e. Set selector = inputData0, verify that selector == 0 or 1, and return error
       TPM_DAA_INPUT_DATA0 on mismatch */
    if (rc == 0) {
	stream = inputData0->buffer;
	stream_size = inputData0->size;
	rc = TPM_Load8(&selector, &stream, &stream_size);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    if (rc == 0) {
	if (stream_size != 0) {
	    printf("TPM_DAASign_Stage10: Error, bad input0 size %u\n", inputData0->size);
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    if (rc == 0) {
	printf("TPM_DAASign_Stage10: selector %u\n", selector);
	switch (selector) {
	  case 1:
	    /* f. If selector == 1, verify that inputSize1 == sizeOf(TPM_DIGEST), and return error
	       TPM_DAA_INPUT_DATA1 on mismatch */
	    if (rc == 0) {
		if (inputData1->size != TPM_DIGEST_SIZE) {
		    printf("TPM_DAASign_Stage10: Error, bad input1 size %u\n", inputData1->size);
		    rc = TPM_DAA_INPUT_DATA1;
		}
	    }
	    /* g. Set DAA_session -> DAA_digest to SHA-1 (DAA_session -> DAA_digest || 1 ||
	       inputData1) */
	    if (rc == 0) {
		rc = TPM_SHA1(tpm_daa_session_data->DAA_session.DAA_digest,
			      TPM_DIGEST_SIZE, tpm_daa_session_data->DAA_session.DAA_digest,
			      1, &selector ,
			      inputData1->size, inputData1->buffer,
			      0, NULL);
		if (rc != 0) {
		    rc = TPM_DAA_INPUT_DATA1;
		}
	    }
	    break;
	  case 0:
	    /* h. If selector == 0, verify that inputData1 is a handle to a TPM identity key (AIK),
	       and return error TPM_DAA_INPUT_DATA1 on mismatch */
	    /* get the key handle */
	    if (rc == 0) {
		stream = inputData1->buffer;
		stream_size = inputData1->size;
		rc = TPM_Load32(&keyHandle, &stream, &stream_size);
		if (rc != 0) {
		    rc = TPM_DAA_INPUT_DATA1;
		}
	    }
	    /* validate inputData1 */
	    if (rc == 0) {
		if (stream_size != 0) {
		    printf("TPM_DAASign_Stage10: Error, bad input1 size %u\n", inputData1->size);
		    rc = TPM_DAA_INPUT_DATA1;
		}
	    }
	    /* get the key */
	    if (rc == 0) {
		rc = TPM_KeyHandleEntries_GetKey(&identityKey, &parentPCRStatus,
						 tpm_state, keyHandle,
						 TRUE,		/* read only */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
		if (rc != 0) {
		    rc = TPM_DAA_INPUT_DATA1;
		}
	    }
	    /* validate that it's an AIK */
	    if (rc == 0) {
		if (identityKey->keyUsage != TPM_KEY_IDENTITY) {
		    printf("TPM_DAASign_Stage10: Error, "
			   "key keyUsage %04hx must be TPM_KEY_IDENTITY\n", identityKey->keyUsage);
		    rc = TPM_DAA_INPUT_DATA1;
		}
	    }
	    /* i. Set DAA_session -> DAA_digest to SHA-1 (DAA_session -> DAA_digest || 0 || n2)
	       where n2 is the modulus of the AIK */
	    if (rc == 0) {
		rc = TPM_SHA1(tpm_daa_session_data->DAA_session.DAA_digest,
			      TPM_DIGEST_SIZE, tpm_daa_session_data->DAA_session.DAA_digest,
			      1, &selector,
			      identityKey->pubKey.size, identityKey->pubKey.buffer,
			      0, NULL);
	    }
	    break;
	  default:
	    printf("TPM_DAASign_Stage10: Error, bad selector %u\n", selector);
	    rc = TPM_DAA_INPUT_DATA0;
	    break;
	}
    }
    /* j. Set outputData = DAA_session -> DAA_digest  */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Set(outputData,
				 TPM_DIGEST_SIZE, tpm_daa_session_data->DAA_session.DAA_digest);
    }
    /* k. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* l. return TPM_SUCCESS. */
    return rc;
}

TPM_RESULT TPM_DAASign_Stage13(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData,
			       TPM_SIZED_BUFFER *inputData0)
{
    TPM_RESULT		rc = 0;
    unsigned char	*r2 = NULL;		/* freed @1 */
    TPM_BIGNUM		r2Bignum = NULL;	/* freed @2 */
    TPM_BIGNUM		s2Bignum = NULL;	/* freed @3 */
    TPM_BIGNUM		cBignum = NULL;		/* freed @4 */
    TPM_BIGNUM		v0Bignum = NULL;	/* freed @5 */
    TPM_DAA_SENSITIVE	tpm_daa_sensitive;

    printf("TPM_DAASign_Stage13:\n");
    TPM_DAASensitive_Init(&tpm_daa_sensitive);	/* freed @6 */
    /* a. Verify that DAA_session ->DAA_stage==13. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific) and return error
       TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Set DAA_private_v0= unwrap(inputData0) using TPM_PERMANENT_DATA -> daaBlobKey */
    if (rc == 0) {
	printf("TPM_DAASign_Stage13: unwrapping to v0\n");
	rc = TPM_ComputeDecrypt(&tpm_daa_sensitive,	/* output */
				tpm_state,		/* decryption and HMAC keys */
				inputData0,		/* encrypted stream */
				TPM_RT_DAA_V0);		/* resourceType expected */
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* e. Verify that SHA-1(DAA_private_v0) == DAA_tpmSpecific -> DAA_digest_v0 and return error
       TPM_DAA_INPUT_DATA0 on mismatch */
    if (rc == 0) {
	printf("TPM_DAASign_Stage13: Checking v0\n");
	rc = TPM_SHA1_SizedBufferCheck(tpm_daa_session_data->DAA_tpmSpecific.DAA_digest_v0,
				       &(tpm_daa_sensitive.internalData),
				       (DAA_power1 + 7) / 8);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* f. Obtain DAA_SIZE_r2 bytes from the MGF1 function and label them r2.  "r2" || DAA_session ->
       DAA_contextSeed) is the Z seed. */
    if (rc == 0) {
	printf("TPM_DAASign_Stage13 Creating r2\n");
	rc = TPM_MGF1_GenerateArray(&r2,			/* returned MGF1 array */
				    DAA_SIZE_r2,		/* size of Y */
				    /* length of the entire seed */
				    sizeof("r2") -1 +
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    sizeof("r2") -1, "r2",
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    tpm_daa_session_data->DAA_session.DAA_contextSeed,
				    0, NULL);
    }
    if (rc == 0) {
	rc = TPM_bin2bn(&r2Bignum, r2, DAA_SIZE_r2);
    }
    /* g. Set s2 = r2 + (DAA_session -> DAA_digest)*( DAA_private_v0) mod 2^DAA_power1 */
    /* (erase all but the lowest DAA_power1 bits of s2) */
    /* FIXME Set c = DAA_session -> DAA_digest */
    if (rc == 0) {
	printf("TPM_DAASign_Stage13: Creating c from DAA_session -> DAA_digest\n");
	rc = TPM_bin2bn(&cBignum, tpm_daa_session_data->DAA_session.DAA_digest, TPM_DIGEST_SIZE);
    }
    /* FIXME Set v0 = DAA_private_v0 */
    if (rc == 0) {
	rc = TPM_bin2bn(&v0Bignum,
			tpm_daa_sensitive.internalData.buffer,
			tpm_daa_sensitive.internalData.size);
    }
    /* s2 = r2 + c * v0 mod 2^DAA_power1 */ 
    if (rc == 0) {
	rc =  TPM_ComputeApBxC(&s2Bignum,	/* result */
			       r2Bignum,	/* A */
			       cBignum,		/* B */
			       v0Bignum);	/* C */
    }
    if (rc == 0) {
	rc = TPM_BN_mask_bits(s2Bignum, DAA_power1);
    }
    /* h. set outputData = s2 */
    if (rc == 0) {
	rc = TPM_bn2binMalloc(&(outputData->buffer),
			      &(outputData->size),
			      s2Bignum, 0);
    }
    /* i. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* j. return TPM_SUCCESS */
    free(r2);						/* @1 */
    TPM_BN_free(r2Bignum);				/* @2 */
    TPM_BN_free(s2Bignum);				/* @3 */
    TPM_BN_free(cBignum);				/* @4 */
    TPM_BN_free(v0Bignum);				/* @5 */
    TPM_DAASensitive_Delete(&tpm_daa_sensitive);	/* @6 */
    return rc;
}

TPM_RESULT TPM_DAASign_Stage14(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData,
			       TPM_SIZED_BUFFER *inputData0)
{
    TPM_RESULT		rc = 0;
    unsigned char	*r2 = NULL;		/* freed @1 */
    TPM_BIGNUM		r2Bignum = NULL;	/* freed @2 */
    TPM_BIGNUM		s12Bignum = NULL;	/* freed @3 */
    TPM_BIGNUM		s12sBignum = NULL;	/* freed @4 */
    TPM_BIGNUM		cBignum = NULL;		/* freed @5 */
    TPM_BIGNUM		v0Bignum = NULL;	/* freed @6 */
    TPM_DAA_SENSITIVE	tpm_daa_sensitive;

    printf("TPM_DAASign_Stage14:\n");
    outputData = outputData;			/* not used */
    TPM_DAASensitive_Init(&tpm_daa_sensitive);	/* freed @7 */
    /* a. Verify that DAA_session ->DAA_stage==14. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific) and return error
       TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Set DAA_private_v0= unwrap(inputData0) using TPM_PERMANENT_DATA -> daaBlobKey */
    if (rc == 0) {
	rc = TPM_ComputeDecrypt(&tpm_daa_sensitive,	/* output */
				tpm_state,		/* decryption and HMAC keys */
				inputData0,		/* encrypted stream */
				TPM_RT_DAA_V0);		/* resourceType expected */
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* e. Verify that SHA-1(DAA_private_v0) == DAA_tpmSpecific -> DAA_digest_v0 and return error
       TPM_DAA_INPUT_DATA0 on mismatch */
    if (rc == 0) {
	printf("TPM_DAASign_Stage14: Checking v0\n");
	rc = TPM_SHA1_SizedBufferCheck(tpm_daa_session_data->DAA_tpmSpecific.DAA_digest_v0,
				       &(tpm_daa_sensitive.internalData),
				       (DAA_power1 + 7) / 8);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* f. Obtain DAA_SIZE_r2 bytes from the MGF1 function and label them r2.  "r2" || DAA_session ->
       DAA_contextSeed is the Z seed. */
    if (rc == 0) {
	printf("TPM_DAASign_Stage14: Creating r2\n");
	rc = TPM_MGF1_GenerateArray(&r2,			/* returned MGF1 array */
				    DAA_SIZE_r2,		/* size of Y */
				    /* length of the entire seed */
				    sizeof("r2") -1 +
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    sizeof("r2") -1, "r2",
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    tpm_daa_session_data->DAA_session.DAA_contextSeed,
				    0, NULL);
    }
    if (rc == 0) {
	rc = TPM_bin2bn(&r2Bignum, r2, DAA_SIZE_r2);
    }
    /* g. Set s12 = r2 + (DAA_session -> DAA_digest)*(DAA_private_v0). */
    /* FIXME Set c = DAA_session -> DAA_digest */
    if (rc == 0) {
	printf("TPM_DAASign_Stage14: Creating c from DAA_session -> DAA_digest\n");
	rc = TPM_bin2bn(&cBignum, tpm_daa_session_data->DAA_session.DAA_digest, TPM_DIGEST_SIZE);
    }
    /* FIXME Set v0 = DAA_private_v0 */
    if (rc == 0) {
	rc = TPM_bin2bn(&v0Bignum,
			tpm_daa_sensitive.internalData.buffer,
			tpm_daa_sensitive.internalData.size);
    }
    /* s12 = r2 + c * v0 */
    if (rc == 0) {
	rc =  TPM_ComputeApBxC(&s12Bignum,	/* result */
			       r2Bignum,	/* A */
			       cBignum,		/* B */
			       v0Bignum);	/* C */
    }
    /* h. Shift s12 right by DAA_power1 bits (erase the lowest DAA_power1 bits). */
    if (rc == 0) {
	rc = TPM_BN_rshift(&s12sBignum, s12Bignum, DAA_power1); /* f becomes f1 */
    }
    /* i. Set DAA_session -> DAA_scratch = s12 */
    if (rc == 0) {
	rc = TPM_ComputeDAAScratch(tpm_daa_session_data->DAA_session.DAA_scratch,
				   sizeof(tpm_daa_session_data->DAA_session.DAA_scratch),
				   s12sBignum);
    }
    /* j. set outputData = NULL */
    /* NOTE Done by caller */
    /* k. increment DAA_session -> DAA_stage by 1 */
    /* NOTE Done by common code */
    /* l. return TPM_SUCCESS */
    free(r2);						/* @1 */
    TPM_BN_free(r2Bignum);				/* @2 */
    TPM_BN_free(s12Bignum);				/* @3 */
    TPM_BN_free(s12sBignum);				/* @4 */
    TPM_BN_free(cBignum);				/* @5 */
    TPM_BN_free(v0Bignum);				/* @6 */
    TPM_DAASensitive_Delete(&tpm_daa_sensitive);	/* @7 */
    return rc;
}

TPM_RESULT TPM_DAASign_Stage15(tpm_state_t *tpm_state,
			       TPM_DAA_SESSION_DATA *tpm_daa_session_data,
			       TPM_SIZED_BUFFER *outputData,
			       TPM_SIZED_BUFFER *inputData0)
{
    TPM_RESULT		rc = 0;
    unsigned char	*r4 = NULL;		/* freed @1 */
    TPM_BIGNUM		r4Bignum = NULL;	/* freed @2 */
    TPM_BIGNUM		s3Bignum = NULL;	/* freed @3 */
    TPM_BIGNUM		cBignum = NULL;		/* freed @4 */
    TPM_BIGNUM		v1Bignum = NULL;	/* freed @5 */
    TPM_BIGNUM		s12Bignum = NULL;	/* freed @6 */
    TPM_DAA_SENSITIVE	tpm_daa_sensitive;

    printf("TPM_DAASign_Stage15:\n");
    TPM_DAASensitive_Init(&tpm_daa_sensitive);	/* freed @7 */
    /* a. Verify that DAA_session ->DAA_stage==15. Return TPM_DAA_STAGE and flush handle on
       mismatch */
    /* NOTE Done by common code */
    /* b. Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return
       error TPM_DAA_ISSUER_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* c. Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific) and return error
       TPM_DAA_TPM_SETTINGS on mismatch */
    /* NOTE Done by common code */
    /* d. Set DAA_private_v1 = unwrap(inputData0) using TPM_PERMANENT_DATA -> daaBlobKey */
    if (rc == 0) {
	rc = TPM_ComputeDecrypt(&tpm_daa_sensitive,	/* output */
				tpm_state,		/* decryption and HMAC keys */
				inputData0,		/* encrypted stream */
				TPM_RT_DAA_V1);		/* resourceType expected */
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* e. Verify that SHA-1(DAA_private_v1) == DAA_tpmSpecific -> DAA_digest_v1 and return error
       TPM_DAA_INPUT_DATA0 on mismatch */
    if (rc == 0) {
	printf("TPM_DAASign_Stage15: Checking v1\n");
	rc = TPM_SHA1_SizedBufferCheck(tpm_daa_session_data->DAA_tpmSpecific.DAA_digest_v1,
				       &(tpm_daa_sensitive.internalData),
				       DAA_SIZE_v1);
	if (rc != 0) {
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* f. Obtain DAA_SIZE_r4 bytes from the MGF1 function and label them r4.  "r4" || DAA_session ->
       DAA_contextSeed is the Z seed. */
    if (rc == 0) {
	printf("TPM_DAASign_Stage15: Creating r4\n");
	rc = TPM_MGF1_GenerateArray(&r4,			/* returned MGF1 array */
				    DAA_SIZE_r4,		/* size of Y */
				    /* length of the entire seed */
				    sizeof("r4") -1 +
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    sizeof("r4") -1, "r4",
				    sizeof(tpm_daa_session_data->DAA_session.DAA_contextSeed),
				    tpm_daa_session_data->DAA_session.DAA_contextSeed,
				    0, NULL);
    }
    if (rc == 0) {
	rc = TPM_bin2bn(&r4Bignum, r4, DAA_SIZE_r4);
    }
    /* g. Set s3 = r4 + (DAA_session -> DAA_digest)*(DAA_private_v1) + (DAA_session ->
       DAA_scratch). */
    /* FIXME Set c = DAA_session -> DAA_digest */
    if (rc == 0) {
	printf("TPM_DAASign_Stage15: Creating c from DAA_session -> DAA_digest\n");
	rc = TPM_bin2bn(&cBignum, tpm_daa_session_data->DAA_session.DAA_digest, TPM_DIGEST_SIZE);
    }
    /* FIXME Set v1 = DAA_private_v1 */
    if (rc == 0) {
	rc = TPM_bin2bn(&v1Bignum,
			tpm_daa_sensitive.internalData.buffer,
			tpm_daa_sensitive.internalData.size);
    }
    /* FIXME Set s12 = DAA_session -> DAA_scratch */
    if (rc == 0) {
	printf("TPM_DAASign_Stage15: Creating s12 from DAA_session -> DAA_scratch\n");
	rc = TPM_bin2bn(&s12Bignum,
			tpm_daa_session_data->DAA_session.DAA_scratch,
			sizeof(tpm_daa_session_data->DAA_session.DAA_scratch));
    }
    /* s3 = r4 + c * v1 + s12 */
    if (rc == 0) {
	rc = TPM_ComputeApBxCpD(&s3Bignum,	/* freed by caller */
				r4Bignum,	/* A */
				cBignum,	/* B */
				v1Bignum,	/* C */
				s12Bignum);	/* D */
    }
    /* h. Set DAA_session -> DAA_scratch = NULL */
    if (rc == 0) {
	tpm_daa_session_data->DAA_session.DAA_scratch_null = TRUE;
    }
    /* i. set outputData = s3 */
    if (rc == 0) {
	rc = TPM_bn2binMalloc(&(outputData->buffer),
			      &(outputData->size),
			      s3Bignum, 0);
    }
    /* j. Terminate the DAA session and all resources associated with the DAA sign session
       handle. */
    /* NOTE Done by caller */
    /* k. return TPM_SUCCESS */
    free(r4);						/* @1 */
    TPM_BN_free(r4Bignum);				/* @2 */
    TPM_BN_free(s3Bignum);				/* @3 */
    TPM_BN_free(cBignum);				/* @4 */
    TPM_BN_free(v1Bignum);				/* @5 */
    TPM_BN_free(s12Bignum);				/* @6 */
    TPM_DAASensitive_Delete(&tpm_daa_sensitive);	/* @7 */
    return rc;
}

/*
  Stage Common Code
*/

/* TPM_DAADigestContext_GenerateDigestJoin() sets tpm_digest to SHA-1(DAA_tpmSpecific ||
   DAA_joinSession))
*/

TPM_RESULT TPM_DAADigestContext_GenerateDigestJoin(TPM_DIGEST tpm_digest,
						   TPM_DAA_SESSION_DATA *tpm_daa_session_data)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	sbuffer;	/* TPM_STORED_DATA serialization */
    
    printf(" TPM_DAADigestContext_GenerateDigestJoin:\n");
    TPM_Sbuffer_Init(&sbuffer);			/* freed @1 */
    /* serialize DAA_tpmSpecific */
    if (rc == 0) {
	rc = TPM_DAATpm_Store(&sbuffer, &(tpm_daa_session_data->DAA_tpmSpecific));
    }
    /* serialize DAA_joinSession */
    if (rc == 0) {
	rc = TPM_DAAJoindata_Store(&sbuffer, &(tpm_daa_session_data->DAA_joinSession));
    }
    /* calculate and return the digest */
    if (rc == 0) {
	rc = TPM_SHA1Sbuffer(tpm_digest, &sbuffer);
    }
    TPM_Sbuffer_Delete(&sbuffer);	/* @1 */
    return rc;
}

/* TPM_DAADigestContext_CheckDigestJoin() verifies that DAA_session -> DAA_digestContext ==
   SHA-1(DAA_tpmSpecific || DAA_joinSession).

   Returns TPM_DAA_TPM_SETTINGS on mismatch
*/

TPM_RESULT TPM_DAADigestContext_CheckDigestJoin(TPM_DAA_SESSION_DATA *tpm_daa_session_data)
{
    TPM_RESULT		rc = 0;
    TPM_DIGEST		tpm_digest;	/* actual digest */
    
    printf(" TPM_DAADigestContext_CheckDigestJoin:\n");
    if (rc == 0) {
	rc = TPM_DAADigestContext_GenerateDigestJoin(tpm_digest, tpm_daa_session_data);
    }
    if (rc == 0) {
	rc = TPM_Digest_Compare(tpm_digest, tpm_daa_session_data->DAA_session.DAA_digestContext);
	if (rc != 0) {
	    rc = TPM_DAA_TPM_SETTINGS;
	}
    }
    return rc;
}

/* TPM_ComputeF() computes the value F common to stages 4.j., 5.j., 14.f., 17.e., 18.e.

   j. Set f = SHA1(DAA_tpmSpecific -> DAA_rekey || DAA_tpmSpecific -> DAA_count || 0) ||
   SHA1(DAA_tpmSpecific -> DAA_rekey || DAA_tpmSpecific -> DAA_count || 1 )
   mod DAA_issuerSettings -> DAA_generic_q
*/

TPM_RESULT TPM_ComputeF(TPM_BIGNUM *fBignum,		/* freed by caller */
			TPM_DAA_SESSION_DATA *tpm_daa_session_data)
{
    TPM_RESULT		rc = 0;
    BYTE		nZero = 0;
    BYTE		nOne = 1;
    uint32_t		nCount;		/* DAA_count in nbo */
    TPM_DIGEST		digest0;	/* first SHA1 calculation */
    TPM_DIGEST		digest1;	/* second SHA1 calculation */
    TPM_BIGNUM		dividend;	/* digest0 || digest1 as a BIGNUM */
    TPM_BIGNUM		modulus;	/* DAA_generic_q as a BIGNUM */
    
    printf(" TPM_ComputeF:\n");
    modulus = NULL;			/* freed @1 */
    dividend = NULL;			/* freed @2 */
    if (rc == 0) {
	rc = TPM_BN_new(fBignum);
    }
    /* SHA1(DAA_tpmSpecific -> DAA_rekey || DAA_tpmSpecific -> DAA_count || 0) */
    if (rc == 0) {
	printf("  TPM_ComputeF: Calculate digest0\n");
	nCount = htonl(tpm_daa_session_data->DAA_tpmSpecific.DAA_count);
	rc = TPM_SHA1(digest0,
		      TPM_DIGEST_SIZE, tpm_daa_session_data->DAA_tpmSpecific.DAA_rekey,
		      sizeof(uint32_t), &nCount,
		      sizeof(BYTE), &nZero,
		      0, NULL);
    }
    /* SHA1(DAA_tpmSpecific -> DAA_rekey || DAA_tpmSpecific -> DAA_count || 1 ) */
    if (rc == 0) {
	printf("  TPM_ComputeF: Calculate digest1\n");
	rc = TPM_SHA1(digest1,
		      TPM_DIGEST_SIZE, tpm_daa_session_data->DAA_tpmSpecific.DAA_rekey,
		      sizeof(uint32_t), &nCount,
		      sizeof(BYTE), &nOne,
		      0, NULL);
    }
    /* Construct digest0 || digest1 as a positive BIGNUM */
    if (rc == 0) {
	rc = TPM_2bin2bn(&dividend,
			 digest0, TPM_DIGEST_SIZE,
			 digest1, TPM_DIGEST_SIZE);
    }	
    /* DAA_generic_q as a positive BIGNUM */
    if (rc == 0) {
	rc = TPM_bin2bn(&modulus,
			tpm_daa_session_data->DAA_issuerSettings.DAA_generic_q,
			sizeof(tpm_daa_session_data->DAA_issuerSettings.DAA_generic_q));
    }
    /* digest mod DAA_generic_q */
    if (rc == 0) {
	rc = TPM_BN_mod(*fBignum, dividend, modulus);
    }	
    TPM_BN_free(modulus);	/* @1 */
    TPM_BN_free(dividend);	/* @2 */
    return rc;
}

/* TPM_ComputeAexpPmodn() performs R = (A ^ P) mod n.

   rBignum is new'ed by this function and must be freed by the caller

   If DAA_scratch is not NULL, r is returned in DAA_scratch.
*/

TPM_RESULT TPM_ComputeAexpPmodn(BYTE *DAA_scratch,
				uint32_t DAA_scratch_size,
				TPM_BIGNUM *rBignum,	/* freed by caller */
				TPM_BIGNUM aBignum,
				TPM_BIGNUM pBignum,
				TPM_BIGNUM nBignum)
{
    TPM_RESULT	rc = 0;
    
    printf(" TPM_ComputeAexpPmodn:\n");
    if (rc == 0) {
	rc = TPM_BN_new(rBignum);
    }
    if (rc == 0) {
	rc = TPM_BN_mod_exp(*rBignum, aBignum, pBignum, nBignum);
    }
    /* if the result should be returned in DAA_scratch */
    if ((rc == 0) && (DAA_scratch != NULL)) {
	/* store the result in DAA_scratch */
	rc = TPM_ComputeDAAScratch(DAA_scratch, DAA_scratch_size, *rBignum);
    }
    return rc;
}

/* TPM_ComputeZxAexpPmodn() performs DAA_scratch = Z * (A ^ P) mod n.

*/

TPM_RESULT TPM_ComputeZxAexpPmodn(BYTE *DAA_scratch,
				  uint32_t DAA_scratch_size,
				  TPM_BIGNUM zBignum,
				  TPM_BIGNUM aBignum,
				  TPM_BIGNUM pBignum,
				  TPM_BIGNUM nBignum)
{
    TPM_RESULT	rc = 0;
    TPM_BIGNUM	rBignum = NULL;		/* freed @1 */
    
    printf(" TPM_ComputeZxAexpPmodn:\n");
    if (rc == 0) {
	printf("  TPM_ComputeZxAexpPmodn: Calculate R = A ^ P mod n\n");
	rc = TPM_ComputeAexpPmodn(NULL,		/* DAA_scratch */
				  0,
				  &rBignum,	/* R */
				  aBignum,	/* A */
				  pBignum,
				  nBignum);
    }
    if (rc == 0) {
	printf("  TPM_ComputeZxAexpPmodn: Calculate R = Z * R mod n\n");
	rc = TPM_BN_mod_mul(rBignum, zBignum, rBignum, nBignum);
    }
    /* store the result in DAA_scratch */
    if (rc == 0) {
	rc = TPM_ComputeDAAScratch(DAA_scratch, DAA_scratch_size, rBignum);
    }
    TPM_BN_free(rBignum);	/* @1 */
    return rc;
}

/* TPM_ComputeApBmodn() performs R = A + B mod n

*/

TPM_RESULT TPM_ComputeApBmodn(TPM_BIGNUM *rBignum, /* freed by caller */
			      TPM_BIGNUM aBignum,
			      TPM_BIGNUM bBignum,
			      TPM_BIGNUM nBignum)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_ComputeApBmodn:\n");
    if (rc == 0) {
	rc = TPM_BN_new(rBignum);	/* freed by caller */
    }
    if (rc == 0) {
	rc = TPM_BN_mod_add(*rBignum, aBignum, bBignum, nBignum); 
    }
    return rc;
}

/* TPM_ComputeApBxC() performs R = A + B * C

*/

TPM_RESULT TPM_ComputeApBxC(TPM_BIGNUM *rBignum,	/* freed by caller */
			    TPM_BIGNUM aBignum,
			    TPM_BIGNUM bBignum,
			    TPM_BIGNUM cBignum)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_ComputeApBxC:\n");
    if (rc == 0) {
	rc = TPM_BN_new(rBignum);	/* freed by caller */
    }
    /* R = B * C */
    if (rc == 0) {
	rc = TPM_BN_mul(*rBignum, bBignum, cBignum); 
    }
    /* R = R + A */
    if (rc == 0) {
	rc = TPM_BN_add(*rBignum, *rBignum, aBignum);
    }
    return rc;
}

/* TPM_ComputeApBxCpD() performs R = A + B * C + D

*/

TPM_RESULT TPM_ComputeApBxCpD(TPM_BIGNUM *rBignum, /* freed by caller */
			      TPM_BIGNUM aBignum,
			      TPM_BIGNUM bBignum,
			      TPM_BIGNUM cBignum,
			      TPM_BIGNUM dBignum)
{
    TPM_RESULT		rc = 0;
    printf(" TPM_ComputeApBxCpD:\n");
    /* R = A + B * C */
    if (rc == 0) {
	rc = TPM_ComputeApBxC(rBignum,	/* freed by caller */
			      aBignum,
			      bBignum,
			      cBignum);
    }
    /* R = R + D */
    if (rc == 0) {
	rc = TPM_BN_add(*rBignum, *rBignum, dBignum);
    }
    return rc;
}

/* TPM_ComputeDAAScratch() stores 'bn' in DAA_scratch

*/

TPM_RESULT TPM_ComputeDAAScratch(BYTE *DAA_scratch,
				 uint32_t DAA_scratch_size,
				 TPM_BIGNUM bn)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_ComputeDAAScratch:\n");
    if (rc == 0) {
	rc = TPM_bn2binArray(DAA_scratch, DAA_scratch_size, bn);
    }
    return rc;
}

/* TPM_ComputeEnlarge() creates a buffer of size 'outSize'

   It copies 'outSize - inSize' zero bytes and then appends 'in'

   'out' must be freed by the caller
*/

TPM_RESULT TPM_ComputeEnlarge(unsigned char **out,	/* freed by caller */
			      uint32_t outSize,
			      unsigned char *in,
			      uint32_t inSize)
{
    TPM_RESULT		rc = 0;

    if (rc == 0) {
	if (outSize <= inSize) {
	    printf("TPM_ComputeEnlarge: Error (fatal), inSize %u outSize %u\n", inSize, outSize);
	    rc = TPM_FAIL;
	}
    }
    if (rc == 0) {
	rc = TPM_Malloc(out, outSize);
    }
    if (rc == 0) {
	memset(*out, 0, outSize - inSize);		/* zero left bytes */
	memcpy(*out + outSize - inSize, in, inSize);	/* copy right bytes */
    }
    return rc;
}

/* TPM_SizedBuffer_ComputeEnlarge() forces 'tpm_sized_buffer' to be 'size' bytes in length.

   If generally useful, this function should be moved to tpm_sizedbuffer.c
*/

TPM_RESULT TPM_SizedBuffer_ComputeEnlarge(TPM_SIZED_BUFFER *tpm_sized_buffer, uint32_t size)
{
    TPM_RESULT		rc = 0;
    unsigned char	*newPtr;	/* new buffer, enlarged */
    
    newPtr = NULL;	/* freed by caller */
    /* if tpm_sized_buffer needs to be enlarged */
    if (tpm_sized_buffer->size != size) {
	if (rc == 0) {
	    /* copy the TPM_SIZED_BUFFER data. enlarged, to newPtr */
	    rc = TPM_ComputeEnlarge(&newPtr, size,	/* output buffer */
				    tpm_sized_buffer->buffer,
				    tpm_sized_buffer->size);
	}
	if (rc == 0) {
	    /* after the copy, the old buffer is no longer needed */
	    free(tpm_sized_buffer->buffer);
	    /* assign the with the enlarged buffer to the TPM_SIZED_BUFFER */
	    tpm_sized_buffer->buffer = newPtr;
	    /* update size */
	    tpm_sized_buffer->size = size;
	}
    }
    return rc;
}

/* TPM_ComputeEncrypt() does join steps common to encrypting output data.

   It serializes the TPM_DAA_SENSITIVE, encrypts it to TPM_DAA_BLOB ->sensitiveData, adds the
   resourceType, generates the TPM_DAA_BLOB -> blobIntegrity HMAC using daaProof, and serializes the
   result to outputData.
*/

TPM_RESULT TPM_ComputeEncrypt(TPM_SIZED_BUFFER *outputData,
			      tpm_state_t *tpm_state,
			      TPM_DAA_SENSITIVE *tpm_daa_sensitive,
			      TPM_RESOURCE_TYPE resourceType)
{
    TPM_RESULT		rc = 0;
    TPM_DAA_BLOB	tpm_daa_blob;
    TPM_STORE_BUFFER	daaSensitiveSbuffer;
    
    printf(" TPM_ComputeEncrypt:\n");
    TPM_DAABlob_Init(&tpm_daa_blob);		/* freed @1 */
    TPM_Sbuffer_Init(&daaSensitiveSbuffer);	/* freed @2 */

    /* serialize the TPM_DAA_SENSITIVE */
    if (rc == 0) {
	rc = TPM_DAASensitive_Store(&daaSensitiveSbuffer, tpm_daa_sensitive);
    }
    /* Create a TPM_DAA_BLOB structure */
    if (rc == 0) {
	printf("  TPM_ComputeEncrypt: Create TPM_DAA_BLOB\n");
	tpm_daa_blob.resourceType = resourceType;
	/* Set TPM_DAA_BLOB -> sensitiveData to the encryption of serialized TPM_DAA_SENSITIVE */
	rc = TPM_SymmetricKeyData_EncryptSbuffer
	     (&(tpm_daa_blob.sensitiveData),			/* output buffer */
	      &daaSensitiveSbuffer,				/* input buffer */
	      tpm_state->tpm_permanent_data.daaBlobKey);	/* key */
    }
    /* set TPM_DAA_BLOB -> blobIntegrity to the HMAC of TPM_DAA_BLOB using daaProof as the secret */
    if (rc == 0) {
	rc = TPM_HMAC_GenerateStructure(tpm_daa_blob.blobIntegrity,		/* HMAC */
					tpm_state->tpm_permanent_data.daaProof, /* HMAC key */
					&tpm_daa_blob,				/* structure */
					(TPM_STORE_FUNCTION_T)TPM_DAABlob_Store); /* store
										     function */
    }
    /* ii. set outputData to the encrypted TPM_DAA_BLOB */
    if (rc == 0) {
	rc = TPM_SizedBuffer_SetStructure(outputData, &tpm_daa_blob,
					  (TPM_STORE_FUNCTION_T )TPM_DAABlob_Store);
    }
    TPM_DAABlob_Delete(&tpm_daa_blob);			/* @1 */
    TPM_Sbuffer_Delete(&daaSensitiveSbuffer);		/* @2 */
    return rc;
}

/* TPM_ComputeDecrypt() does sign steps common to decrypting input data

   It deserializes 'inputData" to a TPM_DAA_BLOB, and validates the resourceType and blobIntegrity
   HMAC using daaProof.	 It decrypts TPM_DAA_BLOB ->sensitiveData and deserializes it to a
   TPM_DAA_SENSITIVE.

   tpm_daa_sensitive must be deleted by the caller
*/

TPM_RESULT TPM_ComputeDecrypt(TPM_DAA_SENSITIVE *tpm_daa_sensitive,
			      tpm_state_t *tpm_state,
			      TPM_SIZED_BUFFER *inputData,
			      TPM_RESOURCE_TYPE resourceType)

{
    TPM_RESULT		rc = 0;
    TPM_DAA_BLOB	tpm_daa_blob;
    unsigned char	*stream;
    uint32_t		stream_size;
    unsigned char	*sensitiveStream;
    uint32_t		sensitiveStreamSize;
    
    printf(" TPM_ComputeDecrypt:\n");
    TPM_DAABlob_Init(&tpm_daa_blob);		/* freed @1 */
    sensitiveStream = NULL;			/* freed @2 */
    /* deserialize inputData to a TPM_DAA_BLOB */
    if (rc == 0) {
	stream = inputData->buffer;
	stream_size = inputData->size;
	rc = TPM_DAABlob_Load(&tpm_daa_blob, &stream, &stream_size);
    }
    if (rc == 0) {
	if (stream_size != 0) {
	    printf("TPM_ComputeDecrypt: Error, bad blob input size %u\n", inputData->size);
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* check blobIntegrity */
    if (rc == 0) {
	rc = TPM_HMAC_CheckStructure(tpm_state->tpm_permanent_data.daaProof,	/* HMAC key */
				     &tpm_daa_blob,				/* structure */
				     tpm_daa_blob.blobIntegrity,		/* expected */
				     (TPM_STORE_FUNCTION_T)TPM_DAABlob_Store,	/* store function */
				     TPM_DAA_INPUT_DATA0);		/* error code */
    }
    /* check resourceType */
    if (rc == 0) {
	if (tpm_daa_blob.resourceType != resourceType) {
	    printf("TPM_ComputeDecrypt: Error, resourceType %08x\n", tpm_daa_blob.resourceType);
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    /* decrypt the TPM_DAA_BLOB -> sensitiveData */
    if (rc == 0) {
	rc = TPM_SymmetricKeyData_Decrypt
	     (&sensitiveStream,					/* output, caller frees */
	      &sensitiveStreamSize,				/* output */
	      tpm_daa_blob.sensitiveData.buffer,		/* input */
	      tpm_daa_blob.sensitiveData.size,			/* input */
	      tpm_state->tpm_permanent_data.daaBlobKey);	/* dec key */
    }
    if (rc == 0) {
	stream = sensitiveStream;
	stream_size = sensitiveStreamSize;
	rc = TPM_DAASensitive_Load(tpm_daa_sensitive, &stream, &stream_size);
    }
    if (rc == 0) {
	if (stream_size != 0) {
	    printf("TPM_ComputeDecrypt: Error, bad sensitive input size %u\n", sensitiveStreamSize);
	    rc = TPM_DAA_INPUT_DATA0;
	}
    }
    TPM_DAABlob_Delete(&tpm_daa_blob);			/* @1 */
    free(sensitiveStream);				/* @2 */
    return rc;
}

/* TPM_SHA1_BignumGenerate() converts the BIGNUM 'bn' to an array, enlarges the array to 'size', and
   computes the SHA-1 hash
   
*/

TPM_RESULT TPM_SHA1_BignumGenerate(TPM_DIGEST tpm_digest,
				   TPM_BIGNUM bn,
				   uint32_t size)
{
    TPM_RESULT	rc = 0;
    unsigned char *bin = NULL;		/* freed @1 */
    unsigned int bytes;
    unsigned char *newBin = NULL;	/* freed @2, new buffer, enlarged */

    if (rc == 0) {
	rc = TPM_bn2binMalloc(&bin, &bytes, bn, 0);	/* freed @1 */
    }
    if (rc == 0) {
	printf(" TPM_SHA1_BignumGenerate: enlarge to %u bytes, is %u bytes\n", size, bytes);
	if (bytes != size) {
	    /* canonicalize the array size */
	    if (rc == 0) {
		rc = TPM_ComputeEnlarge(&newBin, size,	/* output buffer */
					bin, bytes );	/* inout buffer */
	    }
	    if (rc == 0) {
		rc = TPM_SHA1(tpm_digest,
			      size, newBin,
			      0, NULL);
	    }
	}
	else {
	    /* already canonicalized */
	    rc = TPM_SHA1(tpm_digest,
			  bytes, bin,
			  0, NULL);
	}
    }
    free(bin);		/* @1 */
    free(newBin);	/* @2 */
    return rc;
}

/* TPM_SHA1_SizedBufferCheck() enlarges the TPM_SIZED_BUFFER to 'size', computes the SHA-1 hash,
   and validates the digest against 'tpm_digest'

   As a side effect, the TPM_SIZED_BUFFER may be enlarged.
*/

TPM_RESULT TPM_SHA1_SizedBufferCheck(TPM_DIGEST tpm_digest,
				     TPM_SIZED_BUFFER *tpm_sized_buffer,
				     uint32_t size)
{
    TPM_RESULT	rc = 0;

    if (rc == 0) {
	printf(" TPM_SHA1_SizedBufferCheck: enlarge to %u bytes, is %u bytes\n",
	       size, tpm_sized_buffer->size);
	if (tpm_sized_buffer->size != size) {
	    /* canonicalize the array size */
	    rc = TPM_SizedBuffer_ComputeEnlarge(tpm_sized_buffer, size);
	}
    }
    if (rc == 0) {
	rc = TPM_SHA1_Check(tpm_digest,
			    tpm_sized_buffer->size, tpm_sized_buffer->buffer,
			    0, NULL);
    }
    return rc;
}

/*
  Processing functions
*/

/* 26.1 TPM_DAA_Join rev 99

   TPM_DAA_Join is the process that establishes the DAA parameters in the TPM for a specific DAA
   issuing authority.

   outputSize and outputData are always included in the outParamDigest.	 This includes stage 
   0, where the outputData contains the DAA session handle.
*/

TPM_RESULT TPM_Process_DAAJoin(tpm_state_t *tpm_state,
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
    TPM_HANDLE		daaHandle;	/* Session handle */
    BYTE		stage = 0;	/* Processing stage of join */
    TPM_SIZED_BUFFER	inputData0;	/* Data to be used by this capability */
    TPM_SIZED_BUFFER	inputData1;	/* Data to be used by this capability */
    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle used for owner
					   authentication */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* Continue use flag, TRUE if handle is still
						   active */
    TPM_AUTHDATA	ownerAuth;	/* The authorization session digest for inputs and
					   owner. HMAC key: ownerAuth. */

    /* processing */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_BOOL			daaHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*auth_session_data;		/* session data for authHandle */
    TPM_SECRET			*hmacKey;
    TPM_DAA_SESSION_DATA	*tpm_daa_session_data;		/* DAA session for handle */
    
    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_SIZED_BUFFER	outputData;	/* Data produced by this capability */

    printf("TPM_Process_DAAJoin: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&inputData0);	/* freed @1 */
    TPM_SizedBuffer_Init(&inputData1);	/* freed @2 */
    TPM_SizedBuffer_Init(&outputData);	/* freed @3 */
    /*
      get inputs
    */
    /* get handle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&daaHandle, &command, &paramSize);
    }	
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get stage */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DAAJoin: daaHandle %08x\n", daaHandle);
	returnCode = TPM_Load8(&stage, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DAAJoin: stage %u\n", stage);
	/* For stages after stage 0, daaHandle is an input.  Mark it valid so it can be terminated
	   on error. */
	if (stage > 0) {
	    daaHandleValid = TRUE;
	}
	/* get inputData0 */
	returnCode = TPM_SizedBuffer_Load(&inputData0, &command, &paramSize);
    }
    /* get inputData1 */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&inputData1, &command, &paramSize);
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
	    printf("TPM_Process_DAAJoin: Error, command has %u extra bytes\n",
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
    /* 1. Use ownerAuth to verify that the Owner authorized all TPM_DAA_Join input parameters. */
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
    /*
      Common to most or all stages
    */
    /* Validate the DAA session handle after stage 0, stage 0 assigns the handle */
    if (returnCode == TPM_SUCCESS) {
	if (stage > 0) {
	    returnCode =
		TPM_DaaSessions_GetEntry(&tpm_daa_session_data,		/* returns entry in array */
					 tpm_state->tpm_stclear_data.daaSessions, /* array */
					 daaHandle);
	}
    }
    /* Verify that the input state is consistent with the current TPM state */
    if (returnCode == TPM_SUCCESS) {
	if (stage > 0) {
	    returnCode = TPM_DaaSessionData_CheckStage(tpm_daa_session_data, stage);
	}
    }
    /* Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific || DAA_joinSession) and
       return error TPM_DAA_TPM_SETTINGS on mismatch */
    if (returnCode == TPM_SUCCESS) {
	if (stage >= 1) {
	    returnCode = TPM_DAADigestContext_CheckDigestJoin(tpm_daa_session_data);
	}
    }
    /* Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return error
       TPM_DAA_ISSUER_SETTINGS on mismatch */
    if (returnCode == TPM_SUCCESS) {
	if (stage >= 3) {
	    returnCode =
		TPM_SHA1_CheckStructure(tpm_daa_session_data->DAA_tpmSpecific.DAA_digestIssuer,
					&(tpm_daa_session_data->DAA_issuerSettings),
					(TPM_STORE_FUNCTION_T)TPM_DAAIssuer_Store,
					TPM_DAA_ISSUER_SETTINGS);
	}
    }
    /* Stages */
    if (returnCode == TPM_SUCCESS) {
	switch (stage) {
	  case 0 :
	    returnCode = TPM_DAAJoin_Stage00(tpm_state,
					     &tpm_daa_session_data,	/* entry in array */
					     &daaHandleValid,
					     &outputData, &inputData0);
	    if (daaHandleValid) {
		/* For stage 0, daaHandle may be generated.  Extract it from the DAA session and
		   mark it valid, so the session can be terminated on error. */
		daaHandle = tpm_daa_session_data->daaHandle;
	    }
	    break;
	  case 1 :
	    returnCode = TPM_DAAJoin_Stage01(tpm_state,
					     tpm_daa_session_data,
					     &outputData, &inputData0, &inputData1);
	    break;
	  case 2 :
	    returnCode = TPM_DAAJoin_Stage02(tpm_state,
					     tpm_daa_session_data,
					     &outputData, &inputData0, &inputData1);
	    break;
	  case 3 :
	    returnCode = TPM_DAAJoin_Stage03(tpm_state,
					     tpm_daa_session_data,
					     &outputData, &inputData0);
	    break;
	  case 4 :
	    returnCode = TPM_DAAJoin_Stage04(tpm_state,
					     tpm_daa_session_data,
					     &outputData, &inputData0, &inputData1);
	    break;
	  case 5 :
	    returnCode = TPM_DAAJoin_Stage05(tpm_state,
					     tpm_daa_session_data,
					     &outputData, &inputData0, &inputData1);
	    break;
	  case 6 :
	    returnCode = TPM_DAAJoin_Stage06(tpm_state,
					     tpm_daa_session_data,
					     &outputData, &inputData0, &inputData1);
	    break;
	  case 7 :
	    returnCode = TPM_DAAJoin_Stage07(tpm_state,
					     tpm_daa_session_data,
					     &outputData, &inputData0, &inputData1);
	    break;
	  case 8 :
	    returnCode = TPM_DAAJoin_Stage08(tpm_state,
					     tpm_daa_session_data,
					     &outputData, &inputData0);
	    break;
	  case 9 :
	    returnCode = TPM_DAAJoin_Stage09_Sign_Stage2(tpm_state,
							 tpm_daa_session_data,
							 &outputData, &inputData0, &inputData1);
	    break;
	  case 10 :
	    returnCode = TPM_DAAJoin_Stage10_Sign_Stage3(tpm_state,
							 tpm_daa_session_data,
							 &outputData, &inputData0, &inputData1);
	    break;
	  case 11 :
	    returnCode = TPM_DAAJoin_Stage11_Sign_Stage4(tpm_state,
							 tpm_daa_session_data,
							 &outputData, &inputData0, &inputData1);
	    break;
	  case 12 :
	    returnCode = TPM_DAAJoin_Stage12(tpm_state,
					     tpm_daa_session_data,
					     &outputData, &inputData0, &inputData1);
	    break;
	  case 13 :
	    returnCode = TPM_DAAJoin_Stage13_Sign_Stage6(tpm_state,
							 tpm_daa_session_data,
							 &outputData, &inputData0, &inputData1);
	    break;
	  case 14 :
	    returnCode = TPM_DAAJoin_Stage14_Sign_Stage7(tpm_state,
							 tpm_daa_session_data,
							 &outputData, &inputData0);
	    break;
	  case 15 :
	    returnCode = TPM_DAAJoin_Stage15_Sign_Stage8(tpm_state,
							 tpm_daa_session_data,
							 &outputData, &inputData0);
	    break;
	  case 16 :
	    returnCode = TPM_DAAJoin_Stage16_Sign_Stage9(tpm_state,
							 tpm_daa_session_data,
							 &outputData, &inputData0);
	    break;
	  case 17 :
	    returnCode = TPM_DAAJoin_Stage17_Sign_Stage11(tpm_state,
							  tpm_daa_session_data,
							  &outputData);
	    break;
	  case 18 :
	    returnCode = TPM_DAAJoin_Stage18_Sign_Stage12(tpm_state,
							  tpm_daa_session_data,
							  &outputData);
	    break;
	  case 19 :
	    returnCode = TPM_DAAJoin_Stage19(tpm_state,
					     tpm_daa_session_data,
					     &outputData);
	    break;
	  case 20 :
	    returnCode = TPM_DAAJoin_Stage20(tpm_state,
					     tpm_daa_session_data,
					     &outputData);
	    break;
	  case 21 :
	    returnCode = TPM_DAAJoin_Stage21(tpm_state,
					     tpm_daa_session_data,
					     &outputData);
	    break;
	  case 22 :
	    returnCode = TPM_DAAJoin_Stage22(tpm_state,
					     tpm_daa_session_data,
					     &outputData, &inputData0);
	    break;
	  case 23 :
	    returnCode = TPM_DAAJoin_Stage23(tpm_state,
					     tpm_daa_session_data,
					     &outputData,
					     &inputData0);
	    break;
	  case 24 :
	    returnCode = TPM_DAAJoin_Stage24(tpm_state,
					     tpm_daa_session_data,
					     &outputData);
	    break;
	  default :
	    printf("TPM_Process_DAAJoin: Error, Illegal stage\n");
	    returnCode = TPM_DAA_STAGE;
	}
    }
    /*
      Common to most or all stages
    */
    if (returnCode == TPM_SUCCESS) {
	if (stage >= 2) {
	    tpm_daa_session_data->DAA_session.DAA_stage++;
	}
    }
    /* 24.e.Terminate the DAA session and all resources associated with the DAA join session
	  handle. */
    if (returnCode == TPM_SUCCESS) {
	if (stage == 24) {
	    printf("TPM_Process_DAAJoin: Stage 24, terminating DAA session %08x\n",
		   tpm_daa_session_data->daaHandle);
	    TPM_DaaSessionData_Delete(tpm_daa_session_data);
	}
    }
    /* 2. Any error return results in the TPM invalidating all resources associated with the
       join */
    /* NOTE Done after response processing */
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_DAAJoin: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return outputData */
	    returnCode = TPM_SizedBuffer_Store(response, &outputData);
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
    /* if there was an error, terminate the session.  */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /* on error, terminate the DAA session */
    if (((rcf != 0) || (returnCode != TPM_SUCCESS)) && daaHandleValid) {
	TPM_DaaSessions_TerminateHandle(tpm_state->tpm_stclear_data.daaSessions,
					daaHandle);
    }
    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&inputData0);	/* @1 */
    TPM_SizedBuffer_Delete(&inputData1);	/* @2 */
    TPM_SizedBuffer_Delete(&outputData);	/* @3 */
    return rcf;
}

/* 26.2 TPM_DAA_Sign rev 99
   
   TPM protected capability; user must provide authorizations from the TPM Owner.

   outputSize and outputData are always included in the outParamDigest.	 This includes stage 
   0, where the outputData contains the DAA session handle.
*/

TPM_RESULT TPM_Process_DAASign(tpm_state_t *tpm_state,
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
    TPM_HANDLE		daaHandle;	/* Handle to the sign session */
    BYTE		stage = 0;	/* Stage of the sign process */
    TPM_SIZED_BUFFER	inputData0;	/* Data to be used by this capability */
    TPM_SIZED_BUFFER	inputData1;	/* Data to be used by this capability */
    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle used for owner
					   authentication */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* Continue use flag, TRUE if handle is still
						   active */
    TPM_AUTHDATA	ownerAuth;	/* The authorization session digest for inputs and
					   owner. HMAC key: ownerAuth. */

    /* processing */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_BOOL			daaHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*auth_session_data;		/* session data for authHandle */
    TPM_SECRET			*hmacKey;
    TPM_DAA_SESSION_DATA	*tpm_daa_session_data;		/* DAA session for handle */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_SIZED_BUFFER	outputData;	/* Data produced by this capability */

    printf("TPM_Process_DAASign: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&inputData0);	/* freed @1 */
    TPM_SizedBuffer_Init(&inputData1);	/* freed @2 */
    TPM_SizedBuffer_Init(&outputData);	/* freed @3 */
    /*
      get inputs
    */
    /* get handle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&daaHandle, &command, &paramSize);
    }	
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get stage */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DAASign: daaHandle %08x\n", daaHandle);
	returnCode = TPM_Load8(&stage, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DAASign: stage %u\n", stage);
	/* For stages after stage 0, daaHandle is an input.  Mark it valid so it can be terminated
	   on error. */
	if (stage > 0) {
	    daaHandleValid = TRUE;
	}
	/* get inputData0 */
	returnCode = TPM_SizedBuffer_Load(&inputData0, &command, &paramSize);
    }
    /* get inputData1 */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&inputData1, &command, &paramSize);
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
	    printf("TPM_Process_DAASign: Error, command has %u extra bytes\n",
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
    /* 1. Use ownerAuth to verify that the Owner authorized all TPM_DAA_Sign input parameters. */
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
    /*
      Common to most or all stages
    */
    /* Validate the DAA session handle after stage 0, stage 0 assigns the handle */
    if (returnCode == TPM_SUCCESS) {
	if (stage > 0) {
	    returnCode =
		TPM_DaaSessions_GetEntry(&tpm_daa_session_data,		/* returns entry in array */
					 tpm_state->tpm_stclear_data.daaSessions,	/* array */
					 daaHandle);
	}
    }
    /* Verify that the input state is consistent with the current TPM state */
    if (returnCode == TPM_SUCCESS) {
	if (stage > 0) {
	    returnCode = TPM_DaaSessionData_CheckStage(tpm_daa_session_data, stage);
	}
    }
    /* Verify that DAA_session -> DAA_digestContext == SHA-1(DAA_tpmSpecific) and return error
       TPM_DAA_TPM_SETTINGS on mismatch */
    if (returnCode == TPM_SUCCESS) {
	if (stage >= 2) {
	    returnCode =
		TPM_SHA1_CheckStructure(tpm_daa_session_data->DAA_session.DAA_digestContext,
					&(tpm_daa_session_data->DAA_tpmSpecific),
					(TPM_STORE_FUNCTION_T)TPM_DAATpm_Store,
					TPM_DAA_TPM_SETTINGS);
	}
    }
    /* Verify that DAA_tpmSpecific -> DAA_digestIssuer == SHA-1(DAA_issuerSettings) and return error
       TPM_DAA_ISSUER_SETTINGS on mismatch */
    if (returnCode == TPM_SUCCESS) {
	if (stage >= 2) {
	    returnCode =
		TPM_SHA1_CheckStructure(tpm_daa_session_data->DAA_tpmSpecific.DAA_digestIssuer,
					&(tpm_daa_session_data->DAA_issuerSettings),
					(TPM_STORE_FUNCTION_T)TPM_DAAIssuer_Store,
					TPM_DAA_ISSUER_SETTINGS);
	}
    }
    /* Stages */
    if (returnCode == TPM_SUCCESS) {
	switch (stage) {
	  case 0 :
	    returnCode = TPM_DAASign_Stage00(tpm_state,
					     &tpm_daa_session_data,	/* returns entry in array */
					     &daaHandleValid,
					     &outputData, 
					     &inputData0);
	    if (daaHandleValid) {
		/* For stage 0, daaHandle may be generated.  Extract it from the DAA session and
		   mark it valid, so the session can be terminated on error. */
		daaHandle = tpm_daa_session_data->daaHandle;
	    }
	    break;
	  case 1 :
	    returnCode = TPM_DAASign_Stage01(tpm_state,
					     tpm_daa_session_data,
					     &outputData, 
					     &inputData0);
	    break;
	  case 2 :
	    returnCode = TPM_DAAJoin_Stage09_Sign_Stage2(tpm_state,
							 tpm_daa_session_data,
							 &outputData, 
							 &inputData0, &inputData1);
	    break;
	  case 3 :
	    returnCode = TPM_DAAJoin_Stage10_Sign_Stage3(tpm_state,
							 tpm_daa_session_data,
							 &outputData, 
							 &inputData0, &inputData1);
	    break;
	  case 4 :
	    returnCode = TPM_DAAJoin_Stage11_Sign_Stage4(tpm_state,
							 tpm_daa_session_data,
							 &outputData, 
							 &inputData0, &inputData1);
	    break;
	  case 5 :
	    returnCode = TPM_DAASign_Stage05(tpm_state,
					     tpm_daa_session_data,
					     &outputData, 
					     &inputData0, &inputData1);
	    break;
	  case 6 :
	    returnCode = TPM_DAAJoin_Stage13_Sign_Stage6(tpm_state,
							 tpm_daa_session_data,
							 &outputData, 
							 &inputData0, &inputData1);
	    break;
	  case 7 :
	    returnCode = TPM_DAAJoin_Stage14_Sign_Stage7(tpm_state,
							 tpm_daa_session_data,
							 &outputData, 
							 &inputData0);
	    break;
	  case 8 :
	    returnCode = TPM_DAAJoin_Stage15_Sign_Stage8(tpm_state,
							 tpm_daa_session_data,
							 &outputData, 
							 &inputData0);
	    break;
	  case 9 :
	    returnCode = TPM_DAAJoin_Stage16_Sign_Stage9(tpm_state,
							 tpm_daa_session_data,
							 &outputData, 
							 &inputData0);
	    break;
	  case 10 :
	    returnCode = TPM_DAASign_Stage10(tpm_state,
					     tpm_daa_session_data,
					     &outputData, 
					     &inputData0, &inputData1);
	    break;
	  case 11 :
	    returnCode = TPM_DAAJoin_Stage17_Sign_Stage11(tpm_state,
							  tpm_daa_session_data,
							  &outputData);
	    break;
	  case 12 :
	    returnCode = TPM_DAAJoin_Stage18_Sign_Stage12(tpm_state,
							  tpm_daa_session_data,
							  &outputData);
	    break;
	  case 13 :
	    returnCode = TPM_DAASign_Stage13(tpm_state,
					     tpm_daa_session_data,
					     &outputData, 
					     &inputData0);
	    break;
	  case 14 :
	    returnCode = TPM_DAASign_Stage14(tpm_state,
					     tpm_daa_session_data,
					     &outputData, 
					     &inputData0);
	    break;
	  case 15 :
	    returnCode = TPM_DAASign_Stage15(tpm_state,
					     tpm_daa_session_data,
					     &outputData, 
					     &inputData0);
	    break;
	  default :
	    printf("TPM_Process_DAASign: Error, Illegal stage\n");
	    returnCode = TPM_DAA_STAGE;
	}
    }
    /*
      Common to most or all stages
    */
    if (returnCode == TPM_SUCCESS) {
	tpm_daa_session_data->DAA_session.DAA_stage++;
    }
    /* 15.j. Terminate the DAA session and all resources associated with the DAA sign session
       handle. */
    if (returnCode == TPM_SUCCESS) {
	if (stage == 15) {
	    printf("TPM_Process_DAASign: Stage 15, terminating DAA session %08x\n",
		   tpm_daa_session_data->daaHandle);
	    TPM_DaaSessionData_Delete(tpm_daa_session_data);
	}
    }
    /* 2. Any error return results in the TPM invalidating all resources associated with the
       join */
    /* NOTE Done after response processing */
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_DAASign: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return outputData */
	    returnCode = TPM_SizedBuffer_Store(response, &outputData);
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
    /* if there was an error, terminate the session.  */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /* on error, terminate the DAA session */
    if (((rcf != 0) || (returnCode != TPM_SUCCESS)) && daaHandleValid) {
	TPM_DaaSessions_TerminateHandle(tpm_state->tpm_stclear_data.daaSessions,
					daaHandle);
    }
    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&inputData0);	/* @1 */
    TPM_SizedBuffer_Delete(&inputData1);	/* @2 */
    TPM_SizedBuffer_Delete(&outputData);	/* @3 */
    return rcf;
}
