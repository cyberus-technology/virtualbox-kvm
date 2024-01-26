/*
 * $XFree86: xc/programs/Xserver/miext/layer/layer.h,v 1.4 2001/08/01 00:44:58 tsi Exp $
 *
 * Copyright © 2001 Keith Packard, member of The XFree86 Project, Inc.
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

#ifndef _LAYER_H_
#define _LAYER_H_

#include <shadow.h>

#define LAYER_FB	0
#define LAYER_SHADOW	1

typedef struct _LayerKind   *LayerKindPtr;
typedef struct _LayerWin    *LayerWinPtr;
typedef struct _LayerList   *LayerListPtr;
typedef struct _LayerGC	    *LayerGCPtr;
typedef struct _Layer	    *LayerPtr;
typedef struct _LayerScreen *LayerScreenPtr;

/*
 * We'll try to work without a list of windows in each layer
 * for now, this will make computing bounding boxes for each
 * layer rather expensive, so that may need to change at some point.
 */

#define LAYER_SCREEN_PIXMAP ((PixmapPtr) 1)

typedef struct _Layer {
    LayerPtr		pNext;	    /* a list of all layers for this screen */
    LayerKindPtr	pKind;	    /* characteristics of this layer */
    int			refcnt;	    /* reference count, layer is freed when zero */
    int			windows;    /* number of windows, free pixmap when zero */
    int			depth;	    /* window depth in this layer */
    PixmapPtr		pPixmap;    /* pixmap for this layer (may be frame buffer) */
    Bool		freePixmap; /* whether to free this pixmap when done */
    RegionRec		region;	    /* valid set of pPixmap for drawing */
    ShadowUpdateProc	update;	    /* for shadow layers, update/window/closure values */
    ShadowWindowProc	window;
    int			randr;
    void		*closure;
} LayerRec;

/*
 * Call this before wrapping stuff for acceleration, it
 * gives layer pointers to the raw frame buffer functions
 */

Bool
LayerStartInit (ScreenPtr pScreen);

/*
 * Initialize wrappers for each acceleration type and
 * call this function, it will move the needed functions
 * into a new LayerKind and replace them with the generic
 * functions.
 */

int
LayerNewKind (ScreenPtr pScreen);

/*
 * Finally, call this function and layer
 * will wrap the screen functions and prepare for execution
 */

Bool
LayerFinishInit (ScreenPtr pScreen);

/*
 * At any point after LayerStartInit, a new layer can be created.
 */
LayerPtr
LayerCreate (ScreenPtr		pScreen, 
	     int		kind, 
	     int		depth,
	     PixmapPtr		pPixmap,
	     ShadowUpdateProc	update,
	     ShadowWindowProc	window,
	     int		randr,
	     void		*closure);

/*
 * Create a layer pixmap
 */
Bool
LayerCreatePixmap (ScreenPtr pScreen, LayerPtr pLayer);

/*
 * Change a layer pixmap
 */
void
LayerSetPixmap (ScreenPtr pScreen, LayerPtr pLayer, PixmapPtr pPixmap);

/*
 * Destroy a layer pixmap
 */
void
LayerDestroyPixmap (ScreenPtr pScreen, LayerPtr pLayer);

/*
 * Change a layer kind
 */
void
LayerSetKind (ScreenPtr pScreen, LayerPtr pLayer, int kind);

/*
 * Destroy a layer.  The layer must not contain any windows.
 */
void
LayerDestroy (ScreenPtr pScreen, LayerPtr layer);

/*
 * Add a window to a layer
 */
Bool
LayerWindowAdd (ScreenPtr pScreen, LayerPtr pLayer, WindowPtr pWin);

/*
 * Remove a window from a layer
 */

void
LayerWindowRemove (ScreenPtr pScreen, LayerPtr pLayer, WindowPtr pWin);

#endif /* _LAYER_H_ */
