/** @file
 * IPRT - RTUINT32U methods for old 16-bit compilers (mainly for division).
 */

/*
 * Copyright (C) 2011-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef IPRT_INCLUDED_uint32_h
#define IPRT_INCLUDED_uint32_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/asm.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_uint32 RTUInt32 - 32-bit Unsigned Integer Methods for 16-bit compilers.
 * @ingroup grp_rt
 * @{
 */

#define RTUINT32_HAVE_32BIT_BASICS


/**
 * Test if a 32-bit unsigned integer value is zero.
 *
 * @returns true if they are, false if they aren't.
 * @param   pValue          The input and output value.
 */
DECLINLINE(bool) RTUInt32IsZero(PRTUINT32U pValue)
{
    return pValue->s.Lo == 0
        && pValue->s.Hi == 0;
}


/**
 * Set a 32-bit unsigned integer value to zero.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 */
DECLINLINE(PRTUINT32U) RTUInt32SetZero(PRTUINT32U pResult)
{
    pResult->s.Hi = 0;
    pResult->s.Lo = 0;
    return pResult;
}


/**
 * Set a 32-bit unsigned integer value to the maximum value.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 */
DECLINLINE(PRTUINT32U) RTUInt32SetMax(PRTUINT32U pResult)
{
    pResult->s.Hi = UINT16_MAX;
    pResult->s.Lo = UINT16_MAX;
    return pResult;
}




/**
 * Adds two 32-bit unsigned integer values.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(PRTUINT32U) RTUInt32Add(PRTUINT32U pResult, PCRTUINT32U pValue1, PCRTUINT32U pValue2)
{
#ifdef RTUINT32_HAVE_32BIT_BASICS
    pResult->u = pValue1->u + pValue2->u;
#else
    pResult->s.Hi = pValue1->s.Hi + pValue2->s.Hi;
    pResult->s.Lo = pValue1->s.Lo + pValue2->s.Lo;
    if (pResult->s.Lo < pValue1->s.Lo)
        pResult->s.Hi++;
#endif
    return pResult;
}


/**
 * Adds a 32-bit and a 16-bit unsigned integer values.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   uValue2             The second value, 16-bit.
 */
DECLINLINE(PRTUINT32U) RTUInt32AddU16(PRTUINT32U pResult, PCRTUINT32U pValue1, uint16_t uValue2)
{
#ifdef RTUINT32_HAVE_32BIT_BASICS
    pResult->u = pValue1->u + uValue2;
#else
    pResult->s.Hi = pValue1->s.Hi;
    pResult->s.Lo = pValue1->s.Lo + uValue2;
    if (pResult->s.Lo < pValue1->s.Lo)
        pResult->s.Hi++;
#endif
    return pResult;
}


/**
 * Subtracts a 32-bit unsigned integer value from another.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The minuend value.
 * @param   pValue2             The subtrahend value.
 */
DECLINLINE(PRTUINT32U) RTUInt32Sub(PRTUINT32U pResult, PCRTUINT32U pValue1, PCRTUINT32U pValue2)
{
#ifdef RTUINT32_HAVE_32BIT_BASICS
    pResult->u = pValue1->u - pValue2->u;
#else
    pResult->s.Lo = pValue1->s.Lo - pValue2->s.Lo;
    pResult->s.Hi = pValue1->s.Hi - pValue2->s.Hi;
    if (pResult->s.Lo > pValue1->s.Lo)
        pResult->s.Hi--;
#endif
    return pResult;
}


/**
 * Multiplies two 32-bit unsigned integer values.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(PRTUINT32U) RTUInt32Mul(PRTUINT32U pResult, PCRTUINT32U pValue1, PCRTUINT32U pValue2)
{
    pResult->u     = (uint32_t)pValue1->s.Lo * pValue2->s.Lo;
    pResult->s.Hi += pValue1->s.Hi * pValue2->s.Lo;
    pResult->s.Hi += pValue1->s.Lo * pValue2->s.Hi;

    return pResult;
}


/**
 * Multiplies an 32-bit unsigned integer by a 16-bit unsigned integer value.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   uValue2             The second value, 16-bit.
 */
