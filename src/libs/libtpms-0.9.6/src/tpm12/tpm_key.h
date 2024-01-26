/********************************************************************************/
/*                                                                              */
/*                              Key Handler                                     */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_key.h $               */
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

#ifndef TPM_KEY_H
#define TPM_KEY_H

#include "tpm_global.h"
#include "tpm_store.h"
#include "tpm_structures.h"

#define TPM_KEY_RSA_NUMBITS     2048

extern unsigned char tpm_default_rsa_exponent[];

/* TPM_KEY */

void       TPM_Key_Init(TPM_KEY *tpm_key);
void       TPM_Key_InitTag12(TPM_KEY *tpm_key);

TPM_RESULT TPM_Key_Load(TPM_KEY *tpm_key,
                        unsigned char **stream,
                        uint32_t *stream_size);
TPM_RESULT TPM_Key_LoadPubData(TPM_KEY *tpm_key,
			       TPM_BOOL isEK,
                               unsigned char **stream,
                               uint32_t *stream_size);
TPM_RESULT TPM_Key_LoadClear(TPM_KEY *tpm_key,
			     TPM_BOOL isEK,
                             unsigned char **stream,
                             uint32_t *stream_size);
TPM_RESULT TPM_Key_Store(TPM_STORE_BUFFER *sbuffer,
                         TPM_KEY *tpm_key);
TPM_RESULT TPM_Key_StorePubData(TPM_STORE_BUFFER *sbuffer,
				TPM_BOOL isEK,
                                TPM_KEY *tpm_key);
TPM_RESULT TPM_Key_StoreClear(TPM_STORE_BUFFER *sbuffer,
			      TPM_BOOL isEK,
                              TPM_KEY *tpm_key);

void       TPM_Key_Delete(TPM_KEY *tpm_key);

TPM_RESULT TPM_Key_CheckStruct(int *ver, TPM_KEY *tpm_key);
TPM_RESULT TPM_Key_Set(TPM_KEY *tpm_key,
                       tpm_state_t *tpm_state,
                       TPM_KEY *parent_key,
                       TPM_DIGEST *tpm_pcrs,
                       int ver,
                       TPM_KEY_USAGE keyUsage,
                       TPM_KEY_FLAGS keyFlags,
                       TPM_AUTH_DATA_USAGE authDataUsage,
                       TPM_KEY_PARMS *tpm_key_parms,
                       TPM_PCR_INFO *tpm_pcr_info,
                       TPM_PCR_INFO_LONG *tpm_pcr_info_long,
                       uint32_t keyLength,
                       BYTE* publicKey,
                       TPM_STORE_ASYMKEY *tpm_store_asymkey,
                       TPM_MIGRATE_ASYMKEY *tpm_migrate_asymkey);
TPM_RESULT TPM_Key_Copy(TPM_KEY *tpm_key_dest,
                        TPM_KEY *tpm_key_src,
                        TPM_BOOL copyEncData);
TPM_RESULT TPM_Key_LoadStoreAsymKey(TPM_KEY *tpm_key,
				    TPM_BOOL isEK,
                                    unsigned char **stream,
                                    uint32_t *stream_size);
TPM_RESULT TPM_Key_StorePubkey(TPM_STORE_BUFFER *pubkeyStream,
                               const unsigned char **pubkKeyStreamBuffer,
                               uint32_t *pubkeyStreamLength,
                               TPM_KEY *tpm_key);
TPM_RESULT TPM_Key_GenerateRSA(TPM_KEY *tpm_key,
                               tpm_state_t *tpm_state,
                               TPM_KEY *parent_key,
                               TPM_DIGEST *tpm_pcrs,
                               int ver,
                               TPM_KEY_USAGE keyUsage,
                               TPM_KEY_FLAGS keyFlags,
                               TPM_AUTH_DATA_USAGE authDataUsage,
                               TPM_KEY_PARMS *tpm_key_parms,
                               TPM_PCR_INFO *tpm_pcr_info,
                               TPM_PCR_INFO_LONG *tpm_pcr_info_long);

TPM_RESULT TPM_Key_GeneratePubDataDigest(TPM_KEY *tpm_key);
TPM_RESULT TPM_Key_GeneratePubkeyDigest(TPM_DIGEST tpm_digest,
                                        TPM_KEY *tpm_key);
TPM_RESULT TPM_Key_ComparePubkey(TPM_KEY *tpm_key,
                                 TPM_PUBKEY *tpm_pubkey);

TPM_RESULT TPM_Key_CheckPubDataDigest(TPM_KEY *tpm_key);

