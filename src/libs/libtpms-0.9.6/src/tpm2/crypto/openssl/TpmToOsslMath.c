/********************************************************************************/
/*										*/
/*			 TPM to OpenSSL BigNum Shim Layer			*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: TpmToOsslMath.c $		*/
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

/* B.2.3.2. TpmToOsslMath.c */
/* B.2.3.2.1. Introduction */

/* The functions in this file provide the low-level interface between the TPM code and the big
   number and elliptic curve math routines in OpenSSL. */
/* Most math on big numbers require a context. The context contains the memory in which OpenSSL
   creates and manages the big number values. When a OpenSSL math function will be called that
   modifies a BIGNUM value, that value must be created in an OpenSSL context. The first line of code
   in such a function must be: OSSL_ENTER(); and the last operation before returning must be
   OSSL_LEAVE(). OpenSSL variables can then be created with BnNewVariable(). Constant values to be
   used by OpenSSL are created from the bigNum values passed to the functions in this file. Space
   for the BIGNUM control block is allocated in the stack of the function and then it is initialized
   by calling BigInitialized(). That function sets up the values in the BIGNUM structure and sets
   the data pointer to point to the data in the bignum_t. This is only used when the value is known
   to be a constant in the called function. */
/* Because the allocations of constants is on the local stack and the OSSL_ENTER()/OSSL_LEAVE() pair
   flushes everything created in OpenSSL memory, there should be no chance of a memory leak. */


#include "Tpm.h"
#ifdef MATH_LIB_OSSL
#include "TpmToOsslMath_fp.h"

/* B.2.3.2.3.1.	OsslToTpmBn() */
/* This function converts an OpenSSL BIGNUM to a TPM bignum. In this implementation it is assumed
   that OpenSSL uses a different control structure but the same data layout -- an array of
   native-endian words in little-endian order. */
/* Return Value	Meaning */
/* TRUE(1)	success */
/* FALSE(0)	failure because value will not fit or OpenSSL variable doesn't exist */
BOOL
OsslToTpmBn(
	    bigNum          bn,
	    const BIGNUM   *osslBn   // libtpms: added 'const'
	    )
{
    VERIFY(osslBn != NULL);
    // If the bn is NULL, it means that an output value pointer was NULL meaning that
    // the results is simply to be discarded.
    unsigned char buffer[LARGEST_NUMBER + 1];	// libtpms added
    int buffer_len;				// libtpms added

    if(bn != NULL)
	{
#if 1 //libtpms added begin
	    int num_bytes;

	    num_bytes = BN_num_bytes(osslBn);
	    VERIFY(num_bytes >= 0 && sizeof(buffer) >= (size_t)num_bytes);
	    buffer_len = BN_bn2bin(osslBn, buffer);	/* ossl to bin */
	    BnFromBytes(bn, buffer, buffer_len);	/* bin to TPM */
#else // libtpms added end
	    int         i;
	    //
	    VERIFY((unsigned)osslBn->top <= BnGetAllocated(bn));
	    for(i = 0; i < osslBn->top; i++)
		bn->d[i] = osslBn->d[i];
	    BnSetTop(bn, osslBn->top);
#endif // libtpms added
	}
    return TRUE;
 Error:
    return FALSE;
}

/* B.2.3.2.3.2.	BigInitialized() */
/* This function initializes an OSSL BIGNUM from a TPM bigConst. Do not use this for values that are
   passed to OpenSLL when they are not declared as const in the function prototype. Instead, use
   BnNewVariable(). */
BIGNUM *
BigInitialized(
	       BIGNUM             *toInit,
	       bigConst            initializer
	       )
{
#if 1  // libtpms added begin
    BIGNUM *_toInit;
    unsigned char buffer[LARGEST_NUMBER + 1];
    NUMBYTES buffer_len = (NUMBYTES )sizeof(buffer);
#endif // libtpms added end

    if(initializer == NULL)
	FAIL(FATAL_ERROR_PARAMETER);
    if(toInit == NULL || initializer == NULL)
	return NULL;

#if 1  // libtpms added begin
    BnToBytes(initializer, buffer, &buffer_len);	/* TPM to bin */
    _toInit = BN_bin2bn(buffer, buffer_len, NULL);	/* bin to ossl */
    BN_set_flags(_toInit, BN_FLG_CONSTTIME);
    BN_copy(toInit, _toInit);
    BN_clear_free(_toInit);
#else  // libtpms added end
    toInit->d = (BN_ULONG *)&initializer->d[0];
    toInit->dmax = (int)initializer->allocated;
    toInit->top = (int)initializer->size;
    toInit->neg = 0;
    toInit->flags = 0;
#endif
    return toInit;
}

