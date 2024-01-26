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

typedef struct {
    void * (* queryHyperpipeNetworkFunc)(int, int *, int *);
    void * (* queryHyperpipeConfigFunc)(int, int, int *, int *);
    int    (* destroyHyperpipeConfigFunc)(int, int);
    void * (* hyperpipeConfigFunc)(int, int, int *, int *, void *);
} __GLXHyperpipeExtensionFuncs;

typedef struct {
    int    (* bindSwapBarrierFunc)(int, XID, int);
    int    (* queryMaxSwapBarriersFunc)(int);
} __GLXSwapBarrierExtensionFuncs;

void __glXHyperpipeInit(int screen, __GLXHyperpipeExtensionFuncs *funcs);
void __glXSwapBarrierInit(int screen, __GLXSwapBarrierExtensionFuncs *funcs);

typedef struct __GLXconfig __GLXconfig;
struct __GLXconfig {
    __GLXconfig *next;
    GLuint doubleBufferMode;
    GLuint stereoMode;

    GLint redBits, greenBits, blueBits, alphaBits;	/* bits per comp */
    GLuint redMask, greenMask, blueMask, alphaMask;
    GLint rgbBits;		/* total bits for rgb */
    GLint indexBits;		/* total bits for colorindex */

    GLint accumRedBits, accumGreenBits, accumBlueBits, accumAlphaBits;
    GLint depthBits;
    GLint stencilBits;

    GLint numAuxBuffers;

    GLint level;

    GLint pixmapMode;

    /* GLX */
    GLint visualID;
    GLint visualType;     /**< One of the GLX X visual types. (i.e., 
			   * \c GLX_TRUE_COLOR, etc.)
			   */

    /* EXT_visual_rating / GLX 1.2 */
    GLint visualRating;

    /* EXT_visual_info / GLX 1.2 */
    GLint transparentPixel;
				/*    colors are floats scaled to ints */
    GLint transparentRed, transparentGreen, transparentBlue, transparentAlpha;
    GLint transparentIndex;

    /* ARB_multisample / SGIS_multisample */
    GLint sampleBuffers;
    GLint samples;

    /* SGIX_fbconfig / GLX 1.3 */
    GLint drawableType;
    GLint renderType;
    GLint xRenderable;
    GLint fbconfigID;

    /* SGIX_pbuffer / GLX 1.3 */
    GLint maxPbufferWidth;
    GLint maxPbufferHeight;
    GLint maxPbufferPixels;
    GLint optimalPbufferWidth;   /* Only for SGIX_pbuffer. */
    GLint optimalPbufferHeight;  /* Only for SGIX_pbuffer. */

    /* SGIX_visual_select_group */
    GLint visualSelectGroup;

    /* OML_swap_method */
    GLint swapMethod;

    GLint screen;

    /* EXT_texture_from_pixmap */
    GLint bindToTextureRgb;
    GLint bindToTextureRgba;
    GLint bindToMipmapTexture;
    GLint bindToTextureTargets;
    GLint yInverted;
};

GLint glxConvertToXVisualType(int visualType);

/*
** Screen dependent data.  These methods are the interface between the DIX
** and DDX layers of the GLX server extension.  The methods provide an
** interface for context management on a screen.
*/
typedef struct __GLXscreen __GLXscreen;
struct __GLXscreen {
    void          (*destroy)       (__GLXscreen *screen);

    __GLXcontext *(*createContext) (__GLXscreen *screen,
				    __GLXconfig *modes,
				    __GLXcontext *shareContext);

    __GLXdrawable *(*createDrawable)(__GLXscreen *context,
				     DrawablePtr pDraw,
				     int type,
				     XID drawId,
				     __GLXconfig *modes);
    int            (*swapInterval)  (__GLXdrawable *drawable,
				     int interval);

    __GLXHyperpipeExtensionFuncs *hyperpipeFuncs;
    __GLXSwapBarrierExtensionFuncs *swapBarrierFuncs;

    ScreenPtr pScreen;

    /* Linked list of valid fbconfigs for this screen. */
    __GLXconfig *fbconfigs;
    int numFBConfigs;

    /* Subset of fbconfigs that are exposed as GLX visuals. */
    __GLXconfig **visuals;
    GLint numVisuals;

    char *GLextensions;

    char *GLXvendor;
    char *GLXversion;
    char *GLXextensions;

    Bool (*PositionWindow)(WindowPtr pWin, int x, int y);
    Bool (*CloseScreen)(int index, ScreenPtr pScreen);
};


void __glXScreenInit(__GLXscreen *screen, ScreenPtr pScreen);
void __glXScreenDestroy(__GLXscreen *screen);

#endif /* !__GLX_screens_h__ */
