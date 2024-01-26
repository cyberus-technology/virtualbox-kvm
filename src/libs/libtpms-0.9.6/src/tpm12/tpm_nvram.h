/********************************************************************************/
/*                                                                              */
/*                              NVRAM Utilities                                 */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_nvram.h $             */
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

#ifndef TPM_NVRAM_H
#define TPM_NVRAM_H

#include <sys/types.h>

#include "tpm_global.h"
#include "tpm_types.h"

/*
  NVRAM common functions
*/

/*
  TPM_NV_ATTRIBUTES
*/

void       TPM_NVAttributes_Init(TPM_NV_ATTRIBUTES *tpm_nv_attributes);
TPM_RESULT TPM_NVAttributes_Load(TPM_NV_ATTRIBUTES *tpm_nv_attributes,
                                 unsigned char **stream,
                                 uint32_t *stream_size);
TPM_RESULT TPM_NVAttributes_Store(TPM_STORE_BUFFER *sbuffer,
                                  const TPM_NV_ATTRIBUTES *tpm_nv_attributes);
void       TPM_NVAttributes_Delete(TPM_NV_ATTRIBUTES *tpm_nv_attributes);

void       TPM_NVAttributes_Copy(TPM_NV_ATTRIBUTES *tpm_nv_attributes_dest,
                                 TPM_NV_ATTRIBUTES *tpm_nv_attributes_src);

/*
  TPM_NV_DATA_PUBLIC
*/

void       TPM_NVDataPublic_Init(TPM_NV_DATA_PUBLIC *tpm_nv_data_public);
TPM_RESULT TPM_NVDataPublic_Load(TPM_NV_DATA_PUBLIC *tpm_nv_data_public,
                                 unsigned char **stream,
                                 uint32_t *stream_size,
				 TPM_BOOL optimize);
TPM_RESULT TPM_NVDataPublic_Store(TPM_STORE_BUFFER *sbuffer,
                                  const TPM_NV_DATA_PUBLIC *tpm_nv_data_public,
				  TPM_BOOL optimize);
void       TPM_NVDataPublic_Delete(TPM_NV_DATA_PUBLIC *tpm_nv_data_public);

/*
  TPM_NV_DATA_SENSITIVE
*/

void       TPM_NVDataSensitive_Init(TPM_NV_DATA_SENSITIVE *tpm_nv_data_sensitive);
TPM_RESULT TPM_NVDataSensitive_Load(TPM_NV_DATA_SENSITIVE *tpm_nv_data_sensitive,
				    TPM_TAG nvEntriesVersion,
				    unsigned char **stream,
                                    uint32_t *stream_size);
TPM_RESULT TPM_NVDataSensitive_Store(TPM_STORE_BUFFER *sbuffer,
                                     const TPM_NV_DATA_SENSITIVE *tpm_nv_data_sensitive);
void       TPM_NVDataSensitive_Delete(TPM_NV_DATA_SENSITIVE *tpm_nv_data_sensitive);

TPM_RESULT TPM_NVDataSensitive_IsValidIndex(TPM_NV_INDEX nvIndex);
TPM_RESULT TPM_NVDataSensitive_IsGPIO(TPM_BOOL *isGPIO, TPM_NV_INDEX nvIndex);
TPM_RESULT TPM_NVDataSensitive_IsValidPlatformIndex(TPM_NV_INDEX nvIndex);

/*
  NV Index Entries
*/

void       TPM_NVIndexEntries_Init(TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries);
void       TPM_NVIndexEntries_Delete(TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries);
void       TPM_NVIndexEntries_Trace(TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries);
TPM_RESULT TPM_NVIndexEntries_Load(TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries,
				   unsigned char **stream,
				   uint32_t *stream_size);
TPM_RESULT TPM_NVIndexEntries_Store(TPM_STORE_BUFFER *sbuffer,
				    TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries);
void       TPM_NVIndexEntries_StClear(TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries);
TPM_RESULT TPM_NVIndexEntries_LoadVolatile(TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries,
					   unsigned char **stream,
					   uint32_t *stream_size);
