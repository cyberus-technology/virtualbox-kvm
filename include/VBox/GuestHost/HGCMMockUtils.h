/* $Id */
/** @file
 * HGCMMockUtils.h: Utility functions for the HGCM Mocking framework.
 *
 * The utility functions are optional to the actual HGCM Mocking framework and
 * can support testcases which require a more advanced setup.
 *
 * With this one can setup host and guest side threads, which in turn can simulate
 * specific host (i.e. HGCM service) + guest (i.e. like in the Guest Addditions
 * via VbglR3) scenarios.
 *
 * Glossary:
 *
 * Host thread:
 *   - The host thread is used as part of the actual HGCM service being tested and
 *     provides callbacks (@see TSTHGCMUTILSHOSTCALLBACKS) for the unit test.
 * Guest thread:
 *    - The guest thread is used as part of the guest side and mimics
 *      VBoxClient / VBoxTray / VBoxService parts. (i.e. for VbglR3 calls).
 * Task:
 *    - A task is the simplest unit of test execution and used between the guest
 *      and host mocking threads.
 *
 ** @todo Add TstHGCMSimpleHost / TstHGCMSimpleGuest wrappers along those lines:
 *              Callback.pfnOnClientConnected = tstOnHostClientConnected()
 *              TstHGCMSimpleHostInitAndStart(&Callback)
 *              Callback.pfnOnConnected = tstOnGuestConnected()
 *              TstHGCMSimpleClientInitAndStart(&Callback)
 */

/*
 * Copyright (C) 2022-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_GuestHost_HGCMMockUtils_h
#define VBOX_INCLUDED_GuestHost_HGCMMockUtils_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/err.h>
#include <iprt/semaphore.h>
#include <iprt/thread.h>
#include <iprt/types.h>


#include <VBox/GuestHost/HGCMMock.h>
#include <VBox/VBoxGuestLib.h>


#if defined(IN_RING3) /* Only R3 parts implemented so far. */

/** Pointer to a HGCM Mock utils context. */
typedef struct TSTHGCMUTILSCTX *PTSTHGCMUTILSCTX;

/**
 * Structure for keeping a HGCM Mock utils host service callback table.
 */
typedef struct TSTHGCMUTILSHOSTCALLBACKS
{
    DECLCALLBACKMEMBER(int, pfnOnClientConnected,(PTSTHGCMUTILSCTX pCtx, PTSTHGCMMOCKCLIENT pClient, void *pvUser));
} TSTHGCMUTILSHOSTCALLBACKS;
/** Pointer to a HGCM Mock utils host callbacks table. */
typedef TSTHGCMUTILSHOSTCALLBACKS *PTSTHGCMUTILSHOSTCALLBACKS;

/**
 * Structure for keeping a generic HGCM Mock utils task.
 *
 * A task is a single test unit / entity.
 */
typedef struct TSTHGCMUTILSTASK
{
    /** Completion event. */
    RTSEMEVENT                    hEvent;
    /** Completion rc.
     *  Set to VERR_IPE_UNINITIALIZED_STATUS if not completed yet. */
    int                           rcCompleted;
    /** Expected completion rc. */
    int                           rcExpected;
    /** Pointer to opaque (testcase-specific) task parameters.
     *  Might be NULL if not needed / used. */
    void                         *pvUser;
} TSTHGCMUTILSTASK;
/** Pointer to a HGCM Mock utils task. */
typedef TSTHGCMUTILSTASK *PTSTHGCMUTILSTASK;

/** Callback function for HGCM Mock utils threads. */
typedef DECLCALLBACKTYPE(int, FNTSTHGCMUTILSTHREAD,(PTSTHGCMUTILSCTX pCtx, void *pvUser));
/** Pointer to a HGCM Mock utils guest thread callback. */
typedef FNTSTHGCMUTILSTHREAD *PFNTSTHGCMUTILSTHREAD;

/**
 * Structure for keeping a HGCM Mock utils context.
 */
