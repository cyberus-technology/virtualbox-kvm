/********************************************************************************/
/*                                                                              */
/*                        TPM Sized Buffer Handler                              */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_sizedbuffer.h $	*/
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

#ifndef TPM_SIZEDBUFFER_H
#define TPM_SIZEDBUFFER_H

#include "tpm_digest.h"
#include "tpm_store.h"

void       TPM_SizedBuffer_Init(TPM_SIZED_BUFFER *tpm_sized_buffer);
TPM_RESULT TPM_SizedBuffer_Load(TPM_SIZED_BUFFER *tpm_sized_buffer,
                                unsigned char **stream,
                                uint32_t *stream_size);
TPM_RESULT TPM_SizedBuffer_Store(TPM_STORE_BUFFER *sbuffer,
                                 const TPM_SIZED_BUFFER *tpm_sized_buffer); 
TPM_RESULT TPM_SizedBuffer_Set(TPM_SIZED_BUFFER *tpm_sized_buffer,
                               uint32_t size,
                               const unsigned char *data);
TPM_RESULT TPM_SizedBuffer_SetFromStore(TPM_SIZED_BUFFER *tpm_sized_buffer,
                                        TPM_STORE_BUFFER *sbuffer);
TPM_RESULT TPM_SizedBuffer_SetStructure(TPM_SIZED_BUFFER *tpm_sized_buffer,
                                        void *tpmStructure,
                                        TPM_STORE_FUNCTION_T storeFunction);
TPM_RESULT TPM_SizedBuffer_Copy(TPM_SIZED_BUFFER *tpm_sized_buffer_dest,
                               TPM_SIZED_BUFFER *tpm_sized_buffer_src);
void       TPM_SizedBuffer_Delete(TPM_SIZED_BUFFER *tpm_sized_buffer);
TPM_RESULT TPM_SizedBuffer_Allocate(TPM_SIZED_BUFFER *tpm_sized_buffer,
                                    uint32_t size);
TPM_RESULT TPM_SizedBuffer_GetBool(TPM_BOOL *tpm_bool,
                                   TPM_SIZED_BUFFER *tpm_sized_buffer);
TPM_RESULT TPM_SizedBuffer_GetUint32(uint32_t *uint32,
                                     TPM_SIZED_BUFFER *tpm_sized_buffer);
TPM_RESULT TPM_SizedBuffer_Append32(TPM_SIZED_BUFFER *tpm_sized_buffer,
                                    uint32_t uint32);
TPM_RESULT TPM_SizedBuffer_Remove32(TPM_SIZED_BUFFER *tpm_sized_buffer,
                                    uint32_t uint32);
void       TPM_SizedBuffer_Zero(TPM_SIZED_BUFFER *tpm_sized_buffer);

#endif
