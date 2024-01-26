/********************************************************************************/
/*                                                                              */
/*                              TPM Constants                                   */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_constants.h $         */
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

#ifndef TPM_CONSTANTS_H
#define TPM_CONSTANTS_H

#include <stdint.h>

#include <tpm_library_intern.h>

/*
  NOTE implementation Specific
*/

/*
  version, revision, specLevel, errataRev
*/

/* current for released specification revision 103 */

#define TPM_REVISION_MAX 9999
#ifndef TPM_REVISION
#define TPM_REVISION TPM_REVISION_MAX
#endif

#if  (TPM_REVISION >= 116) 

#define TPM_SPEC_LEVEL  0x0002          /* uint16_t The level of ordinals supported */
#define TPM_ERRATA_REV  0x03            /* specification errata level */

#elif  (TPM_REVISION >= 103) 

#define TPM_SPEC_LEVEL  0x0002          /* uint16_t The level of ordinals supported */
#define TPM_ERRATA_REV  0x02            /* specification errata level */

#elif (TPM_REVISION >= 94)

#define TPM_SPEC_LEVEL  0x0002          /* uint16_t The level of ordinals supported */
#define TPM_ERRATA_REV  0x01            /* specification errata level */

#elif (TPM_REVISION >= 85)

#define TPM_SPEC_LEVEL  0x0002          /* uint16_t The level of ordinals supported */
#define TPM_ERRATA_REV  0x00            /* specification errata level */

#else

#define TPM_SPEC_LEVEL  0x0001          /* uint16_t The level of ordinals supported */
#define TPM_ERRATA_REV  0x00            /* specification errata level */

#endif

/* IBM specific */

#if 0   /* at one time vendorID was the PCI vendor ID, this is the IBM code */
#define TPM_VENDOR_ID   "\x00\x00\x10\x14"      /* BYTE[4], the vendor ID, obtained from the TCG,
                                                   typically PCI vendor ID */
#endif



#define TPM_VENDOR_ID    "IBM"  /* 4 bytes, as of rev 99 vendorID and TPM_CAP_PROP_MANUFACTURER
                                   return the same value */
#define TPM_MANUFACTURER "IBM"  /* 4 characters, assigned by TCG, typically stock ticker symbol */


/* Timeouts in microseconds.  These are for the platform specific interface (e.g. the LPC bus
   registers in the PC Client TPM).  They are most likely not applicable to a software TPM.  */
#define TPM_TIMEOUT_A   1000000
#define TPM_TIMEOUT_B   1000000
#define TPM_TIMEOUT_C   1000000
#define TPM_TIMEOUT_D   1000000

/* dictionary attack mitigation */

#define TPM_LOCKOUT_THRESHOLD 5         /* successive failures to trigger lockout, must be greater
                                           than 0 */

/* Denotes the duration value in microseconds of the duration of the three classes of commands:
   Small, Medium and Long.  The command types are in the Part 2 Ordinal Table.  Essentially:

   Long - creating an RSA key pair
   Medium - using an RSA key
   Short  - anything else
*/

#ifndef TPM_SMALL_DURATION
#define TPM_SMALL_DURATION      2000000
#endif

#ifndef TPM_MEDIUM_DURATION     
#define TPM_MEDIUM_DURATION     5000000
#endif

#ifndef TPM_LONG_DURATION
#define TPM_LONG_DURATION      60000000
#endif

/* startup effects */
   
#define    TPM_STARTUP_EFFECTS_VALUE   \
(TPM_STARTUP_EFFECTS_ST_ANY_RT_KEY |    /* key resources init by TPM_Startup(ST_ANY) */ \
 TPM_STARTUP_EFFECTS_ST_STATE_RT_HASH | /* hash resources are init by TPM_Startup(ST_STATE) */ \
 TPM_STARTUP_EFFECTS_ST_CLEAR_AUDITDIGEST) /* auditDigest nulled on TPM_Startup(ST_CLEAR) */

/*
  TPM buffer limits
*/

/* This is the increment by which the TPM_STORE_BUFFER grows.  A larger number saves realloc's.  A
   smaller number saves memory.

   TPM_ALLOC_MAX must be a multiple of this value.
*/

#define TPM_STORE_BUFFER_INCREMENT (TPM_ALLOC_MAX / 64)

/* This is the maximum value of the TPM input and output packet buffer.  It should be large enough
   to accommodate the largest TPM command or response, currently about 1200 bytes.  It should be
   small enough to accommodate whatever software is driving the TPM.

   NOTE: Some commands are somewhat open ended, and related to this parmater.  E.g., The input size
   for the TPM_SHA1Init.  The output size for TPM_GetRandom.
  
   It is returned by TPM_GetCapability -> TPM_CAP_PROP_INPUT_BUFFER
*/

#ifndef TPM_BUFFER_MAX
#define TPM_BUFFER_MAX  0x1000  /* 4k bytes */
#endif

#ifndef TPM_BUFFER_MIN
#define TPM_BUFFER_MIN  0x0c00  /* 3k bytes */
#endif

/* Random number generator */

/* maximum bytes in one TPM_GetRandom() call

   Use maximum input buffer size minus tag, paramSize, returnCode, randomBytesSize.
*/

#define TPM_RANDOM_MAX  (TPM12_GetBufferSize() \
                         - sizeof(TPM_TAG) - sizeof(uint32_t) \
			 - sizeof(TPM_RESULT) - sizeof(uint32_t))

/* Maximum number of bytes that can be sent to TPM_SHA1Update. Must be a multiple of 64 bytes.

   Use maximum input buffer size minus tag, paramSize, ordinal, numBytes.
*/

#define TPM_SHA1_MAXNUMBYTES    (TPM12_GetBufferSize() - 64)

/* extra audit status bits for TSC commands outside the normal ordinal range */
#define TSC_PHYS_PRES_AUDIT     0x01
#define TSC_RESET_ESTAB_AUDIT   0x02


/* TPM_CAP_MFR capabilities */
#define TPM_CAP_PROCESS_ID              0x00000020


/* define a value for an illegal instance handle */

#define TPM_ILLEGAL_INSTANCE_HANDLE     0xffffffff

/*
  NOTE End Implementation Specific
*/

/* 3. Structure Tags rev 105

   There have been some indications that knowing what structure is in use would be valuable
   information in each structure. This new tag will be in each new structure that the TPM defines.
   
   The upper nibble of the value designates the purview of the structure tag.  0 is used for TPM
   structures, 1 for platforms, and 2-F are reserved.
*/

/* 3.1 TPM_STRUCTURE_TAG */

/*                                              Structure   */
#define TPM_TAG_CONTEXTBLOB             0x0001 /*  TPM_CONTEXT_BLOB */
#define TPM_TAG_CONTEXT_SENSITIVE       0x0002 /*  TPM_CONTEXT_SENSITIVE */
#define TPM_TAG_CONTEXTPOINTER          0x0003 /*  TPM_CONTEXT_POINTER */
#define TPM_TAG_CONTEXTLIST             0x0004 /*  TPM_CONTEXT_LIST */
#define TPM_TAG_SIGNINFO                0x0005 /*  TPM_SIGN_INFO */
#define TPM_TAG_PCR_INFO_LONG           0x0006 /*  TPM_PCR_INFO_LONG */
#define TPM_TAG_PERSISTENT_FLAGS        0x0007 /*  TPM_PERSISTENT_FLAGS (deprecated 1.1 struct) */
#define TPM_TAG_VOLATILE_FLAGS          0x0008 /*  TPM_VOLATILE_FLAGS (deprecated 1.1 struct) */
#define TPM_TAG_PERSISTENT_DATA         0x0009 /*  TPM_PERSISTENT_DATA (deprecated 1.1 struct) */
#define TPM_TAG_VOLATILE_DATA           0x000A /*  TPM_VOLATILE_DATA (deprecated 1.1 struct) */
#define TPM_TAG_SV_DATA                 0x000B /*  TPM_SV_DATA */
#define TPM_TAG_EK_BLOB                 0x000C /*  TPM_EK_BLOB */
#define TPM_TAG_EK_BLOB_AUTH            0x000D /*  TPM_EK_BLOB_AUTH */
#define TPM_TAG_COUNTER_VALUE           0x000E /*  TPM_COUNTER_VALUE */
#define TPM_TAG_TRANSPORT_INTERNAL      0x000F /*  TPM_TRANSPORT_INTERNAL */
#define TPM_TAG_TRANSPORT_LOG_IN        0x0010 /*  TPM_TRANSPORT_LOG_IN */
#define TPM_TAG_TRANSPORT_LOG_OUT       0x0011 /*  TPM_TRANSPORT_LOG_OUT */
#define TPM_TAG_AUDIT_EVENT_IN          0x0012 /*  TPM_AUDIT_EVENT_IN */
#define TPM_TAG_AUDIT_EVENT_OUT         0X0013 /*  TPM_AUDIT_EVENT_OUT */
#define TPM_TAG_CURRENT_TICKS           0x0014 /*  TPM_CURRENT_TICKS */
#define TPM_TAG_KEY                     0x0015 /*  TPM_KEY */
#define TPM_TAG_STORED_DATA12           0x0016 /*  TPM_STORED_DATA12 */
#define TPM_TAG_NV_ATTRIBUTES           0x0017 /*  TPM_NV_ATTRIBUTES */
#define TPM_TAG_NV_DATA_PUBLIC          0x0018 /*  TPM_NV_DATA_PUBLIC */
#define TPM_TAG_NV_DATA_SENSITIVE       0x0019 /*  TPM_NV_DATA_SENSITIVE */
#define TPM_TAG_DELEGATIONS             0x001A /*  TPM DELEGATIONS */
#define TPM_TAG_DELEGATE_PUBLIC         0x001B /*  TPM_DELEGATE_PUBLIC */
#define TPM_TAG_DELEGATE_TABLE_ROW      0x001C /*  TPM_DELEGATE_TABLE_ROW */
#define TPM_TAG_TRANSPORT_AUTH          0x001D /*  TPM_TRANSPORT_AUTH */
#define TPM_TAG_TRANSPORT_PUBLIC        0X001E /*  TPM_TRANSPORT_PUBLIC */
#define TPM_TAG_PERMANENT_FLAGS         0X001F /*  TPM_PERMANENT_FLAGS */
#define TPM_TAG_STCLEAR_FLAGS           0X0020 /*  TPM_STCLEAR_FLAGS */
#define TPM_TAG_STANY_FLAGS             0X0021 /*  TPM_STANY_FLAGS */
#define TPM_TAG_PERMANENT_DATA          0X0022 /*  TPM_PERMANENT_DATA */
#define TPM_TAG_STCLEAR_DATA            0X0023 /*  TPM_STCLEAR_DATA */
#define TPM_TAG_STANY_DATA              0X0024 /*  TPM_STANY_DATA */
#define TPM_TAG_FAMILY_TABLE_ENTRY      0X0025 /*  TPM_FAMILY_TABLE_ENTRY */
#define TPM_TAG_DELEGATE_SENSITIVE      0X0026 /*  TPM_DELEGATE_SENSITIVE */
#define TPM_TAG_DELG_KEY_BLOB           0X0027 /*  TPM_DELG_KEY_BLOB */
#define TPM_TAG_KEY12                   0x0028 /*  TPM_KEY12 */
#define TPM_TAG_CERTIFY_INFO2           0X0029 /*  TPM_CERTIFY_INFO2 */
#define TPM_TAG_DELEGATE_OWNER_BLOB     0X002A /*  TPM_DELEGATE_OWNER_BLOB */
#define TPM_TAG_EK_BLOB_ACTIVATE        0X002B /*  TPM_EK_BLOB_ACTIVATE */
#define TPM_TAG_DAA_BLOB                0X002C /*  TPM_DAA_BLOB */
#define TPM_TAG_DAA_CONTEXT             0X002D /*  TPM_DAA_CONTEXT */
#define TPM_TAG_DAA_ENFORCE             0X002E /*  TPM_DAA_ENFORCE */
#define TPM_TAG_DAA_ISSUER              0X002F /*  TPM_DAA_ISSUER */
#define TPM_TAG_CAP_VERSION_INFO        0X0030 /*  TPM_CAP_VERSION_INFO */
#define TPM_TAG_DAA_SENSITIVE           0X0031 /*  TPM_DAA_SENSITIVE */
#define TPM_TAG_DAA_TPM                 0X0032 /*  TPM_DAA_TPM */
#define TPM_TAG_CMK_MIGAUTH             0X0033 /*  TPM_CMK_MIGAUTH */
#define TPM_TAG_CMK_SIGTICKET           0X0034 /*  TPM_CMK_SIGTICKET */
#define TPM_TAG_CMK_MA_APPROVAL         0X0035 /*  TPM_CMK_MA_APPROVAL */
#define TPM_TAG_QUOTE_INFO2             0X0036 /*  TPM_QUOTE_INFO2 */
#define TPM_TAG_DA_INFO                 0x0037 /*  TPM_DA_INFO */
#define TPM_TAG_DA_INFO_LIMITED         0x0038 /*  TPM_DA_INFO_LIMITED */
#define TPM_TAG_DA_ACTION_TYPE          0x0039 /*  TPM_DA_ACTION_TYPE */

