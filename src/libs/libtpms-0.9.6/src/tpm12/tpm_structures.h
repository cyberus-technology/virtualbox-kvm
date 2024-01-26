/********************************************************************************/
/*                                                                              */
/*                              TPM Structures                                  */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_structures.h $        */
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

#ifndef TPM_STRUCTURES_H
#define TPM_STRUCTURES_H

#include <limits.h>
#include "tpm_constants.h"
#include "tpm_memory.h"
#include "tpm_types.h"
#include "tpm_nvram_const.h"

/* Sanity check on build macros are centralized here, since any TPM will use this header */

#if !defined (TPM_POSIX) && !defined (TPM_WINDOWS) && !defined(TPM_SYSTEM_P)
#error "Must define either TPM_POSIX or TPM_WINDOWS or TPM_SYSTEM_P"
#endif

#if defined (TPM_NV_XCRYPTO_FLASH) && defined (TPM_NV_DISK)
#error "Cannot define TPM_NV_XCRYPTO_FLASH and TPM_NV_DISK"
#endif

#if defined (TPM_WINDOWS) && defined (TPM_UNIX_DOMAIN_SOCKET)
#error "Cannot define TPM_WINDOWS and TPM_UNIX_DOMAIN_SOCKET"
#endif

#if defined (TPM_USE_CHARDEV) && defined (TPM_UNIX_DOMAIN_SOCKET)
#error "Cannot define TPM_USE_CHARDEV and TPM_UNIX_DOMAIN_SOCKET"
#endif

#if defined (TPM_NV_XCRYPTO_FLASH) && defined (TPM_UNIX_DOMAIN_SOCKET)
#error "Cannot define TPM_NV_XCRYPTO_FLASH and TPM_UNIX_DOMAIN_SOCKET"
#endif

#if defined (TPM_XCRYPTO_USE_HW) && !defined(TPM_NV_XCRYPTO_FLASH)
#error "TPM_XCRYPTO_USE_HW requires TPM_NV_XCRYPTO_FLASH"
#endif

#if defined (TPM_VTPM) && defined (TPM_UNIX_DOMAIN_SOCKET)
#error "Cannot define TPM_VTPM and TPM_UNIX_DOMAIN_SOCKET"
#endif



#if defined (TPM_V11) && defined (TPM_V12)
#error "Cannot define TPM_V12 and TPM_V11"
#endif

#if !defined (TPM_V11) && !defined (TPM_V12)
#error "Must define either TPM_V12 or TPM_V11"
#endif

#if defined (TPM_DES) && defined (TPM_AES)
#error "Cannot define TPM_DES and TPM_AES"
#endif
#if !defined (TPM_DES) && !defined (TPM_AES)
#error "Must define either TPM_DES or TPM_AES"
#endif

/* This structure is typically a cast from a subset of a larger TPM structure.  Two members - a 4
   bytes size followed by a 4 bytes pointer to the data is a common TPM structure idiom. */

typedef struct tdTPM_SIZED_BUFFER {
    uint32_t size;
    BYTE *buffer;
} TPM_SIZED_BUFFER;

/* This structure implements a safe storage buffer, used throughout the code when serializing
   structures to a stream.
*/

typedef struct tdTPM_STORE_BUFFER {
    unsigned char *buffer;              /* beginning of buffer */
    unsigned char *buffer_current;      /* first empty position in buffer */
    unsigned char *buffer_end;          /* one past last valid position in buffer */
} TPM_STORE_BUFFER;

/* 5.1 TPM_STRUCT_VER rev 100

   This indicates the version of the structure or TPM. 

   Version 1.2 deprecates the use of this structure in all other structures. The structure is not
   deprecated as many of the structures that contain this structure are not deprecated.
*/

#define TPM_MAJOR       0x01

#if defined TPM_V12
#define TPM_MINOR       0x02
#endif

#if defined TPM_V11
#define TPM_MINOR       0x01
#endif

typedef struct tdTPM_STRUCT_VER { 
    BYTE major;         /* This SHALL indicate the major version of the structure. MUST be 0x01 */
    BYTE minor;         /* This SHALL indicate the minor version of the structure. MUST be 0x01 */
    BYTE revMajor;      /* This MUST be 0x00 on output, ignored on input */
    BYTE revMinor;      /* This MUST be 0x00 on output, ignored on input */
} TPM_STRUCT_VER; 

/* 5.2 TPM_VERSION_BYTE rev 87

   Allocating a byte for the version information is wasteful of space. The current allocation does
   not provide sufficient resolution to indicate completely the version of the TPM. To allow for
   backwards compatibility the size of the structure does not change from 1.1.
   
   To enable minor version, or revision, numbers with 2-digit resolution, the byte representing a
   version splits into two BDC encoded nibbles. The ordering of the low and high order provides
   backwards compatibility with existing numbering.
   
   An example of an implementation of this is; a version of 1.23 would have the value 2 in bit
   positions 3-0 and the value 3 in bit positions 7-4.

   TPM_VERSION_BYTE is a byte. The byte is broken up according to the following rule

   7-4 leastSigVer Least significant nibble of the minor version. MUST be values within the range of
        0000-1001
   3-0 mostSigVer Most significant nibble of the minor version. MUST be values within the range of
        0000-1001
*/

/* 5.3 TPM_VERSION rev 116

   This structure provides information relative the version of the TPM. This structure should only
   be in use by TPM_GetCapability to provide the information relative to the TPM.
*/

typedef struct tdTPM_VERSION { 
    TPM_VERSION_BYTE major;     /* This SHALL indicate the major version of the TPM, mostSigVer MUST
                                   be 0x1, leastSigVer MUST be 0x0 */
    TPM_VERSION_BYTE minor;     /* This SHALL indicate the minor version of the TPM, mostSigVer MUST
                                   be 0x1 or 0x2, leastSigVer MUST be 0x0 */
    BYTE revMajor;              /* This SHALL be the value of the TPM_PERMANENT_DATA -> revMajor */
    BYTE revMinor;              /* This SHALL be the value of the TPM_PERMANENT_DATA -> revMinor */
} TPM_VERSION; 

/* 5.4 TPM_DIGEST rev 111

   The digest value reports the result of a hash operation.

   In version 1 the hash algorithm is SHA-1 with a resulting hash result being 20 bytes or 160 bits.

   It is understood that algorithm agility is lost due to fixing the hash at 20 bytes and on
   SHA-1. The reason for fixing is due to the internal use of the digest. It is the authorization
   values, it provides the secrets for the HMAC and the size of 20 bytes determines the values that
   can be stored and encrypted. For this reason, the size is fixed and any changes to this value
   require a new version of the specification.

   The digestSize parameter MUST indicate the block size of the algorithm and MUST be 20 or greater.

   For all TPM v1 hash operations, the hash algorithm MUST be SHA-1 and the digestSize parameter is
   therefore equal to 20.
*/

#define TPM_DIGEST_SIZE 20
typedef BYTE TPM_DIGEST[TPM_DIGEST_SIZE];

#if 0
/* kgold - This was designed as a structure with one element.  Changed to a simple BYTE array, like
   TPM_SECRET. */
typedef struct tdTPM_DIGEST {
    BYTE digest[TPM_DIGEST_SIZE];       /* This SHALL be the actual digest information */
} TPM_DIGEST;
#endif

/* Redefinitions */

typedef TPM_DIGEST TPM_CHOSENID_HASH;   /* This SHALL be the digest of the chosen identityLabel and
                                           privacyCA for a new TPM identity.*/

typedef TPM_DIGEST TPM_COMPOSITE_HASH;  /* This SHALL be the hash of a list of PCR indexes and PCR
                                           values that a key or data is bound to. */

typedef TPM_DIGEST TPM_DIRVALUE;        /* This SHALL be the value of a DIR register */

typedef TPM_DIGEST TPM_HMAC;            /* This shall be the output of the HMAC algorithm */

typedef TPM_DIGEST TPM_PCRVALUE;        /* The value inside of the PCR */

typedef TPM_DIGEST TPM_AUDITDIGEST;     /* This SHALL be the value of the current internal audit
                                           state */

/* 5.5 TPM_NONCE rev 99

   A nonce is a random value that provides protection from replay and other attacks.  Many of the
   commands and protocols in the specification require a nonce. This structure provides a consistent
   view of what a nonce is.
*/

#define TPM_NONCE_SIZE 20
typedef BYTE TPM_NONCE[TPM_NONCE_SIZE];

#if 0
/* kgold - This was designed as a structure with one element.  Changed to a simple BYTE array, like
   TPM_SECRET. */
typedef struct tdTPM_NONCE { 
    BYTE nonce[TPM_NONCE_SIZE];  /* This SHALL be the 20 bytes of random data. When created by the
                                    TPM the value MUST be the next 20 bytes from the RNG */
} TPM_NONCE;
#endif

typedef TPM_NONCE TPM_DAA_TPM_SEED;     /* This SHALL be a random value generated by a TPM
                                           immediately after the EK is installed in that TPM,
                                           whenever an EK is installed in that TPM */
typedef TPM_NONCE TPM_DAA_CONTEXT_SEED; /* This SHALL be a random value */

/* 5.6 TPM_AUTHDATA rev 87

   The authorization data is the information that is saved or passed to provide proof of ownership
   of an entity.  For version 1 this area is always 20 bytes.
*/

#define TPM_AUTHDATA_SIZE 20
typedef BYTE TPM_AUTHDATA[TPM_AUTHDATA_SIZE];

#define TPM_SECRET_SIZE 20
typedef BYTE TPM_SECRET[TPM_SECRET_SIZE];

#if 0   /* kgold - define TPM_SECRET directly, so the size can be defined */
typedef TPM_AUTHDATA TPM_SECRET; /* A secret plain text value used in the authorization process. */
#endif

typedef TPM_AUTHDATA TPM_ENCAUTH; /* A cipher text (encrypted) version of authorization data. The
                                     encryption mechanism depends on the context. */

/* 5.7 TPM_KEY_HANDLE_LIST rev 87

   TPM_KEY_HANDLE_LIST is a structure used to describe the handles of all keys currently loaded into
   a TPM.
*/

#if 0   /* This is the version from the specification part 2 */
typedef struct tdTPM_KEY_HANDLE_LIST {
    uint16_t loaded;                      /* The number of keys currently loaded in the TPM. */
    [size_is(loaded)] TPM_KEY_HANDLE handle[];  /* An array of handles, one for each key currently
                                                   loaded in the TPM */
} TPM_KEY_HANDLE_LIST; 
#endif

/* 5.11 TPM_CHANGEAUTH_VALIDATE rev 87

   This structure provides an area that will stores the new authorization data and the challenger's
   nonce.
*/

typedef struct tdTPM_CHANGEAUTH_VALIDATE { 
    TPM_SECRET newAuthSecret;   /* This SHALL be the new authorization data for the target entity */
    TPM_NONCE n1;               /* This SHOULD be a nonce, to enable the caller to verify that the
                                   target TPM is on-line. */
} TPM_CHANGEAUTH_VALIDATE; 



/* PCR */

/* NOTE: The TPM requires and the code assumes a multiple of CHAR_BIT (8).  48 registers (6 bytes)
   may be a bad number, as it makes TPM_PCR_INFO and TPM_PCR_INFO_LONG indistinguishable in the
   first two bytes. */

#if defined TPM_V11
#define TPM_NUM_PCR 16          /* Use PC Client specification values */
#endif

#if defined TPM_V12
#define TPM_NUM_PCR 24          /* Use PC Client specification values */
#endif

#if (CHAR_BIT != 8)
#error "CHAR_BIT must be 8"
#endif

#if ((TPM_NUM_PCR % 8) != 0)
#error "TPM_NUM_PCR must be a multiple of 8"
#endif

/* 8.1 TPM_PCR_SELECTION rev 110

   This structure provides a standard method of specifying a list of PCR registers.
*/

typedef struct tdTPM_PCR_SELECTION { 
    uint16_t sizeOfSelect;			/* The size in bytes of the pcrSelect structure */
    BYTE pcrSelect[TPM_NUM_PCR/CHAR_BIT];       /* This SHALL be a bit map that indicates if a PCR
                                                   is active or not */
} TPM_PCR_SELECTION; 

/* 8.2 TPM_PCR_COMPOSITE rev 97

   The composite structure provides the index and value of the PCR register to be used when creating
   the value that SEALS an entity to the composite.
*/

typedef struct tdTPM_PCR_COMPOSITE { 
    TPM_PCR_SELECTION select;   /* This SHALL be the indication of which PCR values are active */
#if 0
    uint32_t valueSize;           /* This SHALL be the size of the pcrValue field (not the number of
				     PCR's) */
    TPM_PCRVALUE *pcrValue;     /* This SHALL be an array of TPM_PCRVALUE structures. The values
                                   come in the order specified by the select parameter and are
                                   concatenated into a single blob */
#endif
    TPM_SIZED_BUFFER pcrValue;
} TPM_PCR_COMPOSITE; 

/* 8.3 TPM_PCR_INFO rev 87 

   The TPM_PCR_INFO structure contains the information related to the wrapping of a key or the
   sealing of data, to a set of PCRs.
*/

typedef struct tdTPM_PCR_INFO { 
    TPM_PCR_SELECTION pcrSelection;             /* This SHALL be the selection of PCRs to which the
                                                   data or key is bound. */
    TPM_COMPOSITE_HASH digestAtRelease;         /* This SHALL be the digest of the PCR indices and
                                                   PCR values to verify when revealing Sealed Data
                                                   or using a key that was wrapped to PCRs.  NOTE:
                                                   This is passed in by the host, and used as
                                                   authorization to use the key */
    TPM_COMPOSITE_HASH digestAtCreation;        /* This SHALL be the composite digest value of the
                                                   PCR values, at the time when the sealing is
                                                   performed. NOTE: This is generated at key
                                                   creation, but is just informative to the host,
                                                   not used for authorization */
} TPM_PCR_INFO; 

/* 8.6 TPM_LOCALITY_SELECTION rev 87 

   When used with localityAtCreation only one bit is set and it corresponds to the locality of the
   command creating the structure.

   When used with localityAtRelease the bits indicate which localities CAN perform the release.
*/

typedef BYTE TPM_LOCALITY_SELECTION;

#define TPM_LOC_FOUR    0x10    /* Locality 4 */
#define TPM_LOC_THREE   0x08    /* Locality 3  */
#define TPM_LOC_TWO     0x04    /* Locality 2  */
#define TPM_LOC_ONE     0x02    /* Locality 1  */
#define TPM_LOC_ZERO    0x01    /* Locality 0. This is the same as the legacy interface.  */

#define TPM_LOC_ALL     0x1f    /* kgold - added all localities */
#define TPM_LOC_MAX     4       /* kgold - maximum value for TPM_MODIFIER_INDICATOR */


/* 8.4 TPM_PCR_INFO_LONG rev 109

   The TPM_PCR_INFO structure contains the information related to the wrapping of a key or the
   sealing of data, to a set of PCRs.

   The LONG version includes information necessary to properly define the configuration that creates
   the blob using the PCR selection.
*/

