/********************************************************************************/
/*										*/
/*				Audit Handler					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_audit.c $		*/
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

#include "tpm_auth.h"
#include "tpm_counter.h"
#include "tpm_cryptoh.h"
#include "tpm_debug.h"
#include "tpm_digest.h"
#include "tpm_error.h"
#include "tpm_global.h"
#include "tpm_key.h"
#include "tpm_nonce.h"
#include "tpm_permanent.h"
#include "tpm_process.h"

#include "tpm_audit.h"

/*
  TPM_AUDIT_EVENT_IN 
*/

/* TPM_AuditEventIn_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_AuditEventIn_Init(TPM_AUDIT_EVENT_IN *tpm_audit_event_in)
{
    printf(" TPM_AuditEventIn_Init:\n");
    TPM_Digest_Init(tpm_audit_event_in->inputParms);
    TPM_CounterValue_Init(&(tpm_audit_event_in->auditCount));
    return;
}

/* TPM_AuditEventIn_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_AuditEventIn_Store(TPM_STORE_BUFFER *sbuffer,
				  const TPM_AUDIT_EVENT_IN *tpm_audit_event_in)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_AuditEventIn_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_AUDIT_EVENT_IN); 
    }
    /* store inputParms */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_audit_event_in->inputParms);
    }
    /* store auditCount */
    if (rc == 0) {
	rc = TPM_CounterValue_StorePublic(sbuffer, &(tpm_audit_event_in->auditCount));
    }
    return rc;
}

/* TPM_AuditEventIn_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_AuditEventIn_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_AuditEventIn_Delete(TPM_AUDIT_EVENT_IN *tpm_audit_event_in)
{
    printf(" TPM_AuditEventIn_Delete:\n");
    if (tpm_audit_event_in != NULL) {
	TPM_AuditEventIn_Init(tpm_audit_event_in);
    }
    return;
}

/*
  TPM_AUDIT_EVENT_OUT
*/

/* TPM_AuditEventOut_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_AuditEventOut_Init(TPM_AUDIT_EVENT_OUT *tpm_audit_event_out)
{
    printf(" TPM_AuditEventOut_Init:\n");
    TPM_Digest_Init(tpm_audit_event_out->outputParms);
    TPM_CounterValue_Init(&(tpm_audit_event_out->auditCount));
    return;
}

/* TPM_AuditEventOut_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_AuditEventOut_Store(TPM_STORE_BUFFER *sbuffer,
				  const TPM_AUDIT_EVENT_OUT *tpm_audit_event_out)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_AuditEventOut_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_AUDIT_EVENT_OUT); 
    }
    /* store outputParms */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_audit_event_out->outputParms);
    }
    /* store auditCount */
    if (rc == 0) {
	rc = TPM_CounterValue_StorePublic(sbuffer, &(tpm_audit_event_out->auditCount));
    }
    return rc;
}

/* TPM_AuditEventOut_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_AuditEventOut_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_AuditEventOut_Delete(TPM_AUDIT_EVENT_OUT *tpm_audit_event_out)
{
    printf(" TPM_AuditEventOut_Delete:\n");
    if (tpm_audit_event_out != NULL) {
	TPM_AuditEventOut_Init(tpm_audit_event_out);
    }
    return;
}

/*
  ordinalAuditStatus Processing
*/

/* TPM_OrdinalAuditStatus_Init() initializes the TPM_PERMANENT_DATA 'ordinalAuditStatus' to the
   default

   The flags are stored as a bit map to conserve NVRAM.

   The array is not written back to NVRAM.
*/

TPM_RESULT TPM_OrdinalAuditStatus_Init(TPM_PERMANENT_DATA *tpm_permanent_data)
{
    TPM_RESULT		rc = 0;
    TPM_COMMAND_CODE	ord;		/* iterate through all ordinals */
    TPM_BOOL		auditDefault;	/* result for an ordinal */
    TPM_BOOL 		altered;
    
    printf(" TPM_OrdinalAuditStatus_Init:\n");

    for (ord = 0 ; (rc == 0) && (ord < TPM_ORDINALS_MAX) ; ord++) {
	/* get the default audit state from the ordinals table */
	TPM_OrdinalTable_GetAuditDefault(&auditDefault, ord);
	/* write to the TPM_PERMANENT_DATA bit map */
	rc = TPM_OrdinalAuditStatus_SetAuditStatus(&altered, tpm_permanent_data, auditDefault, ord);
    }
    /* hack for TSC ordinals */
    if (rc == 0) {
	TPM_OrdinalTable_GetAuditDefault(&auditDefault, TSC_ORD_PhysicalPresence);
	rc = TPM_OrdinalAuditStatus_SetAuditStatus(&altered, tpm_permanent_data, auditDefault,
						   TSC_ORD_PhysicalPresence);
    }
    if (rc == 0) {
	TPM_OrdinalTable_GetAuditDefault(&auditDefault, TSC_ORD_ResetEstablishmentBit);
	rc = TPM_OrdinalAuditStatus_SetAuditStatus(&altered, tpm_permanent_data, auditDefault,
						   TSC_ORD_ResetEstablishmentBit);
    }
    return rc;
}

