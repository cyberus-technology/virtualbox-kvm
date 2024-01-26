/* $Id: seamless.h $ */
/** @file
 * X11 Guest client - seamless mode, missing proper description while using the
 * potentially confusing word 'host'.
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

#ifndef GA_INCLUDED_SRC_x11_VBoxClient_seamless_h
#define GA_INCLUDED_SRC_x11_VBoxClient_seamless_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/thread.h>

#include <VBox/log.h>
#include <VBox/VBoxGuestLib.h>      /* for the R3 guest library functions  */

#include "seamless-x11.h"

/**
 * Interface to the host
 */
class SeamlessMain
{
private:
    // We don't want a copy constructor or assignment operator
    SeamlessMain(const SeamlessMain&);
    SeamlessMain& operator=(const SeamlessMain&);

    /** X11 event monitor object */
    SeamlessX11 mX11Monitor;

    /** Thread to start and stop when we enter and leave seamless mode which
     *  monitors X11 windows in the guest. */
    RTTHREAD mX11MonitorThread;
    /** Should the X11 monitor thread be stopping? */
    volatile bool mX11MonitorThreadStopping;

    /** The current seamless mode we are in. */
    VMMDevSeamlessMode mMode;
    /** Is the service currently paused? */
    volatile bool mfPaused;

    /**
     * Waits for a seamless state change events from the host and dispatch it.  This is
     * meant to be called by the host event monitor thread exclusively.
     *
     * @returns        IRPT return code.
     */
    int nextStateChangeEvent(void);

    /** Thread function to monitor X11 window configuration changes. */
    static DECLCALLBACK(int) x11MonitorThread(RTTHREAD self, void *pvUser);

    /** Helper to start the X11 monitor thread. */
    int startX11MonitorThread(void);

    /** Helper to stop the X11 monitor thread again. */
    int stopX11MonitorThread(void);

    /** Is the service currently actively monitoring X11 windows? */
    bool isX11MonitorThreadRunning()
    {
        return mX11MonitorThread != NIL_RTTHREAD;
    }

public:
    SeamlessMain(void);
    virtual ~SeamlessMain();
#ifdef RT_NEED_NEW_AND_DELETE
    RTMEM_IMPLEMENT_NEW_AND_DELETE();
#endif

    /** @copydoc VBCLSERVICE::pfnInit */
    int init(void);

    /** @copydoc VBCLSERVICE::pfnWorker */
    int worker(bool volatile *pfShutdown);

    /** @copydoc VBCLSERVICE::pfnStop */
    void stop(void);

    /** @copydoc VBCLSERVICE::pfnTerm */
    int term(void);
};

#endif /* !GA_INCLUDED_SRC_x11_VBoxClient_seamless_h */
