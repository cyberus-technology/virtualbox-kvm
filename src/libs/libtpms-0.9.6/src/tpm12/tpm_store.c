/********************************************************************************/
/*                                                                              */
/*                              Safe Storage Buffer                             */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_store.c $             */
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

/* Generally useful utilities to serialize structures to a stream */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "tpm_commands.h"
#include "tpm_constants.h"
#include "tpm_crypto.h"
#include "tpm_debug.h"
#include "tpm_error.h"
#include "tpm_memory.h"
#include "tpm_process.h"

#include "tpm_store.h"

/*
  ->buffer;             beginning of buffer
  ->buffer_current;     first empty position in buffer
  ->buffer_end;         one past last valid position in buffer
*/

/* local prototypes */

static void       TPM_Sbuffer_AdjustParamSize(TPM_STORE_BUFFER *sbuffer);
static TPM_RESULT TPM_Sbuffer_AdjustReturnCode(TPM_STORE_BUFFER *sbuffer, TPM_RESULT returnCode);


/* TPM_Sbuffer_Init() sets up a new serialize buffer.  It should be called before the first use. */

void TPM_Sbuffer_Init(TPM_STORE_BUFFER *sbuffer)
{
    sbuffer->buffer = NULL;
    sbuffer->buffer_current = NULL;
    sbuffer->buffer_end = NULL;
}

/* TPM_Sbuffer_Load() loads TPM_STORE_BUFFER that has been serialized using
   TPM_Sbuffer_AppendAsSizedBuffer(), as a size plus stream.
*/

TPM_RESULT TPM_Sbuffer_Load(TPM_STORE_BUFFER *sbuffer,
                            unsigned char **stream,
                            uint32_t *stream_size)
{
    TPM_RESULT  rc = 0;
    uint32_t length;

    /* get the length of the stream to be loaded */
    if (rc == 0) {
        rc = TPM_Load32(&length, stream, stream_size);
    }
    /* check stream_size */
    if (rc == 0) {
        if (*stream_size < length) {
            printf("TPM_Sbuffer_Load: Error, stream_size %u less than %u\n",
                   *stream_size, length);
            rc = TPM_BAD_PARAM_SIZE;
        }
    }
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, *stream, length);
        *stream += length;
        *stream_size -= length;
    }    
    return rc;
}

/* TPM_Sbuffer_Store() cannot simply store the elements, as they are pointers.  Rather, the
   TPM_Sbuffer_AppendAsSizedBuffer() function is used.
*/

/* TPM_Sbuffer_Delete() frees an existing buffer and reinitializes it.  It must be called when a
   TPM_STORE_BUFFER is no longer required, to avoid a memory leak.  The buffer can be reused, but in
   that case TPM_Sbuffer_Clear would be a better choice. */

void TPM_Sbuffer_Delete(TPM_STORE_BUFFER *sbuffer)
{
    free(sbuffer->buffer);
    TPM_Sbuffer_Init(sbuffer);
}

/* TPM_Sbuffer_Clear() removes all data from an existing buffer, allowing reuse.  Memory is NOT
   freed. */

void TPM_Sbuffer_Clear(TPM_STORE_BUFFER *sbuffer)
{
    sbuffer->buffer_current = sbuffer->buffer;
    return;
}

/* TPM_Sbuffer_Get() gets the resulting byte buffer and its size. */

void TPM_Sbuffer_Get(TPM_STORE_BUFFER *sbuffer,
                     const unsigned char **buffer,
                     uint32_t *length)
{
    *length = sbuffer->buffer_current - sbuffer->buffer;
    *buffer = sbuffer->buffer;
    return;
}

/* TPM_Sbuffer_GetAll() gets the resulting byte buffer and its size, as well as the total size. */

void TPM_Sbuffer_GetAll(TPM_STORE_BUFFER *sbuffer,
			unsigned char **buffer,
			uint32_t *length,
			uint32_t *total)
{
    *length = sbuffer->buffer_current - sbuffer->buffer;
    *total = sbuffer->buffer_end - sbuffer->buffer;
    *buffer = sbuffer->buffer;
    return;
}

