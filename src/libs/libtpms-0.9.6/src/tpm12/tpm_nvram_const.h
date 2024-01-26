/********************************************************************************/
/*                                                                              */
/*                              NVRAM Constants                                 */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_nvram_const.h $       */
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

#ifndef TPM_NVRAM_CONST_H
#define TPM_NVRAM_CONST_H

/*
  These are implementation specific constants
*/

/*
  TPMS_MAX defines the maximum number of TPM instances.
*/

#define TPMS_MAX        1

/*
  NVRAM storage directory path
*/


#ifdef TPM_NV_DISK
/* TPM_NV_DISK uses the TPM_PATH environment variable */
#endif

/* Defines the maximum size of the NV defined space, the NV indexes created by TPM_NV_DefineSpace.

   The PC Client requires 2048 bytes.  There is at least (currently) 6 bytes of overhead, a tag and
   a count.
*/

#ifndef TPM_MAX_NV_DEFINED_SIZE
#define TPM_MAX_NV_DEFINED_SIZE 2100
#endif

/* TPM_MAX_NV_SPACE defines the maximum NV space for non-volatile state.

   It does not include the area used for TPM_SaveState.

   See TPM_OWNER_EVICT_KEY_HANDLES, TPM_MIN_COUNTERS, TPM_NUM_FAMILY_TABLE_ENTRY_MIN,
   TPM_NUM_DELEGATE_TABLE_ENTRY_MIN, etc. and the platform specific requirements for NV defined
   space.
*/

#ifndef TPM_MAX_NV_SPACE 



#ifdef TPM_NV_DISK
#define TPM_MAX_NV_SPACE 100000	/* arbitrary value */
#endif

#endif /* TPM_MAX_NV_SPACE */

#ifndef TPM_MAX_NV_SPACE
#error "TPM_MAX_NV_SPACE is not defined"
#endif

/* TPM_MAX_SAVESTATE_SPACE defines the maximum NV space for TPM saved state.

   It is used by TPM_SaveState

   NOTE This macro is based on the maximum number of loaded keys and session.  For example, 3 loaded
   keys, 3 OSAP sessions, and 1 transport session consumes about 2500 bytes.

   See TPM_KEY_HANDLES, TPM_NUM_PCR, TPM_MIN_AUTH_SESSIONS, TPM_MIN_TRANS_SESSIONS,
   TPM_MIN_DAA_SESSIONS, TPM_MIN_SESSION_LIST, etc.
*/

#ifndef TPM_MAX_SAVESTATE_SPACE 



#ifdef TPM_NV_DISK
#define TPM_MAX_SAVESTATE_SPACE 100000	/* arbitrary value */
#endif

#endif	/* TPM_MAX_SAVESTATE_SPACE */

#ifndef TPM_MAX_SAVESTATE_SPACE
#error "TPM_MAX_SAVESTATE_SPACE is not defined"
#endif

/* TPM_MAX_VOLATILESTATE_SPACE defines the maximum NV space for TPM volatile state.

   It is used for applications that save and restore the entire TPM volatile is a non-standard way.
*/

#ifndef TPM_MAX_VOLATILESTATE_SPACE 


#ifdef TPM_NV_DISK
#define TPM_MAX_VOLATILESTATE_SPACE 524288	/* arbitrary value */
#endif

#endif /* TPM_MAX_VOLATILESTATE_SPACE */

#ifndef TPM_MAX_VOLATILESTATE_SPACE
#error "TPM_MAX_VOLATILESTATE_SPACE is not defined"
#endif

#endif
