/** @file
 * Internal hard disk format support API for VBoxHDD cache images.
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

#ifndef VBOX_INCLUDED_vd_cache_backend_h
#define VBOX_INCLUDED_vd_cache_backend_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/vd.h>
#include <VBox/vd-common.h>
#include <VBox/vd-ifs-internal.h>

/**
 * Cache format backend interface used by VBox HDD Container implementation.
 */
typedef struct VDCACHEBACKEND
{
    /** Structure version. VD_CACHEBACKEND_VERSION defines the current version. */
    uint32_t            u32Version;
    /** The name of the backend (constant string). */
    const char          *pszBackendName;
    /** The capabilities of the backend. */
    uint64_t            uBackendCaps;

    /**
     * Pointer to a NULL-terminated array of strings, containing the supported
     * file extensions. Note that some backends do not work on files, so this
     * pointer may just contain NULL.
     */
    const char * const *papszFileExtensions;

    /**
     * Pointer to an array of structs describing each supported config key.
     * Terminated by a NULL config key. Note that some backends do not support
     * the configuration interface, so this pointer may just contain NULL.
     * Mandatory if the backend sets VD_CAP_CONFIG.
     */
    PCVDCONFIGINFO      paConfigInfo;

    /**
     * Probes the given image.
     *
     * @returns VBox status code.
     * @param   pszFilename     Name of the image file.
     * @param   pVDIfsDisk      Pointer to the per-disk VD interface list.
     * @param   pVDIfsImage     Pointer to the per-image VD interface list.
     */
    DECLR3CALLBACKMEMBER(int, pfnProbe, (const char *pszFilename, PVDINTERFACE pVDIfsDisk, PVDINTERFACE pVDIfsImage));

    /**
     * Open a cache image.
     *
     * @returns VBox status code.
     * @param   pszFilename     Name of the image file to open. Guaranteed to be available and
     *                          unchanged during the lifetime of this image.
     * @param   uOpenFlags      Image file open mode, see VD_OPEN_FLAGS_* constants.
     * @param   pVDIfsDisk      Pointer to the per-disk VD interface list.
     * @param   pVDIfsImage     Pointer to the per-image VD interface list.
     * @param   ppBackendData   Opaque state data for this image.
     */
    DECLR3CALLBACKMEMBER(int, pfnOpen, (const char *pszFilename, unsigned uOpenFlags,
                                        PVDINTERFACE pVDIfsDisk, PVDINTERFACE pVDIfsImage,
                                        void **ppBackendData));

    /**
     * Create a cache image.
     *
     * @returns VBox status code.
     * @param   pszFilename     Name of the image file to create. Guaranteed to be available and
     *                          unchanged during the lifetime of this image.
     * @param   cbSize          Image size in bytes.
     * @param   uImageFlags     Flags specifying special image features.
     * @param   pszComment      Pointer to image comment. NULL is ok.
     * @param   pUuid           New UUID of the image. Not NULL.
     * @param   uOpenFlags      Image file open mode, see VD_OPEN_FLAGS_* constants.
     * @param   uPercentStart   Starting value for progress percentage.
     * @param   uPercentSpan    Span for varying progress percentage.
     * @param   pVDIfsDisk      Pointer to the per-disk VD interface list.
     * @param   pVDIfsImage     Pointer to the per-image VD interface list.
     * @param   pVDIfsOperation Pointer to the per-operation VD interface list.
     * @param   ppBackendData   Opaque state data for this image.
     */
    DECLR3CALLBACKMEMBER(int, pfnCreate, (const char *pszFilename, uint64_t cbSize,
                                          unsigned uImageFlags, const char *pszComment,
                                          PCRTUUID pUuid, unsigned uOpenFlags,
                                          unsigned uPercentStart, unsigned uPercentSpan,
                                          PVDINTERFACE pVDIfsDisk,
                                          PVDINTERFACE pVDIfsImage,
                                          PVDINTERFACE pVDIfsOperation,
                                          void **ppBackendData));

    /**
     * Close a cache image.
     *
     * @returns VBox status code.
     * @param   pBackendData    Opaque state data for this image.
     * @param   fDelete         If true, delete the image from the host disk.
     */
    DECLR3CALLBACKMEMBER(int, pfnClose, (void *pBackendData, bool fDelete));

    /**
     * Start a read request.
     *
     * @returns VBox status code.
     * @param   pBackendData    Opaque state data for this image.
     * @param   uOffset         The offset of the virtual disk to read from.
     * @param   cbToRead        How many bytes to read.
     * @param   pIoCtx          I/O context associated with this request.
     * @param   pcbActuallyRead Pointer to returned number of bytes read.
     */
    DECLR3CALLBACKMEMBER(int, pfnRead, (void *pBackendData, uint64_t uOffset, size_t cbToRead,
                                        PVDIOCTX pIoCtx, size_t *pcbActuallyRead));

    /**
     * Start a write request.
     *
     * @returns VBox status code.
     * @param   pBackendData    Opaque state data for this image.
     * @param   uOffset         The offset of the virtual disk to write to.
     * @param   cbToWrite       How many bytes to write.
     * @param   pIoCtx          I/O context associated with this request.
     * @param   pcbWriteProcess Pointer to returned number of bytes that could
     *                          be processed. In case the function returned
     *                          VERR_VD_BLOCK_FREE this is the number of bytes
     *                          that could be written in a full block write,
     *                          when prefixed/postfixed by the appropriate
     *                          amount of (previously read) padding data.
     */
    DECLR3CALLBACKMEMBER(int, pfnWrite, (void *pBackendData, uint64_t uOffset, size_t cbToWrite,
                                         PVDIOCTX pIoCtx, size_t *pcbWriteProcess));

    /**
     * Flush data to disk.
     *
     * @returns VBox status code.
     * @param   pBackendData    Opaque state data for this image.
     * @param   pIoCtx          I/O context associated with this request.
     */
    DECLR3CALLBACKMEMBER(int, pfnFlush, (void *pBackendData, PVDIOCTX pIoCtx));

    /**
     * Discards the given amount of bytes from the cache.
     *
     * @returns VBox status code.
     * @retval  VERR_VD_DISCARD_ALIGNMENT_NOT_MET if the range doesn't meet the required alignment
     *          for the discard.
     * @param   pBackendData         Opaque state data for this image.
     * @param   pIoCtx               I/O context associated with this request.
     * @param   uOffset              The offset of the first byte to discard.
     * @param   cbDiscard            How many bytes to discard.
     */
    DECLR3CALLBACKMEMBER(int, pfnDiscard, (void *pBackendData, PVDIOCTX pIoCtx,
                                           uint64_t uOffset, size_t cbDiscard,
                                           size_t *pcbPreAllocated,
                                           size_t *pcbPostAllocated,
                                           size_t *pcbActuallyDiscarded,
                                           void   **ppbmAllocationBitmap,
                                           unsigned fDiscard));

    /**
     * Get the version of a cache image.
     *
     * @returns version of cache image.
     * @param   pBackendData    Opaque state data for this image.
     */
    DECLR3CALLBACKMEMBER(unsigned, pfnGetVersion, (void *pBackendData));

    /**
     * Get the capacity of a cache image.
     *
     * @returns size of cache image in bytes.
     * @param   pBackendData    Opaque state data for this image.
     */
    DECLR3CALLBACKMEMBER(uint64_t, pfnGetSize, (void *pBackendData));

    /**
     * Get the file size of a cache image.
     *
     * @returns size of cache image in bytes.
     * @param   pBackendData    Opaque state data for this image.
     */
    DECLR3CALLBACKMEMBER(uint64_t, pfnGetFileSize, (void *pBackendData));

    /**
     * Get the image flags of a cache image.
     *
     * @returns image flags of cache image.
     * @param   pBackendData    Opaque state data for this image.
     */
    DECLR3CALLBACKMEMBER(unsigned, pfnGetImageFlags, (void *pBackendData));

    /**
     * Get the open flags of a cache image.
     *
     * @returns open flags of cache image.
     * @param   pBackendData    Opaque state data for this image.
     */
    DECLR3CALLBACKMEMBER(unsigned, pfnGetOpenFlags, (void *pBackendData));

    /**
     * Set the open flags of a cache image. May cause the image to be locked
     * in a different mode or be reopened (which can fail).
     *
     * @returns VBox status code.
     * @param   pBackendData    Opaque state data for this image.
     * @param   uOpenFlags      New open flags for this image.
     */
    DECLR3CALLBACKMEMBER(int, pfnSetOpenFlags, (void *pBackendData, unsigned uOpenFlags));

    /**
     * Get comment of a cache image.
     *
     * @returns VBox status code.
     * @param   pBackendData    Opaque state data for this image.
     * @param   pszComment      Where to store the comment.
     * @param   cbComment       Size of the comment buffer.
     */
    DECLR3CALLBACKMEMBER(int, pfnGetComment, (void *pBackendData, char *pszComment, size_t cbComment));

    /**
     * Set comment of a cache image.
     *
     * @returns VBox status code.
     * @param   pBackendData    Opaque state data for this image.
     * @param   pszComment      Where to get the comment from. NULL resets comment.
     *                          The comment is silently truncated if the image format
     *                          limit is exceeded.
     */
    DECLR3CALLBACKMEMBER(int, pfnSetComment, (void *pBackendData, const char *pszComment));

    /**
     * Get UUID of a cache image.
     *
     * @returns VBox status code.
     * @param   pBackendData    Opaque state data for this image.
     * @param   pUuid           Where to store the image UUID.
     */
    DECLR3CALLBACKMEMBER(int, pfnGetUuid, (void *pBackendData, PRTUUID pUuid));

    /**
     * Set UUID of a cache image.
     *
     * @returns VBox status code.
     * @param   pBackendData    Opaque state data for this image.
     * @param   pUuid           Where to get the image UUID from.
     */
    DECLR3CALLBACKMEMBER(int, pfnSetUuid, (void *pBackendData, PCRTUUID pUuid));

    /**
     * Get last modification UUID of a cache image.
     *
     * @returns VBox status code.
     * @param   pBackendData    Opaque state data for this image.
     * @param   pUuid           Where to store the image modification UUID.
     */
    DECLR3CALLBACKMEMBER(int, pfnGetModificationUuid, (void *pBackendData, PRTUUID pUuid));

    /**
     * Set last modification UUID of a cache image.
     *
     * @returns VBox status code.
     * @param   pBackendData    Opaque state data for this image.
     * @param   pUuid           Where to get the image modification UUID from.
     */
    DECLR3CALLBACKMEMBER(int, pfnSetModificationUuid, (void *pBackendData, PCRTUUID pUuid));

    /**
     * Dump information about a cache image.
     *
     * @param   pBackendData    Opaque state data for this image.
     */
    DECLR3CALLBACKMEMBER(void, pfnDump, (void *pBackendData));

    /** Returns a human readable hard disk location string given a
     *  set of hard disk configuration keys. The returned string is an
     *  equivalent of the full file path for image-based hard disks.
     *  Mandatory for backends with no VD_CAP_FILE and NULL otherwise. */
    DECLR3CALLBACKMEMBER(int, pfnComposeLocation, (PVDINTERFACE pConfig, char **pszLocation));

    /** Returns a human readable hard disk name string given a
     *  set of hard disk configuration keys. The returned string is an
     *  equivalent of the file name part in the full file path for
     *  image-based hard disks. Mandatory for backends with no
     *  VD_CAP_FILE and NULL otherwise. */
    DECLR3CALLBACKMEMBER(int, pfnComposeName, (PVDINTERFACE pConfig, char **pszName));

    /** Initialization safty marker. */
    uint32_t            u32VersionEnd;

} VDCACHEBACKEND;
/** Pointer to VD cache backend. */
typedef VDCACHEBACKEND *PVDCACHEBACKEND;
/** Constant pointer to VD backend. */
typedef const VDCACHEBACKEND *PCVDCACHEBACKEND;

/** The current version of the VDCACHEBACKEND structure. */
#define VD_CACHEBACKEND_VERSION                 VD_VERSION_MAKE(0xff03, 1, 0)

#endif /* !VBOX_INCLUDED_vd_cache_backend_h */
