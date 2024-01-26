/** @file
 * IPRT - C++ Representational State Transfer (REST) Base Classes.
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

#ifndef IPRT_INCLUDED_cpp_restbase_h
#define IPRT_INCLUDED_cpp_restbase_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/errcore.h> /* VERR_NO_MEMORY */
#include <iprt/json.h>
#include <iprt/stdarg.h>
#include <iprt/time.h>
#include <iprt/cpp/ministring.h>


/** @defgroup grp_rt_cpp_restbase   C++ Representational State Transfer (REST) Base Classes.
 * @ingroup grp_rt_cpp
 * @{
 */

/* forward decl: */
class RTCRestOutputBase;
class RTCRestJsonPrimaryCursor;

/**
 * JSON cursor structure.
 *
 * This reduces the number of parameters passed around when deserializing JSON
 * input and also helps constructing full object name for logging and error reporting.
 */
struct RT_DECL_CLASS RTCRestJsonCursor
{
    /** Handle to the value being parsed. */
    RTJSONVAL                           m_hValue;
    /** Name of the value. */
    const char                         *m_pszName;
    /** Pointer to the parent, NULL if primary. */
    struct RTCRestJsonCursor const     *m_pParent;
    /** Pointer to the primary cursor structure. */
    RTCRestJsonPrimaryCursor           *m_pPrimary;

    RTCRestJsonCursor(struct RTCRestJsonCursor const &a_rParent) RT_NOEXCEPT
        : m_hValue(NIL_RTJSONVAL), m_pszName(NULL), m_pParent(&a_rParent), m_pPrimary(a_rParent.m_pPrimary)
    { }

    RTCRestJsonCursor(RTJSONVAL hValue, const char *pszName, struct RTCRestJsonCursor *pParent) RT_NOEXCEPT
        : m_hValue(hValue), m_pszName(pszName), m_pParent(pParent), m_pPrimary(pParent->m_pPrimary)
    { }

    RTCRestJsonCursor(RTJSONVAL hValue, const char *pszName) RT_NOEXCEPT
        : m_hValue(hValue), m_pszName(pszName), m_pParent(NULL), m_pPrimary(NULL)
    { }

    ~RTCRestJsonCursor()
    {
        if (m_hValue != NIL_RTJSONVAL)
        {
            RTJsonValueRelease(m_hValue);
            m_hValue = NIL_RTJSONVAL;
        }
    }
};


/**
 * The primary JSON cursor class.
 */
class RT_DECL_CLASS RTCRestJsonPrimaryCursor
{
public:
    /** The cursor for the first level. */
    RTCRestJsonCursor   m_Cursor;
    /** Error info keeper. */
    PRTERRINFO          m_pErrInfo;

    /** Creates a primary json cursor with optiona error info. */
    RTCRestJsonPrimaryCursor(RTJSONVAL hValue, const char *pszName, PRTERRINFO pErrInfo = NULL) RT_NOEXCEPT
        : m_Cursor(hValue, pszName)
        , m_pErrInfo(pErrInfo)
    {
        m_Cursor.m_pPrimary = this;
    }

    virtual ~RTCRestJsonPrimaryCursor()
    {  }

    /**
     * Add an error message.
     *
     * @returns a_rc
     * @param   a_rCursor       The cursor reporting the error.
     * @param   a_rc            The status code.
     * @param   a_pszFormat     Format string.
     * @param   ...             Format string arguments.
     */
    virtual int addError(RTCRestJsonCursor const &a_rCursor, int a_rc, const char *a_pszFormat, ...) RT_NOEXCEPT;

    /**
     * Reports that the current field is not known.
     *
     * @returns Status to propagate.
     * @param   a_rCursor       The cursor for the field.
     */
    virtual int unknownField(RTCRestJsonCursor const &a_rCursor) RT_NOEXCEPT;

    /**
     * Copies the full path into pszDst.
     *
     * @returns pszDst
     * @param   a_rCursor       The cursor to start walking at.
     * @param   a_pszDst        Where to put the path.
     * @param   a_cbDst         Size of the destination buffer.
     */
    virtual char *getPath(RTCRestJsonCursor const &a_rCursor, char *a_pszDst, size_t a_cbDst) const RT_NOEXCEPT;
};


/**
 * Abstract base class for REST primitive types and data objects (via
 * RTCRestDataObject).
 *
 * The only information this keeps is the null indicator.
 */
class RT_DECL_CLASS RTCRestObjectBase
{
public:
    RTCRestObjectBase() RT_NOEXCEPT;
    RTCRestObjectBase(RTCRestObjectBase const &a_rThat) RT_NOEXCEPT;
    virtual ~RTCRestObjectBase();

    /** Copy assignment operator. */
    RTCRestObjectBase &operator=(RTCRestObjectBase const &a_rThat) RT_NOEXCEPT;

    /**
     * Create a copy of this object.
     *
     * @returns Pointer to copy.
     */
    virtual RTCRestObjectBase *baseClone() const RT_NOEXCEPT = 0;

    /**
     * Tests if the object is @a null.
     * @returns true if null, false if not.
     */
    inline bool isNull(void) const RT_NOEXCEPT { return m_fNullIndicator; };

    /**
     * Sets the object to @a null and fills it with defaults.
     * @returns IPRT status code (from resetToDefault).
     */
    virtual int setNull(void) RT_NOEXCEPT;

    /**
     * Sets the object to not-null state (i.e. undoes setNull()).
     * @remarks Only really important for strings.
     */
    virtual void setNotNull(void) RT_NOEXCEPT;

    /**
     * Resets the object to all default values.
     * @returns IPRT status code.
     */
    virtual int resetToDefault() RT_NOEXCEPT = 0;

    /**
     * Serialize the object as JSON.
     *
     * @returns a_rDst
     * @param   a_rDst      The destination for the serialization.
     */
    virtual RTCRestOutputBase &serializeAsJson(RTCRestOutputBase &a_rDst) const RT_NOEXCEPT = 0;

    /**
     * Deserialize object from the given JSON iterator.
     *
     * @returns IPRT status code.
     * @param   a_rCursor    The JSON cursor.
     */
    virtual int deserializeFromJson(RTCRestJsonCursor const &a_rCursor) RT_NOEXCEPT = 0;