typedef struct tdTPM_PCR_INFO_LONG { 
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;                      /* This SHALL be TPM_TAG_PCR_INFO_LONG  */
#endif
    TPM_LOCALITY_SELECTION localityAtCreation;  /* This SHALL be the locality modifier of the
                                                   function that creates the PCR info structure */
    TPM_LOCALITY_SELECTION localityAtRelease;   /* This SHALL be the locality modifier required to
                                                   reveal Sealed Data or use a key that was wrapped
                                                   to PCRs */
    TPM_PCR_SELECTION creationPCRSelection;     /* This SHALL be the selection of PCRs active when
                                                   the blob is created */
    TPM_PCR_SELECTION releasePCRSelection;      /* This SHALL be the selection of PCRs to which the
                                                   data or key is bound. */
    TPM_COMPOSITE_HASH digestAtCreation;        /* This SHALL be the composite digest value of the
                                                   PCR values, at the time when the sealing is
                                                   performed. */
    TPM_COMPOSITE_HASH digestAtRelease;         /* This SHALL be the digest of the PCR indices and
                                                   PCR values to verify when revealing Sealed Data
                                                   or using a key that was wrapped to PCRs. */
} TPM_PCR_INFO_LONG; 

/* 8.5 TPM_PCR_INFO_SHORT rev 87

   This structure is for defining a digest at release when the only information that is necessary is
   the release configuration.
*/

typedef struct tdTPM_PCR_INFO_SHORT { 
    TPM_PCR_SELECTION pcrSelection;     /* This SHALL be the selection of PCRs that specifies the
                                           digestAtRelease */
    TPM_LOCALITY_SELECTION localityAtRelease;   /* This SHALL be the locality modifier required to
                                                   release the information.  This value must not be
                                                   zero (0). */
    TPM_COMPOSITE_HASH digestAtRelease;         /* This SHALL be the digest of the PCR indices and
                                                   PCR values to verify when revealing auth data */
} TPM_PCR_INFO_SHORT; 

/* 8.8 TPM_PCR_ATTRIBUTES rev 107

   These attributes are available on a per PCR basis.

   The TPM is not required to maintain this structure internally to the TPM.

   When a challenger evaluates a PCR an understanding of this structure is vital to the proper
   understanding of the platform configuration. As this structure is static for all platforms of the
   same type the structure does not need to be reported with each quote.
*/

typedef struct tdTPM_PCR_ATTRIBUTES { 
    TPM_BOOL pcrReset;          /* A value of TRUE SHALL indicate that the PCR register can be reset
                                   using the TPM_PCR_RESET command. */
    TPM_LOCALITY_SELECTION pcrExtendLocal;      /* An indication of which localities can perform
                                                   extends on the PCR. */
    TPM_LOCALITY_SELECTION pcrResetLocal;       /* An indication of which localities can reset the
                                                   PCR */
} TPM_PCR_ATTRIBUTES; 

/*
  9. Storage Structures 
*/

/* 9.1 TPM_STORED_DATA rev 87 

   The definition of this structure is necessary to ensure the enforcement of security properties.
   
   This structure is in use by the TPM_Seal and TPM_Unseal commands to identify the PCR index and
   values that must be present to properly unseal the data.

   This structure only provides 1.1 data store and uses PCR_INFO

   1. This structure is created during the TPM_Seal process. The confidential data is encrypted
   using a nonmigratable key. When the TPM_Unseal decrypts this structure the TPM_Unseal uses the
   public information in the structure to validate the current configuration and release the
   decrypted data

   2. When sealInfoSize is not 0 sealInfo MUST be TPM_PCR_INFO
*/

typedef struct tdTPM_STORED_DATA { 
    TPM_STRUCT_VER ver;         /* This MUST be 1.1.0.0  */
    TPM_SIZED_BUFFER sealInfo;
#if 0
    uint32_t sealInfoSize;	/* Size of the sealInfo parameter */
    BYTE* sealInfo;             /* This SHALL be a structure of type TPM_PCR_INFO or a 0 length
                                   array if the data is not bound to PCRs. */
#endif
    TPM_SIZED_BUFFER encData;
#if 0
    uint32_t encDataSize;	/* This SHALL be the size of the encData parameter */
    BYTE* encData;              /* This shall be an encrypted TPM_SEALED_DATA structure containing
                                   the confidential part of the data. */
#endif
    /* NOTE: kgold - Added this structure, a cache of PCRInfo when not NULL */
    TPM_PCR_INFO *tpm_seal_info;
} TPM_STORED_DATA; 


/* 9.2 TPM_STORED_DATA12 rev 101

   The definition of this structure is necessary to ensure the enforcement of security properties.
   This structure is in use by the TPM_Seal and TPM_Unseal commands to identify the PCR index and
   values that must be present to properly unseal the data.

   1. This structure is created during the TPM_Seal process. The confidential data is encrypted
   using a nonmigratable key. When the TPM_Unseal decrypts this structure the TPM_Unseal uses the
   public information in the structure to validate the current configuration and release the
   decrypted data.

   2. If sealInfoSize is not 0 then sealInfo MUST be TPM_PCR_INFO_LONG
*/

typedef struct tdTPM_STORED_DATA12 { 
    TPM_STRUCTURE_TAG tag;      /* This SHALL be TPM_TAG_STORED_DATA12 */
    TPM_ENTITY_TYPE et;         /* The type of blob */
    TPM_SIZED_BUFFER sealInfo;
#if 0
    uint32_t sealInfoSize;	/* Size of the sealInfo parameter */
    BYTE* sealInfo;             /* This SHALL be a structure of type TPM_PCR_INFO_LONG or a 0 length
                                   array if the data is not bound to PCRs. */
#endif
    TPM_SIZED_BUFFER encData;
#if 0
    uint32_t encDataSize;	/* This SHALL be the size of the encData parameter */
    BYTE* encData;              /* This shall be an encrypted TPM_SEALED_DATA structure containing
                                   the confidential part of the data. */
#endif
    /* NOTE: kgold - Added this structure, a cache of PCRInfo when not NULL */
    TPM_PCR_INFO_LONG *tpm_seal_info_long;
} TPM_STORED_DATA12; 

/* 9.3 TPM_SEALED_DATA rev 87 

   This structure contains confidential information related to sealed data, including the data
   itself.

   1. To tie the TPM_STORED_DATA structure to the TPM_SEALED_DATA structure this structure contains
   a digest of the containing TPM_STORED_DATA structure.

   2. The digest calculation does not include the encDataSize and encData parameters.
*/

typedef struct tdTPM_SEALED_DATA { 
    TPM_PAYLOAD_TYPE payload;   /* This SHALL indicate the payload type of TPM_PT_SEAL */
    TPM_SECRET authData;        /* This SHALL be the authorization data for this value */
    TPM_SECRET tpmProof;        /* This SHALL be a copy of TPM_PERMANENT_FLAGS -> tpmProof */
    TPM_DIGEST storedDigest;    /* This SHALL be a digest of the TPM_STORED_DATA structure,
                                   excluding the fields TPM_STORED_DATA -> encDataSize and
                                   TPM_STORED_DATA -> encData.  */
    TPM_SIZED_BUFFER data;      /* This SHALL be the data to be sealed */
#if 0
    uint32_t dataSize;		/* This SHALL be the size of the data parameter */
    BYTE* data;                 /* This SHALL be the data to be sealed */
#endif
} TPM_SEALED_DATA; 


/* 9.4 TPM_SYMMETRIC_KEY rev 87 

   This structure describes a symmetric key, used during the process "Collating a Request for a
   Trusted Platform Module Identity".
*/

typedef struct tdTPM_SYMMETRIC_KEY { 
    TPM_ALGORITHM_ID algId;     /* This SHALL be the algorithm identifier of the symmetric key. */
    TPM_ENC_SCHEME encScheme;   /* This SHALL fully identify the manner in which the key will be
                                   used for encryption operations.  */
    uint16_t size;		/* This SHALL be the size of the data parameter in bytes */
    BYTE* data;                 /* This SHALL be the symmetric key data */
    /* NOTE Cannot make this a TPM_SIZED_BUFFER because uint16_t */
} TPM_SYMMETRIC_KEY; 

/* 9.5 TPM_BOUND_DATA rev 87 

   This structure is defined because it is used by a TPM_UnBind command in a consistency check.

   The intent of TCG is to promote "best practice" heuristics for the use of keys: a signing key
   shouldn't be used for storage, and so on. These heuristics are used because of the potential
   threats that arise when the same key is used in different ways. The heuristics minimize the
   number of ways in which a given key can be used.

   One such heuristic is that a key of type TPM_KEY_BIND, and no other type of key, should always be
   used to create the blob that is unwrapped by TPM_UnBind. Binding is not a TPM function, so the
   only choice is to perform a check for the correct payload type when a blob is unwrapped by a key
   of type TPM_KEY_BIND. This requires the blob to have internal structure.

   Even though payloadData has variable size, TPM_BOUND_DATA deliberately does not include the size
   of payloadData. This is to maximise the size of payloadData that can be encrypted when
   TPM_BOUND_DATA is encrypted in a single block. When using TPM-UnBind to obtain payloadData, the
   size of payloadData is deduced as a natural result of the (RSA) decryption process.

   1. This structure MUST be used for creating data when (wrapping with a key of type TPM_KEY_BIND)
   or (wrapping using the encryption algorithm TPM_ES_RSAESOAEP_SHA1_MGF1). If it is not, the
   TPM_UnBind command will fail.
*/

typedef struct tdTPM_BOUND_DATA { 
    TPM_STRUCT_VER ver;                 /* This MUST be 1.1.0.0  */
    TPM_PAYLOAD_TYPE payload;           /* This SHALL be the value TPM_PT_BIND  */
    uint32_t payloadDataSize;		/* NOTE: added, not part of serialization */
    BYTE *payloadData;                  /* The bound data */
} TPM_BOUND_DATA; 

/*
  10. TPM_KEY Complex
*/

/* 10.1.1 TPM_RSA_KEY_PARMS rev 87 

   This structure describes the parameters of an RSA key.
*/

/* TPM_RSA_KEY_LENGTH_MAX restricts the maximum size of an RSA key.  It has two uses:
   - bounds the size of the TPM state
   - protects against a denial of service attack where the attacker creates a very large key
*/

#ifdef TPM_RSA_KEY_LENGTH_MAX		/* if the builder defines a value */
#if ((TPM_RSA_KEY_LENGTH_MAX % 16) != 0)
#error "TPM_RSA_KEY_LENGTH_MAX must be a multiple of 16"
#endif
#if (TPM_RSA_KEY_LENGTH_MAX < 2048)
#error "TPM_RSA_KEY_LENGTH_MAX must be at least 2048"
#endif
#endif		/* TPM_RSA_KEY_LENGTH_MAX */

#ifndef TPM_RSA_KEY_LENGTH_MAX		/* default if the builder does not define a value */
#define TPM_RSA_KEY_LENGTH_MAX 2048
#endif

typedef struct tdTPM_RSA_KEY_PARMS { 
    uint32_t keyLength;   /* This specifies the size of the RSA key in bits */
    uint32_t numPrimes;   /* This specifies the number of prime factors used by this RSA key. */
#if 0
    uint32_t exponentSize;	/* This SHALL be the size of the exponent. If the key is using the
                                   default exponent then the exponentSize MUST be 0. */
    BYTE   *exponent;   	/* The public exponent of this key */
#endif
    TPM_SIZED_BUFFER exponent;  /* The public exponent of this key */

} TPM_RSA_KEY_PARMS; 


/* 10.1 TPM_KEY_PARMS rev 87

   This provides a standard mechanism to define the parameters used to generate a key pair, and to
   store the parts of a key shared between the public and private key parts.
*/

typedef struct tdTPM_KEY_PARMS { 
    TPM_ALGORITHM_ID algorithmID;       /* This SHALL be the key algorithm in use */
    TPM_ENC_SCHEME encScheme;   /* This SHALL be the encryption scheme that the key uses to encrypt
                                   information */
    TPM_SIG_SCHEME sigScheme;   /* This SHALL be the signature scheme that the key uses to perform
                                   digital signatures */
#if 0
    uint32_t parmSize;		/* This SHALL be the size of the parms field in bytes */
    BYTE* parms;                /* This SHALL be the parameter information dependent upon the key
                                   algorithm. */
#endif
    TPM_SIZED_BUFFER parms;     /* This SHALL be the parameter information dependent upon the key
                                   algorithm. */
    /* NOTE: kgold - Added this structure.  It acts as a cache of the result of parms and parmSize
       deserialization when non-NULL.  */
    TPM_RSA_KEY_PARMS *tpm_rsa_key_parms;
} TPM_KEY_PARMS; 

/* 10.1.2 TPM_SYMMETRIC_KEY_PARMS rev 87

   This structure describes the parameters for symmetric algorithms 
*/

typedef struct tdTPM_SYMMETRIC_KEY_PARMS { 
    uint32_t keyLength;	/* This SHALL indicate the length of the key in bits */
    uint32_t blockSize;	/* This SHALL indicate the block size of the algorithm*/
    uint32_t ivSize;	/* This SHALL indicate the size of the IV */
    BYTE *IV;		/* The initialization vector */
} TPM_SYMMETRIC_KEY_PARMS; 

#if 0
/* 10.4 TPM_STORE_PUBKEY rev 99

   This structure can be used in conjunction with a corresponding TPM_KEY_PARMS to construct a
   public key which can be unambiguously used.
*/

typedef struct tdTPM_STORE_PUBKEY { 
    uint32_t keyLength;	/* This SHALL be the length of the key field. */
    BYTE   *key;        /* This SHALL be a structure interpreted according to the algorithm Id in
                           the corresponding TPM_KEY_PARMS structure. */
} TPM_STORE_PUBKEY; 
#endif

/* 10.7 TPM_STORE_PRIVKEY rev 87

   This structure can be used in conjunction with a corresponding TPM_PUBKEY to construct a private
   key which can be unambiguously used.
*/

#if 0
typedef struct tdTPM_STORE_PRIVKEY { 
    uint32_t keyLength;	/* This SHALL be the length of the key field. */
    BYTE* key;          /* This SHALL be a structure interpreted according to the algorithm Id in
                           the corresponding TPM_KEY structure. */
} TPM_STORE_PRIVKEY; 
#endif

/* NOTE: Hard coded for RSA keys.  This will change if other algorithms are supported */

typedef struct tdTPM_STORE_PRIVKEY { 
    TPM_SIZED_BUFFER d_key;             /* private key */
    TPM_SIZED_BUFFER p_key;             /* private prime factor */
    TPM_SIZED_BUFFER q_key;             /* private prime factor */
} TPM_STORE_PRIVKEY; 

