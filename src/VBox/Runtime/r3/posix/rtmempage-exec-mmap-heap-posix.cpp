/* $Id: rtmempage-exec-mmap-heap-posix.cpp $ */
/** @file
 * IPRT - RTMemPage*, POSIX with heap.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "internal/iprt.h"
#include <iprt/mem.h>

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/avl.h>
#include <iprt/critsect.h>
#include <iprt/errcore.h>
#include <iprt/once.h>
#include <iprt/param.h>
#include <iprt/string.h>
#include "internal/mem.h"
#include "../alloc-ef.h"

#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
# define MAP_ANONYMOUS MAP_ANON
#endif


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** Threshold at which to we switch to simply calling mmap. */
#define RTMEMPAGEPOSIX_MMAP_THRESHOLD   _128K
/** The size of a heap block (power of two) - in bytes. */
#define RTMEMPAGEPOSIX_BLOCK_SIZE       _2M
AssertCompile(RTMEMPAGEPOSIX_BLOCK_SIZE == (RTMEMPAGEPOSIX_BLOCK_SIZE / PAGE_SIZE) * PAGE_SIZE);
/** The number of pages per heap block. */
#define RTMEMPAGEPOSIX_BLOCK_PAGE_COUNT (RTMEMPAGEPOSIX_BLOCK_SIZE / PAGE_SIZE)


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/** Pointer to a page heap block. */
typedef struct RTHEAPPAGEBLOCK *PRTHEAPPAGEBLOCK;

/**
 * A simple page heap.
 */
typedef struct RTHEAPPAGE
{
    /** Magic number (RTHEAPPAGE_MAGIC). */
    uint32_t            u32Magic;
    /** The number of pages in the heap (in BlockTree). */
    uint32_t            cHeapPages;
    /** The number of currently free pages. */
    uint32_t            cFreePages;
    /** Number of successful calls. */
    uint32_t            cAllocCalls;
    /** Number of successful free calls. */
    uint32_t            cFreeCalls;
    /** The free call number at which we last tried to minimize the heap. */
    uint32_t            uLastMinimizeCall;
    /** Tree of heap blocks. */
    AVLRPVTREE          BlockTree;
    /** Allocation hint no 1 (last freed). */
    PRTHEAPPAGEBLOCK    pHint1;
    /** Allocation hint no 2 (last alloc). */
    PRTHEAPPAGEBLOCK    pHint2;
    /** Critical section protecting the heap. */
    RTCRITSECT          CritSect;
    /** Set if the memory must allocated with execute access. */
    bool                fExec;
} RTHEAPPAGE;
#define RTHEAPPAGE_MAGIC     UINT32_C(0xfeedface)
/** Pointer to a page heap. */
typedef RTHEAPPAGE *PRTHEAPPAGE;


/**
 * Describes a page heap block.
 */
typedef struct RTHEAPPAGEBLOCK
{
    /** The AVL tree node core (void pointer range). */
    AVLRPVNODECORE      Core;
    /** Allocation bitmap.  Set bits marks allocated pages. */
    uint32_t            bmAlloc[RTMEMPAGEPOSIX_BLOCK_PAGE_COUNT / 32];
    /** Allocation boundrary bitmap.  Set bits marks the start of
     *  allocations. */
    uint32_t            bmFirst[RTMEMPAGEPOSIX_BLOCK_PAGE_COUNT / 32];
    /** The number of free pages. */
    uint32_t            cFreePages;
    /** Pointer back to the heap. */
    PRTHEAPPAGE         pHeap;
} RTHEAPPAGEBLOCK;


/**
 * Argument package for rtHeapPageAllocCallback.
 */
typedef struct RTHEAPPAGEALLOCARGS
{
    /** The number of pages to allocate. */
    size_t          cPages;
    /** Non-null on success.  */
    void           *pvAlloc;
    /** RTMEMPAGEALLOC_F_XXX. */
    uint32_t        fFlags;
} RTHEAPPAGEALLOCARGS;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Initialize once structure. */
static RTONCE       g_MemPagePosixInitOnce = RTONCE_INITIALIZER;
/** The page heap. */
static RTHEAPPAGE   g_MemPagePosixHeap;
/** The exec page heap. */
static RTHEAPPAGE   g_MemExecPosixHeap;


