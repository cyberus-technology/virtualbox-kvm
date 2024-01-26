/* $Id: PGMPhys.cpp $ */
/** @file
 * PGM - Page Manager and Monitor, Physical Memory Addressing.
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
 * SPDX-License-Identifier: GPL-3.0-only
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_PGM_PHYS
#define VBOX_WITHOUT_PAGING_BIT_FIELDS /* 64-bit bitfields are just asking for trouble. See @bugref{9841} and others. */
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/iem.h>
#include <VBox/vmm/iom.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/nem.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/pdmdev.h>
#include "PGMInternal.h"
#include <VBox/vmm/vmcc.h>

#include "PGMInline.h"

#include <VBox/sup.h>
#include <VBox/param.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/alloc.h>
#include <iprt/asm.h>
#ifdef VBOX_STRICT
# include <iprt/crc.h>
#endif
#include <iprt/thread.h>
#include <iprt/string.h>
#include <iprt/system.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** The number of pages to free in one batch. */
#define PGMPHYS_FREE_PAGE_BATCH_SIZE    128



/*********************************************************************************************************************************
*   Reading and Writing Guest Pysical Memory                                                                                     *
*********************************************************************************************************************************/

/*
 * PGMR3PhysReadU8-64
 * PGMR3PhysWriteU8-64
 */
#define PGMPHYSFN_READNAME  PGMR3PhysReadU8
#define PGMPHYSFN_WRITENAME PGMR3PhysWriteU8
#define PGMPHYS_DATASIZE    1
#define PGMPHYS_DATATYPE    uint8_t
#include "PGMPhysRWTmpl.h"

#define PGMPHYSFN_READNAME  PGMR3PhysReadU16
#define PGMPHYSFN_WRITENAME PGMR3PhysWriteU16
#define PGMPHYS_DATASIZE    2
#define PGMPHYS_DATATYPE    uint16_t
#include "PGMPhysRWTmpl.h"

#define PGMPHYSFN_READNAME  PGMR3PhysReadU32
#define PGMPHYSFN_WRITENAME PGMR3PhysWriteU32
#define PGMPHYS_DATASIZE    4
#define PGMPHYS_DATATYPE    uint32_t
#include "PGMPhysRWTmpl.h"

#define PGMPHYSFN_READNAME  PGMR3PhysReadU64
#define PGMPHYSFN_WRITENAME PGMR3PhysWriteU64
#define PGMPHYS_DATASIZE    8
#define PGMPHYS_DATATYPE    uint64_t
#include "PGMPhysRWTmpl.h"


/**
 * EMT worker for PGMR3PhysReadExternal.
 */
static DECLCALLBACK(int) pgmR3PhysReadExternalEMT(PVM pVM, PRTGCPHYS pGCPhys, void *pvBuf, size_t cbRead,
                                                  PGMACCESSORIGIN enmOrigin)
{
    VBOXSTRICTRC rcStrict = PGMPhysRead(pVM, *pGCPhys, pvBuf, cbRead, enmOrigin);
    AssertMsg(rcStrict == VINF_SUCCESS, ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict))); NOREF(rcStrict);
    return VINF_SUCCESS;
}


/**
 * Read from physical memory, external users.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS.
 *
 * @param   pVM             The cross context VM structure.
 * @param   GCPhys          Physical address to read from.
 * @param   pvBuf           Where to read into.
 * @param   cbRead          How many bytes to read.
 * @param   enmOrigin       Who is calling.
 *
 * @thread  Any but EMTs.
 */
VMMR3DECL(int) PGMR3PhysReadExternal(PVM pVM, RTGCPHYS GCPhys, void *pvBuf, size_t cbRead, PGMACCESSORIGIN enmOrigin)
{
    VM_ASSERT_OTHER_THREAD(pVM);

    AssertMsgReturn(cbRead > 0, ("don't even think about reading zero bytes!\n"), VINF_SUCCESS);
    LogFlow(("PGMR3PhysReadExternal: %RGp %d\n", GCPhys, cbRead));

    PGM_LOCK_VOID(pVM);

    /*
     * Copy loop on ram ranges.
     */
    PPGMRAMRANGE pRam = pgmPhysGetRangeAtOrAbove(pVM, GCPhys);
    for (;;)
    {
        /* Inside range or not? */
        if (pRam && GCPhys >= pRam->GCPhys)
        {
            /*
             * Must work our way thru this page by page.
             */
            RTGCPHYS off = GCPhys - pRam->GCPhys;
            while (off < pRam->cb)
            {
                unsigned iPage = off >> GUEST_PAGE_SHIFT;
                PPGMPAGE pPage = &pRam->aPages[iPage];

                /*
                 * If the page has an ALL access handler, we'll have to
                 * delegate the job to EMT.
                 */
                if (   PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(pPage)
                    || PGM_PAGE_IS_SPECIAL_ALIAS_MMIO(pPage))
                {
                    PGM_UNLOCK(pVM);

                    return VMR3ReqPriorityCallWait(pVM, VMCPUID_ANY, (PFNRT)pgmR3PhysReadExternalEMT, 5,
                                                   pVM, &GCPhys, pvBuf, cbRead, enmOrigin);
                }
                Assert(!PGM_PAGE_IS_MMIO_OR_SPECIAL_ALIAS(pPage));

                /*
                 * Simple stuff, go ahead.
                 */
                size_t cb = GUEST_PAGE_SIZE - (off & GUEST_PAGE_OFFSET_MASK);
                if (cb > cbRead)
                    cb = cbRead;
                PGMPAGEMAPLOCK PgMpLck;
                const void    *pvSrc;
                int rc = pgmPhysGCPhys2CCPtrInternalReadOnly(pVM, pPage, pRam->GCPhys + off, &pvSrc, &PgMpLck);
                if (RT_SUCCESS(rc))
                {
                    memcpy(pvBuf, pvSrc, cb);
                    pgmPhysReleaseInternalPageMappingLock(pVM, &PgMpLck);
                }
                else
                {
                    AssertLogRelMsgFailed(("pgmPhysGCPhys2CCPtrInternalReadOnly failed on %RGp / %R[pgmpage] -> %Rrc\n",
                                           pRam->GCPhys + off, pPage, rc));
                    memset(pvBuf, 0xff, cb);
                }

                /* next page */
                if (cb >= cbRead)
                {
                    PGM_UNLOCK(pVM);
                    return VINF_SUCCESS;
                }
                cbRead -= cb;
                off    += cb;
                GCPhys += cb;
                pvBuf   = (char *)pvBuf + cb;
            } /* walk pages in ram range. */
        }
        else
        {
            LogFlow(("PGMPhysRead: Unassigned %RGp size=%u\n", GCPhys, cbRead));

            /*
             * Unassigned address space.
             */
            size_t cb = pRam ? pRam->GCPhys - GCPhys : ~(size_t)0;
            if (cb >= cbRead)
            {
                memset(pvBuf, 0xff, cbRead);
                break;
            }
            memset(pvBuf, 0xff, cb);

            cbRead -= cb;
            pvBuf   = (char *)pvBuf + cb;
            GCPhys += cb;
        }

        /* Advance range if necessary. */
        while (pRam && GCPhys > pRam->GCPhysLast)
            pRam = pRam->CTX_SUFF(pNext);
    } /* Ram range walk */

    PGM_UNLOCK(pVM);

    return VINF_SUCCESS;
}


/**
 * EMT worker for PGMR3PhysWriteExternal.
 */
static DECLCALLBACK(int) pgmR3PhysWriteExternalEMT(PVM pVM, PRTGCPHYS pGCPhys, const void *pvBuf, size_t cbWrite,
                                                   PGMACCESSORIGIN enmOrigin)
{
    /** @todo VERR_EM_NO_MEMORY */
    VBOXSTRICTRC rcStrict = PGMPhysWrite(pVM, *pGCPhys, pvBuf, cbWrite, enmOrigin);
    AssertMsg(rcStrict == VINF_SUCCESS, ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict))); NOREF(rcStrict);
    return VINF_SUCCESS;
}


/**
 * Write to physical memory, external users.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS.
 * @retval  VERR_EM_NO_MEMORY.
 *
 * @param   pVM             The cross context VM structure.
 * @param   GCPhys          Physical address to write to.
 * @param   pvBuf           What to write.
 * @param   cbWrite         How many bytes to write.
 * @param   enmOrigin       Who is calling.
 *
 * @thread  Any but EMTs.
 */
VMMDECL(int) PGMR3PhysWriteExternal(PVM pVM, RTGCPHYS GCPhys, const void *pvBuf, size_t cbWrite, PGMACCESSORIGIN enmOrigin)
{
    VM_ASSERT_OTHER_THREAD(pVM);

    AssertMsg(!pVM->pgm.s.fNoMorePhysWrites,
              ("Calling PGMR3PhysWriteExternal after pgmR3Save()! GCPhys=%RGp cbWrite=%#x enmOrigin=%d\n",
               GCPhys, cbWrite, enmOrigin));
    AssertMsgReturn(cbWrite > 0, ("don't even think about writing zero bytes!\n"), VINF_SUCCESS);
    LogFlow(("PGMR3PhysWriteExternal: %RGp %d\n", GCPhys, cbWrite));

    PGM_LOCK_VOID(pVM);

    /*
     * Copy loop on ram ranges, stop when we hit something difficult.
     */
    PPGMRAMRANGE pRam = pgmPhysGetRangeAtOrAbove(pVM, GCPhys);
    for (;;)
    {
        /* Inside range or not? */
        if (pRam && GCPhys >= pRam->GCPhys)
        {
            /*
             * Must work our way thru this page by page.
             */
            RTGCPTR off = GCPhys - pRam->GCPhys;
            while (off < pRam->cb)
            {
                RTGCPTR     iPage = off >> GUEST_PAGE_SHIFT;
                PPGMPAGE    pPage = &pRam->aPages[iPage];

                /*
                 * Is the page problematic, we have to do the work on the EMT.
                 *
                 * Allocating writable pages and access handlers are
                 * problematic, write monitored pages are simple and can be
                 * dealt with here.
                 */
                if (    PGM_PAGE_HAS_ACTIVE_HANDLERS(pPage)
                    ||  PGM_PAGE_GET_STATE(pPage) != PGM_PAGE_STATE_ALLOCATED
                    ||  PGM_PAGE_IS_SPECIAL_ALIAS_MMIO(pPage))
                {
                    if (    PGM_PAGE_GET_STATE(pPage) == PGM_PAGE_STATE_WRITE_MONITORED
                        && !PGM_PAGE_HAS_ACTIVE_HANDLERS(pPage))
                        pgmPhysPageMakeWriteMonitoredWritable(pVM, pPage, GCPhys);
                    else
                    {
                        PGM_UNLOCK(pVM);

                        return VMR3ReqPriorityCallWait(pVM, VMCPUID_ANY, (PFNRT)pgmR3PhysWriteExternalEMT, 5,
                                                       pVM, &GCPhys, pvBuf, cbWrite, enmOrigin);
                    }
                }
                Assert(!PGM_PAGE_IS_MMIO_OR_SPECIAL_ALIAS(pPage));

                /*
                 * Simple stuff, go ahead.
                 */
                size_t cb = GUEST_PAGE_SIZE - (off & GUEST_PAGE_OFFSET_MASK);
                if (cb > cbWrite)
                    cb = cbWrite;
                PGMPAGEMAPLOCK PgMpLck;
                void          *pvDst;
                int rc = pgmPhysGCPhys2CCPtrInternal(pVM, pPage, pRam->GCPhys + off, &pvDst, &PgMpLck);
                if (RT_SUCCESS(rc))
                {
                    memcpy(pvDst, pvBuf, cb);
                    pgmPhysReleaseInternalPageMappingLock(pVM, &PgMpLck);
                }
                else
                    AssertLogRelMsgFailed(("pgmPhysGCPhys2CCPtrInternal failed on %RGp / %R[pgmpage] -> %Rrc\n",
                                           pRam->GCPhys + off, pPage, rc));

                /* next page */
                if (cb >= cbWrite)
                {
                    PGM_UNLOCK(pVM);
                    return VINF_SUCCESS;
                }

                cbWrite -= cb;
                off     += cb;
                GCPhys  += cb;
                pvBuf    = (const char *)pvBuf + cb;
            } /* walk pages in ram range */
        }
        else
        {
            /*
             * Unassigned address space, skip it.
             */
            if (!pRam)
                break;
            size_t cb = pRam->GCPhys - GCPhys;
            if (cb >= cbWrite)
                break;
            cbWrite -= cb;
            pvBuf   = (const char *)pvBuf + cb;
            GCPhys += cb;
        }

        /* Advance range if necessary. */
        while (pRam && GCPhys > pRam->GCPhysLast)
            pRam = pRam->CTX_SUFF(pNext);
    } /* Ram range walk */

    PGM_UNLOCK(pVM);
    return VINF_SUCCESS;
}


/*********************************************************************************************************************************
*   Mapping Guest Physical Memory                                                                                                *
*********************************************************************************************************************************/

/**
 * VMR3ReqCall worker for PGMR3PhysGCPhys2CCPtrExternal to make pages writable.
 *
 * @returns see PGMR3PhysGCPhys2CCPtrExternal
 * @param   pVM         The cross context VM structure.
 * @param   pGCPhys     Pointer to the guest physical address.
 * @param   ppv         Where to store the mapping address.
 * @param   pLock       Where to store the lock.
 */
static DECLCALLBACK(int) pgmR3PhysGCPhys2CCPtrDelegated(PVM pVM, PRTGCPHYS pGCPhys, void **ppv, PPGMPAGEMAPLOCK pLock)
{
    /*
     * Just hand it to PGMPhysGCPhys2CCPtr and check that it's not a page with
     * an access handler after it succeeds.
     */
    int rc = PGM_LOCK(pVM);
    AssertRCReturn(rc, rc);

    rc = PGMPhysGCPhys2CCPtr(pVM, *pGCPhys, ppv, pLock);
    if (RT_SUCCESS(rc))
    {
        PPGMPAGEMAPTLBE pTlbe;
        int rc2 = pgmPhysPageQueryTlbe(pVM, *pGCPhys, &pTlbe);
        AssertFatalRC(rc2);
        PPGMPAGE pPage = pTlbe->pPage;
        if (PGM_PAGE_IS_MMIO_OR_SPECIAL_ALIAS(pPage))
        {
            PGMPhysReleasePageMappingLock(pVM, pLock);
            rc = VERR_PGM_PHYS_PAGE_RESERVED;
        }
        else if (    PGM_PAGE_HAS_ACTIVE_HANDLERS(pPage)
#ifdef PGMPOOL_WITH_OPTIMIZED_DIRTY_PT
                 ||  pgmPoolIsDirtyPage(pVM, *pGCPhys)
#endif
                )
        {
            /* We *must* flush any corresponding pgm pool page here, otherwise we'll
             * not be informed about writes and keep bogus gst->shw mappings around.
             */
            pgmPoolFlushPageByGCPhys(pVM, *pGCPhys);
            Assert(!PGM_PAGE_HAS_ACTIVE_HANDLERS(pPage));
            /** @todo r=bird: return VERR_PGM_PHYS_PAGE_RESERVED here if it still has
             *        active handlers, see the PGMR3PhysGCPhys2CCPtrExternal docs. */
        }
    }

    PGM_UNLOCK(pVM);
    return rc;
}


/**
 * Requests the mapping of a guest page into ring-3, external threads.
 *
 * When you're done with the page, call PGMPhysReleasePageMappingLock() ASAP to
 * release it.
 *
 * This API will assume your intention is to write to the page, and will
 * therefore replace shared and zero pages. If you do not intend to modify the
 * page, use the PGMR3PhysGCPhys2CCPtrReadOnlyExternal() API.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_PGM_PHYS_PAGE_RESERVED it it's a valid page but has no physical
 *          backing or if the page has any active access handlers. The caller
 *          must fall back on using PGMR3PhysWriteExternal.
 * @retval  VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS if it's not a valid physical address.
 *
 * @param   pVM         The cross context VM structure.
 * @param   GCPhys      The guest physical address of the page that should be mapped.
 * @param   ppv         Where to store the address corresponding to GCPhys.
 * @param   pLock       Where to store the lock information that PGMPhysReleasePageMappingLock needs.
 *
 * @remark  Avoid calling this API from within critical sections (other than the
 *          PGM one) because of the deadlock risk when we have to delegating the
 *          task to an EMT.
 * @thread  Any.
 */
VMMR3DECL(int) PGMR3PhysGCPhys2CCPtrExternal(PVM pVM, RTGCPHYS GCPhys, void **ppv, PPGMPAGEMAPLOCK pLock)
{
    AssertPtr(ppv);
    AssertPtr(pLock);

    Assert(VM_IS_EMT(pVM) || !PGMIsLockOwner(pVM));

    int rc = PGM_LOCK(pVM);
    AssertRCReturn(rc, rc);

    /*
     * Query the Physical TLB entry for the page (may fail).
     */
    PPGMPAGEMAPTLBE pTlbe;
    rc = pgmPhysPageQueryTlbe(pVM, GCPhys, &pTlbe);
    if (RT_SUCCESS(rc))
    {
        PPGMPAGE pPage = pTlbe->pPage;
        if (PGM_PAGE_IS_MMIO_OR_SPECIAL_ALIAS(pPage))
            rc = VERR_PGM_PHYS_PAGE_RESERVED;
        else
        {
            /*
             * If the page is shared, the zero page, or being write monitored
             * it must be converted to an page that's writable if possible.
             * We can only deal with write monitored pages here, the rest have
             * to be on an EMT.
             */
            if (    PGM_PAGE_HAS_ACTIVE_HANDLERS(pPage)
                ||  PGM_PAGE_GET_STATE(pPage) != PGM_PAGE_STATE_ALLOCATED
#ifdef PGMPOOL_WITH_OPTIMIZED_DIRTY_PT
                ||  pgmPoolIsDirtyPage(pVM, GCPhys)
#endif
               )
            {
                if (    PGM_PAGE_GET_STATE(pPage) == PGM_PAGE_STATE_WRITE_MONITORED
                    &&  !PGM_PAGE_HAS_ACTIVE_HANDLERS(pPage)
#ifdef PGMPOOL_WITH_OPTIMIZED_DIRTY_PT
                    &&  !pgmPoolIsDirtyPage(pVM, GCPhys) /** @todo we're very likely doing this twice. */
#endif
                   )
                    pgmPhysPageMakeWriteMonitoredWritable(pVM, pPage, GCPhys);
                else
                {
                    PGM_UNLOCK(pVM);

                    return VMR3ReqPriorityCallWait(pVM, VMCPUID_ANY, (PFNRT)pgmR3PhysGCPhys2CCPtrDelegated, 4,
                                                   pVM, &GCPhys, ppv, pLock);
                }
            }

            /*
             * Now, just perform the locking and calculate the return address.
             */
            PPGMPAGEMAP pMap = pTlbe->pMap;
            if (pMap)
                pMap->cRefs++;

            unsigned cLocks = PGM_PAGE_GET_WRITE_LOCKS(pPage);
            if (RT_LIKELY(cLocks < PGM_PAGE_MAX_LOCKS - 1))
            {
                if (cLocks == 0)
                    pVM->pgm.s.cWriteLockedPages++;
                PGM_PAGE_INC_WRITE_LOCKS(pPage);
            }
            else if (cLocks != PGM_PAGE_GET_WRITE_LOCKS(pPage))
            {
                PGM_PAGE_INC_WRITE_LOCKS(pPage);
                AssertMsgFailed(("%RGp / %R[pgmpage] is entering permanent write locked state!\n", GCPhys, pPage));
                if (pMap)
                    pMap->cRefs++; /* Extra ref to prevent it from going away. */
            }

            *ppv = (void *)((uintptr_t)pTlbe->pv | (uintptr_t)(GCPhys & GUEST_PAGE_OFFSET_MASK));
            pLock->uPageAndType = (uintptr_t)pPage | PGMPAGEMAPLOCK_TYPE_WRITE;
            pLock->pvMap = pMap;
        }
    }

    PGM_UNLOCK(pVM);
    return rc;
}


/**
 * Requests the mapping of a guest page into ring-3, external threads.
 *
 * When you're done with the page, call PGMPhysReleasePageMappingLock() ASAP to
 * release it.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_PGM_PHYS_PAGE_RESERVED it it's a valid page but has no physical
 *          backing or if the page as an active ALL access handler. The caller
 *          must fall back on using PGMPhysRead.
 * @retval  VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS if it's not a valid physical address.
 *
 * @param   pVM         The cross context VM structure.
 * @param   GCPhys      The guest physical address of the page that should be mapped.
 * @param   ppv         Where to store the address corresponding to GCPhys.
 * @param   pLock       Where to store the lock information that PGMPhysReleasePageMappingLock needs.
 *
 * @remark  Avoid calling this API from within critical sections (other than
 *          the PGM one) because of the deadlock risk.
 * @thread  Any.
 */
VMMR3DECL(int) PGMR3PhysGCPhys2CCPtrReadOnlyExternal(PVM pVM, RTGCPHYS GCPhys, void const **ppv, PPGMPAGEMAPLOCK pLock)
{
    int rc = PGM_LOCK(pVM);
    AssertRCReturn(rc, rc);

    /*
     * Query the Physical TLB entry for the page (may fail).
     */
    PPGMPAGEMAPTLBE pTlbe;
    rc = pgmPhysPageQueryTlbe(pVM, GCPhys, &pTlbe);
    if (RT_SUCCESS(rc))
    {
        PPGMPAGE pPage = pTlbe->pPage;
#if 1
        /* MMIO pages doesn't have any readable backing. */
        if (PGM_PAGE_IS_MMIO_OR_SPECIAL_ALIAS(pPage))
            rc = VERR_PGM_PHYS_PAGE_RESERVED;
#else
        if (PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(pPage))
            rc = VERR_PGM_PHYS_PAGE_RESERVED;
#endif
        else
        {
            /*
             * Now, just perform the locking and calculate the return address.
             */
            PPGMPAGEMAP pMap = pTlbe->pMap;
            if (pMap)
                pMap->cRefs++;

            unsigned cLocks = PGM_PAGE_GET_READ_LOCKS(pPage);
            if (RT_LIKELY(cLocks < PGM_PAGE_MAX_LOCKS - 1))
            {
                if (cLocks == 0)
                    pVM->pgm.s.cReadLockedPages++;
                PGM_PAGE_INC_READ_LOCKS(pPage);
            }
            else if (cLocks != PGM_PAGE_GET_READ_LOCKS(pPage))
            {
                PGM_PAGE_INC_READ_LOCKS(pPage);
                AssertMsgFailed(("%RGp / %R[pgmpage] is entering permanent readonly locked state!\n", GCPhys, pPage));
                if (pMap)
                    pMap->cRefs++; /* Extra ref to prevent it from going away. */
            }

            *ppv = (void *)((uintptr_t)pTlbe->pv | (uintptr_t)(GCPhys & GUEST_PAGE_OFFSET_MASK));
            pLock->uPageAndType = (uintptr_t)pPage | PGMPAGEMAPLOCK_TYPE_READ;
            pLock->pvMap = pMap;
        }
    }

    PGM_UNLOCK(pVM);
    return rc;
}


/**
 * Requests the mapping of multiple guest page into ring-3, external threads.
 *
 * When you're done with the pages, call PGMPhysBulkReleasePageMappingLock()
 * ASAP to release them.
 *
 * This API will assume your intention is to write to the pages, and will
 * therefore replace shared and zero pages. If you do not intend to modify the
 * pages, use the PGMR3PhysBulkGCPhys2CCPtrReadOnlyExternal() API.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_PGM_PHYS_PAGE_RESERVED if any of the pages has no physical
 *          backing or if any of the pages the page has any active access
 *          handlers. The caller must fall back on using PGMR3PhysWriteExternal.
 * @retval  VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS if @a paGCPhysPages contains
 *          an invalid physical address.
 *
 * @param   pVM             The cross context VM structure.
 * @param   cPages          Number of pages to lock.
 * @param   paGCPhysPages   The guest physical address of the pages that
 *                          should be mapped (@a cPages entries).
 * @param   papvPages       Where to store the ring-3 mapping addresses
 *                          corresponding to @a paGCPhysPages.
 * @param   paLocks         Where to store the locking information that
 *                          pfnPhysBulkReleasePageMappingLock needs (@a cPages
 *                          in length).
 *
 * @remark  Avoid calling this API from within critical sections (other than the
 *          PGM one) because of the deadlock risk when we have to delegating the
 *          task to an EMT.
 * @thread  Any.
 */
VMMR3DECL(int) PGMR3PhysBulkGCPhys2CCPtrExternal(PVM pVM, uint32_t cPages, PCRTGCPHYS paGCPhysPages,
                                                 void **papvPages, PPGMPAGEMAPLOCK paLocks)
{
    Assert(cPages > 0);
    AssertPtr(papvPages);
    AssertPtr(paLocks);

    Assert(VM_IS_EMT(pVM) || !PGMIsLockOwner(pVM));

    int rc = PGM_LOCK(pVM);
    AssertRCReturn(rc, rc);

    /*
     * Lock the pages one by one.
     * The loop body is similar to PGMR3PhysGCPhys2CCPtrExternal.
     */
    int32_t  cNextYield = 128;
    uint32_t iPage;
    for (iPage = 0; iPage < cPages; iPage++)
    {
        if (--cNextYield > 0)
        { /* likely */ }
        else
        {
            PGM_UNLOCK(pVM);
            ASMNopPause();
            PGM_LOCK_VOID(pVM);
            cNextYield = 128;
        }

        /*
         * Query the Physical TLB entry for the page (may fail).
         */
        PPGMPAGEMAPTLBE pTlbe;
        rc = pgmPhysPageQueryTlbe(pVM, paGCPhysPages[iPage], &pTlbe);
        if (RT_SUCCESS(rc))
        { }
        else
            break;
        PPGMPAGE pPage = pTlbe->pPage;

        /*
         * No MMIO or active access handlers.
         */
        if (   !PGM_PAGE_IS_MMIO_OR_SPECIAL_ALIAS(pPage)
            && !PGM_PAGE_HAS_ACTIVE_HANDLERS(pPage))
        { }
        else
        {
            rc = VERR_PGM_PHYS_PAGE_RESERVED;
            break;
        }

        /*
         * The page must be in the allocated state and not be a dirty pool page.
         * We can handle converting a write monitored page to an allocated one, but
         * anything more complicated must be delegated to an EMT.
         */
        bool fDelegateToEmt = false;
        if (PGM_PAGE_GET_STATE(pPage) == PGM_PAGE_STATE_ALLOCATED)
#ifdef PGMPOOL_WITH_OPTIMIZED_DIRTY_PT
            fDelegateToEmt = pgmPoolIsDirtyPage(pVM, paGCPhysPages[iPage]);
#else
            fDelegateToEmt = false;
#endif
        else if (PGM_PAGE_GET_STATE(pPage) == PGM_PAGE_STATE_WRITE_MONITORED)
        {
#ifdef PGMPOOL_WITH_OPTIMIZED_DIRTY_PT
            if (!pgmPoolIsDirtyPage(pVM, paGCPhysPages[iPage]))
                pgmPhysPageMakeWriteMonitoredWritable(pVM, pPage, paGCPhysPages[iPage]);
            else
                fDelegateToEmt = true;
#endif
        }
        else
            fDelegateToEmt = true;
        if (!fDelegateToEmt)
        { }
        else
        {
            /* We could do this delegation in bulk, but considered too much work vs gain. */
            PGM_UNLOCK(pVM);
            rc = VMR3ReqPriorityCallWait(pVM, VMCPUID_ANY, (PFNRT)pgmR3PhysGCPhys2CCPtrDelegated, 4,
                                         pVM, &paGCPhysPages[iPage], &papvPages[iPage], &paLocks[iPage]);
            PGM_LOCK_VOID(pVM);
            if (RT_FAILURE(rc))
                break;
            cNextYield = 128;
        }

        /*
         * Now, just perform the locking and address calculation.
         */
        PPGMPAGEMAP pMap = pTlbe->pMap;
        if (pMap)
            pMap->cRefs++;

        unsigned cLocks = PGM_PAGE_GET_WRITE_LOCKS(pPage);
        if (RT_LIKELY(cLocks < PGM_PAGE_MAX_LOCKS - 1))
        {
            if (cLocks == 0)
                pVM->pgm.s.cWriteLockedPages++;
            PGM_PAGE_INC_WRITE_LOCKS(pPage);
        }
        else if (cLocks != PGM_PAGE_GET_WRITE_LOCKS(pPage))
        {
            PGM_PAGE_INC_WRITE_LOCKS(pPage);
            AssertMsgFailed(("%RGp / %R[pgmpage] is entering permanent write locked state!\n", paGCPhysPages[iPage], pPage));
            if (pMap)
                pMap->cRefs++; /* Extra ref to prevent it from going away. */
        }

        papvPages[iPage] = (void *)((uintptr_t)pTlbe->pv | (uintptr_t)(paGCPhysPages[iPage] & GUEST_PAGE_OFFSET_MASK));
        paLocks[iPage].uPageAndType = (uintptr_t)pPage | PGMPAGEMAPLOCK_TYPE_WRITE;
        paLocks[iPage].pvMap        = pMap;
    }

    PGM_UNLOCK(pVM);

    /*
     * On failure we must unlock any pages we managed to get already.
     */
    if (RT_FAILURE(rc) && iPage > 0)
        PGMPhysBulkReleasePageMappingLocks(pVM, iPage, paLocks);

    return rc;
}


/**
 * Requests the mapping of multiple guest page into ring-3, for reading only,
 * external threads.
 *
 * When you're done with the pages, call PGMPhysReleasePageMappingLock() ASAP
 * to release them.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_PGM_PHYS_PAGE_RESERVED if any of the pages has no physical
 *          backing or if any of the pages the page has an active ALL access
 *          handler. The caller must fall back on using PGMR3PhysWriteExternal.
 * @retval  VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS if @a paGCPhysPages contains
 *          an invalid physical address.
 *
 * @param   pVM             The cross context VM structure.
 * @param   cPages          Number of pages to lock.
 * @param   paGCPhysPages   The guest physical address of the pages that
 *                          should be mapped (@a cPages entries).
 * @param   papvPages       Where to store the ring-3 mapping addresses
 *                          corresponding to @a paGCPhysPages.
 * @param   paLocks         Where to store the lock information that
 *                          pfnPhysReleasePageMappingLock needs (@a cPages
 *                          in length).
 *
 * @remark  Avoid calling this API from within critical sections (other than
 *          the PGM one) because of the deadlock risk.
 * @thread  Any.
 */
VMMR3DECL(int) PGMR3PhysBulkGCPhys2CCPtrReadOnlyExternal(PVM pVM, uint32_t cPages, PCRTGCPHYS paGCPhysPages,
                                                         void const **papvPages, PPGMPAGEMAPLOCK paLocks)
{
    Assert(cPages > 0);
    AssertPtr(papvPages);
    AssertPtr(paLocks);

    Assert(VM_IS_EMT(pVM) || !PGMIsLockOwner(pVM));

    int rc = PGM_LOCK(pVM);
    AssertRCReturn(rc, rc);

    /*
     * Lock the pages one by one.
     * The loop body is similar to PGMR3PhysGCPhys2CCPtrReadOnlyExternal.
     */
    int32_t  cNextYield = 256;
    uint32_t iPage;
    for (iPage = 0; iPage < cPages; iPage++)
    {
        if (--cNextYield > 0)
        { /* likely */ }
        else
        {
            PGM_UNLOCK(pVM);
            ASMNopPause();
            PGM_LOCK_VOID(pVM);
            cNextYield = 256;
        }

        /*
         * Query the Physical TLB entry for the page (may fail).
         */
        PPGMPAGEMAPTLBE pTlbe;
        rc = pgmPhysPageQueryTlbe(pVM, paGCPhysPages[iPage], &pTlbe);
        if (RT_SUCCESS(rc))
        { }
        else
            break;
        PPGMPAGE pPage = pTlbe->pPage;

        /*
         * No MMIO or active all access handlers, everything else can be accessed.
         */
        if (   !PGM_PAGE_IS_MMIO_OR_SPECIAL_ALIAS(pPage)
            && !PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(pPage))
        { }
        else
        {
            rc = VERR_PGM_PHYS_PAGE_RESERVED;
            break;
        }

        /*
         * Now, just perform the locking and address calculation.
         */
        PPGMPAGEMAP pMap = pTlbe->pMap;
        if (pMap)
            pMap->cRefs++;

        unsigned cLocks = PGM_PAGE_GET_READ_LOCKS(pPage);
        if (RT_LIKELY(cLocks < PGM_PAGE_MAX_LOCKS - 1))
        {
            if (cLocks == 0)
                pVM->pgm.s.cReadLockedPages++;
            PGM_PAGE_INC_READ_LOCKS(pPage);
        }
        else if (cLocks != PGM_PAGE_GET_READ_LOCKS(pPage))
        {
            PGM_PAGE_INC_READ_LOCKS(pPage);
            AssertMsgFailed(("%RGp / %R[pgmpage] is entering permanent readonly locked state!\n", paGCPhysPages[iPage], pPage));
            if (pMap)
                pMap->cRefs++; /* Extra ref to prevent it from going away. */
        }

        papvPages[iPage] = (void *)((uintptr_t)pTlbe->pv | (uintptr_t)(paGCPhysPages[iPage] & GUEST_PAGE_OFFSET_MASK));
        paLocks[iPage].uPageAndType = (uintptr_t)pPage | PGMPAGEMAPLOCK_TYPE_READ;
        paLocks[iPage].pvMap        = pMap;
    }

    PGM_UNLOCK(pVM);

    /*
     * On failure we must unlock any pages we managed to get already.
     */
    if (RT_FAILURE(rc) && iPage > 0)
        PGMPhysBulkReleasePageMappingLocks(pVM, iPage, paLocks);

    return rc;
}


