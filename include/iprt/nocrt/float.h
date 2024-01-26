/** @file
 * IPRT / No-CRT - Our minimal float.h.
 */

/*
 * Copyright (C) 2022-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_nocrt_float_h
#define IPRT_INCLUDED_nocrt_float_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

/*
 * Common.
 */
#define FLT_RADIX       2


/*
 * float
 */
#if defined(RT_ARCH_X86) || defined(RT_ARCH_AMD64) || defined(RT_ARCH_ARM64)

# define FLT_MAX            (3.40282347E+38F)
# define FLT_MIN            (1.17549435E-38F)
# define FLT_TRUE_MIN       (1.40129846E-45F)
# define FLT_MAX_EXP        (128)
# define FLT_MIN_EXP        (-125)
# define FLT_MAX_10_EXP     (38)
# define FLT_MIN_10_EXP     (-37)
# define FLT_EPSILON        (1.192092896E-07F)
# define FLT_DIG            (6)
# define FLT_DECIMAL_DIG    (9)
# define FLT_MANT_DIG       (24)
# define FLT_HAS_SUBNORM    (1)

#endif

/*
 * double
 */
#if defined(RT_ARCH_X86) || defined(RT_ARCH_AMD64) || defined(RT_ARCH_ARM64)

# ifdef _MSC_VER
#  define DBL_MAX           (1.7976931348623158E+308)
# else
#  define DBL_MAX           (1.7976931348623157E+308)
# endif
# define DBL_MIN            (2.2250738585072014E-308)
# define DBL_TRUE_MIN       (4.9406564584124654E-324)
# define DBL_MAX_EXP        (1024)
# define DBL_MIN_EXP        (-1021)
# define DBL_MAX_10_EXP     (308)
# define DBL_MIN_10_EXP     (-307)
# define DBL_EPSILON        (2.2204460492503131E-16)
# define DBL_DIG            (15)
# define DBL_DECIMAL_DIG    (17)
# define DBL_MANT_DIG       (53)
# define DBL_HAS_SUBNORM    (1)

#endif

/*
 * long double
 */
#if ((defined(RT_ARCH_X86) || defined(RT_ARCH_AMD64)) && defined(RT_OS_WINDOWS)) || defined(RT_ARCH_ARM64) /*?*/

# define LDBL_MAX           DBL_MAX
# define LDBL_MIN           DBL_MIN
# define LDBL_TRUE_MIN      DBL_TRUE_MIN
# define LDBL_MAX_EXP       DBL_MAX_EXP
# define LDBL_MIN_EXP       DBL_MIN_EXP
# define LDBL_MAX_10_EXP    DBL_MAX_10_EXP
# define LDBL_MIN_10_EXP    DBL_MIN_10_EXP
# define LDBL_EPSILON       DBL_EPSILON
# define LDBL_DIG           DBL_DIG
# define LDBL_DECIMAL_DIG   DBL_DECIMAL_DIG
# define LDBL_MANT_DIG      DBL_MANT_DIG
# define LDBL_HAS_SUBNORM   DBL_HAS_SUBNORM

#elif defined(RT_ARCH_X86) || defined(RT_ARCH_AMD64)

# define LDBL_MAX           (1.1897314953572317650E+4932L)
# define LDBL_MIN           (3.3621031431120935063E-4932L)
# define LDBL_TRUE_MIN      (3.6451995318824746025E-4951L)
# define LDBL_MAX_EXP       (-16381)
# define LDBL_MIN_EXP       (16384)
# define LDBL_MAX_10_EXP    (4932)
# define LDBL_MIN_10_EXP    (-4931)
# define LDBL_EPSILON       (1.0842021724855044340E-19L)
# define LDBL_DIG           (18)
# define LDBL_DECIMAL_DIG   (21)
# define LDBL_MANT_DIG      (64)
# define LDBL_HAS_SUBNORM   (1)

#endif


#endif /* !IPRT_INCLUDED_nocrt_float_h */

