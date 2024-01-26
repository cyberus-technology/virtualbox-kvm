/*
 * $Id$
 *
 * Copyright © 2006 Sun Microsystems
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Sun Microsystems not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Sun Microsystems makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * SUN MICROSYSTEMS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL SUN MICROSYSTEMS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Copyright © 2003 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _COMPINT_H_
#define _COMPINT_H_

#include "misc.h"
#include "scrnintstr.h"
#include "os.h"
#include "regionstr.h"
#include "validate.h"
#include "windowstr.h"
#include "input.h"
#include "resource.h"
#include "colormapst.h"
#include "cursorstr.h"
#include "dixstruct.h"
#include "gcstruct.h"
#include "servermd.h"
#include "dixevents.h"
#include "globals.h"
#include "picturestr.h"
#include "extnsionst.h"
#include "mi.h"
#include "damage.h"
#include "damageextint.h"
#include "xfixes.h"
#include <X11/extensions/compositeproto.h>
#include <assert.h>

/*
 *  enable this for debugging
 
    #define COMPOSITE_DEBUG
 */

typedef struct _CompClientWindow {
    struct _CompClientWindow	*next;
    XID				id;
    int				update;
}  CompClientWindowRec, *CompClientWindowPtr;

typedef struct _CompWindow {
    RegionRec		    borderClip;
    DamagePtr		    damage;	/* for automatic update mode */
    Bool		    damageRegistered;
    Bool		    damaged;
    int			    update;
    CompClientWindowPtr	    clients;
    int			    oldx;
    int			    oldy;
    PixmapPtr		    pOldPixmap;
    int			    borderClipX, borderClipY;
} CompWindowRec, *CompWindowPtr;

#define COMP_ORIGIN_INVALID	    0x80000000

typedef struct _CompSubwindows {
    int			    update;
    CompClientWindowPtr	    clients;
} CompSubwindowsRec, *CompSubwindowsPtr;

#ifndef COMP_INCLUDE_RGB24_VISUAL
#define COMP_INCLUDE_RGB24_VISUAL 0
#endif

typedef struct _CompOverlayClientRec *CompOverlayClientPtr;

typedef struct _CompOverlayClientRec {
    CompOverlayClientPtr pNext;  
    ClientPtr            pClient;
    ScreenPtr            pScreen;
    XID			 resource;
} CompOverlayClientRec;

typedef struct _CompScreen {
    PositionWindowProcPtr	PositionWindow;
    CopyWindowProcPtr		CopyWindow;
    CreateWindowProcPtr		CreateWindow;
    DestroyWindowProcPtr	DestroyWindow;
    RealizeWindowProcPtr	RealizeWindow;
    UnrealizeWindowProcPtr	UnrealizeWindow;
    PaintWindowProcPtr		PaintWindowBackground;
    ClipNotifyProcPtr		ClipNotify;
    /*
     * Called from ConfigureWindow, these
     * three track changes to the offscreen storage
     * geometry
     */
    MoveWindowProcPtr		MoveWindow;
    ResizeWindowProcPtr		ResizeWindow;
    ChangeBorderWidthProcPtr	ChangeBorderWidth;
    /*
     * Reparenting has an effect on Subwindows redirect
     */
    ReparentWindowProcPtr	ReparentWindow;
    
    /*
     * Colormaps for new visuals better not get installed
     */
    InstallColormapProcPtr	InstallColormap;

    ScreenBlockHandlerProcPtr	BlockHandler;
    CloseScreenProcPtr		CloseScreen;
    Bool			damaged;
    int				numAlternateVisuals;
    VisualID			*alternateVisuals;

    WindowPtr                   pOverlayWin;
    CompOverlayClientPtr        pOverlayClients;
    
} CompScreenRec, *CompScreenPtr;

extern int  CompScreenPrivateIndex;
extern int  CompWindowPrivateIndex;
extern int  CompSubwindowsPrivateIndex;

