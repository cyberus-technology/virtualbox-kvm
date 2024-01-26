/* $Id: HGCMMock.h $ */
/** @file
 * HGCMMock.h: Mocking framework for testing HGCM-based host services +
 *             Vbgl code on the host side.
 *
 *             Goal is to run host service + Vbgl code as unmodified as
 *             possible as part of testcases to gain test coverage which
 *             otherwise wouldn't possible for heavily user-centric features
 *             like Shared Clipboard or drag'n drop (DnD).
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

#ifndef VBOX_INCLUDED_GuestHost_HGCMMock_h
#define VBOX_INCLUDED_GuestHost_HGCMMock_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/list.h>
#include <iprt/mem.h>
#include <iprt/rand.h>
#include <iprt/semaphore.h>
#include <iprt/test.h>
#include <iprt/time.h>
#include <iprt/thread.h>
#include <iprt/utf16.h>

#include <VBox/err.h>
#include <VBox/VBoxGuestLib.h>
#include <VBox/hgcmsvc.h>


/*********************************************************************************************************************************
*  Definitions.                                                                                                                  *
*********************************************************************************************************************************/

#if defined(IN_RING3) /* Only R3 parts implemented so far. */

RT_C_DECLS_BEGIN

DECLCALLBACK(DECLEXPORT(int)) VBoxHGCMSvcLoad(VBOXHGCMSVCFNTABLE *ptable);

RT_C_DECLS_END

# define VBGLR3DECL(type) DECL_HIDDEN_NOTHROW(type) VBOXCALL

/** Simple call handle structure for the guest call completion callback. */
typedef struct VBOXHGCMCALLHANDLE_TYPEDEF
{
    /** Where to store the result code on call completion. */
    int32_t rc;
} VBOXHGCMCALLHANDLE_TYPEDEF;

/**
 * Enumeration for a HGCM mock function type.
 */
typedef enum TSTHGCMMOCKFNTYPE
{
    TSTHGCMMOCKFNTYPE_NONE = 0,
    TSTHGCMMOCKFNTYPE_CONNECT,
    TSTHGCMMOCKFNTYPE_DISCONNECT,
    TSTHGCMMOCKFNTYPE_CALL,
    TSTHGCMMOCKFNTYPE_HOST_CALL
} TSTHGCMMOCKFNTYPE;

/** Pointer to a mock HGCM service. */
typedef struct TSTHGCMMOCKSVC *PTSTHGCMMOCKSVC;

/**
 * Structure for mocking a server-side HGCM client.
 */
typedef struct TSTHGCMMOCKCLIENT
{
    /** Pointer to to mock service instance this client belongs to. */
    PTSTHGCMMOCKSVC            pSvc;
    /** Assigned HGCM client ID. */
    uint32_t                   idClient;
    /** Opaque pointer to service-specific client data.
     *  Can be NULL if not being used. */
    void                      *pvClient;
    /** Size (in bytes) of \a pvClient. */
    size_t                     cbClient;
    /** The client's current HGCM call handle. */
    VBOXHGCMCALLHANDLE_TYPEDEF hCall;
    /** Whether the current client call has an asynchronous
     *  call pending or not. */
    bool                       fAsyncExec;
    /** Event semaphore to signal call completion. */
    RTSEMEVENT                 hEvent;
} TSTHGCMMOCKCLIENT;
/** Pointer to a mock HGCM client. */
typedef TSTHGCMMOCKCLIENT *PTSTHGCMMOCKCLIENT;

/**
 * Structure for keeping HGCM mock function parameters.
 */
typedef struct TSTHGCMMOCKFN
{
    /** List node for storing this struct into a queue. */
    RTLISTNODE         Node;
    /** Function type. */
    TSTHGCMMOCKFNTYPE  enmType;
    /** Pointer to associated client. */
    PTSTHGCMMOCKCLIENT pClient;
    /** Union keeping function-specific parameters,
     *  depending on \a enmType. */
    union
    {
        struct
        {
            int32_t             iFunc;
            uint32_t            cParms;
            PVBOXHGCMSVCPARM    pParms;
            VBOXHGCMCALLHANDLE  hCall;
        } Call;
        struct
        {
            int32_t             iFunc;
            uint32_t            cParms;
            PVBOXHGCMSVCPARM    pParms;
        } HostCall;
    } u;
} TSTHGCMMOCKFN;
/** Pointer to a HGCM mock function parameters structure. */
typedef TSTHGCMMOCKFN *PTSTHGCMMOCKFN;

