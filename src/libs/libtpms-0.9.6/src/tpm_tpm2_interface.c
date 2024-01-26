/********************************************************************************/
/*										*/
/*			LibTPM TPM 2 call interface functions				*/
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef LIB_EXPORT
#define LIB_EXPORT
#endif
#include "tpm2/Tpm.h"
#include "tpm2/Manufacture_fp.h"
#include "tpm2/Platform_fp.h"
#include "tpm2/ExecCommand_fp.h"
#include "tpm2/TpmTcpProtocol.h"
#include "tpm2/Simulator_fp.h"
#include "tpm2/_TPM_Hash_Data_fp.h"
#include "tpm2/_TPM_Init_fp.h"
#include "tpm2/StateMarshal.h"
#include "tpm2/PlatformACT.h"
#include "tpm2/PlatformData.h"
#include "tpm2/Volatile.h"
#include "tpm2/crypto/openssl/ExpDCache_fp.h"

#define TPM_HAVE_TPM2_DECLARATIONS
#include "tpm_nvfile.h" // TPM_NVRAM_Loaddata()
#include "tpm_error.h"
#include "tpm_library_intern.h"
#include "tpm_nvfilename.h"

extern BOOL      g_inFailureMode;
static BOOL      reportedFailureCommand;

/*
 * Check whether the main NVRAM file exists. Return TRUE if it doesn, FALSE otherwise
 */
static TPM_BOOL _TPM2_CheckNVRAMFileExists(bool *has_nvram_loaddata_callback)
{
#ifdef TPM_LIBTPMS_CALLBACKS
    struct libtpms_callbacks *cbs = TPMLIB_GetCallbacks();
    const char *name = TPM_PERMANENT_ALL_NAME;
    unsigned char *data = NULL;
    uint32_t length = 0;
    uint32_t tpm_number = 0;
    TPM_RESULT ret;

    *has_nvram_loaddata_callback = cbs->tpm_nvram_loaddata != NULL;
    if (cbs->tpm_nvram_loaddata) {
        ret = cbs->tpm_nvram_loaddata(&data, &length, tpm_number, name);
        free(data);
        /* a file exists once NOT TPM_RETRY is returned */
        if (ret != TPM_RETRY)
            return TRUE;
    }
#else
    *has_nvram_loaddata_callback = FALSE;
#endif /* TPM_LIBTPMS_CALLBACKS */

    return FALSE;
}

static TPM_RESULT TPM2_MainInit(void)
{
    TPM_RESULT ret = TPM_SUCCESS;
    bool has_cached_state;
    bool has_nvram_file;
    bool has_nvram_loaddata_callback;

    g_inFailureMode = FALSE;
    reportedFailureCommand = FALSE;

#ifdef TPM_LIBTPMS_CALLBACKS
    struct libtpms_callbacks *cbs = TPMLIB_GetCallbacks();

    if (cbs->tpm_io_init) {
        ret = cbs->tpm_io_init();
        if (ret != TPM_SUCCESS)
            return ret;
    }

    if (cbs->tpm_nvram_init) {
        ret = cbs->tpm_nvram_init();
        if (ret != TPM_SUCCESS)
            return ret;
    }
#endif /* TPM_LIBTPMS_CALLBACKS */

    _rpc__Signal_PowerOff();

    has_cached_state = HasCachedState(TPMLIB_STATE_PERMANENT);
    has_nvram_file = _TPM2_CheckNVRAMFileExists(&has_nvram_loaddata_callback);

    if (!has_cached_state) {
        if (!has_nvram_file) {
            ret = _plat__NVEnable(NULL);
            if (ret)
                TPMLIB_LogTPM2Error(
                    "%s: _plat__NVEnable(NULL) failed: %d\n",
                    __func__, ret);
            if (TPM_Manufacture(TRUE) < 0 || g_inFailureMode) {
                TPMLIB_LogTPM2Error("%s: TPM_Manufacture(TRUE) failed or TPM in "
                                    "failure mode\n", __func__);
                reportedFailureCommand = TRUE;
            }
        }
    } else if (!has_nvram_loaddata_callback) {
        ret = _plat__NVEnable_NVChipFile(NULL);
        if (ret)
            TPMLIB_LogTPM2Error("%s: _plat__NVEnable_File(NULL) failed: %d\n",
                                __func__, ret);
    }

    _rpc__Signal_PowerOn(FALSE);

    _rpc__Signal_NvOn();

    if (ret == TPM_SUCCESS) {
        if (g_inFailureMode)
            ret = TPM_RC_FAILURE;
    }

    if (ret == TPM_SUCCESS && has_cached_state) {
        NvCommit();
    }

    return ret;
}

