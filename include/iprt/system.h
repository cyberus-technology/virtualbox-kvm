/** @file
 * IPRT - System Information.
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

#ifndef IPRT_INCLUDED_system_h
#define IPRT_INCLUDED_system_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_system RTSystem - System Information
 * @ingroup grp_rt
 * @{
 */

/**
 * Info level for RTSystemGetOSInfo().
 */
typedef enum RTSYSOSINFO
{
    RTSYSOSINFO_INVALID = 0,    /**< The usual invalid entry. */
    RTSYSOSINFO_PRODUCT,        /**< OS product name. (uname -o) */
    RTSYSOSINFO_RELEASE,        /**< OS release. (uname -r) */
    RTSYSOSINFO_VERSION,        /**< OS version, optional. (uname -v) */
    RTSYSOSINFO_SERVICE_PACK,   /**< Service/fix pack level, optional. */
    RTSYSOSINFO_END             /**< End of the valid info levels. */
} RTSYSOSINFO;


/**
 * Queries information about the OS.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_INVALID_PARAMETER if enmInfo is invalid.
 * @retval  VERR_INVALID_POINTER if pszInfoStr is invalid.
 * @retval  VERR_BUFFER_OVERFLOW if the buffer is too small. The buffer will
 *          contain the chopped off result in this case, provided cchInfo isn't 0.
 * @retval  VERR_NOT_SUPPORTED if the info level isn't implemented. The buffer will
 *          contain an empty string.
 *
 * @param   enmInfo         The OS info level.
 * @param   pszInfo         Where to store the result.
 * @param   cchInfo         The size of the output buffer.
 */
RTDECL(int) RTSystemQueryOSInfo(RTSYSOSINFO enmInfo, char *pszInfo, size_t cchInfo);

/**
 * Queries the total amount of RAM in the system.
 *
 * This figure does not given any information about how much memory is
 * currently available. Use RTSystemQueryAvailableRam instead.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS and *pcb on sucess.
 * @retval  VERR_ACCESS_DENIED if the information isn't accessible to the
 *          caller.
 *
 * @param   pcb             Where to store the result (in bytes).
 */
RTDECL(int) RTSystemQueryTotalRam(uint64_t *pcb);

/**
 * Queries the total amount of RAM accessible to the system.
 *
 * This figure should not include memory that is installed but not used,
 * nor memory that will be slow to bring online. The definition of 'slow'
 * here is slower than swapping out a MB of pages to disk.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS and *pcb on success.
 * @retval  VERR_ACCESS_DENIED if the information isn't accessible to the
 *          caller.
 *
 * @param   pcb             Where to store the result (in bytes).
 */
RTDECL(int) RTSystemQueryAvailableRam(uint64_t *pcb);

/**
 * Queries the amount of RAM that is currently locked down or in some other
 * way made impossible to virtualize within reasonably short time.
 *
 * The purposes of this API is, when combined with RTSystemQueryTotalRam, to
 * be able to determine an absolute max limit for how much fixed memory it is
 * (theoretically) possible to allocate (or lock down).
 *
 * The kind memory covered by this function includes:
 *      - locked (wired) memory - like for instance RTR0MemObjLockUser
 *        and RTR0MemObjLockKernel makes,
 *      - kernel pools and heaps - like for instance the ring-0 variant
 *        of RTMemAlloc taps into,
 *      - fixed (not pageable) kernel allocations - like for instance
 *        all the RTR0MemObjAlloc* functions makes,
 *      - any similar memory that isn't easily swapped out, discarded,
 *        or flushed to disk.
 *
 * This works against the value returned by RTSystemQueryTotalRam, and
 * the value reported by this function can never be larger than what a
 * call to RTSystemQueryTotalRam returns.
 *
 * The short time term here is relative to swapping to disk like in
 * RTSystemQueryTotalRam. This could mean that (part of) the dirty buffers
 * in the dynamic I/O cache could be included in the total. If the dynamic
 * I/O cache isn't likely to either flush buffers when the load increases
 * and put them back into normal circulation, they should be included in
 * the memory accounted for here.
 *
 * @retval  VINF_SUCCESS and *pcb on success.
 * @retval  VERR_NOT_SUPPORTED if the information isn't available on the
 *          system in general. The caller must handle this scenario.
 * @retval  VERR_ACCESS_DENIED if the information isn't accessible to the
 *          caller.
 *
 * @param   pcb             Where to store the result (in bytes).
 *
 * @remarks This function could've been inverted and called
 *          RTSystemQueryAvailableRam, but that might give impression that
 *          it would be possible to allocate the amount of memory it
 *          indicates for a single purpose, something which would be very
 *          improbable on most systems.
 *
 * @remarks We might have to add another output parameter to this function
 *          that indicates if some of the memory kinds listed above cannot
 *          be accounted for on the system and therefore is not include in
 *          the returned amount.
 */
