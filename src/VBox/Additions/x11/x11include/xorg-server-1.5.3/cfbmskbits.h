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

/* Optimizations for PSZ == 32 added by Kyle Marvin (marvin@vitec.com) */

#include	<X11/X.h>
#include	<X11/Xmd.h>
#include	"servermd.h"
#include	"compiler.h"

/*
 * ==========================================================================
 * Converted from mfb to support memory-mapped color framebuffer by smarks@sun, 
 * April-May 1987.
 *
 * The way I did the conversion was to consider each longword as an
 * array of four bytes instead of an array of 32 one-bit pixels.  So
 * getbits() and putbits() retain much the same calling sequence, but
 * they move bytes around instead of bits.  Of course, this entails the
 * removal of all of the one-bit-pixel dependencies from the other
 * files, but the major bit-hacking stuff should be covered here.
 *
 * I've created some new macros that make it easier to understand what's 
 * going on in the pixel calculations, and that make it easier to change the 
 * pixel size.
 *
 * name	    explanation
 * ----	    -----------
 * PSZ	    pixel size (in bits)
 * PGSZ     pixel group size (in bits)
 * PGSZB    pixel group size (in bytes)
 * PGSZBMSK mask with lowest PGSZB bits set to 1
 * PPW	    pixels per word (pixels per pixel group)
 * PPWMSK   mask with lowest PPW bits set to 1
 * PLST	    index of last pixel in a word (should be PPW-1)
 * PIM	    pixel index mask (index within a pixel group)
 * PWSH	    pixel-to-word shift (should be log2(PPW))
 * PMSK	    mask with lowest PSZ bits set to 1
 *
 *
 * Here are some sample values.  In the notation cfbA,B: A is PSZ, and
 * B is PGSZB.  All the other values are derived from these
 * two.  This table does not show all combinations!
 *
 * name	    cfb8,4    cfb24,4      cfb32,4    cfb8,8    cfb24,8    cfb32,8
 * ----	    ------    -------      ------     ------    ------     -------
 * PSZ	      8	        24	     32          8        24         32
 * PGSZ	     32         32           32         64        64         64
 * PGSZB      4          4            4          8         8          8
 * PGSZBMSK 0xF        0xF?         0xF        0xFF      0xFF       0xFF
 * PPW	      4	         1            1          8         2          2
 * PPWMSK   0xF        0x1          0x1        0xFF       0x3?       0x3    
 * PLST	      3	         0            0	         7         1          1
 * PIM	    0x3        0x0          0x0	       0x7       0x1?        0x1
 * PWSH	      2	         0            0	         3         1          1
 * PMSK	    0xFF      0xFFFFFF     0xFFFFFFFF 0xFF      0xFFFFFF   0xFFFFFFFF
 *
 *
 * I have also added a new macro, PFILL, that takes one pixel and
 * replicates it throughout a word.  This macro definition is dependent
 * upon pixel and word size; it doesn't use macros like PPW and so
 * forth.  Examples: for monochrome, PFILL(1) => 0xffffffff, PFILL(0) =>
 * 0x00000000.  For 8-bit color, PFILL(0x5d) => 0x5d5d5d5d.  This macro
 * is used primarily for replicating a plane mask into a word.
 *
 * Color framebuffers operations also support the notion of a plane
 * mask.  This mask determines which planes of the framebuffer can be
 * altered; the others are left unchanged.  I have added another
 * parameter to the putbits and putbitsrop macros that is the plane
 * mask.
 * ==========================================================================
 *
 * Keith Packard (keithp@suse.com)
 * 64bit code is no longer supported; it requires DIX support
 * for repadding images which significantly impacts performance
 */

/*
 *  PSZ needs to be defined before we get here.  Usually it comes from a
 *  -DPSZ=foo on the compilation command line.
 */

#ifndef PSZ
#define PSZ 8
#endif

/*
 *  PixelGroup is the data type used to operate on groups of pixels.
 *  We typedef it here to CARD32 with the assumption that you
 *  want to manipulate 32 bits worth of pixels at a time as you can.  If CARD32
 *  is not appropriate for your server, define it to something else
 *  before including this file.  In this case you will also have to define
 *  PGSZB to the size in bytes of PixelGroup.
 */
