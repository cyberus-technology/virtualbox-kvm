/* $Id: clipboard-transfers-http.cpp $ */
/** @file
 * Shared Clipboard: HTTP server implementation for Shared Clipboard transfers on UNIX-y guests / hosts.
 */

/*
 * Copyright (C) 2020-2023 Oracle and/or its affiliates.
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
 * SPDX-License-Identifier: GPL-3.0-only
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <signal.h>

#include <iprt/http.h>
#include <iprt/http-server.h>

#include <iprt/net.h> /* To make use of IPv4Addr in RTGETOPTUNION. */

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/ctype.h>
#include <iprt/errcore.h>
#include <iprt/file.h>
#include <iprt/getopt.h>
#include <iprt/initterm.h>
#include <iprt/list.h>
#define LOG_GROUP LOG_GROUP_SHARED_CLIPBOARD
#include <iprt/log.h>
#include <iprt/mem.h>
#include <iprt/message.h>
#include <iprt/path.h>
#include <iprt/rand.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/thread.h>
#include <iprt/uuid.h>
#include <iprt/vfs.h>

#include <VBox/HostServices/VBoxClipboardSvc.h>
#include <VBox/GuestHost/SharedClipboard-x11.h>
#include <VBox/GuestHost/SharedClipboard-transfers.h>


/*********************************************************************************************************************************
*   Definitations                                                                                                                *
*********************************************************************************************************************************/

typedef struct _SHCLHTTPSERVERTRANSFER
{
    /** The node list. */
    RTLISTNODE          Node;
    /** Pointer to associated transfer. */
    PSHCLTRANSFER       pTransfer;
    /** The (cached) root list of the transfer. NULL if not cached yet. */
    PSHCLROOTLIST       pRootList;
    /** Critical section for serializing access. */
    RTCRITSECT          CritSect;
    /** The handle we're going to use for this HTTP transfer. */
    SHCLOBJHANDLE       hObj;
    /** The virtual path of the HTTP server's root directory for this transfer. */
    char                szPathVirtual[RTPATH_MAX];
} SHCLHTTPSERVERTRANSFER;
typedef SHCLHTTPSERVERTRANSFER *PSHCLHTTPSERVERTRANSFER;


/*********************************************************************************************************************************
*   Prototypes                                                                                                                   *
*********************************************************************************************************************************/
static int shClTransferHttpServerDestroyInternal(PSHCLHTTPSERVER pThis);
static const char *shClTransferHttpServerGetHost(PSHCLHTTPSERVER pSrv);


/*********************************************************************************************************************************
*   Public Shared Clipboard HTTP transfer functions                                                                              *
*********************************************************************************************************************************/

/**
 * Registers a Shared Clipboard transfer to a HTTP context.
 *
 * @returns VBox status code.
 * @param   pCtx                HTTP context to register transfer for.
 * @param   pTransfer           Transfer to register.
 */
int ShClHttpTransferRegister(PSHCLHTTPCONTEXT pCtx, PSHCLTRANSFER pTransfer)
{
    int rc = VINF_SUCCESS;

    /* Start the built-in HTTP server to serve file(s). */
    if (!ShClTransferHttpServerIsRunning(&pCtx->HttpServer)) /* Only one HTTP server per transfer context. */
        rc = ShClTransferHttpServerCreate(&pCtx->HttpServer, NULL /* puPort */);

    if (RT_SUCCESS(rc))
        rc = ShClTransferHttpServerRegisterTransfer(&pCtx->HttpServer, pTransfer);

    return rc;
}

/**
 * Unregisters a formerly registered Shared Clipboard transfer.
 *
 * @returns VBox status code.
 * @param   pCtx                HTTP context to unregister transfer from.
 * @param   pTransfer           Transfer to unregister.
 */