/* TPM_Sbuffer_Set() creates a TPM_STORE_BUFFER from

   'buffer' - pointer to a buffer that was allocated (can be NULL)

   'total' - the total number of allocated bytes (ignored if buffer is NULL)

   'length' - the number of valid bytes in buffer (ignored if buffer is NULL, can be 0, cannot be
   greater than total.
*/

TPM_RESULT TPM_Sbuffer_Set(TPM_STORE_BUFFER *sbuffer,
			   unsigned char *buffer,
			   const uint32_t length,
			   const uint32_t total)
{
    TPM_RESULT rc = 0;

    if (rc == 0) {
	if (sbuffer == NULL) {
	    printf("TPM_Sbuffer_Set: Error (fatal), sbuffer is NULL\n");
	    rc = TPM_FAIL;
	}
    }
    if (rc == 0) {
	if (buffer != NULL) {
	    if (rc == 0) {
		if (length > total) {
		    printf("TPM_Sbuffer_Set: Error (fatal), length %u > total %u\n",
			   length, total);
		    rc = TPM_FAIL;
		}
	    }
	    if (rc == 0) {
		sbuffer->buffer = buffer;
		sbuffer->buffer_current = buffer + length;
		sbuffer->buffer_end = buffer + total;
	    }
	}
	else {	/* buffer == NULL */
	    sbuffer->buffer = NULL;
	    sbuffer->buffer_current = NULL;
	    sbuffer->buffer_end = NULL;
	}
    }
    return rc;
}

/* TPM_Sbuffer_Append() is the basic function to append 'data' of size 'data_length' to the
   TPM_STORE_BUFFER

   Returns 0 if success, TPM_SIZE if the buffer cannot be allocated.
*/

TPM_RESULT TPM_Sbuffer_Append(TPM_STORE_BUFFER *sbuffer,
                              const unsigned char *data,
                              size_t data_length)
{
    TPM_RESULT  rc = 0;
    size_t free_length;         /* length of free bytes in current buffer */
    size_t current_size;        /* size of current buffer */
    size_t current_length;      /* bytes in current buffer */
    size_t new_size;            /* size of new buffer */
    
    /* can data fit? */
    if (rc == 0) {
        /* cast safe as end is always greater than current */
        free_length = (size_t)(sbuffer->buffer_end - sbuffer->buffer_current);
        /* if data cannot fit in buffer as sized */
        if (free_length < data_length) {
            /* This test will fail long before the add uint32_t overflow */
            if (rc == 0) {
                /* cast safe as current is always greater than start */
                current_length = (size_t)(sbuffer->buffer_current - sbuffer->buffer);
                if ((current_length + data_length) > TPM_ALLOC_MAX) {
                    printf("TPM_Sbuffer_Append: "
                           "Error, size %lu + %lu greater than maximum allowed\n",
                           (unsigned long)current_length, (unsigned long)data_length);
                    rc = TPM_SIZE;
                }
            }
            if (rc == 0) {
                /* cast safe as end is always greater than start */
                current_size = (size_t)(sbuffer->buffer_end - sbuffer->buffer);
                /* optimize realloc's by rounding up data_length to the next increment */
                new_size = current_size +       /* currently used */
                           ((((data_length - 1)/TPM_STORE_BUFFER_INCREMENT) + 1) *
                            TPM_STORE_BUFFER_INCREMENT);
                /* but not greater than maximum buffer size */
                if (new_size > TPM_ALLOC_MAX) {
                    new_size = TPM_ALLOC_MAX;
                }
                printf("   TPM_Sbuffer_Append: data_length %lu, growing from %lu to %lu\n",
                       (unsigned long)data_length,
                       (unsigned long)current_size,
                       (unsigned long)new_size);
                rc = TPM_Realloc(&(sbuffer->buffer), new_size);
            }
            if (rc == 0) {
                sbuffer->buffer_end = sbuffer->buffer + new_size;       /* end */
                sbuffer->buffer_current = sbuffer->buffer + current_length; /* new empty position */
            }
        }
    }
    /* append the data */
    if (rc == 0) {
        if (data_length > 0) { /* libtpms added (ubsan) */
            memcpy(sbuffer->buffer_current, data, data_length);
            sbuffer->buffer_current += data_length;
        }
    }
    return rc;
}

