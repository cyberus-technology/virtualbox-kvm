/* $Id: OpenGLTestApp.cpp $ */
/** @file
 * VBox host opengl support test application.
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

#include <iprt/assert.h>
#include <iprt/buildconfig.h>
#include <iprt/errcore.h>
#include <iprt/getopt.h>
#include <iprt/initterm.h>
#include <iprt/ldr.h>
#include <iprt/message.h>
#include <iprt/stream.h>
#ifdef RT_OS_WINDOWS
# include <iprt/win/windows.h>
#endif
#if !defined(RT_OS_WINDOWS) && !defined(RT_OS_OS2)
# include <sys/resource.h>
# include <fcntl.h>
# include <unistd.h>
#endif

#include <string.h>

#define VBOXGLTEST_WITH_LOGGING /** @todo r=andy Is this intentional? */

#ifdef VBOXGLTEST_WITH_LOGGING
# include "package-generated.h"

# include <iprt/log.h>
# include <iprt/param.h>
# include <iprt/time.h>
# include <iprt/system.h>
# include <iprt/process.h>
# include <iprt/env.h>

# include <VBox/log.h>
# include <VBox/version.h>
#endif /* VBOXGLTEST_WITH_LOGGING */

#ifndef RT_OS_WINDOWS
# include <GL/gl.h> /* For GLubyte and friends. */
#endif

#ifdef VBOX_WITH_VIDEOHWACCEL
# include <QApplication>
# if QT_VERSION < QT_VERSION_CHECK(6, 0, 0) && defined(RT_OS_WINDOWS)
#  include <QGLWidget> /* for GL headers on windows */
# endif
# include <VBox/VBoxGL2D.h>
#endif

/**
 * The OpenGL methods to look for when checking 3D presence.
 */
static const char * const g_apszOglMethods[] =
{
#ifdef RT_OS_WINDOWS
    "wglCreateContext",
    "wglDeleteContext",
    "wglMakeCurrent",
    "wglShareLists",
#elif defined(RT_OS_LINUX) || defined(RT_OS_FREEBSD) || defined(RT_OS_SOLARIS)
    "glXQueryVersion",
    "glXChooseVisual",
    "glXCreateContext",
    "glXMakeCurrent",
    "glXDestroyContext",
#endif
    "glAlphaFunc",
    "glBindTexture",
    "glBlendFunc",
    "glClear",
    "glClearColor",
    "glClearDepth",
    "glClearStencil",
    "glClipPlane",
    "glColorMask",
    "glColorPointer",
    "glCullFace",
    "glDeleteTextures",
    "glDepthFunc",
    "glDepthMask",
    "glDepthRange",
    "glDisable",
    "glDisableClientState",
    "glDrawArrays",
    "glDrawElements",
    "glEnable",
    "glEnableClientState",
    "glFogf",
    "glFogfv",
    "glFogi",
    "glFrontFace",
    "glGenTextures",
    "glGetBooleanv",
    "glGetError",
    "glGetFloatv",
    "glGetIntegerv",
    "glGetString",
    "glGetTexImage",
    "glLightModelfv",
    "glLightf",
    "glLightfv",
    "glLineWidth",
    "glLoadIdentity",
    "glLoadMatrixf",
    "glMaterialfv",
    "glMatrixMode",
    "glMultMatrixf",
    "glNormalPointer",
    "glPixelStorei",
    "glPointSize",
    "glPolygonMode",
    "glPolygonOffset",
    "glPopAttrib",
    "glPopMatrix",
    "glPushAttrib",
    "glPushMatrix",
    "glScissor",
    "glShadeModel",
    "glStencilFunc",
    "glStencilMask",
    "glStencilOp",
    "glTexCoordPointer",
    "glTexImage2D",
    "glTexParameterf",
    "glTexParameterfv",
    "glTexParameteri",
    "glTexSubImage2D",
    "glVertexPointer",
    "glViewport"
};


/**
 * Tries to resolve the given OpenGL symbol.
 *
 * @returns Pointer to the symbol or nULL on error.
 * @param   pszSymbol           The symbol to resolve.
 */
