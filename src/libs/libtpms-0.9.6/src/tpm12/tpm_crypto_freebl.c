/********************************************************************************/
/*                                                                              */
/*                      Platform Dependent Crypto                               */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_crypto_freebl.c $     */
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

/* This is the FreeBL implementation

   setenv CVSROOT :pserver:anonymous@cvsmirror.mozilla.org:/cvsroot
   cvs co mosilla/nsprpub
   gmake nss_build_all
*/

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "blapi.h"
#include <gmp.h>

#include "tpm_cryptoh.h"
#include "tpm_debug.h"
#include "tpm_error.h"
#include "tpm_key.h"
#include "tpm_io.h"
#include "tpm_load.h"
#include "tpm_memory.h"
#include "tpm_process.h"
#include "tpm_types.h"

#include "tpm_crypto.h"

/* The TPM OAEP encoding parameter */
static const unsigned char tpm_oaep_pad_str[] = { 'T', 'C', 'P', 'A' };

/* pre-calculate hash of the constant tpm_oaep_pad_str, used often in the OAEP padding
   calculations */
static const unsigned char pHashConst[TPM_DIGEST_SIZE];

/* ASN.1 industry standard SHA1 with RSA object identifier */
static unsigned char sha1Oid[] = {
    0x30, 0x21, 0x30, 0x09, 0x06, 0x05,
    0x2b, 0x0e, 0x03, 0x02, 0x1a, 0x05,
    0x00, 0x04, 0x14};


/*
  local prototypes
*/

static void       TPM_RSAPrivateKeyInit(RSAPrivateKey *rsa_pri_key);
static TPM_RESULT TPM_RSAGeneratePublicToken(RSAPublicKey *rsa_pub_key,
					     unsigned char *narr,
					     uint32_t nbytes,
					     unsigned char *earr,
					     uint32_t ebytes);
static TPM_RESULT TPM_RSAGeneratePrivateToken(RSAPrivateKey *rsa_pri_key,
					      unsigned char *narr,
					      uint32_t nbytes,
					      unsigned char *earr,
					      uint32_t ebytes,
					      unsigned char *darr,
					      uint32_t dbytes);
static TPM_RESULT TPM_RSASignSHA1(unsigned char *signature,
                                  unsigned int *signature_length,
                                  const unsigned char *message,
                                  size_t message_size,
                                  RSAPrivateKey *rsa_pri_key);
static TPM_RESULT TPM_RSASignDER(unsigned char *signature,
                                 unsigned int *signature_length,
                                 const unsigned char *message,  
                                 size_t message_size,
                                 RSAPrivateKey *rsa_pri_key);

static TPM_RESULT TPM_RandomNonZero(BYTE *buffer, size_t bytes);

static TPM_RESULT TPM_PKCS1_PaddingType1Add(unsigned char *output,
					    uint32_t outputLength,
					    const unsigned char *input,
					    uint32_t inputLength);
static TPM_RESULT TPM_PKCS1_PaddingType1Check(uint32_t *padLength,
					      unsigned char *input,
					      uint32_t inputLength);
static TPM_RESULT TPM_PKCS1_PaddingType2Add(unsigned char *encodedMessage,
					    uint32_t encodedMessageLength,
					    const unsigned char *message,
					    uint32_t messageLength);
static TPM_RESULT TPM_PKCS1_PaddingType2Check(unsigned char *outputData,
					      uint32_t *outputDataLength,
					      uint32_t outputDataSize,
					      unsigned char *inputData,
					      uint32_t inputDataLength);

static TPM_RESULT TPM_memcpyPad(unsigned char **bin_out,
				unsigned char *bin_in,
				uint32_t bin_in_length,
				uint32_t padBytes);


/* TPM_SYMMETRIC_KEY_DATA is a crypto library platform dependent symmetric key structure
 */

#ifdef TPM_AES

/* local prototype and structure for AES */

/* AES requires data lengths that are a multiple of the block size */
#define TPM_AES_BITS 128
/* The AES block size is always 16 bytes */
#define TPM_AES_BLOCK_SIZE 16

/* Since the AES key is often derived by truncating the session shared secret, test that it's not
   too large
*/

#if (TPM_AES_BLOCK_SIZE > TPM_SECRET_SIZE)
#error TPM_AES_BLOCK_SIZE larger than TPM_SECRET_SIZE
#endif

/* The AES initial CTR value is derived from a nonce. */

#if (TPM_AES_BLOCK_SIZE > TPM_NONCE_SIZE)
#error TPM_AES_BLOCK_SIZE larger than TPM_NONCE_SIZE
#endif

typedef struct tdTPM_SYMMETRIC_KEY_DATA {
    TPM_TAG tag;
    TPM_BOOL valid;
    TPM_BOOL fill;
    unsigned char userKey[TPM_AES_BLOCK_SIZE];
} TPM_SYMMETRIC_KEY_DATA;

#endif 	/* TPM_AES */

/*
  Crypto library Initialization function
*/
     
TPM_RESULT TPM_Crypto_Init()
{
    TPM_RESULT rc = 0;
    SECStatus rv = SECSuccess;

    printf("TPM_Crypto_Init: FreeBL library\n");
    /* initialize the random number generator */
    if (rc == 0) {
	printf(" TPM_Crypto_Init: Initializing RNG\n");
	rv = RNG_RNGInit();
	if (rv != SECSuccess) {
	    printf("TPM_Crypto_Init: Error (fatal), RNG_RNGInit rv %d\n", rv);
	    rc = TPM_FAIL;
	}
    }
    /* add additional seed  entropy to the random number generator */
    if (rc == 0) {
	printf(" TPM_Crypto_Init: Seeding RNG\n");
	RNG_SystemInfoForRNG();
    }
    if (rc == 0) {
	rv = BL_Init();
	if (rv != SECSuccess) {
	    printf("TPM_Crypto_Init: Error (fatal), BL_Init rv %d\n", rv);
	    rc =TPM_FAIL ;
	}
    }
    /* pre-calculate hash of the constant tpm_oaep_pad_str, used often in the OAEP padding
       calculations */
    if (rc == 0) {
	rc = TPM_SHA1((unsigned char *)pHashConst,	/* cast once to precalculate the constant */
		      sizeof(tpm_oaep_pad_str), tpm_oaep_pad_str,
		      0, NULL);
	TPM_PrintFour("TPM_Crypto_Init: pHashConst", pHashConst);
    }
    return rc;
}

/* TPM_Crypto_TestSpecific() performs any library specific tests

   For FreeBL
*/

