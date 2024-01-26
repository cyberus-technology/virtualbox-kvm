/*
 *
 * Copyright (C) 2000 Keith Packard, member of The XFree86 Project, Inc.
 *               2005 Zack Rusin, Trolltech
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
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#ifndef EXAPRIV_H
#define EXAPRIV_H

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "exa.h"

#include <X11/X.h>
#define NEED_EVENTS
#include <X11/Xproto.h>
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "mibstore.h"
#include "colormapst.h"
#include "gcstruct.h"
#include "input.h"
#include "mipointer.h"
#include "mi.h"
#include "dix.h"
#include "fb.h"
#include "fboverlay.h"
#ifdef RENDER
#include "fbpict.h"
#endif
#include "damage.h"

#define DEBUG_TRACE_FALL	0
#define DEBUG_MIGRATE		0
#define DEBUG_PIXMAP		0
#define DEBUG_OFFSCREEN		0

#if DEBUG_TRACE_FALL
#define EXA_FALLBACK(x)     					\
do {								\
	ErrorF("EXA fallback at %s: ", __FUNCTION__);		\
	ErrorF x;						\
} while (0)

char
exaDrawableLocation(DrawablePtr pDrawable);
#else
#define EXA_FALLBACK(x)
#endif

#if DEBUG_PIXMAP
#define DBG_PIXMAP(a) ErrorF a
#else
#define DBG_PIXMAP(a)
#endif

#ifndef EXA_MAX_FB
#define EXA_MAX_FB   FB_OVERLAY_MAX
#endif

/**
 * This is the list of migration heuristics supported by EXA.  See
 * exaDoMigration() for what their implementations do.
 */
enum ExaMigrationHeuristic {
    ExaMigrationGreedy,
    ExaMigrationAlways,
    ExaMigrationSmart
};

typedef void (*EnableDisableFBAccessProcPtr)(int, Bool);
typedef struct {
    ExaDriverPtr info;
    CreateGCProcPtr 		 SavedCreateGC;
    CloseScreenProcPtr 		 SavedCloseScreen;
    GetImageProcPtr 		 SavedGetImage;
    GetSpansProcPtr 		 SavedGetSpans;
    PaintWindowBackgroundProcPtr SavedPaintWindowBackground;
    CreatePixmapProcPtr 	 SavedCreatePixmap;
    DestroyPixmapProcPtr 	 SavedDestroyPixmap;
    PaintWindowBorderProcPtr 	 SavedPaintWindowBorder;
    CopyWindowProcPtr 		 SavedCopyWindow;
#ifdef RENDER
    CompositeProcPtr             SavedComposite;
    RasterizeTrapezoidProcPtr	 SavedRasterizeTrapezoid;
    AddTrianglesProcPtr		 SavedAddTriangles;
    GlyphsProcPtr                SavedGlyphs;
    TrapezoidsProcPtr            SavedTrapezoids;
#endif
    Bool			 swappedOut;
    enum ExaMigrationHeuristic	 migration;
    Bool			 hideOffscreenPixmapData;
    Bool			 checkDirtyCorrectness;
    unsigned			 disableFbCount;
} ExaScreenPrivRec, *ExaScreenPrivPtr;

/*
 * This is the only completely portable way to
 * compute this info.
 */
#ifndef BitsPerPixel
#define BitsPerPixel(d) (\
    PixmapWidthPaddingInfo[d].notPower2 ? \
    (PixmapWidthPaddingInfo[d].bytesPerPixel * 8) : \
    ((1 << PixmapWidthPaddingInfo[d].padBytesLog2) * 8 / \
    (PixmapWidthPaddingInfo[d].padRoundUp+1)))
#endif

extern int exaScreenPrivateIndex;
extern int exaPixmapPrivateIndex;
#define ExaGetScreenPriv(s)	((ExaScreenPrivPtr)(s)->devPrivates[exaScreenPrivateIndex].ptr)
#define ExaScreenPriv(s)	ExaScreenPrivPtr    pExaScr = ExaGetScreenPriv(s)

/** Align an offset to an arbitrary alignment */
#define EXA_ALIGN(offset, align) (((offset) + (align) - 1) - \
	(((offset) + (align) - 1) % (align)))
/** Align an offset to a power-of-two alignment */
#define EXA_ALIGN2(offset, align) (((offset) + (align) - 1) & ~((align) - 1))

