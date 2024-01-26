/* $XFree86: xc/programs/Xserver/include/colormap.h,v 1.5 2001/12/14 19:59:53 dawes Exp $ */
/*

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

*/
/* $Xorg: colormap.h,v 1.4 2001/02/09 02:05:14 xorgcvs Exp $ */

#ifndef CMAP_H
#define CMAP_H 1

#include "X11/Xproto.h"
#include "screenint.h"
#include "window.h"

/* these follow X.h's AllocNone and AllocAll */
#define CM_PSCREEN 2
#define CM_PWIN	   3
/* Passed internally in colormap.c */
#define REDMAP 0
#define GREENMAP 1
#define BLUEMAP 2
#define PSEUDOMAP 3
#define AllocPrivate (-1)
#define AllocTemporary (-2)
#define DynamicClass  1

/* Values for the flags field of a colormap. These should have 1 bit set
 * and not overlap */
#define IsDefault 1
#define AllAllocated 2
#define BeingCreated 4


typedef CARD32 Pixel;
typedef struct _CMEntry *EntryPtr;
/* moved to screenint.h: typedef struct _ColormapRec *ColormapPtr */
typedef struct _colorResource *colorResourcePtr;

extern int CreateColormap(
#if NeedFunctionPrototypes
    Colormap /*mid*/,
    ScreenPtr /*pScreen*/,
    VisualPtr /*pVisual*/,
    ColormapPtr* /*ppcmap*/,
    int /*alloc*/,
    int /*client*/
#endif
);

extern int FreeColormap(
#if NeedFunctionPrototypes
    pointer /*pmap*/,
    XID /*mid*/
#endif
);

extern int TellLostMap(
#if NeedFunctionPrototypes
    WindowPtr /*pwin*/,
    pointer /* Colormap *pmid */
#endif
);

extern int TellGainedMap(
#if NeedFunctionPrototypes
    WindowPtr /*pwin*/,
    pointer /* Colormap *pmid */
#endif
);

extern int CopyColormapAndFree(
#if NeedFunctionPrototypes
    Colormap /*mid*/,
    ColormapPtr /*pSrc*/,
    int /*client*/
#endif
);

extern int AllocColor(
#if NeedFunctionPrototypes
    ColormapPtr /*pmap*/,
    unsigned short* /*pred*/,
    unsigned short* /*pgreen*/,
    unsigned short* /*pblue*/,
    Pixel* /*pPix*/,
    int /*client*/
#endif
);

extern void FakeAllocColor(
#if NeedFunctionPrototypes
    ColormapPtr /*pmap*/,
    xColorItem * /*item*/
#endif
);

extern void FakeFreeColor(
#if NeedFunctionPrototypes
    ColormapPtr /*pmap*/,
    Pixel /*pixel*/
#endif
);

typedef int (*ColorCompareProcPtr)(
#if NeedNestedPrototypes
    EntryPtr /*pent*/,
    xrgb * /*prgb*/
#endif
);

extern int FindColor(
#if NeedFunctionPrototypes
    ColormapPtr /*pmap*/,
    EntryPtr /*pentFirst*/,
    int /*size*/,
    xrgb* /*prgb*/,
    Pixel* /*pPixel*/,
    int /*channel*/,
    int /*client*/,
    ColorCompareProcPtr /*comp*/
#endif
);

extern int QueryColors(
#if NeedFunctionPrototypes
    ColormapPtr /*pmap*/,
    int /*count*/,
    Pixel* /*ppixIn*/,
    xrgb* /*prgbList*/
#endif
);

extern int FreeClientPixels(
#if NeedFunctionPrototypes
    pointer /*pcr*/,
    XID /*fakeid*/
#endif
);

extern int AllocColorCells(
#if NeedFunctionPrototypes
    int /*client*/,
    ColormapPtr /*pmap*/,
    int /*colors*/,
    int /*planes*/,
    Bool /*contig*/,
    Pixel* /*ppix*/,
    Pixel* /*masks*/
#endif
);

extern int AllocColorPlanes(
#if NeedFunctionPrototypes
    int /*client*/,
    ColormapPtr /*pmap*/,
    int /*colors*/,
    int /*r*/,
    int /*g*/,
    int /*b*/,
    Bool /*contig*/,
    Pixel* /*pixels*/,
    Pixel* /*prmask*/,
    Pixel* /*pgmask*/,
    Pixel* /*pbmask*/
#endif
);

extern int FreeColors(
#if NeedFunctionPrototypes
    ColormapPtr /*pmap*/,
    int /*client*/,
    int /*count*/,
    Pixel* /*pixels*/,
    Pixel /*mask*/
#endif
);

extern int StoreColors(
#if NeedFunctionPrototypes
    ColormapPtr /*pmap*/,
    int /*count*/,
    xColorItem* /*defs*/
#endif
);

extern int IsMapInstalled(
#if NeedFunctionPrototypes
    Colormap /*map*/,
    WindowPtr /*pWin*/
#endif
);

#endif /* CMAP_H */