TPM_RESULT TPM_Crypto_TestSpecific()
{
    TPM_RESULT          rc = 0;
   
    /* Saving the SHA-1 context is fragile code, so test at startup */
    void *context1;
    void *context2;
    unsigned char buffer1[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    unsigned char expect1[] = {0x84,0x98,0x3E,0x44,0x1C,
			       0x3B,0xD2,0x6E,0xBA,0xAE,
			       0x4A,0xA1,0xF9,0x51,0x29,
			       0xE5,0xE5,0x46,0x70,0xF1};
    TPM_DIGEST	actual;
    int		not_equal;
    TPM_STORE_BUFFER sbuffer;
    const unsigned char *stream;
    uint32_t stream_size;
    
    printf(" TPM_Crypto_TestSpecific: Test 1 - SHA1 two parts\n");
    context1 = NULL;			/* freed @1 */
    context2 = NULL;			/* freed @2 */
    TPM_Sbuffer_Init(&sbuffer);		/* freed @3 */
    
    if (rc== 0) {
	rc = TPM_SHA1InitCmd(&context1);				/* freed @1 */
    }
    /* digest the first part of the array */
    if (rc== 0) {
	rc = TPM_SHA1UpdateCmd(context1, buffer1, 16);
    }
    /* store the SHA1 context */
    if (rc== 0) {
	rc = TPM_Sha1Context_Store(&sbuffer, context1);
    }
    /* load the SHA1 context */
    if (rc== 0) {
	TPM_Sbuffer_Get(&sbuffer, &stream, &stream_size);
	rc = TPM_Sha1Context_Load
	     (&context2, (unsigned char **)&stream, &stream_size);	/* freed @2 */
    }
    /* digest the rest of the array */
    if (rc== 0) {
	rc = TPM_SHA1UpdateCmd(context2, buffer1 + 16, sizeof(buffer1) - 17);
    }
    /* get the digest result */
    if (rc== 0) {
	rc = TPM_SHA1FinalCmd(actual, context2);
    }
    /* check the result */
    if (rc == 0) {
	not_equal = memcmp(expect1, actual, TPM_DIGEST_SIZE);
	if (not_equal) {
	    printf("TPM_Crypto_TestSpecific: Error in test 1\n");
	    TPM_PrintFour("\texpect", expect1);
	    TPM_PrintFour("\tactual", actual);
	    rc = TPM_FAILEDSELFTEST;
	}
    }
    TPM_SHA1Delete(&context1);		/* @1 */
    TPM_SHA1Delete(&context2);		/* @2 */
    TPM_Sbuffer_Delete(&sbuffer);	/* @3 */
    return rc;
}

/*
  Random Number Functions
*/

/* TPM_Random() fills 'buffer' with 'bytes' bytes.
 */

TPM_RESULT TPM_Random(BYTE *buffer, size_t bytes)
{
    TPM_RESULT 	rc = 0;
    SECStatus 	rv = SECSuccess;

    printf(" TPM_Random: Requesting %lu bytes\n", (unsigned long)bytes);
    /* generate the random bytes */
    if (rc == 0) {
	rv = RNG_GenerateGlobalRandomBytes(buffer, bytes);
	if (rv != SECSuccess) {
	    printf("TPM_Random: Error (fatal) in RNG_GenerateGlobalRandomBytes rv %d\n", rv);
	    rc = TPM_FAIL;
	}
    }
    return rc;
}

/* TPM_Random() fills 'buffer' with 'bytes' non-zero bytes

   This is used for PKCS padding.
*/

static TPM_RESULT TPM_RandomNonZero(BYTE *buffer, size_t bytes)
{
    TPM_RESULT 	rc = 0;
    size_t 	i;
    SECStatus 	rv = SECSuccess;

    printf(" TPM_RandomNonZero: Requesting %lu bytes\n", (unsigned long)bytes);
    for (i = 0 ; (rc == 0) && (i < bytes) ; ) {
	rv = RNG_GenerateGlobalRandomBytes(buffer, 1);
	if (rv != SECSuccess) {
	    printf("TPM_Random: Error (fatal) in RNG_GenerateGlobalRandomBytes rv %d\n", rv);
	    rc = TPM_FAIL;
	}
	else {
	    if (*buffer != 0x00) {
		buffer++;
		i++;
	    }
	}
    }
    return rc;
}

/* TPM_StirRandomCmd() adds the supplied entropy to the random number generator
 */

TPM_RESULT TPM_StirRandomCmd(TPM_SIZED_BUFFER *inData)
{
    TPM_RESULT 	rc = 0;
    SECStatus 	rv = SECSuccess;

    printf(" TPM_StirRandomCmd:\n");
    if (rc == 0) {
	/* add the seeding material */
	rv = RNG_RandomUpdate(inData->buffer, inData->size);
	if (rv != SECSuccess) {
	    printf("TPM_StirRandom: Error (fatal) in RNG_RandomUpdate rv %d\n", rv);
	    rc = TPM_FAIL;
	} 
    }
    return rc;
}

/*
  RSA Functions
*/

/* TPM_RSAPrivateKeyInit() NULLs all the structure members in preparation for constructing an RSA
   key token from byte arrays using RSA_PopulatePrivateKey()
*/

static void TPM_RSAPrivateKeyInit(RSAPrivateKey *rsa_pri_key)
{
    rsa_pri_key->arena = NULL;
    rsa_pri_key->publicExponent.type = siBuffer;
    rsa_pri_key->publicExponent.data = NULL;
    rsa_pri_key->publicExponent.len = 0;
    rsa_pri_key->modulus.type = siBuffer;
    rsa_pri_key->modulus.data = NULL;
    rsa_pri_key->modulus.len = 0;
    rsa_pri_key->privateExponent.type = siBuffer;
    rsa_pri_key->privateExponent.data = NULL;
    rsa_pri_key->privateExponent.len = 0;
    rsa_pri_key->prime1.type = siBuffer;
    rsa_pri_key->prime1.data = NULL;
    rsa_pri_key->prime1.len = 0;
    rsa_pri_key->prime2.type = siBuffer;
    rsa_pri_key->prime2.data = NULL;
    rsa_pri_key->prime2.len = 0;
    rsa_pri_key->exponent1.type = siBuffer;
    rsa_pri_key->exponent1.data = NULL;
    rsa_pri_key->exponent1.len = 0;
    rsa_pri_key->exponent2.type = siBuffer;
    rsa_pri_key->exponent2.data = NULL;
    rsa_pri_key->exponent2.len = 0;
    rsa_pri_key->coefficient.type = siBuffer;
    rsa_pri_key->coefficient.data = NULL;
    rsa_pri_key->coefficient.len = 0;
    return;
}

/* Generate an RSA key pair of size 'num_bits' using public exponent 'earr'

   'n', 'p', 'q', 'd' must be freed by the caller
*/

TPM_RESULT TPM_RSAGenerateKeyPair(unsigned char **n,            /* public key - modulus */
                                  unsigned char **p,            /* private key prime */
                                  unsigned char **q,            /* private key prime */
                                  unsigned char **d,            /* private key (private exponent) */
                                  int num_bits,                 /* key size in bits */
                                  const unsigned char *earr,    /* public exponent as an array */
                                  uint32_t e_size)
{
    TPM_RESULT rc = 0;
    SECItem publicExponent = { 0, 0, 0};
    RSAPrivateKey *rsaPrivateKey = NULL;  	/* freed @1 */
    unsigned long e;				/* public exponent */

    printf(" TPM_RSAGenerateKeyPair:\n");
    /* initialize in case of error */
    *n = NULL;
    *p = NULL;
    *q = NULL;
    *d = NULL;
    /* check that num_bits is a multiple of 16.  If not, the primes p and q will not be a multiple
       of 8 and will not fit well in a byte */
    if (rc == 0) {
	if ((num_bits % 16) != 0) {
	    printf("TPM_RSAGenerateKeyPair: Error, num_bits %d is not a multiple of 16\n",
		   num_bits);
	    rc = TPM_BAD_KEY_PROPERTY;
	}
    }
    /* convert the e array to an unsigned long */
    if (rc == 0) {
        rc = TPM_LoadLong(&e, earr, e_size);
    }
    /* validate the public exponent against a list of legal values.  Some values (e.g. even numbers)
       can hang the key generator. */
    if (rc == 0) {
	rc = TPM_RSA_exponent_verify(e);
    }
    /* generate the key pair */
    if (rc == 0) {
        printf("  TPM_RSAGenerateKeyPair: num_bits %d exponent %08lx\n", num_bits, e);
	publicExponent.type = siBuffer;
	publicExponent.data = (unsigned char *)earr;
	publicExponent.len = e_size;
	/* Generate and return a new RSA public and private key token */
	rsaPrivateKey = RSA_NewKey(num_bits, &publicExponent);  /* freed @1 */
	if (rsaPrivateKey == NULL) {
            printf("TPM_RSAGenerateKeyPair: Error (fatal) calling RSA_NewKey()\n");
            rc = TPM_FAIL;
        }
    }
    /* Key parts can some times have leading zeros, and some crypto libraries truncate.  However,
       the TPM expects fixed lengths.  These calls restore any removed padding */
    /* load n */
    if (rc == 0) {
	rc = TPM_memcpyPad(n,				/* freed by caller */
			   rsaPrivateKey->modulus.data,
			   rsaPrivateKey->modulus.len,
			   num_bits/8);			/* required length */
    }
    /* load p */
    if (rc == 0) {
	rc = TPM_memcpyPad(p,				/* freed by caller */
			   rsaPrivateKey->prime1.data,
			   rsaPrivateKey->prime1.len,
			   num_bits/16);		/* required length */
    }
    /* load q */
    if (rc == 0) {
	rc = TPM_memcpyPad(q,				/* freed by caller */
			   rsaPrivateKey->prime2.data,
			   rsaPrivateKey->prime2.len,
			   num_bits/16);		/* required length */
    }
    /* load d */
    if (rc == 0) {
	rc = TPM_memcpyPad(d,				/* freed by caller */
			   rsaPrivateKey->privateExponent.data,
			   rsaPrivateKey->privateExponent.len,
			   num_bits/8);			/* required length */
    }
    /* on error, free the components and set back to NULL so subsequent free is safe */
    if (rc != 0) {
        free(*n);
        free(*p);
        free(*q);
        free(*d);
        *n = NULL;
        *p = NULL;
        *q = NULL;
        *d = NULL;
    }
    if (rsaPrivateKey != NULL) {
	PORT_FreeArena(rsaPrivateKey->arena, PR_TRUE);  /* @1 */
    }
    return rc;
}

/* TPM_RSAGeneratePublicToken() generates an RSA key token from n and e
 */

static TPM_RESULT TPM_RSAGeneratePublicToken(RSAPublicKey *rsaPublicKey,
					     unsigned char *narr,      	/* public modulus */
					     uint32_t nbytes,
					     unsigned char *earr,      	/* public exponent */
					     uint32_t ebytes)
{
    TPM_RESULT  rc = 0;

    /* simply assign the arrays to the key token */
    if (rc == 0) {
	printf(" TPM_RSAGeneratePublicToken: nbytes %u ebytes %u\n", nbytes, ebytes);
	rsaPublicKey->arena = NULL;
      	/* public modulus */
	rsaPublicKey->modulus.type = siBuffer;
	rsaPublicKey->modulus.data = narr;
	rsaPublicKey->modulus.len = nbytes;
	/* public exponent */
	rsaPublicKey->publicExponent.type = siBuffer;
	rsaPublicKey->publicExponent.data = earr;
	rsaPublicKey->publicExponent.len = ebytes;
    }
    return rc;
}

/* TPM_RSAGeneratePrivateToken() generates an RSA key token from n, e, d
 */

static TPM_RESULT TPM_RSAGeneratePrivateToken(RSAPrivateKey *rsa_pri_key, /* freed by caller */
					      unsigned char *narr,      /* public modulus */
					      uint32_t nbytes,
					      unsigned char *earr,      /* public exponent */
					      uint32_t ebytes,
					      unsigned char *darr,	/* private exponent */
					      uint32_t dbytes)
{
    TPM_RESULT  rc = 0;
    SECStatus rv = SECSuccess;

    printf("  TPM_RSAGeneratePrivateToken:\n");
    if (rc == 0) {
	rsa_pri_key->arena = NULL;
	/* public exponent */
	rsa_pri_key->publicExponent.type = siBuffer;
	rsa_pri_key->publicExponent.data = earr;
	rsa_pri_key->publicExponent.len = ebytes;
      	/* public modulus */
	rsa_pri_key->modulus.type = siBuffer;
	rsa_pri_key->modulus.data = narr;
	rsa_pri_key->modulus.len = nbytes;
	/* private exponent */
	rsa_pri_key->privateExponent.type = siBuffer;
	rsa_pri_key->privateExponent.data = darr;
	rsa_pri_key->privateExponent.len = dbytes;
	/* given these key parameters (n,e,d), fill in the rest of the parameters */
	rv = RSA_PopulatePrivateKey(rsa_pri_key); 	/* freed by caller */
	if (rv != SECSuccess) {
	    printf("TPM_RSAGeneratePrivateToken: Error, RSA_PopulatePrivateKey rv %d\n", rv);
	    rc = TPM_BAD_PARAMETER;
	}
    }
    return rc;
}

/* TPM_RSAPrivateDecrypt() decrypts 'encrypt_data' using the private key 'n, e, d'.  The OAEP
   padding is removed and 'decrypt_data_length' bytes are moved to 'decrypt_data'.

   'decrypt_data_length' is at most 'decrypt_data_size'.
*/

TPM_RESULT TPM_RSAPrivateDecrypt(unsigned char *decrypt_data,   /* decrypted data */
                                 uint32_t *decrypt_data_length,	/* length of data put into
                                                                   decrypt_data */
                                 size_t decrypt_data_size,      /* size of decrypt_data buffer */
                                 TPM_ENC_SCHEME encScheme,      /* encryption scheme */
                                 unsigned char* encrypt_data,   /* encrypted data */
                                 uint32_t encrypt_data_size,
                                 unsigned char *narr,           /* public modulus */
                                 uint32_t nbytes,
                                 unsigned char *earr,           /* public exponent */
                                 uint32_t ebytes,
                                 unsigned char *darr,           /* private exponent */
                                 uint32_t dbytes)
{
    TPM_RESULT  	rc = 0;
    SECStatus 		rv = SECSuccess;
    RSAPrivateKey	rsa_pri_key;
    unsigned char       *padded_data = NULL;	/* freed @2 */
    int                 padded_data_size = 0;

    printf(" TPM_RSAPrivateDecrypt: Input data size %u\n", encrypt_data_size);
    TPM_RSAPrivateKeyInit(&rsa_pri_key);			/* freed @1 */
    /* the encrypted data size must equal the public key size */
    if (rc == 0) {
	if (encrypt_data_size != nbytes) {
	    printf("TPM_RSAPrivateDecrypt: Error, Encrypted data size is %u not %u\n",
		   encrypt_data_size, nbytes);
	    rc = TPM_DECRYPT_ERROR;
	}
    }
    /* construct the freebl private key object from n,e,d */
    if (rc == 0) {
	rc = TPM_RSAGeneratePrivateToken(&rsa_pri_key,	/* freed @1 */
					 narr,      	/* public modulus */
					 nbytes,
					 earr,      	/* public exponent */
					 ebytes,
					 darr,		/* private exponent */
					 dbytes);
    }
    /* allocate intermediate buffer for the decrypted but still padded data */
    if (rc == 0) {
        /* the size of the decrypted data is guaranteed to be less than this */
        padded_data_size = rsa_pri_key.modulus.len;
        rc = TPM_Malloc(&padded_data, padded_data_size);	/* freed @2 */
    }
    if (rc == 0) {
        /* decrypt with private key.  Must decrypt first and then remove padding because the decrypt
           call cannot specify an encoding parameter */
	rv = RSA_PrivateKeyOp(&rsa_pri_key,		/* private key token */
			      padded_data,		/* to - the decrypted but padded data */
			      encrypt_data);		/* from - the encrypted data */
	if (rv != SECSuccess) {
	    printf("TPM_RSAPrivateDecrypt: Error in RSA_PrivateKeyOp(), rv %d\n", rv);
	    rc = TPM_DECRYPT_ERROR;
	}
   }
    if (rc == 0) {
        printf("  TPM_RSAPrivateDecrypt: RSA_PrivateKeyOp() success\n");
        printf("  TPM_RSAPrivateDecrypt: Padded data size %u\n", padded_data_size);
        TPM_PrintFour("  TPM_RSAPrivateDecrypt: Decrypt padded data", padded_data);
	/* check and remove the padding based on the TPM encryption scheme */
        if (encScheme == TPM_ES_RSAESOAEP_SHA1_MGF1) {
	    /* recovered seed and pHash are not returned */
	    unsigned char seed[TPM_DIGEST_SIZE];
	    unsigned char pHash[TPM_DIGEST_SIZE];
	    if (rc == 0) {
		/* the padded data skips the first 0x00 byte, since it expects the
		   padded data to come from a truncated bignum */
		rc = TPM_RSA_padding_check_PKCS1_OAEP(decrypt_data,		/* to */
						      decrypt_data_length,	/* to length */
						      decrypt_data_size, 	/* to buffer size */
						      padded_data + 1,		/* from */
						      padded_data_size - 1,	/* from length */
						      pHash,			/* 20 bytes */
						      seed);			/* 20 bytes */
	    }
        }
        else if (encScheme == TPM_ES_RSAESPKCSv15) {
            rc = TPM_PKCS1_PaddingType2Check(decrypt_data,          	/* to */
					     decrypt_data_length,	/* to length */	
					     decrypt_data_size,     	/* to buffer size*/
					     padded_data,       	/* from */
					     padded_data_size);  	/* from length */
        }
        else {
            printf("TPM_RSAPrivateDecrypt: Error, unknown encryption scheme %04x\n", encScheme);
            rc = TPM_INAPPROPRIATE_ENC;
        }
    }
    if (rc == 0) {
        printf("  TPM_RSAPrivateDecrypt: RSA_padding_check_PKCS1 recovered %d bytes\n",
	       *decrypt_data_length);
        TPM_PrintFourLimit("  TPM_RSAPrivateDecrypt: Decrypt data", decrypt_data, decrypt_data_size);
    }
    PORT_FreeArena(rsa_pri_key.arena, PR_TRUE);	/* @1 */
    free(padded_data);                  	/* @2 */
    return rc;
}

/* TPM_RSAPublicEncrypt() PKCS1 pads 'decrypt_data' to 'encrypt_data_size' and encrypts using the
   public key 'n, e'.
*/

TPM_RESULT TPM_RSAPublicEncrypt(unsigned char *encrypt_data,    /* encrypted data */
                                size_t encrypt_data_size,       /* size of encrypted data buffer */
                                TPM_ENC_SCHEME encScheme,	/* padding type */
                                const unsigned char *decrypt_data,      /* decrypted data */
                                size_t decrypt_data_size,
                                unsigned char *narr,           /* public modulus */
                                uint32_t nbytes,
                                unsigned char *earr,           /* public exponent */
                                uint32_t ebytes)
{
    TPM_RESULT  	rc = 0;
    unsigned char 	*padded_data = NULL;			/* freed @1 */
    
    printf(" TPM_RSAPublicEncrypt: Input data size %lu\n", (unsigned long)decrypt_data_size);
    /* intermediate buffer for the padded decrypted data */
    if (rc == 0) {
        rc = TPM_Malloc(&padded_data, encrypt_data_size);	/* freed @1 */
    }
    /* pad the decrypted data */
    if (rc == 0) {
	/* based on the TPM encryption scheme */
        if (encScheme == TPM_ES_RSAESOAEP_SHA1_MGF1) {
	    unsigned char seed[TPM_DIGEST_SIZE];
	    if (rc == 0) {
		rc = TPM_Random(seed, TPM_DIGEST_SIZE);
	    }
	    if (rc == 0) {
		padded_data[0] = 0x00;
		rc = TPM_RSA_padding_add_PKCS1_OAEP(padded_data +1,            	/* to */
						    encrypt_data_size -1,      	/* to length */
						    decrypt_data,              	/* from */
						    decrypt_data_size,         	/* from length */
						    pHashConst,			/* 20 bytes */
						    seed);			/* 20 bytes */
	    }
	}
        else if (encScheme == TPM_ES_RSAESPKCSv15) {
            rc = TPM_PKCS1_PaddingType2Add(padded_data,			/* to */
					   encrypt_data_size,		/* to length */
					   decrypt_data,		/* from */
					   decrypt_data_size);		/* from length */
        }
        else {
            printf("TPM_RSAPublicEncrypt: Error, unknown encryption scheme %04x\n", encScheme);
            rc = TPM_INAPPROPRIATE_ENC;
        }
    }
    /* raw public key operation on the already padded input data */
    if (rc == 0) {
	rc = TPM_RSAPublicEncryptRaw(encrypt_data,	/* output */
				     encrypt_data_size,	/* input, size of enc buffer */
				     padded_data,	/* input */
				     encrypt_data_size,	/* input, size of dec buffer */
				     narr,		/* public modulus */
				     nbytes,
				     earr,		/* public exponent */
				     ebytes);
    }
    free(padded_data);                  /* @1 */
    return rc;
}

/* TPM_RSAPublicEncryptRaw() does a raw public key operation without any padding.
   
*/

TPM_RESULT TPM_RSAPublicEncryptRaw(unsigned char *encrypt_data,	/* output */
				   uint32_t encrypt_data_size,	/* input, size of enc buffer */
				   unsigned char *decrypt_data,	/* input */
				   uint32_t decrypt_data_size,	/* input, size of dec buffer */
				   unsigned char *narr,		/* public modulus */
				   uint32_t nbytes,
				   unsigned char *earr,		/* public exponent */
				   uint32_t ebytes)
{
    TPM_RESULT          rc = 0;
    SECStatus 		rv = SECSuccess;
    RSAPublicKey        rsa_pub_key;

    printf("   TPM_RSAPublicEncryptRaw:\n");
    /* the input data size must equal the public key size (already padded) */
    if (rc == 0) {
	if (decrypt_data_size != nbytes) {
	    printf("TPM_RSAPublicEncryptRaw: Error, decrypt data size is %u not %u\n",
		   decrypt_data_size, nbytes);
	    rc = TPM_ENCRYPT_ERROR;
	}
    }
    /* the output data size must equal the public key size */
    if (rc == 0) {
	if (encrypt_data_size != nbytes) {
	    printf("TPM_RSAPublicEncryptRaw: Error, Output data size is %u not %u\n",
		   encrypt_data_size, nbytes);
	    rc = TPM_ENCRYPT_ERROR;
	}
    }
    /* construct the freebl public key object */
    if (rc == 0) {
	rc = TPM_RSAGeneratePublicToken(&rsa_pub_key,	/* freebl public key token */
					narr,      	/* public modulus */
					nbytes,
					earr,      	/* public exponent */
					ebytes);
    }
    if (rc == 0) {
        TPM_PrintFour("  TPM_RSAPublicEncryptRaw: Public modulus", narr);
        TPM_PrintAll("  TPM_RSAPublicEncryptRaw: Public exponent", earr, ebytes);
        TPM_PrintFourLimit("  TPM_RSAPublicEncryptRaw: Decrypt data", decrypt_data, decrypt_data_size);
        /* raw public key operation, encrypt the decrypt_data */
	rv = RSA_PublicKeyOp(&rsa_pub_key,	/* freebl public key token */
			     encrypt_data,      /* output - the encrypted data */
			     decrypt_data);     /* input - the clear text data */
	if (rv != SECSuccess) {
	    printf("TPM_RSAPublicEncrypt: Error in RSA_PublicKeyOp, rv %d\n", rv);
	    rc = TPM_ENCRYPT_ERROR;
	}
    }
    if (rc == 0) {
        TPM_PrintFour("  TPM_RSAPublicEncryptRaw: Encrypt data", encrypt_data);
#if 0	/* NOTE: Uncomment as a debug aid for signature verification */
        TPM_PrintAll("  TPM_RSAPublicEncryptRaw: Encrypt data",
		     encrypt_data, encrypt_data_size);
#endif
    }
    return rc;
}

/* TPM_RSASign() signs 'message' of size 'message_size' using the private key n,e,d and the
   signature scheme 'sigScheme' as specified in PKCS #1 v2.0.

   'signature_length' bytes are moved to 'signature'.  'signature_length' is at most
   'signature_size'.  signature must point to bytes of memory equal to the public modulus size.
*/

TPM_RESULT TPM_RSASign(unsigned char *signature,        /* output */
                       unsigned int *signature_length,  /* output, size of signature */
                       unsigned int signature_size,     /* input, size of signature buffer */
                       TPM_SIG_SCHEME sigScheme,        /* input, type of signature */
                       const unsigned char *message,    /* input */
                       size_t message_size,             /* input */
                       unsigned char *narr,             /* public modulus */
                       uint32_t nbytes,
                       unsigned char *earr,             /* public exponent */
                       uint32_t ebytes,
                       unsigned char *darr,             /* private exponent */
                       uint32_t dbytes)
{
    TPM_RESULT          rc = 0;
    RSAPrivateKey 	rsa_pri_key;

    printf(" TPM_RSASign:\n");
    TPM_RSAPrivateKeyInit(&rsa_pri_key);			/* freed @1 */
    /* construct the free private key object from n,e,d */
    if (rc == 0) {
	rc = TPM_RSAGeneratePrivateToken(&rsa_pri_key,	/* freed @1 */
					 narr,      	/* public modulus */
					 nbytes,
					 earr,      	/* public exponent */
					 ebytes,
					 darr,		/* private exponent */
					 dbytes);
    }
    /* sanity check the size of the output signature buffer */
    if (rc == 0) {
        if (signature_size < nbytes) {
            printf("TPM_RSASign: Error (fatal), buffer %u too small for signature %u\n",
                   signature_size, nbytes);
            rc = TPM_FAIL;      /* internal error, should never occur */
        }
    }
    /* determine the signature scheme for the key */
    if (rc == 0) {
        switch(sigScheme) {
          case TPM_SS_NONE:
            printf("TPM_RSASign: Error, sigScheme TPM_SS_NONE\n");
            rc = TPM_INVALID_KEYUSAGE;
            break;
          case TPM_SS_RSASSAPKCS1v15_SHA1:
          case TPM_SS_RSASSAPKCS1v15_INFO:
            rc = TPM_RSASignSHA1(signature,
                                 signature_length,
                                 message,
                                 message_size,
                                 &rsa_pri_key);
            break;
          case TPM_SS_RSASSAPKCS1v15_DER:
            rc = TPM_RSASignDER(signature,
                                signature_length,
                                message,
                                message_size,
                                &rsa_pri_key);
            break;
          default:
            printf("TPM_RSASign: Error, sigScheme %04hx unknown\n", sigScheme);
            rc = TPM_INVALID_KEYUSAGE;
            break;
        }
    }
    PORT_FreeArena(rsa_pri_key.arena, PR_TRUE);		/* @1 */
    return rc;
}

/* TPM_RSASignSHA1() performs the following:
        prepend a DER encoded algorithm ID (SHA1 and RSA)
        prepend a type 1 pad
        encrypt with the private key
*/

static TPM_RESULT TPM_RSASignSHA1(unsigned char *signature,             /* output */
                                  unsigned int *signature_length, /* output, size of signature */
                                  const unsigned char *message,         /* input */
                                  size_t message_size,                  /* input */
                                  RSAPrivateKey *rsa_pri_key)           /* signing private key */
{
    TPM_RESULT  rc = 0;
    unsigned char *message_der;	         /* DER padded message, freed @1 */

    printf(" TPM_RSASignSHA1: key size %d\n", rsa_pri_key->modulus.len);
    message_der = NULL;         	/* freed @1 */
    /* sanity check, SHA1 messages must be 20 bytes */
    if (rc == 0) {
        if (message_size != TPM_DIGEST_SIZE) {
            printf("TPM_RSASignSHA1: Error, message size %lu not TPM_DIGEST_SIZE\n",
                   (unsigned long)message_size );
            rc = TPM_DECRYPT_ERROR;
        } 
    }
    /* allocate memory for the DER padded message */
    if (rc == 0) {
	rc = TPM_Malloc(&message_der, sizeof(sha1Oid) + message_size);	/* freed @1 */
    }
    if (rc == 0) {
	/* copy the OID */
	memcpy(message_der, sha1Oid, sizeof(sha1Oid));
	/* copy the message */
	memcpy(message_der + sizeof(sha1Oid), message, message_size);
	/* sign the DER padded message */
	rc = TPM_RSASignDER(signature,          		/* output */
			    signature_length, 			/* output, size of signature */
			    message_der,			/* input */
			    sizeof(sha1Oid) + message_size,	/* input */
			    rsa_pri_key);			/* signing private key */
    }
    free(message_der);		/* @1 */
    return rc;
}

/* TPM_RSASignDER() performs the following:
   
        prepend a PKCS1 type 1 pad
        encrypt with the private key

   The caller must ensure that the signature buffer is >= the key size.
*/

static TPM_RESULT TPM_RSASignDER(unsigned char *signature,              /* output */
                                 unsigned int *signature_length, /* output, size of signature */
                                 const unsigned char *message,          /* input */
                                 size_t message_size,                   /* input */
                                 RSAPrivateKey *rsa_pri_key)            /* signing private key */
{
    TPM_RESULT  rc = 0;
    SECStatus 	rv = SECSuccess;
    unsigned char *message_pad;		/* PKCS1 type 1 padded message, freed @1 */
    
    printf(" TPM_RSASignDER: key size %d\n", rsa_pri_key->modulus.len);
    message_pad = NULL;         /* freed @1 */
    /* the padded message size is the same as the key size */
    /* allocate memory for the padded message */
    if (rc == 0) {
        rc = TPM_Malloc(&message_pad, rsa_pri_key->modulus.len);	/* freed @1 */
    }
    /* PKCS1 type 1 pad the message */
    if (rc == 0) {
        printf("  TPM_RSASignDER: Applying PKCS1 type 1 padding, size from %lu to %u\n",
               (unsigned long)message_size, rsa_pri_key->modulus.len);
        TPM_PrintFourLimit("  TPM_RSASignDER: Input message", message, message_size);
        /* This call checks that the message will fit with the padding */
	rc = TPM_PKCS1_PaddingType1Add(message_pad,         		/* to */
				       rsa_pri_key->modulus.len,	/* to length */
				       message,             		/* from */
				       message_size);			/* from length */
    }
    /* raw sign with private key */
    if (rc == 0) {
        printf("  TPM_RSASignDER: Encrypting with private key, message size %d\n",
	       rsa_pri_key->modulus.len);
        TPM_PrintFour("  TPM_RSASignDER: Padded message", message_pad);
        /* sign with private key */
	rv = RSA_PrivateKeyOp(rsa_pri_key,		/* freebl key token */
			      signature,		/* to - the decrypted but padded data */
			      message_pad);		/* from - the encrypted data */
	if (rv != SECSuccess) {
	    printf("TPM_RSASignDER: Error in RSA_PrivateKeyOp(), rv %d\n", rv);
	    rc = TPM_DECRYPT_ERROR;
	}
    }
    if (rc == 0) {
        TPM_PrintFour("  TPM_RSASignDER: signature", signature);
	*signature_length = rsa_pri_key->modulus.len;
    }
    free(message_pad);          /* @1 */
    return rc;
}

/* TPM_RSAVerifySHA1() performs the following:
        decrypt the signature
        verify and remove type 1 pad
        verify and remove DER encoded algorithm ID
        verify the signature on the message
*/

TPM_RESULT TPM_RSAVerifySHA1(unsigned char *signature,          /* input */
			     unsigned int signature_size, 	/* input, size of signature
								   buffer */
			     const unsigned char *message,      /* input */
			     uint32_t message_size,		/* input */
			     unsigned char *narr,		/* public modulus */
			     uint32_t nbytes,
			     unsigned char *earr,		/* public exponent */
			     uint32_t ebytes) 
{
    TPM_RESULT  	rc = 0;
    unsigned char       *padded_data = NULL;	/* decrypted signature, freed @1 */
    uint32_t 		padLength;
    int 		irc;
    
    printf(" TPM_RSAVerifySHA1:\n");
    /* allocate memory for the padded result of the public key operation */
    if (rc == 0) {
	rc = TPM_Malloc(&padded_data, nbytes);	/* freed @1 */
    }
    /* do a raw encrypt of the signature */
    if (rc == 0) {
	rc = TPM_RSAPublicEncryptRaw(padded_data,	/* output */
				     nbytes,		/* input, size of message buffer */
				     signature,		/* input */
				     signature_size,	/* input, size of signature buffer */
				     narr,		/* public modulus */
				     nbytes,
				     earr,		/* public exponent */
				     ebytes);
    }
    /* check PKCS1 padding and OID */
    if (rc == 0) {
	rc = TPM_PKCS1_PaddingType1Check(&padLength,	/* length of the PKCS1 padd and OID */
					 padded_data,	/* input data */
					 nbytes);	/* input data length */
    }
    /* check message length */
    if (rc == 0) {
	if (message_size != (nbytes - padLength)) {
	    printf("TPM_RSAVerifySHA1: Error, "
                   "message size %u not equal to size %u after padding removed\n",
		   message_size, nbytes - padLength);
	    rc = TPM_BAD_SIGNATURE;
	}
    }
    /* check message */
    if (rc == 0) {
	irc = memcmp(message, padded_data + padLength, message_size);
	if (irc != 0) {
	    printf("TPM_RSAVerifySHA1: Error, message mismatch\n");
	    TPM_PrintFourLimit(" TPM_RSAVerifySHA1: message", message, message_size);
	    TPM_PrintFourLimit(" TPM_RSAVerifySHA1: message from signature", padded_data + padLength, message_size);
	    rc = TPM_BAD_SIGNATURE;
	}
    }
    /* public encrypt is general, here we're doing a signature check, so adjust the error message */
    else {
	rc = TPM_BAD_SIGNATURE;
    }
    free(padded_data); 		/* @1 */
    return rc;
}

/* TPM_RSAGetPrivateKey calculates q (2nd prime factor) and d (private key) from n (public key), e
   (public exponent), and p (1st prime factor)

   'qarr', darr' must be freed by the caller.
*/

TPM_RESULT TPM_RSAGetPrivateKey(uint32_t *qbytes, unsigned char **qarr,
                                uint32_t *dbytes, unsigned char **darr,
                                uint32_t nbytes, unsigned char *narr,
                                uint32_t ebytes, unsigned char *earr,
                                uint32_t pbytes, unsigned char *parr)
{
    TPM_RESULT          rc = 0;
    SECStatus 		rv = SECSuccess;
    RSAPrivateKey 	rsa_pri_key;

    /* set to NULL so caller can free after failure */
    printf(" TPM_RSAGetPrivateKey:\n");
    TPM_RSAPrivateKeyInit(&rsa_pri_key);	/* freed @1 */
    *qarr = NULL;
    *darr = NULL;
    /* check input parameters */
    if (rc == 0) {
        if ((narr == NULL) || (nbytes == 0)) {
            printf("TPM_RSAGetPrivateKey: Error, missing n\n");
            rc = TPM_BAD_PARAMETER;
        }
    }
    /* check input parameters */
    if (rc == 0) {
        if ((earr == NULL) || (ebytes == 0)) {
            printf("TPM_RSAGetPrivateKey: Error, missing e\n");
            rc = TPM_BAD_PARAMETER;
        }
    }
    /* check input parameters */
    if (rc == 0) {
        if ((parr == NULL) || (pbytes == 0)) {
            printf("TPM_RSAGetPrivateKey: Error, missing p\n");
            rc = TPM_BAD_PARAMETER;
        }
    }
    /* populate the private key token with n, e, p */
    if (rc == 0) {
	rsa_pri_key.publicExponent.type = siBuffer;
	rsa_pri_key.publicExponent.data = earr;
	rsa_pri_key.publicExponent.len = ebytes;
	rsa_pri_key.modulus.type = siBuffer;
	rsa_pri_key.modulus.data = narr;
	rsa_pri_key.modulus.len = nbytes;
	rsa_pri_key.prime1.type = siBuffer;
	rsa_pri_key.prime1.data = parr;
	rsa_pri_key.prime1.len = pbytes;
	/* fill in the rest of the freebl key token parameters. */
	rv = RSA_PopulatePrivateKey(&rsa_pri_key); 	/* freed @1 */
	if (rv != SECSuccess) {
	    printf("TPM_RSAGetPrivateKey: Error in RSA_PopulatePrivateKey rv %d\n", rv);
	    rc = TPM_BAD_PARAMETER;
	}
    }
    /* extract and pad q */
    if (rc == 0) {
	rc = TPM_memcpyPad(qarr,			/* freed by caller */
			   rsa_pri_key.prime2.data, rsa_pri_key.prime2.len,
			   pbytes);			/* pad to p prime */
	*qbytes = pbytes;
    }
    /* extract and pad d */
    if (rc == 0) {
	rc = TPM_memcpyPad(darr,			/* freed by caller */
			   rsa_pri_key.privateExponent.data, rsa_pri_key.privateExponent.len,
			   nbytes);			/* pad to public modulus */
	*dbytes = nbytes;
    }
    if (rc == 0) {
        TPM_PrintFour("  TPM_RSAGetPrivateKey: Calculated q",  *qarr);
        TPM_PrintFour("  TPM_RSAGetPrivateKey: Calculated d",  *darr);
        printf("  TPM_RSAGetPrivateKey: length of n,p,q,d = %u / %u / %u / %u\n",
               nbytes, pbytes, *qbytes, *dbytes);
    }
    PORT_FreeArena(rsa_pri_key.arena, PR_TRUE);		/* @1 */
    return rc;
}

/*
  PKCS1 Padding Functions
*/

/* TPM_PKCS1_PaddingType1Add() adds PKCS1 type 1 padding.

   The output buffer is preallocated.
*/
    
static TPM_RESULT TPM_PKCS1_PaddingType1Add(unsigned char *output,	/* to */
					    uint32_t outputLength,
					    const unsigned char *input,	/* from */
					    uint32_t inputLength)
{
    TPM_RESULT 	rc = 0;
    uint32_t	psLength;
    uint32_t	index;

    /* sanity check the length, this should never fail */
    printf("   TPM_PKCS1_PaddingType1Add:\n");
    if (rc == 0) {
	if ((inputLength + 11) > outputLength) {
            printf("TPM_PKCS1_PaddingType1Add: Error, input %u too big for output %u\n",
                   inputLength, outputLength);
	    rc = TPM_DECRYPT_ERROR;
	}
    }
    if (rc == 0) {
	index = 0;
	/* psLength is the number of 0xff bytes, subtract 3 for the leading 00,01 and trailing 00 */
	psLength = outputLength - inputLength - 3;
	
	/* add the PKCS1 pad 01 || PS || 00 || T  where PS is at least 8 0xff bytes */
	/* PKCS1 pads to k-1 bytes, implies a leading 0 */
	output[index] = 0x00;
	index++;
	
	output[index] = 0x01;
	index++;
	
	memset(output + index, 0xff, psLength);
	index += psLength;
	
	output[index] = 0x00;
	index++;

	/* add the input data */
	memcpy(output + index, input, inputLength);
	index += inputLength;
    }	 
    return rc;
}

/* TPM_PKCS1_PaddingType1Check() checks PKCS1 type 1 padding and the SHA1withRSA OID
   and returns their length

   Type 1 is: 00 01 FF's 00 OID message
*/
    
static TPM_RESULT TPM_PKCS1_PaddingType1Check(uint32_t *padLength,
					      unsigned char *input,
					      uint32_t inputLength)
{
    TPM_RESULT 	rc = 0;
    int		irc;

    printf("   TPM_PKCS1_PaddingType1Check:\n");
    /* sanity check the length */
    if (rc == 0) {
	if ((sizeof(sha1Oid) + 11) > inputLength) {
	    printf("TPM_PKCS1_PaddingType1Check: Error, "
		   "sizeof(sha1Oid) %lu + 11 > inputLength %u\n",
		   (unsigned long)sizeof(sha1Oid), inputLength);
	    rc = TPM_ENCRYPT_ERROR;
	}
    }
    /* check byte 0 */
    if (rc == 0) {
	*padLength = 0;
	if (input[*padLength] != 0x00) {
	    printf("TPM_PKCS1_PaddingType1Check: Error, byte %u %02x not 0x00\n",
		   *padLength, input[*padLength]);
	    rc = TPM_ENCRYPT_ERROR;
	}
	(*padLength)++;
    }
    /* check byte 1 */
    if (rc == 0) {
	if (input[*padLength] != 0x01) {
	    printf("TPM_PKCS1_PaddingType1Check: Error, byte %u %02x not 0x01\n",
		   *padLength, input[*padLength]);
	    rc = TPM_ENCRYPT_ERROR;
	}
	(*padLength)++;
    }
    /* check for at least 8 0xff bytes */
    for ( ; (rc == 0) && (*padLength < 10) ; (*padLength)++) {
	if (input[*padLength] != 0xff) {
	    printf("TPM_PKCS1_PaddingType1Check: Error, byte %u %02x not 0xff\n",
		   *padLength, input[*padLength]);
	    rc = TPM_ENCRYPT_ERROR;
	}
    }
    /* check for more 0xff bytes */
    for ( ; (rc == 0) && (*padLength < inputLength) ; (*padLength)++) {
	if (input[*padLength] != 0xff) {
	    break;
	}
    }
    /* check for 0x00 byte */
    if (rc == 0) {
	if (input[*padLength] != 0x00) {
	    printf("TPM_PKCS1_PaddingType1Check: Error, byte %u %02x not 0x00\n",
		   *padLength, input[*padLength]);
	    rc = TPM_ENCRYPT_ERROR;
	}
	(*padLength)++;
    }
    /* check length for OID */
    if (rc == 0) {
	if (*padLength + sizeof(sha1Oid) > inputLength) {
	    printf("TPM_PKCS1_PaddingType1Check: Error, "
		   "padLength %u + sizeof(sha1Oid) %lu > inputLength %u\n",
		   *padLength,  (unsigned long)sizeof(sha1Oid), inputLength);
	    rc = TPM_ENCRYPT_ERROR;
	}
    }
    /* check OID */
    if (rc == 0) {
	irc = memcmp(input + *padLength, sha1Oid, sizeof(sha1Oid));
	if (irc != 0) {
	    printf("TPM_PKCS1_PaddingType1Check: Error, OID mismatch\n");
	    TPM_PrintAll("   TPM_PKCS1_PaddingType1Check: OID",
			 input + *padLength, sizeof(sha1Oid));
	    rc = TPM_ENCRYPT_ERROR;
	}
	*padLength += sizeof(sha1Oid);
    }
    return rc;
}

/* TPM_PKCS1_PaddingType2Add() adds the PKCS1 type 2 padding
   
   The output buffer is preallocated.

   See PKCS1 9.1.2.1 Encoding operation
   
   This method cheats a bit by adding a leading 00 as well, which is needed for the RSA operation.

   M	  message to be encoded, an octet string of length at most emLen-10 
   emLen	  intended length in octets of the encoded message

   Output:	
   EM	  encoded message, an octet string of length emLen; or "message too long"
*/

static TPM_RESULT TPM_PKCS1_PaddingType2Add(unsigned char *encodedMessage,    	/* to */
					    uint32_t encodedMessageLength,    	/* to length */
					    const unsigned char *message,	/* from */
					    uint32_t messageLength)		/* from length */
{
    TPM_RESULT	rc = 0;

    printf("   TPM_PKCS1_PaddingType2Add: Message length %u padded length %u\n",
	   messageLength, encodedMessageLength);
    /* 1. If the length of the message M is greater than emLen - 10 octets, output "message too
       long" and stop. */
    if (rc == 0) {
	if ((messageLength + 11) > encodedMessageLength) {
	    printf("TPM_PKCS1_PaddingType2Add: Error, message length too big for padded length\n");
	    rc = TPM_ENCRYPT_ERROR;
	}
    }
    /* 2. Generate an octet string PS of length emLen-||M||-2 consisting of pseudorandomly generated
       nonzero octets. The length of PS will be at least 8 octets. */
    if (rc == 0) {
	rc = TPM_RandomNonZero(encodedMessage + 2, encodedMessageLength - messageLength - 3);
    }
    /* 3. Concatenate PS, the message M, and other padding to form the encoded message EM as: */
    /*	   EM = 02 || PS || 00 || M */
    if (rc == 0) {
	encodedMessage[0] = 0x00;
	encodedMessage[1] = 0x02;
	encodedMessage[encodedMessageLength - messageLength - 1]  = 0x00;
	memcpy(encodedMessage + encodedMessageLength - messageLength, message, messageLength);
    }
    return rc;
}

/* TPM_PKCS1_Type2PaddingCheck checks the PKCS1 type 2 padding and recovers the message

   The output buffer is preallocated.
*/

static
TPM_RESULT TPM_PKCS1_PaddingType2Check(unsigned char *outputData,	/* to */
				       uint32_t *outputDataLength,	/* to length */
				       uint32_t outputDataSize,  /* pre-allocated to length */
				       unsigned char *inputData,	/* from - padded data */
				       uint32_t inputDataLength)	/* from length */
{
    TPM_RESULT	rc = 0;
    size_t 	i;

    printf("   TPM_PKCS1_PaddingType2Check:\n");
    /* check the leading bytes for 0x00, 0x02 */
    if (rc == 0) {
	if ((inputData[0] != 0x00) ||
	    (inputData[1] != 0x02)) {
	    printf("TPM_PKCS1_PaddingType2Check: Error, bad leading bytes %02x %02x\n",
		   inputData[0], inputData[1]);
	    rc = TPM_DECRYPT_ERROR;
	}
    }	 
    /* skip the non-zero random PS */
    for (i = 2 ; (rc == 0) && (i < inputDataLength) ; i++) {
	if (inputData[i] == 0x00) {
	    break;
	}
    }
    /* check for the trailing 0x00 */
    if (rc == 0) {
	if (i == inputDataLength) {
	    printf("TPM_PKCS1_PaddingType2Check: Error, missing trailing 0x00\n");
	    rc = TPM_DECRYPT_ERROR;
	}
    }
    /* check that PS was at least 8 bytes */
    if (rc == 0) {
	if (i < 10) {
	    printf("TPM_PKCS1_PaddingType2Check: Error, bad PS length %lu\n", (unsigned long)i-2);
	    rc = TPM_DECRYPT_ERROR;
	}
    }
    /* check that the output can accommodate the message */
    if (rc == 0) {
	i++;	/* index past the trailing 0x00 */
	*outputDataLength = inputDataLength - i;
	if (*outputDataLength > outputDataSize) {
	    printf("TPM_PKCS1_PaddingType2Check: Error, "
		   "message %u greater than output data size %u\n",
		   *outputDataLength, outputDataSize);
	    rc = TPM_DECRYPT_ERROR;
	}
    }
    /* copy the message */
    if (rc == 0) {
	memcpy(outputData, inputData + inputDataLength - *outputDataLength, *outputDataLength);
    }	 
    return rc;
}

/*
  GNU MP wrappers do error logging and transformation of errors to TPM type errors
*/

/* TPM_BN_num_bytes() wraps the gnump function in a TPM error handler

   Returns number of bytes in the input
*/

TPM_RESULT TPM_BN_num_bytes(unsigned int *numBytes, TPM_BIGNUM bn_in)
{
    TPM_RESULT  rc = 0;
    mpz_t *bn = (mpz_t *)bn_in;

    /* is the bignum zero */
    int result = mpz_cmp_ui(*bn, 0);
    /* mpz_sizeinbase() always returns at least one.  If the value is zero, there should really be 0
       bytes */ 
    if (result == 0) {
	*numBytes = 0;
    }
    /* take the base 2 number and round up to the next byte */
    else {
	*numBytes = (mpz_sizeinbase (*bn, 2) +7) / 8;
    }
    return rc;
}

/* TPM_BN_is_one() wraps the gnump function in a TPM error handler

   Returns success if input is 1
*/

TPM_RESULT TPM_BN_is_one(TPM_BIGNUM bn_in)
{
    TPM_RESULT  rc = 0;
    mpz_t *bn = (mpz_t *)bn_in;
    int         irc;

    irc = mpz_cmp_ui(*bn, 1);
    if (irc != 0) {
        printf("TPM_BN_is_one: Error, result is not 1\n");
        rc = TPM_DAA_WRONG_W;
    }
    return rc;
}


/* TPM_BN_mod() wraps the gnump function in a TPM error handler

   r = a mod m
*/

TPM_RESULT TPM_BN_mod(TPM_BIGNUM rem_in,
		      const TPM_BIGNUM a_in,
		      const TPM_BIGNUM m_in)
{
    TPM_RESULT  rc = 0;
    mpz_t *rBignum = (mpz_t *)rem_in;
    mpz_t *aBignum = (mpz_t *)a_in;
    mpz_t *mBignum = (mpz_t *)m_in;

    /* set r to a mod m */
    mpz_mod(*rBignum, *aBignum, *mBignum);
    return rc;
}

/* TPM_BN_mask_bits() wraps the gnump function in a TPM error handler

   erase all but the lowest n bits of bn
   bn  = bn mod 2^^n
*/

TPM_RESULT TPM_BN_mask_bits(TPM_BIGNUM bn_in, unsigned int n)
{
    TPM_RESULT          rc = 0;
    unsigned int        numBytes;
    mpz_t 		*bn = (mpz_t *)bn_in;

    if (rc == 0) {
        rc = TPM_BN_num_bytes(&numBytes, bn_in);
    }
    if (rc == 0) {
	/* if the BIGNUM is already fewer bits, no need to mask */
        if (numBytes > (n / 8)) {
	    /* divide and return remainder, divisor is 2^^n */
	    mpz_fdiv_r_2exp(*bn, *bn, n);
        }
    }
    return rc;
}

/* TPM_BN_rshift() wraps the gnump function in a TPM error handler

   Shift a right by n bits (discard the lowest n bits) and label the result r
*/

TPM_RESULT TPM_BN_rshift(TPM_BIGNUM *rBignum_in,              /* freed by caller */
                         TPM_BIGNUM aBignum_in,
                         int n)
{
    TPM_RESULT  rc = 0;
    mpz_t 	**rBignum = (mpz_t **)rBignum_in;
    mpz_t 	*aBignum = (mpz_t *)aBignum_in;
    
    printf(" TPM_BN_rshift: n %d\n", n);
    if (rc == 0) {
        rc = TPM_BN_new(rBignum_in);
    }
    if (rc == 0) {
	/* divide and return quotient, rounded down (floor) */
	mpz_fdiv_q_2exp(**rBignum, *aBignum, n); 
    }
    return rc;
}

/* TPM_BN_lshift() wraps the gnump function in a TPM error handler

   Shift a left by n bits and label the result r
*/

TPM_RESULT TPM_BN_lshift(TPM_BIGNUM *rBignum_in,              /* freed by caller */
                         TPM_BIGNUM aBignum_in,
                         int n)
{
    TPM_RESULT  rc = 0;
    mpz_t 	**rBignum = (mpz_t **)rBignum_in;
    mpz_t 	*aBignum = (mpz_t *)aBignum_in;
    
    printf(" TPM_BN_lshift: n %d\n", n);
    if (rc == 0) {
        rc = TPM_BN_new(rBignum_in);
    }
    if (rc == 0) {
	/* multiply by 2^^n is is a left shift by n */
	mpz_mul_2exp(**rBignum, *aBignum, n);
    }
    return rc;
}

/* TPM_BN_add() wraps the gnump function in a TPM error handler

   r = a + b
*/

TPM_RESULT TPM_BN_add(TPM_BIGNUM rBignum_in,
                      TPM_BIGNUM aBignum_in,
                      TPM_BIGNUM bBignum_in)
{
    TPM_RESULT  rc = 0;
    mpz_t 	*rBignum = (mpz_t *)rBignum_in;
    mpz_t 	*aBignum = (mpz_t *)aBignum_in;
    mpz_t 	*bBignum = (mpz_t *)bBignum_in;

    printf(" TPM_BN_add:\n");
    /* result = a + b */
    mpz_add(*rBignum, *aBignum, *bBignum);
    return rc;
}

/* TPM_BN_mul() wraps the gnump function in a TPM error handler

   r = a * b
*/

TPM_RESULT TPM_BN_mul(TPM_BIGNUM rBignum_in,
                      TPM_BIGNUM aBignum_in,
                      TPM_BIGNUM bBignum_in)
{
    TPM_RESULT  rc = 0;
    mpz_t 	*rBignum = (mpz_t *)rBignum_in;
    mpz_t 	*aBignum = (mpz_t *)aBignum_in;
    mpz_t 	*bBignum = (mpz_t *)bBignum_in;

    printf(" TPM_BN_mul:\n");
    /* r = a * b */
    mpz_mul(*rBignum, *aBignum, *bBignum);
    return rc;
}

/* TPM_BN_mod_exp() wraps the gnump function in a TPM error handler

   computes a to the p-th power modulo m (r=a^p % n)
*/

TPM_RESULT TPM_BN_mod_exp(TPM_BIGNUM rBignum_in,
                          TPM_BIGNUM aBignum_in,
                          TPM_BIGNUM pBignum_in,
                          TPM_BIGNUM nBignum_in)
{
    TPM_RESULT  rc = 0;
    mpz_t 	*rBignum = (mpz_t *)rBignum_in;
    mpz_t 	*aBignum = (mpz_t *)aBignum_in;
    mpz_t 	*pBignum = (mpz_t *)pBignum_in;
    mpz_t 	*nBignum = (mpz_t *)nBignum_in;
    
    printf(" TPM_BN_mod_exp:\n");
    mpz_powm(*rBignum, *aBignum, *pBignum, *nBignum);
    return rc;
}

/* TPM_BN_Mod_add() wraps the gnump function in a TPM error handler

   adds a to b modulo m
*/

TPM_RESULT TPM_BN_mod_add(TPM_BIGNUM rBignum_in,
                          TPM_BIGNUM aBignum_in,
                          TPM_BIGNUM bBignum_in,
                          TPM_BIGNUM mBignum_in)
{
    TPM_RESULT  rc = 0;
    mpz_t 	*rBignum = (mpz_t *)rBignum_in;
    mpz_t 	*aBignum = (mpz_t *)aBignum_in;
    mpz_t 	*bBignum = (mpz_t *)bBignum_in;
    mpz_t 	*mBignum = (mpz_t *)mBignum_in;

    printf(" TPM_BN_mod_add:\n");
    /* r = a + b */
    mpz_add(*rBignum, *aBignum, *bBignum);
    /* set r to r mod m */
    mpz_mod(*rBignum, *rBignum, *mBignum);
    return rc;
}

/* TPM_BN_mod_mul() wraps the gnump function in a TPM error handler

   r = (a * b) mod m
*/

TPM_RESULT TPM_BN_mod_mul(TPM_BIGNUM rBignum_in,
                          TPM_BIGNUM aBignum_in,
                          TPM_BIGNUM bBignum_in,
                          TPM_BIGNUM mBignum_in)
{
    TPM_RESULT  rc = 0;
    mpz_t 	*rBignum = (mpz_t *)rBignum_in;
    mpz_t 	*aBignum = (mpz_t *)aBignum_in;
    mpz_t 	*bBignum = (mpz_t *)bBignum_in;
    mpz_t 	*mBignum = (mpz_t *)mBignum_in;

    printf(" TPM_BN_mod_mul:\n");
    /* r = a * b */
    mpz_mul(*rBignum, *aBignum, *bBignum);
    /* set r to r mod m */
    mpz_mod(*rBignum, *rBignum, *mBignum);
    return rc;
}

/* TPM_BN_new() wraps the gnump function in a TPM error handler

   Allocates a new bignum
*/

TPM_RESULT TPM_BN_new(TPM_BIGNUM *bn_in) 	/* freed by caller */
{
    TPM_RESULT  rc = 0;
    mpz_t *bn;
    
    if (rc== 0) {
        rc = TPM_Malloc(bn_in, sizeof(mpz_t));	/* freed by caller */
    }
    if (rc== 0) {
	bn = (mpz_t *)*bn_in;
	mpz_init(*bn);
    }
    return rc;
}

/* TPM_BN_free() wraps the gnump function

 Frees the bignum
*/

void TPM_BN_free(TPM_BIGNUM bn_in)
{    
    mpz_t *bn = (mpz_t *)bn_in;
    if (bn != NULL) {
	mpz_clear(*bn);
	free(bn_in);
    }
    return;
}

/* TPM_bn2bin wraps the function in gnump a TPM error handler.

   Converts a bignum to char array

   'bin' must already be checked for sufficient size.
*/

TPM_RESULT TPM_bn2bin(unsigned char *bin,
		      TPM_BIGNUM bn_in)
{
    TPM_RESULT  rc = 0;
    mpz_t *bn = (mpz_t *)bn_in;

    mpz_export(bin,	/* output */
	       NULL,	/* countp */
	       1, 	/* order, MSB first */
	       1, 	/* size, char */	
	       0, 	/* endian, native (unused) */    
	       0, 	/* nails, don't discard */
	       *bn);	/* input */
    return rc;
}

/* TPM_memcpyPad allocates a buffer 'bin_out' and loads it from 'bin_in'.

   If padBytes is non-zero, 'bin_out' is padded with leading zeros if necessary, so that 'bytes'
   will equal 'padBytes'.  This is used when TPM data structures expect a fixed length while
   the crypto library truncates leading zeros.

   '*bin_out' must be freed by the caller
*/

static TPM_RESULT TPM_memcpyPad(unsigned char **bin_out,
				unsigned char *bin_in,
				uint32_t bin_in_length,
				uint32_t padBytes)
{
    TPM_RESULT  rc = 0;

    printf("   TPM_memcpyPad: padBytes %u\n", padBytes);
    if (rc == 0) {
        /* padBytes 0 says that no padding is required */
        if (padBytes == 0) {
            padBytes = bin_in_length;  /* setting equal yields no padding */
        }
	/* The required output should never be less than the supplied input.  Sanity check and
	   return a fatal error. */
        if (padBytes < bin_in_length) {
            printf("TPM_memcpyPad: Error (fatal), "
                   "padBytes %u less than %u\n", padBytes, bin_in_length);
            rc = TPM_FAIL;
        }
        if (padBytes != bin_in_length) {
            printf("   TPM_memcpyPad: padBytes %u bytes %u\n", padBytes, bin_in_length);
        }
    }
    /* allocate memory for the padded output */
    if (rc == 0) {
        rc = TPM_Malloc(bin_out, padBytes);
    }
    if (rc == 0) {
        memset(*bin_out, 0, padBytes - bin_in_length);   /* leading 0 padding */
        memcpy((*bin_out) + padBytes - bin_in_length,    /* start copy after padding */
	       bin_in, bin_in_length);
    }
    return rc;
}

/* TPM_bin2bn() wraps the gnump function in a TPM error handler

   Converts a char array to bignum

   bn must be freed by the caller.
*/

TPM_RESULT TPM_bin2bn(TPM_BIGNUM *bn_in, const unsigned char *bin, unsigned int bytes)
{
    TPM_RESULT rc = 0;

    if (rc == 0) {
	rc = TPM_BN_new(bn_in);
    }
    if (rc == 0) {
	mpz_t *bn = (mpz_t *)*bn_in;
	mpz_import(*bn,		/* output */
		   bytes,	/* count */
		   1,		/* order, MSB first */
		   1,		/* size, char */
		   0,		/* endian, native (unused) */
		   0,		/* nail, don't discard */
		   bin);	/* input */
    }
    return rc;
}

/*
  Hash Functions
*/

/* TPM_SHA1InitCmd() initializes a platform dependent TPM_SHA1Context structure.

   The structure must be freed using TPM_SHA1FinalCmd()
*/

TPM_RESULT TPM_SHA1InitCmd(void **context)
{
    TPM_RESULT  rc = 0;

    printf(" TPM_SHA1InitCmd:\n");
    if (rc == 0) {
	/* create a new freebl SHA1 context */
	*context = SHA1_NewContext();
	if (*context == NULL) {
	    printf("TPM_SHA1InitCmd:  Error allocating a new context\n");
            rc = TPM_SIZE;
	}
    }
    /* reset the SHA-1 context, preparing it for a fresh round of hashing */
    if (rc== 0) {
	SHA1_Begin(*context);
    }
    return rc;
}

/* TPM_SHA1UpdateCmd() adds 'data' of 'length' to the SHA-1 context
 */

TPM_RESULT TPM_SHA1UpdateCmd(void *context, const unsigned char *data, uint32_t length)
{
    TPM_RESULT  rc = 0;
    
    printf(" TPM_SHA1Update: length %u\n", length);
    if (context != NULL) {
        SHA1_Update(context, data, length);
    }
    else {
        printf("TPM_SHA1Update: Error, no existing SHA1 thread\n");
        rc = TPM_SHA_THREAD;
    }
    return rc;
}

/* TPM_SHA1FinalCmd() extracts the SHA-1 digest 'md' from the context
 */

TPM_RESULT TPM_SHA1FinalCmd(unsigned char *md, void *context)
{
    TPM_RESULT  	rc = 0;
    unsigned int 	digestLen;
    
    printf(" TPM_SHA1FinalCmd:\n");
    if (rc== 0) {
	if (context == NULL) {
	    printf("TPM_SHA1FinalCmd: Error, no existing SHA1 thread\n");
	    rc = TPM_SHA_THREAD;
	}
    }
    if (rc== 0) {
	SHA1_End(context, md, &digestLen, TPM_DIGEST_SIZE);
	/* Sanity check.  For SHA1 it should always be 20 bytes. */
	if (digestLen != TPM_DIGEST_SIZE) {
	    printf("TPM_SHA1Final: Error (fatal), SHA1_End returned %u bytes\n", digestLen);
	    rc = TPM_FAIL;
	}
    }
    return rc;
}

/* TPM_SHA1Delete() zeros and frees the SHA1 context */

void TPM_SHA1Delete(void **context)
{
    if (*context != NULL) {
        printf(" TPM_SHA1Delete:\n");
	/* zero because the SHA1 context might have data left from an HMAC */
	SHA1_DestroyContext(*context, PR_TRUE);	
        *context = NULL;
    }
    return;
}

#if defined (__x86_64__) || \
    defined(__amd64__) || \
    defined(__ia64__) || \
    defined(__powerpc64__) || \
    defined(__s390x__) || \
    (defined(__sparc__) && defined(__arch64__)) || \
    defined(__aarch64__)

#define IS_64
typedef PRUint64 SHA_HW_t;

#elif defined (__i386__) || \
    defined (__powerpc__) || \
    defined (__s390__) || \
    defined(__sparc__) || \
    defined(__arm__)

typedef PRUint32 SHA_HW_t;
#undef IS_64

#else
#error "Cannot determine 32 or 64 bit platform"
#endif

/* The structure returned by the SHA1_Flatten() command and passed to SHA1_Resurrect()
 */

typedef struct SHA1SaveContextStrtd {
    union {
	PRUint32 w[16];             /* input buffer */
	PRUint8  b[64];
    } u;
    PRUint64 size;                /* count of hashed bytes. */
    SHA_HW_t H[22];               /* 5 state variables, 16 tmp values, 1
				     extra */
} SHA1SaveContextStr;


/* TPM_Sha1Context_Load() is non-portable code to deserialize the FreeBL SHA1 context.

   If the contextPresent prepended by TPM_Sha1Context_Store() is FALSE, context remains NULL.  If
   TRUE, context is allocated and loaded.
*/

TPM_RESULT TPM_Sha1Context_Load(void **context,
				unsigned char **stream,
				uint32_t *stream_size)
{
    TPM_RESULT 		rc = 0;
    TPM_BOOL		contextPresent;		/* is there a context to be loaded */
    uint32_t	 	flattenSize;		/* from the freebl library */
    SHA1Context 	*tmpContext = NULL;	/* temp to get flatten size, freed @1 */
    uint32_t		tmp32;			/* temp to recreate 64-bit size */
    SHA1SaveContextStr 	restoreContext;
    size_t		i;
    
    printf(" TPM_Sha1Context_Load: FreeBL\n");
    /* TPM_Sha1Context_Store() stored a flag to indicate whether a context was stored */
    if (rc== 0) {
	rc = TPM_LoadBool(&contextPresent, stream, stream_size);
 	printf(" TPM_Sha1Context_Load: contextPresent %u\n", contextPresent);
    }
    /* check format tag */
    /* In the future, if multiple formats are supported, this check will be replaced by a 'switch'
       on the tag */
    if ((rc== 0) && contextPresent) {
	rc = TPM_CheckTag(TPM_TAG_SHA1CONTEXT_FREEBL_V1, stream, stream_size);
    }
    /* check that context is NULL to detect memory leak */
    if ((rc== 0) && contextPresent) {
	if (*context != NULL) {
            printf("TPM_Sha1Context_Load: Error (fatal), *context %p should be NULL\n", *context );
            rc = TPM_FAIL;
	}
    }
    /* create a temporary context just to get the freebl library size */
    if ((rc== 0) && contextPresent) {
	rc = TPM_SHA1InitCmd((void **)&tmpContext);	/* freed @1 */
    }
    /* get the size of the FreeBL library SHA1 context */
    if ((rc== 0) && contextPresent) {
	flattenSize = SHA1_FlattenSize(tmpContext);
	/* sanity check that the freebl library and TPM structure here are in sync */
	if (flattenSize != sizeof(SHA1SaveContextStr)) {
	    printf("TPM_Sha1Context_Load: Error, "
		   "SHA1 context size %u from SHA1_FlattenSize not equal %lu from structure\n",
		   flattenSize, (unsigned long)sizeof(SHA1SaveContextStr));
	    rc = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      deserialization code to fill in restoreContext
    */
    /* b[0..63] <- u.b[0..63]  (bytes only, no bytswapping) */
    if ((rc== 0) && contextPresent) {
	rc = TPM_Loadn(restoreContext.u.b, 64, stream, stream_size);
    }
    /* count <- size (this is 64 bits on all platforms) */
    if ((rc== 0) && contextPresent) {
	rc = TPM_Load32(&tmp32, stream, stream_size);
	restoreContext.size = (uint64_t)tmp32 << 32;		/* big endian */
    }
    if ((rc== 0) && contextPresent) {
	rc = TPM_Load32(&tmp32, stream, stream_size);
	restoreContext.size += (uint64_t)tmp32 & 0xffffffff;	/* big endian */
    }
    for (i = 0 ; (rc == 0) && contextPresent && (i < 5) ; i++) {
	rc = TPM_Load32(&tmp32, stream, stream_size);
	restoreContext.H[i] = tmp32;	/* H can be 32 or 64 bits */
    }
    /* load the context */
    if ((rc== 0) && contextPresent) {
	/* the size test above ensures that the cast here is safe */
	*context = SHA1_Resurrect((unsigned char *)&restoreContext, NULL);
	if (*context == NULL) {
	    printf("TPM_Sha1Context_Load: Error, could not SHA1_Resurrect\n");
	    rc = TPM_SIZE;
	}
    }
    TPM_SHA1Delete((void *)&tmpContext); 	/* @1 */
    return rc;
}

/* TPM_Sha1Context_Store() is non-portable code to serialize the FreeBL SHA1 context.  context is
   not altered.

   It prepends a contextPresent flag to the stream, FALSE if context is NULL, TRUE if not.
*/

TPM_RESULT TPM_Sha1Context_Store(TPM_STORE_BUFFER *sbuffer,
				 void *context)
{
    TPM_RESULT 		rc = 0;
    SECStatus 		rv = SECSuccess;
    size_t		i;
    unsigned int 	flattenSize;
    SHA1SaveContextStr	saveContext;
    TPM_BOOL		contextPresent;		/* is there a context to be stored */

    printf(" TPM_Sha1Context_Store: FreeBL\n");
    /* store contextPresent */
    if (rc == 0) {
	if (context != NULL) {
	    printf("  TPM_Sha1Context_Store: Storing context\n");
	    contextPresent = TRUE;
	}
	else {
	    printf("  TPM_Sha1Context_Store: No context to store\n");
	    contextPresent = FALSE;
	}
	printf("  TPM_Sha1Context_Store: contextPresent %u \n", contextPresent);
        rc = TPM_Sbuffer_Append(sbuffer, &contextPresent, sizeof(TPM_BOOL));
    }
    /* overall format tag */
    if ((rc== 0) && contextPresent) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_SHA1CONTEXT_FREEBL_V1);
    }
    if ((rc== 0) && contextPresent) {
	/* get the size of the FreeBL SHA1 context */
	flattenSize = SHA1_FlattenSize(context);	/* it will not be NULL here */
	/* sanity check that the freebl library and TPM structure here are in sync */
	if (flattenSize != sizeof(SHA1SaveContextStr)) {
	    printf("TPM_Sha1Context_Store: Error (fatal), "
		   "SHA1 context size %u from SHA1_FlattenSize not equal %lu from structure\n",
		   flattenSize, (unsigned long)sizeof(SHA1SaveContextStr));
	    rc = TPM_FAIL;
	}
    }
    /* store into the structure from the library */
    if ((rc== 0) && contextPresent) {
	/* the size test above ensures that the cast here is safe */
	rv = SHA1_Flatten(context, (unsigned char *)&saveContext);
	if (rv != SECSuccess) {
	    printf("TPM_Sha1Context_Store: Error (fatal), SHA1_Flatten rv %d\n", rv);
	    rc = TPM_FAIL;
	}
    }
    /*
      append the FreeBL SHA1 context to the stream
    */
    /* b[0..63] <- u.b[0..63]  (bytes only, no byte swapping) */
    if ((rc== 0) && contextPresent) {
	rc = TPM_Sbuffer_Append(sbuffer, saveContext.u.b, 64);
    }
    /* count <- size (this is 64 bits on all platforms) */
    if ((rc== 0) && contextPresent) {
	rc = TPM_Sbuffer_Append32(sbuffer, saveContext.size >> 32);	/* big endian */
    }
    if ((rc== 0) && contextPresent) {
	rc = TPM_Sbuffer_Append32(sbuffer, saveContext.size & 0xffffffff);
    }
    /* SHA_HW_t - NSS uses 64 bits on 64 bit platforms for performance reasons only.  The lower 32
       bits are critical, so you can always serialize/deserialize just the lower 32 bits. */
    /* The remainder of the H array is scratch memory and does not need to be preserved or
       transmitted. */
    for (i = 0 ; (rc == 0) && contextPresent && (i < 5) ; i++) {
	rc = TPM_Sbuffer_Append32(sbuffer, saveContext.H[i] & 0xffffffff);
    }
    return rc;
}

/*
  TPM_SYMMETRIC_KEY_DATA
*/

#ifdef TPM_AES

/* TPM_SymmetricKeyData_New() allocates memory for and initializes a TPM_SYMMETRIC_KEY_DATA token.
 */

TPM_RESULT TPM_SymmetricKeyData_New(TPM_SYMMETRIC_KEY_TOKEN *tpm_symmetric_key_data)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_SymmetricKeyData_New:\n");
    if (rc == 0) {
	rc = TPM_Malloc(tpm_symmetric_key_data, sizeof(TPM_SYMMETRIC_KEY_DATA));
    }
    if (rc == 0) {
	TPM_SymmetricKeyData_Init(*tpm_symmetric_key_data);
    }
    return rc;
}

