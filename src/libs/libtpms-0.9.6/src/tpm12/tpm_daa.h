/********************************************************************************/
/*                                                                              */
/*                              DAA Functions                                   */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_daa.h $               */
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

#ifndef TPM_DAA_H
#define TPM_DAA_H

#include "tpm_global.h"
#include "tpm_store.h"

/*
  TPM_DAA_SESSION_DATA  (the entire array)
*/


void       TPM_DaaSessions_Init(TPM_DAA_SESSION_DATA *daaSessions);
TPM_RESULT TPM_DaaSessions_Load(TPM_DAA_SESSION_DATA *daaSessions,
                                unsigned char **stream,
                                uint32_t *stream_size);
TPM_RESULT TPM_DaaSessions_Store(TPM_STORE_BUFFER *sbuffer,
                                 TPM_DAA_SESSION_DATA *daaSessions);
void       TPM_DaaSessions_Delete(TPM_DAA_SESSION_DATA *daaSessions);

void       TPM_DaaSessions_IsSpace(TPM_BOOL *isSpace,
                                   uint32_t *index,
                                   TPM_DAA_SESSION_DATA *daaSessions);
void       TPM_DaaSessions_GetSpace(uint32_t *space,
                                    TPM_DAA_SESSION_DATA *daaSessions);
TPM_RESULT TPM_DaaSessions_StoreHandles(TPM_STORE_BUFFER *sbuffer,
                                        TPM_DAA_SESSION_DATA *daaSessions);
TPM_RESULT TPM_DaaSessions_GetNewHandle(TPM_DAA_SESSION_DATA **tpm_daa_session_data,
                                        TPM_HANDLE *daaHandle,
                                        TPM_BOOL *daaHandleValid,
                                        TPM_DAA_SESSION_DATA *daaSessions);
TPM_RESULT TPM_DaaSessions_GetEntry(TPM_DAA_SESSION_DATA **tpm_daa_session_data,
                                    TPM_DAA_SESSION_DATA *daaSessions,
                                    TPM_HANDLE daaHandle);
TPM_RESULT TPM_DaaSessions_AddEntry(TPM_HANDLE *tpm_handle,
                                    TPM_BOOL keepHandle,
                                    TPM_DAA_SESSION_DATA *daaSessions,
                                    TPM_DAA_SESSION_DATA *tpm_daa_session_data);
TPM_RESULT TPM_DaaSessions_TerminateHandle(TPM_DAA_SESSION_DATA *daaSessions,
                                           TPM_HANDLE daaHandle);


/*
  TPM_DAA_SESSION_DATA (one element of the array)
*/

void       TPM_DaaSessionData_Init(TPM_DAA_SESSION_DATA *tpm_daa_session_data);
TPM_RESULT TPM_DaaSessionData_Load(TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                                   unsigned char **stream,
                                   uint32_t *stream_size);
TPM_RESULT TPM_DaaSessionData_Store(TPM_STORE_BUFFER *sbuffer,
                                    const TPM_DAA_SESSION_DATA *tpm_daa_session_data);
void       TPM_DaaSessionData_Delete(TPM_DAA_SESSION_DATA *tpm_daa_session_data);

void       TPM_DaaSessionData_Copy(TPM_DAA_SESSION_DATA *dest_daa_session_data,
                                   TPM_HANDLE tpm_handle,
                                   TPM_DAA_SESSION_DATA *src_daa_session_data);
TPM_RESULT TPM_DaaSessionData_CheckStage(TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                                         BYTE stage);

/*
  TPM_DAA_ISSUER
*/

void       TPM_DAAIssuer_Init(TPM_DAA_ISSUER *tpm_daa_issuer);
TPM_RESULT TPM_DAAIssuer_Load(TPM_DAA_ISSUER *tpm_daa_issuer,
                              unsigned char **stream,
                              uint32_t *stream_size);
TPM_RESULT TPM_DAAIssuer_Store(TPM_STORE_BUFFER *sbuffer,
                               const TPM_DAA_ISSUER *tpm_daa_issuer);
void       TPM_DAAIssuer_Delete(TPM_DAA_ISSUER *tpm_daa_issuer);

void       TPM_DAAIssuer_Copy(TPM_DAA_ISSUER *dest_daa_issuer,
                              TPM_DAA_ISSUER *src_daa_issuer);

/*
  TPM_DAA_TPM
*/

void       TPM_DAATpm_Init(TPM_DAA_TPM *tpm_daa_tpm);
TPM_RESULT TPM_DAATpm_Load(TPM_DAA_TPM *tpm_daa_tpm,
                           unsigned char **stream,
                           uint32_t *stream_size);
