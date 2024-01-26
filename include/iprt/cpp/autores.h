/** @file
 * IPRT - C++ Resource Management.
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

#ifndef IPRT_INCLUDED_cpp_autores_h
#define IPRT_INCLUDED_cpp_autores_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>
#include <iprt/assert.h>
#include <iprt/cpp/utils.h>



/** @defgroup grp_rt_cpp_autores    C++ Resource Management
 * @ingroup grp_rt_cpp
 * @{
 */

/**
 * A callable class template which returns the correct value against which an
 * IPRT type must be compared to see if it is invalid.
 *
 * @warning This template *must* be specialised for the types it is to work with.
 */
template <class T>
inline T RTAutoResNil(void)
{
    AssertFatalMsgFailed(("Unspecialized template!\n"));
    return (T)0;
}

/** Specialisation of RTAutoResNil for RTFILE */
template <>
inline RTFILE RTAutoResNil(void)
{
    return NIL_RTFILE;
}

/**
 * A function template which calls the correct destructor for an IPRT type.
 *
 * @warning This template *must* be specialised for the types it is to work with.
 */
template <class T>
inline void RTAutoResDestruct(T a_h)
{
    AssertFatalMsgFailed(("Unspecialized template!\n"));
    NOREF(a_h);
}

/**
 * An auto pointer-type class for resources which take a C-style destructor
 * (RTMemFree() or equivalent).
 *
 * The idea of this class is to manage resources which the current code is
 * responsible for freeing.  By wrapping the resource in an RTCAutoRes, you
 * ensure that the resource will be freed when you leave the scope in which
 * the RTCAutoRes is defined, unless you explicitly release the resource.
 *
 * A typical use case is when a function is allocating a number of resources.
 * If any single allocation fails then all other resources must be freed.  If
 * all allocations succeed, then the resources should be returned to the
 * caller.  By placing all allocated resources in RTCAutoRes containers, you
 * ensure that they will be freed on failure, and only have to take care of
 * releasing them when you return them.
 *
 * @param   T           The type of the resource.
 * @param   Destruct    The function to be used to free the resource.
 *                      This parameter must be supplied if there is no
 *                      specialisation of RTAutoDestruct available for @a T.
 * @param   NilRes      The function returning the NIL value for T. Required.
 *                      This parameter must be supplied if there is no
 *                      specialisation of RTAutoResNil available for @a T.
 *
 * @note    The class can not be initialised directly using assignment, due
 *          to the lack of a copy constructor. This is intentional.
 */
template <class T, void Destruct(T) = RTAutoResDestruct<T>, T NilRes(void) = RTAutoResNil<T> >
class RTCAutoRes
    : public RTCNonCopyable
{
protected:
    /** The resource handle. */
    T m_hRes;

public:
    /**
     * Constructor
     *
     * @param   a_hRes      The handle to resource to manage. Defaults to NIL.
     */
    RTCAutoRes(T a_hRes = NilRes())
        : m_hRes(a_hRes)
    {
    }

    /**
     * Destructor.
     *
     * This destroys any resource currently managed by the object.
     */
    ~RTCAutoRes()
    {
        if (m_hRes != NilRes())
            Destruct(m_hRes);
    }

    /**
     * Assignment from a value.
     *
     * This destroys any resource currently managed by the object
     * before taking on the new one.
     *
     * @param   a_hRes      The handle to the new resource.
     */
    RTCAutoRes &operator=(T a_hRes)
    {
        if (m_hRes != NilRes())
            Destruct(m_hRes);
        m_hRes = a_hRes;
        return *this;
    }

    /**
     * Checks if the resource handle is NIL or not.
     */
    bool operator!()
    {
        return m_hRes == NilRes();
    }

    /**
     * Give up ownership the current resource, handing it to the caller.
     *
     * @returns The current resource handle.
     *
     * @note    Nothing happens to the resource when the object goes out of scope.
     */
    T release(void)
    {
        T Tmp = m_hRes;
        m_hRes = NilRes();
        return Tmp;
    }

    /**
     * Deletes the current resources.
     *
     * @param   a_hRes      Handle to a new resource to manage. Defaults to NIL.
     */
    void reset(T a_hRes = NilRes())
    {
        if (a_hRes != m_hRes)
        {
            if (m_hRes != NilRes())
                Destruct(m_hRes);
            m_hRes = a_hRes;
        }
    }

    /**
     * Get the raw resource handle.
     *
     * Typically used passing the handle to some IPRT function while
     * the object remains in scope.
     *
     * @returns The raw resource handle.
     */
    T get(void)
    {
        return m_hRes;
    }
};

/** @} */


/* include after template definition */
#include <iprt/mem.h>

#endif /* !IPRT_INCLUDED_cpp_autores_h */

