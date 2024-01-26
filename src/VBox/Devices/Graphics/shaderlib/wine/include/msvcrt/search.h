/*
 * Heap definitions
 *
 * Copyright 2001 Francois Gouget.
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

#ifndef __WINE_SEARCH_H
#define __WINE_SEARCH_H

#include <crtdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

void* __cdecl _lfind(const void*,const void*,unsigned int*,unsigned int,int (*)(const void*,const void*));
void* __cdecl _lsearch(const void*,void*,unsigned int*,unsigned int,int (*)(const void*,const void*));
void* __cdecl bsearch(const void*,const void*,size_t,size_t,int (*)(const void*,const void*));
void  __cdecl qsort(void*,size_t,size_t,int (*)(const void*,const void*));

#ifdef __cplusplus
}
#endif


static inline void* lfind(const void* match, const void* start, unsigned int* array_size, unsigned int elem_size, int (*cf)(const void*,const void*)) { return _lfind(match, start, array_size, elem_size, cf); }
static inline void* lsearch(const void* match, void* start, unsigned int* array_size, unsigned int elem_size, int (*cf)(const void*,const void*) ) { return _lsearch(match, start, array_size, elem_size, cf); }

#endif /* __WINE_SEARCH_H */
