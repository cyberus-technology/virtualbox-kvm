/** @file
 * IPRT / No-CRT - Minimal unistd.h header.
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

#ifndef IPRT_INCLUDED_nocrt_unistd_h
#define IPRT_INCLUDED_nocrt_unistd_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/nocrt/sys/types.h>
#ifdef IPRT_NO_CRT_FOR_3RD_PARTY
# include <iprt/nocrt/time.h>       /* file.h includes fs.h which includes time.h */
# include <iprt/file.h>             /* for RTFILE_SEEK_XXX */
# include <iprt/assertcompile.h>
#endif


#ifdef IPRT_NO_CRT_FOR_3RD_PARTY

/* Flags for access: */
# define F_OK       0
# define X_OK       1
# define W_OK       2
# define R_OK       4

/* These are also in stdio.h: */
# undef  SEEK_SET
# define SEEK_SET   RTFILE_SEEK_BEGIN
# undef  SEEK_CUR
# define SEEK_CUR   RTFILE_SEEK_CURRENT
# undef  SEEK_END
# define SEEK_END   RTFILE_SEEK_END
AssertCompile(SEEK_SET == 0); AssertCompile(SEEK_CUR == 1); AssertCompile(SEEK_END == 2); /* Also in WDK header mmiscapi.h. */

RT_C_DECLS_BEGIN

int             RT_NOCRT(access)(const char *, int) RT_NOEXCEPT;
int             RT_NOCRT(dup)(int) RT_NOEXCEPT;
int             RT_NOCRT(dup2)(int, int) RT_NOEXCEPT;
ssize_t         RT_NOCRT(read)(int, void *, size_t) RT_NOEXCEPT;
ssize_t         RT_NOCRT(write)(int, const void *, size_t) RT_NOEXCEPT;
int             RT_NOCRT(close)(int) RT_NOEXCEPT;
int             RT_NOCRT(isatty)(int) RT_NOEXCEPT;
char           *RT_NOCRT(getcwd)(char *, size_t) RT_NOEXCEPT;
RTPROCESS       RT_NOCRT(getpid)(void) RT_NOEXCEPT;
RTPROCESS       RT_NOCRT(getppid)(void) RT_NOEXCEPT;
int             RT_NOCRT(unlink)(const char *) RT_NOEXCEPT;

int             RT_NOCRT(_access)(const char *, int) RT_NOEXCEPT;
int             RT_NOCRT(_dup)(int) RT_NOEXCEPT;
int             RT_NOCRT(_dup2)(int, int) RT_NOEXCEPT;
ssize_t         RT_NOCRT(_read)(int, void *, size_t) RT_NOEXCEPT;
ssize_t         RT_NOCRT(_write)(int, const void *, size_t) RT_NOEXCEPT;
int             RT_NOCRT(_close)(int) RT_NOEXCEPT;
int             RT_NOCRT(_isatty)(int) RT_NOEXCEPT;
char           *RT_NOCRT(_getcwd)(char *, size_t) RT_NOEXCEPT;
RTPROCESS       RT_NOCRT(_getpid)(void) RT_NOEXCEPT;
RTPROCESS       RT_NOCRT(_getppid)(void) RT_NOEXCEPT;
int             RT_NOCRT(_unlink)(const char *) RT_NOEXCEPT;

# if !defined(RT_WITHOUT_NOCRT_WRAPPERS) && !defined(RT_WITHOUT_NOCRT_WRAPPER_ALIASES)
#  define access        RT_NOCRT(access)
#  define dup           RT_NOCRT(dup)
#  define dup2          RT_NOCRT(dup2)
#  define read          RT_NOCRT(read)
#  define write         RT_NOCRT(write)
#  define close         RT_NOCRT(close)
#  define isatty        RT_NOCRT(isatty)
#  define getcwd        RT_NOCRT(getcwd)
#  define getpid        RT_NOCRT(getpid)
#  define getppid       RT_NOCRT(getppid)
#  define unlink        RT_NOCRT(unlink)

#  define _access       RT_NOCRT(access)
#  define _dup          RT_NOCRT(dup)
#  define _dup2         RT_NOCRT(dup2)
#  define _read         RT_NOCRT(read)
#  define _write        RT_NOCRT(write)
#  define _close        RT_NOCRT(close)
#  define _isatty       RT_NOCRT(isatty)
#  define _getcwd       RT_NOCRT(getcwd)
#  define _getpid       RT_NOCRT(getpid)
#  define _getppid      RT_NOCRT(getppid)
#  define _unlink       RT_NOCRT(unlink)
# endif

RT_C_DECLS_END

#endif /* IPRT_NO_CRT_FOR_3RD_PARTY */


#endif /* !IPRT_INCLUDED_nocrt_unistd_h */

