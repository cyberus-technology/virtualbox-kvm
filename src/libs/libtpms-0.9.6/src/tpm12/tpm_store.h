/********************************************************************************/
/*                                                                              */
/*                              Safe Storage Buffer                             */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_store.h $             */
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

#ifndef TPM_STORE_H
#define TPM_STORE_H

#include "tpm_global.h"
#include "tpm_load.h"
#include "tpm_types.h"

void       TPM_Sbuffer_Init(TPM_STORE_BUFFER *sbuffer);
TPM_RESULT TPM_Sbuffer_Load(TPM_STORE_BUFFER *sbuffer,
                            unsigned char **stream,
                            uint32_t *stream_size);
/* TPM_Sbuffer_Store(): See TPM_Sbuffer_AppendAsSizedBuffer() */
void       TPM_Sbuffer_Delete(TPM_STORE_BUFFER *sbuffer);

void       TPM_Sbuffer_Clear(TPM_STORE_BUFFER *sbuffer);
void       TPM_Sbuffer_Get(TPM_STORE_BUFFER *sbuffer,
                           const unsigned char **buffer,
                           uint32_t *length);
void 	   TPM_Sbuffer_GetAll(TPM_STORE_BUFFER *sbuffer,
			      unsigned char **buffer,
			      uint32_t *length,
			      uint32_t *total);
TPM_RESULT TPM_Sbuffer_Set(TPM_STORE_BUFFER *sbuffer,
			   unsigned char *buffer,
			   const uint32_t length,
			   const uint32_t total);

TPM_RESULT TPM_Sbuffer_Append(TPM_STORE_BUFFER *sbuffer,
                              const unsigned char *data,
                              size_t data_length);

TPM_RESULT TPM_Sbuffer_Append8(TPM_STORE_BUFFER *sbuffer, uint8_t data);
TPM_RESULT TPM_Sbuffer_Append16(TPM_STORE_BUFFER *sbuffer, uint16_t data);
TPM_RESULT TPM_Sbuffer_Append32(TPM_STORE_BUFFER *sbuffer, uint32_t data);
TPM_RESULT TPM_Sbuffer_AppendAsSizedBuffer(TPM_STORE_BUFFER *destSbuffer,
                                           TPM_STORE_BUFFER *srcSbuffer);
TPM_RESULT TPM_Sbuffer_AppendSBuffer(TPM_STORE_BUFFER *destSbuffer,
                                     TPM_STORE_BUFFER *srcSbuffer);


TPM_RESULT TPM_Sbuffer_StoreInitialResponse(TPM_STORE_BUFFER *response,
                                            TPM_TAG response_tag,
                                            TPM_RESULT returnCode);
TPM_RESULT TPM_Sbuffer_StoreFinalResponse(TPM_STORE_BUFFER *sbuffer,
                                          TPM_RESULT returnCode,
                                          tpm_state_t *tpm_state);

#if 0
TPM_RESULT TPM_Sbuffer_Test(void);
#endif

/* type to byte stream */

void STORE32(unsigned char *buffer, unsigned int offset, uint32_t value);
void STORE16(unsigned char *buffer, unsigned int offset, uint16_t value);
void STORE8 (unsigned char *buffer, unsigned int offset, uint8_t  value);

/* load and store to bitmap */

TPM_RESULT TPM_Bitmap_Load(TPM_BOOL *tpm_bool,
			   uint32_t tpm_bitmap,
			   uint32_t *pos);
TPM_RESULT TPM_Bitmap_Store(uint32_t *tpm_bitmap,
			    TPM_BOOL tpm_bool,
			    uint32_t *pos);

/* generic function prototype for a structure store callback function */

typedef TPM_RESULT (*TPM_STORE_FUNCTION_T )(TPM_STORE_BUFFER *sbuffer,
                                            const void *tpm_structure);

#endif