int ShClHttpTransferUnregister(PSHCLHTTPCONTEXT pCtx, PSHCLTRANSFER pTransfer)
{
    int rc = VINF_SUCCESS;

    if (ShClTransferHttpServerIsRunning(&pCtx->HttpServer))
    {
        /* Try unregistering transfer (if it was registered before). */
        rc = ShClTransferHttpServerUnregisterTransfer(&pCtx->HttpServer, pTransfer);
        if (RT_SUCCESS(rc))
        {
            /* No more registered transfers left? Tear down the HTTP server instance then. */
            if (ShClTransferHttpServerGetTransferCount(&pCtx->HttpServer) == 0)
                rc = ShClTransferHttpServerDestroy(&pCtx->HttpServer);
        }
        AssertRC(rc);
    }

    return rc;
}


/*********************************************************************************************************************************
*   Internal Shared Clipboard HTTP transfer functions                                                                            *
*********************************************************************************************************************************/

DECLINLINE(void) shClHttpTransferLock(PSHCLHTTPSERVERTRANSFER pSrvTx)
{
    int rc2 = RTCritSectEnter(&pSrvTx->CritSect);
    AssertRC(rc2);
}

DECLINLINE(void) shClHttpTransferUnlock(PSHCLHTTPSERVERTRANSFER pSrvTx)
{
    int rc2 = RTCritSectEnter(&pSrvTx->CritSect);
    AssertRC(rc2);
}

/**
 * Return the HTTP server transfer for a specific transfer ID.
 *
 * @returns Pointer to HTTP server transfer if found, NULL if not found.
 * @param   pSrv                HTTP server instance.
 * @param   idTransfer          Transfer ID to return HTTP server transfer for.
 */
static PSHCLHTTPSERVERTRANSFER shClTransferHttpServerGetTransferById(PSHCLHTTPSERVER pSrv, SHCLTRANSFERID idTransfer)
{
    PSHCLHTTPSERVERTRANSFER pSrvTx;
    RTListForEach(&pSrv->lstTransfers, pSrvTx, SHCLHTTPSERVERTRANSFER, Node) /** @todo Slow O(n) lookup, but does it for now. */
    {
        if (pSrvTx->pTransfer->State.uID == idTransfer)
            return pSrvTx;
    }

    return NULL;
}

/**
 * Returns a HTTP server transfer from a given URL.
 *
 * @returns Pointer to HTTP server transfer if found, NULL if not found.
 * @param   pThis               HTTP server instance data.
 * @param   pszUrl              URL to validate.
 */
DECLINLINE(PSHCLHTTPSERVERTRANSFER) shClTransferHttpGetTransferFromUrl(PSHCLHTTPSERVER pThis, const char *pszUrl)
{
    AssertPtrReturn(pszUrl, NULL);

    PSHCLHTTPSERVERTRANSFER pSrvTx = NULL;

    PSHCLHTTPSERVERTRANSFER pSrvTxCur;
    RTListForEach(&pThis->lstTransfers, pSrvTxCur, SHCLHTTPSERVERTRANSFER, Node)
    {
        AssertPtr(pSrvTxCur->pTransfer);

        LogFlowFunc(("pSrvTxCur=%s\n", pSrvTxCur->szPathVirtual));

        /* Be picky here, do a case sensitive comparison. */
        if (RTStrStartsWith(pszUrl, pSrvTxCur->szPathVirtual))
        {
            pSrvTx = pSrvTxCur;
            break;
        }
    }

    if (!pSrvTx)
        LogRel2(("Shared Clipboard: HTTP URL '%s' not valid\n", pszUrl));

    LogFlowFunc(("pszUrl=%s, pSrvTx=%p\n", pszUrl, pSrvTx));
    return pSrvTx;
}

/**
 * Returns a HTTP server transfer from an internal HTTP handle.
 *
 * @returns Pointer to HTTP server transfer if found, NULL if not found.
 * @param   pThis               HTTP server instance data.
 * @param   pvHandle            Handle to return transfer for.
 */
