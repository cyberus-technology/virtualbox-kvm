/** @file
 * IPRT / No-CRT - Our own setjmp header.
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

#ifndef IPRT_INCLUDED_nocrt_setjmp_h
#define IPRT_INCLUDED_nocrt_setjmp_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

RT_C_DECLS_BEGIN

#ifdef RT_ARCH_AMD64
# ifdef RT_OS_WINDOWS /* Also saves rsi, rdi and xmm6-xmm15. */
typedef uint64_t RT_NOCRT(jmp_buf)[10 + (2*10)];
# else
typedef uint64_t RT_NOCRT(jmp_buf)[8];
# endif
#else
typedef uint32_t RT_NOCRT(jmp_buf)[6+2];
#endif

extern DECL_RETURNS_TWICE(int) RT_NOCRT(setjmp)(RT_NOCRT(jmp_buf));
extern DECL_NO_RETURN(int)     RT_NOCRT(longjmp)(RT_NOCRT(jmp_buf), int);

#if !defined(RT_WITHOUT_NOCRT_WRAPPERS) && !defined(RT_WITHOUT_NOCRT_WRAPPER_ALIASES)
# define jmp_buf RT_NOCRT(jmp_buf)
# define setjmp RT_NOCRT(setjmp)
# define longjmp RT_NOCRT(longjmp)
#endif

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_nocrt_setjmp_h */

