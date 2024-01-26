/** @file
 * IPRT Generic I/O queue API.
 */

/*
 * Copyright (C) 2019-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_ioqueue_h
#define IPRT_INCLUDED_ioqueue_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>
#include <iprt/sg.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_ioqueue  IPRT generic I/O queue API
 * @ingroup grp_rt
 *
 * This API models a generic I/O queue which can be attached to different providers
 * for different types of handles.
 *
 * @{
 */


/**
 * I/O queue request operations.
 */
typedef enum RTIOQUEUEOP
{
    /** The usual invalid option. */
    RTIOQUEUEOP_INVALID = 0,
    /** Read request. */
    RTIOQUEUEOP_READ,
    /** Write request. */
    RTIOQUEUEOP_WRITE,
    /** Synchronize (i.e. flush) request. */
    RTIOQUEUEOP_SYNC,
    /** Usual 32bit hack. */
    RTIOQUEUEOP_32BIT_HACK = 0x7fffffff
} RTIOQUEUEOP;
/** Pointer to a I/O queue operation code. */
typedef RTIOQUEUEOP *PRTIOQUEUEOP;

/** I/O queue provider (processes requests put into the I/O queue) handle. */
typedef struct RTIOQUEUEPROVINT      *RTIOQUEUEPROV;
/** I/O queue handle. */
typedef struct RTIOQUEUEINT          *RTIOQUEUE;
/** Pointer to an I/O queue handle. */
typedef RTIOQUEUE                    *PRTIOQUEUE;
/** NIL I/O queue handle value. */
#define NIL_RTIOQUEUE                ((RTIOQUEUE)0)


/**
 * I/O queue completion event.
 */
typedef struct RTIOQUEUECEVT
{
    /** The user data passed when preparing the request. */
    void                        *pvUser;
    /** The IPRT status code for this request. */
    int                         rcReq;
    /** Transferred data size if applicaple by the request. */
    size_t                      cbXfered;
} RTIOQUEUECEVT;
/** Pointer to a I/O queue completion event. */
typedef RTIOQUEUECEVT *PRTIOQUEUECEVT;
/** Pointer to a const I/O queue completion event. */
typedef const RTIOQUEUECEVT *PCRTIOQUEUECEVT;


/**
 * I/O queue provider virtual method table.
 */
