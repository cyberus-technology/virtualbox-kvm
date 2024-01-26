/*
 *
 * Copyright © 1998 Keith Packard
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
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */


#ifndef _FB_H_
#define _FB_H_

#include <X11/X.h>
#include <pixman.h>

#include "scrnintstr.h"
#include "pixmap.h"
#include "pixmapstr.h"
#include "region.h"
#include "gcstruct.h"
#include "colormap.h"
#include "miscstruct.h"
#include "servermd.h"
#include "windowstr.h"
#include "mi.h"
#include "migc.h"
#include "mibstore.h"
#ifdef RENDER
#include "picturestr.h"
#else
#include "picture.h"
#endif

#ifdef FB_ACCESS_WRAPPER

#include "wfbrename.h"
#define FBPREFIX(x) wfb##x
#define WRITE(ptr, val) ((*wfbWriteMemory)((ptr), (val), sizeof(*(ptr))))
#define READ(ptr) ((*wfbReadMemory)((ptr), sizeof(*(ptr))))

#define MEMCPY_WRAPPED(dst, src, size) do {                       \
    size_t _i;                                                    \
    CARD8 *_dst = (CARD8*)(dst), *_src = (CARD8*)(src);           \
    for(_i = 0; _i < size; _i++) {                                \
        WRITE(_dst +_i, READ(_src + _i));                         \
    }                                                             \
} while(0)

#define MEMSET_WRAPPED(dst, val, size) do {                       \
    size_t _i;                                                    \
    CARD8 *_dst = (CARD8*)(dst);                                  \
    for(_i = 0; _i < size; _i++) {                                \
        WRITE(_dst +_i, (val));                                   \
    }                                                             \
} while(0)

#else

#define FBPREFIX(x) fb##x
#define WRITE(ptr, val) (*(ptr) = (val))
#define READ(ptr) (*(ptr))
#define MEMCPY_WRAPPED(dst, src, size) memcpy((dst), (src), (size))
#define MEMSET_WRAPPED(dst, val, size) memset((dst), (val), (size))

#endif

/*
 * This single define controls the basic size of data manipulated
 * by this software; it must be log2(sizeof (FbBits) * 8)
 */

#ifndef FB_SHIFT
#define FB_SHIFT    LOG2_BITMAP_PAD
#endif

#if FB_SHIFT < LOG2_BITMAP_PAD
    error FB_SHIFT must be >= LOG2_BITMAP_PAD
#endif
    
#define FB_UNIT	    (1 << FB_SHIFT)
#define FB_HALFUNIT (1 << (FB_SHIFT-1))
#define FB_MASK	    (FB_UNIT - 1)
#define FB_ALLONES  ((FbBits) -1)
    
#if GLYPHPADBYTES != 4
#error "GLYPHPADBYTES must be 4"
#endif
#if GETLEFTBITS_ALIGNMENT != 1
#error "GETLEFTBITS_ALIGNMENT must be 1"
#endif
/* whether to bother to include 24bpp support */
#ifndef FBNO24BIT
#define FB_24BIT
#endif

/*
 * Unless otherwise instructed, fb includes code to advertise 24bpp
 * windows with 32bpp image format for application compatibility
 */

#ifdef FB_24BIT
#ifndef FBNO24_32
#define FB_24_32BIT
#endif
#endif

#define FB_STIP_SHIFT	LOG2_BITMAP_PAD
#define FB_STIP_UNIT	(1 << FB_STIP_SHIFT)
#define FB_STIP_MASK	(FB_STIP_UNIT - 1)
#define FB_STIP_ALLONES	((FbStip) -1)
    
#define FB_STIP_ODDSTRIDE(s)	(((s) & (FB_MASK >> FB_STIP_SHIFT)) != 0)
#define FB_STIP_ODDPTR(p)	((((long) (p)) & (FB_MASK >> 3)) != 0)
    
#define FbStipStrideToBitsStride(s) (((s) >> (FB_SHIFT - FB_STIP_SHIFT)))
#define FbBitsStrideToStipStride(s) (((s) << (FB_SHIFT - FB_STIP_SHIFT)))
    
#define FbFullMask(n)   ((n) == FB_UNIT ? FB_ALLONES : ((((FbBits) 1) << n) - 1))
    
#if FB_SHIFT == 6
# ifdef WIN32
typedef unsigned __int64    FbBits;
# else
#  if defined(__alpha__) || defined(__alpha) || \
      defined(ia64) || defined(__ia64__) || \
      defined(__sparc64__) || defined(_LP64) || \
      defined(__s390x__) || \
      defined(amd64) || defined (__amd64__) || \
      defined (__powerpc64__) || \
      (defined(sgi) && (_MIPS_SZLONG == 64))
typedef unsigned long	    FbBits;
#  else
typedef unsigned long long  FbBits;
#  endif
# endif
#endif

#if FB_SHIFT == 5
typedef CARD32		    FbBits;
#endif

#if FB_SHIFT == 4
typedef CARD16		    FbBits;
#endif

#if LOG2_BITMAP_PAD == FB_SHIFT
typedef FbBits		    FbStip;
#else
# if LOG2_BITMAP_PAD == 5
typedef CARD32		    FbStip;
# endif
#endif

typedef int		    FbStride;


#ifdef FB_DEBUG
extern void fbValidateDrawable(DrawablePtr d);
extern void fbInitializeDrawable(DrawablePtr d);
extern void fbSetBits (FbStip *bits, int stride, FbStip data);
#define FB_HEAD_BITS   (FbStip) (0xbaadf00d)
#define FB_TAIL_BITS   (FbStip) (0xbaddf0ad)
#else
#define fbValidateDrawable(d)
#define fdInitializeDrawable(d)
#endif

#include "fbrop.h"

#if BITMAP_BIT_ORDER == LSBFirst
#define FbScrLeft(x,n)	((x) >> (n))
#define FbScrRight(x,n)	((x) << (n))
/* #define FbLeftBits(x,n)	((x) & ((((FbBits) 1) << (n)) - 1)) */
#define FbLeftStipBits(x,n) ((x) & ((((FbStip) 1) << (n)) - 1))
#define FbStipMoveLsb(x,s,n)	(FbStipRight (x,(s)-(n)))
#define FbPatternOffsetBits	0
#else
#define FbScrLeft(x,n)	((x) << (n))
#define FbScrRight(x,n)	((x) >> (n))
/* #define FbLeftBits(x,n)	((x) >> (FB_UNIT - (n))) */
#define FbLeftStipBits(x,n) ((x) >> (FB_STIP_UNIT - (n)))
#define FbStipMoveLsb(x,s,n)	(x)
#define FbPatternOffsetBits	(sizeof (FbBits) - 1)
#endif

#include "micoord.h"

#define FbStipLeft(x,n)	FbScrLeft(x,n)
#define FbStipRight(x,n) FbScrRight(x,n)

#define FbRotLeft(x,n)	FbScrLeft(x,n) | (n ? FbScrRight(x,FB_UNIT-n) : 0)
#define FbRotRight(x,n)	FbScrRight(x,n) | (n ? FbScrLeft(x,FB_UNIT-n) : 0)

#define FbRotStipLeft(x,n)  FbStipLeft(x,n) | (n ? FbStipRight(x,FB_STIP_UNIT-n) : 0)
#define FbRotStipRight(x,n)  FbStipRight(x,n) | (n ? FbStipLeft(x,FB_STIP_UNIT-n) : 0)

#define FbLeftMask(x)	    ( ((x) & FB_MASK) ? \
			     FbScrRight(FB_ALLONES,(x) & FB_MASK) : 0)
#define FbRightMask(x)	    ( ((FB_UNIT - (x)) & FB_MASK) ? \
			     FbScrLeft(FB_ALLONES,(FB_UNIT - (x)) & FB_MASK) : 0)

#define FbLeftStipMask(x)   ( ((x) & FB_STIP_MASK) ? \
			     FbStipRight(FB_STIP_ALLONES,(x) & FB_STIP_MASK) : 0)
