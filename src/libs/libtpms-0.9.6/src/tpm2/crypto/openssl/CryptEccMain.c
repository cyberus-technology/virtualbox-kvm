/********************************************************************************/
/*										*/
/*			     	ECC Main					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: CryptEccMain.c $		*/
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

/* 10.2.11 CryptEccMain.c */
/* 10.2.11.1 Includes and Defines */
#include "Tpm.h"
#include "Helpers_fp.h"                // libtpms added
#include "TpmToOsslMath_fp.h"          // libtpms added
#if ALG_ECC
/* This version requires that the new format for ECC data be used */
#if !USE_BN_ECC_DATA
#error "Need to SET USE_BN_ECC_DATA to YES in TpmBuildSwitches.h"
#endif
/* 10.2.11.2 Functions */
#if SIMULATION
void
EccSimulationEnd(
		 void
		 )
{
#if SIMULATION
    // put things to be printed at the end of the simulation here
#endif
}
#endif // SIMULATION
/* 10.2.11.2.1 CryptEccInit() */
/* This function is called at _TPM_Init() */
BOOL
CryptEccInit(
	     void
	     )
{
    return TRUE;
}
/* 10.2.11.2.2 CryptEccStartup() */
/* This function is called at TPM2_Startup(). */
BOOL
CryptEccStartup(
		void
		)
{
    return TRUE;
}
/* 10.2.11.2.3 ClearPoint2B(generic */
/* Initialize the size values of a TPMS_ECC_POINT structure. */
void
ClearPoint2B(
	     TPMS_ECC_POINT      *p          // IN: the point
	     )
{
    if(p != NULL)
	{
	    p->x.t.size = 0;
	    p->y.t.size = 0;
	}
}
/* 10.2.11.2.4 CryptEccGetParametersByCurveId() */
/* This function returns a pointer to the curve data that is associated with the indicated
   curveId. If there is no curve with the indicated ID, the function returns NULL. This function is
   in this module so that it can be called by GetCurve() data. */
/* Return Values Meaning */
/* NULL curve with the indicated TPM_ECC_CURVE is not implemented */
/* non-NULL pointer to the curve data */
LIB_EXPORT const ECC_CURVE *
CryptEccGetParametersByCurveId(
			       TPM_ECC_CURVE       curveId     // IN: the curveID
			       )
{
    int          i;
    for(i = 0; i < ECC_CURVE_COUNT; i++)
	{
	    if(eccCurves[i].curveId == curveId)
		return &eccCurves[i];
	}
    return NULL;
}
/*  10.2.11.2.5 CryptEccGetKeySizeForCurve() */
/* This function returns the key size in bits of the indicated curve */
LIB_EXPORT UINT16
CryptEccGetKeySizeForCurve(
			   TPM_ECC_CURVE            curveId    // IN: the curve
			   )
{
    const ECC_CURVE *curve = CryptEccGetParametersByCurveId(curveId);
    UINT16           keySizeInBits;
    //
    keySizeInBits = (curve != NULL) ? curve->keySizeBits : 0;
    return keySizeInBits;
}
/* 10.2.11.2.6 GetCurveData() */
/* This function returns the a pointer for the parameter data associated with a curve. */
const ECC_CURVE_DATA *
GetCurveData(
	     TPM_ECC_CURVE        curveId     // IN: the curveID
	     )
{
    const ECC_CURVE      *curve = CryptEccGetParametersByCurveId(curveId);
    return (curve != NULL) ? curve->curveData : NULL;
}
/* 10.2.11.2.7	CryptEccGetOID() */
const BYTE *
CryptEccGetOID(
	       TPM_ECC_CURVE       curveId
	       )
{
    const ECC_CURVE         *curve = CryptEccGetParametersByCurveId(curveId);
    return (curve != NULL) ? curve->OID : NULL;
}
/* 10.2.11.2.7 CryptEccGetCurveByIndex() */
/* This function returns the number of the i-th implemented curve. The normal use would be to call
   this function with i starting at 0. When the i is greater than or equal to the number of
   implemented curves, TPM_ECC_NONE is returned. */
