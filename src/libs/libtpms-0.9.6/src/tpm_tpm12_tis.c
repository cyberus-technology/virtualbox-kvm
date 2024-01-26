/********************************************************************************/
/*                                                                              */
/*                              TPM TIS I/O					*/
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_tis.c $		*/
/*                                                                              */
/* (c) Copyright IBM Corporation 2006, 2011.					*/
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

/*
  This file implements the TPM TIS interface out-of-band commands.
*/

#include <stdio.h>
#include <string.h>

#include "tpm12/tpm_crypto.h"
#include "tpm12/tpm_cryptoh.h"
#include "tpm_debug.h"
#include "tpm_error.h"
#include "tpm12/tpm_digest.h"
#include "tpm12/tpm_global.h"
#include "tpm12/tpm_pcr.h"
#include "tpm12/tpm_permanent.h"
#include "tpm12/tpm_platform.h"
#include "tpm12/tpm_process.h"
#include "tpm12/tpm_transport.h"

#include "tpm_tis.h"

/* These commands do not test for TPM_ContinueSelfTest:

   The following operations MUST be available after TPM_Init and before a call to
   TPM_ContinueSelfTest 1.9. TPM_HASH_START / TPM_HASH_DATA / TPM_HASH_END */

/* TPM_IO_Hash_Start() implements the LPC bus TPM_HASH_START command
 */
TPM_RESULT TPM12_IO_Hash_Start(void)
{
    TPM_RESULT		rc = 0;
    tpm_state_t		*tpm_state = tpm_instances[0];	/* TPM global state */
    TPM_PCRVALUE	zeroPCR;
    TPM_BOOL		altered = FALSE;	/* TRUE if the structure has been changed */

    printf("\nTPM_IO_Hash_Start: Ordinal Entry\n");
    TPM_Digest_Init(zeroPCR);

    /* Prior to receiving the TPM_HASH_START command the TPM must have received a TPM_Startup
       command. If the TPM receives a TPM_HASH_START after a TPM_Init but before a startup command,
       the TPM treats this as an error */
    if (rc == 0) {
	if (tpm_state->tpm_stany_flags.postInitialise) {
	    printf("TPM_IO_Hash_Start: Error, postInitialise is TRUE\n");
	    rc = TPM_INVALID_POSTINIT;
	}
    }
    /* NOTE: Done by caller
       (1) If no TPM_ACCESS_x.activeLocality field is set, the TPM MUST set the
       TPM_ACCESS_x.activeLocality field to indicate Locality 4. Any currently executing command
       MUST be aborted per and subject to Section 11.2.3. */
    /* NOTE: Done by caller
       (2) If TPM_ACCESS_x.activeLocality is set, and if the TPM_ACCESS_x.activeLocality field is
       not 4, the TPM MUST ignore this command. */
    /* NOTE: Done by caller
       (3) The TPM MUST clear the write FIFO. */
    if (rc == 0) {
	/* (4) If there is an exclusive transport session, it MUST be invalidated. */
	if (tpm_state->tpm_stany_flags.transportExclusive != 0)	{ /* active exclusive */
	    rc = TPM_TransportSessions_TerminateHandle
		 (tpm_state->tpm_stclear_data.transSessions,
		  tpm_state->tpm_stany_flags.transportExclusive,
		  &(tpm_state->tpm_stany_flags.transportExclusive));
	}
    }
    if (rc == 0) {
	/* (5) Set the TPM_PERMANENT_FLAGS->tpmEstablished flag to TRUE (1). Note: see description of
	   Bit Field: tpmEstablishment in 11.2.11 Access Register. */
	TPM_SetCapability_Flag(&altered,
			       &(tpm_state->tpm_permanent_flags.tpmEstablished),
			       TRUE);
    }
    if (rc == 0) {
	/* (6) Set the TPM_STANY_FLAGS->TOSPresent flag to TRUE (1). */
	tpm_state->tpm_stany_flags.TOSPresent = TRUE;
	/* (7) Set PCRs per column labeled TPM_HASH_START in Table 5: PCR Initial and Reset Values.
	   (PCR 17-22 to zero, others unchanged */
	TPM_PCR_Store(tpm_state->tpm_stclear_data.PCRS, 17, zeroPCR);
	TPM_PCR_Store(tpm_state->tpm_stclear_data.PCRS, 18, zeroPCR);
	TPM_PCR_Store(tpm_state->tpm_stclear_data.PCRS, 19, zeroPCR);
	TPM_PCR_Store(tpm_state->tpm_stclear_data.PCRS, 20, zeroPCR);
	TPM_PCR_Store(tpm_state->tpm_stclear_data.PCRS, 21, zeroPCR);
	TPM_PCR_Store(tpm_state->tpm_stclear_data.PCRS, 22, zeroPCR);
	/* (8) Ignore any data component of the TPM_HASH_START LPC command. */
	/* (9) Allocate tempLocation of a size required to perform the SHA-1 operation. */
	/* (10) Initialize tempLocation per SHA-1. */
	rc = TPM_SHA1InitCmd(&(tpm_state->sha1_context_tis));
    }
    rc = TPM_PermanentAll_NVStore(tpm_state,
				  altered,
				  rc);
    /*
       1) Upon any error in the above steps the TPM:
       a) MUST enter Failure Mode.
       NOTE: Done by caller
       b) MUST release locality.
    */
    if (rc != 0) {
	printf("TPM_IO_Hash_Start: Error, (fatal)\n");
	printf("  TPM_IO_Hash_Start: Set testState to %u \n", TPM_TEST_STATE_FAILURE);
	tpm_state->testState = TPM_TEST_STATE_FAILURE;
    }
    return rc;
}

