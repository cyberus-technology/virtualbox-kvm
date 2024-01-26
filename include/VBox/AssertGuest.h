/** @file
 * VirtualBox - Guest input assertions.
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

#ifndef VBOX_INCLUDED_AssertGuest_h
#define VBOX_INCLUDED_AssertGuest_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/cdefs.h>
#include <iprt/assert.h>


/** @defgroup grp_vbox_assert_guest VBox Guest Input Assertion Macros
 * @{
 */


/** @name Guest input assertions
 *
 * These assertions will only trigger when VBOX_STRICT_GUEST is defined.  When
 * it is undefined they will all be no-ops and generate no code, unless they
 * have other side effected (i.e. the _RETURN, _STMT, _BREAK variations).
 *
 * The assertions build on top of the functions in iprt/assert.h.
 *
 * @{
 */


/** @def ASSERT_GUEST_PANIC()
 * If VBOX_STRICT_GUEST is defined this macro will invoke RTAssertDoPanic if
 * RTAssertShouldPanic returns true. If VBOX_STRICT_GUEST isn't defined it won't
 * do any thing.
 */
#if defined(VBOX_STRICT_GUEST) && !defined(VBOX_STRICT_GUEST_DONT_PANIC)
# define ASSERT_GUEST_PANIC()   do { if (RTAssertShouldPanic()) RTAssertDoPanic(); } while (0)
#else
# define ASSERT_GUEST_PANIC()   do { } while (0)
#endif

/** Wrapper around RTAssertMsg1Weak that prefixes the expression. */
#define ASSERT_GUEST_MSG1(szExpr, iLine, pszFile, pszFunction) \
    RTAssertMsg1Weak("guest-input: " szExpr, iLine, pszFile, pszFunction)


/** @def ASSERT_GUEST
 * Assert that an expression is true. If false, hit breakpoint.
 * @param   a_Expr  Expression which should be true.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST(a_Expr)  \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            ASSERT_GUEST_MSG1(#a_Expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            ASSERT_GUEST_PANIC(); \
        } \
    } while (0)
#else
# define ASSERT_GUEST(a_Expr) do { } while (0)
#endif


/** @def ASSERT_GUEST_STMT
 * Assert that an expression is true. If false, hit breakpoint and execute the
 * statement.
 * @param   a_Expr  Expression which should be true.
 * @param   a_Stmt  Statement to execute on failure.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_STMT(a_Expr, a_Stmt)  \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            ASSERT_GUEST_MSG1(#a_Expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            ASSERT_GUEST_PANIC(); \
            a_Stmt; \
        } \
    } while (0)
#else
# define ASSERT_GUEST_STMT(a_Expr, a_Stmt)  \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            a_Stmt; \
        } \
    } while (0)
#endif


/** @def ASSERT_GUEST_RETURN
 * Assert that an expression is true and returns if it isn't.
 * In VBOX_STRICT_GUEST mode it will hit a breakpoint before returning.
 *
 * @param   a_Expr  Expression which should be true.
 * @param   a_rc    What is to be presented to return.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_RETURN(a_Expr, a_rc) \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            ASSERT_GUEST_MSG1(#a_Expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            ASSERT_GUEST_PANIC(); \
            return (a_rc); \
        } \
    } while (0)
#else
# define ASSERT_GUEST_RETURN(a_Expr, a_rc) \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
            return (a_rc); \
    } while (0)
#endif

/** @def ASSERT_GUEST_STMT_RETURN
 * Assert that an expression is true, if it isn't execute the given statement
 * and return rc.
 *
 * In VBOX_STRICT_GUEST mode it will hit a breakpoint before executing the statement and
 * returning.
 *
 * @param   a_Expr  Expression which should be true.
 * @param   a_Stmt  Statement to execute before returning on failure.
 * @param   a_rc    What is to be presented to return.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_STMT_RETURN(a_Expr, a_Stmt, a_rc) \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            ASSERT_GUEST_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
            ASSERT_GUEST_PANIC(); \
            a_Stmt; \
            return (a_rc); \
        } \
    } while (0)
#else
# define ASSERT_GUEST_STMT_RETURN(a_Expr, a_Stmt, a_rc) \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            a_Stmt; \
            return (a_rc); \
        } \
    } while (0)
#endif

/** @def ASSERT_GUEST_RETURN_VOID
 * Assert that an expression is true and returns if it isn't.
 * In VBOX_STRICT_GUEST mode it will hit a breakpoint before returning.
 *
 * @param   a_Expr  Expression which should be true.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_RETURN_VOID(a_Expr) \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            ASSERT_GUEST_MSG1(#a_Expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            ASSERT_GUEST_PANIC(); \
            return; \
        } \
    } while (0)
#else
# define ASSERT_GUEST_RETURN_VOID(a_Expr) \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
            return; \
    } while (0)
#endif

/** @def ASSERT_GUEST_STMT_RETURN_VOID
 * Assert that an expression is true, if it isn't execute the given statement
 * and return.
 *
 * In VBOX_STRICT_GUEST mode it will hit a breakpoint before returning.
 *
 * @param   a_Expr  Expression which should be true.
 * @param   a_Stmt  Statement to execute before returning on failure.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_STMT_RETURN_VOID(a_Expr, a_Stmt) \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            ASSERT_GUEST_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
            ASSERT_GUEST_PANIC(); \
            a_Stmt; \
            return; \
        } \
    } while (0)
#else
# define ASSERT_GUEST_STMT_RETURN_VOID(a_Expr, a_Stmt) \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            a_Stmt; \
            return; \
        } \
    } while (0)
#endif


/** @def ASSERT_GUEST_BREAK
 * Assert that an expression is true and breaks if it isn't.
 * In VBOX_STRICT_GUEST mode it will hit a breakpoint before breaking.
 *
 * @param   a_Expr  Expression which should be true.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_BREAK(a_Expr) \
    if (RT_LIKELY(!!(a_Expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        ASSERT_GUEST_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_PANIC(); \
        break; \
    } else \
        break
#else
# define ASSERT_GUEST_BREAK(a_Expr) \
    if (RT_LIKELY(!!(a_Expr))) \
    { /* likely */ } \
    else \
        break
#endif

