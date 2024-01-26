/********************************************************************************/
/*                                                                              */
/*                           TPM Memory Allocation                              */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_memory.c $            */
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
#include <stdlib.h>

#include "tpm_debug.h"
#include "tpm_error.h"

#include "tpm_memory.h"

/* TPM_Malloc() is a general purpose wrapper around malloc()
 */

TPM_RESULT TPM_Malloc(unsigned char **buffer, uint32_t size)
{
    TPM_RESULT          rc = 0;
    
    /* assertion test.  The coding style requires that all allocated pointers are initialized to
       NULL.  A non-NULL value indicates either a missing initialization or a pointer reuse (a
       memory leak). */
    if (rc == 0) {
        if (*buffer != NULL) {
            printf("TPM_Malloc: Error (fatal), *buffer %p should be NULL before malloc\n", *buffer);
            rc = TPM_FAIL;
        }
    }
    /* verify that the size is not "too large" */
    if (rc == 0) {
        if (size > TPM_ALLOC_MAX) {
            printf("TPM_Malloc: Error, size %u greater than maximum allowed\n", size);
            rc = TPM_SIZE;
        }       
    }
    /* verify that the size is not 0, this would be implementation defined and should never occur */
    if (rc == 0) {
        if (size == 0) {
            printf("TPM_Malloc: Error (fatal), size is zero\n");
            rc = TPM_FAIL;
        }       
    }
    if (rc == 0) {
        *buffer = malloc(size);
        if (*buffer == NULL) {
            printf("TPM_Malloc: Error allocating %u bytes\n", size);
            rc = TPM_SIZE;
        }
    }
    return rc;
}

/* TPM_Realloc() is a general purpose wrapper around realloc()
 */

TPM_RESULT TPM_Realloc(unsigned char **buffer,
                       uint32_t size)
{
    TPM_RESULT          rc = 0;
    unsigned char       *tmpptr = NULL;
    
    /* verify that the size is not "too large" */
    if (rc == 0) {
        if (size > TPM_ALLOC_MAX) {
            printf("TPM_Realloc: Error, size %u greater than maximum allowed\n", size);
            rc = TPM_SIZE;
        }       
    }
    if (rc == 0) {
        tmpptr = realloc(*buffer, size);
        if (tmpptr == NULL) {
            printf("TPM_Realloc: Error reallocating %u bytes\n", size);
            rc = TPM_SIZE;
        }
    }
    if (rc == 0) {
        *buffer = tmpptr;
    }
    return rc;
}

/* TPM_Free() is the companion to the TPM allocation functions.  It is not used internally.  The
   intent is for use by an application that links directly to a TPM and wants to free memory
   allocated by the TPM.

   It avoids a potential problem if the application uses a different allocation library, perhaps one
   that wraps the functions to detect overflows or memory leaks.
*/

void TPM_Free(unsigned char *buffer)
{
    free(buffer);
    return;
}

