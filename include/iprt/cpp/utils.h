/** @file
 * IPRT - C++ Utilities (useful templates, defines and such).
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

#ifndef IPRT_INCLUDED_cpp_utils_h
#define IPRT_INCLUDED_cpp_utils_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

/** @defgroup grp_rt_cpp        IPRT C++ APIs */

/** @defgroup grp_rt_cpp_util   C++ Utilities
 * @ingroup grp_rt_cpp
 * @{
 */

#define DPTR(CLASS) CLASS##Private *d = static_cast<CLASS##Private *>(d_ptr)
#define QPTR(CLASS) CLASS *q = static_cast<CLASS *>(q_ptr)

/**
 * A simple class used to prevent copying and assignment.
 *
 * Inherit from this class in order to prevent automatic generation
 * of the copy constructor and assignment operator in your class.
 */
class RTCNonCopyable
{
protected:
    RTCNonCopyable() {}
    ~RTCNonCopyable() {}
private:
    RTCNonCopyable(RTCNonCopyable const &);
    RTCNonCopyable &operator=(RTCNonCopyable const &);
};


/**
 * Shortcut to |const_cast<C &>()| that automatically derives the correct
 * type (class) for the const_cast template's argument from its own argument.
 *
 * Can be used to temporarily cancel the |const| modifier on the left-hand side
 * of assignment expressions, like this:
 * @code
 *      const Class That;
 *      ...
 *      unconst(That) = SomeValue;
 * @endcode
 *
 * @todo What to do about the prefix here?
 */
template <class C>
inline C &unconst(const C &that)
{
    return const_cast<C &>(that);
}


/**
 * Shortcut to |const_cast<C *>()| that automatically derives the correct
 * type (class) for the const_cast template's argument from its own argument.
 *
 * Can be used to temporarily cancel the |const| modifier on the left-hand side
 * of assignment expressions, like this:
 * @code
 *      const Class *pThat;
 *      ...
 *      unconst(pThat) = SomeValue;
 * @endcode
 *
 * @todo What to do about the prefix here?
 */
template <class C>
inline C *unconst(const C *that)
{
    return const_cast<C *>(that);
}


/**
 * Macro for generating a non-const getter version from a const getter.
 *
 * @param   a_RetType       The getter return type.
 * @param   a_Class         The class name.
 * @param   a_Getter        The getter name.
 * @param   a_ArgDecls      The argument declaration for the getter method.
 * @param   a_ArgList       The argument list for the call.
 */
#define RT_CPP_GETTER_UNCONST(a_RetType, a_Class, a_Getter, a_ArgDecls, a_ArgList) \
    a_RetType a_Getter a_ArgDecls \
    {  \
        return static_cast< a_Class const *>(this)-> a_Getter a_ArgList; \
    }


/**
 * Macro for generating a non-const getter version from a const getter,
 * unconsting the return value as well.
 *
 * @param   a_RetType       The getter return type.
 * @param   a_Class         The class name.
 * @param   a_Getter        The getter name.
 * @param   a_ArgDecls      The argument declaration for the getter method.
 * @param   a_ArgList       The argument list for the call.
 */
#define RT_CPP_GETTER_UNCONST_RET(a_RetType, a_Class, a_Getter, a_ArgDecls, a_ArgList) \
    a_RetType a_Getter a_ArgDecls \
    {  \
        return const_cast<a_RetType>(static_cast< a_Class const *>(this)-> a_Getter a_ArgList); \
    }


/** @} */

#endif /* !IPRT_INCLUDED_cpp_utils_h */

