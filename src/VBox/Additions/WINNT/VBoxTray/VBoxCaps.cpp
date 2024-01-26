/* $Id: VBoxCaps.cpp $ */
/** @file
 * VBoxCaps.cpp - Capability APIs.
 */

/*
 * Copyright (C) 2013-2023 Oracle and/or its affiliates.
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

#include <iprt/log.h>

#include "VBoxTray.h"
#include "VBoxTrayInternal.h"
#include "VBoxSeamless.h"


typedef enum VBOXCAPS_ENTRY_ACSTATE
{
    /* the given cap is released */
    VBOXCAPS_ENTRY_ACSTATE_RELEASED = 0,
    /* the given cap acquisition is in progress */
    VBOXCAPS_ENTRY_ACSTATE_ACQUIRING,
    /* the given cap is acquired */
    VBOXCAPS_ENTRY_ACSTATE_ACQUIRED
} VBOXCAPS_ENTRY_ACSTATE;


struct VBOXCAPS_ENTRY;
struct VBOXCAPS;

typedef DECLCALLBACKPTR(void, PFNVBOXCAPS_ENTRY_ON_ENABLE,(struct VBOXCAPS *pConsole, struct VBOXCAPS_ENTRY *pCap, BOOL fEnabled));

typedef struct VBOXCAPS_ENTRY
{
    uint32_t fCap;
    uint32_t iCap;
    VBOXCAPS_ENTRY_FUNCSTATE enmFuncState;
    VBOXCAPS_ENTRY_ACSTATE enmAcState;
    PFNVBOXCAPS_ENTRY_ON_ENABLE pfnOnEnable;
} VBOXCAPS_ENTRY;


typedef struct VBOXCAPS
{
    UINT_PTR idTimer;
    VBOXCAPS_ENTRY aCaps[VBOXCAPS_ENTRY_IDX_COUNT];
} VBOXCAPS;

static VBOXCAPS gVBoxCaps;


/* we need to perform Acquire/Release using the file handled we use for rewuesting events from VBoxGuest
 * otherwise Acquisition mechanism will treat us as different client and will not propagate necessary requests
 * */
int VBoxAcquireGuestCaps(uint32_t fOr, uint32_t fNot, bool fCfg)
{
    Log(("VBoxAcquireGuestCaps or(0x%x), not(0x%x), cfx(%d)\n", fOr, fNot, fCfg));
    int rc = VbglR3AcquireGuestCaps(fOr, fNot, fCfg);
    if (RT_FAILURE(rc))
        LogFlowFunc(("VBOXGUEST_IOCTL_GUEST_CAPS_ACQUIRE failed: %Rrc\n", rc));
    return rc;
}

static DECLCALLBACK(void) vboxCapsOnEnableSeamless(struct VBOXCAPS *pConsole, struct VBOXCAPS_ENTRY *pCap, BOOL fEnabled)
{
    RT_NOREF(pConsole, pCap);
    if (fEnabled)
    {
        Log(("vboxCapsOnEnableSeamless: ENABLED\n"));
        Assert(pCap->enmAcState == VBOXCAPS_ENTRY_ACSTATE_ACQUIRED);
        Assert(pCap->enmFuncState == VBOXCAPS_ENTRY_FUNCSTATE_STARTED);
        VBoxSeamlessEnable();
    }
    else
    {
        Log(("vboxCapsOnEnableSeamless: DISABLED\n"));
        Assert(pCap->enmAcState != VBOXCAPS_ENTRY_ACSTATE_ACQUIRED || pCap->enmFuncState != VBOXCAPS_ENTRY_FUNCSTATE_STARTED);
        VBoxSeamlessDisable();
    }
}

static void vboxCapsEntryAcStateSet(VBOXCAPS_ENTRY *pCap, VBOXCAPS_ENTRY_ACSTATE enmAcState)
{
    VBOXCAPS *pConsole = &gVBoxCaps;

    Log(("vboxCapsEntryAcStateSet: new state enmAcState(%d); pCap: fCap(%d), iCap(%d), enmFuncState(%d), enmAcState(%d)\n",
            enmAcState, pCap->fCap, pCap->iCap, pCap->enmFuncState, pCap->enmAcState));

    if (pCap->enmAcState == enmAcState)
        return;

    VBOXCAPS_ENTRY_ACSTATE enmOldAcState = pCap->enmAcState;
    pCap->enmAcState = enmAcState;

    if (enmAcState == VBOXCAPS_ENTRY_ACSTATE_ACQUIRED)
    {
        if (pCap->enmFuncState == VBOXCAPS_ENTRY_FUNCSTATE_STARTED)
        {
            if (pCap->pfnOnEnable)
                pCap->pfnOnEnable(pConsole, pCap, TRUE);
        }
    }
    else if (enmOldAcState == VBOXCAPS_ENTRY_ACSTATE_ACQUIRED && pCap->enmFuncState == VBOXCAPS_ENTRY_FUNCSTATE_STARTED)
    {
        if (pCap->pfnOnEnable)
            pCap->pfnOnEnable(pConsole, pCap, FALSE);
    }
}

