/** @file
 * VD Container API - internal interfaces.
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

#ifndef VBOX_INCLUDED_vd_ifs_internal_h
#define VBOX_INCLUDED_vd_ifs_internal_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/sg.h>
#include <VBox/vd-ifs.h>

RT_C_DECLS_BEGIN

/** @addtogroup grp_vd
 * @internal
 * @{ */

/**
 * Read data callback.
 *
 * @return  VBox status code.
 * @return  VERR_VD_NOT_OPENED if no image is opened in HDD container.
 * @param   pvUser          The opaque data passed for the operation.
 * @param   uOffset         Offset of first reading byte from start of disk.
 *                          Must be aligned to a sector boundary.
 * @param   pvBuffer        Pointer to buffer for reading data.
 * @param   cbBuffer        Number of bytes to read.
 *                          Must be aligned to a sector boundary.
 */
typedef DECLCALLBACKTYPE(int, FNVDPARENTREAD,(void *pvUser, uint64_t uOffset, void *pvBuffer, size_t cbBuffer));
/** Pointer to a FNVDPARENTREAD. */
typedef FNVDPARENTREAD *PFNVDPARENTREAD;

/**
 * Interface to get the parent state.
 *
 * Per-operation interface. Optional, present only if there is a parent, and
 * used only internally for compacting.
 */
typedef struct VDINTERFACEPARENTSTATE
{
    /**
     * Common interface header.
     */
    VDINTERFACE     Core;

    /**
     * Read data callback, see FNVDPARENTREAD for details.
     */
    PFNVDPARENTREAD pfnParentRead;

} VDINTERFACEPARENTSTATE, *PVDINTERFACEPARENTSTATE;


/**
 * Get parent state interface from interface list.
 *
 * @return Pointer to the first parent state interface in the list.
 * @param  pVDIfs    Pointer to the interface list.
 */
DECLINLINE(PVDINTERFACEPARENTSTATE) VDIfParentStateGet(PVDINTERFACE pVDIfs)
{
    PVDINTERFACE pIf = VDInterfaceGet(pVDIfs, VDINTERFACETYPE_PARENTSTATE);

    /* Check that the interface descriptor is a progress interface. */
    AssertMsgReturn(   !pIf
                    || (   (pIf->enmInterface == VDINTERFACETYPE_PARENTSTATE)
                        && (pIf->cbSize == sizeof(VDINTERFACEPARENTSTATE))),
                    ("Not a parent state interface"), NULL);

    return (PVDINTERFACEPARENTSTATE)pIf;
}

/** Forward declaration. Only visible in the VBoxHDD module. */
/** I/O context */
typedef struct VDIOCTX *PVDIOCTX;
/** Storage backend handle. */
typedef struct VDIOSTORAGE *PVDIOSTORAGE;
/** Pointer to a storage backend handle. */
typedef PVDIOSTORAGE *PPVDIOSTORAGE;

/**
 * Completion callback for meta/userdata reads or writes.
 *
 * @return  VBox status code.
 *          VINF_SUCCESS if everything was successful and the transfer can continue.
 *          VERR_VD_ASYNC_IO_IN_PROGRESS if there is another data transfer pending.
 * @param   pBackendData    The opaque backend data.
 * @param   pIoCtx          I/O context associated with this request.
 * @param   pvUser          Opaque user data passed during a read/write request.
 * @param   rcReq           Status code for the completed request.
 */
typedef DECLCALLBACKTYPE(int, FNVDXFERCOMPLETED,(void *pBackendData, PVDIOCTX pIoCtx, void *pvUser, int rcReq));
/** Pointer to FNVDXFERCOMPLETED() */
typedef FNVDXFERCOMPLETED *PFNVDXFERCOMPLETED;

/** Metadata transfer handle. */
typedef struct VDMETAXFER *PVDMETAXFER;
/** Pointer to a metadata transfer handle. */
typedef PVDMETAXFER *PPVDMETAXFER;


