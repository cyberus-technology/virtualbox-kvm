/* $XdotOrg: xserver/xorg/include/cursor.h,v 1.6 2005/08/24 11:18:30 daniels Exp $ */
/* $XFree86: xc/programs/Xserver/include/cursor.h,v 1.6 2002/09/17 01:15:14 dawes Exp $ */
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
/* $Xorg: cursor.h,v 1.4 2001/02/09 02:05:15 xorgcvs Exp $ */

#ifndef CURSOR_H
#define CURSOR_H 

#include "misc.h"
#include "screenint.h"
#include "window.h"

#define NullCursor ((CursorPtr)NULL)

/* Provide support for alpha composited cursors */
#ifdef RENDER
#define ARGB_CURSOR
#endif

typedef struct _Cursor *CursorPtr;
typedef struct _CursorMetric *CursorMetricPtr;

extern CursorPtr rootCursor;

extern int FreeCursor(
    pointer /*pCurs*/,
    XID /*cid*/);

/* Quartz support on Mac OS X pulls in the QuickDraw
   framework whose AllocCursor function conflicts here. */ 
#ifdef __DARWIN__
#define AllocCursor Darwin_X_AllocCursor
#endif
extern CursorPtr AllocCursor(
    unsigned char* /*psrcbits*/,
    unsigned char* /*pmaskbits*/,
    CursorMetricPtr /*cm*/,
    unsigned /*foreRed*/,
    unsigned /*foreGreen*/,
    unsigned /*foreBlue*/,
    unsigned /*backRed*/,
    unsigned /*backGreen*/,
    unsigned /*backBlue*/);

extern CursorPtr AllocCursorARGB(
    unsigned char* /*psrcbits*/,
    unsigned char* /*pmaskbits*/,
    CARD32* /*argb*/,
    CursorMetricPtr /*cm*/,
    unsigned /*foreRed*/,
    unsigned /*foreGreen*/,
    unsigned /*foreBlue*/,
    unsigned /*backRed*/,
    unsigned /*backGreen*/,
    unsigned /*backBlue*/);

extern int AllocGlyphCursor(
    Font /*source*/,
    unsigned int /*sourceChar*/,
    Font /*mask*/,
    unsigned int /*maskChar*/,
    unsigned /*foreRed*/,
    unsigned /*foreGreen*/,
    unsigned /*foreBlue*/,
    unsigned /*backRed*/,
    unsigned /*backGreen*/,
    unsigned /*backBlue*/,
    CursorPtr* /*ppCurs*/,
    ClientPtr /*client*/);

extern CursorPtr CreateRootCursor(
    char* /*pfilename*/,
    unsigned int /*glyph*/);

extern int ServerBitsFromGlyph(
    FontPtr /*pfont*/,
    unsigned int /*ch*/,
    register CursorMetricPtr /*cm*/,
    unsigned char ** /*ppbits*/);

extern Bool CursorMetricsFromGlyph(
    FontPtr /*pfont*/,
    unsigned /*ch*/,
    CursorMetricPtr /*cm*/);

extern void CheckCursorConfinement(
    WindowPtr /*pWin*/);

extern void NewCurrentScreen(
    ScreenPtr /*newScreen*/,
    int /*x*/,
    int /*y*/);

extern Bool PointerConfinedToScreen(void);

extern void GetSpritePosition(
    int * /*px*/,
    int * /*py*/);

#ifdef PANORAMIX
extern int XineramaGetCursorScreen(void);
#endif /* PANORAMIX */

#endif /* CURSOR_H */
