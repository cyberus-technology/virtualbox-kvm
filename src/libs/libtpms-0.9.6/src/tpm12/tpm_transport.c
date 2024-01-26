/********************************************************************************/
/*										*/
/*				Transport					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_transport.c $		*/
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

#include "tpm_audit.h"
#include "tpm_auth.h"
#include "tpm_crypto.h"
#include "tpm_cryptoh.h"
#include "tpm_debug.h"
#include "tpm_digest.h"
#include "tpm_error.h"
#include "tpm_init.h"
#include "tpm_key.h"
#include "tpm_memory.h"
#include "tpm_nonce.h"
#include "tpm_process.h"
#include "tpm_secret.h"
#include "tpm_ticks.h"

#include "tpm_transport.h"

/* TPM_Transport_CryptMgf1() takes a 'src', a preallocated 'dest', and an MGF1 'pad' of length
   'len'.

   'size is the total length of 'src' and 'dest'.
   'index' is the start of the encrypt area
   'len' is the length of the encrypt area
   
   It copies 'src' to 'dest' up to 'index'.
   It then copies 'src' XOR'ed with 'pad' for 'len'
   It then copies the remainder of 'src' to 'dest'
*/

TPM_RESULT TPM_Transport_CryptMgf1(unsigned char *dest,
				   const unsigned char *src,
				   const unsigned char *pad,
				   uint32_t size,
				   uint32_t index,
				   uint32_t len)
{
    TPM_RESULT rc = 0;
    
    printf("  TPM_Transport_CryptMgf1: size %u index %u len %u\n", size, index, len); 
    /* sanity check the length */
    if (rc == 0) {
	if (index + len > size) {
	    printf("TPM_Transport_CryptMgf1: Error (fatal), bad size\n"); 
	    rc = TPM_FAIL;	/* internal error, should never occur */
	}
    }
    if (rc == 0) {
	/* leading clear text area */
	memcpy(dest, src, index);
	dest += index;
	src += index;
	/* encrypt area */
	TPM_XOR(dest, pad, src, len);
	dest += len;
	src += len;
	/* trailing clear text area */
	memcpy(dest, src, size - index - len);
    }
    return rc;
}

/* TPM_Transport_CryptSymmetric() takes a 'src', a preallocated 'dest', and a 'symmetric_key'
   'pad_in' (CTR or IV) of length 'len'.

   'size is the total length of 'src' and 'dest'.
   'index' is the start of the encrypt area
   'len' is the length of the encrypt area
   
   It copies 'src' to 'dest' up to 'index'.
   It then encrypts 'src' to 'dest' using 'symmetric_key and 'pad_in' for 'len'
   It then copies the remainder of 'src' to 'dest'
*/

TPM_RESULT TPM_Transport_CryptSymmetric(unsigned char *dest,
					const unsigned char *src,
					TPM_ALGORITHM_ID algId,			/* algorithm */
					TPM_ENC_SCHEME encScheme,		/* mode */
					const unsigned char *symmetric_key,
					uint32_t symmetric_key_size,
					unsigned char *pad_in,
					uint32_t pad_in_size,
					uint32_t size,
					uint32_t index,
					uint32_t len)
{
    TPM_RESULT rc = 0;
    
    printf("  TPM_Transport_CryptSymmetric: size %u index %u len %u\n", size, index, len); 
    /* sanity check the length */
    if (rc == 0) {
	if (index + len > size) {
	    printf("TPM_Transport_CryptSymmetric: Error (fatal), bad size\n"); 
	    rc = TPM_FAIL;	/* internal error, should never occur */
	}
    }
    if (rc == 0) {
	/* leading clear text area */
	memcpy(dest, src, index);
	dest += index;
	src += index;
	/* encrypt area */
	rc = TPM_SymmetricKeyData_StreamCrypt(dest,			/* output */
					      src,			/* input */
					      len,			/* input */
					      algId,			/* algorithm */
					      encScheme,		/* mode */
					      symmetric_key,		/* input */
					      symmetric_key_size,	/* input */
					      pad_in,			/* input */
					      pad_in_size);		/* input */
    }
    if (rc == 0) {
	dest += len;
	src += len;
	/* trailing clear text area */
	memcpy(dest, src, size - index - len);
    }
    return rc;
}

/*
  Transport Sessions (the entire array)
*/

void TPM_TransportSessions_Init(TPM_TRANSPORT_INTERNAL *transSessions)
{
    size_t i;
    
    printf(" TPM_TransportSessions_Init:\n");
    for (i = 0 ; i < TPM_MIN_TRANS_SESSIONS ; i++) {
	TPM_TransportInternal_Init(&(transSessions[i]));
    }
    return;
}

/* TPM_TransportSessions_Load() reads a count of the number of stored sessions and then loads those
   sessions.

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_TransportSessions_Init()
   After use, call TPM_TransportSessions_Delete() to free memory
*/

TPM_RESULT TPM_TransportSessions_Load(TPM_TRANSPORT_INTERNAL *transSessions,
				      unsigned char **stream,
				      uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;
    size_t		i;
    uint32_t		activeCount;

    printf(" TPM_TransportSessions_Load:\n");
    /* load active count */
    if (rc == 0) {
	rc = TPM_Load32(&activeCount, stream, stream_size);
    }
    if (rc == 0) {
	if (activeCount > TPM_MIN_TRANS_SESSIONS) {
	    printf("TPM_TransportSessions_Load: Error (fatal) %u sessions, %u slots\n",
		   activeCount, TPM_MIN_TRANS_SESSIONS);
	    rc = TPM_FAIL;
	}
    }    
    if (rc == 0) {
	printf(" TPM_TransportSessions_Load: Loading %u sessions\n", activeCount);
    }
    for (i = 0 ; (rc == 0) && (i < activeCount) ; i++) {
	rc = TPM_TransportInternal_Load(&(transSessions[i]), stream, stream_size);
    }
    return rc;
}

/* TPM_TransportSessions_Store() stores a count of the active sessions, followed by the sessions.
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_TransportSessions_Store(TPM_STORE_BUFFER *sbuffer,
				       TPM_TRANSPORT_INTERNAL *transSessions)
{
    TPM_RESULT		rc = 0;
    size_t		i;
    uint32_t		space;		/* free transport session slots */
    uint32_t		activeCount;	/* used transport session slots */

    /* store active count */
    if (rc == 0) {
	TPM_TransportSessions_GetSpace(&space, transSessions);
	activeCount = TPM_MIN_TRANS_SESSIONS - space;
	printf(" TPM_TransSessions_Store: Storing %u sessions\n", activeCount);
	rc = TPM_Sbuffer_Append32(sbuffer, activeCount);
    }
    /* store transport sessions */
    for (i = 0 ; (rc == 0) && (i < TPM_MIN_TRANS_SESSIONS) ; i++) {
	if ((transSessions[i]).valid) {	     /* if the session is active */
	    rc = TPM_TransportInternal_Store(sbuffer, &(transSessions[i]));
	}
    }
    return rc;
}

/* TPM_TransportSessions_Delete() terminates all sessions

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_TransportSessions_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_TransportSessions_Delete(TPM_TRANSPORT_INTERNAL *transSessions)
{
    size_t i;
    
    printf(" TPM_TransportSessions_Delete:\n");
    for (i = 0 ; i < TPM_MIN_TRANS_SESSIONS ; i++) {
	TPM_TransportInternal_Delete(&(transSessions[i]));
    }
    return;
}

/* TPM_TransportSessions_IsSpace() returns 'isSpace' TRUE if an entry is available, FALSE if not.

   If TRUE, 'index' holds the first free position.
*/

void TPM_TransportSessions_IsSpace(TPM_BOOL *isSpace, uint32_t *index,
				   TPM_TRANSPORT_INTERNAL *transSessions)
{
    printf(" TPM_TransportSessions_IsSpace:\n");
    for (*index = 0, *isSpace = FALSE ; *index < TPM_MIN_TRANS_SESSIONS ; (*index)++) {
	if (!((transSessions[*index]).valid)) {
	    printf("  TPM_TransportSessions_IsSpace: Found space at %u\n", *index);
	    *isSpace = TRUE;
	    break;
	}	    
    }
    return;
}

/* TPM_TransportSessions_GetSpace() returns the number of unused transport sessions.

*/

void TPM_TransportSessions_GetSpace(uint32_t *space,
				    TPM_TRANSPORT_INTERNAL *transSessions)
{
    uint32_t i;

    printf(" TPM_TransportSessions_GetSpace:\n");
    for (*space = 0 , i = 0 ; i < TPM_MIN_TRANS_SESSIONS ; i++) {
	if (!((transSessions[i]).valid)) {
	    (*space)++;
	}	    
    }
    return;
}

/* TPM_TransportSessions_StoreHandles() stores

   - the number of loaded sessions
   - a list of session handles
*/

TPM_RESULT TPM_TransportSessions_StoreHandles(TPM_STORE_BUFFER *sbuffer,
					      TPM_TRANSPORT_INTERNAL *transSessions)
{
    TPM_RESULT	rc = 0;
    uint16_t	i;
    uint32_t	space;
    
    printf(" TPM_TransportSessions_StoreHandles:\n");
    /* get the number of loaded handles */
    if (rc == 0) {
	TPM_TransportSessions_GetSpace(&space, transSessions);
	/* store loaded handle count.  Cast safe because of TPM_MIN_TRANS_SESSIONS value */
	printf(" TPM_TransportSessions_StoreHandles: %u handles\n",
	       TPM_MIN_TRANS_SESSIONS - space);
	rc = TPM_Sbuffer_Append16(sbuffer, (uint16_t)(TPM_MIN_TRANS_SESSIONS - space)); 
    }
    for (i = 0 ; (rc == 0) && (i < TPM_MIN_TRANS_SESSIONS) ; i++) {
	if ((transSessions[i]).valid) {		     /* if the index is loaded */
	    rc = TPM_Sbuffer_Append32(sbuffer, (transSessions[i]).transHandle); /* store it */
	}
    }
    return rc;
}

/* TPM_TransportSessions_GetNewHandle() checks for space in the transport sessions table.

   If there is space, it returns a TPM_TRANSPORT_INTERNAL entry in 'tpm_transport_internal'.  The
   entry is marked 'valid'.

   Returns TPM_RESOURCES if there is no space in the transport sessions table.
*/

TPM_RESULT TPM_TransportSessions_GetNewHandle(TPM_TRANSPORT_INTERNAL **tpm_transport_internal,
					      TPM_TRANSPORT_INTERNAL *transportSessions)
{
    TPM_RESULT			rc = 0;
    uint32_t			index;
    TPM_BOOL			isSpace;
    TPM_TRANSHANDLE		transportHandle = 0;	/* no suggested value */
    
    printf(" TPM_TransportSessions_GetNewHandle:\n");
    /* is there an empty entry, get the location index */
    if (rc == 0) {
	TPM_TransportSessions_IsSpace(&isSpace, &index, transportSessions);
	if (!isSpace) {
	    printf("TPM_TransportSessions_GetNewHandle: Error, "
		   "no space in TransportSessions table\n");
	    rc = TPM_RESOURCES;
	}
    }
    /* assign transport handle */
    if (rc == 0) {
	rc = TPM_Handle_GenerateHandle(&transportHandle,	/* I/O */
				       transportSessions,	/* handle array */
				       FALSE,			/* keepHandle */
				       FALSE,			/* isKeyHandle */
				       (TPM_GETENTRY_FUNCTION_T)TPM_TransportSessions_GetEntry);
    }
    if (rc == 0) {
	printf("  TPM_TransportSessions_GetNewHandle: Assigned handle %08x\n", transportHandle);
	/* return the TPM_TRANSPORT_INTERNAL */
	*tpm_transport_internal = &(transportSessions[index]);
	/* assign the handle */
	(*tpm_transport_internal)->transHandle = transportHandle;
	(*tpm_transport_internal)->valid = TRUE;
    }
    return rc;
}

/* TPM_TransportSessions_GetEntry() searches all 'transportSessions' entries for the entry matching
   the handle, and returns the TPM_TRANSPORT_INTERNAL entry associated with the handle.

   Returns
       0 for success
       TPM_INVALID_AUTHHANDLE if the handle is not found
*/

TPM_RESULT TPM_TransportSessions_GetEntry(TPM_TRANSPORT_INTERNAL **tpm_transport_internal,
					  TPM_TRANSPORT_INTERNAL *transportSessions,	/* array */
					  TPM_TRANSHANDLE transportHandle)		/* input */
{
    TPM_RESULT	rc = 0;
    size_t	i;
    TPM_BOOL	found;
    
    printf(" TPM_TransportSessions_GetEntry: transportHandle %08x\n", transportHandle);
    for (i = 0, found = FALSE ; (i < TPM_MIN_TRANS_SESSIONS) && !found ; i++) {
	if ((transportSessions[i].valid) &&		 
	    (transportSessions[i].transHandle == transportHandle)) {	  /* found */
	    found = TRUE;
	    *tpm_transport_internal = &(transportSessions[i]);
	}
    }
    if (!found) {
	printf("  TPM_TransportSessions_GetEntry: transport session handle %08x not found\n",
	       transportHandle);
	rc = TPM_INVALID_AUTHHANDLE;
    }
    return rc;
}

/* TPM_TransportSessions_AddEntry() adds an TPM_TRANSPORT_INTERNAL object to the list.

   If *tpm_handle == 0, a value is assigned.  If *tpm_handle != 0, that value is used if it it not
   currently in use.

   The handle is returned in tpm_handle.
*/