/**
 * Internal I/O interface between the generic VD layer and the backends.
 *
 * Per-image. Always passed to backends.
 */
typedef struct VDINTERFACEIOINT
{
    /**
     * Common interface header.
     */
    VDINTERFACE    Core;

    /**
     * Open callback
     *
     * @return  VBox status code.
     * @param   pvUser          The opaque data passed on container creation.
     * @param   pszLocation     Name of the location to open.
     * @param   fOpen           Flags for opening the backend.
     *                          See RTFILE_O_* \#defines, inventing another set
     *                          of open flags is not worth the mapping effort.
     * @param   ppStorage       Where to store the storage handle.
     */
    DECLR3CALLBACKMEMBER(int, pfnOpen, (void *pvUser, const char *pszLocation,
                                        uint32_t fOpen, PPVDIOSTORAGE ppStorage));

    /**
     * Close callback.
     *
     * @return  VBox status code.
     * @param   pvUser          The opaque data passed on container creation.
     * @param   pStorage        The storage handle to close.
     */
    DECLR3CALLBACKMEMBER(int, pfnClose, (void *pvUser, PVDIOSTORAGE pStorage));

    /**
     * Delete callback.
     *
     * @return  VBox status code.
     * @param   pvUser          The opaque data passed on container creation.
     * @param   pcszFilename    Name of the file to delete.
     */
    DECLR3CALLBACKMEMBER(int, pfnDelete, (void *pvUser, const char *pcszFilename));

    /**
     * Move callback.
     *
     * @return  VBox status code.
     * @param   pvUser          The opaque data passed on container creation.
     * @param   pcszSrc         The path to the source file.
     * @param   pcszDst         The path to the destination file.
     *                          This file will be created.
     * @param   fMove           A combination of the RTFILEMOVE_* flags.
     */
    DECLR3CALLBACKMEMBER(int, pfnMove, (void *pvUser, const char *pcszSrc, const char *pcszDst, unsigned fMove));

    /**
     * Returns the free space on a disk.
     *
     * @return  VBox status code.
     * @param   pvUser          The opaque data passed on container creation.
     * @param   pcszFilename    Name of a file to identify the disk.
     * @param   pcbFreeSpace    Where to store the free space of the disk.
     */
    DECLR3CALLBACKMEMBER(int, pfnGetFreeSpace, (void *pvUser, const char *pcszFilename, int64_t *pcbFreeSpace));

    /**
     * Returns the last modification timestamp of a file.
     *
     * @return  VBox status code.
     * @param   pvUser          The opaque data passed on container creation.
     * @param   pcszFilename    Name of a file to identify the disk.
     * @param   pModificationTime   Where to store the timestamp of the file.
     */
    DECLR3CALLBACKMEMBER(int, pfnGetModificationTime, (void *pvUser, const char *pcszFilename, PRTTIMESPEC pModificationTime));

    /**
     * Returns the size of the opened storage backend.
     *
     * @return  VBox status code.
     * @param   pvUser          The opaque data passed on container creation.
     * @param   pStorage        The storage handle to get the size from.
     * @param   pcbSize         Where to store the size of the storage backend.
     */
    DECLR3CALLBACKMEMBER(int, pfnGetSize, (void *pvUser, PVDIOSTORAGE pStorage,
                                           uint64_t *pcbSize));

    /**
     * Sets the size of the opened storage backend if possible.
     *
     * @return  VBox status code.
     * @retval  VERR_NOT_SUPPORTED if the backend does not support this operation.
     * @param   pvUser          The opaque data passed on container creation.
     * @param   pStorage        The storage handle.
     * @param   cbSize          The new size of the image.
     *
     * @note Depending on the host the underlying storage (backing file, etc.)
     *       might not have all required storage allocated (sparse file) which
     *       can delay writes or fail with a not enough free space error if there
     *       is not enough space on the storage medium when writing to the range for
     *       the first time.
     *       Use VDINTERFACEIOINT::pfnSetAllocationSize to make sure the storage is
     *       really alloacted.
     */
    DECLR3CALLBACKMEMBER(int, pfnSetSize, (void *pvUser, PVDIOSTORAGE pStorage,
                                           uint64_t cbSize));

    /**
     * Sets the size of the opened storage backend making sure the given size
     * is really allocated.
     *
     * @return VBox status code.
     * @param   pvUser          The opaque data passed on container creation.
     * @param   pStorage        The storage handle.
     * @param   cbSize          The new size of the image.
     * @param   fFlags          Flags for controlling the allocation strategy.
     *                          Reserved for future use, MBZ.
     * @param   pIfProgress     Progress interface (optional).
     * @param   uPercentStart   Progress starting point.
     * @param   uPercentSpan    Length of operation in percent.
     */
    DECLR3CALLBACKMEMBER(int, pfnSetAllocationSize, (void *pvUser, PVDIOSTORAGE pStorage,
                                                     uint64_t cbSize, uint32_t fFlags,
                                                     PVDINTERFACEPROGRESS pIfProgress,
                                                     unsigned uPercentStart, unsigned uPercentSpan));

    /**
     * Initiate a read request for user data.
     *
     * @return  VBox status code.
     * @param   pvUser         The opaque user data passed on container creation.
     * @param   pStorage       The storage handle.
     * @param   uOffset        The offset to start reading from.
     * @param   pIoCtx         I/O context passed in the read/write callback.
     * @param   cbRead         How many bytes to read.
     */
    DECLR3CALLBACKMEMBER(int, pfnReadUser, (void *pvUser, PVDIOSTORAGE pStorage,
                                            uint64_t uOffset, PVDIOCTX pIoCtx,
                                            size_t cbRead));

    /**
     * Initiate a write request for user data.
     *
     * @return  VBox status code.
     * @param   pvUser         The opaque user data passed on container creation.
     * @param   pStorage       The storage handle.
     * @param   uOffset        The offset to start writing to.
     * @param   pIoCtx         I/O context passed in the read/write callback.
     * @param   cbWrite        How many bytes to write.
     * @param   pfnCompleted   Completion callback.
     * @param   pvCompleteUser Opaque user data passed in the completion callback.
     */
    DECLR3CALLBACKMEMBER(int, pfnWriteUser, (void *pvUser, PVDIOSTORAGE pStorage,
                                             uint64_t uOffset, PVDIOCTX pIoCtx,
                                             size_t cbWrite,
                                             PFNVDXFERCOMPLETED pfnComplete,
                                             void *pvCompleteUser));

    /**
     * Reads metadata from storage.
     * The current I/O context will be halted.
     *
     * @returns VBox status code.
     * @param   pvUser         The opaque user data passed on container creation.
     * @param   pStorage       The storage handle.
     * @param   uOffset        Offset to start reading from.
     * @param   pvBuffer       Where to store the data.
     * @param   cbBuffer       How many bytes to read.
     * @param   pIoCtx         The I/O context which triggered the read.
     * @param   ppMetaXfer     Where to store the metadata transfer handle on success.
     * @param   pfnCompleted   Completion callback.
     * @param   pvCompleteUser Opaque user data passed in the completion callback.
     *
     * @note    If pIoCtx is NULL the metadata read is handled synchronously
     *          i.e. the call returns only if the data is available in the given
     *          buffer. ppMetaXfer, pfnCompleted and pvCompleteUser are ignored in that case.
     *          Use the synchronous version only when opening/closing the image
     *          or when doing certain operations like resizing, compacting or repairing
     *          the disk.
     */
    DECLR3CALLBACKMEMBER(int, pfnReadMeta, (void *pvUser, PVDIOSTORAGE pStorage,
                                            uint64_t uOffset, void *pvBuffer,
                                            size_t cbBuffer, PVDIOCTX pIoCtx,
                                            PPVDMETAXFER ppMetaXfer,
                                            PFNVDXFERCOMPLETED pfnComplete,
                                            void *pvCompleteUser));

    /**
     * Writes metadata to storage.
     *
     * @returns VBox status code.
     * @param   pvUser         The opaque user data passed on container creation.
     * @param   pStorage       The storage handle.
     * @param   uOffset        Offset to start writing to.
     * @param   pvBuffer       Written data.
     * @param   cbBuffer       How many bytes to write.
     * @param   pIoCtx         The I/O context which triggered the write.
     * @param   pfnCompleted   Completion callback.
     * @param   pvCompleteUser Opaque user data passed in the completion callback.
     *
     * @sa      VDINTERFACEIOINT::pfnReadMeta
     */
    DECLR3CALLBACKMEMBER(int, pfnWriteMeta, (void *pvUser, PVDIOSTORAGE pStorage,
                                             uint64_t uOffset, const void *pvBuffer,
                                             size_t cbBuffer, PVDIOCTX pIoCtx,
                                             PFNVDXFERCOMPLETED pfnComplete,
                                             void *pvCompleteUser));

    /**
     * Releases a metadata transfer handle.
     * The free space can be used for another transfer.
     *
     * @param   pvUser         The opaque user data passed on container creation.
     * @param   pMetaXfer      The metadata transfer handle to release.
     */
    DECLR3CALLBACKMEMBER(void, pfnMetaXferRelease, (void *pvUser, PVDMETAXFER pMetaXfer));

    /**
     * Initiates a flush request.
     *
     * @return  VBox status code.
     * @param   pvUser         The opaque data passed on container creation.
     * @param   pStorage       The storage handle to flush.
     * @param   pIoCtx         I/O context which triggered the flush.
     * @param   pfnCompleted   Completion callback.
     * @param   pvCompleteUser Opaque user data passed in the completion callback.
     *
     * @sa      VDINTERFACEIOINT::pfnReadMeta
     */
    DECLR3CALLBACKMEMBER(int, pfnFlush, (void *pvUser, PVDIOSTORAGE pStorage,
                                         PVDIOCTX pIoCtx,
                                         PFNVDXFERCOMPLETED pfnComplete,
                                         void *pvCompleteUser));

    /**
     * Copies a buffer into the I/O context.
     *
     * @return Number of bytes copied.
     * @param  pvUser          The opaque user data passed on container creation.
     * @param  pIoCtx          I/O context to copy the data to.
     * @param  pvBuffer        Buffer to copy.
     * @param  cbBuffer        Number of bytes to copy.
     */
    DECLR3CALLBACKMEMBER(size_t, pfnIoCtxCopyTo, (void *pvUser, PVDIOCTX pIoCtx,
                                                  const void *pvBuffer, size_t cbBuffer));

    /**
     * Copies data from the I/O context into a buffer.
     *
     * @return Number of bytes copied.
     * @param  pvUser          The opaque user data passed on container creation.
     * @param  pIoCtx          I/O context to copy the data from.
     * @param  pvBuffer        Destination buffer.
     * @param  cbBuffer        Number of bytes to copy.
     */
    DECLR3CALLBACKMEMBER(size_t, pfnIoCtxCopyFrom, (void *pvUser, PVDIOCTX pIoCtx,
                                                    void *pvBuffer, size_t cbBuffer));

    /**
     * Sets the buffer of the given context to a specific byte.
     *
     * @return Number of bytes set.
     * @param  pvUser          The opaque user data passed on container creation.
     * @param  pIoCtx          I/O context to copy the data from.
     * @param  ch              The byte to set.
     * @param  cbSet           Number of bytes to set.
     */
    DECLR3CALLBACKMEMBER(size_t, pfnIoCtxSet, (void *pvUser, PVDIOCTX pIoCtx,
                                               int ch, size_t cbSet));

    /**
     * Creates a segment array from the I/O context data buffer.
     *
     * @returns Number of bytes the array describes.
     * @param  pvUser          The opaque user data passed on container creation.
     * @param  pIoCtx          I/O context to copy the data from.
     * @param  paSeg           The uninitialized segment array.
     *                         If NULL pcSeg will contain the number of segments needed
     *                         to describe the requested amount of data.
     * @param  pcSeg           The number of segments the given array has.
     *                         This will hold the actual number of entries needed upon return.
     * @param  cbData          Number of bytes the new array should describe.
     */
    DECLR3CALLBACKMEMBER(size_t, pfnIoCtxSegArrayCreate, (void *pvUser, PVDIOCTX pIoCtx,
                                                          PRTSGSEG paSeg, unsigned *pcSeg,
                                                          size_t cbData));
    /**
     * Marks the given number of bytes as completed and continues the I/O context.
     *
     * @param   pvUser         The opaque user data passed on container creation.
     * @param   pIoCtx         The I/O context.
     * @param   rcReq          Status code the request completed with.
     * @param   cbCompleted    Number of bytes completed.
     */
    DECLR3CALLBACKMEMBER(void, pfnIoCtxCompleted, (void *pvUser, PVDIOCTX pIoCtx,
                                                   int rcReq, size_t cbCompleted));

    /**
     * Returns whether the given I/O context must be treated synchronously.
     *
     * @returns true if the I/O context must be processed synchronously
     *          false otherwise.
     * @param   pvUser         The opaque user data passed on container creation.
     * @param   pIoCtx         The I/O context.
     */
    DECLR3CALLBACKMEMBER(bool, pfnIoCtxIsSynchronous, (void *pvUser, PVDIOCTX pIoCtx));

    /**
     * Returns whether the user buffer of the I/O context is complete zero
     * from to current position upto the given number of bytes.
     *
     * @returns true if the I/O context user buffer consists solely of zeros
     *          false otherwise.
     * @param   pvUser         The opaque user data passed on container creation.
     * @param   pIoCtx         The I/O context.
     * @param   cbCheck        Number of bytes to check for zeros.
     * @param   fAdvance       Flag whether to advance the buffer pointer if true
     *                         is returned.
     */
    DECLR3CALLBACKMEMBER(bool, pfnIoCtxIsZero, (void *pvUser, PVDIOCTX pIoCtx,
                                                size_t cbCheck, bool fAdvance));

    /**
     * Returns the data unit size, i.e. the smallest size for a transfer.
     * (similar to the sector size of disks).
     *
     * @returns The data unit size.
     * @param   pvUser         The opaque user data passed on container creation.
     * @param   pIoCtx         The I/O context.
     */
    DECLR3CALLBACKMEMBER(size_t, pfnIoCtxGetDataUnitSize, (void *pvUser, PVDIOCTX pIoCtx));

} VDINTERFACEIOINT, *PVDINTERFACEIOINT;