/* TPM_OrdinalAuditStatus_Store() stores a list of all ordinals being audited
 */

TPM_RESULT TPM_OrdinalAuditStatus_Store(TPM_SIZED_BUFFER *ordinalList,
					TPM_PERMANENT_DATA *tpm_permanent_data,
					TPM_COMMAND_CODE startOrdinal)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	sbuffer;
    TPM_COMMAND_CODE	ord;
    TPM_BOOL		auditStatus;
    
    printf(" TPM_OrdinalAuditStatus_Store\n");
    TPM_Sbuffer_Init(&sbuffer);			/* freed @1 */	
    /* scan through the ordinals array */
    for (ord = startOrdinal ; (rc == 0) && (ord < TPM_ORDINALS_MAX) ; ord++ ) {
	/* determine if the ordinal being audited */
	if (rc == 0) {
	    rc = TPM_OrdinalAuditStatus_GetAuditStatus(&auditStatus, ord, tpm_permanent_data);
	}
	/* if being audited */
	if ((rc == 0) && auditStatus) {
	    rc = TPM_Sbuffer_Append32(&sbuffer, ord);	/* append ordinal to the list */
	}
    }
    /* scan the TSC ordinals */
    if (rc == 0) {
	if (rc == 0) {
	    rc = TPM_OrdinalAuditStatus_GetAuditStatus(&auditStatus,
						       TSC_ORD_PhysicalPresence,
						       tpm_permanent_data);
	}
	if ((rc == 0) && auditStatus) {
	    rc = TPM_Sbuffer_Append32(&sbuffer, TSC_ORD_PhysicalPresence);
	}
	if (rc == 0) {
	    rc = TPM_OrdinalAuditStatus_GetAuditStatus(&auditStatus,
						       TSC_ORD_ResetEstablishmentBit,
						       tpm_permanent_data);
	}
	/* if being audited */
	if ((rc == 0) && auditStatus) {
	    rc = TPM_Sbuffer_Append32(&sbuffer, TSC_ORD_ResetEstablishmentBit);
	}
    }
    /* convert the list to a TPM_SIZED_BUFFER */
    if (rc == 0) {
	rc = TPM_SizedBuffer_SetFromStore(ordinalList, &sbuffer);
    }
    TPM_Sbuffer_Delete(&sbuffer);	/* @1 */
    return rc;
}

/* TPM_OrdinalAuditStatus_GetAuditState() gets the audit state for the ordinal
 */

TPM_RESULT TPM_OrdinalAuditStatus_GetAuditStatus(TPM_BOOL *auditStatus,
						 TPM_COMMAND_CODE ordinal,
						 TPM_PERMANENT_DATA *tpm_permanent_data)
{
    TPM_RESULT		rc = 0;
    size_t		index;		/* index of ordinal in array */
    unsigned int	offset;		/* bit position of ordinal in array */
    unsigned char	bit;	

    if (rc == 0) {
	/* handle the TPM ordinals */
	if (ordinal < TPM_ORDINALS_MAX) {
	    index = ordinal/CHAR_BIT;
	    offset = ordinal % CHAR_BIT;
	    bit = 0x01 << offset;
	    *auditStatus = tpm_permanent_data->ordinalAuditStatus[index] & bit;
	}
	/* handle the TSC ordinals */
	else if (ordinal == TSC_ORD_PhysicalPresence) {
	    *auditStatus = tpm_permanent_data->tscOrdinalAuditStatus & TSC_PHYS_PRES_AUDIT;
	}
	else if (ordinal == TSC_ORD_ResetEstablishmentBit) {
	    *auditStatus = tpm_permanent_data->tscOrdinalAuditStatus & TSC_RESET_ESTAB_AUDIT;
	}
	else {
	    printf("TPM_OrdinalAuditStatus_GetAuditStatus: Error (fatal) "
		   "ordinal %08x out of range\n", ordinal);
	    rc = TPM_FAIL;	/* should never occur, always called with ordinal processing */
	}
    }
    /* trace the ordinals with auditing enabled */
    if ((rc == 0) && *auditStatus) {
	printf("  TPM_OrdinalAuditStatus_GetAuditStatus: ordinal %08x status %02x\n",
	       ordinal, *auditStatus);
    }
    return rc;
}

