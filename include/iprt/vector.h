/** @file
 * IPRT - Vector - STL-inspired vector implementation in C.
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

/**
 * @todo the right Doxygen tag here
 * This file defines a set of macros which provide a functionality and an
 * interface roughly similar to the C++ STL vector container.  To create a
 * vector of a particular type one must first explicitly instantiate such a
 * vector in the source file, e.g.
 *   RTVEC_DECL(TopLevels, Window *)
 * without a semi-colon.  This macro will define a structure (struct TopLevels)
 * which contains a dynamically resizeable array of Window * elements.  It
 * will also define a number of inline methods for manipulating the structure,
 * such as
 *   Window *TopLevelsPushBack(struct TopLevels *)
 * which adds a new element to the end of the array and returns it, optionally
 * reallocating the array if there is not enough space for the new element.
 * (This particular method prototype differs from the STL equivalent -
 * push_back - more than most of the other methods).
 *
 * To create a vector, one simply needs to declare the structure, in this case
 *   struct TopLevels = RTVEC_INITIALIZER;
 *
 * There are various other macros for declaring vectors with different
 * allocators (e.g. RTVEC_DECL_ALLOCATOR) or with clean-up functions
 * (e.g. RTVEC_DECL_DELETE).  See the descriptions of the generic methods and
 * the declarator macros below.
 *
 * One particular use of vectors is to assemble an array of a particular type
 * in heap memory without knowing - or counting - the number of elements in
 * advance.  To do this, add the elements onto the array using PushBack, then
 * extract the array from the vector using the (non-STL) Detach method.
 *
 * @note functions in this file are inline to prevent warnings about
 *       unused static functions.  I assume that in this day and age a
 *       compiler makes its own decisions about whether to actually
 *       inline a function.
 * @note since vector structures must be explicitly instanciated unlike the
 *       C++ vector template, care must be taken not to instanciate a
 *       particular type twice, e.g. once in a header and once in a code file.
 *       Only using vectors in code files and keeping them out of interfaces
 *       (or passing them as anonymously) makes it easier to take care of this.
 */

#ifndef IPRT_INCLUDED_vector_h
#define IPRT_INCLUDED_vector_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/

#include <iprt/assert.h>
#include <iprt/cdefs.h>
#include <iprt/errcore.h>
#include <iprt/mem.h>  /** @todo Should the caller include this if they need
                        *        it? */


/**
 * Generic vector structure
 */
/** @todo perhaps we should include an additional member for a parameter to
 * three-argument reallocators, so that we can support e.g. mempools? */
#define RTVEC_DECL_STRUCT(name, type)                                      \
struct name                                                                \
{                                                                          \
    /** The number of elements in the vector */                            \
    size_t mcElements;                                                     \
    /** The current capacity of the vector */                              \
    size_t mcCapacity;                                                     \
    /** The elements themselves */                                         \
    type *mpElements;                                                      \
};

/** Initialiser for an empty vector structure */
#define RTVEC_INITIALIZER { 0, 0, NULL }

/** The unit by which the vector capacity is increased */
#define RTVECIMPL_ALLOC_UNIT 16

/**
 * Generic method - get the size of a vector
 */
/** @todo What is the correct way to do doxygen for this sort of macro? */
#define RTVEC_DECLFN_SIZE(name, type)                                      \
DECLINLINE(size_t) name ## Size(struct name *pVec)                         \
{                                                                          \
    return(pVec->mcElements);                                              \
}

/**
 * Generic method - expand a vector
 */
#define RTVEC_DECLFN_RESERVE(name, type, pfnRealloc)                       \
DECLINLINE(int) name ## Reserve(struct name *pVec, size_t cNewCapacity)    \
{                                                                          \
    void *pvNew;                                                           \
                                                                           \
    if (cNewCapacity <= pVec->mcCapacity)                                  \
        return VINF_SUCCESS;                                               \
    pvNew = pfnRealloc(pVec->mpElements, cNewCapacity * sizeof(type));     \
    if (!pvNew)                                                            \
        return VERR_NO_MEMORY;                                             \
    pVec->mcCapacity = cNewCapacity;                                       \
    pVec->mpElements = (type *)pvNew;                                      \
    return VINF_SUCCESS;                                                   \
}

/**
 * Generic method - return a pointer to the first element in the vector.
 */
#define RTVEC_DECLFN_BEGIN(name, type)                                     \
DECLINLINE(type *) name ## Begin(struct name *pVec)                        \
{                                                                          \
    return(pVec->mpElements);                                              \
}

/**
 * Generic method - return a pointer to one past the last element in the
 * vector.
 */
#define RTVEC_DECLFN_END(name, type)                                       \
DECLINLINE(type *) name ## End(struct name *pVec)                          \
{                                                                          \
    return(&pVec->mpElements[pVec->mcElements]);                           \
}

/**
 * Generic method - add a new, uninitialised element onto a vector and return
 * it.
 * @note this method differs from the STL equivalent by letting the caller
 *       post-initialise the new element rather than copying it from its
 *       argument.
 */