/**
 * Get internal I/O interface from interface list.
 *
 * @return Pointer to the first internal I/O interface in the list.
 * @param  pVDIfs    Pointer to the interface list.
 */
DECLINLINE(PVDINTERFACEIOINT) VDIfIoIntGet(PVDINTERFACE pVDIfs)
{
    PVDINTERFACE pIf = VDInterfaceGet(pVDIfs, VDINTERFACETYPE_IOINT);

    /* Check that the interface descriptor is a progress interface. */
    AssertMsgReturn(   !pIf
                    || (   (pIf->enmInterface == VDINTERFACETYPE_IOINT)
                        && (pIf->cbSize == sizeof(VDINTERFACEIOINT))),
                    ("Not an internal I/O interface"), NULL);

    return (PVDINTERFACEIOINT)pIf;
}

DECLINLINE(int) vdIfIoIntFileOpen(PVDINTERFACEIOINT pIfIoInt, const char *pszFilename,
                                  uint32_t fOpen, PPVDIOSTORAGE ppStorage)
{
    return pIfIoInt->pfnOpen(pIfIoInt->Core.pvUser, pszFilename, fOpen, ppStorage);
}

DECLINLINE(int) vdIfIoIntFileClose(PVDINTERFACEIOINT pIfIoInt, PVDIOSTORAGE pStorage)
{
    return pIfIoInt->pfnClose(pIfIoInt->Core.pvUser, pStorage);
}