typedef struct RTIOQUEUEPROVVTABLE
{
    /** The structure version (RTIOQUEUEPROVVTABLE_VERSION). */
    uint32_t                    uVersion;
    /** Provider ID. */
    const char                  *pszId;
    /** Size of provider specific data for an I/O queue instance. */
    size_t                      cbIoQueueProv;
    /** The handle type the provider is able to process. */
    RTHANDLETYPE                enmHnd;
    /** Additional flags for exposing supported features or quirks to the user. */
    uint32_t                    fFlags;

    /**
     * Returns whether the provider is supported on the calling host system.
     *
     * @returns Flag whether the provider is supported.
     */
    DECLCALLBACKMEMBER(bool, pfnIsSupported,(void));

    /**
     * Initializes the provider specific parts of the given I/O queue.
     *
     * @returns IPRT status code.
     * @param   hIoQueueProv    The I/O queue provider instance to initialize.
     * @param   fFlags          Flags for the queue.
     * @param   cSqEntries      Number of entries for the submission queue.
     * @param   cCqEntries      Number of entries for the completion queue.
     */
    DECLCALLBACKMEMBER(int, pfnQueueInit,(RTIOQUEUEPROV hIoQueueProv, uint32_t fFlags, uint32_t cSqEntries, uint32_t cCqEntries));

    /**
     * Destroys the provider specific parts of the I/O queue and frees all
     * associated resources.
     *
     * @param   hIoQueueProv    The I/O queue provider instance to destroy.
     */
    DECLCALLBACKMEMBER(void, pfnQueueDestroy,(RTIOQUEUEPROV hIoQueueProv));

    /**
     * Registers the given handle for use with the I/O queue instance.
     * The generic code already checked for the correct handle type and that the
     * handle wasn't registered already by tracking all registered handles.
     *
     * @returns IPRT status code.
     * @param   hIoQueueProv    The I/O queue provider instance.
     * @param   pHandle         The handle to register.
     */
    DECLCALLBACKMEMBER(int, pfnHandleRegister,(RTIOQUEUEPROV hIoQueueProv, PCRTHANDLE pHandle));

    /**
     * Deregisters the given handle for use with the I/O queue instance.
     * The generic code already checked for the correct handle type and that the
     * handle was registered previously.
     *
     * @returns IPRT status code.
     * @param   hIoQueueProv    The I/O queue provider instance.
     * @param   pHandle         The handle to deregister.
     */
    DECLCALLBACKMEMBER(int, pfnHandleDeregister,(RTIOQUEUEPROV hIoQueueProv, PCRTHANDLE pHandle));

    /**
     * Prepares a request for the given I/O queue.
     *
     * @returns IPRT status code.
     * @param   hIoQueueProv        The I/O queue provider instance.
     * @param   pHandle             The handle the request is for.
     * @param   enmOp               The operation to perform.
     * @param   off                 Start offset (if applicable, not all handles support/require it and will ignore it).
     * @param   pvBuf               Buffer to use for read/write operations (sync ignores this).
     * @param   cbBuf               Size of the buffer in bytes.
     * @param   fReqFlags           Additional flags for the request.
     * @param   pvUser              Opaque user data which is passed back in the completion event.
     */
    DECLCALLBACKMEMBER(int, pfnReqPrepare,(RTIOQUEUEPROV hIoQueueProv, PCRTHANDLE pHandle, RTIOQUEUEOP enmOp,
                                           uint64_t off, void *pvBuf, size_t cbBuf, uint32_t fReqFlags, void *pvUser));

    /**
     * Prepares a request for the given I/O queue.
     *
     * @returns IPRT status code.
     * @param   hIoQueueProv        The I/O queue provider instance.
     * @param   pHandle             The handle the request is for.
     * @param   enmOp               The operation to perform.
     * @param   off                 Start offset (if applicable, not all handles support/require it and will ignore it).
     * @param   pSgBuf              The S/G buufer to use for read/write operations (sync ignores this).
     * @param   cbSg                Number of bytes to transfer from the S/G buffer.
     * @param   fReqFlags           Additional flags for the request.
     * @param   pvUser              Opaque user data which is passed back in the completion event.
     */
    DECLCALLBACKMEMBER(int, pfnReqPrepareSg,(RTIOQUEUEPROV hIoQueueProv, PCRTHANDLE pHandle, RTIOQUEUEOP enmOp,
                                             uint64_t off, PCRTSGBUF pSgBuf, size_t cbSg, uint32_t fReqFlags, void *pvUser));

    /**
     * Commits all prepared requests to the consumer for processing.
     *
     * @returns IPRT status code.
     * @param   hIoQueueProv        The I/O queue provider instance.
     * @param   pcReqsCommitted     Where to store the number of requests actually committed.
     */
    DECLCALLBACKMEMBER(int, pfnCommit,(RTIOQUEUEPROV hIoQueueProv, uint32_t *pcReqsCommitted));

    /**
     * Waits for completion events from the given I/O queue.
     *
     * @returns IPRT status code.
     * @retval  VERR_IOQUEUE_EMPTY if there is nothing to wait for.
     * @param   hIoQueueProv        The I/O queue provider instance.
     * @param   paCEvt              Pointer to the array of completion event entries to fill.
     * @param   cCEvt               Size of the completion event entry array.
     * @param   cMinWait            Minimum number of completion events to wait for before returning.
     * @param   pcCEvt              Where to store the number of completion events on success.
     * @param   fFlags              Additional flags controlling the wait behavior.
     */
    DECLCALLBACKMEMBER(int, pfnEvtWait,(RTIOQUEUEPROV hIoQueueProv, PRTIOQUEUECEVT paCEvt, uint32_t cCEvt,
                                        uint32_t cMinWait, uint32_t *pcCEvt, uint32_t fFlags));

    /**
     * Wakes up the thread waiting in RTIOQUEUEPROVVTABLE::pfnEvtWait().
     *
     * @returns IPRT status code.
     * @param   hIoQueueProv        The I/O queue provider instance.
     */
    DECLCALLBACKMEMBER(int, pfnEvtWaitWakeup,(RTIOQUEUEPROV hIoQueueProv));

    /** Marks the end of the structure (RTIOQUEUEPROVVTABLE_VERSION). */
    uintptr_t               uEndMarker;
} RTIOQUEUEPROVVTABLE;
/** Pointer to an I/O queue provider vtable. */
typedef RTIOQUEUEPROVVTABLE *PRTIOQUEUEPROVVTABLE;
/** Pointer to a const I/O queue provider vtable. */
typedef RTIOQUEUEPROVVTABLE const *PCRTIOQUEUEPROVVTABLE;

