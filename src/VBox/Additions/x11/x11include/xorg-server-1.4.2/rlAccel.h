/*
 * Rootless Acceleration Code
 */
/*
 * Copyright (c) 2003 Torrey T. Lyons. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "fb.h"

/*
 * rlBlt.c
 */
void
rlBlt (FbBits   *srcLine,
       FbStride	srcStride,
       int	srcX,

       ScreenPtr pDstScreen,
       FbBits   *dstLine,
       FbStride dstStride,
       int	dstX,

       int	width,
       int	height,

       int	alu,
       FbBits	pm,
       int	bpp,

       Bool	reverse,
       Bool	upsidedown);

/*
 * rlCopy.c
 */
RegionPtr
rlCopyArea (DrawablePtr	pSrcDrawable,
	    DrawablePtr	pDstDrawable,
	    GCPtr	pGC,
	    int		xIn, 
	    int		yIn,
	    int		widthSrc, 
	    int		heightSrc,
	    int		xOut, 
	    int		yOut);

/*
 * rlFill.c
 */
void
rlFill (DrawablePtr pDrawable,
	GCPtr	    pGC,
	int	    x,
	int	    y,
	int	    width,
	int	    height);

void
rlSolidBoxClipped (DrawablePtr	pDrawable,
		   RegionPtr	pClip,
		   int		x1,
		   int		y1,
		   int		x2,
		   int		y2,
		   FbBits	and,
		   FbBits	xor);

/*
 * rlFillRect.c
 */
void
rlPolyFillRect(DrawablePtr  pDrawable, 
	       GCPtr	    pGC, 
	       int	    nrect,
	       xRectangle   *prect);

/*
 * rlFillSpans.c
 */
void
rlFillSpans (DrawablePtr    pDrawable,
	     GCPtr	    pGC,
	     int	    n,
	     DDXPointPtr    ppt,
	     int	    *pwidth,
	     int	    fSorted);

/*
 * rlGlyph.c
 */
void
rlImageGlyphBlt (DrawablePtr	pDrawable,
		 GCPtr		pGC,
		 int		x, 
		 int		y,
		 unsigned int	nglyph,
		 CharInfoPtr	*ppciInit,
		 pointer	pglyphBase);

/*
 * rlSolid.c
 */
void
rlSolid (ScreenPtr  pScreen,
         FbBits	    *dst,
	 FbStride   dstStride,
	 int	    dstX,
	 int	    bpp,

	 int	    width,
	 int	    height,

	 FbBits	    and,
	 FbBits	    xor);