static void TPM2_Terminate(void)
{
    TPM_TearDown();

    _rpc__Signal_PowerOff();
    ExpDCacheFree();
}

static TPM_RESULT TPM2_Process(unsigned char **respbuffer, uint32_t *resp_size,
                               uint32_t *respbufsize,
                               unsigned char *command, uint32_t command_size)
{
    uint8_t locality = 0;
    _IN_BUFFER req;
    _OUT_BUFFER resp;
    unsigned char *tmp;

#ifdef TPM_LIBTPMS_CALLBACKS
    struct libtpms_callbacks *cbs = TPMLIB_GetCallbacks();

    if (cbs->tpm_io_getlocality) {
        TPM_MODIFIER_INDICATOR locty;

        locality = cbs->tpm_io_getlocality(&locty, 0);

        locality = locty;
    }
#endif /* TPM_LIBTPMS_CALLBACKS */

    req.BufferSize = command_size;
    req.Buffer = command;

    /* have the TPM 2 write directly into the response buffer */
    if (*respbufsize < TPM_BUFFER_MAX || !*respbuffer) {
        tmp = realloc(*respbuffer, TPM_BUFFER_MAX);
        if (!tmp) {
            TPMLIB_LogTPM2Error("Could not allocated %u bytes.\n",
                                TPM_BUFFER_MAX);
            return TPM_SIZE;
        }
        *respbuffer = tmp;
        *respbufsize = TPM_BUFFER_MAX;
    }
    resp.BufferSize = *respbufsize;
    resp.Buffer = *respbuffer;

    /*
     * signals for cancellation have to come after we start processing
     */
    _rpc__Signal_CancelOff();

    _rpc__Send_Command(locality, req, &resp);

    /* it may come back with a different buffer, especially in failure mode */
    if (resp.Buffer != *respbuffer) {
        if (resp.BufferSize > *respbufsize)
            resp.BufferSize = *respbufsize;
        memcpy(*respbuffer, resp.Buffer, resp.BufferSize);
    }

    *resp_size = resp.BufferSize;

    if (g_inFailureMode && !reportedFailureCommand) {
        reportedFailureCommand = TRUE;
        TPMLIB_LogTPM2Error("%s: Entered failure mode through command:\n",
                            __func__);
        TPMLIB_LogArray(~0, command, command_size);
    }

    return TPM_SUCCESS;
}

TPM_RESULT TPM2_PersistentAllStore(unsigned char **buf,
                                   uint32_t *buflen)
{
    BYTE *buffer;
    INT32 size;
    unsigned char *nbuffer;
    TPM_RESULT ret = TPM_SUCCESS;
    UINT32 written = 0;

    *buflen = NV_MEMORY_SIZE + 32 * 1024;
    *buf = NULL;

    /* the marshal functions do not indicate insufficient
       buffer; to make sure we didn't run out of buffer,
       we check that enough room for the biggest type of
       chunk (64k) is available and try again. */
    do {
        *buflen += 66 * 1024;

        nbuffer = realloc(*buf, *buflen);
        if (nbuffer == NULL) {
            free(*buf);
            *buf = NULL;
            ret = TPM_SIZE;
            written = 0;
            break;
        }

        *buf = buffer = nbuffer;
        size = *buflen;
        written = PERSISTENT_ALL_Marshal(&buffer, &size);
    } while (size < 66 * 1024);

    *buflen = written;

    return ret;
}

static TPM_RESULT TPM2_VolatileAllStore(unsigned char **buffer,
                                        uint32_t *buflen)
{
    TPM_RESULT rc = 0;
    INT32 size = NV_MEMORY_SIZE;
    UINT16 written;
    unsigned char *statebuffer = NULL;

    *buffer = NULL;
    statebuffer = malloc(size);
    if (!statebuffer) {
        TPMLIB_LogTPM2Error("Could not allocate %u bytes.\n", size);
        return TPM_SIZE;
    }

    /* statebuffer will change */
    *buffer = statebuffer;

    written = VolatileSave(&statebuffer, &size);
    if (written >= size) {
        free(*buffer);
        *buffer = NULL;
        rc = TPM_FAIL;
    } else {
        *buflen = written;
    }

    return rc;
}

