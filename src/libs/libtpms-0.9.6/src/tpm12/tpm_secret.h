/********************************************************************************/
/*                                                                              */
/*                              Secret Data Handler                             */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_secret.h $            */
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

#ifndef TPM_SECRET_H
#define TPM_SECRET_H

#include "tpm_store.h"
#include "tpm_structures.h"
#include "tpm_types.h"

void       TPM_Secret_Init(TPM_SECRET tpm_secret);
TPM_RESULT TPM_Secret_Load(TPM_SECRET tpm_secret,
                           unsigned char **stream,
                           uint32_t *stream_size);
TPM_RESULT TPM_Secret_Store(TPM_STORE_BUFFER *sbuffer,
                            const TPM_SECRET tpm_secret);
void       TPM_Secret_Delete(TPM_SECRET tpm_secret);


void       TPM_Secret_Copy(TPM_SECRET destination, const TPM_SECRET source);
TPM_RESULT TPM_Secret_Compare(TPM_SECRET expect, const TPM_SECRET actual);
TPM_RESULT TPM_Secret_Generate(TPM_SECRET tpm_secret);
void       TPM_Secret_XOR(TPM_SECRET output, TPM_SECRET input1, TPM_SECRET input2);


#endif