DECLINLINE(PFNRT) vboxTestOglGetProc(const char *pszSymbol)
{
    int vrc;

#ifdef RT_OS_WINDOWS
    static RTLDRMOD s_hOpenGL32 = NULL;
    if (s_hOpenGL32 == NULL)
    {
        vrc = RTLdrLoadSystem("opengl32", /* fNoUnload = */ true, &s_hOpenGL32);
        if (RT_FAILURE(vrc))
           s_hOpenGL32 = NULL;
    }

    typedef PROC (WINAPI *PFNWGLGETPROCADDRESS)(LPCSTR);
    static PFNWGLGETPROCADDRESS s_wglGetProcAddress = NULL;
    if (s_wglGetProcAddress == NULL)
    {
        if (s_hOpenGL32 != NULL)
        {
            vrc = RTLdrGetSymbol(s_hOpenGL32, "wglGetProcAddress", (void **)&s_wglGetProcAddress);
            if (RT_FAILURE(vrc))
               s_wglGetProcAddress = NULL;
        }
    }

    if (s_wglGetProcAddress)
    {
        /* Khronos: [on failure] "some implementations will return other values. 1, 2, and 3 are used, as well as -1". */
        PFNRT p = (PFNRT)s_wglGetProcAddress(pszSymbol);
        if (RT_VALID_PTR(p))
            return p;

        /* Might be an exported symbol. */
        vrc = RTLdrGetSymbol(s_hOpenGL32, pszSymbol, (void **)&p);
        if (RT_SUCCESS(vrc))
            return p;
    }
#else /* The X11 gang */
    static RTLDRMOD s_hGL = NULL;
    if (s_hGL == NULL)
    {
        static const char s_szLibGL[] = "libGL.so.1";
        vrc = RTLdrLoadEx(s_szLibGL, &s_hGL, RTLDRLOAD_FLAGS_GLOBAL | RTLDRLOAD_FLAGS_NO_UNLOAD, NULL);
        if (RT_FAILURE(vrc))
        {
            s_hGL = NULL;
            return NULL;
        }
    }

    typedef PFNRT (* PFNGLXGETPROCADDRESS)(const GLubyte * procName);
    static PFNGLXGETPROCADDRESS s_glXGetProcAddress = NULL;
    if (s_glXGetProcAddress == NULL)
    {
        vrc = RTLdrGetSymbol(s_hGL, "glXGetProcAddress", (void **)&s_glXGetProcAddress);
        if (RT_FAILURE(vrc))
        {
            s_glXGetProcAddress = NULL;
            return NULL;
        }
    }

    PFNRT p = s_glXGetProcAddress((const GLubyte *)pszSymbol);
    if (RT_VALID_PTR(p))
        return p;

    /* Might be an exported symbol. */
    vrc = RTLdrGetSymbol(s_hGL, pszSymbol, (void **)&p);
    if (RT_SUCCESS(vrc))
        return p;
#endif

    return NULL;
}

static int vboxCheck3DAccelerationSupported()
{
    LogRel(("Testing 3D Support:\n"));

    for (uint32_t i = 0; i < RT_ELEMENTS(g_apszOglMethods); i++)
    {
        PFNRT pfn = vboxTestOglGetProc(g_apszOglMethods[i]);
        if (!pfn)
        {
            LogRel(("Testing 3D Failed\n"));
            return 1;
        }
    }

    LogRel(("Testing 3D Succeeded!\n"));
    return 0;
}

#ifdef VBOX_WITH_VIDEOHWACCEL
static int vboxCheck2DVideoAccelerationSupported()
{
    LogRel(("Testing 2D Support:\n"));
    char *apszDummyArgs[] = { (char *)"GLTest", NULL };
    int   cDummyArgs = RT_ELEMENTS(apszDummyArgs) - 1;
    QApplication app(cDummyArgs, apszDummyArgs);

    VBoxGLTmpContext ctx;
    const MY_QOpenGLContext *pContext = ctx.makeCurrent();
    if (pContext)
    {
        VBoxVHWAInfo supportInfo;
        supportInfo.init(pContext);
        if (supportInfo.isVHWASupported())
        {
            LogRel(("Testing 2D Succeeded!\n"));
            return 0;
        }
    }
    else
    {
        LogRel(("Failed to create gl context\n"));
    }
    LogRel(("Testing 2D Failed\n"));
    return 1;
}
#endif