DECLINLINE(PRTUINT32U) RTUInt32MulByU16(PRTUINT32U pResult, PCRTUINT32U pValue1, uint16_t uValue2)
{
    pResult->u     = (uint32_t)pValue1->s.Lo * uValue2;
    pResult->s.Hi += pValue1->s.Hi * uValue2;
    return pResult;
}


DECLINLINE(PRTUINT32U) RTUInt32DivRem(PRTUINT32U pQuotient, PRTUINT32U pRemainder, PCRTUINT32U pValue1, PCRTUINT32U pValue2);

/**
 * Divides a 32-bit unsigned integer value by another.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The dividend value.
 * @param   pValue2             The divisor value.
 */
DECLINLINE(PRTUINT32U) RTUInt32Div(PRTUINT32U pResult, PCRTUINT32U pValue1, PCRTUINT32U pValue2)
{
    RTUINT32U Ignored;
    return RTUInt32DivRem(pResult, &Ignored, pValue1, pValue2);
}


/**
 * Divides a 32-bit unsigned integer value by another, returning the remainder.
 *
 * @returns pResult
 * @param   pResult             The result variable (remainder).
 * @param   pValue1             The dividend value.
 * @param   pValue2             The divisor value.
 */
DECLINLINE(PRTUINT32U) RTUInt32Mod(PRTUINT32U pResult, PCRTUINT32U pValue1, PCRTUINT32U pValue2)
{
    RTUINT32U Ignored;
    RTUInt32DivRem(&Ignored, pResult, pValue1, pValue2);
    return pResult;
}


/**
 * Bitwise AND of two 32-bit unsigned integer values.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(PRTUINT32U) RTUInt32And(PRTUINT32U pResult, PCRTUINT32U pValue1, PCRTUINT32U pValue2)
{
    pResult->s.Hi = pValue1->s.Hi & pValue2->s.Hi;
    pResult->s.Lo = pValue1->s.Lo & pValue2->s.Lo;
    return pResult;
}


/**
 * Bitwise OR of two 32-bit unsigned integer values.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(PRTUINT32U) RTUInt32Or( PRTUINT32U pResult, PCRTUINT32U pValue1, PCRTUINT32U pValue2)
{
    pResult->s.Hi = pValue1->s.Hi | pValue2->s.Hi;
    pResult->s.Lo = pValue1->s.Lo | pValue2->s.Lo;
    return pResult;
}


/**
 * Bitwise XOR of two 32-bit unsigned integer values.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(PRTUINT32U) RTUInt32Xor(PRTUINT32U pResult, PCRTUINT32U pValue1, PCRTUINT32U pValue2)
{
    pResult->s.Hi = pValue1->s.Hi ^ pValue2->s.Hi;
    pResult->s.Lo = pValue1->s.Lo ^ pValue2->s.Lo;
    return pResult;
}


/**
 * Shifts a 32-bit unsigned integer value @a cBits to the left.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue              The value to shift.
 * @param   cBits               The number of bits to shift it.
 */
DECLINLINE(PRTUINT32U) RTUInt32ShiftLeft(PRTUINT32U pResult, PCRTUINT32U pValue, int cBits)
{
    cBits &= 31;
#ifdef RTUINT32_HAVE_32BIT_BASICS
    pResult->u = pValue->u << cBits;
#else
    if (cBits < 16)
    {
        pResult->s.Lo = pValue->s.Lo << cBits;
        pResult->s.Hi = (pValue->s.Hi << cBits) | (pValue->s.Lo >> (16 - cBits));
    }
    else
    {
        pResult->s.Lo = 0;
        pResult->s.Hi = pValue->s.Lo << (cBits - 16);
    }
#endif
    return pResult;
}


/**
 * Shifts a 32-bit unsigned integer value @a cBits to the right.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue              The value to shift.
 * @param   cBits               The number of bits to shift it.
 */
