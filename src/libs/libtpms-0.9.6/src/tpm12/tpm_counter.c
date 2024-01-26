/********************************************************************************/
/*										*/
/*				Counter Handler					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_counter.c $		*/
/*										*/
//* (c) Copyright IBM Corporation 2006, 2010.					*/
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

#include "tpm_auth.h"
#include "tpm_cryptoh.h"
#include "tpm_debug.h"
#include "tpm_digest.h"
#include "tpm_error.h"
#include "tpm_io.h"
#include "tpm_permanent.h"
#include "tpm_process.h"
#include "tpm_secret.h"

#include "tpm_counter.h"

/*
  Monotonic Counter Resource Handling
*/

/* TPM_Counters_Init() initializes the monotonic counters
 */

void TPM_Counters_Init(TPM_COUNTER_VALUE *monotonicCounters)
{
    uint32_t	i;
    
    for (i = 0 ; i < TPM_MIN_COUNTERS ; i++) {
	TPM_CounterValue_Init(&(monotonicCounters[i]));
    }
    return;
}

/* TPM_Counters_Load() loads the monotonic counters

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
*/

TPM_RESULT TPM_Counters_Load(TPM_COUNTER_VALUE *monotonicCounters,
			     unsigned char **stream,
			     uint32_t *stream_size)
{
    TPM_RESULT	rc = 0;
    uint32_t	i;

    /* load the counters */
    for (i = 0 ; (rc == 0) && (i < TPM_MIN_COUNTERS) ; i++) {
	rc = TPM_CounterValue_Load(&(monotonicCounters[i]), stream, stream_size);
    }
    return rc;
}

TPM_RESULT TPM_Counters_Store(TPM_STORE_BUFFER *sbuffer,
			      TPM_COUNTER_VALUE *monotonicCounters)
{
    TPM_RESULT	rc = 0;
    uint32_t	i;

    /* store the counters */
    for (i = 0 ; (rc == 0) && (i < TPM_MIN_COUNTERS) ; i++)  {
	rc = TPM_CounterValue_Store(sbuffer, &(monotonicCounters[i]));
    }
    return rc;
}

/* TPM_Counters_StoreHandles() stores a count of the created counters and a list of created counter
   handles.
*/

TPM_RESULT TPM_Counters_StoreHandles(TPM_STORE_BUFFER *sbuffer,
				     TPM_COUNTER_VALUE *monotonicCounters)
{
    TPM_RESULT	rc = 0;
    uint16_t	loaded;
    uint32_t	i;

    printf(" TPM_Counters_StoreHandles:\n");
    if (rc == 0) {
	loaded = 0;
	/* count the number of loaded counters */
	for (i = 0 ; i < TPM_MIN_COUNTERS ; i++) {
	    if ((monotonicCounters[i]).valid) {
		loaded++;
	    }
	}
	/* store created handle count */
	rc = TPM_Sbuffer_Append16(sbuffer, loaded); 
    }
    for (i = 0 ; (rc == 0) && (i < TPM_MIN_COUNTERS) ; i++) {
	if ((monotonicCounters[i]).valid) {
	    /* the handle is just the index */
	    rc = TPM_Sbuffer_Append32(sbuffer, i);	/* store it */
	}
    }
    return rc;
}

/* TPM_Counters_GetSpace() returns the number of unused monotonicCounters.
 */

void TPM_Counters_GetSpace(uint32_t *space,
			   TPM_COUNTER_VALUE *monotonicCounters)
{
    uint32_t i;

    printf(" TPM_Counters_GetSpace:\n");
    for (*space = 0 , i = 0 ; i < TPM_MIN_COUNTERS ; i++) {
	if (!(monotonicCounters[i]).valid) {
	    (*space)++;
	}	    
    }
    return;
}

    
/* TPM_Counters_GetNewHandle() checks for space in the monotonicCounters table.

   If there is space, it returns a TPM_COUNTER_VALUE entry in 'tpm_counter_value' and its
   handle in 'countID'.	 The entry is marked 'valid'.

   Returns TPM_RESOURCES if there is no space in the sessions table.  monotonicCounters is not
   altered on error.
*/