typedef struct TSTHGCMUTILSCTX
{
    /** Pointer to the HGCM Mock service instance to use. */
    PTSTHGCMMOCKSVC               pSvc;
    /** Currently we only support one task at a time. */
    TSTHGCMUTILSTASK              Task;
    struct
    {
        RTTHREAD                  hThread;
        volatile bool             fShutdown;
        PFNTSTHGCMUTILSTHREAD     pfnThread;
        void                     *pvUser;
    } Guest;
    struct
    {
        RTTHREAD                  hThread;
        volatile bool             fShutdown;
        TSTHGCMUTILSHOSTCALLBACKS Callbacks;
        void                     *pvUser;
    } Host;
} TSTHGCMUTILSCTX;


/*********************************************************************************************************************************
*  Prototypes.                                                                                                                   *
*********************************************************************************************************************************/
/** @name Context handling.
 * @{ */
void TstHGCMUtilsCtxInit(PTSTHGCMUTILSCTX pCtx, PTSTHGCMMOCKSVC pSvc);
/** @} */

/** @name Task handling.
 * @{ */
PTSTHGCMUTILSTASK TstHGCMUtilsTaskGetCurrent(PTSTHGCMUTILSCTX pCtx);
int TstHGCMUtilsTaskInit(PTSTHGCMUTILSTASK pTask);
void TstHGCMUtilsTaskDestroy(PTSTHGCMUTILSTASK pTask);
int TstHGCMUtilsTaskWait(PTSTHGCMUTILSTASK pTask, RTMSINTERVAL msTimeout);
bool TstHGCMUtilsTaskOk(PTSTHGCMUTILSTASK pTask);
bool TstHGCMUtilsTaskCompleted(PTSTHGCMUTILSTASK pTask);
void TstHGCMUtilsTaskSignal(PTSTHGCMUTILSTASK pTask, int rc);
/** @} */

/** @name Threading.
 * @{ */
int TstHGCMUtilsGuestThreadStart(PTSTHGCMUTILSCTX pCtx, PFNTSTHGCMUTILSTHREAD pFnThread, void *pvUser);
int TstHGCMUtilsGuestThreadStop(PTSTHGCMUTILSCTX pCtx);
int TstHGCMUtilsHostThreadStart(PTSTHGCMUTILSCTX pCtx, PTSTHGCMUTILSHOSTCALLBACKS pCallbacks, void *pvUser);
int TstHGCMUtilsHostThreadStop(PTSTHGCMUTILSCTX pCtx);
/** @} */


/*********************************************************************************************************************************
 * Context                                                                                                                       *
 ********************************************************************************************************************************/
/**
 * Initializes a HGCM Mock utils context.
 *
 * @param   pCtx                Context to intiialize.
 * @param   pSvc                HGCM Mock service instance to use.
 */
void TstHGCMUtilsCtxInit(PTSTHGCMUTILSCTX pCtx, PTSTHGCMMOCKSVC pSvc)
{
    RT_BZERO(pCtx, sizeof(TSTHGCMUTILSCTX));

    pCtx->pSvc = pSvc;
}


/*********************************************************************************************************************************
 * Tasks                                                                                                                         *
 ********************************************************************************************************************************/
/**
 * Returns the current task of a HGCM Mock utils context.
 *
 * @returns Current task of a HGCM Mock utils context. NULL if no current task found.
 * @param   pCtx                HGCM Mock utils context.
 */
PTSTHGCMUTILSTASK TstHGCMUtilsTaskGetCurrent(PTSTHGCMUTILSCTX pCtx)
{
    /* Currently we only support one task at a time. */
    return &pCtx->Task;
}

/**
 * Initializes a HGCM Mock utils task.
 *
 * @returns VBox status code.
 * @param   pTask               Task to initialize.
 */
int TstHGCMUtilsTaskInit(PTSTHGCMUTILSTASK pTask)
{
    pTask->pvUser = NULL;
    pTask->rcCompleted = pTask->rcExpected = VERR_IPE_UNINITIALIZED_STATUS;
    return RTSemEventCreate(&pTask->hEvent);
}