/* TPM_SymmetricKeyData_Free() initializes the key token to wipe secrets.  It then frees the
   TPM_SYMMETRIC_KEY_DATA token and sets it to NULL.
*/

void TPM_SymmetricKeyData_Free(TPM_SYMMETRIC_KEY_TOKEN *tpm_symmetric_key_data)
{
    printf(" TPM_SymmetricKeyData_Free:\n");
    if (*tpm_symmetric_key_data != NULL) {
        TPM_SymmetricKeyData_Init(*tpm_symmetric_key_data);
	free(*tpm_symmetric_key_data);
	*tpm_symmetric_key_data = NULL;
    }
    return;
}

/* TPM_SymmetricKeyData_Init() is AES non-portable code to initialize the TPM_SYMMETRIC_KEY_DATA

   It depends on the TPM_SYMMETRIC_KEY_DATA declaration.
*/

void TPM_SymmetricKeyData_Init(TPM_SYMMETRIC_KEY_TOKEN tpm_symmetric_key_token)
{
    TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data =
	(TPM_SYMMETRIC_KEY_DATA *)tpm_symmetric_key_token;

    printf(" TPM_SymmetricKeyData_Init:\n");
    tpm_symmetric_key_data->tag = TPM_TAG_KEY;
    tpm_symmetric_key_data->valid = FALSE;
    tpm_symmetric_key_data->fill = 0;
    /* zero to wipe secrets */
    memset(tpm_symmetric_key_data->userKey, 0, sizeof(tpm_symmetric_key_data->userKey));
    return;
}

