/** @file
 * IPRT / No-CRT - math.h, AMD inlined functions.
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

#ifndef IPRT_INCLUDED_nocrt_amd64_math_h
#define IPRT_INCLUDED_nocrt_amd64_math_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/asm.h>


#if RT_INLINE_ASM_GNU_STYLE && defined(__SSE__)

DECLINLINE(long double) inline_atan2l(long double lrd1, long double lrd2)
{
    long double lrdResult;
    __asm__ __volatile__("fpatan"
                         : "=t" (lrdResult)
                         : "u" (lrd1),
                           "0" (lrd2)
                         : "st(1)");
    return lrdResult;
}

DECLINLINE(long double) inline_rintl(long double lrd)
{
    long double lrdResult;
    __asm__ __volatile__("frndint"
                         : "=t" (lrdResult)
                         : "0" (lrd));
    return lrdResult;
}

DECLINLINE(float) inline_rintf(float rf)
{
    return (float)inline_rintl(rf);
}

DECLINLINE(double) inline_rint(double rd)
{
    return (double)inline_rintl(rd);
}

DECLINLINE(long double) inline_sqrtl(long double lrd)
{
    long double lrdResult;
    __asm__ __volatile__("fsqrt"
                         : "=t" (lrdResult)
                         : "0" (lrd));
    return lrdResult;
}

DECLINLINE(float) inline_sqrtf(float rf)
{
    return (float)inline_sqrtl(rf);
}

DECLINLINE(double) inline_sqrt(double rd)
{
    return (double)inline_sqrtl(rd);
}


# undef atan2l
# define atan2l(lrd1, lrd2)      inline_atan2l(lrd1, lrd2)
# undef rint
# define rint(rd)                inline_rint(rd)
# undef rintf
# define rintf(rf)               inline_rintf(rf)
# undef rintl
# define rintl(lrd)              inline_rintl(lrd)
# undef sqrt
# define sqrt(rd)                inline_sqrt(rd)
# undef sqrtf
# define sqrtf(rf)               inline_sqrtf(rf)
# undef sqrtl
# define sqrtl(lrd)              inline_sqrtl(lrd)

#endif /* RT_INLINE_ASM_GNU_STYLE */

#endif /* !IPRT_INCLUDED_nocrt_amd64_math_h */

