/** @file
 * Internal VD filter backend interface.
 */

/*
 * Copyright (C) 2014-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_vd_filter_backend_h
#define VBOX_INCLUDED_vd_filter_backend_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/vd.h>
#include <VBox/vd-common.h>
#include <VBox/vd-ifs-internal.h>

/**
 * VD filter backend interface.
 */
typedef struct VDFILTERBACKEND
{
    /** Structure version. VD_FLTBACKEND_VERSION defines the current version. */
    uint32_t            u32Version;
    /** The name of the backend (constant string). */
    const char          *pszBackendName;

    /**
     * Pointer to an array of structs describing each supported config key.
     * Terminated by a NULL config key. Note that some backends do not support
     * the configuration interface, so this pointer may just contain NULL.
     * Mandatory if the backend sets VD_CAP_CONFIG.
     */
    PCVDCONFIGINFO      paConfigInfo;

    /**
     * Creates a new filter instance.
     *
     * @returns VBox status code.
     * @param   pVDIfsDisk      Pointer to the per-disk VD interface list.
     * @param   fFlags          Subset of VD_FILTER_FLAGS_*.
     * @param   pVDIfsFilter    Pointer to the per-filter VD interface list.
     * @param   ppvBackendData  Opaque state data for this filter instance.
     */
    DECLR3CALLBACKMEMBER(int, pfnCreate, (PVDINTERFACE pVDIfsDisk, uint32_t fFlags,
                                          PVDINTERFACE pVDIfsFilter,
                                          void **ppvBackendData));

    /**
     * Destroys a filter instance.
     *
     * @returns VBox status code.
     * @param   pvBackendData   Opaque state data for the filter instance to destroy.
     */
    DECLR3CALLBACKMEMBER(int, pfnDestroy, (void *pvBackendData));

    /**
     * Filters the data of a read from the image chain. The filter is applied
     * after everything was read.
     *
     * @returns VBox status code.
     * @param   pvBackendData   Opaque state data for the filter instance.
     * @param   uOffset         Start offset of the read.
     * @param   cbRead          Number of bytes read.
     * @param   pIoCtx          The I/O context holding the read data.
     */
    DECLR3CALLBACKMEMBER(int, pfnFilterRead, (void *pvBackendData, uint64_t uOffset, size_t cbRead,
                                              PVDIOCTX pIoCtx));

    /**
     * Filters the data of a write to the image chain. The filter is applied
     * before everything is written.
     *
     * @returns VBox status code.
     * @param   pvBackendData   Opaque state data for the filter instance.
     * @param   uOffset         Start offset of the write.
     * @param   cbWrite         Number of bytes to be written.
     * @param   pIoCtx          The I/O context holding the data to write.
     */
    DECLR3CALLBACKMEMBER(int, pfnFilterWrite, (void *pvBackendData, uint64_t uOffset, size_t cbWrite,
                                               PVDIOCTX pIoCtx));

    /** Initialization safty marker. */
    uint32_t            u32VersionEnd;
} VDFILTERBACKEND;
/** Pointer to VD filter backend. */
typedef VDFILTERBACKEND *PVDFILTERBACKEND;
/** Constant pointer to a VD filter backend. */
typedef const VDFILTERBACKEND *PCVDFILTERBACKEND;

/** The current version of the VDFILTERBACKEND structure. */
#define VD_FLTBACKEND_VERSION                   VD_VERSION_MAKE(0xff02, 1, 0)

#endif /* !VBOX_INCLUDED_vd_filter_backend_h */
