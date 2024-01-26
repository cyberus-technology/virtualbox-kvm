/** @file
 * IPRT - Generic List Class.
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

#ifndef IPRT_INCLUDED_cpp_list_h
#define IPRT_INCLUDED_cpp_list_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cpp/meta.h>
#include <iprt/mem.h>
#include <iprt/string.h> /* for memcpy */
#include <iprt/assert.h>

#include <new> /* For std::bad_alloc */

/** @defgroup grp_rt_cpp_list   C++ List support
 * @ingroup grp_rt_cpp
 *
 * @brief  Generic C++ list class support.
 *
 * This list classes manage any amount of data in a fast and easy to use way.
 * They have no dependencies on STL, only on generic memory management methods
 * of IRPT. This allows list handling in situations where the use of STL
 * container classes is forbidden.
 *
 * Not all of the functionality of STL container classes is implemented. There
 * are no iterators or any other high level access/modifier methods (e.g.
 * std::algorithms).
 *
 * The implementation is array based which allows fast access to the items.
 * Appending items is usually also fast, cause the internal array is
 * preallocated. To minimize the memory overhead, native types (that is
 * everything smaller then the size of void*) are directly saved in the array.
 * If bigger types are used (e.g. RTCString) the internal array is an array of
 * pointers to the objects.
 *
 * The size of the internal array will usually not shrink, but grow
 * automatically. Only certain methods, like RTCList::clear or the "=" operator
 * will reset any previously allocated memory. You can call
 * RTCList::setCapacity for manual adjustment. If the size of an new list will
 * be known, calling the constructor with the necessary capacity will speed up
 * the insertion of the new items.
 *
 * For the full public interface these list classes offer see RTCListBase.
 *
 * There are some requirements for the types used which follow:
 * -# They need a default and a copy constructor.
 * -# Some methods (e.g. RTCList::contains) need an equal operator.
 * -# If the type is some complex class (that is, having a constructor which
 *    allocates members on the heap) it has to be greater than sizeof(void*) to
 *    be used correctly. If this is not the case you can manually overwrite the
 *    list behavior. Just add T* as a second parameter to the list template if
 *    your class is called T. Another possibility is to specialize the list for
 *    your target class. See below for more information.
 *
 * The native types like int, bool, ptr, ..., are meeting this criteria, so
 * they are save to use.
 *
 * Please note that the return type of some of the getter methods are slightly
 * different depending on the list type. Native types return the item by value,
 * items with a size greater than sizeof(void*) by reference. As native types
 * saved directly in the internal array, returning a reference to them (and
 * saving them in a reference as well) would make them invalid (or pointing to
 * a wrong item) when the list is changed in the meanwhile. Returning a
 * reference for bigger types isn't problematic and makes sure we get out the
 * best speed of the list. The one exception to this rule is the index
 * operator[]. This operator always return a reference to make it possible to
 * use it as a lvalue. Its your responsibility to make sure the list isn't
 * changed when using the value as reference returned by this operator.
 *
 * The list class is reentrant. For a thread-safe variant see RTCMTList.
 *
 * Implementation details:
 * It is possible to specialize any type. This might be necessary to get the
 * best speed out of the list. Examples are the 64-bit types, which use the
 * native (no pointers) implementation even on a 32-bit host. Consult the
 * source code for more details.
 *
 * Current specialized implementations:
 * - int64_t: RTCList<int64_t>
 * - uint64_t: RTCList<uint64_t>
 *
 * @{
 */

/**
 * The guard definition.
 */
template <bool G>
class RTCListGuard;

/**
 * The default guard which does nothing.
 */
template <>
class RTCListGuard<false>
{
public:
    inline void enterRead() const {}
    inline void leaveRead() const {}
    inline void enterWrite() {}
    inline void leaveWrite() {}

    /* Define our own new and delete. */
#ifdef RT_NEED_NEW_AND_DELETE
    RTMEM_IMPLEMENT_NEW_AND_DELETE();
#else
    RTMEMEF_NEW_AND_DELETE_OPERATORS();
#endif
};

/**
 * General helper template for managing native values in RTCListBase.
 */
