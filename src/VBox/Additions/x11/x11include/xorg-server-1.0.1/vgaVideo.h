/* $XFree86: xc/programs/Xserver/hw/xfree86/xf4bpp/vgaVideo.h,v 1.1.2.1 1998/06/27 14:48:54 dawes Exp $ */
/*
 * Copyright IBM Corporation 1987,1988,1989
 *
 * All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that 
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of IBM not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
*/

/* $XConsortium: vgaVideo.h /main/4 1996/02/21 17:59:14 kaleb $ */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "misc.h"	/* GJA -- for pointer data type */
#ifdef lint
#if defined(volatile)
#undef volatile
#endif
#define volatile /**/
#if defined(const)
#undef const
#endif
#define const /**/
#if defined(signed)
#undef signed
#endif
#define signed /**/
#endif

/*
 * References to all pc ( i.e. '286 ) memory in the
 * regions used by the [ev]ga server ( the 128K windows )
 * MUST be long-word ( i.e. 32-bit ) reads or writes.
 * This definition will change for other memory architectures
 * ( e.g. AIX-Rt )
 */
typedef unsigned char VideoAdapterObject ;
typedef volatile VideoAdapterObject *VideoMemoryPtr ;
typedef volatile VideoAdapterObject *VgaMemoryPtr ;
#if !defined(BITMAP_BIT_ORDER)
#define BITMAP_BIT_ORDER MSBFirst
#endif

#if !defined(IMAGE_BYTE_ORDER)
#define IMAGE_BYTE_ORDER LSBFirst
#endif

/* Bit Ordering Macros */
#if !defined(SCRLEFT8)
#define SCRLEFT8(lw, n)	( (unsigned char) (((unsigned char) lw) << (n)) )
#endif
#if !defined(SCRRIGHT8)
#define SCRRIGHT8(lw, n)	( (unsigned char) (((unsigned char)lw) >> (n)) )
#endif
/* These work ONLY on 8-bit wide Quantities !! */
#define LeftmostBit ( SCRLEFT8( 0xFF, 7 ) & 0xFF )
#define RightmostBit ( SCRRIGHT8( 0xFF, 7 ) & 0xFF )

/*
 * [ev]ga video screen defines & macros
 */
#define VGA_BLACK_PIXEL 0
#define VGA_WHITE_PIXEL 1

#define VGA_MAXPLANES 4
#define VGA_ALLPLANES 0xFL

#define VIDBASE(pDraw) ((volatile unsigned char *) \
	(((PixmapPtr)(((DrawablePtr)(pDraw))->pScreen->devPrivate))-> \
		devPrivate.ptr))
#define BYTES_PER_LINE(pDraw) \
   ((int)((PixmapPtr)(((DrawablePtr)(pDraw))->pScreen->devPrivate))->devKind)

#define ROW_OFFSET( x ) ( ( x ) >> 3 )
#define BIT_OFFSET( x ) ( ( x ) & 0x7 )
#define SCREENADDRESS( pWin, x, y ) \
	( VIDBASE(pWin) + (y) * BYTES_PER_LINE(pWin) + ROW_OFFSET(x) )


