/********************************************************************************/
/*                                                                              */
/*                           TPM Command Processor                              */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_process.h $           */
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

#ifndef TPM_PROCESS_H
#define TPM_PROCESS_H

#include <stdio.h>

/* Commented out.  This is not a standard header.  If needed for a particular platform, replace but
   also add comments and ifdef. */
/* #include <stdint.h> */

#include "tpm_global.h"
#include "tpm_store.h"

/*
  TPM_CAP_VERSION_INFO
*/

void       TPM_CapVersionInfo_Init(TPM_CAP_VERSION_INFO *tpm_cap_version_info);
TPM_RESULT TPM_CapVersionInfo_Load(TPM_CAP_VERSION_INFO *tpm_cap_version_info,
                                   unsigned char **stream,
                                   uint32_t *stream_size);
TPM_RESULT TPM_CapVersionInfo_Store(TPM_STORE_BUFFER *sbuffer,
                                    const TPM_CAP_VERSION_INFO *tpm_cap_version_info);
void       TPM_CapVersionInfo_Delete(TPM_CAP_VERSION_INFO *tpm_cap_version_info);
void       TPM_CapVersionInfo_Set(TPM_CAP_VERSION_INFO *tpm_cap_version_info,
                                  TPM_PERMANENT_DATA *tpm_permanent_data);

/*
  Capability Common Code
*/

void       TPM_SetCapability_Flag(TPM_BOOL *altered,
                                  TPM_BOOL *flag,
                                  TPM_BOOL value);
void       TPM_GetSubCapInt(uint16_t *subCap16,
                            uint32_t *subCap32,
                            TPM_SIZED_BUFFER *subCap);

TPM_RESULT TPM_GetCapabilityCommon(TPM_STORE_BUFFER *capabilityResponse,
                                   tpm_state_t *tpm_state, 
                                   TPM_CAPABILITY_AREA capArea, 
                                   uint16_t subCap16, 
                                   uint32_t subCap32,
                                   TPM_SIZED_BUFFER *subCap);
TPM_RESULT TPM_SetCapabilityCommon(tpm_state_t *tpm_state,
                                   TPM_BOOL ownerAuthorized,
                                   TPM_BOOL presenceAuthorized,
                                   TPM_CAPABILITY_AREA capArea, 
                                   uint16_t subCap16, 
                                   uint32_t subCap32,
                                   TPM_SIZED_BUFFER *subCap,
                                   TPM_SIZED_BUFFER *setValue);

/*
  Processing Functions
*/

TPM_RESULT TPM_ProcessA(unsigned char **response,
			uint32_t *response_size,
			uint32_t *response_total,
			unsigned char *command,
			uint32_t command_size);
TPM_RESULT TPM_Process(TPM_STORE_BUFFER *response,
                       unsigned char *command,
                       uint32_t command_size);
