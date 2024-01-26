/** @file
 * VD Container API - interfaces.
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

#ifndef VBOX_INCLUDED_vd_ifs_h
#define VBOX_INCLUDED_vd_ifs_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/mem.h>
#include <iprt/file.h>
#include <iprt/net.h>
#include <iprt/sg.h>
#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/err.h>

RT_C_DECLS_BEGIN

/** @addtogroup grp_vd
 * @{ */

/** Interface header magic. */
#define VDINTERFACE_MAGIC UINT32_C(0x19701015)

/**
 * Supported interface types.
 */
typedef enum VDINTERFACETYPE
{
    /** First valid interface. */
    VDINTERFACETYPE_FIRST = 0,
    /** Interface to pass error message to upper layers. Per-disk. */
    VDINTERFACETYPE_ERROR = VDINTERFACETYPE_FIRST,
    /** Interface for I/O operations. Per-image. */
    VDINTERFACETYPE_IO,
    /** Interface for progress notification. Per-operation. */
    VDINTERFACETYPE_PROGRESS,
    /** Interface for configuration information. Per-image. */
    VDINTERFACETYPE_CONFIG,
    /** Interface for TCP network stack. Per-image. */
    VDINTERFACETYPE_TCPNET,
    /** Interface for getting parent image state. Per-operation. */
    VDINTERFACETYPE_PARENTSTATE,
    /** Interface for synchronizing accesses from several threads. Per-disk. */
    VDINTERFACETYPE_THREADSYNC,
    /** Interface for I/O between the generic VBoxHDD code and the backend. Per-image (internal).
     * This interface is completely internal and must not be used elsewhere. */
    VDINTERFACETYPE_IOINT,
    /** Interface to query the use of block ranges on the disk. Per-operation. */
    VDINTERFACETYPE_QUERYRANGEUSE,
    /** Interface for the metadata traverse callback. Per-operation. */
    VDINTERFACETYPE_TRAVERSEMETADATA,
    /** Interface for crypto operations. Per-filter. */
    VDINTERFACETYPE_CRYPTO,
    /** invalid interface. */
    VDINTERFACETYPE_INVALID
} VDINTERFACETYPE;

/**
 * Common structure for all interfaces and at the beginning of all types.
 */
typedef struct VDINTERFACE
{
    uint32_t            u32Magic;
    /** Human readable interface name. */
    const char         *pszInterfaceName;
    /** Pointer to the next common interface structure. */
    struct VDINTERFACE *pNext;
    /** Interface type. */
    VDINTERFACETYPE     enmInterface;
    /** Size of the interface. */
    size_t              cbSize;
    /** Opaque user data which is passed on every call. */
    void               *pvUser;
} VDINTERFACE;
/** Pointer to a VDINTERFACE. */
typedef VDINTERFACE *PVDINTERFACE;
/** Pointer to a const VDINTERFACE. */
typedef const VDINTERFACE *PCVDINTERFACE;

/**
 * Helper functions to handle interface lists.
 *
 * @note These interface lists are used consistently to pass per-disk,
 * per-image and/or per-operation callbacks. Those three purposes are strictly
 * separate. See the individual interface declarations for what context they
 * apply to. The caller is responsible for ensuring that the lifetime of the
 * interface descriptors is appropriate for the category of interface.
 */

/**
 * Get a specific interface from a list of interfaces specified by the type.
 *
 * @return  Pointer to the matching interface or NULL if none was found.
 * @param   pVDIfs          Pointer to the VD interface list.
 * @param   enmInterface    Interface to search for.
 */
DECLINLINE(PVDINTERFACE) VDInterfaceGet(PVDINTERFACE pVDIfs, VDINTERFACETYPE enmInterface)
{
    AssertMsgReturn(   enmInterface >= VDINTERFACETYPE_FIRST
                    && enmInterface < VDINTERFACETYPE_INVALID,
                    ("enmInterface=%u", enmInterface), NULL);

    while (pVDIfs)
    {
        AssertMsgBreak(pVDIfs->u32Magic == VDINTERFACE_MAGIC,
                       ("u32Magic=%#x\n", pVDIfs->u32Magic));

        if (pVDIfs->enmInterface == enmInterface)
            return pVDIfs;
        pVDIfs = pVDIfs->pNext;
    }

    /* No matching interface was found. */
    return NULL;
}

/**
 * Add an interface to a list of interfaces.
 *
 * @return VBox status code.
 * @param  pInterface   Pointer to an unitialized common interface structure.
 * @param  pszName      Name of the interface.
 * @param  enmInterface Type of the interface.
 * @param  pvUser       Opaque user data passed on every function call.
 * @param  cbInterface  The interface size.
 * @param  ppVDIfs      Pointer to the VD interface list.
 */
DECLINLINE(int) VDInterfaceAdd(PVDINTERFACE pInterface, const char *pszName, VDINTERFACETYPE enmInterface, void *pvUser,
                               size_t cbInterface, PVDINTERFACE *ppVDIfs)
{
    /* Argument checks. */
    AssertMsgReturn(   enmInterface >= VDINTERFACETYPE_FIRST
                    && enmInterface < VDINTERFACETYPE_INVALID,
                    ("enmInterface=%u", enmInterface), VERR_INVALID_PARAMETER);

    AssertPtrReturn(ppVDIfs, VERR_INVALID_PARAMETER);

    /* Fill out interface descriptor. */
    pInterface->u32Magic         = VDINTERFACE_MAGIC;
    pInterface->cbSize           = cbInterface;
    pInterface->pszInterfaceName = pszName;
    pInterface->enmInterface     = enmInterface;
    pInterface->pvUser           = pvUser;
    pInterface->pNext            = *ppVDIfs;

    /* Remember the new start of the list. */
    *ppVDIfs = pInterface;

    return VINF_SUCCESS;
}

/**
 * Removes an interface from a list of interfaces.
 *
 * @return VBox status code
 * @param  pInterface   Pointer to an initialized common interface structure to remove.
 * @param  ppVDIfs      Pointer to the VD interface list to remove from.
 */
DECLINLINE(int) VDInterfaceRemove(PVDINTERFACE pInterface, PVDINTERFACE *ppVDIfs)
{
    int rc = VERR_NOT_FOUND;

    /* Argument checks. */
    AssertPtrReturn(pInterface, VERR_INVALID_PARAMETER);
    AssertPtrReturn(ppVDIfs, VERR_INVALID_PARAMETER);

    if (*ppVDIfs)
    {
        PVDINTERFACE pPrev = NULL;
        PVDINTERFACE pCurr = *ppVDIfs;

        while (   pCurr
               && (pCurr != pInterface))
        {
            pPrev = pCurr;
            pCurr = pCurr->pNext;
        }

        /* First interface */
        if (!pPrev)
        {
            *ppVDIfs = pCurr->pNext;
            rc = VINF_SUCCESS;
        }
        else if (pCurr)
        {
            Assert(pPrev->pNext == pCurr);
            pPrev->pNext = pCurr->pNext;
            rc = VINF_SUCCESS;
        }
    }

    return rc;
}

/**
 * Interface to deliver error messages (and also informational messages)
 * to upper layers.
 *
 * Per-disk interface. Optional, but think twice if you want to miss the
 * opportunity of reporting better human-readable error messages.
 */
