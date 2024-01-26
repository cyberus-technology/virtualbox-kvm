/** @file
 * IPRT - RTUINT128U & uint128_t methods.
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

#ifndef IPRT_INCLUDED_uint128_h
#define IPRT_INCLUDED_uint128_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/asm.h>
#include <iprt/asm-math.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_uint128 RTUInt128 - 128-bit Unsigned Integer Methods
 * @ingroup grp_rt
 * @{
 */


/**
 * Test if a 128-bit unsigned integer value is zero.
 *
 * @returns true if they are, false if they aren't.
 * @param   pValue          The input and output value.
 */
DECLINLINE(bool) RTUInt128IsZero(PCRTUINT128U pValue)
{
#if ARCH_BITS >= 64
    return pValue->s.Hi == 0
        && pValue->s.Lo == 0;
#else
    return pValue->DWords.dw0 == 0
        && pValue->DWords.dw1 == 0
        && pValue->DWords.dw2 == 0
        && pValue->DWords.dw3 == 0;
#endif
}


/**
 * Set a 128-bit unsigned integer value to zero.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 */
DECLINLINE(PRTUINT128U) RTUInt128SetZero(PRTUINT128U pResult)
{
#if ARCH_BITS >= 64
    pResult->s.Hi = 0;
    pResult->s.Lo = 0;
#else
    pResult->DWords.dw0 = 0;
    pResult->DWords.dw1 = 0;
    pResult->DWords.dw2 = 0;
    pResult->DWords.dw3 = 0;
#endif
    return pResult;
}


/**
 * Set a 128-bit unsigned integer value to the maximum value.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 */
DECLINLINE(PRTUINT128U) RTUInt128SetMax(PRTUINT128U pResult)
{
#if ARCH_BITS >= 64
    pResult->s.Hi = UINT64_MAX;
    pResult->s.Lo = UINT64_MAX;
#else
    pResult->DWords.dw0 = UINT32_MAX;
    pResult->DWords.dw1 = UINT32_MAX;
    pResult->DWords.dw2 = UINT32_MAX;
    pResult->DWords.dw3 = UINT32_MAX;
#endif
    return pResult;
}




/**
 * Adds two 128-bit unsigned integer values.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(PRTUINT128U) RTUInt128Add(PRTUINT128U pResult, PCRTUINT128U pValue1, PCRTUINT128U pValue2)
{
    pResult->s.Hi = pValue1->s.Hi + pValue2->s.Hi;
    pResult->s.Lo = pValue1->s.Lo + pValue2->s.Lo;
    if (pResult->s.Lo < pValue1->s.Lo)
        pResult->s.Hi++;
    return pResult;
}


/**
 * Adds a 128-bit and a 64-bit unsigned integer values.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   uValue2             The second value, 64-bit.
 */
DECLINLINE(PRTUINT128U) RTUInt128AddU64(PRTUINT128U pResult, PCRTUINT128U pValue1, uint64_t uValue2)
{
    pResult->s.Hi = pValue1->s.Hi;
    pResult->s.Lo = pValue1->s.Lo + uValue2;
    if (pResult->s.Lo < pValue1->s.Lo)
        pResult->s.Hi++;
    return pResult;
}


/**
 * Subtracts a 128-bit unsigned integer value from another.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The minuend value.
 * @param   pValue2             The subtrahend value.
 */
DECLINLINE(PRTUINT128U) RTUInt128Sub(PRTUINT128U pResult, PCRTUINT128U pValue1, PCRTUINT128U pValue2)
{
    pResult->s.Lo = pValue1->s.Lo - pValue2->s.Lo;
    pResult->s.Hi = pValue1->s.Hi - pValue2->s.Hi;
    if (pResult->s.Lo > pValue1->s.Lo)
        pResult->s.Hi--;
    return pResult;
}


/**
 * Multiplies two 128-bit unsigned integer values.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(PRTUINT128U) RTUInt128Mul(PRTUINT128U pResult, PCRTUINT128U pValue1, PCRTUINT128U pValue2)
{
    RTUINT64U uTmp;

    /* multiply all dwords in v1 by v2.dw0. */
    pResult->s.Lo = (uint64_t)pValue1->DWords.dw0 * pValue2->DWords.dw0;

    uTmp.u = (uint64_t)pValue1->DWords.dw1 * pValue2->DWords.dw0;
    pResult->DWords.dw3 = 0;
    pResult->DWords.dw2 = uTmp.DWords.dw1;
    pResult->DWords.dw1 += uTmp.DWords.dw0;
    if (pResult->DWords.dw1 < uTmp.DWords.dw0)
        if (pResult->DWords.dw2++ == UINT32_MAX)
            pResult->DWords.dw3++;

    pResult->s.Hi += (uint64_t)pValue1->DWords.dw2 * pValue2->DWords.dw0;
    pResult->DWords.dw3     += pValue1->DWords.dw3 * pValue2->DWords.dw0;

    /* multiply dw0, dw1 & dw2 in v1 by v2.dw1. */
    uTmp.u = (uint64_t)pValue1->DWords.dw0 * pValue2->DWords.dw1;
    pResult->DWords.dw1 += uTmp.DWords.dw0;
    if (pResult->DWords.dw1 < uTmp.DWords.dw0)
        if (pResult->DWords.dw2++ == UINT32_MAX)
            pResult->DWords.dw3++;

    pResult->DWords.dw2 += uTmp.DWords.dw1;
    if (pResult->DWords.dw2 < uTmp.DWords.dw1)
        pResult->DWords.dw3++;

    pResult->s.Hi += (uint64_t)pValue1->DWords.dw1 * pValue2->DWords.dw1;
    pResult->DWords.dw3     += pValue1->DWords.dw2 * pValue2->DWords.dw1;

    /* multiply dw0 & dw1 in v1 by v2.dw2. */
    pResult->s.Hi += (uint64_t)pValue1->DWords.dw0 * pValue2->DWords.dw2;
    pResult->DWords.dw3     += pValue1->DWords.dw1 * pValue2->DWords.dw2;

    /* multiply dw0 in v1 by v2.dw3. */
    pResult->DWords.dw3     += pValue1->DWords.dw0 * pValue2->DWords.dw3;

    return pResult;
}


