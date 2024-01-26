/* $XFree86: xc/programs/Xserver/mfb/maskbits.h,v 3.8tsi Exp $ */
/* Combined Purdue/PurduePlus patches, level 2.1, 1/24/89 */
/***********************************************************
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
/* $Xorg: maskbits.h,v 1.3 2000/08/17 19:53:34 cpqbld Exp $ */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xmd.h>
#include "servermd.h"


/* the following notes use the following conventions:
SCREEN LEFT				SCREEN RIGHT
in this file and maskbits.c, left and right refer to screen coordinates,
NOT bit numbering in registers.

starttab[n]
	bits[0,n-1] = 0	bits[n,PLST] = 1
endtab[n] =
	bits[0,n-1] = 1	bits[n,PLST] = 0

startpartial[], endpartial[]
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
	the contents of the rest of dst are 0.
	this is a right shift on LSBFirst (forward-thinking)
	machines like the VAX, and left shift on MSBFirst
	(backwards) machines like the 680x0 and pc/rt.

SCRRIGHT(dst, x)
	takes dst[0,x] and moves them to dst[PPW-x, PPW]
	the contents of the rest of dst are 0.
	this is a left shift on LSBFirst, right shift
	on MSBFirst.


the remaining macros are cpu-independent; all bit order dependencies
are built into the tables and the two macros above.

maskbits(x, w, startmask, endmask, nlw)
	for a span of width w starting at position x, returns
a mask for ragged bits at start, mask for ragged bits at end,
and the number of whole longwords between the ends.

maskpartialbits(x, w, mask)
	works like maskbits(), except all the bits are in the
	same longword (i.e. (x&PIM + w) <= PPW)

maskPPWbits(x, w, startmask, endmask, nlw)
	as maskbits, but does not calculate nlw.  it is used by
	mfbGlyphBlt to put down glyphs <= PPW bits wide.

-------------------------------------------------------------------

NOTE
	any pointers passed to the following 4 macros are
	guranteed to be PPW-bit aligned.
	The only non-PPW-bit-aligned references ever made are
	to font glyphs, and those are made with getleftbits()
	and getshiftedleftbits (qq.v.)

	For 64-bit server, it is assumed that we will never have font padding
	of more than 4 bytes. The code uses int's to access the fonts
	intead of longs.

getbits(psrc, x, w, dst)
	starting at position x in psrc (x < PPW), collect w
	bits and put them in the screen left portion of dst.
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
	get m bits, move to screen-left of dst, zeroing rest of dst;
	get n bits from next word, move screen-right by m, zeroing
		 lower m bits of word.
	OR the two things together.

putbits(src, x, w, pdst)
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
	get m bits, shift screen-right by x, zero screen-leftmost x
		bits; zero rightmost m bits of *pdst and OR in stuff
		from before the semicolon.
	shift src screen-left by m, zero bits n-PPW;
		zero leftmost n bits of *(pdst+1) and OR in the
		stuff from before the semicolon.

putbitsrop(src, x, w, pdst, ROP)
	like putbits but calls DoRop with the rasterop ROP (see mfb.h for
	DoRop)

putbitsrrop(src, x, w, pdst, ROP)
	like putbits but calls DoRRop with the reduced rasterop ROP
	(see mfb.h for DoRRop)

-----------------------------------------------------------------------
	The two macros below are used only for getting bits from glyphs
in fonts, and glyphs in fonts are gotten only with the following two
mcros.
	You should tune these macros toyour font format and cpu
byte ordering.

NOTE
getleftbits(psrc, w, dst)
	get the leftmost w (w<=32) bits from *psrc and put them
	in dst.  this is used by the mfbGlyphBlt code for glyphs
	<=PPW bits wide.
	psrc is declared (unsigned char *)

	psrc is NOT guaranteed to be PPW-bit aligned.  on  many
	machines this will cause problems, so there are several
	versions of this macro.

	this macro is called ONLY for getting bits from font glyphs,
	and depends on the server-natural font padding.

	for blazing text performance, you want this macro
	to touch memory as infrequently as possible (e.g.
	fetch longwords) and as efficiently as possible
	(e.g. don't fetch misaligned longwords)

getshiftedleftbits(psrc, offset, w, dst)
	used by the font code; like getleftbits, but shifts the
	bits SCRLEFT by offset.
	this is implemented portably, calling getleftbits()
	and SCRLEFT().
	psrc is declared (unsigned char *).
*/