/* TPM_Sbuffer_Append8() is a special append that appends a uint8_t
 */

TPM_RESULT TPM_Sbuffer_Append8(TPM_STORE_BUFFER *sbuffer, uint8_t data)
{
    TPM_RESULT  rc = 0;

    rc = TPM_Sbuffer_Append(sbuffer, (const unsigned char *)(&data), sizeof(uint8_t));
    return rc;
}

/* TPM_Sbuffer_Append16() is a special append that converts a uint16_t to big endian (network byte
   order) and appends. */

TPM_RESULT TPM_Sbuffer_Append16(TPM_STORE_BUFFER *sbuffer, uint16_t data)
{
    TPM_RESULT  rc = 0;

    uint16_t ndata = htons(data);
    rc = TPM_Sbuffer_Append(sbuffer, (const unsigned char *)(&ndata), sizeof(uint16_t));
    return rc;
}

/* TPM_Sbuffer_Append32() is a special append that converts a uint32_t to big endian (network byte
   order) and appends. */

TPM_RESULT TPM_Sbuffer_Append32(TPM_STORE_BUFFER *sbuffer, uint32_t data)
{
    TPM_RESULT  rc = 0;

    uint32_t ndata = htonl(data);
    rc = TPM_Sbuffer_Append(sbuffer, (const unsigned char *)(&ndata), sizeof(uint32_t));
    return rc;
}

/* TPM_Sbuffer_AppendAsSizedBuffer() appends the source to the destination using the
   TPM_SIZED_BUFFER idiom.  That is, for a uint32_t size is stored.  Then the data is stored.

   Use this function when the stream is not self-describing and a size must be prepended.
*/

TPM_RESULT TPM_Sbuffer_AppendAsSizedBuffer(TPM_STORE_BUFFER *destSbuffer,
                                           TPM_STORE_BUFFER *srcSbuffer)
{
    TPM_RESULT  rc = 0;
    const unsigned char *buffer;
    uint32_t length;
    
    if (rc == 0) {
        TPM_Sbuffer_Get(srcSbuffer, &buffer, &length);
        rc = TPM_Sbuffer_Append32(destSbuffer, length);
    }
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(destSbuffer, buffer, length);
    }
    return rc;
}

/* TPM_Sbuffer_AppendSBuffer() appends the source to the destination.  The size is not prepended, so
   the stream must be self-describing.
*/

TPM_RESULT TPM_Sbuffer_AppendSBuffer(TPM_STORE_BUFFER *destSbuffer,
                                     TPM_STORE_BUFFER *srcSbuffer)
{
    TPM_RESULT  rc = 0;
    const unsigned char *buffer;
    uint32_t length;
    
    if (rc == 0) {
        TPM_Sbuffer_Get(srcSbuffer, &buffer, &length);
        rc = TPM_Sbuffer_Append(destSbuffer, buffer, length);
    }
    return rc;
}

/* TPM_Sbuffer_StoreInitialResponse() is a special purpose append specific to a TPM response.

   It appends the first 3 standard response parameters:
        - response_tag
        - parameter size
        - return code
        
   For some TPM commands, this is the entire response.  Other times, additional parameters 
   will be appended.  See TPM_Sbuffer_StoreFinalResponse().

   Returns:
        0 success
        TPM_SIZE response could not fit in buffer
*/