    /**
     * Polymorphic JSON deserialization helper that instantiate the matching class using
     * the discriminator field.
     *
     * @returns IPRT status code.
     * @param   a_rCursor    The JSON cursor.
     * @param   a_ppInstance Where to return the deserialized instance.
     *                       May return an object on failure.
     */
    typedef DECLCALLBACKTYPE(int, FNDESERIALIZEINSTANCEFROMJSON,(RTCRestJsonCursor const &a_rCursor, RTCRestObjectBase **a_ppInstance));
    /** Pointer to a FNDESERIALIZEINSTANCEFROMJSON function. */
    typedef FNDESERIALIZEINSTANCEFROMJSON *PFNDESERIALIZEINSTANCEFROMJSON;

    /**
     * Flags for toString().
     *
     * The kCollectionFormat_xxx bunch controls multiple values in arrays
     * are formatted.  They are ignored by everyone else.
     *
     * @note When adding collection format types, make sure to also
     *       update RTCRestArrayBase::toString().
     * @note Bit 24 is reserved (for kHdrField_MapCollection).
     */
    enum
    {
        kCollectionFormat_Unspecified = 0,  /**< Not specified. */
        kCollectionFormat_csv,              /**< Comma-separated list. */
        kCollectionFormat_ssv,              /**< Space-separated list. */
        kCollectionFormat_tsv,              /**< Tab-separated list. */
        kCollectionFormat_pipes,            /**< Pipe-separated list. */
        kCollectionFormat_multi,            /**< Special collection type that must be handled by caller of toString. */
        kCollectionFormat_Mask = 7,         /**< Collection type mask. */

        kToString_Append = 8                /**< Append to the string/object (rather than assigning). */
    };

    /**
     * String conversion.
     *
     * The default implementation of is a wrapper around serializeAsJson().
     *
     * @returns IPRT status code.
     * @param   a_pDst      Pointer to the destionation string.
     * @param   a_fFlags    kCollectionFormat_xxx.
     */
    virtual int toString(RTCString *a_pDst, uint32_t a_fFlags = kCollectionFormat_Unspecified) const RT_NOEXCEPT;

    /**
     * String convertsion, naive variant.
     *
     * @returns String represenation.
     */
    RTCString toString() const;

    /**
     * Convert from (header) string value.
     *
     * The default implementation of is a wrapper around deserializeFromJson().
     *
     * @returns IPRT status code.
     * @param   a_rValue    The string value string to parse.
     * @param   a_pszName   Field name or similar.
     * @param   a_pErrInfo  Where to return additional error info.  Optional.
     * @param   a_fFlags    kCollectionFormat_xxx.
     */
    virtual int fromString(RTCString const &a_rValue, const char *a_pszName, PRTERRINFO a_pErrInfo = NULL,
                           uint32_t a_fFlags = kCollectionFormat_Unspecified) RT_NOEXCEPT;

    /** Type classification */
    typedef enum kTypeClass
    {
        kTypeClass_Invalid = 0,
        kTypeClass_Bool,                /**< Primitive: bool. */
        kTypeClass_Int64,               /**< Primitive: int64_t. */
        kTypeClass_Int32,               /**< Primitive: int32_t. */
        kTypeClass_Int16,               /**< Primitive: int16_t. */
        kTypeClass_Double,              /**< Primitive: double. */
        kTypeClass_String,              /**< Primitive: string. */
        kTypeClass_Date,                /**< Date. */
        kTypeClass_Uuid,                /**< UUID. */
        kTypeClass_Binary,              /**< Binary blob. */
        kTypeClass_DataObject,          /**< Data object child (RTCRestDataObject). */
        kTypeClass_AnyObject,           /**< Any kind of object (RTCRestAnyObject). */
        kTypeClass_Array,               /**< Array (containing any kind of object). */
        kTypeClass_StringMap,           /**< String map (containing any kind of object). */
        kTypeClass_StringEnum           /**< String enum. */
    } kTypeClass;

    /**
     * Returns the object type class.
     */
    virtual kTypeClass typeClass(void) const RT_NOEXCEPT = 0;

    /**
     * Returns the object type name.
     */
    virtual const char *typeName(void) const RT_NOEXCEPT = 0;

protected:
    /** Null indicator.
     * @remarks The null values could be mapped onto C/C++ NULL pointer values,
     *          with the consequence that all data members in objects and such would
     *          have had to been allocated individually, even simple @a bool members.
     *          Given that we're overly paranoid about heap allocations (std::bad_alloc),
     *          it's more fitting to use a null indicator for us.
     */
    bool    m_fNullIndicator;
};


/**
 * Class wrapping 'bool'.
 */
class RT_DECL_CLASS RTCRestBool : public RTCRestObjectBase
{
public:
    /** Default constructor. */
    RTCRestBool() RT_NOEXCEPT;
    /** Copy constructor. */
    RTCRestBool(RTCRestBool const &a_rThat) RT_NOEXCEPT;
    /** From value constructor. */
    RTCRestBool(bool fValue) RT_NOEXCEPT;
    /** Destructor. */
    virtual ~RTCRestBool();
    /** Copy assignment operator. */
    RTCRestBool &operator=(RTCRestBool const &a_rThat) RT_NOEXCEPT;
    /** Safe copy assignment method. */
    int assignCopy(RTCRestBool const &a_rThat) RT_NOEXCEPT;
    /** Assign value and clear null indicator. */
    void assignValue(bool a_fValue) RT_NOEXCEPT;
    /** Make a clone of this object. */
    inline RTCRestBool *clone() const RT_NOEXCEPT { return (RTCRestBool *)baseClone(); }

    /* Overridden methods: */
    virtual RTCRestObjectBase *baseClone() const RT_NOEXCEPT RT_OVERRIDE;
    virtual int resetToDefault() RT_NOEXCEPT RT_OVERRIDE;
    virtual RTCRestOutputBase &serializeAsJson(RTCRestOutputBase &a_rDst) const RT_NOEXCEPT RT_OVERRIDE;
    virtual int deserializeFromJson(RTCRestJsonCursor const &a_rCursor) RT_NOEXCEPT RT_OVERRIDE;
    virtual int toString(RTCString *a_pDst, uint32_t a_fFlags = 0) const RT_NOEXCEPT RT_OVERRIDE;
    virtual int fromString(RTCString const &a_rValue, const char *a_pszName, PRTERRINFO a_pErrInfo = NULL,
                           uint32_t a_fFlags = kCollectionFormat_Unspecified) RT_NOEXCEPT RT_OVERRIDE;
    virtual kTypeClass typeClass(void) const RT_NOEXCEPT RT_OVERRIDE;
    virtual const char *typeName(void) const RT_NOEXCEPT RT_OVERRIDE;