#ifdef VBOXGLTEST_WITH_LOGGING
static int vboxInitLogging(const char *pszFilename, bool bGenNameSuffix)
{
    PRTLOGGER loggerRelease;
    static const char * const s_apszGroups[] = VBOX_LOGGROUP_NAMES;
    RTUINT fFlags = RTLOGFLAGS_PREFIX_TIME_PROG;
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
    fFlags |= RTLOGFLAGS_USECRLF;
#endif
    const char * pszFilenameFmt;
    RTLOGDEST enmLogDest;
    if(pszFilename)
    {
        if(bGenNameSuffix)
            pszFilenameFmt = "%s.%ld.log";
        else
            pszFilenameFmt = "%s";
        enmLogDest = RTLOGDEST_FILE;
    }
    else
    {
        pszFilenameFmt = NULL;
        enmLogDest = RTLOGDEST_STDOUT;
    }


    int vrc = RTLogCreateEx(&loggerRelease, "VBOX_RELEASE_LOG", fFlags, "all", RT_ELEMENTS(s_apszGroups), s_apszGroups, UINT32_MAX,
                            0 /*cBufDescs*/, NULL /*paBufDescs*/, enmLogDest,
                            NULL /*pfnBeginEnd*/, 0 /*cHistory*/, 0 /*cbHistoryFileMax*/, 0 /*uHistoryTimeMax*/,
                            NULL /*pOutputIf*/, NULL /*pvOutputIfUser*/,
                            NULL /*pErrInfo*/, pszFilenameFmt, pszFilename, RTTimeMilliTS());
    if (RT_SUCCESS(vrc))
    {
        /* some introductory information */
        RTTIMESPEC timeSpec;
        char szTmp[256];
        RTTimeSpecToString(RTTimeNow(&timeSpec), szTmp, sizeof(szTmp));
        RTLogRelLogger(loggerRelease, 0, ~0U,
                       "VBoxTestGL %s r%u %s (%s %s) release log\n"
#ifdef VBOX_BLEEDING_EDGE
                       "EXPERIMENTAL build " VBOX_BLEEDING_EDGE "\n"
#endif
                       "Log opened %s\n",
                       VBOX_VERSION_STRING, RTBldCfgRevision(), VBOX_BUILD_TARGET,
                       __DATE__, __TIME__, szTmp);

        vrc = RTSystemQueryOSInfo(RTSYSOSINFO_PRODUCT, szTmp, sizeof(szTmp));
        if (RT_SUCCESS(vrc) || vrc == VERR_BUFFER_OVERFLOW)
            RTLogRelLogger(loggerRelease, 0, ~0U, "OS Product: %s\n", szTmp);
        vrc = RTSystemQueryOSInfo(RTSYSOSINFO_RELEASE, szTmp, sizeof(szTmp));
        if (RT_SUCCESS(vrc) || vrc == VERR_BUFFER_OVERFLOW)
            RTLogRelLogger(loggerRelease, 0, ~0U, "OS Release: %s\n", szTmp);
        vrc = RTSystemQueryOSInfo(RTSYSOSINFO_VERSION, szTmp, sizeof(szTmp));
        if (RT_SUCCESS(vrc) || vrc == VERR_BUFFER_OVERFLOW)
            RTLogRelLogger(loggerRelease, 0, ~0U, "OS Version: %s\n", szTmp);
        vrc = RTSystemQueryOSInfo(RTSYSOSINFO_SERVICE_PACK, szTmp, sizeof(szTmp));
        if (RT_SUCCESS(vrc) || vrc == VERR_BUFFER_OVERFLOW)
            RTLogRelLogger(loggerRelease, 0, ~0U, "OS Service Pack: %s\n", szTmp);
//        RTLogRelLogger(loggerRelease, 0, ~0U, "Host RAM: %uMB RAM, available: %uMB\n",
//                       uHostRamMb, uHostRamAvailMb);
        /* the package type is interesting for Linux distributions */
        char szExecName[RTPATH_MAX];
        char *pszExecName = RTProcGetExecutablePath(szExecName, sizeof(szExecName));
        RTLogRelLogger(loggerRelease, 0, ~0U,
                       "Executable: %s\n"
                       "Process ID: %u\n"
                       "Package type: %s"
#ifdef VBOX_OSE
                       " (OSE)"
#endif
                       "\n",
                       pszExecName ? pszExecName : "unknown",
                       RTProcSelf(),
                       VBOX_PACKAGE_STRING);

        /* register this logger as the release logger */
        RTLogRelSetDefaultInstance(loggerRelease);

        return VINF_SUCCESS;
    }

    return vrc;
}
#endif

static int vboxInitQuietMode()
{
#if !defined(RT_OS_WINDOWS) && !defined(RT_OS_OS2)
    /* This small test application might crash on some hosts. Do never
     * generate a core dump as most likely some OpenGL library is
     * responsible. */
    struct rlimit lim = { 0, 0 };
    setrlimit(RLIMIT_CORE, &lim);

    /* Redirect stderr to /dev/null */
    int fd = open("/dev/null", O_WRONLY);
    if (fd != -1)
        dup2(fd, STDERR_FILENO);
#endif
    return 0;
}