DECLINLINE(PSHCLHTTPSERVERTRANSFER) shClTransferHttpGetTransferFromHandle(PSHCLHTTPSERVER pThis, void *pvHandle)
{
    AssertPtrReturn(pvHandle, NULL);

    const SHCLTRANSFERID uHandle = *(uint16_t *)pvHandle;

    /** @ŧodo Use a handle lookup table (map) later. */
    PSHCLHTTPSERVERTRANSFER pSrvTxCur;
    RTListForEach(&pThis->lstTransfers, pSrvTxCur, SHCLHTTPSERVERTRANSFER, Node)
    {
        AssertPtr(pSrvTxCur->pTransfer);

        if (pSrvTxCur->pTransfer->State.uID == uHandle) /** @ŧodo We're using the transfer ID as handle for now. */
            return pSrvTxCur;
    }

    return NULL;
}

static int shClTransferHttpGetTransferRoots(PSHCLHTTPSERVER pThis, PSHCLHTTPSERVERTRANSFER pSrvTx)
{
    RT_NOREF(pThis);

    int rc = VINF_SUCCESS;

    if (pSrvTx->pRootList == NULL)
    {
        AssertPtr(pSrvTx->pTransfer);
        rc = ShClTransferRootsGet(pSrvTx->pTransfer, &pSrvTx->pRootList);
    }

    return rc;
}


/*********************************************************************************************************************************
*   HTTP server callback implementations                                                                                         *
*********************************************************************************************************************************/

