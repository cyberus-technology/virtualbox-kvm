/********************************************************************************/
/*										*/
/*				Session Handler					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_session.c $		*/
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
#include "tpm_counter.h"
#include "tpm_crypto.h"
#include "tpm_cryptoh.h"
#include "tpm_daa.h"
#include "tpm_debug.h"
#include "tpm_delegate.h"
#include "tpm_digest.h"
#include "tpm_error.h"
#include "tpm_init.h"
#include "tpm_io.h"
#include "tpm_key.h"
#include "tpm_nonce.h"
#include "tpm_nvram.h"
#include "tpm_pcr.h"
#include "tpm_process.h"
#include "tpm_permanent.h"
#include "tpm_secret.h"
#include "tpm_transport.h"
#include "tpm_types.h"

#include "tpm_session.h"

/* local function prototypes */

static TPM_RESULT TPM_OSAPDelegate(TPM_DIGEST **entityDigest,
				   TPM_SECRET **authData,
				   TPM_AUTH_SESSION_DATA *authSession,
				   tpm_state_t *tpm_state,
				   uint32_t delegateRowIndex);

static TPM_RESULT TPM_LoadContext_CheckKeyLoaded(tpm_state_t *tpm_state,
						 TPM_HANDLE entityHandle,
						 TPM_DIGEST entityDigest);
static TPM_RESULT TPM_LoadContext_CheckKeyLoadedByDigest(tpm_state_t *tpm_state,
							 TPM_DIGEST entityDigest);
static TPM_RESULT TPM_LoadContext_CheckOwnerLoaded(tpm_state_t *tpm_state,
						   TPM_DIGEST entityDigest);
static TPM_RESULT TPM_LoadContext_CheckSrkLoaded(tpm_state_t *tpm_state,
						 TPM_DIGEST entityDigest);
static TPM_RESULT TPM_LoadContext_CheckCounterLoaded(tpm_state_t *tpm_state,
						     TPM_HANDLE entityHandle,
						     TPM_DIGEST entityDigest);
static TPM_RESULT TPM_LoadContext_CheckNvLoaded(tpm_state_t *tpm_state,
						TPM_HANDLE entityHandle,
						TPM_DIGEST entityDigest);

/*
  TPM_AUTH_SESSION_DATA (one element of the array)
*/

/* TPM_AuthSessionData_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_AuthSessionData_Init(TPM_AUTH_SESSION_DATA *tpm_auth_session_data)
{
    printf(" TPM_AuthSessionData_Init:\n");
    tpm_auth_session_data->handle = 0;
    tpm_auth_session_data->protocolID = 0;
    tpm_auth_session_data->entityTypeByte = 0;
    tpm_auth_session_data->adipEncScheme = 0;
    TPM_Nonce_Init(tpm_auth_session_data->nonceEven);
    TPM_Secret_Init(tpm_auth_session_data->sharedSecret);
    TPM_Digest_Init(tpm_auth_session_data->entityDigest);
    TPM_DelegatePublic_Init(&(tpm_auth_session_data->pub));
    tpm_auth_session_data->valid = FALSE;
    return;
}

/* TPM_AuthSessionData_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_AuthSessionData_Init()
   After use, call TPM_AuthSessionData_Delete() to free memory
*/

TPM_RESULT TPM_AuthSessionData_Load(TPM_AUTH_SESSION_DATA *tpm_auth_session_data,
				    unsigned char **stream,
				    uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_AuthSessionData_Load:\n");
    /* load handle */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_auth_session_data->handle), stream, stream_size);
    }
    /* load protocolID */
    if (rc == 0) {
	rc = TPM_Load16(&(tpm_auth_session_data->protocolID), stream, stream_size);
    }
    /* load entityTypeByte */
    if (rc == 0) {
	rc = TPM_Loadn(&(tpm_auth_session_data->entityTypeByte), sizeof(BYTE), stream, stream_size);
    }
    /* load adipEncScheme */
    if (rc == 0) {
	rc = TPM_Loadn(&(tpm_auth_session_data->adipEncScheme), sizeof(BYTE), stream, stream_size);
    }
    /* load nonceEven */
    if (rc == 0) {
	rc = TPM_Nonce_Load(tpm_auth_session_data->nonceEven, stream, stream_size);
    }
    /* load sharedSecret */
    if (rc == 0) {
	rc = TPM_Nonce_Load(tpm_auth_session_data->sharedSecret, stream, stream_size);
    }
    /* load entityDigest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_auth_session_data->entityDigest, stream, stream_size);
    }
    /* load pub */
    if (rc == 0) {
	rc = TPM_DelegatePublic_Load(&(tpm_auth_session_data->pub), stream, stream_size);
    }
    /* set valid */
    if (rc == 0) {
	tpm_auth_session_data->valid = TRUE;
    }
    return rc;
}

/* TPM_AuthSessionData_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_AuthSessionData_Store(TPM_STORE_BUFFER *sbuffer,
				     const TPM_AUTH_SESSION_DATA *tpm_auth_session_data)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_AuthSessionData_Store:\n");
    /* store handle */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_auth_session_data->handle);
    }
    /* store protocolID */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, tpm_auth_session_data->protocolID);
    }
    /* store entityTypeByte */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_auth_session_data->entityTypeByte), sizeof(BYTE));
    }
    /* store adipEncScheme */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_auth_session_data->adipEncScheme), sizeof(BYTE));
    }
    /* store nonceEven */
    if (rc == 0) {
	rc = TPM_Nonce_Store(sbuffer, tpm_auth_session_data->nonceEven);
    }
    /* store sharedSecret */
    if (rc == 0) {
	rc = TPM_Nonce_Store(sbuffer, tpm_auth_session_data->sharedSecret);
    }
    /* store entityDigest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_auth_session_data->entityDigest);
    }
    /* store pub */
    if (rc == 0) {
	rc = TPM_DelegatePublic_Store(sbuffer, &(tpm_auth_session_data->pub));
    }
    return rc;
}

/* TPM_AuthSessionData_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_AuthSessionData_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_AuthSessionData_Delete(TPM_AUTH_SESSION_DATA *tpm_auth_session_data)
{
    printf(" TPM_AuthSessionData_Delete:\n");
    if (tpm_auth_session_data != NULL) {
	TPM_DelegatePublic_Delete(&(tpm_auth_session_data->pub));
	TPM_AuthSessionData_Init(tpm_auth_session_data);
    }
    return;
}

/* TPM_AuthSessionData_Copy() copies the source to the destination.  The source handle is ignored,
   since it might already be used.
*/

void TPM_AuthSessionData_Copy(TPM_AUTH_SESSION_DATA *dest_auth_session_data,
			      TPM_HANDLE tpm_handle,
			      TPM_AUTH_SESSION_DATA *src_auth_session_data)
{
    dest_auth_session_data->handle = tpm_handle;
    dest_auth_session_data->protocolID = src_auth_session_data->protocolID;
    dest_auth_session_data->entityTypeByte = src_auth_session_data->entityTypeByte;
    dest_auth_session_data-> adipEncScheme = src_auth_session_data->adipEncScheme;
    TPM_Nonce_Copy(dest_auth_session_data->nonceEven, src_auth_session_data->nonceEven);
    TPM_Secret_Copy(dest_auth_session_data->sharedSecret, src_auth_session_data->sharedSecret);
    TPM_Digest_Copy(dest_auth_session_data->entityDigest, src_auth_session_data->entityDigest);
    TPM_DelegatePublic_Copy(&(dest_auth_session_data->pub), &(src_auth_session_data->pub));
    dest_auth_session_data->valid= src_auth_session_data->valid;
}

/* TPM_AuthSessionData_GetDelegatePublic() */

TPM_RESULT TPM_AuthSessionData_GetDelegatePublic(TPM_DELEGATE_PUBLIC **delegatePublic,	
						 TPM_AUTH_SESSION_DATA *auth_session_data)	
{
    TPM_RESULT	rc = 0;

    printf(" TPM_AuthSessionData_GetDelegatePublic:\n");
    if (rc == 0) {
	*delegatePublic = &(auth_session_data->pub);
    }
    return rc;	 
}

/* TPM_AuthSessionData_CheckEncScheme() checks that the encryption scheme specified by
   TPM_ENTITY_TYPE is supported by the TPM (by TPM_AuthSessionData_Decrypt)
*/

TPM_RESULT TPM_AuthSessionData_CheckEncScheme(TPM_ADIP_ENC_SCHEME adipEncScheme,
					      TPM_BOOL FIPS)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_AuthSessionData_CheckEncScheme: adipEncScheme %02x\n", adipEncScheme);
    switch (adipEncScheme) {
      case TPM_ET_XOR:
	/* i.If TPM_PERMANENT_FLAGS -> FIPS is TRUE */
	/* (1) All encrypted authorizations MUST use a symmetric key encryption scheme. */
	if (FIPS) {
	    rc = TPM_INAPPROPRIATE_ENC;
	}
	break;
      case TPM_ET_AES128_CTR:
	break;
      default:
	printf("TPM_AuthSessionData_CheckEncScheme: Error, unsupported adipEncScheme\n");
	rc = TPM_INAPPROPRIATE_ENC;
	break;
    }
    return rc;	 
}

/* TPM_AuthSessionData_Decrypt() decrypts the encAuth secret using the algorithm indicated in the
   OSAP or DSAP session

   If 'odd' is FALSE, one decrypt of encAuthEven to a1Even.
   If 'odd' is TRUE, a second decrypt of encAuthOdd to a1Odd is also performed.
*/

TPM_RESULT TPM_AuthSessionData_Decrypt(TPM_DIGEST a1Even,
				       TPM_DIGEST a1Odd,
				       TPM_ENCAUTH encAuthEven,
				       TPM_AUTH_SESSION_DATA *tpm_auth_session_data,
				       TPM_NONCE nonceOdd,
				       TPM_ENCAUTH encAuthOdd,
				       TPM_BOOL odd)	
{
    TPM_RESULT	rc = 0;
    TPM_DIGEST	x1Even;
    TPM_DIGEST	x2Odd;

    printf(" TPM_AuthSessionData_Decrypt:\n");
    /* sanity check - the session must be OSAP or DSAP */
    if (rc == 0) {
	if ((tpm_auth_session_data->protocolID != TPM_PID_OSAP) &&
	    (tpm_auth_session_data->protocolID != TPM_PID_DSAP)) {
	    printf("TPM_AuthSessionData_Decrypt: Error, protocolID should be OSAP, is %04hx\n",
		   tpm_auth_session_data->protocolID);
	    rc = TPM_BAD_MODE;
	}
    }
    if (rc == 0) {
	/* algorithm indicated in the OSAP session */
	switch(tpm_auth_session_data->adipEncScheme) {
	  case TPM_ET_XOR:
	    /* 4. If the entity type indicates XOR encryption for the AuthData secret */
	    /* a.Create X1 the SHA-1 of the concatenation of (authHandle -> sharedSecret ||
	       authLastNonceEven). */
	    if (rc == 0) {
		rc = TPM_SHA1(x1Even,
			      TPM_SECRET_SIZE, tpm_auth_session_data->sharedSecret,
			      TPM_NONCE_SIZE, tpm_auth_session_data->nonceEven,
			      0, NULL);
	    }
	    /* b. Create the decrypted AuthData the XOR of X1 and the encrypted AuthData. */
	    if (rc == 0) {
		TPM_Digest_XOR(a1Even, encAuthEven, x1Even);
	    }
	    /* c. If the command ordinal contains a second AuthData2 secret
	       (e.g. TPM_CreateWrapKey) */
	    /* i. Create X2 the SHA-1 of the concatenation of (authHandle -> sharedSecret ||
	       nonceOdd). */
	    if ((rc == 0) && (odd)) {
		rc = TPM_SHA1(x2Odd,
			      TPM_SECRET_SIZE, tpm_auth_session_data->sharedSecret,
			      TPM_NONCE_SIZE, nonceOdd,
			      0, NULL);
	    }
	    /* ii. Create the decrypted AuthData2 the XOR of X2 and the encrypted AuthData2. */
	    if ((rc == 0) && (odd)) {
		TPM_Digest_XOR(a1Odd, encAuthOdd, x2Odd);
	    }
	    break;
#ifdef TPM_AES	/* if AES is supported */
	  case TPM_ET_AES128_CTR:
	    /* 5. If the entity type indicates symmetric key encryption */
	    /* a. The key for the encryption algorithm is the first bytes of the OSAP shared
	       secret. */
	    /* i. E.g., For AES128, the key is the first 16 bytes of the OSAP shared secret. */
	    /* ii. There is no support for AES keys greater than 128 bits. */
	    /* b. If the entity type indicates CTR mode */
	    /* i. The initial counter value for AuthData is the first bytes of authLastNonceEven. */
	    /* (1) E.g., For AES128, the initial counter value is the first 16 bytes of
	       authLastNonceEven. */
	    /* b. Create the decrypted AuthData from the encrypted AuthData. */
	    if (rc == 0) {
		rc = TPM_SymmetricKeyData_CtrCrypt(a1Even,		/* output data */
						   encAuthEven,		/* input data */
						   TPM_AUTHDATA_SIZE,	/* data size */
						   tpm_auth_session_data->sharedSecret, /* key */
						   TPM_SECRET_SIZE,
						   tpm_auth_session_data->nonceEven,	/* CTR */
						   TPM_NONCE_SIZE);
	    }
	    /* ii. If the command ordinal contains a second AuthData2 secret
	       (e.g. TPM_CreateWrapKey) */
	    /* (1) The initial counter value for AuthData2 is the first bytes of
	       nonceOdd. */
	    /* ii. Create the decrypted AuthData2 from the the encrypted AuthData2. */
	    if ((rc == 0) && (odd)) {
		rc = TPM_SymmetricKeyData_CtrCrypt(a1Odd,		/* output data */
						   encAuthOdd,		/* input data */
						   TPM_AUTHDATA_SIZE,	/* data size */
						   tpm_auth_session_data->sharedSecret, /* key */
						   TPM_SECRET_SIZE,
						   nonceOdd,		/* CTR */
						   TPM_NONCE_SIZE);
	    }
	    /* iii. Additional counter values as required are generated by incrementing the
	       entire counter value as a big endian number. */
	    break;
#endif	/* TPM_AES */
	  default:
	    printf("TPM_AuthSessionData_Decrypt: Error, entityType %02x not supported\n",
		   tpm_auth_session_data->adipEncScheme);
	    rc = TPM_INAPPROPRIATE_ENC;
	    break;
	}
    }
    return rc;	 
}

/*
  TPM_AUTH_SESSION_DATA (the entire array)
*/

void TPM_AuthSessions_Init(TPM_AUTH_SESSION_DATA *authSessions)
{
    size_t i;
    
    printf(" TPM_AuthSessions_Init:\n");
    for (i = 0 ; i < TPM_MIN_AUTH_SESSIONS ; i++) {
	TPM_AuthSessionData_Init(&(authSessions[i]));
    }
    return;
}

/* TPM_AuthSessions_Load() reads a count of the number of stored sessions and then loads those
   sessions.

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_AuthSessions_Init()
*/

TPM_RESULT TPM_AuthSessions_Load(TPM_AUTH_SESSION_DATA *authSessions,
				 unsigned char **stream,
				 uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;
    size_t		i;
    uint32_t		activeCount;

    printf(" TPM_AuthSessions_Load:\n");
    /* load active count */
    if (rc == 0) {
	rc = TPM_Load32(&activeCount, stream, stream_size);
    }
    /* load authorization sessions */
    if (rc == 0) {
	if (activeCount > TPM_MIN_AUTH_SESSIONS) {
	    printf("TPM_AuthSessions_Load: Error (fatal) %u sessions, %u slots\n",
		   activeCount, TPM_MIN_AUTH_SESSIONS);
	    rc = TPM_FAIL;
	}
    }    
    if (rc == 0) {
	printf(" TPM_AuthSessions_Load: Loading %u sessions\n", activeCount);
    }
    for (i = 0 ; (rc == 0) && (i < activeCount) ; i++) {
	rc = TPM_AuthSessionData_Load(&(authSessions[i]), stream, stream_size);
    }
    return rc;
}

/* TPM_AuthSessions_Store() stores a count of the active sessions, followed by the sessions.
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_AuthSessions_Store(TPM_STORE_BUFFER *sbuffer,
				  TPM_AUTH_SESSION_DATA *authSessions)
{
    TPM_RESULT		rc = 0;
    size_t		i;
    uint32_t		space;		/* free authorization session slots */
    uint32_t		activeCount;	/* used authorization session slots */
    
    /* store active count */
    if (rc == 0) {
	TPM_AuthSessions_GetSpace(&space, authSessions);
	activeCount = TPM_MIN_AUTH_SESSIONS - space;
	printf(" TPM_AuthSessions_Store: Storing %u sessions\n", activeCount);
	rc = TPM_Sbuffer_Append32(sbuffer, activeCount);
    }
    /* store auth sessions */
    for (i = 0 ; (rc == 0) && (i < TPM_MIN_AUTH_SESSIONS) ; i++) {
	if ((authSessions[i]).valid) {	  /* if the session is active */
	    printf("  TPM_AuthSessions_Store: Storing %08x\n", authSessions[i].handle);
	    rc = TPM_AuthSessionData_Store(sbuffer, &(authSessions[i]));
	}
    }
    return rc;
}

/* TPM_AuthSessions_Delete() terminates all sessions

*/

void TPM_AuthSessions_Delete(TPM_AUTH_SESSION_DATA *authSessions)
{
    size_t i;
    
    printf(" TPM_AuthSessions_Delete:\n");
    for (i = 0 ; i < TPM_MIN_AUTH_SESSIONS ; i++) {
	TPM_AuthSessionData_Delete(&(authSessions[i]));
    }
    return;
}

/* TPM_AuthSessions_IsSpace() returns 'isSpace' TRUE if an entry is available, FALSE if not.

   If TRUE, 'index' holds the first free position.
*/

void TPM_AuthSessions_IsSpace(TPM_BOOL *isSpace,
			      uint32_t *index,
			      TPM_AUTH_SESSION_DATA *authSessions)
{
    printf(" TPM_AuthSessions_IsSpace:\n");
    for (*index = 0, *isSpace = FALSE ; *index < TPM_MIN_AUTH_SESSIONS ; (*index)++) {
	if (!((authSessions[*index]).valid)) {
	    printf("  TPM_AuthSessions_IsSpace: Found space at %u\n", *index);
	    *isSpace = TRUE;
	    break;
	}	    
    }
    return;
}

void TPM_AuthSessions_Trace(TPM_AUTH_SESSION_DATA *authSessions)
{
    size_t i;
    for (i = 0 ; i < TPM_MIN_AUTH_SESSIONS ; i++) {
	if ((authSessions[i]).valid) {
	    printf(" TPM_AuthSessions_Trace: %lu handle %08x\n",
		   (unsigned long)i, authSessions[i].handle);
	}
    }
    return;
}

/* TPM_AuthSessions_GetSpace() returns the number of unused authHandle's.

*/

void TPM_AuthSessions_GetSpace(uint32_t *space,
			       TPM_AUTH_SESSION_DATA *authSessions)
{
    uint32_t i;

    printf(" TPM_AuthSessions_GetSpace:\n");
    for (*space = 0 , i = 0 ; i < TPM_MIN_AUTH_SESSIONS ; i++) {
	if (!((authSessions[i]).valid)) {
	    (*space)++;
	}	    
    }
    return;
}

/* TPM_AuthSessions_StoreHandles() stores

   - the number of loaded sessions
   - a list of session handles
*/

TPM_RESULT TPM_AuthSessions_StoreHandles(TPM_STORE_BUFFER *sbuffer,
					 TPM_AUTH_SESSION_DATA *authSessions)
{
    TPM_RESULT	rc = 0;
    uint16_t	i;
    uint32_t	space;
    
    printf(" TPM_AuthSessions_StoreHandles:\n");
    /* get the number of loaded handles */
    if (rc == 0) {
	TPM_AuthSessions_GetSpace(&space, authSessions);
	/* store loaded handle count.  Cast safe because of TPM_MIN_AUTH_SESSIONS value */
	rc = TPM_Sbuffer_Append16(sbuffer, (uint16_t)(TPM_MIN_AUTH_SESSIONS - space)); 
    }
    for (i = 0 ; (rc == 0) && (i < TPM_MIN_AUTH_SESSIONS) ; i++) {
	if ((authSessions[i]).valid) {			  /* if the index is loaded */
	    rc = TPM_Sbuffer_Append32(sbuffer, (authSessions[i]).handle);	/* store it */
	}
    }
    return rc;
}

/* TPM_AuthSessions_GetNewHandle() checks for space in the authorization sessions table.

   If there is space, it returns a TPM_AUTH_SESSION_DATA entry in 'tpm_auth_session_data' and its
   handle in 'authHandle'.  The entry is marked 'valid'.

   If *authHandle non-zero, the suggested value is tried first.

   Returns TPM_RESOURCES if there is no space in the sessions table.
*/

TPM_RESULT TPM_AuthSessions_GetNewHandle(TPM_AUTH_SESSION_DATA **tpm_auth_session_data,
					 TPM_AUTHHANDLE *authHandle,
					 TPM_AUTH_SESSION_DATA *authSessions)
{
    TPM_RESULT			rc = 0;
    uint32_t			index;
    TPM_BOOL			isSpace;
    
    printf(" TPM_AuthSessions_GetNewHandle:\n");
    /* is there an empty entry, get the location index */
    if (rc == 0) {
	TPM_AuthSessions_IsSpace(&isSpace, &index, authSessions);
	if (!isSpace) {
	    printf("TPM_AuthSessions_GetNewHandle: Error, no space in authSessions table\n");
	    TPM_AuthSessions_Trace(authSessions);
	    rc = TPM_RESOURCES;
	}
    }
    if (rc == 0) {
	rc = TPM_Handle_GenerateHandle(authHandle,		/* I/O */
				       authSessions,		/* handle array */
				       FALSE,			/* keepHandle */
				       FALSE,			/* isKeyHandle */
				       (TPM_GETENTRY_FUNCTION_T)TPM_AuthSessions_GetEntry);
    }
    if (rc == 0) {
	printf("  TPM_AuthSessions_GetNewHandle: Assigned handle %08x\n", *authHandle);
	*tpm_auth_session_data = &(authSessions[index]);
	/* assign the handle */
	(*tpm_auth_session_data)->handle = *authHandle;
	(*tpm_auth_session_data)->valid = TRUE;
    }
    return rc;
}

/* TPM_AuthSessions_GetEntry() searches all entries for the entry matching the handle, and
   returns the TPM_AUTH_SESSION_DATA entry associated with the handle.

   Returns
	0 for success
	TPM_INVALID_AUTHHANDLE if the handle is not found
*/

