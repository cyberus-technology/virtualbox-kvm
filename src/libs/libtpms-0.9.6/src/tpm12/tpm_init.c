/********************************************************************************/
/*                                                                              */
/*                         TPM Initialization                                   */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_init.c $		*/
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "tpm_admin.h"
#include "tpm_cryptoh.h"
#include "tpm_crypto.h"
#include "tpm_daa.h"
#include "tpm_debug.h"
#include "tpm_digest.h"
#include "tpm_error.h"
#include "tpm_io.h"
#include "tpm_memory.h"
#include "tpm_nonce.h"
#include "tpm_nvfile.h"
#include "tpm_pcr.h"
#include "tpm_process.h"
#include "tpm_permanent.h"
#include "tpm_platform.h"
#include "tpm_session.h"
#include "tpm_startup.h"
#include "tpm_structures.h"
#include "tpm_ticks.h"
#include "tpm_transport.h"


#include "tpm_init.h"

/* local prototypes */

static TPM_RESULT TPM_CheckTypes(void);


/* TPM_Init transitions the TPM from a power-off state to one where the TPM begins an initialization
   process.  TPM_Init could be the result of power being applied to the platform or a hard reset.
   TPM_Init sets an internal flag to indicate that the TPM is undergoing initialization. The TPM
   must complete initialization before it is operational. The completion of initialization requires
   the receipt of the TPM_Startup command.

   This is different from the debug function TPM_Process_Init(), which initializes a TPM.

   The call tree for initialization is as follows:

   main()
        TPM_MainInit()
                TPM_IO_Init() - initializes the TPM I/O interface
                TPM_Crypto_Init() - initializes cryptographic libraries
                TPM_NVRAM_Init() - get NVRAM path once
                TPM_LimitedSelfTest() - as per the specification
                TPM_Global_Init() - initializes the TPM state

   Returns: 0 on success

            non-zero on a fatal error where the TPM should not continue.  These are fatal errors
            where the TPM just exits.  It can't even enter shutdown.

   A self test error may cause one or all TPM's to enter shutdown, but is not fatal.
*/

