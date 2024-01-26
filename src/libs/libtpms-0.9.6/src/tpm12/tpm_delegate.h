/********************************************************************************/
/*                                                                              */
/*                              Delegate Handler                                */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_delegate.h $  	*/
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

#ifndef TPM_DELEGATE_H
#define TPM_DELEGATE_H

#include "tpm_structures.h"

/*
  TPM_DELEGATE_PUBLIC 
*/

void       TPM_DelegatePublic_Init(TPM_DELEGATE_PUBLIC *tpm_delegate_public);
TPM_RESULT TPM_DelegatePublic_Load(TPM_DELEGATE_PUBLIC *tpm_delegate_public,
                                   unsigned char **stream,
                                   uint32_t *stream_size);
TPM_RESULT TPM_DelegatePublic_Store(TPM_STORE_BUFFER *sbuffer,
                                    const TPM_DELEGATE_PUBLIC *tpm_delegate_public);
void       TPM_DelegatePublic_Delete(TPM_DELEGATE_PUBLIC *tpm_delegate_public);

TPM_RESULT TPM_DelegatePublic_Copy(TPM_DELEGATE_PUBLIC *dest,
                                   TPM_DELEGATE_PUBLIC *src);

/*
  TPM_DELEGATE_SENSITIVE 
*/

void       TPM_DelegateSensitive_Init(TPM_DELEGATE_SENSITIVE *tpm_delegate_sensitive);
TPM_RESULT TPM_DelegateSensitive_Load(TPM_DELEGATE_SENSITIVE *tpm_delegate_sensitive,
                                      unsigned char **stream,
                                      uint32_t *stream_size);
TPM_RESULT TPM_DelegateSensitive_Store(TPM_STORE_BUFFER *sbuffer,
                                       const TPM_DELEGATE_SENSITIVE *tpm_delegate_sensitive);
void       TPM_DelegateSensitive_Delete(TPM_DELEGATE_SENSITIVE *tpm_delegate_sensitive);

TPM_RESULT TPM_DelegateSensitive_DecryptEncData(TPM_DELEGATE_SENSITIVE *tpm_delegate_sensitive,
                                                TPM_SIZED_BUFFER *sensitiveArea,
                                                TPM_SYMMETRIC_KEY_TOKEN delegateKey);

/*
  TPM_DELEGATIONS
*/

void       TPM_Delegations_Init(TPM_DELEGATIONS *tpm_delegations);
TPM_RESULT TPM_Delegations_Load(TPM_DELEGATIONS *tpm_delegations,
                                unsigned char **stream,
                                uint32_t *stream_size);
TPM_RESULT TPM_Delegations_Store(TPM_STORE_BUFFER *sbuffer,
                                 const TPM_DELEGATIONS *tpm_delegations);
void       TPM_Delegations_Delete(TPM_DELEGATIONS *tpm_delegations);

void       TPM_Delegations_Copy(TPM_DELEGATIONS *dest,
                                TPM_DELEGATIONS *src);
TPM_RESULT TPM_Delegations_CheckPermissionDelegation(TPM_DELEGATIONS *newDelegations,
                                                     TPM_DELEGATIONS *currentDelegations);
TPM_RESULT TPM_Delegations_CheckPermission(tpm_state_t *tpm_state,
                                           TPM_DELEGATE_PUBLIC *delegatePublic,
                                           TPM_ENT_TYPE entityType,     
                                           TPM_COMMAND_CODE ordinal);
TPM_RESULT TPM_Delegations_CheckOwnerPermission(TPM_DELEGATIONS *tpm_delegations,
                                                TPM_COMMAND_CODE ordinal);
TPM_RESULT TPM_Delegations_CheckKeyPermission(TPM_DELEGATIONS *tpm_delegations,
                                              TPM_COMMAND_CODE ordinal);

/*
  TPM_DELEGATE_OWNER_BLOB
*/

void       TPM_DelegateOwnerBlob_Init(TPM_DELEGATE_OWNER_BLOB *tpm_delegate_owner_blob);
TPM_RESULT TPM_DelegateOwnerBlob_Load(TPM_DELEGATE_OWNER_BLOB *tpm_delegate_owner_blob,
                                      unsigned char **stream,
                                      uint32_t *stream_size);
