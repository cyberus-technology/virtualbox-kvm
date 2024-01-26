/*
 * mipointer.h
 *
 */

/* $Xorg: mipointer.h,v 1.4 2001/02/09 02:05:21 xorgcvs Exp $ */

/*

Copyright 1989, 1998  The Open Group

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
*/
/* $XFree86: xc/programs/Xserver/mi/mipointer.h,v 3.9 2001/12/14 20:00:24 dawes Exp $ */

#ifndef MIPOINTER_H
#define MIPOINTER_H

#include "cursor.h"
#include "input.h"

typedef struct _miPointerSpriteFuncRec {
    Bool	(*RealizeCursor)(
                    ScreenPtr /* pScr */,
                    CursorPtr /* pCurs */
                    );
    Bool	(*UnrealizeCursor)(
                    ScreenPtr /* pScr */,
                    CursorPtr /* pCurs */
                    );
    void	(*SetCursor)(
                    ScreenPtr /* pScr */,
                    CursorPtr /* pCurs */,
                    int  /* x */,
                    int  /* y */
                    );
    void	(*MoveCursor)(
                    ScreenPtr /* pScr */,
                    int  /* x */,
                    int  /* y */
                    );
} miPointerSpriteFuncRec, *miPointerSpriteFuncPtr;

typedef struct _miPointerScreenFuncRec {
    Bool	(*CursorOffScreen)(
                    ScreenPtr* /* ppScr */,
                    int*  /* px */,
                    int*  /* py */
                    );
    void	(*CrossScreen)(
                    ScreenPtr /* pScr */,
                    int  /* entering */
                    );
    void	(*WarpCursor)(
                    ScreenPtr /* pScr */,
                    int  /* x */,
                    int  /* y */
                    );
    void	(*EnqueueEvent)(
                    xEventPtr /* event */
                    );
    void	(*NewEventScreen)(
                    ScreenPtr /* pScr */,
		    Bool /* fromDIX */
                    );
} miPointerScreenFuncRec, *miPointerScreenFuncPtr;

extern Bool miDCInitialize(
    ScreenPtr /*pScreen*/,
    miPointerScreenFuncPtr /*screenFuncs*/
);

extern Bool miPointerInitialize(
    ScreenPtr /*pScreen*/,
    miPointerSpriteFuncPtr /*spriteFuncs*/,
    miPointerScreenFuncPtr /*screenFuncs*/,
    Bool /*waitForUpdate*/
);

extern void miPointerWarpCursor(
    ScreenPtr /*pScreen*/,
    int /*x*/,
    int /*y*/
);

extern int miPointerGetMotionBufferSize(
    void
);

extern int miPointerGetMotionEvents(
    DeviceIntPtr /*pPtr*/,
    xTimecoord * /*coords*/,
    unsigned long /*start*/,
    unsigned long /*stop*/,
    ScreenPtr /*pScreen*/
);

extern void miPointerUpdate(
    void
);

extern void miPointerDeltaCursor(
    int /*dx*/,
    int /*dy*/,
    unsigned long /*time*/
);

extern void miPointerAbsoluteCursor(
    int /*x*/,
    int /*y*/,
    unsigned long /*time*/
);

extern void miPointerPosition(
    int * /*x*/,
    int * /*y*/
);

#undef miRegisterPointerDevice
extern void miRegisterPointerDevice(
    ScreenPtr /*pScreen*/,
    DevicePtr /*pDevice*/
);

extern void miPointerSetNewScreen(
    int, /*screen_no*/
	int, /*x*/
	int /*y*/
);
extern ScreenPtr miPointerCurrentScreen(
    void
);

#define miRegisterPointerDevice(pScreen,pDevice) \
       _miRegisterPointerDevice(pScreen,pDevice)

extern void _miRegisterPointerDevice(
    ScreenPtr /*pScreen*/,
    DeviceIntPtr /*pDevice*/
);

extern int miPointerScreenIndex;

#endif /* MIPOINTER_H */
