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

#ifndef __GLXSINGLE_H
#define __GLXSINGLE_H

extern int __glXForwardSingleReq( __GLXclientState *cl, GLbyte *pc );
extern int __glXForwardPipe0WithReply( __GLXclientState *cl, GLbyte *pc );
extern int __glXForwardAllWithReply( __GLXclientState *cl, GLbyte *pc );

extern int __glXForwardSingleReqSwap( __GLXclientState *cl, GLbyte *pc );

extern int __glXForwardPipe0WithReplySwap( __GLXclientState *cl, GLbyte *pc );
extern int __glXForwardPipe0WithReplySwapsv( __GLXclientState *cl, GLbyte *pc );
extern int __glXForwardPipe0WithReplySwapiv( __GLXclientState *cl, GLbyte *pc );
extern int __glXForwardPipe0WithReplySwapdv( __GLXclientState *cl, GLbyte *pc );

extern int __glXForwardAllWithReplySwap( __GLXclientState *cl, GLbyte *pc );
extern int __glXForwardAllWithReplySwapsv( __GLXclientState *cl, GLbyte *pc );
extern int __glXForwardAllWithReplySwapiv( __GLXclientState *cl, GLbyte *pc );
extern int __glXForwardAllWithReplySwapdv( __GLXclientState *cl, GLbyte *pc );

extern int __glXDisp_ReadPixels(__GLXclientState *cl, GLbyte *pc);
extern int __glXDispSwap_GetTexImage(__GLXclientState *cl, GLbyte *pc);
extern int __glXDispSwap_GetColorTable(__GLXclientState *cl, GLbyte *pc);

#endif
