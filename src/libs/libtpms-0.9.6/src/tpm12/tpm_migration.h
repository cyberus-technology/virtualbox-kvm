/********************************************************************************/
/*                                                                              */
/*                              Migration                                       */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_migration.h $         */
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

#ifndef TPM_MIGRATION_H
#define TPM_MIGRATION_H

#include "tpm_global.h"

/*
  TPM_MIGRATIONKEYAUTH
*/
  
void       TPM_Migrationkeyauth_Init(TPM_MIGRATIONKEYAUTH *tpm_migrationkeyauth);
TPM_RESULT TPM_Migrationkeyauth_Load(TPM_MIGRATIONKEYAUTH *tpm_migrationkeyauth,
                                     unsigned char **stream,
                                     uint32_t *stream_size);
TPM_RESULT TPM_Migrationkeyauth_Store(TPM_STORE_BUFFER *sbuffer,
                                      TPM_MIGRATIONKEYAUTH *tpm_migrationkeyauth);
void       TPM_Migrationkeyauth_Delete(TPM_MIGRATIONKEYAUTH *tpm_migrationkeyauth);

/*
  TPM_MSA_COMPOSITE
*/

void       TPM_MsaComposite_Init(TPM_MSA_COMPOSITE *tpm_msa_composite);
TPM_RESULT TPM_MsaComposite_Load(TPM_MSA_COMPOSITE *tpm_msa_composite,
                                 unsigned char **stream,
                                 uint32_t *stream_size);
TPM_RESULT TPM_MsaComposite_Store(TPM_STORE_BUFFER *sbuffer,
                                  const TPM_MSA_COMPOSITE *tpm_msa_composite);
void       TPM_MsaComposite_Delete(TPM_MSA_COMPOSITE *tpm_msa_composite);

TPM_RESULT TPM_MsaComposite_CheckMigAuthDigest(TPM_DIGEST tpm_digest,
                                               TPM_MSA_COMPOSITE *tpm_msa_composite);
TPM_RESULT TPM_MsaComposite_CheckSigTicket(TPM_DIGEST sigTicket,
                                           TPM_SECRET tpmProof,
                                           TPM_MSA_COMPOSITE *tpm_msa_composite,
                                           TPM_CMK_SIGTICKET *tpm_cmk_sigticket);

/*
  TPM_CMK_AUTH
*/

void       TPM_CmkAuth_Init(TPM_CMK_AUTH *tpm_cmk_auth);
TPM_RESULT TPM_CmkAuth_Load(TPM_CMK_AUTH *tpm_cmk_auth,
                            unsigned char **stream,
                            uint32_t *stream_size);
TPM_RESULT TPM_CmkAuth_Store(TPM_STORE_BUFFER *sbuffer,
                             const TPM_CMK_AUTH *tpm_cmk_auth);
void       TPM_CmkAuth_Delete(TPM_CMK_AUTH *tpm_cmk_auth);

/*
  TPM_CMK_MIGAUTH
*/

void       TPM_CmkMigauth_Init(TPM_CMK_MIGAUTH *tpm_cmk_migauth);
TPM_RESULT TPM_CmkMigauth_Load(TPM_CMK_MIGAUTH *tpm_cmk_migauth,
                               unsigned char **stream,
                               uint32_t *stream_size);
TPM_RESULT TPM_CmkMigauth_Store(TPM_STORE_BUFFER *sbuffer,
                                const TPM_CMK_MIGAUTH *tpm_cmk_migauth);
void       TPM_CmkMigauth_Delete(TPM_CMK_MIGAUTH *tpm_cmk_migauth);

TPM_RESULT TPM_CmkMigauth_CheckHMAC(TPM_BOOL *valid,
                                    TPM_HMAC tpm_hmac,
                                    TPM_SECRET tpm_hmac_key,
                                    TPM_CMK_MIGAUTH *tpm_cmk_migauth);

/*
  TPM_CMK_SIGTICKET
*/

void       TPM_CmkSigticket_Init(TPM_CMK_SIGTICKET *tpm_cmk_sigticket);
TPM_RESULT TPM_CmkSigticket_Load(TPM_CMK_SIGTICKET *tpm_cmk_sigticket,
                                 unsigned char **stream,
                                 uint32_t *stream_size);
TPM_RESULT TPM_CmkSigticket_Store(TPM_STORE_BUFFER *sbuffer,
                                  const TPM_CMK_SIGTICKET *tpm_cmk_sigticket);
