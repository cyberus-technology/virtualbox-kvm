/********************************************************************************/
/*										*/
/*			     	ECC Main					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: CryptEccMain_fp.h $	*/
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

#ifndef CRYPTECCMAIN_FP_H
#define CRYPTECCMAIN_FP_H

void
EccSimulationEnd(
		 void
		 );
BOOL
CryptEccInit(
	     void
	     );
BOOL
CryptEccStartup(
		void
		);
void
ClearPoint2B(
	     TPMS_ECC_POINT      *p          // IN: the point
	     );
LIB_EXPORT const ECC_CURVE *
CryptEccGetParametersByCurveId(
			       TPM_ECC_CURVE       curveId     // IN: the curveID
			       );
LIB_EXPORT UINT16
CryptEccGetKeySizeForCurve(
			   TPM_ECC_CURVE            curveId    // IN: the curve
			   );
const ECC_CURVE_DATA *
GetCurveData(
	     TPM_ECC_CURVE        curveId     // IN: the curveID
	     );
const BYTE *
CryptEccGetOID(
	       TPM_ECC_CURVE       curveId
	       );
LIB_EXPORT TPM_ECC_CURVE
CryptEccGetCurveByIndex(
			UINT16               i
			);
LIB_EXPORT BOOL
CryptEccGetParameter(
		     TPM2B_ECC_PARAMETER     *out,       // OUT: place to put parameter
		     char                     p,         // IN: the parameter selector
		     TPM_ECC_CURVE            curveId    // IN: the curve id
		     );
TPMI_YES_NO
CryptCapGetECCCurve(
		    TPM_ECC_CURVE    curveID,       // IN: the starting ECC curve
		    UINT32           maxCount,      // IN: count of returned curves
		    TPML_ECC_CURVE  *curveList      // OUT: ECC curve list
		    );
const TPMT_ECC_SCHEME *
CryptGetCurveSignScheme(
			TPM_ECC_CURVE    curveId        // IN: The curve selector
			);
BOOL
CryptGenerateR(
	       TPM2B_ECC_PARAMETER     *r,             // OUT: the generated random value
	       UINT16                  *c,             // IN/OUT: count value.
	       TPMI_ECC_CURVE           curveID,       // IN: the curve for the value
	       TPM2B_NAME              *name           // IN: optional name of a key to
	       //     associate with 'r'
	       );
UINT16
CryptCommit(
	    void
	    );
void
CryptEndCommit(
	       UINT16           c              // IN: the counter value of the commitment
	       );
BOOL
CryptEccGetParameters(
		      TPM_ECC_CURVE                curveId,       // IN: ECC curve ID
		      TPMS_ALGORITHM_DETAIL_ECC   *parameters     // OUT: ECC parameters
		      );
const bignum_t *
BnGetCurvePrime(
		TPM_ECC_CURVE            curveId
		);
const bignum_t *
BnGetCurveOrder(
		TPM_ECC_CURVE            curveId
		);
BOOL
BnIsOnCurve(
	    pointConst                   Q,
	    const ECC_CURVE_DATA        *C
	    );
BOOL
BnIsValidPrivateEcc(
		    bigConst                 x,         // IN: private key to check
		    bigCurve                 E          // IN: the curve to check
		    );
LIB_EXPORT BOOL
CryptEccIsValidPrivateKey(
			  TPM2B_ECC_PARAMETER     *d,
			  TPM_ECC_CURVE            curveId
			  );
TPM_RC
BnPointMult(
	    bigPoint             R,         // OUT: computed point
	    pointConst           S,         // IN: optional point to multiply by 'd'
	    bigConst             d,         // IN: scalar for [d]S or [d]G
	    pointConst           Q,         // IN: optional second point
	    bigConst             u,         // IN: optional second scalar
	    bigCurve             E          // IN: curve parameters
	    );
BOOL
BnEccGetPrivate(
		bigNum                   dOut,      // OUT: the qualified random value
		const ECC_CURVE_DATA    *C,         // IN: curve for which the private key
#if USE_OPENSSL_FUNCTIONS_EC
		const EC_GROUP          *G,         // IN: the EC_GROUP to use; must be != NULL for rand == NULL
		BOOL                     noLeadingZeros, // IN: require that all bytes in the private key be set
                                                         //     result may not have leading zero bytes
#endif
		//     needs to be appropriate
		RAND_STATE              *rand       // IN: state for DRBG
		);
BOOL
BnEccGenerateKeyPair(
		     bigNum               bnD,            // OUT: private scalar
		     bn_point_t          *ecQ,            // OUT: public point
		     bigCurve             E,              // IN: curve for the point
		     RAND_STATE          *rand            // IN: DRBG state to use
		     );
LIB_EXPORT TPM_RC
CryptEccNewKeyPair(
		   TPMS_ECC_POINT          *Qout,      // OUT: the public point
		   TPM2B_ECC_PARAMETER     *dOut,      // OUT: the private scalar
		   TPM_ECC_CURVE            curveId    // IN: the curve for the key
		   );
LIB_EXPORT TPM_RC
CryptEccPointMultiply(
		      TPMS_ECC_POINT      *Rout,              // OUT: the product point R
		      TPM_ECC_CURVE        curveId,           // IN: the curve to use
		      TPMS_ECC_POINT      *Pin,               // IN: first point (can be null)
		      TPM2B_ECC_PARAMETER *dIn,               // IN: scalar value for [dIn]Qin
		      //     the Pin
		      TPMS_ECC_POINT      *Qin,               // IN: point Q
		      TPM2B_ECC_PARAMETER *uIn                // IN: scalar value for the multiplier
		      //     of Q
		      );
LIB_EXPORT BOOL
CryptEccIsPointOnCurve(
		       TPM_ECC_CURVE            curveId,       // IN: the curve selector
		       TPMS_ECC_POINT          *Qin            // IN: the point.
		       );
LIB_EXPORT TPM_RC
CryptEccGenerateKey(
		    TPMT_PUBLIC         *publicArea,        // IN/OUT: The public area template for
		    //      the new key. The public key
		    //      area will be replaced computed
		    //      ECC public key
		    TPMT_SENSITIVE      *sensitive,         // OUT: the sensitive area will be
		    //      updated to contain the private
		    //      ECC key and the symmetric
		    //      encryption key
		    RAND_STATE          *rand               // IN: if not NULL, the deterministic
		    //     RNG state
		    );

// 		libtpms added begin
LIB_EXPORT BOOL
CryptEccIsCurveRuntimeUsable(
			     TPMI_ECC_CURVE curveId
			    );
//		libtpms added end

#endif
