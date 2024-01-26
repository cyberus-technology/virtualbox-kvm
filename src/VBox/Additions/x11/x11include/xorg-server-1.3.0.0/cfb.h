/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or The Open Group
not be used in advertising or publicity pertaining to 
distribution  of  the software  without specific prior 
written permission. Sun and The Open Group make no 
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

#if !defined(__CFB_H__) || defined(CFB_PROTOTYPES_ONLY)

#include <X11/X.h>
#include "globals.h"
#include "pixmap.h"
#include "region.h"
#include "gc.h"
#include "colormap.h"
#include "miscstruct.h"
#include "servermd.h"
#include "windowstr.h"
#include "mfb.h"
#undef PixelType

#include "cfbmap.h"

#ifndef CfbBits
#define CfbBits CARD32
#endif

#ifndef CFB_PROTOTYPES_ONLY
#define __CFB_H__
/*
   private filed of pixmap
   pixmap.devPrivate = (unsigned int *)pointer_to_bits
   pixmap.devKind = width_of_pixmap_in_bytes
*/

extern int  cfbGCPrivateIndex;
extern int  cfbWindowPrivateIndex;

/* private field of GC */
typedef struct {
    unsigned char       rop;            /* special case rop values */
    /* next two values unused in cfb, included for compatibility with mfb */
    unsigned char       ropOpStip;      /* rop for opaque stipple */
    /* this value is ropFillArea in mfb, usurped for cfb */
    unsigned char       oneRect;	/*  drawable has one clip rect */
    CfbBits	xor, and;	/* reduced rop values */
    } cfbPrivGC;

typedef cfbPrivGC	*cfbPrivGCPtr;

#define cfbGetGCPrivate(pGC)	((cfbPrivGCPtr)\
	(pGC)->devPrivates[cfbGCPrivateIndex].ptr)

#define cfbGetCompositeClip(pGC) ((pGC)->pCompositeClip)

/* way to carry RROP info around */
typedef struct {
    unsigned char	rop;
    CfbBits	xor, and;
} cfbRRopRec, *cfbRRopPtr;

/* private field of window */
typedef struct {
    unsigned	char fastBorder; /* non-zero if border is 32 bits wide */
    unsigned	char fastBackground;
    unsigned short unused; /* pad for alignment with Sun compiler */
    DDXPointRec	oldRotate;
    PixmapPtr	pRotatedBackground;
    PixmapPtr	pRotatedBorder;
    } cfbPrivWin;

#define cfbGetWindowPrivate(_pWin) ((cfbPrivWin *)\
	(_pWin)->devPrivates[cfbWindowPrivateIndex].ptr)


/* cfb8bit.c */

extern int cfbSetStipple(
    int /*alu*/,
    CfbBits /*fg*/,
    CfbBits /*planemask*/
);

extern int cfbSetOpaqueStipple(
    int /*alu*/,
    CfbBits /*fg*/,
    CfbBits /*bg*/,
    CfbBits /*planemask*/
);

extern int cfbComputeClipMasks32(
    BoxPtr /*pBox*/,
    int /*numRects*/,
    int /*x*/,
    int /*y*/,
    int /*w*/,
    int /*h*/,
    CARD32 * /*clips*/
);
#endif /* !CFB_PROTOTYPES_ONLY */
/* cfb8cppl.c */

extern void cfbCopyImagePlane(
    DrawablePtr /*pSrcDrawable*/,
    DrawablePtr /*pDstDrawable*/,
    int /*rop*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/
);

#ifndef CFB_PROTOTYPES_ONLY
extern void cfbCopyPlane8to1(
    DrawablePtr /*pSrcDrawable*/,
    DrawablePtr /*pDstDrawable*/,
    int /*rop*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/,
    unsigned long /*bitPlane*/
);

extern void cfbCopyPlane16to1(
    DrawablePtr /*pSrcDrawable*/,
    DrawablePtr /*pDstDrawable*/,
    int /*rop*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/,
    unsigned long /*bitPlane*/
);

extern void cfbCopyPlane24to1(
    DrawablePtr /*pSrcDrawable*/,
    DrawablePtr /*pDstDrawable*/,
    int /*rop*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/,
    unsigned long /*bitPlane*/
);

