/** @file
 * Host channel.
 */

/*
 * Copyright (C) 2012-2023 Oracle and/or its affiliates.
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

#include <iprt/alloc.h>
#include <iprt/string.h>
#include <iprt/asm.h>
#include <iprt/assert.h>

#include "HostChannel.h"


static DECLCALLBACK(void) HostChannelCallbackEvent(void *pvCallbacks, void *pvInstance,
                                                   uint32_t u32Id, const void *pvEvent, uint32_t cbEvent);
static DECLCALLBACK(void) HostChannelCallbackDeleted(void *pvCallbacks, void *pvChannel);


/* A registered provider of channels. */
typedef struct VBOXHOSTCHPROVIDER
{
    int32_t volatile cRefs;

    RTLISTNODE nodeContext; /* Member of the list of providers in the service context. */

    VBOXHOSTCHCTX *pCtx;

    VBOXHOSTCHANNELINTERFACE iface;

    char *pszName;

    RTLISTANCHOR listChannels;
} VBOXHOSTCHPROVIDER;

/* An established channel. */
typedef struct VBOXHOSTCHINSTANCE
{
    int32_t volatile cRefs;

    RTLISTNODE nodeClient;    /* In the client, for cleanup when a client disconnects. */
    RTLISTNODE nodeProvider;  /* In the provider, needed for cleanup when the provider is unregistered. */

    VBOXHOSTCHCLIENT *pClient; /* The client which uses the channel. */
    VBOXHOSTCHPROVIDER *pProvider; /* NULL if the provider was unregistered. */
    void *pvChannel;               /* Provider's context of the channel. */
    uint32_t u32Handle;        /* handle assigned to the channel by the service. */
} VBOXHOSTCHINSTANCE;

struct VBOXHOSTCHCTX
{
    bool fInitialized;

    RTLISTANCHOR listProviders;
};

/* The channel callbacks context. The provider passes the pointer as a callback parameter.
 * Created for the provider and deleted when the provider says so.
 */
typedef struct VBOXHOSTCHCALLBACKCTX
{
    RTLISTNODE nodeClient;     /* In the client, for cleanup when a client disconnects. */

    VBOXHOSTCHCLIENT *pClient; /* The client which uses the channel, NULL when the client does not exist. */
} VBOXHOSTCHCALLBACKCTX;

/* Only one service instance is supported. */
static VBOXHOSTCHCTX g_ctx = { false };

static VBOXHOSTCHANNELCALLBACKS g_callbacks =
{
    HostChannelCallbackEvent,
    HostChannelCallbackDeleted
};


/*
 * Provider management.
 */

static void vhcProviderDestroy(VBOXHOSTCHPROVIDER *pProvider)
{
    RTStrFree(pProvider->pszName);
}

static int32_t vhcProviderAddRef(VBOXHOSTCHPROVIDER *pProvider)
{
    return ASMAtomicIncS32(&pProvider->cRefs);
}

static void vhcProviderRelease(VBOXHOSTCHPROVIDER *pProvider)
{
    int32_t c = ASMAtomicDecS32(&pProvider->cRefs);
    Assert(c >= 0);
    if (c == 0)
    {
        vhcProviderDestroy(pProvider);
        RTMemFree(pProvider);
    }
}

static VBOXHOSTCHPROVIDER *vhcProviderFind(VBOXHOSTCHCTX *pCtx, const char *pszName)
{
    VBOXHOSTCHPROVIDER *pProvider = NULL;

    int rc = vboxHostChannelLock();

    if (RT_SUCCESS(rc))
    {
        VBOXHOSTCHPROVIDER *pIter;
        RTListForEach(&pCtx->listProviders, pIter, VBOXHOSTCHPROVIDER, nodeContext)
        {
            if (RTStrCmp(pIter->pszName, pszName) == 0)
            {
                pProvider = pIter;

                vhcProviderAddRef(pProvider);

                break;
            }
        }

        vboxHostChannelUnlock();
    }

    return pProvider;
}

static int vhcProviderRegister(VBOXHOSTCHCTX *pCtx, VBOXHOSTCHPROVIDER *pProvider)
{
    int rc = vboxHostChannelLock();

    if (RT_SUCCESS(rc))
    {
        /** @todo check a duplicate. */

        RTListAppend(&pCtx->listProviders, &pProvider->nodeContext);

        vboxHostChannelUnlock();
    }

    if (RT_FAILURE(rc))
    {
        vhcProviderRelease(pProvider);
    }

    return rc;
}

