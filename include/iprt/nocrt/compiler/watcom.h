/** @file
 * IPRT / No-CRT - Open Watcom specifics.
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

#ifndef IPRT_INCLUDED_nocrt_compiler_watcom_h
#define IPRT_INCLUDED_nocrt_compiler_watcom_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>

/* stddef.h for size_t and such */
#if 0
# include <../h/stddef.h>

# ifndef  _SSIZE_T_DEFINED_
#  define _SSIZE_T_DEFINED_
typedef signed int      ssize_t;
# endif

#else

# define _SIZE_T_DEFINED_
# define __size_t
typedef unsigned        size_t;
typedef size_t       _w_size_t;

# define _SSIZE_T_DEFINED_
# define __ssize_t
typedef signed int      ssize_t;

# define _RSIZE_T_DEFINED
typedef size_t          rsize_t;

# define _PTRDIFF_T_DEFINED_
# ifdef __HUGE__
typedef long            ptrdiff_t;
# else
typedef int             ptrdiff_t;
# endif

# ifndef  _WCHAR_T_DEFINED /* predefined in C++ mode? */
#  define _WCHAR_T_DEFINED
typedef unsigned short  wchar_t;
# endif

# ifndef NULL
#  ifndef __cplusplus
#   define NULL         ((void *)0)
#  elif  defined(__SMALL__) || defined(__MEDIUM__) || !defined(_M_I86)
#   define NULL         (0)
#  else
#   define NULL          (0L)
#  endif
# endif

# define offsetof(a_Type, a_Member) RT_OFFSETOF(a_Type, a_Member)

# if defined(_M_I86) && (defined(__SMALL__) || defined(__MEDIUM__))
typedef int             intptr_t;
typedef unsigned int    uintptr_t;
# elif defined(_M_I86) || (!defined(__COMPACT__) && !defined(__LARGE__))
typedef long            intptr_t;
typedef unsigned long   uintptr_t;
# else /* 32-bit compile using far data pointers (16:32) */
typedef long long       intptr_t;
typedef unsigned long long uintptr_t;
# endif

#endif

/* stdarg.h */
#include <../h/stdarg.h>

#endif /* !IPRT_INCLUDED_nocrt_compiler_watcom_h */

