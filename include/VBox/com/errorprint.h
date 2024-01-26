/** @file
 * MS COM / XPCOM Abstraction Layer - Error Reporting.
 *
 * Error printing macros using shared functions defined in shared glue code.
 * Use these CHECK_* macros for efficient error checking around calling COM
 * methods.
 */

/*
 * Copyright (C) 2009-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_com_errorprint_h
#define VBOX_INCLUDED_com_errorprint_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/com/ErrorInfo.h>


/** @defgroup grp_com_error_reporting   Error Reporting
 * @ingroup grp_com
 * @{
 */

namespace com
{

// shared prototypes; these are defined in shared glue code and are
// compiled only once for all front-ends
void GluePrintErrorInfo(const com::ErrorInfo &info);
void GluePrintErrorContext(const char *pcszContext, const char *pcszSourceFile, uint32_t uLine, bool fWarning = false);
void GluePrintRCMessage(HRESULT hrc);
void GlueHandleComError(ComPtr<IUnknown> iface, const char *pcszContext, HRESULT hrc, const char *pcszSourceFile, uint32_t uLine);
void GlueHandleComErrorNoCtx(ComPtr<IUnknown> iface, HRESULT hrc);
void GlueHandleComErrorProgress(ComPtr<IProgress> progress, const char *pcszContext, HRESULT hrc,
                                const char *pcszSourceFile, uint32_t uLine);

/**
 * Extended macro that implements all the other CHECK_ERROR2XXX macros.
 *
 * Calls the method of the given interface and checks the return status code.
 * If the status indicates failure, as much information as possible is reported
 * about the error, including current source file and line.
 *
 * After reporting an error, the statement |stmtError| is executed.
 *
 * This macro family is intended for command line tools like VBoxManage, but
 * could also be handy for debugging.
 *
 * @param   type        For defining @a hrc locally inside the the macro
 *                      expansion, pass |HRESULT| otherwise |RT_NOTHING|.
 * @param   hrc         Name of the HRESULT variable to assign the result of the
 *                      method call to.
 * @param   iface       The interface pointer (can be a smart pointer object).
 * @param   method      The method to invoke together with the parameters.
 * @param   stmtError   Statement to be executed after reporting failures.  This
 *                      can be a |break| or |return| statement, if so desired.
 *
 * @remarks Unlike CHECK_ERROR, CHECK_ERROR_RET and family, this macro family
 *          does not presuppose a |rc| variable but instead either let the user
 *          specify the variable to use or employs  a local variable |hrcCheck|
 *          within its own scope.
 *
 * @sa      CHECK_ERROR2, CHECK_ERROR2I, CHECK_ERROR2_STMT, CHECK_ERROR2I_STMT,
 *          CHECK_ERROR2_BREAK, CHECK_ERROR2I_BREAK, CHECK_ERROR2_RET,
 *          CHECK_ERROR2I_RET
 */
#define CHECK_ERROR2_EX(type, hrc, iface, method, stmtError) \
    if (1) { \
        type hrc = iface->method; \
        if (SUCCEEDED(hrc) && !SUCCEEDED_WARNING(hrc)) \
        { /*likely*/ } \
        else \
        { \
            com::GlueHandleComError(iface, #method, (hrc), __FILE__, __LINE__); \
            if (!SUCCEEDED_WARNING(hrc)) \
            { \
                stmtError; \
            } \
        } \
    } else do { /* nothing */ } while (0)


/**
 *  Calls the given method of the given interface and then checks if the return
 *  value (COM result code) indicates a failure. If so, prints the failed
 *  function/line/file, the description of the result code and attempts to
 *  query the extended error information on the current thread (using
 *  com::ErrorInfo) if the interface reports that it supports error information.
 *
 *  Used by command line tools or for debugging and assumes the |HRESULT rc|
 *  variable is accessible for assigning in the current scope.
 * @sa CHECK_ERROR2, CHECK_ERROR2I
 */
#define CHECK_ERROR(iface, method) \
    do { \
        hrc = iface->method; \
        if (FAILED(hrc) || SUCCEEDED_WARNING(hrc)) \
            com::GlueHandleComError(iface, #method, hrc, __FILE__, __LINE__); \
    } while (0)
/**
 * Simplified version of CHECK_ERROR2_EX, no error statement or type necessary.
 *
 * @param   hrc         Name of the HRESULT variable to assign the result of the
 *                      method call to.
 * @param   iface       The interface pointer (can be a smart pointer object).
 * @param   method      The method to invoke together with the parameters.
 * @sa CHECK_ERROR2I, CHECK_ERROR2_EX
 */
#define CHECK_ERROR2(hrc, iface, method)            CHECK_ERROR2_EX(RT_NOTHING, hrc, iface, method, (void)1)
/**
 * Simplified version of CHECK_ERROR2_EX that uses an internal variable
 * |hrcCheck| for holding the result and have no error statement.
 *
 * @param   iface       The interface pointer (can be a smart pointer object).
 * @param   method      The method to invoke together with the parameters.
 * @sa CHECK_ERROR2, CHECK_ERROR2_EX
 */
#define CHECK_ERROR2I(iface, method)                CHECK_ERROR2_EX(HRESULT, hrcCheck, iface, method, (void)1)


/**
 * Same as CHECK_ERROR except that it also executes the statement |stmt| on
 * failure.
 * @sa CHECK_ERROR2_STMT, CHECK_ERROR2I_STMT
 */
#define CHECK_ERROR_STMT(iface, method, stmt) \
    do { \
        hrc = iface->method; \
        if (FAILED(hrc) || SUCCEEDED_WARNING(hrc)) \
        { \
            com::GlueHandleComError(iface, #method, hrc, __FILE__, __LINE__); \
            if (!SUCCEEDED_WARNING(hrc) \
            { \
                stmt; \
            } \
        } \
    } while (0)
/**
 * Simplified version of CHECK_ERROR2_EX (no @a hrc type).
 *
 * @param   hrc         Name of the HRESULT variable to assign the result of the
 *                      method call to.
 * @param   iface       The interface pointer (can be a smart pointer object).
 * @param   method      The method to invoke together with the parameters.
 * @param   stmt        Statement to be executed after reporting failures.
 * @sa CHECK_ERROR2I_STMT, CHECK_ERROR2_EX
 */
#define CHECK_ERROR2_STMT(hrc, iface, method, stmt) CHECK_ERROR2_EX(RT_NOTHING, hrc, iface, method, stmt)
/**
 * Simplified version of CHECK_ERROR2_EX that uses an internal variable
 * |hrcCheck| for holding the result.
 *
 * @param   iface       The interface pointer (can be a smart pointer object).
 * @param   method      The method to invoke together with the parameters.
 * @param   stmt        Statement to be executed after reporting failures.
 * @sa CHECK_ERROR2_STMT, CHECK_ERROR2_EX
 */
#define CHECK_ERROR2I_STMT(iface, method, stmt)     CHECK_ERROR2_EX(HRESULT, hrcCheck, iface, method, stmt)


/**
 *  Does the same as CHECK_ERROR(), but executes the |break| statement on
 *  failure.
 * @sa CHECK_ERROR2_BREAK, CHECK_ERROR2I_BREAK
 */
#ifdef __GNUC__
# define CHECK_ERROR_BREAK(iface, method) \
    __extension__ \
    ({ \
        hrc = iface->method; \
        if (FAILED(hrc) || SUCCEEDED_WARNING(hrc)) \
        { \
            com::GlueHandleComError(iface, #method, hrc, __FILE__, __LINE__); \
            if (!SUCCEEDED_WARNING(hrc)) \
                break; \
        } \
    })
#else
# define CHECK_ERROR_BREAK(iface, method) \
    if (1) \
    { \
        hrc = iface->method; \
        if (FAILED(hrc)) \
        { \
            com::GlueHandleComError(iface, #method, hrc, __FILE__, __LINE__); \
            if (!SUCCEEDED_WARNING(hrc)) \
                break; \
        } \
    } \
    else do {} while (0)
#endif
/**
 * Simplified version of CHECK_ERROR2_EX that executes the |break| statement
 * after error reporting (no @a hrc type).
 *
 * @param   hrc         The result variable (type HRESULT).
 * @param   iface       The interface pointer (can be a smart pointer object).
 * @param   method      The method to invoke together with the parameters.
 * @sa CHECK_ERROR2I_BREAK, CHECK_ERROR2_EX
 */
#define CHECK_ERROR2_BREAK(hrc, iface, method)      CHECK_ERROR2_EX(RT_NOTHING, hrc, iface, method, break)
/**
 * Simplified version of CHECK_ERROR2_EX that executes the |break| statement
 * after error reporting and that uses an internal variable |hrcCheck| for
 * holding the result.
 *
 * @param   iface       The interface pointer (can be a smart pointer object).
 * @param   method      The method to invoke together with the parameters.
 * @sa CHECK_ERROR2_BREAK, CHECK_ERROR2_EX
 */
#define CHECK_ERROR2I_BREAK(iface, method)          CHECK_ERROR2_EX(HRESULT, hrcCheck, iface, method, break)
/**
 * Simplified version of CHECK_ERROR2_EX that executes the |stmt;break|
 * statements after error reporting and that uses an internal variable
 * |hrcCheck| for holding the result.
 *
 * @param   iface       The interface pointer (can be a smart pointer object).
 * @param   method      The method to invoke together with the parameters.
 * @param   stmt        Statement to be executed after reporting failures.
 * @sa CHECK_ERROR2_BREAK, CHECK_ERROR2_EX
 */
#define CHECK_ERROR2I_BREAK_STMT(iface, method, stmt) CHECK_ERROR2_EX(HRESULT, hrcCheck, iface, method, stmt; break)


/**
 * Does the same as CHECK_ERROR(), but executes the |return ret| statement on
 * failure.
 * @sa CHECK_ERROR2_RET, CHECK_ERROR2I_RET
 */
#define CHECK_ERROR_RET(iface, method, ret) \
    do { \
        hrc = iface->method; \
        if (FAILED(hrc) || SUCCEEDED_WARNING(hrc)) \
        { \
            com::GlueHandleComError(iface, #method, hrc, __FILE__, __LINE__); \
            if (!SUCCEEDED_WARNING(hrc)) \
                return (ret); \
        } \
    } while (0)
/**
 * Simplified version of CHECK_ERROR2_EX that executes the |return (rcRet)|
 * statement after error reporting.
 *
 * @param   hrc         The result variable (type HRESULT).
 * @param   iface       The interface pointer (can be a smart pointer object).
 * @param   method      The method to invoke together with the parameters.
 * @param   rcRet       What to return on failure.
 */
#define CHECK_ERROR2_RET(hrc, iface, method, rcRet) CHECK_ERROR2_EX(RT_NOTHING, hrc, iface, method, return (rcRet))
/**
 * Simplified version of CHECK_ERROR2_EX that executes the |return (rcRet)|
 * statement after error reporting and that uses an internal variable |hrcCheck|
 * for holding the result.
 *
 * @param   iface       The interface pointer (can be a smart pointer object).
 * @param   method      The method to invoke together with the parameters.
 * @param   rcRet       What to return on failure.  Use |hrcCheck| to return
 *                      the status code of the method call.
 */
#define CHECK_ERROR2I_RET(iface, method, rcRet)     CHECK_ERROR2_EX(HRESULT, hrcCheck, iface, method, return (rcRet))


/**
 * Check the progress object for an error and if there is one print out the
 * extended error information.
 * @remarks Requires HRESULT variable named @a rc.
 */
#define CHECK_PROGRESS_ERROR(progress, msg) \
    do { \
        LONG iRc; \
        hrc = progress->COMGETTER(ResultCode)(&iRc); \
        if (FAILED(hrc) || FAILED(iRc)) \
        { \
            if (SUCCEEDED(hrc)) hrc = iRc; else iRc = hrc; \
            RTMsgError msg; \
            com::GlueHandleComErrorProgress(progress, __PRETTY_FUNCTION__, iRc, __FILE__, __LINE__); \
        } \
    } while (0)

/**
 * Does the same as CHECK_PROGRESS_ERROR(), but executes the |break| statement
 * on failure.
 * @remarks Requires HRESULT variable named @a rc.
 */
#ifdef __GNUC__
# define CHECK_PROGRESS_ERROR_BREAK(progress, msg) \
    __extension__ \
    ({ \
        LONG iRc; \
        hrc = progress->COMGETTER(ResultCode)(&iRc); \
        if (FAILED(hrc) || FAILED(iRc)) \
        { \
            if (SUCCEEDED(hrc)) hrc = iRc; else iRc = hrc; \
            RTMsgError msg; \
            com::GlueHandleComErrorProgress(progress, __PRETTY_FUNCTION__, iRc, __FILE__, __LINE__); \
            break; \
        } \
    })
#else
# define CHECK_PROGRESS_ERROR_BREAK(progress, msg) \
    if (1) \
    { \
        LONG iRc; \
        hrc = progress->COMGETTER(ResultCode)(&iRc); \
        if (FAILED(hrc) || FAILED(iRc)) \
        { \
            if (SUCCEEDED(hrc)) hrc = iRc; else iRc = hrc; \
            RTMsgError msg; \
            com::GlueHandleComErrorProgress(progress, __PRETTY_FUNCTION__, iRc, __FILE__, __LINE__); \
            break; \
        } \
    } \
    else do {} while (0)
#endif

/**
 * Does the same as CHECK_PROGRESS_ERROR(), but executes the |return ret|
 * statement on failure.
 */
#define CHECK_PROGRESS_ERROR_RET(progress, msg, ret) \
    do { \
        LONG iRc; \
        HRESULT hrcCheck = progress->COMGETTER(ResultCode)(&iRc); \
        if (SUCCEEDED(hrcCheck) && SUCCEEDED(iRc)) \
        { /* likely */ } \
        else \
        { \
            RTMsgError msg; \
            com::GlueHandleComErrorProgress(progress, __PRETTY_FUNCTION__, \
                                            SUCCEEDED(hrcCheck) ? iRc : hrcCheck, __FILE__, __LINE__); \
            return (ret); \
        } \
    } while (0)

/**
 *  Asserts the given expression is true. When the expression is false, prints
 *  a line containing the failed function/line/file; otherwise does nothing.
 */
#define ASSERT(expr) \
    do { \
        if (!(expr)) \
        { \
            RTPrintf ("[!] ASSERTION FAILED at line %d: %s\n", __LINE__, #expr); \
            Log (("[!] ASSERTION FAILED at line %d: %s\n", __LINE__, #expr)); \
        } \
    } while (0)


/**
 * Does the same as ASSERT(), but executes the |return ret| statement if the
 * expression to assert is false;
 * @remarks WARNING! @a expr is evalutated TWICE!
 */
#define ASSERT_RET(expr, ret) \
    do { ASSERT(expr); if (!(expr)) return (ret); } while (0)

/**
 * Does the same as ASSERT(), but executes the |break| statement if the
 * expression to assert is false;
 * @remarks WARNING! @a expr is evalutated TWICE!
 */
#define ASSERT_BREAK(expr, ret) \
    if (1) { ASSERT(expr); if (!(expr)) break; } else do {} while (0)

} /* namespace com */


/** @} */

#endif /* !VBOX_INCLUDED_com_errorprint_h */

