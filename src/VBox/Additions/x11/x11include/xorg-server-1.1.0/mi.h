/* $Xorg: mi.h,v 1.4 2001/02/09 02:05:20 xorgcvs Exp $ */
/***********************************************************

Copyright 1987, 1998  The Open Group

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


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $XFree86: xc/programs/Xserver/mi/mi.h,v 3.9 2001/08/06 20:51:16 dawes Exp $ */

#ifndef MI_H
#define MI_H
#include <X11/X.h>
#include "region.h"
#include "validate.h"
#include "window.h"
#include "gc.h"
#include <X11/fonts/font.h>
#include "input.h"
#include "cursor.h"

#define MiBits	CARD32

typedef struct _miDash *miDashPtr;
#define EVEN_DASH	0
#define ODD_DASH	~0

/* miarc.c */

extern void miPolyArc(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
);

/* mibitblt.c */

extern RegionPtr miCopyArea(
    DrawablePtr /*pSrcDrawable*/,
    DrawablePtr /*pDstDrawable*/,
    GCPtr /*pGC*/,
    int /*xIn*/,
    int /*yIn*/,
    int /*widthSrc*/,
    int /*heightSrc*/,
    int /*xOut*/,
    int /*yOut*/
);

extern void miOpqStipDrawable(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    RegionPtr /*prgnSrc*/,
    MiBits * /*pbits*/,
    int /*srcx*/,
    int /*w*/,
    int /*h*/,
    int /*dstx*/,
    int /*dsty*/
);

extern RegionPtr miCopyPlane(
    DrawablePtr /*pSrcDrawable*/,
    DrawablePtr /*pDstDrawable*/,
    GCPtr /*pGC*/,
    int /*srcx*/,
    int /*srcy*/,
    int /*width*/,
    int /*height*/,
    int /*dstx*/,
    int /*dsty*/,
    unsigned long /*bitPlane*/
);

extern void miGetImage(
    DrawablePtr /*pDraw*/,
    int /*sx*/,
    int /*sy*/,
    int /*w*/,
    int /*h*/,
    unsigned int /*format*/,
    unsigned long /*planeMask*/,
    char * /*pdstLine*/
);

extern void miPutImage(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*depth*/,
    int /*x*/,
    int /*y*/,
    int /*w*/,
    int /*h*/,
    int /*leftPad*/,
    int /*format*/,
    char * /*pImage*/
);

/* micursor.c */

extern void miRecolorCursor(
    ScreenPtr /*pScr*/,
    CursorPtr /*pCurs*/,
    Bool /*displayed*/
);

/* midash.c */

extern miDashPtr miDashLine(
    int /*npt*/,
    DDXPointPtr /*ppt*/,
    unsigned int /*nDash*/,
    unsigned char * /*pDash*/,
    unsigned int /*offset*/,
    int * /*pnseg*/
);

extern void miStepDash(
    int /*dist*/,
    int * /*pDashIndex*/,
    unsigned char * /*pDash*/,
    int /*numInDashList*/,
    int * /*pDashOffset*/
);

/* mieq.c */


#ifndef INPUT_H
typedef struct _DeviceRec *DevicePtr;
#endif

extern Bool mieqInit(
    DevicePtr /*pKbd*/,
    DevicePtr /*pPtr*/
);

extern void mieqEnqueue(
    xEventPtr /*e*/
);

extern void mieqSwitchScreen(
    ScreenPtr /*pScreen*/,
    Bool /*fromDIX*/
);

extern void mieqProcessInputEvents(
    void
);

/* miexpose.c */

extern RegionPtr miHandleExposures(
    DrawablePtr /*pSrcDrawable*/,
    DrawablePtr /*pDstDrawable*/,
    GCPtr /*pGC*/,
    int /*srcx*/,
    int /*srcy*/,
    int /*width*/,
    int /*height*/,
    int /*dstx*/,
    int /*dsty*/,
    unsigned long /*plane*/
);

