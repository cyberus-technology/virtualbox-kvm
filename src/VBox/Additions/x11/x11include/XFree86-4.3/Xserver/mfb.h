/* $XFree86: xc/programs/Xserver/mfb/mfb.h,v 1.19 2003/02/18 21:30:01 tsi Exp $ */
/* Combined Purdue/PurduePlus patches, level 2.0, 1/17/89 */
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
/* $Xorg: mfb.h,v 1.4 2001/02/09 02:05:18 xorgcvs Exp $ */

#if !defined(_MFB_H_) || defined(MFB_PROTOTYPES_ONLY)
#ifndef MFB_PROTOTYPES_ONLY
#define _MFB_H_
#endif

/* Monochrome Frame Buffer definitions 
   written by drewry, september 1986
*/
#include "pixmap.h"
#include "region.h"
#include "gc.h"
#include "colormap.h"
#include "miscstruct.h"
#include "mibstore.h"

extern int InverseAlu[];


/* warning: PixelType definition duplicated in maskbits.h */
#ifndef PixelType
#define PixelType CARD32
#endif /* PixelType */
#ifndef MfbBits
#define MfbBits CARD32
#endif

/* mfbbitblt.c */

extern void mfbDoBitblt(
#if NeedFunctionPrototypes
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/
#endif
);

extern RegionPtr mfbCopyArea(
#if NeedFunctionPrototypes
    DrawablePtr /*pSrcDrawable*/,
    DrawablePtr /*pDstDrawable*/,
    GCPtr/*pGC*/,
    int /*srcx*/,
    int /*srcy*/,
    int /*width*/,
    int /*height*/,
    int /*dstx*/,
    int /*dsty*/
#endif
);

extern Bool mfbRegisterCopyPlaneProc(
#if NeedFunctionPrototypes
    ScreenPtr /*pScreen*/,
    RegionPtr (* /*proc*/)(
#if NeedNestedPrototypes
	DrawablePtr         /* pSrcDrawable */,
	DrawablePtr         /* pDstDrawable */,
	GCPtr               /* pGC */,
	int                 /* srcx */,
	int                 /* srcy */,
	int                 /* width */,
	int                 /* height */,
	int                 /* dstx */,
	int                 /* dsty */,
	unsigned long	    /* bitPlane */
#endif
	)
#endif
);

extern RegionPtr mfbCopyPlane(
#if NeedFunctionPrototypes
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
#endif
);
/* mfbbltC.c */

extern void mfbDoBitbltCopy(
#if NeedFunctionPrototypes
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/
#endif
);
/* mfbbltCI.c */

extern void mfbDoBitbltCopyInverted(
#if NeedFunctionPrototypes
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/
#endif
);
/* mfbbltG.c */

extern void mfbDoBitbltGeneral(
#if NeedFunctionPrototypes
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/
#endif
);
/* mfbbltO.c */

extern void mfbDoBitbltOr(
#if NeedFunctionPrototypes
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/
#endif
);
/* mfbbltX.c */

extern void mfbDoBitbltXor(
#if NeedFunctionPrototypes
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/
#endif
);
/* mfbbres.c */

extern void mfbBresS(
#if NeedFunctionPrototypes
    int /*rop*/,
    PixelType * /*addrl*/,
    int /*nlwidth*/,
    int /*signdx*/,
    int /*signdy*/,
    int /*axis*/,
    int /*x1*/,
    int /*y1*/,
    int /*e*/,
    int /*e1*/,
    int /*e2*/,
    int /*len*/
#endif
);
/* mfbbresd.c */

extern void mfbBresD(
#if NeedFunctionPrototypes
    int /*fgrop*/,
    int /*bgrop*/,
    int * /*pdashIndex*/,
    unsigned char * /*pDash*/,
    int /*numInDashList*/,
    int * /*pdashOffset*/,
    int /*isDoubleDash*/,
    PixelType * /*addrl*/,
    int /*nlwidth*/,
    int /*signdx*/,
    int /*signdy*/,
    int /*axis*/,
    int /*x1*/,
    int /*y1*/,
    int /*e*/,
    int /*e1*/,
    int /*e2*/,
    int /*len*/
#endif
);
/* mfbbstore.c */

