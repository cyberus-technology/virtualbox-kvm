/*
 * $XFree86: xc/programs/Xserver/render/picture.h,v 1.20tsi Exp $
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

#ifndef _PICTURE_H_
#define _PICTURE_H_

typedef struct _DirectFormat	*DirectFormatPtr;
typedef struct _PictFormat	*PictFormatPtr;
typedef struct _Picture		*PicturePtr;

/*
 * While the protocol is generous in format support, the
 * sample implementation allows only packed RGB and GBR
 * representations for data to simplify software rendering,
 */
#define PICT_FORMAT(bpp,type,a,r,g,b)	(((bpp) << 24) |  \
					 ((type) << 16) | \
					 ((a) << 12) | \
					 ((r) << 8) | \
					 ((g) << 4) | \
					 ((b)))

/*
 * gray/color formats use a visual index instead of argb
 */
#define PICT_VISFORMAT(bpp,type,vi)	(((bpp) << 24) |  \
					 ((type) << 16) | \
					 ((vi)))

#define PICT_FORMAT_BPP(f)	(((f) >> 24)       )
#define PICT_FORMAT_TYPE(f)	(((f) >> 16) & 0xff)
#define PICT_FORMAT_A(f)	(((f) >> 12) & 0x0f)
#define PICT_FORMAT_R(f)	(((f) >>  8) & 0x0f)
#define PICT_FORMAT_G(f)	(((f) >>  4) & 0x0f)
#define PICT_FORMAT_B(f)	(((f)      ) & 0x0f)
#define PICT_FORMAT_RGB(f)	(((f)      ) & 0xfff)
#define PICT_FORMAT_VIS(f)	(((f)      ) & 0xffff)

#define PICT_TYPE_OTHER	0
#define PICT_TYPE_A	1
#define PICT_TYPE_ARGB	2
#define PICT_TYPE_ABGR	3
#define PICT_TYPE_COLOR	4
#define PICT_TYPE_GRAY	5

#define PICT_FORMAT_COLOR(f)	(PICT_FORMAT_TYPE(f) & 2)

/* 32bpp formats */
#define PICT_a8r8g8b8	PICT_FORMAT(32,PICT_TYPE_ARGB,8,8,8,8)
#define PICT_x8r8g8b8	PICT_FORMAT(32,PICT_TYPE_ARGB,0,8,8,8)
#define PICT_a8b8g8r8	PICT_FORMAT(32,PICT_TYPE_ABGR,8,8,8,8)
#define PICT_x8b8g8r8	PICT_FORMAT(32,PICT_TYPE_ABGR,0,8,8,8)

/* 24bpp formats */
#define PICT_r8g8b8	PICT_FORMAT(24,PICT_TYPE_ARGB,0,8,8,8)
#define PICT_b8g8r8	PICT_FORMAT(24,PICT_TYPE_ABGR,0,8,8,8)

/* 16bpp formats */
#define PICT_r5g6b5	PICT_FORMAT(16,PICT_TYPE_ARGB,0,5,6,5)
#define PICT_b5g6r5	PICT_FORMAT(16,PICT_TYPE_ABGR,0,5,6,5)

#define PICT_a1r5g5b5	PICT_FORMAT(16,PICT_TYPE_ARGB,1,5,5,5)
#define PICT_x1r5g5b5	PICT_FORMAT(16,PICT_TYPE_ARGB,0,5,5,5)
#define PICT_a1b5g5r5	PICT_FORMAT(16,PICT_TYPE_ABGR,1,5,5,5)
#define PICT_x1b5g5r5	PICT_FORMAT(16,PICT_TYPE_ABGR,0,5,5,5)
#define PICT_a4r4g4b4	PICT_FORMAT(16,PICT_TYPE_ARGB,4,4,4,4)
#define PICT_x4r4g4b4	PICT_FORMAT(16,PICT_TYPE_ARGB,0,4,4,4)
#define PICT_a4b4g4r4	PICT_FORMAT(16,PICT_TYPE_ABGR,4,4,4,4)
#define PICT_x4b4g4r4	PICT_FORMAT(16,PICT_TYPE_ABGR,0,4,4,4)

/* 8bpp formats */
#define PICT_a8		PICT_FORMAT(8,PICT_TYPE_A,8,0,0,0)
#define PICT_r3g3b2	PICT_FORMAT(8,PICT_TYPE_ARGB,0,3,3,2)
#define PICT_b2g3r3	PICT_FORMAT(8,PICT_TYPE_ABGR,0,3,3,2)
#define PICT_a2r2g2b2	PICT_FORMAT(8,PICT_TYPE_ARGB,2,2,2,2)
#define PICT_a2b2g2r2	PICT_FORMAT(8,PICT_TYPE_ABGR,2,2,2,2)

#define PICT_c8		PICT_FORMAT(8,PICT_TYPE_COLOR,0,0,0,0)
#define PICT_g8		PICT_FORMAT(8,PICT_TYPE_GRAY,0,0,0,0)

/* 4bpp formats */
#define PICT_a4		PICT_FORMAT(4,PICT_TYPE_A,4,0,0,0)
#define PICT_r1g2b1	PICT_FORMAT(4,PICT_TYPE_ARGB,0,1,2,1)
#define PICT_b1g2r1	PICT_FORMAT(4,PICT_TYPE_ABGR,0,1,2,1)
#define PICT_a1r1g1b1	PICT_FORMAT(4,PICT_TYPE_ARGB,1,1,1,1)
#define PICT_a1b1g1r1	PICT_FORMAT(4,PICT_TYPE_ABGR,1,1,1,1)
				    
