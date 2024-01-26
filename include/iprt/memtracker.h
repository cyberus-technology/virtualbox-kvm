/** @file
 * IPRT - Memory Tracker.
 */

/*
 * Copyright (C) 2010-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_memtracker_h
#define IPRT_INCLUDED_memtracker_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/list.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_memtracker RTMemTracker - Memory Allocation Tracker.
 * @ingroup grp_rt
 * @{
 */

/**
 * The allocation/free method.
 */
typedef enum RTMEMTRACKERMETHOD
{
    RTMEMTRACKERMETHOD_INVALID = 0,
    RTMEMTRACKERMETHOD_ALLOC,
    RTMEMTRACKERMETHOD_ALLOCZ,
    RTMEMTRACKERMETHOD_REALLOC_PREP,   /**< Internal, don't use. */
    RTMEMTRACKERMETHOD_REALLOC_DONE,   /**< Internal, don't use. */
    RTMEMTRACKERMETHOD_REALLOC_FAILED, /**< Internal, don't use. */
    RTMEMTRACKERMETHOD_FREE,

    RTMEMTRACKERMETHOD_NEW,
    RTMEMTRACKERMETHOD_NEW_ARRAY,
    RTMEMTRACKERMETHOD_DELETE,
    RTMEMTRACKERMETHOD_DELETE_ARRAY,
    RTMEMTRACKERMETHOD_END,
    RTMEMTRACKERMETHOD_32BIT_HACK = 0x7fffffff
} RTMEMTRACKERMETHOD;

/** Pointer to a tag structure. */
typedef struct RTMEMTRACKERTAG *PRTMEMTRACKERTAG;

/** Pointer to a user structure. */
typedef struct RTMEMTRACKERUSER *PRTMEMTRACKERUSER;

/**
 * Memory Tracking Header for use with RTMemTrackerHdrAlloc,
 * RTMemTrackerHdrReallocPrep, RTMemTrackerHdrReallocDone and
 * RTMemTrackerHdrFree.
 */
typedef struct RTMEMTRACKERHDR
{
    /** Magic value / eye catcher (RTMEMTRACKERHDR_MAGIC). */
    size_t              uMagic;
    /** The allocation size, user data only.  */
    size_t              cbUser;
    /** The list entry. */
    RTLISTNODE          ListEntry;
    /** Pointer to the user structure where this header is linked. */
    PRTMEMTRACKERUSER   pUser;
    /** Pointer to the per-tag structure. */
    PRTMEMTRACKERTAG    pTag;
    /** The tag string. */
    const char         *pszTag;
    /** The caller address. */
    void               *pvCaller;
    /** Pointer to the user data we're tracking. */
    void               *pvUser;
    /** Alignment padding. */
    size_t              uReserved;
} RTMEMTRACKERHDR;
/** Pointer to a memory tracker header. */
typedef RTMEMTRACKERHDR *PRTMEMTRACKERHDR;
/** Pointer to a const memory tracker header. */
typedef RTMEMTRACKERHDR *PPRTMEMTRACKERHDR;

/** Magic value for RTMEMTRACKERHDR::uMagic (Kelly Link). */
#if ARCH_BITS == 64
# define RTMEMTRACKERHDR_MAGIC          UINT64_C(0x1907691919690719)
#else
# define RTMEMTRACKERHDR_MAGIC          UINT32_C(0x19690719)
#endif
/** Magic number used when reallocated. */
#if ARCH_BITS == 64
# define RTMEMTRACKERHDR_MAGIC_REALLOC  UINT64_C(0x0000691919690000)
#else
# define RTMEMTRACKERHDR_MAGIC_REALLOC  UINT32_C(0x19690000)
#endif
/** Magic number used when freed. */
#define RTMEMTRACKERHDR_MAGIC_FREE      (~RTMEMTRACKERHDR_MAGIC)


/**
 * Initializes the allocation header and links it to the relevant tag.
 *
 * @returns Pointer to the user data part.
 * @param   pv                  The header + user data block.  This must be at
 *                              least @a cb + sizeof(RTMEMTRACKERHDR).
 * @param   cbUser              The user data size (bytes).
 * @param   pszTag              The tag string.
 * @param   pvCaller            The return address.
 * @param   enmMethod           The method that the user called.
 */
