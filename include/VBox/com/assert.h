/** @file
 * MS COM / XPCOM Abstraction Layer - Assertion macros for COM/XPCOM.
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

#ifndef VBOX_INCLUDED_com_assert_h
#define VBOX_INCLUDED_com_assert_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/assert.h>

/** @defgroup grp_com_assert    Assertion Macros for COM/XPCOM
 * @ingroup grp_com
 * @{
 */


/**
 *  Asserts that the COM result code is succeeded in strict builds.
 *  In non-strict builds the result code will be NOREF'ed to kill compiler warnings.
 *
 *  @param hrc      The COM result code
 */
#define AssertComRC(hrc) \
    do { AssertMsg(SUCCEEDED(hrc), ("COM RC = %Rhrc (0x%08X)\n", hrc, hrc)); NOREF(hrc); } while (0)

/**
 *  Same as AssertComRC, except the caller already knows we failed.
 *
 *  @param hrc      The COM result code
 */
#define AssertComRCFailed(hrc) \
    do { AssertMsgFailed(("COM RC = %Rhrc (0x%08X)\n", hrc, hrc)); NOREF(hrc); } while (0)

/**
 *  A special version of AssertComRC that returns the given expression
 *  if the result code is failed.
 *
 *  @param hrc      The COM result code
 *  @param RetExpr  The expression to return
 */
#define AssertComRCReturn(hrc, RetExpr) \
    AssertMsgReturn(SUCCEEDED(hrc), ("COM RC = %Rhrc (0x%08X)\n", hrc, hrc), RetExpr)

/**
 *  A special version of AssertComRC that returns the given result code
 *  if it is failed.
 *
 *  @param hrc      The COM result code
 */
#define AssertComRCReturnRC(hrc) \
    AssertMsgReturn(SUCCEEDED(hrc), ("COM RC = %Rhrc (0x%08X)\n", hrc, hrc), hrc)

/**
 *  A special version of AssertComRC that returns if the result code is failed.
 *
 *  @param hrc      The COM result code
 */
#define AssertComRCReturnVoid(hrc) \
    AssertMsgReturnVoid(SUCCEEDED(hrc), ("COM RC = %Rhrc (0x%08X)\n", hrc, hrc))

/**
 *  A special version of AssertComRC that evaluates the given expression and
 *  breaks if the result code is failed.
 *
 *  @param hrc          The COM result code
 *  @param PreBreakExpr The expression to evaluate on failure.
 */
#define AssertComRCBreak(hrc, PreBreakExpr) \
    if (!SUCCEEDED(hrc)) { AssertComRCFailed(hrc); PreBreakExpr; break; } else do {} while (0)

/**
 *  A special version of AssertComRC that evaluates the given expression and
 *  throws it if the result code is failed.
 *
 *  @param hrc          The COM result code
 *  @param ThrowMeExpr  The expression which result to be thrown on failure.
 */
#define AssertComRCThrow(hrc, ThrowMeExpr) \
    do { if (SUCCEEDED(hrc)) { /*likely*/} else { AssertComRCFailed(hrc); throw (ThrowMeExpr); } } while (0)

/**
 *  A special version of AssertComRC that just breaks if the result code is
 *  failed.
 *
 *  @param hrc      The COM result code
 */
#define AssertComRCBreakRC(hrc) \
    if (!SUCCEEDED(hrc)) { AssertComRCFailed(hrc); break; } else do {} while (0)

/**
 *  A special version of AssertComRC that just throws @a hrc if the result code
 *  is failed.
 *
 *  @param hrc      The COM result code
 */
#define AssertComRCThrowRC(hrc) \
    do { if (SUCCEEDED(hrc)) { /*likely*/ } else { AssertComRCFailed(hrc); throw hrc; } } while (0)

/** @} */

#endif /* !VBOX_INCLUDED_com_assert_h */

