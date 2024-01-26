/** @file
 * PDM - Pluggable Device Manager, Common Definitions & Types.
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

#ifndef VBOX_INCLUDED_vmm_pdmcommon_h
#define VBOX_INCLUDED_vmm_pdmcommon_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>


/** @defgroup grp_pdm_common    Common Definitions & Types
 * @ingroup grp_pdm
 *
 * Not all the types here are "common", they are here to work around header
 * ordering issues.
 *
 * @{
 */

/** Makes a PDM structure version out of an unique magic value and major &
 * minor version numbers.
 *
 * @returns 32-bit structure version number.
 *
 * @param   uMagic      16-bit magic value.  This must be unique.
 * @param   uMajor      12-bit major version number.  Structures with different
 *                      major numbers are not compatible.
 * @param   uMinor      4-bit minor version number.  When only the minor version
 *                      differs, the structures will be 100% backwards
 *                      compatible.
 */
#define PDM_VERSION_MAKE(uMagic, uMajor, uMinor) \
    ( ((uint32_t)(uMagic) << 16) | ((uint32_t)((uMajor) & 0xff) << 4) | ((uint32_t)((uMinor) & 0xf) << 0) )

/**
 * Version of PDM_VERSION_MAKE that's compatible with the preprocessor.
 *
 * @returns 32-bit structure version number.
 *
 * @param   uMagic      16-bit magic value, no suffix.  This must be unique.
 * @param   uMajor      12-bit major version number, no suffix.  Structures with
 *                      different major numbers are not compatible.
 * @param   uMinor      4-bit minor version number, no suffix.  When only the
 *                      minor version differs, the structures will be 100%
 *                      backwards compatible.
 */
#define PDM_VERSION_MAKE_PP(uMagic, uMajor, uMinor) \
    ( (UINT32_C(uMagic) << 16) | ((UINT32_C(uMajor) & UINT32_C(0xff)) << 4) | ((UINT32_C(uMinor) & UINT32_C(0xf)) << 0) )

/** Checks if @a uVerMagic1 is compatible with @a uVerMagic2.
 *
 * @returns true / false.
 * @param   uVerMagic1  Typically the runtime version of the struct.  This must
 *                      have the same magic and major version as @a uVerMagic2
 *                      and the minor version must be greater or equal to that
 *                      of @a uVerMagic2.
 * @param   uVerMagic2  Typically the version the code was compiled against.
 *
 * @remarks The parameters will be referenced more than once.
 */
#define PDM_VERSION_ARE_COMPATIBLE(uVerMagic1, uVerMagic2) \
    (    (uVerMagic1) == (uVerMagic2) \
      || (   (uVerMagic1) >= (uVerMagic2) \
          && ((uVerMagic1) & UINT32_C(0xfffffff0)) == ((uVerMagic2) & UINT32_C(0xfffffff0)) ) \
    )


/** @name PDM Attach/Detach Callback Flags.
 * Used by PDMDeviceAttach, PDMDeviceDetach, PDMDriverAttach, PDMDriverDetach,
 * FNPDMDEVATTACH, FNPDMDEVDETACH, FNPDMDRVATTACH, FNPDMDRVDETACH and
 * FNPDMDRVCONSTRUCT.
 * @{ */
/** The attach/detach command is not a hotplug event. */
#define PDM_TACH_FLAGS_NOT_HOT_PLUG     RT_BIT_32(0)
/** Indicates that no attach or detach callbacks should be made.
 * This is mostly for internal use.  */
#define PDM_TACH_FLAGS_NO_CALLBACKS     RT_BIT_32(1)
/** @} */


/**
 * Is asynchronous handling of suspend or power off notification completed?
 *
 * This is called to check whether the USB device has quiesced.  Don't deadlock.
 * Avoid blocking.  Do NOT wait for anything.
 *
 * @returns true if done, false if more work to be done.
 *
 * @param   pUsbIns             The USB device instance.
 *
 * @thread  EMT(0)
 */
typedef DECLCALLBACKTYPE(bool, FNPDMUSBASYNCNOTIFY,(PPDMUSBINS pUsbIns));
/** Pointer to a FNPDMUSBASYNCNOTIFY. */
typedef FNPDMUSBASYNCNOTIFY *PFNPDMUSBASYNCNOTIFY;

/**
 * Is asynchronous handling of suspend or power off notification completed?
 *
 * This is called to check whether the device has quiesced.  Don't deadlock.
 * Avoid blocking.  Do NOT wait for anything.
 *
 * @returns true if done, false if more work to be done.
 *
 * @param   pDevIns             The device instance.
 * @remarks The caller will enter the device critical section.
 * @thread  EMT(0)
 */
typedef DECLCALLBACKTYPE(bool, FNPDMDEVASYNCNOTIFY,(PPDMDEVINS pDevIns));
/** Pointer to a FNPDMDEVASYNCNOTIFY. */
typedef FNPDMDEVASYNCNOTIFY *PFNPDMDEVASYNCNOTIFY;

/**
 * Is asynchronous handling of suspend or power off notification completed?
 *
 * This is called to check whether the driver has quiesced.  Don't deadlock.
 * Avoid blocking.  Do NOT wait for anything.
 *
 * @returns true if done, false if more work to be done.
 *
 * @param   pDrvIns             The driver instance.
 *
 * @thread  EMT(0)
 */
typedef DECLCALLBACKTYPE(bool, FNPDMDRVASYNCNOTIFY,(PPDMDRVINS pDrvIns));
/** Pointer to a FNPDMDRVASYNCNOTIFY. */
typedef FNPDMDRVASYNCNOTIFY *PFNPDMDRVASYNCNOTIFY;


/**
 * The ring-0 driver request handler.
 *
 * @returns VBox status code. PDMDevHlpCallR0 will return this.
 * @param   pDevIns     The device instance (the ring-0 mapping).
 * @param   uOperation  The operation.
 * @param   u64Arg      Optional integer argument for the operation.
 */
typedef DECLCALLBACKTYPE(int, FNPDMDEVREQHANDLERR0,(PPDMDEVINS pDevIns, uint32_t uOperation, uint64_t u64Arg));
/** Ring-0 pointer to a FNPDMDEVREQHANDLERR0. */
typedef R0PTRTYPE(FNPDMDEVREQHANDLERR0 *) PFNPDMDEVREQHANDLERR0;

/**
 * The ring-0 driver request handler.
 *
 * @returns VBox status code. PDMDrvHlpCallR0 will return this.
 * @param   pDrvIns     The driver instance (the ring-0 mapping).
 * @param   uOperation  The operation.
 * @param   u64Arg      Optional integer argument for the operation.
 */
typedef DECLCALLBACKTYPE(int, FNPDMDRVREQHANDLERR0,(PPDMDRVINS pDrvIns, uint32_t uOperation, uint64_t u64Arg));
/** Ring-0 pointer to a FNPDMDRVREQHANDLERR0. */
typedef R0PTRTYPE(FNPDMDRVREQHANDLERR0 *) PFNPDMDRVREQHANDLERR0;


/** @} */

#endif /* !VBOX_INCLUDED_vmm_pdmcommon_h */