TPM_RESULT TPM_Process_Wrapped(TPM_STORE_BUFFER *response,
                               unsigned char *command,
                               uint32_t command_size,
                               tpm_state_t *targetInstance,
                               TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_GetCommandParams(TPM_TAG *tag,
                                        uint32_t *paramSize ,
                                        TPM_COMMAND_CODE *ordinal,
                                        unsigned char **command,
                                        uint32_t *command_size);
TPM_RESULT TPM_Process_GetResponseParams(TPM_TAG *tag,
                                         uint32_t *paramSize ,
                                         TPM_RESULT *returnCode,
                                         unsigned char **response,
                                         uint32_t *response_size);

TPM_RESULT TPM_Process_Unused(tpm_state_t *tpm_state,
                              TPM_STORE_BUFFER *response,
                              TPM_TAG tag,
                              uint32_t paramSize,
                              TPM_COMMAND_CODE ordinal,
                              unsigned char *command,
                              TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_GetCapability(tpm_state_t *tpm_state,
                                     TPM_STORE_BUFFER *response,
                                     TPM_TAG tag,
                                     uint32_t paramSize,
                                     TPM_COMMAND_CODE ordinal,
                                     unsigned char *command,
                                     TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_GetCapabilityOwner(tpm_state_t *tpm_state,
                                          TPM_STORE_BUFFER *response,
                                          TPM_TAG tag,
                                          uint32_t paramSize,
                                          TPM_COMMAND_CODE ordinal,
                                          unsigned char *command,
                                          TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_GetCapabilitySigned(tpm_state_t *tpm_state,
                                           TPM_STORE_BUFFER *response,
                                           TPM_TAG tag,
                                           uint32_t paramSize,
                                           TPM_COMMAND_CODE ordinal,
                                           unsigned char *command,
                                           TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_SetCapability(tpm_state_t *tpm_state,
                                     TPM_STORE_BUFFER *response,
                                     TPM_TAG tag,
                                     uint32_t paramSize,
                                     TPM_COMMAND_CODE ordinal,
                                     unsigned char *command,
                                     TPM_TRANSPORT_INTERNAL *transportInternal);

/*
  Processing Utilities
*/

/* tag checking */

TPM_RESULT TPM_CheckRequestTag210(TPM_TAG tpm_tag);
TPM_RESULT TPM_CheckRequestTag21 (TPM_TAG tpm_tag);
TPM_RESULT TPM_CheckRequestTag2  (TPM_TAG tpm_tag);
TPM_RESULT TPM_CheckRequestTag10 (TPM_TAG tpm_tag);
TPM_RESULT TPM_CheckRequestTag1  (TPM_TAG tpm_tag);
TPM_RESULT TPM_CheckRequestTag0  (TPM_TAG tpm_tag);

/* TPM state checking */

TPM_RESULT TPM_CheckState(tpm_state_t *tpm_state,
                          TPM_TAG tag,
                          uint32_t tpm_check_map);
TPM_RESULT TPM_Process_Preprocess(tpm_state_t *tpm_state,
                                  TPM_COMMAND_CODE ordinal,
                                  TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Check_SHA1Context(tpm_state_t *tpm_state,
                                 TPM_COMMAND_CODE ordinal,
                                 TPM_TRANSPORT_INTERNAL *transportInternal);

/* ordinal processing */

/* Prototype for all ordinal processing functions

   tpm_state: the entire TPM instance non-volatile and volatile state
   response: the buffer to hold the ordinal response
   tag: the command tag
   paramSize: bytes left after the tag, paramSize, and ordinal
   ordinal: the ordinal being called (could be hard coded, but eliminates cut/paste errors)
   command: the remainder of the command packet
   transportInternal: if not NULL, indicates that this function was called recursively from
	TPM_ExecuteTransport
*/

typedef TPM_RESULT (*tpm_process_function_t)(tpm_state_t *tpm_state,
                                             TPM_STORE_BUFFER *response,
                                             TPM_TAG tag,
                                             uint32_t paramSize,
                                             TPM_COMMAND_CODE ordinal,
                                             unsigned char *command,
                                             TPM_TRANSPORT_INTERNAL *transportInternal);

typedef struct tdTPM_ORDINAL_TABLE {
    TPM_COMMAND_CODE ordinal;
    tpm_process_function_t process_function_v11;        /* processing function for TPM 1.1 */
    tpm_process_function_t process_function_v12;        /* processing function for TPM 1.2 */
    TPM_BOOL auditable;                                 /* FALSE for functions never audited */
    TPM_BOOL auditDefault;                              /* TRUE if auditing is enabled by default */
    uint16_t ownerPermissionBlock;			/* 0:unused, 1:per1 2:per2 */
    uint32_t ownerPermissionPosition;			/* owner permission bit position */
    uint16_t keyPermissionBlock;			/* 0:unused, 1:per1 2:per2 */
    uint32_t keyPermissionPosition;			/* key permission bit position */
    uint32_t inputHandleSize;				/* bytes of input handles (or other bytes
                                                           not to be encrypted or transport
                                                           audited) */
    uint32_t keyHandles;				/* number of input key handles */
    uint32_t outputHandleSize;				/* bytes of output handles (or other bytes
                                                           not to be encrypted or transport
                                                           audited */
    TPM_BOOL transportWrappable;                        /* can be wrapped in transport session */
    TPM_BOOL instanceWrappable;                         /* ordinal can be wrapped and called by
                                                           a parent instance  */
    TPM_BOOL hardwareWrappable;                         /* ordinal can be wrapped and call the
                                                           hardware TPM instance  */
} TPM_ORDINAL_TABLE;

TPM_RESULT TPM_OrdinalTable_GetEntry(TPM_ORDINAL_TABLE **entry,
                                     TPM_ORDINAL_TABLE *ordinalTable,
                                     TPM_COMMAND_CODE ordinal);
void       TPM_OrdinalTable_GetProcessFunction(tpm_process_function_t *tpm_process_function,
                                               TPM_ORDINAL_TABLE *ordinalTable,
                                               TPM_COMMAND_CODE ordinal);
void       TPM_OrdinalTable_GetAuditable(TPM_BOOL *auditable,
                                         TPM_COMMAND_CODE ordinal);
void       TPM_OrdinalTable_GetAuditDefault(TPM_BOOL *auditDefault,
                                            TPM_COMMAND_CODE ordinal);
TPM_RESULT TPM_OrdinalTable_GetOwnerPermission(uint16_t *ownerPermissionBlock,
                                               uint32_t *ownerPermissionPosition,
                                               TPM_COMMAND_CODE ordinal);
TPM_RESULT TPM_OrdinalTable_GetKeyPermission(uint16_t *keyPermissionBlock,
                                             uint32_t *keyPermissionPosition,
                                             TPM_COMMAND_CODE ordinal);
TPM_RESULT TPM_OrdinalTable_ParseWrappedCmd(uint32_t *datawStart,
                                            uint32_t *datawLen,
                                            uint32_t *keyHandles,
                                            uint32_t *keyHandle1Index,
                                            uint32_t *keyHandle2Index,
                                            TPM_COMMAND_CODE *ordinal,
                                            TPM_BOOL *transportWrappable,
                                            TPM_SIZED_BUFFER *wrappedCmd);
TPM_RESULT TPM_OrdinalTable_ParseWrappedRsp(uint32_t *datawStart,
                                            uint32_t *datawLen,
                                            TPM_RESULT *rcw,
                                            TPM_COMMAND_CODE ordinal,
                                            const unsigned char *wrappedRspStream,
                                            uint32_t wrappedRspStreamSize);


TPM_RESULT TPM_GetInParamDigest(TPM_DIGEST inParamDigest,
                                TPM_BOOL *auditStatus,
                                TPM_BOOL *transportEncrypt,
                                tpm_state_t *tpm_state,
                                TPM_TAG tag,
                                TPM_COMMAND_CODE ordinal,
                                unsigned char *inParamStart,
                                unsigned char *inParamEnd,
                                TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_GetOutParamDigest(TPM_DIGEST outParamDigest,
                                 TPM_BOOL auditStatus,  
                                 TPM_BOOL transportEncrypt,
                                 TPM_TAG tag,                   
                                 TPM_RESULT returnCode,
                                 TPM_COMMAND_CODE ordinal,
                                 unsigned char *outParamStart,
                                 uint32_t outParamLength);
TPM_RESULT TPM_ProcessAudit(tpm_state_t *tpm_state,
                            TPM_BOOL transportEncrypt,
                            TPM_DIGEST inParamDigest,
                            TPM_DIGEST outParamDigest,
                            TPM_COMMAND_CODE ordinal);

/*
  defines for TPM_CheckState check map
*/

#define TPM_CHECK_NOT_SHUTDOWN          0x00000001
#define TPM_CHECK_ENABLED               0x00000004
#define TPM_CHECK_ACTIVATED             0x00000008
#define TPM_CHECK_OWNER                 0x00000010
#define TPM_CHECK_NO_LOCKOUT            0x00000020
#define TPM_CHECK_NV_NOAUTH             0x00000040

/* default conditions to check */
#define TPM_CHECK_ALL                   0x0000003f      /* all state */
#define TPM_CHECK_ALLOW_NO_OWNER        0x0000002f      /* all state but owner installed */

#endif
