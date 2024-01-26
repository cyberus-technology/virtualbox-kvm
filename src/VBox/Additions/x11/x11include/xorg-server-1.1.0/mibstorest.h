/*
 * mibstorest.h
 *
 * internal structure definitions for mi backing store
 */

/* $Xorg: mibstorest.h,v 1.4 2001/02/09 02:05:20 xorgcvs Exp $ */

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

/* $XFree86: xc/programs/Xserver/mi/mibstorest.h,v 1.4 2001/01/17 22:37:06 dawes Exp $ */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "mibstore.h"
#include "regionstr.h"

/*
 * One of these structures is allocated per GC used with a backing-store
 * drawable.
 */

typedef struct {
    GCPtr	    pBackingGC;	    /* Copy of the GC but with graphicsExposures
				     * set FALSE and the clientClip set to
				     * clip output to the valid regions of the
				     * backing pixmap. */
    int		    guarantee;      /* GuaranteeNothing, etc. */
    unsigned long   serialNumber;   /* clientClip computed time */
    unsigned long   stateChanges;   /* changes in parent gc since last copy */
    GCOps	    *wrapOps;	    /* wrapped ops */
    GCFuncs	    *wrapFuncs;	    /* wrapped funcs */
} miBSGCRec, *miBSGCPtr;

/*
 * one of these structures is allocated per Window with backing store
 */

typedef struct {
    PixmapPtr	  pBackingPixmap;   /* Pixmap for saved areas */
    short	  x;		    /* origin of pixmap relative to window */
    short	  y;
    RegionRec	  SavedRegion;	    /* Valid area in pBackingPixmap */
    char    	  viewable; 	    /* Tracks pWin->viewable so SavedRegion may
				     * be initialized correctly when the window
				     * is first mapped */
    char    	  status;    	    /* StatusNoPixmap, etc. */
    char	  backgroundState;  /* background type */
    PixUnion	  background;	    /* background pattern */
} miBSWindowRec, *miBSWindowPtr;

#define StatusNoPixmap	1	/* pixmap has not been created */
#define StatusVirtual	2	/* pixmap is virtual, tiled with background */
#define StatusVDirty	3	/* pixmap is virtual, visiblt has contents */
#define StatusBadAlloc	4	/* pixmap create failed, do not try again */
#define StatusContents	5	/* pixmap is created, has valid contents */

typedef struct {
    /*
     * screen func wrappers
     */
    CloseScreenProcPtr	CloseScreen;
    GetImageProcPtr	GetImage;
    GetSpansProcPtr	GetSpans;
    ChangeWindowAttributesProcPtr ChangeWindowAttributes;
    CreateGCProcPtr	CreateGC;
    DestroyWindowProcPtr DestroyWindow;
} miBSScreenRec, *miBSScreenPtr;