LIB_EXPORT TPM_ECC_CURVE
CryptEccGetCurveByIndex(
			UINT16               i
			)
{
    if(i >= ECC_CURVE_COUNT)
	return TPM_ECC_NONE;
    return eccCurves[i].curveId;
}
/* 10.2.11.2.8 CryptEccGetParameter() */
/* This function returns an ECC curve parameter. The parameter is selected by a single character
   designator from the set of {PNABXYH}. */
/* Return Values Meaning */
/* TRUE curve exists and parameter returned */
/* FALSE curve does not exist or parameter selector */
LIB_EXPORT BOOL
CryptEccGetParameter(
		     TPM2B_ECC_PARAMETER     *out,       // OUT: place to put parameter
		     char                     p,         // IN: the parameter selector
		     TPM_ECC_CURVE            curveId    // IN: the curve id
		     )
{
    const ECC_CURVE_DATA    *curve = GetCurveData(curveId);
    bigConst                 parameter = NULL;
    if(curve != NULL)
	{
	    switch(p)
		{
		  case 'p':
		    parameter = CurveGetPrime(curve);
		    break;
		  case 'n':
		    parameter = CurveGetOrder(curve);
		    break;
		  case 'a':
		    parameter = CurveGet_a(curve);
		    break;
		  case 'b':
		    parameter = CurveGet_b(curve);
		    break;
		  case 'x':
		    parameter = CurveGetGx(curve);
		    break;
		  case 'y':
		    parameter = CurveGetGy(curve);
		    break;
		  case 'h':
		    parameter = CurveGetCofactor(curve);
		    break;
		  default:
		    FAIL(FATAL_ERROR_INTERNAL);
		    break;
		}
	}
    // If not debugging and we get here with parameter still NULL, had better
    // not try to convert so just return FALSE instead.
    return (parameter != NULL) ? BnTo2B(parameter, &out->b, 0) : 0;
}
/* 10.2.11.2.9 CryptCapGetECCCurve() */
/* This function returns the list of implemented ECC curves. */
/* Return Values Meaning */
/* YES if no more ECC curve is available */
/* NO if there are more ECC curves not reported */
TPMI_YES_NO
CryptCapGetECCCurve(
		    TPM_ECC_CURVE    curveID,       // IN: the starting ECC curve
		    UINT32           maxCount,      // IN: count of returned curves
		    TPML_ECC_CURVE  *curveList      // OUT: ECC curve list
		    )
{
    TPMI_YES_NO       more = NO;
    UINT16            i;
    UINT32            count = ECC_CURVE_COUNT;
    TPM_ECC_CURVE     curve;
    // Initialize output property list
    curveList->count = 0;
    // The maximum count of curves we may return is MAX_ECC_CURVES
    if(maxCount > MAX_ECC_CURVES) maxCount = MAX_ECC_CURVES;
    // Scan the eccCurveValues array
    for(i = 0; i < count; i++)
	{
	    curve = CryptEccGetCurveByIndex(i);
	    // If curveID is less than the starting curveID, skip it
	    if(curve < curveID)
		continue;
            if (!CryptEccIsCurveRuntimeUsable(curve)) // libtpms added: runtime filter supported curves
                continue;
	    if(curveList->count < maxCount)
		{
		    // If we have not filled up the return list, add more curves to
		    // it
		    curveList->eccCurves[curveList->count] = curve;
		    curveList->count++;
		}
	    else
		{
		    // If the return list is full but we still have curves
		    // available, report this and stop iterating
		    more = YES;
		    break;
		}
	}
    return more;
}
/* 10.2.11.2.10 CryptGetCurveSignScheme() */
/* This function will return a pointer to the scheme of the curve. */
const TPMT_ECC_SCHEME *
CryptGetCurveSignScheme(
			TPM_ECC_CURVE    curveId        // IN: The curve selector
			)
{
    const ECC_CURVE         *curve = CryptEccGetParametersByCurveId(curveId);
    if(curve != NULL)
	return &(curve->sign);
    else
	return NULL;
}
/* 10.2.11.2.11 CryptGenerateR() */
/* This function computes the commit random value for a split signing scheme. */
/* If c is NULL, it indicates that r is being generated for TPM2_Commit(). If c is not NULL, the TPM
   will validate that the gr.commitArray bit associated with the input value of c is SET. If not,
   the TPM returns FALSE and no r value is generated. */