/*
  SW TPM Tags
*/

/*
  These tags are used to describe the format of serialized TPM non-volatile state
*/

/* These describe the overall format */

/* V1 state is the sequence permanent data, permanent flags, owner evict keys, NV defined space */

#define TPM_TAG_NVSTATE_V1		0x0001		/* svn revision 4078 */

/* These tags describe the TPM_PERMANENT_DATA format */

/* For the first release, use the standard TPM_TAG_PERMANENT_DATA tag.  Since this tag is never
   visible outside the TPM, the tag value can be changed if the format changes.
*/

/* These tags describe the TPM_PERMANENT_FLAGS format */

/* The TPM_PERMANENT_FLAGS structure changed from rev 94 to 103.  Unfortunately, the standard TPM
   tag did not change.  Define distinguishing values here.
*/

#define TPM_TAG_NVSTATE_PF94		0x0001
#define TPM_TAG_NVSTATE_PF103		0x0002

/* This tag describes the owner evict key format */

#define TPM_TAG_NVSTATE_OE_V1		0x0001

/* This tag describes the NV defined space format */

#define TPM_TAG_NVSTATE_NV_V1		0x0001

/* V2 added the NV public optimization */

#define TPM_TAG_NVSTATE_NV_V2		0x0002

/*
  These tags are used to describe the format of serialized TPM volatile state
*/

/* These describe the overall format */

/* V1 state is the sequence TPM Parameters, TPM_STCLEAR_FLAGS, TPM_STANY_FLAGS, TPM_STCLEAR_DATA,
   TPM_STANY_DATA, TPM_KEY_HANDLE_ENTRY, SHA1 context(s), TPM_TRANSHANDLE, testState, NV volatile
   flags */

#define TPM_TAG_VSTATE_V1		0x0001

/* This tag defines the TPM Parameters format */

#define TPM_TAG_TPM_PARAMETERS_V1	0x0001

/* This tag defines the TPM_STCLEAR_FLAGS format */

/* V1 is the TCG standard returned by the getcap.  It's unlikely that this will change */

#define TPM_TAG_STCLEAR_FLAGS_V1	0x0001

/* These tags describe the TPM_STANY_FLAGS format */

/* For the first release, use the standard TPM_TAG_STANY_FLAGS tag.  Since this tag is never visible
   outside the TPM, the tag value can be changed if the format changes.
*/

/* This tag defines the TPM_STCLEAR_DATA format */

/* V2 deleted the ordinalResponse, responseCount */ 

#define TPM_TAG_STCLEAR_DATA_V2         0X0024

/* These tags describe the TPM_STANY_DATA format */

/* For the first release, use the standard TPM_TAG_STANY_DATA tag.  Since this tag is never visible
   outside the TPM, the tag value can be changed if the format changes.
*/

/* This tag defines the key handle entries format */

#define TPM_TAG_KEY_HANDLE_ENTRIES_V1	0x0001

/* This tag defines the SHA-1 context format */

#define TPM_TAG_SHA1CONTEXT_OSSL_V1	0x0001		/* for openssl */

#define TPM_TAG_SHA1CONTEXT_FREEBL_V1	0x0101		/* for freebl */

/* This tag defines the NV index entries volatile format */

#define TPM_TAG_NV_INDEX_ENTRIES_VOLATILE_V1	0x0001

/* 4. Types
 */

/* 4.1 TPM_RESOURCE_TYPE rev 87 */

#define TPM_RT_KEY      0x00000001  /* The handle is a key handle and is the result of a LoadKey
                                       type operation */
   
#define TPM_RT_AUTH     0x00000002  /* The handle is an authorization handle. Auth handles come from
                                       TPM_OIAP, TPM_OSAP and TPM_DSAP */
   
#define TPM_RT_HASH     0X00000003  /* Reserved for hashes */

#define TPM_RT_TRANS    0x00000004  /* The handle is for a transport session. Transport handles come
                                       from TPM_EstablishTransport */
   
#define TPM_RT_CONTEXT  0x00000005  /* Resource wrapped and held outside the TPM using the context
                                       save/restore commands */

#define TPM_RT_COUNTER  0x00000006  /* Reserved for counters */

#define TPM_RT_DELEGATE 0x00000007  /* The handle is for a delegate row. These are the internal rows
                                       held in NV storage by the TPM */
   
#define TPM_RT_DAA_TPM  0x00000008  /* The value is a DAA TPM specific blob */
                                      
#define TPM_RT_DAA_V0   0x00000009  /* The value is a DAA V0 parameter */
                                     
#define TPM_RT_DAA_V1   0x0000000A  /* The value is a DAA V1 parameter */
                                     
/* 4.2 TPM_PAYLOAD_TYPE rev 87

   This structure specifies the type of payload in various messages. 
*/

#define TPM_PT_ASYM             0x01    /* The entity is an asymmetric key */
#define TPM_PT_BIND             0x02    /* The entity is bound data */
#define TPM_PT_MIGRATE          0x03    /* The entity is a migration blob */
#define TPM_PT_MAINT            0x04    /* The entity is a maintenance blob */
#define TPM_PT_SEAL             0x05    /* The entity is sealed data */
#define TPM_PT_MIGRATE_RESTRICTED 0x06  /* The entity is a restricted-migration asymmetric key */
#define TPM_PT_MIGRATE_EXTERNAL 0x07    /* The entity is a external migratable key */
#define TPM_PT_CMK_MIGRATE      0x08    /* The entity is a CMK migratable blob */
/* 0x09 - 0x7F Reserved for future use by TPM */
/* 0x80 - 0xFF Vendor specific payloads */

/* 4.3 TPM_ENTITY_TYPE rev 100

   This specifies the types of entity that are supported by the TPM. 

   The LSB is used to indicate the entity type.  The MSB is used to indicate the ADIP 
   encryption scheme when applicable.

   For compatibility with TPM 1.1, this mapping is maintained:

   0x0001 specifies a keyHandle entity with XOR encryption
   0x0002 specifies an owner entity with XOR encryption
   0x0003 specifies some data entity with XOR encryption
   0x0004 specifies the SRK entity with XOR encryption
   0x0005 specifies a key entity with XOR encryption

   When the entity is not being used for ADIP encryption, the MSB MUST be 0x00.
*/

/* TPM_ENTITY_TYPE LSB Values (entity type) */

#define TPM_ET_KEYHANDLE        0x01    /* The entity is a keyHandle or key */
#define TPM_ET_OWNER            0x02    /*0x40000001 The entity is the TPM Owner */
#define TPM_ET_DATA             0x03    /* The entity is some data */
#define TPM_ET_SRK              0x04    /*0x40000000 The entity is the SRK */
#define TPM_ET_KEY              0x05    /* The entity is a key or keyHandle */
#define TPM_ET_REVOKE           0x06    /*0x40000002 The entity is the RevokeTrust value */
#define TPM_ET_DEL_OWNER_BLOB   0x07    /* The entity is a delegate owner blob */
#define TPM_ET_DEL_ROW          0x08    /* The entity is a delegate row */
#define TPM_ET_DEL_KEY_BLOB     0x09    /* The entity is a delegate key blob */
#define TPM_ET_COUNTER          0x0A    /* The entity is a counter */
#define TPM_ET_NV               0x0B    /* The entity is a NV index */
#define TPM_ET_OPERATOR         0x0C    /* The entity is the operator */
#define TPM_ET_RESERVED_HANDLE  0x40    /* Reserved. This value avoids collisions with the handle
                                           MSB setting.*/

/* TPM_ENTITY_TYPE MSB Values (ADIP encryption scheme) */

#define TPM_ET_XOR              0x00    /* XOR  */
#define TPM_ET_AES128_CTR       0x06    /* AES 128 bits in CTR mode */

/* 4.4 Handles rev 88

   Handles provides pointers to TPM internal resources. Handles should provide the ability to locate
   a value without collision.

   1. The TPM MAY order and set a handle to any value the TPM determines is appropriate

   2. The handle value SHALL provide assurance that collisions SHOULD not occur in 2^24 handles

   4.4.1 Reserved Key Handles 

   The reserved key handles. These values specify specific keys or specific actions for the TPM. 
*/