/* to match CFB and allow algorithm sharing ...
 * name	   mfb32  mfb64  explanation
 * ----	   ------ -----  -----------
 * PGSZ    32      64    pixel group size (in bits; same as PPW for mfb)
 * PGSZB    4      8     pixel group size (in bytes)
 * PPW	   32     64     pixels per word (pixels per pixel group)
 * PLST	   31     63     index of last pixel in a word (should be PPW-1)
 * PIM	   0x1f   0x3f   pixel index mask (index within a pixel group)
 * PWSH	   5       6     pixel-to-word shift (should be log2(PPW))
 *
 * The MFB_ versions are here so that cfb can include maskbits.h to get
 * the bitmap constants without conflicting with its own P* constants.
 * 
 * Keith Packard (keithp@suse.com):
 * Note mfb64 is no longer supported; it requires DIX support
 * for realigning images which costs too much
 */	    

/* warning: PixelType definition duplicated in mfb.h */
#ifndef PixelType
#define PixelType CARD32
#endif /* PixelType */
#ifndef MfbBits
#define MfbBits CARD32
#endif

#define MFB_PGSZB 4
#define MFB_PPW		(MFB_PGSZB<<3) /* assuming 8 bits per byte */
#define MFB_PGSZ	MFB_PPW
#define MFB_PLST	(MFB_PPW-1)
#define MFB_PIM		MFB_PLST

/* set PWSH = log2(PPW) using brute force */

#if MFB_PPW == 32
#define MFB_PWSH 5
#endif /* MFB_PPW == 32 */

/* XXX don't use these five */
extern PixelType starttab[];
extern PixelType endtab[];
extern PixelType partmasks[MFB_PPW][MFB_PPW];
extern PixelType rmask[];
extern PixelType mask[];
/* XXX use these five */
extern PixelType mfbGetstarttab(int);
extern PixelType mfbGetendtab(int);
extern PixelType mfbGetpartmasks(int, int);
extern PixelType mfbGetrmask(int);
extern PixelType mfbGetmask(int);

#ifndef MFB_CONSTS_ONLY

#define PGSZB	MFB_PGSZB
#define PPW	MFB_PPW
#define PGSZ	MFB_PGSZ
#define PLST	MFB_PLST
#define PIM	MFB_PIM
#define PWSH	MFB_PWSH

#define BitLeft(b,s)	SCRLEFT(b,s)
#define BitRight(b,s)	SCRRIGHT(b,s)

#ifdef XFree86Server
#define LONG2CHARSSAMEORDER(x) ((MfbBits)(x))
#define LONG2CHARSDIFFORDER( x ) ( ( ( ( x ) & (MfbBits)0x000000FF ) << 0x18 ) \
                        | ( ( ( x ) & (MfbBits)0x0000FF00 ) << 0x08 ) \
                        | ( ( ( x ) & (MfbBits)0x00FF0000 ) >> 0x08 ) \
                        | ( ( ( x ) & (MfbBits)0xFF000000 ) >> 0x18 ) )
#endif /* XFree86Server */

#if (BITMAP_BIT_ORDER == IMAGE_BYTE_ORDER)
#define LONG2CHARS(x) ((MfbBits)(x))
#else
/*
 *  the unsigned case below is for compilers like
 *  the Danbury C and i386cc
 */
#define LONG2CHARS( x ) ( ( ( ( x ) & (MfbBits)0x000000FF ) << 0x18 ) \
                        | ( ( ( x ) & (MfbBits)0x0000FF00 ) << 0x08 ) \
                        | ( ( ( x ) & (MfbBits)0x00FF0000 ) >> 0x08 ) \
                        | ( ( ( x ) & (MfbBits)0xFF000000 ) >> 0x18 ) )
#endif /* BITMAP_BIT_ORDER */

