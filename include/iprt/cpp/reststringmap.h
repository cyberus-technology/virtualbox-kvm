/** @file
 * IPRT - C++ Representational State Transfer (REST) String Map Template.
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

#ifndef IPRT_INCLUDED_cpp_reststringmap_h
#define IPRT_INCLUDED_cpp_reststringmap_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/list.h>
#include <iprt/string.h>
#include <iprt/cpp/restbase.h>


/** @defgroup grp_rt_cpp_reststingmap   C++ Representational State Transfer (REST) String Map Template
 * @ingroup grp_rt_cpp
 * @{
 */

/**
 * Abstract base class for the RTCRestStringMap template.
 */
class RT_DECL_CLASS RTCRestStringMapBase : public RTCRestObjectBase
{
public:
    /** Default destructor. */
    RTCRestStringMapBase() RT_NOEXCEPT;
    /** Copy constructor. */
    RTCRestStringMapBase(RTCRestStringMapBase const &a_rThat);
    /** Destructor. */
    virtual ~RTCRestStringMapBase();
    /** Copy assignment operator. */
    RTCRestStringMapBase &operator=(RTCRestStringMapBase const &a_rThat);

    /* Overridden methods: */
    virtual RTCRestObjectBase *baseClone() const RT_NOEXCEPT RT_OVERRIDE;
    virtual int resetToDefault() RT_NOEXCEPT RT_OVERRIDE;
    virtual RTCRestOutputBase &serializeAsJson(RTCRestOutputBase &a_rDst) const RT_NOEXCEPT RT_OVERRIDE;
    virtual int deserializeFromJson(RTCRestJsonCursor const &a_rCursor) RT_NOEXCEPT RT_OVERRIDE;
    // later?
    //virtual int toString(RTCString *a_pDst, uint32_t a_fFlags = kCollectionFormat_Unspecified) const RT_NOEXCEPT RT_OVERRIDE;
    //virtual int fromString(RTCString const &a_rValue, const char *a_pszName, PRTERRINFO a_pErrInfo = NULL,
    //                       uint32_t a_fFlags = kCollectionFormat_Unspecified) RT_NOEXCEPT RT_OVERRIDE;
    virtual kTypeClass typeClass(void) const RT_NOEXCEPT RT_OVERRIDE;
    virtual const char *typeName(void) const RT_NOEXCEPT RT_OVERRIDE;

    /**
     * Clear the content of the map.
     */
    void clear() RT_NOEXCEPT;

    /**
     * Checks if the map is empty.
     */
    inline bool isEmpty() const RT_NOEXCEPT { return m_cEntries == 0; }

    /**
     * Gets the number of entries in the map.
     */
    size_t size() const RT_NOEXCEPT;

    /**
     * Checks if the map contains the given key.
     * @returns true if key found, false if not.
     * @param   a_pszKey   The key to check fo.
     */
    bool containsKey(const char *a_pszKey) const RT_NOEXCEPT;

    /**
     * Checks if the map contains the given key.
     * @returns true if key found, false if not.
     * @param   a_rStrKey   The key to check fo.
     */
    bool containsKey(RTCString const &a_rStrKey) const RT_NOEXCEPT;

    /**
     * Remove any key-value pair with the given key.
     * @returns true if anything was removed, false if not found.
     * @param   a_pszKey    The key to remove.
     */
    bool remove(const char *a_pszKey) RT_NOEXCEPT;

    /**
     * Remove any key-value pair with the given key.
     * @returns true if anything was removed, false if not found.
     * @param   a_rStrKey   The key to remove.
     */
    bool remove(RTCString const &a_rStrKey) RT_NOEXCEPT;

