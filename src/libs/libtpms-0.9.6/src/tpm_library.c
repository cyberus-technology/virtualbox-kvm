/********************************************************************************/
/*										*/
/*			LibTPM interface functions				*/
/*			     Written by Stefan Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_library.c $		*/
/*										*/
/* (c) Copyright IBM Corporation 2010.						*/
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

#include <config.h>

#include <assert.h>
#include <string.h>
#if defined __FreeBSD__
# define _WITH_DPRINTF
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#ifndef VBOX
#include <unistd.h>
#endif
#include <stdbool.h>

#ifdef USE_FREEBL_CRYPTO_LIBRARY
# include <plbase64.h>
#endif

#ifdef USE_OPENSSL_CRYPTO_LIBRARY
# ifndef VBOX
# include <openssl/bio.h>
# include <openssl/evp.h>
# else
#  include <iprt/errcore.h>
#  include <iprt/base64.h>
# endif
#endif

#include "tpm_debug.h"
#include "tpm_error.h"
#include "tpm_library.h"
#include "tpm_library_intern.h"
#include "tpm_nvfilename.h"
#include "tpm_tis.h"

static const struct tags_and_indices {
    const char    *starttag;
    const char    *endtag;
} tags_and_indices[] = {
  [TPMLIB_BLOB_TYPE_INITSTATE] =
    {
      .starttag = TPMLIB_INITSTATE_START_TAG,
      .endtag   = TPMLIB_INITSTATE_END_TAG,
    },
};

static const struct tpm_interface *const tpm_iface[] = {
#if WITH_TPM1
    &TPM12Interface,
#else
    &DisabledInterface,
#endif
#if WITH_TPM2
    &TPM2Interface,
#else
    &DisabledInterface,
#endif
    NULL,
};

static int debug_fd = -1;
static unsigned debug_level = 0;
static char *debug_prefix = NULL;

static struct sized_buffer cached_blobs[TPMLIB_STATE_SAVE_STATE + 1];

static int tpmvers_choice = 0; /* default is TPM1.2 */
static TPM_BOOL tpmvers_locked = FALSE;

uint32_t TPMLIB_GetVersion(void)
{
    return TPM_LIBRARY_VERSION;
}

TPM_RESULT TPMLIB_ChooseTPMVersion(TPMLIB_TPMVersion ver)
{
    /* TPMLIB_Terminate will reset previous choice */
    if (tpmvers_locked)
        return TPM_FAIL;

    switch (ver) {
#if WITH_TPM1
    case TPMLIB_TPM_VERSION_1_2:
        if (tpmvers_choice != 0)
            ClearAllCachedState();

        tpmvers_choice = 0; // entry 0 in tpm_iface
        return TPM_SUCCESS;
#endif
#if WITH_TPM2
    case TPMLIB_TPM_VERSION_2:
        if (tpmvers_choice != 1)
            ClearAllCachedState();

        tpmvers_choice = 1; // entry 1 in tpm_iface
        return TPM_SUCCESS;
#endif
    default:
        return TPM_FAIL;
    }
}

TPM_RESULT TPMLIB_MainInit(void)
{
    if (!tpm_iface[tpmvers_choice]) {
        return TPM_FAIL;
    }

    tpmvers_locked = TRUE;

    return tpm_iface[tpmvers_choice]->MainInit();
}

void TPMLIB_Terminate(void)
{
    tpm_iface[tpmvers_choice]->Terminate();

    tpmvers_locked = FALSE;
}

/*
 * Send a command to the TPM. The command buffer must hold a well formatted
 * TPM command and the command_size indicate the size of the command.
 * The respbuffer parameter may be provided by the user and grow if
 * the respbufsize size indicator is determined to be too small for the
 * response. In that case a new buffer will be allocated and the size of that
 * buffer returned in the respbufsize parameter. resp_size describes the
 * size of the actual response within the respbuffer.
 */
TPM_RESULT TPMLIB_Process(unsigned char **respbuffer, uint32_t *resp_size,
                          uint32_t *respbufsize,
		          unsigned char *command, uint32_t command_size)
{
    return tpm_iface[tpmvers_choice]->Process(respbuffer,
                                 resp_size, respbufsize,
                                 command, command_size);
}

/*
 * Get the volatile state from the TPM. This function will return the
 * buffer and the length of the buffer to the caller in case everything
 * went alright.
 */
TPM_RESULT TPMLIB_VolatileAll_Store(unsigned char **buffer,
                                    uint32_t *buflen)
{
    return tpm_iface[tpmvers_choice]->VolatileAllStore(buffer, buflen);
}

/*
 *  Have the TPM cancel an ongoing command
 */
TPM_RESULT TPMLIB_CancelCommand(void)
{
    return tpm_iface[tpmvers_choice]->CancelCommand();
}