TPM_RESULT TPM_Key_GenerateEncData(TPM_KEY *tpm_key,
                                   TPM_KEY *parent_key);
TPM_RESULT TPM_Key_DecryptEncData(TPM_KEY *tpm_key,
                                  TPM_KEY *parent_key);

TPM_RESULT TPM_Key_GetStoreAsymkey(TPM_STORE_ASYMKEY **tpm_store_asymkey,
                                   TPM_KEY *tpm_key);
TPM_RESULT TPM_Key_GetMigrateAsymkey(TPM_MIGRATE_ASYMKEY **tpm_migrate_asymkey,
                                     TPM_KEY *tpm_key);
TPM_RESULT TPM_Key_GetUsageAuth(TPM_SECRET **usageAuth,
                                TPM_KEY *tpm_key);
TPM_RESULT TPM_Key_GetPublicKey(uint32_t	*nbytes,
                                unsigned char   **narr,
                                TPM_KEY         *tpm_key);
TPM_RESULT TPM_Key_GetPrimeFactorP(uint32_t 		*pbytes,
                                   unsigned char        **parr,
                                   TPM_KEY              *tpm_key);
TPM_RESULT TPM_Key_GetPrivateKey(uint32_t	*dbytes,
                                 unsigned char  **darr,
                                 TPM_KEY        *tpm_key);
TPM_RESULT TPM_Key_GetExponent(uint32_t		*ebytes,
                               unsigned char    **earr,
                               TPM_KEY  *tpm_key);
TPM_RESULT TPM_Key_CheckProperties(int *ver,
                                   TPM_KEY *tpm_key,
                                   uint32_t keyLength,
                                   TPM_BOOL FIPS);
TPM_RESULT TPM_Key_GetPCRUsage(TPM_BOOL *pcrUsage,
                               TPM_KEY *tpm_key,
                               size_t start_index);
TPM_RESULT TPM_Key_GetLocalityAtRelease(TPM_LOCALITY_SELECTION *localityAtRelease,
                                        TPM_KEY *tpm_key);
TPM_RESULT TPM_Key_CheckPCRDigest(TPM_KEY *tpm_key,
                                  tpm_state_t *tpm_state);
TPM_RESULT TPM_Key_CheckRestrictDelegate(TPM_KEY *tpm_key,
                                         TPM_CMK_DELEGATE restrictDelegate);

/*
  TPM_KEY_FLAGS
*/

TPM_RESULT TPM_KeyFlags_Load(TPM_KEY_FLAGS *tpm_key_flags,
                             unsigned char **stream,
                             uint32_t *stream_size);

/*
  TPM_KEY_PARMS
*/

void       TPM_KeyParms_Init(TPM_KEY_PARMS *tpm_key_parms);
TPM_RESULT TPM_KeyParms_Set(TPM_KEY_PARMS *tpm_key_parms,
                            TPM_ALGORITHM_ID algorithmID,
                            TPM_ENC_SCHEME encScheme,
                            TPM_SIG_SCHEME sigScheme,
                            uint32_t parmSize,
                            BYTE* parms);
#if 0
TPM_RESULT TPM_KeyParms_SetRSA(TPM_KEY_PARMS *tpm_key_parms,
                               TPM_ALGORITHM_ID algorithmID,
                               TPM_ENC_SCHEME encScheme,
                               TPM_SIG_SCHEME sigScheme,
                               uint32_t keyLength,
                               TPM_SIZED_BUFFER *exponent);
#endif
TPM_RESULT TPM_KeyParms_Copy(TPM_KEY_PARMS *tpm_key_parms_dest,
                             TPM_KEY_PARMS *tpm_key_parms_src);
TPM_RESULT TPM_KeyParms_Load(TPM_KEY_PARMS *tpm_key_parms,
                             unsigned char **stream,
                             uint32_t *stream_size);
TPM_RESULT TPM_KeyParms_Store(TPM_STORE_BUFFER *sbuffer,
                              TPM_KEY_PARMS *tpm_key_parms);
void       TPM_KeyParms_Delete(TPM_KEY_PARMS *tpm_key_parms);
TPM_RESULT TPM_KeyParms_GetRSAKeyParms(TPM_RSA_KEY_PARMS **tpm_rsa_key_parms,
                                       TPM_KEY_PARMS *tpm_key_parms);
TPM_RESULT TPM_KeyParms_GetExponent(uint32_t		*ebytes,
                                    unsigned char       **earr,
                                    TPM_KEY_PARMS       *tpm_key_parms);