TPM_RESULT TPM_DAATpm_Store(TPM_STORE_BUFFER *sbuffer,
                            const TPM_DAA_TPM *tpm_daa_tpm);
void       TPM_DAATpm_Delete(TPM_DAA_TPM *tpm_daa_tpm);

void       TPM_DAATpm_Copy(TPM_DAA_TPM *dest_daa_tpm, TPM_DAA_TPM *src_daa_tpm);

/*
  TPM_DAA_CONTEXT
*/

void       TPM_DAAContext_Init(TPM_DAA_CONTEXT *tpm_daa_context);
TPM_RESULT TPM_DAAContext_Load(TPM_DAA_CONTEXT *tpm_daa_context,
                               unsigned char **stream,
                               uint32_t *stream_size);
TPM_RESULT TPM_DAAContext_Store(TPM_STORE_BUFFER *sbuffer,
                                const TPM_DAA_CONTEXT *tpm_daa_context);
void       TPM_DAAContext_Delete(TPM_DAA_CONTEXT *tpm_daa_context);

void       TPM_DAAContext_Copy(TPM_DAA_CONTEXT *dest_daa_context, TPM_DAA_CONTEXT *src_daa_context);

/*
  TPM_DAA_JOINDATA
*/

void       TPM_DAAJoindata_Init(TPM_DAA_JOINDATA *tpm_daa_joindata);
TPM_RESULT TPM_DAAJoindata_Load(TPM_DAA_JOINDATA *tpm_daa_joindata,
                                unsigned char **stream,
                                uint32_t *stream_size);
TPM_RESULT TPM_DAAJoindata_Store(TPM_STORE_BUFFER *sbuffer,
                                 const TPM_DAA_JOINDATA *tpm_daa_joindata);
void       TPM_DAAJoindata_Delete(TPM_DAA_JOINDATA *tpm_daa_joindata);

void       TPM_DAAJoindata_Copy(TPM_DAA_JOINDATA *dest_daa_joindata,
                                TPM_DAA_JOINDATA *src_daa_joindata);

/*
  TPM_DAA_BLOB
*/

void       TPM_DAABlob_Init(TPM_DAA_BLOB *tpm_daa_blob);
TPM_RESULT TPM_DAABlob_Load(TPM_DAA_BLOB *tpm_daa_blob,
                            unsigned char **stream,
                            uint32_t *stream_size);
TPM_RESULT TPM_DAABlob_Store(TPM_STORE_BUFFER *sbuffer,
                             const TPM_DAA_BLOB *tpm_daa_blob);
void       TPM_DAABlob_Delete(TPM_DAA_BLOB *tpm_daa_blob);

/*
  TPM_DAA_SENSITIVE
*/

void       TPM_DAASensitive_Init(TPM_DAA_SENSITIVE *tpm_daa_sensitive);
TPM_RESULT TPM_DAASensitive_Load(TPM_DAA_SENSITIVE *tpm_daa_sensitive,
                                 unsigned char **stream,
                                 uint32_t *stream_size);
TPM_RESULT TPM_DAASensitive_Store(TPM_STORE_BUFFER *sbuffer,
                                  const TPM_DAA_SENSITIVE *tpm_daa_sensitive);
void       TPM_DAASensitive_Delete(TPM_DAA_SENSITIVE *tpm_daa_sensitive);

/*
  Stage Common Code
*/

TPM_RESULT TPM_DAADigestContext_GenerateDigestJoin(TPM_DIGEST tpm_digest,
                                                   TPM_DAA_SESSION_DATA *tpm_daa_session_data);
TPM_RESULT TPM_DAADigestContext_CheckDigestJoin(TPM_DAA_SESSION_DATA *tpm_daa_session_data);

TPM_RESULT TPM_DAASession_CheckStage(TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                                     BYTE stage);
TPM_RESULT TPM_ComputeF(TPM_BIGNUM *fBignum,
                        TPM_DAA_SESSION_DATA *tpm_daa_session_data);
TPM_RESULT TPM_ComputeAexpPmodn(BYTE *DAA_scratch,
                                uint32_t DAA_scratch_size,
                                TPM_BIGNUM *rBignum,
                                TPM_BIGNUM xBignum,
                                TPM_BIGNUM fBignum,
                                TPM_BIGNUM nBignum);
TPM_RESULT TPM_ComputeZxAexpPmodn(BYTE *DAA_scratch,
                                  uint32_t DAA_scratch_size,
                                  TPM_BIGNUM zBignum,
                                  TPM_BIGNUM aBignum,
                                  TPM_BIGNUM pBignum,
                                  TPM_BIGNUM nBignum);
