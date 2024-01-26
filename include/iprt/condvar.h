/** @file
 * IPRT - Condition Variable.
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

#ifndef IPRT_INCLUDED_condvar_h
#define IPRT_INCLUDED_condvar_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#if defined(RT_LOCK_STRICT_ORDER) && defined(IN_RING3)
# include <iprt/lockvalidator.h>
#endif


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_condvar    RTCondVar - Condition Variable
 *
 * Condition variables combines mutex semaphore or critical sections with event
 * semaphores.  See @ref grp_rt_sems_mutex, @ref grp_rt_critsect,
 * @ref grp_rt_sems_event and @ref grp_rt_sems_event_multi.
 *
 * @ingroup grp_rt
 * @{
 */


/**
 * Create a condition variable.
 *
 * @returns iprt status code.
 * @param   phCondVar           Where to store the handle to the newly created
 *                              condition variable.
 */
RTDECL(int)  RTCondVarCreate(PRTCONDVAR phCondVar);

/**
 * Create a condition variable.
 *
 * @returns iprt status code.
 * @param   phCondVar           Where to store the handle to the newly created
 *                              condition variable.
 * @param   fFlags              Flags, any combination of the
 *                              RTCONDVAR_FLAGS_XXX \#defines.
 * @param   hClass              The class (no reference consumed).  Since we
 *                              don't do order checks on condition variables,
 *                              the use of the class is limited to controlling
 *                              the timeout threshold for deadlock detection.
 * @param   pszNameFmt          Name format string for the lock validator,
 *                              optional (NULL).  Max length is 32 bytes.
 * @param   ...                 Format string arguments.
 */
RTDECL(int)  RTCondVarCreateEx(PRTCONDVAR phCondVar, uint32_t fFlags, RTLOCKVALCLASS hClass,
                               const char *pszNameFmt, ...) RT_IPRT_FORMAT_ATTR(4, 5);

/** @name RTCondVarCreateEx flags
 * @{ */
/** Disables lock validation. */
#define RTCONDVAR_FLAGS_NO_LOCK_VAL     UINT32_C(0x00000001)
/** @} */

/**
 * Destroy a condition variable.
 *
 * @returns iprt status code.
 * @param   hCondVar            Handle of the condition variable.  NIL_RTCONDVAR
 *                              is quietly ignored (VINF_SUCCESS).
 */
RTDECL(int)  RTCondVarDestroy(RTCONDVAR hCondVar);

/**
 * Signal the condition variable, waking up exactly one thread.
 *
 * It is recommended that the caller holds the associated lock, but this is not
 * strictly speaking necessary.
 *
 * If no threads are waiting on the condition variable, the call will have no
 * effect on the variable.
 *
 * @returns iprt status code.
 * @param   hCondVar            The condition variable to signal.
 */
RTDECL(int)  RTCondVarSignal(RTCONDVAR hCondVar);

/**
 * Signal the condition variable, waking up all blocked threads.
 *
 * It is recommended that the caller holds the associated lock, but this is not
 * strictly speaking necessary.
 *
 * If no threads are waiting on the condition variable, the call will have no
 * effect on the variable.
 *
 * @returns iprt status code.
 * @param   hCondVar            The condition variable to broadcast.
 */
RTDECL(int)  RTCondVarBroadcast(RTCONDVAR hCondVar);

/**
 * Wait for the condition variable to be signaled, resume on interruption.
 *
 * This function will resume if the wait is interrupted by an async system event
 * (like a unix signal) or similar.
 *
 * @returns iprt status code.
 *          Will not return VERR_INTERRUPTED.
 * @param   hCondVar            The condition variable to wait on.
 * @param   hMtx                The mutex to leave during the wait and which
 *                              will be re-enter before returning.
 * @param   cMillies            Number of milliseconds to wait.  Use
 *                              RT_INDEFINITE_WAIT to wait forever.
 */
RTDECL(int)  RTCondVarMutexWait(RTCONDVAR hCondVar, RTSEMMUTEX hMtx, RTMSINTERVAL cMillies);

/**
 * Wait for the condition variable to be signaled, return on interruption.
 *
 * This function will not resume the wait if interrupted.
 *
 * @returns iprt status code.
 * @param   hCondVar            The condition variable to wait on.
 * @param   hMtx                The mutex to leave during the wait and which
 *                              will be re-enter before returning.
 * @param   cMillies            Number of milliseconds to wait.  Use
 *                              RT_INDEFINITE_WAIT to wait forever.
 */
RTDECL(int)  RTCondVarMutexWaitNoResume(RTCONDVAR hCondVar, RTSEMMUTEX hMtx, RTMSINTERVAL cMillies);

/**
 * Wait for the condition variable to be signaled, resume on interruption.
 *
 * This function will resume if the wait is interrupted by an async system event
 * (like a unix signal) or similar.
 *
 * @returns iprt status code.
 *          Will not return VERR_INTERRUPTED.
 * @param   hCondVar            The condition variable to wait on.
 * @param   hRWSem              The read/write semaphore to write-leave during
 *                              the wait and which will be re-enter in write
 *                              mode before returning.
 * @param   cMillies            Number of milliseconds to wait.  Use
 *                              RT_INDEFINITE_WAIT to wait forever.
 */
