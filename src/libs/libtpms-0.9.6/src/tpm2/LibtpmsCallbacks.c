/********************************************************************************/
/*										*/
/*				Libtpms Callbacks				*/
/*			     Written by Stefan Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*										*/
/* (c) Copyright IBM Corporation 2018.						*/
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

#include <stdint.h>
#include <string.h>

#include "Platform.h"
#include "LibtpmsCallbacks.h"
#include "NVMarshal.h"

#define TPM_HAVE_TPM2_DECLARATIONS
#include "tpm_library_intern.h"
#include "tpm_error.h"
#include "tpm_nvfilename.h"

int
libtpms_plat__NVEnable(void)
{
    unsigned char *data = NULL;
    uint32_t length = 0;
    struct libtpms_callbacks *cbs = TPMLIB_GetCallbacks();
    TPM_RC rc;
    bool is_empty_state;

    /* try to get state blob set via TPMLIB_SetState() */
    GetCachedState(TPMLIB_STATE_PERMANENT, &data, &length, &is_empty_state);
    if (is_empty_state) {
        memset(s_NV, 0, NV_MEMORY_SIZE);
        return 0;
    }

    if (data == NULL && cbs->tpm_nvram_loaddata) {
        uint32_t tpm_number = 0;
        const char *name = TPM_PERMANENT_ALL_NAME;
        TPM_RESULT ret;

        ret = cbs->tpm_nvram_loaddata(&data, &length, tpm_number, name);
        switch (ret) {
        case TPM_RETRY:
            if (!cbs->tpm_nvram_storedata) {
                return -1;
            }
            memset(s_NV, 0, NV_MEMORY_SIZE);
            return 0;

        case TPM_SUCCESS:
            /* got the data -- unmarshal them... */
            break;

        case TPM_FAIL:
        default:
            return -1;
        }
    }

    if (data) {
        unsigned char *buffer = data;
        INT32 size = length;

        rc = PERSISTENT_ALL_Unmarshal(&buffer, &size);
        free(data);
        if (rc != TPM_RC_SUCCESS)
            return -1;
         return 0;
    }
    return LIBTPMS_CALLBACK_FALLTHROUGH; /* -2 */
}

int
libtpms_plat__NVDisable(
		 void
		 )
{
    struct libtpms_callbacks *cbs = TPMLIB_GetCallbacks();

    if (cbs->tpm_nvram_loaddata)
        return 0;
    return LIBTPMS_CALLBACK_FALLTHROUGH; /* -2 */
}

int
libtpms_plat__IsNvAvailable(
		     void
		     )
{
    struct libtpms_callbacks *cbs = TPMLIB_GetCallbacks();

    if (cbs->tpm_nvram_loaddata &&
        cbs->tpm_nvram_storedata) {
        return 1;
    }
    return LIBTPMS_CALLBACK_FALLTHROUGH; /* -2 */
}

int
libtpms_plat__NvCommit(
		void
		)
{
    struct libtpms_callbacks *cbs = TPMLIB_GetCallbacks();

    if (cbs->tpm_nvram_storedata) {
        uint32_t tpm_number = 0;
        const char *name = TPM_PERMANENT_ALL_NAME;
        TPM_RESULT ret;
        BYTE *buf;
        uint32_t buflen;

        ret = TPM2_PersistentAllStore(&buf, &buflen);
        if (ret != TPM_SUCCESS)
            return ret;

        ret = cbs->tpm_nvram_storedata(buf, buflen,
                                       tpm_number, name);
        free(buf);
        if (ret == TPM_SUCCESS)
            return 0;

        return -1;
    }
    return LIBTPMS_CALLBACK_FALLTHROUGH; /* -2 */
}

int
libtpms_plat__PhysicalPresenceAsserted(
				BOOL *pp
				)
{
    struct libtpms_callbacks *cbs = TPMLIB_GetCallbacks();

    if (cbs->tpm_io_getphysicalpresence) {
        uint32_t tpm_number = 0;
        TPM_RESULT res;
        unsigned char mypp;

        res = cbs->tpm_io_getphysicalpresence(&mypp, tpm_number);
        if (res == TPM_SUCCESS) {
            *pp = mypp;
            return 0;
        }
    }
    return LIBTPMS_CALLBACK_FALLTHROUGH;
}