template <typename T1, typename T2>
class RTCListHelper
{
public:
    static inline void      set(T2 *p, size_t i, const T1 &v) { p[i] = v; }
    static inline T1 &      at(T2 *p, size_t i) { return p[i]; }
    static inline const T1 &atConst(T2 const *p, size_t i) { return p[i]; }
    static inline size_t    find(T2 *p, const T1 &v, size_t cElements)
    {
        size_t i = cElements;
        while (i-- > 0)
            if (p[i] == v)
                return i;
        return cElements;
    }
    static inline void      copyTo(T2 *p, T2 *const p1 , size_t iTo, size_t cSize)
    {
        if (cSize > 0)
            memcpy(&p[iTo], &p1[0], sizeof(T1) * cSize);
    }
    static inline void      erase(T2 * /* p */, size_t /* i */) { /* Nothing to do here. */ }
    static inline void      eraseRange(T2 * /* p */, size_t /* cFrom */, size_t /* cSize */) { /* Nothing to do here. */ }
};

/**
 * Specialized helper template for managing pointer values in RTCListBase.
 */
template <typename T1>
class RTCListHelper<T1, T1*>
{
public:
    static inline void      set(T1 **p, size_t i, const T1 &v) { p[i] = new T1(v); }
    static inline T1 &      at(T1 **p, size_t i) { return *p[i]; }
    static inline const T1 &atConst(T1 * const *p, size_t i) { return *p[i]; }
    static inline size_t    find(T1 **p, const T1 &v, size_t cElements)
    {
        size_t i = cElements;
        while (i-- > 0)
            if (*p[i] == v)
                return i;
        return cElements;
    }
    static inline void      copyTo(T1 **p, T1 **const p1 , size_t iTo, size_t cSize)
    {
        for (size_t i = 0; i < cSize; ++i)
            p[iTo + i] = new T1(*p1[i]);
    }
    static inline void      erase(T1 **p, size_t i) { delete p[i]; }
    static inline void      eraseRange(T1 **p, size_t iFrom, size_t cItems)
    {
        while (cItems-- > 0)
            delete p[iFrom++];
    }
};

/**
 * This is the base class for all other list classes. It implements the
 * necessary list functionality in a type independent way and offers the public
 * list interface to the user.
 */
template <class T, typename ITYPE, bool MT>
class RTCListBase
{
    /** @name Traits.
     *
     * Defines the return type of most of the getter methods. If the internal
     * used type is a pointer, we return a reference. If not we return by
     * value.
     *
     * @{
     */
    typedef typename RTCIfPtr<ITYPE, T&, T>::result GET_RTYPE;
    typedef typename RTCIfPtr<ITYPE, const T&, T>::result GET_CRTYPE;
    /** @}  */

public:
    /**
     * Creates a new list.
     *
     * This preallocates @a cCapacity elements within the list.
     *
     * @param   cCapacity    The initial capacity the list has.
     * @throws  std::bad_alloc
     */
    RTCListBase(size_t cCapacity = kDefaultCapacity)
        : m_pArray(0)
        , m_cElements(0)
        , m_cCapacity(0)
    {
        if (cCapacity > 0)
            growArray(cCapacity);
    }

    /**
     * Creates a copy of another list.
     *
     * The other list will be fully copied and the capacity will be the same as
     * the size of the other list.
     *
     * @param   other          The list to copy.
     * @throws  std::bad_alloc
     */
    RTCListBase(const RTCListBase<T, ITYPE, MT>& other)
        : m_pArray(0)
        , m_cElements(0)
        , m_cCapacity(0)
    {
        other.m_guard.enterRead();

        size_t const cElementsOther = other.m_cElements;
        resizeArrayNoErase(cElementsOther);
        RTCListHelper<T, ITYPE>::copyTo(m_pArray, other.m_pArray, 0, cElementsOther);
        m_cElements = cElementsOther;

        other.m_guard.leaveRead();
    }

    /**
     * Destructor.
     */
    ~RTCListBase()
    {
        RTCListHelper<T, ITYPE>::eraseRange(m_pArray, 0, m_cElements);
        if (m_pArray)
        {
            RTMemFree(m_pArray);
            m_pArray = NULL;
        }
        m_cElements = m_cCapacity = 0;
    }

