/* $Id: VBoxSharedClipboardSvc-x11.cpp $ */
/** @file
 * Shared Clipboard Service - Linux host.
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
 * SPDX-License-Identifier: GPL-3.0-only
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_SHARED_CLIPBOARD
#include <iprt/assert.h>
#include <iprt/critsect.h>
#include <iprt/env.h>
#include <iprt/mem.h>
#include <iprt/semaphore.h>
#include <iprt/string.h>
#include <iprt/asm.h>

#include <VBox/GuestHost/SharedClipboard.h>
#include <VBox/GuestHost/SharedClipboard-x11.h>
#include <VBox/HostServices/VBoxClipboardSvc.h>
#include <iprt/errcore.h>

#include "VBoxSharedClipboardSvc-internal.h"

/* Number of currently extablished connections. */
static volatile uint32_t g_cShClConnections;


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Global context information used by the host glue for the X11 clipboard backend.
 */
struct SHCLCONTEXT
{
    /** This mutex is grabbed during any critical operations on the clipboard
     * which might clash with others. */
    RTCRITSECT           CritSect;
    /** X11 context data. */
    SHCLX11CTX           X11;
    /** Pointer to the VBox host client data structure. */
    PSHCLCLIENT          pClient;
    /** We set this when we start shutting down as a hint not to post any new
     * requests. */
    bool                 fShuttingDown;
};


/*********************************************************************************************************************************
*   Prototypes                                                                                                                   *
*********************************************************************************************************************************/
static DECLCALLBACK(int) shClReportFormatsCallback(PSHCLCONTEXT pCtx, uint32_t fFormats, void *pvUser);
static DECLCALLBACK(int) shClSendDataToDestCallback(PSHCLCONTEXT pCtx, void *pv, uint32_t cb, void *pvUser);
static DECLCALLBACK(int) shClRequestDataFromSourceCallback(PSHCLCONTEXT pCtx, SHCLFORMAT uFmt, void **ppv, uint32_t *pcb, void *pvUser);


int ShClBackendInit(PSHCLBACKEND pBackend, VBOXHGCMSVCFNTABLE *pTable)
{
    RT_NOREF(pBackend);

    LogFlowFuncEnter();

    /* Override the connection limit. */
    for (uintptr_t i = 0; i < RT_ELEMENTS(pTable->acMaxClients); i++)
        pTable->acMaxClients[i] = RT_MIN(VBOX_SHARED_CLIPBOARD_X11_CONNECTIONS_MAX, pTable->acMaxClients[i]);

    RT_ZERO(pBackend->Callbacks);
    /* Use internal callbacks by default. */
    pBackend->Callbacks.pfnReportFormats           = shClReportFormatsCallback;
    pBackend->Callbacks.pfnOnRequestDataFromSource = shClRequestDataFromSourceCallback;
    pBackend->Callbacks.pfnOnSendDataToDest        = shClSendDataToDestCallback;

    return VINF_SUCCESS;
}

void ShClBackendDestroy(PSHCLBACKEND pBackend)
{
    RT_NOREF(pBackend);

    LogFlowFuncEnter();
}