static TPM_RESULT TPM2_CancelCommand(void)
{
    _rpc__Signal_CancelOn();

    return TPM_SUCCESS;
}

static TPM_RESULT TPM2_GetTPMProperty(enum TPMLIB_TPMProperty prop,
                                      int *result)
{
    switch (prop) {
    case  TPMPROP_TPM_RSA_KEY_LENGTH_MAX:
        *result = MAX_RSA_KEY_BITS;
        break;

    case  TPMPROP_TPM_KEY_HANDLES:
        *result = MAX_HANDLE_NUM;
        break;

    /* not supported for TPM 2 */
    case  TPMPROP_TPM_OWNER_EVICT_KEY_HANDLES:
    case  TPMPROP_TPM_MIN_AUTH_SESSIONS:
    case  TPMPROP_TPM_MIN_TRANS_SESSIONS:
    case  TPMPROP_TPM_MIN_DAA_SESSIONS:
    case  TPMPROP_TPM_MIN_SESSION_LIST:
    case  TPMPROP_TPM_MIN_COUNTERS:
    case  TPMPROP_TPM_NUM_FAMILY_TABLE_ENTRY_MIN:
    case  TPMPROP_TPM_NUM_DELEGATE_TABLE_ENTRY_MIN:
    case  TPMPROP_TPM_SPACE_SAFETY_MARGIN:
    case  TPMPROP_TPM_MAX_NV_SPACE:
    case  TPMPROP_TPM_MAX_SAVESTATE_SPACE:
    case  TPMPROP_TPM_MAX_VOLATILESTATE_SPACE:

    default:
        return TPM_FAIL;
    }

    return TPM_SUCCESS;
}

/*
 * TPM2_GetInfo:
 *
 * @flags: logical or of flags that query for information
 *
 * Return a JSON document with contents queried for by the user's passed flags
 */
static char *TPM2_GetInfo(enum TPMLIB_InfoFlags flags)
{
    const char *tpmspec =
    "\"TPMSpecification\":{"
        "\"family\":\"2.0\","
        "\"level\":" STRINGIFY(SPEC_LEVEL_NUM) ","
        "\"revision\":" STRINGIFY(SPEC_VERSION)
    "}";
    const char *tpmattrs_temp =
    "\"TPMAttributes\":{"
        "\"manufacturer\":\"id:00001014\","
        "\"version\":\"id:%08X\","
        "\"model\":\"swtpm\""
    "}";
    const char *tpmfeatures_temp =
    "\"TPMFeatures\":{"
        "\"RSAKeySizes\":[%s],"
        "\"CamelliaKeySizes\":[%s]"
    "}";
    char *fmt = NULL, *buffer;
    bool printed = false;
    char *tpmattrs = NULL;
    char *tpmfeatures = NULL;
    char rsakeys[32];
    char camelliakeys[16];
    size_t n;

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
        if (asprintf(&tpmattrs, tpmattrs_temp, FIRMWARE_V1) < 0)
            goto error;
        if (asprintf(&buffer, fmt,  printed ? "," : "",
                     tpmattrs, "%s%s%s") < 0)
            goto error;
        free(fmt);
        printed = true;
    }

    if ((flags & TPMLIB_INFO_TPMFEATURES)) {
        fmt = buffer;
        buffer = NULL;
        n = snprintf(rsakeys, sizeof(rsakeys), "%s2048%s%s",
                     RSA_1024 ? "1024," : "",
                     RSA_3072 ? ",3072" : "",
                     RSA_4096 ? ",4096" : "");
        if (n >= sizeof(rsakeys))
            goto error;
        n = snprintf(camelliakeys, sizeof(camelliakeys), "%s%s%s",
                     CAMELLIA_128 ? "128" : "",
                     CAMELLIA_192 ? ",192" : "",
                     CAMELLIA_256 ? ",256" : "");
        if (n >= sizeof(camelliakeys))
            goto error;
        if (asprintf(&tpmfeatures, tpmfeatures_temp,
                     rsakeys, camelliakeys) < 0)
            goto error;
        if (asprintf(&buffer, fmt,  printed ? "," : "",
                     tpmfeatures, "%s%s%s") < 0)
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
    free(tpmattrs);
    free(tpmfeatures);

    return buffer;

error:
    free(fmt);
    free(buffer);
    free(tpmattrs);
    free(tpmfeatures);

    return NULL;
}