static int vhcProviderUnregister(VBOXHOSTCHPROVIDER *pProvider)
{
    int rc = vboxHostChannelLock();

    if (RT_SUCCESS(rc))
    {
        /** @todo check that the provider is in the list. */
        /** @todo mark the provider as invalid in each instance. also detach channels? */

        RTListNodeRemove(&pProvider->nodeContext);

        vboxHostChannelUnlock();

        vhcProviderRelease(pProvider);
    }

    return rc;
}


/*
 * Select an unique handle for the new channel.
 * Works under the lock.
 */
static int vhcHandleCreate(VBOXHOSTCHCLIENT *pClient, uint32_t *pu32Handle)
{
    bool fOver = false;

    for(;;)
    {
        uint32_t u32Handle = ASMAtomicIncU32(&pClient->u32HandleSrc);

        if (u32Handle == 0)
        {
            if (fOver)
            {
                return VERR_NOT_SUPPORTED;
            }

            fOver = true;
            continue;
        }

        VBOXHOSTCHINSTANCE *pDuplicate = NULL;
        VBOXHOSTCHINSTANCE *pIter;
        RTListForEach(&pClient->listChannels, pIter, VBOXHOSTCHINSTANCE, nodeClient)
        {
            if (pIter->u32Handle == u32Handle)
            {
                pDuplicate = pIter;
                break;
            }
        }

        if (pDuplicate == NULL)
        {
            *pu32Handle = u32Handle;
            break;
        }
    }

    return VINF_SUCCESS;
}


/*
 * Channel instance management.
 */

static void vhcInstanceDestroy(VBOXHOSTCHINSTANCE *pInstance)
{
    RT_NOREF1(pInstance);
    HOSTCHLOG(("HostChannel: destroy %p\n", pInstance));
}

static int32_t vhcInstanceAddRef(VBOXHOSTCHINSTANCE *pInstance)
{
    HOSTCHLOG(("INST: %p %d addref\n", pInstance, pInstance->cRefs));
    return ASMAtomicIncS32(&pInstance->cRefs);
}

static void vhcInstanceRelease(VBOXHOSTCHINSTANCE *pInstance)
{
    int32_t c = ASMAtomicDecS32(&pInstance->cRefs);
    HOSTCHLOG(("INST: %p %d release\n", pInstance, pInstance->cRefs));
    Assert(c >= 0);
    if (c == 0)
    {
        vhcInstanceDestroy(pInstance);
        RTMemFree(pInstance);
    }
}

static int vhcInstanceCreate(VBOXHOSTCHCLIENT *pClient, VBOXHOSTCHINSTANCE **ppInstance)
{
    int rc = VINF_SUCCESS;

    VBOXHOSTCHINSTANCE *pInstance = (VBOXHOSTCHINSTANCE *)RTMemAllocZ(sizeof(VBOXHOSTCHINSTANCE));

    if (pInstance)
    {
        rc = vboxHostChannelLock();

        if (RT_SUCCESS(rc))
        {
            rc = vhcHandleCreate(pClient, &pInstance->u32Handle);

            if (RT_SUCCESS(rc))
            {
                /* Used by the client, that is in the list of channels. */
                vhcInstanceAddRef(pInstance);
                /* Add to the list of created channel instances. It is inactive while pClient is 0. */
                RTListAppend(&pClient->listChannels, &pInstance->nodeClient);

                /* Return to the caller. */
                vhcInstanceAddRef(pInstance);
                *ppInstance = pInstance;
            }

            vboxHostChannelUnlock();
        }

        if (RT_FAILURE(rc))
        {
            RTMemFree(pInstance);
        }
    }
    else
    {
        rc = VERR_NO_MEMORY;
    }

    return rc;
}

static VBOXHOSTCHINSTANCE *vhcInstanceFind(VBOXHOSTCHCLIENT *pClient, uint32_t u32Handle)
{
    VBOXHOSTCHINSTANCE *pInstance = NULL;

    int rc = vboxHostChannelLock();

    if (RT_SUCCESS(rc))
    {
        VBOXHOSTCHINSTANCE *pIter;
        RTListForEach(&pClient->listChannels, pIter, VBOXHOSTCHINSTANCE, nodeClient)
        {
            if (   pIter->pClient
                && pIter->u32Handle == u32Handle)
            {
                pInstance = pIter;

                vhcInstanceAddRef(pInstance);

                break;
            }
        }

        vboxHostChannelUnlock();
    }

    return pInstance;
}