DECLINLINE(int) vdIfIoIntFileDelete(PVDINTERFACEIOINT pIfIoInt, const char *pszFilename)
{
    return pIfIoInt->pfnDelete(pIfIoInt->Core.pvUser, pszFilename);
}

DECLINLINE(int) vdIfIoIntFileMove(PVDINTERFACEIOINT pIfIoInt, const char *pszSrc,
                                  const char *pszDst, unsigned fMove)
{
    return pIfIoInt->pfnMove(pIfIoInt->Core.pvUser, pszSrc, pszDst, fMove);
}

DECLINLINE(int) vdIfIoIntFileGetFreeSpace(PVDINTERFACEIOINT pIfIoInt, const char *pszFilename,
                                          int64_t *pcbFree)
{
    return pIfIoInt->pfnGetFreeSpace(pIfIoInt->Core.pvUser, pszFilename, pcbFree);
}

DECLINLINE(int) vdIfIoIntFileGetModificationTime(PVDINTERFACEIOINT pIfIoInt, const char *pcszFilename,
                                                 PRTTIMESPEC pModificationTime)
{
    return pIfIoInt->pfnGetModificationTime(pIfIoInt->Core.pvUser, pcszFilename,
                                            pModificationTime);
}

DECLINLINE(int) vdIfIoIntFileGetSize(PVDINTERFACEIOINT pIfIoInt, PVDIOSTORAGE pStorage,
                                     uint64_t *pcbSize)
{
    return pIfIoInt->pfnGetSize(pIfIoInt->Core.pvUser, pStorage, pcbSize);
}

