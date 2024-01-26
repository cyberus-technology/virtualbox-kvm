/** @file
 * IPRT - Polling I/O Handles.
 */

/*
 * Copyright (C) 2010-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_poll_h
#define IPRT_INCLUDED_poll_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_poll      RTPoll - Polling I/O Handles
 * @ingroup grp_rt
 * @{
 */

/** @name Poll events
 * @{ */
/** Readable without blocking. */
#define RTPOLL_EVT_READ         RT_BIT_32(0)
/** Writable without blocking. */
#define RTPOLL_EVT_WRITE        RT_BIT_32(1)
/** Error condition, hangup, exception or similar. */
#define RTPOLL_EVT_ERROR        RT_BIT_32(2)
/** Mask of the valid bits. */
#define RTPOLL_EVT_VALID_MASK   UINT32_C(0x00000007)
/** @} */

/**
 * Polls on the specified poll set until an event occurs on one of the handles
 * or the timeout expires.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS if an event occurred on a handle.  Note that these
 * @retval  VERR_INVALID_HANDLE if @a hPollSet is invalid.
 * @retval  VERR_CONCURRENT_ACCESS if another thread is already accessing the set. The
 *          user is responsible for ensuring single threaded access.
 * @retval  VERR_TIMEOUT if @a cMillies ellapsed without any events.
 * @retval  VERR_DEADLOCK if @a cMillies is set to RT_INDEFINITE_WAIT and there
 *          are no valid handles in the set.
 *
 * @param   hPollSet            The set to poll on.
 * @param   cMillies            Number of milliseconds to wait.  Use
 *                              RT_INDEFINITE_WAIT to wait for ever.
 * @param   pfEvents            Where to return details about the events that
 *                              occurred.  Optional.
 * @param   pid                 Where to return the ID associated with the
 *                              handle when calling RTPollSetAdd.  Optional.
 *
 * @sa      RTPollNoResume
 *
 * @remarks The caller is responsible for ensuring
 */
RTDECL(int) RTPoll(RTPOLLSET hPollSet, RTMSINTERVAL cMillies, uint32_t *pfEvents, uint32_t *pid);

/**
 * Same as RTPoll except that it will return when interrupted.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS if an event occurred on a handle.  Note that these
 * @retval  VERR_INVALID_HANDLE if @a hPollSet is invalid.
 * @retval  VERR_CONCURRENT_ACCESS if another thread is already accessing the set. The
 *          user is responsible for ensuring single threaded access.
 * @retval  VERR_TIMEOUT if @a cMillies ellapsed without any events.
 * @retval  VERR_DEADLOCK if @a cMillies is set to RT_INDEFINITE_WAIT and there
 *          are no valid handles in the set.
 * @retval  VERR_INTERRUPTED if a signal or other asynchronous event interrupted
 *          the polling.
 *
 * @param   hPollSet            The set to poll on.
 * @param   cMillies            Number of milliseconds to wait.  Use
 *                              RT_INDEFINITE_WAIT to wait for ever.
 * @param   pfEvents            Where to return details about the events that
 *                              occurred.  Optional.
 * @param   pid                 Where to return the ID associated with the
 *                              handle when calling RTPollSetAdd.  Optional.
 */
RTDECL(int) RTPollNoResume(RTPOLLSET hPollSet, RTMSINTERVAL cMillies, uint32_t *pfEvents, uint32_t *pid);

/**
 * Creates a poll set with no members.
 *
 * @returns IPRT status code.
 * @param   phPollSet           Where to return the poll set handle.
 */
RTDECL(int)  RTPollSetCreate(PRTPOLLSET phPollSet);

/**
 * Destroys a poll set.
 *
 * @returns IPRT status code.
 * @param   hPollSet            The poll set to destroy.  NIL_POLLSET is quietly
 *                              ignored (VINF_SUCCESS).
 */
RTDECL(int)  RTPollSetDestroy(RTPOLLSET hPollSet);

/**
 * Adds a generic handle to the poll set.
 *
 * If a handle is entered more than once, it is recommended to add the one with
 * RTPOLL_EVT_ERROR first to ensure that you get the right ID back when an error
 * actually occurs.  On some hosts it is possible that polling for
 * RTPOLL_EVT_READ on a socket may cause it to return error conditions because
 * the two cannot so easily be distinguished.
 *
 * Also note that RTPOLL_EVT_ERROR may be returned by RTPoll even if not asked
 * for.
 *
 * @returns IPRT status code
 * @retval  VERR_CONCURRENT_ACCESS if another thread is already accessing the set. The
 *          user is responsible for ensuring single threaded access.
 * @retval  VERR_POLL_HANDLE_NOT_POLLABLE if the specified handle is not
 *          pollable.
 * @retval  VERR_POLL_HANDLE_ID_EXISTS if the handle ID is already in use in the
 *          set.
 *
 * @param   hPollSet            The poll set to modify.
 * @param   pHandle             The handle to add.  NIL handles are quietly
 *                              ignored.
 * @param   fEvents             Which events to poll for.
 * @param   id                  The handle ID.
 */