extern void mfbSaveAreas(
#if NeedFunctionPrototypes
    PixmapPtr /*pPixmap*/,
    RegionPtr /*prgnSave*/,
    int /*xorg*/,
    int /*yorg*/,
    WindowPtr /*pWin*/
#endif
);

extern void mfbRestoreAreas(
#if NeedFunctionPrototypes
    PixmapPtr /*pPixmap*/,
    RegionPtr /*prgnRestore*/,
    int /*xorg*/,
    int /*yorg*/,
    WindowPtr /*pWin*/
#endif
);
/* mfbclip.c */

extern RegionPtr mfbPixmapToRegion(
#if NeedFunctionPrototypes
    PixmapPtr /*pPix*/
#endif
);
/* mfbcmap.c */

extern int mfbListInstalledColormaps(
#if NeedFunctionPrototypes
    ScreenPtr /*pScreen*/,
    Colormap * /*pmaps*/
#endif
);

extern void mfbInstallColormap(
#if NeedFunctionPrototypes
    ColormapPtr /*pmap*/
#endif
);

extern void mfbUninstallColormap(
#if NeedFunctionPrototypes
    ColormapPtr /*pmap*/
#endif
);

extern void mfbResolveColor(
#if NeedFunctionPrototypes
    unsigned short * /*pred*/,
    unsigned short * /*pgreen*/,
    unsigned short * /*pblue*/,
    VisualPtr /*pVisual*/
#endif
);

extern Bool mfbCreateColormap(
#if NeedFunctionPrototypes
    ColormapPtr /*pMap*/
#endif
);

extern void mfbDestroyColormap(
#if NeedFunctionPrototypes
    ColormapPtr /*pMap*/
#endif
);

extern Bool mfbCreateDefColormap(
#if NeedFunctionPrototypes
    ScreenPtr /*pScreen*/
#endif
);
/* mfbfillarc.c */

extern void mfbPolyFillArcSolid(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
#endif
);
/* mfbfillrct.c */

extern void mfbPolyFillRect(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nrectFill*/,
    xRectangle * /*prectInit*/
#endif
);
/* mfbfillsp.c */

extern void mfbBlackSolidFS(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
#endif
);

extern void mfbWhiteSolidFS(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
#endif
);

extern void mfbInvertSolidFS(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
#endif
);

extern void mfbWhiteStippleFS(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
#endif
);

extern void mfbBlackStippleFS(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
#endif
);

extern void mfbInvertStippleFS(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
#endif
);

extern void mfbTileFS(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
#endif
);

extern void mfbUnnaturalTileFS(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
#endif
);

extern void mfbUnnaturalStippleFS(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
#endif
);
/* mfbfont.c */

extern Bool mfbRealizeFont(
#if NeedFunctionPrototypes
    ScreenPtr /*pscr*/,
    FontPtr /*pFont*/
#endif
);

extern Bool mfbUnrealizeFont(
#if NeedFunctionPrototypes
    ScreenPtr /*pscr*/,
    FontPtr /*pFont*/
#endif
);
/* mfbgc.c */

extern Bool mfbCreateGC(
#if NeedFunctionPrototypes
    GCPtr /*pGC*/
#endif
);

extern void mfbValidateGC(
#if NeedFunctionPrototypes
    GCPtr /*pGC*/,
    unsigned long /*changes*/,
    DrawablePtr /*pDrawable*/
#endif
);

extern int mfbReduceRop(
#if NeedFunctionPrototypes
    int /*alu*/,
    Pixel /*src*/
#endif
);

/* mfbgetsp.c */

