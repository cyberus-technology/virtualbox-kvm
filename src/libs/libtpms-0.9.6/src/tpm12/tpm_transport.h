/********************************************************************************/
/*                                                                              */
/*                              Transport                                       */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_transport.h $		*/
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

#ifndef TPM_TRANSPORT_H
#define TPM_TRANSPORT_H

#include "tpm_global.h"

/*
  Transport Encryption for wrapped commands and responses
*/

TPM_RESULT TPM_Transport_CryptMgf1(unsigned char *dest,
                                   const unsigned char *src,
                                   const unsigned char *pad,
                                   uint32_t size,
                                   uint32_t index,
                                   uint32_t len);

TPM_RESULT TPM_Transport_CryptSymmetric(unsigned char *dest,
                                        const unsigned char *src,
                                        TPM_ALGORITHM_ID algId,
                                        TPM_ENC_SCHEME encScheme,
                                        const unsigned char *symmetric_key,
                                        uint32_t symmetric_key_size,
                                        unsigned char *pad_in,
                                        uint32_t pad_in_size,
                                        uint32_t size,
                                        uint32_t index,
                                        uint32_t len);

/*
  Transport Sessions (the entire array)
*/

void       TPM_TransportSessions_Init(TPM_TRANSPORT_INTERNAL *transSessions);
TPM_RESULT TPM_TransportSessions_Load(TPM_TRANSPORT_INTERNAL *transSessions,
                                      unsigned char **stream,
                                      uint32_t *stream_size);
TPM_RESULT TPM_TransportSessions_Store(TPM_STORE_BUFFER *sbuffer,
                                       TPM_TRANSPORT_INTERNAL *transSessions);
void       TPM_TransportSessions_Delete(TPM_TRANSPORT_INTERNAL *transSessions);

void       TPM_TransportSessions_IsSpace(TPM_BOOL *isSpace, uint32_t *index,
                                         TPM_TRANSPORT_INTERNAL *transSessions);
void       TPM_TransportSessions_GetSpace(uint32_t *space,
                                          TPM_TRANSPORT_INTERNAL *transSessions);
TPM_RESULT TPM_TransportSessions_StoreHandles(TPM_STORE_BUFFER *sbuffer,
                                              TPM_TRANSPORT_INTERNAL *transSessions);
TPM_RESULT TPM_TransportSessions_GetNewHandle(TPM_TRANSPORT_INTERNAL **tpm_transport_internal,
                                              TPM_TRANSPORT_INTERNAL *transportSessions);
TPM_RESULT TPM_TransportSessions_GetEntry(TPM_TRANSPORT_INTERNAL **tpm_transport_internal ,
                                          TPM_TRANSPORT_INTERNAL *transportSessions,
                                          TPM_TRANSHANDLE transportHandle);
TPM_RESULT TPM_TransportSessions_AddEntry(TPM_HANDLE *tpm_handle,
                                          TPM_BOOL keepHandle,
                                          TPM_TRANSPORT_INTERNAL *transSessions,
                                          TPM_TRANSPORT_INTERNAL *tpm_transport_internal);
TPM_RESULT TPM_TransportSessions_TerminateHandle(TPM_TRANSPORT_INTERNAL *tpm_transport_internal,
                                                 TPM_TRANSHANDLE transportHandle,
                                                 TPM_TRANSHANDLE *transportExclusive);

/*
  TPM_TRANSPORT_PUBLIC
*/

void       TPM_TransportPublic_Init(TPM_TRANSPORT_PUBLIC *tpm_transport_public);
TPM_RESULT TPM_TransportPublic_Load(TPM_TRANSPORT_PUBLIC *tpm_transport_public,
                                    unsigned char **stream,
                                    uint32_t *stream_size);
TPM_RESULT TPM_TransportPublic_Store(TPM_STORE_BUFFER *sbuffer,
                                     const TPM_TRANSPORT_PUBLIC *tpm_transport_public);
void       TPM_TransportPublic_Delete(TPM_TRANSPORT_PUBLIC *tpm_transport_public);

TPM_RESULT TPM_TransportPublic_Copy(TPM_TRANSPORT_PUBLIC *dest,
                                     const TPM_TRANSPORT_PUBLIC *src);
void       TPM_TransportPublic_CheckAlgId(TPM_BOOL *supported,
                                          TPM_ALGORITHM_ID algId);
TPM_RESULT TPM_TransportPublic_CheckEncScheme(uint32_t *blockSize,
                                              TPM_ALGORITHM_ID algId,
                                              TPM_ENC_SCHEME encScheme,
                                              TPM_BOOL FIPS);

/*
  TPM_TRANSPORT_INTERNAL
*/

