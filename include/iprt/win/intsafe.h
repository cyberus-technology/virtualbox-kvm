/** @file
 * Safe way to include intsafe.h.
 */

/*
 * Copyright (C) 2016-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef IPRT_INCLUDED_win_intsafe_h
#define IPRT_INCLUDED_win_intsafe_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* There's a conflict between the Visual C++ 2010 stdint.h and wDK 7.1 intsafe.h
   that we must to mediate here.  Current approach is to use the stuff from
   intsafe.h rather than the other. */
#ifndef _INTSAFE_H_INCLUDED_
# include <iprt/stdint.h>
# undef INT8_MIN
# undef INT16_MIN
# undef INT32_MIN
# undef INT8_MAX
# undef INT16_MAX
# undef INT32_MAX
# undef UINT8_MAX
# undef UINT16_MAX
# undef UINT32_MAX
# undef INT64_MIN
# undef INT64_MAX
# undef UINT64_MAX

# ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable:4668) /* intsafe.h(55) : warning C4668: '__midl' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif' */
#  if _MSC_VER >= 1800 /*RT_MSC_VER_VC120*/
#   pragma warning(disable:4005) /* sdk/v7.1/include/sal_supp.h(57) : warning C4005: '__useHeader' : macro redefinition */
#  endif
# endif

# include <intsafe.h>

# ifdef _MSC_VER
#  pragma warning(pop)
# endif

#endif /* !_INTSAFE_H_INCLUDED_ */

#endif /* !IPRT_INCLUDED_win_intsafe_h */