/** @def ASSERT_GUEST_CONTINUE
 * Assert that an expression is true and continue if it isn't.
 * In VBOX_STRICT_GUEST mode it will hit a breakpoint before continuing.
 *
 * @param   a_Expr  Expression which should be true.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_CONTINUE(a_Expr) \
    if (RT_LIKELY(!!(a_Expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        ASSERT_GUEST_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_PANIC(); \
        continue; \
    } else do {} while (0)
#else
# define ASSERT_GUEST_CONTINUE(a_Expr) \
    if (RT_LIKELY(!!(a_Expr))) \
    { /* likely */ } \
    else \
        continue
#endif

/** @def ASSERT_GUEST_STMT_BREAK
 * Assert that an expression is true and breaks if it isn't.
 * In VBOX_STRICT_GUEST mode it will hit a breakpoint before doing break.
 *
 * @param   a_Expr  Expression which should be true.
 * @param   a_Stmt  Statement to execute before break in case of a failed assertion.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_STMT_BREAK(a_Expr, a_Stmt) \
    if (RT_LIKELY(!!(a_Expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        ASSERT_GUEST_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_PANIC(); \
        a_Stmt; \
        break; \
    } else do {} while (0)
#else
# define ASSERT_GUEST_STMT_BREAK(a_Expr, a_Stmt) \
    if (RT_LIKELY(!!(a_Expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        a_Stmt; \
        break; \
    } else do {} while (0)
#endif


/** @def ASSERT_GUEST_MSG
 * Assert that an expression is true. If it's not print message and hit breakpoint.
 * @param   a_Expr  Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_MSG(a_Expr, a)  \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            ASSERT_GUEST_MSG1(#a_Expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertMsg2Weak a; \
            ASSERT_GUEST_PANIC(); \
        } \
    } while (0)
#else
# define ASSERT_GUEST_MSG(a_Expr, a)  do { } while (0)
#endif

/** @def ASSERT_GUEST_MSG_STMT
 * Assert that an expression is true.  If it's not print message and hit
 * breakpoint and execute the statement.
 *
 * @param   a_Expr  Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 * @param   a_Stmt  Statement to execute in case of a failed assertion.
 *
 * @remarks The expression and statement will be evaluated in all build types.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_MSG_STMT(a_Expr, a, a_Stmt)  \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            ASSERT_GUEST_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
            RTAssertMsg2Weak a; \
            ASSERT_GUEST_PANIC(); \
            a_Stmt; \
        } \
    } while (0)
#else
# define ASSERT_GUEST_MSG_STMT(a_Expr, a, a_Stmt)  \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            a_Stmt; \
        } \
    } while (0)
#endif

/** @def ASSERT_GUEST_MSG_RETURN
 * Assert that an expression is true and returns if it isn't.
 * In VBOX_STRICT_GUEST mode it will hit a breakpoint before returning.
 *
 * @param   a_Expr  Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 * @param   a_rc    What is to be presented to return.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_MSG_RETURN(a_Expr, a, a_rc)  \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            ASSERT_GUEST_MSG1(#a_Expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertMsg2Weak a; \
            ASSERT_GUEST_PANIC(); \
            return (a_rc); \
        } \
    } while (0)
#else
# define ASSERT_GUEST_MSG_RETURN(a_Expr, a, a_rc) \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
            return (a_rc); \
    } while (0)
#endif

/** @def ASSERT_GUEST_MSG_STMT_RETURN
 * Assert that an expression is true, if it isn't execute the statement and
 * return.
 *
 * In VBOX_STRICT_GUEST mode it will hit a breakpoint before returning.
 *
 * @param   a_Expr  Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 * @param   a_Stmt  Statement to execute before returning in case of a failed
 *                  assertion.
 * @param   a_rc    What is to be presented to return.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_MSG_STMT_RETURN(a_Expr, a, a_Stmt, a_rc)  \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            ASSERT_GUEST_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
            RTAssertMsg2Weak a; \
            ASSERT_GUEST_PANIC(); \
            a_Stmt; \
            return (a_rc); \
        } \
    } while (0)
#else
# define ASSERT_GUEST_MSG_STMT_RETURN(a_Expr, a, a_Stmt, a_rc) \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            a_Stmt; \
            return (a_rc); \
        } \
    } while (0)
#endif

/** @def ASSERT_GUEST_MSG_RETURN_VOID
 * Assert that an expression is true and returns if it isn't.
 * In VBOX_STRICT_GUEST mode it will hit a breakpoint before returning.
 *
 * @param   a_Expr  Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_MSG_RETURN_VOID(a_Expr, a)  \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            ASSERT_GUEST_MSG1(#a_Expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertMsg2Weak a; \
            ASSERT_GUEST_PANIC(); \
            return; \
        } \
    } while (0)
#else
# define ASSERT_GUEST_MSG_RETURN_VOID(a_Expr, a) \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
            return; \
    } while (0)
#endif

/** @def ASSERT_GUEST_MSG_STMT_RETURN_VOID
 * Assert that an expression is true, if it isn't execute the statement and
 * return.
 *
 * In VBOX_STRICT_GUEST mode it will hit a breakpoint before returning.
 *
 * @param   a_Expr  Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 * @param   a_Stmt  Statement to execute before return in case of a failed assertion.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_MSG_STMT_RETURN_VOID(a_Expr, a, a_Stmt)  \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            ASSERT_GUEST_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
            RTAssertMsg2Weak a; \
            ASSERT_GUEST_PANIC(); \
            a_Stmt; \
            return; \
        } \
    } while (0)
#else
# define ASSERT_GUEST_MSG_STMT_RETURN_VOID(a_Expr, a, a_Stmt) \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            a_Stmt; \
            return; \
        } \
    } while (0)
#endif


/** @def ASSERT_GUEST_MSG_BREAK
 * Assert that an expression is true and breaks if it isn't.
 * In VBOX_STRICT_GUEST mode it will hit a breakpoint before returning.
 *
 * @param   a_Expr  Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_MSG_BREAK(a_Expr, a) \
    if (RT_LIKELY(!!(a_Expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        ASSERT_GUEST_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        ASSERT_GUEST_PANIC(); \
        break; \
    } else \
        break
#else
# define ASSERT_GUEST_MSG_BREAK(a_Expr, a) \
    if (RT_LIKELY(!!(a_Expr))) \
    { /* likely */ } \
    else \
        break
#endif

