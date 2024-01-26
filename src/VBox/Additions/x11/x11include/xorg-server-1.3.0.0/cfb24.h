/*
 * Copyright (C) 1994-1998 The XFree86 Project, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _CFB24_H_
#define _CFB24_H_

/*
 * C's preprocessing language substitutes >text<, not values...
 */

#ifdef OLDPSZ
# undef OLDPSZ
#endif

#ifdef PSZ

# if (PSZ == 8)
#  define OLDPSZ 8
# endif

# if (PSZ == 16)
#  define OLDPSZ 16 
# endif

# if (PSZ == 24)
#  define OLDPSZ 24 
# endif

# if (PSZ == 32)
#  define OLDPSZ 32 
# endif

# ifndef OLDPSZ
   /* Maybe an #error here ? */
# endif

# undef PSZ

#endif

#define PSZ 24
#define CFB_PROTOTYPES_ONLY
#include "cfb.h"
#undef CFB_PROTOTYPES_ONLY
#include "cfbunmap.h"

#undef PSZ
#ifdef OLDPSZ

# if (OLDPSZ == 8)
#  define PSZ 8
# endif

# if (OLDPSZ == 16)
#  define PSZ 16 
# endif

# if (OLDPSZ == 24)
#  define PSZ 24 
# endif

# if (OLDPSZ == 32)
#  define PSZ 32 
# endif

# undef OLDPSZ

#endif

#endif /* _CFB24_H_ */