/* TPM_SymmetricKeyData_Load() is AES non-portable code to deserialize the TPM_SYMMETRIC_KEY_DATA

   It depends on the above TPM_SYMMETRIC_KEY_DATA declaration.
*/

TPM_RESULT TPM_SymmetricKeyData_Load(TPM_SYMMETRIC_KEY_TOKEN tpm_symmetric_key_token,
                                     unsigned char **stream,
                                     uint32_t *stream_size)
{
    TPM_RESULT rc = 0;
    TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data =
	(TPM_SYMMETRIC_KEY_DATA *)tpm_symmetric_key_token;
    
    printf(" TPM_SymmetricKeyData_Load:\n");
    /* check tag */
    if (rc == 0) {
        rc = TPM_CheckTag(TPM_TAG_KEY, stream, stream_size);
    }
    /* load valid */
    if (rc == 0) {
        rc = TPM_LoadBool(&(tpm_symmetric_key_data->valid), stream, stream_size);
    }
    /* load fill */
    if (rc == 0) {
        rc = TPM_Load8(&(tpm_symmetric_key_data->fill), stream, stream_size);
    }
    /* The AES key is a simple array. */
    if (rc == 0) {
        rc = TPM_Loadn(tpm_symmetric_key_data->userKey, sizeof(tpm_symmetric_key_data->userKey),
                       stream, stream_size);
    }
    return rc;
}

