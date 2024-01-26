/** @file
 * PDM - Pluggable Device Manager, Threads.
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

#ifndef VBOX_INCLUDED_vmm_pdmthread_h
#define VBOX_INCLUDED_vmm_pdmthread_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/cdefs.h>
#include <VBox/types.h>
#ifdef IN_RING3
# include <iprt/thread.h>
#endif

RT_C_DECLS_BEGIN

/** @defgroup grp_pdm_thread    The PDM Threads API
 * @ingroup grp_pdm
 * @{
 */

/**
 * The thread state
 */
typedef enum PDMTHREADSTATE
{
    /** The usual invalid 0 entry. */
    PDMTHREADSTATE_INVALID = 0,
    /** The thread is initializing.
     * Prev state: none
     * Next state: suspended, terminating (error) */
    PDMTHREADSTATE_INITIALIZING,
    /** The thread has been asked to suspend.
     * Prev state: running
     * Next state: suspended */
    PDMTHREADSTATE_SUSPENDING,
    /** The thread is supended.
     * Prev state: suspending, initializing
     * Next state: resuming, terminated. */
    PDMTHREADSTATE_SUSPENDED,
    /** The thread is active.
     * Prev state: suspended
     * Next state: running, terminating. */
    PDMTHREADSTATE_RESUMING,
    /** The thread is active.
     * Prev state: resuming
     * Next state: suspending, terminating. */
    PDMTHREADSTATE_RUNNING,
    /** The thread has been asked to terminate.
     * Prev state: initializing, suspended, resuming, running
     * Next state: terminated. */
    PDMTHREADSTATE_TERMINATING,
    /** The thread is terminating / has terminated.
     * Prev state: terminating
     * Next state: none */
    PDMTHREADSTATE_TERMINATED,
    /** The usual 32-bit hack. */
    PDMTHREADSTATE_32BIT_HACK = 0x7fffffff
} PDMTHREADSTATE;

/** A pointer to a PDM thread. */
typedef R3PTRTYPE(struct PDMTHREAD *) PPDMTHREAD;
/** A pointer to a pointer to a PDM thread. */
typedef PPDMTHREAD *PPPDMTHREAD;

/**
 * PDM thread, device variation.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance.
 * @param   pThread     The PDM thread data.
 */
typedef DECLCALLBACKTYPE(int, FNPDMTHREADDEV,(PPDMDEVINS pDevIns, PPDMTHREAD pThread));
/** Pointer to a FNPDMTHREADDEV(). */
typedef FNPDMTHREADDEV *PFNPDMTHREADDEV;

/**
 * PDM thread, USB device variation.
 *
 * @returns VBox status code.
 * @param   pUsbIns     The USB device instance.
 * @param   pThread     The PDM thread data.
 */
typedef DECLCALLBACKTYPE(int, FNPDMTHREADUSB,(PPDMUSBINS pUsbIns, PPDMTHREAD pThread));
/** Pointer to a FNPDMTHREADUSB(). */
typedef FNPDMTHREADUSB *PFNPDMTHREADUSB;

/**
 * PDM thread, driver variation.
 *
 * @returns VBox status code.
 * @param   pDrvIns     The driver instance.
 * @param   pThread     The PDM thread data.
 */
typedef DECLCALLBACKTYPE(int, FNPDMTHREADDRV,(PPDMDRVINS pDrvIns, PPDMTHREAD pThread));
/** Pointer to a FNPDMTHREADDRV(). */
typedef FNPDMTHREADDRV *PFNPDMTHREADDRV;

/**
 * PDM thread, driver variation.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pThread     The PDM thread data.
 */
typedef DECLCALLBACKTYPE(int, FNPDMTHREADINT,(PVM pVM, PPDMTHREAD pThread));
/** Pointer to a FNPDMTHREADINT(). */
typedef FNPDMTHREADINT *PFNPDMTHREADINT;

/**
 * PDM thread, driver variation.
 *
 * @returns VBox status code.
 * @param   pThread     The PDM thread data.
 */
typedef int FNPDMTHREADEXT(PPDMTHREAD pThread);
/** Pointer to a FNPDMTHREADEXT(). */
typedef FNPDMTHREADEXT *PFNPDMTHREADEXT;



/**
 * PDM thread wakeup call, device variation.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance.
 * @param   pThread     The PDM thread data.
 */
typedef DECLCALLBACKTYPE(int, FNPDMTHREADWAKEUPDEV,(PPDMDEVINS pDevIns, PPDMTHREAD pThread));
/** Pointer to a FNPDMTHREADDEV(). */
typedef FNPDMTHREADWAKEUPDEV *PFNPDMTHREADWAKEUPDEV;

/**
 * PDM thread wakeup call, device variation.
 *
 * @returns VBox status code.
 * @param   pUsbIns     The USB device instance.
 * @param   pThread     The PDM thread data.
 */
typedef DECLCALLBACKTYPE(int, FNPDMTHREADWAKEUPUSB,(PPDMUSBINS pUsbIns, PPDMTHREAD pThread));
/** Pointer to a FNPDMTHREADUSB(). */
typedef FNPDMTHREADWAKEUPUSB *PFNPDMTHREADWAKEUPUSB;

/**
 * PDM thread wakeup call, driver variation.
 *
 * @returns VBox status code.
 * @param   pDrvIns     The driver instance.
 * @param   pThread     The PDM thread data.
 */
