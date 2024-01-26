/*
 * Copyright (c) 1997  Metro Link Incorporated
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 *
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xisb.h,v 1.1 1998/12/05 14:40:10 dawes Exp $ */

#ifndef	_xisb_H_
#define _xisb_H_

/******************************************************************************
 *		Definitions
 *									structs, typedefs, #defines, enums
 *****************************************************************************/

typedef struct _XISBuffer
{
	int fd;
	int trace;
	int block_duration;
	xf86ssize_t current;	/* bytes read */
	xf86ssize_t end;
	xf86ssize_t buffer_size;
	unsigned char *buf;
} XISBuffer;

/******************************************************************************
 *		Declarations
 *								variables:	use xisb_LOC in front
 *											of globals.
 *											put locals in the .c file.
 *****************************************************************************/
XISBuffer * XisbNew (int fd, xf86ssize_t size);
void XisbFree (XISBuffer *b);
int XisbRead (XISBuffer *b);
xf86ssize_t XisbWrite (XISBuffer *b, unsigned char *msg, xf86ssize_t len);
void XisbTrace (XISBuffer *b, int trace);
void XisbBlockDuration (XISBuffer *b, int block_duration);

/*
 *	DO NOT PUT ANYTHING AFTER THIS ENDIF
 */
#endif