/* TPM_SymmetricKeyData_Store() is AES non-portable code to serialize the TPM_SYMMETRIC_KEY_DATA

   It depends on the above TPM_SYMMETRIC_KEY_DATA declaration.
*/

TPM_RESULT TPM_SymmetricKeyData_Store(TPM_STORE_BUFFER *sbuffer,
                                      const TPM_SYMMETRIC_KEY_TOKEN tpm_symmetric_key_token)
{
    TPM_RESULT rc = 0;
    TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data =
	(TPM_SYMMETRIC_KEY_DATA *)tpm_symmetric_key_token;
    
    printf(" TPM_SymmetricKeyData_Store:\n");
    /* store tag */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append16(sbuffer, tpm_symmetric_key_data->tag);
    }
    /* store valid */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, &(tpm_symmetric_key_data->valid), sizeof(TPM_BOOL));
    }
    /* store fill */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, &(tpm_symmetric_key_data->fill), sizeof(TPM_BOOL));
    }
    /* store AES key */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer,
                                tpm_symmetric_key_data->userKey,
                                sizeof(tpm_symmetric_key_data->userKey));
    }
    return rc;
}

/* TPM_SymmetricKeyData_GenerateKey() is AES non-portable code to generate a random symmetric key

   tpm_symmetric_key_data should be initialized before and after use
*/