/* 10.6 TPM_STORE_ASYMKEY rev 87

   The TPM_STORE_ASYMKEY structure provides the area to identify the confidential information
   related to a key.  This will include the private key factors for an asymmetric key.

   The structure is designed so that encryption of a TPM_STORE_ASYMKEY structure containing a 2048
   bit RSA key can be done in one operation if the encrypting key is 2048 bits.

   Using typical RSA notation the structure would include P, and when loading the key include the
   unencrypted P*Q which would be used to recover the Q value.

   To accommodate the future use of multiple prime RSA keys the specification of additional prime
   factors is an optional capability.

   This structure provides the basis of defining the protection of the private key.  Changes in this
   structure MUST be reflected in the TPM_MIGRATE_ASYMKEY structure (section 10.8).
*/

typedef struct tdTPM_STORE_ASYMKEY {    
    TPM_PAYLOAD_TYPE payload;           /* This SHALL set to TPM_PT_ASYM to indicate an asymmetric
                                           key. If used in TPM_CMK_ConvertMigration the value SHALL
                                           be TPM_PT_MIGRATE_EXTERNAL. If used in TPM_CMK_CreateKey
                                           the value SHALL be TPM_PT_MIGRATE_RESTRICTED  */
    TPM_SECRET usageAuth;               /* This SHALL be the authorization data necessary to
                                           authorize the use of this value */
    TPM_SECRET migrationAuth;           /* This SHALL be the migration authorization data for a
                                           migratable key, or the TPM secret value tpmProof for a
                                           non-migratable key created by the TPM.

                                           If the TPM sets this parameter to the value tpmProof,
                                           then the TPM_KEY.keyFlags.migratable of the corresponding
                                           TPM_KEY structure MUST be set to 0.

                                           If this parameter is set to the migration authorization
                                           data for the key in parameter PrivKey, then the
                                           TPM_KEY.keyFlags.migratable of the corresponding TPM_KEY
                                           structure SHOULD be set to 1. */
    TPM_DIGEST pubDataDigest;           /* This SHALL be the digest of the corresponding TPM_KEY
                                           structure, excluding the fields TPM_KEY.encSize and
                                           TPM_KEY.encData.

                                           When TPM_KEY -> pcrInfoSize is 0 then the digest
                                           calculation has no input from the pcrInfo field. The
                                           pcrInfoSize field MUST always be part of the digest
                                           calculation.
                                        */
    TPM_STORE_PRIVKEY privKey;          /* This SHALL be the private key data. The privKey can be a
                                           variable length which allows for differences in the key
                                           format. The maximum size of the area would be 151
                                           bytes. */
} TPM_STORE_ASYMKEY;            

/* 10.8 TPM_MIGRATE_ASYMKEY rev 87

   The TPM_MIGRATE_ASYMKEY structure provides the area to identify the private key factors of a
   asymmetric key while the key is migrating between TPM's.

   This structure provides the basis of defining the protection of the private key.

   k1k2 - 132 privkey.key (128 + 4)
   k1 - 20, OAEP seed
   k2 - 112, partPrivKey
   TPM_STORE_PRIVKEY 4 partPrivKey.keyLength
                     108 partPrivKey.key (128 - 20)
*/

typedef struct tdTPM_MIGRATE_ASYMKEY {
    TPM_PAYLOAD_TYPE payload;   /* This SHALL set to TPM_PT_MIGRATE or TPM_PT_CMK_MIGRATE to
                                   indicate an migrating asymmetric key or TPM_PT_MAINT to indicate
                                   a maintenance key. */
    TPM_SECRET usageAuth;       /* This SHALL be a copy of the usageAuth from the TPM_STORE_ASYMKEY
                                   structure. */
    TPM_DIGEST pubDataDigest;   /* This SHALL be a copy of the pubDataDigest from the
                                   TPM_STORE_ASYMKEY structure. */
#if 0
    uint32_t partPrivKeyLen;	/* This SHALL be the size of the partPrivKey field */
    BYTE *partPrivKey;          /* This SHALL be the k2 area as described in TPM_CreateMigrationBlob
                                   */
#endif
    TPM_SIZED_BUFFER partPrivKey;
} TPM_MIGRATE_ASYMKEY; 

/* 10.2 TPM_KEY rev 87 

   The TPM_KEY structure provides a mechanism to transport the entire asymmetric key pair. The
   private portion of the key is always encrypted.

   The reason for using a size and pointer for the PCR info structure is save space when the key is
   not bound to a PCR. The only time the information for the PCR is kept with the key is when the
   key needs PCR info.

   The 1.2 version has a change in the PCRInfo area. For 1.2 the structure uses the
   TPM_PCR_INFO_LONG structure to properly define the PCR registers in use.
*/

typedef struct tdTPM_KEY { 
    TPM_STRUCT_VER ver;         /* This MUST be 1.1.0.0 */
    TPM_KEY_USAGE keyUsage;     /* This SHALL be the TPM key usage that determines the operations
                                   permitted with this key */
    TPM_KEY_FLAGS keyFlags;     /* This SHALL be the indication of migration, redirection etc.*/
    TPM_AUTH_DATA_USAGE authDataUsage;  /* This SHALL Indicate the conditions where it is required
                                           that authorization be presented.*/
    TPM_KEY_PARMS algorithmParms;       /* This SHALL be the information regarding the algorithm for
                                           this key*/
#if 0
    uint32_t PCRInfoSize;	/* This SHALL be the length of the pcrInfo parameter. If the key is
                                   not bound to a PCR this value SHOULD be 0.*/
    BYTE* PCRInfo;              /* This SHALL be a structure of type TPM_PCR_INFO, or an empty array
                                   if the key is not bound to PCRs.*/
    TPM_STORE_PUBKEY pubKey;    /* This SHALL be the public portion of the key */
    uint32_t encDataSize;	/* This SHALL be the size of the encData parameter. */
    BYTE* encData;              /* This SHALL be an encrypted TPM_STORE_ASYMKEY structure or
                                   TPM_MIGRATE_ASYMKEY structure */
#endif
    TPM_SIZED_BUFFER pcrInfo;
    TPM_SIZED_BUFFER pubKey;
    TPM_SIZED_BUFFER encData;
    /* This SHALL be an encrypted TPM_STORE_ASYMKEY structure or TPM_MIGRATE_ASYMKEY structure */
    /* NOTE: kgold - Added these structures, a cache of PCRInfo when not NULL */
    TPM_PCR_INFO *tpm_pcr_info;                 /* for TPM_KEY */
    TPM_PCR_INFO_LONG *tpm_pcr_info_long;       /* for TPM_KEY12 */
    /* NOTE: kgold - Added these structures.  They act as a cache of the result of encData
       decryption when non-NULL.  In the case of internal keys (e.g. SRK) there is no encData, so
       these structures are always non-NULL. */
    TPM_STORE_ASYMKEY *tpm_store_asymkey;
    TPM_MIGRATE_ASYMKEY *tpm_migrate_asymkey;
} TPM_KEY; 

/* 10.3 TPM_KEY12 rev 87

   This provides the same functionality as TPM_KEY but uses the new PCR_INFO_LONG structures and the
   new structure tagging. In all other aspects this is the same structure.
*/

/* NOTE: The TPM_KEY12 structure is never instantiated.  It is just needed for the cast of TPM_KEY
   to get the TPM_KEY12->tag member. */

typedef struct tdTPM_KEY12 { 
    TPM_STRUCTURE_TAG tag;      /* MUST be TPM_TAG_KEY12 */
    uint16_t fill;		/* MUST be 0x0000 */
    TPM_KEY_USAGE keyUsage;     /* This SHALL be the TPM key usage that determines the operations
                                   permitted with this key */
    TPM_KEY_FLAGS keyFlags;     /* This SHALL be the indication of migration, redirection etc. */
    TPM_AUTH_DATA_USAGE authDataUsage;  /* This SHALL Indicate the conditions where it is required
                                           that authorization be presented. */
    TPM_KEY_PARMS algorithmParms;       /* This SHALL be the information regarding the algorithm for
                                           this key */
#if 0
    uint32_t PCRInfoSize;	/* This SHALL be the length of the pcrInfo parameter. If the key is
                                   not bound to a PCR this value SHOULD be 0. */
    BYTE* PCRInfo;              /* This SHALL be a structure of type TPM_PCR_INFO_LONG, or an empty
                                   array if the key is not bound to PCRs. */
    TPM_STORE_PUBKEY pubKey;    /* This SHALL be the public portion of the key */
    uint32_t encDataSize;	/* This SHALL be the size of the encData parameter. */
    BYTE* encData;              /* This SHALL be an encrypted TPM_STORE_ASYMKEY structure
                                   TPM_MIGRATE_ASYMKEY structure */
#endif
    TPM_SIZED_BUFFER pcrInfo;
    TPM_SIZED_BUFFER pubKey;
    TPM_SIZED_BUFFER encData;
} TPM_KEY12; 


/* 10.5 TPM_PUBKEY rev 99

   The TPM_PUBKEY structure contains the public portion of an asymmetric key pair. It contains all
   the information necessary for its unambiguous usage. It is possible to construct this structure
   from a TPM_KEY, using the algorithmParms and pubKey fields.

   The pubKey member of this structure shall contain the public key for a specific algorithm.
*/

typedef struct tdTPM_PUBKEY { 
    TPM_KEY_PARMS algorithmParms;       /* This SHALL be the information regarding this key */
#if 0
    TPM_STORE_PUBKEY pubKey;            /* This SHALL be the public key information */
#endif
    TPM_SIZED_BUFFER pubKey;
} TPM_PUBKEY; 

/* 5.b. The TPM must support a minimum of 2 key slots. */

#ifdef TPM_KEY_HANDLES
#if (TPM_KEY_HANDLES < 2)
#error "TPM_KEY_HANDLES minimum is 2"
#endif
#endif 

/* Set the default to 3 so that there can be one owner evict key */

#ifndef TPM_KEY_HANDLES 
#define TPM_KEY_HANDLES 3     /* entries in global TPM_KEY_HANDLE_ENTRY array */
#endif

/* TPM_GetCapability uses a uint_16 for the number of key slots */

#if (TPM_KEY_HANDLES > 0xffff)
#error "TPM_KEY_HANDLES must be less than 0x10000"
#endif

/* The TPM does not have to support any minimum number of owner evict keys.  Adjust this value to
   match the amount of NV space available.  An owner evict key consumes about 512 bytes.

   A value greater than (TPM_KEY_HANDLES - 2) is useless, as the TPM reserves 2 key slots for
   non-owner evict keys to avoid blocking.
*/

#ifndef TPM_OWNER_EVICT_KEY_HANDLES 
#define TPM_OWNER_EVICT_KEY_HANDLES 1 
#endif

#if (TPM_OWNER_EVICT_KEY_HANDLES > (TPM_KEY_HANDLES - 2))
#error "TPM_OWNER_EVICT_KEY_HANDLES too large for TPM_KEY_HANDLES"
#endif

/* This is the version used by the TPM implementation.  It is part of the global TPM state */

/* kgold: Added TPM_KEY member.  There needs to be a mapping between a key handle
   and the pointer to TPM_KEY objects, and this seems to be the right place for it. */

typedef struct tdTPM_KEY_HANDLE_ENTRY {
    TPM_KEY_HANDLE handle;      /* Handles for a key currently loaded in the TPM */
    TPM_KEY *key;               /* Pointer to the key object */
    TPM_BOOL parentPCRStatus;   /* TRUE if parent of this key uses PCR's */
    TPM_KEY_CONTROL keyControl; /* Attributes that can control various aspects of key usage and
                                   manipulation. */
} TPM_KEY_HANDLE_ENTRY; 

/* 5.12 TPM_MIGRATIONKEYAUTH rev 87

   This structure provides the proof that the associated public key has TPM Owner authorization to
   be a migration key.
*/

typedef struct tdTPM_MIGRATIONKEYAUTH { 
    TPM_PUBKEY migrationKey;            /* This SHALL be the public key of the migration facility */
    TPM_MIGRATE_SCHEME migrationScheme; /* This shall be the type of migration operation.*/
    TPM_DIGEST digest;                  /* This SHALL be the digest value of the concatenation of
                                           migration key, migration scheme and tpmProof */
} TPM_MIGRATIONKEYAUTH; 

/* 5.13 TPM_COUNTER_VALUE rev 87

   This structure returns the counter value. For interoperability, the value size should be 4 bytes.
*/

#define TPM_COUNTER_LABEL_SIZE  4
#define TPM_COUNT_ID_NULL 0xffffffff    /* unused value TPM_CAP_PROP_ACTIVE_COUNTER expects this
                                           value if no counter is active */
#define TPM_COUNT_ID_ILLEGAL 0xfffffffe /* after releasing an active counter */

typedef struct tdTPM_COUNTER_VALUE {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;              /* TPM_TAG_COUNTER_VALUE */
#endif
    BYTE label[TPM_COUNTER_LABEL_SIZE]; /* The label for the counter */
    TPM_ACTUAL_COUNT counter;           /* The 32-bit counter value. */
    /* NOTE: Added.  TPMWG email says the specification structure is the public part, but these are
       vendor specific private members. */
    TPM_SECRET authData;                /* Authorization secret for counter */
    TPM_BOOL valid;
    TPM_DIGEST digest;                  /* for OSAP comparison */
} TPM_COUNTER_VALUE; 

/* 5.14 TPM_SIGN_INFO Structure rev 102

   This is an addition in 1.2 and is the structure signed for certain commands (e.g.,
   TPM_ReleaseTransportSigned).  Some commands have a structure specific to that command (e.g.,
   TPM_Quote uses TPM_QUOTE_INFO) and do not use TPM_SIGN_INFO.

   TPM_Sign uses this structure when the signature scheme is TPM_SS_RSASSAPKCS1v15_INFO.
*/

#define TPM_SIGN_INFO_FIXED_SIZE 4

typedef struct tdTPM_SIGN_INFO { 
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;      /* TPM_TAG_SIGNINFO */
#endif
    BYTE fixed[TPM_SIGN_INFO_FIXED_SIZE];       /* The ASCII text that identifies what function was
                                                   performing the signing operation*/
    TPM_NONCE replay;           /* Nonce provided by caller to prevent replay attacks */
#if 0
    uint32_t dataLen;		/* The length of the data area */
    BYTE* data;                 /* The data that is being signed */
#endif
    TPM_SIZED_BUFFER data;      /* The data that is being signed */
} TPM_SIGN_INFO; 

/* 5.15 TPM_MSA_COMPOSITE Structure rev 87

   TPM_MSA_COMPOSITE contains an arbitrary number of digests of public keys belonging to Migration
   Authorities. An instance of TPM_MSA_COMPOSITE is incorporated into the migrationAuth value of a
   certified-migration-key (CMK), and any of the Migration Authorities specified in that instance is
   able to approve the migration of that certified-migration-key.
   
   TPMs MUST support TPM_MSA_COMPOSITE structures with MSAlist of four (4) or less, and MAY support
   larger values of MSAlist.
*/

typedef struct tdTPM_MSA_COMPOSITE {
    uint32_t MSAlist;			/* The number of migAuthDigests. MSAlist MUST be one (1) or
                                           greater. */
    TPM_DIGEST *migAuthDigest;          /* An arbitrary number of digests of public keys belonging
                                           to Migration Authorities. */
} TPM_MSA_COMPOSITE;