typedef struct VDINTERFACEERROR
{
    /**
     * Common interface header.
     */
    VDINTERFACE    Core;

    /**
     * Error message callback. Must be able to accept special IPRT format
     * strings.
     *
     * @param   pvUser          The opaque data passed on container creation.
     * @param   rc              The VBox error code.
     * @param   SRC_POS         Use RT_SRC_POS.
     * @param   pszFormat       Error message format string.
     * @param   va              Error message arguments.
     */
    DECLR3CALLBACKMEMBER(void, pfnError, (void *pvUser, int rc, RT_SRC_POS_DECL,
                                          const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(6, 0));

    /**
     * Informational message callback. May be NULL. Used e.g. in
     * VDDumpImages(). Must be able to accept special IPRT format strings.
     *
     * @return  VBox status code.
     * @param   pvUser          The opaque data passed on container creation.
     * @param   pszFormat       Message format string.
     * @param   va              Message arguments.
     */
    DECLR3CALLBACKMEMBER(int, pfnMessage, (void *pvUser, const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(2, 0));

} VDINTERFACEERROR, *PVDINTERFACEERROR;

/**
 * Get error interface from interface list.
 *
 * @return Pointer to the first error interface in the list.
 * @param  pVDIfs    Pointer to the interface list.
 */
DECLINLINE(PVDINTERFACEERROR) VDIfErrorGet(PVDINTERFACE pVDIfs)
{
    PVDINTERFACE pIf = VDInterfaceGet(pVDIfs, VDINTERFACETYPE_ERROR);

    /* Check that the interface descriptor is a progress interface. */
    AssertMsgReturn(   !pIf
                    || (   (pIf->enmInterface == VDINTERFACETYPE_ERROR)
                        && (pIf->cbSize == sizeof(VDINTERFACEERROR))),
                    ("Not an error interface\n"), NULL);

    return (PVDINTERFACEERROR)pIf;
}

/**
 * Signal an error to the frontend.
 *
 * @returns VBox status code.
 * @param   pIfError           The error interface.
 * @param   rc                 The status code.
 * @param   SRC_POS            The position in the source code.
 * @param   pszFormat          The format string to pass.
 * @param   ...                Arguments to the format string.
 */
DECLINLINE(int) RT_IPRT_FORMAT_ATTR(6, 7) vdIfError(PVDINTERFACEERROR pIfError, int rc, RT_SRC_POS_DECL,
                                                    const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    if (pIfError)
        pIfError->pfnError(pIfError->Core.pvUser, rc, RT_SRC_POS_ARGS, pszFormat, va);
    va_end(va);

#if defined(LOG_ENABLED) && defined(Log)
    va_start(va, pszFormat);
    Log(("vdIfError: %N\n", pszFormat, &va));
    va_end(va);
#endif
    return rc;
}

/**
 * Signal an informational message to the frontend.
 *
 * @returns VBox status code.
 * @param   pIfError           The error interface.
 * @param   pszFormat          The format string to pass.
 * @param   ...                Arguments to the format string.
 */
DECLINLINE(int) RT_IPRT_FORMAT_ATTR(2, 3) vdIfErrorMessage(PVDINTERFACEERROR pIfError, const char *pszFormat, ...)
{
    int rc = VINF_SUCCESS;
    va_list va;
    va_start(va, pszFormat);
    if (pIfError && pIfError->pfnMessage)
        rc = pIfError->pfnMessage(pIfError->Core.pvUser, pszFormat, va);
    va_end(va);

#if defined(LOG_ENABLED) && defined(Log)
    va_start(va, pszFormat);
    Log(("vdIfErrorMessage: %N\n", pszFormat, &va));
    va_end(va);
#endif
    return rc;
}

/**
 * Completion callback which is called by the interface owner
 * to inform the backend that a task finished.
 *
 * @return  VBox status code.
 * @param   pvUser          Opaque user data which is passed on request submission.
 * @param   rcReq           Status code of the completed request.
 */
typedef DECLCALLBACKTYPE(int, FNVDCOMPLETED,(void *pvUser, int rcReq));
/** Pointer to FNVDCOMPLETED() */
typedef FNVDCOMPLETED *PFNVDCOMPLETED;

/**
 * Support interface for I/O
 *
 * Per-image. Optional as input.
 */
typedef struct VDINTERFACEIO
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
     * @param   pfnCompleted    The callback which is called whenever a task
     *                          completed. The backend has to pass the user data
     *                          of the request initiator (ie the one who calls
     *                          VDAsyncRead or VDAsyncWrite) in pvCompletion
     *                          if this is NULL.
     * @param   ppvStorage      Where to store the opaque storage handle.
     */
    DECLR3CALLBACKMEMBER(int, pfnOpen, (void *pvUser, const char *pszLocation,
                                        uint32_t fOpen,
                                        PFNVDCOMPLETED pfnCompleted,
                                        void **ppvStorage));

    /**
     * Close callback.
     *
     * @return  VBox status code.
     * @param   pvUser          The opaque data passed on container creation.
     * @param   pvStorage       The opaque storage handle to close.
     */
    DECLR3CALLBACKMEMBER(int, pfnClose, (void *pvUser, void *pvStorage));

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
     * @param   pvStorage       The opaque storage handle to get the size from.
     * @param   pcb             Where to store the size of the storage backend.
     */
    DECLR3CALLBACKMEMBER(int, pfnGetSize, (void *pvUser, void *pvStorage, uint64_t *pcb));

    /**
     * Sets the size of the opened storage backend if possible.
     *
     * @return  VBox status code.
     * @retval  VERR_NOT_SUPPORTED if the backend does not support this operation.
     * @param   pvUser          The opaque data passed on container creation.
     * @param   pvStorage       The opaque storage handle to set the size for.
     * @param   cb              The new size of the image.
     *
     * @note Depending on the host the underlying storage (backing file, etc.)
     *       might not have all required storage allocated (sparse file) which
     *       can delay writes or fail with a not enough free space error if there
     *       is not enough space on the storage medium when writing to the range for
     *       the first time.
     *       Use VDINTERFACEIO::pfnSetAllocationSize to make sure the storage is
     *       really alloacted.
     */
    DECLR3CALLBACKMEMBER(int, pfnSetSize, (void *pvUser, void *pvStorage, uint64_t cb));

    /**
     * Sets the size of the opened storage backend making sure the given size
     * is really allocated.
     *
     * @return VBox status code.
     * @retval VERR_NOT_SUPPORTED if the implementer of the interface doesn't support
     *         this method.
     * @param  pvUser          The opaque data passed on container creation.
     * @param  pvStorage       The storage handle.
     * @param  cbSize          The new size of the image.
     * @param  fFlags          Flags for controlling the allocation strategy.
     *                         Reserved for future use, MBZ.
     */
    DECLR3CALLBACKMEMBER(int, pfnSetAllocationSize, (void *pvUser, void *pvStorage,
                                                     uint64_t cbSize, uint32_t fFlags));

    /**
     * Synchronous write callback.
     *
     * @return  VBox status code.
     * @param   pvUser          The opaque data passed on container creation.
     * @param   pvStorage       The storage handle to use.
     * @param   off             The offset to start from.
     * @param   pvBuf           Pointer to the bits need to be written.
     * @param   cbToWrite       How many bytes to write.
     * @param   pcbWritten      Where to store how many bytes were actually written.
     */
    DECLR3CALLBACKMEMBER(int, pfnWriteSync, (void *pvUser, void *pvStorage, uint64_t off,
                                             const void *pvBuf, size_t cbToWrite, size_t *pcbWritten));

    /**
     * Synchronous read callback.
     *
     * @return  VBox status code.
     * @param   pvUser          The opaque data passed on container creation.
     * @param   pvStorage       The storage handle to use.
     * @param   off             The offset to start from.
     * @param   pvBuf           Where to store the read bits.
     * @param   cbToRead        How many bytes to read.
     * @param   pcbRead         Where to store how many bytes were actually read.
     */
    DECLR3CALLBACKMEMBER(int, pfnReadSync, (void *pvUser, void *pvStorage, uint64_t off,
                                            void *pvBuf, size_t cbToRead, size_t *pcbRead));

    /**
     * Flush data to the storage backend.
     *
     * @return  VBox status code.
     * @param   pvUser          The opaque data passed on container creation.
     * @param   pvStorage       The storage handle to flush.
     */
    DECLR3CALLBACKMEMBER(int, pfnFlushSync, (void *pvUser, void *pvStorage));

    /**
     * Initiate an asynchronous read request.
     *
     * @return  VBox status code.
     * @param   pvUser         The opaque user data passed on container creation.
     * @param   pvStorage      The storage handle.
     * @param   uOffset        The offset to start reading from.
     * @param   paSegments     Scatter gather list to store the data in.
     * @param   cSegments      Number of segments in the list.
     * @param   cbRead         How many bytes to read.
     * @param   pvCompletion   The opaque user data which is returned upon completion.
     * @param   ppTask         Where to store the opaque task handle.
     */
    DECLR3CALLBACKMEMBER(int, pfnReadAsync, (void *pvUser, void *pvStorage, uint64_t uOffset,
                                             PCRTSGSEG paSegments, size_t cSegments,
                                             size_t cbRead, void *pvCompletion,
                                             void **ppTask));

    /**
     * Initiate an asynchronous write request.
     *
     * @return  VBox status code.
     * @param   pvUser         The opaque user data passed on conatiner creation.
     * @param   pvStorage      The storage handle.
     * @param   uOffset        The offset to start writing to.
     * @param   paSegments     Scatter gather list of the data to write
     * @param   cSegments      Number of segments in the list.
     * @param   cbWrite        How many bytes to write.
     * @param   pvCompletion   The opaque user data which is returned upon completion.
     * @param   ppTask         Where to store the opaque task handle.
     */
    DECLR3CALLBACKMEMBER(int, pfnWriteAsync, (void *pvUser, void *pvStorage, uint64_t uOffset,
                                              PCRTSGSEG paSegments, size_t cSegments,
                                              size_t cbWrite, void *pvCompletion,
                                              void **ppTask));

    /**
     * Initiates an async flush request.
     *
     * @return  VBox status code.
     * @param   pvUser          The opaque data passed on container creation.
     * @param   pvStorage       The storage handle to flush.
     * @param   pvCompletion    The opaque user data which is returned upon completion.
     * @param   ppTask          Where to store the opaque task handle.
     */
    DECLR3CALLBACKMEMBER(int, pfnFlushAsync, (void *pvUser, void *pvStorage,
                                              void *pvCompletion, void **ppTask));

} VDINTERFACEIO, *PVDINTERFACEIO;

/**
 * Get I/O interface from interface list.
 *
 * @return Pointer to the first I/O interface in the list.
 * @param  pVDIfs    Pointer to the interface list.
 */
DECLINLINE(PVDINTERFACEIO) VDIfIoGet(PVDINTERFACE pVDIfs)
{
    PVDINTERFACE pIf = VDInterfaceGet(pVDIfs, VDINTERFACETYPE_IO);

    /* Check that the interface descriptor is a progress interface. */
    AssertMsgReturn(   !pIf
                    || (   (pIf->enmInterface == VDINTERFACETYPE_IO)
                        && (pIf->cbSize == sizeof(VDINTERFACEIO))),
                    ("Not a I/O interface"), NULL);

    return (PVDINTERFACEIO)pIf;
}

DECLINLINE(int) vdIfIoFileOpen(PVDINTERFACEIO pIfIo, const char *pszFilename,
                               uint32_t fOpen, PFNVDCOMPLETED pfnCompleted,
                               void **ppStorage)
{
    return pIfIo->pfnOpen(pIfIo->Core.pvUser, pszFilename, fOpen, pfnCompleted, ppStorage);
}

DECLINLINE(int) vdIfIoFileClose(PVDINTERFACEIO pIfIo, void *pStorage)
{
    return pIfIo->pfnClose(pIfIo->Core.pvUser, pStorage);
}

DECLINLINE(int) vdIfIoFileDelete(PVDINTERFACEIO pIfIo, const char *pszFilename)
{
    return pIfIo->pfnDelete(pIfIo->Core.pvUser, pszFilename);
}

DECLINLINE(int) vdIfIoFileMove(PVDINTERFACEIO pIfIo, const char *pszSrc,
                               const char *pszDst, unsigned fMove)
{
    return pIfIo->pfnMove(pIfIo->Core.pvUser, pszSrc, pszDst, fMove);
}

DECLINLINE(int) vdIfIoFileGetFreeSpace(PVDINTERFACEIO pIfIo, const char *pszFilename,
                                       int64_t *pcbFree)
{
    return pIfIo->pfnGetFreeSpace(pIfIo->Core.pvUser, pszFilename, pcbFree);
}

DECLINLINE(int) vdIfIoFileGetModificationTime(PVDINTERFACEIO pIfIo, const char *pcszFilename,
                                              PRTTIMESPEC pModificationTime)
{
    return pIfIo->pfnGetModificationTime(pIfIo->Core.pvUser, pcszFilename,
                                         pModificationTime);
}

DECLINLINE(int) vdIfIoFileGetSize(PVDINTERFACEIO pIfIo, void *pStorage,
                                  uint64_t *pcbSize)
{
    return pIfIo->pfnGetSize(pIfIo->Core.pvUser, pStorage, pcbSize);
}

DECLINLINE(int) vdIfIoFileSetSize(PVDINTERFACEIO pIfIo, void *pStorage,
                                  uint64_t cbSize)
{
    return pIfIo->pfnSetSize(pIfIo->Core.pvUser, pStorage, cbSize);
}

DECLINLINE(int) vdIfIoFileWriteSync(PVDINTERFACEIO pIfIo, void *pStorage,
                                    uint64_t uOffset, const void *pvBuffer, size_t cbBuffer,
                                    size_t *pcbWritten)
{
    return pIfIo->pfnWriteSync(pIfIo->Core.pvUser, pStorage, uOffset,
                               pvBuffer, cbBuffer, pcbWritten);
}

DECLINLINE(int) vdIfIoFileReadSync(PVDINTERFACEIO pIfIo, void *pStorage,
                                   uint64_t uOffset, void *pvBuffer, size_t cbBuffer,
                                   size_t *pcbRead)
{
    return pIfIo->pfnReadSync(pIfIo->Core.pvUser, pStorage, uOffset,
                              pvBuffer, cbBuffer, pcbRead);
}

DECLINLINE(int) vdIfIoFileFlushSync(PVDINTERFACEIO pIfIo, void *pStorage)
{
    return pIfIo->pfnFlushSync(pIfIo->Core.pvUser, pStorage);
}

/**
 * Create a VFS stream handle around a VD I/O interface.
 *
 * The I/O interface will not be closed or free by the stream, the caller will
 * do so after it is done with the stream and has released the instances of the
 * I/O stream object returned by this API.
 *
 * @return  VBox status code.
 * @param   pVDIfsIo        Pointer to the VD I/O interface.
 * @param   pvStorage       The storage argument to pass to the interface
 *                          methods.
 * @param   fFlags          RTFILE_O_XXX, access mask requied.
 * @param   phVfsIos        Where to return the VFS I/O stream handle on
 *                          success.
 */
VBOXDDU_DECL(int) VDIfCreateVfsStream(PVDINTERFACEIO pVDIfsIo, void *pvStorage, uint32_t fFlags, PRTVFSIOSTREAM phVfsIos);

struct VDINTERFACEIOINT;

/**
 * Create a VFS file handle around a VD I/O interface.
 *
 * The I/O interface will not be closed or free by the VFS file, the caller will
 * do so after it is done with the VFS file and has released the instances of
 * the VFS object returned by this API.
 *
 * @return  VBox status code.
 * @param   pVDIfs          Pointer to the VD I/O interface.  If NULL, then @a
 *                          pVDIfsInt must be specified.
 * @param   pVDIfsInt       Pointer to the internal VD I/O interface.  If NULL,
 *                          then @ pVDIfs must be specified.
 * @param   pvStorage       The storage argument to pass to the interface
 *                          methods.
 * @param   fFlags          RTFILE_O_XXX, access mask requied.
 * @param   phVfsFile       Where to return the VFS file handle on success.
 */
VBOXDDU_DECL(int) VDIfCreateVfsFile(PVDINTERFACEIO pVDIfs, struct VDINTERFACEIOINT *pVDIfsInt, void *pvStorage,
                                    uint32_t fFlags, PRTVFSFILE phVfsFile);

/**
 * Creates an VD I/O interface wrapper around an IPRT VFS I/O stream.
 *
 * @return  VBox status code.
 * @param   hVfsIos         The IPRT VFS I/O stream handle. The handle will be
 *                          retained by the returned I/O interface (released on
 *                          close or destruction).
 * @param   fAccessMode     The access mode (RTFILE_O_ACCESS_MASK) to accept.
 * @param   ppIoIf          Where to return the pointer to the VD I/O interface.
 *                          This must be passed to VDIfDestroyFromVfsStream().
 */
VBOXDDU_DECL(int) VDIfCreateFromVfsStream(RTVFSIOSTREAM hVfsIos, uint32_t fAccessMode, PVDINTERFACEIO *ppIoIf);

/**
 * Destroys the VD I/O interface returned by VDIfCreateFromVfsStream.
 *
 * @returns VBox status code.
 * @param   pIoIf           The I/O interface pointer returned by
 *                          VDIfCreateFromVfsStream.  NULL will be quietly
 *                          ignored.
 */
VBOXDDU_DECL(int) VDIfDestroyFromVfsStream(PVDINTERFACEIO pIoIf);


/**
 * Callback which provides progress information about a currently running
 * lengthy operation.
 *
 * @return  VBox status code.
 * @param   pvUser          The opaque user data associated with this interface.
 * @param   uPercentage     Completion percentage.
 */
typedef DECLCALLBACKTYPE(int, FNVDPROGRESS,(void *pvUser, unsigned uPercentage));
/** Pointer to FNVDPROGRESS() */
typedef FNVDPROGRESS *PFNVDPROGRESS;

/**
 * Progress notification interface
 *
 * Per-operation. Optional.
 */
typedef struct VDINTERFACEPROGRESS
{
    /**
     * Common interface header.
     */
    VDINTERFACE    Core;

    /**
     * Progress notification callbacks.
     */
    PFNVDPROGRESS pfnProgress;

} VDINTERFACEPROGRESS, *PVDINTERFACEPROGRESS;

/** Initializer for VDINTERFACEPROGRESS.  */
#define VDINTERFACEPROGRESS_INITALIZER(a_pfnProgress) { { 0, NULL, NULL, VDINTERFACETYPE_INVALID, 0, NULL }, a_pfnProgress }

/**
 * Get progress interface from interface list.
 *
 * @return Pointer to the first progress interface in the list.
 * @param  pVDIfs    Pointer to the interface list.
 */
DECLINLINE(PVDINTERFACEPROGRESS) VDIfProgressGet(PVDINTERFACE pVDIfs)
{
    PVDINTERFACE pIf = VDInterfaceGet(pVDIfs, VDINTERFACETYPE_PROGRESS);

    /* Check that the interface descriptor is a progress interface. */
    AssertMsgReturn(   !pIf
                    || (   (pIf->enmInterface == VDINTERFACETYPE_PROGRESS)
                        && (pIf->cbSize == sizeof(VDINTERFACEPROGRESS))),
                    ("Not a progress interface"), NULL);

    return (PVDINTERFACEPROGRESS)pIf;
}

/**
 * Signal new progress information to the frontend.
 *
 * @returns VBox status code.
 * @param   pIfProgress        The progress interface.
 * @param   uPercentage        Completion percentage.
 */
DECLINLINE(int) vdIfProgress(PVDINTERFACEPROGRESS pIfProgress, unsigned uPercentage)
{
    if (pIfProgress)
        return pIfProgress->pfnProgress(pIfProgress->Core.pvUser, uPercentage);
    return VINF_SUCCESS;
}

/**
 * Configuration information interface
 *
 * Per-image. Optional for most backends, but mandatory for images which do
 * not operate on files (including standard block or character devices).
 */
typedef struct VDINTERFACECONFIG
{
    /**
     * Common interface header.
     */
    VDINTERFACE    Core;

    /**
     * Validates that the keys are within a set of valid names.
     *
     * @return  true if all key names are found in pszzAllowed.
     * @return  false if not.
     * @param   pvUser          The opaque user data associated with this interface.
     * @param   pszzValid       List of valid key names separated by '\\0' and ending with
     *                          a double '\\0'.
     */
    DECLR3CALLBACKMEMBER(bool, pfnAreKeysValid, (void *pvUser, const char *pszzValid));

    /**
     * Retrieves the length of the string value associated with a key (including
     * the terminator, for compatibility with CFGMR3QuerySize).
     *
     * @return  VBox status code.
     *          VERR_CFGM_VALUE_NOT_FOUND means that the key is not known.
     * @param   pvUser          The opaque user data associated with this interface.
     * @param   pszName         Name of the key to query.
     * @param   pcbValue        Where to store the value length. Non-NULL.
     */
    DECLR3CALLBACKMEMBER(int, pfnQuerySize, (void *pvUser, const char *pszName, size_t *pcbValue));

    /**
     * Query the string value associated with a key.
     *
     * @return  VBox status code.
     *          VERR_CFGM_VALUE_NOT_FOUND means that the key is not known.
     *          VERR_CFGM_NOT_ENOUGH_SPACE means that the buffer is not big enough.
     * @param   pvUser          The opaque user data associated with this interface.
     * @param   pszName         Name of the key to query.
     * @param   pszValue        Pointer to buffer where to store value.
     * @param   cchValue        Length of value buffer.
     */
    DECLR3CALLBACKMEMBER(int, pfnQuery, (void *pvUser, const char *pszName, char *pszValue, size_t cchValue));

    /**
     * Query the bytes value associated with a key.
     *
     * @return  VBox status code.
     *          VERR_CFGM_VALUE_NOT_FOUND means that the key is not known.
     *          VERR_CFGM_NOT_ENOUGH_SPACE means that the buffer is not big enough.
     * @param   pvUser          The opaque user data associated with this interface.
     * @param   pszName         Name of the key to query.
     * @param   ppvData         Pointer to buffer where to store the data.
     * @param   cbData          Length of data buffer.
     */
    DECLR3CALLBACKMEMBER(int, pfnQueryBytes, (void *pvUser, const char *pszName, void *ppvData, size_t cbData));

    /**
     * Set a named property to a specified string value, optionally creating if it doesn't exist.
     *
     * @return  VBox status code.
     *          VERR_CFGM_VALUE_NOT_FOUND means that the key is not known and fCreate flag was not set.
     * @param   pvUser          The opaque user data associated with this interface.
     * @param   fCreate         Create property if it doesn't exist (if property exists, it is not an error)
     * @param   pszName         Name of the key to query.
     * @param   pszValue        String value to set the name property to.
     */
    DECLR3CALLBACKMEMBER(int, pfnUpdate, (void *pvUser, bool fCreate,
            const char *pszName, const char *pszValue));

} VDINTERFACECONFIG, *PVDINTERFACECONFIG;

/**
 * Get configuration information interface from interface list.
 *
 * @return Pointer to the first configuration information interface in the list.
 * @param  pVDIfs    Pointer to the interface list.
 */
DECLINLINE(PVDINTERFACECONFIG) VDIfConfigGet(PVDINTERFACE pVDIfs)
{
    PVDINTERFACE pIf = VDInterfaceGet(pVDIfs, VDINTERFACETYPE_CONFIG);

    /* Check that the interface descriptor is a progress interface. */
    AssertMsgReturn(   !pIf
                    || (   (pIf->enmInterface == VDINTERFACETYPE_CONFIG)
                        && (pIf->cbSize == sizeof(VDINTERFACECONFIG))),
                    ("Not a config interface"), NULL);

    return (PVDINTERFACECONFIG)pIf;
}

/**
 * Query configuration, validates that the keys are within a set of valid names.
 *
 * @return  true if all key names are found in pszzAllowed.
 * @return  false if not.
 * @param   pCfgIf      Pointer to configuration callback table.
 * @param   pszzValid   List of valid names separated by '\\0' and ending with
 *                      a double '\\0'.
 */
DECLINLINE(bool) VDCFGAreKeysValid(PVDINTERFACECONFIG pCfgIf, const char *pszzValid)
{
    return pCfgIf->pfnAreKeysValid(pCfgIf->Core.pvUser, pszzValid);
}

/**
 * Checks whether a given key is existing.
 *
 * @return  true if the key exists.
 * @return  false if the key does not exist.
 * @param   pCfgIf      Pointer to configuration callback table.
 * @param   pszName     Name of the key.
 */
DECLINLINE(bool) VDCFGIsKeyExisting(PVDINTERFACECONFIG pCfgIf, const char *pszName)
{
    size_t cb = 0;
    int rc = pCfgIf->pfnQuerySize(pCfgIf->Core.pvUser, pszName, &cb);
    return rc == VERR_CFGM_VALUE_NOT_FOUND ? false : true;
}

/**
 * Query configuration, unsigned 64-bit integer value with default.
 *
 * @return  VBox status code.
 * @param   pCfgIf      Pointer to configuration callback table.
 * @param   pszName     Name of an integer value
 * @param   pu64        Where to store the value. Set to default on failure.
 * @param   u64Def      The default value.
 */
DECLINLINE(int) VDCFGQueryU64Def(PVDINTERFACECONFIG pCfgIf,
                                 const char *pszName, uint64_t *pu64,
                                 uint64_t u64Def)
{
    char aszBuf[32];
    int rc = pCfgIf->pfnQuery(pCfgIf->Core.pvUser, pszName, aszBuf, sizeof(aszBuf));
    if (RT_SUCCESS(rc))
    {
        rc = RTStrToUInt64Full(aszBuf, 0, pu64);
    }
    else if (rc == VERR_CFGM_VALUE_NOT_FOUND)
    {
        rc = VINF_SUCCESS;
        *pu64 = u64Def;
    }
    return rc;
}

/**
 * Query configuration, unsigned 64-bit integer value.
 *
 * @return  VBox status code.
 * @param   pCfgIf      Pointer to configuration callback table.
 * @param   pszName     Name of an integer value
 * @param   pu64        Where to store the value.
 */
DECLINLINE(int) VDCFGQueryU64(PVDINTERFACECONFIG pCfgIf, const char *pszName,
                              uint64_t *pu64)
{
    char aszBuf[32];
    int rc = pCfgIf->pfnQuery(pCfgIf->Core.pvUser, pszName, aszBuf, sizeof(aszBuf));
    if (RT_SUCCESS(rc))
    {
        rc = RTStrToUInt64Full(aszBuf, 0, pu64);
    }

    return rc;
}

/**
 * Query configuration, unsigned 32-bit integer value with default.
 *
 * @return  VBox status code.
 * @param   pCfgIf      Pointer to configuration callback table.
 * @param   pszName     Name of an integer value
 * @param   pu32        Where to store the value. Set to default on failure.
 * @param   u32Def      The default value.
 */
DECLINLINE(int) VDCFGQueryU32Def(PVDINTERFACECONFIG pCfgIf,
                                 const char *pszName, uint32_t *pu32,
                                 uint32_t u32Def)
{
    uint64_t u64;
    int rc = VDCFGQueryU64Def(pCfgIf, pszName, &u64, u32Def);
    if (RT_SUCCESS(rc))
    {
        if (!(u64 & UINT64_C(0xffffffff00000000)))
            *pu32 = (uint32_t)u64;
        else
            rc = VERR_CFGM_INTEGER_TOO_BIG;
    }
    return rc;
}

/**
 * Query configuration, bool value with default.
 *
 * @return  VBox status code.
 * @param   pCfgIf      Pointer to configuration callback table.
 * @param   pszName     Name of an integer value
 * @param   pf          Where to store the value. Set to default on failure.
 * @param   fDef        The default value.
 */
DECLINLINE(int) VDCFGQueryBoolDef(PVDINTERFACECONFIG pCfgIf,
                                  const char *pszName, bool *pf,
                                  bool fDef)
{
    uint64_t u64;
    int rc = VDCFGQueryU64Def(pCfgIf, pszName, &u64, fDef);
    if (RT_SUCCESS(rc))
        *pf = u64 ? true : false;
    return rc;
}

/**
 * Query configuration, bool value.
 *
 * @return  VBox status code.
 * @param   pCfgIf      Pointer to configuration callback table.
 * @param   pszName     Name of an integer value
 * @param   pf          Where to store the value.
 */
DECLINLINE(int) VDCFGQueryBool(PVDINTERFACECONFIG pCfgIf, const char *pszName,
                               bool *pf)
{
    uint64_t u64;
    int rc = VDCFGQueryU64(pCfgIf, pszName, &u64);
    if (RT_SUCCESS(rc))
        *pf = u64 ? true : false;
    return rc;
}

/**
 * Query configuration, dynamically allocated (RTMemAlloc) zero terminated
 * character value.
 *
 * @return  VBox status code.
 * @param   pCfgIf      Pointer to configuration callback table.
 * @param   pszName     Name of an zero terminated character value
 * @param   ppszString  Where to store the string pointer. Not set on failure.
 *                      Free this using RTMemFree().
 */
DECLINLINE(int) VDCFGQueryStringAlloc(PVDINTERFACECONFIG pCfgIf,
                                      const char *pszName, char **ppszString)
{
    size_t cb;
    int rc = pCfgIf->pfnQuerySize(pCfgIf->Core.pvUser, pszName, &cb);
    if (RT_SUCCESS(rc))
    {
        char *pszString = (char *)RTMemAlloc(cb);
        if (pszString)
        {
            rc = pCfgIf->pfnQuery(pCfgIf->Core.pvUser, pszName, pszString, cb);
            if (RT_SUCCESS(rc))
                *ppszString = pszString;
            else
                RTMemFree(pszString);
        }
        else
            rc = VERR_NO_MEMORY;
    }
    return rc;
}

/**
 * Query configuration, dynamically allocated (RTMemAlloc) zero terminated
 * character value with default.
 *
 * @return  VBox status code.
 * @param   pCfgIf      Pointer to configuration callback table.
 * @param   pszName     Name of an zero terminated character value
 * @param   ppszString  Where to store the string pointer. Not set on failure.
 *                      Free this using RTMemFree().
 * @param   pszDef      The default value.
 */
DECLINLINE(int) VDCFGQueryStringAllocDef(PVDINTERFACECONFIG pCfgIf,
                                         const char *pszName,
                                         char **ppszString,
                                         const char *pszDef)
{
    size_t cb;
    int rc = pCfgIf->pfnQuerySize(pCfgIf->Core.pvUser, pszName, &cb);
    if (rc == VERR_CFGM_VALUE_NOT_FOUND || rc == VERR_CFGM_NO_PARENT)
    {
        cb = strlen(pszDef) + 1;
        rc = VINF_SUCCESS;
    }
    if (RT_SUCCESS(rc))
    {
        char *pszString = (char *)RTMemAlloc(cb);
        if (pszString)
        {
            rc = pCfgIf->pfnQuery(pCfgIf->Core.pvUser, pszName, pszString, cb);
            if (rc == VERR_CFGM_VALUE_NOT_FOUND || rc == VERR_CFGM_NO_PARENT)
            {
                memcpy(pszString, pszDef, cb);
                rc = VINF_SUCCESS;
            }
            if (RT_SUCCESS(rc))
                *ppszString = pszString;
            else
                RTMemFree(pszString);
        }
        else
            rc = VERR_NO_MEMORY;
    }
    return rc;
}

/**
 * Query configuration, dynamically allocated (RTMemAlloc) byte string value.
 *
 * @return  VBox status code.
 * @param   pCfgIf      Pointer to configuration callback table.
 * @param   pszName     Name of an zero terminated character value
 * @param   ppvData     Where to store the byte string pointer. Not set on failure.
 *                      Free this using RTMemFree().
 * @param   pcbData     Where to store the byte string length.
 */
DECLINLINE(int) VDCFGQueryBytesAlloc(PVDINTERFACECONFIG pCfgIf,
                                     const char *pszName, void **ppvData, size_t *pcbData)
{
    size_t cb;
    int rc = pCfgIf->pfnQuerySize(pCfgIf->Core.pvUser, pszName, &cb);
    if (RT_SUCCESS(rc))
    {
        char *pbData;
        Assert(cb);

        pbData = (char *)RTMemAlloc(cb);
        if (pbData)
        {
            if(pCfgIf->pfnQueryBytes)
                rc = pCfgIf->pfnQueryBytes(pCfgIf->Core.pvUser, pszName, pbData, cb);
            else
                rc = pCfgIf->pfnQuery(pCfgIf->Core.pvUser, pszName, pbData, cb);

            if (RT_SUCCESS(rc))
            {
                *ppvData = pbData;
                /* Exclude terminator if the byte data was obtained using the string query callback. */
                *pcbData = cb;
                if (!pCfgIf->pfnQueryBytes)
                    (*pcbData)--;
            }
            else
                RTMemFree(pbData);
        }
        else
            rc = VERR_NO_MEMORY;
    }
    return rc;
}

/**
 * Set property value to string (optionally create if non-existent).
 *
 * @return  VBox status code.
 * @param   pCfgIf      Pointer to configuration callback table.
 * @param   fCreate     Create the property if it doesn't exist
 * @param   pszName     Name of property
 * @param   pszValue    String value to assign to property
 */
DECLINLINE(int) VDCFGUpdate(PVDINTERFACECONFIG pCfgIf, bool fCreate, const char *pszName, const char *pszValue)
{
    int rc = pCfgIf->pfnUpdate(pCfgIf->Core.pvUser, fCreate, pszName, pszValue);
    return rc;
}

/**
 * Set property value to Unsigned Int 64-bit (optionally create if non-existent).
 *
 * @return  VBox status code.
 * @param   pCfgIf      Pointer to configuration callback table.
 * @param   fCreate     Create the property if it doesn't exist
 * @param   pszName     Name of property
 * @param   u64Value    64-bit unsigned value to save with property.
 */

DECLINLINE(int) VDCFGUpdateU64(PVDINTERFACECONFIG pCfgIf, bool fCreate, const char *pszName, uint64_t u64Value)
{
     int rc = 0;
     char pszValue[21];
     (void) RTStrPrintf(pszValue, sizeof(pszValue), "%RU64", u64Value);
     rc = VDCFGUpdate(pCfgIf, fCreate, pszName, pszValue);
     return rc;
}



/** Forward declaration of a VD socket. */
typedef struct VDSOCKETINT *VDSOCKET;
/** Pointer to a VD socket. */
typedef VDSOCKET *PVDSOCKET;
/** Nil socket handle. */
#define NIL_VDSOCKET ((VDSOCKET)0)

/** Connect flag to indicate that the backend wants to use the extended
 * socket I/O multiplexing call. This might not be supported on all configurations
 * (internal networking and iSCSI)
 * and the backend needs to take appropriate action.
 */
#define VD_INTERFACETCPNET_CONNECT_EXTENDED_SELECT RT_BIT_32(0)

/** @name Select events
 * @{ */
/** Readable without blocking. */
#define VD_INTERFACETCPNET_EVT_READ         RT_BIT_32(0)
/** Writable without blocking. */
#define VD_INTERFACETCPNET_EVT_WRITE        RT_BIT_32(1)
/** Error condition, hangup, exception or similar. */
#define VD_INTERFACETCPNET_EVT_ERROR        RT_BIT_32(2)
/** Hint for the select that getting interrupted while waiting is more likely.
 * The interface implementation can optimize the waiting strategy based on this.
 * It is assumed that it is more likely to get one of the above socket events
 * instead of being interrupted if the flag is not set. */
#define VD_INTERFACETCPNET_HINT_INTERRUPT   RT_BIT_32(3)
/** Mask of the valid bits. */
#define VD_INTERFACETCPNET_EVT_VALID_MASK   UINT32_C(0x0000000f)
/** @} */

/**
 * TCP network stack interface
 *
 * Per-image. Mandatory for backends which have the VD_CAP_TCPNET bit set.
 */
typedef struct VDINTERFACETCPNET
{
    /**
     * Common interface header.
     */
    VDINTERFACE    Core;

    /**
     * Creates a socket. The socket is not connected if this succeeds.
     *
     * @return  iprt status code.
     * @retval  VERR_NOT_SUPPORTED if the combination of flags is not supported.
     * @param   fFlags    Combination of the VD_INTERFACETCPNET_CONNECT_* \#defines.
     * @param   phVdSock  Where to store the handle.
     */
    DECLR3CALLBACKMEMBER(int, pfnSocketCreate, (uint32_t fFlags, PVDSOCKET phVdSock));

    /**
     * Destroys the socket.
     *
     * @return iprt status code.
     * @param  hVdSock    Socket handle (/ pointer).
     */
    DECLR3CALLBACKMEMBER(int, pfnSocketDestroy, (VDSOCKET hVdSock));

    /**
     * Connect as a client to a TCP port.
     *
     * @return  iprt status code.
     * @param   hVdSock         Socket handle (/ pointer)..
     * @param   pszAddress      The address to connect to.
     * @param   uPort           The port to connect to.
     * @param   cMillies        Number of milliseconds to wait for the connect attempt to complete.
     *                          Use RT_INDEFINITE_WAIT to wait for ever.
     *                          Use RT_SOCKETCONNECT_DEFAULT_WAIT to wait for the default time
     *                          configured on the running system.
     */
    DECLR3CALLBACKMEMBER(int, pfnClientConnect, (VDSOCKET hVdSock, const char *pszAddress, uint32_t uPort,
                                                 RTMSINTERVAL cMillies));

    /**
     * Close a TCP connection.
     *
     * @return  iprt status code.
     * @param   hVdSock         Socket handle (/ pointer).
     */
    DECLR3CALLBACKMEMBER(int, pfnClientClose, (VDSOCKET hVdSock));

    /**
     * Returns whether the socket is currently connected to the client.
     *
     * @returns true if the socket is connected.
     *          false otherwise.
     * @param   hVdSock     Socket handle (/ pointer).
     */
    DECLR3CALLBACKMEMBER(bool, pfnIsClientConnected, (VDSOCKET hVdSock));

    /**
     * Socket I/O multiplexing.
     * Checks if the socket is ready for reading.
     *
     * @return  iprt status code.
     * @param   hVdSock     Socket handle (/ pointer).
     * @param   cMillies    Number of milliseconds to wait for the socket.
     *                      Use RT_INDEFINITE_WAIT to wait for ever.
     */
    DECLR3CALLBACKMEMBER(int, pfnSelectOne, (VDSOCKET hVdSock, RTMSINTERVAL cMillies));

    /**
     * Receive data from a socket.
     *
     * @return  iprt status code.
     * @param   hVdSock     Socket handle (/ pointer).
     * @param   pvBuffer    Where to put the data we read.
     * @param   cbBuffer    Read buffer size.
     * @param   pcbRead     Number of bytes read.
     *                      If NULL the entire buffer will be filled upon successful return.
     *                      If not NULL a partial read can be done successfully.
     */
    DECLR3CALLBACKMEMBER(int, pfnRead, (VDSOCKET hVdSock, void *pvBuffer, size_t cbBuffer, size_t *pcbRead));

    /**
     * Send data to a socket.
     *
     * @return  iprt status code.
     * @param   hVdSock     Socket handle (/ pointer).
     * @param   pvBuffer    Buffer to write data to socket.
     * @param   cbBuffer    How much to write.
     */
    DECLR3CALLBACKMEMBER(int, pfnWrite, (VDSOCKET hVdSock, const void *pvBuffer, size_t cbBuffer));

    /**
     * Send data from scatter/gather buffer to a socket.
     *
     * @return  iprt status code.
     * @param   hVdSock     Socket handle (/ pointer).
     * @param   pSgBuf      Scatter/gather buffer to write data to socket.
     */
    DECLR3CALLBACKMEMBER(int, pfnSgWrite, (VDSOCKET hVdSock, PCRTSGBUF pSgBuf));

    /**
     * Receive data from a socket - not blocking.
     *
     * @return  iprt status code.
     * @param   hVdSock     Socket handle (/ pointer).
     * @param   pvBuffer    Where to put the data we read.
     * @param   cbBuffer    Read buffer size.
     * @param   pcbRead     Number of bytes read.
     */
    DECLR3CALLBACKMEMBER(int, pfnReadNB, (VDSOCKET hVdSock, void *pvBuffer, size_t cbBuffer, size_t *pcbRead));

    /**
     * Send data to a socket - not blocking.
     *
     * @return  iprt status code.
     * @param   hVdSock     Socket handle (/ pointer).
     * @param   pvBuffer    Buffer to write data to socket.
     * @param   cbBuffer    How much to write.
     * @param   pcbWritten  Number of bytes written.
     */
    DECLR3CALLBACKMEMBER(int, pfnWriteNB, (VDSOCKET hVdSock, const void *pvBuffer, size_t cbBuffer, size_t *pcbWritten));

    /**
     * Send data from scatter/gather buffer to a socket - not blocking.
     *
     * @return  iprt status code.
     * @param   hVdSock     Socket handle (/ pointer).
     * @param   pSgBuf      Scatter/gather buffer to write data to socket.
     * @param   pcbWritten  Number of bytes written.
     */
    DECLR3CALLBACKMEMBER(int, pfnSgWriteNB, (VDSOCKET hVdSock, PRTSGBUF pSgBuf, size_t *pcbWritten));

    /**
     * Flush socket write buffers.
     *
     * @return  iprt status code.
     * @param   hVdSock     Socket handle (/ pointer).
     */
    DECLR3CALLBACKMEMBER(int, pfnFlush, (VDSOCKET hVdSock));

    /**
     * Enables or disables delaying sends to coalesce packets.
     *
     * @return  iprt status code.
     * @param   hVdSock     Socket handle (/ pointer).
     * @param   fEnable     When set to true enables coalescing.
     */
    DECLR3CALLBACKMEMBER(int, pfnSetSendCoalescing, (VDSOCKET hVdSock, bool fEnable));

    /**
     * Gets the address of the local side.
     *
     * @return  iprt status code.
     * @param   hVdSock     Socket handle (/ pointer).
     * @param   pAddr       Where to store the local address on success.
     */
    DECLR3CALLBACKMEMBER(int, pfnGetLocalAddress, (VDSOCKET hVdSock, PRTNETADDR pAddr));

    /**
     * Gets the address of the other party.
     *
     * @return  iprt status code.
     * @param   hVdSock     Socket handle (/ pointer).
     * @param   pAddr       Where to store the peer address on success.
     */
    DECLR3CALLBACKMEMBER(int, pfnGetPeerAddress, (VDSOCKET hVdSock, PRTNETADDR pAddr));

    /**
     * Socket I/O multiplexing - extended version which can be woken up.
     * Checks if the socket is ready for reading or writing.
     *
     * @return  iprt status code.
     * @retval  VERR_INTERRUPTED if the thread was woken up by a pfnPoke call.
     * @param   hVdSock     VD Socket handle(/pointer).
     * @param   fEvents     Mask of events to wait for.
     * @param   pfEvents    Where to store the received events.
     * @param   cMillies    Number of milliseconds to wait for the socket.
     *                      Use RT_INDEFINITE_WAIT to wait for ever.
     */
    DECLR3CALLBACKMEMBER(int, pfnSelectOneEx, (VDSOCKET hVdSock, uint32_t fEvents,
                                               uint32_t *pfEvents, RTMSINTERVAL cMillies));

    /**
     * Wakes up the thread waiting in pfnSelectOneEx.
     *
     * @return iprt status code.
     * @param  hVdSock      VD Socket handle(/pointer).
     */
    DECLR3CALLBACKMEMBER(int, pfnPoke, (VDSOCKET hVdSock));

} VDINTERFACETCPNET, *PVDINTERFACETCPNET;

/**
 * Get TCP network stack interface from interface list.
 *
 * @return Pointer to the first TCP network stack interface in the list.
 * @param  pVDIfs    Pointer to the interface list.
 */
DECLINLINE(PVDINTERFACETCPNET) VDIfTcpNetGet(PVDINTERFACE pVDIfs)
{
    PVDINTERFACE pIf = VDInterfaceGet(pVDIfs, VDINTERFACETYPE_TCPNET);

    /* Check that the interface descriptor is a progress interface. */
    AssertMsgReturn(   !pIf
                    || (   (pIf->enmInterface == VDINTERFACETYPE_TCPNET)
                        && (pIf->cbSize == sizeof(VDINTERFACETCPNET))),
                    ("Not a TCP net interface"), NULL);

    return (PVDINTERFACETCPNET)pIf;
}


/**
 * Interface to synchronize concurrent accesses by several threads.
 *
 * @note The scope of this interface is to manage concurrent accesses after
 * the HDD container has been created, and they must stop before destroying the
 * container. Opening or closing images is covered by the synchronization, but
 * that does not mean it is safe to close images while a thread executes
 * #VDMerge or #VDCopy operating on these images. Making them safe would require
 * the lock to be held during the entire operation, which prevents other
 * concurrent acitivities.
 *
 * @note Right now this is kept as simple as possible, and does not even
 * attempt to provide enough information to allow e.g. concurrent write
 * accesses to different areas of the disk. The reason is that it is very
 * difficult to predict which area of a disk is affected by a write,
 * especially when different image formats are mixed. Maybe later a more
 * sophisticated interface will be provided which has the necessary information
 * about worst case affected areas.
 *
 * Per-disk interface. Optional, needed if the disk is accessed concurrently
 * by several threads, e.g. when merging diff images while a VM is running.
 */
typedef struct VDINTERFACETHREADSYNC
{
    /**
     * Common interface header.
     */
    VDINTERFACE    Core;

    /**
     * Start a read operation.
     */
    DECLR3CALLBACKMEMBER(int, pfnStartRead, (void *pvUser));

    /**
     * Finish a read operation.
     */
    DECLR3CALLBACKMEMBER(int, pfnFinishRead, (void *pvUser));

    /**
     * Start a write operation.
     */
    DECLR3CALLBACKMEMBER(int, pfnStartWrite, (void *pvUser));

    /**
     * Finish a write operation.
     */
    DECLR3CALLBACKMEMBER(int, pfnFinishWrite, (void *pvUser));

} VDINTERFACETHREADSYNC, *PVDINTERFACETHREADSYNC;

/**
 * Get thread synchronization interface from interface list.
 *
 * @return Pointer to the first thread synchronization interface in the list.
 * @param  pVDIfs    Pointer to the interface list.
 */
DECLINLINE(PVDINTERFACETHREADSYNC) VDIfThreadSyncGet(PVDINTERFACE pVDIfs)
{
    PVDINTERFACE pIf = VDInterfaceGet(pVDIfs, VDINTERFACETYPE_THREADSYNC);

    /* Check that the interface descriptor is a progress interface. */
    AssertMsgReturn(   !pIf
                    || (   (pIf->enmInterface == VDINTERFACETYPE_THREADSYNC)
                        && (pIf->cbSize == sizeof(VDINTERFACETHREADSYNC))),
                    ("Not a thread synchronization interface"), NULL);

    return (PVDINTERFACETHREADSYNC)pIf;
}

/**
 * Interface to query usage of disk ranges.
 *
 * Per-operation interface. Optional.
 */
typedef struct VDINTERFACEQUERYRANGEUSE
{
    /**
     * Common interface header.
     */
    VDINTERFACE    Core;

    /**
     * Query use of a disk range.
     */
    DECLR3CALLBACKMEMBER(int, pfnQueryRangeUse, (void *pvUser, uint64_t off, uint64_t cb,
                                                 bool *pfUsed));

} VDINTERFACEQUERYRANGEUSE, *PVDINTERFACEQUERYRANGEUSE;

/**
 * Get query range use interface from interface list.
 *
 * @return Pointer to the first thread synchronization interface in the list.
 * @param  pVDIfs    Pointer to the interface list.
 */
DECLINLINE(PVDINTERFACEQUERYRANGEUSE) VDIfQueryRangeUseGet(PVDINTERFACE pVDIfs)
{
    PVDINTERFACE pIf = VDInterfaceGet(pVDIfs, VDINTERFACETYPE_QUERYRANGEUSE);

    /* Check that the interface descriptor is a progress interface. */
    AssertMsgReturn(   !pIf
                    || (   (pIf->enmInterface == VDINTERFACETYPE_QUERYRANGEUSE)
                        && (pIf->cbSize == sizeof(VDINTERFACEQUERYRANGEUSE))),
                    ("Not a query range use interface"), NULL);

    return (PVDINTERFACEQUERYRANGEUSE)pIf;
}

DECLINLINE(int) vdIfQueryRangeUse(PVDINTERFACEQUERYRANGEUSE pIfQueryRangeUse, uint64_t off, uint64_t cb,
                                  bool *pfUsed)
{
    return pIfQueryRangeUse->pfnQueryRangeUse(pIfQueryRangeUse->Core.pvUser, off, cb, pfUsed);
}


/**
 * Interface used to retrieve keys for cryptographic operations.
 *
 * Per-module interface. Optional but cryptographic modules might fail and
 * return an error if this is not present.
 */
typedef struct VDINTERFACECRYPTO
{
    /**
     * Common interface header.
     */
    VDINTERFACE    Core;

    /**
     * Retains a key identified by the ID. The caller will only hold a reference
     * to the key and must not modify the key buffer in any way.
     *
     * @returns VBox status code.
     * @param   pvUser          The opaque user data associated with this interface.
     * @param   pszId           The alias/id for the key to retrieve.
     * @param   ppbKey          Where to store the pointer to the key buffer on success.
     * @param   pcbKey          Where to store the size of the key in bytes on success.
     */
    DECLR3CALLBACKMEMBER(int, pfnKeyRetain, (void *pvUser, const char *pszId, const uint8_t **ppbKey, size_t *pcbKey));

    /**
     * Releases one reference of the key identified by the given identifier.
     * The caller must not access the key buffer after calling this operation.
     *
     * @returns VBox status code.
     * @param   pvUser          The opaque user data associated with this interface.
     * @param   pszId           The alias/id for the key to release.
     *
     * @note  It is advised to release the key whenever it is not used anymore so
     *        the entity storing the key can do anything to make retrieving the key
     *        from memory more difficult like scrambling the memory buffer for
     *        instance.
     */
    DECLR3CALLBACKMEMBER(int, pfnKeyRelease, (void *pvUser, const char *pszId));

    /**
     * Gets a reference to the password identified by the given ID to open a key store supplied through the config interface.
     *
     * @returns VBox status code.
     * @param   pvUser          The opaque user data associated with this interface.
     * @param   pszId           The alias/id for the password to retain.
     * @param   ppszPassword    Where to store the password to unlock the key store on success.
     */
    DECLR3CALLBACKMEMBER(int, pfnKeyStorePasswordRetain, (void *pvUser, const char *pszId, const char **ppszPassword));

    /**
     * Releases a reference of the password previously acquired with VDINTERFACECRYPTO::pfnKeyStorePasswordRetain()
     * identified by the given ID.
     *
     * @returns VBox status code.
     * @param   pvUser          The opaque user data associated with this interface.
     * @param   pszId           The alias/id for the password to release.
     */
    DECLR3CALLBACKMEMBER(int, pfnKeyStorePasswordRelease, (void *pvUser, const char *pszId));

    /**
     * Saves a key store.
     *
     * @returns VBox status code.
     * @param   pvUser          The opaque user data associated with this interface.
     * @param   pvKeyStore      The key store to save.
     * @param   cbKeyStore      Size of the key store in bytes.
     *
     * @note The format is filter specific and should be treated as binary data.
     */
    DECLR3CALLBACKMEMBER(int, pfnKeyStoreSave, (void *pvUser, const void *pvKeyStore, size_t cbKeyStore));

    /**
     * Returns the parameters after the key store was loaded successfully.
     *
     * @returns VBox status code.
     * @param   pvUser          The opaque user data associated with this interface.
     * @param   pszCipher       The cipher identifier the DEK is used for.
     * @param   pbDek           The raw DEK which was contained in the key store loaded by
     *                          VDINTERFACECRYPTO::pfnKeyStoreLoad().
     * @param   cbDek           The size of the DEK.
     *
     * @note The provided pointer to the DEK is only valid until this call returns.
     *       The content might change afterwards with out notice (when scrambling the key
     *       for further protection for example) or might be even freed.
     *
     * @note This method is optional and can be NULL if the caller does not require the
     *       parameters.
     */
    DECLR3CALLBACKMEMBER(int, pfnKeyStoreReturnParameters, (void *pvUser, const char *pszCipher,
                                                            const uint8_t *pbDek, size_t cbDek));

} VDINTERFACECRYPTO, *PVDINTERFACECRYPTO;


/**
 * Get error interface from interface list.
 *
 * @return Pointer to the first error interface in the list.
 * @param  pVDIfs    Pointer to the interface list.
 */
DECLINLINE(PVDINTERFACECRYPTO) VDIfCryptoGet(PVDINTERFACE pVDIfs)
{
    PVDINTERFACE pIf = VDInterfaceGet(pVDIfs, VDINTERFACETYPE_CRYPTO);

    /* Check that the interface descriptor is a crypto interface. */
    AssertMsgReturn(   !pIf
                    || (   (pIf->enmInterface == VDINTERFACETYPE_CRYPTO)
                        && (pIf->cbSize == sizeof(VDINTERFACECRYPTO))),
                    ("Not an crypto interface\n"), NULL);

    return (PVDINTERFACECRYPTO)pIf;
}

/**
 * Retains a key identified by the ID. The caller will only hold a reference
 * to the key and must not modify the key buffer in any way.
 *
 * @returns VBox status code.
 * @param   pIfCrypto       Pointer to the crypto interface.
 * @param   pszId           The alias/id for the key to retrieve.
 * @param   ppbKey          Where to store the pointer to the key buffer on success.
 * @param   pcbKey          Where to store the size of the key in bytes on success.
 */
DECLINLINE(int) vdIfCryptoKeyRetain(PVDINTERFACECRYPTO pIfCrypto, const char *pszId, const uint8_t **ppbKey, size_t *pcbKey)
{
    return pIfCrypto->pfnKeyRetain(pIfCrypto->Core.pvUser, pszId, ppbKey, pcbKey);
}

/**
 * Releases one reference of the key identified by the given identifier.
 * The caller must not access the key buffer after calling this operation.
 *
 * @returns VBox status code.
 * @param   pIfCrypto       Pointer to the crypto interface.
 * @param   pszId           The alias/id for the key to release.
 *
 * @note  It is advised to release the key whenever it is not used anymore so
 *        the entity storing the key can do anything to make retrieving the key
 *        from memory more difficult like scrambling the memory buffer for
 *        instance.
 */
DECLINLINE(int) vdIfCryptoKeyRelease(PVDINTERFACECRYPTO pIfCrypto, const char *pszId)
{
    return pIfCrypto->pfnKeyRelease(pIfCrypto->Core.pvUser, pszId);
}

/**
 * Gets a reference to the password identified by the given ID to open a key store supplied through the config interface.
 *
 * @returns VBox status code.
 * @param   pIfCrypto       Pointer to the crypto interface.
 * @param   pszId           The alias/id for the password to retain.
 * @param   ppszPassword    Where to store the password to unlock the key store on success.
 */
DECLINLINE(int) vdIfCryptoKeyStorePasswordRetain(PVDINTERFACECRYPTO pIfCrypto, const char *pszId, const char **ppszPassword)
{
    return pIfCrypto->pfnKeyStorePasswordRetain(pIfCrypto->Core.pvUser, pszId, ppszPassword);
}

/**
 * Releases a reference of the password previously acquired with VDINTERFACECRYPTO::pfnKeyStorePasswordRetain()
 * identified by the given ID.
 *
 * @returns VBox status code.
 * @param   pIfCrypto       Pointer to the crypto interface.
 * @param   pszId           The alias/id for the password to release.
 */
DECLINLINE(int) vdIfCryptoKeyStorePasswordRelease(PVDINTERFACECRYPTO pIfCrypto, const char *pszId)
{
    return pIfCrypto->pfnKeyStorePasswordRelease(pIfCrypto->Core.pvUser, pszId);
}

/**
 * Saves a key store.
 *
 * @returns VBox status code.
 * @param   pIfCrypto       Pointer to the crypto interface.
 * @param   pvKeyStore      The key store to save.
 * @param   cbKeyStore      Size of the key store in bytes.
 *
 * @note The format is filter specific and should be treated as binary data.
 */
DECLINLINE(int) vdIfCryptoKeyStoreSave(PVDINTERFACECRYPTO pIfCrypto, const void *pvKeyStore, size_t cbKeyStore)
{
    return pIfCrypto->pfnKeyStoreSave(pIfCrypto->Core.pvUser, pvKeyStore, cbKeyStore);
}

/**
 * Returns the parameters after the key store was loaded successfully.
 *
 * @returns VBox status code.
 * @param   pIfCrypto       Pointer to the crypto interface.
 * @param   pszCipher       The cipher identifier the DEK is used for.
 * @param   pbDek           The raw DEK which was contained in the key store loaded by
 *                          VDINTERFACECRYPTO::pfnKeyStoreLoad().
 * @param   cbDek           The size of the DEK.
 *
 * @note The provided pointer to the DEK is only valid until this call returns.
 *       The content might change afterwards with out notice (when scrambling the key
 *       for further protection for example) or might be even freed.
 *
 * @note This method is optional and can be NULL if the caller does not require the
 *       parameters.
 */
DECLINLINE(int) vdIfCryptoKeyStoreReturnParameters(PVDINTERFACECRYPTO pIfCrypto, const char *pszCipher,
                                                   const uint8_t *pbDek, size_t cbDek)
{
    if (pIfCrypto->pfnKeyStoreReturnParameters)
        return pIfCrypto->pfnKeyStoreReturnParameters(pIfCrypto->Core.pvUser, pszCipher, pbDek, cbDek);

    return VINF_SUCCESS;
}


RT_C_DECLS_END

/** @} */

#endif /* !VBOX_INCLUDED_vd_ifs_h */
