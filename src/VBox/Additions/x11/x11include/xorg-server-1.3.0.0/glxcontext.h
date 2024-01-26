#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

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

/* XXX: should be defined somewhere globally */
#define CAPI

#include "GL/internal/glcore.h"

typedef struct __GLXtextureFromPixmap __GLXtextureFromPixmap;
struct __GLXtextureFromPixmap {
    int (*bindTexImage)		(__GLXcontext *baseContext,
				 int           buffer,
				 __GLXpixmap  *pixmap);
    int (*releaseTexImage)	(__GLXcontext *baseContext,
				 int           buffer,
				 __GLXpixmap  *pixmap);
};


struct __GLXcontext {
    void           (*destroy)       (__GLXcontext *context);
    int            (*makeCurrent)   (__GLXcontext *context);
    int            (*loseCurrent)   (__GLXcontext *context);
    int            (*copy)          (__GLXcontext *dst,
				     __GLXcontext *src,
				     unsigned long mask);
    int            (*forceCurrent)  (__GLXcontext *context);

    __GLXtextureFromPixmap *textureFromPixmap;

    /*
    ** list of context structs
    */
    __GLXcontext *last;
    __GLXcontext *next;

    /*
    ** list of contexts bound to the same drawable
    */
    __GLXcontext *nextDrawPriv;
    __GLXcontext *nextReadPriv;

    /*
    ** mode struct for this context
    */
    __GLcontextModes *modes;

    /*
    ** Pointer to screen info data for this context.  This is set
    ** when the context is created.
    */
    ScreenPtr pScreen;
    __GLXscreen *pGlxScreen;

    /*
    ** This context is created with respect to this visual.
    */
    VisualRec *pVisual;

    /*
    ** The XID of this context.
    */
    XID id;

    /*
    ** The XID of the shareList context.
    */
    XID share_id;

    /*
    ** Visual id.
    */
    VisualID vid;

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
    ** Whether this context is a direct rendering context.
    */
    GLboolean isDirect;

    /*
    ** Window pending state
    */
    GLuint pendingState;

    /*
    ** This flag keeps track of whether there are unflushed GL commands.
    */
    GLboolean hasUnflushedCommands;
    
    /*
    ** Current rendering mode for this context.
    */
    GLenum renderMode;
    
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
    __GLXpixmap *drawPixmap;
    __GLXpixmap *readPixmap;

    /*
    ** The drawable private this context is bound to
    */
    __GLXdrawable *drawPriv;
    __GLXdrawable *readPriv;
};

/* pending state defines */
#define __GLX_PENDING_RESIZE	0x1
#define	__GLX_PENDING_DESTROY	0x2
#define __GLX_PENDING_SWAP	0x4

void __glXContextDestroy(__GLXcontext *context);

#endif /* !__GLX_context_h__ */