TPM_RESULT TPM_Counters_GetNewHandle(TPM_COUNTER_VALUE **tpm_counter_value,
				     TPM_COUNT_ID *countID,
				     TPM_COUNTER_VALUE *monotonicCounters)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL is_space;
    
    printf(" TPM_Counters_GetNewHandle:\n");
    for (*countID = 0, is_space = FALSE ;
	 *countID < TPM_MIN_COUNTERS ;
	 (*countID)++) {
	
	if (!(monotonicCounters[*countID]).valid) {
	    is_space = TRUE;
	    break;
	}	    
    }
    /* NOTE: According to TPMWG email, TPM_COUNT_ID can be an index */
    if (is_space) {
	printf("  TPM_Counters_GetNewHandle: Assigned handle %u\n", *countID);
	*tpm_counter_value = &(monotonicCounters[*countID]);
	(*tpm_counter_value)->valid = TRUE;			/* mark it occupied */
    }
    else {
	printf("TPM_Counters_GetNewHandle: Error, no space in monotonicCounters table\n");
	rc = TPM_RESOURCES;
    }
    return rc;
}

/* TPM_Counters_GetNextCount() searches the monotonicCounters for the maximum count, and returns
   nextCount equal to the incremented maximum count.

   The counter does not have to be valid (created).  It can be invalid (released).
*/

void TPM_Counters_GetNextCount(TPM_ACTUAL_COUNT *nextCount,
			       TPM_COUNTER_VALUE *monotonicCounters)
{
    TPM_COUNT_ID countID;
    TPM_ACTUAL_COUNT maxCount = 0;

    printf(" TPM_Counters_GetNextCount:\n");
    for (countID = 0 ; countID < TPM_MIN_COUNTERS ; countID++) {
	if (monotonicCounters[countID].counter > maxCount) {
	    maxCount = monotonicCounters[countID].counter;
	}
    }
    *nextCount = maxCount + 1;
    printf("  TPM_Counters_GetNextCount: Next count %u\n", *nextCount);
    return;
}

/* TPM_Counters_IsValidId() verifies that countID is in range and a created counter
 */

TPM_RESULT TPM_Counters_IsValidId(TPM_COUNTER_VALUE *monotonicCounters,
				  TPM_COUNT_ID countID)
{
    TPM_RESULT		rc = 0;
   
    printf(" TPM_Counters_IsValidId: countID %u\n", countID);
    /* range check */
    if (rc == 0) {
	if (countID >= TPM_MIN_COUNTERS) {
	    printf("TPM_Counters_IsValidId: Error countID %u out of range\n", countID);
	    rc = TPM_BAD_COUNTER ;
	}
    }
    /* validity (creation) check */
    if (rc == 0) {
	if (!(monotonicCounters[countID].valid)) {
	    printf("TPM_Counters_IsValidId: Error countID %u invalid\n", countID);
	    rc = TPM_BAD_COUNTER ;
	}	    
    }
    return rc;
}


/* TPM_Counters_GetCounterValue() gets the TPM_COUNTER_VALUE associated with the countID.

 */

TPM_RESULT TPM_Counters_GetCounterValue(TPM_COUNTER_VALUE **tpm_counter_value,
					TPM_COUNTER_VALUE *monotonicCounters,
					TPM_COUNT_ID countID)
{
    TPM_RESULT		rc = 0;
    
    printf(" TPM_Counters_GetCounterValue: countID %u\n", countID);
    /* valid counter check */
    if (rc == 0) {
	rc = TPM_Counters_IsValidId(monotonicCounters, countID);
    }
    if (rc == 0) {
	*tpm_counter_value = &(monotonicCounters[countID]);
    }
    return rc;
}

/* TPM_Counters_Release() iterates through all monotonicCounter's, and releases those that are
   created.

   The resource is set invalid, and the authorization data and digest are cleared.

   a. This includes invalidating all currently allocated counters. The result will be no
   currently allocated counters and the new owner will need to allocate counters. The actual
   count value will continue to increase.
*/

