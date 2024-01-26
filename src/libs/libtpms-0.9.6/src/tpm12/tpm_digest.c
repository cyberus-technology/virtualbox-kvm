/********************************************************************************/
/*										*/
/*				Digest Handler					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_digest.c $		*/
/*										*/
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

#include <stdio.h>
#include <string.h>

#include "tpm_debug.h"
#include "tpm_error.h"
#include "tpm_structures.h"

#include "tpm_digest.h"

/* TPM_Digest_Init resets a digest structure to zeros */

void TPM_Digest_Init(TPM_DIGEST tpm_digest)
{
    printf("  TPM_Digest_Init:\n");
    memset(tpm_digest, 0, TPM_DIGEST_SIZE);
    return;
}

/* TPM_Digest_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
*/

TPM_RESULT TPM_Digest_Load(TPM_DIGEST tpm_digest,
			   unsigned char **stream,
			   uint32_t *stream_size)
{
    TPM_RESULT	rc = 0;

    printf("  TPM_Digest_Load:\n");
    rc = TPM_Loadn(tpm_digest, TPM_DIGEST_SIZE, stream, stream_size);
    return rc;
}

/* TPM_Digest_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes

   After use, call TPM_Sbuffer_Delete() to free memory
*/

TPM_RESULT TPM_Digest_Store(TPM_STORE_BUFFER *sbuffer,
			    const TPM_DIGEST tpm_digest)
{
    TPM_RESULT	rc = 0;

    printf("  TPM_Digest_Store:\n");
    rc = TPM_Sbuffer_Append(sbuffer, tpm_digest, TPM_DIGEST_SIZE);	
    return rc;
}

void TPM_Digest_Set(TPM_DIGEST tpm_digest)
{
    printf("  TPM_Digest_Set:\n");
    memset(tpm_digest, 0xff, TPM_DIGEST_SIZE);
}

void TPM_Digest_Copy(TPM_DIGEST destination, const TPM_DIGEST source)
{
    printf("  TPM_Digest_Copy:\n");
    memcpy(destination, source, TPM_DIGEST_SIZE);
    return;
}

void TPM_Digest_XOR(TPM_DIGEST out, const TPM_DIGEST in1, const TPM_DIGEST in2)
{
    size_t i;
    
    printf(" TPM_Digest_XOR:\n");
    for (i = 0 ; i < TPM_DIGEST_SIZE ; i++) {
	out[i] = in1[i] ^ in2[i];
    }
    return;
}

/* TPM_Digest_Compare() compares two digests, returning 0 if they are equal
 */

TPM_RESULT TPM_Digest_Compare(const TPM_DIGEST expect, const TPM_DIGEST actual)
{
    TPM_RESULT	rc = 0;

    printf("  TPM_Digest_Compare:\n");
    rc = memcmp(expect, actual, TPM_DIGEST_SIZE);
    if (rc != 0) {
	printf("TPM_Digest_Compare: Error comparing digest\n");
	TPM_PrintFour("   TPM_Digest_Compare: Expect", expect);
	TPM_PrintFour("   TPM_Digest_Compare: Actual", actual);
	rc = TPM_AUTHFAIL;
    }
    return rc;
}

void TPM_Digest_IsZero(TPM_BOOL *isZero, TPM_DIGEST tpm_digest)
{
    size_t i;

    printf("  TPM_Digest_IsZero:\n");
    for (i = 0, *isZero = TRUE ; (i < TPM_DIGEST_SIZE) && *isZero ; i++) {
	if (tpm_digest[i] != 0) {
	    *isZero = FALSE;
	}
    }
    return;
}

#if 0
void TPM_Digest_IsMinusOne(TPM_BOOL *isMinusOne, TPM_DIGEST tpm_digest)
{
    size_t i;

    printf("  TPM_Digest_IsMinusOne:\n");
    for (i = 0, *isMinusOne = TRUE ; (i < TPM_DIGEST_SIZE) && *isMinusOne ; i++) {
	if (tpm_digest[i] != 0xff) {
	    *isMinusOne = FALSE;
	}
    }
    return;
}
#endif