/**
 * Multiplies an 128-bit unsigned integer by a 64-bit unsigned integer value.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   uValue2             The second value, 64-bit.
 */
#if defined(RT_ARCH_AMD64)
RTDECL(PRTUINT128U) RTUInt128MulByU64(PRTUINT128U pResult, PCRTUINT128U pValue1, uint64_t uValue2);
#else
DECLINLINE(PRTUINT128U) RTUInt128MulByU64(PRTUINT128U pResult, PCRTUINT128U pValue1, uint64_t uValue2)
{
    uint32_t const uLoValue2 = (uint32_t)uValue2;
    uint32_t const uHiValue2 = (uint32_t)(uValue2 >> 32);
    RTUINT64U uTmp;

    /* multiply all dwords in v1 by uLoValue1. */
    pResult->s.Lo = (uint64_t)pValue1->DWords.dw0 * uLoValue2;

    uTmp.u = (uint64_t)pValue1->DWords.dw1 * uLoValue2;
    pResult->DWords.dw3 = 0;
    pResult->DWords.dw2 = uTmp.DWords.dw1;
    pResult->DWords.dw1 += uTmp.DWords.dw0;
    if (pResult->DWords.dw1 < uTmp.DWords.dw0)
        if (pResult->DWords.dw2++ == UINT32_MAX)
            pResult->DWords.dw3++;

    pResult->s.Hi += (uint64_t)pValue1->DWords.dw2 * uLoValue2;
    pResult->DWords.dw3     += pValue1->DWords.dw3 * uLoValue2;

    /* multiply dw0, dw1 & dw2 in v1 by uHiValue2. */
    uTmp.u = (uint64_t)pValue1->DWords.dw0 * uHiValue2;
    pResult->DWords.dw1 += uTmp.DWords.dw0;
    if (pResult->DWords.dw1 < uTmp.DWords.dw0)
        if (pResult->DWords.dw2++ == UINT32_MAX)
            pResult->DWords.dw3++;

    pResult->DWords.dw2 += uTmp.DWords.dw1;
    if (pResult->DWords.dw2 < uTmp.DWords.dw1)
        pResult->DWords.dw3++;

    pResult->s.Hi += (uint64_t)pValue1->DWords.dw1 * uHiValue2;
    pResult->DWords.dw3     += pValue1->DWords.dw2 * uHiValue2;

    return pResult;
}
#endif


/**
 * Multiplies two 64-bit unsigned integer values with 128-bit precision.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   uValue1             The first value. 64-bit.
 * @param   uValue2             The second value, 64-bit.
 */
DECLINLINE(PRTUINT128U) RTUInt128MulU64ByU64(PRTUINT128U pResult, uint64_t uValue1, uint64_t uValue2)
{
#ifdef RT_ARCH_AMD64
    pResult->s.Lo = ASMMult2xU64Ret2xU64(uValue1, uValue2, &pResult->s.Hi);
#else
    uint32_t const uLoValue1 = (uint32_t)uValue1;
    uint32_t const uHiValue1 = (uint32_t)(uValue1 >> 32);
    uint32_t const uLoValue2 = (uint32_t)uValue2;
    uint32_t const uHiValue2 = (uint32_t)(uValue2 >> 32);
    RTUINT64U uTmp;

    /* Multiply uLoValue1 and uHiValue1 by uLoValue1. */
    pResult->s.Lo = (uint64_t)uLoValue1 * uLoValue2;

    uTmp.u = (uint64_t)uHiValue1 * uLoValue2;
    pResult->DWords.dw3 = 0;
    pResult->DWords.dw2 = uTmp.DWords.dw1;
    pResult->DWords.dw1 += uTmp.DWords.dw0;
    if (pResult->DWords.dw1 < uTmp.DWords.dw0)
        if (pResult->DWords.dw2++ == UINT32_MAX)
            pResult->DWords.dw3++;

    /* Multiply uLoValue1 and uHiValue1 by uHiValue2. */
    uTmp.u = (uint64_t)uLoValue1 * uHiValue2;
    pResult->DWords.dw1 += uTmp.DWords.dw0;
    if (pResult->DWords.dw1 < uTmp.DWords.dw0)
        if (pResult->DWords.dw2++ == UINT32_MAX)
            pResult->DWords.dw3++;

    pResult->DWords.dw2 += uTmp.DWords.dw1;
    if (pResult->DWords.dw2 < uTmp.DWords.dw1)
        pResult->DWords.dw3++;

    pResult->s.Hi += (uint64_t)uHiValue1 * uHiValue2;
#endif
    return pResult;
}


/**
 * Multiplies an 128-bit unsigned integer by a 64-bit unsigned integer value,
 * returning a 256-bit result (top 64 bits are zero).
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   uValue2             The second value, 64-bit.
 */
#if defined(RT_ARCH_AMD64)
RTDECL(PRTUINT256U) RTUInt128MulByU64Ex(PRTUINT256U pResult, PCRTUINT128U pValue1, uint64_t uValue2);
#else
DECLINLINE(PRTUINT256U) RTUInt128MulByU64Ex(PRTUINT256U pResult, PCRTUINT128U pValue1, uint64_t uValue2)
{
    /* multiply the two qwords in pValue1 by uValue2. */
    uint64_t uTmp = 0;
    pResult->QWords.qw0 = ASMMult2xU64Ret2xU64(pValue1->s.Lo, uValue2, &uTmp);
    pResult->QWords.qw1 = ASMMult2xU64Ret2xU64(pValue1->s.Hi, uValue2, &pResult->QWords.qw2);
    pResult->QWords.qw3 = 0;
    pResult->QWords.qw1 += uTmp;
    if (pResult->QWords.qw1 < uTmp)
        pResult->QWords.qw2++; /* This cannot overflow AFAIK: 0xffff*0xffff = 0xFFFE0001 */

    return pResult;
}
#endif