/* 5.16 TPM_CMK_AUTH 

   The signed digest of TPM_CMK_AUTH is a ticket to prove that the entity with public key
   "migrationAuthority" has approved the public key "destination Key" as a migration destination for
   the key with public key "sourceKey".

   Normally the digest of TPM_CMK_AUTH is signed by the private key corresponding to
   "migrationAuthority".

   To reduce data size, TPM_CMK_AUTH contains just the digests of "migrationAuthority",
   "destinationKey" and "sourceKey".
*/

typedef struct tdTPM_CMK_AUTH { 
    TPM_DIGEST migrationAuthorityDigest;        /* The digest of the public key of a Migration
                                                   Authority */
    TPM_DIGEST destinationKeyDigest;            /* The digest of a TPM_PUBKEY structure that is an
                                                   approved destination key for the private key
                                                   associated with "sourceKey"*/
    TPM_DIGEST sourceKeyDigest;                 /* The digest of a TPM_PUBKEY structure whose
                                                   corresponding private key is approved by the
                                                   Migration Authority to be migrated as a child to
                                                   the destinationKey.  */
} TPM_CMK_AUTH;

/* 5.18 TPM_SELECT_SIZE rev 87

  This structure provides the indication for the version and sizeOfSelect structure in GetCapability
*/

typedef struct tdTPM_SELECT_SIZE {
    BYTE major;         /* This SHALL indicate the major version of the TPM. This MUST be 0x01 */
    BYTE minor;         /* This SHALL indicate the minor version of the TPM. This MAY be 0x01 or
                           0x02 */
    uint16_t reqSize;	/* This SHALL indicate the value for a sizeOfSelect field in the
                           TPM_SELECTION structure */
} TPM_SELECT_SIZE;

/* 5.19 TPM_CMK_MIGAUTH rev 89

   Structure to keep track of the CMK migration authorization
*/

typedef struct tdTPM_CMK_MIGAUTH {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;      /* Set to TPM_TAG_CMK_MIGAUTH */
#endif
    TPM_DIGEST msaDigest;       /* The digest of a TPM_MSA_COMPOSITE structure containing the
                                   migration authority public key and parameters. */
    TPM_DIGEST pubKeyDigest;    /* The hash of the associated public key */
} TPM_CMK_MIGAUTH;

/* 5.20 TPM_CMK_SIGTICKET rev 87

   Structure to keep track of the CMK migration authorization
*/

typedef struct tdTPM_CMK_SIGTICKET {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;      /* Set to TPM_TAG_CMK_SIGTICKET */
#endif
    TPM_DIGEST verKeyDigest;    /* The hash of a TPM_PUBKEY structure containing the public key and
                                   parameters of the key that can verify the ticket */
    TPM_DIGEST signedData;      /* The ticket data */
} TPM_CMK_SIGTICKET;

/* 5.21 TPM_CMK_MA_APPROVAL rev 87
    
   Structure to keep track of the CMK migration authorization
*/

typedef struct tdTPM_CMK_MA_APPROVAL {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;                      /* Set to TPM_TAG_CMK_MA_APPROVAL */
#endif
    TPM_DIGEST migrationAuthorityDigest;        /* The hash of a TPM_MSA_COMPOSITE structure
                                                   containing the hash of one or more migration
                                                   authority public keys and parameters. */
} TPM_CMK_MA_APPROVAL;

/* 20.2 Delegate Definitions rev 101

   The delegations are in a 64-bit field. Each bit describes a capability that the TPM Owner can
   delegate to a trusted process by setting that bit. Each delegation bit setting is independent of
   any other delegation bit setting in a row.

   If a TPM command is not listed in the following table, then the TPM Owner cannot delegate that
   capability to a trusted process. For the TPM commands that are listed in the following table, if
   the bit associated with a TPM command is set to zero in the row of the table that identifies a
   trusted process, then that process has not been delegated to use that TPM command.

   The minimum granularity for delegation is at the ordinal level. It is not possible to delegate an
   option of an ordinal. This implies that if the options present a difficulty and there is a need
   to separate the delegations then there needs to be a split into two separate ordinals.
*/

#define TPM_DEL_OWNER_BITS 0x00000001 
#define TPM_DEL_KEY_BITS   0x00000002 

typedef struct tdTPM_DELEGATIONS { 
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;      /* This SHALL be TPM_TAG_DELEGATIONS */
#endif
    uint32_t delegateType;        /* Owner or key */
    uint32_t per1;                /* The first block of permissions */
    uint32_t per2;                /* The second block of permissions */
} TPM_DELEGATIONS; 

/* 20.4 TPM_FAMILY_LABEL rev 85

   Used in the family table to hold a one-byte numeric value (sequence number) that software can map
   to a string of bytes that can be displayed or used by applications.

   This is not sensitive data. 
*/

#if 0
typedef struct tdTPM_FAMILY_LABEL { 
    BYTE label;         /* A sequence number that software can map to a string of bytes that can be
                           displayed or used by the applications. This MUST not contain sensitive
                           information. */
} TPM_FAMILY_LABEL; 
#endif

typedef BYTE TPM_FAMILY_LABEL;  /* NOTE: No need for a structure here */

/* 20.5 TPM_FAMILY_TABLE_ENTRY rev 101

   The family table entry is an individual row in the family table. There are no sensitive values in
   a family table entry.

   Each family table entry contains values to facilitate table management: the familyID sequence
   number value that associates a family table row with one or more delegate table rows, a
   verification sequence number value that identifies when rows in the delegate table were last
   verified, and BYTE family label value that software can map to an ASCII text description of the
   entity using the family table entry
*/

typedef struct tdTPM_FAMILY_TABLE_ENTRY { 
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;              /* This SHALL be TPM_TAG_FAMILY_TABLE_ENTRY */
#endif
    TPM_FAMILY_LABEL familyLabel;       /* A sequence number that software can map to a string of
                                           bytes that can be displayed of used by the applications.
                                           This MUST not contain sensitive information. */
    TPM_FAMILY_ID familyID;             /* The family ID in use to tie values together. This is not
                                           a sensitive value. */
    TPM_FAMILY_VERIFICATION verificationCount;  /* The value inserted into delegation rows to
                                                   indicate that they are the current generation of
                                                   rows. Used to identify when a row in the delegate
                                                   table was last verified. This is not a sensitive
                                                   value. */
    TPM_FAMILY_FLAGS flags;             /* See section on TPM_FAMILY_FLAGS. */
    /* NOTE Added */
    TPM_BOOL valid;
} TPM_FAMILY_TABLE_ENTRY;

/* 20.6 TPM_FAMILY_TABLE rev 87

   The family table is stored in a TPM shielded location. There are no confidential values in the
   family table.  The family table contains a minimum of 8 rows.
*/

#ifdef TPM_NUM_FAMILY_TABLE_ENTRY_MIN 
#if (TPM_NUM_FAMILY_TABLE_ENTRY_MIN < 8)
#error "TPM_NUM_FAMILY_TABLE_ENTRY_MIN minimum is 8"
#endif
#endif 

#ifndef TPM_NUM_FAMILY_TABLE_ENTRY_MIN 
#define TPM_NUM_FAMILY_TABLE_ENTRY_MIN 8
#endif

typedef struct tdTPM_FAMILY_TABLE { 
    TPM_FAMILY_TABLE_ENTRY famTableRow[TPM_NUM_FAMILY_TABLE_ENTRY_MIN]; 
} TPM_FAMILY_TABLE;

/* 20.7 TPM_DELEGATE_LABEL rev 87

   Used in both the delegate table and the family table to hold a string of bytes that can be
   displayed or used by applications. This is not sensitive data.
*/

#if 0
typedef struct tdTPM_DELEGATE_LABEL { 
    BYTE label;         /* A byte that can be displayed or used by the applications. This MUST not
                           contain sensitive information.  */
} TPM_DELEGATE_LABEL; 
#endif

typedef BYTE TPM_DELEGATE_LABEL;        /* NOTE: No need for structure */

/* 20.8 TPM_DELEGATE_PUBLIC rev 101

   The information of a delegate row that is public and does not have any sensitive information.

   PCR_INFO_SHORT is appropriate here as the command to create this is done using owner
   authorization, hence the owner authorized the command and the delegation. There is no need to
   validate what configuration was controlling the platform during the blob creation.
*/

typedef struct tdTPM_DELEGATE_PUBLIC { 
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;              /* This SHALL be TPM_TAG_DELEGATE_PUBLIC  */
#endif
    TPM_DELEGATE_LABEL rowLabel;        /* This SHALL be the label for the row. It
                                           MUST not contain any sensitive information. */
    TPM_PCR_INFO_SHORT pcrInfo;         /* This SHALL be the designation of the process that can use
                                           the permission. This is a not sensitive
                                           value. PCR_SELECTION may be NULL.

                                           If selected the pcrInfo MUST be checked on each use of
                                           the delegation. Use of the delegation is where the
                                           delegation is passed as an authorization handle. */
    TPM_DELEGATIONS permissions;        /* This SHALL be the permissions that are allowed to the
                                           indicated process. This is not a sensitive value. */
    TPM_FAMILY_ID familyID;             /* This SHALL be the family ID that identifies which family
                                           the row belongs to. This is not a sensitive value. */
    TPM_FAMILY_VERIFICATION verificationCount;  /* A copy of verificationCount from the associated
                                                   family table. This is not a sensitive value. */
} TPM_DELEGATE_PUBLIC; 


/* 20.9 TPM_DELEGATE_TABLE_ROW rev 101

   A row of the delegate table. 
*/

typedef struct tdTPM_DELEGATE_TABLE_ROW { 
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;      /* This SHALL be TPM_TAG_DELEGATE_TABLE_ROW */
#endif
    TPM_DELEGATE_PUBLIC pub;    /* This SHALL be the public information for a table row. */
    TPM_SECRET authValue;       /* This SHALL be the authorization value that can use the
                                   permissions. This is a sensitive value. */
    /* NOTE Added */
    TPM_BOOL valid;
} TPM_DELEGATE_TABLE_ROW; 

/* 20.10 TPM_DELEGATE_TABLE rev 87

   This is the delegate table. The table contains a minimum of 2 rows.

   This will be an entry in the TPM_PERMANENT_DATA structure.
*/

#ifdef TPM_NUM_DELEGATE_TABLE_ENTRY_MIN 
#if (TPM_NUM_DELEGATE_TABLE_ENTRY_MIN < 2)
#error "TPM_NUM_DELEGATE_TABLE_ENTRY_MIN minimum is 2"
#endif
#endif 

#ifndef TPM_NUM_DELEGATE_TABLE_ENTRY_MIN 
#define TPM_NUM_DELEGATE_TABLE_ENTRY_MIN 2
#endif


typedef struct tdTPM_DELEGATE_TABLE { 
    TPM_DELEGATE_TABLE_ROW delRow[TPM_NUM_DELEGATE_TABLE_ENTRY_MIN]; /* The array of delegations */
} TPM_DELEGATE_TABLE; 

/* 20.11 TPM_DELEGATE_SENSITIVE rev 115

   The TPM_DELEGATE_SENSITIVE structure is the area of a delegate blob that contains sensitive
   information.

   This structure is normative for loading unencrypted blobs before there is an owner.  It is
   informative for TPM_CreateOwnerDelegation and TPM_LoadOwnerDelegation after there is an owner and
   encrypted blobs are used, since the structure is under complete control of the TPM.
*/

typedef struct tdTPM_DELEGATE_SENSITIVE {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;      /* This MUST be TPM_TAG_DELEGATE_SENSITIVE */
#endif
    TPM_SECRET authValue;       /* AuthData value */
} TPM_DELEGATE_SENSITIVE;

/* 20.12 TPM_DELEGATE_OWNER_BLOB rev 87

   This data structure contains all the information necessary to externally store a set of owner
   delegation rights that can subsequently be loaded or used by this TPM.
   
   The encryption mechanism for the sensitive area is a TPM choice. The TPM may use asymmetric
   encryption and the SRK for the key. The TPM may use symmetric encryption and a secret key known
   only to the TPM.
*/

typedef struct tdTPM_DELEGATE_OWNER_BLOB {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;      /* This MUST be TPM_TAG_DELG_OWNER_BLOB */
#endif
    TPM_DELEGATE_PUBLIC pub;    /* The public information for this blob */
    TPM_DIGEST integrityDigest; /* The HMAC to guarantee the integrity of the entire structure */
    TPM_SIZED_BUFFER additionalArea;    /* An area that the TPM can add to the blob which MUST NOT
                                           contain any sensitive information. This would include any
                                           IV material for symmetric encryption */
    TPM_SIZED_BUFFER sensitiveArea;     /* The area that contains the encrypted
                                           TPM_DELEGATE_SENSITIVE */
} TPM_DELEGATE_OWNER_BLOB;

/* 20.13 TPM_DELEGATE_KEY_BLOB rev 87
    
   A structure identical to TPM_DELEGATE_OWNER_BLOB but which stores delegation information for user
   keys.  As compared to TPM_DELEGATE_OWNER_BLOB, it adds a hash of the corresponding public key
   value to the public information.
*/

typedef struct tdTPM_DELEGATE_KEY_BLOB {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;              /* This MUST be TPM_TAG_DELG_KEY_BLOB */
#endif
    TPM_DELEGATE_PUBLIC pub;            /* The public information for this blob */
    TPM_DIGEST integrityDigest;         /* The HMAC to guarantee the integrity of the entire
                                           structure */
    TPM_DIGEST pubKeyDigest;            /* The digest, that uniquely identifies the key for which
                                           this usage delegation applies.  */
    TPM_SIZED_BUFFER additionalArea;    /* An area that the TPM can add to the blob which MUST NOT
                                           contain any sensitive information. This would include any
                                           IV material for symmetric encryption */
    TPM_SIZED_BUFFER sensitiveArea;     /* The area that contains the encrypted
                                           TPM_DELEGATE_SENSITIVE */
} TPM_DELEGATE_KEY_BLOB;

/* 15.1 TPM_CURRENT_TICKS rev 110

   This structure holds the current number of time ticks in the TPM. The value is the number of time
   ticks from the start of the current session. Session start is a variable function that is
   platform dependent. Some platforms may have batteries or other power sources and keep the TPM
   clock session across TPM initialization sessions.
   
   The <tickRate> element of the TPM_CURRENT_TICKS structure provides the number of microseconds per
   tick.  The platform manufacturer must satisfy input clock requirements set by the TPM vendor to
   ensure the accuracy of the tickRate.
   
   No external entity may ever set the current number of time ticks held in TPM_CURRENT_TICKS. This
   value is always reset to 0 when a new clock session starts and increments under control of the
   TPM.
   
   Maintaining the relationship between the number of ticks counted by the TPM and some real world
   clock is a task for external software.
*/

/* This is not a true UINT64, but a special structure to hold currentTicks */

