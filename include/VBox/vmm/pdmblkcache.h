/** @file
 * PDM - Pluggable Device Manager, Block cache.
 */

/*
 * Copyright (C) 2007-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_vmm_pdmblkcache_h
#define VBOX_INCLUDED_vmm_pdmblkcache_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#include <iprt/sg.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_pdm_blk_cache  The PDM Block Cache API
 * @ingroup grp_pdm
 * @{
 */

/** Pointer to a PDM block cache. */
typedef struct PDMBLKCACHE *PPDMBLKCACHE;
/** Pointer to a PDM block cache pointer. */
typedef PPDMBLKCACHE *PPPDMBLKCACHE;

/** I/O transfer handle. */
typedef struct PDMBLKCACHEIOXFER *PPDMBLKCACHEIOXFER;

/**
 * Block cache I/O request transfer direction.
 */
typedef enum PDMBLKCACHEXFERDIR
{
    /** Read */
    PDMBLKCACHEXFERDIR_READ = 0,
    /** Write */
    PDMBLKCACHEXFERDIR_WRITE,
    /** Flush */
    PDMBLKCACHEXFERDIR_FLUSH,
    /** Discard */
    PDMBLKCACHEXFERDIR_DISCARD
} PDMBLKCACHEXFERDIR;

/**
 * Completion callback for drivers.
 *
 * @param   pDrvIns         The driver instance.
 * @param   pvUser          User argument given during request initiation.
 * @param   rc              The status code of the completed request.
 */
typedef DECLCALLBACKTYPE(void, FNPDMBLKCACHEXFERCOMPLETEDRV,(PPDMDRVINS pDrvIns, void *pvUser, int rc));
/** Pointer to a FNPDMBLKCACHEXFERCOMPLETEDRV(). */
typedef FNPDMBLKCACHEXFERCOMPLETEDRV *PFNPDMBLKCACHEXFERCOMPLETEDRV;

/**
 * I/O enqueue callback for drivers.
 *
 * @param   pDrvIns         The driver instance.
 * @param   enmXferDir      Transfer direction.
 * @param   off             Transfer offset.
 * @param   cbXfer          Transfer size.
 * @param   pSgBuf          Scather / gather buffer for the transfer.
 * @param   hIoXfer         I/O transfer handle to ping on completion.
 */
typedef DECLCALLBACKTYPE(int, FNPDMBLKCACHEXFERENQUEUEDRV,(PPDMDRVINS pDrvIns, PDMBLKCACHEXFERDIR enmXferDir, uint64_t off,
                                                           size_t cbXfer, PCRTSGBUF pSgBuf, PPDMBLKCACHEIOXFER hIoXfer));
/** Pointer to a FNPDMBLKCACHEXFERENQUEUEDRV(). */
typedef FNPDMBLKCACHEXFERENQUEUEDRV *PFNPDMBLKCACHEXFERENQUEUEDRV;

/**
 * Discard enqueue callback for drivers.
 *
 * @param   pDrvIns         The driver instance.
 * @param   paRanges        Ranges to discard.
 * @param   cRanges         Number of range entries.
 * @param   hIoXfer         I/O handle to return on completion.
 */
typedef DECLCALLBACKTYPE(int, FNPDMBLKCACHEXFERENQUEUEDISCARDDRV,(PPDMDRVINS pDrvIns, PCRTRANGE paRanges, unsigned cRanges,
                                                                  PPDMBLKCACHEIOXFER hIoXfer));
/** Pointer to a FNPDMBLKCACHEXFERENQUEUEDISCARDDRV(). */
typedef FNPDMBLKCACHEXFERENQUEUEDISCARDDRV *PFNPDMBLKCACHEXFERENQUEUEDISCARDDRV;

/**
 * Completion callback for devices.
 *
 * @param   pDevIns         The device instance.
 * @param   pvUser          User argument given during request initiation.
 * @param   rc              The status code of the completed request.
 */