TPM_RESULT TPM_MainInit(void)
{
    TPM_RESULT  rc = 0;         /* results for common code, fatal errors */
    uint32_t    i;
    TPM_RESULT  testRc = 0;     /* temporary place to hold common self tests failure before the tpm
                                   state is created */
    tpm_state_t *tpm_state;     /* TPM instance state */
    bool        has_cached_state = false;

    tpm_state = NULL;           /* freed @1 */
    /* preliminary check that platform specific sizes are correct */
    if (rc == 0) {
        rc = TPM_CheckTypes();
    }
    /* initialize the TPM to host interface */
    if (rc == 0) {
        printf("TPM_MainInit: Initialize the TPM to host interface\n");
        rc = TPM_IO_Init();
    }
    /* initialize cryptographic functions */
    if (rc == 0) {
        printf("TPM_MainInit: Initialize the TPM crypto support\n");
        rc = TPM_Crypto_Init();
    }
    /* initialize NVRAM static variables.  This must be called before the global TPM state is
       loaded */
    if (rc == 0) {
        printf("TPM_MainInit: Initialize the TPM NVRAM\n");
        rc = TPM_NVRAM_Init();
    }
    /* run the initial subset of self tests once */
    if (rc == 0) {
        printf("TPM_MainInit: Run common limited self tests\n");
        /* an error is a fatal error, causes a shutdown of the TPM */
        testRc = TPM_LimitedSelfTestCommon();
    }   
    /* initialize the global structure for the TPM */
    for (i = 0 ; (rc == 0) && (i < TPMS_MAX) ; i++) {
        printf("TPM_MainInit: Initializing global TPM %lu\n", (unsigned long)i);
        /* Need to malloc and init a TPM state if this is the first time through or if the
           state was saved in the array.  Otherwise, the malloc'ed structure from the previous
           time through the loop can be reused. */
        if ((rc == 0) && (tpm_state == NULL)) {
            if (rc == 0) {
                rc = TPM_Malloc((unsigned char **)&tpm_state, sizeof(tpm_state_t));
            }
            /* initialize the global instance state */
            if (rc == 0) {
                rc = TPM_Global_Init(tpm_state);                /* freed @2 */
            }
        }
        if (rc == 0) {
            has_cached_state = HasCachedState(TPMLIB_STATE_PERMANENT);

            /* record the TPM number in the state */
            tpm_state->tpm_number = i;
            /* If the instance exists in NVRAM, it it initialized and saved in the tpm_instances[]
               array. Restores TPM_PERMANENT_FLAGS and TPM_PERMANENT_DATA to in-memory
               structures. */
            /* Returns TPM_RETRY on non-existent file */
            rc = TPM_PermanentAll_NVLoad(tpm_state);
        }
        /* If there was no state for TPM 0 (instance 0 does not exist), initialize state for the
           first time using TPM_Global_Init() above.  It is created and set to default values.  */
        if ((rc == TPM_RETRY) && (i == 0)) {
            /* save the state for TPM 0 (first time through) */
            rc = TPM_PermanentAll_NVStore(tpm_state,
					  TRUE,		/* write NV */
					  0);		/* no roll back */
        }
#ifdef TPM_VOLATILE_LOAD
	/* if volatile state exists at startup, load it.  This is used for fail-over restart. */
	if (rc == 0) {
	    rc = TPM_VolatileAll_NVLoad(tpm_state);
	}
#endif	/* TPM_VOLATILE_LOAD */
        /* libtpms: due to the SetState() API we have to write the permanent state back to
           a file now */
        if (rc == 0 && has_cached_state) {
            rc = TPM_PermanentAll_NVStore(tpm_state,
                                          TRUE,		/* write NV */
                                          0);		/* no roll back */
        }
        /* if permanent state was loaded successfully (or stored successfully for TPM 0 the first
           time) */
        if (rc == 0) {
            printf("TPM_MainInit: Creating global TPM instance %lu\n", (unsigned long)i);
            /* set the testState for the TPM based on the common selftest result */
            if (testRc != 0) {
                /* a. When the TPM detects a failure during any self-test, it SHOULD delete values
                   preserved by TPM_SaveState. */
                TPM_SaveState_NVDelete(tpm_state,
                                       FALSE);        /* ignore error if the state does not exist */
		printf("  TPM_MainInit: Set testState to %u \n", TPM_TEST_STATE_FAILURE);
                tpm_state->testState = TPM_TEST_STATE_FAILURE;
            }
            /* save state in array */
            tpm_instances[i] = tpm_state;
            tpm_state = NULL;   /* flag that the malloc'ed structure was used.  It should not be
                                   freed, and a new instance is needed the next time through the
                                   loop */
        }
        /* If there was the non-fatal error TPM_RETRY, the instance does not exist.  If instance > 0
           does not exist, the array entry is set to NULL.  Continue */
        else if (rc == TPM_RETRY) {
            printf("TPM_MainInit: Not Creating global TPM %lu\n", (unsigned long)i);
            tpm_instances[i] = NULL;    /* flag that the instance does not exist */
            rc = 0;                     /* Instance does not exist, not fatal error */
        }
    }
   /* run individual self test on a TPM */
    for (i = 0 ;
         (rc == 0) && (i < TPMS_MAX) && (tpm_instances[i] != NULL) &&
             (tpm_instances[i]->testState != TPM_TEST_STATE_FAILURE) ;  /* don't continue if already
                                                                           error */
         i++) {
        printf("TPM_MainInit: Run limited self tests on TPM %lu\n", (unsigned long)i);
        testRc = TPM_LimitedSelfTestTPM(tpm_instances[i]);
        if (testRc != 0) {
            /* a. When the TPM detects a failure during any self-test, it SHOULD delete values
               preserved by TPM_SaveState. */
            TPM_SaveState_NVDelete(tpm_state,
                                   FALSE);        /* ignore error if the state does not exist */
        }
    }
    /* the _Delete(), free() clean up if the last created instance was not required */
    TPM_Global_Delete(tpm_state); 	/* @2 */
    free(tpm_state);                    /* @1 */
    return rc;
}

/* TPM_CheckTypes() checks that the assumed TPM types are correct for the platform
 */

static TPM_RESULT TPM_CheckTypes(void)
{
    TPM_RESULT  rc = 0;         /* fatal errors */

    /* These should be removed at compile time */
    if (rc == 0) {
        if (sizeof(uint16_t) != 2) {
            printf("TPM_CheckTypes: Error (fatal), uint16_t size %lu not supported\n",
                   (unsigned long)sizeof(uint16_t));
            rc = TPM_FAIL;
        }
    }    
    if (rc == 0) {
        if (sizeof(uint32_t) != 4) {
            printf("TPM_CheckTypes: Error (fatal), uint32_t size %lu not supported\n",
                   (unsigned long)sizeof(uint32_t));
            rc = TPM_FAIL;
        }
    }
    if (rc == 0) {
        if ((sizeof(time_t) != 4) &&    /* for 32-bit machines */
            (sizeof(time_t) != 8)) {    /* for 64-bit machines */
            printf("TPM_CheckTypes: Error (fatal), time_t size %lu not supported\n",
                   (unsigned long)sizeof(time_t));
            rc = TPM_FAIL;
        }
    }
    return rc;
}

/*
  TPM_STANY_FLAGS
*/