/* 4.4.1 Reserved Key Handles rev 87

   The reserved key handles. These values specify specific keys or specific actions for the TPM.

   TPM_KH_TRANSPORT indicates to TPM_EstablishTransport that there is no encryption key, and that
   the "secret" wrapped parameters are actually passed unencrypted.
*/

#define TPM_KH_SRK              0x40000000 /* The handle points to the SRK */
#define TPM_KH_OWNER            0x40000001 /* The handle points to the TPM Owner */
#define TPM_KH_REVOKE           0x40000002 /* The handle points to the RevokeTrust value */
#define TPM_KH_TRANSPORT        0x40000003 /* The handle points to the TPM_EstablishTransport static
                                              authorization */
#define TPM_KH_OPERATOR         0x40000004 /* The handle points to the Operator auth */
#define TPM_KH_ADMIN            0x40000005 /* The handle points to the delegation administration
                                              auth */
#define TPM_KH_EK               0x40000006 /* The handle points to the PUBEK, only usable with
                                              TPM_OwnerReadInternalPub */

/* 4.5 TPM_STARTUP_TYPE rev 87

   To specify what type of startup is occurring.  
*/

#define TPM_ST_CLEAR            0x0001 /* The TPM is starting up from a clean state */
#define TPM_ST_STATE            0x0002 /* The TPM is starting up from a saved state */
#define TPM_ST_DEACTIVATED      0x0003 /* The TPM is to startup and set the deactivated flag to
                                          TRUE */

/* 4.6 TPM_STARTUP_EFFECTS rev 101

   This structure lists for the various resources and sessions on a TPM the affect that TPM_Startup
   has on the values.

   There are three ST_STATE options for keys (restore all, restore non-volatile, or restore none)
   and two ST_CLEAR options (restore non-volatile or restore none).  As bit 4 was insufficient to
   describe the possibilities, it is deprecated.  Software should use TPM_CAP_KEY_HANDLE to
   determine which keys are loaded after TPM_Startup.

   31-9 No information and MUST be FALSE
   
   8 TPM_RT_DAA_TPM resources are initialized by TPM_Startup(ST_STATE)
   7 TPM_Startup has no effect on auditDigest 
   6 auditDigest is set to all zeros on TPM_Startup(ST_CLEAR) but not on other types of TPM_Startup 
   5 auditDigest is set to all zeros on TPM_Startup(any)
   4 TPM_RT_KEY Deprecated, as the meaning was subject to interpretation.  (Was:TPM_RT_KEY resources
     are initialized by TPM_Startup(ST_ANY))
   3 TPM_RT_AUTH resources are initialized by TPM_Startup(ST_STATE) 
   2 TPM_RT_HASH resources are initialized by TPM_Startup(ST_STATE) 
   1 TPM_RT_TRANS resources are initialized by TPM_Startup(ST_STATE) 
   0 TPM_RT_CONTEXT session (but not key) resources are initialized by TPM_Startup(ST_STATE) 
*/


#define TPM_STARTUP_EFFECTS_ST_STATE_RT_DAA             0x00000100      /* bit 8 */
#define TPM_STARTUP_EFFECTS_STARTUP_NO_AUDITDIGEST      0x00000080      /* bit 7 */
#define TPM_STARTUP_EFFECTS_ST_CLEAR_AUDITDIGEST        0x00000040      /* bit 6 */
#define TPM_STARTUP_EFFECTS_STARTUP_AUDITDIGEST         0x00000020      /* bit 5 */
#define TPM_STARTUP_EFFECTS_ST_ANY_RT_KEY               0x00000010      /* bit 4 */
#define TPM_STARTUP_EFFECTS_ST_STATE_RT_AUTH            0x00000008      /* bit 3 */
#define TPM_STARTUP_EFFECTS_ST_STATE_RT_HASH            0x00000004      /* bit 2 */
#define TPM_STARTUP_EFFECTS_ST_STATE_RT_TRANS           0x00000002      /* bit 1 */
#define TPM_STARTUP_EFFECTS_ST_STATE_RT_CONTEXT         0x00000001      /* bit 0 */

/* 4.7 TPM_PROTOCOL_ID rev 87 

   This value identifies the protocol in use. 
*/

#define TPM_PID_NONE            0x0000  /* kgold - added */
#define TPM_PID_OIAP            0x0001  /* The OIAP protocol. */
#define TPM_PID_OSAP            0x0002  /* The OSAP protocol. */
#define TPM_PID_ADIP            0x0003  /* The ADIP protocol. */
#define TPM_PID_ADCP            0X0004  /* The ADCP protocol. */
#define TPM_PID_OWNER           0X0005  /* The protocol for taking ownership of a TPM. */
#define TPM_PID_DSAP            0x0006  /* The DSAP protocol */
#define TPM_PID_TRANSPORT       0x0007  /*The transport protocol */

/* 4.8 TPM_ALGORITHM_ID rev 99

   This table defines the types of algorithms that may be supported by the TPM. 

   The TPM MUST support the algorithms TPM_ALG_RSA, TPM_ALG_SHA, TPM_ALG_HMAC, and TPM_ALG_MGF1
*/

#define TPM_ALG_RSA     0x00000001      /* The RSA algorithm. */
/* #define TPM_ALG_DES  0x00000002         (was the DES algorithm) */
/* #define TPM_ALG_3DES 0X00000003         (was the 3DES algorithm in EDE mode) */
#define TPM_ALG_SHA     0x00000004      /* The SHA1 algorithm */
#define TPM_ALG_HMAC    0x00000005      /* The RFC 2104 HMAC algorithm */
#define TPM_ALG_AES128  0x00000006      /* The AES algorithm, key size 128 */
#define TPM_ALG_MGF1    0x00000007      /* The XOR algorithm using MGF1 to create a string the size
                                           of the encrypted block */
#define TPM_ALG_AES192  0x00000008      /* AES, key size 192 */
#define TPM_ALG_AES256  0x00000009      /* AES, key size 256 */
#define TPM_ALG_XOR     0x0000000A      /* XOR using the rolling nonces */

/* 4.9 TPM_PHYSICAL_PRESENCE rev 87

*/

#define TPM_PHYSICAL_PRESENCE_HW_DISABLE        0x0200 /* Sets the physicalPresenceHWEnable to FALSE
                                                        */
#define TPM_PHYSICAL_PRESENCE_CMD_DISABLE       0x0100 /* Sets the physicalPresenceCMDEnable to
                                                          FALSE */
#define TPM_PHYSICAL_PRESENCE_LIFETIME_LOCK     0x0080 /* Sets the physicalPresenceLifetimeLock to
                                                          TRUE */
#define TPM_PHYSICAL_PRESENCE_HW_ENABLE         0x0040 /* Sets the physicalPresenceHWEnable to TRUE
                                                        */
#define TPM_PHYSICAL_PRESENCE_CMD_ENABLE        0x0020 /* Sets the physicalPresenceCMDEnable to TRUE
                                                        */
#define TPM_PHYSICAL_PRESENCE_NOTPRESENT        0x0010 /* Sets PhysicalPresence = FALSE */
#define TPM_PHYSICAL_PRESENCE_PRESENT           0x0008 /* Sets PhysicalPresence = TRUE */
#define TPM_PHYSICAL_PRESENCE_LOCK              0x0004 /* Sets PhysicalPresenceLock = TRUE */

#define TPM_PHYSICAL_PRESENCE_MASK              0xfc03  /* ~ OR of all above bits */

/* 4.10 TPM_MIGRATE_SCHEME rev 103

   The scheme indicates how the StartMigrate command should handle the migration of the encrypted
   blob.
*/

#define TPM_MS_MIGRATE                  0x0001 /* A public key that can be used with all TPM
                                                  migration commands other than 'ReWrap' mode. */
#define TPM_MS_REWRAP                   0x0002 /* A public key that can be used for the ReWrap mode
                                                  of TPM_CreateMigrationBlob. */
#define TPM_MS_MAINT                    0x0003 /* A public key that can be used for the Maintenance
                                                  commands */
#define TPM_MS_RESTRICT_MIGRATE         0x0004 /* The key is to be migrated to a Migration
                                                  Authority. */
#define TPM_MS_RESTRICT_APPROVE         0x0005 /* The key is to be migrated to an entity approved by
                                                  a Migration Authority using double wrapping */

/* 4.11 TPM_EK_TYPE rev 87 

   This structure indicates what type of information that the EK is dealing with.
*/

#define TPM_EK_TYPE_ACTIVATE    0x0001  /* The blob MUST be TPM_EK_BLOB_ACTIVATE */
#define TPM_EK_TYPE_AUTH        0x0002  /* The blob MUST be TPM_EK_BLOB_AUTH */

/* 4.12 TPM_PLATFORM_SPECIFIC rev 87

   This enumerated type indicates the platform specific spec that the information relates to.
*/

#define TPM_PS_PC_11            0x0001  /* PC Specific version 1.1 */
#define TPM_PS_PC_12            0x0002  /* PC Specific version 1.2 */
#define TPM_PS_PDA_12           0x0003  /* PDA Specific version 1.2 */
#define TPM_PS_Server_12        0x0004  /* Server Specific version 1.2 */
#define TPM_PS_Mobile_12        0x0005  /* Mobil Specific version 1.2 */

/* 5.8 TPM_KEY_USAGE rev 101

   This table defines the types of keys that are possible.  Each value defines for what operation
   the key can be used.  Most key usages can be CMKs.  See 4.2, TPM_PAYLOAD_TYPE.

   Each key has a setting defining the encryption and signature scheme to use. The selection of a
   key usage value limits the choices of encryption and signature schemes.
*/

#define TPM_KEY_UNINITIALIZED   0x0000  /* NOTE: Added.  This seems like a good place to indicate
                                           that a TPM_KEY structure has not been initialized */

#define TPM_KEY_SIGNING         0x0010  /* This SHALL indicate a signing key. The [private] key
                                           SHALL be used for signing operations, only. This means
                                           that it MUST be a leaf of the Protected Storage key
                                           hierarchy. */

#define TPM_KEY_STORAGE         0x0011  /* This SHALL indicate a storage key. The key SHALL be used
                                           to wrap and unwrap other keys in the Protected Storage
                                           hierarchy */

#define TPM_KEY_IDENTITY        0x0012  /* This SHALL indicate an identity key. The key SHALL be
                                           used for operations that require a TPM identity, only. */

#define TPM_KEY_AUTHCHANGE      0X0013  /* This SHALL indicate an ephemeral key that is in use
                                           during the ChangeAuthAsym process, only. */

#define TPM_KEY_BIND            0x0014  /* This SHALL indicate a key that can be used for TPM_Bind
                                           and TPM_Unbind operations only. */