#ifndef OSSL_DEBUG
#   define BIGNUM_PRINT(label, bn, eol)
#   define DEBUG_PRINT(x)
#else
#   define DEBUG_PRINT(x)   printf("%s", x)
#   define BIGNUM_PRINT(label, bn, eol) BIGNUM_print((label), (bn), (eol))

static
void BIGNUM_print(
		  const char      *label,
		  const BIGNUM    *a,
		  BOOL             eol
		  )
{
    BN_ULONG        *d;
    int              i;
    int              notZero = FALSE;
    if(label != NULL)
	printf("%s", label);
    if(a == NULL)
	{
	    printf("NULL");
	    goto done;
	}
    if (a->neg)
	printf("-");
    for(i = a->top, d = &a->d[i - 1]; i > 0; i--)
	{
	    int         j;
	    BN_ULONG    l = *d--;
	    for(j = BN_BITS2 - 8; j >= 0; j -= 8)
		{
		    BYTE    b = (BYTE)((l >> j) & 0xFF);
		    notZero = notZero || (b != 0);
		    if(notZero)
			printf("%02x", b);
		}
	    if(!notZero)
		printf("0");
	}
 done:
    if(eol)
	printf("\n");
    return;
}
#endif

/* B.2.3.2.3.4. BnNewVariable() */
/* This function allocates a new variable in the provided context. If the context does not exist or
   the allocation fails, it is a catastrophic failure. */
static BIGNUM *
BnNewVariable(
	      BN_CTX          *CTX
	      )
{
    BIGNUM          *new;
    //
    // This check is intended to protect against calling this function without
    // having initialized the CTX.
    if((CTX == NULL) || ((new = BN_CTX_get(CTX)) == NULL))
	FAIL(FATAL_ERROR_ALLOCATION);
    return new;
}

#if LIBRARY_COMPATIBILITY_CHECK

BOOL
MathLibraryCompatibilityCheck(
			      void
			      )
{
    OSSL_ENTER();
    BIGNUM              *osslTemp = BnNewVariable(CTX);
#if 0
    crypt_uword_t        i;
#endif
    BYTE                 test[] = {0x1F, 0x1E, 0x1D, 0x1C, 0x1B, 0x1A, 0x19, 0x18,
				   0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10,
				   0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
				   0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00};
    BN_VAR(tpmTemp, sizeof(test) * 8); // allocate some space for a test value
    //
    // Convert the test data to a bigNum
    BnFromBytes(tpmTemp, test, sizeof(test));
    // Convert the test data to an OpenSSL BIGNUM
    BN_bin2bn(test, sizeof(test), osslTemp);
    // Make sure the values are consistent
#if 0
    VERIFY(osslTemp->top == (int)tpmTemp->size);
    for(i = 0; i < tpmTemp->size; i++)
	VERIFY(osslTemp->d[i] == tpmTemp->d[i]);
#endif
    OSSL_LEAVE();
    return 1;
#if 0
 Error:
    return 0;
#endif
}

#endif

/* B.2.3.2.3.3. BnModMult() */
/* Does multiply and divide returning the remainder of the divide. */
/* Return Value	Meaning */
/* TRUE(1)	success */
/* FALSE(0)	failure in operation */

LIB_EXPORT BOOL
BnModMult(
	  bigNum              result,
	  bigConst            op1,
	  bigConst            op2,
	  bigConst            modulus
	  )
{
    OSSL_ENTER();
    BOOL                OK = TRUE;
    BIGNUM              *bnResult = BN_NEW();
    BIGNUM              *bnTemp = BN_NEW();
    BIG_INITIALIZED(bnOp1, op1);
    BIG_INITIALIZED(bnOp2, op2);
    BIG_INITIALIZED(bnMod, modulus);
    //
    VERIFY(BN_mul(bnTemp, bnOp1, bnOp2, CTX));
    VERIFY(BN_div(NULL, bnResult, bnTemp, bnMod, CTX));
    VERIFY(OsslToTpmBn(result, bnResult));
    goto Exit;
 Error:
    OK = FALSE;
 Exit:
    BN_clear_free(bnMod); // libtpms added
    BN_clear_free(bnOp2); // libtpms added
    BN_clear_free(bnOp1); // libtpms added
    OSSL_LEAVE();
    return OK;
}