/* TPM_StanyFlags_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_StanyFlags_Init(TPM_STANY_FLAGS *tpm_stany_flags)
{
    printf(" TPM_StanyFlags_Init:\n");
    tpm_stany_flags->postInitialise = TRUE;
    tpm_stany_flags->localityModifier = 0;
    tpm_stany_flags->transportExclusive = 0;
    tpm_stany_flags->TOSPresent = FALSE;
    /* NOTE added */
    tpm_stany_flags->stateSaved = FALSE;
    return;
}

/* TPM_StanyFlags_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_StanyFlags_Init()
*/

TPM_RESULT TPM_StanyFlags_Load(TPM_STANY_FLAGS *tpm_stany_flags,
			       unsigned char **stream,
			       uint32_t *stream_size)
{
    TPM_RESULT          rc = 0;

    printf(" TPM_StanyFlags_Load:\n");
    /* check tag */
    if (rc == 0) {
        rc = TPM_CheckTag(TPM_TAG_STANY_FLAGS, stream, stream_size);
    }
    /* load postInitialise*/
    if (rc == 0) {
        rc = TPM_LoadBool(&(tpm_stany_flags->postInitialise), stream, stream_size); 
    }
    /* load localityModifier */
    if (rc == 0) {
        rc = TPM_Load32(&(tpm_stany_flags->localityModifier), stream, stream_size);
    }
    /* load transportExclusive */
    if (rc == 0) {
        rc = TPM_Load32(&(tpm_stany_flags->transportExclusive), stream, stream_size);
    }
    /* load TOSPresent */
    if (rc == 0) {
        rc = TPM_LoadBool(&(tpm_stany_flags->TOSPresent), stream, stream_size); 
    }
    /* load stateSaved */
    if (rc == 0) {
        rc = TPM_LoadBool(&(tpm_stany_flags->stateSaved), stream, stream_size); 
    }
    return rc;
}

/* TPM_StanyFlags_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_StanyFlags_Store(TPM_STORE_BUFFER *sbuffer,
				TPM_STANY_FLAGS *tpm_stany_flags)
{
    TPM_RESULT          rc = 0;

    printf(" TPM_StanyFlags_Store:\n");
    /* store tag */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_STANY_FLAGS);
    }
    /* store postInitialise*/
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, &(tpm_stany_flags->postInitialise), sizeof(TPM_BOOL));
    }
    /* store localityModifier */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append32(sbuffer, tpm_stany_flags->localityModifier);
    }
    /* store transportExclusive */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append32(sbuffer, tpm_stany_flags->transportExclusive);
    }
    /* store TOSPresent */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, &(tpm_stany_flags->TOSPresent), sizeof(TPM_BOOL));
    }
    /* store stateSaved */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, &(tpm_stany_flags->stateSaved), sizeof(TPM_BOOL));
    }
    return rc;
}

/*
  TPM_STCLEAR_FLAGS
*/

/* TPM_StclearFlags_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_StclearFlags_Init(TPM_STCLEAR_FLAGS *tpm_stclear_flags)
{
    printf(" TPM_StclearFlags_Init:\n");
    /* tpm_stclear_flags->deactivated; no default state */
    tpm_stclear_flags->disableForceClear = FALSE;
    tpm_stclear_flags->physicalPresence = FALSE;
    tpm_stclear_flags->physicalPresenceLock = FALSE;
    tpm_stclear_flags->bGlobalLock = FALSE;
    return;
}

/* TPM_StclearFlags_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_StclearFlags_Init()
*/

TPM_RESULT TPM_StclearFlags_Load(TPM_STCLEAR_FLAGS *tpm_stclear_flags,
                                 unsigned char **stream,
                                 uint32_t *stream_size)
{
    TPM_RESULT          rc = 0;

    printf(" TPM_StclearFlags_Load:\n");
    /* check tag */
    if (rc == 0) {
        rc = TPM_CheckTag(TPM_TAG_STCLEAR_FLAGS, stream, stream_size);
    }
    /* load deactivated */
    if (rc == 0) {
        rc = TPM_LoadBool(&(tpm_stclear_flags->deactivated), stream, stream_size); 
    }
    /* load disableForceClear */
    if (rc == 0) {
        rc = TPM_LoadBool(&(tpm_stclear_flags->disableForceClear), stream, stream_size);
    }
    /* load physicalPresence */
    if (rc == 0) {
        rc = TPM_LoadBool(&(tpm_stclear_flags->physicalPresence), stream, stream_size);
    }
    /* load physicalPresenceLock */
    if (rc == 0) {
        rc = TPM_LoadBool(&(tpm_stclear_flags->physicalPresenceLock), stream, stream_size);
    }
    /* load bGlobalLock */
    if (rc == 0) {
        rc = TPM_LoadBool(&(tpm_stclear_flags->bGlobalLock), stream, stream_size);
    }
    return rc;
}