typedef struct tdTPM_UINT64 {
    uint32_t sec;
    uint32_t usec;
} TPM_UINT64;

typedef struct tdTPM_CURRENT_TICKS {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;      /* TPM_TAG_CURRENT_TICKS */
#endif
    TPM_UINT64 currentTicks;    /* The number of ticks since the start of this tick session */
    /* upper is seconds, lower is useconds */
    uint16_t tickRate;		/* The number of microseconds per tick. The maximum resolution of
                                   the TPM tick counter is thus 1 microsecond. The minimum
                                   resolution SHOULD be 1 millisecond. */
    TPM_NONCE tickNonce;        /* TPM_NONCE tickNonce The nonce created by the TPM when resetting
                                   the currentTicks to 0.  This indicates the beginning of a time
                                   session.  This value MUST be valid before the first use of
                                   TPM_CURRENT_TICKS. The value can be set at TPM_Startup or just
                                   prior to first use. */
    /* NOTE Added */
    TPM_UINT64 initialTime;     /* Time from TPM_GetTimeOfDay() */
} TPM_CURRENT_TICKS;

/*
  13. Transport Structures
*/

/* 13.1 TPM _TRANSPORT_PUBLIC rev 87

   The public information relative to a transport session
*/

typedef struct tdTPM_TRANSPORT_PUBLIC {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG   tag;                    /* TPM_TAG_TRANSPORT_PUBLIC */
#endif
    TPM_TRANSPORT_ATTRIBUTES transAttributes;   /* The attributes of this session */
    TPM_ALGORITHM_ID algId;                     /* This SHALL be the algorithm identifier of the
                                                   symmetric key. */
    TPM_ENC_SCHEME encScheme;                   /* This SHALL fully identify the manner in which the
                                                   key will be used for encryption operations. */
} TPM_TRANSPORT_PUBLIC;

/* 13.2 TPM_TRANSPORT_INTERNAL rev 88

   The internal information regarding transport session
*/

/* 7.6 TPM_STANY_DATA */

#ifdef TPM_MIN_TRANS_SESSIONS
#if (TPM_MIN_TRANS_SESSIONS < 3)
#error "TPM_MIN_TRANS_SESSIONS minimum is 3"
#endif
#endif 

#ifndef TPM_MIN_TRANS_SESSIONS
#define TPM_MIN_TRANS_SESSIONS 3
#endif

typedef struct tdTPM_TRANSPORT_INTERNAL {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;              /* TPM_TAG_TRANSPORT_INTERNAL */
#endif
    TPM_AUTHDATA authData;              /* The shared secret for this session */
    TPM_TRANSPORT_PUBLIC transPublic;   /* The public information of this session */
    TPM_TRANSHANDLE transHandle;        /* The handle for this session */
    TPM_NONCE transNonceEven;           /* The even nonce for the rolling protocol */
    TPM_DIGEST transDigest;             /* The log of transport events */
    /* added kgold */
    TPM_BOOL valid;                     /* entry is valid */
} TPM_TRANSPORT_INTERNAL;

/* 13.3 TPM_TRANSPORT_LOG_IN rev 87

   The logging of transport commands occurs in two steps, before execution with the input 
   parameters and after execution with the output parameters.
   
   This structure is in use for input log calculations.
*/

typedef struct tdTPM_TRANSPORT_LOG_IN {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG   tag;    /* TPM_TAG_TRANSPORT_LOG_IN */
#endif
    TPM_DIGEST parameters;      /* The actual parameters contained in the digest are subject to the
                                   rules of the command using this structure. To find the exact
                                   calculation refer to the actions in the command using this
                                   structure. */
    TPM_DIGEST pubKeyHash;      /* The hash of any keys in the transport command */
} TPM_TRANSPORT_LOG_IN;

/* 13.4 TPM_TRANSPORT_LOG_OUT rev 88

   The logging of transport commands occurs in two steps, before execution with the input parameters
   and after execution with the output parameters.
   
   This structure is in use for output log calculations. 
   
   This structure is in use for the INPUT logging during releaseTransport.
*/

typedef struct tdTPM_TRANSPORT_LOG_OUT {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;              /* TPM_TAG_TRANSPORT_LOG_OUT */
#endif
    TPM_CURRENT_TICKS currentTicks;     /* The current tick count. This SHALL be the value of the
                                           current TPM tick counter.  */
    TPM_DIGEST parameters;              /* The actual parameters contained in the digest are subject
                                           to the rules of the command using this structure. To find
                                           the exact calculation refer to the actions in the command
                                           using this structure. */
    TPM_MODIFIER_INDICATOR locality;    /* The locality that called TPM_ExecuteTransport */
} TPM_TRANSPORT_LOG_OUT;

/* 13.5 TPM_TRANSPORT_AUTH structure rev 87

   This structure provides the validation for the encrypted AuthData value.
*/

typedef struct tdTPM_TRANSPORT_AUTH {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG   tag;    /* TPM_TAG_TRANSPORT_AUTH */
#endif
    TPM_AUTHDATA authData;      /* The AuthData value */
} TPM_TRANSPORT_AUTH;

/* 22.3 TPM_DAA_ISSUER rev 91

   This structure is the abstract representation of non-secret settings controlling a DAA
   context. The structure is required when loading public DAA data into a TPM.  TPM_DAA_ISSUER
   parameters are normally held outside the TPM as plain text data, and loaded into a TPM when a DAA
   session is required. A TPM_DAA_ISSUER structure contains no integrity check: the TPM_DAA_ISSUER
   structure at time of JOIN is indirectly verified by the issuer during the JOIN process, and a
   digest of the verified TPM_DAA_ISSUER structure is held inside the TPM_DAA_TPM structure created
   by the JOIN process.  Parameters DAA_digest_X are digests of public DAA_generic_X parameters, and
   used to verify that the correct value of DAA_generic_X has been loaded. DAA_generic_q is stored
   in its native form to reduce command complexity.
*/

typedef struct tdTPM_DAA_ISSUER {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG   tag;    /* MUST be TPM_TAG_DAA_ISSUER */
#endif
    TPM_DIGEST  DAA_digest_R0;  /* A digest of the parameter "R0", which is not secret and may be
                                   common to many TPMs.  */
    TPM_DIGEST  DAA_digest_R1;  /* A digest of the parameter "R1", which is not secret and may be
                                   common to many TPMs.  */
    TPM_DIGEST  DAA_digest_S0;  /* A digest of the parameter "S0", which is not secret and may be
                                   common to many TPMs.  */
    TPM_DIGEST  DAA_digest_S1;  /* A digest of the parameter "S1", which is not secret and may be
                                   common to many TPMs. */
    TPM_DIGEST  DAA_digest_n;   /* A digest of the parameter "n", which is not secret and may be
                                   common to many TPMs.  */
    TPM_DIGEST  DAA_digest_gamma;       /* A digest of the parameter "gamma", which is not secret
                                           and may be common to many TPMs.  */
    BYTE        DAA_generic_q[26];      /* The parameter q, which is not secret and may be common to
                                           many TPMs. Note that q is slightly larger than a digest,
                                           but is stored in its native form to simplify the
                                           TPM_DAA_join command. Otherwise, JOIN requires 3 input
                                           parameters. */
} TPM_DAA_ISSUER;

/* 22.4 TPM_DAA_TPM rev 91

   This structure is the abstract representation of TPM specific parameters used during a DAA 
   context. TPM-specific DAA parameters may be stored outside the TPM, and hence this 
   structure is needed to save private DAA data from a TPM, or load private DAA data into a 
   TPM.
   
   If a TPM_DAA_TPM structure is stored outside the TPM, it is stored in a confidential format that
   can be interpreted only by the TPM created it. This is to ensure that secret parameters are
   rendered confidential, and that both secret and non-secret data in TPM_DAA_TPM form a
   self-consistent set.
  
   TPM_DAA_TPM includes a digest of the public DAA parameters that were used during creation of the
   TPM_DAA_TPM structure. This is needed to verify that a TPM_DAA_TPM is being used with the public
   DAA parameters used to create the TPM_DAA_TPM structure.  Parameters DAA_digest_v0 and
   DAA_digest_v1 are digests of public DAA_private_v0 and DAA_private_v1 parameters, and used to
   verify that the correct private parameters have been loaded.
   
   Parameter DAA_count is stored in its native form, because it is smaller than a digest, and is
   required to enforce consistency.
*/

typedef struct tdTPM_DAA_TPM {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;      /* MUST be TPM_TAG_DAA_TPM */
#endif
    TPM_DIGEST  DAA_digestIssuer;       /* A digest of a TPM_DAA_ISSUER structure that contains the
                                           parameters used to generate this TPM_DAA_TPM
                                           structure. */
    TPM_DIGEST  DAA_digest_v0;  /* A digest of the parameter "v0", which is secret and specific to
                                   this TPM. "v0" is generated during a JOIN phase.  */
    TPM_DIGEST  DAA_digest_v1;  /* A digest of the parameter "v1", which is secret and specific to
                                   this TPM. "v1" is generated during a JOIN phase.  */
    TPM_DIGEST  DAA_rekey;      /* A digest related to the rekeying process, which is not secret but
                                   is specific to this TPM, and must be consistent across JOIN/SIGN
                                   sessions. "rekey" is generated during a JOIN phase. */
    uint32_t      DAA_count;	/* The parameter "count", which is not secret but must be consistent
                                   across JOIN/SIGN sessions. "count" is an input to the TPM from
                                   the host system. */
} TPM_DAA_TPM;

/* 22.5 TPM_DAA_CONTEXT rev 91

   TPM_DAA_CONTEXT structure is created and used inside a TPM, and never leaves the TPM.  This
   entire section is informative as the TPM does not expose this structure.  TPM_DAA_CONTEXT
   includes a digest of the public and private DAA parameters that were used during creation of the
   TPM_DAA_CONTEXT structure. This is needed to verify that a TPM_DAA_CONTEXT is being used with the
   public and private DAA parameters used to create the TPM_DAA_CONTEXT structure.
*/

typedef struct tdTPM_DAA_CONTEXT {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG   tag;    /* MUST be TPM_TAG_DAA_CONTEXT */
#endif
    TPM_DIGEST  DAA_digestContext;      /* A digest of parameters used to generate this
                                           structure. The parameters vary, depending on whether the
                                           session is a JOIN session or a SIGN session. */
    TPM_DIGEST  DAA_digest;     /* A running digest of certain parameters generated during DAA
                                   computation; operationally the same as a PCR (which holds a
                                   running digest of integrity metrics). */
    TPM_DAA_CONTEXT_SEED        DAA_contextSeed;        /* The seed used to generate other DAA
                                                           session parameters */
    BYTE        DAA_scratch[256];       /* Memory used to hold different parameters at different
                                           times of DAA computation, but only one parameter at a
                                           time.  The maximum size of this field is 256 bytes */
    BYTE        DAA_stage;      /* A counter, indicating the stage of DAA computation that was most
                                   recently completed. The value of the counter is zero if the TPM
                                   currently contains no DAA context.

                                   When set to zero (0) the TPM MUST clear all other fields in this
                                   structure.

                                   The TPM MUST set DAA_stage to 0 on TPM_Startup(ANY) */
    TPM_BOOL    DAA_scratch_null;       
} TPM_DAA_CONTEXT;

/* 22.6 TPM_DAA_JOINDATA rev 91

   This structure is the abstract representation of data that exists only during a specific JOIN
   session.
*/

typedef struct tdTPM_DAA_JOINDATA {
    BYTE        DAA_join_u0[128];       /* A TPM-specific secret "u0", used during the JOIN phase,
                                           and discarded afterwards.  */
    BYTE        DAA_join_u1[138];       /* A TPM-specific secret "u1", used during the JOIN phase,
                                           and discarded afterwards.  */
    TPM_DIGEST  DAA_digest_n0;  /* A digest of the parameter "n0", which is an RSA public key with
                                   exponent 2^16 +1 */
} TPM_DAA_JOINDATA;

/* DAA Session structure

*/

#ifdef TPM_MIN_DAA_SESSIONS 
#if (TPM_MIN_DAA_SESSIONS < 1)
#error "TPM_MIN_DAA_SESSIONS minimum is 1"
#endif
#endif 

#ifndef TPM_MIN_DAA_SESSIONS 
#define TPM_MIN_DAA_SESSIONS 1
#endif

typedef struct tdTPM_DAA_SESSION_DATA {
    TPM_DAA_ISSUER      DAA_issuerSettings;     /* A set of DAA issuer parameters controlling a DAA
                                                   session. (non-secret) */
    TPM_DAA_TPM         DAA_tpmSpecific;        /* A set of DAA parameters associated with a
                                                   specific TPM. (secret) */
    TPM_DAA_CONTEXT     DAA_session;            /* A set of DAA parameters associated with a DAA
                                                   session. (secret) */
    TPM_DAA_JOINDATA    DAA_joinSession;        /* A set of DAA parameters used only during the JOIN
                                                   phase of a DAA session, and generated by the
                                                   TPM. (secret) */
    /* added kgold */
    TPM_HANDLE          daaHandle;              /* DAA session handle */
    TPM_BOOL            valid;                  /* array entry is valid */
    /* FIXME should have handle type Join or Sign */
} TPM_DAA_SESSION_DATA;

/* 22.8 TPM_DAA_BLOB rev 98

   The structure passed during the join process
*/

typedef struct tdTPM_DAA_BLOB {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;              /* MUST be TPM_TAG_DAA_BLOB */
#endif
    TPM_RESOURCE_TYPE resourceType;     /* The resource type: enc(DAA_tpmSpecific) or enc(v0) or
                                           enc(v1) */
    BYTE label[16];                     /* Label for identification of the blob. Free format
                                           area. */
    TPM_DIGEST blobIntegrity;           /* The integrity of the entire blob including the sensitive
                                           area. This is a HMAC calculation with the entire
                                           structure (including sensitiveData) being the hash and
                                           daaProof is the secret */
    TPM_SIZED_BUFFER additionalData;    /* Additional information set by the TPM that helps define
                                           and reload the context. The information held in this area
                                           MUST NOT expose any information held in shielded
                                           locations. This should include any IV for symmetric
                                           encryption */
    TPM_SIZED_BUFFER sensitiveData;     /* A TPM_DAA_SENSITIVE structure */
#if 0
    uint32_t additionalSize;              
    [size_is(additionalSize)] BYTE* additionalData;
    uint32_t sensitiveSize;
    [size_is(sensitiveSize)] BYTE* sensitiveData;
#endif
} TPM_DAA_BLOB;

/* 22.9 TPM_DAA_SENSITIVE rev 91
   
   The encrypted area for the DAA parameters
*/

typedef struct tdTPM_DAA_SENSITIVE {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;              /* MUST be TPM_TAG_DAA_SENSITIVE */
#endif
    TPM_SIZED_BUFFER internalData;      /* DAA_tpmSpecific or DAA_private_v0 or DAA_private_v1 */
#if 0
    uint32_t internalSize;
    [size_is(internalSize)] BYTE* internalData;
#endif
} TPM_DAA_SENSITIVE;

