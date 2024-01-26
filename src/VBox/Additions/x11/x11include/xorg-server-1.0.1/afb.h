/* $XFree86: xc/programs/Xserver/afb/afb.h,v 3.10 2003/10/29 22:15:19 tsi Exp $ */
/* Combined Purdue/PurduePlus patches, level 2.0, 1/17/89 */
/***********************************************************

Copyright (c) 1987  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


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
/* $XConsortium: afb.h,v 5.31 94/04/17 20:28:15 dpw Exp $ */
/* Monochrome Frame Buffer definitions
   written by drewry, september 1986
*/

#include "pixmap.h"
#include "region.h"
#include "gc.h"
#include "colormap.h"
#include "regionstr.h"
#include "mibstore.h"
#include "mfb.h"

extern int afbInverseAlu[];
extern int afbScreenPrivateIndex;
/* warning: PixelType definition duplicated in maskbits.h */
#ifndef PixelType
#define PixelType CARD32
#endif /* PixelType */

#define AFB_MAX_DEPTH 8

/* afbbitblt.c */

extern void afbDoBitblt(
	DrawablePtr /*pSrc*/,
	DrawablePtr /*pDst*/,
	int /*alu*/,
	RegionPtr /*prgnDst*/,
	DDXPointPtr /*pptSrc*/,
	unsigned long /*planemask*/
);

extern RegionPtr afbBitBlt(
	DrawablePtr /*pSrc*/,
	DrawablePtr /*pDst*/,
	GCPtr /*pGC*/,
	int /*srcx*/,
	int /*srcy*/,
	int /*width*/,
	int /*height*/,
	int /*dstx*/,
	int /*dsty*/,
	void (*doBitBlt)(
		DrawablePtr /*pSrc*/,
		DrawablePtr /*pDst*/,
		int /*alu*/,
		RegionPtr /*prgnDst*/,
		DDXPointPtr /*pptSrc*/,
		unsigned long /*planemask*/
        ),
	unsigned long /*planemask*/
);

extern RegionPtr afbCopyArea(
	DrawablePtr /*pSrcDrawable*/,
	DrawablePtr /*pDstDrawable*/,
	GCPtr/*pGC*/,
	int /*srcx*/,
	int /*srcy*/,
	int /*width*/,
	int /*height*/,
	int /*dstx*/,
	int /*dsty*/
);

extern RegionPtr afbCopyPlane(
	DrawablePtr /*pSrcDrawable*/,
	DrawablePtr /*pDstDrawable*/,
	GCPtr/*pGC*/,
	int /*srcx*/,
	int /*srcy*/,
	int /*width*/,
	int /*height*/,
	int /*dstx*/,
	int /*dsty*/,
	unsigned long /*plane*/
);

extern void afbCopy1ToN(
	DrawablePtr /*pSrc*/,
	DrawablePtr /*pDst*/,
	int /*alu*/,
	RegionPtr /*prgnDst*/,
	DDXPointPtr /*pptSrc*/,
	unsigned long /*planemask*/
);
/* afbbltC.c */

extern void afbDoBitbltCopy(
	DrawablePtr /*pSrc*/,
	DrawablePtr /*pDst*/,
	int /*alu*/,
	RegionPtr /*prgnDst*/,
	DDXPointPtr /*pptSrc*/,
	unsigned long /*planemask*/
);
/* afbbltCI.c */

extern void afbDoBitbltCopyInverted(
	DrawablePtr /*pSrc*/,
	DrawablePtr /*pDst*/,
	int /*alu*/,
	RegionPtr /*prgnDst*/,
	DDXPointPtr /*pptSrc*/,
	unsigned long /*planemask*/
);
/* afbbltG.c */

extern void afbDoBitbltGeneral(
	DrawablePtr /*pSrc*/,
	DrawablePtr /*pDst*/,
	int /*alu*/,
	RegionPtr /*prgnDst*/,
	DDXPointPtr /*pptSrc*/,
	unsigned long /*planemask*/
);
/* afbbltO.c */