extern void mfbGetSpans(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    int /*wMax*/,
    DDXPointPtr /*ppt*/,
    int * /*pwidth*/,
    int /*nspans*/,
    char * /*pdstStart*/
#endif
);
/* mfbhrzvert.c */

extern void mfbHorzS(
#if NeedFunctionPrototypes
    int /*rop*/,
    PixelType * /*addrl*/,
    int /*nlwidth*/,
    int /*x1*/,
    int /*y1*/,
    int /*len*/
#endif
);

extern void mfbVertS(
#if NeedFunctionPrototypes
    int /*rop*/,
    PixelType * /*addrl*/,
    int /*nlwidth*/,
    int /*x1*/,
    int /*y1*/,
    int /*len*/
#endif
);
/* mfbigbblak.c */

extern void mfbImageGlyphBltBlack(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
#endif
);
/* mfbigbwht.c */

extern void mfbImageGlyphBltWhite(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
#endif
);
/* mfbimage.c */

extern void mfbPutImage(
#if NeedFunctionPrototypes
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
#endif
);

extern void mfbGetImage(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    int /*sx*/,
    int /*sy*/,
    int /*w*/,
    int /*h*/,
    unsigned int /*format*/,
    unsigned long /*planeMask*/,
    char * /*pdstLine*/
#endif
);
/* mfbline.c */

extern void mfbLineSS(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/
#endif
);

extern void mfbLineSD(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/
#endif
);

/* mfbmisc.c */

extern void mfbQueryBestSize(
#if NeedFunctionPrototypes
    int /*class*/,
    unsigned short * /*pwidth*/,
    unsigned short * /*pheight*/,
    ScreenPtr /*pScreen*/
#endif
);
/* mfbpablack.c */

extern void mfbSolidBlackArea(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    int /*nbox*/,
    BoxPtr /*pbox*/,
    int /*alu*/,
    PixmapPtr /*nop*/
#endif
);

extern void mfbStippleBlackArea(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    int /*nbox*/,
    BoxPtr /*pbox*/,
    int /*alu*/,
    PixmapPtr /*pstipple*/
#endif
);
/* mfbpainv.c */

extern void mfbSolidInvertArea(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    int /*nbox*/,
    BoxPtr /*pbox*/,
    int /*alu*/,
    PixmapPtr /*nop*/
#endif
);

extern void mfbStippleInvertArea(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    int /*nbox*/,
    BoxPtr /*pbox*/,
    int /*alu*/,
    PixmapPtr /*pstipple*/
#endif
);
/* mfbpawhite.c */

extern void mfbSolidWhiteArea(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    int /*nbox*/,
    BoxPtr /*pbox*/,
    int /*alu*/,
    PixmapPtr /*nop*/
#endif
);

extern void mfbStippleWhiteArea(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    int /*nbox*/,
    BoxPtr /*pbox*/,
    int /*alu*/,
    PixmapPtr /*pstipple*/
#endif
);
/* mfbpgbblak.c */

extern void mfbPolyGlyphBltBlack(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
#endif
);
/* mfbpgbinv.c */

extern void mfbPolyGlyphBltInvert(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
#endif
);
/* mfbpgbwht.c */

extern void mfbPolyGlyphBltWhite(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
#endif
);
/* mfbpixmap.c */

extern PixmapPtr mfbCreatePixmap(
#if NeedFunctionPrototypes
    ScreenPtr /*pScreen*/,
    int /*width*/,
    int /*height*/,
    int /*depth*/
#endif
);

extern Bool mfbDestroyPixmap(
#if NeedFunctionPrototypes
    PixmapPtr /*pPixmap*/
#endif
);

extern PixmapPtr mfbCopyPixmap(
#if NeedFunctionPrototypes
    PixmapPtr /*pSrc*/
#endif
);

extern void mfbPadPixmap(
#if NeedFunctionPrototypes
    PixmapPtr /*pPixmap*/
#endif
);