#define TPM_KEY_LEGACY          0x0015  /* This SHALL indicate a key that can perform signing and
                                           binding operations. The key MAY be used for both signing
                                           and binding operations. The TPM_KEY_LEGACY key type is to
                                           allow for use by applications where both signing and
                                           encryption operations occur with the same key. */

#define TPM_KEY_MIGRATE         0x0016  /* This SHALL indicate a key in use for TPM_MigrateKey */

/* 5.8.1 TPM_ENC_SCHEME Mandatory Key Usage Schemes rev 99

   The TPM MUST check that the encryption scheme defined for use with the key is a valid scheme for
   the key type, as follows:
*/

#define TPM_ES_NONE                     0x0001 
#define TPM_ES_RSAESPKCSv15             0x0002 
#define TPM_ES_RSAESOAEP_SHA1_MGF1      0x0003 
#define TPM_ES_SYM_CTR                  0x0004 
#define TPM_ES_SYM_OFB                  0x0005

/* 5.8.1 TPM_SIG_SCHEME Mandatory Key Usage Schemes rev 99

   The TPM MUST check that the signature scheme defined for use with the key is a valid scheme for
   the key type, as follows:
*/

#define TPM_SS_NONE                     0x0001 
#define TPM_SS_RSASSAPKCS1v15_SHA1      0x0002 
#define TPM_SS_RSASSAPKCS1v15_DER       0x0003 
#define TPM_SS_RSASSAPKCS1v15_INFO      0x0004 

/* 5.9 TPM_AUTH_DATA_USAGE rev 110

   The indication to the TPM when authorization sessions for an entity are required.  Future
   versions may allow for more complex decisions regarding AuthData checking.
*/

#define TPM_AUTH_NEVER         0x00 /* This SHALL indicate that usage of the key without
                                       authorization is permitted. */

#define TPM_AUTH_ALWAYS        0x01 /* This SHALL indicate that on each usage of the key the
                                       authorization MUST be performed. */

#define TPM_NO_READ_PUBKEY_AUTH 0x03 /* This SHALL indicate that on commands that require the TPM to
                                       use the the key, the authorization MUST be performed. For
                                       commands that cause the TPM to read the public portion of the
                                       key, but not to use the key (e.g. TPM_GetPubKey), the
                                       authorization may be omitted. */

/* 5.10 TPM_KEY_FLAGS rev 110

   This table defines the meanings of the bits in a TPM_KEY_FLAGS structure, used in
   TPM_STORE_ASYMKEY and TPM_CERTIFY_INFO.
   
   The value of TPM_KEY_FLAGS MUST be decomposed into individual mask values. The presence of a mask
   value SHALL have the effect described in the above table
   
   On input, all undefined bits MUST be zero. The TPM MUST return an error if any undefined bit is
   set. On output, the TPM MUST set all undefined bits to zero.
*/

#ifdef TPM_V12
#define TPM_KEY_FLAGS_MASK      0x0000001f
#else
#define TPM_KEY_FLAGS_MASK      0x00000007
#endif

#define TPM_REDIRECTION         0x00000001 /* This mask value SHALL indicate the use of redirected
                                              output. */

#define TPM_MIGRATABLE          0x00000002 /* This mask value SHALL indicate that the key is
                                              migratable. */

#define TPM_ISVOLATILE          0x00000004 /* This mask value SHALL indicate that the key MUST be
                                              unloaded upon execution of the
                                              TPM_Startup(ST_Clear). This does not indicate that a
                                              non-volatile key will remain loaded across
                                              TPM_Startup(ST_Clear) events. */

#define TPM_PCRIGNOREDONREAD    0x00000008 /* When TRUE the TPM MUST NOT check digestAtRelease or
                                              localityAtRelease for commands that read the public
                                              portion of the key (e.g., TPM_GetPubKey) and MAY NOT
                                              check digestAtRelease or localityAtRelease for
                                              commands that use the public portion of the key
                                              (e.g. TPM_Seal)

                                              When FALSE the TPM MUST check digestAtRelease and
                                              localityAtRelease for commands that read or use the
                                              public portion of the key */

#define TPM_MIGRATEAUTHORITY    0x00000010 /* When set indicates that the key is under control of a
                                              migration authority. The TPM MUST only allow the
                                              creation of a key with this flag in
                                              TPM_MA_CreateKey */

/* 5.17 TPM_CMK_DELEGATE values rev 89

   The bits of TPM_CMK_DELEGATE are flags that determine how the TPM responds to delegated requests
   to manipulate a certified-migration-key, a loaded key with payload type TPM_PT_MIGRATE_RESTRICTED
   or TPM_PT_MIGRATE_EXTERNAL..

   26:0 reserved MUST be 0

   The default value of TPM_CMK_Delegate is zero (0)
*/

#define TPM_CMK_DELEGATE_SIGNING        0x80000000 /* When set to 1, this bit SHALL indicate that a
                                                      delegated command may manipulate a CMK of
                                                      TPM_KEY_USAGE == TPM_KEY_SIGNING */
#define TPM_CMK_DELEGATE_STORAGE        0x40000000 /* When set to 1, this bit SHALL indicate that a
                                                      delegated command may manipulate a CMK of
                                                      TPM_KEY_USAGE == TPM_KEY_STORAGE */
#define TPM_CMK_DELEGATE_BIND           0x20000000 /* When set to 1, this bit SHALL indicate that a
                                                      delegated command may manipulate a CMK of
                                                      TPM_KEY_USAGE == TPM_KEY_BIND */
#define TPM_CMK_DELEGATE_LEGACY         0x10000000 /* When set to 1, this bit SHALL indicate that a
                                                      delegated command may manipulate a CMK of
                                                      TPM_KEY_USAGE == TPM_KEY_LEGACY */
#define TPM_CMK_DELEGATE_MIGRATE        0x08000000 /* When set to 1, this bit SHALL indicate that a
                                                      delegated command may manipulate a CMK of
                                                      TPM_KEY_USAGE == TPM_KEY_MIGRATE */

/* 6. TPM_TAG (Command and Response Tags) rev 100

   These tags indicate to the TPM the construction of the command either as input or as output. The
   AUTH indicates that there are one or more AuthData values that follow the command
   parameters.
*/

#define TPM_TAG_RQU_COMMAND             0x00C1 /* A command with no authentication.  */
#define TPM_TAG_RQU_AUTH1_COMMAND       0x00C2 /* An authenticated command with one authentication
                                                  handle */
#define TPM_TAG_RQU_AUTH2_COMMAND       0x00C3 /* An authenticated command with two authentication
                                                  handles */
#define TPM_TAG_RSP_COMMAND             0x00C4 /* A response from a command with no authentication
                                                */
#define TPM_TAG_RSP_AUTH1_COMMAND       0x00C5 /* An authenticated response with one authentication
                                                  handle */
#define TPM_TAG_RSP_AUTH2_COMMAND       0x00C6 /* An authenticated response with two authentication
                                                  handles */

/* TIS 7.2 PCR Attributes

*/

#define TPM_DEBUG_PCR 		16
#define TPM_LOCALITY_4_PCR	17
#define TPM_LOCALITY_3_PCR	18
#define TPM_LOCALITY_2_PCR	19
#define TPM_LOCALITY_1_PCR	20

/* 10.9 TPM_KEY_CONTROL rev 87

   Attributes that can control various aspects of key usage and manipulation.

   Allows for controlling of the key when loaded and how to handle TPM_Startup issues.
*/

#define TPM_KEY_CONTROL_OWNER_EVICT     0x00000001      /* Owner controls when the key is evicted
                                                           from the TPM. When set the TPM MUST
                                                           preserve key the key across all TPM_Init
                                                           invocations. */

/* 13.1.1 TPM_TRANSPORT_ATTRIBUTES Definitions */

#define TPM_TRANSPORT_ENCRYPT           0x00000001      /* The session will provide encryption using
                                                           the internal encryption algorithm */
#define TPM_TRANSPORT_LOG               0x00000002      /* The session will provide a log of all
                                                           operations that occur in the session */
#define TPM_TRANSPORT_EXCLUSIVE         0X00000004      /* The transport session is exclusive and
                                                           any command executed outside the
                                                           transport session causes the invalidation
                                                           of the session */

/* 21.1 TPM_CAPABILITY_AREA rev 115

   To identify a capability to be queried. 
*/

#define TPM_CAP_ORD             0x00000001 /* Boolean value. TRUE indicates that the TPM supports
                                              the ordinal. FALSE indicates that the TPM does not
                                              support the ordinal.  Unimplemented optional ordinals
                                              and unused (unassigned) ordinals return FALSE. */
#define TPM_CAP_ALG             0x00000002 /* Boolean value. TRUE means that the TPM supports the
                                              asymmetric algorithm for TPM_Sign, TPM_Seal,
                                              TPM_UnSeal and TPM_UnBind and related commands. FALSE
                                              indicates that the asymmetric algorithm is not
                                              supported for these types of commands. The TPM MAY
                                              return TRUE or FALSE for other than asymmetric
                                              algorithms that it supports. Unassigned and
                                              unsupported algorithm IDs return FALSE.*/

#define TPM_CAP_PID             0x00000003 /* Boolean value. TRUE indicates that the TPM supports
                                              the protocol, FALSE indicates that the TPM does not
                                              support the protocol.  */
#define TPM_CAP_FLAG            0x00000004 /* Return the TPM_PERMANENT_FLAGS structure or Return the
                                              TPM_STCLEAR_FLAGS structure */
#define TPM_CAP_PROPERTY        0x00000005 /* See following table for the subcaps */
#define TPM_CAP_VERSION         0x00000006 /* TPM_STRUCT_VER structure. The Major and Minor must
                                              indicate 1.1. The firmware revision MUST indicate
                                              0.0 */
#define TPM_CAP_KEY_HANDLE      0x00000007 /* A TPM_KEY_HANDLE_LIST structure that enumerates all
                                              key handles loaded on the TPM.  */
#define TPM_CAP_CHECK_LOADED    0x00000008 /* A Boolean value. TRUE indicates that the TPM has
                                              enough memory available to load a key of the type
                                              specified by TPM_KEY_PARMS. FALSE indicates that the
                                              TPM does not have enough memory.  */
#define TPM_CAP_SYM_MODE        0x00000009 /* Subcap TPM_SYM_MODE
                                              A Boolean value. TRUE indicates that the TPM supports
                                              the TPM_SYM_MODE, FALSE indicates the TPM does not
                                              support the mode. */
#define TPM_CAP_KEY_STATUS      0x0000000C /* Boolean value of ownerEvict. The handle MUST point to
                                              a valid key handle.*/
#define TPM_CAP_NV_LIST         0x0000000D /* A list of TPM_NV_INDEX values that are currently
                                              allocated NV storage through TPM_NV_DefineSpace. */
