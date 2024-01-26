/* $XFree86: xc/programs/Xserver/iplan2p4/ipl.h,v 3.5 2001/01/30 22:06:21 tsi Exp $ */
/* $XConsortium: ipl.h,v 5.37 94/04/17 20:28:38 dpw Exp $ */
/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or X Consortium
not be used in advertising or publicity pertaining to 
distribution  of  the software  without specific prior 
written permission. Sun and X Consortium make no 
representations about the suitability of this software for 
any purpose. It is provided "as is" without any express or 
implied warranty.

SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO  THIS  SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-
NESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SUN BE  LI-
ABLE  FOR  ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,  DATA  OR
PROFITS,  WHETHER  IN  AN  ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

/* Modified nov 94 by Martin Schaller (Martin_Schaller@maus.r.de) for use with
interleaved planes */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include "pixmap.h"
#include "region.h"
#include "gc.h"
#include "colormap.h"
#include "miscstruct.h"
#include "servermd.h"
#include "windowstr.h"
#include "mfb.h"
#undef PixelType

#include "iplmap.h"

/*
   private filed of pixmap
   pixmap.devPrivate = (unsigned int *)pointer_to_bits
   pixmap.devKind = width_of_pixmap_in_bytes
*/

extern int  iplGCPrivateIndex;
extern int  iplWindowPrivateIndex;

/* private field of GC */
typedef struct {
    unsigned char       rop;            /* special case rop values */
    /* next two values unused in ipl, included for compatibility with mfb */
    unsigned char       ropOpStip;      /* rop for opaque stipple */
    /* this value is ropFillArea in mfb, usurped for ipl */
    unsigned char       oneRect;	/*  drawable has one clip rect */
    unsigned long	xor, and;	/* reduced rop values */
    unsigned short 	xorg[INTER_PLANES],andg[INTER_PLANES];
    } iplPrivGC;

typedef iplPrivGC	*iplPrivGCPtr;

#define iplGetGCPrivate(pGC)	((iplPrivGCPtr)\
	(pGC)->devPrivates[iplGCPrivateIndex].ptr)

#define iplGetCompositeClip(pGC) ((pGC)->pCompositeClip)

/* way to carry RROP info around */
typedef struct {
    unsigned char	rop;
    unsigned long	xor, and;
    unsigned short 	xorg[INTER_PLANES],andg[INTER_PLANES];
} iplRRopRec, *iplRRopPtr;

/* private field of window */
typedef struct {
    unsigned	char fastBorder; /* non-zero if border is 32 bits wide */
    unsigned	char fastBackground;
    unsigned short unused; /* pad for alignment with Sun compiler */
    DDXPointRec	oldRotate;
    PixmapPtr	pRotatedBackground;
    PixmapPtr	pRotatedBorder;
    } iplPrivWin;

#define iplGetWindowPrivate(_pWin) ((iplPrivWin *)\
	(_pWin)->devPrivates[iplWindowPrivateIndex].ptr)


/* ipl8bit.c */

extern int iplSetStipple(
    int /*alu*/,
    unsigned long /*fg*/,
    unsigned long /*planemask*/
);

extern int iplSetOpaqueStipple(
    int /*alu*/,
    unsigned long /*fg*/,
    unsigned long /*bg*/,
    unsigned long /*planemask*/
);

extern int iplComputeClipMasks32(
    BoxPtr /*pBox*/,
    int /*numRects*/,
    int /*x*/,
    int /*y*/,
    int /*w*/,
    int /*h*/,
    CARD32 * /*clips*/
);
/* ipl8cppl.c */

extern void iplCopyImagePlane(
    DrawablePtr /*pSrcDrawable*/,
    DrawablePtr /*pDstDrawable*/,
    int /*rop*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/
);

extern void iplCopyPlane8to1(
    DrawablePtr /*pSrcDrawable*/,
    DrawablePtr /*pDstDrawable*/,
    int /*rop*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/,
    unsigned long /*bitPlane*/
);
/* ipl8lineCO.c */

extern int ipl8LineSS1RectCopy(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/,
    DDXPointPtr /*pptInitOrig*/,
    int * /*x1p*/,
    int * /*y1p*/,
    int * /*x2p*/,
    int * /*y2p*/
);