extern void afbDoBitbltOr(
	DrawablePtr /*pSrc*/,
	DrawablePtr /*pDst*/,
	int /*alu*/,
	RegionPtr /*prgnDst*/,
	DDXPointPtr /*pptSrc*/,
	unsigned long /*planemask*/
);
/* afbbltX.c */

extern void afbDoBitbltXor(
	DrawablePtr /*pSrc*/,
	DrawablePtr /*pDst*/,
	int /*alu*/,
	RegionPtr /*prgnDst*/,
	DDXPointPtr /*pptSrc*/,
	unsigned long /*planemask*/
);
/* afbbres.c */

extern void afbBresS(
	PixelType * /*addrl*/,
	int /*nlwidth*/,
	int /*sizeDst*/,
	int /*depthDst*/,
	int /*signdx*/,
	int /*signdy*/,
	int /*axis*/,
	int /*x1*/,
	int /*y1*/,
	int /*e*/,
	int /*e1*/,
	int /*e2*/,
	int /*len*/,
	unsigned char * /*rrops*/
);
/* afbbresd.c */

extern void afbBresD(
	int * /*pdashIndex*/,
	unsigned char * /*pDash*/,
	int /*numInDashList*/,
	int * /*pdashOffset*/,
	int /*isDoubleDash*/,
	PixelType * /*addrl*/,
	int /*nlwidth*/,
	int /*sizeDst*/,
	int /*depthDst*/,
	int /*signdx*/,
	int /*signdy*/,
	int /*axis*/,
	int /*x1*/,
	int /*y1*/,
	int /*e*/,
	int /*e1*/,
	int /*e2*/,
	int /*len*/,
	unsigned char * /*rrops*/,
	unsigned char * /*bgrrops*/
);
/* afbbstore.c */

extern void afbSaveAreas(
	PixmapPtr /*pPixmap*/,
	RegionPtr /*prgnSave*/,
	int /*xorg*/,
	int /*yorg*/,
	WindowPtr /*pWin*/
);

extern void afbRestoreAreas(
	PixmapPtr /*pPixmap*/,
	RegionPtr /*prgnRestore*/,
	int /*xorg*/,
	int /*yorg*/,
	WindowPtr /*pWin*/
);
/* afbclip.c */

extern RegionPtr afbPixmapToRegion(
	PixmapPtr /*pPix*/
);

/* afbcmap.c */

extern int afbListInstalledColormaps(
	ScreenPtr /*pScreen*/,
	Colormap * /*pmaps*/
);

extern void afbInstallColormap(
	ColormapPtr /*pmap*/
);

extern void afbUninstallColormap(
	ColormapPtr /*pmap*/
);

extern void afbResolveColor(
	unsigned short * /*pred*/,
	unsigned short * /*pgreen*/,
	unsigned short * /*pblue*/,
	VisualPtr /*pVisual*/
);

extern Bool afbInitializeColormap(
	ColormapPtr /*pmap*/
);

extern int afbExpandDirectColors(
	ColormapPtr /*pmap*/,
	int /*ndefs*/,
	xColorItem * /*indefs*/,
	xColorItem * /*outdefs*/
);

extern Bool afbCreateDefColormap(
	ScreenPtr /*pScreen*/
);

extern Bool afbSetVisualTypes(
	int /*depth*/,
	int /*visuals*/,
	int /*bitsPerRGB*/
);

extern Bool afbInitVisuals(
	VisualPtr * /*visualp*/,
	DepthPtr * /*depthp*/,
	int * /*nvisualp*/,
	int * /*ndepthp*/,
	int * /*rootDepthp*/,
	VisualID * /*defaultVisp*/,
	unsigned long /*sizes*/,
	int /*bitsPerRGB*/
);

/* afbfillarc.c */

extern void afbPolyFillArcSolid(
	DrawablePtr /*pDraw*/,
	GCPtr /*pGC*/,
	int /*narcs*/,
	xArc * /*parcs*/
);
/* afbfillrct.c */

extern void afbPolyFillRect(
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	int /*nrectFill*/,
	xRectangle * /*prectInit*/
);