TPM_RESULT TPM_AuthSessions_GetEntry(TPM_AUTH_SESSION_DATA **tpm_auth_session_data, /* session for
										       authHandle */
				     TPM_AUTH_SESSION_DATA *authSessions, /* points to first session
									     */
				     TPM_AUTHHANDLE authHandle) /* input */
{
    TPM_RESULT	rc = 0;
    size_t	i;
    TPM_BOOL	found;
    
    printf(" TPM_AuthSessions_GetEntry: authHandle %08x\n", authHandle);
    for (i = 0, found = FALSE ; (i < TPM_MIN_AUTH_SESSIONS) && !found ; i++) {
	if ((authSessions[i].valid) &&		    
	    (authSessions[i].handle == authHandle)) {	  /* found */
	    found = TRUE;
	    *tpm_auth_session_data = &(authSessions[i]);
	}
    }
    if (!found) {
	printf("  TPM_AuthSessions_GetEntry: session handle %08x not found\n",
	       authHandle);
	rc = TPM_INVALID_AUTHHANDLE;
    }
    return rc;
}

/* TPM_AuthSessions_AddEntry() adds an TPM_AUTH_SESSION_DATA object to the list.

   If *tpm_handle == 0, a value is assigned.  If *tpm_handle != 0, that value is used if it it not
   currently in use.

   The handle is returned in tpm_handle.
*/

TPM_RESULT TPM_AuthSessions_AddEntry(TPM_HANDLE *tpm_handle,				/* i/o */
				     TPM_BOOL keepHandle,				/* input */
				     TPM_AUTH_SESSION_DATA *authSessions,		/* input */
				     TPM_AUTH_SESSION_DATA *tpm_auth_session_data)	/* input */
{
    TPM_RESULT			rc = 0;
    uint32_t			index;
    TPM_BOOL			isSpace;
    
    printf(" TPM_AuthSessions_AddEntry: handle %08x, keepHandle %u\n",
	   *tpm_handle, keepHandle);
    /* check for valid TPM_AUTH_SESSION_DATA */
    if (rc == 0) {
	if (tpm_auth_session_data == NULL) {	/* NOTE: should never occur */
	    printf("TPM_AuthSessions_AddEntry: Error (fatal), NULL TPM_AUTH_SESSION_DATA\n");
	    rc = TPM_FAIL;
	}
    }
    /* is there an empty entry, get the location index */
    if (rc == 0) {
	TPM_AuthSessions_IsSpace(&isSpace, &index, authSessions);
	if (!isSpace) {
	    printf("TPM_AuthSessions_AddEntry: Error, session entries full\n");
	    TPM_AuthSessions_Trace(authSessions);
	    rc = TPM_RESOURCES;
	}
    }
    if (rc == 0) {
	rc = TPM_Handle_GenerateHandle(tpm_handle,		/* I/O */
				       authSessions,		/* handle array */
				       keepHandle,		/* keepHandle */
				       FALSE,			/* isKeyHandle */
				       (TPM_GETENTRY_FUNCTION_T)TPM_AuthSessions_GetEntry);
    }
    if (rc == 0) {
	TPM_AuthSessionData_Copy(&(authSessions[index]), *tpm_handle, tpm_auth_session_data);
	authSessions[index].valid = TRUE;
	printf("  TPM_AuthSessions_AddEntry: Index %u handle %08x\n",
	       index, authSessions[index].handle);
    }
    return rc;
}

/* TPM_AuthSessions_GetData() checks that authHandle indexes a valid TPM_AUTH_SESSION_DATA object.
   If so, a pointer to the object is returned in tpm_auth_session_data.

   If required protocolID is either TPM_PID_OIAP or TPM_PID_OSAP, the object is checked for that
   type.  TPM_PID_OSAP will accept DSAP as well.  If it is TPM_PID_NONE, either is accepted.  Any
   other value is unsupported.

   If the session protocolID is OIAP, the input entityAuth is echoed back as the HMAC key.
   entityDigest is ignored and may be NULL.

   If the session protocolID is OSAP or DSAP, the function must check that the entity used to set up
   the session is the same as the entity specified in the processing command.  It does that by
   comparing the entityDigest to that saved during setup of the OSAP session.  The shared secret is
   returned as the HMAC key.  entityAuth is ignored and may be NULL.

   If the session protocolID is DSAP, the TPM_DELEGATE_PUBLIC saved during the TPM_DSAP session
   setup is checked for permission and PCR's.  The entityType (TPM_ET_KEYHANDLE or TPM_ET_OWNER) is
   checked against the TPM_DELEGATE_PUBLIC -> TPM_DELEGATIONS delegateType.  Then the bit map is
   fetched from the ordinals table and verified against the per1 or per 2 values.  The pcrInfo is
   checked against the current PCR values.
   
   The saved entityDigest depends upon the entity type:

	TPM_ET_KEYHANDLE:	pubDataDigest
	TPM_ET_OWNER:		ownerAuth
	TPM_ET_SRK:		TPM_KEY -> key_digest
	TPM_ET_COUNTER:		TPM_COUNTER_VALUE -> digest
	TPM_ET_NV:		TPM_NV_DATA_SENSITIVE -> digest
*/

TPM_RESULT TPM_AuthSessions_GetData(TPM_AUTH_SESSION_DATA **tpm_auth_session_data, /* session for
										      authHandle */
				    TPM_SECRET **hmacKey,			/* output */
				    tpm_state_t *tpm_state,	/* input */
				    TPM_AUTHHANDLE authHandle,	/* input */
				    TPM_PROTOCOL_ID protocolID, /* input: required protocol */
				    TPM_ENT_TYPE entityType,	/* input: entity type */
				    TPM_COMMAND_CODE ordinal,	/* input: for delegation */
				    TPM_KEY *tpmKey,	     /* input, for delegate restrictions */
				    TPM_SECRET *entityAuth,	/* input OIAP hmac key */
				    TPM_DIGEST entityDigest)	/* input OSAP session setup auth */
{
    TPM_RESULT	rc = 0;
    TPM_DELEGATE_TABLE_ROW	*delegateTableRow;
    
    printf(" TPM_AuthSessions_GetData: authHandle %08x\n", authHandle);
    if (rc == 0) {
	rc = TPM_AuthSessions_GetEntry(tpm_auth_session_data,
				       tpm_state->tpm_stclear_data.authSessions,
				       authHandle);
	if (rc != 0) {
	    printf("TPM_AuthSessions_GetData: Error, authHandle %08x not found\n", authHandle);
	}
    }
    /* If a specific protocol is required, check that the handle points to the correct session type
       */
    if (rc == 0) {
	switch (protocolID) {	/* what protocol is required */
	  case TPM_PID_NONE:	/* accept any protocol */
	    break;
	  case TPM_PID_OIAP:
	    if ((*tpm_auth_session_data)->protocolID != TPM_PID_OIAP) {
		printf("TPM_AuthSessions_GetData: Error, "
		       "session protocolID should be OIAP, is %04hx\n",
		       (*tpm_auth_session_data)->protocolID);
		rc = TPM_BAD_MODE;
	    }
	    break;
	  case TPM_PID_OSAP:
	    /* Any ordinal requiring OSAP should also accept DSAP */
	    if (((*tpm_auth_session_data)->protocolID != TPM_PID_OSAP) &&
		((*tpm_auth_session_data)->protocolID != TPM_PID_DSAP)) {
		printf("TPM_AuthSessions_GetData: Error, "
		       "session protocolID should be OSAP or DSAP, is %04hx\n",
		       (*tpm_auth_session_data)->protocolID);
		rc = TPM_BAD_MODE;
	    }
	    break;
	  default:	/* should not occur */
	    printf("TPM_AuthSessions_GetData: Error, required protocolID %04hx unsupported\n",
		   protocolID);
	    rc = TPM_BAD_MODE;
	    break;
	}
    }
    /* if the entity is owner auth, verify that an owner is installed */
    if (rc == 0) {
	if (entityType == TPM_ET_OWNER) {
	    if (!tpm_state->tpm_permanent_data.ownerInstalled) {
		printf("TPM_AuthSessions_GetData: Error, no owner installed\n");
		rc = TPM_AUTHFAIL;
	    }
	}
    }
    /* session protocol specific processing */
    if (rc == 0) {
	switch ((*tpm_auth_session_data)->protocolID) {
	  case TPM_PID_OIAP:
	    /* a. If the command using the OIAP session requires owner authorization */
	    /* i. If TPM_STCLEAR_DATA -> ownerReference is TPM_KH_OWNER, the secret AuthData is
	       TPM_PERMANENT_DATA -> ownerAuth */
	    /* ii. If TPM_STCLEAR_DATA -> ownerReference is pointing to a delegate row */
	    if ((entityType == TPM_ET_OWNER) &&
		(tpm_state->tpm_stclear_data.ownerReference != TPM_KH_OWNER)) {
		printf("  TPM_AuthSessions_GetData: Delegating to row %u\n",
		       tpm_state->tpm_stclear_data.ownerReference);
		/* (1) Set R1 a row index to TPM_STCLEAR_DATA -> ownerReference */
		/* (2) Set D1 a TPM_DELEGATE_TABLE_ROW to TPM_PERMANENT_DATA -> delegateTable ->
		       delRow[R1] */
		if (rc == 0) {
		    rc = TPM_DelegateTable_GetValidRow
			 (&delegateTableRow,
			  &(tpm_state->tpm_permanent_data.delegateTable),
			  tpm_state->tpm_stclear_data.ownerReference);
		}
		/* (4) Validate the TPM_DELEGATE_PUBLIC D1 -> pub based on the command ordinal	*/
		/* (a) Validate D1 -> pub -> permissions based on the command ordinal */
		/* (b) Validate D1 -> pub -> pcrInfo based on the PCR values */
		if (rc == 0) {
		    rc = TPM_Delegations_CheckPermission(tpm_state,
							 &(delegateTableRow->pub),
							 entityType,
							 ordinal);
		}
		/* (3) Set the secret AuthData to D1 -> authValue */
		if (rc == 0) {
		    *hmacKey = &(delegateTableRow->authValue);
		}
	    }
	    /* not owner or owner but not delegated */
	    else {
		/* the hmac key is the input authorization secret */
		*hmacKey = entityAuth;
	    }
	    break;
	  case TPM_PID_OSAP:
	  case TPM_PID_DSAP:	/* the first part of DSAP is the same as OSAP */
	    /* ensure that the OSAP shared secret is that derived from the entity using OSAP */
	    if (rc == 0) {
		rc = TPM_Digest_Compare(entityDigest, (*tpm_auth_session_data)->entityDigest);
	    }
	    /* extra processing for DSAP sessions */
	    if ((*tpm_auth_session_data)->protocolID == TPM_PID_DSAP) {
		/* check that delegation is allowed for the ordinal */
		if (rc == 0) {
		    rc = TPM_Delegations_CheckPermission(tpm_state,
							 &((*tpm_auth_session_data)->pub),
							 entityType,	/* required for ordinal */
							 ordinal);
		}
		/* check restrictions on delegation of a certified migration key */
		if ((rc == 0) && (entityType == TPM_ET_KEYHANDLE)) {
		    rc = TPM_Key_CheckRestrictDelegate
			 (tpmKey,
			  tpm_state->tpm_permanent_data.restrictDelegate);
		}
	    }
	    /* the HMAC key is the shared secret calculated during OSAP setup */
	    if (rc == 0) {
		*hmacKey = &((*tpm_auth_session_data)->sharedSecret);
	    }
	    break;
	  default:	/* should not occur */
	    printf("TPM_AuthSessions_GetData: session protocolID %04hx unsupported\n",
		   (*tpm_auth_session_data)->protocolID);
	    rc = TPM_AUTHFAIL;
	    break;
	}
    }
    return rc;
}

/* TPM_AuthSessions_TerminateHandle() terminates the session associated with 'authHandle'.

*/

TPM_RESULT TPM_AuthSessions_TerminateHandle(TPM_AUTH_SESSION_DATA *authSessions,
					    TPM_AUTHHANDLE authHandle)
{
    TPM_RESULT	rc = 0;
    TPM_AUTH_SESSION_DATA *tpm_auth_session_data;

    printf(" TPM_AuthSessions_TerminateHandle: Handle %08x\n", authHandle);
    /* get the TPM_AUTH_SESSION_DATA associated with the TPM_AUTHHANDLE */
    if (rc == 0) {
	rc = TPM_AuthSessions_GetEntry(&tpm_auth_session_data, authSessions, authHandle);
    }
    /* invalidate the valid handle */
    if (rc == 0) {
	TPM_AuthSessionData_Delete(tpm_auth_session_data);
    }
    return rc;
}

/* TPM_AuthSessions_TerminateEntity() terminates all OSAP and DSAP sessions connected to the
   entityType.

   If the session associated with authHandle is terminated, continueAuthSession is set to FALSE for
   the ordinal response.

   If the entityDigest is NULL, all sessions are terminated.  If entityDigest is not NULL, only
   those with a matching entityDigest are terminated.
 */

void TPM_AuthSessions_TerminateEntity(TPM_BOOL *continueAuthSession,
				      TPM_AUTHHANDLE authHandle,
				      TPM_AUTH_SESSION_DATA *authSessions,
				      TPM_ENT_TYPE entityType,
				      TPM_DIGEST *entityDigest)
{
    uint32_t		i;
    TPM_BOOL		terminate;
    TPM_RESULT		match;

    printf(" TPM_AuthSessions_TerminateEntity: entityType %04x\n", entityType);
    for (i = 0 ; i < TPM_MIN_AUTH_SESSIONS ; i++) {
	terminate = FALSE;
	if ((authSessions[i].valid) &&			    /* if the entry is valid */
	    ((authSessions[i].protocolID == TPM_PID_OSAP) ||	/* if it's OSAP or DSAP */
	     (authSessions[i].protocolID == TPM_PID_DSAP)) &&
	    (authSessions[i].entityTypeByte == entityType)) {	/* connected to entity type */
	    /* if entityDigest is NULL, terminate all matching entityType */
	    if (entityDigest == NULL) {
		terminate = TRUE;
	    }
	    /* if entityDigest is not NULL, terminate only those matching entityDigest */
	    else {
		match = TPM_Digest_Compare(*entityDigest, authSessions[i].entityDigest);
		if (match == 0) {
		    terminate = TRUE;
		}
	    }
	}
	if (terminate) {
	    printf("  TPM_AuthSessions_TerminateEntity: Terminating handle %08x\n",
		   authSessions[i].handle);
	    /* if terminating the ordinal's session */
	    if (authSessions[i].handle == authHandle) {
		*continueAuthSession = FALSE;	/* for the ordinal response */
	    }
	    TPM_AuthSessionData_Delete(&authSessions[i]);
	}	    
    }
    return;
}

/* TPM_AuthSessions_TerminatexSAP terminates all OSAP and DSAP sessions

   If the session associated with authHandle is terminated, continueAuthSession is set to FALSE for
   the ordinal response.

   It is safe to call this function during ordinal processing provided a copy of the shared secret
   is first saved for the response HMAC calculation.

   The evenNonce is newly created for the response.  The oddNonce and continueAuthSession are
   command inputs, not part of the session data structure.
*/

void TPM_AuthSessions_TerminatexSAP(TPM_BOOL *continueAuthSession,
				    TPM_AUTHHANDLE authHandle,
				    TPM_AUTH_SESSION_DATA *authSessions)
{
    uint32_t		i;

    printf(" TPM_AuthSessions_TerminatexSAP:\n");
    for (i = 0 ; i < TPM_MIN_AUTH_SESSIONS ; i++) {
	if ((authSessions[i].protocolID == TPM_PID_OSAP) ||
	    (authSessions[i]. protocolID == TPM_PID_DSAP)) {
	    /* if terminating the ordinal's session */
	    if (authSessions[i].handle == authHandle) {
		*continueAuthSession = FALSE;	/* for the ordinal response */
	    }
	    printf("  TPM_AuthSessions_TerminatexSAP: Terminating handle %08x\n",
		   authSessions[i].handle);
	    TPM_AuthSessionData_Delete(&authSessions[i]);
	}	    
    }
    return;
}

/*
  Context List

  Methods to manipulate the TPM_STANY_DATA->contextList[TPM_MAX_SESSION_LIST] array
*/

/* TPM_ContextList_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_ContextList_Init(uint32_t *contextList)
{
    size_t i;
    
    printf(" TPM_ContextList_Init:\n");
    for (i = 0 ; i < TPM_MIN_SESSION_LIST ; i++) {
	contextList[i] = 0;
    }
    return;
}

/* TPM_ContextList_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_ContextList_Init()
*/

TPM_RESULT TPM_ContextList_Load(uint32_t *contextList,
				unsigned char **stream,
				uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;
    size_t		i;
    
    printf(" TPM_ContextList_Load:\n");
    for (i = 0 ; (rc == 0) && (i < TPM_MIN_SESSION_LIST) ; i++) {
	rc = TPM_Load32(&(contextList[i]), stream, stream_size); 
    }
    return rc;
}

/* TPM_ContextList_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_ContextList_Store(TPM_STORE_BUFFER *sbuffer,
				 const uint32_t *contextList)
{
    TPM_RESULT		rc = 0;
    size_t		i;

    printf(" TPM_ContextList_Store: Storing %u contexts\n", TPM_MIN_SESSION_LIST);
    for (i = 0 ; (rc == 0) && (i < TPM_MIN_SESSION_LIST) ; i++) {
	rc = TPM_Sbuffer_Append32(sbuffer, contextList[i]); 
    }
    return rc;
}

/* TPM_ContextList_GetSpace() returns 'space', the number of unused context list entries.

   If 'space' is non-zero, 'entry' points to the first unused index.
*/

void TPM_ContextList_GetSpace(uint32_t *space,
			      uint32_t *entry,
			      const uint32_t *contextList)
{
    uint32_t i;

    printf(" TPM_ContextList_GetSpace:\n");
    for (*space = 0 , i = 0 ; i < TPM_MIN_SESSION_LIST ; i++) {
	if (contextList[i] == 0) {	/* zero values are free space */
	    if (*space == 0) {
		*entry = i;	/* point to the first non-zero entry */
	    }
	    (*space)++;
	}	    
    }
    return;
}

/* TPM_ContextList_GetEntry() gets the entry index corresponding to the value

*/

TPM_RESULT TPM_ContextList_GetEntry(uint32_t *entry,
				    const uint32_t *contextList,
				    uint32_t value)
{
    TPM_RESULT	rc = 0;
    
    printf(" TPM_ContextList_GetEntry:\n");
    if (rc == 0) {
	if (value == 0) {
	    printf("TPM_ContextList_GetEntry: Error, value %d never found\n", value);
	    rc = TPM_BADCONTEXT;
	}
    }
    if (rc == 0) {
	for (*entry = 0 ; *entry < TPM_MIN_SESSION_LIST ; (*entry)++) {
	    if (contextList[*entry] == value) {
		break;
	    }
	}
	if (*entry == TPM_MIN_SESSION_LIST) {
	    printf("TPM_ContextList_GetEntry: Error, value %d not found\n", value);
	    rc = TPM_BADCONTEXT;
	}
    }
    return rc;
}

/* TPM_ContextList_StoreHandles() stores

   - the number of loaded context entries
   - a list of context handles
*/

TPM_RESULT TPM_ContextList_StoreHandles(TPM_STORE_BUFFER *sbuffer,
					const uint32_t *contextList)
{
    TPM_RESULT	rc = 0;
    uint16_t	i;
    uint16_t	loaded;
    
    printf(" TPM_ContextList_StoreHandles:\n");
    if (rc == 0) {
	loaded = 0;
	/* count the number of loaded handles */
	for (i = 0 ; i < TPM_MIN_SESSION_LIST ; i++) {
	    if (contextList[i] != 0) {
		loaded++;
	    }
	}
	/* store 'loaded' handle count */
	rc = TPM_Sbuffer_Append16(sbuffer, loaded); 
    }
    for (i = 0 ; (rc == 0) && (i < TPM_MIN_SESSION_LIST ) ; i++) {
	if (contextList[i] != 0) {	/* if the index is loaded */
	    rc = TPM_Sbuffer_Append32(sbuffer, contextList[i]); /* store it */
	}
    }
    return rc;
}

/*
  TPM_CONTEXT_BLOB
*/

/* TPM_ContextBlob_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_ContextBlob_Init(TPM_CONTEXT_BLOB *tpm_context_blob)
{
    printf(" TPM_ContextBlob_Init:\n");
    tpm_context_blob->resourceType = 0;
    tpm_context_blob->handle = 0;
    memset(tpm_context_blob->label, 0, TPM_CONTEXT_LABEL_SIZE);
    tpm_context_blob->contextCount = 0;
    TPM_Digest_Init(tpm_context_blob->integrityDigest);
    TPM_SizedBuffer_Init(&(tpm_context_blob->additionalData));
    TPM_SizedBuffer_Init(&(tpm_context_blob->sensitiveData));
    return;
}

/* TPM_ContextBlob_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_ContextBlob_Init()
   After use, call TPM_ContextBlob_Delete() to free memory
*/

TPM_RESULT TPM_ContextBlob_Load(TPM_CONTEXT_BLOB *tpm_context_blob,
				unsigned char **stream,
				uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_ContextBlob_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_CONTEXTBLOB, stream, stream_size);
    }
    /* load resourceType */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_context_blob->resourceType), stream, stream_size);
    }
    /* load handle */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_context_blob->handle), stream, stream_size);
    }
    /* load label */
    if (rc == 0) {
	rc = TPM_Loadn(tpm_context_blob->label, TPM_CONTEXT_LABEL_SIZE, stream, stream_size);
    }
    /* load contextCount */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_context_blob->contextCount), stream, stream_size);
    }
    /* load integrityDigest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_context_blob->integrityDigest, stream, stream_size);
    }
    /* load additionalData */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_context_blob->additionalData), stream, stream_size);
    }
    /* load sensitiveData */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_context_blob->sensitiveData), stream, stream_size);
    }
    return rc;
}

/* TPM_ContextBlob_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_ContextBlob_Store(TPM_STORE_BUFFER *sbuffer,
				 const TPM_CONTEXT_BLOB *tpm_context_blob)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_ContextBlob_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_CONTEXTBLOB);
    }
    /* store resourceType */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_context_blob->resourceType);
    }
    /* store handle */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_context_blob->handle);
    }
    /* store label */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, tpm_context_blob->label, TPM_CONTEXT_LABEL_SIZE);
    }
    /* store contextCount */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_context_blob->contextCount);
    }
    /* store integrityDigest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_context_blob->integrityDigest);
    }
    /* store additionalData */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_context_blob->additionalData));
    }
    /* store sensitiveData */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_context_blob->sensitiveData));
    }
    return rc;
}

/* TPM_ContextBlob_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_ContextBlob_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_ContextBlob_Delete(TPM_CONTEXT_BLOB *tpm_context_blob)
{
    printf(" TPM_ContextBlob_Delete:\n");
    if (tpm_context_blob != NULL) {
	TPM_SizedBuffer_Delete(&(tpm_context_blob->additionalData));
	TPM_SizedBuffer_Delete(&(tpm_context_blob->sensitiveData));
	TPM_ContextBlob_Init(tpm_context_blob);
    }
    return;
}

/*
  TPM_CONTEXT_SENSITIVE
*/

/* TPM_ContextSensitive_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_ContextSensitive_Init(TPM_CONTEXT_SENSITIVE *tpm_context_sensitive)
{
    printf(" TPM_ContextSensitive_Init:\n");
    TPM_Nonce_Init(tpm_context_sensitive->contextNonce);
    TPM_SizedBuffer_Init(&(tpm_context_sensitive->internalData));
    return;
}

/* TPM_ContextSensitive_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_ContextSensitive_Init()
   After use, call TPM_ContextSensitive_Delete() to free memory
*/

