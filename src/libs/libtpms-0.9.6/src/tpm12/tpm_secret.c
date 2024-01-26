/********************************************************************************/
/*                                                                              */
/*                              Secret Data Handler                             */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_secret.c $            */
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

#include <stdio.h>
#include <string.h>

#include "tpm_crypto.h"
#include "tpm_debug.h"
#include "tpm_error.h"
#include "tpm_store.h"

#include "tpm_secret.h"

void TPM_Secret_Init(TPM_SECRET tpm_secret)
{
    printf("  TPM_Secret_Init:\n");
    memset(tpm_secret, 0, TPM_SECRET_SIZE);
    return;
}

/* TPM_Secret_Load()
   
   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes

   After use, call TPM_Secret_Delete() to free memory
*/

TPM_RESULT TPM_Secret_Load(TPM_SECRET tpm_secret,
                           unsigned char **stream,
                           uint32_t *stream_size)
{
    TPM_RESULT  rc = 0;

    printf("  TPM_Secret_Load:\n");
    rc = TPM_Loadn(tpm_secret, TPM_SECRET_SIZE, stream, stream_size);
    return rc;
}

/* TPM_Secret_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes

   After use, call TPM_Sbuffer_Delete() to free memory
*/

TPM_RESULT TPM_Secret_Store(TPM_STORE_BUFFER *sbuffer,
                            const TPM_SECRET tpm_secret)
{
     TPM_RESULT rc = 0;

     printf("  TPM_Secret_Store:\n");
     rc = TPM_Sbuffer_Append(sbuffer, tpm_secret, TPM_SECRET_SIZE);     
     return rc;
}

/* TPM_Secret_Delete()
   
   No-OP if the parameter is NULL, else:
   frees memory allocated for the secret
   sets pointers to NULL
   calls TPM_Secret_Init to set members back to default values
   The secret itself is not freed
   returns 0 or error codes
*/   

void TPM_Secret_Delete(TPM_SECRET tpm_secret)
{
    printf("  TPM_Secret_Delete:\n");
    if (tpm_secret != NULL) {
        TPM_Secret_Init(tpm_secret);
    }
    return;
}

/* TPM_Secret_Copy() copies the source to the destination
 */

void TPM_Secret_Copy(TPM_SECRET destination, const TPM_SECRET source)
{
    printf("  TPM_Secret_Copy:\n");
    memcpy(destination, source, TPM_SECRET_SIZE);
    return;
}

/* TPM_Secret_Compare() compares the source to the destination.

   Returns TPM_AUTHFAIL if the nonces are not equal
*/

TPM_RESULT TPM_Secret_Compare(TPM_SECRET expect, const TPM_SECRET actual)
{
    TPM_RESULT  rc = 0;

    printf("  TPM_Secret_Compare:\n");
    rc = memcmp(expect, actual, TPM_SECRET_SIZE);
    if (rc != 0) {
        printf("TPM_Secret_Compare: Error comparing secret\n");
        rc = TPM_AUTHFAIL;
    }
    return rc;
}

/* TPM_Secret_Generate() generates a new TPM_SECRET from the random number generator
 */

TPM_RESULT TPM_Secret_Generate(TPM_SECRET tpm_secret)
{
    TPM_RESULT  rc = 0;

    printf("  TPM_Secret_Generate:\n");
    rc = TPM_Random(tpm_secret, TPM_SECRET_SIZE);
    return rc;
}

/* TPM_Secret_XOR() XOR's the source and the destination, and returns the result on output.
 */

void TPM_Secret_XOR(TPM_SECRET output, TPM_SECRET input1, TPM_SECRET input2)
{
    size_t i;

    printf("  TPM_Secret_XOR:\n");
    for (i = 0 ; i < TPM_SECRET_SIZE ; i++) {
        output[i] = input1[i] ^ input2[i];
    }
    return;
}