#define TPM_CAP_MFR             0x00000010 /* Manufacturer specific. The manufacturer may provide
                                              any additional information regarding the TPM and the
                                              TPM state but MUST not expose any sensitive
                                              information.  */
#define TPM_CAP_NV_INDEX        0x00000011 /* A TPM_NV_DATA_PUBLIC structure that indicates the
                                              values for the TPM_NV_INDEX.  Returns TPM_BADINDEX if
                                              the index is not in the TPM_CAP_NV_LIST list. */
#define TPM_CAP_TRANS_ALG       0x00000012 /* Boolean value. TRUE means that the TPM supports the
                                              algorithm for TPM_EstablishTransport,
                                              TPM_ExecuteTransport and
                                              TPM_ReleaseTransportSigned. FALSE indicates that for
                                              these three commands the algorithm is not supported."
                                              */
#define TPM_CAP_HANDLE          0x00000014 /* A TPM_KEY_HANDLE_LIST structure that enumerates all
                                              handles currently loaded in the TPM for the given
                                              resource type.  */
#define TPM_CAP_TRANS_ES        0x00000015 /* Boolean value. TRUE means the TPM supports the
                                              encryption scheme in a transport session for at least
                                              one algorithm..  */
#define TPM_CAP_AUTH_ENCRYPT    0x00000017 /* Boolean value. TRUE indicates that the TPM supports
                                              the encryption algorithm in OSAP encryption of
                                              AuthData values */
#define TPM_CAP_SELECT_SIZE     0x00000018 /* Boolean value. TRUE indicates that the TPM supports
                                              the size for the given version. For instance a request
                                              could ask for version 1.1 size 2 and the TPM would
                                              indicate TRUE. For 1.1 size 3 the TPM would indicate
                                              FALSE. For 1.2 size 3 the TPM would indicate TRUE. */
#define TPM_CAP_DA_LOGIC        0x00000019 /* (OPTIONAL)
                                              A TPM_DA_INFO or TPM_DA_INFO_LIMITED structure that
                                              returns data according to the selected entity type
                                              (e.g., TPM_ET_KEYHANDLE, TPM_ET_OWNER, TPM_ET_SRK,
                                              TPM_ET_COUNTER, TPM_ET_OPERATOR, etc.). If the
                                              implemented dictionary attack logic does not support
                                              different secret types, the entity type can be
                                              ignored. */
#define TPM_CAP_VERSION_VAL     0x0000001A /* TPM_CAP_VERSION_INFO structure. The TPM fills in the
                                              structure and returns the information indicating what
                                              the TPM currently supports. */

#define TPM_CAP_FLAG_PERMANENT  0x00000108 /* Return the TPM_PERMANENT_FLAGS structure */
#define TPM_CAP_FLAG_VOLATILE   0x00000109 /* Return the TPM_STCLEAR_FLAGS structure */

/* 21.2 CAP_PROPERTY Subcap values for CAP_PROPERTY rev 105

   The TPM_CAP_PROPERTY capability has numerous subcap values.  The definition for all subcap values
   occurs in this table.

   TPM_CAP_PROP_MANUFACTURER returns a vendor ID unique to each manufacturer. The same value is
   returned as the TPM_CAP_VERSION_INFO -> tpmVendorID.  A company abbreviation such as a null
   terminated stock ticker is a typical choice. However, there is no requirement that the value
   contain printable characters.  The document "TCG Vendor Naming" lists the vendor ID values.

   TPM_CAP_PROP_MAX_xxxSESS is a constant.  At TPM_Startup(ST_CLEAR) TPM_CAP_PROP_xxxSESS ==
   TPM_CAP_PROP_MAX_xxxSESS.  As sessions are created on the TPM, TPM_CAP_PROP_xxxSESS decreases
   toward zero.  As sessions are terminated, TPM_CAP_PROP_xxxSESS increases toward
   TPM_CAP_PROP_MAX_xxxSESS.

   There is a similar relationship between the constants TPM_CAP_PROP_MAX_COUNTERS and
   TPM_CAP_PROP_MAX_CONTEXT and the varying TPM_CAP_PROP_COUNTERS and TPM_CAP_PROP_CONTEXT.
   
   In one typical implementation where authorization and transport sessions reside in separate
   pools, TPM_CAP_PROP_SESSIONS will be the sum of TPM_CAP_PROP_AUTHSESS and TPM_CAP_PROP_TRANSESS.
   In another typical implementation where authorization and transport sessions share the same pool,
   TPM_CAP_PROP_SESSIONS, TPM_CAP_PROP_AUTHSESS, and TPM_CAP_PROP_TRANSESS will all be equal.
*/

#define TPM_CAP_PROP_PCR                0x00000101    /* uint32_t value. Returns the number of PCR
                                                         registers supported by the TPM */
#define TPM_CAP_PROP_DIR                0x00000102    /* uint32_t. Deprecated. Returns the number of
                                                         DIR, which is now fixed at 1 */
#define TPM_CAP_PROP_MANUFACTURER       0x00000103    /* uint32_t value.  Returns the vendor ID
                                                         unique to each TPM manufacturer. */
#define TPM_CAP_PROP_KEYS               0x00000104    /* uint32_t value. Returns the number of 2048-
                                                         bit RSA keys that can be loaded. This may
                                                         vary with time and circumstances. */
#define TPM_CAP_PROP_MIN_COUNTER        0x00000107    /* uint32_t. The minimum amount of time in
                                                         10ths of a second that must pass between
                                                         invocations of incrementing the monotonic
                                                         counter. */
#define TPM_CAP_PROP_AUTHSESS           0x0000010A    /* uint32_t. The number of available
                                                         authorization sessions. This may vary with
                                                         time and circumstances. */
#define TPM_CAP_PROP_TRANSESS           0x0000010B    /* uint32_t. The number of available transport
                                                         sessions. This may vary with time and
                                                         circumstances.  */
#define TPM_CAP_PROP_COUNTERS           0x0000010C    /* uint32_t. The number of available monotonic
                                                         counters. This may vary with time and
                                                         circumstances. */
#define TPM_CAP_PROP_MAX_AUTHSESS       0x0000010D    /* uint32_t. The maximum number of loaded
                                                         authorization sessions the TPM supports */
#define TPM_CAP_PROP_MAX_TRANSESS       0x0000010E    /* uint32_t. The maximum number of loaded
                                                         transport sessions the TPM supports. */
#define TPM_CAP_PROP_MAX_COUNTERS       0x0000010F    /* uint32_t. The maximum number of monotonic
                                                         counters under control of TPM_CreateCounter
                                                         */
#define TPM_CAP_PROP_MAX_KEYS           0x00000110    /* uint32_t. The maximum number of 2048 RSA
                                                         keys that the TPM can support. The number
                                                         does not include the EK or SRK. */
#define TPM_CAP_PROP_OWNER              0x00000111    /* BOOL. A value of TRUE indicates that the
                                                         TPM has successfully installed an owner. */
#define TPM_CAP_PROP_CONTEXT            0x00000112    /* uint32_t. The number of available saved
                                                         session slots. This may vary with time and
                                                         circumstances. */
#define TPM_CAP_PROP_MAX_CONTEXT        0x00000113    /* uint32_t. The maximum number of saved
                                                         session slots. */
#define TPM_CAP_PROP_FAMILYROWS         0x00000114    /* uint32_t. The maximum number of rows in the
                                                         family table */
#define TPM_CAP_PROP_TIS_TIMEOUT        0x00000115    /* A 4 element array of uint32_t values each
                                                         denoting the timeout value in microseconds
                                                         for the following in this order:
                                                         
                                                         TIMEOUT_A, TIMEOUT_B, TIMEOUT_C, TIMEOUT_D 

                                                         Where these timeouts are to be used is
                                                         determined by the platform specific TPM
                                                         Interface Specification. */
#define TPM_CAP_PROP_STARTUP_EFFECT     0x00000116    /* The TPM_STARTUP_EFFECTS structure */
#define TPM_CAP_PROP_DELEGATE_ROW       0x00000117    /* uint32_t. The maximum size of the delegate
                                                         table in rows. */
#define TPM_CAP_PROP_MAX_DAASESS        0x00000119    /* uint32_t. The maximum number of loaded DAA
                                                         sessions (join or sign) that the TPM
                                                         supports */
#define TPM_CAP_PROP_DAASESS            0x0000011A    /* uint32_t. The number of available DAA
                                                         sessions. This may vary with time and
                                                         circumstances */
#define TPM_CAP_PROP_CONTEXT_DIST       0x0000011B    /* uint32_t. The maximum distance between
                                                         context count values. This MUST be at least
                                                         2^16-1. */
#define TPM_CAP_PROP_DAA_INTERRUPT      0x0000011C    /* BOOL. A value of TRUE indicates that the
                                                         TPM will accept ANY command while executing
                                                         a DAA Join or Sign.

                                                         A value of FALSE indicates that the TPM
                                                         will invalidate the DAA Join or Sign upon
                                                         the receipt of any command other than the
                                                         next join/sign in the session or a
                                                         TPM_SaveContext */
#define TPM_CAP_PROP_SESSIONS           0X0000011D    /* uint32_t. The number of available sessions
                                                         from the pool. This MAY vary with time and
                                                         circumstances. Pool sessions include
                                                         authorization and transport sessions. */
#define TPM_CAP_PROP_MAX_SESSIONS       0x0000011E    /* uint32_t. The maximum number of sessions
                                                         the TPM supports. */
#define TPM_CAP_PROP_CMK_RESTRICTION    0x0000011F    /* uint32_t TPM_Permanent_Data ->
                                                         restrictDelegate
                                                       */
#define TPM_CAP_PROP_DURATION           0x00000120    /* A 3 element array of uint32_t values each
                                                         denoting the duration value in microseconds
                                                         of the duration of the three classes of
                                                         commands: Small, Medium and Long in the
                                                         following in this order: SMALL_DURATION,
                                                         MEDIUM_DURATION, LONG_DURATION */
#define TPM_CAP_PROP_ACTIVE_COUNTER     0x00000122      /* TPM_COUNT_ID. The id of the current
                                                           counter. 0xff..ff if no counter is active
                                                        */
#define TPM_CAP_PROP_MAX_NV_AVAILABLE   0x00000123      /*uint32_t. Deprecated.  The maximum number
                                                          of NV space that can be allocated, MAY
                                                          vary with time and circumstances.  This
                                                          capability was not implemented
                                                          consistently, and is replaced by
                                                          TPM_NV_INDEX_TRIAL. */
#define TPM_CAP_PROP_INPUT_BUFFER       0x00000124      /* uint32_t. The maximum size of the TPM
                                                           input buffer or output buffer in
                                                           bytes. */

