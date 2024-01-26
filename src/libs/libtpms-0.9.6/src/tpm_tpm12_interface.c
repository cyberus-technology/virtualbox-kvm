/********************************************************************************/
/*										*/
/*			LibTPM TPM 1.2 call interface functions				*/
/*			     Written by Stefan Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*										*/
/* (c) Copyright IBM Corporation 2015.						*/
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

#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tpm_debug.h"
#include "tpm_error.h"
#include "tpm12/tpm_init.h"
#include "tpm_library_intern.h"
#include "tpm12/tpm_process.h"
#include "tpm12/tpm_startup.h"
#include "tpm12/tpm_global.h"
#include "tpm12/tpm_permanent.h"
#include "tpm_nvfile.h"

static TPM_RESULT TPM12_MainInit(void)
{
    return TPM_MainInit();
}

static void TPM12_Terminate(void)
{
    TPM_Global_Delete(tpm_instances[0]);
    free(tpm_instances[0]);
    tpm_instances[0] = NULL;
}

static TPM_RESULT TPM12_Process(unsigned char **respbuffer, uint32_t *resp_size,
                                uint32_t *respbufsize,
                                unsigned char *command, uint32_t command_size)
{
    *resp_size = 0;
    return TPM_ProcessA(respbuffer, resp_size, respbufsize,
                        command, command_size);
}

static TPM_RESULT TPM12_VolatileAllStore(unsigned char **buffer,
                                         uint32_t *buflen)
{
    TPM_RESULT rc;
    TPM_STORE_BUFFER tsb;
    TPM_Sbuffer_Init(&tsb);
    uint32_t total;

#ifdef TPM_DEBUG
    assert(tpm_instances[0] != NULL);
#endif

    rc = TPM_VolatileAll_Store(&tsb, tpm_instances[0]);

    if (rc == TPM_SUCCESS) {
        /* caller now owns the buffer and needs to free it */
        TPM_Sbuffer_GetAll(&tsb, buffer, buflen, &total);
    } else {
        TPM_Sbuffer_Delete(&tsb);
        *buflen = 0;
        *buffer = NULL;
    }

    return rc;
}

static TPM_RESULT TPM12_CancelCommand(void)
{
    return TPM_FAIL; /* not supported */
}


static TPM_RESULT TPM12_GetTPMProperty(enum TPMLIB_TPMProperty prop,
                                int *result)
{
    switch (prop) {
    case  TPMPROP_TPM_RSA_KEY_LENGTH_MAX:
        *result = TPM_RSA_KEY_LENGTH_MAX;
        break;

    case  TPMPROP_TPM_KEY_HANDLES:
        *result = TPM_KEY_HANDLES;
        break;

    case  TPMPROP_TPM_OWNER_EVICT_KEY_HANDLES:
        *result = TPM_OWNER_EVICT_KEY_HANDLES;
        break;

    case  TPMPROP_TPM_MIN_AUTH_SESSIONS:
        *result = TPM_MIN_AUTH_SESSIONS;
        break;

    case  TPMPROP_TPM_MIN_TRANS_SESSIONS:
        *result = TPM_MIN_TRANS_SESSIONS;
        break;

    case  TPMPROP_TPM_MIN_DAA_SESSIONS:
        *result = TPM_MIN_DAA_SESSIONS;
        break;

    case  TPMPROP_TPM_MIN_SESSION_LIST:
        *result = TPM_MIN_SESSION_LIST;
        break;

    case  TPMPROP_TPM_MIN_COUNTERS:
        *result = TPM_MIN_COUNTERS;
        break;

    case  TPMPROP_TPM_NUM_FAMILY_TABLE_ENTRY_MIN:
        *result = TPM_NUM_FAMILY_TABLE_ENTRY_MIN;
        break;

    case  TPMPROP_TPM_NUM_DELEGATE_TABLE_ENTRY_MIN:
        *result = TPM_NUM_DELEGATE_TABLE_ENTRY_MIN;
        break;

    case  TPMPROP_TPM_SPACE_SAFETY_MARGIN:
        *result = TPM_SPACE_SAFETY_MARGIN;
        break;

    case  TPMPROP_TPM_MAX_NV_SPACE:
        /* fill up 20 kb.; this provides some safety margin (currently
           >4Kb) for possible future expansion of this blob */
        *result = ROUNDUP(TPM_MAX_NV_SPACE, 20 * 1024);
        break;

    case  TPMPROP_TPM_MAX_SAVESTATE_SPACE:
        *result = TPM_MAX_SAVESTATE_SPACE;
        break;

    case  TPMPROP_TPM_MAX_VOLATILESTATE_SPACE:
        *result = TPM_MAX_VOLATILESTATE_SPACE;
        break;

    default:
        return TPM_FAIL;
    }

    return TPM_SUCCESS;
}

