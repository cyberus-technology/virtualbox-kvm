/*
 * Copyright © 2004 Eric Anholt
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Eric Anholt not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Eric Anholt makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * ERIC ANHOLT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ERIC ANHOLT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $Header: /cvs/xorg/xserver/xorg/miext/cw/cw.h,v 1.14 2005/12/09 18:32:46 ajax Exp $ */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "gcstruct.h"
#include "picturestr.h"

/*
 * One of these structures is allocated per GC that gets used with a window with
 * backing pixmap.
 */

typedef struct {
    GCPtr	    pBackingGC;	    /* Copy of the GC but with graphicsExposures
				     * set FALSE and the clientClip set to
				     * clip output to the valid regions of the
				     * backing pixmap. */
    unsigned long   serialNumber;   /* clientClip computed time */
    unsigned long   stateChanges;   /* changes in parent gc since last copy */
    GCOps	    *wrapOps;	    /* wrapped ops */
    GCFuncs	    *wrapFuncs;	    /* wrapped funcs */
} cwGCRec, *cwGCPtr;

extern int cwGCIndex;

#define getCwGC(pGC)	((cwGCPtr)(pGC)->devPrivates[cwGCIndex].ptr)
#define setCwGC(pGC,p)	((pGC)->devPrivates[cwGCIndex].ptr = (pointer) (p))

/*
 * One of these structures is allocated per Picture that gets used with a
 * window with a backing pixmap
 */

typedef struct {
    PicturePtr	    pBackingPicture;
    unsigned long   serialNumber;
    unsigned long   stateChanges;
} cwPictureRec, *cwPicturePtr;

#define getCwPicture(pPicture)	((cwPicturePtr)(pPicture)->devPrivates[cwPictureIndex].ptr)
#define setCwPicture(pPicture,p) ((pPicture)->devPrivates[cwPictureIndex].ptr = (pointer) (p))

extern int  cwPictureIndex;

extern int cwWindowIndex;

#define cwWindowPrivate(pWindow)    ((pWindow)->devPrivates[cwWindowIndex].ptr)
#define getCwPixmap(pWindow)	    ((PixmapPtr) cwWindowPrivate(pWindow))
#define setCwPixmap(pWindow,pPixmap) (cwWindowPrivate(pWindow) = (pointer) (pPixmap))

#define cwDrawableIsRedirWindow(pDraw)					\
	((pDraw)->type == DRAWABLE_WINDOW &&				\
	 getCwPixmap((WindowPtr) (pDraw)) != NULL)

typedef struct {
    /*
     * screen func wrappers
     */
    CloseScreenProcPtr		CloseScreen;
    GetImageProcPtr		GetImage;
    GetSpansProcPtr		GetSpans;
    CreateGCProcPtr		CreateGC;

    PaintWindowBackgroundProcPtr PaintWindowBackground;
    PaintWindowBorderProcPtr	PaintWindowBorder;
    CopyWindowProcPtr		CopyWindow;

    GetWindowPixmapProcPtr	GetWindowPixmap;
    SetWindowPixmapProcPtr	SetWindowPixmap;
    
#ifdef RENDER
    DestroyPictureProcPtr	DestroyPicture;
    ChangePictureClipProcPtr	ChangePictureClip;
    DestroyPictureClipProcPtr	DestroyPictureClip;
    
    ChangePictureProcPtr	ChangePicture;
    ValidatePictureProcPtr	ValidatePicture;

    CompositeProcPtr		Composite;
    GlyphsProcPtr		Glyphs;
    CompositeRectsProcPtr	CompositeRects;

    TrapezoidsProcPtr		Trapezoids;
    TrianglesProcPtr		Triangles;
    TriStripProcPtr		TriStrip;
    TriFanProcPtr		TriFan;

    RasterizeTrapezoidProcPtr	RasterizeTrapezoid;
#endif
} cwScreenRec, *cwScreenPtr;

extern int cwScreenIndex;

#define getCwScreen(pScreen)	((cwScreenPtr)(pScreen)->devPrivates[cwScreenIndex].ptr)
#define setCwScreen(pScreen,p)	((cwScreenPtr)(pScreen)->devPrivates[cwScreenIndex].ptr = (p))

#define CW_OFFSET_XYPOINTS(ppt, npt) do { \
    DDXPointPtr _ppt = (DDXPointPtr)(ppt); \
    int _i; \
    for (_i = 0; _i < npt; _i++) { \
	_ppt[_i].x += dst_off_x; \
	_ppt[_i].y += dst_off_y; \
    } \
} while (0)

#define CW_OFFSET_RECTS(prect, nrect) do { \
    int _i; \
    for (_i = 0; _i < nrect; _i++) { \
	(prect)[_i].x += dst_off_x; \
	(prect)[_i].y += dst_off_y; \
    } \
} while (0)

#define CW_OFFSET_ARCS(parc, narc) do { \
    int _i; \
    for (_i = 0; _i < narc; _i++) { \
	(parc)[_i].x += dst_off_x; \
	(parc)[_i].y += dst_off_y; \
    } \
} while (0)

#define CW_OFFSET_XY_DST(x, y) do { \
    (x) = (x) + dst_off_x; \
    (y) = (y) + dst_off_y; \
} while (0)

#define CW_OFFSET_XY_SRC(x, y) do { \
    (x) = (x) + src_off_x; \
    (y) = (y) + src_off_y; \
} while (0)

/* cw.c */
DrawablePtr
cwGetBackingDrawable(DrawablePtr pDrawable, int *x_off, int *y_off);

/* cw_render.c */

void
cwInitializeRender (ScreenPtr pScreen);

void
cwFiniRender (ScreenPtr pScreen);

/* cw.c */

void
miInitializeCompositeWrapper(ScreenPtr pScreen);

/* Must be called before miInitializeCompositeWrapper */
void
miDisableCompositeWrapper(ScreenPtr pScreen);