/**
 * Structure for keeping a HGCM mock service instance.
 */
typedef struct TSTHGCMMOCKSVC
{
    /** HGCM helper table to use. */
    VBOXHGCMSVCHELPERS fnHelpers;
    /** HGCM service function table to use. */
    VBOXHGCMSVCFNTABLE fnTable;
    /** Next HGCM client ID to assign.
     *  0 is considered as being invalid. */
    HGCMCLIENTID       uNextClientId;
    /** Size (in bytes) of opaque pvClient area to reserve
     *  for a connected client. */
    size_t             cbClient;
    /** Array of connected HGCM mock clients.
     *  Currently limited to 4 clients maximum. */
    TSTHGCMMOCKCLIENT  aHgcmClient[4];
    /** Thread handle for the service's main loop. */
    RTTHREAD           hThread;
    /** Event semaphore for signalling a message
     *  queue change. */
    RTSEMEVENT         hEventQueue;
    /** Event semaphore for clients connecting to the server. */
    RTSEMEVENT         hEventConnect;
    /** Number of current host calls being served.
     *  Currently limited to one call at a time. */
    uint8_t            cHostCallers;
    /** Result code of last returned host call. */
    int                rcHostCall;
    /** Event semaphore for host calls. */
    RTSEMEVENT         hEventHostCall;
    /** List (queue) of function calls to process. */
    RTLISTANCHOR       lstCall;
    /** Shutdown indicator flag. */
    volatile bool      fShutdown;
} TSTHGCMMOCKSVC;

/** Static HGCM service to mock. */
static TSTHGCMMOCKSVC s_tstHgcmSvc;

/*********************************************************************************************************************************
*  Prototypes.                                                                                                                   *
*********************************************************************************************************************************/
PTSTHGCMMOCKSVC    TstHgcmMockSvcInst(void);
PTSTHGCMMOCKCLIENT TstHgcmMockSvcWaitForConnectEx(PTSTHGCMMOCKSVC pSvc, RTMSINTERVAL msTimeout);
PTSTHGCMMOCKCLIENT TstHgcmMockSvcWaitForConnect(PTSTHGCMMOCKSVC pSvc);
int                TstHgcmMockSvcCreate(PTSTHGCMMOCKSVC pSvc, size_t cbClient);
int                TstHgcmMockSvcDestroy(PTSTHGCMMOCKSVC pSvc);
int                TstHgcmMockSvcStart(PTSTHGCMMOCKSVC pSvc);
int                TstHgcmMockSvcStop(PTSTHGCMMOCKSVC pSvc);

int                TstHgcmMockSvcHostCall(PTSTHGCMMOCKSVC pSvc, void *pvService, int32_t function, uint32_t cParms, VBOXHGCMSVCPARM paParms[]);

VBGLR3DECL(int)    VbglR3HGCMConnect(const char *pszServiceName, HGCMCLIENTID *pidClient);
VBGLR3DECL(int)    VbglR3HGCMDisconnect(HGCMCLIENTID idClient);
VBGLR3DECL(int)    VbglR3HGCMCall(PVBGLIOCHGCMCALL pInfo, size_t cbInfo);



/*********************************************************************************************************************************
*  Internal functions                                                                                                           *
*********************************************************************************************************************************/

/**
 * Initializes a HGCM mock client.
 *
 * @return VBox status code.
 * @param  pClient              Client instance to initialize.
 * @param  idClient             HGCM client ID to assign.
 * @param  cbClient             Size (in bytes) of service-specific (opaque) client data to allocate.
 */
static int tstHgcmMockClientInit(PTSTHGCMMOCKCLIENT pClient, uint32_t idClient, size_t cbClient)
{
    RT_BZERO(pClient, sizeof(TSTHGCMMOCKCLIENT));

    pClient->idClient = idClient;
    if (cbClient)
    {
        pClient->pvClient = RTMemAllocZ(cbClient);
        AssertPtrReturn(pClient->pvClient, VERR_NO_MEMORY);
        pClient->cbClient = cbClient;
    }

    return RTSemEventCreate(&pClient->hEvent);
}