/**
 * Converts a GC physical address to a HC ring-3 pointer, with some
 * additional checks.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VINF_PGM_PHYS_TLB_CATCH_WRITE and *ppv set if the page has a write
 *          access handler of some kind.
 * @retval  VERR_PGM_PHYS_TLB_CATCH_ALL if the page has a handler catching all
 *          accesses or is odd in any way.
 * @retval  VERR_PGM_PHYS_TLB_UNASSIGNED if the page doesn't exist.
 *
 * @param   pVM         The cross context VM structure.
 * @param   GCPhys      The GC physical address to convert.  Since this is only
 *                      used for filling the REM TLB, the A20 mask must be
 *                      applied before calling this API.
 * @param   fWritable   Whether write access is required.
 * @param   ppv         Where to store the pointer corresponding to GCPhys on
 *                      success.
 */
VMMR3DECL(int) PGMR3PhysTlbGCPhys2Ptr(PVM pVM, RTGCPHYS GCPhys, bool fWritable, void **ppv)
{
    PGM_LOCK_VOID(pVM);
    PGM_A20_ASSERT_MASKED(VMMGetCpu(pVM), GCPhys);

    PPGMRAMRANGE pRam;
    PPGMPAGE pPage;
    int rc = pgmPhysGetPageAndRangeEx(pVM, GCPhys, &pPage, &pRam);
    if (RT_SUCCESS(rc))
    {
        if (PGM_PAGE_IS_BALLOONED(pPage))
            rc = VINF_PGM_PHYS_TLB_CATCH_WRITE;
        else if (!PGM_PAGE_HAS_ANY_HANDLERS(pPage))
            rc = VINF_SUCCESS;
        else
        {
            if (PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(pPage)) /* catches MMIO */
                rc = VERR_PGM_PHYS_TLB_CATCH_ALL;
            else if (PGM_PAGE_HAS_ACTIVE_HANDLERS(pPage))
            {
                /** @todo Handle TLB loads of virtual handlers so ./test.sh can be made to work
                 *        in -norawr0 mode. */
                if (fWritable)
                    rc = VINF_PGM_PHYS_TLB_CATCH_WRITE;
            }
            else
            {
                /* Temporarily disabled physical handler(s), since the recompiler
                   doesn't get notified when it's reset we'll have to pretend it's
                   operating normally. */
                if (pgmHandlerPhysicalIsAll(pVM, GCPhys))
                    rc = VERR_PGM_PHYS_TLB_CATCH_ALL;
                else
                    rc = VINF_PGM_PHYS_TLB_CATCH_WRITE;
            }
        }
        if (RT_SUCCESS(rc))
        {
            int rc2;

            /* Make sure what we return is writable. */
            if (fWritable)
                switch (PGM_PAGE_GET_STATE(pPage))
                {
                    case PGM_PAGE_STATE_ALLOCATED:
                        break;
                    case PGM_PAGE_STATE_BALLOONED:
                        AssertFailed();
                        break;
                    case PGM_PAGE_STATE_ZERO:
                    case PGM_PAGE_STATE_SHARED:
                        if (rc == VINF_PGM_PHYS_TLB_CATCH_WRITE)
                            break;
                        RT_FALL_THRU();
                    case PGM_PAGE_STATE_WRITE_MONITORED:
                        rc2 = pgmPhysPageMakeWritable(pVM, pPage, GCPhys & ~(RTGCPHYS)GUEST_PAGE_OFFSET_MASK);
                        AssertLogRelRCReturn(rc2, rc2);
                        break;
                }

            /* Get a ring-3 mapping of the address. */
            PPGMPAGER3MAPTLBE pTlbe;
            rc2 = pgmPhysPageQueryTlbe(pVM, GCPhys, &pTlbe);
            AssertLogRelRCReturn(rc2, rc2);
            *ppv = (void *)((uintptr_t)pTlbe->pv | (uintptr_t)(GCPhys & GUEST_PAGE_OFFSET_MASK));
            /** @todo mapping/locking hell; this isn't horribly efficient since
             *        pgmPhysPageLoadIntoTlb will repeat the lookup we've done here. */

            Log6(("PGMR3PhysTlbGCPhys2Ptr: GCPhys=%RGp rc=%Rrc pPage=%R[pgmpage] *ppv=%p\n", GCPhys, rc, pPage, *ppv));
        }
        else
            Log6(("PGMR3PhysTlbGCPhys2Ptr: GCPhys=%RGp rc=%Rrc pPage=%R[pgmpage]\n", GCPhys, rc, pPage));

        /* else: handler catching all access, no pointer returned. */
    }
    else
        rc = VERR_PGM_PHYS_TLB_UNASSIGNED;

    PGM_UNLOCK(pVM);
    return rc;
}



/*********************************************************************************************************************************
*   RAM Range Management                                                                                                         *
*********************************************************************************************************************************/

#define MAKE_LEAF(a_pNode) \
    do { \
        (a_pNode)->pLeftR3  = NIL_RTR3PTR; \
        (a_pNode)->pRightR3 = NIL_RTR3PTR; \
        (a_pNode)->pLeftR0  = NIL_RTR0PTR; \
        (a_pNode)->pRightR0 = NIL_RTR0PTR; \
    } while (0)

#define INSERT_LEFT(a_pParent, a_pNode) \
    do { \
        (a_pParent)->pLeftR3 = (a_pNode); \
        (a_pParent)->pLeftR0 = (a_pNode)->pSelfR0; \
    } while (0)
#define INSERT_RIGHT(a_pParent, a_pNode) \
    do { \
        (a_pParent)->pRightR3 = (a_pNode); \
        (a_pParent)->pRightR0 = (a_pNode)->pSelfR0; \
    } while (0)


/**
 * Recursive tree builder.
 *
 * @param   ppRam           Pointer to the iterator variable.
 * @param   iDepth          The current depth.  Inserts a leaf node if 0.
 */
static PPGMRAMRANGE pgmR3PhysRebuildRamRangeSearchTreesRecursively(PPGMRAMRANGE *ppRam, int iDepth)
{
    PPGMRAMRANGE pRam;
    if (iDepth <= 0)
    {
        /*
         * Leaf node.
         */
        pRam = *ppRam;
        if (pRam)
        {
            *ppRam = pRam->pNextR3;
            MAKE_LEAF(pRam);
        }
    }
    else
    {

        /*
         * Intermediate node.
         */
        PPGMRAMRANGE pLeft = pgmR3PhysRebuildRamRangeSearchTreesRecursively(ppRam, iDepth - 1);

        pRam = *ppRam;
        if (!pRam)
            return pLeft;
        *ppRam = pRam->pNextR3;
        MAKE_LEAF(pRam);
        INSERT_LEFT(pRam, pLeft);

        PPGMRAMRANGE pRight = pgmR3PhysRebuildRamRangeSearchTreesRecursively(ppRam, iDepth - 1);
        if (pRight)
            INSERT_RIGHT(pRam, pRight);
    }
    return pRam;
}


/**
 * Rebuilds the RAM range search trees.
 *
 * @param   pVM         The cross context VM structure.
 */
static void pgmR3PhysRebuildRamRangeSearchTrees(PVM pVM)
{

    /*
     * Create the reasonably balanced tree in a sequential fashion.
     * For simplicity (laziness) we use standard recursion here.
     */
    int             iDepth = 0;
    PPGMRAMRANGE    pRam   = pVM->pgm.s.pRamRangesXR3;
    PPGMRAMRANGE    pRoot  = pgmR3PhysRebuildRamRangeSearchTreesRecursively(&pRam, 0);
    while (pRam)
    {
        PPGMRAMRANGE pLeft = pRoot;

        pRoot = pRam;
        pRam = pRam->pNextR3;
        MAKE_LEAF(pRoot);
        INSERT_LEFT(pRoot, pLeft);

        PPGMRAMRANGE pRight = pgmR3PhysRebuildRamRangeSearchTreesRecursively(&pRam, iDepth);
        if (pRight)
            INSERT_RIGHT(pRoot, pRight);
        /** @todo else: rotate the tree. */

        iDepth++;
    }

    pVM->pgm.s.pRamRangeTreeR3 = pRoot;
    pVM->pgm.s.pRamRangeTreeR0 = pRoot ? pRoot->pSelfR0 : NIL_RTR0PTR;

#ifdef VBOX_STRICT
    /*
     * Verify that the above code works.
     */
    unsigned cRanges = 0;
    for (pRam = pVM->pgm.s.pRamRangesXR3; pRam; pRam = pRam->pNextR3)
        cRanges++;
    Assert(cRanges > 0);

    unsigned cMaxDepth = ASMBitLastSetU32(cRanges);
    if ((1U << cMaxDepth) < cRanges)
        cMaxDepth++;

    for (pRam = pVM->pgm.s.pRamRangesXR3; pRam; pRam = pRam->pNextR3)
    {
        unsigned     cDepth = 0;
        PPGMRAMRANGE pRam2 = pVM->pgm.s.pRamRangeTreeR3;
        for (;;)
        {
            if (pRam == pRam2)
                break;
            Assert(pRam2);
            if (pRam->GCPhys < pRam2->GCPhys)
                pRam2 = pRam2->pLeftR3;
            else
                pRam2 = pRam2->pRightR3;
        }
        AssertMsg(cDepth <= cMaxDepth, ("cDepth=%d cMaxDepth=%d\n", cDepth, cMaxDepth));
    }
#endif /* VBOX_STRICT */
}

#undef MAKE_LEAF
#undef INSERT_LEFT
#undef INSERT_RIGHT

/**
 * Relinks the RAM ranges using the pSelfRC and pSelfR0 pointers.
 *
 * Called when anything was relocated.
 *
 * @param   pVM         The cross context VM structure.
 */
void pgmR3PhysRelinkRamRanges(PVM pVM)
{
    PPGMRAMRANGE pCur;

#ifdef VBOX_STRICT
    for (pCur = pVM->pgm.s.pRamRangesXR3; pCur; pCur = pCur->pNextR3)
    {
        Assert((pCur->GCPhys     & GUEST_PAGE_OFFSET_MASK) == 0);
        Assert((pCur->GCPhysLast & GUEST_PAGE_OFFSET_MASK) == GUEST_PAGE_OFFSET_MASK);
        Assert((pCur->cb         & GUEST_PAGE_OFFSET_MASK) == 0);
        Assert(pCur->cb == pCur->GCPhysLast - pCur->GCPhys + 1);
        for (PPGMRAMRANGE pCur2 = pVM->pgm.s.pRamRangesXR3; pCur2; pCur2 = pCur2->pNextR3)
            Assert(   pCur2 == pCur
                   || strcmp(pCur2->pszDesc, pCur->pszDesc)); /** @todo fix MMIO ranges!! */
    }
#endif

    pCur = pVM->pgm.s.pRamRangesXR3;
    if (pCur)
    {
        pVM->pgm.s.pRamRangesXR0 = pCur->pSelfR0;

        for (; pCur->pNextR3; pCur = pCur->pNextR3)
            pCur->pNextR0 = pCur->pNextR3->pSelfR0;

        Assert(pCur->pNextR0 == NIL_RTR0PTR);
    }
    else
    {
        Assert(pVM->pgm.s.pRamRangesXR0 == NIL_RTR0PTR);
    }
    ASMAtomicIncU32(&pVM->pgm.s.idRamRangesGen);

    pgmR3PhysRebuildRamRangeSearchTrees(pVM);
}


/**
 * Links a new RAM range into the list.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pNew        Pointer to the new list entry.
 * @param   pPrev       Pointer to the previous list entry. If NULL, insert as head.
 */
static void pgmR3PhysLinkRamRange(PVM pVM, PPGMRAMRANGE pNew, PPGMRAMRANGE pPrev)
{
    AssertMsg(pNew->pszDesc, ("%RGp-%RGp\n", pNew->GCPhys, pNew->GCPhysLast));

    PGM_LOCK_VOID(pVM);

    PPGMRAMRANGE pRam = pPrev ? pPrev->pNextR3 : pVM->pgm.s.pRamRangesXR3;
    pNew->pNextR3 = pRam;
    pNew->pNextR0 = pRam ? pRam->pSelfR0 : NIL_RTR0PTR;

    if (pPrev)
    {
        pPrev->pNextR3 = pNew;
        pPrev->pNextR0 = pNew->pSelfR0;
    }
    else
    {
        pVM->pgm.s.pRamRangesXR3 = pNew;
        pVM->pgm.s.pRamRangesXR0 = pNew->pSelfR0;
    }
    ASMAtomicIncU32(&pVM->pgm.s.idRamRangesGen);

    pgmR3PhysRebuildRamRangeSearchTrees(pVM);
    PGM_UNLOCK(pVM);
}


/**
 * Unlink an existing RAM range from the list.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pRam        Pointer to the new list entry.
 * @param   pPrev       Pointer to the previous list entry. If NULL, insert as head.
 */
static void pgmR3PhysUnlinkRamRange2(PVM pVM, PPGMRAMRANGE pRam, PPGMRAMRANGE pPrev)
{
    Assert(pPrev ? pPrev->pNextR3 == pRam : pVM->pgm.s.pRamRangesXR3 == pRam);

    PGM_LOCK_VOID(pVM);

    PPGMRAMRANGE pNext = pRam->pNextR3;
    if (pPrev)
    {
        pPrev->pNextR3 = pNext;
        pPrev->pNextR0 = pNext ? pNext->pSelfR0 : NIL_RTR0PTR;
    }
    else
    {
        Assert(pVM->pgm.s.pRamRangesXR3 == pRam);
        pVM->pgm.s.pRamRangesXR3 = pNext;
        pVM->pgm.s.pRamRangesXR0 = pNext ? pNext->pSelfR0 : NIL_RTR0PTR;
    }
    ASMAtomicIncU32(&pVM->pgm.s.idRamRangesGen);

    pgmR3PhysRebuildRamRangeSearchTrees(pVM);
    PGM_UNLOCK(pVM);
}


/**
 * Unlink an existing RAM range from the list.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pRam        Pointer to the new list entry.
 */
static void pgmR3PhysUnlinkRamRange(PVM pVM, PPGMRAMRANGE pRam)
{
    PGM_LOCK_VOID(pVM);

    /* find prev. */
    PPGMRAMRANGE pPrev = NULL;
    PPGMRAMRANGE pCur = pVM->pgm.s.pRamRangesXR3;
    while (pCur != pRam)
    {
        pPrev = pCur;
        pCur = pCur->pNextR3;
    }
    AssertFatal(pCur);

    pgmR3PhysUnlinkRamRange2(pVM, pRam, pPrev);
    PGM_UNLOCK(pVM);
}


/**
 * Gets the number of ram ranges.
 *
 * @returns Number of ram ranges.  Returns UINT32_MAX if @a pVM is invalid.
 * @param   pVM             The cross context VM structure.
 */
VMMR3DECL(uint32_t) PGMR3PhysGetRamRangeCount(PVM pVM)
{
    VM_ASSERT_VALID_EXT_RETURN(pVM, UINT32_MAX);

    PGM_LOCK_VOID(pVM);
    uint32_t cRamRanges = 0;
    for (PPGMRAMRANGE pCur = pVM->pgm.s.CTX_SUFF(pRamRangesX); pCur; pCur = pCur->CTX_SUFF(pNext))
        cRamRanges++;
    PGM_UNLOCK(pVM);
    return cRamRanges;
}


/**
 * Get information about a range.
 *
 * @returns VINF_SUCCESS or VERR_OUT_OF_RANGE.
 * @param   pVM             The cross context VM structure.
 * @param   iRange          The ordinal of the range.
 * @param   pGCPhysStart    Where to return the start of the range. Optional.
 * @param   pGCPhysLast     Where to return the address of the last byte in the
 *                          range. Optional.
 * @param   ppszDesc        Where to return the range description. Optional.
 * @param   pfIsMmio        Where to indicate that this is a pure MMIO range.
 *                          Optional.
 */
VMMR3DECL(int) PGMR3PhysGetRange(PVM pVM, uint32_t iRange, PRTGCPHYS pGCPhysStart, PRTGCPHYS pGCPhysLast,
                                 const char **ppszDesc, bool *pfIsMmio)
{
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);

    PGM_LOCK_VOID(pVM);
    uint32_t iCurRange = 0;
    for (PPGMRAMRANGE pCur = pVM->pgm.s.CTX_SUFF(pRamRangesX); pCur; pCur = pCur->CTX_SUFF(pNext), iCurRange++)
        if (iCurRange == iRange)
        {
            if (pGCPhysStart)
                *pGCPhysStart = pCur->GCPhys;
            if (pGCPhysLast)
                *pGCPhysLast  = pCur->GCPhysLast;
            if (ppszDesc)
                *ppszDesc     = pCur->pszDesc;
            if (pfIsMmio)
                *pfIsMmio     = !!(pCur->fFlags & PGM_RAM_RANGE_FLAGS_AD_HOC_MMIO);

            PGM_UNLOCK(pVM);
            return VINF_SUCCESS;
        }
    PGM_UNLOCK(pVM);
    return VERR_OUT_OF_RANGE;
}


/*********************************************************************************************************************************
*   RAM                                                                                                                          *
*********************************************************************************************************************************/

/**
 * Frees the specified RAM page and replaces it with the ZERO page.
 *
 * This is used by ballooning, remapping MMIO2, RAM reset and state loading.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pReq            Pointer to the request.  This is NULL when doing a
 *                          bulk free in NEM memory mode.
 * @param   pcPendingPages  Where the number of pages waiting to be freed are
 *                          kept.  This will normally be incremented.  This is
 *                          NULL when doing a bulk free in NEM memory mode.
 * @param   pPage           Pointer to the page structure.
 * @param   GCPhys          The guest physical address of the page, if applicable.
 * @param   enmNewType      New page type for NEM notification, since several
 *                          callers will change the type upon successful return.
 *
 * @remarks The caller must own the PGM lock.
 */
int pgmPhysFreePage(PVM pVM, PGMMFREEPAGESREQ pReq, uint32_t *pcPendingPages, PPGMPAGE pPage, RTGCPHYS GCPhys,
                    PGMPAGETYPE enmNewType)
{
    /*
     * Assert sanity.
     */
    PGM_LOCK_ASSERT_OWNER(pVM);
    if (RT_UNLIKELY(    PGM_PAGE_GET_TYPE(pPage) != PGMPAGETYPE_RAM
                    &&  PGM_PAGE_GET_TYPE(pPage) != PGMPAGETYPE_ROM_SHADOW))
    {
        AssertMsgFailed(("GCPhys=%RGp pPage=%R[pgmpage]\n", GCPhys, pPage));
        return VMSetError(pVM, VERR_PGM_PHYS_NOT_RAM, RT_SRC_POS, "GCPhys=%RGp type=%d", GCPhys, PGM_PAGE_GET_TYPE(pPage));
    }

    /** @todo What about ballooning of large pages??! */
    Assert(   PGM_PAGE_GET_PDE_TYPE(pPage) != PGM_PAGE_PDE_TYPE_PDE
           && PGM_PAGE_GET_PDE_TYPE(pPage) != PGM_PAGE_PDE_TYPE_PDE_DISABLED);

    if (    PGM_PAGE_IS_ZERO(pPage)
        ||  PGM_PAGE_IS_BALLOONED(pPage))
        return VINF_SUCCESS;

    const uint32_t idPage = PGM_PAGE_GET_PAGEID(pPage);
    Log3(("pgmPhysFreePage: idPage=%#x GCPhys=%RGp pPage=%R[pgmpage]\n", idPage, GCPhys, pPage));
    if (RT_UNLIKELY(!PGM_IS_IN_NEM_MODE(pVM)
                    ?    idPage == NIL_GMM_PAGEID
                      ||  idPage > GMM_PAGEID_LAST
                      ||  PGM_PAGE_GET_CHUNKID(pPage) == NIL_GMM_CHUNKID
                    :    idPage != NIL_GMM_PAGEID))
    {
        AssertMsgFailed(("GCPhys=%RGp pPage=%R[pgmpage]\n", GCPhys, pPage));
        return VMSetError(pVM, VERR_PGM_PHYS_INVALID_PAGE_ID, RT_SRC_POS, "GCPhys=%RGp idPage=%#x", GCPhys, pPage);
    }
#ifdef VBOX_WITH_NATIVE_NEM
    const RTHCPHYS HCPhysPrev = PGM_PAGE_GET_HCPHYS(pPage);
#endif

    /* update page count stats. */
    if (PGM_PAGE_IS_SHARED(pPage))
        pVM->pgm.s.cSharedPages--;
    else
        pVM->pgm.s.cPrivatePages--;
    pVM->pgm.s.cZeroPages++;

    /* Deal with write monitored pages. */
    if (PGM_PAGE_GET_STATE(pPage) == PGM_PAGE_STATE_WRITE_MONITORED)
    {
        PGM_PAGE_SET_WRITTEN_TO(pVM, pPage);
        pVM->pgm.s.cWrittenToPages++;
    }

    /*
     * pPage = ZERO page.
     */
    PGM_PAGE_SET_HCPHYS(pVM, pPage, pVM->pgm.s.HCPhysZeroPg);
    PGM_PAGE_SET_STATE(pVM, pPage, PGM_PAGE_STATE_ZERO);
    PGM_PAGE_SET_PAGEID(pVM, pPage, NIL_GMM_PAGEID);
    PGM_PAGE_SET_PDE_TYPE(pVM, pPage, PGM_PAGE_PDE_TYPE_DONTCARE);
    PGM_PAGE_SET_PTE_INDEX(pVM, pPage, 0);
    PGM_PAGE_SET_TRACKING(pVM, pPage, 0);

    /* Flush physical page map TLB entry. */
    pgmPhysInvalidatePageMapTLBEntry(pVM, GCPhys);
    IEMTlbInvalidateAllPhysicalAllCpus(pVM, NIL_VMCPUID); /// @todo move to the perform step.

#ifdef VBOX_WITH_PGM_NEM_MODE
    /*
     * Skip the rest if we're doing a bulk free in NEM memory mode.
     */
    if (!pReq)
        return VINF_SUCCESS;
    AssertLogRelReturn(!pVM->pgm.s.fNemMode, VERR_PGM_NOT_SUPPORTED_FOR_NEM_MODE);
#endif

#ifdef VBOX_WITH_NATIVE_NEM
    /* Notify NEM. */
    /** @todo Remove this one? */
    if (VM_IS_NEM_ENABLED(pVM))
    {
        uint8_t u2State = PGM_PAGE_GET_NEM_STATE(pPage);
        NEMHCNotifyPhysPageChanged(pVM, GCPhys, HCPhysPrev, pVM->pgm.s.HCPhysZeroPg, pVM->pgm.s.abZeroPg,
                                   pgmPhysPageCalcNemProtection(pPage, enmNewType), enmNewType, &u2State);
        PGM_PAGE_SET_NEM_STATE(pPage, u2State);
    }
#else
    RT_NOREF(enmNewType);
#endif

    /*
     * Make sure it's not in the handy page array.
     */
    for (uint32_t i = pVM->pgm.s.cHandyPages; i < RT_ELEMENTS(pVM->pgm.s.aHandyPages); i++)
    {
        if (pVM->pgm.s.aHandyPages[i].idPage == idPage)
        {
            pVM->pgm.s.aHandyPages[i].HCPhysGCPhys = NIL_GMMPAGEDESC_PHYS;
            pVM->pgm.s.aHandyPages[i].fZeroed      = false;
            pVM->pgm.s.aHandyPages[i].idPage       = NIL_GMM_PAGEID;
            break;
        }
        if (pVM->pgm.s.aHandyPages[i].idSharedPage == idPage)
        {
            pVM->pgm.s.aHandyPages[i].idSharedPage = NIL_GMM_PAGEID;
            break;
        }
    }

    /*
     * Push it onto the page array.
     */
    uint32_t iPage = *pcPendingPages;
    Assert(iPage < PGMPHYS_FREE_PAGE_BATCH_SIZE);
    *pcPendingPages += 1;

    pReq->aPages[iPage].idPage = idPage;

    if (iPage + 1 < PGMPHYS_FREE_PAGE_BATCH_SIZE)
        return VINF_SUCCESS;

    /*
     * Flush the pages.
     */
    int rc = GMMR3FreePagesPerform(pVM, pReq, PGMPHYS_FREE_PAGE_BATCH_SIZE);
    if (RT_SUCCESS(rc))
    {
        GMMR3FreePagesRePrep(pVM, pReq, PGMPHYS_FREE_PAGE_BATCH_SIZE, GMMACCOUNT_BASE);
        *pcPendingPages = 0;
    }
    return rc;
}


/**
 * Frees a range of pages, replacing them with ZERO pages of the specified type.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pRam        The RAM range in which the pages resides.
 * @param   GCPhys      The address of the first page.
 * @param   GCPhysLast  The address of the last page.
 * @param   pvMmio2     Pointer to the ring-3 mapping of any MMIO2 memory that
 *                      will replace the pages we're freeing up.
 */
static int pgmR3PhysFreePageRange(PVM pVM, PPGMRAMRANGE pRam, RTGCPHYS GCPhys, RTGCPHYS GCPhysLast, void *pvMmio2)
{
    PGM_LOCK_ASSERT_OWNER(pVM);

#ifdef VBOX_WITH_PGM_NEM_MODE
    /*
     * In simplified memory mode we don't actually free the memory,
     * we just unmap it and let NEM do any unlocking of it.
     */
    if (pVM->pgm.s.fNemMode)
    {
        Assert(VM_IS_NEM_ENABLED(pVM) || VM_IS_EXEC_ENGINE_IEM(pVM));
        uint8_t u2State = 0; /* (We don't support UINT8_MAX here.) */
        if (VM_IS_NEM_ENABLED(pVM))
        {
            uint32_t const  fNemNotify = (pvMmio2 ? NEM_NOTIFY_PHYS_MMIO_EX_F_MMIO2 : 0) | NEM_NOTIFY_PHYS_MMIO_EX_F_REPLACE;
            int rc = NEMR3NotifyPhysMmioExMapEarly(pVM, GCPhys, GCPhysLast - GCPhys + 1, fNemNotify,
                                                   pRam->pvR3 ? (uint8_t *)pRam->pvR3 + GCPhys - pRam->GCPhys : NULL,
                                                   pvMmio2, &u2State, NULL /*puNemRange*/);
            AssertLogRelRCReturn(rc, rc);
        }

        /* Iterate the pages. */
        PPGMPAGE pPageDst   = &pRam->aPages[(GCPhys - pRam->GCPhys) >> GUEST_PAGE_SHIFT];
        uint32_t cPagesLeft = ((GCPhysLast - GCPhys) >> GUEST_PAGE_SHIFT) + 1;
        while (cPagesLeft-- > 0)
        {
            int rc = pgmPhysFreePage(pVM, NULL, NULL, pPageDst, GCPhys, PGMPAGETYPE_MMIO);
            AssertLogRelRCReturn(rc, rc); /* We're done for if this goes wrong. */

            PGM_PAGE_SET_TYPE(pVM, pPageDst, PGMPAGETYPE_MMIO);
            PGM_PAGE_SET_NEM_STATE(pPageDst, u2State);

            GCPhys += GUEST_PAGE_SIZE;
            pPageDst++;
        }
        return VINF_SUCCESS;
    }
#else  /* !VBOX_WITH_PGM_NEM_MODE */
    RT_NOREF(pvMmio2);
#endif /* !VBOX_WITH_PGM_NEM_MODE */

    /*
     * Regular mode.
     */
    /* Prepare. */
    uint32_t            cPendingPages = 0;
    PGMMFREEPAGESREQ    pReq;
    int rc = GMMR3FreePagesPrepare(pVM, &pReq, PGMPHYS_FREE_PAGE_BATCH_SIZE, GMMACCOUNT_BASE);
    AssertLogRelRCReturn(rc, rc);

#ifdef VBOX_WITH_NATIVE_NEM
    /* Tell NEM up-front. */
    uint8_t u2State = UINT8_MAX;
    if (VM_IS_NEM_ENABLED(pVM))
    {
        uint32_t const fNemNotify = (pvMmio2 ? NEM_NOTIFY_PHYS_MMIO_EX_F_MMIO2 : 0) | NEM_NOTIFY_PHYS_MMIO_EX_F_REPLACE;
        rc = NEMR3NotifyPhysMmioExMapEarly(pVM, GCPhys, GCPhysLast - GCPhys + 1, fNemNotify, NULL, pvMmio2,
                                           &u2State, NULL /*puNemRange*/);
        AssertLogRelRCReturnStmt(rc, GMMR3FreePagesCleanup(pReq), rc);
    }
#endif

    /* Iterate the pages. */
    PPGMPAGE pPageDst   = &pRam->aPages[(GCPhys - pRam->GCPhys) >> GUEST_PAGE_SHIFT];
    uint32_t cPagesLeft = ((GCPhysLast - GCPhys) >> GUEST_PAGE_SHIFT) + 1;
    while (cPagesLeft-- > 0)
    {
        rc = pgmPhysFreePage(pVM, pReq, &cPendingPages, pPageDst, GCPhys, PGMPAGETYPE_MMIO);
        AssertLogRelRCReturn(rc, rc); /* We're done for if this goes wrong. */

        PGM_PAGE_SET_TYPE(pVM, pPageDst, PGMPAGETYPE_MMIO);
#ifdef VBOX_WITH_NATIVE_NEM
        if (u2State != UINT8_MAX)
            PGM_PAGE_SET_NEM_STATE(pPageDst, u2State);
#endif

        GCPhys += GUEST_PAGE_SIZE;
        pPageDst++;
    }

    /* Finish pending and cleanup. */
    if (cPendingPages)
    {
        rc = GMMR3FreePagesPerform(pVM, pReq, cPendingPages);
        AssertLogRelRCReturn(rc, rc);
    }
    GMMR3FreePagesCleanup(pReq);

    return rc;
}


/**
 * PGMR3PhysRegisterRam worker that initializes and links a RAM range.
 *
 * In NEM mode, this will allocate the pages backing the RAM range and this may
 * fail.  NEM registration may also fail.  (In regular HM mode it won't fail.)
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pNew            The new RAM range.
 * @param   GCPhys          The address of the RAM range.
 * @param   GCPhysLast      The last address of the RAM range.
 * @param   R0PtrNew        Ditto for R0.
 * @param   fFlags          PGM_RAM_RANGE_FLAGS_FLOATING or zero.
 * @param   pszDesc         The description.
 * @param   pPrev           The previous RAM range (for linking).
 */
static int pgmR3PhysInitAndLinkRamRange(PVM pVM, PPGMRAMRANGE pNew, RTGCPHYS GCPhys, RTGCPHYS GCPhysLast,
                                        RTR0PTR R0PtrNew, uint32_t fFlags, const char *pszDesc, PPGMRAMRANGE pPrev)
{
    /*
     * Initialize the range.
     */
    pNew->pSelfR0       = R0PtrNew;
    pNew->GCPhys        = GCPhys;
    pNew->GCPhysLast    = GCPhysLast;
    pNew->cb            = GCPhysLast - GCPhys + 1;
    pNew->pszDesc       = pszDesc;
    pNew->fFlags        = fFlags;
    pNew->uNemRange     = UINT32_MAX;
    pNew->pvR3          = NULL;
    pNew->paLSPages     = NULL;

    uint32_t const cPages = pNew->cb >> GUEST_PAGE_SHIFT;
#ifdef VBOX_WITH_PGM_NEM_MODE
    if (!pVM->pgm.s.fNemMode)
#endif
    {
        RTGCPHYS iPage = cPages;
        while (iPage-- > 0)
            PGM_PAGE_INIT_ZERO(&pNew->aPages[iPage], pVM, PGMPAGETYPE_RAM);

        /* Update the page count stats. */
        pVM->pgm.s.cZeroPages += cPages;
        pVM->pgm.s.cAllPages  += cPages;
    }
#ifdef VBOX_WITH_PGM_NEM_MODE
    else
    {
        int rc = SUPR3PageAlloc(RT_ALIGN_Z(pNew->cb, HOST_PAGE_SIZE) >> HOST_PAGE_SHIFT,
                                pVM->pgm.s.fUseLargePages ? SUP_PAGE_ALLOC_F_LARGE_PAGES : 0, &pNew->pvR3);
        if (RT_FAILURE(rc))
            return rc;

        RTGCPHYS iPage = cPages;
        while (iPage-- > 0)
            PGM_PAGE_INIT(&pNew->aPages[iPage], UINT64_C(0x0000fffffffff000), NIL_GMM_PAGEID,
                          PGMPAGETYPE_RAM, PGM_PAGE_STATE_ALLOCATED);

        /* Update the page count stats. */
        pVM->pgm.s.cPrivatePages += cPages;
        pVM->pgm.s.cAllPages     += cPages;
    }
#endif

    /*
     * Link it.
     */
    pgmR3PhysLinkRamRange(pVM, pNew, pPrev);

#ifdef VBOX_WITH_NATIVE_NEM
    /*
     * Notify NEM now that it has been linked.
     */
    if (VM_IS_NEM_ENABLED(pVM))
    {
        uint8_t u2State = UINT8_MAX;
        int rc = NEMR3NotifyPhysRamRegister(pVM, GCPhys, pNew->cb, pNew->pvR3, &u2State, &pNew->uNemRange);
        if (RT_SUCCESS(rc))
        {
            if (u2State != UINT8_MAX)
                pgmPhysSetNemStateForPages(&pNew->aPages[0], cPages, u2State);
        }
        else
            pgmR3PhysUnlinkRamRange2(pVM, pNew, pPrev);
        return rc;
    }
#endif
    return VINF_SUCCESS;
}


