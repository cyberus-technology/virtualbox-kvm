/********************************************************************************/
/*										*/
/*		 Miscellaneous Memory Manipulation Routines    			*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: Memory.c $		*/
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

/* 9.12 Memory.c */
/* 9.12.1 Description */
/* This file contains a set of miscellaneous memory manipulation routines. Many of the functions
   have the same semantics as functions defined in string.h. Those functions are not used directly
   in the TPM because they are not safe */
/* This version uses string.h after adding guards.  This is because the math libraries invariably
   use those functions so it is not practical to prevent those library functions from being pulled
   into the build. */
/* 9.12.2 Includes and Data Definitions */
#include "Tpm.h"
#include "Memory_fp.h"
/* 9.12.3 Functions */
/* 9.12.3.1 MemoryCopy() */
/* This is an alias for memmove. This is used in place of memcpy because some of the moves may
   overlap and rather than try to make sure that memmove is used when necessary, it is always
   used. */

void
MemoryCopy(
	   void        *dest,
	   const void  *src,
	   int          sSize
	   )
{
    if (dest != src)
	memmove(dest, src, sSize);
}

/* 9.12.3.2 MemoryEqual() */
/* This function indicates if two buffers have the same values in the indicated number of bytes. */
/* Return Values Meaning */
/* TRUE all octets are the same */
/* FALSE all octets are not the same */
BOOL
MemoryEqual(
	    const void      *buffer1,       // IN: compare buffer1
	    const void      *buffer2,       // IN: compare buffer2
	    unsigned int     size           // IN: size of bytes being compared
	    )
{
    BYTE         equal = 0;
    const BYTE  *b1 = (BYTE *)buffer1;
    const BYTE  *b2 = (BYTE *)buffer2;
    //
    // Compare all bytes so that there is no leakage of information
    // due to timing differences.
    for(; size > 0; size--)
	equal |= (*b1++ ^ *b2++);
    return (equal == 0);
}
/* 9.12.3.3 MemoryCopy2B() */
/* This function copies a TPM2B. This can be used when the TPM2B types are the same or different. */
/* This function returns the number of octets in the data buffer of the TPM2B. */
LIB_EXPORT INT16
MemoryCopy2B(
	     TPM2B           *dest,          // OUT: receiving TPM2B
	     const TPM2B     *source,        // IN: source TPM2B
	     unsigned int     dSize          // IN: size of the receiving buffer
	     )
{
    pAssert(dest != NULL);
    if(source == NULL)
	dest->size = 0;
    else
	{
	    pAssert(source->size <= dSize);
	    MemoryCopy(dest->buffer, source->buffer, source->size);
	    dest->size = source->size;
	}
    return dest->size;
}
/* 9.12.3.4 MemoryConcat2B() */
/* This function will concatenate the buffer contents of a TPM2B to the buffer contents of
   another TPM2B and adjust the size accordingly (a := (a | b)). */
void
MemoryConcat2B(
	       TPM2B           *aInOut,        // IN/OUT: destination 2B
	       TPM2B           *bIn,           // IN: second 2B
	       unsigned int     aMaxSize       // IN: The size of aInOut.buffer (max values for
	       //     aInOut.size)
	       )
{
    pAssert(bIn->size <= aMaxSize - aInOut->size);
    MemoryCopy(&aInOut->buffer[aInOut->size], &bIn->buffer, bIn->size);
    aInOut->size = aInOut->size + bIn->size;
    return;
}
/* 9.12.3.5 MemoryEqual2B() */
/* This function will compare two TPM2B structures. To be equal, they need to be the same size and
   the buffer contexts need to be the same in all octets. */
/* Return Values Meaning */
/* TRUE size and buffer contents are the same */
/* FALSE size or buffer contents are not the same */
BOOL
MemoryEqual2B(
	      const TPM2B     *aIn,           // IN: compare value
	      const TPM2B     *bIn            // IN: compare value
	      )
{
    if(aIn->size != bIn->size)
	return FALSE;
    return MemoryEqual(aIn->buffer, bIn->buffer, aIn->size);
}
/* 9.12.3.6 MemorySet() */
/* This function will set all the octets in the specified memory range to the specified octet
   value. */
/* NOTE: A previous version had an additional parameter (dSize) that was intended to make sure that
   the destination would not be overrun. The problem is that, in use, all that was happening was
   that the value of size was used for dSize so there was no benefit in the extra parameter. */

void
MemorySet(
	  void            *dest,
	  int              value,
	  size_t           size
	  )
{
    memset(dest, value, size);
}

/* 9.12.3.7 MemoryPad2B() */
/* Function to pad a TPM2B with zeros and adjust the size. */

void
MemoryPad2B(
	    TPM2B           *b,
	    UINT16           newSize
	    )
{
    MemorySet(&b->buffer[b->size], 0, newSize - b->size);
    b->size = newSize;
}

/* 9.12.3.8 Uint16ToByteArray() */
/* Function to write an integer to a byte array */

void
Uint16ToByteArray(
		  UINT16              i,
		  BYTE                *a
		  )
{
    a[1] = (BYTE)(i); i >>= 8;
    a[0] = (BYTE)(i);
}

/* 9.12.3.9 Uint32ToByteArray() */
/* Function to write an integer to a byte array */

void
Uint32ToByteArray(
		  UINT32              i,
		  BYTE                *a
		  )
{
    a[3] = (BYTE)(i); i >>= 8;
    a[2] = (BYTE)(i); i >>= 8;
    a[1] = (BYTE)(i); i >>= 8;
    a[0] = (BYTE)(i);
}

/* 9.12.3.10 Uint64ToByteArray() */
/* Function to write an integer to a byte array */

void
Uint64ToByteArray(
		  UINT64               i,
		  BYTE                *a
		  )
{
    a[7] = (BYTE)(i); i >>= 8;
    a[6] = (BYTE)(i); i >>= 8;
    a[5] = (BYTE)(i); i >>= 8;
    a[4] = (BYTE)(i); i >>= 8;
    a[3] = (BYTE)(i); i >>= 8;
    a[2] = (BYTE)(i); i >>= 8;
    a[1] = (BYTE)(i); i >>= 8;
    a[0] = (BYTE)(i);
}

/* 9.12.3.11	ByteArrayToUint8() */
/* Function to write a UINT8 to a byte array. This is included for completeness and to allow certain
   macro expansions */
#if 0                 // libtpms added
UINT8
ByteArrayToUint8(
		 BYTE                *a
		 )
{
    return          *a;
}
#endif                // libtpms added

/* 9.12.3.12 ByteArrayToUint16() */
/* Function to write an integer to a byte array */

UINT16
ByteArrayToUint16(
		  BYTE                *a
		  )
{
    return ((UINT16)a[0] << 8) + a[1];
}

/* 9.12.3.13 ByteArrayToUint32() */
/* Function to write an integer to a byte array */

UINT32
ByteArrayToUint32(
		  BYTE                *a
		  )
{
    return (UINT32)((((((UINT32)a[0] << 8) + a[1]) << 8) + (UINT32)a[2]) << 8) + a[3];
}

/* 9.12.3.14 ByteArrayToUint64() */
/* Function to write an integer to a byte array */

UINT64
ByteArrayToUint64(
		  BYTE                *a
		  )
{
    return (((UINT64)BYTE_ARRAY_TO_UINT32(a)) << 32) + BYTE_ARRAY_TO_UINT32(&a[4]);
}