/* TPM_OrdinalAuditStatus_SetAuditStatus() sets the TPM_PERMANENT_DATA -> ordinalAuditStatus for the
   ordinal

   The flags are stored as a bit map to conserve NVRAM.

   The array is not written back to NVRAM.  On error, TPM_PERMANENT_DATA is not changed.

   altered is TRUE if the bit was changed, 
*/

TPM_RESULT TPM_OrdinalAuditStatus_SetAuditStatus(TPM_BOOL *altered,
						 TPM_PERMANENT_DATA *tpm_permanent_data,
						 TPM_BOOL auditStatus,
						 TPM_COMMAND_CODE ordinal)
{
    TPM_RESULT		rc = 0;
    TPM_BOOL		auditable;	/* TRUE if the ordinal is auditable by this TPM
					   implementation */
    size_t		index;		/* index of ordinal in array */
    unsigned int	offset;		/* bit position of ordinal in array */
    unsigned char	bit;

    *altered = FALSE;			/* default, returned on error */
#if 0
    printf(" TPM_OrdinalAuditStatus_SetAuditStatus: ordinal %08x status %02x\n",
	   ordinal, auditStatus);
#endif
    /* If trying to set, screen against the 'never audit' ordinal table */
    if ((rc == 0) && auditStatus) {
	TPM_OrdinalTable_GetAuditable(&auditable, ordinal);
	/* if it is a 'never audit' ordinal, it can not be set */
	if (!auditable) {
	    printf("TPM_OrdinalAuditStatus_SetAuditStatus: "
		   "Error, cannot audit ordinal %08x\n", ordinal);
	    rc = TPM_BAD_PARAMETER;
	}
    }
    if (rc == 0) {
	/* handle the TPM ordinals */
	if (ordinal < TPM_ORDINALS_MAX) {
	    index = ordinal/CHAR_BIT;
	    offset = ordinal % CHAR_BIT;
	    bit = 0x01 << offset;
	    /* determine if the bit is to be altered */
	    if (((tpm_permanent_data->ordinalAuditStatus[index] & bit) && !auditStatus) ||
		(!(tpm_permanent_data->ordinalAuditStatus[index] & bit) && auditStatus)) {

		*altered = TRUE;
	    }
	    if (auditStatus) {
		/* set the bit */
		tpm_permanent_data->ordinalAuditStatus[index] |= bit;
	    }
	    else {
		/* clear the bit */
		tpm_permanent_data->ordinalAuditStatus[index] &= ~bit;
	    }
	}
	/* handle the TSC ordinals */
	else if (ordinal == TSC_ORD_PhysicalPresence) {
	    /* determine if the bit is to be altered */
	    if (((tpm_permanent_data->tscOrdinalAuditStatus & TSC_PHYS_PRES_AUDIT)
		 && !auditStatus) ||
		(!(tpm_permanent_data->tscOrdinalAuditStatus & TSC_PHYS_PRES_AUDIT)
		 && auditStatus)) {

		*altered = TRUE;
	    }
	    if (auditStatus) {
		tpm_permanent_data->tscOrdinalAuditStatus |= TSC_PHYS_PRES_AUDIT;
	    }
	    else {
		tpm_permanent_data->tscOrdinalAuditStatus &= ~TSC_PHYS_PRES_AUDIT;
	    }
	}
	else if (ordinal == TSC_ORD_ResetEstablishmentBit) {
	    if (auditStatus) {
		/* determine if the bit is to be altered */
		if (((tpm_permanent_data->tscOrdinalAuditStatus & TSC_RESET_ESTAB_AUDIT)
		     && !auditStatus) ||
		    (!(tpm_permanent_data->tscOrdinalAuditStatus & TSC_RESET_ESTAB_AUDIT)
		     && auditStatus)) {

		    *altered = TRUE;
		}
		tpm_permanent_data->tscOrdinalAuditStatus |= TSC_RESET_ESTAB_AUDIT;
	    }
	    else {
		tpm_permanent_data->tscOrdinalAuditStatus &= ~TSC_RESET_ESTAB_AUDIT;
	    }
	}
	else {
	    printf("TPM_OrdinalAuditStatus_SetAuditStatus: Error ordinal %08x out of range\n",
		   ordinal);
	    rc = TPM_BADINDEX;
	}
    }
    return rc;
}

