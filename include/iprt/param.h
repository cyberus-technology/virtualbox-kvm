/** @file
 * IPRT - Parameter Definitions.
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_param_h
#define IPRT_INCLUDED_param_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>

/** @todo Much of the PAGE_* stuff here is obsolete and highly risky to have around.
 * As for component configs (MM_*), either we gather all in here or we move those bits away! */

/** @defgroup   grp_rt_param    System Parameter Definitions
 * @ingroup grp_rt_cdefs
 * @{
 */

/* Undefine PAGE_SIZE and PAGE_SHIFT to avoid unnecessary noice when clashing
 * with system headers. Include system headers before / after iprt depending
 * on which you wish to take precedence. */
#undef PAGE_SIZE
#undef PAGE_SHIFT

/* Undefine PAGE_OFFSET_MASK to avoid the conflict with the-linux-kernel.h */
#undef PAGE_OFFSET_MASK

/**
 * i386 Page size.
 */
#if defined(RT_ARCH_SPARC64)
# define PAGE_SIZE          8192
#elif defined(RT_ARCH_ARM64)
# define PAGE_SIZE          16384
#else
# define PAGE_SIZE          4096
#endif

/**
 * i386 Page shift.
 * This is used to convert between size (in bytes) and page count.
 */
#if defined(RT_ARCH_SPARC64)
# define PAGE_SHIFT         13
#elif defined(RT_ARCH_ARM64)
# define PAGE_SHIFT         14
#else
# define PAGE_SHIFT         12
#endif

/**
 * i386 Page offset mask.
 *
 * @note If you do one-complement this, always insert a target type case after
 *       the operator!  Otherwise you may end up with weird results.
 */
#if defined(RT_ARCH_SPARC64)
# define PAGE_OFFSET_MASK    0x1fff
#elif defined(RT_ARCH_ARM64)
# define PAGE_OFFSET_MASK    0x3fff
#else
# define PAGE_OFFSET_MASK    0xfff
#endif

/**
 * Page address mask for the uintptr_t sized pointers.
 *
 * Be careful when using this since it may be a size too big!
 * @remark  Physical addresses are always masked using X86_PTE_PAE_PG_MASK!
 */
#define PAGE_BASE_MASK      (~(uintptr_t)PAGE_OFFSET_MASK)

/**
 * Get the page aligned address of a POINTER in the CURRENT context.
 *
 * @returns Page aligned address (it's an uintptr_t).
 * @param   pv      The virtual address to align.
 *
 * @remarks Physical addresses are always masked using X86_PTE_PAE_PG_MASK!
 * @remarks This only works with POINTERS in the current context.
 *          Do NOT use on guest address or physical address!
 */
#define PAGE_ADDRESS(pv)    ((uintptr_t)(pv) & ~(uintptr_t)PAGE_OFFSET_MASK)

/**
 * Get the page aligned address of a physical address
 *
 * @returns Page aligned address (it's an RTHCPHYS or RTGCPHYS).
 * @param   Phys    The physical address to align.
 */
#define PHYS_PAGE_ADDRESS(Phys) ((Phys) & X86_PTE_PAE_PG_MASK)

/**
 * Host max path (the reasonable value).
 * @remarks defined both by iprt/param.h and iprt/path.h.
 */
#if !defined(IPRT_INCLUDED_path_h) || defined(DOXYGEN_RUNNING)
# define RTPATH_MAX         (4096 + 4)    /* (PATH_MAX + 1) on linux w/ some alignment */
#endif

/** @} */

#endif /* !IPRT_INCLUDED_param_h */