/* 21.4 Set_Capability Values rev 107
 */
   
#define TPM_SET_PERM_FLAGS      0x00000001      /* The ability to set a value is field specific and
                                                   a review of the structure will disclose the
                                                   ability and requirements to set a value */
#define TPM_SET_PERM_DATA       0x00000002      /* The ability to set a value is field specific and
                                                   a review of the structure will disclose the
                                                   ability and requirements to set a value */
#define TPM_SET_STCLEAR_FLAGS   0x00000003      /* The ability to set a value is field specific and
                                                   a review of the structure will disclose the
                                                   ability and requirements to set a value */
#define TPM_SET_STCLEAR_DATA    0x00000004      /* The ability to set a value is field specific and
                                                   a review of the structure will disclose the
                                                   ability and requirements to set a value */
#define TPM_SET_STANY_FLAGS     0x00000005      /* The ability to set a value is field specific and
                                                   a review of the structure will disclose the
                                                   ability and requirements to set a value */
#define TPM_SET_STANY_DATA      0x00000006      /* The ability to set a value is field specific and
                                                   a review of the structure will disclose the
                                                   ability and requirements to set a value */
#define TPM_SET_VENDOR          0x00000007      /* This area allows the vendor to set specific areas
                                                   in the TPM according to the normal shielded
                                                   location requirements */

/* Set Capability sub caps */

/* TPM_PERMANENT_FLAGS */

#define  TPM_PF_DISABLE                         1
#define  TPM_PF_OWNERSHIP                       2
#define  TPM_PF_DEACTIVATED                     3
#define  TPM_PF_READPUBEK                       4
#define  TPM_PF_DISABLEOWNERCLEAR               5
#define  TPM_PF_ALLOWMAINTENANCE                6
#define  TPM_PF_PHYSICALPRESENCELIFETIMELOCK    7
#define  TPM_PF_PHYSICALPRESENCEHWENABLE        8
#define  TPM_PF_PHYSICALPRESENCECMDENABLE       9
#define  TPM_PF_CEKPUSED                        10
#define  TPM_PF_TPMPOST                         11
#define  TPM_PF_TPMPOSTLOCK                     12
#define  TPM_PF_FIPS                            13
#define  TPM_PF_OPERATOR                        14
#define  TPM_PF_ENABLEREVOKEEK                  15
#define  TPM_PF_NV_LOCKED                       16
#define  TPM_PF_READSRKPUB                      17
#define  TPM_PF_TPMESTABLISHED                  18
#define  TPM_PF_MAINTENANCEDONE                 19
#define  TPM_PF_DISABLEFULLDALOGICINFO          20

/* TPM_STCLEAR_FLAGS */

#define  TPM_SF_DEACTIVATED                     1
#define  TPM_SF_DISABLEFORCECLEAR               2
#define  TPM_SF_PHYSICALPRESENCE                3
#define  TPM_SF_PHYSICALPRESENCELOCK            4
#define  TPM_SF_BGLOBALLOCK                     5
                                                
/* TPM_STANY_FLAGS */                           
                                                
#define  TPM_AF_POSTINITIALISE                  1
#define  TPM_AF_LOCALITYMODIFIER                2
#define  TPM_AF_TRANSPORTEXCLUSIVE              3
#define  TPM_AF_TOSPRESENT                      4
                                                
/* TPM_PERMANENT_DATA */                        
                                                
#define  TPM_PD_REVMAJOR                        1
#define  TPM_PD_REVMINOR                        2
#define  TPM_PD_TPMPROOF                        3
#define  TPM_PD_OWNERAUTH                       4
#define  TPM_PD_OPERATORAUTH                    5
#define  TPM_PD_MANUMAINTPUB                    6
#define  TPM_PD_ENDORSEMENTKEY                  7
#define  TPM_PD_SRK                             8
#define  TPM_PD_DELEGATEKEY                     9
#define  TPM_PD_CONTEXTKEY                      10
#define  TPM_PD_AUDITMONOTONICCOUNTER           11
#define  TPM_PD_MONOTONICCOUNTER                12
#define  TPM_PD_PCRATTRIB                       13
#define  TPM_PD_ORDINALAUDITSTATUS              14
#define  TPM_PD_AUTHDIR                         15
#define  TPM_PD_RNGSTATE                        16
#define  TPM_PD_FAMILYTABLE                     17
#define  TPM_DELEGATETABLE                      18
#define  TPM_PD_EKRESET                         19
#define  TPM_PD_LASTFAMILYID                    21
#define  TPM_PD_NOOWNERNVWRITE                  22
#define  TPM_PD_RESTRICTDELEGATE                23
#define  TPM_PD_TPMDAASEED                      24
#define  TPM_PD_DAAPROOF                        25
                                                
/* TPM_STCLEAR_DATA */                          
                                                
#define  TPM_SD_CONTEXTNONCEKEY                 1
#define  TPM_SD_COUNTID                         2
#define  TPM_SD_OWNERREFERENCE                  3
#define  TPM_SD_DISABLERESETLOCK                4
#define  TPM_SD_PCR                             5
#define  TPM_SD_DEFERREDPHYSICALPRESENCE        6

/* TPM_STCLEAR_DATA -> deferredPhysicalPresence bits */

#define  TPM_DPP_UNOWNED_FIELD_UPGRADE  0x00000001      /* bit 0 TPM_FieldUpgrade */
                                
/* TPM_STANY_DATA */                            
                                                
#define  TPM_AD_CONTEXTNONCESESSION             1
#define  TPM_AD_AUDITDIGEST                     2
#define  TPM_AD_CURRENTTICKS                    3
#define  TPM_AD_CONTEXTCOUNT                    4
#define  TPM_AD_CONTEXTLIST                     5
#define  TPM_AD_SESSIONS                        6

/*  17. Ordinals rev 110

    Ordinals are 32 bit values of type TPM_COMMAND_CODE. The upper byte contains values that serve
    as flag indicators, the next byte contains values indicating what committee designated the
    ordinal, and the final two bytes contain the Command Ordinal Index.

       3                   2                   1 
     1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |P|C|V| Reserved|    Purview    |     Command Ordinal Index     |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 

    Where: 

    P is Protected/Unprotected command. When 0 the command is a Protected command, when 1 the
    command is an Unprotected command.

    C is Non-Connection/Connection related command. When 0 this command passes through to either the
    protected (TPM) or unprotected (TSS) components.

    V is TPM/Vendor command. When 0 the command is TPM defined, when 1 the command is vendor
    defined.

    All reserved area bits are set to 0. 
*/

/* The following masks are created to allow for the quick definition of the commands */

#define TPM_PROTECTED_COMMAND   0x00000000 /* TPM protected command, specified in main specification
                                            */
#define TPM_UNPROTECTED_COMMAND 0x80000000 /* TSS command, specified in the TSS specification */
#define TPM_CONNECTION_COMMAND  0x40000000 /* TSC command, protected connection commands are
                                              specified in the main specification Unprotected
                                              connection commands are specified in the TSS */
#define TPM_VENDOR_COMMAND      0x20000000 /* Command that is vendor specific for a given TPM or
                                              TSS.  */


/* The following Purviews have been defined: */

#define TPM_MAIN        0x00 /* Command is from the main specification  */
#define TPM_PC          0x01 /* Command is specific to the PC  */
#define TPM_PDA         0x02 /* Command is specific to a PDA  */
#define TPM_CELL_PHONE  0x03 /* Command is specific to a cell phone  */
#define TPM_SERVER      0x04 /* Command is specific to servers  */
#define TPM_PERIPHERAL  0x05 /* Command is specific to peripherals */
#define TPM_TSS         0x06 /* Command is specific to TSS */

/* Combinations for the main specification would be:   */

#define TPM_PROTECTED_ORDINAL   (TPM_PROTECTED_COMMAND   | TPM_MAIN)
#define TPM_UNPROTECTED_ORDINAL (TPM_UNPROTECTED_COMMAND | TPM_MAIN)
#define TPM_CONNECTION_ORDINAL  (TPM_CONNECTION_COMMAND  | TPM_MAIN)

/* Command ordinals */