typedef DECLCALLBACKTYPE(void, FNPDMBLKCACHEXFERCOMPLETEDEV,(PPDMDEVINS pDevIns, void *pvUser, int rc));
/** Pointer to a FNPDMBLKCACHEXFERCOMPLETEDEV(). */
typedef FNPDMBLKCACHEXFERCOMPLETEDEV *PFNPDMBLKCACHEXFERCOMPLETEDEV;

/**
 * I/O enqueue callback for devices.
 *
 * @param   pDevIns         The device instance.
 * @param   enmXferDir      Transfer direction.
 * @param   off             Transfer offset.
 * @param   cbXfer          Transfer size.
 * @param   pSgBuf          Scather / gather buffer for the transfer.
 * @param   hIoXfer         I/O transfer handle to ping on completion.
 */
typedef DECLCALLBACKTYPE(int, FNPDMBLKCACHEXFERENQUEUEDEV,(PPDMDEVINS pDevIns, PDMBLKCACHEXFERDIR enmXferDir, uint64_t off,
                                                           size_t cbXfer, PCRTSGBUF pSgBuf, PPDMBLKCACHEIOXFER hIoXfer));
/** Pointer to a FNPDMBLKCACHEXFERENQUEUEDEV(). */
typedef FNPDMBLKCACHEXFERENQUEUEDEV *PFNPDMBLKCACHEXFERENQUEUEDEV;

/**
 * Discard enqueue callback for devices.
 *
 * @param   pDevIns         The device instance.
 * @param   paRanges        Ranges to discard.
 * @param   cRanges         Number of range entries.
 * @param   hIoXfer         I/O handle to return on completion.
 */
typedef DECLCALLBACKTYPE(int, FNPDMBLKCACHEXFERENQUEUEDISCARDDEV,(PPDMDEVINS pDevIns, PCRTRANGE paRanges, unsigned cRanges,
                                                                  PPDMBLKCACHEIOXFER hIoXfer));
/** Pointer to a FNPDMBLKCACHEXFERENQUEUEDISCARDDEV(). */
typedef FNPDMBLKCACHEXFERENQUEUEDISCARDDEV *PFNPDMBLKCACHEXFERENQUEUEDISCARDDEV;

/**
 * Completion callback for drivers.
 *
 * @param   pvUserInt       User argument given to PDMR3BlkCacheRetainInt.
 * @param   pvUser          User argument given during request initiation.
 * @param   rc              The status code of the completed request.
 */
typedef DECLCALLBACKTYPE(void, FNPDMBLKCACHEXFERCOMPLETEINT,(void *pvUserInt, void *pvUser, int rc));
/** Pointer to a FNPDMBLKCACHEXFERCOMPLETEINT(). */
typedef FNPDMBLKCACHEXFERCOMPLETEINT *PFNPDMBLKCACHEXFERCOMPLETEINT;

/**
 * I/O enqueue callback for internal users.
 *
 * @param   pvUser          User data.
 * @param   enmXferDir      Transfer direction.
 * @param   off             Transfer offset.
 * @param   cbXfer          Transfer size.
 * @param   pSgBuf          Scather / gather buffer for the transfer.
 * @param   hIoXfer         I/O transfer handle to ping on completion.
 */
typedef DECLCALLBACKTYPE(int, FNPDMBLKCACHEXFERENQUEUEINT,(void *pvUser, PDMBLKCACHEXFERDIR enmXferDir, uint64_t off,
                                                           size_t cbXfer, PCRTSGBUF pSgBuf, PPDMBLKCACHEIOXFER hIoXfer));
/** Pointer to a FNPDMBLKCACHEXFERENQUEUEINT(). */
typedef FNPDMBLKCACHEXFERENQUEUEINT *PFNPDMBLKCACHEXFERENQUEUEINT;

/**
 * Discard enqueue callback for VMM internal users.
 *
 * @param   pvUser          User data.
 * @param   paRanges        Ranges to discard.
 * @param   cRanges         Number of range entries.
 * @param   hIoXfer         I/O handle to return on completion.
 */