/**
 * Destroys a HGCM mock client.
 *
 * @return VBox status code.
 * @param  pClient              Client instance to destroy.
 */
static int tstHgcmMockClientDestroy(PTSTHGCMMOCKCLIENT pClient)
{
    int rc = RTSemEventDestroy(pClient->hEvent);
    if (RT_SUCCESS(rc))
    {
        if (pClient->pvClient)
        {
            Assert(pClient->cbClient);
            RTMemFree(pClient->pvClient);
            pClient->pvClient = NULL;
            pClient->cbClient = 0;
        }

        pClient->hEvent = NIL_RTSEMEVENT;
    }

    return rc;
}

/* @copydoc VBOXHGCMSVCFNTABLE::pfnConnect */
static DECLCALLBACK(int) tstHgcmMockSvcConnect(PTSTHGCMMOCKSVC pSvc, void *pvService, uint32_t *pidClient)
{
    RT_NOREF(pvService);

    PTSTHGCMMOCKFN pFn = (PTSTHGCMMOCKFN)RTMemAllocZ(sizeof(TSTHGCMMOCKFN));
    AssertPtrReturn(pFn, VERR_NO_MEMORY);

    PTSTHGCMMOCKCLIENT pClient = &pSvc->aHgcmClient[pSvc->uNextClientId];

    int rc = tstHgcmMockClientInit(pClient, pSvc->uNextClientId, pSvc->cbClient);
    if (RT_FAILURE(rc))
        return rc;

    pFn->enmType = TSTHGCMMOCKFNTYPE_CONNECT;
    pFn->pClient = pClient;

    RTListAppend(&pSvc->lstCall, &pFn->Node);
    pFn = NULL; /* Thread takes ownership now. */

    int rc2 = RTSemEventSignal(pSvc->hEventQueue);
    AssertRCReturn(rc2, rc2);
    rc2 = RTSemEventWait(pClient->hEvent, RT_MS_30SEC);
    AssertRCReturn(rc2, rc2);

    ASMAtomicIncU32(&pSvc->uNextClientId);

    rc2 = RTSemEventSignal(pSvc->hEventConnect);
    AssertRCReturn(rc2, rc2);

    *pidClient = pClient->idClient;

    return VINF_SUCCESS;
}

/* @copydoc VBOXHGCMSVCFNTABLE::pfnDisconnect */
static DECLCALLBACK(int) tstHgcmMockSvcDisconnect(PTSTHGCMMOCKSVC pSvc, void *pvService, uint32_t idClient)
{
    RT_NOREF(pvService);

    PTSTHGCMMOCKCLIENT pClient = &pSvc->aHgcmClient[idClient];

    PTSTHGCMMOCKFN pFn = (PTSTHGCMMOCKFN)RTMemAllocZ(sizeof(TSTHGCMMOCKFN));
    AssertPtrReturn(pFn, VERR_NO_MEMORY);

    pFn->enmType = TSTHGCMMOCKFNTYPE_DISCONNECT;
    pFn->pClient = pClient;

    RTListAppend(&pSvc->lstCall, &pFn->Node);
    pFn = NULL; /* Thread takes ownership now. */

    int rc2 = RTSemEventSignal(pSvc->hEventQueue);
    AssertRCReturn(rc2, rc2);

    rc2 = RTSemEventWait(pClient->hEvent, RT_MS_30SEC);
    AssertRCReturn(rc2, rc2);

    return tstHgcmMockClientDestroy(pClient);
}

