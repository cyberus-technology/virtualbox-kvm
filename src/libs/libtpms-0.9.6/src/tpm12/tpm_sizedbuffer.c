/********************************************************************************/
/*                                                                              */
/*                        TPM Sized Buffer Handler                              */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_sizedbuffer.c $       */
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
#include <string.h>

#include "tpm_cryptoh.h"
#include "tpm_debug.h"
#include "tpm_error.h"
#include "tpm_memory.h"
#include "tpm_process.h"
#include "tpm_types.h"

#include "tpm_sizedbuffer.h"

void TPM_SizedBuffer_Init(TPM_SIZED_BUFFER *tpm_sized_buffer)
{
    tpm_sized_buffer->size = 0;
    tpm_sized_buffer->buffer = NULL;
    return;
}

/* TPM_SizedBuffer_Load() allocates and sets a sized buffer from a stream.  A sized buffer structure
   has two members:

   - 4 bytes size
   - pointer to array of 'size'

   This structure is typically a cast from a subset of a larger TPM structure.  Two members - a 4
   bytes size followed by a 4 bytes pointer to the data is a common TPM structure idiom.

   This function correctly handles a 'size' of 0.

   Call TPM_SizedBuffer_Init() before first use
   Call TPM_SizedBuffer_Delete() after use
*/

TPM_RESULT TPM_SizedBuffer_Load(TPM_SIZED_BUFFER *tpm_sized_buffer,     /* result */
                                unsigned char **stream,		/* pointer to next parameter */
                                uint32_t *stream_size)		/* stream size left */
{
    TPM_RESULT  rc = 0;
    
    printf("  TPM_SizedBuffer_Load:\n");
    if (rc == 0) {
        rc = TPM_Load32(&(tpm_sized_buffer->size), stream, stream_size);
    }
    /* if the size is not 0 */
    if ((rc == 0) && (tpm_sized_buffer->size > 0)) {
        /* allocate memory for the buffer */
        if (rc == 0) {
            rc = TPM_Malloc(&(tpm_sized_buffer->buffer), tpm_sized_buffer->size);
        }
        /* copy the buffer */
        if (rc == 0) {
            rc = TPM_Loadn(tpm_sized_buffer->buffer, tpm_sized_buffer->size, stream, stream_size);
        }
    }
    return rc;
}

/* TPM_SizedBuffer_Set() reallocs a sized buffer and copies 'size' bytes of 'data' into it.

   If the sized buffer already has data, the buffer is realloc'ed.

   This function correctly handles a 'size' of 0.
   
   Call TPM_SizedBuffer_Delete() to free the buffer
*/

TPM_RESULT TPM_SizedBuffer_Set(TPM_SIZED_BUFFER *tpm_sized_buffer,
                               uint32_t size,
                               const unsigned char *data)
{
    TPM_RESULT  rc = 0;
    
    printf("  TPM_SizedBuffer_Set:\n");
    /* allocate memory for the buffer, and copy the buffer */
    if (rc == 0) {
        if (size > 0) {
            rc = TPM_Realloc(&(tpm_sized_buffer->buffer),
                             size);
            if (rc == 0) {
                tpm_sized_buffer->size = size;
                memcpy(tpm_sized_buffer->buffer, data, size);
            }
        }
        /* if size is zero */
        else {
            TPM_SizedBuffer_Delete(tpm_sized_buffer);
        }
    }
    return rc;
}

/* TPM_SizedBuffer_SetFromStore() reallocs a sized buffer and copies 'sbuffer" data into it.

   This function correctly handles an 'sbuffer' of 0 length.
 */

TPM_RESULT TPM_SizedBuffer_SetFromStore(TPM_SIZED_BUFFER *tpm_sized_buffer,
                                        TPM_STORE_BUFFER *sbuffer)
{
    TPM_RESULT          rc = 0;
    const unsigned char *data;
    uint32_t              size;
    
    if (rc == 0) {
        /* get the stream and its size from the TPM_STORE_BUFFER */
        TPM_Sbuffer_Get(sbuffer, &data, &size);
        rc = TPM_SizedBuffer_Set(tpm_sized_buffer, size, data);
    }
    return rc;
}

