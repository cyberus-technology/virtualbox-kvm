/********************************************************************************/
/*										*/
/*				Tick Handler					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_ticks.c $		*/
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

#include <string.h>
#include <stdio.h>

#include "tpm_auth.h"
#include "tpm_cryptoh.h"
#include "tpm_debug.h"
#include "tpm_digest.h"
#include "tpm_error.h"
#include "tpm_key.h"
#include "tpm_structures.h"
#include "tpm_nonce.h"
#include "tpm_process.h"
#include "tpm_time.h"

#include "tpm_ticks.h"

static void TPM_Uint64_ConvertFrom(uint32_t *upper,
				   uint32_t *lower,
				   uint32_t sec,
				   uint32_t usec);
static void TPM_Uint64_ConvertTo(uint32_t *sec,
				 uint32_t *usec,
				 uint32_t upper,
				 uint32_t lower);

/*
  UINT64 for currentTicks

  Internally, the UINT64 is stored as sec || usec.  This makes calculations easy since TPM_GetTimeOfDay
  returns those structure elements.

  The TPM_Uint64_Store() function, the public interface, converts this to a true 64 bit integer.
*/

/* TPM_Uint64_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_Uint64_Init(TPM_UINT64 *tpm_uint64)
{
    printf(" TPM_Uint64_Init:\n");
    tpm_uint64->sec = 0;
    tpm_uint64->usec = 0;
    return;
}

/* TPM_Uint64_Load()
   
   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes

   This function does the conversion from a 64 bit usec  to sec / usec.
*/

TPM_RESULT TPM_Uint64_Load(TPM_UINT64 *tpm_uint64,
			    unsigned char **stream,
			    uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;
    uint32_t		upper;
    uint32_t		lower;

    printf(" TPM_Uint64_Load:\n");
    /* load upper */
    if (rc == 0) {
	rc = TPM_Load32(&upper, stream, stream_size);
    }
    /* load lower */
    if (rc == 0) {
	rc = TPM_Load32(&lower, stream, stream_size);
    }
    /* convert from 64 bit usec to sec, usec */
    if (rc == 0) {
	TPM_Uint64_ConvertTo(&(tpm_uint64->sec),
			     &(tpm_uint64->usec),
			     upper,
			     lower);
    }
    return rc;
}

/* TPM_Uint64_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes

   This function does the conversion from sec / usec to a 64 bit usec.
*/

TPM_RESULT TPM_Uint64_Store(TPM_STORE_BUFFER *sbuffer,
			    const TPM_UINT64 *tpm_uint64)
{
    TPM_RESULT		rc = 0;
    uint32_t		upper;
    uint32_t		lower;

    printf(" TPM_Uint64_Store:\n");
    /* store upper */
    if (rc == 0) {
	/* convert to 64 bit number */
	TPM_Uint64_ConvertFrom(&upper, &lower, tpm_uint64->sec, tpm_uint64->usec);
	rc = TPM_Sbuffer_Append32(sbuffer, upper);
    }
    /* store lower */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, lower);
    }
    return rc;
}

void TPM_Uint64_Copy(TPM_UINT64 *dest,
		     const TPM_UINT64 *src)
{
    printf(" TPM_Uint64_Copy:\n");
    dest->sec = src->sec;
    dest->usec = src->usec;
    return;
}

/* TPM_Uint64_ConvertFrom() does the calculation result = sec * 1000000 + usec and splits the result
   into two uint32_t's.

   This may not be portable if the compiler does not support long long.
*/
   
/* TPM_Uint64_ConvertTo() does the calculation uint32_t || uint32_t to sec and usec.

   This may not be portable if the compiler does not support long long.
*/
   
#if defined(TPM_POSIX) || defined(TPM_SYSTEM_P) || defined(TPM_WINDOWS) /*VBOX addition*/

