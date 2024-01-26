/** @file
 * IPRT - Generic thread-safe list Class.
 */

/*
 * Copyright (C) 2011-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_cpp_mtlist_h
#define IPRT_INCLUDED_cpp_mtlist_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cpp/list.h>
#include <iprt/semaphore.h>

/** @addtogroup grp_rt_cpp_list
 * @{
 */

/**
 * A guard class for thread-safe read/write access.
 */
template <>
class RTCListGuard<true>
{
public:
    RTCListGuard() : m_hRWSem(NIL_RTSEMRW)
    {
#if defined(RT_LOCK_STRICT_ORDER) && defined(IN_RING3)
        RTLOCKVALCLASS hClass;
        int rc = RTLockValidatorClassCreate(&hClass, true /*fAutodidact*/, RT_SRC_POS, "RTCListGuard");
        AssertStmt(RT_SUCCESS(rc), hClass = NIL_RTLOCKVALCLASS);
        rc = RTSemRWCreateEx(&m_hRWSem, 0 /*fFlags*/, hClass, RTLOCKVAL_SUB_CLASS_NONE, NULL /*pszNameFmt*/);
        AssertRC(rc);
#else
        int rc = RTSemRWCreateEx(&m_hRWSem, 0 /*fFlags*/, NIL_RTLOCKVALCLASS, 0, NULL);
        AssertRC(rc);
#endif
    }

    ~RTCListGuard()
    {
        RTSemRWDestroy(m_hRWSem);
        m_hRWSem = NIL_RTSEMRW;
    }

    inline void enterRead() const { int rc = RTSemRWRequestRead(m_hRWSem, RT_INDEFINITE_WAIT); AssertRC(rc); }
    inline void leaveRead() const { int rc = RTSemRWReleaseRead(m_hRWSem); AssertRC(rc); }
    inline void enterWrite()      { int rc = RTSemRWRequestWrite(m_hRWSem, RT_INDEFINITE_WAIT); AssertRC(rc); }
    inline void leaveWrite()      { int rc = RTSemRWReleaseWrite(m_hRWSem); AssertRC(rc); }

    /* Define our own new and delete. */
    RTMEMEF_NEW_AND_DELETE_OPERATORS();

private:
    mutable RTSEMRW m_hRWSem;
};

/**
 * @brief Generic thread-safe list class.
 *
 * RTCMTList is a thread-safe implementation of the list class. It uses a
 * read/write semaphore to serialize the access to the items. Several readers
 * can simultaneous access different or the same item. If one thread is writing
 * to an item, the other accessors are blocked until the write has finished.
 *
 * Although the access is guarded, the user has to make sure the list content
 * is consistent when iterating over the list or doing any other kind of access
 * which makes assumptions about the list content. For a finer control of access
 * restrictions, use your own locking mechanism and the standard list
 * implementation.
 *
 * @see RTCListBase
 */
template <class T, typename ITYPE = typename RTCIf<(sizeof(T) > sizeof(void*)), T*, T>::result>
class RTCMTList : public RTCListBase<T, ITYPE, true>
{
    /* Traits */
    typedef RTCListBase<T, ITYPE, true> BASE;

public:
    /**
     * Creates a new list.
     *
     * This preallocates @a cCapacity elements within the list.
     *
     * @param   cCapacity    The initial capacity the list has.
     * @throws  std::bad_alloc
     */
    RTCMTList(size_t cCapacity = BASE::kDefaultCapacity)
        : BASE(cCapacity) {}

    /* Define our own new and delete. */
    RTMEMEF_NEW_AND_DELETE_OPERATORS();
};

/**
 * Specialized thread-safe list class for using the native type list for
 * unsigned 64-bit values even on a 32-bit host.
 *
 * @see RTCListBase
 */
template <>
class RTCMTList<uint64_t>: public RTCListBase<uint64_t, uint64_t, true>
{
    /* Traits */
    typedef RTCListBase<uint64_t, uint64_t, true> BASE;

public:
    /**
     * Creates a new list.
     *
     * This preallocates @a cCapacity elements within the list.
     *
     * @param   cCapacity    The initial capacity the list has.
     * @throws  std::bad_alloc
     */
    RTCMTList(size_t cCapacity = BASE::kDefaultCapacity)
        : BASE(cCapacity) {}

    /* Define our own new and delete. */
    RTMEMEF_NEW_AND_DELETE_OPERATORS();
};

/**
 * Specialized thread-safe list class for using the native type list for
 * signed 64-bit values even on a 32-bit host.
 *
 * @see RTCListBase
 */
template <>
class RTCMTList<int64_t>: public RTCListBase<int64_t, int64_t, true>
{
    /* Traits */
    typedef RTCListBase<int64_t, int64_t, true> BASE;

public:
    /**
     * Creates a new list.
     *
     * This preallocates @a cCapacity elements within the list.
     *
     * @param   cCapacity    The initial capacity the list has.
     * @throws  std::bad_alloc
     */
    RTCMTList(size_t cCapacity = BASE::kDefaultCapacity)
        : BASE(cCapacity) {}

    /* Define our own new and delete. */
    RTMEMEF_NEW_AND_DELETE_OPERATORS();
};

/** @} */

#endif /* !IPRT_INCLUDED_cpp_mtlist_h */

