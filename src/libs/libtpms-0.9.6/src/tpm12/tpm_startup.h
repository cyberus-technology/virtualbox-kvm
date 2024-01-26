/********************************************************************************/
/*                                                                              */
/*                          TPM Admin Startup and State                         */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_startup.h $           */
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

#ifndef TPM_STARTUP_H
#define TPM_STARTUP_H

#include "tpm_global.h"
#include "tpm_store.h"
#include "tpm_types.h"

/*
  Startup
*/

TPM_RESULT TPM_Startup_Clear(tpm_state_t *tpm_state);
TPM_RESULT TPM_Startup_State(tpm_state_t *tpm_state);
TPM_RESULT TPM_Startup_Deactivated(tpm_state_t *tpm_state);
#if 0
TPM_RESULT TPM_Startup_Any(tpm_state_t *tpm_state);
#endif

/*
  Save State
*/

TPM_RESULT TPM_SaveState_Load(tpm_state_t *tpm_state,
                              unsigned char **stream,
                              uint32_t *stream_size);
TPM_RESULT TPM_SaveState_Store(TPM_STORE_BUFFER *sbuffer,
                               tpm_state_t *tpm_state);

TPM_RESULT TPM_SaveState_NVLoad(tpm_state_t *tpm_state);
TPM_RESULT TPM_SaveState_NVStore(tpm_state_t *tpm_state);
TPM_RESULT TPM_SaveState_NVDelete(tpm_state_t *tpm_state,
                                  TPM_BOOL mustExist);

void       TPM_SaveState_IsSaveKey(TPM_BOOL *save, 
                                   TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entry);

/*
  Volatile State
*/

TPM_RESULT TPM_VolatileAll_Load(tpm_state_t *tpm_state,
				unsigned char **stream,
				uint32_t *stream_size);
TPM_RESULT TPM_VolatileAll_Store(TPM_STORE_BUFFER *sbuffer,
				 tpm_state_t *tpm_state);
TPM_RESULT TPM_VolatileAll_NVLoad(tpm_state_t *tpm_state);
TPM_RESULT TPM_VolatileAll_NVStore(tpm_state_t *tpm_state);

/*
  Compiled in TPM Parameters
*/

TPM_RESULT TPM_Parameters_Load(unsigned char **stream,
			       uint32_t *stream_size);
TPM_RESULT TPM_Parameters_Store(TPM_STORE_BUFFER *sbuffer);
TPM_RESULT TPM_Parameters_Check8(uint8_t expected,
				 const char *parameter,
				 unsigned char **stream,
				 uint32_t *stream_size);
TPM_RESULT TPM_Parameters_Check16(uint16_t expected,
				  const char *parameter,
				  unsigned char **stream,
				  uint32_t *stream_size);
TPM_RESULT TPM_Parameters_Check32(uint32_t expected,
				  const char *parameter,
				  unsigned char **stream,
				  uint32_t *stream_size);

/*
  Processing Functions
*/

TPM_RESULT TPM_Process_Reset(tpm_state_t *tpm_state,
                             TPM_STORE_BUFFER *response,
                             TPM_TAG tag,
                             uint32_t paramSize,
                             TPM_COMMAND_CODE ordinal,
                             unsigned char *command,
                             TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_Startup(tpm_state_t *tpm_state,
                               TPM_STORE_BUFFER *response,
                               TPM_TAG tag,
                               uint32_t paramSize,
                               TPM_COMMAND_CODE ordinal,
                               unsigned char *command,
                               TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_SaveState(tpm_state_t *tpm_state,
                                 TPM_STORE_BUFFER *response,
                                 TPM_TAG tag,
                                 uint32_t paramSize,
                                 TPM_COMMAND_CODE ordinal,
                                 unsigned char *command,
                                 TPM_TRANSPORT_INTERNAL *transportInternal);

#endif