/* TPM_StclearFlags_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_StclearFlags_Store(TPM_STORE_BUFFER *sbuffer,
                                  const TPM_STCLEAR_FLAGS *tpm_stclear_flags)
{
    TPM_RESULT          rc = 0;
    
    printf(" TPM_StclearFlags_Store:\n");
    /* store tag */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_STCLEAR_FLAGS);
    }
    /* store deactivated */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, &(tpm_stclear_flags->deactivated),
				sizeof(TPM_BOOL));
    }
    /* store disableForceClear */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, &(tpm_stclear_flags->disableForceClear),
				sizeof(TPM_BOOL));
    }
    /* store physicalPresence */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, &(tpm_stclear_flags->physicalPresence),
				sizeof(TPM_BOOL));
    }
    /* store physicalPresenceLock */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, &(tpm_stclear_flags->physicalPresenceLock),
				sizeof(TPM_BOOL));
    }
    /* store bGlobalLock */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, &(tpm_stclear_flags->bGlobalLock),
				sizeof(TPM_BOOL));
    }
    return rc;
}

/* TPM_StclearFlags_StoreBitmap() serializes TPM_STCLEAR_FLAGS structure into a bit map

 */

TPM_RESULT TPM_StclearFlags_StoreBitmap(uint32_t *tpm_bitmap,
                                        const TPM_STCLEAR_FLAGS *tpm_stclear_flags)
{
    TPM_RESULT  rc = 0;
    uint32_t	pos = 0;        /* position in bitmap */
    
    printf(" TPM_StclearFlags_StoreBitmap:\n");
    *tpm_bitmap = 0;
    /* store deactivated */
    if (rc == 0) {
        rc = TPM_Bitmap_Store(tpm_bitmap, tpm_stclear_flags->deactivated, &pos);
    }
    /* store disableForceClear */
    if (rc == 0) {
        rc = TPM_Bitmap_Store(tpm_bitmap, tpm_stclear_flags->disableForceClear, &pos);
    }
    /* store physicalPresence */
    if (rc == 0) {
        rc = TPM_Bitmap_Store(tpm_bitmap, tpm_stclear_flags->physicalPresence, &pos);
    }
    /* store physicalPresenceLock */
    if (rc == 0) {
        rc = TPM_Bitmap_Store(tpm_bitmap, tpm_stclear_flags->physicalPresenceLock, &pos);
    }
    /* store bGlobalLock */
    if (rc == 0) {
        rc = TPM_Bitmap_Store(tpm_bitmap, tpm_stclear_flags->bGlobalLock, &pos);
    }
    return rc;
}

/*
  TPM_STANY_DATA
*/

/* TPM_StanyData_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
*/

TPM_RESULT TPM_StanyData_Init(TPM_STANY_DATA *tpm_stany_data)
{
    TPM_RESULT rc = 0;

    printf(" TPM_StanyData_Init:\n");
    if (rc == 0) {
        /* The tpm_stany_data->currentTicks holds the time of day at initialization.  Both nonce
           generation and current time of day can return an error */
        TPM_CurrentTicks_Init(&(tpm_stany_data->currentTicks));
        rc = TPM_CurrentTicks_Start(&(tpm_stany_data->currentTicks));
    }
    return rc;
}

/* TPM_StanyData_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_StanyData_Init()
   After use, call TPM_StanyData_Delete() to free memory
*/

TPM_RESULT TPM_StanyData_Load(TPM_STANY_DATA *tpm_stany_data,
			      unsigned char **stream,
			      uint32_t *stream_size)
{
    TPM_RESULT          rc = 0;

    printf(" TPM_StanyData_Load:\n");
    tpm_stany_data = tpm_stany_data;
    /* check tag */
    if (rc == 0) {
        rc = TPM_CheckTag(TPM_TAG_STANY_DATA, stream, stream_size);
    }
    /* load currentTicks */
    if (rc == 0) {
	rc = TPM_CurrentTicks_LoadAll(&(tpm_stany_data->currentTicks), stream, stream_size);
    }
    return rc;
}

/* TPM_StanyData_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_StanyData_Store(TPM_STORE_BUFFER *sbuffer,
			       TPM_STANY_DATA *tpm_stany_data)
{
    TPM_RESULT          rc = 0;

    printf(" TPM_StanyData_Store:\n");
    tpm_stany_data = tpm_stany_data;
    /* store tag */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_STANY_DATA);
    }
    /* store currentTicks*/
    if (rc == 0) {
	rc = TPM_CurrentTicks_StoreAll(sbuffer, &(tpm_stany_data->currentTicks));
    }
    return rc;
}

/* TPM_StanyData_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   set members back to default values
   The object itself is not freed
*/   

void TPM_StanyData_Delete(TPM_STANY_DATA *tpm_stany_data)
{
    printf(" TPM_StanyData_Delete:\n");
    /* nothing to free */
    tpm_stany_data = tpm_stany_data;
    return;
}

/*
  TPM_STCLEAR_DATA
*/