extern void mfbXRotatePixmap(
#if NeedFunctionPrototypes
    PixmapPtr /*pPix*/,
    int /*rw*/
#endif
);

extern void mfbYRotatePixmap(
#if NeedFunctionPrototypes
    PixmapPtr /*pPix*/,
    int /*rh*/
#endif
);

extern void mfbCopyRotatePixmap(
#if NeedFunctionPrototypes
    PixmapPtr /*psrcPix*/,
    PixmapPtr * /*ppdstPix*/,
    int /*xrot*/,
    int /*yrot*/
#endif
);
/* mfbplyblack.c */

extern void mfbFillPolyBlack(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*shape*/,
    int /*mode*/,
    int /*count*/,
    DDXPointPtr /*ptsIn*/
#endif
);
/* mfbplyinv.c */

extern void mfbFillPolyInvert(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*shape*/,
    int /*mode*/,
    int /*count*/,
    DDXPointPtr /*ptsIn*/
#endif
);
/* mfbplywhite.c */

extern void mfbFillPolyWhite(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*shape*/,
    int /*mode*/,
    int /*count*/,
    DDXPointPtr /*ptsIn*/
#endif
);
/* mfbpntwin.c */

extern void mfbPaintWindow(
#if NeedFunctionPrototypes
    WindowPtr /*pWin*/,
    RegionPtr /*pRegion*/,
    int /*what*/
#endif
);
/* mfbpolypnt.c */

extern void mfbPolyPoint(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    xPoint * /*pptInit*/
#endif
);
/* mfbpushpxl.c */

extern void mfbSolidPP(
#if NeedFunctionPrototypes
    GCPtr /*pGC*/,
    PixmapPtr /*pBitMap*/,
    DrawablePtr /*pDrawable*/,
    int /*dx*/,
    int /*dy*/,
    int /*xOrg*/,
    int /*yOrg*/
#endif
);

extern void mfbPushPixels(
#if NeedFunctionPrototypes
    GCPtr /*pGC*/,
    PixmapPtr /*pBitMap*/,
    DrawablePtr /*pDrawable*/,
    int /*dx*/,
    int /*dy*/,
    int /*xOrg*/,
    int /*yOrg*/
#endif
);
/* mfbscrclse.c */

extern Bool mfbCloseScreen(
#if NeedFunctionPrototypes
    int /*index*/,
    ScreenPtr /*pScreen*/
#endif
);
/* mfbscrinit.c */

extern Bool mfbAllocatePrivates(
#if NeedFunctionPrototypes
    ScreenPtr /*pScreen*/,
    int * /*pWinIndex*/,
    int * /*pGCIndex*/
#endif
);

extern Bool mfbScreenInit(
#if NeedFunctionPrototypes
    ScreenPtr /*pScreen*/,
    pointer /*pbits*/,
    int /*xsize*/,
    int /*ysize*/,
    int /*dpix*/,
    int /*dpiy*/,
    int /*width*/
#endif
);

extern PixmapPtr mfbGetWindowPixmap(
#if NeedFunctionPrototypes
    WindowPtr /*pWin*/
#endif
);

extern void mfbSetWindowPixmap(
#if NeedFunctionPrototypes
    WindowPtr /*pWin*/,
    PixmapPtr /*pPix*/
#endif
);

/* mfbseg.c */

extern void mfbSegmentSS(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSeg*/
#endif
);

extern void mfbSegmentSD(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSeg*/
#endif
);
/* mfbsetsp.c */

extern void mfbSetScanline(
#if NeedFunctionPrototypes
    int /*y*/,
    int /*xOrigin*/,
    int /*xStart*/,
    int /*xEnd*/,
    PixelType * /*psrc*/,
    int /*alu*/,
    PixelType * /*pdstBase*/,
    int /*widthDst*/
#endif
);

extern void mfbSetSpans(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    char * /*psrc*/,
    DDXPointPtr /*ppt*/,
    int * /*pwidth*/,
    int /*nspans*/,
    int /*fSorted*/
#endif
);
/* mfbteblack.c */