/* B.2.3.2.3.4. BnMult() */
/* Multiplies two numbers */
/* Return Value	Meaning */
/* TRUE(1)	success */
/* FALSE(0)	failure in operation */

LIB_EXPORT BOOL
BnMult(
       bigNum               result,
       bigConst             multiplicand,
       bigConst             multiplier
       )
{
    OSSL_ENTER();
    BIGNUM              *bnTemp = BN_NEW();
    BOOL                 OK = TRUE;
    BIG_INITIALIZED(bnA, multiplicand);
    BIG_INITIALIZED(bnB, multiplier);
    //
    VERIFY(BN_mul(bnTemp, bnA, bnB, CTX));
    VERIFY(OsslToTpmBn(result, bnTemp));
    goto Exit;
 Error:
    OK = FALSE;
 Exit:
    BN_clear_free(bnB); // libtpms added
    BN_clear_free(bnA); // libtpms added
    OSSL_LEAVE();
    return OK;
}

/* B.2.3.2.3.5. BnDiv() */
/* This function divides two bigNum values. The function returns FALSE if there is an error in the
   operation. */
/* Return Value	Meaning */
/* TRUE(1)	success */
/* FALSE(0)	failure in operation */

LIB_EXPORT BOOL
BnDiv(
      bigNum               quotient,
      bigNum               remainder,
      bigConst             dividend,
      bigConst             divisor
      )
{
    OSSL_ENTER();
    BIGNUM              *bnQ = BN_NEW();
    BIGNUM              *bnR = BN_NEW();
    BOOL                 OK = TRUE;
    BIG_INITIALIZED(bnDend, dividend);
    BIG_INITIALIZED(bnSor, divisor);
    //
    if(BnEqualZero(divisor))
	FAIL(FATAL_ERROR_DIVIDE_ZERO);
    VERIFY(BN_div(bnQ, bnR, bnDend, bnSor, CTX));
    VERIFY(OsslToTpmBn(quotient, bnQ));
    VERIFY(OsslToTpmBn(remainder, bnR));
    DEBUG_PRINT("In BnDiv:\n");
    BIGNUM_PRINT("   bnDividend: ", bnDend, TRUE);
    BIGNUM_PRINT("    bnDivisor: ", bnSor, TRUE);
    BIGNUM_PRINT("   bnQuotient: ", bnQ, TRUE);
    BIGNUM_PRINT("  bnRemainder: ", bnR, TRUE);
    goto Exit;
 Error:
    OK = FALSE;
 Exit:
    BN_clear_free(bnSor);  // libtpms added
    BN_clear_free(bnDend); // libtpms added
    OSSL_LEAVE();
    return OK;
}

#if ALG_RSA
#if !RSA_KEY_SIEVE		// libtpms added
/* B.2.3.2.3.6. BnGcd() */
/* Get the greatest common divisor of two numbers */
/* Return Value	Meaning */
/* TRUE(1)	success */
/* FALSE(0)	failure in operation */

LIB_EXPORT BOOL
BnGcd(
      bigNum      gcd,            // OUT: the common divisor
      bigConst    number1,        // IN:
      bigConst    number2         // IN:
      )
{
    OSSL_ENTER();
    BIGNUM              *bnGcd = BN_NEW();
    BOOL                 OK = TRUE;
    BIG_INITIALIZED(bn1, number1);
    BIG_INITIALIZED(bn2, number2);
    //
    BN_set_flags(bn1, BN_FLG_CONSTTIME); // number1 is secret prime number
    VERIFY(BN_gcd(bnGcd, bn1, bn2, CTX));
    VERIFY(OsslToTpmBn(gcd, bnGcd));
    goto Exit;
 Error:
    OK = FALSE;
 Exit:
    BN_clear_free(bn2);  // libtpms added
    BN_clear_free(bn1);  // libtpms added
    OSSL_LEAVE();
    return OK;
}
#endif				// libtpms added