TPM_RESULT TPM_ContextSensitive_Load(TPM_CONTEXT_SENSITIVE *tpm_context_sensitive,
				     unsigned char **stream,
				     uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_ContextSensitive_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_CONTEXT_SENSITIVE, stream, stream_size);
    }
    /* load contextNonce */
    if (rc == 0) {
	rc = TPM_Nonce_Load(tpm_context_sensitive->contextNonce, stream, stream_size);
    }
    /* load internalData */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_context_sensitive->internalData), stream, stream_size);
    }
    return rc;
}

/* TPM_ContextSensitive_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_ContextSensitive_Store(TPM_STORE_BUFFER *sbuffer,
				      const TPM_CONTEXT_SENSITIVE *tpm_context_sensitive)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_ContextSensitive_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_CONTEXT_SENSITIVE);
    }
    /* store contextNonce */
    if (rc == 0) {
	rc = TPM_Nonce_Store(sbuffer, tpm_context_sensitive->contextNonce);
    }
    /* store internalData */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_context_sensitive->internalData));
    }
    return rc;
}

/* TPM_ContextSensitive_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_ContextSensitive_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_ContextSensitive_Delete(TPM_CONTEXT_SENSITIVE *tpm_context_sensitive)
{
    printf(" TPM_ContextSensitive_Delete:\n");
    if (tpm_context_sensitive != NULL) {
	TPM_SizedBuffer_Delete(&(tpm_context_sensitive->internalData));
	TPM_ContextSensitive_Init(tpm_context_sensitive);
    }
    return;
}

/*
  Processing Functions
*/


/* 18.1 TPM_OIAP rev 87

*/

TPM_RESULT TPM_Process_OIAP(tpm_state_t *tpm_state,
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
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_AUTH_SESSION_DATA	*authSession;		/* the empty structure to be filled */
    TPM_BOOL			got_handle = FALSE;

    /* output parameters */
    uint32_t		outParamStart;			/* starting point of outParam's */
    uint32_t		outParamEnd;			/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_AUTHHANDLE	authHandle = 0; 		/* 0, no suggested value */

    printf("TPM_Process_OIAP: Ordinal Entry\n");
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
	    printf("TPM_Process_OIAP: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. The TPM_OIAP command allows the creation of an authorization session handle and the
	  tracking of the handle by the TPM. The TPM generates the handle and nonce. */
    /* 2. The TPM has an internal limit as to the number of handles that may be open at one time, so
	  the request for a new handle may fail if there is insufficient space available. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_GetNewHandle(&authSession,
						   &authHandle,
						   tpm_state->tpm_stclear_data.authSessions);
    }
    /* 3. Internally the TPM will do the following: */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_OIAP: Using authHandle %08x\n", authHandle);
	got_handle = TRUE;
	/* a. TPM allocates space to save handle, protocol identification, both nonces and any other
	   information the TPM needs to manage the session. */
	authSession->protocolID = TPM_PID_OIAP;
	/* b. TPM generates authHandle and nonceEven, returns these to caller */
	returnCode = TPM_Nonce_Generate(authSession->nonceEven);
    }
    /* 4. On each subsequent use of the OIAP session the TPM MUST generate a new nonceEven value. */
    /* 5. When TPM_OIAP is wrapped in an encrypted transport session no input or output
       parameters encrypted */
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_OIAP: Ordinal returnCode %08x %u\n",
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
	    /* append authHandle */
	    returnCode = TPM_Sbuffer_Append32(response, authHandle);
	}
	/* append nonceEven */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Nonce_Store(response, authSession->nonceEven);
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
    /* if the handle is not being returned, it should be terminated */
    if (((returnCode != 0) || (rcf != 0)) && got_handle) {
	TPM_AuthSessionData_Delete(authSession);
    }
    return rcf;
}

/* 18.2 TPM_OSAP rev 98

   The TPM_OSAP command creates the authorization handle, the shared secret and generates nonceEven
   and nonceEvenOSAP.

   1 The TPM_OSAP command allows the creation of an authorization handle and the tracking of the
   handle by the TPM. The TPM generates the handle, nonceEven and nonceEvenOSAP.

   2. The TPM has an internal limit on the number of handles that may be open at one time, so the
   request for a new handle may fail if there is insufficient space available.

   3. The TPM_OSAP allows the binding of an authorization to a specific entity. This allows the
   caller to continue to send in authorization data for each command but not have to request the
   information or cache the actual authorization data.

   4. When TPM_OSAP is wrapped in an encrypted transport session, no input or output parameters are
      encrypted

   5. If the owner pointer is pointing to a delegate row, the TPM internally MUST treat the OSAP
   session as a DSAP session

   6. TPM_ET_SRK or TPM_ET_KEYHANDLE with a value of TPM_KH_SRK MUST specify the SRK.

   7. If the entity is tied to PCR values, the PCR's are not validated during the TPM_OSAP ordinal
   session creation.  The PCR's are validated when the OSAP session is used.
*/

TPM_RESULT TPM_Process_OSAP(tpm_state_t *tpm_state,
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
    TPM_ENTITY_TYPE	entityType;		/* The type of entity in use */
    uint32_t		entityValue = 0;	/* The selection value based on entityType, e.g. a
						   keyHandle # */
    TPM_NONCE		nonceOddOSAP;		/* The nonce generated by the caller associated with
						   the shared secret. */
    /* processing parameters */
    unsigned char *	inParamStart;		/* starting point of inParam's */
    unsigned char *	inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST		inParamDigest;
    TPM_BOOL		auditStatus;		/* audit the ordinal */
    TPM_BOOL		transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_AUTH_SESSION_DATA *authSession;		/* the empty structure to be filled */
    TPM_BOOL		got_handle = FALSE;
    TPM_SECRET		*authData;		/* usageAuth for the entity */
    TPM_DIGEST		*entityDigest = NULL;	/* digest of the entity establishing the OSAP
						   session, initialize to silence compiler */
    TPM_KEY		*authKey;		/* key to authorize */
    TPM_BOOL		parentPCRStatus;
    TPM_COUNTER_VALUE	*counterValue;			/* associated with entityValue */
    TPM_NV_DATA_SENSITIVE *tpm_nv_data_sensitive;	/* associated with entityValue */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_AUTHHANDLE	authHandle = 0; /* Handle that TPM creates that points to the authorization
					   state. */
    TPM_NONCE		nonceEvenOSAP;	/* Nonce generated by TPM and associated with shared
					   secret. */

    printf("TPM_Process_OSAP: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* save the ending point of inParam's for authorization and auditing */
    inParamEnd = command;
    /* get entityType */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load16(&entityType, &command, &paramSize);
    }
    /* get entityValue */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_OSAP: entityType %04hx\n", entityType);
	returnCode = TPM_Load32(&entityValue, &command, &paramSize);
    }
    /* get nonceOddOSAP */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_OSAP: entityValue %08x\n", entityValue);
	returnCode = TPM_Nonce_Load(nonceOddOSAP, &command, &paramSize);
    }
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
						     TPM_CHECK_OWNER |
						     TPM_CHECK_NO_LOCKOUT));
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_OSAP: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. The TPM creates S1 a storage area that keeps track of the information associated with the
       authorization. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_GetNewHandle(&authSession,
						   &authHandle,
						   tpm_state->tpm_stclear_data.authSessions);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_OSAP: Using authHandle %08x\n", authHandle);
	got_handle = TRUE;
	/* 2. S1 MUST track the following information: */
	/* a. Protocol identification */
	authSession->protocolID = TPM_PID_OSAP; /* save protocol identification */
	authSession->entityTypeByte = entityType & 0x00ff;	/* save entity type LSB */
	/* b. nonceEven */
	/* i. Initialized to the next value from the TPM RNG */
	TPM_Nonce_Generate(authSession->nonceEven);
	/* c. shared secret NOTE: determined below */
	/* d. ADIP encryption scheme from TPM_ENTITY_TYPE entityType */
	authSession->adipEncScheme = (entityType >> 8) & 0x00ff;	/* save entity type MSB */
	/* e. Any other internal TPM state the TPM needs to manage the session */
	/* 3. The TPM MUST create and MAY track the following information */
	/* a. nonceEvenOSAP */
	/* i. Initialized to the next value from the TPM RNG */
	TPM_Nonce_Generate(nonceEvenOSAP);
	/* 4. HMAC, shared secret NOTE: determined below */
	/* 5. Check if the ADIP encryption scheme specified by entityType is supported, if not
	   return TPM_INAPPROPRIATE_ENC. */
	returnCode = TPM_AuthSessionData_CheckEncScheme(authSession->adipEncScheme,
							tpm_state->tpm_permanent_flags.FIPS);
    }
    if (returnCode == TPM_SUCCESS) {
	switch (authSession->entityTypeByte) {
	  case TPM_ET_KEYHANDLE:
	    /* 6. If entityType = TPM_ET_KEYHANDLE */
	    /* a. The entity to authorize is a key held in the TPM. entityValue contains the
	       keyHandle that holds the key. */
	    /* b. If entityValue is TPM_KH_OPERATOR return TPM_BAD_HANDLE */
	    if (returnCode == TPM_SUCCESS) {
		if (entityValue == TPM_KH_OPERATOR) {
		    printf("TPM_Process_OSAP: Error, "
			   "entityType TPM_ET_KEYHANDLE entityValue TPM_KH_OPERATOR\n");
		    returnCode = TPM_BAD_HANDLE;
		}
	    }
	    /* look up and get the TPM_KEY authorization data */
	    if (returnCode == TPM_SUCCESS) {
		/* get the TPM_KEY, entityValue is the handle */
		printf("TPM_Process_OSAP: entityType TPM_ET_KEYHANDLE entityValue %08x\n",
		       entityValue);
		/* TPM_KeyHandleEntries_GetKey() does the mapping from TPM_KH_SRK to the SRK */
		returnCode = TPM_KeyHandleEntries_GetKey(&authKey,
							 &parentPCRStatus,
							 tpm_state,
							 entityValue,
							 TRUE,		/* read only */
							 TRUE,		/* ignore PCRs */
							 FALSE);	/* cannot use EK */
	    }
	    if (returnCode == TPM_SUCCESS) {
		/* get the entityDigest for the key */
		entityDigest = &(authKey->tpm_store_asymkey->pubDataDigest);
		/* get the usageAuth for the key */
		returnCode = TPM_Key_GetUsageAuth(&authData, authKey);
	    }
	    break;
	  case TPM_ET_OWNER:
	    /* 7. else if entityType = TPM_ET_OWNER */ 
	    /* a. This value indicates that the entity is the TPM owner. entityValue is ignored. */
	    /* b. The HMAC key is the secret pointed to by ownerReference (owner secret or delegated
	       secret) */
	    if (returnCode == TPM_SUCCESS) {
		printf("TPM_Process_OSAP: entityType TPM_ET_OWNER, ownerReference %08x\n",
		       tpm_state->tpm_stclear_data.ownerReference);
		/* verify that an owner is installed */
		if (!tpm_state->tpm_permanent_data.ownerInstalled) {
		    printf("TPM_Process_OSAP: Error, no owner\n");
		    returnCode = TPM_BAD_PARAMETER;
		}
	    }
	    if (returnCode == TPM_SUCCESS) {
		/* owner reference is owner, use the owner authorization data */
		if (tpm_state->tpm_stclear_data.ownerReference == TPM_KH_OWNER) {
		    entityDigest = &(tpm_state->tpm_permanent_data.ownerAuth);
		    authData = &(tpm_state->tpm_permanent_data.ownerAuth);
		}
		/* Description 5. If the owner pointer is pointing to a delegate row, the TPM
		   internally MUST treat the OSAP session as a DSAP session */
		else {
		    returnCode = TPM_OSAPDelegate(&entityDigest,
						  &authData,
						  authSession,
						  tpm_state,
						  tpm_state->tpm_stclear_data.ownerReference);
		}
	    }
	    break;
	  case TPM_ET_SRK:
	    /* 8. else if entityType = TPM_ET_SRK */
	    /* a. The entity to authorize is the SRK. entityValue is ignored. */
	    printf("TPM_Process_OSAP: entityType TPM_ET_SRK\n");
	    entityDigest = &(tpm_state->tpm_permanent_data.srk.tpm_store_asymkey->pubDataDigest);
	    returnCode = TPM_Key_GetUsageAuth(&authData, &(tpm_state->tpm_permanent_data.srk));
	    break;
	  case TPM_ET_COUNTER:
	    /* 9. else if entityType = TPM_ET_COUNTER */
	    /* a. The entity is a monotonic counter, entityValue contains the counter handle */
	    if (returnCode == TPM_SUCCESS) {
		printf("TPM_Process_OSAP: entityType TPM_ET_COUNTER entityValue %08x\n",
		       entityValue);
		returnCode =
		    TPM_Counters_GetCounterValue(&counterValue,
						 tpm_state->tpm_permanent_data.monotonicCounter,
						 entityValue);
	    }
	    if (returnCode == TPM_SUCCESS) {
		/* get the entityDigest for the counter */
		entityDigest = &(counterValue->digest);
		/* get the authData for the counter */
		authData = &(counterValue->authData);
	    }
	    break;
	  case TPM_ET_NV:
	    /* 10. else if entityType = TPM_ET_NV 
	       a. The entity is a NV index, entityValue contains the NV index */
	    if (returnCode == TPM_SUCCESS) {
		printf("TPM_Process_OSAP: entityType TPM_ET_NV\n");
		returnCode = TPM_NVIndexEntries_GetEntry(&tpm_nv_data_sensitive,
							 &(tpm_state->tpm_nv_index_entries),
							 entityValue);
	    }
	    if (returnCode == TPM_SUCCESS) {
		/* get the entityDigest for the NV data */
		entityDigest = &(tpm_nv_data_sensitive->digest);
		/* get the authData for the NV data */
		authData = &(tpm_nv_data_sensitive->authValue);
	    }
	    break;
	  default:
	    /* 11. else return TPM_INVALID_PARAMETER */
	    printf("TPM_Process_OSAP: Error, unknown entityType %04x\n", entityType);
	    returnCode = TPM_BAD_PARAMETER;
	    break;
	}
    }
    /* 2.c. shared secret */
    /* 4. The TPM calculates the shared secret using an HMAC calculation. The key for the HMAC
       calculation is the secret AuthData assigned to the key handle identified by entityValue.
       The input to the HMAC calculation is the concatenation of nonces nonceEvenOSAP and
       nonceOddOSAP.  The output of the HMAC calculation is the shared secret which is saved in
       the authorization area associated with authHandle */
    if (returnCode == TPM_SUCCESS) {
	TPM_Digest_Copy(authSession->entityDigest, *entityDigest);
	TPM_PrintFour("TPM_Process_OSAP: entityDigest", *entityDigest);
	TPM_PrintFour("TPM_Process_OSAP: authData", *authData);
	TPM_PrintFour("TPM_Process_OSAP: nonceEvenOSAP", nonceEvenOSAP);
	TPM_PrintFour("TPM_Process_OSAP: nonceOddOSAP", nonceOddOSAP);
	returnCode = TPM_HMAC_Generate(authSession->sharedSecret,
				       *authData,			/* HMAC key */
				       TPM_NONCE_SIZE, nonceEvenOSAP,
				       TPM_NONCE_SIZE, nonceOddOSAP,
				       0, NULL);
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_OSAP: sharedSecret", authSession->sharedSecret);
    }
    /* 12. On each subsequent use of the OSAP session the TPM MUST generate a new nonce value.
       NOTE: Done as the response is generated. */
    /* 13. The TPM MUST ensure that OSAP shared secret is only available while the OSAP session is
       valid.
    */
    /* 14.  The session MUST terminate upon any of the following conditions:
       a.  The command that uses the session returns an error
       NOTE Done by command
       b.  The resource is evicted from the TPM or otherwise invalidated
       NOTE Done by evict or flush
       c.  The session is used in any command for which the shared secret is used to encrypt an
       input parameter (TPM_ENCAUTH)
       NOTE Done by the command
       d.  The TPM Owner is cleared
       NOTE Done by owner clear
       e.  TPM_ChangeAuthOwner is executed and this session is attached to the owner authorization
       NOTE Done by TPM_ChangeAuthOwner 
       f.  The session explicitly terminated with continueAuth, TPM_Reset or TPM_FlushSpecific
       NOTE Done by the ordinal processing
       g. All OSAP sessions associated with the delegation table MUST be invalidated when any of the
       following commands execute:
       i. TPM_Delegate_Manage
       ii. TPM_Delegate_CreateOwnerDelegation with Increment==TRUE
       iii. TPM_Delegate_LoadOwnerDelegation
       NOTE Done by the ordinal processing
    */
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_OSAP: Ordinal returnCode %08x %u\n",
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
	    /* append authHandle */
	    returnCode = TPM_Sbuffer_Append32(response, authHandle);
	}
	/* append nonceEven */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Nonce_Store(response, authSession->nonceEven);
	}
	/* append nonceEvenOSAP */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Nonce_Store(response, nonceEvenOSAP);
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
    /* if the handle is not being returned, it should be terminated */
    if (((returnCode != 0) || (rcf != 0)) && got_handle) {
	TPM_AuthSessionData_Delete(authSession);
    }
    /*
      cleanup
    */
    return rcf;
}

/* 18.3 TPM_DSAP rev 106

   The TPM_DSAP command creates the authorization session handle using a delegated AuthData value
   passed into the command as an encrypted blob or from the internal delegation table. It can be
   used to start an authorization session for a user key or the owner.
  
   As in TPM_OSAP, it generates a shared secret and generates nonceEven and nonceEvenOSAP.

   1. The TPM_DSAP command allows the creation of an authorization session handle and the tracking
   of the handle by the TPM. The TPM generates the handle, nonceEven and nonceEvenOSAP.

   2. The TPM has an internal limit on the number of handles that may be open at one time, so the
   request for a new handle may fail if there is insufficient space available.

   3. The TPM_DSAP allows the binding of a delegated authorization to a specific entity. This allows
   the caller to continue to send in AuthData for each command but not have to request the
   information or cache the actual AuthData.
   
   4. On each subsequent use of the DSAP session the TPM MUST generate a new nonce value and check if
   the ordinal to be executed has delegation to execute. The TPM MUST ensure that the DSAP shared
   secret is only available while the DSAP session is valid.

   5. When TPM_DSAP is wrapped in an encrypted transport session
	a. For input the only parameter encrypted or logged is entityValue
	b. For output no parameters are encrypted or logged

   6. The DSAP session MUST terminate under any of the following conditions

	a. The command that uses the session returns an error
	b. If attached to a key, when the key is evicted from the TPM or otherwise invalidated
	c. The session is used in any command for which the shared secret is used to encrypt an
		input parameter (TPM_ENCAUTH)
	d. The TPM Owner is cleared
	e. TPM_ChangeAuthOwner is executed and this session is attached to the owner
		authorization
	f. The session explicitly terminated with continueAuth, TPM_Reset or TPM_FlushSpecific
	g. All DSAP sessions MUST be invalidated when any of the following commands execute:

		i. TPM_Delegate_CreateOwnerDelegation
			(1) When Increment is TRUE
		ii. TPM_Delegate_LoadOwnerDelegation
		iii. TPM_Delegate_Manage

	NOTE Done by the ordinal processing
		
   entityType = TPM_ET_DEL_OWNER_BLOB
	The entityValue parameter contains a delegation blob structure.
   entityType = TPM_ET_DEL_ROW
	The entityValue parameter contains a row number in the nv Delegation table which should be
	used for the AuthData value.
*/

