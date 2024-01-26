/********************************************************************************/
/*                                                                              */
/*                              TPM Platform I/O                                */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_platform.c $		*/
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
#include <string.h>

#include "tpm_debug.h"
#include "tpm_pcr.h"
#include "tpm_platform.h"

#ifdef TPM_LIBTPMS_CALLBACKS
#include "tpm_library_intern.h"
#endif

#ifndef TPM_IO_LOCALITY

/* TPM_IO_GetLocality() is platform specific code to set the localityModifier before an ordinal is
   processed.

   Place holder, to be modified for the platform.
*/

TPM_RESULT TPM_IO_GetLocality(TPM_MODIFIER_INDICATOR *localityModifier,
			      uint32_t tpm_number)
{
    TPM_RESULT  rc = 0;

#ifdef TPM_LIBTPMS_CALLBACKS
    struct libtpms_callbacks *cbs = TPMLIB_GetCallbacks();

    /* call user-provided function if available, otherwise execute
       default behavior */
    if (cbs->tpm_io_getlocality) {
        rc = cbs->tpm_io_getlocality(localityModifier, tpm_number);
        return rc;
    }
#else
    tpm_number = tpm_number;	/* to silence the compiler */
#endif

    if (rc == 0) {
        *localityModifier = 0;
        printf("  TPM_IO_GetLocality: localityModifier %u\n", *localityModifier);
        rc = TPM_LocalityModifier_CheckLegal(*localityModifier);
    }
    return rc;
}

#endif  /* TPM_IO_LOCALITY */

#ifndef TPM_IO_PHYSICAL_PRESENCE 

/* TPM_IO_GetPhysicalPresence() is platform specific code to get the hardware physicalPresence
   state.

   Place holder, to be modified for the platform.
*/

TPM_RESULT TPM_IO_GetPhysicalPresence(TPM_BOOL *physicalPresence,
				      uint32_t tpm_number)
{
    TPM_RESULT  rc = 0;
    
#ifdef TPM_LIBTPMS_CALLBACKS
    struct libtpms_callbacks *cbs = TPMLIB_GetCallbacks();

    /* call user-provided function if available, otherwise execute
       default behavior */
    if (cbs->tpm_io_getphysicalpresence) {
        rc = cbs->tpm_io_getphysicalpresence(physicalPresence, tpm_number);
        return rc;
    }
#else
    tpm_number = tpm_number;	/* to silence the compiler */
#endif
    *physicalPresence = FALSE;
    return rc;
}

#endif  /* TPM_IO_PHYSICAL_PRESENCE */

#ifndef TPM_IO_GPIO

/* TPM_IO_GPIO_Write() should write 'dataSize' bytes of 'data' to 'nvIndex' at the GPIO port.

   Place holder, to be modified for the platform.
*/

TPM_RESULT TPM_IO_GPIO_Write(TPM_NV_INDEX nvIndex,
                             uint32_t dataSize,
                             BYTE *data,
			     uint32_t tpm_number)
{
    TPM_RESULT  rc = 0;
    tpm_number = tpm_number;	/* to silence the compiler */

#if defined TPM_PCCLIENT                /* These values are from the PC Client specification */
    printf(" TPM_IO_GPIO_Write: nvIndex %08x\n", nvIndex);
    TPM_PrintAll(" TPM_IO_GPIO_Write: Stub", data, dataSize);
    /* #elif Add other platform specific values here */
#else                                   /* This is the default case for the main specification */
    nvIndex = nvIndex;          /* unused parameter, to quiet the compiler */
    dataSize = dataSize;
    data = data;
    printf("TPM_IO_GPIO_Write: Error (fatal), platform does not support GPIO\n");
    rc = TPM_FAIL;      /* Should never get here.  The invalid address be detected earlier */
#endif
    return rc;
}

/* TPM_IO_GPIO_Read()

   Place holder, to be modified for the platform.
*/

TPM_RESULT TPM_IO_GPIO_Read(TPM_NV_INDEX nvIndex,
                            uint32_t dataSize,
                            BYTE *data,
			    uint32_t tpm_number)
{
    TPM_RESULT  rc = 0;
    tpm_number = tpm_number;	/* to silence the compiler */

#if defined TPM_PCCLIENT                /* These values are from the PC Client specification */
    printf(" TPM_IO_GPIO_Read: nvIndex %08x\n", nvIndex);
    memset(data, 0, dataSize);
    /* #elif Add other platform specific values here */
#else                                   /* This is the default case for the main specification */
    nvIndex = nvIndex;;         /* unused parameter, to quiet the compiler */
    dataSize = dataSize;;
    data = data;
    printf("TPM_IO_GPIO_Read: Error (fatal), platform does not support GPIO\n");
    rc = TPM_FAIL;      /* Should never get here.  The invalid address be detected earlier */
#endif
    return rc;
}

#endif  /* TPM_IO_GPIO */