TPM_RESULT TPM_ComputeApBmodn(TPM_BIGNUM *rBignum,
                              TPM_BIGNUM aBignum,
                              TPM_BIGNUM bBignum,
                              TPM_BIGNUM nBignum);
TPM_RESULT TPM_ComputeApBxC(TPM_BIGNUM *rBignum,
                            TPM_BIGNUM aBignum,
                            TPM_BIGNUM bBignum,
                            TPM_BIGNUM cBignum);
TPM_RESULT TPM_ComputeApBxCpD(TPM_BIGNUM *rBignum,
                              TPM_BIGNUM aBignum,
                              TPM_BIGNUM bBignum,
                              TPM_BIGNUM cBignum,
                              TPM_BIGNUM dBignum);
TPM_RESULT TPM_ComputeDAAScratch(BYTE *DAA_scratch,
                                 uint32_t DAA_scratch_size,
                                 TPM_BIGNUM bn);
TPM_RESULT TPM_ComputeEnlarge(unsigned char **out,
                              uint32_t outSize,
                              unsigned char *in,
                              uint32_t inSize);
TPM_RESULT TPM_SizedBuffer_ComputeEnlarge(TPM_SIZED_BUFFER *tpm_sized_buffer, uint32_t size);
TPM_RESULT TPM_ComputeEncrypt(TPM_SIZED_BUFFER *outputData,
                              tpm_state_t *tpm_state,
                              TPM_DAA_SENSITIVE *tpm_daa_sensitive,
                              TPM_RESOURCE_TYPE resourceType);
TPM_RESULT TPM_ComputeDecrypt(TPM_DAA_SENSITIVE *tpm_daa_sensitive,
                              tpm_state_t *tpm_state,
                              TPM_SIZED_BUFFER *inputData,
                              TPM_RESOURCE_TYPE resourceType);

TPM_RESULT TPM_SHA1_BignumGenerate(TPM_DIGEST tpm_digest,
                                   TPM_BIGNUM bn,
                                   uint32_t size);
TPM_RESULT TPM_SHA1_SizedBufferCheck(TPM_DIGEST tpm_digest,
                                     TPM_SIZED_BUFFER *tpm_sized_buffer,
                                     uint32_t size);

/*
  Processing Common Functions
*/

TPM_RESULT TPM_DAAJoin_Stage00(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA **tpm_daa_session_data,
                               TPM_BOOL *daaHandleValid,
                               TPM_SIZED_BUFFER *outputData,
                               TPM_SIZED_BUFFER *inputData0);
TPM_RESULT TPM_DAAJoin_Stage01(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData,
                               TPM_SIZED_BUFFER *inputData0,
                               TPM_SIZED_BUFFER *inputData1);
TPM_RESULT TPM_DAAJoin_Stage02(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData,
                               TPM_SIZED_BUFFER *inputData0,
                               TPM_SIZED_BUFFER *inputData1);
TPM_RESULT TPM_DAAJoin_Stage03(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData,
                               TPM_SIZED_BUFFER *inputData0);
TPM_RESULT TPM_DAAJoin_Stage04(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData,
                               TPM_SIZED_BUFFER *inputData0,
                               TPM_SIZED_BUFFER *inputData1);
TPM_RESULT TPM_DAAJoin_Stage05(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData,
                               TPM_SIZED_BUFFER *inputData0,
                               TPM_SIZED_BUFFER *inputData1);
TPM_RESULT TPM_DAAJoin_Stage06(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData,
                               TPM_SIZED_BUFFER *inputData0,
                               TPM_SIZED_BUFFER *inputData1);
TPM_RESULT TPM_DAAJoin_Stage07(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData,
                               TPM_SIZED_BUFFER *inputData0,
                               TPM_SIZED_BUFFER *inputData1);
TPM_RESULT TPM_DAAJoin_Stage08(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData,
                               TPM_SIZED_BUFFER *inputData0);
TPM_RESULT TPM_DAAJoin_Stage09_Sign_Stage2(tpm_state_t *tpm_state,
                                           TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                                           TPM_SIZED_BUFFER *outputData,
                                           TPM_SIZED_BUFFER *inputData0,
                                           TPM_SIZED_BUFFER *inputData1);
TPM_RESULT TPM_DAAJoin_Stage10_Sign_Stage3(tpm_state_t *tpm_state,
                                           TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                                           TPM_SIZED_BUFFER *outputData,
                                           TPM_SIZED_BUFFER *inputData0,
                                           TPM_SIZED_BUFFER *inputData1);
