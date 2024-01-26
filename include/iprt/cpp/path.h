/** @file
 * IPRT - C++ path utilities.
 */

/*
 * Copyright (C) 2017-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_cpp_path_h
#define IPRT_INCLUDED_cpp_path_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/assert.h>
#include <iprt/errcore.h>
#include <iprt/path.h>
#include <iprt/cpp/ministring.h>


/** @defgroup grp_rt_cpp_path    C++ Path Utilities
 * @ingroup grp_rt_cpp
 * @{
 */

/**
 * RTPathAbs() wrapper for working directly on a RTCString instance.
 *
 * @returns IPRT status code.
 * @param   rStrAbs         Reference to the destination string.
 * @param   pszRelative     The relative source string.
 */
DECLINLINE(int) RTPathAbsCxx(RTCString &rStrAbs, const char *pszRelative)
{
    Assert(rStrAbs.c_str() != pszRelative);
    int rc = rStrAbs.reserveNoThrow(RTPATH_MAX);
    if (RT_SUCCESS(rc))
    {
        unsigned cTries = 8;
        for (;;)
        {
            char *pszDst = rStrAbs.mutableRaw();
            size_t cbCap = rStrAbs.capacity();
            rc = RTPathAbsEx(NULL, pszRelative, RTPATH_STR_F_STYLE_HOST, pszDst, &cbCap);
            if (RT_SUCCESS(rc))
                break;
            *pszDst = '\0';
            if (rc != VERR_BUFFER_OVERFLOW)
                break;
            if (--cTries == 0)
                break;
            rc = rStrAbs.reserveNoThrow(RT_MIN(RT_ALIGN_Z(cbCap, 64), RTPATH_MAX));
            if (RT_FAILURE(rc))
                break;
        }
        rStrAbs.jolt();
    }
    return rc;
}


/**
 * RTPathAbs() wrapper for working directly on a RTCString instance.
 *
 * @returns IPRT status code.
 * @param   rStrAbs         Reference to the destination string.
 * @param   rStrRelative    Reference to the relative source string.
 */
DECLINLINE(int) RTPathAbsCxx(RTCString &rStrAbs, RTCString const &rStrRelative)
{
    return RTPathAbsCxx(rStrAbs, rStrRelative.c_str());
}



/**
 * RTPathAbsEx() wrapper for working directly on a RTCString instance.
 *
 * @returns IPRT status code.
 * @param   rStrAbs         Reference to the destination string.
 * @param   pszBase         The base path, optional.
 * @param   pszRelative     The relative source string.
 * @param   fFlags          RTPATH_STR_F_STYLE_XXX and RTPATHABS_F_XXX flags.
 */
DECLINLINE(int) RTPathAbsExCxx(RTCString &rStrAbs, const char *pszBase, const char *pszRelative, uint32_t fFlags = RTPATH_STR_F_STYLE_HOST)
{
    Assert(rStrAbs.c_str() != pszRelative);
    int rc = rStrAbs.reserveNoThrow(RTPATH_MAX);
    if (RT_SUCCESS(rc))
    {
        unsigned cTries = 8;
        for (;;)
        {
            char *pszDst = rStrAbs.mutableRaw();
            size_t cbCap = rStrAbs.capacity();
            rc = RTPathAbsEx(pszBase, pszRelative, fFlags, pszDst, &cbCap);
            if (RT_SUCCESS(rc))
                break;
            *pszDst = '\0';
            if (rc != VERR_BUFFER_OVERFLOW)
                break;
            if (--cTries == 0)
                break;
            rc = rStrAbs.reserveNoThrow(RT_MIN(RT_ALIGN_Z(cbCap, 64), RTPATH_MAX));
            if (RT_FAILURE(rc))
                break;
        }
        rStrAbs.jolt();
    }
    return rc;
}


DECLINLINE(int) RTPathAbsExCxx(RTCString &rStrAbs, RTCString const &rStrBase, RTCString const &rStrRelative, uint32_t fFlags = RTPATH_STR_F_STYLE_HOST)
{
    return RTPathAbsExCxx(rStrAbs, rStrBase.c_str(), rStrRelative.c_str(), fFlags);
}