/*
  Common Processing Functions
*/

/* 8.1 Audit Generation rev 109

   TPM_AuditDigest_ExtendIn() extends the audit digest with a digest of input parameters
*/

TPM_RESULT TPM_AuditDigest_ExtendIn(tpm_state_t *tpm_state,
				    TPM_DIGEST inParamDigest)
{
    TPM_RESULT		rc = 0;
    TPM_AUDIT_EVENT_IN	tpm_audit_event_in;
    TPM_STORE_BUFFER	eventIn_sbuffer;
    const unsigned char *eventIn_buffer;	/* serialized buffer */
    uint32_t		eventIn_length;		/* serialization length */
    
    printf(" TPM_AuditDigest_ExtendIn:\n");
    TPM_AuditEventIn_Init(&tpm_audit_event_in);		/* freed @1 */
    TPM_Sbuffer_Init(&eventIn_sbuffer);			/* freed @2 */

    if (rc == 0) {
	/* b. Create A1 a TPM_AUDIT_EVENT_IN structure */
	/* NOTE Done by TPM_AuditEventIn_Init() */
	/* i. Set A1 -> inputParms to the digest of the input parameters from the command */
	/* (1) Digest value according to the HMAC digest rules of the "above the line" parameters
	   (i.e. the first HMAC digest calculation). */
	TPM_Digest_Copy(tpm_audit_event_in.inputParms, inParamDigest);
	/* ii. Set A1 -> auditCount to TPM_PERMANENT_DATA -> auditMonotonicCounter */
	TPM_CounterValue_CopyPublic(&(tpm_audit_event_in.auditCount),
				    &(tpm_state->tpm_permanent_data.auditMonotonicCounter));
	/* serialize the A1 TPM_AUDIT_EVENT_IN object */
	rc = TPM_AuditEventIn_Store(&eventIn_sbuffer, &tpm_audit_event_in);

    }
    if (rc == 0) {
	/* get the serialization results */
	TPM_Sbuffer_Get(&eventIn_sbuffer, &eventIn_buffer, &eventIn_length);
	/* c. Set TPM_STANY_DATA -> auditDigest to SHA-1 (TPM_STANY_DATA -> auditDigest || A1) */
	TPM_PrintFour("  TPM_AuditDigest_ExtendIn: Previous digest",
		      tpm_state->tpm_stclear_data.auditDigest);
	TPM_PrintAll("  TPM_AuditDigest_ExtendIn: TPM_AUDIT_EVENT_IN", eventIn_buffer, eventIn_length);
	rc = TPM_SHA1(tpm_state->tpm_stclear_data.auditDigest,
		      TPM_DIGEST_SIZE, tpm_state->tpm_stclear_data.auditDigest,
		      eventIn_length, eventIn_buffer,
		      0, NULL);
	TPM_PrintFour("  TPM_AuditDigest_ExtendIn: Current digest (in)",
		      tpm_state->tpm_stclear_data.auditDigest);
    }
    TPM_AuditEventIn_Delete(&tpm_audit_event_in);	/* @1 */
    TPM_Sbuffer_Delete(&eventIn_sbuffer);		/* @2 */
    return rc;
}

/* 8.1 Audit Generation rev 109

   TPM_AuditDigest_ExtendOut() extends the audit digest with a digest of output parameters
*/