/**
 * PGMR3PhysRegisterRam worker that registers a high chunk.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   GCPhys          The address of the RAM.
 * @param   cRamPages       The number of RAM pages to register.
 * @param   iChunk          The chunk number.
 * @param   pszDesc         The RAM range description.
 * @param   ppPrev          Previous RAM range pointer. In/Out.
 */
static int pgmR3PhysRegisterHighRamChunk(PVM pVM, RTGCPHYS GCPhys, uint32_t cRamPages, uint32_t iChunk,
                                         const char *pszDesc, PPGMRAMRANGE *ppPrev)
{
    const char *pszDescChunk = iChunk == 0
                             ? pszDesc
                             : MMR3HeapAPrintf(pVM, MM_TAG_PGM_PHYS, "%s (#%u)", pszDesc, iChunk + 1);
    AssertReturn(pszDescChunk, VERR_NO_MEMORY);

    /*
     * Allocate memory for the new chunk.
     */
    size_t const cChunkPages  = RT_ALIGN_Z(RT_UOFFSETOF_DYN(PGMRAMRANGE, aPages[cRamPages]), HOST_PAGE_SIZE) >> HOST_PAGE_SHIFT;
    PSUPPAGE     paChunkPages = (PSUPPAGE)RTMemTmpAllocZ(sizeof(SUPPAGE) * cChunkPages);
    AssertReturn(paChunkPages, VERR_NO_TMP_MEMORY);
    RTR0PTR      R0PtrChunk   = NIL_RTR0PTR;
    void        *pvChunk      = NULL;
    int rc = SUPR3PageAllocEx(cChunkPages, 0 /*fFlags*/, &pvChunk, &R0PtrChunk, paChunkPages);
    if (RT_SUCCESS(rc))
    {
        Assert(R0PtrChunk != NIL_RTR0PTR || PGM_IS_IN_NEM_MODE(pVM));
        memset(pvChunk, 0, cChunkPages << HOST_PAGE_SHIFT);

        PPGMRAMRANGE pNew = (PPGMRAMRANGE)pvChunk;

        /*
         * Ok, init and link the range.
         */
        rc = pgmR3PhysInitAndLinkRamRange(pVM, pNew, GCPhys, GCPhys + ((RTGCPHYS)cRamPages << GUEST_PAGE_SHIFT) - 1,
                                          R0PtrChunk, PGM_RAM_RANGE_FLAGS_FLOATING, pszDescChunk, *ppPrev);
        if (RT_SUCCESS(rc))
            *ppPrev = pNew;

        if (RT_FAILURE(rc))
            SUPR3PageFreeEx(pvChunk, cChunkPages);
    }

    RTMemTmpFree(paChunkPages);
    return rc;
}


/**
 * Sets up a range RAM.
 *
 * This will check for conflicting registrations, make a resource
 * reservation for the memory (with GMM), and setup the per-page
 * tracking structures (PGMPAGE).
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   GCPhys          The physical address of the RAM.
 * @param   cb              The size of the RAM.
 * @param   pszDesc         The description - not copied, so, don't free or change it.
 */
VMMR3DECL(int) PGMR3PhysRegisterRam(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, const char *pszDesc)
{
   /*
     * Validate input.
     */
    Log(("PGMR3PhysRegisterRam: GCPhys=%RGp cb=%RGp pszDesc=%s\n", GCPhys, cb, pszDesc));
    AssertReturn(RT_ALIGN_T(GCPhys, GUEST_PAGE_SIZE, RTGCPHYS) == GCPhys, VERR_INVALID_PARAMETER);
    AssertReturn(RT_ALIGN_T(cb, GUEST_PAGE_SIZE, RTGCPHYS) == cb, VERR_INVALID_PARAMETER);
    AssertReturn(cb > 0, VERR_INVALID_PARAMETER);
    RTGCPHYS GCPhysLast = GCPhys + (cb - 1);
    AssertMsgReturn(GCPhysLast > GCPhys, ("The range wraps! GCPhys=%RGp cb=%RGp\n", GCPhys, cb), VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszDesc, VERR_INVALID_POINTER);
    VM_ASSERT_EMT_RETURN(pVM, VERR_VM_THREAD_NOT_EMT);

    PGM_LOCK_VOID(pVM);

    /*
     * Find range location and check for conflicts.
     */
    PPGMRAMRANGE    pPrev = NULL;
    PPGMRAMRANGE    pRam = pVM->pgm.s.pRamRangesXR3;
    while (pRam && GCPhysLast >= pRam->GCPhys)
    {
        AssertLogRelMsgReturnStmt(   GCPhysLast < pRam->GCPhys
                                  || GCPhys     > pRam->GCPhysLast,
                                  ("%RGp-%RGp (%s) conflicts with existing %RGp-%RGp (%s)\n",
                                   GCPhys, GCPhysLast, pszDesc, pRam->GCPhys, pRam->GCPhysLast, pRam->pszDesc),
                                  PGM_UNLOCK(pVM), VERR_PGM_RAM_CONFLICT);

        /* next */
        pPrev = pRam;
        pRam = pRam->pNextR3;
    }

    /*
     * Register it with GMM (the API bitches).
     */
    const RTGCPHYS cPages = cb >> GUEST_PAGE_SHIFT;
    int rc = MMR3IncreaseBaseReservation(pVM, cPages);
    if (RT_FAILURE(rc))
    {
        PGM_UNLOCK(pVM);
        return rc;
    }

    if (    GCPhys >= _4G
        &&  cPages > 256)
    {
        /*
         * The PGMRAMRANGE structures for the high memory can get very big.
         * There used to be some limitations on SUPR3PageAllocEx allocation
         * sizes, so traditionally we limited this to 16MB chunks. These days
         * we do ~64 MB chunks each covering 16GB of guest RAM, making sure
         * each range is a multiple of 1GB to enable eager hosts to use 1GB
         * pages in NEM mode.
         *
         * See also pgmR3PhysMmio2CalcChunkCount.
         */
        uint32_t const cPagesPerChunk = _4M;
        Assert(RT_ALIGN_32(cPagesPerChunk, X86_PD_PAE_SHIFT - X86_PAGE_SHIFT)); /* NEM large page requirement: 1GB pages. */

        RTGCPHYS cPagesLeft  = cPages;
        RTGCPHYS GCPhysChunk = GCPhys;
        uint32_t iChunk      = 0;
        while (cPagesLeft > 0)
        {
            uint32_t cPagesInChunk = cPagesLeft;
            if (cPagesInChunk > cPagesPerChunk)
                cPagesInChunk = cPagesPerChunk;

            rc = pgmR3PhysRegisterHighRamChunk(pVM, GCPhysChunk, cPagesInChunk, iChunk, pszDesc, &pPrev);
            AssertRCReturn(rc, rc);

            /* advance */
            GCPhysChunk += (RTGCPHYS)cPagesInChunk << GUEST_PAGE_SHIFT;
            cPagesLeft  -= cPagesInChunk;
            iChunk++;
        }
    }
    else
    {
        /*
         * Allocate, initialize and link the new RAM range.
         */
        const size_t cbRamRange = RT_UOFFSETOF_DYN(PGMRAMRANGE, aPages[cPages]);
        PPGMRAMRANGE pNew = NULL;
        RTR0PTR      pNewR0 = NIL_RTR0PTR;
        rc = SUPR3PageAllocEx(RT_ALIGN_Z(cbRamRange, HOST_PAGE_SIZE) >> HOST_PAGE_SHIFT, 0 /*fFlags*/,
                              (void **)&pNew, &pNewR0, NULL /*paPages*/);
        AssertLogRelMsgRCReturn(rc, ("rc=%Rrc cbRamRange=%zu\n", rc, cbRamRange), rc);

        rc = pgmR3PhysInitAndLinkRamRange(pVM, pNew, GCPhys, GCPhysLast, pNewR0, 0 /*fFlags*/, pszDesc, pPrev);
        AssertLogRelMsgRCReturn(rc, ("rc=%Rrc cbRamRange=%zu\n", rc, cbRamRange), rc);
    }
    pgmPhysInvalidatePageMapTLB(pVM);

    PGM_UNLOCK(pVM);
    return rc;
}


/**
 * Worker called by PGMR3InitFinalize if we're configured to pre-allocate RAM.
 *
 * We do this late in the init process so that all the ROM and MMIO ranges have
 * been registered already and we don't go wasting memory on them.
 *
 * @returns VBox status code.
 *
 * @param   pVM     The cross context VM structure.
 */
int pgmR3PhysRamPreAllocate(PVM pVM)
{
    Assert(pVM->pgm.s.fRamPreAlloc);
    Log(("pgmR3PhysRamPreAllocate: enter\n"));
#ifdef VBOX_WITH_PGM_NEM_MODE
    AssertLogRelReturn(!pVM->pgm.s.fNemMode, VERR_PGM_NOT_SUPPORTED_FOR_NEM_MODE);
#endif

    /*
     * Walk the RAM ranges and allocate all RAM pages, halt at
     * the first allocation error.
     */
    uint64_t cPages = 0;
    uint64_t NanoTS = RTTimeNanoTS();
    PGM_LOCK_VOID(pVM);
    for (PPGMRAMRANGE pRam = pVM->pgm.s.pRamRangesXR3; pRam; pRam = pRam->pNextR3)
    {
        PPGMPAGE    pPage  = &pRam->aPages[0];
        RTGCPHYS    GCPhys = pRam->GCPhys;
        uint32_t    cLeft  = pRam->cb >> GUEST_PAGE_SHIFT;
        while (cLeft-- > 0)
        {
            if (PGM_PAGE_GET_TYPE(pPage) == PGMPAGETYPE_RAM)
            {
                switch (PGM_PAGE_GET_STATE(pPage))
                {
                    case PGM_PAGE_STATE_ZERO:
                    {
                        int rc = pgmPhysAllocPage(pVM, pPage, GCPhys);
                        if (RT_FAILURE(rc))
                        {
                            LogRel(("PGM: RAM Pre-allocation failed at %RGp (in %s) with rc=%Rrc\n", GCPhys, pRam->pszDesc, rc));
                            PGM_UNLOCK(pVM);
                            return rc;
                        }
                        cPages++;
                        break;
                    }

                    case PGM_PAGE_STATE_BALLOONED:
                    case PGM_PAGE_STATE_ALLOCATED:
                    case PGM_PAGE_STATE_WRITE_MONITORED:
                    case PGM_PAGE_STATE_SHARED:
                        /* nothing to do here. */
                        break;
                }
            }

            /* next */
            pPage++;
            GCPhys += GUEST_PAGE_SIZE;
        }
    }
    PGM_UNLOCK(pVM);
    NanoTS = RTTimeNanoTS() - NanoTS;

    LogRel(("PGM: Pre-allocated %llu pages in %llu ms\n", cPages, NanoTS / 1000000));
    Log(("pgmR3PhysRamPreAllocate: returns VINF_SUCCESS\n"));
    return VINF_SUCCESS;
}


/**
 * Checks shared page checksums.
 *
 * @param   pVM     The cross context VM structure.
 */
void pgmR3PhysAssertSharedPageChecksums(PVM pVM)
{
#ifdef VBOX_STRICT
    PGM_LOCK_VOID(pVM);

    if (pVM->pgm.s.cSharedPages > 0)
    {
        /*
         * Walk the ram ranges.
         */
        for (PPGMRAMRANGE pRam = pVM->pgm.s.pRamRangesXR3; pRam; pRam = pRam->pNextR3)
        {
            uint32_t iPage = pRam->cb >> GUEST_PAGE_SHIFT;
            AssertMsg(((RTGCPHYS)iPage << GUEST_PAGE_SHIFT) == pRam->cb,
                      ("%RGp %RGp\n", (RTGCPHYS)iPage << GUEST_PAGE_SHIFT, pRam->cb));

            while (iPage-- > 0)
            {
                PPGMPAGE pPage = &pRam->aPages[iPage];
                if (PGM_PAGE_IS_SHARED(pPage))
                {
                    uint32_t u32Checksum = pPage->s.u2Unused0/* | ((uint32_t)pPage->s.u2Unused1 << 8)*/;
                    if (!u32Checksum)
                    {
                        RTGCPHYS    GCPhysPage  = pRam->GCPhys + ((RTGCPHYS)iPage << GUEST_PAGE_SHIFT);
                        void const *pvPage;
                        int rc = pgmPhysPageMapReadOnly(pVM, pPage, GCPhysPage, &pvPage);
                        if (RT_SUCCESS(rc))
                        {
                            uint32_t u32Checksum2 = RTCrc32(pvPage, GUEST_PAGE_SIZE);
# if 0
                            AssertMsg((u32Checksum2 & /*UINT32_C(0x00000303)*/ 0x3) == u32Checksum, ("GCPhysPage=%RGp\n", GCPhysPage));
# else
                            if ((u32Checksum2 & /*UINT32_C(0x00000303)*/ 0x3) == u32Checksum)
                                LogFlow(("shpg %#x @ %RGp %#x [OK]\n", PGM_PAGE_GET_PAGEID(pPage), GCPhysPage, u32Checksum2));
                            else
                                AssertMsgFailed(("shpg %#x @ %RGp %#x\n", PGM_PAGE_GET_PAGEID(pPage), GCPhysPage, u32Checksum2));
# endif
                        }
                        else
                            AssertRC(rc);
                    }
                }

            } /* for each page */

        } /* for each ram range */
    }

    PGM_UNLOCK(pVM);
#endif /* VBOX_STRICT */
    NOREF(pVM);
}


/**
 * Resets the physical memory state.
 *
 * ASSUMES that the caller owns the PGM lock.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 */
int pgmR3PhysRamReset(PVM pVM)
{
    PGM_LOCK_ASSERT_OWNER(pVM);

    /* Reset the memory balloon. */
    int rc = GMMR3BalloonedPages(pVM, GMMBALLOONACTION_RESET, 0);
    AssertRC(rc);

#ifdef VBOX_WITH_PAGE_SHARING
    /* Clear all registered shared modules. */
    pgmR3PhysAssertSharedPageChecksums(pVM);
    rc = GMMR3ResetSharedModules(pVM);
    AssertRC(rc);
#endif
    /* Reset counters. */
    pVM->pgm.s.cReusedSharedPages = 0;
    pVM->pgm.s.cBalloonedPages    = 0;

    return VINF_SUCCESS;
}


/**
 * Resets (zeros) the RAM after all devices and components have been reset.
 *
 * ASSUMES that the caller owns the PGM lock.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 */
int pgmR3PhysRamZeroAll(PVM pVM)
{
    PGM_LOCK_ASSERT_OWNER(pVM);

    /*
     * We batch up pages that should be freed instead of calling GMM for
     * each and every one of them.
     */
    uint32_t            cPendingPages = 0;
    PGMMFREEPAGESREQ    pReq;
    int rc = GMMR3FreePagesPrepare(pVM, &pReq, PGMPHYS_FREE_PAGE_BATCH_SIZE, GMMACCOUNT_BASE);
    AssertLogRelRCReturn(rc, rc);

    /*
     * Walk the ram ranges.
     */
    for (PPGMRAMRANGE pRam = pVM->pgm.s.pRamRangesXR3; pRam; pRam = pRam->pNextR3)
    {
        uint32_t iPage = pRam->cb >> GUEST_PAGE_SHIFT;
        AssertMsg(((RTGCPHYS)iPage << GUEST_PAGE_SHIFT) == pRam->cb, ("%RGp %RGp\n", (RTGCPHYS)iPage << GUEST_PAGE_SHIFT, pRam->cb));

        if (   !pVM->pgm.s.fRamPreAlloc
#ifdef VBOX_WITH_PGM_NEM_MODE
            && !pVM->pgm.s.fNemMode
#endif
            && pVM->pgm.s.fZeroRamPagesOnReset)
        {
            /* Replace all RAM pages by ZERO pages. */
            while (iPage-- > 0)
            {
                PPGMPAGE pPage = &pRam->aPages[iPage];
                switch (PGM_PAGE_GET_TYPE(pPage))
                {
                    case PGMPAGETYPE_RAM:
                        /* Do not replace pages part of a 2 MB continuous range
                           with zero pages, but zero them instead. */
                        if (   PGM_PAGE_GET_PDE_TYPE(pPage) == PGM_PAGE_PDE_TYPE_PDE
                            || PGM_PAGE_GET_PDE_TYPE(pPage) == PGM_PAGE_PDE_TYPE_PDE_DISABLED)
                        {
                            void *pvPage;
                            rc = pgmPhysPageMap(pVM, pPage, pRam->GCPhys + ((RTGCPHYS)iPage << GUEST_PAGE_SHIFT), &pvPage);
                            AssertLogRelRCReturn(rc, rc);
                            RT_BZERO(pvPage, GUEST_PAGE_SIZE);
                        }
                        else if (PGM_PAGE_IS_BALLOONED(pPage))
                        {
                            /* Turn into a zero page; the balloon status is lost when the VM reboots. */
                            PGM_PAGE_SET_STATE(pVM, pPage, PGM_PAGE_STATE_ZERO);
                        }
                        else if (!PGM_PAGE_IS_ZERO(pPage))
                        {
                            rc = pgmPhysFreePage(pVM, pReq, &cPendingPages, pPage,
                                                 pRam->GCPhys + ((RTGCPHYS)iPage << GUEST_PAGE_SHIFT), PGMPAGETYPE_RAM);
                            AssertLogRelRCReturn(rc, rc);
                        }
                        break;

                    case PGMPAGETYPE_MMIO2_ALIAS_MMIO:
                    case PGMPAGETYPE_SPECIAL_ALIAS_MMIO: /** @todo perhaps leave the special page alone?  I don't think VT-x copes with this code. */
                        pgmHandlerPhysicalResetAliasedPage(pVM, pPage, pRam->GCPhys + ((RTGCPHYS)iPage << GUEST_PAGE_SHIFT),
                                                           pRam, true /*fDoAccounting*/, false /*fFlushIemTlbs*/);
                        break;

                    case PGMPAGETYPE_MMIO2:
                    case PGMPAGETYPE_ROM_SHADOW: /* handled by pgmR3PhysRomReset. */
                    case PGMPAGETYPE_ROM:
                    case PGMPAGETYPE_MMIO:
                        break;
                    default:
                        AssertFailed();
                }
            } /* for each page */
        }
        else
        {
            /* Zero the memory. */
            while (iPage-- > 0)
            {
                PPGMPAGE pPage = &pRam->aPages[iPage];
                switch (PGM_PAGE_GET_TYPE(pPage))
                {
                    case PGMPAGETYPE_RAM:
                        switch (PGM_PAGE_GET_STATE(pPage))
                        {
                            case PGM_PAGE_STATE_ZERO:
                                break;

                            case PGM_PAGE_STATE_BALLOONED:
                                /* Turn into a zero page; the balloon status is lost when the VM reboots. */
                                PGM_PAGE_SET_STATE(pVM, pPage, PGM_PAGE_STATE_ZERO);
                                break;

                            case PGM_PAGE_STATE_SHARED:
                            case PGM_PAGE_STATE_WRITE_MONITORED:
                                rc = pgmPhysPageMakeWritable(pVM, pPage, pRam->GCPhys + ((RTGCPHYS)iPage << GUEST_PAGE_SHIFT));
                                AssertLogRelRCReturn(rc, rc);
                                RT_FALL_THRU();

                            case PGM_PAGE_STATE_ALLOCATED:
                                if (pVM->pgm.s.fZeroRamPagesOnReset)
                                {
                                    void *pvPage;
                                    rc = pgmPhysPageMap(pVM, pPage, pRam->GCPhys + ((RTGCPHYS)iPage << GUEST_PAGE_SHIFT), &pvPage);
                                    AssertLogRelRCReturn(rc, rc);
                                    RT_BZERO(pvPage, GUEST_PAGE_SIZE);
                                }
                                break;
                        }
                        break;

                    case PGMPAGETYPE_MMIO2_ALIAS_MMIO:
                    case PGMPAGETYPE_SPECIAL_ALIAS_MMIO: /** @todo perhaps leave the special page alone?  I don't think VT-x copes with this code. */
                        pgmHandlerPhysicalResetAliasedPage(pVM, pPage, pRam->GCPhys + ((RTGCPHYS)iPage << GUEST_PAGE_SHIFT),
                                                           pRam, true /*fDoAccounting*/, false /*fFlushIemTlbs*/);
                        break;

                    case PGMPAGETYPE_MMIO2:
                    case PGMPAGETYPE_ROM_SHADOW:
                    case PGMPAGETYPE_ROM:
                    case PGMPAGETYPE_MMIO:
                        break;
                    default:
                        AssertFailed();

                }
            } /* for each page */
        }

    }

    /*
     * Finish off any pages pending freeing.
     */
    if (cPendingPages)
    {
        rc = GMMR3FreePagesPerform(pVM, pReq, cPendingPages);
        AssertLogRelRCReturn(rc, rc);
    }
    GMMR3FreePagesCleanup(pReq);

    /*
     * Flush the IEM TLB, just to be sure it really is done.
     */
    IEMTlbInvalidateAllPhysicalAllCpus(pVM, NIL_VMCPUID);

    return VINF_SUCCESS;
}


/**
 * Frees all RAM during VM termination
 *
 * ASSUMES that the caller owns the PGM lock.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 */
int pgmR3PhysRamTerm(PVM pVM)
{
    PGM_LOCK_ASSERT_OWNER(pVM);

    /* Reset the memory balloon. */
    int rc = GMMR3BalloonedPages(pVM, GMMBALLOONACTION_RESET, 0);
    AssertRC(rc);

#ifdef VBOX_WITH_PAGE_SHARING
    /*
     * Clear all registered shared modules.
     */
    pgmR3PhysAssertSharedPageChecksums(pVM);
    rc = GMMR3ResetSharedModules(pVM);
    AssertRC(rc);

    /*
     * Flush the handy pages updates to make sure no shared pages are hiding
     * in there.  (Not unlikely if the VM shuts down, apparently.)
     */
# ifdef VBOX_WITH_PGM_NEM_MODE
    if (!pVM->pgm.s.fNemMode)
# endif
        rc = VMMR3CallR0(pVM, VMMR0_DO_PGM_FLUSH_HANDY_PAGES, 0, NULL);
#endif

    /*
     * We batch up pages that should be freed instead of calling GMM for
     * each and every one of them.
     */
    uint32_t            cPendingPages = 0;
    PGMMFREEPAGESREQ    pReq;
    rc = GMMR3FreePagesPrepare(pVM, &pReq, PGMPHYS_FREE_PAGE_BATCH_SIZE, GMMACCOUNT_BASE);
    AssertLogRelRCReturn(rc, rc);

    /*
     * Walk the ram ranges.
     */
    for (PPGMRAMRANGE pRam = pVM->pgm.s.pRamRangesXR3; pRam; pRam = pRam->pNextR3)
    {
        uint32_t iPage = pRam->cb >> GUEST_PAGE_SHIFT;
        AssertMsg(((RTGCPHYS)iPage << GUEST_PAGE_SHIFT) == pRam->cb, ("%RGp %RGp\n", (RTGCPHYS)iPage << GUEST_PAGE_SHIFT, pRam->cb));

        while (iPage-- > 0)
        {
            PPGMPAGE pPage = &pRam->aPages[iPage];
            switch (PGM_PAGE_GET_TYPE(pPage))
            {
                case PGMPAGETYPE_RAM:
                    /* Free all shared pages. Private pages are automatically freed during GMM VM cleanup. */
                    /** @todo change this to explicitly free private pages here. */
                    if (PGM_PAGE_IS_SHARED(pPage))
                    {
                        rc = pgmPhysFreePage(pVM, pReq, &cPendingPages, pPage,
                                             pRam->GCPhys + ((RTGCPHYS)iPage << GUEST_PAGE_SHIFT), PGMPAGETYPE_RAM);
                        AssertLogRelRCReturn(rc, rc);
                    }
                    break;

                case PGMPAGETYPE_MMIO2_ALIAS_MMIO:
                case PGMPAGETYPE_SPECIAL_ALIAS_MMIO:
                case PGMPAGETYPE_MMIO2:
                case PGMPAGETYPE_ROM_SHADOW: /* handled by pgmR3PhysRomReset. */
                case PGMPAGETYPE_ROM:
                case PGMPAGETYPE_MMIO:
                    break;
                default:
                    AssertFailed();
            }
        } /* for each page */
    }

    /*
     * Finish off any pages pending freeing.
     */
    if (cPendingPages)
    {
        rc = GMMR3FreePagesPerform(pVM, pReq, cPendingPages);
        AssertLogRelRCReturn(rc, rc);
    }
    GMMR3FreePagesCleanup(pReq);
    return VINF_SUCCESS;
}



/*********************************************************************************************************************************
*   MMIO                                                                                                                         *
*********************************************************************************************************************************/

/**
 * This is the interface IOM is using to register an MMIO region.
 *
 * It will check for conflicts and ensure that a RAM range structure
 * is present before calling the PGMR3HandlerPhysicalRegister API to
 * register the callbacks.
 *
 * @returns VBox status code.
 *
 * @param   pVM             The cross context VM structure.
 * @param   GCPhys          The start of the MMIO region.
 * @param   cb              The size of the MMIO region.
 * @param   hType           The physical access handler type registration.
 * @param   uUser           The user argument.
 * @param   pszDesc         The description of the MMIO region.
 */
VMMR3DECL(int) PGMR3PhysMMIORegister(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, PGMPHYSHANDLERTYPE hType,
                                     uint64_t uUser, const char *pszDesc)
{
    /*
     * Assert on some assumption.
     */
    VM_ASSERT_EMT(pVM);
    AssertReturn(!(cb & GUEST_PAGE_OFFSET_MASK), VERR_INVALID_PARAMETER);
    AssertReturn(!(GCPhys & GUEST_PAGE_OFFSET_MASK), VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszDesc, VERR_INVALID_POINTER);
    AssertReturn(*pszDesc, VERR_INVALID_PARAMETER);
#ifdef VBOX_STRICT
    PCPGMPHYSHANDLERTYPEINT pType = pgmHandlerPhysicalTypeHandleToPtr(pVM, hType);
    Assert(pType);
    Assert(pType->enmKind == PGMPHYSHANDLERKIND_MMIO);
#endif

    int rc = PGM_LOCK(pVM);
    AssertRCReturn(rc, rc);

    /*
     * Make sure there's a RAM range structure for the region.
     */
    RTGCPHYS GCPhysLast = GCPhys + (cb - 1);
    bool fRamExists = false;
    PPGMRAMRANGE pRamPrev = NULL;
    PPGMRAMRANGE pRam = pVM->pgm.s.pRamRangesXR3;
    while (pRam && GCPhysLast >= pRam->GCPhys)
    {
        if (    GCPhysLast >= pRam->GCPhys
            &&  GCPhys     <= pRam->GCPhysLast)
        {
            /* Simplification: all within the same range. */
            AssertLogRelMsgReturnStmt(   GCPhys     >= pRam->GCPhys
                                      && GCPhysLast <= pRam->GCPhysLast,
                                      ("%RGp-%RGp (MMIO/%s) falls partly outside %RGp-%RGp (%s)\n",
                                       GCPhys, GCPhysLast, pszDesc,
                                       pRam->GCPhys, pRam->GCPhysLast, pRam->pszDesc),
                                      PGM_UNLOCK(pVM),
                                      VERR_PGM_RAM_CONFLICT);

            /* Check that it's all RAM or MMIO pages. */
            PCPGMPAGE pPage = &pRam->aPages[(GCPhys - pRam->GCPhys) >> GUEST_PAGE_SHIFT];
            uint32_t  cLeft = cb >> GUEST_PAGE_SHIFT;
            while (cLeft-- > 0)
            {
                AssertLogRelMsgReturnStmt(   PGM_PAGE_GET_TYPE(pPage) == PGMPAGETYPE_RAM
                                          || PGM_PAGE_GET_TYPE(pPage) == PGMPAGETYPE_MMIO,
                                          ("%RGp-%RGp (MMIO/%s): %RGp is not a RAM or MMIO page - type=%d desc=%s\n",
                                           GCPhys, GCPhysLast, pszDesc, pRam->GCPhys, PGM_PAGE_GET_TYPE(pPage), pRam->pszDesc),
                                          PGM_UNLOCK(pVM),
                                          VERR_PGM_RAM_CONFLICT);
                pPage++;
            }

            /* Looks good. */
            fRamExists = true;
            break;
        }

        /* next */
        pRamPrev = pRam;
        pRam = pRam->pNextR3;
    }
    PPGMRAMRANGE pNew;
    if (fRamExists)
    {
        pNew = NULL;

        /*
         * Make all the pages in the range MMIO/ZERO pages, freeing any
         * RAM pages currently mapped here. This might not be 100% correct
         * for PCI memory, but we're doing the same thing for MMIO2 pages.
         */
        rc = pgmR3PhysFreePageRange(pVM, pRam, GCPhys, GCPhysLast, NULL);
        AssertRCReturnStmt(rc, PGM_UNLOCK(pVM), rc);

        /* Force a PGM pool flush as guest ram references have been changed. */
        /** @todo not entirely SMP safe; assuming for now the guest takes
         *   care of this internally (not touch mapped mmio while changing the
         *   mapping). */
        PVMCPU pVCpu = VMMGetCpu(pVM);
        pVCpu->pgm.s.fSyncFlags |= PGM_SYNC_CLEAR_PGM_POOL;
        VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3);
    }
    else
    {
        /*
         * No RAM range, insert an ad hoc one.
         *
         * Note that we don't have to tell REM about this range because
         * PGMHandlerPhysicalRegisterEx will do that for us.
         */
        Log(("PGMR3PhysMMIORegister: Adding ad hoc MMIO range for %RGp-%RGp %s\n", GCPhys, GCPhysLast, pszDesc));

        /* Alloc. */
        const uint32_t cPages      = cb >> GUEST_PAGE_SHIFT;
        const size_t   cbRamRange  = RT_UOFFSETOF_DYN(PGMRAMRANGE, aPages[cPages]);
        const size_t   cRangePages = RT_ALIGN_Z(cbRamRange, HOST_PAGE_SIZE) >> HOST_PAGE_SHIFT;
        RTR0PTR        pNewR0      = NIL_RTR0PTR;
        rc = SUPR3PageAllocEx(cRangePages, 0 /*fFlags*/, (void **)&pNew, &pNewR0, NULL /*paPages*/);
        AssertLogRelMsgRCReturnStmt(rc, ("cbRamRange=%zu\n", cbRamRange), PGM_UNLOCK(pVM), rc);

#ifdef VBOX_WITH_NATIVE_NEM
        /* Notify NEM. */
        uint8_t u2State = 0; /* (must have valid state as there can't be anything to preserve) */
        if (VM_IS_NEM_ENABLED(pVM))
        {
            rc = NEMR3NotifyPhysMmioExMapEarly(pVM, GCPhys, cPages << GUEST_PAGE_SHIFT, 0 /*fFlags*/, NULL, NULL,
                                               &u2State, &pNew->uNemRange);
            AssertLogRelRCReturnStmt(rc, SUPR3PageFreeEx(pNew, cRangePages), rc);
        }
#endif

        /* Initialize the range. */
        pNew->pSelfR0       = pNewR0;
        pNew->GCPhys        = GCPhys;
        pNew->GCPhysLast    = GCPhysLast;
        pNew->cb            = cb;
        pNew->pszDesc       = pszDesc;
        pNew->fFlags        = PGM_RAM_RANGE_FLAGS_AD_HOC_MMIO;
        pNew->pvR3          = NULL;
        pNew->paLSPages     = NULL;

        uint32_t iPage = cPages;
        while (iPage-- > 0)
        {
            PGM_PAGE_INIT_ZERO(&pNew->aPages[iPage], pVM, PGMPAGETYPE_MMIO);
#ifdef VBOX_WITH_NATIVE_NEM
            PGM_PAGE_SET_NEM_STATE(&pNew->aPages[iPage], u2State);
#endif
        }
        Assert(PGM_PAGE_GET_TYPE(&pNew->aPages[0]) == PGMPAGETYPE_MMIO);

        /* update the page count stats. */
        pVM->pgm.s.cPureMmioPages += cPages;
        pVM->pgm.s.cAllPages      += cPages;

        /* link it */
        pgmR3PhysLinkRamRange(pVM, pNew, pRamPrev);
    }

    /*
     * Register the access handler.
     */
    rc = PGMHandlerPhysicalRegister(pVM, GCPhys, GCPhysLast, hType, uUser, pszDesc);
    if (RT_SUCCESS(rc))
    {
#ifdef VBOX_WITH_NATIVE_NEM
        /* Late NEM notification. */
        if (VM_IS_NEM_ENABLED(pVM))
        {
            uint32_t const fNemNotify = (fRamExists ? NEM_NOTIFY_PHYS_MMIO_EX_F_REPLACE : 0);
            rc = NEMR3NotifyPhysMmioExMapLate(pVM, GCPhys, GCPhysLast - GCPhys + 1, fNemNotify,
                                              fRamExists ? (uint8_t *)pRam->pvR3 + (uintptr_t)(GCPhys - pRam->GCPhys) : NULL,
                                              NULL, !fRamExists ? &pRam->uNemRange : NULL);
            AssertLogRelRCReturn(rc, rc);
        }
#endif
    }
    /** @todo the phys handler failure handling isn't complete, esp. wrt NEM. */
    else if (!fRamExists)
    {
        pVM->pgm.s.cPureMmioPages -= cb >> GUEST_PAGE_SHIFT;
        pVM->pgm.s.cAllPages      -= cb >> GUEST_PAGE_SHIFT;

        /* remove the ad hoc range. */
        pgmR3PhysUnlinkRamRange2(pVM, pNew, pRamPrev);
        pNew->cb = pNew->GCPhys = pNew->GCPhysLast = NIL_RTGCPHYS;
        SUPR3PageFreeEx(pRam, RT_ALIGN_Z(RT_UOFFSETOF_DYN(PGMRAMRANGE, aPages[cb >> GUEST_PAGE_SHIFT]),
                                         HOST_PAGE_SIZE) >> HOST_PAGE_SHIFT);
    }
    pgmPhysInvalidatePageMapTLB(pVM);

    PGM_UNLOCK(pVM);
    return rc;
}


