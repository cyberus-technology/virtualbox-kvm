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
#include "privates.h"
#include "miscstruct.h"
#include "mibstore.h"

extern int InverseAlu[];
extern int mfbGetInverseAlu(int i);

/* warning: PixelType definition duplicated in maskbits.h */
#ifndef PixelType
#define PixelType CARD32
#endif /* PixelType */
#ifndef MfbBits
#define MfbBits CARD32
#endif

/* mfbbitblt.c */

extern void mfbDoBitblt(
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/
);

extern RegionPtr mfbCopyArea(
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

extern Bool mfbRegisterCopyPlaneProc(
    ScreenPtr /*pScreen*/,
    RegionPtr (* /*proc*/)(
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
	)
);

extern RegionPtr mfbCopyPlane(
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
/* mfbbltC.c */

extern void mfbDoBitbltCopy(
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/
);
/* mfbbltCI.c */

extern void mfbDoBitbltCopyInverted(
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/
);
/* mfbbltG.c */

extern void mfbDoBitbltGeneral(
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/
);
/* mfbbltO.c */

extern void mfbDoBitbltOr(
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/
);
/* mfbbltX.c */

extern void mfbDoBitbltXor(
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/
);
/* mfbbres.c */

extern void mfbBresS(
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
);
/* mfbbresd.c */

extern void mfbBresD(
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
);

/* mfbclip.c */

extern RegionPtr mfbPixmapToRegion(
    PixmapPtr /*pPix*/
);

#ifndef MFB_PROTOTYPES_ONLY
typedef RegionPtr (*mfbPixmapToRegionProc)(PixmapPtr);

extern mfbPixmapToRegionProc *mfbPixmapToRegionWeak(void);
#endif

/* mfbcmap.c */

extern int mfbListInstalledColormaps(
    ScreenPtr /*pScreen*/,
    Colormap * /*pmaps*/
);

extern void mfbInstallColormap(
    ColormapPtr /*pmap*/
);

extern void mfbUninstallColormap(
    ColormapPtr /*pmap*/
);

extern void mfbResolveColor(
    unsigned short * /*pred*/,
    unsigned short * /*pgreen*/,
    unsigned short * /*pblue*/,
    VisualPtr /*pVisual*/
);

extern Bool mfbCreateColormap(
    ColormapPtr /*pMap*/
);

extern void mfbDestroyColormap(
    ColormapPtr /*pMap*/
);

extern Bool mfbCreateDefColormap(
    ScreenPtr /*pScreen*/
);
/* mfbfillarc.c */

extern void mfbPolyFillArcSolid(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
);
/* mfbfillrct.c */

extern void mfbPolyFillRect(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nrectFill*/,
    xRectangle * /*prectInit*/
);
/* mfbfillsp.c */

extern void mfbBlackSolidFS(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);

extern void mfbWhiteSolidFS(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);

extern void mfbInvertSolidFS(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);

extern void mfbWhiteStippleFS(
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);

extern void mfbBlackStippleFS(
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);

extern void mfbInvertStippleFS(
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);

extern void mfbTileFS(
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);

extern void mfbUnnaturalTileFS(
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);

extern void mfbUnnaturalStippleFS(
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);
/* mfbfont.c */

extern Bool mfbRealizeFont(
    ScreenPtr /*pscr*/,
    FontPtr /*pFont*/
);

extern Bool mfbUnrealizeFont(
    ScreenPtr /*pscr*/,
    FontPtr /*pFont*/
);

#ifndef MFB_PROTOTYPES_ONLY
typedef void (*mfbRealizeFontProc)(ScreenPtr, FontPtr);
typedef void (*mfbUnrealizeFontProc)(ScreenPtr, FontPtr);

extern mfbRealizeFontProc *mfbRealizeFontWeak(void);
extern mfbUnrealizeFontProc *mfbUnrealizeFontWeak(void);
#endif

/* mfbgc.c */

extern Bool mfbCreateGC(
    GCPtr /*pGC*/
);

extern void mfbValidateGC(
    GCPtr /*pGC*/,
    unsigned long /*changes*/,
    DrawablePtr /*pDrawable*/
);

extern int mfbReduceRop(
    int /*alu*/,
    Pixel /*src*/
);

/* mfbgetsp.c */

extern void mfbGetSpans(
    DrawablePtr /*pDrawable*/,
    int /*wMax*/,
    DDXPointPtr /*ppt*/,
    int * /*pwidth*/,
    int /*nspans*/,
    char * /*pdstStart*/
);
/* mfbhrzvert.c */

extern void mfbHorzS(
    int /*rop*/,
    PixelType * /*addrl*/,
    int /*nlwidth*/,
    int /*x1*/,
    int /*y1*/,
    int /*len*/
);

extern void mfbVertS(
    int /*rop*/,
    PixelType * /*addrl*/,
    int /*nlwidth*/,
    int /*x1*/,
    int /*y1*/,
    int /*len*/
);
/* mfbigbblak.c */

extern void mfbImageGlyphBltBlack(
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
);
/* mfbigbwht.c */

extern void mfbImageGlyphBltWhite(
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
);
/* mfbimage.c */

extern void mfbPutImage(
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

extern void mfbGetImage(
    DrawablePtr /*pDrawable*/,
    int /*sx*/,
    int /*sy*/,
    int /*w*/,
    int /*h*/,
    unsigned int /*format*/,
    unsigned long /*planeMask*/,
    char * /*pdstLine*/
);
/* mfbline.c */

extern void mfbLineSS(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/
);

extern void mfbLineSD(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/
);

/* mfbmisc.c */

extern void mfbQueryBestSize(
    int /*class*/,
    unsigned short * /*pwidth*/,
    unsigned short * /*pheight*/,
    ScreenPtr /*pScreen*/
);

#ifndef MFB_PROTOTYPES_ONLY
typedef void (*mfbQueryBestSizeProc)(int, unsigned short *, unsigned short *,
                                     ScreenPtr);

extern mfbQueryBestSizeProc *mfbQueryBestSizeWeak(void);
#endif

/* mfbpablack.c */

extern void mfbSolidBlackArea(
    DrawablePtr /*pDraw*/,
    int /*nbox*/,
    BoxPtr /*pbox*/,
    int /*alu*/,
    PixmapPtr /*nop*/
);

extern void mfbStippleBlackArea(
    DrawablePtr /*pDraw*/,
    int /*nbox*/,
    BoxPtr /*pbox*/,
    int /*alu*/,
    PixmapPtr /*pstipple*/
);
/* mfbpainv.c */

extern void mfbSolidInvertArea(
    DrawablePtr /*pDraw*/,
    int /*nbox*/,
    BoxPtr /*pbox*/,
    int /*alu*/,
    PixmapPtr /*nop*/
);

extern void mfbStippleInvertArea(
    DrawablePtr /*pDraw*/,
    int /*nbox*/,
    BoxPtr /*pbox*/,
    int /*alu*/,
    PixmapPtr /*pstipple*/
);
/* mfbpawhite.c */

extern void mfbSolidWhiteArea(
    DrawablePtr /*pDraw*/,
    int /*nbox*/,
    BoxPtr /*pbox*/,
    int /*alu*/,
    PixmapPtr /*nop*/
);

extern void mfbStippleWhiteArea(
    DrawablePtr /*pDraw*/,
    int /*nbox*/,
    BoxPtr /*pbox*/,
    int /*alu*/,
    PixmapPtr /*pstipple*/
);

/* mfbpgbinv.c */

extern void mfbPolyGlyphBltBlack(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
);
/* mfbpgbinv.c */

extern void mfbPolyGlyphBltInvert(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
);
/* mfbpgbwht.c */

extern void mfbPolyGlyphBltWhite(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
);
/* mfbpixmap.c */

extern PixmapPtr mfbCreatePixmap(
    ScreenPtr /*pScreen*/,
    int /*width*/,
    int /*height*/,
    int /*depth*/,
    unsigned /*usage_hint*/
);

extern Bool mfbDestroyPixmap(
    PixmapPtr /*pPixmap*/
);

extern PixmapPtr mfbCopyPixmap(
    PixmapPtr /*pSrc*/
);

extern void mfbPadPixmap(
    PixmapPtr /*pPixmap*/
);

extern void mfbXRotatePixmap(
    PixmapPtr /*pPix*/,
    int /*rw*/
);

extern void mfbYRotatePixmap(
    PixmapPtr /*pPix*/,
    int /*rh*/
);

extern void mfbCopyRotatePixmap(
    PixmapPtr /*psrcPix*/,
    PixmapPtr * /*ppdstPix*/,
    int /*xrot*/,
    int /*yrot*/
);
/* mfbplyblack.c */

extern void mfbFillPolyBlack(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*shape*/,
    int /*mode*/,
    int /*count*/,
    DDXPointPtr /*ptsIn*/
);
/* mfbplyinv.c */

extern void mfbFillPolyInvert(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*shape*/,
    int /*mode*/,
    int /*count*/,
    DDXPointPtr /*ptsIn*/
);

/* mfbpntwin.c */

extern void mfbFillPolyWhite(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*shape*/,
    int /*mode*/,
    int /*count*/,
    DDXPointPtr /*ptsIn*/
);
/* mfbpolypnt.c */

extern void mfbPolyPoint(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    xPoint * /*pptInit*/
);
/* mfbpushpxl.c */

extern void mfbSolidPP(
    GCPtr /*pGC*/,
    PixmapPtr /*pBitMap*/,
    DrawablePtr /*pDrawable*/,
    int /*dx*/,
    int /*dy*/,
    int /*xOrg*/,
    int /*yOrg*/
);

extern void mfbPushPixels(
    GCPtr /*pGC*/,
    PixmapPtr /*pBitMap*/,
    DrawablePtr /*pDrawable*/,
    int /*dx*/,
    int /*dy*/,
    int /*xOrg*/,
    int /*yOrg*/
);

#ifndef MFB_PROTOTYPES_ONLY
typedef void (*mfbPushPixelsProc)(GCPtr, PixmapPtr, DrawablePtr, int, int,
                                  int, int);

extern mfbPushPixelsProc *mfbPushPixelsWeak(void);
#endif

/* mfbscrclse.c */

extern Bool mfbCloseScreen(
    int /*index*/,
    ScreenPtr /*pScreen*/
);
/* mfbscrinit.c */

extern Bool mfbAllocatePrivates(
    ScreenPtr /*pScreen*/,
    DevPrivateKey * /*pGCKey*/
);

extern Bool mfbScreenInit(
    ScreenPtr /*pScreen*/,
    pointer /*pbits*/,
    int /*xsize*/,
    int /*ysize*/,
    int /*dpix*/,
    int /*dpiy*/,
    int /*width*/
);

extern PixmapPtr mfbGetWindowPixmap(
    WindowPtr /*pWin*/
);

extern void mfbSetWindowPixmap(
    WindowPtr /*pWin*/,
    PixmapPtr /*pPix*/
);

extern void mfbFillInScreen(ScreenPtr pScreen);

/* mfbseg.c */

extern void mfbSegmentSS(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSeg*/
);

extern void mfbSegmentSD(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSeg*/
);
/* mfbsetsp.c */

extern void mfbSetScanline(
    int /*y*/,
    int /*xOrigin*/,
    int /*xStart*/,
    int /*xEnd*/,
    PixelType * /*psrc*/,
    int /*alu*/,
    PixelType * /*pdstBase*/,
    int /*widthDst*/
);

extern void mfbSetSpans(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    char * /*psrc*/,
    DDXPointPtr /*ppt*/,
    int * /*pwidth*/,
    int /*nspans*/,
    int /*fSorted*/
);
/* mfbteblack.c */

extern void mfbTEGlyphBltBlack(
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
);
/* mfbtewhite.c */

extern void mfbTEGlyphBltWhite(
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
);
/* mfbtileC.c */

extern void mfbTileAreaPPWCopy(
    DrawablePtr /*pDraw*/,
    int /*nbox*/,
    BoxPtr /*pbox*/,
    int /*alu*/,
    PixmapPtr /*ptile*/
);
/* mfbtileG.c */

extern void mfbTileAreaPPWGeneral(
    DrawablePtr /*pDraw*/,
    int /*nbox*/,
    BoxPtr /*pbox*/,
    int /*alu*/,
    PixmapPtr /*ptile*/
);

extern void mfbTileAreaPPW(
    DrawablePtr /*pDraw*/,
    int /*nbox*/,
    BoxPtr /*pbox*/,
    int /*alu*/,
    PixmapPtr /*ptile*/
);
/* mfbwindow.c */

extern Bool mfbCreateWindow(
    WindowPtr /*pWin*/
);

extern Bool mfbDestroyWindow(
    WindowPtr /*pWin*/
);

extern Bool mfbMapWindow(
    WindowPtr /*pWindow*/
);

extern Bool mfbPositionWindow(
    WindowPtr /*pWin*/,
    int /*x*/,
    int /*y*/
);

extern Bool mfbUnmapWindow(
    WindowPtr /*pWindow*/
);

extern void mfbCopyWindow(
    WindowPtr /*pWin*/,
    DDXPointRec /*ptOldOrg*/,
    RegionPtr /*prgnSrc*/
);

extern Bool mfbChangeWindowAttributes(
    WindowPtr /*pWin*/,
    unsigned long /*mask*/
);
/* mfbzerarc.c */

extern void mfbZeroPolyArcSS(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
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
	      DrawablePtr /*pDraw*/,
	      int /*nbox*/,
	      BoxPtr /*pbox*/,
	      int /*alu*/,
	      PixmapPtr /*nop*/
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

extern DevPrivateKey mfbGetGCPrivateKey(void);
#ifdef PIXMAP_PER_WINDOW
extern DevPrivateKey frameGetWindowPrivateKey(void);
#endif

#ifndef MFB_PROTOTYPES_ONLY
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

#endif /* MFB_PROTOTYPES_ONLY */
#endif /* _MFB_H_ */
