/* $Id: sg.cpp $ */
/** @file
 * IPRT - S/G buffer handling.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/sg.h>
#include <iprt/string.h>
#include <iprt/assert.h>
#include <iprt/asm.h>


static void *rtSgBufGet(PRTSGBUF pSgBuf, size_t *pcbData)
{
    size_t cbData;
    void *pvBuf;

    /* Check that the S/G buffer has memory left. */
    if (RT_UNLIKELY(   pSgBuf->idxSeg == pSgBuf->cSegs
                    && !pSgBuf->cbSegLeft))
    {
        *pcbData = 0;
        return NULL;
    }

#ifndef RDESKTOP
    AssertMsg(      pSgBuf->cbSegLeft <= 128 * _1M
              &&    (uintptr_t)pSgBuf->pvSegCur                     >= (uintptr_t)pSgBuf->paSegs[pSgBuf->idxSeg].pvSeg
              &&    (uintptr_t)pSgBuf->pvSegCur + pSgBuf->cbSegLeft <= (uintptr_t)pSgBuf->paSegs[pSgBuf->idxSeg].pvSeg + pSgBuf->paSegs[pSgBuf->idxSeg].cbSeg,
              ("pSgBuf->idxSeg=%d pSgBuf->cSegs=%d pSgBuf->pvSegCur=%p pSgBuf->cbSegLeft=%zd pSgBuf->paSegs[%d].pvSeg=%p pSgBuf->paSegs[%d].cbSeg=%zd\n",
               pSgBuf->idxSeg, pSgBuf->cSegs, pSgBuf->pvSegCur, pSgBuf->cbSegLeft, pSgBuf->idxSeg, pSgBuf->paSegs[pSgBuf->idxSeg].pvSeg, pSgBuf->idxSeg,
               pSgBuf->paSegs[pSgBuf->idxSeg].cbSeg));
#endif

    cbData = RT_MIN(*pcbData, pSgBuf->cbSegLeft);
    pvBuf  = pSgBuf->pvSegCur;
    pSgBuf->cbSegLeft -= cbData;

    /* Advance to the next segment if required. */
    if (!pSgBuf->cbSegLeft)
    {
        pSgBuf->idxSeg++;

        if (pSgBuf->idxSeg < pSgBuf->cSegs)
        {
            pSgBuf->pvSegCur  = pSgBuf->paSegs[pSgBuf->idxSeg].pvSeg;
            pSgBuf->cbSegLeft = pSgBuf->paSegs[pSgBuf->idxSeg].cbSeg;
        }

        *pcbData = cbData;
    }
    else
        pSgBuf->pvSegCur = (uint8_t *)pSgBuf->pvSegCur + cbData;

    return pvBuf;
}


RTDECL(void) RTSgBufInit(PRTSGBUF pSgBuf, PCRTSGSEG paSegs, size_t cSegs)
{
    AssertPtr(pSgBuf);
    Assert(   (cSegs > 0 && RT_VALID_PTR(paSegs))
           || (!cSegs && !paSegs));
    Assert(cSegs < (~(unsigned)0 >> 1));

    pSgBuf->paSegs    = paSegs;
    pSgBuf->cSegs     = (unsigned)cSegs;
    pSgBuf->idxSeg    = 0;
    if (cSegs && paSegs)
    {
        pSgBuf->pvSegCur  = paSegs[0].pvSeg;
        pSgBuf->cbSegLeft = paSegs[0].cbSeg;
    }
    else
    {
        pSgBuf->pvSegCur  = NULL;
        pSgBuf->cbSegLeft = 0;
    }
}


RTDECL(void) RTSgBufReset(PRTSGBUF pSgBuf)
{
    AssertPtrReturnVoid(pSgBuf);

    pSgBuf->idxSeg = 0;
    if (pSgBuf->cSegs)
    {
        pSgBuf->pvSegCur  = pSgBuf->paSegs[0].pvSeg;
        pSgBuf->cbSegLeft = pSgBuf->paSegs[0].cbSeg;
    }
    else
    {
        pSgBuf->pvSegCur  = NULL;
        pSgBuf->cbSegLeft = 0;
    }
}


