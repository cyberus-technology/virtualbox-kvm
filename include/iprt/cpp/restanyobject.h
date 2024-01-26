/** @file
 * IPRT - C++ Representational State Transfer (REST) Any Object Class.
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

#ifndef IPRT_INCLUDED_cpp_restanyobject_h
#define IPRT_INCLUDED_cpp_restanyobject_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cpp/restbase.h>
#include <iprt/cpp/restarray.h>
#include <iprt/cpp/reststringmap.h>


/** @defgroup grp_rt_cpp_restanyobj C++ Representational State Transfer (REST) Any Object Class.
 * @ingroup grp_rt_cpp
 * @{
 */

/**
 * Wrapper object that can represent any kind of basic REST object.
 *
 * This class is the result of a couple of design choices made in our REST
 * data model.  If could have been avoided if we used pointers all over
 * the place and didn't rely entirely on the object specific implementations
 * of deserializeFromJson and fromString to do the deserializing or everything.
 *
 * The assumption, though, was that most of the data we're dealing with has a
 * known structure and maps to fixed types.  So, the data model was optimized
 * for that rather than flexiblity here.
 */
class RT_DECL_CLASS RTCRestAnyObject : public RTCRestObjectBase
{
public:
    /** Default constructor. */
    RTCRestAnyObject() RT_NOEXCEPT;
    /** Destructor. */
    virtual ~RTCRestAnyObject();

    /** Copy constructor. */
    RTCRestAnyObject(RTCRestAnyObject const &a_rThat);
    /** Copy assignment operator. */
    RTCRestAnyObject &operator=(RTCRestAnyObject const &a_rThat);

    /** Safe copy assignment method. */
    int assignCopy(RTCRestAnyObject const &a_rThat) RT_NOEXCEPT;
    /** Safe copy assignment method, boolean variant. */
    int assignCopy(RTCRestBool const &a_rThat) RT_NOEXCEPT;
    /** Safe copy assignment method, int64_t variant. */
    int assignCopy(RTCRestInt64 const &a_rThat) RT_NOEXCEPT;
    /** Safe copy assignment method, int32_t variant. */
    int assignCopy(RTCRestInt32 const &a_rThat) RT_NOEXCEPT;
    /** Safe copy assignment method, int16_t variant. */
    int assignCopy(RTCRestInt16 const &a_rThat) RT_NOEXCEPT;
    /** Safe copy assignment method, double variant. */
    int assignCopy(RTCRestDouble const &a_rThat) RT_NOEXCEPT;
    /** Safe copy assignment method, string variant. */
    int assignCopy(RTCRestString const &a_rThat) RT_NOEXCEPT;
    /** Safe copy assignment method, array variant. */
    int assignCopy(RTCRestArray<RTCRestAnyObject> const &a_rThat) RT_NOEXCEPT;
    /** Safe copy assignment method, string map variant. */
    int assignCopy(RTCRestStringMap<RTCRestAnyObject> const &a_rThat) RT_NOEXCEPT;

    /** Safe value assignment method, boolean variant. */
    int assignValue(bool a_fValue) RT_NOEXCEPT;
    /** Safe value assignment method, int64_t variant. */
    int assignValue(int64_t a_iValue) RT_NOEXCEPT;
    /** Safe value assignment method, int32_t variant. */
    int assignValue(int32_t a_iValue) RT_NOEXCEPT;
    /** Safe value assignment method, int16_t variant. */
    int assignValue(int16_t a_iValue) RT_NOEXCEPT;
    /** Safe value assignment method, double variant. */
    int assignValue(double a_iValue) RT_NOEXCEPT;
    /** Safe value assignment method, string variant. */
    int assignValue(RTCString const &a_rValue) RT_NOEXCEPT;
    /** Safe value assignment method, C-string variant. */
    int assignValue(const char *a_pszValue) RT_NOEXCEPT;

    /** Make a clone of this object. */
    inline RTCRestAnyObject *clone() const RT_NOEXCEPT { return (RTCRestAnyObject *)baseClone(); }

    /* Overridden methods: */
    virtual RTCRestObjectBase *baseClone() const RT_NOEXCEPT RT_OVERRIDE;
    virtual int setNull(void) RT_NOEXCEPT RT_OVERRIDE;
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
    /** Deserialization w/ instantiation. */
    static FNDESERIALIZEINSTANCEFROMJSON deserializeInstanceFromJson;

protected:
    /** The data. */
    RTCRestObjectBase *m_pData;
};

/** @} */

#endif /* !IPRT_INCLUDED_cpp_restanyobject_h */

