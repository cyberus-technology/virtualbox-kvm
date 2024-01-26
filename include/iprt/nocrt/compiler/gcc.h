/** @file
 * IPRT / No-CRT - GCC specifics.
 *
 * A quick hack for freebsd where there are no separate location
 * for compiler specific headers like on linux, mingw, os2, ++.
 * This file will be cleaned up later...
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

#ifndef IPRT_INCLUDED_nocrt_compiler_gcc_h
#define IPRT_INCLUDED_nocrt_compiler_gcc_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* stddef.h */
#ifdef __PTRDIFF_TYPE__
typedef __PTRDIFF_TYPE__ ptrdiff_t;
#elif ARCH_BITS == 32
typedef int32_t ptrdiff_t;
#elif ARCH_BITS == 64
typedef int64_t ptrdiff_t;
#else
# error "ARCH_BITS is undefined or incorrect."
#endif
#define _PTRDIFF_T_DECLARED

#ifdef __SIZE_TYPE__
typedef __SIZE_TYPE__ size_t;
#elif ARCH_BITS == 32
typedef uint32_t size_t;
#elif ARCH_BITS == 64
typedef uint64_t size_t;
#else
# error "ARCH_BITS is undefined or incorrect."
#endif
#define _SIZE_T_DECLARED

#ifndef __cplusplus
# ifdef __WCHAR_TYPE__
typedef __WCHAR_TYPE__ wchar_t;
# elif defined(RT_OS_OS2) || defined(RT_OS_WINDOWS)
typedef uint16_t wchar_t;
# else
typedef int wchar_t;
# endif
# define _WCHAR_T_DECLARED
#endif

#ifdef __WINT_TYPE__
typedef __WINT_TYPE__ wint_t;
#else
typedef unsigned int wint_t;
#endif
#define _WINT_T_DECLARED

#ifndef NULL
# ifdef __cplusplus
#  define NULL  0
# else
#  define NULL  ((void *)0)
# endif
#endif


#ifndef offsetof
# if defined(__cplusplus) && defined(__offsetof__)
#  define offsetof(type, memb)
    (__offsetof__ (reinterpret_cast<size_t>(&reinterpret_cast<const volatile char &>(static_cast<type *>(0)->memb))) )
# else
#  define offsetof(type, memb) ((size_t)&((type *)0)->memb)
# endif
#endif


/* sys/types.h */
#ifdef __SSIZE_TYPE__
typedef __SSIZE_TYPE__ ssize_t;
#elif ARCH_BITS == 32
typedef int32_t ssize_t;
#elif ARCH_BITS == 64
typedef int64_t ssize_t;
#else
# define ARCH_BITS 123123
# error "ARCH_BITS is undefined or incorrect."
#endif
#define _SSIZE_T_DECLARED


/* stdarg.h */
typedef __builtin_va_list   va_list;
#if __GNUC__       == 3 \
 && __GNUC_MINOR__ == 2
# define va_start(va, arg)  __builtin_stdarg_start(va, arg)
#else
# define va_start(va, arg)  __builtin_va_start(va, arg)
#endif
#define va_end(va)          __builtin_va_end(va)
#define va_arg(va, type)    __builtin_va_arg(va, type)
#define va_copy(dst, src)   __builtin_va_copy(dst, src)


#endif /* !IPRT_INCLUDED_nocrt_compiler_gcc_h */
