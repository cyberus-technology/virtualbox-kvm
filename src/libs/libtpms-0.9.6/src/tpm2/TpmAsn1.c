/********************************************************************************/
/*										*/
/*			     TPM ASN.1						*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: TpmAsn1.c $		*/
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
/*  (c) Copyright IBM Corp. and others, 2019					*/
/*										*/
/********************************************************************************/

/* 10.2.23	TpmAsn1.c */
/* 10.2.23.1	Includes */
#include "Tpm.h"
#define _OIDS_
#include "OIDs.h"
#include "TpmAsn1.h"
#include "TpmAsn1_fp.h"
/* 10.2.23.2	Unmarshaling Functions */
/* 10.2.23.2.1	ASN1UnmarshalContextInitialize() */
/* Function does standard initialization of a context. */
/* Return Value	Meaning */
/* TRUE(1)	success */
/* FALSE(0)	failure */
BOOL
ASN1UnmarshalContextInitialize(
			       ASN1UnmarshalContext    *ctx,
			       INT16                    size,
			       BYTE                    *buffer
			       )
{
    VERIFY(buffer != NULL);
    VERIFY(size > 0);
    ctx->buffer = buffer;
    ctx->size = size;
    ctx->offset = 0;
    ctx->tag = 0xFF;
    return TRUE;
 Error:
    return FALSE;
}
/* 10.2.23.2.2	ASN1DecodeLength() */
/* This function extracts the length of an element from buffer starting at offset. */
/* Return Value	Meaning */
/* >=0	the extracted length */
/* <0	an error */
INT16
ASN1DecodeLength(
		 ASN1UnmarshalContext        *ctx
		 )
{
    BYTE                first;                  // Next octet in buffer
    INT16               value;
    //
    VERIFY(ctx->offset < ctx->size);
    first = NEXT_OCTET(ctx);
    // If the number of octets of the entity is larger than 127, then the first octet
    // is the number of octets in the length specifier.
    if(first >= 0x80)
	{
	    // Make sure that this length field is contained with the structure being
	    // parsed
	    CHECK_SIZE(ctx, (first & 0x7F));
	    if(first == 0x82)
		{
		    // Two octets of size
		    // get the next value
		    value = (INT16)NEXT_OCTET(ctx);
		    // Make sure that the result will fit in an INT16
		    VERIFY(value < 0x0080);
		    // Shift up and add next octet
		    value = (value << 8) + NEXT_OCTET(ctx);
		}
	    else if(first == 0x81)
		value = NEXT_OCTET(ctx);
	    // Sizes larger than will fit in a INT16 are an error
	    else
		goto Error;
	}
    else
	value = first;
    // Make sure that the size defined something within the current context
    CHECK_SIZE(ctx, value);
    return value;
 Error:
    ctx->size = -1;             // Makes everything fail from now on.
    return -1;
}
/* 10.2.23.2.3	ASN1NextTag() */
/* This function extracts the next type from buffer starting at offset. It advances offset as it
   parses the type and the length of the type. It returns the length of the type. On return, the
   length octets starting at offset are the octets of the type. */
/*     Return Value	Meaning */
/*     >=0	the number of octets in type */
/*     <0	an error */
INT16
ASN1NextTag(
	    ASN1UnmarshalContext    *ctx
	    )
{
    // A tag to get?
    VERIFY(ctx->offset < ctx->size);
    // Get it
    ctx->tag = NEXT_OCTET(ctx);
    // Make sure that it is not an extended tag
    VERIFY((ctx->tag & 0x1F) != 0x1F);
    // Get the length field and return that
    return ASN1DecodeLength(ctx);
    
 Error:
    // Attempt to read beyond the end of the context or an illegal tag
    ctx->size = -1;         // Persistent failure
    ctx->tag = 0xFF;
    return -1;
}
/* 10.2.23.2.4	ASN1GetBitStringValue() */
/* Try to parse a bit string of up to 32 bits from a value that is expected to be a bit string. The
   bit string is left justified so that the MSb of the input is the MSb of the returned value. If
   there is a general parsing error, the context->size is set to -1. */
/*     Return Value	Meaning */
/*     TRUE(1)	success */
/*     FALSE(0)	failure */