TPM_RESULT TPM_Counters_Release(TPM_COUNTER_VALUE *monotonicCounters)
{
    TPM_RESULT	 rc = 0;
    TPM_COUNT_ID i;
    
    printf(" TPM_Counters_Release:\n");
    for (i = 0 ; i < TPM_MIN_COUNTERS ; i++) {
	if (monotonicCounters[i].valid) {
	    /* the actual count value does not reset to zero */
	    printf(" TPM_Counters_Release: Releasing %u\n", i);
	    TPM_Secret_Init(monotonicCounters[i].authData);
	    TPM_Digest_Init(monotonicCounters[i].digest);
	    monotonicCounters[i].valid = FALSE;
	}
    }
    return rc;
}

/* TPM_Counters_GetActiveCounter() gets the active counter based on the value in TPM_STCLEAR_DATA ->
   countID */

void TPM_Counters_GetActiveCounter(TPM_COUNT_ID *activeCounter,
				   TPM_COUNT_ID countID)
{
    if (countID < TPM_MIN_COUNTERS) {
	*activeCounter = countID;
    }
    else {
	*activeCounter = TPM_COUNT_ID_NULL;
    }
}

/*
  TPM_COUNTER_VALUE
*/

/* TPM_CounterValue_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_CounterValue_Init(TPM_COUNTER_VALUE *tpm_counter_value)
{
    printf(" TPM_CounterValue_Init:\n");
    memset(tpm_counter_value->label, 0, TPM_COUNTER_LABEL_SIZE);
    tpm_counter_value->counter = 0;
    TPM_Secret_Init(tpm_counter_value->authData);
    tpm_counter_value->valid = FALSE;
    return;
}

/* TPM_CounterValue_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
*/

TPM_RESULT TPM_CounterValue_Load(TPM_COUNTER_VALUE *tpm_counter_value,	/* result */
				 unsigned char **stream,		/* pointer to next
									   parameter */
				 uint32_t *stream_size)			/* stream size left */
{
    TPM_RESULT	rc = 0;
    
    printf(" TPM_CounterValue_Load:\n");
    /* check tag */
    if (rc == 0) {	
	rc = TPM_CheckTag(TPM_TAG_COUNTER_VALUE, stream, stream_size);
    }
    /* load label */
    if (rc == 0) {
	rc = TPM_Loadn(tpm_counter_value->label, TPM_COUNTER_LABEL_SIZE, stream, stream_size);
    }
    /* load counter */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_counter_value->counter), stream, stream_size);
    }	
    /* load authData */
    if (rc == 0) {
	rc = TPM_Secret_Load(tpm_counter_value->authData, stream, stream_size);
    }	
    /* load valid */
    if (rc == 0) {
	rc = TPM_LoadBool(&(tpm_counter_value->valid), stream, stream_size);
    }	
    return rc;
}

/* TPM_CounterValue_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes

   It is typically used to store the structure in the permanent data file.
*/

TPM_RESULT TPM_CounterValue_Store(TPM_STORE_BUFFER *sbuffer,
				  const TPM_COUNTER_VALUE *tpm_counter_value)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_CounterValue_Store:\n");
    /* store tag, label, counter */
    if (rc == 0) {	
	rc = TPM_CounterValue_StorePublic(sbuffer, tpm_counter_value); 
    }
    /* store authData */
    if (rc == 0) {
	rc = TPM_Secret_Store(sbuffer, tpm_counter_value->authData);
    }	
    /* store valid */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_counter_value->valid), sizeof(TPM_BOOL));
    }	
    return rc;
}

/* TPM_CounterValue_StorePublic()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes

   This version only stores the public, externally visible fields: tag, label, counter.	 It is
   typically used to return outgoing parameters.
*/

