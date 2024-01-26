/*
 * mipointrst.h
 *
 */

/* $Xorg: mipointrst.h,v 1.4 2001/02/09 02:05:21 xorgcvs Exp $ */

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
/* $XFree86: xc/programs/Xserver/mi/mipointrst.h,v 1.3 2001/04/19 14:14:07 tsi Exp $ */

#include "mipointer.h"
#include "scrnintstr.h"

#define MOTION_SIZE	256

typedef struct {
    xTimecoord	    event;
    ScreenPtr	    pScreen;
} miHistoryRec, *miHistoryPtr;

typedef struct {
    ScreenPtr		    pScreen;    /* current screen */
    ScreenPtr		    pSpriteScreen;/* screen containing current sprite */
    CursorPtr		    pCursor;    /* current cursor */
    CursorPtr		    pSpriteCursor;/* cursor on screen */
    BoxRec		    limits;	/* current constraints */
    Bool		    confined;	/* pointer can't change screens */
    int			    x, y;	/* hot spot location */
    int			    devx, devy;	/* sprite position */
    DevicePtr		    pPointer;   /* pointer device structure */
    miHistoryRec	    history[MOTION_SIZE];
    int			    history_start, history_end;
} miPointerRec, *miPointerPtr;

typedef struct {
    miPointerSpriteFuncPtr  spriteFuncs;	/* sprite-specific methods */
    miPointerScreenFuncPtr  screenFuncs;	/* screen-specific methods */
    CloseScreenProcPtr	    CloseScreen;
    Bool		    waitForUpdate;	/* don't move cursor in SIGIO */
    Bool		    showTransparent;	/* show empty cursors */
} miPointerScreenRec, *miPointerScreenPtr;