/* Return Values Meaning */
/* TRUE r value computed */
/* FALSE no r value computed */
BOOL
CryptGenerateR(
	       TPM2B_ECC_PARAMETER     *r,             // OUT: the generated random value
	       UINT16                  *c,             // IN/OUT: count value.
	       TPMI_ECC_CURVE           curveID,       // IN: the curve for the value
	       TPM2B_NAME              *name           // IN: optional name of a key to
	       //     associate with 'r'
	       )
{
    // This holds the marshaled g_commitCounter.
    TPM2B_TYPE(8B, 8);
    TPM2B_8B                 cntr = {{8,{0}}};
    UINT32                   iterations;
    TPM2B_ECC_PARAMETER      n;
    UINT64                   currentCount = gr.commitCounter;
    UINT16                   t1;
    //
    if(!CryptEccGetParameter(&n, 'n', curveID))
	return FALSE;
    // If this is the commit phase, use the current value of the commit counter
    if(c != NULL)
	{
	    // if the array bit is not set, can't use the value.
	    if(!TEST_BIT((*c & COMMIT_INDEX_MASK), gr.commitArray))
		return FALSE;
	    // If it is the sign phase, figure out what the counter value was
	    // when the commitment was made.
	    //
	    // When gr.commitArray has less than 64K bits, the extra
	    // bits of 'c' are used as a check to make sure that the
	    // signing operation is not using an out of range count value
	    t1 = (UINT16)currentCount;
	    // If the lower bits of c are greater or equal to the lower bits of t1
	    // then the upper bits of t1 must be one more than the upper bits
	    // of c
	    if((*c & COMMIT_INDEX_MASK) >= (t1 & COMMIT_INDEX_MASK))
		// Since the counter is behind, reduce the current count
		currentCount = currentCount - (COMMIT_INDEX_MASK + 1);
	    t1 = (UINT16)currentCount;
	    if((t1 & ~COMMIT_INDEX_MASK) != (*c & ~COMMIT_INDEX_MASK))
		return FALSE;
	    // set the counter to the value that was
	    // present when the commitment was made
	    currentCount = (currentCount & 0xffffffffffff0000ULL) | *c; /* libtpms changed */
	}
    // Marshal the count value to a TPM2B buffer for the KDF
    cntr.t.size = sizeof(currentCount);
    UINT64_TO_BYTE_ARRAY(currentCount, cntr.t.buffer);
    // Now can do the KDF to create the random value for the signing operation
    // During the creation process, we may generate an r that does not meet the
    // requirements of the random value.
    // want to generate a new r.
    r->t.size = n.t.size;
    for(iterations = 1; iterations < 1000000;)
	{
	    int     i;

	    CryptKDFa(CONTEXT_INTEGRITY_HASH_ALG, &gr.commitNonce.b, COMMIT_STRING,
		      (TPM2B *)name, &cntr.b, n.t.size * 8,	// libtpms ubsan
		      r->t.buffer, &iterations, FALSE);		// libtpms changed

	    // "random" value must be less than the prime
	    if(UnsignedCompareB(r->b.size, r->b.buffer, n.t.size, n.t.buffer) >= 0)
		continue;

	    // in this implementation it is required that at least bit
	    // in the upper half of the number be set
	    for(i = n.t.size / 2; i >= 0; i--)
		if(r->b.buffer[i] != 0)
		    return TRUE;
	}
    return FALSE;
}
/* 10.2.11.2.12 CryptCommit() */
/* This function is called when the count value is committed. The gr.commitArray value associated
   with the current count value is SET and g_commitCounter is incremented. The low-order 16 bits of
   old value of the counter is returned. */
UINT16
CryptCommit(
	    void
	    )
{
    UINT16      oldCount = (UINT16)gr.commitCounter;
    gr.commitCounter++;
    SET_BIT(oldCount & COMMIT_INDEX_MASK, gr.commitArray);
    return oldCount;
}
/* 10.2.11.2.13 CryptEndCommit() */
/* This function is called when the signing operation using the committed value is completed. It
   clears the gr.commitArray bit associated with the count value so that it can't be used again. */