extern void miSendGraphicsExpose(
    ClientPtr /*client*/,
    RegionPtr /*pRgn*/,
    XID /*drawable*/,
    int /*major*/,
    int /*minor*/
);

extern void miSendExposures(
    WindowPtr /*pWin*/,
    RegionPtr /*pRgn*/,
    int /*dx*/,
    int /*dy*/
);

extern void miWindowExposures(
    WindowPtr /*pWin*/,
    RegionPtr /*prgn*/,
    RegionPtr /*other_exposed*/
);

extern void miPaintWindow(
    WindowPtr /*pWin*/,
    RegionPtr /*prgn*/,
    int /*what*/
);

extern void miClearDrawable(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/
);

/* mifillrct.c */

extern void miPolyFillRect(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nrectFill*/,
    xRectangle * /*prectInit*/
);

/* miglblt.c */

extern void miPolyGlyphBlt(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
);

extern void miImageGlyphBlt(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
);

/* mipoly.c */

extern void miFillPolygon(
    DrawablePtr /*dst*/,
    GCPtr /*pgc*/,
    int /*shape*/,
    int /*mode*/,
    int /*count*/,
    DDXPointPtr /*pPts*/
);

/* mipolycon.c */

extern Bool miFillConvexPoly(
    DrawablePtr /*dst*/,
    GCPtr /*pgc*/,
    int /*count*/,
    DDXPointPtr /*ptsIn*/
);

/* mipolygen.c */

extern Bool miFillGeneralPoly(
    DrawablePtr /*dst*/,
    GCPtr /*pgc*/,
    int /*count*/,
    DDXPointPtr /*ptsIn*/
);

/* mipolypnt.c */

extern void miPolyPoint(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    xPoint * /*pptInit*/
);

/* mipolyrect.c */

extern void miPolyRectangle(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*nrects*/,
    xRectangle * /*pRects*/
);

/* mipolyseg.c */

extern void miPolySegment(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSegs*/
);

/* mipolytext.c */

extern int miPolyText(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    int /*count*/,
    char * /*chars*/,
    FontEncoding /*fontEncoding*/
);

extern int miPolyText8(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    int /*count*/,
    char * /*chars*/
);

extern int miPolyText16(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    int /*count*/,
    unsigned short * /*chars*/
);

extern int miImageText(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    int /*count*/,
    char * /*chars*/,
    FontEncoding /*fontEncoding*/
);

extern void miImageText8(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    int /*count*/,
    char * /*chars*/
);

extern void miImageText16(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    int /*count*/,
    unsigned short * /*chars*/
);

/* mipushpxl.c */

extern void miPushPixels(
    GCPtr /*pGC*/,
    PixmapPtr /*pBitMap*/,
    DrawablePtr /*pDrawable*/,
    int /*dx*/,
    int /*dy*/,
    int /*xOrg*/,
    int /*yOrg*/
);

/* miregion.c */

/* see also region.h */

extern Bool miRectAlloc(
    RegionPtr /*pRgn*/,
    int /*n*/
);

extern void miSetExtents(
    RegionPtr /*pReg*/
);

extern int miFindMaxBand(
    RegionPtr /*prgn*/
);

#ifdef DEBUG
extern Bool miValidRegion(
    RegionPtr /*prgn*/
);
#endif

extern Bool miRegionDataCopy(RegionPtr dst, RegionPtr src);
extern Bool miRegionBroken(RegionPtr pReg);

/* miscrinit.c */

extern Bool miModifyPixmapHeader(
    PixmapPtr /*pPixmap*/,
    int /*width*/,
    int /*height*/,
    int /*depth*/,
    int /*bitsPerPixel*/,
    int /*devKind*/,
    pointer /*pPixData*/
);

extern Bool miCloseScreen(
    int /*index*/,
    ScreenPtr /*pScreen*/
);

extern Bool miCreateScreenResources(
    ScreenPtr /*pScreen*/
);

extern Bool miScreenDevPrivateInit(
    ScreenPtr /*pScreen*/,
    int /*width*/,
    pointer /*pbits*/
);