/*
 * TPM12_GetInfo:
 *
 * @flags: logical or of flags that query for information
 *
 * Return a JSON document with contents queried for by the user's passed flags
 */
static char *TPM12_GetInfo(enum TPMLIB_InfoFlags flags)
{
    const char *tpmspec =
    "\"TPMSpecification\":{"
        "\"family\":\"1.2\","
        "\"level\":2,"
        "\"revision\":116"
    "}";
    const char *tpmattrs =
    "\"TPMAttributes\":{"
        "\"manufacturer\":\"id:00001014\","
        "\"version\":\"id:00740001\"," /* 146.1 */
        "\"model\":\"swtpm\""
    "}";
    char *fmt = NULL, *buffer;
    bool printed = false;

    if (!(buffer = strdup("{%s%s%s}")))
        return NULL;

    if ((flags & TPMLIB_INFO_TPMSPECIFICATION)) {
        fmt = buffer;
        buffer = NULL;
        if (asprintf(&buffer, fmt, "", tpmspec, "%s%s%s") < 0)
            goto error;
        free(fmt);
        printed = true;
    }
    if ((flags & TPMLIB_INFO_TPMATTRIBUTES)) {
        fmt = buffer;
        buffer = NULL;
        if (asprintf(&buffer, fmt,  printed ? "," : "",
                     tpmattrs, "%s%s%s") < 0)
            goto error;
        free(fmt);
        printed = true;
    }

    /* nothing else to add */
    fmt = buffer;
    buffer = NULL;
    if (asprintf(&buffer, fmt, "", "", "") < 0)
        goto error;
    free(fmt);

    return buffer;

error:
    free(fmt);
    free(buffer);

    return NULL;
}

static uint32_t tpm12_buffersize = TPM_BUFFER_MAX;

static uint32_t TPM12_SetBufferSize(uint32_t wanted_size,
                                    uint32_t *min_size,
                                    uint32_t *max_size)
{
    if (min_size)
        *min_size = TPM_BUFFER_MIN;
    if (max_size)
        *max_size = TPM_BUFFER_MAX;

    if (wanted_size == 0)
        return tpm12_buffersize;

    if (wanted_size > TPM_BUFFER_MAX)
        wanted_size = TPM_BUFFER_MAX;
    else if (wanted_size < TPM_BUFFER_MIN)
        wanted_size = TPM_BUFFER_MIN;

    tpm12_buffersize = wanted_size;

    return tpm12_buffersize;
}

uint32_t TPM12_GetBufferSize(void)
{
    return TPM12_SetBufferSize(0, NULL, NULL);
}

static TPM_RESULT TPM12_ValidateState(enum TPMLIB_StateType st,
                                      unsigned int flags)
{
    TPM_RESULT ret = TPM_SUCCESS;
    tpm_state_t tpm_state;
    enum TPMLIB_StateType sts[] = {
        TPMLIB_STATE_PERMANENT,
        TPMLIB_STATE_VOLATILE,
        TPMLIB_STATE_SAVE_STATE,
        0,
    };
    enum TPMLIB_StateType c_st;
    unsigned i;

#ifdef TPM_LIBTPMS_CALLBACKS
    struct libtpms_callbacks *cbs = TPMLIB_GetCallbacks();

    if (cbs->tpm_nvram_init) {
        ret = cbs->tpm_nvram_init();
        if (ret != TPM_SUCCESS)
            return ret;
    }
#endif

    ret = TPM_Global_Init(&tpm_state);
    tpm_state.tpm_number = 0;

    if (ret == TPM_SUCCESS) {
        /* permanent state needs to be there and loaded first */
        ret = TPM_PermanentAll_NVLoad(&tpm_state);
    }

    for (i = 0; sts[i] && ret == TPM_SUCCESS; i++) {
        c_st = st & sts[i];

        /* 'cached' state is known to 'work', so skip it */
        if (!c_st || HasCachedState(c_st))
            continue;

        switch (c_st) {
        case TPMLIB_STATE_PERMANENT:
            break;
        case TPMLIB_STATE_VOLATILE:
            ret = TPM_VolatileAll_NVLoad(&tpm_state);
            break;
        case TPMLIB_STATE_SAVE_STATE:
            ret = TPM_SaveState_NVLoad(&tpm_state);
            break;
        }
    }

    TPM_Global_Delete(&tpm_state);

    return ret;
}