void
CryptEndCommit(
	       UINT16           c              // IN: the counter value of the commitment
	       )
{
    ClearBit((c & COMMIT_INDEX_MASK), gr.commitArray, sizeof(gr.commitArray));
}
/* 10.2.11.2.14 CryptEccGetParameters() */
/* This function returns the ECC parameter details of the given curve */
/* Return Values Meaning */
/* TRUE success */
/* FALSE unsupported ECC curve ID */
BOOL
CryptEccGetParameters(
		      TPM_ECC_CURVE                curveId,       // IN: ECC curve ID
		      TPMS_ALGORITHM_DETAIL_ECC   *parameters     // OUT: ECC parameters
		      )
{
    const ECC_CURVE             *curve = CryptEccGetParametersByCurveId(curveId);
    const ECC_CURVE_DATA        *data;
    BOOL                         found = curve != NULL;
    if(found)
	{
	    data = curve->curveData;
	    parameters->curveID = curve->curveId;
	    parameters->keySize = curve->keySizeBits;
	    parameters->kdf = curve->kdf;
	    parameters->sign = curve->sign;
	    /* BnTo2B(data->prime, &parameters->p.b, 0); */
	    BnTo2B(data->prime, &parameters->p.b, parameters->p.t.size);
	    BnTo2B(data->a, &parameters->a.b, parameters->p.t.size /* libtpms changed for HLK */);
	    BnTo2B(data->b, &parameters->b.b, parameters->p.t.size /* libtpms changed for HLK */);
	    BnTo2B(data->base.x, &parameters->gX.b, parameters->p.t.size);
	    BnTo2B(data->base.y, &parameters->gY.b, parameters->p.t.size);
	    BnTo2B(data->order, &parameters->n.b, 0);
	    BnTo2B(data->h, &parameters->h.b, 0);
	}
    return found;
}
/* 10.2.11.2.15 BnGetCurvePrime() */
/* This function is used to get just the prime modulus associated with a curve */
const bignum_t *
BnGetCurvePrime(
		TPM_ECC_CURVE            curveId
		)
{
    const ECC_CURVE_DATA    *C = GetCurveData(curveId);
    return (C != NULL) ? CurveGetPrime(C) : NULL;
}
/* 10.2.11.2.16 BnGetCurveOrder() */
/* This function is used to get just the curve order */
const bignum_t *
BnGetCurveOrder(
		TPM_ECC_CURVE            curveId
		)
{
    const ECC_CURVE_DATA    *C = GetCurveData(curveId);
    return (C != NULL) ? CurveGetOrder(C) : NULL;
}
/* 10.2.11.2.17 BnIsOnCurve() */
/* This function checks if a point is on the curve. */
BOOL
BnIsOnCurve(
	    pointConst                   Q,
	    const ECC_CURVE_DATA        *C
	    )
{
    BN_VAR(right, (MAX_ECC_KEY_BITS * 3));
    BN_VAR(left, (MAX_ECC_KEY_BITS * 2));
    bigConst                   prime = CurveGetPrime(C);
    //
    // Show that point is on the curve y^2 = x^3 + ax + b;
    // Or y^2 = x(x^2 + a) + b
    // y^2
    BnMult(left, Q->y, Q->y);
    BnMod(left, prime);
    // x^2
    BnMult(right, Q->x, Q->x);
    // x^2 + a
    BnAdd(right, right, CurveGet_a(C));
    //    BnMod(right, CurveGetPrime(C));
    // x(x^2 + a)
    BnMult(right, right, Q->x);
    // x(x^2 + a) + b
    BnAdd(right, right, CurveGet_b(C));
    BnMod(right, prime);
    if(BnUnsignedCmp(left, right) == 0)
	return TRUE;
    else
	return FALSE;
}
/* 10.2.11.2.18 BnIsValidPrivateEcc() */
/* Checks that 0 < x < q */
BOOL
BnIsValidPrivateEcc(
		    bigConst                 x,         // IN: private key to check
		    bigCurve                 E          // IN: the curve to check
		    )
{
    BOOL        retVal;
    retVal = (!BnEqualZero(x)
	      && (BnUnsignedCmp(x, CurveGetOrder(AccessCurveData(E))) < 0));
    return retVal;
}
LIB_EXPORT BOOL
CryptEccIsValidPrivateKey(
			  TPM2B_ECC_PARAMETER     *d,
			  TPM_ECC_CURVE            curveId
			  )
{
    BN_INITIALIZED(bnD, MAX_ECC_PARAMETER_BYTES * 8, d);
    return !BnEqualZero(bnD) && (BnUnsignedCmp(bnD, BnGetCurveOrder(curveId)) < 0);
}
/* 10.2.11.2.19 BnPointMul() */
/* This function does a point multiply of the form R = [d]S + [u]Q where the parameters are bigNum
   values. If S is NULL and d is not NULL, then it computes R = [d]G + [u]Q or just R = [d]G if u
   and Q are NULL. If skipChecks is TRUE, then the function will not verify that the inputs are
   correct for the domain. This would be the case when the values were created by the CryptoEngine()
   code. It will return TPM_RC_NO_RESULT if the resulting point is the point at infinity. */
