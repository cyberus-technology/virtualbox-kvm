/********************************************************************************/
/*                                                                              */
/*                              TPM Sessions Handler                            */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_session.h $           */
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

#ifndef TPM_SESSION_H
#define TPM_SESSION_H

#include "tpm_global.h"
#include "tpm_store.h"
#include "tpm_types.h"

/*
  TPM_AUTH_SESSION_DATA (the entire array)
*/

void       TPM_AuthSessions_Init(TPM_AUTH_SESSION_DATA *authSessions);
TPM_RESULT TPM_AuthSessions_Load(TPM_AUTH_SESSION_DATA *authSessions,
                                 unsigned char **stream,
                                 uint32_t *stream_size);
TPM_RESULT TPM_AuthSessions_Store(TPM_STORE_BUFFER *sbuffer,
                                  TPM_AUTH_SESSION_DATA *authSessions);
void       TPM_AuthSessions_Delete(TPM_AUTH_SESSION_DATA *authSessions);


void       TPM_AuthSessions_IsSpace(TPM_BOOL *isSpace, uint32_t *index,
                                    TPM_AUTH_SESSION_DATA *authSessions);
void       TPM_AuthSessions_Trace(TPM_AUTH_SESSION_DATA *authSessions);
void       TPM_AuthSessions_GetSpace(uint32_t *space,
                                     TPM_AUTH_SESSION_DATA *authSessions);
TPM_RESULT TPM_AuthSessions_StoreHandles(TPM_STORE_BUFFER *sbuffer,
                                         TPM_AUTH_SESSION_DATA *authSessions);
TPM_RESULT TPM_AuthSessions_GetNewHandle(TPM_AUTH_SESSION_DATA **tpm_auth_session_data,
                                         TPM_AUTHHANDLE *authHandle,
                                         TPM_AUTH_SESSION_DATA *authSessions);
TPM_RESULT TPM_AuthSessions_GetEntry(TPM_AUTH_SESSION_DATA **tpm_auth_session_data,
                                     TPM_AUTH_SESSION_DATA *authSessions,
                                     TPM_AUTHHANDLE authHandle);
TPM_RESULT TPM_AuthSessions_AddEntry(TPM_HANDLE *tpm_handle,
                                     TPM_BOOL keepHandle,
                                     TPM_AUTH_SESSION_DATA *authSessions,
                                     TPM_AUTH_SESSION_DATA *tpm_auth_session_data);
TPM_RESULT TPM_AuthSessions_GetData(TPM_AUTH_SESSION_DATA **tpm_auth_session_data,
                                    TPM_SECRET **hmacKey,
                                    tpm_state_t *tpm_state,
                                    TPM_AUTHHANDLE authHandle,  
                                    TPM_PROTOCOL_ID protocolID,
                                    TPM_ENT_TYPE entityType,
                                    TPM_COMMAND_CODE ordinal,
                                    TPM_KEY *tpmKey,
                                    TPM_SECRET *entityAuth,
                                    TPM_DIGEST entityDigest);

TPM_RESULT TPM_AuthSessions_TerminateHandle(TPM_AUTH_SESSION_DATA *authSessions,
                                            TPM_AUTHHANDLE authHandle);
void       TPM_AuthSessions_TerminateEntity(TPM_BOOL *continueAuthSession,
                                            TPM_AUTHHANDLE authHandle,
                                            TPM_AUTH_SESSION_DATA *authSessions,
                                            TPM_ENT_TYPE entityType,
                                            TPM_DIGEST *entityDigest);
void       TPM_AuthSessions_TerminatexSAP(TPM_BOOL *continueAuthSession,
                                          TPM_AUTHHANDLE authHandle,
                                          TPM_AUTH_SESSION_DATA *authSessions);

/*
  TPM_AUTH_SESSION_DATA (one element of the array)
*/


void       TPM_AuthSessionData_Init(TPM_AUTH_SESSION_DATA *tpm_auth_session_data);
TPM_RESULT TPM_AuthSessionData_Load(TPM_AUTH_SESSION_DATA *tpm_auth_session_data,
                                    unsigned char **stream,
                                    uint32_t *stream_size);
TPM_RESULT TPM_AuthSessionData_Store(TPM_STORE_BUFFER *sbuffer,
                                     const TPM_AUTH_SESSION_DATA *tpm_auth_session_data);
void       TPM_AuthSessionData_Delete(TPM_AUTH_SESSION_DATA *tpm_auth_session_data);


void       TPM_AuthSessionData_Copy(TPM_AUTH_SESSION_DATA *dest_auth_session_data,
                                    TPM_HANDLE tpm_handle,
                                    TPM_AUTH_SESSION_DATA *src_auth_session_data);
TPM_RESULT TPM_AuthSessionData_GetDelegatePublic(TPM_DELEGATE_PUBLIC **delegatePublic,  
                                                 TPM_AUTH_SESSION_DATA *auth_session_data);
TPM_RESULT TPM_AuthSessionData_CheckEncScheme(TPM_ADIP_ENC_SCHEME adipEncScheme,
                                              TPM_BOOL FIPS);
TPM_RESULT TPM_AuthSessionData_Decrypt(TPM_DIGEST a1Even,
                                       TPM_DIGEST a1Odd,
                                       TPM_ENCAUTH encAuthEven,
                                       TPM_AUTH_SESSION_DATA *tpm_auth_session_data,
                                       TPM_NONCE nonceOdd,
                                       TPM_ENCAUTH encAuthOdd,
                                       TPM_BOOL odd);

/*
  Context List
*/

void       TPM_ContextList_Init(uint32_t *contextList);
TPM_RESULT TPM_ContextList_Load(uint32_t *contextList,
                                unsigned char **stream,
                                uint32_t *stream_size);