TPM_RESULT TPM_Sbuffer_StoreInitialResponse(TPM_STORE_BUFFER *response,
                                            TPM_TAG request_tag,
                                            TPM_RESULT returnCode)
{
    TPM_RESULT  rc = 0;
    TPM_TAG     response_tag;
    
    printf(" TPM_Sbuffer_StoreInitialResponse: returnCode %08x\n", returnCode);
    if (rc == 0) {
        if (request_tag == TPM_TAG_RQU_COMMAND) {
            response_tag = TPM_TAG_RSP_COMMAND;
        }
        else if (request_tag == TPM_TAG_RQU_AUTH1_COMMAND) {
            response_tag = TPM_TAG_RSP_AUTH1_COMMAND;
        }
        else if (request_tag == TPM_TAG_RQU_AUTH2_COMMAND) {
            response_tag = TPM_TAG_RSP_AUTH2_COMMAND;
        }
        /* input tag error, returnCode is handled by caller TPM_CheckRequestTag() */
        else {
            response_tag = TPM_TAG_RSP_COMMAND;
        }
    }
    /* tag */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append16(response, response_tag);
    }
    /* paramSize */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append32(response,
				  sizeof(TPM_TAG) + sizeof(uint32_t) + sizeof(TPM_RESULT));
    }
    /* returnCode */    
    if (rc == 0) {
        rc = TPM_Sbuffer_Append32(response, returnCode);
    }
    return rc;
}

/* TPM_Sbuffer_StoreFinalResponse() is a special purpose append specific to a TPM response.

   It is used after TPM_Sbuffer_StoreInitialResponse() and all additional parameters are appended.

   1 - If the additional parameters were successfully appended, this function adjusts the
   preliminary parameter size set by TPM_Sbuffer_StoreInitialResponse() to reflect the additional
   appends.

   2 - If there was a failure during the additional appends, this function adjusts the return code
   and removes the additional appends.
*/

TPM_RESULT TPM_Sbuffer_StoreFinalResponse(TPM_STORE_BUFFER *sbuffer,
                                          TPM_RESULT returnCode,
                                          tpm_state_t *tpm_state)
{
    TPM_RESULT  	rc = 0;
    const unsigned char *buffer;
    uint32_t 		length;

    printf(" TPM_Sbuffer_StoreFinalResponse: returnCode %08x\n", returnCode);
    /* determine whether the response would exceed the output buffer size */
    TPM_Sbuffer_Get(sbuffer, &buffer, &length);
    if (length > TPM12_GetBufferSize()) {
	printf("TPM_Sbuffer_StoreFinalResponse: Error, response buffer %u exceeds max %u\n",
	       length, TPM12_GetBufferSize());
	returnCode = TPM_SIZE;
    }
    if (returnCode == TPM_SUCCESS) {
        TPM_Sbuffer_AdjustParamSize(sbuffer);
    }
    else {
        /* TPM_FAIL is reserved for "should never occur" errors that indicate a software or hardware
           failure */
        if ((returnCode == TPM_FAIL) && (tpm_state != NULL)) {
	    printf("  TPM_Sbuffer_StoreFinalResponse: Set testState to %u \n",
		   TPM_TEST_STATE_FAILURE);
            tpm_state->testState = TPM_TEST_STATE_FAILURE;
        }
        rc = TPM_Sbuffer_AdjustReturnCode(sbuffer, returnCode);
    }
    return rc;
}

/* TPM_Sbuffer_AdjustParamSize() is a special purpose function to go back and adjust the response
   paramSize after the response buffer is complete
*/

static void TPM_Sbuffer_AdjustParamSize(TPM_STORE_BUFFER *sbuffer)
{
    uint32_t paramSize;   /* the correct paramsize */
    uint32_t nParamSize;  /* the correct paramsize, in network byte order */
    uint32_t paramSizeOffset;
    
    /* actual size */
    paramSize = sbuffer->buffer_current - sbuffer->buffer;
    paramSizeOffset = sizeof(TPM_TAG);
    nParamSize = htonl(paramSize);      /* network byte order */
    /* overwrite the original size */
    memcpy(sbuffer->buffer + paramSizeOffset, &nParamSize, sizeof(uint32_t));
    return;
}

/* TPM_Sbuffer_AdjustReturnCode() is a special function to go back and adjust the response tag and
   returnCode if there was a failure while appending the rest of the parameters.

   This should never fail, because sbuffer was allocated during TPM_Sbuffer_StoreInitialResponse().
*/

