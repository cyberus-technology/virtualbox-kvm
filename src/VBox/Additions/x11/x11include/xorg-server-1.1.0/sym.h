/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/sym.h,v 1.6 2000/10/24 00:06:55 anderson Exp $ */

/*
 *
 * Copyright 1995,96 by Metro Link, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Metro Link, Inc. not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Metro Link, Inc. makes no
 * representations about the suitability of this software for any purpose.
 *  It is provided "as is" without express or implied warranty.
 *
 * METRO LINK, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL METRO LINK, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef _SYM_H
#define _SYM_H

/*
 * This structure is used to pass in symbol information that is being
 * added to the symbol table.
 */

typedef void (*funcptr) (void);

typedef struct {
    char *symName;
    funcptr offset;
} LOOKUP;

#define SYMFUNC( func ) { #func, (funcptr)&func },
#define SYMFUNCALIAS( name, func ) { name, (funcptr)&func },
#define SYMVAR( var ) { #var, (funcptr)&var },
#define SYMVARALIAS( name, var ) { name, (funcptr)&var },

#endif /* _SYM_H */