RTDECL(void) RTSgBufClone(PRTSGBUF pSgBufTo, PCRTSGBUF pSgBufFrom)
{
    AssertPtr(pSgBufTo);
    AssertPtr(pSgBufFrom);

    pSgBufTo->paSegs    = pSgBufFrom->paSegs;
    pSgBufTo->cSegs     = pSgBufFrom->cSegs;
    pSgBufTo->idxSeg    = pSgBufFrom->idxSeg;
    pSgBufTo->pvSegCur  = pSgBufFrom->pvSegCur;
    pSgBufTo->cbSegLeft = pSgBufFrom->cbSegLeft;
}


RTDECL(void *) RTSgBufGetNextSegment(PRTSGBUF pSgBuf, size_t *pcbSeg)
{
    AssertPtrReturn(pSgBuf, NULL);
    AssertPtrReturn(pcbSeg, NULL);

    if (!*pcbSeg)
        *pcbSeg = pSgBuf->cbSegLeft;

    return rtSgBufGet(pSgBuf, pcbSeg);
}


RTDECL(size_t) RTSgBufCopy(PRTSGBUF pSgBufDst, PRTSGBUF pSgBufSrc, size_t cbCopy)
{
    AssertPtrReturn(pSgBufDst, 0);
    AssertPtrReturn(pSgBufSrc, 0);

    size_t cbLeft = cbCopy;
    while (cbLeft)
    {
        size_t cbThisCopy = RT_MIN(RT_MIN(pSgBufDst->cbSegLeft, cbLeft), pSgBufSrc->cbSegLeft);
        if (!cbThisCopy)
            break;

        size_t cbTmp = cbThisCopy;
        void *pvBufDst = rtSgBufGet(pSgBufDst, &cbTmp);
        Assert(cbTmp == cbThisCopy);
        void *pvBufSrc = rtSgBufGet(pSgBufSrc, &cbTmp);
        Assert(cbTmp == cbThisCopy);

        memcpy(pvBufDst, pvBufSrc, cbThisCopy);

        cbLeft -= cbThisCopy;
    }

    return cbCopy - cbLeft;
}


RTDECL(int) RTSgBufCmp(PCRTSGBUF pSgBuf1, PCRTSGBUF pSgBuf2, size_t cbCmp)
{
    AssertPtrReturn(pSgBuf1, 0);
    AssertPtrReturn(pSgBuf2, 0);

    /* Set up the temporary buffers */
    RTSGBUF SgBuf1;
    RTSgBufClone(&SgBuf1, pSgBuf1);
    RTSGBUF SgBuf2;
    RTSgBufClone(&SgBuf2, pSgBuf2);

    size_t cbLeft = cbCmp;
    while (cbLeft)
    {
        size_t cbThisCmp = RT_MIN(RT_MIN(SgBuf1.cbSegLeft, cbLeft), SgBuf2.cbSegLeft);
        if (!cbThisCmp)
            break;

        size_t cbTmp = cbThisCmp;
        void *pvBuf1 = rtSgBufGet(&SgBuf1, &cbTmp);
        Assert(cbTmp == cbThisCmp);
        void *pvBuf2 = rtSgBufGet(&SgBuf2, &cbTmp);
        Assert(cbTmp == cbThisCmp);

        int rc = memcmp(pvBuf1, pvBuf2, cbThisCmp);
        if (rc)
            return rc;

        cbLeft -= cbThisCmp;
    }

    return 0;
}