/* TPM_StclearData_Init()

   If pcrInit is TRUE, resets the PCR's

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_StclearData_Init(TPM_STCLEAR_DATA *tpm_stclear_data,
                          TPM_PCR_ATTRIBUTES *pcrAttrib,
                          TPM_BOOL pcrInit)
{
    printf(" TPM_StclearData_Init:\n");
    TPM_Nonce_Init(tpm_stclear_data->contextNonceKey);
    tpm_stclear_data->countID = TPM_COUNT_ID_NULL;      /* NULL value - unselected counter */
    tpm_stclear_data->ownerReference = TPM_KH_OWNER;
    tpm_stclear_data->disableResetLock = FALSE;
    /* initialize PCR's */
    if (pcrInit) {
        printf("TPM_StclearData_Init: Initializing PCR's\n");
        TPM_PCRs_Init(tpm_stclear_data->PCRS, pcrAttrib);
    }
#if  (TPM_REVISION >= 103)      /* added for rev 103 */
    tpm_stclear_data->deferredPhysicalPresence = 0;
#endif
    tpm_stclear_data->authFailCount = 0;
    tpm_stclear_data->authFailTime = 0;
    /* initialize authorization, transport, DAA sessions, and saved sessions */
    TPM_StclearData_SessionInit(tpm_stclear_data);
    TPM_Digest_Init(tpm_stclear_data->auditDigest);
    TPM_Sbuffer_Init(&(tpm_stclear_data->ordinalResponse));
    return;
}

/* TPM_StclearData_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_StclearData_Init()
   After use, call TPM_StclearData_Delete() to free memory
*/

TPM_RESULT TPM_StclearData_Load(TPM_STCLEAR_DATA *tpm_stclear_data,
                                unsigned char **stream,
                                uint32_t *stream_size,
                                TPM_PCR_ATTRIBUTES *pcrAttrib)
{
    TPM_RESULT          rc = 0;
    TPM_STRUCTURE_TAG   tag = 0;

    printf(" TPM_StclearData_Load:\n");
    /* get tag */
    if (rc == 0) {      
        rc = TPM_Load16(&tag, stream, stream_size);
    }
    /* check tag */
    if (rc == 0) {
	printf("  TPM_StclearData_Load: stream version %04hx\n", tag);
	switch (tag) {
	  case TPM_TAG_STCLEAR_DATA:
	  case TPM_TAG_STCLEAR_DATA_V2:
	    break;
	  default:
            printf("TPM_StclearData_Load: Error (fatal), version %04x unsupported\n", tag);
            rc = TPM_FAIL;
	    break;
	}
    }
    /* load contextNonceKey */
    if (rc == 0) {
        rc = TPM_Nonce_Load(tpm_stclear_data->contextNonceKey, stream, stream_size);
    }
    /* load countID */
    if (rc == 0) {
        rc = TPM_Load32(&(tpm_stclear_data->countID), stream, stream_size);
    }
    /* load ownerReference */
    if (rc == 0) {
        rc = TPM_Load32(&(tpm_stclear_data->ownerReference), stream, stream_size);
    }
    /* load disableResetLock */
    if (rc == 0) {
        rc = TPM_LoadBool(&(tpm_stclear_data->disableResetLock), stream, stream_size);
    }
    /* load PCR's */
    if (rc == 0) {
        rc = TPM_PCRs_Load(tpm_stclear_data->PCRS, pcrAttrib, stream, stream_size);
    }
#if  (TPM_REVISION >= 103)      /* added for rev 103 */
    /* load deferredPhysicalPresence */
    if (rc == 0) {
        rc = TPM_Load32(&(tpm_stclear_data->deferredPhysicalPresence), stream, stream_size);
    }
#endif
    /* load authFailCount */
    if (rc == 0) {
        rc = TPM_Load32(&(tpm_stclear_data->authFailCount), stream, stream_size);
    }
    /* load authFailTime */
    if (rc == 0) {
        rc = TPM_Load32(&(tpm_stclear_data->authFailTime), stream, stream_size);
    }
    /* load authorization sessions */
    if (rc == 0) {
        rc = TPM_AuthSessions_Load(tpm_stclear_data->authSessions, stream, stream_size); 
    }
    /* load transport sessions */
    if (rc == 0) {
        rc = TPM_TransportSessions_Load(tpm_stclear_data->transSessions, stream, stream_size); 
    }
    /* load DAA sessions */
    if (rc == 0) {
        rc = TPM_DaaSessions_Load(tpm_stclear_data->daaSessions, stream, stream_size); 
    }
    /* load contextNonceSession */
    if (rc == 0) {
        rc = TPM_Nonce_Load(tpm_stclear_data->contextNonceSession, stream, stream_size);
    }
    /* load contextCount */
    if (rc == 0) {
        rc = TPM_Load32(&(tpm_stclear_data->contextCount), stream, stream_size);
    }
    /* load contextList */
    if (rc == 0) {
        rc = TPM_ContextList_Load(tpm_stclear_data->contextList, stream, stream_size);
    }
    /* load auditDigest */
    if (rc == 0) {
        rc = TPM_Digest_Load(tpm_stclear_data->auditDigest, stream, stream_size);
        TPM_PrintFour("  TPM_StclearData_Load: auditDigest", tpm_stclear_data->auditDigest);
    }
    /* no need to store and load ordinalResponse */
    if (tag == TPM_TAG_STCLEAR_DATA) {
	/* but it's there for some versions */
	if (rc == 0) {
	    TPM_STORE_BUFFER ordinalResponse;
	    TPM_Sbuffer_Init(&ordinalResponse);
	    rc = TPM_Sbuffer_Load(&ordinalResponse, stream, stream_size);
	    TPM_Sbuffer_Delete(&ordinalResponse);
	}
	if (rc == 0) {
	    uint32_t responseCount;
	    rc = TPM_Load32(&responseCount, stream, stream_size);
	}
    }
    return rc;
}