#ifdef RT_OS_OS2
/*
 * A quick mmap/munmap mockup for avoid duplicating lots of good code.
 */
# define INCL_BASE
# include <os2.h>
# undef  MAP_PRIVATE
# define MAP_PRIVATE    0
# undef  MAP_ANONYMOUS
# define MAP_ANONYMOUS  0
# undef  MAP_FAILED
# define MAP_FAILED  (void *)-1
# undef mmap
# define mmap   iprt_mmap
# undef munmap
# define munmap iprt_munmap

static void *mmap(void *pvWhere, size_t cb, int fProt, int fFlags, int fd, off_t off)
{
    NOREF(pvWhere); NOREF(fd); NOREF(off);
    void   *pv    = NULL;
    ULONG  fAlloc = OBJ_ANY | PAG_COMMIT;
    if (fProt & PROT_EXEC)
        fAlloc |= PAG_EXECUTE;
    if (fProt & PROT_READ)
        fAlloc |= PAG_READ;
    if (fProt & PROT_WRITE)
        fAlloc |= PAG_WRITE;
    APIRET rc = DosAllocMem(&pv, cb, fAlloc);
    if (rc == NO_ERROR)
        return pv;
    errno = ENOMEM;
    return MAP_FAILED;
}

static int munmap(void *pv, size_t cb)
{
    APIRET rc = DosFreeMem(pv);
    if (rc == NO_ERROR)
        return 0;
    errno = EINVAL;
    return -1;
}

#endif

/**
 * Initializes the heap.
 *
 * @returns IPRT status code.
 * @param   pHeap           The page heap to initialize.
 * @param   fExec           Whether the heap memory should be marked as
 *                          executable or not.
 */
int RTHeapPageInit(PRTHEAPPAGE pHeap, bool fExec)
{
    int rc = RTCritSectInitEx(&pHeap->CritSect,
                              RTCRITSECT_FLAGS_NO_LOCK_VAL | RTCRITSECT_FLAGS_NO_NESTING | RTCRITSECT_FLAGS_BOOTSTRAP_HACK,
                              NIL_RTLOCKVALCLASS, RTLOCKVAL_SUB_CLASS_NONE, NULL);
    if (RT_SUCCESS(rc))
    {
        pHeap->cHeapPages           = 0;
        pHeap->cFreePages           = 0;
        pHeap->cAllocCalls          = 0;
        pHeap->cFreeCalls           = 0;
        pHeap->uLastMinimizeCall    = 0;
        pHeap->BlockTree            = NULL;
        pHeap->fExec                = fExec;
        pHeap->u32Magic             = RTHEAPPAGE_MAGIC;
    }
    return rc;
}


/**
 * Deletes the heap and all the memory it tracks.
 *
 * @returns IPRT status code.
 * @param   pHeap           The page heap to delete.
 */
int RTHeapPageDelete(PRTHEAPPAGE pHeap)
{
    NOREF(pHeap);
    return VERR_NOT_IMPLEMENTED;
}


/**
 * Applies flags to an allocation.
 *
 * @param   pv              The allocation.
 * @param   cb              The size of the allocation (page aligned).
 * @param   fFlags          RTMEMPAGEALLOC_F_XXX.
 */
DECLINLINE(void) rtMemPagePosixApplyFlags(void *pv, size_t cb, uint32_t fFlags)
{
#ifndef RT_OS_OS2
    if (fFlags & RTMEMPAGEALLOC_F_ADVISE_LOCKED)
    {
        int rc = mlock(pv, cb);
# ifndef RT_OS_SOLARIS /* mlock(3C) on Solaris requires the priv_lock_memory privilege */
        AssertMsg(rc == 0, ("mlock %p LB %#zx -> %d errno=%d\n", pv, cb, rc, errno));
# endif
        NOREF(rc);
    }

# ifdef MADV_DONTDUMP
    if (fFlags & RTMEMPAGEALLOC_F_ADVISE_NO_DUMP)
    {
        int rc = madvise(pv, cb, MADV_DONTDUMP);
        AssertMsg(rc == 0, ("madvice %p LB %#zx MADV_DONTDUMP -> %d errno=%d\n", pv, cb, rc, errno));
        NOREF(rc);
    }
# endif
#endif

    if (fFlags & RTMEMPAGEALLOC_F_ZERO)
        RT_BZERO(pv, cb);
}