#define EXA_PIXMAP_SCORE_MOVE_IN    10
#define EXA_PIXMAP_SCORE_MAX	    20
#define EXA_PIXMAP_SCORE_MOVE_OUT   -10
#define EXA_PIXMAP_SCORE_MIN	    -20
#define EXA_PIXMAP_SCORE_PINNED	    1000
#define EXA_PIXMAP_SCORE_INIT	    1001

#define ExaGetPixmapPriv(p)	((ExaPixmapPrivPtr)(p)->devPrivates[exaPixmapPrivateIndex].ptr)
#define ExaSetPixmapPriv(p,a)	((p)->devPrivates[exaPixmapPrivateIndex].ptr = (pointer) (a))
#define ExaPixmapPriv(p)	ExaPixmapPrivPtr pExaPixmap = ExaGetPixmapPriv(p)

typedef struct {
    ExaOffscreenArea *area;
    int		    score;	/**< score for the move-in vs move-out heuristic */

    CARD8	    *sys_ptr;	/**< pointer to pixmap data in system memory */
    int		    sys_pitch;	/**< pitch of pixmap in system memory */

    CARD8	    *fb_ptr;	/**< pointer to pixmap data in framebuffer memory */
    int		    fb_pitch;	/**< pitch of pixmap in framebuffer memory */
    unsigned int    fb_size;	/**< size of pixmap in framebuffer memory */

    /**
     * The damage record contains the areas of the pixmap's current location
     * (framebuffer or system) that have been damaged compared to the other
     * location.
     */
    DamagePtr	    pDamage;
    /**
     * The valid region marks the valid bits of a drawable (at least, as it's
     * derived from damage, which may be overreported).
     */
    RegionRec	    validReg;
} ExaPixmapPrivRec, *ExaPixmapPrivPtr;
 
typedef struct _ExaMigrationRec {
    Bool as_dst;
    Bool as_src;
    PixmapPtr pPix;
} ExaMigrationRec, *ExaMigrationPtr;

/**
 * exaDDXDriverInit must be implemented by the DDX using EXA, and is the place
 * to set EXA options or hook in screen functions to handle using EXA as the AA.
  */
void exaDDXDriverInit (ScreenPtr pScreen);

/* exa_unaccel.c */
void
exaPrepareAccessGC(GCPtr pGC);

void
exaFinishAccessGC(GCPtr pGC);

void
ExaCheckFillSpans  (DrawablePtr pDrawable, GCPtr pGC, int nspans,
		   DDXPointPtr ppt, int *pwidth, int fSorted);

void
ExaCheckSetSpans (DrawablePtr pDrawable, GCPtr pGC, char *psrc,
		 DDXPointPtr ppt, int *pwidth, int nspans, int fSorted);

void
ExaCheckPutImage (DrawablePtr pDrawable, GCPtr pGC, int depth,
		 int x, int y, int w, int h, int leftPad, int format,
		 char *bits);

RegionPtr
ExaCheckCopyArea (DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
		 int srcx, int srcy, int w, int h, int dstx, int dsty);

RegionPtr
ExaCheckCopyPlane (DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
		  int srcx, int srcy, int w, int h, int dstx, int dsty,
		  unsigned long bitPlane);

void
ExaCheckPolyPoint (DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
		  DDXPointPtr pptInit);

void
ExaCheckPolylines (DrawablePtr pDrawable, GCPtr pGC,
		  int mode, int npt, DDXPointPtr ppt);

void
ExaCheckPolySegment (DrawablePtr pDrawable, GCPtr pGC,
		    int nsegInit, xSegment *pSegInit);

void
ExaCheckPolyArc (DrawablePtr pDrawable, GCPtr pGC,
		int narcs, xArc *pArcs);

void
ExaCheckPolyFillRect (DrawablePtr pDrawable, GCPtr pGC,
		     int nrect, xRectangle *prect);

void
ExaCheckImageGlyphBlt (DrawablePtr pDrawable, GCPtr pGC,
		      int x, int y, unsigned int nglyph,
		      CharInfoPtr *ppci, pointer pglyphBase);

void
ExaCheckPolyGlyphBlt (DrawablePtr pDrawable, GCPtr pGC,
		     int x, int y, unsigned int nglyph,
		     CharInfoPtr *ppci, pointer pglyphBase);

void
ExaCheckPushPixels (GCPtr pGC, PixmapPtr pBitmap,
		   DrawablePtr pDrawable,
		   int w, int h, int x, int y);

void
ExaCheckGetImage (DrawablePtr pDrawable,
		 int x, int y, int w, int h,
		 unsigned int format, unsigned long planeMask,
		 char *d);

