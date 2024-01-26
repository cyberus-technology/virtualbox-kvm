/** @file
 * IPRT - Hardened AVL tree slab allocator.
 */

/*
 * Copyright (C) 2022-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_cpp_hardavlslaballocator_h
#define IPRT_INCLUDED_cpp_hardavlslaballocator_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/string.h>

/** @addtogroup grp_rt_cpp_hardavl
 * @{
 */


/**
 * Slab allocator for the hardened AVL tree.
 */
template<typename NodeType>
struct RTCHardAvlTreeSlabAllocator
{
    /** Pointer to an array of nodes. */
    NodeType   *m_paNodes;
    /** Node allocation bitmap: 1 = free, 0 = allocated. */
    uint64_t   *m_pbmAlloc;
    /** Max number of nodes in m_paNodes and valid bits in m_pbmAlloc. */
    uint32_t    m_cNodes;
    /** Pointer error counter. */
    uint32_t    m_cErrors;
    /** Allocation hint. */
    uint32_t    m_idxAllocHint;
    uint32_t    m_uPadding;

    enum
    {
        kNilIndex              = 0,
        kErr_IndexOutOfBound   = -1,
        kErr_PointerOutOfBound = -2,
        kErr_MisalignedPointer = -3,
        kErr_NodeIsFree        = -4,
        kErr_Last              = kErr_NodeIsFree
    };

    RTCHardAvlTreeSlabAllocator() RT_NOEXCEPT
        : m_paNodes(NULL)
        , m_pbmAlloc(NULL)
        , m_cNodes(0)
        , m_cErrors(0)
        , m_idxAllocHint(0)
        , m_uPadding(0)
    {}

    inline void initSlabAllocator(uint32_t a_cNodes, NodeType *a_paNodes, uint64_t *a_pbmAlloc) RT_NOEXCEPT
    {
        m_cNodes   = a_cNodes;
        m_paNodes  = a_paNodes;
        m_pbmAlloc = a_pbmAlloc;

        /* Initialize the allocation bit. */
        RT_BZERO(a_pbmAlloc, (a_cNodes + 63) / 64 * 8);
        ASMBitSetRange(a_pbmAlloc, 0, a_cNodes);
    }

    inline NodeType *ptrFromInt(uint32_t a_idxNode1) RT_NOEXCEPT
    {
        if (a_idxNode1 == (uint32_t)kNilIndex)
            return NULL;
        AssertMsgReturnStmt(a_idxNode1 <= m_cNodes, ("a_idxNode1=%#x m_cNodes=%#x\n", a_idxNode1, m_cNodes),
                            m_cErrors++, (NodeType *)(intptr_t)kErr_IndexOutOfBound);
        AssertMsgReturnStmt(ASMBitTest(m_pbmAlloc, a_idxNode1 - 1) == false, ("a_idxNode1=%#x\n", a_idxNode1),
                            m_cErrors++, (NodeType *)(intptr_t)kErr_NodeIsFree);
        return &m_paNodes[a_idxNode1 - 1];
    }

    static inline bool isPtrRetOkay(NodeType *a_pNode) RT_NOEXCEPT
    {
        return (uintptr_t)a_pNode < (uintptr_t)kErr_Last;
    }

    static inline int ptrErrToStatus(NodeType *a_pNode) RT_NOEXCEPT
    {
        return (int)(intptr_t)a_pNode - (VERR_HARDAVL_INDEX_OUT_OF_BOUNDS - kErr_IndexOutOfBound);
    }