/** @def ASSERT_GUEST_MSG_STMT_BREAK
 * Assert that an expression is true and breaks if it isn't.
 * In VBOX_STRICT_GUEST mode it will hit a breakpoint before doing break.
 *
 * @param   a_Expr  Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 * @param   a_Stmt  Statement to execute before break in case of a failed assertion.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_MSG_STMT_BREAK(a_Expr, a, a_Stmt) \
    if (RT_LIKELY(!!(a_Expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        ASSERT_GUEST_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        ASSERT_GUEST_PANIC(); \
        a_Stmt; \
        break; \
    } else \
        break
#else
# define ASSERT_GUEST_MSG_STMT_BREAK(a_Expr, a, a_Stmt) \
    if (RT_LIKELY(!!(a_Expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        a_Stmt; \
        break; \
    } else \
        break
#endif

/** @def ASSERT_GUEST_FAILED
 * An assertion failed, hit breakpoint.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_FAILED()  \
    do { \
        ASSERT_GUEST_MSG1("failed", __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        ASSERT_GUEST_PANIC(); \
    } while (0)
#else
# define ASSERT_GUEST_FAILED()         do { } while (0)
#endif

/** @def ASSERT_GUEST_FAILED_STMT
 * An assertion failed, hit breakpoint and execute statement.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_FAILED_STMT(a_Stmt) \
    do { \
        ASSERT_GUEST_MSG1("failed", __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        ASSERT_GUEST_PANIC(); \
        a_Stmt; \
    } while (0)
#else
# define ASSERT_GUEST_FAILED_STMT(a_Stmt)     do { a_Stmt; } while (0)
#endif

/** @def ASSERT_GUEST_FAILED_RETURN
 * An assertion failed, hit breakpoint (VBOX_STRICT_GUEST mode only) and return.
 *
 * @param   a_rc    The a_rc to return.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_FAILED_RETURN(a_rc)  \
    do { \
        ASSERT_GUEST_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_PANIC(); \
        return (a_rc); \
    } while (0)
#else
# define ASSERT_GUEST_FAILED_RETURN(a_rc)  \
    do { \
        return (a_rc); \
    } while (0)
#endif

/** @def ASSERT_GUEST_FAILED_STMT_RETURN
 * An assertion failed, hit breakpoint (VBOX_STRICT_GUEST mode only), execute a
 * statement and return a value.
 *
 * @param   a_Stmt  The statement to execute before returning.
 * @param   a_rc    The value to return.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_FAILED_STMT_RETURN(a_Stmt, a_rc)  \
    do { \
        ASSERT_GUEST_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_PANIC(); \
        a_Stmt; \
        return (a_rc); \
    } while (0)
#else
# define ASSERT_GUEST_FAILED_STMT_RETURN(a_Stmt, a_rc)  \
    do { \
        a_Stmt; \
        return (a_rc); \
    } while (0)
#endif

/** @def ASSERT_GUEST_FAILED_RETURN_VOID
 * An assertion failed, hit breakpoint (VBOX_STRICT_GUEST mode only) and return.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_FAILED_RETURN_VOID()  \
    do { \
        ASSERT_GUEST_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_PANIC(); \
        return; \
    } while (0)
#else
# define ASSERT_GUEST_FAILED_RETURN_VOID()  \
    do { \
        return; \
    } while (0)
#endif

/** @def ASSERT_GUEST_FAILED_STMT_RETURN_VOID
 * An assertion failed, hit breakpoint (VBOX_STRICT_GUEST mode only), execute a
 * statement and return.
 *
 * @param a_Stmt The statement to execute before returning.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_FAILED_STMT_RETURN_VOID(a_Stmt)  \
    do { \
        ASSERT_GUEST_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_PANIC(); \
        a_Stmt; \
        return; \
    } while (0)
#else
# define ASSERT_GUEST_FAILED_STMT_RETURN_VOID(a_Stmt)  \
    do { \
        a_Stmt; \
        return; \
    } while (0)
#endif


/** @def ASSERT_GUEST_FAILED_BREAK
 * An assertion failed, hit breakpoint (VBOX_STRICT_GUEST mode only) and break.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_FAILED_BREAK()  \
    if (1) { \
        ASSERT_GUEST_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_PANIC(); \
        break; \
    } else \
        break
#else
# define ASSERT_GUEST_FAILED_BREAK()  \
    if (1) \
        break; \
    else \
        break
#endif

/** @def ASSERT_GUEST_FAILED_STMT_BREAK
 * An assertion failed, hit breakpoint (VBOX_STRICT_GUEST mode only), execute
 * the given statement and break.
 *
 * @param   a_Stmt  Statement to execute before break.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_FAILED_STMT_BREAK(a_Stmt) \
    if (1) { \
        ASSERT_GUEST_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_PANIC(); \
        a_Stmt; \
        break; \
    } else \
        break
#else
# define ASSERT_GUEST_FAILED_STMT_BREAK(a_Stmt) \
    if (1) { \
        a_Stmt; \
        break; \
    } else \
        break
#endif


/** @def ASSERT_GUEST_MSG_FAILED
 * An assertion failed print a message and a hit breakpoint.
 *
 * @param   a   printf argument list (in parenthesis).
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_MSG_FAILED(a)  \
    do { \
        ASSERT_GUEST_MSG1("failed", __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        ASSERT_GUEST_PANIC(); \
    } while (0)
#else
# define ASSERT_GUEST_MSG_FAILED(a)     do { } while (0)
#endif

/** @def ASSERT_GUEST_MSG_FAILED_RETURN
 * An assertion failed, hit breakpoint with message (VBOX_STRICT_GUEST mode only) and return.
 *
 * @param   a       printf argument list (in parenthesis).
 * @param   a_rc    What is to be presented to return.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_MSG_FAILED_RETURN(a, a_rc)  \
    do { \
        ASSERT_GUEST_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        ASSERT_GUEST_PANIC(); \
        return (a_rc); \
    } while (0)
#else
# define ASSERT_GUEST_MSG_FAILED_RETURN(a, a_rc)  \
    do { \
        return (a_rc); \
    } while (0)
#endif

/** @def ASSERT_GUEST_MSG_FAILED_RETURN_VOID
 * An assertion failed, hit breakpoint with message (VBOX_STRICT_GUEST mode only) and return.
 *
 * @param   a       printf argument list (in parenthesis).
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_MSG_FAILED_RETURN_VOID(a)  \
    do { \
        ASSERT_GUEST_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        ASSERT_GUEST_PANIC(); \
        return; \
    } while (0)
#else
# define ASSERT_GUEST_MSG_FAILED_RETURN_VOID(a)  \
    do { \
        return; \
    } while (0)
#endif


/** @def ASSERT_GUEST_MSG_FAILED_BREAK
 * An assertion failed, hit breakpoint with message (VBOX_STRICT_GUEST mode only) and break.
 *
 * @param   a       printf argument list (in parenthesis).
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_MSG_FAILED_BREAK(a)  \
    if (1) { \
        ASSERT_GUEST_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        ASSERT_GUEST_PANIC(); \
        break; \
    } else \
        break
#else
# define ASSERT_GUEST_MSG_FAILED_BREAK(a)  \
    if (1) \
        break; \
    else \
        break
#endif

/** @def ASSERT_GUEST_MSG_FAILED_STMT_BREAK
 * An assertion failed, hit breakpoint (VBOX_STRICT_GUEST mode only), execute
 * the given statement and break.
 *
 * @param   a       printf argument list (in parenthesis).
 * @param   a_Stmt  Statement to execute before break.
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_MSG_FAILED_STMT_BREAK(a, a_Stmt) \
    if (1) { \
        ASSERT_GUEST_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        ASSERT_GUEST_PANIC(); \
        a_Stmt; \
        break; \
    } else \
        break
#else
# define ASSERT_GUEST_MSG_FAILED_STMT_BREAK(a, a_Stmt) \
    if (1) { \
        a_Stmt; \
        break; \
    } else \
        break
#endif

/** @} */