/* 7.1 TPM_PERMANENT_FLAGS rev 110

   These flags maintain state information for the TPM. The values are not affected by any
   TPM_Startup command.

   The flag history includes:

   Rev 62 specLevel 1 errataRev 0:  15 BOOLs
   Rev 85 specLevel 2 errataRev 0:  19 BOOLs
        Added: nvLocked, readSRKPub, tpmEstablished, maintenanceDone
   Rev 94 specLevel 2 errataRev 1:  19 BOOLs
   Rev 103 specLevel 2 errataRev 2:  20 BOOLs
        Added: disableFullDALogicInfo
*/

typedef struct tdTPM_PERMANENT_FLAGS { 
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;      /* TPM_TAG_PERMANENT_FLAGS */
#endif
    TPM_BOOL disable;           /* disable The state of the disable flag. The default state is TRUE
                                   */
    TPM_BOOL ownership;         /* The ability to install an owner. The default state is TRUE. */
    TPM_BOOL deactivated;       /* The state of the inactive flag. The default state is TRUE. */
    TPM_BOOL readPubek;         /* The ability to read the PUBEK without owner authorization. The
                                   default state is TRUE.

                                   set TRUE on owner clear
                                   set FALSE on take owner, disablePubekRead
                                */
    TPM_BOOL disableOwnerClear; /* Whether the owner authorized clear commands are active. The
                                   default state is FALSE. */
    TPM_BOOL allowMaintenance;  /* Whether the TPM Owner may create a maintenance archive. The
                                   default state is TRUE. */
    TPM_BOOL physicalPresenceLifetimeLock; /* This bit can only be set to TRUE; it cannot be set to
                                           FALSE except during the manufacturing process.

                                           FALSE: The state of either physicalPresenceHWEnable or
                                           physicalPresenceCMDEnable MAY be changed. (DEFAULT)

                                           TRUE: The state of either physicalPresenceHWEnable or
                                           physicalPresenceCMDEnable MUST NOT be changed for the
                                           life of the TPM. */
    TPM_BOOL physicalPresenceHWEnable;  /* FALSE: Disable the hardware signal indicating physical
                                           presence. (DEFAULT)

                                           TRUE: Enables the hardware signal indicating physical
                                           presence. */
    TPM_BOOL physicalPresenceCMDEnable;         /* FALSE: Disable the command indicating physical
                                           presence. (DEFAULT)

                                           TRUE: Enables the command indicating physical
                                           presence. */
    TPM_BOOL CEKPUsed;          /* TRUE: The PRIVEK and PUBEK were created using
                                   TPM_CreateEndorsementKeyPair.

                                   FALSE: The PRIVEK and PUBEK were created using a manufacturer's
                                   process.  NOTE: This flag has no default value as the key pair
                                   MUST be created by one or the other mechanism. */
    TPM_BOOL TPMpost;           /* TRUE: After TPM_Startup, if there is a call to
                                   TPM_ContinueSelfTest the TPM MUST execute the actions of
                                   TPM_SelfTestFull

                                   FALSE: After TPM_Startup, if there is a call to
                                   TPM_ContinueSelfTest the TPM MUST execute TPM_ContinueSelfTest

                                   If the TPM supports the implicit invocation of
                                   TPM_ContinueSelftTest upon the use of an untested resource, the
                                   TPM MUST use the TPMPost flag to call either TPM_ContinueSelfTest
                                   or TPM_SelfTestFull

                                   The TPM manufacturer sets this bit during TPM manufacturing and
                                   the bit is unchangeable after shipping the TPM

                                   The default state is FALSE */
    TPM_BOOL TPMpostLock;       /* With the clarification of TPMPost TPMpostLock is now 
                                   unnecessary. 
                                   This flag is now deprecated */
    TPM_BOOL FIPS;              /* TRUE: This TPM operates in FIPS mode 
                                   FALSE: This TPM does NOT operate in FIPS mode */
    TPM_BOOL tpmOperator;       /* TRUE: The operator authorization value is valid 
                                   FALSE: the operator authorization value is not set */
    TPM_BOOL enableRevokeEK;    /* TRUE: The TPM_RevokeTrust command is active 
                                   FALSE: the TPM RevokeTrust command is disabled */
    TPM_BOOL nvLocked;          /* TRUE: All NV area authorization checks are active
                                   FALSE: No NV area checks are performed, except for maxNVWrites.
                                   FALSE is the default value */
    TPM_BOOL readSRKPub;        /* TRUE: GetPubKey will return the SRK pub key
                                   FALSE: GetPubKey will not return the SRK pub key
                                   Default SHOULD be FALSE */
    TPM_BOOL tpmEstablished;    /* TRUE: TPM_HASH_START has been executed at some time
                                   FALSE: TPM_HASH_START has not been executed at any time
                                   Default is FALSE - resets using TPM_ResetEstablishmentBit */
    TPM_BOOL maintenanceDone;   /* TRUE: A maintenance archive has been created for the current
                                   SRK */
#if  (TPM_REVISION >= 103)      /* added for rev 103 */
    TPM_BOOL disableFullDALogicInfo; /* TRUE: The full dictionary attack TPM_GetCapability info is
                                        deactivated.  The returned structure is TPM_DA_INFO_LIMITED.
                                        FALSE: The full dictionary attack TPM_GetCapability info is
                                        activated.  The returned structure is TPM_DA_INFO.
                                        Default is FALSE.
                                     */
#endif
    /* NOTE: Cannot add vendor specific flags here, since TPM_GetCapability() returns the serialized
       structure */
} TPM_PERMANENT_FLAGS; 

/* 7.2 TPM_STCLEAR_FLAGS rev 109

   These flags maintain state that is reset on each TPM_Startup(ST_Clear) command. The values are
   not affected by TPM_Startup(ST_State) commands.
*/

typedef struct tdTPM_STCLEAR_FLAGS { 
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;              /* TPM_TAG_STCLEAR_FLAGS */
#endif
    TPM_BOOL deactivated;               /* Prevents the operation of most capabilities. There is no
                                           default state. It is initialized by TPM_Startup to the
                                           same value as TPM_PERMANENT_FLAGS ->
                                           deactivated. TPM_SetTempDeactivated sets it to TRUE. */
    TPM_BOOL disableForceClear;         /* Prevents the operation of TPM_ForceClear when TRUE. The
                                           default state is FALSE.  TPM_DisableForceClear sets it to
                                           TRUE. */
    TPM_BOOL physicalPresence;          /* Command assertion of physical presence. The default state
                                           is FALSE.  This flag is affected by the
                                           TSC_PhysicalPresence command but not by the hardware
                                           signal.  */
    TPM_BOOL physicalPresenceLock;      /* Indicates whether changes to the TPM_STCLEAR_FLAGS ->
                                           physicalPresence flag are permitted.
                                           TPM_Startup(ST_CLEAR) sets PhysicalPresenceLock to its
                                           default state of FALSE (allow changes to the
                                           physicalPresence flag). When TRUE, the physicalPresence
                                           flag is FALSE. TSC_PhysicalPresence can change the state
                                           of physicalPresenceLock.  */
    TPM_BOOL bGlobalLock;               /* Set to FALSE on each TPM_Startup(ST_CLEAR). Set to TRUE
                                           when a write to NV_Index =0 is successful */
    /* NOTE: Cannot add vendor specific flags here, since TPM_GetCapability() returns the serialized
       structure */
} TPM_STCLEAR_FLAGS; 


/* 7.3 TPM_STANY_FLAGS rev 87

   These flags reset on any TPM_Startup command. 
*/

typedef struct tdTPM_STANY_FLAGS {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;      /* TPM_TAG_STANY_FLAGS   */
#endif
    TPM_BOOL postInitialise;    /* Prevents the operation of most capabilities. There is no default
                                   state. It is initialized by TPM_Init to TRUE. TPM_Startup sets it
                                   to FALSE.  */
    TPM_MODIFIER_INDICATOR localityModifier; /*This SHALL indicate for each command the presence of
                                               a locality modifier for the command. It MUST be set
                                               to NULL after the TPM executes each command.  */
#if 0
    TPM_BOOL transportExclusive; /* Defaults to FALSE. TRUE when there is an exclusive transport
                                    session active. Execution of ANY command other than
                                    TPM_ExecuteTransport or TPM_ReleaseTransportSigned MUST
                                    invalidate the exclusive transport session. */    
#endif
    TPM_TRANSHANDLE transportExclusive; /* Defaults to 0x00000000, Set to the handle when an
                                           exclusive transport session is active */
    TPM_BOOL TOSPresent;        /* Defaults to FALSE
                                   Set to TRUE on TPM_HASH_START
                                   set to FALSE using setCapability */
    /* NOTE: Added kgold */
    TPM_BOOL stateSaved;        /* Defaults to FALSE
                                   Set to TRUE on TPM_SaveState
                                   Set to FALSE on any other ordinal

                                   This is an optimization flag, so the file need not be deleted if
                                   it does not exist.
                                */
} TPM_STANY_FLAGS;

/* 7.4 TPM_PERMANENT_DATA rev 105

   This structure contains the data fields that are permanently held in the TPM and not affected by
   TPM_Startup(any).

   Many of these fields contain highly confidential and privacy sensitive material. The TPM must
   maintain the protections around these fields.
*/

#ifdef TPM_MIN_COUNTERS
#if (TPM_MIN_COUNTERS < 4)
#error "TPM_MIN_COUNTERS minimum is 4"
#endif
#endif

#ifndef TPM_MIN_COUNTERS
#define TPM_MIN_COUNTERS 4 /* the minimum number of counters is 4 */
#endif

#define TPM_DELEGATE_KEY TPM_KEY 
#define TPM_MAX_NV_WRITE_NOOWNER 64 

/* Although the ordinal is 32 bits, only the lower 8 bits seem to be used.  So for now, define an
   array of 256/8 bytes for ordinalAuditStatus - kgold */

#define TPM_ORDINALS_MAX        256     /* assumes a multiple of CHAR_BIT */
#define TPM_AUTHDIR_SIZE        1       /* Number of DIR registers */




typedef struct tdTPM_PERMANENT_DATA {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;      /* TPM_TAG_PERMANENT_DATA */
#endif
    BYTE revMajor;              /* This is the TPM major revision indicator. This SHALL be set by
                                   the TPME, only. The default value is manufacturer-specific. */
    BYTE revMinor;              /* This is the TPM minor revision indicator. This SHALL be set by
                                   the TPME, only. The default value is manufacturer-specific. */
    TPM_SECRET tpmProof;        /* This is a random number that each TPM maintains to validate blobs
                                   in the SEAL and other processes. The default value is
                                   manufacturer-specific. */
    TPM_NONCE EKReset;          /* Nonce held by TPM to validate TPM_RevokeTrust. This value is set
                                   as the next 20 bytes from the TPM RNG when the EK is set
                                   (was fipsReset - kgold) */
    TPM_SECRET ownerAuth;       /* This is the TPM-Owner's authorization data. The default value is
                                   manufacturer-specific. */
    TPM_SECRET operatorAuth;    /* The value that allows the execution of the SetTempDeactivated
                                   command */
    TPM_DIRVALUE authDIR;       /* The array of TPM Owner authorized DIR. Points to the same
                                   location as the NV index value. (kgold - was array of 1) */
#ifndef TPM_NOMAINTENANCE
    TPM_PUBKEY manuMaintPub;    /* This is the manufacturer's public key to use in the maintenance
                                   operations. The default value is manufacturer-specific. */
#endif
    TPM_KEY endorsementKey;     /* This is the TPM's endorsement key pair. */
    TPM_KEY srk;                /* This is the TPM's StorageRootKey. */
    TPM_SYMMETRIC_KEY_TOKEN contextKey;  /* This is the key in use to perform context saves. The key
					    may be symmetric or asymmetric. The key size is
					    predicated by the algorithm in use. */
    TPM_SYMMETRIC_KEY_TOKEN delegateKey;	/* This key encrypts delegate rows that are stored
						   outside the TPM. */
    TPM_COUNTER_VALUE auditMonotonicCounter;    /* This SHALL be the audit monotonic counter for the
                                                   TPM. This value starts at 0 and increments
                                                   according to the rules of auditing */
    TPM_COUNTER_VALUE monotonicCounter[TPM_MIN_COUNTERS];       /* This SHALL be the monotonic
                                                                   counters for the TPM. The
                                                                   individual counters start and
                                                                   increment according to the rules
                                                                   of monotonic counters. */
    TPM_PCR_ATTRIBUTES pcrAttrib[TPM_NUM_PCR];  /* The attributes for all of the PCR registers
                                                   supported by the TPM. */
    BYTE ordinalAuditStatus[TPM_ORDINALS_MAX/CHAR_BIT]; /* Table indicating which ordinals are being
                                                           audited. */
#if 0
    /* kgold - The xcrypto RNG is good enough that this is not needed */
    BYTE* rngState;                     /* State information describing the random number
                                           generator. */
#endif
    TPM_FAMILY_TABLE familyTable;       /* The family table in use for delegations */
    TPM_DELEGATE_TABLE delegateTable;   /* The delegate table */
    uint32_t lastFamilyID;	/* A value that sets the high water mark for family ID's. Set to 0
                                   during TPM manufacturing and never reset. */
    uint32_t noOwnerNVWrite;	/* The count of NV writes that have occurred when there is no TPM
                                   Owner.

                                   This value starts at 0 in manufacturing and after each
                                   TPM_OwnerClear. If the value exceeds 64 the TPM returns
                                   TPM_MAXNVWRITES to any command attempting to manipulate the NV
                                   storage. */
    TPM_CMK_DELEGATE restrictDelegate;  /* The settings that allow for the delegation and
                                           use on CMK keys.  Default value is false. */
    TPM_DAA_TPM_SEED tpmDAASeed;        /* This SHALL be a random value generated after generation
                                           of the EK.

                                           tpmDAASeed does not change during TPM Owner changes.  If
                                           the EK is removed (RevokeTrust) then the TPM MUST
                                           invalidate the tpmDAASeed. The owner can force a change
                                           in the value through TPM_SetCapability.

                                           (linked to daaProof) */
    TPM_NONCE daaProof;         /* This is a random number that each TPM maintains to validate blobs
                                   in the DAA processes. The default value is manufacturer-specific.

                                   The value is not changed when the owner is changed.  It is
                                   changed when the EK changes.  The owner can force a change in the
                                   value through TPM_SetCapability. */
    TPM_SYMMETRIC_KEY_TOKEN daaBlobKey;  /* This is the key in use to perform DAA encryption and
					    decryption.  The key may be symmetric or asymmetric. The
					    key size is predicated by the algorithm in use.

					    This value MUST be changed when daaProof changes.

					    This key MUST NOT be a copy of the EK or SRK.

					    (linked to daaProof) */
    /* NOTE: added kgold */
    TPM_BOOL ownerInstalled;            /* TRUE: The TPM has an owner installed.
                                           FALSE: The TPM has no owner installed. (default) */
    BYTE tscOrdinalAuditStatus;         /* extra byte to track TSC ordinals */
    TPM_BOOL allowLoadMaintPub;         /* TRUE allows the TPM_LoadManuMaintPub command */
    
} TPM_PERMANENT_DATA; 