/* afbply1rct.c */
extern void afbFillPolygonSolid(
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	int /*mode*/,
	int /*shape*/,
	int /*count*/,
	DDXPointPtr /*ptsIn*/
);

/* afbfillsp.c */

extern void afbSolidFS(
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	int /*nInit*/,
	DDXPointPtr /*pptInit*/,
	int * /*pwidthInit*/,
	int /*fSorted*/
);

extern void afbStippleFS(
	DrawablePtr /*pDrawable*/,
	GCPtr/*pGC*/,
	int /*nInit*/,
	DDXPointPtr /*pptInit*/,
	int * /*pwidthInit*/,
	int /*fSorted*/
);

extern void afbTileFS(
	DrawablePtr /*pDrawable*/,
	GCPtr/*pGC*/,
	int /*nInit*/,
	DDXPointPtr /*pptInit*/,
	int * /*pwidthInit*/,
	int /*fSorted*/
);

extern void afbUnnaturalTileFS(
	DrawablePtr /*pDrawable*/,
	GCPtr/*pGC*/,
	int /*nInit*/,
	DDXPointPtr /*pptInit*/,
	int * /*pwidthInit*/,
	int /*fSorted*/
);

extern void afbUnnaturalStippleFS(
	DrawablePtr /*pDrawable*/,
	GCPtr/*pGC*/,
	int /*nInit*/,
	DDXPointPtr /*pptInit*/,
	int * /*pwidthInit*/,
	int /*fSorted*/
);

extern void afbOpaqueStippleFS(
	DrawablePtr /*pDrawable*/,
	GCPtr/*pGC*/,
	int /*nInit*/,
	DDXPointPtr /*pptInit*/,
	int * /*pwidthInit*/,
	int /*fSorted*/
);

extern void afbUnnaturalOpaqueStippleFS(
	DrawablePtr /*pDrawable*/,
	GCPtr/*pGC*/,
	int /*nInit*/,
	DDXPointPtr /*pptInit*/,
	int * /*pwidthInit*/,
	int /*fSorted*/
);

/* afbfont.c */

extern Bool afbRealizeFont(
	ScreenPtr /*pscr*/,
	FontPtr /*pFont*/
);

extern Bool afbUnrealizeFont(
	ScreenPtr /*pscr*/,
	FontPtr /*pFont*/
);
/* afbgc.c */

extern Bool afbCreateGC(
	GCPtr /*pGC*/
);

extern void afbValidateGC(
	GCPtr /*pGC*/,
	unsigned long /*changes*/,
	DrawablePtr /*pDrawable*/
);

extern void afbDestroyGC(
	GCPtr /*pGC*/
);

extern void afbReduceRop(
	int /*alu*/,
	Pixel /*src*/,
	unsigned long /*planemask*/,
	int /*depth*/,
	unsigned char * /*rrops*/
);

extern void afbReduceOpaqueStipple (
	Pixel /*fg*/,
	Pixel /*bg*/,
	unsigned long /*planemask*/,
	int /*depth*/,
	unsigned char * /*rrops*/
);

extern void afbComputeCompositeClip(
   GCPtr /*pGC*/,
   DrawablePtr /*pDrawable*/
);

/* afbgetsp.c */

extern void afbGetSpans(
	DrawablePtr /*pDrawable*/,
	int /*wMax*/,
	DDXPointPtr /*ppt*/,
	int * /*pwidth*/,
	int /*nspans*/,
	char * /*pdstStart*/
);
/* afbhrzvert.c */

extern void afbHorzS(
	PixelType * /*addrl*/,
	int /*nlwidth*/,
	int /*sizeDst*/,
	int /*depthDst*/,
	int /*x1*/,
	int /*y1*/,
	int /*len*/,
	unsigned char * /*rrops*/
);

extern void afbVertS(
	PixelType * /*addrl*/,
	int /*nlwidth*/,
	int /*sizeDst*/,
	int /*depthDst*/,
	int /*x1*/,
	int /*y1*/,
	int /*len*/,
	unsigned char * /*rrops*/
);
/* afbigbblak.c */