typedef DECLCALLBACKTYPE(int, FNPDMTHREADWAKEUPDRV,(PPDMDRVINS pDrvIns, PPDMTHREAD pThread));
/** Pointer to a FNPDMTHREADDRV(). */
typedef FNPDMTHREADWAKEUPDRV *PFNPDMTHREADWAKEUPDRV;

/**
 * PDM thread wakeup call, internal variation.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pThread     The PDM thread data.
 */
typedef DECLCALLBACKTYPE(int, FNPDMTHREADWAKEUPINT,(PVM pVM, PPDMTHREAD pThread));
/** Pointer to a FNPDMTHREADWAKEUPINT(). */
typedef FNPDMTHREADWAKEUPINT *PFNPDMTHREADWAKEUPINT;

/**
 * PDM thread wakeup call, external variation.
 *
 * @returns VBox status code.
 * @param   pThread     The PDM thread data.
 */
typedef int FNPDMTHREADWAKEUPEXT(PPDMTHREAD pThread);
/** Pointer to a FNPDMTHREADEXT(). */
typedef FNPDMTHREADWAKEUPEXT *PFNPDMTHREADWAKEUPEXT;


/**
 * PDM Thread instance data.
 */
typedef struct PDMTHREAD
{
    /** PDMTHREAD_VERSION. */
    uint32_t                    u32Version;
    /** The thread state. */
    PDMTHREADSTATE volatile     enmState;
    /** The thread handle. */
    RTTHREAD                    Thread;
    /** The user parameter. */
    R3PTRTYPE(void *)           pvUser;
    /** Data specific to the kind of thread.
     * This should really be in PDMTHREADINT, but is placed here because of the
     * function pointer typedefs. So, don't touch these, please.
     */
    union
    {
        /** PDMTHREADTYPE_DEVICE data. */
        struct
        {
            /** The device instance. */
            PPDMDEVINSR3                        pDevIns;
            /** The thread function. */
            R3PTRTYPE(PFNPDMTHREADDEV)          pfnThread;
            /** Thread. */
            R3PTRTYPE(PFNPDMTHREADWAKEUPDEV)    pfnWakeUp;
        } Dev;

        /** PDMTHREADTYPE_USB data. */
        struct
        {
            /** The device instance. */
            PPDMUSBINS                          pUsbIns;
            /** The thread function. */
            R3PTRTYPE(PFNPDMTHREADUSB)          pfnThread;
            /** Thread. */
            R3PTRTYPE(PFNPDMTHREADWAKEUPUSB)    pfnWakeUp;
        } Usb;

        /** PDMTHREADTYPE_DRIVER data. */
        struct
        {
            /** The driver instance. */
            R3PTRTYPE(PPDMDRVINS)               pDrvIns;
            /** The thread function. */
            R3PTRTYPE(PFNPDMTHREADDRV)          pfnThread;
            /** Thread. */
            R3PTRTYPE(PFNPDMTHREADWAKEUPDRV)    pfnWakeUp;
        } Drv;

        /** PDMTHREADTYPE_INTERNAL data. */
        struct
        {
            /** The thread function. */
            R3PTRTYPE(PFNPDMTHREADINT)          pfnThread;
            /** Thread. */
            R3PTRTYPE(PFNPDMTHREADWAKEUPINT)    pfnWakeUp;
        } Int;

        /** PDMTHREADTYPE_EXTERNAL data. */
        struct
        {
            /** The thread function. */
            R3PTRTYPE(PFNPDMTHREADEXT)          pfnThread;
            /** Thread. */
            R3PTRTYPE(PFNPDMTHREADWAKEUPEXT)    pfnWakeUp;
        } Ext;
    } u;

    /** Internal data. */
    union
    {
#ifdef PDMTHREADINT_DECLARED
        PDMTHREADINT            s;
#endif
        uint8_t                 padding[64];
    } Internal;
} PDMTHREAD;

/** PDMTHREAD::u32Version value. */
#define PDMTHREAD_VERSION                       PDM_VERSION_MAKE(0xefff, 1, 0)

#ifdef IN_RING3
VMMR3DECL(int) PDMR3ThreadCreate(PVM pVM, PPPDMTHREAD ppThread, void *pvUser, PFNPDMTHREADINT pfnThread,
                                 PFNPDMTHREADWAKEUPINT pfnWakeUp, size_t cbStack, RTTHREADTYPE enmType, const char *pszName);
VMMR3DECL(int) PDMR3ThreadCreateExternal(PVM pVM, PPPDMTHREAD ppThread, void *pvUser, PFNPDMTHREADEXT pfnThread,
                                         PFNPDMTHREADWAKEUPEXT pfnWakeUp, size_t cbStack, RTTHREADTYPE enmType, const char *pszName);
VMMR3DECL(int) PDMR3ThreadDestroy(PPDMTHREAD pThread, int *pRcThread);
VMMR3DECL(int) PDMR3ThreadIAmSuspending(PPDMTHREAD pThread);
VMMR3DECL(int) PDMR3ThreadIAmRunning(PPDMTHREAD pThread);
VMMR3DECL(int) PDMR3ThreadSleep(PPDMTHREAD pThread, RTMSINTERVAL cMillies);
VMMR3DECL(int) PDMR3ThreadSuspend(PPDMTHREAD pThread);
VMMR3DECL(int) PDMR3ThreadResume(PPDMTHREAD pThread);
#endif /* IN_RING3 */

/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_pdmthread_h */
