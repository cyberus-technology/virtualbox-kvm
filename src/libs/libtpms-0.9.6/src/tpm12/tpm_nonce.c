/********************************************************************************/
/*                                                                              */
/*                              Nonce Handler                                   */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_nonce.c $             */
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
#include "tpm_structures.h"

#include "tpm_nonce.h"

/* TPM_Nonce_Init resets a nonce structure to zeros */

void TPM_Nonce_Init(TPM_NONCE tpm_nonce)
{
    size_t i;

    printf("  TPM_Nonce_Init:\n");
    for (i = 0 ; i < TPM_NONCE_SIZE ; i++) {
        tpm_nonce[i] = 0;
    }
    return;
}

/* TPM_Nonce_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
*/


TPM_RESULT TPM_Nonce_Load(TPM_NONCE tpm_nonce,
                          unsigned char **stream,
                          uint32_t *stream_size)
{
    TPM_RESULT  rc = 0;
    
    printf("  TPM_Nonce_Load:\n");
    rc = TPM_Loadn(tpm_nonce, TPM_NONCE_SIZE, stream, stream_size);
    return rc;
}

/* TPM_Nonce_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes

   After use, call TPM_Sbuffer_Delete() to free memory
*/

TPM_RESULT TPM_Nonce_Store(TPM_STORE_BUFFER *sbuffer,
                           const TPM_NONCE tpm_nonce)
{
    TPM_RESULT  rc = 0;

    printf("  TPM_Nonce_Store:\n");
    rc = TPM_Sbuffer_Append(sbuffer, tpm_nonce, TPM_NONCE_SIZE);        
    return rc;
}

/* TPM_Nonce_Copy() copies the source to the destination
 */

void TPM_Nonce_Copy(TPM_NONCE destination, const TPM_NONCE source)
{
    printf("  TPM_Nonce_Copy:\n");
    memcpy(destination, source, TPM_NONCE_SIZE);
    return;
}

/* TPM_Nonce_Compare() compares the source to the destination.

   Returns TPM_AUTHFAIL if the nonces are not equal
*/

TPM_RESULT TPM_Nonce_Compare(TPM_NONCE expect, const TPM_NONCE actual)
{
    TPM_RESULT  rc = 0;

    printf("  TPM_Nonce_Compare:\n");
    rc = memcmp(expect, actual, TPM_NONCE_SIZE);
    if (rc != 0) {
        printf("TPM_Nonce_Compare: Error comparing nonce\n");
        TPM_PrintFour(" TPM_Nonce_Compare: Expect", expect);
        TPM_PrintFour(" TPM_Nonce_Compare: Actual", actual);
        rc = TPM_AUTHFAIL;
    }
    return rc;
}

/* TPM_Nonce_Generate() generates a new nonce from the random number generator
 */

TPM_RESULT TPM_Nonce_Generate(TPM_NONCE tpm_nonce)
{
    TPM_RESULT  rc = 0;

    printf("  TPM_Nonce_Generate:\n");
    rc = TPM_Random(tpm_nonce, TPM_NONCE_SIZE);
    return rc;
}

/* TPM_Nonce_IsZero() returns 'isZero' TRUE is all bytes 'tpm_nonce' are 0x00
 */

void TPM_Nonce_IsZero(TPM_BOOL *isZero, TPM_NONCE tpm_nonce)
{
    size_t i;

    printf("  TPM_Nonce_IsZero:\n");
    for (i = 0, *isZero = TRUE ; (i < TPM_NONCE_SIZE) && *isZero ; i++) {
        if (tpm_nonce[i] != 0) {
            *isZero = FALSE;
        }
    }
    return;
}

