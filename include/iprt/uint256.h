/** @file
 * IPRT - RTUINT256U methods.
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

#ifndef IPRT_INCLUDED_uint256_h
#define IPRT_INCLUDED_uint256_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/asm.h>
#include <iprt/asm-math.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_uint256 RTUInt256 - 256-bit Unsigned Integer Methods
 * @ingroup grp_rt
 * @{
 */


/**
 * Test if a 256-bit unsigned integer value is zero.
 *
 * @returns true if they are, false if they aren't.
 * @param   pValue          The input and output value.
 */
DECLINLINE(bool) RTUInt256IsZero(PCRTUINT256U pValue)
{
#if ARCH_BITS >= 64
    return pValue->QWords.qw0 == 0
        && pValue->QWords.qw1 == 0
        && pValue->QWords.qw2 == 0
        && pValue->QWords.qw3 == 0;
#else
    return pValue->DWords.dw0 == 0
        && pValue->DWords.dw1 == 0
        && pValue->DWords.dw2 == 0
        && pValue->DWords.dw3 == 0
        && pValue->DWords.dw4 == 0
        && pValue->DWords.dw5 == 0
        && pValue->DWords.dw6 == 0
        && pValue->DWords.dw7 == 0;
#endif
}


/**
 * Set a 256-bit unsigned integer value to zero.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 */
DECLINLINE(PRTUINT256U) RTUInt256SetZero(PRTUINT256U pResult)
{
#if ARCH_BITS >= 64
    pResult->QWords.qw0 = 0;
    pResult->QWords.qw1 = 0;
    pResult->QWords.qw2 = 0;
    pResult->QWords.qw3 = 0;
#else
    pResult->DWords.dw0 = 0;
    pResult->DWords.dw1 = 0;
    pResult->DWords.dw2 = 0;
    pResult->DWords.dw3 = 0;
    pResult->DWords.dw4 = 0;
    pResult->DWords.dw5 = 0;
    pResult->DWords.dw6 = 0;
    pResult->DWords.dw7 = 0;
#endif
    return pResult;
}


/**
 * Set a 256-bit unsigned integer value to the maximum value.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 */
DECLINLINE(PRTUINT256U) RTUInt256SetMax(PRTUINT256U pResult)
{
#if ARCH_BITS >= 64
    pResult->QWords.qw0 = UINT64_MAX;
    pResult->QWords.qw1 = UINT64_MAX;
    pResult->QWords.qw2 = UINT64_MAX;
    pResult->QWords.qw3 = UINT64_MAX;
#else
    pResult->DWords.dw0 = UINT32_MAX;
    pResult->DWords.dw1 = UINT32_MAX;
    pResult->DWords.dw2 = UINT32_MAX;
    pResult->DWords.dw3 = UINT32_MAX;
    pResult->DWords.dw4 = UINT32_MAX;
    pResult->DWords.dw5 = UINT32_MAX;
    pResult->DWords.dw6 = UINT32_MAX;
    pResult->DWords.dw7 = UINT32_MAX;
#endif
    return pResult;
}




/**
 * Adds two 256-bit unsigned integer values.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(PRTUINT256U) RTUInt256Add(PRTUINT256U pResult, PCRTUINT256U pValue1, PCRTUINT256U pValue2)
{
    unsigned uCarry;
    pResult->QWords.qw0 = pValue1->QWords.qw0 + pValue2->QWords.qw0;
    uCarry = pResult->QWords.qw0 < pValue1->QWords.qw0;

    pResult->QWords.qw1 = pValue1->QWords.qw1 + pValue2->QWords.qw1 + uCarry;
    uCarry = uCarry ? pResult->QWords.qw1 <= pValue1->QWords.qw1 : pResult->QWords.qw1 < pValue1->QWords.qw1;

    pResult->QWords.qw2 = pValue1->QWords.qw2 + pValue2->QWords.qw2 + uCarry;
    uCarry = uCarry ? pResult->QWords.qw2 <= pValue1->QWords.qw2 : pResult->QWords.qw2 < pValue1->QWords.qw2;

    pResult->QWords.qw3 = pValue1->QWords.qw3 + pValue2->QWords.qw3 + uCarry;
    return pResult;
}


/**
 * Adds a 256-bit and a 64-bit unsigned integer values.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   uValue2             The second value, 64-bit.
 */
DECLINLINE(PRTUINT256U) RTUInt256AddU64(PRTUINT256U pResult, PCRTUINT256U pValue1, uint64_t uValue2)
{
    pResult->QWords.qw3 = pValue1->QWords.qw3;
    pResult->QWords.qw2 = pValue1->QWords.qw2;
    pResult->QWords.qw1 = pValue1->QWords.qw1;
    pResult->QWords.qw0 = pValue1->QWords.qw0 + uValue2;
    if (pResult->QWords.qw0 < uValue2)
        if (pResult->QWords.qw1++ == UINT64_MAX)
            if (pResult->QWords.qw2++ == UINT64_MAX)
                pResult->QWords.qw3++;
    return pResult;
}


/**
 * Subtracts a 256-bit unsigned integer value from another.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The minuend value.
 * @param   pValue2             The subtrahend value.
 */