/*
 * Get a property of the TPM. The functions currently only
 * return compile-time #defines but this may change in future
 * versions where we may return parameters with which the TPM
 * was created (rather than compiled).
 */
TPM_RESULT TPMLIB_GetTPMProperty(enum TPMLIB_TPMProperty prop,
                                 int *result)
{
    switch (prop) {
    case  TPMPROP_TPM_BUFFER_MAX:
        *result = TPM_BUFFER_MAX;
        break;

    default:
        return tpm_iface[tpmvers_choice]->GetTPMProperty(prop, result);
    }

    return TPM_SUCCESS;
}

char *TPMLIB_GetInfo(enum TPMLIB_InfoFlags flags)
{
    return tpm_iface[tpmvers_choice]->GetInfo(flags);
}

TPM_RESULT TPMLIB_SetState(enum TPMLIB_StateType st,
                           const unsigned char *buffer, uint32_t buflen)
{
    return tpm_iface[tpmvers_choice]->SetState(st, buffer, buflen);
}

TPM_RESULT TPMLIB_GetState(enum TPMLIB_StateType st,
                           unsigned char **buffer, uint32_t *buflen)
{
    return tpm_iface[tpmvers_choice]->GetState(st, buffer, buflen);
}

TPM_RESULT TPM_IO_Hash_Start(void)
{
    return tpm_iface[tpmvers_choice]->HashStart();
}

TPM_RESULT TPM_IO_Hash_Data(const unsigned char *data, uint32_t data_length)
{
    return tpm_iface[tpmvers_choice]->HashData(data, data_length);
}

TPM_RESULT TPM_IO_Hash_End(void)
{
    return tpm_iface[tpmvers_choice]->HashEnd();
}

TPM_RESULT TPM_IO_TpmEstablished_Get(TPM_BOOL *tpmEstablished)
{
    return tpm_iface[tpmvers_choice]->TpmEstablishedGet(tpmEstablished);
}

TPM_RESULT TPM_IO_TpmEstablished_Reset(void)
{
    return tpm_iface[tpmvers_choice]->TpmEstablishedReset();
}

uint32_t TPMLIB_SetBufferSize(uint32_t wanted_size,
                              uint32_t *min_size,
                              uint32_t *max_size)
{
    return tpm_iface[tpmvers_choice]->SetBufferSize(wanted_size,
                                                    min_size,
                                                    max_size);
}

TPM_RESULT TPMLIB_ValidateState(enum TPMLIB_StateType st,
                                unsigned int flags)
{
    return tpm_iface[tpmvers_choice]->ValidateState(st, flags);
}

static struct libtpms_callbacks libtpms_cbs;

struct libtpms_callbacks *TPMLIB_GetCallbacks(void)
{
    return &libtpms_cbs;
}

TPM_RESULT TPMLIB_RegisterCallbacks(struct libtpms_callbacks *callbacks)
{
    int max_size = sizeof(struct libtpms_callbacks);

    /* restrict the size of the structure to what we know currently
       future versions may know more callbacks */
    if (callbacks->sizeOfStruct < max_size)
        max_size = callbacks->sizeOfStruct;

    /* clear the internal callback structure and copy the user provided
       callbacks into it */
    memset(&libtpms_cbs, 0x0, sizeof(libtpms_cbs));
    memcpy(&libtpms_cbs, callbacks, max_size);

    return TPM_SUCCESS;
}