void       TPM_CmkSigticket_Delete(TPM_CMK_SIGTICKET *tpm_cmk_sigticket);

/*
  TPM_CMK_MA_APPROVAL
*/

void       TPM_CmkMaApproval_Init(TPM_CMK_MA_APPROVAL *tpm_cmk_ma_approval);
TPM_RESULT TPM_CmkMaApproval_Load(TPM_CMK_MA_APPROVAL *tpm_cmk_ma_approval,
                                  unsigned char **stream,
                                  uint32_t *stream_size);
TPM_RESULT TPM_CmkMaApproval_Store(TPM_STORE_BUFFER *sbuffer,
                                   const TPM_CMK_MA_APPROVAL *tpm_cmk_ma_approval);
void       TPM_CmkMaApproval_Delete(TPM_CMK_MA_APPROVAL *tpm_cmk_ma_approval);

TPM_RESULT TPM_CmkMaApproval_CheckHMAC(TPM_BOOL *valid,
                                       TPM_HMAC tpm_hmac,
                                       TPM_SECRET tpm_hmac_key,
                                       TPM_CMK_MA_APPROVAL *tpm_cmk_ma_approval);

/*
  Processing Functions
*/

TPM_RESULT TPM_CreateBlobCommon(TPM_SIZED_BUFFER *outData,
                                TPM_STORE_ASYMKEY *d1Key,
                                TPM_DIGEST pHash,
                                TPM_PAYLOAD_TYPE payload_type,
                                TPM_SIZED_BUFFER *random,
                                TPM_PUBKEY *migrationKey);

TPM_RESULT TPM_Process_CreateMigrationBlob(tpm_state_t *tpm_state,
                                           TPM_STORE_BUFFER *response,
                                           TPM_TAG tag,
                                           uint32_t paramSize,
                                           TPM_COMMAND_CODE ordinal,
                                           unsigned char *command,
                                           TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_ConvertMigrationBlob(tpm_state_t *tpm_state,
                                            TPM_STORE_BUFFER *response,
                                            TPM_TAG tag,
                                            uint32_t paramSize,
                                            TPM_COMMAND_CODE ordinal,
                                            unsigned char *command,
                                            TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_AuthorizeMigrationKey(tpm_state_t *tpm_state,
                                             TPM_STORE_BUFFER *response,
                                             TPM_TAG tag,
                                             uint32_t paramSize,
                                             TPM_COMMAND_CODE ordinal,
                                             unsigned char *command,
                                             TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_MigrateKey(tpm_state_t *tpm_state,
                                  TPM_STORE_BUFFER *response,
                                  TPM_TAG tag,
                                  uint32_t paramSize,
                                  TPM_COMMAND_CODE ordinal,
                                  unsigned char *command,
                                  TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_CMK_CreateKey(tpm_state_t *tpm_state,
                                     TPM_STORE_BUFFER *response,
                                     TPM_TAG tag,
                                     uint32_t paramSize,
                                     TPM_COMMAND_CODE ordinal,
                                     unsigned char *command,
                                     TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_CMK_CreateTicket(tpm_state_t *tpm_state,
                                        TPM_STORE_BUFFER *response,
                                        TPM_TAG tag,
                                        uint32_t paramSize,
                                        TPM_COMMAND_CODE ordinal,
                                        unsigned char *command,
                                        TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_CMK_CreateBlob(tpm_state_t *tpm_state,
                                      TPM_STORE_BUFFER *response,
                                      TPM_TAG tag,
                                      uint32_t paramSize,
                                      TPM_COMMAND_CODE ordinal,
                                      unsigned char *command,
                                      TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_CMK_SetRestrictions(tpm_state_t *tpm_state,
                                           TPM_STORE_BUFFER *response,
                                           TPM_TAG tag,
                                           uint32_t paramSize,
                                           TPM_COMMAND_CODE ordinal,
                                           unsigned char *command,
                                           TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_CMK_ApproveMA(tpm_state_t *tpm_state,
                                     TPM_STORE_BUFFER *response,
                                     TPM_TAG tag,
                                     uint32_t paramSize,
                                     TPM_COMMAND_CODE ordinal,
                                     unsigned char *command,
                                     TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_CMK_ConvertMigration(tpm_state_t *tpm_state,
                                            TPM_STORE_BUFFER *response,
                                            TPM_TAG tag,
                                            uint32_t paramSize,
                                            TPM_COMMAND_CODE ordinal,
                                            unsigned char *command,
                                            TPM_TRANSPORT_INTERNAL *transportInternal);



#endif
