/********************************************************************************/
/*										*/
/*			Simple Operations on Big Numbers     			*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: BnMath.c $		*/
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

/* 10.2.3 BnMath.c */

/* 10.2.3.1	Introduction */
/* The simulator code uses the canonical form whenever possible in order to make the code in Part 3
   more accessible. The canonical data formats are simple and not well suited for complex big number
   computations. When operating on big numbers, the data format is changed for easier
   manipulation. The format is native words in little-endian format. As the magnitude of the number
   decreases, the length of the array containing the number decreases but the starting address
   doesn't change. */
/* The functions in this file perform simple operations on these big numbers. Only the more complex
   operations are passed to the underlying support library. Although the support library would have
   most of these functions, the interface code to convert the format for the values is greater than
   the size of the code to implement the functions here. So, rather than incur the overhead of
   conversion, they are done here. */
/* If an implementer would prefer, the underlying library can be used simply by making code
   substitutions here. */
/* NOTE: There is an intention to continue to augment these functions so that there would be no need
   to use an external big number library. */
/* Many of these functions have no error returns and will always return TRUE. This is to allow them
   to be used in guarded sequences. That is: OK = OK || BnSomething(s); where the BnSomething()
   function should not be called if OK isn't true. */

/* 10.2.3.2 Includes */
#include "Tpm.h"
/* A constant value of zero as a stand in for NULL bigNum values */
const bignum_t   BnConstZero = {1, 0, {0}};
/* 10.2.3.3 Functions */
/* 10.2.3.3.1 AddSame() */
/* Adds two values that are the same size. This function allows result to be the same as either of
   the addends. This is a nice function to put into assembly because handling the carry for
   multi-precision stuff is not as easy in C (unless there is a REALLY smart compiler). It would be
   nice if there were idioms in a language that a compiler could recognize what is going on and
   optimize loops like this. */
/* Return Values Meaning */
/* 0 no carry out */
/* 1 carry out */
static BOOL
AddSame(
	crypt_uword_t           *result,
	const crypt_uword_t     *op1,
	const crypt_uword_t     *op2,
	int                      count
	)
{
    int         carry = 0;
    int         i;
    for(i = 0; i < count; i++)
	{
	    crypt_uword_t        a = op1[i];
	    crypt_uword_t        sum = a + op2[i];
	    result[i] = sum + carry;
	    // generate a carry if the sum is less than either of the inputs
	    // propagate a carry if there was a carry and the sum + carry is zero
	    // do this using bit operations rather than logical operations so that
	    // the time is about the same.
	    //             propagate term      | generate term
	    carry = ((result[i] == 0) & carry) | (sum < a);
	}
    return carry;
}
/* 10.2.3.3.2 CarryProp() */
/* Propagate a carry */
static int
CarryProp(
	  crypt_uword_t           *result,
	  const crypt_uword_t     *op,
	  int                      count,
	  int                      carry
	  )
{
    for(; count; count--)
	carry = ((*result++ = *op++ + carry) == 0) & carry;
    return carry;
}
static void
CarryResolve(
	     bigNum          result,
	     int             stop,
	     int             carry
	     )
{
    if(carry)
	{
	    pAssert((unsigned)stop < result->allocated);
	    result->d[stop++] = 1;
	}
    BnSetTop(result, stop);
}
/* 10.2.3.3.3 BnAdd() */
/* This function adds two bigNum values. Always returns TRUE */
LIB_EXPORT BOOL
BnAdd(
      bigNum           result,
      bigConst         op1,
      bigConst         op2
      )
{
    crypt_uword_t    stop;
    int              carry;
    const bignum_t   *n1 = op1;
    const bignum_t   *n2 = op2;
    //
    if(n2->size > n1->size)
	{
	    n1 = op2;
	    n2 = op1;
	}
    pAssert(result->allocated >= n1->size);
    stop = MIN(n1->size, n2->allocated);
    carry = (int)AddSame(result->d, n1->d, n2->d, (int)stop);
    if(n1->size > stop)
	carry = CarryProp(&result->d[stop], &n1->d[stop], (int)(n1->size - stop), carry);
    CarryResolve(result, (int)n1->size, carry);
    return TRUE;
}
/* 10.2.3.3.4 BnAddWord() */
/* Adds a word value to a bigNum. */
LIB_EXPORT BOOL
BnAddWord(
	  bigNum           result,
	  bigConst         op,
	  crypt_uword_t    word
	  )
{
    int              carry;
    //
    carry = (result->d[0] = op->d[0] + word) < word;
    carry = CarryProp(&result->d[1], &op->d[1], (int)(op->size - 1), carry);
    CarryResolve(result, (int)op->size, carry);
    return TRUE;
}
/* 10.2.3.3.5 SubSame() */
/* Subtract two values that have the same size. */
static int
SubSame(
	crypt_uword_t           *result,
	const crypt_uword_t     *op1,
	const crypt_uword_t     *op2,
	int                      count
	)
{
    int                  borrow = 0;
    int                  i;
    for(i = 0; i < count; i++)
	{
	    crypt_uword_t    a = op1[i];
	    crypt_uword_t    diff = a - op2[i];
	    result[i] = diff - borrow;
	    //       generate   |      propagate
	    borrow = (diff > a) | ((diff == 0) & borrow);
	}
    return borrow;
}
/* 10.2.3.3.6 BorrowProp() */
/* This propagates a borrow. If borrow is true when the end of the array is reached, then it means
   that op2 was larger than op1 and we don't handle that case so an assert is generated. This design
   choice was made because our only bigNum computations are on large positive numbers (primes) or on
   fields. Propagate a borrow. */