/** The RTIOQUEUEPROVVTABLE structure version. */
#define RTIOQUEUEPROVVTABLE_VERSION    RT_MAKE_U32_FROM_U8(0xff,0xf,1,0)

/** @name RTIOQUEUEPROVVTABLE::fFlags
 * @{ */
/** Provider supports S/G lists. */
#define RTIOQUEUEPROVVTABLE_F_SG            RT_BIT_32(0)
/** Mask of the valid I/O stream feature flags. */
#define RTIOQUEUEPROVVTABLE_F_VALID_MASK    UINT32_C(0x00000001)
/** @}  */


/**
 * Tries to return the best I/O queue provider for the given handle type on the called
 * host system.
 *
 * @returns Pointer to the I/O queue provider handle table or NULL if no suitable
 *          provider was found for the given handle type.
 * @param   enmHnd              The handle type to look for a provider.
 */
RTDECL(PCRTIOQUEUEPROVVTABLE) RTIoQueueProviderGetBestForHndType(RTHANDLETYPE enmHnd);


/**
 * Returns the I/O queue provider with the given ID.
 *
 * @returns Pointer to the I/O queue provider handle table or NULL if no provider with
 *          the given ID was found.
 * @param   pszId               The ID to look for.
 */
RTDECL(PCRTIOQUEUEPROVVTABLE) RTIoQueueProviderGetById(const char *pszId);


/**
 * Creates a new I/O queue with the given consumer.
 *
 * @returns IPRT status code.
 * @param   phIoQueue           Where to store the handle to the I/O queue on success.
 * @param   pProvVTable         The I/O queue provider vtable which will process the requests.
 * @param   fFlags              Flags for the queue (MBZ for now).
 * @param   cSqEntries          Number of entries for the submission queue.
 * @param   cCqEntries          Number of entries for the completion queue.
 *
 * @note The number of submission and completion queue entries serve only as a hint to the
 *       provider implementation. It may decide to align the number to a smaller or greater
 *       size.
 */
RTDECL(int) RTIoQueueCreate(PRTIOQUEUE phIoQueue, PCRTIOQUEUEPROVVTABLE pProvVTable,
                            uint32_t fFlags, uint32_t cSqEntries, uint32_t cCqEntries);


/**
 * Destroys the given I/O queue.
 *
 * @returns IPRT status code.
 * @retval  VERR_IOQUEUE_BUSY if the I/O queue is still processing requests.
 * @param   hIoQueue            The I/O queue handle to destroy.
 */
RTDECL(int) RTIoQueueDestroy(RTIOQUEUE hIoQueue);


/**
 * Registers the given handle for use with the I/O queue.
 *
 * @returns IPRT status code.
 * @retval  VERR_ALREADY_EXISTS if the handle was already registered.
 * @retval  VERR_NOT_SUPPORTED if the handle type is not supported by the consumer
 *          for the given I/O queue.
 * @param   hIoQueue            The I/O queue handle.
 * @param   pHandle             The handle to register.
 */
RTDECL(int) RTIoQueueHandleRegister(RTIOQUEUE hIoQueue, PCRTHANDLE pHandle);


/**
 * Deregisters the given handle from the given I/O queue.
 *
 * @returns IPRT status code.
 * @retval  VERR_IOQUEUE_HANDLE_NOT_REGISTERED if the handle wasn't registered by a call to RTIoQueueHandleRegister().
 * @param   hIoQueue            The I/O queue handle.
 * @param   pHandle             The handle to deregister.
 */