DECLINLINE(int) vdIfIoIntFileSetSize(PVDINTERFACEIOINT pIfIoInt, PVDIOSTORAGE pStorage,
                                     uint64_t cbSize)
{
    return pIfIoInt->pfnSetSize(pIfIoInt->Core.pvUser, pStorage, cbSize);
}

DECLINLINE(int) vdIfIoIntFileSetAllocationSize(PVDINTERFACEIOINT pIfIoInt, PVDIOSTORAGE pStorage,
                                               uint64_t cbSize, uint32_t fFlags,
                                               PVDINTERFACEPROGRESS pIfProgress,
                                               unsigned uPercentStart, unsigned uPercentSpan)
{
    return pIfIoInt->pfnSetAllocationSize(pIfIoInt->Core.pvUser, pStorage, cbSize, fFlags,
                                          pIfProgress, uPercentStart, uPercentSpan);
}

DECLINLINE(int) vdIfIoIntFileWriteSync(PVDINTERFACEIOINT pIfIoInt, PVDIOSTORAGE pStorage,
                                       uint64_t uOffset, const void *pvBuffer, size_t cbBuffer)
{
    return pIfIoInt->pfnWriteMeta(pIfIoInt->Core.pvUser, pStorage,
                                  uOffset, pvBuffer, cbBuffer, NULL,
                                  NULL, NULL);
}