DECLINLINE(PRTUINT32U) RTUInt32ShiftRight(PRTUINT32U pResult, PCRTUINT32U pValue, int cBits)
{
    cBits &= 31;
#ifdef RTUINT32_HAVE_32BIT_BASICS
    pResult->u = pValue->u >> cBits;
#else
    if (cBits < 16)
    {
        pResult->s.Hi = pValue->s.Hi >> cBits;
        pResult->s.Lo = (pValue->s.Lo >> cBits) | (pValue->s.Hi << (16 - cBits));
    }
    else
    {
        pResult->s.Hi = 0;
        pResult->s.Lo = pValue->s.Hi >> (cBits - 16);
    }
#endif
    return pResult;
}


/**
 * Boolean not (result 0 or 1).
 *
 * @returns pResult.
 * @param   pResult             The result variable.
 * @param   pValue              The value.
 */
DECLINLINE(PRTUINT32U) RTUInt32BooleanNot(PRTUINT32U pResult, PCRTUINT32U pValue)
{
    pResult->s.Lo = pValue->s.Lo || pValue->s.Hi ? 0 : 1;
    pResult->s.Hi = 0;
    return pResult;
}


/**
 * Bitwise not (flips each bit of the 32 bits).
 *
 * @returns pResult.
 * @param   pResult             The result variable.
 * @param   pValue              The value.
 */
DECLINLINE(PRTUINT32U) RTUInt32BitwiseNot(PRTUINT32U pResult, PCRTUINT32U pValue)
{
    pResult->s.Hi = ~pValue->s.Hi;
    pResult->s.Lo = ~pValue->s.Lo;
    return pResult;
}


/**
 * Assigns one 32-bit unsigned integer value to another.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue              The value to assign.
 */
DECLINLINE(PRTUINT32U) RTUInt32Assign(PRTUINT32U pResult, PCRTUINT32U pValue)
{
    pResult->s.Hi = pValue->s.Hi;
    pResult->s.Lo = pValue->s.Lo;
    return pResult;
}


/**
 * Assigns a boolean value to 32-bit unsigned integer.
 *
 * @returns pValueResult
 * @param   pValueResult        The result variable.
 * @param   fValue              The boolean value.
 */
DECLINLINE(PRTUINT32U) RTUInt32AssignBoolean(PRTUINT32U pValueResult, bool fValue)
{
    pValueResult->s.Lo = fValue;
    pValueResult->s.Hi = 0;
    return pValueResult;
}


/**
 * Assigns a 8-bit unsigned integer value to 32-bit unsigned integer.
 *
 * @returns pValueResult
 * @param   pValueResult        The result variable.
 * @param   u8Value             The 8-bit unsigned integer value.
 */
DECLINLINE(PRTUINT32U) RTUInt32AssignU8(PRTUINT32U pValueResult, uint8_t u8Value)
{
    pValueResult->s.Lo = u8Value;
    pValueResult->s.Hi = 0;
    return pValueResult;
}


/**
 * Assigns a 16-bit unsigned integer value to 32-bit unsigned integer.
 *
 * @returns pValueResult
 * @param   pValueResult        The result variable.
 * @param   u16Value            The 16-bit unsigned integer value.
 */
DECLINLINE(PRTUINT32U) RTUInt32AssignU16(PRTUINT32U pValueResult, uint16_t u16Value)
{
    pValueResult->s.Lo = u16Value;
    pValueResult->s.Hi = 0;
    return pValueResult;
}


/**
 * Adds two 32-bit unsigned integer values, storing the result in the first.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The first value and result.
 * @param   pValue2         The second value.
 */
DECLINLINE(PRTUINT32U) RTUInt32AssignAdd(PRTUINT32U pValue1Result, PCRTUINT32U pValue2)
{
#ifdef RTUINT32_HAVE_32BIT_BASICS
    pValue1Result->u += pValue2->u;
#else
    uint16_t const uTmp = pValue1Result->s.Lo;
    pValue1Result->s.Lo += pValue2->s.Lo;
    if (pValue1Result->s.Lo < uTmp)
        pValue1Result->s.Hi++;
    pValue1Result->s.Hi += pValue2->s.Hi;
#endif
    return pValue1Result;
}


