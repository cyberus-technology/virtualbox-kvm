/********************************************************************************/
/*                                                                              */
/*                              Digest Handler                                  */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_digest.h $            */
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

#ifndef TPM_DIGEST_H
#define TPM_DIGEST_H

#include "tpm_structures.h"
#include "tpm_store.h"

void       TPM_Digest_Init(TPM_DIGEST tpm_digest);
TPM_RESULT TPM_Digest_Load(TPM_DIGEST tpm_digest,
                           unsigned char **stream,
                           uint32_t *stream_size);
TPM_RESULT TPM_Digest_Store(TPM_STORE_BUFFER *sbuffer,
                            const TPM_DIGEST tpm_digest);

void       TPM_Digest_Set(TPM_DIGEST tpm_digest);
void       TPM_Digest_Copy(TPM_DIGEST destination, const TPM_DIGEST source);
void       TPM_Digest_XOR(TPM_DIGEST out,
                          const TPM_DIGEST in1,
                          const TPM_DIGEST in2);
TPM_RESULT TPM_Digest_Compare(const TPM_DIGEST expect, const TPM_DIGEST actual);
void       TPM_Digest_IsZero(TPM_BOOL *isZero, TPM_DIGEST tpm_digest);
#if 0
void 	   TPM_Digest_IsMinusOne(TPM_BOOL *isMinusOne, TPM_DIGEST tpm_digest);
#endif

#endif