TPM_RESULT TPM_DelegateOwnerBlob_Store(TPM_STORE_BUFFER *sbuffer,
                                       const TPM_DELEGATE_OWNER_BLOB *tpm_delegate_owner_blob);
void       TPM_DelegateOwnerBlob_Delete(TPM_DELEGATE_OWNER_BLOB *tpm_delegate_owner_blob);

/*
  TPM_DELEGATE_KEY_BLOB
*/

void       TPM_DelegateKeyBlob_Init(TPM_DELEGATE_KEY_BLOB *tpm_delegate_key_blob);
TPM_RESULT TPM_DelegateKeyBlob_Load(TPM_DELEGATE_KEY_BLOB *tpm_delegate_key_blob,
                                    unsigned char **stream,
                                    uint32_t *stream_size);
TPM_RESULT TPM_DelegateKeyBlob_Store(TPM_STORE_BUFFER *sbuffer,
                                     const TPM_DELEGATE_KEY_BLOB *tpm_delegate_key_blob);
void       TPM_DelegateKeyBlob_Delete(TPM_DELEGATE_KEY_BLOB *tpm_delegate_key_blob);

/*
  TPM_FAMILY_TABLE
*/

void       TPM_FamilyTable_Init(TPM_FAMILY_TABLE *tpm_family_table);
TPM_RESULT TPM_FamilyTable_Load(TPM_FAMILY_TABLE *tpm_family_table,
                                unsigned char **stream,
                                uint32_t *stream_size);
TPM_RESULT TPM_FamilyTable_Store(TPM_STORE_BUFFER *sbuffer,
                                 const TPM_FAMILY_TABLE *tpm_family_table,
				 TPM_BOOL store_tag);
void       TPM_FamilyTable_Delete(TPM_FAMILY_TABLE *tpm_family_table);

TPM_RESULT TPM_FamilyTable_StoreValid(TPM_STORE_BUFFER *sbuffer,
                                      const TPM_FAMILY_TABLE *tpm_family_table,
				      TPM_BOOL store_tag);
TPM_RESULT TPM_FamilyTable_GetEntry(TPM_FAMILY_TABLE_ENTRY **tpm_family_table_entry,
                                    TPM_FAMILY_TABLE *tpm_family_table,
                                    TPM_FAMILY_ID familyID);
TPM_RESULT TPM_FamilyTable_GetEnabledEntry(TPM_FAMILY_TABLE_ENTRY **tpm_family_table_entry,
                                           TPM_FAMILY_TABLE *tpm_family_table,
                                           TPM_FAMILY_ID familyID);
TPM_RESULT TPM_FamilyTable_IsSpace(TPM_FAMILY_TABLE_ENTRY **tpm_family_table_entry,
                                   TPM_FAMILY_TABLE *tpm_family_table);

/*
  TPM_FAMILY_TABLE_ENTRY
*/

void       TPM_FamilyTableEntry_Init(TPM_FAMILY_TABLE_ENTRY *tpm_family_table_entry);
TPM_RESULT TPM_FamilyTableEntry_Load(TPM_FAMILY_TABLE_ENTRY *tpm_family_table_entry,
                                     unsigned char **stream,
                                     uint32_t *stream_size);
TPM_RESULT TPM_FamilyTableEntry_Store(TPM_STORE_BUFFER *sbuffer,
                                      const TPM_FAMILY_TABLE_ENTRY *tpm_family_table_entry,
				      TPM_BOOL store_tag);
TPM_RESULT TPM_FamilyTableEntry_StorePublic(TPM_STORE_BUFFER *sbuffer,
                                            const TPM_FAMILY_TABLE_ENTRY *tpm_family_table_entry,
					    TPM_BOOL store_tag);
void       TPM_FamilyTableEntry_Delete(TPM_FAMILY_TABLE_ENTRY *tpm_family_table_entry);

/*
  TPM_DELEGATE_TABLE
*/

