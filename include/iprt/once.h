/** @file
 * IPRT - Execute Once.
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

#ifndef IPRT_INCLUDED_once_h
#define IPRT_INCLUDED_once_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/asm.h>
#include <iprt/errcore.h>
#include <iprt/list.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_once       RTOnce - Execute Once
 * @ingroup grp_rt
 * @{
 */

/**
 * Callback that gets executed once.
 *
 * @returns IPRT style status code, RTOnce returns this.
 *
 * @param   pvUser          The user parameter.
 */
typedef DECLCALLBACKTYPE(int32_t, FNRTONCE,(void *pvUser));
/** Pointer to a FNRTONCE. */
typedef FNRTONCE *PFNRTONCE;

/**
 * Callback that gets executed on IPRT/process termination.
 *
 * @param   pvUser          The user parameter.
 * @param   fLazyCleanUpOk  Indicates whether lazy clean-up is OK (see
 *                          initterm.h).
 */
typedef DECLCALLBACKTYPE(void, FNRTONCECLEANUP,(void *pvUser, bool fLazyCleanUpOk));
/** Pointer to a FNRTONCE. */
typedef FNRTONCECLEANUP *PFNRTONCECLEANUP;

/**
 * Execute once structure.
 *
 * This is typically a global variable that is statically initialized
 * by RTONCE_INITIALIZER.
 */
typedef struct RTONCE
{
    /** Event semaphore that the other guys are blocking on. */
    RTSEMEVENTMULTI volatile    hEventMulti;
    /** Reference counter for hEventMulti. */
    int32_t volatile            cEventRefs;
    /** See RTONCESTATE. */
    int32_t volatile            iState;
    /** The return code of pfnOnce. */
    int32_t volatile            rc;

    /** Pointer to the clean-up function. */
    PFNRTONCECLEANUP            pfnCleanUp;
    /** Argument to hand to the clean-up function. */
    void                       *pvUser;
    /** Clean-up list entry. */
    RTLISTNODE                  CleanUpNode;
} RTONCE;
/** Pointer to a execute once struct. */
typedef RTONCE *PRTONCE;

/**
 * The execute once statemachine.
 */
typedef enum RTONCESTATE
{
    /** RTOnce() has not been called.
     *  Next: NO_SEM */
    RTONCESTATE_UNINITIALIZED = 1,
    /** RTOnce() is busy, no race.
     *  Next: CREATING_SEM, DONE */
    RTONCESTATE_BUSY_NO_SEM,
    /** More than one RTOnce() caller is busy.
     *  Next: BUSY_HAVE_SEM, BUSY_SPIN, DONE_CREATING_SEM, DONE */
    RTONCESTATE_BUSY_CREATING_SEM,
    /** More than one RTOnce() caller, the first is busy, the others are
     *  waiting.
     *  Next: DONE */
    RTONCESTATE_BUSY_HAVE_SEM,
    /** More than one RTOnce() caller, the first is busy, the others failed to
     *  create a semaphore and are spinning.
     *  Next: DONE */
    RTONCESTATE_BUSY_SPIN,
    /** More than one RTOnce() caller, the first has completed, the others
     *  are busy creating the semaphore.
     *  Next: DONE_HAVE_SEM */
    RTONCESTATE_DONE_CREATING_SEM,
    /** More than one RTOnce() caller, the first is busy grabbing the
     *  semaphore, while the others are waiting.
     *  Next: DONE */
    RTONCESTATE_DONE_HAVE_SEM,
    /** The execute once stuff has completed. */
    RTONCESTATE_DONE = 16
} RTONCESTATE;

/** Static initializer for RTONCE variables. */
#define RTONCE_INITIALIZER \
    { NIL_RTSEMEVENTMULTI, 0, RTONCESTATE_UNINITIALIZED, VERR_INTERNAL_ERROR, NULL, NULL, { NULL, NULL } }


/**
 * Serializes execution of the pfnOnce function, making sure it's
 * executed exactly once and that nobody returns from RTOnce before
 * it has executed successfully.
 *
 * @returns IPRT like status code returned by pfnOnce.
 *
 * @param   pOnce           Pointer to the execute once variable.
 * @param   pfnOnce         The function to executed once.
 * @param   pfnCleanUp      The function that will be doing the cleaning up.
 *                          Optional.
 * @param   pvUser          The user parameter for pfnOnce.
 */
RTDECL(int) RTOnceSlow(PRTONCE pOnce, PFNRTONCE pfnOnce, FNRTONCECLEANUP pfnCleanUp, void *pvUser);

/**
 * Serializes execution of the pfnOnce function, making sure it's
 * executed exactly once and that nobody returns from RTOnce before
 * it has executed successfully.
 *
 * @returns IPRT like status code returned by pfnOnce.
 *
 * @param   pOnce           Pointer to the execute once variable.
 * @param   pfnOnce         The function to executed once.
 * @param   pvUser          The user parameter for pfnOnce.
 */
DECLINLINE(int) RTOnce(PRTONCE pOnce, PFNRTONCE pfnOnce, void *pvUser)
{
    int32_t iState = ASMAtomicUoReadS32(&pOnce->iState);
    if (RT_LIKELY(   iState == RTONCESTATE_DONE
                  || iState == RTONCESTATE_DONE_CREATING_SEM
                  || iState == RTONCESTATE_DONE_HAVE_SEM ))
        return ASMAtomicUoReadS32(&pOnce->rc);
    return RTOnceSlow(pOnce, pfnOnce, NULL, pvUser);
}

/**
 * Execute pfnOnce once and register a termination clean-up callback.
 *
 * Serializes execution of the pfnOnce function, making sure it's
 * executed exactly once and that nobody returns from RTOnce before
 * it has executed successfully.
 *
 * @returns IPRT like status code returned by pfnOnce.
 *
 * @param   pOnce           Pointer to the execute once variable.
 * @param   pfnOnce         The function to executed once.
 * @param   pfnCleanUp      The function that will be doing the cleaning up.
 * @param   pvUser          The user parameter for pfnOnce.
 */
DECLINLINE(int) RTOnceEx(PRTONCE pOnce, PFNRTONCE pfnOnce, PFNRTONCECLEANUP pfnCleanUp, void *pvUser)
{
    int32_t iState = ASMAtomicUoReadS32(&pOnce->iState);
    if (RT_LIKELY(   iState == RTONCESTATE_DONE
                  || iState == RTONCESTATE_DONE_CREATING_SEM
                  || iState == RTONCESTATE_DONE_HAVE_SEM ))
        return ASMAtomicUoReadS32(&pOnce->rc);
    return RTOnceSlow(pOnce, pfnOnce, pfnCleanUp, pvUser);
}

/**
 * Resets an execute once variable.
 *
 * The caller is responsible for making sure there are no concurrent accesses to
 * the execute once variable.
 *
 * @param   pOnce           Pointer to the execute once variable.
 */
RTDECL(void) RTOnceReset(PRTONCE pOnce);

/**
 * Check whether the execute once variable was successfullly initialized.
 */
DECLINLINE(bool) RTOnceWasInitialized(PRTONCE pOnce)
{
    int32_t const iState = ASMAtomicUoReadS32(&pOnce->iState);
    int32_t const rc     = ASMAtomicUoReadS32(&pOnce->rc);
    return RT_SUCCESS(rc)
        && (   iState == RTONCESTATE_DONE
            || iState == RTONCESTATE_DONE_CREATING_SEM
            || iState == RTONCESTATE_DONE_HAVE_SEM);
}

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_once_h */

