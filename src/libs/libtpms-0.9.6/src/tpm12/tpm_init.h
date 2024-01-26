/********************************************************************************/
/*                                                                              */
/*                         TPM Initialization                                   */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_init.h $              */
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

#ifndef TPM_INIT_H
#define TPM_INIT_H

#include "tpm_global.h"
#include "tpm_store.h"
#include "tpm_structures.h"

/* Power up initialization */
TPM_RESULT TPM_MainInit(void);

/*
  TPM_STANY_FLAGS
*/

void       TPM_StanyFlags_Init(TPM_STANY_FLAGS *tpm_stany_flags);

TPM_RESULT TPM_StanyFlags_Load(TPM_STANY_FLAGS *tpm_stany_flags,
			       unsigned char **stream,
			       uint32_t *stream_size);
TPM_RESULT TPM_StanyFlags_Store(TPM_STORE_BUFFER *sbuffer,
				TPM_STANY_FLAGS *tpm_stany_flags);

/*
  TPM_STCLEAR_FLAGS
*/

void       TPM_StclearFlags_Init(TPM_STCLEAR_FLAGS *tpm_stclear_flags);
TPM_RESULT TPM_StclearFlags_Load(TPM_STCLEAR_FLAGS *tpm_stclear_flags,
                                 unsigned char **stream,
                                 uint32_t *stream_size);
TPM_RESULT TPM_StclearFlags_Store(TPM_STORE_BUFFER *sbuffer,
                                  const TPM_STCLEAR_FLAGS *tpm_stclear_flags);
TPM_RESULT TPM_StclearFlags_StoreBitmap(uint32_t *tpm_bitmap,
                                        const TPM_STCLEAR_FLAGS *tpm_stclear_flags);
/*
  TPM_STANY_DATA
*/

TPM_RESULT TPM_StanyData_Init(TPM_STANY_DATA *tpm_stany_data);
TPM_RESULT TPM_StanyData_Load(TPM_STANY_DATA *tpm_stany_data,
			      unsigned char **stream,
			      uint32_t *stream_size);
TPM_RESULT TPM_StanyData_Store(TPM_STORE_BUFFER *sbuffer,
			       TPM_STANY_DATA *tpm_stany_data);
void       TPM_StanyData_Delete(TPM_STANY_DATA *tpm_stany_data);

/*
  TPM_STCLEAR_DATA
*/

void       TPM_StclearData_Init(TPM_STCLEAR_DATA *tpm_stclear_data,
                                TPM_PCR_ATTRIBUTES *pcrAttrib,
                                TPM_BOOL pcrInit);
TPM_RESULT TPM_StclearData_Load(TPM_STCLEAR_DATA *tpm_stclear_data,
                                unsigned char **stream,
                                uint32_t *stream_size,
                                TPM_PCR_ATTRIBUTES *pcrAttrib);
TPM_RESULT TPM_StclearData_Store(TPM_STORE_BUFFER *sbuffer,
                                 TPM_STCLEAR_DATA *tpm_stclear_data,
                                 TPM_PCR_ATTRIBUTES *pcrAttrib);
void       TPM_StclearData_Delete(TPM_STCLEAR_DATA *tpm_stclear_data,
                                  TPM_PCR_ATTRIBUTES *pcrAttrib,
                                  TPM_BOOL pcrInit);

void       TPM_StclearData_SessionInit(TPM_STCLEAR_DATA *tpm_stclear_data);
void       TPM_StclearData_SessionDelete(TPM_STCLEAR_DATA *tpm_stclear_data);
void       TPM_StclearData_AuthSessionDelete(TPM_STCLEAR_DATA *tpm_stclear_data);

/* Actions */

TPM_RESULT TPM_InitCmd(tpm_state_t *tpm_state);

/*
  Processing Functions
*/

TPM_RESULT TPM_Process_Init(tpm_state_t *tpm_state,
                            TPM_STORE_BUFFER *response,
                            TPM_TAG tag,
                            uint32_t paramSize,
                            TPM_COMMAND_CODE ordinal,
                            unsigned char *command,
                            TPM_TRANSPORT_INTERNAL *transportInternal);

/* generic function prototype for a handle array getEntry callback function */

typedef TPM_RESULT (*TPM_GETENTRY_FUNCTION_T )(void **entry,
                                               void *entries,
                                               TPM_HANDLE handle);

TPM_RESULT TPM_Handle_GenerateHandle(TPM_HANDLE *tpm_handle,
                                     void *tpm_handle_entries,
                                     TPM_BOOL keepHandle,
                                     TPM_BOOL isKeyHandle,
                                     TPM_GETENTRY_FUNCTION_T getEntryFunction);

#endif