TPM_RESULT TPM_TransportSessions_AddEntry(TPM_HANDLE *tpm_handle,			/* i/o */
					  TPM_BOOL keepHandle,				/* input */
					  TPM_TRANSPORT_INTERNAL *transSessions,	/* input */
					  TPM_TRANSPORT_INTERNAL *tpm_transport_internal) /* in */
{
    TPM_RESULT			rc = 0;
    uint32_t			index;
    TPM_BOOL			isSpace;
    
    printf(" TPM_TransportSessions_AddEntry: handle %08x, keepHandle %u\n",
	   *tpm_handle, keepHandle);
    /* check for valid TPM_TRANSPORT_INTERNAL */
    if (rc == 0) {
	if (tpm_transport_internal == NULL) {	/* NOTE: should never occur */
	    printf("TPM_TransportSessions_AddEntry: Error (fatal), NULL TPM_TRANSPORT_INTERNAL\n");
	    rc = TPM_FAIL;
	}
    }
    /* is there an empty entry, get the location index */
    if (rc == 0) {
	TPM_TransportSessions_IsSpace(&isSpace, &index, transSessions);
	if (!isSpace) {
	    printf("TPM_TransportSessions_AddEntry: Error, transport session entries full\n");
	    rc = TPM_RESOURCES;
	}
    }
    if (rc == 0) {
	rc = TPM_Handle_GenerateHandle(tpm_handle,		/* I/O */
				       transSessions,		/* handle array */
				       keepHandle,		/* keepHandle */
				       FALSE,			/* isKeyHandle */
				       (TPM_GETENTRY_FUNCTION_T)TPM_TransportSessions_GetEntry);
    }
    if (rc == 0) {
	tpm_transport_internal->transHandle = *tpm_handle;
	tpm_transport_internal->valid = TRUE;
	TPM_TransportInternal_Copy(&(transSessions[index]), tpm_transport_internal);
	printf("  TPM_TransportSessions_AddEntry: Index %u handle %08x\n",
	       index, transSessions[index].transHandle);
    }
    return rc;
}

/* TPM_TransportSessions_TerminateHandle() terminates the session associated with
   'transporthHandle'.
   
   If the session is exclusive (indicated by a match with TPM_STANY_FLAGS -> transportExclusive),
   clear that flag.
*/

TPM_RESULT TPM_TransportSessions_TerminateHandle(TPM_TRANSPORT_INTERNAL *transportSessions,
						 TPM_TRANSHANDLE transportHandle,
						 TPM_TRANSHANDLE *transportExclusive)
{
    TPM_RESULT	rc = 0;
    TPM_TRANSPORT_INTERNAL *tpm_transport_internal;

    printf(" TPM_TransportSessions_TerminateHandle: Handle %08x\n", transportHandle);
    /* get the TPM_TRANSPORT_INTERNAL associated with the TPM_TRANSHANDLE */
    if (rc == 0) {
	rc = TPM_TransportSessions_GetEntry(&tpm_transport_internal,
					    transportSessions,
					    transportHandle);
    }
    /* if the session being terminated is exclusive, reset the flag */
    if (rc == 0) {
	if (transportHandle == *transportExclusive) {
	    printf("  TPM_TransportSessions_TerminateHandle: Is exclusive transport session\n");
	    if (!(tpm_transport_internal->transPublic.transAttributes & TPM_TRANSPORT_EXCLUSIVE)) {
		printf("TPM_TransportSessions_TerminateHandle: Error (fatal), "
		       "attribute is not exclusive\n");
		rc = TPM_FAIL;	/* internal error, should not occur */
	    }
	    *transportExclusive = 0;
	}
    }
    /* invalidate the valid handle */
    if (rc == 0) {
	TPM_TransportInternal_Delete(tpm_transport_internal);
    }
    return rc;
}

/*
  TPM_TRANSPORT_PUBLIC
*/

/* TPM_TransportPublic_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_TransportPublic_Init(TPM_TRANSPORT_PUBLIC *tpm_transport_public)
{
    printf(" TPM_TransportPublic_Init:\n");
    tpm_transport_public->transAttributes = 0;
    tpm_transport_public->algId = 0;
    tpm_transport_public->encScheme = TPM_ES_NONE;
    return;
}

/* TPM_TransportPublic_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_TransportPublic_Init()
   After use, call TPM_TransportPublic_Delete() to free memory
*/

TPM_RESULT TPM_TransportPublic_Load(TPM_TRANSPORT_PUBLIC *tpm_transport_public,
			   unsigned char **stream,
			   uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_TransportPublic_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_TRANSPORT_PUBLIC, stream, stream_size);
    }
    /* load transAttributes */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_transport_public->transAttributes ), stream, stream_size);
    }
    /* load algId */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_transport_public->algId), stream, stream_size);
    }
    /* load encScheme */
    if (rc == 0) {
	rc = TPM_Load16(&(tpm_transport_public->encScheme), stream, stream_size);
    }
    return rc;
}

/* TPM_TransportPublic_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_TransportPublic_Store(TPM_STORE_BUFFER *sbuffer,
				     const TPM_TRANSPORT_PUBLIC *tpm_transport_public)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_TransportPublic_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_TRANSPORT_PUBLIC);
    }
    /* store transAttributes */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_transport_public->transAttributes);
    }
    /* store algId */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_transport_public->algId);
    }
    /* store encScheme */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, tpm_transport_public->encScheme);
    }
    return rc;
}

/* TPM_TransportPublic_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_TransportPublic_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_TransportPublic_Delete(TPM_TRANSPORT_PUBLIC *tpm_transport_public)
{
    printf(" TPM_TransportPublic_Delete:\n");
    if (tpm_transport_public != NULL) {
	TPM_TransportPublic_Init(tpm_transport_public);
    }
    return;
}

/* TPM_TransportPublic_Copy() copies the 'src' to the 'dest' structure
   
*/

TPM_RESULT TPM_TransportPublic_Copy(TPM_TRANSPORT_PUBLIC *dest,
				    const TPM_TRANSPORT_PUBLIC *src)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_TransportPublic_Copy:\n");
    /* copy transAttributes */
    dest->transAttributes = src->transAttributes;
    /* copy algId */
    dest->algId = src->algId;
    /* copy encScheme */
    dest->encScheme = src->encScheme;
    return rc;
}

/* TPM_TransportPublic_CheckAlgId() returns 'supported' TRUE if the transport encryption algorithm
   is supported by the TPM

*/

void TPM_TransportPublic_CheckAlgId(TPM_BOOL *supported,
				    TPM_ALGORITHM_ID algId)
{
    printf(" TPM_TransportPublic_CheckAlgId: %08x\n", algId);
    switch (algId) {
	/* supported protocols */
      case TPM_ALG_MGF1:
      case TPM_ALG_AES128:
	*supported = TRUE;
	break;
	/* unsupported protocols */
      case TPM_ALG_RSA:
      case TPM_ALG_SHA:
      case TPM_ALG_HMAC:
      case TPM_ALG_AES192:
      case TPM_ALG_AES256:
      default:
	*supported = FALSE;
	break;
    }	
    return;
}

/* TPM_TransportPublic_CheckEncScheme() returns success and the blockSize if the transport algId and
   encScheme are supported by the TPM.
*/

TPM_RESULT TPM_TransportPublic_CheckEncScheme(uint32_t *blockSize,
					      TPM_ALGORITHM_ID algId,
					      TPM_ENC_SCHEME encScheme,
					      TPM_BOOL FIPS)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_TransportPublic_CheckEncScheme: algId %08x encScheme %04hx\n", algId, encScheme);
    switch (algId) {
	/* supported protocols with no encScheme */
      case TPM_ALG_MGF1:
	*blockSize = 0;		/* MGF1 does not use blocks */
	if (FIPS) {
	    printf("TPM_TransportPublic_CheckEncScheme: Error, "
		   "TPM_ALG_MGF1 not supported in FIPS\n");
	    rc = TPM_INAPPROPRIATE_ENC;
	}
	/* For TPM_ALG_MGF1, TPM_ENC_SCHEME is not used.  The TPM MAY validate that TPM_ENC_SCHEME
	   is TPM_ES_NONE. */
	if (encScheme != TPM_ES_NONE) {
	    printf("TPM_TransportPublic_CheckEncScheme: Error, "
		   "TPM_ALG_MGF1 must use TPM_ES_NONE\n");
	    rc = TPM_INAPPROPRIATE_ENC;
	}
	break;
	/* protocols with encScheme */
      case TPM_ALG_AES128:
	switch(encScheme) {
	  case TPM_ES_SYM_CTR:	/* CTR mode */
	  case TPM_ES_SYM_OFB:	/* OFB mode */
	    *blockSize = 128/8;
	    break;
	  default:
	    printf("TPM_TransportPublic_CheckEncScheme: Error, AES128 encScheme not supported\n");
	    rc = TPM_INAPPROPRIATE_ENC;
	    break;
	}
	break;
	/* unsupported protocols */
      case TPM_ALG_AES192:
      case TPM_ALG_AES256:
      case TPM_ALG_RSA:
      case TPM_ALG_SHA:
      case TPM_ALG_HMAC:
      case TPM_ALG_XOR:
      default:
	printf("TPM_TransportPublic_CheckEncScheme: Error, algId not supported\n");
	rc = TPM_BAD_KEY_PROPERTY;
	break;
    }	
    return rc;
}

/*
  TPM_TRANSPORT_INTERNAL
*/

/* TPM_TransportInternal_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_TransportInternal_Init(TPM_TRANSPORT_INTERNAL *tpm_transport_internal)
{
    printf(" TPM_TransportInternal_Init:\n");
    TPM_Secret_Init(tpm_transport_internal->authData);
    TPM_TransportPublic_Init(&(tpm_transport_internal->transPublic));
    tpm_transport_internal->transHandle = 0;
    TPM_Nonce_Init(tpm_transport_internal->transNonceEven);
    TPM_Digest_Init(tpm_transport_internal->transDigest);
    tpm_transport_internal->valid = FALSE;
    return;
}

/* TPM_TransportInternal_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_TransportInternal_Init()
   After use, call TPM_TransportInternal_Delete() to free memory
*/

TPM_RESULT TPM_TransportInternal_Load(TPM_TRANSPORT_INTERNAL *tpm_transport_internal,
				      unsigned char **stream,
				      uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_TransportInternal_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_TRANSPORT_INTERNAL, stream, stream_size);
    }
    /* load authData */
    if (rc == 0) {
	rc = TPM_Secret_Load(tpm_transport_internal->authData, stream, stream_size);
    }
    /* load transPublic */
    if (rc == 0) {
	rc = TPM_TransportPublic_Load(&(tpm_transport_internal->transPublic), stream, stream_size);
    }
    /* load transHandle */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_transport_internal->transHandle), stream, stream_size);
    }
    /* load transNonceEven */
    if (rc == 0) {
	rc = TPM_Nonce_Load(tpm_transport_internal->transNonceEven, stream, stream_size);
    }
    /* load transDigest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_transport_internal->transDigest, stream, stream_size);
    }
    /* load valid */
    if (rc == 0) {
	tpm_transport_internal->valid = TRUE;
    }
    return rc;
}

/* TPM_TransportInternal_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_TransportInternal_Store(TPM_STORE_BUFFER *sbuffer,
				       const TPM_TRANSPORT_INTERNAL *tpm_transport_internal)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_TransportInternal_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_TRANSPORT_INTERNAL);
    }
    /* store authData */
    if (rc == 0) {
	rc = TPM_Secret_Store(sbuffer, tpm_transport_internal->authData);
    }
    /* store transPublic */
    if (rc == 0) {
	rc = TPM_TransportPublic_Store(sbuffer, &(tpm_transport_internal->transPublic));
    }
    /* store transHandle */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_transport_internal->transHandle);
    }
    /* store transNonceEven */
    if (rc == 0) {
	rc = TPM_Nonce_Store(sbuffer, tpm_transport_internal->transNonceEven);
    }
    /* store transDigest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_transport_internal->transDigest);
    }
    return rc;
}

/* TPM_TransportInternal_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_TransportInternal_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_TransportInternal_Delete(TPM_TRANSPORT_INTERNAL *tpm_transport_internal)
{
    printf(" TPM_TransportInternal_Delete:\n");
    if (tpm_transport_internal != NULL) {
	TPM_TransportPublic_Delete(&(tpm_transport_internal->transPublic));
	TPM_TransportInternal_Init(tpm_transport_internal);
    }
    return;
}

/* TPM_TransportInternal_Copy() copies the source to the destination.

*/

void TPM_TransportInternal_Copy(TPM_TRANSPORT_INTERNAL *dest_transport_internal,
				TPM_TRANSPORT_INTERNAL *src_transport_internal)
{
    TPM_Secret_Copy(dest_transport_internal->authData, src_transport_internal->authData);
    TPM_TransportPublic_Copy(&(dest_transport_internal->transPublic),
			     &(src_transport_internal->transPublic));
    dest_transport_internal->transHandle = src_transport_internal->transHandle;
    TPM_Nonce_Copy(dest_transport_internal->transNonceEven, src_transport_internal->transNonceEven);
    TPM_Digest_Copy(dest_transport_internal->transDigest, src_transport_internal->transDigest);
    dest_transport_internal->valid = src_transport_internal->valid;
}

/* TPM_TransportInternal_Check() checks the authorization of a command.

   There is no need to protect against dictionary attacks.  The first failure terminates the
   transport session.

   Returns TPM_AUTH2FAIL if the TPM_AUTHDATA does not match.
*/