TPM_RESULT TPM_ContextList_Store(TPM_STORE_BUFFER *sbuffer,
                                 const uint32_t *contextList);

TPM_RESULT TPM_ContextList_StoreHandles(TPM_STORE_BUFFER *sbuffer,
                                        const uint32_t *contextList);
void       TPM_ContextList_GetSpace(uint32_t *space,
                                    uint32_t *entry,
                                    const uint32_t *contextList);
TPM_RESULT TPM_ContextList_GetEntry(uint32_t *entry,
                                    const uint32_t *contextList,
                                    uint32_t value);

/*
  TPM_CONTEXT_BLOB
*/

void       TPM_ContextBlob_Init(TPM_CONTEXT_BLOB *tpm_context_blob);
TPM_RESULT TPM_ContextBlob_Load(TPM_CONTEXT_BLOB *tpm_context_blob,
                                unsigned char **stream,
                                uint32_t *stream_size);
TPM_RESULT TPM_ContextBlob_Store(TPM_STORE_BUFFER *sbuffer,
                                 const TPM_CONTEXT_BLOB *tpm_context_blob);
void       TPM_ContextBlob_Delete(TPM_CONTEXT_BLOB *tpm_context_blob);

/*
  TPM_CONTEXT_SENSITIVE
*/

void       TPM_ContextSensitive_Init(TPM_CONTEXT_SENSITIVE *tpm_context_sensitive);
TPM_RESULT TPM_ContextSensitive_Load(TPM_CONTEXT_SENSITIVE *tpm_context_sensitive,
                                     unsigned char **stream,
                                     uint32_t *stream_size);
TPM_RESULT TPM_ContextSensitive_Store(TPM_STORE_BUFFER *sbuffer,
                                      const TPM_CONTEXT_SENSITIVE *tpm_context_sensitive);
void       TPM_ContextSensitive_Delete(TPM_CONTEXT_SENSITIVE *tpm_context_sensitive);

/*
  Processing Functions
*/

TPM_RESULT TPM_Process_OIAP(tpm_state_t *tpm_state,
                            TPM_STORE_BUFFER *response,
                            TPM_TAG tag,
                            uint32_t paramSize,
                            TPM_COMMAND_CODE ordinal,
                            unsigned char *command,
                            TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_OSAP(tpm_state_t *tpm_state,
                            TPM_STORE_BUFFER *response,
                            TPM_TAG tag,
                            uint32_t paramSize,
                            TPM_COMMAND_CODE ordinal,
                            unsigned char *command,
                            TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_DSAP(tpm_state_t *tpm_state,
                            TPM_STORE_BUFFER *response,
                            TPM_TAG tag,
                            uint32_t paramSize,
                            TPM_COMMAND_CODE ordinal,
                            unsigned char *command,
                            TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_SetOwnerPointer(tpm_state_t *tpm_state,
                                       TPM_STORE_BUFFER *response,
                                       TPM_TAG tag,
                                       uint32_t paramSize,
                                       TPM_COMMAND_CODE ordinal,
                                       unsigned char *command,
                                       TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_TerminateHandle(tpm_state_t *tpm_state,
                                       TPM_STORE_BUFFER *response,
                                       TPM_TAG tag,
                                       uint32_t paramSize,
                                       TPM_COMMAND_CODE ordinal,
                                       unsigned char *command,
                                       TPM_TRANSPORT_INTERNAL *transportInternal);


TPM_RESULT TPM_Process_FlushSpecific(tpm_state_t *tpm_state,
                                     TPM_STORE_BUFFER *response,
                                     TPM_TAG tag,
                                     uint32_t paramSize,
                                     TPM_COMMAND_CODE ordinal,
                                     unsigned char *command,
                                     TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_SaveContext(tpm_state_t *tpm_state,
                                   TPM_STORE_BUFFER *response,
                                   TPM_TAG tag,
                                   uint32_t paramSize,
                                   TPM_COMMAND_CODE ordinal,
                                   unsigned char *command,
                                   TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_LoadContext(tpm_state_t *tpm_state,
                                   TPM_STORE_BUFFER *response,
                                   TPM_TAG tag,
                                   uint32_t paramSize,
                                   TPM_COMMAND_CODE ordinal,
                                   unsigned char *command,
                                   TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_KeyControlOwner(tpm_state_t *tpm_state,
                                       TPM_STORE_BUFFER *response,
                                       TPM_TAG tag,
                                       uint32_t paramSize,
                                       TPM_COMMAND_CODE ordinal,
                                       unsigned char *command,
                                       TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_SaveKeyContext(tpm_state_t *tpm_state,
                                      TPM_STORE_BUFFER *response,
                                      TPM_TAG tag,
                                      uint32_t paramSize,
                                      TPM_COMMAND_CODE ordinal,
                                      unsigned char *command,
                                      TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_LoadKeyContext(tpm_state_t *tpm_state,
                                      TPM_STORE_BUFFER *response,
                                      TPM_TAG tag,
                                      uint32_t paramSize,
                                      TPM_COMMAND_CODE ordinal,
                                      unsigned char *command,
                                      TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_SaveAuthContext(tpm_state_t *tpm_state,
                                       TPM_STORE_BUFFER *response,
                                       TPM_TAG tag,
                                       uint32_t paramSize,
                                       TPM_COMMAND_CODE ordinal,
                                       unsigned char *command,
                                       TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_LoadAuthContext(tpm_state_t *tpm_state,
                                       TPM_STORE_BUFFER *response,
                                       TPM_TAG tag,
                                       uint32_t paramSize,
                                       TPM_COMMAND_CODE ordinal,
                                       unsigned char *command,
                                       TPM_TRANSPORT_INTERNAL *transportInternal);






#endif