/** @name Guest input release log assertions
 *
 * These assertions will work like normal strict assertion when VBOX_STRICT_GUEST is
 * defined and LogRel statements when VBOX_STRICT_GUEST is undefined.  Typically
 * used for important guest input that it would be helpful to find in VBox.log
 * if the guest doesn't get it right.
 *
 * @{
 */


/** @def ASSERT_GUEST_LOGREL_MSG1
 * RTAssertMsg1Weak (strict builds) / LogRel wrapper (non-strict).
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_LOGREL_MSG1(szExpr, iLine, pszFile, pszFunction) \
    RTAssertMsg1Weak("guest-input: " szExpr, iLine, pszFile, pszFunction)
#else
# define ASSERT_GUEST_LOGREL_MSG1(szExpr, iLine, pszFile, pszFunction) \
    LogRel(("ASSERT_GUEST_LOGREL %s(%d) %s: %s\n", (pszFile), (iLine), (pszFunction), (szExpr) ))
#endif

/** @def ASSERT_GUEST_LOGREL_MSG2
 * RTAssertMsg2Weak (strict builds) / LogRel wrapper (non-strict).
 */
#ifdef VBOX_STRICT_GUEST
# define ASSERT_GUEST_LOGREL_MSG2(a)  RTAssertMsg2Weak a
#else
# define ASSERT_GUEST_LOGREL_MSG2(a)  LogRel(a)
#endif

/** @def ASSERT_GUEST_LOGREL
 * Assert that an expression is true.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a_Expr  Expression which should be true.
 */
#define ASSERT_GUEST_LOGREL(a_Expr) \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            ASSERT_GUEST_LOGREL_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
            ASSERT_GUEST_PANIC(); \
        } \
    } while (0)

/** @def ASSERT_GUEST_LOGREL_RETURN
 * Assert that an expression is true, return \a a_rc if it isn't.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a_Expr  Expression which should be true.
 * @param   a_rc    What is to be presented to return.
 */
#define ASSERT_GUEST_LOGREL_RETURN(a_Expr, a_rc) \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            ASSERT_GUEST_LOGREL_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
            ASSERT_GUEST_PANIC(); \
            return (a_rc); \
        } \
    } while (0)

/** @def ASSERT_GUEST_LOGREL_RETURN_VOID
 * Assert that an expression is true, return void if it isn't.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a_Expr  Expression which should be true.
 */
#define ASSERT_GUEST_LOGREL_RETURN_VOID(a_Expr) \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else \
        { \
            ASSERT_GUEST_LOGREL_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
            ASSERT_GUEST_PANIC(); \
            return; \
        } \
    } while (0)

/** @def ASSERT_GUEST_LOGREL_BREAK
 * Assert that an expression is true, break if it isn't.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a_Expr  Expression which should be true.
 */
#define ASSERT_GUEST_LOGREL_BREAK(a_Expr) \
    if (RT_LIKELY(!!(a_Expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        ASSERT_GUEST_LOGREL_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_PANIC(); \
        break; \
    } \
    else \
        break

/** @def ASSERT_GUEST_LOGREL_STMT_BREAK
 * Assert that an expression is true, execute \a a_Stmt and break if it isn't.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a_Expr  Expression which should be true.
 * @param   a_Stmt  Statement to execute before break in case of a failed assertion.
 */
#define ASSERT_GUEST_LOGREL_STMT_BREAK(a_Expr, a_Stmt) \
    if (RT_LIKELY(!!(a_Expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        ASSERT_GUEST_LOGREL_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_PANIC(); \
        a_Stmt; \
        break; \
    } else \
        break

/** @def ASSERT_GUEST_LOGREL_MSG
 * Assert that an expression is true.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a_Expr  Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 */
#define ASSERT_GUEST_LOGREL_MSG(a_Expr, a) \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else\
        { \
            ASSERT_GUEST_LOGREL_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
            ASSERT_GUEST_LOGREL_MSG2(a); \
            ASSERT_GUEST_PANIC(); \
        } \
    } while (0)

/** @def ASSERT_GUEST_LOGREL_MSG_STMT
 * Assert that an expression is true, execute \a a_Stmt and break if it isn't
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a_Expr  Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 * @param   a_Stmt  Statement to execute in case of a failed assertion.
 */
#define ASSERT_GUEST_LOGREL_MSG_STMT(a_Expr, a, a_Stmt) \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else\
        { \
            ASSERT_GUEST_LOGREL_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
            ASSERT_GUEST_LOGREL_MSG2(a); \
            ASSERT_GUEST_PANIC(); \
            a_Stmt; \
        } \
    } while (0)

/** @def ASSERT_GUEST_LOGREL_MSG_RETURN
 * Assert that an expression is true, return \a a_rc if it isn't.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a_Expr  Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 * @param   a_rc    What is to be presented to return.
 */
#define ASSERT_GUEST_LOGREL_MSG_RETURN(a_Expr, a, a_rc) \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else\
        { \
            ASSERT_GUEST_LOGREL_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
            ASSERT_GUEST_LOGREL_MSG2(a); \
            ASSERT_GUEST_PANIC(); \
            return (a_rc); \
        } \
    } while (0)

/** @def ASSERT_GUEST_LOGREL_MSG_STMT_RETURN
 * Assert that an expression is true, execute @a a_Stmt and return @a rcRet if it
 * isn't.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a_Expr  Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 * @param   a_Stmt  Statement to execute before returning in case of a failed
 *                  assertion.
 * @param   rcRet   What is to be presented to return.
 */
#define ASSERT_GUEST_LOGREL_MSG_STMT_RETURN(a_Expr, a, a_Stmt, rcRet) \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else\
        { \
            ASSERT_GUEST_LOGREL_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
            ASSERT_GUEST_LOGREL_MSG2(a); \
            ASSERT_GUEST_PANIC(); \
            a_Stmt; \
            return (rcRet); \
        } \
    } while (0)