/* 7.6 TPM_STANY_DATA */

#ifdef TPM_MIN_AUTH_SESSIONS
#if (TPM_MIN_AUTH_SESSIONS < 3)
#error "TPM_MIN_AUTH_SESSIONS minimum is 3"
#endif
#endif

#ifndef TPM_MIN_AUTH_SESSIONS 
#define TPM_MIN_AUTH_SESSIONS 3
#endif

/* NOTE: Vendor specific */

typedef struct tdTPM_AUTH_SESSION_DATA {
    /* vendor specific */
    TPM_AUTHHANDLE handle;      /* Handle for a session */
    TPM_PROTOCOL_ID protocolID; /* TPM_PID_OIAP, TPM_PID_OSAP, TPM_PID_DSAP */
    TPM_ENT_TYPE entityTypeByte;        /* The type of entity in use (TPM_ET_SRK, TPM_ET_OWNER,
                                           TPM_ET_KEYHANDLE ... */
    TPM_ADIP_ENC_SCHEME adipEncScheme;  /* ADIP encryption scheme */
    TPM_NONCE nonceEven;        /* OIAP, OSAP, DSAP */
    TPM_SECRET sharedSecret;    /* OSAP */
    TPM_DIGEST entityDigest;    /* OSAP tracks which entity established the OSAP session */
    TPM_DELEGATE_PUBLIC pub;    /* DSAP */
    TPM_BOOL valid;             /* added kgold: array entry is valid */
} TPM_AUTH_SESSION_DATA;


/* 3.   contextList MUST support a minimum of 16 entries, it MAY support more. */

#ifdef TPM_MIN_SESSION_LIST 
#if (TPM_MIN_SESSION_LIST < 16)
#error "TPM_MIN_SESSION_LIST minimum is 16"
#endif
#endif 

#ifndef TPM_MIN_SESSION_LIST 
#define TPM_MIN_SESSION_LIST 16
#endif

/* 7.5 TPM_STCLEAR_DATA rev 101

   This is an informative structure and not normative. It is purely for convenience of writing the
   spec.

   Most of the data in this structure resets on TPM_Startup(ST_Clear). A TPM may implement rules
   that provide longer-term persistence for the data. The TPM reflects how it handles the data in
   various TPM_GetCapability fields including startup effects.
*/

typedef struct tdTPM_STCLEAR_DATA {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;      /* TPM_TAG_STCLEAR_DATA */
#endif
    TPM_NONCE contextNonceKey;  /* This is the nonce in use to properly identify saved key context
                                   blobs This SHALL be set to all zeros on each TPM_Startup
                                   (ST_Clear).
                                */
    TPM_COUNT_ID countID;       /* This is the handle for the current monotonic counter.  This SHALL
                                   be set to zero on each TPM_Startup(ST_Clear). */
    uint32_t ownerReference;	/* Points to where to obtain the owner secret in OIAP and OSAP
                                   commands. This allows a TSS to manage 1.1 applications on a 1.2
                                   TPM where delegation is in operation. */
    TPM_BOOL disableResetLock;  /* Disables TPM_ResetLockValue upon authorization failure.
                                   The value remains TRUE for the timeout period.

                                   Default is FALSE.

                                   The value is in the STCLEAR_DATA structure as the
                                   implementation of this flag is TPM vendor specific. */
    TPM_PCRVALUE PCRS[TPM_NUM_PCR];     /* Platform configuration registers */
#if  (TPM_REVISION >= 103)      /* added for rev 103 */
    uint32_t deferredPhysicalPresence;	/* The value can save the assertion of physicalPresence.
                                           Individual bits indicate to its ordinal that
                                           physicalPresence was previously asserted when the
                                           software state is such that it can no longer be asserted.
                                           Set to zero on each TPM_Startup(ST_Clear). */
#endif
    /* NOTE: Added for dictionary attack mitigation */
    uint32_t authFailCount;	/* number of authorization failures without a TPM_ResetLockValue */
    uint32_t authFailTime;	/* time of threshold failure in seconds */
    /* NOTE: Moved from TPM_STANY_DATA.  Saving this state is optional.  This implementation
       does. */
    TPM_AUTH_SESSION_DATA authSessions[TPM_MIN_AUTH_SESSIONS];  /* List of current
                                                                   sessions. Sessions can be OSAP,
                                                                   OIAP, DSAP and Transport */
    /* NOTE: Added for transport */
    TPM_TRANSPORT_INTERNAL transSessions[TPM_MIN_TRANS_SESSIONS];
    /* 22.7 TPM_STANY_DATA Additions (for DAA) - moved to TPM_STCLEAR_DATA for startup state */
    TPM_DAA_SESSION_DATA daaSessions[TPM_MIN_DAA_SESSIONS];
    /* 1. The group of contextNonceSession, contextCount, contextList MUST reset at the same
       time. */
    TPM_NONCE contextNonceSession;      /* This is the nonce in use to properly identify saved
                                           session context blobs.  This MUST be set to all zeros on
                                           each TPM_Startup (ST_Clear).  The nonce MAY be set to
                                           null on TPM_Startup( any). */
    uint32_t contextCount;		/* This is the counter to avoid session context blob replay
                                           attacks.  This MUST be set to 0 on each TPM_Startup
                                           (ST_Clear).  The value MAY be set to 0 on TPM_Startup
                                           (any). */
    uint32_t contextList[TPM_MIN_SESSION_LIST];	/* This is the list of outstanding session blobs.
                                                   All elements of this array MUST be set to 0 on
                                                   each TPM_Startup (ST_Clear).  The values MAY be
                                                   set to 0 on TPM_Startup (any). */
    /* NOTE Added auditDigest effect, saved with ST_STATE */
    TPM_DIGEST auditDigest;             /* This is the extended value that is the audit log. This
                                           SHALL be set to all zeros at the start of each audit
                                           session. */
    /* NOTE Storage for the ordinal response */
    TPM_STORE_BUFFER ordinalResponse;           /* outgoing response buffer for this ordinal */
} TPM_STCLEAR_DATA; 

/* 7.6 TPM_STANY_DATA rev 87

   This is an informative structure and not normative. It is purely for convenience of writing the
   spec.
    
   Most of the data in this structure resets on TPM_Startup(ST_State). A TPM may implement rules
   that provide longer-term persistence for the data. The TPM reflects how it handles the data in
   various getcapability fields including startup effects.
*/

typedef struct tdTPM_STANY_DATA {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;              /* TPM_TAG_STANY_DATA */
#endif
    TPM_CURRENT_TICKS currentTicks;     /* This is the current tick counter.  This is reset to 0
                                           according to the rules when the TPM can tick. See the
                                           section on the tick counter for details. */
} TPM_STANY_DATA;

/* 11. Signed Structures  */

/* 11.1 TPM_CERTIFY_INFO rev 101

   When the TPM certifies a key, it must provide a signature with a TPM identity key on information
   that describes that key. This structure provides the mechanism to do so.

   Key usage and keyFlags must have their upper byte set to zero to avoid collisions with the other
   signature headers.
*/

typedef struct tdTPM_CERTIFY_INFO { 
    TPM_STRUCT_VER version;             /* This MUST be 1.1.0.0  */
    TPM_KEY_USAGE keyUsage;             /* This SHALL be the same value that would be set in a
                                           TPM_KEY representation of the key to be certified. The
                                           upper byte MUST be zero */
    TPM_KEY_FLAGS keyFlags;             /* This SHALL be set to the same value as the corresponding
                                           parameter in the TPM_KEY structure that describes the
                                           public key that is being certified. The upper byte MUST
                                           be zero */
    TPM_AUTH_DATA_USAGE authDataUsage;  /* This SHALL be the same value that would be set in a
                                           TPM_KEY representation of the key to be certified */
    TPM_KEY_PARMS algorithmParms;       /* This SHALL be the same value that would be set in a
                                           TPM_KEY representation of the key to be certified */
    TPM_DIGEST pubkeyDigest;            /* This SHALL be a digest of the value TPM_KEY -> pubKey ->
                                           key in a TPM_KEY representation of the key to be
                                           certified */
    TPM_NONCE data;                     /* This SHALL be externally provided data.  */
    TPM_BOOL parentPCRStatus;           /* This SHALL indicate if any parent key was wrapped to a
                                           PCR */
    TPM_SIZED_BUFFER pcrInfo;           /*  */
#if 0
    uint32_t PCRInfoSize;		/* This SHALL be the size of the pcrInfo parameter. A value
                                           of zero indicates that the key is not wrapped to a PCR */
    BYTE* PCRInfo;                      /* This SHALL be the TPM_PCR_INFO structure.  */
#endif
    /* NOTE: kgold - Added this structure, a cache of PCRInfo when not NULL */
    TPM_PCR_INFO *tpm_pcr_info;
} TPM_CERTIFY_INFO;

/* 11.2 TPM_CERTIFY_INFO2 rev 101

   When the TPM certifies a key, it must provide a signature with a TPM identity key on information
   that describes that key. This structure provides the mechanism to do so.

   Key usage and keyFlags must have their upper byte set to zero to avoid collisions with the other
   signature headers.
*/

typedef struct tdTPM_CERTIFY_INFO2 { 
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;              /* MUST be TPM_TAG_CERTIFY_INFO2  */
#endif
    BYTE fill;                          /* MUST be 0x00  */
    TPM_PAYLOAD_TYPE payloadType;       /* This SHALL be the same value that would be set in a
                                           TPM_KEY representation of the key to be certified */
    TPM_KEY_USAGE keyUsage;             /* This SHALL be the same value that would be set in a
                                           TPM_KEY representation of the key to be certified. The
                                           upper byte MUST be zero */
    TPM_KEY_FLAGS keyFlags;             /* This SHALL be set to the same value as the corresponding
                                           parameter in the TPM_KEY structure that describes the
                                           public key that is being certified. The upper byte MUST
                                           be zero.  */
    TPM_AUTH_DATA_USAGE authDataUsage;  /* This SHALL be the same value that would be set in a
                                           TPM_KEY representation of the key to be certified */
    TPM_KEY_PARMS algorithmParms;       /* This SHALL be the same value that would be set in a
                                           TPM_KEY representation of the key to be certified */
    TPM_DIGEST pubkeyDigest;            /* This SHALL be a digest of the value TPM_KEY -> pubKey ->
                                           key in a TPM_KEY representation of the key to be
                                           certified */
    TPM_NONCE data;                     /* This SHALL be externally provided data.  */
    TPM_BOOL parentPCRStatus;           /* This SHALL indicate if any parent key was wrapped to a
                                           PCR */
#if 0
    uint32_t PCRInfoSize;		/* This SHALL be the size of the pcrInfo parameter. A value
                                           of zero indicates that the key is not wrapped to a PCR */
    BYTE* PCRInfo;                      /* This SHALL be the TPM_PCR_INFO_SHORT structure.  */
#endif
    TPM_SIZED_BUFFER pcrInfo;
#if 0
    uint32_t migrationAuthoritySize;	/* This SHALL be the size of migrationAuthority */
    BYTE *migrationAuthority;           /* If the key to be certified has [payload ==
                                           TPM_PT_MIGRATE_RESTRICTED or payload
                                           ==TPM_PT_MIGRATE_EXTERNAL], migrationAuthority is the
                                           digest of the TPM_MSA_COMPOSITE and has TYPE ==
                                           TPM_DIGEST. Otherwise it is NULL. */
#endif
    TPM_SIZED_BUFFER migrationAuthority;
    /* NOTE: kgold - Added this structure, a cache of PCRInfo when not NULL */
    TPM_PCR_INFO_SHORT *tpm_pcr_info_short;
} TPM_CERTIFY_INFO2;

/* 11.3 TPM_QUOTE_INFO rev 87

   This structure provides the mechanism for the TPM to quote the current values of a list of PCRs.
*/

typedef struct tdTPM_QUOTE_INFO { 
    TPM_STRUCT_VER version;             /* This MUST be 1.1.0.0 */
    BYTE fixed[4];                      /* This SHALL always be the string 'QUOT' */
    TPM_COMPOSITE_HASH digestValue;     /* This SHALL be the result of the composite hash algorithm
                                           using the current values of the requested PCR indices. */
    TPM_NONCE externalData;             /* 160 bits of externally supplied data */
} TPM_QUOTE_INFO;

/* 11.4 TPM_QUOTE_INFO2 rev 87

   This structure provides the mechanism for the TPM to quote the current values of a list of PCRs.
*/

typedef struct tdTPM_QUOTE_INFO2 {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;              /* This SHALL be TPM_TAG_QUOTE_INFO2 */
#endif
    BYTE fixed[4];                      /* This SHALL always be the string 'QUT2' */
    TPM_NONCE externalData;             /* 160 bits of externally supplied data  */
    TPM_PCR_INFO_SHORT infoShort;       /*  */
} TPM_QUOTE_INFO2;

/* 12.1 TPM_EK_BLOB rev 87
  
  This structure provides a wrapper to each type of structure that will be in use when the
  endorsement key is in use.
*/

typedef struct tdTPM_EK_BLOB {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;      /* TPM_TAG_EK_BLOB */
#endif
    TPM_EK_TYPE ekType;         /* This SHALL be set to reflect the type of blob in use */
    TPM_SIZED_BUFFER    blob;   /* The blob of information depending on the type */
#if 0
    uint32_t blobSize;    /* */
    [size_is(blobSize)] byte* blob;     /* */
#endif
} TPM_EK_BLOB;

/* 12.2 TPM_EK_BLOB_ACTIVATE rev 87

   This structure contains the symmetric key to encrypt the identity credential.  This structure
   always is contained in a TPM_EK_BLOB.
*/

typedef struct tdTPM_EK_BLOB_ACTIVATE {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;              /* TPM_TAG_EK_BLOB_ACTIVATE */
#endif
    TPM_SYMMETRIC_KEY sessionKey;       /* This SHALL be the session key used by the CA to encrypt
                                           the TPM_IDENTITY_CREDENTIAL */
    TPM_DIGEST idDigest;                /* This SHALL be the digest of the TPM identity public key
                                           that is being certified by the CA */
    TPM_PCR_INFO_SHORT pcrInfo;         /* This SHALL indicate the PCR's and localities */
} TPM_EK_BLOB_ACTIVATE;

/* 12.3 TPM_EK_BLOB_AUTH rev 87

   This structure contains the symmetric key to encrypt the identity credential.  This structure
   always is contained in a TPM_EK_BLOB.
*/

typedef struct tdTPM_EK_BLOB_AUTH {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;      /* TPM_TAG_EK_BLOB_AUTH */
#endif
    TPM_SECRET authValue;       /* This SHALL be the authorization value */
} TPM_EK_BLOB_AUTH;