DECLINLINE(int) vdIfIoIntFileReadSync(PVDINTERFACEIOINT pIfIoInt, PVDIOSTORAGE pStorage,
                                      uint64_t uOffset, void *pvBuffer, size_t cbBuffer)
{
    return pIfIoInt->pfnReadMeta(pIfIoInt->Core.pvUser, pStorage,
                                 uOffset, pvBuffer, cbBuffer, NULL,
                                 NULL, NULL, NULL);
}

DECLINLINE(int) vdIfIoIntFileFlushSync(PVDINTERFACEIOINT pIfIoInt, PVDIOSTORAGE pStorage)
{
    return pIfIoInt->pfnFlush(pIfIoInt->Core.pvUser, pStorage, NULL, NULL, NULL);
}

DECLINLINE(int) vdIfIoIntFileReadUser(PVDINTERFACEIOINT pIfIoInt, PVDIOSTORAGE pStorage,
                                      uint64_t uOffset, PVDIOCTX pIoCtx, size_t cbRead)
{
    return pIfIoInt->pfnReadUser(pIfIoInt->Core.pvUser, pStorage,
                                 uOffset, pIoCtx, cbRead);
}

DECLINLINE(int) vdIfIoIntFileWriteUser(PVDINTERFACEIOINT pIfIoInt, PVDIOSTORAGE pStorage,
                                       uint64_t uOffset, PVDIOCTX pIoCtx, size_t cbWrite,
                                       PFNVDXFERCOMPLETED pfnComplete,
                                       void *pvCompleteUser)
{
    return pIfIoInt->pfnWriteUser(pIfIoInt->Core.pvUser, pStorage,
                                  uOffset, pIoCtx, cbWrite, pfnComplete,
                                  pvCompleteUser);
}