#define RTVEC_DECLFN_PUSHBACK(name, type)                                  \
DECLINLINE(type *) name ## PushBack(struct name *pVec)                     \
{                                                                          \
    Assert(pVec->mcElements <= pVec->mcCapacity);                          \
    if (   pVec->mcElements == pVec->mcCapacity                            \
        && RT_FAILURE(name ## Reserve(pVec,   pVec->mcCapacity             \
                                            + RTVECIMPL_ALLOC_UNIT)))      \
        return NULL;                                                       \
    ++pVec->mcElements;                                                    \
    return &pVec->mpElements[pVec->mcElements - 1];                        \
}

/**
 * Generic method - drop the last element from the vector.
 */
#define RTVEC_DECLFN_POPBACK(name)                                         \
DECLINLINE(void) name ## PopBack(struct name *pVec)                        \
{                                                                          \
    Assert(pVec->mcElements <= pVec->mcCapacity);                          \
    --pVec->mcElements;                                                    \
}

/**
 * Generic method - drop the last element from the vector, calling a clean-up
 * method first.
 *
 * By taking an adapter function for the element to be dropped as an
 * additional macro parameter we can support clean-up by pointer
 * (pfnAdapter maps T* -> T*) or by value (maps T* -> T).  pfnAdapter takes
 * one argument of type @a type * and must return whatever type pfnDelete
 * expects.
 */
/** @todo find a better name for pfnAdapter? */
#define RTVEC_DECLFN_POPBACK_DELETE(name, type, pfnDelete, pfnAdapter)     \
DECLINLINE(void) name ## PopBack(struct name *pVec)                        \
{                                                                          \
    Assert(pVec->mcElements <= pVec->mcCapacity);                          \
    --pVec->mcElements;                                                    \
    pfnDelete(pfnAdapter(&pVec->mpElements[pVec->mcElements]));            \
}

/**
 * Generic method - reset a vector to empty.
 * @note This function does not free any memory
 */
#define RTVEC_DECLFN_CLEAR(name)                                           \
DECLINLINE(void) name ## Clear(struct name *pVec)                          \
{                                                                          \
    Assert(pVec->mcElements <= pVec->mcCapacity);                          \
    pVec->mcElements = 0;                                                  \
}

/**
 * Generic method - reset a vector to empty, calling a clean-up method on each
 * element first.
 * @note See @a RTVEC_DECLFN_POPBACK_DELETE for an explanation of pfnAdapter
 * @note This function does not free any memory
 * @note The cleanup function is currently called on the elements from first
 *       to last.  The testcase expects this.
 */
#define RTVEC_DECLFN_CLEAR_DELETE(name, pfnDelete, pfnAdapter)             \
DECLINLINE(void) name ## Clear(struct name *pVec)                          \
{                                                                          \
    size_t i;                                                              \
                                                                           \
    Assert(pVec->mcElements <= pVec->mcCapacity);                          \
    for (i = 0; i < pVec->mcElements; ++i)                                 \
        pfnDelete(pfnAdapter(&pVec->mpElements[i]));                       \
    pVec->mcElements = 0;                                                  \
}

/**
 * Generic method - detach the array contained inside a vector and reset the
 * vector to empty.
 * @note This function does not free any memory
 */
#define RTVEC_DECLFN_DETACH(name, type)                                    \
DECLINLINE(type *) name ## Detach(struct name *pVec)                       \
{                                                                          \
    type *pArray = pVec->mpElements;                                       \
                                                                           \
    Assert(pVec->mcElements <= pVec->mcCapacity);                          \
    pVec->mcElements = 0;                                                  \
    pVec->mpElements = NULL;                                               \
    pVec->mcCapacity = 0;                                                  \
    return pArray;                                                         \
}

/** Common declarations for all vector types */
#define RTVEC_DECL_COMMON(name, type, pfnRealloc)                          \
    RTVEC_DECL_STRUCT(name, type)                                          \
    RTVEC_DECLFN_SIZE(name, type)                                          \
    RTVEC_DECLFN_RESERVE(name, type, pfnRealloc)                           \
    RTVEC_DECLFN_BEGIN(name, type)                                         \
    RTVEC_DECLFN_END(name, type)                                           \
    RTVEC_DECLFN_PUSHBACK(name, type)                                      \
    RTVEC_DECLFN_DETACH(name, type)

/**
 * Declarator macro - declare a vector type
 * @param  name        the name of the C struct type describing the vector as
 *                     well as the prefix of the functions for manipulating it
 * @param  type        the type of the objects contained in the vector
 * @param  pfnRealloc  the memory reallocation function used for expanding the
 *                     vector
 */
#define RTVEC_DECL_ALLOCATOR(name, type, pfnRealloc)                       \
    RTVEC_DECL_COMMON(name, type, pfnRealloc)                              \
    RTVEC_DECLFN_POPBACK(name)                                             \
    RTVEC_DECLFN_CLEAR(name)

/**
 * Generic method - inline id mapping delete adapter function - see the
 * explanation of pfnAdapter in @a RTVEC_DECLFN_POPBACK_DELETE.
 */
