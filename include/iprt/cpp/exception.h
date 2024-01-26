/** @file
 * IPRT - C++ Base Exceptions.
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

#ifndef IPRT_INCLUDED_cpp_exception_h
#define IPRT_INCLUDED_cpp_exception_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cpp/ministring.h>
#include <exception>

#if RT_MSC_PREREQ(RT_MSC_VER_VC140)
# pragma warning(push)
# pragma warning(disable:4275) /* non dll-interface class 'std::exception' used as base for dll-interface class 'RTCError' */
#endif


/** @defgroup grp_rt_cpp_exceptions     C++ Exceptions
 * @ingroup grp_rt_cpp
 * @{
 */

/**
 * Base exception class for IPRT, derived from std::exception.
 * The XML exceptions are based on this.
 */
class RT_DECL_CLASS RTCError
    : public std::exception
{
public:

    RTCError(const char *pszMessage)
        : m_strMsg(pszMessage)
    {
    }

    RTCError(const RTCString &a_rstrMessage)
        : m_strMsg(a_rstrMessage)
    {
    }

    RTCError(const RTCError &a_rSrc)
        : std::exception(a_rSrc),
          m_strMsg(a_rSrc.what())
    {
    }

    virtual ~RTCError() throw()
    {
    }

    void operator=(const RTCError &a_rSrc)
    {
        m_strMsg = a_rSrc.what();
    }

    void setWhat(const char *a_pszMessage)
    {
        m_strMsg = a_pszMessage;
    }

    virtual const char *what() const throw()
    {
        return m_strMsg.c_str();
    }

private:
    /**
     * Hidden default constructor making sure that the extended one above is
     * always used.
     */
    RTCError();

protected:
    /** The exception message. */
    RTCString m_strMsg;
};

/** @} */

#if RT_MSC_PREREQ(RT_MSC_VER_VC140)
# pragma warning(pop)
#endif
#endif /* !IPRT_INCLUDED_cpp_exception_h */