    /** Factory method. */
    static DECLCALLBACK(RTCRestObjectBase *) createInstance(void) RT_NOEXCEPT;
    /** @copydoc RTCRestObjectBase::FNDESERIALIZEINSTANCEFROMJSON */
    static DECLCALLBACK(int) deserializeInstanceFromJson(RTCRestJsonCursor const &a_rCursor, RTCRestObjectBase **a_ppInstance) RT_NOEXCEPT;

public:
    /** The value. */
    bool    m_fValue;
};


/**
 * Class wrapping 'int64_t'.
 */
class RT_DECL_CLASS RTCRestInt64 : public RTCRestObjectBase
{
public:
    /** Default constructor. */
    RTCRestInt64() RT_NOEXCEPT;
    /** Copy constructor. */
    RTCRestInt64(RTCRestInt64 const &a_rThat) RT_NOEXCEPT;
    /** From value constructor. */
    RTCRestInt64(int64_t a_iValue) RT_NOEXCEPT;
    /** Destructor. */
    virtual ~RTCRestInt64();
    /** Copy assignment operator. */
    RTCRestInt64 &operator=(RTCRestInt64 const &a_rThat) RT_NOEXCEPT;
    /** Safe copy assignment method. */
    int assignCopy(RTCRestInt64 const &a_rThat) RT_NOEXCEPT;
    /** Assign value and clear null indicator. */
    void assignValue(int64_t a_iValue) RT_NOEXCEPT;
    /** Make a clone of this object. */
    inline RTCRestInt64 *clone() const RT_NOEXCEPT { return (RTCRestInt64 *)baseClone(); }

    /* Overridden methods: */
    virtual RTCRestObjectBase *baseClone() const RT_NOEXCEPT RT_OVERRIDE;
    virtual int resetToDefault() RT_NOEXCEPT RT_OVERRIDE;
    virtual RTCRestOutputBase &serializeAsJson(RTCRestOutputBase &a_rDst) const RT_NOEXCEPT RT_OVERRIDE;
    virtual int deserializeFromJson(RTCRestJsonCursor const &a_rCursor) RT_NOEXCEPT RT_OVERRIDE;
    virtual int toString(RTCString *a_pDst, uint32_t a_fFlags = 0) const RT_NOEXCEPT RT_OVERRIDE;
    virtual int fromString(RTCString const &a_rValue, const char *a_pszName, PRTERRINFO a_pErrInfo = NULL,
                           uint32_t a_fFlags = kCollectionFormat_Unspecified) RT_NOEXCEPT RT_OVERRIDE;
    virtual kTypeClass typeClass(void) const RT_NOEXCEPT RT_OVERRIDE;
    virtual const char *typeName(void) const RT_NOEXCEPT RT_OVERRIDE;

    /** Factory method. */
    static DECLCALLBACK(RTCRestObjectBase *) createInstance(void) RT_NOEXCEPT;
    /** @copydoc RTCRestObjectBase::FNDESERIALIZEINSTANCEFROMJSON */
    static DECLCALLBACK(int) deserializeInstanceFromJson(RTCRestJsonCursor const &a_rCursor, RTCRestObjectBase **a_ppInstance) RT_NOEXCEPT;

public:
    /** The value. */
    int64_t m_iValue;
};


/**
 * Class wrapping 'int32_t'.
 */
class RT_DECL_CLASS RTCRestInt32 : public RTCRestObjectBase
{
public:
    /** Default constructor. */
    RTCRestInt32() RT_NOEXCEPT;
    /** Copy constructor. */
    RTCRestInt32(RTCRestInt32 const &a_rThat) RT_NOEXCEPT;
    /** From value constructor. */
    RTCRestInt32(int32_t iValue) RT_NOEXCEPT;
    /** Destructor. */
    virtual ~RTCRestInt32() RT_NOEXCEPT;
    /** Copy assignment operator. */
    RTCRestInt32 &operator=(RTCRestInt32 const &a_rThat) RT_NOEXCEPT;
    /** Safe copy assignment method. */
    int assignCopy(RTCRestInt32 const &a_rThat) RT_NOEXCEPT;
    /** Assign value and clear null indicator. */
    void assignValue(int32_t a_iValue) RT_NOEXCEPT;
    /** Make a clone of this object. */
    inline RTCRestInt32 *clone() const { return (RTCRestInt32 *)baseClone(); }

    /* Overridden methods: */
    virtual RTCRestObjectBase *baseClone() const RT_NOEXCEPT RT_OVERRIDE;
    virtual int resetToDefault() RT_NOEXCEPT RT_OVERRIDE;
    virtual RTCRestOutputBase &serializeAsJson(RTCRestOutputBase &a_rDst) const RT_NOEXCEPT RT_OVERRIDE;
    virtual int deserializeFromJson(RTCRestJsonCursor const &a_rCursor) RT_NOEXCEPT RT_OVERRIDE;
    virtual int toString(RTCString *a_pDst, uint32_t a_fFlags = 0) const RT_NOEXCEPT RT_OVERRIDE;
    virtual int fromString(RTCString const &a_rValue, const char *a_pszName, PRTERRINFO a_pErrInfo = NULL,
                           uint32_t a_fFlags = kCollectionFormat_Unspecified) RT_NOEXCEPT RT_OVERRIDE;
    virtual kTypeClass typeClass(void) const RT_NOEXCEPT RT_OVERRIDE;
    virtual const char *typeName(void) const RT_NOEXCEPT RT_OVERRIDE;

    /** Factory method. */
    static DECLCALLBACK(RTCRestObjectBase *) createInstance(void) RT_NOEXCEPT;
    /** @copydoc RTCRestObjectBase::FNDESERIALIZEINSTANCEFROMJSON */
    static DECLCALLBACK(int) deserializeInstanceFromJson(RTCRestJsonCursor const &a_rCursor, RTCRestObjectBase **a_ppInstance) RT_NOEXCEPT;

public:
    /** The value. */
    int32_t m_iValue;
};


/**
 * Class wrapping 'int16_t'.
 */
class RT_DECL_CLASS RTCRestInt16 : public RTCRestObjectBase
{
public:
    /** Default constructor. */
    RTCRestInt16() RT_NOEXCEPT;
    /** Copy constructor. */
    RTCRestInt16(RTCRestInt16 const &a_rThat) RT_NOEXCEPT;
    /** From value constructor. */
    RTCRestInt16(int16_t iValue) RT_NOEXCEPT;
    /** Destructor. */
    virtual ~RTCRestInt16();
    /** Copy assignment operator. */
    RTCRestInt16 &operator=(RTCRestInt16 const &a_rThat) RT_NOEXCEPT;
    /** Safe copy assignment method. */
    int assignCopy(RTCRestInt16 const &a_rThat) RT_NOEXCEPT;
    /** Assign value and clear null indicator. */
    void assignValue(int16_t a_iValue) RT_NOEXCEPT;
    /** Make a clone of this object. */
    inline RTCRestInt16 *clone() const RT_NOEXCEPT { return (RTCRestInt16 *)baseClone(); }

