/** @file
 * IPRT / No-CRT - Our own assert.h header.
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

#ifndef IPRT_INCLUDED_nocrt_assert_h
#define IPRT_INCLUDED_nocrt_assert_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/assert.h>

DECL_FORCE_INLINE(void) rtCrtAssertPanic(void)
{
    RTAssertPanic();
}

/* Mesa uses assert() in such a way that we must not have any 'do {} while'
   wrappers in the expansion, so we partially cook our own assert here but
   using the standard iprt/assert.h building blocks. */
#define assert(a_Expr) (RT_LIKELY(!!(a_Expr)) ? (void)0 \
                        : RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__), \
                          rtCrtAssertPanic(), (void)0 )

#endif /* !IPRT_INCLUDED_nocrt_assert_h */