DECLINLINE(PRTUINT256U) RTUInt256Sub(PRTUINT256U pResult, PCRTUINT256U pValue1, PCRTUINT256U pValue2)
{
    unsigned uBorrow;
    pResult->QWords.qw0 = pValue1->QWords.qw0 - pValue2->QWords.qw0;
    uBorrow = pResult->QWords.qw0 > pValue1->QWords.qw0;

    pResult->QWords.qw1 = pValue1->QWords.qw1 - pValue2->QWords.qw1 - uBorrow;
    uBorrow = uBorrow ? pResult->QWords.qw1 >= pValue1->QWords.qw1 : pResult->QWords.qw1 > pValue1->QWords.qw1;

    pResult->QWords.qw2 = pValue1->QWords.qw2 - pValue2->QWords.qw2 - uBorrow;
    uBorrow = uBorrow ? pResult->QWords.qw2 >= pValue1->QWords.qw2 : pResult->QWords.qw2 > pValue1->QWords.qw2;

    pResult->QWords.qw3 = pValue1->QWords.qw3 - pValue2->QWords.qw3 - uBorrow;
    return pResult;
}


/**
 * Multiplies two 256-bit unsigned integer values.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
RTDECL(PRTUINT256U) RTUInt256Mul(PRTUINT256U pResult, PCRTUINT256U pValue1, PCRTUINT256U pValue2);

/**
 * Multiplies an 256-bit unsigned integer by a 64-bit unsigned integer value.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   uValue2             The second value, 64-bit.
 */
RTDECL(PRTUINT256U) RTUInt256MulByU64(PRTUINT256U pResult, PCRTUINT256U pValue1, uint64_t uValue2);

/**
 * Divides a 256-bit unsigned integer value by another, returning both quotient
 * and remainder.
 *
 * @returns pQuotient, NULL if pValue2 is 0.
 * @param   pQuotient           Where to return the quotient.
 * @param   pRemainder          Where to return the remainder.
 * @param   pValue1             The dividend value.
 * @param   pValue2             The divisor value.
 */
RTDECL(PRTUINT256U) RTUInt256DivRem(PRTUINT256U pQuotient, PRTUINT256U pRemainder, PCRTUINT256U pValue1, PCRTUINT256U pValue2);

/**
 * Divides a 256-bit unsigned integer value by another.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The dividend value.
 * @param   pValue2             The divisor value.
 */
DECLINLINE(PRTUINT256U) RTUInt256Div(PRTUINT256U pResult, PCRTUINT256U pValue1, PCRTUINT256U pValue2)
{
    RTUINT256U Ignored;
    return RTUInt256DivRem(pResult, &Ignored, pValue1, pValue2);
}


/**
 * Divides a 256-bit unsigned integer value by another, returning the remainder.
 *
 * @returns pResult
 * @param   pResult             The result variable (remainder).
 * @param   pValue1             The dividend value.
 * @param   pValue2             The divisor value.
 */
DECLINLINE(PRTUINT256U) RTUInt256Mod(PRTUINT256U pResult, PCRTUINT256U pValue1, PCRTUINT256U pValue2)
{
    RTUINT256U Ignored;
    RTUInt256DivRem(&Ignored, pResult, pValue1, pValue2);
    return pResult;
}


/**
 * Bitwise AND of two 256-bit unsigned integer values.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(PRTUINT256U) RTUInt256And(PRTUINT256U pResult, PCRTUINT256U pValue1, PCRTUINT256U pValue2)
{
    pResult->QWords.qw0 = pValue1->QWords.qw0 & pValue2->QWords.qw0;
    pResult->QWords.qw1 = pValue1->QWords.qw1 & pValue2->QWords.qw1;
    pResult->QWords.qw2 = pValue1->QWords.qw2 & pValue2->QWords.qw2;
    pResult->QWords.qw3 = pValue1->QWords.qw3 & pValue2->QWords.qw3;
    return pResult;
}


/**
 * Bitwise OR of two 256-bit unsigned integer values.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(PRTUINT256U) RTUInt256Or( PRTUINT256U pResult, PCRTUINT256U pValue1, PCRTUINT256U pValue2)
{
    pResult->QWords.qw0 = pValue1->QWords.qw0 | pValue2->QWords.qw0;
    pResult->QWords.qw1 = pValue1->QWords.qw1 | pValue2->QWords.qw1;
    pResult->QWords.qw2 = pValue1->QWords.qw2 | pValue2->QWords.qw2;
    pResult->QWords.qw3 = pValue1->QWords.qw3 | pValue2->QWords.qw3;
    return pResult;
}


/**
 * Bitwise XOR of two 256-bit unsigned integer values.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(PRTUINT256U) RTUInt256Xor(PRTUINT256U pResult, PCRTUINT256U pValue1, PCRTUINT256U pValue2)
{
    pResult->QWords.qw0 = pValue1->QWords.qw0 ^ pValue2->QWords.qw0;
    pResult->QWords.qw1 = pValue1->QWords.qw1 ^ pValue2->QWords.qw1;
    pResult->QWords.qw2 = pValue1->QWords.qw2 ^ pValue2->QWords.qw2;
    pResult->QWords.qw3 = pValue1->QWords.qw3 ^ pValue2->QWords.qw3;
    return pResult;
}


/**
 * Shifts a 256-bit unsigned integer value @a cBits to the left.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue              The value to shift.
 * @param   cBits               The number of bits to shift it.  This is masked
 *                              by 255 before shifting.
 */