static TPM_RESULT _TPM_PermanentAll_Store(TPM_STORE_BUFFER *sbuffer,
                                          tpm_state_t *tpm_state)
{
    const unsigned char *buffer = NULL;
    uint32_t buflen;

    return TPM_PermanentAll_Store(sbuffer, &buffer, &buflen, tpm_state);
}

/*
 * TPM_PermanentAll_NVLoad_Preserve
 *
 * @tpm_state: The tpm_state to load the permanent state into
 *
 * Call TPM_PermanentAll_NVLoad and preserve any cached data that a call
 * to TPM_PermanentAll_NVLoad (TPM_NVRAM_LoadData) may otherwise consume
 * and remove if it was available.
 */
static TPM_RESULT TPM_PermanentAll_NVLoad_Preserve(tpm_state_t *tpm_state)
{
    TPM_RESULT ret;
    unsigned char *buffer = NULL;
    uint32_t buffer_len;
    bool is_empty_buffer;

    ret = CopyCachedState(TPMLIB_STATE_PERMANENT,
                          &buffer, &buffer_len, &is_empty_buffer);
    if (ret == TPM_SUCCESS) {
        ret = TPM_PermanentAll_NVLoad(tpm_state);

        /* restore a previous empty buffer or any valid buffer */
        if (is_empty_buffer || buffer != NULL)
            SetCachedState(TPMLIB_STATE_PERMANENT, buffer, buffer_len);
    }

    return ret;
}

/*
 * Get the state blob of the given type. If we TPM is not running, we
 * get the cached state blobs, if available, otherwise we try to read
 * it from files. In case the TPM is running, we get it from the running
 * TPM.
 */
static TPM_RESULT TPM12_GetState(enum TPMLIB_StateType st,
                                 unsigned char **buffer, uint32_t *buflen)
{
    TPM_RESULT ret = TPM_FAIL;
    TPM_STORE_BUFFER tsb;
    uint32_t total;

    /* TPM not running ? */
    if (tpm_instances[0] == NULL) {
        struct libtpms_callbacks *cbs = TPMLIB_GetCallbacks();
        bool is_empty_buffer;

        /* try cached blob before file */
        ret = CopyCachedState(st, buffer, buflen, &is_empty_buffer);
        if (ret != TPM_SUCCESS || *buffer != NULL || is_empty_buffer)
            return ret;

        if (cbs->tpm_nvram_init) {
            ret = cbs->tpm_nvram_init();
            if (ret != TPM_SUCCESS)
                return ret;

            ret = TPM_NVRAM_LoadData(buffer, buflen, 0,
                                     TPMLIB_StateTypeToName(st));
        } else {
            ret = TPM_FAIL;
        }
        return ret;
    }

    TPM_Sbuffer_Init(&tsb);

    switch (st) {
    case TPMLIB_STATE_PERMANENT:
        ret = _TPM_PermanentAll_Store(&tsb, tpm_instances[0]);
        break;
    case TPMLIB_STATE_VOLATILE:
        ret = TPM_VolatileAll_Store(&tsb, tpm_instances[0]);
        break;
    case TPMLIB_STATE_SAVE_STATE:
        ret = TPM_SaveState_Store(&tsb, tpm_instances[0]);
        break;
    }

    if (ret == TPM_SUCCESS) {
        /* caller now owns the buffer and needs to free it */
        TPM_Sbuffer_GetAll(&tsb, buffer, buflen, &total);
    } else {
        TPM_Sbuffer_Delete(&tsb);
        *buflen = 0;
        *buffer = NULL;
    }

    return ret;
}

