/* $XFree86: xc/programs/Xserver/hw/xfree86/common/scoasm.h,v 3.1 2003/08/24 17:36:49 dawes Exp $ */

/*
 * Copyright (c) 1996 by The XFree86 Project, Inc.
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
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

/*
 * scoasm.h - used to define inline versions of certain functions which
 * do NOT appear in sys/inline.h.
 */
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#if defined(__SCO__) && defined(__USLC__)
#ifndef _SCOASM_HDR_INC
#define _SCOASM_HDR_INC

asm     void outl(port,val)
{
%reg	port,val;
	movl	port, %edx
	movl	val, %eax
	outl	(%dx)
%reg	port; mem	val;
	movl	port, %edx
	movl    val, %eax
	outl	(%dx)
%mem	port; reg	val;
	movw	port, %dx
	movl	val, %eax
	outl	(%dx)
%mem	port,val;
	movw	port, %dx
	movl    val, %eax
	outl	(%dx)
}

asm	void outw(port,val)
{
%reg	port,val;
	movl	port, %edx
	movl	val, %eax
	data16
	outl	(%dx)
%reg	port; mem	val;
	movl	port, %edx
	movw	val, %ax
	data16
	outl	(%dx)
%mem	port; reg	val;
	movw	port, %dx
	movl	val, %eax
	data16
	outl	(%dx)
%mem	port,val;
	movw	port, %dx
	movw	val, %ax
	data16
	outl	(%dx)
}

asm	void outb(port,val)
{
%reg	port,val;
	movl	port, %edx
	movl	val, %eax
	outb	(%dx)
%reg	port; mem	val;
	movl	port, %edx
	movb	val, %al
	outb	(%dx)
%mem	port; reg	val;
	movw	port, %dx
	movl	val, %eax
	outb	(%dx)
%mem	port,val;
	movw	port, %dx
	movb	val, %al
	outb	(%dx)
}

asm     int inl(port)
{
%reg	port;
	movl	port, %edx
	inl	(%dx)
%mem	port;
	movw	port, %dx
	inl	(%dx)
}

asm	int inw(port)
{
%reg	port;
	subl    %eax, %eax
	movl	port, %edx
	data16
	inl	(%dx)
%mem	port;
	subl    %eax, %eax
	movw	port, %dx
	data16
	inl	(%dx)
}

asm	int inb(port)
{
%reg	port;
	subl    %eax, %eax
	movl	port, %edx
	inb	(%dx)
%mem	port;
	subl    %eax, %eax
	movw	port, %dx
	inb	(%dx)
}

#endif /* _SCOASM_HDR_INC */
#endif /* __SCO__ && __USLC__ */