DECLINLINE(int) RTPathAbsExCxx(RTCString &rStrAbs, const char *pszBase, RTCString const &rStrRelative, uint32_t fFlags = RTPATH_STR_F_STYLE_HOST)
{
    return RTPathAbsExCxx(rStrAbs, pszBase, rStrRelative.c_str(), fFlags);
}


DECLINLINE(int) RTPathAbsExCxx(RTCString &rStrAbs, RTCString const &rStrBase, const char *pszRelative, uint32_t fFlags = RTPATH_STR_F_STYLE_HOST)
{
    return RTPathAbsExCxx(rStrAbs, rStrBase.c_str(), pszRelative, fFlags);
}



/**
 * RTPathAppPrivateNoArch() wrapper for working directly on a RTCString instance.
 *
 * @returns IPRT status code.
 * @param   rStrDst         Reference to the destination string.
 */
DECLINLINE(int) RTPathAppPrivateNoArchCxx(RTCString &rStrDst)
{
    int rc = rStrDst.reserveNoThrow(RTPATH_MAX);
    if (RT_SUCCESS(rc))
    {
        char *pszDst = rStrDst.mutableRaw();
        rc = RTPathAppPrivateNoArch(pszDst, rStrDst.capacity());
        if (RT_FAILURE(rc))
            *pszDst = '\0';
        rStrDst.jolt();
    }
    return rc;

}


/**
 * RTPathAppend() wrapper for working directly on a RTCString instance.
 *
 * @returns IPRT status code.
 * @param   rStrDst         Reference to the destination string.
 * @param   pszAppend       One or more components to append to the path already
 *                          present in @a rStrDst.
 */
DECLINLINE(int) RTPathAppendCxx(RTCString &rStrDst, const char *pszAppend)
{
    Assert(rStrDst.c_str() != pszAppend);
    size_t cbEstimate = rStrDst.length() + 1 + strlen(pszAppend) + 1;
    int rc;
    if (rStrDst.capacity() >= cbEstimate)
        rc = VINF_SUCCESS;
    else
        rc = rStrDst.reserveNoThrow(RT_ALIGN_Z(cbEstimate, 8));
    if (RT_SUCCESS(rc))
    {
        rc = RTPathAppend(rStrDst.mutableRaw(), rStrDst.capacity(), pszAppend);
        if (rc == VERR_BUFFER_OVERFLOW)
        {
            rc = rStrDst.reserveNoThrow(RTPATH_MAX);
            if (RT_SUCCESS(rc))
                rc = RTPathAppend(rStrDst.mutableRaw(), rStrDst.capacity(), pszAppend);
        }
        rStrDst.jolt();
    }
    return rc;
}


/**
 * RTPathAppend() wrapper for working directly on a RTCString instance.
 *
 * @returns IPRT status code.
 * @param   rStrDst         Reference to the destination string.
 * @param   rStrAppend      One or more components to append to the path already
 *                          present in @a rStrDst.
 */
DECLINLINE(int) RTPathAppendCxx(RTCString &rStrDst, RTCString const &rStrAppend)
{
    Assert(&rStrDst != &rStrAppend);
    size_t cbEstimate = rStrDst.length() + 1 + rStrAppend.length() + 1;
    int rc;
    if (rStrDst.capacity() >= cbEstimate)
        rc = VINF_SUCCESS;
    else
        rc = rStrDst.reserveNoThrow(RT_ALIGN_Z(cbEstimate, 8));
    if (RT_SUCCESS(rc))
    {
        rc = RTPathAppend(rStrDst.mutableRaw(), rStrDst.capacity(), rStrAppend.c_str());
        if (rc == VERR_BUFFER_OVERFLOW)
        {
            rc = rStrDst.reserveNoThrow(RTPATH_MAX);
            if (RT_SUCCESS(rc))
                rc = RTPathAppend(rStrDst.mutableRaw(), rStrDst.capacity(), rStrAppend.c_str());
        }
        rStrDst.jolt();
    }
    return rc;
}


/** @} */

#endif /* !IPRT_INCLUDED_cpp_path_h */