static uint32_t tpm2_buffersize = TPM_BUFFER_MAX;

static uint32_t TPM2_SetBufferSize(uint32_t wanted_size,
                                   uint32_t *min_size,
                                   uint32_t *max_size)
{
    const uint32_t min = MAX_CONTEXT_SIZE + 128;
    const uint32_t max = TPM_BUFFER_MAX;

    if (min_size)
        *min_size = min;
    if (max_size)
        *max_size = max;

    if (wanted_size == 0)
        return tpm2_buffersize;

    if (wanted_size > max)
        wanted_size = max;
    else if (wanted_size < min)
        wanted_size = min;

    tpm2_buffersize = wanted_size;

    return tpm2_buffersize;
}

uint32_t TPM2_GetBufferSize(void)
{
    return TPM2_SetBufferSize(0, NULL, NULL);
}

/*
 * Validate the state blobs to check whether they can be
 * successfully used by a TPM_INIT.
*/
static TPM_RESULT TPM2_ValidateState(enum TPMLIB_StateType st,
                                     unsigned int flags)
{
    TPM_RESULT ret = TPM_SUCCESS;
    TPM_RC rc = TPM_RC_SUCCESS;
    unsigned char *data = NULL;
    uint32_t length;
    unsigned char bak_NV[NV_MEMORY_SIZE];
    INT32 size;
    BYTE *buffer;
    BOOL restored;

    /* make backup of current NvChip memory */
    memcpy(bak_NV, s_NV, sizeof(bak_NV));

#ifdef TPM_LIBTPMS_CALLBACKS
    struct libtpms_callbacks *cbs = TPMLIB_GetCallbacks();

    if (cbs->tpm_nvram_init) {
        ret = cbs->tpm_nvram_init();
        if (ret != TPM_SUCCESS)
            return ret;
    }

#endif

    if ((rc == TPM_RC_SUCCESS) &&
        (st & (TPMLIB_STATE_PERMANENT | TPMLIB_STATE_SAVE_STATE))) {

#ifdef TPM_LIBTPMS_CALLBACKS
        if (cbs->tpm_nvram_loaddata) {
            ret = cbs->tpm_nvram_loaddata(&data, &length, 0,
                                          TPM_PERMANENT_ALL_NAME);
            if (ret != TPM_SUCCESS)
                return ret;
        }
#endif

        if (!data)
            return TPM_FAIL;

        buffer = data;
        size = length;
        rc = PERSISTENT_ALL_Unmarshal(&buffer, &size);
        free(data);
    }

    if ((rc == TPM_RC_SUCCESS) &&
        (st & TPMLIB_STATE_VOLATILE)) {
        rc = VolatileLoad(&restored);
    }

    ret = rc;

    return ret;
}

/*
 * Get the state blob of the given type. If the TPM is not running, we
 * get the cached state blobs, if available, otherwise we try to read
 * it from files. In case the TPM is running, we get it from the running
 * TPM.
 */
static TPM_RESULT TPM2_GetState(enum TPMLIB_StateType st,
                                unsigned char **buffer, uint32_t *buflen)
{
    TPM_RESULT ret = TPM_FAIL;

    if (!_rpc__Signal_IsPowerOn()) {
        struct libtpms_callbacks *cbs = TPMLIB_GetCallbacks();
        bool is_empty_buffer;

        ret = CopyCachedState(st, buffer, buflen, &is_empty_buffer);
        if (ret != TPM_SUCCESS || *buffer != NULL || is_empty_buffer)
            return ret;

        if (cbs->tpm_nvram_init) {
            ret = cbs->tpm_nvram_init();
            if (ret != TPM_SUCCESS)
                return ret;

            /* we can call the TPM 1.2 function here ... */
            ret = TPM_NVRAM_LoadData(buffer, buflen, 0,
                                     TPMLIB_StateTypeToName(st));
        } else {
            ret = TPM_FAIL;
        }
        return ret;
    }

    /* from the running TPM */
    switch (st) {
    case TPMLIB_STATE_PERMANENT:
        ret = TPM2_PersistentAllStore(buffer, buflen);
        break;
    case TPMLIB_STATE_VOLATILE:
        ret = TPM2_VolatileAllStore(buffer, buflen);
        break;
    case TPMLIB_STATE_SAVE_STATE:
        *buffer = NULL;
        *buflen = 0;
        ret = 0;
        break;
    }

    return ret;
}