/*
 * Set the state the TPM 1.2 will use upon next TPM_MainInit(). The TPM 1.2
 * must not have been started, yet, or it must have been terminated for this
 * function to set the state.
 *
 * @st: The TPMLIB_StateType describing the type of blob in the buffer
 * @buffer: pointer to the buffer containing the state blob; NULL pointer clears
 *          previous state
 * @buflen: length of the buffer
 */
static TPM_RESULT TPM12_SetState(enum TPMLIB_StateType st,
                                 const unsigned char *buffer, uint32_t buflen)
{
    TPM_RESULT ret = TPM_SUCCESS;
    unsigned char *stream = NULL, *orig_stream = NULL;
    uint32_t stream_size = buflen;
    tpm_state_t *tpm_state = NULL;

    if (buffer == NULL) {
        SetCachedState(st, NULL, 0);
        return TPM_SUCCESS;
    }

    if (tpm_instances[0])
        return TPM_INVALID_POSTINIT;

    if (ret == TPM_SUCCESS) {
        stream = malloc(buflen);
        if (!stream) {
            TPMLIB_LogError("Could not allocate %u bytes.\n", buflen);
            ret = TPM_SIZE;
        }
    }

    if (ret == TPM_SUCCESS) {
        orig_stream = stream;
        memcpy(stream, buffer, buflen);

        tpm_state = malloc(sizeof(tpm_state_t));
        if (!tpm_state) {
            TPMLIB_LogError("Could not allocated %zu bytes.\n",
                            sizeof(tpm_state_t));
            ret = TPM_SIZE;
        }
    }

    if (ret == TPM_SUCCESS) {
        ret = TPM_Global_Init(tpm_state);
    }

    /* test whether we can accept the blob */
    if (ret == TPM_SUCCESS) {
        tpm_state->tpm_number = 0;

        switch (st) {
        case TPMLIB_STATE_PERMANENT:
            ret = TPM_PermanentAll_Load(tpm_state, &stream, &stream_size);
            break;
        case TPMLIB_STATE_VOLATILE:
            /* permanent state needs to be there and loaded first */
            ret = TPM_PermanentAll_NVLoad_Preserve(tpm_state);
            if (ret == TPM_SUCCESS)
                ret = TPM_VolatileAll_Load(tpm_state, &stream, &stream_size);
            break;
        case TPMLIB_STATE_SAVE_STATE:
            ret = TPM_PermanentAll_NVLoad_Preserve(tpm_state);
            if (ret == TPM_SUCCESS)
                 ret = TPM_SaveState_Load(tpm_state, &stream, &stream_size);
            break;
        }
        if (ret)
            ClearAllCachedState();
    }

    /* cache the blob for the TPM_MainInit() to pick it up */
    if (ret == TPM_SUCCESS) {
        SetCachedState(st, orig_stream, buflen);
    } else {
        free(orig_stream);
    }

    TPM_Global_Delete(tpm_state);
    free(tpm_state);

    return ret;
}

const struct tpm_interface TPM12Interface = {
    .MainInit = TPM12_MainInit,
    .Terminate = TPM12_Terminate,
    .Process = TPM12_Process,
    .VolatileAllStore = TPM12_VolatileAllStore,
    .CancelCommand = TPM12_CancelCommand,
    .GetTPMProperty = TPM12_GetTPMProperty,
    .GetInfo = TPM12_GetInfo,
    .TpmEstablishedGet = TPM12_IO_TpmEstablished_Get,
    .TpmEstablishedReset = TPM12_IO_TpmEstablished_Reset,
    .HashStart = TPM12_IO_Hash_Start,
    .HashData = TPM12_IO_Hash_Data,
    .HashEnd = TPM12_IO_Hash_End,
    .SetBufferSize = TPM12_SetBufferSize,
    .ValidateState = TPM12_ValidateState,
    .SetState = TPM12_SetState,
    .GetState = TPM12_GetState,
};
