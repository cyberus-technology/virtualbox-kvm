/********************************************************************************/
/*                                                                              */
/*                      Permanent Flag and Data Handler                         */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_permanent.h $		*/
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

#ifndef TPM_PERMANENT_H
#define TPM_PERMANENT_H

#include "tpm_global.h"
#include "tpm_structures.h"


/*
  TPM_PERMANENT_FLAGS
*/

void       TPM_PermanentFlags_Init(TPM_PERMANENT_FLAGS *tpm_permanent_flags);
TPM_RESULT TPM_PermanentFlags_Load(TPM_PERMANENT_FLAGS *tpm_permanent_flags,
                                   unsigned char **stream,
                                   uint32_t *stream_size);
TPM_RESULT TPM_PermanentFlags_Store(TPM_STORE_BUFFER *sbuffer,
                                    const TPM_PERMANENT_FLAGS *tpm_permanent_flags);
TPM_RESULT TPM_PermanentFlags_LoadBitmap(TPM_PERMANENT_FLAGS *tpm_permanent_flags,
					 TPM_TAG permanentFlagsVersion,
					 uint32_t tpm_bitmap);
TPM_RESULT TPM_PermanentFlags_StoreBitmap(uint32_t *tpm_bitmap,
					  const TPM_PERMANENT_FLAGS *tpm_permanent_flags);
TPM_RESULT TPM_PermanentFlags_StoreBytes(TPM_STORE_BUFFER *sbuffer,
					 const TPM_PERMANENT_FLAGS *tpm_permanent_flags);

/*
  TPM_PERMANENT_DATA
*/

TPM_RESULT TPM_PermanentData_Init(TPM_PERMANENT_DATA *tpm_permanent_data,
                                  TPM_BOOL instanceData);
TPM_RESULT TPM_PermanentData_Load(TPM_PERMANENT_DATA *tpm_permanent_data,
                                  unsigned char **stream,
                                  uint32_t *stream_size,
                                  TPM_BOOL instanceData);
TPM_RESULT TPM_PermanentData_Store(TPM_STORE_BUFFER *sbuffer,
                                   TPM_PERMANENT_DATA *tpm_permanent_data,
                                   TPM_BOOL instanceData);
void       TPM_PermanentData_Delete(TPM_PERMANENT_DATA *tpm_permanent_data,
                                    TPM_BOOL instanceData);
void       TPM_PermanentData_Zero(TPM_PERMANENT_DATA *tpm_permanent_data,
				  TPM_BOOL instanceData);

TPM_RESULT TPM_PermanentData_InitDaa(TPM_PERMANENT_DATA *tpm_permanent_data);

/*
  PermanentAll is TPM_PERMANENT_DATA plus TPM_PERMANENT_FLAGS
*/

TPM_RESULT TPM_PermanentAll_Load(tpm_state_t *tpm_state,
				 unsigned char **stream,
				 uint32_t *stream_size);
TPM_RESULT TPM_PermanentAll_Store(TPM_STORE_BUFFER *sbuffer,
				  const unsigned char **buffer,
				  uint32_t *length,
				  tpm_state_t *tpm_state);

TPM_RESULT TPM_PermanentAll_NVLoad(tpm_state_t *tpm_state);
TPM_RESULT TPM_PermanentAll_NVStore(tpm_state_t *tpm_state,
				    TPM_BOOL writeAllNV,
				    TPM_RESULT rcIn);
TPM_RESULT TPM_PermanentAll_NVDelete(uint32_t tpm_number,
				     TPM_BOOL mustExist);

TPM_RESULT TPM_PermanentAll_IsSpace(tpm_state_t *tpm_state);
TPM_RESULT TPM_PermanentAll_GetSpace(uint32_t *bytes_free,
				     tpm_state_t *tpm_state);

#endif
