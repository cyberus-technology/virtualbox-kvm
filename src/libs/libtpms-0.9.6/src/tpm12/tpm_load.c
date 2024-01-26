/********************************************************************************/
/*                                                                              */
/*                      Load from Stream Utilities                              */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*           $Id: tpm_load.c $               */
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

/* Generally useful utilities to deserialize structures from a stream */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tpm_debug.h"
#include "tpm_error.h"
#include "tpm_sizedbuffer.h"
#include "tpm_types.h"

#include "tpm_load.h"

/* The LOADn() functions convert a big endian stream to integer types */

uint32_t LOAD32(const unsigned char *buffer, unsigned int offset)
{
    unsigned int i;
    uint32_t result = 0;

    for (i = 0 ; i < 4 ; i++) {
	result <<= 8;
	result |= buffer[offset + i];
    }
    return result;
}

uint16_t LOAD16(const unsigned char *buffer, unsigned int offset)
{
    unsigned int i;
    uint16_t result = 0;

    for (i = 0 ; i < 2 ; i++) {
	result <<= 8;
	result |= buffer[offset + i];
    }
    return result;
}

uint8_t LOAD8(const unsigned char *buffer, unsigned int offset)
{
    uint8_t result = 0;

    result |= buffer[offset];
    return result;
}

/* TPM_Load32() loads 'tpm_uint32' from the stream.

   It checks that the stream has sufficient data, and adjusts 'stream'
   and 'stream_size' past the data.
*/

TPM_RESULT TPM_Load32(uint32_t *tpm_uint32,
                      unsigned char **stream,
                      uint32_t *stream_size)
{
    TPM_RESULT  rc = 0;
    
    /* check stream_size */
    if (rc == 0) {
        if (*stream_size < sizeof(uint32_t)) {
            printf("TPM_Load32: Error, stream_size %u less than %lu\n",
                   *stream_size, (unsigned long)sizeof(uint32_t));
            rc = TPM_BAD_PARAM_SIZE;
        }
    }
    /* load the parameter */
    if (rc == 0) {
        *tpm_uint32 = LOAD32(*stream, 0);
        *stream += sizeof (uint32_t);
        *stream_size -= sizeof (uint32_t);
    }
    return rc;
}

/* TPM_Load16() loads 'tpm_uint16' from the stream.

   It checks that the stream has sufficient data, and adjusts 'stream'
   and 'stream_size' past the data.
*/

TPM_RESULT TPM_Load16(uint16_t *tpm_uint16,
                      unsigned char **stream,
                      uint32_t *stream_size)
{
    TPM_RESULT  rc = 0;
    
    /* check stream_size */
    if (rc == 0) {
        if (*stream_size < sizeof(uint16_t)) {
            printf("TPM_Load16: Error, stream_size %u less than %lu\n",
                   *stream_size, (unsigned long)sizeof(uint16_t));
            rc = TPM_BAD_PARAM_SIZE;
        }
    }
    /* load the parameter */
    if (rc == 0) {
        *tpm_uint16 = LOAD16(*stream, 0);
        *stream += sizeof (uint16_t);
        *stream_size -= sizeof (uint16_t);
    }
    return rc;
}

TPM_RESULT TPM_Load8(uint8_t *tpm_uint8,
                     unsigned char **stream,
                     uint32_t *stream_size)
{
    TPM_RESULT  rc = 0;
    /* check stream_size */
    if (rc == 0) {
        if (*stream_size < sizeof(uint8_t)) {
            printf("TPM_Load8: Error, stream_size %u less than %lu\n",
                   *stream_size, (unsigned long)sizeof(uint8_t));
            rc = TPM_BAD_PARAM_SIZE;
        }
    }
    /* load the parameter */
    if (rc == 0) {
        *tpm_uint8 = LOAD8(*stream, 0);
        *stream += sizeof (uint8_t);
        *stream_size -= sizeof (uint8_t);
    }
    return rc;
}

/* Boolean incoming parameter values other than 0x00 and 0x01 have an implementation specific
   interpretation.  The TPM SHOULD return TPM_BAD_PARAMETER.
*/