/**
 * This is the interface IOM is using to register an MMIO region.
 *
 * It will take care of calling PGMHandlerPhysicalDeregister and clean up
 * any ad hoc PGMRAMRANGE left behind.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   GCPhys          The start of the MMIO region.
 * @param   cb              The size of the MMIO region.
 */
VMMR3DECL(int) PGMR3PhysMMIODeregister(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb)
{
    VM_ASSERT_EMT(pVM);

    int rc = PGM_LOCK(pVM);
    AssertRCReturn(rc, rc);

    /*
     * First deregister the handler, then check if we should remove the ram range.
     */
    rc = PGMHandlerPhysicalDeregister(pVM, GCPhys);
    if (RT_SUCCESS(rc))
    {
        RTGCPHYS        GCPhysLast  = GCPhys + (cb - 1);
        PPGMRAMRANGE    pRamPrev    = NULL;
        PPGMRAMRANGE    pRam        = pVM->pgm.s.pRamRangesXR3;
        while (pRam && GCPhysLast >= pRam->GCPhys)
        {
            /** @todo We're being a bit too careful here. rewrite. */
            if (    GCPhysLast == pRam->GCPhysLast
                &&  GCPhys     == pRam->GCPhys)
            {
                Assert(pRam->cb == cb);

                /*
                 * See if all the pages are dead MMIO pages.
                 */
                uint32_t const  cGuestPages = cb >> GUEST_PAGE_SHIFT;
                bool            fAllMMIO    = true;
                uint32_t        iPage       = 0;
                uint32_t        cLeft       = cGuestPages;
                while (cLeft-- > 0)
                {
                    PPGMPAGE    pPage       = &pRam->aPages[iPage];
                    if (   !PGM_PAGE_IS_MMIO_OR_ALIAS(pPage)
                        /*|| not-out-of-action later */)
                    {
                        fAllMMIO = false;
                        AssertMsgFailed(("%RGp %R[pgmpage]\n", pRam->GCPhys + ((RTGCPHYS)iPage << GUEST_PAGE_SHIFT), pPage));
                        break;
                    }
                    Assert(   PGM_PAGE_IS_ZERO(pPage)
                           || PGM_PAGE_GET_TYPE(pPage) == PGMPAGETYPE_MMIO2_ALIAS_MMIO
                           || PGM_PAGE_GET_TYPE(pPage) == PGMPAGETYPE_SPECIAL_ALIAS_MMIO);
                    pPage++;
                }
                if (fAllMMIO)
                {
                    /*
                     * Ad-hoc range, unlink and free it.
                     */
                    Log(("PGMR3PhysMMIODeregister: Freeing ad hoc MMIO range for %RGp-%RGp %s\n",
                         GCPhys, GCPhysLast, pRam->pszDesc));
                    /** @todo check the ad-hoc flags? */

#ifdef VBOX_WITH_NATIVE_NEM
                    if (VM_IS_NEM_ENABLED(pVM)) /* Notify REM before we unlink the range. */
                    {
                        rc = NEMR3NotifyPhysMmioExUnmap(pVM, GCPhys, GCPhysLast - GCPhys + 1, 0 /*fFlags*/,
                                                        NULL, NULL, NULL, &pRam->uNemRange);
                        AssertLogRelRCReturn(rc, rc);
                    }
#endif

                    pVM->pgm.s.cAllPages      -= cGuestPages;
                    pVM->pgm.s.cPureMmioPages -= cGuestPages;

                    pgmR3PhysUnlinkRamRange2(pVM, pRam, pRamPrev);
                    const uint32_t cPages      = pRam->cb >> GUEST_PAGE_SHIFT;
                    const size_t   cbRamRange  = RT_UOFFSETOF_DYN(PGMRAMRANGE, aPages[cPages]);
                    pRam->cb = pRam->GCPhys = pRam->GCPhysLast = NIL_RTGCPHYS;
                    SUPR3PageFreeEx(pRam, RT_ALIGN_Z(cbRamRange, HOST_PAGE_SIZE) >> HOST_PAGE_SHIFT);
                    break;
                }
            }

            /*
             * Range match? It will all be within one range (see PGMAllHandler.cpp).
             */
            if (    GCPhysLast >= pRam->GCPhys
                &&  GCPhys     <= pRam->GCPhysLast)
            {
                Assert(GCPhys     >= pRam->GCPhys);
                Assert(GCPhysLast <= pRam->GCPhysLast);

                /*
                 * Turn the pages back into RAM pages.
                 */
                uint32_t iPage = (GCPhys - pRam->GCPhys) >> GUEST_PAGE_SHIFT;
                uint32_t cLeft = cb >> GUEST_PAGE_SHIFT;
                while (cLeft--)
                {
                    PPGMPAGE pPage = &pRam->aPages[iPage];
                    AssertMsg(   (PGM_PAGE_IS_MMIO(pPage) && PGM_PAGE_IS_ZERO(pPage))
                              || PGM_PAGE_GET_TYPE(pPage) == PGMPAGETYPE_MMIO2_ALIAS_MMIO
                              || PGM_PAGE_GET_TYPE(pPage) == PGMPAGETYPE_SPECIAL_ALIAS_MMIO,
                              ("%RGp %R[pgmpage]\n", pRam->GCPhys + ((RTGCPHYS)iPage << GUEST_PAGE_SHIFT), pPage));
                    if (PGM_PAGE_IS_MMIO_OR_ALIAS(pPage))
                        PGM_PAGE_SET_TYPE(pVM, pPage, PGMPAGETYPE_RAM);
                    iPage++;
                }

#ifdef VBOX_WITH_NATIVE_NEM
                /* Notify REM (failure will probably leave things in a non-working state). */
                if (VM_IS_NEM_ENABLED(pVM))
                {
                    uint8_t u2State = UINT8_MAX;
                    rc = NEMR3NotifyPhysMmioExUnmap(pVM, GCPhys, GCPhysLast - GCPhys + 1, NEM_NOTIFY_PHYS_MMIO_EX_F_REPLACE,
                                                    pRam->pvR3 ? (uint8_t *)pRam->pvR3 + GCPhys - pRam->GCPhys : NULL,
                                                    NULL, &u2State, &pRam->uNemRange);
                    AssertLogRelRCReturn(rc, rc);
                    if (u2State != UINT8_MAX)
                        pgmPhysSetNemStateForPages(&pRam->aPages[(GCPhys - pRam->GCPhys) >> GUEST_PAGE_SHIFT],
                                                   cb >> GUEST_PAGE_SHIFT, u2State);
                }
#endif
                break;
            }

            /* next */
            pRamPrev = pRam;
            pRam = pRam->pNextR3;
        }
    }

    /* Force a PGM pool flush as guest ram references have been changed. */
    /** @todo Not entirely SMP safe; assuming for now the guest takes care of
     *       this internally (not touch mapped mmio while changing the mapping). */
    PVMCPU pVCpu = VMMGetCpu(pVM);
    pVCpu->pgm.s.fSyncFlags |= PGM_SYNC_CLEAR_PGM_POOL;
    VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3);

    pgmPhysInvalidatePageMapTLB(pVM);
    pgmPhysInvalidRamRangeTlbs(pVM);
    PGM_UNLOCK(pVM);
    return rc;
}



/*********************************************************************************************************************************
*   MMIO2                                                                                                                        *
*********************************************************************************************************************************/

/**
 * Locate a MMIO2 range.
 *
 * @returns Pointer to the MMIO2 range.
 * @param   pVM             The cross context VM structure.
 * @param   pDevIns         The device instance owning the region.
 * @param   iSubDev         The sub-device number.
 * @param   iRegion         The region.
 * @param   hMmio2          Handle to look up.  If NIL, use the @a iSubDev and
 *                          @a iRegion.
 */
DECLINLINE(PPGMREGMMIO2RANGE) pgmR3PhysMmio2Find(PVM pVM, PPDMDEVINS pDevIns, uint32_t iSubDev,
                                                 uint32_t iRegion, PGMMMIO2HANDLE hMmio2)
{
    if (hMmio2 != NIL_PGMMMIO2HANDLE)
    {
        if (hMmio2 <= RT_ELEMENTS(pVM->pgm.s.apMmio2RangesR3) && hMmio2 != 0)
        {
            PPGMREGMMIO2RANGE pCur = pVM->pgm.s.apMmio2RangesR3[hMmio2 - 1];
            if (pCur && pCur->pDevInsR3 == pDevIns)
            {
                Assert(pCur->idMmio2 == hMmio2);
                AssertReturn(pCur->fFlags & PGMREGMMIO2RANGE_F_FIRST_CHUNK, NULL);
                return pCur;
            }
            Assert(!pCur);
        }
        for (PPGMREGMMIO2RANGE pCur = pVM->pgm.s.pRegMmioRangesR3; pCur; pCur = pCur->pNextR3)
            if (pCur->idMmio2 == hMmio2)
            {
                AssertBreak(pCur->pDevInsR3 == pDevIns);
                AssertReturn(pCur->fFlags & PGMREGMMIO2RANGE_F_FIRST_CHUNK, NULL);
                return pCur;
            }
    }
    else
    {
        /*
         * Search the list.  There shouldn't be many entries.
         */
        /** @todo Optimize this lookup! There may now be many entries and it'll
         *        become really slow when doing MMR3HyperMapMMIO2 and similar. */
        for (PPGMREGMMIO2RANGE pCur = pVM->pgm.s.pRegMmioRangesR3; pCur; pCur = pCur->pNextR3)
            if (   pCur->pDevInsR3 == pDevIns
                && pCur->iRegion == iRegion
                && pCur->iSubDev == iSubDev)
                return pCur;
    }
    return NULL;
}


/**
 * Worker for PGMR3PhysMmio2ControlDirtyPageTracking and PGMR3PhysMmio2Map.
 */
static int pgmR3PhysMmio2EnableDirtyPageTracing(PVM pVM, PPGMREGMMIO2RANGE pFirstMmio2)
{
    int rc = VINF_SUCCESS;
    for (PPGMREGMMIO2RANGE pCurMmio2 = pFirstMmio2; pCurMmio2; pCurMmio2 = pCurMmio2->pNextR3)
    {
        Assert(!(pCurMmio2->fFlags & PGMREGMMIO2RANGE_F_IS_TRACKING));
        int rc2 = pgmHandlerPhysicalExRegister(pVM, pCurMmio2->pPhysHandlerR3, pCurMmio2->RamRange.GCPhys,
                                               pCurMmio2->RamRange.GCPhysLast);
        AssertLogRelMsgRC(rc2, ("%#RGp-%#RGp %s failed -> %Rrc\n", pCurMmio2->RamRange.GCPhys, pCurMmio2->RamRange.GCPhysLast,
                                pCurMmio2->RamRange.pszDesc, rc2));
        if (RT_SUCCESS(rc2))
            pCurMmio2->fFlags |= PGMREGMMIO2RANGE_F_IS_TRACKING;
        else if (RT_SUCCESS(rc))
            rc = rc2;
        if (pCurMmio2->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK)
            return rc;
    }
    AssertFailed();
    return rc;
}


/**
 * Worker for PGMR3PhysMmio2ControlDirtyPageTracking and PGMR3PhysMmio2Unmap.
 */
static int pgmR3PhysMmio2DisableDirtyPageTracing(PVM pVM, PPGMREGMMIO2RANGE pFirstMmio2)
{
    for (PPGMREGMMIO2RANGE pCurMmio2 = pFirstMmio2; pCurMmio2; pCurMmio2 = pCurMmio2->pNextR3)
    {
        if (pCurMmio2->fFlags & PGMREGMMIO2RANGE_F_IS_TRACKING)
        {
            int rc2 = pgmHandlerPhysicalExDeregister(pVM, pCurMmio2->pPhysHandlerR3);
            AssertLogRelMsgRC(rc2, ("%#RGp-%#RGp %s failed -> %Rrc\n", pCurMmio2->RamRange.GCPhys, pCurMmio2->RamRange.GCPhysLast,
                                    pCurMmio2->RamRange.pszDesc, rc2));
            pCurMmio2->fFlags &= ~PGMREGMMIO2RANGE_F_IS_TRACKING;
        }
        if (pCurMmio2->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK)
            return VINF_SUCCESS;
    }
    AssertFailed();
    return VINF_SUCCESS;

}


/**
 * Calculates the number of chunks
 *
 * @returns Number of registration chunk needed.
 * @param   pVM             The cross context VM structure.
 * @param   cb              The size of the MMIO/MMIO2 range.
 * @param   pcPagesPerChunk Where to return the number of pages tracked by each
 *                          chunk.  Optional.
 * @param   pcbChunk        Where to return the guest mapping size for a chunk.
 */
static uint16_t pgmR3PhysMmio2CalcChunkCount(PVM pVM, RTGCPHYS cb, uint32_t *pcPagesPerChunk, uint32_t *pcbChunk)
{
    RT_NOREF_PV(pVM); /* without raw mode */

    /*
     * This is the same calculation as PGMR3PhysRegisterRam does, except we'll be
     * needing a few bytes extra the PGMREGMMIO2RANGE structure.
     *
     * Note! In additions, we've got a 24 bit sub-page range for MMIO2 ranges, leaving
     *       us with an absolute maximum of 16777215 pages per chunk (close to 64 GB).
     */
    uint32_t const cPagesPerChunk = _4M;
    Assert(RT_ALIGN_32(cPagesPerChunk, X86_PD_PAE_SHIFT - X86_PAGE_SHIFT)); /* NEM large page requirement: 1GB pages. */
    uint32_t const cbChunk       = RT_UOFFSETOF_DYN(PGMREGMMIO2RANGE, RamRange.aPages[cPagesPerChunk]);
    AssertRelease(cPagesPerChunk < _16M);

    if (pcbChunk)
        *pcbChunk = cbChunk;
    if (pcPagesPerChunk)
        *pcPagesPerChunk = cPagesPerChunk;

    /* Calc the number of chunks we need. */
    RTGCPHYS const cGuestPages = cb >> GUEST_PAGE_SHIFT;
    uint16_t cChunks = (uint16_t)((cGuestPages + cPagesPerChunk - 1) / cPagesPerChunk);
    AssertRelease((RTGCPHYS)cChunks * cPagesPerChunk >= cGuestPages);
    return cChunks;
}


/**
 * Worker for PGMR3PhysMMIO2Register that allocates and the PGMREGMMIO2RANGE
 * structures and does basic initialization.
 *
 * Caller must set type specfic members and initialize the PGMPAGE structures.
 *
 * This was previously also used by PGMR3PhysMmio2PreRegister, a function for
 * pre-registering MMIO that was later (6.1) replaced by a new handle based IOM
 * interface.  The reference to caller and type above is purely historical.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pDevIns         The device instance owning the region.
 * @param   iSubDev         The sub-device number (internal PCI config number).
 * @param   iRegion         The region number.  If the MMIO2 memory is a PCI
 *                          I/O region this number has to be the number of that
 *                          region. Otherwise it can be any number safe
 *                          UINT8_MAX.
 * @param   cb              The size of the region.  Must be page aligned.
 * @param   fFlags          PGMPHYS_MMIO2_FLAGS_XXX.
 * @param   idMmio2         The MMIO2 ID for the first chunk.
 * @param   pszDesc         The description.
 * @param   ppHeadRet       Where to return the pointer to the first
 *                          registration chunk.
 *
 * @thread  EMT
 */
static int pgmR3PhysMmio2Create(PVM pVM, PPDMDEVINS pDevIns, uint32_t iSubDev, uint32_t iRegion, RTGCPHYS cb, uint32_t fFlags,
                                uint8_t idMmio2, const char *pszDesc, PPGMREGMMIO2RANGE *ppHeadRet)
{
    /*
     * Figure out how many chunks we need and of which size.
     */
    uint32_t cPagesPerChunk;
    uint16_t cChunks = pgmR3PhysMmio2CalcChunkCount(pVM, cb, &cPagesPerChunk, NULL);
    AssertReturn(cChunks, VERR_PGM_PHYS_MMIO_EX_IPE);

    /*
     * Allocate the chunks.
     */
    PPGMREGMMIO2RANGE *ppNext = ppHeadRet;
    *ppNext = NULL;

    int rc = VINF_SUCCESS;
    uint32_t cPagesLeft = cb >> GUEST_PAGE_SHIFT;
    for (uint16_t iChunk = 0; iChunk < cChunks && RT_SUCCESS(rc); iChunk++, idMmio2++)
    {
        /*
         * We currently do a single RAM range for the whole thing.  This will
         * probably have to change once someone needs really large MMIO regions,
         * as we will be running into SUPR3PageAllocEx limitations and such.
         */
        const uint32_t   cPagesTrackedByChunk = RT_MIN(cPagesLeft, cPagesPerChunk);
        const size_t     cbRange = RT_UOFFSETOF_DYN(PGMREGMMIO2RANGE, RamRange.aPages[cPagesTrackedByChunk]);
        PPGMREGMMIO2RANGE pNew    = NULL;

        /*
         * Allocate memory for the registration structure.
         */
        size_t const cChunkPages  = RT_ALIGN_Z(cbRange, HOST_PAGE_SIZE) >> HOST_PAGE_SHIFT;
        size_t const cbChunk      = (1 + cChunkPages + 1) << HOST_PAGE_SHIFT;
        AssertLogRelBreakStmt(cbChunk == (uint32_t)cbChunk, rc = VERR_OUT_OF_RANGE);
        RTR0PTR      R0PtrChunk   = NIL_RTR0PTR;
        void        *pvChunk      = NULL;
        rc = SUPR3PageAllocEx(cChunkPages, 0 /*fFlags*/, &pvChunk, &R0PtrChunk, NULL /*paPages*/);
        AssertLogRelMsgRCBreak(rc, ("rc=%Rrc, cChunkPages=%#zx\n", rc, cChunkPages));

        Assert(R0PtrChunk != NIL_RTR0PTR || PGM_IS_IN_NEM_MODE(pVM));
        RT_BZERO(pvChunk, cChunkPages << HOST_PAGE_SHIFT);

        pNew = (PPGMREGMMIO2RANGE)pvChunk;
        pNew->RamRange.fFlags   = PGM_RAM_RANGE_FLAGS_FLOATING;
        pNew->RamRange.pSelfR0  = R0PtrChunk + RT_UOFFSETOF(PGMREGMMIO2RANGE, RamRange);

        /*
         * Initialize the registration structure (caller does specific bits).
         */
        pNew->pDevInsR3             = pDevIns;
        //pNew->pvR3                = NULL;
        //pNew->pNext               = NULL;
        if (iChunk == 0)
            pNew->fFlags |= PGMREGMMIO2RANGE_F_FIRST_CHUNK;
        if (iChunk + 1 == cChunks)
            pNew->fFlags |= PGMREGMMIO2RANGE_F_LAST_CHUNK;
        if (fFlags & PGMPHYS_MMIO2_FLAGS_TRACK_DIRTY_PAGES)
            pNew->fFlags |= PGMREGMMIO2RANGE_F_TRACK_DIRTY_PAGES;
        pNew->iSubDev               = iSubDev;
        pNew->iRegion               = iRegion;
        pNew->idSavedState          = UINT8_MAX;
        pNew->idMmio2               = idMmio2;
        //pNew->pPhysHandlerR3      = NULL;
        //pNew->paLSPages           = NULL;
        pNew->RamRange.GCPhys       = NIL_RTGCPHYS;
        pNew->RamRange.GCPhysLast   = NIL_RTGCPHYS;
        pNew->RamRange.pszDesc      = pszDesc;
        pNew->RamRange.cb           = pNew->cbReal = (RTGCPHYS)cPagesTrackedByChunk << X86_PAGE_SHIFT;
        pNew->RamRange.fFlags      |= PGM_RAM_RANGE_FLAGS_AD_HOC_MMIO_EX;
        pNew->RamRange.uNemRange    = UINT32_MAX;
        //pNew->RamRange.pvR3       = NULL;
        //pNew->RamRange.paLSPages  = NULL;

        *ppNext = pNew;
        ASMCompilerBarrier();
        cPagesLeft -= cPagesTrackedByChunk;
        ppNext = &pNew->pNextR3;

        /*
         * Pre-allocate a handler if we're tracking dirty pages, unless NEM takes care of this.
         */
        if (   (fFlags & PGMPHYS_MMIO2_FLAGS_TRACK_DIRTY_PAGES)
#ifdef VBOX_WITH_PGM_NEM_MODE
            && (!VM_IS_NEM_ENABLED(pVM) || !NEMR3IsMmio2DirtyPageTrackingSupported(pVM))
#endif
            )

        {
            rc = pgmHandlerPhysicalExCreate(pVM, pVM->pgm.s.hMmio2DirtyPhysHandlerType, idMmio2, pszDesc, &pNew->pPhysHandlerR3);
            AssertLogRelMsgRCBreak(rc, ("idMmio2=%zu\n", idMmio2));
        }
    }
    Assert(cPagesLeft == 0);

    if (RT_SUCCESS(rc))
    {
        Assert((*ppHeadRet)->fFlags & PGMREGMMIO2RANGE_F_FIRST_CHUNK);
        return VINF_SUCCESS;
    }

    /*
     * Free floating ranges.
     */
    while (*ppHeadRet)
    {
        PPGMREGMMIO2RANGE pFree = *ppHeadRet;
        *ppHeadRet = pFree->pNextR3;

        if (pFree->pPhysHandlerR3)
        {
            pgmHandlerPhysicalExDestroy(pVM, pFree->pPhysHandlerR3);
            pFree->pPhysHandlerR3 = NULL;
        }

        if (pFree->RamRange.fFlags & PGM_RAM_RANGE_FLAGS_FLOATING)
        {
            const size_t    cbRange     = RT_UOFFSETOF_DYN(PGMREGMMIO2RANGE,
                                                           RamRange.aPages[pFree->RamRange.cb >> X86_PAGE_SHIFT]);
            size_t const    cChunkPages = RT_ALIGN_Z(cbRange, HOST_PAGE_SIZE) >> HOST_PAGE_SHIFT;
            SUPR3PageFreeEx(pFree, cChunkPages);
        }
    }

    return rc;
}


/**
 * Common worker PGMR3PhysMmio2PreRegister & PGMR3PhysMMIO2Register that links a
 * complete registration entry into the lists and lookup tables.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pNew            The new MMIO / MMIO2 registration to link.
 */
static void pgmR3PhysMmio2Link(PVM pVM, PPGMREGMMIO2RANGE pNew)
{
    Assert(pNew->idMmio2 != UINT8_MAX);

    /*
     * Link it into the list (order doesn't matter, so insert it at the head).
     *
     * Note! The range we're linking may consist of multiple chunks, so we
     *       have to find the last one.
     */
    PPGMREGMMIO2RANGE pLast = pNew;
    for (pLast = pNew; ; pLast = pLast->pNextR3)
    {
        if (pLast->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK)
            break;
        Assert(pLast->pNextR3);
        Assert(pLast->pNextR3->pDevInsR3 == pNew->pDevInsR3);
        Assert(pLast->pNextR3->iSubDev   == pNew->iSubDev);
        Assert(pLast->pNextR3->iRegion   == pNew->iRegion);
        Assert(pLast->pNextR3->idMmio2   == pLast->idMmio2 + 1);
    }

    PGM_LOCK_VOID(pVM);

    /* Link in the chain of ranges at the head of the list. */
    pLast->pNextR3 = pVM->pgm.s.pRegMmioRangesR3;
    pVM->pgm.s.pRegMmioRangesR3 = pNew;

    /* Insert the MMIO2 range/page IDs. */
    uint8_t idMmio2 = pNew->idMmio2;
    for (;;)
    {
        Assert(pVM->pgm.s.apMmio2RangesR3[idMmio2 - 1] == NULL);
        Assert(pVM->pgm.s.apMmio2RangesR0[idMmio2 - 1] == NIL_RTR0PTR);
        pVM->pgm.s.apMmio2RangesR3[idMmio2 - 1] = pNew;
        pVM->pgm.s.apMmio2RangesR0[idMmio2 - 1] = pNew->RamRange.pSelfR0 - RT_UOFFSETOF(PGMREGMMIO2RANGE, RamRange);
        if (pNew->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK)
            break;
        pNew = pNew->pNextR3;
        idMmio2++;
    }

    pgmPhysInvalidatePageMapTLB(pVM);
    PGM_UNLOCK(pVM);
}


/**
 * Allocate and register an MMIO2 region.
 *
 * As mentioned elsewhere, MMIO2 is just RAM spelled differently.  It's RAM
 * associated with a device. It is also non-shared memory with a permanent
 * ring-3 mapping and page backing (presently).
 *
 * A MMIO2 range may overlap with base memory if a lot of RAM is configured for
 * the VM, in which case we'll drop the base memory pages.  Presently we will
 * make no attempt to preserve anything that happens to be present in the base
 * memory that is replaced, this is of course incorrect but it's too much
 * effort.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success, *ppv pointing to the R3 mapping of the
 *          memory.
 * @retval  VERR_ALREADY_EXISTS if the region already exists.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pDevIns         The device instance owning the region.
 * @param   iSubDev         The sub-device number.
 * @param   iRegion         The region number.  If the MMIO2 memory is a PCI
 *                          I/O region this number has to be the number of that
 *                          region.  Otherwise it can be any number save
 *                          UINT8_MAX.
 * @param   cb              The size of the region.  Must be page aligned.
 * @param   fFlags          Reserved for future use, must be zero.
 * @param   pszDesc         The description.
 * @param   ppv             Where to store the pointer to the ring-3 mapping of
 *                          the memory.
 * @param   phRegion        Where to return the MMIO2 region handle.  Optional.
 * @thread  EMT
 */
VMMR3_INT_DECL(int) PGMR3PhysMmio2Register(PVM pVM, PPDMDEVINS pDevIns, uint32_t iSubDev, uint32_t iRegion, RTGCPHYS cb,
                                           uint32_t fFlags, const char *pszDesc, void **ppv, PGMMMIO2HANDLE *phRegion)
{
    /*
     * Validate input.
     */
    AssertPtrReturn(ppv, VERR_INVALID_POINTER);
    *ppv = NULL;
    if (phRegion)
    {
        AssertPtrReturn(phRegion, VERR_INVALID_POINTER);
        *phRegion = NIL_PGMMMIO2HANDLE;
    }
    VM_ASSERT_EMT_RETURN(pVM, VERR_VM_THREAD_NOT_EMT);
    AssertPtrReturn(pDevIns, VERR_INVALID_PARAMETER);
    AssertReturn(iSubDev <= UINT8_MAX, VERR_INVALID_PARAMETER);
    AssertReturn(iRegion <= UINT8_MAX, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszDesc, VERR_INVALID_POINTER);
    AssertReturn(*pszDesc, VERR_INVALID_PARAMETER);
    AssertReturn(pgmR3PhysMmio2Find(pVM, pDevIns, iSubDev, iRegion, NIL_PGMMMIO2HANDLE) == NULL, VERR_ALREADY_EXISTS);
    AssertReturn(!(cb & GUEST_PAGE_OFFSET_MASK), VERR_INVALID_PARAMETER);
    AssertReturn(cb, VERR_INVALID_PARAMETER);
    AssertReturn(!(fFlags & ~PGMPHYS_MMIO2_FLAGS_VALID_MASK), VERR_INVALID_FLAGS);

    const uint32_t cGuestPages = cb >> GUEST_PAGE_SHIFT;
    AssertLogRelReturn(((RTGCPHYS)cGuestPages << GUEST_PAGE_SHIFT) == cb, VERR_INVALID_PARAMETER);
    AssertLogRelReturn(cGuestPages <= (MM_MMIO_64_MAX >> X86_PAGE_SHIFT), VERR_OUT_OF_RANGE);
    AssertLogRelReturn(cGuestPages <= PGM_MMIO2_MAX_PAGE_COUNT, VERR_OUT_OF_RANGE);

    /*
     * For the 2nd+ instance, mangle the description string so it's unique.
     */
    if (pDevIns->iInstance > 0) /** @todo Move to PDMDevHlp.cpp and use a real string cache. */
    {
        pszDesc = MMR3HeapAPrintf(pVM, MM_TAG_PGM_PHYS, "%s [%u]", pszDesc, pDevIns->iInstance);
        if (!pszDesc)
            return VERR_NO_MEMORY;
    }

    /*
     * Allocate an MMIO2 range ID (not freed on failure).
     *
     * The zero ID is not used as it could be confused with NIL_GMM_PAGEID, so
     * the IDs goes from 1 thru PGM_MMIO2_MAX_RANGES.
     */
    unsigned cChunks = pgmR3PhysMmio2CalcChunkCount(pVM, cb, NULL, NULL);

    PGM_LOCK_VOID(pVM);
    AssertCompile(PGM_MMIO2_MAX_RANGES < 255);
    uint8_t const  idMmio2          = pVM->pgm.s.cMmio2Regions + 1;
    unsigned const cNewMmio2Regions = pVM->pgm.s.cMmio2Regions + cChunks;
    if (cNewMmio2Regions > PGM_MMIO2_MAX_RANGES)
    {
        PGM_UNLOCK(pVM);
        AssertLogRelFailedReturn(VERR_PGM_TOO_MANY_MMIO2_RANGES);
    }
    pVM->pgm.s.cMmio2Regions = cNewMmio2Regions;
    PGM_UNLOCK(pVM);

    /*
     * Try reserve and allocate the backing memory first as this is what is
     * most likely to fail.
     */
    int rc = MMR3AdjustFixedReservation(pVM, cGuestPages, pszDesc);
    if (RT_SUCCESS(rc))
    {
        const uint32_t cHostPages = RT_ALIGN_T(cb, HOST_PAGE_SIZE, RTGCPHYS) >> HOST_PAGE_SHIFT;
        PSUPPAGE paPages = (PSUPPAGE)RTMemTmpAlloc(cHostPages * sizeof(SUPPAGE));
        if (RT_SUCCESS(rc))
        {
            void   *pvPages   = NULL;
#ifndef VBOX_WITH_LINEAR_HOST_PHYS_MEM
            RTR0PTR pvPagesR0 = NIL_RTR0PTR;
#endif
#ifdef VBOX_WITH_PGM_NEM_MODE
            if (PGM_IS_IN_NEM_MODE(pVM))
                rc = SUPR3PageAlloc(cHostPages, pVM->pgm.s.fUseLargePages ? SUP_PAGE_ALLOC_F_LARGE_PAGES : 0, &pvPages);
            else
#endif
            {
#ifndef VBOX_WITH_LINEAR_HOST_PHYS_MEM
                rc = SUPR3PageAllocEx(cHostPages, 0 /*fFlags*/, &pvPages, &pvPagesR0, paPages);
#else
                rc = SUPR3PageAllocEx(cHostPages, 0 /*fFlags*/, &pvPages, NULL /*pR0Ptr*/, paPages);
#endif
            }
            if (RT_SUCCESS(rc))
            {
                memset(pvPages, 0, cGuestPages * GUEST_PAGE_SIZE);

                /*
                 * Create the registered MMIO range record for it.
                 */
                PPGMREGMMIO2RANGE pNew;
                rc = pgmR3PhysMmio2Create(pVM, pDevIns, iSubDev, iRegion, cb, fFlags, idMmio2, pszDesc, &pNew);
                if (RT_SUCCESS(rc))
                {
                    if (phRegion)
                        *phRegion = idMmio2;    /* The ID of the first chunk. */

                    uint32_t iSrcPage   = 0;
                    uint8_t *pbCurPages = (uint8_t *)pvPages;
                    for (PPGMREGMMIO2RANGE pCur = pNew; pCur; pCur = pCur->pNextR3)
                    {
                        pCur->pvR3          = pbCurPages;
#ifndef VBOX_WITH_LINEAR_HOST_PHYS_MEM
                        pCur->pvR0          = pvPagesR0 + (iSrcPage << GUEST_PAGE_SHIFT);
#endif
                        pCur->RamRange.pvR3 = pbCurPages;

                        uint32_t iDstPage = pCur->RamRange.cb >> GUEST_PAGE_SHIFT;
#ifdef VBOX_WITH_PGM_NEM_MODE
                        if (PGM_IS_IN_NEM_MODE(pVM))
                            while (iDstPage-- > 0)
                                PGM_PAGE_INIT(&pNew->RamRange.aPages[iDstPage], UINT64_C(0x0000ffffffff0000),
                                              PGM_MMIO2_PAGEID_MAKE(idMmio2, iDstPage),
                                              PGMPAGETYPE_MMIO2, PGM_PAGE_STATE_ALLOCATED);
                        else
#endif
                        {
                            AssertRelease(HOST_PAGE_SHIFT == GUEST_PAGE_SHIFT);
                            while (iDstPage-- > 0)
                                PGM_PAGE_INIT(&pNew->RamRange.aPages[iDstPage], paPages[iDstPage + iSrcPage].Phys,
                                              PGM_MMIO2_PAGEID_MAKE(idMmio2, iDstPage),
                                              PGMPAGETYPE_MMIO2, PGM_PAGE_STATE_ALLOCATED);
                        }

                        /* advance. */
                        iSrcPage   += pCur->RamRange.cb >> GUEST_PAGE_SHIFT;
                        pbCurPages += pCur->RamRange.cb;
                    }

                    RTMemTmpFree(paPages);

                    /*
                     * Update the page count stats, link the registration and we're done.
                     */
                    pVM->pgm.s.cAllPages += cGuestPages;
                    pVM->pgm.s.cPrivatePages += cGuestPages;

                    pgmR3PhysMmio2Link(pVM, pNew);

                    *ppv = pvPages;
                    return VINF_SUCCESS;
                }

                SUPR3PageFreeEx(pvPages, cHostPages);
            }
        }
        RTMemTmpFree(paPages);
        MMR3AdjustFixedReservation(pVM, -(int32_t)cGuestPages, pszDesc);
    }
    if (pDevIns->iInstance > 0)
        MMR3HeapFree((void *)pszDesc);
    return rc;
}


