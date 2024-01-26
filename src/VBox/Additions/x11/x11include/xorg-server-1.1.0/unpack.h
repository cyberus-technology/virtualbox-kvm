/* $XFree86: xc/programs/Xserver/GL/glx/unpack.h,v 1.3 2001/03/21 16:29:37 dawes Exp $ */
#ifndef __GLX_unpack_h__
#define __GLX_unpack_h__

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

#define __GLX_PAD(s) (((s)+3) & (GLuint)~3)

/*
** Fetch the context-id out of a SingleReq request pointed to by pc.
*/
#define __GLX_GET_SINGLE_CONTEXT_TAG(pc) (((xGLXSingleReq*)pc)->contextTag)
#define __GLX_GET_VENDPRIV_CONTEXT_TAG(pc) (((xGLXVendorPrivateReq*)pc)->contextTag)

/*
** Fetch a double from potentially unaligned memory.
*/
#ifdef __GLX_ALIGN64
#define __GLX_MEM_COPY(dst,src,n)	memcpy(dst,src,n)
#define __GLX_GET_DOUBLE(dst,src)	__GLX_MEM_COPY(&dst,src,8)
#else
#define __GLX_GET_DOUBLE(dst,src)	(dst) = *((GLdouble*)(src))
#endif

extern void __glXMemInit(void);

extern xGLXSingleReply __glXReply;

#define __GLX_BEGIN_REPLY(size) \
  	__glXReply.length = __GLX_PAD(size) >> 2;	\
  	__glXReply.type = X_Reply; 			\
  	__glXReply.sequenceNumber = client->sequence;

#define __GLX_SEND_HEADER() \
	WriteToClient( client, sz_xGLXSingleReply, (char *)&__glXReply);

#define __GLX_PUT_RETVAL(a) \
  	__glXReply.retval = (a);
  
#define __GLX_PUT_SIZE(a) \
  	__glXReply.size = (a);

#define __GLX_PUT_RENDERMODE(m) \
        __glXReply.pad3 = (m)

/*
** Get a buffer to hold returned data, with the given alignment.  If we have
** to realloc, allocate size+align, in case the pointer has to be bumped for
** alignment.  The answerBuffer should already be aligned.
**
** NOTE: the cast (long)res below assumes a long is large enough to hold a
** pointer.
*/
#define __GLX_GET_ANSWER_BUFFER(res,cl,size,align)			 \
    if ((size) > sizeof(answerBuffer)) {				 \
	int bump;							 \
	if ((cl)->returnBufSize < (size)+(align)) {			 \
	    (cl)->returnBuf = (GLbyte*)Xrealloc((cl)->returnBuf,	 \
						(size)+(align));         \
	    if (!(cl)->returnBuf) {					 \
		return BadAlloc;					 \
	    }								 \
	    (cl)->returnBufSize = (size)+(align);			 \
	}								 \
	res = (char*)cl->returnBuf;					 \
	bump = (long)(res) % (align);					 \
	if (bump) res += (align) - (bump);				 \
    } else {								 \
	res = (char *)answerBuffer;					 \
    }

#define __GLX_PUT_BYTE() \
  	*(GLbyte *)&__glXReply.pad3 = *(GLbyte *)answer
	  
#define __GLX_PUT_SHORT() \
  	*(GLshort *)&__glXReply.pad3 = *(GLshort *)answer
	  
#define __GLX_PUT_INT() \
  	*(GLint *)&__glXReply.pad3 = *(GLint *)answer
	  
#define __GLX_PUT_FLOAT() \
  	*(GLfloat *)&__glXReply.pad3 = *(GLfloat *)answer
	  
#define __GLX_PUT_DOUBLE() \
  	*(GLdouble *)&__glXReply.pad3 = *(GLdouble *)answer
	  
#define __GLX_SEND_BYTE_ARRAY(len) \
	WriteToClient(client, __GLX_PAD((len)*__GLX_SIZE_INT8), (char *)answer)

#define __GLX_SEND_SHORT_ARRAY(len) \
	WriteToClient(client, __GLX_PAD((len)*__GLX_SIZE_INT16), (char *)answer)
  
#define __GLX_SEND_INT_ARRAY(len) \
	WriteToClient(client, (len)*__GLX_SIZE_INT32, (char *)answer)
  
#define __GLX_SEND_FLOAT_ARRAY(len) \
	WriteToClient(client, (len)*__GLX_SIZE_FLOAT32, (char *)answer)
  
#define __GLX_SEND_DOUBLE_ARRAY(len) \
	WriteToClient(client, (len)*__GLX_SIZE_FLOAT64, (char *)answer)


#define __GLX_SEND_VOID_ARRAY(len)  __GLX_SEND_BYTE_ARRAY(len)
#define __GLX_SEND_UBYTE_ARRAY(len)  __GLX_SEND_BYTE_ARRAY(len)
#define __GLX_SEND_USHORT_ARRAY(len) __GLX_SEND_SHORT_ARRAY(len)
#define __GLX_SEND_UINT_ARRAY(len)  __GLX_SEND_INT_ARRAY(len)