TPM_RESULT TPM_CounterValue_StorePublic(TPM_STORE_BUFFER *sbuffer,
					const TPM_COUNTER_VALUE *tpm_counter_value)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_CounterValue_StorePublic:\n");
    /* store tag */
    if (rc == 0) {	
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_COUNTER_VALUE); 
    }
    /* store label */
    if (rc == 0) {	
	rc = TPM_Sbuffer_Append(sbuffer, tpm_counter_value->label, TPM_COUNTER_LABEL_SIZE);
    }
    /* store counter */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_counter_value->counter); 
    }
    return rc;
}

/* TPM_CounterValue_CopyPublic() copies the public, externally visible fields: tag, label, counter.
 */

void TPM_CounterValue_CopyPublic(TPM_COUNTER_VALUE *dst_tpm_counter_value,
				 TPM_COUNTER_VALUE *src_tpm_counter_value)
{
    memcpy(dst_tpm_counter_value->label, src_tpm_counter_value->label, TPM_COUNTER_LABEL_SIZE);
    dst_tpm_counter_value->counter = src_tpm_counter_value->counter;
    return;
}

/* TPM_CounterValue_Set()
   
   Sets the label, counter, and authData members from input parameters, and sets the digest from
   members.
*/

TPM_RESULT TPM_CounterValue_Set(TPM_COUNTER_VALUE *tpm_counter_value,
				TPM_COUNT_ID countID,
				BYTE *label,
				TPM_ACTUAL_COUNT counter,
				TPM_SECRET authData)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_CounterValue_Set:\n");
    tpm_counter_value->counter = counter;
    memcpy(tpm_counter_value->label, label, TPM_COUNTER_LABEL_SIZE);
    TPM_Secret_Copy(tpm_counter_value->authData, authData);
    /* create a hopefully unique digest of the object for the OSAP setup.  The cast is OK here since
       the actual value of the digest is never verified. */
    rc = TPM_SHA1(tpm_counter_value->digest,
		  sizeof(TPM_COUNT_ID), (unsigned char *)&countID, 
		  TPM_COUNTER_LABEL_SIZE, label,
		  TPM_SECRET_SIZE, authData,
		  0, NULL);
    return rc;
 
}

/* TPM_CounterValue_Release() releases a counter.

   The resource is set invalid, and the authorization data and digest are cleared.
*/

TPM_RESULT TPM_CounterValue_Release(TPM_COUNTER_VALUE *tpm_counter_value,
				    TPM_COUNT_ID countID)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_CounterValue_Release: countID %u\n", countID);
    /* sanity check */
    if (rc == 0) {
	if (!tpm_counter_value->valid) {
	    printf("TPM_CounterValue_Release: Error (fatal), countID %u not valid\n", countID);
	    rc = TPM_FAIL;	/* should never occur */
	}
    }
    if (rc == 0) {
	TPM_Secret_Init(tpm_counter_value->authData);
	TPM_Digest_Init(tpm_counter_value->digest);
	tpm_counter_value->valid = FALSE;
    }
    return rc;
}

/*
  Processing Functions
*/

/* 25.1 TPM_CreateCounter rev 98

  This command creates the counter but does not select the counter. Counter creation assigns an
  AuthData value to the counter and sets the counters original start value. The original start value
  is the current internal base value plus one. Setting the new counter to the internal base avoids
  attacks on the system that are attempting to use old counter values.

  This command creates a new monotonic counter. The TPM MUST support a minimum of 4 concurrent 
  counters.
*/