    /**
     * Sets a new capacity within the list.
     *
     * If the new capacity is bigger than the old size, it will be simply
     * preallocated more space for the new items.  If the new capacity is
     * smaller than the previous size, items at the end of the list will be
     * deleted.
     *
     * @param   cCapacity   The new capacity within the list.
     * @throws  std::bad_alloc
     */
    void setCapacity(size_t cCapacity)
    {
        m_guard.enterWrite();
        resizeArray(cCapacity);
        m_guard.leaveWrite();
    }

    /**
     * Return the current capacity of the list.
     *
     * @return   The actual capacity.
     */
    size_t capacity() const
    {
        m_guard.enterRead();
        size_t cRet = m_cCapacity;
        m_guard.leaveRead();
        return cRet;
    }

    /**
     * Check if an list contains any items.
     *
     * @return   True if there is more than zero items, false otherwise.
     */
    bool isEmpty() const
    {
        m_guard.enterRead();
        bool fEmpty = m_cElements == 0;
        m_guard.leaveRead();
        return fEmpty;
    }

    /**
     * Return the current count of elements within the list.
     *
     * @return   The current element count.
     */
    size_t size() const
    {
        m_guard.enterRead();
        size_t cRet = m_cElements;
        m_guard.leaveRead();
        return cRet;
    }

    /**
     * Inserts an item to the list at position @a i.
     *
     * @param   i     The position of the new item.  The must be within or at the
     *                exact end of the list.  Indexes specified beyond the end of
     *                the list will be changed to an append() operation and strict
     *                builds will raise an assert.
     * @param   val   The new item.
     * @return  a reference to this list.
     * @throws  std::bad_alloc
     */
    RTCListBase<T, ITYPE, MT> &insert(size_t i, const T &val)
    {
        m_guard.enterWrite();

        AssertMsgStmt(i <= m_cElements, ("i=%zu m_cElements=%zu\n", i, m_cElements), i = m_cElements);

        if (m_cElements == m_cCapacity)
            growArray(m_cCapacity + kDefaultCapacity);

        memmove(&m_pArray[i + 1], &m_pArray[i], (m_cElements - i) * sizeof(ITYPE));
        RTCListHelper<T, ITYPE>::set(m_pArray, i, val);
        ++m_cElements;

        m_guard.leaveWrite();
        return *this;
    }

    /**
     * Inserts a list to the list at position @a i.
     *
     * @param   i       The position of the new item.  The must be within or at the
     *                  exact end of the list.  Indexes specified beyond the end of
     *                  the list will be changed to an append() operation and strict
     *                  builds will raise an assert.
     * @param   other   The other list. This MUST not be the same as the destination
     *                  list, will assert and return without doing anything if this
     *                  happens.
     * @return  a reference to this list.
     * @throws  std::bad_alloc
     */
    RTCListBase<T, ITYPE, MT> &insert(size_t i, const RTCListBase<T, ITYPE, MT> &other)
    {
        AssertReturn(this != &other, *this);

        other.m_guard.enterRead();
        m_guard.enterWrite();

        AssertMsgStmt(i <= m_cElements, ("i=%zu m_cElements=%zu\n", i, m_cElements), i = m_cElements);

        size_t cElementsOther = other.m_cElements;
        if (RT_LIKELY(cElementsOther > 0))
        {
            if (m_cCapacity - m_cElements < cElementsOther)
                growArray(m_cCapacity + (cElementsOther - (m_cCapacity - m_cElements)));
            if (i < m_cElements)
                memmove(&m_pArray[i + cElementsOther], &m_pArray[i], (m_cElements - i) * sizeof(ITYPE));

            RTCListHelper<T, ITYPE>::copyTo(&m_pArray[i], other.m_pArray, 0, cElementsOther);
            m_cElements += cElementsOther;
        }

        m_guard.leaveWrite();
        other.m_guard.leaveRead();
        return *this;
    }

    /**
     * Prepend an item to the list.
     *
     * @param   val   The new item.
     * @return  a reference to this list.
     * @throws  std::bad_alloc
     */
    RTCListBase<T, ITYPE, MT> &prepend(const T &val)
    {
        return insert(0, val);
    }

    /**
     * Prepend a list of type T to the list.
     *
     * @param   other   The list to prepend.
     * @return  a reference to this list.
     * @throws  std::bad_alloc
     */
    RTCListBase<T, ITYPE, MT> &prepend(const RTCListBase<T, ITYPE, MT> &other)
    {
        return insert(0, other);
    }