/* TPM_StclearData_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_StclearData_Store(TPM_STORE_BUFFER *sbuffer,
                                 TPM_STCLEAR_DATA *tpm_stclear_data,
                                 TPM_PCR_ATTRIBUTES *pcrAttrib)
{
    TPM_RESULT          rc = 0;

    printf(" TPM_StclearData_Store:\n");
    /* store tag */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_STCLEAR_DATA_V2);
    }
    /* store contextNonceKey */
    if (rc == 0) {
        rc = TPM_Nonce_Store(sbuffer, tpm_stclear_data->contextNonceKey);
    }
    /* store countID */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append32(sbuffer, tpm_stclear_data->countID);
    }
    /* store ownerReference */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append32(sbuffer, tpm_stclear_data->ownerReference);
    }
    /* store disableResetLock */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, &(tpm_stclear_data->disableResetLock),
                                sizeof(TPM_BOOL));
    }
    /* store PCR's */
    if (rc == 0) {
        rc = TPM_PCRs_Store(sbuffer, tpm_stclear_data->PCRS, pcrAttrib);
    }
#if  (TPM_REVISION >= 103)      /* added for rev 103 */
    /* store deferredPhysicalPresence */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append32(sbuffer, tpm_stclear_data->deferredPhysicalPresence);
    }
#endif
    /* store authFailCount */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append32(sbuffer, tpm_stclear_data->authFailCount);
    }
    /* store authFailTime */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append32(sbuffer, tpm_stclear_data->authFailTime);
    }
    /* store authorization sessions */
    if (rc == 0) {
        rc = TPM_AuthSessions_Store(sbuffer, tpm_stclear_data->authSessions);
    }
    /* store transport sessions */
    if (rc == 0) {
        rc = TPM_TransportSessions_Store(sbuffer, tpm_stclear_data->transSessions);
    }
    /* store DAA sessions */
    if (rc == 0) {
        rc = TPM_DaaSessions_Store(sbuffer, tpm_stclear_data->daaSessions);
    }
    /* store contextNonceSession */
    if (rc == 0) {
        rc = TPM_Nonce_Store(sbuffer, tpm_stclear_data->contextNonceSession);
    }
    /* store contextCount */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append32(sbuffer, tpm_stclear_data->contextCount);
    }
    /* store contextList */
    if (rc == 0) {
        rc = TPM_ContextList_Store(sbuffer, tpm_stclear_data->contextList);
    }
    /* store auditDigest */
    if (rc == 0) {
        TPM_PrintFour("  TPM_StclearData_Store: auditDigest", tpm_stclear_data->auditDigest);
        rc = TPM_Digest_Store(sbuffer, tpm_stclear_data->auditDigest);
    }
    /* no need to store and load ordinalResponse */
    return rc;
}

/* TPM_StclearData_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_StclearData_Init to set members back to default values
   The object itself is not freed
*/   

/* TPM_StclearData_Delete() frees any memory associated with TPM_STCLEAR_DATA, and then
   reinitializes the structure.

   If pcrInit is TRUE, the PCR's are initialized.
*/

void TPM_StclearData_Delete(TPM_STCLEAR_DATA *tpm_stclear_data,
                            TPM_PCR_ATTRIBUTES *pcrAttrib,
                            TPM_BOOL pcrInit)
{
    printf(" TPM_StclearData_Delete:\n");
    if (tpm_stclear_data != NULL) {
        TPM_StclearData_SessionDelete(tpm_stclear_data);/* authorization, transport, and DAA
                                                           sessions */
        TPM_Sbuffer_Delete(&(tpm_stclear_data->ordinalResponse));
        TPM_StclearData_Init(tpm_stclear_data, pcrAttrib, pcrInit);
    }
    return;
}

/* TPM_StclearData_SessionInit() initializes the structure members associated with authorization,
   transport, and DAA sessions.

   It must be called whenever the sessions are invalidated.
*/

