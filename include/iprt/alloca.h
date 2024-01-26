/** @file
 * IPRT - alloca().
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

#ifndef IPRT_INCLUDED_alloca_h
#define IPRT_INCLUDED_alloca_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#if defined(IN_RC) || defined(IN_RING0_AGNOSTIC)
# error "No alloca() in raw-mode and agnostic ring-0 context as it may have external dependencies like libgcc."
#endif

/*
 * If there are more difficult platforms out there, we'll do OS
 * specific #ifdefs. But for now we'll just include the headers
 * which normally contains the alloca() prototype.
 * When we're in kernel territory it starts getting a bit more
 * interesting of course...
 */
#if defined(IN_RING0) \
 && (   defined(RT_OS_DARWIN) \
     || defined(RT_OS_FREEBSD) \
     || defined(RT_OS_LINUX) \
     || defined(RT_OS_NETBSD) \
     || defined(RT_OS_SOLARIS))
/* ASSUMES GNU C */
# define alloca(cb) __builtin_alloca(cb)

#elif defined(IPRT_NO_CRT) && defined(RT_OS_WINDOWS)
# include <iprt/types.h>

RT_C_DECLS_BEGIN
# ifdef RT_ARCH_X86
void * __cdecl _alloca(size_t);
# else
void          *_alloca(size_t);
# endif
# define alloca _alloca
RT_C_DECLS_END

#else
# include <stdlib.h>
# if !defined(RT_OS_DARWIN) && !defined(RT_OS_FREEBSD) && !defined(RT_OS_NETBSD)
#  include <malloc.h>
# endif
# if defined(RT_OS_SOLARIS) || defined(RT_OS_LINUX)
#  include <alloca.h>
# endif

# if defined(RT_OS_SOLARIS) && (defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)) \
  && defined(_SYS_REGSET_H) && !defined(IPRT_NO_SOLARIS_UCONTEXT_CLEANUPS)
/* Solaris' sys/regset.h pollutes the namespace with register constants that
   frequently conflicts with structure members and variable/parameter names.  */
#  undef CS
#  undef DS
#  undef EAX
#  undef EBP
#  undef EBX
#  undef ECX
#  undef EDI
#  undef EDX
#  undef EFL
#  undef EIP
#  undef ERR
#  undef ES
#  undef ESI
#  undef ESP
#  undef FS
#  undef GS
#  undef SS
#  undef TRAPNO
#  undef UESP
# endif
#endif

#endif /* !IPRT_INCLUDED_alloca_h */