#define FbRightStipMask(x)  ( ((FB_STIP_UNIT - (x)) & FB_STIP_MASK) ? \
			     FbScrLeft(FB_STIP_ALLONES,(FB_STIP_UNIT - (x)) & FB_STIP_MASK) : 0)

#define FbBitsMask(x,w)	(FbScrRight(FB_ALLONES,(x) & FB_MASK) & \
			 FbScrLeft(FB_ALLONES,(FB_UNIT - ((x) + (w))) & FB_MASK))

#define FbStipMask(x,w)	(FbStipRight(FB_STIP_ALLONES,(x) & FB_STIP_MASK) & \
			 FbStipLeft(FB_STIP_ALLONES,(FB_STIP_UNIT - ((x)+(w))) & FB_STIP_MASK))


#define FbMaskBits(x,w,l,n,r) { \
    n = (w); \
    r = FbRightMask((x)+n); \
    l = FbLeftMask(x); \
    if (l) { \
	n -= FB_UNIT - ((x) & FB_MASK); \
	if (n < 0) { \
	    n = 0; \
	    l &= r; \
	    r = 0; \
	} \
    } \
    n >>= FB_SHIFT; \
}

#ifdef FBNOPIXADDR
#define FbMaskBitsBytes(x,w,copy,l,lb,n,r,rb) FbMaskBits(x,w,l,n,r)
#define FbDoLeftMaskByteRRop(dst,lb,l,and,xor) { \
    *dst = FbDoMaskRRop(*dst,and,xor,l); \
}
#define FbDoRightMaskByteRRop(dst,rb,r,and,xor) { \
    *dst = FbDoMaskRRop(*dst,and,xor,r); \
}
#else

#define FbByteMaskInvalid   0x10

#define FbPatternOffset(o,t)  ((o) ^ (FbPatternOffsetBits & ~(sizeof (t) - 1)))

#define FbPtrOffset(p,o,t)		((t *) ((CARD8 *) (p) + (o)))
#define FbSelectPatternPart(xor,o,t)	((xor) >> (FbPatternOffset (o,t) << 3))
#define FbStorePart(dst,off,t,xor)	(WRITE(FbPtrOffset(dst,off,t), \
					 FbSelectPart(xor,off,t)))
#ifndef FbSelectPart
#define FbSelectPart(x,o,t) FbSelectPatternPart(x,o,t)
#endif

#define FbMaskBitsBytes(x,w,copy,l,lb,n,r,rb) { \
    n = (w); \
    lb = 0; \
    rb = 0; \
    r = FbRightMask((x)+n); \
    if (r) { \
	/* compute right byte length */ \
	if ((copy) && (((x) + n) & 7) == 0) { \
	    rb = (((x) + n) & FB_MASK) >> 3; \
	} else { \
	    rb = FbByteMaskInvalid; \
	} \
    } \
    l = FbLeftMask(x); \
    if (l) { \
	/* compute left byte length */ \
	if ((copy) && ((x) & 7) == 0) { \
	    lb = ((x) & FB_MASK) >> 3; \
	} else { \
	    lb = FbByteMaskInvalid; \
	} \
	/* subtract out the portion painted by leftMask */ \
	n -= FB_UNIT - ((x) & FB_MASK); \
	if (n < 0) { \
	    if (lb != FbByteMaskInvalid) { \
		if (rb == FbByteMaskInvalid) { \
		    lb = FbByteMaskInvalid; \
		} else if (rb) { \
		    lb |= (rb - lb) << (FB_SHIFT - 3); \
		    rb = 0; \
		} \
	    } \
	    n = 0; \
	    l &= r; \
	    r = 0; \
	}\
    } \
    n >>= FB_SHIFT; \
}

#if FB_SHIFT == 6
#define FbDoLeftMaskByteRRop6Cases(dst,xor) \
    case (sizeof (FbBits) - 7) | (1 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 7,CARD8,xor); \
	break; \
    case (sizeof (FbBits) - 7) | (2 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 7,CARD8,xor); \
	FbStorePart(dst,sizeof (FbBits) - 6,CARD8,xor); \
	break; \
    case (sizeof (FbBits) - 7) | (3 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 7,CARD8,xor); \
	FbStorePart(dst,sizeof (FbBits) - 6,CARD16,xor); \
	break; \
    case (sizeof (FbBits) - 7) | (4 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 7,CARD8,xor); \
	FbStorePart(dst,sizeof (FbBits) - 6,CARD16,xor); \
	FbStorePart(dst,sizeof (FbBits) - 4,CARD8,xor); \
	break; \
    case (sizeof (FbBits) - 7) | (5 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 7,CARD8,xor); \
	FbStorePart(dst,sizeof (FbBits) - 6,CARD16,xor); \
	FbStorePart(dst,sizeof (FbBits) - 4,CARD16,xor); \
	break; \
    case (sizeof (FbBits) - 7) | (6 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 7,CARD8,xor); \
	FbStorePart(dst,sizeof (FbBits) - 6,CARD16,xor); \
	FbStorePart(dst,sizeof (FbBits) - 4,CARD16,xor); \
	FbStorePart(dst,sizeof (FbBits) - 2,CARD8,xor); \
	break; \
    case (sizeof (FbBits) - 7): \
	FbStorePart(dst,sizeof (FbBits) - 7,CARD8,xor); \
	FbStorePart(dst,sizeof (FbBits) - 6,CARD16,xor); \
	FbStorePart(dst,sizeof (FbBits) - 4,CARD32,xor); \
	break; \
    case (sizeof (FbBits) - 6) | (1 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 6,CARD8,xor); \
	break; \
    case (sizeof (FbBits) - 6) | (2 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 6,CARD16,xor); \
	break; \
    case (sizeof (FbBits) - 6) | (3 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 6,CARD16,xor); \
	FbStorePart(dst,sizeof (FbBits) - 4,CARD8,xor); \
	break; \
    case (sizeof (FbBits) - 6) | (4 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 6,CARD16,xor); \
	FbStorePart(dst,sizeof (FbBits) - 4,CARD16,xor); \
	break; \
    case (sizeof (FbBits) - 6) | (5 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 6,CARD16,xor); \
	FbStorePart(dst,sizeof (FbBits) - 4,CARD16,xor); \
	FbStorePart(dst,sizeof (FbBits) - 2,CARD8,xor); \
	break; \
    case (sizeof (FbBits) - 6): \
	FbStorePart(dst,sizeof (FbBits) - 6,CARD16,xor); \
	FbStorePart(dst,sizeof (FbBits) - 4,CARD32,xor); \
	break; \
    case (sizeof (FbBits) - 5) | (1 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 5,CARD8,xor); \
	break; \
    case (sizeof (FbBits) - 5) | (2 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 5,CARD8,xor); \
	FbStorePart(dst,sizeof (FbBits) - 4,CARD8,xor); \
	break; \
    case (sizeof (FbBits) - 5) | (3 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 5,CARD8,xor); \
	FbStorePart(dst,sizeof (FbBits) - 4,CARD16,xor); \
	break; \
    case (sizeof (FbBits) - 5) | (4 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 5,CARD8,xor); \
	FbStorePart(dst,sizeof (FbBits) - 4,CARD16,xor); \
	FbStorePart(dst,sizeof (FbBits) - 2,CARD8,xor); \
	break; \
    case (sizeof (FbBits) - 5): \
	FbStorePart(dst,sizeof (FbBits) - 5,CARD8,xor); \
	FbStorePart(dst,sizeof (FbBits) - 4,CARD32,xor); \
	break; \
    case (sizeof (FbBits) - 4) | (1 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 4,CARD8,xor); \
	break; \
    case (sizeof (FbBits) - 4) | (2 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 4,CARD16,xor); \
	break; \
    case (sizeof (FbBits) - 4) | (3 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 4,CARD16,xor); \
	FbStorePart(dst,sizeof (FbBits) - 2,CARD8,xor); \
	break; \
    case (sizeof (FbBits) - 4): \
	FbStorePart(dst,sizeof (FbBits) - 4,CARD32,xor); \
	break;