TPM_RESULT TPM_Process_DSAP(tpm_state_t *tpm_state,
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
    TPM_ENTITY_TYPE	entityType;	/* The type of delegation information to use */
    TPM_KEY_HANDLE	keyHandle = 0;	/* Key for which delegated authority corresponds, or 0 if
					   delegated owner activity.  Only relevant if entityValue
					   equals TPM_DELEGATE_USEKEY_BLOB */
    TPM_NONCE		nonceOddDSAP;	/* The nonce generated by the caller associated with the
					   shared secret. */
    TPM_SIZED_BUFFER	entityValue;	/* TPM_DELEGATE_KEY_BLOB or TPM_DELEGATE_OWNER_BLOB or
					   index. MUST not be empty. If entityType is TPM_ET_DEL_ROW
					   then entityValue is a TPM_DELEGATE_INDEX */
    
    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_AUTH_SESSION_DATA	*authSession;		/* the empty structure to be filled */
    unsigned char		*stream;		/* temp input stream */
    uint32_t			stream_size;
    TPM_DELEGATE_OWNER_BLOB	b1DelegateOwnerBlob;
    TPM_DELEGATE_KEY_BLOB	k1DelegateKeyBlob;
    TPM_KEY			*delKey;		/* key corresponding to keyHandle */
    TPM_BOOL			parentPCRStatus;
    TPM_DELEGATE_SENSITIVE	s1DelegateSensitive;
    uint32_t			delegateRowIndex;
    TPM_DELEGATE_TABLE_ROW	*d1DelegateTableRow;
    TPM_SECRET			*a1AuthValue = NULL;
    TPM_FAMILY_TABLE_ENTRY	*familyRow;		/* family table row containing familyID */

    TPM_BOOL		got_handle = FALSE;

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_AUTHHANDLE	authHandle = 0; /* Handle that TPM creates that points to the authorization
					   state. */
    TPM_NONCE		nonceEvenDSAP;	/* Nonce generated by TPM and associated with shared
					   secret. */

    printf("TPM_Process_DSAP: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&entityValue);			/* freed @1 */
    TPM_DelegateOwnerBlob_Init(&b1DelegateOwnerBlob);	/* freed @2 */
    TPM_DelegateKeyBlob_Init(&k1DelegateKeyBlob);	/* freed @3 */
    TPM_DelegateSensitive_Init(&s1DelegateSensitive);	/* freed @4 */
    /*
      get inputs
    */
    /* get entityType */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load16(&entityType, &command, &paramSize);
    }
    /* get keyHandle */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DSAP: entityType %04hx\n", entityType);
	returnCode = TPM_Load32(&keyHandle, &command, &paramSize);
    }
    /* get nonceOddDSAP */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DSAP: keyHandle %08x\n", keyHandle);
	returnCode = TPM_Nonce_Load(nonceOddDSAP, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command + sizeof(uint32_t);	/* audit entityValue but not entityValueSize */
    /* get entityValue */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&entityValue, &command, &paramSize);
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
						     TPM_CHECK_OWNER |
						     TPM_CHECK_NO_LOCKOUT));
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_DSAP: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    if (returnCode == TPM_SUCCESS) {
	/* use a temporary copy so the original values are not moved */
	stream = entityValue.buffer;
	stream_size = entityValue.size;
	switch (entityType & 0x00ff) {	/* entity type LSB is the actual entity type */
	  case TPM_ET_DEL_OWNER_BLOB:
	    /* 1. If entityType == TPM_ET_DEL_OWNER_BLOB */
	    /* a. Map entityValue to B1 a TPM_DELEGATE_OWNER_BLOB */
	    /* b. Validate that B1 is a valid TPM_DELEGATE_OWNER_BLOB, return TPM_WRONG_ENTITYTYPE
	       on error */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_DelegateOwnerBlob_Load(&b1DelegateOwnerBlob,
							&stream, &stream_size);
		if (returnCode != TPM_SUCCESS) {
		    returnCode = TPM_WRONG_ENTITYTYPE;
		}
	    }
	    /* c. Locate B1 -> pub -> familyID in the TPM_FAMILY_TABLE and set familyRow to
	       indicate row, return TPM_BADINDEX if not found */
	    /* d. Set FR to TPM_FAMILY_TABLE.famTableRow[familyRow] */
	    /* e. If FR -> flags TPM_FAMFLAG_ENABLED is FALSE, return TPM_DISABLED_CMD */
	    if (returnCode == TPM_SUCCESS) {
		returnCode =
		    TPM_FamilyTable_GetEnabledEntry(&familyRow,
						    &(tpm_state->tpm_permanent_data.familyTable),
						    b1DelegateOwnerBlob.pub.familyID);
	    }
	    /* f. Verify that B1->verificationCount equals FR -> verificationCount. */
	    if (returnCode == TPM_SUCCESS) {
		if (b1DelegateOwnerBlob.pub.verificationCount != familyRow->verificationCount) {
		    printf("TPM_Process_DSAP: Error, verificationCount mismatch %u %u\n",
			   b1DelegateOwnerBlob.pub.verificationCount, familyRow->verificationCount);
		    returnCode = TPM_FAMILYCOUNT;
		}
	    }
	    /* g. Validate the integrity of the blob */
	    /* i. Copy B1 -> integrityDigest to H2 */
	    /* ii. Set B1 -> integrityDigest to NULL */
	    /* iii. Create H3 the HMAC of B1 using tpmProof as the secret */
	    /* iv. Compare H2 to H3 return TPM_AUTHFAIL on mismatch */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_HMAC_CheckStructure
			     (tpm_state->tpm_permanent_data.tpmProof,		/* key */
			      &b1DelegateOwnerBlob,				/* structure */
			      b1DelegateOwnerBlob.integrityDigest,		/* expected */
			      (TPM_STORE_FUNCTION_T)TPM_DelegateOwnerBlob_Store,/* store function */
			      TPM_AUTHFAIL);					/* error code */
	    }
	    /* h. Create S1 a TPM_DELEGATE_SENSITIVE by decrypting B1 -> sensitiveArea using
	       TPM_DELEGATE_KEY */
	    /* i. Validate S1 values */
	    /* i. S1 -> tag is TPM_TAG_DELEGATE_SENSITIVE */
	    /* ii. Return TPM_BAD_DELEGATE on error */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_DelegateSensitive_DecryptEncData
			     (&s1DelegateSensitive, 				/* decrypted data */
			      &(b1DelegateOwnerBlob.sensitiveArea), 
			      tpm_state->tpm_permanent_data.delegateKey);
	    }
	    /* j. Set A1 to S1 -> authValue */
	    if (returnCode == TPM_SUCCESS) {
		a1AuthValue = &(s1DelegateSensitive.authValue);
	    }
	    break;
	  case TPM_ET_DEL_ROW:
	    /* 2. Else if entityType == TPM_ET_DEL_ROW */
	    /* a. Verify that entityValue points to a valid row in the delegation table. */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_Load32(&delegateRowIndex, &stream, &stream_size);
	    }
	    /* b. Set D1 to the delegation information in the row. */
	    if (returnCode == TPM_SUCCESS) {
		returnCode =
		    TPM_DelegateTable_GetValidRow(&d1DelegateTableRow,
						  &(tpm_state->tpm_permanent_data.delegateTable),
						  delegateRowIndex);
	    }
	    if (returnCode == TPM_SUCCESS) {
		/* c. Set A1 to D1->authValue. */
		a1AuthValue = &d1DelegateTableRow->authValue;
		/* d. Locate D1 -> familyID in the TPM_FAMILY_TABLE and set familyRow to indicate
		   that row, return TPM_BADINDEX if not found */
		/* e. Set FR to TPM_FAMILY_TABLE.FamTableRow[familyRow] */
		/* f. If FR -> flags TPM_FAMFLAG_ENABLED is FALSE, return TPM_DISABLED_CMD */
		returnCode =
		    TPM_FamilyTable_GetEnabledEntry(&familyRow,
						    &(tpm_state->tpm_permanent_data.familyTable),
						    d1DelegateTableRow->pub.familyID);
	    }
	    /* g. Verify that D1->verificationCount equals FR -> verificationCount. */
	    if (returnCode == TPM_SUCCESS) {
		if (d1DelegateTableRow->pub.verificationCount != familyRow->verificationCount) {
		    printf("TPM_Process_DSAP: Error, verificationCount mismatch %u %u\n",
			   d1DelegateTableRow->pub.verificationCount, familyRow->verificationCount);
		    returnCode = TPM_FAMILYCOUNT;
		}
	    }
	    break;
	  case TPM_ET_DEL_KEY_BLOB:
	    /* 3. Else if entityType == TPM_ET_DEL_KEY_BLOB */
	    /* a. Map entityValue to K1 a TPM_DELEGATE_KEY_BLOB */
	    /* b. Validate that K1 is a valid TPM_DELEGATE_KEY_BLOB, return TPM_WRONG_ENTITYTYPE on
	       error */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_DelegateKeyBlob_Load(&k1DelegateKeyBlob, &stream, &stream_size);
		if (returnCode != TPM_SUCCESS) {
		    returnCode = TPM_WRONG_ENTITYTYPE;
		}
	    }
	    /* c. Locate K1 -> pub -> familyID in the TPM_FAMILY_TABLE and set familyRow to
	       indicate that row, return TPM_BADINDEX if not found */
	    /* d. Set FR to TPM_FAMILY_TABLE.FamTableRow[familyRow] */
	    /* e. If FR -> flags TPM_FAMFLAG_ENABLED is FALSE, return TPM_DISABLED_CMD */
	    if (returnCode == TPM_SUCCESS) {
		returnCode =
		    TPM_FamilyTable_GetEnabledEntry(&familyRow,
						    &(tpm_state->tpm_permanent_data.familyTable),
						    k1DelegateKeyBlob.pub.familyID);
	    }
	    /* f. Verify that K1 -> pub -> verificationCount equals FR -> verificationCount. */
	    if (returnCode == TPM_SUCCESS) {
		if (k1DelegateKeyBlob.pub.verificationCount != familyRow->verificationCount) {
		    printf("TPM_Process_DSAP: Error, verificationCount mismatch %u %u\n",
			   k1DelegateKeyBlob.pub.verificationCount, familyRow->verificationCount);
		    returnCode = TPM_FAMILYCOUNT;
		}
	    }
	    /* g. Validate the integrity of the blob */
	    /* i. Copy K1 -> integrityDigest to H2 */
	    /* ii. Set K1 -> integrityDigest to NULL */
	    /* iii. Create H3 the HMAC of K1 using tpmProof as the secret */
	    /* iv. Compare H2 to H3 return TPM_AUTHFAIL on mismatch */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_HMAC_CheckStructure
			     (tpm_state->tpm_permanent_data.tpmProof,		/* key */
			      &k1DelegateKeyBlob,				/* structure */
			      k1DelegateKeyBlob.integrityDigest,		/* expected */
			      (TPM_STORE_FUNCTION_T)TPM_DelegateKeyBlob_Store,	/* store function */
			      TPM_AUTHFAIL);					/* error code */
	    }
	    /* h. Validate that K1 -> pubKeyDigest identifies keyHandle, return TPM_KEYNOTFOUND on
	       error */
	    /* get the TPM_KEY corresponding to keyHandle */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_KeyHandleEntries_GetKey(&delKey,
							 &parentPCRStatus,
							 tpm_state,
							 keyHandle,
							 TRUE,		/* read only */
							 TRUE,		/* ignore PCRs at setup */
							 FALSE);	/* cannot use EK */
	    }
	    if (returnCode == TPM_SUCCESS) {	
		returnCode = TPM_SHA1_CheckStructure(k1DelegateKeyBlob.pubKeyDigest,
						     &(delKey->pubKey),
						     (TPM_STORE_FUNCTION_T)TPM_SizedBuffer_Store,
						     TPM_KEYNOTFOUND);
	    }
	    /* i. Create S1 a TPM_DELEGATE_SENSITIVE by decrypting K1 -> sensitiveArea using
	       TPM_DELEGATE_KEY */
	    /* j. Validate S1 values */
	    /* i. S1 -> tag is TPM_TAG_DELEGATE_SENSITIVE */
	    /* ii. Return TPM_BAD_DELEGATE on error */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_DelegateSensitive_DecryptEncData
			     (&s1DelegateSensitive,			/* decrypted data */
			      &(k1DelegateKeyBlob.sensitiveArea), 
			      tpm_state->tpm_permanent_data.delegateKey);
	    }
	    /* k. Set A1 to S1 -> authValue */
	    if (returnCode == TPM_SUCCESS) {
		a1AuthValue = &(s1DelegateSensitive.authValue);
	    }
	    break;
	  default:
	    /* 4. Else return TPM_BAD_PARAMETER */
	    printf("TPM_Process_DSAP: Error, bad entityType %04hx\n", entityType);
	    returnCode = TPM_BAD_PARAMETER;
	}
    }
    /* 5. Generate a new authorization session handle and reserve space to save protocol
       identification, shared secret, pcrInfo, both nonces, ADIP encryption scheme, delegated
       permission bits and any other information the TPM needs to manage the session. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_GetNewHandle(&authSession,
						   &authHandle,
						   tpm_state->tpm_stclear_data.authSessions);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DSAP: Using authHandle %08x\n", authHandle);
	got_handle = TRUE;
	/* save protocol identification */
	authSession->protocolID = TPM_PID_DSAP;
	/* save the ADIP encryption scheme */
	authSession->adipEncScheme = (entityType >> 8) & 0x00ff;
	/* NOTE: added: Check if the ADIP encryption scheme specified by entityType is supported, if
	   not return TPM_INAPPROPRIATE_ENC. */
	returnCode = TPM_AuthSessionData_CheckEncScheme(authSession->adipEncScheme,
							tpm_state->tpm_permanent_flags.FIPS);
    }
    if (returnCode == TPM_SUCCESS) {
	if (entityType == TPM_ET_DEL_KEY_BLOB) {
	    /* map the entity type to a key */
	    authSession->entityTypeByte = TPM_ET_KEYHANDLE;
	    /* Save the entityDigest for comparison during use.	 */
	    TPM_Digest_Copy(authSession->entityDigest, delKey->tpm_store_asymkey->pubDataDigest);
	    /* Save the TPM_DELEGATE_PUBLIC to check the permissions and pcrInfo at DSAP session
	       use. */
	    returnCode =TPM_DelegatePublic_Copy(&(authSession->pub), &(k1DelegateKeyBlob.pub));
	}
	else {
	    /* owner or blob or delegate row are both owner auth */
	    authSession->entityTypeByte = TPM_ET_OWNER;
	    /* Save the entityDigest for comparison during use.	 */
	    TPM_Digest_Copy(authSession->entityDigest, tpm_state->tpm_permanent_data.ownerAuth);
	    /* Save the TPM_DELEGATE_PUBLIC to check the permissions and pcrInfo at DSAP session
	       use. */
	    if (entityType == TPM_ET_DEL_OWNER_BLOB) {
		returnCode = TPM_DelegatePublic_Copy(&(authSession->pub),
						     &(b1DelegateOwnerBlob.pub));
	    }
	    else {	/* TPM_ET_DEL_ROW */
		returnCode = TPM_DelegatePublic_Copy(&(authSession->pub),
						     &(d1DelegateTableRow->pub));
	    }
	}
	/* 6. Read two new values from the RNG to generate nonceEven and nonceEvenOSAP. */
	TPM_Nonce_Generate(authSession->nonceEven);
	TPM_Nonce_Generate(nonceEvenDSAP);
    }
    /* 7. The TPM calculates the shared secret using an HMAC calculation. The key for the HMAC
       calculation is A1.  The input to the HMAC calculation is the concatenation of nonces
       nonceEvenOSAP and nonceOddOSAP.	The output of the HMAC calculation is the shared secret
       which is saved in the authorization area associated with authHandle. */
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_DSAP: authData", *a1AuthValue);
	TPM_PrintFour("TPM_Process_DSAP: nonceEvenOSAP", nonceEvenDSAP);
	TPM_PrintFour("TPM_Process_DSAP: nonceOddOSAP", nonceOddDSAP);
	returnCode = TPM_HMAC_Generate(authSession->sharedSecret,
				       *a1AuthValue,			/* HMAC key */
				       TPM_NONCE_SIZE, nonceEvenDSAP,
				       TPM_NONCE_SIZE, nonceOddDSAP,
				       0, NULL);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_DSAP: Ordinal returnCode %08x %u\n",
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
	    /* append authHandle */
	    returnCode = TPM_Sbuffer_Append32(response, authHandle);
	}
	/* append nonceEven */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Nonce_Store(response, authSession->nonceEven);
	}
	/* append nonceEvenDSAP */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Nonce_Store(response, nonceEvenDSAP);
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
    /* if the handle is not being returned, it should be terminated */
    if (((returnCode != 0) || (rcf != 0)) && got_handle) {
	TPM_AuthSessionData_Delete(authSession);
    }
    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&entityValue);		/* @1 */
    TPM_DelegateOwnerBlob_Delete(&b1DelegateOwnerBlob); /* @2 */
    TPM_DelegateKeyBlob_Delete(&k1DelegateKeyBlob);	/* @3 */
    TPM_DelegateSensitive_Delete(&s1DelegateSensitive); /* @4 */
    return rcf;
}

/* TPM_DSAPDelegate() implements the actions common to TPM_DSAP and TPM_OSAP with
   ownerReference pointing to a delegate row.

   'entityDigest' and 'authData' are returned, as they are used by common code.
   authSession.

   protocolID is changed to DSAP.
   the TPM_DELEGATE_PUBLIC blob is copied to the OSAP/DSAP session structure.
*/

static TPM_RESULT TPM_OSAPDelegate(TPM_DIGEST **entityDigest,
				   TPM_SECRET **authData,
				   TPM_AUTH_SESSION_DATA *authSession,
				   tpm_state_t *tpm_state,
				   uint32_t delegateRowIndex)
{
    TPM_RESULT			rc = 0;
    TPM_DELEGATE_TABLE_ROW	*d1DelegateTableRow;
    TPM_FAMILY_TABLE_ENTRY	*familyRow;		/* family table row containing familyID */

    printf("TPM_DSAPCommon: Index %u\n", delegateRowIndex);
    /* 2. Else if entityType == TPM_ET_DEL_ROW */
    /* a. Verify that entityValue points to a valid row in the delegation table. */
    /* b. Set d1 to the delegation information in the row. */
    if (rc == TPM_SUCCESS) {
	rc = TPM_DelegateTable_GetValidRow(&d1DelegateTableRow,
					   &(tpm_state->tpm_permanent_data.delegateTable),
					   delegateRowIndex);
    }
    if (rc == TPM_SUCCESS) {
	/* d. Locate D1 -> familyID in the TPM_FAMILY_TABLE and set familyRow to indicate that
	   row, return TPM_BADINDEX if not found */
	/* e. Set FR to TPM_FAMILY_TABLE.FamTableRow[familyRow] */
	/* f. If FR -> flags TPM_FAMFLAG_ENABLED is FALSE, return TPM_DISABLED_CMD */
	rc = TPM_FamilyTable_GetEnabledEntry(&familyRow,
					     &(tpm_state->tpm_permanent_data.familyTable),
					     d1DelegateTableRow->pub.familyID);
    }
    /* g. Verify that d1->verificationCount equals FR -> verificationCount. */
    if (rc == TPM_SUCCESS) {
	if (d1DelegateTableRow->pub.verificationCount != familyRow->verificationCount) {
	    printf("TPM_DSAPCommon: Error, verificationCount mismatch %u %u\n",
		   d1DelegateTableRow->pub.verificationCount, familyRow->verificationCount);
	    rc = TPM_FAMILYCOUNT;
	}
    }
    if (rc == TPM_SUCCESS) {
	/* c. Set a1 to d1->authValue. */
	*authData = &d1DelegateTableRow->authValue;	/* use owner delegate authorization value */
	/* indicate later that the entity is the 'owner'.  Use the real owner auth because the
	   ordinal doesn't know about the delegation */
	*entityDigest = &(tpm_state->tpm_permanent_data.ownerAuth);
	authSession->protocolID = TPM_PID_DSAP;		/* change from OSAP to DSAP */
	/* Save the TPM_DELEGATE_PUBLIC to check the permissions and pcrInfo at DSAP session
	   use. */
	rc = TPM_DelegatePublic_Copy(&(authSession->pub),
				     &(d1DelegateTableRow->pub));
    }
    return rc;
}

/* 18.4 TPM_SetOwnerPointer rev 109

   This command will set a reference to which secret the TPM will use when executing an owner secret
   related OIAP or OSAP session.

   This command should only be used to provide an owner delegation function for legacy code that
   does not itself support delegation. Normally, TPM_STCLEAR_DATA->ownerReference points to
   TPM_KH_OWNER, indicating that OIAP and OSAP sessions should use the owner authorization.  This
   command allows ownerReference to point to an index in the delegation table, indicating that
   OIAP and OSAP sessions should use the delegation authorization.

   In use, a TSS supporting delegation would create and load the owner delegation and set the owner
   pointer to that delegation.	From then on, a legacy TSS application would use its OIAP and OSAP
   sessions with the delegated owner authorization.

   Since this command is not authorized, the ownerReference is open to DoS attacks. Applications can
   attempt to recover from a failing owner authorization by resetting ownerReference to an
   appropriate value.

   This command intentionally does not clear OSAP sessions.  A TPM 1.1 application gets the benefit
   of owner delegation, while the original owner can use a pre-existing OSAP session with the actual
   owner authorization.
*/