DECLINLINE(PRTUINT256U) RTUInt256ShiftLeft(PRTUINT256U pResult, PCRTUINT256U pValue, unsigned cBits)
{
    /* This is a bit bulky & impractical since we cannot access the data using
       an array because it is organized according to host endianness. Sigh. */
    cBits &= 255;
    if (!(cBits & 0x3f))
    {
        if (cBits == 0)
            *pResult = *pValue;
        else
        {
            pResult->QWords.qw0 = 0;
            if (cBits == 64)
            {
                pResult->QWords.qw1 = pValue->QWords.qw0;
                pResult->QWords.qw2 = pValue->QWords.qw1;
                pResult->QWords.qw3 = pValue->QWords.qw2;
            }
            else
            {
                pResult->QWords.qw1 = 0;
                if (cBits == 128)
                {
                    pResult->QWords.qw2 = pValue->QWords.qw0;
                    pResult->QWords.qw3 = pValue->QWords.qw1;
                }
                else
                {
                    pResult->QWords.qw2 = 0;
                    pResult->QWords.qw3 = pValue->QWords.qw0;
                }
            }
        }
    }
    else if (cBits < 128)
    {
        if (cBits < 64)
        {
            pResult->QWords.qw0  = pValue->QWords.qw0 << cBits;
            pResult->QWords.qw1  = pValue->QWords.qw0 >> (64 - cBits);
            pResult->QWords.qw1 |= pValue->QWords.qw1 << cBits;
            pResult->QWords.qw2  = pValue->QWords.qw1 >> (64 - cBits);
            pResult->QWords.qw2 |= pValue->QWords.qw2 << cBits;
            pResult->QWords.qw3  = pValue->QWords.qw2 >> (64 - cBits);
            pResult->QWords.qw3 |= pValue->QWords.qw3 << cBits;
        }
        else
        {
            cBits -= 64;
            pResult->QWords.qw0  = 0;
            pResult->QWords.qw1  = pValue->QWords.qw0 << cBits;
            pResult->QWords.qw2  = pValue->QWords.qw0 >> (64 - cBits);
            pResult->QWords.qw2 |= pValue->QWords.qw1 << cBits;
            pResult->QWords.qw3  = pValue->QWords.qw1 >> (64 - cBits);
            pResult->QWords.qw3 |= pValue->QWords.qw2 << cBits;
        }
    }
    else
    {
        if (cBits < 192)
        {
            cBits -= 128;
            pResult->QWords.qw0  = 0;
            pResult->QWords.qw1  = 0;
            pResult->QWords.qw2  = pValue->QWords.qw0 << cBits;
            pResult->QWords.qw3  = pValue->QWords.qw0 >> (64 - cBits);
            pResult->QWords.qw3 |= pValue->QWords.qw1 << cBits;
        }
        else
        {
            cBits -= 192;
            pResult->QWords.qw0  = 0;
            pResult->QWords.qw1  = 0;
            pResult->QWords.qw2  = 0;
            pResult->QWords.qw3  = pValue->QWords.qw0 << cBits;
        }
    }
    return pResult;
}


/**
 * Shifts a 256-bit unsigned integer value @a cBits to the right.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue              The value to shift.
 * @param   cBits               The number of bits to shift it.  This is masked
 *                              by 255 before shifting.
 */