/**
 * Avoids some gotos in rtHeapPageAllocFromBlock.
 *
 * @returns VINF_SUCCESS.
 * @param   pBlock          The block.
 * @param   iPage           The page to start allocating at.
 * @param   cPages          The number of pages.
 * @param   fFlags          RTMEMPAGEALLOC_F_XXX.
 * @param   ppv             Where to return the allocation address.
 */
DECLINLINE(int) rtHeapPageAllocFromBlockSuccess(PRTHEAPPAGEBLOCK pBlock, uint32_t iPage, size_t cPages, uint32_t fFlags, void **ppv)
{
    PRTHEAPPAGE pHeap = pBlock->pHeap;

    ASMBitSet(&pBlock->bmFirst[0], iPage);
    pBlock->cFreePages -= cPages;
    pHeap->cFreePages  -= cPages;
    if (!pHeap->pHint2 || pHeap->pHint2->cFreePages < pBlock->cFreePages)
        pHeap->pHint2 = pBlock;
    pHeap->cAllocCalls++;

    void *pv = (uint8_t *)pBlock->Core.Key + (iPage << PAGE_SHIFT);
    *ppv = pv;

    if (fFlags)
        rtMemPagePosixApplyFlags(pv, cPages << PAGE_SHIFT, fFlags);

    return VINF_SUCCESS;
}


/**
 * Checks if a page range is free in the specified block.
 *
 * @returns @c true if the range is free, @c false if not.
 * @param   pBlock          The block.
 * @param   iFirst          The first page to check.
 * @param   cPages          The number of pages to check.
 */
DECLINLINE(bool) rtHeapPageIsPageRangeFree(PRTHEAPPAGEBLOCK pBlock, uint32_t iFirst, uint32_t cPages)
{
    uint32_t i = iFirst + cPages;
    while (i-- > iFirst)
    {
        if (ASMBitTest(&pBlock->bmAlloc[0], i))
            return false;
        Assert(!ASMBitTest(&pBlock->bmFirst[0], i));
    }
    return true;
}


/**
 * Tries to allocate a chunk of pages from a heap block.
 *
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_NO_MEMORY if the allocation failed.
 * @param   pBlock          The block to allocate from.
 * @param   cPages          The size of the allocation.
 * @param   fFlags          RTMEMPAGEALLOC_F_XXX.
 * @param   ppv             Where to return the allocation address on success.
 */
DECLINLINE(int) rtHeapPageAllocFromBlock(PRTHEAPPAGEBLOCK pBlock, size_t cPages, uint32_t fFlags, void **ppv)
{
    if (pBlock->cFreePages >= cPages)
    {
        int iPage = ASMBitFirstClear(&pBlock->bmAlloc[0], RTMEMPAGEPOSIX_BLOCK_PAGE_COUNT);
        Assert(iPage >= 0);

        /* special case: single page. */
        if (cPages == 1)
        {
            ASMBitSet(&pBlock->bmAlloc[0], iPage);
            return rtHeapPageAllocFromBlockSuccess(pBlock, iPage, cPages, fFlags, ppv);
        }

        while (   iPage >= 0
               && (unsigned)iPage <= RTMEMPAGEPOSIX_BLOCK_PAGE_COUNT - cPages)
        {
            if (rtHeapPageIsPageRangeFree(pBlock, iPage + 1, cPages - 1))
            {
                ASMBitSetRange(&pBlock->bmAlloc[0], iPage, iPage + cPages);
                return rtHeapPageAllocFromBlockSuccess(pBlock, iPage, cPages, fFlags, ppv);
            }

            /* next */
            iPage = ASMBitNextSet(&pBlock->bmAlloc[0], RTMEMPAGEPOSIX_BLOCK_PAGE_COUNT, iPage);
            if (iPage < 0 || iPage >= RTMEMPAGEPOSIX_BLOCK_PAGE_COUNT - 1)
                break;
            iPage = ASMBitNextClear(&pBlock->bmAlloc[0], RTMEMPAGEPOSIX_BLOCK_PAGE_COUNT, iPage);
        }
    }

    return VERR_NO_MEMORY;
}