/**
 * Multiplies two 128-bit unsigned integer values, returning a 256-bit result.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(PRTUINT256U) RTUInt128MulEx(PRTUINT256U pResult, PCRTUINT128U pValue1, PCRTUINT128U pValue2)
{
    RTUInt128MulByU64Ex(pResult, pValue1, pValue2->s.Lo);
    if (pValue2->s.Hi)
    {
        /* Multiply the two qwords in pValue1 by the high part of uValue2. */
        uint64_t uTmpHi = 0;
        uint64_t uTmpLo = ASMMult2xU64Ret2xU64(pValue1->s.Lo, pValue2->s.Hi, &uTmpHi);
        pResult->QWords.qw1 += uTmpLo;
        if (pResult->QWords.qw1 < uTmpLo)
            if (++pResult->QWords.qw2 == 0)
                pResult->QWords.qw3++;  /* (cannot overflow, was == 0) */
        pResult->QWords.qw2 += uTmpHi;
        if (pResult->QWords.qw2 < uTmpHi)
            pResult->QWords.qw3++;      /* (cannot overflow, was <= 1) */

        uTmpLo = ASMMult2xU64Ret2xU64(pValue1->s.Hi, pValue2->s.Hi, &uTmpHi);
        pResult->QWords.qw2 += uTmpLo;
        if (pResult->QWords.qw2 < uTmpLo)
            pResult->QWords.qw3++;      /* (cannot overflow, was <= 2) */
        pResult->QWords.qw3 += uTmpHi;
    }

    return pResult;
}


DECLINLINE(PRTUINT128U) RTUInt128DivRem(PRTUINT128U pQuotient, PRTUINT128U pRemainder, PCRTUINT128U pValue1, PCRTUINT128U pValue2);

/**
 * Divides a 128-bit unsigned integer value by another.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The dividend value.
 * @param   pValue2             The divisor value.
 */
DECLINLINE(PRTUINT128U) RTUInt128Div(PRTUINT128U pResult, PCRTUINT128U pValue1, PCRTUINT128U pValue2)
{
    RTUINT128U Ignored;
    return RTUInt128DivRem(pResult, &Ignored, pValue1, pValue2);
}


/**
 * Divides a 128-bit unsigned integer value by another, returning the remainder.
 *
 * @returns pResult
 * @param   pResult             The result variable (remainder).
 * @param   pValue1             The dividend value.
 * @param   pValue2             The divisor value.
 */
DECLINLINE(PRTUINT128U) RTUInt128Mod(PRTUINT128U pResult, PCRTUINT128U pValue1, PCRTUINT128U pValue2)
{
    RTUINT128U Ignored;
    RTUInt128DivRem(&Ignored, pResult, pValue1, pValue2);
    return pResult;
}


/**
 * Bitwise AND of two 128-bit unsigned integer values.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(PRTUINT128U) RTUInt128And(PRTUINT128U pResult, PCRTUINT128U pValue1, PCRTUINT128U pValue2)
{
    pResult->s.Hi = pValue1->s.Hi & pValue2->s.Hi;
    pResult->s.Lo = pValue1->s.Lo & pValue2->s.Lo;
    return pResult;
}


/**
 * Bitwise OR of two 128-bit unsigned integer values.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(PRTUINT128U) RTUInt128Or( PRTUINT128U pResult, PCRTUINT128U pValue1, PCRTUINT128U pValue2)
{
    pResult->s.Hi = pValue1->s.Hi | pValue2->s.Hi;
    pResult->s.Lo = pValue1->s.Lo | pValue2->s.Lo;
    return pResult;
}


/**
 * Bitwise XOR of two 128-bit unsigned integer values.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(PRTUINT128U) RTUInt128Xor(PRTUINT128U pResult, PCRTUINT128U pValue1, PCRTUINT128U pValue2)
{
    pResult->s.Hi = pValue1->s.Hi ^ pValue2->s.Hi;
    pResult->s.Lo = pValue1->s.Lo ^ pValue2->s.Lo;
    return pResult;
}


/**
 * Shifts a 128-bit unsigned integer value @a cBits to the left.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue              The value to shift.
 * @param   cBits               The number of bits to shift it.
 */
DECLINLINE(PRTUINT128U) RTUInt128ShiftLeft(PRTUINT128U pResult, PCRTUINT128U pValue, int cBits)
{
    cBits &= 127;
    if (cBits < 64)
    {
        pResult->s.Lo = pValue->s.Lo << cBits;
        pResult->s.Hi = (pValue->s.Hi << cBits) | (pValue->s.Lo >> (64 - cBits));
    }
    else
    {
        pResult->s.Lo = 0;
        pResult->s.Hi = pValue->s.Lo << (cBits - 64);
    }
    return pResult;
}


/**
 * Shifts a 128-bit unsigned integer value @a cBits to the right.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue              The value to shift.
 * @param   cBits               The number of bits to shift it.
 */