/**
 * Subtracts two 32-bit unsigned integer values, storing the result in the
 * first.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The minuend value and result.
 * @param   pValue2         The subtrahend value.
 */
DECLINLINE(PRTUINT32U) RTUInt32AssignSub(PRTUINT32U pValue1Result, PCRTUINT32U pValue2)
{
#ifdef RTUINT32_HAVE_32BIT_BASICS
    pValue1Result->u -= pValue2->u;
#else
    uint32_t const uTmp = pValue1Result->s.Lo;
    pValue1Result->s.Lo -= pValue2->s.Lo;
    if (pValue1Result->s.Lo > uTmp)
        pValue1Result->s.Hi--;
    pValue1Result->s.Hi -= pValue2->s.Hi;
#endif
    return pValue1Result;
}


/**
 * Multiplies two 32-bit unsigned integer values, storing the result in the
 * first.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The first value and result.
 * @param   pValue2         The second value.
 */
DECLINLINE(PRTUINT32U) RTUInt32AssignMul(PRTUINT32U pValue1Result, PCRTUINT32U pValue2)
{
    RTUINT32U Result;
    RTUInt32Mul(&Result, pValue1Result, pValue2);
    *pValue1Result = Result;
    return pValue1Result;
}


/**
 * Divides a 32-bit unsigned integer value by another, storing the result in
 * the first.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The dividend value and result.
 * @param   pValue2         The divisor value.
 */
DECLINLINE(PRTUINT32U) RTUInt32AssignDiv(PRTUINT32U pValue1Result, PCRTUINT32U pValue2)
{
    RTUINT32U Result;
    RTUINT32U Ignored;
    RTUInt32DivRem(&Result, &Ignored, pValue1Result, pValue2);
    *pValue1Result = Result;
    return pValue1Result;
}


/**
 * Divides a 32-bit unsigned integer value by another, storing the remainder in
 * the first.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The dividend value and result (remainder).
 * @param   pValue2         The divisor value.
 */
DECLINLINE(PRTUINT32U) RTUInt32AssignMod(PRTUINT32U pValue1Result, PCRTUINT32U pValue2)
{
    RTUINT32U Ignored;
    RTUINT32U Result;
    RTUInt32DivRem(&Ignored, &Result, pValue1Result, pValue2);
    *pValue1Result = Result;
    return pValue1Result;
}


/**
 * Performs a bitwise AND of two 32-bit unsigned integer values and assigned
 * the result to the first one.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The first value and result.
 * @param   pValue2         The second value.
 */
DECLINLINE(PRTUINT32U) RTUInt32AssignAnd(PRTUINT32U pValue1Result, PCRTUINT32U pValue2)
{
    pValue1Result->s.Hi &= pValue2->s.Hi;
    pValue1Result->s.Lo &= pValue2->s.Lo;
    return pValue1Result;
}


/**
 * Performs a bitwise AND of a 32-bit unsigned integer value and a mask made up
 * of the first N bits, assigning the result to the the 32-bit value.
 *
 * @returns pValueResult.
 * @param   pValueResult    The value and result.
 * @param   cBits           The number of bits to AND (counting from the first
 *                          bit).
 */
DECLINLINE(PRTUINT32U) RTUInt32AssignAndNFirstBits(PRTUINT32U pValueResult, unsigned cBits)
{
#ifdef RTUINT32_HAVE_32BIT_BASICS
    if (cBits < 32)
        pValueResult->u &= RT_BIT_32(cBits) - 1;
#else
    if (cBits <= 16)
    {
        if (cBits != 16)
            pValueResult->s.Lo &= (UINT16_C(1) << cBits) - 1;
        pValueResult->s.Hi = 0;
    }
    else if (cBits < 16)
        pValueResult->s.Hi &= (UINT16_C(1) << (cBits - 16)) - 1;
#endif
    return pValueResult;
}