/**
 * RTAvlrPVDoWithAll callback.
 *
 * @returns 0 to continue the enum, non-zero to quit it.
 * @param   pNode           The node.
 * @param   pvUser          The user argument.
 */
static DECLCALLBACK(int) rtHeapPageAllocCallback(PAVLRPVNODECORE pNode, void *pvUser)
{
    PRTHEAPPAGEBLOCK        pBlock = RT_FROM_MEMBER(pNode,  RTHEAPPAGEBLOCK, Core);
    RTHEAPPAGEALLOCARGS    *pArgs  = (RTHEAPPAGEALLOCARGS *)pvUser;
    int rc = rtHeapPageAllocFromBlock(pBlock, pArgs->cPages, pArgs->fFlags, &pArgs->pvAlloc);
    return RT_SUCCESS(rc) ? 1 : 0;
}


/**
 * Worker for RTHeapPageAlloc.
 *
 * @returns IPRT status code
 * @param   pHeap           The heap - locked.
 * @param   cPages          The page count.
 * @param   pszTag          The tag.
 * @param   fFlags          RTMEMPAGEALLOC_F_XXX.
 * @param   ppv             Where to return the address of the allocation
 *                          on success.
 */
static int rtHeapPageAllocLocked(PRTHEAPPAGE pHeap, size_t cPages, const char *pszTag, uint32_t fFlags, void **ppv)
{
    int rc;
    NOREF(pszTag);

    /*
     * Use the hints first.
     */
    if (pHeap->pHint1)
    {
        rc = rtHeapPageAllocFromBlock(pHeap->pHint1, cPages, fFlags, ppv);
        if (rc != VERR_NO_MEMORY)
            return rc;
    }
    if (pHeap->pHint2)
    {
        rc = rtHeapPageAllocFromBlock(pHeap->pHint2, cPages, fFlags, ppv);
        if (rc != VERR_NO_MEMORY)
            return rc;
    }

    /*
     * Search the heap for a block with enough free space.
     *
     * N.B. This search algorithm is not optimal at all. What (hopefully) saves
     *      it are the two hints above.
     */
    if (pHeap->cFreePages >= cPages)
    {
        RTHEAPPAGEALLOCARGS Args;
        Args.cPages  = cPages;
        Args.pvAlloc = NULL;
        Args.fFlags  = fFlags;
        RTAvlrPVDoWithAll(&pHeap->BlockTree, true /*fFromLeft*/, rtHeapPageAllocCallback, &Args);
        if (Args.pvAlloc)
        {
            *ppv = Args.pvAlloc;
            return VINF_SUCCESS;
        }
    }

    /*
     * Didn't find anytyhing, so expand the heap with a new block.
     */
    RTCritSectLeave(&pHeap->CritSect);
    void *pvPages;
    pvPages = mmap(NULL, RTMEMPAGEPOSIX_BLOCK_SIZE,
                   PROT_READ | PROT_WRITE | (pHeap->fExec ? PROT_EXEC : 0),
                   MAP_PRIVATE | MAP_ANONYMOUS,
                   -1, 0);
    if (pvPages == MAP_FAILED)
    {
        RTCritSectEnter(&pHeap->CritSect);
        return RTErrConvertFromErrno(errno);

    }
    /** @todo Eliminate this rtMemBaseAlloc dependency! */
    PRTHEAPPAGEBLOCK pBlock;
#ifdef RTALLOC_REPLACE_MALLOC
    if (g_pfnOrgMalloc)
        pBlock = (PRTHEAPPAGEBLOCK)g_pfnOrgMalloc(sizeof(*pBlock));
    else
#endif
        pBlock = (PRTHEAPPAGEBLOCK)rtMemBaseAlloc(sizeof(*pBlock));
    if (!pBlock)
    {
        munmap(pvPages, RTMEMPAGEPOSIX_BLOCK_SIZE);
        RTCritSectEnter(&pHeap->CritSect);
        return VERR_NO_MEMORY;
    }

    RT_ZERO(*pBlock);
    pBlock->Core.Key        = pvPages;
    pBlock->Core.KeyLast    = (uint8_t *)pvPages + RTMEMPAGEPOSIX_BLOCK_SIZE - 1;
    pBlock->cFreePages      = RTMEMPAGEPOSIX_BLOCK_PAGE_COUNT;
    pBlock->pHeap           = pHeap;

    RTCritSectEnter(&pHeap->CritSect);

    bool fRc = RTAvlrPVInsert(&pHeap->BlockTree, &pBlock->Core); Assert(fRc); NOREF(fRc);
    pHeap->cFreePages      +=  RTMEMPAGEPOSIX_BLOCK_PAGE_COUNT;
    pHeap->cHeapPages      +=  RTMEMPAGEPOSIX_BLOCK_PAGE_COUNT;

    /*
     * Grab memory from the new block (cannot fail).
     */
    rc = rtHeapPageAllocFromBlock(pBlock, cPages, fFlags, ppv);
    Assert(rc == VINF_SUCCESS);

    return rc;
}