/* TPM_IO_Hash_Data() implements the LPC bus TPM_HASH_DATA command
 */
TPM_RESULT TPM12_IO_Hash_Data(const unsigned char *data,
			      uint32_t data_length)
{
    TPM_RESULT 		rc = 0;
    tpm_state_t		*tpm_state = tpm_instances[0];	/* TPM global state */

    printf("\nTPM_IO_Hash_Data: Ordinal Entry\n");
    /* (1) Transform tempLocation per SHA-1 with data received from this command. */
    /* (2) Repeat for each TPM_HASH_DATA LPC command received. */
    if (rc == 0) {
	if (tpm_state->sha1_context_tis == NULL) {
	    printf("TPM_IO_Hash_Data: Error, no existing SHA1 thread\n");
	    rc = TPM_SHA_THREAD;
	}
    }
    if (rc == 0) {
	rc = TPM_SHA1UpdateCmd(tpm_state->sha1_context_tis, data, data_length);
    }
    /*
       1) Upon any error in the above steps the TPM:
       a) MUST enter Failure Mode.
       NOTE: Done by caller
       b) MUST release locality.
    */
    if (rc != 0) {
	printf("TPM_IO_Hash_Data: Error, (fatal)\n");
	printf("  TPM_IO_Hash_Data: Set testState to %u \n", TPM_TEST_STATE_FAILURE);
	tpm_state->testState = TPM_TEST_STATE_FAILURE;
    }
    return rc;
}

/* TPM_IO_Hash_End() implements the LPC bus TPM_HASH_END command
 */
