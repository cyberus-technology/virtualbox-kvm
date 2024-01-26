/********************************************************************************/
/*                                                                              */
/*                              Audit Handler                                   */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_audit.h $             */
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

#ifndef TPM_AUDIT_H
#define TPM_AUDIT_H

#include "tpm_global.h"
#include "tpm_store.h"
#include "tpm_structures.h"

/*
  TPM_AUDIT_EVENT_IN
*/

void       TPM_AuditEventIn_Init(TPM_AUDIT_EVENT_IN *tpm_audit_event_in);
TPM_RESULT TPM_AuditEventIn_Store(TPM_STORE_BUFFER *sbuffer,
                                  const TPM_AUDIT_EVENT_IN *tpm_audit_event_in);
void       TPM_AuditEventIn_Delete(TPM_AUDIT_EVENT_IN *tpm_audit_event_in);

/*
  TPM_AUDIT_EVENT_OUT
*/

void       TPM_AuditEventOut_Init(TPM_AUDIT_EVENT_OUT *tpm_audit_event_out);
TPM_RESULT TPM_AuditEventOut_Store(TPM_STORE_BUFFER *sbuffer,
                                   const TPM_AUDIT_EVENT_OUT *tpm_audit_event_out);
void       TPM_AuditEventOut_Delete(TPM_AUDIT_EVENT_OUT *tpm_audit_event_out);

/*
  ordinalAuditStatus Processing
*/

TPM_RESULT TPM_OrdinalAuditStatus_Init(TPM_PERMANENT_DATA *tpm_permanent_data);
TPM_RESULT TPM_OrdinalAuditStatus_Store(TPM_SIZED_BUFFER *ordinalList,
                                        TPM_PERMANENT_DATA *tpm_permanent_data,
                                        TPM_COMMAND_CODE startOrdinal);
TPM_RESULT TPM_OrdinalAuditStatus_GetAuditStatus(TPM_BOOL *auditStatus,
                                                 TPM_COMMAND_CODE ordinal,
                                                 TPM_PERMANENT_DATA *tpm_permanent_data);
TPM_RESULT TPM_OrdinalAuditStatus_SetAuditStatus(TPM_BOOL *altered,
						 TPM_PERMANENT_DATA *tpm_permanent_data,
                                                 TPM_BOOL auditStatus,
                                                 TPM_COMMAND_CODE ordinal);

/*
  Common Processing Functions
*/

TPM_RESULT TPM_AuditDigest_ExtendIn(tpm_state_t *tpm_state,
                                    TPM_DIGEST inParamDigest);
TPM_RESULT TPM_AuditDigest_ExtendOut(tpm_state_t *tpm_state,
                                     TPM_DIGEST outParamDigest);

/*
  Processing Functions
*/

TPM_RESULT TPM_Process_GetAuditDigest(tpm_state_t *tpm_state,
                                      TPM_STORE_BUFFER *response,
                                      TPM_TAG tag,
                                      uint32_t paramSize,
                                      TPM_COMMAND_CODE ordinal,
                                      unsigned char *command,
                                      TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_GetAuditDigestSigned(tpm_state_t *tpm_state,
                                            TPM_STORE_BUFFER *response,
                                            TPM_TAG tag,
                                            uint32_t paramSize,
                                            TPM_COMMAND_CODE ordinal,
                                            unsigned char *command,
                                            TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_SetOrdinalAuditStatus(tpm_state_t *tpm_state,
                                             TPM_STORE_BUFFER *response,
                                             TPM_TAG tag,
                                             uint32_t paramSize,
                                             TPM_COMMAND_CODE ordinal,
                                             unsigned char *command,
                                             TPM_TRANSPORT_INTERNAL *transportInternal);


#endif