TPM_RESULT TPM_KeyParms_CheckProperties(TPM_KEY_PARMS *tpm_key_parms,
                                        TPM_KEY_USAGE tpm_key_usage,
                                        uint32_t keyLength,
                                        TPM_BOOL FIPS);
TPM_RESULT TPM_KeyParams_CheckDefaultExponent(TPM_SIZED_BUFFER *exponent);

/*
  TPM_PUBKEY
*/

void       TPM_Pubkey_Init(TPM_PUBKEY *tpm_pubkey);
TPM_RESULT TPM_Pubkey_Load(TPM_PUBKEY *tpm_pubkey,
                           unsigned char **stream,
                           uint32_t *stream_size);
TPM_RESULT TPM_Pubkey_Store(TPM_STORE_BUFFER *sbuffer,
                            TPM_PUBKEY *tpm_pubkey);
void       TPM_Pubkey_Delete(TPM_PUBKEY *tpm_pubkey);

TPM_RESULT TPM_Pubkey_Set(TPM_PUBKEY *tpm_pubkey,
                          TPM_KEY *tpm_key);
TPM_RESULT TPM_Pubkey_Copy(TPM_PUBKEY *dest_tpm_pubkey,
                           TPM_PUBKEY *src_tpm_pubkey);
TPM_RESULT TPM_Pubkey_GetExponent(uint32_t 	*ebytes,
                                  unsigned char **earr,
                                  TPM_PUBKEY    *tpm_pubkey);
TPM_RESULT TPM_Pubkey_GetPublicKey(uint32_t		*nbytes,
                                   unsigned char        **narr,
                                   TPM_PUBKEY           *tpm_pubkey);

/*
  TPM_KEY_HANDLE_ENTRY
*/

void       TPM_KeyHandleEntry_Init(TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entry);
TPM_RESULT TPM_KeyHandleEntry_Load(TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entry,
                                   unsigned char **stream,
                                   uint32_t *stream_size);
TPM_RESULT TPM_KeyHandleEntry_Store(TPM_STORE_BUFFER *sbuffer,
                                    const TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entry);
void       TPM_KeyHandleEntry_Delete(TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entry);

TPM_RESULT TPM_KeyHandleEntry_FlushSpecific(tpm_state_t *tpm_state,
                                            TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entry);

/*
  TPM_KEY_HANDLE_ENTRY entries list
*/

void       TPM_KeyHandleEntries_Init(TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries);
void       TPM_KeyHandleEntries_Delete(TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries);

TPM_RESULT TPM_KeyHandleEntries_Load(tpm_state_t *tpm_state,
				     unsigned char **stream,
				     uint32_t *stream_size);
TPM_RESULT TPM_KeyHandleEntries_Store(TPM_STORE_BUFFER *sbuffer,
				      tpm_state_t *tpm_state);

TPM_RESULT TPM_KeyHandleEntries_StoreHandles(TPM_STORE_BUFFER *sbuffer,
                                             const TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries);
TPM_RESULT TPM_KeyHandleEntries_DeleteHandle(TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries,
                                             TPM_KEY_HANDLE tpm_key_handle);

void       TPM_KeyHandleEntries_IsSpace(TPM_BOOL *isSpace, uint32_t *index,
                                        const TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries);
void       TPM_KeyHandleEntries_GetSpace(uint32_t *space,
                                         const TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries);
void       TPM_KeyHandleEntries_IsEvictSpace(TPM_BOOL *isSpace,
                                             const TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries,
                                             uint32_t minSpace);
TPM_RESULT TPM_KeyHandleEntries_AddKeyEntry(TPM_KEY_HANDLE *tpm_key_handle,
                                            TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries,
                                            TPM_KEY *tpm_key,
                                            TPM_BOOL parentPCRStatus,
                                            TPM_KEY_CONTROL keyControl);
TPM_RESULT TPM_KeyHandleEntries_AddEntry(TPM_KEY_HANDLE *tpm_key_handle,
                                         TPM_BOOL keepHandle,
                                         TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries,
                                         TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entry);
TPM_RESULT TPM_KeyHandleEntries_GetEntry(TPM_KEY_HANDLE_ENTRY **tpm_key_handle_entry,
                                         TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries,
                                         TPM_KEY_HANDLE tpm_key_handle);
TPM_RESULT TPM_KeyHandleEntries_GetKey(TPM_KEY **tpm_key,
                                       TPM_BOOL *parentPCRStatus,
                                       tpm_state_t *tpm_state,
                                       TPM_KEY_HANDLE tpm_key_handle,
                                       TPM_BOOL readOnly,
                                       TPM_BOOL ignorePCRs,
                                       TPM_BOOL allowEK);
