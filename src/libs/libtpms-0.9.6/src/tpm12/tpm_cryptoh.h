/********************************************************************************/
/*                                                                              */
/*                      High Level Platform Independent Crypto                  */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_cryptoh.h $           */
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

#ifndef TPM_CRYPTOH_H
#define TPM_CRYPTOH_H

#include "tpm_global.h"
#include "tpm_types.h"
#include "tpm_sizedbuffer.h"
#include "tpm_structures.h"

/*
  TPM_SIGN_INFO
*/

void       TPM_SignInfo_Init(TPM_SIGN_INFO *tpm_sign_info);
TPM_RESULT TPM_SignInfo_Store(TPM_STORE_BUFFER *sbuffer,
                              const TPM_SIGN_INFO *tpm_sign_info);
void       TPM_SignInfo_Delete(TPM_SIGN_INFO *tpm_sign_info);

/*
  TPM_CERTIFY_INFO
*/

void       TPM_CertifyInfo_Init(TPM_CERTIFY_INFO *tpm_certify_info);
#if 0
TPM_RESULT TPM_CertifyInfo_Load(TPM_CERTIFY_INFO *tpm_certify_info,
                                unsigned char **stream,
                                uint32_t *stream_size);
#endif
TPM_RESULT TPM_CertifyInfo_Store(TPM_STORE_BUFFER *sbuffer,
                                 TPM_CERTIFY_INFO *tpm_certify_info);
void       TPM_CertifyInfo_Delete(TPM_CERTIFY_INFO *tpm_certify_info);

TPM_RESULT TPM_CertifyInfo_Set(TPM_CERTIFY_INFO *tpm_certify_info,
                               TPM_KEY *tpm_key);

/*
  TPM_CERTIFY_INFO2
*/

void       TPM_CertifyInfo2_Init(TPM_CERTIFY_INFO2 *tpm_certify_info2);
#if 0
TPM_RESULT TPM_CertifyInfo2_Load(TPM_CERTIFY_INFO2 *tpm_certify_info2,
                                 unsigned char **stream,
                                 uint32_t *stream_size);
#endif
TPM_RESULT TPM_CertifyInfo2_Store(TPM_STORE_BUFFER *sbuffer,
                                  TPM_CERTIFY_INFO2 *tpm_certify_info2);
void       TPM_CertifyInfo2_Delete(TPM_CERTIFY_INFO2 *tpm_certify_info2);

TPM_RESULT TPM_CertifyInfo2_Set(TPM_CERTIFY_INFO2 *tpm_certify_info2,
                                TPM_KEY *tpm_key);

/*
  TPM_SYMMETRIC_KEY
*/

void       TPM_SymmetricKey_Init(TPM_SYMMETRIC_KEY *tpm_symmetric_key);
TPM_RESULT TPM_SymmetricKey_Load(TPM_SYMMETRIC_KEY *tpm_symmetric_key,
                                 unsigned char **stream,
                                 uint32_t *stream_size);
TPM_RESULT TPM_SymmetricKey_Store(TPM_STORE_BUFFER *sbuffer,
                                  const TPM_SYMMETRIC_KEY *tpm_symmetric_key);
void       TPM_SymmetricKey_Delete(TPM_SYMMETRIC_KEY *tpm_symmetric_key);

TPM_RESULT TPM_SymmetricKeyData_EncryptSbuffer(TPM_SIZED_BUFFER *encrypt_data,
                                               TPM_STORE_BUFFER *sbuffer,
                                               const TPM_SYMMETRIC_KEY_TOKEN
					       tpm_symmetric_key_data);
TPM_RESULT TPM_SymmetricKeyData_StreamCrypt(unsigned char *data_out,
                                            const unsigned char *data_in,
                                            uint32_t data_size,
                                            TPM_ALGORITHM_ID algId,
                                            TPM_ENC_SCHEME encScheme,
                                            const unsigned char *symmetric_key,
                                            uint32_t symmetric_key_size,
                                            unsigned char *pad_in,
                                            uint32_t pad_in_size);

/*
  RSA functions
*/


TPM_RESULT TPM_RSAPrivateDecryptMalloc(unsigned char **decrypt_data,
                                       uint32_t *decrypt_data_length,
                                       unsigned char *encrypt_data,
                                       uint32_t encrypt_data_size,
                                       TPM_KEY *tpm_key);

TPM_RESULT TPM_RSAPrivateDecryptH(unsigned char *decrypt_data,
                                  uint32_t *decrypt_data_length,
                                  uint32_t decrypt_data_size,
                                  unsigned char *encrypt_data,
                                  uint32_t encrypt_data_size,
                                  TPM_KEY *tpm_key);