extern void mfbTEGlyphBltBlack(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
#endif
);
/* mfbtewhite.c */

extern void mfbTEGlyphBltWhite(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
#endif
);
/* mfbtileC.c */

extern void mfbTileAreaPPWCopy(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    int /*nbox*/,
    BoxPtr /*pbox*/,
    int /*alu*/,
    PixmapPtr /*ptile*/
#endif
);
/* mfbtileG.c */

extern void mfbTileAreaPPWGeneral(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    int /*nbox*/,
    BoxPtr /*pbox*/,
    int /*alu*/,
    PixmapPtr /*ptile*/
#endif
);

extern void mfbTileAreaPPW(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    int /*nbox*/,
    BoxPtr /*pbox*/,
    int /*alu*/,
    PixmapPtr /*ptile*/
#endif
);
/* mfbwindow.c */

extern Bool mfbCreateWindow(
#if NeedFunctionPrototypes
    WindowPtr /*pWin*/
#endif
);

extern Bool mfbDestroyWindow(
#if NeedFunctionPrototypes
    WindowPtr /*pWin*/
#endif
);

extern Bool mfbMapWindow(
#if NeedFunctionPrototypes
    WindowPtr /*pWindow*/
#endif
);

extern Bool mfbPositionWindow(
#if NeedFunctionPrototypes
    WindowPtr /*pWin*/,
    int /*x*/,
    int /*y*/
#endif
);

extern Bool mfbUnmapWindow(
#if NeedFunctionPrototypes
    WindowPtr /*pWindow*/
#endif
);

extern void mfbCopyWindow(
#if NeedFunctionPrototypes
    WindowPtr /*pWin*/,
    DDXPointRec /*ptOldOrg*/,
    RegionPtr /*prgnSrc*/
#endif
);

extern Bool mfbChangeWindowAttributes(
#if NeedFunctionPrototypes
    WindowPtr /*pWin*/,
    unsigned long /*mask*/
#endif
);
/* mfbzerarc.c */

extern void mfbZeroPolyArcSS(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
#endif
);

#ifndef MFB_PROTOTYPES_ONLY
/*
   private filed of pixmap
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
typedef void (*mfbFillAreaProcPtr)(
#if NeedNestedPrototypes
	      DrawablePtr /*pDraw*/,
	      int /*nbox*/,
	      BoxPtr /*pbox*/,
	      int /*alu*/,
	      PixmapPtr /*nop*/
#endif
	      );

typedef struct {
    unsigned char	rop;		/* reduction of rasterop to 1 of 3 */
    unsigned char	ropOpStip;	/* rop for opaque stipple */
    unsigned char	ropFillArea;	/*  == alu, rop, or ropOpStip */
    unsigned char	unused1[sizeof(long) - 3];	/* Alignment */
    mfbFillAreaProcPtr 	FillArea;	/* fills regions; look at the code */
    } mfbPrivGC;
typedef mfbPrivGC	*mfbPrivGCPtr;
#endif

extern int  mfbGCPrivateIndex;		/* index into GC private array */
extern int  mfbWindowPrivateIndex;	/* index into Window private array */
#ifdef PIXMAP_PER_WINDOW
extern int  frameWindowPrivateIndex;	/* index into Window private array */
#endif

#ifndef MFB_PROTOTYPES_ONLY
/* private field of window */
typedef struct {
    unsigned char fastBorder;	/* non-zero if border tile is 32 bits wide */
    unsigned char fastBackground;
    unsigned short unused; /* pad for alignment with Sun compiler */
    DDXPointRec	oldRotate;
    PixmapPtr	pRotatedBackground;
    PixmapPtr	pRotatedBorder;
    } mfbPrivWin;

/* Common macros for extracting drawing information */