/* B.2.3.2.3.7. BnModExp() */
/* Do modular exponentiation using bigNum values. The conversion from a bignum_t to a bigNum is
   trivial as they are based on the same structure */
/* Return Value	Meaning */
/* TRUE(1)	success */
/* FALSE(0)	failure in operation */

LIB_EXPORT BOOL
BnModExp(
	 bigNum               result,         // OUT: the result
	 bigConst             number,         // IN: number to exponentiate
	 bigConst             exponent,       // IN:
	 bigConst             modulus         // IN:
	 )
{
    OSSL_ENTER();
    BIGNUM              *bnResult = BN_NEW();
    BOOL                 OK = TRUE;
    BIG_INITIALIZED(bnN, number);
    BIG_INITIALIZED(bnE, exponent);
    BIG_INITIALIZED(bnM, modulus);
    //
    BN_set_flags(bnE, BN_FLG_CONSTTIME); // exponent may be private
    VERIFY(BN_mod_exp(bnResult, bnN, bnE, bnM, CTX));
    VERIFY(OsslToTpmBn(result, bnResult));
    goto Exit;
 Error:
    OK = FALSE;
 Exit:
    BN_clear_free(bnM); // libtpms added
    BN_clear_free(bnE); // libtpms added
    BN_clear_free(bnN); // libtpms added
    OSSL_LEAVE();
    return OK;
}

/* B.2.3.2.3.8. BnModInverse() */
/* Modular multiplicative inverse */
/* Return Value	Meaning */
/* TRUE(1)	success */
/* FALSE(0)	failure in operation */

LIB_EXPORT BOOL
BnModInverse(
	     bigNum               result,
	     bigConst             number,
	     bigConst             modulus
	     )
{
    OSSL_ENTER();
    BIGNUM              *bnResult = BN_NEW();
    BOOL                 OK = TRUE;
    BIG_INITIALIZED(bnN, number);
    BIG_INITIALIZED(bnM, modulus);
    //
    BN_set_flags(bnN, BN_FLG_CONSTTIME); // number may be private
    VERIFY(BN_mod_inverse(bnResult, bnN, bnM, CTX) != NULL);
    VERIFY(OsslToTpmBn(result, bnResult));
    goto Exit;
 Error:
    OK = FALSE;
 Exit:
    BN_clear_free(bnM); // libtpms added
    BN_clear_free(bnN); // libtpms added
    OSSL_LEAVE();
    return OK;
}

#endif // TPM_ALG_RSA

#if ALG_ECC

/* B.2.3.2.3.9. PointFromOssl() */
/* Function to copy the point result from an OSSL function to a bigNum */
/* Return Value	Meaning */
/* TRUE(1)	success */
/* FALSE(0)	failure in operation */
static BOOL
PointFromOssl(
	      bigPoint         pOut,      // OUT: resulting point
	      EC_POINT        *pIn,       // IN: the point to return
	      bigCurve         E          // IN: the curve
	      )
{
    BIGNUM         *x = NULL;
    BIGNUM         *y = NULL;
    BOOL            OK;
    BN_CTX_start(E->CTX);
    //
    x = BN_CTX_get(E->CTX);
    y = BN_CTX_get(E->CTX);
    if(y == NULL)
	FAIL(FATAL_ERROR_ALLOCATION);
    // If this returns false, then the point is at infinity
#if OPENSSL_VERSION_NUMBER >= 0x10101000L && \
    !defined(LIBRESSL_VERSION_NUMBER)					// libtpms added begin VBox: fixed openssl version number check from 0x10100000L
    OK = EC_POINT_get_affine_coordinates(E->G, pIn, x, y, E->CTX);
#else									// libtpms added begin
    OK = EC_POINT_get_affine_coordinates_GFp(E->G, pIn, x, y, E->CTX);
#endif									// libtpms added end
    if(OK)
	{
	    OsslToTpmBn(pOut->x, x);
	    OsslToTpmBn(pOut->y, y);
	    BnSetWord(pOut->z, 1);
	}
    else
	BnSetWord(pOut->z, 0);
    BN_CTX_end(E->CTX);
    return OK;
}
/* B.2.3.2.3.10. EcPointInitialized() */
/* Allocate and initialize a point. */