    /**
     * Creates a new value and inserts it under the given key, returning the new value.
     *
     * @returns VINF_SUCCESS or VWRN_ALREADY_EXISTS on success.
     *          VERR_ALREADY_EXISTS, VERR_NO_MEMORY or VERR_NO_STR_MEMORY on failure.
     * @param   a_ppValue   Where to return the pointer to the value.
     * @param   a_pszKey    The key to put it under.
     * @param   a_cchKey    The length of the key.  Default is the entire string.
     * @param   a_fReplace  Whether to replace or fail on key collision.
     */
    int putNewValue(RTCRestObjectBase **a_ppValue, const char *a_pszKey, size_t a_cchKey = RTSTR_MAX, bool a_fReplace = false) RT_NOEXCEPT;

    /**
     * Creates a new value and inserts it under the given key, returning the new value.
     *
     * @returns VINF_SUCCESS or VWRN_ALREADY_EXISTS on success.
     *          VERR_ALREADY_EXISTS, VERR_NO_MEMORY or VERR_NO_STR_MEMORY on failure.
     * @param   a_ppValue   Where to return the pointer to the value.
     * @param   a_rStrKey   The key to put it under.
     * @param   a_fReplace  Whether to replace or fail on key collision.
     */
    int putNewValue(RTCRestObjectBase **a_ppValue, RTCString const &a_rStrKey, bool a_fReplace = false) RT_NOEXCEPT;

protected:
    /** Map entry. */
    typedef struct MapEntry
    {
        /** String space core. */
        RTSTRSPACECORE      Core;
        /** List node for enumeration. */
        RTLISTNODE          ListEntry;
        /** The key.
         * @remarks Core.pszString points to the value of this object.  So, consider it const. */
        RTCString           strKey;
        /** The value. */
        RTCRestObjectBase  *pValue;
    } MapEntry;
    /** The map tree. */
    RTSTRSPACE          m_Map;
    /** The enumeration list head (MapEntry). */
    RTLISTANCHOR        m_ListHead;
    /** Number of map entries. */
    size_t              m_cEntries;

public:
    /** @name Map Iteration
     * @{  */
    /** Const iterator. */
    class ConstIterator
    {
    private:
        MapEntry            *m_pCur;
        ConstIterator() RT_NOEXCEPT;
    protected:
        ConstIterator(MapEntry *a_pEntry) RT_NOEXCEPT : m_pCur(a_pEntry) { }
    public:
        ConstIterator(ConstIterator const &a_rThat) RT_NOEXCEPT : m_pCur(a_rThat.m_pCur) { }

        /** Gets the key string. */
        inline RTCString const         &getKey() RT_NOEXCEPT   { return m_pCur->strKey; }
        /** Gets poitner to the value object. */
        inline RTCRestObjectBase const *getValue() RT_NOEXCEPT { return m_pCur->pValue; }

        /** Advance to the next map entry. */
        inline ConstIterator &operator++() RT_NOEXCEPT
        {
            m_pCur = RTListNodeGetNextCpp(&m_pCur->ListEntry, MapEntry, ListEntry);
            return *this;
        }

        /** Advance to the previous map entry. */
        inline ConstIterator &operator--() RT_NOEXCEPT
        {
            m_pCur = RTListNodeGetPrevCpp(&m_pCur->ListEntry, MapEntry, ListEntry);
            return *this;
        }

        /** Compare equal. */
        inline bool operator==(ConstIterator const &a_rThat) RT_NOEXCEPT { return m_pCur == a_rThat.m_pCur; }
        /** Compare not equal. */
        inline bool operator!=(ConstIterator const &a_rThat) RT_NOEXCEPT { return m_pCur != a_rThat.m_pCur; }

        /* Map class must be friend so it can use the MapEntry constructor. */
        friend class RTCRestStringMapBase;
    };