TPM_RESULT TPM_Process_CreateCounter(tpm_state_t *tpm_state,
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
    TPM_ENCAUTH encAuth;		/* The encrypted auth data for the new counter */
    BYTE label[TPM_COUNTER_LABEL_SIZE]; /* Label to associate with counter */
    TPM_AUTHHANDLE authHandle;		/* The authorization session handle used for owner
					   authentication. */
    TPM_NONCE nonceOdd;			/* Nonce generated by system associated with authHandle */
    TPM_BOOL continueAuthSession = TRUE;	/* Ignored */
    TPM_AUTHDATA ownerAuth;		/* Authorization ownerAuth. */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus = FALSE;	/* audit the ordinal */
    TPM_BOOL			transportEncrypt = FALSE;	/* wrapped in encrypted transport
								   session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey = NULL;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_SECRET			a1Auth;
    TPM_ACTUAL_COUNT		nextCount;
    TPM_BOOL			writeAllNV= FALSE;	/* flag to write back NV */
    
    /* output parameters */
    uint32_t		outParamStart;		/* starting point of outParam's */
    uint32_t		outParamEnd;		/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_COUNT_ID	countID = 0;		/* The handle for the counter */
    TPM_COUNTER_VALUE	*counterValue = NULL;	/* The starting counter value */

    printf("TPM_Process_CreateCounter: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get authData */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Authdata_Load(encAuth, &command, &paramSize);
    }
    /* get label */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Loadn(label, TPM_COUNTER_LABEL_SIZE, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_CreateCounter: label", label);
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
	    printf("TPM_Process_CreateCounter: Error, command has %u extra bytes\n",
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
    /* 1. Using the authHandle field, validate the owner's AuthData to execute the command and all
       of the incoming parameters. The authorization session MUST be OSAP or DSAP. */
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
    /* 2. Ignore continueAuthSession on input and set continueAuthSession to FALSE on output */
    if (returnCode == TPM_SUCCESS) {
	continueAuthSession = FALSE;
    }
    /* 3. Create a1 by decrypting encAuth according to the ADIP indicated by authHandle. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessionData_Decrypt(a1Auth,
						 NULL,
						 encAuth,
						 auth_session_data,
						 NULL,
						 NULL,
						 FALSE);	/* even and odd */
    }
    /* 4. Validate that there is sufficient internal space in the TPM to create a new counter. If
       there is insufficient space the command returns an error. */
    /* a. The TPM MUST provide storage for a1, TPM_COUNTER_VALUE, countID, and any other internal
       data the TPM needs to associate with the counter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Counters_GetNewHandle(&counterValue,		/* structure */
					       &countID,		/* index */
					       tpm_state->tpm_permanent_data.monotonicCounter);
    }
    if (returnCode == TPM_SUCCESS) {
	writeAllNV = TRUE;
	/* 5. Increment the max counter value */
	TPM_Counters_GetNextCount(&nextCount,
				  tpm_state->tpm_permanent_data.monotonicCounter);
	/* 6. Set the counter to the max counter value */
	/* 7. Set the counter label to label */
	returnCode = TPM_CounterValue_Set(counterValue,
					  countID,
					  label,
					  nextCount,
					  a1Auth);
	/* 8. Create a countID */
	/* NOTE Done in TPM_Counters_GetNewHandle() */
    }
    /* save the permanent data structure in NVRAM */
    returnCode = TPM_PermanentAll_NVStore(tpm_state,
					  writeAllNV,
					  returnCode);
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_CreateCounter: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return the countID */
	    returnCode = TPM_Sbuffer_Append32(response, countID);
	}
	if (returnCode == TPM_SUCCESS) {
	    /* Return the TPM_COUNTER_VALUE publicly visible members */
	    returnCode = TPM_CounterValue_StorePublic(response, counterValue);
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
    return rcf;
}

/* 25.2 TPM_IncrementCounter rev 87

  This authorized command increments the indicated counter by one. Once a counter has been
  incremented then all subsequent increments must be for the same handle until a successful
  TPM_Startup(ST_CLEAR) is executed.

  The order for checking validation of the command parameters when no counter is active, keeps an
  attacker from creating a denial-of-service attack.

  This function increments the counter by 1.
  The TPM MAY implement increment throttling to avoid burn problems
*/