/**
 * Performs a bitwise OR of two 32-bit unsigned integer values and assigned
 * the result to the first one.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The first value and result.
 * @param   pValue2         The second value.
 */
DECLINLINE(PRTUINT32U) RTUInt32AssignOr(PRTUINT32U pValue1Result, PCRTUINT32U pValue2)
{
    pValue1Result->s.Hi |= pValue2->s.Hi;
    pValue1Result->s.Lo |= pValue2->s.Lo;
    return pValue1Result;
}


/**
 * ORs in a bit and assign the result to the input value.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The first value and result.
 * @param   iBit            The bit to set (0 based).
 */
DECLINLINE(PRTUINT32U) RTUInt32AssignOrBit(PRTUINT32U pValue1Result, unsigned iBit)
{
#ifdef RTUINT32_HAVE_32BIT_BASICS
    pValue1Result->u |= RT_BIT_32(iBit);
#else
    if (iBit >= 32)
        pValue1Result->s.Hi |= UINT16_C(1) << (iBit - 32);
    else
        pValue1Result->s.Lo |= UINT16_C(1) << iBit;
#endif
    return pValue1Result;
}



/**
 * Performs a bitwise XOR of two 32-bit unsigned integer values and assigned
 * the result to the first one.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The first value and result.
 * @param   pValue2         The second value.
 */
DECLINLINE(PRTUINT32U) RTUInt32AssignXor(PRTUINT32U pValue1Result, PCRTUINT32U pValue2)
{
    pValue1Result->s.Hi ^= pValue2->s.Hi;
    pValue1Result->s.Lo ^= pValue2->s.Lo;
    return pValue1Result;
}


/**
 * Performs a bitwise left shift on a 32-bit unsigned integer value, assigning
 * the result to it.
 *
 * @returns pValueResult.
 * @param   pValueResult    The first value and result.
 * @param   cBits           The number of bits to shift.
 */
DECLINLINE(PRTUINT32U) RTUInt32AssignShiftLeft(PRTUINT32U pValueResult, int cBits)
{
#ifndef RTUINT32_HAVE_32BIT_BASICS
    RTUINT32U const InVal = *pValueResult;
#endif
    if (cBits > 0)
    {
        /* (left shift) */
        cBits &= 31;
#ifdef RTUINT32_HAVE_32BIT_BASICS
        pValueResult->u <<= cBits;
#else
        if (cBits >= 16)
        {
            pValueResult->s.Lo  = 0;
            pValueResult->s.Hi  = InVal.s.Lo << (cBits - 16);
        }
        else
        {
            pValueResult->s.Hi  = InVal.s.Hi << cBits;
            pValueResult->s.Hi |= InVal.s.Lo >> (16 - cBits);
            pValueResult->s.Lo  = InVal.s.Lo << cBits;
        }
#endif
    }
    else if (cBits < 0)
    {
        /* (right shift) */
        cBits = -cBits;
        cBits &= 31;
#ifdef RTUINT32_HAVE_32BIT_BASICS
        pValueResult->u >>= cBits;
#else
        if (cBits >= 16)
        {
            pValueResult->s.Hi  = 0;
            pValueResult->s.Lo  = InVal.s.Hi >> (cBits - 16);
        }
        else
        {
            pValueResult->s.Lo  = InVal.s.Lo >> cBits;
            pValueResult->s.Lo |= InVal.s.Hi << (16 - cBits);
            pValueResult->s.Hi  = InVal.s.Hi >> cBits;
        }
#endif
    }
    return pValueResult;
}


/**
 * Performs a bitwise left shift on a 32-bit unsigned integer value, assigning
 * the result to it.
 *
 * @returns pValueResult.
 * @param   pValueResult    The first value and result.
 * @param   cBits           The number of bits to shift.
 */
