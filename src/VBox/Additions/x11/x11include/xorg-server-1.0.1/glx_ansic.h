#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _glx_ansic_h_
#define _glx_ansic_h_

/* $XFree86: xc/programs/Xserver/GL/include/GL/glx_ansic.h,v 1.5 2001/03/21 20:49:08 dawes Exp $ */
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
*/

/*
** this needs to check whether we're using XFree86 at all, and then
** which version we're using. Use these macros if version is 3.9+, else
** use normal commands below.
*/

/*
** turns out this include file only exists for XFree86 3.9+ 
** I notice that not having it is not an error and does not stop the build,
** but having it will allow opengl and glx to be built for 3.9+. We no longer
** need an explicit define in the Makefile, just point to the correct X source
** tree and all should be taken care of.
*/

#ifdef XFree86Server

#ifdef XFree86LOADER
#include "xf86_ansic.h"
#endif
#ifndef assert
#define assert(a)
#endif

#else

#if defined(Lynx) && defined(__assert_h)
#undef __assert_h
#endif
#ifdef assert
#undef assert
#endif
#include <assert.h>

#endif


#define GLX_STDOUT			stdout
#define GLX_STDERR			stderr
#define __glXPrintf			printf
#define __glXFprintf			fprintf
#define __glXSprintf			sprintf
#define __glXVfprintf			vfprintf
#define __glXVsprintf			vsprintf
#define __glXFopen			fopen
#define __glXFclose			fclose
#define __glXCos(x)			cos(x)
#define __glXSin(x)			sin(x)
#define __glXAtan(x)			atan(x)
#define __glXAbs(x)			abs(x)
#define __glXLog(x)			log(x)
#define __glXCeil(x)			ceil(x)
#define __glXFloor(x)			floor(x)
#define __glXSqrt(x)			sqrt(x)
#define __glXPow(x, y)			pow(x, y)
#define __glXMemmove(dest, src, n)	memmove(dest, src, n)
#define __glXMemcpy(dest, src, n)	memcpy(dest, src, n)
#define __glXMemset(s, c, n)		memset(s, c, n)
#define __glXStrdup(str)		xstrdup(str)
#define __glXStrcpy(dest, src)		strcpy(dest, src)
#define __glXStrncpy(dest, src, n)	strncpy(dest, src, n)
#define __glXStrcat(dest, src)		strcat(dest, src)
#define __glXStrncat(dest, src, n)	strncat(dest, src, n)
#define __glXStrcmp(s1, s2)		strcmp(s1, s2)
#define __glXStrncmp(s1, s2, n)		strncmp(s1, s2, n)
#define __glXStrlen(str)		strlen(str)
#define __glXAbort()			abort()
#define __glXStrtok(s, delim)		strtok(s, delim)
#define __glXStrcspn(s, reject)		strcspn(s, reject)
#define __glXGetenv(a)			getenv(a)
#define __glXAtoi(a)			atoi(a)

#endif /* _glx_ansic_h_ */