/**
 * Destroys a HGCM Mock utils task.
 *
 * @param   pTask               Task to destroy.
 */
void TstHGCMUtilsTaskDestroy(PTSTHGCMUTILSTASK pTask)
{
    RTSemEventDestroy(pTask->hEvent);
}

/**
 * Waits for a HGCM Mock utils task to complete.
 *
 * @returns VBox status code.
 * @param   pTask               Task to wait for.
 * @param   msTimeout           Timeout (in ms) to wait.
 */
int TstHGCMUtilsTaskWait(PTSTHGCMUTILSTASK pTask, RTMSINTERVAL msTimeout)
{
    return RTSemEventWait(pTask->hEvent, msTimeout);
}

/**
 * Returns if the HGCM Mock utils task has been completed successfully.
 *
 * @returns \c true if successful, \c false if not.
 * @param   pTask               Task to check.
 */
bool TstHGCMUtilsTaskOk(PTSTHGCMUTILSTASK pTask)
{
    return pTask->rcCompleted == pTask->rcExpected;
}

/**
 * Returns if the HGCM Mock utils task has been completed (failed or succeeded).
 *
 * @returns \c true if completed, \c false if (still) running.
 * @param   pTask               Task to check.
 */
bool TstHGCMUtilsTaskCompleted(PTSTHGCMUTILSTASK pTask)
{
    return pTask->rcCompleted != VERR_IPE_UNINITIALIZED_STATUS;
}

/**
 * Signals a HGCM Mock utils task to complete its operation.
 *
 * @param   pTask               Task to complete.
 * @param   rc                  Task result to set for completion.
 */
void TstHGCMUtilsTaskSignal(PTSTHGCMUTILSTASK pTask, int rc)
{
    AssertMsg(pTask->rcCompleted == VERR_IPE_UNINITIALIZED_STATUS, ("Task already completed\n"));
    pTask->rcCompleted = rc;
    int rc2 = RTSemEventSignal(pTask->hEvent);
    AssertRC(rc2);
}


/*********************************************************************************************************************************
 * Threading                                                                                                                     *
 ********************************************************************************************************************************/

/**
 * Thread worker for the guest side thread.
 *
 * @returns VBox status code.
 * @param   hThread             Thread handle.
 * @param   pvUser              Pointer of type PTSTHGCMUTILSCTX.
 *
 * @note    Runs in the guest thread.
 */
static DECLCALLBACK(int) tstHGCMUtilsGuestThread(RTTHREAD hThread, void *pvUser)
{
    RT_NOREF(hThread);
    PTSTHGCMUTILSCTX pCtx = (PTSTHGCMUTILSCTX)pvUser;
    AssertPtr(pCtx);

    RTThreadUserSignal(hThread);

    if (pCtx->Guest.pfnThread)
        return pCtx->Guest.pfnThread(pCtx, pCtx->Guest.pvUser);

    return VINF_SUCCESS;
}

/**
 * Starts the guest side thread.
 *
 * @returns VBox status code.
 * @param   pCtx                HGCM Mock utils context to start guest thread for.
 * @param   pFnThread           Pointer to custom thread worker function to call within the guest side thread.
 * @param   pvUser              User-supplied pointer to guest thread context data. Optional and can be NULL.
 */
int TstHGCMUtilsGuestThreadStart(PTSTHGCMUTILSCTX pCtx, PFNTSTHGCMUTILSTHREAD pFnThread, void *pvUser)
{
    pCtx->Guest.pfnThread = pFnThread;
    pCtx->Guest.pvUser    = pvUser;

    int rc = RTThreadCreate(&pCtx->Guest.hThread, tstHGCMUtilsGuestThread, pCtx, 0, RTTHREADTYPE_DEFAULT, RTTHREADFLAGS_WAITABLE,
                            "tstShClGst");
    if (RT_SUCCESS(rc))
        rc = RTThreadUserWait(pCtx->Guest.hThread, RT_MS_30SEC);

    return rc;
}

