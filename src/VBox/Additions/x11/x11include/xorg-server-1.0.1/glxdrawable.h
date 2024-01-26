/* $XFree86: xc/programs/Xserver/GL/glx/glxdrawable.h,v 1.3 2001/03/21 16:29:36 dawes Exp $ */
#ifndef _GLX_drawable_h_
#define _GLX_drawable_h_

/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
**
*/

typedef struct {

    DrawablePtr pDraw;
    __GLXvisualConfig *pGlxVisual;
    __GLXscreenInfo *pGlxScreen;
    __GLXFBConfig *pFBConfig;
    ScreenPtr pScreen;
    Bool idExists;
    int refcnt;
    XID *be_xids;

} __GLXpixmap;

struct __GLXdrawablePrivateRec {
    /*
    ** list of drawable private structs
    */
    struct __GLXdrawablePrivateRec *last;
    struct __GLXdrawablePrivateRec *next;

    DrawablePtr pDraw;
    XID drawId;
    __GLXpixmap *pGlxPixmap;

    /*
    ** Either DRAWABLE_PIXMAP or DRAWABLE_WINDOW, copied from pDraw above.
    ** Needed by the resource freer because pDraw might already have been
    ** freed.
    */
    int type;

    /*
    ** Configuration of the visual to which this drawable was created.
    */
    __GLXvisualConfig *pGlxVisual;

    /*
    ** cached drawable size and origin
    */
    GLint xorigin, yorigin;
    GLint width, height;

    /*
    ** list of contexts bound to this drawable
    */
    struct __GLXcontextRec *glxc;

    /*
    ** "methods" that the drawble should be able to respond to.
    */
    void (*freeBuffers)(struct __GLXdrawablePrivateRec *);
    void (*updatePalette)(struct __GLXdrawablePrivateRec *);
    GLboolean (*swapBuffers)(struct __GLXdrawablePrivateRec *);

    /*
    ** The GL drawable (information shared between GLX and the GL core)
    */
    __GLdrawablePrivate glPriv;

    /*
    ** reference count
    */
    int refCount;
};

typedef struct {
    DrawablePtr pDraw;
    int type;
    Bool idExists;
    int refcnt;                         /* contexts bound */
    __GLXFBConfig *pGlxFBConfig;
    ScreenPtr pScreen;
} __glXWindow;


typedef struct {
    __GLXscreenInfo *pGlxScreen;
    __GLXFBConfig *pFBConfig;
    ScreenPtr pScreen;
    Bool idExists;
    int refcnt;
    XID *be_xids;
} __glXPbuffer;

#endif /* !__GLX_drawable_h__ */
