/*
 * $XFree86: xc/programs/Xserver/miext/layer/layerstr.h,v 1.2 2001/06/04 09:45:41 keithp Exp $
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

#ifndef _LAYERSTR_H_
#define _LAYERSTR_H_

#include    <X11/X.h>
#include    "scrnintstr.h"
#include    "windowstr.h"
#include    <X11/fonts/font.h>
#include    "dixfontstr.h"
#include    <X11/fonts/fontstruct.h>
#include    "mi.h"
#include    "regionstr.h"
#include    "globals.h"
#include    "gcstruct.h"
#include    "layer.h"
#ifdef RENDER
#include    "picturestr.h"
#endif

extern int layerScrPrivateIndex;
extern int layerGCPrivateIndex;
extern int layerWinPrivateIndex;

/*
 * One of these for each possible set of underlying
 * rendering code.  The first kind always points at the
 * underlying frame buffer code and is created in LayerStartInit
 * so that LayerNewKind can unwrap the screen and prepare it
 * for another wrapping sequence.
 *
 * The set of functions wrapped here must be at least the union
 * of all functions wrapped by any rendering layer in use; they're
 * easy to add, so don't be shy
 */

typedef struct _LayerKind {
    int				kind;			/* kind index */

    CloseScreenProcPtr		CloseScreen;
    
    CreateWindowProcPtr		CreateWindow;
    DestroyWindowProcPtr	DestroyWindow;
    ChangeWindowAttributesProcPtr ChangeWindowAttributes;
    PaintWindowBackgroundProcPtr PaintWindowBackground;
    PaintWindowBorderProcPtr	PaintWindowBorder;
    CopyWindowProcPtr		CopyWindow;
    
    CreatePixmapProcPtr		CreatePixmap;
    DestroyPixmapProcPtr	DestroyPixmap;

    CreateGCProcPtr		CreateGC;
#ifdef RENDER
    CompositeProcPtr		Composite;
    GlyphsProcPtr		Glyphs;
    CompositeRectsProcPtr	CompositeRects;
#endif
} LayerKindRec;

#define LayerWrap(orig,lay,member,func) \
    (((lay)->member = (orig)->member),\
     ((orig)->member = (func)))
#define LayerUnwrap(orig,lay,member) \
    ((orig)->member = (lay)->member)

/*
 * This is the window private structure allocated for
 * all windows.  There are two possible alternatives here,
 * either the window belongs to a single layer and uses its
 * internal clip/borderClip lists or the window belongs to one
 * or more layers and uses a separate clip/borderclip for each
 * layer.  When this is integrated into the core window struct,
 * the LayerWinKind can become a single bit saving 8 bytes per
 * window.
 */

typedef struct _LayerWin {
    Bool		isList;
    union {
	LayerPtr	pLayer;
	LayerListPtr	pLayList;
    } u;
} LayerWinRec;

typedef struct _LayerList {
    LayerListPtr    pNext;	    /* list of layers for this window */
    LayerPtr	    pLayer;	    /* the layer */
    Bool	    inheritClip;    /* use the window clipList/borderClip */
    RegionRec	    clipList;	    /* per-layer clip/border clip lists */
    RegionRec	    borderClip;
} LayerListRec;

#define layerGetWinPriv(pWin)	    ((LayerWinPtr) (pWin)->devPrivates[layerWinPrivateIndex].ptr)
#define layerWinPriv(pWin)	    LayerWinPtr	pLayWin = layerGetWinPriv(pWin)

#define layerWinLayer(pLayWin)	    ((pLayWin)->isList ? (pLayWin)->u.pLayList->pLayer : (pLayWin)->u.pLayer)

typedef struct _LayerWinLoop {
    LayerWinPtr	    pLayWin;
    LayerListPtr    pLayList;
    PixmapPtr	    pPixmap;	    /* original window pixmap */
    RegionRec	    clipList;	    /* saved original clipList contents */
    RegionRec	    borderClip;	    /* saved original borderClip contents */
} LayerWinLoopRec, *LayerWinLoopPtr;

#define layerWinFirstLayer(pLayWin,pLayList) ((pLayWin)->isList ? ((pLayList) = (pLayWin)->u.pLayList)->pLayer : pLayWin->u.pLayer)
#define layerWinNextLayer(pLayWin,pLayList) ((pLayWin)->isList ? ((pLayList) = (pLayList)->pNext)->pLayer : 0)
					      
LayerPtr
LayerWindowFirst (WindowPtr pWin, LayerWinLoopPtr pLoop);

LayerPtr
LayerWindowNext (WindowPtr pWin, LayerWinLoopPtr pLoop);

void
LayerWindowDone (WindowPtr pWin, LayerWinLoopPtr pLoop);


/*
 * This is the GC private structure allocated for all GCs.
 * XXX this is really messed up; I'm not sure how to fix it yet
 */

typedef struct _LayerGC {
    GCFuncs	    *funcs;
    LayerKindPtr    pKind;
} LayerGCRec;

#define layerGetGCPriv(pGC)	    ((LayerGCPtr) (pGC)->devPrivates[layerGCPrivateIndex].ptr)
#define layerGCPriv(pGC)	    LayerGCPtr pLayGC = layerGetGCPriv(pGC)

/*
 * This is the screen private, it contains
 * the layer kinds and the layers themselves
 */
typedef struct _LayerScreen {
    int		    nkinds;	    /* number of elements in kinds array */
    LayerKindPtr    kinds;	    /* created kinds; reallocated when new ones added */
    LayerPtr	    pLayers;	    /* list of layers for this screen */
} LayerScreenRec;