void
ExaCheckGetSpans (DrawablePtr pDrawable,
		 int wMax,
		 DDXPointPtr ppt,
		 int *pwidth,
		 int nspans,
		 char *pdstStart);

void
ExaCheckSaveAreas (PixmapPtr	pPixmap,
		  RegionPtr	prgnSave,
		  int		xorg,
		  int		yorg,
		  WindowPtr	pWin);

void
ExaCheckRestoreAreas (PixmapPtr	pPixmap,
		     RegionPtr	prgnSave,
		     int	xorg,
		     int    	yorg,
		     WindowPtr	pWin);

void
ExaCheckPaintWindow (WindowPtr pWin, RegionPtr pRegion, int what);

CARD32
exaGetPixmapFirstPixel (PixmapPtr pPixmap); 

/* exa_accel.c */
void
exaCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc);

Bool
exaFillRegionTiled (DrawablePtr	pDrawable, RegionPtr pRegion, PixmapPtr pTile,
		    DDXPointPtr pPatOrg, CARD32 planemask, CARD32 alu);

void
exaPaintWindow(WindowPtr pWin, RegionPtr pRegion, int what);

void
exaGetImage (DrawablePtr pDrawable, int x, int y, int w, int h,
	     unsigned int format, unsigned long planeMask, char *d);

void
exaGetSpans (DrawablePtr pDrawable, int wMax, DDXPointPtr ppt, int *pwidth,
	     int nspans, char *pdstStart);

extern const GCOps exaOps;

#ifdef RENDER
void
ExaCheckComposite (CARD8      op,
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
#endif

/* exa_offscreen.c */
void
ExaOffscreenSwapOut (ScreenPtr pScreen);

void
ExaOffscreenSwapIn (ScreenPtr pScreen);

Bool
exaOffscreenInit(ScreenPtr pScreen);

void
ExaOffscreenFini (ScreenPtr pScreen);

/* exa.c */
void
exaPrepareAccess(DrawablePtr pDrawable, int index);

void
exaFinishAccess(DrawablePtr pDrawable, int index);

void
exaPixmapDirty(PixmapPtr pPix, int x1, int y1, int x2, int y2);

void
exaGetDrawableDeltas (DrawablePtr pDrawable, PixmapPtr pPixmap,
		      int *xp, int *yp);

Bool
exaDrawableIsOffscreen (DrawablePtr pDrawable);

Bool
exaPixmapIsOffscreen(PixmapPtr p);

PixmapPtr
exaGetOffscreenPixmap (DrawablePtr pDrawable, int *xp, int *yp);

PixmapPtr
exaGetDrawablePixmap(DrawablePtr pDrawable);

RegionPtr
exaCopyArea(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable, GCPtr pGC,
	    int srcx, int srcy, int width, int height, int dstx, int dsty);

void
exaCopyNtoN (DrawablePtr    pSrcDrawable,
	     DrawablePtr    pDstDrawable,
	     GCPtr	    pGC,
	     BoxPtr	    pbox,
	     int	    nbox,
	     int	    dx,
	     int	    dy,
	     Bool	    reverse,
	     Bool	    upsidedown,
	     Pixel	    bitplane,
	     void	    *closure);

/* exa_render.c */
void
exaComposite(CARD8	op,
	     PicturePtr pSrc,
	     PicturePtr pMask,
	     PicturePtr pDst,
	     INT16	xSrc,
	     INT16	ySrc,
	     INT16	xMask,
	     INT16	yMask,
	     INT16	xDst,
	     INT16	yDst,
	     CARD16	width,
	     CARD16	height);

void
exaTrapezoids (CARD8 op, PicturePtr pSrc, PicturePtr pDst,
               PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
               int ntrap, xTrapezoid *traps);

void
exaRasterizeTrapezoid (PicturePtr pPicture, xTrapezoid  *trap,
		       int x_off, int y_off);

void
exaAddTriangles (PicturePtr pPicture, INT16 x_off, INT16 y_off, int ntri,
		 xTriangle *tris);

void
exaGlyphs (CARD8	op,
	  PicturePtr	pSrc,
	  PicturePtr	pDst,
	  PictFormatPtr	maskFormat,
	  INT16		xSrc,
	  INT16		ySrc,
	  int		nlist,
	  GlyphListPtr	list,
	  GlyphPtr	*glyphs);

/* exa_migration.c */
void
exaDoMigration (ExaMigrationPtr pixmaps, int npixmaps, Bool can_accel);

void
exaPixmapSave (ScreenPtr pScreen, ExaOffscreenArea *area);

#endif /* EXAPRIV_H */