/* Error Returns Meaning */
/* TPM_RC_NO_RESULT result of multiplication is a point at infinity */
/* TPM_RC_ECC_POINT S or Q is not on the curve */
/* TPM_RC_VALUE d or u is not < n */
TPM_RC
BnPointMult(
	    bigPoint             R,         // OUT: computed point
	    pointConst           S,         // IN: optional point to multiply by 'd'
	    bigConst             d,         // IN: scalar for [d]S or [d]G
	    pointConst           Q,         // IN: optional second point
	    bigConst             u,         // IN: optional second scalar
	    bigCurve             E          // IN: curve parameters
	    )
{
    BOOL                 OK;
    //
    TEST(TPM_ALG_ECDH);
    // Need one scalar
    OK = (d != NULL || u != NULL);
    // If S is present, then d has to be present. If S is not
    // present, then d may or may not be present
    OK = OK && (((S == NULL) == (d == NULL)) || (d != NULL));
    // either both u and Q have to be provided or neither can be provided (don't
    // know what to do if only one is provided.
    OK = OK && ((u == NULL) == (Q == NULL));
    OK = OK && (E != NULL);
    if(!OK)
	return TPM_RC_VALUE;
    OK = (S == NULL) || BnIsOnCurve(S, AccessCurveData(E));
    OK = OK && ((Q == NULL) || BnIsOnCurve(Q, AccessCurveData(E)));
    if(!OK)
	return TPM_RC_ECC_POINT;
    if((d != NULL) && (S == NULL))
	S = CurveGetG(AccessCurveData(E));
    // If only one scalar, don't need Shamir's trick
    if((d == NULL) || (u == NULL))
	{
	    if(d == NULL)
		OK = BnEccModMult(R, Q, u, E);
	    else
		OK = BnEccModMult(R, S, d, E);
	}
    else
	{
	    OK = BnEccModMult2(R, S, d, Q, u, E);
	}
    return  (OK ? TPM_RC_SUCCESS : TPM_RC_NO_RESULT);
}
/* 10.2.11.2.20	BnEccGetPrivate() */
/* This function gets random values that are the size of the key plus 64 bits. The value is reduced
   (mod (q - 1)) and incremented by 1 (q is the order of the curve. This produces a value (d) such
   that 1 <= d < q. This is the method of FIPS 186-4 Section B.4.1 'Key Pair Generation Using Extra
   Random Value Meaning */
