/********************************************************************************/
/*										*/
/*			  TPM to OpenSSL BigNum Shim Layer			*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: TpmToOsslMath_fp.h $	*/
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

#ifndef TPMTOOSSLMATH_FP_H
#define TPMTOOSSLMATH_FP_H

#include <openssl/bn.h>

BOOL
OsslToTpmBn(
	    bigNum          bn,
	    const BIGNUM   *osslBn   // libtpms added 'const'
	    );
BIGNUM *
BigInitialized(
	       BIGNUM             *toInit,
	       bigConst            initializer
	       );
// libtpms added begin
#ifdef VBOX
LIB_EXPORT
#endif
EC_POINT *
EcPointInitialized(
		   pointConst          initializer,
		   bigCurve            E
		   );
// libtpms added end
LIB_EXPORT BOOL
BnModMult(
	  bigNum              result,
	  bigConst            op1,
	  bigConst            op2,
	  bigConst            modulus
	  );
LIB_EXPORT BOOL
BnMult(
       bigNum               result,
       bigConst             multiplicand,
       bigConst             multiplier
       );
LIB_EXPORT BOOL
BnDiv(
      bigNum               quotient,
      bigNum               remainder,
      bigConst             dividend,
      bigConst             divisor
      );
LIB_EXPORT BOOL
BnGcd(
      bigNum      gcd,            // OUT: the common divisor
      bigConst    number1,        // IN:
      bigConst    number2         // IN:
      );
LIB_EXPORT BOOL
BnModExp(
	 bigNum               result,         // OUT: the result
	 bigConst             number,         // IN: number to exponentiate
	 bigConst             exponent,       // IN:
	 bigConst             modulus         // IN:
	 );
LIB_EXPORT BOOL
BnModInverse(
	     bigNum               result,
	     bigConst             number,
	     bigConst             modulus
	     );
bigCurve
BnCurveInitialize(
		  bigCurve          E,           // IN: curve structure to initialize
		  TPM_ECC_CURVE     curveId      // IN: curve identifier
		  );
LIB_EXPORT BOOL
BnEccModMult(
	     bigPoint             R,         // OUT: computed point
	     pointConst           S,         // IN: point to multiply by 'd' (optional)
	     bigConst             d,         // IN: scalar for [d]S
	     bigCurve             E
	     );
LIB_EXPORT BOOL
BnEccModMult2(
	      bigPoint             R,         // OUT: computed point
	      pointConst           S,         // IN: optional point
	      bigConst             d,         // IN: scalar for [d]S or [d]G
	      pointConst           Q,         // IN: second point
	      bigConst             u,         // IN: second scalar
	      bigCurve             E          // IN: curve
	      );
LIB_EXPORT BOOL
BnEccAdd(
	 bigPoint             R,         // OUT: computed point
	 pointConst           S,         // IN: point to multiply by 'd'
	 pointConst           Q,         // IN: second point
	 bigCurve             E          // IN: curve
	 );

#endif