static int
BorrowProp(
	   crypt_uword_t           *result,
	   const crypt_uword_t     *op,
	   int                      size,
	   int                      borrow
	   )
{
    for(; size > 0; size--)
	borrow = ((*result++ = *op++ - borrow) == MAX_CRYPT_UWORD) && borrow;
    return borrow;
}
/* 10.2.3.3.7 BnSub() */
/* This function does subtraction of two bigNum values and returns result = op1 - op2 when op1 is
   greater than op2. If op2 is greater than op1, then a fault is generated. This function always
   returns TRUE. */

LIB_EXPORT BOOL
BnSub(
      bigNum           result,
      bigConst         op1,
      bigConst         op2
      )
{
    int             borrow;
    int             stop = (int)MIN(op1->size, op2->allocated);
    //
    // Make sure that op2 is not obviously larger than op1
    pAssert(op1->size >= op2->size);
    borrow = SubSame(result->d, op1->d, op2->d, stop);
    if(op1->size > (crypt_uword_t)stop)
	borrow = BorrowProp(&result->d[stop], &op1->d[stop], (int)(op1->size - stop),
			    borrow);
    pAssert(!borrow);
    BnSetTop(result, op1->size);
    return TRUE;
}
/* 10.2.3.3.8 BnSubWord() */
/* This function subtracts a word value from a bigNum. This function always returns TRUE. */
LIB_EXPORT BOOL
BnSubWord(
	  bigNum           result,
	  bigConst     op,
	  crypt_uword_t    word
	  )
{
    int             borrow;
    //
    pAssert(op->size > 1 || word <= op->d[0]);
    borrow = word > op->d[0];
    result->d[0] = op->d[0] - word;
    borrow = BorrowProp(&result->d[1], &op->d[1], (int)(op->size - 1), borrow);
    pAssert(!borrow);
    BnSetTop(result, op->size);
    return TRUE;
}
/* 10.2.3.3.9 BnUnsignedCmp() */
/* This function performs a comparison of op1 to op2. The compare is approximately constant time if
   the size of the values used in the compare is consistent across calls (from the same line in the
   calling code). */
