/********************************************************************************/
/*                                                                              */
/*                              Storage Functions                               */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_storage.h $           */
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

#ifndef TPM_STORAGE_H
#define TPM_STORAGE_H

#include "tpm_global.h"
#include "tpm_store.h"
#include "tpm_types.h"

/*
  TPM_BOUND_DATA
*/

void       TPM_BoundData_Init(TPM_BOUND_DATA *tpm_bound_data);
TPM_RESULT TPM_BoundData_Load(TPM_BOUND_DATA *tpm_bound_data,
                              unsigned char **stream,
                              uint32_t *stream_size);
#if 0
TPM_RESULT TPM_BoundData_Store(TPM_STORE_BUFFER *sbuffer,
                               const TPM_BOUND_DATA *tpm_bound_data);
#endif
void       TPM_BoundData_Delete(TPM_BOUND_DATA *tpm_bound_data);

/*
  TPM_SEALED_DATA
*/

void       TPM_SealedData_Init(TPM_SEALED_DATA *tpm_sealed_data);
TPM_RESULT TPM_SealedData_Load(TPM_SEALED_DATA *tpm_sealed_data,
                               unsigned char **stream,
                               uint32_t *stream_size);
TPM_RESULT TPM_SealedData_Store(TPM_STORE_BUFFER *sbuffer,
                                const TPM_SEALED_DATA *tpm_sealed_data);
void       TPM_SealedData_Delete(TPM_SEALED_DATA *tpm_sealed_data);

TPM_RESULT TPM_SealedData_DecryptEncData(TPM_SEALED_DATA *tpm_sealed_data,
                                         TPM_SIZED_BUFFER *enc_data,
                                         TPM_KEY *tpm_key);
TPM_RESULT TPM_SealedData_GenerateEncData(TPM_SIZED_BUFFER *enc_data,
                                          const TPM_SEALED_DATA *tpm_sealed_data,
                                          TPM_KEY *tpm_key);

/*
  TPM_STORED_DATA
*/

void       TPM_StoredData_Init(TPM_STORED_DATA *tpm_stored_data,
                               unsigned int version);
TPM_RESULT TPM_StoredData_Load(TPM_STORED_DATA *tpm_stored_data,
                               unsigned int *version,
                               unsigned char **stream,
                               uint32_t *stream_size);
TPM_RESULT TPM_StoredData_Store(TPM_STORE_BUFFER *sbuffer,
                                TPM_STORED_DATA *tpm_stored_data,
                                unsigned int version);
void       TPM_StoredData_Delete(TPM_STORED_DATA *tpm_stored_data,
                                 unsigned int version);

TPM_RESULT TPM_StoredData_CheckTag(TPM_STORED_DATA12 *tpm_stored_data12);
TPM_RESULT TPM_StoredData_StoreClearData(TPM_STORE_BUFFER *sbuffer,
                                         TPM_STORED_DATA *tpm_stored_data,
                                         unsigned int version);
TPM_RESULT TPM_StoredData_GenerateDigest(TPM_DIGEST tpm_digest,
                                         TPM_STORED_DATA *tpm_stored_data,
                                         unsigned int version);

/*
  Processing functions
*/

TPM_RESULT TPM_Process_Seal(tpm_state_t *tpm_state,
                            TPM_STORE_BUFFER *response,
                            TPM_TAG tag,
                            uint32_t paramSize,
                            TPM_COMMAND_CODE ordinal,
                            unsigned char *command,
                            TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_Sealx(tpm_state_t *tpm_state,
                             TPM_STORE_BUFFER *response,
                             TPM_TAG tag,
                             uint32_t paramSize,
                             TPM_COMMAND_CODE ordinal,
                             unsigned char *command,
                             TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_Unseal(tpm_state_t *tpm_state,
                              TPM_STORE_BUFFER *response,
                              TPM_TAG tag,
                              uint32_t paramSize,
                              TPM_COMMAND_CODE ordinal,
                              unsigned char *command,
                              TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_UnBind(tpm_state_t *tpm_state,
                              TPM_STORE_BUFFER *response,
                              TPM_TAG tag,
                              uint32_t paramSize,
                              TPM_COMMAND_CODE ordinal,
                              unsigned char *command,
                              TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_CreateWrapKey(tpm_state_t *tpm_state,
                                     TPM_STORE_BUFFER *response,
                                     TPM_TAG tag,
                                     uint32_t paramSize,
                                     TPM_COMMAND_CODE ordinal,
                                     unsigned char *command,
                                     TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_LoadKey(tpm_state_t *tpm_state,
                               TPM_STORE_BUFFER *response,
                               TPM_TAG tag,
                               uint32_t paramSize,
                               TPM_COMMAND_CODE ordinal,
                               unsigned char *command,
                               TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_LoadKey2(tpm_state_t *tpm_state,
                                TPM_STORE_BUFFER *response,
                                TPM_TAG tag,
                                uint32_t paramSize,
                                TPM_COMMAND_CODE ordinal,
                                unsigned char *command,
                                TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_GetPubKey(tpm_state_t *tpm_state,
                                 TPM_STORE_BUFFER *response,
                                 TPM_TAG tag,
                                 uint32_t paramSize,
                                 TPM_COMMAND_CODE ordinal,
                                 unsigned char *command,
                                 TPM_TRANSPORT_INTERNAL *transportInternal);
            


#endif
