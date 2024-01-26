/*
 * Copyright (C) 1999 Patrik Stridvall
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

#if defined(__WINE_PSHPACK_H15)

   /* Depth > 15 */
#  error "Alignment nesting > 15 is not supported"

#else

#  if !defined(__WINE_PSHPACK_H)
#    define __WINE_PSHPACK_H  4
     /* Depth == 1 */
#  elif !defined(__WINE_PSHPACK_H2)
#    define __WINE_PSHPACK_H2 4
     /* Depth == 2 */
#    define __WINE_INTERNAL_POPPACK
#    include <poppack.h>
#  elif !defined(__WINE_PSHPACK_H3)
#    define __WINE_PSHPACK_H3 4
     /* Depth == 3 */
#    define __WINE_INTERNAL_POPPACK
#    include <poppack.h>
#  elif !defined(__WINE_PSHPACK_H4)
#    define __WINE_PSHPACK_H4 4
     /* Depth == 4 */
#    define __WINE_INTERNAL_POPPACK
#    include <poppack.h>
#  elif !defined(__WINE_PSHPACK_H5)
#    define __WINE_PSHPACK_H5 4
     /* Depth == 5 */
#    define __WINE_INTERNAL_POPPACK
#    include <poppack.h>
#  elif !defined(__WINE_PSHPACK_H6)
#    define __WINE_PSHPACK_H6 4
     /* Depth == 6 */
#    define __WINE_INTERNAL_POPPACK
#    include <poppack.h>
#  elif !defined(__WINE_PSHPACK_H7)
#    define __WINE_PSHPACK_H7 4
     /* Depth == 7 */
#    define __WINE_INTERNAL_POPPACK
#    include <poppack.h>
#  elif !defined(__WINE_PSHPACK_H8)
#    define __WINE_PSHPACK_H8 4
     /* Depth == 8 */
#    define __WINE_INTERNAL_POPPACK
#    include <poppack.h>
#  elif !defined(__WINE_PSHPACK_H9)
#    define __WINE_PSHPACK_H9 4
     /* Depth == 9 */
#    define __WINE_INTERNAL_POPPACK
#    include <poppack.h>
#  elif !defined(__WINE_PSHPACK_H10)
#    define __WINE_PSHPACK_H10 4
     /* Depth == 10 */
#    define __WINE_INTERNAL_POPPACK
#    include <poppack.h>
#  elif !defined(__WINE_PSHPACK_H11)
#    define __WINE_PSHPACK_H11 4
     /* Depth == 11 */
#    define __WINE_INTERNAL_POPPACK
#    include <poppack.h>
#  elif !defined(__WINE_PSHPACK_H12)
#    define __WINE_PSHPACK_H12 4
     /* Depth == 12 */
#    define __WINE_INTERNAL_POPPACK
#    include <poppack.h>
#  elif !defined(__WINE_PSHPACK_H13)
#    define __WINE_PSHPACK_H13 4
     /* Depth == 13 */
#    define __WINE_INTERNAL_POPPACK
#    include <poppack.h>
#  elif !defined(__WINE_PSHPACK_H14)
#    define __WINE_PSHPACK_H14 4
     /* Depth == 14 */
#    define __WINE_INTERNAL_POPPACK
#    include <poppack.h>
#  elif !defined(__WINE_PSHPACK_H15)
#    define __WINE_PSHPACK_H15 4
     /* Depth == 15 */
#    define __WINE_INTERNAL_POPPACK
#    include <poppack.h>
#  endif

#  if defined(_MSC_VER) && (_MSC_VER >= 800)
#   pragma warning(disable:4103)
#  endif
#  if defined(__clang_major__) && defined(__has_warning)
#   if __has_warning("-Wpragma-pack")
#    pragma clang diagnostic ignored "-Wpragma-pack"
#   endif
#  endif

#  pragma pack(4)

#endif
