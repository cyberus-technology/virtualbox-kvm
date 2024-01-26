/********************************************************************************/
/*                                                                              */
/*                         TPM Debug Utilities                                  */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_debug.c $             */
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

#include "tpm_debug.h"
#undef printf

#if 0

int swallow_rc = 0;

int tpm_swallow_printf_args(const char *format, ...)
{
    format = format;	/* to silence compiler */
    return 0;
}

#else

void TPM_PrintFourLimit(const char *string,
                        const unsigned char *buff, size_t buflen)
{
    if (buff != NULL) {
        switch (buflen) {
        case 0:
            TPMLIB_LogPrintf("%s (no data)\n", string);
            break;
        case 1:
            TPMLIB_LogPrintf("%s %02x\n",
                   string,
                   buff[0]);
            break;
        case 2:
            TPMLIB_LogPrintf("%s %02x %02x\n",
                   string,
                   buff[0],
                   buff[1]);
            break;
        case 3:
            TPMLIB_LogPrintf("%s %02x %02x %02x\n",
                   string,
                   buff[0],
                   buff[1],
                   buff[2]);
            break;
        default:
            TPMLIB_LogPrintf("%s %02x %02x %02x %02x\n",
                   string,
                   buff[0],
                   buff[1],
                   buff[2],
                   buff[3]);
        }
    }
    else {
        TPMLIB_LogPrintf("%s null\n", string);
    }
    return;
}

/* TPM_PrintFour() prints a prefix plus 4 bytes of a buffer */

void TPM_PrintFour(const char *string, const unsigned char* buff)
{
    TPM_PrintFourLimit(string, buff, 4);
}

#endif

/* TPM_PrintAll() prints 'string', the length, and then the entire byte array
 */

void TPM_PrintAll(const char *string, const unsigned char* buff, uint32_t length)
{
    uint32_t i;
    int indent;

    if (buff != NULL) {
        indent = TPMLIB_LogPrintf("%s length %u\n", string, length);
        if (indent < 0)
            return;

        for (i = 0 ; i < length ; i++) {
            if (i && !( i % 16 ))
                TPMLIB_LogPrintfA(0, "\n");

            if (!(i % 16))
                TPMLIB_LogPrintf(" %.2X ", buff[i]);
            else
                TPMLIB_LogPrintfA(0, "%.2X ", buff[i]);
        }
        TPMLIB_LogPrintfA(0, "\n");
    } else {
        TPMLIB_LogPrintf("%s null\n", string);
    }
    return;
}
