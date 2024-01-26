/********************************************************************************/
/*                                                                              */
/*                          TPM Admin Test and Opt-in                           */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_admin.h $             */
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

#ifndef TPM_ADMIN_H
#define TPM_ADMIN_H

#include "tpm_types.h"
#include "tpm_global.h"
#include "tpm_store.h"

TPM_RESULT TPM_LimitedSelfTestCommon(void);
TPM_RESULT TPM_LimitedSelfTestTPM(tpm_state_t *tpm_state);

TPM_RESULT TPM_ContinueSelfTestCmd(tpm_state_t *tpm_state);
TPM_RESULT TPM_SelfTestFullCmd(tpm_state_t *tpm_state);

TPM_RESULT TPM_Process_ContinueSelfTest(tpm_state_t *tpm_state,
                                        TPM_STORE_BUFFER *response,
                                        TPM_TAG tag,
                                        uint32_t paramSize,
                                        TPM_COMMAND_CODE ordinal,
                                        unsigned char *command,
                                        TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_SelfTestFull(tpm_state_t *tpm_state,
                                    TPM_STORE_BUFFER *response,
                                    TPM_TAG tag,
                                    uint32_t paramSize,
                                    TPM_COMMAND_CODE ordinal,
                                    unsigned char *command,
                                    TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_GetTestResult(tpm_state_t *tpm_state,
                                     TPM_STORE_BUFFER *response,
                                     TPM_TAG tag,
                                     uint32_t paramSize,
                                     TPM_COMMAND_CODE ordinal,
                                     unsigned char *command,
                                     TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_SetOwnerInstall(tpm_state_t *tpm_state,
                                       TPM_STORE_BUFFER *response,
                                       TPM_TAG tag,
                                       uint32_t paramSize,
                                       TPM_COMMAND_CODE ordinal,
                                       unsigned char *command,
                                       TPM_TRANSPORT_INTERNAL *transportInternal);
            
TPM_RESULT TPM_Process_OwnerSetDisable(tpm_state_t *tpm_state,
                                       TPM_STORE_BUFFER *response,
                                       TPM_TAG tag,
                                       uint32_t paramSize,
                                       TPM_COMMAND_CODE ordinal,
                                       unsigned char *command,
                                       TPM_TRANSPORT_INTERNAL *transportInternal);
            
TPM_RESULT TPM_Process_PhysicalDisable(tpm_state_t *tpm_state,
                                       TPM_STORE_BUFFER *response,
                                       TPM_TAG tag,
                                       uint32_t paramSize,
                                       TPM_COMMAND_CODE ordinal,
                                       unsigned char *command,
                                       TPM_TRANSPORT_INTERNAL *transportInternal);
            
TPM_RESULT TPM_Process_PhysicalEnable(tpm_state_t *tpm_state,
                                      TPM_STORE_BUFFER *response,
                                      TPM_TAG tag,
                                      uint32_t paramSize,
                                      TPM_COMMAND_CODE ordinal,
                                      unsigned char *command,
                                      TPM_TRANSPORT_INTERNAL *transportInternal);
            
TPM_RESULT TPM_Process_PhysicalSetDeactivated(tpm_state_t *tpm_state,
                                              TPM_STORE_BUFFER *response,
                                              TPM_TAG tag,
                                              uint32_t paramSize,
                                              TPM_COMMAND_CODE ordinal,
                                              unsigned char *command,
                                              TPM_TRANSPORT_INTERNAL *transportInternal);
            
TPM_RESULT TPM_Process_SetTempDeactivated(tpm_state_t *tpm_state,
                                          TPM_STORE_BUFFER *response,
                                          TPM_TAG tag,
                                          uint32_t paramSize,
                                          TPM_COMMAND_CODE ordinal,
                                          unsigned char *command,
                                          TPM_TRANSPORT_INTERNAL *transportInternal);
            
TPM_RESULT TPM_Process_SetOperatorAuth(tpm_state_t *tpm_state,
                                       TPM_STORE_BUFFER *response,
                                       TPM_TAG tag,
                                       uint32_t paramSize,
                                       TPM_COMMAND_CODE ordinal,
                                       unsigned char *command,
                                       TPM_TRANSPORT_INTERNAL *transportInternal);
            
TPM_RESULT TPM_Process_ResetLockValue(tpm_state_t *tpm_state,
                                      TPM_STORE_BUFFER *response,
                                      TPM_TAG tag,
                                      uint32_t paramSize,
                                      TPM_COMMAND_CODE ordinal,
                                      unsigned char *command,
                                      TPM_TRANSPORT_INTERNAL *transportInternal);

#endif