    /**
     * Append a default item to the list.
     *
     * @return  a mutable reference to the item
     * @throws  std::bad_alloc
     */
    GET_RTYPE append()
    {
        m_guard.enterWrite();
        if (m_cElements == m_cCapacity)
            growArray(m_cCapacity + kDefaultCapacity);
        RTCListHelper<T, ITYPE>::set(m_pArray, m_cElements, T());
        GET_RTYPE rRet = RTCListHelper<T, ITYPE>::at(m_pArray, m_cElements);
        ++m_cElements;
        m_guard.leaveWrite();

        return rRet;
    }

    /**
     * Append an item to the list.
     *
     * @param   val   The new item.
     * @return  a reference to this list.
     * @throws  std::bad_alloc
     */
    RTCListBase<T, ITYPE, MT> &append(const T &val)
    {
        m_guard.enterWrite();
        if (m_cElements == m_cCapacity)
            growArray(m_cCapacity + kDefaultCapacity);
        RTCListHelper<T, ITYPE>::set(m_pArray, m_cElements, val);
        ++m_cElements;
        m_guard.leaveWrite();

        return *this;
    }

    /**
     * Append a list of type T to the list.
     *
     * @param   other   The list to append. Must not be the same as the destination
     *                  list, will assert and return without doing anything.
     * @return  a reference to this list.
     * @throws  std::bad_alloc
     */
    RTCListBase<T, ITYPE, MT> &append(const RTCListBase<T, ITYPE, MT> &other)
    {
        AssertReturn(this != &other, *this);

        other.m_guard.enterRead();
        m_guard.enterWrite();

        insert(m_cElements, other);

        m_guard.leaveWrite();
        other.m_guard.leaveRead();
        return *this;
    }

    /**
     * Copy the items of the other list into this list.
     *
     * All previous items of this list are deleted.
     *
     * @param   other   The list to copy.
     * @return  a reference to this list.
     */
    RTCListBase<T, ITYPE, MT> &operator=(const RTCListBase<T, ITYPE, MT>& other)
    {
        /* Prevent self assignment */
        if (RT_LIKELY(this != &other))
        {
            other.m_guard.enterRead();
            m_guard.enterWrite();

            /* Delete all items. */
            RTCListHelper<T, ITYPE>::eraseRange(m_pArray, 0, m_cElements);

            /* Need we to realloc memory. */
            if (other.m_cElements != m_cCapacity)
                resizeArrayNoErase(other.m_cElements);
            m_cElements = other.m_cElements;

            /* Copy new items. */
            RTCListHelper<T, ITYPE>::copyTo(m_pArray, other.m_pArray, 0, other.m_cElements);

            m_guard.leaveWrite();
            other.m_guard.leaveRead();
        }
        return *this;
    }

    /**
     * Compares if this list's items match the other list.
     *
     * @returns \c true if both lists contain the same items, \c false if not.
     * @param   other   The list to compare this list with.
     */
    bool operator==(const RTCListBase<T, ITYPE, MT>& other)
    {
        /* Prevent self comparrison */
        if (RT_LIKELY(this == &other))
            return true;

        other.m_guard.enterRead();
        m_guard.enterRead();

        bool fEqual = true;
        if (other.m_cElements == m_cElements)
        {
            for (size_t i = 0; i < m_cElements; i++)
            {
                if (RTCListHelper<T, ITYPE>::at(m_pArray, i) != RTCListHelper<T, ITYPE>::at(other.m_pArray, i))
                {
                    fEqual = false;
                    break;
                }
            }
        }
        else
            fEqual = false;

        m_guard.leaveRead();
        other.m_guard.leaveRead();

        return fEqual;
    }

    /**
     * Compares if this list's items do not match the other list.
     *
     * @returns \c true if the lists do not match, \c false if otherwise.
     * @param   other   The list to compare this list with.
     */
    bool operator!=(const RTCListBase<T, ITYPE, MT>& other)
    {
        return !(*this == other);
    }

