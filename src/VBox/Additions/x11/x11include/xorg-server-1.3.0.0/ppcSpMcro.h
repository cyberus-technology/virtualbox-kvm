/*
 * Copyright IBM Corporation 1987,1988,1989
 *
 * All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that 
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of IBM not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
*/
/* $XConsortium: ppcSpMcro.h /main/3 1996/02/21 17:58:36 kaleb $ */

/* This screwy macro is used in all the spans routines and you find
   it all over the place, so it is a macro just to tidy things up.
*/

#define SETSPANPTRS(IN,N,IPW,PW,IPPT,PPT,FPW,FPPT,FSORT)		\
	{								\
	N = IN * miFindMaxBand(pGC->pCompositeClip);			\
	if(!(PW = (int *)ALLOCATE_LOCAL(N * sizeof(int))))		\
		return;							\
	if(!(PPT = (DDXPointRec *)ALLOCATE_LOCAL(N * sizeof(DDXPointRec)))) \
		{							\
		DEALLOCATE_LOCAL(PW);					\
		return;							\
    		}							\
	FPW = PW;							\
	FPPT = PPT;							\
	N = miClipSpans(pGC->pCompositeClip, IPPT, IPW, IN,		\
		PPT, PW, FSORT);					\
	}