TPM_RESULT TPM_Process_IncrementCounter(tpm_state_t *tpm_state,
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
    TPM_COUNT_ID countID;		/* The handle of a valid counter */
    TPM_AUTHHANDLE authHandle;		/* The authorization session handle used for counter
					   authorization */
    TPM_NONCE nonceOdd;			/* Nonce generated by system associated with authHandle */
    TPM_BOOL continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   session handle */
    TPM_AUTHDATA counterAuth;		/* The authorization session digest that authorizes the use
					   of countID. HMAC key: countID -> authData */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
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
    TPM_COUNTER_VALUE	*counterValue = NULL;	/* The counter value */

    printf("TPM_Process_IncrementCounter: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get countID */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&countID, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_IncrementCounter: countID %u\n", countID);
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
					counterAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_IncrementCounter: Error, command has %u extra bytes\n",
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
    /* The first check is that either there is no active counter and the countID has been created
       or that the countID is the active counter */
    if (returnCode == TPM_SUCCESS) {
	/* 1. If TPM_STCLEAR_DATA -> countID is NULL */
	if (tpm_state->tpm_stclear_data.countID == TPM_COUNT_ID_NULL) {
	    /* a. Validate that countID is a valid counter, return TPM_BAD_COUNTER on mismatch */
	    returnCode = TPM_Counters_IsValidId(tpm_state->tpm_permanent_data.monotonicCounter,
						countID);
	}
	/* 2. else (TPM_STCLEAR_DATA -> countID is not NULL */
	else {	
	    /* a. If TPM_STCLEAR_DATA -> countID does not equal countID */
	    if (tpm_state->tpm_stclear_data.countID != countID) {
		if (tpm_state->tpm_stclear_data.countID == TPM_COUNT_ID_ILLEGAL) {
		    printf("TPM_Process_IncrementCounter: Error, counter has been released\n");
		}
		else {
		    printf("TPM_Process_IncrementCounter: Error, %u is already active\n",
			   tpm_state->tpm_stclear_data.countID);
		}
		/* i. Return TPM_BAD_COUNTER */
		returnCode = TPM_BAD_COUNTER;
	    }
	}
    }
    /* b. Validate the command parameters using counterAuth */
    /* Get the TPM_COUNTER_VALUE associated with the countID */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Counters_GetCounterValue(&counterValue,
						  tpm_state->tpm_permanent_data.monotonicCounter,
						  countID);
    }
    /* get the session data */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_NONE,
					      TPM_ET_COUNTER,
					      ordinal,
					      NULL,
					      &(counterValue->authData),	/* OIAP */
					      counterValue->digest);		/* OSAP */
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
					counterAuth);		/* Authorization digest for input */
    }
    if (returnCode == TPM_SUCCESS) {
	/* 1. If TPM_STCLEAR_DATA -> countID is NULL */
	if (tpm_state->tpm_stclear_data.countID == TPM_COUNT_ID_NULL) {
	    /* c. Set TPM_STCLEAR_DATA -> countID to countID */
	    tpm_state->tpm_stclear_data.countID = countID;
	    printf("TPM_Process_IncrementCounter: Setting %u as active counter\n", countID);
	}
    }
    if (returnCode == TPM_SUCCESS) {
	/* 3. Increments the counter by 1 */
	counterValue->counter++;	/* in TPM_PERMANENT_DATA */
	/* save the permanent data structure in NVRAM */
	returnCode = TPM_PermanentAll_NVStore(tpm_state,
					      TRUE,
					      returnCode);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_IncrementCounter: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* 4. Return new count value in count */
	    returnCode = TPM_CounterValue_StorePublic(response, counterValue);
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

/* 25.3 TPM_ReadCounter rev 87

   Reading the counter provides the caller with the current number in the sequence.

   This returns the current value for the counter indicated. The counter MAY be any valid counter.
*/

TPM_RESULT TPM_Process_ReadCounter(tpm_state_t *tpm_state,
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
    TPM_COUNT_ID countID;			/* ID value of the counter */

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

    printf("TPM_Process_ReadCounter: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get countID */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&countID, &command, &paramSize);
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
	    printf("TPM_Process_ReadCounter: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. Validate that countID points to a valid counter. Return TPM_BAD_COUNTER on error. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_ReadCounter: countID %u\n", countID);
	returnCode = TPM_Counters_IsValidId(tpm_state->tpm_permanent_data.monotonicCounter,
					    countID);
    }
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_ReadCounter: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* 2. Return count (directly from TPM_PERMANENT_DATA) */
	    returnCode = TPM_CounterValue_StorePublic
			 (response, &(tpm_state->tpm_permanent_data.monotonicCounter[countID]));
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

/* 25.4 TPM_ReleaseCounter rev 87

  This command releases a counter such that no reads or increments of the indicated counter will
  succeed.

  The TPM uses countID to locate a valid counter. 
*/

TPM_RESULT TPM_Process_ReleaseCounter(tpm_state_t *tpm_state,
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
    TPM_COUNT_ID countID;		/* ID value of the counter */
    TPM_AUTHHANDLE authHandle;		/* The authorization session handle used for countID
					   authorization */
    TPM_NONCE nonceOdd;			/* Nonce associated with countID */
    TPM_BOOL continueAuthSession = TRUE;	/* Ignored */
    TPM_AUTHDATA counterAuth;		/* The authorization session digest that authorizes the use
					   of countID.	HMAC key: countID -> authData */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus = FALSE;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt = FALSE;	/* wrapped in encrypted transport
								   session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_COUNTER_VALUE		*counterValue;		/* associated with countID */
    TPM_SECRET			savedAuth;		/* saved copy for response */
    TPM_BOOL			writeAllNV = FALSE;	/* flag to write back NV*/

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_ReleaseCounter: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get countID */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&countID, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_ReleaseCounter: countID %u\n", countID);
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
	returnCode = TPM_CheckRequestTag1(tag);
    }
    /* get the 'below the line' authorization parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					counterAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_ReleaseCounter: Error, command has %u extra bytes\n",
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
    /* 1. Authenticate the command and the parameters using the AuthData pointed to by
       countID. Return TPM_AUTHFAIL on error */
    /* Get the TPM_COUNTER_VALUE associated with the countID */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Counters_GetCounterValue(&counterValue,
						  tpm_state->tpm_permanent_data.monotonicCounter,
						  countID);
    }
    /* get the session data */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_NONE,
					      TPM_ET_COUNTER,
					      ordinal,
					      NULL,
					      &(counterValue->authData),	/* OIAP */
					      counterValue->digest);		/* OSAP */
    }
    if (returnCode == TPM_SUCCESS) {
	/* make a copy of the HMAC key for the response, since it gets invalidated */
	TPM_Secret_Copy(savedAuth, *hmacKey);
	/* Validate the authorization to use the key pointed to by countID */
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					counterAuth);		/* Authorization digest for input */
    }
    /* 3. The TPM invalidates sessions */
    /* a. MUST invalidate all OSAP sessions associated with the counter */
    /* b. MAY invalidate any other session */
    /* NOTE: Actions reversed because the sessions can't be found after the digest is initialized */
    if (returnCode == TPM_SUCCESS) {
	TPM_AuthSessions_TerminateEntity(&continueAuthSession,
					 authHandle,
					 tpm_state->tpm_stclear_data.authSessions,
					 TPM_ET_COUNTER,		/* TPM_ENTITY_TYPE */
					 &(counterValue->digest));	/* entityDigest */
    }
    /* 2. The TPM invalidates all internal information regarding the counter. This includes
       releasing countID such that any subsequent attempts to use countID will fail. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_ReleaseCounter: Releasing counter %u\n", countID);
	returnCode = TPM_CounterValue_Release(counterValue, countID);
    }
    if (returnCode == TPM_SUCCESS) {
	writeAllNV= TRUE;
	/* 4. If TPM_STCLEAR_DATA -> countID equals countID,  */
	if (tpm_state->tpm_stclear_data.countID == countID ) {
	    printf("TPM_Process_ReleaseCounter: Deactivating counter %u\n", countID);
	    /* a. Set TPM_STCLEAR_DATA -> countID to an illegal value (not the NULL value) */
	    tpm_state->tpm_stclear_data.countID = TPM_COUNT_ID_ILLEGAL;
	}
    }
    /* save the permanent data structure in NVRAM */
    returnCode = TPM_PermanentAll_NVStore(tpm_state,
					  writeAllNV,
					  returnCode);
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_ReleaseCounter: Ordinal returnCode %08x %u\n",
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
					    savedAuth,		/* saved countID HMAC key */
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