RTDECL(int) RTSystemQueryUnavailableRam(uint64_t *pcb);


/**
 * The DMI strings.
 */
typedef enum RTSYSDMISTR
{
    /** Invalid zero entry. */
    RTSYSDMISTR_INVALID = 0,
    /** The product name. */
    RTSYSDMISTR_PRODUCT_NAME,
    /** The product version. */
    RTSYSDMISTR_PRODUCT_VERSION,
    /** The product UUID. */
    RTSYSDMISTR_PRODUCT_UUID,
    /** The product serial. */
    RTSYSDMISTR_PRODUCT_SERIAL,
    /** The system manufacturer. */
    RTSYSDMISTR_MANUFACTURER,
    /** The end of the valid strings. */
    RTSYSDMISTR_END,
    /** The usual 32-bit hack.  */
    RTSYSDMISTR_32_BIT_HACK = 0x7fffffff
} RTSYSDMISTR;

/**
 * Queries a DMI string.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_BUFFER_OVERFLOW if the buffer is too small.  The buffer will
 *          contain the chopped off result in this case, provided cbBuf isn't 0.
 * @retval  VERR_ACCESS_DENIED if the information isn't accessible to the
 *          caller.
 * @retval  VERR_NOT_SUPPORTED if the information isn't available on the system
 *          in general.  The caller must expect this status code and deal with
 *          it.
 *
 * @param   enmString           Which string to query.
 * @param   pszBuf              Where to store the string.  This is always
 *                              terminated, even on error.
 * @param   cbBuf               The buffer size.
 */
RTDECL(int) RTSystemQueryDmiString(RTSYSDMISTR enmString, char *pszBuf, size_t cbBuf);

/** @name Flags for RTSystemReboot and RTSystemShutdown.
 * @{ */
/** Reboot the system after shutdown. */
#define RTSYSTEM_SHUTDOWN_REBOOT            UINT32_C(0)
/** Reboot the system after shutdown.
 * The call may return VINF_SYS_MAY_POWER_OFF if the OS /
 * hardware combination may power off instead of halting. */
#define RTSYSTEM_SHUTDOWN_HALT              UINT32_C(1)
/** Power off the system after shutdown.
 * This may be equvivalent to a RTSYSTEM_SHUTDOWN_HALT on systems where we
 * cannot figure out whether the hardware/OS implements the actual powering
 * off.  If we can figure out that it's not supported, an
 * VERR_SYS_CANNOT_POWER_OFF error is raised. */
#define RTSYSTEM_SHUTDOWN_POWER_OFF         UINT32_C(2)
/** Power off the system after shutdown, or halt it if that's not possible. */
#define RTSYSTEM_SHUTDOWN_POWER_OFF_HALT    UINT32_C(3)
/** The shutdown action mask. */
#define RTSYSTEM_SHUTDOWN_ACTION_MASK       UINT32_C(3)
/** Unplanned shutdown/reboot. */
#define RTSYSTEM_SHUTDOWN_UNPLANNED         UINT32_C(0)
/** Planned shutdown/reboot. */
#define RTSYSTEM_SHUTDOWN_PLANNED           RT_BIT_32(2)
/** Force the system to shutdown/reboot regardless of objecting application
 *  or other stuff.  This flag might not be realized on all systems. */
#define RTSYSTEM_SHUTDOWN_FORCE             RT_BIT_32(3)
/** Parameter validation mask. */
#define RTSYSTEM_SHUTDOWN_VALID_MASK        UINT32_C(0x0000000f)
/** @} */

/**
 * Shuts down the system.
 *
 * @returns IPRT status code on failure, on success it may or may not return
 *          depending on the OS.
 * @retval  VINF_SUCCESS
 * @retval  VINF_SYS_MAY_POWER_OFF
 * @retval  VERR_SYS_SHUTDOWN_FAILED
 * @retval  VERR_SYS_CANNOT_POWER_OFF
 *
 * @param   cMsDelay            The delay before the actual reboot.  If this is
 *                              not supported by the OS, an immediate reboot
 *                              will be performed.
 * @param   fFlags              Shutdown flags, see RTSYSTEM_SHUTDOWN_XXX.
 * @param   pszLogMsg           Message for the log and users about why we're
 *                              shutting down.
 */
RTDECL(int) RTSystemShutdown(RTMSINTERVAL cMsDelay, uint32_t fFlags, const char *pszLogMsg);

