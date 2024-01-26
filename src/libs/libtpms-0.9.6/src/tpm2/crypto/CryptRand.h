/********************************************************************************/
/*										*/
/*		DRBG with a behavior according to SP800-90A			*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: CryptRand.h $		*/
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
/*  (c) Copyright IBM Corp. and others, 2016 - 2020				*/
/*										*/
/********************************************************************************/

/* 10.1.4 CryptRand.h */
/* 10.1.4.1 Introduction */
/* This file contains constant definition shared by CryptUtil() and the parts of the Crypto
   Engine. */
#ifndef _CRYPT_RAND_H
#define _CRYPT_RAND_H
/* DRBG Structures and Defines Values and structures for the random number generator. These values
   are defined in this header file so that the size of the RNG state can be known to TPM.lib. This
   allows the allocation of some space in NV memory for the state to be stored on an orderly
   shutdown. The DRBG based on a symmetric block cipher is defined by three values, */
/* a) the key size */
/* b) the block size (the IV size) */
/* c) the symmetric algorithm */
#define DRBG_KEY_SIZE_BITS      AES_MAX_KEY_SIZE_BITS
#define DRBG_IV_SIZE_BITS       (AES_MAX_BLOCK_SIZE * 8)
#define DRBG_ALGORITHM          TPM_ALG_AES
typedef tpmKeyScheduleAES DRBG_KEY_SCHEDULE;
#define DRBG_ENCRYPT_SETUP(key, keySizeInBits, schedule)		\
    TpmCryptSetEncryptKeyAES(key, keySizeInBits, schedule)
#define DRBG_ENCRYPT(keySchedule, in, out)				\
    TpmCryptEncryptAES(SWIZZLE(keySchedule, in, out))
#if     ((DRBG_KEY_SIZE_BITS % RADIX_BITS) != 0)	\
    || ((DRBG_IV_SIZE_BITS % RADIX_BITS) != 0)
#error "Key size and IV for DRBG must be even multiples of the radix"
#endif
#if (DRBG_KEY_SIZE_BITS % DRBG_IV_SIZE_BITS) != 0
#error "Key size for DRBG must be even multiple of the cypher block size"
#endif
/*     Derived values */
#define DRBG_MAX_REQUESTS_PER_RESEED (1 << 48)
#define DRBG_MAX_REQEST_SIZE (1 << 32)
#define pDRBG_KEY(seed)    ((DRBG_KEY *)&(((BYTE *)(seed))[0]))
#define pDRBG_IV(seed)     ((DRBG_IV *)&(((BYTE *)(seed))[DRBG_KEY_SIZE_BYTES]))
#define DRBG_KEY_SIZE_WORDS     (BITS_TO_CRYPT_WORDS(DRBG_KEY_SIZE_BITS))
#define DRBG_KEY_SIZE_BYTES     (DRBG_KEY_SIZE_WORDS * RADIX_BYTES)
#define DRBG_IV_SIZE_WORDS      (BITS_TO_CRYPT_WORDS(DRBG_IV_SIZE_BITS))
#define DRBG_IV_SIZE_BYTES      (DRBG_IV_SIZE_WORDS * RADIX_BYTES)
#define DRBG_SEED_SIZE_WORDS    (DRBG_KEY_SIZE_WORDS + DRBG_IV_SIZE_WORDS)
#define DRBG_SEED_SIZE_BYTES    (DRBG_KEY_SIZE_BYTES + DRBG_IV_SIZE_BYTES)
typedef union
{
    BYTE            bytes[DRBG_KEY_SIZE_BYTES];
    crypt_uword_t   words[DRBG_KEY_SIZE_WORDS];
} DRBG_KEY;
typedef union
{
    BYTE            bytes[DRBG_IV_SIZE_BYTES];
    crypt_uword_t   words[DRBG_IV_SIZE_WORDS];
} DRBG_IV;
typedef union
{
    BYTE            bytes[DRBG_SEED_SIZE_BYTES];
    crypt_uword_t   words[DRBG_SEED_SIZE_WORDS];
} DRBG_SEED;
#define CTR_DRBG_MAX_REQUESTS_PER_RESEED        ((UINT64)1 << 20)
#define CTR_DRBG_MAX_BYTES_PER_REQUEST          (1 << 16)
#   define CTR_DRBG_MIN_ENTROPY_INPUT_LENGTH    DRBG_SEED_SIZE_BYTES
#   define CTR_DRBG_MAX_ENTROPY_INPUT_LENGTH    DRBG_SEED_SIZE_BYTES
#   define CTR_DRBG_MAX_ADDITIONAL_INPUT_LENGTH DRBG_SEED_SIZE_BYTES
#define     TESTING         (1 << 0)
#define     ENTROPY         (1 << 1)
#define     TESTED          (1 << 2)
#define     IsTestStateSet(BIT)    ((g_cryptoSelfTestState.rng & BIT) != 0)
#define     SetTestStateBit(BIT)   (g_cryptoSelfTestState.rng |= BIT)
#define     ClearTestStateBit(BIT) (g_cryptoSelfTestState.rng &= ~BIT)
#define     IsSelfTest()    IsTestStateSet(TESTING)
#define     SetSelfTest()   SetTestStateBit(TESTING)
#define     ClearSelfTest() ClearTestStateBit(TESTING)
#define     IsEntropyBad()      IsTestStateSet(ENTROPY)
#define     SetEntropyBad()     SetTestStateBit(ENTROPY)
#define     ClearEntropyBad()   ClearTestStateBit(ENTROPY)
#define     IsDrbgTested()      IsTestStateSet(TESTED)
#define     SetDrbgTested()     SetTestStateBit(TESTED)
#define     ClearDrbgTested()   ClearTestStateBit(TESTED)
    typedef struct
    {
	UINT64      reseedCounter;
	UINT32      magic;
	DRBG_SEED   seed; // contains the key and IV for the counter mode DRBG
	SEED_COMPAT_LEVEL seedCompatLevel;   // libtpms added: the compatibility level for keeping backwards compatibility
	UINT32      lastValue[4];   // used when the TPM does continuous self-test
	// for FIPS compliance of DRBG
    } DRBG_STATE, *pDRBG_STATE;
