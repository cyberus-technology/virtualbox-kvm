/** @file
 * IPRT - Generic Doubly Linked List, using 32-bit offset instead of pointers.
 */

/*
 * Copyright (C) 2010-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_list_off32_h
#define IPRT_INCLUDED_list_off32_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

/** @defgroup grp_rt_list_off32 RTListOff32 - Generic Doubly Linked List based on 32-bit offset.
 * @ingroup grp_rt
 *
 * This is the same as @ref grp_rt_list , except that instead of pointers we use
 * 32-bit offsets.  The list implementation is circular, with a dummy node as
 * anchor.  Be careful with the dummy node when walking the list.
 *
 * @{
 */

RT_C_DECLS_BEGIN

/**
 * A list node of a doubly linked list.
 */
typedef struct RTLISTOFF32NODE
{
    /** Offset to the next list node, relative to this structure. */
    int32_t offNext;
    /** Offset to the previous list node, relative to this structure. */
    int32_t offPrev;
} RTLISTOFF32NODE;
/** Pointer to a list node. */
typedef RTLISTOFF32NODE *PRTLISTOFF32NODE;
/** Pointer to a const list node. */
typedef RTLISTOFF32NODE const *PCRTLISTOFF32NODE;
/** Pointer to a list node pointer. */
typedef PRTLISTOFF32NODE *PPRTLISTOFF32NODE;

/** The anchor (head/tail) of a doubly linked list.
 *
 * @remarks Please always use this instead of RTLISTOFF32NODE to indicate a list
 *          head/tail.  It makes the code so much easier to read.  Also,
 *          always mention the actual list node type(s) in the comment.
 * @remarks Must be allocated in a similar manner as the nodes, so as to
 *          keep it within a 32-bit distance from them.
 */
typedef RTLISTOFF32NODE RTLISTOFF32ANCHOR;
/** Pointer to a doubly linked list anchor. */
typedef RTLISTOFF32ANCHOR *PRTLISTOFF32ANCHOR;
/** Pointer to a const doubly linked list anchor. */
typedef RTLISTOFF32ANCHOR const *PCRTLISTOFF32ANCHOR;


/**
 * Initialize a list.
 *
 * @param   pList               Pointer to an unitialised list.
 */
DECLINLINE(void) RTListOff32Init(PRTLISTOFF32NODE pList)
{
    pList->offNext = 0;
    pList->offPrev = 0;
}

/**
 * Internal macro for converting an offset to a pointer.
 * @returns PRTLISTOFF32NODE
 * @param   a_pNode             The node the offset is relative to.
 * @param   a_off               The offset.
 */
#define RTLISTOFF32_TO_PTR(a_pNode, a_off)          ((PRTLISTOFF32NODE)((intptr_t)(a_pNode) + (a_off)))

/**
 * Internal macro for getting the pointer to the next node.
 * @returns PRTLISTOFF32NODE
 * @param   a_pNode             The node the offset is relative to.
 */
#define RTLISTOFF32_NEXT_PTR(a_pNode)               RTLISTOFF32_TO_PTR(a_pNode, (a_pNode)->offNext)

/**
 * Internal macro for getting the pointer to the previous node.
 * @returns PRTLISTOFF32NODE
 * @param   a_pNode             The node the offset is relative to.
 */
#define RTLISTOFF32_PREV_PTR(a_pNode)               RTLISTOFF32_TO_PTR(a_pNode, (a_pNode)->offPrev)

/**
 * Internal macro for converting an a pointer to an offset.
 * @returns offset
 * @param   a_pNode             The node the offset is relative to.
 * @param   a_pOtherNode        The pointer to convert.
 */
#define RTLISTOFF32_TO_OFF(a_pNode, a_pOtherNode)   ((int32_t)((intptr_t)(a_pOtherNode) - (intptr_t)(a_pNode)))

/**
 * Internal macro for getting the pointer to the next node.
 * @returns PRTLISTOFF32NODE
 * @param   a_pNode             The node which offNext member should be set.
 * @param   a_pNewNext          Pointer to the new next node.
 */
#define RTLISTOFF32_SET_NEXT_PTR(a_pNode, a_pNewNext) \
    do { (a_pNode)->offNext = RTLISTOFF32_TO_OFF(a_pNode, a_pNewNext); } while (0)

/**
 * Internal macro for getting the pointer to the previous node.
 * @returns PRTLISTOFF32NODE
 * @param   a_pNode             The node which offPrev member should be set.
 * @param   a_pNewPrev          Pointer to the new previous node.
 */