TPM_RESULT TPM_TransportInternal_Check(TPM_DIGEST		inParamDigest,	/* digest of inputs
										   above line */
				       TPM_TRANSPORT_INTERNAL	*tpm_transport_internal,
				       TPM_NONCE		transNonceOdd,	/* Nonce generated
										   by system
										   associated with
										   transHandle */
				       TPM_BOOL			continueTransSession,
				       TPM_AUTHDATA		transAuth)	/* Authorization
										   digest for
										   input */
{
    TPM_RESULT		rc = 0;
    TPM_BOOL		valid;
    
    printf(" TPM_TransportInternal_Check:\n");
    if (rc == 0) {
	TPM_PrintFour("  TPM_TransportInternal_Check: inParamDigest", inParamDigest);
	TPM_PrintFour("  TPM_TransportInternal_Check: usageAuth (key)",
		      tpm_transport_internal->authData);
	TPM_PrintFour("  TPM_TransportInternal_Check: nonceEven",
		      tpm_transport_internal->transNonceEven);
	TPM_PrintFour("  TPM_TransportInternal_Check: nonceOdd", transNonceOdd);
	printf       ("  TPM_TransportInternal_Check: continueSession %02x\n", continueTransSession);
	/* HMAC the inParamDigest, transLastNonceEven, transNonceOdd, continueTransSession */
	/* transLastNonceEven is retrieved from internal transport session storage */
	rc = TPM_HMAC_Check(&valid,
			    transAuth,					/* expected, from command */
			    tpm_transport_internal->authData,		/* key */
			    sizeof(TPM_DIGEST), inParamDigest,		/* command digest */
			    sizeof(TPM_NONCE), tpm_transport_internal->transNonceEven,	/* 2H */
			    sizeof(TPM_NONCE), transNonceOdd,				/* 3H */
			    sizeof(TPM_BOOL), &continueTransSession,			/* 4H */
			    0, NULL);
    }
    if (rc == 0) {
	if (!valid) {
	    printf("TPM_TransportInternal_Check: Error, authorization failed\n");
	    rc = TPM_AUTH2FAIL;
	}
    }
    return rc;
}

/* TPM_TransportInternal_Set() sets the transport response transAuth.

   It conditionally generates the next transNonceEven.

   It appends transNonceEven and continueTransSession to the response.

   It generates transAuth using outParamDigest and the standard 'below the line' HMAC rules and
   appends it to the response.
*/

TPM_RESULT TPM_TransportInternal_Set(TPM_STORE_BUFFER		*response,
				     TPM_TRANSPORT_INTERNAL	*tpm_transport_internal,
				     TPM_DIGEST			outParamDigest,
				     TPM_NONCE			transNonceOdd,
				     TPM_BOOL			continueTransSession,
				     TPM_BOOL			generateNonceEven)
{
    TPM_RESULT		rc = 0;
    TPM_AUTHDATA	transAuth;	/* The authorization digest for the returned parameters */

    printf(" TPM_TransportInternal_Set:\n");
    /* generate transNonceEven if not already done by caller */
    if ((rc == 0) && generateNonceEven) {
	rc = TPM_Nonce_Generate(tpm_transport_internal->transNonceEven);
    }
    /* append transNonceEven */
    if (rc == 0) {
	rc = TPM_Nonce_Store(response, tpm_transport_internal->transNonceEven);
    }
    /* append continueTransSession*/
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(response, &continueTransSession, sizeof(TPM_BOOL));
    }
    /* Calculate transAuth using the transport session authData */
    if (rc == 0) {
	rc = TPM_Authdata_Generate(transAuth,					/* result */
				   tpm_transport_internal->authData,		/* HMAC key */
				   outParamDigest,				/* params */
				   tpm_transport_internal->transNonceEven,
				   transNonceOdd,
				   continueTransSession);
    }
    /* append transAuth */
    if (rc == 0) {
	rc = TPM_Authdata_Store(response, transAuth);
    }
    return rc;
}

/*
  TPM_TRANSPORT_LOG_IN
*/

/* TPM_TransportLogIn_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_TransportLogIn_Init(TPM_TRANSPORT_LOG_IN *tpm_transport_log_in)
{
    printf(" TPM_TransportLogIn_Init:\n");
    TPM_Digest_Init(tpm_transport_log_in->parameters);
    TPM_Digest_Init(tpm_transport_log_in->pubKeyHash);
    return;
}

/* TPM_TransportLogIn_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_TransportLogIn_Store(TPM_STORE_BUFFER *sbuffer,
				    const TPM_TRANSPORT_LOG_IN *tpm_transport_log_in)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_TransportLogIn_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_TRANSPORT_LOG_IN);
    }
    /* store parameters */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_transport_log_in->parameters);
    }
    /* store pubKeyHash */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_transport_log_in->pubKeyHash);
    }
    return rc;
}

/* TPM_TransportLogIn_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_TransportLogIn_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_TransportLogIn_Delete(TPM_TRANSPORT_LOG_IN *tpm_transport_log_in)
{
    printf(" TPM_TransportLogIn_Delete:\n");
    if (tpm_transport_log_in != NULL) {
	TPM_TransportLogIn_Init(tpm_transport_log_in);
    }
    return;
}

/* TPM_TransportLogIn_Extend() extends 'tpm_digest'

   tpm_digest = SHA-1 (tpm_digest || tpm_transport_log_in)
*/

TPM_RESULT TPM_TransportLogIn_Extend(TPM_DIGEST tpm_digest,
				     TPM_TRANSPORT_LOG_IN *tpm_transport_log_in)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	sbuffer;
    const unsigned char *buffer;	/* serialized buffer */
    uint32_t		length; /* serialization length */
    
    printf(" TPM_TransportLogIn_Extend:\n");
    TPM_Sbuffer_Init(&sbuffer);		/* freed @1 */
    /* serialize TPM_TRANSPORT_LOG_IN */
    if (rc == 0) {
	rc = TPM_TransportLogIn_Store(&sbuffer, tpm_transport_log_in);
    }
    if (rc == 0) {
	/* get the TPM_TRANSPORT_LOG_IN	 serialization results */
	TPM_Sbuffer_Get(&sbuffer, &buffer, &length);
	TPM_PrintAll("  TPM_TransportLogIn_Extend: transDigest in", tpm_digest, TPM_DIGEST_SIZE);
	TPM_PrintAll("  TPM_TransportLogIn_Extend", buffer, length);
	rc = TPM_SHA1(tpm_digest,
		      TPM_DIGEST_SIZE, tpm_digest,
		      length, buffer,
		      0, NULL);
	TPM_PrintAll("  TPM_TransportLogIn_Extend: transDigest out", tpm_digest, TPM_DIGEST_SIZE);
    }
    TPM_Sbuffer_Delete(&sbuffer);	/* @1 */
    return rc;
}

/*
  TPM_TRANSPORT_LOG_OUT
*/

/* TPM_TransportLogOut_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_TransportLogOut_Init(TPM_TRANSPORT_LOG_OUT *tpm_transport_log_out)
{
    printf(" TPM_TransportLogOut_Init:\n");
    TPM_CurrentTicks_Init(&(tpm_transport_log_out->currentTicks));
    TPM_Digest_Init(tpm_transport_log_out->parameters);
    tpm_transport_log_out = 0;
    return;
}

/* TPM_TransportLogOut_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_TransportLogOut_Store(TPM_STORE_BUFFER *sbuffer,
				    const TPM_TRANSPORT_LOG_OUT *tpm_transport_log_out)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_TransportLogOut_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_TRANSPORT_LOG_OUT);
    }
    /* store currentTicks */
    if (rc == 0) {
	rc = TPM_CurrentTicks_Store(sbuffer, &(tpm_transport_log_out->currentTicks));
    }
    /* store parameters */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_transport_log_out->parameters);
    }
    /* store locality */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_transport_log_out->locality);
    }
    return rc;
}

/* TPM_TransportLogOut_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_TransportLogOut_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_TransportLogOut_Delete(TPM_TRANSPORT_LOG_OUT *tpm_transport_log_out)
{
    printf(" TPM_TransportLogOut_Delete:\n");
    if (tpm_transport_log_out != NULL) {
	TPM_TransportLogOut_Init(tpm_transport_log_out);
    }
    return;
}

/* TPM_TransportLogOut_Extend() extends 'tpm_digest'

   tpm_digest = SHA-1 (tpm_digest || tpm_transport_log_out)
*/

TPM_RESULT TPM_TransportLogOut_Extend(TPM_DIGEST tpm_digest,
				      TPM_TRANSPORT_LOG_OUT *tpm_transport_log_out)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	sbuffer;
    const unsigned char *buffer;	/* serialized buffer */
    uint32_t		length; /* serialization length */
    
    printf(" TPM_TransportLogOut_Extend:\n");
    TPM_Sbuffer_Init(&sbuffer);		/* freed @1 */
    /* serialize TPM_TRANSPORT_LOG_OUT */
    if (rc == 0) {
	rc = TPM_TransportLogOut_Store(&sbuffer, tpm_transport_log_out);
    }
    if (rc == 0) {
	/* get the TPM_TRANSPORT_LOG_OUT  serialization results */
	TPM_Sbuffer_Get(&sbuffer, &buffer, &length);
	TPM_PrintAll("  TPM_TransportLogOut_Extend: transDigest in", tpm_digest, TPM_DIGEST_SIZE);
	TPM_PrintAll("  TPM_TransportLogOut_Extend:", buffer, length);
	rc = TPM_SHA1(tpm_digest,
		      TPM_DIGEST_SIZE, tpm_digest,
		      length, buffer,
		      0, NULL);
	TPM_PrintAll("  TPM_TransportLogOut_Extend: transDigest out", tpm_digest, TPM_DIGEST_SIZE);
    }
    TPM_Sbuffer_Delete(&sbuffer);	/* @1 */
    return rc;
}

/*
  TPM_TRANSPORT_AUTH
*/

/* TPM_TransportAuth_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_TransportAuth_Init(TPM_TRANSPORT_AUTH *tpm_transport_auth)
{
    printf(" TPM_TransportAuth_Init:\n");
    TPM_Secret_Init(tpm_transport_auth->authData);
    return;
}

/* TPM_TransportAuth_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_TransportAuth_Init()
   After use, call TPM_TransportAuth_Delete() to free memory
*/

TPM_RESULT TPM_TransportAuth_Load(TPM_TRANSPORT_AUTH *tpm_transport_auth,
				  unsigned char **stream,
				  uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_TransportAuth_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_TRANSPORT_AUTH, stream, stream_size);
    }
    /* load authData */
    if (rc == 0) {
	rc = TPM_Secret_Load(tpm_transport_auth->authData, stream, stream_size);
    }
    return rc;
}

/* TPM_TransportAuth_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_TransportAuth_Store(TPM_STORE_BUFFER *sbuffer,
				   const TPM_TRANSPORT_AUTH *tpm_transport_auth)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_TransportAuth_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_TRANSPORT_AUTH);
    }
    /* store authData */
    if (rc == 0) {
	rc = TPM_Secret_Store(sbuffer, tpm_transport_auth->authData);
    }
    return rc;
}

/* TPM_TransportAuth_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_TransportAuth_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_TransportAuth_Delete(TPM_TRANSPORT_AUTH *tpm_transport_auth)
{
    printf(" TPM_TransportAuth_Delete:\n");
    if (tpm_transport_auth != NULL) {
	TPM_TransportAuth_Init(tpm_transport_auth);
    }
    return;
}

/* TPM_TransportAuth_DecryptSecret() decrypts the secret using the private key.	 The
   result is deserialized and stored in the TPM_TRANSPORT_AUTH structure.
*/

TPM_RESULT TPM_TransportAuth_DecryptSecret(TPM_TRANSPORT_AUTH *tpm_transport_auth,	/* result */
					   TPM_SIZED_BUFFER *secret,	/* encrypted input */
					   TPM_KEY *tpm_key)		/* key for decrypting */
{
    TPM_RESULT		rc = 0;
    unsigned char	*decryptData = NULL;	/* freed @1 */
    uint32_t		decryptDataLength = 0;	/* actual valid data */
    unsigned char	*stream;
    uint32_t		stream_size;
    
    printf(" TPM_TransportAuth_DecryptSecret:\n");
    /* allocate space for the decrypted data */
    if (rc == 0) {
	rc = TPM_RSAPrivateDecryptMalloc(&decryptData,		/* decrypted data */
					 &decryptDataLength,	/* actual size of decrypted data */
					 secret->buffer,	/* encrypted data */
					 secret->size,		/* encrypted data size */
					 tpm_key);
    }
    /* load the TPM_TRANSPORT_AUTH structure from the decrypted data stream */
    if (rc == 0) {
	/* use temporary variables, because TPM_TransportAuth_Load() moves the stream */
	stream = decryptData;
	stream_size = decryptDataLength;
	rc = TPM_TransportAuth_Load(tpm_transport_auth, &stream, &stream_size);
    }
    free(decryptData);		/* @1 */
    return rc;
}

/*
  Processing Functions
*/

/* 24.1 TPM_EstablishTransport rev 98

   This establishes the transport session. Depending on the attributes specified for the session
   this may establish shared secrets, encryption keys, and session logs. The session will be in use
   for by the TPM_ExecuteTransport command.

   The only restriction on what can happen inside of a transport session is that there is no
   "nesting" of sessions. It is permissible to perform operations that delete internal state and
   make the TPM inoperable.
*/
  
