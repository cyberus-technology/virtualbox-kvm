/** @file
 * IPRT - Big Integer Numbers.
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_bignum_h
#define IPRT_INCLUDED_bignum_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rtbignum      RTBigNum - Big Integer Numbers
 * @ingroup grp_rt
 * @{
 */

/** The big integer number element type. */
#if ARCH_BITS == 64
typedef uint64_t RTBIGNUMELEMENT;
#else
typedef uint32_t RTBIGNUMELEMENT;
#endif
/** Pointer to a big integer number element. */
typedef RTBIGNUMELEMENT        *PRTBIGNUMELEMENT;
/** Pointer to a const big integer number element. */
typedef RTBIGNUMELEMENT const  *PCRTBIGNUMELEMENT;

/** The size (in bytes) of one array element. */
#if ARCH_BITS == 64
# define RTBIGNUM_ELEMENT_SIZE          8
#else
# define RTBIGNUM_ELEMENT_SIZE          4
#endif
/** The number of bits in one array element. */
#define RTBIGNUM_ELEMENT_BITS           (RTBIGNUM_ELEMENT_SIZE * 8)
/** Returns the bitmask corrsponding to given bit number. */
#if ARCH_BITS == 64
# define RTBIGNUM_ELEMENT_BIT(iBit)     RT_BIT_64(iBit)
#else
# define RTBIGNUM_ELEMENT_BIT(iBit)     RT_BIT_32(iBit)
#endif
/** The maximum value one element can hold. */
#if ARCH_BITS == 64
# define RTBIGNUM_ELEMENT_MAX           UINT64_MAX
#else
# define RTBIGNUM_ELEMENT_MAX           UINT32_MAX
#endif
/** Mask including all the element bits set to 1. */
#define RTBIGNUM_ELEMENT_MASK           RTBIGNUM_ELEMENT_MAX


/**
 * IPRT big integer number.
 */
typedef struct RTBIGNUM
{
    /** Elements array where the magnitue of the value is stored.  */
    RTBIGNUMELEMENT    *pauElements;
    /** The current number of elements we're using in the pauElements array. */
    uint32_t            cUsed;
    /** The current allocation size of pauElements. */
    uint32_t            cAllocated;
    /** Reserved for future use. */
    uint32_t            uReserved;

    /** Set if it's a negative number, clear if positive or zero. */
    uint32_t            fNegative : 1;

    /** Whether to use a the data is sensitive (RTBIGNUMINIT_F_SENSITIVE). */
    uint32_t            fSensitive : 1;
    /** The number is currently scrambled */
    uint32_t            fCurScrambled : 1;

    /** Bits reserved for future use. */
    uint32_t            fReserved : 30;
} RTBIGNUM;


RTDECL(int) RTBigNumInit(PRTBIGNUM pBigNum, uint32_t fFlags, void const *pvRaw, size_t cbRaw);
RTDECL(int) RTBigNumInitZero(PRTBIGNUM pBigNum, uint32_t fFlags);

/** @name RTBIGNUMINIT_F_XXX - RTBigNumInit flags.
 * @{ */
/** The number is sensitive so use a safer allocator, scramble it when not
 * in use, and apply RTMemWipeThoroughly before freeing.  The RTMemSafer API
 * takes care of these things.
 * @note When using this flag, concurrent access is not possible! */
#define RTBIGNUMINIT_F_SENSITIVE        RT_BIT(0)
/** Big endian number. */
#define RTBIGNUMINIT_F_ENDIAN_BIG       RT_BIT(1)
/** Little endian number. */
#define RTBIGNUMINIT_F_ENDIAN_LITTLE    RT_BIT(2)
/** The raw number is unsigned. */
#define RTBIGNUMINIT_F_UNSIGNED         RT_BIT(3)
/** The raw number is signed. */
#define RTBIGNUMINIT_F_SIGNED           RT_BIT(4)
/** @} */