#define RTLISTOFF32_SET_PREV_PTR(a_pNode, a_pNewPrev) \
    do { (a_pNode)->offPrev = RTLISTOFF32_TO_OFF(a_pNode, a_pNewPrev); } while (0)



/**
 * Append a node to the end of the list.
 *
 * @param   pList               The list to append the node to.
 * @param   pNode               The node to append.
 */
DECLINLINE(void) RTListOff32Append(PRTLISTOFF32NODE pList, PRTLISTOFF32NODE pNode)
{
    PRTLISTOFF32NODE pLast = RTLISTOFF32_PREV_PTR(pList);
    RTLISTOFF32_SET_NEXT_PTR(pLast, pNode);
    RTLISTOFF32_SET_PREV_PTR(pNode, pLast);
    RTLISTOFF32_SET_NEXT_PTR(pNode, pList);
    RTLISTOFF32_SET_PREV_PTR(pList, pNode);
}

/**
 * Add a node as the first element of the list.
 *
 * @param   pList               The list to prepend the node to.
 * @param   pNode               The node to prepend.
 */
DECLINLINE(void) RTListOff32Prepend(PRTLISTOFF32NODE pList, PRTLISTOFF32NODE pNode)
{
    PRTLISTOFF32NODE pFirst = RTLISTOFF32_NEXT_PTR(pList);
    RTLISTOFF32_SET_PREV_PTR(pFirst, pNode);
    RTLISTOFF32_SET_NEXT_PTR(pNode, pFirst);
    RTLISTOFF32_SET_PREV_PTR(pNode, pList);
    RTLISTOFF32_SET_NEXT_PTR(pList, pNode);
}

/**
 * Inserts a node after the specified one.
 *
 * @param   pCurNode            The current node.
 * @param   pNewNode            The node to insert.
 */
DECLINLINE(void) RTListOff32NodeInsertAfter(PRTLISTOFF32NODE pCurNode, PRTLISTOFF32NODE pNewNode)
{
    RTListOff32Prepend(pCurNode, pNewNode);
}

/**
 * Inserts a node before the specified one.
 *
 * @param   pCurNode            The current node.
 * @param   pNewNode            The node to insert.
 */
DECLINLINE(void) RTListOff32NodeInsertBefore(PRTLISTOFF32NODE pCurNode, PRTLISTOFF32NODE pNewNode)
{
    RTListOff32Append(pCurNode, pNewNode);
}

/**
 * Remove a node from a list.
 *
 * @param   pNode               The node to remove.
 */
DECLINLINE(void) RTListOff32NodeRemove(PRTLISTOFF32NODE pNode)
{
    PRTLISTOFF32NODE pPrev = RTLISTOFF32_PREV_PTR(pNode);
    PRTLISTOFF32NODE pNext = RTLISTOFF32_NEXT_PTR(pNode);

    RTLISTOFF32_SET_NEXT_PTR(pPrev, pNext);
    RTLISTOFF32_SET_PREV_PTR(pNext, pPrev);

    /* poison */
    pNode->offNext = INT32_MAX / 2;
    pNode->offPrev = INT32_MAX / 2;
}

/**
 * Checks if a node is the last element in the list.
 *
 * @retval  true if the node is the last element in the list.
 * @retval  false otherwise
 *
 * @param   pList               The list.
 * @param   pNode               The node to check.
 */
#define RTListOff32NodeIsLast(pList, pNode)  (RTLISTOFF32_NEXT_PTR(pNode) == (pList))

/**
 * Checks if a node is the first element in the list.
 *
 * @retval  true if the node is the first element in the list.
 * @retval  false otherwise.
 *
 * @param   pList               The list.
 * @param   pNode               The node to check.
 */
#define RTListOff32NodeIsFirst(pList, pNode) (RTLISTOFF32_PREV_PTR(pNode) == (pList))

/**
 * Checks if a type converted node is actually the dummy element (@a pList).
 *
 * @retval  true if the node is the dummy element in the list.
 * @retval  false otherwise.
 *
 * @param   pList               The list.
 * @param   pNode               The node structure to check.  Typically
 *                              something obtained from RTListOff32NodeGetNext()
 *                              or RTListOff32NodeGetPrev().  This is NOT a
 *                              PRTLISTOFF32NODE but something that contains a
 *                              RTLISTOFF32NODE member!
 * @param   Type                Structure the list node is a member of.
 * @param   Member              The list node member.
 */