extern void afbImageGlyphBlt (
	DrawablePtr /*pDrawable*/,
	GCPtr/*pGC*/,
	int /*x*/,
	int /*y*/,
	unsigned int /*nglyph*/,
	CharInfoPtr * /*ppci*/,
	pointer /*pglyphBase*/
);
/* afbigbwht.c */

/* afbimage.c */

extern void afbPutImage(
	DrawablePtr /*dst*/,
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

extern void afbGetImage(
	DrawablePtr /*pDrawable*/,
	int /*sx*/,
	int /*sy*/,
	int /*w*/,
	int /*h*/,
	unsigned int /*format*/,
	unsigned long /*planeMask*/,
	char * /*pdstLine*/
);
/* afbline.c */

extern void afbLineSS(
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	int /*mode*/,
	int /*npt*/,
	DDXPointPtr /*pptInit*/
);

extern void afbLineSD(
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	int /*mode*/,
	int /*npt*/,
	DDXPointPtr /*pptInit*/
);

/* afbmisc.c */

extern void afbQueryBestSize(
	int /*class*/,
	unsigned short * /*pwidth*/,
	unsigned short * /*pheight*/,
	ScreenPtr /*pScreen*/
);
/* afbpntarea.c */

extern void afbSolidFillArea(
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	unsigned char * /*rrops*/
);

extern void afbStippleAreaPPW(
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	PixmapPtr /*pstipple*/,
	unsigned char * /*rrops*/
);
extern void afbStippleArea(
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	PixmapPtr /*pstipple*/,
	int /*xOff*/,
	int /*yOff*/,
	unsigned char * /*rrops*/
);
/* afbplygblt.c */

extern void afbPolyGlyphBlt(
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	int /*x*/,
	int /*y*/,
	unsigned int /*nglyph*/,
	CharInfoPtr * /*ppci*/,
	pointer /*pglyphBase*/
);

/* afbpixmap.c */

extern PixmapPtr afbCreatePixmap(
	ScreenPtr /*pScreen*/,
	int /*width*/,
	int /*height*/,
	int /*depth*/
);

extern Bool afbDestroyPixmap(
	PixmapPtr /*pPixmap*/
);

extern PixmapPtr afbCopyPixmap(
	PixmapPtr /*pSrc*/
);

extern void afbPadPixmap(
	PixmapPtr /*pPixmap*/
);

extern void afbXRotatePixmap(
	PixmapPtr /*pPix*/,
	int /*rw*/
);

extern void afbYRotatePixmap(
	PixmapPtr /*pPix*/,
	int /*rh*/
);

extern void afbCopyRotatePixmap(
	PixmapPtr /*psrcPix*/,
	PixmapPtr * /*ppdstPix*/,
	int /*xrot*/,
	int /*yrot*/
);
extern void afbPaintWindow(
	WindowPtr /*pWin*/,
	RegionPtr /*pRegion*/,
	int /*what*/
);
/* afbpolypnt.c */

extern void afbPolyPoint(
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	int /*mode*/,
	int /*npt*/,
	xPoint * /*pptInit*/
);
/* afbpushpxl.c */

extern void afbPushPixels(
	GCPtr /*pGC*/,
	PixmapPtr /*pBitMap*/,
	DrawablePtr /*pDrawable*/,
	int /*dx*/,
	int /*dy*/,
	int /*xOrg*/,
	int /*yOrg*/
);
/* afbscrclse.c */

extern Bool afbCloseScreen(
	int /*index*/,
	ScreenPtr /*pScreen*/
);
/* afbscrinit.c */

extern Bool afbAllocatePrivates(
	ScreenPtr /*pScreen*/,
	int * /*pWinIndex*/,
	int * /*pGCIndex*/
);

extern Bool afbScreenInit(
	ScreenPtr /*pScreen*/,
	pointer /*pbits*/,
	int /*xsize*/,
	int /*ysize*/,
	int /*dpix*/,
	int /*dpiy*/,
	int /*width*/
);

extern PixmapPtr afbGetWindowPixmap(
	WindowPtr /*pWin*/
);

extern void afbSetWindowPixmap(
	WindowPtr /*pWin*/,
	PixmapPtr /*pPix*/
);

/* afbseg.c */

extern void afbSegmentSS(
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	int /*nseg*/,
	xSegment * /*pSeg*/
);

extern void afbSegmentSD(
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	int /*nseg*/,
	xSegment * /*pSeg*/
);
/* afbsetsp.c */

extern void afbSetScanline(
	int /*y*/,
	int /*xOrigin*/,
	int /*xStart*/,
	int /*xEnd*/,
	PixelType * /*psrc*/,
	int /*alu*/,
	PixelType * /*pdstBase*/,
	int /*widthDst*/,
	int /*sizeDst*/,
	int /*depthDst*/,
	int /*sizeSrc*/
);

extern void afbSetSpans(
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	char * /*psrc*/,
	DDXPointPtr /*ppt*/,
	int * /*pwidth*/,
	int /*nspans*/,
	int /*fSorted*/
);
/* afbtegblt.c */

extern void afbTEGlyphBlt(
	DrawablePtr /*pDrawable*/,
	GCPtr/*pGC*/,
	int /*x*/,
	int /*y*/,
	unsigned int /*nglyph*/,
	CharInfoPtr * /*ppci*/,
	pointer /*pglyphBase*/
);
/* afbtileC.c */

extern void afbTileAreaPPWCopy(
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	int /*alu*/,
	PixmapPtr /*ptile*/,
	unsigned long /*planemask*/
);
/* afbtileG.c */

extern void afbTileAreaPPWGeneral(
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	int /*alu*/,
	PixmapPtr /*ptile*/,
	unsigned long /*planemask*/
);

extern void afbTileAreaCopy(
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	int /*alu*/,
	PixmapPtr /*ptile*/,
	int /*xOff*/,
	int /*yOff*/,
	unsigned long /*planemask*/
);
/* afbtileG.c */

extern void afbTileAreaGeneral(
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	int /*alu*/,
	PixmapPtr /*ptile*/,
	int /*xOff*/,
	int /*yOff*/,
	unsigned long /*planemask*/
);

extern void afbOpaqueStippleAreaPPWCopy(
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	int /*alu*/,
	PixmapPtr /*ptile*/,
	unsigned char */*rropsOS*/,
	unsigned long /*planemask*/
);
/* afbtileG.c */

extern void afbOpaqueStippleAreaPPWGeneral(
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	int /*alu*/,
	PixmapPtr /*ptile*/,
	unsigned char */*rropsOS*/,
	unsigned long /*planemask*/
);

extern void afbOpaqueStippleAreaCopy(
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	int /*alu*/,
	PixmapPtr /*ptile*/,
	int /*xOff*/,
	int /*yOff*/,
	unsigned char */*rropsOS*/,
	unsigned long /*planemask*/
);
/* afbtileG.c */

extern void afbOpaqueStippleAreaGeneral(
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	int /*alu*/,
	PixmapPtr /*ptile*/,
	int /*xOff*/,
	int /*yOff*/,
	unsigned char */*rropsOS*/,
	unsigned long /*planemask*/
);

/* afbwindow.c */

extern Bool afbCreateWindow(
	WindowPtr /*pWin*/
);

extern Bool afbDestroyWindow(
	WindowPtr /*pWin*/
);

extern Bool afbMapWindow(
	WindowPtr /*pWindow*/
);

extern Bool afbPositionWindow(
	WindowPtr /*pWin*/,
	int /*x*/,
	int /*y*/
);

extern Bool afbUnmapWindow(
	WindowPtr /*pWindow*/
);

extern void afbCopyWindow(
	WindowPtr /*pWin*/,
	DDXPointRec /*ptOldOrg*/,
	RegionPtr /*prgnSrc*/
);

extern Bool afbChangeWindowAttributes(
	WindowPtr /*pWin*/,
	unsigned long /*mask*/
);
/* afbzerarc.c */

extern void afbZeroPolyArcSS(
	DrawablePtr /*pDraw*/,
	GCPtr /*pGC*/,
	int /*narcs*/,
	xArc * /*parcs*/
);

/*
	private field of pixmap
	pixmap.devPrivate = (PixelType *)pointer_to_bits
	pixmap.devKind = width_of_pixmap_in_bytes

	private field of screen
	a pixmap, for which we allocate storage.  devPrivate is a pointer to
the bits in the hardware framebuffer.  note that devKind can be poked to
make the code work for framebuffers that are wider than their
displayable screen (e.g. the early vsII, which displayed 960 pixels
across, but was 1024 in the hardware.)

	private field of GC
*/

typedef struct {
	unsigned char rrops[AFB_MAX_DEPTH];		/* reduction of rasterop to 1 of 3 */
	unsigned char rropOS[AFB_MAX_DEPTH];	/* rop for opaque stipple */
} afbPrivGC;
typedef afbPrivGC *afbPrivGCPtr;

extern int afbGCPrivateIndex;			/* index into GC private array */
extern int afbWindowPrivateIndex;		/* index into Window private array */
#ifdef PIXMAP_PER_WINDOW
extern int frameWindowPrivateIndex;		/* index into Window private array */
#endif

#define afbGetGCPrivate(pGC) \
	((afbPrivGC *)((pGC)->devPrivates[afbGCPrivateIndex].ptr))

/* private field of window */
typedef struct {
	unsigned char fastBorder;	/* non-zero if border tile is 32 bits wide */
	unsigned char fastBackground;
	unsigned short unused; /* pad for alignment with Sun compiler */
	DDXPointRec oldRotate;
	PixmapPtr pRotatedBackground;
	PixmapPtr pRotatedBorder;
} afbPrivWin;

/* Common macros for extracting drawing information */

#define afbGetTypedWidth(pDrawable,wtype)( \
	(((pDrawable)->type == DRAWABLE_WINDOW) ? \
	 (int)(((PixmapPtr)((pDrawable)->pScreen->devPrivates[afbScreenPrivateIndex].ptr))->devKind) : \
	 (int)(((PixmapPtr)pDrawable)->devKind)) / sizeof (wtype))