#ifndef PixelGroup
#define PixelGroup CARD32
#define PGSZB 4
#endif /* PixelGroup */
    
#ifndef CfbBits
#define CfbBits	CARD32
#endif

#define PGSZ	(PGSZB << 3)
#define PPW	(PGSZ/PSZ)
#define PLST	(PPW-1)
#define PIM	PLST
#define PMSK	(((PixelGroup)1 << PSZ) - 1)
#define PPWMSK  (((PixelGroup)1 << PPW) - 1) /* instead of BITMSK */
#define PGSZBMSK (((PixelGroup)1 << PGSZB) - 1)

/*  set PWSH = log2(PPW) using brute force */

#if PPW == 1
#define PWSH 0
#else
#if PPW == 2
#define PWSH 1
#else
#if PPW == 4
#define PWSH 2
#else
#if PPW == 8
#define PWSH 3
#else
#if PPW == 16
#define PWSH 4
#endif /* PPW == 16 */
#endif /* PPW == 8 */
#endif /* PPW == 4 */
#endif /* PPW == 2 */
#endif /* PPW == 1 */

/*  Defining PIXEL_ADDR means that individual pixels are addressable by this
 *  machine (as type PixelType).  A possible CFB architecture which supported
 *  8-bits-per-pixel on a non byte-addressable machine would not have this
 *  defined.
 *
 *  Defining FOUR_BIT_CODE means that cfb knows how to stipple on this machine;
 *  eventually, stippling code for 16 and 32 bit devices should be written
 *  which would allow them to also use FOUR_BIT_CODE.  There isn't that
 *  much to do in those cases, but it would make them quite a bit faster.
 */

#if PSZ == 8
#define PIXEL_ADDR
typedef CARD8 PixelType;
#define FOUR_BIT_CODE
#endif

#if PSZ == 16
#define PIXEL_ADDR
typedef CARD16 PixelType;
#endif

#if PSZ == 24
#undef PMSK
#define PMSK	0xFFFFFF
/*#undef PIM
#define PIM 3*/
#define PIXEL_ADDR
typedef CARD32 PixelType;
#endif

#if PSZ == 32
#undef PMSK
#define PMSK	0xFFFFFFFF
#define PIXEL_ADDR
typedef CARD32 PixelType;
#endif