TPM_RESULT TPM_SymmetricKeyData_GenerateKey(TPM_SYMMETRIC_KEY_TOKEN tpm_symmetric_key_token)
{
    TPM_RESULT rc = 0;
    TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data =
	(TPM_SYMMETRIC_KEY_DATA *)tpm_symmetric_key_token;
    
    printf(" TPM_SymmetricKeyData_GenerateKey:\n");
    /* generate a random key */
    if (rc == 0) {
        rc = TPM_Random(tpm_symmetric_key_data->userKey, sizeof(tpm_symmetric_key_data->userKey));
    }
    if (rc == 0) {
        tpm_symmetric_key_data->valid = TRUE;
    }
    return rc;
}

/* TPM_SymmetricKeyData_Encrypt() is AES non-portable code to CBC encrypt 'decrypt_data' to
   'encrypt_data'

   The stream is padded as per PKCS#7 / RFC2630

   'encrypt_data' must be free by the caller
*/

TPM_RESULT TPM_SymmetricKeyData_Encrypt(unsigned char **encrypt_data,   /* output, caller frees */
                                        uint32_t *encrypt_length,		/* output */
                                        const unsigned char *decrypt_data,	/* input */
                                        uint32_t decrypt_length,		/* input */
                                        const TPM_SYMMETRIC_KEY_TOKEN
					tpm_symmetric_key_token) 		/* input */
{
    TPM_RESULT          rc = 0;
    SECStatus 		rv;
    AESContext 		*cx;
    uint32_t		pad_length;
    uint32_t		output_length;			/* dummy */
    unsigned char       *decrypt_data_pad;
    unsigned char       ivec[TPM_AES_BLOCK_SIZE];       /* initial chaining vector */
    TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data =
	(TPM_SYMMETRIC_KEY_DATA *)tpm_symmetric_key_token;

    printf(" TPM_SymmetricKeyData_Encrypt: Length %u\n", decrypt_length);
    decrypt_data_pad = NULL;    /* freed @1 */
    cx = NULL;    		/* freed @2 */
    
    /* sanity check that the AES key has previously been generated */
    if (rc == 0) {
	if (!tpm_symmetric_key_data->valid) {
	    printf("TPM_SymmetricKeyData_Encrypt: Error (fatal), AES key not valid\n");
	    rc = TPM_FAIL;
	}
    }
    if (rc == 0) {
        /* calculate the PKCS#7 / RFC2630 pad length and padded data length */
        pad_length = TPM_AES_BLOCK_SIZE - (decrypt_length % TPM_AES_BLOCK_SIZE);
        *encrypt_length = decrypt_length + pad_length;
        printf("  TPM_SymmetricKeyData_Encrypt: Padded length %u pad length %u\n",
               *encrypt_length, pad_length);
        /* allocate memory for the encrypted response */
        rc = TPM_Malloc(encrypt_data, *encrypt_length);
    }
    /* allocate memory for the padded decrypted data */
    if (rc == 0) {
        rc = TPM_Malloc(&decrypt_data_pad, *encrypt_length);
    }
    if (rc == 0) {
        /* set the IV */
        memset(ivec, 0, sizeof(ivec));
	/* create a new AES context */
	cx = AES_CreateContext(tpm_symmetric_key_data->userKey,
			       ivec, 			/* CBC initialization vector */
			       NSS_AES_CBC,		/* CBC mode */
			       TRUE,			/* encrypt */
			       TPM_AES_BLOCK_SIZE,	/* key length */
			       TPM_AES_BLOCK_SIZE);	/* AES  block length */
	if (cx == NULL) {
	    printf("TPM_SymmetricKeyData_Encrypt: Error creating AES context\n");
	    rc = TPM_SIZE;
	}
    }
    /* pad the decrypted clear text data */
    if (rc == 0) {
        /* unpadded original data */
        memcpy(decrypt_data_pad, decrypt_data, decrypt_length);
        /* last gets pad = pad length */
        memset(decrypt_data_pad + decrypt_length, pad_length, pad_length);
        /* encrypt the padded input to the output */
        TPM_PrintFour("  TPM_SymmetricKeyData_Encrypt: Input", decrypt_data_pad);
	/* perform the AES encryption */
	rv = AES_Encrypt(cx,
			 *encrypt_data, &output_length, *encrypt_length,	/* output */
			 decrypt_data_pad, *encrypt_length);			/* input */

	if (rv != SECSuccess) {
	    printf("TPM_SymmetricKeyData_Encrypt: Error, rv %d\n", rv);
	    rc = TPM_ENCRYPT_ERROR;
	}
    }
    if (rc == 0) {
       TPM_PrintFour("  TPM_SymmetricKeyData_Encrypt: Output", *encrypt_data);
    }	
    free(decrypt_data_pad);     	/* @1 */
    if (cx != NULL) {
	/* due to a FreeBL bug, must zero the context before destroying it */
	unsigned char dummy_key[TPM_AES_BLOCK_SIZE];
	unsigned char dummy_ivec[TPM_AES_BLOCK_SIZE];
	memset(dummy_key, 0x00, TPM_AES_BLOCK_SIZE);
	memset(dummy_ivec, 0x00, TPM_AES_BLOCK_SIZE);
	rv = AES_InitContext(cx,			/* AES context */
			     dummy_key,			/* AES key */
			     TPM_AES_BLOCK_SIZE,	/* key length */
			     dummy_ivec, 		/* ivec */
			     NSS_AES_CBC,		/* CBC mode */
			     TRUE,			/* encrypt */
			     TPM_AES_BLOCK_SIZE);	/* AES  block length */
	AES_DestroyContext(cx, PR_TRUE);	/* @2 */
    }
    return rc;
}