RTDECL(int) RTIoQueueHandleDeregister(RTIOQUEUE hIoQueue, PCRTHANDLE pHandle);


/**
 * Prepares a request for the given I/O queue.
 *
 * @returns IPRT status code.
 * @retval  VERR_IOQUEUE_FULL if the I/O queue can't accept the new request because the submission queue is full.
 * @retval  VERR_IOQUEUE_HANDLE_NOT_REGISTERED if the handle wasn't registered for use with RTIoQueueHandleRegister() yet.
 * @param   hIoQueue            The I/O queue handle.
 * @param   pHandle             The handle the request is for.
 * @param   enmOp               The operation to perform.
 * @param   off                 Start offset (if applicable, not all handles support/require it and will ignore it).
 * @param   pvBuf               Buffer to use for read/write operations (sync ignores this).
 * @param   cbBuf               Size of the buffer in bytes.
 * @param   fReqFlags           Additional flags for the request.
 * @param   pvUser              Opaque user data which is passed back in the completion event.
 */
RTDECL(int) RTIoQueueRequestPrepare(RTIOQUEUE hIoQueue, PCRTHANDLE pHandle, RTIOQUEUEOP enmOp,
                                    uint64_t off, void *pvBuf, size_t cbBuf, uint32_t fReqFlags,
                                    void *pvUser);


/**
 * Prepares a request for the given I/O queue - S/G buffer variant.
 *
 * @returns IPRT status code.
 * @retval  VERR_IOQUEUE_FULL if the I/O queue can't accept the new request because the submission queue is full.
 * @retval  VERR_IOQUEUE_HANDLE_NOT_REGISTERED if the handle wasn't registered for use with RTIoQueueHandleRegister() yet.
 * @param   hIoQueue            The I/O queue handle.
 * @param   pHandle             The handle the request is for.
 * @param   enmOp               The operation to perform.
 * @param   off                 Start offset (if applicable, not all handles support/require it and will ignore it).
 * @param   pSgBuf              The S/G buufer to use for read/write operations (sync ignores this).
 * @param   cbSg                Number of bytes to transfer from the S/G buffer.
 * @param   fReqFlags           Additional flags for the request.
 * @param   pvUser              Opaque user data which is passed back in the completion event.
 */
RTDECL(int) RTIoQueueRequestPrepareSg(RTIOQUEUE hIoQueue, PCRTHANDLE pHandle, RTIOQUEUEOP enmOp,
                                      uint64_t off, PCRTSGBUF pSgBuf, size_t cbSg, uint32_t fReqFlags,
                                      void *pvUser);


/**
 * Commits all prepared requests to the consumer for processing.
 *
 * @returns IPRT status code.
 * @retval  VERR_IOQUEUE_EMPTY if there is nothing to commit.
 * @param   hIoQueue            The I/O queue handle.
 */
RTDECL(int) RTIoQueueCommit(RTIOQUEUE hIoQueue);


/**
 * Waits for completion events from the given I/O queue.
 *
 * @returns IPRT status code.
 * @retval  VERR_IOQUEUE_EMPTY if there is nothing to wait for.
 * @param   hIoQueue            The I/O queue handle.
 * @param   paCEvt              Pointer to the array of completion event entries to fill.
 * @param   cCEvt               Size of the completion event entry array.
 * @param   cMinWait            Minimum number of completion events to wait for before returning.
 * @param   pcCEvt              Where to store the number of completion events on success.
 * @param   fFlags              Additional flags controlling the wait behavior.
 */
RTDECL(int) RTIoQueueEvtWait(RTIOQUEUE hIoQueue, PRTIOQUEUECEVT paCEvt, uint32_t cCEvt, uint32_t cMinWait,
                             uint32_t *pcCEvt, uint32_t fFlags);


/**
 * Wakes up the thread waiting in RTIoQueueEvtWait().
 *
 * @returns IPRT status code.
 * @param   hIoQueue            The I/O queue handle to wake up.
 */
RTDECL(int) RTIoQueueEvtWaitWakeup(RTIOQUEUE hIoQueue);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_ioqueue_h */