#define FbDoRightMaskByteRRop6Cases(dst,xor) \
    case 4: \
	FbStorePart(dst,0,CARD32,xor); \
	break; \
    case 5: \
	FbStorePart(dst,0,CARD32,xor); \
	FbStorePart(dst,4,CARD8,xor); \
	break; \
    case 6: \
	FbStorePart(dst,0,CARD32,xor); \
	FbStorePart(dst,4,CARD16,xor); \
	break; \
    case 7: \
	FbStorePart(dst,0,CARD32,xor); \
	FbStorePart(dst,4,CARD16,xor); \
	FbStorePart(dst,6,CARD8,xor); \
	break;
#else
#define FbDoLeftMaskByteRRop6Cases(dst,xor)
#define FbDoRightMaskByteRRop6Cases(dst,xor)
#endif

#define FbDoLeftMaskByteRRop(dst,lb,l,and,xor) { \
    switch (lb) { \
    FbDoLeftMaskByteRRop6Cases(dst,xor) \
    case (sizeof (FbBits) - 3) | (1 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 3,CARD8,xor); \
	break; \
    case (sizeof (FbBits) - 3) | (2 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 3,CARD8,xor); \
	FbStorePart(dst,sizeof (FbBits) - 2,CARD8,xor); \
	break; \
    case (sizeof (FbBits) - 2) | (1 << (FB_SHIFT - 3)): \
	FbStorePart(dst,sizeof (FbBits) - 2,CARD8,xor); \
	break; \
    case sizeof (FbBits) - 3: \
	FbStorePart(dst,sizeof (FbBits) - 3,CARD8,xor); \
    case sizeof (FbBits) - 2: \
	FbStorePart(dst,sizeof (FbBits) - 2,CARD16,xor); \
	break; \
    case sizeof (FbBits) - 1: \
	FbStorePart(dst,sizeof (FbBits) - 1,CARD8,xor); \
	break; \
    default: \
	WRITE(dst, FbDoMaskRRop(READ(dst), and, xor, l)); \
	break; \
    } \
}


#define FbDoRightMaskByteRRop(dst,rb,r,and,xor) { \
    switch (rb) { \
    case 1: \
	FbStorePart(dst,0,CARD8,xor); \
	break; \
    case 2: \
	FbStorePart(dst,0,CARD16,xor); \
	break; \
    case 3: \
	FbStorePart(dst,0,CARD16,xor); \
	FbStorePart(dst,2,CARD8,xor); \
	break; \
    FbDoRightMaskByteRRop6Cases(dst,xor) \
    default: \
	WRITE(dst, FbDoMaskRRop (READ(dst), and, xor, r)); \
    } \
}
#endif

#define FbMaskStip(x,w,l,n,r) { \
    n = (w); \
    r = FbRightStipMask((x)+n); \
    l = FbLeftStipMask(x); \
    if (l) { \
	n -= FB_STIP_UNIT - ((x) & FB_STIP_MASK); \
	if (n < 0) { \
	    n = 0; \
	    l &= r; \
	    r = 0; \
	} \
    } \
    n >>= FB_STIP_SHIFT; \
}

/*
 * These macros are used to transparently stipple
 * in copy mode; the expected usage is with 'n' constant
 * so all of the conditional parts collapse into a minimal
 * sequence of partial word writes
 *
 * 'n' is the bytemask of which bytes to store, 'a' is the address
 * of the FbBits base unit, 'o' is the offset within that unit
 *
 * The term "lane" comes from the hardware term "byte-lane" which
 */

#define FbLaneCase1(n,a,o)  ((n) == 0x01 ? (void) \
			     WRITE((CARD8 *) ((a)+FbPatternOffset(o,CARD8)), \
			      fgxor) : (void) 0)
#define FbLaneCase2(n,a,o)  ((n) == 0x03 ? (void) \
			     WRITE((CARD16 *) ((a)+FbPatternOffset(o,CARD16)), \
			      fgxor) : \
			     ((void)FbLaneCase1((n)&1,a,o), \
				    FbLaneCase1((n)>>1,a,(o)+1)))
#define FbLaneCase4(n,a,o)  ((n) == 0x0f ? (void) \
			     WRITE((CARD32 *) ((a)+FbPatternOffset(o,CARD32)), \
			      fgxor) : \
			     ((void)FbLaneCase2((n)&3,a,o), \
				    FbLaneCase2((n)>>2,a,(o)+2)))
#define FbLaneCase8(n,a,o)  ((n) == 0x0ff ? (void) (*(FbBits *) ((a)+(o)) = fgxor) : \
			     ((void)FbLaneCase4((n)&15,a,o), \
				    FbLaneCase4((n)>>4,a,(o)+4)))

#if FB_SHIFT == 6
#define FbLaneCase(n,a)   FbLaneCase8(n,(CARD8 *) (a),0)
#endif

#if FB_SHIFT == 5
#define FbLaneCase(n,a)   FbLaneCase4(n,(CARD8 *) (a),0)
#endif

/* Rotate a filled pixel value to the specified alignement */
#define FbRot24(p,b)	    (FbScrRight(p,b) | FbScrLeft(p,24-(b)))
#define FbRot24Stip(p,b)    (FbStipRight(p,b) | FbStipLeft(p,24-(b)))

/* step a filled pixel value to the next/previous FB_UNIT alignment */
#define FbNext24Pix(p)	(FbRot24(p,(24-FB_UNIT%24)))
#define FbPrev24Pix(p)	(FbRot24(p,FB_UNIT%24))
#define FbNext24Stip(p)	(FbRot24(p,(24-FB_STIP_UNIT%24)))
#define FbPrev24Stip(p)	(FbRot24(p,FB_STIP_UNIT%24))

/* step a rotation value to the next/previous rotation value */
#if FB_UNIT == 64
#define FbNext24Rot(r)        ((r) == 16 ? 0 : (r) + 8)
#define FbPrev24Rot(r)        ((r) == 0 ? 16 : (r) - 8)

#if IMAGE_BYTE_ORDER == MSBFirst
#define FbFirst24Rot(x)		(((x) + 8) % 24)
#else
#define FbFirst24Rot(x)		((x) % 24)
#endif

#endif

#if FB_UNIT == 32
#define FbNext24Rot(r)        ((r) == 0 ? 16 : (r) - 8)
#define FbPrev24Rot(r)        ((r) == 16 ? 0 : (r) + 8)

#if IMAGE_BYTE_ORDER == MSBFirst
#define FbFirst24Rot(x)		(((x) + 16) % 24)
#else
#define FbFirst24Rot(x)		((x) % 24)
#endif
#endif

#define FbNext24RotStip(r)        ((r) == 0 ? 16 : (r) - 8)
#define FbPrev24RotStip(r)        ((r) == 16 ? 0 : (r) + 8)

/* Whether 24-bit specific code is needed for this filled pixel value */
#define FbCheck24Pix(p)	((p) == FbNext24Pix(p))

/* Macros for dealing with dashing */

#define FbDashDeclare	\
    unsigned char	*__dash, *__firstDash, *__lastDash
    
#define FbDashInit(pGC,pPriv,dashOffset,dashlen,even) {	    \
    (even) = TRUE;					    \
    __firstDash = (pGC)->dash;				    \
    __lastDash = __firstDash + (pGC)->numInDashList;	    \
    (dashOffset) %= (pPriv)->dashLength;		    \
							    \
    __dash = __firstDash;				    \
    while ((dashOffset) >= ((dashlen) = *__dash))	    \
    {							    \
	(dashOffset) -= (dashlen);			    \
	(even) = 1-(even);				    \
	if (++__dash == __lastDash)			    \
	    __dash = __firstDash;			    \
    }							    \
    (dashlen) -= (dashOffset);				    \
}

#define FbDashNext(dashlen) {				    \
    if (++__dash == __lastDash)				    \
	__dash = __firstDash;				    \
    (dashlen) = *__dash;				    \
}

/* as numInDashList is always even, this case can skip a test */

#define FbDashNextEven(dashlen) {			    \
    (dashlen) = *++__dash;				    \
}

