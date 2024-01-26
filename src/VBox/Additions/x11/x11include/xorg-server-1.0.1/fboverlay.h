/*
 * $XFree86: xc/programs/Xserver/fb/fboverlay.h,v 1.4tsi Exp $
 *
 * Copyright © 2000 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#ifndef _FBOVERLAY_H_
#define _FBOVERLAY_H_

extern int	fbOverlayGeneration;
extern int	fbOverlayScreenPrivateIndex; /* XXX should be static */
extern int	fbOverlayGetScreenPrivateIndex(void);

#ifndef FB_OVERLAY_MAX
#define FB_OVERLAY_MAX	2
#endif

typedef	void	(*fbOverlayPaintKeyProc) (DrawablePtr, RegionPtr, CARD32, int);

typedef struct _fbOverlayLayer {
    union {
	struct {
	    pointer	pbits;
	    int		width;
	    int		depth;
	} init;
	struct {
	    PixmapPtr	pixmap;
	    RegionRec	region;
	} run;
    } u;
    CARD32	key;	    /* special pixel value */
} FbOverlayLayer;

typedef struct _fbOverlayScrPriv {
    int			    nlayers;
    fbOverlayPaintKeyProc   PaintKey;
    fbCopyProc		    CopyWindow;
    FbOverlayLayer	    layer[FB_OVERLAY_MAX];
} FbOverlayScrPrivRec, *FbOverlayScrPrivPtr;

#define fbOverlayGetScrPriv(s) \
    ((fbOverlayGetScreenPrivateIndex() != -1) ? \
     (s)->devPrivates[fbOverlayGetScreenPrivateIndex()].ptr : NULL)
Bool
fbOverlayCreateWindow(WindowPtr pWin);

Bool
fbOverlayCloseScreen (int iScreen, ScreenPtr pScreen);

int
fbOverlayWindowLayer(WindowPtr pWin);

Bool
fbOverlayCreateScreenResources(ScreenPtr pScreen);

void
fbOverlayPaintKey (DrawablePtr	pDrawable,
		   RegionPtr	pRegion,
		   CARD32	pixel,
		   int		layer);
void
fbOverlayUpdateLayerRegion (ScreenPtr	pScreen,
			    int		layer,
			    RegionPtr	prgn);

    
void
fbOverlayCopyWindow(WindowPtr	pWin,
		    DDXPointRec	ptOldOrg,
		    RegionPtr	prgnSrc);
    
void
fbOverlayWindowExposures (WindowPtr	pWin,
			  RegionPtr	prgn,
			  RegionPtr	other_exposed);

void
fbOverlayPaintWindow(WindowPtr pWin, RegionPtr pRegion, int what);


Bool
fbOverlaySetupScreen(ScreenPtr	pScreen,
		     pointer	pbits1,
		     pointer	pbits2,
		     int	xsize,
		     int	ysize,
		     int	dpix,
		     int	dpiy,
		     int	width1,
		     int	width2,
		     int	bpp1,
		     int	bpp2);

Bool
fbOverlayFinishScreenInit(ScreenPtr	pScreen,
			  pointer	pbits1,
			  pointer	pbits2,
			  int		xsize,
			  int		ysize,
			  int		dpix,
			  int		dpiy,
			  int		width1,
			  int		width2,
			  int		bpp1,
			  int		bpp2,
			  int		depth1,
			  int		depth2);

#endif /* _FBOVERLAY_H_ */
