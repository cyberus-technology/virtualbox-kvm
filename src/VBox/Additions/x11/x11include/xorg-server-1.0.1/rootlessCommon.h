/*
 * Common internal rootless definitions and code
 */
/*
 * Copyright (c) 2001 Greg Parker. All Rights Reserved.
 * Copyright (c) 2002-2004 Torrey T. Lyons. All Rights Reserved.
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
/* $XFree86: xc/programs/Xserver/miext/rootless/rootlessCommon.h,v 1.5 2004/07/02 01:30:33 torrey Exp $ */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _ROOTLESSCOMMON_H
#define _ROOTLESSCOMMON_H

#include "rootless.h"
#include "fb.h"

#ifdef RENDER
#include "picturestr.h"
#endif


// Debug output, or not.
#ifdef ROOTLESSDEBUG
#define RL_DEBUG_MSG ErrorF
#else
#define RL_DEBUG_MSG(a, ...)
#endif


// Global variables
extern int rootlessGCPrivateIndex;
extern int rootlessScreenPrivateIndex;
extern int rootlessWindowPrivateIndex;


// RootlessGCRec: private per-gc data
typedef struct {
    GCFuncs *originalFuncs;
    GCOps *originalOps;
} RootlessGCRec;


// RootlessScreenRec: per-screen private data
typedef struct _RootlessScreenRec {
    // Rootless implementation functions
    RootlessFrameProcsPtr imp;

    // Wrapped screen functions
    CreateScreenResourcesProcPtr CreateScreenResources;
    CloseScreenProcPtr CloseScreen;

    CreateWindowProcPtr CreateWindow;
    DestroyWindowProcPtr DestroyWindow;
    RealizeWindowProcPtr RealizeWindow;
    UnrealizeWindowProcPtr UnrealizeWindow;
    MoveWindowProcPtr MoveWindow;
    ResizeWindowProcPtr ResizeWindow;
    RestackWindowProcPtr RestackWindow;
    ReparentWindowProcPtr ReparentWindow;
    ChangeBorderWidthProcPtr ChangeBorderWidth;
    PositionWindowProcPtr PositionWindow;
    ChangeWindowAttributesProcPtr ChangeWindowAttributes;

    CreateGCProcPtr CreateGC;
    PaintWindowBackgroundProcPtr PaintWindowBackground;
    PaintWindowBorderProcPtr PaintWindowBorder;
    CopyWindowProcPtr CopyWindow;
    GetImageProcPtr GetImage;
    SourceValidateProcPtr SourceValidate;

    MarkOverlappedWindowsProcPtr MarkOverlappedWindows;
    ValidateTreeProcPtr ValidateTree;

#ifdef SHAPE
    SetShapeProcPtr SetShape;
#endif

#ifdef RENDER
    CompositeProcPtr Composite;
    GlyphsProcPtr Glyphs;
#endif

    void *pixmap_data;
    unsigned int pixmap_data_size;

    void *redisplay_timer;
    unsigned int redisplay_timer_set :1;
    unsigned int redisplay_queued :1;
    unsigned int redisplay_expired :1;
} RootlessScreenRec, *RootlessScreenPtr;


#undef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#undef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))

// "Definition of the Porting Layer for the X11 Sample Server" says
// unwrap and rewrap of screen functions is unnecessary, but
// screen->CreateGC changes after a call to cfbCreateGC.

#define SCREEN_UNWRAP(screen, fn) \
    screen->fn = SCREENREC(screen)->fn;

#define SCREEN_WRAP(screen, fn) \
    SCREENREC(screen)->fn = screen->fn; \
    screen->fn = Rootless##fn


// Accessors for screen and window privates

#define SCREENREC(pScreen) \
   ((RootlessScreenRec *)(pScreen)->devPrivates[rootlessScreenPrivateIndex].ptr)

#define WINREC(pWin) \
    ((RootlessWindowRec *)(pWin)->devPrivates[rootlessWindowPrivateIndex].ptr)


// Call a rootless implementation function.
// Many rootless implementation functions are allowed to be NULL.
#define CallFrameProc(pScreen, proc, params)            \
    if (SCREENREC(pScreen)->frameProcs.proc) {          \
        RL_DEBUG_MSG("calling frame proc " #proc " ");  \
        SCREENREC(pScreen)->frameProcs.proc params;     \
    }


// BoxRec manipulators
// Copied from shadowfb

#define TRIM_BOX(box, pGC) { \
    BoxPtr extents = &pGC->pCompositeClip->extents;\
    if(box.x1 < extents->x1) box.x1 = extents->x1; \
    if(box.x2 > extents->x2) box.x2 = extents->x2; \
    if(box.y1 < extents->y1) box.y1 = extents->y1; \
    if(box.y2 > extents->y2) box.y2 = extents->y2; \
}