/* TRUE		success */
/* FALSE	failure generating private key */
#if !USE_OPENSSL_FUNCTIONS_EC          // libtpms added
BOOL
BnEccGetPrivate(
		bigNum                   dOut,      // OUT: the qualified random value
		const ECC_CURVE_DATA    *C,         // IN: curve for which the private key
		//     needs to be appropriate
		RAND_STATE              *rand       // IN: state for DRBG
		)
{
    bigConst                 order = CurveGetOrder(C);
    BOOL                     OK;
    UINT32                   orderBits = BnSizeInBits(order);
    UINT32                   orderBytes = BITS_TO_BYTES(orderBits);
    BN_VAR(bnExtraBits, MAX_ECC_KEY_BITS + 64);
    BN_VAR(nMinus1, MAX_ECC_KEY_BITS);
    //
    OK = BnGetRandomBits(bnExtraBits, (orderBytes * 8) + 64, rand);
    OK = OK && BnSubWord(nMinus1, order, 1);
    OK = OK && BnMod(bnExtraBits, nMinus1);
    OK = OK && BnAddWord(dOut, bnExtraBits, 1);
    return OK && !g_inFailureMode;
}
#else                                  // libtpms added begin
BOOL
BnEccGetPrivate(
		bigNum                   dOut,      // OUT: the qualified random value
		const ECC_CURVE_DATA    *C,         // IN: curve for which the private key
		const EC_GROUP          *G,         // IN: the EC_GROUP to use; must be != NULL for rand == NULL
		BOOL                     noLeadingZeros, // IN: require that all bytes in the private key be set
		                                         //     result may not have leading zero bytes
		//     needs to be appropriate
		RAND_STATE              *rand       // IN: state for DRBG
		)
{
    bigConst                 order = CurveGetOrder(C);
    BOOL                     OK;
    UINT32                   orderBits = BnSizeInBits(order);
    UINT32                   orderBytes = BITS_TO_BYTES(orderBits);
    UINT32                   requestedBits = 0;
    BN_VAR(bnExtraBits, MAX_ECC_KEY_BITS + 64);
    BN_VAR(nMinus1, MAX_ECC_KEY_BITS);

    if (rand == NULL) {
        if (noLeadingZeros)
            requestedBits = orderBits;

        return OpenSSLEccGetPrivate(dOut, G, requestedBits);
    }

    //
    OK = BnGetRandomBits(bnExtraBits, (orderBytes * 8) + 64, rand);
    OK = OK && BnSubWord(nMinus1, order, 1);
    OK = OK && BnMod(bnExtraBits, nMinus1);
    OK = OK && BnAddWord(dOut, bnExtraBits, 1);
    return OK && !g_inFailureMode;
}
#endif // USE_OPENSSL_FUNCTIONS_EC        libtpms added end
/* 10.2.11.2.21 BnEccGenerateKeyPair() */
/* This function gets a private scalar from the source of random bits and does the point multiply to
   get the public key. */
#if !USE_OPENSSL_FUNCTIONS_EC // libtpms added

BOOL
BnEccGenerateKeyPair(
		     bigNum               bnD,            // OUT: private scalar
		     bn_point_t          *ecQ,            // OUT: public point
		     bigCurve             E,              // IN: curve for the point
		     RAND_STATE          *rand            // IN: DRBG state to use
		     )
{
    BOOL                 OK = FALSE;
    // Get a private scalar
    OK = BnEccGetPrivate(bnD, AccessCurveData(E), rand);
    // Do a point multiply
    OK = OK && BnEccModMult(ecQ, NULL, bnD, E);
    if(!OK)
	BnSetWord(ecQ->z, 0);
    else
	BnSetWord(ecQ->z, 1);
    return OK;
}

#else // libtpms added begin

/* In this version of BnEccGenerateKeyPair we take a dual approach to constant
   time requirements: For curves whose order is at the byte boundary, e.g.
   NIST P224/P256/P384, we make sure that bnD has all bytes set (no leading zeros)
   so that OpenSSL BIGNUM code will not reduce the number of bytes and the
   subsequent BnEccModMult() would run faster for a shoter value. For all other
   curves whose order is not at the byte boundary, e.g. NIST P521, we simply
   always add the order of the curve to bnD and call BnEccModMult() with the
   result bnD1, which leads to the same result. */