int main(int argc, char **argv)
{
    RTR3InitExe(argc, &argv, 0);

    int vrc = 0;
    if (argc < 2)
    {
        /* backwards compatibility: check 3D */
        vrc = vboxCheck3DAccelerationSupported();
    }
    else
    {
        static const RTGETOPTDEF s_aOptionDefs[] =
        {
            { "--test",           't',   RTGETOPT_REQ_STRING },
            { "-test",            't',   RTGETOPT_REQ_STRING },
#ifdef VBOXGLTEST_WITH_LOGGING
            { "--log",            'l',   RTGETOPT_REQ_STRING },
            { "--log-to-stdout",  'L',   RTGETOPT_REQ_NOTHING },
#endif
        };

        RTGETOPTSTATE State;
        vrc = RTGetOptInit(&State, argc, argv, &s_aOptionDefs[0], RT_ELEMENTS(s_aOptionDefs), 1, 0);
        AssertRCReturn(vrc, 49);

#ifdef VBOX_WITH_VIDEOHWACCEL
        bool fTest2D = false;
#endif
        bool fTest3D = false;
#ifdef VBOXGLTEST_WITH_LOGGING
        bool fLog = false;
        bool fLogSuffix = false;
        const char * pLog = NULL;
#endif

        for (;;)
        {
            RTGETOPTUNION Val;
            vrc = RTGetOpt(&State, &Val);
            if (!vrc)
                break;
            switch (vrc)
            {
                case 't':
                    if (!strcmp(Val.psz, "3D") || !strcmp(Val.psz, "3d"))
                        fTest3D = true;
#ifdef VBOX_WITH_VIDEOHWACCEL
                    else if (!strcmp(Val.psz, "2D") || !strcmp(Val.psz, "2d"))
                        fTest2D = true;
#endif
                    else
                        return RTMsgErrorExit(RTEXITCODE_FAILURE, "Unknown test: %s", Val.psz);
                    break;

#ifdef VBOXGLTEST_WITH_LOGGING
                case 'l':
                    fLog = true;
                    pLog = Val.psz;
                    break;
                case 'L':
                    fLog = true;
                    pLog = NULL;
                    break;
#endif
                case 'h':
                    RTPrintf(VBOX_PRODUCT " Helper for testing 2D/3D OpenGL capabilities %u.%u.%u\n"
                             "Copyright (C) 2009-" VBOX_C_YEAR " " VBOX_VENDOR "\n"
                             "\n"
                             "Parameters:\n"
#ifdef VBOX_WITH_VIDEOHWACCEL
                             "  --test 2D             test for 2D (video) OpenGL capabilities\n"
#endif
                             "  --test 3D             test for 3D OpenGL capabilities\n"
#ifdef VBOXGLTEST_WITH_LOGGING
                             "  --log <log_file_name> log the GL test result to the given file\n"
                             "  --log-to-stdout       log the GL test result to stdout\n"
                             "\n"
                             "Logging can alternatively be enabled by specifying the VBOXGLTEST_LOG=<log_file_name> env variable\n"

#endif
                             "\n",
                            RTBldCfgVersionMajor(), RTBldCfgVersionMinor(), RTBldCfgVersionBuild());
                    return RTEXITCODE_SUCCESS;

                case 'V':
                    RTPrintf("$Revision: 155484 $\n");
                    return RTEXITCODE_SUCCESS;

                case VERR_GETOPT_UNKNOWN_OPTION:
                case VINF_GETOPT_NOT_OPTION:
                default:
                    return RTGetOptPrintError(vrc, &Val);
            }
        }

        /*
         * Init logging and output.
         */
#ifdef VBOXGLTEST_WITH_LOGGING
        if (!fLog)
        {
            /* check the VBOXGLTEST_LOG env var */
            pLog = RTEnvGet("VBOXGLTEST_LOG");
            if(pLog)
                fLog = true;
            fLogSuffix = true;
        }
        if (fLog)
            vrc = vboxInitLogging(pLog, fLogSuffix);
        else
#endif
            vrc = vboxInitQuietMode();

        /*
         * Do the job.
         */
        if (!vrc && fTest3D)
            vrc = vboxCheck3DAccelerationSupported();

#ifdef VBOX_WITH_VIDEOHWACCEL
        if (!vrc && fTest2D)
            vrc = vboxCheck2DVideoAccelerationSupported();
#endif
    }

    /*RTR3Term();*/
    return vrc;

}

#ifdef RT_OS_WINDOWS
extern "C" int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    RT_NOREF(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
    return main(__argc, __argv);
}
#endif

