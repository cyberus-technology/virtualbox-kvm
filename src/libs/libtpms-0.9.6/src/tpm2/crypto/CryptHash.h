/********************************************************************************/
/*										*/
/*			    Hash structure definitions  			*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: CryptHash.h $		*/
/*										*/
/*  Licenses and Notices							*/
/*										*/
/*  1. Copyright Licenses:							*/
/*										*/
/*  - Trusted Computing Group (TCG) grants to the user of the source code in	*/
/*    this specification (the "Source Code") a worldwide, irrevocable, 		*/
/*    nonexclusive, royalty free, copyright license to reproduce, create 	*/
/*    derivative works, distribute, display and perform the Source Code and	*/
/*    derivative works thereof, and to grant others the rights granted herein.	*/
/*										*/
/*  - The TCG grants to the user of the other parts of the specification 	*/
/*    (other than the Source Code) the rights to reproduce, distribute, 	*/
/*    display, and perform the specification solely for the purpose of 		*/
/*    developing products based on such documents.				*/
/*										*/
/*  2. Source Code Distribution Conditions:					*/
/*										*/
/*  - Redistributions of Source Code must retain the above copyright licenses, 	*/
/*    this list of conditions and the following disclaimers.			*/
/*										*/
/*  - Redistributions in binary form must reproduce the above copyright 	*/
/*    licenses, this list of conditions	and the following disclaimers in the 	*/
/*    documentation and/or other materials provided with the distribution.	*/
/*										*/
/*  3. Disclaimers:								*/
/*										*/
/*  - THE COPYRIGHT LICENSES SET FORTH ABOVE DO NOT REPRESENT ANY FORM OF	*/
/*  LICENSE OR WAIVER, EXPRESS OR IMPLIED, BY ESTOPPEL OR OTHERWISE, WITH	*/
/*  RESPECT TO PATENT RIGHTS HELD BY TCG MEMBERS (OR OTHER THIRD PARTIES)	*/
/*  THAT MAY BE NECESSARY TO IMPLEMENT THIS SPECIFICATION OR OTHERWISE.		*/
/*  Contact TCG Administration (admin@trustedcomputinggroup.org) for 		*/
/*  information on specification licensing rights available through TCG 	*/
/*  membership agreements.							*/
/*										*/
/*  - THIS SPECIFICATION IS PROVIDED "AS IS" WITH NO EXPRESS OR IMPLIED 	*/
/*    WARRANTIES WHATSOEVER, INCLUDING ANY WARRANTY OF MERCHANTABILITY OR 	*/
/*    FITNESS FOR A PARTICULAR PURPOSE, ACCURACY, COMPLETENESS, OR 		*/
/*    NONINFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS, OR ANY WARRANTY 		*/
/*    OTHERWISE ARISING OUT OF ANY PROPOSAL, SPECIFICATION OR SAMPLE.		*/
/*										*/
/*  - Without limitation, TCG and its members and licensors disclaim all 	*/
/*    liability, including liability for infringement of any proprietary 	*/
/*    rights, relating to use of information in this specification and to the	*/
/*    implementation of this specification, and TCG disclaims all liability for	*/
/*    cost of procurement of substitute goods or services, lost profits, loss 	*/
/*    of use, loss of data or any incidental, consequential, direct, indirect, 	*/
/*    or special damages, whether under contract, tort, warranty or otherwise, 	*/
/*    arising in any way out of use or reliance upon this specification or any 	*/
/*    information herein.							*/
/*										*/
/*  (c) Copyright IBM Corp. and others, 2016 - 2021				*/
/*										*/
/********************************************************************************/

#ifndef CRYPTHASH_H
#define CRYPTHASH_H

/* 10.1.3.1	Introduction */

/* This header contains the hash structure definitions used in the TPM code to define the amount of
   space to be reserved for the hash state. This allows the TPM code to not have to import all of
   the symbols used by the hash computations. This lets the build environment of the TPM code not to
   have include the header files associated with the CryptoEngine() code. */

/* 10.1.3.2	Hash-related Structures */

union SMAC_STATES;

/* These definitions add the high-level methods for processing state that may be an SMAC */
typedef void(* SMAC_DATA_METHOD)(
				 union SMAC_STATES       *state,
				 UINT32                   size,
				 const BYTE              *buffer
				 );