DECLINLINE(int) vdIfIoIntFileReadMeta(PVDINTERFACEIOINT pIfIoInt, PVDIOSTORAGE pStorage,
                                      uint64_t uOffset, void *pvBuffer,
                                      size_t cbBuffer, PVDIOCTX pIoCtx,
                                      PPVDMETAXFER ppMetaXfer,
                                      PFNVDXFERCOMPLETED pfnComplete,
                                      void *pvCompleteUser)
{
    return pIfIoInt->pfnReadMeta(pIfIoInt->Core.pvUser, pStorage,
                                 uOffset, pvBuffer, cbBuffer, pIoCtx,
                                 ppMetaXfer, pfnComplete, pvCompleteUser);
}

DECLINLINE(int) vdIfIoIntFileWriteMeta(PVDINTERFACEIOINT pIfIoInt, PVDIOSTORAGE pStorage,
                                       uint64_t uOffset, void *pvBuffer,
                                       size_t cbBuffer, PVDIOCTX pIoCtx,
                                       PFNVDXFERCOMPLETED pfnComplete,
                                       void *pvCompleteUser)
{
    return pIfIoInt->pfnWriteMeta(pIfIoInt->Core.pvUser, pStorage,
                                  uOffset, pvBuffer, cbBuffer, pIoCtx,
                                  pfnComplete, pvCompleteUser);
}

DECLINLINE(void) vdIfIoIntMetaXferRelease(PVDINTERFACEIOINT pIfIoInt, PVDMETAXFER pMetaXfer)
{
    pIfIoInt->pfnMetaXferRelease(pIfIoInt->Core.pvUser, pMetaXfer);
}

DECLINLINE(int) vdIfIoIntFileFlush(PVDINTERFACEIOINT pIfIoInt, PVDIOSTORAGE pStorage,
                                   PVDIOCTX pIoCtx, PFNVDXFERCOMPLETED pfnComplete,
                                   void *pvCompleteUser)
{
    return pIfIoInt->pfnFlush(pIfIoInt->Core.pvUser, pStorage, pIoCtx, pfnComplete,
                              pvCompleteUser);
}

DECLINLINE(size_t) vdIfIoIntIoCtxCopyTo(PVDINTERFACEIOINT pIfIoInt, PVDIOCTX pIoCtx,
                                        const void *pvBuffer, size_t cbBuffer)
{
    return pIfIoInt->pfnIoCtxCopyTo(pIfIoInt->Core.pvUser, pIoCtx, pvBuffer, cbBuffer);
}

