/* $Id: seamless.cpp $ */
/** @file
 * X11 Guest client - seamless mode: main logic, communication with the host and
 * wrapper interface for the main code of the VBoxClient deamon.  The
 * X11-specific parts are split out into their own file for ease of testing.
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
*   Header files                                                                                                                 *
*********************************************************************************************************************************/
#include <new>

#include <X11/Xlib.h>

#include <iprt/asm.h>
#include <iprt/errcore.h>
#include <iprt/mem.h>

#include <VBox/log.h>
#include <VBox/VBoxGuestLib.h>

#include "VBoxClient.h"
#include "seamless.h"


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/

/**
 * Struct for keeping a service instance.
 */
struct SEAMLESSSERVICE
{
    /** Seamless service object. */
    SeamlessMain mSeamless;
};

/** Service instance data. */
static SEAMLESSSERVICE g_Svc;


SeamlessMain::SeamlessMain(void)
{
    mX11MonitorThread         = NIL_RTTHREAD;
    mX11MonitorThreadStopping = false;

    mMode    = VMMDev_Seamless_Disabled;
    mfPaused = true;
}

SeamlessMain::~SeamlessMain()
{
    /* Stopping will be done via main.cpp. */
}

/**
 * Update the set of visible rectangles in the host.
 */
static void sendRegionUpdate(RTRECT *pRects, size_t cRects)
{
    if (   cRects
        && !pRects)  /* Assertion */
    {
        VBClLogError(("Region update called with NULL pointer\n"));
        return;
    }
    VbglR3SeamlessSendRects(cRects, pRects);
}

/** @copydoc VBCLSERVICE::pfnInit */
int SeamlessMain::init(void)
{
    int rc;
    const char *pcszStage;

    do
    {
        pcszStage = "Connecting to the X server";
        rc = mX11Monitor.init(sendRegionUpdate);
        if (RT_FAILURE(rc))
            break;
        pcszStage = "Setting guest IRQ filter mask";
        rc = VbglR3CtlFilterMask(VMMDEV_EVENT_SEAMLESS_MODE_CHANGE_REQUEST, 0);
        if (RT_FAILURE(rc))
            break;
        pcszStage = "Reporting support for seamless capability";
        rc = VbglR3SeamlessSetCap(true);
        if (RT_FAILURE(rc))
            break;
        rc = startX11MonitorThread();
        if (RT_FAILURE(rc))
            break;

    } while(0);

    if (RT_FAILURE(rc))
        VBClLogError("Failed to start in stage '%s' -- error %Rrc\n", pcszStage, rc);

    return rc;
}

/** @copydoc VBCLSERVICE::pfnWorker */
int SeamlessMain::worker(bool volatile *pfShutdown)
{
    int rc = VINF_SUCCESS;

    /* Let the main thread know that it can continue spawning services. */
    RTThreadUserSignal(RTThreadSelf());

    /* This will only exit if something goes wrong. */
    for (;;)
    {
        if (ASMAtomicReadBool(pfShutdown))
            break;

        rc = nextStateChangeEvent();

        if (rc == VERR_TRY_AGAIN)
            rc = VINF_SUCCESS;

        if (RT_FAILURE(rc))
            break;

        if (ASMAtomicReadBool(pfShutdown))
            break;

        /* If we are not stopping, sleep for a bit to avoid using up too
           much CPU while retrying. */
        RTThreadYield();
    }

    return rc;
}

/** @copydoc VBCLSERVICE::pfnStop */
void SeamlessMain::stop(void)
{
    VbglR3SeamlessSetCap(false);
    VbglR3CtlFilterMask(0, VMMDEV_EVENT_SEAMLESS_MODE_CHANGE_REQUEST);
    stopX11MonitorThread();
}

/** @copydoc VBCLSERVICE::pfnTerm */
int SeamlessMain::term(void)
{
    mX11Monitor.uninit();
    return VINF_SUCCESS;
}

/**
 * Waits for a seamless state change events from the host and dispatch it.
 *
 * @returns VBox return code, or
 *          VERR_TRY_AGAIN if no new status is available and we have to try it again
 *          at some later point in time.
 */
int SeamlessMain::nextStateChangeEvent(void)
{
    VMMDevSeamlessMode newMode = VMMDev_Seamless_Disabled;

    int rc = VbglR3SeamlessWaitEvent(&newMode);
    if (RT_SUCCESS(rc))
    {
        mMode = newMode;
        switch (newMode)
        {
            case VMMDev_Seamless_Visible_Region:
                /* A simplified seamless mode, obtained by making the host VM window
                 * borderless and making the guest desktop transparent. */
                VBClLogVerbose(2, "\"Visible region\" mode requested\n");
                break;
            case VMMDev_Seamless_Disabled:
                VBClLogVerbose(2, "\"Disabled\" mode requested\n");
                break;
            case VMMDev_Seamless_Host_Window:
                /* One host window represents one guest window.  Not yet implemented. */
                VBClLogVerbose(2, "Unsupported \"host window\" mode requested\n");
                return VERR_NOT_SUPPORTED;
            default:
                VBClLogError("Unsupported mode %d requested\n", newMode);
                return VERR_NOT_SUPPORTED;
        }
    }
    if (   RT_SUCCESS(rc)
        || rc == VERR_TRY_AGAIN)
    {
        if (mMode == VMMDev_Seamless_Visible_Region)
            mfPaused = false;
        else
            mfPaused = true;
        mX11Monitor.interruptEventWait();
    }
    else
        VBClLogError("VbglR3SeamlessWaitEvent returned %Rrc\n", rc);

    return rc;
}

