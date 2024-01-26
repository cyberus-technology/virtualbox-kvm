/** @file
 * IPRT - Linux symver and compatibility definitions.
 */

/*
 * Copyright (C) 2008-2023 Oracle and/or its affiliates.
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

/* Various tricks to produce binaries which can be run on old Linux
 * distributions.  This will almost certainly need updating as time
 * goes by. */

#ifndef IPRT_INCLUDED_linux_symvers_h
#define IPRT_INCLUDED_linux_symvers_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Please use -fno-stack-protector on the command line to avoid stack check
 * functions which are not available in EL3 for 32-bit builds. */

/* Use versions of glibc symbols which are available in 32-bit EL3 or
 * 64-bit EL4.  Currently only those symbols needed by the Additions,
 * though this could probably be extended to work for host builds too. */
#if defined(RT_ARCH_AMD64)
__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");
__asm__(".symver posix_spawn,posix_spawn@GLIBC_2.2.5");
#else  /* RT_ARCH_X86 */
__asm__(".symver posix_spawn,posix_spawn@GLIBC_2.2");
#endif

/* Do not use *_chk functions */
#undef _FORTIFY_SOURCE

/* Do not use __isoc99_* functions */
#undef __USE_GNU
#define __USE_GNU 1

/* And EL5 wants this too with __USE_GNU */
#undef _GNU_SOURCE
#define _GNU_SOURCE 1

/* Tell IPRT not to use newer functions */
#include <features.h>
#undef __GLIBC_MINOR__
#define __GLIBC_MINOR__ 3

/* Do not use fcntl64 */
#include <fcntl.h>
#ifdef fnctl
# undef fcntl
#endif
#if defined(RT_ARCH_AMD64)
__asm__(".symver fcntl64,fcntl@GLIBC_2.2.5");
#else
__asm__(".symver fcntl64,fcntl@GLIBC_2.0");
#endif

/* Do not use ISO C99 scanf which has a glibc 2.7 dependency. */
#undef __GLIBC_USE_DEPRECATED_SCANF
#define __GLIBC_USE_DEPRECATED_SCANF 1
#endif /* !IPRT_INCLUDED_linux_symvers_h */