static VBOXHOSTCHINSTANCE *vhcInstanceFindByChannelPtr(VBOXHOSTCHCLIENT *pClient, void *pvChannel)
{
    VBOXHOSTCHINSTANCE *pInstance = NULL;

    if (pvChannel == NULL)
    {
        return NULL;
    }

    int rc = vboxHostChannelLock();

    if (RT_SUCCESS(rc))
    {
        VBOXHOSTCHINSTANCE *pIter;
        RTListForEach(&pClient->listChannels, pIter, VBOXHOSTCHINSTANCE, nodeClient)
        {
            if (   pIter->pClient
                && pIter->pvChannel == pvChannel)
            {
                pInstance = pIter;

                vhcInstanceAddRef(pInstance);

                break;
            }
        }

        vboxHostChannelUnlock();
    }

    return pInstance;
}

static void vhcInstanceDetach(VBOXHOSTCHINSTANCE *pInstance)
{
    HOSTCHLOG(("HostChannel: detach %p\n", pInstance));

    if (pInstance->pProvider)
    {
        pInstance->pProvider->iface.HostChannelDetach(pInstance->pvChannel);
        RTListNodeRemove(&pInstance->nodeProvider);
        vhcProviderRelease(pInstance->pProvider);
        pInstance->pProvider = NULL;
        vhcInstanceRelease(pInstance); /* Not in the provider's list anymore. */
    }

    int rc = vboxHostChannelLock();

    if (RT_SUCCESS(rc))
    {
        RTListNodeRemove(&pInstance->nodeClient);

        vboxHostChannelUnlock();

        vhcInstanceRelease(pInstance); /* Not used by the client anymore. */
    }
}

/*
 * Channel callback contexts.
 */
static int vhcCallbackCtxCreate(VBOXHOSTCHCLIENT *pClient, VBOXHOSTCHCALLBACKCTX **ppCallbackCtx)
{
    int rc = VINF_SUCCESS;

    VBOXHOSTCHCALLBACKCTX *pCallbackCtx = (VBOXHOSTCHCALLBACKCTX *)RTMemAllocZ(sizeof(VBOXHOSTCHCALLBACKCTX));

    if (pCallbackCtx != NULL)
    {
        /* The callback context is accessed by the providers threads. */
        rc = vboxHostChannelLock();
        if (RT_SUCCESS(rc))
        {
            RTListAppend(&pClient->listContexts, &pCallbackCtx->nodeClient);
            pCallbackCtx->pClient = pClient;

            vboxHostChannelUnlock();
        }
        else
        {
            RTMemFree(pCallbackCtx);
        }
    }
    else
    {
        rc = VERR_NO_MEMORY;
    }

    if (RT_SUCCESS(rc))
    {
        *ppCallbackCtx = pCallbackCtx;
    }

    return rc;
}

static int vhcCallbackCtxDelete(VBOXHOSTCHCALLBACKCTX *pCallbackCtx)
{
    int rc = vboxHostChannelLock();
    if (RT_SUCCESS(rc))
    {
        VBOXHOSTCHCLIENT *pClient = pCallbackCtx->pClient;

        if (pClient != NULL)
        {
            /* The callback is associated with a client.
             * Check that the callback is in the list and remove it from the list.
             */
            bool fFound = false;

            VBOXHOSTCHCALLBACKCTX *pIter;
            RTListForEach(&pClient->listContexts, pIter, VBOXHOSTCHCALLBACKCTX, nodeClient)
            {
                if (pIter == pCallbackCtx)
                {
                    fFound = true;
                    break;
                }
            }

            if (fFound)
            {
                RTListNodeRemove(&pCallbackCtx->nodeClient);
            }
            else
            {
                AssertFailed();
                rc = VERR_INVALID_PARAMETER;
            }
        }
        else
        {
            /* It is not in the clients anymore. May be the client has been disconnected.
             * Just free the memory.
             */
        }

        vboxHostChannelUnlock();
    }

    if (RT_SUCCESS(rc))
    {
        RTMemFree(pCallbackCtx);
    }

    return rc;
}

