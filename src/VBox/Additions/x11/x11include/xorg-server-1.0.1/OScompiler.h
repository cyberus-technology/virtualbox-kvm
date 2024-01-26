/* $XFree86: xc/programs/Xserver/hw/xfree86/xf4bpp/OScompiler.h,v 1.3 1999/01/31 12:22:15 dawes Exp $ */
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
/* $XConsortium: OScompiler.h /main/4 1996/02/21 17:56:09 kaleb $ */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef __COMPILER_DEPENDANCIES__
#define __COMPILER_DEPENDANCIES__

#define MOVE( src, dst, length ) memcpy( dst, src, length)
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))
#define ABS(x) (((x)>0)?(x):-(x))

#include "misc.h"
#include "xf86_ansic.h"
#include "compiler.h"

#ifdef lint
/* So that lint doesn't complain about constructs it doesn't understand */
#ifdef volatile
#undef volatile
#endif
#define volatile
#ifdef const
#undef const
#endif
#define const
#ifdef signed
#undef signed
#endif
#define signed
#ifdef _ANSI_DECLS_
#undef _ANSI_DECLS_
#endif
#endif

#endif /* !__COMPILER_DEPENDANCIES__ */
