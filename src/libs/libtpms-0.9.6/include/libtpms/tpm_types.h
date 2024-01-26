/********************************************************************************/
/*                                                                              */
/*                              TPM Types                                       */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_types.h $             */
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

#ifndef TPM_TYPES_H
#define TPM_TYPES_H

#include <stdint.h>

#if defined (TPM_POSIX) || defined (TPM_SYSTEM_P)
#include <netinet/in.h>         /* for byte order conversions */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 2.2.1 Basic data types rev 87 */
#if !defined(TPM_HAVE_TPM2_DECLARATIONS)
typedef unsigned char  BYTE;            /* Basic byte used to transmit all character fields.  */
#endif
typedef unsigned char  TPM_BOOL;        /* TRUE/FALSE field. TRUE = 0x01, FALSE = 0x00 Use TPM_BOOL
                                           because MS VC++ defines BOOL on Windows */

/* 2.2.2 Boolean types rev 107 */

#undef TRUE
#define TRUE   0x01  /* Assertion   */
#undef FALSE
#define FALSE  0x00  /* Contradiction   */

/* 2.2.3 Helper redefinitions rev 101

   The following definitions are to make the definitions more explicit and easier to read.

   NOTE: They cannot be changed without breaking the serialization.
*/

typedef BYTE  TPM_AUTH_DATA_USAGE;      /* Indicates the conditions where it is required that
                                           authorization be presented.  */
typedef BYTE  TPM_PAYLOAD_TYPE;         /* The information as to what the payload is in an encrypted
                                           structure */
typedef BYTE  TPM_VERSION_BYTE;         /* The version info breakdown */
typedef BYTE  TPM_DA_STATE;             /* The state of the dictionary attack mitigation logic */

/* added kgold */
typedef BYTE  TPM_ENT_TYPE;             /* LSB of TPM_ENTITY_TYPE */
typedef BYTE  TPM_ADIP_ENC_SCHEME;      /* MSB of TPM_ENTITY_TYPE */

typedef uint16_t  TPM_PROTOCOL_ID;	/* The protocol in use.  */
typedef uint16_t  TPM_STARTUP_TYPE;	/* Indicates the start state.  */
typedef uint16_t  TPM_ENC_SCHEME;	/* The definition of the encryption scheme. */
typedef uint16_t  TPM_SIG_SCHEME;	/* The definition of the signature scheme. */
typedef uint16_t  TPM_MIGRATE_SCHEME;	/* The definition of the migration scheme */
typedef uint16_t  TPM_PHYSICAL_PRESENCE; /* Sets the state of the physical presence mechanism. */
typedef uint16_t  TPM_ENTITY_TYPE;	/* Indicates the types of entity that are supported by the
                                           TPM. */
typedef uint16_t  TPM_KEY_USAGE;	/* Indicates the permitted usage of the key.  */
typedef uint16_t  TPM_EK_TYPE;		/* The type of asymmetric encrypted structure in use by the
                                           endorsement key  */
typedef uint16_t  TPM_STRUCTURE_TAG;	/* The tag for the structure */
typedef uint16_t  TPM_PLATFORM_SPECIFIC; /* The platform specific spec to which the information
                                           relates to */
typedef uint32_t  TPM_COMMAND_CODE;	/* The command ordinal. */
typedef uint32_t  TPM_CAPABILITY_AREA;	/* Identifies a TPM capability area. */
typedef uint32_t  TPM_KEY_FLAGS;	/* Indicates information regarding a key. */
#if !defined(TPM_HAVE_TPM2_DECLARATIONS)
typedef uint32_t  TPM_ALGORITHM_ID;	/* Indicates the type of algorithm.  */
typedef uint32_t  TPM_MODIFIER_INDICATOR; /* The locality modifier  */
#endif
typedef uint32_t  TPM_ACTUAL_COUNT;	/* The actual number of a counter.  */
typedef uint32_t  TPM_TRANSPORT_ATTRIBUTES;	/* Attributes that define what options are in use
                                                   for a transport session */
typedef uint32_t  TPM_AUTHHANDLE;	/* Handle to an authorization session  */
typedef uint32_t  TPM_DIRINDEX;		/* Index to a DIR register  */
typedef uint32_t  TPM_KEY_HANDLE;	/* The area where a key is held assigned by the TPM.  */
typedef uint32_t  TPM_PCRINDEX;		/* Index to a PCR register  */
typedef uint32_t  TPM_RESULT;		/* The return code from a function  */
typedef uint32_t  TPM_RESOURCE_TYPE;	/* The types of resources that a TPM may have using internal
                                           resources */
typedef uint32_t  TPM_KEY_CONTROL;	/* Allows for controlling of the key when loaded and how to
                                           handle TPM_Startup issues  */
#if !defined(TPM_HAVE_TPM2_DECLARATIONS)
typedef uint32_t  TPM_NV_INDEX;		/* The index into the NV storage area  */
#endif
typedef uint32_t  TPM_FAMILY_ID;	/* The family ID. Families ID's are automatically assigned a
                                           sequence number by the TPM. A trusted process can set the
                                           FamilyID value in an individual row to zero, which
                                           invalidates that row. The family ID resets to zero on
                                           each change of TPM Owner.  */
typedef uint32_t  TPM_FAMILY_VERIFICATION;	/* A value used as a label for the most recent
						   verification of this family. Set to zero when not
						   in use.  */
typedef uint32_t  TPM_STARTUP_EFFECTS;	/* How the TPM handles var  */
typedef uint32_t  TPM_SYM_MODE;		/* The mode of a symmetric encryption  */
typedef uint32_t  TPM_FAMILY_FLAGS;	/* The family flags  */
typedef uint32_t  TPM_DELEGATE_INDEX;	/* The index value for the delegate NV table  */
typedef uint32_t  TPM_CMK_DELEGATE;	/* The restrictions placed on delegation of CMK
                                           commands */
typedef uint32_t  TPM_COUNT_ID;		/* The ID value of a monotonic counter  */
typedef uint32_t  TPM_REDIT_COMMAND;	/* A command to execute  */
typedef uint32_t  TPM_TRANSHANDLE;	/* A transport session handle  */
#if !defined(TPM_HAVE_TPM2_DECLARATIONS)
typedef uint32_t  TPM_HANDLE;		/* A generic handle could be key, transport etc.  */
#endif
typedef uint32_t  TPM_FAMILY_OPERATION;	/* What operation is happening  */

/* Not in specification */

typedef uint16_t  TPM_TAG;		/* The command and response tags */

typedef unsigned char *	TPM_SYMMETRIC_KEY_TOKEN;	/* abstract symmetric key token */
typedef unsigned char *	TPM_BIGNUM;			/* abstract bignum */

#ifdef __cplusplus
}
#endif

#endif