TPM_RESULT TPM_Process_SetOwnerPointer(tpm_state_t *tpm_state,
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
    TPM_ENTITY_TYPE	entityType;		/* The type of entity in use */
    uint32_t		entityValue = 0;	/* The selection value based on entityType */
    
    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_STCLEAR_DATA		*v1StClearData;
    TPM_DELEGATE_TABLE_ROW	*b1DelegateTableRow;	/* delegate row indicated by entityValue */
    TPM_FAMILY_TABLE_ENTRY	*familyRow;		/* family table row containing familyID */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_SetOwnerPointer: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get entityType */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load16(&entityType, &command, &paramSize);
    }
    /* get entityValue */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_SetOwnerPointer: entityType %04hx\n", entityType);
	returnCode = TPM_Load32(&entityValue, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_SetOwnerPointer: entityValue %08x\n", entityValue);
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
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_SetOwnerPointer: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. Map TPM_STCLEAR_DATA to V1 */
    if (returnCode == TPM_SUCCESS) {
	v1StClearData = &(tpm_state->tpm_stclear_data); 
	/* 2. If entityType = TPM_ET_DEL_ROW */
	if (entityType == TPM_ET_DEL_ROW) {
	    /* a. This value indicates that the entity is a delegate row. entityValue is a delegate
	       index in the delegation table.  */
	    /* b. Validate that entityValue points to a legal row within the delegate table stored
	       within the TPM. If not return TPM_BADINDEX */
	    /* i. Set D1 to the delegation information in the row. */
	    if (returnCode == TPM_SUCCESS) {
		returnCode =
		    TPM_DelegateTable_GetValidRow(&b1DelegateTableRow,
						  &(tpm_state->tpm_permanent_data.delegateTable),
						  entityValue);
		
	    }
	    /* c. Locate D1 -> familyID in the TPM_FAMILY_TABLE and set familyRow to indicate that
	       row, return TPM_BADINDEX if not found. */
	    /* d. Set FR to TPM_FAMILY_TABLE.famTableRow[familyRow] */
	    /* e. If FR -> flags TPM_FAMFLAG_ENABLED is FALSE, return TPM_DISABLED_CMD */
	    if (returnCode == TPM_SUCCESS) {
		returnCode =
		    TPM_FamilyTable_GetEnabledEntry(&familyRow,
						    &(tpm_state->tpm_permanent_data.familyTable),
						    b1DelegateTableRow->pub.familyID);
	    }
	    /* f. Verify that B1->verificationCount equals FR -> verificationCount. */
	    if (returnCode == TPM_SUCCESS) {
		if (b1DelegateTableRow->pub.verificationCount != familyRow->verificationCount) {
		    printf("TPM_Process_SetOwnerPointer: Error, "
			   "verificationCount mismatch %u %u\n",
			   b1DelegateTableRow->pub.verificationCount, familyRow->verificationCount);
		    returnCode = TPM_FAMILYCOUNT;
		}
	    }
	    /* g. The TPM sets V1-> ownerReference to entityValue */
	    /* h. Return TPM_SUCCESS */
	    if (returnCode == TPM_SUCCESS) {
		printf("TPM_Process_SetOwnerPointer: Setting ownerReference to %08x\n",
		       entityValue);
		v1StClearData->ownerReference = entityValue;
	    }
	}
	/* 3. else if entityType = TPM_ET_OWNER */
	else if (entityType == TPM_ET_OWNER) {
	    /* a. This value indicates that the entity is the TPM owner. entityValue is ignored.  */
	    /* b. The TPM sets V1-> ownerReference to TPM_KH_OWNER */
	    /* c. Return TPM_SUCCESS */
	    printf("TPM_Process_SetOwnerPointer: Setting ownerReference to %08x\n", TPM_KH_OWNER);
	    v1StClearData->ownerReference = TPM_KH_OWNER;
	}
	/* 4. Return TPM_BAD_PARAMETER */
	else {
	    printf("TPM_Process_SetOwnerPointer: Error, bad entityType\n");
	    returnCode = TPM_BAD_PARAMETER;
	}
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_SetOwnerPointer: Ordinal returnCode %08x %u\n",
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

/* 27.1.2 TPM_Terminate_Handle rev 87

   This allows the TPM manager to clear out information in a session handle.  

   The TPM may maintain the authorization session even though a key attached to it has been unloaded
   or the authorization session itself has been unloaded in some way. When a command is executed
   that requires this session, it is the responsibility of the external software to load both the
   entity and the authorization session information prior to command execution.

   The TPM SHALL terminate the session and destroy all data associated with the session indicated.
*/

TPM_RESULT TPM_Process_TerminateHandle(tpm_state_t *tpm_state,
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
    TPM_AUTHHANDLE authHandle;

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

    printf("TPM_Process_TerminateHandle: Ordinal Entry\n");
    /*
      get inputs
    */
    /* get handle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&authHandle, &command, &paramSize);
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
	returnCode = TPM_CheckState(tpm_state, tag, (TPM_CHECK_NOT_SHUTDOWN |
						     TPM_CHECK_NO_LOCKOUT));
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_TerminateHandle: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* terminate the handle */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_TerminateHandle: Using authHandle %08x\n", authHandle);
	returnCode = TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions,
						      authHandle);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_TerminateHandle: Ordinal returnCode %08x %u\n",
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

/* 22.1 TPM_FlushSpecific rev 104
	 
   TPM_FlushSpecific flushes from the TPM a specific handle.

   TPM_FlushSpecific releases the resources associated with the given handle.
*/

TPM_RESULT TPM_Process_FlushSpecific(tpm_state_t *tpm_state,
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
    TPM_HANDLE		handle;			/* The handle of the item to flush */
    TPM_RESOURCE_TYPE	resourceType = 0;	/*  The type of resource that is being flushed */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    uint32_t			r1Resource;		/* the context resource being flushed */
    TPM_KEY_HANDLE_ENTRY	*tpm_key_handle_entry;	/* key table entry for the handle */
    
    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    
    printf("TPM_Process_FlushSpecific: Ordinal Entry\n");
    /*
      get inputs
    */
    /* get handle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&handle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get resourceType parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_FlushSpecific: Handle %08x\n", handle);
	returnCode = TPM_Load32(&resourceType, &command, &paramSize);
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
	    printf("TPM_Process_FlushSpecific: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    if (returnCode == TPM_SUCCESS) {
	switch (resourceType) {
	  case TPM_RT_CONTEXT:
	    /* 1. If resourceType is TPM_RT_CONTEXT */
	    if (returnCode == TPM_SUCCESS) {
		printf("TPM_Process_FlushSpecific: Flushing context count %08x\n", handle);
		/* a. The handle for a context is not a handle but the "context count" value. The
		   TPM uses the "context count" value to locate the proper contextList entry and
		   sets R1 to the contextList entry */
		returnCode = TPM_ContextList_GetEntry(&r1Resource,	/* index into
									   contextList[] */
						      tpm_state->tpm_stclear_data.contextList,
						      handle);
		/* 7. Validate that R1 determined by resourceType and handle points to a valid
		   allocated resource.	Return TPM_BAD_PARAMETER on error. */
		if (returnCode != TPM_SUCCESS) {
		    printf("TPM_Process_FlushSpecific: Error, context count %08x not found\n",
			   handle);
		    returnCode = TPM_BAD_PARAMETER;
		}
	    }
	    /* 8. Invalidate R1 and all internal resources allocated to R1 */
	    /* a. Resources include authorization sessions */
	    if (returnCode == TPM_SUCCESS) {
		/* setting the entry to 0 prevents the session from being reloaded. */
		tpm_state->tpm_stclear_data.contextList[r1Resource] = 0;
	    }
	    break;
	  case TPM_RT_KEY:
	    /* 2. Else if resourceType is TPM_RT_KEY */
	    /* a. Set R1 to the key pointed to by handle */
	    if (returnCode == TPM_SUCCESS) {
		printf("TPM_Process_FlushSpecific: Flushing key handle %08x\n", handle);
		returnCode = TPM_KeyHandleEntries_GetEntry(&tpm_key_handle_entry,
							   tpm_state->tpm_key_handle_entries,
							   handle);
		/* 7. Validate that R1 determined by resourceType and handle points to a valid
		   allocated resource.	Return TPM_BAD_PARAMETER on error. */
		if (returnCode != TPM_SUCCESS) {
		    printf("TPM_Process_FlushSpecific: Error, key handle %08x not found\n",
			   handle);
		    returnCode = TPM_BAD_PARAMETER;
		}
	    }
	    /* b. If R1 -> ownerEvict is TRUE return TPM_KEY_OWNER_CONTROL */
	    if (returnCode == TPM_SUCCESS) {
		if (tpm_key_handle_entry->keyControl & TPM_KEY_CONTROL_OWNER_EVICT) {
		    printf("TPM_Process_FlushSpecific: Error, keyHandle specifies owner evict\n");
		    returnCode = TPM_KEY_OWNER_CONTROL;
		}
	    }
	    /* 8. Invalidate R1 and all internal resources allocated to R1 */
	    /* a. Resources include authorization sessions */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_KeyHandleEntry_FlushSpecific(tpm_state, tpm_key_handle_entry);
	    }
	    break;
	  case TPM_RT_AUTH:
	    /* NOTE replaces deprecated TPM_Terminate_Handle */
	    /* 3. Else if resourceType is TPM_RT_AUTH */
	    /* a. Set R1 to the authorization session pointed to by handle */
	    /* 7. Validate that R1 determined by resourceType and handle points to a valid allocated
	       resource.  Return TPM_BAD_PARAMETER on error. */
	    /* 8. Invalidate R1 and all internal resources allocated to R1 */
	    /* a. Resources include authorization sessions */
	    printf("TPM_Process_FlushSpecific: Flushing authorization session handle %08x\n",
		   handle);
	    returnCode = TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions,
							  handle);
	    break;
	  case TPM_RT_TRANS:
	    /* 4. Else if resourceType is TPM_RT_TRANS */
	    /* a. Set R1 to the transport session pointed to by handle */
	    /* 7. Validate that R1 determined by resourceType and handle points to a valid allocated
	       resource.  Return TPM_BAD_PARAMETER on error. */
	    /* 8. Invalidate R1 and all internal resources allocated to R1 */
	    /* a. Resources include authorization sessions */
	    printf("TPM_Process_FlushSpecific: Flushing transport session handle %08x\n", handle);
	    returnCode = TPM_TransportSessions_TerminateHandle
			 (tpm_state->tpm_stclear_data.transSessions,
			  handle,
			  &(tpm_state->tpm_stany_flags.transportExclusive));
	    break;
	  case TPM_RT_DAA_TPM:
	    /* 5. Else if resourceType is TPM_RT_DAA_TPM */
	    /* a. Set R1 to the DAA session pointed to by handle */
	    /* 7. Validate that R1 determined by resourceType and handle points to a valid allocated
	       resource.  Return TPM_BAD_PARAMETER on error. */
	    /* 8. Invalidate R1 and all internal resources allocated to R1 */
	    /* a. Resources include authorization sessions */
	    printf("TPM_Process_FlushSpecific: Flushing DAA session handle %08x\n", handle);
	    returnCode = TPM_DaaSessions_TerminateHandle(tpm_state->tpm_stclear_data.daaSessions,
							 handle);
	    break;
	  default:
	    /* 6. Else return TPM_INVALID_RESOURCE */
	    printf("TPM_Process_FlushSpecific: Error, invalid resourceType %08x\n", resourceType);
	    returnCode = TPM_INVALID_RESOURCE;
	    break;
	}
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_FlushSpecific: Ordinal returnCode %08x %u\n",
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
    
/* 21.2 TPM_SaveContext rev 107

  SaveContext saves a loaded resource outside the TPM. After successful execution of the command the
  TPM automatically releases the internal memory for sessions but leaves keys in place.

  The caller of the function uses the label field to add additional sequencing, anti-replay or other
  items to the blob. The information does not need to be confidential but needs to be part of the
  blob integrity.
*/

TPM_RESULT TPM_Process_SaveContext(tpm_state_t *tpm_state,
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
    TPM_HANDLE		handle;			/* Handle of the resource being saved. */
    TPM_RESOURCE_TYPE	resourceType = 0;	/* The type of resource that is being saved */
    BYTE		label[TPM_CONTEXT_LABEL_SIZE];	/* Label for identification purposes */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_STORE_BUFFER		b1_sbuffer;		/* serialization of b1 */
    TPM_STCLEAR_DATA		*v1StClearData = NULL;
    TPM_KEY_HANDLE_ENTRY	*tpm_key_handle_entry;	/* key table entry for the handle */
    TPM_AUTH_SESSION_DATA	*tpm_auth_session_data = NULL; /* session table entry for the handle */
    TPM_TRANSPORT_INTERNAL	*tpm_transport_internal; /* transport table entry for the handle */
    TPM_DAA_SESSION_DATA	*tpm_daa_session_data;	/* daa session table entry for the handle */
    TPM_NONCE			*n1ContextNonce = NULL;
    TPM_SYMMETRIC_KEY_TOKEN 	k1ContextKey = NULL;
    TPM_STORE_BUFFER		r1ContextSensitive; /* serialization of sensitive data clear text */
    TPM_CONTEXT_SENSITIVE	c1ContextSensitive;
    TPM_CONTEXT_BLOB		b1ContextBlob;
    TPM_STORE_BUFFER		c1_sbuffer;		/* serialization of c1ContextSensitive */
    uint32_t			contextIndex = 0;	/* free index in context list */
    uint32_t			space;			/* free space in context list */
    TPM_BOOL			isZero;
    
    /* output parameters */
    uint32_t		outParamStart;			/* starting point of outParam's */
    uint32_t		outParamEnd;			/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    
    printf("TPM_Process_SaveContext: Ordinal Entry\n");
    TPM_Sbuffer_Init(&b1_sbuffer);			/* freed @1 */
    TPM_Sbuffer_Init(&r1ContextSensitive);		/* freed @2 */
    TPM_ContextBlob_Init(&b1ContextBlob);		/* freed @3 */
    TPM_ContextSensitive_Init(&c1ContextSensitive);	/* freed @4 */
    TPM_Sbuffer_Init(&c1_sbuffer);			/* freed @6 */
    /*
      get inputs
    */
    /* get handle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&handle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get resourceType */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_SaveContext: handle %08x\n", handle);
	returnCode = TPM_Load32(&resourceType, &command, &paramSize);
    }
    /* get label */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_SaveContext: resourceType %08x\n", resourceType);
	returnCode = TPM_Loadn(label, TPM_CONTEXT_LABEL_SIZE, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_SaveContext: label", label);
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
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_SaveContext: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. Map V1 to TPM_STANY_DATA NOTE MAY be TPM_STCLEAR_DATA */
    if (returnCode == TPM_SUCCESS) {
	v1StClearData = &(tpm_state->tpm_stclear_data);
    }
    /* 2. Validate that handle points to resource that matches resourceType, return
       TPM_INVALID_RESOURCE on error */
    /* 3. Validate that resourceType is a resource from the following list if not return
       TPM_INVALID_RESOURCE */
    if (returnCode == TPM_SUCCESS) {
	switch (resourceType) {
	  case TPM_RT_KEY:
	    /* a. TPM_RT_KEY */
	    printf("TPM_Process_SaveContext: Resource is key handle %08x\n", handle);
	    /* check if the key handle is valid */
	    returnCode = TPM_KeyHandleEntries_GetEntry(&tpm_key_handle_entry,
						       tpm_state->tpm_key_handle_entries,
						       handle);
	    break;
	  case TPM_RT_AUTH:
	    /* b. TPM_RT_AUTH */
	    printf("TPM_Process_SaveContext: Resource is session handle %08x\n", handle);
	    returnCode = TPM_AuthSessions_GetEntry(&tpm_auth_session_data,
						   v1StClearData->authSessions,
						   handle);
	    break;
	  case TPM_RT_TRANS:
	    /* c. TPM_RT_TRANS */
	    printf("TPM_Process_SaveContext: Resource is transport handle %08x\n", handle);
	    returnCode = TPM_TransportSessions_GetEntry(&tpm_transport_internal,
							v1StClearData->transSessions,
							handle);
	    break;
	  case TPM_RT_DAA_TPM:	
	    /* d. TPM_RT_DAA_TPM */
	    printf("TPM_Process_SaveContext: Resource is DAA handle %08x\n", handle);
	    returnCode = TPM_DaaSessions_GetEntry(&tpm_daa_session_data,
						  v1StClearData->daaSessions,
						  handle);
	    break;
	  default:
	    printf("TPM_Process_SaveContext: Error, invalid resourceType %08x\n", resourceType);
	    returnCode = TPM_INVALID_RESOURCE;
	    break;
	}
	if (returnCode != 0) {
	    printf("TPM_Process_SaveContext: Error, handle %08x not found\n", handle);
	    returnCode = TPM_INVALID_RESOURCE;
	}
    }
    /* 4. Locate the correct nonce */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_SaveContext: Locating nonce\n");
	/* a. If resourceType is TPM_RT_KEY */
	if (resourceType == TPM_RT_KEY) {
	    if (returnCode == TPM_SUCCESS) {
		/* i. If TPM_STCLEAR_DATA -> contextNonceKey is NULLS */
		TPM_Nonce_IsZero(&isZero, tpm_state->tpm_stclear_data.contextNonceKey);
		if (isZero) {
		    /* (1) Set TPM_STCLEAR_DATA -> contextNonceKey to the next value from the TPM
		       RNG */
		    returnCode = TPM_Nonce_Generate(tpm_state->tpm_stclear_data.contextNonceKey);
		}
	    }
	    if (returnCode == TPM_SUCCESS) {
		/* ii. Map N1 to TPM_STCLEAR_DATA -> contextNonceKey */
		n1ContextNonce = &(tpm_state->tpm_stclear_data.contextNonceKey);
		/* iii. If the key has TPM_KEY_CONTROL_OWNER_EVICT set then return TPM_OWNER_CONTROL
		 */
		if (tpm_key_handle_entry->keyControl & TPM_KEY_CONTROL_OWNER_EVICT) {
		    printf("TPM_Process_SaveContext: Error, key under owner control\n");
		    returnCode = TPM_OWNER_CONTROL;
		}
	    }
	}
	/* b. Else (resource not TPM_RT_KEY) */
	else {
	    if (returnCode == TPM_SUCCESS) {
		/* i. If V1 -> contextNonceSession is NULLS */
		TPM_Nonce_IsZero(&isZero, v1StClearData->contextNonceSession);
		if (isZero) {
		    /* (1) Set V1 -> contextNonceSession to the next value from the TPM RNG */
		    returnCode = TPM_Nonce_Generate(v1StClearData->contextNonceSession);
		}
	    }
	    /* ii. Map N1 to V1 -> contextNonceSession */
	    if (returnCode == TPM_SUCCESS) {
		n1ContextNonce = &(v1StClearData->contextNonceSession);
	    }
	}
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_SaveContext: Building sensitive data\n");
	/* 5. Set K1 to TPM_PERMANENT_DATA -> contextKey */
	k1ContextKey = tpm_state->tpm_permanent_data.contextKey;
	/* 6. Create R1 by putting the sensitive part of the resource pointed to by handle into a
	   structure. The structure is a TPM manufacturer option. The TPM MUST ensure that ALL
	   sensitive information of the resource is included in R1. */
	/* NOTE Since the contextKey is a symmetric key, the entire resource is put into the
	   sensitiveData */
	switch (resourceType) {
	  case TPM_RT_KEY:
	    returnCode = TPM_KeyHandleEntry_Store(&r1ContextSensitive, tpm_key_handle_entry);
	    break;
	  case TPM_RT_AUTH:
	    returnCode = TPM_AuthSessionData_Store(&r1ContextSensitive, tpm_auth_session_data);
	    break;
	  case TPM_RT_TRANS:
	    returnCode = TPM_TransportInternal_Store(&r1ContextSensitive, tpm_transport_internal);
	    break;
	  case TPM_RT_DAA_TPM:
	    returnCode = TPM_DaaSessionData_Store(&r1ContextSensitive, tpm_daa_session_data);
	    break;
	  default:
	    printf("TPM_Process_SaveContext: Error, invalid resourceType %08x", resourceType);
	    returnCode = TPM_INVALID_RESOURCE;
	    break;
	}
    }
    /* 7. Create C1 a TPM_CONTEXT_SENSITIVE structure */
    /* NOTE Done at TPM_ContextSensitive_Init() */
    /* a. C1 forms the inner encrypted wrapper for the blob. All saved context blobs MUST include a
       TPM_CONTEXT_SENSITIVE structure and the TPM_CONTEXT_SENSITIVE structure MUST be encrypted. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_SaveContext: Building TPM_CONTEXT_SENSITIVE\n");
	/* b. Set C1 -> contextNonce to N1 */
	TPM_Nonce_Copy(c1ContextSensitive.contextNonce, *n1ContextNonce);
	/* c. Set C1 -> internalData to R1 */
	returnCode = TPM_SizedBuffer_SetFromStore(&(c1ContextSensitive.internalData),
						  &r1ContextSensitive);
    }
    /* 8. Create B1 a TPM_CONTEXT_BLOB */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_SaveContext: Building TPM_CONTEXT_BLOB\n");
	/* a. Set B1 -> tag to TPM_TAG_CONTEXTBLOB */
	/* NOTE Done at TPM_ContextBlob_Init() */
	/* b. Set B1 -> resourceType to resourceType */
	b1ContextBlob.resourceType = resourceType;
	/* c. Set B1 -> handle to handle */
	b1ContextBlob.handle = handle;
	/* d. Set B1 -> integrityDigest to NULL */
	/* NOTE Done at TPM_ContextBlob_Init() */
	/* e. Set B1 -> label to label */
	memcpy(b1ContextBlob.label, label, TPM_CONTEXT_LABEL_SIZE);

    }
    /* f. Set B1 -> additionalData to information determined by the TPM manufacturer. This data will
       help the TPM to reload and reset context. This area MUST NOT hold any data that is sensitive
       (symmetric IV are fine, prime factors of an RSA key are not).  */
    /* i. For OSAP sessions, and for DSAP sessions attached to keys, the hash of the entity MUST be
       included in additionalData */
    /* NOTE Included in TPM_AUTH_SESSION_DATA.	This is implementation defined, and the manufacturer
       can put everything in sensitive data.  */
    /* g. Set B1 -> additionalSize to the size of additionalData */
    /* NOTE Initialized by TPM_ContextBlob_Init() */
    /* h. Set B1 -> sensitiveSize to the size of C1 */
    /* i. Set B1 -> sensitiveData to C1 */
    /* serialize C1 */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_ContextSensitive_Store(&c1_sbuffer, &c1ContextSensitive);
    }
    /* Here the clear text goes into TPM_CONTEXT_BLOB->sensitiveData */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_SetFromStore(&(b1ContextBlob.sensitiveData), &c1_sbuffer);
    }
    if (returnCode == TPM_SUCCESS) {
	/* 9. If resourceType is TPM_RT_KEY */
	if (resourceType == TPM_RT_KEY) {
	    /* a. Set B1 -> contextCount to 0 */
	    b1ContextBlob.contextCount = 0;
	}
	/* 10. Else */
	else {
	    printf("TPM_Process_SaveContext: Processing session context count\n");
	    if (returnCode == TPM_SUCCESS) {
		/* a. If V1 -> contextCount > 2^32-2 then */
		if (v1StClearData->contextCount > 0xfffffffe) {
		    /* i. Return with TPM_TOOMANYCONTEXTS */
		    printf("TPM_Process_SaveContext: Error, too many contexts\n");
		    returnCode = TPM_TOOMANYCONTEXTS;
		}
	    }
	    /* b. Else */
	    if (returnCode == TPM_SUCCESS) {
		/* i. Validate that the TPM can still manage the new count value */
		/* (1) If the distance between the oldest saved context and the contextCount is
		   too large return TPM_CONTEXT_GAP */
		/* Since contextCount is uint32_t, this is not applicable here.  From email: Does
		   the TPM have the ability to keep track of the context delta. It is possible to
		   keep track of things with just a byte or so internally, if this is done a gap of
		   greater than 2^16 or so might be too large, hence the context gap message */
	    }
	    /* ii. Find contextIndex such that V1 -> contextList[contextIndex] equals 0. If not
	       found exit with TPM_NOCONTEXTSPACE */
	    if (returnCode == TPM_SUCCESS) {
		TPM_ContextList_GetSpace(&space, &contextIndex, v1StClearData->contextList);
		if (space == 0) {
		    printf("TPM_Process_SaveContext: Error, no space in context list\n");
		    returnCode = TPM_NOCONTEXTSPACE;
		}
	    }
	    if (returnCode == TPM_SUCCESS) {
		/* iii. Increment V1 -> contextCount by 1 */
		v1StClearData->contextCount++;
		/* iv. Set V1-> contextList[contextIndex] to V1 -> contextCount */
		v1StClearData->contextList[contextIndex] = v1StClearData->contextCount;
		/* v. Set B1 -> contextCount to V1 -> contextCount */
		b1ContextBlob.contextCount = v1StClearData->contextCount;
	    }	
	    /* c. The TPM MUST invalidate all information regarding the resource except for
	       information needed for reloading */
	    if (returnCode == TPM_SUCCESS) {
		switch (resourceType) {
		  case TPM_RT_AUTH:
		    returnCode = TPM_AuthSessions_TerminateHandle(v1StClearData->authSessions,
								  handle);
		    break;
		  case TPM_RT_TRANS:
		    returnCode = TPM_TransportSessions_TerminateHandle
				 (v1StClearData->transSessions,
				  handle,
				  &(tpm_state->tpm_stany_flags.transportExclusive));
		    break;
		  case TPM_RT_DAA_TPM:
		    returnCode = TPM_DaaSessions_TerminateHandle(v1StClearData->daaSessions,
								 handle);
		    break;
		  default:
		    printf("TPM_Process_SaveContext: Error, invalid resourceType %08x",
			   resourceType);
		    returnCode = TPM_INVALID_RESOURCE;
		    break;
		}
	    }
	}
    }
    /* 11. Calculate B1 -> integrityDigest the HMAC of B1 using TPM_PERMANENT_DATA -> tpmProof as
       the secret.  NOTE It is calculated on the cleartext data */
    if (returnCode == TPM_SUCCESS) {
	/* This is a bit circular.  It's safe since the TPM_CONTEXT_BLOB is serialized before the
	   HMAC is generated.  The result is put back into the structure.  */
	printf("TPM_Process_SaveContext: Digesting TPM_CONTEXT_BLOB\n");
	returnCode = TPM_HMAC_GenerateStructure
		     (b1ContextBlob.integrityDigest,		/* HMAC */
		      tpm_state->tpm_permanent_data.tpmProof,	/* HMAC key */
		      &b1ContextBlob,				/* structure */
		      (TPM_STORE_FUNCTION_T)TPM_ContextBlob_Store);	/* store function */
    }
    /* 12. Create E1 by encrypting C1 using K1 as the key */
    /* a. Set B1 -> sensitiveSize to the size of E1 */
    /* b. Set B1 -> sensitiveData to E1 */
    if (returnCode == TPM_SUCCESS) {
	/* The cleartext went into sensitiveData for the integrityDigest calculation.  Free it now,
	   before the encrypted data is stored there. */
	TPM_SizedBuffer_Delete(&(b1ContextBlob.sensitiveData));
	returnCode = TPM_SymmetricKeyData_EncryptSbuffer(&(b1ContextBlob.sensitiveData),
							 &c1_sbuffer,
							 k1ContextKey);
    }
    /* 13. Set contextSize to the size of B1 */
    /* 14. Return B1 in contextBlob */
    /* Since the redundant size parameter must be returned, the TPM_CONTEXT_BLOB is serialized
       first.  Later, rather than the usual _Store to the response, the already serialized buffer is
       stored. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_ContextBlob_Store(&b1_sbuffer, &b1ContextBlob);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_SaveContext: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return contextSize and contextBlob */
	    returnCode = TPM_Sbuffer_AppendAsSizedBuffer(response, &b1_sbuffer);
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
    TPM_Sbuffer_Delete(&b1_sbuffer);			/* @1 */
    TPM_Sbuffer_Delete(&r1ContextSensitive);		/* @2 */
    TPM_ContextBlob_Delete(&b1ContextBlob);		/* @3 */
    TPM_ContextSensitive_Delete(&c1ContextSensitive);	/* @4 */
    TPM_Sbuffer_Delete(&c1_sbuffer);			/* @6 */
    return rcf;
}