/*
 * Set the state the TPM 2 will use upon next TPM_MainInit(). The TPM 2
 * must not have been started, yet, or it must have been terminated for this
 * function to set the state.
 *
 * @st: The TPMLIB_StateType describing the type of blob in the buffer
 * @buffer: pointer to the buffer containing the state blob; NULL pointer clears
 *          previous state
 * @buflen: length of the buffer
 */
static TPM_RESULT TPM2_SetState(enum TPMLIB_StateType st,
                                const unsigned char *buffer, uint32_t buflen)
{
    TPM_RESULT ret = TPM_SUCCESS;
    TPM_RC rc = TPM_RC_SUCCESS;
    BYTE *stream = NULL, *orig_stream = NULL;
    INT32 stream_size = buflen;
    unsigned char *permanent = NULL, *ptr;
    INT32 permanent_len;

    if (buffer == NULL) {
        SetCachedState(st, NULL, 0);
        return TPM_SUCCESS;
    }

    if (_rpc__Signal_IsPowerOn())
        return TPM_INVALID_POSTINIT;

    if (ret == TPM_SUCCESS) {
        stream = malloc(buflen);
        if (!stream)
            ret = TPM_SIZE;
    }

    if (ret == TPM_SUCCESS) {
        orig_stream = stream;
        memcpy(stream, buffer, buflen);
    }

    /* test whether we can accept the blob */
    if (ret == TPM_SUCCESS) {
        switch (st) {
        case TPMLIB_STATE_PERMANENT:
            rc = PERSISTENT_ALL_Unmarshal(&stream, &stream_size);
            break;
        case TPMLIB_STATE_VOLATILE:
            /* load permanent state first */
            rc = TPM2_GetState(TPMLIB_STATE_PERMANENT,
                               &permanent, (uint32_t *)&permanent_len);
            if (rc == TPM_RC_SUCCESS) {
                ptr = permanent;
                rc = PERSISTENT_ALL_Unmarshal(&ptr, &permanent_len);
                if (rc == TPM_RC_SUCCESS)
                    rc = VolatileState_Load(&stream, &stream_size);
            }
            break;
        case TPMLIB_STATE_SAVE_STATE:
            if (buffer != NULL)
                rc = TPM_BAD_TYPE;
            break;
        }
        ret = rc;
        if (ret != TPM_SUCCESS)
            ClearAllCachedState();
    }

    /* cache the blob for the TPM_MainInit() to pick it up */
    if (ret == TPM_SUCCESS) {
        SetCachedState(st, orig_stream, buflen);
    } else {
        free(orig_stream);
    }
    free(permanent);

    return ret;
}

const struct tpm_interface TPM2Interface = {
    .MainInit = TPM2_MainInit,
    .Terminate = TPM2_Terminate,
    .Process = TPM2_Process,
    .VolatileAllStore = TPM2_VolatileAllStore,
    .CancelCommand = TPM2_CancelCommand,
    .GetTPMProperty = TPM2_GetTPMProperty,
    .GetInfo = TPM2_GetInfo,
    .TpmEstablishedGet = TPM2_IO_TpmEstablished_Get,
    .TpmEstablishedReset = TPM2_IO_TpmEstablished_Reset,
    .HashStart = TPM2_IO_Hash_Start,
    .HashData = TPM2_IO_Hash_Data,
    .HashEnd = TPM2_IO_Hash_End,
    .SetBufferSize = TPM2_SetBufferSize,
    .ValidateState = TPM2_ValidateState,
    .SetState = TPM2_SetState,
    .GetState = TPM2_GetState,
};
