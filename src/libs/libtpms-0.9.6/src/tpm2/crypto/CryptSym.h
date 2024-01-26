/********************************************************************************/
/*										*/
/*		Implementation of the symmetric block cipher modes 		*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: CryptSym.h $		*/
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
/*  (c) Copyright IBM Corp. and others, 2017 - 2021 				*/
/*										*/
/********************************************************************************/


#ifndef CRYPTSYM_H
#define CRYPTSYM_H

#if ALG_AES
#   define IF_IMPLEMENTED_AES(op)    op(AES, aes)
#else
#   define IF_IMPLEMENTED_AES(op)
#endif
#if ALG_SM4
#   define IF_IMPLEMENTED_SM4(op)    op(SM4, sm4)
#else
#   define IF_IMPLEMENTED_SM4(op)
#endif
#if ALG_CAMELLIA
#   define IF_IMPLEMENTED_CAMELLIA(op)    op(CAMELLIA, camellia)
#else
#   define IF_IMPLEMENTED_CAMELLIA(op)
#endif
#if ALG_TDES
#   define IF_IMPLEMENTED_TDES(op)    op(TDES, tdes)
#else
#   define IF_IMPLEMENTED_TDES(op)
#endif

#define FOR_EACH_SYM(op)		\
    IF_IMPLEMENTED_AES(op)		\
    IF_IMPLEMENTED_SM4(op)		\
    IF_IMPLEMENTED_CAMELLIA(op)		\
    IF_IMPLEMENTED_TDES(op)

						/* libtpms added begin */
#define FOR_EACH_SYM_WITHOUT_TDES(op)	\
    IF_IMPLEMENTED_AES(op)		\
    IF_IMPLEMENTED_SM4(op)		\
    IF_IMPLEMENTED_CAMELLIA(op)			/* libtpms added end */

/* Macros for creating the key schedule union */

#define     KEY_SCHEDULE(SYM, sym)      tpmKeySchedule##SYM sym;
//#define     TDES    DES[3]			/* libtpms commented */
typedef union tpmCryptKeySchedule_t {
    FOR_EACH_SYM_WITHOUT_TDES(KEY_SCHEDULE)	/* libtpms changed from FOR_EACH_SYM */
    tpmKeyScheduleTDES  tdes[3];		/* libtpms added */

#if SYMMETRIC_ALIGNMENT == 8
    uint64_t            alignment;
#else
    uint32_t            alignment;
#endif
} tpmCryptKeySchedule_t;

/* Each block cipher within a library is expected to conform to the same calling conventions with
   three parameters (keySchedule, in, and out) in the same order. That means that all algorithms
   would use the same order of the same parameters. The code is written assuming the (keySchedule,
   in, and out) order. However, if the library uses a different order, the order can be changed with
   a SWIZZLE macro that puts the parameters in the correct order. Note that all algorithms have to
   use the same order and number of parameters because the code to build the calling list is common
   for each call to encrypt or decrypt with the algorithm chosen by setting a function pointer to
   select the algorithm that is used. */
#   define ENCRYPT(keySchedule, in, out)	\
    encrypt(SWIZZLE(keySchedule, in, out))
#   define DECRYPT(keySchedule, in, out)	\
    decrypt(SWIZZLE(keySchedule, in, out))

/* Note that the macros rely on encrypt as local values in the functions that use these
   macros. Those parameters are set by the macro that set the key schedule to be used for the
   call. */

#define ENCRYPT_CASE(ALG, alg)						\
    case TPM_ALG_##ALG:							\
    TpmCryptSetEncryptKey##ALG(key, keySizeInBits, &keySchedule.alg);	\
    encrypt = (TpmCryptSetSymKeyCall_t)TpmCryptEncrypt##ALG;		\
    break;
#define DECRYPT_CASE(ALG, alg)						\
    case TPM_ALG_##ALG:							\
    TpmCryptSetDecryptKey##ALG(key, keySizeInBits, &keySchedule.alg);	\
    decrypt = (TpmCryptSetSymKeyCall_t)TpmCryptDecrypt##ALG;		\
    break;

#endif