/* 21.3 TPM_LoadContext rev 107

   TPM_LoadContext loads into the TPM a previously saved context. The command returns the handle.
*/

TPM_RESULT TPM_Process_LoadContext(tpm_state_t *tpm_state,
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
    TPM_HANDLE entityHandle;		/* The handle the TPM MUST use to locate the entity ties
					   to the OSAP/DSAP session */
    TPM_BOOL keepHandle;		/* Indication if the handle MUST be preserved */
    uint32_t contextSize;		/* The size of the following context blob */
    TPM_CONTEXT_BLOB b1ContextBlob;	/* The context blob */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			key_added = FALSE;	/* key has been added to handle list */
    TPM_BOOL			auth_session_added = FALSE;
    TPM_BOOL			trans_session_added = FALSE;
    TPM_BOOL			daa_session_added = FALSE;
    TPM_STCLEAR_DATA		*v1StClearData = NULL;
    unsigned char		*m1Decrypt;		/* decrypted sensitive data */
    uint32_t			m1_length;		/* actual data in m1 */
    unsigned char		*stream;
    uint32_t			stream_size;
    TPM_CONTEXT_SENSITIVE	c1ContextSensitive;
    TPM_KEY_HANDLE_ENTRY	tpm_key_handle_entry;
    TPM_AUTH_SESSION_DATA	tpm_auth_session_data;	/* loaded authorization session */
    TPM_TRANSPORT_INTERNAL	tpm_transport_internal; /* loaded transport session */
    TPM_DAA_SESSION_DATA	tpm_daa_session_data;	/* loaded daa session */
    TPM_DIGEST			entityDigest;		/* digest of the entity corresponding to
							   entityHandle */
    uint32_t			contextIndex;

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_LoadContext: Ordinal Entry\n");
    TPM_ContextBlob_Init(&b1ContextBlob);			/* freed @1 */
    TPM_KeyHandleEntry_Init(&tpm_key_handle_entry);		/* no free */
    m1Decrypt = NULL;						/* freed @2 */
    TPM_ContextSensitive_Init(&c1ContextSensitive);		/* freed @3 */
    TPM_AuthSessionData_Init(&tpm_auth_session_data);		/* freed @4 */
    TPM_TransportInternal_Init(&tpm_transport_internal);	/* freed @5 */
    TPM_DaaSessionData_Init(&tpm_daa_session_data);		/* freed @6 */
    /*
      get inputs
    */
    /* get parameter entityHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&entityHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get keepHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadContext: entityHandle %08x\n", entityHandle);
	returnCode = TPM_LoadBool(&keepHandle, &command, &paramSize);
    }
    /* get contextSize parameter (redundant, not used) */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadContext: keepHandle %02x\n", keepHandle);
	returnCode = TPM_Load32(&contextSize, &command, &paramSize);
    }
    /* get contextBlob parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_ContextBlob_Load(&b1ContextBlob, &command, &paramSize);
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
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_LoadContext: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. Map contextBlob to B1, a TPM_CONTEXT_BLOB structure */
    /* NOTE Done by TPM_ContextBlob_Load() */
    if (returnCode == TPM_SUCCESS) {
	/* 2. Map V1 to TPM_STANY_DATA NOTE MAY be TPM_STCLEAR_DATA */
	v1StClearData = &(tpm_state->tpm_stclear_data);
	/* 3. Create M1 by decrypting B1 -> sensitiveData using TPM_PERMANENT_DATA -> contextKey */
	printf("TPM_Process_LoadContext: Decrypting sensitiveData\n");
	returnCode = TPM_SymmetricKeyData_Decrypt(&m1Decrypt,		/* decrypted data */
						  &m1_length,		/* length decrypted data */
						  b1ContextBlob.sensitiveData.buffer, /* encrypt */
						  b1ContextBlob.sensitiveData.size,
						  tpm_state->tpm_permanent_data.contextKey);
    }
    /* 4. Create C1 and R1 by splitting M1 into a TPM_CONTEXT_SENSITIVE structure and internal
       resource data */
    /* NOTE R1 is manufacturer specific data that might be part of the blob.  This implementation
       does not use R1 */
    if (returnCode == TPM_SUCCESS) {
	stream = m1Decrypt;
	stream_size = m1_length;
	returnCode = TPM_ContextSensitive_Load(&c1ContextSensitive, &stream, &stream_size);
    }
    /* Parse the TPM_CONTEXT_SENSITIVE -> internalData depending on the resource type */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadContext: Parsing TPM_CONTEXT_SENSITIVE -> internalData\n");
	stream = c1ContextSensitive.internalData.buffer;
	stream_size = c1ContextSensitive.internalData.size;
	switch (b1ContextBlob.resourceType) {
	  case TPM_RT_KEY:
	    printf("TPM_Process_LoadContext: Loading TPM_KEY_HANDLE_ENTRY\n");
	    returnCode = TPM_KeyHandleEntry_Load(&tpm_key_handle_entry, &stream, &stream_size);
	    break;
	  case TPM_RT_AUTH:
	    printf("TPM_Process_LoadContext: Loading TPM_AUTH_SESSION_DATA\n");
	    returnCode = TPM_AuthSessionData_Load(&tpm_auth_session_data, &stream, &stream_size);
	    printf("TPM_Process_LoadContext: protocolID %02x entityTypeByte %02x\n",
		   tpm_auth_session_data.protocolID, tpm_auth_session_data.entityTypeByte);
	    break;
	  case TPM_RT_TRANS:
	    printf("TPM_Process_LoadContext: Loading TPM_TRANSPORT_INTERNAL\n");
	    returnCode = TPM_TransportInternal_Load(&tpm_transport_internal,
						    &stream, &stream_size);
	    break;
	  case TPM_RT_DAA_TPM:
	    printf("TPM_Process_LoadContext: Loading TPM_DAA_SESSION_DATA\n");
	    returnCode = TPM_DaaSessionData_Load(&tpm_daa_session_data, &stream, &stream_size);
	    printf("TPM_Process_LoadContext: stage %u\n",
		   tpm_daa_session_data.DAA_session.DAA_stage);
	    break;
	  default:
	    printf("TPM_Process_LoadContext: Error, invalid resourceType %08x",
		   b1ContextBlob.resourceType);
	    returnCode = TPM_INVALID_RESOURCE;
	    break;
	}	
    }
    /* 5. Check contextNonce */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadContext: Checking contextNonce\n");
	/* a. If B1 -> resourceType is NOT TPM_RT_KEY */
	if (b1ContextBlob.resourceType != TPM_RT_KEY) {
	    /* i. If C1 -> contextNonce does not equal V1 -> contextNonceSession return
	       TPM_BADCONTEXT */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_Nonce_Compare(v1StClearData->contextNonceSession,
					       c1ContextSensitive.contextNonce);
		if (returnCode != TPM_SUCCESS) {
		    printf("TPM_Process_LoadContext: Error comparing non-key contextNonce\n");
		    returnCode = TPM_BADCONTEXT;
		}
	    }
	    /* ii. Validate that the resource pointed to by the context is loaded (i.e. for OSAP the
	       key referenced is loaded and DSAP connected to the key) return TPM_RESOURCEMISSING */
	    /* (1) For OSAP sessions and for DSAP sessions attached to keys, the TPM MUST validate
	       that the hash of the entity matches the entity held by the TPM */
	    /* (2) For OSAP and DSAP sessions referring to a key, verify that entityHandle
	       identifies the key linked to this OSAP/DSAP session, if not return TPM_BAD_HANDLE. */
	    if ((returnCode == TPM_SUCCESS) && (b1ContextBlob.resourceType == TPM_RT_AUTH)) {
		if ((tpm_auth_session_data.protocolID == TPM_PID_OSAP) ||
		    (tpm_auth_session_data.protocolID == TPM_PID_DSAP)) {
		    /* check that the entity is loaded, and get the entity's digest */
		    switch (tpm_auth_session_data.entityTypeByte) {
		      case TPM_ET_KEYHANDLE:
			returnCode = TPM_LoadContext_CheckKeyLoaded(tpm_state,
								    entityHandle,
								    entityDigest);
			break;
		      case TPM_ET_OWNER:
			returnCode = TPM_LoadContext_CheckOwnerLoaded(tpm_state,
								      entityDigest);
			break;
		      case TPM_ET_SRK:
			returnCode = TPM_LoadContext_CheckSrkLoaded(tpm_state,
								    entityDigest);
			break;
		      case TPM_ET_COUNTER:
			returnCode = TPM_LoadContext_CheckCounterLoaded(tpm_state,
									entityHandle,
									entityDigest);
			break;
		      case TPM_ET_NV:
			returnCode = TPM_LoadContext_CheckNvLoaded(tpm_state,
								   entityHandle,
								   entityDigest);
			break;
		      default:
			printf("TPM_Process_LoadContext: Error, invalid session entityType %02x\n",
			       tpm_auth_session_data.entityTypeByte);
			returnCode = TPM_WRONG_ENTITYTYPE;
			break;
		    }
		    if (returnCode == TPM_SUCCESS) {
			returnCode= TPM_Digest_Compare(entityDigest,
						       tpm_auth_session_data.entityDigest);
			if (returnCode != TPM_SUCCESS) {
			    printf("TPM_Process_LoadContext: Error, "
				   "OSAP or DSAP entityDigest mismatch\n");
			    returnCode = TPM_RESOURCEMISSING;
			}
		    }
		}
	    }
	}
	/* b. Else (TPM_RT_KEY) */
	else {
	    /* i. If C1 -> internalData -> parentPCRStatus is FALSE and C1 -> internalData ->
	       isVolatile is FALSE */
	    /* NOTE parentPCRStatus and keyFlags are not security sensitive data, could be in
	       additionalData */
	    /* (1) Ignore C1 -> contextNonce */
	    if (returnCode == TPM_SUCCESS) {
		if (tpm_key_handle_entry.parentPCRStatus ||
		    (tpm_key_handle_entry.key->keyFlags & TPM_ISVOLATILE)) {
		    /* ii. else */
		    /* (1) If C1 -> contextNonce does not equal TPM_STCLEAR_DATA -> contextNonceKey
		       return TPM_BADCONTEXT */
		    returnCode = TPM_Nonce_Compare(v1StClearData->contextNonceKey,
						   c1ContextSensitive.contextNonce);
		    if (returnCode != 0) {
			printf("TPM_Process_LoadContext: Error comparing contextNonceKey\n");
			returnCode = TPM_BADCONTEXT;
		    }
		}
	    }
	}
    }
    /* 6. Validate the structure */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadContext: Checking integrityDigest\n");
	/* a. Set H1 to B1 -> integrityDigest */
	/* NOTE Done by TPM_HMAC_CheckStructure() */
	/* b. Set B1 -> integrityDigest to all zeros */
	/* NOTE Done by TPM_HMAC_CheckStructure() */
	/* c. Copy M1 to B1 -> sensitiveData (integrityDigest HMAC uses cleartext) */
	returnCode = TPM_SizedBuffer_Set(&(b1ContextBlob.sensitiveData), m1_length, m1Decrypt);
    }
    /* d. Create H2 the HMAC of B1 using TPM_PERMANENT_DATA -> tpmProof as the HMAC key */
    /* e. If H2 does not equal H1 return TPM_BADCONTEXT */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_HMAC_CheckStructure
		     (tpm_state->tpm_permanent_data.tpmProof,		/* key */
		      &b1ContextBlob,					/* structure */
		      b1ContextBlob.integrityDigest,			/* expected */
		      (TPM_STORE_FUNCTION_T)TPM_ContextBlob_Store,	/* store function */
		      TPM_BADCONTEXT);					/* error code */
    }
    /* 9. If B1 -> resourceType is NOT TPM_RT_KEY */
    if ((returnCode == TPM_SUCCESS) && (b1ContextBlob.resourceType != TPM_RT_KEY)) {
	printf("TPM_Process_LoadContext: Checking contextCount\n");
	/* a. Find contextIndex such that V1 -> contextList[contextIndex] equals B1 ->
	   TPM_CONTEXT_BLOB -> contextCount */
	/* b. If not found then return TPM_BADCONTEXT */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_ContextList_GetEntry(&contextIndex,
						  v1StClearData->contextList,
						  b1ContextBlob.contextCount);
	}
	/* c. Set V1 -> contextList[contextIndex] to 0 */
	if (returnCode == TPM_SUCCESS) {
	    v1StClearData->contextList[contextIndex] = 0;
	}
    }
    /* 10. Process B1 to return the resource back into TPM use */
    /* restore the entity, try to keep the handle as 'handle' */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadContext: Adding entry to table\n");
	switch (b1ContextBlob.resourceType) {
	  case TPM_RT_KEY:
	    returnCode = TPM_KeyHandleEntries_AddEntry(&(b1ContextBlob.handle),
						       keepHandle,
						       tpm_state->tpm_key_handle_entries,
						       &tpm_key_handle_entry);
	    key_added = TRUE;
	    break;
	  case TPM_RT_AUTH:
	    returnCode = TPM_AuthSessions_AddEntry(&(b1ContextBlob.handle),	/* input/output */
						   keepHandle,
						   v1StClearData->authSessions,
						   &tpm_auth_session_data);
	    auth_session_added = TRUE;
	    break;
	  case TPM_RT_TRANS:
	    returnCode = TPM_TransportSessions_AddEntry(&(b1ContextBlob.handle), /* input/output */
							keepHandle,
							v1StClearData->transSessions,
							&tpm_transport_internal);
	    trans_session_added = TRUE;
	    break;
	  case TPM_RT_DAA_TPM:
	    returnCode = TPM_DaaSessions_AddEntry(&(b1ContextBlob.handle),	/* input/output */
						  keepHandle,
						  v1StClearData->daaSessions,
						  &tpm_daa_session_data);
	    daa_session_added = TRUE;
	    break;
	  default:
	    printf("TPM_Process_LoadContext: Error, invalid resourceType %08x\n",
		   b1ContextBlob.resourceType);
	    returnCode = TPM_INVALID_RESOURCE;
	    break;
	}
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_LoadContext: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* return handle */
	    returnCode = TPM_Sbuffer_Append32(response, b1ContextBlob.handle);
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
    /* if there was a failure, roll back */
    if ((rcf != 0) || (returnCode != TPM_SUCCESS)) {
	TPM_Key_Delete(tpm_key_handle_entry.key);	/* free on error */
	free(tpm_key_handle_entry.key);			/* free on error */
	if (key_added) {
	    /* if there was a failure and inKey was stored in the handle list, free the handle.
	       Ignore errors, since only one error code can be returned. */
	    TPM_KeyHandleEntries_DeleteHandle(tpm_state->tpm_key_handle_entries,
					      b1ContextBlob.handle);
	}
	if (auth_session_added) {
	    TPM_AuthSessions_TerminateHandle(v1StClearData->authSessions, b1ContextBlob.handle);
	}
	if (trans_session_added) {
	    TPM_TransportSessions_TerminateHandle(v1StClearData->transSessions,
						  b1ContextBlob.handle,
						  &(tpm_state->tpm_stany_flags.transportExclusive));
	}
	if (daa_session_added) {
	    TPM_DaaSessions_TerminateHandle(v1StClearData->daaSessions, b1ContextBlob.handle);
	}
    }
    TPM_ContextBlob_Delete(&b1ContextBlob);			/* @1 */
    free(m1Decrypt);						/* @2 */
    TPM_ContextSensitive_Delete(&c1ContextSensitive);		/* @3 */
    TPM_AuthSessionData_Delete(&tpm_auth_session_data);		/* @4 */
    TPM_TransportInternal_Delete(&tpm_transport_internal);	/* @5 */
    TPM_DaaSessionData_Delete(&tpm_daa_session_data);		/* @6 */
    return rcf;
}

/* TPM_LoadContext_CheckKeyLoaded() validates that the key associated with a loading authorization
   context is loaded.

   It returns the key pubDataDigest for comparison with the digest of the loading context.
*/

static TPM_RESULT TPM_LoadContext_CheckKeyLoaded(tpm_state_t *tpm_state,
						 TPM_HANDLE entityHandle,
						 TPM_DIGEST entityDigest)
{
    TPM_RESULT			rc = 0;
    TPM_KEY_HANDLE_ENTRY	*key_handle_entry;

    printf("TPM_LoadContext_CheckKeyLoaded: handle %08x\n", entityHandle);
    /* get the key associated with entityHandle */
    /* special case, SRK is not in the key handle list */
    if (entityHandle == TPM_KH_SRK) {
	if (tpm_state->tpm_permanent_data.ownerInstalled) {
	    TPM_Digest_Copy(entityDigest,
			    tpm_state->tpm_permanent_data.srk.tpm_store_asymkey->pubDataDigest);
	}
	else {
	    printf("TPM_LoadContext_CheckKeyLoaded: Error, ownerInstalled is FALSE\n");
	    rc = TPM_NOSRK;
	}
    }
    /* normal case, key is in the key handle list */
    else {
	rc = TPM_KeyHandleEntries_GetEntry(&key_handle_entry,
					   tpm_state->tpm_key_handle_entries,
					   entityHandle);
	if (rc == 0) {
	    TPM_Digest_Copy(entityDigest, key_handle_entry->key->tpm_store_asymkey->pubDataDigest);
	}
	else {
	    printf("TPM_LoadContext_CheckKeyLoaded: Error, key handle %08x not found\n",
		   entityHandle);
	    rc = TPM_BAD_HANDLE;
	}
    }
    return rc;
}

/* TPM_LoadContext_CheckKeyLoadedByDigest() validates that the key associated with a loading
   authorization context is loaded.

   It compares the key the pubDataDigest to the digest of the loading context.
*/

static TPM_RESULT TPM_LoadContext_CheckKeyLoadedByDigest(tpm_state_t *tpm_state,
							 TPM_DIGEST entityDigest)
{
    TPM_RESULT			rc = TPM_RETRY;		/* any non-zero value will do */
    size_t			start;
    size_t			current;
    TPM_KEY_HANDLE_ENTRY	*key_handle_entry;

    printf("TPM_LoadContext_CheckKeyLoadedByDigest:\n");
    /* get the key associated with entityDigest */
    start = 0;
    /* iterate through all keys in the key handle table */
    while ((rc != 0) &&		/* a match sets rc to 0, terminates loop */
	   /* returns TPM_RETRY when at the end of the table, terminates loop */
	   (TPM_KeyHandleEntries_GetNextEntry(&key_handle_entry,
					      &current,
					      tpm_state->tpm_key_handle_entries,
					      start)) == 0) {
	

	start = current + 1;
	rc = TPM_Digest_Compare(entityDigest,
				key_handle_entry->key->tpm_store_asymkey->pubDataDigest);
    }
    /* if that failed, check the SRK */
    if (rc != 0) {
	if (tpm_state->tpm_permanent_data.ownerInstalled) {
	    rc = TPM_Digest_Compare
		 (entityDigest,
		  tpm_state->tpm_permanent_data.srk.tpm_store_asymkey->pubDataDigest);
	}
    }	 
    if (rc != 0) {
	printf("TPM_LoadContext_CheckKeyLoadedByDigest: "
	       "Error, OSAP or DSAP entityDigest mismatch\n");
	rc = TPM_RESOURCEMISSING;
    }
    return rc;
}

/* TPM_LoadContext_CheckOwnerLoaded() validates that the owner is loaded.

   It returns the owner authorization for comparison with the digest of the loading context.
*/

static TPM_RESULT TPM_LoadContext_CheckOwnerLoaded(tpm_state_t *tpm_state,
						   TPM_DIGEST entityDigest)
{
    TPM_RESULT			rc = 0;

    printf("TPM_LoadContext_CheckOwnerLoaded:\n");
    /* verify that an owner is installed */
    if (rc == 0) {
	if (!tpm_state->tpm_permanent_data.ownerInstalled) {
	    printf("TPM_LoadContext_CheckOwnerLoaded: Error, no owner\n");
	    rc = TPM_RESOURCEMISSING;
	}
    }
    if (rc == 0) {
	TPM_Digest_Copy(entityDigest, tpm_state->tpm_permanent_data.ownerAuth);
    }
    return rc;
}

/* TPM_LoadContext_CheckSrkLoaded() validates that the SRK is loaded.

   It returns the SRK pubDataDigest for comparison with the digest of the loading context.
*/

static TPM_RESULT TPM_LoadContext_CheckSrkLoaded(tpm_state_t *tpm_state,
						 TPM_DIGEST entityDigest)
{
    TPM_RESULT			rc = 0;

    printf("TPM_LoadContext_CheckSrkLoaded:\n");
    /* verify that an owner is installed */
    if (rc == 0) {
	if (!tpm_state->tpm_permanent_data.ownerInstalled) {
	    printf("TPM_LoadContext_CheckSrkLoaded: Error, no SRK\n");
	    rc = TPM_RESOURCEMISSING;
	}
    }
    if (rc == 0) {
	TPM_Digest_Copy(entityDigest,
			tpm_state->tpm_permanent_data.srk.tpm_store_asymkey->pubDataDigest);
    }
    return rc;
}

/* TPM_LoadContext_CheckCounterLoaded() validates that the counter associated with a loading
   authorization context is loaded.

   It returns the counter authorization for comparison with the digest of the loading context.
*/

static TPM_RESULT TPM_LoadContext_CheckCounterLoaded(tpm_state_t *tpm_state,
						     TPM_HANDLE entityHandle,
						     TPM_DIGEST entityDigest)
{
    TPM_RESULT			rc = 0;
    TPM_COUNTER_VALUE		*counterValue;		/* associated with entityHandle */

    printf("TPM_LoadContext_CheckCounterLoaded: handle %08x\n", entityHandle);
    if (rc == 0) {
	rc = TPM_Counters_GetCounterValue(&counterValue,
					  tpm_state->tpm_permanent_data.monotonicCounter,
					  entityHandle);
	if (rc != 0) {
	    printf("TPM_LoadContext_CheckCounterLoaded: Error, no counter\n");
	    rc = TPM_RESOURCEMISSING;
	}	    
    }
    if (rc == 0) {
	TPM_Digest_Copy(entityDigest, counterValue->digest);
    }
    return rc;
}

/* TPM_LoadContext_CheckNvLoaded() validates that the NV space associated with a loading
   authorization context exists.
*/

static TPM_RESULT TPM_LoadContext_CheckNvLoaded(tpm_state_t *tpm_state,
						TPM_HANDLE entityHandle,
						TPM_DIGEST entityDigest)
{

    TPM_RESULT			rc = 0;
    TPM_NV_DATA_SENSITIVE	*tpm_nv_data_sensitive;	/* associated with entityValue */

    printf(" TPM_LoadContext_CheckNvLoaded: handle %08x\n", entityHandle);
    if (rc == 0) {
	rc = TPM_NVIndexEntries_GetEntry(&tpm_nv_data_sensitive,
					 &(tpm_state->tpm_nv_index_entries),
					 entityHandle);
	if (rc != 0) {
	    printf("TPM_LoadContext_CheckNvLoaded: Error, no NV at index %08x\n", entityHandle);
	    rc = TPM_RESOURCEMISSING;
	}	    
    }
    if (rc == 0) {
	TPM_Digest_Copy(entityDigest, tpm_nv_data_sensitive->digest);
    }
    return rc;
}

