/** @file
 * Safe way to include Windows.h.
 */

/*
 * Copyright (C) 2016-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_win_windows_h
#define IPRT_INCLUDED_win_windows_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* winioctl.h in windows 10 SDKs up to 22000(?) has a warning(push/pop) bug in the
   portion taken from ntddscm.h causing trouble when using _WIN32_WINNT or NTDDI_VERSION
   older than NTDDI_WIN10_RS5.  In 18362 winioctl.h also tests against _WIN32_WINNT_WIN10_TH2
   and other sdkddkver.h defines which only exist in NTDDI variants, not in _WIN32_WINNT_XXX,
   so we fake up those too to keep the precompiler warning free.

   Work around this by blocking out the buggy section on winioctl.h for now if the
   NTDDI_VERSION target is too small.

   WDK_NTDDI_VERSION is not present in the W7 SDK, not sure when exactly it was added.
   NTDDI_WIN10_RS5 is W10 1809. NTDDI_WIN10_CO is Windows 11? */
#include <sdkddkver.h>
#ifdef _WIN32_WINNT_WIN10
# ifndef _WIN32_WINNT_WIN10_TH2
#  define _WIN32_WINNT_WIN10_TH2 _WIN32_WINNT_WIN10
# endif
# ifndef _WIN32_WINNT_WIN10_RS1
#  define _WIN32_WINNT_WIN10_RS1 _WIN32_WINNT_WIN10
# endif
# ifndef _WIN32_WINNT_WIN10_RS2
#  define _WIN32_WINNT_WIN10_RS2 _WIN32_WINNT_WIN10
# endif
# ifndef _WIN32_WINNT_WIN10_RS3
#  define _WIN32_WINNT_WIN10_RS3 _WIN32_WINNT_WIN10
# endif
# ifndef _WIN32_WINNT_WIN10_RS4
#  define _WIN32_WINNT_WIN10_RS4 _WIN32_WINNT_WIN10
# endif
# ifndef _WIN32_WINNT_WIN10_RS5
#  define _WIN32_WINNT_WIN10_RS5 _WIN32_WINNT_WIN10
# endif
#endif
#if defined(NTDDI_WIN10_RS5) && !defined(NTDDI_WIN10_CO) && defined(WDK_NTDDI_VERSION)
# if NTDDI_VERSION < NTDDI_WIN10_RS5
#  define _NTDDSCM_H_ buggy, hope nobody needs it.
# endif
#endif

#ifdef _MSC_VER
/*
 * Unfortunately, the Windows.h file in SDK 7.1 is not clean wrt warning C4668:
 *      wincrypt.h(1848) : warning C4668: 'NTDDI_WINLH' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
 */
# pragma warning(push)
# pragma warning(disable:4668)
# pragma warning(disable:4480) /* W10/wincrypt.h(9193) : warning C4480: nonstandard extension used: specifying underlying type for enum 'CertKeyType' */
# if _MSC_VER >= 1800 /*RT_MSC_VER_VC120*/
#  pragma warning(disable:4005) /* sdk/v7.1/include/sal_supp.h(57) : warning C4005: '__useHeader' : macro redefinition */
# endif
# ifdef __cplusplus
#  if _MSC_VER >= 1900 /*RT_MSC_VER_VC140*/
#   pragma warning(disable:5039) /* winbase.h(13179): warning C5039: 'TpSetCallbackCleanupGroup': pointer or reference to potentially throwing function passed to 'extern "C"' function under -EHc. Undefined behavior may occur if this function throws an exception. */
#  endif
# else
#  pragma warning(disable:4255) /* warning C4255: 'FARPROC' : no function prototype given: converting '()' to '(void)' */
# endif
#endif

#include <Windows.h>

#ifdef _MSC_VER
# pragma warning(pop)
/* VS2010: Something causes this to be re-enabled above and triggering errors using RT_FLEXIBLE_ARRAY. */
# pragma warning(disable:4200)
#endif

#endif /* !IPRT_INCLUDED_win_windows_h */