TPM_RESULT TPM_Process_EstablishTransport(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE		encHandle;	/* The handle to the key that encrypted the blob */
    TPM_TRANSPORT_PUBLIC	transPublic;	/* The public information describing the transport
						   session */
    TPM_SIZED_BUFFER		secret;		/* The encrypted secret area */
    TPM_AUTHHANDLE		authHandle;	/* The authorization session handle used for
						   keyHandle authorization */
    TPM_NONCE			nonceOdd;	/* Nonce generated by system associated with
						   authHandle */
    TPM_BOOL			continueAuthSession = TRUE;	/* The continue use flag for the
								   authorization session handle */
    TPM_AUTHDATA		keyAuth;	/* Authorization. HMAC key: encKey.usageAuth */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL; /* session data for dataAuthHandle */
    TPM_SECRET			*hmacKey;
    TPM_KEY			*encKey = NULL;		/* the key specified by encHandle */
    TPM_BOOL			parentPCRStatus;
    TPM_SECRET			*encKeyUsageAuth;
    TPM_AUTHDATA		*a1AuthData = NULL;
    TPM_TRANSPORT_INTERNAL	*t1TpmTransportInternal;
    TPM_TRANSPORT_AUTH		k1TransportAuth;
    uint32_t			blockSize;		/* symmetric key block size, not used */
    TPM_TRANSPORT_LOG_IN	l1TransportLogIn;
    TPM_TRANSPORT_LOG_OUT	l2TransportLogOut;
    TPM_STORE_BUFFER		transPublicSbuffer;	/* serialized transPublic */
    const unsigned char		*transPublicBuffer;	/* serialized buffer */
    uint32_t			transPublicLength;	/* serialization length */
    TPM_STORE_BUFFER		currentTicksSbuffer;	/* serialized currentTicks */
    const unsigned char		*currentTicksBuffer;	/* serialized buffer */
    uint32_t			currentTicksLength;	/* serialization length */
    TPM_COMMAND_CODE		nOrdinal;		/* ordinal in nbo */
    uint32_t			nSecretSize;		/* secretSize in nbo */
    TPM_RESULT			nReturnCode;		/* returnCode in nbo */
    TPM_MODIFIER_INDICATOR	nLocality;		/* locality in nbo */
    TPM_BOOL			trans_session_added = FALSE;
    /* output parameters  */
    uint32_t			outParamStart;		/* starting point of outParam's */
    uint32_t			outParamEnd;		/* ending point of outParam's */
    TPM_DIGEST			outParamDigest;
    TPM_CURRENT_TICKS		currentTicks;		/* The current tick count  */
    TPM_NONCE			transNonceEven; 	/* The even nonce in use for subsequent
							   execute transport */
    
    printf("TPM_Process_EstablishTransport: Ordinal Entry\n");
    TPM_TransportPublic_Init(&transPublic);			/* freed @1 */
    TPM_SizedBuffer_Init(&secret);				/* freed @2 */
    TPM_CurrentTicks_Init(&currentTicks);			/* no need to free */
    TPM_TransportAuth_Init(&k1TransportAuth);			/* freed @4 */
    TPM_TransportLogIn_Init(&l1TransportLogIn);			/* freed @5 */
    TPM_TransportLogOut_Init(&l2TransportLogOut);		/* freed @6 */
    TPM_Sbuffer_Init(&transPublicSbuffer);			/* freed @7 */
    TPM_Sbuffer_Init(&currentTicksSbuffer);			/* freed @8 */
    /*
      get inputs
    */
    /* get encHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&encHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get transPublic */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_EstablishTransport: keyHandle %08x\n", encHandle);
	returnCode = TPM_TransportPublic_Load(&transPublic, &command, &paramSize);
    }	 
    /* get secret */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_EstablishTransport: transPublic->transAttributes %08x\n",
	       transPublic.transAttributes);
	returnCode = TPM_SizedBuffer_Load(&secret, &command, &paramSize);
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
	    printf("TPM_Process_EstablishTransport: Error, command has %u extra bytes\n",
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
	/* 1. If encHandle is TPM_KH_TRANSPORT then */
	if (encHandle == TPM_KH_TRANSPORT) {
	    printf("TPM_Process_EstablishTransport: TPM_KH_TRANSPORT clear text secret\n");
	    /* a. If tag is NOT TPM_TAG_RQU_COMMAND return TPM_BADTAG */
	    if (returnCode == TPM_SUCCESS) {
		if (tag != TPM_TAG_RQU_COMMAND) {
		    printf("TPM_Process_EstablishTransport: Error, "
			   "TPM_KH_TRANSPORT but not auth-0\n");
		    returnCode = TPM_BADTAG;
		}
	    }
	    /* b. If transPublic -> transAttributes specifies TPM_TRANSPORT_ENCRYPT return
	       TPM_BAD_SCHEME */
	    if (returnCode == TPM_SUCCESS) {
		if (transPublic.transAttributes & TPM_TRANSPORT_ENCRYPT) {
		    printf("TPM_Process_EstablishTransport: Error, "
			   "TPM_KH_TRANSPORT but TPM_TRANSPORT_ENCRYPT\n");
		    returnCode = TPM_BAD_SCHEME;
		}
	    }
	    /* c. If secretSize is not 20 return TPM_BAD_PARAM_SIZE */
	    if (returnCode == TPM_SUCCESS) {
		if (secret.size != TPM_DIGEST_SIZE) {
		    printf("TPM_Process_EstablishTransport: Error, secretSize %u not %u\n",
			   secret.size, TPM_DIGEST_SIZE);
		    returnCode = TPM_BAD_PARAM_SIZE;
		}
	    }
	    /* d. Set A1 to secret */
	    if (returnCode == TPM_SUCCESS) {
		a1AuthData = (TPM_AUTHDATA *)(secret.buffer);
		TPM_PrintFour("TPM_Process_EstablishTransport: transport clear text authData", *a1AuthData);
	    }
	}
	/* 2. Else */
	else {
	    printf("TPM_Process_EstablishTransport: Decrypt secret\n");
	    /* get the key corresponding to the encHandle parameter */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_KeyHandleEntries_GetKey(&encKey, &parentPCRStatus, tpm_state,
							 encHandle,
							 FALSE,	 /* not r/o, using to encrypt */
							 FALSE,		/* do not ignore PCRs */
							 FALSE);	/* cannot use EK */
	    }
	    /* a. encHandle -> keyUsage MUST be TPM_KEY_STORAGE or TPM_KEY_LEGACY return
	       TPM_INVALID_KEYUSAGE on error */
	    if (returnCode == TPM_SUCCESS) {
		if ((encKey->keyUsage != TPM_KEY_STORAGE) &&
		    (encKey->keyUsage != TPM_KEY_LEGACY)) {
		    printf("TPM_Process_EstablishTransport: Error, "
			   "key keyUsage %04hx must be TPM_KEY_STORAGE or TPM_KEY_LEGACY\n",
			   encKey->keyUsage);
		    returnCode = TPM_INVALID_KEYUSAGE;
		}
	    }
	    /* b. If encHandle -> authDataUsage does not equal TPM_AUTH_NEVER and tag is NOT
	       TPM_TAG_RQU_AUTH1_COMMAND return TPM_AUTHFAIL */
	    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_AUTH1_COMMAND)) {
		if (encKey->authDataUsage != TPM_AUTH_NEVER) {
		    printf("TPM_Process_EstablishTransport: Error, "
			   "encKey authorization required\n");
		    returnCode = TPM_AUTHFAIL;
		}
	    }
	    /* get encHandle -> usageAuth */
	    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
		returnCode = TPM_Key_GetUsageAuth(&encKeyUsageAuth, encKey);
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
						      encKey,
						      encKeyUsageAuth,		/* OIAP */
						      encKey->tpm_store_asymkey->
						      pubDataDigest);		/*OSAP */
	    }
	    /* c. Using encHandle -> usageAuth, validate the AuthData to use the key and the
	       parameters to the command */
	    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
		returnCode = TPM_Authdata_Check(tpm_state,
						*hmacKey,		/* HMAC key */
						inParamDigest,
						auth_session_data, /* authorization session */
						nonceOdd,	   /* Nonce generated by system
								      associated with authHandle */
						continueAuthSession,
						keyAuth);      /* Authorization digest for input */
	    }
	    /* d. Create K1 a TPM_TRANSPORT_AUTH structure by decrypting secret using the key
	       pointed to by encHandle */
	    /* e. Validate K1 for tag */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_TransportAuth_DecryptSecret(&k1TransportAuth,
							     &secret,
							     encKey);
		
	    }
	    /* f. Set A1 to K1 -> authData */
	    if (returnCode == TPM_SUCCESS) {
		a1AuthData = &(k1TransportAuth.authData);
		TPM_PrintFour("TPM_Process_EstablishTransport: transport decrypted authData",
			      *a1AuthData);
	    }
	}
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_EstablishTransport: transport authData", *a1AuthData);
    }
    /* 3. If transPublic -> transAttributes has TPM_TRANSPORT_ENCRYPT */
    if ((returnCode == TPM_SUCCESS) && (transPublic.transAttributes & TPM_TRANSPORT_ENCRYPT)) {
	printf("TPM_Process_EstablishTransport: Check encrypt attributes\n");
	/* a. If TPM_PERMANENT_FLAGS -> FIPS is true and transPublic -> algId is equal to
	   TPM_ALG_MGF1 return TPM_INAPPROPRIATE_ENC */
	/* b. Check if the transPublic -> algId is supported, if not return TPM_BAD_KEY_PROPERTY */
	/* c. If transPublic -> algid is TPM_ALG_AESXXX, check that transPublic -> encScheme is
	   supported, if not return TPM_INAPPROPRIATE_ENC */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_TransportPublic_CheckEncScheme(&blockSize,
							    transPublic.algId,
							    transPublic.encScheme,
							    tpm_state->tpm_permanent_flags.FIPS);
	}
	/* d. Perform any initializations necessary for the algorithm */
    }
    /* 4. Generate transNonceEven from the TPM RNG */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Nonce_Generate(transNonceEven);
    }
    /* 5. Create T1 a TPM_TRANSPORT_INTERNAL structure */
    /* NOTE Done by TPM_TransportInternal_Init() */
    /* a. Ensure that the TPM has sufficient internal space to allocate the transport session,
       return TPM_RESOURCES on error */
    /* b. Assign a T1 -> transHandle value. This value is assigned by the TPM */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_EstablishTransport: Construct TPM_TRANSPORT_INTERNAL\n");
	returnCode =
	    TPM_TransportSessions_GetNewHandle(&t1TpmTransportInternal,
					       tpm_state->tpm_stclear_data.transSessions);
    }
    if (returnCode == TPM_SUCCESS) {
	/* record that the entry is allocated, for invalidation on error */
	trans_session_added = TRUE;
	/* c. Set T1 -> transDigest to NULL */
	TPM_Digest_Init(t1TpmTransportInternal->transDigest);
	/* d. Set T1 -> transPublic to transPublic */
	TPM_TransportPublic_Copy(&(t1TpmTransportInternal->transPublic), &transPublic);
	/* e. Set T1-> transNonceEven to transNonceEven */
	TPM_Nonce_Copy(t1TpmTransportInternal->transNonceEven , transNonceEven);
	/* f. Set T1 -> authData to A1 */
	TPM_Secret_Copy(t1TpmTransportInternal->authData, *a1AuthData);
	/* 6. If TPM_STANY_DATA -> currentTicks is not properly initialized */
	/* a. Initialize the TPM_STANY_DATA -> currentTicks */
	returnCode = TPM_CurrentTicks_Update(&(tpm_state->tpm_stany_data.currentTicks));
    }
    /* 7. Set currentTicks to TPM_STANY_DATA -> currentTicks  */
    if (returnCode == TPM_SUCCESS) {
	TPM_CurrentTicks_Copy(&currentTicks, &(tpm_state->tpm_stany_data.currentTicks));
    }
    /* 8. If T1 -> transPublic -> transAttributes has TPM_TRANSPORT_LOG set then */
    if ((returnCode == TPM_SUCCESS) &&
	(t1TpmTransportInternal->transPublic.transAttributes & TPM_TRANSPORT_LOG)) {

	printf("TPM_Process_EstablishTransport: Construct TPM_TRANSPORT_LOG_IN\n");
	/* a. Create L1 a TPM_TRANSPORT_LOG_IN structure */
	/* NOTE Done by TPM_TransportLogIn_Init() */
	/* i. Set L1 -> parameters to SHA-1 (ordinal || transPublic || secretSize || secret) */
	/* serialize transPublic */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_TransportPublic_Store(&transPublicSbuffer, &transPublic);
	}
	if (returnCode == TPM_SUCCESS) {
	    /* get the transPublic serialization results */
	    TPM_Sbuffer_Get(&transPublicSbuffer, &transPublicBuffer, &transPublicLength);
	    /* digest the fields */
	    nOrdinal = htonl(ordinal);
	    nSecretSize = htonl(secret.size);
	    returnCode = TPM_SHA1(l1TransportLogIn.parameters,
				  sizeof(TPM_COMMAND_CODE), &nOrdinal,
				  transPublicLength, transPublicBuffer,
				  sizeof(uint32_t), &nSecretSize,
				  secret.size, secret.buffer,
				  0, NULL);
	}
	if (returnCode == TPM_SUCCESS) {
	    /* ii. Set L1 -> pubKeyHash to NULL */
	    /* NOTE Done by TPM_TransportLogIn_Init() */
	    /* iii. Set T1 -> transDigest to SHA-1 (T1 -> transDigest || L1) */
	    printf("TPM_Process_EstablishTransport: Extend transDigest with input\n");
	    returnCode = TPM_TransportLogIn_Extend(t1TpmTransportInternal->transDigest,
						   &l1TransportLogIn);
	}
	/* b. Create L2 a TPM_TRANSPORT_LOG_OUT structure */
	/* NOTE Done by TPM_TransportLogOut_Init() */
	/* i. Set L2 -> parameters to SHA-1 (returnCode || ordinal || locality || currentTicks ||
	   transNonceEven) */
	/* serialize currentTicks */
	if (returnCode == TPM_SUCCESS) {
	    printf("TPM_Process_EstablishTransport: Construct TPM_TRANSPORT_LOG_OUT\n");
	    returnCode = TPM_CurrentTicks_Store(&currentTicksSbuffer, &currentTicks);
	}
	if (returnCode == TPM_SUCCESS) {
	    /* get the currentTicks serialization results */
	    TPM_Sbuffer_Get(&currentTicksSbuffer, &currentTicksBuffer, &currentTicksLength);
	    nReturnCode = htonl(returnCode);
	    nLocality = htonl(tpm_state->tpm_stany_flags.localityModifier);
	    returnCode = TPM_SHA1(l2TransportLogOut.parameters,
				  sizeof(TPM_RESULT), &nReturnCode,
				  sizeof(TPM_COMMAND_CODE), &nOrdinal,
				  sizeof(TPM_MODIFIER_INDICATOR), &nLocality,
				  currentTicksLength, currentTicksBuffer,
				  TPM_NONCE_SIZE, transNonceEven,
				  0, NULL);
	}
	if (returnCode == TPM_SUCCESS) {
	    /* ii. Set L2 -> locality to the locality of this command */
	    l2TransportLogOut.locality = tpm_state->tpm_stany_flags.localityModifier;
	    /* iii. Set L2 -> currentTicks to currentTicks, this MUST be the same value that is
	       returned in the currentTicks parameter */
	    TPM_CurrentTicks_Copy(&(l2TransportLogOut.currentTicks), &currentTicks);
	    /* iv. Set T1 -> transDigest to SHA-1 (T1 -> transDigest || L2) */
	    printf("TPM_Process_EstablishTransport: Extend transDigest with output\n");
	    returnCode = TPM_TransportLogOut_Extend(t1TpmTransportInternal->transDigest,
						    &l2TransportLogOut);
	}
    }
    /* 9. If T1 -> transPublic -> transAttributes has TPM_TRANSPORT_EXCLUSIVE then
	  set TPM_STANY_FLAGS -> transportExclusive to TRUE */
    if (returnCode == TPM_SUCCESS) {
	if (t1TpmTransportInternal->transPublic.transAttributes & TPM_TRANSPORT_EXCLUSIVE) {
	    printf("TPM_Process_EstablishTransport: Session is exclusive\n");
	    tpm_state->tpm_stany_flags.transportExclusive = t1TpmTransportInternal->transHandle;
	}
    }
    /* a. Execution of any command other than TPM_ExecuteTransport or TPM_ReleaseTransportSigned
       targeting this transport session will cause the abnormal invalidation of this transport
       session transHandle */
    /* b. The TPM gives no indication, other than invalidation of transHandle, that the session is
       terminated */
    /* NOTE Done by TPM_Process_Preprocess() */
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_EstablishTransport: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	/* 10. Return T1 -> transHandle as transHandle */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Sbuffer_Append32(response, t1TpmTransportInternal->transHandle);
	}
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return locality */
	    returnCode = TPM_Sbuffer_Append32(response,
					      tpm_state->tpm_stany_flags.localityModifier);
	}
	/* return currentTicks */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_CurrentTicks_Store(response, &currentTicks);
	}
	if (returnCode == TPM_SUCCESS) {
	    /* return transNonceEven */
	    returnCode = TPM_Nonce_Store(response, transNonceEven);
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
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING))) &&
	trans_session_added) {
	TPM_TransportSessions_TerminateHandle(tpm_state->tpm_stclear_data.transSessions,
					      t1TpmTransportInternal->transHandle,
					      &(tpm_state->tpm_stany_flags.transportExclusive));
    }
    /*
      cleanup
    */
    TPM_TransportPublic_Delete(&transPublic);			/* @1 */
    TPM_SizedBuffer_Delete(&secret);				/* @2 */
    TPM_TransportAuth_Delete(&k1TransportAuth);			/* @4 */
    TPM_TransportLogIn_Delete(&l1TransportLogIn);		/* @5 */
    TPM_TransportLogOut_Delete(&l2TransportLogOut);		/* @6 */
    TPM_Sbuffer_Delete(&transPublicSbuffer);			/* @7 */
    TPM_Sbuffer_Delete(&currentTicksSbuffer);			/* @8 */
    return rcf;
}