DECLINLINE(PRTUINT256U) RTUInt256ShiftRight(PRTUINT256U pResult, PCRTUINT256U pValue, unsigned cBits)
{
    /* This is a bit bulky & impractical since we cannot access the data using
       an array because it is organized according to host endianness. Sigh. */
    cBits &= 255;
    if (!(cBits & 0x3f))
    {
        if (cBits == 0)
            *pResult = *pValue;
        else
        {
            if (cBits == 64)
            {
                pResult->QWords.qw0 = pValue->QWords.qw1;
                pResult->QWords.qw1 = pValue->QWords.qw2;
                pResult->QWords.qw2 = pValue->QWords.qw3;
            }
            else
            {
                if (cBits == 128)
                {
                    pResult->QWords.qw0 = pValue->QWords.qw2;
                    pResult->QWords.qw1 = pValue->QWords.qw3;
                }
                else
                {
                    pResult->QWords.qw0 = pValue->QWords.qw3;
                    pResult->QWords.qw1 = 0;
                }
                pResult->QWords.qw2 = 0;
            }
            pResult->QWords.qw3 = 0;
        }
    }
    else if (cBits < 128)
    {
        if (cBits < 64)
        {
            pResult->QWords.qw0  = pValue->QWords.qw0 >> cBits;
            pResult->QWords.qw0 |= pValue->QWords.qw1 << (64 - cBits);
            pResult->QWords.qw1  = pValue->QWords.qw1 >> cBits;
            pResult->QWords.qw1 |= pValue->QWords.qw2 << (64 - cBits);
            pResult->QWords.qw2  = pValue->QWords.qw2 >> cBits;
            pResult->QWords.qw2 |= pValue->QWords.qw3 << (64 - cBits);
            pResult->QWords.qw3  = pValue->QWords.qw3 >> cBits;
        }
        else
        {
            cBits -= 64;
            pResult->QWords.qw0  = pValue->QWords.qw1 >> cBits;
            pResult->QWords.qw0 |= pValue->QWords.qw2 << (64 - cBits);
            pResult->QWords.qw1  = pValue->QWords.qw2 >> cBits;
            pResult->QWords.qw1 |= pValue->QWords.qw3 << (64 - cBits);
            pResult->QWords.qw2  = pValue->QWords.qw3 >> cBits;
            pResult->QWords.qw3  = 0;
        }
    }
    else
    {
        if (cBits < 192)
        {
            cBits -= 128;
            pResult->QWords.qw0  = pValue->QWords.qw2 >> cBits;
            pResult->QWords.qw0 |= pValue->QWords.qw3 << (64 - cBits);
            pResult->QWords.qw1  = pValue->QWords.qw3 >> cBits;
            pResult->QWords.qw2  = 0;
            pResult->QWords.qw3  = 0;
        }
        else
        {
            cBits -= 192;
            pResult->QWords.qw0  = pValue->QWords.qw3 >> cBits;
            pResult->QWords.qw1  = 0;
            pResult->QWords.qw2  = 0;
            pResult->QWords.qw3  = 0;
        }
    }
    return pResult;
}


/**
 * Boolean not (result 0 or 1).
 *
 * @returns pResult.
 * @param   pResult             The result variable.
 * @param   pValue              The value.
 */
DECLINLINE(PRTUINT256U) RTUInt256BooleanNot(PRTUINT256U pResult, PCRTUINT256U pValue)
{
    pResult->QWords.qw0 = RTUInt256IsZero(pValue);
    pResult->QWords.qw1 = 0;
    pResult->QWords.qw2 = 0;
    pResult->QWords.qw3 = 0;
    return pResult;
}


/**
 * Bitwise not (flips each bit of the 256 bits).
 *
 * @returns pResult.
 * @param   pResult             The result variable.
 * @param   pValue              The value.
 */
DECLINLINE(PRTUINT256U) RTUInt256BitwiseNot(PRTUINT256U pResult, PCRTUINT256U pValue)
{
    pResult->QWords.qw0 = ~pValue->QWords.qw0;
    pResult->QWords.qw1 = ~pValue->QWords.qw1;
    pResult->QWords.qw2 = ~pValue->QWords.qw2;
    pResult->QWords.qw3 = ~pValue->QWords.qw3;
    return pResult;
}


/**
 * Assigns one 256-bit unsigned integer value to another.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue              The value to assign.
 */
DECLINLINE(PRTUINT256U) RTUInt256Assign(PRTUINT256U pResult, PCRTUINT256U pValue)
{
    pResult->QWords.qw0 = pValue->QWords.qw0;
    pResult->QWords.qw1 = pValue->QWords.qw1;
    pResult->QWords.qw2 = pValue->QWords.qw2;
    pResult->QWords.qw3 = pValue->QWords.qw3;
    return pResult;
}


/**
 * Assigns a boolean value to 256-bit unsigned integer.
 *
 * @returns pValueResult
 * @param   pValueResult        The result variable.
 * @param   fValue              The boolean value.
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignBoolean(PRTUINT256U pValueResult, bool fValue)
{
    pValueResult->QWords.qw0 = fValue;
    pValueResult->QWords.qw1 = 0;
    pValueResult->QWords.qw2 = 0;
    pValueResult->QWords.qw3 = 0;
    return pValueResult;
}


/**
 * Assigns a 8-bit unsigned integer value to 256-bit unsigned integer.
 *
 * @returns pValueResult
 * @param   pValueResult        The result variable.
 * @param   u8Value             The 8-bit unsigned integer value.
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignU8(PRTUINT256U pValueResult, uint8_t u8Value)
{
    pValueResult->QWords.qw0 = u8Value;
    pValueResult->QWords.qw1 = 0;
    pValueResult->QWords.qw2 = 0;
    pValueResult->QWords.qw3 = 0;
    return pValueResult;
}


/**
 * Assigns a 16-bit unsigned integer value to 256-bit unsigned integer.
 *
 * @returns pValueResult
 * @param   pValueResult        The result variable.
 * @param   u16Value            The 16-bit unsigned integer value.
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignU16(PRTUINT256U pValueResult, uint16_t u16Value)
{
    pValueResult->QWords.qw0 = u16Value;
    pValueResult->QWords.qw1 = 0;
    pValueResult->QWords.qw2 = 0;
    pValueResult->QWords.qw3 = 0;
    return pValueResult;
}


/**
 * Assigns a 32-bit unsigned integer value to 256-bit unsigned integer.
 *
 * @returns pValueResult
 * @param   pValueResult        The result variable.
 * @param   u32Value            The 32-bit unsigned integer value.
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignU32(PRTUINT256U pValueResult, uint32_t u32Value)
{
    pValueResult->QWords.qw0 = u32Value;
    pValueResult->QWords.qw1 = 0;
    pValueResult->QWords.qw2 = 0;
    pValueResult->QWords.qw3 = 0;
    return pValueResult;
}


/**
 * Assigns a 64-bit unsigned integer value to 256-bit unsigned integer.
 *
 * @returns pValueResult
 * @param   pValueResult        The result variable.
 * @param   u64Value            The 64-bit unsigned integer value.
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignU64(PRTUINT256U pValueResult, uint64_t u64Value)
{
    pValueResult->QWords.qw0 = u64Value;
    pValueResult->QWords.qw1 = 0;
    pValueResult->QWords.qw2 = 0;
    pValueResult->QWords.qw3 = 0;
    return pValueResult;
}


/**
 * Adds two 256-bit unsigned integer values, storing the result in the first.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The first value and result.
 * @param   pValue2         The second value.
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignAdd(PRTUINT256U pValue1Result, PCRTUINT256U pValue2)
{
    RTUINT256U const uTmpValue1 = *pValue1Result; /* lazy bird */
    return RTUInt256Add(pValue1Result, &uTmpValue1, pValue2);
}