TPM_RESULT TPM_AuditDigest_ExtendOut(tpm_state_t *tpm_state,
				     TPM_DIGEST outParamDigest)
{
    TPM_RESULT		rc = 0;
    TPM_AUDIT_EVENT_OUT tpm_audit_event_out;
    TPM_STORE_BUFFER	eventOut_sbuffer;
    const unsigned char *eventOut_buffer;	/* serialized buffer */
    uint32_t		eventOut_length;	/* serialization length */
    
    printf(" TPM_AuditDigest_ExtendOut:\n");
    TPM_AuditEventOut_Init(&tpm_audit_event_out);	/* freed @1 */
    TPM_Sbuffer_Init(&eventOut_sbuffer);		/* freed @2 */

    if (rc == 0) {
	/* d. Create A2 a TPM_AUDIT_EVENT_OUT structure */
	/* NOTE Done by TPM_AuditEventOut_Init() */
	/* i. Set A2 -> outputParms to the digest of the output parameters from the command */
	/* (1). Digest value according to the HMAC digest rules of the "above the line" parameters
	   (i.e. the first HMAC digest calculation). */
	TPM_Digest_Copy(tpm_audit_event_out.outputParms, outParamDigest);
	/* ii. Set A2 -> auditCount to TPM_PERMANENT_DATA -> auditMonotonicCounter */
	TPM_CounterValue_CopyPublic(&(tpm_audit_event_out.auditCount),
				    &(tpm_state->tpm_permanent_data.auditMonotonicCounter));
	/* serialize the A2 TPM_AUDIT_EVENT_OUT object */
	rc = TPM_AuditEventOut_Store(&eventOut_sbuffer, &tpm_audit_event_out);
    }
    if (rc == 0) {
	/* get the serialization results */
	TPM_Sbuffer_Get(&eventOut_sbuffer, &eventOut_buffer, &eventOut_length);
	/* e. Set TPM_STANY_DATA -> auditDigest to SHA-1 (TPM_STANY_DATA -> auditDigest || A2) */
	TPM_PrintFour("  TPM_AuditDigest_ExtendOut: Previous digest",
		      tpm_state->tpm_stclear_data.auditDigest);
	TPM_PrintAll("  TPM_AuditDigest_ExtendOut: TPM_AUDIT_EVENT_OUT", eventOut_buffer, eventOut_length);
	rc = TPM_SHA1(tpm_state->tpm_stclear_data.auditDigest,
		      TPM_DIGEST_SIZE, tpm_state->tpm_stclear_data.auditDigest,
		      eventOut_length, eventOut_buffer,
		      0, NULL);
	TPM_PrintFour("  TPM_AuditDigest_ExtendOut: Current digest (out)",
		      tpm_state->tpm_stclear_data.auditDigest);
    }
    TPM_AuditEventOut_Delete(&tpm_audit_event_out);	/* @1 */
    TPM_Sbuffer_Delete(&eventOut_sbuffer);		/* @2 */
    return rc;
}

/*
  Processing Functions
*/

/* The TPM generates an audit event in response to the TPM executing a command that has the audit
   flag set to TRUE for that command.

   The TPM maintains an extended value for all audited operations.
*/

/* 8.3 TPM_GetAuditDigest rev 87

   This returns the current audit digest. The external audit log has the responsibility to track the
   parameters that constitute the audit digest.

   This value may be unique to an individual TPM.  The value however will be changing at a rate set
   by the TPM Owner. Those attempting to use this value may find it changing without their
   knowledge.  This value represents a very poor source of tracking uniqueness.
*/

TPM_RESULT TPM_Process_GetAuditDigest(tpm_state_t *tpm_state,
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
    uint32_t	startOrdinal;	/* The starting ordinal for the list of audited ordinals */

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
    TPM_DIGEST		auditDigest;	/* Log of all audited events */
    TPM_BOOL		more;		/* TRUE if the output does not contain a full list of
					   audited ordinals */
    TPM_SIZED_BUFFER	ordList;	/* List of ordinals that are audited. */

    printf("TPM_Process_GetAuditDigest: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&ordList);	/* freed @1 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get startOrdinal parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&startOrdinal, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_GetAuditDigest: startOrdinal %08x\n", startOrdinal);
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
	    printf("TPM_Process_GetAuditDigest: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    if (returnCode == TPM_SUCCESS) {
	/* 1. The TPM sets auditDigest to TPM_STANY_DATA -> auditDigest */
	TPM_Digest_Copy(auditDigest, tpm_state->tpm_stclear_data.auditDigest);
	/* 2. The TPM sets counterValue to TPM_PERMANENT_DATA -> auditMonotonicCounter */
	/* NOTE Since there is only one, use it directly on the output */
	printf("TPM_Process_GetAuditDigest: Counter value %08x\n",
	       tpm_state->tpm_permanent_data.auditMonotonicCounter.counter);
	/* 3. The TPM creates an ordered list of audited ordinals. The list starts at startOrdinal
	   listing each ordinal that is audited. */
	/* a. If startOrdinal is 0 then the first ordinal that could be audited would be TPM_OIAP
	   (ordinal 0x0000000A) */
	/* b. The next ordinal would be TPM_OSAP (ordinal 0x0000000B) */
	returnCode = TPM_OrdinalAuditStatus_Store(&ordList,
						  &(tpm_state->tpm_permanent_data),
						  startOrdinal);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_GetAuditDigest: ordSize %u\n", ordList.size);
	/* 4. If the ordered list does not fit in the output buffer the TPM sets more to TRUE */
	more = FALSE;
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_GetAuditDigest: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	/* append counterValue */
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* append counterValue */
	    returnCode = TPM_CounterValue_StorePublic
			 (response,
			  &(tpm_state->tpm_permanent_data.auditMonotonicCounter));
	}
	/* 5. Return TPM_STANY_DATA -> auditDigest as auditDigest */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Digest_Store(response, auditDigest);
	}
	/* append more */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Sbuffer_Append(response, &more, sizeof(TPM_BOOL));
	}
	/* append ordList */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_SizedBuffer_Store(response, &ordList);
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
    TPM_SizedBuffer_Delete(&ordList);	/* @1 */
    return rcf;
}