typedef DECLCALLBACKTYPE(int, FNPDMBLKCACHEXFERENQUEUEDISCARDINT,(void *pvUser, PCRTRANGE paRanges, unsigned cRanges,
                                                                  PPDMBLKCACHEIOXFER hIoXfer));
/** Pointer to a FNPDMBLKCACHEXFERENQUEUEDISCARDINT(). */
typedef FNPDMBLKCACHEXFERENQUEUEDISCARDINT *PFNPDMBLKCACHEXFERENQUEUEDISCARDINT;

/**
 * Completion callback for USB devices.
 *
 * @param   pUsbIns         The USB device instance.
 * @param   pvUser          User argument given during request initiation.
 * @param   rc              The status code of the completed request.
 */
typedef DECLCALLBACKTYPE(void, FNPDMBLKCACHEXFERCOMPLETEUSB,(PPDMUSBINS pUsbIns, void *pvUser, int rc));
/** Pointer to a FNPDMBLKCACHEXFERCOMPLETEUSB(). */
typedef FNPDMBLKCACHEXFERCOMPLETEUSB *PFNPDMBLKCACHEXFERCOMPLETEUSB;

/**
 * I/O enqueue callback for USB devices.
 *
 * @param   pUsbIns         The USB device instance.
 * @param   enmXferDir      Transfer direction.
 * @param   off             Transfer offset.
 * @param   cbXfer          Transfer size.
 * @param   pSgBuf          Scather / gather buffer for the transfer.
 * @param   hIoXfer         I/O transfer handle to ping on completion.
 */
typedef DECLCALLBACKTYPE(int, FNPDMBLKCACHEXFERENQUEUEUSB,(PPDMUSBINS pUsbIns, PDMBLKCACHEXFERDIR enmXferDir, uint64_t off,
                                                           size_t cbXfer, PCRTSGBUF pSgBuf, PPDMBLKCACHEIOXFER hIoXfer));
/** Pointer to a FNPDMBLKCACHEXFERENQUEUEUSB(). */
typedef FNPDMBLKCACHEXFERENQUEUEUSB *PFNPDMBLKCACHEXFERENQUEUEUSB;

/**
 * Discard enqueue callback for USB devices.
 *
 * @param   pUsbIns         The USB device instance.
 * @param   paRanges        Ranges to discard.
 * @param   cRanges         Number of range entries.
 * @param   hIoXfer         I/O handle to return on completion.
 */
typedef DECLCALLBACKTYPE(int, FNPDMBLKCACHEXFERENQUEUEDISCARDUSB,(PPDMUSBINS pUsbIns, PCRTRANGE paRanges, unsigned cRanges,
                                                                  PPDMBLKCACHEIOXFER hIoXfer));
/** Pointer to a FNPDMBLKCACHEXFERENQUEUEDISCARDUSB(). */
typedef FNPDMBLKCACHEXFERENQUEUEDISCARDUSB *PFNPDMBLKCACHEXFERENQUEUEDISCARDUSB;

/**
 * Create a block cache user for a driver instance.
 *
 * @returns VBox status code.
 * @param   pVM                      The cross context VM structure.
 * @param   pDrvIns                  The driver instance.
 * @param   ppBlkCache               Where to store the handle to the block cache.
 * @param   pfnXferComplete          The I/O transfer complete callback.
 * @param   pfnXferEnqueue           The I/O request enqueue callback.
 * @param   pfnXferEnqueueDiscard    The discard request enqueue callback.
 * @param   pcszId                   Unique ID used to identify the user.
 */
VMMR3DECL(int) PDMR3BlkCacheRetainDriver(PVM pVM, PPDMDRVINS pDrvIns, PPPDMBLKCACHE ppBlkCache,
                                         PFNPDMBLKCACHEXFERCOMPLETEDRV pfnXferComplete,
                                         PFNPDMBLKCACHEXFERENQUEUEDRV pfnXferEnqueue,
                                         PFNPDMBLKCACHEXFERENQUEUEDISCARDDRV pfnXferEnqueueDiscard,
                                         const char *pcszId);

