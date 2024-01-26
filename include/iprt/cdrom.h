/** @file
 * IPRT CD/DVD/BD-ROM Drive API.
 */

/*
 * Copyright (C) 2012-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_cdrom_h
#define IPRT_INCLUDED_cdrom_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_cdrom  IPRT CD/DVD/BD-ROM Drive API
 * @ingroup grp_rt
 *
 * The user of the API is currently resposible for serializing calls to it.
 *
 * @{
 */

/** CD-ROM drive handle. */
typedef struct RTCDROMINT  *RTCDROM;
/** Pointer to a CD-ROM handle. */
typedef RTCDROM            *PRTCDROM;
/** NIL CD-ROM handle value. */
#define NIL_RTCDROM         ((RTCDROM)0)


/** @name CD-ROM open flags.
 * @{ */
#define RTCDROM_O_READ              RT_BIT(0)
#define RTCDROM_O_WRITE             RT_BIT(1)
#define RTCDROM_O_CONTROL           RT_BIT(2)
#define RTCDROM_O_QUERY             RT_BIT(3)
#define RTCDROM_O_ALL_ACCESS        (RTCDROM_O_READ | RTCDROM_O_WRITE | RTCDROM_O_CONTROL | RTCDROM_O_QUERY)
/** @}  */

/**
 * Opens the CD-ROM drive (by name).
 *
 * @returns IPRT status code.
 * @param   pszName             The CD-ROM name (path).
 * @param   fFlags              Open flags, see RTCDROM_O_XXX.
 * @param   phCdrom             Where to return the CDROM handle.
 */
RTDECL(int)         RTCdromOpen(const char *pszName, uint32_t fFlags, PRTCDROM phCdrom);

/**
 * Retains a reference to the CD-ROM handle.
 *
 * @returns New reference count, UINT32_MAX on invalid handle (asserted).
 * @param   hCdrom              The CD-ROM handle to retain.
 */
RTDECL(uint32_t)    RTCdromRetain(RTCDROM hCdrom);

/**
 * Releases a reference to the CD-ROM handle.
 *
 * When the reference count reaches zero, the CD-ROM handle is destroy.
 *
 * @returns New reference count, UINT32_MAX on invalid handle (asserted).
 * @param   hCdrom              The CD-ROM handle to retain.
 */
RTDECL(uint32_t)    RTCdromRelease(RTCDROM hCdrom);

/**
 * Query the primary mount point of the CD-ROM.
 *
 * @returns IPRT status code.
 * @retval  VERR_BUFFER_OVERFLOW if the buffer is too small.  The buffer will be
 *          set to an empty string if possible.
 *
 * @param   hCdrom              The CD-ROM handle.
 * @param   pszMountPoint       Where to return the mount point.
 * @param   cbMountPoint        The size of the mount point buffer.
 */
RTDECL(int)         RTCdromQueryMountPoint(RTCDROM hCdrom, char *pszMountPoint, size_t cbMountPoint);

/**
 * Unmounts all file-system mounts related to the CD-ROM.
 *
 * @returns IPRT status code.
 * @param   hCdrom              The CD-ROM handle.
 */
RTDECL(int)         RTCdromUnmount(RTCDROM hCdrom);

/**
 * Ejects the CD-ROM from the drive.
 *
 * @returns IPRT status code.
 * @param   hCdrom              The CD-ROM handle.
 * @param   fForce              If set, unmount and unlock will be performed.
 */
RTDECL(int)         RTCdromEject(RTCDROM hCdrom, bool fForce);

/**
 * Locks the CD-ROM so it cannot be ejected by the user or system.
 *
 * @returns IPRT status code.
 * @param   hCdrom              The CD-ROM handle.
 */
RTDECL(int)         RTCdromLock(RTCDROM hCdrom);

/**
 * Unlocks the CD-ROM so it can be ejected by the user or system.
 *
 * @returns IPRT status code.
 * @param   hCdrom              The CD-ROM handle.
 */
RTDECL(int)         RTCdromUnlock(RTCDROM hCdrom);


/** @name Ordinal / Enumeration
 * @{ */
/**
 * Get the current number of CD-ROMs.
 *
 * This is handy for using RTCdromOpenByOrdinal() or RTCdromOrdinalToName() to
 * perform some kind of enumeration of all drives.
 *
 * @returns Number of CD-ROM drivers in the system.
 */
RTDECL(unsigned)    RTCdromCount(void);

/**
 * Translates an CD-ROM drive ordinal number to a path suitable for RTCdromOpen.
 *
 * @returns IRPT status code.
 * @retval  VINF_SUCCESS on success, with the name in the buffer.
 * @retval  VERR_BUFFER_OVERFLOW if the buffer is too small.  The buffer will be
 *          set to an empty string if possible, in order to prevent trouble.
 * @retval  VERR_OUT_OF_RANGE if the ordinal number is higher than the current
 *          number of CD-ROM drives.
 *
 * @param   iCdrom              The CD-ROM drive ordinal.  Starts at 0.
 * @param   pszName             Where to return the name (path).
 * @param   cbName              Size of the output buffer.
 *
 * @remarks The ordinals are volatile.  They may change as drives are attached
 *          or detected from the host.
 */
RTDECL(int)         RTCdromOrdinalToName(unsigned iCdrom, char *pszName, size_t cbName);

/**
 * Combination of RTCdromOrdinalToName() and RTCdromOpen().
 *
 * @returns IPRT status code.
 * @param   iCdrom              The CD-ROM number.
 * @param   fFlags              Open flags, see RTCDROM_O_XXX.
 * @param   phCdrom             Where to return the CDROM handle .
 * @remarks See remarks on RTCdromOrdinalToName().
 */
RTDECL(int)         RTCdromOpenByOrdinal(unsigned iCdrom, uint32_t fFlags, PRTCDROM phCdrom);

/** @} */

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_cdrom_h */

