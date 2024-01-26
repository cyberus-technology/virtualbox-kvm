/*
 * mispritest.h
 *
 * mi sprite structures
 */


/*

Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.
*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _MISPRITEST_H_
#define _MISPRITEST_H_

# include   "misprite.h"
#ifdef RENDER
# include   "picturestr.h"
#endif
# include   "damage.h"

typedef struct {
    CursorPtr	    pCursor;
    int		    x;			/* cursor hotspot */
    int		    y;
    BoxRec	    saved;		/* saved area from the screen */
    Bool	    isUp;		/* cursor in frame buffer */
    Bool	    shouldBeUp;		/* cursor should be displayed */
    WindowPtr	    pCacheWin;		/* window the cursor last seen in */
    Bool	    isInCacheWin;
    Bool	    checkPixels;	/* check colormap collision */
    ScreenPtr       pScreen;
} miCursorInfoRec, *miCursorInfoPtr;

/*
 * per screen information
 */

typedef struct {
    /* screen procedures */
    CloseScreenProcPtr			CloseScreen;
    GetImageProcPtr			GetImage;
    GetSpansProcPtr			GetSpans;
    SourceValidateProcPtr		SourceValidate;
    
    /* window procedures */
    CopyWindowProcPtr			CopyWindow;
    
    /* colormap procedures */
    InstallColormapProcPtr		InstallColormap;
    StoreColorsProcPtr			StoreColors;
    
    /* os layer procedures */
    ScreenBlockHandlerProcPtr		BlockHandler;
    
    /* device cursor procedures */
    DeviceCursorInitializeProcPtr       DeviceCursorInitialize;
    DeviceCursorCleanupProcPtr          DeviceCursorCleanup;

    xColorItem	    colors[2];
    ColormapPtr     pInstalledMap;
    ColormapPtr     pColormap;
    VisualPtr	    pVisual;
    miSpriteCursorFuncPtr    funcs;
    DamagePtr	    pDamage;		/* damage tracking structure */
} miSpriteScreenRec, *miSpriteScreenPtr;

#define SOURCE_COLOR	0
#define MASK_COLOR	1

/*
 * Overlap BoxPtr and Box elements
 */
#define BOX_OVERLAP(pCbox,X1,Y1,X2,Y2) \
 	(((pCbox)->x1 <= (X2)) && ((X1) <= (pCbox)->x2) && \
	 ((pCbox)->y1 <= (Y2)) && ((Y1) <= (pCbox)->y2))

/*
 * Overlap BoxPtr, origins, and rectangle
 */
#define ORG_OVERLAP(pCbox,xorg,yorg,x,y,w,h) \
    BOX_OVERLAP((pCbox),(x)+(xorg),(y)+(yorg),(x)+(xorg)+(w),(y)+(yorg)+(h))

/*
 * Overlap BoxPtr, origins and RectPtr
 */
#define ORGRECT_OVERLAP(pCbox,xorg,yorg,pRect) \
    ORG_OVERLAP((pCbox),(xorg),(yorg),(pRect)->x,(pRect)->y, \
		(int)((pRect)->width), (int)((pRect)->height))
/*
 * Overlap BoxPtr and horizontal span
 */
#define SPN_OVERLAP(pCbox,y,x,w) BOX_OVERLAP((pCbox),(x),(y),(x)+(w),(y))

#define LINE_SORT(x1,y1,x2,y2) \
{ int _t; \
  if (x1 > x2) { _t = x1; x1 = x2; x2 = _t; } \
  if (y1 > y2) { _t = y1; y1 = y2; y2 = _t; } }

#define LINE_OVERLAP(pCbox,x1,y1,x2,y2,lw2) \
    BOX_OVERLAP((pCbox), (x1)-(lw2), (y1)-(lw2), (x2)+(lw2), (y2)+(lw2))

#endif /* _MISPRITEST_H_ */