LIB_EXPORT EC_POINT *   // libtpms: exported function
EcPointInitialized(
		   pointConst          initializer,
		   bigCurve            E
		   )
{
    EC_POINT            *P = NULL;
    
    if(initializer != NULL)
	{
	    BIG_INITIALIZED(bnX, initializer->x);
	    BIG_INITIALIZED(bnY, initializer->y);
	    if(E == NULL)
		FAIL(FATAL_ERROR_ALLOCATION);
	    P = EC_POINT_new(E->G);
#if OPENSSL_VERSION_NUMBER >= 0x10101000L && \
    !defined(LIBRESSL_VERSION_NUMBER)					// libtpms added begin VBox: fixed openssl version number check from 0x10100000L
	    if(!EC_POINT_set_affine_coordinates(E->G, P, bnX, bnY, E->CTX))
#else									// libtpms added end
	    if(!EC_POINT_set_affine_coordinates_GFp(E->G, P, bnX, bnY, E->CTX))
#endif									// libtpms added
		P = NULL;
	    BN_clear_free(bnX); // libtpms added
	    BN_clear_free(bnY); // libtpms added
	}
    return P;
}

/* B.2.3.2.3.11. BnCurveInitialize() */
/* This function initializes the OpenSSL group definition */
/* It is a fatal error if groupContext is not provided. */
/* Return Values Meaning */
/* NULL the TPM_ECC_CURVE is not valid */
/* non-NULL points to a structure in groupContext */

LIB_EXPORT bigCurve
BnCurveInitialize(
		  bigCurve          E,           // IN: curve structure to initialize
		  TPM_ECC_CURVE     curveId      // IN: curve identifier
		  )
{
    const ECC_CURVE_DATA    *C = GetCurveData(curveId);
    if(C == NULL)
	E = NULL;
    if(E != NULL)
	{
	    // This creates the OpenSSL memory context that stays in effect as long as the
	    // curve (E) is defined.
	    OSSL_ENTER();                       // if the allocation fails, the TPM fails
	    EC_POINT                *P = NULL;
	    BIG_INITIALIZED(bnP, C->prime);
	    BIG_INITIALIZED(bnA, C->a);
	    BIG_INITIALIZED(bnB, C->b);
	    BIG_INITIALIZED(bnX, C->base.x);
	    BIG_INITIALIZED(bnY, C->base.y);
	    BIG_INITIALIZED(bnN, C->order);
	    BIG_INITIALIZED(bnH, C->h);
	    //
	    E->C = C;
	    E->CTX = CTX;
	    
	    // initialize EC group, associate a generator point and initialize the point
	    // from the parameter data
	    // Create a group structure
	    E->G = EC_GROUP_new_curve_GFp(bnP, bnA, bnB, CTX);
	    VERIFY(E->G != NULL);
	    
	    // Allocate a point in the group that will be used in setting the
	    // generator. This is not needed after the generator is set.
	    P = EC_POINT_new(E->G);
	    VERIFY(P != NULL);
	    
	    // Need to use this in case Montgomery method is being used
#if OPENSSL_VERSION_NUMBER >= 0x10101000L && \
    !defined(LIBRESSL_VERSION_NUMBER)						// libtpms added begin VBox: fixed openssl version number check from 0x10100000L
	    VERIFY(EC_POINT_set_affine_coordinates(E->G, P, bnX, bnY, CTX));
#else										// libtpms added end
	    VERIFY(EC_POINT_set_affine_coordinates_GFp(E->G, P, bnX, bnY, CTX));
#endif										// libtpms added
	    // Now set the generator
	    VERIFY(EC_GROUP_set_generator(E->G, P, bnN, bnH));
	    
	    EC_POINT_free(P);
	    goto Exit_free;  // libtpms changed
	Error:
	    EC_POINT_free(P);
	    BnCurveFree(E);
	    E = NULL;

 Exit_free:			// libtpms added begin
	    BN_clear_free(bnH);
	    BN_clear_free(bnN);
	    BN_clear_free(bnY);
	    BN_clear_free(bnX);
	    BN_clear_free(bnB);
	    BN_clear_free(bnA);
	    BN_clear_free(bnP); // libtpms added end
	}
// Exit:
    return E;
}

/* B.2.3.2.3.15.	BnCurveFree() */
/* This function will free the allocated components of the curve and end the frame in which the
   curve data exists */