    /** Returns iterator for the first map entry (unless it's empty and it's also the end). */
    inline ConstIterator begin() const RT_NOEXCEPT
    {
        if (!RTListIsEmpty(&m_ListHead))
            return ConstIterator(RTListNodeGetNextCpp(&m_ListHead, MapEntry, ListEntry));
        return end();
    }
    /** Returns iterator for the last map entry (unless it's empty and it's also the end). */
    inline ConstIterator last() const RT_NOEXCEPT
    {
        if (!RTListIsEmpty(&m_ListHead))
            return ConstIterator(RTListNodeGetPrevCpp(&m_ListHead, MapEntry, ListEntry));
        return end();
    }
    /** Returns the end iterator.  This does not ever refer to an actual map entry. */
    inline ConstIterator end() const RT_NOEXCEPT
    {
        return ConstIterator(RT_FROM_CPP_MEMBER(&m_ListHead, MapEntry, ListEntry));
    }
    /** @} */


protected:
    /**
     * Helper for creating a clone.
     *
     * @returns Pointer to new map object on success, NULL if out of memory.
     */
    virtual RTCRestStringMapBase *createClone(void) const RT_NOEXCEPT = 0;

    /**
     * Wrapper around the value constructor.
     *
     * @returns Pointer to new value object on success, NULL if out of memory.
     */
    virtual RTCRestObjectBase *createValue(void) RT_NOEXCEPT = 0;

    /**
     * For accessing the static deserializeInstanceFromJson() method of the value.
     */
    virtual int deserializeValueInstanceFromJson(RTCRestJsonCursor const &a_rCursor, RTCRestObjectBase **a_ppInstance) RT_NOEXCEPT = 0;

    /**
     * Worker for the copy assignment method and copyMapWorkerMayThrow.
     *
     * This will use createEntryCopy to do the copying.
     *
     * @returns VINF_SUCCESS on success, VERR_NO_MEMORY or VERR_NO_STR_MEMORY on failure.
     * @param   a_rThat     The map to copy.  Caller makes 100% sure the it has
     *                      the same type as the destination.
     */
    int copyMapWorkerNoThrow(RTCRestStringMapBase const &a_rThat) RT_NOEXCEPT;

    /**
     * Wrapper around copyMapWorkerNoThrow() that throws allocation errors, making
     * it suitable for copy constructors and assignment operators.
     */
    void copyMapWorkerMayThrow(RTCRestStringMapBase const &a_rThat);

    /**
     * Worker for performing inserts.
     *
     * @returns VINF_SUCCESS or VWRN_ALREADY_EXISTS on success.
     *          VERR_ALREADY_EXISTS, VERR_NO_MEMORY or VERR_NO_STR_MEMORY on failure.
     * @param   a_pszKey        The key.
     * @param   a_pValue        The value to insert.  Ownership is transferred to the map on success.
     * @param   a_fReplace      Whether to replace existing key-value pair with matching key.
     * @param   a_cchKey        The key length, the whole string by default.
     */
    int putWorker(const char *a_pszKey, RTCRestObjectBase *a_pValue, bool a_fReplace, size_t a_cchKey = RTSTR_MAX) RT_NOEXCEPT;

    /**
     * Worker for performing inserts.
     *
     * @returns VINF_SUCCESS or VWRN_ALREADY_EXISTS on success.
     *          VERR_ALREADY_EXISTS, VERR_NO_MEMORY or VERR_NO_STR_MEMORY on failure.
     * @param   a_pszKey        The key.
     * @param   a_rValue        The value to copy into the map.
     * @param   a_fReplace      Whether to replace existing key-value pair with matching key.
     * @param   a_cchKey        The key length, the whole string by default.
     */
    int putCopyWorker(const char *a_pszKey, RTCRestObjectBase const &a_rValue, bool a_fReplace, size_t a_cchKey = RTSTR_MAX) RT_NOEXCEPT;

    /**
     * Worker for getting the value corresponding to the given key.
     *
     * @returns Pointer to the value object if found, NULL if key not in the map.
     * @param   a_pszKey        The key which value to look up.
     */
    RTCRestObjectBase *getWorker(const char *a_pszKey) RT_NOEXCEPT;