/** @copydoc RTHTTPSERVERCALLBACKS::pfnOpen */
static DECLCALLBACK(int) shClTransferHttpOpen(PRTHTTPCALLBACKDATA pData, PRTHTTPSERVERREQ pReq, void **ppvHandle)
{
    PSHCLHTTPSERVER pThis = (PSHCLHTTPSERVER)pData->pvUser;
    Assert(pData->cbUser == sizeof(SHCLHTTPSERVER));

    int rc;

    PSHCLHTTPSERVERTRANSFER pSrvTx = shClTransferHttpGetTransferFromUrl(pThis, pReq->pszUrl);
    if (pSrvTx)
    {
        shClHttpTransferLock(pSrvTx);

        AssertPtr(pSrvTx->pTransfer);

        SHCLOBJOPENCREATEPARMS openParms;
        rc = ShClTransferObjOpenParmsInit(&openParms);
        if (RT_SUCCESS(rc))
        {
            openParms.fCreate = SHCL_OBJ_CF_ACCESS_READ
                              | SHCL_OBJ_CF_ACCESS_DENYWRITE;

            rc = RTStrCopy(openParms.pszPath, openParms.cbPath, "foo"); /** @ŧodo BUGBUG !!!! */
            if (RT_SUCCESS(rc))
            {
                rc = ShClTransferObjOpen(pSrvTx->pTransfer, &openParms, &pSrvTx->hObj);
                if (RT_SUCCESS(rc))
                {
                    *ppvHandle = &pSrvTx->hObj;
                    LogRel2(("Shared Clipboard: HTTP transfer (handle %RU64) started ...\n", pSrvTx->hObj));
                }
            }

            ShClTransferObjOpenParmsDestroy(&openParms);
        }

        shClHttpTransferUnlock(pSrvTx);
    }
    else
        rc = VERR_NOT_FOUND;

    if (RT_FAILURE(rc))
        LogRel(("Shared Clipboard: Error starting HTTP transfer for '%s', rc=%Rrc\n", pReq->pszUrl, rc));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/** @copydoc RTHTTPSERVERCALLBACKS::pfnRead */
static DECLCALLBACK(int) shClTransferHttpRead(PRTHTTPCALLBACKDATA pData, void *pvHandle, void *pvBuf, size_t cbBuf, size_t *pcbRead)
{
    PSHCLHTTPSERVER pThis = (PSHCLHTTPSERVER)pData->pvUser;
    Assert(pData->cbUser == sizeof(SHCLHTTPSERVER));

    RT_NOREF(pvBuf, cbBuf, pcbRead);

    int rc;

    PSHCLHTTPSERVERTRANSFER pSrvTx = shClTransferHttpGetTransferFromHandle(pThis, pvHandle);
    if (pSrvTx)
    {
        Assert(pSrvTx->hObj != SHCLOBJHANDLE_INVALID);

        uint32_t cbRead;
        rc = ShClTransferObjRead(pSrvTx->pTransfer, pSrvTx->hObj, pvBuf, cbBuf, 0 /* fFlags */, &cbRead);
        if (RT_SUCCESS(rc))
        {
            *pcbRead = (uint32_t)cbRead;
        }

        if (RT_FAILURE(rc))
            LogRel(("Shared Clipboard: Error reading HTTP transfer (handle %RU64), rc=%Rrc\n", pSrvTx->hObj, rc));
    }
    else
        rc = VERR_NOT_FOUND;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/** @copydoc RTHTTPSERVERCALLBACKS::pfnClose */
static DECLCALLBACK(int) shClTransferHttpClose(PRTHTTPCALLBACKDATA pData, void *pvHandle)
{
    PSHCLHTTPSERVER pThis = (PSHCLHTTPSERVER)pData->pvUser;
    Assert(pData->cbUser == sizeof(SHCLHTTPSERVER));

    int rc;

    PSHCLHTTPSERVERTRANSFER pSrvTx = shClTransferHttpGetTransferFromHandle(pThis, pvHandle);
    if (pSrvTx)
    {
        shClHttpTransferLock(pSrvTx);

        Assert(pSrvTx->hObj != SHCLOBJHANDLE_INVALID);
        rc = ShClTransferObjClose(pSrvTx->pTransfer, pSrvTx->hObj);
        if (RT_SUCCESS(rc))
        {
            pSrvTx->hObj = SHCLOBJHANDLE_INVALID;
            LogRel2(("Shared Clipboard: HTTP transfer %RU16 done\n", pSrvTx->pTransfer->State.uID));
        }

        if (RT_FAILURE(rc))
            LogRel(("Shared Clipboard: Error closing HTTP transfer (handle %RU64), rc=%Rrc\n", pSrvTx->hObj, rc));

        shClHttpTransferUnlock(pSrvTx);
    }
    else
        rc = VERR_NOT_FOUND;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/** @copydoc RTHTTPSERVERCALLBACKS::pfnQueryInfo */
static DECLCALLBACK(int) shClTransferHttpQueryInfo(PRTHTTPCALLBACKDATA pData,
                                                   PRTHTTPSERVERREQ pReq, PRTFSOBJINFO pObjInfo, char **ppszMIMEHint)
{
    PSHCLHTTPSERVER pThis = (PSHCLHTTPSERVER)pData->pvUser;
    Assert(pData->cbUser == sizeof(SHCLHTTPSERVER));

    int rc;

    PSHCLHTTPSERVERTRANSFER pSrvTx = shClTransferHttpGetTransferFromUrl(pThis, pReq->pszUrl);
    if (pSrvTx)
    {
        shClHttpTransferLock(pSrvTx);

        rc = shClTransferHttpGetTransferRoots(pThis, pSrvTx);

        shClHttpTransferUnlock(pSrvTx);
    }
    else
        rc = VERR_NOT_FOUND;

    RT_NOREF(pObjInfo, ppszMIMEHint);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/** @copydoc RTHTTPSERVERCALLBACKS::pfnDestroy */
static DECLCALLBACK(int) shClTransferHttpDestroy(PRTHTTPCALLBACKDATA pData)
{
    PSHCLHTTPSERVER pThis = (PSHCLHTTPSERVER)pData->pvUser;
    Assert(pData->cbUser == sizeof(SHCLHTTPSERVER));

    return shClTransferHttpServerDestroyInternal(pThis);
}


/*********************************************************************************************************************************
*   Internal Shared Clipboard HTTP server functions                                                                              *
*********************************************************************************************************************************/

/**
 * Destroys a Shared Clipboard HTTP server instance, internal version.
 *
 * @returns VBox status code.
 * @param   pSrv                Shared Clipboard HTTP server instance to destroy.
 */
static int shClTransferHttpServerDestroyInternal(PSHCLHTTPSERVER pSrv)
{
    PSHCLHTTPSERVERTRANSFER pSrvTx, pSrvTxNext;
    RTListForEachSafe(&pSrv->lstTransfers, pSrvTx, pSrvTxNext, SHCLHTTPSERVERTRANSFER, Node)
    {
        RTListNodeRemove(&pSrvTx->Node);

        RTMemFree(pSrvTx);
        pSrvTx = NULL;
    }

    RTHttpServerResponseDestroy(&pSrv->Resp);

    pSrv->hHTTPServer = NIL_RTHTTPSERVER;

    int rc = VINF_SUCCESS;

    if (RTCritSectIsInitialized(&pSrv->CritSect))
        rc = RTCritSectDelete(&pSrv->CritSect);

    return rc;
}

/**
 * Locks the critical section of a Shared Clipboard HTTP server instance.
 *
 * @param   pSrv                Shared Clipboard HTTP server instance to lock.
 */
DECLINLINE(void) shClTransferHttpServerLock(PSHCLHTTPSERVER pSrv)
{
    int rc2 = RTCritSectEnter(&pSrv->CritSect);
    AssertRC(rc2);
}

/**
 * Unlocks the critical section of a Shared Clipboard HTTP server instance.
 *
 * @param   pSrv                Shared Clipboard HTTP server instance to unlock.
 */
DECLINLINE(void) shClTransferHttpServerUnlock(PSHCLHTTPSERVER pSrv)
{
    int rc2 = RTCritSectLeave(&pSrv->CritSect);
    AssertRC(rc2);
}

/**
 * Initializes a new Shared Clipboard HTTP server instance.
 *
 * @param   pSrv                HTTP server instance to initialize.
 */
static void shClTransferHttpServerInitInternal(PSHCLHTTPSERVER pSrv)
{
    pSrv->hHTTPServer = NIL_RTHTTPSERVER;
    pSrv->uPort       = 0;
    RTListInit(&pSrv->lstTransfers);
    pSrv->cTransfers  = 0;
    int rc2 = RTHttpServerResponseInit(&pSrv->Resp);
    AssertRC(rc2);
}


/*********************************************************************************************************************************
*   Public Shared Clipboard HTTP server functions                                                                                *
*********************************************************************************************************************************/

/**
 * Initializes a new Shared Clipboard HTTP server instance.
 *
 * @param   pSrv                HTTP server instance to initialize.
 */
void ShClTransferHttpServerInit(PSHCLHTTPSERVER pSrv)
{
    AssertPtrReturnVoid(pSrv);

    shClTransferHttpServerInitInternal(pSrv);
}

/**
 * Creates a new Shared Clipboard HTTP server instance, extended version.
 *
 * @returns VBox status code.
 * @param   pSrv                HTTP server instance to create.
 * @param   uPort               TCP port number to use.
 */
int ShClTransferHttpServerCreateEx(PSHCLHTTPSERVER pSrv, uint16_t uPort)
{
    AssertPtrReturn(pSrv, VERR_INVALID_POINTER);

    RTHTTPSERVERCALLBACKS Callbacks;
    RT_ZERO(Callbacks);

    Callbacks.pfnOpen          = shClTransferHttpOpen;
    Callbacks.pfnRead          = shClTransferHttpRead;
    Callbacks.pfnClose         = shClTransferHttpClose;
    Callbacks.pfnQueryInfo     = shClTransferHttpQueryInfo;
    Callbacks.pfnDestroy       = shClTransferHttpDestroy;

    /* Note: The server always and *only* runs against the localhost interface. */
    int rc = RTHttpServerCreate(&pSrv->hHTTPServer, "localhost", uPort, &Callbacks,
                                pSrv, sizeof(SHCLHTTPSERVER));
    if (RT_SUCCESS(rc))
    {
        rc = RTCritSectInit(&pSrv->CritSect);
        AssertRCReturn(rc, rc);

        pSrv->uPort = uPort;

        LogRel2(("Shared Clipboard: HTTP server running at port %RU16\n", pSrv->uPort));
    }
    else
    {
        int rc2 = shClTransferHttpServerDestroyInternal(pSrv);
        AssertRC(rc2);
    }

    if (RT_FAILURE(rc))
        LogRel(("Shared Clipboard: HTTP server failed to run, rc=%Rrc\n", rc));

    return rc;
}

/**
 * Creates a new Shared Clipboard HTTP server instance.
 *
 * This does automatic probing of TCP ports if one already is being used.
 *
 * @returns VBox status code.
 * @param   pSrv                HTTP server instance to create.
 * @param   puPort              Where to return the TCP port number being used on success. Optional.
 */
int ShClTransferHttpServerCreate(PSHCLHTTPSERVER pSrv, uint16_t *puPort)
{
    AssertPtrReturn(pSrv, VERR_INVALID_POINTER);
    /* puPort is optional. */

    /** @todo Try favorite ports first (e.g. 8080, 8000, ...)? */

    RTRAND hRand;
    int rc = RTRandAdvCreateSystemFaster(&hRand); /* Should be good enough for this task. */
    if (RT_SUCCESS(rc))
    {
        uint16_t uPort;
        for (int i = 0; i < 32; i++)
        {
#ifdef DEBUG_andy
            uPort = 8080; /* Make the port predictable, but only for me, mwahaha! :-). */
#else
            uPort = RTRandAdvU32Ex(hRand, 1024, UINT16_MAX);
#endif
            rc = ShClTransferHttpServerCreateEx(pSrv, (uint32_t)uPort);
            if (RT_SUCCESS(rc))
            {
                if (puPort)
                    *puPort = uPort;
                break;
            }
        }

        RTRandAdvDestroy(hRand);
    }

    return rc;
}

/**
 * Destroys a Shared Clipboard HTTP server instance.
 *
 * @returns VBox status code.
 * @param   pSrv                HTTP server instance to destroy.
 */
int ShClTransferHttpServerDestroy(PSHCLHTTPSERVER pSrv)
{
    AssertPtrReturn(pSrv, VERR_INVALID_POINTER);

    if (pSrv->hHTTPServer == NIL_RTHTTPSERVER)
        return VINF_SUCCESS;

    Assert(pSrv->cTransfers == 0); /* Sanity. */

    int rc = RTHttpServerDestroy(pSrv->hHTTPServer);
    if (RT_SUCCESS(rc))
        rc = shClTransferHttpServerDestroyInternal(pSrv);

    if (RT_SUCCESS(rc))
        LogRel2(("Shared Clipboard: HTTP server stopped\n"));
    else
        LogRel(("Shared Clipboard: HTTP server failed to stop, rc=%Rrc\n", rc));

    return rc;
}

/**
 * Registers a Shared Clipboard transfer to a HTTP server instance.
 *
 * @returns VBox status code.
 * @param   pSrv                HTTP server instance to register transfer for.
 * @param   pTransfer           Transfer to register.
 */
int ShClTransferHttpServerRegisterTransfer(PSHCLHTTPSERVER pSrv, PSHCLTRANSFER pTransfer)
{
    AssertPtrReturn(pSrv, VERR_INVALID_POINTER);
    AssertPtrReturn(pTransfer, VERR_INVALID_POINTER);

    AssertReturn(pTransfer->State.uID, VERR_INVALID_PARAMETER); /* Paranoia. */

    shClTransferHttpServerLock(pSrv);

    PSHCLHTTPSERVERTRANSFER pSrvTx = (PSHCLHTTPSERVERTRANSFER)RTMemAllocZ(sizeof(SHCLHTTPSERVERTRANSFER));
    AssertPtrReturn(pSrvTx, VERR_NO_MEMORY);

    RTUUID Uuid;
    int rc = RTUuidCreate(&Uuid);
    if (RT_SUCCESS(rc))
    {
        char szUuid[64];
        rc = RTUuidToStr(&Uuid, szUuid, sizeof(szUuid));
        if (RT_SUCCESS(rc))
        {
            rc = RTCritSectInit(&pSrvTx->CritSect);
            AssertRC(rc);

            /* Create the virtual HTTP path for the transfer.
             * Every transfer has a dedicated HTTP path. */
#ifdef DEBUG_andy
            ssize_t cch = RTStrPrintf2(pSrvTx->szPathVirtual, sizeof(pSrvTx->szPathVirtual), "/d1bbda60-80b7-45dc-a41c-ac4686c1d988/10664");
#else
            ssize_t cch = RTStrPrintf2(pSrvTx->szPathVirtual, sizeof(pSrvTx->szPathVirtual), "%s/%RU16/", szUuid, pTransfer->State.uID);
#endif
            AssertReturn(cch, VERR_BUFFER_OVERFLOW);

            pSrvTx->pTransfer = pTransfer;
            pSrvTx->pRootList = NULL;
            pSrvTx->hObj      = SHCLOBJHANDLE_INVALID;

            RTListAppend(&pSrv->lstTransfers, &pSrvTx->Node);
            pSrv->cTransfers++;

            LogFunc(("pTransfer=%p, idTransfer=%RU16, szPath=%s -> %RU32 transfers\n",
                     pSrvTx->pTransfer, pSrvTx->pTransfer->State.uID, pSrvTx->szPathVirtual, pSrv->cTransfers));

            LogRel2(("Shared Clipboard: Registered HTTP transfer %RU16, now %RU32 HTTP transfers total\n",
                     pTransfer->State.uID, pSrv->cTransfers));
        }
    }

    if (RT_FAILURE(rc))
        RTMemFree(pSrvTx);

    shClTransferHttpServerUnlock(pSrv);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Unregisters a formerly registered Shared Clipboard transfer.
 *
 * @returns VBox status code.
 * @param   pSrv                HTTP server instance to unregister transfer from.
 * @param   pTransfer           Transfer to unregister.
 */
int ShClTransferHttpServerUnregisterTransfer(PSHCLHTTPSERVER pSrv, PSHCLTRANSFER pTransfer)
{
    AssertPtrReturn(pSrv, VERR_INVALID_POINTER);
    AssertPtrReturn(pTransfer, VERR_INVALID_POINTER);

    shClTransferHttpServerLock(pSrv);

    AssertReturn(pSrv->cTransfers, VERR_WRONG_ORDER);

    int rc = VINF_SUCCESS;

    PSHCLHTTPSERVERTRANSFER pSrvTx;
    RTListForEach(&pSrv->lstTransfers, pSrvTx, SHCLHTTPSERVERTRANSFER, Node)
    {
        AssertPtr(pSrvTx->pTransfer);
        if (pSrvTx->pTransfer->State.uID == pTransfer->State.uID)
        {
            RTListNodeRemove(&pSrvTx->Node);

            Assert(pSrv->cTransfers);
            pSrv->cTransfers--;

            LogFunc(("pTransfer=%p, idTransfer=%RU16, szPath=%s -> %RU32 transfers\n",
                     pSrvTx->pTransfer, pSrvTx->pTransfer->State.uID, pSrvTx->szPathVirtual, pSrv->cTransfers));

            LogRel2(("Shared Clipboard: Unregistered HTTP transfer %RU16, now %RU32 HTTP transfers total\n",
                     pTransfer->State.uID, pSrv->cTransfers));

            rc = RTCritSectDelete(&pSrvTx->CritSect);
            AssertRC(rc);

            RTMemFree(pSrvTx);
            pSrvTx = NULL;

            rc = VINF_SUCCESS;
            break;
        }
    }

    shClTransferHttpServerUnlock(pSrv);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Returns whether a specific transfer ID is registered with a HTTP server instance or not.
 *
 * @returns \c true if the transfer ID is registered, \c false if not.
 * @param   pSrv                HTTP server instance.
 * @param   idTransfer          Transfer ID to check for.
 */
bool ShClTransferHttpServerHasTransfer(PSHCLHTTPSERVER pSrv, SHCLTRANSFERID idTransfer)
{
    AssertPtrReturn(pSrv, false);

    shClTransferHttpServerLock(pSrv);

    const bool fRc = shClTransferHttpServerGetTransferById(pSrv, idTransfer) != NULL;

    shClTransferHttpServerUnlock(pSrv);

    return fRc;
}

/**
 * Returns the used TCP port number of a HTTP server instance.
 *
 * @returns TCP port number. 0 if not specified yet.
 * @param   pSrv                HTTP server instance to return port for.
 */
uint16_t ShClTransferHttpServerGetPort(PSHCLHTTPSERVER pSrv)
{
    AssertPtrReturn(pSrv, 0);

    shClTransferHttpServerLock(pSrv);

    const uint16_t uPort = pSrv->uPort;

    shClTransferHttpServerUnlock(pSrv);

    return uPort;
}

/**
 * Returns the number of registered HTTP server transfers of a HTTP server instance.
 *
 * @returns Number of registered transfers.
 * @param   pSrv                HTTP server instance to return registered transfers for.
 */
uint32_t ShClTransferHttpServerGetTransferCount(PSHCLHTTPSERVER pSrv)
{
    AssertPtrReturn(pSrv, 0);

    shClTransferHttpServerLock(pSrv);

    const uint32_t cTransfers = pSrv->cTransfers;

    shClTransferHttpServerUnlock(pSrv);

    return cTransfers;
}

/**
 * Returns the host name (scheme) of a HTTP server instance.
 *
 * @param   pSrv                HTTP server instance to return host name (scheme) for.
 *
 * @returns Host name (scheme).
 */
static const char *shClTransferHttpServerGetHost(PSHCLHTTPSERVER pSrv)
{
    RT_NOREF(pSrv);
    return "http://localhost"; /* Hardcoded for now. */
}

/**
 * Returns an allocated string with a HTTP server instance's address.
 *
 * @returns Allocated string with a HTTP server instance's address, or NULL on OOM.
 *          Needs to be free'd by the caller using RTStrFree().
 * @param   pSrv                HTTP server instance to return address for.
 */
char *ShClTransferHttpServerGetAddressA(PSHCLHTTPSERVER pSrv)
{
    AssertPtrReturn(pSrv, NULL);

    shClTransferHttpServerLock(pSrv);

    char *pszAddress = RTStrAPrintf2("%s:%RU16", shClTransferHttpServerGetHost(pSrv), pSrv->uPort);
    AssertPtr(pszAddress);

    shClTransferHttpServerUnlock(pSrv);

    return pszAddress;
}

/**
 * Returns an allocated string with the URL of a given Shared Clipboard transfer ID.
 *
 * @returns Allocated string with the URL of a given Shared Clipboard transfer ID, or NULL if not found.
 *          Needs to be free'd by the caller using RTStrFree().
 * @param   pSrv                HTTP server instance to return URL for.
 */
char *ShClTransferHttpServerGetUrlA(PSHCLHTTPSERVER pSrv, SHCLTRANSFERID idTransfer)
{
    AssertPtrReturn(pSrv, NULL);
    AssertReturn(idTransfer != NIL_SHCLTRANSFERID, NULL);

    shClTransferHttpServerLock(pSrv);

    PSHCLHTTPSERVERTRANSFER pSrvTx = shClTransferHttpServerGetTransferById(pSrv, idTransfer);
    if (!pSrvTx)
    {
        AssertFailed();
        shClTransferHttpServerUnlock(pSrv);
        return NULL;
    }

    AssertReturn(RTStrNLen(pSrvTx->szPathVirtual, RTPATH_MAX), NULL);
    char *pszUrl = RTStrAPrintf2("%s:%RU16/%s", shClTransferHttpServerGetHost(pSrv), pSrv->uPort, pSrvTx->szPathVirtual);
    AssertPtr(pszUrl);

    shClTransferHttpServerUnlock(pSrv);

    return pszUrl;
}

/**
 * Returns whether a given HTTP server instance is running or not.
 *
 * @returns \c true if running, or \c false if not.
 * @param   pSrv                HTTP server instance to check running state for.
 */
bool ShClTransferHttpServerIsRunning(PSHCLHTTPSERVER pSrv)
{
    AssertPtrReturn(pSrv, false);

    return (pSrv->hHTTPServer != NIL_RTHTTPSERVER); /* Seems enough for now. */
}