    inline uint32_t ptrToInt(NodeType *a_pNode) RT_NOEXCEPT
    {
        if (a_pNode == NULL)
            return 0;
        uintptr_t const offNode  = (uintptr_t)a_pNode - (uintptr_t)m_paNodes;
        uintptr_t const idxNode0 = offNode / sizeof(m_paNodes[0]);
        AssertMsgReturnStmt((offNode % sizeof(m_paNodes[0])) == 0,
                            ("pNode=%p / offNode=%#zx vs m_paNodes=%p L %#x, each %#x bytes\n",
                             a_pNode, offNode, m_paNodes, m_cNodes, sizeof(m_paNodes[0])),
                            m_cErrors++, (uint32_t)kErr_MisalignedPointer);
        AssertMsgReturnStmt(idxNode0 < m_cNodes,
                            ("pNode=%p vs m_paNodes=%p L %#x\n", a_pNode, m_paNodes, m_cNodes),
                            m_cErrors++, (uint32_t)kErr_PointerOutOfBound);
        AssertMsgReturnStmt(ASMBitTest(m_pbmAlloc, idxNode0) == false, ("a_pNode=%p idxNode0=%#x\n", a_pNode, idxNode0),
                            m_cErrors++, (uint32_t)kErr_NodeIsFree);
        return idxNode0 + 1;
    }

    static inline bool isIdxRetOkay(uint32_t a_idxNode) RT_NOEXCEPT
    {
        return a_idxNode < (uint32_t)kErr_Last;
    }

    static inline int idxErrToStatus(uint32_t a_idxNode) RT_NOEXCEPT
    {
        return (int)a_idxNode - (VERR_HARDAVL_INDEX_OUT_OF_BOUNDS - kErr_IndexOutOfBound);
    }

    inline bool isIntValid(uint32_t a_idxNode1) RT_NOEXCEPT
    {
        return a_idxNode1 <= m_cNodes;
    }

    inline int freeNode(NodeType *a_pNode) RT_NOEXCEPT
    {
        uint32_t idxNode1 = ptrToInt(a_pNode);
        if (idxNode1 == (uint32_t)kNilIndex)
            return 0;
        if (idxNode1 < (uint32_t)kErr_Last)
        {
            AssertMsgReturnStmt(ASMAtomicBitTestAndSet(m_pbmAlloc, idxNode1 - 1) == false,
                                ("a_pNode=%p idxNode1=%#x\n", a_pNode, idxNode1),
                                m_cErrors++, kErr_NodeIsFree);
            return 0;
        }
        return (int)idxNode1;
    }

    inline NodeType *allocateNode(void) RT_NOEXCEPT
    {
        /*
         * Use the hint first, then scan the whole bitmap.
         * Note! We don't expect concurrent allocation calls, so no need to repeat.
         */
        uint32_t const idxHint = m_idxAllocHint;
        uint32_t       idxNode0;
        if (   idxHint >= m_cNodes
            || (int32_t)(idxNode0 = (uint32_t)ASMBitNextSet(m_pbmAlloc, m_cNodes, idxHint)) < 0)
            idxNode0 = (uint32_t)ASMBitFirstSet(m_pbmAlloc, m_cNodes);
        if ((int32_t)idxNode0 >= 0)
        {
            if (ASMAtomicBitTestAndClear(m_pbmAlloc, idxNode0) == true)
            {
                m_idxAllocHint = idxNode0;
                return &m_paNodes[idxNode0];
            }
            AssertMsgFailed(("idxNode0=%#x\n", idxNode0));
            m_cErrors++;
        }
        return NULL;
    }
};


/**
 * Placeholder structure for ring-3 slab allocator.
 */
typedef struct RTCHardAvlTreeSlabAllocatorR3_T
{
    /** Pointer to an array of nodes. */
    RTR3PTR     m_paNodes;
    /** Node allocation bitmap: 1 = free, 0 = allocated. */
    RTR3PTR     m_pbmAlloc;
    /** Max number of nodes in m_paNodes and valid bits in m_pbmAlloc. */
    uint32_t    m_cNodes;
    /** Pointer error counter. */
    uint32_t    m_cErrors;
    /** Allocation hint. */
    uint32_t    m_idxAllocHint;
    uint32_t    m_uPadding;
} RTCHardAvlTreeSlabAllocatorR3_T;
AssertCompileSize(RTCHardAvlTreeSlabAllocatorR3_T,
                  sizeof(RTCHardAvlTreeSlabAllocator<RTUINT128U>) - (sizeof(void *) - sizeof(RTR3PTR)) * 2);

/** @} */

#endif /* !IPRT_INCLUDED_cpp_hardavlslaballocator_h */