#define afbGetByteWidth(pDrawable) afbGetTypedWidth(pDrawable, unsigned char)

#define afbGetPixelWidth(pDrawable) afbGetTypedWidth(pDrawable, PixelType)

#define afbGetTypedWidthAndPointer(pDrawable, width, pointer, wtype, ptype) {\
	PixmapPtr   _pPix; \
	if ((pDrawable)->type == DRAWABLE_WINDOW) \
		_pPix = (PixmapPtr)(pDrawable)->pScreen->devPrivates[afbScreenPrivateIndex].ptr; \
	else \
		_pPix = (PixmapPtr)(pDrawable); \
	(pointer) = (ptype *) _pPix->devPrivate.ptr; \
	(width) = ((int) _pPix->devKind) / sizeof (wtype); \
}

#define afbGetPixelWidthSizeDepthAndPointer(pDrawable, width, size, dep, pointer) {\
	PixmapPtr _pPix; \
	if ((pDrawable)->type == DRAWABLE_WINDOW) \
		_pPix = (PixmapPtr)(pDrawable)->pScreen->devPrivates[afbScreenPrivateIndex].ptr; \
	else \
		_pPix = (PixmapPtr)(pDrawable); \
	(pointer) = (PixelType *)_pPix->devPrivate.ptr; \
	(width) = ((int)_pPix->devKind) / sizeof (PixelType); \
	(size) = (width) * _pPix->drawable.height; \
	(dep) = _pPix->drawable.depth; (void)(dep); \
}

