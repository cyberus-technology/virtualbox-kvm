/*-
 * mibstore.h --
 *	Header file for users of the MI backing-store scheme.
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *	"$Xorg: mibstore.h,v 1.3 2000/08/17 19:53:37 cpqbld Exp $
 */


/* $XFree86: xc/programs/Xserver/mi/mibstore.h,v 1.4 2001/01/17 22:37:06 dawes Exp $ */

#ifndef _MIBSTORE_H
#define _MIBSTORE_H

#include "screenint.h"

extern void miInitializeBackingStore(
    ScreenPtr /*pScreen*/
);

#endif /* _MIBSTORE_H */
