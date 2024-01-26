/* $Xorg: screenint.h,v 1.4 2001/02/09 02:05:15 xorgcvs Exp $ */
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

******************************************************************/
/* $XFree86: xc/programs/Xserver/include/screenint.h,v 1.5 2001/12/14 19:59:56 dawes Exp $ */
#ifndef SCREENINT_H
#define SCREENINT_H

#include "misc.h"

typedef struct _PixmapFormat *PixmapFormatPtr;
typedef struct _Visual *VisualPtr;
typedef struct _Depth  *DepthPtr;
typedef struct _Screen *ScreenPtr;

extern void ResetScreenPrivates(
#if NeedFunctionPrototypes
    void
#endif
);

extern int AllocateScreenPrivateIndex(
#if NeedFunctionPrototypes
    void
#endif
);

extern void ResetWindowPrivates(
#if NeedFunctionPrototypes
    void
#endif
);

extern int AllocateWindowPrivateIndex(
#if NeedFunctionPrototypes
    void
#endif
);

extern Bool AllocateWindowPrivate(
#if NeedFunctionPrototypes
    ScreenPtr /* pScreen */,
    int /* index */,
    unsigned /* amount */
#endif
);

extern void ResetGCPrivates(
#if NeedFunctionPrototypes
    void
#endif
);

extern int AllocateGCPrivateIndex(
#if NeedFunctionPrototypes
    void
#endif
);

extern Bool AllocateGCPrivate(
#if NeedFunctionPrototypes
    ScreenPtr /* pScreen */,
    int /* index */,
    unsigned /* amount */
#endif
);

extern int AddScreen(
#if NeedFunctionPrototypes
    Bool (* /*pfnInit*/)(
#if NeedNestedPrototypes
	int /*index*/,
	ScreenPtr /*pScreen*/,
	int /*argc*/,
	char ** /*argv*/
#endif
    ),
    int /*argc*/,
    char** /*argv*/
#endif
);

#ifdef PIXPRIV

extern void ResetPixmapPrivates(
#if NeedFunctionPrototypes
    void
#endif
);

extern int AllocatePixmapPrivateIndex(
#if NeedFunctionPrototypes
    void
#endif
);

extern Bool AllocatePixmapPrivate(
#if NeedFunctionPrototypes
    ScreenPtr /* pScreen */,
    int /* index */,
    unsigned /* amount */
#endif
);

#endif /* PIXPRIV */

extern void ResetColormapPrivates(
#if NeedFunctionPrototypes
    void
#endif
);


typedef struct _ColormapRec *ColormapPtr;
typedef int (*InitCmapPrivFunc)(
#if NeedNestedPrototypes
	ColormapPtr
#endif
);

extern int AllocateColormapPrivateIndex(
#if NeedFunctionPrototypes
    InitCmapPrivFunc /* initPrivFunc */
#endif
);

#endif /* SCREENINT_H */
