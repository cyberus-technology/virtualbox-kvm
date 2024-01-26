/********************************************************************************/
/*										*/
/*			    PRNG Test Vectors 					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: PRNG_TestVectors.h $	*/
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
/*  (c) Copyright IBM Corp. and others, 2016 - 2019				*/
/*										*/
/********************************************************************************/

/* 10.1.17	PRNG_TestVectors.h */

#ifndef     _MSBN_DRBG_TEST_VECTORS_H
#define     _MSBN_DRBG_TEST_VECTORS_H
//#if DRBG_ALGORITHM == TPM_ALG_AES && DRBG_KEY_BITS == 256
#if DRBG_KEY_SIZE_BITS == 256

/* Entropy is the size of the state. The state is the size of the key plus the IV. The IV is a
   block. If Key = 256 and Block = 128 then State = 384 */

#define DRBG_TEST_INITIATE_ENTROPY				\
        0x0d, 0x15, 0xaa, 0x80, 0xb1, 0x6c, 0x3a, 0x10,		\
	0x90, 0x6c, 0xfe, 0xdb, 0x79, 0x5d, 0xae, 0x0b,		\
	0x5b, 0x81, 0x04, 0x1c, 0x5c, 0x5b, 0xfa, 0xcb,		\
	0x37, 0x3d, 0x44, 0x40, 0xd9, 0x12, 0x0f, 0x7e,		\
	0x3d, 0x6c, 0xf9, 0x09, 0x86, 0xcf, 0x52, 0xd8,		\
	0x5d, 0x3e, 0x94, 0x7d, 0x8c, 0x06, 0x1f, 0x91
#define DRBG_TEST_RESEED_ENTROPY				\
        0x6e, 0xe7, 0x93, 0xa3, 0x39, 0x55, 0xd7, 0x2a,		\
	0xd1, 0x2f, 0xd8, 0x0a, 0x8a, 0x3f, 0xcf, 0x95,		\
	0xed, 0x3b, 0x4d, 0xac, 0x57, 0x95, 0xfe, 0x25,		\
	0xcf, 0x86, 0x9f, 0x7c, 0x27, 0x57, 0x3b, 0xbc,		\
	0x56, 0xf1, 0xac, 0xae, 0x13, 0xa6, 0x50, 0x42,		\
	0xb3, 0x40, 0x09, 0x3c, 0x46, 0x4a, 0x7a, 0x22
#define DRBG_TEST_GENERATED_INTERM				\
        0x28, 0xe0, 0xeb, 0xb8, 0x21, 0x01, 0x66, 0x50,		\
	0x8c, 0x8f, 0x65, 0xf2, 0x20, 0x7b, 0xd0, 0xa3
#define DRBG_TEST_GENERATED					\
        0x94, 0x6f, 0x51, 0x82, 0xd5, 0x45, 0x10, 0xb9,		\
	0x46, 0x12, 0x48, 0xf5, 0x71, 0xca, 0x06, 0xc9
#elif DRBG_KEY_SIZE_BITS == 128
#define DRBG_TEST_INITIATE_ENTROPY				\
        0x8f, 0xc1, 0x1b, 0xdb, 0x5a, 0xab, 0xb7, 0xe0,		\
	0x93, 0xb6, 0x14, 0x28, 0xe0, 0x90, 0x73, 0x03,		\
	0xcb, 0x45, 0x9f, 0x3b, 0x60, 0x0d, 0xad, 0x87,		\
	0x09, 0x55, 0xf2, 0x2d, 0xa8, 0x0a, 0x44, 0xf8
#define DRBG_TEST_RESEED_ENTROPY				\
        0x0c, 0xd5, 0x3c, 0xd5, 0xec, 0xcd, 0x5a, 0x10,		\
	0xd7, 0xea, 0x26, 0x61, 0x11, 0x25, 0x9b, 0x05,		\
	0x57, 0x4f, 0xc6, 0xdd, 0xd8, 0xbe, 0xd8, 0xbd,		\
	0x72, 0x37, 0x8c, 0xf8, 0x2f, 0x1d, 0xba, 0x2a
#define DRBG_TEST_GENERATED_INTERM				\
        0xdc, 0x3c, 0xf6, 0xbf, 0x5b, 0xd3, 0x41, 0x13,		\
	0x5f, 0x2c, 0x68, 0x11, 0xa1, 0x07, 0x1c, 0x87
#define DRBG_TEST_GENERATED					\
        0xb6, 0x18, 0x50, 0xde, 0xcf, 0xd7, 0x10, 0x6d,		\
	0x44, 0x76, 0x9a, 0x8e, 0x6e, 0x8c, 0x1a, 0xd4
#endif
#endif      //     _MSBN_DRBG_TEST_VECTORS_H