/**
 * Adds a 64-bit unsigned integer value to a 256-bit unsigned integer values,
 * storing the result in the 256-bit one.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The first value and result.
 * @param   uValue2         The second value, 64-bit.
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignAddU64(PRTUINT256U pValue1Result, uint64_t uValue2)
{
    RTUINT256U const uTmpValue1 = *pValue1Result; /* lazy bird */
    return RTUInt256AddU64(pValue1Result, &uTmpValue1, uValue2);
}


/**
 * Subtracts two 256-bit unsigned integer values, storing the result in the
 * first.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The minuend value and result.
 * @param   pValue2         The subtrahend value.
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignSub(PRTUINT256U pValue1Result, PCRTUINT256U pValue2)
{
    RTUINT256U const uTmpValue1 = *pValue1Result; /* lazy bird */
    return RTUInt256Sub(pValue1Result, &uTmpValue1, pValue2);
}


#if 0
/**
 * Negates a 256 number, storing the result in the input.
 *
 * @returns pValueResult.
 * @param   pValueResult    The value to negate.
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignNeg(PRTUINT256U pValueResult)
{
    /* result = 0 - value */
    if (pValueResult->s.Lo != 0)
    {
        pValueResult->s.Lo = UINT64_C(0) - pValueResult->s.Lo;
        pValueResult->s.Hi = UINT64_MAX  - pValueResult->s.Hi;
    }
    else
        pValueResult->s.Hi = UINT64_C(0) - pValueResult->s.Hi;
    return pValueResult;
}
#endif


/**
 * Multiplies two 256-bit unsigned integer values, storing the result in the
 * first.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The first value and result.
 * @param   pValue2         The second value.
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignMul(PRTUINT256U pValue1Result, PCRTUINT256U pValue2)
{
    RTUINT256U Result;
    RTUInt256Mul(&Result, pValue1Result, pValue2);
    *pValue1Result = Result;
    return pValue1Result;
}


/**
 * Divides a 256-bit unsigned integer value by another, storing the result in
 * the first.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The dividend value and result.
 * @param   pValue2         The divisor value.
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignDiv(PRTUINT256U pValue1Result, PCRTUINT256U pValue2)
{
    RTUINT256U Result;
    RTUINT256U Ignored;
    RTUInt256DivRem(&Result, &Ignored, pValue1Result, pValue2);
    *pValue1Result = Result;
    return pValue1Result;
}


/**
 * Divides a 256-bit unsigned integer value by another, storing the remainder in
 * the first.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The dividend value and result (remainder).
 * @param   pValue2         The divisor value.
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignMod(PRTUINT256U pValue1Result, PCRTUINT256U pValue2)
{
    RTUINT256U Ignored;
    RTUINT256U Result;
    RTUInt256DivRem(&Ignored, &Result, pValue1Result, pValue2);
    *pValue1Result = Result;
    return pValue1Result;
}


/**
 * Performs a bitwise AND of two 256-bit unsigned integer values and assigned
 * the result to the first one.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The first value and result.
 * @param   pValue2         The second value.
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignAnd(PRTUINT256U pValue1Result, PCRTUINT256U pValue2)
{
    pValue1Result->QWords.qw0 &= pValue2->QWords.qw0;
    pValue1Result->QWords.qw1 &= pValue2->QWords.qw1;
    pValue1Result->QWords.qw2 &= pValue2->QWords.qw2;
    pValue1Result->QWords.qw3 &= pValue2->QWords.qw3;
    return pValue1Result;
}


#if 0
/**
 * Performs a bitwise AND of a 256-bit unsigned integer value and a mask made
 * up of the first N bits, assigning the result to the the 256-bit value.
 *
 * @returns pValueResult.
 * @param   pValueResult    The value and result.
 * @param   cBits           The number of bits to AND (counting from the first
 *                          bit).
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignAndNFirstBits(PRTUINT256U pValueResult, unsigned cBits)
{
    if (cBits <= 64)
    {
        if (cBits != 64)
            pValueResult->s.Lo &= (RT_BIT_64(cBits) - 1);
        pValueResult->s.Hi = 0;
    }
    else if (cBits < 256)
        pValueResult->s.Hi &= (RT_BIT_64(cBits - 64) - 1);
/** @todo \#if ARCH_BITS >= 64 */
    return pValueResult;
}
#endif