#ifdef STRICT_ANSI_SHIFT
#define SHL(x,y)    ((y) >= PPW ? 0 : LONG2CHARS(LONG2CHARS(x) << (y)))
#define SHR(x,y)    ((y) >= PPW ? 0 : LONG2CHARS(LONG2CHARS(x) >> (y)))
#else
#define SHL(x,y)    LONG2CHARS(LONG2CHARS(x) << (y))
#define SHR(x,y)    LONG2CHARS(LONG2CHARS(x) >> (y))
#endif

#if (BITMAP_BIT_ORDER == MSBFirst)	/* pc/rt, 680x0 */
#define SCRLEFT(lw, n)	SHL((PixelType)(lw),(n))
#define SCRRIGHT(lw, n)	SHR((PixelType)(lw),(n))
#else					/* vax, intel */
#define SCRLEFT(lw, n)	SHR((PixelType)(lw),(n))
#define SCRRIGHT(lw, n)	SHL((PixelType)(lw),(n))
#endif

#define DoRRop(alu, src, dst) \
(((alu) == RROP_BLACK) ? ((dst) & ~(src)) : \
 ((alu) == RROP_WHITE) ? ((dst) | (src)) : \
 ((alu) == RROP_INVERT) ? ((dst) ^ (src)) : \
  (dst))

/* A generalized form of a x4 Duff's Device */
#define Duff(counter, block) { \
  while (counter >= 4) {\
     { block; } \
     { block; } \
     { block; } \
     { block; } \
     counter -= 4; \
  } \
     switch (counter & 3) { \
     case 3:	{ block; } \
     case 2:	{ block; } \
     case 1:	{ block; } \
     case 0: \
     counter = 0; \
   } \
}

#define maskbits(x, w, startmask, endmask, nlw) \
    startmask = mfbGetstarttab((x) & PIM); \
    endmask = mfbGetendtab(((x)+(w)) & PIM); \
    if (startmask) \
	nlw = (((w) - (PPW - ((x) & PIM))) >> PWSH); \
    else \
	nlw = (w) >> PWSH;

#define maskpartialbits(x, w, mask) \
    mask = mfbGetpartmasks((x) & PIM, (w) & PIM);

#define maskPPWbits(x, w, startmask, endmask) \
    startmask = mfbGetstarttab((x) & PIM); \
    endmask = mfbGetendtab(((x)+(w)) & PIM);

#ifdef __GNUC__ /* XXX don't want for Alpha? */
#ifdef vax
#define FASTGETBITS(psrc,x,w,dst) \
    __asm ("extzv %1,%2,%3,%0" \
	 : "=g" (dst) \
	 : "g" (x), "g" (w), "m" (*(char *)(psrc)))
#define getbits(psrc,x,w,dst) FASTGETBITS(psrc,x,w,dst)

#define FASTPUTBITS(src, x, w, pdst) \
    __asm ("insv %3,%1,%2,%0" \
	 : "=m" (*(char *)(pdst)) \
	 : "g" (x), "g" (w), "g" (src))
#define putbits(src, x, w, pdst) FASTPUTBITS(src, x, w, pdst)
#endif /* vax */
#ifdef mc68020
#define FASTGETBITS(psrc, x, w, dst) \
    __asm ("bfextu %3{%1:%2},%0" \
    : "=d" (dst) : "di" (x), "di" (w), "o" (*(char *)(psrc)))

#define getbits(psrc,x,w,dst) \
{ \
    FASTGETBITS(psrc, x, w, dst);\
    dst = SHL(dst,(32-(w))); \
}

#define FASTPUTBITS(src, x, w, pdst) \
    __asm ("bfins %3,%0{%1:%2}" \
	 : "=o" (*(char *)(pdst)) \
	 : "di" (x), "di" (w), "d" (src), "0" (*(char *) (pdst)))

#define putbits(src, x, w, pdst) FASTPUTBITS(SHR((src),32-(w)), x, w, pdst)

#endif /* mc68020 */
#endif /* __GNUC__ */

/*  The following flag is used to override a bugfix for sun 3/60+CG4 machines,
 */

/*  We don't need to be careful about this unless we're dealing with sun3's 
 *  We will default its usage for those who do not know anything, but will
 *  override its effect if the machine doesn't look like a sun3 
 */
#if !defined(mc68020) || !defined(sun)
#define NO_3_60_CG4
#endif

