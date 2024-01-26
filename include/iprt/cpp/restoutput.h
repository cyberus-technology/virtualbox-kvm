/** @file
 * IPRT - C++ Representational State Transfer (REST) Output Classes.
 */

/*
 * Copyright (C) 2008-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_cpp_restoutput_h
#define IPRT_INCLUDED_cpp_restoutput_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/stdarg.h>
#include <iprt/cpp/ministring.h>


/** @defgroup grp_rt_cpp_restoutput C++ Representational State Transfer (REST) Output Classes.
 * @ingroup grp_rt_cpp
 * @{
 */


/**
 * Abstract base class for serializing data objects.
 */
class RT_DECL_CLASS RTCRestOutputBase
{
public:
    RTCRestOutputBase() RT_NOEXCEPT;
    virtual ~RTCRestOutputBase();

    /**
     * Raw output function.
     *
     * @returns Number of bytes outputted.
     * @param   a_pchString     The string to output (not necessarily terminated).
     * @param   a_cchToWrite    The length of the string
     */
    virtual size_t output(const char *a_pchString, size_t a_cchToWrite) RT_NOEXCEPT = 0;

    /**
     * RTStrPrintf like function (see @ref pg_rt_str_format).
     *
     * @returns Number of bytes outputted.
     * @param   pszFormat   The format string.
     * @param   ...         Argument specfied in @a pszFormat.
     */
    inline size_t printf(const char *pszFormat, ...) RT_NOEXCEPT RT_IPRT_FORMAT_ATTR(2, 3)
    {
        va_list va;
        va_start(va, pszFormat);
        size_t cchWritten = this->vprintf(pszFormat, va);
        va_end(va);
        return cchWritten;
    }

    /**
     * RTStrPrintfV like function (see @ref pg_rt_str_format).
     *
     * @returns Number of bytes outputted.
     * @param   pszFormat   The format string.
     * @param   va          Argument specfied in @a pszFormat.
     */
    size_t vprintf(const char *pszFormat, va_list va) RT_NOEXCEPT RT_IPRT_FORMAT_ATTR(2, 0);

    /**
     * Begins an array.
     * @returns Previous output state.  Pass to endArray() when done.
     */
    virtual uint32_t beginArray() RT_NOEXCEPT;

    /**
     * Ends an array.
     * @param   a_uOldState     Previous output state (returned by beginArray()).
     */
    virtual void endArray(uint32_t a_uOldState) RT_NOEXCEPT;

    /**
     * Begins an object.
     * @returns Previous output state.  Pass to endObject() when done.
     */
    virtual uint32_t beginObject() RT_NOEXCEPT;

    /**
     * Ends an array.
     * @param   a_uOldState     Previous output state (returned by beginObject()).
     */
    virtual void endObject(uint32_t a_uOldState) RT_NOEXCEPT;

    /**
     * Outputs a value separator.
     * This is called before a value, not after.
     */
    virtual void valueSeparator() RT_NOEXCEPT;

    /**
     * Outputs a value separator, name and name separator.
     */
    virtual void valueSeparatorAndName(const char *a_pszName, size_t a_cchName) RT_NOEXCEPT;

    /** Outputs a null-value. */
    void nullValue() RT_NOEXCEPT;

protected:
    /** The current indentation level (bits 15:0) and separator state (bit 31). */
    uint32_t m_uState;

    /** @callback_method_impl{FNRTSTROUTPUT} */
    static DECLCALLBACK(size_t) printfOutputCallback(void *pvArg, const char *pachChars, size_t cbChars) RT_NOEXCEPT;
};


/**
 * Abstract base class for pretty output.
 */
class RT_DECL_CLASS RTCRestOutputPrettyBase : public RTCRestOutputBase
{
public:
    RTCRestOutputPrettyBase() RT_NOEXCEPT;
    virtual ~RTCRestOutputPrettyBase();

    /**
     * Begins an array.
     * @returns Previous output state.  Pass to endArray() when done.
     */
    virtual uint32_t beginArray() RT_NOEXCEPT RT_OVERRIDE;

    /**
     * Ends an array.
     * @param   a_uOldState     Previous output state (returned by beginArray()).
     */
    virtual void endArray(uint32_t a_uOldState) RT_NOEXCEPT RT_OVERRIDE;

