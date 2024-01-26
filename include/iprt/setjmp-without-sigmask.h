/** @file
 * IPRT - setjmp/long without signal mask saving and restoring.
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

#ifndef IPRT_INCLUDED_setjmp_without_sigmask_h
#define IPRT_INCLUDED_setjmp_without_sigmask_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


#include <iprt/cdefs.h>
#include <setjmp.h>

/*
 * System V and ANSI-C setups does not by default map setjmp/longjmp to the
 * signal mask saving/restoring variants (Linux included).  This is mainly
 * an issue on BSD derivatives.
 */
#if defined(IN_RING3) \
 && (   defined(RT_OS_DARWIN) \
     || defined(RT_OS_DRAGONFLY) \
     || defined(RT_OS_FREEBSD) \
     || defined(RT_OS_NETBSD) \
     || defined(RT_OS_OPENBSD) )
# undef setjmp /* /Library/Developer/CommandLineTools/usr/bin/../include/c++/v1/setjmp.h defines it on macOS. */
# define setjmp  _setjmp
# define longjmp _longjmp
#endif


#endif /* !IPRT_INCLUDED_setjmp_without_sigmask_h */