/** @def ASSERT_GUEST_LOGREL_MSG_RETURN_VOID
 * Assert that an expression is true, return (void) if it isn't.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a_Expr  Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 */
#define ASSERT_GUEST_LOGREL_MSG_RETURN_VOID(a_Expr, a) \
    do { \
        if (RT_LIKELY(!!(a_Expr))) \
        { /* likely */ } \
        else\
        { \
            ASSERT_GUEST_LOGREL_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
            ASSERT_GUEST_LOGREL_MSG2(a); \
            ASSERT_GUEST_PANIC(); \
            return; \
        } \
    } while (0)

/** @def ASSERT_GUEST_LOGREL_MSG_BREAK
 * Assert that an expression is true, break if it isn't.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a_Expr  Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 */
#define ASSERT_GUEST_LOGREL_MSG_BREAK(a_Expr, a) \
    if (RT_LIKELY(!!(a_Expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        ASSERT_GUEST_LOGREL_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_LOGREL_MSG2(a); \
        ASSERT_GUEST_PANIC(); \
        break; \
    } \
    else \
        break

/** @def ASSERT_GUEST_LOGREL_MSG_STMT_BREAK
 * Assert that an expression is true, execute \a a_Stmt and break if it isn't.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a_Expr  Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 * @param   a_Stmt  Statement to execute before break in case of a failed assertion.
 */
#define ASSERT_GUEST_LOGREL_MSG_STMT_BREAK(a_Expr, a, a_Stmt) \
    if (RT_LIKELY(!!(a_Expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        ASSERT_GUEST_LOGREL_MSG1(#a_Expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_LOGREL_MSG2(a); \
        ASSERT_GUEST_PANIC(); \
        a_Stmt; \
        break; \
    } else \
        break

/** @def ASSERT_GUEST_LOGREL_FAILED
 * An assertion failed.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 */
#define ASSERT_GUEST_LOGREL_FAILED() \
    do { \
        ASSERT_GUEST_LOGREL_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_PANIC(); \
    } while (0)

/** @def ASSERT_GUEST_LOGREL_FAILED_RETURN
 * An assertion failed.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a_rc    What is to be presented to return.
 */
#define ASSERT_GUEST_LOGREL_FAILED_RETURN(a_rc) \
    do { \
        ASSERT_GUEST_LOGREL_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_PANIC(); \
        return (a_rc); \
    } while (0)

/** @def ASSERT_GUEST_LOGREL_FAILED_RETURN_VOID
 * An assertion failed, hit a breakpoint and return.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 */
#define ASSERT_GUEST_LOGREL_FAILED_RETURN_VOID() \
    do { \
        ASSERT_GUEST_LOGREL_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_PANIC(); \
        return; \
    } while (0)

/** @def ASSERT_GUEST_LOGREL_FAILED_BREAK
 * An assertion failed, break.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 */
#define ASSERT_GUEST_LOGREL_FAILED_BREAK() \
    if (1) \
    { \
        ASSERT_GUEST_LOGREL_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_PANIC(); \
        break; \
    } else \
        break

/** @def ASSERT_GUEST_LOGREL_FAILED_STMT_BREAK
 * An assertion failed, execute \a a_Stmt and break.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a_Stmt  Statement to execute before break.
 */
#define ASSERT_GUEST_LOGREL_FAILED_STMT_BREAK(a_Stmt) \
    if (1) \
    { \
        ASSERT_GUEST_LOGREL_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_PANIC(); \
        a_Stmt; \
        break; \
    } else \
        break

/** @def ASSERT_GUEST_LOGREL_MSG_FAILED
 * An assertion failed.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a   printf argument list (in parenthesis).
 */
#define ASSERT_GUEST_LOGREL_MSG_FAILED(a) \
    do { \
        ASSERT_GUEST_LOGREL_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_LOGREL_MSG2(a); \
        ASSERT_GUEST_PANIC(); \
    } while (0)

/** @def ASSERT_GUEST_LOGREL_MSG_FAILED_STMT
 * An assertion failed, execute @a a_Stmt.
 *
 * Strict builds will hit a breakpoint, non-strict will only do LogRel. The
 * statement will be executed in regardless of build type.
 *
 * @param   a       printf argument list (in parenthesis).
 * @param   a_Stmt  Statement to execute after raising/logging the assertion.
 */
#define ASSERT_GUEST_LOGREL_MSG_FAILED_STMT(a, a_Stmt) \
    do { \
        ASSERT_GUEST_LOGREL_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_LOGREL_MSG2(a); \
        ASSERT_GUEST_PANIC(); \
        a_Stmt; \
    } while (0)

/** @def ASSERT_GUEST_LOGREL_MSG_FAILED_RETURN
 * An assertion failed, return \a a_rc.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a   printf argument list (in parenthesis).
 * @param   a_rc  What is to be presented to return.
 */
#define ASSERT_GUEST_LOGREL_MSG_FAILED_RETURN(a, a_rc) \
    do { \
        ASSERT_GUEST_LOGREL_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_LOGREL_MSG2(a); \
        ASSERT_GUEST_PANIC(); \
        return (a_rc); \
    } while (0)

/** @def ASSERT_GUEST_LOGREL_MSG_FAILED_STMT_RETURN
 * An assertion failed, execute @a a_Stmt and return @a a_rc.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a       printf argument list (in parenthesis).
 * @param   a_Stmt  Statement to execute before returning in case of a failed
 *                  assertion.
 * @param   a_rc    What is to be presented to return.
 */
#define ASSERT_GUEST_LOGREL_MSG_FAILED_STMT_RETURN(a, a_Stmt, a_rc) \
    do { \
        ASSERT_GUEST_LOGREL_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_LOGREL_MSG2(a); \
        ASSERT_GUEST_PANIC(); \
        a_Stmt; \
        return (a_rc); \
    } while (0)

/** @def ASSERT_GUEST_LOGREL_MSG_FAILED_RETURN_VOID
 * An assertion failed, return void.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a   printf argument list (in parenthesis).
 */
#define ASSERT_GUEST_LOGREL_MSG_FAILED_RETURN_VOID(a) \
    do { \
        ASSERT_GUEST_LOGREL_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_LOGREL_MSG2(a); \
        ASSERT_GUEST_PANIC(); \
        return; \
    } while (0)

/** @def ASSERT_GUEST_LOGREL_MSG_FAILED_STMT_RETURN_VOID
 * An assertion failed, execute @a a_Stmt and return void.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a       printf argument list (in parenthesis).
 * @param   a_Stmt  Statement to execute before returning in case of a failed
 *                  assertion.
 */
#define ASSERT_GUEST_LOGREL_MSG_FAILED_STMT_RETURN_VOID(a, a_Stmt) \
    do { \
        ASSERT_GUEST_LOGREL_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_LOGREL_MSG2(a); \
        ASSERT_GUEST_PANIC(); \
        a_Stmt; \
        return; \
    } while (0)

/** @def ASSERT_GUEST_LOGREL_MSG_FAILED_BREAK
 * An assertion failed, break.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a   printf argument list (in parenthesis).
 */
#define ASSERT_GUEST_LOGREL_MSG_FAILED_BREAK(a) \
    if (1)\
    { \
        ASSERT_GUEST_LOGREL_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_LOGREL_MSG2(a); \
        ASSERT_GUEST_PANIC(); \
        break; \
    } else \
        break

/** @def ASSERT_GUEST_LOGREL_MSG_FAILED_STMT_BREAK
 * An assertion failed, execute \a a_Stmt and break.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a   printf argument list (in parenthesis).
 * @param   a_Stmt  Statement to execute before break.
 */
#define ASSERT_GUEST_LOGREL_MSG_FAILED_STMT_BREAK(a, a_Stmt) \
    if (1) \
    { \
        ASSERT_GUEST_LOGREL_MSG1("failed", __LINE__, __FILE__, __PRETTY_FUNCTION__); \
        ASSERT_GUEST_LOGREL_MSG2(a); \
        ASSERT_GUEST_PANIC(); \
        a_Stmt; \
        break; \
    } else \
        break

/** @} */


/** @name Convenience Assertions Macros
 * @{
 */

/** @def ASSERT_GUEST_RC
 * Asserts a iprt status code successful.
 *
 * On failure it will print info about the rc and hit a breakpoint.
 *
 * @param   rc  iprt status code.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_RC(rc)                         ASSERT_GUEST_MSG_RC(rc, ("%Rra\n", (rc)))

/** @def ASSERT_GUEST_RC_STMT
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and execute
 * @a stmt if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   stmt    Statement to execute before returning in case of a failed
 *                  assertion.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_RC_STMT(rc, stmt)              ASSERT_GUEST_MSG_RC_STMT(rc, ("%Rra\n", (rc)), stmt)

/** @def ASSERT_GUEST_RC_RETURN
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and return if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_RC_RETURN(rc, rcRet)           ASSERT_GUEST_MSG_RC_RETURN(rc, ("%Rra\n", (rc)), rcRet)

/** @def ASSERT_GUEST_RC_STMT_RETURN
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only), execute
 * @a stmt and returns @a rcRet if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   stmt    Statement to execute before returning in case of a failed
 *                  assertion.
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_RC_STMT_RETURN(rc, stmt, rcRet) ASSERT_GUEST_MSG_RC_STMT_RETURN(rc, ("%Rra\n", (rc)), stmt, rcRet)

/** @def ASSERT_GUEST_RC_RETURN_VOID
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and return if it isn't.
 *
 * @param   rc      iprt status code.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_RC_RETURN_VOID(rc)             ASSERT_GUEST_MSG_RC_RETURN_VOID(rc, ("%Rra\n", (rc)))

/** @def ASSERT_GUEST_RC_STMT_RETURN_VOID
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only), and
 * execute the given statement/return if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   stmt    Statement to execute before returning on failure.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_RC_STMT_RETURN_VOID(rc, stmt)  ASSERT_GUEST_MSG_RC_STMT_RETURN_VOID(rc, ("%Rra\n", (rc)), stmt)

/** @def ASSERT_GUEST_RC_BREAK
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and break if it isn't.
 *
 * @param   rc      iprt status code.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_RC_BREAK(rc)                   ASSERT_GUEST_MSG_RC_BREAK(rc, ("%Rra\n", (rc)))

/** @def ASSERT_GUEST_RC_STMT_BREAK
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and break if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_RC_STMT_BREAK(rc, stmt)        ASSERT_GUEST_MSG_RC_STMT_BREAK(rc, ("%Rra\n", (rc)), stmt)

/** @def ASSERT_GUEST_MSG_RC
 * Asserts a iprt status code successful.
 *
 * It prints a custom message and hits a breakpoint on FAILURE.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_MSG_RC(rc, msg) \
    do { ASSERT_GUEST_MSG(RT_SUCCESS_NP(rc), msg); NOREF(rc); } while (0)

/** @def ASSERT_GUEST_MSG_RC_STMT
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and
 * execute @a stmt if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before returning in case of a failed
 *                  assertion.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_MSG_RC_STMT(rc, msg, stmt) \
    do {   ASSERT_GUEST_MSG_STMT(RT_SUCCESS_NP(rc), msg, stmt); NOREF(rc); } while (0)

/** @def ASSERT_GUEST_MSG_RC_RETURN
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and return
 * @a rcRet if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_MSG_RC_RETURN(rc, msg, rcRet) \
    do {   ASSERT_GUEST_MSG_RETURN(RT_SUCCESS_NP(rc), msg, rcRet); NOREF(rc); } while (0)

/** @def ASSERT_GUEST_MSG_RC_STMT_RETURN
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only), execute
 * @a stmt and return @a rcRet if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before returning in case of a failed
 *                  assertion.
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_MSG_RC_STMT_RETURN(rc, msg, stmt, rcRet) \
    do {   ASSERT_GUEST_MSG_STMT_RETURN(RT_SUCCESS_NP(rc), msg, stmt, rcRet); NOREF(rc); } while (0)

/** @def ASSERT_GUEST_MSG_RC_RETURN_VOID
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and return
 * void if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_MSG_RC_RETURN_VOID(rc, msg) \
    do {   ASSERT_GUEST_MSG_RETURN_VOID(RT_SUCCESS_NP(rc), msg); NOREF(rc); } while (0)

/** @def ASSERT_GUEST_MSG_RC_STMT_RETURN_VOID
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only), execute
 * @a stmt and return void if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_MSG_RC_STMT_RETURN_VOID(rc, msg, stmt) \
    do {   ASSERT_GUEST_MSG_STMT_RETURN_VOID(RT_SUCCESS_NP(rc), msg, stmt); NOREF(rc); } while (0)

/** @def ASSERT_GUEST_MSG_RC_BREAK
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and break
 * if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_MSG_RC_BREAK(rc, msg) \
    if (1) { ASSERT_GUEST_MSG_BREAK(RT_SUCCESS(rc), msg); NOREF(rc); } else do {} while (0)

/** @def ASSERT_GUEST_MSG_RC_STMT_BREAK
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only), execute
 * @a stmt and break if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_MSG_RC_STMT_BREAK(rc, msg, stmt) \
    if (1) { ASSERT_GUEST_MSG_STMT_BREAK(RT_SUCCESS_NP(rc), msg, stmt); NOREF(rc); } else do {} while (0)

/** @def ASSERT_GUEST_RC_SUCCESS
 * Asserts an iprt status code equals VINF_SUCCESS.
 *
 * On failure it will print info about the rc and hit a breakpoint.
 *
 * @param   rc  iprt status code.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_RC_SUCCESS(rc)                 do { ASSERT_GUEST_MSG((rc) == VINF_SUCCESS, ("%Rra\n", (rc))); NOREF(rc); } while (0)

/** @def ASSERT_GUEST_RC_SUCCESS_RETURN
 * Asserts that an iprt status code equals VINF_SUCCESS, bitch (RT_STRICT mode only) and return if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_RC_SUCCESS_RETURN(rc, rcRet)   ASSERT_GUEST_MSG_RETURN((rc) == VINF_SUCCESS, ("%Rra\n", (rc)), rcRet)

/** @def ASSERT_GUEST_RC_SUCCESS_RETURN_VOID
 * Asserts that an iprt status code equals VINF_SUCCESS, bitch (RT_STRICT mode only) and return if it isn't.
 *
 * @param   rc      iprt status code.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_RC_SUCCESS_RETURN_VOID(rc)     ASSERT_GUEST_MSG_RETURN_VOID((rc) == VINF_SUCCESS, ("%Rra\n", (rc)))

/** @def ASSERT_GUEST_RC_SUCCESS_BREAK
 * Asserts that an iprt status code equals VINF_SUCCESS, bitch (RT_STRICT mode only) and break if it isn't.
 *
 * @param   rc      iprt status code.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_RC_SUCCESS_BREAK(rc)            ASSERT_GUEST_MSG_BREAK((rc) == VINF_SUCCESS, ("%Rra\n", (rc)))

/** @def ASSERT_GUEST_RC_SUCCESS_STMT_BREAK
 * Asserts that an iprt status code equals VINF_SUCCESS, bitch (RT_STRICT mode only) and break if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_RC_SUCCESS_STMT_BREAK(rc, stmt) ASSERT_GUEST_MSG_STMT_BREAK((rc) == VINF_SUCCESS, ("%Rra\n", (rc)), stmt)

/** @def ASSERT_GUEST_GCPHYS32
 * Asserts that the high dword of a physical address is zero
 *
 * @param   GCPhys      The address (RTGCPHYS).
 */
#define ASSERT_GUEST_GCPHYS32(GCPhys)               ASSERT_GUEST_MSG(VALID_PHYS32(GCPhys), ("%RGp\n", (RTGCPHYS)(GCPhys)))


/** @def ASSERT_GUEST_RC
 * Asserts a iprt status code successful.
 *
 * On failure it will print info about the rc and hit a breakpoint.
 *
 * @param   rc  iprt status code.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_RC(rc)                         ASSERT_GUEST_LOGREL_MSG_RC(rc, ("%Rra\n", (rc)))

/** @def ASSERT_GUEST_LOGREL_RC_STMT
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and execute
 * @a stmt if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   stmt    Statement to execute before returning in case of a failed
 *                  assertion.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_RC_STMT(rc, stmt)              ASSERT_GUEST_LOGREL_MSG_RC_STMT(rc, ("%Rra\n", (rc)), stmt)

/** @def ASSERT_GUEST_LOGREL_RC_RETURN
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and return if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_RC_RETURN(rc, rcRet)           ASSERT_GUEST_LOGREL_MSG_RC_RETURN(rc, ("%Rra\n", (rc)), rcRet)

/** @def ASSERT_GUEST_LOGREL_RC_STMT_RETURN
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only), execute
 * @a stmt and returns @a rcRet if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   stmt    Statement to execute before returning in case of a failed
 *                  assertion.
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_RC_STMT_RETURN(rc, stmt, rcRet) ASSERT_GUEST_LOGREL_MSG_RC_STMT_RETURN(rc, ("%Rra\n", (rc)), stmt, rcRet)

/** @def ASSERT_GUEST_LOGREL_RC_RETURN_VOID
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and return if it isn't.
 *
 * @param   rc      iprt status code.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_RC_RETURN_VOID(rc)             ASSERT_GUEST_LOGREL_MSG_RC_RETURN_VOID(rc, ("%Rra\n", (rc)))

/** @def ASSERT_GUEST_LOGREL_RC_STMT_RETURN_VOID
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only), and
 * execute the given statement/return if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   stmt    Statement to execute before returning on failure.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_RC_STMT_RETURN_VOID(rc, stmt)  ASSERT_GUEST_LOGREL_MSG_RC_STMT_RETURN_VOID(rc, ("%Rra\n", (rc)), stmt)

/** @def ASSERT_GUEST_LOGREL_RC_BREAK
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and break if it isn't.
 *
 * @param   rc      iprt status code.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_RC_BREAK(rc)                   ASSERT_GUEST_LOGREL_MSG_RC_BREAK(rc, ("%Rra\n", (rc)))

/** @def ASSERT_GUEST_LOGREL_RC_STMT_BREAK
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and break if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_RC_STMT_BREAK(rc, stmt)        ASSERT_GUEST_LOGREL_MSG_RC_STMT_BREAK(rc, ("%Rra\n", (rc)), stmt)

/** @def ASSERT_GUEST_LOGREL_MSG_RC
 * Asserts a iprt status code successful.
 *
 * It prints a custom message and hits a breakpoint on FAILURE.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_MSG_RC(rc, msg) \
    do { ASSERT_GUEST_LOGREL_MSG(RT_SUCCESS_NP(rc), msg); NOREF(rc); } while (0)

/** @def ASSERT_GUEST_LOGREL_MSG_RC_STMT
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and
 * execute @a stmt if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before returning in case of a failed
 *                  assertion.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_MSG_RC_STMT(rc, msg, stmt) \
    do {   ASSERT_GUEST_LOGREL_MSG_STMT(RT_SUCCESS_NP(rc), msg, stmt); NOREF(rc); } while (0)

/** @def ASSERT_GUEST_LOGREL_MSG_RC_RETURN
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and return
 * @a rcRet if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_MSG_RC_RETURN(rc, msg, rcRet) \
    do {   ASSERT_GUEST_LOGREL_MSG_RETURN(RT_SUCCESS_NP(rc), msg, rcRet); NOREF(rc); } while (0)

/** @def ASSERT_GUEST_LOGREL_MSG_RC_STMT_RETURN
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only), execute
 * @a stmt and return @a rcRet if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before returning in case of a failed
 *                  assertion.
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_MSG_RC_STMT_RETURN(rc, msg, stmt, rcRet) \
    do {   ASSERT_GUEST_LOGREL_MSG_STMT_RETURN(RT_SUCCESS_NP(rc), msg, stmt, rcRet); NOREF(rc); } while (0)

/** @def ASSERT_GUEST_LOGREL_MSG_RC_RETURN_VOID
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and return
 * void if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_MSG_RC_RETURN_VOID(rc, msg) \
    do {   ASSERT_GUEST_LOGREL_MSG_RETURN_VOID(RT_SUCCESS_NP(rc), msg); NOREF(rc); } while (0)

/** @def ASSERT_GUEST_LOGREL_MSG_RC_STMT_RETURN_VOID
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only), execute
 * @a stmt and return void if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_MSG_RC_STMT_RETURN_VOID(rc, msg, stmt) \
    do {   ASSERT_GUEST_LOGREL_MSG_STMT_RETURN_VOID(RT_SUCCESS_NP(rc), msg, stmt); NOREF(rc); } while (0)

/** @def ASSERT_GUEST_LOGREL_MSG_RC_BREAK
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and break
 * if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_MSG_RC_BREAK(rc, msg) \
    if (1) { ASSERT_GUEST_LOGREL_MSG_BREAK(RT_SUCCESS(rc), msg); NOREF(rc); } else do {} while (0)

/** @def ASSERT_GUEST_LOGREL_MSG_RC_STMT_BREAK
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only), execute
 * @a stmt and break if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_MSG_RC_STMT_BREAK(rc, msg, stmt) \
    if (1) { ASSERT_GUEST_LOGREL_MSG_STMT_BREAK(RT_SUCCESS_NP(rc), msg, stmt); NOREF(rc); } else do {} while (0)

/** @def ASSERT_GUEST_LOGREL_RC_SUCCESS
 * Asserts an iprt status code equals VINF_SUCCESS.
 *
 * On failure it will print info about the rc and hit a breakpoint.
 *
 * @param   rc  iprt status code.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_RC_SUCCESS(rc)                 do { ASSERT_GUEST_LOGREL_MSG((rc) == VINF_SUCCESS, ("%Rra\n", (rc))); NOREF(rc); } while (0)

/** @def ASSERT_GUEST_LOGREL_RC_SUCCESS_RETURN
 * Asserts that an iprt status code equals VINF_SUCCESS, bitch (RT_STRICT mode only) and return if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_RC_SUCCESS_RETURN(rc, rcRet)   ASSERT_GUEST_LOGREL_MSG_RETURN((rc) == VINF_SUCCESS, ("%Rra\n", (rc)), rcRet)

/** @def ASSERT_GUEST_LOGREL_RC_SUCCESS_RETURN_VOID
 * Asserts that an iprt status code equals VINF_SUCCESS, bitch (RT_STRICT mode only) and return if it isn't.
 *
 * @param   rc      iprt status code.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_RC_SUCCESS_RETURN_VOID(rc)     ASSERT_GUEST_LOGREL_MSG_RETURN_VOID((rc) == VINF_SUCCESS, ("%Rra\n", (rc)))

/** @def ASSERT_GUEST_LOGREL_RC_SUCCESS_BREAK
 * Asserts that an iprt status code equals VINF_SUCCESS, bitch (RT_STRICT mode only) and break if it isn't.
 *
 * @param   rc      iprt status code.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_RC_SUCCESS_BREAK(rc)            ASSERT_GUEST_LOGREL_MSG_BREAK((rc) == VINF_SUCCESS, ("%Rra\n", (rc)))

/** @def ASSERT_GUEST_LOGREL_RC_SUCCESS_STMT_BREAK
 * Asserts that an iprt status code equals VINF_SUCCESS, bitch (RT_STRICT mode only) and break if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define ASSERT_GUEST_LOGREL_RC_SUCCESS_STMT_BREAK(rc, stmt) ASSERT_GUEST_LOGREL_MSG_STMT_BREAK((rc) == VINF_SUCCESS, ("%Rra\n", (rc)), stmt)

/** @def ASSERT_GUEST_LOGREL_GCPHYS32
 * Asserts that the high dword of a physical address is zero
 *
 * @param   GCPhys      The address (RTGCPHYS).
 */
#define ASSERT_GUEST_LOGREL_GCPHYS32(GCPhys)               ASSERT_GUEST_LOGREL_MSG(VALID_PHYS32(GCPhys), ("%RGp\n", (RTGCPHYS)(GCPhys)))


/** @} */


/** @} */

#endif /* !VBOX_INCLUDED_AssertGuest_h */