/* 24.2 TPM_ExecuteTransport rev 117

   Delivers a wrapped TPM command to the TPM where the TPM unwraps the command and then executes the
   command.

   TPM_ExecuteTransport uses the same rolling nonce paradigm as other authorized TPM commands. The
   even nonces start in EstablishTransport and change on each invocation of TPM_ExecuteTransport.

   The only restriction on what can happen inside of a transport session is that there is no
   "nesting" of sessions. It is permissible to perform operations that delete internal state and
   make the TPM inoperable.

   Because, in general, key handles are not logged, a digest of the corresponding public key is 
   logged. In cases where the key handle is logged (e.g. TPM_OwnerReadInternalPub), the 
   public key is also logged.

   The wrapped command is audited twice - once according to the actions of TPM_ExecuteTransport and
   once within the wrapped command itself according to the special rules for auditing a command
   wrapped in an encrypted transport session.
*/

TPM_RESULT TPM_Process_ExecuteTransport(tpm_state_t *tpm_state,
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
    TPM_SIZED_BUFFER	wrappedCmd;		/* The wrapped command */
    TPM_TRANSHANDLE	transHandle;		/* The transport session handle */
    TPM_NONCE		transNonceOdd;		/* Nonce generated by caller */
    TPM_BOOL		continueTransSession;	/* The continue use flag for the authorization
						   session handle */
    TPM_AUTHDATA	transAuth;		/* HMAC for transHandle key: transHandle ->
						   authData */
    
    /* processing parameters */
    /*	   unsigned char *		inParamStart;	/\* starting point of inParam's *\/ */
    /*	   unsigned char *		inParamEnd;	/\* ending point of inParam's *\/ */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transHandleValid = FALSE;
    TPM_TRANSPORT_INTERNAL	*t1TpmTransportInternal;
    TPM_TRANSPORT_INTERNAL	t1TransportCopy;	/* because original might be invalidated */
    TPM_BOOL			transportWrappable;	/* inner command can be wrapped in
							   transport */
    uint32_t			keyHandles;		/* number of key handles in ordw */
    uint32_t			keyHandle1Index; /* index of key handles in wrapped command */
    uint32_t			keyHandle2Index;
    TPM_KEY_HANDLE		keyHandle1;		/* key handles in wrapped command */
    TPM_KEY_HANDLE		keyHandle2;
    uint32_t			blockSize;		/* symmetric key block size, if needed */
    TPM_RESOURCE_TYPE		wrappedResourceType;   	/* for key handle special cases */
    TPM_COMMAND_CODE		ordw;			/* wrapped ORDW */
    uint32_t			e1Dataw;		/* index into wrapped E1 */
    uint32_t			len1;			/* wrapped LEN1 */
    unsigned char		*g1Mgf1;		/* input MGF1 XOR string */
    unsigned char		*g2Mgf1;		/* output MGF1 XOR string */
    unsigned char		*decryptCmd;		/* decrypted wrapped command */
    unsigned char		*cmdStream;		/* temporary for constructing decryptCmd */
    uint32_t			cmdStreamSize;
    TPM_COMMAND_CODE		nOrdw;			/* ordinal in nbo */
    TPM_COMMAND_CODE		nOrdet;			/* ordinal in nbo */
    uint32_t			nWrappedCmdSize;	/* wrappedCmdSize in nbo */
    TPM_DIGEST			h1InWrappedDigest;
    TPM_DIGEST			h2OutWrappedDigest;
    TPM_TRANSPORT_LOG_IN	l2TransportLogIn;
    TPM_TRANSPORT_LOG_OUT	l3TransportLogOut;
    TPM_DIGEST			k2PubkeyDigest;
    TPM_DIGEST			k3PubkeyDigest;
    TPM_KEY			*k2Key;			/* wrapped command keys */
    TPM_KEY			*k3Key;
    TPM_BOOL			parentPCRStatus;
    TPM_STORE_BUFFER		wrappedRspSbuffer;
    const unsigned char		*wrappedRspStream;
    uint32_t			wrappedRspStreamSize;
    uint32_t			s2Dataw;		/* index into S2 */
    uint32_t			len2;			/* length of S2 */
    TPM_RESULT			rcw;			/* wrapped return code */
    TPM_RESULT			nRcw;			/* return code in nbo */
    TPM_STORE_BUFFER		currentTicksSbuffer;
    const unsigned char		*currentTicksBuffer;	/* serialized buffer */
    uint32_t			currentTicksLength;	/* serialization length */
    TPM_RESULT			nRCet;			/* return code in nbo */
    TPM_MODIFIER_INDICATOR	nLocality;		/* locality in nbo */
    uint32_t			nWrappedRspStreamSize;	/* wrappedRspStreamSize in nbo */
    unsigned char		*encryptRsp;		/* encrypted response */
    
    /* output parameters  */
    TPM_DIGEST			outParamDigest;
    TPM_UINT64			currentTicks;	/* The current ticks when the command was
						   executed  */
    TPM_SIZED_BUFFER		wrappedRsp;	/* The wrapped response */

    printf("TPM_Process_ExecuteTransport: Ordinal Entry\n");
    transportInternal = transportInternal;		/* TPM_ExecuteTransport cannot be wrapped */
    TPM_SizedBuffer_Init(&wrappedCmd);			/* freed @1 */
    TPM_SizedBuffer_Init(&wrappedRsp);			/* freed @2 */
    g1Mgf1 = NULL;					/* freed @3 */
    decryptCmd = NULL;					/* freed @4 */
    TPM_TransportLogIn_Init(&l2TransportLogIn);		/* freed @5 */
    TPM_TransportLogOut_Init(&l3TransportLogOut);	/* freed @6 */
    TPM_Sbuffer_Init(&wrappedRspSbuffer);		/* freed @7 */
    TPM_Sbuffer_Init(&currentTicksSbuffer);		/* freed @8 */
    g2Mgf1 = NULL;					/* freed @9 */
    TPM_TransportInternal_Init(&t1TransportCopy);	/* freed @10 */
    encryptRsp = NULL;					/* freed @11 */
    /*
      get inputs
    */
    if (returnCode == TPM_SUCCESS) {
	/* save the starting point of inParam's for authorization and auditing */
	/*	inParamStart = command; */
	/* get wrappedCmd */
	returnCode = TPM_SizedBuffer_Load(&wrappedCmd, &command, &paramSize);
    }	 
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_ExecuteTransport: wrapped command size %u\n", wrappedCmd.size);
	/* save the ending point of inParam's for authorization and auditing */
	/*	inParamEnd = command; */
	/* NOTE: The common TPM_GetInParamDigest() is not called here, since inParamDigest cannot be
	   calculated until the wrapped command is decrypted */
	returnCode = TPM_OrdinalAuditStatus_GetAuditStatus(&auditStatus,
							   ordinal,
							   &(tpm_state->tpm_permanent_data));
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
	returnCode = TPM_AuthParams_Get(&transHandle,
					&transHandleValid,
					transNonceOdd,
					&continueTransSession,
					transAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_ExecuteTransport: transHandle %08x\n", transHandle);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_ExecuteTransport: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	transHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* if there is an active exclusive transport session and it's not this session, terminate it */
    if (returnCode == TPM_SUCCESS) {
	if ((tpm_state->tpm_stany_flags.transportExclusive != 0) &&
	    (tpm_state->tpm_stany_flags.transportExclusive != transHandle)) {
	    returnCode = TPM_TransportSessions_TerminateHandle
			 (tpm_state->tpm_stclear_data.transSessions,
			  tpm_state->tpm_stany_flags.transportExclusive,
			  &(tpm_state->tpm_stany_flags.transportExclusive));
	}
    }
    /* 1. Using transHandle locate the TPM_TRANSPORT_INTERNAL structure T1 */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_TransportSessions_GetEntry(&t1TpmTransportInternal,
						    tpm_state->tpm_stclear_data.transSessions,
						    transHandle);
    }
    /* For the corner case where the wrapped command invalidates the transport session, make a copy
       for the response. */
    if (returnCode == TPM_SUCCESS) {
	TPM_TransportInternal_Copy(&t1TransportCopy,
				   t1TpmTransportInternal);
    }
    /* 2. Parse wrappedCmd */
    /* a. Set TAGw, LENw, and ORDw to the parameters from wrappedCmd */
    /* b. Set E1 to DATAw */
    /* i. This pointer is ordinal dependent and requires the execute transport command to parse
       wrappedCmd */
    /* c. Set LEN1 to the length of DATAw */
    /* i. DATAw always ends at the start of AUTH1w if AUTH1w is present */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_OrdinalTable_ParseWrappedCmd(&e1Dataw,		/* index into wrappedCmd */
						      &len1,
						      &keyHandles,
						      &keyHandle1Index, /* index into key handles */
						      &keyHandle2Index,
						      &ordw,
						      &transportWrappable,
						      &wrappedCmd);
	if (returnCode != TPM_SUCCESS) {
	    printf("TPM_Process_ExecuteTransport: Error parsing wrapped command\n");
	}
    }
    /* 3. If LEN1 is less than 0, or if ORDw is unknown, unimplemented, or cannot be determined
       a. Return TPM_BAD_PARAMETER */
    if (returnCode == TPM_SUCCESS) {
	if (wrappedCmd.size < e1Dataw + len1) {
	    printf("TPM_Process_ExecuteTransport: Error (fatal), wrappedCmdSize %u e1 %u len1 %u\n",
		   wrappedCmd.size, e1Dataw, len1);
	    returnCode = TPM_FAIL;	/* internal error, should never occur */
	}
    }
    /* allocate memory for the decrypted command, which is always the same length as the encrypted
       command */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Malloc(&decryptCmd, wrappedCmd.size);
    }
    /* 4. If T1 -> transPublic -> transAttributes has TPM_TRANSPORT_ENCRYPT set then */
    if ((returnCode == TPM_SUCCESS) &&
	(t1TransportCopy.transPublic.transAttributes & TPM_TRANSPORT_ENCRYPT) &&
	(len1 != 0)) {			/* some commands have no DATAw area to encrypt */
	/* a. If T1 -> transPublic -> algId is TPM_ALG_MGF1 */
	if (t1TransportCopy.transPublic.algId == TPM_ALG_MGF1) {
	    printf("TPM_Process_ExecuteTransport: Wrapped command MGF1 encrypted\n");
	    /* i. Using the MGF1 function, create string G1 of length LEN1. The inputs to the MGF1
	       are transLastNonceEven, transNonceOdd, "in", and T1 -> authData. These four values
	       concatenated together form the Z value that is the seed for the MGF1. */
	    if (returnCode == TPM_SUCCESS) {
		returnCode =
		    TPM_MGF1_GenerateArray(&g1Mgf1,	/* G1 MGF1 array */
					   len1,		/* G1 length */
					   
					   TPM_NONCE_SIZE + TPM_NONCE_SIZE + sizeof("in") - 1 +
					   TPM_AUTHDATA_SIZE,	/* seed length */
					   
					   TPM_NONCE_SIZE, t1TransportCopy.transNonceEven, 
					   TPM_NONCE_SIZE, transNonceOdd, 
					   sizeof("in") - 1, "in", 
					   TPM_AUTHDATA_SIZE, t1TransportCopy.authData, 
					   0, NULL);
	    }
	    /* ii. Create C1 by performing an XOR of G1 and wrappedCmd starting at E1. */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_Transport_CryptMgf1(decryptCmd,	/* output */
						     wrappedCmd.buffer, /* input */
						     g1Mgf1,		/* XOR pad */
						     wrappedCmd.size,	/* total size of buffers */
						     e1Dataw,	/* start of encrypted part */
						     len1);	/* length of encrypted part */
	    }
	}
	/* b. If the encryption algorithm requires an IV or CTR calculate the IV or CTR value */
	else {
	    printf("TPM_Process_ExecuteTransport: "
		   "Wrapped command algId %08x encScheme %04hx encrypted\n",
		   t1TransportCopy.transPublic.algId, t1TransportCopy.transPublic.encScheme);
	    /* This function call should not fail, as the parameters were checked at
	       TPM_EstablishTransport.	The call is used here to get the block size. */
	    if (returnCode == TPM_SUCCESS) {
		returnCode =
		    TPM_TransportPublic_CheckEncScheme(&blockSize,
						       t1TransportCopy.transPublic.algId,
						       t1TransportCopy.transPublic.encScheme,
						       tpm_state->tpm_permanent_flags.FIPS);
	    }
	    /* i. Using the MGF1 function, create string IV1 or CTR1 with a length set by the block
	       size of the encryption algorithm. The inputs to the MGF1 are transLastNonceEven,
	       transNonceOdd, and "in". These three values concatenated together form the Z value
	       that is the seed for the MGF1. Note that any terminating characters within the string
	       "in" are ignored, so a total of 42 bytes are hashed. */
	    if (returnCode == TPM_SUCCESS) {
		returnCode =
		    TPM_MGF1_GenerateArray(&g1Mgf1,	/* G1 MGF1 array */
					   blockSize,	/* G1 length */
					   
					   TPM_NONCE_SIZE + TPM_NONCE_SIZE + sizeof("in") - 1,
					   /* seed length */
					   
					   TPM_NONCE_SIZE, t1TransportCopy.transNonceEven, 
					   TPM_NONCE_SIZE, transNonceOdd, 
					   sizeof("in") - 1, "in", 
					   0, NULL);
	    }
	    /* ii. The symmetric key is taken from the first bytes of T1 -> authData. */
	    /* iii. Decrypt DATAw and replace the DATAw area of E1 creating C1 */
	    if (returnCode == TPM_SUCCESS) {
		returnCode =
		    TPM_Transport_CryptSymmetric(decryptCmd,		/* output */
						 wrappedCmd.buffer,	/* input */
						 t1TransportCopy.transPublic.algId,
						 t1TransportCopy.transPublic.encScheme,
						 t1TransportCopy.authData, /* key */
						 TPM_AUTHDATA_SIZE,	/* key size */
						 g1Mgf1,		/* pad, IV or CTR */
						 blockSize,
						 wrappedCmd.size,	/* total size of buffers */
						 e1Dataw,	/* start of encrypted part */
						 len1);		/* length of encrypted part */
	    }
	}
	/* 4.c. TPM_OSAP, TPM_OIAP have no parameters encrypted */
	/* NOTE Handled by len1 = 0 */
	/* 4.d. TPM_DSAP has special rules for parameter encryption */
	/* NOTE Handled by setting inputHandleSize to all but entityValue */
    }
    /* 5. Else (no encryption) */
    else if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_ExecuteTransport: Wrapped command not encrypted\n");
	/* a. Set C1 to the DATAw area E1 of wrappedCmd */
	memcpy(decryptCmd, wrappedCmd.buffer, wrappedCmd.size);
    }

    /* Now that the wrapped command is decrypted, handle the special cases (e.g., TPM_FlushSpecific
       and TPM_SaveContext) where the handle may or may not be a key handle, dependent on the value
       of resourceType */
    if ((returnCode == TPM_SUCCESS) && (keyHandles == 0xffffffff)) {
	printf("TPM_Process_ExecuteTransport: key handles special case\n");
	
	/* point to the resourceType in the decrypted stream, directly after the key handle */
	cmdStream = decryptCmd + keyHandle1Index + sizeof(TPM_KEY_HANDLE);
	cmdStreamSize = wrappedCmd.size - keyHandle1Index - sizeof(TPM_KEY_HANDLE);
	returnCode = TPM_Load32(&wrappedResourceType, &cmdStream , &cmdStreamSize );
    }
    /* ii. If the resourceType is TPM_RT_KEY, then the public key MUST be logged */
    if ((returnCode == TPM_SUCCESS) && (keyHandles == 0xffffffff)) {
	printf("TPM_Process_ExecuteTransport: special case resource type %08x\n",
	       wrappedResourceType);
	if (wrappedResourceType == TPM_RT_KEY) {
	    printf("TPM_Process_ExecuteTransport: Special case, 1 key handle\n");
	    keyHandles = 1;
	}
	else {
	    keyHandles = 0;
	}
    }
    
    /* 6. Create H1 the SHA-1 of (ORDw || C1).	*/
    /* a. C1 MUST point at the decrypted DATAw area of E1 */
    /* b. The TPM MAY use this calculation for both execute transport authorization, authorization
       of the wrapped command and transport log creation */
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_ExecuteTransport: DATAw decrypted", decryptCmd);
	printf("TPM_Process_ExecuteTransport: Create H1\n");
	nOrdw = htonl(ordw);
	returnCode = TPM_SHA1(h1InWrappedDigest,
			      sizeof(TPM_COMMAND_CODE), &nOrdw,
			      len1, decryptCmd + e1Dataw,
			      0, NULL);
    }
    /* 7. Validate the incoming transport session authorization */
    /* a. Set inParamDigest to SHA-1 (ORDet || wrappedCmdSize || H1) */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_ExecuteTransport: Validate AUTHet\n");
	nOrdet = htonl(ordinal);
	nWrappedCmdSize = htonl(wrappedCmd.size);
	returnCode = TPM_SHA1(inParamDigest,
			      sizeof(TPM_COMMAND_CODE), &nOrdet,
			      sizeof(uint32_t), &nWrappedCmdSize,
			      TPM_DIGEST_SIZE, h1InWrappedDigest,
			      0, NULL);
    }
    /* b. Calculate the HMAC of (inParamDigest || transLastNonceEven || transNonceOdd ||
       continueTransSession) using T1 -> authData as the HMAC key */
    /* c. Validate transAuth, on errors return TPM_AUTHFAIL */
    if (returnCode == TPM_SUCCESS) {
	returnCode =
	    TPM_TransportInternal_Check(inParamDigest,
					&t1TransportCopy,	/* transport session */
					transNonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueTransSession,
					transAuth);		/* Authorization digest for input */
    }
    /* 8. If TPM_ExecuteTransport requires auditing */
    /* a. Create TPM_AUDIT_EVENT_IN using H1 */
    /* NOTE: Done during response */
    /* 9. If ORDw is from the list of following commands return TPM_NO_WRAP_TRANSPORT */
    /* a. TPM_EstablishTransport */
    /* b. TPM_ExecuteTransport */
    /* c. TPM_ReleaseTransportSigned */
    if (returnCode == TPM_SUCCESS) {
	if (!transportWrappable) {
	    printf("TPM_Process_ExecuteTransport: Error, ordinal %08x cannot be wrapped\n",
		   ordw);
	    returnCode = TPM_NO_WRAP_TRANSPORT;
	}
    }
    /* 10. If T1 -> transPublic -> transAttributes has TPM_TRANSPORT_LOG set then */
    if ((returnCode == TPM_SUCCESS) &&
	(t1TransportCopy.transPublic.transAttributes & TPM_TRANSPORT_LOG)) {
	printf("TPM_Process_ExecuteTransport: Create transport log\n");
	/* a. Create L2 a TPM_TRANSPORT_LOG_IN structure */
	/* NOTE Done by TPM_TransportLogIn_Init() */
	/* b. Set L2 -> parameters to H1 */
	TPM_Digest_Copy(l2TransportLogIn.parameters, h1InWrappedDigest);
	/* c. If ORDw is a command with no key handles */
	/* i. Set L2 -> pubKeyHash to NULL */
	/* NOTE Done by TPM_TransportLogIn_Init() */
	if ((returnCode == TPM_SUCCESS) && ((keyHandles == 1) || (keyHandles == 2))) {
	    if (returnCode == TPM_SUCCESS) {
		/* point to the first key handle in the decrypted stream */
		cmdStream = decryptCmd + keyHandle1Index;
		cmdStreamSize = wrappedCmd.size - keyHandle1Index;
		/* get the key handle */
		returnCode = TPM_Load32(&keyHandle1, &cmdStream, &cmdStreamSize);
	    }
	    /* get the first key */
	    if (returnCode == TPM_SUCCESS) {
		printf("TPM_Process_ExecuteTransport: Create pubKeyHash for key 1 handle %08x\n",
		       keyHandle1);
		returnCode = TPM_KeyHandleEntries_GetKey(&k2Key,
							 &parentPCRStatus,
							 tpm_state,
							 keyHandle1,
							 TRUE,		/* read-only */
							 FALSE,		/* do not ignore PCRs */
							 TRUE);		/* can use EK */
	    }
	    /* 10.d. If ORDw is a command with one key handle */
	    /* 10.i. Create K2 the hash of the TPM_STORE_PUBKEY structure of the key pointed to by
	       the key handle. */
	    if (returnCode == TPM_SUCCESS) {
		returnCode =
		    TPM_SHA1_GenerateStructure(k2PubkeyDigest,
					       &(k2Key->pubKey),
					       (TPM_STORE_FUNCTION_T)TPM_SizedBuffer_Store);
	    }
	}
	if ((returnCode == TPM_SUCCESS) && (keyHandles == 1)) {
	    printf("TPM_Process_ExecuteTransport: Digesting one public key\n");
	    /* 10.ii. Set L2 -> pubKeyHash to SHA-1 (K2) */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_SHA1(l2TransportLogIn.pubKeyHash,
				      TPM_DIGEST_SIZE, k2PubkeyDigest,
				      0, NULL);
	    }
	}
	/* 10.e. If ORDw is a command with two key handles */
	if ((returnCode == TPM_SUCCESS) && (keyHandles == 2)) {
	    printf("TPM_Process_ExecuteTransport: Digesting two public keys\n");
	    /* i. Create K2 the hash of the TPM_STORE_PUBKEY structure of the key pointed to by the
	       first key handle. */
	    /* NOTE Done above for 1 or 2 key case */
	    if (returnCode == TPM_SUCCESS) {
		/* point to the second key handle in the decrypted stream */
		cmdStream = decryptCmd + keyHandle2Index;
		cmdStreamSize = wrappedCmd.size - keyHandle2Index;
		/* get the key handle */
		returnCode = TPM_Load32(&keyHandle2, &cmdStream, &cmdStreamSize);
	    }
	    /* get the second key */
	    if (returnCode == TPM_SUCCESS) {
		printf("TPM_Process_ExecuteTransport: Create pubKeyHash for key 2 handle %08x\n",
		       keyHandle2);
		returnCode = TPM_KeyHandleEntries_GetKey(&k3Key,
							 &parentPCRStatus,
							 tpm_state,
							 keyHandle2,
							 TRUE,		/* read-only */
							 FALSE,		/* do not ignore PCRs */
							 TRUE);		/* can use EK */
	    }
	    /* ii. Create K3 the hash of the TPM_STORE_PUBKEY structure of the key pointed to by the
	       second key handle. */
	    if (returnCode == TPM_SUCCESS) {
		returnCode =
		    TPM_SHA1_GenerateStructure(k3PubkeyDigest,
					       &(k3Key->pubKey),
					       (TPM_STORE_FUNCTION_T)TPM_SizedBuffer_Store);
	    }
	    /* 10.iii. Set L2 -> pubKeyHash to SHA-1 (K2 || K3) */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_SHA1(l2TransportLogIn.pubKeyHash,
				      TPM_DIGEST_SIZE, k2PubkeyDigest,
				      TPM_DIGEST_SIZE, k3PubkeyDigest,
				      0, NULL);
	    }
	}
	/* 10.f. Set T1 -> transDigest to the SHA-1 (T1 -> transDigest || L2) */
	if (returnCode == TPM_SUCCESS) {
	    printf("TPM_Process_ExecuteTransport: Extend transDigest with input\n");
	    returnCode = TPM_TransportLogIn_Extend(t1TransportCopy.transDigest,
						   &l2TransportLogIn);
	}
	/* 10.g. If ORDw is a command with key handles, and the key is not loaded, return
	   TPM_INVALID_KEYHANDLE. */
	/* NOTE Done by TPM_KeyHandleEntries_GetKey() */
    }
    /* 11. Send the wrapped command to the normal TPM command parser, the output is C2 and the
	   return code is RCw */
    /* a. If ORDw is a command that is audited then the TPM MUST perform the input and output audit
       of the command as part of this action */
    /* b. The TPM MAY use H1 as the data value in the authorization and audit calculations during
       the execution of C1 */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_ExecuteTransport: Call wrapped command\n");
	returnCode = TPM_Process_Wrapped(&wrappedRspSbuffer,	/* response buffer */
					 decryptCmd,		/* complete command array */
					 wrappedCmd.size,	/* actual bytes in command */
					 tpm_state,
					 &t1TransportCopy);	/* TPM_ExecuteTransport */
	printf("TPM_Process_ExecuteTransport: Completed wrapped command\n");
    }
    /* 12. Set CT1 to TPM_STANY_DATA -> currentTicks -> currentTicks and return CT1 in the
       currentTicks output parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CurrentTicks_Update(&(tpm_state->tpm_stany_data.currentTicks));
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_Uint64_Copy(&currentTicks, &(tpm_state->tpm_stany_data.currentTicks.currentTicks));
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Uint64_Store(&currentTicksSbuffer,
				      &(tpm_state->tpm_stany_data.currentTicks.currentTicks));
    }
    /* 13. Calculate S2 the pointer to the DATAw area of C2 */
    /* a. Calculate LEN2 the length of S2 according to the same rules that calculated LEN1 */
    if (returnCode == TPM_SUCCESS) {
	/* get the response buffer and length */
	TPM_Sbuffer_Get(&wrappedRspSbuffer, &wrappedRspStream, &wrappedRspStreamSize);
	/* parse the three standard input parameters, check paramSize against wrappedRsp->size */
	returnCode = TPM_OrdinalTable_ParseWrappedRsp(&s2Dataw, 
						      &len2,
						      &rcw,
						      ordw,
						      wrappedRspStream,
						      wrappedRspStreamSize);
    }
    /* 14. Create H2 the SHA-1 of (RCw || ORDw || S2) */
    /* a. The TPM MAY use this calculation for execute transport authorization and transport log out
       creation */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_ExecuteTransport: Create H2\n");
	nRcw = htonl(rcw);
	/* The TPM_ExecuteTransport input ordinal and output ordinal, currentTicks and locality are
	   not audited.  This was simply an error, and not a deliberate attempt to make
	   TPM_ExecuteTransport different from other ordinals. */
	returnCode = TPM_SHA1(h2OutWrappedDigest,
			      sizeof(TPM_RESULT), &nRcw,
			      sizeof(TPM_COMMAND_CODE), &nOrdw,
			      len2,  wrappedRspStream + s2Dataw,
			      0, NULL);
    }
    /* 15. Calculate the outgoing transport session authorization */
    /* a. Create the new transNonceEven for the output of the command */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Nonce_Generate(t1TransportCopy.transNonceEven);
    }
    /* b. Set outParamDigest to SHA-1 (RCet || ORDet || TPM_STANY_DATA -> currentTicks ->
       currentTicks || locality || wrappedRspSize || H2) */
    if (returnCode == TPM_SUCCESS) {
	nRCet = htonl(returnCode);
	TPM_Sbuffer_Get(&currentTicksSbuffer, &currentTicksBuffer, &currentTicksLength);
	nLocality = htonl(tpm_state->tpm_stany_flags.localityModifier);
	nWrappedRspStreamSize = htonl(wrappedRspStreamSize);
	returnCode = TPM_SHA1(outParamDigest,
			      sizeof(TPM_RESULT), &nRCet,
			      sizeof(TPM_COMMAND_CODE), &nOrdet,
			      currentTicksLength, currentTicksBuffer,
			      sizeof(TPM_MODIFIER_INDICATOR), &nLocality,
			      sizeof(uint32_t), &nWrappedRspStreamSize,
			      TPM_DIGEST_SIZE, h2OutWrappedDigest,
			      0, NULL);
    }	 
    /* c. Calculate transAuth, the HMAC of (outParamDigest || transNonceEven || transNonceOdd ||
       continueTransSession) using T1 -> authData as the HMAC key */
    /* NOTE: Done as part of response */
    /* 16. If T1 -> transPublic -> transAttributes has TPM_TRANSPORT_LOG set then */
    if ((returnCode == TPM_SUCCESS) &&
	(t1TransportCopy.transPublic.transAttributes & TPM_TRANSPORT_LOG)) {
	/* a. Create L3 a TPM_TRANSPORT_LOG_OUT structure */
	/* NOTE Done by TPM_TransportLogOut_Init() */
	/* b. Set L3 -> parameters to H2 */
	TPM_Digest_Copy(l3TransportLogOut.parameters, h2OutWrappedDigest);
	/* c. Set L3 -> currentTicks to TPM_STANY_DATA -> currentTicks */
	TPM_CurrentTicks_Copy(&(l3TransportLogOut.currentTicks),
			      &(tpm_state->tpm_stany_data.currentTicks));
	/* d. Set L3 -> locality to TPM_STANY_DATA -> localityModifier */
	l3TransportLogOut.locality = tpm_state->tpm_stany_flags.localityModifier;
	/* e. Set T1 -> transDigest to the SHA-1 (T1 -> transDigest || L3) */
	printf("TPM_Process_ExecuteTransport: Extend transDigest with output\n");
	returnCode = TPM_TransportLogOut_Extend(t1TransportCopy.transDigest,
						&l3TransportLogOut);
    }
    /* allocate memory for the encrypted response, which is always the same length as the decrypted
       response */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Malloc(&encryptRsp, wrappedRspStreamSize);
    }
    /* 17. If T1 -> transPublic -> transAttributes has TPM_TRANSPORT_ENCRYPT set then */
    if ((returnCode == TPM_SUCCESS) &&
	(t1TransportCopy.transPublic.transAttributes & TPM_TRANSPORT_ENCRYPT) &&
	(len2 != 0)) {			/* some commands have no DATAw area to encrypt */
	/* NOTE No TPM_OSAP encryption handled by len2 = 0 */
	/* a. If T1 -> transPublic -> algId is TPM_ALG_MGF1 */
	if (t1TransportCopy.transPublic.algId == TPM_ALG_MGF1) {
	    printf("TPM_Process_ExecuteTransport: Wrapped response MGF1 encrypted\n");
	    /* i. Using the MGF1 function, create string G2 of length LEN2. The inputs to the MGF1
	       are transNonceEven, transNonceOdd, "out", and T1 -> authData. These four values
	       concatenated together form the Z value that is the seed for the MGF1. */
	    if (returnCode == TPM_SUCCESS) {
		returnCode =
		    TPM_MGF1_GenerateArray(&g2Mgf1,	/* G2 MGF1 array */
					   len2,		/* G2 length */
					   
					   TPM_NONCE_SIZE + TPM_NONCE_SIZE + sizeof("out") - 1 +
					   TPM_AUTHDATA_SIZE,	/* seed length */
					   
					   TPM_NONCE_SIZE, t1TransportCopy.transNonceEven, 
					   TPM_NONCE_SIZE, transNonceOdd, 
					   sizeof("out") - 1, "out", 
					   TPM_AUTHDATA_SIZE, t1TransportCopy.authData, 
					   0, NULL);
	    }
	    /* ii. Create E2 by performing an XOR of G2 and C2 starting at S2. */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_Transport_CryptMgf1(encryptRsp,
						     wrappedRspStream,
						     g2Mgf1,
						     wrappedRspStreamSize,
						     s2Dataw,
						     len2);
	    }
	}
	/* b. Else */
	else {
	    printf("TPM_Process_ExecuteTransport: "
		   "Wrapped response algId %08x encScheme %04hx encrypted\n",
		   t1TransportCopy.transPublic.algId, t1TransportCopy.transPublic.encScheme);
	    /* This function call should not fail, as the parameters were checked at
	       TPM_EstablishTransport.	The call is used here to get the block size.

	       This is a duplicate of the call for the command.	 However, there are cases where
	       there is no encrypted command (len1 == 0) so the call was not made.  Rather than
	       keep track of whether blockSize is valid, it's clearer to just call the function
	       twice in some cases.
	    */
	    if (returnCode == TPM_SUCCESS) {
		returnCode =
		    TPM_TransportPublic_CheckEncScheme(&blockSize,
						       t1TransportCopy.transPublic.algId,
						       t1TransportCopy.transPublic.encScheme,
						       tpm_state->tpm_permanent_flags.FIPS);
	    }
	    /* i. Create IV2 or CTR2 using the same algorithm as IV1 or CTR1 with the input values
	       transNonceEven, transNonceOdd, and "out". Note that any terminating characters within
	       the string "out" are ignored, so a total of 43 bytes are hashed. */
	    if (returnCode == TPM_SUCCESS) {
		returnCode =
		    TPM_MGF1_GenerateArray(&g2Mgf1,	/* G2 MGF1 array */
					   blockSize,	/* G2 length */
					   
					   TPM_NONCE_SIZE + TPM_NONCE_SIZE + sizeof("out") - 1,
					   /* seed length */
					   
					   TPM_NONCE_SIZE, t1TransportCopy.transNonceEven, 
					   TPM_NONCE_SIZE, transNonceOdd, 
					   sizeof("out") - 1, "out", 
					   0, NULL);
	    }
	    /* ii. The symmetric key is taken from the first bytes of T1 -> authData */
	    /* iii. Create E2 by encrypting C2 starting at S2 */
	    if (returnCode == TPM_SUCCESS) {
		returnCode =
		    TPM_Transport_CryptSymmetric(encryptRsp,		/* output */
						 wrappedRspStream,	/* input */
						 t1TransportCopy.transPublic.algId,
						 t1TransportCopy.transPublic.encScheme,
						 t1TransportCopy.authData, /* key */
						 TPM_AUTHDATA_SIZE,	/* key size */
						 g2Mgf1,		/* pad, IV or CTR */
						 blockSize,
						 wrappedRspStreamSize,	/* total size of buffers */
						 s2Dataw,	/* start of encrypted part */
						 len2);		/* length of encrypted part */
	    }
	}
    }
    /* 18. Else	 (no encryption) */
    else if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_ExecuteTransport: Wrapped response not encrypted\n");
	/* a. Set E2 to the DATAw area S2 of wrappedRsp */
	memcpy(encryptRsp, wrappedRspStream ,wrappedRspStreamSize);
    }
    /* 19. If continueTransSession is FALSE */
    /* a. Invalidate all session data related to transHandle */
    /* NOTE: Done after response */
    /* 20. If TPM_ExecuteTranport requires auditing */
    /* a. Create TPM_AUDIT_EVENT_OUT using H2 */
    /* NOTE: Done during response */
    /* 21. Return C2 but with S2 replaced by E2 in the wrappedRsp parameter */
    if (returnCode == TPM_SUCCESS) {
	/* if the wrapped command invalidated the transport session, set continueTransSession to
	   FALSE */
	if (!(t1TpmTransportInternal->valid)) {
	    continueTransSession = FALSE;
	}
	/* if the session is still valid, copy the copy back to the original so the log gets
	   updated */
	else {
	    TPM_TransportInternal_Copy(t1TpmTransportInternal, &t1TransportCopy);
	}
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_ExecuteTransport: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	/* return currentTicks */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Sbuffer_Append(response, currentTicksBuffer, currentTicksLength);
	}
	/* return locality */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Sbuffer_Append32(response,
					      tpm_state->tpm_stany_flags.localityModifier);
	}
	/* return wrappedRspSize */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Sbuffer_Append32(response, wrappedRspStreamSize);
	}
	/* return wrappedRsp */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Sbuffer_Append(response, encryptRsp, wrappedRspStreamSize);
	}
	/* non-standard - digest the above the line output parameters, H1 used */
	/* non-standard - calculate and set the below the line parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_TransportInternal_Set(response,
						   &t1TransportCopy,
						   outParamDigest,
						   transNonceOdd,
						   continueTransSession,
						   FALSE);    /* transNonceEven already generated */
	}
	/* audit if required */
	if ((returnCode == TPM_SUCCESS) && auditStatus) {
	    returnCode = TPM_ProcessAudit(tpm_state,
					  FALSE,		/* transportEncrypt */
					  h1InWrappedDigest,
					  h2OutWrappedDigest,	/* exception to normal digest */
					  ordinal);
	}
	/* adjust the initial response */
	rcf = TPM_Sbuffer_StoreFinalResponse(response, returnCode, tpm_state);
    }
    /* if there was an error, or continueTransSession is FALSE, terminate the session */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueTransSession) &&
	transHandleValid) {
	TPM_TransportSessions_TerminateHandle(tpm_state->tpm_stclear_data.transSessions,
					      transHandle,
					      &(tpm_state->tpm_stany_flags.transportExclusive));
    }
    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&wrappedCmd);		/* @1 */
    TPM_SizedBuffer_Delete(&wrappedRsp);		/* @2 */
    free(g1Mgf1);					/* @3 */
    free (decryptCmd);					/* @4 */
    TPM_TransportLogIn_Delete(&l2TransportLogIn);	/* @5 */
    TPM_TransportLogOut_Delete(&l3TransportLogOut);	/* @6 */
    TPM_Sbuffer_Delete(&wrappedRspSbuffer);		/* @7 */
    TPM_Sbuffer_Delete(&currentTicksSbuffer);		/* @8 */
    free(g2Mgf1);					/* @9 */
    TPM_TransportInternal_Delete(&t1TransportCopy);	/* @10 */
    free(encryptRsp);					/* @11 */
    return rcf;
}