TPM_RESULT TPM_LoadBool(TPM_BOOL *tpm_bool,
                        unsigned char **stream,
                        uint32_t *stream_size)
{
    TPM_RESULT  rc = 0;
    /* check stream_size */
    if (rc == 0) {
        if (*stream_size < sizeof(TPM_BOOL)) {
            printf("TPM_LoadBool: Error, stream_size %u less than %lu\n",
                   *stream_size, (unsigned long)sizeof(TPM_BOOL));
            rc = TPM_BAD_PARAM_SIZE;
        }
    }
    /* load the parameter */
    if (rc == 0) {
        *tpm_bool = LOAD8(*stream, 0);
        *stream += sizeof (uint8_t);
        *stream_size -= sizeof (uint8_t);
    }
    if (rc == 0) {
        if ((*tpm_bool != TRUE) && (*tpm_bool != FALSE)) {
            printf("TPM_LoadBool: Error, illegal value %02x\n", *tpm_bool);
            rc = TPM_BAD_PARAMETER;
        }
    }
    return rc;
}

/* TPM_Loadn() copies 'data_length' bytes from 'stream' to 'data' with
   no endian adjustments. */

TPM_RESULT TPM_Loadn(BYTE *data,
                     size_t data_length,
                     unsigned char **stream,
                     uint32_t *stream_size)
{
    TPM_RESULT  rc = 0;
    /* check stream_size */
    if (rc == 0) {
        if (*stream_size < data_length) {
            printf("TPM_Loadn: Error, stream_size %u less than %lu\n",
                   *stream_size, (unsigned long)data_length);
            rc = TPM_BAD_PARAM_SIZE;
        }
    }
    /* load the parameter */
    if (rc == 0) {
        memcpy(data, *stream, data_length);
        *stream += data_length;
        *stream_size -= data_length;
    }
    return rc;
}

/* TPM_LoadLong() creates a long from a stream in network byte order.

   The stream is not advanced.
*/

TPM_RESULT TPM_LoadLong(unsigned long *result,
                        const unsigned char *stream,
                        uint32_t stream_size)
{
    TPM_RESULT          rc = 0;
    size_t		i;		/* byte iterator */

    printf(" TPM_LoadLong:\n");
    if (rc == 0) {
        if (stream_size > sizeof(unsigned long)) {
            printf(" TPM_LoadLong: Error, stream size %u too large\n", stream_size);
            rc = TPM_BAD_PARAM_SIZE;
        }
    }
    if (rc == 0) {
	*result = 0;    /* initialize all bytes to 0 in case buffer is less than sizeof(unsigned
			   long) bytes */
	for (i = 0 ; i < stream_size ; i++) {
	    /* copy big endian stream, put lowest address in an upper byte, highest address in byte
	       0 */
	    *result |= (unsigned long)(((unsigned long)stream[i]) << ((stream_size - 1 - i) * 8));
	}
	printf(" TPM_LoadLong: Result %08lx\n", *result);
    }
    return rc;
}

#if 0
/* TPM_LoadString() returns a pointer to a C string.  It does not copy the string.

 */

TPM_RESULT TPM_LoadString(const char **name,
                          unsigned char **stream,
                          uint32_t *stream_size)
{
    TPM_RESULT          rc = 0;
    char                *ptr;
    
    *name = NULL;
    /* search for the first nul character */
    if (rc == 0) {      
        ptr = memchr(*stream, (int)'\0', *stream_size);
        if (ptr == NULL) {
            rc = TPM_BAD_PARAM_SIZE;
        }
    }
    if (rc == 0) {
        *name = (char *)*stream;        /* cast because converting binary to string */
        *stream_size -= (ptr - *name) + 1;
        *stream = (unsigned char *)ptr + 1;
    }
    return rc;
}
#endif

/* TPM_CheckTag() loads a TPM_STRUCTURE_TAG from 'stream'.  It check that the value is 'expectedTag'
   and returns TPM_INVALID_STRUCTURE on error.

*/

TPM_RESULT TPM_CheckTag(TPM_STRUCTURE_TAG expectedTag,
			unsigned char **stream,
			uint32_t   *stream_size)
{
    TPM_RESULT          rc = 0;
    TPM_STRUCTURE_TAG   tag;
                           
    if (rc == 0) {      
        rc = TPM_Load16(&tag, stream, stream_size);
    }
    if (rc == 0) {
        if (tag != expectedTag) {
            printf("TPM_CheckTag: Error, tag expected %04x found %04hx\n", expectedTag, tag);
            rc = TPM_INVALID_STRUCTURE;
        }
    }
    return rc;
}

