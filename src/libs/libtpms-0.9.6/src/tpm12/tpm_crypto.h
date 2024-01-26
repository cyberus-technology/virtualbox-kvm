/********************************************************************************/
/*                                                                              */
/*                      Platform Dependent Crypto                               */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_crypto.h $            */
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

#ifndef TPM_CRYPTO_H
#define TPM_CRYPTO_H

#include "config.h"  /* libtpms added */

#include "tpm_secret.h"
#include "tpm_types.h"

/* self test */

TPM_RESULT TPM_Crypto_Init(void);
TPM_RESULT TPM_Crypto_TestSpecific(void);

/* random number */

TPM_RESULT TPM_Random(BYTE *buffer, size_t bytes);
TPM_RESULT TPM_StirRandomCmd(TPM_SIZED_BUFFER *inData);

/*
  bignum
*/

TPM_RESULT TPM_BN_num_bytes(unsigned int *numBytes, TPM_BIGNUM bn_in);
TPM_RESULT TPM_BN_is_one(TPM_BIGNUM bn_in);
TPM_RESULT TPM_BN_mod(TPM_BIGNUM rem_in,
		      const TPM_BIGNUM a_in,
		      const TPM_BIGNUM m_in);
TPM_RESULT TPM_BN_mask_bits(TPM_BIGNUM bn_in, unsigned int n);
TPM_RESULT TPM_BN_rshift(TPM_BIGNUM *rBignum_in,
                         TPM_BIGNUM aBignum_in,
                         int n);
TPM_RESULT TPM_BN_lshift(TPM_BIGNUM *rBignum_in,
                         TPM_BIGNUM aBignum_in,
                         int n);
TPM_RESULT TPM_BN_add(TPM_BIGNUM rBignum_in,
                      TPM_BIGNUM aBignum_in,
                      TPM_BIGNUM bBignum_in);
TPM_RESULT TPM_BN_mul(TPM_BIGNUM rBignum_in,
                      TPM_BIGNUM aBignum_in,
                      TPM_BIGNUM bBignum_in);
TPM_RESULT TPM_BN_mod_exp(TPM_BIGNUM rBignum_in,
                          TPM_BIGNUM aBignum_in,
                          TPM_BIGNUM pBignum_in,
                          TPM_BIGNUM nBignum_in);
TPM_RESULT TPM_BN_mod_mul(TPM_BIGNUM rBignum_in,
                          TPM_BIGNUM aBignum_in,
                          TPM_BIGNUM bBignum_in,
                          TPM_BIGNUM mBignum_in);
TPM_RESULT TPM_BN_mod_add(TPM_BIGNUM rBignum_in,
                          TPM_BIGNUM aBignum_in,
                          TPM_BIGNUM bBignum_in,
                          TPM_BIGNUM mBignum_in);

TPM_RESULT TPM_bin2bn(TPM_BIGNUM *bn_in,
		      const unsigned char *bin,
		      unsigned int bytes);
TPM_RESULT TPM_bn2bin(unsigned char *bin,
		      TPM_BIGNUM bn_in);

TPM_RESULT TPM_BN_new(TPM_BIGNUM *bn_in);
void 	   TPM_BN_free(TPM_BIGNUM bn_in);

/* RSA */
    
TPM_RESULT TPM_RSAGenerateKeyPair(unsigned char **n,
                                  unsigned char **p,
                                  unsigned char **q,
                                  unsigned char **d,
                                  int num_bit,
                                  const unsigned char *earr,
                                  uint32_t e_size);

TPM_RESULT TPM_RSAPrivateDecrypt(unsigned char *decrypt_data,
                                 uint32_t *decrypt_data_length,
                                 size_t decrypt_data_size,
                                 TPM_ENC_SCHEME encScheme,
                                 unsigned char* encrypt_data,
                                 uint32_t encrypt_data_size,
                                 unsigned char *n,
                                 uint32_t nbytes,
                                 unsigned char *e,
                                 uint32_t ebytes,
                                 unsigned char *d,
                                 uint32_t dbytes);

TPM_RESULT TPM_RSAPublicEncrypt(unsigned char* encrypt_data,
                                size_t encrypt_data_size,
                                TPM_ENC_SCHEME encScheme,
                                const unsigned char *decrypt_data,
                                size_t decrypt_data_size,
                                unsigned char *narr,
                                uint32_t nbytes,
                                unsigned char *earr,
                                uint32_t ebytes);