/* Return Values Meaning */
/* < 0 op1 is less than op2 */
/* 0 op1 is equal to op2 */
/* > 0 op1 is greater than op2 */
LIB_EXPORT int
BnUnsignedCmp(
	      bigConst               op1,
	      bigConst               op2
	      )
{
    int             retVal;
    int             diff;
    int              i;
    //
    pAssert((op1 != NULL) && (op2 != NULL));
    retVal = (int)(op1->size - op2->size);
    if(retVal == 0)
	{
	    for(i = (int)(op1->size - 1); i >= 0; i--)
		{
		    diff = (op1->d[i] < op2->d[i]) ? -1 : (op1->d[i] != op2->d[i]);
		    retVal = retVal == 0 ? diff : retVal;
		}
	}
    else
	retVal = (retVal < 0) ? -1 : 1;
    return retVal;
}
/* 10.2.3.3.10 BnUnsignedCmpWord() */
/* Compare a bigNum to a crypt_uword_t. */
/* Return Value	Meaning */
/* -1	op1 is less that word */
/* 0	op1 is equal to word */
/* 1	op1 is greater than word */
LIB_EXPORT int
BnUnsignedCmpWord(
		  bigConst             op1,
		  crypt_uword_t        word
		  )
{
    if(op1->size > 1)
	return 1;
    else if(op1->size == 1)
	return (op1->d[0] < word) ? -1 : (op1->d[0] > word);
    else // op1 is zero
	// equal if word is zero
	return (word == 0) ? 0 : -1;
}
/* 10.2.3.3.11 BnModWord() */
/* This function does modular division of a big number when the modulus is a word value. */
LIB_EXPORT crypt_word_t
BnModWord(
	  bigConst         numerator,
	  crypt_word_t     modulus
	  )
{
    BN_MAX(remainder);
    BN_VAR(mod, RADIX_BITS);
    //
    mod->d[0] = modulus;
    mod->size = (modulus != 0);
    BnDiv(NULL, remainder, numerator, mod);
    return remainder->d[0];
}
/* 10.2.3.3.12 Msb() */
/* Returns the bit number of the most significant bit of a crypt_uword_t. The number for the least
   significant bit of any bigNum value is 0. The maximum return value is RADIX_BITS - 1, */
/* Return Values Meaning */
/* -1 the word was zero */
/* n the bit number of the most significant bit in the word */
LIB_EXPORT int
Msb(
    crypt_uword_t           word
    )
{
    int             retVal = -1;
    //
#if RADIX_BITS == 64
    if(word & 0xffffffff00000000) { retVal += 32; word >>= 32; }
#endif
    if(word & 0xffff0000) { retVal += 16; word >>= 16; }
    if(word & 0x0000ff00) { retVal += 8; word >>= 8; }
    if(word & 0x000000f0) { retVal += 4; word >>= 4; }
    if(word & 0x0000000c) { retVal += 2; word >>= 2; }
    if(word & 0x00000002) { retVal += 1; word >>= 1; }
    return retVal + (int)word;
}
/* 10.2.3.3.13 BnMsb() */
/* This function returns the number of the MSb() of a bigNum value. */
/*     Return Value	Meaning */
/*     -1	the word was zero or bn was NULL */
/*     n	the bit number of the most significant bit in the word */

LIB_EXPORT int
BnMsb(
      bigConst            bn
      )
{
    // If the value is NULL, or the size is zero then treat as zero and return -1
    if(bn != NULL && bn->size > 0)
	{
	    int         retVal = Msb(bn->d[bn->size - 1]);
	    retVal += (int)(bn->size - 1) * RADIX_BITS;
	    return retVal;
	}
    else
	return -1;
}
/* 10.2.3.3.14 BnSizeInBits() */
/* Returns the number of bits required to hold a number. It is one greater than the Msb. */
LIB_EXPORT unsigned
BnSizeInBits(
	     bigConst                 n
	     )
{
    int     bits = BnMsb(n) + 1;
    //
    return bits < 0 ? 0 : (unsigned)bits;
}
/* 10.2.3.3.15 BnSetWord() */
/* Change the value of a bignum_t to a word value. */
LIB_EXPORT bigNum
BnSetWord(
	  bigNum               n,
	  crypt_uword_t        w
	  )
{
    if(n != NULL)
	{
	    pAssert(n->allocated > 1);
	    n->d[0] = w;
	    BnSetTop(n, (w != 0) ? 1 : 0);
	}
    return n;
}
/* 10.2.3.3.16 BnSetBit() */
/* SET a bit in a bigNum. Bit 0 is the least-significant bit in the 0th digit_t. The function always
   return TRUE */
