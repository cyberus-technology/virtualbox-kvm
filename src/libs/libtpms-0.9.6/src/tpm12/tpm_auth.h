/********************************************************************************/
/*                                                                              */
/*                              Authorization                                   */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_auth.h $              */
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

#ifndef TPM_AUTH_H
#define TPM_AUTH_H

#include "tpm_global.h"
#include "tpm_session.h"
#include "tpm_store.h"
#include "tpm_structures.h"
#include "tpm_types.h"

/*
  TPM_AUTHDATA
*/

#if 0
void       TPM_Authdata_Init(TPM_AUTHDATA tpm_authdata);
#endif
TPM_RESULT TPM_Authdata_Load(TPM_AUTHDATA tpm_authdata,
                             unsigned char **stream,
                             uint32_t *stream_size);
TPM_RESULT TPM_Authdata_Store(TPM_STORE_BUFFER *sbuffer,
                             const TPM_AUTHDATA tpm_authdata);

TPM_RESULT TPM_Authdata_Generate(TPM_AUTHDATA resAuth,
                                 TPM_SECRET usageAuth,
                                 TPM_DIGEST outParamDigest,
                                 TPM_NONCE nonceEven,
                                 TPM_NONCE nonceOdd,
                                 TPM_BOOL continueSession);

TPM_RESULT TPM_Authdata_Check(tpm_state_t       *tpm_state,
                              TPM_SECRET        hmacKey,
                              TPM_DIGEST        inParamDigest,
                              TPM_AUTH_SESSION_DATA *tpm_auth_session_data,
                              TPM_NONCE         nonceOdd,
                              TPM_BOOL          continueSession,
                              TPM_AUTHDATA      usageAuth);
TPM_RESULT TPM_Auth2data_Check(tpm_state_t      *tpm_state,
                               TPM_SECRET       hmacKey,
                               TPM_DIGEST       inParamDigest,
                               TPM_AUTH_SESSION_DATA *tpm_auth_session_data,
                               TPM_NONCE        nonceOdd,
                               TPM_BOOL         continueSession,
                               TPM_AUTHDATA     usageAuth);

TPM_RESULT TPM_Authdata_Fail(tpm_state_t *tpm_state);
TPM_RESULT TPM_Authdata_GetState(TPM_DA_STATE *state,
                                 uint32_t *timeLeft,
                                 tpm_state_t *tpm_state);
TPM_RESULT TPM_Authdata_CheckState(tpm_state_t *tpm_state);

/*
  Utilities for command input and output parameter load and store
*/

TPM_RESULT TPM_AuthParams_Get(TPM_AUTHHANDLE *authHandle,
                              TPM_BOOL *authHandleValid,
                              TPM_NONCE nonceOdd,
                              TPM_BOOL *continueAuthSession,
                              TPM_AUTHDATA authData,
                              unsigned char **command,
                              uint32_t *paramSize);

TPM_RESULT TPM_AuthParams_Set(TPM_STORE_BUFFER *response,
                              TPM_SECRET hmacKey,
                              TPM_AUTH_SESSION_DATA *auth_session_data, 
                              TPM_DIGEST outParamDigest,
                              TPM_NONCE nonceOdd,
                              TPM_BOOL continueAuthSession);

/*
  TPM_CHANGEAUTH_VALIDATE
*/

void       TPM_ChangeauthValidate_Init(TPM_CHANGEAUTH_VALIDATE *tpm_changeauth_validate);
TPM_RESULT TPM_ChangeauthValidate_Load(TPM_CHANGEAUTH_VALIDATE *tpm_changeauth_validate,
                                       unsigned char **stream,
                                       uint32_t *stream_size);
#if 0
TPM_RESULT TPM_ChangeauthValidate_Store(TPM_STORE_BUFFER *sbuffer,
                                        const TPM_CHANGEAUTH_VALIDATE *tpm_changeauth_validate);
#endif
void       TPM_ChangeauthValidate_Delete(TPM_CHANGEAUTH_VALIDATE *tpm_changeauth_validate);

/*
  TPM_DA_INFO
*/

void       TPM_DaInfo_Init(TPM_DA_INFO *tpm_da_info);
TPM_RESULT TPM_DaInfo_Store(TPM_STORE_BUFFER *sbuffer,
                            const TPM_DA_INFO *tpm_da_info);
void       TPM_DaInfo_Delete(TPM_DA_INFO *tpm_da_info);

TPM_RESULT TPM_DaInfo_Set(TPM_DA_INFO *tpm_da_info,
                          tpm_state_t *tpm_state);

/*
  TPM_DA_INFO_LIMITED
*/

void       TPM_DaInfoLimited_Init(TPM_DA_INFO_LIMITED *tpm_da_info_limited);
TPM_RESULT TPM_DaInfoLimited_Store(TPM_STORE_BUFFER *sbuffer,
                                   const TPM_DA_INFO_LIMITED *tpm_da_info_limited);
void       TPM_DaInfoLimited_Delete(TPM_DA_INFO_LIMITED *tpm_da_info_limited);

TPM_RESULT TPM_DaInfoLimited_Set(TPM_DA_INFO_LIMITED *tpm_da_info_limited,
                                 tpm_state_t *tpm_state);

/*
  Processing functions
*/

TPM_RESULT TPM_Process_ChangeAuth(tpm_state_t *tpm_state,
                                  TPM_STORE_BUFFER *response,
                                  TPM_TAG tag,
                                  uint32_t paramSize,
                                  TPM_COMMAND_CODE ordinal,
                                  unsigned char *command,
                                  TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_ChangeAuthOwner(tpm_state_t *tpm_state,
                                       TPM_STORE_BUFFER *response,
                                       TPM_TAG tag,
                                       uint32_t paramSize,
                                       TPM_COMMAND_CODE ordinal,
                                       unsigned char *command,
                                       TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_ChangeAuthAsymStart(tpm_state_t *tpm_state,
                                           TPM_STORE_BUFFER *response,
                                           TPM_TAG tag,
                                           uint32_t paramSize,
                                           TPM_COMMAND_CODE ordinal,
                                           unsigned char *command,
                                           TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_ChangeAuthAsymFinish(tpm_state_t *tpm_state,
                                            TPM_STORE_BUFFER *response,
                                            TPM_TAG tag,
                                            uint32_t paramSize,
                                            TPM_COMMAND_CODE ordinal,
                                            unsigned char *command,
                                            TPM_TRANSPORT_INTERNAL *transportInternal);

#endif