static void TPM_Uint64_ConvertFrom(uint32_t *upper,
				   uint32_t *lower,
				   uint32_t sec,
				   uint32_t usec)
{
    long long result;
    
    printf("  TPM_Uint64_ConvertFrom: sec %u, usec %u\n", sec, usec);
    result = (sec * 1000000LL) + (long long)usec;
    printf("   TPM_Uint64_ConvertFrom: Result usec %llu, %llx\n", result, result);
    *upper = (result >> 32) & 0xffffffff;
    *lower = result & 0xffffffff;
    printf("   TPM_Uint64_ConvertFrom: Upper %u, %x\n", *upper, *upper);
    printf("   TPM_Uint64_ConvertFrom: Lower %u, %x\n", *lower, *lower);
    return;
}

static void TPM_Uint64_ConvertTo(uint32_t *sec,
				 uint32_t *usec,
				 uint32_t upper,
				 uint32_t lower)
{
    long long result;

    printf("   TPM_Uint64_ConvertTo: Upper %u, %x\n", upper, upper);
    printf("   TPM_Uint64_ConvertTo: Lower %u, %x\n", lower, lower);
    result = ((long long)upper << 32) | (long long)lower;
    printf("   TPM_Uint64_ConvertTo: Result usec %llu, %llx\n", result, result);
    *sec = result / 1000000LL;
    *usec = result % 1000000LL;
    printf("  TPM_Uint64_ConvertTo: sec %u, usec %u\n", *sec, *usec);
    return;
}

#endif


TPM_RESULT TPM_Uint64_Test()
{
    TPM_RESULT rc = 0;
    TPM_UINT64 uint64In;
    TPM_UINT64 uint64Out;
    TPM_STORE_BUFFER sbuffer;
    unsigned char *stream;
    uint32_t stream_size;

    printf("  TPM_Uint64_Test\n");
    TPM_Sbuffer_Init(&sbuffer);
    uint64In.sec = 12345678;
    uint64In.usec = 781234;

    if (rc == 0) {
	rc = TPM_Uint64_Store(&sbuffer, &uint64In);
    }
    if (rc == 0) {
	TPM_Sbuffer_Get(&sbuffer, (const unsigned char **)&stream, &stream_size);
	rc = TPM_Uint64_Load(&uint64Out, &stream, &stream_size);
    }
    if (rc == 0) {
	if ((uint64In.sec != uint64Out.sec) ||
	    (uint64In.usec != uint64Out.usec)) {
	    printf("TPM_Uint64_Test: Error (fatal)\n");
	    rc = TPM_FAILEDSELFTEST;
	}
    }
    TPM_Sbuffer_Delete(&sbuffer);
    return rc;
}

/*
  TPM_CURRENT_TICKS
*/

/* TPM_CurrentTicks_Init() initializes the tick structure

*/

void TPM_CurrentTicks_Init(TPM_CURRENT_TICKS *tpm_current_ticks)
{
    printf(" TPM_CurrentTicks_Init:\n");
    TPM_Uint64_Init(&(tpm_current_ticks->currentTicks));
    tpm_current_ticks->tickRate = TPM_TICK_RATE;
    TPM_Nonce_Init(tpm_current_ticks->tickNonce);
    TPM_Uint64_Init(&(tpm_current_ticks->initialTime));
    return;
}

/* TPM_CurrentTicks_Start() sets the initialTime member to the
   current time of day.

   It assumes TPM_CurrentTicks_Init() has been called
*/

TPM_RESULT TPM_CurrentTicks_Start(TPM_CURRENT_TICKS *tpm_current_ticks)
{
    TPM_RESULT rc = 0;

    printf(" TPM_CurrentTicks_Start:\n");
    if (rc == 0) {
	/* current is relative to the initial value, and is always 0 */
	TPM_Uint64_Init(&(tpm_current_ticks->currentTicks));
	/* save the current time */
	rc = TPM_GetTimeOfDay(&(tpm_current_ticks->initialTime.sec),
			      &(tpm_current_ticks->initialTime.usec));
    }
    if (rc == 0) {
	tpm_current_ticks->tickRate = TPM_TICK_RATE;
	rc = TPM_Nonce_Generate(tpm_current_ticks->tickNonce);
    }
    return rc;
}

