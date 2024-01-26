/** @file
 * IPRT - Message Formatting.
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

#ifndef IPRT_INCLUDED_message_h
#define IPRT_INCLUDED_message_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/stdarg.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_msg        RTMsg - Message Formatting
 * @ingroup grp_rt
 * @{
 */

/**
 * Sets the program name to use.
 *
 * @returns IPRT status code.
 * @param   pszFormat       The program name format string.
 * @param   ...             Format arguments.
 */
RTDECL(int)  RTMsgSetProgName(const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(1, 2);

/**
 * Print error message to standard error.
 *
 * The message will be prefixed with the file name part of process image name
 * (i.e. no path) and "error: ".  If the message doesn't end with a new line,
 * one will be added.  The caller should call this with an empty string if
 * unsure whether the cursor is currently position at the start of a new line.
 *
 * @returns IPRT status code.
 * @param   pszFormat       The message format string.
 * @param   ...             Format arguments.
 */
RTDECL(int)  RTMsgError(const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(1, 2);

/**
 * Print error message to standard error.
 *
 * The message will be prefixed with the file name part of process image name
 * (i.e. no path) and "error: ".  If the message doesn't end with a new line,
 * one will be added.  The caller should call this with an empty string if
 * unsure whether the cursor is currently position at the start of a new line.
 *
 * @returns IPRT status code.
 * @param   pszFormat       The message format string.
 * @param   va              Format arguments.
 */
RTDECL(int)  RTMsgErrorV(const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(1, 0);

/**
 * Same as RTMsgError() except for the return value.
 *
 * @returns @a enmExitCode
 * @param   enmExitCode     What to exit code to return.  This is mainly for
 *                          saving some vertical space in the source file.
 * @param   pszFormat       The message format string.
 * @param   ...             Format arguments.
 */
RTDECL(RTEXITCODE) RTMsgErrorExit(RTEXITCODE enmExitCode, const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(2, 3);

/**
 * Same as RTMsgErrorV() except for the return value.
 *
 * @returns @a enmExitCode
 * @param   enmExitCode     What to exit code to return.  This is mainly for
 *                          saving some vertical space in the source file.
 * @param   pszFormat       The message format string.
 * @param   va              Format arguments.
 */
RTDECL(RTEXITCODE) RTMsgErrorExitV(RTEXITCODE enmExitCode, const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(2, 0);

/**
 * Same as RTMsgError() except for always returning RTEXITCODE_FAILURE.
 *
 * @returns RTEXITCODE_FAILURE
 * @param   pszFormat       The message format string.
 * @param   ...             Format arguments.
 */
RTDECL(RTEXITCODE) RTMsgErrorExitFailure(const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(1, 2);

/**
 * Same as RTMsgErrorV() except for always returning RTEXITCODE_FAILURE.
 *
 * @returns RTEXITCODE_FAILURE
 * @param   pszFormat       The message format string.
 * @param   va              Format arguments.
 */
RTDECL(RTEXITCODE) RTMsgErrorExitFailureV(const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(1, 0);

/**
 * Same as RTMsgError() except for the return value.
 *
 * @returns @a rcRet
 * @param   rcRet           What IPRT status to return. This is mainly for
 *                          saving some vertical space in the source file.
 * @param   pszFormat       The message format string.
 * @param   ...             Format arguments.
 */
RTDECL(int) RTMsgErrorRc(int rcRet, const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(2, 3);

/**
 * Same as RTMsgErrorV() except for the return value.
 *
 * @returns @a rcRet
 * @param   rcRet           What IPRT status to return. This is mainly for
 *                          saving some vertical space in the source file.
 * @param   pszFormat       The message format string.
 * @param   va              Format arguments.
 */
RTDECL(int) RTMsgErrorRcV(int rcRet, const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(2, 0);

/**
 * For reporting syntax errors.
 *
 * @returns RTEXITCODE_SYNTAX
 * @param   pszFormat       The message format string.  Newline not needed.
 * @param   ...             Format arguments.
 */
RTDECL(RTEXITCODE) RTMsgSyntax(const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(1, 2);

/**
 * For reporting syntax errors.
 *
 * @returns RTEXITCODE_SYNTAX
 * @param   pszFormat       The message format string.  Newline not needed.
 * @param   va              Format arguments.
 */
RTDECL(RTEXITCODE) RTMsgSyntaxV(const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(1, 0);

/**
 * Print an error message for a RTR3Init failure and suggest an exit code.
 *
 * @code
 *
 * int rc = RTR3Init();
 * if (RT_FAILURE(rc))
 *     return RTMsgInitFailure(rc);
 *
 * @endcode
 *
 * @returns Appropriate exit code.
 * @param   rcRTR3Init      The status code returned by RTR3Init.
 */
RTDECL(RTEXITCODE) RTMsgInitFailure(int rcRTR3Init);

/**
 * Print informational message to standard error.
 *
 * The message will be prefixed with the file name part of process image name
 * (i.e. no path) and "warning: ".  If the message doesn't end with a new line,
 * one will be added.  The caller should call this with an empty string if
 * unsure whether the cursor is currently position at the start of a new line.
 *
 * @returns IPRT status code.
 * @param   pszFormat       The message format string.
 * @param   ...             Format arguments.
 */
RTDECL(int)  RTMsgWarning(const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(1, 2);

/**
 * Print informational message to standard error.
 *
 * The message will be prefixed with the file name part of process image name
 * (i.e. no path) and "warning: ".  If the message doesn't end with a new line,
 * one will be added.  The caller should call this with an empty string if
 * unsure whether the cursor is currently position at the start of a new line.
 *
 * @returns IPRT status code.
 * @param   pszFormat       The message format string.
 * @param   va              Format arguments.
 */
RTDECL(int)  RTMsgWarningV(const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(1, 0);

/**
 * Print informational message to standard output.
 *
 * The message will be prefixed with the file name part of process image name
 * (i.e. no path) and "info: ".  If the message doesn't end with a new line,
 * one will be added.  The caller should call this with an empty string if
 * unsure whether the cursor is currently position at the start of a new line.
 *
 * @returns IPRT status code.
 * @param   pszFormat       The message format string.
 * @param   ...             Format arguments.
 */
RTDECL(int)  RTMsgInfo(const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(1, 2);

/**
 * Print informational message to standard output.
 *
 * The message will be prefixed with the file name part of process image name
 * (i.e. no path) and "info: ".  If the message doesn't end with a new line,
 * one will be added.  The caller should call this with an empty string if
 * unsure whether the cursor is currently position at the start of a new line.
 *
 * @returns IPRT status code.
 * @param   pszFormat       The message format string.
 * @param   va              Format arguments.
 */
RTDECL(int)  RTMsgInfoV(const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(1, 0);



/** @defgroup grp_rt_msg_refentry Help generated from refentry/manpage.
 *
 * The refentry/manpage docbook source in doc/manual/en_US/man_* is processed by
 * doc/manual/docbook-refentry-to-C-help.xsl and turned a set of the structures
 * defined here.
 *
 * @{
 */

/** The non-breaking space character.
 * @remarks We could've used U+00A0, but it is easier both to encode and to
 *          search and replace a single ASCII character. */
#define RTMSGREFENTRY_NBSP              '\b'

/** @name REFENTRYSTR_SCOPE_XXX - Common string scoping and flags.
 * @{ */
/** Same scope as previous string table entry, flags are reset and can be
 *  ORed in. */
#define RTMSGREFENTRYSTR_SCOPE_SAME     UINT64_C(0)
/** Global scope. */
#define RTMSGREFENTRYSTR_SCOPE_GLOBAL   UINT64_C(0x0fffffffffffffff)
/** Scope mask. */
#define RTMSGREFENTRYSTR_SCOPE_MASK     UINT64_C(0x0fffffffffffffff)
/** Flags mask. */
#define RTMSGREFENTRYSTR_FLAGS_MASK     UINT64_C(0xf000000000000000)
/** Command synopsis, special hanging indent rules applies. */
#define RTMSGREFENTRYSTR_FLAGS_SYNOPSIS RT_BIT_64(63)
/** @} */

/** String table entry for a refentry. */
typedef struct RTMSGREFENTRYSTR
{
    /** The scope of the string.  There are two predefined scopes,
     *  REFENTRYSTR_SCOPE_SAME and REFENTRYSTR_SCOPE_GLOBAL.  The rest are
     *  reference entry specific. */
    uint64_t        fScope;
    /** The string.  Non-breaking space is represented by the char
     * REFENTRY_NBSP defines, just in case the string needs wrapping.  There is
     * no trailing newline, that's implicit. */
    const char     *psz;
} RTMSGREFENTRYSTR;
/** Pointer to a read-only string table entry. */
typedef const RTMSGREFENTRYSTR *PCRTMSGREFENTRYSTR;

/** Refentry string table. */
typedef struct RTMSGREFENTRYSTRTAB
{
    /** Number of strings. */
    uint16_t            cStrings;
    /** Reserved for future use. */
    uint16_t            fReserved;
    /** Pointer to the string table. */
    PCRTMSGREFENTRYSTR  paStrings;
} RTMSGREFENTRYSTRTAB;
/** Pointer to a read-only string table. */
typedef RTMSGREFENTRYSTRTAB const *PCRTMSGREFENTRYSTRTAB;

/**
 * Help extracted from a docbook refentry document.
 */
typedef struct RTMSGREFENTRY
{
    /** Internal reference entry identifier.  */
    int64_t             idInternal;
    /** Usage synopsis. */
    RTMSGREFENTRYSTRTAB Synopsis;
    /** Full help. */
    RTMSGREFENTRYSTRTAB Help;
    /** Brief command description. */
    const char         *pszBrief;
} RTMSGREFENTRY;
/** Pointer to a read-only refentry help extract structure. */
typedef RTMSGREFENTRY const *PCRTMSGREFENTRY;


#ifndef IPRT_INCLUDED_stream_h
typedef struct RTSTREAM *PRTSTREAM;
#endif


/**
 * Print the synopsis to the given stream.
 *
 * @returns Current number of pending blank lines.
 * @param   pStrm               The output stream.
 * @param   pEntry              The refentry to print the help for.
 */
RTDECL(int) RTMsgRefEntrySynopsis(PRTSTREAM pStrm, PCRTMSGREFENTRY pEntry);


/**
 * Print the synopsis to the given stream.
 *
 * @returns Current number of pending blank lines.
 * @param   pStrm               The output stream.
 * @param   pEntry              The refentry to print the help for.
 * @param   fScope              The scope inclusion mask.
 * @param   fFlags              RTMSGREFENTRY_SYNOPSIS_F_XXX.
 */
RTDECL(int) RTMsgRefEntrySynopsisEx(PRTSTREAM pStrm, PCRTMSGREFENTRY pEntry, uint64_t fScope, uint32_t fFlags);
/** @name  RTMSGREFENTRY_SYNOPSIS_F_XXX -  Flags for RTMsgRefEntrySynopsisEx.
 * @{  */
/** Prefix the output with 'Usage:'.   */
#define RTMSGREFENTRY_SYNOPSIS_F_USAGE      RT_BIT_32(0)
/** @}  */


/**
 * Print the help text to the given stream.
 *
 * @returns Current number of pending blank lines.
 * @param   pStrm               The output stream.
 * @param   pEntry              The refentry to print the help for.
 */
RTDECL(int) RTMsgRefEntryHelp(PRTSTREAM pStrm, PCRTMSGREFENTRY pEntry);

/**
 * Print the help text to the given stream, extended version.
 *
 * @returns Current number of pending blank lines.
 * @param   pStrm               The output stream.
 * @param   pEntry              The refentry to print the help for.
 * @param   fScope              The scope inclusion mask.
 * @param   fFlags              Reserved, MBZ.
 */
RTDECL(int) RTMsgRefEntryHelpEx(PRTSTREAM pStrm, PCRTMSGREFENTRY pEntry, uint64_t fScope, uint32_t fFlags);

/**
 * Prints a string table.
 *
 * @returns Current number of pending blank lines.
 * @param   pStrm               The output stream.
 * @param   pStrTab             The string table.
 * @param   fScope              The selection scope.
 * @param   pcPendingBlankLines In: Pending blank lines from previous string
 *                              table.  Out: Pending blank lines.
 * @param   pcLinesWritten      Pointer to variable that should be incremented
 *                              by the number of lines written.  Optional.
 */
RTDECL(int) RTMsgRefEntryPrintStringTable(PRTSTREAM pStrm, PCRTMSGREFENTRYSTRTAB pStrTab, uint64_t fScope,
                                          uint32_t *pcPendingBlankLines, uint32_t *pcLinesWritten);

/** @} */


/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_message_h */