LIB_EXPORT BOOL
BnSetBit(
	 bigNum           bn,        // IN/OUT: big number to modify
	 unsigned int     bitNum     // IN: Bit number to SET
	 )
{
    crypt_uword_t            offset = bitNum / RADIX_BITS;
    pAssert(bn->allocated * RADIX_BITS >= bitNum);
    // Grow the number if necessary to set the bit.
    while(bn->size <= offset)
	bn->d[bn->size++] = 0;
    bn->d[offset] |= ((crypt_uword_t)1 << RADIX_MOD(bitNum));
    return TRUE;
}
/* 10.2.3.3.17 BnTestBit() */
/* Check to see if a bit is SET in a bignum_t. The 0th bit is the LSb() of d[0]. */
/* Return Values Meaning */
/* TRUE the bit is set */
/* FALSE the bit is not set or the number is out of range */
LIB_EXPORT BOOL
BnTestBit(
	  bigNum               bn,        // IN: number to check
	  unsigned int         bitNum     // IN: bit to test
	  )
{
    crypt_uword_t         offset = RADIX_DIV(bitNum);
    //
    if(bn->size > offset)
	return ((bn->d[offset] & (((crypt_uword_t)1) << RADIX_MOD(bitNum))) != 0);
    else
	return FALSE;
}
/* 10.2.3.3.18 BnMaskBits() */
/* Function to mask off high order bits of a big number. The returned value will have no more than
   maskBit bits set. */
/* NOTE: There is a requirement that unused words of a bignum_t are set to zero. */
/* Return Values Meaning */
/* TRUE result masked */
/* FALSE the input was not as large as the mask */
LIB_EXPORT BOOL
BnMaskBits(
	   bigNum           bn,        // IN/OUT: number to mask
	   crypt_uword_t    maskBit    // IN: the bit number for the mask.
	   )
{
    crypt_uword_t    finalSize;
    BOOL             retVal;
    finalSize = BITS_TO_CRYPT_WORDS(maskBit);
    retVal = (finalSize <= bn->allocated);
    if(retVal && (finalSize > 0))
	{
	    crypt_uword_t   mask;
	    mask = ~((crypt_uword_t)0) >> RADIX_MOD(maskBit);
	    bn->d[finalSize - 1] &= mask;
	}
    BnSetTop(bn, finalSize);
    return retVal;
}
/* 10.2.3.3.19 BnShiftRight() */
/* Function will shift a bigNum to the right by the shiftAmount. This function always returns
   TRUE. */
LIB_EXPORT BOOL
BnShiftRight(
	     bigNum           result,
	     bigConst         toShift,
	     uint32_t         shiftAmount
	     )
{
    uint32_t         offset = (shiftAmount >> RADIX_LOG2);
    uint32_t         i;
    uint32_t         shiftIn;
    crypt_uword_t    finalSize;
    //
    shiftAmount = shiftAmount & RADIX_MASK;
    shiftIn = RADIX_BITS - shiftAmount;
    // The end size is toShift->size - offset less one additional
    // word if the shiftAmount would make the upper word == 0
    if(toShift->size > offset)
	{
	    finalSize = toShift->size - offset;
	    finalSize -= (toShift->d[toShift->size - 1] >> shiftAmount) == 0 ? 1 : 0;
	}
    else
	finalSize = 0;
    pAssert(finalSize <= result->allocated);
    if(finalSize != 0)
	{
	    for(i = 0; i < finalSize; i++)
		{
		    result->d[i] = (toShift->d[i + offset] >> shiftAmount)
				   | (toShift->d[i + offset + 1] << shiftIn);
		}
	    if(offset == 0)
		result->d[i] = toShift->d[i] >> shiftAmount;
	}
    BnSetTop(result, finalSize);
    return TRUE;
}
/* 10.2.3.3.20	BnGetRandomBits() */
/* Return Value	Meaning */
/* TRUE(1)	success */
/* FALSE(0)	failure */
LIB_EXPORT BOOL
BnGetRandomBits(
		bigNum           n,
		size_t           bits,
		RAND_STATE      *rand
		)
{
    // Since this could be used for ECC key generation using the extra bits method,
    // make sure that the value is large enough
    TPM2B_TYPE(LARGEST, LARGEST_NUMBER + 8);
    TPM2B_LARGEST    large;
    //
    large.b.size = (UINT16)BITS_TO_BYTES(bits);
    if(DRBG_Generate(rand, large.t.buffer, large.t.size) == large.t.size)
	{
	    if(BnFrom2B(n, &large.b) != NULL)
		{
		    if(BnMaskBits(n, (crypt_uword_t)bits))
			return TRUE;
		}
	}
    return FALSE;
}
/* 10.2.3.3.21 BnGenerateRandomInRange() */
/* Function to generate a random number r in the range 1 <= r < limit. The function gets a random
   number of bits that is the size of limit. There is some some probability that the returned number
   is going to be greater than or equal to the limit. If it is, try again. There is no more than 50%
   chance that the next number is also greater, so try again. We keep trying until we get a value
   that meets the criteria. Since limit is very often a number with a LOT of high order ones, this
   rarely would need a second try. */
