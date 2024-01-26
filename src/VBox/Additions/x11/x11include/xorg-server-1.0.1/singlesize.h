/* $XFree86$ */
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _singlesize_h_
#define _singlesize_h_

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

#include "indirect_size.h"

extern GLint __glReadPixels_size(GLenum format, GLenum type,
				 GLint width, GLint height);
extern GLint __glGetTexEnvfv_size(GLenum pname);
extern GLint __glGetTexEnviv_size(GLenum pname);
extern GLint __glGetTexGenfv_size(GLenum pname);
extern GLint __glGetTexGendv_size(GLenum pname);
extern GLint __glGetTexGeniv_size(GLenum pname);
extern GLint __glGetTexParameterfv_size(GLenum pname);
extern GLint __glGetTexParameteriv_size(GLenum pname);
extern GLint __glGetLightfv_size(GLenum pname);
extern GLint __glGetLightiv_size(GLenum pname);
extern GLint __glGetMap_size(GLenum pname, GLenum query);
extern GLint __glGetMapdv_size(GLenum target, GLenum query);
extern GLint __glGetMapfv_size(GLenum target, GLenum query);
extern GLint __glGetMapiv_size(GLenum target, GLenum query);
extern GLint __glGetMaterialfv_size(GLenum pname);
extern GLint __glGetMaterialiv_size(GLenum pname);
extern GLint __glGetPixelMap_size(GLenum map);
extern GLint __glGetPixelMapfv_size(GLenum map);
extern GLint __glGetPixelMapuiv_size(GLenum map);
extern GLint __glGetPixelMapusv_size(GLenum map);
extern GLint __glGet_size(GLenum sq);
extern GLint __glGetDoublev_size(GLenum sq);
extern GLint __glGetFloatv_size(GLenum sq);
extern GLint __glGetIntegerv_size(GLenum sq);
extern GLint __glGetBooleanv_size(GLenum sq);
extern GLint __glGetTexLevelParameterfv_size(GLenum pname);
extern GLint __glGetTexLevelParameteriv_size(GLenum pname);
extern GLint __glGetTexImage_size(GLenum target, GLint level, GLenum format,
				  GLenum type, GLint width, GLint height,
				  GLint depth);
extern GLint __glGetColorTableParameterfv_size(GLenum pname);
extern GLint __glGetColorTableParameteriv_size(GLenum pname);
extern GLint __glGetConvolutionParameterfv_size(GLenum pname);
extern GLint __glGetConvolutionParameteriv_size(GLenum pname);
extern GLint __glGetHistogramParameterfv_size(GLenum pname);
extern GLint __glGetHistogramParameteriv_size(GLenum pname);
extern GLint __glGetMinmaxParameterfv_size(GLenum pname);
extern GLint __glGetMinmaxParameteriv_size(GLenum pname);

#endif /* _singlesize_h_ */