LIB_EXPORT void
BnCurveFree(
	    bigCurve E
	    )
{
    if(E)
	{
	    EC_GROUP_free(E->G);
	    OsslContextLeave(E->CTX);
	}
}

/* B.2.3.2.3.11. BnEccModMult() */
/* This functi2n does a point multiply of the form R = [d]S */
/* Return Values Meaning */
/* FALSE failure in operation; treat as result being point at infinity */

LIB_EXPORT BOOL
BnEccModMult(
	     bigPoint             R,         // OUT: computed point
	     pointConst           S,         // IN: point to multiply by 'd' (optional)
	     bigConst             d,         // IN: scalar for [d]S
	     bigCurve             E
	     )
{
    EC_POINT            *pR = EC_POINT_new(E->G);
    EC_POINT            *pS = EcPointInitialized(S, E);
    BIG_INITIALIZED(bnD, d);
    
    if(S == NULL)
	EC_POINT_mul(E->G, pR, bnD, NULL, NULL, E->CTX);
    else
	EC_POINT_mul(E->G, pR, NULL, pS, bnD, E->CTX);
    PointFromOssl(R, pR, E);
    EC_POINT_clear_free(pR); // libtpms changed
    EC_POINT_clear_free(pS); // libtpms changed
    BN_clear_free(bnD); // libtpms added
    return !BnEqualZero(R->z);
}

/* B.2.3.2.3.13. BnEccModMult2() */
/* This function does a point multiply of the form R = [d]G + [u]Q */
/* FALSE	failure in operation; treat as result being point at infinity */

LIB_EXPORT BOOL
BnEccModMult2(
	      bigPoint             R,         // OUT: computed point
	      pointConst           S,         // IN: optional point
	      bigConst             d,         // IN: scalar for [d]S or [d]G
	      pointConst           Q,         // IN: second point
	      bigConst             u,         // IN: second scalar
	      bigCurve             E          // IN: curve
	      )
{
    EC_POINT            *pR = EC_POINT_new(E->G);
    EC_POINT            *pS = EcPointInitialized(S, E);
    BIG_INITIALIZED(bnD, d);
    EC_POINT            *pQ = EcPointInitialized(Q, E);
    BIG_INITIALIZED(bnU, u);
    
    if(S == NULL || S == (pointConst)&(AccessCurveData(E)->base))
	EC_POINT_mul(E->G, pR, bnD, pQ, bnU, E->CTX);
    else
	{
	    const EC_POINT        *points[2];
	    const BIGNUM          *scalars[2];
	    points[0] = pS;
	    points[1] = pQ;
	    scalars[0] = bnD;
	    scalars[1] = bnU;
	    EC_POINTs_mul(E->G, pR, NULL, 2, points, scalars, E->CTX);
	}
    PointFromOssl(R, pR, E);
    EC_POINT_clear_free(pR); // libtpms changed
    EC_POINT_clear_free(pS); // libtpms changed
    EC_POINT_clear_free(pQ); // libtpms changed
    BN_clear_free(bnD); // libtpms added
    BN_clear_free(bnU); // libtpms added

    return !BnEqualZero(R->z);
}

/* B.2.3.2.4. BnEccAdd() */
/* This function does addition of two points. */
/* Return Values Meaning */
/* FALSE failure in operation; treat as result being point at infinity */
LIB_EXPORT BOOL
BnEccAdd(
	 bigPoint             R,         // OUT: computed point
	 pointConst           S,         // IN: point to multiply by 'd'
	 pointConst           Q,         // IN: second point
	 bigCurve             E          // IN: curve
	 )
{
    EC_POINT            *pR = EC_POINT_new(E->G);
    EC_POINT            *pS = EcPointInitialized(S, E);
    EC_POINT            *pQ = EcPointInitialized(Q, E);
    //
    EC_POINT_add(E->G, pR, pS, pQ, E->CTX);
    PointFromOssl(R, pR, E);
    EC_POINT_clear_free(pR); // libtpms changed
    EC_POINT_clear_free(pS); // libtpms changed
    EC_POINT_clear_free(pQ); // libtpms changed
    return !BnEqualZero(R->z);
}

#endif // ALG_ECC
#endif // MATH_LIB_OSSL