extern void cfbCopyPlane32to1(
    DrawablePtr /*pSrcDrawable*/,
    DrawablePtr /*pDstDrawable*/,
    int /*rop*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/,
    unsigned long /*bitPlane*/
);
#endif

/* cfb8lineCO.c */

extern int cfb8LineSS1RectCopy(
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

extern void cfb8LineSS1Rect(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/
);

extern void cfb8ClippedLineCopy(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x1*/,
    int /*y1*/,
    int /*x2*/,
    int /*y2*/,
    BoxPtr /*boxp*/,
    Bool /*shorten*/
);
/* cfb8lineCP.c */

extern int cfb8LineSS1RectPreviousCopy(
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
/* cfb8lineG.c */

extern int cfb8LineSS1RectGeneral(
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

extern void cfb8ClippedLineGeneral(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x1*/,
    int /*y1*/,
    int /*x2*/,
    int /*y2*/,
    BoxPtr /*boxp*/,
    Bool /*shorten*/
);
/* cfb8lineX.c */

extern int cfb8LineSS1RectXor(
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

extern void cfb8ClippedLineXor(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x1*/,
    int /*y1*/,
    int /*x2*/,
    int /*y2*/,
    BoxPtr /*boxp*/,
    Bool /*shorten*/
);
/* cfb8segC.c */

extern int cfb8SegmentSS1RectCopy(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSegInit*/
);
/* cfb8segCS.c */

extern int cfb8SegmentSS1RectShiftCopy(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSegInit*/
);

extern void cfb8SegmentSS1Rect(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSegInit*/
);
/* cfb8segG.c */

extern int cfb8SegmentSS1RectGeneral(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSegInit*/
);
/* cfbsegX.c */

extern int cfb8SegmentSS1RectXor(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSegInit*/
);
/* cfballpriv.c */

extern Bool cfbAllocatePrivates(
    ScreenPtr /*pScreen*/,
    int * /*window_index*/,
    int * /*gc_index*/
);
/* cfbbitblt.c */

extern RegionPtr cfbBitBlt(
    DrawablePtr /*pSrcDrawable*/,
    DrawablePtr /*pDstDrawable*/,
    GCPtr/*pGC*/,
    int /*srcx*/,
    int /*srcy*/,
    int /*width*/,
    int /*height*/,
    int /*dstx*/,
    int /*dsty*/,
    void (* /*doBitBlt*/)(
	DrawablePtr /*pSrc*/,
	DrawablePtr /*pDst*/,
	int /*alu*/,
	RegionPtr /*prgnDst*/,
	DDXPointPtr /*pptSrc*/,
	unsigned long /*planemask*/
	),
    unsigned long /*bitPlane*/
);

#define cfbCopyPlaneExpand cfbBitBlt

extern RegionPtr cfbCopyPlaneReduce(
    DrawablePtr /*pSrcDrawable*/,
    DrawablePtr /*pDstDrawable*/,
    GCPtr /*pGC*/,
    int /*srcx*/,
    int /*srcy*/,
    int /*width*/,
    int /*height*/,
    int /*dstx*/,
    int /*dsty*/,
    void (* /*doCopyPlane*/)(
	DrawablePtr /*pSrc*/,
	DrawablePtr /*pDst*/,
	int /*alu*/,
	RegionPtr /*prgnDst*/,
	DDXPointPtr /*pptSrc*/,
	unsigned long /*planemask*/,
	unsigned long /*bitPlane*/ /* We must know which plane to reduce! */
	),
    unsigned long /*bitPlane*/
);

extern void cfbDoBitblt(
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/
);

extern RegionPtr cfbCopyArea(
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

#ifndef CFB_PROTOTYPES_ONLY
extern void cfbCopyPlane1to8(
    DrawablePtr /*pSrcDrawable*/,
    DrawablePtr /*pDstDrawable*/,
    int /*rop*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/
);
#endif

extern RegionPtr cfbCopyPlane(
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
/* cfbbltC.c */

extern void cfbDoBitbltCopy(
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/
);
/* cfbbltG.c */

extern void cfbDoBitbltGeneral(
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/
);
/* cfbbltO.c */

extern void cfbDoBitbltOr(
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/
);
/* cfbbltX.c */

extern void cfbDoBitbltXor(
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/
);
/* cfbbres.c */

extern void cfbBresS(
    int /*rop*/,
    CfbBits /*and*/,
    CfbBits /*xor*/,
    CfbBits * /*addrl*/,
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
/* cfbbresd.c */

extern void cfbBresD(
    cfbRRopPtr /*rrops*/,
    int * /*pdashIndex*/,
    unsigned char * /*pDash*/,
    int /*numInDashList*/,
    int * /*pdashOffset*/,
    int /*isDoubleDash*/,
    CfbBits * /*addrl*/,
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
/* cfbbstore.c */

extern void cfbSaveAreas(
    PixmapPtr /*pPixmap*/,
    RegionPtr /*prgnSave*/,
    int /*xorg*/,
    int /*yorg*/,
    WindowPtr /*pWin*/
);

extern void cfbRestoreAreas(
    PixmapPtr /*pPixmap*/,
    RegionPtr /*prgnRestore*/,
    int /*xorg*/,
    int /*yorg*/,
    WindowPtr /*pWin*/
);
/* cfbcmap.c */

#ifndef CFB_PROTOTYPES_ONLY
extern int cfbListInstalledColormaps(
    ScreenPtr	/*pScreen*/,
    Colormap	* /*pmaps*/
);

extern void cfbInstallColormap(
    ColormapPtr	/*pmap*/
);

extern void cfbUninstallColormap(
    ColormapPtr	/*pmap*/
);

extern void cfbResolveColor(
    unsigned short * /*pred*/,
    unsigned short * /*pgreen*/,
    unsigned short * /*pblue*/,
    VisualPtr /*pVisual*/
);

extern Bool cfbInitializeColormap(
    ColormapPtr /*pmap*/
);

extern int cfbExpandDirectColors(
    ColormapPtr /*pmap*/,
    int /*ndef*/,
    xColorItem * /*indefs*/,
    xColorItem * /*outdefs*/
);

extern Bool cfbCreateDefColormap(
    ScreenPtr /*pScreen*/
);

extern Bool cfbSetVisualTypes(
    int /*depth*/,
    int /*visuals*/,
    int /*bitsPerRGB*/
);

extern void cfbClearVisualTypes(void);

extern Bool cfbInitVisuals(
    VisualPtr * /*visualp*/,
    DepthPtr * /*depthp*/,
    int * /*nvisualp*/,
    int * /*ndepthp*/,
    int * /*rootDepthp*/,
    VisualID * /*defaultVisp*/,
    unsigned long /*sizes*/,
    int /*bitsPerRGB*/
);
#endif
/* cfbfillarcC.c */

extern void cfbPolyFillArcSolidCopy(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
);
/* cfbfillarcG.c */

extern void cfbPolyFillArcSolidGeneral(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
);
/* cfbfillrct.c */

extern void cfbFillBoxTileOdd(
    DrawablePtr /*pDrawable*/,
    int /*n*/,
    BoxPtr /*rects*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/
);

extern void cfbFillRectTileOdd(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
);

extern void cfbPolyFillRect(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nrectFill*/,
    xRectangle * /*prectInit*/
);
/* cfbfillsp.c */

extern void cfbUnnaturalTileFS(
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);

extern void cfbUnnaturalStippleFS(
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);

#ifndef CFB_PROTOTYPES_ONLY
extern void cfb8Stipple32FS(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);

extern void cfb8OpaqueStipple32FS(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);
#endif
/* cfbgc.c */

extern GCOpsPtr cfbMatchCommon(
    GCPtr /*pGC*/,
    cfbPrivGCPtr /*devPriv*/
);

extern Bool cfbCreateGC(
    GCPtr /*pGC*/
);

extern void cfbValidateGC(
    GCPtr /*pGC*/,
    unsigned long /*changes*/,
    DrawablePtr /*pDrawable*/
);

/* cfbgetsp.c */

extern void cfbGetSpans(
    DrawablePtr /*pDrawable*/,
    int /*wMax*/,
    DDXPointPtr /*ppt*/,
    int * /*pwidth*/,
    int /*nspans*/,
    char * /*pdstStart*/
);
/* cfbglblt8.c */

extern void cfbPolyGlyphBlt8(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
);
/* cfbglrop8.c */

extern void cfbPolyGlyphRop8(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
);
/* cfbhrzvert.c */

extern void cfbHorzS(
    int /*rop*/,
    CfbBits /*and*/,
    CfbBits /*xor*/,
    CfbBits * /*addrl*/,
    int /*nlwidth*/,
    int /*x1*/,
    int /*y1*/,
    int /*len*/
);

extern void cfbVertS(
    int /*rop*/,
    CfbBits /*and*/,
    CfbBits /*xor*/,
    CfbBits * /*addrl*/,
    int /*nlwidth*/,
    int /*x1*/,
    int /*y1*/,
    int /*len*/
);
/* cfbigblt8.c */

extern void cfbImageGlyphBlt8(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
);
/* cfbimage.c */

extern void cfbPutImage(
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

extern void cfbGetImage(
    DrawablePtr /*pDrawable*/,
    int /*sx*/,
    int /*sy*/,
    int /*w*/,
    int /*h*/,
    unsigned int /*format*/,
    unsigned long /*planeMask*/,
    char * /*pdstLine*/
);
/* cfbline.c */

extern void cfbLineSS(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/
);

extern void cfbLineSD(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/
);
/* cfbmskbits.c */
/* cfbpixmap.c */

extern PixmapPtr cfbCreatePixmap(
    ScreenPtr /*pScreen*/,
    int /*width*/,
    int /*height*/,
    int /*depth*/
);

extern Bool cfbDestroyPixmap(
    PixmapPtr /*pPixmap*/
);

extern PixmapPtr cfbCopyPixmap(
    PixmapPtr /*pSrc*/
);

extern void cfbPadPixmap(
    PixmapPtr /*pPixmap*/
);

extern void cfbXRotatePixmap(
    PixmapPtr /*pPix*/,
    int /*rw*/
);

extern void cfbYRotatePixmap(
    PixmapPtr /*pPix*/,
    int /*rh*/
);

extern void cfbCopyRotatePixmap(
    PixmapPtr /*psrcPix*/,
    PixmapPtr * /*ppdstPix*/,
    int /*xrot*/,
    int /*yrot*/
);
/* cfbply1rctC.c */

extern void cfbFillPoly1RectCopy(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*shape*/,
    int /*mode*/,
    int /*count*/,
    DDXPointPtr /*ptsIn*/
);
/* cfbply1rctG.c */

extern void cfbFillPoly1RectGeneral(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*shape*/,
    int /*mode*/,
    int /*count*/,
    DDXPointPtr /*ptsIn*/
);
/* cfbpntwin.c */

extern void cfbPaintWindow(
    WindowPtr /*pWin*/,
    RegionPtr /*pRegion*/,
    int /*what*/
);

extern void cfbFillBoxSolid(
    DrawablePtr /*pDrawable*/,
    int /*nBox*/,
    BoxPtr /*pBox*/,
    unsigned long /*pixel*/
);

extern void cfbFillBoxTile32(
    DrawablePtr /*pDrawable*/,
    int /*nBox*/,
    BoxPtr /*pBox*/,
    PixmapPtr /*tile*/
);
/* cfbpolypnt.c */

extern void cfbPolyPoint(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    xPoint * /*pptInit*/
);
/* cfbpush8.c */

#ifndef CFB_PROTOTYPES_ONLY
extern void cfbPushPixels8(
    GCPtr /*pGC*/,
    PixmapPtr /*pBitmap*/,
    DrawablePtr /*pDrawable*/,
    int /*dx*/,
    int /*dy*/,
    int /*xOrg*/,
    int /*yOrg*/
);
/* cfbrctstp8.c */

extern void cfb8FillRectOpaqueStippled32(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
);

extern void cfb8FillRectTransparentStippled32(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
);

extern void cfb8FillRectStippledUnnatural(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
);
#endif
/* cfbrrop.c */

extern int cfbReduceRasterOp(
    int /*rop*/,
    CfbBits /*fg*/,
    CfbBits /*pm*/,
    CfbBits * /*andp*/,
    CfbBits * /*xorp*/
);
/* cfbscrinit.c */

extern Bool cfbCloseScreen(
    int /*index*/,
    ScreenPtr /*pScreen*/
);

extern Bool cfbSetupScreen(
    ScreenPtr /*pScreen*/,
    pointer /*pbits*/,
    int /*xsize*/,
    int /*ysize*/,
    int /*dpix*/,
    int /*dpiy*/,
    int /*width*/
);

extern Bool cfbFinishScreenInit(
    ScreenPtr /*pScreen*/,
    pointer /*pbits*/,
    int /*xsize*/,
    int /*ysize*/,
    int /*dpix*/,
    int /*dpiy*/,
    int /*width*/
);

extern Bool cfbScreenInit(
    ScreenPtr /*pScreen*/,
    pointer /*pbits*/,
    int /*xsize*/,
    int /*ysize*/,
    int /*dpix*/,
    int /*dpiy*/,
    int /*width*/
);

extern PixmapPtr cfbGetScreenPixmap(
    ScreenPtr /*pScreen*/
);

extern void cfbSetScreenPixmap(
    PixmapPtr /*pPix*/
);

/* cfbseg.c */

extern void cfbSegmentSS(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSeg*/
);

extern void cfbSegmentSD(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSeg*/
);
/* cfbsetsp.c */

extern void cfbSetScanline(
    int /*y*/,
    int /*xOrigin*/,
    int /*xStart*/,
    int /*xEnd*/,
    unsigned int * /*psrc*/,
    int /*alu*/,
    int * /*pdstBase*/,
    int /*widthDst*/,
    unsigned long /*planemask*/
);

extern void cfbSetSpans(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    char * /*psrc*/,
    DDXPointPtr /*ppt*/,
    int * /*pwidth*/,
    int /*nspans*/,
    int /*fSorted*/
);
/* cfbsolidC.c */

extern void cfbFillRectSolidCopy(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
);

extern void cfbSolidSpansCopy(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);
/* cfbsolidG.c */

extern void cfbFillRectSolidGeneral(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
);

extern void cfbSolidSpansGeneral(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);
/* cfbsolidX.c */

extern void cfbFillRectSolidXor(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
);

extern void cfbSolidSpansXor(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);
/* cfbteblt8.c */

#ifndef CFB_PROTOTYPES_ONLY
extern void cfbTEGlyphBlt8(
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*xInit*/,
    int /*yInit*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
);
#endif
/* cfbtegblt.c */

extern void cfbTEGlyphBlt(
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*x*/,
    int /*y*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
);
/* cfbtile32C.c */

extern void cfbFillRectTile32Copy(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
);

extern void cfbTile32FSCopy(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);
/* cfbtile32G.c */

extern void cfbFillRectTile32General(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
);

extern void cfbTile32FSGeneral(
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
);
/* cfbtileoddC.c */

extern void cfbFillBoxTileOddCopy(
    DrawablePtr /*pDrawable*/,
    int /*nBox*/,
    BoxPtr /*pBox*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/,
    int /*alu*/,
    unsigned long /*planemask*/
);

extern void cfbFillSpanTileOddCopy(
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

extern void cfbFillBoxTile32sCopy(
    DrawablePtr /*pDrawable*/,
    int /*nBox*/,
    BoxPtr /*pBox*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/,
    int /*alu*/,
    unsigned long /*planemask*/
);

extern void cfbFillSpanTile32sCopy(
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
/* cfbtileoddG.c */

extern void cfbFillBoxTileOddGeneral(
    DrawablePtr /*pDrawable*/,
    int /*nBox*/,
    BoxPtr /*pBox*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/,
    int /*alu*/,
    unsigned long /*planemask*/
);

extern void cfbFillSpanTileOddGeneral(
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

extern void cfbFillBoxTile32sGeneral(
    DrawablePtr /*pDrawable*/,
    int /*nBox*/,
    BoxPtr /*pBox*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/,
    int /*alu*/,
    unsigned long /*planemask*/
);

extern void cfbFillSpanTile32sGeneral(
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
/* cfbwindow.c */

extern Bool cfbCreateWindow(
    WindowPtr /*pWin*/
);

extern Bool cfbDestroyWindow(
    WindowPtr /*pWin*/
);

extern Bool cfbMapWindow(
    WindowPtr /*pWindow*/
);

extern Bool cfbPositionWindow(
    WindowPtr /*pWin*/,
    int /*x*/,
    int /*y*/
);

extern Bool cfbUnmapWindow(
    WindowPtr /*pWindow*/
);

extern void cfbCopyWindow(
    WindowPtr /*pWin*/,
    DDXPointRec /*ptOldOrg*/,
    RegionPtr /*prgnSrc*/
);

extern Bool cfbChangeWindowAttributes(
    WindowPtr /*pWin*/,
    unsigned long /*mask*/
);
/* cfbzerarcC.c */

extern void cfbZeroPolyArcSS8Copy(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
);
/* cfbzerarcG.c */

extern void cfbZeroPolyArcSS8General(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
);
/* cfbzerarcX.c */

extern void cfbZeroPolyArcSS8Xor(
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
);

#if (!defined(SINGLEDEPTH) && PSZ != 8) || defined(FORCE_SEPARATE_PRIVATE)

#define CFB_NEED_SCREEN_PRIVATE

extern int cfbScreenPrivateIndex;
#endif

#ifndef CFB_PROTOTYPES_ONLY

/* Common macros for extracting drawing information */

#define cfbGetWindowPixmap(d) \
    ((* ((DrawablePtr)(d))->pScreen->GetWindowPixmap)((WindowPtr)(d)))

#define cfbGetTypedWidth(pDrawable,wtype) (\
    (((pDrawable)->type != DRAWABLE_PIXMAP) ? \
     (int) (cfbGetWindowPixmap(pDrawable)->devKind) : \
     (int)(((PixmapPtr)pDrawable)->devKind)) / sizeof (wtype))

#define cfbGetByteWidth(pDrawable) cfbGetTypedWidth(pDrawable, unsigned char)

#define cfbGetPixelWidth(pDrawable) cfbGetTypedWidth(pDrawable, PixelType)

#define cfbGetLongWidth(pDrawable) cfbGetTypedWidth(pDrawable, CfbBits)
    
#define cfbGetTypedWidthAndPointer(pDrawable, width, pointer, wtype, ptype) {\
    PixmapPtr   _pPix; \
    if ((pDrawable)->type != DRAWABLE_PIXMAP) \
	_pPix = cfbGetWindowPixmap(pDrawable); \
    else \
	_pPix = (PixmapPtr) (pDrawable); \
    (pointer) = (ptype *) _pPix->devPrivate.ptr; \
    (width) = ((int) _pPix->devKind) / sizeof (wtype); \
}

#define cfbGetByteWidthAndPointer(pDrawable, width, pointer) \
    cfbGetTypedWidthAndPointer(pDrawable, width, pointer, unsigned char, unsigned char)

#define cfbGetLongWidthAndPointer(pDrawable, width, pointer) \
    cfbGetTypedWidthAndPointer(pDrawable, width, pointer, CfbBits, CfbBits)

#define cfbGetPixelWidthAndPointer(pDrawable, width, pointer) \
    cfbGetTypedWidthAndPointer(pDrawable, width, pointer, PixelType, PixelType)

#define cfbGetWindowTypedWidthAndPointer(pWin, width, pointer, wtype, ptype) {\
    PixmapPtr	_pPix = cfbGetWindowPixmap((DrawablePtr) (pWin)); \
    (pointer) = (ptype *) _pPix->devPrivate.ptr; \
    (width) = ((int) _pPix->devKind) / sizeof (wtype); \
}

#define cfbGetWindowLongWidthAndPointer(pWin, width, pointer) \
    cfbGetWindowTypedWidthAndPointer(pWin, width, pointer, CfbBits, CfbBits)

#define cfbGetWindowByteWidthAndPointer(pWin, width, pointer) \
    cfbGetWindowTypedWidthAndPointer(pWin, width, pointer, unsigned char, unsigned char)

#define cfbGetWindowPixelWidthAndPointer(pDrawable, width, pointer) \
    cfbGetWindowTypedWidthAndPointer(pDrawable, width, pointer, PixelType, PixelType)

/*
 * XFree86 empties the root BorderClip when the VT is inactive,
 * here's a macro which uses that to disable GetImage and GetSpans
 */
#define cfbWindowEnabled(pWin) \
    REGION_NOTEMPTY((pWin)->drawable.pScreen, \
		    &WindowTable[(pWin)->drawable.pScreen->myNum]->borderClip)

#define cfbDrawableEnabled(pDrawable) \
    ((pDrawable)->type == DRAWABLE_PIXMAP ? \
     TRUE : cfbWindowEnabled((WindowPtr) pDrawable))

#include "micoord.h"

#endif /* !CFB_PROTOTYPES_ONLY */

#endif
