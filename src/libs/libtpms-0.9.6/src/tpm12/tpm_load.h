/********************************************************************************/
/*                                                                              */
/*                      Load from Stream Utilities                              */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*           $Id: tpm_load.h $               */
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

#ifndef TPM_LOAD_H
#define TPM_LOAD_H

#include "tpm_types.h"

TPM_RESULT TPM_Load32(uint32_t *tpm_uint32,
                      unsigned char **stream,
                      uint32_t *stream_size);
TPM_RESULT TPM_Load16(uint16_t *tpm_uint16,
                      unsigned char **stream,
                      uint32_t *stream_size);
TPM_RESULT TPM_Load8(uint8_t *tpm_uint8,
                     unsigned char **stream,
                     uint32_t *stream_size);
TPM_RESULT TPM_Loadn(BYTE *data,
                     size_t data_length,
                     unsigned char **stream,
                     uint32_t *stream_size);
TPM_RESULT TPM_LoadBool(TPM_BOOL *tpm_bool,
                        unsigned char **stream,
                        uint32_t *stream_size);

TPM_RESULT TPM_LoadLong(unsigned long *result,
                        const unsigned char *stream,
                        uint32_t stream_size);
#if 0
TPM_RESULT TPM_LoadString(const char **name,
                          unsigned char **stream,
                          uint32_t *stream_size);
#endif
TPM_RESULT TPM_CheckTag(TPM_STRUCTURE_TAG expectedTag,
			unsigned char **stream,
                        uint32_t *stream_size);

/* byte stream to type */

uint32_t LOAD32(const unsigned char *buffer, unsigned int offset);
uint16_t LOAD16(const unsigned char *buffer, unsigned int offset);
uint8_t  LOAD8(const unsigned char *buffer, unsigned int offset);

#endif