#define layerGetScrPriv(pScreen)    ((LayerScreenPtr) (pScreen)->devPrivates[layerScrPrivateIndex].ptr)
#define layerScrPriv(pScreen)	    LayerScreenPtr  pLayScr = layerGetScrPriv(pScreen)

Bool
layerCloseScreen (int index, ScreenPtr pScreen);

Bool
layerCreateWindow (WindowPtr pWin);

Bool
layerDestroyWindow (WindowPtr pWin);

Bool
layerChangeWindowAttributes (WindowPtr pWin, unsigned long mask);

void
layerPaintWindowBackground (WindowPtr pWin, RegionPtr pRegion, int what);

void
layerPaintWindowBorder (WindowPtr pWin, RegionPtr pRegion, int what);

void
layerCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc);

PixmapPtr
layerCreatePixmap (ScreenPtr pScreen, int width, int height, int depth);

Bool
layerDestroyPixmap (PixmapPtr pPixmap);

Bool
layerCreateGC (GCPtr pGC);

#ifdef RENDER
void
layerComposite (CARD8      op,
		PicturePtr pSrc,
		PicturePtr pMask,
		PicturePtr pDst,
		INT16      xSrc,
		INT16      ySrc,
		INT16      xMask,
		INT16      yMask,
		INT16      xDst,
		INT16      yDst,
		CARD16     width,
		CARD16     height);
void
layerGlyphs (CARD8	    op,
	     PicturePtr	    pSrc,
	     PicturePtr	    pDst,
	     PictFormatPtr  maskFormat,
	     INT16	    xSrc,
	     INT16	    ySrc,
	     int	    nlist,
	     GlyphListPtr   list,
	     GlyphPtr	    *glyphs);

void
layerCompositeRects (CARD8	    op,
		     PicturePtr	    pDst,
		     xRenderColor   *color,
		     int	    nRect,
		     xRectangle	    *rects);
#endif
void layerValidateGC(GCPtr, unsigned long, DrawablePtr);
void layerChangeGC(GCPtr, unsigned long);
void layerCopyGC(GCPtr, unsigned long, GCPtr);
void layerDestroyGC(GCPtr);
void layerChangeClip(GCPtr, int, pointer, int);
void layerDestroyClip(GCPtr);
void layerCopyClip(GCPtr, GCPtr);

void
layerFillSpans(DrawablePtr  pDraw,
	       GC	    *pGC,
	       int	    nInit,	
	       DDXPointPtr  pptInit,	
	       int	    *pwidthInit,		
	       int	    fSorted);

void
layerSetSpans(DrawablePtr	pDraw,
	      GCPtr		pGC,
	      char		*pcharsrc,
	      DDXPointPtr 	pptInit,
	      int		*pwidthInit,
	      int		nspans,
	      int		fSorted);

void
layerPutImage(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		depth, 
    int x, int y, int w, int h,
    int		leftPad,
    int		format,
    char 	*pImage 
);

RegionPtr
layerCopyArea(
    DrawablePtr pSrc,
    DrawablePtr pDst,
    GC *pGC,
    int srcx, int srcy,
    int width, int height,
    int dstx, int dsty 
);

RegionPtr
layerCopyPlane(
    DrawablePtr	pSrc,
    DrawablePtr	pDst,
    GCPtr pGC,
    int	srcx, int srcy,
    int	width, int height,
    int	dstx, int dsty,
    unsigned long bitPlane 
);

void
layerPolyPoint(
    DrawablePtr pDraw,
    GCPtr pGC,
    int mode,
    int npt,
    xPoint *pptInit 
);
void
layerPolylines(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		mode,		
    int		npt,		
    DDXPointPtr pptInit 
);

void 
layerPolySegment(
    DrawablePtr	pDraw,
    GCPtr	pGC,
    int		nseg,
    xSegment	*pSeg 
);

void
layerPolyRectangle(
    DrawablePtr  pDraw,
    GCPtr        pGC,
    int	         nRects,
    xRectangle  *pRects 
);

void
layerPolyArc(
    DrawablePtr	pDraw,
    GCPtr	pGC,
    int		narcs,
    xArc	*parcs 
);

void
layerFillPolygon(
    DrawablePtr	pDraw,
    GCPtr	pGC,
    int		shape,
    int		mode,
    int		count,
    DDXPointPtr	pptInit 
);

void 
layerPolyFillRect(
    DrawablePtr	pDraw,
    GCPtr	pGC,
    int		nRectsInit, 
    xRectangle	*pRectsInit 
);

void
layerPolyFillArc(
    DrawablePtr	pDraw,
    GCPtr	pGC,
    int		narcs,
    xArc	*parcs 
);

int
layerPolyText8(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		x, 
    int 	y,
    int 	count,
    char	*chars 
);

int
layerPolyText16(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		x,
    int		y,
    int 	count,
    unsigned short *chars 
);

void
layerImageText8(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		x, 
    int		y,
    int 	count,
    char	*chars 
);

void
layerImageText16(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		x,
    int		y,
    int 	count,
    unsigned short *chars 
);

void
layerImageGlyphBlt(
    DrawablePtr pDraw,
    GCPtr pGC,
    int x, int y,
    unsigned int nglyph,
    CharInfoPtr *ppci,
    pointer pglyphBase 
);

void
layerPolyGlyphBlt(
    DrawablePtr pDraw,
    GCPtr pGC,
    int x, int y,
    unsigned int nglyph,
    CharInfoPtr *ppci,
    pointer pglyphBase 
);

void
layerPushPixels(
    GCPtr	pGC,
    PixmapPtr	pBitMap,
    DrawablePtr pDraw,
    int	dx, int dy, int xOrg, int yOrg 
);

#endif /* _LAYERSTR_H_ */