    /**
     * Replace an item in the list.
     *
     * @param   i     The position of the item to replace.  If this is out of range,
     *                the request will be ignored, strict builds will assert.
     * @param   val   The new value.
     * @return  a reference to this list.
     */
    RTCListBase<T, ITYPE, MT> &replace(size_t i, const T &val)
    {
        m_guard.enterWrite();

        if (i < m_cElements)
        {
            RTCListHelper<T, ITYPE>::erase(m_pArray, i);
            RTCListHelper<T, ITYPE>::set(m_pArray, i, val);
        }
        else
            AssertMsgFailed(("i=%zu m_cElements=%zu\n", i, m_cElements));

        m_guard.leaveWrite();
        return *this;
    }

    /**
     * Applies a filter of type T to this list.
     *
     * @param   other           The list which contains the elements to filter out from this list.
     * @return  a reference to this list.
     */
    RTCListBase<T, ITYPE, MT> &filter(const RTCListBase<T, ITYPE, MT> &other)
    {
        AssertReturn(this != &other, *this);

        other.m_guard.enterRead();
        m_guard.enterWrite();

        for (size_t i = 0; i < m_cElements; i++)
        {
            for (size_t f = 0; f < other.m_cElements; f++)
            {
                if (RTCListHelper<T, ITYPE>::at(m_pArray, i) == RTCListHelper<T, ITYPE>::at(other.m_pArray, f))
                    removeAtLocked(i);
            }
        }

        m_guard.leaveWrite();
        other.m_guard.leaveRead();
        return *this;
    }

    /**
     * Return the first item as constant object.
     *
     * @return   A reference or pointer to the first item.
     *
     * @note     No boundary checks are done. Make sure there is at least one
     *           element.
     */
    GET_CRTYPE first() const
    {
        m_guard.enterRead();
        Assert(m_cElements > 0);
        GET_CRTYPE res = RTCListHelper<T, ITYPE>::at(m_pArray, 0);
        m_guard.leaveRead();
        return res;
    }

    /**
     * Return the first item.
     *
     * @return   A reference or pointer to the first item.
     *
     * @note     No boundary checks are done. Make sure there is at least one
     *           element.
     */
    GET_RTYPE first()
    {
        m_guard.enterRead();
        Assert(m_cElements > 0);
        GET_RTYPE res = RTCListHelper<T, ITYPE>::at(m_pArray, 0);
        m_guard.leaveRead();
        return res;
    }

    /**
     * Return the last item as constant object.
     *
     * @return   A reference or pointer to the last item.
     *
     * @note     No boundary checks are done. Make sure there is at least one
     *           element.
     */
    GET_CRTYPE last() const
    {
        m_guard.enterRead();
        Assert(m_cElements > 0);
        GET_CRTYPE res = RTCListHelper<T, ITYPE>::at(m_pArray, m_cElements - 1);
        m_guard.leaveRead();
        return res;
    }

    /**
     * Return the last item.
     *
     * @return   A reference or pointer to the last item.
     *
     * @note     No boundary checks are done. Make sure there is at least one
     *           element.
     */
    GET_RTYPE last()
    {
        m_guard.enterRead();
        Assert(m_cElements > 0);
        GET_RTYPE res = RTCListHelper<T, ITYPE>::at(m_pArray, m_cElements - 1);
        m_guard.leaveRead();
        return res;
    }

    /**
     * Return the item at position @a i as constant object.
     *
     * @param   i     The position of the item to return.  This better not be out of
     *                bounds, however should it be the last element of the array
     *                will be return and strict builds will raise an assertion.
     *                Should the array be empty, a crash is very likely.
     * @return  The item at position @a i.
     */
    GET_CRTYPE at(size_t i) const
    {
        m_guard.enterRead();
        AssertMsgStmt(i < m_cElements, ("i=%zu m_cElements=%zu\n", i, m_cElements), i = m_cElements - 1);
        GET_CRTYPE res = RTCListHelper<T, ITYPE>::at(m_pArray, i);
        m_guard.leaveRead();
        return res;
    }

    /**
     * Return the item at position @a i.
     *
     * @param   i     The position of the item to return.  This better not be out of
     *                bounds, however should it be the last element of the array
     *                will be return and strict builds will raise an assertion.
     *                Should the array be empty, a crash is very likely.
     * @return   The item at position @a i.
     */
    GET_RTYPE at(size_t i)
    {
        m_guard.enterRead();
        AssertMsgStmt(i < m_cElements, ("i=%zu m_cElements=%zu\n", i, m_cElements), i = m_cElements - 1);
        GET_RTYPE res = RTCListHelper<T, ITYPE>::at(m_pArray, i);
        m_guard.leaveRead();
        return res;
    }