void TPM_StclearData_SessionInit(TPM_STCLEAR_DATA *tpm_stclear_data)
{
    printf(" TPM_StclearData_SessionInit:\n");
    /* active sessions */
    TPM_AuthSessions_Init(tpm_stclear_data->authSessions);
    TPM_TransportSessions_Init(tpm_stclear_data->transSessions);
    TPM_DaaSessions_Init(tpm_stclear_data->daaSessions);
    /* saved sessions */
    TPM_Nonce_Init(tpm_stclear_data->contextNonceSession);
    tpm_stclear_data->contextCount = 0;
    TPM_ContextList_Init(tpm_stclear_data->contextList);
    return;
}

/* TPM_StclearData_SessionDelete() deletes the structure members associated with authorization,
   transport, and DAA sessions.

   It must be called whenever the sessions are invalidated.
*/

void TPM_StclearData_SessionDelete(TPM_STCLEAR_DATA *tpm_stclear_data)
{
    printf(" TPM_StclearData_SessionDelete:\n");
    /* active and saved authorization sessions, the authSessions table and the 3 contextList
       entries */
    TPM_StclearData_AuthSessionDelete(tpm_stclear_data);
    /* loaded transport sessions */
    TPM_TransportSessions_Delete(tpm_stclear_data->transSessions);
    /* loaded DAA sessions */
    TPM_DaaSessions_Delete(tpm_stclear_data->daaSessions);
    return;
}

/* TPM_StclearData_AuthSessionDelete() deletes the structure members associated with authorization
   sessions.  It clears the authSessions table and the 3 contextList members.

   It must be called whenever the sessions are invalidated.
*/

void TPM_StclearData_AuthSessionDelete(TPM_STCLEAR_DATA *tpm_stclear_data)
{
    printf(" TPM_StclearData_AuthSessionDelete:\n");
    /* active sessions */
    TPM_AuthSessions_Delete(tpm_stclear_data->authSessions);
    /* saved sessions */
    TPM_Nonce_Init(tpm_stclear_data->contextNonceSession);
    tpm_stclear_data->contextCount = 0;
    TPM_ContextList_Init(tpm_stclear_data->contextList);
    return;
}

/*
  TPM_InitCmd() executes the actions of the TPM_Init 'ordinal'
*/

TPM_RESULT TPM_InitCmd(tpm_state_t *tpm_state)
{
    TPM_RESULT  rc = 0;
    uint32_t	tpm_number;
    
    printf(" TPM_Init:\n");
    /* Release all resources for the TPM and reinitialize */
    if (rc == TPM_SUCCESS) {
        tpm_number = tpm_state->tpm_number;     /* save the TPM value */
        TPM_Global_Delete(tpm_state);		/* delete all the state */
	rc = TPM_Global_Init(tpm_state);	/* re-allocate the state */
    }
    /* Reload non-volatile memory */
    if (rc == TPM_SUCCESS) {
        tpm_state->tpm_number = tpm_number;     /* restore the TPM number */
        /* Returns TPM_RETRY on non-existent file */
        rc = TPM_PermanentAll_NVLoad(tpm_state);	/* reload the state */
        if (rc == TPM_RETRY) {
            printf("TPM_Init: Error (fatal), non-existent instance\n");
            rc = TPM_FAIL;
        }
    }
    return rc;
}

/*
  TPM_Handle_GenerateHandle() is a utility function that returns an unused handle.

  It's really not an initialization function, but as the handle arrays are typically in
  TPM_STCLEAR_DATA, it's a reasonable home.

  If 'tpm_handle' is non-zero, it is the first value tried.  If 'keepHandle' is TRUE, it is the only
  value tried.

  If 'tpm_handle' is zero, a random value is assigned.  If 'keepHandle' is TRUE, an error returned,
  as zero is an illegal handle value.

  If 'isKeyHandle' is TRUE, special checking is performed to avoid reserved values.

  'getEntryFunction' is a function callback to check whether the handle has already been assigned to
  an entry in the appropriate handle list.
*/