/* 8.4	TPM_GetAuditDigestSigned rev 101

   The signing of the audit log returns the entire digest value and the list of currently audited
   commands.

   The inclusion of the list of audited commands as an atomic operation is to tie the current digest
   value with the list of commands that are being audited.

   Note to future architects

   When auditing functionality is active in a TPM, it may seem logical to remove this ordinal from
   the active set of ordinals as the signing functionality of this command could be handled in a
   signed transport session. While true this command has a secondary affect also, resetting the
   audit log digest. As the reset requires TPM Owner authentication there must be some way in this
   command to reflect the TPM Owner wishes. By requiring that a TPM Identity key be the only key
   that can sign and reset the TPM Owners authentication is implicit in the execution of the command
   (TPM Identity Keys are created and controlled by the TPM Owner only). Hence while one might want
   to remove an ordinal this is not one that can be removed if auditing is functional.
*/

TPM_RESULT TPM_Process_GetAuditDigestSigned(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE	keyHandle;	/* The handle of a loaded key that can perform digital
					   signatures. */
    TPM_BOOL		closeAudit;	/* Indication if audit session should be closed */
    TPM_NONCE		antiReplay;	/* A nonce to prevent replay attacks */
    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle used for key
					   authentication. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession;	/* The continue use flag for the authorization session
					   handle */
    TPM_AUTHDATA	keyAuth;	/* Authorization. HMAC key: key.usageAuth. */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_KEY			*sigKey = NULL;			/* the key specified by keyHandle */
    TPM_SECRET			*keyUsageAuth;
    TPM_BOOL			parentPCRStatus;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_SIGN_INFO		d1SignInfo;
    TPM_SIZED_BUFFER		d3SizedBuffer;	/* List of ordinals that are audited. */
    TPM_STORE_BUFFER		d2Sbuffer;	/* data to be signed */
    TPM_DIGEST			h1;
    
    /* output parameters */
    uint32_t			outParamStart;	/* starting point of outParam's */
    uint32_t			outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST			outParamDigest;
    TPM_DIGEST			ordinalDigest;	/* Digest of all audited ordinals */
    TPM_SIZED_BUFFER		sig;		/* The signature of the area */

    printf("TPM_Process_GetAuditDigestSigned: Ordinal Entry\n");
    TPM_SignInfo_Init(&d1SignInfo);		/* freed @1 */
    TPM_SizedBuffer_Init(&d3SizedBuffer);	/* freed @2 */
    TPM_Sbuffer_Init(&d2Sbuffer);		/* freed @3 */
    TPM_SizedBuffer_Init(&sig);			/* freed @4 */
    /*
      get inputs
    */
    /* get keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&keyHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get closeAudit parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_GetAuditDigestSigned: keyHandle %08x\n", keyHandle);
	returnCode = TPM_LoadBool(&closeAudit, &command, &paramSize);
    }
    /* get antiReplay parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Digest_Load(antiReplay, &command, &paramSize);
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
	    printf("TPM_Process_GetAuditDigestSigned: Error, command has %u extra bytes\n",
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
    /* 1. Validate the AuthData and parameters using keyAuth, return TPM_AUTHFAIL on error */
    /* get the key corresponding to the keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&sigKey, &parentPCRStatus, tpm_state, keyHandle,
						 FALSE,		/* not read-only */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND)){
	if (sigKey->authDataUsage != TPM_AUTH_NEVER) {
	    printf("TPM_Process_GetAuditDigestSigned: Error, authorization required\n");
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
    /* validate the authorization to use the key pointed to by keyHandle */
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
    /* 2.Validate that keyHandle -> keyUsage is TPM_KEY_SIGNING, TPM_KEY_IDENTITY or TPM_KEY_LEGACY,
	 if not return TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	if ((sigKey->keyUsage != TPM_KEY_SIGNING) &&
	    (sigKey->keyUsage != TPM_KEY_IDENTITY) &&
	    (sigKey->keyUsage != TPM_KEY_LEGACY)) {
	    printf("TPM_Process_GetAuditDigestSigned: Error, keyUsage %04hx is invalid\n",
		   sigKey->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 3. The TPM validates that the key pointed to by keyHandle has a signature scheme of
       TPM_SS_RSASSAPKCS1v15_SHA1 or TPM_SS_RSASSAPKCS1v15_INFO, return TPM_INVALID_KEYUSAGE on
       error */
    if (returnCode == TPM_SUCCESS) {
	if ((sigKey->algorithmParms.sigScheme != TPM_SS_RSASSAPKCS1v15_SHA1) &&
	    (sigKey->algorithmParms.sigScheme != TPM_SS_RSASSAPKCS1v15_INFO)) {
	    printf("TPM_Process_GetAuditDigestSigned: Error, invalid sigScheme %04hx\n",
		   sigKey->algorithmParms.sigScheme);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    if (returnCode == TPM_SUCCESS) {
	/* 4. Create D1 a TPM_SIGN_INFO structure and set the structure defaults */
	/* NOTE Done by TPM_SignInfo_Init() */
	/* a. Set D1 -> fixed to "ADIG" */
	memcpy(d1SignInfo.fixed, "ADIG", TPM_SIGN_INFO_FIXED_SIZE);
	/* b. Set D1 -> replay to antiReplay */
	TPM_Nonce_Copy(d1SignInfo.replay, antiReplay);
	/* c. Create D3 a list of all audited ordinals as defined in the TPM_GetAuditDigest
	   uint32_t[] ordList outgoing parameter */
	returnCode = TPM_OrdinalAuditStatus_Store(&d3SizedBuffer,
						  &(tpm_state->tpm_permanent_data),
						  0);
    }
    /* d. Create D4 (ordinalDigest outgoing parameter) the SHA-1 of D3 */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SHA1(ordinalDigest,
			      d3SizedBuffer.size, d3SizedBuffer.buffer, 
			      0, NULL);
    }
    if (returnCode == TPM_SUCCESS) {
	/* e. Set auditDigest to TPM_STANY_DATA -> auditDigest */
	/* NOTE: Use it directly on the output */
	/* f. Set counterValue to TPM_PERMANENT_DATA -> auditMonotonicCounter */
	/* NOTE Since there is only one, use it directly on the output */
	/* g. Create D2 the concatenation of auditDigest || counterValue || D4 */
	returnCode = TPM_Sbuffer_Append(&d2Sbuffer,
					tpm_state->tpm_stclear_data.auditDigest, TPM_DIGEST_SIZE);
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode =
	    TPM_CounterValue_StorePublic(&d2Sbuffer,
					 &(tpm_state->tpm_permanent_data.auditMonotonicCounter));
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Sbuffer_Append(&d2Sbuffer,
					ordinalDigest, TPM_DIGEST_SIZE);
    }
    /* h. Set D1 -> data to D2 */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_SetFromStore(&(d1SignInfo.data), &d2Sbuffer);
    }
    /* i. Create a digital signature of the SHA-1 of D1 by using the signature scheme for keyHandle
     */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SHA1_GenerateStructure(h1, &d1SignInfo,
						(TPM_STORE_FUNCTION_T)TPM_SignInfo_Store);
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_RSASignToSizedBuffer(&sig,		/* signature */
					      h1,		/* message */
					      TPM_DIGEST_SIZE,	/* message size */
					      sigKey);		/* input, signing key */
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_GetAuditDigestSigned: auditDigest",
		      tpm_state->tpm_stclear_data.auditDigest); 
	TPM_PrintFour("TPM_Process_GetAuditDigestSigned: ordinalDigest",
		      ordinalDigest); 
    }
    /* j. Set ordinalDigest to D4 */
    /* NOTE Created directly in ordinalDigest */
    /* 5. If closeAudit == TRUE */
    if ((returnCode == TPM_SUCCESS) && closeAudit) {
	/* a. If keyHandle->keyUsage is TPM_KEY_IDENTITY */
	if (sigKey->keyUsage == TPM_KEY_IDENTITY) {
	    /* i. TPM_STANY_DATA -> auditDigest MUST be set to all zeros. */
	    TPM_Digest_Init(tpm_state->tpm_stclear_data.auditDigest);
	}
	/* b. Else */
	else {
	    /* i. Return TPM_INVALID_KEYUSAGE */
	    printf("TPM_Process_GetAuditDigestSigned: Error, "
		   "cannot closeAudit with keyUsage %04hx\n", sigKey->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_GetAuditDigestSigned: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return counterValue */
	    returnCode = TPM_CounterValue_StorePublic
			 (response,
			  &(tpm_state->tpm_permanent_data.auditMonotonicCounter));
	}
	/* return auditDigest */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Digest_Store(response,  tpm_state->tpm_stclear_data.auditDigest);
	}
	/* return ordinalDigest */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Digest_Store(response, ordinalDigest);
	}
	/* return sig */
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
    TPM_SignInfo_Delete(&d1SignInfo);		/* @1 */
    TPM_SizedBuffer_Delete(&d3SizedBuffer);	/* @2 */
    TPM_Sbuffer_Delete(&d2Sbuffer);		/* @3 */
    TPM_SizedBuffer_Delete(&sig);		/* @4 */
    return rcf;
}