/*
 * Host channel service functions.
 */

int vboxHostChannelInit(void)
{
    VBOXHOSTCHCTX *pCtx = &g_ctx;

    if (pCtx->fInitialized)
    {
        return VERR_NOT_SUPPORTED;
    }

    pCtx->fInitialized = true;
    RTListInit(&pCtx->listProviders);

    return VINF_SUCCESS;
}

void vboxHostChannelDestroy(void)
{
    VBOXHOSTCHCTX *pCtx = &g_ctx;

    VBOXHOSTCHPROVIDER *pIter;
    VBOXHOSTCHPROVIDER *pIterNext;
    RTListForEachSafe(&pCtx->listProviders, pIter, pIterNext, VBOXHOSTCHPROVIDER, nodeContext)
    {
        vhcProviderUnregister(pIter);
    }
    pCtx->fInitialized = false;
}

int vboxHostChannelClientConnect(VBOXHOSTCHCLIENT *pClient)
{
    /* A guest client is connecting to the service.
     * Later the client will use Attach calls to connect to channel providers.
     * pClient is already zeroed.
     */
    pClient->pCtx = &g_ctx;

    RTListInit(&pClient->listChannels);
    RTListInit(&pClient->listEvents);
    RTListInit(&pClient->listContexts);

    return VINF_SUCCESS;
}

void vboxHostChannelClientDisconnect(VBOXHOSTCHCLIENT *pClient)
{
    /* Clear the list of contexts and prevent acceess to the client. */
    int rc = vboxHostChannelLock();
    if (RT_SUCCESS(rc))
    {
        VBOXHOSTCHCALLBACKCTX *pIter;
        VBOXHOSTCHCALLBACKCTX *pNext;
        RTListForEachSafe(&pClient->listContexts, pIter, pNext, VBOXHOSTCHCALLBACKCTX, nodeClient)
        {
            pIter->pClient = NULL;
            RTListNodeRemove(&pIter->nodeClient);
        }

        vboxHostChannelUnlock();
    }

    /* If there are attached channels, detach them. */
    VBOXHOSTCHINSTANCE *pIter;
    VBOXHOSTCHINSTANCE *pIterNext;
    RTListForEachSafe(&pClient->listChannels, pIter, pIterNext, VBOXHOSTCHINSTANCE, nodeClient)
    {
        vhcInstanceDetach(pIter);
    }
}

int vboxHostChannelAttach(VBOXHOSTCHCLIENT *pClient,
                          uint32_t *pu32Handle,
                          const char *pszName,
                          uint32_t u32Flags)
{
    int rc = VINF_SUCCESS;

    HOSTCHLOG(("HostChannel: Attach: (%d) [%s] 0x%08X\n", pClient->u32ClientID, pszName, u32Flags));

    /* Look if there is a provider. */
    VBOXHOSTCHPROVIDER *pProvider = vhcProviderFind(pClient->pCtx, pszName);

    if (pProvider)
    {
        VBOXHOSTCHINSTANCE *pInstance = NULL;

        rc = vhcInstanceCreate(pClient, &pInstance);

        if (RT_SUCCESS(rc))
        {
            VBOXHOSTCHCALLBACKCTX *pCallbackCtx = NULL;
            rc = vhcCallbackCtxCreate(pClient, &pCallbackCtx);

            if (RT_SUCCESS(rc))
            {
                void *pvChannel = NULL;
                rc = pProvider->iface.HostChannelAttach(pProvider->iface.pvProvider,
                                                        &pvChannel,
                                                        u32Flags,
                                                        &g_callbacks, pCallbackCtx);

                if (RT_SUCCESS(rc))
                {
                    vhcProviderAddRef(pProvider);
                    pInstance->pProvider = pProvider;

                    pInstance->pClient = pClient;
                    pInstance->pvChannel = pvChannel;

                    /* It is already in the channels list of the client. */

                    vhcInstanceAddRef(pInstance); /* Referenced by the list of provider's channels. */
                    RTListAppend(&pProvider->listChannels, &pInstance->nodeProvider);

                    *pu32Handle = pInstance->u32Handle;

                    HOSTCHLOG(("HostChannel: Attach: (%d) handle %d\n", pClient->u32ClientID, pInstance->u32Handle));
                }

                if (RT_FAILURE(rc))
                {
                    vhcCallbackCtxDelete(pCallbackCtx);
                }
            }

            if (RT_FAILURE(rc))
            {
                vhcInstanceDetach(pInstance);
            }

            vhcInstanceRelease(pInstance);
        }

        vhcProviderRelease(pProvider);
    }
    else
    {
        rc = VERR_NOT_SUPPORTED;
    }

    return rc;
}