    /* Overridden methods: */
    virtual RTCRestObjectBase *baseClone() const RT_NOEXCEPT RT_OVERRIDE;
    virtual int resetToDefault() RT_NOEXCEPT RT_OVERRIDE;
    virtual RTCRestOutputBase &serializeAsJson(RTCRestOutputBase &a_rDst) const RT_NOEXCEPT RT_OVERRIDE;
    virtual int deserializeFromJson(RTCRestJsonCursor const &a_rCursor) RT_NOEXCEPT RT_OVERRIDE;
    virtual int toString(RTCString *a_pDst, uint32_t a_fFlags = 0) const RT_NOEXCEPT RT_OVERRIDE;
    virtual int fromString(RTCString const &a_rValue, const char *a_pszName, PRTERRINFO a_pErrInfo = NULL,
                           uint32_t a_fFlags = kCollectionFormat_Unspecified) RT_NOEXCEPT RT_OVERRIDE;
    virtual kTypeClass typeClass(void) const RT_NOEXCEPT RT_OVERRIDE;
    virtual const char *typeName(void) const RT_NOEXCEPT RT_OVERRIDE;

    /** Factory method. */
    static DECLCALLBACK(RTCRestObjectBase *) createInstance(void) RT_NOEXCEPT;
    /** @copydoc RTCRestObjectBase::FNDESERIALIZEINSTANCEFROMJSON */
    static DECLCALLBACK(int) deserializeInstanceFromJson(RTCRestJsonCursor const &a_rCursor, RTCRestObjectBase **a_ppInstance) RT_NOEXCEPT;

public:
    /** The value. */
    int16_t m_iValue;
};


/**
 * Class wrapping 'double'.
 */
class RT_DECL_CLASS RTCRestDouble : public RTCRestObjectBase
{
public:
    /** Default constructor. */
    RTCRestDouble() RT_NOEXCEPT;
    /** Copy constructor. */
    RTCRestDouble(RTCRestDouble const &a_rThat) RT_NOEXCEPT;
    /** From value constructor. */
    RTCRestDouble(double rdValue) RT_NOEXCEPT;
    /** Destructor. */
    virtual ~RTCRestDouble();
    /** Copy assignment operator. */
    RTCRestDouble &operator=(RTCRestDouble const &a_rThat) RT_NOEXCEPT;
    /** Safe copy assignment method. */
    int assignCopy(RTCRestDouble const &a_rThat) RT_NOEXCEPT;
    /** Assign value and clear null indicator. */
    void assignValue(double a_rdValue) RT_NOEXCEPT;
    /** Make a clone of this object. */
    inline RTCRestDouble *clone() const RT_NOEXCEPT { return (RTCRestDouble *)baseClone(); }

    /* Overridden methods: */
    virtual RTCRestObjectBase *baseClone() const RT_NOEXCEPT RT_OVERRIDE;
    virtual int resetToDefault() RT_NOEXCEPT RT_OVERRIDE;
    virtual RTCRestOutputBase &serializeAsJson(RTCRestOutputBase &a_rDst) const RT_NOEXCEPT RT_OVERRIDE;
    virtual int deserializeFromJson(RTCRestJsonCursor const &a_rCursor) RT_NOEXCEPT RT_OVERRIDE;
    virtual int toString(RTCString *a_pDst, uint32_t a_fFlags = 0) const RT_NOEXCEPT RT_OVERRIDE;
    virtual int fromString(RTCString const &a_rValue, const char *a_pszName, PRTERRINFO a_pErrInfo = NULL,
                           uint32_t a_fFlags = kCollectionFormat_Unspecified) RT_NOEXCEPT RT_OVERRIDE;
    virtual kTypeClass typeClass(void) const RT_NOEXCEPT RT_OVERRIDE;
    virtual const char *typeName(void) const RT_NOEXCEPT RT_OVERRIDE;

    /** Factory method. */
    static DECLCALLBACK(RTCRestObjectBase *) createInstance(void) RT_NOEXCEPT;
    /** @copydoc RTCRestObjectBase::FNDESERIALIZEINSTANCEFROMJSON */
    static DECLCALLBACK(int) deserializeInstanceFromJson(RTCRestJsonCursor const &a_rCursor, RTCRestObjectBase **a_ppInstance) RT_NOEXCEPT;

public:
    /** The value. */
    double m_rdValue;
};


/**
 * Class wrapping 'RTCString'.
 */
class RT_DECL_CLASS RTCRestString : public RTCRestObjectBase, public RTCString
{
public:
    /** Default constructor. */
    RTCRestString() RT_NOEXCEPT;
    /** Destructor. */
    virtual ~RTCRestString();

    /** Copy constructor. */
    RTCRestString(RTCRestString const &a_rThat);
    /** From value constructor. */
    RTCRestString(RTCString const &a_rThat);
    /** From value constructor. */
    RTCRestString(const char *a_pszSrc);
    /** Safe copy assignment method. */
    int assignCopy(RTCRestString const &a_rThat) RT_NOEXCEPT;
    /** Safe copy assignment method. */
    int assignCopy(RTCString const &a_rThat) RT_NOEXCEPT;
    /** Safe copy assignment method. */
    int assignCopy(const char *a_pszThat) RT_NOEXCEPT;
    /** Make a clone of this object. */
    inline RTCRestString *clone() const RT_NOEXCEPT { return (RTCRestString *)baseClone(); }

    /* Overridden methods: */
    virtual RTCRestObjectBase *baseClone() const RT_NOEXCEPT RT_OVERRIDE;
    virtual int setNull(void) RT_NOEXCEPT RT_OVERRIDE; /* (ambigious, so overrider it to make sure.) */
    virtual int resetToDefault() RT_NOEXCEPT RT_OVERRIDE;
    virtual RTCRestOutputBase &serializeAsJson(RTCRestOutputBase &a_rDst) const RT_NOEXCEPT RT_OVERRIDE;
    virtual int deserializeFromJson(RTCRestJsonCursor const &a_rCursor) RT_NOEXCEPT RT_OVERRIDE;
    virtual int toString(RTCString *a_pDst, uint32_t a_fFlags = kCollectionFormat_Unspecified) const RT_NOEXCEPT RT_OVERRIDE;
    virtual int fromString(RTCString const &a_rValue, const char *a_pszName, PRTERRINFO a_pErrInfo = NULL,
                           uint32_t a_fFlags = kCollectionFormat_Unspecified) RT_NOEXCEPT RT_OVERRIDE;
    virtual kTypeClass typeClass(void) const RT_NOEXCEPT RT_OVERRIDE;
    virtual const char *typeName(void) const RT_NOEXCEPT RT_OVERRIDE;

