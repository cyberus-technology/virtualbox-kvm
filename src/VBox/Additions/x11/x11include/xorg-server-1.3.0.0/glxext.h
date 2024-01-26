#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _glxext_h_
#define _glxext_h_

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

/*
 * Added by VA Linux for XFree86 4.0.x
 */
typedef struct {
    int type;
    void (*resetExtension)(void);
    Bool (*initVisuals)(
        VisualPtr *       visualp,
        DepthPtr *        depthp,
        int *             nvisualp,
        int *             ndepthp,
        int *             rootDepthp,
        VisualID *        defaultVisp,
        unsigned long     sizes,
        int               bitsPerRGB
        );
    void (*setVisualConfigs)(
        int                nconfigs,
        __GLXvisualConfig *configs,
        void              **privates
        );
} __GLXextensionInfo;

extern GLboolean __glXFreeContext(__GLXcontext *glxc);
extern void __glXFlushContextCache(void);

extern void __glXErrorCallBack(__GLinterface *gc, GLenum code);
extern void __glXClearErrorOccured(void);
extern GLboolean __glXErrorOccured(void);
extern void __glXResetLargeCommandStatus(__GLXclientState*);

extern int DoMakeCurrent( __GLXclientState *cl, GLXDrawable drawId,
    GLXDrawable readId, GLXContextID contextId, GLXContextTag tag );
extern int DoGetVisualConfigs(__GLXclientState *cl, unsigned screen,
    GLboolean do_swap);
extern int DoGetFBConfigs(__GLXclientState *cl, unsigned screen,
    GLboolean do_swap);
extern int DoCreateContext(__GLXclientState *cl, GLXContextID gcId,
    GLXContextID shareList, VisualID visual, GLuint screen, GLboolean isDirect);
extern int DoCreateGLXPixmap(__GLXclientState *cl, XID fbconfigId,
    GLuint screenNum, XID pixmapId, XID glxpixmapId);
extern int DoDestroyPixmap(__GLXclientState *cl, XID glxpixmapId);

extern int DoQueryContext(__GLXclientState *cl, GLXContextID gcId);

extern int DoRender(__GLXclientState *cl, GLbyte *pc, int do_swap);
extern int DoRenderLarge(__GLXclientState *cl, GLbyte *pc, int do_swap);

extern void GlxExtensionInit(void);

extern const char GLServerVersion[];
extern int DoGetString(__GLXclientState *cl, GLbyte *pc, GLboolean need_swap);

extern int GlxInitVisuals(
    VisualPtr *       visualp,
    DepthPtr *        depthp,
    int *             nvisualp,
    int *             ndepthp,
    int *             rootDepthp,
    VisualID *        defaultVisp,
    unsigned long     sizes,
    int               bitsPerRGB,
    int               preferredVis
);

typedef struct {
    void * (* queryHyperpipeNetworkFunc)(int, int *, int *);
    void * (* queryHyperpipeConfigFunc)(int, int, int *, int *);
    int    (* destroyHyperpipeConfigFunc)(int, int);
    void * (* hyperpipeConfigFunc)(int, int, int *, int *, void *);
} __GLXHyperpipeExtensionFuncs;

extern void __glXHyperpipeInit(int screen, __GLXHyperpipeExtensionFuncs *funcs);

extern __GLXHyperpipeExtensionFuncs *__glXHyperpipeFuncs;

typedef struct {
    int    (* bindSwapBarrierFunc)(int, XID, int);
    int    (* queryMaxSwapBarriersFunc)(int);
} __GLXSwapBarrierExtensionFuncs;

extern void __glXSwapBarrierInit(int screen, __GLXSwapBarrierExtensionFuncs *funcs);

extern __GLXSwapBarrierExtensionFuncs *__glXSwapBarrierFuncs;

#endif /* _glxext_h_ */