TPM_RESULT TPM_Handle_GenerateHandle(TPM_HANDLE *tpm_handle,
                                     void *tpm_handle_entries,
                                     TPM_BOOL keepHandle,
                                     TPM_BOOL isKeyHandle,
                                     TPM_GETENTRY_FUNCTION_T getEntryFunction)
{
    TPM_RESULT                  rc = 0;
    TPM_RESULT                  getRc = 0;
    unsigned int                timeout;                /* collision timeout */
    void                        *used_handle_entry;     /* place holder for discarded entry */
    TPM_BOOL                    done;
    
    printf(" TPM_Handle_GenerateHandle: handle %08x, keepHandle %u\n",
           *tpm_handle, keepHandle);
    /* if the input value must be used */
    if (keepHandle) {
        /* 0 is illegal and cannot be kept */
        if (rc == 0) {
            if (*tpm_handle == 0) {
                printf("TPM_Handle_GenerateHandle: Error, cannot keep handle 0\n");
                rc = TPM_BAD_HANDLE;
            }
        }
        /* key handles beginning with 0x40 are reserved special values */
        if (rc == 0) {
            if (isKeyHandle) {
                if ((*tpm_handle & 0xff000000) == 0x40000000) {
                    printf("TPM_Handle_GenerateHandle: Error, cannot keep reserved key handle\n");
                    rc = TPM_BAD_HANDLE;
                }
            }
        }
        /* check if the handle is already used */
        if (rc == 0) {
            getRc = getEntryFunction(&used_handle_entry,        /* discarded entry */
                                     tpm_handle_entries,        /* handle array */
                                     *tpm_handle);              /* search for handle */
            /* success mean the handle has already been assigned */
            if (getRc == 0) {
                printf("TPM_Handle_GenerateHandle: Error handle already in use\n");
                rc = TPM_BAD_HANDLE;
            }
        }
    }
    /* input value is recommended but not required */
    else {
        /* implement a crude timeout in case the random number generator fails and there are too
           many collisions */
        for (done = FALSE, timeout = 0 ; (rc == 0) && !done && (timeout < 1000) ; timeout++) {
            /* If no handle has been assigned, try a random value.  If a handle has been assigned,
               try it first */
            if (rc == 0) {
                if (*tpm_handle == 0) {
                    rc = TPM_Random((BYTE *)tpm_handle, sizeof(uint32_t));
                }
            }
            /* if the random value is 0, reject it immediately */
            if (rc == 0) {
                if (*tpm_handle == 0) {
                    printf("  TPM_Handle_GenerateHandle: Random value 0 rejected\n");
                    continue;
                }
            }
            /* if the value is a reserved key handle, reject it immediately */
            if (rc == 0) {
                if (isKeyHandle) {
                    if ((*tpm_handle & 0xff000000) == 0x40000000) {
                        printf("  TPM_Handle_GenerateHandle: Random value %08x rejected\n",
                               *tpm_handle);
                        *tpm_handle = 0;                /* ignore the assigned value */
                        continue;
                    }
                }
            }
            /* test if the handle has already been used */
            if (rc == 0) {
                getRc = getEntryFunction(&used_handle_entry,    /* discarded entry */
                                         tpm_handle_entries,    /* handle array */
                                         *tpm_handle);          /* search for handle */
                if (getRc != 0) {               /* not found, done */
                    printf("  TPM_Handle_GenerateHandle: Assigned Handle %08x\n",
                           *tpm_handle);
                    done = TRUE;
                }
                else {                          /* found, try again */
                    *tpm_handle = 0;            /* ignore the assigned value */
                    printf("  TPM_Handle_GenerateHandle: Handle %08x already used\n",
                           *tpm_handle);
                }
            }
        }
        if (!done) {
            printf("TPM_Handle_GenerateHandle: Error (fatal), random number generator failed\n");
            rc = TPM_FAIL;      
        }
    }
    return rc;
}

/*
  Processing Functions
*/

/* TPM_Init

   This ordinal should not be implemented, since it allows software to imitate a reboot.  That would
   be a major security hole, since the PCR's are reset.

   It is only here for regression tests.
*/

TPM_RESULT TPM_Process_Init(tpm_state_t *tpm_state,
                            TPM_STORE_BUFFER *response,
                            TPM_TAG tag,
                            uint32_t paramSize,
                            TPM_COMMAND_CODE ordinal,
                            unsigned char *command,
                            TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT  rcf = 0;                        /* fatal error precluding response */
    TPM_RESULT  returnCode = TPM_SUCCESS;       /* command return code */
    
    printf("TPM_Process_Init: Ordinal Entry\n");
    ordinal = ordinal;                          /* not used */
    command = command;                          /* not used */
    transportInternal = transportInternal;      /* not used */
    /* check state */
    /* NOTE: Allow at any time. */
    /*
      get inputs
    */
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
        returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
        if (paramSize != 0) {
            printf("TPM_Process_Init: Error, command has %u extra bytes\n",
                   paramSize);
            returnCode = TPM_BAD_PARAM_SIZE;
        }
    }
    /*
      Processing
    */
    if (returnCode == TPM_SUCCESS) {
#ifdef TPM_TEST
        returnCode = TPM_InitCmd(tpm_state);
#else
        tpm_state = tpm_state;  /* to quiet the compiler */
        printf("TPM_Process_Init: Error, bad ordinal\n");
        returnCode = TPM_BAD_ORDINAL;
#endif
    }
    /*
      response
    */
    if (rcf == 0) {
        printf("TPM_Process_Init: Ordinal returnCode %08x %u\n",
               returnCode, returnCode);
        rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    return rcf;
}