DECLINLINE(PRTUINT128U) RTUInt128ShiftRight(PRTUINT128U pResult, PCRTUINT128U pValue, int cBits)
{
    cBits &= 127;
    if (cBits < 64)
    {
        pResult->s.Hi = pValue->s.Hi >> cBits;
        pResult->s.Lo = (pValue->s.Lo >> cBits) | (pValue->s.Hi << (64 - cBits));
    }
    else
    {
        pResult->s.Hi = 0;
        pResult->s.Lo = pValue->s.Hi >> (cBits - 64);
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
DECLINLINE(PRTUINT128U) RTUInt128BooleanNot(PRTUINT128U pResult, PCRTUINT128U pValue)
{
    pResult->s.Lo = pValue->s.Lo || pValue->s.Hi ? 0 : 1;
    pResult->s.Hi = 0;
    return pResult;
}


/**
 * Bitwise not (flips each bit of the 128 bits).
 *
 * @returns pResult.
 * @param   pResult             The result variable.
 * @param   pValue              The value.
 */
DECLINLINE(PRTUINT128U) RTUInt128BitwiseNot(PRTUINT128U pResult, PCRTUINT128U pValue)
{
    pResult->s.Hi = ~pValue->s.Hi;
    pResult->s.Lo = ~pValue->s.Lo;
    return pResult;
}


/**
 * Assigns one 128-bit unsigned integer value to another.
 *
 * @returns pResult
 * @param   pResult             The result variable.
 * @param   pValue              The value to assign.
 */
DECLINLINE(PRTUINT128U) RTUInt128Assign(PRTUINT128U pResult, PCRTUINT128U pValue)
{
#if ARCH_BITS >= 64
    pResult->s.Hi = pValue->s.Hi;
    pResult->s.Lo = pValue->s.Lo;
#else
    pResult->DWords.dw0 = pValue->DWords.dw0;
    pResult->DWords.dw1 = pValue->DWords.dw1;
    pResult->DWords.dw2 = pValue->DWords.dw2;
    pResult->DWords.dw3 = pValue->DWords.dw3;
#endif
    return pResult;
}


/**
 * Assigns a boolean value to 128-bit unsigned integer.
 *
 * @returns pValueResult
 * @param   pValueResult        The result variable.
 * @param   fValue              The boolean value.
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignBoolean(PRTUINT128U pValueResult, bool fValue)
{
#if ARCH_BITS >= 64
    pValueResult->s.Lo = fValue;
    pValueResult->s.Hi = 0;
#else
    pValueResult->DWords.dw0 = fValue;
    pValueResult->DWords.dw1 = 0;
    pValueResult->DWords.dw2 = 0;
    pValueResult->DWords.dw3 = 0;
#endif
    return pValueResult;
}


/**
 * Assigns a 8-bit unsigned integer value to 128-bit unsigned integer.
 *
 * @returns pValueResult
 * @param   pValueResult        The result variable.
 * @param   u8Value             The 8-bit unsigned integer value.
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignU8(PRTUINT128U pValueResult, uint8_t u8Value)
{
#if ARCH_BITS >= 64
    pValueResult->s.Lo = u8Value;
    pValueResult->s.Hi = 0;
#else
    pValueResult->DWords.dw0 = u8Value;
    pValueResult->DWords.dw1 = 0;
    pValueResult->DWords.dw2 = 0;
    pValueResult->DWords.dw3 = 0;
#endif
    return pValueResult;
}


/**
 * Assigns a 16-bit unsigned integer value to 128-bit unsigned integer.
 *
 * @returns pValueResult
 * @param   pValueResult        The result variable.
 * @param   u16Value            The 16-bit unsigned integer value.
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignU16(PRTUINT128U pValueResult, uint16_t u16Value)
{
#if ARCH_BITS >= 64
    pValueResult->s.Lo = u16Value;
    pValueResult->s.Hi = 0;
#else
    pValueResult->DWords.dw0 = u16Value;
    pValueResult->DWords.dw1 = 0;
    pValueResult->DWords.dw2 = 0;
    pValueResult->DWords.dw3 = 0;
#endif
    return pValueResult;
}


/**
 * Assigns a 32-bit unsigned integer value to 128-bit unsigned integer.
 *
 * @returns pValueResult
 * @param   pValueResult        The result variable.
 * @param   u32Value            The 32-bit unsigned integer value.
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignU32(PRTUINT128U pValueResult, uint32_t u32Value)
{
#if ARCH_BITS >= 64
    pValueResult->s.Lo = u32Value;
    pValueResult->s.Hi = 0;
#else
    pValueResult->DWords.dw0 = u32Value;
    pValueResult->DWords.dw1 = 0;
    pValueResult->DWords.dw2 = 0;
    pValueResult->DWords.dw3 = 0;
#endif
    return pValueResult;
}


/**
 * Assigns a 64-bit unsigned integer value to 128-bit unsigned integer.
 *
 * @returns pValueResult
 * @param   pValueResult        The result variable.
 * @param   u64Value            The 64-bit unsigned integer value.
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignU64(PRTUINT128U pValueResult, uint64_t u64Value)
{
    pValueResult->s.Lo = u64Value;
    pValueResult->s.Hi = 0;
    return pValueResult;
}


/**
 * Adds two 128-bit unsigned integer values, storing the result in the first.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The first value and result.
 * @param   pValue2         The second value.
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignAdd(PRTUINT128U pValue1Result, PCRTUINT128U pValue2)
{
    uint64_t const uTmp = pValue1Result->s.Lo;
    pValue1Result->s.Lo += pValue2->s.Lo;
    if (pValue1Result->s.Lo < uTmp)
        pValue1Result->s.Hi++;
    pValue1Result->s.Hi += pValue2->s.Hi;
    return pValue1Result;
}


/**
 * Adds a 64-bit unsigned integer value to a 128-bit unsigned integer values,
 * storing the result in the 128-bit one.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The first value and result.
 * @param   uValue2         The second value, 64-bit.
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignAddU64(PRTUINT128U pValue1Result, uint64_t uValue2)
{
    pValue1Result->s.Lo += uValue2;
    if (pValue1Result->s.Lo < uValue2)
        pValue1Result->s.Hi++;
    return pValue1Result;
}


/**
 * Subtracts two 128-bit unsigned integer values, storing the result in the
 * first.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The minuend value and result.
 * @param   pValue2         The subtrahend value.
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignSub(PRTUINT128U pValue1Result, PCRTUINT128U pValue2)
{
    uint64_t const uTmp = pValue1Result->s.Lo;
    pValue1Result->s.Lo -= pValue2->s.Lo;
    if (pValue1Result->s.Lo > uTmp)
        pValue1Result->s.Hi--;
    pValue1Result->s.Hi -= pValue2->s.Hi;
    return pValue1Result;
}


/**
 * Negates a 128 number, storing the result in the input.
 *
 * @returns pValueResult.
 * @param   pValueResult    The value to negate.
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignNeg(PRTUINT128U pValueResult)
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


/**
 * Multiplies two 128-bit unsigned integer values, storing the result in the
 * first.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The first value and result.
 * @param   pValue2         The second value.
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignMul(PRTUINT128U pValue1Result, PCRTUINT128U pValue2)
{
    RTUINT128U Result;
    RTUInt128Mul(&Result, pValue1Result, pValue2);
    *pValue1Result = Result;
    return pValue1Result;
}


/**
 * Divides a 128-bit unsigned integer value by another, storing the result in
 * the first.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The dividend value and result.
 * @param   pValue2         The divisor value.
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignDiv(PRTUINT128U pValue1Result, PCRTUINT128U pValue2)
{
    RTUINT128U Result;
    RTUINT128U Ignored;
    RTUInt128DivRem(&Result, &Ignored, pValue1Result, pValue2);
    *pValue1Result = Result;
    return pValue1Result;
}


/**
 * Divides a 128-bit unsigned integer value by another, storing the remainder in
 * the first.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The dividend value and result (remainder).
 * @param   pValue2         The divisor value.
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignMod(PRTUINT128U pValue1Result, PCRTUINT128U pValue2)
{
    RTUINT128U Ignored;
    RTUINT128U Result;
    RTUInt128DivRem(&Ignored, &Result, pValue1Result, pValue2);
    *pValue1Result = Result;
    return pValue1Result;
}


/**
 * Performs a bitwise AND of two 128-bit unsigned integer values and assigned
 * the result to the first one.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The first value and result.
 * @param   pValue2         The second value.
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignAnd(PRTUINT128U pValue1Result, PCRTUINT128U pValue2)
{
#if ARCH_BITS >= 64
    pValue1Result->s.Hi &= pValue2->s.Hi;
    pValue1Result->s.Lo &= pValue2->s.Lo;
#else
    pValue1Result->DWords.dw0 &= pValue2->DWords.dw0;
    pValue1Result->DWords.dw1 &= pValue2->DWords.dw1;
    pValue1Result->DWords.dw2 &= pValue2->DWords.dw2;
    pValue1Result->DWords.dw3 &= pValue2->DWords.dw3;
#endif
    return pValue1Result;
}


/**
 * Performs a bitwise AND of a 128-bit unsigned integer value and a mask made
 * up of the first N bits, assigning the result to the the 128-bit value.
 *
 * @returns pValueResult.
 * @param   pValueResult    The value and result.
 * @param   cBits           The number of bits to AND (counting from the first
 *                          bit).
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignAndNFirstBits(PRTUINT128U pValueResult, unsigned cBits)
{
    if (cBits <= 64)
    {
        if (cBits != 64)
            pValueResult->s.Lo &= (RT_BIT_64(cBits) - 1);
        pValueResult->s.Hi = 0;
    }
    else if (cBits < 128)
        pValueResult->s.Hi &= (RT_BIT_64(cBits - 64) - 1);
/** @todo \#if ARCH_BITS >= 64 */
    return pValueResult;
}


/**
 * Performs a bitwise OR of two 128-bit unsigned integer values and assigned
 * the result to the first one.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The first value and result.
 * @param   pValue2         The second value.
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignOr(PRTUINT128U pValue1Result, PCRTUINT128U pValue2)
{
#if ARCH_BITS >= 64
    pValue1Result->s.Hi |= pValue2->s.Hi;
    pValue1Result->s.Lo |= pValue2->s.Lo;
#else
    pValue1Result->DWords.dw0 |= pValue2->DWords.dw0;
    pValue1Result->DWords.dw1 |= pValue2->DWords.dw1;
    pValue1Result->DWords.dw2 |= pValue2->DWords.dw2;
    pValue1Result->DWords.dw3 |= pValue2->DWords.dw3;
#endif
    return pValue1Result;
}


/**
 * ORs in a bit and assign the result to the input value.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The first value and result.
 * @param   iBit            The bit to set (0 based).
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignOrBit(PRTUINT128U pValue1Result, uint32_t iBit)
{
#if ARCH_BITS >= 64
    if (iBit >= 64)
        pValue1Result->s.Hi |= RT_BIT_64(iBit - 64);
    else
        pValue1Result->s.Lo |= RT_BIT_64(iBit);
#else
    if (iBit >= 64)
    {
        if (iBit >= 96)
            pValue1Result->DWords.dw3 |= RT_BIT_32(iBit - 96);
        else
            pValue1Result->DWords.dw2 |= RT_BIT_32(iBit - 64);
    }
    else
    {
        if (iBit >= 32)
            pValue1Result->DWords.dw1 |= RT_BIT_32(iBit - 32);
        else
            pValue1Result->DWords.dw0 |= RT_BIT_32(iBit);
    }
#endif
    return pValue1Result;
}



/**
 * Performs a bitwise XOR of two 128-bit unsigned integer values and assigned
 * the result to the first one.
 *
 * @returns pValue1Result.
 * @param   pValue1Result   The first value and result.
 * @param   pValue2         The second value.
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignXor(PRTUINT128U pValue1Result, PCRTUINT128U pValue2)
{
#if ARCH_BITS >= 64
    pValue1Result->s.Hi ^= pValue2->s.Hi;
    pValue1Result->s.Lo ^= pValue2->s.Lo;
#else
    pValue1Result->DWords.dw0 ^= pValue2->DWords.dw0;
    pValue1Result->DWords.dw1 ^= pValue2->DWords.dw1;
    pValue1Result->DWords.dw2 ^= pValue2->DWords.dw2;
    pValue1Result->DWords.dw3 ^= pValue2->DWords.dw3;
#endif
    return pValue1Result;
}


/**
 * Performs a bitwise left shift on a 128-bit unsigned integer value, assigning
 * the result to it.
 *
 * @returns pValueResult.
 * @param   pValueResult    The first value and result.
 * @param   cBits           The number of bits to shift.
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignShiftLeft(PRTUINT128U pValueResult, int cBits)
{
    RTUINT128U const InVal = *pValueResult;
/** @todo \#if ARCH_BITS >= 64 */
    if (cBits > 0)
    {
        /* (left shift) */
        if (cBits >= 128)
            RTUInt128SetZero(pValueResult);
        else if (cBits >= 64)
        {
            pValueResult->s.Lo  = 0;
            pValueResult->s.Hi  = InVal.s.Lo << (cBits - 64);
        }
        else
        {
            pValueResult->s.Hi  = InVal.s.Hi << cBits;
            pValueResult->s.Hi |= InVal.s.Lo >> (64 - cBits);
            pValueResult->s.Lo  = InVal.s.Lo << cBits;
        }
    }
    else if (cBits < 0)
    {
        /* (right shift) */
        cBits = -cBits;
        if (cBits >= 128)
            RTUInt128SetZero(pValueResult);
        else if (cBits >= 64)
        {
            pValueResult->s.Hi  = 0;
            pValueResult->s.Lo  = InVal.s.Hi >> (cBits - 64);
        }
        else
        {
            pValueResult->s.Lo  = InVal.s.Lo >> cBits;
            pValueResult->s.Lo |= InVal.s.Hi << (64 - cBits);
            pValueResult->s.Hi  = InVal.s.Hi >> cBits;
        }
    }
    return pValueResult;
}


