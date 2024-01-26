/********************************************************************************/
/*										*/
/*			     				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: BnMath_fp.h $			*/
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
/*  (c) Copyright IBM Corp. and others, 2016					*/
/*										*/
/********************************************************************************/

#ifndef BNMATH_FP_H
#define BNMATH_FP_H

LIB_EXPORT BOOL
BnAdd(
      bigNum           result,
      bigConst         op1,
      bigConst         op2
      );
LIB_EXPORT BOOL
BnAddWord(
	  bigNum           result,
	  bigConst         op,
	  crypt_uword_t    word
	  );
LIB_EXPORT BOOL
BnSub(
      bigNum           result,
      bigConst         op1,
      bigConst         op2
      );
LIB_EXPORT BOOL
BnSubWord(
	  bigNum           result,
	  bigConst     op,
	  crypt_uword_t    word
	  );
LIB_EXPORT int
BnUnsignedCmp(
	      bigConst               op1,
	      bigConst               op2
	      );
LIB_EXPORT int
BnUnsignedCmpWord(
		  bigConst             op1,
		  crypt_uword_t        word
		  );
LIB_EXPORT crypt_word_t
BnModWord(
	  bigConst         numerator,
	  crypt_word_t     modulus
	  );
LIB_EXPORT int
Msb(
    crypt_uword_t           word
    );
LIB_EXPORT int
BnMsb(
      bigConst            bn
      );
LIB_EXPORT unsigned
BnSizeInBits(
	     bigConst                 n
	     );
LIB_EXPORT bigNum
BnSetWord(
	  bigNum               n,
	  crypt_uword_t        w
	  );
LIB_EXPORT BOOL
BnSetBit(
	 bigNum           bn,        // IN/OUT: big number to modify
	 unsigned int     bitNum     // IN: Bit number to SET
	 );
LIB_EXPORT BOOL
BnTestBit(
	  bigNum               bn,        // IN: number to check
	  unsigned int         bitNum     // IN: bit to test
	  );
LIB_EXPORT BOOL
BnMaskBits(
	   bigNum           bn,        // IN/OUT: number to mask
	   crypt_uword_t    maskBit    // IN: the bit number for the mask.
	   );
LIB_EXPORT BOOL
BnShiftRight(
	     bigNum           result,
	     bigConst         toShift,
	     uint32_t         shiftAmount
	     );
LIB_EXPORT BOOL
BnGetRandomBits(
		bigNum           n,
		size_t           bits,
		RAND_STATE      *rand
		);
LIB_EXPORT BOOL
BnGenerateRandomInRange(
			bigNum           dest,
			bigConst         limit,
			RAND_STATE      *rand
			);
// libtpms added begin
LIB_EXPORT BOOL
BnGenerateRandomInRangeAllBytes(
			bigNum           dest,
			bigConst         limit,
			RAND_STATE      *rand
			);
// libtpms added end

#endif