/* the following notes use the following conventions:
SCREEN LEFT				SCREEN RIGHT
in this file and maskbits.c, left and right refer to screen coordinates,
NOT bit numbering in registers.

cfbstarttab[n] 
	pixels[0,n-1] = 0's	pixels[n,PPW-1] = 1's
cfbendtab[n] =
	pixels[0,n-1] = 1's	pixels[n,PPW-1] = 0's

cfbstartpartial[], cfbendpartial[]
	these are used as accelerators for doing putbits and masking out
bits that are all contained between longword boudaries.  the extra
256 bytes of data seems a small price to pay -- code is smaller,
and narrow things (e.g. window borders) go faster.

the names may seem misleading; they are derived not from which end
of the word the bits are turned on, but at which end of a scanline
the table tends to be used.

look at the tables and macros to understand boundary conditions.
(careful readers will note that starttab[n] = ~endtab[n] for n != 0)

-----------------------------------------------------------------------
these two macros depend on the screen's bit ordering.
in both of them x is a screen position.  they are used to
combine bits collected from multiple longwords into a
single destination longword, and to unpack a single
source longword into multiple destinations.

SCRLEFT(dst, x)
	takes dst[x, PPW] and moves them to dst[0, PPW-x]
	the contents of the rest of dst are 0 ONLY IF
	dst is UNSIGNED.
	is cast as an unsigned.
	this is a right shift on the VAX, left shift on
	Sun and pc-rt.

SCRRIGHT(dst, x)
	takes dst[0,x] and moves them to dst[PPW-x, PPW]
	the contents of the rest of dst are 0 ONLY IF
	dst is UNSIGNED.
	this is a left shift on the VAX, right shift on
	Sun and pc-rt.


the remaining macros are cpu-independent; all bit order dependencies
are built into the tables and the two macros above.

maskbits(x, w, startmask, endmask, nlw)
	for a span of width w starting at position x, returns
a mask for ragged pixels at start, mask for ragged pixels at end,
and the number of whole longwords between the ends.

maskpartialbits(x, w, mask)
	works like maskbits(), except all the pixels are in the
	same longword (i.e. (x&0xPIM + w) <= PPW)

mask32bits(x, w, startmask, endmask, nlw)
	as maskbits, but does not calculate nlw.  it is used by
	cfbGlyphBlt to put down glyphs <= PPW bits wide.

getbits(psrc, x, w, dst)
	starting at position x in psrc (x < PPW), collect w
	pixels and put them in the screen left portion of dst.
	psrc is a longword pointer.  this may span longword boundaries.
	it special-cases fetching all w bits from one longword.

	+--------+--------+		+--------+
	|    | m |n|      |	==> 	| m |n|  |
	+--------+--------+		+--------+
	    x      x+w			0     w
	psrc     psrc+1			dst
			m = PPW - x
			n = w - m

	implementation:
	get m pixels, move to screen-left of dst, zeroing rest of dst;
	get n pixels from next word, move screen-right by m, zeroing
		 lower m pixels of word.
	OR the two things together.

putbits(src, x, w, pdst, planemask)
	starting at position x in pdst, put down the screen-leftmost
	w bits of src.  pdst is a longword pointer.  this may
	span longword boundaries.
	it special-cases putting all w bits into the same longword.

	+--------+			+--------+--------+
	| m |n|  |		==>	|    | m |n|      |
	+--------+			+--------+--------+
	0     w				     x     x+w
	dst				pdst     pdst+1
			m = PPW - x
			n = w - m

	implementation:
	get m pixels, shift screen-right by x, zero screen-leftmost x
		pixels; zero rightmost m bits of *pdst and OR in stuff
		from before the semicolon.
	shift src screen-left by m, zero bits n-32;
		zero leftmost n pixels of *(pdst+1) and OR in the
		stuff from before the semicolon.

putbitsrop(src, x, w, pdst, planemask, ROP)
	like putbits but calls DoRop with the rasterop ROP (see cfb.h for
	DoRop)

getleftbits(psrc, w, dst)
	get the leftmost w (w<=PPW) bits from *psrc and put them
	in dst.  this is used by the cfbGlyphBlt code for glyphs
	<=PPW bits wide.
*/

#if	(BITMAP_BIT_ORDER == MSBFirst)
#define BitRight(lw,n)	((lw) >> (n))
#define BitLeft(lw,n)	((lw) << (n))
#else	/* (BITMAP_BIT_ORDER == LSBFirst) */
#define BitRight(lw,n)	((lw) << (n))
#define BitLeft(lw,n)	((lw) >> (n))
#endif	/* (BITMAP_BIT_ORDER == MSBFirst) */

#define SCRLEFT(lw, n)	BitLeft (lw, (n) * PSZ)
#define SCRRIGHT(lw, n)	BitRight(lw, (n) * PSZ)

/*
 * Note that the shift direction is independent of the byte ordering of the 
 * machine.  The following is portable code.
 */
#if PPW == 16
#define PFILL(p) ( ((p)&PMSK)          | \
		   ((p)&PMSK) <<   PSZ | \
		   ((p)&PMSK) << 2*PSZ | \
		   ((p)&PMSK) << 3*PSZ | \
		   ((p)&PMSK) << 4*PSZ | \
		   ((p)&PMSK) << 5*PSZ | \
		   ((p)&PMSK) << 6*PSZ | \
		   ((p)&PMSK) << 7*PSZ | \
		   ((p)&PMSK) << 8*PSZ | \
		   ((p)&PMSK) << 9*PSZ | \
		   ((p)&PMSK) << 10*PSZ | \
		   ((p)&PMSK) << 11*PSZ | \
		   ((p)&PMSK) << 12*PSZ | \
		   ((p)&PMSK) << 13*PSZ | \
		   ((p)&PMSK) << 14*PSZ | \
		   ((p)&PMSK) << 15*PSZ ) 