void ShClBackendSetCallbacks(PSHCLBACKEND pBackend, PSHCLCALLBACKS pCallbacks)
{
#define SET_FN_IF_NOT_NULL(a_Fn) \
    if (pCallbacks->pfn##a_Fn) \
        pBackend->Callbacks.pfn##a_Fn = pCallbacks->pfn##a_Fn;

    SET_FN_IF_NOT_NULL(ReportFormats);
    SET_FN_IF_NOT_NULL(OnClipboardRead);
    SET_FN_IF_NOT_NULL(OnClipboardWrite);
    SET_FN_IF_NOT_NULL(OnRequestDataFromSource);
    SET_FN_IF_NOT_NULL(OnSendDataToDest);

#undef SET_FN_IF_NOT_NULL
}

/**
 * @note  On the host, we assume that some other application already owns
 *        the clipboard and leave ownership to X11.
 */
int ShClBackendConnect(PSHCLBACKEND pBackend, PSHCLCLIENT pClient, bool fHeadless)
{
    int rc;

    /* Check if maximum allowed connections count has reached. */
    if (ASMAtomicIncU32(&g_cShClConnections) > VBOX_SHARED_CLIPBOARD_X11_CONNECTIONS_MAX)
    {
        ASMAtomicDecU32(&g_cShClConnections);
        LogRel(("Shared Clipboard: maximum amount for client connections reached\n"));
        return VERR_OUT_OF_RESOURCES;
    }

    PSHCLCONTEXT pCtx = (PSHCLCONTEXT)RTMemAllocZ(sizeof(SHCLCONTEXT));
    if (pCtx)
    {
        rc = RTCritSectInit(&pCtx->CritSect);
        if (RT_SUCCESS(rc))
        {
            rc = ShClX11Init(&pCtx->X11, &pBackend->Callbacks, pCtx, fHeadless);
            if (RT_SUCCESS(rc))
            {
                pClient->State.pCtx = pCtx;
                pCtx->pClient = pClient;

                rc = ShClX11ThreadStart(&pCtx->X11, true /* grab shared clipboard */);
                if (RT_FAILURE(rc))
                    ShClX11Destroy(&pCtx->X11);
            }

            if (RT_FAILURE(rc))
                RTCritSectDelete(&pCtx->CritSect);
        }

        if (RT_FAILURE(rc))
        {
            pClient->State.pCtx = NULL;
            RTMemFree(pCtx);
        }
    }
    else
        rc = VERR_NO_MEMORY;

    if (RT_FAILURE(rc))
    {
        /* Restore active connections count. */
        ASMAtomicDecU32(&g_cShClConnections);
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int ShClBackendSync(PSHCLBACKEND pBackend, PSHCLCLIENT pClient)
{
    RT_NOREF(pBackend);

    LogFlowFuncEnter();

    /* Tell the guest we have no data in case X11 is not available.  If
     * there is data in the host clipboard it will automatically be sent to
     * the guest when the clipboard starts up. */
    if (ShClSvcIsBackendActive())
        return ShClSvcHostReportFormats(pClient, VBOX_SHCL_FMT_NONE);
    return VINF_SUCCESS;
}

/*
 * Shut down the shared clipboard service and "disconnect" the guest.
 * Note!  Host glue code
 */
int ShClBackendDisconnect(PSHCLBACKEND pBackend, PSHCLCLIENT pClient)
{
    RT_NOREF(pBackend);

    LogFlowFuncEnter();

    PSHCLCONTEXT pCtx = pClient->State.pCtx;
    AssertPtr(pCtx);

    /* Drop the reference to the client, in case it is still there.  This
     * will cause any outstanding clipboard data requests from X11 to fail
     * immediately. */
    pCtx->fShuttingDown = true;

    int rc = ShClX11ThreadStop(&pCtx->X11);
    /** @todo handle this slightly more reasonably, or be really sure
     *        it won't go wrong. */
    AssertRC(rc);

    ShClX11Destroy(&pCtx->X11);
    RTCritSectDelete(&pCtx->CritSect);

    RTMemFree(pCtx);

    /* Decrease active connections count. */
    ASMAtomicDecU32(&g_cShClConnections);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int ShClBackendReportFormats(PSHCLBACKEND pBackend, PSHCLCLIENT pClient, SHCLFORMATS fFormats)
{
    RT_NOREF(pBackend);

    int rc = ShClX11ReportFormatsToX11(&pClient->State.pCtx->X11, fFormats);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/** Structure describing a request for clipoard data from the guest. */
struct CLIPREADCBREQ
{
    /** User-supplied data pointer, based on the request type. */
    void                *pv;
    /** The size (in bytes) of the the user-supplied pointer in pv. */
    uint32_t             cb;
    /** The actual size of the data written. */
    uint32_t            *pcbActual;
    /** The request's event ID. */
    SHCLEVENTID          idEvent;
};

/**
 * @note   We always fail or complete asynchronously.
 * @note   On success allocates a CLIPREADCBREQ structure which must be
 *         freed in ClipCompleteDataRequestFromX11 when it is called back from
 *         the backend code.
 */
int ShClBackendReadData(PSHCLBACKEND pBackend, PSHCLCLIENT pClient, PSHCLCLIENTCMDCTX pCmdCtx, SHCLFORMAT uFormat,
                        void *pvData, uint32_t cbData, uint32_t *pcbActual)
{
    RT_NOREF(pBackend);

    AssertPtrReturn(pClient,   VERR_INVALID_POINTER);
    AssertPtrReturn(pCmdCtx,   VERR_INVALID_POINTER);
    AssertPtrReturn(pvData,    VERR_INVALID_POINTER);
    AssertPtrReturn(pcbActual, VERR_INVALID_POINTER);

    RT_NOREF(pCmdCtx);

    LogFlowFunc(("pClient=%p, uFormat=%#x, pv=%p, cb=%RU32, pcbActual=%p\n",
                 pClient, uFormat, pvData, cbData, pcbActual));

    int rc;

    CLIPREADCBREQ *pReq = (CLIPREADCBREQ *)RTMemAllocZ(sizeof(CLIPREADCBREQ));
    if (pReq)
    {
        PSHCLEVENT pEvent;
        rc = ShClEventSourceGenerateAndRegisterEvent(&pClient->EventSrc, &pEvent);
        if (RT_SUCCESS(rc))
        {
            pReq->pv        = pvData;
            pReq->cb        = cbData;
            pReq->pcbActual = pcbActual;
            pReq->idEvent   = pEvent->idEvent;

            /* Note: ShClX11ReadDataFromX11() will consume pReq on success. */
            rc = ShClX11ReadDataFromX11(&pClient->State.pCtx->X11, uFormat, pReq);
            if (RT_SUCCESS(rc))
            {
                PSHCLEVENTPAYLOAD pPayload;
                rc = ShClEventWait(pEvent, 30 * 1000, &pPayload);
                if (RT_SUCCESS(rc))
                {
                    if (pPayload)
                    {
                        memcpy(pvData, pPayload->pvData, RT_MIN(cbData, pPayload->cbData));

                        *pcbActual = (uint32_t)pPayload->cbData;

                        ShClPayloadFree(pPayload);
                    }
                    else /* No payload given; could happen on invalid / not-expected formats. */
                        *pcbActual = 0;
                }
            }

            ShClEventRelease(pEvent);
        }

        if (RT_FAILURE(rc))
            RTMemFree(pReq);
    }
    else
        rc = VERR_NO_MEMORY;

    if (RT_FAILURE(rc))
        LogRel(("Shared Clipboard: Error reading host clipboard data from X11, rc=%Rrc\n", rc));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int ShClBackendWriteData(PSHCLBACKEND pBackend, PSHCLCLIENT pClient, PSHCLCLIENTCMDCTX pCmdCtx, SHCLFORMAT uFormat, void *pvData, uint32_t cbData)
{
    RT_NOREF(pBackend, pClient, pCmdCtx, uFormat, pvData, cbData);

    LogFlowFuncEnter();

    /* Nothing to do here yet. */

    LogFlowFuncLeave();
    return VINF_SUCCESS;
}

/** @copydoc SHCLCALLBACKS::pfnReportFormats */
static DECLCALLBACK(int) shClReportFormatsCallback(PSHCLCONTEXT pCtx, uint32_t fFormats, void *pvUser)
{
    RT_NOREF(pvUser);

    LogFlowFunc(("pCtx=%p, fFormats=%#x\n", pCtx, fFormats));

    int rc = VINF_SUCCESS;
    PSHCLCLIENT pClient = pCtx->pClient;
    AssertPtr(pClient);

    rc = RTCritSectEnter(&pClient->CritSect);
    if (RT_SUCCESS(rc))
    {
        if (ShClSvcIsBackendActive())
        {
            /** @todo r=bird: BUGBUG: Revisit this   */
            if (fFormats != VBOX_SHCL_FMT_NONE) /* No formats to report? */
            {
                rc = ShClSvcHostReportFormats(pCtx->pClient, fFormats);
            }
        }

        RTCritSectLeave(&pClient->CritSect);
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/** @copydoc SHCLCALLBACKS::pfnOnSendDataToDest */
static DECLCALLBACK(int) shClSendDataToDestCallback(PSHCLCONTEXT pCtx, void *pv, uint32_t cb, void *pvUser)
{
    AssertPtrReturn(pCtx,   VERR_INVALID_POINTER);
    AssertPtrReturn(pvUser, VERR_INVALID_POINTER);

    PSHCLX11READDATAREQ pData = (PSHCLX11READDATAREQ)pvUser;
    CLIPREADCBREQ      *pReq  = pData->pReq;

    LogFlowFunc(("rcCompletion=%Rrc, pReq=%p, pv=%p, cb=%RU32, idEvent=%RU32\n",
                 pData->rcCompletion, pReq, pv, cb, pReq->idEvent));

    if (pReq->idEvent != NIL_SHCLEVENTID)
    {
        int rc2;

        PSHCLEVENTPAYLOAD pPayload = NULL;
        if (   RT_SUCCESS(pData->rcCompletion)
            && pv
            && cb)
        {
            rc2 = ShClPayloadAlloc(pReq->idEvent, pv, cb, &pPayload);
            AssertRC(rc2);
        }

        rc2 = RTCritSectEnter(&pCtx->pClient->CritSect);
        if (RT_SUCCESS(rc2))
        {
            const PSHCLEVENT pEvent = ShClEventSourceGetFromId(&pCtx->pClient->EventSrc, pReq->idEvent);
            if (pEvent)
                rc2 = ShClEventSignal(pEvent, pPayload);

            RTCritSectLeave(&pCtx->pClient->CritSect);

            if (RT_SUCCESS(rc2))
                pPayload = NULL;
        }

        if (pPayload)
            ShClPayloadFree(pPayload);
    }

    if (pReq)
        RTMemFree(pReq);

    LogRel2(("Shared Clipboard: Reading X11 clipboard data from host completed with %Rrc\n", pData->rcCompletion));

    return VINF_SUCCESS;
}

/** @copydoc SHCLCALLBACKS::pfnOnRequestDataFromSource */
static DECLCALLBACK(int) shClRequestDataFromSourceCallback(PSHCLCONTEXT pCtx, SHCLFORMAT uFmt, void **ppv, uint32_t *pcb, void *pvUser)
{
    RT_NOREF(pvUser);

    LogFlowFunc(("pCtx=%p, uFmt=0x%x\n", pCtx, uFmt));

    if (pCtx->fShuttingDown)
    {
        /* The shared clipboard is disconnecting. */
        LogRel(("Shared Clipboard: Host requested guest clipboard data after guest had disconnected\n"));
        return VERR_WRONG_ORDER;
    }

    PSHCLCLIENT pClient = pCtx->pClient;
    AssertPtr(pClient);

    RTCritSectEnter(&pClient->CritSect);

    int rc = VINF_SUCCESS;

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    /*
     * Note: We always return a generic URI list here.
     *       As we don't know which Atom target format was requested by the caller, the X11 clipboard codes needs
     *       to decide & transform the list into the actual clipboard Atom target format the caller wanted.
     */
    if (uFmt == VBOX_SHCL_FMT_URI_LIST)
    {
        PSHCLTRANSFER pTransfer;
        rc = shClSvcTransferStart(pCtx->pClient,
                                  SHCLTRANSFERDIR_FROM_REMOTE, SHCLSOURCE_REMOTE,
                                  &pTransfer);
        if (RT_SUCCESS(rc))
        {

        }
        else
            LogRel(("Shared Clipboard: Initializing read transfer from guest failed with %Rrc\n", rc));

        *ppv = NULL;
        *pcb = 0;

        rc = VERR_NO_DATA;
    }
#endif

    if (RT_SUCCESS(rc))
    {
        /* Request data from the guest. */
        PSHCLEVENT pEvent;
        rc = ShClSvcGuestDataRequest(pCtx->pClient, uFmt, &pEvent);
        if (RT_SUCCESS(rc))
        {
            RTCritSectLeave(&pClient->CritSect);

            PSHCLEVENTPAYLOAD pPayload;
            rc = ShClEventWait(pEvent, 30 * 1000, &pPayload);
            if (RT_SUCCESS(rc))
            {
                if (   !pPayload
                    || !pPayload->cbData)
                {
                    rc = VERR_NO_DATA;
                }
                else
                {
                    *ppv = pPayload->pvData;
                    *pcb = pPayload->cbData;
                }
            }

            RTCritSectEnter(&pClient->CritSect);

            ShClEventRelease(pEvent);
        }
    }

    RTCritSectLeave(&pClient->CritSect);

    if (RT_FAILURE(rc))
        LogRel(("Shared Clipboard: Requesting data in format %#x for X11 host failed with %Rrc\n", uFmt, rc));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS

int ShClBackendTransferCreate(PSHCLBACKEND pBackend, PSHCLCLIENT pClient, PSHCLTRANSFER pTransfer)
{
    RT_NOREF(pBackend);
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS_HTTP
    return ShClHttpTransferRegister(&pClient->State.pCtx->X11.HttpCtx, pTransfer);
#else
    RT_NOREF(pClient, pTransfer);
#endif
    return VERR_NOT_IMPLEMENTED;
}

int ShClBackendTransferDestroy(PSHCLBACKEND pBackend, PSHCLCLIENT pClient, PSHCLTRANSFER pTransfer)
{
    RT_NOREF(pBackend);
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS_HTTP
    return ShClHttpTransferUnregister(&pClient->State.pCtx->X11.HttpCtx, pTransfer);
#else
    RT_NOREF(pClient, pTransfer);
#endif

    return VINF_SUCCESS;
}

int ShClBackendTransferGetRoots(PSHCLBACKEND pBackend, PSHCLCLIENT pClient, PSHCLTRANSFER pTransfer)
{
    RT_NOREF(pBackend);

    LogFlowFuncEnter();

    PSHCLEVENT pEvent;
    int rc = ShClEventSourceGenerateAndRegisterEvent(&pClient->EventSrc, &pEvent);
    if (RT_SUCCESS(rc))
    {
        CLIPREADCBREQ *pReq = (CLIPREADCBREQ *)RTMemAllocZ(sizeof(CLIPREADCBREQ));
        if (pReq)
        {
            pReq->idEvent = pEvent->idEvent;

            rc = ShClX11ReadDataFromX11(&pClient->State.pCtx->X11, VBOX_SHCL_FMT_URI_LIST, pReq);
            if (RT_SUCCESS(rc))
            {
                /* X supplies the data asynchronously, so we need to wait for data to arrive first. */
                PSHCLEVENTPAYLOAD pPayload;
                rc = ShClEventWait(pEvent, 30 * 1000, &pPayload);
                if (RT_SUCCESS(rc))
                {
                    rc = ShClTransferRootsSet(pTransfer,
                                              (char *)pPayload->pvData, pPayload->cbData + 1 /* Include termination */);
                }
            }
        }
        else
            rc = VERR_NO_MEMORY;

        ShClEventRelease(pEvent);
    }
    else
        rc = VERR_SHCLPB_MAX_EVENTS_REACHED;

    LogFlowFuncLeaveRC(rc);
    return rc;
}
#endif /* VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS */

