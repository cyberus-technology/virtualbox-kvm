/** @file
 * DBGF - Debugger Facility.
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

#ifndef VBOX_INCLUDED_vmm_dbgftrace_h
#define VBOX_INCLUDED_vmm_dbgftrace_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/trace.h>
#include <VBox/types.h>

RT_C_DECLS_BEGIN
/** @defgroup grp_dbgf_trace  Tracing
 * @ingroup grp_dbgf
 *
 * @{
 */

#if (defined(RTTRACE_ENABLED) || defined(DBGFTRACE_ENABLED)) && !defined(DBGFTRACE_DISABLED)
# undef DBGFTRACE_ENABLED
# undef DBGFTRACE_DISABLED
# define DBGFTRACE_ENABLED
#else
# undef DBGFTRACE_ENABLED
# undef DBGFTRACE_DISABLED
# define DBGFTRACE_DISABLED
#endif

VMMDECL(int) DBGFR3TraceConfig(PVM pVM, const char *pszConfig);


/** @name VMM Internal Trace Macros
 * @remarks The user of these macros is responsible of including VBox/vmm/vm.h.
 * @{
 */
/**
 * Records a 64-bit unsigned integer together with a tag string.
 */
#ifdef DBGFTRACE_ENABLED
# define DBGFTRACE_U64_TAG(a_pVM, a_u64, a_pszTag) \
    do { RTTraceBufAddMsgF((a_pVM)->CTX_SUFF(hTraceBuf), "%'llu %s", (a_u64), (a_pszTag)); } while (0)
#else
# define DBGFTRACE_U64_TAG(a_pVM, a_u64, a_pszTag) do { } while (0)
#endif

/**
 * Records a 64-bit unsigned integer together with two tag strings.
 */
#ifdef DBGFTRACE_ENABLED
# define DBGFTRACE_U64_TAG2(a_pVM, a_u64, a_pszTag1, a_pszTag2) \
    do { RTTraceBufAddMsgF((a_pVM)->CTX_SUFF(hTraceBuf), "%'llu %s %s", (a_u64), (a_pszTag1), (a_pszTag2)); } while (0)
#else
# define DBGFTRACE_U64_TAG2(a_pVM, a_u64, a_pszTag1, a_pszTag2) do { } while (0)
#endif

#ifdef RT_COMPILER_SUPPORTS_VA_ARGS
/**
 * Add a custom string (req. variadict macro support).
 */
# ifdef DBGFTRACE_ENABLED
#  define DBGFTRACE_CUSTOM(a_pVM, ...) \
     do { RTTraceBufAddMsgF((a_pVM)->CTX_SUFF(hTraceBuf), __VA_ARGS__); } while (0)
# else
#  define DBGFTRACE_CUSTOM(a_pVM, ...) do { } while (0)
# endif
#endif

/**
 * Records the current source position.
 */
#ifdef DBGFTRACE_ENABLED
# define DBGFTRACE_POS(a_pVM) \
    do { RTTraceBufAddPos((a_pVM)->CTX_SUFF(hTraceBuf), RT_SRC_POS); } while (0)
#else
# define DBGFTRACE_POS(a_pVM) do { } while (0)
#endif

/**
 * Records the current source position along with a 64-bit unsigned integer.
 */
#ifdef DBGFTRACE_ENABLED
# define DBGFTRACE_POS_U64(a_pVM, a_u64) \
    do { RTTraceBufAddPosMsgF((a_pVM)->CTX_SUFF(hTraceBuf), RT_SRC_POS, "%'llu", (a_u64)); } while (0)
#else
# define DBGFTRACE_POS_U64(a_pVM, a_u64) do { } while (0)
#endif
/** @} */


/** @name Tracing Macros for PDM Devices, Drivers and USB Devices.
 * @{
 */

/**
 * Get the trace buffer handle.
 * @param   a_pIns      The instance (pDevIns, pDrvIns or pUsbIns).
 */
#define DBGFTRACE_PDM_TRACEBUF(a_pIns)  ( (a_pIns)->CTX_SUFF(pHlp)->pfnDBGFTraceBuf((a_pIns)) )

/**
 * Records a tagged 64-bit unsigned integer.
 */
#ifdef DBGFTRACE_ENABLED
# define DBGFTRACE_PDM_U64_TAG(a_pIns, a_u64, a_pszTag) \
    do { RTTraceBufAddMsgF(DBGFTRACE_PDM_TRACEBUF(a_pIns), "%'llu %s", (a_u64), (a_pszTag)); } while (0)
#else
# define DBGFTRACE_PDM_U64_TAG(a_pIns, a_u64, a_pszTag) do { } while (0)
#endif

/**
 * Records the current source position.
 */
#ifdef DBGFTRACE_ENABLED
# define DBGFTRACE_PDM_POS(a_pIns) \
    do { RTTraceBufAddPos(DBGFTRACE_PDM_TRACEBUF(a_pIns), RT_SRC_POS); } while (0)
#else
# define DBGFTRACE_PDM_POS(a_pIns) do { } while (0)
#endif

/**
 * Records the current source position along with a 64-bit unsigned integer.
 */
#ifdef DBGFTRACE_ENABLED
# define DBGFTRACE_PDM_POS_U64(a_pIns, a_u64) \
    do { RTTraceBufAddPosMsgF(DBGFTRACE_PDM_TRACEBUF(a_pIns), RT_SRC_POS, "%'llu", (a_u64)); } while (0)
#else
# define DBGFTRACE_PDM_POS_U64(a_pIns, a_u64) do { } while (0)
#endif
/** @} */


/** @} */
RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_dbgftrace_h */
