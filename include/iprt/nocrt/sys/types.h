/** @file
 * IPRT / No-CRT - Our own sys/types header.
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

#ifndef IPRT_INCLUDED_nocrt_sys_types_h
#define IPRT_INCLUDED_nocrt_sys_types_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#if defined(IPRT_INCLUDED_types_h) && !defined(IPRT_COMPLETED_types_h)
# error "Can't include nocrt/sys/types.h from iprt/types.h"
#endif

#include <iprt/types.h>

/* #if !defined(MSC-define) && !defined(GNU/LINUX-define) */
#if !defined(_DEV_T_DEFINED) && !defined(__dev_t_defined)
typedef RTDEV       dev_t;
#endif
#if !defined(_UCRT_RESTORE_CLANG_WARNINGS) /* MSC specific type */
typedef int         errno_t;
#endif
#if !defined(_INO_T_DEFINED) && !defined(__ino_t_defined)
typedef RTINODE     ino_t;
#endif
#if !defined(_OFF_T_DEFINED) && !defined(__off_t_defined)
typedef RTFOFF      off_t;
#endif
#if !defined(_PID_T_DEFINED) && !defined(__pid_t_defined)
typedef RTPROCESS   pid_t;
#endif

#endif /* !IPRT_INCLUDED_nocrt_sys_types_h */

