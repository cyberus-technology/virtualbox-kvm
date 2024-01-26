/********************************************************************************/
/*                                                                              */
/*                              Time Utilities                                  */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_time.c $              */
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

/* TPM_GetTimeOfDay() calls platform specific code to get the time in seconds and microseconds
 */

#include <stdio.h>

#include "tpm_debug.h"
#include "tpm_error.h"

#include "tpm_time.h"

/* TPM_GetTimeOfDay() gets the current time of day.

   Must return TPM_FAIL on error, so that the caller knows to shut down the TPM
*/

#ifndef VBOX
#ifdef TPM_POSIX

#include <sys/time.h>

TPM_RESULT TPM_GetTimeOfDay(uint32_t *tv_sec, uint32_t *tv_usec)
{
    TPM_RESULT          rc = 0;
    struct timeval      tval;
    int                 irc;
    
    irc = gettimeofday(&tval, NULL );   /* get the time */
    if (irc == 0) {
        *tv_sec = tval.tv_sec;
        *tv_usec = tval.tv_usec;
        printf(" TPM_GetTimeOfDay: %d sec %d usec\n",*tv_sec, *tv_usec);
    }
    else {
        printf("TPM_GetTimeOfDay: Error (fatal) getting time of day\n");
        rc = TPM_FAIL;
    }
    return rc;
}
#endif
#else
# include <iprt/time.h>

TPM_RESULT TPM_GetTimeOfDay(uint32_t *tv_sec, uint32_t *tv_usec)
{
    TPM_RESULT rc = 0;
    RTTIMESPEC Timespec;
    int64_t i64Us;

    RTTimeNow(&Timespec);
    i64Us = RTTimeSpecGetMicro(&Timespec);
    *tv_sec = (uint32_t)(i64Us / RT_US_1SEC);
    *tv_usec = (uint32_t)(i64Us % RT_US_1SEC);
    return rc;
}
#endif