/**
 * Allocates one or more pages off the heap.
 *
 * @returns IPRT status code.
 * @param   pHeap           The page heap.
 * @param   cPages          The number of pages to allocate.
 * @param   pszTag          The allocation tag.
 * @param   fFlags          RTMEMPAGEALLOC_F_XXX.
 * @param   ppv             Where to return the pointer to the pages.
 */
int RTHeapPageAlloc(PRTHEAPPAGE pHeap, size_t cPages, const char *pszTag, uint32_t fFlags, void **ppv)
{
    /*
     * Validate input.
     */
    AssertPtr(ppv);
    *ppv = NULL;
    AssertPtrReturn(pHeap, VERR_INVALID_HANDLE);
    AssertReturn(pHeap->u32Magic == RTHEAPPAGE_MAGIC, VERR_INVALID_HANDLE);
    AssertMsgReturn(cPages < RTMEMPAGEPOSIX_BLOCK_SIZE, ("%#zx\n", cPages), VERR_OUT_OF_RANGE);

    /*
     * Grab the lock and call a worker with many returns.
     */
    int rc = RTCritSectEnter(&pHeap->CritSect);
    if (RT_SUCCESS(rc))
    {
        rc = rtHeapPageAllocLocked(pHeap, cPages, pszTag, fFlags, ppv);
        RTCritSectLeave(&pHeap->CritSect);
    }

    return rc;
}


/**
 * RTAvlrPVDoWithAll callback.
 *
 * @returns 0 to continue the enum, non-zero to quit it.
 * @param   pNode           The node.
 * @param   pvUser          Pointer to a block pointer variable. For returning
 *                          the address of the block to be freed.
 */
static DECLCALLBACK(int) rtHeapPageFindUnusedBlockCallback(PAVLRPVNODECORE pNode, void *pvUser)
{
    PRTHEAPPAGEBLOCK pBlock = RT_FROM_MEMBER(pNode, RTHEAPPAGEBLOCK, Core);
    if (pBlock->cFreePages == RTMEMPAGEPOSIX_BLOCK_PAGE_COUNT)
    {
        *(PRTHEAPPAGEBLOCK *)pvUser = pBlock;
        return 1;
    }
    return 0;
}


/**
 * Allocates one or more pages off the heap.
 *
 * @returns IPRT status code.
 * @param   pHeap           The page heap.
 * @param   pv              Pointer to what RTHeapPageAlloc returned.
 * @param   cPages          The number of pages that was allocated.
 */