#define mfbGetTypedWidth(pDrawable,wtype) (\
    (((pDrawable)->type == DRAWABLE_WINDOW) ? \
     (int) (((PixmapPtr)((pDrawable)->pScreen->devPrivate))->devKind) : \
     (int)(((PixmapPtr)pDrawable)->devKind)) / sizeof (wtype))

#define mfbGetByteWidth(pDrawable) mfbGetTypedWidth(pDrawable, unsigned char)

#define mfbGetPixelWidth(pDrawable) mfbGetTypedWidth(pDrawable, PixelType)
    
#define mfbGetTypedWidthAndPointer(pDrawable, width, pointer, wtype, ptype) {\
    PixmapPtr   _pPix; \
    if ((pDrawable)->type == DRAWABLE_WINDOW) \
	_pPix = (PixmapPtr) (pDrawable)->pScreen->devPrivate; \
    else \
	_pPix = (PixmapPtr) (pDrawable); \
    (pointer) = (ptype *) _pPix->devPrivate.ptr; \
    (width) = ((int) _pPix->devKind) / sizeof (wtype); \
}

#define mfbGetByteWidthAndPointer(pDrawable, width, pointer) \
    mfbGetTypedWidthAndPointer(pDrawable, width, pointer, unsigned char, unsigned char)

#define mfbGetPixelWidthAndPointer(pDrawable, width, pointer) \
    mfbGetTypedWidthAndPointer(pDrawable, width, pointer, PixelType, PixelType)

#define mfbGetWindowTypedWidthAndPointer(pWin, width, pointer, wtype, ptype) {\
    PixmapPtr	_pPix = (PixmapPtr) (pWin)->drawable.pScreen->devPrivate; \
    (pointer) = (ptype *) _pPix->devPrivate.ptr; \
    (width) = ((int) _pPix->devKind) / sizeof (wtype); \
}

#define mfbGetWindowPixelWidthAndPointer(pWin, width, pointer) \
    mfbGetWindowTypedWidthAndPointer(pWin, width, pointer, PixelType, PixelType)

#define mfbGetWindowByteWidthAndPointer(pWin, width, pointer) \
    mfbGetWindowTypedWidthAndPointer(pWin, width, pointer, char, char)

/*  mfb uses the following macros to calculate addresses in drawables.
 *  To support banked framebuffers, the macros come in four flavors.
 *  All four collapse into the same definition on unbanked devices.
 *  
 *  mfbScanlineFoo - calculate address and do bank switching
 *  mfbScanlineFooNoBankSwitch - calculate address, don't bank switch
 *  mfbScanlineFooSrc - calculate address, switch source bank
 *  mfbScanlineFooDst - calculate address, switch destination bank
 */

/* The NoBankSwitch versions are the same for banked and unbanked cases */

#define mfbScanlineIncNoBankSwitch(_ptr, _off) _ptr += (_off)
#define mfbScanlineOffsetNoBankSwitch(_ptr, _off) ((_ptr) + (_off))
#define mfbScanlineDeltaNoBankSwitch(_ptr, _y, _w) \
    mfbScanlineOffsetNoBankSwitch(_ptr, (_y) * (_w))
#define mfbScanlineNoBankSwitch(_ptr, _x, _y, _w) \
    mfbScanlineOffsetNoBankSwitch(_ptr, (_y) * (_w) + ((_x) >> MFB_PWSH))

#ifdef MFB_LINE_BANK

#include "mfblinebank.h" /* get macro definitions from this file */

#else /* !MFB_LINE_BANK - unbanked case */

#define mfbScanlineInc(_ptr, _off)       mfbScanlineIncNoBankSwitch(_ptr, _off)
#define mfbScanlineIncSrc(_ptr, _off)     mfbScanlineInc(_ptr, _off)
#define mfbScanlineIncDst(_ptr, _off)     mfbScanlineInc(_ptr, _off)