#define PFILL2(p, pf) { \
    pf = (p) & PMSK; \
    pf |= (pf << PSZ); \
    pf |= (pf << 2*PSZ); \
    pf |= (pf << 4*PSZ); \
    pf |= (pf << 8*PSZ); \
}
#endif /* PPW == 16 */
#if PPW == 8
#define PFILL(p) ( ((p)&PMSK)          | \
		   ((p)&PMSK) <<   PSZ | \
		   ((p)&PMSK) << 2*PSZ | \
		   ((p)&PMSK) << 3*PSZ | \
		   ((p)&PMSK) << 4*PSZ | \
		   ((p)&PMSK) << 5*PSZ | \
		   ((p)&PMSK) << 6*PSZ | \
		   ((p)&PMSK) << 7*PSZ )
#define PFILL2(p, pf) { \
    pf = (p) & PMSK; \
    pf |= (pf << PSZ); \
    pf |= (pf << 2*PSZ); \
    pf |= (pf << 4*PSZ); \
}
#endif
#if PPW == 4
#define PFILL(p) ( ((p)&PMSK)          | \
		   ((p)&PMSK) <<   PSZ | \
		   ((p)&PMSK) << 2*PSZ | \
		   ((p)&PMSK) << 3*PSZ )
#define PFILL2(p, pf) { \
    pf = (p) & PMSK; \
    pf |= (pf << PSZ); \
    pf |= (pf << 2*PSZ); \
}
#endif
#if PPW == 2
#define PFILL(p) ( ((p)&PMSK)          | \
		   ((p)&PMSK) <<   PSZ )
#define PFILL2(p, pf) { \
    pf = (p) & PMSK; \
    pf |= (pf << PSZ); \
}
#endif
#if PPW == 1
#define PFILL(p)	(p)
#define PFILL2(p,pf)	(pf = (p))
#endif

/*
 * Reduced raster op - using precomputed values, perform the above
 * in three instructions
 */

#define DoRRop(dst, and, xor)	(((dst) & (and)) ^ (xor))

#define DoMaskRRop(dst, and, xor, mask) \
    (((dst) & ((and) | ~(mask))) ^ (xor & mask))

#if PSZ != 32 || PPW != 1

# if (PSZ == 24 && PPW == 1)
#define maskbits(x, w, startmask, endmask, nlw) {\
    startmask = cfbstarttab[(x)&3]; \
    endmask = cfbendtab[((x)+(w)) & 3]; \
    nlw = ((((x)+(w))*3)>>2) - (((x)*3 +3)>>2); \
}

#define mask32bits(x, w, startmask, endmask) \
    startmask = cfbstarttab[(x)&3]; \
    endmask = cfbendtab[((x)+(w)) & 3];

#define maskpartialbits(x, w, mask) \
    mask = cfbstartpartial[(x) & 3] & cfbendpartial[((x)+(w)) & 3];

#define maskbits24(x, w, startmask, endmask, nlw) \
    startmask = cfbstarttab24[(x) & 3]; \
    endmask = cfbendtab24[((x)+(w)) & 3]; \
    if (startmask){ \
	nlw = (((w) - (4 - ((x) & 3))) >> 2); \
    } else { \
	nlw = (w) >> 2; \
    }

#define getbits24(psrc, dst, index) {\
    register int idx; \
    switch(idx = ((index)&3)<<1){ \
    	case 0: \
		dst = (*(psrc) &cfbmask[idx]); \
		break; \
    	case 6: \
		dst = BitLeft((*(psrc) &cfbmask[idx]), cfb24Shift[idx]); \
		break; \
	default: \
		dst = BitLeft((*(psrc) &cfbmask[idx]), cfb24Shift[idx]) | \
		BitRight(((*((psrc)+1)) &cfbmask[idx+1]), cfb24Shift[idx+1]); \
	}; \
}