/**
 * Create a block cache user for a device instance.
 *
 * @returns VBox status code.
 * @param   pVM                      The cross context VM structure.
 * @param   pDevIns                  The device instance.
 * @param   ppBlkCache               Where to store the handle to the block cache.
 * @param   pfnXferComplete          The I/O transfer complete callback.
 * @param   pfnXferEnqueue           The I/O request enqueue callback.
 * @param   pfnXferEnqueueDiscard    The discard request enqueue callback.
 * @param   pcszId                   Unique ID used to identify the user.
 */
VMMR3DECL(int) PDMR3BlkCacheRetainDevice(PVM pVM, PPDMDEVINS pDevIns, PPPDMBLKCACHE ppBlkCache,
                                         PFNPDMBLKCACHEXFERCOMPLETEDEV pfnXferComplete,
                                         PFNPDMBLKCACHEXFERENQUEUEDEV pfnXferEnqueue,
                                         PFNPDMBLKCACHEXFERENQUEUEDISCARDDEV pfnXferEnqueueDiscard,
                                         const char *pcszId);

/**
 * Create a block cache user for a USB instance.
 *
 * @returns VBox status code.
 * @param   pVM                      The cross context VM structure.
 * @param   pUsbIns                  The USB device instance.
 * @param   ppBlkCache               Where to store the handle to the block cache.
 * @param   pfnXferComplete          The I/O transfer complete callback.
 * @param   pfnXferEnqueue           The I/O request enqueue callback.
 * @param   pfnXferEnqueueDiscard    The discard request enqueue callback.
 * @param   pcszId                   Unique ID used to identify the user.
 */
VMMR3DECL(int) PDMR3BlkCacheRetainUsb(PVM pVM, PPDMUSBINS pUsbIns, PPPDMBLKCACHE ppBlkCache,
                                      PFNPDMBLKCACHEXFERCOMPLETEUSB pfnXferComplete,
                                      PFNPDMBLKCACHEXFERENQUEUEUSB pfnXferEnqueue,
                                      PFNPDMBLKCACHEXFERENQUEUEDISCARDUSB pfnXferEnqueueDiscard,
                                      const char *pcszId);

/**
 * Create a block cache user for internal use by VMM.
 *
 * @returns VBox status code.
 * @param   pVM                      The cross context VM structure.
 * @param   pvUser                   Opaque user data.
 * @param   ppBlkCache               Where to store the handle to the block cache.
 * @param   pfnXferComplete          The I/O transfer complete callback.
 * @param   pfnXferEnqueue           The I/O request enqueue callback.
 * @param   pfnXferEnqueueDiscard    The discard request enqueue callback.
 * @param   pcszId                   Unique ID used to identify the user.
 */
VMMR3DECL(int) PDMR3BlkCacheRetainInt(PVM pVM, void *pvUser, PPPDMBLKCACHE ppBlkCache,
                                      PFNPDMBLKCACHEXFERCOMPLETEINT pfnXferComplete,
                                      PFNPDMBLKCACHEXFERENQUEUEINT pfnXferEnqueue,
                                      PFNPDMBLKCACHEXFERENQUEUEDISCARDINT pfnXferEnqueueDiscard,
                                      const char *pcszId);

/**
 * Releases a block cache handle.
 *
 * @param   pBlkCache       Block cache handle.
 */
VMMR3DECL(void) PDMR3BlkCacheRelease(PPDMBLKCACHE pBlkCache);

/**
 * Releases all block cache handles for a device instance.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pDevIns         The device instance.
 */
VMMR3DECL(void) PDMR3BlkCacheReleaseDevice(PVM pVM, PPDMDEVINS pDevIns);

/**
 * Releases all block cache handles for a driver instance.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pDrvIns         The driver instance.
 */