extern void ipl8LineSS1Rect(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/
);

extern void ipl8ClippedLineCopy(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x1*/,
    int /*y1*/,
    int /*x2*/,
    int /*y2*/,
    BoxPtr /*boxp*/,
    Bool /*shorten*/
);
/* ipl8lineCP.c */

extern int ipl8LineSS1RectPreviousCopy(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/,
    DDXPointPtr /*pptInitOrig*/,
    int * /*x1p*/,
    int * /*y1p*/,
    int * /*x2p*/,
    int * /*y2p*/
);
/* ipl8lineG.c */

extern int ipl8LineSS1RectGeneral(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/,
    DDXPointPtr /*pptInitOrig*/,
    int * /*x1p*/,
    int * /*y1p*/,
    int * /*x2p*/,
    int * /*y2p*/
);

extern void ipl8ClippedLineGeneral(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x1*/,
    int /*y1*/,
    int /*x2*/,
    int /*y2*/,
    BoxPtr /*boxp*/,
    Bool /*shorten*/
);
/* ipl8lineX.c */

extern int ipl8LineSS1RectXor(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/,
    DDXPointPtr /*pptInitOrig*/,
    int * /*x1p*/,
    int * /*y1p*/,
    int * /*x2p*/,
    int * /*y2p*/
);

extern void ipl8ClippedLineXor(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x1*/,
    int /*y1*/,
    int /*x2*/,
    int /*y2*/,
    BoxPtr /*boxp*/,
    Bool /*shorten*/
);
/* ipl8segC.c */

extern int ipl8SegmentSS1RectCopy(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSegInit*/
);
/* ipl8segCS.c */

extern int ipl8SegmentSS1RectShiftCopy(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSegInit*/
);

extern void ipl8SegmentSS1Rect(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSegInit*/
);
/* ipl8segG.c */

extern int ipl8SegmentSS1RectGeneral(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSegInit*/
);
/* iplsegX.c */

extern int ipl8SegmentSS1RectXor(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSegInit*/
);
/* iplallpriv.c */

extern Bool iplAllocatePrivates(
    ScreenPtr /*pScreen*/,
    int * /*window_index*/,
    int * /*gc_index*/
);
/* iplbitblt.c */

extern RegionPtr iplBitBlt(
    DrawablePtr /*pSrcDrawable*/,
    DrawablePtr /*pDstDrawable*/,
    GCPtr/*pGC*/,
    int /*srcx*/,
    int /*srcy*/,
    int /*width*/,
    int /*height*/,
    int /*dstx*/,
    int /*dsty*/,
    void (* /*doBitBlt*/)(),
    unsigned long /*bitPlane*/
);

extern void iplDoBitblt(
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/
);