#define putbits24(src, w, pdst, planemask, index) {\
    register PixelGroup dstpixel; \
    register unsigned int idx; \
    switch(idx = ((index)&3)<<1){ \
    	case 0: \
		dstpixel = (*(pdst) &cfbmask[idx]); \
		break; \
    	case 6: \
		dstpixel = BitLeft((*(pdst) &cfbmask[idx]), cfb24Shift[idx]); \
		break; \
	default: \
		dstpixel = BitLeft((*(pdst) &cfbmask[idx]), cfb24Shift[idx])| \
		BitRight(((*((pdst)+1)) &cfbmask[idx+1]), cfb24Shift[idx+1]); \
	}; \
    dstpixel &= ~(planemask); \
    dstpixel |= (src & planemask); \
    *(pdst) &= cfbrmask[idx]; \
    switch(idx){ \
    	case 0: \
		*(pdst) |=  (dstpixel & cfbmask[idx]); \
		break; \
    	case 2: \
    	case 4: \
		pdst++;idx++; \
		*(pdst) = ((*(pdst))  & cfbrmask[idx]) | \
				(BitLeft(dstpixel, cfb24Shift[idx]) & cfbmask[idx]); \
		pdst--;idx--; \
    	case 6: \
		*(pdst) |=  (BitRight(dstpixel, cfb24Shift[idx]) & cfbmask[idx]); \
		break; \
	}; \
}

#define putbitsrop24(src, x, pdst, planemask, rop) \
{ \
    register PixelGroup t1, dstpixel; \
    register unsigned int idx; \
    switch(idx = (x)<<1){ \
    	case 0: \
		dstpixel = (*(pdst) &cfbmask[idx]); \
		break; \
    	case 6: \
		dstpixel = BitLeft((*(pdst) &cfbmask[idx]), cfb24Shift[idx]); \
		break; \
	default: \
		dstpixel = BitLeft((*(pdst) &cfbmask[idx]), cfb24Shift[idx])| \
		BitRight(((*((pdst)+1)) &cfbmask[idx+1]), cfb24Shift[idx+1]); \
	}; \
    DoRop(t1, rop, (src), dstpixel); \
    dstpixel &= ~planemask; \
    dstpixel |= (t1 & planemask); \
    *(pdst) &= cfbrmask[idx]; \
    switch(idx){ \
    	case 0: \
		*(pdst) |= (dstpixel & cfbmask[idx]); \
		break; \
    	case 2: \
    	case 4: \
		*((pdst)+1) = ((*((pdst)+1))  & cfbrmask[idx+1]) | \
				(BitLeft(dstpixel, cfb24Shift[idx+1]) & (cfbmask[idx+1])); \
    	case 6: \
		*(pdst) |= (BitRight(dstpixel, cfb24Shift[idx]) & cfbmask[idx]); \
	}; \
}
# else  /* PSZ == 24 && PPW == 1 */
#define maskbits(x, w, startmask, endmask, nlw) \
    startmask = cfbstarttab[(x)&PIM]; \
    endmask = cfbendtab[((x)+(w)) & PIM]; \
    if (startmask) \
	nlw = (((w) - (PPW - ((x)&PIM))) >> PWSH); \
    else \
	nlw = (w) >> PWSH;

#define maskpartialbits(x, w, mask) \
    mask = cfbstartpartial[(x) & PIM] & cfbendpartial[((x) + (w)) & PIM];

#define mask32bits(x, w, startmask, endmask) \
    startmask = cfbstarttab[(x)&PIM]; \
    endmask = cfbendtab[((x)+(w)) & PIM];

/* FIXME */
#define maskbits24(x, w, startmask, endmask, nlw) \
    abort()
#define getbits24(psrc, dst, index) \
    abort()
#define putbits24(src, w, pdst, planemask, index) \
    abort()
#define putbitsrop24(src, x, pdst, planemask, rop) \
    abort()

#endif /* PSZ == 24 && PPW == 1 */

#define getbits(psrc, x, w, dst) \
if ( ((x) + (w)) <= PPW) \
{ \
    dst = SCRLEFT(*(psrc), (x)); \
} \
else \
{ \
    int m; \
    m = PPW-(x); \
    dst = (SCRLEFT(*(psrc), (x)) & cfbendtab[m]) | \
	  (SCRRIGHT(*((psrc)+1), m) & cfbstarttab[m]); \
}


