/*
 * $Xorg: stip68kgnu.h,v 1.4 2001/02/09 02:04:39 xorgcvs Exp $
 *
Copyright 1990, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.
 *
 * Author:  Keith Packard, MIT X Consortium
 */
/* $XFree86: xc/programs/Xserver/cfb/stip68kgnu.h,v 3.3 2001/01/17 22:36:37 dawes Exp $ */

/*
 * Stipple stack macro for 68k GCC
 */

#define STIPPLE(addr,stipple,value,width,count,shift) \
    __asm volatile ( \
       "lea	5f,%/a1\n\
	moveq	#28,%/d2\n\
	addl	%2,%/d2\n\
	moveq	#28,%/d3\n\
	subql	#4,%2\n\
	negl	%2\n\
1:\n\
	movel	%0,%/a0\n\
	addl	%6,%0\n\
	movel	%3@+,%/d1\n\
	jeq	3f\n\
	movel	%/d1,%/d0\n\
	lsrl	%/d2,%/d0\n\
	lsll	#5,%/d0\n\
	lsll	%2,%/d1\n\
	jmp	%/a1@(%/d0:l)\n\
2:\n\
	addl	#4,%/a0\n\
	movel	%/d1,%/d0\n\
	lsrl	%/d3,%/d0\n\
	lsll	#5,%/d0\n\
	lsll	#4,%/d1\n\
	jmp	%/a1@(%/d0:l)\n\
5:\n\
	jne 2b ; dbra %1,1b ; jra 4f\n\
	. = 5b + 0x20\n\
	moveb	%5,%/a0@(3)\n\
	andl	%/d1,%/d1 ; jne 2b ; dbra %1,1b ; jra 4f\n\
	. = 5b + 0x40\n\
	moveb	%5,%/a0@(2)\n\
	andl	%/d1,%/d1 ; jne 2b ; dbra %1,1b ; jra 4f\n\
	. = 5b + 0x60\n\
	movew	%5,%/a0@(2)\n\
	andl	%/d1,%/d1 ; jne 2b ; dbra %1,1b ; jra 4f\n\
	. = 5b + 0x80\n\
	moveb	%5,%/a0@(1)\n\
	andl	%/d1,%/d1 ; jne 2b ; dbra %1,1b ; jra 4f ;\n\
	. = 5b + 0xa0\n\
	moveb	%5,%/a0@(3) ; moveb	%5,%/a0@(1)\n\
	andl	%/d1,%/d1 ; jne 2b ; dbra %1,1b ; jra 4f ;\n\
	. = 5b + 0xc0\n\
	movew	%5,%/a0@(1)\n\
	andl	%/d1,%/d1 ; jne 2b ; dbra %1,1b ; jra 4f ;\n\
	. = 5b + 0xe0\n\
	movew	%5,%/a0@(2) ; moveb	%5,%/a0@(1)\n\
	andl	%/d1,%/d1 ; jne 2b ; dbra %1,1b ; jra 4f ;\n\
	. = 5b + 0x100\n\
	moveb	%5,%/a0@\n\
	andl	%/d1,%/d1 ; jne 2b ; dbra %1,1b ; jra 4f ;\n\
	. = 5b + 0x120\n\
	moveb	%5,%/a0@(3) ; moveb	%5,%/a0@\n\
	andl	%/d1,%/d1 ; jne 2b ; dbra %1,1b ; jra 4f ;\n\
	. = 5b + 0x140\n\
	moveb	%5,%/a0@(2) ; moveb	%5,%/a0@\n\
	andl	%/d1,%/d1 ; jne 2b ; dbra %1,1b ; jra 4f ;\n\
	. = 5b + 0x160\n\
	movew	%5,%/a0@(2) ; moveb	%5,%/a0@\n\
	andl	%/d1,%/d1 ; jne 2b ; dbra %1,1b ; jra 4f ;\n\
	. = 5b + 0x180\n\
	movew	%5,%/a0@\n\
	andl	%/d1,%/d1 ; jne 2b ; dbra %1,1b ; jra 4f ;\n\
	. = 5b + 0x1a0\n\
	moveb	%5,%/a0@(3) ; movew	%5,%/a0@\n\
	andl	%/d1,%/d1 ; jne 2b ; dbra %1,1b ; jra 4f ;\n\
	. = 5b + 0x1c0\n\
	moveb	%5,%/a0@(2) ; movew	%5,%/a0@\n\
	andl	%/d1,%/d1 ; jne 2b ; dbra %1,1b ; jra 4f ;\n\
	. = 5b + 0x1e0\n\
	movel	%5,%/a0@\n\
	andl	%/d1,%/d1 ; jne 2b ; \n\
3: 	dbra %1,1b ; \n\
4:\n"\
	    : "=a" (addr),	    /* %0 */ \
	      "=d" (count),	    /* %1 */ \
	      "=d" (shift),	    /* %2 */ \
	      "=a" (stipple)	    /* %3 */ \
	    : "0" (addr),	    /* %4 */ \
	      "d" (value),	    /* %5 */ \
	      "a" (width),	    /* %6 */ \
	      "1" (count-1),	    /* %7 */ \
	      "2" (shift),	    /* %8 */ \
	      "3" (stipple)	    /* %9 */ \
	    : /* ctemp */	    "d0", \
 	      /* c */		    "d1", \
	      /* lshift */	    "d2", \
	      /* rshift */	    "d3", \
 	      /* atemp */	    "a0", \
 	      /* case */	    "a1")
