/*
 * Copyright 2000 Francois Gouget.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * Oracle LGPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL or LGPL is available it will apply instead, Oracle elects to use only
 * the Lesser General Public License version 2.1 (LGPLv2) at this time for any software where
 * a choice of LGPL license versions is made available with the language indicating
 * that LGPLv2 or any later version may be used, or where a choice of which version
 * of the LGPL is applied is otherwise unspecified.
 */

#ifndef __WINE_STDDEF_H
#define __WINE_STDDEF_H

#include <crtdefs.h>

#ifndef NULL
#ifdef __cplusplus
#define NULL  0
#else
#define NULL  ((void *)0)
#endif
#endif

#ifdef _WIN64
#define offsetof(s,m)       (size_t)((ptrdiff_t)&(((s*)NULL)->m))
#else
#define offsetof(s,m)       (size_t)&(((s*)NULL)->m)
#endif


#ifdef __cplusplus
extern "C" {
#endif

__msvcrt_ulong __cdecl __threadid(void);
__msvcrt_ulong __cdecl __threadhandle(void);
#define _threadid    (__threadid())

#ifdef __cplusplus
}
#endif

#endif /* __WINE_STDDEF_H */