#define putbits(src, x, w, pdst, planemask) \
if ( ((x)+(w)) <= PPW) \
{ \
    PixelGroup tmpmask; \
    maskpartialbits((x), (w), tmpmask); \
    tmpmask &= PFILL(planemask); \
    *(pdst) = (*(pdst) & ~tmpmask) | (SCRRIGHT(src, x) & tmpmask); \
} \
else \
{ \
    unsigned int m; \
    unsigned int n; \
    PixelGroup pm = PFILL(planemask); \
    m = PPW-(x); \
    n = (w) - m; \
    *(pdst) = (*(pdst) & (cfbendtab[x] | ~pm)) | \
	(SCRRIGHT(src, x) & (cfbstarttab[x] & pm)); \
    *((pdst)+1) = (*((pdst)+1) & (cfbstarttab[n] | ~pm)) | \
	(SCRLEFT(src, m) & (cfbendtab[n] & pm)); \
}
#if defined(__GNUC__) && defined(mc68020)
#undef getbits
#define FASTGETBITS(psrc, x, w, dst) \
    asm ("bfextu %3{%1:%2},%0" \
	 : "=d" (dst) : "di" (x), "di" (w), "o" (*(char *)(psrc)))

#define getbits(psrc,x,w,dst) \
{ \
    FASTGETBITS(psrc, (x) * PSZ, (w) * PSZ, dst); \
    dst = SCRLEFT(dst,PPW-(w)); \
}

#define FASTPUTBITS(src, x, w, pdst) \
    asm ("bfins %3,%0{%1:%2}" \
	 : "=o" (*(char *)(pdst)) \
	 : "di" (x), "di" (w), "d" (src), "0" (*(char *) (pdst)))

#undef putbits
#define putbits(src, x, w, pdst, planemask) \
{ \
    if (planemask != PMSK) { \
        PixelGroup _m, _pm; \
        FASTGETBITS(pdst, (x) * PSZ , (w) * PSZ, _m); \
        PFILL2(planemask, _pm); \
        _m &= (~_pm); \
        _m |= (SCRRIGHT(src, PPW-(w)) & _pm); \
        FASTPUTBITS(_m, (x) * PSZ, (w) * PSZ, pdst); \
    } else { \
        FASTPUTBITS(SCRRIGHT(src, PPW-(w)), (x) * PSZ, (w) * PSZ, pdst); \
    } \
}
    

#endif /* mc68020 */

#define putbitsrop(src, x, w, pdst, planemask, rop) \
if ( ((x)+(w)) <= PPW) \
{ \
    PixelGroup tmpmask; \
    PixelGroup t1, t2; \
    maskpartialbits((x), (w), tmpmask); \
    PFILL2(planemask, t1); \
    tmpmask &= t1; \
    t1 = SCRRIGHT((src), (x)); \
    DoRop(t2, rop, t1, *(pdst)); \
    *(pdst) = (*(pdst) & ~tmpmask) | (t2 & tmpmask); \
} \
else \
{ \
    CfbBits m; \
    CfbBits n; \
    PixelGroup t1, t2; \
    PixelGroup pm; \
    PFILL2(planemask, pm); \
    m = PPW-(x); \
    n = (w) - m; \
    t1 = SCRRIGHT((src), (x)); \
    DoRop(t2, rop, t1, *(pdst)); \
    *(pdst) = (*(pdst) & (cfbendtab[x] | ~pm)) | (t2 & (cfbstarttab[x] & pm));\
    t1 = SCRLEFT((src), m); \
    DoRop(t2, rop, t1, *((pdst) + 1)); \
    *((pdst)+1) = (*((pdst)+1) & (cfbstarttab[n] | ~pm)) | \
	(t2 & (cfbendtab[n] & pm)); \
}

#else /* PSZ == 32 && PPW == 1*/

/*
 * These macros can be optimized for 32-bit pixels since there is no
 * need to worry about left/right edge masking.  These macros were
 * derived from the above using the following reductions:
 *
 *	- x & PIW = 0 	[since PIW = 0]
 *	- all masking tables are only indexed by 0  [ due to above ]
 *	- cfbstartab[0] and cfbendtab[0] = 0 	[ no left/right edge masks]
 *    - cfbstartpartial[0] and cfbendpartial[0] = ~0 [no partial pixel mask]
 *
 * Macro reduction based upon constants cannot be performed automatically
 *       by the compiler since it does not know the contents of the masking
 *       arrays in cfbmskbits.c.
 */
#define maskbits(x, w, startmask, endmask, nlw) \
    startmask = endmask = 0; \
    nlw = (w);

#define maskpartialbits(x, w, mask) \
    mask = 0xFFFFFFFF;

#define mask32bits(x, w, startmask, endmask) \
    startmask = endmask = 0;