int vboxHostChannelDetach(VBOXHOSTCHCLIENT *pClient,
                          uint32_t u32Handle)
{
    HOSTCHLOG(("HostChannel: Detach: (%d) handle %d\n", pClient->u32ClientID, u32Handle));

    int rc = VINF_SUCCESS;

    VBOXHOSTCHINSTANCE *pInstance = vhcInstanceFind(pClient, u32Handle);

    if (pInstance)
    {
        vhcInstanceDetach(pInstance);

        vhcInstanceRelease(pInstance);
    }
    else
    {
        rc = VERR_NOT_SUPPORTED;
    }

    return rc;
}

int vboxHostChannelSend(VBOXHOSTCHCLIENT *pClient,
                        uint32_t u32Handle,
                        const void *pvData,
                        uint32_t cbData)
{
    HOSTCHLOG(("HostChannel: Send: (%d) handle %d, %d bytes\n", pClient->u32ClientID, u32Handle, cbData));

    int rc = VINF_SUCCESS;

    VBOXHOSTCHINSTANCE *pInstance = vhcInstanceFind(pClient, u32Handle);

    if (pInstance)
    {
        if (pInstance->pProvider)
        {
            pInstance->pProvider->iface.HostChannelSend(pInstance->pvChannel, pvData, cbData);
        }

        vhcInstanceRelease(pInstance);
    }
    else
    {
        rc = VERR_NOT_SUPPORTED;
    }

    return rc;
}

int vboxHostChannelRecv(VBOXHOSTCHCLIENT *pClient,
                        uint32_t u32Handle,
                        void *pvData,
                        uint32_t cbData,
                        uint32_t *pu32SizeReceived,
                        uint32_t *pu32SizeRemaining)
{
    HOSTCHLOG(("HostChannel: Recv: (%d) handle %d, cbData %d\n", pClient->u32ClientID, u32Handle, cbData));

    int rc = VINF_SUCCESS;

    VBOXHOSTCHINSTANCE *pInstance = vhcInstanceFind(pClient, u32Handle);

    if (pInstance)
    {
        if (pInstance->pProvider)
        {
            rc = pInstance->pProvider->iface.HostChannelRecv(pInstance->pvChannel, pvData, cbData,
                                                             pu32SizeReceived, pu32SizeRemaining);

            HOSTCHLOG(("HostChannel: Recv: (%d) handle %d, rc %Rrc, cbData %d, recv %d, rem %d\n",
                       pClient->u32ClientID, u32Handle, rc, cbData, *pu32SizeReceived, *pu32SizeRemaining));
        }

        vhcInstanceRelease(pInstance);
    }
    else
    {
        rc = VERR_NOT_SUPPORTED;
    }

    return rc;
}

int vboxHostChannelControl(VBOXHOSTCHCLIENT *pClient,
                           uint32_t u32Handle,
                           uint32_t u32Code,
                           void *pvParm,
                           uint32_t cbParm,
                           void *pvData,
                           uint32_t cbData,
                           uint32_t *pu32SizeDataReturned)
{
    HOSTCHLOG(("HostChannel: Control: (%d) handle %d, cbData %d\n", pClient->u32ClientID, u32Handle, cbData));

    int rc = VINF_SUCCESS;

    VBOXHOSTCHINSTANCE *pInstance = vhcInstanceFind(pClient, u32Handle);

    if (pInstance)
    {
        if (pInstance->pProvider)
        {
            pInstance->pProvider->iface.HostChannelControl(pInstance->pvChannel, u32Code,
                                                           pvParm, cbParm,
                                                           pvData, cbData, pu32SizeDataReturned);
        }

        vhcInstanceRelease(pInstance);
    }
    else
    {
        rc = VERR_NOT_SUPPORTED;
    }

    return rc;
}

typedef struct VBOXHOSTCHANNELEVENT
{
    RTLISTNODE NodeEvent;

    uint32_t u32ChannelHandle;

    uint32_t u32Id;
    void *pvEvent;
    uint32_t cbEvent;
} VBOXHOSTCHANNELEVENT;