#define TPM_ORD_ActivateIdentity                0x0000007A
#define TPM_ORD_AuthorizeMigrationKey           0x0000002B
#define TPM_ORD_CertifyKey                      0x00000032
#define TPM_ORD_CertifyKey2                     0x00000033
#define TPM_ORD_CertifySelfTest                 0x00000052
#define TPM_ORD_ChangeAuth                      0x0000000C
#define TPM_ORD_ChangeAuthAsymFinish            0x0000000F
#define TPM_ORD_ChangeAuthAsymStart             0x0000000E
#define TPM_ORD_ChangeAuthOwner                 0x00000010
#define TPM_ORD_CMK_ApproveMA                   0x0000001D
#define TPM_ORD_CMK_ConvertMigration            0x00000024
#define TPM_ORD_CMK_CreateBlob                  0x0000001B
#define TPM_ORD_CMK_CreateKey                   0x00000013
#define TPM_ORD_CMK_CreateTicket                0x00000012
#define TPM_ORD_CMK_SetRestrictions             0x0000001C
#define TPM_ORD_ContinueSelfTest                0x00000053
#define TPM_ORD_ConvertMigrationBlob            0x0000002A
#define TPM_ORD_CreateCounter                   0x000000DC
#define TPM_ORD_CreateEndorsementKeyPair        0x00000078
#define TPM_ORD_CreateMaintenanceArchive        0x0000002C
#define TPM_ORD_CreateMigrationBlob             0x00000028
#define TPM_ORD_CreateRevocableEK               0x0000007F
#define TPM_ORD_CreateWrapKey                   0x0000001F
#define TPM_ORD_DAA_Join                        0x00000029
#define TPM_ORD_DAA_Sign                        0x00000031
#define TPM_ORD_Delegate_CreateKeyDelegation    0x000000D4
#define TPM_ORD_Delegate_CreateOwnerDelegation  0x000000D5
#define TPM_ORD_Delegate_LoadOwnerDelegation    0x000000D8
#define TPM_ORD_Delegate_Manage                 0x000000D2
#define TPM_ORD_Delegate_ReadTable              0x000000DB
#define TPM_ORD_Delegate_UpdateVerification     0x000000D1
#define TPM_ORD_Delegate_VerifyDelegation       0x000000D6
#define TPM_ORD_DirRead                         0x0000001A
#define TPM_ORD_DirWriteAuth                    0x00000019
#define TPM_ORD_DisableForceClear               0x0000005E
#define TPM_ORD_DisableOwnerClear               0x0000005C
#define TPM_ORD_DisablePubekRead                0x0000007E
#define TPM_ORD_DSAP                            0x00000011
#define TPM_ORD_EstablishTransport              0x000000E6
#define TPM_ORD_EvictKey                        0x00000022
#define TPM_ORD_ExecuteTransport                0x000000E7
#define TPM_ORD_Extend                          0x00000014
#define TPM_ORD_FieldUpgrade                    0x000000AA
#define TPM_ORD_FlushSpecific                   0x000000BA
#define TPM_ORD_ForceClear                      0x0000005D
#define TPM_ORD_GetAuditDigest                  0x00000085
#define TPM_ORD_GetAuditDigestSigned            0x00000086
#define TPM_ORD_GetAuditEvent                   0x00000082
#define TPM_ORD_GetAuditEventSigned             0x00000083
#define TPM_ORD_GetCapability                   0x00000065
#define TPM_ORD_GetCapabilityOwner              0x00000066
#define TPM_ORD_GetCapabilitySigned             0x00000064
#define TPM_ORD_GetOrdinalAuditStatus           0x0000008C
#define TPM_ORD_GetPubKey                       0x00000021
#define TPM_ORD_GetRandom                       0x00000046
#define TPM_ORD_GetTestResult                   0x00000054
#define TPM_ORD_GetTicks                        0x000000F1
#define TPM_ORD_IncrementCounter                0x000000DD
#define TPM_ORD_Init                            0x00000097
#define TPM_ORD_KeyControlOwner                 0x00000023
#define TPM_ORD_KillMaintenanceFeature          0x0000002E
#define TPM_ORD_LoadAuthContext                 0x000000B7
#define TPM_ORD_LoadContext                     0x000000B9
#define TPM_ORD_LoadKey                         0x00000020
#define TPM_ORD_LoadKey2                        0x00000041
#define TPM_ORD_LoadKeyContext                  0x000000B5
#define TPM_ORD_LoadMaintenanceArchive          0x0000002D
#define TPM_ORD_LoadManuMaintPub                0x0000002F
#define TPM_ORD_MakeIdentity                    0x00000079
#define TPM_ORD_MigrateKey                      0x00000025
#define TPM_ORD_NV_DefineSpace                  0x000000CC
#define TPM_ORD_NV_ReadValue                    0x000000CF
#define TPM_ORD_NV_ReadValueAuth                0x000000D0
#define TPM_ORD_NV_WriteValue                   0x000000CD
#define TPM_ORD_NV_WriteValueAuth               0x000000CE
#define TPM_ORD_OIAP                            0x0000000A
#define TPM_ORD_OSAP                            0x0000000B
#define TPM_ORD_OwnerClear                      0x0000005B
#define TPM_ORD_OwnerReadInternalPub            0x00000081
#define TPM_ORD_OwnerReadPubek                  0x0000007D
#define TPM_ORD_OwnerSetDisable                 0x0000006E
#define TPM_ORD_PCR_Reset                       0x000000C8
#define TPM_ORD_PcrRead                         0x00000015
#define TPM_ORD_PhysicalDisable                 0x00000070
#define TPM_ORD_PhysicalEnable                  0x0000006F
#define TPM_ORD_PhysicalSetDeactivated          0x00000072
#define TPM_ORD_Quote                           0x00000016
#define TPM_ORD_Quote2                          0x0000003E
#define TPM_ORD_ReadCounter                     0x000000DE
#define TPM_ORD_ReadManuMaintPub                0x00000030
#define TPM_ORD_ReadPubek                       0x0000007C
#define TPM_ORD_ReleaseCounter                  0x000000DF
#define TPM_ORD_ReleaseCounterOwner             0x000000E0
#define TPM_ORD_ReleaseTransportSigned          0x000000E8
#define TPM_ORD_Reset                           0x0000005A
#define TPM_ORD_ResetLockValue                  0x00000040
#define TPM_ORD_RevokeTrust                     0x00000080
#define TPM_ORD_SaveAuthContext                 0x000000B6
#define TPM_ORD_SaveContext                     0x000000B8
#define TPM_ORD_SaveKeyContext                  0x000000B4
#define TPM_ORD_SaveState                       0x00000098
#define TPM_ORD_Seal                            0x00000017
#define TPM_ORD_Sealx                           0x0000003D
#define TPM_ORD_SelfTestFull                    0x00000050
#define TPM_ORD_SetCapability                   0x0000003F
#define TPM_ORD_SetOperatorAuth                 0x00000074
#define TPM_ORD_SetOrdinalAuditStatus           0x0000008D
#define TPM_ORD_SetOwnerInstall                 0x00000071
#define TPM_ORD_SetOwnerPointer                 0x00000075
#define TPM_ORD_SetRedirection                  0x0000009A
#define TPM_ORD_SetTempDeactivated              0x00000073
#define TPM_ORD_SHA1Complete                    0x000000A2
#define TPM_ORD_SHA1CompleteExtend              0x000000A3
#define TPM_ORD_SHA1Start                       0x000000A0
#define TPM_ORD_SHA1Update                      0x000000A1
#define TPM_ORD_Sign                            0x0000003C
#define TPM_ORD_Startup                         0x00000099
#define TPM_ORD_StirRandom                      0x00000047
#define TPM_ORD_TakeOwnership                   0x0000000D
#define TPM_ORD_Terminate_Handle                0x00000096
#define TPM_ORD_TickStampBlob                   0x000000F2
#define TPM_ORD_UnBind                          0x0000001E
#define TPM_ORD_Unseal                          0x00000018

#define TSC_ORD_PhysicalPresence                0x4000000A
#define TSC_ORD_ResetEstablishmentBit           0x4000000B

/* 19. NV storage structures */

/* 19.1 TPM_NV_INDEX rev 110

     The index provides the handle to identify the area of storage. The reserved bits allow for a
     segregation of the index name space to avoid name collisions.

     The TPM may check the resvd bits for zero.  Thus, applications should set the bits to zero.

     The TCG defines the space where the high order bits (T, P, U) are 0. The other spaces are
     controlled by the indicated entity.

     T is the TPM manufacturer reserved bit. 0 indicates a TCG defined value. 1 indicates a TPM
     manufacturer specific value.

     P is the platform manufacturer reserved bit. 0 indicates a TCG defined value. 1 indicates that
     the index is controlled by the platform manufacturer.

     U is for the platform user. 0 indicates a TCG defined value. 1 indicates that the index is
     controlled by the platform user.

     The TPM_NV_INDEX is a 32-bit value.
     3                   2                   1
     1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |T|P|U|D| resvd |   Purview      |         Index                |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

     Where:

     1. The TPM MAY return an error if the reserved area bits are not set to 0.

     2. The TPM MUST accept all values for T, P, and U

     3. D indicates defined. 1 indicates that the index is permanently defined and that any
        TPM_NV_DefineSpace operation will fail after nvLocked is set TRUE.

     a. TCG reserved areas MAY have D set to 0 or 1
        
     4. Purview is the value used to indicate the platform specific area. This value is the
     same as used for command ordinals.

     a. The TPM MUST reject purview values that the TPM cannot support. This means that an
     index value for a PDA MUST be rejected by a TPM designed to work only on the PC Client.
*/

#define TPM_NV_INDEX_T_BIT              0x80000000
#define TPM_NV_INDEX_P_BIT              0x40000000
#define TPM_NV_INDEX_U_BIT              0x20000000
#define TPM_NV_INDEX_D_BIT              0x10000000
/* added kgold */
#define TPM_NV_INDEX_RESVD              0x0f000000
#define TPM_NV_INDEX_PURVIEW_BIT        16
#define TPM_NV_INDEX_PURVIEW_MASK       0x00ff0000

/* 19.1.1 Required TPM_NV_INDEX values rev 97

   The required index values must be found on each TPM regardless of platform. These areas are
   always present and do not require a TPM_DefineSpace command to allocate.

   A platform specific specification may add additional required index values for the platform.

   The TPM MUST reserve the space as indicated for the required index values
*/

#define TPM_NV_INDEX_LOCK  0xFFFFFFFF   /* This value turns on the NV authorization
                                           protections. Once executed all NV areas use the
                                           protections as defined. This value never resets.

                                           Attempting to execute TPM_NV_DefineSpace on this value
                                           with non-zero size MAY result in a TPM_BADINDEX
                                           response.
                                        */

#define TPM_NV_INDEX0      0x00000000   /* This value allows for the setting of the bGlobalLock
                                           flag, which is only reset on TPM_Startup(ST_Clear)

                                           Attempting to execute TPM_NV_WriteValue with a size other
                                           than zero MAY result in the TPM_BADINDEX error code.
                                        */

#define TPM_NV_INDEX_DIR   0x10000001   /* Size MUST be 20. This index points to the deprecated DIR
                                           command area from 1.1.  The TPM MUST map this reserved
                                           space to be the area operated on by the 1.1 DIR commands.
                                           */

/* 19.1.2 Reserved Index values rev 116

  The reserved values are defined to avoid index collisions. These values are not in each and every
  TPM.

  1. The reserved index values are to avoid index value collisions. 
  2. These index values require a TPM_DefineSpace to have the area for the index allocated 
  3. A platform specific specification MAY indicate that reserved values are required. 
  4. The reserved index values MAY have their D bit set by the TPM vendor to permanently
*/

#define TPM_NV_INDEX_TPM                0x0000Fxxx      /* Reserved for TPM use */
#define TPM_NV_INDEX_EKCert             0x0000F000      /* The Endorsement credential */

#define TPM_NV_INDEX_TPM_CC             0x0000F001      /* The TPM Conformance credential */
#define TPM_NV_INDEX_PlatformCert       0x0000F002      /* The platform credential */
#define TPM_NV_INDEX_Platform_CC        0x0000F003      /* The Platform conformance credential */
#define TPM_NV_INDEX_TRIAL              0x0000F004      /* To try TPM_NV_DefineSpace without
                                                           actually allocating NV space */

#if 0
#define TPM_NV_INDEX_PC                 0x0001xxxx      /* Reserved for PC Client use */
#define TPM_NV_INDEX_GPIO_xx            0x000116xx      /* Reserved for GPIO pins */
#define TPM_NV_INDEX_PDA                0x0002xxxx      /* Reserved for PDA use */
#define TPM_NV_INDEX_MOBILE             0x0003xxxx      /* Reserved for mobile use */
#define TPM_NV_INDEX_SERVER             0x0004xxxx      /* Reserved for Server use */
#define TPM_NV_INDEX_PERIPHERAL         0x0005xxxx      /* Reserved for peripheral use */
#define TPM_NV_INDEX_TSS                0x0006xxxx      /* Reserved for TSS use */
#define TPM_NV_INDEX_GROUP_RESV         0x00xxxxxx      /* Reserved for TCG WG use */
#endif                                 

#define TPM_NV_INDEX_GPIO_00            0x00011600      /* GPIO-Express-00 */