/**
 * Checks if we're executing inside a virtual machine (VM).
 *
 * The current implemention is very simplistic and won't try to detect the
 * presence of a virtual machine monitor (VMM) unless it openly tells us it is
 * there.
 *
 * @returns true if inside a VM, false if on real hardware.
 *
 * @todo    If more information is needed, like which VMM it is and which
 *          version and such, add one or two new APIs.
 */
RTDECL(bool) RTSystemIsInsideVM(void);

/**
 * System firmware types.
 */
typedef enum RTSYSFWTYPE
{
    /** Invalid zero value. */
    RTSYSFWTYPE_INVALID = 0,
    /** Unknown firmware. */
    RTSYSFWTYPE_UNKNOWN,
    /** Firmware is BIOS. */
    RTSYSFWTYPE_BIOS,
    /** Firmware is UEFI. */
    RTSYSFWTYPE_UEFI,
    /** End valid firmware values (exclusive).  */
    RTSYSFWTYPE_END,
    /** The usual 32-bit hack.  */
    RTSYSFWTYPE_32_BIT_HACK = 0x7fffffff
} RTSYSFWTYPE;
/** Pointer to a system firmware type. */
typedef RTSYSFWTYPE *PRTSYSFWTYPE;

/**
 * Queries the system's firmware type.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SUPPORTED if not supported or implemented.
 * @param   penmType    Where to return the firmware type on success.
 */
RTDECL(int) RTSystemQueryFirmwareType(PRTSYSFWTYPE penmType);

/**
 * Translates the @a enmType value to a string.
 *
 * @returns Read-only name.
 * @param   enmType     The firmware type to convert to string.
 */
RTDECL(const char *) RTSystemFirmwareTypeName(RTSYSFWTYPE enmType);

/**
 * Boolean firmware values queriable via RTSystemQueryFirmwareBoolean().
 */
typedef enum RTSYSFWBOOL
{
    /** Invalid property, do not use. */
    RTSYSFWBOOL_INVALID = 0,
    /** Whether Secure Boot is enabled or not (type: boolean). */
    RTSYSFWBOOL_SECURE_BOOT,
    /** End of valid    */
    RTSYSFWBOOL_END,
    /** The usual 32-bit hack.  */
    RTSYSFWBOOL_32_BIT_HACK = 0x7fffffff
} RTSYSFWBOOL;

/**
 * Queries the value of a firmware property.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SUPPORTED if we cannot query firmware properties on the host.
 * @retval  VERR_SYS_UNSUPPORTED_FIRMWARE_PROPERTY if @a enmBoolean isn't
 *          supported.
 * @param   enmBoolean  The value to query.
 * @param   pfValue     Where to return the value.
 */
RTDECL(int) RTSystemQueryFirmwareBoolean(RTSYSFWBOOL enmBoolean, bool *pfValue);

#ifdef RT_OS_WINDOWS

/**
 * Get the Windows NT build number.
 *
 * @returns NT build number.
 *
 * @remarks Windows NT only.  Requires IPRT to be initialized.
 */
RTDECL(uint32_t) RTSystemGetNtBuildNo(void);

/** Makes an NT version for comparison with RTSystemGetNtVersion(). */
# define RTSYSTEM_MAKE_NT_VERSION(a_uMajor, a_uMinor, a_uBuild) \
    ( ((uint64_t)(a_uMajor) << 52) | ((uint64_t)((a_uMinor) & 0xfffU) << 40) | ((uint32_t)(a_uBuild)) )
/** Extracts the major version number from a RTSYSTEM_MAKE_NT_VERSION value. */
# define RTSYSTEM_NT_VERSION_GET_MAJOR(a_uNtVersion) ((uint32_t)((a_uNtVersion) >> 52))
/** Extracts the minor version number from a RTSYSTEM_MAKE_NT_VERSION value. */
# define RTSYSTEM_NT_VERSION_GET_MINOR(a_uNtVersion) ((uint32_t)((a_uNtVersion) >> 40) & UINT32_C(0xfff))
/** Extracts the build number from a RTSYSTEM_MAKE_NT_VERSION value. */
# define RTSYSTEM_NT_VERSION_GET_BUILD(a_uNtVersion) ((uint32_t)(a_uNtVersion))

/**
 * Get the Windows NT version number.
 *
 * @returns Version formatted using RTSYSTEM_MAKE_NT_VERSION().
 *
 * @remarks Windows NT only.  Requires IPRT to be initialized.
 */
RTDECL(uint64_t) RTSystemGetNtVersion(void);

/**
 * Get the Windows NT product type (OSVERSIONINFOW::wProductType).
 */
RTDECL(uint8_t) RTSystemGetNtProductType(void);

#endif /* RT_OS_WINDOWS */

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_system_h */

