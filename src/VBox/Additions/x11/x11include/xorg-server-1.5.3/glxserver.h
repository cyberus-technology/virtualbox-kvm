#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _GLX_server_h_
#define _GLX_server_h_

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

#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/Xmd.h>
#include <misc.h>
#include <dixstruct.h>
#include <pixmapstr.h>
#include <gcstruct.h>
#include <extnsionst.h>
#include <resource.h>
#include <scrnintstr.h>

/*
** The X header misc.h defines these math functions.
*/
#undef abs
#undef fabs

#define GL_GLEXT_PROTOTYPES /* we want prototypes */
#include <GL/gl.h>
#include <GL/glxproto.h>
#include <GL/glxint.h>

/* For glxscreens.h */
typedef struct __GLXdrawable __GLXdrawable;
typedef struct __GLXcontext __GLXcontext;

#include "glxscreens.h"
#include "glxdrawable.h"
#include "glxcontext.h"


#define GLX_SERVER_MAJOR_VERSION 1
#define GLX_SERVER_MINOR_VERSION 2

#ifndef True
#define True 1
#endif
#ifndef False
#define False 0
#endif

/*
** GLX resources.
*/
typedef XID GLXContextID;
typedef XID GLXPixmap;
typedef XID GLXDrawable;

typedef struct __GLXclientStateRec __GLXclientState;

extern __GLXscreen *glxGetScreen(ScreenPtr pScreen);
extern __GLXclientState *glxGetClient(ClientPtr pClient);

/************************************************************************/

void GlxExtensionInit(void);

void GlxSetVisualConfigs(int nconfigs, 
                         __GLXvisualConfig *configs, void **privates);

struct _glapi_table;
void GlxSetRenderTables (struct _glapi_table *table);

void __glXScreenInitVisuals(__GLXscreen *screen);

/*
** The last context used (from the server's persective) is cached.
*/
extern __GLXcontext *__glXLastContext;
extern __GLXcontext *__glXForceCurrent(__GLXclientState*, GLXContextTag, int*);

extern ClientPtr __pGlxClient;

int __glXError(int error);

/*
** Macros to set, unset, and retrieve the flag that says whether a context
** has unflushed commands.
*/
#define __GLX_NOTE_UNFLUSHED_CMDS(glxc) glxc->hasUnflushedCommands = GL_TRUE
#define __GLX_NOTE_FLUSHED_CMDS(glxc) glxc->hasUnflushedCommands = GL_FALSE
#define __GLX_HAS_UNFLUSHED_CMDS(glxc) (glxc->hasUnflushedCommands)

/************************************************************************/

typedef struct __GLXprovider __GLXprovider;
struct __GLXprovider {
    __GLXscreen *(*screenProbe)(ScreenPtr pScreen);
    const char    *name;
    __GLXprovider *next;
};

void GlxPushProvider(__GLXprovider *provider);

enum {
    GLX_MINIMAL_VISUALS,
    GLX_TYPICAL_VISUALS,
    GLX_ALL_VISUALS
};

void GlxSetVisualConfig(int config);

void __glXsetEnterLeaveServerFuncs(void (*enter)(GLboolean),
				   void (*leave)(GLboolean));
void __glXenterServer(GLboolean rendering);
void __glXleaveServer(GLboolean rendering);

void glxSuspendClients(void);
void glxResumeClients(void);

/*
** State kept per client.
*/
struct __GLXclientStateRec {
    /*
    ** Whether this structure is currently being used to support a client.
    */
    Bool inUse;

    /*
    ** Buffer for returned data.
    */
    GLbyte *returnBuf;
    GLint returnBufSize;

    /*
    ** Keep track of large rendering commands, which span multiple requests.
    */
    GLint largeCmdBytesSoFar;		/* bytes received so far	*/
    GLint largeCmdBytesTotal;		/* total bytes expected		*/
    GLint largeCmdRequestsSoFar;	/* requests received so far	*/
    GLint largeCmdRequestsTotal;	/* total requests expected	*/
    GLbyte *largeCmdBuf;
    GLint largeCmdBufSize;

    /*
    ** Keep a list of all the contexts that are current for this client's
    ** threads.
    */
    __GLXcontext **currentContexts;
    GLint numCurrentContexts;

    /* Back pointer to X client record */
    ClientPtr client;

    int GLClientmajorVersion;
    int GLClientminorVersion;
    char *GLClientextensions;
};

/************************************************************************/

/*
** Dispatch tables.
*/
typedef void (*__GLXdispatchRenderProcPtr)(GLbyte *);
typedef int (*__GLXdispatchSingleProcPtr)(__GLXclientState *, GLbyte *);
typedef int (*__GLXdispatchVendorPrivProcPtr)(__GLXclientState *, GLbyte *);

/*
 * Dispatch for GLX commands.
 */
typedef int (*__GLXprocPtr)(__GLXclientState *, char *pc);

/*
 * Tables for computing the size of each rendering command.
 */
typedef int (*gl_proto_size_func)(const GLbyte *, Bool);

typedef struct {
    int bytes;
    gl_proto_size_func varsize;
} __GLXrenderSizeData;

/************************************************************************/

/*
** X resources.
*/
extern RESTYPE __glXContextRes;
extern RESTYPE __glXClientRes;
extern RESTYPE __glXPixmapRes;
extern RESTYPE __glXDrawableRes;

/************************************************************************/

/*
** Prototypes.
*/

extern char *__glXcombine_strings(const char *, const char *);

/*
** Routines for sending swapped replies.
*/

extern void __glXSwapMakeCurrentReply(ClientPtr client,
				      xGLXMakeCurrentReply *reply);
extern void __glXSwapIsDirectReply(ClientPtr client,
				   xGLXIsDirectReply *reply);
extern void __glXSwapQueryVersionReply(ClientPtr client,
				       xGLXQueryVersionReply *reply);
extern void __glXSwapQueryContextInfoEXTReply(ClientPtr client,
					      xGLXQueryContextInfoEXTReply *reply,
					      int *buf);
extern void __glXSwapGetDrawableAttributesReply(ClientPtr client,
						xGLXGetDrawableAttributesReply *reply, CARD32 *buf);
extern void glxSwapQueryExtensionsStringReply(ClientPtr client,
				xGLXQueryExtensionsStringReply *reply, char *buf);
extern void glxSwapQueryServerStringReply(ClientPtr client,
				xGLXQueryServerStringReply *reply, char *buf);


/*
 * Routines for computing the size of variably-sized rendering commands.
 */

extern int __glXTypeSize(GLenum enm);
extern int __glXImageSize(GLenum format, GLenum type,
    GLenum target, GLsizei w, GLsizei h, GLsizei d,
    GLint imageHeight, GLint rowLength, GLint skipImages, GLint skipRows,
    GLint alignment);

#endif /* !__GLX_server_h__ */