/* @copydoc VBOXHGCMSVCFNTABLE::pfnCall */
static DECLCALLBACK(int) tstHgcmMockSvcCall(PTSTHGCMMOCKSVC pSvc, void *pvService, VBOXHGCMCALLHANDLE callHandle, uint32_t idClient, void *pvClient,
                                            int32_t function, uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    RT_NOREF(pvService, pvClient);

    PTSTHGCMMOCKCLIENT pClient = &pSvc->aHgcmClient[idClient];

    PTSTHGCMMOCKFN pFn = (PTSTHGCMMOCKFN)RTMemAllocZ(sizeof(TSTHGCMMOCKFN));
    AssertPtrReturn(pFn, VERR_NO_MEMORY);

    const size_t cbParms = cParms * sizeof(VBOXHGCMSVCPARM);

    pFn->enmType         = TSTHGCMMOCKFNTYPE_CALL;
    pFn->pClient         = pClient;

    pFn->u.Call.hCall    = callHandle;
    pFn->u.Call.iFunc    = function;
    pFn->u.Call.pParms   = (PVBOXHGCMSVCPARM)RTMemDup(paParms, cbParms);
    AssertPtrReturn(pFn->u.Call.pParms, VERR_NO_MEMORY);
    pFn->u.Call.cParms   = cParms;

    RTListAppend(&pSvc->lstCall, &pFn->Node);

    int rc2 = RTSemEventSignal(pSvc->hEventQueue);
    AssertRCReturn(rc2, rc2);

    rc2 = RTSemEventWait(pClient->hEvent, RT_INDEFINITE_WAIT);
    AssertRCReturn(rc2, rc2);

    memcpy(paParms, pFn->u.Call.pParms, cbParms);

    return VINF_SUCCESS; /** @todo Return host call rc */
}

/* @copydoc VBOXHGCMSVCFNTABLE::pfnHostCall */
/** Note: Public for also being able to test host calls via testcases. */
int TstHgcmMockSvcHostCall(PTSTHGCMMOCKSVC pSvc, void *pvService, int32_t function, uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    RT_NOREF(pvService);
    AssertReturn(pSvc->cHostCallers == 0, VERR_WRONG_ORDER); /* Only one host call at a time. */

    pSvc->cHostCallers++;

    PTSTHGCMMOCKFN pFn = (PTSTHGCMMOCKFN)RTMemAllocZ(sizeof(TSTHGCMMOCKFN));
    AssertPtrReturn(pFn, VERR_INVALID_POINTER);

    pFn->enmType           = TSTHGCMMOCKFNTYPE_HOST_CALL;
    pFn->u.HostCall.iFunc  = function;
    if (cParms)
    {
        pFn->u.HostCall.pParms = (PVBOXHGCMSVCPARM)RTMemDup(paParms, cParms * sizeof(VBOXHGCMSVCPARM));
        AssertPtrReturn(pFn->u.HostCall.pParms, VERR_NO_MEMORY);
        pFn->u.HostCall.cParms = cParms;
    }

    RTListAppend(&pSvc->lstCall, &pFn->Node);
    pFn = NULL; /* Thread takes ownership now. */

    int rc2 = RTSemEventSignal(pSvc->hEventQueue);
    AssertRC(rc2);

    rc2 = RTSemEventWait(pSvc->hEventHostCall, RT_INDEFINITE_WAIT);
    AssertRCReturn(rc2, rc2);

    Assert(pSvc->cHostCallers);
    pSvc->cHostCallers--;

    return pSvc->rcHostCall;
}

/**
 * Call completion callback for guest calls.
 *
 * @return VBox status code.
 * @param  callHandle           Call handle to complete.
 * @param  rc                   Return code to return to the caller.
 */
static DECLCALLBACK(int) tstHgcmMockSvcCallComplete(VBOXHGCMCALLHANDLE callHandle, int32_t rc)
{
    PTSTHGCMMOCKSVC pSvc = TstHgcmMockSvcInst();

    size_t i = 0;
    for (; RT_ELEMENTS(pSvc->aHgcmClient); i++)
    {
        PTSTHGCMMOCKCLIENT pClient = &pSvc->aHgcmClient[i];
        if (&pClient->hCall == callHandle) /* Slow, but works for now. */
        {
            if (rc == VINF_HGCM_ASYNC_EXECUTE)
            {
                Assert(pClient->fAsyncExec == false);
            }
            else /* Complete call + notify client. */
            {
                callHandle->rc = rc;

                int rc2 = RTSemEventSignal(pClient->hEvent);
                AssertRCReturn(rc2, rc2);
            }

            return VINF_SUCCESS;
        }
    }

    return VERR_NOT_FOUND;
}

/**
 * Main thread of HGCM mock service.
 *
 * @return VBox status code.
 * @param  hThread              Thread handle.
 * @param  pvUser               User-supplied data of type PTSTHGCMMOCKSVC.
 */