/* TPM_SizedBuffer_SetStructure() serializes the structure 'tpmStructure' using the function
   'storeFunction', storing the result in a TPM_SIZED_BUFFER.
*/

TPM_RESULT TPM_SizedBuffer_SetStructure(TPM_SIZED_BUFFER *tpm_sized_buffer,
                                        void *tpmStructure,
                                        TPM_STORE_FUNCTION_T storeFunction)
{
    TPM_RESULT          rc = 0;
    TPM_STORE_BUFFER    sbuffer;        /* serialized tpmStructure */

    printf("  TPM_SizedBuffer_SetStructure:\n");
    TPM_Sbuffer_Init(&sbuffer);                         /* freed @1 */
    /* serialize the structure */
    if (rc == 0) {
        if (tpmStructure != NULL) {
            rc = storeFunction(&sbuffer, tpmStructure);
        }
    }
    /* copy to TPM_SIZED_BUFFER */
    if (rc == 0) {
        rc = TPM_SizedBuffer_SetFromStore(tpm_sized_buffer, &sbuffer);
    }
    TPM_Sbuffer_Delete(&sbuffer);                       /* @1 */
    return rc;
}

TPM_RESULT TPM_SizedBuffer_Copy(TPM_SIZED_BUFFER *tpm_sized_buffer_dest,
                                TPM_SIZED_BUFFER *tpm_sized_buffer_src)
{
    TPM_RESULT  rc = 0;
    rc = TPM_SizedBuffer_Set(tpm_sized_buffer_dest,
                             tpm_sized_buffer_src->size,
                             tpm_sized_buffer_src->buffer);
    return rc;
}


/* TPM_SizedBuffer_Store() serializes a TPM_SIZED_BUFFER into a TPM_STORE_BUFFER
 */

TPM_RESULT TPM_SizedBuffer_Store(TPM_STORE_BUFFER *sbuffer,
                                 const TPM_SIZED_BUFFER *tpm_sized_buffer)
{
    TPM_RESULT  rc = 0;

    printf("  TPM_SizedBuffer_Store:\n");
    /* append the size */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append32(sbuffer, tpm_sized_buffer->size);
    }
    /* append the data */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, tpm_sized_buffer->buffer, tpm_sized_buffer->size);
    }
    return rc;
}

void TPM_SizedBuffer_Delete(TPM_SIZED_BUFFER *tpm_sized_buffer)
{
    printf("  TPM_SizedBuffer_Delete:\n");
    if (tpm_sized_buffer != NULL) {
        free(tpm_sized_buffer->buffer);
        TPM_SizedBuffer_Init(tpm_sized_buffer);
    }
    return;
}

/* TPM_SizedBuffer_Allocate() allocates 'size' bytes of memory and sets the TPM_SIZED_BUFFER
   members.

   The buffer data is not initialized.
*/

TPM_RESULT TPM_SizedBuffer_Allocate(TPM_SIZED_BUFFER *tpm_sized_buffer,
                                    uint32_t size)
{
    TPM_RESULT  rc = 0;

    printf("  TPM_SizedBuffer_Allocate: Size %u\n", size);
    tpm_sized_buffer->size = size;
    rc = TPM_Malloc(&(tpm_sized_buffer->buffer), size);
    return rc;
}

/* TPM_SizedBuffer_GetBool() converts from a TPM_SIZED_BUFFER to a TPM_BOOL.

   If the size does not indicate a TPM_BOOL, an error is returned.
*/

TPM_RESULT TPM_SizedBuffer_GetBool(TPM_BOOL *tpm_bool,
                                   TPM_SIZED_BUFFER *tpm_sized_buffer)
{
    TPM_RESULT rc = 0;
    
    if (tpm_sized_buffer->size == sizeof(TPM_BOOL)) {
        *tpm_bool = *(TPM_BOOL *)tpm_sized_buffer->buffer;
        printf("  TPM_SizedBuffer_GetBool: bool %02x\n", *tpm_bool);
    }
    else {
        printf("TPM_SizedBuffer_GetBool: Error, buffer size %08x is not a BOOL\n",
               tpm_sized_buffer->size);
        rc = TPM_BAD_PARAMETER;
    }
    return rc;
}