BOOL
ASN1GetBitStringValue(
		      ASN1UnmarshalContext        *ctx,
		      UINT32                      *val
		      )
{
    int                  shift;
    INT16                length;
    UINT32               value = 0;
    int                  inputBits;
    //
    length = ASN1NextTag(ctx);
    VERIFY(length >= 1);
    VERIFY(ctx->tag == ASN1_BITSTRING);
    // Get the shift value for the bit field (how many bits to lop off of the end)
    shift = NEXT_OCTET(ctx);
    length--;
    // Get the number of bits in the input
    inputBits = (8 * length) - shift;
    // the shift count has to make sense
    VERIFY((shift < 8) && ((length > 0) || (shift == 0)));
    // if there are any bytes left
    for(; length > 1; length--)
	{
	    
	    // for all but the last octet, just shift and add the new octet
	    VERIFY((value & 0xFF000000) == 0); // can't loose significant bits
	    value = (value << 8) + NEXT_OCTET(ctx);
	    
	}
    if(length == 1)
	{
	    // for the last octet, just shift the accumulated value enough to
	    // accept the significant bits in the last octet and shift the last
	    // octet down
	    VERIFY(((value & (0xFF000000 << (8 - shift)))) == 0);
	    value = (value << (8 - shift)) + (NEXT_OCTET(ctx) >> shift);
	    
	}
    // 'Left justify' the result
    if(inputBits > 0)
	value <<= (32 - inputBits);
    *val = value;
    return TRUE;
 Error:
    ctx->size = -1;
    return FALSE;
}

/* 10.2.23.3	Marshaling Functions */
/* 10.2.23.3.1	Introduction */
/* Marshaling of an ASN.1 structure is accomplished from the bottom up. That is, the things that
   will be at the end of the structure are added last. To manage the collecting of the relative
   sizes, start a context for the outermost container, if there is one, and then placing items in
   from the bottom up. If the bottom-most item is also within a structure, create a nested context
   by calling ASN1StartMarshalingContext(). */
/* The context control structure contains a buffer pointer, an offset, an end and a stack. offset is
   the offset from the start of the buffer of the last added byte. When offset reaches 0, the buffer
   is full. offset is a signed value so that, when it becomes negative, there is an overflow. Only
   two functions are allowed to move bytes into the buffer: ASN1PushByte() and
   ASN1PushBytes(). These functions make sure that no data is written beyond the end of the
   buffer. */
/* When a new context is started, the current value of end is pushed on the stack and end is set to
   'offset. As bytes are added, offset gets smaller. At any time, the count of bytes in the current
   context is simply end - offset. */
/* Since starting a new context involves setting end = offset, the number of bytes in the context
   starts at 0. The nominal way of ending a context is to use end - offset to set the length value,
   and then a tag is added to the buffer. Then the previous end value is popped meaning that the
   context just ended becomes a member of the now current context. */
/* The nominal strategy for building a completed ASN.1 structure is to push everything into the
   buffer and then move everything to the start of the buffer. The move is simple as the size of the
   move is the initial end value minus the final offset value. The destination is buffer and the
   source is buffer + offset. As Skippy would say "Easy peasy, Joe." */
/* It is not necessary to provide a buffer into which the data is placed. If no buffer is provided,
   then the marshaling process will return values needed for marshaling. On strategy for filling the
   buffer would be to execute the process for building the structure without using a buffer. This
   would return the overall size of the structure. Then that amount of data could be allocated for
   the buffer and the fill process executed again with the data going into the buffer. At the end,
   the data would be in its final resting place. */