/*
 * For 32-bit operations, getbits(), putbits(), and putbitsrop() 
 * will only be invoked with x = 0 and w = PPW (1).  The getbits() 
 * macro is only called within left/right edge logic, which doesn't
 * happen for 32-bit pixels.
 */
#define getbits(psrc, x, w, dst) (dst) = *(psrc)

#define putbits(src, x, w, pdst, planemask) \
    *(pdst) = (*(pdst) & ~planemask) | (src & planemask);

#define putbitsrop(src, x, w, pdst, planemask, rop) \
{ \
    PixelGroup t1; \
    DoRop(t1, rop, (src), *(pdst)); \
    *(pdst) = (*(pdst) & ~planemask) | (t1 & planemask); \
}

#endif /* PSZ != 32 */

/*
 * Use these macros only when you're using the MergeRop stuff
 * in ../mfb/mergerop.h
 */

/* useful only when not spanning destination longwords */
#if PSZ == 24
#define putbitsmropshort24(src,x,w,pdst,index) {\
    PixelGroup   _tmpmask; \
    PixelGroup   _t1; \
    maskpartialbits ((x), (w), _tmpmask); \
    _t1 = SCRRIGHT((src), (x)); \
    DoMaskMergeRop24(_t1, pdst, _tmpmask, index); \
}
#endif
#define putbitsmropshort(src,x,w,pdst) {\
    PixelGroup   _tmpmask; \
    PixelGroup   _t1; \
    maskpartialbits ((x), (w), _tmpmask); \
    _t1 = SCRRIGHT((src), (x)); \
    *pdst = DoMaskMergeRop(_t1, *pdst, _tmpmask); \
}

/* useful only when spanning destination longwords */
#define putbitsmroplong(src,x,w,pdst) { \
    PixelGroup   _startmask, _endmask; \
    int		    _m; \
    PixelGroup   _t1; \
    _m = PPW - (x); \
    _startmask = cfbstarttab[x]; \
    _endmask = cfbendtab[(w) - _m]; \
    _t1 = SCRRIGHT((src), (x)); \
    pdst[0] = DoMaskMergeRop(_t1,pdst[0],_startmask); \
    _t1 = SCRLEFT ((src),_m); \
    pdst[1] = DoMaskMergeRop(_t1,pdst[1],_endmask); \
}

#define putbitsmrop(src,x,w,pdst) \
if ((x) + (w) <= PPW) {\
    putbitsmropshort(src,x,w,pdst); \
} else { \
    putbitsmroplong(src,x,w,pdst); \
}

#if GETLEFTBITS_ALIGNMENT == 1
#define getleftbits(psrc, w, dst)	dst = *((unsigned int *) psrc)
#define getleftbits24(psrc, w, dst, idx){	\
	regiseter int index; \
	switch(index = ((idx)&3)<<1){ \
	case 0: \
	dst = (*((unsigned int *) psrc))&cfbmask[index]; \
	break; \
	case 2: \
	case 4: \
	dst = BitLeft(((*((unsigned int *) psrc))&cfbmask[index]), cfb24Shift[index]); \
	dst |= BitRight(((*((unsigned int *) psrc)+1)&cfbmask[index]), cfb4Shift[index]); \
	break; \
	case 6: \
	dst = BitLeft((*((unsigned int *) psrc)),cfb24Shift[index]); \
	break; \
	}; \
}
#endif /* GETLEFTBITS_ALIGNMENT == 1 */

#define getglyphbits(psrc, x, w, dst) \
{ \
    dst = BitLeft((unsigned) *(psrc), (x)); \
    if ( ((x) + (w)) > 32) \
	dst |= (BitRight((unsigned) *((psrc)+1), 32-(x))); \
}
#if GETLEFTBITS_ALIGNMENT == 2
#define getleftbits(psrc, w, dst) \
    { \
	if ( ((int)(psrc)) & 0x01 ) \
		getglyphbits( ((unsigned int *)(((char *)(psrc))-1)), 8, (w), (dst) ); \
	else \
		dst = *((unsigned int *) psrc); \
    }
#endif /* GETLEFTBITS_ALIGNMENT == 2 */