/**
 * The actual X11 window configuration change monitor thread function.
 */
int SeamlessMain::x11MonitorThread(RTTHREAD hThreadSelf, void *pvUser)
{
    RT_NOREF(hThreadSelf);

    SeamlessMain *pThis = (SeamlessMain *)pvUser;
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;

    RTThreadUserSignal(hThreadSelf);

    VBClLogVerbose(2, "X11 monitor thread started\n");

    while (!pThis->mX11MonitorThreadStopping)
    {
        if (!pThis->mfPaused)
        {
            rc = pThis->mX11Monitor.start();
            if (RT_FAILURE(rc))
                VBClLogFatalError("Failed to change the X11 seamless service state, mfPaused=%RTbool, rc=%Rrc\n",
                                  pThis->mfPaused, rc);
        }

        pThis->mX11Monitor.nextConfigurationEvent();

        if (   pThis->mfPaused
            || pThis->mX11MonitorThreadStopping)
        {
            pThis->mX11Monitor.stop();
        }
    }

    VBClLogVerbose(2, "X11 monitor thread ended\n");

    return rc;
}

/**
 * Start the X11 window configuration change monitor thread.
 */
int SeamlessMain::startX11MonitorThread(void)
{
    mX11MonitorThreadStopping = false;

    if (isX11MonitorThreadRunning())
        return VINF_SUCCESS;

    int rc = RTThreadCreate(&mX11MonitorThread, x11MonitorThread, this, 0,
                            RTTHREADTYPE_MSG_PUMP, RTTHREADFLAGS_WAITABLE,
                            "seamless x11");
    if (RT_SUCCESS(rc))
        rc = RTThreadUserWait(mX11MonitorThread, RT_MS_30SEC);

    if (RT_FAILURE(rc))
        VBClLogError("Failed to start X11 monitor thread, rc=%Rrc\n", rc);

    return rc;
}

/**
 * Stops the monitor thread.
 */
int SeamlessMain::stopX11MonitorThread(void)
{
    if (!isX11MonitorThreadRunning())
        return VINF_SUCCESS;

    mX11MonitorThreadStopping = true;
    if (!mX11Monitor.interruptEventWait())
    {
        VBClLogError("Unable to notify X11 monitor thread\n");
        return VERR_INVALID_STATE;
    }

    int rcThread;
    int rc = RTThreadWait(mX11MonitorThread, RT_MS_30SEC, &rcThread);
    if (RT_SUCCESS(rc))
        rc = rcThread;

    if (RT_SUCCESS(rc))
    {
        mX11MonitorThread = NIL_RTTHREAD;
    }
    else
        VBClLogError("Waiting for X11 monitor thread to stop failed, rc=%Rrc\n", rc);

    return rc;
}

/**
 * @interface_method_impl{VBCLSERVICE,pfnInit}
 */
static DECLCALLBACK(int) vbclSeamlessInit(void)
{
    return g_Svc.mSeamless.init();
}

/**
 * @interface_method_impl{VBCLSERVICE,pfnWorker}
 */
static DECLCALLBACK(int) vbclSeamlessWorker(bool volatile *pfShutdown)
{
    return g_Svc.mSeamless.worker(pfShutdown);
}

/**
 * @interface_method_impl{VBCLSERVICE,pfnStop}
 */
static DECLCALLBACK(void) vbclSeamlessStop(void)
{
    return g_Svc.mSeamless.stop();
}

/**
 * @interface_method_impl{VBCLSERVICE,pfnTerm}
 */
static DECLCALLBACK(int) vbclSeamlessTerm(void)
{
    return g_Svc.mSeamless.term();
}

VBCLSERVICE g_SvcSeamless =
{
    "seamless",                 /* szName */
    "Seamless Mode Support",    /* pszDescription */
    ".vboxclient-seamless",     /* pszPidFilePathTemplate */
    NULL,                       /* pszUsage */
    NULL,                       /* pszOptions */
    NULL,                       /* pfnOption */
    vbclSeamlessInit,           /* pfnInit */
    vbclSeamlessWorker,         /* pfnWorker */
    vbclSeamlessStop,           /* pfnStop*/
    vbclSeamlessTerm            /* pfnTerm */
};