#if USE_FREEBL_CRYPTO_LIBRARY
TPM_RESULT TPM_RSAPublicEncryptRaw(unsigned char *encrypt_data,
				   uint32_t encrypt_data_size,
				   unsigned char *decrypt_data,
				   uint32_t decrypt_data_size,
				   unsigned char *narr,
				   uint32_t nbytes,
				   unsigned char *earr,
				   uint32_t ebytes);
#endif
    
TPM_RESULT TPM_RSAGetPrivateKey(uint32_t *qbytes, unsigned char **qarr,
                                uint32_t *dbytes, unsigned char **darr,
                                uint32_t nbytes, unsigned char *narr,
                                uint32_t ebytes, unsigned char *earr,
                                uint32_t pbytes, unsigned char *parr);
TPM_RESULT TPM_RSASign(unsigned char *signature,
                       unsigned int *signature_length,
                       unsigned int signature_size,
                       TPM_SIG_SCHEME sigScheme,
                       const unsigned char *message,
                       size_t message_size,
                       unsigned char *narr,
                       uint32_t nbytes,
                       unsigned char *earr,
                       uint32_t ebytes,
                       unsigned char *darr,
                       uint32_t dbytes);
TPM_RESULT TPM_RSAVerifySHA1(unsigned char *signature,
			     unsigned int signature_size,
			     const unsigned char *message,
			     uint32_t message_size,
			     unsigned char *narr,
			     uint32_t nbytes,
			     unsigned char *earr,
			     uint32_t ebytes);

/* SHA-1 */

TPM_RESULT TPM_SHA1InitCmd(void **context);
TPM_RESULT TPM_SHA1UpdateCmd(void *context, const unsigned char *data, uint32_t length);
TPM_RESULT TPM_SHA1FinalCmd(unsigned char *md, void *context);
void       TPM_SHA1Delete(void **context);

/* SHA-1 Context */

TPM_RESULT TPM_Sha1Context_Load(void **context,
				unsigned char **stream,
				uint32_t *stream_size);
TPM_RESULT TPM_Sha1Context_Store(TPM_STORE_BUFFER *sbuffer,
				 void *context);

/*
  TPM_SYMMETRIC_KEY_DATA
*/

TPM_RESULT TPM_SymmetricKeyData_New(TPM_SYMMETRIC_KEY_TOKEN *tpm_symmetric_key_data);
void       TPM_SymmetricKeyData_Free(TPM_SYMMETRIC_KEY_TOKEN *tpm_symmetric_key_data);
void       TPM_SymmetricKeyData_Init(TPM_SYMMETRIC_KEY_TOKEN tpm_symmetric_key_token);
TPM_RESULT TPM_SymmetricKeyData_Load(TPM_SYMMETRIC_KEY_TOKEN tpm_symmetric_key_token,
                                     unsigned char **stream,
                                     uint32_t *stream_size);
TPM_RESULT TPM_SymmetricKeyData_Store(TPM_STORE_BUFFER *sbuffer,
                                      const TPM_SYMMETRIC_KEY_TOKEN tpm_symmetric_key_token);
TPM_RESULT TPM_SymmetricKeyData_GenerateKey(TPM_SYMMETRIC_KEY_TOKEN tpm_symmetric_key_token);
TPM_RESULT TPM_SymmetricKeyData_Encrypt(unsigned char **encrypt_data,
                                        uint32_t *encrypt_length,
                                        const unsigned char *decrypt_data,
                                        uint32_t decrypt_length,
                                        const TPM_SYMMETRIC_KEY_TOKEN tpm_symmetric_key_token);
TPM_RESULT TPM_SymmetricKeyData_Decrypt(unsigned char **decrypt_data,
                                        uint32_t *decrypt_length,
                                        const unsigned char *encrypt_data,
                                        uint32_t encrypt_length,
                                        const TPM_SYMMETRIC_KEY_TOKEN tpm_symmetric_key_token);
TPM_RESULT TPM_SymmetricKeyData_CtrCrypt(unsigned char *data_out,
                                         const unsigned char *data_in,
                                         uint32_t data_size,
                                         const unsigned char *symmetric_key,
                                         uint32_t symmetric_key_size,
                                         const unsigned char *ctr_in,
                                         uint32_t ctr_in_size);
TPM_RESULT TPM_SymmetricKeyData_OfbCrypt(unsigned char *data_out,
                                         const unsigned char *data_in,
                                         uint32_t data_size,
                                         const unsigned char *symmetric_key,
                                         uint32_t symmetric_key_size,
                                         unsigned char *ivec_in,
                                         uint32_t ivec_in_size);
#endif