#define RTListOff32NodeIsDummy(pList, pNode, Type, Member) \
         ( (pNode) == RT_FROM_MEMBER((pList), Type, Member) )
/** @copydoc RTListOff32NodeIsDummy */
#define RTListOff32NodeIsDummyCpp(pList, pNode, Type, Member) \
         ( (pNode) == RT_FROM_CPP_MEMBER((pList), Type, Member) )

/**
 * Checks if a list is empty.
 *
 * @retval  true if the list is empty.
 * @retval  false otherwise.
 *
 * @param   pList               The list to check.
 */
#define RTListOff32IsEmpty(pList)   ((pList)->offNext == 0)

/**
 * Returns the next node in the list.
 *
 * @returns The next node.
 *
 * @param   pCurNode            The current node.
 * @param   Type                Structure the list node is a member of.
 * @param   Member              The list node member.
 */
#define RTListOff32NodeGetNext(pCurNode, Type, Member) \
    RT_FROM_MEMBER(RTLISTOFF32_NEXT_PTR(pCurNode), Type, Member)
/** @copydoc RTListOff32NodeGetNext */
#define RTListOff32NodeGetNextCpp(pCurNode, Type, Member) \
    RT_FROM_CPP_MEMBER(RTLISTOFF32_NEXT_PTR(pCurNode), Type, Member)

/**
 * Returns the previous node in the list.
 *
 * @returns The previous node.
 *
 * @param   pCurNode            The current node.
 * @param   Type                Structure the list node is a member of.
 * @param   Member              The list node member.
 */
#define RTListOff32NodeGetPrev(pCurNode, Type, Member) \
    RT_FROM_MEMBER(RTLISTOFF32_PREV_PTR(pCurNode), Type, Member)
/** @copydoc RTListOff32NodeGetPrev */
#define RTListOff32NodeGetPrevCpp(pCurNode, Type, Member) \
    RT_FROM_CPP_MEMBER(RTLISTOFF32_PREV_PTR(pCurNode), Type, Member)

/**
 * Returns the first element in the list (checks for empty list).
 *
 * @retval  Pointer to the first list element.
 * @retval  NULL if the list is empty.
 *
 * @param   pList               List to get the first element from.
 * @param   Type                Structure the list node is a member of.
 * @param   Member              The list node member.
 */
#define RTListOff32GetFirst(pList, Type, Member) \
    ((pList)->offNext != 0 ? RTListOff32NodeGetNext(pList, Type, Member) : NULL)
/** @copydoc RTListOff32GetFirst */
#define RTListOff32GetFirstCpp(pList, Type, Member) \
    ((pList)->offNext != 0 ? RTListOff32NodeGetNextCpp(pList, Type, Member) : NULL)

/**
 * Returns the last element in the list (checks for empty list).
 *
 * @retval  Pointer to the last list element.
 * @retval  NULL if the list is empty.
 *
 * @param   pList               List to get the last element from.
 * @param   Type                Structure the list node is a member of.
 * @param   Member              The list node member.
 */
#define RTListOff32GetLast(pList, Type, Member) \
    ((pList)->offPrev != 0 ? RTListOff32NodeGetPrev(pList, Type, Member) : NULL)
/** @copydoc RTListOff32GetLast */
#define RTListOff32GetLastCpp(pList, Type, Member) \
    ((pList)->offPrev != 0 ? RTListOff32NodeGetPrevCpp(pList, Type, Member) : NULL)

/**
 * Returns the next node in the list or NULL if the end has been reached.
 *
 * @returns The next node or NULL.
 *
 * @param   pList               The list @a pCurNode is linked on.
 * @param   pCurNode            The current node, of type @a Type.
 * @param   Type                Structure the list node is a member of.
 * @param   Member              The list node member.
 */
#define RTListOff32GetNext(pList, pCurNode, Type, Member) \
    ( RTLISTOFF32_NEXT_PTR(&(pCurNode)->Member) != (pList) \
      ? RT_FROM_MEMBER(RTLISTOFF32_NEXT_PTR(&(pCurNode)->Member), Type, Member) : NULL )
/** @copydoc RTListOff32GetNext */
#define RTListOff32GetNextCpp(pList, pCurNode, Type, Member) \
    ( RTLISTOFF32_NEXT_PTR(&(pCurNode)->Member) != (pList) \
      ? RT_FROM_CPP_MEMBER(RTLISTOFF32_NEXT_PTR(&(pCurNode)->Member), Type, Member) : NULL )