static TPM_RESULT TPM_Sbuffer_AdjustReturnCode(TPM_STORE_BUFFER *sbuffer, TPM_RESULT returnCode)
{
    TPM_RESULT  rc = 0;
    
    if (rc == 0) {
        /* erase the previous result without freeing the buffer */
        sbuffer->buffer_current = sbuffer->buffer;
        /* error tag */
        rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_RSP_COMMAND);
    }
    /* paramSize */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append32(sbuffer, sizeof(TPM_TAG) + sizeof(uint32_t) + sizeof(TPM_RESULT));
    }
    /* returnCode */    
    if (rc == 0) {
        rc = TPM_Sbuffer_Append32(sbuffer, returnCode);
    }
    return rc;
}

#if 0
/* Test appending to the TPM_STORE_BUFFER up to the limit */

TPM_RESULT TPM_Sbuffer_Test(void)
{
    TPM_RESULT  rc = 0;
    TPM_STORE_BUFFER sbuffer;
    size_t total_count;
    unsigned char count;
    unsigned char data[256];    /* dummy data */
    
    printf(" TPM_Sbuffer_Test:\n");
    TPM_Sbuffer_Init(&sbuffer);
    total_count = 0;
    while ((total_count != TPM_ALLOC_MAX) && rc == 0) {
        if (rc == 0) {
            rc = TPM_Random(&count, 1);
        }
        if (rc == 0) {
            printf(" TPM_Sbuffer_Test: count %u\n", count);
            /* last time through */
            if (total_count + count > TPM_ALLOC_MAX) {
                count = TPM_ALLOC_MAX - total_count;
            }
            rc = TPM_Sbuffer_Append(&sbuffer,data, count);
        }
        if (rc == 0) {
            total_count += count;
        }
        printf(" TPM_Sbuffer_Test: total_count %lu\n", (unsigned long)total_count);
    }
    TPM_Sbuffer_Delete(&sbuffer);
    return rc;
}
#endif

/* type to byte stream */
void STORE32(unsigned char *buffer, unsigned int offset, uint32_t value)
{
    buffer[offset + 0] = value >> 24;
    buffer[offset + 1] = value >> 16;
    buffer[offset + 2] = value >>  8;
    buffer[offset + 3] = value >>  0;
}

void STORE16(unsigned char *buffer, unsigned int offset, uint16_t value)
{
    buffer[offset + 0] = value >> 8;
    buffer[offset + 1] = value >> 0;
}

void STORE8(unsigned char *buffer, unsigned int offset, uint8_t value)

{
    buffer[offset + 0] = value >> 0;
}

/* TPM_Bitmap_Load() is a safe loading of a TPM_BOOL from a bitmap.

   If 'pos' is >= 32, the function fails.
   TPM_BOOL is TRUE. if The bit at pos is set
   'pos' is incremented after the load.
*/
   
TPM_RESULT TPM_Bitmap_Load(TPM_BOOL *tpm_bool,
			   uint32_t tpm_bitmap,
			   uint32_t *pos)
{
    TPM_RESULT  rc = 0;
    
    if (rc == 0) {
        if ((*pos) >= (sizeof(uint32_t) * CHAR_BIT)) {
            printf("TPM_Bitmap_Load: Error (fatal), loading from position %u\n", *pos);
            rc = TPM_FAIL;      /* should never occur */
        }
    }   
    if (rc == 0) {
        *tpm_bool = (tpm_bitmap & (1 << (*pos))) != 0;
	(*pos)++;
    }
    return rc;
}
 
/* TPM_Bitmap_Store() is a safe storing of a TPM_BOOL into a bitmap.

   If 'pos' is >= 32, the function fails.
   The bit at pos is set if the TPM_BOOL is TRUE.
   'pos' is incremented after the store.
*/
   
TPM_RESULT TPM_Bitmap_Store(uint32_t *tpm_bitmap,
			    TPM_BOOL tpm_bool,
			    uint32_t *pos)
{
    TPM_RESULT  rc = 0;
    
    if (rc == 0) {
        if ((*pos) >= (sizeof(uint32_t) * CHAR_BIT)) {
            printf("TPM_Bitmap_Store: Error (fatal), storing to position %u\n", *pos);
            rc = TPM_FAIL;      /* should never occur */
        }
    }   
    if (rc == 0) {
        if (tpm_bool) {
            *tpm_bitmap |= (1 << (*pos));
        }
        (*pos)++;
    }   
    return rc;
}
 
