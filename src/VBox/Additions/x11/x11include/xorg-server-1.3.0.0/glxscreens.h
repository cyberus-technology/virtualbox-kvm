#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _GLX_screens_h_
#define _GLX_screens_h_

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

/*
** Screen dependent data.  These methods are the interface between the DIX
** and DDX layers of the GLX server extension.  The methods provide an
** interface for context management on a screen.
*/
typedef struct __GLXscreen __GLXscreen;
struct __GLXscreen {
    void          (*destroy)       (__GLXscreen *screen);

    __GLXcontext *(*createContext) (__GLXscreen *screen,
				    __GLcontextModes *modes,
				    __GLXcontext *shareContext);

    __GLXdrawable *(*createDrawable)(__GLXscreen *context,
				     DrawablePtr pDraw,
				     XID drawId,
				     __GLcontextModes *modes);
    int            (*swapInterval)  (__GLXdrawable *drawable,
				     int interval);

    ScreenPtr pScreen;

    /**
     * Linked list of valid context modes for this screen.
     */
    __GLcontextModes *modes;

    void **pVisualPriv;
    GLint numVisuals;
    GLint numUsableVisuals;

    char *GLextensions;

    char *GLXvendor;
    char *GLXversion;
    char *GLXextensions;

    /*
    ** Things that are not statically set.
    */
    Bool (*WrappedPositionWindow)(WindowPtr pWin, int x, int y);

};


void __glXScreenInit(__GLXscreen *screen, ScreenPtr pScreen);
void __glXScreenDestroy(__GLXscreen *screen);

void __glXInitScreens(void);
extern void __glXResetScreens(void);

#endif /* !__GLX_screens_h__ */