/**
 * Deregisters and frees an MMIO2 region.
 *
 * Any physical access handlers registered for the region must be deregistered
 * before calling this function.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pDevIns         The device instance owning the region.
 * @param   hMmio2          The MMIO2 handle to deregister, or NIL if all
 *                          regions for the given device is to be deregistered.
 */
VMMR3_INT_DECL(int) PGMR3PhysMmio2Deregister(PVM pVM, PPDMDEVINS pDevIns, PGMMMIO2HANDLE hMmio2)
{
    /*
     * Validate input.
     */
    VM_ASSERT_EMT_RETURN(pVM, VERR_VM_THREAD_NOT_EMT);
    AssertPtrReturn(pDevIns, VERR_INVALID_PARAMETER);

    /*
     * The loop here scanning all registrations will make sure that multi-chunk ranges
     * get properly deregistered, though it's original purpose was the wildcard iRegion.
     */
    PGM_LOCK_VOID(pVM);
    int rc = VINF_SUCCESS;
    unsigned cFound = 0;
    PPGMREGMMIO2RANGE pPrev = NULL;
    PPGMREGMMIO2RANGE pCur = pVM->pgm.s.pRegMmioRangesR3;
    while (pCur)
    {
        uint32_t const fFlags = pCur->fFlags;
        if (    pCur->pDevInsR3 == pDevIns
            &&  (   hMmio2 == NIL_PGMMMIO2HANDLE
                 || pCur->idMmio2 == hMmio2))
        {
            cFound++;

            /*
             * Unmap it if it's mapped.
             */
            if (fFlags & PGMREGMMIO2RANGE_F_MAPPED)
            {
                int rc2 = PGMR3PhysMmio2Unmap(pVM, pCur->pDevInsR3, pCur->idMmio2, pCur->RamRange.GCPhys);
                AssertRC(rc2);
                if (RT_FAILURE(rc2) && RT_SUCCESS(rc))
                    rc = rc2;
            }

            /*
             * Unlink it
             */
            PPGMREGMMIO2RANGE pNext = pCur->pNextR3;
            if (pPrev)
                pPrev->pNextR3 = pNext;
            else
                pVM->pgm.s.pRegMmioRangesR3 = pNext;
            pCur->pNextR3 = NULL;

            uint8_t idMmio2 = pCur->idMmio2;
            Assert(idMmio2 <= RT_ELEMENTS(pVM->pgm.s.apMmio2RangesR3));
            if (idMmio2 <= RT_ELEMENTS(pVM->pgm.s.apMmio2RangesR3))
            {
                Assert(pVM->pgm.s.apMmio2RangesR3[idMmio2 - 1] == pCur);
                pVM->pgm.s.apMmio2RangesR3[idMmio2 - 1] = NULL;
                pVM->pgm.s.apMmio2RangesR0[idMmio2 - 1] = NIL_RTR0PTR;
            }

            /*
             * Free the memory.
             */
            uint32_t const cGuestPages = pCur->cbReal >> GUEST_PAGE_SHIFT;
            uint32_t const cHostPages  = RT_ALIGN_T(pCur->cbReal, HOST_PAGE_SIZE, RTGCPHYS) >> HOST_PAGE_SHIFT;
#ifdef VBOX_WITH_PGM_NEM_MODE
            if (!pVM->pgm.s.fNemMode)
#endif
            {
                int rc2 = SUPR3PageFreeEx(pCur->pvR3, cHostPages);
                AssertRC(rc2);
                if (RT_FAILURE(rc2) && RT_SUCCESS(rc))
                    rc = rc2;

                rc2 = MMR3AdjustFixedReservation(pVM, -(int32_t)cGuestPages, pCur->RamRange.pszDesc);
                AssertRC(rc2);
                if (RT_FAILURE(rc2) && RT_SUCCESS(rc))
                    rc = rc2;
            }
#ifdef VBOX_WITH_PGM_NEM_MODE
            else
            {
                int rc2 = SUPR3PageFreeEx(pCur->pvR3, cHostPages);
                AssertRC(rc2);
                if (RT_FAILURE(rc2) && RT_SUCCESS(rc))
                    rc = rc2;
            }
#endif

            if (pCur->pPhysHandlerR3)
            {
                pgmHandlerPhysicalExDestroy(pVM, pCur->pPhysHandlerR3);
                pCur->pPhysHandlerR3 = NULL;
            }

            /* we're leaking hyper memory here if done at runtime. */
#ifdef VBOX_STRICT
            VMSTATE const enmState = VMR3GetState(pVM);
            AssertMsg(   enmState == VMSTATE_POWERING_OFF
                      || enmState == VMSTATE_POWERING_OFF_LS
                      || enmState == VMSTATE_OFF
                      || enmState == VMSTATE_OFF_LS
                      || enmState == VMSTATE_DESTROYING
                      || enmState == VMSTATE_TERMINATED
                      || enmState == VMSTATE_CREATING
                      , ("%s\n", VMR3GetStateName(enmState)));
#endif

            if (pCur->RamRange.fFlags & PGM_RAM_RANGE_FLAGS_FLOATING)
            {
                const size_t    cbRange     = RT_UOFFSETOF_DYN(PGMREGMMIO2RANGE, RamRange.aPages[cGuestPages]);
                size_t const    cChunkPages = RT_ALIGN_Z(cbRange, HOST_PAGE_SIZE) >> HOST_PAGE_SHIFT;
                SUPR3PageFreeEx(pCur, cChunkPages);
            }
            /*else
            {
                rc = MMHyperFree(pVM, pCur); - does not work, see the alloc call.
                AssertRCReturn(rc, rc);
            } */


            /* update page count stats */
            pVM->pgm.s.cAllPages     -= cGuestPages;
            pVM->pgm.s.cPrivatePages -= cGuestPages;

            /* next */
            pCur = pNext;
            if (hMmio2 != NIL_PGMMMIO2HANDLE)
            {
                if (fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK)
                    break;
                hMmio2++;
                Assert(pCur->idMmio2 == hMmio2);
                Assert(pCur->pDevInsR3 == pDevIns);
                Assert(!(pCur->fFlags & PGMREGMMIO2RANGE_F_FIRST_CHUNK));
            }
        }
        else
        {
            pPrev = pCur;
            pCur = pCur->pNextR3;
        }
    }
    pgmPhysInvalidatePageMapTLB(pVM);
    PGM_UNLOCK(pVM);
    return !cFound && hMmio2 != NIL_PGMMMIO2HANDLE  ? VERR_NOT_FOUND : rc;
}


/**
 * Maps a MMIO2 region.
 *
 * This is typically done when a guest / the bios / state loading changes the
 * PCI config.  The replacing of base memory has the same restrictions as during
 * registration, of course.
 *
 * @returns VBox status code.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pDevIns         The device instance owning the region.
 * @param   hMmio2          The handle of the region to map.
 * @param   GCPhys          The guest-physical address to be remapped.
 */
VMMR3_INT_DECL(int) PGMR3PhysMmio2Map(PVM pVM, PPDMDEVINS pDevIns, PGMMMIO2HANDLE hMmio2, RTGCPHYS GCPhys)
{
    /*
     * Validate input.
     *
     * Note! It's safe to walk the MMIO/MMIO2 list since registrations only
     *       happens during VM construction.
     */
    VM_ASSERT_EMT_RETURN(pVM, VERR_VM_THREAD_NOT_EMT);
    AssertPtrReturn(pDevIns, VERR_INVALID_PARAMETER);
    AssertReturn(GCPhys != NIL_RTGCPHYS, VERR_INVALID_PARAMETER);
    AssertReturn(GCPhys != 0, VERR_INVALID_PARAMETER);
    AssertReturn(!(GCPhys & GUEST_PAGE_OFFSET_MASK), VERR_INVALID_PARAMETER);
    AssertReturn(hMmio2 != NIL_PGMMMIO2HANDLE, VERR_INVALID_HANDLE);

    PPGMREGMMIO2RANGE pFirstMmio = pgmR3PhysMmio2Find(pVM, pDevIns, UINT32_MAX, UINT32_MAX, hMmio2);
    AssertReturn(pFirstMmio, VERR_NOT_FOUND);
    Assert(pFirstMmio->fFlags & PGMREGMMIO2RANGE_F_FIRST_CHUNK);

    PPGMREGMMIO2RANGE pLastMmio = pFirstMmio;
    RTGCPHYS          cbRange   = 0;
    for (;;)
    {
        AssertReturn(!(pLastMmio->fFlags & PGMREGMMIO2RANGE_F_MAPPED), VERR_WRONG_ORDER);
        Assert(pLastMmio->RamRange.GCPhys == NIL_RTGCPHYS);
        Assert(pLastMmio->RamRange.GCPhysLast == NIL_RTGCPHYS);
        Assert(pLastMmio->pDevInsR3 == pFirstMmio->pDevInsR3);
        Assert(pLastMmio->iSubDev   == pFirstMmio->iSubDev);
        Assert(pLastMmio->iRegion   == pFirstMmio->iRegion);
        cbRange += pLastMmio->RamRange.cb;
        if (pLastMmio->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK)
            break;
        pLastMmio = pLastMmio->pNextR3;
    }

    RTGCPHYS GCPhysLast = GCPhys + cbRange - 1;
    AssertLogRelReturn(GCPhysLast > GCPhys, VERR_INVALID_PARAMETER);

    /*
     * Find our location in the ram range list, checking for restriction
     * we don't bother implementing yet (partially overlapping, multiple
     * ram ranges).
     */
    PGM_LOCK_VOID(pVM);

    AssertReturnStmt(!(pFirstMmio->fFlags & PGMREGMMIO2RANGE_F_MAPPED), PGM_UNLOCK(pVM), VERR_WRONG_ORDER);

    bool fRamExists = false;
    PPGMRAMRANGE pRamPrev = NULL;
    PPGMRAMRANGE pRam = pVM->pgm.s.pRamRangesXR3;
    while (pRam && GCPhysLast >= pRam->GCPhys)
    {
        if (    GCPhys     <= pRam->GCPhysLast
            &&  GCPhysLast >= pRam->GCPhys)
        {
            /* Completely within? */
            AssertLogRelMsgReturnStmt(   GCPhys     >= pRam->GCPhys
                                      && GCPhysLast <= pRam->GCPhysLast,
                                      ("%RGp-%RGp (MMIOEx/%s) falls partly outside %RGp-%RGp (%s)\n",
                                       GCPhys, GCPhysLast, pFirstMmio->RamRange.pszDesc,
                                       pRam->GCPhys, pRam->GCPhysLast, pRam->pszDesc),
                                      PGM_UNLOCK(pVM),
                                      VERR_PGM_RAM_CONFLICT);

            /* Check that all the pages are RAM pages. */
            PPGMPAGE pPage      = &pRam->aPages[(GCPhys - pRam->GCPhys) >> GUEST_PAGE_SHIFT];
            uint32_t cPagesLeft = cbRange >> GUEST_PAGE_SHIFT;
            while (cPagesLeft-- > 0)
            {
                AssertLogRelMsgReturnStmt(PGM_PAGE_GET_TYPE(pPage) == PGMPAGETYPE_RAM,
                                          ("%RGp isn't a RAM page (%d) - mapping %RGp-%RGp (MMIO2/%s).\n",
                                           GCPhys, PGM_PAGE_GET_TYPE(pPage), GCPhys, GCPhysLast, pFirstMmio->RamRange.pszDesc),
                                          PGM_UNLOCK(pVM),
                                          VERR_PGM_RAM_CONFLICT);
                pPage++;
            }

            /* There can only be one MMIO/MMIO2 chunk matching here! */
            AssertLogRelMsgReturnStmt(pFirstMmio->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK,
                                      ("%RGp-%RGp (MMIOEx/%s, flags %#X) consists of multiple chunks whereas the RAM somehow doesn't!\n",
                                       GCPhys, GCPhysLast, pFirstMmio->RamRange.pszDesc, pFirstMmio->fFlags),
                                      PGM_UNLOCK(pVM),
                                      VERR_PGM_PHYS_MMIO_EX_IPE);

            fRamExists = true;
            break;
        }

        /* next */
        pRamPrev = pRam;
        pRam = pRam->pNextR3;
    }
    Log(("PGMR3PhysMmio2Map: %RGp-%RGp fRamExists=%RTbool %s\n", GCPhys, GCPhysLast, fRamExists, pFirstMmio->RamRange.pszDesc));


    /*
     * Make the changes.
     */
    RTGCPHYS GCPhysCur = GCPhys;
    for (PPGMREGMMIO2RANGE pCurMmio = pFirstMmio; ; pCurMmio = pCurMmio->pNextR3)
    {
        pCurMmio->RamRange.GCPhys = GCPhysCur;
        pCurMmio->RamRange.GCPhysLast = GCPhysCur + pCurMmio->RamRange.cb - 1;
        if (pCurMmio->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK)
        {
            Assert(pCurMmio->RamRange.GCPhysLast == GCPhysLast);
            break;
        }
        GCPhysCur += pCurMmio->RamRange.cb;
    }

    if (fRamExists)
    {
        /*
         * Make all the pages in the range MMIO/ZERO pages, freeing any
         * RAM pages currently mapped here. This might not be 100% correct
         * for PCI memory, but we're doing the same thing for MMIO2 pages.
         *
         * We replace these MMIO/ZERO pages with real pages in the MMIO2 case.
         */
        Assert(pFirstMmio->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK); /* Only one chunk */
        Assert(pFirstMmio->pvR3 == pFirstMmio->RamRange.pvR3);
        Assert(pFirstMmio->RamRange.pvR3 != NULL);

#ifdef VBOX_WITH_PGM_NEM_MODE
        /* We cannot mix MMIO2 into a RAM range in simplified memory mode because pRam->pvR3 can't point
           both at the RAM and MMIO2, so we won't ever write & read from the actual MMIO2 memory if we try. */
        AssertLogRelMsgReturn(!pVM->pgm.s.fNemMode, ("%s at %RGp-%RGp\n", pFirstMmio->RamRange.pszDesc, GCPhys, GCPhysLast),
                              VERR_PGM_NOT_SUPPORTED_FOR_NEM_MODE);
#endif

        int rc = pgmR3PhysFreePageRange(pVM, pRam, GCPhys, GCPhysLast, pFirstMmio->RamRange.pvR3);
        AssertRCReturnStmt(rc, PGM_UNLOCK(pVM), rc);

        /* Replace the pages, freeing all present RAM pages. */
        PPGMPAGE pPageSrc   = &pFirstMmio->RamRange.aPages[0];
        PPGMPAGE pPageDst   = &pRam->aPages[(GCPhys - pRam->GCPhys) >> GUEST_PAGE_SHIFT];
        uint32_t cPagesLeft = pFirstMmio->RamRange.cb >> GUEST_PAGE_SHIFT;
        while (cPagesLeft-- > 0)
        {
            Assert(PGM_PAGE_IS_MMIO(pPageDst));

            RTHCPHYS const HCPhys = PGM_PAGE_GET_HCPHYS(pPageSrc);
            uint32_t const idPage = PGM_PAGE_GET_PAGEID(pPageSrc);
            PGM_PAGE_SET_PAGEID(pVM, pPageDst, idPage);
            PGM_PAGE_SET_HCPHYS(pVM, pPageDst, HCPhys);
            PGM_PAGE_SET_TYPE(pVM, pPageDst, PGMPAGETYPE_MMIO2);
            PGM_PAGE_SET_STATE(pVM, pPageDst, PGM_PAGE_STATE_ALLOCATED);
            PGM_PAGE_SET_PDE_TYPE(pVM, pPageDst, PGM_PAGE_PDE_TYPE_DONTCARE);
            PGM_PAGE_SET_PTE_INDEX(pVM, pPageDst, 0);
            PGM_PAGE_SET_TRACKING(pVM, pPageDst, 0);
            /* NEM state is set by pgmR3PhysFreePageRange. */

            pVM->pgm.s.cZeroPages--;
            GCPhys += GUEST_PAGE_SIZE;
            pPageSrc++;
            pPageDst++;
        }

        /* Flush physical page map TLB. */
        pgmPhysInvalidatePageMapTLB(pVM);

        /* Force a PGM pool flush as guest ram references have been changed. */
        /** @todo not entirely SMP safe; assuming for now the guest takes care of
         *  this internally (not touch mapped mmio while changing the mapping). */
        PVMCPU pVCpu = VMMGetCpu(pVM);
        pVCpu->pgm.s.fSyncFlags |= PGM_SYNC_CLEAR_PGM_POOL;
        VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3);
    }
    else
    {
        /*
         * No RAM range, insert the ones prepared during registration.
         */
        for (PPGMREGMMIO2RANGE pCurMmio = pFirstMmio; ; pCurMmio = pCurMmio->pNextR3)
        {
#ifdef VBOX_WITH_NATIVE_NEM
            /* Tell NEM and get the new NEM state for the pages. */
            uint8_t u2NemState = 0;
            if (VM_IS_NEM_ENABLED(pVM))
            {
                int rc = NEMR3NotifyPhysMmioExMapEarly(pVM, pCurMmio->RamRange.GCPhys,
                                                       pCurMmio->RamRange.GCPhysLast - pCurMmio->RamRange.GCPhys + 1,
                                                       NEM_NOTIFY_PHYS_MMIO_EX_F_MMIO2
                                                       | (pCurMmio->fFlags & PGMREGMMIO2RANGE_F_TRACK_DIRTY_PAGES
                                                          ? NEM_NOTIFY_PHYS_MMIO_EX_F_TRACK_DIRTY_PAGES : 0),
                                                       NULL /*pvRam*/, pCurMmio->RamRange.pvR3,
                                                       &u2NemState, &pCurMmio->RamRange.uNemRange);
                AssertLogRelRCReturnStmt(rc, PGM_UNLOCK(pVM), rc);
            }
#endif

            /* Clear the tracking data of pages we're going to reactivate. */
            PPGMPAGE pPageSrc   = &pCurMmio->RamRange.aPages[0];
            uint32_t cPagesLeft = pCurMmio->RamRange.cb >> GUEST_PAGE_SHIFT;
            while (cPagesLeft-- > 0)
            {
                PGM_PAGE_SET_TRACKING(pVM, pPageSrc, 0);
                PGM_PAGE_SET_PTE_INDEX(pVM, pPageSrc, 0);
#ifdef VBOX_WITH_NATIVE_NEM
                PGM_PAGE_SET_NEM_STATE(pPageSrc, u2NemState);
#endif
                pPageSrc++;
            }

            /* link in the ram range */
            pgmR3PhysLinkRamRange(pVM, &pCurMmio->RamRange, pRamPrev);

            if (pCurMmio->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK)
            {
                Assert(pCurMmio->RamRange.GCPhysLast == GCPhysLast);
                break;
            }
            pRamPrev = &pCurMmio->RamRange;
        }
    }

    /*
     * If the range have dirty page monitoring enabled, enable that.
     *
     * We ignore failures here for now because if we fail, the whole mapping
     * will have to be reversed and we'll end up with nothing at all on the
     * screen and a grumpy guest, whereas if we just go on, we'll only have
     * visual distortions to gripe about.  There will be something in the
     * release log.
     */
    if (   pFirstMmio->pPhysHandlerR3
        && (pFirstMmio->fFlags & PGMREGMMIO2RANGE_F_TRACKING_ENABLED))
        pgmR3PhysMmio2EnableDirtyPageTracing(pVM, pFirstMmio);

    /*
     * We're good, set the flags and invalid the mapping TLB.
     */
    for (PPGMREGMMIO2RANGE pCurMmio = pFirstMmio; ; pCurMmio = pCurMmio->pNextR3)
    {
        pCurMmio->fFlags |= PGMREGMMIO2RANGE_F_MAPPED;
        if (fRamExists)
            pCurMmio->fFlags |= PGMREGMMIO2RANGE_F_OVERLAPPING;
        else
            pCurMmio->fFlags &= ~PGMREGMMIO2RANGE_F_OVERLAPPING;
        if (pCurMmio->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK)
            break;
    }
    pgmPhysInvalidatePageMapTLB(pVM);

#ifdef VBOX_WITH_NATIVE_NEM
    /*
     * Late NEM notification.
     */
    if (VM_IS_NEM_ENABLED(pVM))
    {
        int rc;
        uint32_t fNemFlags = NEM_NOTIFY_PHYS_MMIO_EX_F_MMIO2;
        if (pFirstMmio->fFlags & PGMREGMMIO2RANGE_F_TRACK_DIRTY_PAGES)
            fNemFlags |= NEM_NOTIFY_PHYS_MMIO_EX_F_TRACK_DIRTY_PAGES;
        if (fRamExists)
            rc = NEMR3NotifyPhysMmioExMapLate(pVM, GCPhys, GCPhysLast - GCPhys + 1, fNemFlags | NEM_NOTIFY_PHYS_MMIO_EX_F_REPLACE,
                                              pRam->pvR3 ? (uint8_t *)pRam->pvR3 + GCPhys - pRam->GCPhys : NULL, pFirstMmio->pvR3,
                                              NULL /*puNemRange*/);
        else
        {
            rc = VINF_SUCCESS;
            for (PPGMREGMMIO2RANGE pCurMmio = pFirstMmio; ; pCurMmio = pCurMmio->pNextR3)
            {
                rc = NEMR3NotifyPhysMmioExMapLate(pVM, pCurMmio->RamRange.GCPhys, pCurMmio->RamRange.cb, fNemFlags,
                                                  NULL, pCurMmio->RamRange.pvR3, &pCurMmio->RamRange.uNemRange);
                if ((pCurMmio->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK) || RT_FAILURE(rc))
                    break;
            }
        }
        AssertLogRelRCReturnStmt(rc, PGMR3PhysMmio2Unmap(pVM, pDevIns, hMmio2, GCPhys); PGM_UNLOCK(pVM), rc);
    }
#endif

    PGM_UNLOCK(pVM);

    return VINF_SUCCESS;
}


/**
 * Unmaps an MMIO2 region.
 *
 * This is typically done when a guest / the bios / state loading changes the
 * PCI config. The replacing of base memory has the same restrictions as during
 * registration, of course.
 */
VMMR3_INT_DECL(int) PGMR3PhysMmio2Unmap(PVM pVM, PPDMDEVINS pDevIns, PGMMMIO2HANDLE hMmio2, RTGCPHYS GCPhys)
{
    /*
     * Validate input
     */
    VM_ASSERT_EMT_RETURN(pVM, VERR_VM_THREAD_NOT_EMT);
    AssertPtrReturn(pDevIns, VERR_INVALID_PARAMETER);
    AssertReturn(hMmio2 != NIL_PGMMMIO2HANDLE, VERR_INVALID_HANDLE);
    if (GCPhys != NIL_RTGCPHYS)
    {
        AssertReturn(GCPhys != 0, VERR_INVALID_PARAMETER);
        AssertReturn(!(GCPhys & GUEST_PAGE_OFFSET_MASK), VERR_INVALID_PARAMETER);
    }

    PPGMREGMMIO2RANGE pFirstMmio = pgmR3PhysMmio2Find(pVM, pDevIns, UINT32_MAX, UINT32_MAX, hMmio2);
    AssertReturn(pFirstMmio, VERR_NOT_FOUND);
    Assert(pFirstMmio->fFlags & PGMREGMMIO2RANGE_F_FIRST_CHUNK);

    int rc = PGM_LOCK(pVM);
    AssertRCReturn(rc, rc);

    PPGMREGMMIO2RANGE pLastMmio = pFirstMmio;
    RTGCPHYS          cbRange   = 0;
    for (;;)
    {
        AssertReturnStmt(pLastMmio->fFlags & PGMREGMMIO2RANGE_F_MAPPED, PGM_UNLOCK(pVM), VERR_WRONG_ORDER);
        AssertReturnStmt(pLastMmio->RamRange.GCPhys == GCPhys + cbRange || GCPhys == NIL_RTGCPHYS, PGM_UNLOCK(pVM), VERR_INVALID_PARAMETER);
        Assert(pLastMmio->pDevInsR3 == pFirstMmio->pDevInsR3);
        Assert(pLastMmio->iSubDev   == pFirstMmio->iSubDev);
        Assert(pLastMmio->iRegion   == pFirstMmio->iRegion);
        cbRange += pLastMmio->RamRange.cb;
        if (pLastMmio->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK)
            break;
        pLastMmio = pLastMmio->pNextR3;
    }

    Log(("PGMR3PhysMmio2Unmap: %RGp-%RGp %s\n",
         pFirstMmio->RamRange.GCPhys, pLastMmio->RamRange.GCPhysLast, pFirstMmio->RamRange.pszDesc));

    uint16_t const fOldFlags = pFirstMmio->fFlags;
    AssertReturnStmt(fOldFlags & PGMREGMMIO2RANGE_F_MAPPED, PGM_UNLOCK(pVM), VERR_WRONG_ORDER);

    /*
     * If monitoring dirty pages, we must deregister the handlers first.
     */
    if (   pFirstMmio->pPhysHandlerR3
        && (fOldFlags & PGMREGMMIO2RANGE_F_TRACKING_ENABLED))
        pgmR3PhysMmio2DisableDirtyPageTracing(pVM, pFirstMmio);

    /*
     * Unmap it.
     */
    int            rcRet     = VINF_SUCCESS;
#ifdef VBOX_WITH_NATIVE_NEM
    uint32_t const fNemFlags = NEM_NOTIFY_PHYS_MMIO_EX_F_MMIO2
                             | (fOldFlags & PGMREGMMIO2RANGE_F_TRACK_DIRTY_PAGES
                                ? NEM_NOTIFY_PHYS_MMIO_EX_F_TRACK_DIRTY_PAGES : 0);
#endif
    if (fOldFlags & PGMREGMMIO2RANGE_F_OVERLAPPING)
    {
        /*
         * We've replaced RAM, replace with zero pages.
         *
         * Note! This is where we might differ a little from a real system, because
         *       it's likely to just show the RAM pages as they were before the
         *       MMIO/MMIO2 region was mapped here.
         */
        /* Only one chunk allowed when overlapping! */
        Assert(fOldFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK);

        /* Restore the RAM pages we've replaced. */
        PPGMRAMRANGE pRam = pVM->pgm.s.pRamRangesXR3;
        while (pRam->GCPhys > pFirstMmio->RamRange.GCPhysLast)
            pRam = pRam->pNextR3;

        PPGMPAGE pPageDst   = &pRam->aPages[(pFirstMmio->RamRange.GCPhys - pRam->GCPhys) >> GUEST_PAGE_SHIFT];
        uint32_t cPagesLeft = pFirstMmio->RamRange.cb >> GUEST_PAGE_SHIFT;
        pVM->pgm.s.cZeroPages += cPagesLeft; /** @todo not correct for NEM mode  */

#ifdef VBOX_WITH_NATIVE_NEM
        if (VM_IS_NEM_ENABLED(pVM)) /* Notify NEM. Note! we cannot be here in simple memory mode, see mapping function. */
        {
            uint8_t u2State = UINT8_MAX;
            rc = NEMR3NotifyPhysMmioExUnmap(pVM, pFirstMmio->RamRange.GCPhys, pFirstMmio->RamRange.cb,
                                            fNemFlags | NEM_NOTIFY_PHYS_MMIO_EX_F_REPLACE,
                                            pRam->pvR3
                                            ? (uint8_t *)pRam->pvR3 + pFirstMmio->RamRange.GCPhys - pRam->GCPhys : NULL,
                                            pFirstMmio->pvR3, &u2State, &pRam->uNemRange);
            AssertRCStmt(rc, rcRet = rc);
            if (u2State != UINT8_MAX)
                pgmPhysSetNemStateForPages(pPageDst, cPagesLeft, u2State);
        }
#endif

        while (cPagesLeft-- > 0)
        {
            PGM_PAGE_INIT_ZERO(pPageDst, pVM, PGMPAGETYPE_RAM);
            pPageDst++;
        }

        /* Flush physical page map TLB. */
        pgmPhysInvalidatePageMapTLB(pVM);

        /* Update range state. */
        pFirstMmio->RamRange.GCPhys = NIL_RTGCPHYS;
        pFirstMmio->RamRange.GCPhysLast = NIL_RTGCPHYS;
        pFirstMmio->fFlags &= ~(PGMREGMMIO2RANGE_F_OVERLAPPING | PGMREGMMIO2RANGE_F_MAPPED);
    }
    else
    {
        /*
         * Unlink the chunks related to the MMIO/MMIO2 region.
         */
        for (PPGMREGMMIO2RANGE pCurMmio = pFirstMmio; ; pCurMmio = pCurMmio->pNextR3)
        {
#ifdef VBOX_WITH_NATIVE_NEM
            if (VM_IS_NEM_ENABLED(pVM)) /* Notify NEM. */
            {
                uint8_t u2State = UINT8_MAX;
                rc = NEMR3NotifyPhysMmioExUnmap(pVM, pCurMmio->RamRange.GCPhys, pCurMmio->RamRange.cb, fNemFlags,
                                                NULL, pCurMmio->pvR3, &u2State, &pCurMmio->RamRange.uNemRange);
                AssertRCStmt(rc, rcRet = rc);
                if (u2State != UINT8_MAX)
                    pgmPhysSetNemStateForPages(pCurMmio->RamRange.aPages, pCurMmio->RamRange.cb >> GUEST_PAGE_SHIFT, u2State);
            }
#endif
            pgmR3PhysUnlinkRamRange(pVM, &pCurMmio->RamRange);
            pCurMmio->RamRange.GCPhys = NIL_RTGCPHYS;
            pCurMmio->RamRange.GCPhysLast = NIL_RTGCPHYS;
            pCurMmio->fFlags &= ~(PGMREGMMIO2RANGE_F_OVERLAPPING | PGMREGMMIO2RANGE_F_MAPPED);
            if (pCurMmio->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK)
                break;
        }
    }

    /* Force a PGM pool flush as guest ram references have been changed. */
    /** @todo not entirely SMP safe; assuming for now the guest takes care
     *  of this internally (not touch mapped mmio while changing the
     *  mapping). */
    PVMCPU pVCpu = VMMGetCpu(pVM);
    pVCpu->pgm.s.fSyncFlags |= PGM_SYNC_CLEAR_PGM_POOL;
    VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3);

    pgmPhysInvalidatePageMapTLB(pVM);
    pgmPhysInvalidRamRangeTlbs(pVM);

    PGM_UNLOCK(pVM);
    return rcRet;
}


/**
 * Reduces the mapping size of a MMIO2 region.
 *
 * This is mainly for dealing with old saved states after changing the default
 * size of a mapping region.  See PGMDevHlpMMIOExReduce and
 * PDMPCIDEV::pfnRegionLoadChangeHookR3.
 *
 * The region must not currently be mapped when making this call.  The VM state
 * must be state restore or VM construction.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pDevIns         The device instance owning the region.
 * @param   hMmio2          The handle of the region to reduce.
 * @param   cbRegion        The new mapping size.
 */