RTDECL(int) RTBigNumClone(PRTBIGNUM pBigNum, PCRTBIGNUM pSrc);

RTDECL(int) RTBigNumDestroy(PRTBIGNUM pBigNum);


/**
 * The minimum number of bits require store the two's complement representation
 * of the number.
 *
 * @returns Width in number of bits.
 * @param   pBigNum         The big number.
 */
RTDECL(uint32_t) RTBigNumBitWidth(PCRTBIGNUM pBigNum);
RTDECL(uint32_t) RTBigNumByteWidth(PCRTBIGNUM pBigNum);


/**
 * Converts the big number to a sign-extended big endian byte sequence.
 *
 * @returns IPRT status code
 * @retval  VERR_BUFFER_OVERFLOW if the specified buffer is too small.
 * @param   pBigNum         The big number.
 * @param   pvBuf           The output buffer (size is at least cbWanted).
 * @param   cbWanted        The number of bytes wanted.
 */
RTDECL(int) RTBigNumToBytesBigEndian(PCRTBIGNUM pBigNum, void *pvBuf, size_t cbWanted);

/**
 * Compares two numbers.
 *
 * @retval  -1 if pLeft < pRight.
 * @retval  0 if pLeft == pRight.
 * @retval  1 if pLeft > pRight.
 *
 * @param   pLeft           The left side number.
 * @param   pRight          The right side number.
 */
RTDECL(int) RTBigNumCompare(PRTBIGNUM pLeft, PRTBIGNUM pRight);
RTDECL(int) RTBigNumCompareWithU64(PRTBIGNUM pLeft, uint64_t uRight);
RTDECL(int) RTBigNumCompareWithS64(PRTBIGNUM pLeft, int64_t iRight);

RTDECL(int) RTBigNumAssign(PRTBIGNUM pDst, PCRTBIGNUM pSrc);
RTDECL(int) RTBigNumNegate(PRTBIGNUM pResult, PCRTBIGNUM pBigNum);
RTDECL(int) RTBigNumNegateThis(PRTBIGNUM pThis);

RTDECL(int) RTBigNumAdd(PRTBIGNUM pResult, PCRTBIGNUM pAugend, PCRTBIGNUM pAddend);
RTDECL(int) RTBigNumSubtract(PRTBIGNUM pResult, PCRTBIGNUM pMinuend, PCRTBIGNUM pSubtrahend);
RTDECL(int) RTBigNumMultiply(PRTBIGNUM pResult, PCRTBIGNUM pMultiplicand, PCRTBIGNUM pMultiplier);
RTDECL(int) RTBigNumDivide(PRTBIGNUM pQuotient, PRTBIGNUM pRemainder, PCRTBIGNUM pDividend, PCRTBIGNUM pDivisor);
RTDECL(int) RTBigNumDivideKnuth(PRTBIGNUM pQuotient, PRTBIGNUM pRemainder, PCRTBIGNUM pDividend, PCRTBIGNUM pDivisor);
RTDECL(int) RTBigNumDivideLong(PRTBIGNUM pQuotient, PRTBIGNUM pRemainder, PCRTBIGNUM pDividend, PCRTBIGNUM pDivisor);
RTDECL(int) RTBigNumModulo(PRTBIGNUM pRemainder, PCRTBIGNUM pDividend, PCRTBIGNUM pDivisor);
RTDECL(int) RTBigNumExponentiate(PRTBIGNUM pResult, PCRTBIGNUM pBase, PCRTBIGNUM pExponent);
RTDECL(int) RTBigNumShiftLeft(PRTBIGNUM pResult, PCRTBIGNUM pValue, uint32_t cBits);
RTDECL(int) RTBigNumShiftRight(PRTBIGNUM pResult, PCRTBIGNUM pValue, uint32_t cBits);

RTDECL(int) RTBigNumModExp(PRTBIGNUM pResult, PRTBIGNUM pBase, PRTBIGNUM pExponent, PRTBIGNUM pModulus);


/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_bignum_h */