static int is_base64ltr(char c)
{
    return ((c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
             c == '+' ||
             c == '/' ||
             c == '=');
}

#ifdef USE_OPENSSL_CRYPTO_LIBRARY
static unsigned char *TPMLIB_OpenSSL_Base64Decode(char *input,
                                                  unsigned int outputlen)
{
#ifndef VBOX
    BIO *b64, *bmem;
    unsigned char *res = NULL;
    int n;

    b64 = BIO_new(BIO_f_base64());
    if (!b64) {
        return NULL;
    }

    bmem = BIO_new_mem_buf(input, strlen(input));
    if (!bmem) {
        BIO_free(b64);
        goto cleanup;
    }
    bmem = BIO_push(b64, bmem);
    BIO_set_flags(bmem, BIO_FLAGS_BASE64_NO_NL);

    res = malloc(outputlen);
    if (!res) {
        TPMLIB_LogError("Could not allocate %u bytes.\n", outputlen);
        goto cleanup;
    }

    n = BIO_read(bmem, res, outputlen);
    if (n <= 0) {
        free(res);
        res = NULL;
        goto cleanup;
    }

cleanup:
    BIO_free_all(bmem);

    return res;
#else
    ssize_t cbDec = RTBase64DecodedSize(input, NULL /*ppszEnd*/);
    if (cbDec > 0)
    {
        void *pvData = malloc(cbDec);
        if (pvData)
        {
            int rc = RTBase64Decode(input, pvData, cbDec, NULL /*pcbActual*/, NULL /*ppszEnd*/);
            if (RT_SUCCESS(rc))
                return (uint8_t *)pvData;

            free(pvData);
        }
    }

    return NULL;        
#endif
}
#endif

/*
 * Base64 decode the string starting at 'start' and the last
 * valid character may be a 'end'. The length of the decoded string
 * is returned in *length.
 */
static unsigned char *TPMLIB_Base64Decode(const char *start, const char *end,
                                          size_t *length)
{
    unsigned char *ret = NULL;
    char *input = NULL, *d;
    const char *s;
    char c;
    unsigned int numbase64chars = 0;

    if (end < start)
        return NULL;

    while (end > start && !is_base64ltr(*end))
        end--;

    end++;

    input = malloc(end - start + 1);
    if (!input) {
        TPMLIB_LogError("Could not allocate %u bytes.\n",
                        (unsigned int)(end - start + 1));
        return NULL;
    }

    /* copy from source string skipping '\n' and '\r' and using
       '=' to calculate the exact length */
    d = input;
    s = start;

    while (s < end) {
        c = *s;
        if (is_base64ltr(c)) {
            *d = c;
            d++;
            if (c != '=') {
                numbase64chars++;
            }
        } else if (c == 0) {
            break;
        }
        s++;
    }
    *d = 0;

    *length = (numbase64chars / 4) * 3;
    switch (numbase64chars % 4) {
    case 2:
    case 3:
        *length += (numbase64chars % 4) - 1;
        break;
    case 0:
        break;
    case 1:
        fprintf(stderr,"malformed base64\n");
        goto err_exit;
    break;
    }

#ifdef USE_FREEBL_CRYPTO_LIBRARY
    ret = (unsigned char *)PL_Base64Decode(input, 0, NULL);
#endif

#ifdef USE_OPENSSL_CRYPTO_LIBRARY
    ret = TPMLIB_OpenSSL_Base64Decode(input, *length);
#endif

err_exit:
    free(input);

    return ret;
}

static unsigned char *TPMLIB_GetPlaintext(const char *stream,
                                          const char *starttag,
                                          const char *endtag,
                                          size_t *length)
{
    char *start, *end;
    unsigned char *plaintext = NULL;

    start = strstr(stream, starttag);
    if (start) {
        start += strlen(starttag);
        while (isspace((int)*start))
            start++;
        end = strstr(start, endtag);
        if (end) {
            plaintext = TPMLIB_Base64Decode(start, --end, length);
        }
    }
    return plaintext;
}

TPM_RESULT TPMLIB_DecodeBlob(const char *buffer, enum TPMLIB_BlobType type,
                             unsigned char **result, size_t *result_len)
{
    TPM_RESULT res = TPM_SUCCESS;

    *result = TPMLIB_GetPlaintext(buffer,
                                  tags_and_indices[type].starttag,
                                  tags_and_indices[type].endtag,
                                  result_len);

    if (*result == NULL) {
        res = TPM_FAIL;
    }

    return res;
}

void TPMLIB_SetDebugFD(int fd)
{
    debug_fd = fd;
}

void TPMLIB_SetDebugLevel(unsigned level)
{
    debug_level = level;
}

TPM_RESULT TPMLIB_SetDebugPrefix(const char *prefix)
{
    free(debug_prefix);

    if (prefix) {
        debug_prefix = strdup(prefix);
        if (!debug_prefix)
            return TPM_FAIL;
    } else {
        debug_prefix = NULL;
    }

    return TPM_SUCCESS;
}

int TPMLIB_LogPrintf(const char *format, ...)
{
#ifndef VBOX
    unsigned level = debug_level, i;
    va_list args;
    char buffer[256];
    int n;

    if (!debug_fd || !debug_level)
        return -1;

    va_start(args, format);
    n = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (n < 0 || n >= (int)sizeof(buffer))
        return -1;

    level--;

    i = 0;
    while (1) {
        if (buffer[i] == 0)
            return -1;
        if (buffer[i] != ' ')
            break;
        if (i == level)
            return -1;
        i++;
    }

    if (debug_prefix)
        dprintf(debug_fd, "%s", debug_prefix);
    dprintf(debug_fd, "%s", buffer);

    return i;
#else
    return 0;
#endif
}

/*
 * TPMLIB_LogPrintfA: Printf to the logfd without indentation check
 *
 * @indent: how many spaces to indent; indent of ~0 forces logging
 *          with indent 0 even if not debug_level is set
 * @format: format to use for formatting the following parameters
 * @...: varargs
 */
void TPMLIB_LogPrintfA(unsigned int indent, const char *format, ...)
{
#ifndef VBOX
    va_list args;
    char spaces[20];
    int fd;

    if (indent != (unsigned int)~0) {
        if (!debug_fd || !debug_level)
           return;
        fd = debug_fd;
    } else {
        indent = 0;
        fd = (debug_fd >= 0) ? debug_fd : STDERR_FILENO;
    }

    if (indent) {
        if (indent > sizeof(spaces) - 1)
            indent = sizeof(spaces) - 1;
        memset(spaces, ' ', indent);
        spaces[indent] = 0;
        dprintf(fd, "%s", spaces);
    }

    va_start(args, format);
    vdprintf(fd, format, args);
    va_end(args);
#endif
}

/*
 * TPMLIB_LogArray: Display an array of data
 *
 * @indent: how many spaces to indent; indent of ~0 forces logging
 *          with indent 0 even if not debug_level is set
 * @data: the data to print
 * @datalen: length of the data
 */
void TPMLIB_LogArray(unsigned int indent, const unsigned char *data,
                     size_t datalen)
{
    char line[80];
    size_t i, o = 0;

    for (i = 0; i < datalen; i++) {
        snprintf(&line[o], sizeof(line) - o, "%02x ", data[i]);
        o += 3;
        if (o >= 16 * 3) {
            TPMLIB_LogPrintfA(indent, "%s\n", line);
            o = 0;
        }
    }
    if (o > 0) {
        TPMLIB_LogPrintfA(indent, "%s\n", line);
    }
}

void ClearCachedState(enum TPMLIB_StateType st)
{
    free(cached_blobs[st].buffer);
    cached_blobs[st].buffer = NULL;
    cached_blobs[st].buflen = 0;
}

void ClearAllCachedState(void)
{
    ClearCachedState(TPMLIB_STATE_VOLATILE);
    ClearCachedState(TPMLIB_STATE_PERMANENT);
    ClearCachedState(TPMLIB_STATE_SAVE_STATE);
}

/*
 * Set buffer for cached state; we allow setting an empty cached state
 * by the caller passing a NULL pointer for the buffer.
 */
void SetCachedState(enum TPMLIB_StateType st,
                    unsigned char *buffer, uint32_t buflen)
{
    free(cached_blobs[st].buffer);
    cached_blobs[st].buffer = buffer;
    cached_blobs[st].buflen = buffer ? buflen : BUFLEN_EMPTY_BUFFER;
}

void GetCachedState(enum TPMLIB_StateType st,
                    unsigned char **buffer, uint32_t *buflen,
                    bool *is_empty_buffer)
{
     /* caller owns blob now */
    *buffer = cached_blobs[st].buffer;
    *buflen = cached_blobs[st].buflen;
    *is_empty_buffer = (*buflen == BUFLEN_EMPTY_BUFFER);
    cached_blobs[st].buffer = NULL;
    cached_blobs[st].buflen = 0;
}

bool HasCachedState(enum TPMLIB_StateType st)
{
    return (cached_blobs[st].buffer != NULL || cached_blobs[st].buflen != 0);
}

TPM_RESULT CopyCachedState(enum TPMLIB_StateType st,
                           unsigned char **buffer, uint32_t *buflen,
                           bool *is_empty_buffer)
{
    TPM_RESULT ret = TPM_SUCCESS;

    /* buflen may indicate an empty buffer */
    *buflen = cached_blobs[st].buflen;
    *is_empty_buffer = (*buflen == BUFLEN_EMPTY_BUFFER);

    if (cached_blobs[st].buffer) {
        *buffer = malloc(*buflen);
        if (!*buffer) {
            TPMLIB_LogError("Could not allocate %u bytes.\n", *buflen);
            ret = TPM_SIZE;
        } else {
            memcpy(*buffer, cached_blobs[st].buffer, *buflen);
        }
    } else {
        *buffer = NULL;
    }

    return ret;
}

const char *TPMLIB_StateTypeToName(enum TPMLIB_StateType st)
{
    switch (st) {
    case TPMLIB_STATE_PERMANENT:
        return TPM_PERMANENT_ALL_NAME;
    case TPMLIB_STATE_VOLATILE:
        return TPM_VOLATILESTATE_NAME;
    case TPMLIB_STATE_SAVE_STATE:
        return TPM_SAVESTATE_NAME;
    }
    return NULL;
}

enum TPMLIB_StateType TPMLIB_NameToStateType(const char *name)
{
    if (!name)
        return 0;
    if (!strcmp(name, TPM_PERMANENT_ALL_NAME))
        return TPMLIB_STATE_PERMANENT;
    if (!strcmp(name, TPM_VOLATILESTATE_NAME))
        return TPMLIB_STATE_VOLATILE;
    if (!strcmp(name, TPM_SAVESTATE_NAME))
        return TPMLIB_STATE_SAVE_STATE;
    return 0;
}