/**
 * Returns the previous node in the list or NULL if the start has been reached.
 *
 * @returns The previous node or NULL.
 *
 * @param   pList               The list @a pCurNode is linked on.
 * @param   pCurNode            The current node, of type @a Type.
 * @param   Type                Structure the list node is a member of.
 * @param   Member              The list node member.
 */
#define RTListOff32GetPrev(pList, pCurNode, Type, Member) \
    ( RTLISTOFF32_PREV_PTR(&(pCurNode)->Member) != (pList) \
      ? RT_FROM_MEMBER(RTLISTOFF32_PREV_PTR(&(pCurNode)->Member), Type, Member) : NULL )
/** @copydoc RTListOff32GetPrev */
#define RTListOff32GetPrevCpp(pList, pCurNode, Type, Member) \
    ( RTLISTOFF32_PREV_PTR(&(pCurNode)->Member) != (pList) \
      ? RT_FROM_CPP_MEMBER(RTLISTOFF32_PREV_PTR(&(pCurNode)->Member), Type, Member) : NULL )

/**
 * Enumerate the list in head to tail order.
 *
 * @param   pList               List to enumerate.
 * @param   pIterator           The iterator variable name.
 * @param   Type                Structure the list node is a member of.
 * @param   Member              The list node member name.
 */
#define RTListOff32ForEach(pList, pIterator, Type, Member) \
    for (pIterator = RTListOff32NodeGetNext(pList, Type, Member); \
         !RTListOff32NodeIsDummy(pList, pIterator, Type, Member); \
         pIterator = RT_FROM_MEMBER(RTLISTOFF32_NEXT_PTR(&(pIterator)->Member), Type, Member) )
/** @copydoc RTListOff32ForEach */
#define RTListOff32ForEachCpp(pList, pIterator, Type, Member) \
    for (pIterator = RTListOff32NodeGetNextCpp(pList, Type, Member); \
         !RTListOff32NodeIsDummyCpp(pList, pIterator, Type, Member); \
         pIterator = RT_FROM_CPP_MEMBER(RTLISTOFF32_NEXT_PTR(&(pIterator)->Member), Type, Member) )


/**
 * Enumerate the list in head to tail order, safe against removal of the
 * current node.
 *
 * @param   pList               List to enumerate.
 * @param   pIterator           The iterator variable name.
 * @param   pIterNext           The name of the variable saving the pointer to
 *                              the next element.
 * @param   Type                Structure the list node is a member of.
 * @param   Member              The list node member name.
 */
#define RTListOff32ForEachSafe(pList, pIterator, pIterNext, Type, Member) \
    for (pIterator = RTListOff32NodeGetNext(pList, Type, Member), \
         pIterNext = RT_FROM_MEMBER(RTLISTOFF32_NEXT_PTR(&(pIterator)->Member), Type, Member); \
         !RTListOff32NodeIsDummy(pList, pIterator, Type, Member); \
         pIterator = pIterNext, \
         pIterNext = RT_FROM_MEMBER(RTLISTOFF32_NEXT_PTR(&(pIterator)->Member), Type, Member) )
/** @copydoc RTListOff32ForEachSafe */
#define RTListOff32ForEachSafeCpp(pList, pIterator, pIterNext, Type, Member) \
    for (pIterator = RTListOff32NodeGetNextCpp(pList, Type, Member), \
         pIterNext = RT_FROM_CPP_MEMBER(RTLISTOFF32_NEXT_PTR(&(pIterator)->Member), Type, Member); \
         !RTListOff32NodeIsDummyCpp(pList, pIterator, Type, Member); \
         pIterator = pIterNext, \
         pIterNext = RT_FROM_CPP_MEMBER(RTLISTOFF32_NEXT_PTR(&(pIterator)->Member), Type, Member) )


/**
 * Enumerate the list in reverse order (tail to head).
 *
 * @param   pList               List to enumerate.
 * @param   pIterator           The iterator variable name.
 * @param   Type                Structure the list node is a member of.
 * @param   Member              The list node member name.
 */
#define RTListOff32ForEachReverse(pList, pIterator, Type, Member) \
    for (pIterator = RTListOff32NodeGetPrev(pList, Type, Member); \
         !RTListOff32NodeIsDummy(pList, pIterator, Type, Member); \
         pIterator = RT_FROM_MEMBER(RTLISTOFF32_NEXT_PTR(&(pIterator)->Member), Type, Member) )