BOOL
BnEccGenerateKeyPair(
		     bigNum               bnD,            // OUT: private scalar
		     bn_point_t          *ecQ,            // OUT: public point
		     bigCurve             E,              // IN: curve for the point
		     RAND_STATE          *rand            // IN: DRBG state to use
		     )
{
    BOOL                 OK = FALSE;
    bigConst             order = CurveGetOrder(AccessCurveData(E));
    UINT32               orderBits = BnSizeInBits(order);
    BOOL                 atByteBoundary = (orderBits & 7) == 0;
    BOOL                 noLeadingZeros = atByteBoundary;
    ECC_NUM(bnD1);

    // We request that bnD not have leading zeros if it is at byte-boundary,
    // like for example it is the case for NIST P256.
    OK = BnEccGetPrivate(bnD, AccessCurveData(E), E->G, noLeadingZeros, rand);
    if (!atByteBoundary) {
        // for NIST P521 we can add the order to bnD to ensure we have
        // a constant amount of bytes; the result is the same as if we
        // were doing the BnEccModMult() calculation with bnD.
        OK = OK && BnAdd(bnD1, bnD, order);
        OK = OK && BnEccModMult(ecQ, NULL, bnD1, E);
    } else {
        OK = OK && BnEccModMult(ecQ, NULL, bnD, E);
    }

    if(!OK)
	BnSetWord(ecQ->z, 0);
    else
	BnSetWord(ecQ->z, 1);
    return OK;
}

#endif // libtpms added end

/* 10.2.11.2.21 CryptEccNewKeyPair */
/* This function creates an ephemeral ECC. It is ephemeral in that is expected that the private part
   of the key will be discarded */
LIB_EXPORT TPM_RC
CryptEccNewKeyPair(
		   TPMS_ECC_POINT          *Qout,      // OUT: the public point
		   TPM2B_ECC_PARAMETER     *dOut,      // OUT: the private scalar
		   TPM_ECC_CURVE            curveId    // IN: the curve for the key
		   )
{
    CURVE_INITIALIZED(E, curveId);
    POINT(ecQ);
    ECC_NUM(bnD);
    BOOL                    OK;
    if(E == NULL)
	return TPM_RC_CURVE;
    TEST(TPM_ALG_ECDH);
    OK = BnEccGenerateKeyPair(bnD, ecQ, E, NULL);
    if(OK)
	{
	    BnPointTo2B(Qout, ecQ, E);
	    BnTo2B(bnD, &dOut->b, Qout->x.t.size);
	}
    else
	{
	    Qout->x.t.size = Qout->y.t.size = dOut->t.size = 0;
	}
    CURVE_FREE(E);
    return OK ? TPM_RC_SUCCESS : TPM_RC_NO_RESULT;
}
/* 10.2.11.2.22 CryptEccPointMultiply() */
/* This function computes 'R := [dIn]G + [uIn]QIn. Where dIn and uIn are scalars, G and QIn are
   points on the specified curve and G is the default generator of the curve. */
/* The xOut and yOut parameters are optional and may be set to NULL if not used. */
/* It is not necessary to provide uIn if QIn is specified but one of uIn and dIn must be
   provided. If dIn and QIn are specified but uIn is not provided, then R = [dIn]QIn. */
/* If the multiply produces the point at infinity, the TPM_RC_NO_RESULT is returned. */
/* The sizes of xOut and yOut' will be set to be the size of the degree of the curve */
/* It is a fatal error if dIn and uIn are both unspecified (NULL) or if Qin or Rout is
   unspecified. */
/* Error Returns Meaning */
/* TPM_RC_ECC_POINT the point Pin or Qin is not on the curve */
/* TPM_RC_NO_RESULT the product point is at infinity */
/* TPM_RC_CURVE bad curve */
/* TPM_RC_VALUE dIn or uIn out of range */
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
		      )
{
    CURVE_INITIALIZED(E, curveId);
    POINT_INITIALIZED(ecP, Pin);
    ECC_INITIALIZED(bnD, dIn);      // If dIn is null, then bnD is null
    ECC_INITIALIZED(bnU, uIn);
    POINT_INITIALIZED(ecQ, Qin);
    POINT(ecR);
    TPM_RC             retVal;
    //
    retVal = BnPointMult(ecR, ecP, bnD, ecQ, bnU, E);
    if(retVal == TPM_RC_SUCCESS)
	BnPointTo2B(Rout, ecR, E);
    else
	ClearPoint2B(Rout);
    CURVE_FREE(E);
    return retVal;
}
/* 10.2.11.2.23 CryptEccIsPointOnCurve() */
/* This function is used to test if a point is on a defined curve. It does this by checking that y^2
   mod p = x^3 + a*x + b mod p */