TPM_RESULT TPM_RSAPublicEncryptSbuffer_Key(TPM_SIZED_BUFFER *enc_data,
                                           TPM_STORE_BUFFER *sbuffer,
                                           TPM_KEY *tpm_key);
TPM_RESULT TPM_RSAPublicEncrypt_Key(TPM_SIZED_BUFFER *enc_data,
                                    const unsigned char *decrypt_data,
                                    size_t decrypt_data_size,
                                    TPM_KEY *tpm_key);
TPM_RESULT TPM_RSAPublicEncrypt_Pubkey(TPM_SIZED_BUFFER *enc_data,
                                       const unsigned char *decrypt_data,
                                       size_t decrypt_data_size,
                                       TPM_PUBKEY *tpm_pubkey);

TPM_RESULT TPM_RSAPublicEncrypt_Common(TPM_SIZED_BUFFER *enc_data,
                                       const unsigned char *decrypt_data,
                                       size_t 		decrypt_data_size,
                                       TPM_ENC_SCHEME 	encScheme,
                                       unsigned char    *narr,
                                       uint32_t 	nbytes,
                                       unsigned char    *earr,
                                       uint32_t		ebytes);

TPM_RESULT TPM_RSASignH(unsigned char *signature,
                        unsigned int *signature_length,
                        unsigned int signature_size,
                        const unsigned char *message,
                        size_t message_size,
                        TPM_KEY *tpm_key);
                                  
TPM_RESULT TPM_RSASignToSizedBuffer(TPM_SIZED_BUFFER *signature,
                                    const unsigned char *message,
                                    size_t message_size,
                                    TPM_KEY *tpm_key);

TPM_RESULT TPM_RSAVerifyH(TPM_SIZED_BUFFER *signature,
                          const unsigned char *message,
                          uint32_t message_size,
                          TPM_PUBKEY *tpm_pubkey);
TPM_RESULT TPM_RSAVerify(unsigned char *signature,
                         unsigned int signature_size,
                         TPM_SIG_SCHEME sigScheme,
                         const unsigned char *message,
                         uint32_t message_size,
                         unsigned char *narr,
                         uint32_t nbytes,
                         unsigned char *earr,
                         uint32_t ebytes);

TPM_RESULT TPM_RSA_exponent_verify(unsigned long exponent);

/*
  OAEP Padding 
*/

TPM_RESULT TPM_RSA_padding_add_PKCS1_OAEP(unsigned char *em, uint32_t emLen,
                                          const unsigned char *from, uint32_t fLen,
                                          const unsigned char *pHash,
                                          const unsigned char *seed);
TPM_RESULT TPM_RSA_padding_check_PKCS1_OAEP(unsigned char *to, uint32_t *tLen, uint32_t tSize, 
                                            const unsigned char *em, uint32_t emLen,
                                            unsigned char *pHash,
                                            unsigned char *seed);

/*
  Digest functions - SHA-1 and HMAC
*/

TPM_RESULT TPM_SHA1(TPM_DIGEST md, ...);
TPM_RESULT TPM_SHA1_Check(TPM_DIGEST digest_expect, ...);
TPM_RESULT TPM_SHA1Sbuffer(TPM_DIGEST tpm_digest,
                           TPM_STORE_BUFFER *sbuffer);
TPM_RESULT TPM_SHA1_GenerateStructure(TPM_DIGEST tpm_digest,
                                      void *tpmStructure,
                                      TPM_STORE_FUNCTION_T storeFunction);
TPM_RESULT TPM_SHA1_CheckStructure(TPM_DIGEST expected_digest,
                                   void *tpmStructure,
                                   TPM_STORE_FUNCTION_T storeFunction,
                                   TPM_RESULT error);

TPM_RESULT TPM_HMAC_GenerateSbuffer(TPM_HMAC tpm_hmac,
                                    const TPM_SECRET hmac_key,
                                    TPM_STORE_BUFFER *sbuffer);
TPM_RESULT TPM_HMAC_GenerateStructure(TPM_HMAC tpm_hmac,
                                      const TPM_SECRET hmac_key,
                                      void *tpmStructure,
                                      TPM_STORE_FUNCTION_T storeFunction);
TPM_RESULT TPM_HMAC_Generate(TPM_HMAC tpm_hmac,
                             const TPM_SECRET hmac_key,
                             ...);

TPM_RESULT TPM_HMAC_CheckSbuffer(TPM_BOOL *valid,
                                 TPM_HMAC expect,
                                 const TPM_SECRET hmac_key,
                                 TPM_STORE_BUFFER *sbuffer);
