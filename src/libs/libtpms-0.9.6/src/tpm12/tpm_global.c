/********************************************************************************/
/*                                                                              */
/*                           Global Variables                                   */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_global.c $            */
/*                                                                              */
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

#include "tpm_crypto.h"
#include "tpm_debug.h"
#include "tpm_digest.h"
#include "tpm_error.h"
#include "tpm_io.h"
#include "tpm_init.h"
#include "tpm_key.h"
#include "tpm_nvfile.h"
#include "tpm_nvram.h"
#include "tpm_permanent.h"
#include "tpm_platform.h"
#include "tpm_startup.h"
#include "tpm_structures.h"


#include "tpm_global.h"

/* state for the TPM's */
tpm_state_t *tpm_instances[TPMS_MAX];

/* TPM_Global_Init initializes the tpm_state to default values.

   It does not load any data from or store data to NVRAM
*/

TPM_RESULT TPM_Global_Init(tpm_state_t *tpm_state)
{
    TPM_RESULT rc = 0;
    
    printf("TPM_Global_Init: TPMs %lu\n",
           (unsigned long)sizeof(tpm_instances)/sizeof(tpm_state_t *));
    /* initialize the TPM_STANY_FLAGS structure */
    if (rc == 0) {
        /* set the structure to 0 for security, clean out old secrets */
        memset(tpm_state, 0 , sizeof(tpm_state_t));
        /* the virtual TPM number NOTE: This must be done early as it is used to construct
           nn.permall file names */
        tpm_state->tpm_number = TPM_ILLEGAL_INSTANCE_HANDLE;
        /* initialize the TPM_PERMANENT_FLAGS structure */
        printf("TPM_Global_Init: Initializing TPM_PERMANENT_FLAGS\n");
        TPM_PermanentFlags_Init(&(tpm_state->tpm_permanent_flags));
        /* initialize the TPM_STCLEAR_FLAGS structure */
        printf("TPM_Global_Init: Initializing TPM_STCLEAR_FLAGS\n");
        TPM_StclearFlags_Init(&(tpm_state->tpm_stclear_flags));
        /* initialize the TPM_STANY_FLAGS structure */
        printf("TPM_Global_Init: Initializing TPM_STANY_FLAGS\n");
        TPM_StanyFlags_Init(&(tpm_state->tpm_stany_flags));
        /* initialize TPM_PERMANENT_DATA structure */
        printf("TPM_Global_Init: Initializing TPM_PERMANENT_DATA\n");
        rc = TPM_PermanentData_Init(&(tpm_state->tpm_permanent_data), TRUE);
    }
    if (rc == 0) {
	/* initialize TPM_STCLEAR_DATA structure */
        printf("TPM_Global_Init: Initializing TPM_STCLEAR_DATA\n");
        TPM_StclearData_Init(&(tpm_state->tpm_stclear_data),
                             tpm_state->tpm_permanent_data.pcrAttrib,
                             TRUE);     /* initialize the PCR's */
	/* initialize TPM_STANY_DATA structure */
        printf("TPM_Global_Init: Initializing TPM_STANY_DATA\n");
        rc = TPM_StanyData_Init(&(tpm_state->tpm_stany_data));
    }
    /* initialize the TPM_KEY_HANDLE_LIST structure */
    if (rc == 0) {
        printf("TPM_Global_Init: Initializing TPM_KEY_HANDLE_LIST\n");
        TPM_KeyHandleEntries_Init(tpm_state->tpm_key_handle_entries);
	/* initialize the SHA1 thread context */
	tpm_state->sha1_context = NULL;
	/* initialize the TIS SHA1 thread context */
	tpm_state->sha1_context_tis = NULL;
	tpm_state->transportHandle = 0;
        printf("TPM_Global_Init: Initializing TPM_NV_INDEX_ENTRIES\n");
	TPM_NVIndexEntries_Init(&(tpm_state->tpm_nv_index_entries));
    }
    /* comes up in limited operation mode */
    /* shutdown is set on a self test failure, before calling TPM_Global_Init() */
    if (rc == 0) {
	printf("  TPM_Global_Init: Set testState to %u \n", TPM_TEST_STATE_LIMITED);
	tpm_state->testState = TPM_TEST_STATE_LIMITED;
    }
    else {
	printf("  TPM_Global_Init: Set testState to %u \n", TPM_TEST_STATE_FAILURE);
	tpm_state->testState = TPM_TEST_STATE_FAILURE;
    }
    return rc;
}

#if 0
/* TPM_Global_Load() loads the tpm_state_t global structures for the TPM instance from NVRAM.

   tpm_state->tpm_number must be set by the caller.
   
   Returns

   0 on success.
   TPM_FAIL on failure to load (fatal), since it should never occur
*/