int vboxHostChannelEventWait(VBOXHOSTCHCLIENT *pClient,
                             bool *pfEvent,
                             VBOXHGCMCALLHANDLE callHandle,
                             VBOXHGCMSVCPARM *paParms)
{
    int rc = vboxHostChannelLock();
    if (RT_FAILURE(rc))
    {
        return rc;
    }

    if (pClient->fAsync)
    {
        /* If there is a wait request already, cancel it. */
        vboxHostChannelReportAsync(pClient, 0, VBOX_HOST_CHANNEL_EVENT_CANCELLED, NULL, 0);
        pClient->fAsync = false;
    }

    /* Check if there is something in the client's event queue. */
    VBOXHOSTCHANNELEVENT *pEvent = RTListGetFirst(&pClient->listEvents, VBOXHOSTCHANNELEVENT, NodeEvent);

    HOSTCHLOG(("HostChannel: QueryEvent: (%d), event %p\n", pClient->u32ClientID, pEvent));

    if (pEvent)
    {
        /* Report the event. */
        RTListNodeRemove(&pEvent->NodeEvent);

        HOSTCHLOG(("HostChannel: QueryEvent: (%d), cbEvent %d\n",
                   pClient->u32ClientID, pEvent->cbEvent));

        vboxHostChannelEventParmsSet(paParms, pEvent->u32ChannelHandle,
                                     pEvent->u32Id, pEvent->pvEvent, pEvent->cbEvent);

        *pfEvent = true;

        RTMemFree(pEvent);
    }
    else
    {
        /* No event available at the time. Process asynchronously. */
        pClient->fAsync           = true;
        pClient->async.callHandle = callHandle;
        pClient->async.paParms    = paParms;

        /* Tell the caller that there is no event. */
        *pfEvent = false;
    }

    vboxHostChannelUnlock();
    return rc;
}

int vboxHostChannelEventCancel(VBOXHOSTCHCLIENT *pClient)
{
    int rc = vboxHostChannelLock();

    if (RT_SUCCESS(rc))
    {
        if (pClient->fAsync)
        {
            /* If there is a wait request alredy, cancel it. */
            vboxHostChannelReportAsync(pClient, 0, VBOX_HOST_CHANNEL_EVENT_CANCELLED, NULL, 0);

            pClient->fAsync = false;
        }

        vboxHostChannelUnlock();
    }

    return rc;
}

/* @thread provider */
static DECLCALLBACK(void) HostChannelCallbackEvent(void *pvCallbacks, void *pvChannel,
                                                   uint32_t u32Id, const void *pvEvent, uint32_t cbEvent)
{
    VBOXHOSTCHCALLBACKCTX *pCallbackCtx = (VBOXHOSTCHCALLBACKCTX *)pvCallbacks;

    int rc = vboxHostChannelLock();
    if (RT_FAILURE(rc))
    {
        return;
    }

    /* Check that the structure is still associated with a client.
     * The client can disconnect and will be invalid.
     */
    VBOXHOSTCHCLIENT *pClient = pCallbackCtx->pClient;

    if (pClient == NULL)
    {
        vboxHostChannelUnlock();

        HOSTCHLOG(("HostChannel: CallbackEvent[%p]: client gone.\n", pvEvent));

        /* The client does not exist anymore, skip the event. */
        return;
    }

    bool fFound = false;

    VBOXHOSTCHCALLBACKCTX *pIter;
    RTListForEach(&pClient->listContexts, pIter, VBOXHOSTCHCALLBACKCTX, nodeClient)
    {
        if (pIter == pCallbackCtx)
        {
            fFound = true;
            break;
        }
    }

    if (!fFound)
    {
        AssertFailed();

        vboxHostChannelUnlock();

        HOSTCHLOG(("HostChannel: CallbackEvent[%p]: client does not have the context.\n", pvEvent));

        /* The context is not in the list of contexts. Skip the event. */
        return;
    }

    VBOXHOSTCHINSTANCE *pInstance = vhcInstanceFindByChannelPtr(pClient, pvChannel);

    HOSTCHLOG(("HostChannel: CallbackEvent[%p]: (%d) instance %p\n",
               pCallbackCtx, pClient->u32ClientID, pInstance));

    if (!pInstance)
    {
        /* Instance was already detached. Skip the event. */
        vboxHostChannelUnlock();

        return;
    }

    uint32_t u32ChannelHandle = pInstance->u32Handle;

    HOSTCHLOG(("HostChannel: CallbackEvent: (%d) handle %d, async %d, cbEvent %d\n",
               pClient->u32ClientID, u32ChannelHandle, pClient->fAsync, cbEvent));

    /* Check whether the event is waited. */
    if (pClient->fAsync)
    {
        /* Report the event. */
        vboxHostChannelReportAsync(pClient, u32ChannelHandle, u32Id, pvEvent, cbEvent);

        pClient->fAsync = false;
    }
    else
    {
        /* Put it to the queue. */
        VBOXHOSTCHANNELEVENT *pEvent = (VBOXHOSTCHANNELEVENT *)RTMemAlloc(sizeof(VBOXHOSTCHANNELEVENT) + cbEvent);

        if (pEvent)
        {
            pEvent->u32ChannelHandle = u32ChannelHandle;
            pEvent->u32Id = u32Id;

            if (cbEvent)
            {
                pEvent->pvEvent = &pEvent[1];
                memcpy(pEvent->pvEvent, pvEvent, cbEvent);
            }
            else
            {
                pEvent->pvEvent = NULL;
            }

            pEvent->cbEvent = cbEvent;

            RTListAppend(&pClient->listEvents, &pEvent->NodeEvent);
        }
    }

    vboxHostChannelUnlock();

    vhcInstanceRelease(pInstance);
}

