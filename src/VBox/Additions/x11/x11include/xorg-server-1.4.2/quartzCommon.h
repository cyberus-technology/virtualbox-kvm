/*
 * quartzCommon.h
 *
 * Common definitions used internally by all Quartz modes
 *
 * This file should be included before any X11 or IOKit headers
 * so that it can avoid symbol conflicts.
 *
 * Copyright (c) 2001-2004 Torrey T. Lyons and Greg Parker.
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

#ifndef _QUARTZCOMMON_H
#define _QUARTZCOMMON_H

// QuickDraw in ApplicationServices has the following conflicts with
// the basic X server headers. Use QD_<name> to use the QuickDraw
// definition of any of these symbols, or the normal name for the
// X11 definition.
#define Cursor       QD_Cursor
#define WindowPtr    QD_WindowPtr
#define Picture      QD_Picture
#include <ApplicationServices/ApplicationServices.h>
#undef Cursor
#undef WindowPtr
#undef Picture

// Quartz specific per screen storage structure
typedef struct {
    // List of CoreGraphics displays that this X11 screen covers.
    // This is more than one CG display for video mirroring and
    // rootless PseudoramiX mode.
    // No CG display will be covered by more than one X11 screen.
    int displayCount;
    CGDirectDisplayID *displayIDs;
} QuartzScreenRec, *QuartzScreenPtr;

#define QUARTZ_PRIV(pScreen) \
    ((QuartzScreenPtr)pScreen->devPrivates[quartzScreenIndex].ptr)

// Data stored at startup for Cocoa front end
extern int              quartzEventWriteFD;
extern int              quartzStartClients;

// User preferences used by Quartz modes
extern int              quartzRootless;
extern int              quartzUseSysBeep;
extern int              quartzUseAGL;
extern int              quartzEnableKeyEquivalents;

// Other shared data
extern int              quartzServerVisible;
extern int              quartzServerQuitting;
extern int              quartzScreenIndex;
extern int              aquaMenuBarHeight;

// Name of GLX bundle for native OpenGL
extern const char      *quartzOpenGLBundle;

void QuartzReadPreferences(void);
void QuartzMessageMainThread(unsigned msg, void *data, unsigned length);
void QuartzMessageServerThread(int type, int argc, ...);
void QuartzSetWindowMenu(int nitems, const char **items,
                         const char *shortcuts);
void QuartzFSCapture(void);
void QuartzFSRelease(void);
int  QuartzFSUseQDCursor(int depth);
void QuartzBlockHandler(void *blockData, void *pTimeout, void *pReadmask);
void QuartzWakeupHandler(void *blockData, int result, void *pReadmask);

// Messages that can be sent to the main thread.
enum {
    kQuartzServerHidden,
    kQuartzServerStarted,
    kQuartzServerDied,
    kQuartzCursorUpdate,
    kQuartzPostEvent,
    kQuartzSetWindowMenu,
    kQuartzSetWindowMenuCheck,
    kQuartzSetFrontProcess,
    kQuartzSetCanQuit
};

#endif  /* _QUARTZCOMMON_H */