/*
** PERFORMANCE NOTE:
** Machine dependent optimizations abound here; these swapping macros can
** conceivably be replaced with routines that do the job faster.
*/
#define __GLX_DECLARE_SWAP_VARIABLES \
	GLbyte sw; \
  	GLbyte *swapPC;		\
  	GLbyte *swapEnd


#define __GLX_SWAP_INT(pc) 			\
  	sw = ((GLbyte *)(pc))[0]; 		\
  	((GLbyte *)(pc))[0] = ((GLbyte *)(pc))[3]; 	\
  	((GLbyte *)(pc))[3] = sw; 		\
  	sw = ((GLbyte *)(pc))[1]; 		\
  	((GLbyte *)(pc))[1] = ((GLbyte *)(pc))[2]; 	\
  	((GLbyte *)(pc))[2] = sw;	

#define __GLX_SWAP_SHORT(pc) \
  	sw = ((GLbyte *)(pc))[0]; 		\
  	((GLbyte *)(pc))[0] = ((GLbyte *)(pc))[1]; 	\
  	((GLbyte *)(pc))[1] = sw; 	

#define __GLX_SWAP_DOUBLE(pc) \
  	sw = ((GLbyte *)(pc))[0]; 		\
  	((GLbyte *)(pc))[0] = ((GLbyte *)(pc))[7]; 	\
  	((GLbyte *)(pc))[7] = sw; 		\
  	sw = ((GLbyte *)(pc))[1]; 		\
  	((GLbyte *)(pc))[1] = ((GLbyte *)(pc))[6]; 	\
  	((GLbyte *)(pc))[6] = sw;			\
  	sw = ((GLbyte *)(pc))[2]; 		\
  	((GLbyte *)(pc))[2] = ((GLbyte *)(pc))[5]; 	\
  	((GLbyte *)(pc))[5] = sw;			\
  	sw = ((GLbyte *)(pc))[3]; 		\
  	((GLbyte *)(pc))[3] = ((GLbyte *)(pc))[4]; 	\
  	((GLbyte *)(pc))[4] = sw;	

#define __GLX_SWAP_FLOAT(pc) \
  	sw = ((GLbyte *)(pc))[0]; 		\
  	((GLbyte *)(pc))[0] = ((GLbyte *)(pc))[3]; 	\
  	((GLbyte *)(pc))[3] = sw; 		\
  	sw = ((GLbyte *)(pc))[1]; 		\
  	((GLbyte *)(pc))[1] = ((GLbyte *)(pc))[2]; 	\
  	((GLbyte *)(pc))[2] = sw;	

#define __GLX_SWAP_INT_ARRAY(pc, count) \
  	swapPC = ((GLbyte *)(pc));		\
  	swapEnd = ((GLbyte *)(pc)) + (count)*__GLX_SIZE_INT32;\
  	while (swapPC < swapEnd) {		\
	    __GLX_SWAP_INT(swapPC);		\
	    swapPC += __GLX_SIZE_INT32;		\
	}
	
#define __GLX_SWAP_SHORT_ARRAY(pc, count) \
  	swapPC = ((GLbyte *)(pc));		\
  	swapEnd = ((GLbyte *)(pc)) + (count)*__GLX_SIZE_INT16;\
  	while (swapPC < swapEnd) {		\
	    __GLX_SWAP_SHORT(swapPC);		\
	    swapPC += __GLX_SIZE_INT16;		\
	}
	
#define __GLX_SWAP_DOUBLE_ARRAY(pc, count) \
  	swapPC = ((GLbyte *)(pc));		\
  	swapEnd = ((GLbyte *)(pc)) + (count)*__GLX_SIZE_FLOAT64;\
  	while (swapPC < swapEnd) {		\
	    __GLX_SWAP_DOUBLE(swapPC);		\
	    swapPC += __GLX_SIZE_FLOAT64;	\
	}
    
#define __GLX_SWAP_FLOAT_ARRAY(pc, count) \
  	swapPC = ((GLbyte *)(pc));		\
  	swapEnd = ((GLbyte *)(pc)) + (count)*__GLX_SIZE_FLOAT32;\
  	while (swapPC < swapEnd) {		\
	    __GLX_SWAP_FLOAT(swapPC);		\
	    swapPC += __GLX_SIZE_FLOAT32;	\
	}

#define __GLX_SWAP_REPLY_HEADER() \
  	__GLX_SWAP_SHORT(&__glXReply.sequenceNumber); \
  	__GLX_SWAP_INT(&__glXReply.length);

#define __GLX_SWAP_REPLY_RETVAL() \
  	__GLX_SWAP_INT(&__glXReply.retval)

#define __GLX_SWAP_REPLY_SIZE() \
  	__GLX_SWAP_INT(&__glXReply.size)

#endif /* !__GLX_unpack_h__ */