/**
 * Performs a bitwise left shift on a 128-bit unsigned integer value, assigning
 * the result to it.
 *
 * @returns pValueResult.
 * @param   pValueResult    The first value and result.
 * @param   cBits           The number of bits to shift.
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignShiftRight(PRTUINT128U pValueResult, int cBits)
{
    return RTUInt128AssignShiftLeft(pValueResult, -cBits);
}


/**
 * Performs a bitwise NOT on a 128-bit unsigned integer value, assigning the
 * result to it.
 *
 * @returns pValueResult
 * @param   pValueResult    The value and result.
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignBitwiseNot(PRTUINT128U pValueResult)
{
#if ARCH_BITS >= 64
    pValueResult->s.Hi = ~pValueResult->s.Hi;
    pValueResult->s.Lo = ~pValueResult->s.Lo;
#else
    pValueResult->DWords.dw0 = ~pValueResult->DWords.dw0;
    pValueResult->DWords.dw1 = ~pValueResult->DWords.dw1;
    pValueResult->DWords.dw2 = ~pValueResult->DWords.dw2;
    pValueResult->DWords.dw3 = ~pValueResult->DWords.dw3;
#endif
    return pValueResult;
}


/**
 * Performs a boolean NOT on a 128-bit unsigned integer value, assigning the
 * result to it.
 *
 * @returns pValueResult
 * @param   pValueResult    The value and result.
 */