#define GetCompScreen(s) ((CompScreenPtr) ((s)->devPrivates[CompScreenPrivateIndex].ptr))
#define GetCompWindow(w) ((CompWindowPtr) ((w)->devPrivates[CompWindowPrivateIndex].ptr))
#define GetCompSubwindows(w) ((CompSubwindowsPtr) ((w)->devPrivates[CompSubwindowsPrivateIndex].ptr))

extern RESTYPE		CompositeClientWindowType;
extern RESTYPE		CompositeClientSubwindowsType;

/*
 * compalloc.c
 */

void
compReportDamage (DamagePtr pDamage, RegionPtr pRegion, void *closure);

Bool
compRedirectWindow (ClientPtr pClient, WindowPtr pWin, int update);

void
compFreeClientWindow (WindowPtr pWin, XID id);

int
compUnredirectWindow (ClientPtr pClient, WindowPtr pWin, int update);

int
compRedirectSubwindows (ClientPtr pClient, WindowPtr pWin, int update);

void
compFreeClientSubwindows (WindowPtr pWin, XID id);

int
compUnredirectSubwindows (ClientPtr pClient, WindowPtr pWin, int update);

int
compRedirectOneSubwindow (WindowPtr pParent, WindowPtr pWin);

int
compUnredirectOneSubwindow (WindowPtr pParent, WindowPtr pWin);

Bool
compAllocPixmap (WindowPtr pWin);

void
compFreePixmap (WindowPtr pWin);

Bool
compReallocPixmap (WindowPtr pWin, int x, int y,
		   unsigned int w, unsigned int h, int bw);

/*
 * compext.c
 */

void
CompositeExtensionInit (void);

/*
 * compinit.c
 */

Bool
CompositeRegisterAlternateVisuals (ScreenPtr pScreen,
				   VisualID *vids, int nVisuals);

Bool
compScreenInit (ScreenPtr pScreen);

/*
 * compwindow.c
 */

#ifdef COMPOSITE_DEBUG
void
compCheckTree (ScreenPtr pScreen);
#else
#define compCheckTree(s)
#endif

void
compSetPixmap (WindowPtr pWin, PixmapPtr pPixmap);

Bool
compCheckRedirect (WindowPtr pWin);

Bool
compPositionWindow (WindowPtr pWin, int x, int y);

Bool
compRealizeWindow (WindowPtr pWin);

Bool
compUnrealizeWindow (WindowPtr pWin);

void
compPaintWindowBackground (WindowPtr pWin, RegionPtr pRegion, int what);

void
compClipNotify (WindowPtr pWin, int dx, int dy);

void
compMoveWindow (WindowPtr pWin, int x, int y, WindowPtr pSib, VTKind kind);

void
compResizeWindow (WindowPtr pWin, int x, int y,
		  unsigned int w, unsigned int h, WindowPtr pSib);

void
compChangeBorderWidth (WindowPtr pWin, unsigned int border_width);

void
compReparentWindow (WindowPtr pWin, WindowPtr pPriorParent);

Bool
compCreateWindow (WindowPtr pWin);

Bool
compDestroyWindow (WindowPtr pWin);

void
compSetRedirectBorderClip (WindowPtr pWin, RegionPtr pRegion);

RegionPtr
compGetRedirectBorderClip (WindowPtr pWin);

void
compCopyWindow (WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc);

void
compWindowUpdate (WindowPtr pWin);

void
deleteCompOverlayClientsForScreen (ScreenPtr pScreen);

int
ProcCompositeGetOverlayWindow (ClientPtr client);

int
ProcCompositeReleaseOverlayWindow (ClientPtr client);

int
SProcCompositeGetOverlayWindow (ClientPtr client);

int
SProcCompositeReleaseOverlayWindow (ClientPtr client);

WindowPtr
CompositeRealChildHead (WindowPtr pWin);

int
DeleteWindowNoInputDevices(pointer value, XID wid);

#endif /* _COMPINT_H_ */