#if GETLEFTBITS_ALIGNMENT == 4
#define getleftbits(psrc, w, dst) \
    { \
	int off, off_b; \
	off_b = (off = ( ((int)(psrc)) & 0x03)) << 3; \
	getglyphbits( \
		(unsigned int *)( ((char *)(psrc)) - off), \
		(off_b), (w), (dst) \
	       ); \
    }
#endif /* GETLEFTBITS_ALIGNMENT == 4 */

/*
 * getstipplepixels( psrcstip, x, w, ones, psrcpix, destpix )
 *
 * Converts bits to pixels in a reasonable way.  Takes w (1 <= w <= PPW)
 * bits from *psrcstip, starting at bit x; call this a quartet of bits.
 * Then, takes the pixels from *psrcpix corresponding to the one-bits (if
 * ones is TRUE) or the zero-bits (if ones is FALSE) of the quartet
 * and puts these pixels into destpix.
 *
 * Example:
 *
 *      getstipplepixels( &(0x08192A3B), 17, 4, 1, &(0x4C5D6E7F), dest )
 *
 * 0x08192A3B = 0000 1000 0001 1001 0010 1010 0011 1011
 *
 * This will take 4 bits starting at bit 17, so the quartet is 0x5 = 0101.
 * It will take pixels from 0x4C5D6E7F corresponding to the one-bits in this
 * quartet, so dest = 0x005D007F.
 *
 * XXX Works with both byte order.
 * XXX This works for all values of x and w within a doubleword.
 */
#if (BITMAP_BIT_ORDER == MSBFirst)
#define getstipplepixels( psrcstip, x, w, ones, psrcpix, destpix ) \
{ \
    PixelGroup q; \
    int m; \
    if ((m = ((x) - ((PPW*PSZ)-PPW))) > 0) { \
        q = (*(psrcstip)) << m; \
	if ( (x)+(w) > (PPW*PSZ) ) \
	    q |= *((psrcstip)+1) >> ((PPW*PSZ)-m); \
    } \
    else \
        q = (*(psrcstip)) >> -m; \
    q = QuartetBitsTable[(w)] & ((ones) ? q : ~q); \
    *(destpix) = (*(psrcpix)) & QuartetPixelMaskTable[q]; \
}
/* I just copied this to get the linker satisfied on PowerPC,
 * so this may not be correct at all.
 */
#define getstipplepixels24(psrcstip,xt,ones,psrcpix,destpix,stipindex) \
{ \
    PixelGroup q; \
    q = *(psrcstip) >> (xt); \
    q = ((ones) ? q : ~q) & 1; \
    *(destpix) = (*(psrcpix)) & QuartetPixelMaskTable[q]; \
}
#else /* BITMAP_BIT_ORDER == LSB */

/* this must load 32 bits worth; for most machines, thats an int */
#define CfbFetchUnaligned(x)	ldl_u(x)

#define getstipplepixels( psrcstip, xt, w, ones, psrcpix, destpix ) \
{ \
    PixelGroup q; \
    q = CfbFetchUnaligned(psrcstip) >> (xt); \
    if ( ((xt)+(w)) > (PPW*PSZ) ) \
        q |= (CfbFetchUnaligned((psrcstip)+1)) << ((PPW*PSZ)-(xt)); \
    q = QuartetBitsTable[(w)] & ((ones) ? q : ~q); \
    *(destpix) = (*(psrcpix)) & QuartetPixelMaskTable[q]; \
}
#if PSZ == 24
#define getstipplepixels24(psrcstip,xt,ones,psrcpix,destpix,stipindex) \
{ \
    PixelGroup q; \
    q = *(psrcstip) >> (xt); \
    q = ((ones) ? q : ~q) & 1; \
    *(destpix) = (*(psrcpix)) & QuartetPixelMaskTable[q]; \
}
#endif /* PSZ == 24 */
#endif

extern PixelGroup cfbstarttab[];
extern PixelGroup cfbendtab[];
extern PixelGroup cfbstartpartial[];
extern PixelGroup cfbendpartial[];
extern PixelGroup cfbrmask[];
extern PixelGroup cfbmask[];
extern PixelGroup QuartetBitsTable[];
extern PixelGroup QuartetPixelMaskTable[];
#if PSZ == 24
extern int cfb24Shift[];
#endif