#define afbGetByteWidthAndPointer(pDrawable, width, pointer) \
	afbGetTypedWidthAndPointer(pDrawable, width, pointer, unsigned char, unsigned char)

#define afbGetPixelWidthAndPointer(pDrawable, width, pointer) \
	afbGetTypedWidthAndPointer(pDrawable, width, pointer, PixelType, PixelType)

#define afbGetWindowTypedWidthAndPointer(pWin, width, pointer, wtype, ptype) {\
	PixmapPtr	_pPix = (PixmapPtr)(pWin)->drawable.pScreen->devPrivates[afbScreenPrivateIndex].ptr; \
	(pointer) = (ptype *) _pPix->devPrivate.ptr; \
	(width) = ((int) _pPix->devKind) / sizeof (wtype); \
}

#define afbGetWindowPixelWidthAndPointer(pWin, width, pointer) \
	afbGetWindowTypedWidthAndPointer(pWin, width, pointer, PixelType, PixelType)

#define afbGetWindowByteWidthAndPointer(pWin, width, pointer) \
	afbGetWindowTypedWidthAndPointer(pWin, width, pointer, char, char)

/*  afb uses the following macros to calculate addresses in drawables.
 *  To support banked framebuffers, the macros come in four flavors.
 *  All four collapse into the same definition on unbanked devices.
 *
 *  afbScanlineFoo - calculate address and do bank switching
 *  afbScanlineFooNoBankSwitch - calculate address, don't bank switch
 *  afbScanlineFooSrc - calculate address, switch source bank
 *  afbScanlineFooDst - calculate address, switch destination bank
 */