/** @copydoc RTListOff32ForEachReverse */
#define RTListOff32ForEachReverseCpp(pList, pIterator, Type, Member) \
    for (pIterator = RTListOff32NodeGetPrevCpp(pList, Type, Member); \
         !RTListOff32NodeIsDummyCpp(pList, pIterator, Type, Member); \
         pIterator = RT_FROM_CPP_MEMBER(RTLISTOFF32_PREV_PTR(&(pIterator)->Member), Type, Member) )


/**
 * Enumerate the list in reverse order (tail to head).
 *
 * @param   pList               List to enumerate.
 * @param   pIterator           The iterator variable name.
 * @param   pIterPrev           The name of the variable saving the pointer to
 *                              the previous element.
 * @param   Type                Structure the list node is a member of.
 * @param   Member              The list node member name.
 */
#define RTListOff32ForEachReverseSafe(pList, pIterator, pIterPrev, Type, Member) \
    for (pIterator = RTListOff32NodeGetPrev(pList, Type, Member), \
         pIterPrev = RT_FROM_MEMBER(RTLISTOFF32_NEXT_PTR(&(pIterator)->Member), Type, Member); \
         !RTListOff32NodeIsDummy(pList, pIterator, Type, Member); \
         pIterator = pIterPrev, \
         pIterPrev = RT_FROM_MEMBER(RTLISTOFF32_NEXT_PTR(&(pIterator)->Member), Type, Member) )
/** @copydoc RTListOff32ForEachReverseSafe */
#define RTListOff32ForEachReverseSafeCpp(pList, pIterator, pIterPrev, Type, Member) \
    for (pIterator = RTListOff32NodeGetPrevCpp(pList, Type, Member), \
         pIterPrev = RT_FROM_CPP_MEMBER(RTLISTOFF32_NEXT_PTR(&(pIterator)->Member), Type, Member); \
         !RTListOff32NodeIsDummyCpp(pList, pIterator, Type, Member); \
         pIterator = pIterPrev, \
         pIterPrev = RT_FROM_CPP_MEMBER(RTLISTOFF32_NEXT_PTR(&(pIterator)->Member), Type, Member) )


/**
 * Move the given list to a new list header.
 *
 * @param   pListDst            The new list.
 * @param   pListSrc            The list to move.
 */
DECLINLINE(void) RTListOff32Move(PRTLISTOFF32NODE pListDst, PRTLISTOFF32NODE pListSrc)
{
    if (!RTListOff32IsEmpty(pListSrc))
    {
        PRTLISTOFF32NODE pFirst = RTLISTOFF32_NEXT_PTR(pListSrc);
        PRTLISTOFF32NODE pLast  = RTLISTOFF32_PREV_PTR(pListSrc);

        RTLISTOFF32_SET_NEXT_PTR(pListDst, pFirst);
        RTLISTOFF32_SET_PREV_PTR(pListDst, pLast);

        /* Adjust the first and last element links */
        RTLISTOFF32_SET_NEXT_PTR(pLast, pListDst);
        RTLISTOFF32_SET_PREV_PTR(pFirst, pListDst);

        /* Finally remove the elements from the source list */
        RTListOff32Init(pListSrc);
    }
}

/**
 * List concatenation.
 *
 * @param   pListDst            The destination list.
 * @param   pListSrc            The source list to concatenate.
 */
DECLINLINE(void) RTListOff32Concatenate(PRTLISTOFF32ANCHOR pListDst, PRTLISTOFF32ANCHOR pListSrc)
{
    if (!RTListOff32IsEmpty(pListSrc))
    {
        PRTLISTOFF32NODE pFirstSrc = RTLISTOFF32_NEXT_PTR(pListSrc);
        PRTLISTOFF32NODE pLastSrc  = RTLISTOFF32_PREV_PTR(pListSrc);
        PRTLISTOFF32NODE pLastDst  = RTLISTOFF32_PREV_PTR(pListDst);

        RTLISTOFF32_SET_NEXT_PTR(pLastDst, pFirstSrc);
        RTLISTOFF32_SET_PREV_PTR(pFirstSrc, pLastDst);

        RTLISTOFF32_SET_NEXT_PTR(pLastSrc, pListDst);
        RTLISTOFF32_SET_PREV_PTR(pListDst, pLastSrc);

        /* Finally remove the elements from the source list */
        RTListOff32Init(pListSrc);
    }
}

/** @} */
RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_list_off32_h */

