/********************************************************************************/
/*                                                                              */
/*                              Counter Handler                                 */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_counter.h $           */
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

#ifndef TPM_COUNTER_H
#define TPM_COUNTER_H

#include "tpm_global.h"
#include "tpm_store.h"
#include "tpm_structures.h"

/*
  Counter Resource Handling
*/

void       TPM_Counters_Init(TPM_COUNTER_VALUE *monotonicCounters);
TPM_RESULT TPM_Counters_Load(TPM_COUNTER_VALUE *monotonicCountersa,
                             unsigned char **stream,
                             uint32_t *stream_size);
TPM_RESULT TPM_Counters_Store(TPM_STORE_BUFFER *sbuffer,
                              TPM_COUNTER_VALUE *monotonicCounters);

TPM_RESULT TPM_Counters_StoreHandles(TPM_STORE_BUFFER *sbuffer,
                                     TPM_COUNTER_VALUE *monotonicCounters);
TPM_RESULT TPM_Counters_GetNewHandle(TPM_COUNTER_VALUE **tpm_counter_value,
                                     TPM_COUNT_ID *countID,
                                     TPM_COUNTER_VALUE *monotonicCounters);
void       TPM_Counters_GetSpace(uint32_t *space,
                                 TPM_COUNTER_VALUE *monotonicCounters);
void       TPM_Counters_GetNextCount(TPM_ACTUAL_COUNT *nextCount,
                                     TPM_COUNTER_VALUE *monotonicCounters);
TPM_RESULT TPM_Counters_IsValidId(TPM_COUNTER_VALUE *monotonicCounters,
                                  TPM_COUNT_ID countID);
TPM_RESULT TPM_Counters_GetCounterValue(TPM_COUNTER_VALUE **tpm_counter_value,
                                        TPM_COUNTER_VALUE *monotonicCounters,
                                        TPM_COUNT_ID countID);
TPM_RESULT TPM_Counters_Release(TPM_COUNTER_VALUE *monotonicCounters);
void       TPM_Counters_GetActiveCounter(TPM_COUNT_ID *activeCounter,
                                         TPM_COUNT_ID countID);


/*
  TPM_COUNTER_VALUE
*/

void       TPM_CounterValue_Init(TPM_COUNTER_VALUE *tpm_counter_value);
TPM_RESULT TPM_CounterValue_Load(TPM_COUNTER_VALUE *tpm_counter_value,
                                 unsigned char **stream,
                                 uint32_t *stream_size);
TPM_RESULT TPM_CounterValue_Store(TPM_STORE_BUFFER *sbuffer,
                                  const TPM_COUNTER_VALUE *tpm_counter_value);

TPM_RESULT TPM_CounterValue_StorePublic(TPM_STORE_BUFFER *sbuffer,
                                        const TPM_COUNTER_VALUE *tpm_counter_value);
void       TPM_CounterValue_CopyPublic(TPM_COUNTER_VALUE *dst_tpm_counter_value,
                                       TPM_COUNTER_VALUE *src_tpm_counter_value);
TPM_RESULT TPM_CounterValue_Set(TPM_COUNTER_VALUE *tpm_counter_value,
                                TPM_COUNT_ID countID,
                                BYTE *label,
                                TPM_ACTUAL_COUNT counter,
                                TPM_SECRET authData);
TPM_RESULT TPM_CounterValue_Release(TPM_COUNTER_VALUE *tpm_counter_value,
                                    TPM_COUNT_ID countID);
/*
  Processing Functions
*/

TPM_RESULT TPM_Process_CreateCounter(tpm_state_t *tpm_state,
                                     TPM_STORE_BUFFER *response,
                                     TPM_TAG tag,
                                     uint32_t paramSize,
                                     TPM_COMMAND_CODE ordinal,
                                     unsigned char *command,
                                     TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_IncrementCounter(tpm_state_t *tpm_state,
                                        TPM_STORE_BUFFER *response,
                                        TPM_TAG tag,
                                        uint32_t paramSize,
                                        TPM_COMMAND_CODE ordinal,
                                        unsigned char *command,
                                        TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_ReadCounter(tpm_state_t *tpm_state,
                                   TPM_STORE_BUFFER *response,
                                   TPM_TAG tag,
                                   uint32_t paramSize,
                                   TPM_COMMAND_CODE ordinal,
                                   unsigned char *command,
                                   TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_ReleaseCounter(tpm_state_t *tpm_state,
                                      TPM_STORE_BUFFER *response,
                                      TPM_TAG tag,
                                      uint32_t paramSize,
                                      TPM_COMMAND_CODE ordinal,
                                      unsigned char *command,
                                      TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_ReleaseCounterOwner(tpm_state_t *tpm_state,
                                           TPM_STORE_BUFFER *response,
                                           TPM_TAG tag,
                                           uint32_t paramSize,
                                           TPM_COMMAND_CODE ordinal,
                                           unsigned char *command,
                                           TPM_TRANSPORT_INTERNAL *transportInternal);


#endif
