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

#ifndef VBOX
#if defined __FreeBSD__ || defined __DragonFly__
# include <sys/endian.h>
#elif defined __APPLE__
# include <libkern/OSByteOrder.h>
#else
# include <endian.h>
#endif
#endif
#include <string.h>

#include "config.h"

#include "assert.h"
#include "NVMarshal.h"
#include "Volatile.h"

#define TPM_HAVE_TPM2_DECLARATIONS
#include "tpm_library_intern.h"

TPM_RC
VolatileState_Load(BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    BYTE hash[SHA1_DIGEST_SIZE], acthash[SHA1_DIGEST_SIZE];
    UINT16 hashAlg = TPM_ALG_SHA1;

    if (rc == TPM_RC_SUCCESS) {
        if ((UINT32)*size < sizeof(hash))
            return TPM_RC_INSUFFICIENT;

        CryptHashBlock(hashAlg, *size - sizeof(hash), *buffer,
                       sizeof(acthash), acthash);
        rc = VolatileState_Unmarshal(buffer, size);
        /* specific error has already been reported */
    }

    if (rc == TPM_RC_SUCCESS) {
        /*
         * advance pointer towards hash if we have a later version of
         * the state that has extra data we didn't read
         */
        if (*size > 0 && (UINT32)*size > sizeof(hash)) {
            *buffer += *size - sizeof(hash);
            *size = sizeof(hash);
        }
        rc = Array_Unmarshal(hash, sizeof(hash), buffer, size);
        if (rc != TPM_RC_SUCCESS)
            TPMLIB_LogTPM2Error("Error unmarshalling volatile state hash: "
                                "0x%02x\n", rc);
    }

    if (rc == TPM_RC_SUCCESS) {
        if (memcmp(acthash, hash, sizeof(hash))) {
            rc = TPM_RC_HASH;
            TPMLIB_LogTPM2Error("Volatile state checksum error: 0x%02x\n",
                                rc);
        }
    }

    if (rc != TPM_RC_SUCCESS)
        g_inFailureMode = TRUE;

    return rc;
}

UINT16
VolatileState_Save(BYTE **buffer, INT32 *size)
{
    UINT16 written;
    const BYTE *start;
    BYTE hash[SHA1_DIGEST_SIZE];
    TPM_ALG_ID hashAlg = TPM_ALG_SHA1;

    start = *buffer;
    written = VolatileState_Marshal(buffer, size);

    /* append the checksum */
    CryptHashBlock(hashAlg, written, start, sizeof(hash), hash);
    written += Array_Marshal(hash, sizeof(hash), buffer, size);

    return written;
}