/**
 * Stops the guest side thread.
 *
 * @returns VBox status code.
 * @param   pCtx                HGCM Mock utils context to stop guest thread for.
 */
int TstHGCMUtilsGuestThreadStop(PTSTHGCMUTILSCTX pCtx)
{
    ASMAtomicWriteBool(&pCtx->Guest.fShutdown, true);

    int rcThread;
    int rc = RTThreadWait(pCtx->Guest.hThread, RT_MS_30SEC, &rcThread);
    if (RT_SUCCESS(rc))
        rc = rcThread;
    if (RT_SUCCESS(rc))
        pCtx->Guest.hThread = NIL_RTTHREAD;

    return rc;
}

/**
 * Thread worker function for the host side HGCM service.
 *
 * @returns VBox status code.
 * @param   hThread             Thread handle.
 * @param   pvUser              Pointer of type PTSTHGCMUTILSCTX.
 *
 * @note    Runs in the host service thread.
 */
static DECLCALLBACK(int) tstHGCMUtilsHostThreadWorker(RTTHREAD hThread, void *pvUser)
{
    RT_NOREF(hThread);
    PTSTHGCMUTILSCTX pCtx = (PTSTHGCMUTILSCTX)pvUser;
    AssertPtr(pCtx);

    int rc = VINF_SUCCESS;

    RTThreadUserSignal(hThread);

    PTSTHGCMMOCKSVC const pSvc = TstHgcmMockSvcInst();

    for (;;)
    {
        if (ASMAtomicReadBool(&pCtx->Host.fShutdown))
            break;

        /* Wait for a new (mock) HGCM client to connect. */
        PTSTHGCMMOCKCLIENT pMockClient = TstHgcmMockSvcWaitForConnectEx(pSvc, 100 /* ms */);
        if (pMockClient) /* Might be NULL when timed out. */
        {
            if (pCtx->Host.Callbacks.pfnOnClientConnected)
                /* ignore rc */ pCtx->Host.Callbacks.pfnOnClientConnected(pCtx, pMockClient, pCtx->Host.pvUser);
        }
    }

    return rc;
}

/**
 * Starts the host side thread.
 *
 * @returns VBox status code.
 * @param   pCtx                HGCM Mock utils context to start host thread for.
 * @param   pCallbacks          Pointer to host callback table to use.
 * @param   pvUser              User-supplied pointer to reach into the host thread callbacks.
 */
int TstHGCMUtilsHostThreadStart(PTSTHGCMUTILSCTX pCtx, PTSTHGCMUTILSHOSTCALLBACKS pCallbacks, void *pvUser)
{
    memcpy(&pCtx->Host.Callbacks, pCallbacks, sizeof(TSTHGCMUTILSHOSTCALLBACKS));
    pCtx->Host.pvUser = pvUser;

    int rc = RTThreadCreate(&pCtx->Host.hThread, tstHGCMUtilsHostThreadWorker, pCtx, 0, RTTHREADTYPE_DEFAULT, RTTHREADFLAGS_WAITABLE,
                            "tstShClHst");
    if (RT_SUCCESS(rc))
        rc = RTThreadUserWait(pCtx->Host.hThread, RT_MS_30SEC);

    return rc;
}

/**
 * Stops the host side thread.
 *
 * @returns VBox status code.
 * @param   pCtx                HGCM Mock utils context to stop host thread for.
 */
int TstHGCMUtilsHostThreadStop(PTSTHGCMUTILSCTX pCtx)
{
    ASMAtomicWriteBool(&pCtx->Host.fShutdown, true);

    int rcThread;
    int rc = RTThreadWait(pCtx->Host.hThread, RT_MS_30SEC, &rcThread);
    if (RT_SUCCESS(rc))
        rc = rcThread;
    if (RT_SUCCESS(rc))
        pCtx->Host.hThread = NIL_RTTHREAD;

    return rc;
}

#endif /* IN_RING3 */

#endif /* !VBOX_INCLUDED_GuestHost_HGCMMockUtils_h */

