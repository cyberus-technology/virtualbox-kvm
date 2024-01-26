/********************************************************************************/
/*                                                                              */
/*                         TPM Debug Utilities                                  */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_debug.h $             */
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

#ifndef TPM_DEBUG_H
#define TPM_DEBUG_H

#include "tpm_types.h"
#include "tpm_library_intern.h"

/* prototypes */

void TPM_PrintFour(const char *string, const unsigned char* buff);
void TPM_PrintFourLimit(const char *string,
                        const unsigned char* buff, size_t bufflen);
void TPM_PrintAll(const char *string, const unsigned char* buff, uint32_t length);

#if 0
#ifndef TPM_DEBUG       /* if debug is turned off */

/* dummy function to match the printf prototype */
int tpm_swallow_printf_args(const char *format, ...);

/* assign to this dummy value to eliminate "statement has no effect" warnings */
extern int swallow_rc;

/* redefine printf to null */
#define printf swallow_rc = swallow_rc && tpm_swallow_printf_args
#define TPM_PrintFour(arg1, arg2)

#endif  /* TPM_DEBUG */
#endif

#ifdef printf
# undef  printf
#endif
#define printf(...) TPMLIB_LogPrintf(__VA_ARGS__);

#endif