#define mfbScanlineOffset(_ptr, _off) mfbScanlineOffsetNoBankSwitch(_ptr, _off)
#define mfbScanlineOffsetSrc(_ptr, _off)  mfbScanlineOffset(_ptr, _off)
#define mfbScanlineOffsetDst(_ptr, _off)  mfbScanlineOffset(_ptr, _off)

#define mfbScanlineSrc(_ptr, _x, _y, _w)  mfbScanline(_ptr, _x, _y, _w)
#define mfbScanlineDst(_ptr, _x, _y, _w)  mfbScanline(_ptr, _x, _y, _w)

#define mfbScanlineDeltaSrc(_ptr, _y, _w) mfbScanlineDelta(_ptr, _y, _w)
#define mfbScanlineDeltaDst(_ptr, _y, _w) mfbScanlineDelta(_ptr, _y, _w)

#endif /* MFB_LINE_BANK */

#define mfbScanlineDelta(_ptr, _y, _w) \
    mfbScanlineOffset(_ptr, (_y) * (_w))

#define mfbScanline(_ptr, _x, _y, _w) \
    mfbScanlineOffset(_ptr, (_y) * (_w) + ((_x) >> MFB_PWSH))


/* precomputed information about each glyph for GlyphBlt code.
   this saves recalculating the per glyph information for each box.
*/
typedef struct _pos{
    int xpos;		/* xposition of glyph's origin */
    int xchar;		/* x position mod 32 */
    int leftEdge;
    int rightEdge;
    int topEdge;
    int bottomEdge;
    PixelType *pdstBase;	/* longword with character origin */
    int widthGlyph;	/* width in bytes of this glyph */
} TEXTPOS;

/* reduced raster ops for mfb */
#define RROP_BLACK	GXclear
#define RROP_WHITE	GXset
#define RROP_NOP	GXnoop
#define RROP_INVERT	GXinvert

/* macros for mfbbitblt.c, mfbfillsp.c
   these let the code do one switch on the rop per call, rather
than a switch on the rop per item (span or rectangle.)
*/

#define fnCLEAR(src, dst)	(0)
#define fnAND(src, dst) 	(src & dst)
#define fnANDREVERSE(src, dst)	(src & ~dst)
#define fnCOPY(src, dst)	(src)
#define fnANDINVERTED(src, dst)	(~src & dst)
#define fnNOOP(src, dst)	(dst)
#define fnXOR(src, dst)		(src ^ dst)
#define fnOR(src, dst)		(src | dst)
#define fnNOR(src, dst)		(~(src | dst))
#define fnEQUIV(src, dst)	(~src ^ dst)
#define fnINVERT(src, dst)	(~dst)
#define fnORREVERSE(src, dst)	(src | ~dst)
#define fnCOPYINVERTED(src, dst)(~src)
#define fnORINVERTED(src, dst)	(~src | dst)
#define fnNAND(src, dst)	(~(src & dst))
#undef fnSET
#define fnSET(src, dst)		(MfbBits)(~0)

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
	switch (alu) \
	{ \
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
 *  as -D's on the compile command line.  See mfb/Imakefile.  This
 *  fixes XBUG 6319.
 *
 *  This seems like a good place to point out that mfb's use of the
 *  words black and white is an unfortunate misnomer.  In mfb code, black
 *  means zero, and white means one.
 */
#define MFB_OPEQ_WHITE  |=
#define MFB_OPEQ_BLACK  &=~
#define MFB_OPEQ_INVERT ^=
#define MFB_EQWHOLEWORD_WHITE   =~0
#define MFB_EQWHOLEWORD_BLACK   =0
#define MFB_EQWHOLEWORD_INVERT  ^=~0
#define MFB_OP_WHITE    /* nothing */
#define MFB_OP_BLACK    ~

/*
 * if MFB is built as a module, it shouldn't call libc functions.
 */
#ifdef XFree86LOADER
#include "xf86_ansic.h"
#endif

#endif /* MFB_PROTOTYPES_ONLY */
#endif /* _MFB_H_ */