RTDECL(int) RTPollSetAdd(RTPOLLSET hPollSet, PCRTHANDLE pHandle, uint32_t fEvents, uint32_t id);

/**
 * Removes a generic handle from the poll set.
 *
 * @returns IPRT status code
 * @retval  VERR_INVALID_HANDLE if @a hPollSet not valid.
 * @retval  VERR_CONCURRENT_ACCESS if another thread is already accessing the set. The
 *          user is responsible for ensuring single threaded access.
 * @retval  VERR_POLL_HANDLE_ID_NOT_FOUND if @a id doesn't resolve to a valid
 *          handle.
 *
 * @param   hPollSet            The poll set to modify.
 * @param   id                  The handle ID of the handle that should be
 *                              removed.
 */
RTDECL(int) RTPollSetRemove(RTPOLLSET hPollSet, uint32_t id);


/**
 * Query a handle in the poll set by it's ID.
 *
 * @returns IPRT status code
 * @retval  VINF_SUCCESS if the handle was found.  @a *pHandle is set.
 * @retval  VERR_INVALID_HANDLE if @a hPollSet is invalid.
 * @retval  VERR_CONCURRENT_ACCESS if another thread is already accessing the set. The
 *          user is responsible for ensuring single threaded access.
 * @retval  VERR_POLL_HANDLE_ID_NOT_FOUND if there is no handle with that ID.
 *
 * @param   hPollSet            The poll set to query.
 * @param   id                  The ID of the handle.
 * @param   pHandle             Where to return the handle details.  Optional.
 */
RTDECL(int) RTPollSetQueryHandle(RTPOLLSET hPollSet, uint32_t id, PRTHANDLE pHandle);

/**
 * Gets the number of handles in the set.
 *
 * @retval  The handle count.
 * @retval  UINT32_MAX if @a hPollSet is invalid or there is concurrent access.
 *
 * @param   hPollSet            The poll set.
 */
RTDECL(uint32_t) RTPollSetGetCount(RTPOLLSET hPollSet);

/**
 * Modifies the events to poll for for the given id.
 *
 * @returns IPRT status code.
 * @retval  VERR_INVALID_HANDLE if @a hPollSet not valid.
 * @retval  VERR_CONCURRENT_ACCESS if another thread is already accessing the set. The
 *          user is responsible for ensuring single threaded access.
 * @retval  VERR_POLL_HANDLE_ID_NOT_FOUND if @a id doesn't resolve to a valid
 *          handle.
 *
 * @param   hPollSet            The poll set to modify.
 * @param   id                  The handle ID to change the events for.
 * @param   fEvents             Which events to poll for.
 */
RTDECL(int) RTPollSetEventsChange(RTPOLLSET hPollSet, uint32_t id, uint32_t fEvents);

/**
 * Adds a pipe handle to the set.
 *
 * @returns See RTPollSetAdd.
 *
 * @param   hPollSet            The poll set.
 * @param   hPipe               The pipe handle.
 * @param   fEvents             Which events to poll for.
 * @param   id                  The handle ID.
 *
 * @todo    Maybe we could figure out what to poll for depending on the kind of
 *          pipe we're dealing with.
 */
DECLINLINE(int) RTPollSetAddPipe(RTPOLLSET hPollSet, RTPIPE hPipe, uint32_t fEvents, uint32_t id)
{
    RTHANDLE Handle;
    Handle.enmType = RTHANDLETYPE_PIPE;
    Handle.u.uInt  = 0;
    Handle.u.hPipe = hPipe;
    return RTPollSetAdd(hPollSet, &Handle, fEvents, id);
}

/**
 * Adds a socket handle to the set.
 *
 * @returns See RTPollSetAdd.
 *
 * @param   hPollSet            The poll set.
 * @param   hSocket             The socket handle.
 * @param   fEvents             Which events to poll for.
 * @param   id                  The handle ID.
 */
DECLINLINE(int) RTPollSetAddSocket(RTPOLLSET hPollSet, RTSOCKET hSocket, uint32_t fEvents, uint32_t id)
{
    RTHANDLE Handle;
    Handle.enmType   = RTHANDLETYPE_SOCKET;
    Handle.u.uInt    = 0;
    Handle.u.hSocket = hSocket;
    return RTPollSetAdd(hPollSet, &Handle, fEvents, id);
}

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_poll_h */