/* TPM_CurrentTicks_LoadAll() loads the standard TCG structure plus the SW TPM members
   
   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_CurrentTicks_Init()
*/

TPM_RESULT TPM_CurrentTicks_LoadAll(TPM_CURRENT_TICKS *tpm_current_ticks,
				    unsigned char **stream,
				    uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_CurrentTicks_LoadAll:\n");
    /* load tag */
    if (rc == 0) {
        rc = TPM_CheckTag(TPM_TAG_CURRENT_TICKS, stream, stream_size);
    }
    /* load currentTicks */
    if (rc == 0) {
	rc = TPM_Uint64_Load(&(tpm_current_ticks->currentTicks), stream, stream_size);
    }
    /* load tickRate */
    if (rc == 0) {
	rc = TPM_Load16(&(tpm_current_ticks->tickRate), stream, stream_size);
    }
    /* load tickNonce */
    if (rc == 0) {
	rc = TPM_Nonce_Load(tpm_current_ticks->tickNonce, stream, stream_size);
    }
    /* load initialTime */
    if (rc == 0) {
	rc = TPM_Uint64_Load(&(tpm_current_ticks->initialTime), stream, stream_size);
    }
    return rc;
}

/* TPM_CurrentTicks_Store() stores the standard TCG structure
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_CurrentTicks_Store(TPM_STORE_BUFFER *sbuffer,
				  const TPM_CURRENT_TICKS *tpm_current_ticks)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_CurrentTicks_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_CURRENT_TICKS);
    }
    /* store currentTicks */
    if (rc == 0) {
	rc = TPM_Uint64_Store(sbuffer, &(tpm_current_ticks->currentTicks));
    }
    /* store tickRate */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, tpm_current_ticks->tickRate);
    }
    /* store tickNonce */
    if (rc == 0) {
	rc = TPM_Nonce_Store(sbuffer, tpm_current_ticks->tickNonce);
    }
    return rc;
}

/* TPM_CurrentTicks_Store() stores the standard TCG structure plus the SW TPM members
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_CurrentTicks_StoreAll(TPM_STORE_BUFFER *sbuffer,
				     const TPM_CURRENT_TICKS *tpm_current_ticks)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_CurrentTicks_StoreAll:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_CurrentTicks_Store(sbuffer, tpm_current_ticks);
    }
    /* store initialTime */
    if (rc == 0) {
	rc = TPM_Uint64_Store(sbuffer, &(tpm_current_ticks->initialTime));
    }
    return rc;
}

/* TPM_CurrentTicks_Update() updates the currentTicks member of TPM_CURRENT_TICKS
   relative to the initial time

*/