/* The NoBankSwitch versions are the same for banked and unbanked cases */

#define afbScanlineIncNoBankSwitch(_ptr, _off) _ptr += (_off)
#define afbScanlineOffsetNoBankSwitch(_ptr, _off) ((_ptr) + (_off))
#define afbScanlineDeltaNoBankSwitch(_ptr, _y, _w) \
	afbScanlineOffsetNoBankSwitch(_ptr, (_y) * (_w))
#define afbScanlineNoBankSwitch(_ptr, _x, _y, _w) \
	afbScanlineOffsetNoBankSwitch(_ptr, (_y) * (_w) + ((_x) >> MFB_PWSH))

#ifdef MFB_LINE_BANK

#include "afblinebank.h" /* get macro definitions from this file */

#else /* !MFB_LINE_BANK - unbanked case */

#define afbScanlineInc(_ptr, _off)				afbScanlineIncNoBankSwitch(_ptr, _off)
#define afbScanlineIncSrc(_ptr, _off)			afbScanlineInc(_ptr, _off)
#define afbScanlineIncDst(_ptr, _off)			afbScanlineInc(_ptr, _off)

#define afbScanlineOffset(_ptr, _off)			afbScanlineOffsetNoBankSwitch(_ptr, _off)
#define afbScanlineOffsetSrc(_ptr, _off)		afbScanlineOffset(_ptr, _off)
#define afbScanlineOffsetDst(_ptr, _off)		afbScanlineOffset(_ptr, _off)

#define afbScanlineSrc(_ptr, _x, _y, _w)		afbScanline(_ptr, _x, _y, _w)
#define afbScanlineDst(_ptr, _x, _y, _w)		afbScanline(_ptr, _x, _y, _w)

#define afbScanlineDeltaSrc(_ptr, _y, _w)	afbScanlineDelta(_ptr, _y, _w)
#define afbScanlineDeltaDst(_ptr, _y, _w)	afbScanlineDelta(_ptr, _y, _w)

#endif /* MFB_LINE_BANK */

#define afbScanlineDelta(_ptr, _y, _w) \
	afbScanlineOffset(_ptr, (_y) * (_w))

#define afbScanline(_ptr, _x, _y, _w) \
	afbScanlineOffset(_ptr, (_y) * (_w) + ((_x) >> MFB_PWSH))

/* precomputed information about each glyph for GlyphBlt code.
   this saves recalculating the per glyph information for each box.
*/

typedef struct _afbpos{
	int xpos;					/* xposition of glyph's origin */
	int xchar;					/* x position mod 32 */
	int leftEdge;
	int rightEdge;
	int topEdge;
	int bottomEdge;
	PixelType *pdstBase;		/* longword with character origin */
	int widthGlyph;			/* width in bytes of this glyph */
} afbTEXTPOS;

/* reduced raster ops for afb */
#define RROP_BLACK	GXclear
#define RROP_WHITE	GXset
#define RROP_NOP		GXnoop
#define RROP_INVERT	GXinvert
#define RROP_COPY		GXcopy