/* Return Value	Meaning */
/* TRUE(1)	success */
/* FALSE(0)	failure */
LIB_EXPORT BOOL
BnGenerateRandomInRange(
			bigNum           dest,
			bigConst         limit,
			RAND_STATE      *rand
			)
{
    size_t   bits = BnSizeInBits(limit);
    //
    if(bits < 2)
	{
	    BnSetWord(dest, 0);
	    return FALSE;
	}
    else
	{
	    while(BnGetRandomBits(dest, bits, rand)
		  && (BnEqualZero(dest) || (BnUnsignedCmp(dest, limit) >= 0)));
	}
    return !g_inFailureMode;
}

// libtpms added begin

// This version of BnSizeInBits skips any leading zero bytes in bigConst
// and thus calculates the bits that OpenSSL will work with after truncating
// the leading zeros
static /*VBOX: LIB_EXPORT*/ unsigned
BnSizeInBitsSkipLeadingZeros(
	     bigConst                 n
	     )
{
    int                firstByte;
    unsigned           bitSize = BnSizeInBits(n);
    crypt_uword_t      i;

    if (bitSize <= 8)
	return bitSize;

    // search for the first limb that is non-zero
    for (i = 0; i < n->size; i++) {
        if (n->d[i] != 0)
            break;
    }
    if (i >= n->size)
        return 0; // should never happen

    // get the first byte in this limb that is non-zero
    firstByte = (RADIX_BITS - 1 - Msb(n->d[i])) >> 3;

    return bitSize - i * sizeof(n->d[0]) - (firstByte << 3);
}


/* This is a version of BnGenerateRandomInRange that ensures that the upper most
   byte is non-zero, so that the number will not be shortened and subsequent operations
   will not have a timing-sidechannel
 */
LIB_EXPORT BOOL
BnGenerateRandomInRangeAllBytes(
			bigNum           dest,
			bigConst         limit,
			RAND_STATE      *rand
			)
{
    BOOL     OK;
    int      repeats = 0;
    int      maxRepeats;
    unsigned requestedBits;
    unsigned requestedBytes;
    unsigned numBytes;

    if (rand)
	return BnGenerateRandomInRange(dest, limit, rand);

    // a 'limit' like 'BN_P638_n' has leading zeros and we only need 73 bytes not 80
    requestedBits = BnSizeInBitsSkipLeadingZeros(limit);
    requestedBytes = BITS_TO_BYTES(requestedBits);
    maxRepeats = 8;
    if (requestedBits & 7)
	maxRepeats += (9 - (requestedBits & 7));

    while (true) {
	OK = BnGenerateRandomInRange(dest, limit, rand);
	if (!OK)
	    break;
	if (repeats < maxRepeats) {
	    numBytes = BITS_TO_BYTES(BnSizeInBitsSkipLeadingZeros(dest));
	    if (numBytes < requestedBytes) {
		repeats++;
		continue;
	    }
	}
	break;
    }

    return OK;
}
// libtpms added end
