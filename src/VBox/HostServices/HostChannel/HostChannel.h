/* @file
 *
 * Host Channel
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

#ifndef VBOX_INCLUDED_SRC_HostChannel_HostChannel_h
#define VBOX_INCLUDED_SRC_HostChannel_HostChannel_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/list.h>

#define LOG_GROUP LOG_GROUP_HGCM
#include <VBox/log.h>
#include <VBox/HostServices/VBoxHostChannel.h>

#define HOSTCHLOG Log

#ifdef DEBUG_sunlover
# undef HOSTCHLOG
# define HOSTCHLOG LogRel
#endif /* DEBUG_sunlover */

struct VBOXHOSTCHCTX;
typedef struct VBOXHOSTCHCTX VBOXHOSTCHCTX;

typedef struct VBOXHOSTCHCLIENT
{
    RTLISTNODE nodeClient;

    VBOXHOSTCHCTX *pCtx;

    uint32_t u32ClientID;

    RTLISTANCHOR listChannels;
    uint32_t volatile u32HandleSrc;

    RTLISTANCHOR listContexts; /* Callback contexts. */

    RTLISTANCHOR listEvents;

    bool fAsync;        /* Guest is waiting for a message. */

    struct {
        VBOXHGCMCALLHANDLE callHandle;
        VBOXHGCMSVCPARM *paParms;
    } async;

} VBOXHOSTCHCLIENT;


/*
 * The service functions. Locking is between the service thread and the host channel provider thread.
 */
int vboxHostChannelLock(void);
void vboxHostChannelUnlock(void);

int vboxHostChannelInit(void);
void vboxHostChannelDestroy(void);

int vboxHostChannelClientConnect(VBOXHOSTCHCLIENT *pClient);
void vboxHostChannelClientDisconnect(VBOXHOSTCHCLIENT *pClient);

int vboxHostChannelAttach(VBOXHOSTCHCLIENT *pClient,
                          uint32_t *pu32Handle,
                          const char *pszName,
                          uint32_t u32Flags);
int vboxHostChannelDetach(VBOXHOSTCHCLIENT *pClient,
                          uint32_t u32Handle);

int vboxHostChannelSend(VBOXHOSTCHCLIENT *pClient,
                        uint32_t u32Handle,
                        const void *pvData,
                        uint32_t cbData);
int vboxHostChannelRecv(VBOXHOSTCHCLIENT *pClient,
                        uint32_t u32Handle,
                        void *pvData,
                        uint32_t cbData,
                        uint32_t *pu32DataReceived,
                        uint32_t *pu32DataRemaining);
int vboxHostChannelControl(VBOXHOSTCHCLIENT *pClient,
                           uint32_t u32Handle,
                           uint32_t u32Code,
                           void *pvParm,
                           uint32_t cbParm,
                           void *pvData,
                           uint32_t cbData,
                           uint32_t *pu32SizeDataReturned);

int vboxHostChannelEventWait(VBOXHOSTCHCLIENT *pClient,
                             bool *pfEvent,
                             VBOXHGCMCALLHANDLE callHandle,
                             VBOXHGCMSVCPARM *paParms);

int vboxHostChannelEventCancel(VBOXHOSTCHCLIENT *pClient);

int vboxHostChannelQuery(VBOXHOSTCHCLIENT *pClient,
                         const char *pszName,
                         uint32_t u32Code,
                         void *pvParm,
                         uint32_t cbParm,
                         void *pvData,
                         uint32_t cbData,
                         uint32_t *pu32SizeDataReturned);

int vboxHostChannelRegister(const char *pszName,
                            const VBOXHOSTCHANNELINTERFACE *pInterface,
                            uint32_t cbInterface);
int vboxHostChannelUnregister(const char *pszName);


void vboxHostChannelEventParmsSet(VBOXHGCMSVCPARM *paParms,
                                  uint32_t u32ChannelHandle,
                                  uint32_t u32Id,
                                  const void *pvEvent,
                                  uint32_t cbEvent);

void vboxHostChannelReportAsync(VBOXHOSTCHCLIENT *pClient,
                                uint32_t u32ChannelHandle,
                                uint32_t u32Id,
                                const void *pvEvent,
                                uint32_t cbEvent);

#endif /* !VBOX_INCLUDED_SRC_HostChannel_HostChannel_h */