static DECLCALLBACK(int) tstHgcmMockSvcThread(RTTHREAD hThread, void *pvUser)
{
    RT_NOREF(hThread);
    PTSTHGCMMOCKSVC pSvc = (PTSTHGCMMOCKSVC)pvUser;

    pSvc->uNextClientId      = 0;

    pSvc->fnTable.cbSize     = sizeof(pSvc->fnTable);
    pSvc->fnTable.u32Version = VBOX_HGCM_SVC_VERSION;

    RT_ZERO(pSvc->fnHelpers);
    pSvc->fnHelpers.pfnCallComplete = tstHgcmMockSvcCallComplete;
    pSvc->fnTable.pHelpers          = &pSvc->fnHelpers;

    int rc = VBoxHGCMSvcLoad(&pSvc->fnTable);
    if (RT_SUCCESS(rc))
    {
        RTThreadUserSignal(hThread);

        for (;;)
        {
            rc = RTSemEventWait(pSvc->hEventQueue, 10 /* ms */);
            if (ASMAtomicReadBool(&pSvc->fShutdown))
            {
                rc = VINF_SUCCESS;
                break;
            }
            if (rc == VERR_TIMEOUT)
                continue;

            PTSTHGCMMOCKFN pFn = RTListGetFirst(&pSvc->lstCall, TSTHGCMMOCKFN, Node);
            if (pFn)
            {
                switch (pFn->enmType)
                {
                    case TSTHGCMMOCKFNTYPE_CONNECT:
                    {
                        rc = pSvc->fnTable.pfnConnect(pSvc->fnTable.pvService,
                                                      pFn->pClient->idClient, pFn->pClient->pvClient,
                                                      VMMDEV_REQUESTOR_USR_NOT_GIVEN /* fRequestor */, false /* fRestoring */);

                        int rc2 = RTSemEventSignal(pFn->pClient->hEvent);
                        AssertRC(rc2);
                        break;
                    }

                    case TSTHGCMMOCKFNTYPE_DISCONNECT:
                    {
                        rc = pSvc->fnTable.pfnDisconnect(pSvc->fnTable.pvService,
                                                         pFn->pClient->idClient, pFn->pClient->pvClient);

                        int rc2 = RTSemEventSignal(pFn->pClient->hEvent);
                        AssertRC(rc2);
                        break;
                    }

                    case TSTHGCMMOCKFNTYPE_CALL:
                    {
                        pSvc->fnTable.pfnCall(NULL, pFn->u.Call.hCall, pFn->pClient->idClient, pFn->pClient->pvClient,
                                              pFn->u.Call.iFunc, pFn->u.Call.cParms, pFn->u.Call.pParms, RTTimeMilliTS());

                        /* Note: Call will be completed in the call completion callback. */
                        break;
                    }

                    case TSTHGCMMOCKFNTYPE_HOST_CALL:
                    {
                        pSvc->rcHostCall = pSvc->fnTable.pfnHostCall(NULL, pFn->u.HostCall.iFunc, pFn->u.HostCall.cParms, pFn->u.HostCall.pParms);

                        int rc2 = RTSemEventSignal(pSvc->hEventHostCall);
                        AssertRC(rc2);
                        break;
                    }

                    default:
                        AssertFailed();
                        break;
                }
                RTListNodeRemove(&pFn->Node);
                RTMemFree(pFn);
            }
        }
    }

    return rc;
}


/*********************************************************************************************************************************
*  Public functions                                                                                                           *
*********************************************************************************************************************************/

/**
 * Returns the pointer to the HGCM mock service instance.
 *
 * @return Pointer to HGCM mock service instance.
 */
PTSTHGCMMOCKSVC TstHgcmMockSvcInst(void)
{
    return &s_tstHgcmSvc;
}

/**
 * Waits for a HGCM mock client to connect, extended version.
 *
 * @return Pointer to connected client, or NULL if ran into timeout.
 * @param  pSvc                 HGCM mock service instance.
 * @param  msTimeout            Timeout (in ms) to wait for connection.
 */
PTSTHGCMMOCKCLIENT TstHgcmMockSvcWaitForConnectEx(PTSTHGCMMOCKSVC pSvc, RTMSINTERVAL msTimeout)
{
    int rc = RTSemEventWait(pSvc->hEventConnect, msTimeout);
    if (RT_SUCCESS(rc))
    {
        Assert(pSvc->uNextClientId);
        return &pSvc->aHgcmClient[pSvc->uNextClientId - 1];
    }
    return NULL;
}