DECLINLINE(PRTUINT128U) RTUInt128AssignBooleanNot(PRTUINT128U pValueResult)
{
    return RTUInt128AssignBoolean(pValueResult, RTUInt128IsZero(pValueResult));
}


/**
 * Compares two 128-bit unsigned integer values.
 *
 * @retval  0 if equal.
 * @retval  -1 if the first value is smaller than the second.
 * @retval  1  if the first value is larger than the second.
 *
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(int) RTUInt128Compare(PCRTUINT128U pValue1, PCRTUINT128U pValue2)
{
#if ARCH_BITS >= 64
    if (pValue1->s.Hi != pValue2->s.Hi)
        return pValue1->s.Hi > pValue2->s.Hi ? 1 : -1;
    if (pValue1->s.Lo != pValue2->s.Lo)
        return pValue1->s.Lo > pValue2->s.Lo ? 1 : -1;
    return 0;
#else
    if (pValue1->DWords.dw3 != pValue2->DWords.dw3)
        return pValue1->DWords.dw3 > pValue2->DWords.dw3 ? 1 : -1;
    if (pValue1->DWords.dw2 != pValue2->DWords.dw2)
        return pValue1->DWords.dw2 > pValue2->DWords.dw2 ? 1 : -1;
    if (pValue1->DWords.dw1 != pValue2->DWords.dw1)
        return pValue1->DWords.dw1 > pValue2->DWords.dw1 ? 1 : -1;
    if (pValue1->DWords.dw0 != pValue2->DWords.dw0)
        return pValue1->DWords.dw0 > pValue2->DWords.dw0 ? 1 : -1;
    return 0;
#endif
}


/**
 * Tests if a 128-bit unsigned integer value is smaller than another.
 *
 * @returns true if the first value is smaller, false if not.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(bool) RTUInt128IsSmaller(PCRTUINT128U pValue1, PCRTUINT128U pValue2)
{
#if ARCH_BITS >= 64
    return pValue1->s.Hi < pValue2->s.Hi
        || (   pValue1->s.Hi == pValue2->s.Hi
            && pValue1->s.Lo  < pValue2->s.Lo);
#else
    return pValue1->DWords.dw3 < pValue2->DWords.dw3
        || (   pValue1->DWords.dw3 == pValue2->DWords.dw3
            && (   pValue1->DWords.dw2  < pValue2->DWords.dw2
                || (   pValue1->DWords.dw2 == pValue2->DWords.dw2
                    && (   pValue1->DWords.dw1  < pValue2->DWords.dw1
                        || (   pValue1->DWords.dw1 == pValue2->DWords.dw1
                            && pValue1->DWords.dw0  < pValue2->DWords.dw0)))));
#endif
}


/**
 * Tests if a 128-bit unsigned integer value is larger than another.
 *
 * @returns true if the first value is larger, false if not.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(bool) RTUInt128IsLarger(PCRTUINT128U pValue1, PCRTUINT128U pValue2)
{
#if ARCH_BITS >= 64
    return pValue1->s.Hi > pValue2->s.Hi
        || (   pValue1->s.Hi == pValue2->s.Hi
            && pValue1->s.Lo  > pValue2->s.Lo);
#else
    return pValue1->DWords.dw3 > pValue2->DWords.dw3
        || (   pValue1->DWords.dw3 == pValue2->DWords.dw3
            && (   pValue1->DWords.dw2  > pValue2->DWords.dw2
                || (   pValue1->DWords.dw2 == pValue2->DWords.dw2
                    && (   pValue1->DWords.dw1  > pValue2->DWords.dw1
                        || (   pValue1->DWords.dw1 == pValue2->DWords.dw1
                            && pValue1->DWords.dw0  > pValue2->DWords.dw0)))));
#endif
}


/**
 * Tests if a 128-bit unsigned integer value is larger or equal than another.
 *
 * @returns true if the first value is larger or equal, false if not.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(bool) RTUInt128IsLargerOrEqual(PCRTUINT128U pValue1, PCRTUINT128U pValue2)
{
#if ARCH_BITS >= 64
    return pValue1->s.Hi > pValue2->s.Hi
        || (   pValue1->s.Hi == pValue2->s.Hi
            && pValue1->s.Lo >= pValue2->s.Lo);
#else
    return pValue1->DWords.dw3 > pValue2->DWords.dw3
        || (   pValue1->DWords.dw3 == pValue2->DWords.dw3
            && (   pValue1->DWords.dw2  > pValue2->DWords.dw2
                || (   pValue1->DWords.dw2 == pValue2->DWords.dw2
                    && (   pValue1->DWords.dw1  > pValue2->DWords.dw1
                        || (   pValue1->DWords.dw1 == pValue2->DWords.dw1
                            && pValue1->DWords.dw0 >= pValue2->DWords.dw0)))));
#endif
}


/**
 * Tests if two 128-bit unsigned integer values not equal.
 *
 * @returns true if equal, false if not equal.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(bool) RTUInt128IsEqual(PCRTUINT128U pValue1, PCRTUINT128U pValue2)
{
#if ARCH_BITS >= 64
    return pValue1->s.Hi == pValue2->s.Hi
        && pValue1->s.Lo == pValue2->s.Lo;
#else
    return pValue1->DWords.dw0 == pValue2->DWords.dw0
        && pValue1->DWords.dw1 == pValue2->DWords.dw1
        && pValue1->DWords.dw2 == pValue2->DWords.dw2
        && pValue1->DWords.dw3 == pValue2->DWords.dw3;
#endif
}


/**
 * Tests if two 128-bit unsigned integer values are not equal.
 *
 * @returns true if not equal, false if equal.
 * @param   pValue1             The first value.
 * @param   pValue2             The second value.
 */
