/********************************************************************************/
/*                                                                              */
/*                              Tick Handler                                    */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_ticks.h $             */
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

#ifndef TPM_TICKS_H
#define TPM_TICKS_H

#include "tpm_types.h"
#include "tpm_global.h"
#include "tpm_structures.h"

/*
  UINT64
*/

void       TPM_Uint64_Init(TPM_UINT64 *tpm_uint64);
TPM_RESULT TPM_Uint64_Load(TPM_UINT64 *tpm_uint64,
			   unsigned char **stream,
			   uint32_t *stream_size);
TPM_RESULT TPM_Uint64_Store(TPM_STORE_BUFFER *sbuffer,
                            const TPM_UINT64 *tpm_uint64);
void       TPM_Uint64_Copy(TPM_UINT64 *dest,
                           const TPM_UINT64 *src);
TPM_RESULT TPM_Uint64_Test(void);

/*
  TPM_CURRENT_TICKS
*/

void       TPM_CurrentTicks_Init(TPM_CURRENT_TICKS *tpm_current_ticks);
TPM_RESULT TPM_CurrentTicks_Store(TPM_STORE_BUFFER *sbuffer,
                                  const TPM_CURRENT_TICKS *tpm_current_ticks);
TPM_RESULT TPM_CurrentTicks_LoadAll(TPM_CURRENT_TICKS *tpm_current_ticks,
				    unsigned char **stream,
				    uint32_t *stream_size);
TPM_RESULT TPM_CurrentTicks_StoreAll(TPM_STORE_BUFFER *sbuffer,
				     const TPM_CURRENT_TICKS *tpm_current_ticks);
TPM_RESULT TPM_CurrentTicks_Start(TPM_CURRENT_TICKS *tpm_current_ticks);
TPM_RESULT TPM_CurrentTicks_Update(TPM_CURRENT_TICKS *tpm_current_ticks);
void       TPM_CurrentTicks_Copy(TPM_CURRENT_TICKS *dest,
                                 TPM_CURRENT_TICKS *src);

/*
  Processing Functions
*/

TPM_RESULT TPM_Process_GetTicks(tpm_state_t *tpm_state,
                                TPM_STORE_BUFFER *response,
                                TPM_TAG tag,
                                uint32_t paramSize,
                                TPM_COMMAND_CODE ordinal,
                                unsigned char *command,
                                TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_TickStampBlob(tpm_state_t *tpm_state,
                                     TPM_STORE_BUFFER *response,
                                     TPM_TAG tag,
                                     uint32_t paramSize,
                                     TPM_COMMAND_CODE ordinal,
                                     unsigned char *command,
                                     TPM_TRANSPORT_INTERNAL *transportInternal);

#endif