RTDECL(int) RTSgBufCmpEx(PRTSGBUF pSgBuf1, PRTSGBUF pSgBuf2, size_t cbCmp, size_t *poffDiff, bool fAdvance)
{
    AssertPtrReturn(pSgBuf1, 0);
    AssertPtrReturn(pSgBuf2, 0);

    RTSGBUF SgBuf1Tmp;
    RTSGBUF SgBuf2Tmp;
    PRTSGBUF pSgBuf1Tmp;
    PRTSGBUF pSgBuf2Tmp;

    if (!fAdvance)
    {
        /* Set up the temporary buffers */
        RTSgBufClone(&SgBuf1Tmp, pSgBuf1);
        RTSgBufClone(&SgBuf2Tmp, pSgBuf2);
        pSgBuf1Tmp = &SgBuf1Tmp;
        pSgBuf2Tmp = &SgBuf2Tmp;
    }
    else
    {
        pSgBuf1Tmp = pSgBuf1;
        pSgBuf2Tmp = pSgBuf2;
    }

    size_t cbLeft = cbCmp;
    size_t off    = 0;
    while (cbLeft)
    {
        size_t cbThisCmp = RT_MIN(RT_MIN(pSgBuf1Tmp->cbSegLeft, cbLeft), pSgBuf2Tmp->cbSegLeft);
        if (!cbThisCmp)
            break;

        size_t cbTmp = cbThisCmp;
        uint8_t *pbBuf1 = (uint8_t *)rtSgBufGet(pSgBuf1Tmp, &cbTmp);
        Assert(cbTmp == cbThisCmp);
        uint8_t *pbBuf2 = (uint8_t *)rtSgBufGet(pSgBuf2Tmp, &cbTmp);
        Assert(cbTmp == cbThisCmp);

        int iDiff = memcmp(pbBuf1, pbBuf2, cbThisCmp);
        if (iDiff)
        {
            /* Locate the first byte that differs if the caller requested this. */
            if (poffDiff)
            {
                while (   cbThisCmp-- > 0
                       && *pbBuf1 == *pbBuf2)
                {
                    pbBuf1++;
                    pbBuf2++;
                    off++;
                }

                *poffDiff = off;
            }
            return iDiff;
        }

        cbLeft -= cbThisCmp;
        off    += cbThisCmp;
    }

    return 0;
}


RTDECL(size_t) RTSgBufSet(PRTSGBUF pSgBuf, uint8_t ubFill, size_t cbSet)
{
    AssertPtrReturn(pSgBuf, 0);

    size_t cbLeft = cbSet;

    while (cbLeft)
    {
        size_t cbThisSet = cbLeft;
        void *pvBuf = rtSgBufGet(pSgBuf, &cbThisSet);

        if (!cbThisSet)
            break;

        memset(pvBuf, ubFill, cbThisSet);

        cbLeft -= cbThisSet;
    }

    return cbSet - cbLeft;
}


RTDECL(size_t) RTSgBufCopyToBuf(PRTSGBUF pSgBuf, void *pvBuf, size_t cbCopy)
{
    AssertPtrReturn(pSgBuf, 0);
    AssertPtrReturn(pvBuf, 0);

    size_t cbLeft = cbCopy;

    while (cbLeft)
    {
        size_t cbThisCopy = cbLeft;
        void *pvSrc = rtSgBufGet(pSgBuf, &cbThisCopy);

        if (!cbThisCopy)
            break;

        memcpy(pvBuf, pvSrc, cbThisCopy);

        cbLeft -= cbThisCopy;
        pvBuf = (void *)((uintptr_t)pvBuf + cbThisCopy);
    }

    return cbCopy - cbLeft;
}


RTDECL(size_t) RTSgBufCopyFromBuf(PRTSGBUF pSgBuf, const void *pvBuf, size_t cbCopy)
{
    AssertPtrReturn(pSgBuf, 0);
    AssertPtrReturn(pvBuf, 0);

    size_t cbLeft = cbCopy;

    while (cbLeft)
    {
        size_t cbThisCopy = cbLeft;
        void *pvDst = rtSgBufGet(pSgBuf, &cbThisCopy);

        if (!cbThisCopy)
            break;

        memcpy(pvDst, pvBuf, cbThisCopy);

        cbLeft -= cbThisCopy;
        pvBuf = (const void *)((uintptr_t)pvBuf + cbThisCopy);
    }

    return cbCopy - cbLeft;
}


RTDECL(size_t) RTSgBufCopyToFn(PRTSGBUF pSgBuf, size_t cbCopy, PFNRTSGBUFCOPYTO pfnCopyTo, void *pvUser)
{
    AssertPtrReturn(pSgBuf, 0);
    AssertPtrReturn(pfnCopyTo, 0);

    size_t cbLeft = cbCopy;

    while (cbLeft)
    {
        size_t cbThisCopy = cbLeft;
        void *pvSrc = rtSgBufGet(pSgBuf, &cbThisCopy);

        if (!cbThisCopy)
            break;

        size_t cbThisCopied = pfnCopyTo(pSgBuf, pvSrc, cbThisCopy, pvUser);
        cbLeft -= cbThisCopied;
        if (cbThisCopied < cbThisCopy)
            break;
    }

    return cbCopy - cbLeft;
}