RTDECL(int)  RTCondVarRWWriteWait(RTCONDVAR hCondVar, RTSEMRW hRWSem, RTMSINTERVAL cMillies);

/**
 * Wait for the condition variable to be signaled, return on interruption.
 *
 * This function will not resume the wait if interrupted.
 *
 * @returns iprt status code.
 * @param   hCondVar            The condition variable to wait on.
 * @param   hRWSem              The read/write semaphore to write-leave during
 *                              the wait and which will be re-enter in write
 *                              mode before returning.
 * @param   cMillies            Number of milliseconds to wait.  Use
 *                              RT_INDEFINITE_WAIT to wait forever.
 */
RTDECL(int)  RTCondVarRWWriteWaitNoResume(RTCONDVAR hCondVar, RTSEMRW hRWSem, RTMSINTERVAL cMillies);

/**
 * Wait for the condition variable to be signaled, resume on interruption.
 *
 * This function will resume if the wait is interrupted by an async system event
 * (like a unix signal) or similar.
 *
 * @returns iprt status code.
 *          Will not return VERR_INTERRUPTED.
 * @param   hCondVar            The condition variable to wait on.
 * @param   hRWSem              The read/write semaphore to read-leave during
 *                              the wait and which will be re-enter in read mode
 *                              before returning.
 * @param   cMillies            Number of milliseconds to wait.  Use
 *                              RT_INDEFINITE_WAIT to wait forever.
 */
RTDECL(int)  RTCondVarRWReadWait(RTCONDVAR hCondVar, RTSEMRW hRWSem, RTMSINTERVAL cMillies);

/**
 * Wait for the condition variable to be signaled, return on interruption.
 *
 * This function will not resume the wait if interrupted.
 *
 * @returns iprt status code.
 * @param   hCondVar            The condition variable to wait on.
 * @param   hRWSem              The read/write semaphore to read-leave during
 *                              the wait and which will be re-enter in read mode
 *                              before returning.
 * @param   cMillies            Number of milliseconds to wait.  Use
 *                              RT_INDEFINITE_WAIT to wait forever.
 */
RTDECL(int)  RTCondVarRWReadWaitNoResume(RTCONDVAR hCondVar, RTSEMRW hRWSem, RTMSINTERVAL cMillies);

/**
 * Wait for the condition variable to be signaled, resume on interruption.
 *
 * This function will resume if the wait is interrupted by an async system event
 * (like a unix signal) or similar.
 *
 * @returns iprt status code.
 *          Will not return VERR_INTERRUPTED.
 * @param   hCondVar            The condition variable to wait on.
 * @param   pCritSect           The critical section to leave during the wait
 *                              and which will be re-enter before returning.
 * @param   cMillies            Number of milliseconds to wait.  Use
 *                              RT_INDEFINITE_WAIT to wait forever.
 */
RTDECL(int)  RTCondVarCritSectWait(RTCONDVAR hCondVar, PRTCRITSECT pCritSect, RTMSINTERVAL cMillies);

/**
 * Wait for the condition variable to be signaled, return on interruption.
 *
 * This function will not resume the wait if interrupted.
 *
 * @returns iprt status code.
 * @param   hCondVar            The condition variable to wait on.
 * @param   pCritSect           The critical section to leave during the wait
 *                              and which will be re-enter before returning.
 * @param   cMillies            Number of milliseconds to wait.  Use
 *                              RT_INDEFINITE_WAIT to wait forever.
 */
RTDECL(int)  RTCondVarCritSectWaitNoResume(RTCONDVAR hCondVar, PRTCRITSECT pCritSect, RTMSINTERVAL cMillies);

/**
 * Sets the signaller thread to one specific thread.
 *
 * This is only used for validating usage and deadlock detection.  When used
 * after calls to RTCondVarAddSignaller, the specified thread will be the only
 * signalling thread.
 *
 * @param   hCondVar            The condition variable.
 * @param   hThread             The thread that will signal it.  Pass
 *                              NIL_RTTHREAD to indicate that there is no
 *                              special signalling thread.
 */
RTDECL(void) RTCondVarSetSignaller(RTCONDVAR hCondVar, RTTHREAD hThread);

/**
 * To add more signalling threads.
 *
 * First call RTCondVarSetSignaller then add further threads with this.
 *
 * @param   hCondVar            The condition variable.
 * @param   hThread             The thread that will signal it. NIL_RTTHREAD is
 *                              not accepted.
 */
RTDECL(void) RTCondVarAddSignaller(RTCONDVAR hCondVar, RTTHREAD hThread);

/**
 * To remove a signalling thread.
 *
 * Reverts work done by RTCondVarAddSignaller and RTCondVarSetSignaller.
 *
 * @param   hCondVar            The condition variable.
 * @param   hThread             A previously added thread.
 */
RTDECL(void) RTCondVarRemoveSignaller(RTCONDVAR hCondVar, RTTHREAD hThread);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_condvar_h */