TPM_RESULT TPM_NVIndexEntries_StoreVolatile(TPM_STORE_BUFFER *sbuffer,
					    TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries);
TPM_RESULT TPM_NVIndexEntries_GetVolatile(TPM_NV_DATA_ST **tpm_nv_data_st,
					  TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries);
TPM_RESULT TPM_NVIndexEntries_SetVolatile(TPM_NV_DATA_ST *tpm_nv_data_st,
					  TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries);
TPM_RESULT TPM_NVIndexEntries_GetFreeEntry(TPM_NV_DATA_SENSITIVE **tpm_nv_data_sensitive,
					   TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries);
TPM_RESULT TPM_NVIndexEntries_GetEntry(TPM_NV_DATA_SENSITIVE **tpm_nv_data_sensitive,
				       TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries,
				       TPM_NV_INDEX nvIndex);
TPM_RESULT TPM_NVIndexEntries_GetUsedCount(uint32_t *count,
					   TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries);
TPM_RESULT TPM_NVIndexEntries_GetNVList(TPM_STORE_BUFFER *sbuffer,
					TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries);
TPM_RESULT TPM_NVIndexEntries_GetUsedSpace(uint32_t *usedSpace,
					   TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries);
TPM_RESULT TPM_NVIndexEntries_DeleteOwnerAuthorized(TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries,
						    TPM_BOOL deleteAllNvram);
TPM_RESULT TPM_NVIndexEntries_GetUsedSpace(uint32_t *usedSpace,
					   TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries);
TPM_RESULT TPM_NVIndexEntries_GetFreeSpace(uint32_t *freeSpace,
					   TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries);
TPM_RESULT TPM_NVIndexEntries_GetDataPublic(TPM_NV_DATA_PUBLIC **tpm_nv_data_public,
					    TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries,
					    TPM_NV_INDEX nvIndex);

/*
  Processing Functions
*/


TPM_RESULT TPM_Process_NVReadValue(tpm_state_t *tpm_state,
                                   TPM_STORE_BUFFER *response,
                                   TPM_TAG tag,
                                   uint32_t paramSize,
                                   TPM_COMMAND_CODE ordinal,
                                   unsigned char *command,
                                   TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_NVReadValueAuth(tpm_state_t *tpm_state,
                                       TPM_STORE_BUFFER *response,
                                       TPM_TAG tag,
                                       uint32_t paramSize,
                                       TPM_COMMAND_CODE ordinal,
                                       unsigned char *command,
                                       TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_NVWriteValue(tpm_state_t *tpm_state,
                                    TPM_STORE_BUFFER *response,
                                    TPM_TAG tag,
                                    uint32_t paramSize,
                                    TPM_COMMAND_CODE ordinal,
                                    unsigned char *command,
                                    TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_NVWriteValueAuth(tpm_state_t *tpm_state,
                                        TPM_STORE_BUFFER *response,
                                        TPM_TAG tag,
                                        uint32_t paramSize,
                                        TPM_COMMAND_CODE ordinal,
                                        unsigned char *command,
                                        TPM_TRANSPORT_INTERNAL *transportInternal);


TPM_RESULT TPM_Process_NVDefineSpace(tpm_state_t *tpm_state,
                                     TPM_STORE_BUFFER *response,
                                     TPM_TAG tag,
                                     uint32_t paramSize,
                                     TPM_COMMAND_CODE ordinal,
                                     unsigned char *command,
                                     TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_DirRead(tpm_state_t *tpm_state,
                               TPM_STORE_BUFFER *response,
                               TPM_TAG tag,
                               uint32_t paramSize,
                               TPM_COMMAND_CODE ordinal,
                               unsigned char *command,
                               TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_DirWriteAuth(tpm_state_t *tpm_state,
                                    TPM_STORE_BUFFER *response,
                                    TPM_TAG tag,
                                    uint32_t paramSize,
                                    TPM_COMMAND_CODE ordinal,
                                    unsigned char *command,
                                    TPM_TRANSPORT_INTERNAL *transportInternal);

#endif