VMMR3_INT_DECL(int) PGMR3PhysMmio2Reduce(PVM pVM, PPDMDEVINS pDevIns, PGMMMIO2HANDLE hMmio2, RTGCPHYS cbRegion)
{
    /*
     * Validate input
     */
    VM_ASSERT_EMT_RETURN(pVM, VERR_VM_THREAD_NOT_EMT);
    AssertPtrReturn(pDevIns, VERR_INVALID_PARAMETER);
    AssertReturn(hMmio2 != NIL_PGMMMIO2HANDLE, VERR_INVALID_HANDLE);
    AssertReturn(cbRegion >= X86_PAGE_SIZE, VERR_INVALID_PARAMETER);
    AssertReturn(!(cbRegion & X86_PAGE_OFFSET_MASK), VERR_UNSUPPORTED_ALIGNMENT);
    VMSTATE enmVmState = VMR3GetState(pVM);
    AssertLogRelMsgReturn(   enmVmState == VMSTATE_CREATING
                          || enmVmState == VMSTATE_LOADING,
                          ("enmVmState=%d (%s)\n", enmVmState, VMR3GetStateName(enmVmState)),
                          VERR_VM_INVALID_VM_STATE);

    int rc = PGM_LOCK(pVM);
    AssertRCReturn(rc, rc);

    PPGMREGMMIO2RANGE pFirstMmio = pgmR3PhysMmio2Find(pVM, pDevIns, UINT32_MAX, UINT32_MAX, hMmio2);
    if (pFirstMmio)
    {
        Assert(pFirstMmio->fFlags & PGMREGMMIO2RANGE_F_FIRST_CHUNK);
        if (!(pFirstMmio->fFlags & PGMREGMMIO2RANGE_F_MAPPED))
        {
            /*
             * NOTE! Current implementation does not support multiple ranges.
             *       Implement when there is a real world need and thus a testcase.
             */
            AssertLogRelMsgStmt(pFirstMmio->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK,
                                ("%s: %#x\n", pFirstMmio->RamRange.pszDesc, pFirstMmio->fFlags),
                                rc = VERR_NOT_SUPPORTED);
            if (RT_SUCCESS(rc))
            {
                /*
                 * Make the change.
                 */
                Log(("PGMR3PhysMmio2Reduce: %s changes from %RGp bytes (%RGp) to %RGp bytes.\n",
                     pFirstMmio->RamRange.pszDesc, pFirstMmio->RamRange.cb, pFirstMmio->cbReal, cbRegion));

                AssertLogRelMsgStmt(cbRegion <= pFirstMmio->cbReal,
                                    ("%s: cbRegion=%#RGp cbReal=%#RGp\n", pFirstMmio->RamRange.pszDesc, cbRegion, pFirstMmio->cbReal),
                                    rc = VERR_OUT_OF_RANGE);
                if (RT_SUCCESS(rc))
                {
                    pFirstMmio->RamRange.cb = cbRegion;
                }
            }
        }
        else
            rc = VERR_WRONG_ORDER;
    }
    else
        rc = VERR_NOT_FOUND;

    PGM_UNLOCK(pVM);
    return rc;
}


/**
 * Validates @a hMmio2, making sure it belongs to @a pDevIns.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pDevIns     The device which allegedly owns @a hMmio2.
 * @param   hMmio2      The handle to validate.
 */
VMMR3_INT_DECL(int) PGMR3PhysMmio2ValidateHandle(PVM pVM, PPDMDEVINS pDevIns, PGMMMIO2HANDLE hMmio2)
{
    /*
     * Validate input
     */
    VM_ASSERT_EMT_RETURN(pVM, VERR_VM_THREAD_NOT_EMT);
    AssertPtrReturn(pDevIns, VERR_INVALID_POINTER);

    /*
     * Just do this the simple way.  No need for locking as this is only taken at
     */
    PGM_LOCK_VOID(pVM);
    PPGMREGMMIO2RANGE pFirstMmio = pgmR3PhysMmio2Find(pVM, pDevIns, UINT32_MAX, UINT32_MAX, hMmio2);
    PGM_UNLOCK(pVM);
    AssertReturn(pFirstMmio, VERR_INVALID_HANDLE);
    AssertReturn(pFirstMmio->fFlags & PGMREGMMIO2RANGE_F_FIRST_CHUNK, VERR_INVALID_HANDLE);
    return VINF_SUCCESS;
}


/**
 * Gets the mapping address of an MMIO2 region.
 *
 * @returns Mapping address, NIL_RTGCPHYS if not mapped or invalid handle.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pDevIns     The device owning the MMIO2 handle.
 * @param   hMmio2      The region handle.
 */
VMMR3_INT_DECL(RTGCPHYS) PGMR3PhysMmio2GetMappingAddress(PVM pVM, PPDMDEVINS pDevIns, PGMMMIO2HANDLE hMmio2)
{
    AssertPtrReturn(pDevIns, NIL_RTGCPHYS);

    PPGMREGMMIO2RANGE pFirstRegMmio = pgmR3PhysMmio2Find(pVM, pDevIns, UINT32_MAX, UINT32_MAX, hMmio2);
    AssertReturn(pFirstRegMmio, NIL_RTGCPHYS);

    if (pFirstRegMmio->fFlags & PGMREGMMIO2RANGE_F_MAPPED)
        return pFirstRegMmio->RamRange.GCPhys;
    return NIL_RTGCPHYS;
}


/**
 * Worker for PGMR3PhysMmio2QueryAndResetDirtyBitmap.
 *
 * Called holding the PGM lock.
 */
static int pgmR3PhysMmio2QueryAndResetDirtyBitmapLocked(PVM pVM, PPDMDEVINS pDevIns, PGMMMIO2HANDLE hMmio2,
                                                        void *pvBitmap, size_t cbBitmap)
{
    /*
     * Continue validation.
     */
    PPGMREGMMIO2RANGE pFirstRegMmio = pgmR3PhysMmio2Find(pVM, pDevIns, UINT32_MAX, UINT32_MAX, hMmio2);
    AssertReturn(pFirstRegMmio, VERR_INVALID_HANDLE);
    AssertReturn(   (pFirstRegMmio->fFlags & (PGMREGMMIO2RANGE_F_TRACK_DIRTY_PAGES | PGMREGMMIO2RANGE_F_FIRST_CHUNK))
                 ==                          (PGMREGMMIO2RANGE_F_TRACK_DIRTY_PAGES | PGMREGMMIO2RANGE_F_FIRST_CHUNK),
                 VERR_INVALID_FUNCTION);
    AssertReturn(pDevIns == pFirstRegMmio->pDevInsR3, VERR_NOT_OWNER);

    RTGCPHYS cbTotal     = 0;
    uint16_t fTotalDirty = 0;
    for (PPGMREGMMIO2RANGE pCur = pFirstRegMmio;;)
    {
        cbTotal     += pCur->RamRange.cb; /* Not using cbReal here, because NEM is not in on the creating, only the mapping. */
        fTotalDirty |= pCur->fFlags;
        if (pCur->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK)
            break;
        pCur = pCur->pNextR3;
        AssertPtrReturn(pCur, VERR_INTERNAL_ERROR_5);
        AssertReturn(   (pCur->fFlags & (PGMREGMMIO2RANGE_F_TRACK_DIRTY_PAGES | PGMREGMMIO2RANGE_F_FIRST_CHUNK))
                     ==                  PGMREGMMIO2RANGE_F_TRACK_DIRTY_PAGES,
                     VERR_INTERNAL_ERROR_4);
    }
    size_t const cbTotalBitmap = RT_ALIGN_T(cbTotal, GUEST_PAGE_SIZE * 64, RTGCPHYS) / GUEST_PAGE_SIZE / 8;

    if (cbBitmap)
    {
        AssertPtrReturn(pvBitmap, VERR_INVALID_POINTER);
        AssertReturn(RT_ALIGN_P(pvBitmap, sizeof(uint64_t)) == pvBitmap, VERR_INVALID_POINTER);
        AssertReturn(cbBitmap == cbTotalBitmap, VERR_INVALID_PARAMETER);
    }

    /*
     * Do the work.
     */
    int rc = VINF_SUCCESS;
    if (pvBitmap)
    {
#ifdef VBOX_WITH_PGM_NEM_MODE
        if (pFirstRegMmio->pPhysHandlerR3 == NULL)
        {
/** @todo This does not integrate at all with --execute-all-in-iem, leaving the
 * screen blank when using it together with --driverless.  Fixing this won't be
 * entirely easy as we take the PGM_PAGE_HNDL_PHYS_STATE_DISABLED page status to
 * mean a dirty page. */
            AssertReturn(VM_IS_NEM_ENABLED(pVM), VERR_INTERNAL_ERROR_4);
            uint8_t *pbBitmap = (uint8_t *)pvBitmap;
            for (PPGMREGMMIO2RANGE pCur = pFirstRegMmio; pCur; pCur = pCur->pNextR3)
            {
                size_t const cbBitmapChunk = pCur->RamRange.cb / GUEST_PAGE_SIZE / 8;
                Assert((RTGCPHYS)cbBitmapChunk * GUEST_PAGE_SIZE * 8 == pCur->RamRange.cb);
                int rc2 = NEMR3PhysMmio2QueryAndResetDirtyBitmap(pVM, pCur->RamRange.GCPhys, pCur->RamRange.cb,
                                                                 pCur->RamRange.uNemRange, pbBitmap, cbBitmapChunk);
                if (RT_FAILURE(rc2) && RT_SUCCESS(rc))
                    rc = rc2;
                if (pCur->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK)
                    break;
                pbBitmap += pCur->RamRange.cb / GUEST_PAGE_SIZE / 8;
            }
        }
        else
#endif
        if (fTotalDirty & PGMREGMMIO2RANGE_F_IS_DIRTY)
        {
            if (   (pFirstRegMmio->fFlags & (PGMREGMMIO2RANGE_F_MAPPED | PGMREGMMIO2RANGE_F_TRACKING_ENABLED))
                ==                          (PGMREGMMIO2RANGE_F_MAPPED | PGMREGMMIO2RANGE_F_TRACKING_ENABLED))
            {
                /*
                 * Reset each chunk, gathering dirty bits.
                 */
                RT_BZERO(pvBitmap, cbBitmap); /* simpler for now. */
                uint32_t iPageNo = 0;
                for (PPGMREGMMIO2RANGE pCur = pFirstRegMmio; pCur; pCur = pCur->pNextR3)
                {
                    if (pCur->fFlags & PGMREGMMIO2RANGE_F_IS_DIRTY)
                    {
                        int rc2 = pgmHandlerPhysicalResetMmio2WithBitmap(pVM, pCur->RamRange.GCPhys, pvBitmap, iPageNo);
                        if (RT_FAILURE(rc2) && RT_SUCCESS(rc))
                            rc = rc2;
                        pCur->fFlags &= ~PGMREGMMIO2RANGE_F_IS_DIRTY;
                    }
                    if (pCur->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK)
                        break;
                    iPageNo += pCur->RamRange.cb >> GUEST_PAGE_SHIFT;
                }
            }
            else
            {
                /*
                 * If not mapped or tracking is disabled, we return the
                 * PGMREGMMIO2RANGE_F_IS_DIRTY status for all pages.  We cannot
                 * get more accurate data than that after unmapping or disabling.
                 */
                RT_BZERO(pvBitmap, cbBitmap);
                uint32_t iPageNo = 0;
                for (PPGMREGMMIO2RANGE pCur = pFirstRegMmio; pCur; pCur = pCur->pNextR3)
                {
                    if (pCur->fFlags & PGMREGMMIO2RANGE_F_IS_DIRTY)
                    {
                        ASMBitSetRange(pvBitmap, iPageNo, iPageNo + (pCur->RamRange.cb >> GUEST_PAGE_SHIFT));
                        pCur->fFlags &= ~PGMREGMMIO2RANGE_F_IS_DIRTY;
                    }
                    if (pCur->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK)
                        break;
                    iPageNo += pCur->RamRange.cb >> GUEST_PAGE_SHIFT;
                }
            }
        }
        /*
         * No dirty chunks.
         */
        else
            RT_BZERO(pvBitmap, cbBitmap);
    }
    /*
     * No bitmap. Reset the region if tracking is currently enabled.
     */
    else if (   (pFirstRegMmio->fFlags & (PGMREGMMIO2RANGE_F_MAPPED | PGMREGMMIO2RANGE_F_TRACKING_ENABLED))
             ==                          (PGMREGMMIO2RANGE_F_MAPPED | PGMREGMMIO2RANGE_F_TRACKING_ENABLED))
    {
#ifdef VBOX_WITH_PGM_NEM_MODE
        if (pFirstRegMmio->pPhysHandlerR3 == NULL)
        {
            AssertReturn(VM_IS_NEM_ENABLED(pVM), VERR_INTERNAL_ERROR_4);
            for (PPGMREGMMIO2RANGE pCur = pFirstRegMmio; pCur; pCur = pCur->pNextR3)
            {
                int rc2 = NEMR3PhysMmio2QueryAndResetDirtyBitmap(pVM, pCur->RamRange.GCPhys, pCur->RamRange.cb,
                                                                 pCur->RamRange.uNemRange, NULL, 0);
                if (RT_FAILURE(rc2) && RT_SUCCESS(rc))
                    rc = rc2;
                if (pCur->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK)
                    break;
            }
        }
        else
#endif
        {
            for (PPGMREGMMIO2RANGE pCur = pFirstRegMmio; pCur; pCur = pCur->pNextR3)
            {
                pCur->fFlags &= ~PGMREGMMIO2RANGE_F_IS_DIRTY;
                int rc2 = PGMHandlerPhysicalReset(pVM, pCur->RamRange.GCPhys);
                if (RT_FAILURE(rc2) && RT_SUCCESS(rc))
                    rc = rc2;
                if (pCur->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK)
                    break;
            }
        }
    }

    return rc;
}


/**
 * Queries the dirty page bitmap and resets the monitoring.
 *
 * The PGMPHYS_MMIO2_FLAGS_TRACK_DIRTY_PAGES flag must be specified when
 * creating the range for this to work.
 *
 * @returns VBox status code.
 * @retval  VERR_INVALID_FUNCTION if not created using
 *          PGMPHYS_MMIO2_FLAGS_TRACK_DIRTY_PAGES.
 * @param   pVM         The cross context VM structure.
 * @param   pDevIns     The device owning the MMIO2 handle.
 * @param   hMmio2      The region handle.
 * @param   pvBitmap    The output bitmap.  Must be 8-byte aligned.  Ignored
 *                      when @a cbBitmap is zero.
 * @param   cbBitmap    The size of the bitmap.  Must be the size of the whole
 *                      MMIO2 range, rounded up to the nearest 8 bytes.
 *                      When zero only a reset is done.
 */
VMMR3_INT_DECL(int) PGMR3PhysMmio2QueryAndResetDirtyBitmap(PVM pVM, PPDMDEVINS pDevIns, PGMMMIO2HANDLE hMmio2,
                                                           void *pvBitmap, size_t cbBitmap)
{
    /*
     * Do some basic validation before grapping the PGM lock and continuing.
     */
    AssertPtrReturn(pDevIns, VERR_INVALID_POINTER);
    AssertReturn(RT_ALIGN_Z(cbBitmap, sizeof(uint64_t)) == cbBitmap, VERR_INVALID_PARAMETER);
    int rc = PGM_LOCK(pVM);
    if (RT_SUCCESS(rc))
    {
        STAM_PROFILE_START(&pVM->pgm.s.StatMmio2QueryAndResetDirtyBitmap, a);
        rc = pgmR3PhysMmio2QueryAndResetDirtyBitmapLocked(pVM, pDevIns, hMmio2, pvBitmap, cbBitmap);
        STAM_PROFILE_STOP(&pVM->pgm.s.StatMmio2QueryAndResetDirtyBitmap, a);
        PGM_UNLOCK(pVM);
    }
    return rc;
}


/**
 * Worker for PGMR3PhysMmio2ControlDirtyPageTracking
 *
 * Called owning the PGM lock.
 */
static int pgmR3PhysMmio2ControlDirtyPageTrackingLocked(PVM pVM, PPDMDEVINS pDevIns, PGMMMIO2HANDLE hMmio2, bool fEnabled)
{
    /*
     * Continue validation.
     */
    PPGMREGMMIO2RANGE pFirstRegMmio = pgmR3PhysMmio2Find(pVM, pDevIns, UINT32_MAX, UINT32_MAX, hMmio2);
    AssertReturn(pFirstRegMmio, VERR_INVALID_HANDLE);
    AssertReturn(   (pFirstRegMmio->fFlags & (PGMREGMMIO2RANGE_F_TRACK_DIRTY_PAGES | PGMREGMMIO2RANGE_F_FIRST_CHUNK))
                 ==                          (PGMREGMMIO2RANGE_F_TRACK_DIRTY_PAGES | PGMREGMMIO2RANGE_F_FIRST_CHUNK)
                 , VERR_INVALID_FUNCTION);
    AssertReturn(pDevIns == pFirstRegMmio->pDevInsR3, VERR_NOT_OWNER);

#ifdef VBOX_WITH_PGM_NEM_MODE
    /*
     * This is a nop if NEM is responsible for doing the tracking, we simply
     * leave the tracking on all the time there.
     */
    if (pFirstRegMmio->pPhysHandlerR3 == NULL)
    {
        AssertReturn(VM_IS_NEM_ENABLED(pVM), VERR_INTERNAL_ERROR_4);
        return VINF_SUCCESS;
    }
#endif

    /*
     * Anyting needing doing?
     */
    if (fEnabled != RT_BOOL(pFirstRegMmio->fFlags & PGMREGMMIO2RANGE_F_TRACKING_ENABLED))
    {
        LogFlowFunc(("fEnabled=%RTbool %s\n", fEnabled, pFirstRegMmio->RamRange.pszDesc));

        /*
         * Update the PGMREGMMIO2RANGE_F_TRACKING_ENABLED flag.
         */
        for (PPGMREGMMIO2RANGE pCur = pFirstRegMmio;;)
        {
            if (fEnabled)
                pCur->fFlags |= PGMREGMMIO2RANGE_F_TRACKING_ENABLED;
            else
                pCur->fFlags &= ~PGMREGMMIO2RANGE_F_TRACKING_ENABLED;
            if (pCur->fFlags & PGMREGMMIO2RANGE_F_LAST_CHUNK)
                break;
            pCur = pCur->pNextR3;
            AssertPtrReturn(pCur, VERR_INTERNAL_ERROR_5);
            AssertReturn(   (pCur->fFlags & (PGMREGMMIO2RANGE_F_TRACK_DIRTY_PAGES | PGMREGMMIO2RANGE_F_FIRST_CHUNK))
                         ==                  PGMREGMMIO2RANGE_F_TRACK_DIRTY_PAGES
                         , VERR_INTERNAL_ERROR_4);
        }

        /*
         * Enable/disable handlers if currently mapped.
         *
         * We ignore status codes here as we've already changed the flags and
         * returning a failure status now would be confusing.  Besides, the two
         * functions will continue past failures.  As argued in the mapping code,
         * it's in the release log.
         */
        if (pFirstRegMmio->fFlags & PGMREGMMIO2RANGE_F_MAPPED)
        {
            if (fEnabled)
                pgmR3PhysMmio2EnableDirtyPageTracing(pVM, pFirstRegMmio);
            else
                pgmR3PhysMmio2DisableDirtyPageTracing(pVM, pFirstRegMmio);
        }
    }
    else
        LogFlowFunc(("fEnabled=%RTbool %s - no change\n", fEnabled, pFirstRegMmio->RamRange.pszDesc));

    return VINF_SUCCESS;
}


/**
 * Controls the dirty page tracking for an MMIO2 range.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pDevIns     The device owning the MMIO2 memory.
 * @param   hMmio2      The handle of the region.
 * @param   fEnabled    The new tracking state.
 */
VMMR3_INT_DECL(int) PGMR3PhysMmio2ControlDirtyPageTracking(PVM pVM, PPDMDEVINS pDevIns, PGMMMIO2HANDLE hMmio2, bool fEnabled)
{
    /*
     * Do some basic validation before grapping the PGM lock and continuing.
     */
    AssertPtrReturn(pDevIns, VERR_INVALID_POINTER);
    int rc = PGM_LOCK(pVM);
    if (RT_SUCCESS(rc))
    {
        rc = pgmR3PhysMmio2ControlDirtyPageTrackingLocked(pVM, pDevIns, hMmio2, fEnabled);
        PGM_UNLOCK(pVM);
    }
    return rc;
}


/**
 * Changes the region number of an MMIO2 region.
 *
 * This is only for dealing with save state issues, nothing else.
 *
 * @return VBox status code.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pDevIns     The device owning the MMIO2 memory.
 * @param   hMmio2      The handle of the region.
 * @param   iNewRegion  The new region index.
 *
 * @thread  EMT(0)
 * @sa      @bugref{9359}
 */
VMMR3_INT_DECL(int) PGMR3PhysMmio2ChangeRegionNo(PVM pVM, PPDMDEVINS pDevIns, PGMMMIO2HANDLE hMmio2, uint32_t iNewRegion)
{
    /*
     * Validate input.
     */
    VM_ASSERT_EMT0_RETURN(pVM, VERR_VM_THREAD_NOT_EMT);
    VM_ASSERT_STATE_RETURN(pVM, VMSTATE_LOADING, VERR_VM_INVALID_VM_STATE);
    AssertPtrReturn(pDevIns, VERR_INVALID_PARAMETER);
    AssertReturn(hMmio2 != NIL_PGMMMIO2HANDLE, VERR_INVALID_HANDLE);
    AssertReturn(iNewRegion <= UINT8_MAX, VERR_INVALID_PARAMETER);

    AssertReturn(pVM->enmVMState == VMSTATE_LOADING, VERR_INVALID_STATE);

    int rc = PGM_LOCK(pVM);
    AssertRCReturn(rc, rc);

    PPGMREGMMIO2RANGE pFirstRegMmio = pgmR3PhysMmio2Find(pVM, pDevIns, UINT32_MAX, UINT32_MAX, hMmio2);
    AssertReturnStmt(pFirstRegMmio, PGM_UNLOCK(pVM), VERR_NOT_FOUND);
    AssertReturnStmt(pgmR3PhysMmio2Find(pVM, pDevIns, pFirstRegMmio->iSubDev, iNewRegion, NIL_PGMMMIO2HANDLE) == NULL,
                     PGM_UNLOCK(pVM), VERR_RESOURCE_IN_USE);

    /*
     * Make the change.
     */
    pFirstRegMmio->iRegion = (uint8_t)iNewRegion;

    PGM_UNLOCK(pVM);
    return VINF_SUCCESS;
}



/*********************************************************************************************************************************
*   ROM                                                                                                                          *
*********************************************************************************************************************************/

/**
 * Worker for PGMR3PhysRomRegister.
 *
 * This is here to simplify lock management, i.e. the caller does all the
 * locking and we can simply return without needing to remember to unlock
 * anything first.
 *
 * @returns VBox status code.
 * @param   pVM                 The cross context VM structure.
 * @param   pDevIns             The device instance owning the ROM.
 * @param   GCPhys              First physical address in the range.
 *                              Must be page aligned!
 * @param   cb                  The size of the range (in bytes).
 *                              Must be page aligned!
 * @param   pvBinary            Pointer to the binary data backing the ROM image.
 * @param   cbBinary            The size of the binary data pvBinary points to.
 *                              This must be less or equal to @a cb.
 * @param   fFlags              Mask of flags. PGMPHYS_ROM_FLAGS_SHADOWED
 *                              and/or PGMPHYS_ROM_FLAGS_PERMANENT_BINARY.
 * @param   pszDesc             Pointer to description string. This must not be freed.
 */