/* @thread provider */
static DECLCALLBACK(void) HostChannelCallbackDeleted(void *pvCallbacks, void *pvChannel)
{
    RT_NOREF1(pvChannel);
    vhcCallbackCtxDelete((VBOXHOSTCHCALLBACKCTX *)pvCallbacks);
}

int vboxHostChannelQuery(VBOXHOSTCHCLIENT *pClient,
                         const char *pszName,
                         uint32_t u32Code,
                         void *pvParm,
                         uint32_t cbParm,
                         void *pvData,
                         uint32_t cbData,
                         uint32_t *pu32SizeDataReturned)
{
    HOSTCHLOG(("HostChannel: Query: (%d) name [%s], cbData %d\n", pClient->u32ClientID, pszName, cbData));

    int rc = VINF_SUCCESS;

    /* Look if there is a provider. */
    VBOXHOSTCHPROVIDER *pProvider = vhcProviderFind(pClient->pCtx, pszName);

    if (pProvider)
    {
        pProvider->iface.HostChannelControl(NULL, u32Code,
                                            pvParm, cbParm,
                                            pvData, cbData, pu32SizeDataReturned);

        vhcProviderRelease(pProvider);
    }
    else
    {
        rc = VERR_NOT_SUPPORTED;
    }

    return rc;
}

int vboxHostChannelRegister(const char *pszName,
                            const VBOXHOSTCHANNELINTERFACE *pInterface,
                            uint32_t cbInterface)
{
    RT_NOREF1(cbInterface);
    int rc = VINF_SUCCESS;

    VBOXHOSTCHCTX *pCtx = &g_ctx;

    VBOXHOSTCHPROVIDER *pProvider = (VBOXHOSTCHPROVIDER *)RTMemAllocZ(sizeof(VBOXHOSTCHPROVIDER));

    if (pProvider)
    {
        pProvider->pCtx = pCtx;
        pProvider->iface = *pInterface;

        RTListInit(&pProvider->listChannels);

        pProvider->pszName = RTStrDup(pszName);
        if (pProvider->pszName)
        {
            vhcProviderAddRef(pProvider);
            rc = vhcProviderRegister(pCtx, pProvider);
        }
        else
        {
            RTMemFree(pProvider);
            rc = VERR_NO_MEMORY;
        }
    }
    else
    {
        rc = VERR_NO_MEMORY;
    }

    return rc;
}

int vboxHostChannelUnregister(const char *pszName)
{
    int rc = VINF_SUCCESS;

    VBOXHOSTCHCTX *pCtx = &g_ctx;

    VBOXHOSTCHPROVIDER *pProvider = vhcProviderFind(pCtx, pszName);

    if (pProvider)
    {
        rc = vhcProviderUnregister(pProvider);
        vhcProviderRelease(pProvider);
    }

    return rc;
}