typedef UINT16(* SMAC_END_METHOD)(
				  union SMAC_STATES       *state,
				  UINT32                   size,
				  BYTE                    *buffer
				  );
typedef struct sequenceMethods {
    SMAC_DATA_METHOD          data;
    SMAC_END_METHOD           end;
} SMAC_METHODS;
#define SMAC_IMPLEMENTED (CC_MAC || CC_MAC_Start)

/* These definitions are here because the SMAC state is in the union of hash states. */

typedef struct tpmCmacState {
    TPM_ALG_ID              symAlg;
    UINT16                  keySizeBits;
    INT16                   bcount; // current count of bytes accumulated in IV
    TPM2B_IV                iv;     // IV buffer
    TPM2B_SYM_KEY           symKey;
} tpmCmacState_t;

typedef union SMAC_STATES {
#if ALG_CMAC
    tpmCmacState_t          cmac;
#endif
    UINT64                  pad;
} SMAC_STATES;

typedef struct SMAC_STATE {
    SMAC_METHODS            smacMethods;
    SMAC_STATES             state;
} SMAC_STATE;

#if ALG_SHA1
#   define  IF_IMPLEMENTED_SHA1(op)     op(SHA1, Sha1)
#else
#   define  IF_IMPLEMENTED_SHA1(op)
#endif
#if ALG_SHA256
#   define  IF_IMPLEMENTED_SHA256(op)     op(SHA256, Sha256)
#else
#   define  IF_IMPLEMENTED_SHA256(op)
#endif
#if ALG_SHA384
#   define  IF_IMPLEMENTED_SHA384(op)     op(SHA384, Sha384)
#else
#   define  IF_IMPLEMENTED_SHA384(op)
#endif
#if ALG_SHA512
#   define  IF_IMPLEMENTED_SHA512(op)     op(SHA512, Sha512)
#else
#   define  IF_IMPLEMENTED_SHA512(op)
#endif
#if ALG_SM3_256
#   define  IF_IMPLEMENTED_SM3_256(op)     op(SM3_256, Sm3_256)
#else
#   define  IF_IMPLEMENTED_SM3_256(op)
#endif
#if ALG_SHA3_256
#   define  IF_IMPLEMENTED_SHA3_256(op)     op(SHA3_256, Sha3_256)
#else
#   define  IF_IMPLEMENTED_SHA3_256(op)
#endif
#if ALG_SHA3_384
#   define  IF_IMPLEMENTED_SHA3_384(op)     op(SHA3_384, Sha3_384)
#else
#   define  IF_IMPLEMENTED_SHA3_384(op)
#endif
#if ALG_SHA3_512
#   define  IF_IMPLEMENTED_SHA3_512(op)     op(SHA3_512, Sha3_512)
#else
#   define  IF_IMPLEMENTED_SHA3_512(op)
#endif

/* SHA512 added kgold */
#define FOR_EACH_HASH(op)		    \
    IF_IMPLEMENTED_SHA1(op)		    \
    IF_IMPLEMENTED_SHA256(op)		    \
    IF_IMPLEMENTED_SHA384(op)		    \
    IF_IMPLEMENTED_SHA512(op)		    \
    IF_IMPLEMENTED_SM3_256(op)		    \
    IF_IMPLEMENTED_SHA3_256(op)		    \
    IF_IMPLEMENTED_SHA3_384(op)		    \
    IF_IMPLEMENTED_SHA3_512(op)

#define HASH_TYPE(HASH, Hash)   tpmHashState##HASH##_t  Hash;

typedef union
{
    FOR_EACH_HASH(HASH_TYPE)
    // Additions for symmetric block cipher MAC
#if SMAC_IMPLEMENTED
    SMAC_STATE                 smac;
#endif
    // to force structure alignment to be no worse than HASH_ALIGNMENT
#if HASH_ALIGNMENT == 8
    uint64_t             align;
#else
    uint32_t             align;
#endif
} ANY_HASH_STATE;

typedef ANY_HASH_STATE *PANY_HASH_STATE;
typedef const ANY_HASH_STATE    *PCANY_HASH_STATE;
#define ALIGNED_SIZE(x, b) ((((x) + (b) - 1) / (b)) * (b))
/* MAX_HASH_STATE_SIZE will change with each implementation. It is assumed that a hash state will
   not be larger than twice the block size plus some overhead (in this case, 16 bytes). The overall
   size needs to be as large as any of the hash contexts. The structure needs to start on an
   alignment boundary and be an even multiple of the alignment */