static int pgmR3PhysRomRegisterLocked(PVM pVM, PPDMDEVINS pDevIns, RTGCPHYS GCPhys, RTGCPHYS cb,
                                      const void *pvBinary, uint32_t cbBinary, uint8_t fFlags, const char *pszDesc)
{
    /*
     * Validate input.
     */
    AssertPtrReturn(pDevIns, VERR_INVALID_PARAMETER);
    AssertReturn(RT_ALIGN_T(GCPhys, GUEST_PAGE_SIZE, RTGCPHYS) == GCPhys, VERR_INVALID_PARAMETER);
    AssertReturn(RT_ALIGN_T(cb, GUEST_PAGE_SIZE, RTGCPHYS) == cb, VERR_INVALID_PARAMETER);
    RTGCPHYS GCPhysLast = GCPhys + (cb - 1);
    AssertReturn(GCPhysLast > GCPhys, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pvBinary, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszDesc, VERR_INVALID_POINTER);
    AssertReturn(!(fFlags & ~PGMPHYS_ROM_FLAGS_VALID_MASK), VERR_INVALID_PARAMETER);
    VM_ASSERT_STATE_RETURN(pVM, VMSTATE_CREATING, VERR_VM_INVALID_VM_STATE);

    const uint32_t cGuestPages = cb >> GUEST_PAGE_SHIFT;
#ifdef VBOX_WITH_PGM_NEM_MODE
    const uint32_t cHostPages  = RT_ALIGN_T(cb, HOST_PAGE_SIZE, RTGCPHYS) >> HOST_PAGE_SHIFT;
#endif

    /*
     * Find the ROM location in the ROM list first.
     */
    PPGMROMRANGE    pRomPrev = NULL;
    PPGMROMRANGE    pRom = pVM->pgm.s.pRomRangesR3;
    while (pRom && GCPhysLast >= pRom->GCPhys)
    {
        if (    GCPhys     <= pRom->GCPhysLast
            &&  GCPhysLast >= pRom->GCPhys)
            AssertLogRelMsgFailedReturn(("%RGp-%RGp (%s) conflicts with existing %RGp-%RGp (%s)\n",
                                         GCPhys, GCPhysLast, pszDesc,
                                         pRom->GCPhys, pRom->GCPhysLast, pRom->pszDesc),
                                        VERR_PGM_RAM_CONFLICT);
        /* next */
        pRomPrev = pRom;
        pRom = pRom->pNextR3;
    }

    /*
     * Find the RAM location and check for conflicts.
     *
     * Conflict detection is a bit different than for RAM registration since a
     * ROM can be located within a RAM range. So, what we have to check for is
     * other memory types (other than RAM that is) and that we don't span more
     * than one RAM range (lazy).
     */
    bool            fRamExists = false;
    PPGMRAMRANGE    pRamPrev = NULL;
    PPGMRAMRANGE    pRam = pVM->pgm.s.pRamRangesXR3;
    while (pRam && GCPhysLast >= pRam->GCPhys)
    {
        if (    GCPhys     <= pRam->GCPhysLast
            &&  GCPhysLast >= pRam->GCPhys)
        {
            /* completely within? */
            AssertLogRelMsgReturn(   GCPhys     >= pRam->GCPhys
                                  && GCPhysLast <= pRam->GCPhysLast,
                                  ("%RGp-%RGp (%s) falls partly outside %RGp-%RGp (%s)\n",
                                   GCPhys, GCPhysLast, pszDesc,
                                   pRam->GCPhys, pRam->GCPhysLast, pRam->pszDesc),
                                  VERR_PGM_RAM_CONFLICT);
            fRamExists = true;
            break;
        }

        /* next */
        pRamPrev = pRam;
        pRam = pRam->pNextR3;
    }
    if (fRamExists)
    {
        PPGMPAGE pPage = &pRam->aPages[(GCPhys - pRam->GCPhys) >> GUEST_PAGE_SHIFT];
        uint32_t cPagesLeft = cGuestPages;
        while (cPagesLeft-- > 0)
        {
            AssertLogRelMsgReturn(PGM_PAGE_GET_TYPE(pPage) == PGMPAGETYPE_RAM,
                                  ("%RGp (%R[pgmpage]) isn't a RAM page - registering %RGp-%RGp (%s).\n",
                                   pRam->GCPhys + ((RTGCPHYS)(uintptr_t)(pPage - &pRam->aPages[0]) << GUEST_PAGE_SHIFT),
                                   pPage, GCPhys, GCPhysLast, pszDesc), VERR_PGM_RAM_CONFLICT);
            Assert(PGM_PAGE_IS_ZERO(pPage) || PGM_IS_IN_NEM_MODE(pVM));
            pPage++;
        }
    }

    /*
     * Update the base memory reservation if necessary.
     */
    uint32_t cExtraBaseCost = fRamExists ? 0 : cGuestPages;
    if (fFlags & PGMPHYS_ROM_FLAGS_SHADOWED)
        cExtraBaseCost += cGuestPages;
    if (cExtraBaseCost)
    {
        int rc = MMR3IncreaseBaseReservation(pVM, cExtraBaseCost);
        if (RT_FAILURE(rc))
            return rc;
    }

#ifdef VBOX_WITH_NATIVE_NEM
    /*
     * Early NEM notification before we've made any changes or anything.
     */
    uint32_t const fNemNotify = (fRamExists ? NEM_NOTIFY_PHYS_ROM_F_REPLACE : 0)
                              | (fFlags & PGMPHYS_ROM_FLAGS_SHADOWED ? NEM_NOTIFY_PHYS_ROM_F_SHADOW : 0);
    uint8_t        u2NemState = UINT8_MAX;
    uint32_t       uNemRange  = 0;
    if (VM_IS_NEM_ENABLED(pVM))
    {
        int rc = NEMR3NotifyPhysRomRegisterEarly(pVM, GCPhys, cGuestPages << GUEST_PAGE_SHIFT,
                                                fRamExists ? PGM_RAMRANGE_CALC_PAGE_R3PTR(pRam, GCPhys) : NULL,
                                                fNemNotify, &u2NemState, fRamExists ? &pRam->uNemRange : &uNemRange);
        AssertLogRelRCReturn(rc, rc);
    }
#endif

    /*
     * Allocate memory for the virgin copy of the RAM.  In simplified memory mode,
     * we allocate memory for any ad-hoc RAM range and for shadow pages.
     */
    PGMMALLOCATEPAGESREQ pReq  = NULL;
#ifdef VBOX_WITH_PGM_NEM_MODE
    void                *pvRam = NULL;
    void                *pvAlt = NULL;
    if (pVM->pgm.s.fNemMode)
    {
        if (!fRamExists)
        {
            int rc = SUPR3PageAlloc(cHostPages, 0, &pvRam);
            if (RT_FAILURE(rc))
                return rc;
        }
        if (fFlags & PGMPHYS_ROM_FLAGS_SHADOWED)
        {
            int rc = SUPR3PageAlloc(cHostPages, 0, &pvAlt);
            if (RT_FAILURE(rc))
            {
                if (pvRam)
                    SUPR3PageFree(pvRam, cHostPages);
                return rc;
            }
        }
    }
    else
#endif
    {
        int rc = GMMR3AllocatePagesPrepare(pVM, &pReq, cGuestPages, GMMACCOUNT_BASE);
        AssertRCReturn(rc, rc);

        for (uint32_t iPage = 0; iPage < cGuestPages; iPage++)
        {
            pReq->aPages[iPage].HCPhysGCPhys = GCPhys + (iPage << GUEST_PAGE_SHIFT);
            pReq->aPages[iPage].fZeroed      = false;
            pReq->aPages[iPage].idPage       = NIL_GMM_PAGEID;
            pReq->aPages[iPage].idSharedPage = NIL_GMM_PAGEID;
        }

        rc = GMMR3AllocatePagesPerform(pVM, pReq);
        if (RT_FAILURE(rc))
        {
            GMMR3AllocatePagesCleanup(pReq);
            return rc;
        }
    }

    /*
     * Allocate the new ROM range and RAM range (if necessary).
     */
    PPGMROMRANGE pRomNew     = NULL;
    RTR0PTR      pRomNewR0   = NIL_RTR0PTR;
    size_t const cbRomRange  = RT_ALIGN_Z(RT_UOFFSETOF_DYN(PGMROMRANGE, aPages[cGuestPages]), 128);
    size_t const cbRamRange  = fRamExists ? 0 : RT_UOFFSETOF_DYN(PGMROMRANGE, aPages[cGuestPages]);
    size_t const cRangePages = RT_ALIGN_Z(cbRomRange + cbRamRange, HOST_PAGE_SIZE) >> HOST_PAGE_SHIFT;
    int rc = SUPR3PageAllocEx(cRangePages, 0 /*fFlags*/, (void **)&pRomNew, &pRomNewR0, NULL /*paPages*/);
    if (RT_SUCCESS(rc))
    {

        /*
         * Initialize and insert the RAM range (if required).
         */
        PPGMRAMRANGE       pRamNew;
        uint32_t const     idxFirstRamPage = fRamExists ? (GCPhys - pRam->GCPhys) >> GUEST_PAGE_SHIFT : 0;
        PPGMROMPAGE        pRomPage        = &pRomNew->aPages[0];
        if (!fRamExists)
        {
            /* New RAM range. */
            pRamNew = (PPGMRAMRANGE)((uintptr_t)pRomNew + cbRomRange);
            pRamNew->pSelfR0       = !pRomNewR0 ? NIL_RTR0PTR : pRomNewR0 + cbRomRange;
            pRamNew->GCPhys        = GCPhys;
            pRamNew->GCPhysLast    = GCPhysLast;
            pRamNew->cb            = cb;
            pRamNew->pszDesc       = pszDesc;
            pRamNew->fFlags        = PGM_RAM_RANGE_FLAGS_AD_HOC_ROM;
            pRamNew->pvR3          = NULL;
            pRamNew->paLSPages     = NULL;
#ifdef VBOX_WITH_NATIVE_NEM
            pRamNew->uNemRange     = uNemRange;
#endif

            PPGMPAGE pRamPage = &pRamNew->aPages[idxFirstRamPage];
#ifdef VBOX_WITH_PGM_NEM_MODE
            if (pVM->pgm.s.fNemMode)
            {
                AssertPtr(pvRam); Assert(pReq == NULL);
                pRamNew->pvR3 = pvRam;
                for (uint32_t iPage = 0; iPage < cGuestPages; iPage++, pRamPage++, pRomPage++)
                {
                    PGM_PAGE_INIT(pRamPage, UINT64_C(0x0000fffffffff000), NIL_GMM_PAGEID,
                                  PGMPAGETYPE_ROM, PGM_PAGE_STATE_ALLOCATED);
                    pRomPage->Virgin = *pRamPage;
                }
            }
            else
#endif
                for (uint32_t iPage = 0; iPage < cGuestPages; iPage++, pRamPage++, pRomPage++)
                {
                    PGM_PAGE_INIT(pRamPage,
                                  pReq->aPages[iPage].HCPhysGCPhys,
                                  pReq->aPages[iPage].idPage,
                                  PGMPAGETYPE_ROM,
                                  PGM_PAGE_STATE_ALLOCATED);

                    pRomPage->Virgin = *pRamPage;
                }

            pVM->pgm.s.cAllPages     += cGuestPages;
            pVM->pgm.s.cPrivatePages += cGuestPages;
            pgmR3PhysLinkRamRange(pVM, pRamNew, pRamPrev);
        }
        else
        {
            /* Existing RAM range. */
            PPGMPAGE pRamPage = &pRam->aPages[idxFirstRamPage];
#ifdef VBOX_WITH_PGM_NEM_MODE
            if (pVM->pgm.s.fNemMode)
            {
                Assert(pvRam == NULL); Assert(pReq == NULL);
                for (uint32_t iPage = 0; iPage < cGuestPages; iPage++, pRamPage++, pRomPage++)
                {
                    Assert(PGM_PAGE_GET_HCPHYS(pRamPage) == UINT64_C(0x0000fffffffff000));
                    Assert(PGM_PAGE_GET_PAGEID(pRamPage) == NIL_GMM_PAGEID);
                    Assert(PGM_PAGE_GET_STATE(pRamPage) == PGM_PAGE_STATE_ALLOCATED);
                    PGM_PAGE_SET_TYPE(pVM, pRamPage, PGMPAGETYPE_ROM);
                    PGM_PAGE_SET_STATE(pVM, pRamPage,  PGM_PAGE_STATE_ALLOCATED);
                    PGM_PAGE_SET_PDE_TYPE(pVM, pRamPage, PGM_PAGE_PDE_TYPE_DONTCARE);
                    PGM_PAGE_SET_PTE_INDEX(pVM, pRamPage, 0);
                    PGM_PAGE_SET_TRACKING(pVM, pRamPage, 0);

                    pRomPage->Virgin = *pRamPage;
                }
            }
            else
#endif
            {
                for (uint32_t iPage = 0; iPage < cGuestPages; iPage++, pRamPage++, pRomPage++)
                {
                    PGM_PAGE_SET_TYPE(pVM, pRamPage,   PGMPAGETYPE_ROM);
                    PGM_PAGE_SET_HCPHYS(pVM, pRamPage, pReq->aPages[iPage].HCPhysGCPhys);
                    PGM_PAGE_SET_STATE(pVM, pRamPage,  PGM_PAGE_STATE_ALLOCATED);
                    PGM_PAGE_SET_PAGEID(pVM, pRamPage, pReq->aPages[iPage].idPage);
                    PGM_PAGE_SET_PDE_TYPE(pVM, pRamPage, PGM_PAGE_PDE_TYPE_DONTCARE);
                    PGM_PAGE_SET_PTE_INDEX(pVM, pRamPage, 0);
                    PGM_PAGE_SET_TRACKING(pVM, pRamPage, 0);

                    pRomPage->Virgin = *pRamPage;
                }
                pVM->pgm.s.cZeroPages    -= cGuestPages;
                pVM->pgm.s.cPrivatePages += cGuestPages;
            }
            pRamNew = pRam;
        }

#ifdef VBOX_WITH_NATIVE_NEM
        /* Set the NEM state of the pages if needed. */
        if (u2NemState != UINT8_MAX)
            pgmPhysSetNemStateForPages(&pRamNew->aPages[idxFirstRamPage], cGuestPages, u2NemState);
#endif

        /* Flush physical page map TLB. */
        pgmPhysInvalidatePageMapTLB(pVM);

        /*
         * Register the ROM access handler.
         */
        rc = PGMHandlerPhysicalRegister(pVM, GCPhys, GCPhysLast, pVM->pgm.s.hRomPhysHandlerType, GCPhys, pszDesc);
        if (RT_SUCCESS(rc))
        {
            /*
             * Copy the image over to the virgin pages.
             * This must be done after linking in the RAM range.
             */
            size_t          cbBinaryLeft = cbBinary;
            PPGMPAGE        pRamPage     = &pRamNew->aPages[idxFirstRamPage];
            for (uint32_t iPage = 0; iPage < cGuestPages; iPage++, pRamPage++)
            {
                void *pvDstPage;
                rc = pgmPhysPageMap(pVM, pRamPage, GCPhys + (iPage << GUEST_PAGE_SHIFT), &pvDstPage);
                if (RT_FAILURE(rc))
                {
                    VMSetError(pVM, rc, RT_SRC_POS, "Failed to map virgin ROM page at %RGp", GCPhys);
                    break;
                }
                if (cbBinaryLeft >= GUEST_PAGE_SIZE)
                {
                    memcpy(pvDstPage, (uint8_t const *)pvBinary + ((size_t)iPage << GUEST_PAGE_SHIFT), GUEST_PAGE_SIZE);
                    cbBinaryLeft -= GUEST_PAGE_SIZE;
                }
                else
                {
                    RT_BZERO(pvDstPage, GUEST_PAGE_SIZE); /* (shouldn't be necessary, but can't hurt either) */
                    if (cbBinaryLeft > 0)
                    {
                        memcpy(pvDstPage, (uint8_t const *)pvBinary + ((size_t)iPage << GUEST_PAGE_SHIFT), cbBinaryLeft);
                        cbBinaryLeft = 0;
                    }
                }
            }
            if (RT_SUCCESS(rc))
            {
                /*
                 * Initialize the ROM range.
                 * Note that the Virgin member of the pages has already been initialized above.
                 */
                pRomNew->pSelfR0    = pRomNewR0;
                pRomNew->GCPhys     = GCPhys;
                pRomNew->GCPhysLast = GCPhysLast;
                pRomNew->cb         = cb;
                pRomNew->fFlags     = fFlags;
                pRomNew->idSavedState = UINT8_MAX;
                pRomNew->cbOriginal = cbBinary;
                pRomNew->pszDesc    = pszDesc;
#ifdef VBOX_WITH_PGM_NEM_MODE
                pRomNew->pbR3Alternate = (uint8_t *)pvAlt;
#endif
                pRomNew->pvOriginal = fFlags & PGMPHYS_ROM_FLAGS_PERMANENT_BINARY
                                    ? pvBinary : RTMemDup(pvBinary, cbBinary);
                if (pRomNew->pvOriginal)
                {
                    for (unsigned iPage = 0; iPage < cGuestPages; iPage++)
                    {
                        PPGMROMPAGE pPage = &pRomNew->aPages[iPage];
                        pPage->enmProt = PGMROMPROT_READ_ROM_WRITE_IGNORE;
#ifdef VBOX_WITH_PGM_NEM_MODE
                        if (pVM->pgm.s.fNemMode)
                            PGM_PAGE_INIT(&pPage->Shadow, UINT64_C(0x0000fffffffff000), NIL_GMM_PAGEID,
                                          PGMPAGETYPE_ROM_SHADOW, PGM_PAGE_STATE_ALLOCATED);
                        else
#endif
                            PGM_PAGE_INIT_ZERO(&pPage->Shadow, pVM, PGMPAGETYPE_ROM_SHADOW);
                    }

                    /* update the page count stats for the shadow pages. */
                    if (fFlags & PGMPHYS_ROM_FLAGS_SHADOWED)
                    {
#ifdef VBOX_WITH_PGM_NEM_MODE
                        if (pVM->pgm.s.fNemMode)
                            pVM->pgm.s.cPrivatePages += cGuestPages;
                        else
#endif
                            pVM->pgm.s.cZeroPages += cGuestPages;
                        pVM->pgm.s.cAllPages += cGuestPages;
                    }

                    /*
                     * Insert the ROM range, tell REM and return successfully.
                     */
                    pRomNew->pNextR3 = pRom;
                    pRomNew->pNextR0 = pRom ? pRom->pSelfR0 : NIL_RTR0PTR;

                    if (pRomPrev)
                    {
                        pRomPrev->pNextR3 = pRomNew;
                        pRomPrev->pNextR0 = pRomNew->pSelfR0;
                    }
                    else
                    {
                        pVM->pgm.s.pRomRangesR3 = pRomNew;
                        pVM->pgm.s.pRomRangesR0 = pRomNew->pSelfR0;
                    }

                    pgmPhysInvalidatePageMapTLB(pVM);
#ifdef VBOX_WITH_PGM_NEM_MODE
                    if (!pVM->pgm.s.fNemMode)
#endif
                        GMMR3AllocatePagesCleanup(pReq);

#ifdef VBOX_WITH_NATIVE_NEM
                    /*
                     * Notify NEM again.
                     */
                    if (VM_IS_NEM_ENABLED(pVM))
                    {
                        u2NemState = UINT8_MAX;
                        rc = NEMR3NotifyPhysRomRegisterLate(pVM, GCPhys, cb, PGM_RAMRANGE_CALC_PAGE_R3PTR(pRamNew, GCPhys),
                                                            fNemNotify, &u2NemState,
                                                            fRamExists ? &pRam->uNemRange : &pRamNew->uNemRange);
                        if (u2NemState != UINT8_MAX)
                            pgmPhysSetNemStateForPages(&pRamNew->aPages[idxFirstRamPage], cGuestPages, u2NemState);
                        if (RT_SUCCESS(rc))
                            return rc;
                    }
                    else
#endif
                        return rc;

                    /*
                     * bail out
                     */
#ifdef VBOX_WITH_NATIVE_NEM
                    /* unlink */
                    if (pRomPrev)
                    {
                        pRomPrev->pNextR3 = pRom;
                        pRomPrev->pNextR0 = pRom ? pRom->pSelfR0 : NIL_RTR0PTR;
                    }
                    else
                    {
                        pVM->pgm.s.pRomRangesR3 = pRom;
                        pVM->pgm.s.pRomRangesR0 = pRom ? pRom->pSelfR0 : NIL_RTR0PTR;
                    }

                    if (fFlags & PGMPHYS_ROM_FLAGS_SHADOWED)
                    {
# ifdef VBOX_WITH_PGM_NEM_MODE
                        if (pVM->pgm.s.fNemMode)
                            pVM->pgm.s.cPrivatePages -= cGuestPages;
                        else
# endif
                            pVM->pgm.s.cZeroPages -= cGuestPages;
                        pVM->pgm.s.cAllPages -= cGuestPages;
                    }
#endif
                }
                else
                    rc = VERR_NO_MEMORY;
            }

            int rc2 = PGMHandlerPhysicalDeregister(pVM, GCPhys);
            AssertRC(rc2);
        }

        if (!fRamExists)
            pgmR3PhysUnlinkRamRange2(pVM, pRamNew, pRamPrev);
        else
        {
            PPGMPAGE pRamPage = &pRam->aPages[idxFirstRamPage];
#ifdef VBOX_WITH_PGM_NEM_MODE
            if (pVM->pgm.s.fNemMode)
            {
                Assert(pvRam == NULL); Assert(pReq == NULL);
                for (uint32_t iPage = 0; iPage < cGuestPages; iPage++, pRamPage++, pRomPage++)
                {
                    Assert(PGM_PAGE_GET_HCPHYS(pRamPage) == UINT64_C(0x0000fffffffff000));
                    Assert(PGM_PAGE_GET_PAGEID(pRamPage) == NIL_GMM_PAGEID);
                    Assert(PGM_PAGE_GET_STATE(pRamPage) == PGM_PAGE_STATE_ALLOCATED);
                    PGM_PAGE_SET_TYPE(pVM, pRamPage, PGMPAGETYPE_RAM);
                    PGM_PAGE_SET_STATE(pVM, pRamPage,  PGM_PAGE_STATE_ALLOCATED);
                }
            }
            else
#endif
            {
                for (uint32_t iPage = 0; iPage < cGuestPages; iPage++, pRamPage++)
                    PGM_PAGE_INIT_ZERO(pRamPage, pVM, PGMPAGETYPE_RAM);
                pVM->pgm.s.cZeroPages    += cGuestPages;
                pVM->pgm.s.cPrivatePages -= cGuestPages;
            }
        }

        SUPR3PageFreeEx(pRomNew, cRangePages);
    }

    /** @todo Purge the mapping cache or something... */
#ifdef VBOX_WITH_PGM_NEM_MODE
    if (pVM->pgm.s.fNemMode)
    {
        Assert(!pReq);
        if (pvRam)
            SUPR3PageFree(pvRam, cHostPages);
        if (pvAlt)
            SUPR3PageFree(pvAlt, cHostPages);
    }
    else
#endif
    {
        GMMR3FreeAllocatedPages(pVM, pReq);
        GMMR3AllocatePagesCleanup(pReq);
    }
    return rc;
}


/**
 * Registers a ROM image.
 *
 * Shadowed ROM images requires double the amount of backing memory, so,
 * don't use that unless you have to. Shadowing of ROM images is process
 * where we can select where the reads go and where the writes go. On real
 * hardware the chipset provides means to configure this. We provide
 * PGMR3PhysProtectROM() for this purpose.
 *
 * A read-only copy of the ROM image will always be kept around while we
 * will allocate RAM pages for the changes on demand (unless all memory
 * is configured to be preallocated).
 *
 * @returns VBox status code.
 * @param   pVM                 The cross context VM structure.
 * @param   pDevIns             The device instance owning the ROM.
 * @param   GCPhys              First physical address in the range.
 *                              Must be page aligned!
 * @param   cb                  The size of the range (in bytes).
 *                              Must be page aligned!
 * @param   pvBinary            Pointer to the binary data backing the ROM image.
 * @param   cbBinary            The size of the binary data pvBinary points to.
 *                              This must be less or equal to @a cb.
 * @param   fFlags              Mask of flags, PGMPHYS_ROM_FLAGS_XXX.
 * @param   pszDesc             Pointer to description string. This must not be freed.
 *
 * @remark  There is no way to remove the rom, automatically on device cleanup or
 *          manually from the device yet. This isn't difficult in any way, it's
 *          just not something we expect to be necessary for a while.
 */
VMMR3DECL(int) PGMR3PhysRomRegister(PVM pVM, PPDMDEVINS pDevIns, RTGCPHYS GCPhys, RTGCPHYS cb,
                                    const void *pvBinary, uint32_t cbBinary, uint8_t fFlags, const char *pszDesc)
{
    Log(("PGMR3PhysRomRegister: pDevIns=%p GCPhys=%RGp(-%RGp) cb=%RGp pvBinary=%p cbBinary=%#x fFlags=%#x pszDesc=%s\n",
         pDevIns, GCPhys, GCPhys + cb, cb, pvBinary, cbBinary, fFlags, pszDesc));
    PGM_LOCK_VOID(pVM);
    int rc = pgmR3PhysRomRegisterLocked(pVM, pDevIns, GCPhys, cb, pvBinary, cbBinary, fFlags, pszDesc);
    PGM_UNLOCK(pVM);
    return rc;
}


/**
 * Called by PGMR3MemSetup to reset the shadow, switch to the virgin, and verify
 * that the virgin part is untouched.
 *
 * This is done after the normal memory has been cleared.
 *
 * ASSUMES that the caller owns the PGM lock.
 *
 * @param   pVM         The cross context VM structure.
 */
int pgmR3PhysRomReset(PVM pVM)
{
    PGM_LOCK_ASSERT_OWNER(pVM);
    for (PPGMROMRANGE pRom = pVM->pgm.s.pRomRangesR3; pRom; pRom = pRom->pNextR3)
    {
        const uint32_t cGuestPages = pRom->cb >> GUEST_PAGE_SHIFT;

        if (pRom->fFlags & PGMPHYS_ROM_FLAGS_SHADOWED)
        {
            /*
             * Reset the physical handler.
             */
            int rc = PGMR3PhysRomProtect(pVM, pRom->GCPhys, pRom->cb, PGMROMPROT_READ_ROM_WRITE_IGNORE);
            AssertRCReturn(rc, rc);

            /*
             * What we do with the shadow pages depends on the memory
             * preallocation option. If not enabled, we'll just throw
             * out all the dirty pages and replace them by the zero page.
             */
#ifdef VBOX_WITH_PGM_NEM_MODE
            if (pVM->pgm.s.fNemMode)
            {
                /* Clear all the shadow pages (currently using alternate backing). */
                RT_BZERO(pRom->pbR3Alternate, pRom->cb);
            }
            else
#endif
            if (!pVM->pgm.s.fRamPreAlloc)
            {
                /* Free the dirty pages. */
                uint32_t            cPendingPages = 0;
                PGMMFREEPAGESREQ    pReq;
                rc = GMMR3FreePagesPrepare(pVM, &pReq, PGMPHYS_FREE_PAGE_BATCH_SIZE, GMMACCOUNT_BASE);
                AssertRCReturn(rc, rc);

                for (uint32_t iPage = 0; iPage < cGuestPages; iPage++)
                    if (   !PGM_PAGE_IS_ZERO(&pRom->aPages[iPage].Shadow)
                        && !PGM_PAGE_IS_BALLOONED(&pRom->aPages[iPage].Shadow))
                    {
                        Assert(PGM_PAGE_GET_STATE(&pRom->aPages[iPage].Shadow) == PGM_PAGE_STATE_ALLOCATED);
                        rc = pgmPhysFreePage(pVM, pReq, &cPendingPages, &pRom->aPages[iPage].Shadow,
                                             pRom->GCPhys + (iPage << GUEST_PAGE_SHIFT),
                                             (PGMPAGETYPE)PGM_PAGE_GET_TYPE(&pRom->aPages[iPage].Shadow));
                        AssertLogRelRCReturn(rc, rc);
                    }

                if (cPendingPages)
                {
                    rc = GMMR3FreePagesPerform(pVM, pReq, cPendingPages);
                    AssertLogRelRCReturn(rc, rc);
                }
                GMMR3FreePagesCleanup(pReq);
            }
            else
            {
                /* clear all the shadow pages. */
                for (uint32_t iPage = 0; iPage < cGuestPages; iPage++)
                {
                    if (PGM_PAGE_IS_ZERO(&pRom->aPages[iPage].Shadow))
                        continue;
                    Assert(!PGM_PAGE_IS_BALLOONED(&pRom->aPages[iPage].Shadow));
                    void *pvDstPage;
                    const RTGCPHYS GCPhys = pRom->GCPhys + (iPage << GUEST_PAGE_SHIFT);
                    rc = pgmPhysPageMakeWritableAndMap(pVM, &pRom->aPages[iPage].Shadow, GCPhys, &pvDstPage);
                    if (RT_FAILURE(rc))
                        break;
                    RT_BZERO(pvDstPage, GUEST_PAGE_SIZE);
                }
                AssertRCReturn(rc, rc);
            }
        }

        /*
         * Restore the original ROM pages after a saved state load.
         * Also, in strict builds check that ROM pages remain unmodified.
         */
#ifndef VBOX_STRICT
        if (pVM->pgm.s.fRestoreRomPagesOnReset)
#endif
        {
            size_t         cbSrcLeft = pRom->cbOriginal;
            uint8_t const *pbSrcPage = (uint8_t const *)pRom->pvOriginal;
            uint32_t       cRestored = 0;
            for (uint32_t iPage = 0; iPage < cGuestPages && cbSrcLeft > 0; iPage++, pbSrcPage += GUEST_PAGE_SIZE)
            {
                const RTGCPHYS GCPhys    = pRom->GCPhys + (iPage << GUEST_PAGE_SHIFT);
                PPGMPAGE const pPage     = pgmPhysGetPage(pVM, GCPhys);
                void const    *pvDstPage = NULL;
                int rc = pgmPhysPageMapReadOnly(pVM, pPage, GCPhys, &pvDstPage);
                if (RT_FAILURE(rc))
                    break;

                if (memcmp(pvDstPage, pbSrcPage, RT_MIN(cbSrcLeft, GUEST_PAGE_SIZE)))
                {
                    if (pVM->pgm.s.fRestoreRomPagesOnReset)
                    {
                        void *pvDstPageW = NULL;
                        rc = pgmPhysPageMap(pVM, pPage, GCPhys, &pvDstPageW);
                        AssertLogRelRCReturn(rc, rc);
                        memcpy(pvDstPageW, pbSrcPage, RT_MIN(cbSrcLeft, GUEST_PAGE_SIZE));
                        cRestored++;
                    }
                    else
                        LogRel(("pgmR3PhysRomReset: %RGp: ROM page changed (%s)\n", GCPhys, pRom->pszDesc));
                }
                cbSrcLeft -= RT_MIN(cbSrcLeft, GUEST_PAGE_SIZE);
            }
            if (cRestored > 0)
                LogRel(("PGM: ROM \"%s\": Reloaded %u of %u pages.\n", pRom->pszDesc, cRestored, cGuestPages));
        }
    }

    /* Clear the ROM restore flag now as we only need to do this once after
       loading saved state. */
    pVM->pgm.s.fRestoreRomPagesOnReset = false;

    return VINF_SUCCESS;
}


/**
 * Called by PGMR3Term to free resources.
 *
 * ASSUMES that the caller owns the PGM lock.
 *
 * @param   pVM         The cross context VM structure.
 */
void pgmR3PhysRomTerm(PVM pVM)
{
    /*
     * Free the heap copy of the original bits.
     */
    for (PPGMROMRANGE pRom = pVM->pgm.s.pRomRangesR3; pRom; pRom = pRom->pNextR3)
    {
        if (   pRom->pvOriginal
            && !(pRom->fFlags & PGMPHYS_ROM_FLAGS_PERMANENT_BINARY))
        {
            RTMemFree((void *)pRom->pvOriginal);
            pRom->pvOriginal = NULL;
        }
    }
}


/**
 * Change the shadowing of a range of ROM pages.
 *
 * This is intended for implementing chipset specific memory registers
 * and will not be very strict about the input. It will silently ignore
 * any pages that are not the part of a shadowed ROM.
 *
 * @returns VBox status code.
 * @retval  VINF_PGM_SYNC_CR3
 *
 * @param   pVM         The cross context VM structure.
 * @param   GCPhys      Where to start. Page aligned.
 * @param   cb          How much to change. Page aligned.
 * @param   enmProt     The new ROM protection.
 */
VMMR3DECL(int) PGMR3PhysRomProtect(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, PGMROMPROT enmProt)
{
    /*
     * Check input
     */
    if (!cb)
        return VINF_SUCCESS;
    AssertReturn(!(GCPhys & GUEST_PAGE_OFFSET_MASK), VERR_INVALID_PARAMETER);
    AssertReturn(!(cb & GUEST_PAGE_OFFSET_MASK), VERR_INVALID_PARAMETER);
    RTGCPHYS GCPhysLast = GCPhys + (cb - 1);
    AssertReturn(GCPhysLast > GCPhys, VERR_INVALID_PARAMETER);
    AssertReturn(enmProt >= PGMROMPROT_INVALID && enmProt <= PGMROMPROT_END, VERR_INVALID_PARAMETER);

    /*
     * Process the request.
     */
    PGM_LOCK_VOID(pVM);
    int  rc = VINF_SUCCESS;
    bool fFlushTLB = false;
    for (PPGMROMRANGE pRom = pVM->pgm.s.pRomRangesR3; pRom; pRom = pRom->pNextR3)
    {
        if (   GCPhys     <= pRom->GCPhysLast
            && GCPhysLast >= pRom->GCPhys
            && (pRom->fFlags & PGMPHYS_ROM_FLAGS_SHADOWED))
        {
            /*
             * Iterate the relevant pages and make necessary the changes.
             */
#ifdef VBOX_WITH_NATIVE_NEM
            PPGMRAMRANGE const pRam = pgmPhysGetRange(pVM, GCPhys);
            AssertPtrReturn(pRam, VERR_INTERNAL_ERROR_3);
#endif
            bool fChanges = false;
            uint32_t const cPages = pRom->GCPhysLast <= GCPhysLast
                                  ? pRom->cb >> GUEST_PAGE_SHIFT
                                  : (GCPhysLast - pRom->GCPhys + 1) >> GUEST_PAGE_SHIFT;
            for (uint32_t iPage = (GCPhys - pRom->GCPhys) >> GUEST_PAGE_SHIFT;
                 iPage < cPages;
                 iPage++)
            {
                PPGMROMPAGE pRomPage = &pRom->aPages[iPage];
                if (PGMROMPROT_IS_ROM(pRomPage->enmProt) != PGMROMPROT_IS_ROM(enmProt))
                {
                    fChanges = true;

                    /* flush references to the page. */
                    RTGCPHYS const GCPhysPage = pRom->GCPhys + (iPage << GUEST_PAGE_SHIFT);
                    PPGMPAGE pRamPage = pgmPhysGetPage(pVM, GCPhysPage);
                    int rc2 = pgmPoolTrackUpdateGCPhys(pVM, GCPhysPage, pRamPage, true /*fFlushPTEs*/, &fFlushTLB);
                    if (rc2 != VINF_SUCCESS && (rc == VINF_SUCCESS || RT_FAILURE(rc2)))
                        rc = rc2;
#ifdef VBOX_WITH_NATIVE_NEM
                    uint8_t u2State = PGM_PAGE_GET_NEM_STATE(pRamPage);
#endif

                    PPGMPAGE pOld = PGMROMPROT_IS_ROM(pRomPage->enmProt) ? &pRomPage->Virgin : &pRomPage->Shadow;
                    PPGMPAGE pNew = PGMROMPROT_IS_ROM(pRomPage->enmProt) ? &pRomPage->Shadow : &pRomPage->Virgin;

                    *pOld = *pRamPage;
                    *pRamPage = *pNew;
                    /** @todo preserve the volatile flags (handlers) when these have been moved out of HCPhys! */

#ifdef VBOX_WITH_NATIVE_NEM
# ifdef VBOX_WITH_PGM_NEM_MODE
                    /* In simplified mode we have to switch the page data around too. */
                    if (pVM->pgm.s.fNemMode)
                    {
                        uint8_t         abPage[GUEST_PAGE_SIZE];
                        uint8_t * const pbRamPage = PGM_RAMRANGE_CALC_PAGE_R3PTR(pRam, GCPhysPage);
                        memcpy(abPage, &pRom->pbR3Alternate[(size_t)iPage << GUEST_PAGE_SHIFT], sizeof(abPage));
                        memcpy(&pRom->pbR3Alternate[(size_t)iPage << GUEST_PAGE_SHIFT], pbRamPage, sizeof(abPage));
                        memcpy(pbRamPage, abPage, sizeof(abPage));
                    }
# endif
                    /* Tell NEM about the backing and protection change. */
                    if (VM_IS_NEM_ENABLED(pVM))
                    {
                        PGMPAGETYPE enmType = (PGMPAGETYPE)PGM_PAGE_GET_TYPE(pNew);
                        NEMHCNotifyPhysPageChanged(pVM, GCPhys, PGM_PAGE_GET_HCPHYS(pOld), PGM_PAGE_GET_HCPHYS(pNew),
                                                   PGM_RAMRANGE_CALC_PAGE_R3PTR(pRam, GCPhysPage),
                                                   pgmPhysPageCalcNemProtection(pRamPage, enmType), enmType, &u2State);
                        PGM_PAGE_SET_NEM_STATE(pRamPage, u2State);
                    }
#endif
                }
                pRomPage->enmProt = enmProt;
            }

            /*
             * Reset the access handler if we made changes, no need
             * to optimize this.
             */
            if (fChanges)
            {
                int rc2 = PGMHandlerPhysicalReset(pVM, pRom->GCPhys);
                if (RT_FAILURE(rc2))
                {
                    PGM_UNLOCK(pVM);
                    AssertRC(rc);
                    return rc2;
                }
            }

            /* Advance - cb isn't updated. */
            GCPhys = pRom->GCPhys + (cPages << GUEST_PAGE_SHIFT);
        }
    }
    PGM_UNLOCK(pVM);
    if (fFlushTLB)
        PGM_INVL_ALL_VCPU_TLBS(pVM);

    return rc;
}



/*********************************************************************************************************************************
*   Ballooning                                                                                                                   *
*********************************************************************************************************************************/

#if HC_ARCH_BITS == 64 && (defined(RT_OS_WINDOWS) || defined(RT_OS_SOLARIS) || defined(RT_OS_LINUX) || defined(RT_OS_FREEBSD))

/**
 * Rendezvous callback used by PGMR3ChangeMemBalloon that changes the memory balloon size
 *
 * This is only called on one of the EMTs while the other ones are waiting for
 * it to complete this function.
 *
 * @returns VINF_SUCCESS (VBox strict status code).
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT. Unused.
 * @param   pvUser      User parameter
 */
static DECLCALLBACK(VBOXSTRICTRC) pgmR3PhysChangeMemBalloonRendezvous(PVM pVM, PVMCPU pVCpu, void *pvUser)
{
    uintptr_t          *paUser          = (uintptr_t *)pvUser;
    bool                fInflate        = !!paUser[0];
    unsigned            cPages          = paUser[1];
    RTGCPHYS           *paPhysPage      = (RTGCPHYS *)paUser[2];
    uint32_t            cPendingPages   = 0;
    PGMMFREEPAGESREQ    pReq;
    int                 rc;

    Log(("pgmR3PhysChangeMemBalloonRendezvous: %s %x pages\n", (fInflate) ? "inflate" : "deflate", cPages));
    PGM_LOCK_VOID(pVM);

    if (fInflate)
    {
        /* Flush the PGM pool cache as we might have stale references to pages that we just freed. */
        pgmR3PoolClearAllRendezvous(pVM, pVCpu, NULL);

        /* Replace pages with ZERO pages. */
        rc = GMMR3FreePagesPrepare(pVM, &pReq, PGMPHYS_FREE_PAGE_BATCH_SIZE, GMMACCOUNT_BASE);
        if (RT_FAILURE(rc))
        {
            PGM_UNLOCK(pVM);
            AssertLogRelRC(rc);
            return rc;
        }

        /* Iterate the pages. */
        for (unsigned i = 0; i < cPages; i++)
        {
            PPGMPAGE pPage = pgmPhysGetPage(pVM, paPhysPage[i]);
            if (    pPage == NULL
                ||  PGM_PAGE_GET_TYPE(pPage) != PGMPAGETYPE_RAM)
            {
                Log(("pgmR3PhysChangeMemBalloonRendezvous: invalid physical page %RGp pPage->u3Type=%d\n", paPhysPage[i], pPage ? PGM_PAGE_GET_TYPE(pPage) : 0));
                break;
            }

            LogFlow(("balloon page: %RGp\n", paPhysPage[i]));

            /* Flush the shadow PT if this page was previously used as a guest page table. */
            pgmPoolFlushPageByGCPhys(pVM, paPhysPage[i]);

            rc = pgmPhysFreePage(pVM, pReq, &cPendingPages, pPage, paPhysPage[i], (PGMPAGETYPE)PGM_PAGE_GET_TYPE(pPage));
            if (RT_FAILURE(rc))
            {
                PGM_UNLOCK(pVM);
                AssertLogRelRC(rc);
                return rc;
            }
            Assert(PGM_PAGE_IS_ZERO(pPage));
            PGM_PAGE_SET_STATE(pVM, pPage, PGM_PAGE_STATE_BALLOONED);
        }

        if (cPendingPages)
        {
            rc = GMMR3FreePagesPerform(pVM, pReq, cPendingPages);
            if (RT_FAILURE(rc))
            {
                PGM_UNLOCK(pVM);
                AssertLogRelRC(rc);
                return rc;
            }
        }
        GMMR3FreePagesCleanup(pReq);
    }
    else
    {
        /* Iterate the pages. */
        for (unsigned i = 0; i < cPages; i++)
        {
            PPGMPAGE pPage = pgmPhysGetPage(pVM, paPhysPage[i]);
            AssertBreak(pPage && PGM_PAGE_GET_TYPE(pPage) == PGMPAGETYPE_RAM);

            LogFlow(("Free ballooned page: %RGp\n", paPhysPage[i]));

            Assert(PGM_PAGE_IS_BALLOONED(pPage));

            /* Change back to zero page.  (NEM does not need to be informed.) */
            PGM_PAGE_SET_STATE(pVM, pPage, PGM_PAGE_STATE_ZERO);
        }

        /* Note that we currently do not map any ballooned pages in our shadow page tables, so no need to flush the pgm pool. */
    }

    /* Notify GMM about the balloon change. */
    rc = GMMR3BalloonedPages(pVM, (fInflate) ? GMMBALLOONACTION_INFLATE : GMMBALLOONACTION_DEFLATE, cPages);
    if (RT_SUCCESS(rc))
    {
        if (!fInflate)
        {
            Assert(pVM->pgm.s.cBalloonedPages >= cPages);
            pVM->pgm.s.cBalloonedPages -= cPages;
        }
        else
            pVM->pgm.s.cBalloonedPages += cPages;
    }

    PGM_UNLOCK(pVM);

    /* Flush the recompiler's TLB as well. */
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
        CPUMSetChangedFlags(pVM->apCpusR3[i], CPUM_CHANGED_GLOBAL_TLB_FLUSH);

    AssertLogRelRC(rc);
    return rc;
}


/**
 * Frees a range of ram pages, replacing them with ZERO pages; helper for PGMR3PhysFreeRamPages
 *
 * @param   pVM         The cross context VM structure.
 * @param   fInflate    Inflate or deflate memory balloon
 * @param   cPages      Number of pages to free
 * @param   paPhysPage  Array of guest physical addresses
 */
static DECLCALLBACK(void) pgmR3PhysChangeMemBalloonHelper(PVM pVM, bool fInflate, unsigned cPages, RTGCPHYS *paPhysPage)
{
    uintptr_t paUser[3];

    paUser[0] = fInflate;
    paUser[1] = cPages;
    paUser[2] = (uintptr_t)paPhysPage;
    int rc = VMMR3EmtRendezvous(pVM, VMMEMTRENDEZVOUS_FLAGS_TYPE_ONCE, pgmR3PhysChangeMemBalloonRendezvous, (void *)paUser);
    AssertRC(rc);

    /* Made a copy in PGMR3PhysFreeRamPages; free it here. */
    RTMemFree(paPhysPage);
}

