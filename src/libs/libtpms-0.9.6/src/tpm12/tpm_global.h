/********************************************************************************/
/*                                                                              */
/*                           Global Variables                                   */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_global.h $            */
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

#ifndef TPM_GLOBAL_H
#define TPM_GLOBAL_H

#include "tpm_nvram_const.h"
#include "tpm_types.h"
#include "tpm_structures.h"

#define TPM_TEST_STATE_LIMITED  1       /* limited operation mode */
#define TPM_TEST_STATE_FULL     2       /* full operation mode */
#define TPM_TEST_STATE_FAILURE  3       /* failure mode */

typedef struct tdTPM_STATE
{
    /* the number of the virtual TPM */
    uint32_t tpm_number;
    /* 7.1 TPM_PERMANENT_FLAGS */
    TPM_PERMANENT_FLAGS tpm_permanent_flags; 
    /* 7.2 TPM_STCLEAR_FLAGS */
    TPM_STCLEAR_FLAGS tpm_stclear_flags;
    /* 7.3 TPM_STANY_FLAGS  */
    TPM_STANY_FLAGS tpm_stany_flags;
    /* 7.4 TPM_PERMANENT_DATA */
    TPM_PERMANENT_DATA tpm_permanent_data;
    /* 7.5 TPM_STCLEAR_DATA  */
    TPM_STCLEAR_DATA tpm_stclear_data;
    /* 7.6 TPM_STANY_DATA  */
    TPM_STANY_DATA tpm_stany_data;
    /* 5.6 TPM_KEY_HANDLE_ENTRY */
    TPM_KEY_HANDLE_ENTRY tpm_key_handle_entries[TPM_KEY_HANDLES];
    /* Context for SHA1 functions */
    void *sha1_context;
    void *sha1_context_tis;
    TPM_TRANSHANDLE transportHandle;    /* non-zero if the context was set up in a transport
                                           session */
    /* self test shutdown */
    uint32_t testState;
    /* NVRAM volatile data marker.  Cleared at TPM_Startup(ST_Clear), it holds all indexes which
       have been read.  The index not being present indicates that some volatile fields should be
       cleared at first read. */
    TPM_NV_INDEX_ENTRIES tpm_nv_index_entries;
    /* NOTE: members added here should be initialized by TPM_Global_Init() and possibly added to
       TPM_SaveState_Load() and TPM_SaveState_Store() */
} tpm_state_t;

/* state for the TPM */
extern tpm_state_t *tpm_instances[];


/*
  tpm_state_t
*/

TPM_RESULT TPM_Global_Init(tpm_state_t *tpm_state);
#if 0
TPM_RESULT TPM_Global_Load(tpm_state_t *tpm_state);
TPM_RESULT TPM_Global_Store(tpm_state_t *tpm_state);
#endif
void       TPM_Global_Delete(tpm_state_t *tpm_state);


TPM_RESULT TPM_Global_GetPhysicalPresence(TPM_BOOL *physicalPresence,
                                          const tpm_state_t *tpm_state);

#endif