/* 8.5 TPM_SetOrdinalAuditStatus rev 109

   Set the audit flag for a given ordinal. This command requires the authentication of the TPM
   Owner.
*/

TPM_RESULT TPM_Process_SetOrdinalAuditStatus(tpm_state_t *tpm_state,
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
    TPM_COMMAND_CODE	ordinalToAudit; /* The ordinal whose audit flag is to be set */
    TPM_BOOL		auditState;	/* Value for audit flag */
    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle used for owner
					   authentication. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   session handle */
    TPM_AUTHDATA	ownerAuth;	/* The authorization session digest for inputs and owner
					   authentication.  HMAC key: ownerAuth. */
  
    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			altered;		/* status is changing */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    
    /* output parameters */
    uint32_t			outParamStart;	/* starting point of outParam's */
    uint32_t			outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST			outParamDigest;

    printf("TPM_Process_SetOrdinalAuditStatus: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get ordinalToAudit parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&ordinalToAudit, &command, &paramSize);
    }
    /* get auditState parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_LoadBool(&auditState, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_SetOrdinalAuditStatus: ordinalToAudit %08x auditState %02x\n",
	       ordinalToAudit, auditState);
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
	    printf("TPM_Process_SetOrdinalAuditStatus: Error, command has %u extra bytes\n",
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
    /* 1. Validate the AuthData to execute the command and the parameters */
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
    /* calculate and set the below the line parameters */
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
    /* 2. Validate that the ordinal points to a valid TPM ordinal, return TPM_BADINDEX on error */
    /* a. Valid TPM ordinal means an ordinal that the TPM implementation supports */
    /* Done by TPM_OrdinalAuditStatus_SetAuditState() */
    /* 3. Set the non-volatile flag associated with ordinalToAudit to the value in auditState */
    /* NOTE: On error, TPM_PERMANENT_DATA is not changed */
    if (returnCode == TPM_SUCCESS) {
	returnCode =
	    TPM_OrdinalAuditStatus_SetAuditStatus(&altered,
						  &(tpm_state->tpm_permanent_data),
						  auditState,		/* uninitialized */
						  ordinalToAudit);
	/* It's not really uninitialized, but beam doesn't understand that TPM_GetInParamDigest()
	   can't turn a FALSE into a TRUE */
    }
    /* Store the permanent data back to NVRAM */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_PermanentAll_NVStore(tpm_state,
					      altered,
					      returnCode);
    }
    /* Audit Generation 3.b. Corner Cases: TPM_SetOrdinalAuditStatus: In the case where the
       ordinalToAudit is TPM_ORD_SetOrdinalAuditStatus, audit is based on the initial state, not the
       final state. */
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_SetOrdinalAuditStatus: Ordinal returnCode %08x %u\n",
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
    /* if there was an error, terminate the session.  */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    return rcf;
}