DECLINLINE(PRTUINT32U) RTUInt32AssignShiftRight(PRTUINT32U pValueResult, int cBits)
{
    return RTUInt32AssignShiftLeft(pValueResult, -cBits);
}


/**
 * Performs a bitwise NOT on a 32-bit unsigned integer value, assigning the
 * result to it.
 *
 * @returns pValueResult
 * @param   pValueResult    The value and result.
 */
DECLINLINE(PRTUINT32U) RTUInt32AssignBitwiseNot(PRTUINT32U pValueResult)
{
    pValueResult->s.Hi = ~pValueResult->s.Hi;
    pValueResult->s.Lo = ~pValueResult->s.Lo;
    return pValueResult;
}


/**
 * Performs a boolean NOT on a 32-bit unsigned integer value, assigning the
 * result to it.
 *
 * @returns pValueResult
 * @param   pValueResult    The value and result.
 */
DECLINLINE(PRTUINT32U) RTUInt32AssignBooleanNot(PRTUINT32U pValueResult)
{
    return RTUInt32AssignBoolean(pValueResult, RTUInt32IsZero(pValueResult));
}


/**
 * Compares two 32-bit unsigned integer values.
 *
 * @retval  0 if equal.
 * @retval  -1 if the first value is smaller than the second.
 * @retval  1  if the first value is larger than the second.
 *
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(int) RTUInt32Compare(PCRTUINT32U pValue1, PCRTUINT32U pValue2)
{
    if (pValue1->s.Hi != pValue2->s.Hi)
        return pValue1->s.Hi > pValue2->s.Hi ? 1 : -1;
    if (pValue1->s.Lo != pValue2->s.Lo)
        return pValue1->s.Lo > pValue2->s.Lo ? 1 : -1;
    return 0;
}


/**
 * Tests if a 64-bit unsigned integer value is smaller than another.
 *
 * @returns true if the first value is smaller, false if not.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(bool) RTUInt32IsSmaller(PCRTUINT32U pValue1, PCRTUINT32U pValue2)
{
#ifdef RTUINT32_HAVE_32BIT_BASICS
    return pValue1->u < pValue2->u;
#else
    return pValue1->s.Hi < pValue2->s.Hi
        || (   pValue1->s.Hi == pValue2->s.Hi
            && pValue1->s.Lo  < pValue2->s.Lo);
#endif
}


/**
 * Tests if a 32-bit unsigned integer value is larger than another.
 *
 * @returns true if the first value is larger, false if not.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(bool) RTUInt32IsLarger(PCRTUINT32U pValue1, PCRTUINT32U pValue2)
{
#ifdef RTUINT32_HAVE_32BIT_BASICS
    return pValue1->u > pValue2->u;
#else
    return pValue1->s.Hi > pValue2->s.Hi
        || (   pValue1->s.Hi == pValue2->s.Hi
            && pValue1->s.Lo  > pValue2->s.Lo);
#endif
}


/**
 * Tests if a 64-bit unsigned integer value is larger or equal than another.
 *
 * @returns true if the first value is larger or equal, false if not.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(bool) RTUInt32IsLargerOrEqual(PCRTUINT32U pValue1, PCRTUINT32U pValue2)
{
#ifdef RTUINT32_HAVE_32BIT_BASICS
    return pValue1->u >= pValue2->u;
#else
    return pValue1->s.Hi > pValue2->s.Hi
        || (   pValue1->s.Hi == pValue2->s.Hi
            && pValue1->s.Lo >= pValue2->s.Lo);
#endif
}


/**
 * Tests if two 64-bit unsigned integer values not equal.
 *
 * @returns true if equal, false if not equal.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(bool) RTUInt32IsEqual(PCRTUINT32U pValue1, PCRTUINT32U pValue2)
{
#ifdef RTUINT32_HAVE_32BIT_BASICS
    return pValue1->u == pValue2->u;
#else
    return pValue1->s.Hi == pValue2->s.Hi
        && pValue1->s.Lo == pValue2->s.Lo;
#endif
}


/**
 * Tests if two 64-bit unsigned integer values are not equal.
 *
 * @returns true if not equal, false if equal.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(bool) RTUInt32IsNotEqual(PCRTUINT32U pValue1, PCRTUINT32U pValue2)
{
    return !RTUInt32IsEqual(pValue1, pValue2);
}


/**
 * Sets a bit in a 32-bit unsigned integer type.
 *
 * @returns pValueResult.
 * @param   pValueResult    The input and output value.
 * @param   iBit            The bit to set.
 */
