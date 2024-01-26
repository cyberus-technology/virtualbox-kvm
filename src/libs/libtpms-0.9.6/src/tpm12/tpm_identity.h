/********************************************************************************/
/*                                                                              */
/*                      TPM Identity Handling                                   */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_identity.h $          */
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

#ifndef TPM_IDENTITY_H
#define TPM_IDENTITY_H

#include "tpm_global.h"

/*
  TPM_EK_BLOB
*/

void       TPM_EKBlob_Init(TPM_EK_BLOB *tpm_ek_blob);
TPM_RESULT TPM_EKBlob_Load(TPM_EK_BLOB *tpm_ek_blob,
                           unsigned char **stream,
                           uint32_t *stream_size);
#if 0
TPM_RESULT TPM_EKBlob_Store(TPM_STORE_BUFFER *sbuffer,
                            const TPM_EK_BLOB *tpm_ek_blob);
#endif
void       TPM_EKBlob_Delete(TPM_EK_BLOB *tpm_ek_blob);

/*
  TPM_EK_BLOB_ACTIVATE
*/

void       TPM_EKBlobActivate_Init(TPM_EK_BLOB_ACTIVATE *tpm_ek_blob_activate);
TPM_RESULT TPM_EKBlobActivate_Load(TPM_EK_BLOB_ACTIVATE *tpm_ek_blob_activate,
                                   unsigned char **stream,
                                   uint32_t *stream_size);
#if 0
TPM_RESULT TPM_EKBlobActivate_Store(TPM_STORE_BUFFER *sbuffer,
                                    const TPM_EK_BLOB_ACTIVATE *tpm_ek_blob_activate);
#endif
void       TPM_EKBlobActivate_Delete(TPM_EK_BLOB_ACTIVATE *tpm_ek_blob_activate);

/*
  TPM_EK_BLOB_AUTH
*/

#if 0
void       TPM_EKBlobAuth_Init(TPM_EK_BLOB_AUTH *tpm_ek_blob_auth);
TPM_RESULT TPM_EKBlobAuth_Load(TPM_EK_BLOB_AUTH *tpm_ek_blob_auth,
                               unsigned char **stream,
                               uint32_t *stream_size);
TPM_RESULT TPM_EKBlobAuth_Store(TPM_STORE_BUFFER *sbuffer,
                                const TPM_EK_BLOB_AUTH *tpm_ek_blob_auth);
void       TPM_EKBlobAuth_Delete(TPM_EK_BLOB_AUTH *tpm_ek_blob_auth);
#endif


/*
  TPM_IDENTITY_CONTENTS
*/

void       TPM_IdentityContents_Init(TPM_IDENTITY_CONTENTS *tpm_identity_contents);
#if 0
TPM_RESULT TPM_IdentityContents_Load(TPM_IDENTITY_CONTENTS *tpm_identity_contents,
                                     unsigned char **stream,
                                     uint32_t *stream_size);
#endif
TPM_RESULT TPM_IdentityContents_Store(TPM_STORE_BUFFER *sbuffer,
                                      TPM_IDENTITY_CONTENTS *tpm_identity_contents);
void       TPM_IdentityContents_Delete(TPM_IDENTITY_CONTENTS *tpm_identity_contents);

/*
  TPM_ASYM_CA_CONTENTS
*/

void       TPM_AsymCaContents_Init(TPM_ASYM_CA_CONTENTS *tpm_asym_ca_contents);
TPM_RESULT TPM_AsymCaContents_Load(TPM_ASYM_CA_CONTENTS *tpm_asym_ca_contents,
                                   unsigned char **stream,
                                   uint32_t *stream_size);
#if 0
TPM_RESULT TPM_AsymCaContents_Store(TPM_STORE_BUFFER *sbuffer,
                                    const TPM_ASYM_CA_CONTENTS *tpm_asym_ca_contents);
#endif
void       TPM_AsymCaContents_Delete(TPM_ASYM_CA_CONTENTS *tpm_asym_ca_contents);


/*
  Processing Functions
*/


TPM_RESULT TPM_Process_MakeIdentity(tpm_state_t *tpm_state,
                                    TPM_STORE_BUFFER *response,
                                    TPM_TAG tag,
                                    uint32_t paramSize,
                                    TPM_COMMAND_CODE ordinal,
                                    unsigned char *command,
                                    TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_ActivateIdentity(tpm_state_t *tpm_state,
                                        TPM_STORE_BUFFER *response,
                                        TPM_TAG tag,
                                        uint32_t paramSize,
                                        TPM_COMMAND_CODE ordinal,
                                        unsigned char *command,
                                        TPM_TRANSPORT_INTERNAL *transportInternal);


#endif