RTDECL(size_t) RTSgBufCopyFromFn(PRTSGBUF pSgBuf, size_t cbCopy, PFNRTSGBUFCOPYFROM pfnCopyFrom, void *pvUser)
{
    AssertPtrReturn(pSgBuf, 0);
    AssertPtrReturn(pfnCopyFrom, 0);

    size_t cbLeft = cbCopy;

    while (cbLeft)
    {
        size_t cbThisCopy = cbLeft;
        void *pvDst = rtSgBufGet(pSgBuf, &cbThisCopy);

        if (!cbThisCopy)
            break;

        size_t cbThisCopied = pfnCopyFrom(pSgBuf, pvDst, cbThisCopy, pvUser);
        cbLeft -= cbThisCopied;
        if (cbThisCopied < cbThisCopy)
            break;
    }

    return cbCopy - cbLeft;
}


RTDECL(size_t) RTSgBufAdvance(PRTSGBUF pSgBuf, size_t cbAdvance)
{
    AssertPtrReturn(pSgBuf, 0);

    size_t cbLeft = cbAdvance;
    while (cbLeft)
    {
        size_t cbThisAdvance = cbLeft;
        rtSgBufGet(pSgBuf, &cbThisAdvance);
        if (!cbThisAdvance)
            break;

        cbLeft -= cbThisAdvance;
    }

    return cbAdvance - cbLeft;
}


RTDECL(size_t) RTSgBufSegArrayCreate(PRTSGBUF pSgBuf, PRTSGSEG paSeg, unsigned *pcSeg, size_t cbData)
{
    AssertPtrReturn(pSgBuf, 0);
    AssertPtrReturn(pcSeg, 0);

    unsigned cSeg = 0;
    size_t   cb = 0;

    if (!paSeg)
    {
        if (pSgBuf->cbSegLeft > 0)
        {
            size_t idx = pSgBuf->idxSeg;
            cSeg = 1;

            cb     += RT_MIN(pSgBuf->cbSegLeft, cbData);
            cbData -= RT_MIN(pSgBuf->cbSegLeft, cbData);

            while (   cbData
                   && idx < pSgBuf->cSegs - 1)
            {
                idx++;
                cSeg++;
                cb     += RT_MIN(pSgBuf->paSegs[idx].cbSeg, cbData);
                cbData -= RT_MIN(pSgBuf->paSegs[idx].cbSeg, cbData);
            }
        }
    }
    else
    {
        while (   cbData
               && cSeg < *pcSeg)
        {
            size_t  cbThisSeg = cbData;
            void *pvSeg = rtSgBufGet(pSgBuf, &cbThisSeg);

            if (!cbThisSeg)
            {
                Assert(!pvSeg);
                break;
            }

            AssertMsg(cbThisSeg <= cbData, ("Impossible!\n"));

            paSeg[cSeg].cbSeg = cbThisSeg;
            paSeg[cSeg].pvSeg = pvSeg;
            cSeg++;
            cbData -= cbThisSeg;
            cb     += cbThisSeg;
        }
    }

    *pcSeg = cSeg;

    return cb;
}


RTDECL(bool) RTSgBufIsZero(PRTSGBUF pSgBuf, size_t cbCheck)
{
    RTSGBUF SgBufTmp;
    RTSgBufClone(&SgBufTmp, pSgBuf);

    bool   fIsZero = true;
    size_t cbLeft  = cbCheck;
    while (cbLeft)
    {
        size_t cbThisCheck = cbLeft;
        void *pvBuf = rtSgBufGet(&SgBufTmp, &cbThisCheck);
        if (!cbThisCheck)
            break;
        fIsZero = ASMMemIsZero(pvBuf, cbThisCheck);
        if (!fIsZero)
            break;
        cbLeft -= cbThisCheck;
    }

    return fIsZero;
}

