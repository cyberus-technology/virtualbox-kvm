/* $XFree86: xc/programs/Xserver/include/misc.h,v 3.28 2001/12/14 19:59:55 dawes Exp $ */
/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


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

Copyright 1992, 1993 Data General Corporation;
Copyright 1992, 1993 OMRON Corporation  

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that the
above copyright notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting documentation, and that
neither the name OMRON or DATA GENERAL be used in advertising or publicity
pertaining to distribution of the software without specific, written prior
permission of the party whose name is to be used.  Neither OMRON or 
DATA GENERAL make any representation about the suitability of this software
for any purpose.  It is provided "as is" without express or implied warranty.  

OMRON AND DATA GENERAL EACH DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL OMRON OR DATA GENERAL BE LIABLE FOR ANY SPECIAL, INDIRECT
OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
OF THIS SOFTWARE.

******************************************************************/
/* $Xorg: misc.h,v 1.5 2001/02/09 02:05:15 xorgcvs Exp $ */
#ifndef MISC_H
#define MISC_H 1
/*
 *  X internal definitions 
 *
 */

extern unsigned long globalSerialNumber;
extern unsigned long serverGeneration;

#include <X11/Xosdefs.h>
#include <X11/Xfuncproto.h>
#include <X11/Xmd.h>
#include <X11/X.h>
#include <X11/Xdefs.h>

#ifndef IN_MODULE
#ifndef NULL
#include <stddef.h>
#endif
#endif

#ifndef MAXSCREENS
#define MAXSCREENS	16
#endif
#define MAXCLIENTS	256
#define MAXDITS		1
#define MAXEXTENSIONS	128
#define MAXFORMATS	8
#define MAXVISUALS_PER_SCREEN 50

typedef unsigned long PIXEL;
typedef unsigned long ATOM;


#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifndef _XTYPEDEF_CALLBACKLISTPTR
typedef struct _CallbackList *CallbackListPtr; /* also in dix.h */
#define _XTYPEDEF_CALLBACKLISTPTR
#endif

typedef struct _xReq *xReqPtr;

#include "os.h" 	/* for ALLOCATE_LOCAL and DEALLOCATE_LOCAL */
#ifndef IN_MODULE
#include <X11/Xfuncs.h> /* for bcopy, bzero, and bcmp */
#endif

#define NullBox ((BoxPtr)0)
#define MILLI_PER_MIN (1000 * 60)
#define MILLI_PER_SECOND (1000)

    /* this next is used with None and ParentRelative to tell
       PaintWin() what to use to paint the background. Also used
       in the macro IS_VALID_PIXMAP */

#define USE_BACKGROUND_PIXEL 3
#define USE_BORDER_PIXEL 3


/* byte swap a 32-bit literal */
#define lswapl(x) ((((x) & 0xff) << 24) |\
		   (((x) & 0xff00) << 8) |\
		   (((x) & 0xff0000) >> 8) |\
		   (((x) >> 24) & 0xff))

/* byte swap a short literal */
#define lswaps(x) ((((x) & 0xff) << 8) | (((x) >> 8) & 0xff))

#undef min
#undef max

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#ifndef IN_MODULE
/* abs() is a function, not a macro; include the file declaring
 * it in case we haven't done that yet.
 */  
#include <stdlib.h>
#endif /* IN_MODULE */
#ifndef Fabs
#define Fabs(a) ((a) > 0.0 ? (a) : -(a))	/* floating absolute value */
#endif
#define sign(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))
/* this assumes b > 0 */
#define modulus(a, b, d)    if (((d) = (a) % (b)) < 0) (d) += (b)
/*
 * return the least significant bit in x which is set
 *
 * This works on 1's complement and 2's complement machines.
 * If you care about the extra instruction on 2's complement
 * machines, change to ((x) & (-(x)))
 */
#define lowbit(x) ((x) & (~(x) + 1))

#ifndef IN_MODULE
/* XXX Not for modules */
#include <limits.h>
#if !defined(MAXSHORT) || !defined(MINSHORT) || \
    !defined(MAXINT) || !defined(MININT)
/*
 * Some implementations #define these through <math.h>, so preclude
 * #include'ing it later.
 */

#include <math.h>
#endif
#undef MAXSHORT
#define MAXSHORT SHRT_MAX
#undef MINSHORT
#define MINSHORT SHRT_MIN
#undef MAXINT
#define MAXINT INT_MAX
#undef MININT
#define MININT INT_MIN

#include <assert.h>
#include <ctype.h>
#include <stdio.h>	/* for fopen, etc... */

#endif

/* some macros to help swap requests, replies, and events */

#define LengthRestB(stuff) \
    ((client->req_len << 2) - sizeof(*stuff))

#define LengthRestS(stuff) \
    ((client->req_len << 1) - (sizeof(*stuff) >> 1))

#define LengthRestL(stuff) \
    (client->req_len - (sizeof(*stuff) >> 2))

#define SwapRestS(stuff) \
    SwapShorts((short *)(stuff + 1), LengthRestS(stuff))

#define SwapRestL(stuff) \
    SwapLongs((CARD32 *)(stuff + 1), LengthRestL(stuff))

/* byte swap a 32-bit value */
#define swapl(x, n) { \
		 n = ((char *) (x))[0];\
		 ((char *) (x))[0] = ((char *) (x))[3];\
		 ((char *) (x))[3] = n;\
		 n = ((char *) (x))[1];\
		 ((char *) (x))[1] = ((char *) (x))[2];\
		 ((char *) (x))[2] = n; }

/* byte swap a short */
#define swaps(x, n) { \
		 n = ((char *) (x))[0];\
		 ((char *) (x))[0] = ((char *) (x))[1];\
		 ((char *) (x))[1] = n; }

/* copy 32-bit value from src to dst byteswapping on the way */
#define cpswapl(src, dst) { \
                 ((char *)&(dst))[0] = ((char *) &(src))[3];\
                 ((char *)&(dst))[1] = ((char *) &(src))[2];\
                 ((char *)&(dst))[2] = ((char *) &(src))[1];\
                 ((char *)&(dst))[3] = ((char *) &(src))[0]; }

/* copy short from src to dst byteswapping on the way */
#define cpswaps(src, dst) { \
		 ((char *) &(dst))[0] = ((char *) &(src))[1];\
		 ((char *) &(dst))[1] = ((char *) &(src))[0]; }

extern void SwapLongs(
#if NeedFunctionPrototypes
    CARD32 *list,
    unsigned long count
#endif
);

extern void SwapShorts(
#if NeedFunctionPrototypes
    short *list,
    unsigned long count
#endif
);

extern void MakePredeclaredAtoms(
#if NeedFunctionPrototypes
    void
#endif
);

extern int Ones(
#if NeedFunctionPrototypes
    unsigned long /*mask*/
#endif
);

typedef struct _xPoint *DDXPointPtr;
typedef struct _Box *BoxPtr;
typedef struct _xEvent *xEventPtr;
typedef struct _xRectangle *xRectanglePtr;
typedef struct _GrabRec *GrabPtr;

/*  typedefs from other places - duplicated here to minimize the amount
 *  of unnecessary junk that one would normally have to include to get
 *  these symbols defined
 */

#ifndef _XTYPEDEF_CHARINFOPTR
typedef struct _CharInfo *CharInfoPtr; /* also in fonts/include/font.h */
#define _XTYPEDEF_CHARINFOPTR
#endif

#endif /* MISC_H */