DECLINLINE(PRTUINT32U) RTUInt32BitSet(PRTUINT32U pValueResult, unsigned iBit)
{
#ifdef RTUINT32_HAVE_32BIT_BASICS
    if (iBit < 32)
        pValueResult->u |= RT_BIT_32(iBit);
#else
    if (iBit < 16)
        pValueResult->s.Lo |= UINT16_C(1) << iBit;
    else if (iBit < 32)
        pValueResult->s.Hi |= UINT16_C(1) << (iBit - 32);
#endif
    return pValueResult;
}


/**
 * Sets a bit in a 32-bit unsigned integer type.
 *
 * @returns pValueResult.
 * @param   pValueResult    The input and output value.
 * @param   iBit            The bit to set.
 */
DECLINLINE(PRTUINT32U) RTUInt32BitClear(PRTUINT32U pValueResult, unsigned iBit)
{
#ifdef RTUINT32_HAVE_32BIT_BASICS
    if (iBit < 32)
        pValueResult->u &= ~RT_BIT_32(iBit);

#else
    if (iBit < 16)
        pValueResult->s.Lo &= ~RT_BIT_32(iBit);
    else if (iBit < 32)
        pValueResult->s.Hi &= ~RT_BIT_32(iBit - 32);
#endif
    return pValueResult;
}


/**
 * Tests if a bit in a 32-bit unsigned integer value is set.
 *
 * @returns pValueResult.
 * @param   pValueResult    The input and output value.
 * @param   iBit            The bit to test.
 */
DECLINLINE(bool) RTUInt32BitTest(PRTUINT32U pValueResult, unsigned iBit)
{
    bool fRc;
#ifdef RTUINT32_HAVE_32BIT_BASICS
    if (iBit < 32)
        fRc = RT_BOOL(pValueResult->u & RT_BIT_32(iBit));
#else
    if (iBit < 16)
        fRc = RT_BOOL(pValueResult->s.Lo & (UINT16_C(1) << iBit));
    else if (iBit < 32)
        fRc = RT_BOOL(pValueResult->s.Hi & (UINT16_C(1) << (iBit - 64)));
#endif
    else
        fRc = false;
    return fRc;
}


/**
 * Set a range of bits a 32-bit unsigned integer value.
 *
 * @returns pValueResult.
 * @param   pValueResult    The input and output value.
 * @param   iFirstBit       The first bit to test.
 * @param   cBits           The number of bits to set.
 */
DECLINLINE(PRTUINT32U) RTUInt32BitSetRange(PRTUINT32U pValueResult, unsigned iFirstBit, unsigned cBits)
{
    /* bounds check & fix. */
    if (iFirstBit < 32)
    {
#ifdef RTUINT32_HAVE_32BIT_BASICS
        if (iFirstBit + cBits < 32)
            pValueResult->u |= (RT_BIT_32(cBits) - 1) << iFirstBit;
        else
            pValueResult->u = UINT32_MAX << iFirstBit;
#else
        if (iFirstBit + cBits > 32)
            cBits = 32 - iFirstBit;
        if (iFirstBit + cBits < 16)
            pValueResult->s.Lo |= ((UINT16_C(1) << cBits) - 1) << iFirstBit;
        else if (iFirstBit + cBits < 32 && iFirstBit >= 16)
            pValueResult->s.Hi |= ((UINT16_C(1) << cBits) - 1) << (iFirstBit - 16);
        else
            while (cBits-- > 0)
                RTUInt32BitSet(pValueResult, iFirstBit++);
#endif
    }
    return pValueResult;
}