/* macros for afbbitblt.c, afbfillsp.c
	these let the code do one switch on the rop per call, rather
	than a switch on the rop per item (span or rectangle.)
*/

#define fnCLEAR(src, dst)				(0)
#define fnAND(src, dst)					(src & dst)
#define fnANDREVERSE(src, dst)		(src & ~dst)
#define fnCOPY(src, dst)				(src)
#define fnANDINVERTED(src, dst)		(~src & dst)
#define fnNOOP(src, dst)				(dst)
#define fnXOR(src, dst)					(src ^ dst)
#define fnOR(src, dst)					(src | dst)
#define fnNOR(src, dst)					(~(src | dst))
#define fnEQUIV(src, dst)				(~src ^ dst)
#define fnINVERT(src, dst)				(~dst)
#define fnORREVERSE(src, dst)			(src | ~dst)
#define fnCOPYINVERTED(src, dst)		(~src)
#define fnORINVERTED(src, dst)		(~src | dst)
#define fnNAND(src, dst)				(~(src & dst))
#undef fnSET
#define fnSET(src, dst)					(~0)

/*  Using a "switch" statement is much faster in most cases
 *  since the compiler can do a look-up table or multi-way branch
 *  instruction, depending on the architecture.  The result on
 *  A Sun 3/50 is at least 2.5 times faster, assuming a uniform
 *  distribution of RasterOp operation types.
 *
 *  However, doing some profiling on a running system reveals
 *  GXcopy is the operation over 99.5% of the time and
 *  GXxor is the next most frequent (about .4%), so we make special
 *  checks for those first.
 *
 *  Note that this requires a change to the "calling sequence"
 *  since we can't engineer a "switch" statement to have an lvalue.
 */
#undef DoRop
#define DoRop(result, alu, src, dst) \
{ \
	if (alu == GXcopy) \
		result = fnCOPY (src, dst); \
	else if (alu == GXxor) \
		result = fnXOR (src, dst); \
	else \
		switch (alu) { \
			case GXclear: \
				result = fnCLEAR (src, dst); \
				break; \
			case GXand: \
				result = fnAND (src, dst); \
				break; \
			case GXandReverse: \
				result = fnANDREVERSE (src, dst); \
				break; \
			case GXandInverted: \
				result = fnANDINVERTED (src, dst); \
				break; \
			default: \
			case GXnoop: \
				result = fnNOOP (src, dst); \
				break; \
			case GXor: \
				result = fnOR (src, dst); \
				break; \
			case GXnor: \
				result = fnNOR (src, dst); \
				break; \
			case GXequiv: \
				result = fnEQUIV (src, dst); \
				break; \
			case GXinvert: \
				result = fnINVERT (src, dst); \
				break; \
			case GXorReverse: \
				result = fnORREVERSE (src, dst); \
				break; \
			case GXcopyInverted: \
				result = fnCOPYINVERTED (src, dst); \
				break; \
			case GXorInverted: \
				result = fnORINVERTED (src, dst); \
				break; \
			case GXnand: \
				result = fnNAND (src, dst); \
				break; \
			case GXset: \
				result = fnSET (src, dst); \
				break; \
	} \
}


/*  C expression fragments for various operations.  These get passed in
 *  as -D's on the compile command line.  See afb/Imakefile.  This
 *  fixes XBUG 6319.
 *
 *  This seems like a good place to point out that afb's use of the
 *  words black and white is an unfortunate misnomer.  In afb code, black
 *  means zero, and white means one.
 */
#define MFB_OPEQ_WHITE				|=
#define MFB_OPEQ_BLACK				&=~
#define MFB_OPEQ_INVERT				^=
#define MFB_EQWHOLEWORD_WHITE		=~0
#define MFB_EQWHOLEWORD_BLACK		=0
#define MFB_EQWHOLEWORD_INVERT	^=~0
#define MFB_OP_WHITE					/* nothing */
#define MFB_OP_BLACK					~

#ifdef XFree86LOADER
#include "xf86_ansic.h"
#endif