/**
 * Performs a bitwise OR of two 256-bit unsigned integer values and assigned
 * the result to the first one.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The first value and result.
 * @param   pValue2         The second value.
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignOr(PRTUINT256U pValue1Result, PCRTUINT256U pValue2)
{
    pValue1Result->QWords.qw0 |= pValue2->QWords.qw0;
    pValue1Result->QWords.qw1 |= pValue2->QWords.qw1;
    pValue1Result->QWords.qw2 |= pValue2->QWords.qw2;
    pValue1Result->QWords.qw3 |= pValue2->QWords.qw3;
    return pValue1Result;
}


DECLINLINE(PRTUINT256U) RTUInt256BitSet(PRTUINT256U pValueResult, unsigned iBit);

/**
 * ORs in a bit and assign the result to the input value.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The first value and result.
 * @param   iBit            The bit to set (0 based).
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignOrBit(PRTUINT256U pValue1Result, uint32_t iBit)
{
    return RTUInt256BitSet(pValue1Result, (unsigned)iBit);
}


/**
 * Performs a bitwise XOR of two 256-bit unsigned integer values and assigned
 * the result to the first one.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The first value and result.
 * @param   pValue2         The second value.
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignXor(PRTUINT256U pValue1Result, PCRTUINT256U pValue2)
{
    pValue1Result->QWords.qw0 ^= pValue2->QWords.qw0;
    pValue1Result->QWords.qw1 ^= pValue2->QWords.qw1;
    pValue1Result->QWords.qw2 ^= pValue2->QWords.qw2;
    pValue1Result->QWords.qw3 ^= pValue2->QWords.qw3;
    return pValue1Result;
}


/**
 * Performs a bitwise left shift on a 256-bit unsigned integer value, assigning
 * the result to it.
 *
 * @returns pValueResult.
 * @param   pValueResult    The first value and result.
 * @param   cBits           The number of bits to shift - signed.  Negative
 *                          values are translated to right shifts.  If the
 *                          absolute value is 256 or higher, the value is set to
 *                          zero.
 *
 * @note    This works differently from RTUInt256ShiftLeft and
 *          RTUInt256ShiftRight in that the shift count is signed and not masked
 *          by 255.
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignShiftLeft(PRTUINT256U pValueResult, int cBits)
{
    if (cBits == 0)
        return pValueResult;
    if (cBits > 0)
    {
        /* (left shift) */
        if (cBits < 256)
        {
            RTUINT256U const InVal = *pValueResult;
            return RTUInt256ShiftLeft(pValueResult, &InVal, cBits);
        }
    }
    else if (cBits > -256)
    {
        /* (right shift) */
        cBits = -cBits;
        RTUINT256U const InVal = *pValueResult;
        return RTUInt256ShiftRight(pValueResult, &InVal, cBits);
    }
    return RTUInt256SetZero(pValueResult);
}


/**
 * Performs a bitwise left shift on a 256-bit unsigned integer value, assigning
 * the result to it.
 *
 * @returns pValueResult.
 * @param   pValueResult    The first value and result.
 * @param   cBits           The number of bits to shift - signed.  Negative
 *                          values are translated to left shifts.  If the
 *                          absolute value is 256 or higher, the value is set to
 *                          zero.
 *
 * @note    This works differently from RTUInt256ShiftRight and
 *          RTUInt256ShiftLeft in that the shift count is signed and not masked
 *          by 255.
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignShiftRight(PRTUINT256U pValueResult, int cBits)
{
    if (cBits == 0)
        return pValueResult;
    if (cBits > 0)
    {
        /* (right shift) */
        if (cBits < 256)
        {
            RTUINT256U const InVal = *pValueResult;
            return RTUInt256ShiftRight(pValueResult, &InVal, cBits);
        }
    }
    else if (cBits > -256)
    {
        /* (left shift) */
        cBits = -cBits;
        RTUINT256U const InVal = *pValueResult;
        return RTUInt256ShiftLeft(pValueResult, &InVal, cBits);
    }
    return RTUInt256SetZero(pValueResult);
}


/**
 * Performs a bitwise NOT on a 256-bit unsigned integer value, assigning the
 * result to it.
 *
 * @returns pValueResult
 * @param   pValueResult    The value and result.
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignBitwiseNot(PRTUINT256U pValueResult)
{
    pValueResult->QWords.qw0 = ~pValueResult->QWords.qw0;
    pValueResult->QWords.qw1 = ~pValueResult->QWords.qw1;
    pValueResult->QWords.qw2 = ~pValueResult->QWords.qw2;
    pValueResult->QWords.qw3 = ~pValueResult->QWords.qw3;
    return pValueResult;
}


/**
 * Performs a boolean NOT on a 256-bit unsigned integer value, assigning the
 * result to it.
 *
 * @returns pValueResult
 * @param   pValueResult    The value and result.
 */