/* This is gross.  We want to #define u_putbits as something which can be used
 * in the case of the 3/60+CG4, but if we use /bin/cc or are on another
 * machine type, we want nothing to do with u_putbits.  What a hastle.  Here
 * I used slo_putbits as something which either u_putbits or putbits could be
 * defined as.
 *
 * putbits gets it iff it is not already defined with FASTPUTBITS above.
 * u_putbits gets it if we have FASTPUTBITS (putbits) from above and have not
 * 	overridden the NO_3_60_CG4 flag.
 */

#define slo_putbits(src, x, w, pdst) \
{ \
    register int n = (x)+(w)-PPW; \
    \
    if (n <= 0) \
    { \
	register PixelType tmpmask; \
	maskpartialbits((x), (w), tmpmask); \
	*(pdst) = (*(pdst) & ~tmpmask) | \
		(SCRRIGHT(src, x) & tmpmask); \
    } \
    else \
    { \
	register int d = PPW-(x); \
	*(pdst) = (*(pdst) & mfbGetendtab(x)) | (SCRRIGHT((src), x)); \
	(pdst)[1] = ((pdst)[1] & mfbGetstarttab(n)) | \
		(SCRLEFT(src, d) & mfbGetendtab(n)); \
    } \
}

#if defined(putbits) && !defined(NO_3_60_CG4)
#define u_putbits(src, x, w, pdst) slo_putbits(src, x, w, pdst)
#else
#define u_putbits(src, x, w, pdst) putbits(src, x, w, pdst)
#endif

#if !defined(putbits) 
#define putbits(src, x, w, pdst) slo_putbits(src, x, w, pdst)
#endif

/* Now if we have not gotten any really good bitfield macros, try some
 * moderately fast macros.  Alas, I don't know how to do asm instructions
 * without gcc.
 */

#ifndef getbits
#define getbits(psrc, x, w, dst) \
{ \
    dst = SCRLEFT(*(psrc), (x)); \
    if ( ((x) + (w)) > PPW) \
	dst |= (SCRRIGHT(*((psrc)+1), PPW-(x))); \
}
#endif

/*  We have to special-case putbitsrop because of 3/60+CG4 combos
 */

#define u_putbitsrop(src, x, w, pdst, rop) \
{\
	register PixelType t1, t2; \
	register int n = (x)+(w)-PPW; \
	\
	t1 = SCRRIGHT((src), (x)); \
	DoRop(t2, rop, t1, *(pdst)); \
	\
    if (n <= 0) \
    { \
	register PixelType tmpmask; \
	\
	maskpartialbits((x), (w), tmpmask); \
	*(pdst) = (*(pdst) & ~tmpmask) | (t2 & tmpmask); \
    } \
    else \
    { \
	int m = PPW-(x); \
	*(pdst) = (*(pdst) & mfbGetendtab(x)) | (t2 & mfbGetstarttab(x)); \
	t1 = SCRLEFT((src), m); \
	DoRop(t2, rop, t1, (pdst)[1]); \
	(pdst)[1] = ((pdst)[1] & mfbGetstarttab(n)) | (t2 & mfbGetendtab(n)); \
    } \
}

/* If our getbits and putbits are FAST enough,
 * do this brute force, it's faster
 */

#if defined(FASTPUTBITS) && defined(FASTGETBITS) && defined(NO_3_60_CG4)
#if (BITMAP_BIT_ORDER == MSBFirst)
#define putbitsrop(src, x, w, pdst, rop) \
{ \
  register PixelType _tmp, _tmp2; \
  FASTGETBITS(pdst, x, w, _tmp); \
  _tmp2 = SCRRIGHT(src, PPW-(w)); \
  DoRop(_tmp, rop, _tmp2, _tmp) \
  FASTPUTBITS(_tmp, x, w, pdst); \
}
#define putbitsrrop(src, x, w, pdst, rop) \
{ \
  register PixelType _tmp, _tmp2; \
 \
  FASTGETBITS(pdst, x, w, _tmp); \
  _tmp2 = SCRRIGHT(src, PPW-(w)); \
  _tmp= DoRRop(rop, _tmp2, _tmp); \
  FASTPUTBITS(_tmp, x, w, pdst); \
}
#undef u_putbitsrop
#else
#define putbitsrop(src, x, w, pdst, rop) \
{ \
  register PixelType _tmp; \
  FASTGETBITS(pdst, x, w, _tmp); \
  DoRop(_tmp, rop, src, _tmp) \
  FASTPUTBITS(_tmp, x, w, pdst); \
}
#define putbitsrrop(src, x, w, pdst, rop) \
{ \
  register PixelType _tmp; \
 \
  FASTGETBITS(pdst, x, w, _tmp); \
  _tmp= DoRRop(rop, src, _tmp); \
  FASTPUTBITS(_tmp, x, w, pdst); \
}
#undef u_putbitsrop
#endif
#endif