#define RTVEC_DECLFN_DELETE_ADAPTER_ID(name, type)                         \
DECLINLINE(type *) name ## DeleteAdapterId(type *arg)                      \
{                                                                          \
    return arg;                                                            \
}

/**
 * Generic method - inline pointer-to-value mapping delete adapter function -
 * see the explanation of pfnAdapter in @a RTVEC_DECLFN_POPBACK_DELETE.
 */
#define RTVEC_DECLFN_DELETE_ADAPTER_TO_VALUE(name, type)                   \
DECLINLINE(type) name ## DeleteAdapterToValue(type *arg)                   \
{                                                                          \
    return *arg;                                                           \
}

/**
 * Declarator macro - declare a vector type with a cleanup callback to be used
 * when elements are dropped from the vector.  The callback takes a pointer to
 * @a type,
 * NOT a value of type @a type.
 * @param  name        the name of the C struct type describing the vector as
 *                     well as the prefix of the functions for manipulating it
 * @param  type        the type of the objects contained in the vector
 * @param  pfnRealloc  the memory reallocation function used for expanding the
 *                     vector
 * @param  pfnDelete   the cleanup callback function - signature
 *                     void pfnDelete(type *)
 */
#define RTVEC_DECL_ALLOCATOR_DELETE(name, type, pfnRealloc, pfnDelete)     \
    RTVEC_DECL_COMMON(name, type, pfnRealloc)                              \
    RTVEC_DECLFN_DELETE_ADAPTER_ID(name, type)                             \
    RTVEC_DECLFN_POPBACK_DELETE(name, type, pfnDelete,                     \
                                name ## DeleteAdapterId)                   \
    RTVEC_DECLFN_CLEAR_DELETE(name, pfnDelete, name ## DeleteAdapterId)

/**
 * Declarator macro - declare a vector type with a cleanup callback to be used
 * when elements are dropped from the vector.  The callback takes a parameter
 * of type @a type, NOT a pointer to @a type.
 * @param  name        the name of the C struct type describing the vector as
 *                     well as the prefix of the functions for manipulating it
 * @param  type        the type of the objects contained in the vector
 * @param  pfnRealloc  the memory reallocation function used for expanding the
 *                     vector
 * @param  pfnDelete   the cleanup callback function - signature
 *                     void pfnDelete(type)
 */
#define RTVEC_DECL_ALLOCATOR_DELETE_BY_VALUE(name, type, pfnRealloc,       \
                                             pfnDelete)                    \
    RTVEC_DECL_COMMON(name, type, pfnRealloc)                              \
    RTVEC_DECLFN_DELETE_ADAPTER_TO_VALUE(name, type)                       \
    RTVEC_DECLFN_POPBACK_DELETE(name, type, pfnDelete,                     \
                                name ## DeleteAdapterToValue)              \
    RTVEC_DECLFN_CLEAR_DELETE(name, pfnDelete,                             \
                              name ## DeleteAdapterToValue)

/**
 * Inline wrapper around RTMemRealloc macro to get a function usable as a
 * callback.
 */
DECLINLINE(void *) rtvecReallocDefTag(void *pv, size_t cbNew)
{
    return RTMemRealloc(pv, cbNew);
}

/**
 * Declarator macro - declare a vector type (see @a RTVEC_DECL_ALLOCATOR)
 * using RTMemRealloc as a memory allocator
 * @param  name        the name of the C struct type describing the vector as
 *                     well as the prefix of the functions for manipulating it
 * @param  type        the type of the objects contained in the vector
 */
#define RTVEC_DECL(name, type)                                             \
    RTVEC_DECL_ALLOCATOR(name, type, rtvecReallocDefTag)

/**
 * Declarator macro - declare a vector type with a cleanup by pointer callback
 * (see @a RTVEC_DECL_ALLOCATOR_DELETE) using RTMemRealloc as a memory
 * allocator
 * @param  name        the name of the C struct type describing the vector as
 *                     well as the prefix of the functions for manipulating it
 * @param  type        the type of the objects contained in the vector
 * @param  pfnDelete   the cleanup callback function - signature
 *                     void pfnDelete(type *)
 */
#define RTVEC_DECL_DELETE(name, type, pfnDelete)                           \
    RTVEC_DECL_ALLOCATOR_DELETE(name, type, rtvecReallocDefTag, pfnDelete)

/**
 * Declarator macro - declare a vector type with a cleanup by value callback
 * (see @a RTVEC_DECL_ALLOCATOR_DELETE_BY_VALUE) using RTMemRealloc as a memory
 * allocator
 * @param  name        the name of the C struct type describing the vector as
 *                     well as the prefix of the functions for manipulating it
 * @param  type        the type of the objects contained in the vector
 * @param  pfnDelete   the cleanup callback function - signature
 *                     void pfnDelete(type)
 */
#define RTVEC_DECL_DELETE_BY_VALUE(name, type, pfnDelete)                  \
    RTVEC_DECL_ALLOCATOR_DELETE_BY_VALUE(name, type, rtvecReallocDefTag,   \
                                         pfnDelete)

#endif /* !IPRT_INCLUDED_vector_h */