#define FbDashNextOdd(dashlen)	FbDashNext(dashlen)

#define FbDashStep(dashlen,even) {			    \
    if (!--(dashlen)) {					    \
	FbDashNext(dashlen);				    \
	(even) = 1-(even);				    \
    }							    \
}

/* XXX fb*PrivateIndex should be static, but it breaks the ABI */

extern int	fbGCPrivateIndex;
extern int	fbGetGCPrivateIndex(void);
#ifndef FB_NO_WINDOW_PIXMAPS
extern int	fbWinPrivateIndex;
extern int	fbGetWinPrivateIndex(void);
#endif
extern const GCOps	fbGCOps;
extern const GCFuncs	fbGCFuncs;

#ifdef FB_24_32BIT
#define FB_SCREEN_PRIVATE
#endif

/* Framebuffer access wrapper */
typedef FbBits (*ReadMemoryProcPtr)(const void *src, int size);
typedef void (*WriteMemoryProcPtr)(void *dst, FbBits value, int size);
typedef void (*SetupWrapProcPtr)(ReadMemoryProcPtr  *pRead,
                                 WriteMemoryProcPtr *pWrite,
                                 DrawablePtr         pDraw);
typedef void (*FinishWrapProcPtr)(DrawablePtr pDraw);

#ifdef FB_ACCESS_WRAPPER

#define fbPrepareAccess(pDraw) \
	fbGetScreenPrivate((pDraw)->pScreen)->setupWrap( \
		&wfbReadMemory, \
		&wfbWriteMemory, \
		(pDraw))
#define fbFinishAccess(pDraw) \
	fbGetScreenPrivate((pDraw)->pScreen)->finishWrap(pDraw)

#else

#define fbPrepareAccess(pPix)
#define fbFinishAccess(pDraw)

#endif


#ifdef FB_SCREEN_PRIVATE
extern int	fbScreenPrivateIndex;
extern int	fbGetScreenPrivateIndex(void);

/* private field of a screen */
typedef struct {
    unsigned char	win32bpp;	/* window bpp for 32-bpp images */
    unsigned char	pix32bpp;	/* pixmap bpp for 32-bpp images */
#ifdef FB_ACCESS_WRAPPER
    SetupWrapProcPtr	setupWrap;	/* driver hook to set pixmap access wrapping */
    FinishWrapProcPtr	finishWrap;	/* driver hook to clean up pixmap access wrapping */
#endif
} FbScreenPrivRec, *FbScreenPrivPtr;

#define fbGetScreenPrivate(pScreen) ((FbScreenPrivPtr) \
				     (pScreen)->devPrivates[fbGetScreenPrivateIndex()].ptr)
#endif

/* private field of GC */
typedef struct {
    FbBits		and, xor;	/* reduced rop values */
    FbBits		bgand, bgxor;	/* for stipples */
    FbBits		fg, bg, pm;	/* expanded and filled */
    unsigned int	dashLength;	/* total of all dash elements */
    unsigned char    	oneRect;	/* clip list is single rectangle */
    unsigned char    	evenStipple;	/* stipple is even */
    unsigned char    	bpp;		/* current drawable bpp */
} FbGCPrivRec, *FbGCPrivPtr;

#define fbGetGCPrivate(pGC)	((FbGCPrivPtr)\
	(pGC)->devPrivates[fbGetGCPrivateIndex()].ptr)

#define fbGetCompositeClip(pGC) ((pGC)->pCompositeClip)
#define fbGetExpose(pGC)	((pGC)->fExpose)
#define fbGetFreeCompClip(pGC)	((pGC)->freeCompClip)
#define fbGetRotatedPixmap(pGC)	((pGC)->pRotatedPixmap)

#define fbGetScreenPixmap(s)	((PixmapPtr) (s)->devPrivate)
#ifdef FB_NO_WINDOW_PIXMAPS
#define fbGetWindowPixmap(d)	fbGetScreenPixmap(((DrawablePtr) (d))->pScreen)
#else
#define fbGetWindowPixmap(pWin)	((PixmapPtr)\
	((WindowPtr) (pWin))->devPrivates[fbGetWinPrivateIndex()].ptr)
#endif

#ifdef ROOTLESS
#define __fbPixDrawableX(pPix)	((pPix)->drawable.x)
#define __fbPixDrawableY(pPix)	((pPix)->drawable.y)
#else
#define __fbPixDrawableX(pPix)	0
#define __fbPixDrawableY(pPix)	0
#endif

#ifdef COMPOSITE
#define __fbPixOffXWin(pPix)	(__fbPixDrawableX(pPix) - (pPix)->screen_x)
#define __fbPixOffYWin(pPix)	(__fbPixDrawableY(pPix) - (pPix)->screen_y)
#else
#define __fbPixOffXWin(pPix)	(__fbPixDrawableX(pPix))
#define __fbPixOffYWin(pPix)	(__fbPixDrawableY(pPix))
#endif
#define __fbPixOffXPix(pPix)	(__fbPixDrawableX(pPix))
#define __fbPixOffYPix(pPix)	(__fbPixDrawableY(pPix))

#define fbGetDrawable(pDrawable, pointer, stride, bpp, xoff, yoff) { \
    PixmapPtr   _pPix; \
    if ((pDrawable)->type != DRAWABLE_PIXMAP) { \
	_pPix = fbGetWindowPixmap(pDrawable); \
	(xoff) = __fbPixOffXWin(_pPix); \
	(yoff) = __fbPixOffYWin(_pPix); \
    } else { \
	_pPix = (PixmapPtr) (pDrawable); \
	(xoff) = __fbPixOffXPix(_pPix); \
	(yoff) = __fbPixOffYPix(_pPix); \
    } \
    fbPrepareAccess(pDrawable); \
    (pointer) = (FbBits *) _pPix->devPrivate.ptr; \
    (stride) = ((int) _pPix->devKind) / sizeof (FbBits); (void)(stride); \
    (bpp) = _pPix->drawable.bitsPerPixel;  (void)(bpp); \
}

#define fbGetStipDrawable(pDrawable, pointer, stride, bpp, xoff, yoff) { \
    PixmapPtr   _pPix; \
    if ((pDrawable)->type != DRAWABLE_PIXMAP) { \
	_pPix = fbGetWindowPixmap(pDrawable); \
	(xoff) = __fbPixOffXWin(_pPix); \
	(yoff) = __fbPixOffYWin(_pPix); \
    } else { \
	_pPix = (PixmapPtr) (pDrawable); \
	(xoff) = __fbPixOffXPix(_pPix); \
	(yoff) = __fbPixOffYPix(_pPix); \
    } \
    fbPrepareAccess(pDrawable); \
    (pointer) = (FbStip *) _pPix->devPrivate.ptr; \
    (stride) = ((int) _pPix->devKind) / sizeof (FbStip); (void)(stride); \
    (bpp) = _pPix->drawable.bitsPerPixel; (void)(bpp); \
}

/*
 * XFree86 empties the root BorderClip when the VT is inactive,
 * here's a macro which uses that to disable GetImage and GetSpans
 */

#define fbWindowEnabled(pWin) \
    REGION_NOTEMPTY((pWin)->drawable.pScreen, \
		    &WindowTable[(pWin)->drawable.pScreen->myNum]->borderClip)

#define fbDrawableEnabled(pDrawable) \
    ((pDrawable)->type == DRAWABLE_PIXMAP ? \
     TRUE : fbWindowEnabled((WindowPtr) pDrawable))

#define FbPowerOfTwo(w)	    (((w) & ((w) - 1)) == 0)
/*
 * Accelerated tiles are power of 2 width <= FB_UNIT
 */
#define FbEvenTile(w)	    ((w) <= FB_UNIT && FbPowerOfTwo(w))
/*
 * Accelerated stipples are power of 2 width and <= FB_UNIT/dstBpp
 * with dstBpp a power of 2 as well
 */
#define FbEvenStip(w,bpp)   ((w) * (bpp) <= FB_UNIT && FbPowerOfTwo(w) && FbPowerOfTwo(bpp))