#ifndef putbitsrop
#define putbitsrop(src, x, w, pdst, rop)  u_putbitsrop(src, x, w, pdst, rop)
#endif 

#ifndef putbitsrrop
#define putbitsrrop(src, x, w, pdst, rop) \
{\
	register PixelType t1, t2; \
	register int n = (x)+(w)-PPW; \
	\
	t1 = SCRRIGHT((src), (x)); \
	t2 = DoRRop(rop, t1, *(pdst)); \
	\
    if (n <= 0) \
    { \
	register PixelType tmpmask; \
	\
	maskpartialbits((x), (w), tmpmask); \
	*(pdst) = (*(pdst) & ~tmpmask) | (t2 & tmpmask); \
    } \
    else \
    { \
	int m = PPW-(x); \
	*(pdst) = (*(pdst) & mfbGetendtab(x)) | (t2 & mfbGetstarttab(x)); \
	t1 = SCRLEFT((src), m); \
	t2 = DoRRop(rop, t1, (pdst)[1]); \
	(pdst)[1] = ((pdst)[1] & mfbGetstarttab(n)) | (t2 & mfbGetendtab(n)); \
    } \
}
#endif

#if GETLEFTBITS_ALIGNMENT == 1
#define getleftbits(psrc, w, dst)	dst = *((CARD32 *)(pointer) psrc)
#endif /* GETLEFTBITS_ALIGNMENT == 1 */

#if GETLEFTBITS_ALIGNMENT == 2
#define getleftbits(psrc, w, dst) \
    { \
	if ( ((int)(psrc)) & 0x01 ) \
		getbits( ((CARD32 *)(((char *)(psrc))-1)), 8, (w), (dst) ); \
	else \
		getbits(psrc, 0, w, dst); \
    }
#endif /* GETLEFTBITS_ALIGNMENT == 2 */

#if GETLEFTBITS_ALIGNMENT == 4
#define getleftbits(psrc, w, dst) \
    { \
	int off, off_b; \
	off_b = (off = ( ((int)(psrc)) & 0x03)) << 3; \
	getbits( \
		(CARD32 *)( ((char *)(psrc)) - off), \
		(off_b), (w), (dst) \
	       ); \
    }
#endif /* GETLEFTBITS_ALIGNMENT == 4 */


#define getshiftedleftbits(psrc, offset, w, dst) \
	getleftbits((psrc), (w), (dst)); \
	dst = SCRLEFT((dst), (offset));

/* FASTGETBITS and FASTPUTBITS are not necessarily correct implementations of
 * getbits and putbits, but they work if used together.
 *
 * On a MSBFirst machine, a cpu bitfield extract instruction (like bfextu)
 * could normally assign its result to a 32-bit word register in the screen
 * right position.  This saves canceling register shifts by not fighting the
 * natural cpu byte order.
 *
 * Unfortunately, these fail on a 3/60+CG4 and cannot be used unmodified. Sigh.
 */
#if defined(FASTGETBITS) && defined(FASTPUTBITS)
#ifdef NO_3_60_CG4
#define u_FASTPUT(aa, bb, cc, dd)  FASTPUTBITS(aa, bb, cc, dd)
#else
#define u_FASTPUT(aa, bb, cc, dd)  u_putbits(SCRLEFT(aa, PPW-(cc)), bb, cc, dd)
#endif

#define getandputbits(psrc, srcbit, dstbit, width, pdst) \
{ \
    register PixelType _tmpbits; \
    FASTGETBITS(psrc, srcbit, width, _tmpbits); \
    u_FASTPUT(_tmpbits, dstbit, width, pdst); \
}