DECLINLINE(PRTUINT256U) RTUInt256AssignBooleanNot(PRTUINT256U pValueResult)
{
    return RTUInt256AssignBoolean(pValueResult, RTUInt256IsZero(pValueResult));
}


/**
 * Compares two 256-bit unsigned integer values.
 *
 * @retval  0 if equal.
 * @retval  -1 if the first value is smaller than the second.
 * @retval  1  if the first value is larger than the second.
 *
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(int) RTUInt256Compare(PCRTUINT256U pValue1, PCRTUINT256U pValue2)
{
    if (pValue1->QWords.qw3 != pValue2->QWords.qw3)
        return pValue1->QWords.qw3 > pValue2->QWords.qw3 ? 1 : -1;
    if (pValue1->QWords.qw2 != pValue2->QWords.qw2)
        return pValue1->QWords.qw2 > pValue2->QWords.qw2 ? 1 : -1;
    if (pValue1->QWords.qw1 != pValue2->QWords.qw1)
        return pValue1->QWords.qw1 > pValue2->QWords.qw1 ? 1 : -1;
    if (pValue1->QWords.qw0 != pValue2->QWords.qw0)
        return pValue1->QWords.qw3 > pValue2->QWords.qw3 ? 1 : -1;
    return 0;
}


/**
 * Tests if a 256-bit unsigned integer value is smaller than another.
 *
 * @returns true if the first value is smaller, false if not.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(bool) RTUInt256IsSmaller(PCRTUINT256U pValue1, PCRTUINT256U pValue2)
{
    return pValue1->QWords.qw3 < pValue2->QWords.qw3
        || (   pValue1->QWords.qw3 == pValue2->QWords.qw3
            && (   pValue1->QWords.qw2  < pValue2->QWords.qw2
                || (   pValue1->QWords.qw2 == pValue2->QWords.qw2
                    && (   pValue1->QWords.qw1  < pValue2->QWords.qw1
                        || (   pValue1->QWords.qw1 == pValue2->QWords.qw1
                            && pValue1->QWords.qw0  < pValue2->QWords.qw0)))));
}


/**
 * Tests if a 256-bit unsigned integer value is larger than another.
 *
 * @returns true if the first value is larger, false if not.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(bool) RTUInt256IsLarger(PCRTUINT256U pValue1, PCRTUINT256U pValue2)
{
    return pValue1->QWords.qw3 > pValue2->QWords.qw3
        || (   pValue1->QWords.qw3 == pValue2->QWords.qw3
            && (   pValue1->QWords.qw2  > pValue2->QWords.qw2
                || (   pValue1->QWords.qw2 == pValue2->QWords.qw2
                    && (   pValue1->QWords.qw1  > pValue2->QWords.qw1
                        || (   pValue1->QWords.qw1 == pValue2->QWords.qw1
                            && pValue1->QWords.qw0  > pValue2->QWords.qw0)))));
}


/**
 * Tests if a 256-bit unsigned integer value is larger or equal than another.
 *
 * @returns true if the first value is larger or equal, false if not.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(bool) RTUInt256IsLargerOrEqual(PCRTUINT256U pValue1, PCRTUINT256U pValue2)
{
    return pValue1->QWords.qw3 > pValue2->QWords.qw3
        || (   pValue1->QWords.qw3 == pValue2->QWords.qw3
            && (   pValue1->QWords.qw2  > pValue2->QWords.qw2
                || (   pValue1->QWords.qw2 == pValue2->QWords.qw2
                    && (   pValue1->QWords.qw1  > pValue2->QWords.qw1
                        || (   pValue1->QWords.qw1 == pValue2->QWords.qw1
                            && pValue1->QWords.qw0 >= pValue2->DWords.dw0)))));
}


/**
 * Tests if two 256-bit unsigned integer values not equal.
 *
 * @returns true if equal, false if not equal.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(bool) RTUInt256IsEqual(PCRTUINT256U pValue1, PCRTUINT256U pValue2)
{
    return pValue1->QWords.qw0 == pValue2->QWords.qw0
        && pValue1->QWords.qw1 == pValue2->QWords.qw1
        && pValue1->QWords.qw2 == pValue2->QWords.qw2
        && pValue1->QWords.qw3 == pValue2->QWords.qw3;
}


/**
 * Tests if two 256-bit unsigned integer values are not equal.
 *
 * @returns true if not equal, false if equal.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(bool) RTUInt256IsNotEqual(PCRTUINT256U pValue1, PCRTUINT256U pValue2)
{
    return !RTUInt256IsEqual(pValue1, pValue2);
}


/**
 * Sets a bit in a 256-bit unsigned integer type.
 *
 * @returns pValueResult.
 * @param   pValueResult    The input and output value.
 * @param   iBit            The bit to set.
 */