/*
 * fb24_32.c
 */
void
fb24_32GetSpans(DrawablePtr	pDrawable, 
		int		wMax, 
		DDXPointPtr	ppt, 
		int		*pwidth, 
		int		nspans, 
		char		*pchardstStart);

void
fb24_32SetSpans (DrawablePtr	    pDrawable,
		 GCPtr		    pGC,
		 char		    *src,
		 DDXPointPtr	    ppt,
		 int		    *pwidth,
		 int		    nspans,
		 int		    fSorted);

void
fb24_32PutZImage (DrawablePtr	pDrawable,
		  RegionPtr	pClip,
		  int		alu,
		  FbBits	pm,
		  int		x,
		  int		y,
		  int		width,
		  int		height,
		  CARD8		*src,
		  FbStride	srcStride);
    
void
fb24_32GetImage (DrawablePtr     pDrawable,
		 int             x,
		 int             y,
		 int             w,
		 int             h,
		 unsigned int    format,
		 unsigned long   planeMask,
		 char            *d);

void
fb24_32CopyMtoN (DrawablePtr pSrcDrawable,
		 DrawablePtr pDstDrawable,
		 GCPtr       pGC,
		 BoxPtr      pbox,
		 int         nbox,
		 int         dx,
		 int         dy,
		 Bool        reverse,
		 Bool        upsidedown,
		 Pixel       bitplane,
		 void        *closure);

PixmapPtr
fb24_32ReformatTile(PixmapPtr pOldTile, int bitsPerPixel);
    
Bool
fb24_32CreateScreenResources(ScreenPtr pScreen);

Bool
fb24_32ModifyPixmapHeader (PixmapPtr   pPixmap,
			   int         width,
			   int         height,
			   int         depth,
			   int         bitsPerPixel,
			   int         devKind,
			   pointer     pPixData);

/*
 * fballpriv.c
 */
Bool
fbAllocatePrivates(ScreenPtr pScreen, int *pGCIndex);
    
/*
 * fbarc.c
 */

void
fbPolyArc (DrawablePtr	pDrawable,
	   GCPtr	pGC,
	   int		narcs,
	   xArc		*parcs);

/*
 * fbbits.c
 */

void	
fbBresSolid8(DrawablePtr    pDrawable,
	     GCPtr	    pGC,
	     int	    dashOffset,
	     int	    signdx,
	     int	    signdy,
	     int	    axis,
	     int	    x,
	     int	    y,
	     int	    e,
	     int	    e1,
	     int	    e3,
	     int	    len);

void	
fbBresDash8 (DrawablePtr    pDrawable,
	     GCPtr	    pGC,
	     int	    dashOffset,
	     int	    signdx,
	     int	    signdy,
	     int	    axis,
	     int	    x,
	     int	    y,
	     int	    e,
	     int	    e1,
	     int	    e3,
	     int	    len);

void	
fbDots8 (FbBits	    *dst,
	 FbStride   dstStride,
	 int	    dstBpp,
	 BoxPtr	    pBox,
	 xPoint	    *pts,
	 int	    npt,
	 int	    xorg,
	 int	    yorg,
	 int	    xoff,
	 int	    yoff,
	 FbBits	    and,
	 FbBits	    xor);

void	
fbArc8 (FbBits	    *dst,
	FbStride    dstStride,
	int	    dstBpp,
	xArc	    *arc,
	int	    dx,
	int	    dy,
	FbBits	    and,
	FbBits	    xor);

void
fbGlyph8 (FbBits    *dstLine,
	  FbStride  dstStride,
	  int	    dstBpp,
	  FbStip    *stipple,
	  FbBits    fg,
	  int	    height,
	  int	    shift);

void
fbPolyline8 (DrawablePtr    pDrawable,
	     GCPtr	    pGC,
	     int	    mode,
	     int	    npt,
	     DDXPointPtr    ptsOrig);

void
fbPolySegment8 (DrawablePtr pDrawable,
		GCPtr	    pGC,
		int	    nseg,
		xSegment    *pseg);

void	
fbBresSolid16(DrawablePtr   pDrawable,
	      GCPtr	    pGC,
	      int	    dashOffset,
	      int	    signdx,
	      int	    signdy,
	      int	    axis,
	      int	    x,
	      int	    y,
	      int	    e,
	      int	    e1,
	      int	    e3,
	      int	    len);

void	
fbBresDash16(DrawablePtr    pDrawable,
	     GCPtr	    pGC,
	     int	    dashOffset,
	     int	    signdx,
	     int	    signdy,
	     int	    axis,
	     int	    x,
	     int	    y,
	     int	    e,
	     int	    e1,
	     int	    e3,
	     int	    len);

void	
fbDots16(FbBits	    *dst,
	 FbStride   dstStride,
	 int	    dstBpp,
	 BoxPtr	    pBox,
	 xPoint	    *pts,
	 int	    npt,
	 int	    xorg,
	 int	    yorg,
	 int	    xoff,
	 int	    yoff,
	 FbBits	    and,
	 FbBits	    xor);

void	
fbArc16(FbBits	    *dst,
	FbStride    dstStride,
	int	    dstBpp,
	xArc	    *arc,
	int	    dx,
	int	    dy,
	FbBits	    and,
	FbBits	    xor);

void
fbGlyph16(FbBits    *dstLine,
	  FbStride  dstStride,
	  int	    dstBpp,
	  FbStip    *stipple,
	  FbBits    fg,
	  int	    height,
	  int	    shift);

void
fbPolyline16 (DrawablePtr   pDrawable,
	      GCPtr	    pGC,
	      int	    mode,
	      int	    npt,
	      DDXPointPtr   ptsOrig);

void
fbPolySegment16 (DrawablePtr	pDrawable,
		 GCPtr		pGC,
		 int		nseg,
		 xSegment	*pseg);


void	
fbBresSolid24(DrawablePtr   pDrawable,
	      GCPtr	    pGC,
	      int	    dashOffset,
	      int	    signdx,
	      int	    signdy,
	      int	    axis,
	      int	    x,
	      int	    y,
	      int	    e,
	      int	    e1,
	      int	    e3,
	      int	    len);

void	
fbBresDash24(DrawablePtr    pDrawable,
	     GCPtr	    pGC,
	     int	    dashOffset,
	     int	    signdx,
	     int	    signdy,
	     int	    axis,
	     int	    x,
	     int	    y,
	     int	    e,
	     int	    e1,
	     int	    e3,
	     int	    len);

void	
fbDots24(FbBits	    *dst,
	 FbStride   dstStride,
	 int	    dstBpp,
	 BoxPtr	    pBox,
	 xPoint	    *pts,
	 int	    npt,
	 int	    xorg,
	 int	    yorg,
	 int	    xoff,
	 int	    yoff,
	 FbBits	    and,
	 FbBits	    xor);

void	
fbArc24(FbBits	    *dst,
	FbStride    dstStride,
	int	    dstBpp,
	xArc	    *arc,
	int	    dx,
	int	    dy,
	FbBits	    and,
	FbBits	    xor);

void
fbGlyph24(FbBits    *dstLine,
	  FbStride  dstStride,
	  int	    dstBpp,
	  FbStip    *stipple,
	  FbBits    fg,
	  int	    height,
	  int	    shift);

void
fbPolyline24 (DrawablePtr   pDrawable,
	      GCPtr	    pGC,
	      int	    mode,
	      int	    npt,
	      DDXPointPtr   ptsOrig);

void
fbPolySegment24 (DrawablePtr	pDrawable,
		 GCPtr		pGC,
		 int		nseg,
		 xSegment	*pseg);


void	
fbBresSolid32(DrawablePtr   pDrawable,
	      GCPtr	    pGC,
	      int	    dashOffset,
	      int	    signdx,
	      int	    signdy,
	      int	    axis,
	      int	    x,
	      int	    y,
	      int	    e,
	      int	    e1,
	      int	    e3,
	      int	    len);