extern Bool miScreenInit(
    ScreenPtr /*pScreen*/,
    pointer /*pbits*/,
    int /*xsize*/,
    int /*ysize*/,
    int /*dpix*/,
    int /*dpiy*/,
    int /*width*/,
    int /*rootDepth*/,
    int /*numDepths*/,
    DepthPtr /*depths*/,
    VisualID /*rootVisual*/,
    int /*numVisuals*/,
    VisualPtr /*visuals*/
);

extern int miAllocateGCPrivateIndex(
    void
);

extern PixmapPtr miGetScreenPixmap(
    ScreenPtr pScreen
);

extern void miSetScreenPixmap(
    PixmapPtr pPix
);

/* mivaltree.c */

extern int miShapedWindowIn(
    ScreenPtr /*pScreen*/,
    RegionPtr /*universe*/,
    RegionPtr /*bounding*/,
    BoxPtr /*rect*/,
    int /*x*/,
    int /*y*/
);

typedef void 
(*SetRedirectBorderClipProcPtr) (WindowPtr pWindow, RegionPtr pRegion);

typedef RegionPtr
(*GetRedirectBorderClipProcPtr) (WindowPtr pWindow);

void
miRegisterRedirectBorderClipProc (SetRedirectBorderClipProcPtr setBorderClip,
				  GetRedirectBorderClipProcPtr getBorderClip);

extern int miValidateTree(
    WindowPtr /*pParent*/,
    WindowPtr /*pChild*/,
    VTKind /*kind*/
);

extern void miWideLine(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pPts*/
);

extern void miWideDash(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pPts*/
);

extern void miMiter(
    void
);

extern void miNotMiter(
    void
);

/* miwindow.c */

extern void miClearToBackground(
    WindowPtr /*pWin*/,
    int /*x*/,
    int /*y*/,
    int /*w*/,
    int /*h*/,
    Bool /*generateExposures*/
);

extern Bool miChangeSaveUnder(
    WindowPtr /*pWin*/,
    WindowPtr /*first*/
);

extern void miPostChangeSaveUnder(
    WindowPtr /*pWin*/,
    WindowPtr /*pFirst*/
);

extern void miMarkWindow(
    WindowPtr /*pWin*/
);

extern Bool miMarkOverlappedWindows(
    WindowPtr /*pWin*/,
    WindowPtr /*pFirst*/,
    WindowPtr * /*ppLayerWin*/
);

extern void miHandleValidateExposures(
    WindowPtr /*pWin*/
);

extern void miMoveWindow(
    WindowPtr /*pWin*/,
    int /*x*/,
    int /*y*/,
    WindowPtr /*pNextSib*/,
    VTKind /*kind*/
);

extern void miSlideAndSizeWindow(
    WindowPtr /*pWin*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*w*/,
    unsigned int /*h*/,
    WindowPtr /*pSib*/
);

extern WindowPtr miGetLayerWindow(
    WindowPtr /*pWin*/
);

extern void miSetShape(
    WindowPtr /*pWin*/
);

extern void miChangeBorderWidth(
    WindowPtr /*pWin*/,
    unsigned int /*width*/
);

extern void miMarkUnrealizedWindow(
    WindowPtr /*pChild*/,
    WindowPtr /*pWin*/,
    Bool /*fromConfigure*/
);

extern void miSegregateChildren(WindowPtr pWin, RegionPtr pReg, int depth);

/* mizerarc.c */

extern void miZeroPolyArc(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
);

/* mizerline.c */

extern void miZeroLine(
    DrawablePtr /*dst*/,
    GCPtr /*pgc*/,
    int /*mode*/,
    int /*nptInit*/,
    DDXPointRec * /*pptInit*/
);

extern void miZeroDashLine(
    DrawablePtr /*dst*/,
    GCPtr /*pgc*/,
    int /*mode*/,
    int /*nptInit*/,
    DDXPointRec * /*pptInit*/
);

extern void miPolyFillArc(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
);

#endif /* MI_H */