/* 12.5 TPM_IDENTITY_CONTENTS rev 87

   TPM_MakeIdentity uses this structure and the signature of this structure goes to a privacy CA
   during the certification process.
*/

typedef struct tdTPM_IDENTITY_CONTENTS {
    TPM_STRUCT_VER ver;                 /* This MUST be 1.1.0.0 */
    uint32_t ordinal;			/* This SHALL be the ordinal of the TPM_MakeIdentity
                                           command. */
    TPM_CHOSENID_HASH labelPrivCADigest;        /* This SHALL be the result of hashing the chosen
                                                   identityLabel and privacyCA for the new TPM
                                                   identity */
    TPM_PUBKEY identityPubKey;          /* This SHALL be the public key structure of the identity
                                           key */
} TPM_IDENTITY_CONTENTS; 

/* 12.8 TPM_ASYM_CA_CONTENTS rev 87

   This structure contains the symmetric key to encrypt the identity credential.
*/

typedef struct tdTPM_ASYM_CA_CONTENTS {
    TPM_SYMMETRIC_KEY sessionKey;       /* This SHALL be the session key used by the CA to encrypt
                                           the TPM_IDENTITY_CREDENTIAL */
    TPM_DIGEST idDigest;                /* This SHALL be the digest of the TPM_PUBKEY of the key
                                           that is being certified by the CA */
} TPM_ASYM_CA_CONTENTS;

/*
  14. Audit Structures
*/

/* 14.1 TPM_AUDIT_EVENT_IN rev 87

   This structure provides the auditing of the command upon receipt of the command. It provides the
   information regarding the input parameters.
*/

typedef struct tdTPM_AUDIT_EVENT_IN {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG   tag;            /* TPM_TAG_AUDIT_EVENT_IN */
#endif
    TPM_DIGEST inputParms;              /* Digest value according to the HMAC digest rules of the
                                           "above the line" parameters (i.e. the first HMAC digest
                                           calculation). When there are no HMAC rules, the input
                                           digest includes all parameters including and after the
                                           ordinal. */
    TPM_COUNTER_VALUE auditCount;       /* The current value of the audit monotonic counter */
} TPM_AUDIT_EVENT_IN;

/* 14.2 TPM_AUDIT_EVENT_OUT rev 87

  This structure reports the results of the command execution. It includes the return code and the
  output parameters.
*/

typedef struct tdTPM_AUDIT_EVENT_OUT {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;              /* TPM_TAG_AUDIT_EVENT_OUT */
#endif
    TPM_DIGEST outputParms;             /* Digest value according to the HMAC digest rules of the
                                           "above the line" parameters (i.e. the first HMAC digest
                                           calculation). When there are no HMAC rules, the output
                                           digest includes the return code, the ordinal, and all
                                           parameters after the return code. */
    TPM_COUNTER_VALUE auditCount;       /* The current value of the audit monotonic counter */
} TPM_AUDIT_EVENT_OUT;

/*
  18. Context structures
*/

/* 18.1 TPM_CONTEXT_BLOB rev 102

   This is the header for the wrapped context. The blob contains all information necessary to reload
   the context back into the TPM.
   
   The additional data is used by the TPM manufacturer to save information that will assist in the
   reloading of the context. This area must not contain any shielded data. For instance, the field
   could contain some size information that allows the TPM more efficient loads of the context. The
   additional area could not contain one of the primes for a RSA key.
   
   To ensure integrity of the blob when using symmetric encryption the TPM vendor could use some
   valid cipher chaining mechanism. To ensure the integrity without depending on correct
   implementation, the TPM_CONTEXT_BLOB structure uses a HMAC of the entire structure using tpmProof
   as the secret value.

   Since both additionalData and sensitiveData are informative, any or all of additionalData 
   could be moved to sensitiveData.
*/

#define TPM_CONTEXT_LABEL_SIZE 16

typedef struct tdTPM_CONTEXT_BLOB {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;              /* MUST be TPM_TAG_CONTEXTBLOB */
#endif
    TPM_RESOURCE_TYPE resourceType;     /* The resource type */
    TPM_HANDLE handle;                  /* Previous handle of the resource */
    BYTE label[TPM_CONTEXT_LABEL_SIZE]; /* Label for identification of the blob. Free format
                                           area. */
    uint32_t contextCount;		/* MUST be TPM_STANY_DATA -> contextCount when creating the
                                           structure.  This value is ignored for context blobs that
                                           reference a key. */
    TPM_DIGEST integrityDigest;         /* The integrity of the entire blob including the sensitive
                                           area. This is a HMAC calculation with the entire
                                           structure (including sensitiveData) being the hash and
                                           tpmProof is the secret */
#if 0
    uint32_t additionalSize;
    [size_is(additionalSize)] BYTE* additionalData;
    uint32_t sensitiveSize;
    [size_is(sensitiveSize)] BYTE* sensitiveData;
#endif
    TPM_SIZED_BUFFER additionalData;    /* Additional information set by the TPM that helps define
                                           and reload the context. The information held in this area
                                           MUST NOT expose any information held in shielded
                                           locations. This should include any IV for symmetric
                                           encryption */
    TPM_SIZED_BUFFER sensitiveData;     /* The normal information for the resource that can be
                                           exported */
} TPM_CONTEXT_BLOB;

/* 18.2 TPM_CONTEXT_SENSITIVE rev 87

   The internal areas that the TPM needs to encrypt and store off the TPM.

   This is an informative structure and the TPM can implement in any manner they wish.
*/

typedef struct tdTPM_CONTEXT_SENSITIVE {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;              /* MUST be TPM_TAG_CONTEXT_SENSITIVE */
#endif
    TPM_NONCE contextNonce;             /* On context blobs other than keys this MUST be
                                           TPM_STANY_DATA - > contextNonceSession For keys the value
                                           is TPM_STCLEAR_DATA -> contextNonceKey */
#if 0
    uint32_t internalSize;
    [size_is(internalSize)] BYTE* internalData;
#endif
    TPM_SIZED_BUFFER internalData;      /* The internal data area */
} TPM_CONTEXT_SENSITIVE;

/* 19.2 TPM_NV_ATTRIBUTES rev 99

   This structure allows the TPM to keep track of the data and permissions to manipulate the area. 
*/

typedef struct tdTPM_NV_ATTRIBUTES { 
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;      /* TPM_TAG_NV_ATTRIBUTES */
#endif
    uint32_t attributes;	/* The attribute area */
} TPM_NV_ATTRIBUTES; 

/* 19.3 TPM_NV_DATA_PUBLIC rev 110

   This structure represents the public description and controls on the NV area.

   bReadSTClear and bWriteSTClear are volatile, in that they are set FALSE at TPM_Startup(ST_Clear).
   bWriteDefine is persistent, in that it remains TRUE through startup.

   A pcrSelect of 0 indicates that the digestAsRelease is not checked.  In this case, the TPM is not
   required to consume NVRAM space to store the digest, although it may do so.  When
   TPM_GetCapability (TPM_CAP_NV_INDEX) returns the structure, a TPM that does not store the digest
   can return zero.  A TPM that does store the digest may return either the digest or zero.
*/

typedef struct tdTPM_NV_DATA_PUBLIC { 
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;              /* This SHALL be TPM_TAG_NV_DATA_PUBLIC */
#endif
    TPM_NV_INDEX nvIndex;               /* The index of the data area */
    TPM_PCR_INFO_SHORT pcrInfoRead;     /* The PCR selection that allows reading of the area */
    TPM_PCR_INFO_SHORT pcrInfoWrite;    /* The PCR selection that allows writing of the area */
    TPM_NV_ATTRIBUTES permission;       /* The permissions for manipulating the area */
    TPM_BOOL bReadSTClear;              /* Set to FALSE on each TPM_Startup(ST_Clear) and set to
                                           TRUE after a ReadValuexxx with datasize of 0 */
    TPM_BOOL bWriteSTClear;             /* Set to FALSE on each TPM_Startup(ST_CLEAR) and set to
                                           TRUE after a WriteValuexxx with a datasize of 0. */
    TPM_BOOL bWriteDefine;              /* Set to FALSE after TPM_NV_DefineSpace and set to TRUE
                                           after a successful WriteValuexxx with a datasize of 0 */
    uint32_t dataSize;			/* The size of the data area in bytes */
} TPM_NV_DATA_PUBLIC; 

/*  19.4 TPM_NV_DATA_SENSITIVE rev 101
  
    This is an internal structure that the TPM uses to keep the actual NV data and the controls
    regarding the area.
*/

typedef struct tdTPM_NV_DATA_SENSITIVE { 
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;      /* This SHALL be TPM_TAG_NV_DATA_SENSITIVE */
#endif
    TPM_NV_DATA_PUBLIC pubInfo; /* The public information regarding this area */
    TPM_AUTHDATA authValue;     /* The authorization value to manipulate the value */
    BYTE *data;                 /* The data area. This MUST not contain any sensitive information as
                                   the TPM does not provide any confidentiality on the data. */
    /* NOTE Added kg */
    TPM_DIGEST digest;          /* for OSAP comparison */
} TPM_NV_DATA_SENSITIVE;

typedef struct tdTPM_NV_INDEX_ENTRIES {
    uint32_t nvIndexCount;			/* number of entries */
    TPM_NV_DATA_SENSITIVE *tpm_nvindex_entry;	/* array of TPM_NV_DATA_SENSITIVE */
} TPM_NV_INDEX_ENTRIES;

/* TPM_NV_DATA_ST

   This is a cache of the the NV defined space volatile flags, used during error rollback
*/

typedef struct tdTPM_NV_DATA_ST {
    TPM_NV_INDEX nvIndex;               /* The index of the data area */
    TPM_BOOL bReadSTClear;
    TPM_BOOL bWriteSTClear;
} TPM_NV_DATA_ST;

/*
  21. Capability areas
*/

/* 21.6 TPM_CAP_VERSION_INFO rev 99

   This structure is an output from a TPM_GetCapability -> TPM_CAP_VERSION_VAL request.  TPM returns
   the current version and revision of the TPM.

   The specLevel and errataRev are defined in the document "Specification and File Naming
   Conventions"

   The tpmVendorID is a value unique to each vendor. It is defined in the document "TCG Vendor
   Naming".

   The vendor specific area allows the TPM vendor to provide support for vendor options. The TPM
   vendor may define the area to the TPM vendor's needs.
*/

typedef struct tdTPM_CAP_VERSION_INFO {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;      /* MUST be TPM_TAG_CAP_VERSION_INFO */
#endif
    TPM_VERSION version;        /* The version and revision */
    uint16_t specLevel;		/* A number indicating the level of ordinals supported */
    BYTE errataRev;             /* A number indicating the errata version of the specification */
    BYTE tpmVendorID[4];        /* The vendor ID unique to each TPM manufacturer. */
    uint16_t vendorSpecificSize;  /* The size of the vendor specific area */
    BYTE* vendorSpecific;       /* Vendor specific information */
    /* NOTE Cannot be TPM_SIZED_BUFFER, because of uint16_t */
} TPM_CAP_VERSION_INFO;

/* 21.10 TPM_DA_ACTION_TYPE rev 100

   This structure indicates the action taken when the dictionary attack mitigation logic is active,
   when TPM_DA_STATE is TPM_DA_STATE_ACTIVE.
*/   

typedef struct tdTPM_DA_ACTION_TYPE {
    TPM_STRUCTURE_TAG tag;      /* MUST be TPM_TAG_DA_ACTION_TYPE */
    uint32_t actions;		/* The action taken when TPM_DA_STATE is TPM_DA_STATE_ACTIVE. */
} TPM_DA_ACTION_TYPE;

/* 21.7  TPM_DA_INFO rev 100
   
   This structure is an output from a TPM_GetCapability -> TPM_CAP_DA_LOGIC request if
   TPM_PERMANENT_FLAGS -> disableFullDALogicInfo is FALSE.
   
   It returns static information describing the TPM response to authorization failures that might
   indicate a dictionary attack and dynamic information regarding the current state of the
   dictionary attack mitigation logic.
*/

typedef struct tdTPM_DA_INFO {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;      /* MUST be TPM_TAG_DA_INFO */
#endif
    TPM_DA_STATE state;         /* Dynamic.  The actual state of the dictionary attack mitigation
                                   logic.  See 21.9. */
    uint16_t currentCount;	/* Dynamic.  The actual count of the authorization failure counter
                                   for the selected entity type */
    uint16_t thresholdCount;	/* Static.  Dictionary attack mitigation threshold count for the
                                   selected entity type */
    TPM_DA_ACTION_TYPE actionAtThreshold;       /* Static Action of the TPM when currentCount passes
                                                   thresholdCount. See 21.10. */
    uint32_t actionDependValue;	/* Dynamic.  Action being taken when the dictionary attack
                                   mitigation logic is active.  E.g., when actionAtThreshold is
                                   TPM_DA_ACTION_TIMEOUT, this is the lockout time remaining in
                                   seconds. */
    TPM_SIZED_BUFFER vendorData;        /* Vendor specific data field */
} TPM_DA_INFO;

/* 21.8 TPM_DA_INFO_LIMITED rev 100

   This structure is an output from a TPM_GetCapability -> TPM_CAP_DA_LOGIC request if
   TPM_PERMANENT_FLAGS -> disableFullDALogicInfo is TRUE.
   
   It returns static information describing the TPM response to authorization failures that might
   indicate a dictionary attack and dynamic information regarding the current state of the
   dictionary attack mitigation logic. This structure omits information that might aid an attacker.
*/

typedef struct tdTPM_DA_INFO_LIMITED {
#ifdef TPM_USE_TAG_IN_STRUCTURE
    TPM_STRUCTURE_TAG tag;      /* MUST be TPM_TAG_DA_INFO_LIMITED */
#endif
    TPM_DA_STATE state;         /* Dynamic.  The actual state of the dictionary attack mitigation
                                   logic.  See 21.9. */
    TPM_DA_ACTION_TYPE actionAtThreshold;       /* Static Action of the TPM when currentCount passes
                                                   thresholdCount. See 21.10. */
    TPM_SIZED_BUFFER vendorData;        /* Vendor specific data field */
} TPM_DA_INFO_LIMITED;

#endif

/* Sanity check the size of the NV file vs. the maximum allocation size

   The multipliers are very conservative
*/

#if (TPM_ALLOC_MAX <					\
	(4000 +						\
	 (TPM_OWNER_EVICT_KEY_HANDLES * 2000) +		\
	 TPM_MAX_NV_DEFINED_SPACE))
#error "TPM_ALLOC_MAX too small for NV file size"
#endif

/* Sanity check the size of the volatile file vs. the maximum allocation size
 
   The multipliers are very conservative
*/

#if (TPM_ALLOC_MAX <					\
	(4000 +						\
	 TPM_KEY_HANDLES * 2000 +			\
	 TPM_MIN_TRANS_SESSIONS * 500 +			\
	 TPM_MIN_DAA_SESSIONS * 2000 +			\
	 TPM_MIN_AUTH_SESSIONS * 500))
#error "TPM_ALLOC_MAX too small for volatile file size"
#endif