int RTHeapPageFree(PRTHEAPPAGE pHeap, void *pv, size_t cPages)
{
    /*
     * Validate input.
     */
    if (!pv)
        return VINF_SUCCESS;
    AssertPtrReturn(pHeap, VERR_INVALID_HANDLE);
    AssertReturn(pHeap->u32Magic == RTHEAPPAGE_MAGIC, VERR_INVALID_HANDLE);

    /*
     * Grab the lock and look up the page.
     */
    int rc = RTCritSectEnter(&pHeap->CritSect);
    if (RT_SUCCESS(rc))
    {
        PRTHEAPPAGEBLOCK pBlock = (PRTHEAPPAGEBLOCK)RTAvlrPVRangeGet(&pHeap->BlockTree, pv);
        if (pBlock)
        {
            /*
             * Validate the specified address range.
             */
            uint32_t const iPage = (uint32_t)(((uintptr_t)pv - (uintptr_t)pBlock->Core.Key) >> PAGE_SHIFT);
            /* Check the range is within the block. */
            bool fOk = iPage + cPages <= RTMEMPAGEPOSIX_BLOCK_PAGE_COUNT;
            /* Check that it's the start of an allocation. */
            fOk = fOk && ASMBitTest(&pBlock->bmFirst[0], iPage);
            /* Check that the range ends at an allocation boundrary. */
            fOk = fOk && (   iPage + cPages == RTMEMPAGEPOSIX_BLOCK_PAGE_COUNT
                          || ASMBitTest(&pBlock->bmFirst[0], iPage + cPages)
                          || !ASMBitTest(&pBlock->bmAlloc[0], iPage + cPages));
            /* Check the other pages. */
            uint32_t const iLastPage = iPage + cPages - 1;
            for (uint32_t i = iPage + 1; i < iLastPage && fOk; i++)
                fOk = ASMBitTest(&pBlock->bmAlloc[0], i)
                   && !ASMBitTest(&pBlock->bmFirst[0], i);
            if (fOk)
            {
                /*
                 * Free the memory.
                 */
                ASMBitClearRange(&pBlock->bmAlloc[0], iPage, iPage + cPages);
                ASMBitClear(&pBlock->bmFirst[0], iPage);
                pBlock->cFreePages += cPages;
                pHeap->cFreePages  += cPages;
                pHeap->cFreeCalls++;
                if (!pHeap->pHint1 || pHeap->pHint1->cFreePages < pBlock->cFreePages)
                    pHeap->pHint1 = pBlock;

                /** @todo Add bitmaps for tracking madvice and mlock so we can undo those. */

                /*
                 * Shrink the heap. Not very efficient because of the AVL tree.
                 */
                if (   pHeap->cFreePages >= RTMEMPAGEPOSIX_BLOCK_PAGE_COUNT * 3
                    && pHeap->cFreePages >= pHeap->cHeapPages / 2 /* 50% free */
                    && pHeap->cFreeCalls - pHeap->uLastMinimizeCall > RTMEMPAGEPOSIX_BLOCK_PAGE_COUNT
                   )
                {
                    uint32_t cFreePageTarget = pHeap->cHeapPages / 4; /* 25% free */
                    while (pHeap->cFreePages > cFreePageTarget)
                    {
                        pHeap->uLastMinimizeCall = pHeap->cFreeCalls;

                        pBlock = NULL;
                        RTAvlrPVDoWithAll(&pHeap->BlockTree, false /*fFromLeft*/,
                                          rtHeapPageFindUnusedBlockCallback, &pBlock);
                        if (!pBlock)
                            break;

                        void *pv2 = RTAvlrPVRemove(&pHeap->BlockTree, pBlock->Core.Key); Assert(pv2); NOREF(pv2);
                        pHeap->cHeapPages -= RTMEMPAGEPOSIX_BLOCK_PAGE_COUNT;
                        pHeap->cFreePages -= RTMEMPAGEPOSIX_BLOCK_PAGE_COUNT;
                        pHeap->pHint1      = NULL;
                        pHeap->pHint2      = NULL;
                        RTCritSectLeave(&pHeap->CritSect);

                        munmap(pBlock->Core.Key, RTMEMPAGEPOSIX_BLOCK_SIZE);
                        pBlock->Core.Key = pBlock->Core.KeyLast = NULL;
                        pBlock->cFreePages = 0;
#ifdef RTALLOC_REPLACE_MALLOC
                        if (g_pfnOrgFree)
                            g_pfnOrgFree(pBlock);
                        else
#endif
                            rtMemBaseFree(pBlock);

                        RTCritSectEnter(&pHeap->CritSect);
                    }
                }
            }
            else
                rc = VERR_INVALID_POINTER;
        }
        else
            rc = VERR_INVALID_POINTER;

        RTCritSectLeave(&pHeap->CritSect);
    }

    return rc;
}


/**
 * Initializes the heap.
 *
 * @returns IPRT status code
 * @param   pvUser              Unused.
 */