    /** Factory method. */
    static DECLCALLBACK(RTCRestObjectBase *) createInstance(void) RT_NOEXCEPT;
    /** @copydoc RTCRestObjectBase::FNDESERIALIZEINSTANCEFROMJSON */
    static DECLCALLBACK(int) deserializeInstanceFromJson(RTCRestJsonCursor const &a_rCursor, RTCRestObjectBase **a_ppInstance) RT_NOEXCEPT;

    /** @name RTCString assignment methods we need to replace to manage the null indicator
     * @{ */
    int assignNoThrow(const RTCString &a_rSrc) RT_NOEXCEPT;
    int assignNoThrow(const char *a_pszSrc) RT_NOEXCEPT;
    int assignNoThrow(const RTCString &a_rSrc, size_t a_offSrc, size_t a_cchSrc = npos) RT_NOEXCEPT;
    int assignNoThrow(const char *a_pszSrc, size_t a_cchSrc) RT_NOEXCEPT;
    int assignNoThrow(size_t a_cTimes, char a_ch) RT_NOEXCEPT;
    int printfNoThrow(const char *pszFormat, ...) RT_NOEXCEPT RT_IPRT_FORMAT_ATTR(1, 2);
    int printfVNoThrow(const char *pszFormat, va_list va) RT_NOEXCEPT RT_IPRT_FORMAT_ATTR(1, 0);
    RTCRestString &operator=(const char *a_pcsz);
    RTCRestString &operator=(const RTCString &a_rThat);
    RTCRestString &operator=(const RTCRestString &a_rThat);
    RTCRestString &assign(const RTCString &a_rSrc);
    RTCRestString &assign(const char *a_pszSrc);
    RTCRestString &assign(const RTCString &a_rSrc, size_t a_offSrc, size_t a_cchSrc = npos);
    RTCRestString &assign(const char *a_pszSrc, size_t a_cchSrc);
    RTCRestString &assign(size_t a_cTimes, char a_ch);
    RTCRestString &printf(const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(1, 2);
    RTCRestString &printfV(const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(1, 0);
    /** @} */
};


/**
 * Date class.
 *
 * There are numerous ways of formatting a timestamp and the specifications
 * we're currently working with doesn't have a way of telling it seems.
 * Thus, decoding need to have fail safes built in so the user can give hints.
 * The formatting likewise needs to be told which format to use by the user.
 *
 * Two side-effects of the format stuff is that the default constructor creates
 * an object that is null, and resetToDefault will do the same bug leave the
 * format as a hint.
 */
class RT_DECL_CLASS RTCRestDate : public RTCRestObjectBase
{
public:
    /** Default constructor.
     * @note The result is a null-object.   */
    RTCRestDate() RT_NOEXCEPT;
    /** Copy constructor. */
    RTCRestDate(RTCRestDate const &a_rThat);
    /** Destructor. */
    virtual ~RTCRestDate();
    /** Copy assignment operator. */
    RTCRestDate &operator=(RTCRestDate const &a_rThat);
    /** Safe copy assignment method. */
    int assignCopy(RTCRestDate const &a_rThat) RT_NOEXCEPT;
    /** Make a clone of this object. */
    inline RTCRestDate *clone() const  RT_NOEXCEPT{ return (RTCRestDate *)baseClone(); }

    /* Overridden methods: */
    virtual RTCRestObjectBase *baseClone() const RT_NOEXCEPT RT_OVERRIDE;
    virtual int resetToDefault() RT_NOEXCEPT RT_OVERRIDE;
    virtual RTCRestOutputBase &serializeAsJson(RTCRestOutputBase &a_rDst) const RT_NOEXCEPT RT_OVERRIDE;
    virtual int deserializeFromJson(RTCRestJsonCursor const &a_rCursor) RT_NOEXCEPT RT_OVERRIDE;
    virtual int toString(RTCString *a_pDst, uint32_t a_fFlags = 0) const RT_NOEXCEPT RT_OVERRIDE;
    virtual int fromString(RTCString const &a_rValue, const char *a_pszName, PRTERRINFO a_pErrInfo = NULL,
                           uint32_t a_fFlags = kCollectionFormat_Unspecified) RT_NOEXCEPT RT_OVERRIDE;
    virtual kTypeClass typeClass(void) const RT_NOEXCEPT RT_OVERRIDE;
    virtual const char *typeName(void) const RT_NOEXCEPT RT_OVERRIDE;

    /** Factory method. */
    static DECLCALLBACK(RTCRestObjectBase *) createInstance(void) RT_NOEXCEPT;
    /** @copydoc RTCRestObjectBase::FNDESERIALIZEINSTANCEFROMJSON */
    static DECLCALLBACK(int) deserializeInstanceFromJson(RTCRestJsonCursor const &a_rCursor, RTCRestObjectBase **a_ppInstance) RT_NOEXCEPT;

    /** Date formats. */
    typedef enum
    {
        kFormat_Invalid = 0,
        kFormat_Rfc2822,            /**< Format it according to RFC-2822. */
        kFormat_Rfc7131,            /**< Format it according to RFC-7131 (HTTP). */
        kFormat_Rfc3339,            /**< Format it according to RFC-3339 (ISO-8601) (no fraction). */
        kFormat_Rfc3339_Fraction_2, /**< Format it according to RFC-3339 (ISO-8601) with two digit fraction (hundreths). */
        kFormat_Rfc3339_Fraction_3, /**< Format it according to RFC-3339 (ISO-8601) with three digit fraction (milliseconds). */
        kFormat_Rfc3339_Fraction_6, /**< Format it according to RFC-3339 (ISO-8601) with six digit fraction (microseconds). */
        kFormat_Rfc3339_Fraction_9, /**< Format it according to RFC-3339 (ISO-8601) with nine digit fraction (nanoseconds). */
        kFormat_End
    } kFormat;

    /**
     * Assigns the value, formats it as a string and clears the null indicator.
     *
     * @returns VINF_SUCCESS, VERR_NO_STR_MEMORY or VERR_INVALID_PARAMETER.
     * @param   a_pTimeSpec     The time spec to set.
     * @param   a_enmFormat     The date format to use when formatting it.
     */
    int assignValue(PCRTTIMESPEC a_pTimeSpec, kFormat a_enmFormat) RT_NOEXCEPT;
    int assignValueRfc2822(PCRTTIMESPEC a_pTimeSpec) RT_NOEXCEPT; /**< Convenience method for email/whatnot. */
    int assignValueRfc7131(PCRTTIMESPEC a_pTimeSpec) RT_NOEXCEPT; /**< Convenience method for HTTP date. */
    int assignValueRfc3339(PCRTTIMESPEC a_pTimeSpec) RT_NOEXCEPT; /**< Convenience method for ISO-8601 timstamp. */

    /**
     * Assigns the current UTC time and clears the null indicator .
     *
     * @returns VINF_SUCCESS, VERR_NO_STR_MEMORY or VERR_INVALID_PARAMETER.
     * @returns VINF_SUCCESS or VERR_NO_STR_MEMORY.
     * @param   a_enmFormat     The date format to use when formatting it.
     */
    int assignNow(kFormat a_enmFormat) RT_NOEXCEPT;
    int assignNowRfc2822() RT_NOEXCEPT; /**< Convenience method for email/whatnot. */
    int assignNowRfc7131() RT_NOEXCEPT; /**< Convenience method for HTTP date. */
    int assignNowRfc3339() RT_NOEXCEPT; /**< Convenience method for ISO-8601 timstamp. */

    /**
     * Sets the format to help deal with decoding issues.
     *
     * This can also be used to change the date format for an okay timespec.
     * @returns IPRT status code.
     * @param   a_enmFormat     The date format to try/set.
     */
    int setFormat(kFormat a_enmFormat) RT_NOEXCEPT;

    /** Check if the value is okay (m_TimeSpec & m_Exploded). */
    inline bool              isOkay() const RT_NOEXCEPT         { return m_fTimeSpecOkay; }
    /** Get the timespec value. */
    inline RTTIMESPEC const &getTimeSpec() const RT_NOEXCEPT    { return m_TimeSpec; }
    /** Get the exploded time. */
    inline RTTIME const     &getExploded() const RT_NOEXCEPT    { return m_Exploded; }
    /** Gets the format. */
    inline kFormat           getFormat() const RT_NOEXCEPT      { return m_enmFormat; }
    /** Get the formatted/raw string value. */
    inline RTCString const  &getString() const RT_NOEXCEPT      { return m_strFormatted; }

    /** Get nanoseconds since unix epoch. */
    inline int64_t           getEpochNano() const RT_NOEXCEPT   { return RTTimeSpecGetNano(&m_TimeSpec); }
    /** Get seconds since unix epoch. */
    inline int64_t           getEpochSeconds() const RT_NOEXCEPT { return RTTimeSpecGetSeconds(&m_TimeSpec); }
    /** Checks if UTC time. */
    inline bool              isUtc() const RT_NOEXCEPT { return (m_Exploded.fFlags & RTTIME_FLAGS_TYPE_MASK) != RTTIME_FLAGS_TYPE_LOCAL; }
    /** Checks if local time. */
    inline bool              isLocal() const RT_NOEXCEPT { return (m_Exploded.fFlags & RTTIME_FLAGS_TYPE_MASK) == RTTIME_FLAGS_TYPE_LOCAL; }

protected:
    /** The value. */
    RTTIMESPEC  m_TimeSpec;
    /** The exploded time value. */
    RTTIME      m_Exploded;
    /** Set if m_TimeSpec is okay, consult m_strFormatted if not. */
    bool        m_fTimeSpecOkay;
    /** The format / format hint. */
    kFormat     m_enmFormat;
    /** The formatted date string.
     * This will be the raw input string for a deserialized value, where as for
     * a value set by the user it will be the formatted value. */
    RTCString   m_strFormatted;

    /**
     * Explodes and formats the m_TimeSpec value.
     *
     * Sets m_Exploded, m_strFormatted, m_fTimeSpecOkay, and m_enmFormat, clears m_fNullIndicator.
     *
     * @returns VINF_SUCCESS or VERR_NO_STR_MEMORY.
     * @param   a_enmFormat The format to use.
     */
    int explodeAndFormat(kFormat a_enmFormat) RT_NOEXCEPT;

    /**
     * Formats the m_Exploded value.
     *
     * Sets m_strFormatted, m_fTimeSpecOkay, and m_enmFormat, clears m_fNullIndicator.
     *
     * @returns VINF_SUCCESS or VERR_NO_STR_MEMORY.
     * @param   a_enmFormat The format to use.
     */
    int format(kFormat a_enmFormat) RT_NOEXCEPT;

    /**
     * Internal worker that attempts to decode m_strFormatted.
     *
     * Sets m_fTimeSpecOkay.
     *
     * @returns IPRT status code.
     * @param   enmFormat   Specific format to try, kFormat_Invalid (default) to try guess it.
     */
    int decodeFormattedString(kFormat enmFormat = kFormat_Invalid) RT_NOEXCEPT;
};


/** We should provide a proper UUID class eventually.  Currently it is not used. */
typedef RTCRestString RTCRestUuid;


/**
 * String enum base class.
 */
class RT_DECL_CLASS RTCRestStringEnumBase : public RTCRestObjectBase
{
public:
    /** Enum map entry. */
    typedef struct ENUMMAPENTRY
    {
        const char *pszName;
        uint32_t    cchName;
        int32_t     iValue;
    } ENUMMAPENTRY;

    /** Default constructor. */
    RTCRestStringEnumBase() RT_NOEXCEPT;
    /** Destructor. */
    virtual ~RTCRestStringEnumBase();

    /** Copy constructor. */
    RTCRestStringEnumBase(RTCRestStringEnumBase const &a_rThat);
    /** Copy assignment operator. */
    RTCRestStringEnumBase &operator=(RTCRestStringEnumBase const &a_rThat);

    /** Safe copy assignment method. */
    int assignCopy(RTCRestStringEnumBase const &a_rThat) RT_NOEXCEPT;
    /** Safe copy assignment method. */
    inline int assignCopy(RTCString const &a_rThat) RT_NOEXCEPT    { return setByString(a_rThat); }
    /** Safe copy assignment method. */
    inline int assignCopy(const char *a_pszThat) RT_NOEXCEPT       { return setByString(a_pszThat); }

    /* Overridden methods: */
    virtual int resetToDefault() RT_NOEXCEPT RT_OVERRIDE;
    virtual RTCRestOutputBase &serializeAsJson(RTCRestOutputBase &a_rDst) const RT_NOEXCEPT RT_OVERRIDE;
    virtual int deserializeFromJson(RTCRestJsonCursor const &a_rCursor) RT_NOEXCEPT RT_OVERRIDE;
    virtual int toString(RTCString *a_pDst, uint32_t a_fFlags = kCollectionFormat_Unspecified) const RT_NOEXCEPT RT_OVERRIDE;
    virtual int fromString(RTCString const &a_rValue, const char *a_pszName, PRTERRINFO a_pErrInfo = NULL,
                           uint32_t a_fFlags = kCollectionFormat_Unspecified) RT_NOEXCEPT RT_OVERRIDE;
    virtual kTypeClass typeClass(void) const RT_NOEXCEPT RT_OVERRIDE;

    /**
     * Sets the value given a C-string value.
     *
     * @retval VINF_SUCCESS on success.
     * @retval VWRN_NOT_FOUND if not mappable to enum value.
     * @retval VERR_NO_STR_MEMORY if not mappable and we're out of memory.
     * @param   a_pszValue      The string value.
     * @param   a_cchValue      The string value length.  Optional.
     */
    int setByString(const char *a_pszValue, size_t a_cchValue = RTSTR_MAX) RT_NOEXCEPT;

    /**
     * Sets the value given a string value.
     *
     * @retval VINF_SUCCESS on success.
     * @retval VWRN_NOT_FOUND if not mappable to enum value.
     * @retval VERR_NO_STR_MEMORY if not mappable and we're out of memory.
     * @param   a_rValue        The string value.
     */
    int setByString(RTCString const &a_rValue) RT_NOEXCEPT;

    /**
     * Gets the string value.
     */
    const char *getString() const RT_NOEXCEPT;

    /** Maps the given string value to an enum. */
    int stringToEnum(const char *a_pszValue, size_t a_cchValue = RTSTR_MAX) RT_NOEXCEPT;
    /** Maps the given string value to an enum. */
    int stringToEnum(RTCString const &a_rStrValue) RT_NOEXCEPT;
    /** Maps the given string value to an enum. */
    const char *enumToString(int a_iEnumValue, size_t *a_pcchString) RT_NOEXCEPT;


protected:
    /** The enum value. */
    int                 m_iEnumValue;
    /** The string value if not a match. */
    RTCString           m_strValue;

    /**
     * Worker for setting the object to the given enum value.
     *
     * @retval  true on success.
     * @retval  false if a_iEnumValue can't be translated.
     * @param   a_iEnumValue    The enum value to set.
     */
    bool                setWorker(int a_iEnumValue) RT_NOEXCEPT;

    /** Helper for implementing RTCRestObjectBase::clone(). */
    RTCRestObjectBase  *cloneWorker(RTCRestStringEnumBase *a_pDst) const RT_NOEXCEPT;

    /**
     * Gets the mapping table.
     *
     * @returns Pointer to the translation table.
     * @param   pcEntries   Where to return the translation table size.
     */
    virtual ENUMMAPENTRY const *getMappingTable(size_t *pcEntries) const RT_NOEXCEPT = 0;
};


/**
 * String enum template class.
 *
 * Takes the enum type as argument.
 */
template <typename EnumType>
class RTCRestStringEnum : public RTCRestStringEnumBase
{
public:
    typedef EnumType Type;  /**< The enum type. */

    /** Default constructor */
    RTCRestStringEnum() RT_NOEXCEPT : RTCRestStringEnumBase() { }
    /** Constructor with initial enum value. */
    RTCRestStringEnum(Type a_enmValue) RT_NOEXCEPT : RTCRestStringEnumBase() { set(a_enmValue); }
    /** Constructor with string default. */
    RTCRestStringEnum(const char *a_pszDefault) : RTCRestStringEnumBase() { setByString(a_pszDefault); }
    /** Copy constructor */
    RTCRestStringEnum(RTCRestStringEnum const &a_rThat) : RTCRestStringEnumBase(a_rThat) { }
    /** Make a clone of this object. */
    inline RTCRestStringEnum *clone() const RT_NOEXCEPT { return (RTCRestStringEnum *)baseClone(); }

    virtual RTCRestObjectBase *baseClone() const RT_NOEXCEPT RT_OVERRIDE
    {
        return cloneWorker(new (std::nothrow) RTCRestStringEnum());
    }

    /** Copy assignment operator. */
    RTCRestStringEnum &operator=(RTCRestStringEnum const &a_rThat) RT_NOEXCEPT
    {
        RTCRestStringEnumBase::operator=(a_rThat);
        return *this;
    }

    /**
     * Gets the enum value.
     * @returns enum value.
     * @retval  kXxxxInvalid means there was no mapping for the string, or that
     *          no value has been assigned yet.
     */
    Type get() const RT_NOEXCEPT { return (Type)m_iEnumValue; }

    /**
     * Sets the object value to @a a_enmType
     *
     * @returns true if a_enmType is valid, false if not.
     * @param   a_enmType   The new value.
     */
    bool set(Type a_enmType) RT_NOEXCEPT { return setWorker((int)a_enmType); }

    virtual const char *typeName(void) const RT_NOEXCEPT RT_OVERRIDE { return "RTCRestStringEnum<EnumType>"; }

    /** Factory method. */
    static DECLCALLBACK(RTCRestObjectBase *) createInstance(void) RT_NOEXCEPT
    {
        return new (std::nothrow) RTCRestStringEnum();
    }

    /** @copydoc RTCRestObjectBase::FNDESERIALIZEINSTANCEFROMJSON */
    static DECLCALLBACK(int) deserializeInstanceFromJson(RTCRestJsonCursor const &a_rCursor, RTCRestObjectBase **a_ppInstance) RT_NOEXCEPT
    {
        *a_ppInstance = new (std::nothrow) RTCRestStringEnum();
        if (*a_ppInstance)
            return (*a_ppInstance)->deserializeFromJson(a_rCursor);
        return a_rCursor.m_pPrimary->addError(a_rCursor, VERR_NO_MEMORY, "Out of memory");
    }

protected:
    /** Enum mapping table. */
    static const ENUMMAPENTRY s_aMappingTable[];
    /** Enum mapping table size. */
    static const size_t       s_cMappingTable;

    virtual ENUMMAPENTRY const *getMappingTable(size_t *pcEntries) const RT_NOEXCEPT RT_OVERRIDE
    {
        *pcEntries = s_cMappingTable;
        return s_aMappingTable;
    }
};


/**
 * Class for handling binary blobs (strings).
 *
 * There are specializations of this class for body parameters and responses,
 * see RTCRestBinaryParameter and RTCRestBinaryResponse.
 */
class RT_DECL_CLASS RTCRestBinary : public RTCRestObjectBase
{
public:
    /** Default constructor. */
    RTCRestBinary() RT_NOEXCEPT;
    /** Destructor. */
    virtual ~RTCRestBinary();

    /** Safe copy assignment method. */
    virtual int assignCopy(RTCRestBinary const &a_rThat) RT_NOEXCEPT;
    /** Safe buffer copy method. */
    virtual int assignCopy(void const *a_pvData, size_t a_cbData) RT_NOEXCEPT;

    /** Use the specified data buffer directly. */
    virtual int assignReadOnly(void const *a_pvData, size_t a_cbData) RT_NOEXCEPT;
    /** Use the specified data buffer directly. */
    virtual int assignWriteable(void *a_pvBuf, size_t a_cbBuf) RT_NOEXCEPT;
    /** Frees the data held by the object and resets it default state. */
    virtual void freeData() RT_NOEXCEPT;

    /** Returns a pointer to the data blob. */
    inline const uint8_t  *getPtr()  const RT_NOEXCEPT { return m_pbData; }
    /** Gets the size of the data. */
    inline size_t          getSize() const RT_NOEXCEPT { return m_cbData; }

    /** Make a clone of this object. */
    inline RTCRestBinary  *clone() const RT_NOEXCEPT { return (RTCRestBinary *)baseClone(); }

    /* Overridden methods: */
    virtual RTCRestObjectBase *baseClone() const RT_NOEXCEPT RT_OVERRIDE;
    virtual int setNull(void) RT_NOEXCEPT RT_OVERRIDE;
    virtual int resetToDefault(void) RT_NOEXCEPT RT_OVERRIDE;
    virtual RTCRestOutputBase &serializeAsJson(RTCRestOutputBase &a_rDst) const RT_NOEXCEPT RT_OVERRIDE;
    virtual int deserializeFromJson(RTCRestJsonCursor const &a_rCursor) RT_NOEXCEPT RT_OVERRIDE;
    virtual int toString(RTCString *a_pDst, uint32_t a_fFlags = kCollectionFormat_Unspecified) const RT_NOEXCEPT RT_OVERRIDE;
    virtual int fromString(RTCString const &a_rValue, const char *a_pszName, PRTERRINFO a_pErrInfo = NULL,
                           uint32_t a_fFlags = kCollectionFormat_Unspecified) RT_NOEXCEPT RT_OVERRIDE;
    virtual kTypeClass typeClass(void) const RT_NOEXCEPT RT_OVERRIDE;
    virtual const char *typeName(void) const RT_NOEXCEPT RT_OVERRIDE;

    /** Factory method. */
    static DECLCALLBACK(RTCRestObjectBase *) createInstance(void) RT_NOEXCEPT;
    /** @copydoc RTCRestObjectBase::FNDESERIALIZEINSTANCEFROMJSON */
    static DECLCALLBACK(int) deserializeInstanceFromJson(RTCRestJsonCursor const &a_rCursor, RTCRestObjectBase **a_ppInstance) RT_NOEXCEPT;

protected:
    /** Pointer to data blob. */
    uint8_t    *m_pbData;
    /** Amount of valid data in the blob. */
    size_t      m_cbData;
    /** Number of bytes allocated for the m_pbData buffer. */
    size_t      m_cbAllocated;
    /** Set if the data is freeable, only ever clear if user data. */
    bool        m_fFreeable;
    /** Set if the data blob is readonly user provided data. */
    bool        m_fReadOnly;

private:
    /* No copy constructor or copy assignment: */
    RTCRestBinary(RTCRestBinary const &a_rThat);
    RTCRestBinary &operator=(RTCRestBinary const &a_rThat);
};


/**
 * Abstract base class for REST data model classes.
 */
class RT_DECL_CLASS RTCRestDataObject : public RTCRestObjectBase
{
public:
    RTCRestDataObject() RT_NOEXCEPT;
    RTCRestDataObject(RTCRestDataObject const &a_rThat) RT_NOEXCEPT;
    virtual ~RTCRestDataObject();

    /* Overridden methods:*/
    virtual int resetToDefault() RT_NOEXCEPT RT_OVERRIDE;
    virtual RTCRestOutputBase &serializeAsJson(RTCRestOutputBase &a_rDst) const RT_NOEXCEPT RT_OVERRIDE;
    virtual int deserializeFromJson(RTCRestJsonCursor const &a_rCursor) RT_NOEXCEPT RT_OVERRIDE;
    virtual kTypeClass typeClass(void) const RT_NOEXCEPT RT_OVERRIDE;

    /**
     * Serialize the object members as JSON.
     *
     * @returns a_rDst
     * @param   a_rDst      The destination for the serialization.
     */
    virtual RTCRestOutputBase &serializeMembersAsJson(RTCRestOutputBase &a_rDst) const RT_NOEXCEPT;

    /**
     * Deserialize object from the given JSON iterator.
     *
     * @returns IPRT status code.
     * @retval  VERR_NOT_FOUND if field is unknown.  Top level caller will do
     *          invoke unknownField() on it.
     *
     * @param   a_rCursor   The JSON cursor with the current member.
     * @param   a_cchName   The length of a_rCursor.m_pszName.
     */
    virtual int deserializeMemberFromJson(RTCRestJsonCursor const &a_rCursor, size_t a_cchName) RT_NOEXCEPT;

protected:
    /** The is-set bits for all the fields. */
    uint64_t m_fIsSet;

    /** Copy assignment operator. */
    RTCRestDataObject &operator=(RTCRestDataObject const &a_rThat) RT_NOEXCEPT;

    /** Safe copy assignment method. */
    virtual int assignCopy(RTCRestDataObject const &a_rThat) RT_NOEXCEPT;
};


/**
 * Abstract base class for polymorphic REST data model classes.
 */
class RT_DECL_CLASS RTCRestPolyDataObject : public RTCRestDataObject
{
public:
    RTCRestPolyDataObject() RT_NOEXCEPT;
    RTCRestPolyDataObject(RTCRestPolyDataObject const &a_rThat) RT_NOEXCEPT;
    virtual ~RTCRestPolyDataObject();

    /* Overridden methods:*/
    virtual int resetToDefault() RT_NOEXCEPT RT_OVERRIDE;

    /** Checks if the instance is of a child class (@c true) or of the parent (@c false). */
    virtual bool isChild() const RT_NOEXCEPT;

protected:

    /** Copy assignment operator. */
    RTCRestPolyDataObject &operator=(RTCRestPolyDataObject const &a_rThat) RT_NOEXCEPT;
};


/** @} */

#endif /* !IPRT_INCLUDED_cpp_restbase_h */

