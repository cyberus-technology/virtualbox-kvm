/** @file
 * Safe way to include winsock2.h.
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

#ifndef IPRT_INCLUDED_win_winsock2_h
#define IPRT_INCLUDED_win_winsock2_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* winsock2.h includes windows.h, but without winsock.h, so we need to do it
   up front using our wrapper header to avoid repeating tricks here. */
#define _WINSOCKAPI_ /* do not include winsock.h via windows.h */
#include <iprt/win/windows.h>

#ifdef _MSC_VER
/*
 * Unfortunately, the Windows.h file in SDK 7.1 is not clean wrt warning C4668:
 *      wincrypt.h(1848) : warning C4668: 'NTDDI_WINLH' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
 */
# pragma warning(push)
# pragma warning(disable:4668)
# ifndef __cplusplus
#  pragma warning(disable:4255) /* warning C4255: 'FARPROC' : no function prototype given: converting '()' to '(void)' */
# endif
# if _MSC_VER >= 1800 /*RT_MSC_VER_VC120*/
#  pragma warning(disable:4005) /* sdk/v7.1/include/sal_supp.h(57) : warning C4005: '__useHeader' : macro redefinition */
# endif
# if _MSC_VER >= 1900 /*RT_MSC_VER_VC140*/
#  ifdef __cplusplus
#   pragma warning(disable:5039) /* winbase.h(13179): warning C5039: 'TpSetCallbackCleanupGroup': pointer or reference to potentially throwing function passed to 'extern "C"' function under -EHc. Undefined behavior may occur if this function throws an exception. */
#  endif
# endif
#endif

#include <winsock2.h>

#ifdef _MSC_VER
# pragma warning(pop)
#endif

#endif /* !IPRT_INCLUDED_win_winsock2_h */