VMMR3DECL(void) PDMR3BlkCacheReleaseDriver(PVM pVM, PPDMDRVINS pDrvIns);

/**
 * Releases all block cache handles for a USB device instance.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pUsbIns         The USB device instance.
 */
VMMR3DECL(void) PDMR3BlkCacheReleaseUsb(PVM pVM, PPDMUSBINS pUsbIns);

/**
 * Creates a read task on the given endpoint.
 *
 * @returns VBox status code.
 * @param   pBlkCache       The cache instance.
 * @param   off             Where to start reading from.
 * @param   pSgBuf          Scatter gather buffer store the data in.
 * @param   cbRead          The overall number of bytes to read.
 * @param   pvUser          Opaque user data returned in the completion callback
 *                          upon completion of the read.
 */
VMMR3DECL(int) PDMR3BlkCacheRead(PPDMBLKCACHE pBlkCache, uint64_t off, PCRTSGBUF pSgBuf, size_t cbRead, void *pvUser);

/**
 * Creates a write task on the given endpoint.
 *
 * @returns VBox status code.
 * @param   pBlkCache       The cache instance.
 * @param   off             Where to start writing at.
 * @param   pSgBuf          Scatter gather buffer gather the data from.
 * @param   cbWrite         The overall number of bytes to write.
 * @param   pvUser          Opaque user data returned in the completion callback
 *                          upon completion of the task.
 */
VMMR3DECL(int) PDMR3BlkCacheWrite(PPDMBLKCACHE pBlkCache, uint64_t off, PCRTSGBUF pSgBuf, size_t cbWrite, void *pvUser);

/**
 * Creates a flush task on the given endpoint.
 *
 * @returns VBox status code.
 * @param   pBlkCache       The cache instance.
 * @param   pvUser          Opaque user data returned in the completion callback
 *                          upon completion of the task.
 */
VMMR3DECL(int) PDMR3BlkCacheFlush(PPDMBLKCACHE pBlkCache, void *pvUser);

/**
 * Discards the given ranges from the cache.
 *
 * @returns VBox status code.
 * @param   pBlkCache       The cache instance.
 * @param   paRanges        Array of ranges to discard.
 * @param   cRanges         Number of ranges in the array.
 * @param   pvUser          Opaque user data returned in the completion callback
 *                          upon completion of the task.
 */
VMMR3DECL(int) PDMR3BlkCacheDiscard(PPDMBLKCACHE pBlkCache, PCRTRANGE paRanges, unsigned cRanges, void *pvUser);

/**
 * Notify the cache of a complete I/O transfer.
 *
 * @param   pBlkCache       The cache instance.
 * @param   hIoXfer         The I/O transfer handle which completed.
 * @param   rcIoXfer        The status code of the completed request.
 */
VMMR3DECL(void) PDMR3BlkCacheIoXferComplete(PPDMBLKCACHE pBlkCache, PPDMBLKCACHEIOXFER hIoXfer, int rcIoXfer);

/**
 * Suspends the block cache.
 *
 * The cache waits until all I/O transfers completed and stops to enqueue new
 * requests after the call returned but will not accept reads, write or flushes
 * either.
 *
 * @returns VBox status code.
 * @param   pBlkCache       The cache instance.
 */
VMMR3DECL(int) PDMR3BlkCacheSuspend(PPDMBLKCACHE pBlkCache);

/**
 * Resumes operation of the block cache.
 *
 * @returns VBox status code.
 * @param   pBlkCache       The cache instance.
 */
VMMR3DECL(int) PDMR3BlkCacheResume(PPDMBLKCACHE pBlkCache);

/**
 * Clears the block cache and removes all entries.
 *
 * The cache waits until all I/O transfers completed.
 *
 * @returns VBox status code.
 * @param   pBlkCache       The cache instance.
 */
VMMR3DECL(int) PDMR3BlkCacheClear(PPDMBLKCACHE pBlkCache);

/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_pdmblkcache_h */