TPM_RESULT TPM12_IO_Hash_End(void)
{
    TPM_RESULT 		rc = 0;
    TPM_PCRVALUE	zeroPCR;
    TPM_DIGEST 		extendDigest;
    tpm_state_t		*tpm_state = tpm_instances[0];	/* TPM global state */

    printf("\nTPM_IO_Hash_End: Ordinal Entry\n");
    if (rc == 0) {
	if (tpm_state->sha1_context_tis == NULL) {
	    printf("TPM_IO_Hash_End: Error, no existing SHA1 thread\n");
	    rc = TPM_SHA_THREAD;
	}
    }
    /* (1) Ignore any data sent with the command. */
    /* (2) Perform finalize operation on tempLocation per SHA-1. */
    if (rc == 0) {
	rc = TPM_SHA1FinalCmd(extendDigest, tpm_state->sha1_context_tis);
    }
    /* (3) Perform an “extend” operation, as defined in the TPM_Extend command, of the value within
       tempLocation into PCR[Locality 4]. */
    if (rc == 0) {
	/* In the previous line above, “PCR[Locality 4]” within and before the SHA-1 function is
	   TPM_PCRVALUE = 0 (i.e., 20 bytes of all zeros). */
	TPM_Digest_Init(zeroPCR);	/* initial PCR value */
	/* PCR[Locality 4] = SHA-1( PCR[Locality 4] || tempLoc) */
	rc = TPM_SHA1(tpm_state->tpm_stclear_data.PCRS[TPM_LOCALITY_4_PCR],
		      TPM_DIGEST_SIZE, zeroPCR,
		      TPM_DIGEST_SIZE, extendDigest,
		      0, NULL);
    }
    /* NOTE: Done by caller
       (4) Clear TPM_ACCESS_x.activeLocality for Locality 4. */
    /*
       1) Upon any error in the above steps the TPM:
       a) MUST enter Failure Mode.
       NOTE: Done by caller
       b) MUST release locality.
    */
    if (rc != 0) {
	printf("TPM_IO_Hash_End: Error, (fatal)\n");
	printf("  TPM_IO_Hash_End: Set testState to %u \n", TPM_TEST_STATE_FAILURE);
	tpm_state->testState = TPM_TEST_STATE_FAILURE;
    }
    TPM_SHA1Delete(&(tpm_state->sha1_context_tis));
    return rc;
}

TPM_RESULT TPM12_IO_TpmEstablished_Get(TPM_BOOL *tpmEstablished)
{
    TPM_RESULT 		rc = 0;
    tpm_state_t		*tpm_state = tpm_instances[0];	/* TPM global state */

    if (rc == 0) {
	*tpmEstablished = tpm_state->tpm_permanent_flags.tpmEstablished;
    }
    /*
       1) Upon any error in the above steps the TPM:
       a) MUST enter Failure Mode.
       NOTE: Done by caller
       b) MUST release locality.
    */
    if (rc != 0) {
	printf("TPM_IO_TpmEstablished_Get: Error, (fatal)\n");
	printf("  TPM_IO_TpmEstablished_Get: Set testState to %u \n", TPM_TEST_STATE_FAILURE);
	tpm_state->testState = TPM_TEST_STATE_FAILURE;
    }
    return 0;
}

TPM_RESULT TPM12_IO_TpmEstablished_Reset(void)
{
    TPM_RESULT          returnCode = 0;
    tpm_state_t		*tpm_state = tpm_instances[0];	/* TPM global state */
    TPM_BOOL		writeAllNV = FALSE;	/* flag to write back flags */

    if (returnCode == TPM_SUCCESS) {
        returnCode = TPM_IO_GetLocality(&(tpm_state->tpm_stany_flags.localityModifier),
                                        tpm_state->tpm_number);
    }

    /* 1. Validate the assertion of locality 3 or locality 4 */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Locality_Check(TPM_LOC_THREE | TPM_LOC_FOUR,  /* BYTE bitmap */
					tpm_state->tpm_stany_flags.localityModifier);
    }
    /* 2. Set TPM_PERMANENT_FLAGS -> tpmEstablished to FALSE */
    if (returnCode == TPM_SUCCESS) {
	TPM_SetCapability_Flag(&writeAllNV,					/* altered */
			       &(tpm_state->tpm_permanent_flags.tpmEstablished),	/* flag */
			       FALSE);							/* value */

    }
    /* Store the permanent flags back to NVRAM */
    returnCode = TPM_PermanentAll_NVStore(tpm_state,
					  writeAllNV,
					  returnCode);

    return returnCode;
}