extern RegionPtr iplCopyArea(
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

extern void iplCopyPlane1to8(
    DrawablePtr /*pSrcDrawable*/,
    DrawablePtr /*pDstDrawable*/,
    int /*rop*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/,
    unsigned long /*bitPlane*/
);

extern RegionPtr iplCopyPlane(
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
/* iplbltC.c */

extern void iplDoBitbltCopy(
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/
);
/* iplbltG.c */

extern void iplDoBitbltGeneral(
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/
);
/* iplbltO.c */

extern void iplDoBitbltOr(
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/
);
/* iplbltX.c */

extern void iplDoBitbltXor(
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/
);
/* iplbres.c */

extern void iplBresS(
    int /*rop*/,
    unsigned short * /*and*/,
    unsigned short * /*xor*/,
    unsigned short * /*addrl*/,
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
/* iplbresd.c */

extern void iplBresD(
    iplRRopPtr /*rrops*/,
    int * /*pdashIndex*/,
    unsigned char * /*pDash*/,
    int /*numInDashList*/,
    int * /*pdashOffset*/,
    int /*isDoubleDash*/,
    unsigned short * /*addrl*/,
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
/* iplbstore.c */

extern void iplSaveAreas(
    PixmapPtr /*pPixmap*/,
    RegionPtr /*prgnSave*/,
    int /*xorg*/,
    int /*yorg*/,
    WindowPtr /*pWin*/
);

extern void iplRestoreAreas(
    PixmapPtr /*pPixmap*/,
    RegionPtr /*prgnRestore*/,
    int /*xorg*/,
    int /*yorg*/,
    WindowPtr /*pWin*/
);
/* iplcmap.c */

extern int iplListInstalledColormaps(
    ScreenPtr	/*pScreen*/,
    Colormap	* /*pmaps*/
);

extern void iplInstallColormap(
    ColormapPtr	/*pmap*/
);

extern void iplUninstallColormap(
    ColormapPtr	/*pmap*/
);

extern void iplResolveColor(
    unsigned short * /*pred*/,
    unsigned short * /*pgreen*/,
    unsigned short * /*pblue*/,
    VisualPtr /*pVisual*/
);

extern Bool iplInitializeColormap(
    ColormapPtr /*pmap*/
);

extern int iplExpandDirectColors(
    ColormapPtr /*pmap*/,
    int /*ndef*/,
    xColorItem * /*indefs*/,
    xColorItem * /*outdefs*/
);

extern Bool iplCreateDefColormap(
    ScreenPtr /*pScreen*/
);

extern Bool iplSetVisualTypes(
    int /*depth*/,
    int /*visuals*/,
    int /*bitsPerRGB*/
);

extern Bool iplInitVisuals(
    VisualPtr * /*visualp*/,
    DepthPtr * /*depthp*/,
    int * /*nvisualp*/,
    int * /*ndepthp*/,
    int * /*rootDepthp*/,
    VisualID * /*defaultVisp*/,
    unsigned long /*sizes*/,
    int /*bitsPerRGB*/
);
/* iplfillarcC.c */

extern void iplPolyFillArcSolidCopy(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
);
/* iplfillarcG.c */

extern void iplPolyFillArcSolidGeneral(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
);
/* iplfillrct.c */

extern void iplFillBoxTileOdd(
    DrawablePtr /*pDrawable*/,
    int /*n*/,
    BoxPtr /*rects*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/
);

extern void iplFillRectTileOdd(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
);

extern void iplPolyFillRect(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nrectFill*/,
    xRectangle * /*prectInit*/
);
/* iplfillsp.c */

extern void iplUnnaturalTileFS(
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);

extern void iplUnnaturalStippleFS(
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);

extern void ipl8Stipple32FS(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);

extern void ipl8OpaqueStipple32FS(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);
/* iplgc.c */

extern GCOpsPtr iplMatchCommon(
    GCPtr /*pGC*/,
    iplPrivGCPtr /*devPriv*/
);

extern Bool iplCreateGC(
    GCPtr /*pGC*/
);

extern void iplValidateGC(
    GCPtr /*pGC*/,
    unsigned long /*changes*/,
    DrawablePtr /*pDrawable*/
);

/* iplgetsp.c */

extern void iplGetSpans(
    DrawablePtr /*pDrawable*/,
    int /*wMax*/,
    DDXPointPtr /*ppt*/,
    int * /*pwidth*/,
    int /*nspans*/,
    char * /*pdstStart*/
);
/* iplglblt8.c */

extern void iplPolyGlyphBlt8(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
);
/* iplglrop8.c */

extern void iplPolyGlyphRop8(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
);
/* iplhrzvert.c */

extern int iplHorzS(
    int /*rop*/,
    unsigned short * /*and*/,
    unsigned short * /*xor*/,
    unsigned short * /*addrg*/,
    int /*nlwidth*/,
    int /*x1*/,
    int /*y1*/,
    int /*len*/
);

extern int iplVertS(
    int /*rop*/,
    unsigned short * /*and*/,
    unsigned short * /*xor*/,
    unsigned short * /*addrg*/,
    int /*nlwidth*/,
    int /*x1*/,
    int /*y1*/,
    int /*len*/
);
/* ipligblt8.c */

extern void iplImageGlyphBlt8(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
);
/* iplimage.c */

extern void iplPutImage(
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

extern void iplGetImage(
    DrawablePtr /*pDrawable*/,
    int /*sx*/,
    int /*sy*/,
    int /*w*/,
    int /*h*/,
    unsigned int /*format*/,
    unsigned long /*planeMask*/,
    char * /*pdstLine*/
);
/* iplline.c */

extern void iplLineSS(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/
);

extern void iplLineSD(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/
);
/* iplmskbits.c */
/* iplpixmap.c */

extern PixmapPtr iplCreatePixmap(
    ScreenPtr /*pScreen*/,
    int /*width*/,
    int /*height*/,
    int /*depth*/
);

extern Bool iplDestroyPixmap(
    PixmapPtr /*pPixmap*/
);

extern PixmapPtr iplCopyPixmap(
    PixmapPtr /*pSrc*/
);

extern void iplPadPixmap(
    PixmapPtr /*pPixmap*/
);

extern void iplXRotatePixmap(
    PixmapPtr /*pPix*/,
    int /*rw*/
);

extern void iplYRotatePixmap(
    PixmapPtr /*pPix*/,
    int /*rh*/
);

extern void iplCopyRotatePixmap(
    PixmapPtr /*psrcPix*/,
    PixmapPtr * /*ppdstPix*/,
    int /*xrot*/,
    int /*yrot*/
);
/* iplply1rctC.c */

extern void iplFillPoly1RectCopy(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*shape*/,
    int /*mode*/,
    int /*count*/,
    DDXPointPtr /*ptsIn*/
);
/* iplply1rctG.c */

extern void iplFillPoly1RectGeneral(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*shape*/,
    int /*mode*/,
    int /*count*/,
    DDXPointPtr /*ptsIn*/
);
/* iplpntwin.c */

extern void iplPaintWindow(
    WindowPtr /*pWin*/,
    RegionPtr /*pRegion*/,
    int /*what*/
);

extern void iplFillBoxSolid(
    DrawablePtr /*pDrawable*/,
    int /*nBox*/,
    BoxPtr /*pBox*/,
    unsigned long /*pixel*/
);

extern void iplFillBoxTile32(
    DrawablePtr /*pDrawable*/,
    int /*nBox*/,
    BoxPtr /*pBox*/,
    PixmapPtr /*tile*/
);
/* iplpolypnt.c */

extern void iplPolyPoint(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    xPoint * /*pptInit*/
);
/* iplpush8.c */

extern void iplPushPixels8(
    GCPtr /*pGC*/,
    PixmapPtr /*pBitmap*/,
    DrawablePtr /*pDrawable*/,
    int /*dx*/,
    int /*dy*/,
    int /*xOrg*/,
    int /*yOrg*/
);
/* iplrctstp8.c */

extern void ipl8FillRectOpaqueStippled32(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
);

extern void ipl8FillRectTransparentStippled32(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
);

extern void ipl8FillRectStippledUnnatural(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
);
/* iplrrop.c */

extern int iplReduceRasterOp(
    int /*rop*/,
    unsigned long /*fg*/,
    unsigned long /*pm*/,
    unsigned short * /*andp*/,
    unsigned short * /*xorp*/
);
/* iplscrinit.c */

extern Bool iplCloseScreen(
    int /*index*/,
    ScreenPtr /*pScreen*/
);

extern Bool iplSetupScreen(
    ScreenPtr /*pScreen*/,
    pointer /*pbits*/,
    int /*xsize*/,
    int /*ysize*/,
    int /*dpix*/,
    int /*dpiy*/,
    int /*width*/
);

extern int iplFinishScreenInit(
    ScreenPtr /*pScreen*/,
    pointer /*pbits*/,
    int /*xsize*/,
    int /*ysize*/,
    int /*dpix*/,
    int /*dpiy*/,
    int /*width*/
);

extern Bool iplScreenInit(
    ScreenPtr /*pScreen*/,
    pointer /*pbits*/,
    int /*xsize*/,
    int /*ysize*/,
    int /*dpix*/,
    int /*dpiy*/,
    int /*width*/
);

extern PixmapPtr iplGetScreenPixmap(
    ScreenPtr /*pScreen*/
);

extern void iplSetScreenPixmap(
    PixmapPtr /*pPix*/
);

/* iplseg.c */

extern void iplSegmentSS(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSeg*/
);

extern void iplSegmentSD(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSeg*/
);
/* iplsetsp.c */

extern int iplSetScanline(
    int /*y*/,
    int /*xOrigin*/,
    int /*xStart*/,
    int /*xEnd*/,
    unsigned int * /*psrc*/,
    int /*alu*/,
    unsigned short * /*pdstBase*/,
    int /*widthDst*/,
    unsigned long /*planemask*/
);

extern void iplSetSpans(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    char * /*psrc*/,
    DDXPointPtr /*ppt*/,
    int * /*pwidth*/,
    int /*nspans*/,
    int /*fSorted*/
);
/* iplsolidC.c */

extern void iplFillRectSolidCopy(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
);

extern void iplSolidSpansCopy(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);
/* iplsolidG.c */

extern void iplFillRectSolidGeneral(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
);

extern void iplSolidSpansGeneral(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);
/* iplsolidX.c */

extern void iplFillRectSolidXor(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
);

extern void iplSolidSpansXor(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);
/* iplteblt8.c */

extern void iplTEGlyphBlt8(
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*xInit*/,
    int /*yInit*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
);
/* ipltegblt.c */

extern void iplTEGlyphBlt(
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
);
/* ipltile32C.c */

extern void iplFillRectTile32Copy(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
);

extern void iplTile32FSCopy(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);
/* ipltile32G.c */

extern void iplFillRectTile32General(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
);

extern void iplTile32FSGeneral(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);
/* ipltileoddC.c */

extern void iplFillBoxTileOddCopy(
    DrawablePtr /*pDrawable*/,
    int /*nBox*/,
    BoxPtr /*pBox*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/,
    int /*alu*/,
    unsigned long /*planemask*/
);

extern void iplFillSpanTileOddCopy(
    DrawablePtr /*pDrawable*/,
    int /*n*/,
    DDXPointPtr /*ppt*/,
    int * /*pwidth*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/,
    int /*alu*/,
    unsigned long /*planemask*/
);

extern void iplFillBoxTile32sCopy(
    DrawablePtr /*pDrawable*/,
    int /*nBox*/,
    BoxPtr /*pBox*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/,
    int /*alu*/,
    unsigned long /*planemask*/
);

extern void iplFillSpanTile32sCopy(
    DrawablePtr /*pDrawable*/,
    int /*n*/,
    DDXPointPtr /*ppt*/,
    int * /*pwidth*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/,
    int /*alu*/,
    unsigned long /*planemask*/
);
/* ipltileoddG.c */

extern void iplFillBoxTileOddGeneral(
    DrawablePtr /*pDrawable*/,
    int /*nBox*/,
    BoxPtr /*pBox*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/,
    int /*alu*/,
    unsigned long /*planemask*/
);

extern void iplFillSpanTileOddGeneral(
    DrawablePtr /*pDrawable*/,
    int /*n*/,
    DDXPointPtr /*ppt*/,
    int * /*pwidth*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/,
    int /*alu*/,
    unsigned long /*planemask*/
);

extern void iplFillBoxTile32sGeneral(
    DrawablePtr /*pDrawable*/,
    int /*nBox*/,
    BoxPtr /*pBox*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/,
    int /*alu*/,
    unsigned long /*planemask*/
);

extern void iplFillSpanTile32sGeneral(
    DrawablePtr /*pDrawable*/,
    int /*n*/,
    DDXPointPtr /*ppt*/,
    int * /*pwidth*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/,
    int /*alu*/,
    unsigned long /*planemask*/
);
/* iplwindow.c */

extern Bool iplCreateWindow(
    WindowPtr /*pWin*/
);

extern Bool iplDestroyWindow(
    WindowPtr /*pWin*/
);

extern Bool iplMapWindow(
    WindowPtr /*pWindow*/
);

extern Bool iplPositionWindow(
    WindowPtr /*pWin*/,
    int /*x*/,
    int /*y*/
);

extern Bool iplUnmapWindow(
    WindowPtr /*pWindow*/
);

extern void iplCopyWindow(
    WindowPtr /*pWin*/,
    DDXPointRec /*ptOldOrg*/,
    RegionPtr /*prgnSrc*/
);

extern Bool iplChangeWindowAttributes(
    WindowPtr /*pWin*/,
    unsigned long /*mask*/
);
/* iplzerarcC.c */

extern void iplZeroPolyArcSS8Copy(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
);
/* iplzerarcG.c */

extern void iplZeroPolyArcSS8General(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
);
/* iplzerarcX.c */

extern void iplZeroPolyArcSS8Xor(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
);

/* Common macros for extracting drawing information */

#if (!defined(SINGLEDEPTH) && PSZ != 8) || defined(FORCE_SEPARATE_PRIVATE)

#define CFB_NEED_SCREEN_PRIVATE

extern int iplScreenPrivateIndex;
#endif

#define iplGetWindowPixmap(d) \
    ((* ((DrawablePtr)(d))->pScreen->GetWindowPixmap)((WindowPtr)(d)))

#define iplGetTypedWidth(pDrawable,wtype) (\
    (((pDrawable)->type != DRAWABLE_PIXMAP) ? \
     (int) (iplGetWindowPixmap(pDrawable)->devKind) : \
     (int)(((PixmapPtr)pDrawable)->devKind)) / sizeof (wtype))

#define iplGetByteWidth(pDrawable) iplGetTypedWidth(pDrawable, unsigned char)

#define iplGetPixelWidth(pDrawable) iplGetTypedWidth(pDrawable, PixelType)

#define iplGetLongWidth(pDrawable) iplGetTypedWidth(pDrawable, unsigned long)
    
#define iplGetTypedWidthAndPointer(pDrawable, width, pointer, wtype, ptype) {\
    PixmapPtr   _pPix; \
    if ((pDrawable)->type != DRAWABLE_PIXMAP) \
	_pPix = iplGetWindowPixmap(pDrawable); \
    else \
	_pPix = (PixmapPtr) (pDrawable); \
    (pointer) = (ptype *) _pPix->devPrivate.ptr; \
    (width) = ((int) _pPix->devKind) / sizeof (wtype); \
}

#define iplGetByteWidthAndPointer(pDrawable, width, pointer) \
    iplGetTypedWidthAndPointer(pDrawable, width, pointer, unsigned char, unsigned char)

#define iplGetLongWidthAndPointer(pDrawable, width, pointer) \
    iplGetTypedWidthAndPointer(pDrawable, width, pointer, unsigned long, unsigned long)

#define iplGetPixelWidthAndPointer(pDrawable, width, pointer) \
    iplGetTypedWidthAndPointer(pDrawable, width, pointer, PixelType, PixelType)

#define iplGetWindowTypedWidthAndPointer(pWin, width, pointer, wtype, ptype) {\
    PixmapPtr	_pPix = iplGetWindowPixmap((DrawablePtr) (pWin)); \
    (pointer) = (ptype *) _pPix->devPrivate.ptr; \
    (width) = ((int) _pPix->devKind) / sizeof (wtype); \
}

#define iplGetWindowLongWidthAndPointer(pWin, width, pointer) \
    iplGetWindowTypedWidthAndPointer(pWin, width, pointer, unsigned long, unsigned long)

#define iplGetWindowByteWidthAndPointer(pWin, width, pointer) \
    iplGetWindowTypedWidthAndPointer(pWin, width, pointer, unsigned char, unsigned char)

#define iplGetWindowPixelWidthAndPointer(pDrawable, width, pointer) \
    iplGetWindowTypedWidthAndPointer(pDrawable, width, pointer, PixelType, PixelType)

/* Macros which handle a coordinate in a single register */

/* Most compilers will convert divide by 65536 into a shift, if signed
 * shifts exist.  If your machine does arithmetic shifts and your compiler
 * can't get it right, add to this line.
 */

/* mips compiler - what a joke - it CSEs the 65536 constant into a reg
 * forcing as to use div instead of shift.  Let's be explicit.
 */

#if defined(mips) || defined(sparc) || defined(__alpha) || defined(__alpha__)
#define GetHighWord(x) (((int) (x)) >> 16)
#else
#define GetHighWord(x) (((int) (x)) / 65536)
#endif

#if IMAGE_BYTE_ORDER == MSBFirst
#define intToCoord(i,x,y)   (((x) = GetHighWord(i)), ((y) = (int) ((short) (i))))
#define coordToInt(x,y)	(((x) << 16) | (y))
#define intToX(i)	(GetHighWord(i))
#define intToY(i)	((int) ((short) i))
#else
#define intToCoord(i,x,y)   (((x) = (int) ((short) (i))), ((y) = GetHighWord(i)))
#define coordToInt(x,y)	(((y) << 16) | (x))
#define intToX(i)	((int) ((short) (i)))
#define intToY(i)	(GetHighWord(i))
#endif
