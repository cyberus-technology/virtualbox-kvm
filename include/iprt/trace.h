/** @file
 * IPRT - Tracing.
 */

/*
 * Copyright (C) 2011-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_trace_h
#define IPRT_INCLUDED_trace_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/stdarg.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_trace      RTTrace - Tracing
 * @ingroup grp_rt
 *
 * The tracing facility is somewhat similar to a stripped down logger that
 * outputs to a circular buffer.  Part of the idea here is that it can the
 * overhead is much smaller and that it can be done without involving any
 * locking or other thing that could throw off timing.
 *
 * @{
 */


#ifdef DOXYGEN_RUNNING
# define RTTRACE_DISABLED
# define RTTRACE_ENABLED
#endif

/** @def RTTRACE_DISABLED
 * Use this compile time define to disable all tracing macros.  This trumps
 * RTTRACE_ENABLED.
 */

/** @def RTTRACE_ENABLED
 * Use this compile time define to enable tracing when not in debug mode
 */

/*
 * Determine whether tracing is enabled and forcefully normalize the indicators.
 */
#if (defined(DEBUG) || defined(RTTRACE_ENABLED)) && !defined(RTTRACE_DISABLED)
# undef  RTTRACE_DISABLED
# undef  RTTRACE_ENABLED
# define RTTRACE_ENABLED
#else
# undef  RTTRACE_DISABLED
# undef  RTTRACE_ENABLED
# define RTTRACE_DISABLED
#endif


/** @name RTTRACEBUF_FLAGS_XXX - RTTraceBufCarve and RTTraceBufCreate flags.
 * @{ */
/** Free the memory block on release using RTMemFree(). */
#define RTTRACEBUF_FLAGS_FREE_ME        RT_BIT_32(0)
/** Whether the trace buffer is disabled or enabled. */
#define RTTRACEBUF_FLAGS_DISABLED       RT_BIT_32(RTTRACEBUF_FLAGS_DISABLED_BIT)
/** The bit number corresponding to the RTTRACEBUF_FLAGS_DISABLED mask. */
#define RTTRACEBUF_FLAGS_DISABLED_BIT   1
/** Mask of the valid flags. */
#define RTTRACEBUF_FLAGS_MASK           UINT32_C(0x00000003)
/** @}  */


RTDECL(int)         RTTraceBufCreate(PRTTRACEBUF hTraceBuf, uint32_t cEntries, uint32_t cbEntry, uint32_t fFlags);
RTDECL(int)         RTTraceBufCarve(PRTTRACEBUF hTraceBuf, uint32_t cEntries, uint32_t cbEntry, uint32_t fFlags,
                                    void *pvBlock, size_t *pcbBlock);
RTDECL(uint32_t)    RTTraceBufRetain(RTTRACEBUF hTraceBuf);
RTDECL(uint32_t)    RTTraceBufRelease(RTTRACEBUF hTraceBuf);
RTDECL(int)         RTTraceBufDumpToLog(RTTRACEBUF hTraceBuf);
RTDECL(int)         RTTraceBufDumpToAssert(RTTRACEBUF hTraceBuf);

/**
 * Trace buffer callback for processing one entry.
 *
 * Used by RTTraceBufEnumEntries.
 *
 * @returns IPRT status code.  Any status code but VINF_SUCCESS will abort the
 *          enumeration and be returned by RTTraceBufEnumEntries.
 * @param   hTraceBuf           The trace buffer handle.
 * @param   iEntry              The entry number.
 * @param   NanoTS              The timestamp of the entry.
 * @param   idCpu               The ID of the CPU which added the entry.
 * @param   pszMsg              The message text.
 * @param   pvUser              The user argument.
 */
typedef DECLCALLBACKTYPE(int, FNRTTRACEBUFCALLBACK,(RTTRACEBUF hTraceBuf, uint32_t iEntry, uint64_t NanoTS,
                                                    RTCPUID idCpu, const char *pszMsg, void *pvUser));
/** Pointer to trace buffer enumeration callback function. */
typedef FNRTTRACEBUFCALLBACK *PFNRTTRACEBUFCALLBACK;