/* TPM_SizedBuffer_GetUint32() converts from a TPM_SIZED_BUFFER to a uint32_t.

   If the size does not indicate a uint32_t, an error is returned.
*/

TPM_RESULT TPM_SizedBuffer_GetUint32(uint32_t *uint32,
                                     TPM_SIZED_BUFFER *tpm_sized_buffer)
{
    TPM_RESULT rc = 0;
    unsigned char *stream;
    uint32_t stream_size;
    
    if (rc == 0) {
        if (tpm_sized_buffer->size != sizeof(uint32_t)) {
            printf("TPM_GetUint32: Error, buffer size %08x is not a uint32_t\n",
                   tpm_sized_buffer->size);
            rc = TPM_BAD_PARAMETER;
        }
    }
    if (rc == 0) {
        stream = tpm_sized_buffer->buffer;
        stream_size = tpm_sized_buffer->size;
        rc = TPM_Load32(uint32, &stream, &stream_size);
    }
    return rc;
}

/* TPM_SizedBuffer_Append32() appends a uint32_t to a TPM_SIZED_BUFFER

*/

TPM_RESULT TPM_SizedBuffer_Append32(TPM_SIZED_BUFFER *tpm_sized_buffer,
                                    uint32_t uint32)
{
    TPM_RESULT rc = 0;

    printf("  TPM_SizedBuffer_Append32: Current size %u uint32 %08x\n",
           tpm_sized_buffer->size, uint32);
    /* allocate space for another uint32_t */
    if (rc == 0) {
        rc = TPM_Realloc(&(tpm_sized_buffer->buffer),
                         tpm_sized_buffer->size + sizeof(uint32_t));
    }
    if (rc == 0) {
        uint32_t ndata = htonl(uint32);           /* convert to network byte order */
        memcpy(tpm_sized_buffer->buffer + tpm_sized_buffer->size, /* append at end */
               (char *)&ndata,                  /* cast safe after conversion */
               sizeof(uint32_t));
        tpm_sized_buffer->size += sizeof(uint32_t);
    }    
    return rc;
}

/* TPM_SizedBuffer_Remove32() removes the uint32_t with value from a TPM_SIZED_BUFFER

*/

TPM_RESULT TPM_SizedBuffer_Remove32(TPM_SIZED_BUFFER *tpm_sized_buffer,
                                    uint32_t uint32)
{
    TPM_RESULT		rc = 0;
    unsigned char	*stream;
    uint32_t		stream_size;
    uint32_t		bufferValue;
    TPM_BOOL		found = FALSE;
    unsigned char	*from;
    unsigned char	*to;
        
    printf("  TPM_SizedBuffer_Remove32: Current size %u uint32 %08x\n",
           tpm_sized_buffer->size, uint32);

    stream = tpm_sized_buffer->buffer;
    stream_size =  tpm_sized_buffer->size;
    
    /* search for the uint32 */
    while ((rc == 0) && (stream_size != 0) && !found) {
        /* get the next value */
        if (rc == 0) {
            rc = TPM_Load32(&bufferValue, &stream, &stream_size);
        }
        /* if the value is the one to be removed */
        if (rc == 0) {
            if (bufferValue == uint32) {
                found = TRUE;
                /* shift the reset of the buffer down by a uint32_t */
                for (from = stream, to = (stream - sizeof(uint32_t)) ;
                     /* go to the end of the buffer */
                     from < (tpm_sized_buffer->buffer + tpm_sized_buffer->size) ;
                     from++, to++) {
                    *to = *from;
                }
                /* adjust the size */
                tpm_sized_buffer->size -= sizeof(uint32_t);
            }
        }
    }
    if (!found) {
        printf("TPM_SizedBuffer_Remove32: Error, value not found\n");
        rc = TPM_BAD_HANDLE;
    }
    return rc;
}

/* TPM_SizedBuffer_Zero() overwrites all data in the buffer with zeros

 */

void TPM_SizedBuffer_Zero(TPM_SIZED_BUFFER *tpm_sized_buffer)
{
    printf("  TPM_SizedBuffer_Zero:\n");
    if (tpm_sized_buffer->buffer != NULL) {
        memset(tpm_sized_buffer->buffer, 0, tpm_sized_buffer->size);
    }
    return;
}