/* 10.2.23.3.2	ASN1InitialializeMarshalContext() */
/* This creates a structure for handling marshaling of an ASN.1 formatted data structure. */
void
ASN1InitialializeMarshalContext(
				ASN1MarshalContext      *ctx,
				INT16                    length,
				BYTE                    *buffer
				)
{
    ctx->buffer = buffer;
    if(buffer)
	ctx->offset = length;
    else
	ctx->offset = INT16_MAX;
    ctx->end = ctx->offset;
    ctx->depth = -1;
}
/* 10.2.23.3.3	ASN1StartMarshalContext() */
/* This starts a new constructed element. It is constructed on top of the value that was previously placed in the structure. */
void
ASN1StartMarshalContext(
			ASN1MarshalContext      *ctx
			)
{
    pAssert((ctx->depth + 1) < MAX_DEPTH);
    ctx->depth++;
    ctx->ends[ctx->depth] = ctx->end;
    ctx->end = ctx->offset;
}
/* 10.2.23.3.4	ASN1EndMarshalContext() */
/* This function restores the end pointer for an encapsulating structure. */
/* Return Value	Meaning */
/* > 0	the size of the encapsulated structure that was just ended */
/* <= 0	an error */
INT16
ASN1EndMarshalContext(
		      ASN1MarshalContext      *ctx
		      )
{
    INT16                   length;
    pAssert(ctx->depth >= 0);
    length = ctx->end - ctx->offset;
    ctx->end = ctx->ends[ctx->depth--];
    if((ctx->depth == -1) && (ctx->buffer))
	{
	    MemoryCopy(ctx->buffer, ctx->buffer + ctx->offset, ctx->end - ctx->offset);
	}
    return length;
}
/* 10.2.23.3.5 ASN1EndEncapsulation() */
/* This function puts a tag and length in the buffer. In this function, an embedded BIT_STRING is
   assumed to be a collection of octets. To indicate that all bits are used, a byte of zero is
   prepended. If a raw bit-string is needed, a new function like ASN1PushInteger() would be
   needed. */
/*     Return Value	Meaning */
/*     > 0	number of octets in the encapsulation */
/*     == 0	failure */
UINT16
ASN1EndEncapsulation(
		     ASN1MarshalContext          *ctx,
		     BYTE                         tag
		     )
{
    // only add a leading zero for an encapsulated BIT STRING
    if (tag == ASN1_BITSTRING)
	ASN1PushByte(ctx, 0);
    ASN1PushTagAndLength(ctx, tag, ctx->end - ctx->offset);
    return ASN1EndMarshalContext(ctx);
}
/* 10.2.23.3.6	ASN1PushByte() */
BOOL
ASN1PushByte(
	     ASN1MarshalContext          *ctx,
	     BYTE                         b
	     )
{
    if(ctx->offset > 0)
	{
	    ctx->offset -= 1;
	    if(ctx->buffer)
		ctx->buffer[ctx->offset] = b;
	    return TRUE;
	}
    ctx->offset = -1;
    return FALSE;
}
/* 10.2.23.3.7	ASN1PushBytes() */
/* Push some raw bytes onto the buffer. count cannot be zero. */
/* Return Value	Meaning */
/* > 0	count bytes */
/* == 0	failure unless count was zero */
INT16
ASN1PushBytes(
	      ASN1MarshalContext          *ctx,
	      INT16                        count,
	      const BYTE                  *buffer
	      )
{
    // make sure that count is not negative which would mess up the math; and that
    // if there is a count, there is a buffer
    VERIFY((count >= 0) && ((buffer != NULL) || (count == 0)));
    // back up the offset to determine where the new octets will get pushed
    ctx->offset -= count;
    // can't go negative
    VERIFY(ctx->offset >= 0);
    // if there are buffers, move the data, otherwise, assume that this is just a
    // test.
    if(count && buffer && ctx->buffer)
	MemoryCopy(&ctx->buffer[ctx->offset], buffer, count);
    return count;
 Error:
    ctx->offset = -1;
    return 0;
}
/* 10.2.23.3.8	ASN1PushNull() */
/* Return Value	Meaning */
/* > 0	count bytes */
/* == 0	failure unless count was zero */
INT16
ASN1PushNull(
	     ASN1MarshalContext      *ctx
	     )
{
    ASN1PushByte(ctx, 0);
    ASN1PushByte(ctx, ASN1_NULL);
    return (ctx->offset >= 0) ? 2 : 0;
}
/* 10.2.23.3.9	ASN1PushLength() */
/* Push a length value. This will only handle length values that fit in an INT16. */
/* Return Value	Meaning */
/* > 0	number of bytes added */
/* == 0	failure */
INT16
ASN1PushLength(
	       ASN1MarshalContext          *ctx,
	       INT16                        len
	       )
{
    UINT16                       start = ctx->offset;
    VERIFY(len >= 0);
    if(len <= 127)
	ASN1PushByte(ctx, (BYTE)len);
    else
	{
	    ASN1PushByte(ctx, (BYTE)(len & 0xFF));
	    len >>= 8;
	    if(len == 0)
		ASN1PushByte(ctx, 0x81);
	    else
		{
		    ASN1PushByte(ctx, (BYTE)(len));
		    ASN1PushByte(ctx, 0x82);
		}
	}
    goto Exit;
 Error:
    ctx->offset = -1;
 Exit:
    return (ctx->offset > 0) ? start - ctx->offset : 0;
}
/* 10.2.23.3.10	ASN1PushTagAndLength() */
/* Return Value	Meaning */
/* > 0	number of bytes added */
/* == 0	failure */
INT16
ASN1PushTagAndLength(
		     ASN1MarshalContext          *ctx,
		     BYTE                         tag,
		     INT16                        length
		     )
{
    INT16       bytes;
    bytes = ASN1PushLength(ctx, length);
    bytes += (INT16)ASN1PushByte(ctx, tag);
    return (ctx->offset < 0) ? 0 : bytes;
}
/* 10.2.23.3.11	ASN1PushTaggedOctetString() */
/* This function will push a random octet string. */
/* Return Value	Meaning */
/* > 0	number of bytes added */
/* == 0	failure */
INT16
ASN1PushTaggedOctetString(
			  ASN1MarshalContext          *ctx,
			  INT16                        size,
			  const BYTE                  *string,
			  BYTE                         tag
			  )
{
    ASN1PushBytes(ctx, size, string);
    // PushTagAndLenght just tells how many octets it added so the total size of this
    // element is the sum of those octets and input size.
    size += ASN1PushTagAndLength(ctx, tag, size);
    return size;
}
/* 10.2.23.3.12	ASN1PushUINT() */
/* This function pushes an native-endian integer value. This just changes a native-endian integer
   into a big-endian byte string and calls ASN1PushInteger(). That function will remove leading
   zeros and make sure that the number is positive. */