/* TPM_SymmetricKeyData_Decrypt() is AES non-portable code to CBC decrypt 'encrypt_data' to
   'decrypt_data'

   The stream must be padded as per PKCS#7 / RFC2630

   decrypt_data must be free by the caller
*/

TPM_RESULT TPM_SymmetricKeyData_Decrypt(unsigned char **decrypt_data,   /* output, caller frees */
                                        uint32_t *decrypt_length,		/* output */
                                        const unsigned char *encrypt_data,	/* input */
                                        uint32_t encrypt_length,		/* input */
                                        const TPM_SYMMETRIC_KEY_TOKEN
					tpm_symmetric_key_token) 		/* input */
{
    TPM_RESULT          rc = 0;
    SECStatus 		rv;
    AESContext 		*cx;
    uint32_t		pad_length;
    uint32_t		output_length;			/* dummy */
    uint32_t		i;
    unsigned char       *pad_data;
    unsigned char       ivec[TPM_AES_BLOCK_SIZE];       /* initial chaining vector */
    TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data =
	(TPM_SYMMETRIC_KEY_DATA *)tpm_symmetric_key_token;
    
    printf(" TPM_SymmetricKeyData_Decrypt: Length %u\n", encrypt_length);
    cx = NULL;    /* freed @1 */

    /* sanity check encrypted length */
    if (rc == 0) {
        if (encrypt_length < TPM_AES_BLOCK_SIZE) {
            printf("TPM_SymmetricKeyData_Decrypt: Error, bad length\n");
            rc = TPM_DECRYPT_ERROR;
        }
    }
    /* sanity check that the AES key has previously been generated */
    if (rc == 0) {
	if (!tpm_symmetric_key_data->valid) {
	    printf("TPM_SymmetricKeyData_Decrypt: Error (fatal), AES key not valid\n");
	    rc = TPM_FAIL;
	}
    }
    /* allocate memory for the PKCS#7 / RFC2630 padded decrypted data */
    if (rc == 0) {
        rc = TPM_Malloc(decrypt_data, encrypt_length);
    }
    if (rc == 0) {
        /* set the IV */
        memset(ivec, 0, sizeof(ivec));
	/* create a new AES context */
	cx = AES_CreateContext(tpm_symmetric_key_data->userKey,
			       ivec, 			/* CBC initialization vector */ 
			       NSS_AES_CBC,		/* CBC mode */
			       FALSE,			/* decrypt */
			       TPM_AES_BLOCK_SIZE,	/* key length */
			       TPM_AES_BLOCK_SIZE);	/* AES  block length */
	if (cx == NULL) {
	    printf("TPM_SymmetricKeyData_Decrypt: Error creating AES context\n");
	    rc = TPM_SIZE;
	}
    }
    /* decrypt the input to the PKCS#7 / RFC2630 padded output */
    if (rc == 0) {
        TPM_PrintFour("  TPM_SymmetricKeyData_Decrypt: Input", encrypt_data);
	/* perform the AES decryption */
	rv = AES_Decrypt(cx,
			 *decrypt_data, &output_length, encrypt_length,	/* output */
			 encrypt_data, encrypt_length);			/* input */
	if (rv != SECSuccess) {
	    printf("TPM_SymmetricKeyData_Decrypt: Error, rv %d\n", rv);
	    rc = TPM_DECRYPT_ERROR;
	}
    }
    if (rc == 0) {
        TPM_PrintFour("  TPM_SymmetricKeyData_Decrypt: Output", *decrypt_data);
    }
    /* get the pad length */
    if (rc == 0) {
        /* get the pad length from the last byte */
        pad_length = (uint32_t)*(*decrypt_data + encrypt_length - 1);
        /* sanity check the pad length */
        printf(" TPM_SymmetricKeyData_Decrypt: Pad length %u\n", pad_length);
        if ((pad_length == 0) ||
            (pad_length > TPM_AES_BLOCK_SIZE)) {
            printf("TPM_SymmetricKeyData_Decrypt: Error, illegal pad length\n");
            rc = TPM_DECRYPT_ERROR;
        }
    }
    if (rc == 0) {
        /* get the unpadded length */
        *decrypt_length = encrypt_length - pad_length;
        /* pad starting point */
        pad_data = *decrypt_data + *decrypt_length;
        /* sanity check the pad */
        for (i = 0 ; i < pad_length ; i++, pad_data++) {
            if (*pad_data != pad_length) {
                printf("TPM_SymmetricKeyData_Decrypt: Error, bad pad %02x at index %u\n",
                       *pad_data, i);
                rc = TPM_DECRYPT_ERROR;
            }
        }
    }
    if (cx != NULL) {
	/* due to a FreeBL bug, must zero the context before destroying it */
	unsigned char dummy_key[TPM_AES_BLOCK_SIZE];
	unsigned char dummy_ivec[TPM_AES_BLOCK_SIZE];
	memset(dummy_key, 0x00, TPM_AES_BLOCK_SIZE);
	memset(dummy_ivec, 0x00, TPM_AES_BLOCK_SIZE);
	rv = AES_InitContext(cx,			/* AES context */
			     dummy_key,			/* AES key */
			     TPM_AES_BLOCK_SIZE,	/* key length */
			     dummy_ivec, 		/* ivec */
			     NSS_AES_CBC,		/* CBC mode */
			     TRUE,			/* encrypt */
			     TPM_AES_BLOCK_SIZE);	/* AES  block length */
	AES_DestroyContext(cx, PR_TRUE);	/* @1 */
    }
    return rc;
}

