/********************************************************************************/
/*										*/
/*				OpenSSL helper functions			*/
/*			     Written by Stefan Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*										*/
/* (c) Copyright IBM Corporation 2020.					*/
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

#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>

#include "tpm_debug.h"
#include "tpm_error.h"
#include "tpm_openssl_helpers.h"

#include "tpm_crypto.h"

#if USE_OPENSSL_FUNCTIONS_RSA

TPM_RESULT TPM_RSAGenerateEVP_PKEY(EVP_PKEY **pkey,              /* out: pkey */
				   unsigned char *narr,      /* public modulus */
				   uint32_t nbytes,
				   unsigned char *earr,      /* public exponent */
				   uint32_t ebytes,
				   unsigned char *darr,	/* private exponent */
				   uint32_t dbytes)
{
    TPM_RESULT  rc = 0;
    int         irc;
    BIGNUM *    n = NULL;
    BIGNUM *    e = NULL;
    BIGNUM *    d = NULL;
    RSA *       rsakey = NULL;

    /* sanity check for the free */
    if (rc == 0) {
	if (*pkey != NULL) {
            printf("TPM_RSAGeneratePrivateToken: Error (fatal), pkey %p should be NULL\n",
		   *pkey);
            rc = TPM_FAIL;
	}
    }
    /* construct the OpenSSL private key object */
    if (rc == 0) {
        *pkey = EVP_PKEY_new();                        /* freed by caller */
        if (*pkey == NULL) {
            printf("TPM_RSAGeneratePrivateToken: Error in EVP_PKEY_new()\n");
            rc = TPM_FAIL;
        }
    }
    if (rc == 0) {
        rc = TPM_bin2bn((TPM_BIGNUM *)&n, narr, nbytes);	/* freed by caller */
    }
    if (rc == 0) {
        rc = TPM_bin2bn((TPM_BIGNUM *)&e, earr, ebytes);	/* freed by caller */
    }
    if (rc == 0) {
        if (darr != NULL) {
            rc = TPM_bin2bn((TPM_BIGNUM *)&d, darr, dbytes);	/* freed by caller */
        }
    }
    if (rc == 0) {
        rsakey = RSA_new();
        if (rsakey == NULL) {
            printf("TPM_RSAGeneratePrivateToken: Error in RSA_new()\n");
            rc = TPM_FAIL;
        }
    }
    if (rc == 0) {
        irc = RSA_set0_key(rsakey, n, e, d);
        if (irc != 1) {
            printf("TPM_RSAGeneratePrivateToken: Error in RSA_set0_key()\n");
            rc = TPM_FAIL;
        } else {
            n = NULL;
            e = NULL;
            d = NULL;
        }
    }
    if (rc == 0) {
        RSA_set_flags(rsakey, RSA_FLAG_NO_BLINDING);
        irc = EVP_PKEY_assign_RSA(*pkey, rsakey);
        if (irc == 0) {
            printf("TPM_RSAGeneratePrivateToken: Error in EVP_PKEY_assign_RSA()\n");
            rc = TPM_FAIL;
        } else {
            rsakey = NULL;
        }
    }

    if (rc != 0) {
        EVP_PKEY_free(*pkey);
        *pkey = NULL;
        RSA_free(rsakey);
        BN_free(n);
        BN_free(e);
        BN_clear_free(d);
    }

    return rc;
}

#endif

