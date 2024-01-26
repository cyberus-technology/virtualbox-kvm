/* $Xorg: migc.h,v 1.4 2001/02/09 02:05:21 xorgcvs Exp $ */
/*

Copyright 1993, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

/* $XFree86: xc/programs/Xserver/mi/migc.h,v 1.7 2001/08/06 20:51:18 dawes Exp $ */

extern void miChangeGC(
    GCPtr  /*pGC*/,
    unsigned long /*mask*/
);

extern void miDestroyGC(
    GCPtr  /*pGC*/
);

extern GCOpsPtr miCreateGCOps(
    GCOpsPtr /*prototype*/
);

extern void miDestroyGCOps(
    GCOpsPtr /*ops*/
);

extern void miDestroyClip(
    GCPtr /*pGC*/
);

extern void miChangeClip(
    GCPtr   /*pGC*/,
    int     /*type*/,
    pointer /*pvalue*/,
    int     /*nrects*/
);

extern void miCopyClip(
    GCPtr /*pgcDst*/,
    GCPtr /*pgcSrc*/
);

extern void miCopyGC(
    GCPtr /*pGCSrc*/,
    unsigned long /*changes*/,
    GCPtr /*pGCDst*/
);

extern void miComputeCompositeClip(
    GCPtr       /*pGC*/,
    DrawablePtr /*pDrawable*/
);