    /**
     * Return the item at position @a i as mutable reference.
     *
     * @param   i     The position of the item to return.  This better not be out of
     *                bounds, however should it be the last element of the array
     *                will be return and strict builds will raise an assertion.
     *                Should the array be empty, a crash is very likely.
     * @return   The item at position @a i.
     */
    T &operator[](size_t i)
    {
        m_guard.enterRead();
        AssertMsgStmt(i < m_cElements, ("i=%zu m_cElements=%zu\n", i, m_cElements), i = m_cElements - 1);
        T &res = RTCListHelper<T, ITYPE>::at(m_pArray, i);
        m_guard.leaveRead();
        return res;
    }

    /**
     * Return the item at position @a i as immutable reference.
     *
     * @param   i     The position of the item to return.  This better not be out of
     *                bounds, however should it be the last element of the array
     *                will be return and strict builds will raise an assertion.
     *                Should the array be empty, a crash is very likely.
     * @return   The item at position @a i.
     */
    const T &operator[](size_t i) const
    {
        m_guard.enterRead();
        AssertMsgStmt(i < m_cElements, ("i=%zu m_cElements=%zu\n", i, m_cElements), i = m_cElements - 1);
        const T &rRet = RTCListHelper<T, ITYPE>::atConst(m_pArray, i);
        m_guard.leaveRead();
        return rRet;
    }

    /**
     * Return a copy of the item at position @a i or default value if out of range.
     *
     * @param   i              The position of the item to return.
     * @return  Copy of the item at position @a i or default value.
     */
    T value(size_t i) const
    {
        m_guard.enterRead();
        if (RT_LIKELY(i < m_cElements))
        {
            T res = RTCListHelper<T, ITYPE>::at(m_pArray, i);
            m_guard.leaveRead();
            return res;
        }
        m_guard.leaveRead();
        return T();
    }

    /**
     * Return a copy of the item at position @a i, or @a defaultVal if out of range.
     *
     * @param   i              The position of the item to return.
     * @param   defaultVal     The value to return in case @a i is invalid.
     * @return  Copy of the item at position @a i or @a defaultVal.
     */
    T value(size_t i, const T &defaultVal) const
    {
        m_guard.enterRead();
        if (RT_LIKELY(i < m_cElements))
        {
            T res = RTCListHelper<T, ITYPE>::at(m_pArray, i);
            m_guard.leaveRead();
            return res;
        }
        m_guard.leaveRead();
        return defaultVal;
    }

    /**
     * Check if @a val is contained in the array.
     *
     * @param   val     The value to check for.
     * @return  true if it is found, false otherwise.
     */
    bool contains(const T &val) const
    {
        m_guard.enterRead();
        bool fRc = RTCListHelper<T, ITYPE>::find(m_pArray, val, m_cElements) < m_cElements;
        m_guard.leaveRead();
        return fRc;
    }

    /**
     * Remove the first item.
     *
     * @note You should make sure the list isn't empty. Strict builds will assert.
     *       The other builds will quietly ignore the request.
     */
    void removeFirst()
    {
        removeAt(0);
    }

    /**
     * Remove the last item.
     *
     * @note You should make sure the list isn't empty. Strict builds will assert.
     *       The other builds will quietly ignore the request.
     */
    void removeLast()
    {
        m_guard.enterWrite();
        removeAtLocked(m_cElements - 1);
        m_guard.leaveWrite();
    }

    /**
     * Remove the item at position @a i.
     *
     * @param   i   The position of the item to remove.  Out of bounds values will
     *              be ignored and an assertion will be raised in strict builds.
     */
    void removeAt(size_t i)
    {
        m_guard.enterWrite();
        removeAtLocked(i);
        m_guard.leaveWrite();
    }

