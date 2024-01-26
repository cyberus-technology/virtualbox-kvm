/** @file
 * VirtualBox Guest Additions Driver for Solaris - Solaris helper functions.
 */

/*
 * Copyright (C) 2012-2023 Oracle and/or its affiliates.
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

#include "solaris.h"
#include <iprt/alloc.h>


/*********************************************************************************************************************************
*   Helper functions                                                                                                             *
*********************************************************************************************************************************/

void miocack(queue_t *pWriteQueue, mblk_t *pMBlk, int cbData, int rc)
{
    struct iocblk *pIOCBlk = (struct iocblk *)pMBlk->b_rptr;

    pMBlk->b_datap->db_type = M_IOCACK;
    pIOCBlk->ioc_count = cbData;
    pIOCBlk->ioc_rval = rc;
    pIOCBlk->ioc_error = 0;
    qreply(pWriteQueue, pMBlk);
}

void miocnak(queue_t *pWriteQueue, mblk_t *pMBlk, int cbData, int iErr)
{
    struct iocblk *pIOCBlk = (struct iocblk *)pMBlk->b_rptr;

    pMBlk->b_datap->db_type = M_IOCNAK;
    pIOCBlk->ioc_count = cbData;
    pIOCBlk->ioc_error = iErr ? iErr : EINVAL;
    pIOCBlk->ioc_rval = 0;
    qreply(pWriteQueue, pMBlk);
}

/* This does not work like the real version, but does some sanity testing
 * and sets a flag. */
int miocpullup(mblk_t *pMBlk, size_t cbMsg)
{
    struct iocblk *pIOCBlk = (struct iocblk *)pMBlk->b_rptr;

    if (pIOCBlk->ioc_count == TRANSPARENT)
        return EINVAL;
    if (   !pMBlk->b_cont
        || pMBlk->b_cont->b_wptr < pMBlk->b_cont->b_rptr + cbMsg)
        return EINVAL;
    pMBlk->b_flag |= F_TEST_PULLUP;
    return 0;
}

void mcopyin(mblk_t *pMBlk, void *pvState, size_t cbData, void *pvUser)
{
    struct iocblk *pIOCBlk = (struct iocblk *)pMBlk->b_rptr;
    struct copyreq *pCopyReq = (struct copyreq *)pMBlk->b_rptr;

    AssertReturnVoid(   pvUser
                     || (   pMBlk->b_datap->db_type == M_IOCTL
                         && pIOCBlk->ioc_count == TRANSPARENT
                         && pMBlk->b_cont->b_rptr));
    pMBlk->b_datap->db_type = M_COPYIN;
    pMBlk->b_wptr = pMBlk->b_rptr + sizeof(*pCopyReq);
    pCopyReq->cq_private = pvState;
    pCopyReq->cq_size = cbData;
    pCopyReq->cq_addr = pvUser ? pvUser : *(void **)pMBlk->b_cont->b_rptr;
    if (pMBlk->b_cont)
    {
        freemsg(pMBlk->b_cont);
        pMBlk->b_cont = NULL;
    }
}

void mcopyout(mblk_t *pMBlk, void *pvState, size_t cbData, void *pvUser,
              mblk_t *pMBlkData)
{
    struct iocblk *pIOCBlk = (struct iocblk *)pMBlk->b_rptr;
    struct copyreq *pCopyReq = (struct copyreq *)pMBlk->b_rptr;

    AssertReturnVoid(   pvUser
                     || (   pMBlk->b_datap->db_type == M_IOCTL
                         && pIOCBlk->ioc_count == TRANSPARENT
                         && pMBlk->b_cont->b_rptr));
    pMBlk->b_datap->db_type = M_COPYOUT;
    pMBlk->b_wptr = pMBlk->b_rptr + sizeof(*pCopyReq);
    pCopyReq->cq_private = pvState;
    pCopyReq->cq_size = cbData;
    pCopyReq->cq_addr = pvUser ? pvUser : *(void **)pMBlk->b_cont->b_rptr;
    if (pMBlkData)
    {
        if (pMBlk->b_cont)
            freemsg(pMBlk->b_cont);
        pMBlk->b_cont = pMBlkData;
        pMBlkData->b_wptr = pMBlkData->b_rptr + cbData;
    }
}

/* This does not work like the real version but is easy to test the result of.
 */
void qreply(queue_t *pQueue, mblk_t *pMBlk)
{
    OTHERQ(pQueue)->q_first = pMBlk;
}

/** @todo reference counting */
mblk_t *allocb(size_t cb, uint_t cPrio)
{
    unsigned char *pch = RTMemAllocZ(cb);
    struct msgb *pMBlk = (struct msgb *)RTMemAllocZ(sizeof(struct msgb));
    struct datab *pDBlk = (struct datab *)RTMemAllocZ(sizeof(struct datab));
    if (!pch || !pMBlk || !pDBlk)
    {
        RTMemFree(pch);
        RTMemFree(pMBlk);
        RTMemFree(pDBlk);
        return NULL;
    }
    NOREF(cPrio);
    pMBlk->b_rptr = pch;
    pMBlk->b_wptr = pMBlk->b_rptr + cb;
    pMBlk->b_datap = pDBlk;
    pDBlk->db_base = pMBlk->b_rptr;
    pDBlk->db_lim = pMBlk->b_wptr;
    pDBlk->db_type = M_DATA;
    return pMBlk;
}

/** @todo reference counting */
void freemsg(mblk_t *pMBlk)
{
    if (!pMBlk)
        return;
    RTMemFree(pMBlk->b_rptr);
    RTMemFree(pMBlk->b_datap);
    freemsg(pMBlk->b_cont);
    RTMemFree(pMBlk);
}