TPM_RESULT TPM_CurrentTicks_Update(TPM_CURRENT_TICKS *tpm_current_ticks)
{
    TPM_RESULT	rc = 0;
    uint32_t	currentTimeSec;
    uint32_t	currentTimeUsec;
    
    printf(" TPM_CurrentTicks_Update: Initial %u sec %u usec\n",
	   tpm_current_ticks->initialTime.sec, tpm_current_ticks->initialTime.usec);
    /* get the current time of day */
    if (rc == 0) {
	rc = TPM_GetTimeOfDay(&currentTimeSec, &currentTimeUsec);
    }
    /* Calculate:
	 currentTimeSec currentTimeUsec
       - initialTimeSec initialTimeUsec
    */
    if (rc == 0) {
	/* case 1: no borrow */
	if (currentTimeUsec >= tpm_current_ticks->initialTime.usec) {
	    /* subtract usec */
	    tpm_current_ticks->currentTicks.usec = currentTimeUsec -
						    tpm_current_ticks->initialTime.usec;

	    /* check that time went forward */
	    if (currentTimeSec >= tpm_current_ticks->initialTime.sec) {
		/* subtract sec */
		tpm_current_ticks->currentTicks.sec = currentTimeSec -
							tpm_current_ticks->initialTime.sec;
	    }
	    else {
		printf(" TPM_CurrentTicks_Update: Error (fatal), illegal current time\n");
		rc = TPM_FAIL;
	    }
	}
	/* case 2: borrow */
	else {
	    /* subtract usec with borrow */
	    tpm_current_ticks->currentTicks.usec = 1000000 + currentTimeUsec -
						    tpm_current_ticks->initialTime.usec;
	    /* check that time went forward, with borrow */
	    if ((currentTimeSec - 1) >= tpm_current_ticks->initialTime.sec) {
		/* subtract sec */
		tpm_current_ticks->currentTicks.sec = currentTimeSec - 1 - 
							tpm_current_ticks->initialTime.sec;
	    }
	    else {
		printf(" TPM_CurrentTicks_Update: Error (fatal), illegal current time\n");
		rc = TPM_FAIL;
	    }
	}
    }
    if (rc == 0) {
	printf(" TPM_CurrentTicks_Update: Ticks %u sec %u usec\n",
	       tpm_current_ticks->currentTicks.sec,
	       tpm_current_ticks->currentTicks.usec);
    }
    return rc;
}

/* TPM_CurrentTicks_Copy() copies the 'src' to 'dest'

*/

void TPM_CurrentTicks_Copy(TPM_CURRENT_TICKS *dest,
			   TPM_CURRENT_TICKS *src)
{
    printf(" TPM_CurrentTicks_Copy:\n");
    TPM_Uint64_Copy(&(dest->currentTicks), &(src->currentTicks));
    dest->tickRate = src->tickRate;
    TPM_Nonce_Copy(dest->tickNonce, src->tickNonce);
    TPM_Uint64_Copy(&(dest->initialTime), &(src->initialTime));
    return;
}

/*
  Processing Functions
*/

/* 23. Timing Ticks rev 87

   The TPM timing ticks are always available for use. The association of timing ticks to actual time
   is a protocol that occurs outside of the TPM. See the design document for details.

   The setting of the clock type variable is a one time operation that allows the TPM to be
   configured to the type of platform that is installed on.

   The ability for the TPM to continue to increment the timer ticks across power cycles of the
   platform is a TPM and platform manufacturer decision.
*/

/* 23.1 TPM_GetTicks rev 87

   This command returns the current tick count of the TPM.

   This command returns the current time held in the TPM. It is the responsibility of the external
   system to maintain any relation between this time and a UTC value or local real time value.
*/

