/*
 * quartz.h
 *
 * External interface of the Quartz display modes seen by the generic, mode
 * independent parts of the Darwin X server.
 */
/*
 * Copyright (c) 2001-2003 Greg Parker and Torrey T. Lyons.
 *                 All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */
/* $XdotOrg: xc/programs/Xserver/hw/darwin/quartz/quartz.h,v 1.3 2004/07/30 19:12:17 torrey Exp $ */
/* $XFree86: xc/programs/Xserver/hw/darwin/quartz/quartz.h,v 1.7 2003/11/12 20:21:51 torrey Exp $ */

#ifndef _QUARTZ_H
#define _QUARTZ_H

#include "quartzPasteboard.h"

#include "screenint.h"
#include "window.h"

/*------------------------------------------
   Quartz display mode function types
  ------------------------------------------*/

/*
 * Display mode initialization
 */
typedef void (*DisplayInitProc)(void);
typedef Bool (*AddScreenProc)(int index, ScreenPtr pScreen);
typedef Bool (*SetupScreenProc)(int index, ScreenPtr pScreen);
typedef void (*InitInputProc)(int argc, char **argv);

/*
 * Cursor functions
 */
typedef Bool (*InitCursorProc)(ScreenPtr pScreen);
typedef void (*CursorUpdateProc)(void);

/*
 * Suspend and resume X11 activity
 */
typedef void (*SuspendScreenProc)(ScreenPtr pScreen);
typedef void (*ResumeScreenProc)(ScreenPtr pScreen, int x, int y);
typedef void (*CaptureScreensProc)(void);
typedef void (*ReleaseScreensProc)(void);

/*
 * Screen state change support
 */
typedef void (*ScreenChangedProc)(void);
typedef void (*AddPseudoramiXScreensProc)(int *x, int *y, int *width, int *height);
typedef void (*UpdateScreenProc)(ScreenPtr pScreen);

/*
 * Rootless helper functions
 */
typedef Bool (*IsX11WindowProc)(void *nsWindow, int windowNumber);
typedef void (*HideWindowsProc)(Bool hide);

/*
 * Rootless functions for optional export to GLX layer
 */
typedef void * (*FrameForWindowProc)(WindowPtr pWin, Bool create);
typedef WindowPtr (*TopLevelParentProc)(WindowPtr pWindow);
typedef Bool (*CreateSurfaceProc)
    (ScreenPtr pScreen, Drawable id, DrawablePtr pDrawable,
     unsigned int client_id, unsigned int *surface_id,
     unsigned int key[2], void (*notify) (void *arg, void *data),
     void *notify_data);
typedef Bool (*DestroySurfaceProc)
    (ScreenPtr pScreen, Drawable id, DrawablePtr pDrawable,
     void (*notify) (void *arg, void *data), void *notify_data);

/*
 * Quartz display mode function list
 */
typedef struct _QuartzModeProcs {
    DisplayInitProc DisplayInit;
    AddScreenProc AddScreen;
    SetupScreenProc SetupScreen;
    InitInputProc InitInput;

    InitCursorProc InitCursor;
    CursorUpdateProc CursorUpdate;	// Not used if NULL

    SuspendScreenProc SuspendScreen;
    ResumeScreenProc ResumeScreen;
    CaptureScreensProc CaptureScreens;	// Only called in fullscreen
    ReleaseScreensProc ReleaseScreens;	// Only called in fullscreen

    ScreenChangedProc ScreenChanged;
    AddPseudoramiXScreensProc AddPseudoramiXScreens;
    UpdateScreenProc UpdateScreen;

    IsX11WindowProc IsX11Window;
    HideWindowsProc HideWindows;

    FrameForWindowProc FrameForWindow;
    TopLevelParentProc TopLevelParent;
    CreateSurfaceProc CreateSurface;
    DestroySurfaceProc DestroySurface;
} QuartzModeProcsRec, *QuartzModeProcsPtr;

extern QuartzModeProcsPtr quartzProcs;

Bool QuartzLoadDisplayBundle(const char *dpyBundleName);

#endif