void       TPM_DelegateTable_Init(TPM_DELEGATE_TABLE *tpm_delegate_table);
TPM_RESULT TPM_DelegateTable_Load(TPM_DELEGATE_TABLE *tpm_delegate_table,
                                  unsigned char **stream,
                                  uint32_t *stream_size);
TPM_RESULT TPM_DelegateTable_Store(TPM_STORE_BUFFER *sbuffer,
                                   const TPM_DELEGATE_TABLE *tpm_delegate_table);
void       TPM_DelegateTable_Delete(TPM_DELEGATE_TABLE *tpm_delegate_table);

TPM_RESULT TPM_DelegateTable_StoreValid(TPM_STORE_BUFFER *sbuffer,
                                        const TPM_DELEGATE_TABLE *tpm_delegate_table);
TPM_RESULT TPM_DelegateTable_GetRow(TPM_DELEGATE_TABLE_ROW **delegateTableRow,
                                    TPM_DELEGATE_TABLE *tpm_delegate_table,
                                    uint32_t rowIndex);
TPM_RESULT TPM_DelegateTable_GetValidRow(TPM_DELEGATE_TABLE_ROW **delegateTableRow,
                                         TPM_DELEGATE_TABLE *tpm_delegate_table,
                                         uint32_t rowIndex);


/*
  TPM_DELEGATE_TABLE_ROW
*/

void       TPM_DelegateTableRow_Init(TPM_DELEGATE_TABLE_ROW *tpm_delegate_table_row);
TPM_RESULT TPM_DelegateTableRow_Load(TPM_DELEGATE_TABLE_ROW *tpm_delegate_table_row,
                                     unsigned char **stream,
                                     uint32_t *stream_size);
TPM_RESULT TPM_DelegateTableRow_Store(TPM_STORE_BUFFER *sbuffer,
                                      const TPM_DELEGATE_TABLE_ROW *tpm_delegate_table_row);
void       TPM_DelegateTableRow_Delete(TPM_DELEGATE_TABLE_ROW *tpm_delegate_table_row);




/*
  Processing Functions
*/

TPM_RESULT TPM_Process_DelegateManage(tpm_state_t *tpm_state,
                                      TPM_STORE_BUFFER *response,
                                      TPM_TAG tag,
                                      uint32_t paramSize,
                                      TPM_COMMAND_CODE ordinal,
                                      unsigned char *command,
                                      TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_DelegateCreateKeyDelegation(tpm_state_t *tpm_state,
                                                   TPM_STORE_BUFFER *response,
                                                   TPM_TAG tag,
                                                   uint32_t paramSize,
                                                   TPM_COMMAND_CODE ordinal,
                                                   unsigned char *command,
                                                   TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_DelegateCreateOwnerDelegation(tpm_state_t *tpm_state,
                                                     TPM_STORE_BUFFER *response,
                                                     TPM_TAG tag,
                                                     uint32_t paramSize,
                                                     TPM_COMMAND_CODE ordinal,
                                                     unsigned char *command,
                                                     TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_DelegateLoadOwnerDelegation(tpm_state_t *tpm_state,
                                                   TPM_STORE_BUFFER *response,
                                                   TPM_TAG tag,
                                                   uint32_t paramSize,
                                                   TPM_COMMAND_CODE ordinal,
                                                   unsigned char *command,
                                                   TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_DelegateReadTable(tpm_state_t *tpm_state,
                                         TPM_STORE_BUFFER *response,
                                         TPM_TAG tag,
                                         uint32_t paramSize,
                                         TPM_COMMAND_CODE ordinal,
                                         unsigned char *command,
                                         TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_DelegateUpdateVerification(tpm_state_t *tpm_state,
                                                  TPM_STORE_BUFFER *response,
                                                  TPM_TAG tag,
                                                  uint32_t paramSize,
                                                  TPM_COMMAND_CODE ordinal,
                                                  unsigned char *command,
                                                  TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_DelegateVerifyDelegation(tpm_state_t *tpm_state,
                                                TPM_STORE_BUFFER *response,
                                                TPM_TAG tag,
                                                uint32_t paramSize,
                                                TPM_COMMAND_CODE ordinal,
                                                unsigned char *command,
                                                TPM_TRANSPORT_INTERNAL *transportInternal);

#endif