static void vboxCapsEntryFuncStateSet(VBOXCAPS_ENTRY *pCap, VBOXCAPS_ENTRY_FUNCSTATE enmFuncState)
{
    VBOXCAPS *pConsole = &gVBoxCaps;

    Log(("vboxCapsEntryFuncStateSet: new state enmAcState(%d); pCap: fCap(%d), iCap(%d), enmFuncState(%d), enmAcState(%d)\n",
            enmFuncState, pCap->fCap, pCap->iCap, pCap->enmFuncState, pCap->enmAcState));

    if (pCap->enmFuncState == enmFuncState)
        return;

    VBOXCAPS_ENTRY_FUNCSTATE enmOldFuncState = pCap->enmFuncState;

    pCap->enmFuncState = enmFuncState;

    if (enmFuncState == VBOXCAPS_ENTRY_FUNCSTATE_STARTED)
    {
        Assert(enmOldFuncState == VBOXCAPS_ENTRY_FUNCSTATE_SUPPORTED);
        if (pCap->enmAcState == VBOXCAPS_ENTRY_ACSTATE_ACQUIRED)
        {
            if (pCap->pfnOnEnable)
                pCap->pfnOnEnable(pConsole, pCap, TRUE);
        }
    }
    else if (pCap->enmAcState == VBOXCAPS_ENTRY_ACSTATE_ACQUIRED && enmOldFuncState == VBOXCAPS_ENTRY_FUNCSTATE_STARTED)
    {
        if (pCap->pfnOnEnable)
            pCap->pfnOnEnable(pConsole, pCap, FALSE);
    }
}

void VBoxCapsEntryFuncStateSet(uint32_t iCup, VBOXCAPS_ENTRY_FUNCSTATE enmFuncState)
{
    VBOXCAPS *pConsole = &gVBoxCaps;
    VBOXCAPS_ENTRY *pCap = &pConsole->aCaps[iCup];
    vboxCapsEntryFuncStateSet(pCap, enmFuncState);
}

int VBoxCapsInit()
{
    VBOXCAPS *pConsole = &gVBoxCaps;
    memset(pConsole, 0, sizeof (*pConsole));
    pConsole->aCaps[VBOXCAPS_ENTRY_IDX_SEAMLESS].fCap = VMMDEV_GUEST_SUPPORTS_SEAMLESS;
    pConsole->aCaps[VBOXCAPS_ENTRY_IDX_SEAMLESS].iCap = VBOXCAPS_ENTRY_IDX_SEAMLESS;
    pConsole->aCaps[VBOXCAPS_ENTRY_IDX_SEAMLESS].pfnOnEnable = vboxCapsOnEnableSeamless;
    pConsole->aCaps[VBOXCAPS_ENTRY_IDX_GRAPHICS].fCap = VMMDEV_GUEST_SUPPORTS_GRAPHICS;
    pConsole->aCaps[VBOXCAPS_ENTRY_IDX_GRAPHICS].iCap = VBOXCAPS_ENTRY_IDX_GRAPHICS;
    return VINF_SUCCESS;
}

int VBoxCapsReleaseAll()
{
    VBOXCAPS *pConsole = &gVBoxCaps;
    Log(("VBoxCapsReleaseAll\n"));
    int rc = VBoxAcquireGuestCaps(0, VMMDEV_GUEST_SUPPORTS_SEAMLESS | VMMDEV_GUEST_SUPPORTS_GRAPHICS, false);
    if (!RT_SUCCESS(rc))
    {
        LogFlowFunc(("vboxCapsEntryReleaseAll VBoxAcquireGuestCaps failed rc %d\n", rc));
        return rc;
    }

    if (pConsole->idTimer)
    {
        Log(("killing console timer\n"));
        KillTimer(g_hwndToolWindow, pConsole->idTimer);
        pConsole->idTimer = 0;
    }

    for (int i = 0; i < RT_ELEMENTS(pConsole->aCaps); ++i)
    {
        vboxCapsEntryAcStateSet(&pConsole->aCaps[i], VBOXCAPS_ENTRY_ACSTATE_RELEASED);
    }

    return rc;
}

void VBoxCapsTerm()
{
    VBOXCAPS *pConsole = &gVBoxCaps;
    VBoxCapsReleaseAll();
    memset(pConsole, 0, sizeof (*pConsole));
}

BOOL VBoxCapsEntryIsAcquired(uint32_t iCap)
{
    VBOXCAPS *pConsole = &gVBoxCaps;
    return pConsole->aCaps[iCap].enmAcState == VBOXCAPS_ENTRY_ACSTATE_ACQUIRED;
}

BOOL VBoxCapsEntryIsEnabled(uint32_t iCap)
{
    VBOXCAPS *pConsole = &gVBoxCaps;
    return pConsole->aCaps[iCap].enmAcState == VBOXCAPS_ENTRY_ACSTATE_ACQUIRED
            && pConsole->aCaps[iCap].enmFuncState == VBOXCAPS_ENTRY_FUNCSTATE_STARTED;
}

