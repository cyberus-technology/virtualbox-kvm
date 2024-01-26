/* $XFree86: xc/programs/Xserver/GL/glx/glxcontext.h,v 1.3 2001/03/21 16:29:36 dawes Exp $ */
#ifndef _GLX_context_h_
#define _GLX_context_h_

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

typedef struct __GLXcontextRec __GLXcontext;

#include "GL/internal/glcore.h"

struct __GLXcontextRec {
    /*
    ** list of context structs
    */
    struct __GLXcontextRec *last;
    struct __GLXcontextRec *next;

    /*
    ** Pointer to screen info data for this context.  This is set
    ** when the context is created.
    */
    ScreenPtr pScreen;
    __GLXscreenInfo *pGlxScreen;

    /*
    ** This context is created with respect to this visual.
    */
    VisualRec *pVisual;
    __GLXvisualConfig *pGlxVisual;
    __GLXFBConfig *pFBConfig;

    /*
    ** The XID of this context.
    */
    XID id;
    XID *real_ids;

    /*
    ** The XID of the shareList context.
    */
    XID share_id;

    /*
    ** Visual id.
    */
    VisualID vid;
    VisualID *real_vids;

    /*
    ** screen number.
    */
    GLint screen;

    /*
    ** Whether this context's ID still exists.
    */
    GLboolean idExists;
    
    /*
    ** Whether this context is current for some client.
    */
    GLboolean isCurrent;
    
    /*
    ** Buffers for feedback and selection.
    */
    GLfloat *feedbackBuf;
    GLint feedbackBufSize;	/* number of elements allocated */
    GLuint *selectBuf;
    GLint selectBufSize;	/* number of elements allocated */

    /*
    ** Set only if current drawable is a glx pixmap.
    */
    __GLXpixmap *pGlxPixmap;
    __GLXpixmap *pGlxReadPixmap;
    __glXWindow *pGlxWindow;
    __glXWindow *pGlxReadWindow;
    __glXPbuffer *pGlxPbuffer;
    __glXPbuffer *pGlxReadPbuffer;

};

#endif /* !__GLX_context_h__ */