#define TRANSLATE_BOX(box, pDraw) { \
    box.x1 += pDraw->x; \
    box.x2 += pDraw->x; \
    box.y1 += pDraw->y; \
    box.y2 += pDraw->y; \
}

#define TRIM_AND_TRANSLATE_BOX(box, pDraw, pGC) { \
    TRANSLATE_BOX(box, pDraw); \
    TRIM_BOX(box, pGC); \
}

#define BOX_NOT_EMPTY(box) \
    (((box.x2 - box.x1) > 0) && ((box.y2 - box.y1) > 0))


// HUGE_ROOT and NORMAL_ROOT
// We don't want to clip windows to the edge of the screen.
// HUGE_ROOT temporarily makes the root window really big.
// This is needed as a wrapper around any function that calls
// SetWinSize or SetBorderSize which clip a window against its
// parents, including the root.

extern RegionRec rootlessHugeRoot;

#define HUGE_ROOT(pWin)                         \
    do {                                        \
        WindowPtr w = pWin;                     \
        while (w->parent)                       \
            w = w->parent;                      \
        saveRoot = w->winSize;                  \
        w->winSize = rootlessHugeRoot;          \
    } while (0)

#define NORMAL_ROOT(pWin)                       \
    do {                                        \
        WindowPtr w = pWin;                     \
        while (w->parent)                       \
            w = w->parent;                      \
        w->winSize = saveRoot;                  \
    } while (0)


// Returns TRUE if this window is a top-level window (i.e. child of the root)
// The root is not a top-level window.
#define IsTopLevel(pWin) \
    ((pWin)  &&  (pWin)->parent  &&  !(pWin)->parent->parent)

// Returns TRUE if this window is a root window
#define IsRoot(pWin) \
    ((pWin) == WindowTable[(pWin)->drawable.pScreen->myNum])


/*
 * SetPixmapBaseToScreen
 *  Move the given pixmap's base address to where pixel (0, 0)
 *  would be if the pixmap's actual data started at (x, y).
 *  Can't access the bits before the first word of the drawable's data in
 *  rootless mode, so make sure our base address is always 32-bit aligned.
 */
#define SetPixmapBaseToScreen(pix, _x, _y) {                                \
    PixmapPtr   _pPix = (PixmapPtr) (pix);                                  \
    _pPix->devPrivate.ptr = (char *) (_pPix->devPrivate.ptr) -              \
                            ((int)(_x) * _pPix->drawable.bitsPerPixel/8 +   \
                             (int)(_y) * _pPix->devKind);                   \
    if (_pPix->drawable.bitsPerPixel != FB_UNIT) {                          \
        unsigned _diff = ((unsigned) _pPix->devPrivate.ptr) &               \
                         (FB_UNIT / CHAR_BIT - 1);                          \
        _pPix->devPrivate.ptr = (char *) (_pPix->devPrivate.ptr) -          \
                                _diff;                                      \
        _pPix->drawable.x = _diff /                                         \
                            (_pPix->drawable.bitsPerPixel / CHAR_BIT);      \
    }                                                                       \
}


// Returns TRUE if this window is visible inside a frame
// (e.g. it is visible and has a top-level or root parent)
Bool IsFramedWindow(WindowPtr pWin);

// Routines that cause regions to get redrawn.
// DamageRegion and DamageRect are in global coordinates.
// DamageBox is in window-local coordinates.
void RootlessDamageRegion(WindowPtr pWindow, RegionPtr pRegion);
void RootlessDamageRect(WindowPtr pWindow, int x, int y, int w, int h);
void RootlessDamageBox(WindowPtr pWindow, BoxPtr pBox);
void RootlessRedisplay(WindowPtr pWindow);
void RootlessRedisplayScreen(ScreenPtr pScreen);

void RootlessQueueRedisplay(ScreenPtr pScreen);

// Move a window to its proper location on the screen.
void RootlessRepositionWindow(WindowPtr pWin);

// Move the window to it's correct place in the physical stacking order.
void RootlessReorderWindow(WindowPtr pWin);

#endif /* _ROOTLESSCOMMON_H */