TPM_RESULT TPM_Global_Load(tpm_state_t *tpm_state)
{
    TPM_RESULT rc = 0;

    printf("TPM_Global_Load:\n");
    /* TPM_PERMANENT_DATA, TPM_PERMANENT_FLAGS, owner evict keys, and NV defined space. */
    if (rc == 0) {
	rc = TPM_PermanentAll_NVLoad(tpm_state);
    }
    if (rc == 0) {
	rc = TPM_VolatileAll_NVLoad(tpm_state);
    }
    return rc;
}

/* TPM_Global_Store() store the tpm_state_t global structure for the TPM instance to NVRAM

   tpm_state->tpm_number must be set by the caller.
*/

TPM_RESULT TPM_Global_Store(tpm_state_t *tpm_state)
{
    TPM_RESULT rc = 0;

    printf(" TPM_Global_Store:\n");
    if (rc == 0) {
	rc = TPM_PermanentAll_NVStore(tpm_state, TRUE, 0);
    }
    if (rc == 0) {
	rc = TPM_VolatileAll_NVStore(tpm_state);
    }
    return rc;
}
#endif

/* TPM_Global_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_Global_Init to set members back to default values
   The object itself is not freed
*/

void TPM_Global_Delete(tpm_state_t *tpm_state)
{
    printf(" TPM_Global_Delete:\n");
    if (tpm_state != NULL) {
	/* TPM_PERMANENT_FLAGS have no allocated memory or secrets */
	/* TPM_STCLEAR_FLAGS have no allocated memory or secrets */
	/* TPM_STANY_FLAGS have no allocated memory or secrets */
	printf("  TPM_Global_Delete: Deleting TPM_PERMANENT_DATA\n");
	TPM_PermanentData_Delete(&(tpm_state->tpm_permanent_data), TRUE);
	printf("  TPM_Global_Delete: Deleting TPM_STCLEAR_DATA\n");
	TPM_StclearData_Delete(&(tpm_state->tpm_stclear_data),
			       tpm_state->tpm_permanent_data.pcrAttrib,
			       TRUE);       /* reset the PCR's */
	printf("  TPM_Global_Delete: Deleting TPM_STANY_DATA\n");
	TPM_StanyData_Delete(&(tpm_state->tpm_stany_data));
	printf("  TPM_Global_Delete: Deleting key handle entries\n");
	TPM_KeyHandleEntries_Delete(tpm_state->tpm_key_handle_entries);
	printf("  TPM_Global_Delete: Deleting SHA1 contexts\n");
	TPM_SHA1Delete(&(tpm_state->sha1_context));
	TPM_SHA1Delete(&(tpm_state->sha1_context_tis));
	TPM_NVIndexEntries_Delete(&(tpm_state->tpm_nv_index_entries));
    }
    return;
}


/* TPM_Global_GetPhysicalPresence() returns 'physicalPresence' TRUE if either TPM_STCLEAR_FLAGS ->
   physicalPresence is TRUE or hardware physical presence is indicated.

   The physicalPresenceHWEnable and physicalPresenceCMDEnable flags MUST mask their respective
   signals before further processing. The hardware signal, if enabled by the
   physicalPresenceHWEnable flag, MUST be logically ORed with the PhysicalPresence flag, if enabled,
   to obtain the final physical presence value used to allow or disallow local commands.
*/

TPM_RESULT TPM_Global_GetPhysicalPresence(TPM_BOOL *physicalPresence,
                                          const tpm_state_t *tpm_state)
{
    TPM_RESULT  rc = 0;
    *physicalPresence = FALSE;

    /* is CMD physical presence enabled */
    printf("  TPM_Global_GetPhysicalPresence: physicalPresenceCMDEnable is %02x\n",
	   tpm_state->tpm_permanent_flags.physicalPresenceCMDEnable);
    if (tpm_state->tpm_permanent_flags.physicalPresenceCMDEnable) {
	printf("  TPM_Global_GetPhysicalPresence: physicalPresence flag is %02x\n",
	       tpm_state->tpm_stclear_flags.physicalPresence);
	/* if enabled, check for physicalPresence set by the command ordinal */
	*physicalPresence = tpm_state->tpm_stclear_flags.physicalPresence;
    }

    /* if the software flag is true, result is true, no need to check the hardware */
    /* if the TPM_STCLEAR_FLAGS flag is FALSE, check the hardware */
    if (!(*physicalPresence)) {
	printf("  TPM_Global_GetPhysicalPresence: physicalPresenceHWEnable is %02x\n",
	       tpm_state->tpm_permanent_flags.physicalPresenceHWEnable);
        /* if physicalPresenceHWEnable is FALSE, the hardware signal is disabled */
        if (tpm_state->tpm_permanent_flags.physicalPresenceHWEnable) {
            /* if enabled, check the hardware signal */
            rc = TPM_IO_GetPhysicalPresence(physicalPresence, tpm_state->tpm_number);
	    printf("  TPM_Global_GetPhysicalPresence: physicalPresence HW is %02x\n",
		   *physicalPresence);
        }
    }
    printf("  TPM_Global_GetPhysicalPresence: physicalPresence is %02x\n",
	   *physicalPresence);
    return rc;
}

