/********************************************************************************/
/*										*/
/*			LibTPM compile-time choices (#defines)			*/
/*                        Written by Stefan Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_library_conf.h $		*/
/*										*/
/* (c) Copyright IBM Corporation 2010.						*/
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
#ifndef TPM_LIBRARY_CONF_H
#define TPM_LIBRARY_CONF_H

/* Note: None of these defines should be used directly; rather,
 *       the TPMLIB_GetTPMProperty() call should be used to
 *       query their value.
 *       Since tpm_constants.h defines some default values if none
 *       other are defined, this header should be included before
 *       tpm_constants.h is included.
 */

/* need to restrict the maximum size of keys to cap the below blobs */
#define TPM_RSA_KEY_LENGTH_MAX     2048

/* maximum size of the IO buffer used for requests and responses */
#define TPM_BUFFER_MAX             4096

/*
 * Below the following acronyms are used to identify what
 * #define influences which one of the state blobs the TPM
 * produces.
 *
 * PA : permanentall
 * SS : savestate
 * VA : volatileall
 *
 * BAL: contributes to the ballooning of the state blob
 */

/*
 * Do not touch these #define's anymore. They are fixed forever
 * and define the properties of the TPM library and have a
 * direct influence on the size requirements of the TPM's block
 * store and the organization of data inside that block store.
 */
/*
 * Every 2048 bit key in volatile space accounts for an
 * increase of maximum of 559 bytes (PCR_INFO_LONG, tied to PCRs).
 */
#define TPM_KEY_HANDLES                  20            /* SS, VA,  BAL */

/*
 * Every 2048 bit key on which the owner evict key flag is set
 * accounts for an increase of 559 bytes of the permanentall
 * blob.
 */
#define TPM_OWNER_EVICT_KEY_HANDLES      10            /* PA, BAL */

/*
 * The largest auth session is DSAP; each such session consumes 119 bytes
 */
#define TPM_MIN_AUTH_SESSIONS            16            /* SS, VA, BAL */

/*
 * Every transport session accounts for an increase of 78 bytes
 */
#define TPM_MIN_TRANS_SESSIONS           16            /* SS, VA, BAL */
/*
 * Every DAA session accounts for an increase of 844 bytes.
 */
#define TPM_MIN_DAA_SESSIONS              2            /* SS, VA, BAL */

#define TPM_MIN_SESSION_LIST            128            /* SS, VA */
#define TPM_MIN_COUNTERS                  8            /* PA */
#define TPM_NUM_FAMILY_TABLE_ENTRY_MIN   16            /* PA */
#define TPM_NUM_DELEGATE_TABLE_ENTRY_MIN  4            /* PA */

/*
 * NB: above #defines directly influence the largest size of the
 * 'permanentall', 'savestate' and 'volatileall' data. If these
 * #define's allow the below space requirements to be exceeded, the
 * TPM may go into shutdown mode, something we would definitely
 * like to prevent. We are mostly concerned about the size of
 * the 'permanentall' blob, which is capped by TPM_MAX_NV_SPACE,
 * and that of the 'savestate' blob, which is capped by
 * TPM_MAX_SAVESTATE_SPACE.
 */

#define TPM_SPACE_SAFETY_MARGIN      (4 * 1024)

/*
 * As of V0.5.1 (may have increased since then):
 *     permanent space + 10 keys = 7920  bytes
 * full volatile space           = 17223 bytes
 * full savestate space          = 16992 bytes
 */

/*
 * For the TPM_MAX_NV_SPACE we cannot provide a safety margin here
 * since the TPM will allow NVRAM spaces to allocate everything.
 * So, we tell the user in TPMLIB_GetTPMProperty that it's 20kb. This
 * gives us some safety margin for the future.
 */
#define TPM_PERMANENT_ALL_BASE_SIZE  (2334 /* incl. SRK, EK */ + \
                                      2048 /* extra space */)

#define TPM_MAX_NV_DEFINED_SIZE      (2048    /* min.  NVRAM spaces */ + \
                                      26*1024 /* extra NVRAM space */ )

#define TPM_MAX_NV_SPACE             (TPM_PERMANENT_ALL_BASE_SIZE +       \
                                      TPM_OWNER_EVICT_KEY_HANDLES * 559 + \
                                      TPM_MAX_NV_DEFINED_SIZE)

#define TPM_MAX_SAVESTATE_SPACE      (972 + /* base size */         \
                                      TPM_KEY_HANDLES * 559 +       \
                                      TPM_MIN_TRANS_SESSIONS * 78 + \
                                      TPM_MIN_DAA_SESSIONS * 844 +  \
                                      TPM_MIN_AUTH_SESSIONS * 119 + \
                                      TPM_SPACE_SAFETY_MARGIN)

#define TPM_MAX_VOLATILESTATE_SPACE  (1203  + /* base size */       \
                                      TPM_KEY_HANDLES * 559 +       \
                                      TPM_MIN_TRANS_SESSIONS * 78 + \
                                      TPM_MIN_DAA_SESSIONS * 844 +  \
                                      TPM_MIN_AUTH_SESSIONS * 119 + \
                                      TPM_SPACE_SAFETY_MARGIN)

/*
 * The timeouts in microseconds.
 *
 * The problem with the timeouts is that on a heavily utilized
 * virtualized platform, the processing of the TPM's commands will
 * take much longer than on a system that's not very busy. So, we
 * now choose values that are very high so that we don't hit timeouts
 * in TPM drivers just because the system is busy. However, hitting
 * timeouts on a very busy system may be inevitable...
 */

#define TPM_SMALL_DURATION    ( 50 * 1000 * 1000)
#define TPM_MEDIUM_DURATION   (100 * 1000 * 1000)
#define TPM_LONG_DURATION     (300 * 1000 * 1000)


#ifdef VBOX
# if defined(RT_OS_WINDOWS)
#  include <iprt/string.h>

#  define __attribute__(unused)

DECLINLINE(int) asprintf(char **ret, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int rc = RTStrAPrintfV(ret, format, args);
    va_end(args);
    return rc;
}

#  if defined(TPM_V12)
#   include <iprt/asm.h>
#   include <iprt/cdefs.h>

#   define htonl(a_Val) RT_H2BE_U32(a_Val)
#   define htons(a_Val) RT_H2BE_U16(a_Val)
#   define ntohs(a_Val) RT_BE2H_U16(a_Val)
#  endif
# endif
#endif
#endif /* TPM_LIBRARY_CONF_H */