/* 24.3 TPM_ReleaseTransportSigned rev 101

   This command completes the transport session. If logging for this session is turned on, then this
   command returns a hash of all operations performed during the session along with a digital
   signature of the hash.

   This command serves no purpose if logging is turned off, and results in an error if attempted.

   This command uses two authorization sessions, the key that will sign the log and the
   authorization from the session. Having the session authorization proves that the requester that
   is signing the log is the owner of the session. If this restriction is not put in then an
   attacker can close the log and sign using their own key.

   The hash of the session log includes the information associated with the input phase of execution
   of the TPM_ReleaseTransportSigned command. It cannot include the output phase information.
*/

TPM_RESULT TPM_Process_ReleaseTransportSigned(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE	keyHandle;	/* Handle of a loaded key that will perform the signing */
    TPM_NONCE		antiReplay;	/* Value provided by caller for anti-replay protection */
    TPM_AUTHHANDLE	authHandle;	/* The authorization session to use key */
    TPM_NONCE		authNonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession;	/* The continue use flag for the authorization session
					   handle */
    TPM_AUTHDATA	keyAuth;	/* The authorization session digest that authorizes the use
					   of key. HMAC key: key -> usageAuth  */
    TPM_TRANSHANDLE	transHandle;	/* The transport session handle */
    TPM_NONCE		transNonceOdd;	/* Nonce supplied by caller for transport session */
    TPM_BOOL		continueTransSession = TRUE;	/* The continue use flag for the
							   authorization session handle */
    TPM_AUTHDATA	transAuth;	/* HMAC for transport session key: tranHandle -> authData */
    
    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_BOOL			transHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_TRANSPORT_INTERNAL	*t1TpmTransportInternal = NULL;
    TPM_SECRET			*hmacKey;
    TPM_KEY			*sigKey = NULL;		/* the key specified by keyHandle */
    TPM_BOOL			parentPCRStatus;
    TPM_SECRET			*keyUsageAuth;
    TPM_TRANSPORT_LOG_OUT	a1TransportLogOut;
    TPM_SIGN_INFO		h1SignInfo;
    TPM_DIGEST			h1Digest;	/* digest of h1SignInfo */

    /* output parameters  */
    uint32_t		outParamStart;		/* starting point of outParam's */
    uint32_t		outParamEnd;		/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_CURRENT_TICKS	*currentTicks = NULL;	/* The current time according to the TPM */
    TPM_SIZED_BUFFER	signature;		/* The signature of the digest */

    printf("TPM_Process_ReleaseTransportSigned: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&signature);			/* freed @1 */
    TPM_TransportLogOut_Init(&a1TransportLogOut);	/* freed @2 */
    TPM_SignInfo_Init(&h1SignInfo);			/* freed @3 */
    /*
      get inputs

    */
    /* get keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&keyHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get antiReplay */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_ReleaseTransportSigned: keyHandle %08x\n", keyHandle );
	returnCode = TPM_Nonce_Load(antiReplay, &command, &paramSize);
    }
    /* save the ending point of inParam's for authorization and auditing */
    inParamEnd = command;
    /* digest the input parameters */
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_ReleaseTransportSigned: antiReplay", antiReplay);
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
					authNonceOdd,
					&continueAuthSession,
					keyAuth,
					&command, &paramSize);
	printf("TPM_Process_ReleaseTransportSigned: authHandle %08x\n", authHandle);
    }
    /* get the 'below the line' authorization parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthParams_Get(&transHandle,
					&transHandleValid,
					transNonceOdd,
					&continueTransSession,
					transAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_ReleaseTransportSigned: transHandle %08x\n", transHandle); 
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_ReleaseTransportSigned: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	transHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* if there is an active exclusive transport session and it's not this session, terminate it */
    if (returnCode == TPM_SUCCESS) {
	if ((tpm_state->tpm_stany_flags.transportExclusive != 0) &&
	    (tpm_state->tpm_stany_flags.transportExclusive != transHandle)) {
	    returnCode =
		TPM_TransportSessions_TerminateHandle(tpm_state->tpm_stclear_data.transSessions,
						      tpm_state->tpm_stany_flags.transportExclusive,
						      &(tpm_state->tpm_stany_flags.transportExclusive));
	}
    }
    /* 1. Using transHandle locate the TPM_TRANSPORT_INTERNAL structure T1 */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_TransportSessions_GetEntry(&t1TpmTransportInternal,
						    tpm_state->tpm_stclear_data.transSessions,
						    transHandle);
    }
    /* get the key corresponding to the keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&sigKey, &parentPCRStatus, tpm_state, keyHandle,
						 FALSE,		/* not r/o, used to sign */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    /* 2. Validate that keyHandle -> sigScheme is TPM_SS_RSASSAPKCS1v15_SHA1 or
	TPM_SS_RSASSAPKCS1v15_INFO, if not return TPM_INAPPROPRIATE_SIG. */
    if (returnCode == TPM_SUCCESS) {
	if ((sigKey->algorithmParms.sigScheme != TPM_SS_RSASSAPKCS1v15_SHA1) &&
	    (sigKey->algorithmParms.sigScheme != TPM_SS_RSASSAPKCS1v15_INFO)) {
	    printf("TPM_Process_ReleaseTransportSigned: Error, invalid sigKey sigScheme %04hx\n",
		   sigKey->algorithmParms.sigScheme);
	    returnCode = TPM_INAPPROPRIATE_SIG;
	}
    }
    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_AUTH2_COMMAND)) {
	if (sigKey->authDataUsage != TPM_AUTH_NEVER) {
	    printf("TPM_Process_ReleaseTransportSigned: Error, authorization required\n");
	    returnCode = TPM_AUTHFAIL;
	}
    }
    /* get keyHandle -> usageAuth */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_Key_GetUsageAuth(&keyUsageAuth, sigKey);
    }	 
    /* get the session data */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_NONE,
					      TPM_ET_KEYHANDLE,
					      ordinal,
					      sigKey,
					      keyUsageAuth,		/* OIAP */
					      sigKey->tpm_store_asymkey->pubDataDigest); /* OSAP */
    }
    /* 3. Validate that keyHandle -> keyUsage is TPM_KEY_SIGNING, if not return
	  TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	if (sigKey ->keyUsage != TPM_KEY_SIGNING) {
	    printf("TPM_Process_ReleaseTransportSigned: Error, keyUsage %04hx is invalid\n",
		   sigKey ->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 4. Using key -> authData validate the command and parameters, on error return TPM_AUTHFAIL */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					authNonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					keyAuth);		/* Authorization digest for input */
    }
    /* 5. Using transHandle -> authData validate the command and parameters, on error return
	  TPM_AUTH2FAIL	 */
    if (returnCode == TPM_SUCCESS) {
	returnCode =
	    TPM_TransportInternal_Check(inParamDigest,
					t1TpmTransportInternal, /* transport session */
					transNonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueTransSession,
					transAuth);		/* Authorization digest for input */
    }
    /* 7. Else */
    if (returnCode == TPM_SUCCESS) {
	if (!(t1TpmTransportInternal->transPublic.transAttributes & TPM_TRANSPORT_LOG)) {
	    /* a. Return TPM_BAD_MODE */
	    printf("TPM_Process_ReleaseTransportSigned: Error, TPM_TRANSPORT_LOG not set\n");
	    returnCode = TPM_BAD_MODE;
	}
    }
    /* 6. If T1 -> transAttributes has TPM_TRANSPORT_LOG set then update the current ticks
	  structure */
    if (returnCode == TPM_SUCCESS) {
	currentTicks = &(tpm_state->tpm_stany_data.currentTicks);
	/* update the ticks based on the current time */
	returnCode = TPM_CurrentTicks_Update(currentTicks);
    }
    if (returnCode == TPM_SUCCESS) {
	/* a. Create A1 a TPM_TRANSPORT_LOG_OUT structure */
	/* NOTE Done by TPM_TransportLogOut_Init() */
	/* b. Set A1 -> parameters to the SHA-1 (ordinal || antiReplay) */
	TPM_Digest_Copy(a1TransportLogOut.parameters, inParamDigest);
	/* c. Set A1 -> currentTicks to TPM_STANY_DATA -> currentTicks	*/
	TPM_CurrentTicks_Copy(&(a1TransportLogOut.currentTicks), currentTicks);
	/* d. Set A1 -> locality to the locality modifier for this command */
	a1TransportLogOut.locality = tpm_state->tpm_stany_flags.localityModifier;
	/* e. Set T1 -> transDigest to SHA-1 (T1 -> transDigest || A1) */
	printf("TPM_Process_ReleaseTransportSigned: Extend transDigest with output\n");
	returnCode = TPM_TransportLogOut_Extend(t1TpmTransportInternal->transDigest,
						&a1TransportLogOut);
    }
    if (returnCode == TPM_SUCCESS) {
	/* 8. Create H1 a TPM_SIGN_INFO structure and set the structure defaults */
	/* NOTE: Done by TPM_SignInfo_Init() */
	/* a. Set H1 -> fixed to "TRAN" */
	memcpy(h1SignInfo.fixed, "TRAN", TPM_SIGN_INFO_FIXED_SIZE);
	/* b. Set H1 -> replay to antiReplay */
	TPM_Nonce_Copy(h1SignInfo.replay, antiReplay);
	/* c. Set H1 -> data to T1 -> transDigest */
	returnCode = TPM_SizedBuffer_Set(&(h1SignInfo.data),
					 TPM_DIGEST_SIZE,
					 t1TpmTransportInternal->transDigest);
    }
    /* d. Sign SHA-1 hash of H1 using the key pointed to by keyHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SHA1_GenerateStructure(h1Digest, &h1SignInfo,
						(TPM_STORE_FUNCTION_T)TPM_SignInfo_Store);
	TPM_PrintAll("TPM_Process_ReleaseTransportSigned: h1Digest", h1Digest, TPM_DIGEST_SIZE);
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_RSASignToSizedBuffer(&signature,	/* signature */
					      h1Digest,		/* message */
					      TPM_DIGEST_SIZE,	/* message size */
					      sigKey);		/* input, signing key */
    }
    /* 9. Invalidate all session data related to T1 */
    /* NOTE Done after response */
    /* 10. Set continueTransSession to FALSE */
    if (returnCode == TPM_SUCCESS) {
	continueTransSession = FALSE;
    }
    /* 11. Return TPM_SUCCESS */
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_ReleaseTransportSigned: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return locality */
	    returnCode = TPM_Sbuffer_Append32(response,
					      tpm_state->tpm_stany_flags.localityModifier);
	}
	/* return currentTicks */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_CurrentTicks_Store(response, currentTicks);
	}
	if (returnCode == TPM_SUCCESS) {
	    /* return signature */
	    returnCode = TPM_SizedBuffer_Store(response, &signature);
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
	/* calculate and set the optional below the line parameters */
	if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH2_COMMAND)) {
	    returnCode = TPM_AuthParams_Set(response,
					    *hmacKey,	/* owner HMAC key */
					    auth_session_data,
					    outParamDigest,
					    authNonceOdd,
					    continueAuthSession);
	}
	/* calculate and set the below the line parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_TransportInternal_Set(response,
						   t1TpmTransportInternal,
						   outParamDigest,
						   transNonceOdd,
						   continueTransSession,
						   TRUE);    /* generate transNonceEven */
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
    /* if there was an error, or continueTransSession is FALSE, terminate the session */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueTransSession) &&
	transHandleValid) {
	TPM_TransportSessions_TerminateHandle(tpm_state->tpm_stclear_data.transSessions,
					      transHandle,
					      &(tpm_state->tpm_stany_flags.transportExclusive));
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
    TPM_SizedBuffer_Delete(&signature);			/* @1 */
    TPM_TransportLogOut_Delete(&a1TransportLogOut);	/* @2 */
    TPM_SignInfo_Delete(&h1SignInfo);			/* @3 */
    return rcf;
}