/* It is a fatal error if Q is not specified (is NULL). */
/* Return Values Meaning */
/* TRUE point is on curve */
/* FALSE point is not on curve or curve is not supported */
LIB_EXPORT BOOL
CryptEccIsPointOnCurve(
		       TPM_ECC_CURVE            curveId,       // IN: the curve selector
		       TPMS_ECC_POINT          *Qin            // IN: the point.
		       )
{
    const ECC_CURVE_DATA    *C = GetCurveData(curveId);
    POINT_INITIALIZED(ecQ, Qin);
    BOOL            OK;
    //
    pAssert(Qin != NULL);
    OK = (C != NULL && (BnIsOnCurve(ecQ, C)));
    return OK;
}
/* 10.2.11.2.24 CryptEccGenerateKey() */
/* This function generates an ECC key pair based on the input parameters. This routine uses KDFa()
   to produce candidate numbers. The method is according to FIPS 186-3, section B.1.2 "Key Pair
   Generation by Testing Candidates." According to the method in FIPS 186-3, the resulting private
   value d should be 1 <= d < n where n is the order of the base point. */
/* It is a fatal error if Qout, dOut, is not provided (is NULL). */
/* If the curve is not supported If seed is not provided, then a random number will be used for the
   key */
/* Error Returns Meaning */
/* TPM_RC_CURVE curve is not supported */
/* TPM_RC_NO_RESULT could not verify key with signature (FIPS only) */
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
		    )
{
    CURVE_INITIALIZED(E, publicArea->parameters.eccDetail.curveID);
    ECC_NUM(bnD);
    POINT(ecQ);
    BOOL                     OK;
    TPM_RC                   retVal;
    TEST(TPM_ALG_ECDSA); // ECDSA is used to verify each key
    // Validate parameters
    if(E == NULL)
	ERROR_RETURN(TPM_RC_CURVE);
    publicArea->unique.ecc.x.t.size = 0;
    publicArea->unique.ecc.y.t.size = 0;
    sensitive->sensitive.ecc.t.size = 0;
    OK = BnEccGenerateKeyPair(bnD, ecQ, E, rand);
    if(OK)
	{
	    BnPointTo2B(&publicArea->unique.ecc, ecQ, E);
	    BnTo2B(bnD, &sensitive->sensitive.ecc.b, publicArea->unique.ecc.x.t.size);
	}
#if FIPS_COMPLIANT
    // See if PWCT is required
    if(OK && (IS_ATTRIBUTE(publicArea->objectAttributes, TPMA_OBJECT, sign)))
	{
	    ECC_NUM(bnT);
	    ECC_NUM(bnS);
	    TPM2B_DIGEST    digest;
	    TEST(TPM_ALG_ECDSA);
	    digest.t.size = MIN(sensitive->sensitive.ecc.t.size, sizeof(digest.t.buffer));
	    // Get a random value to sign using the built in DRBG state
	    DRBG_Generate(NULL, digest.t.buffer, digest.t.size);
	    if(g_inFailureMode)
		return TPM_RC_FAILURE;
	    BnSignEcdsa(bnT, bnS, E, bnD, &digest, NULL);
	    // and make sure that we can validate the signature
	    OK = BnValidateSignatureEcdsa(bnT, bnS, E, ecQ, &digest) == TPM_RC_SUCCESS;
	}
#endif
    retVal = (OK) ? TPM_RC_SUCCESS : TPM_RC_NO_RESULT;
 Exit:
    CURVE_FREE(E);
    return retVal;
}

//		libtpms added begin
// Support for some curves may be compiled in but they may not be
// supported by openssl's crypto library.
LIB_EXPORT BOOL
CryptEccIsCurveRuntimeUsable(
			     TPMI_ECC_CURVE curveId
			    )
{
    CURVE_INITIALIZED(E, curveId);
    if (E == NULL)
	return FALSE;
    CURVE_FREE(E);
    return TRUE;
}
//		libtpms added end

#endif  // TPM_ALG_ECC