TPM_RESULT TPM_KeyHandleEntries_SetParentPCRStatus(TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries,
                                                   TPM_KEY_HANDLE tpm_key_handle,
                                                   TPM_BOOL parentPCRStatus);
TPM_RESULT TPM_KeyHandleEntries_GetNextEntry(TPM_KEY_HANDLE_ENTRY **tpm_key_handle_entry,
                                             size_t *current,
                                             TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries,
                                             size_t start);

TPM_RESULT TPM_KeyHandleEntries_OwnerEvictLoad(TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries,
					       unsigned char **stream, uint32_t *stream_size);
TPM_RESULT TPM_KeyHandleEntries_OwnerEvictStore(TPM_STORE_BUFFER *sbuffer,
						const TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries);
TPM_RESULT TPM_KeyHandleEntries_OwnerEvictGetCount(uint16_t *count,
						   const TPM_KEY_HANDLE_ENTRY
						   *tpm_key_handle_entries);
void       TPM_KeyHandleEntries_OwnerEvictDelete(TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries);

/* TPM_RSA_KEY_PARMS */

void       TPM_RSAKeyParms_Init(TPM_RSA_KEY_PARMS *tpm_rsa_key_parms);
TPM_RESULT TPM_RSAKeyParms_Load(TPM_RSA_KEY_PARMS *tpm_rsa_key_parms,
                                unsigned char **stream, 
                                uint32_t *stream_size);
TPM_RESULT TPM_RSAKeyParms_Store(TPM_STORE_BUFFER *sbuffer,
                                 const TPM_RSA_KEY_PARMS *tpm_rsa_key_parms);
void       TPM_RSAKeyParms_Delete(TPM_RSA_KEY_PARMS *tpm_rsa_key_parms);

TPM_RESULT TPM_RSAKeyParms_Copy(TPM_RSA_KEY_PARMS *tpm_rsa_key_parms_dest,
                                TPM_RSA_KEY_PARMS *tpm_rsa_key_parms_src);
TPM_RESULT TPM_RSAKeyParms_New(TPM_RSA_KEY_PARMS **tpm_rsa_key_parms);
TPM_RESULT TPM_RSAKeyParms_GetExponent(uint32_t *ebytes,
                                       unsigned char **earr,
                                       TPM_RSA_KEY_PARMS *tpm_rsa_key_parms);

/* TPM_STORE_ASYMKEY */

void       TPM_StoreAsymkey_Init(TPM_STORE_ASYMKEY *tpm_store_asymkey);
TPM_RESULT TPM_StoreAsymkey_Load(TPM_STORE_ASYMKEY *tpm_store_asymkey,
				 TPM_BOOL isEK,
                                 unsigned char **stream,        
                                 uint32_t *stream_size,
                                 TPM_KEY_PARMS *tpm_key_parms,
                                 TPM_SIZED_BUFFER *tpm_store_pubkey);
TPM_RESULT TPM_StoreAsymkey_Store(TPM_STORE_BUFFER *sbuffer,
				  TPM_BOOL isEK,
                                  const TPM_STORE_ASYMKEY *tpm_store_asymkey);
void       TPM_StoreAsymkey_Delete(TPM_STORE_ASYMKEY *tpm_store_asymkey);

TPM_RESULT TPM_StoreAsymkey_GenerateEncData(TPM_SIZED_BUFFER *encData,
                                            TPM_STORE_ASYMKEY *tpm_store_asymkey,
                                            TPM_KEY *parent_key);
TPM_RESULT TPM_StoreAsymkey_GetPrimeFactorP(uint32_t 		*pbytes,
                                            unsigned char       **parr,
                                            TPM_STORE_ASYMKEY   *tpm_store_asymkey);
void       TPM_StoreAsymkey_GetO1Size(uint32_t			*o1_size,
                                      TPM_STORE_ASYMKEY         *tpm_store_asymkey);
TPM_RESULT TPM_StoreAsymkey_CheckO1Size(uint32_t o1_size,
                                        uint32_t k1k2_length);
TPM_RESULT TPM_StoreAsymkey_StoreO1(BYTE                *o1,
                                    uint32_t		o1_size,
                                    TPM_STORE_ASYMKEY   *tpm_store_asymkey,
                                    TPM_DIGEST          pHash,
                                    TPM_PAYLOAD_TYPE    payload_type,
                                    TPM_SECRET          usageAuth);