TPM_RESULT TPM_Process_GetTicks(tpm_state_t *tpm_state,
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
    TPM_CURRENT_TICKS	*t1CurrentTicks = NULL;		/* The current time held in the TPM */

    printf("TPM_Process_GetTicks: Ordinal Entry\n");
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
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_ALLOW_NO_OWNER);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_GetTicks: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    if (returnCode == TPM_SUCCESS) {
	/* 1. Set T1 to the internal TPM_CURRENT_TICKS structure */
	t1CurrentTicks = &(tpm_state->tpm_stany_data.currentTicks);
	/* update the ticks based on the current time */
	returnCode = TPM_CurrentTicks_Update(t1CurrentTicks);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_GetTicks: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* 2. Return T1 as currentTime. */
	    returnCode = TPM_CurrentTicks_Store(response, t1CurrentTicks);
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

/* 23.2 TPM_TickStampBlob rev 101

   This command applies a time stamp to the passed blob. The TPM makes no representation regarding
   the blob merely that the blob was present at the TPM at the time indicated.

   The function performs a digital signature on the hash of digestToStamp and the current tick
   count.

   It is the responsibility of the external system to maintain any relation between tick count and a
   UTC value or local real time value.

*/

TPM_RESULT TPM_Process_TickStampBlob(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE keyHandle;	/* The keyHandle identifier of a loaded key that can perform digital
				   signatures. */
    TPM_NONCE antiReplay;	/* Anti replay value added to signature */
    TPM_DIGEST digestToStamp;	/* The digest to perform the tick stamp on */
    TPM_AUTHHANDLE authHandle;	/* The authorization session handle used for keyHandle authorization
				   */
    TPM_NONCE nonceOdd;		/* Nonce generated by system associated with authHandle */
    TPM_BOOL continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   session handle */
    TPM_AUTHDATA privAuth;	/* The authorization session digest that authorizes the use of
				   keyHandle. HMAC key: key.usageAuth */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_KEY			*sigKey;			/* signing key */
    TPM_SECRET			*keyUsageAuth;
    TPM_SECRET			*hmacKey;
    TPM_BOOL			parentPCRStatus;
    TPM_SIGN_INFO		h1SignInfo;
    TPM_STORE_BUFFER		h2Data;
    TPM_STORE_BUFFER		h1sbuffer;	/* serialization of h1SignInfo */
    TPM_DIGEST			h3Digest;	/* digest to be signed */
    
    /* output parameters */
    uint32_t		outParamStart;		/* starting point of outParam's */
    uint32_t		outParamEnd;		/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_CURRENT_TICKS	*currentTicks = NULL;	/* The current time according to the TPM */
    TPM_SIZED_BUFFER	sig;			/* The resulting digital signature. */

    printf("TPM_Process_TickStampBlob: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&sig);		/* freed @1 */
    TPM_SignInfo_Init(&h1SignInfo);	/* freed @2 */
    TPM_Sbuffer_Init(&h2Data);		/* freed @3 */
    TPM_Sbuffer_Init(&h1sbuffer);	/* freed @4 */
    /*
      get inputs
    */
    /*	get keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&keyHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get antiReplay parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_TickStampBlob: keyHandle %08x\n", keyHandle);
	returnCode = TPM_Nonce_Load(antiReplay, &command, &paramSize);
    }
    /* get digestToStamp parameter */
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_TickStampBlob: antiReplay", antiReplay);
	returnCode = TPM_Digest_Load(digestToStamp, &command, &paramSize);
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
	TPM_PrintFour("TPM_Process_TickStampBlob: digestToStamp", digestToStamp);
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
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	printf("TPM_Process_TickStampBlob: authHandle %08x\n", authHandle);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_TickStampBlob: Error, command has %u extra bytes\n",
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
	returnCode = TPM_KeyHandleEntries_GetKey(&sigKey, &parentPCRStatus, tpm_state, keyHandle,
						 FALSE,		/* not read-only */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND)) {
	if (sigKey->authDataUsage != TPM_AUTH_NEVER) {
	    printf("TPM_Process_TickStampBlob: Error, authorization required\n");
	    returnCode = TPM_AUTHFAIL;
	}
    }
    /* get keyHandle -> usageAuth */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_Key_GetUsageAuth(&keyUsageAuth, sigKey);
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
					      keyUsageAuth,		/* OIAP */
					      sigKey->tpm_store_asymkey->pubDataDigest); /* OSAP */
    }
    /* 1. The TPM validates the AuthData to use the key pointed to by keyHandle.  */
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
    /* 2. Validate that keyHandle -> keyUsage is TPM_KEY_SIGNING, TPM_KEY_IDENTITY or
       TPM_KEY_LEGACY, if not return the error code TPM_INVALID_KEYUSAGE. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_TickStampBlob: Checking key properties\n");
	if ((sigKey->keyUsage != TPM_KEY_SIGNING) &&
	    (sigKey->keyUsage != TPM_KEY_IDENTITY) &&
	    (sigKey->keyUsage != TPM_KEY_LEGACY)) {
	    printf("TPM_Process_TickStampBlob: Error, keyUsage %04hx is invalid\n",
		   sigKey->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 3. Validate that keyHandle -> sigScheme is TPM_SS_RSASSAPKCS1v15_SHA1 or
       TPM_SS_RSASSAPKCS1v15_INFO, if not return TPM_INAPPROPRIATE_SIG. */
    if (returnCode == TPM_SUCCESS) {
	if ((sigKey->algorithmParms.sigScheme != TPM_SS_RSASSAPKCS1v15_SHA1) &&
	    (sigKey->algorithmParms.sigScheme != TPM_SS_RSASSAPKCS1v15_INFO)) {
	    printf("TPM_Process_TickStampBlob: Error, invalid sigKey sigScheme %04hx\n",
		   sigKey->algorithmParms.sigScheme);
	    returnCode = TPM_INAPPROPRIATE_SIG;
	}
    }
    /* 4. If TPM_STCLEAR_DATA -> currentTicks is not properly initialized */
    /* a. Initialize the TPM_STCLEAR_DATA -> currentTicks */
    /* NOTE: Always initialized */
    /* 5. Create T1, a TPM_CURRENT_TICKS structure. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_TickStampBlob: Creating TPM_CURRENT_TICKS structure\n");
	currentTicks = &(tpm_state->tpm_stany_data.currentTicks);
	/* update the ticks based on the current time */
	returnCode = TPM_CurrentTicks_Update(currentTicks);
    }
    if (returnCode == TPM_SUCCESS) {
	/* 6. Create H1 a TPM_SIGN_INFO structure and set the structure defaults */
	printf("TPM_Process_TickStampBlob: Creating TPM_SIGN_INFO structure\n");
	/* NOTE: Done by TPM_SignInfo_Init() */
	/* a. Set H1 -> fixed to 'TSTP' */
	memcpy(h1SignInfo.fixed, "TSTP", TPM_SIGN_INFO_FIXED_SIZE);
	/* b. Set H1 -> replay to antiReplay */
	TPM_Nonce_Copy(h1SignInfo.replay, antiReplay );
	/* c. Create H2 the concatenation of digestToStamp || T1 */
	/* add digestToStamp */
	returnCode = TPM_Digest_Store(&h2Data, digestToStamp);
    }
    /* add T1 (currentTicks) */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CurrentTicks_Store(&h2Data, currentTicks);
    }
    /* d. Set H1 -> dataLen to the length of H2 */
    /* e. Set H1 -> data */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_SetFromStore(&(h1SignInfo.data), &h2Data);
    }
    /* 7. The TPM computes the signature, sig, using the key referenced by keyHandle, using SHA-1 of
       H1 as the information to be signed */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_TickStampBlob: Digesting TPM_SIGN_INFO structure\n");
	returnCode = TPM_SHA1_GenerateStructure(h3Digest, &h1SignInfo,
						(TPM_STORE_FUNCTION_T)TPM_SignInfo_Store);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_TickStampBlob: Signing TPM_SIGN_INFO digest\n");
	returnCode = TPM_RSASignToSizedBuffer(&sig,		/* signature */
					      h3Digest,		/* message */
					      TPM_DIGEST_SIZE,	/* message size */
					      sigKey);		/* input, signing key */
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_TickStampBlob: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* 7. The TPM returns T1 as currentTicks parameter */
	    returnCode = TPM_CurrentTicks_Store(response, currentTicks);
	}
	/* 6. Return the signature in sig */
	if (returnCode == TPM_SUCCESS) {
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
    TPM_SizedBuffer_Delete(&sig);	/* @1 */
    TPM_SignInfo_Delete(&h1SignInfo);	/* @2 */
    TPM_Sbuffer_Delete(&h2Data);	/* @3 */
    TPM_Sbuffer_Delete(&h1sbuffer);	/* @4 */
    return rcf;
}