#endif /* 64-bit host && (Windows || Solaris || Linux || FreeBSD) */

/**
 * Inflate or deflate a memory balloon
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   fInflate    Inflate or deflate memory balloon
 * @param   cPages      Number of pages to free
 * @param   paPhysPage  Array of guest physical addresses
 */
VMMR3DECL(int) PGMR3PhysChangeMemBalloon(PVM pVM, bool fInflate, unsigned cPages, RTGCPHYS *paPhysPage)
{
    /* This must match GMMR0Init; currently we only support memory ballooning on all 64-bit hosts except Mac OS X */
#if HC_ARCH_BITS == 64 && (defined(RT_OS_WINDOWS) || defined(RT_OS_SOLARIS) || defined(RT_OS_LINUX) || defined(RT_OS_FREEBSD))
    int rc;

    /* Older additions (ancient non-functioning balloon code) pass wrong physical addresses. */
    AssertReturn(!(paPhysPage[0] & 0xfff), VERR_INVALID_PARAMETER);

    /* We own the IOM lock here and could cause a deadlock by waiting for another VCPU that is blocking on the IOM lock.
     * In the SMP case we post a request packet to postpone the job.
     */
    if (pVM->cCpus > 1)
    {
        unsigned cbPhysPage = cPages * sizeof(paPhysPage[0]);
        RTGCPHYS *paPhysPageCopy = (RTGCPHYS *)RTMemAlloc(cbPhysPage);
        AssertReturn(paPhysPageCopy, VERR_NO_MEMORY);

        memcpy(paPhysPageCopy, paPhysPage, cbPhysPage);

        rc = VMR3ReqCallNoWait(pVM, VMCPUID_ANY_QUEUE, (PFNRT)pgmR3PhysChangeMemBalloonHelper, 4, pVM, fInflate, cPages, paPhysPageCopy);
        AssertRC(rc);
    }
    else
    {
        uintptr_t paUser[3];

        paUser[0] = fInflate;
        paUser[1] = cPages;
        paUser[2] = (uintptr_t)paPhysPage;
        rc = VMMR3EmtRendezvous(pVM, VMMEMTRENDEZVOUS_FLAGS_TYPE_ONCE, pgmR3PhysChangeMemBalloonRendezvous, (void *)paUser);
        AssertRC(rc);
    }
    return rc;

#else
    NOREF(pVM); NOREF(fInflate); NOREF(cPages); NOREF(paPhysPage);
    return VERR_NOT_IMPLEMENTED;
#endif
}


/*********************************************************************************************************************************
*   Write Monitoring                                                                                                             *
*********************************************************************************************************************************/

/**
 * Rendezvous callback used by PGMR3WriteProtectRAM that write protects all
 * physical RAM.
 *
 * This is only called on one of the EMTs while the other ones are waiting for
 * it to complete this function.
 *
 * @returns VINF_SUCCESS (VBox strict status code).
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT. Unused.
 * @param   pvUser      User parameter, unused.
 */
static DECLCALLBACK(VBOXSTRICTRC) pgmR3PhysWriteProtectRAMRendezvous(PVM pVM, PVMCPU pVCpu, void *pvUser)
{
    int rc = VINF_SUCCESS;
    NOREF(pvUser); NOREF(pVCpu);

    PGM_LOCK_VOID(pVM);
#ifdef PGMPOOL_WITH_OPTIMIZED_DIRTY_PT
    pgmPoolResetDirtyPages(pVM);
#endif

    /** @todo pointless to write protect the physical page pointed to by RSP. */

    for (PPGMRAMRANGE pRam = pVM->pgm.s.CTX_SUFF(pRamRangesX);
         pRam;
         pRam = pRam->CTX_SUFF(pNext))
    {
        uint32_t cPages = pRam->cb >> GUEST_PAGE_SHIFT;
        for (uint32_t iPage = 0; iPage < cPages; iPage++)
        {
            PPGMPAGE    pPage = &pRam->aPages[iPage];
            PGMPAGETYPE enmPageType = (PGMPAGETYPE)PGM_PAGE_GET_TYPE(pPage);

            if (    RT_LIKELY(enmPageType == PGMPAGETYPE_RAM)
                ||  enmPageType == PGMPAGETYPE_MMIO2)
            {
                /*
                 * A RAM page.
                 */
                switch (PGM_PAGE_GET_STATE(pPage))
                {
                    case PGM_PAGE_STATE_ALLOCATED:
                        /** @todo Optimize this: Don't always re-enable write
                         * monitoring if the page is known to be very busy. */
                        if (PGM_PAGE_IS_WRITTEN_TO(pPage))
                            PGM_PAGE_CLEAR_WRITTEN_TO(pVM, pPage);

                        pgmPhysPageWriteMonitor(pVM, pPage, pRam->GCPhys + ((RTGCPHYS)iPage << GUEST_PAGE_SHIFT));
                        break;

                    case PGM_PAGE_STATE_SHARED:
                        AssertFailed();
                        break;

                    case PGM_PAGE_STATE_WRITE_MONITORED:    /* nothing to change. */
                    default:
                        break;
                }
            }
        }
    }
    pgmR3PoolWriteProtectPages(pVM);
    PGM_INVL_ALL_VCPU_TLBS(pVM);
    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
        CPUMSetChangedFlags(pVM->apCpusR3[idCpu], CPUM_CHANGED_GLOBAL_TLB_FLUSH);

    PGM_UNLOCK(pVM);
    return rc;
}

/**
 * Protect all physical RAM to monitor writes
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3DECL(int) PGMR3PhysWriteProtectRAM(PVM pVM)
{
    VM_ASSERT_EMT_RETURN(pVM, VERR_VM_THREAD_NOT_EMT);

    int rc = VMMR3EmtRendezvous(pVM, VMMEMTRENDEZVOUS_FLAGS_TYPE_ONCE, pgmR3PhysWriteProtectRAMRendezvous, NULL);
    AssertRC(rc);
    return rc;
}


/*********************************************************************************************************************************
*   Stats.                                                                                                                       *
*********************************************************************************************************************************/

/**
 * Query the amount of free memory inside VMMR0
 *
 * @returns VBox status code.
 * @param   pUVM                The user mode VM handle.
 * @param   pcbAllocMem         Where to return the amount of memory allocated
 *                              by VMs.
 * @param   pcbFreeMem          Where to return the amount of memory that is
 *                              allocated from the host but not currently used
 *                              by any VMs.
 * @param   pcbBallonedMem      Where to return the sum of memory that is
 *                              currently ballooned by the VMs.
 * @param   pcbSharedMem        Where to return the amount of memory that is
 *                              currently shared.
 */
VMMR3DECL(int) PGMR3QueryGlobalMemoryStats(PUVM pUVM, uint64_t *pcbAllocMem, uint64_t *pcbFreeMem,
                                           uint64_t *pcbBallonedMem, uint64_t *pcbSharedMem)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    VM_ASSERT_VALID_EXT_RETURN(pUVM->pVM, VERR_INVALID_VM_HANDLE);

    uint64_t cAllocPages   = 0;
    uint64_t cFreePages    = 0;
    uint64_t cBalloonPages = 0;
    uint64_t cSharedPages  = 0;
    if (!SUPR3IsDriverless())
    {
        int rc = GMMR3QueryHypervisorMemoryStats(pUVM->pVM, &cAllocPages, &cFreePages, &cBalloonPages, &cSharedPages);
        AssertRCReturn(rc, rc);
    }

    if (pcbAllocMem)
        *pcbAllocMem    = cAllocPages * _4K;

    if (pcbFreeMem)
        *pcbFreeMem     = cFreePages * _4K;

    if (pcbBallonedMem)
        *pcbBallonedMem = cBalloonPages * _4K;

    if (pcbSharedMem)
        *pcbSharedMem   = cSharedPages * _4K;

    Log(("PGMR3QueryVMMMemoryStats: all=%llx free=%llx ballooned=%llx shared=%llx\n",
         cAllocPages, cFreePages, cBalloonPages, cSharedPages));
    return VINF_SUCCESS;
}


/**
 * Query memory stats for the VM.
 *
 * @returns VBox status code.
 * @param   pUVM                The user mode VM handle.
 * @param   pcbTotalMem         Where to return total amount memory the VM may
 *                              possibly use.
 * @param   pcbPrivateMem       Where to return the amount of private memory
 *                              currently allocated.
 * @param   pcbSharedMem        Where to return the amount of actually shared
 *                              memory currently used by the VM.
 * @param   pcbZeroMem          Where to return the amount of memory backed by
 *                              zero pages.
 *
 * @remarks The total mem is normally larger than the sum of the three
 *          components.  There are two reasons for this, first the amount of
 *          shared memory is what we're sure is shared instead of what could
 *          possibly be shared with someone.  Secondly, because the total may
 *          include some pure MMIO pages that doesn't go into any of the three
 *          sub-counts.
 *
 * @todo Why do we return reused shared pages instead of anything that could
 *       potentially be shared?  Doesn't this mean the first VM gets a much
 *       lower number of shared pages?
 */
VMMR3DECL(int) PGMR3QueryMemoryStats(PUVM pUVM, uint64_t *pcbTotalMem, uint64_t *pcbPrivateMem,
                                     uint64_t *pcbSharedMem, uint64_t *pcbZeroMem)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);

    if (pcbTotalMem)
        *pcbTotalMem    = (uint64_t)pVM->pgm.s.cAllPages            * GUEST_PAGE_SIZE;

    if (pcbPrivateMem)
        *pcbPrivateMem  = (uint64_t)pVM->pgm.s.cPrivatePages        * GUEST_PAGE_SIZE;

    if (pcbSharedMem)
        *pcbSharedMem   = (uint64_t)pVM->pgm.s.cReusedSharedPages   * GUEST_PAGE_SIZE;

    if (pcbZeroMem)
        *pcbZeroMem     = (uint64_t)pVM->pgm.s.cZeroPages           * GUEST_PAGE_SIZE;

    Log(("PGMR3QueryMemoryStats: all=%x private=%x reused=%x zero=%x\n", pVM->pgm.s.cAllPages, pVM->pgm.s.cPrivatePages, pVM->pgm.s.cReusedSharedPages, pVM->pgm.s.cZeroPages));
    return VINF_SUCCESS;
}



/*********************************************************************************************************************************
*   Chunk Mappings and Page Allocation                                                                                           *
*********************************************************************************************************************************/

/**
 * Tree enumeration callback for dealing with age rollover.
 * It will perform a simple compression of the current age.
 */
static DECLCALLBACK(int) pgmR3PhysChunkAgeingRolloverCallback(PAVLU32NODECORE pNode, void *pvUser)
{
    /* Age compression - ASSUMES iNow == 4. */
    PPGMCHUNKR3MAP pChunk = (PPGMCHUNKR3MAP)pNode;
    if (pChunk->iLastUsed >= UINT32_C(0xffffff00))
        pChunk->iLastUsed = 3;
    else if (pChunk->iLastUsed >= UINT32_C(0xfffff000))
        pChunk->iLastUsed = 2;
    else if (pChunk->iLastUsed)
        pChunk->iLastUsed = 1;
    else /* iLastUsed = 0 */
        pChunk->iLastUsed = 4;

    NOREF(pvUser);
    return 0;
}


/**
 * The structure passed in the pvUser argument of pgmR3PhysChunkUnmapCandidateCallback().
 */
typedef struct PGMR3PHYSCHUNKUNMAPCB
{
    PVM                 pVM;            /**< Pointer to the VM. */
    PPGMCHUNKR3MAP      pChunk;         /**< The chunk to unmap. */
} PGMR3PHYSCHUNKUNMAPCB, *PPGMR3PHYSCHUNKUNMAPCB;


/**
 * Callback used to find the mapping that's been unused for
 * the longest time.
 */
static DECLCALLBACK(int) pgmR3PhysChunkUnmapCandidateCallback(PAVLU32NODECORE pNode, void *pvUser)
{
    PPGMCHUNKR3MAP          pChunk = (PPGMCHUNKR3MAP)pNode;
    PPGMR3PHYSCHUNKUNMAPCB  pArg   = (PPGMR3PHYSCHUNKUNMAPCB)pvUser;

    /*
     * Check for locks and compare when last used.
     */
    if (pChunk->cRefs)
        return 0;
    if (pChunk->cPermRefs)
        return 0;
    if (   pArg->pChunk
        && pChunk->iLastUsed >= pArg->pChunk->iLastUsed)
        return 0;

    /*
     * Check that it's not in any of the TLBs.
     */
    PVM pVM = pArg->pVM;
    if (   pVM->pgm.s.ChunkR3Map.Tlb.aEntries[PGM_CHUNKR3MAPTLB_IDX(pChunk->Core.Key)].idChunk
        == pChunk->Core.Key)
    {
        pChunk = NULL;
        return 0;
    }
#ifdef VBOX_STRICT
    for (unsigned i = 0; i < RT_ELEMENTS(pVM->pgm.s.ChunkR3Map.Tlb.aEntries); i++)
    {
        Assert(pVM->pgm.s.ChunkR3Map.Tlb.aEntries[i].pChunk != pChunk);
        Assert(pVM->pgm.s.ChunkR3Map.Tlb.aEntries[i].idChunk != pChunk->Core.Key);
    }
#endif

    for (unsigned i = 0; i < RT_ELEMENTS(pVM->pgm.s.PhysTlbR3.aEntries); i++)
        if (pVM->pgm.s.PhysTlbR3.aEntries[i].pMap == pChunk)
            return 0;

    pArg->pChunk = pChunk;
    return 0;
}


/**
 * Finds a good candidate for unmapping when the ring-3 mapping cache is full.
 *
 * The candidate will not be part of any TLBs, so no need to flush
 * anything afterwards.
 *
 * @returns Chunk id.
 * @param   pVM         The cross context VM structure.
 */
static int32_t pgmR3PhysChunkFindUnmapCandidate(PVM pVM)
{
    PGM_LOCK_ASSERT_OWNER(pVM);

    /*
     * Enumerate the age tree starting with the left most node.
     */
    STAM_PROFILE_START(&pVM->pgm.s.Stats.StatChunkFindCandidate, a);
    PGMR3PHYSCHUNKUNMAPCB Args;
    Args.pVM    = pVM;
    Args.pChunk = NULL;
    RTAvlU32DoWithAll(&pVM->pgm.s.ChunkR3Map.pTree, true /*fFromLeft*/, pgmR3PhysChunkUnmapCandidateCallback, &Args);
    Assert(Args.pChunk);
    if (Args.pChunk)
    {
        Assert(Args.pChunk->cRefs == 0);
        Assert(Args.pChunk->cPermRefs == 0);
        STAM_PROFILE_STOP(&pVM->pgm.s.Stats.StatChunkFindCandidate, a);
        return Args.pChunk->Core.Key;
    }

    STAM_PROFILE_STOP(&pVM->pgm.s.Stats.StatChunkFindCandidate, a);
    return INT32_MAX;
}


/**
 * Rendezvous callback used by pgmR3PhysUnmapChunk that unmaps a chunk
 *
 * This is only called on one of the EMTs while the other ones are waiting for
 * it to complete this function.
 *
 * @returns VINF_SUCCESS (VBox strict status code).
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT. Unused.
 * @param   pvUser      User pointer. Unused
 *
 */
static DECLCALLBACK(VBOXSTRICTRC) pgmR3PhysUnmapChunkRendezvous(PVM pVM, PVMCPU pVCpu, void *pvUser)
{
    int rc = VINF_SUCCESS;
    PGM_LOCK_VOID(pVM);
    NOREF(pVCpu); NOREF(pvUser);

    if (pVM->pgm.s.ChunkR3Map.c >= pVM->pgm.s.ChunkR3Map.cMax)
    {
        /* Flush the pgm pool cache; call the internal rendezvous handler as we're already in a rendezvous handler here. */
        /** @todo also not really efficient to unmap a chunk that contains PD
         *  or PT pages. */
        pgmR3PoolClearAllRendezvous(pVM, pVM->apCpusR3[0], NULL /* no need to flush the REM TLB as we already did that above */);

        /*
         * Request the ring-0 part to unmap a chunk to make space in the mapping cache.
         */
        GMMMAPUNMAPCHUNKREQ Req;
        Req.Hdr.u32Magic = SUPVMMR0REQHDR_MAGIC;
        Req.Hdr.cbReq    = sizeof(Req);
        Req.pvR3         = NULL;
        Req.idChunkMap   = NIL_GMM_CHUNKID;
        Req.idChunkUnmap = pgmR3PhysChunkFindUnmapCandidate(pVM);
        if (Req.idChunkUnmap != INT32_MAX)
        {
            STAM_PROFILE_START(&pVM->pgm.s.Stats.StatChunkUnmap, a);
            rc = VMMR3CallR0(pVM, VMMR0_DO_GMM_MAP_UNMAP_CHUNK, 0, &Req.Hdr);
            STAM_PROFILE_STOP(&pVM->pgm.s.Stats.StatChunkUnmap, a);
            if (RT_SUCCESS(rc))
            {
                /*
                 * Remove the unmapped one.
                 */
                PPGMCHUNKR3MAP pUnmappedChunk = (PPGMCHUNKR3MAP)RTAvlU32Remove(&pVM->pgm.s.ChunkR3Map.pTree, Req.idChunkUnmap);
                AssertRelease(pUnmappedChunk);
                AssertRelease(!pUnmappedChunk->cRefs);
                AssertRelease(!pUnmappedChunk->cPermRefs);
                pUnmappedChunk->pv       = NULL;
                pUnmappedChunk->Core.Key = UINT32_MAX;
                MMR3HeapFree(pUnmappedChunk);
                pVM->pgm.s.ChunkR3Map.c--;
                pVM->pgm.s.cUnmappedChunks++;

                /*
                 * Flush dangling PGM pointers (R3 & R0 ptrs to GC physical addresses).
                 */
                /** @todo We should not flush chunks which include cr3 mappings. */
                for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
                {
                    PPGMCPU pPGM = &pVM->apCpusR3[idCpu]->pgm.s;

                    pPGM->pGst32BitPdR3    = NULL;
                    pPGM->pGstPaePdptR3    = NULL;
                    pPGM->pGstAmd64Pml4R3  = NULL;
                    pPGM->pGstEptPml4R3    = NULL;
                    pPGM->pGst32BitPdR0    = NIL_RTR0PTR;
                    pPGM->pGstPaePdptR0    = NIL_RTR0PTR;
                    pPGM->pGstAmd64Pml4R0  = NIL_RTR0PTR;
                    pPGM->pGstEptPml4R0    = NIL_RTR0PTR;
                    for (unsigned i = 0; i < RT_ELEMENTS(pPGM->apGstPaePDsR3); i++)
                    {
                        pPGM->apGstPaePDsR3[i] = NULL;
                        pPGM->apGstPaePDsR0[i] = NIL_RTR0PTR;
                    }

                    /* Flush REM TLBs. */
                    CPUMSetChangedFlags(pVM->apCpusR3[idCpu], CPUM_CHANGED_GLOBAL_TLB_FLUSH);
                }
            }
        }
    }
    PGM_UNLOCK(pVM);
    return rc;
}

/**
 * Unmap a chunk to free up virtual address space (request packet handler for pgmR3PhysChunkMap)
 *
 * @param   pVM         The cross context VM structure.
 */
static DECLCALLBACK(void) pgmR3PhysUnmapChunk(PVM pVM)
{
    int rc = VMMR3EmtRendezvous(pVM, VMMEMTRENDEZVOUS_FLAGS_TYPE_ONCE, pgmR3PhysUnmapChunkRendezvous, NULL);
    AssertRC(rc);
}


/**
 * Maps the given chunk into the ring-3 mapping cache.
 *
 * This will call ring-0.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   idChunk     The chunk in question.
 * @param   ppChunk     Where to store the chunk tracking structure.
 *
 * @remarks Called from within the PGM critical section.
 * @remarks Can be called from any thread!
 */
int pgmR3PhysChunkMap(PVM pVM, uint32_t idChunk, PPPGMCHUNKR3MAP ppChunk)
{
    int rc;

    PGM_LOCK_ASSERT_OWNER(pVM);

    /*
     * Move the chunk time forward.
     */
    pVM->pgm.s.ChunkR3Map.iNow++;
    if (pVM->pgm.s.ChunkR3Map.iNow == 0)
    {
        pVM->pgm.s.ChunkR3Map.iNow = 4;
        RTAvlU32DoWithAll(&pVM->pgm.s.ChunkR3Map.pTree, true /*fFromLeft*/, pgmR3PhysChunkAgeingRolloverCallback, NULL);
    }

    /*
     * Allocate a new tracking structure first.
     */
    PPGMCHUNKR3MAP pChunk = (PPGMCHUNKR3MAP)MMR3HeapAllocZ(pVM, MM_TAG_PGM_CHUNK_MAPPING, sizeof(*pChunk));
    AssertReturn(pChunk, VERR_NO_MEMORY);
    pChunk->Core.Key  = idChunk;
    pChunk->iLastUsed = pVM->pgm.s.ChunkR3Map.iNow;

    /*
     * Request the ring-0 part to map the chunk in question.
     */
    GMMMAPUNMAPCHUNKREQ Req;
    Req.Hdr.u32Magic = SUPVMMR0REQHDR_MAGIC;
    Req.Hdr.cbReq    = sizeof(Req);
    Req.pvR3         = NULL;
    Req.idChunkMap   = idChunk;
    Req.idChunkUnmap = NIL_GMM_CHUNKID;

    /* Must be callable from any thread, so can't use VMMR3CallR0. */
    STAM_PROFILE_START(&pVM->pgm.s.Stats.StatChunkMap, a);
    rc = SUPR3CallVMMR0Ex(VMCC_GET_VMR0_FOR_CALL(pVM), NIL_VMCPUID, VMMR0_DO_GMM_MAP_UNMAP_CHUNK, 0, &Req.Hdr);
    STAM_PROFILE_STOP(&pVM->pgm.s.Stats.StatChunkMap, a);
    if (RT_SUCCESS(rc))
    {
        pChunk->pv = Req.pvR3;

        /*
         * If we're running out of virtual address space, then we should
         * unmap another chunk.
         *
         * Currently, an unmap operation requires that all other virtual CPUs
         * are idling and not by chance making use of the memory we're
         * unmapping.  So, we create an async unmap operation here.
         *
         * Now, when creating or restoring a saved state this wont work very
         * well since we may want to restore all guest RAM + a little something.
         * So, we have to do the unmap synchronously.  Fortunately for us
         * though, during these operations the other virtual CPUs are inactive
         * and it should be safe to do this.
         */
        /** @todo Eventually we should lock all memory when used and do
         *        map+unmap as one kernel call without any rendezvous or
         *        other precautions. */
        if (pVM->pgm.s.ChunkR3Map.c + 1 >= pVM->pgm.s.ChunkR3Map.cMax)
        {
            switch (VMR3GetState(pVM))
            {
                case VMSTATE_LOADING:
                case VMSTATE_SAVING:
                {
                    PVMCPU pVCpu = VMMGetCpu(pVM);
                    if (   pVCpu
                        && pVM->pgm.s.cDeprecatedPageLocks == 0)
                    {
                        pgmR3PhysUnmapChunkRendezvous(pVM, pVCpu, NULL);
                        break;
                    }
                }
                RT_FALL_THRU();
                default:
                    rc = VMR3ReqCallNoWait(pVM, VMCPUID_ANY_QUEUE, (PFNRT)pgmR3PhysUnmapChunk, 1, pVM);
                    AssertRC(rc);
                    break;
            }
        }

        /*
         * Update the tree.  We must do this after any unmapping to make sure
         * the chunk we're going to return isn't unmapped by accident.
         */
        AssertPtr(Req.pvR3);
        bool fRc = RTAvlU32Insert(&pVM->pgm.s.ChunkR3Map.pTree, &pChunk->Core);
        AssertRelease(fRc);
        pVM->pgm.s.ChunkR3Map.c++;
        pVM->pgm.s.cMappedChunks++;
    }
    else
    {
        /** @todo this may fail because of /proc/sys/vm/max_map_count, so we
         *        should probably restrict ourselves on linux. */
        AssertRC(rc);
        MMR3HeapFree(pChunk);
        pChunk = NULL;
    }

    *ppChunk = pChunk;
    return rc;
}


/**
 * Invalidates the TLB for the ring-3 mapping cache.
 *
 * @param   pVM         The cross context VM structure.
 */
VMMR3DECL(void) PGMR3PhysChunkInvalidateTLB(PVM pVM)
{
    PGM_LOCK_VOID(pVM);
    for (unsigned i = 0; i < RT_ELEMENTS(pVM->pgm.s.ChunkR3Map.Tlb.aEntries); i++)
    {
        pVM->pgm.s.ChunkR3Map.Tlb.aEntries[i].idChunk = NIL_GMM_CHUNKID;
        pVM->pgm.s.ChunkR3Map.Tlb.aEntries[i].pChunk = NULL;
    }
    /* The page map TLB references chunks, so invalidate that one too. */
    pgmPhysInvalidatePageMapTLB(pVM);
    PGM_UNLOCK(pVM);
}


/**
 * Response to VM_FF_PGM_NEED_HANDY_PAGES and helper for pgmPhysEnsureHandyPage.
 *
 * This function will also work the VM_FF_PGM_NO_MEMORY force action flag, to
 * signal and clear the out of memory condition.  When called, this API is used
 * to try clear the condition when the user wants to resume.
 *
 * @returns The following VBox status codes.
 * @retval  VINF_SUCCESS on success. FFs cleared.
 * @retval  VINF_EM_NO_MEMORY if we're out of memory. The FF is not cleared in
 *          this case and it gets accompanied by VM_FF_PGM_NO_MEMORY.
 *
 * @param   pVM         The cross context VM structure.
 *
 * @remarks The VINF_EM_NO_MEMORY status is for the benefit of the FF processing
 *          in EM.cpp and shouldn't be propagated outside TRPM, HM, EM and
 *          pgmPhysEnsureHandyPage. There is one exception to this in the \#PF
 *          handler.
 */
VMMR3DECL(int) PGMR3PhysAllocateHandyPages(PVM pVM)
{
    PGM_LOCK_VOID(pVM);

    /*
     * Allocate more pages, noting down the index of the first new page.
     */
    uint32_t iClear = pVM->pgm.s.cHandyPages;
    AssertMsgReturn(iClear <= RT_ELEMENTS(pVM->pgm.s.aHandyPages), ("%d", iClear), VERR_PGM_HANDY_PAGE_IPE);
    Log(("PGMR3PhysAllocateHandyPages: %d -> %d\n", iClear, RT_ELEMENTS(pVM->pgm.s.aHandyPages)));
    int rc = VMMR3CallR0(pVM, VMMR0_DO_PGM_ALLOCATE_HANDY_PAGES, 0, NULL);
    /** @todo we should split this up into an allocate and flush operation. sometimes you want to flush and not allocate more (which will trigger the vm account limit error) */
    if (    rc == VERR_GMM_HIT_VM_ACCOUNT_LIMIT
        &&  pVM->pgm.s.cHandyPages > 0)
    {
        /* Still handy pages left, so don't panic. */
        rc = VINF_SUCCESS;
    }

    if (RT_SUCCESS(rc))
    {
        AssertMsg(rc == VINF_SUCCESS, ("%Rrc\n", rc));
        Assert(pVM->pgm.s.cHandyPages > 0);
#ifdef VBOX_STRICT
        uint32_t i;
        for (i = iClear; i < pVM->pgm.s.cHandyPages; i++)
            if (   pVM->pgm.s.aHandyPages[i].idPage == NIL_GMM_PAGEID
                || pVM->pgm.s.aHandyPages[i].idSharedPage != NIL_GMM_PAGEID
                || (pVM->pgm.s.aHandyPages[i].HCPhysGCPhys & GUEST_PAGE_OFFSET_MASK))
                break;
        if (i != pVM->pgm.s.cHandyPages)
        {
            RTAssertMsg1Weak(NULL, __LINE__, __FILE__, __FUNCTION__);
            RTAssertMsg2Weak("i=%d iClear=%d cHandyPages=%d\n", i, iClear, pVM->pgm.s.cHandyPages);
            for (uint32_t j = iClear; j < pVM->pgm.s.cHandyPages; j++)
                RTAssertMsg2Add("%03d: idPage=%d HCPhysGCPhys=%RHp idSharedPage=%d%s\n", j,
                                pVM->pgm.s.aHandyPages[j].idPage,
                                pVM->pgm.s.aHandyPages[j].HCPhysGCPhys,
                                pVM->pgm.s.aHandyPages[j].idSharedPage,
                                j == i ? " <---" : "");
            RTAssertPanic();
        }
#endif
    }
    else
    {
        /*
         * We should never get here unless there is a genuine shortage of
         * memory (or some internal error). Flag the error so the VM can be
         * suspended ASAP and the user informed. If we're totally out of
         * handy pages we will return failure.
         */
        /* Report the failure. */
        LogRel(("PGM: Failed to procure handy pages; rc=%Rrc cHandyPages=%#x\n"
                "     cAllPages=%#x cPrivatePages=%#x cSharedPages=%#x cZeroPages=%#x\n",
                rc, pVM->pgm.s.cHandyPages,
                pVM->pgm.s.cAllPages, pVM->pgm.s.cPrivatePages, pVM->pgm.s.cSharedPages, pVM->pgm.s.cZeroPages));

        if (   rc != VERR_NO_MEMORY
            && rc != VERR_NO_PHYS_MEMORY
            && rc != VERR_LOCK_FAILED)
            for (uint32_t i = 0; i < RT_ELEMENTS(pVM->pgm.s.aHandyPages); i++)
            {
                LogRel(("PGM: aHandyPages[#%#04x] = {.HCPhysGCPhys=%RHp, .idPage=%#08x, .idSharedPage=%#08x}\n",
                        i, pVM->pgm.s.aHandyPages[i].HCPhysGCPhys, pVM->pgm.s.aHandyPages[i].idPage,
                        pVM->pgm.s.aHandyPages[i].idSharedPage));
                uint32_t const idPage = pVM->pgm.s.aHandyPages[i].idPage;
                if (idPage != NIL_GMM_PAGEID)
                {
                    for (PPGMRAMRANGE pRam = pVM->pgm.s.pRamRangesXR3;
                         pRam;
                         pRam = pRam->pNextR3)
                    {
                        uint32_t const cPages = pRam->cb >> GUEST_PAGE_SHIFT;
                        for (uint32_t iPage = 0; iPage < cPages; iPage++)
                            if (PGM_PAGE_GET_PAGEID(&pRam->aPages[iPage]) == idPage)
                                LogRel(("PGM: Used by %RGp %R[pgmpage] (%s)\n",
                                        pRam->GCPhys + ((RTGCPHYS)iPage << GUEST_PAGE_SHIFT), &pRam->aPages[iPage], pRam->pszDesc));
                    }
                }
            }

        if (rc == VERR_NO_MEMORY)
        {
            uint64_t cbHostRamAvail = 0;
            int rc2 = RTSystemQueryAvailableRam(&cbHostRamAvail);
            if (RT_SUCCESS(rc2))
                LogRel(("Host RAM: %RU64MB available\n", cbHostRamAvail / _1M));
            else
                LogRel(("Cannot determine the amount of available host memory\n"));
        }

        /* Set the FFs and adjust rc. */
        VM_FF_SET(pVM, VM_FF_PGM_NEED_HANDY_PAGES);
        VM_FF_SET(pVM, VM_FF_PGM_NO_MEMORY);
        if (    rc == VERR_NO_MEMORY
            ||  rc == VERR_NO_PHYS_MEMORY
            ||  rc == VERR_LOCK_FAILED)
            rc = VINF_EM_NO_MEMORY;
    }

    PGM_UNLOCK(pVM);
    return rc;
}


/*********************************************************************************************************************************
*   Other Stuff                                                                                                                  *
*********************************************************************************************************************************/

/**
 * Sets the Address Gate 20 state.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   fEnable     True if the gate should be enabled.
 *                      False if the gate should be disabled.
 */
VMMDECL(void) PGMR3PhysSetA20(PVMCPU pVCpu, bool fEnable)
{
    LogFlow(("PGMR3PhysSetA20 %d (was %d)\n", fEnable, pVCpu->pgm.s.fA20Enabled));
    if (pVCpu->pgm.s.fA20Enabled != fEnable)
    {
#ifdef VBOX_WITH_NESTED_HWVIRT_VMX
        PCCPUMCTX pCtx = CPUMQueryGuestCtxPtr(pVCpu);
        if (   CPUMIsGuestInVmxRootMode(pCtx)
            && !fEnable)
        {
            Log(("Cannot enter A20M mode while in VMX root mode\n"));
            return;
        }
#endif
        pVCpu->pgm.s.fA20Enabled = fEnable;
        pVCpu->pgm.s.GCPhysA20Mask = ~((RTGCPHYS)!fEnable << 20);
        if (VM_IS_NEM_ENABLED(pVCpu->CTX_SUFF(pVM)))
            NEMR3NotifySetA20(pVCpu, fEnable);
#ifdef PGM_WITH_A20
        VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3);
        pgmR3RefreshShadowModeAfterA20Change(pVCpu);
        HMFlushTlb(pVCpu);
#endif
#if 0 /* PGMGetPage will apply the A20 mask to the GCPhys it returns, so we must invalid both sides of the TLB. */
        IEMTlbInvalidateAllPhysical(pVCpu);
#else
        IEMTlbInvalidateAll(pVCpu);
#endif
        STAM_REL_COUNTER_INC(&pVCpu->pgm.s.cA20Changes);
    }
}