/* 21.1 TPM_KeyControlOwner rev 116
    
    This command controls some attributes of keys that are stored within the TPM key cache.

    1. Set an internal bit within the key cache that controls some attribute of a loaded key.

    2.When a key is set to ownerEvict, the key handle value remains the same as long as the key
    remains ownerEvict.  The key handle value persists through TPM_Startup.
    
    OwnerEvict: If this bit is set to true, this key remains in the TPM non-volatile storage 
    through all TPM_Startup events. The only way to evict this key is for the TPM Owner to 
    execute this command again, setting the owner control bit to false and then executing 
    TPM_FlushSpecific.

    The key handle does not reference an authorized entity and is not validated.

    The check for two remaining key slots ensures that users can load the two keys required to
    execute many commands.  Since only the owner can flush owner evict keys, non-owner commands
    could be blocked if this test was not performed.
*/

TPM_RESULT TPM_Process_KeyControlOwner(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE	keyHandle;		/* The handle of a loaded key. */
    TPM_PUBKEY		pubKey;			/* The public key associated with the loaded key */
    TPM_KEY_CONTROL	bitName = 0;		/* The name of the bit to be modified */
    TPM_BOOL		bitValue = FALSE;	/* The value to set the bit to */
    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle used for owner
					   authentication. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   session handle */
    TPM_AUTHDATA	ownerAuth;	/* HMAC authorization: key ownerAuth */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_SECRET			*hmacKey;
    TPM_KEY_HANDLE_ENTRY	*tpm_key_handle_entry;	/* entry for keyHandle */
    TPM_BOOL			isSpace;
    TPM_BOOL			oldOwnerEvict;		/* original owner evict state */
    uint16_t 			ownerEvictCount;	/* current number of owner evict keys */
    
    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_KeyControlOwner: Ordinal Entry\n");
    TPM_Pubkey_Init(&pubKey);		/* freed @1 */
    /*
      get inputs
    */
    /* get keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&keyHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get pubKey parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_KeyControlOwner: keyHandle %08x\n", keyHandle);
	returnCode = TPM_Pubkey_Load(&pubKey, &command, &paramSize);
    }
    /* get bitName parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&bitName, &command, &paramSize);
    }
    /* get bitValue parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load8(&bitValue, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_KeyControlOwner: bitName %08x bitValue %02x\n", bitName, bitValue);
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
	    printf("TPM_Process_KeyControlOwner: Error, command has %u extra bytes\n",
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
    /* 1. Validate the AuthData using the owner authentication value, on error return TPM_AUTHFAIL
       */
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
    /* 2. Validate that keyHandle refers to a loaded key, return TPM_INVALID_KEYHANDLE on error. */
    /* get the key corresponding to the keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetEntry(&tpm_key_handle_entry,
						   tpm_state->tpm_key_handle_entries,
						   keyHandle);
	if (returnCode != TPM_SUCCESS) {
	    printf("TPM_Process_KeyControlOwner: Error, key handle not loaded\n");
	    returnCode = TPM_INVALID_KEYHANDLE;
	}
    }
    /* If the keyUsage field of the key indicated by keyHandle does not have the value
       TPM_KEY_SIGNING, TPM_KEY_STORAGE, TPM_KEY_IDENTITY, TPM_KEY_BIND, or TPM_KEY_LEGACY, the TPM
       must return the error code TPM_INVALID_KEYUSAGE. */
    if (returnCode == TPM_SUCCESS) {
	if ((tpm_key_handle_entry->key->keyUsage != TPM_KEY_SIGNING) &&
	    (tpm_key_handle_entry->key->keyUsage != TPM_KEY_STORAGE) &&
	    (tpm_key_handle_entry->key->keyUsage != TPM_KEY_IDENTITY) &&
	    (tpm_key_handle_entry->key->keyUsage != TPM_KEY_BIND) &&
	    (tpm_key_handle_entry->key->keyUsage != TPM_KEY_LEGACY)) {
	    printf("TPM_Process_KeyControlOwner: Error, invalid key keyUsage %04hx\n",
		   tpm_key_handle_entry->key->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 3. Validate that pubKey matches the key held by the TPM pointed to by keyHandle, return
       TPM_BAD_PARAMETER on mismatch */
    /* a. This check is added so that virtualization of the keyHandle does not result in attacks, as
       the keyHandle is not associated with an authorization value */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_ComparePubkey(tpm_key_handle_entry->key, &pubKey);
	if (returnCode != TPM_SUCCESS) {
	    printf("TPM_Process_KeyControlOwner: Error comparing pubKey\n");
	    returnCode = TPM_BAD_PARAMETER;
	}
    }	 
    /* 4. Validate that bitName is valid, return TPM_BAD_MODE on error.	 NOTE Valid means a legal
       TPM_KEY_CONTROL value */
    if (returnCode == TPM_SUCCESS) {
	switch(bitName) {
	    /* 5. If bitName == TPM_KEY_CONTROL_OWNER_EVICT */
	  case TPM_KEY_CONTROL_OWNER_EVICT:
	    /* save the old value to determine if NVRAM update is necessary */
	    oldOwnerEvict = tpm_key_handle_entry->keyControl & TPM_KEY_CONTROL_OWNER_EVICT;
	    /* a. If bitValue == TRUE */
	    if (bitValue) {
		printf("TPM_Process_KeyControlOwner: setting key owner evict\n");
		if (!oldOwnerEvict) {	/* if the key is not owner evict */
		    /* i. Verify that after this operation at least two key slots will be present
		       within the TPM that can store any type of key both of which do NOT have the
		       OwnerEvict bit set, on error return TPM_NOSPACE */
		    if (returnCode == TPM_SUCCESS) {
			TPM_KeyHandleEntries_IsEvictSpace(&isSpace,
							  tpm_state->tpm_key_handle_entries,
							  2);	/* minSpace */
			if (!isSpace) {
			    printf("TPM_Process_KeyControlOwner: Error, "
				   "Need 2 non-evict slots\n");
			    returnCode = TPM_NOSPACE;
			}
		    }
		    /* ii. Verify that for this key handle, parentPCRStatus is FALSE and isVolatile
		       is FALSE.  Return TPM_BAD_PARAMETER on error. */
		    if (returnCode == TPM_SUCCESS) {
			if (tpm_key_handle_entry->parentPCRStatus ||
			    tpm_key_handle_entry->key->keyFlags & TPM_ISVOLATILE) {
			    printf("TPM_Process_KeyControlOwner: Error, "
				   "parentPCRStatus or Volatile\n");
			    returnCode = TPM_BAD_PARAMETER;
			}
		    }
		    /* check the current number of occupied owner evict key slots */
		    if (returnCode == TPM_SUCCESS) {
			returnCode = TPM_KeyHandleEntries_OwnerEvictGetCount
				     (&ownerEvictCount,
				      tpm_state->tpm_key_handle_entries);
		    }
		    /* check that the number of owner evict key slots will not be exceeded */
		    if (returnCode == TPM_SUCCESS) {
			if (ownerEvictCount == TPM_OWNER_EVICT_KEY_HANDLES) {
			    printf("TPM_Process_KeyControlOwner: Error, "
				   "no evict space, only %u evict slots\n",
				   TPM_OWNER_EVICT_KEY_HANDLES);
			    returnCode = TPM_NOSPACE;
			}
		    }
		    /* iii. Set ownerEvict within the internal key storage structure to TRUE. */
		    if (returnCode == TPM_SUCCESS) {
			tpm_key_handle_entry->keyControl |= TPM_KEY_CONTROL_OWNER_EVICT;
		    }
		    /* if the old value was FALSE, write the entry to NVRAM */
		    if (returnCode == TPM_SUCCESS) {
			returnCode = TPM_PermanentAll_NVStore(tpm_state,
							      TRUE,	/* write NV */
							      0);	/* no roll back */
		    }
		}
		else {	/* if the key is already owner evict, nothing to do */
		    printf("TPM_Process_KeyControlOwner: key is already owner evict\n");
		}
	    }
	    /* b. Else if bitValue == FALSE */
	    else {
		if (oldOwnerEvict) {		/* if the key is currently owner evict */
		    printf("TPM_Process_KeyControlOwner: setting key not owner evict\n");
		    /* i. Set ownerEvict within the internal key storage structure to FALSE. */
		    if (returnCode == TPM_SUCCESS) {
			tpm_key_handle_entry->keyControl &= ~TPM_KEY_CONTROL_OWNER_EVICT;
		    }
		    /* if the old value was TRUE, delete the entry from NVRAM */
		    if (returnCode == TPM_SUCCESS) {
			returnCode = TPM_PermanentAll_NVStore(tpm_state,
							      TRUE,	/* write NV */
							      0);	/* no roll back */
		    }
		}
		else { 	/* if the key is already not owner evict, nothing to do */
		    printf("TPM_Process_KeyControlOwner: key is already not owner evict\n");
		}
	    }
	    break;
	  default:
	    printf("TPM_Process_KeyControlOwner: Invalid bitName %08x\n", bitName);
	    returnCode = TPM_BAD_MODE;
	    break;
	}
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_KeyControlOwner: Ordinal returnCode %08x %u\n",
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
    TPM_Pubkey_Delete(&pubKey);		/* @1 */
    return rcf;
}

/* 27.2 Context management

  The 1.1 context commands were written for specific resource types. The 1.2 commands are generic
  for all resource types. So the Savexxx commands are replaced by TPM_SaveContext and the LoadXXX
  commands by TPM_LoadContext.
*/

/* 27.2.1 TPM_SaveKeyContext rev 87

  SaveKeyContext saves a loaded key outside the TPM. After creation of the key context blob the TPM
  automatically releases the internal memory used by that key. The format of the key context blob is
  specific to a TPM.
*/

TPM_RESULT TPM_Process_SaveKeyContext(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE	keyHandle;	/* The key which will be kept outside the TPM */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_KEY_HANDLE_ENTRY	*tpm_key_handle_entry;	/* key table entry for the key handle */
    TPM_BOOL			isZero;			/* contextNonceKey not set yet */
    TPM_CONTEXT_SENSITIVE	contextSensitive;
    TPM_STORE_BUFFER		contextSensitive_sbuffer; /* serialization of contextSensitive */
    TPM_CONTEXT_BLOB		contextBlob;
    TPM_STORE_BUFFER		contextBlob_sbuffer;		/* serialization of contextBlob */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_SaveKeyContext: Ordinal Entry\n");
    TPM_ContextSensitive_Init(&contextSensitive);	/* freed @1 */
    TPM_Sbuffer_Init(&contextSensitive_sbuffer);	/* freed @2 */
    TPM_ContextBlob_Init(&contextBlob);			/* freed @3 */
    TPM_Sbuffer_Init(&contextBlob_sbuffer);		/* freed @4 */
    /*
      get inputs
    */
    /* get keyHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&keyHandle, &command, &paramSize);
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
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_ALLOW_NO_OWNER);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_SaveKeyContext: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. This command allows saving a loaded key outside the TPM. After creation of the
       KeyContextBlob, the TPM automatically releases the internal memory used by that key. The
       format of the key context blob is specific to a TPM.

       2. A TPM protected capability belonging to the TPM that created a key context blob MUST be
       the only entity that can interpret the contents of that blob. If a cryptographic technique is
       used for this purpose, the level of security provided by that technique SHALL be at least as
       secure as a 2048 bit RSA algorithm. Any secrets (such as keys) used in such a cryptographic
       technique MUST be generated using the TPM's random number generator. Any symmetric key MUST
       be used within the power-on session during which it was created, only.

       3. A key context blob SHALL enable verification of the integrity of the contents of the blob
       by a TPM protected capability.

       4. A key context blob SHALL enable verification of the session validity of the contents of
       the blob by a TPM protected capability. The method SHALL ensure that all key context blobs
       are rendered invalid if power to the TPM is interrupted.
    */
    /* check if the key handle is valid */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_SaveKeyContext: Handle %08x\n", keyHandle);
	returnCode = TPM_KeyHandleEntries_GetEntry(&tpm_key_handle_entry,
						   tpm_state->tpm_key_handle_entries,
						   keyHandle);
    }
    /* use the contextNonceKey to invalidate a blob at power up */
    if (returnCode == TPM_SUCCESS) {
	/* If TPM_STCLEAR_DATA -> contextNonceKey is NULLS */
	TPM_Nonce_IsZero(&isZero, tpm_state->tpm_stclear_data.contextNonceKey);
	if (isZero) {
	    /* Set TPM_STCLEAR_DATA -> contextNonceKey to the next value from the TPM RNG */
	    returnCode = TPM_Nonce_Generate(tpm_state->tpm_stclear_data.contextNonceKey);
	}
    }
    /* Create internalData by putting the sensitive part of the resource pointed to by handle into a
       structure. The structure is a TPM manufacturer option. The TPM MUST ensure that ALL sensitive
       information of the resource is included in internalData.	 For a key, the sensitive part is
       the TPM_STORE_ASYMKEY */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_SaveKeyContext: Building TPM_CONTEXT_SENSITIVE\n");
	returnCode = TPM_SizedBuffer_SetStructure(&(contextSensitive.internalData),
						  tpm_key_handle_entry,
						  (TPM_STORE_FUNCTION_T)TPM_KeyHandleEntry_Store);
    }
    if (returnCode == TPM_SUCCESS) {
	/* TPM_CONTEXT_SENSITIVE -> contextNonce */
	TPM_Nonce_Copy(contextSensitive.contextNonce, tpm_state->tpm_stclear_data.contextNonceKey);
	/* TPM_CONTEXT_BLOB -> resourceType, handle, integrityDigest */
	printf("TPM_Process_SaveKeyContext: Building TPM_CONTEXT_BLOB\n");
	contextBlob.resourceType = TPM_RT_KEY;
	contextBlob.handle = keyHandle;
	contextBlob.contextCount = 0;
    }
    /* TPM_CONTEXT_BLOB -> sensitiveData */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_ContextSensitive_Store(&contextSensitive_sbuffer, &contextSensitive);
    }
    /* Here the clear text goes into TPM_CONTEXT_BLOB->sensitiveData */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_SetFromStore(&(contextBlob.sensitiveData),
						  &contextSensitive_sbuffer);
    }
    /* Calculate TPM_CONTEXT_BLOB -> integrityDigest, the HMAC of TPM_CONTEXT_BLOB using
       TPM_PERMANENT_DATA -> tpmProof as the secret */
    if (returnCode == TPM_SUCCESS) {
	/* This is a bit circular.  It's safe since the TPM_CONTEXT_BLOB is serialized before the
	   HMAC is generated.	The result is put back into the structure.  */
	printf("TPM_Process_SaveKeyContext: Digesting TPM_CONTEXT_BLOB\n");
	returnCode = TPM_HMAC_GenerateStructure
		     (contextBlob.integrityDigest,		/* HMAC */
		      tpm_state->tpm_permanent_data.tpmProof,	/* HMAC key */
		      &contextBlob,				/* structure */
		      (TPM_STORE_FUNCTION_T)TPM_ContextBlob_Store);	/* store function */
    }
    /* encrypt TPM_CONTEXT_SENSITIVE using as TPM_PERMANENT_DATA -> contextKey the key.	 Store the
       result in TPM_CONTEXT_BLOB -> sensitiveData */
    if (returnCode == TPM_SUCCESS) {
	/* The cleartext went into sensitiveData for the integrityDigest calculation.  Free it now,
	   before the encrypted data is stored there. */
	TPM_SizedBuffer_Delete(&(contextBlob.sensitiveData));
	printf("TPM_Process_SaveKeyContext: Encrypting TPM_CONTEXT_SENSITIVE\n");
	returnCode =
	    TPM_SymmetricKeyData_EncryptSbuffer(&(contextBlob.sensitiveData),
						&contextSensitive_sbuffer,
						tpm_state->tpm_permanent_data.contextKey);
    }
    /* serialize TPM_CONTEXT_BLOB */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_ContextBlob_Store(&contextBlob_sbuffer, &contextBlob);
    }
    /* invalidate the key handle and delete the key */
    if (returnCode == TPM_SUCCESS) {
	/* free the key resources, free the key itself, and remove entry from the key handle entries
	   list */
	TPM_KeyHandleEntry_Delete(tpm_key_handle_entry);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_SaveKeyContext: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return keyContextSize and keyContextBlob */
	    returnCode = TPM_Sbuffer_AppendAsSizedBuffer(response, &contextBlob_sbuffer);
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
    TPM_ContextSensitive_Delete(&contextSensitive);	/* @1 */
    TPM_Sbuffer_Delete(&contextSensitive_sbuffer);	/* @2 */
    TPM_ContextBlob_Delete(&contextBlob);		/* @3 */
    TPM_Sbuffer_Delete(&contextBlob_sbuffer);		/* @4 */
    return rcf;
}

/* 27.2.2 TPM_LoadKeyContext rev 87

  LoadKeyContext loads a key context blob into the TPM previously retrieved by a SaveKeyContext
  call. After successful completion the handle returned by this command can be used to access the
  key.
*/

TPM_RESULT TPM_Process_LoadKeyContext(tpm_state_t *tpm_state,
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
    uint32_t		keyContextSize;		/* The size of the following key context blob */
    TPM_CONTEXT_BLOB	keyContextBlob;		/* The key context blob */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    unsigned char		*stream;
    uint32_t			stream_size;
    unsigned char	*contextSensitiveBuffer;	/* decrypted sensitive data */
    uint32_t		contextSensitiveBuffer_length;	/* actual data in contextSensitiveBuffer */
    TPM_CONTEXT_SENSITIVE	contextSensitive;
    TPM_KEY_HANDLE_ENTRY	*used_key_handle_entry;
    TPM_KEY_HANDLE_ENTRY	tpm_key_handle_entry;
    TPM_RESULT			getRc;			/* is the handle value free */
    TPM_BOOL			isSpace;
    uint32_t			index;			/* free space index */
    TPM_BOOL			key_added = FALSE;	/* key has been added to handle list */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_KEY_HANDLE	keyHandle;	/* The handle assigned to the key after it has been
					   successfully loaded. */

    printf("TPM_Process_LoadKeyContext: Ordinal Entry\n");
    TPM_ContextBlob_Init(&keyContextBlob);		/* freed @1 */
    contextSensitiveBuffer = NULL;			/* freed @2 */
    TPM_ContextSensitive_Init(&contextSensitive);	/* freed @3 */
    TPM_KeyHandleEntry_Init(&tpm_key_handle_entry);	/* no free */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get keyContextSize parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&keyContextSize, &command, &paramSize);
    }
    /* get keyContextBlob parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_ContextBlob_Load(&keyContextBlob, &command, &paramSize);
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
	    printf("TPM_Process_LoadKeyContext: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. This command allows loading a key context blob into the TPM previously retrieved by a
	TPM_SaveKeyContext call. After successful completion the handle returned by this command can
	be used to access the key.

	2. The contents of a key context blob SHALL be discarded unless the contents have passed an
	integrity test.	 This test SHALL (statistically) prove that the contents of the blob are the
	same as when the blob was created.

	3. The contents of a key context blob SHALL be discarded unless the contents have passed a
	session validity test. This test SHALL (statistically) prove that the blob was created by
	this TPM during this power-on session.
    */
    if (returnCode == TPM_SUCCESS) {
	if (keyContextBlob.resourceType != TPM_RT_KEY) {
	    printf("TPM_Process_LoadKeyContext: Error, resourceType %08x should be TPM_RT_KEY\n",
		   keyContextBlob.resourceType);
	    returnCode	=TPM_BAD_PARAMETER;
	}
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadKeyContext: Decrypting TPM_CONTEXT_SENSITIVE stream\n");
	returnCode =
	    TPM_SymmetricKeyData_Decrypt(&contextSensitiveBuffer,	/* decrypted data */
					 &contextSensitiveBuffer_length, /* length decrypted data */
					 keyContextBlob.sensitiveData.buffer, /* encrypted */
					 keyContextBlob.sensitiveData.size,
					 tpm_state->tpm_permanent_data.contextKey);
    }
    /* deserialize TPM_CONTEXT_SENSITIVE */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadKeyContext: Creating TPM_CONTEXT_SENSITIVE\n");
	stream = contextSensitiveBuffer;
	stream_size = contextSensitiveBuffer_length;
	returnCode = TPM_ContextSensitive_Load(&contextSensitive, &stream, &stream_size);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadKeyContext: Loading TPM_KEY_HANDLE_ENTRY from internalData\n");
	stream = contextSensitive.internalData.buffer;
	stream_size = contextSensitive.internalData.size;
	returnCode = TPM_KeyHandleEntry_Load(&tpm_key_handle_entry, &stream, &stream_size);
    }
    /* check contextNonce */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadKeyContext: Checking TPM_CONTEXT_SENSITIVE -> contextNonce\n");
	returnCode = TPM_Nonce_Compare(tpm_state->tpm_stclear_data.contextNonceKey,
				       contextSensitive.contextNonce);
	if (returnCode != TPM_SUCCESS) {
	    printf("TPM_Process_LoadKeyContext: Error comparing contextNonceKey\n");
	    returnCode = TPM_BADCONTEXT;
	}
    }
    /* Move decrypted data back to keyContextBlob for integrityDigest check. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Set(&(keyContextBlob.sensitiveData),
					 contextSensitiveBuffer_length, contextSensitiveBuffer);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadKeyContext: Checking integrityDigest\n");
	/* make a copy of integrityDigest, because it needs to be 0 for the HMAC calculation */
	/* NOTE Done by TPM_HMAC_CheckStructure() */
	/* b. Set B1 -> integrityDigest to NULL */
	/* NOTE Done by TPM_HMAC_CheckStructure() */
	/* verify the integrityDigest HMAC of TPM_CONTEXT_BLOB using TPM_PERMANENT_DATA -> tpmProof
	   as the HMAC key */
	returnCode = TPM_HMAC_CheckStructure
		     (tpm_state->tpm_permanent_data.tpmProof,		/* key */
		      &keyContextBlob,					/* structure */
		      keyContextBlob.integrityDigest,			/* expected */
		      (TPM_STORE_FUNCTION_T)TPM_ContextBlob_Store,	/* store function */
		      TPM_BADCONTEXT);					/* error code */
    }
    /* try to use the saved handle value when possible */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadKeyContext: Checking if suggested handle %08x is free\n",
	       keyContextBlob.handle);
	/* check if the key handle is free */
	getRc = TPM_KeyHandleEntries_GetEntry(&used_key_handle_entry,
					      tpm_state->tpm_key_handle_entries,
					      keyContextBlob.handle);
	/* GetEntry TPM_SUCCESS means the handle is already used */
	if (getRc == TPM_SUCCESS) {
	    keyHandle = 0;		/* no suggested handle */
	}
	/* not success means that the handle value is not currently used */
	else {
	    keyHandle = keyContextBlob.handle;
	}
    }
    /* check that there is space in the key handle entries */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadKeyContext: Checking for table space\n");
	TPM_KeyHandleEntries_IsSpace(&isSpace, &index,
				     tpm_state->tpm_key_handle_entries);
	/* if there is no space, return error */
	if (!isSpace) {
	    printf("TPM_Process_LoadKeyContext: Error, no room in table\n");
	    returnCode = TPM_RESOURCES;
	}
    }
    /* restore the entity, try to keep the handle as 'handle' */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadKeyContext: Adding entry to table\n");
	returnCode = TPM_KeyHandleEntries_AddEntry(&keyHandle,
						   FALSE,		/* keep handle */
						   tpm_state->tpm_key_handle_entries,
						   &tpm_key_handle_entry);
	key_added = TRUE;
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_LoadKeyContext: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* return keyHandle */
	    returnCode = TPM_Sbuffer_Append32(response, keyHandle);
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
    TPM_ContextBlob_Delete(&keyContextBlob);		/* @1 */
    free(contextSensitiveBuffer);			/* @2 */
    TPM_ContextSensitive_Delete(&contextSensitive);	/* @3 */
    /* if there was a failure, roll back */
    if ((rcf != 0) || (returnCode != TPM_SUCCESS)) {
	TPM_Key_Delete(tpm_key_handle_entry.key);	/* @5 */
	free(tpm_key_handle_entry.key);			/* @5 */
	if (key_added) {
	    /* if there was a failure and a key was stored in the handle list, free the handle.
	       Ignore errors, since only one error code can be returned. */
	    TPM_KeyHandleEntries_DeleteHandle(tpm_state->tpm_key_handle_entries, keyHandle);
	}
    }
    return rcf;
}