#define PICT_c4		PICT_FORMAT(4,PICT_TYPE_COLOR,0,0,0,0)
#define PICT_g4		PICT_FORMAT(4,PICT_TYPE_GRAY,0,0,0,0)

/* 1bpp formats */
#define PICT_a1		PICT_FORMAT(1,PICT_TYPE_A,1,0,0,0)

#define PICT_g1		PICT_FORMAT(1,PICT_TYPE_GRAY,0,0,0,0)

/*
 * For dynamic indexed visuals (GrayScale and PseudoColor), these control the 
 * selection of colors allocated for drawing to Pictures.  The default
 * policy depends on the size of the colormap:
 *
 * Size		Default Policy
 * ----------------------------
 *  < 64	PolicyMono
 *  < 256	PolicyGray
 *  256		PolicyColor (only on PseudoColor)
 *
 * The actual allocation code lives in miindex.c, and so is
 * austensibly server dependent, but that code does:
 *
 * PolicyMono	    Allocate no additional colors, use black and white
 * PolicyGray	    Allocate 13 gray levels (11 cells used)
 * PolicyColor	    Allocate a 4x4x4 cube and 13 gray levels (71 cells used)
 * PolicyAll	    Allocate as big a cube as possible, fill with gray (all)
 *
 * Here's a picture to help understand how many colors are
 * actually allocated (this is just the gray ramp):
 *
 *                 gray level
 * all   0000 1555 2aaa 4000 5555 6aaa 8000 9555 aaaa bfff d555 eaaa ffff
 * b/w   0000                                                        ffff
 * 4x4x4                     5555                aaaa
 * extra      1555 2aaa 4000      6aaa 8000 9555      bfff d555 eaaa
 *
 * The default colormap supplies two gray levels (black/white), the
 * 4x4x4 cube allocates another two and nine more are allocated to fill
 * in the 13 levels.  When the 4x4x4 cube is not allocated, a total of
 * 11 cells are allocated.
 */   

#define PictureCmapPolicyInvalid    -1
#define PictureCmapPolicyDefault    0
#define PictureCmapPolicyMono	    1
#define PictureCmapPolicyGray	    2
#define PictureCmapPolicyColor	    3
#define PictureCmapPolicyAll	    4

extern int  PictureCmapPolicy;

int	PictureParseCmapPolicy (const char *name);

extern int	RenderErrBase;
extern int	RenderClientPrivateIndex;

/* Fixed point updates from Carl Worth, USC, Information Sciences Institute */

#if defined(WIN32) && !defined(__GNUC__)
typedef __int64		xFixed_32_32;
#else
#  if defined (_LP64) || \
      defined(__alpha__) || defined(__alpha) || \
      defined(ia64) || defined(__ia64__) || \
      defined(__sparc64__) || \
      defined(__s390x__) || \
      defined(amd64) || defined (__amd64__) || \
      (defined(sgi) && (_MIPS_SZLONG == 64))
typedef long		xFixed_32_32;
# else
#  if defined(__GNUC__) && \
    ((__GNUC__ > 2) || \
     ((__GNUC__ == 2) && defined(__GNUC_MINOR__) && (__GNUC_MINOR__ > 7)))
__extension__
#  endif
typedef long long int	xFixed_32_32;
# endif
#endif

typedef xFixed_32_32	xFixed_48_16;

#define MAX_FIXED_48_16	    ((xFixed_48_16) 0x7fffffff)
#define MIN_FIXED_48_16	    (-((xFixed_48_16) 1 << 31))

typedef CARD32		xFixed_1_31;
typedef CARD32		xFixed_1_16;
typedef INT32		xFixed_16_16;

/*
 * An unadorned "xFixed" is the same as xFixed_16_16, 
 * (since it's quite common in the code) 
 */
typedef	xFixed_16_16	xFixed;
#define XFIXED_BITS	16

#define xFixedToInt(f)	(int) ((f) >> XFIXED_BITS)
#define IntToxFixed(i)	((xFixed) (i) << XFIXED_BITS)
#define xFixedE		((xFixed) 1)
#define xFixed1		(IntToxFixed(1))
#define xFixed1MinusE	(xFixed1 - xFixedE)
#define xFixedFrac(f)	((f) & xFixed1MinusE)
#define xFixedFloor(f)	((f) & ~xFixed1MinusE)
#define xFixedCeil(f)	xFixedFloor((f) + xFixed1MinusE)

#define xFixedFraction(f)	((f) & xFixed1MinusE)
#define xFixedMod2(f)		((f) & (xFixed1 | xFixed1MinusE))

/* whether 't' is a well defined not obviously empty trapezoid */
#define xTrapezoidValid(t)  ((t)->left.p1.y != (t)->left.p2.y && \
			     (t)->right.p1.y != (t)->right.p2.y && \
			     (int) ((t)->bottom - (t)->top) > 0)

/*
 * Standard NTSC luminance conversions:
 *
 *  y = r * 0.299 + g * 0.587 + b * 0.114
 *
 * Approximate this for a bit more speed:
 *
 *  y = (r * 153 + g * 301 + b * 58) / 512
 *
 * This gives 17 bits of luminance; to get 15 bits, lop the low two
 */

#define CvtR8G8B8toY15(s)	(((((s) >> 16) & 0xff) * 153 + \
				  (((s) >>  8) & 0xff) * 301 + \
				  (((s)      ) & 0xff) * 58) >> 2)

#endif /* _PICTURE_H_ */
