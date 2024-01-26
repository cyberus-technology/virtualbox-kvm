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

#ifndef IPRT_INCLUDED_sg_h
#define IPRT_INCLUDED_sg_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_sgbuf  RTSgBuf - Scatter / Gather Buffers
 * @ingroup grp_rt
 * @{
 */

/** Pointer to a const S/G entry. */
typedef const struct RTSGBUF *PCRTSGBUF;

/**
 * Callback for RTSgBufCopyToFn() called on every segment of the given S/G buffer.
 *
 * @returns Number of bytes copied for this segment, a value smaller than cbSrc will stop the copy operation.
 * @param   pSgBuf          The S/G buffer for reference.
 * @param   pvSrc           Where to copy from.
 * @param   cbSrc           The number of bytes in the source buffer.
 * @param   pvUser          Opaque user data passed in RTSgBufCopyToFn().
 */
typedef DECLCALLBACKTYPE(size_t, FNRTSGBUFCOPYTO, (PCRTSGBUF pSgBuf, const void *pvSrc, size_t cbSrc, void *pvUser));
/** Pointer to a FNRTSGBUFCOPYTO. */
typedef FNRTSGBUFCOPYTO *PFNRTSGBUFCOPYTO;

/**
 * Callback for RTSgBufCopyFromFn() called on every segment of the given S/G buffer.
 *
 * @returns Number of bytes copied for this segment, a value smaller than cbDst will stop the copy operation.
 * @param   pSgBuf          The S/G buffer for reference.
 * @param   pvDst           Where to copy to.
 * @param   cbDst           The number of bytes in the destination buffer.
 * @param   pvUser          Opaque user data passed in RTSgBufCopyFromFn().
 */
typedef DECLCALLBACKTYPE(size_t, FNRTSGBUFCOPYFROM, (PCRTSGBUF pSgBuf, void *pvDst, size_t cbDst, void *pvUser));
/** Pointer to a FNRTSGBUFCOPYFROM. */
typedef FNRTSGBUFCOPYFROM *PFNRTSGBUFCOPYFROM;

/**
 * A S/G entry.
 */
typedef struct RTSGSEG
{
    /** Pointer to the segment buffer. */
    void   *pvSeg;
    /** Size of the segment buffer. */
    size_t  cbSeg;
} RTSGSEG;
/** Pointer to a S/G entry. */
typedef RTSGSEG *PRTSGSEG;
/** Pointer to a const S/G entry. */
typedef const RTSGSEG *PCRTSGSEG;
/** Pointer to a S/G entry pointer. */
typedef PRTSGSEG *PPRTSGSEG;

/**
 * A S/G buffer.
 *
 * The members should be treated as private.
 *
 * @warning There is a lot of code, especially in the VFS area of IPRT, that
 *          totally ignores the idxSeg, pvSegCur and cbSegLeft members!  So,
 *          it is not recommended to pass buffers that aren't fully reset or
 *          where cbSegLeft is shorter than what paSegs describes.
 */
typedef struct RTSGBUF
{
    /** Pointer to the scatter/gather array. */
    PCRTSGSEG paSegs;
    /** Number of segments. */
    unsigned  cSegs;

    /** Current segment we are in. */
    unsigned  idxSeg;
    /** Pointer to current byte within the current segment. */
    void     *pvSegCur;
    /** Number of bytes left in the current segment. */
    size_t    cbSegLeft;
} RTSGBUF;
/** Pointer to a S/G entry. */
typedef RTSGBUF *PRTSGBUF;
/** Pointer to a S/G entry pointer. */
typedef PRTSGBUF *PPRTSGBUF;


/**
 * Sums up the length of all the segments.
 *
 * @returns The complete segment length.
 * @param   pSgBuf      The S/G buffer to check out.
 */
DECLINLINE(size_t) RTSgBufCalcTotalLength(PCRTSGBUF pSgBuf)
{
    size_t   cb = 0;
    unsigned i  = pSgBuf->cSegs;
    while (i-- > 0)
        cb += pSgBuf->paSegs[i].cbSeg;
    return cb;
}

/**
 * Sums up the number of bytes left from the current position.
 *
 * @returns Number of bytes left.
 * @param   pSgBuf      The S/G buffer to check out.
 */
DECLINLINE(size_t) RTSgBufCalcLengthLeft(PCRTSGBUF pSgBuf)
{
    size_t   cb = pSgBuf->cbSegLeft;
    unsigned i  = pSgBuf->cSegs;
    while (i-- > pSgBuf->idxSeg + 1)
        cb += pSgBuf->paSegs[i].cbSeg;
    return cb;
}

/**
 * Checks if the current buffer position is at the start of the first segment.
 *
 * @returns true / false.
 * @param   pSgBuf      The S/G buffer to check out.
 */
DECLINLINE(bool) RTSgBufIsAtStart(PCRTSGBUF pSgBuf)
{
    return pSgBuf->idxSeg == 0
        && (   pSgBuf->cSegs == 0
            || pSgBuf->pvSegCur == pSgBuf->paSegs[0].pvSeg);
}

/**
 * Checks if the current buffer position is at the end of all the segments.
 *
 * @returns true / false.
 * @param   pSgBuf      The S/G buffer to check out.
 */
DECLINLINE(bool) RTSgBufIsAtEnd(PCRTSGBUF pSgBuf)
{
    return pSgBuf->idxSeg > pSgBuf->cSegs
        || (   pSgBuf->idxSeg == pSgBuf->cSegs
            && pSgBuf->cbSegLeft == 0);
}

/**
 * Checks if the current buffer position is at the start of the current segment.
 *
 * @returns true / false.
 * @param   pSgBuf      The S/G buffer to check out.
 */
DECLINLINE(bool) RTSgBufIsAtStartOfSegment(PCRTSGBUF pSgBuf)
{
    return pSgBuf->idxSeg < pSgBuf->cSegs
        && pSgBuf->paSegs[pSgBuf->idxSeg].pvSeg == pSgBuf->pvSegCur;
}

/**
 * Initialize a S/G buffer structure.
 *
 * @param   pSgBuf    Pointer to the S/G buffer to initialize.
 * @param   paSegs    Pointer to the start of the segment array.
 * @param   cSegs     Number of segments in the array.
 *
 * @note paSegs and cSegs can be NULL and 0 respectively to indicate an empty
 *       S/G buffer.  Operations on the S/G buffer will not do anything in this
 *       case.
 */
RTDECL(void) RTSgBufInit(PRTSGBUF pSgBuf, PCRTSGSEG paSegs, size_t cSegs);

/**
 * Resets the internal buffer position of the S/G buffer to the beginning.
 *
 * @param   pSgBuf    The S/G buffer to reset.
 */
RTDECL(void) RTSgBufReset(PRTSGBUF pSgBuf);

/**
 * Clones a given S/G buffer.
 *
 * @param   pSgBufNew    The new S/G buffer to clone to.
 * @param   pSgBufOld    The source S/G buffer to clone from.
 *
 * @note This is only a shallow copy. Both S/G buffers will point to the
 *       same segment array.
 */
RTDECL(void) RTSgBufClone(PRTSGBUF pSgBufNew, PCRTSGBUF pSgBufOld);

/**
 * Returns the next segment in the S/G buffer or NULL if no segments left.
 *
 * @returns Pointer to the next segment in the S/G buffer.
 * @param   pSgBuf      The S/G buffer.
 * @param   cbDesired   The max number of bytes to get.
 * @param   pcbSeg      Where to store the size of the returned segment, this is
 *                      equal or smaller than @a cbDesired.
 *
 * @note    Use RTSgBufAdvance() to advance after read/writing into the buffer.
 */
DECLINLINE(void *) RTSgBufGetCurrentSegment(PRTSGBUF pSgBuf, size_t cbDesired, size_t *pcbSeg)
{
    if (!RTSgBufIsAtEnd(pSgBuf))
    {
        *pcbSeg = RT_MIN(cbDesired, pSgBuf->cbSegLeft);
        return pSgBuf->pvSegCur;
    }
    *pcbSeg = 0;
    return NULL;
}

/**
 * Returns the next segment in the S/G buffer or NULL if no segment is left.
 *
 * @returns Pointer to the next segment in the S/G buffer.
 * @param   pSgBuf       The S/G buffer.
 * @param   pcbSeg       Where to store the size of the returned segment.
 *                       Holds the number of bytes requested initially or 0 to
 *                       indicate that the size doesn't matter.
 *                       This may contain fewer bytes on success if the current segment
 *                       is smaller than the amount of bytes requested.
 *
 * @note This operation advances the internal buffer pointer of both S/G buffers.
 */
RTDECL(void *) RTSgBufGetNextSegment(PRTSGBUF pSgBuf, size_t *pcbSeg);

/**
 * Copy data between two S/G buffers.
 *
 * @returns The number of bytes copied.
 * @param   pSgBufDst    The destination S/G buffer.
 * @param   pSgBufSrc    The source S/G buffer.
 * @param   cbCopy       Number of bytes to copy.
 *
 * @note This operation advances the internal buffer pointer of both S/G buffers.
 */
RTDECL(size_t) RTSgBufCopy(PRTSGBUF pSgBufDst, PRTSGBUF pSgBufSrc, size_t cbCopy);

/**
 * Compares the content of two S/G buffers.
 *
 * @returns Whatever memcmp returns.
 * @param   pSgBuf1      First S/G buffer.
 * @param   pSgBuf2      Second S/G buffer.
 * @param   cbCmp        How many bytes to compare.
 *
 * @note This operation doesn't change the internal position of the S/G buffers.
 */
RTDECL(int) RTSgBufCmp(PCRTSGBUF pSgBuf1, PCRTSGBUF pSgBuf2, size_t cbCmp);

/**
 * Compares the content of two S/G buffers - advanced version.
 *
 * @returns Whatever memcmp returns.
 * @param   pSgBuf1      First S/G buffer.
 * @param   pSgBuf2      Second S/G buffer.
 * @param   cbCmp        How many bytes to compare.
 * @param   poffDiff     Where to store the offset of the first different byte
 *                       in the buffer starting from the position of the S/G
 *                       buffer before this call.
 * @param   fAdvance     Flag whether the internal buffer position should be advanced.
 *
 */
RTDECL(int) RTSgBufCmpEx(PRTSGBUF pSgBuf1, PRTSGBUF pSgBuf2, size_t cbCmp, size_t *poffDiff, bool fAdvance);

/**
 * Fills an S/G buf with a constant byte.
 *
 * @returns The number of actually filled bytes.
 *          Can be less than than cbSet if the end of the S/G buffer was reached.
 * @param   pSgBuf       The S/G buffer.
 * @param   ubFill       The byte to fill the buffer with.
 * @param   cbSet        How many bytes to set.
 *
 * @note This operation advances the internal buffer pointer of the S/G buffer.
 */
RTDECL(size_t) RTSgBufSet(PRTSGBUF pSgBuf, uint8_t ubFill, size_t cbSet);

/**
 * Copies data from an S/G buffer into a given non scattered buffer.
 *
 * @returns Number of bytes copied.
 * @param   pSgBuf       The S/G buffer to copy from.
 * @param   pvBuf        Buffer to copy the data into.
 * @param   cbCopy       How many bytes to copy.
 *
 * @note This operation advances the internal buffer pointer of the S/G buffer.
 */
RTDECL(size_t) RTSgBufCopyToBuf(PRTSGBUF pSgBuf, void *pvBuf, size_t cbCopy);

/**
 * Copies data from a non scattered buffer into an S/G buffer.
 *
 * @returns Number of bytes copied.
 * @param   pSgBuf       The S/G buffer to copy to.
 * @param   pvBuf        Buffer to copy the data from.
 * @param   cbCopy       How many bytes to copy.
 *
 * @note This operation advances the internal buffer pointer of the S/G buffer.
 */
RTDECL(size_t) RTSgBufCopyFromBuf(PRTSGBUF pSgBuf, const void *pvBuf, size_t cbCopy);

/**
 * Copies data from the given S/G buffer to a destination handled by the given callback.
 *
 * @returns Number of bytes copied.
 * @param   pSgBuf       The S/G buffer to copy from.
 * @param   cbCopy       How many bytes to copy.
 * @param   pfnCopyTo    The callback to call on every S/G buffer segment until the operation finished.
 * @param   pvUser       Opaque user data to pass in the given callback.
 *
 * @note This operation advances the internal buffer pointer of the S/G buffer.
 */
RTDECL(size_t) RTSgBufCopyToFn(PRTSGBUF pSgBuf, size_t cbCopy, PFNRTSGBUFCOPYTO pfnCopyTo, void *pvUser);

/**
 * Copies data to the given S/G buffer from a destination handled by the given callback.
 *
 * @returns Number of bytes copied.
 * @param   pSgBuf       The S/G buffer to copy to.
 * @param   cbCopy       How many bytes to copy.
 * @param   pfnCopyFrom  The callback to call on every S/G buffer segment until the operation finished.
 * @param   pvUser       Opaque user data to pass in the given callback.
 *
 * @note This operation advances the internal buffer pointer of the S/G buffer.
 */
RTDECL(size_t) RTSgBufCopyFromFn(PRTSGBUF pSgBuf, size_t cbCopy, PFNRTSGBUFCOPYFROM pfnCopyFrom, void *pvUser);

/**
 * Advances the internal buffer pointer.
 *
 * @returns Number of bytes the pointer was moved forward.
 * @param   pSgBuf      The S/G buffer.
 * @param   cbAdvance   Number of bytes to move forward.
 */
RTDECL(size_t) RTSgBufAdvance(PRTSGBUF pSgBuf, size_t cbAdvance);

/**
 * Constructs a new segment array starting from the current position
 * and describing the given number of bytes.
 *
 * @returns Number of bytes the array describes.
 * @param   pSgBuf      The S/G buffer.
 * @param   paSeg       The uninitialized segment array.
 *                      If NULL pcSeg will contain the number of segments needed
 *                      to describe the requested amount of data.
 * @param   pcSeg       The number of segments the given array has.
 *                      This will hold the actual number of entries needed upon return.
 * @param   cbData      Number of bytes the new array should describe.
 *
 * @note This operation advances the internal buffer pointer of the S/G buffer if paSeg is not NULL.
 */
RTDECL(size_t) RTSgBufSegArrayCreate(PRTSGBUF pSgBuf, PRTSGSEG paSeg, unsigned *pcSeg, size_t cbData);

/**
 * Returns whether the given S/G buffer is zeroed out from the current position
 * upto the number of bytes to check.
 *
 * @returns true if the buffer has only zeros
 *          false otherwise.
 * @param   pSgBuf      The S/G buffer.
 * @param   cbCheck     Number of bytes to check.
 */
RTDECL(bool) RTSgBufIsZero(PRTSGBUF pSgBuf, size_t cbCheck);

/**
 * Maps the given S/G buffer to a segment array of another type (for example to
 * iovec on POSIX or WSABUF on Windows).
 *
 * @param   paMapped    Where to store the pointer to the start of the native
 *                      array or NULL.  The memory needs to be freed with
 *                      RTMemTmpFree().
 * @param   pSgBuf      The S/G buffer to map.
 * @param   Struct      Struct used as the destination.
 * @param   pvBufField  Name of the field holding the pointer to a buffer.
 * @param   TypeBufPtr  Type of the buffer pointer.
 * @param   cbBufField  Name of the field holding the size of the buffer.
 * @param   TypeBufSize Type of the field for the buffer size.
 * @param   cSegsMapped Where to store the number of segments the native array
 *                      has.
 *
 * @note    This operation maps the whole S/G buffer starting at the current
 *          internal position.  The internal buffer position is unchanged by
 *          this operation.
 *
 * @remark  Usage is a bit ugly but saves a few lines of duplicated code
 *          somewhere else and makes it possible to keep the S/G buffer members
 *          private without going through RTSgBufSegArrayCreate() first.
 */
#define RTSgBufMapToNative(paMapped, pSgBuf, Struct, pvBufField, TypeBufPtr, cbBufField, TypeBufSize, cSegsMapped) \
    do \
    { \
        AssertCompileMemberSize(Struct, pvBufField, RT_SIZEOFMEMB(RTSGSEG, pvSeg)); \
        /*AssertCompile(RT_SIZEOFMEMB(Struct, cbBufField) >=  RT_SIZEOFMEMB(RTSGSEG, cbSeg));*/ \
        (cSegsMapped) = (pSgBuf)->cSegs - (pSgBuf)->idxSeg; \
        \
        /* We need room for at least one segment. */ \
        if ((pSgBuf)->cSegs == (pSgBuf)->idxSeg) \
            (cSegsMapped)++; \
        \
        (paMapped) = (Struct *)RTMemTmpAllocZ((cSegsMapped) * sizeof(Struct)); \
        if ((paMapped)) \
        { \
            /* The first buffer is special because we could be in the middle of a segment. */ \
            (paMapped)[0].pvBufField = (TypeBufPtr)(pSgBuf)->pvSegCur; \
            (paMapped)[0].cbBufField = (TypeBufSize)(pSgBuf)->cbSegLeft; \
            \
            for (unsigned i = 1; i < (cSegsMapped); i++) \
            { \
                (paMapped)[i].pvBufField = (TypeBufPtr)(pSgBuf)->paSegs[(pSgBuf)->idxSeg + i].pvSeg; \
                (paMapped)[i].cbBufField = (TypeBufSize)(pSgBuf)->paSegs[(pSgBuf)->idxSeg + i].cbSeg; \
            } \
        } \
    } while (0)

RT_C_DECLS_END

/** @} */

#endif /* !IPRT_INCLUDED_sg_h */