/**
 * Waits for a HGCM mock client to connect.
 *
 * @return Pointer to connected client, or NULL if waiting for connection was aborted.
 * @param  pSvc                 HGCM mock service instance.
 */
PTSTHGCMMOCKCLIENT TstHgcmMockSvcWaitForConnect(PTSTHGCMMOCKSVC pSvc)
{
    return TstHgcmMockSvcWaitForConnectEx(pSvc, RT_MS_30SEC);
}

/**
 * Creates a HGCM mock service instance.
 *
 * @return VBox status code.
 * @param  pSvc                 HGCM mock service instance to create.
 * @param  cbClient             Size (in bytes) of service-specific client data to
 *                              allocate for a HGCM mock client.
 */
int TstHgcmMockSvcCreate(PTSTHGCMMOCKSVC pSvc, size_t cbClient)
{
    AssertReturn(cbClient, VERR_INVALID_PARAMETER);

    RT_ZERO(pSvc->aHgcmClient);
    pSvc->fShutdown = false;
    int rc = RTSemEventCreate(&pSvc->hEventQueue);
    if (RT_SUCCESS(rc))
    {
        rc = RTSemEventCreate(&pSvc->hEventHostCall);
        if (RT_SUCCESS(rc))
        {
            rc = RTSemEventCreate(&pSvc->hEventConnect);
            if (RT_SUCCESS(rc))
            {
                RTListInit(&pSvc->lstCall);

                pSvc->cbClient = cbClient;
            }
        }
    }

    return rc;
}

/**
 * Destroys a HGCM mock service instance.
 *
 * @return VBox status code.
 * @param  pSvc                 HGCM mock service instance to destroy.
 */
int TstHgcmMockSvcDestroy(PTSTHGCMMOCKSVC pSvc)
{
    int rc = RTSemEventDestroy(pSvc->hEventQueue);
    if (RT_SUCCESS(rc))
    {
        rc = RTSemEventDestroy(pSvc->hEventHostCall);
        if (RT_SUCCESS(rc))
            RTSemEventDestroy(pSvc->hEventConnect);
    }
    return rc;
}

/**
 * Starts a HGCM mock service instance.
 *
 * @return VBox status code.
 * @param  pSvc                 HGCM mock service instance to start.
 */
int TstHgcmMockSvcStart(PTSTHGCMMOCKSVC pSvc)
{
    int rc = RTThreadCreate(&pSvc->hThread, tstHgcmMockSvcThread, pSvc, 0, RTTHREADTYPE_DEFAULT, RTTHREADFLAGS_WAITABLE,
                            "MockSvc");
    if (RT_SUCCESS(rc))
        rc = RTThreadUserWait(pSvc->hThread, RT_MS_30SEC);

    return rc;
}

/**
 * Stops a HGCM mock service instance.
 *
 * @return VBox status code.
 * @param  pSvc                 HGCM mock service instance to stop.
 */
int TstHgcmMockSvcStop(PTSTHGCMMOCKSVC pSvc)
{
    ASMAtomicWriteBool(&pSvc->fShutdown, true);

    int rcThread;
    int rc = RTThreadWait(pSvc->hThread, RT_MS_30SEC, &rcThread);
    if (RT_SUCCESS(rc))
        rc = rcThread;
    if (RT_SUCCESS(rc))
    {
        pSvc->hThread = NIL_RTTHREAD;
    }

    return rc;
}


/*********************************************************************************************************************************
*   VbglR3 stubs                                                                                                                 *
*********************************************************************************************************************************/

/**
 * Connects to an HGCM mock service.
 *
 * @returns VBox status code
 * @param   pszServiceName  Name of the host service.
 * @param   pidClient       Where to put the client ID on success. The client ID
 *                          must be passed to all the other calls to the service.
 */
VBGLR3DECL(int) VbglR3HGCMConnect(const char *pszServiceName, HGCMCLIENTID *pidClient)
{
    RT_NOREF(pszServiceName);

    PTSTHGCMMOCKSVC pSvc = TstHgcmMockSvcInst();

    return tstHgcmMockSvcConnect(pSvc, pSvc->fnTable.pvService, pidClient);
}

/**
 * Disconnect from an HGCM mock service.
 *
 * @returns VBox status code.
 * @param   idClient        The client id returned by VbglR3HGCMConnect().
 */