#define getandputrop(psrc, srcbit, dstbit, width, pdst, rop) \
{ \
  register PixelType _tmpsrc, _tmpdst; \
  FASTGETBITS(pdst, dstbit, width, _tmpdst); \
  FASTGETBITS(psrc, srcbit, width, _tmpsrc); \
  DoRop(_tmpdst, rop, _tmpsrc, _tmpdst); \
  u_FASTPUT(_tmpdst, dstbit, width, pdst); \
}

#define getandputrrop(psrc, srcbit, dstbit, width, pdst, rop) \
{ \
  register PixelType _tmpsrc, _tmpdst; \
  FASTGETBITS(pdst, dstbit, width, _tmpdst); \
  FASTGETBITS(psrc, srcbit, width, _tmpsrc); \
  _tmpdst = DoRRop(rop, _tmpsrc, _tmpdst); \
  u_FASTPUT(_tmpdst, dstbit, width, pdst); \
}

#define getandputbits0(psrc, srcbit, width, pdst) \
	getandputbits(psrc, srcbit, 0, width, pdst)

#define getandputrop0(psrc, srcbit, width, pdst, rop) \
    	getandputrop(psrc, srcbit, 0, width, pdst, rop)

#define getandputrrop0(psrc, srcbit, width, pdst, rop) \
    	getandputrrop(psrc, srcbit, 0, width, pdst, rop)


#else /* Slow poke */

/* pairs of getbits/putbits happen frequently. Some of the code can
 * be shared or avoided in a few specific instances.  It gets us a
 * small advantage, so we do it.  The getandput...0 macros are the only ones
 * which speed things here.  The others are here for compatibility w/the above
 * FAST ones
 */

#define getandputbits(psrc, srcbit, dstbit, width, pdst) \
{ \
    register PixelType _tmpbits; \
    getbits(psrc, srcbit, width, _tmpbits); \
    putbits(_tmpbits, dstbit, width, pdst); \
}

#define getandputrop(psrc, srcbit, dstbit, width, pdst, rop) \
{ \
    register PixelType _tmpbits; \
    getbits(psrc, srcbit, width, _tmpbits) \
    putbitsrop(_tmpbits, dstbit, width, pdst, rop) \
}

#define getandputrrop(psrc, srcbit, dstbit, width, pdst, rop) \
{ \
    register PixelType _tmpbits; \
    getbits(psrc, srcbit, width, _tmpbits) \
    putbitsrrop(_tmpbits, dstbit, width, pdst, rop) \
}


#define getandputbits0(psrc, sbindex, width, pdst) \
{			/* unroll the whole damn thing to see how it * behaves */ \
    register int          _flag = PPW - (sbindex); \
    register PixelType _src; \
 \
    _src = SCRLEFT (*(psrc), (sbindex)); \
    if ((width) > _flag) \
	_src |=  SCRRIGHT (*((psrc) + 1), _flag); \
 \
    *(pdst) = (*(pdst) & mfbGetstarttab((width))) | (_src & mfbGetendtab((width))); \
}


#define getandputrop0(psrc, sbindex, width, pdst, rop) \
{			\
    register int          _flag = PPW - (sbindex); \
    register PixelType _src; \
 \
    _src = SCRLEFT (*(psrc), (sbindex)); \
    if ((width) > _flag) \
	_src |=  SCRRIGHT (*((psrc) + 1), _flag); \
    DoRop(_src, rop, _src, *(pdst)); \
 \
    *(pdst) = (*(pdst) & mfbGetstarttab((width))) | (_src & mfbGetendtab((width))); \
}

#define getandputrrop0(psrc, sbindex, width, pdst, rop) \
{ \
    int             _flag = PPW - (sbindex); \
    register PixelType _src; \
 \
    _src = SCRLEFT (*(psrc), (sbindex)); \
    if ((width) > _flag) \
	_src |=  SCRRIGHT (*((psrc) + 1), _flag); \
    _src = DoRRop(rop, _src, *(pdst)); \
 \
    *(pdst) = (*(pdst) & mfbGetstarttab((width))) | (_src & mfbGetendtab((width))); \
}

#endif  /* FASTGETBITS && FASTPUTBITS */

#endif /* MFB_CONSTS_ONLY */
