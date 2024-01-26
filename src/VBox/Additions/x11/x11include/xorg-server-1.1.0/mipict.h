/*
 * $XFree86: xc/programs/Xserver/render/mipict.h,v 1.12 2002/11/05 05:34:40 keithp Exp $
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

#ifndef _MIPICT_H_
#define _MIPICT_H_

#include "picturestr.h"

#define MI_MAX_INDEXED	256 /* XXX depth must be <= 8 */

#if MI_MAX_INDEXED <= 256
typedef CARD8 miIndexType;
#endif

typedef struct _miIndexed {
    Bool	color;
    CARD32	rgba[MI_MAX_INDEXED];
    miIndexType	ent[32768];
} miIndexedRec, *miIndexedPtr;

#define miCvtR8G8B8to15(s) ((((s) >> 3) & 0x001f) | \
			     (((s) >> 6) & 0x03e0) | \
			     (((s) >> 9) & 0x7c00))
#define miIndexToEnt15(mif,rgb15) ((mif)->ent[rgb15])
#define miIndexToEnt24(mif,rgb24) miIndexToEnt15(mif,miCvtR8G8B8to15(rgb24))

#define miIndexToEntY24(mif,rgb24) ((mif)->ent[CvtR8G8B8toY15(rgb24)])

int
miCreatePicture (PicturePtr pPicture);

void
miDestroyPicture (PicturePtr pPicture);

void
miDestroyPictureClip (PicturePtr pPicture);

int
miChangePictureClip (PicturePtr    pPicture,
		     int	   type,
		     pointer	   value,
		     int	   n);

void
miChangePicture (PicturePtr pPicture,
		 Mask       mask);

void
miValidatePicture (PicturePtr pPicture,
		   Mask       mask);

int
miChangePictureTransform (PicturePtr	pPicture,
			  PictTransform *transform);

int
miChangePictureFilter (PicturePtr pPicture,
		       int	  filter,
		       xFixed     *params,
		       int	  nparams);

Bool
miClipPicture (RegionPtr    pRegion,
	       PicturePtr   pPicture,
	       INT16	    xReg,
	       INT16	    yReg,
	       INT16	    xPict,
	       INT16	    yPict);

Bool
miComputeCompositeRegion (RegionPtr	pRegion,
			  PicturePtr	pSrc,
			  PicturePtr	pMask,
			  PicturePtr	pDst,
			  INT16		xSrc,
			  INT16		ySrc,
			  INT16		xMask,
			  INT16		yMask,
			  INT16		xDst,
			  INT16		yDst,
			  CARD16	width,
			  CARD16	height);

Bool
miPictureInit (ScreenPtr pScreen, PictFormatPtr formats, int nformats);

Bool
miRealizeGlyph (ScreenPtr pScreen,
		GlyphPtr  glyph);

void
miUnrealizeGlyph (ScreenPtr pScreen,
		  GlyphPtr  glyph);

void
miGlyphExtents (int		nlist,
		GlyphListPtr	list,
		GlyphPtr	*glyphs,
		BoxPtr		extents);

void
miGlyphs (CARD8		op,
	  PicturePtr	pSrc,
	  PicturePtr	pDst,
	  PictFormatPtr	maskFormat,
	  INT16		xSrc,
	  INT16		ySrc,
	  int		nlist,
	  GlyphListPtr	list,
	  GlyphPtr	*glyphs);

void
miRenderColorToPixel (PictFormatPtr pPict,
		      xRenderColor  *color,
		      CARD32	    *pixel);

void
miRenderPixelToColor (PictFormatPtr pPict,
		      CARD32	    pixel,
		      xRenderColor  *color);

Bool
miIsSolidAlpha (PicturePtr pSrc);

void
miCompositeRects (CARD8		op,
		  PicturePtr	pDst,
		  xRenderColor  *color,
		  int		nRect,
		  xRectangle    *rects);

void
miTrapezoidBounds (int ntrap, xTrapezoid *traps, BoxPtr box);

void
miTrapezoids (CARD8	    op,
	      PicturePtr    pSrc,
	      PicturePtr    pDst,
	      PictFormatPtr maskFormat,
	      INT16	    xSrc,
	      INT16	    ySrc,
	      int	    ntrap,
	      xTrapezoid    *traps);

void
miPointFixedBounds (int npoint, xPointFixed *points, BoxPtr bounds);
    
void
miTriangleBounds (int ntri, xTriangle *tris, BoxPtr bounds);

void
miRasterizeTriangle (PicturePtr	pMask,
		     xTriangle	*tri,
		     int	x_off,
		     int	y_off);

void
miTriangles (CARD8	    op,
	     PicturePtr	    pSrc,
	     PicturePtr	    pDst,
	     PictFormatPtr  maskFormat,
	     INT16	    xSrc,
	     INT16	    ySrc,
	     int	    ntri,
	     xTriangle	    *tris);

void
miTriStrip (CARD8	    op,
	    PicturePtr	    pSrc,
	    PicturePtr	    pDst,
	    PictFormatPtr   maskFormat,
	    INT16	    xSrc,
	    INT16	    ySrc,
	    int		    npoint,
	    xPointFixed	    *points);

void
miTriFan (CARD8		op,
	  PicturePtr	pSrc,
	  PicturePtr	pDst,
	  PictFormatPtr maskFormat,
	  INT16		xSrc,
	  INT16		ySrc,
	  int		npoint,
	  xPointFixed	*points);

PicturePtr
miCreateAlphaPicture (ScreenPtr	    pScreen, 
		      PicturePtr    pDst,
		      PictFormatPtr pPictFormat,
		      CARD16	    width,
		      CARD16	    height);

Bool
miInitIndexed (ScreenPtr	pScreen,
	       PictFormatPtr	pFormat);

void
miCloseIndexed (ScreenPtr	pScreen,
		PictFormatPtr	pFormat);

void
miUpdateIndexed (ScreenPtr	pScreen,
		 PictFormatPtr	pFormat,
		 int		ndef,
		 xColorItem	*pdef);

#endif /* _MIPICT_H_ */