    /**
     * Remove a range of items from the list.
     *
     * @param   iStart  The start position of the items to remove.
     * @param   iEnd    The end position of the items to remove (excluded).
     */
    void removeRange(size_t iStart, size_t iEnd)
    {
        AssertReturnVoid(iStart <= iEnd);
        m_guard.enterWrite();

        AssertMsgStmt(iEnd   <= m_cElements, ("iEnd=%zu m_cElements=%zu\n",   iEnd,   m_cElements), iEnd   = m_cElements);
        AssertMsgStmt(iStart <  m_cElements, ("iStart=%zu m_cElements=%zu\n", iStart, m_cElements), iStart = m_cElements);
        size_t const cElements = iEnd - iStart;
        if (cElements > 0)
        {
            Assert(iStart < m_cElements);
            RTCListHelper<T, ITYPE>::eraseRange(m_pArray, iStart, cElements);
            if (m_cElements > iEnd)
                memmove(&m_pArray[iStart], &m_pArray[iEnd], (m_cElements - iEnd) * sizeof(ITYPE));
            m_cElements -= cElements;
        }

        m_guard.leaveWrite();
    }

    /**
     * Delete all items in the list.
     */
    void clear()
    {
        m_guard.enterWrite();

        /* Values cleanup */
        RTCListHelper<T, ITYPE>::eraseRange(m_pArray, 0, m_cElements);
        if (m_cElements != kDefaultCapacity)
            resizeArrayNoErase(kDefaultCapacity);
        m_cElements = 0;

        m_guard.leaveWrite();
    }

    /**
     * Return the raw array.
     *
     * For native types this is a pointer to continuous memory of the items. For
     * pointer types this is a continuous memory of pointers to the items.
     *
     * @warning If you change anything in the underlaying list, this memory
     *          will very likely become invalid. So take care when using this
     *          method and better try to avoid using it.
     *
     * @returns the raw memory.
     */
    ITYPE *raw() const
    {
        m_guard.enterRead();
        ITYPE *pRet = m_pArray;
        m_guard.leaveRead();
        return pRet;
    }

    RTCListBase<T, ITYPE, MT> &operator<<(const T &val)
    {
        return append(val);
    }

    /* Define our own new and delete. */
#ifdef RT_NEED_NEW_AND_DELETE
    RTMEM_IMPLEMENT_NEW_AND_DELETE();
#else
    RTMEMEF_NEW_AND_DELETE_OPERATORS();
#endif

    /**
     * The default capacity of the list. This is also used as grow factor.
     */
    static const size_t kDefaultCapacity;

protected:

    /**
     * Generic resizes the array, surplus elements are erased.
     *
     * @param   cElementsNew    The new array size.
     * @throws  std::bad_alloc.
     */
    void resizeArray(size_t cElementsNew)
    {
        /* Same size? */
        if (cElementsNew == m_cCapacity)
            return;

        /* If we get smaller we have to delete some of the objects at the end
           of the list. */
        if (   cElementsNew < m_cElements
            && m_pArray)
            RTCListHelper<T, ITYPE>::eraseRange(m_pArray, cElementsNew, m_cElements - cElementsNew);

        resizeArrayNoErase(cElementsNew);
    }

    /**
     * Resizes the array without doing the erase() thing on surplus elements.
     *
     * @param   cElementsNew    The new array size.
     * @throws  std::bad_alloc.
     */
    void resizeArrayNoErase(size_t cElementsNew)
    {
        /* Same size? */
        if (cElementsNew == m_cCapacity)
            return;

        /* Resize the array. */
        if (cElementsNew > 0)
        {
            void *pvNew = RTMemRealloc(m_pArray, sizeof(ITYPE) * cElementsNew);
            if (!pvNew)
            {
#ifdef RT_EXCEPTIONS_ENABLED
                throw std::bad_alloc();
#endif
                return;
            }
            m_pArray = static_cast<ITYPE*>(pvNew);
        }
        /* If we get zero we delete the array it self. */
        else if (m_pArray)
        {
            RTMemFree(m_pArray);
            m_pArray = NULL;
        }

        m_cCapacity = cElementsNew;
        if (m_cElements > cElementsNew)
            m_cElements = cElementsNew;
    }

    /**
     * Special realloc method which require that the array will grow.
     *
     * @param   cElementsNew    The new array size.
     * @throws  std::bad_alloc.
     * @note No boundary checks are done!
     */
    void growArray(size_t cElementsNew)
    {
        Assert(cElementsNew > m_cCapacity);
        void *pvNew = RTMemRealloc(m_pArray, sizeof(ITYPE) * cElementsNew);
        if (pvNew)
        {
            m_cCapacity = cElementsNew;
            m_pArray = static_cast<ITYPE*>(pvNew);
        }
        else
        {
#ifdef RT_EXCEPTIONS_ENABLED
            throw std::bad_alloc();
#endif
        }
    }