void	
fbBresDash32(DrawablePtr    pDrawable,
	     GCPtr	    pGC,
	     int	    dashOffset,
	     int	    signdx,
	     int	    signdy,
	     int	    axis,
	     int	    x,
	     int	    y,
	     int	    e,
	     int	    e1,
	     int	    e3,
	     int	    len);

void	
fbDots32(FbBits	    *dst,
	 FbStride   dstStride,
	 int	    dstBpp,
	 BoxPtr	    pBox,
	 xPoint	    *pts,
	 int	    npt,
	 int	    xorg,
	 int	    yorg,
	 int	    xoff,
	 int	    yoff,
	 FbBits	    and,
	 FbBits	    xor);

void	
fbArc32(FbBits	    *dst,
	FbStride    dstStride,
	int	    dstBpp,
	xArc	    *arc,
	int	    dx,
	int	    dy,
	FbBits	    and,
	FbBits	    xor);

void
fbGlyph32(FbBits    *dstLine,
	  FbStride  dstStride,
	  int	    dstBpp,
	  FbStip    *stipple,
	  FbBits    fg,
	  int	    height,
	  int	    shift);
void
fbPolyline32 (DrawablePtr   pDrawable,
	      GCPtr	    pGC,
	      int	    mode,
	      int	    npt,
	      DDXPointPtr   ptsOrig);

void
fbPolySegment32 (DrawablePtr	pDrawable,
		 GCPtr		pGC,
		 int		nseg,
		 xSegment	*pseg);

/*
 * fbblt.c
 */
void
fbBlt (FbBits   *src, 
       FbStride	srcStride,
       int	srcX,
       
       FbBits   *dst,
       FbStride dstStride,
       int	dstX,
       
       int	width, 
       int	height,
       
       int	alu,
       FbBits	pm,
       int	bpp,
       
       Bool	reverse,
       Bool	upsidedown);

void
fbBlt24 (FbBits	    *srcLine,
	 FbStride   srcStride,
	 int	    srcX,

	 FbBits	    *dstLine,
	 FbStride   dstStride,
	 int	    dstX,

	 int	    width, 
	 int	    height,

	 int	    alu,
	 FbBits	    pm,

	 Bool	    reverse,
	 Bool	    upsidedown);
    
void
fbBltStip (FbStip   *src,
	   FbStride srcStride,	    /* in FbStip units, not FbBits units */
	   int	    srcX,
	   
	   FbStip   *dst,
	   FbStride dstStride,	    /* in FbStip units, not FbBits units */
	   int	    dstX,

	   int	    width, 
	   int	    height,

	   int	    alu,
	   FbBits   pm,
	   int	    bpp);
    
/*
 * fbbltone.c
 */
void
fbBltOne (FbStip   *src,
	  FbStride srcStride,
	  int	   srcX,
	  FbBits   *dst,
	  FbStride dstStride,
	  int	   dstX,
	  int	   dstBpp,

	  int	   width,
	  int	   height,

	  FbBits   fgand,
	  FbBits   fbxor,
	  FbBits   bgand,
	  FbBits   bgxor);
 
#ifdef FB_24BIT
void
fbBltOne24 (FbStip    *src,
	  FbStride  srcStride,	    /* FbStip units per scanline */
	  int	    srcX,	    /* bit position of source */
	  FbBits    *dst,
	  FbStride  dstStride,	    /* FbBits units per scanline */
	  int	    dstX,	    /* bit position of dest */
	  int	    dstBpp,	    /* bits per destination unit */

	  int	    width,	    /* width in bits of destination */
	  int	    height,	    /* height in scanlines */

	  FbBits    fgand,	    /* rrop values */
	  FbBits    fgxor,
	  FbBits    bgand,
	  FbBits    bgxor);
#endif

void
fbBltPlane (FbBits	    *src,
	    FbStride	    srcStride,
	    int		    srcX,
	    int		    srcBpp,

	    FbStip	    *dst,
	    FbStride	    dstStride,
	    int		    dstX,
	    
	    int		    width,
	    int		    height,
	    
	    FbStip	    fgand,
	    FbStip	    fgxor,
	    FbStip	    bgand,
	    FbStip	    bgxor,
	    Pixel	    planeMask);

/*
 * fbbstore.c
 */
void
fbSaveAreas(PixmapPtr	pPixmap,
	    RegionPtr	prgnSave,
	    int		xorg,
	    int		yorg,
	    WindowPtr	pWin);

void
fbRestoreAreas(PixmapPtr    pPixmap,
	       RegionPtr    prgnRestore,
	       int	    xorg,
	       int	    yorg,
	       WindowPtr    pWin);

/*
 * fbcmap.c
 */
int
fbListInstalledColormaps(ScreenPtr pScreen, Colormap *pmaps);

void
fbInstallColormap(ColormapPtr pmap);

void
fbUninstallColormap(ColormapPtr pmap);

void
fbResolveColor(unsigned short	*pred, 
	       unsigned short	*pgreen, 
	       unsigned short	*pblue,
	       VisualPtr	pVisual);

Bool
fbInitializeColormap(ColormapPtr pmap);

int
fbExpandDirectColors (ColormapPtr   pmap, 
		      int	    ndef,
		      xColorItem    *indefs,
		      xColorItem    *outdefs);

Bool
fbCreateDefColormap(ScreenPtr pScreen);

void
fbClearVisualTypes(void);

Bool
fbHasVisualTypes (int depth);

Bool
fbSetVisualTypes (int depth, int visuals, int bitsPerRGB);

Bool
fbSetVisualTypesAndMasks (int depth, int visuals, int bitsPerRGB,
			  Pixel redMask, Pixel greenMask, Pixel blueMask);

Bool
fbInitVisuals (VisualPtr    *visualp, 
	       DepthPtr	    *depthp,
	       int	    *nvisualp,
	       int	    *ndepthp,
	       int	    *rootDepthp,
	       VisualID	    *defaultVisp,
	       unsigned long	sizes,
	       int	    bitsPerRGB);

/*
 * fbcopy.c
 */

typedef void	(*fbCopyProc) (DrawablePtr  pSrcDrawable,
			       DrawablePtr  pDstDrawable,
			       GCPtr	    pGC,
			       BoxPtr	    pDstBox,
			       int	    nbox,
			       int	    dx,
			       int	    dy,
			       Bool	    reverse,
			       Bool	    upsidedown,
			       Pixel	    bitplane,
			       void	    *closure);

void
fbCopyNtoN (DrawablePtr	pSrcDrawable,
	    DrawablePtr	pDstDrawable,
	    GCPtr	pGC,
	    BoxPtr	pbox,
	    int		nbox,
	    int		dx,
	    int		dy,
	    Bool	reverse,
	    Bool	upsidedown,
	    Pixel	bitplane,
	    void	*closure);

void
fbCopy1toN (DrawablePtr	pSrcDrawable,
	    DrawablePtr	pDstDrawable,
	    GCPtr	pGC,
	    BoxPtr	pbox,
	    int		nbox,
	    int		dx,
	    int		dy,
	    Bool	reverse,
	    Bool	upsidedown,
	    Pixel	bitplane,
	    void	*closure);

void
fbCopyNto1 (DrawablePtr	pSrcDrawable,
	    DrawablePtr	pDstDrawable,
	    GCPtr	pGC,
	    BoxPtr	pbox,
	    int		nbox,
	    int		dx,
	    int		dy,
	    Bool	reverse,
	    Bool	upsidedown,
	    Pixel	bitplane,
	    void	*closure);

void
fbCopyRegion (DrawablePtr   pSrcDrawable,
	      DrawablePtr   pDstDrawable,
	      GCPtr	    pGC,
	      RegionPtr	    pDstRegion,
	      int	    dx,
	      int	    dy,
	      fbCopyProc    copyProc,
	      Pixel	    bitPlane,
	      void	    *closure);

RegionPtr
fbDoCopy (DrawablePtr	pSrcDrawable,
	  DrawablePtr	pDstDrawable,
	  GCPtr		pGC,
	  int		xIn, 
	  int		yIn,
	  int		widthSrc, 
	  int		heightSrc,
	  int		xOut, 
	  int		yOut,
	  fbCopyProc	copyProc,
	  Pixel		bitplane,
	  void		*closure);
	  