#define MAX_HASH_STATE_SIZE ((2 * MAX_HASH_BLOCK_SIZE) + 16)
#define MAX_HASH_STATE_SIZE_ALIGNED					\
    ALIGNED_SIZE(MAX_HASH_STATE_SIZE, HASH_ALIGNMENT)
/* This is an aligned byte array that will hold any of the hash contexts. */
typedef  ANY_HASH_STATE ALIGNED_HASH_STATE;
/* The header associated with the hash library is expected to define the methods which include the
   calling sequence. When not compiling CryptHash.c, the methods are not defined so we need
   placeholder functions for the structures */
#ifndef HASH_START_METHOD_DEF
#   define HASH_START_METHOD_DEF    void (HASH_START_METHOD)(void)
#endif
#ifndef HASH_DATA_METHOD_DEF
#   define HASH_DATA_METHOD_DEF     void (HASH_DATA_METHOD)(void)
#endif
#ifndef HASH_END_METHOD_DEF
#   define HASH_END_METHOD_DEF      void (HASH_END_METHOD)(void)
#endif
#ifndef HASH_STATE_COPY_METHOD_DEF
#   define HASH_STATE_COPY_METHOD_DEF     void (HASH_STATE_COPY_METHOD)(void)
#endif
#ifndef  HASH_STATE_EXPORT_METHOD_DEF
#   define  HASH_STATE_EXPORT_METHOD_DEF   void (HASH_STATE_EXPORT_METHOD)(void)
#endif
#ifndef  HASH_STATE_IMPORT_METHOD_DEF
#   define  HASH_STATE_IMPORT_METHOD_DEF   void (HASH_STATE_IMPORT_METHOD)(void)
#endif
/* Define the prototypical function call for each of the methods. This defines the order in which
   the parameters are passed to the underlying function. */
typedef HASH_START_METHOD_DEF;
typedef HASH_DATA_METHOD_DEF;
typedef HASH_END_METHOD_DEF;
typedef HASH_STATE_COPY_METHOD_DEF;
typedef HASH_STATE_EXPORT_METHOD_DEF;
typedef HASH_STATE_IMPORT_METHOD_DEF;
typedef struct _HASH_METHODS
{
    HASH_START_METHOD           *start;
    HASH_DATA_METHOD            *data;
    HASH_END_METHOD             *end;
    HASH_STATE_COPY_METHOD      *copy;      // Copy a hash block
    HASH_STATE_EXPORT_METHOD    *copyOut;   // Copy a hash block from a hash
    // context
    HASH_STATE_IMPORT_METHOD    *copyIn;    // Copy a hash block to a proper hash
    // context
} HASH_METHODS, *PHASH_METHODS;

#define HASH_TPM2B(HASH, Hash)  TPM2B_TYPE(HASH##_DIGEST, HASH##_DIGEST_SIZE);

FOR_EACH_HASH(HASH_TPM2B)

/* When the TPM implements RSA, the hash-dependent OID pointers are part of the HASH_DEF. These
   macros conditionally add the OID reference to the HASH_DEF and the HASH_DEF_TEMPLATE. */
#if ALG_RSA
#define PKCS1_HASH_REF   const BYTE  *PKCS1;
#define PKCS1_OID(NAME)  , OID_PKCS1_##NAME
#else
#define PKCS1_HASH_REF
#define PKCS1_OID(NAME)
#endif

/* When the TPM implements ECC, the hash-dependent OID pointers are part of the HASH_DEF. These
   macros conditionally add the OID reference to the HASH_DEF and the HASH_DEF_TEMPLATE. */
#if ALG_ECDSA
#define ECDSA_HASH_REF    const BYTE  *ECDSA;
#define ECDSA_OID(NAME)  , OID_ECDSA_##NAME
#else
#define ECDSA_HASH_REF
#define ECDSA_OID(NAME)
#endif