#define TPM_NV_INDEX_GPIO_START         0x00011600      /* Reserved for GPIO pins */
#define TPM_NV_INDEX_GPIO_END           0x000116ff      /* Reserved for GPIO pins */

/* 19.2 TPM_NV_ATTRIBUTES rev 99

   The attributes TPM_NV_PER_AUTHREAD and TPM_NV_PER_OWNERREAD cannot both be set to TRUE.
   Similarly, the attributes TPM_NV_PER_AUTHWRITE and TPM_NV_PER_OWNERWRITE cannot both be set to
   TRUE.
*/

#define TPM_NV_PER_READ_STCLEAR         0x80000000 /* 31: The value can be read until locked by a
                                                      read with a data size of 0.  It can only be
                                                      unlocked by TPM_Startup(ST_Clear) or a
                                                      successful write. Lock held for each area in
                                                      bReadSTClear. */
/* #define 30:19 Reserved */
#define TPM_NV_PER_AUTHREAD             0x00040000 /* 18: The value requires authorization to read
                                                      */
#define TPM_NV_PER_OWNERREAD            0x00020000 /* 17: The value requires TPM Owner authorization
                                                      to read. */
#define TPM_NV_PER_PPREAD               0x00010000 /* 16: The value requires physical presence to
                                                      read */
#define TPM_NV_PER_GLOBALLOCK           0x00008000 /* 15: The value is writable until a write to
                                                      index 0 is successful. The lock of this
                                                      attribute is reset by
                                                      TPM_Startup(ST_CLEAR). Lock held by SF ->
                                                      bGlobalLock */
#define TPM_NV_PER_WRITE_STCLEAR        0x00004000 /* 14: The value is writable until a write to
                                                      the specified index with a datasize of 0 is
                                                      successful. The lock of this attribute is
                                                      reset by TPM_Startup(ST_CLEAR). Lock held for
                                                      each area in bWriteSTClear. */
#define TPM_NV_PER_WRITEDEFINE          0x00002000 /* 13: Lock set by writing to the index with a
                                                      datasize of 0. Lock held for each area in
                                                      bWriteDefine.  This is a persistent lock. */
#define TPM_NV_PER_WRITEALL             0x00001000 /* 12: The value must be written in a single
                                                      operation */
/* #define 11:3 Reserved for write additions */
#define TPM_NV_PER_AUTHWRITE            0x00000004 /* 2: The value requires authorization to write
                                                      */
#define TPM_NV_PER_OWNERWRITE           0x00000002 /* 1: The value requires TPM Owner authorization
                                                      to write */
#define TPM_NV_PER_PPWRITE              0x00000001 /* 0: The value requires physical presence to
                                                      write */

/* 20.2.1 Owner Permission Settings rev 87 */

/* Per1 bits */

#define TPM_DELEGATE_PER1_MASK                          0xffffffff      /* mask of legal bits */
#define TPM_DELEGATE_KeyControlOwner                    31
#define TPM_DELEGATE_SetOrdinalAuditStatus              30
#define TPM_DELEGATE_DirWriteAuth                       29
#define TPM_DELEGATE_CMK_ApproveMA                      28
#define TPM_DELEGATE_NV_WriteValue                      27
#define TPM_DELEGATE_CMK_CreateTicket                   26
#define TPM_DELEGATE_NV_ReadValue                       25
#define TPM_DELEGATE_Delegate_LoadOwnerDelegation       24
#define TPM_DELEGATE_DAA_Join                           23
#define TPM_DELEGATE_AuthorizeMigrationKey              22
#define TPM_DELEGATE_CreateMaintenanceArchive           21
#define TPM_DELEGATE_LoadMaintenanceArchive             20
#define TPM_DELEGATE_KillMaintenanceFeature             19
#define TPM_DELEGATE_OwnerReadInternalPub               18
#define TPM_DELEGATE_ResetLockValue                     17
#define TPM_DELEGATE_OwnerClear                         16
#define TPM_DELEGATE_DisableOwnerClear                  15
#define TPM_DELEGATE_NV_DefineSpace                     14
#define TPM_DELEGATE_OwnerSetDisable                    13
#define TPM_DELEGATE_SetCapability                      12
#define TPM_DELEGATE_MakeIdentity                       11
#define TPM_DELEGATE_ActivateIdentity                   10
#define TPM_DELEGATE_OwnerReadPubek                     9 
#define TPM_DELEGATE_DisablePubekRead                   8 
#define TPM_DELEGATE_SetRedirection                     7 
#define TPM_DELEGATE_FieldUpgrade                       6 
#define TPM_DELEGATE_Delegate_UpdateVerification        5 
#define TPM_DELEGATE_CreateCounter                      4 
#define TPM_DELEGATE_ReleaseCounterOwner                3 
#define TPM_DELEGATE_Delegate_Manage                    2 
#define TPM_DELEGATE_Delegate_CreateOwnerDelegation     1 
#define TPM_DELEGATE_DAA_Sign                           0 

/* Per2 bits */
#define TPM_DELEGATE_PER2_MASK                          0x00000000      /* mask of legal bits */
/* All reserved */

/* 20.2.3 Key Permission settings rev 85 */

/* Per1 bits */

#define TPM_KEY_DELEGATE_PER1_MASK                      0x1fffffff      /* mask of legal bits */
#define TPM_KEY_DELEGATE_CMK_ConvertMigration           28
#define TPM_KEY_DELEGATE_TickStampBlob                  27
#define TPM_KEY_DELEGATE_ChangeAuthAsymStart            26
#define TPM_KEY_DELEGATE_ChangeAuthAsymFinish           25
#define TPM_KEY_DELEGATE_CMK_CreateKey                  24
#define TPM_KEY_DELEGATE_MigrateKey                     23
#define TPM_KEY_DELEGATE_LoadKey2                       22
#define TPM_KEY_DELEGATE_EstablishTransport             21
#define TPM_KEY_DELEGATE_ReleaseTransportSigned         20
#define TPM_KEY_DELEGATE_Quote2                         19
#define TPM_KEY_DELEGATE_Sealx                          18
#define TPM_KEY_DELEGATE_MakeIdentity                   17
#define TPM_KEY_DELEGATE_ActivateIdentity               16
#define TPM_KEY_DELEGATE_GetAuditDigestSigned           15
#define TPM_KEY_DELEGATE_Sign                           14
#define TPM_KEY_DELEGATE_CertifyKey2                    13
#define TPM_KEY_DELEGATE_CertifyKey                     12
#define TPM_KEY_DELEGATE_CreateWrapKey                  11
#define TPM_KEY_DELEGATE_CMK_CreateBlob                 10
#define TPM_KEY_DELEGATE_CreateMigrationBlob            9 
#define TPM_KEY_DELEGATE_ConvertMigrationBlob           8 
#define TPM_KEY_DELEGATE_Delegate_CreateKeyDelegation   7 
#define TPM_KEY_DELEGATE_ChangeAuth                     6 
#define TPM_KEY_DELEGATE_GetPubKey                      5 
#define TPM_KEY_DELEGATE_UnBind                         4 
#define TPM_KEY_DELEGATE_Quote                          3 
#define TPM_KEY_DELEGATE_Unseal                         2 
#define TPM_KEY_DELEGATE_Seal                           1 
#define TPM_KEY_DELEGATE_LoadKey                        0 

/* Per2 bits */
#define TPM_KEY_DELEGATE_PER2_MASK                      0x00000000      /* mask of legal bits */
/* All reserved */

/* 20.3 TPM_FAMILY_FLAGS rev 87

   These flags indicate the operational state of the delegation and family table. These flags
   are additions to TPM_PERMANENT_FLAGS and are not stand alone values.
*/

#define TPM_DELEGATE_ADMIN_LOCK 0x00000002 /* TRUE: Some TPM_Delegate_XXX commands are locked and
                                              return TPM_DELEGATE_LOCK
                                             
                                              FALSE: TPM_Delegate_XXX commands are available

                                              Default is FALSE */
#define TPM_FAMFLAG_ENABLED     0x00000001 /* When TRUE the table is enabled. The default value is
                                              FALSE.  */

/* 20.14 TPM_FAMILY_OPERATION Values rev 87

   These are the opFlag values used by TPM_Delegate_Manage.
*/

#define TPM_FAMILY_CREATE       0x00000001      /* Create a new family */
#define TPM_FAMILY_ENABLE       0x00000002      /* Set or reset the enable flag for this family. */
#define TPM_FAMILY_ADMIN        0x00000003      /* Prevent administration of this family. */
#define TPM_FAMILY_INVALIDATE   0x00000004      /* Invalidate a specific family row. */

/* 21.9 TPM_DA_STATE rev 100
   
   TPM_DA_STATE enumerates the possible states of the dictionary attack mitigation logic.
*/

#define TPM_DA_STATE_INACTIVE   0x00    /* The dictionary attack mitigation logic is currently
                                           inactive */
#define TPM_DA_STATE_ACTIVE     0x01    /* The dictionary attack mitigation logic is
                                           active. TPM_DA_ACTION_TYPE (21.10) is in progress. */

/* 21.10 TPM_DA_ACTION_TYPE rev 100
 */

/* 31-4 Reserved  No information and MUST be FALSE */

#define TPM_DA_ACTION_FAILURE_MODE      0x00000008 /* bit 3: The TPM is in failure mode. */
#define TPM_DA_ACTION_DEACTIVATE        0x00000004 /* bit 2: The TPM is in the deactivated state. */
#define TPM_DA_ACTION_DISABLE           0x00000002 /* bit 1: The TPM is in the disabled state. */
#define TPM_DA_ACTION_TIMEOUT           0x00000001 /* bit 0: The TPM will be in a locked state for
                                                      TPM_DA_INFO -> actionDependValue seconds. This
                                                      value is dynamic, depending on the time the
                                                      lock has been active.  */

/* 22. DAA Structures rev 91
   
   All byte and bit areas are byte arrays treated as large integers
*/

#define DAA_SIZE_r0             43
#define DAA_SIZE_r1             43
#define DAA_SIZE_r2             128
#define DAA_SIZE_r3             168
#define DAA_SIZE_r4             219
#define DAA_SIZE_NT             20
#define DAA_SIZE_v0             128
#define DAA_SIZE_v1             192
#define DAA_SIZE_NE             256
#define DAA_SIZE_w              256
#define DAA_SIZE_issuerModulus  256

/* check that DAA_SIZE_issuerModulus will fit in DAA_scratch */
#if (DAA_SIZE_issuerModulus != 256)
#error "DAA_SIZE_issuerModulus must be 256"
#endif

/* 22.2 Constant definitions rev 91 */

#define DAA_power0      104  
#define DAA_power1      1024  

#endif