/* 27.2.3 TPM_SaveAuthContext rev 87

  SaveAuthContext saves a loaded authorization session outside the TPM. After creation of the
  authorization context blob, the TPM automatically releases the internal memory used by that
  session. The format of the authorization context blob is specific to a TPM.
*/

TPM_RESULT TPM_Process_SaveAuthContext(tpm_state_t *tpm_state,
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
    TPM_AUTHHANDLE	authHandle;	/* Authorization session which will be kept outside the TPM
					 */
    
    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_AUTH_SESSION_DATA	*tpm_auth_session_data; /* session table entry for the handle */
    TPM_BOOL			isZero;			/* contextNonceSession not set yet */
    TPM_STCLEAR_DATA		*v1StClearData = NULL;
    uint32_t			contextIndex = 0;	/* free index in context list */
    uint32_t			space;			/* free space in context list */
    TPM_CONTEXT_SENSITIVE	contextSensitive;
    TPM_STORE_BUFFER		contextSensitive_sbuffer; /* serialization of contextSensitive */
    TPM_CONTEXT_BLOB		contextBlob;
    TPM_STORE_BUFFER		contextBlob_sbuffer;	/* serialization of contextBlob */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_SaveAuthContext: Ordinal Entry\n");
    TPM_ContextSensitive_Init(&contextSensitive);	/* freed @1 */
    TPM_Sbuffer_Init(&contextSensitive_sbuffer);	/* freed @2 */
    TPM_ContextBlob_Init(&contextBlob);			/* freed @3 */
    TPM_Sbuffer_Init(&contextBlob_sbuffer);		/* freed @4 */
    /*
      get inputs
    */
    /* get authHandle */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&authHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_SaveAuthContext: authHandle %08x\n", authHandle);
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
	    printf("TPM_Process_SaveAuthContext: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* This command allows saving a loaded authorization session outside the TPM. After creation of
       the authContextBlob, the TPM automatically releases the internal memory used by that
       session. The format of the authorization context blob is specific to a TPM.

       A TPM protected capability belonging to the TPM that created an authorization context blob
       MUST be the only entity that can interpret the contents of that blob. If a cryptographic
       technique is used for this purpose, the level of security provided by that technique SHALL be
       at least as secure as a 2048 bit RSA algorithm. Any secrets (such as keys) used in such a
       cryptographic technique MUST be generated using the TPM's random number generator. Any
       symmetric key MUST be used within the power-on session during which it was created, only.

       An authorization context blob SHALL enable verification of the integrity of the contents of
       the blob by a TPM protected capability.

       An authorization context blob SHALL enable verification of the session validity of the
       contents of the blob by a TPM protected capability. The method SHALL ensure that all
       authorization context blobs are rendered invalid if power to the TPM is interrupted.
    */
    /* 1. Map V1 to TPM_STANY_DATA NOTE MAY be TPM_STCLEAR_DATA */
    if (returnCode == TPM_SUCCESS) {
	v1StClearData = &(tpm_state->tpm_stclear_data);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_SaveAuthContext: Handle %08x\n", authHandle);
	returnCode = TPM_AuthSessions_GetEntry(&tpm_auth_session_data,
					       v1StClearData->authSessions,
					       authHandle);
    }
    if (returnCode == TPM_SUCCESS) {
	/* If TPM_STANY_DATA -> contextNonceSession is NULLS */
	TPM_Nonce_IsZero(&isZero, v1StClearData->contextNonceSession);
	if (isZero) {
	    /* Set TPM_STANY_DATA -> contextNonceSession to the next value from the TPM RNG */
	    returnCode = TPM_Nonce_Generate(v1StClearData->contextNonceSession);
	}
    }
    /* Create internalData by putting the sensitive part of the resource pointed to by handle into a
       structure. The structure is a TPM manufacturer option. The TPM MUST ensure that ALL sensitive
       information of the resource is included in internalData.	 For a session, the entire structure
       can fit in the sensitive part. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_SaveAuthContext: Building TPM_CONTEXT_SENSITIVE\n");
	returnCode = TPM_SizedBuffer_SetStructure(&(contextSensitive.internalData),
						  tpm_auth_session_data,
						  (TPM_STORE_FUNCTION_T)TPM_AuthSessionData_Store);
    }
    if (returnCode == TPM_SUCCESS) {
    }
    if (returnCode == TPM_SUCCESS) {
	/* TPM_CONTEXT_SENSITIVE -> contextNonce */
	TPM_Nonce_Copy(contextSensitive.contextNonce, v1StClearData->contextNonceSession);
	/* TPM_CONTEXT_BLOB -> resourceType, handle, integrityDigest */
	printf("TPM_Process_SaveAuthContext: Building TPM_CONTEXT_BLOB\n");
	contextBlob.resourceType = TPM_RT_AUTH;
	contextBlob.handle = authHandle;
	TPM_Digest_Init(contextBlob.integrityDigest);
    }
    /* TPM_CONTEXT_BLOB -> sensitiveData */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_ContextSensitive_Store(&contextSensitive_sbuffer, &contextSensitive);
    }
    /* Here the clear text goes into TPM_CONTEXT_BLOB->sensitiveData */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_SetFromStore(&(contextBlob.sensitiveData),
						  &contextSensitive_sbuffer);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_SaveAuthContext: Processing session context count\n");
	/* a. If V1 -> contextCount > 2^32-2 then */
	if (v1StClearData->contextCount > 0xfffffffe) {
	    /* i. Return with TPM_TOOMANYCONTEXTS */
	    printf("TPM_Process_SaveAuthContext: Error, too many contexts\n");
	    returnCode = TPM_TOOMANYCONTEXTS;
	}
    }
    /* b. Else */
    if (returnCode == TPM_SUCCESS) {
	/* i. Increment V1 -> contextCount by 1 */
	v1StClearData->contextCount++;
	/* ii. Validate that the TPM can still manage the new count value */
	/* (1) If the distance between the oldest saved context and the contextCount is
	   too large return TPM_CONTEXT_GAP */
	/* Since contextCount is uint32_t, this is not applicable here.  From email: Does the
	   TPM have the ability to keep track of the context delta. It is possible to keep
	   track of things with just a byte or so internally, if this is done a gap of
	   greater than 2^16 or so might be too large, hence the context gap message */
    }
    /* iii. Find contextIndex such that V1 -> contextList[contextIndex] equals 0. If not
       found exit with TPM_NOCONTEXTSPACE */
    if (returnCode == TPM_SUCCESS) {
	TPM_ContextList_GetSpace(&space, &contextIndex, v1StClearData->contextList);
	if (space == 0) {
	    printf("TPM_Process_SaveAuthContext: Error, no space in context list\n");
	    returnCode = TPM_NOCONTEXTSPACE;
	}
    }
    if (returnCode == TPM_SUCCESS) {
	/* iv. Set V1-> contextList[contextIndex] to V1 -> contextCount */
	v1StClearData->contextList[contextIndex] = v1StClearData->contextCount;
	/* v. Set B1 -> contextCount to V1 -> contextCount */
	contextBlob.contextCount = v1StClearData->contextCount;
    }	
    /* c. The TPM MUST invalidate all information regarding the resource except for information
       needed for reloading */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_TerminateHandle(v1StClearData->authSessions, authHandle);
    }
    /* Calculate TPM_CONTEXT_BLOB -> integrityDigest, the HMAC of TPM_CONTEXT_BLOB using
       TPM_PERMANENT_DATA -> tpmProof as the secret */
    if (returnCode == TPM_SUCCESS) {
	/* This is a bit circular.  It's safe since the TPM_CONTEXT_BLOB is serialized before the
	   HMAC is generated.	The result is put back into the structure.  */
	printf("TPM_Process_SaveAuthContext: Digesting TPM_CONTEXT_BLOB\n");
	returnCode = TPM_HMAC_GenerateStructure
		     (contextBlob.integrityDigest,		/* HMAC */
		      tpm_state->tpm_permanent_data.tpmProof,	/* HMAC key */
		      &contextBlob,				/* structure */
		      (TPM_STORE_FUNCTION_T)TPM_ContextBlob_Store);	/* store function */
    }
    /* encrypt TPM_CONTEXT_SENSITIVE using as TPM_PERMANENT_DATA -> contextKey the key.	 Store the
       result in TPM_CONTEXT_BLOB -> sensitiveData */
    if (returnCode == TPM_SUCCESS) {
	/* The cleartext went into sensitiveData for the integrityDigest calculation.  Free it now,
	   before the encrypted data is stored there. */
	TPM_SizedBuffer_Delete(&(contextBlob.sensitiveData));
	printf("TPM_Process_SaveAuthContext: Encrypting TPM_CONTEXT_SENSITIVE\n");
	returnCode =
	    TPM_SymmetricKeyData_EncryptSbuffer(&(contextBlob.sensitiveData),
						&contextSensitive_sbuffer,
						tpm_state->tpm_permanent_data.contextKey);
    }
    /* serialize TPM_CONTEXT_BLOB */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_ContextBlob_Store(&contextBlob_sbuffer, &contextBlob);
    }
     /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_SaveAuthContext: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return authContextSize and authContextBlob */
	    returnCode = TPM_Sbuffer_AppendAsSizedBuffer(response, &contextBlob_sbuffer);
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
    TPM_ContextSensitive_Delete(&contextSensitive);	/* @1 */
    TPM_Sbuffer_Delete(&contextSensitive_sbuffer);	/* @2 */
    TPM_ContextBlob_Delete(&contextBlob);		/* @3 */
    TPM_Sbuffer_Delete(&contextBlob_sbuffer);		/* @4 */
    return rcf;
}

/* 27.2.4 TPM_LoadAuthContext rev 106

   LoadAuthContext loads an authorization context blob into the TPM previously retrieved by a
   SaveAuthContext call. After successful completion, the handle returned by this command can be used
   to access the authorization session.
*/

TPM_RESULT TPM_Process_LoadAuthContext(tpm_state_t *tpm_state,
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
    uint32_t		authContextSize;	/* The size of the following auth context blob */
    TPM_CONTEXT_BLOB	authContextBlob;	/* The auth context blob */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    unsigned char		*stream;
    uint32_t			stream_size;
    unsigned char	*contextSensitiveBuffer;	/* decrypted sensitive data */
    uint32_t		contextSensitiveBuffer_length;	/* actual data in contextSensitiveBuffer */
    TPM_CONTEXT_SENSITIVE	contextSensitive;
    TPM_AUTH_SESSION_DATA	tpm_auth_session_data;
    TPM_AUTH_SESSION_DATA	*used_auth_session_data;
    TPM_RESULT			getRc;			/* is the handle value free */
    TPM_BOOL			isSpace;
    uint32_t			index;			/* free space index */
    TPM_BOOL	auth_session_added = FALSE;	/* session key has been added to handle list */
    TPM_STCLEAR_DATA		*v1StClearData = NULL;
    uint32_t			contextIndex;
    TPM_DIGEST			entityDigest;		/* digest of the entity used to set up the
							   OSAP or DSAP session */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_KEY_HANDLE	authHandle;	/* The handle assigned to the authorization session after it
					   has been successfully loaded. */

    printf("TPM_Process_LoadAuthContext: Ordinal Entry\n");
    TPM_ContextBlob_Init(&authContextBlob);		/* freed @1 */
    contextSensitiveBuffer = NULL;			/* freed @2 */
    TPM_ContextSensitive_Init(&contextSensitive);	/* freed @3 */
    TPM_AuthSessionData_Init(&tpm_auth_session_data);	/* freed @4 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get authContextSize parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&authContextSize, &command, &paramSize);
    }
    /* get authContextBlob parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_ContextBlob_Load(&authContextBlob, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadAuthContext: handle %08x\n", authContextBlob.handle);
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
	    printf("TPM_Process_LoadAuthContext: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* This command allows loading an authorization context blob into the TPM previously retrieved
       by a TPM_SaveAuthContext call. After successful completion, the handle returned by this
       command can be used to access the authorization session.

       The contents of an authorization context blob SHALL be discarded unless the contents have
       passed an integrity test. This test SHALL (statistically) prove that the contents of the blob
       are the same as when the blob was created.

       The contents of an authorization context blob SHALL be discarded unless the contents have
       passed a session validity test. This test SHALL (statistically) prove that the blob was
       created by this TPM during this power-on session.

       For an OSAP authorization context blob referring to a key, verify that the key linked to this
       session is resident in the TPM.
    */
    if (returnCode == TPM_SUCCESS) {
	/* 2. Map V1 to TPM_STANY_DATA NOTE MAY be TPM_STCLEAR_DATA */
	v1StClearData = &(tpm_state->tpm_stclear_data);
	if (authContextBlob.resourceType != TPM_RT_AUTH) {
	    printf("TPM_Process_LoadAuthContext: Error, resourceType %08x should be TPM_RT_AUTH\n",
		   authContextBlob.resourceType);
	    returnCode = TPM_BAD_PARAMETER;
	}
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadAuthContext: Decrypting TPM_CONTEXT_SENSITIVE stream\n");
	returnCode =
	    TPM_SymmetricKeyData_Decrypt(&contextSensitiveBuffer,	 /* decrypted data */
					 &contextSensitiveBuffer_length, /* length decrypted data */
					 authContextBlob.sensitiveData.buffer, /* encrypted */
					 authContextBlob.sensitiveData.size,
					 tpm_state->tpm_permanent_data.contextKey);
    }
    /* deserialize TPM_CONTEXT_SENSITIVE */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadAuthContext: Creating TPM_CONTEXT_SENSITIVE\n");
	stream = contextSensitiveBuffer;
	stream_size = contextSensitiveBuffer_length;
	returnCode = TPM_ContextSensitive_Load(&contextSensitive,
					       &stream,
					       &stream_size);
    }
    /* Parse the TPM_CONTEXT_SENSITIVE -> internalData to TPM_AUTH_SESSION_DATA	 */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadAuthContext: Loading TPM_AUTH_SESSION_DATA from internalData\n");
	stream = contextSensitive.internalData.buffer;
	stream_size = contextSensitive.internalData.size;
	returnCode = TPM_AuthSessionData_Load(&tpm_auth_session_data, &stream, &stream_size);
    }
    /* check contextNonce */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadAuthContext: protocolID %04x entityTypeByte %02x\n",
	       tpm_auth_session_data.protocolID, tpm_auth_session_data.entityTypeByte);
	printf("TPM_Process_LoadAuthContext: Checking TPM_CONTEXT_SENSITIVE -> contextNonce\n");
	returnCode = TPM_Nonce_Compare(v1StClearData->contextNonceSession,
				       contextSensitive.contextNonce);
	if (returnCode != TPM_SUCCESS) {
	    printf("TPM_Process_LoadAuthContext: Error comparing contextNonceSession\n");
	    returnCode = TPM_BADCONTEXT;
	}
    }
    if (returnCode == TPM_SUCCESS) {
	if ((tpm_auth_session_data.protocolID == TPM_PID_OSAP) ||
	    (tpm_auth_session_data.protocolID == TPM_PID_DSAP)) {
	    /* check that the entity is loaded, and that the entity's digest equals that of the OSAP
	       or DSAP session */
	    switch (tpm_auth_session_data.entityTypeByte) {
	      case TPM_ET_OWNER:
		printf("TPM_Process_LoadAuthContext: Owner OSAP/DSAP session\n");
		/* check for owner */
		if (returnCode == TPM_SUCCESS) {
		    returnCode = TPM_LoadContext_CheckOwnerLoaded(tpm_state, entityDigest);
		}
		/* compare entity digest */
		if (returnCode == TPM_SUCCESS) {
		    returnCode= TPM_Digest_Compare(entityDigest,
						   tpm_auth_session_data.entityDigest);
		    if (returnCode != TPM_SUCCESS) {
			printf("TPM_Process_LoadAuthContext: "
			       "Error, OSAP or DSAP entityDigest mismatch\n");
			returnCode = TPM_RESOURCEMISSING;
		    }
		}
		break;
	      case TPM_ET_SRK:
		printf("TPM_Process_LoadAuthContext: SRK OSAP/DSAP session\n");
		/* check for SRK */
		if (returnCode == TPM_SUCCESS) {
		    returnCode = TPM_LoadContext_CheckSrkLoaded(tpm_state, entityDigest);
		}
		/* compare entity digest */
		if (returnCode == TPM_SUCCESS) {
		    returnCode= TPM_Digest_Compare(entityDigest,
						   tpm_auth_session_data.entityDigest);
		    if (returnCode != TPM_SUCCESS) {
			printf("TPM_Process_LoadAuthContext: "
			       "Error, OSAP or DSAP entityDigest mismatch\n");
			returnCode = TPM_RESOURCEMISSING;
		    }
		}
		break;
	      case TPM_ET_KEYHANDLE:
		printf("TPM_Process_LoadAuthContext: Key OSAP/DSAP session\n");
		/* for keys */
		returnCode =
		    TPM_LoadContext_CheckKeyLoadedByDigest(tpm_state,
							   tpm_auth_session_data.entityDigest);
		break;
	      case TPM_ET_COUNTER:
		printf("TPM_Process_LoadAuthContext: Counter OSAP/DSAP session\n");
#if 0	/* TPM_LoadAuthContext is a deprecated 1.1 command, where there was no counter */
		returnCode =
		    TPM_LoadContext_CheckCounterLoaded(tpm_state,
						       entityHandle,
						       entityDigest);
#endif
		break;
	      case TPM_ET_NV:
		printf("TPM_Process_LoadAuthContext: NV OSAP/DSAP session\n");
#if 0	/* TPM_LoadAuthContext is a deprecated 1.1 command, where there was no NV space */
		returnCode =
		    TPM_LoadContext_CheckNvLoaded(tpm_state,
						  entityHandle,
						  entityDigest);
#endif
		break;
	      default:
		printf("TPM_Process_LoadAuthContext: Error, invalid session entityType %02x\n",
		       tpm_auth_session_data.entityTypeByte);
		returnCode = TPM_WRONG_ENTITYTYPE;
		break;
	    }
	}
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadAuthContext: Checking integrityDigest\n");
	/* b. Set B1 -> integrityDigest to NULL */
	/* NOTE Done by TPM_HMAC_CheckStructure() */
	/* c. Copy M1 to B1 -> sensitiveData (integrityDigest HMAC uses cleartext) */
	returnCode = TPM_SizedBuffer_Set(&(authContextBlob.sensitiveData),
					 contextSensitiveBuffer_length, contextSensitiveBuffer);
	/* verify the integrityDigest HMAC of TPM_CONTEXT_BLOB using TPM_PERMANENT_DATA -> tpmProof
	   as the HMAC key */
	returnCode = TPM_HMAC_CheckStructure
		     (tpm_state->tpm_permanent_data.tpmProof,		/* key */
		      &authContextBlob,					/* structure */
		      authContextBlob.integrityDigest,			/* expected */
		      (TPM_STORE_FUNCTION_T)TPM_ContextBlob_Store,	/* store function */
		      TPM_BADCONTEXT);					/* error code */
    }
    /* try to use the saved handle value when possible */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadAuthContext: Checking if suggested handle %08x is free\n",
	       authContextBlob.handle);
	/* check if the auth handle is free */
	getRc = TPM_AuthSessions_GetEntry(&used_auth_session_data,
					  tpm_state->tpm_stclear_data.authSessions,
					  authContextBlob.handle);
	/* GetEntry TPM_SUCCESS means the handle is already used */
	if (getRc == TPM_SUCCESS) {
	    authHandle = 0;		/* no suggested handle */
	}
	/* not success means that the handle value is not currently used */
	else {
	    authHandle = authContextBlob.handle;
	}
    }
    /* check that there is space in the authorization handle entries */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadAuthContext: Checking for table space\n");
	TPM_AuthSessions_IsSpace(&isSpace, &index,
				 tpm_state->tpm_stclear_data.authSessions);
	/* if there is no space, return error */
	if (!isSpace) {
	    printf("TPM_Process_LoadAuthContext: Error, no room in table\n");
	    TPM_AuthSessions_Trace(tpm_state->tpm_stclear_data.authSessions);
	    returnCode = TPM_RESOURCES;
	}
    }
    /* a. Find contextIndex such that V1 -> contextList[contextIndex] equals B1 ->
       TPM_CONTEXT_BLOB -> contextCount */
    /* b. If not found then return TPM_BADCONTEXT */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadAuthContext: Checking contextCount\n");
	returnCode = TPM_ContextList_GetEntry(&contextIndex,
					      v1StClearData->contextList,
					      authContextBlob.contextCount);
    }
    /* c. Set V1 -> contextList[contextIndex] to 0 */
    if (returnCode == TPM_SUCCESS) {
	v1StClearData->contextList[contextIndex] = 0;
    }
    /* restore the entity, try to keep the handle as 'handle' */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_AddEntry(&authHandle,		/* input/output */
					       FALSE,			/* keepHandle */
					       v1StClearData->authSessions,
					       &tpm_auth_session_data);
	auth_session_added = TRUE;
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_LoadAuthContext: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* return authHandle */
	    returnCode = TPM_Sbuffer_Append32(response, authHandle);
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
    TPM_ContextBlob_Delete(&authContextBlob);		/* @1 */
    free(contextSensitiveBuffer);			/* @2 */
    TPM_ContextSensitive_Delete(&contextSensitive);	/* @3 */
    TPM_AuthSessionData_Delete(&tpm_auth_session_data); /* @4 */
    /* if there was a failure, roll back */
    if ((rcf != 0) || (returnCode != TPM_SUCCESS)) {
	if (auth_session_added) {
	    TPM_AuthSessionData_Delete(&tpm_auth_session_data);
	}
    }
    return rcf;
}