typedef const struct
{
    HASH_METHODS         method;
    uint16_t             blockSize;
    uint16_t             digestSize;
    uint16_t             contextSize;
    uint16_t             hashAlg;
    const BYTE          *OID;
    PKCS1_HASH_REF      // PKCS1 OID
    ECDSA_HASH_REF      // ECDSA OID
} HASH_DEF, *PHASH_DEF;

/* Macro to fill in the HASH_DEF for an algorithm. For SHA1, the instance would be:
   HASH_DEF_TEMPLATE(Sha1, SHA1) This handles the difference in capitalization for the various
   pieces. */

#define HASH_DEF_TEMPLATE(HASH, Hash)					\
    HASH_DEF    Hash##_Def= {						\
			     {(HASH_START_METHOD *)&tpmHashStart_##HASH, \
			      (HASH_DATA_METHOD *)&tpmHashData_##HASH,	\
			      (HASH_END_METHOD *)&tpmHashEnd_##HASH,	\
			      (HASH_STATE_COPY_METHOD *)&tpmHashStateCopy_##HASH, \
			      (HASH_STATE_EXPORT_METHOD *)&tpmHashStateExport_##HASH, \
			      (HASH_STATE_IMPORT_METHOD *)&tpmHashStateImport_##HASH, \
			     },						\
			     HASH##_BLOCK_SIZE,     /*block size */	\
			     HASH##_DIGEST_SIZE,    /*data size */	\
			     sizeof(tpmHashState##HASH##_t),		\
			     TPM_ALG_##HASH, OID_##HASH			\
			     PKCS1_OID(HASH) ECDSA_OID(HASH)};

/* These definitions are for the types that can be in a hash state structure. These types are used
   in the cryptographic utilities. This is a define rather than an enum so that the size of this
   field can be explicit. */
typedef BYTE    HASH_STATE_TYPE;
#define HASH_STATE_EMPTY        ((HASH_STATE_TYPE) 0)
#define HASH_STATE_HASH         ((HASH_STATE_TYPE) 1)
#define HASH_STATE_HMAC         ((HASH_STATE_TYPE) 2)
#if CC_MAC || CC_MAC_Start
#define HASH_STATE_SMAC         ((HASH_STATE_TYPE) 3)
#endif
/* This is the structure that is used for passing a context into the hashing functions. It should be
   the same size as the function context used within the hashing functions. This is checked when the
   hash function is initialized. This version uses a new layout for the contexts and a different
   definition. The state buffer is an array of HASH_UNIT values so that a decent compiler will put
   the structure on a HASH_UNIT boundary. If the structure is not properly aligned, the code that
   manipulates the structure will copy to a properly aligned structure before it is used and copy
   the result back. This just makes things slower. */
/*     NOTE: This version of the state had the pointer to the update method in the state. This is to
       allow the SMAC functions to use the same structure without having to replicate the entire
       HASH_DEF structure. */
typedef struct _HASH_STATE
{
    HASH_STATE_TYPE          type;               // type of the context
    TPM_ALG_ID               hashAlg;
    PHASH_DEF                def;
    ANY_HASH_STATE           state;
} HASH_STATE, *PHASH_STATE;
typedef const HASH_STATE *PCHASH_STATE;

/* 10.1.3.3 HMAC State Structures */
/* This header contains the hash structure definitions used in the TPM code to define the amount of
   space to be reserved for the hash state. This allows the TPM code to not have to import all of
   the symbols used by the hash computations. This lets the build environment of the TPM code not to
   have include the header files associated with the CryptoEngine() code. */

/* An HMAC_STATE structure contains an opaque HMAC stack state. A caller would use this structure
   when performing incremental HMAC operations. This structure contains a hash state and an HMAC key
   and allows slightly better stack optimization than adding an HMAC key to each hash state. */
typedef struct hmacState
{
    HASH_STATE           hashState;          // the hash state
    TPM2B_HASH_BLOCK     hmacKey;            // the HMAC key
} HMAC_STATE, *PHMAC_STATE;
/* This is for the external hash state. This implementation assumes that the size of the exported
   hash state is no larger than the internal hash state. */
typedef struct
{
    BYTE                     buffer[sizeof(HASH_STATE)];
} EXPORT_HASH_STATE, *PEXPORT_HASH_STATE;
typedef const EXPORT_HASH_STATE *PCEXPORT_HASH_STATE;

#endif	//  _CRYPT_HASH_H