/**
 * Test if all the bits of a 32-bit unsigned integer value are set.
 *
 * @returns true if they are, false if they aren't.
 * @param   pValue          The input and output value.
 */
DECLINLINE(bool) RTUInt32BitAreAllSet(PRTUINT32U pValue)
{
    return pValue->s.Hi == UINT16_MAX
        && pValue->s.Lo == UINT16_MAX;
}


/**
 * Test if all the bits of a 32-bit unsigned integer value are clear.
 *
 * @returns true if they are, false if they aren't.
 * @param   pValue          The input and output value.
 */
DECLINLINE(bool) RTUInt32BitAreAllClear(PRTUINT32U pValue)
{
    return RTUInt32IsZero(pValue);
}


DECLINLINE(unsigned) RTUInt32BitCount(PCRTUINT32U pValue)
{
    unsigned cBits;
    if (pValue->s.Hi != 0)
        cBits = 16 + ASMBitLastSetU16(pValue->s.Hi);
    else
        cBits = ASMBitLastSetU16(pValue->s.Lo);
    return cBits;
}


/**
 * Divides a 32-bit unsigned integer value by another, returning both quotient
 * and remainder.
 *
 * @returns pQuotient, NULL if pValue2 is 0.
 * @param   pQuotient           Where to return the quotient.
 * @param   pRemainder          Where to return the remainder.
 * @param   pValue1             The dividend value.
 * @param   pValue2             The divisor value.
 */
DECLINLINE(PRTUINT32U) RTUInt32DivRem(PRTUINT32U pQuotient, PRTUINT32U pRemainder, PCRTUINT32U pValue1, PCRTUINT32U pValue2)
{
    int iDiff;

    /*
     * Sort out all the special cases first.
     */
    /* Divide by zero or 1? */
    if (!pValue2->s.Hi)
    {
        if (!pValue2->s.Lo)
            return NULL;

        if (pValue2->s.Lo == 1)
        {
            RTUInt32SetZero(pRemainder);
            *pQuotient = *pValue1;
            return pQuotient;
        }
        /** @todo RTUInt32DivModByU32 */
    }

    /* Dividend is smaller? */
    iDiff = RTUInt32Compare(pValue1, pValue2);
    if (iDiff < 0)
    {
        *pRemainder = *pValue1;
        RTUInt32SetZero(pQuotient);
    }

    /* The values are equal? */
    else if (iDiff == 0)
    {
        RTUInt32SetZero(pRemainder);
        RTUInt32AssignU8(pQuotient, 1);
    }
    else
    {
        /*
         * Prepare.
         */
        unsigned  iBitAdder = RTUInt32BitCount(pValue1) - RTUInt32BitCount(pValue2);
        RTUINT32U NormDivisor = *pValue2;
        if (iBitAdder)
        {
            RTUInt32ShiftLeft(&NormDivisor, pValue2, iBitAdder);
            if (RTUInt32IsLarger(&NormDivisor, pValue1))
            {
                RTUInt32AssignShiftRight(&NormDivisor, 1);
                iBitAdder--;
            }
        }
        else
            NormDivisor = *pValue2;

        RTUInt32SetZero(pQuotient);
        *pRemainder = *pValue1;

        /*
         * Do the division.
         */
        if (RTUInt32IsLargerOrEqual(pRemainder, pValue2))
        {
            for (;;)
            {
                if (RTUInt32IsLargerOrEqual(pRemainder, &NormDivisor))
                {
                    RTUInt32AssignSub(pRemainder, &NormDivisor);
                    RTUInt32AssignOrBit(pQuotient, iBitAdder);
                }
                if (RTUInt32IsSmaller(pRemainder, pValue2))
                    break;
                RTUInt32AssignShiftRight(&NormDivisor, 1);
                iBitAdder--;
            }
        }
    }
    return pQuotient;
}


/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_uint32_h */

