/********************************************************************************/
/*										*/
/*			  Marshalling and unmarshalling of state		*/
/*			     Written by Stefan Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*										*/
/* (c) Copyright IBM Corporation 2017,2018.					*/
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

#include <stdlib.h>

#include "config.h"

#include "StateMarshal.h"
#include "Volatile.h"

#define TPM_HAVE_TPM2_DECLARATIONS
#include "tpm_library_intern.h"
#include "tpm_nvfilename.h"
#include "tpm_error.h"
#include "tpm_memory.h"

UINT16
VolatileSave(BYTE **buffer, INT32 *size)
{
    return VolatileState_Save(buffer, size);
}

TPM_RC
VolatileLoad(BOOL *restored)
{
    TPM_RC rc = TPM_RC_SUCCESS;

#ifdef TPM_LIBTPMS_CALLBACKS
    unsigned char *data = NULL;
    uint32_t length = 0;
    struct libtpms_callbacks *cbs = TPMLIB_GetCallbacks();
    TPM_RESULT ret = TPM_SUCCESS;
    bool is_empty_state;

    *restored = FALSE;

    /* try to get state blob set via TPMLIB_SetState() */
    GetCachedState(TPMLIB_STATE_VOLATILE, &data, &length, &is_empty_state);
    if (is_empty_state)
        return rc;

    if (!data && cbs->tpm_nvram_loaddata) {
        uint32_t tpm_number = 0;
        const char *name = TPM_VOLATILESTATE_NAME;

        ret = cbs->tpm_nvram_loaddata(&data, &length, tpm_number, name);
    }

    if (data && ret == TPM_SUCCESS) {
        unsigned char *p = data;
        rc = VolatileState_Load(&data, (INT32 *)&length);
        /*
         * if this failed, VolatileState_Load will have started
         * failure mode.
         */
        free(p);

        *restored = (rc == 0);
    }
#endif /* TPM_LIBTPMS_CALLBACKS */

    return rc;
}