#define DRBG_MAGIC   ((UINT32) 0x47425244) // "DRBG" backwards so that it displays
typedef struct KDF_STATE 
{
    UINT64               counter;
    UINT32               magic;
    UINT32               limit;
    TPM2B               *seed;
    const TPM2B         *label;
    TPM2B               *context;
    TPM_ALG_ID           hash;
    TPM_ALG_ID           kdf;
    UINT16               digestSize;
    TPM2B_DIGEST         residual;
} KDF_STATE, *pKDR_STATE;
#define KDF_MAGIC    ((UINT32) 0x4048444a) // "KDF " backwards
/* Make sure that any other structures added to this union start with a 64-bit counter and a 32-bit
   magic number */
typedef union
{
    DRBG_STATE      drbg;
    KDF_STATE       kdf;
} RAND_STATE;
/* This is the state used when the library uses a random number generator. A special function is
   installed for the library to call. That function picks up the state from this location and uses
   it for the generation of the random number. */
extern RAND_STATE           *s_random;
/* When instrumenting RSA key sieve */
#if  RSA_INSTRUMENT
#define PRIME_INDEX(x)  ((x) == 512 ? 0 : (x) == 1024 ? 1 : 2)
#   define INSTRUMENT_SET(a, b) ((a) = (b))
#   define INSTRUMENT_ADD(a, b) (a) = (a) + (b)
#   define INSTRUMENT_INC(a)    (a) = (a) + 1
extern UINT32  PrimeIndex;
extern UINT32  failedAtIteration[10];
extern UINT32  PrimeCounts[3];
extern UINT32  MillerRabinTrials[3];
extern UINT32  totalFieldsSieved[3];
extern UINT32  bitsInFieldAfterSieve[3];
extern UINT32  emptyFieldsSieved[3];
extern UINT32  noPrimeFields[3];
extern UINT32  primesChecked[3];
extern UINT16  lastSievePrime;
#else
#   define INSTRUMENT_SET(a, b)
#   define INSTRUMENT_ADD(a, b)
#   define INSTRUMENT_INC(a)
#endif
#endif // _CRYPT_RAND_H