    /**
     * Remove the item at position @a i.
     *
     * @param   i   The position of the item to remove.  Out of bounds values will
     *              be ignored and an assertion will be raised in strict builds.
     * @remarks
     */
    void removeAtLocked(size_t i)
    {
        AssertMsgReturnVoid(i < m_cElements, ("i=%zu m_cElements=%zu\n", i, m_cElements));

        RTCListHelper<T, ITYPE>::erase(m_pArray, i);
        if (i < m_cElements - 1)
            memmove(&m_pArray[i], &m_pArray[i + 1], (m_cElements - i - 1) * sizeof(ITYPE));
        --m_cElements;
    }


    /** The internal list array. */
    ITYPE *m_pArray;
    /** The current count of items in use. */
    size_t m_cElements;
    /** The current capacity of the internal array. */
    size_t m_cCapacity;
    /** The guard used to serialize the access to the items. */
    RTCListGuard<MT> m_guard;
};

template <class T, typename ITYPE, bool MT>
const size_t RTCListBase<T, ITYPE, MT>::kDefaultCapacity = 10;

/**
 * Template class which automatically determines the type of list to use.
 *
 * @see RTCListBase
 */
template <class T, typename ITYPE = typename RTCIf<(sizeof(T) > sizeof(void*)), T*, T>::result>
class RTCList : public RTCListBase<T, ITYPE, false>
{
    /* Traits */
    typedef RTCListBase<T, ITYPE, false> BASE;

public:
    /**
     * Creates a new list.
     *
     * This preallocates @a cCapacity elements within the list.
     *
     * @param   cCapacity    The initial capacity the list has.
     * @throws  std::bad_alloc
     */
    RTCList(size_t cCapacity = BASE::kDefaultCapacity)
        : BASE(cCapacity) {}

    RTCList(const BASE &other)
        : BASE(other) {}

    /* Define our own new and delete. */
#ifdef RT_NEED_NEW_AND_DELETE
    RTMEM_IMPLEMENT_NEW_AND_DELETE();
#else
    RTMEMEF_NEW_AND_DELETE_OPERATORS();
#endif
};

/**
 * Specialized class for using the native type list for unsigned 64-bit
 * values even on a 32-bit host.
 *
 * @see RTCListBase
 */
template <>
class RTCList<uint64_t>: public RTCListBase<uint64_t, uint64_t, false>
{
    /* Traits */
    typedef RTCListBase<uint64_t, uint64_t, false> BASE;

public:
    /**
     * Creates a new list.
     *
     * This preallocates @a cCapacity elements within the list.
     *
     * @param   cCapacity    The initial capacity the list has.
     * @throws  std::bad_alloc
     */
    RTCList(size_t cCapacity = BASE::kDefaultCapacity)
        : BASE(cCapacity) {}

    /* Define our own new and delete. */
#ifdef RT_NEED_NEW_AND_DELETE
    RTMEM_IMPLEMENT_NEW_AND_DELETE();
#else
    RTMEMEF_NEW_AND_DELETE_OPERATORS();
#endif
};

/**
 * Specialized class for using the native type list for signed 64-bit
 * values even on a 32-bit host.
 *
 * @see RTCListBase
 */
template <>
class RTCList<int64_t>: public RTCListBase<int64_t, int64_t, false>
{
    /* Traits */
    typedef RTCListBase<int64_t, int64_t, false> BASE;

public:
    /**
     * Creates a new list.
     *
     * This preallocates @a cCapacity elements within the list.
     *
     * @param   cCapacity    The initial capacity the list has.
     * @throws  std::bad_alloc
     */
    RTCList(size_t cCapacity = BASE::kDefaultCapacity)
        : BASE(cCapacity) {}

    /* Define our own new and delete. */
#ifdef RT_NEED_NEW_AND_DELETE
    RTMEM_IMPLEMENT_NEW_AND_DELETE();
#else
    RTMEMEF_NEW_AND_DELETE_OPERATORS();
#endif
};

/** @} */

#endif /* !IPRT_INCLUDED_cpp_list_h */

