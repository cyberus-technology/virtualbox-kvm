/** @file
 * PDM - Pluggable Device Manager, Queues.
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

#ifndef VBOX_INCLUDED_vmm_pdmqueue_h
#define VBOX_INCLUDED_vmm_pdmqueue_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_pdm_queue     The PDM Queues API
 * @ingroup grp_pdm
 * @{
 */

/** Pointer to a PDM queue. */
typedef struct PDMQUEUE *PPDMQUEUE;

/** Pointer to a PDM queue item core. */
typedef union PDMQUEUEITEMCORE *PPDMQUEUEITEMCORE;

/**
 * PDM queue item core.
 */
typedef union PDMQUEUEITEMCORE
{
    /** The next queue item on the pending list (UINT32_MAX for NIL). */
    uint32_t volatile               iNext;
    /** The next item about to be flushed. */
    R3PTRTYPE(PPDMQUEUEITEMCORE)    pNext;
    /** Make sure the core is 64-bit wide. */
    uint64_t                        u64View;
} PDMQUEUEITEMCORE;


/**
 * Queue consumer callback for devices.
 *
 * @returns Success indicator.
 *          If false the item will not be removed and the flushing will stop.
 * @param   pDevIns     The device instance.
 * @param   pItem       The item to consume. Upon return this item will be freed.
 * @remarks The device critical section will NOT be entered before calling the
 *          callback.  No locks will be held, but for now it's safe to assume
 *          that only one EMT will do queue callbacks at any one time.
 */
typedef DECLCALLBACKTYPE(bool, FNPDMQUEUEDEV,(PPDMDEVINS pDevIns, PPDMQUEUEITEMCORE pItem));
/** Pointer to a FNPDMQUEUEDEV(). */
typedef FNPDMQUEUEDEV *PFNPDMQUEUEDEV;

/**
 * Queue consumer callback for USB devices.
 *
 * @returns Success indicator.
 *          If false the item will not be removed and the flushing will stop.
 * @param   pUsbIns     The USB device instance.
 * @param   pItem       The item to consume. Upon return this item will be freed.
 * @remarks No locks will be held, but for now it's safe to assume that only one
 *          EMT will do queue callbacks at any one time.
 */
typedef DECLCALLBACKTYPE(bool, FNPDMQUEUEUSB,(PPDMUSBINS pUsbIns, PPDMQUEUEITEMCORE pItem));
/** Pointer to a FNPDMQUEUEUSB(). */
typedef FNPDMQUEUEUSB *PFNPDMQUEUEUSB;

/**
 * Queue consumer callback for drivers.
 *
 * @returns Success indicator.
 *          If false the item will not be removed and the flushing will stop.
 * @param   pDrvIns     The driver instance.
 * @param   pItem       The item to consume. Upon return this item will be freed.
 * @remarks No locks will be held, but for now it's safe to assume that only one
 *          EMT will do queue callbacks at any one time.
 */
typedef DECLCALLBACKTYPE(bool, FNPDMQUEUEDRV,(PPDMDRVINS pDrvIns, PPDMQUEUEITEMCORE pItem));
/** Pointer to a FNPDMQUEUEDRV(). */
typedef FNPDMQUEUEDRV *PFNPDMQUEUEDRV;

/**
 * Queue consumer callback for internal component.
 *
 * @returns Success indicator.
 *          If false the item will not be removed and the flushing will stop.
 * @param   pVM         The cross context VM structure.
 * @param   pItem       The item to consume. Upon return this item will be freed.
 * @remarks No locks will be held, but for now it's safe to assume that only one
 *          EMT will do queue callbacks at any one time.
 */
typedef DECLCALLBACKTYPE(bool, FNPDMQUEUEINT,(PVM pVM, PPDMQUEUEITEMCORE pItem));
/** Pointer to a FNPDMQUEUEINT(). */
typedef FNPDMQUEUEINT *PFNPDMQUEUEINT;

/**
 * Queue consumer callback for external component.
 *
 * @returns Success indicator.
 *          If false the item will not be removed and the flushing will stop.
 * @param   pvUser      User argument.
 * @param   pItem       The item to consume. Upon return this item will be freed.
 * @remarks No locks will be held, but for now it's safe to assume that only one
 *          EMT will do queue callbacks at any one time.
 */
typedef DECLCALLBACKTYPE(bool, FNPDMQUEUEEXT,(void *pvUser, PPDMQUEUEITEMCORE pItem));
/** Pointer to a FNPDMQUEUEEXT(). */
typedef FNPDMQUEUEEXT *PFNPDMQUEUEEXT;

#ifdef VBOX_IN_VMM
VMMR3_INT_DECL(int)  PDMR3QueueCreateDevice(PVM pVM, PPDMDEVINS pDevIns, size_t cbItem, uint32_t cItems,
                                            uint32_t cMilliesInterval, PFNPDMQUEUEDEV pfnCallback,
                                            bool fRZEnabled, const char *pszName, PDMQUEUEHANDLE *phQueue);
VMMR3_INT_DECL(int)  PDMR3QueueCreateDriver(PVM pVM, PPDMDRVINS pDrvIns, size_t cbItem, uint32_t cItems,
                                            uint32_t cMilliesInterval, PFNPDMQUEUEDRV pfnCallback,
                                            const char *pszName, PDMQUEUEHANDLE *phQueue);
VMMR3_INT_DECL(int)  PDMR3QueueCreateInternal(PVM pVM, size_t cbItem, uint32_t cItems,
                                              uint32_t cMilliesInterval, PFNPDMQUEUEINT pfnCallback,
                                              bool fRZEnabled, const char *pszName, PDMQUEUEHANDLE *phQueue);
VMMR3DECL(int)       PDMR3QueueCreateExternal(PVM pVM, size_t cbItem, uint32_t cItems, uint32_t cMilliesInterval,
                                              PFNPDMQUEUEEXT pfnCallback, void *pvUser, const char *pszName, PDMQUEUEHANDLE *phQueue);
VMMR3DECL(int)       PDMR3QueueDestroy(PVM pVM, PDMQUEUEHANDLE hQueue, void *pvOwner);
VMMR3_INT_DECL(int)  PDMR3QueueDestroyDevice(PVM pVM, PPDMDEVINS pDevIns);
VMMR3_INT_DECL(int)  PDMR3QueueDestroyDriver(PVM pVM, PPDMDRVINS pDrvIns);
VMMR3DECL(void)      PDMR3QueueFlushAll(PVM pVM);
#endif /* VBOX_IN_VMM */

VMMDECL(PPDMQUEUEITEMCORE)  PDMQueueAlloc(PVMCC pVM, PDMQUEUEHANDLE hQueue, void *pvOwner);
VMMDECL(int)                PDMQueueInsert(PVMCC pVM, PDMQUEUEHANDLE hQueue, void *pvOwner, PPDMQUEUEITEMCORE pInsert);
VMMDECL(int)                PDMQueueFlushIfNecessary(PVMCC pVM, PDMQUEUEHANDLE hQueue, void *pvOwner);

/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_pdmqueue_h */

