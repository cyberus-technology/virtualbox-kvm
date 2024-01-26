/********************************************************************************/
/*                                                                              */
/*                           Ver Structure Handler                              */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_ver.h $               */
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

#ifndef TPM_VER_H
#define TPM_VER_H

#include "tpm_types.h"
#include "tpm_store.h"
#include "tpm_structures.h"

/* TPM_STRUCT_VER */

void       TPM_StructVer_Init(TPM_STRUCT_VER *tpm_struct_ver);
TPM_RESULT TPM_StructVer_Load(TPM_STRUCT_VER *tpm_struct_ver,
                              unsigned char **stream,
                              uint32_t *stream_size);
TPM_RESULT TPM_StructVer_Store(TPM_STORE_BUFFER *sbuffer,
                               const TPM_STRUCT_VER *tpm_struct_ver);
void       TPM_StructVer_Copy(TPM_STRUCT_VER *tpm_struct_ver_dest,
                              TPM_STRUCT_VER *tpm_struct_ver_src);
TPM_RESULT TPM_StructVer_CheckVer(TPM_STRUCT_VER *tpm_struct_ver);

/* TPM_VERSION */

void       TPM_Version_Init(TPM_VERSION *tpm_version);
void       TPM_Version_Set(TPM_VERSION *tpm_version,
                           TPM_PERMANENT_DATA *tpm_permanent_data);
#if 0
TPM_RESULT TPM_Version_Load(TPM_VERSION *tpm_version,
                            unsigned char **stream,
                            uint32_t *stream_size);
#endif
TPM_RESULT TPM_Version_Store(TPM_STORE_BUFFER *sbuffer,
                             const TPM_VERSION *tpm_version);
void       TPM_Version_Delete(TPM_VERSION *tpm_version);


 

#endif