/**
 * Enumerates the used trace buffer entries, calling @a pfnCallback for each.
 *
 * @returns IPRT status code.  Should the callback (@a pfnCallback) return
 *          anything other than VINF_SUCCESS, then the enumeration will be
 *          aborted and the status code will be returned by this function.
 * @retval  VINF_SUCCESS
 * @retval  VERR_INVALID_HANDLE
 * @retval  VERR_INVALID_PARAMETER
 * @retval  VERR_INVALID_POINTER
 *
 * @param   hTraceBuf           The trace buffer handle.  Special handles are
 *                              accepted.
 * @param   pfnCallback         The callback to call for each entry.
 * @param   pvUser              The user argument for the callback.
 */
RTDECL(int)         RTTraceBufEnumEntries(RTTRACEBUF hTraceBuf, PFNRTTRACEBUFCALLBACK pfnCallback, void *pvUser);

/**
 * Gets the entry size used by the specified trace buffer.
 *
 * @returns The size on success, 0 if the handle is invalid.
 *
 * @param   hTraceBuf           The trace buffer handle.  Special handles are
 *                              accepted.
 */
RTDECL(uint32_t)    RTTraceBufGetEntrySize(RTTRACEBUF hTraceBuf);

/**
 * Gets the number of entries in the specified trace buffer.
 *
 * @returns The entry count on success, 0 if the handle is invalid.
 *
 * @param   hTraceBuf           The trace buffer handle.  Special handles are
 *                              accepted.
 */
RTDECL(uint32_t)    RTTraceBufGetEntryCount(RTTRACEBUF hTraceBuf);


/**
 * Disables tracing.
 *
 * @returns @c true if tracing was enabled prior to this call, @c false if
 *          disabled already.
 *
 * @param   hTraceBuf           The trace buffer handle.  Special handles are
 *                              accepted.
 */
RTDECL(bool)        RTTraceBufDisable(RTTRACEBUF hTraceBuf);

/**
 * Enables tracing.
 *
 * @returns @c true if tracing was enabled prior to this call, @c false if
 *          disabled already.
 *
 * @param   hTraceBuf           The trace buffer handle.  Special handles are
 *                              accepted.
 */
RTDECL(bool)        RTTraceBufEnable(RTTRACEBUF hTraceBuf);


RTDECL(int)         RTTraceBufAddMsg(      RTTRACEBUF hTraceBuf, const char *pszMsg);
RTDECL(int)         RTTraceBufAddMsgF(     RTTRACEBUF hTraceBuf, const char *pszMsgFmt, ...) RT_IPRT_FORMAT_ATTR(2, 3);
RTDECL(int)         RTTraceBufAddMsgV(     RTTRACEBUF hTraceBuf, const char *pszMsgFmt, va_list va) RT_IPRT_FORMAT_ATTR(2, 0);
RTDECL(int)         RTTraceBufAddMsgEx(    RTTRACEBUF hTraceBuf, const char *pszMsg, size_t cbMaxMsg);

RTDECL(int)         RTTraceBufAddPos(      RTTRACEBUF hTraceBuf, RT_SRC_POS_DECL);
RTDECL(int)         RTTraceBufAddPosMsg(   RTTRACEBUF hTraceBuf, RT_SRC_POS_DECL, const char *pszMsg);
RTDECL(int)         RTTraceBufAddPosMsgEx( RTTRACEBUF hTraceBuf, RT_SRC_POS_DECL, const char *pszMsg, size_t cbMaxMsg);
RTDECL(int)         RTTraceBufAddPosMsgF(  RTTRACEBUF hTraceBuf, RT_SRC_POS_DECL, const char *pszMsgFmt, ...) RT_IPRT_FORMAT_ATTR(5, 6);
RTDECL(int)         RTTraceBufAddPosMsgV(  RTTRACEBUF hTraceBuf, RT_SRC_POS_DECL, const char *pszMsgFmt, va_list va) RT_IPRT_FORMAT_ATTR(5, 0);


RTDECL(int)         RTTraceSetDefaultBuf(RTTRACEBUF hTraceBuf);
RTDECL(RTTRACEBUF)  RTTraceGetDefaultBuf(void);


/** @def RTTRACE_BUF
 * The trace buffer used by the macros.
 */
#ifndef RTTRACE_BUF
# define RTTRACE_BUF        NULL
#endif

/**
 * Record the current source position.
 */
#ifdef RTTRACE_ENABLED
# define RTTRACE_POS()              do { RTTraceBufAddPos(RTTRACE_BUF, RT_SRC_POS); } while (0)
#else
# define RTTRACE_POS()              do { } while (0)
#endif


/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_trace_h */

