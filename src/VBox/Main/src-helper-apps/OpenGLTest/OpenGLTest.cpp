/* $Id: OpenGLTest.cpp $ */
/** @file
 * VBox host opengl support test - generic implementation.
 */

/*
 * Copyright (C) 2009-2023 Oracle and/or its affiliates.
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

#include <VBox/err.h>
#include <iprt/assert.h>
#include <iprt/env.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/process.h>
#include <iprt/string.h>
#include <iprt/time.h>
#include <iprt/thread.h>
#include <iprt/env.h>
#include <iprt/log.h>

#include <VBox/VBoxOGL.h>

bool RTCALL VBoxOglIs3DAccelerationSupported(void)
{
    if (RTEnvExist("VBOX_3D_FORCE_SUPPORTED"))
    {
        LogRel(("VBOX_3D_FORCE_SUPPORTED is specified, skipping 3D test, and treating as supported\n"));
        return true;
    }

    static char pszVBoxPath[RTPATH_MAX];
    const char *papszArgs[4] = { NULL, "-test", "3D", NULL};

#ifdef __SANITIZE_ADDRESS__
    /* The OpenGL test tool contains a number of memory leaks which cause it to
     * return failure when run with ASAN unless we disable the leak detector. */
    RTENV env;
    if (RT_FAILURE(RTEnvClone(&env, RTENV_DEFAULT)))
        return false;
    RTEnvPutEx(env, "ASAN_OPTIONS=detect_leaks=0");  /* If this fails we will notice later */
#endif
    int vrc = RTPathExecDir(pszVBoxPath, RTPATH_MAX);
    AssertRCReturn(vrc, false);
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
    vrc = RTPathAppend(pszVBoxPath, RTPATH_MAX, "VBoxTestOGL.exe");
#else
    vrc = RTPathAppend(pszVBoxPath, RTPATH_MAX, "VBoxTestOGL");
#endif
    papszArgs[0] = pszVBoxPath;         /* argv[0] */
    AssertRCReturn(vrc, false);

    RTPROCESS Process;
#ifndef __SANITIZE_ADDRESS__
    vrc = RTProcCreate(pszVBoxPath, papszArgs, RTENV_DEFAULT, 0, &Process);
#else
    vrc = RTProcCreate(pszVBoxPath, papszArgs, env, 0, &Process);
    RTEnvDestroy(env);
#endif
    if (RT_FAILURE(vrc))
        return false;

    uint64_t StartTS = RTTimeMilliTS();

    RTPROCSTATUS ProcStatus = {0};
    while (1)
    {
        vrc = RTProcWait(Process, RTPROCWAIT_FLAGS_NOBLOCK, &ProcStatus);
        if (vrc != VERR_PROCESS_RUNNING)
            break;

#ifndef DEBUG_misha
        if (RTTimeMilliTS() - StartTS > 30*1000 /* 30 sec */)
        {
            RTProcTerminate(Process);
            RTThreadSleep(100);
            RTProcWait(Process, RTPROCWAIT_FLAGS_NOBLOCK, &ProcStatus);
            return false;
        }
#endif
        RTThreadSleep(100);
    }

    if (RT_SUCCESS(vrc))
        if (   ProcStatus.enmReason == RTPROCEXITREASON_NORMAL
            && ProcStatus.iStatus   == 0)
            return true;

    return false;
}