TPM_RESULT TPM_DAAJoin_Stage11_Sign_Stage4(tpm_state_t *tpm_state,
                                           TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                                           TPM_SIZED_BUFFER *outputData,
                                           TPM_SIZED_BUFFER *inputData0,
                                           TPM_SIZED_BUFFER *inputData1);
TPM_RESULT TPM_DAAJoin_Stage12(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData,
                               TPM_SIZED_BUFFER *inputData0,
                               TPM_SIZED_BUFFER *inputData1);
TPM_RESULT TPM_DAAJoin_Stage13_Sign_Stage6(tpm_state_t *tpm_state,
                                           TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                                           TPM_SIZED_BUFFER *outputData,
                                           TPM_SIZED_BUFFER *inputData0,
                                           TPM_SIZED_BUFFER *inputData1);
TPM_RESULT TPM_DAAJoin_Stage14_Sign_Stage7(tpm_state_t *tpm_state,
                                           TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                                           TPM_SIZED_BUFFER *outputData,
                                           TPM_SIZED_BUFFER *inputData0);
TPM_RESULT TPM_DAAJoin_Stage15_Sign_Stage8(tpm_state_t *tpm_state,
                                           TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                                           TPM_SIZED_BUFFER *outputData,
                                           TPM_SIZED_BUFFER *inputData0);
TPM_RESULT TPM_DAAJoin_Stage16_Sign_Stage9(tpm_state_t *tpm_state,
                                           TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                                           TPM_SIZED_BUFFER *outputData,
                                           TPM_SIZED_BUFFER *inputData0);
TPM_RESULT TPM_DAAJoin_Stage17_Sign_Stage11(tpm_state_t *tpm_state,
                                            TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                                            TPM_SIZED_BUFFER *outputData);
TPM_RESULT TPM_DAAJoin_Stage18_Sign_Stage12(tpm_state_t *tpm_state,
                                            TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                                            TPM_SIZED_BUFFER *outputData);
TPM_RESULT TPM_DAAJoin_Stage19(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData);
TPM_RESULT TPM_DAAJoin_Stage20(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData);
TPM_RESULT TPM_DAAJoin_Stage21(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData);
TPM_RESULT TPM_DAAJoin_Stage22(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData,
                               TPM_SIZED_BUFFER *inputData0);
TPM_RESULT TPM_DAAJoin_Stage23(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData,
                               TPM_SIZED_BUFFER *inputData0);
TPM_RESULT TPM_DAAJoin_Stage24(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData);

TPM_RESULT TPM_DAASign_Stage00(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA **tpm_daa_session_data,
                               TPM_BOOL *daaHandleValid,
                               TPM_SIZED_BUFFER *outputData,
                               TPM_SIZED_BUFFER *inputData0);
TPM_RESULT TPM_DAASign_Stage01(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData,
                               TPM_SIZED_BUFFER *inputData0);
TPM_RESULT TPM_DAASign_Stage05(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData,
                               TPM_SIZED_BUFFER *inputData0,
                               TPM_SIZED_BUFFER *inputData1);
TPM_RESULT TPM_DAASign_Stage10(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData,
                               TPM_SIZED_BUFFER *inputData0,
                               TPM_SIZED_BUFFER *inputData1);
TPM_RESULT TPM_DAASign_Stage13(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData,
                               TPM_SIZED_BUFFER *inputData0);
TPM_RESULT TPM_DAASign_Stage14(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData,
                               TPM_SIZED_BUFFER *inputData0);
TPM_RESULT TPM_DAASign_Stage15(tpm_state_t *tpm_state,
                               TPM_DAA_SESSION_DATA *tpm_daa_session_data,
                               TPM_SIZED_BUFFER *outputData,
                               TPM_SIZED_BUFFER *inputData0);

/*
  Processing functions
*/

TPM_RESULT TPM_Process_DAAJoin(tpm_state_t *tpm_state,
                               TPM_STORE_BUFFER *response,
                               TPM_TAG tag,
                               uint32_t paramSize,
                               TPM_COMMAND_CODE ordinal,
                               unsigned char *command,
                               TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_DAASign(tpm_state_t *tpm_state,
                               TPM_STORE_BUFFER *response,
                               TPM_TAG tag,
                               uint32_t paramSize,
                               TPM_COMMAND_CODE ordinal,
                               unsigned char *command,
                               TPM_TRANSPORT_INTERNAL *transportInternal);

#endif