VBGLR3DECL(int) VbglR3HGCMDisconnect(HGCMCLIENTID idClient)
{
    PTSTHGCMMOCKSVC pSvc = TstHgcmMockSvcInst();

    return tstHgcmMockSvcDisconnect(pSvc, pSvc->fnTable.pvService, idClient);
}

/**
 * Makes a fully prepared HGCM call to an HGCM mock service.
 *
 * @returns VBox status code.
 * @param   pInfo           Fully prepared HGCM call info.
 * @param   cbInfo          Size of the info.  This may sometimes be larger than
 *                          what the parameter count indicates because of
 *                          parameter changes between versions and such.
 */
VBGLR3DECL(int) VbglR3HGCMCall(PVBGLIOCHGCMCALL pInfo, size_t cbInfo)
{
    RT_NOREF(cbInfo);

    AssertMsg(pInfo->Hdr.cbIn  == cbInfo, ("cbIn=%#x cbInfo=%#zx\n", pInfo->Hdr.cbIn, cbInfo));
    AssertMsg(pInfo->Hdr.cbOut == cbInfo, ("cbOut=%#x cbInfo=%#zx\n", pInfo->Hdr.cbOut, cbInfo));
    Assert(sizeof(*pInfo) + pInfo->cParms * sizeof(HGCMFunctionParameter) <= cbInfo);

    HGCMFunctionParameter *offSrcParms = VBGL_HGCM_GET_CALL_PARMS(pInfo);
    PVBOXHGCMSVCPARM       paDstParms  = (PVBOXHGCMSVCPARM)RTMemAlloc(pInfo->cParms * sizeof(VBOXHGCMSVCPARM));

    uint16_t i = 0;
    for (; i < pInfo->cParms; i++)
    {
        switch (offSrcParms->type)
        {
            case VMMDevHGCMParmType_32bit:
            {
                paDstParms[i].type     = VBOX_HGCM_SVC_PARM_32BIT;
                paDstParms[i].u.uint32 = offSrcParms->u.value32;
                break;
            }

            case VMMDevHGCMParmType_64bit:
            {
                paDstParms[i].type     = VBOX_HGCM_SVC_PARM_64BIT;
                paDstParms[i].u.uint64 = offSrcParms->u.value64;
                break;
            }

            case VMMDevHGCMParmType_LinAddr:
            {
                paDstParms[i].type           = VBOX_HGCM_SVC_PARM_PTR;
                paDstParms[i].u.pointer.addr = (void *)offSrcParms->u.LinAddr.uAddr;
                paDstParms[i].u.pointer.size = offSrcParms->u.LinAddr.cb;
                break;
            }

            default:
                AssertFailed();
                break;
        }

        offSrcParms++;
    }

    PTSTHGCMMOCKSVC const pSvc = TstHgcmMockSvcInst();

    int rc2 = tstHgcmMockSvcCall(pSvc, pSvc->fnTable.pvService, &pSvc->aHgcmClient[pInfo->u32ClientID].hCall,
                                 pInfo->u32ClientID, pSvc->aHgcmClient[pInfo->u32ClientID].pvClient,
                                 pInfo->u32Function, pInfo->cParms, paDstParms);
    if (RT_SUCCESS(rc2))
    {
        offSrcParms = VBGL_HGCM_GET_CALL_PARMS(pInfo);

        for (i = 0; i < pInfo->cParms; i++)
        {
            paDstParms[i].type = offSrcParms->type;
            switch (paDstParms[i].type)
            {
                case VMMDevHGCMParmType_32bit:
                    offSrcParms->u.value32 = paDstParms[i].u.uint32;
                    break;

                case VMMDevHGCMParmType_64bit:
                    offSrcParms->u.value64 = paDstParms[i].u.uint64;
                    break;

                case VMMDevHGCMParmType_LinAddr:
                {
                    offSrcParms->u.LinAddr.cb = paDstParms[i].u.pointer.size;
                    break;
                }

                default:
                    AssertFailed();
                    break;
            }

            offSrcParms++;
        }
    }

    RTMemFree(paDstParms);

    if (RT_SUCCESS(rc2))
        rc2 = pSvc->aHgcmClient[pInfo->u32ClientID].hCall.rc;

    return rc2;
}

#endif /* IN_RING3 */

#endif /* !VBOX_INCLUDED_GuestHost_HGCMMock_h */