RegionPtr
fbCopyArea (DrawablePtr	pSrcDrawable,
	    DrawablePtr	pDstDrawable,
	    GCPtr	pGC,
	    int		xIn, 
	    int		yIn,
	    int		widthSrc, 
	    int		heightSrc,
	    int		xOut, 
	    int		yOut);

RegionPtr
fbCopyPlane (DrawablePtr    pSrcDrawable,
	     DrawablePtr    pDstDrawable,
	     GCPtr	    pGC,
	     int	    xIn, 
	     int	    yIn,
	     int	    widthSrc, 
	     int	    heightSrc,
	     int	    xOut, 
	     int	    yOut,
	     unsigned long  bitplane);

/*
 * fbfill.c
 */
void
fbFill (DrawablePtr pDrawable,
	GCPtr	    pGC,
	int	    x,
	int	    y,
	int	    width,
	int	    height);

void
fbSolidBoxClipped (DrawablePtr	pDrawable,
		   RegionPtr	pClip,
		   int		xa,
		   int		ya,
		   int		xb,
		   int		yb,
		   FbBits	and,
		   FbBits	xor);

/*
 * fbfillrect.c
 */
void
fbPolyFillRect(DrawablePtr  pDrawable, 
	       GCPtr	    pGC, 
	       int	    nrectInit,
	       xRectangle   *prectInit);

#define fbPolyFillArc miPolyFillArc

#define fbFillPolygon miFillPolygon

/*
 * fbfillsp.c
 */
void
fbFillSpans (DrawablePtr    pDrawable,
	     GCPtr	    pGC,
	     int	    nInit,
	     DDXPointPtr    pptInit,
	     int	    *pwidthInit,
	     int	    fSorted);


/*
 * fbgc.c
 */

Bool
fbCreateGC(GCPtr pGC);

void
fbPadPixmap (PixmapPtr pPixmap);
    
void
fbValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDrawable);

/*
 * fbgetsp.c
 */
void
fbGetSpans(DrawablePtr	pDrawable, 
	   int		wMax, 
	   DDXPointPtr	ppt, 
	   int		*pwidth, 
	   int		nspans, 
	   char		*pchardstStart);

/*
 * fbglyph.c
 */

Bool
fbGlyphIn (RegionPtr	pRegion,
	   int		x,
	   int		y,
	   int		width,
	   int		height);
    
void
fbPolyGlyphBlt (DrawablePtr	pDrawable,
		GCPtr		pGC,
		int		x, 
		int		y,
		unsigned int	nglyph,
		CharInfoPtr	*ppci,
		pointer		pglyphBase);

void
fbImageGlyphBlt (DrawablePtr	pDrawable,
		 GCPtr		pGC,
		 int		x,
		 int		y,
		 unsigned int	nglyph,
		 CharInfoPtr	*ppci,
		 pointer	pglyphBase);

/*
 * fbimage.c
 */

void
fbPutImage (DrawablePtr	pDrawable,
	    GCPtr	pGC,
	    int		depth,
	    int		x,
	    int		y,
	    int		w,
	    int		h,
	    int		leftPad,
	    int		format,
	    char	*pImage);

void
fbPutZImage (DrawablePtr	pDrawable,
	     RegionPtr		pClip,
	     int		alu,
	     FbBits		pm,
	     int		x,
	     int		y,
	     int		width,
	     int		height,
	     FbStip		*src,
	     FbStride		srcStride);

void
fbPutXYImage (DrawablePtr	pDrawable,
	      RegionPtr		pClip,
	      FbBits		fg,
	      FbBits		bg,
	      FbBits		pm,
	      int		alu,
	      Bool		opaque,
	      
	      int		x,
	      int		y,
	      int		width,
	      int		height,

	      FbStip		*src,
	      FbStride		srcStride,
	      int		srcX);

void
fbGetImage (DrawablePtr	    pDrawable,
	    int		    x,
	    int		    y,
	    int		    w,
	    int		    h,
	    unsigned int    format,
	    unsigned long   planeMask,
	    char	    *d);
/*
 * fbline.c
 */

void
fbZeroLine (DrawablePtr	pDrawable,
	    GCPtr	pGC,
	    int		mode,
	    int		npt,
	    DDXPointPtr	ppt);

void
fbZeroSegment (DrawablePtr  pDrawable,
	       GCPtr	    pGC,
	       int	    nseg,
	       xSegment	    *pSegs);

void
fbPolyLine (DrawablePtr	pDrawable,
	    GCPtr	pGC,
	    int		mode,
	    int		npt,
	    DDXPointPtr	ppt);

void
fbFixCoordModePrevious (int npt,
			DDXPointPtr ppt);

void
fbPolySegment (DrawablePtr  pDrawable,
	       GCPtr	    pGC,
	       int	    nseg,
	       xSegment	    *pseg);

#define fbPolyRectangle	miPolyRectangle

/*
 * fbpict.c
 */

Bool
fbPictureInit (ScreenPtr pScreen,
	       PictFormatPtr formats,
	       int nformats);

/*
 * fbpixmap.c
 */

PixmapPtr
fbCreatePixmapBpp (ScreenPtr pScreen, int width, int height, int depth, int bpp);

PixmapPtr
fbCreatePixmap (ScreenPtr pScreen, int width, int height, int depth);

Bool
fbDestroyPixmap (PixmapPtr pPixmap);

RegionPtr
fbPixmapToRegion(PixmapPtr pPix);

/*
 * fbpoint.c
 */

void
fbDots (FbBits	    *dstOrig,
	FbStride    dstStride,
	int	    dstBpp,
	BoxPtr	    pBox,
	xPoint	    *pts,
	int	    npt,
	int	    xorg,
	int	    yorg,
	int	    xoff,
	int	    yoff,
	FbBits	    andOrig,
	FbBits	    xorOrig);

void
fbPolyPoint (DrawablePtr    pDrawable,
	     GCPtr	    pGC,
	     int	    mode,
	     int	    npt,
	     xPoint	    *pptInit);

/*
 * fbpush.c
 */
void
fbPushPattern (DrawablePtr  pDrawable,
	       GCPtr	    pGC,
	       
	       FbStip	    *src,
	       FbStride	    srcStride,
	       int	    srcX,

	       int	    x,
	       int	    y,

	       int	    width,
	       int	    height);

void
fbPushFill (DrawablePtr	pDrawable,
	    GCPtr	pGC,

	    FbStip	*src,
	    FbStride	srcStride,
	    int		srcX,
	    
	    int		x,
	    int		y,
	    int		width,
	    int		height);

void
fbPush1toN (DrawablePtr	pSrcDrawable,
	    DrawablePtr	pDstDrawable,
	    GCPtr	pGC,
	    BoxPtr	pbox,
	    int		nbox,
	    int		dx,
	    int		dy,
	    Bool	reverse,
	    Bool	upsidedown,
	    Pixel	bitplane,
	    void	*closure);

void
fbPushImage (DrawablePtr    pDrawable,
	     GCPtr	    pGC,
	     
	     FbStip	    *src,
	     FbStride	    srcStride,
	     int	    srcX,

	     int	    x,
	     int	    y,
	     int	    width,
	     int	    height);

void
fbPushPixels (GCPtr	    pGC,
	      PixmapPtr	    pBitmap,
	      DrawablePtr   pDrawable,
	      int	    dx,
	      int	    dy,
	      int	    xOrg,
	      int	    yOrg);


/*
 * fbscreen.c
 */

Bool
fbCloseScreen (int indx, ScreenPtr pScreen);

Bool
fbRealizeFont(ScreenPtr pScreen, FontPtr pFont);

Bool
fbUnrealizeFont(ScreenPtr pScreen, FontPtr pFont);

void
fbQueryBestSize (int class, 
		 unsigned short *width, unsigned short *height,
		 ScreenPtr pScreen);

PixmapPtr
_fbGetWindowPixmap (WindowPtr pWindow);

