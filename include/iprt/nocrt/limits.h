/** @file
 * IPRT / No-CRT - Our own limits header.
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

#ifndef IPRT_INCLUDED_nocrt_limits_h
#define IPRT_INCLUDED_nocrt_limits_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

#define CHAR_BIT        8
#define SCHAR_MAX       0x7f
#define SCHAR_MIN       (-0x7f - 1)
#define UCHAR_MAX       0xff
#if 1 /* ASSUMES: signed char */
# define CHAR_MAX       SCHAR_MAX
# define CHAR_MIN       SCHAR_MIN
#else
# define CHAR_MAX       UCHAR_MAX
# define CHAR_MIN       0
#endif

#define WORD_BIT        16
#define USHRT_MAX       0xffff
#define SHRT_MAX        0x7fff
#define SHRT_MIN        (-0x7fff - 1)

/* ASSUMES 32-bit int */
#define UINT_MAX        0xffffffffU
#define INT_MAX         0x7fffffff
#define INT_MIN         (-0x7fffffff - 1)

#if defined(RT_ARCH_X86) || defined(RT_OS_WINDOWS) || defined(RT_ARCH_SPARC) || defined(RT_ARCH_ARM32)
# define LONG_BIT       32
# define ULONG_MAX      0xffffffffU
# define LONG_MAX       0x7fffffff
# define LONG_MIN       (-0x7fffffff - 1)
#elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_SPARC64) || defined(RT_ARCH_ARM64)
# define LONG_BIT       64
# define ULONG_MAX      UINT64_C(0xffffffffffffffff)
# define LONG_MAX       INT64_C(0x7fffffffffffffff)
# define LONG_MIN       (INT64_C(-0x7fffffffffffffff) - 1)
#else
# error "PORTME"
#endif

#define LLONG_BIT       64
#define ULLONG_MAX      UINT64_C(0xffffffffffffffff)
#define LLONG_MAX       INT64_C(0x7fffffffffffffff)
#define LLONG_MIN       (INT64_C(-0x7fffffffffffffff) - 1)

#undef   SIZE_MAX
#undef   SIZE_T_MAX
#undef   SSIZE_MAX
#undef   INTPTR_MAX
#undef   UINTPTR_MAX
#if ARCH_BITS == 32
# define SIZE_T_MAX     0xffffffffU
# define SSIZE_MAX      0x7fffffff
# define INTPTR_MAX     0x7fffffff
# define UINTPTR_MAX    0xffffffffU
#elif ARCH_BITS == 64
# define SIZE_T_MAX     UINT64_C(0xffffffffffffffff)
# define SSIZE_MAX      INT64_C(0x7fffffffffffffff)
# define INTPTR_MAX     INT64_C(0x7fffffffffffffff)
# define UINTPTR_MAX    UINT64_C(0xffffffffffffffff)
#else
# error "huh?"
#endif
#define SIZE_MAX        SIZE_T_MAX

/*#define OFF_MAX         __OFF_MAX
#define OFF_MIN         __OFF_MIN*/

#endif /* !IPRT_INCLUDED_nocrt_limits_h */