DECLINLINE(size_t) vdIfIoIntIoCtxCopyFrom(PVDINTERFACEIOINT pIfIoInt, PVDIOCTX pIoCtx,
                                          void *pvBuffer, size_t cbBuffer)
{
    return pIfIoInt->pfnIoCtxCopyFrom(pIfIoInt->Core.pvUser, pIoCtx, pvBuffer, cbBuffer);
}

DECLINLINE(size_t) vdIfIoIntIoCtxSet(PVDINTERFACEIOINT pIfIoInt, PVDIOCTX pIoCtx,
                                     int ch, size_t cbSet)
{
    return pIfIoInt->pfnIoCtxSet(pIfIoInt->Core.pvUser, pIoCtx, ch, cbSet);
}

DECLINLINE(size_t) vdIfIoIntIoCtxSegArrayCreate(PVDINTERFACEIOINT pIfIoInt, PVDIOCTX pIoCtx,
                                                PRTSGSEG paSeg, unsigned *pcSeg,
                                                size_t cbData)
{
    return pIfIoInt->pfnIoCtxSegArrayCreate(pIfIoInt->Core.pvUser, pIoCtx, paSeg, pcSeg, cbData);
}

DECLINLINE(bool) vdIfIoIntIoCtxIsSynchronous(PVDINTERFACEIOINT pIfIoInt, PVDIOCTX pIoCtx)
{
    return pIfIoInt->pfnIoCtxIsSynchronous(pIfIoInt->Core.pvUser, pIoCtx);
}

DECLINLINE(bool) vdIfIoIntIoCtxIsZero(PVDINTERFACEIOINT pIfIoInt, PVDIOCTX pIoCtx,
                                      size_t cbCheck, bool fAdvance)
{
    return pIfIoInt->pfnIoCtxIsZero(pIfIoInt->Core.pvUser, pIoCtx, cbCheck, fAdvance);
}

DECLINLINE(size_t) vdIfIoIntIoCtxGetDataUnitSize(PVDINTERFACEIOINT pIfIoInt, PVDIOCTX pIoCtx)
{
    return pIfIoInt->pfnIoCtxGetDataUnitSize(pIfIoInt->Core.pvUser, pIoCtx);
}

/**
 * Interface for the metadata traverse callback.
 *
 * Per-operation interface. Present only for the metadata traverse callback.
 */
typedef struct VDINTERFACETRAVERSEMETADATA
{
    /**
     * Common interface header.
     */
    VDINTERFACE    Core;

    /**
     * Traverse callback.
     *
     * @returns VBox status code.
     * @param   pvUser          The opaque data passed for the operation.
     * @param   pvMetadataChunk Pointer to a chunk of the image metadata.
     * @param   cbMetadataChunk Size of the metadata chunk
     */
    DECLR3CALLBACKMEMBER(int, pfnMetadataCallback, (void *pvUser, const void *pvMetadataChunk,
                                                    size_t cbMetadataChunk));

} VDINTERFACETRAVERSEMETADATA, *PVDINTERFACETRAVERSEMETADATA;


/**
 * Get parent state interface from interface list.
 *
 * @return Pointer to the first parent state interface in the list.
 * @param  pVDIfs    Pointer to the interface list.
 */
DECLINLINE(PVDINTERFACETRAVERSEMETADATA) VDIfTraverseMetadataGet(PVDINTERFACE pVDIfs)
{
    PVDINTERFACE pIf = VDInterfaceGet(pVDIfs, VDINTERFACETYPE_TRAVERSEMETADATA);

    /* Check that the interface descriptor the correct interface. */
    AssertMsgReturn(   !pIf
                    || (   (pIf->enmInterface == VDINTERFACETYPE_TRAVERSEMETADATA)
                        && (pIf->cbSize == sizeof(VDINTERFACETRAVERSEMETADATA))),
                    ("Not a traverse metadata interface"), NULL);

    return (PVDINTERFACETRAVERSEMETADATA)pIf;
}

/** @} */
RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vd_ifs_internal_h */
