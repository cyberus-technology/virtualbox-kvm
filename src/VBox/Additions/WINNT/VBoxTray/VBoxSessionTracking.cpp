/* $Id: VBoxSessionTracking.cpp $ */
/** @file
 * VBoxSessionTracking.cpp - Session tracking.
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


#include <iprt/errcore.h>
#include <iprt/ldr.h>
#include <iprt/log.h>
#include <iprt/string.h>
#include <iprt/win/windows.h>

/* Default desktop state tracking */
#include <Wtsapi32.h>

#include "VBoxTray.h"


/* St (session [state] tracking) functionality API impl */

typedef struct VBOXST
{
    HWND hWTSAPIWnd;
    RTLDRMOD hLdrModWTSAPI32;
    BOOL fIsConsole;
    WTS_CONNECTSTATE_CLASS enmConnectState;
    UINT_PTR idDelayedInitTimer;
    BOOL (WINAPI * pfnWTSRegisterSessionNotification)(HWND hWnd, DWORD dwFlags);
    BOOL (WINAPI * pfnWTSUnRegisterSessionNotification)(HWND hWnd);
    BOOL (WINAPI * pfnWTSQuerySessionInformationA)(HANDLE hServer, DWORD SessionId, WTS_INFO_CLASS WTSInfoClass, LPTSTR *ppBuffer, DWORD *pBytesReturned);
} VBOXST;

static VBOXST gVBoxSt;

/** @todo r=andy Lacks documentation / Doxygen headers. What does this, and why? */

int vboxStCheckState()
{
    int rc = VINF_SUCCESS;
    WTS_CONNECTSTATE_CLASS *penmConnectState = NULL;
    USHORT *pProtocolType = NULL;
    DWORD cbBuf = 0;
    if (gVBoxSt.pfnWTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSConnectState,
                                               (LPTSTR *)&penmConnectState, &cbBuf))
    {
        if (gVBoxSt.pfnWTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSClientProtocolType,
                                                   (LPTSTR *)&pProtocolType, &cbBuf))
        {
            gVBoxSt.fIsConsole = (*pProtocolType == 0);
            gVBoxSt.enmConnectState = *penmConnectState;
            return VINF_SUCCESS;
        }

        DWORD dwErr = GetLastError();
        LogFlowFunc(("WTSQuerySessionInformationA WTSClientProtocolType failed, error = %08X\n", dwErr));
        rc = RTErrConvertFromWin32(dwErr);
    }
    else
    {
        DWORD dwErr = GetLastError();
        LogFlowFunc(("WTSQuerySessionInformationA WTSConnectState failed, error = %08X\n", dwErr));
        rc = RTErrConvertFromWin32(dwErr);
    }

    /* failure branch, set to "console-active" state */
    gVBoxSt.fIsConsole = TRUE;
    gVBoxSt.enmConnectState = WTSActive;

    return rc;
}

int vboxStInit(HWND hWnd)
{
    RT_ZERO(gVBoxSt);
    int rc = RTLdrLoadSystem("WTSAPI32.DLL", false /*fNoUnload*/, &gVBoxSt.hLdrModWTSAPI32);
    if (RT_SUCCESS(rc))
    {
        rc = RTLdrGetSymbol(gVBoxSt.hLdrModWTSAPI32, "WTSRegisterSessionNotification",
                            (void **)&gVBoxSt.pfnWTSRegisterSessionNotification);
        if (RT_SUCCESS(rc))
        {
            rc = RTLdrGetSymbol(gVBoxSt.hLdrModWTSAPI32, "WTSUnRegisterSessionNotification",
                                (void **)&gVBoxSt.pfnWTSUnRegisterSessionNotification);
            if (RT_SUCCESS(rc))
            {
                rc = RTLdrGetSymbol(gVBoxSt.hLdrModWTSAPI32, "WTSQuerySessionInformationA",
                                    (void **)&gVBoxSt.pfnWTSQuerySessionInformationA);
                if (RT_FAILURE(rc))
                    LogFlowFunc(("WTSQuerySessionInformationA not found\n"));
            }
            else
                LogFlowFunc(("WTSUnRegisterSessionNotification not found\n"));
        }
        else
            LogFlowFunc(("WTSRegisterSessionNotification not found\n"));
        if (RT_SUCCESS(rc))
        {
            gVBoxSt.hWTSAPIWnd = hWnd;
            if (gVBoxSt.pfnWTSRegisterSessionNotification(gVBoxSt.hWTSAPIWnd, NOTIFY_FOR_THIS_SESSION))
                vboxStCheckState();
            else
            {
                DWORD dwErr = GetLastError();
                LogFlowFunc(("WTSRegisterSessionNotification failed, error = %08X\n", dwErr));
                if (dwErr == RPC_S_INVALID_BINDING)
                {
                    gVBoxSt.idDelayedInitTimer = SetTimer(gVBoxSt.hWTSAPIWnd, TIMERID_VBOXTRAY_ST_DELAYED_INIT_TIMER,
                                                          2000, (TIMERPROC)NULL);
                    gVBoxSt.fIsConsole = TRUE;
                    gVBoxSt.enmConnectState = WTSActive;
                    rc = VINF_SUCCESS;
                }
                else
                    rc = RTErrConvertFromWin32(dwErr);
            }

            if (RT_SUCCESS(rc))
                return VINF_SUCCESS;
        }

        RTLdrClose(gVBoxSt.hLdrModWTSAPI32);
    }
    else
        LogFlowFunc(("WTSAPI32 load failed, rc = %Rrc\n", rc));

    RT_ZERO(gVBoxSt);
    gVBoxSt.fIsConsole = TRUE;
    gVBoxSt.enmConnectState = WTSActive;
    return rc;
}