/* Return Value	Meaning */
/* > 0	count bytes */
/* == 0	failure unless count was zero */
INT16
ASN1PushUINT(
	     ASN1MarshalContext      *ctx,
	     UINT32                   integer
	     )
{
    BYTE                    marshaled[4];
    UINT32_TO_BYTE_ARRAY(integer, marshaled);
    return ASN1PushInteger(ctx, 4, marshaled);
}
/* 10.2.23.3.13	ASN1PushInteger */
/* Push a big-endian integer on the end of the buffer */
/* Return Value	Meaning */
/* > 0	the number of bytes marshaled for the integer */
/* == 0	failure */
INT16
ASN1PushInteger(
		ASN1MarshalContext  *ctx,           // IN/OUT: buffer context
		INT16                iLen,          // IN: octets of the integer
		BYTE                *integer        // IN: big-endian integer
		)
{
    // no leading 0's
    while((*integer == 0) && (--iLen > 0))
	integer++;
    // Move the bytes to the buffer
    ASN1PushBytes(ctx, iLen, integer);
    // if needed, add a leading byte of 0 to make the number positive
    if(*integer & 0x80)
	iLen += (INT16)ASN1PushByte(ctx, 0);
    // PushTagAndLenght just tells how many octets it added so the total size of this
    // element is the sum of those octets and the adjusted input size.
    iLen +=  ASN1PushTagAndLength(ctx, ASN1_INTEGER, iLen);
    return iLen;
}
/* 10.2.23.3.14	ASN1PushOID() */
/* This function is used to add an OID. An OID is 0x06 followed by a byte of size followed by size
   bytes. This is used to avoid having to do anything special in the definition of an OID. */
/* Return Value	Meaning */
/* > 0	the number of bytes marshaled for the integer */
/* == 0	failure */
INT16
ASN1PushOID(
	    ASN1MarshalContext          *ctx,
	    const BYTE                  *OID
	    )
{
    if((*OID == ASN1_OBJECT_IDENTIFIER) && ((OID[1] & 0x80) == 0))
	{
	    return ASN1PushBytes(ctx, OID[1] + 2, OID);
	}
    ctx->offset = -1;
    return 0;
}