BOOL VBoxCapsCheckTimer(WPARAM wParam)
{
    VBOXCAPS *pConsole = &gVBoxCaps;
    if (wParam != pConsole->idTimer)
        return FALSE;

    uint32_t u32AcquiredCaps = 0;
    BOOL fNeedNewTimer = FALSE;

    for (int i = 0; i < RT_ELEMENTS(pConsole->aCaps); ++i)
    {
        VBOXCAPS_ENTRY *pCap = &pConsole->aCaps[i];
        if (pCap->enmAcState != VBOXCAPS_ENTRY_ACSTATE_ACQUIRING)
            continue;

        int rc = VBoxAcquireGuestCaps(pCap->fCap, 0, false);
        if (RT_SUCCESS(rc))
        {
            vboxCapsEntryAcStateSet(&pConsole->aCaps[i], VBOXCAPS_ENTRY_ACSTATE_ACQUIRED);
            u32AcquiredCaps |= pCap->fCap;
        }
        else
        {
            Assert(rc == VERR_RESOURCE_BUSY);
            fNeedNewTimer = TRUE;
        }
    }

    if (!fNeedNewTimer)
    {
        KillTimer(g_hwndToolWindow, pConsole->idTimer);
        /* cleanup timer data */
        pConsole->idTimer = 0;
    }

    return TRUE;
}

int VBoxCapsEntryRelease(uint32_t iCap)
{
    VBOXCAPS *pConsole = &gVBoxCaps;
    VBOXCAPS_ENTRY *pCap = &pConsole->aCaps[iCap];
    if (pCap->enmAcState == VBOXCAPS_ENTRY_ACSTATE_RELEASED)
    {
        LogFlowFunc(("invalid cap[%d] state[%d] on release\n", iCap, pCap->enmAcState));
        return VERR_INVALID_STATE;
    }

    if (pCap->enmAcState == VBOXCAPS_ENTRY_ACSTATE_ACQUIRED)
    {
        int rc = VBoxAcquireGuestCaps(0, pCap->fCap, false);
        AssertRC(rc);
    }

    vboxCapsEntryAcStateSet(pCap, VBOXCAPS_ENTRY_ACSTATE_RELEASED);

    return VINF_SUCCESS;
}

int VBoxCapsEntryAcquire(uint32_t iCap)
{
    VBOXCAPS *pConsole = &gVBoxCaps;
    Assert(VBoxConsoleIsAllowed());
    VBOXCAPS_ENTRY *pCap = &pConsole->aCaps[iCap];
    Log(("VBoxCapsEntryAcquire %d\n", iCap));
    if (pCap->enmAcState != VBOXCAPS_ENTRY_ACSTATE_RELEASED)
    {
        LogFlowFunc(("invalid cap[%d] state[%d] on acquire\n", iCap, pCap->enmAcState));
        return VERR_INVALID_STATE;
    }

    vboxCapsEntryAcStateSet(pCap, VBOXCAPS_ENTRY_ACSTATE_ACQUIRING);
    int rc = VBoxAcquireGuestCaps(pCap->fCap, 0, false);
    if (RT_SUCCESS(rc))
    {
        vboxCapsEntryAcStateSet(pCap, VBOXCAPS_ENTRY_ACSTATE_ACQUIRED);
        return VINF_SUCCESS;
    }

    if (rc != VERR_RESOURCE_BUSY)
    {
        LogFlowFunc(("vboxCapsEntryReleaseAll VBoxAcquireGuestCaps failed rc %d\n", rc));
        return rc;
    }

    LogFlowFunc(("iCap %d is busy!\n", iCap));

    /* the cap was busy, most likely it is still used by other VBoxTray instance running in another session,
     * queue the retry timer */
    if (!pConsole->idTimer)
    {
        pConsole->idTimer = SetTimer(g_hwndToolWindow, TIMERID_VBOXTRAY_CAPS_TIMER, 100, (TIMERPROC)NULL);
        if (!pConsole->idTimer)
        {
            DWORD dwErr = GetLastError();
            LogFlowFunc(("SetTimer error %08X\n", dwErr));
            return RTErrConvertFromWin32(dwErr);
        }
    }

    return rc;
}

int VBoxCapsAcquireAllSupported()
{
    VBOXCAPS *pConsole = &gVBoxCaps;
    Log(("VBoxCapsAcquireAllSupported\n"));
    for (int i = 0; i < RT_ELEMENTS(pConsole->aCaps); ++i)
    {
        if (pConsole->aCaps[i].enmFuncState >= VBOXCAPS_ENTRY_FUNCSTATE_SUPPORTED)
        {
            Log(("VBoxCapsAcquireAllSupported acquiring cap %d, state %d\n", i, pConsole->aCaps[i].enmFuncState));
            VBoxCapsEntryAcquire(i);
        }
        else
        {
            LogFlowFunc(("VBoxCapsAcquireAllSupported: WARN: cap %d not supported, state %d\n", i, pConsole->aCaps[i].enmFuncState));
        }
    }
    return VINF_SUCCESS;
}