DECLINLINE(bool) RTUInt128IsNotEqual(PCRTUINT128U pValue1, PCRTUINT128U pValue2)
{
    return !RTUInt128IsEqual(pValue1, pValue2);
}


/**
 * Sets a bit in a 128-bit unsigned integer type.
 *
 * @returns pValueResult.
 * @param   pValueResult    The input and output value.
 * @param   iBit            The bit to set.
 */
DECLINLINE(PRTUINT128U) RTUInt128BitSet(PRTUINT128U pValueResult, unsigned iBit)
{
    if (iBit < 64)
    {
#if ARCH_BITS >= 64
        pValueResult->s.Lo |= RT_BIT_64(iBit);
#else
        if (iBit < 32)
            pValueResult->DWords.dw0 |= RT_BIT_32(iBit);
        else
            pValueResult->DWords.dw1 |= RT_BIT_32(iBit - 32);
#endif
    }
    else if (iBit < 128)
    {
#if ARCH_BITS >= 64
        pValueResult->s.Hi |= RT_BIT_64(iBit - 64);
#else
        if (iBit < 96)
            pValueResult->DWords.dw2 |= RT_BIT_32(iBit - 64);
        else
            pValueResult->DWords.dw3 |= RT_BIT_32(iBit - 96);
#endif
    }
    return pValueResult;
}


/**
 * Sets a bit in a 128-bit unsigned integer type.
 *
 * @returns pValueResult.
 * @param   pValueResult    The input and output value.
 * @param   iBit            The bit to set.
 */
DECLINLINE(PRTUINT128U) RTUInt128BitClear(PRTUINT128U pValueResult, unsigned iBit)
{
    if (iBit < 64)
    {
#if ARCH_BITS >= 64
        pValueResult->s.Lo &= ~RT_BIT_64(iBit);
#else
        if (iBit < 32)
            pValueResult->DWords.dw0 &= ~RT_BIT_32(iBit);
        else
            pValueResult->DWords.dw1 &= ~RT_BIT_32(iBit - 32);
#endif
    }
    else if (iBit < 128)
    {
#if ARCH_BITS >= 64
        pValueResult->s.Hi &= ~RT_BIT_64(iBit - 64);
#else
        if (iBit < 96)
            pValueResult->DWords.dw2 &= ~RT_BIT_32(iBit - 64);
        else
            pValueResult->DWords.dw3 &= ~RT_BIT_32(iBit - 96);
#endif
    }
    return pValueResult;
}


/**
 * Tests if a bit in a 128-bit unsigned integer value is set.
 *
 * @returns pValueResult.
 * @param   pValueResult    The input and output value.
 * @param   iBit            The bit to test.
 */
DECLINLINE(bool) RTUInt128BitTest(PRTUINT128U pValueResult, unsigned iBit)
{
    bool fRc;
    if (iBit < 64)
    {
#if ARCH_BITS >= 64
        fRc = RT_BOOL(pValueResult->s.Lo & RT_BIT_64(iBit));
#else
        if (iBit < 32)
            fRc = RT_BOOL(pValueResult->DWords.dw0 & RT_BIT_32(iBit));
        else
            fRc = RT_BOOL(pValueResult->DWords.dw1 & RT_BIT_32(iBit - 32));
#endif
    }
    else if (iBit < 128)
    {
#if ARCH_BITS >= 64
        fRc = RT_BOOL(pValueResult->s.Hi & RT_BIT_64(iBit - 64));
#else
        if (iBit < 96)
            fRc = RT_BOOL(pValueResult->DWords.dw2 & RT_BIT_32(iBit - 64));
        else
            fRc = RT_BOOL(pValueResult->DWords.dw3 & RT_BIT_32(iBit - 96));
#endif
    }
    else
        fRc = false;
    return fRc;
}


/**
 * Set a range of bits a 128-bit unsigned integer value.
 *
 * @returns pValueResult.
 * @param   pValueResult    The input and output value.
 * @param   iFirstBit       The first bit to test.
 * @param   cBits           The number of bits to set.
 */