/* TPM_SymmetricKeyData_CtrCrypt() does an encrypt or decrypt (they are the same XOR operation with
   a CTR mode pad) of 'data_in' to 'data_out'.

   TPM_SymmetricKeyData_CtrCrypt() is a TPM variant of the standard CTR encrypt function that
   increments only the low 4 bytes of the counter.

   NOTE: This function looks general, but is currently hard coded to AES128.

   'symmetric key' is the raw key, not converted to a non-portable form
   'ctr_in' is the initial CTR value before possible truncation
*/

TPM_RESULT TPM_SymmetricKeyData_CtrCrypt(unsigned char *data_out,               /* output */
                                         const unsigned char *data_in,          /* input */
                                         uint32_t data_size,			/* input */
                                         const unsigned char *symmetric_key,    /* input */
                                         uint32_t symmetric_key_size,		/* input */
					 const unsigned char *ctr_in,		/* input */
                                         uint32_t ctr_in_size)			/* input */
{
    TPM_RESULT  	rc = 0;
    SECStatus 		rv;
    AESContext 		*cx = NULL;
    unsigned char 	ctr[TPM_AES_BLOCK_SIZE];
    unsigned char 	pad_buffer[TPM_AES_BLOCK_SIZE];	/* the XOR pad */
    uint32_t		output_length;			/* dummy */
    uint32_t 		cint;				/* counter as a 32-bit integer */

    printf(" TPM_SymmetricKeyData_CtrCrypt: data_size %u\n", data_size);
    symmetric_key_size = symmetric_key_size;
    /* check the input CTR size, it can be truncated, but cannot be smaller than the AES key */
    if (rc == 0) {
        if (ctr_in_size < sizeof(ctr)) {
            printf("  TPM_SymmetricKeyData_CtrCrypt: Error (fatal)"
                   ", CTR size %u too small for AES key\n", ctr_in_size);
            rc = TPM_FAIL;              /* should never occur */
        }
    }
    if (rc == 0) {
        /* make a truncated copy of CTR, since this function alters the value */
        memcpy(ctr, ctr_in, sizeof(ctr));
        TPM_PrintFour("  TPM_SymmetricKeyData_CtrCrypt: CTR", ctr);
    }
    /* create a new AES context */
    if (rc == 0) {
	cx = AES_CreateContext(symmetric_key,		/* AES key */
			       NULL, 			/* ivec not used in NSS_AES */
			       NSS_AES,			/* mode */
			       TRUE,			/* encrypt */
			       TPM_AES_BLOCK_SIZE,	/* key length */
			       TPM_AES_BLOCK_SIZE);	/* AES  block length */
	if (cx == NULL) {
	    printf("TPM_SymmetricKeyData_CtrCrypt: Error creating AES context\n");
	    rc = TPM_SIZE;
	}
    }
    while (data_size != 0) {
        printf("  TPM_SymmetricKeyData_CtrCrypt : data_size remaining %u\n", data_size);
	/* initialize the context each time through the loop */
	if (rc == 0) {
	    rv = AES_InitContext(cx,			/* AES context */
				 symmetric_key,		/* AES key */
				 TPM_AES_BLOCK_SIZE,	/* key length */
				 NULL, 			/* ivec not used in NSS_AES */
				 NSS_AES,		/* mode */
				 TRUE,			/* encrypt */
				 TPM_AES_BLOCK_SIZE);	/* AES  block length */
	    if (rv != SECSuccess) {
		printf("TPM_SymmetricKeyData_CtrCrypt: Error, rv %d\n", rv);
		rc = TPM_ENCRYPT_ERROR;
	    }
	}
	/* get an XOR pad array by encrypting the CTR with the AES key */
	if (rc == 0) {
	    rv = AES_Encrypt(cx,
			     pad_buffer, &output_length, TPM_AES_BLOCK_SIZE,	/* output */
			     ctr, TPM_AES_BLOCK_SIZE);				/* input */

	    if (rv != SECSuccess) {
		printf("TPM_SymmetricKeyData_CtrCrypt: Error, rv %d\n", rv);
		rc = TPM_ENCRYPT_ERROR;
	    }
	}
	if (rc == 0) {
	    /* partial or full last data block */
	    if (data_size <= TPM_AES_BLOCK_SIZE) {
		TPM_XOR(data_out, data_in, pad_buffer, data_size);
		data_size = 0;
	    }
	    /* full block, not the last block */
	    else {
		TPM_XOR(data_out, data_in, pad_buffer, TPM_AES_BLOCK_SIZE);
		data_in += TPM_AES_BLOCK_SIZE;
		data_out += TPM_AES_BLOCK_SIZE;
		data_size -= TPM_AES_BLOCK_SIZE;
	    }
	    /* if not the last block, increment CTR, only the low 4 bytes */
	    if (data_size != 0) {
		/* CTR is a big endian array, so the low 4 bytes are used */
		cint = LOAD32(ctr, TPM_AES_BLOCK_SIZE-4);     /* byte array to uint32_t */
		cint++;                     /* increment */
		STORE32(ctr, TPM_AES_BLOCK_SIZE-4, cint);     /* uint32_t to byte array */
	    }
	}
    }
    if (cx != NULL) {
	/* due to a FreeBL bug, must zero the context before destroying it */
	unsigned char dummy_key[TPM_AES_BLOCK_SIZE];
	memset(dummy_key, 0x00, TPM_AES_BLOCK_SIZE);
	rv = AES_InitContext(cx,			/* AES context */
			     dummy_key,			/* AES key */
			     TPM_AES_BLOCK_SIZE,	/* key length */
			     NULL, 			/* ivec not used in NSS_AES */
			     NSS_AES,			/* mode */
			     TRUE,			/* encrypt */
			     TPM_AES_BLOCK_SIZE);	/* AES  block length */
	AES_DestroyContext(cx, PR_TRUE);	/* @2 */
    }
    return rc;
}

/* TPM_SymmetricKeyData_OfbCrypt() does an encrypt or decrypt (they are the same XOR operation with
   a OFB mode pad) of 'data_in' to 'data_out'

   NOTE: This function looks general, but is currently hard coded to AES128.

   'symmetric key' is the raw key, not converted to a non-portable form
   'ivec_in' is the initial IV value before possible truncation
*/

TPM_RESULT TPM_SymmetricKeyData_OfbCrypt(unsigned char *data_out,       	/* output */
                                         const unsigned char *data_in,  	/* input */
                                         uint32_t data_size,			/* input */
                                         const unsigned char *symmetric_key,    /* in */
                                         uint32_t symmetric_key_size,		/* in */
                                         unsigned char *ivec_in,        	/* input */
                                         uint32_t ivec_in_size)			/* input */
{
    TPM_RESULT  	rc = 0;
    SECStatus 		rv;
    AESContext 		*cx = NULL;
    unsigned char       ivec_loop[TPM_AES_BLOCK_SIZE];       	/* ivec input to loop */
    unsigned char 	pad_buffer[TPM_AES_BLOCK_SIZE];       	/* the XOR pad */
    uint32_t		output_length;				/* dummy */

    printf(" TPM_SymmetricKeyData_OfbCrypt: data_size %u\n", data_size);
    symmetric_key_size = symmetric_key_size;
    /* check the input OFB size, it can be truncated, but cannot be smaller than the AES key */
    if (rc == 0) {
        if (ivec_in_size < TPM_AES_BLOCK_SIZE) {
            printf("  TPM_SymmetricKeyData_OfbCrypt: Error (fatal),"
                   "IV size %u too small for AES key\n", ivec_in_size);
            rc = TPM_FAIL;              /* should never occur */
        }
    }
    /* first time through, the ivec_loop will be the input ivec */
    if (rc == 0) {
	memcpy(ivec_loop, ivec_in, sizeof(ivec_loop));
        TPM_PrintFour("  TPM_SymmetricKeyData_OfbCrypt: IV", ivec_loop);
    }
    /* create a new AES context */
    if (rc == 0) {
	cx = AES_CreateContext(symmetric_key,
			       NULL, 			/* ivec not used in NSS_AES */
			       NSS_AES,			/* mode */
			       TRUE,			/* encrypt */
			       TPM_AES_BLOCK_SIZE,	/* key length */
			       TPM_AES_BLOCK_SIZE);	/* AES  block length */
	if (cx == NULL) {
	    printf("TPM_SymmetricKeyData_OfbCrypt: Error creating AES context\n");
	    rc = TPM_SIZE;
	}
    }
    while (data_size != 0) {
        printf("   TPM_SymmetricKeyData_OfbCrypt: data_size remaining %u\n", data_size);
	/* initialize the context each time through the loop */
	if (rc == 0) {
	    rv = AES_InitContext(cx,			/* AES context */
				 symmetric_key,		/* AES key */
				 TPM_AES_BLOCK_SIZE,	/* key length */
				 NULL, 			/* ivec not used in NSS_AES */
				 NSS_AES,		/* mode */
				 TRUE,			/* encrypt */
				 TPM_AES_BLOCK_SIZE);	/* AES  block length */
	    if (rv != SECSuccess) {
		printf("TPM_SymmetricKeyData_OfbCrypt: Error, rv %d\n", rv);
		rc = TPM_ENCRYPT_ERROR;
	    }
	}
	/* get an XOR pad array by encrypting the IV with the AES key */
	if (rc == 0) {
	    TPM_PrintFour("  TPM_SymmetricKeyData_OfbCrypt: IV", ivec_loop);
	    rv = AES_Encrypt(cx,
			     pad_buffer, &output_length, TPM_AES_BLOCK_SIZE,	/* output */
			     ivec_loop, TPM_AES_BLOCK_SIZE);			/* input */

	    if (rv != SECSuccess) {
		printf("TPM_SymmetricKeyData_OfbCrypt: Error, rv %d\n", rv);
		rc = TPM_ENCRYPT_ERROR;
	    }
	}
	if (rc == 0) {
	    /* partial or full last data block */
	    if (data_size <= TPM_AES_BLOCK_SIZE) {
		TPM_XOR(data_out, data_in, pad_buffer, data_size);
		data_size = 0;
	    }
	    /* full block, not the last block */
	    else {
		TPM_XOR(data_out, data_in, pad_buffer, TPM_AES_BLOCK_SIZE);
		data_in += TPM_AES_BLOCK_SIZE;
		data_out += TPM_AES_BLOCK_SIZE;
		data_size -= TPM_AES_BLOCK_SIZE;
	    }
	    /* if not the last block, wrap the pad_buffer back to ivec_loop (output feed back)  */
	    memcpy(ivec_loop, pad_buffer, TPM_AES_BLOCK_SIZE);
	}
    }
    if (cx != NULL) {
	/* due to a FreeBL bug, must zero the context before destroying it */
	unsigned char dummy_key[TPM_AES_BLOCK_SIZE];
	memset(dummy_key, 0x00, TPM_AES_BLOCK_SIZE);
	rv = AES_InitContext(cx,			/* AES context */
			     dummy_key,			/* AES key */
			     TPM_AES_BLOCK_SIZE,	/* key length */
			     NULL, 			/* ivec not used in NSS_AES */
			     NSS_AES,			/* mode */
			     TRUE,			/* encrypt */
			     TPM_AES_BLOCK_SIZE);	/* AES  block length */
	AES_DestroyContext(cx, PR_TRUE);	/* @2 */
    }
    return rc;
}

#endif  /* TPM_AES */