void
_fbSetWindowPixmap (WindowPtr pWindow, PixmapPtr pPixmap);

Bool
fbSetupScreen(ScreenPtr	pScreen, 
	      pointer	pbits,		/* pointer to screen bitmap */
	      int	xsize, 		/* in pixels */
	      int	ysize,
	      int	dpix,		/* dots per inch */
	      int	dpiy,
	      int	width,		/* pixel width of frame buffer */
	      int	bpp);		/* bits per pixel of frame buffer */

Bool
wfbFinishScreenInit(ScreenPtr	pScreen,
		    pointer	pbits,
		    int		xsize,
		    int		ysize,
		    int		dpix,
		    int		dpiy,
		    int		width,
		    int		bpp,
		    SetupWrapProcPtr setupWrap,
		    FinishWrapProcPtr finishWrap);

Bool
wfbScreenInit(ScreenPtr	pScreen,
	      pointer	pbits,
	      int	xsize,
	      int	ysize,
	      int	dpix,
	      int	dpiy,
	      int	width,
	      int	bpp,
	      SetupWrapProcPtr setupWrap,
	      FinishWrapProcPtr finishWrap);

Bool
fbFinishScreenInit(ScreenPtr	pScreen,
		   pointer	pbits,
		   int		xsize,
		   int		ysize,
		   int		dpix,
		   int		dpiy,
		   int		width,
		   int		bpp);

Bool
fbScreenInit(ScreenPtr	pScreen,
	     pointer	pbits,
	     int	xsize,
	     int	ysize,
	     int	dpix,
	     int	dpiy,
	     int	width,
	     int	bpp);

void
fbInitializeBackingStore (ScreenPtr pScreen);
    
/*
 * fbseg.c
 */
typedef void	FbBres (DrawablePtr	pDrawable,
			GCPtr		pGC,
			int		dashOffset,
			int		signdx,
			int		signdy,
			int		axis,
			int		x,
			int		y,
			int		e,
			int		e1,
			int		e3,
			int		len);

FbBres fbBresSolid, fbBresDash, fbBresFill, fbBresFillDash;
/*
 * fbsetsp.c
 */

void
fbSetSpans (DrawablePtr	    pDrawable,
	    GCPtr	    pGC,
	    char	    *src,
	    DDXPointPtr	    ppt,
	    int		    *pwidth,
	    int		    nspans,
	    int		    fSorted);

FbBres *
fbSelectBres (DrawablePtr   pDrawable,
	      GCPtr	    pGC);

void
fbBres (DrawablePtr	pDrawable,
	GCPtr		pGC,
	int		dashOffset,
	int		signdx,
	int		signdy,
	int		axis,
	int		x,
	int		y,
	int		e,
	int		e1,
	int		e3,
	int		len);

void
fbSegment (DrawablePtr	pDrawable,
	   GCPtr	pGC,
	   int		xa,
	   int		ya,
	   int		xb,
	   int		yb,
	   Bool		drawLast,
	   int		*dashOffset);


/*
 * fbsolid.c
 */

void
fbSolid (FbBits	    *dst,
	 FbStride   dstStride,
	 int	    dstX,
	 int	    bpp,

	 int	    width,
	 int	    height,

	 FbBits	    and,
	 FbBits	    xor);

#ifdef FB_24BIT
void
fbSolid24 (FbBits   *dst,
	   FbStride dstStride,
	   int	    dstX,

	   int	    width,
	   int	    height,

	   FbBits   and,
	   FbBits   xor);
#endif

/*
 * fbstipple.c
 */

void
fbTransparentSpan (FbBits   *dst,
		   FbBits   stip,
		   FbBits   fgxor,
		   int	    n);

void
fbEvenStipple (FbBits   *dst,
	       FbStride dstStride,
	       int	dstX,
	       int	dstBpp,

	       int	width,
	       int	height,

	       FbStip   *stip,
	       FbStride	stipStride,
	       int	stipHeight,

	       FbBits   fgand,
	       FbBits   fgxor,
	       FbBits   bgand,
	       FbBits   bgxor,

	       int	xRot,
	       int	yRot);

void
fbOddStipple (FbBits	*dst,
	      FbStride	dstStride,
	      int	dstX,
	      int	dstBpp,

	      int	width,
	      int	height,

	      FbStip	*stip,
	      FbStride	stipStride,
	      int	stipWidth,
	      int	stipHeight,

	      FbBits	fgand,
	      FbBits	fgxor,
	      FbBits	bgand,
	      FbBits	bgxor,

	      int	xRot,
	      int	yRot);

void
fbStipple (FbBits   *dst,
	   FbStride dstStride,
	   int	    dstX,
	   int	    dstBpp,

	   int	    width,
	   int	    height,

	   FbStip   *stip,
	   FbStride stipStride,
	   int	    stipWidth,
	   int	    stipHeight,
	   Bool	    even,

	   FbBits   fgand,
	   FbBits   fgxor,
	   FbBits   bgand,
	   FbBits   bgxor,

	   int	    xRot,
	   int	    yRot);

/*
 * fbtile.c
 */

void
fbEvenTile (FbBits	*dst,
	    FbStride	dstStride,
	    int		dstX,

	    int		width,
	    int		height,

	    FbBits	*tile,
	    FbStride	tileStride,
	    int		tileHeight,

	    int		alu,
	    FbBits	pm,
	    int		xRot,
	    int		yRot);

void
fbOddTile (FbBits	*dst,
	   FbStride	dstStride,
	   int		dstX,

	   int		width,
	   int		height,

	   FbBits	*tile,
	   FbStride	tileStride,
	   int		tileWidth,
	   int		tileHeight,

	   int		alu,
	   FbBits	pm,
	   int		bpp,
	   
	   int		xRot,
	   int		yRot);

void
fbTile (FbBits	    *dst,
	FbStride    dstStride,
	int	    dstX,

	int	    width,
	int	    height,

	FbBits	    *tile,
	FbStride    tileStride,
	int	    tileWidth,
	int	    tileHeight,
	
	int	    alu,
	FbBits	    pm,
	int	    bpp,
	
	int	    xRot,
	int	    yRot);

/*
 * fbutil.c
 */
FbBits
fbReplicatePixel (Pixel p, int bpp);

void
fbReduceRasterOp (int rop, FbBits fg, FbBits pm, FbBits *andp, FbBits *xorp);

#ifdef FB_ACCESS_WRAPPER
extern ReadMemoryProcPtr wfbReadMemory;
extern WriteMemoryProcPtr wfbWriteMemory;
#endif

/*
 * fbwindow.c
 */

Bool
fbCreateWindow(WindowPtr pWin);

Bool
fbDestroyWindow(WindowPtr pWin);

Bool
fbMapWindow(WindowPtr pWindow);

Bool
fbPositionWindow(WindowPtr pWin, int x, int y);

Bool 
fbUnmapWindow(WindowPtr pWindow);
    
void
fbCopyWindowProc (DrawablePtr	pSrcDrawable,
		  DrawablePtr	pDstDrawable,
		  GCPtr		pGC,
		  BoxPtr	pbox,
		  int		nbox,
		  int		dx,
		  int		dy,
		  Bool		reverse,
		  Bool		upsidedown,
		  Pixel		bitplane,
		  void		*closure);

void 
fbCopyWindow(WindowPtr	    pWin, 
	     DDXPointRec    ptOldOrg, 
	     RegionPtr	    prgnSrc);

Bool
fbChangeWindowAttributes(WindowPtr pWin, unsigned long mask);

void
fbFillRegionSolid (DrawablePtr	pDrawable,
		   RegionPtr	pRegion,
		   FbBits	and,
		   FbBits	xor);

void
fbFillRegionTiled (DrawablePtr	pDrawable,
		   RegionPtr	pRegion,
		   PixmapPtr	pTile);

void
fbPaintWindow(WindowPtr pWin, RegionPtr pRegion, int what);


pixman_image_t *image_from_pict (PicturePtr pict,
				 Bool       has_clip);
void free_pixman_pict (PicturePtr, pixman_image_t *);

#endif /* _FB_H_ */