static DECLCALLBACK(int) rtMemPagePosixInitOnce(void *pvUser)
{
    NOREF(pvUser);
    int rc = RTHeapPageInit(&g_MemPagePosixHeap, false /*fExec*/);
    if (RT_SUCCESS(rc))
    {
        rc = RTHeapPageInit(&g_MemExecPosixHeap, true /*fExec*/);
        if (RT_SUCCESS(rc))
            return rc;
        RTHeapPageDelete(&g_MemPagePosixHeap);
    }
    return rc;
}


/**
 * Allocates memory from the specified heap.
 *
 * @returns Address of the allocated memory.
 * @param   cb                  The number of bytes to allocate.
 * @param   pszTag              The tag.
 * @param   fFlags              RTMEMPAGEALLOC_F_XXX.
 * @param   pHeap               The heap to use.
 */
static void *rtMemPagePosixAlloc(size_t cb, const char *pszTag, uint32_t fFlags, PRTHEAPPAGE pHeap)
{
    /*
     * Validate & adjust the input.
     */
    Assert(cb > 0);
    NOREF(pszTag);
    cb = RT_ALIGN_Z(cb, PAGE_SIZE);

    /*
     * If the allocation is relatively large, we use mmap/munmap directly.
     */
    void *pv;
    if (cb >= RTMEMPAGEPOSIX_MMAP_THRESHOLD)
    {

        pv = mmap(NULL, cb,
                  PROT_READ | PROT_WRITE | (pHeap == &g_MemExecPosixHeap ? PROT_EXEC : 0),
                  MAP_PRIVATE | MAP_ANONYMOUS,
                  -1, 0);
        if (pv != MAP_FAILED)
        {
            AssertPtr(pv);

            if (fFlags)
                rtMemPagePosixApplyFlags(pv, cb, fFlags);
        }
        else
            pv = NULL;
    }
    else
    {
        int rc = RTOnce(&g_MemPagePosixInitOnce, rtMemPagePosixInitOnce, NULL);
        if (RT_SUCCESS(rc))
            rc = RTHeapPageAlloc(pHeap, cb >> PAGE_SHIFT, pszTag, fFlags, &pv);
        if (RT_FAILURE(rc))
            pv = NULL;
    }

    return pv;
}


/**
 * Free memory allocated by rtMemPagePosixAlloc.
 *
 * @param   pv                  The address of the memory to free.
 * @param   cb                  The size.
 * @param   pHeap               The heap.
 */
static void rtMemPagePosixFree(void *pv, size_t cb, PRTHEAPPAGE pHeap)
{
    /*
     * Validate & adjust the input.
     */
    if (!pv)
        return;
    AssertPtr(pv);
    Assert(cb > 0);
    Assert(!((uintptr_t)pv & PAGE_OFFSET_MASK));
    cb = RT_ALIGN_Z(cb, PAGE_SIZE);

    /*
     * If the allocation is relatively large, we use mmap/munmap directly.
     */
    if (cb >= RTMEMPAGEPOSIX_MMAP_THRESHOLD)
    {
        int rc = munmap(pv, cb);
        AssertMsg(rc == 0, ("rc=%d pv=%p cb=%#zx\n", rc, pv, cb)); NOREF(rc);
    }
    else
    {
        int rc = RTHeapPageFree(pHeap, pv, cb >> PAGE_SHIFT);
        AssertRC(rc);
    }
}





RTDECL(void *) RTMemPageAllocTag(size_t cb, const char *pszTag) RT_NO_THROW_DEF
{
    return rtMemPagePosixAlloc(cb, pszTag, 0, &g_MemPagePosixHeap);
}


RTDECL(void *) RTMemPageAllocZTag(size_t cb, const char *pszTag) RT_NO_THROW_DEF
{
    return rtMemPagePosixAlloc(cb, pszTag, RTMEMPAGEALLOC_F_ZERO, &g_MemPagePosixHeap);
}


RTDECL(void *) RTMemPageAllocExTag(size_t cb, uint32_t fFlags, const char *pszTag) RT_NO_THROW_DEF
{
    AssertReturn(!(fFlags & ~RTMEMPAGEALLOC_F_VALID_MASK), NULL);
    return rtMemPagePosixAlloc(cb, pszTag, fFlags, &g_MemPagePosixHeap);
}


RTDECL(void) RTMemPageFree(void *pv, size_t cb) RT_NO_THROW_DEF
{
    return rtMemPagePosixFree(pv, cb, &g_MemPagePosixHeap);
}