void vboxStTerm(void)
{
    if (!gVBoxSt.hWTSAPIWnd)
    {
        LogFlowFunc(("vboxStTerm called for non-initialized St\n"));
        return;
    }

    if (gVBoxSt.idDelayedInitTimer)
    {
        /* notification is not registered, just kill timer */
        KillTimer(gVBoxSt.hWTSAPIWnd, gVBoxSt.idDelayedInitTimer);
        gVBoxSt.idDelayedInitTimer = 0;
    }
    else
    {
        if (!gVBoxSt.pfnWTSUnRegisterSessionNotification(gVBoxSt.hWTSAPIWnd))
        {
            LogFlowFunc(("WTSAPI32 load failed, error = %08X\n", GetLastError()));
        }
    }

    RTLdrClose(gVBoxSt.hLdrModWTSAPI32);
    RT_ZERO(gVBoxSt);
}

#define VBOXST_DBG_MAKECASE(_val) case _val: return #_val;

static const char* vboxStDbgGetString(DWORD val)
{
    switch (val)
    {
        VBOXST_DBG_MAKECASE(WTS_CONSOLE_CONNECT);
        VBOXST_DBG_MAKECASE(WTS_CONSOLE_DISCONNECT);
        VBOXST_DBG_MAKECASE(WTS_REMOTE_CONNECT);
        VBOXST_DBG_MAKECASE(WTS_REMOTE_DISCONNECT);
        VBOXST_DBG_MAKECASE(WTS_SESSION_LOGON);
        VBOXST_DBG_MAKECASE(WTS_SESSION_LOGOFF);
        VBOXST_DBG_MAKECASE(WTS_SESSION_LOCK);
        VBOXST_DBG_MAKECASE(WTS_SESSION_UNLOCK);
        VBOXST_DBG_MAKECASE(WTS_SESSION_REMOTE_CONTROL);
        default:
            LogFlowFunc(("invalid WTS state %d\n", val));
            return "Unknown";
    }
}

BOOL vboxStCheckTimer(WPARAM wEvent)
{
    if (wEvent != gVBoxSt.idDelayedInitTimer)
        return FALSE;

    if (gVBoxSt.pfnWTSRegisterSessionNotification(gVBoxSt.hWTSAPIWnd, NOTIFY_FOR_THIS_SESSION))
    {
        KillTimer(gVBoxSt.hWTSAPIWnd, gVBoxSt.idDelayedInitTimer);
        gVBoxSt.idDelayedInitTimer = 0;
        vboxStCheckState();
    }
    else
    {
        LogFlowFunc(("timer WTSRegisterSessionNotification failed, error = %08X\n", GetLastError()));
        Assert(gVBoxSt.fIsConsole == TRUE);
        Assert(gVBoxSt.enmConnectState == WTSActive);
    }

    return TRUE;
}

BOOL vboxStIsActiveConsole(void)
{
    return (gVBoxSt.enmConnectState == WTSActive && gVBoxSt.fIsConsole);
}

BOOL vboxStHandleEvent(WPARAM wEvent)
{
    RT_NOREF(wEvent);
    LogFlowFunc(("WTS Event: %s\n", vboxStDbgGetString(wEvent)));
    BOOL fOldIsActiveConsole = vboxStIsActiveConsole();

    vboxStCheckState();

    return !vboxStIsActiveConsole() != !fOldIsActiveConsole;
}