    /**
     * Worker for getting the value corresponding to the given key, const variant.
     *
     * @returns Pointer to the value object if found, NULL if key not in the map.
     * @param   a_pszKey        The key which value to look up.
     */
    RTCRestObjectBase const *getWorker(const char *a_pszKey) const RT_NOEXCEPT;

private:
    static DECLCALLBACK(int) stringSpaceDestructorCallback(PRTSTRSPACECORE pStr, void *pvUser) RT_NOEXCEPT;
};


/**
 * Limited map class.
 */
template<class ValueType> class RTCRestStringMap : public RTCRestStringMapBase
{
public:
    /** Default constructor, creates emtpy map. */
    RTCRestStringMap() RT_NOEXCEPT
        : RTCRestStringMapBase()
    {}

    /** Copy constructor. */
    RTCRestStringMap(RTCRestStringMap const &a_rThat)
        : RTCRestStringMapBase()
    {
        copyMapWorkerMayThrow(a_rThat);
    }

    /** Destructor. */
    virtual ~RTCRestStringMap()
    {
       /* nothing to do here. */
    }

    /** Copy assignment operator. */
    RTCRestStringMap &operator=(RTCRestStringMap const &a_rThat)
    {
        copyMapWorkerMayThrow(a_rThat);
        return *this;
    }

    /** Safe copy assignment method. */
    int assignCopy(RTCRestStringMap const &a_rThat) RT_NOEXCEPT
    {
        return copyMapWorkerNoThrow(a_rThat);
    }

    /** Make a clone of this object. */
    inline RTCRestStringMap *clone() const RT_NOEXCEPT
    {
        return (RTCRestStringMap *)baseClone();
    }

    /** Factory method. */
    static DECLCALLBACK(RTCRestObjectBase *) createInstance(void) RT_NOEXCEPT
    {
        return new (std::nothrow) RTCRestStringMap<ValueType>();
    }

    /** Factory method for values. */
    static DECLCALLBACK(RTCRestObjectBase *) createValueInstance(void) RT_NOEXCEPT
    {
        return new (std::nothrow) ValueType();
    }

    /** @copydoc RTCRestObjectBase::FNDESERIALIZEINSTANCEFROMJSON */
    static DECLCALLBACK(int) deserializeInstanceFromJson(RTCRestJsonCursor const &a_rCursor, RTCRestObjectBase **a_ppInstance) RT_NOEXCEPT
    {
        *a_ppInstance = new (std::nothrow) RTCRestStringMap<ValueType>();
        if (*a_ppInstance)
            return (*a_ppInstance)->deserializeFromJson(a_rCursor);
        return a_rCursor.m_pPrimary->addError(a_rCursor, VERR_NO_MEMORY, "Out of memory");
    }

    /**
     * Inserts the given object into the map.
     *
     * @returns VINF_SUCCESS or VWRN_ALREADY_EXISTS on success.
     *          VERR_ALREADY_EXISTS, VERR_NO_MEMORY or VERR_NO_STR_MEMORY on failure.
     * @param   a_pszKey        The key.
     * @param   a_pValue        The value to insert.  Ownership is transferred to the map on success.
     * @param   a_fReplace      Whether to replace existing key-value pair with matching key.
     */
    inline int put(const char *a_pszKey, ValueType *a_pValue, bool a_fReplace = false) RT_NOEXCEPT
    {
        return putWorker(a_pszKey, a_pValue, a_fReplace);
    }

    /**
     * Inserts the given object into the map.
     *
     * @returns VINF_SUCCESS or VWRN_ALREADY_EXISTS on success.
     *          VERR_ALREADY_EXISTS, VERR_NO_MEMORY or VERR_NO_STR_MEMORY on failure.
     * @param   a_rStrKey       The key.
     * @param   a_pValue        The value to insert.  Ownership is transferred to the map on success.
     * @param   a_fReplace      Whether to replace existing key-value pair with matching key.
     */
    inline int put(RTCString const &a_rStrKey, ValueType *a_pValue, bool a_fReplace = false) RT_NOEXCEPT
    {
        return putWorker(a_rStrKey.c_str(), a_pValue, a_fReplace, a_rStrKey.length());
    }

    /**
     * Inserts a copy of the given object into the map.
     *
     * @returns VINF_SUCCESS or VWRN_ALREADY_EXISTS on success.
     *          VERR_ALREADY_EXISTS, VERR_NO_MEMORY or VERR_NO_STR_MEMORY on failure.
     * @param   a_pszKey        The key.
     * @param   a_rValue        The value to insert a copy of.
     * @param   a_fReplace      Whether to replace existing key-value pair with matching key.
     */
    inline int putCopy(const char *a_pszKey, const ValueType &a_rValue, bool a_fReplace = false) RT_NOEXCEPT
    {
        return putCopyWorker(a_pszKey, a_rValue, a_fReplace);
    }

    /**
     * Inserts a copy of the given object into the map.
     *
     * @returns VINF_SUCCESS or VWRN_ALREADY_EXISTS on success.
     *          VERR_ALREADY_EXISTS, VERR_NO_MEMORY or VERR_NO_STR_MEMORY on failure.
     * @param   a_rStrKey       The key.
     * @param   a_rValue        The value to insert a copy of.
     * @param   a_fReplace      Whether to replace existing key-value pair with matching key.
     */
    inline int putCopy(RTCString const &a_rStrKey, const ValueType &a_rValue, bool a_fReplace = false) RT_NOEXCEPT
    {
        return putCopyWorker(a_rStrKey.c_str(), a_rValue, a_fReplace, a_rStrKey.length());
    }

    /**
     * Gets the value corresponding to the given key.
     *
     * @returns Pointer to the value object if found, NULL if key not in the map.
     * @param   a_pszKey        The key which value to look up.
     */
    inline ValueType *get(const char *a_pszKey) RT_NOEXCEPT
    {
        return (ValueType *)getWorker(a_pszKey);
    }

    /**
     * Gets the value corresponding to the given key.
     *
     * @returns Pointer to the value object if found, NULL if key not in the map.
     * @param   a_rStrKey       The key which value to look up.
     */
    inline ValueType *get(RTCString const &a_rStrKey) RT_NOEXCEPT
    {
        return (ValueType *)getWorker(a_rStrKey.c_str());
    }

    /**
     * Gets the const value corresponding to the given key.
     *
     * @returns Pointer to the value object if found, NULL if key not in the map.
     * @param   a_pszKey        The key which value to look up.
     */
    inline ValueType const *get(const char *a_pszKey) const RT_NOEXCEPT
    {
        return (ValueType const *)getWorker(a_pszKey);
    }

    /**
     * Gets the const value corresponding to the given key.
     *
     * @returns Pointer to the value object if found, NULL if key not in the map.
     * @param   a_rStrKey       The key which value to look up.
     */
    inline ValueType const *get(RTCString const &a_rStrKey) const RT_NOEXCEPT
    {
        return (ValueType const *)getWorker(a_rStrKey.c_str());
    }

    /** @todo enumerator*/

protected:
    virtual RTCRestStringMapBase *createClone(void) const RT_NOEXCEPT RT_OVERRIDE
    {
        return new (std::nothrow) RTCRestStringMap();
    }

    virtual RTCRestObjectBase *createValue(void) RT_NOEXCEPT RT_OVERRIDE
    {
        return new (std::nothrow) ValueType();
    }

    virtual int deserializeValueInstanceFromJson(RTCRestJsonCursor const &a_rCursor, RTCRestObjectBase **a_ppInstance) RT_NOEXCEPT RT_OVERRIDE
    {
        return ValueType::deserializeInstanceFromJson(a_rCursor, a_ppInstance);
    }
};


/** @} */

#endif /* !IPRT_INCLUDED_cpp_reststringmap_h */