DECLINLINE(PRTUINT128U) RTUInt128BitSetRange(PRTUINT128U pValueResult, unsigned iFirstBit, unsigned cBits)
{
    /* bounds check & fix. */
    if (iFirstBit < 128)
    {
        if (iFirstBit + cBits > 128)
            cBits = 128 - iFirstBit;

#if ARCH_BITS >= 64
        if (iFirstBit + cBits < 64)
            pValueResult->s.Lo |= (RT_BIT_64(cBits) - 1) << iFirstBit;
        else if (iFirstBit + cBits < 128 && iFirstBit >= 64)
            pValueResult->s.Hi |= (RT_BIT_64(cBits) - 1) << (iFirstBit - 64);
        else
#else
        if (iFirstBit + cBits < 32)
            pValueResult->DWords.dw0 |= (RT_BIT_32(cBits) - 1) << iFirstBit;
        else if (iFirstBit + cBits < 64 && iFirstBit >= 32)
            pValueResult->DWords.dw1 |= (RT_BIT_32(cBits) - 1) << (iFirstBit - 32);
        else if (iFirstBit + cBits < 96 && iFirstBit >= 64)
            pValueResult->DWords.dw2 |= (RT_BIT_32(cBits) - 1) << (iFirstBit - 64);
        else if (iFirstBit + cBits < 128 && iFirstBit >= 96)
            pValueResult->DWords.dw3 |= (RT_BIT_32(cBits) - 1) << (iFirstBit - 96);
        else
#endif
            while (cBits-- > 0)
                RTUInt128BitSet(pValueResult, iFirstBit++);
    }
    return pValueResult;
}


/**
 * Test if all the bits of a 128-bit unsigned integer value are set.
 *
 * @returns true if they are, false if they aren't.
 * @param   pValue          The input and output value.
 */
DECLINLINE(bool) RTUInt128BitAreAllSet(PRTUINT128U pValue)
{
#if ARCH_BITS >= 64
    return pValue->s.Hi == UINT64_MAX
        && pValue->s.Lo == UINT64_MAX;
#else
    return pValue->DWords.dw0 == UINT32_MAX
        && pValue->DWords.dw1 == UINT32_MAX
        && pValue->DWords.dw2 == UINT32_MAX
        && pValue->DWords.dw3 == UINT32_MAX;
#endif
}


/**
 * Test if all the bits of a 128-bit unsigned integer value are clear.
 *
 * @returns true if they are, false if they aren't.
 * @param   pValue          The input and output value.
 */
DECLINLINE(bool) RTUInt128BitAreAllClear(PRTUINT128U pValue)
{
#if ARCH_BITS >= 64
    return pValue->s.Hi == 0
        && pValue->s.Lo == 0;
#else
    return pValue->DWords.dw0 == 0
        && pValue->DWords.dw1 == 0
        && pValue->DWords.dw2 == 0
        && pValue->DWords.dw3 == 0;
#endif
}


/**
 * Number of significant bits in the value.
 *
 * This is the same a ASMBitLastSetU64 and ASMBitLastSetU32.
 *
 * @returns 0 if zero, 1-base index of the last bit set.
 * @param   pValue              The value to examine.
 */
DECLINLINE(uint32_t) RTUInt128BitCount(PCRTUINT128U pValue)
{
    uint32_t cBits;
    if (pValue->s.Hi != 0)
    {
        if (pValue->DWords.dw3)
            cBits = 96 + ASMBitLastSetU32(pValue->DWords.dw3);
        else
            cBits = 64 + ASMBitLastSetU32(pValue->DWords.dw2);
    }
    else
    {
        if (pValue->DWords.dw1)
            cBits = 32 + ASMBitLastSetU32(pValue->DWords.dw1);
        else
            cBits =  0 + ASMBitLastSetU32(pValue->DWords.dw0);
    }
    return cBits;
}


/**
 * Divides a 128-bit unsigned integer value by another, returning both quotient
 * and remainder.
 *
 * @returns pQuotient, NULL if pValue2 is 0.
 * @param   pQuotient           Where to return the quotient.
 * @param   pRemainder          Where to return the remainder.
 * @param   pValue1             The dividend value.
 * @param   pValue2             The divisor value.
 */
DECLINLINE(PRTUINT128U) RTUInt128DivRem(PRTUINT128U pQuotient, PRTUINT128U pRemainder, PCRTUINT128U pValue1, PCRTUINT128U pValue2)
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
            RTUInt128SetZero(pRemainder);
            *pQuotient = *pValue1;
            return pQuotient;
        }
        /** @todo RTUint128DivModBy64 */
    }

    /* Dividend is smaller? */
    iDiff = RTUInt128Compare(pValue1, pValue2);
    if (iDiff < 0)
    {
        *pRemainder = *pValue1;
        RTUInt128SetZero(pQuotient);
    }

    /* The values are equal? */
    else if (iDiff == 0)
    {
        RTUInt128SetZero(pRemainder);
        RTUInt128AssignU64(pQuotient, 1);
    }
    else
    {
        /*
         * Prepare.
         */
        uint32_t   iBitAdder = RTUInt128BitCount(pValue1) - RTUInt128BitCount(pValue2);
        RTUINT128U NormDivisor = *pValue2;
        if (iBitAdder)
        {
            RTUInt128ShiftLeft(&NormDivisor, pValue2, iBitAdder);
            if (RTUInt128IsLarger(&NormDivisor, pValue1))
            {
                RTUInt128AssignShiftRight(&NormDivisor, 1);
                iBitAdder--;
            }
        }
        else
            NormDivisor = *pValue2;

        RTUInt128SetZero(pQuotient);
        *pRemainder = *pValue1;

        /*
         * Do the division.
         */
        if (RTUInt128IsLargerOrEqual(pRemainder, pValue2))
        {
            for (;;)
            {
                if (RTUInt128IsLargerOrEqual(pRemainder, &NormDivisor))
                {
                    RTUInt128AssignSub(pRemainder, &NormDivisor);
                    RTUInt128AssignOrBit(pQuotient, iBitAdder);
                }
                if (RTUInt128IsSmaller(pRemainder, pValue2))
                    break;
                RTUInt128AssignShiftRight(&NormDivisor, 1);
                iBitAdder--;
            }
        }
    }
    return pQuotient;
}


/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_uint128_h */