TPM_RESULT TPM_HMAC_Check(TPM_BOOL *valid,
                          TPM_HMAC expect,
                          const TPM_SECRET key,
                          ...);
TPM_RESULT TPM_HMAC_CheckStructure(const TPM_SECRET hmac_key,
                                   void *structure,
                                   TPM_HMAC expect,
                                   TPM_STORE_FUNCTION_T storeFunction,
                                   TPM_RESULT error);

/*
  XOR
*/

void       TPM_XOR(unsigned char *out,
                   const unsigned char *in1,
                   const unsigned char *in2,
                   size_t length);

/*
  MGF1
*/

TPM_RESULT TPM_MGF1(unsigned char       *array,
                    uint32_t		arrayLen,
                    const unsigned char *seed,
                    uint32_t		seedLen);
TPM_RESULT TPM_MGF1_GenerateArray(unsigned char **array,
				  uint32_t arrayLen,
				  uint32_t seedLen,
				  ...);
/* bignum */

TPM_RESULT TPM_bn2binMalloc(unsigned char **bin,
			    unsigned int *bytes,
			    TPM_BIGNUM bn_in,
			    uint32_t padBytes);
TPM_RESULT TPM_bn2binArray(unsigned char *bin,
			   unsigned int bytes,
			   TPM_BIGNUM bn);
TPM_RESULT TPM_2bin2bn(TPM_BIGNUM *bignum_in,
                       const unsigned char *bin0, uint32_t size0,
                       const unsigned char *bin1, uint32_t size1);

/*
  Self Test
*/

TPM_RESULT TPM_CryptoTest(void);


/*
  Processing functions
*/

TPM_RESULT TPM_Process_Sign(tpm_state_t *tpm_state,
                            TPM_STORE_BUFFER *response,
                            TPM_TAG tag,
                            uint32_t paramSize,
                            TPM_COMMAND_CODE ordinal,
                            unsigned char *command,
                            TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_SHA1Start(tpm_state_t *tpm_state,
                                 TPM_STORE_BUFFER *response,
                                 TPM_TAG tag,
                                 uint32_t paramSize,
                                 TPM_COMMAND_CODE ordinal,
                                 unsigned char *command,
                                 TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_SHA1Update(tpm_state_t *tpm_state,
                                  TPM_STORE_BUFFER *response,
                                  TPM_TAG tag,
                                  uint32_t paramSize,
                                  TPM_COMMAND_CODE ordinal,
                                  unsigned char *command,
                                  TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_SHA1Complete(tpm_state_t *tpm_state,
                                    TPM_STORE_BUFFER *response,
                                    TPM_TAG tag,
                                    uint32_t paramSize,
                                    TPM_COMMAND_CODE ordinal,
                                    unsigned char *command,
                                    TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_SHA1CompleteExtend(tpm_state_t *tpm_state,
                                          TPM_STORE_BUFFER *response,
                                          TPM_TAG tag,
                                          uint32_t paramSize,
                                          TPM_COMMAND_CODE ordinal,
                                          unsigned char *command,
                                          TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_GetRandom(tpm_state_t *tpm_state,
                                 TPM_STORE_BUFFER *response,
                                 TPM_TAG tag,
                                 uint32_t paramSize,
                                 TPM_COMMAND_CODE ordinal,
                                 unsigned char *command,
                                 TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_StirRandom(tpm_state_t *tpm_state,
                                  TPM_STORE_BUFFER *response,
                                  TPM_TAG tag,
                                  uint32_t paramSize,
                                  TPM_COMMAND_CODE ordinal,
                                  unsigned char *command,
                                  TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_CertifyKey(tpm_state_t *tpm_state,
                                  TPM_STORE_BUFFER *response,
                                  TPM_TAG tag,
                                  uint32_t paramSize,
                                  TPM_COMMAND_CODE ordinal,
                                  unsigned char *command,
                                  TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_CertifyKey2(tpm_state_t *tpm_state,
                                   TPM_STORE_BUFFER *response,
                                   TPM_TAG tag,
                                   uint32_t paramSize,
                                   TPM_COMMAND_CODE ordinal,
                                   unsigned char *command,
                                   TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_CertifySelfTest(tpm_state_t *tpm_state,
                                       TPM_STORE_BUFFER *response,
                                       TPM_TAG tag,
                                       uint32_t paramSize,
                                       TPM_COMMAND_CODE ordinal,
                                       unsigned char *command,
                                       TPM_TRANSPORT_INTERNAL *transportInternal);


#endif