    /**
     * Begins an object.
     * @returns Previous output state.  Pass to endObject() when done.
     */
    virtual uint32_t beginObject() RT_NOEXCEPT RT_OVERRIDE;

    /**
     * Ends an array.
     * @param   a_uOldState     Previous output state (returned by beginObject()).
     */
    virtual void endObject(uint32_t a_uOldState) RT_NOEXCEPT RT_OVERRIDE;

    /**
     * Outputs a value separator.
     * This is called before a value, not after.
     */
    virtual void valueSeparator() RT_NOEXCEPT RT_OVERRIDE;

    /**
     * Outputs a value separator, name and name separator.
     */
    virtual void valueSeparatorAndName(const char *a_pszName, size_t a_cchName) RT_NOEXCEPT RT_OVERRIDE;

protected:
    /** Helper for outputting the correct amount of indentation. */
    void outputIndentation() RT_NOEXCEPT;
};


/**
 * Serialize to a string object.
 */
class RT_DECL_CLASS RTCRestOutputToString : public RTCRestOutputBase
{
public:
    /**
     * Creates an instance that appends to @a a_pDst.
     * @param   a_pDst      Pointer to the destination string object.
     *                      NULL is not accepted and will assert.
     * @param   a_fAppend   Whether to append to the current string value, or
     *                      nuke the string content before starting the output.
     */
    RTCRestOutputToString(RTCString *a_pDst, bool a_fAppend = false) RT_NOEXCEPT;
    virtual ~RTCRestOutputToString();

    virtual size_t output(const char *a_pchString, size_t a_cchToWrite) RT_NOEXCEPT RT_OVERRIDE;

    /**
     * Finalizes the output and releases the string object to the caller.
     *
     * @returns The released string object.  NULL if we ran out of memory or if
     *          called already.
     *
     * @remark  This sets m_pDst to NULL and the object cannot be use for any
     *          more output afterwards.
     */
    virtual RTCString *finalize() RT_NOEXCEPT;

protected:
    /** Pointer to the destination string.  NULL after finalize().   */
    RTCString  *m_pDst;
    /** Set if we ran out of memory and should ignore subsequent calls. */
    bool        m_fOutOfMemory;

    /* Make non-copyable (RTCNonCopyable causes warnings): */
    RTCRestOutputToString(RTCRestOutputToString const &);
    RTCRestOutputToString *operator=(RTCRestOutputToString const &);
};


/**
 * Serialize pretty JSON to a string object.
 */
class RT_DECL_CLASS RTCRestOutputPrettyToString : public RTCRestOutputPrettyBase
{
public:
    /**
     * Creates an instance that appends to @a a_pDst.
     * @param   a_pDst      Pointer to the destination string object.
     *                      NULL is not accepted and will assert.
     * @param   a_fAppend   Whether to append to the current string value, or
     *                      nuke the string content before starting the output.
     */
    RTCRestOutputPrettyToString(RTCString *a_pDst, bool a_fAppend = false) RT_NOEXCEPT;
    virtual ~RTCRestOutputPrettyToString();

    virtual size_t output(const char *a_pchString, size_t a_cchToWrite) RT_NOEXCEPT RT_OVERRIDE;

    /**
     * Finalizes the output and releases the string object to the caller.
     *
     * @returns The released string object.  NULL if we ran out of memory or if
     *          called already.
     *
     * @remark  This sets m_pDst to NULL and the object cannot be use for any
     *          more output afterwards.
     */
    virtual RTCString *finalize() RT_NOEXCEPT;

protected:
    /** Pointer to the destination string.  NULL after finalize().   */
    RTCString  *m_pDst;
    /** Set if we ran out of memory and should ignore subsequent calls. */
    bool        m_fOutOfMemory;

    /* Make non-copyable (RTCNonCopyable causes warnings): */
    RTCRestOutputPrettyToString(RTCRestOutputToString const &);
    RTCRestOutputPrettyToString *operator=(RTCRestOutputToString const &);
};



/** @} */

#endif /* !IPRT_INCLUDED_cpp_restoutput_h */