RTDECL(void *) RTMemTrackerHdrAlloc(void *pv, size_t cbUser, const char *pszTag, void *pvCaller, RTMEMTRACKERMETHOD enmMethod);

/**
 * Prepares for a realloc, i.e. invalidates the header.
 *
 * @returns Pointer to the user data part.
 * @param   pvOldUser           Pointer to the old user data.
 * @param   cbOldUser           The size of the old user data, 0 if not
 *                              known.
 * @param   pszTag              The tag string.
 * @param   pvCaller            The return address.
 */
RTDECL(void *) RTMemTrackerHdrReallocPrep(void *pvOldUser, size_t cbOldUser, const char *pszTag, void *pvCaller);

/**
 * Initializes the allocation header and links it to the relevant tag.
 *
 * @returns Pointer to the user data part.
 * @param   pvNew               The new header + user data block.  This must be
 *                              at least @a cb + sizeof(RTMEMTRACKERHDR).  If
 *                              this is NULL, we assume the realloc() call
 *                              failed.
 * @param   cbNewUser           The user data size (bytes).
 * @param   pvOldUser           Pointer to the old user data.  This is only
 *                              valid on failure of course and used to bail out
 *                              in that case.  Should not be NULL.
 * @param   pszTag              The tag string.
 * @param   pvCaller            The return address.
 */
RTDECL(void *) RTMemTrackerHdrReallocDone(void *pvNew, size_t cbNewUser, void *pvOldUser, const char *pszTag, void *pvCaller);


/**
 * Do the accounting on free.
 *
 * @returns @a pv.
 * @param   pvUser              Pointer to the user data.
 * @param   cbUser              The size of the user data, 0 if not known.
 * @param   pszTag              The tag string.
 * @param   pvCaller            The return address.
 * @param   enmMethod           The method that the user called.
 */
RTDECL(void *) RTMemTrackerHdrFree(void *pvUser, size_t cbUser, const char *pszTag, void *pvCaller, RTMEMTRACKERMETHOD enmMethod);


/**
 * Dumps all the allocations and tag statistics to the log.
 */
RTDECL(void) RTMemTrackerDumpAllToLog(void);

/**
 * Dumps all the allocations and tag statistics to the release log.
 */
RTDECL(void) RTMemTrackerDumpAllToLogRel(void);

/**
 * Dumps all the allocations and tag statistics to standard out.
 */
RTDECL(void) RTMemTrackerDumpAllToStdOut(void);

/**
 * Dumps all the allocations and tag statistics to standard err.
 */
RTDECL(void) RTMemTrackerDumpAllToStdErr(void);

/**
 * Dumps all the allocations and tag statistics to the specified filename.
 */
RTDECL(void) RTMemTrackerDumpAllToFile(const char *pszFilename);


/**
 * Dumps all the tag statistics to the log.
 *
 * @param   fVerbose        Whether to print all the stats or just the ones
 *                          relevant to hunting leaks.
 */
RTDECL(void) RTMemTrackerDumpStatsToLog(bool fVerbose);

/**
 * Dumps all the tag statistics to the release log.
 *
 * @param   fVerbose        Whether to print all the stats or just the ones
 *                          relevant to hunting leaks.
 */
RTDECL(void) RTMemTrackerDumpStatsToLogRel(bool fVerbose);

/**
 * Dumps all the tag statistics to standard out.
 *
 * @param   fVerbose        Whether to print all the stats or just the ones
 *                          relevant to hunting leaks.
 */
RTDECL(void) RTMemTrackerDumpStatsToStdOut(bool fVerbose);

/**
 * Dumps all the tag statistics to standard err.
 *
 * @param   fVerbose        Whether to print all the stats or just the ones
 *                          relevant to hunting leaks.
 */
RTDECL(void) RTMemTrackerDumpStatsToStdErr(bool fVerbose);

/**
 * Dumps all the tag statistics to the specified filename.
 *
 * @param   fVerbose        Whether to print all the stats or just the ones
 *                          relevant to hunting leaks.
 * @param   pszFilename         The name of the file to dump to.
 */
RTDECL(void) RTMemTrackerDumpStatsToFile(bool fVerbose, const char *pszFilename);



/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_memtracker_h */