void       TPM_TransportInternal_Init(TPM_TRANSPORT_INTERNAL *tpm_transport_internal);
TPM_RESULT TPM_TransportInternal_Load(TPM_TRANSPORT_INTERNAL *tpm_transport_internal,
                                      unsigned char **stream,
                                      uint32_t *stream_size);
TPM_RESULT TPM_TransportInternal_Store(TPM_STORE_BUFFER *sbuffer,
                                       const TPM_TRANSPORT_INTERNAL *tpm_transport_internal);
void       TPM_TransportInternal_Delete(TPM_TRANSPORT_INTERNAL *tpm_transport_internal);

void TPM_TransportInternal_Copy(TPM_TRANSPORT_INTERNAL *dest_transport_internal,
                                TPM_TRANSPORT_INTERNAL *src_transport_internal);
TPM_RESULT TPM_TransportInternal_Check(TPM_DIGEST               inParamDigest,
                                       TPM_TRANSPORT_INTERNAL   *tpm_transport_internal,
                                       TPM_NONCE                transNonceOdd,
                                       TPM_BOOL                         continueTransSession,
                                       TPM_AUTHDATA             transAuth);
TPM_RESULT TPM_TransportInternal_Set(TPM_STORE_BUFFER           *response,
                                     TPM_TRANSPORT_INTERNAL     *tpm_transport_internal,
                                     TPM_DIGEST                 outParamDigest,
                                     TPM_NONCE                  transNonceOdd,
                                     TPM_BOOL                   continueTransSession,
                                     TPM_BOOL                   generateNonceEven);

/*
  TPM_TRANSPORT_LOG_IN
*/

void       TPM_TransportLogIn_Init(TPM_TRANSPORT_LOG_IN *tpm_transport_log_in);
TPM_RESULT TPM_TransportLogIn_Store(TPM_STORE_BUFFER *sbuffer,
                                    const TPM_TRANSPORT_LOG_IN *tpm_transport_log_in);
void       TPM_TransportLogIn_Delete(TPM_TRANSPORT_LOG_IN *tpm_transport_log_in);

TPM_RESULT TPM_TransportLogIn_Extend(TPM_DIGEST tpm_digest,
                                     TPM_TRANSPORT_LOG_IN *tpm_transport_log_in);

/*
  TPM_TRANSPORT_LOG_OUT
*/

void       TPM_TransportLogOut_Init(TPM_TRANSPORT_LOG_OUT *tpm_transport_log_out);
TPM_RESULT TPM_TransportLogOut_Store(TPM_STORE_BUFFER *sbuffer,
                                     const TPM_TRANSPORT_LOG_OUT *tpm_transport_log_out);
void       TPM_TransportLogOut_Delete(TPM_TRANSPORT_LOG_OUT *tpm_transport_log_out);

TPM_RESULT TPM_TransportLogOut_Extend(TPM_DIGEST tpm_digest,
                                      TPM_TRANSPORT_LOG_OUT *tpm_transport_log_out);

/*
  TPM_TRANSPORT_AUTH
*/

void       TPM_TransportAuth_Init(TPM_TRANSPORT_AUTH *tpm_transport_auth);
TPM_RESULT TPM_TransportAuth_Load(TPM_TRANSPORT_AUTH *tpm_transport_auth,
                                  unsigned char **stream,
                                  uint32_t *stream_size);
TPM_RESULT TPM_TransportAuth_Store(TPM_STORE_BUFFER *sbuffer,
                                   const TPM_TRANSPORT_AUTH *tpm_transport_auth);
void       TPM_TransportAuth_Delete(TPM_TRANSPORT_AUTH *tpm_transport_auth);

TPM_RESULT TPM_TransportAuth_DecryptSecret(TPM_TRANSPORT_AUTH *tpm_transport_auth,
                                           TPM_SIZED_BUFFER *secret,
                                           TPM_KEY *tpm_key);

/* Command Processing Functions */

TPM_RESULT TPM_Process_EstablishTransport(tpm_state_t *tpm_state,
                                          TPM_STORE_BUFFER *response,
                                          TPM_TAG tag,
                                          uint32_t paramSize,
                                          TPM_COMMAND_CODE ordinal,
                                          unsigned char *command,
                                          TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_ExecuteTransport(tpm_state_t *tpm_state,
                                        TPM_STORE_BUFFER *response,
                                        TPM_TAG tag,
                                        uint32_t paramSize,
                                        TPM_COMMAND_CODE ordinal,
                                        unsigned char *command,
                                        TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_ReleaseTransportSigned(tpm_state_t *tpm_state,
                                              TPM_STORE_BUFFER *response,
                                              TPM_TAG tag,
                                              uint32_t paramSize,
                                              TPM_COMMAND_CODE ordinal,
                                              unsigned char *command,
                                              TPM_TRANSPORT_INTERNAL *transportInternal);


#endif