/* 25.5 TPM_ReleaseCounterOwner rev 101

   This command releases a counter such that no reads or increments of the indicated counter will
   succeed.

   This invalidates all information regarding a counter.
*/

TPM_RESULT TPM_Process_ReleaseCounterOwner(tpm_state_t *tpm_state,
					   TPM_STORE_BUFFER *response,
					   TPM_TAG tag,
					   uint32_t paramSize,
					   TPM_COMMAND_CODE ordinal,
					   unsigned char *command,
					   TPM_TRANSPORT_INTERNAL *transportInternal)
{	
    TPM_RESULT	rcf = 0;				/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    TPM_COUNT_ID countID;		/* ID value of the counter */
    TPM_AUTHHANDLE authHandle;		/* The authorization session handle used for owner
					   authentication */
    TPM_NONCE nonceOdd;			/* Nonce generated by system associated with authHandle */
    TPM_BOOL continueAuthSession = FALSE;	/* The continue use flag for the authorization
						   session handle */
    TPM_AUTHDATA ownerAuth;		/* The authorization session digest that authorizes the
					   inputs. HMAC key: ownerAuth */
  
    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus = FALSE;	/* audit the ordinal */
    TPM_BOOL			transportEncrypt = TRUE; /* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey = NULL;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_COUNTER_VALUE		*counterValue;		/* associated with countID */
    TPM_BOOL			writeAllNV = FALSE;	/* flag to write back NV */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_ReleaseCounterOwner: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get countID */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&countID, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_ReleaseCounterOwner: countID %u\n", countID);
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
	    printf("TPM_Process_ReleaseCounterOwner: Error, command has %u extra bytes\n",
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
    /* 1. Validate that ownerAuth properly authorizes the command and parameters */
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
    /* 2. The TPM uses countID to locate a valid counter. Return TPM_BAD_COUNTER if not found. */
    /* Get the TPM_COUNTER_VALUE associated with the countID */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Counters_GetCounterValue(&counterValue,
						  tpm_state->tpm_permanent_data.monotonicCounter,
						  countID);
    }
    /* NOTE: Actions reversed because the sessions can't be found after the digest is initialized */
    if (returnCode == TPM_SUCCESS) {
	TPM_AuthSessions_TerminateEntity(&continueAuthSession,
					 authHandle,
					 tpm_state->tpm_stclear_data.authSessions,
					 TPM_ET_COUNTER,		/* TPM_ENTITY_TYPE */
					 &(counterValue->digest));	/* entityDigest */
    }
    /* 3. The TPM invalidates all internal information regarding the counter. This includes
       releasing countID such that any subsequent attempts to use countID will fail. */
    /* NOTE: This function can only return a TPM_FAIL error, so that the failure to store
       TPM_PERMANENT_DATA will already be reported as fatal. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CounterValue_Release(counterValue, countID);
    }
    /* 4. The TPM invalidates sessions */
    /* a. MUST invalidate all OSAP sessions associated with the counter */
    /* b. MAY invalidate any other session */
    if (returnCode == TPM_SUCCESS) {
	writeAllNV = TRUE;
	/* 5. If TPM_STCLEAR_DATA -> countID equals countID,  */
	if (tpm_state->tpm_stclear_data.countID == countID ) {
	    printf("TPM_Process_ReleaseCounterOwner: Deactivating counter %u\n", countID);
	    /* a. Set TPM_STCLEAR_DATA -> countID to an illegal value (not the zero value) */
	    tpm_state->tpm_stclear_data.countID = TPM_COUNT_ID_ILLEGAL;
	}
    }
    /* save the permanent data structure in NVRAM */
    returnCode = TPM_PermanentAll_NVStore(tpm_state,
					  writeAllNV,
					  returnCode);
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_ReleaseCounterOwner: Ordinal returnCode %08x %u\n",
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