TPM_RESULT TPM_StoreAsymkey_LoadO1(TPM_STORE_ASYMKEY    *tpm_store_asymkey,
                                   BYTE                 *o1,
                                   uint32_t		o1_size);

/* TPM_MIGRATE_ASYMKEY */

void       TPM_MigrateAsymkey_Init(TPM_MIGRATE_ASYMKEY *tpm_migrate_asymkey);
TPM_RESULT TPM_MigrateAsymkey_Load(TPM_MIGRATE_ASYMKEY *tpm_migrate_asymkey,
                                   unsigned char **stream,
                                   uint32_t *stream_size);
TPM_RESULT TPM_MigrateAsymkey_Store(TPM_STORE_BUFFER *sbuffer,
                                    const TPM_MIGRATE_ASYMKEY *tpm_migrate_asymkey);
void       TPM_MigrateAsymkey_Delete(TPM_MIGRATE_ASYMKEY *tpm_migrate_asymkey);

/*
  TPM_STORE_PRIVKEY
*/

void       TPM_StorePrivkey_Init(TPM_STORE_PRIVKEY *tpm_store_privkey);
TPM_RESULT TPM_StorePrivkey_Store(TPM_STORE_BUFFER *sbuffer,
                                  const TPM_STORE_PRIVKEY *tpm_store_privkey);
void       TPM_StorePrivkey_Delete(TPM_STORE_PRIVKEY *tpm_store_privkey);

TPM_RESULT TPM_StorePrivkey_Convert(TPM_STORE_ASYMKEY *tpm_store_asymkey,
                                    TPM_KEY_PARMS *tpm_key_parms,
                                    TPM_SIZED_BUFFER *pubKey);


/* Command Processing Functions */


TPM_RESULT TPM_Process_ReadPubek(tpm_state_t *tpm_state,
                                 TPM_STORE_BUFFER *response,
                                 TPM_TAG tag,
                                 uint32_t paramSize,
                                 TPM_COMMAND_CODE ordinal,
                                 unsigned char *command,
                                 TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_CreateRevocableEK(tpm_state_t *tpm_state,
                                         TPM_STORE_BUFFER *response,
                                         TPM_TAG tag,
                                         uint32_t paramSize,
                                         TPM_COMMAND_CODE ordinal,
                                         unsigned char *command,
                                         TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_CreateEndorsementKeyPair(tpm_state_t *tpm_state,
                                                TPM_STORE_BUFFER *response,
                                                TPM_TAG tag,
                                                uint32_t paramSize,
                                                TPM_COMMAND_CODE ordinal,
                                                unsigned char *command,
                                                TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_CreateEndorsementKeyPair_Common(TPM_KEY *endorsementKey,
                                               TPM_PUBKEY *pubEndorsementKey,
                                               TPM_DIGEST checksum,
                                               TPM_BOOL *writePermanentData,
                                               tpm_state_t *tpm_state,
                                               TPM_KEY_PARMS *keyInfo,
                                               TPM_NONCE antiReplay);

TPM_RESULT TPM_Process_RevokeTrust(tpm_state_t *tpm_state,
                                   TPM_STORE_BUFFER *response,
                                   TPM_TAG tag,
                                   uint32_t paramSize,
                                   TPM_COMMAND_CODE ordinal,
                                   unsigned char *command,
                                   TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_DisablePubekRead(tpm_state_t *tpm_state,
                                        TPM_STORE_BUFFER *response,
                                        TPM_TAG tag,
                                        uint32_t paramSize,
                                        TPM_COMMAND_CODE ordinal,
                                        unsigned char *command,
                                        TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_OwnerReadPubek(tpm_state_t *tpm_state,
                                      TPM_STORE_BUFFER *response,
                                      TPM_TAG tag,
                                      uint32_t paramSize,
                                      TPM_COMMAND_CODE ordinal,
                                      unsigned char *command,
                                      TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_EvictKey(tpm_state_t *tpm_state,
                                TPM_STORE_BUFFER *response,
                                TPM_TAG tag,
                                uint32_t paramSize,
                                TPM_COMMAND_CODE ordinal,
                                unsigned char *command,
                                TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_OwnerReadInternalPub(tpm_state_t *tpm_state,
                                            TPM_STORE_BUFFER *response,
                                            TPM_TAG tag,
                                            uint32_t paramSize,
                                            TPM_COMMAND_CODE ordinal,
                                            unsigned char *command,
                                            TPM_TRANSPORT_INTERNAL *transportInternal);

#endif