DECLINLINE(PRTUINT256U) RTUInt256BitSet(PRTUINT256U pValueResult, unsigned iBit)
{
    if (iBit < 256)
    {
        unsigned idxQWord = iBit >> 6;
#ifdef RT_BIG_ENDIAN
        idxQWord = RT_ELEMENTS(pValueResult->au64) - idxQWord;
#endif
        iBit &= 0x3f;
        pValueResult->au64[idxQWord] |= RT_BIT_64(iBit);
    }
    return pValueResult;
}


/**
 * Sets a bit in a 256-bit unsigned integer type.
 *
 * @returns pValueResult.
 * @param   pValueResult    The input and output value.
 * @param   iBit            The bit to set.
 */
DECLINLINE(PRTUINT256U) RTUInt256BitClear(PRTUINT256U pValueResult, unsigned iBit)
{
    if (iBit < 256)
    {
        unsigned idxQWord = iBit >> 6;
#ifdef RT_BIG_ENDIAN
        idxQWord = RT_ELEMENTS(pValueResult->au64) - idxQWord;
#endif
        iBit &= 0x3f;
        pValueResult->au64[idxQWord] &= ~RT_BIT_64(iBit);
    }
    return pValueResult;
}


/**
 * Tests if a bit in a 256-bit unsigned integer value is set.
 *
 * @returns pValueResult.
 * @param   pValueResult    The input and output value.
 * @param   iBit            The bit to test.
 */
DECLINLINE(bool) RTUInt256BitTest(PRTUINT256U pValueResult, unsigned iBit)
{
    bool fRc;
    if (iBit < 256)
    {
        unsigned idxQWord = iBit >> 6;
#ifdef RT_BIG_ENDIAN
        idxQWord = RT_ELEMENTS(pValueResult->au64) - idxQWord;
#endif
        iBit &= 0x3f;
        fRc = RT_BOOL(pValueResult->au64[idxQWord] & RT_BIT_64(iBit));
    }
    else
        fRc = false;
    return fRc;
}


/**
 * Set a range of bits a 256-bit unsigned integer value.
 *
 * @returns pValueResult.
 * @param   pValueResult    The input and output value.
 * @param   iFirstBit       The first bit to test.
 * @param   cBits           The number of bits to set.
 */
DECLINLINE(PRTUINT256U) RTUInt256BitSetRange(PRTUINT256U pValueResult, unsigned iFirstBit, unsigned cBits)
{
    /* bounds check & fix. */
    if (iFirstBit < 256)
    {
        if (iFirstBit + cBits > 256)
            cBits = 256 - iFirstBit;

        /* Work the au64 array: */
#ifdef RT_BIG_ENDIAN
        int       idxQWord = RT_ELEMENTS(pValueResult->au64) - (iFirstBit >> 6);
        int const idxInc   = -1;
#else
        int       idxQWord = iFirstBit >> 6;
        int const idxInc   = 1;
#endif
        while (cBits > 0)
        {
            unsigned iQWordFirstBit = iFirstBit & 0x3f;
            unsigned cQWordBits     = cBits + iQWordFirstBit >= 64 ? 64 - iQWordFirstBit : cBits;
            pValueResult->au64[idxQWord] |= cQWordBits < 64 ? (RT_BIT_64(cQWordBits) - 1) << iQWordFirstBit : UINT64_MAX;

            idxQWord  += idxInc;
            iFirstBit += cQWordBits;
            cBits     -= cQWordBits;
        }
    }
    return pValueResult;
}


/**
 * Test if all the bits of a 256-bit unsigned integer value are set.
 *
 * @returns true if they are, false if they aren't.
 * @param   pValue          The input and output value.
 */
DECLINLINE(bool) RTUInt256BitAreAllSet(PRTUINT256U pValue)
{
    return pValue->QWords.qw0 == UINT64_MAX
        && pValue->QWords.qw1 == UINT64_MAX
        && pValue->QWords.qw2 == UINT64_MAX
        && pValue->QWords.qw3 == UINT64_MAX;
}


/**
 * Test if all the bits of a 256-bit unsigned integer value are clear.
 *
 * @returns true if they are, false if they aren't.
 * @param   pValue          The input and output value.
 */
DECLINLINE(bool) RTUInt256BitAreAllClear(PRTUINT256U pValue)
{
    return RTUInt256IsZero(pValue);
}


/**
 * Number of significant bits in the value.
 *
 * This is the same a ASMBitLastSetU64 and ASMBitLastSetU32.
 *
 * @returns 0 if zero, 1-base index of the last bit set.
 * @param   pValue              The value to examine.
 */
DECLINLINE(uint32_t) RTUInt256BitCount(PCRTUINT256U pValue)
{
    uint64_t u64;
    uint32_t cBits;
    if ((u64 = pValue->QWords.qw3) != 0)
        cBits = 192;
    else if ((u64 = pValue->QWords.qw2) != 0)
        cBits = 128;
    else if ((u64 = pValue->QWords.qw1) != 0)
        cBits = 64;
    else
    {
        u64 = pValue->QWords.qw0;
        cBits = 0;
    }
    return cBits + ASMBitLastSetU64(u64);
}


/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_uint256_h */

