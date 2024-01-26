/** @file
 * IPRT - Time.
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

#ifndef IPRT_INCLUDED_time_h
#define IPRT_INCLUDED_time_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/assertcompile.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_time   RTTime - Time
 * @ingroup grp_rt
 * @{
 */

/** Time Specification.
 *
 * Use the inline RTTimeSpecGet/Set to operate on structure this so we
 * can easily change the representation if required later.
 *
 * The current representation is in nanoseconds relative to the unix epoch
 * (1970-01-01 00:00:00 UTC). This gives us an approximate span from
 * 1678 to 2262 without sacrificing the resolution offered by the various
 * host OSes (BSD & LINUX 1ns, NT 100ns).
 */
typedef struct RTTIMESPEC
{
    /** Nanoseconds since epoch.
     * The name is intentially too long to be comfortable to use because you should be
     * using inline helpers! */
    int64_t     i64NanosecondsRelativeToUnixEpoch;
} RTTIMESPEC;


/** @name RTTIMESPEC methods
 * @{ */

/**
 * Gets the time as nanoseconds relative to the unix epoch.
 *
 * @returns Nanoseconds relative to unix epoch.
 * @param   pTime       The time spec to interpret.
 */
DECLINLINE(int64_t) RTTimeSpecGetNano(PCRTTIMESPEC pTime)
{
    return pTime->i64NanosecondsRelativeToUnixEpoch;
}


/**
 * Sets the time give by nanoseconds relative to the unix epoch.
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 * @param   i64Nano     The new time in nanoseconds.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecSetNano(PRTTIMESPEC pTime, int64_t i64Nano)
{
    pTime->i64NanosecondsRelativeToUnixEpoch = i64Nano;
    return pTime;
}


/**
 * Gets the time as microseconds relative to the unix epoch.
 *
 * @returns microseconds relative to unix epoch.
 * @param   pTime       The time spec to interpret.
 */
DECLINLINE(int64_t) RTTimeSpecGetMicro(PCRTTIMESPEC pTime)
{
    return pTime->i64NanosecondsRelativeToUnixEpoch / RT_NS_1US;
}


/**
 * Sets the time given by microseconds relative to the unix epoch.
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 * @param   i64Micro    The new time in microsecond.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecSetMicro(PRTTIMESPEC pTime, int64_t i64Micro)
{
    pTime->i64NanosecondsRelativeToUnixEpoch = i64Micro * RT_NS_1US;
    return pTime;
}


/**
 * Gets the time as milliseconds relative to the unix epoch.
 *
 * @returns milliseconds relative to unix epoch.
 * @param   pTime       The time spec to interpret.
 */
DECLINLINE(int64_t) RTTimeSpecGetMilli(PCRTTIMESPEC pTime)
{
    return pTime->i64NanosecondsRelativeToUnixEpoch / RT_NS_1MS;
}


/**
 * Sets the time given by milliseconds relative to the unix epoch.
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 * @param   i64Milli    The new time in milliseconds.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecSetMilli(PRTTIMESPEC pTime, int64_t i64Milli)
{
    pTime->i64NanosecondsRelativeToUnixEpoch = i64Milli * RT_NS_1MS;
    return pTime;
}


/**
 * Gets the time as seconds relative to the unix epoch.
 *
 * @returns seconds relative to unix epoch.
 * @param   pTime       The time spec to interpret.
 */
DECLINLINE(int64_t) RTTimeSpecGetSeconds(PCRTTIMESPEC pTime)
{
    return pTime->i64NanosecondsRelativeToUnixEpoch / RT_NS_1SEC;
}


/**
 * Sets the time given by seconds relative to the unix epoch.
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 * @param   i64Seconds  The new time in seconds.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecSetSeconds(PRTTIMESPEC pTime, int64_t i64Seconds)
{
    pTime->i64NanosecondsRelativeToUnixEpoch = i64Seconds * RT_NS_1SEC;
    return pTime;
}


/**
 * Makes the time spec absolute like abs() does (i.e. a positive value).
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecAbsolute(PRTTIMESPEC pTime)
{
    if (pTime->i64NanosecondsRelativeToUnixEpoch < 0)
        pTime->i64NanosecondsRelativeToUnixEpoch = -pTime->i64NanosecondsRelativeToUnixEpoch;
    return pTime;
}


/**
 * Negates the time.
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecNegate(PRTTIMESPEC pTime)
{
    pTime->i64NanosecondsRelativeToUnixEpoch = -pTime->i64NanosecondsRelativeToUnixEpoch;
    return pTime;
}


/**
 * Adds a time period to the time.
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 * @param   pTimeAdd    The time spec to add to pTime.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecAdd(PRTTIMESPEC pTime, PCRTTIMESPEC pTimeAdd)
{
    pTime->i64NanosecondsRelativeToUnixEpoch += pTimeAdd->i64NanosecondsRelativeToUnixEpoch;
    return pTime;
}


/**
 * Adds a time period give as nanoseconds from the time.
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 * @param   i64Nano     The time period in nanoseconds.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecAddNano(PRTTIMESPEC pTime, int64_t i64Nano)
{
    pTime->i64NanosecondsRelativeToUnixEpoch += i64Nano;
    return pTime;
}


/**
 * Adds a time period give as microseconds from the time.
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 * @param   i64Micro    The time period in microseconds.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecAddMicro(PRTTIMESPEC pTime, int64_t i64Micro)
{
    pTime->i64NanosecondsRelativeToUnixEpoch += i64Micro * RT_NS_1US;
    return pTime;
}


/**
 * Adds a time period give as milliseconds from the time.
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 * @param   i64Milli    The time period in milliseconds.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecAddMilli(PRTTIMESPEC pTime, int64_t i64Milli)
{
    pTime->i64NanosecondsRelativeToUnixEpoch += i64Milli * RT_NS_1MS;
    return pTime;
}


/**
 * Adds a time period give as seconds from the time.
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 * @param   i64Seconds  The time period in seconds.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecAddSeconds(PRTTIMESPEC pTime, int64_t i64Seconds)
{
    pTime->i64NanosecondsRelativeToUnixEpoch += i64Seconds * RT_NS_1SEC;
    return pTime;
}


/**
 * Subtracts a time period from the time.
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 * @param   pTimeSub    The time spec to subtract from pTime.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecSub(PRTTIMESPEC pTime, PCRTTIMESPEC pTimeSub)
{
    pTime->i64NanosecondsRelativeToUnixEpoch -= pTimeSub->i64NanosecondsRelativeToUnixEpoch;
    return pTime;
}


/**
 * Subtracts a time period give as nanoseconds from the time.
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 * @param   i64Nano     The time period in nanoseconds.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecSubNano(PRTTIMESPEC pTime, int64_t i64Nano)
{
    pTime->i64NanosecondsRelativeToUnixEpoch -= i64Nano;
    return pTime;
}


/**
 * Subtracts a time period give as microseconds from the time.
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 * @param   i64Micro    The time period in microseconds.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecSubMicro(PRTTIMESPEC pTime, int64_t i64Micro)
{
    pTime->i64NanosecondsRelativeToUnixEpoch -= i64Micro * RT_NS_1US;
    return pTime;
}


/**
 * Subtracts a time period give as milliseconds from the time.
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 * @param   i64Milli    The time period in milliseconds.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecSubMilli(PRTTIMESPEC pTime, int64_t i64Milli)
{
    pTime->i64NanosecondsRelativeToUnixEpoch -= i64Milli * RT_NS_1MS;
    return pTime;
}


/**
 * Subtracts a time period give as seconds from the time.
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 * @param   i64Seconds  The time period in seconds.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecSubSeconds(PRTTIMESPEC pTime, int64_t i64Seconds)
{
    pTime->i64NanosecondsRelativeToUnixEpoch -= i64Seconds * RT_NS_1SEC;
    return pTime;
}


/**
 * Gives the time in seconds and nanoseconds.
 *
 * @param   pTime           The time spec to interpret.
 * @param   *pi32Seconds    Where to store the time period in seconds.
 * @param   *pi32Nano       Where to store the time period in nanoseconds.
 */
DECLINLINE(void) RTTimeSpecGetSecondsAndNano(PRTTIMESPEC pTime, int32_t *pi32Seconds, int32_t *pi32Nano)
{
    int64_t i64 = RTTimeSpecGetNano(pTime);
    int32_t i32Nano = (int32_t)(i64 % RT_NS_1SEC);
    i64 /= RT_NS_1SEC;
    if (i32Nano < 0)
    {
        i32Nano += RT_NS_1SEC;
        i64--;
    }
    *pi32Seconds = (int32_t)i64;
    *pi32Nano    = i32Nano;
}

/** @def RTTIME_LINUX_KERNEL_PREREQ
 * Prerequisite minimum linux kernel version.
 * @note Cannot really be moved to iprt/cdefs.h, see the-linux-kernel.h */
/** @def RTTIME_LINUX_KERNEL_PREREQ_LT
 * Prerequisite maxium linux kernel version (LT=less-than).
 * @note Cannot really be moved to iprt/cdefs.h, see the-linux-kernel.h */
#if defined(RT_OS_LINUX) && defined(LINUX_VERSION_CODE) && defined(KERNEL_VERSION)
# define RTTIME_LINUX_KERNEL_PREREQ(a, b, c)    (LINUX_VERSION_CODE >= KERNEL_VERSION(a, b, c))
# define RTTIME_LINUX_KERNEL_PREREQ_LT(a, b, c) (!RTTIME_LINUX_KERNEL_PREREQ(a, b, c))
#else
# define RTTIME_LINUX_KERNEL_PREREQ(a, b, c)    0
# define RTTIME_LINUX_KERNEL_PREREQ_LT(a, b, c) 0
#endif

/* PORTME: Add struct timeval guard macro here. */
#if defined(RTTIME_INCL_TIMEVAL) \
 || defined(_SYS__TIMEVAL_H_) \
 || defined(_SYS_TIME_H) \
 || defined(_TIMEVAL) \
 || defined(_STRUCT_TIMEVAL) \
 || (   defined(RT_OS_LINUX) \
     && defined(_LINUX_TIME_H) \
     && (   !defined(__KERNEL__) \
         || RTTIME_LINUX_KERNEL_PREREQ_LT(5,6,0) /* @bugref{9757} */ ) ) \
 || (defined(RT_OS_NETBSD) && defined(_SYS_TIME_H_))

/**
 * Gets the time as POSIX timeval.
 *
 * @returns pTime.
 * @param   pTime       The time spec to interpret.
 * @param   pTimeval    Where to store the time as POSIX timeval.
 */
DECLINLINE(struct timeval *) RTTimeSpecGetTimeval(PCRTTIMESPEC pTime, struct timeval *pTimeval)
{
    int64_t i64 = RTTimeSpecGetMicro(pTime);
    int32_t i32Micro = (int32_t)(i64 % RT_US_1SEC);
    i64 /= RT_US_1SEC;
    if (i32Micro < 0)
    {
        i32Micro += RT_US_1SEC;
        i64--;
    }
    pTimeval->tv_sec = (time_t)i64;
    pTimeval->tv_usec = i32Micro;
    return pTimeval;
}

/**
 * Sets the time as POSIX timeval.
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 * @param   pTimeval    Pointer to the POSIX timeval struct with the new time.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecSetTimeval(PRTTIMESPEC pTime, const struct timeval *pTimeval)
{
    return RTTimeSpecAddMicro(RTTimeSpecSetSeconds(pTime, pTimeval->tv_sec), pTimeval->tv_usec);
}

#endif /* various ways of detecting struct timeval */


/* PORTME: Add struct timespec guard macro here. */
#if defined(RTTIME_INCL_TIMESPEC) \
 || defined(_SYS__TIMESPEC_H_) \
 || defined(TIMEVAL_TO_TIMESPEC) \
 || defined(_TIMESPEC) \
 || (   defined(_STRUCT_TIMESPEC) \
     && (   !defined(RT_OS_LINUX) \
         || !defined(__KERNEL__) \
         || RTTIME_LINUX_KERNEL_PREREQ_LT(5,6,0) /* @bugref{9757} */ ) ) \
 || (defined(RT_OS_NETBSD) && defined(_SYS_TIME_H_))

/**
 * Gets the time as POSIX timespec.
 *
 * @returns pTimespec.
 * @param   pTime       The time spec to interpret.
 * @param   pTimespec   Where to store the time as POSIX timespec.
 */
DECLINLINE(struct timespec *) RTTimeSpecGetTimespec(PCRTTIMESPEC pTime, struct timespec *pTimespec)
{
    int64_t i64 = RTTimeSpecGetNano(pTime);
    int32_t i32Nano = (int32_t)(i64 % RT_NS_1SEC);
    i64 /= RT_NS_1SEC;
    if (i32Nano < 0)
    {
        i32Nano += RT_NS_1SEC;
        i64--;
    }
    pTimespec->tv_sec = (time_t)i64;
    pTimespec->tv_nsec = i32Nano;
    return pTimespec;
}

/**
 * Sets the time as POSIX timespec.
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 * @param   pTimespec   Pointer to the POSIX timespec struct with the new time.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecSetTimespec(PRTTIMESPEC pTime, const struct timespec *pTimespec)
{
    return RTTimeSpecAddNano(RTTimeSpecSetSeconds(pTime, pTimespec->tv_sec), pTimespec->tv_nsec);
}

#endif /* various ways of detecting struct timespec */


#if defined(RT_OS_LINUX) && defined(_LINUX_TIME64_H) /* since linux 3.17 */

/**
 * Gets the time a linux 64-bit timespec structure.
 * @returns pTimespec.
 * @param   pTime       The time spec to modify.
 * @param   pTimespec   Where to store the time as linux 64-bit timespec.
 */
DECLINLINE(struct timespec64 *) RTTimeSpecGetTimespec64(PCRTTIMESPEC pTime, struct timespec64 *pTimespec)
{
    int64_t i64 = RTTimeSpecGetNano(pTime);
    int32_t i32Nano = (int32_t)(i64 % RT_NS_1SEC);
    i64 /= RT_NS_1SEC;
    if (i32Nano < 0)
    {
        i32Nano += RT_NS_1SEC;
        i64--;
    }
    pTimespec->tv_sec = i64;
    pTimespec->tv_nsec = i32Nano;
    return pTimespec;
}

/**
 * Sets the time from a linux 64-bit timespec structure.
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 * @param   pTimespec   Pointer to the linux 64-bit timespec struct with the new time.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecSetTimespec64(PRTTIMESPEC pTime, const struct timespec64 *pTimespec)
{
    return RTTimeSpecAddNano(RTTimeSpecSetSeconds(pTime, pTimespec->tv_sec), pTimespec->tv_nsec);
}

#endif /* RT_OS_LINUX && _LINUX_TIME64_H */


/** The offset of the unix epoch and the base for NT time (in 100ns units).
 * Nt time starts at 1601-01-01 00:00:00. */
#define RTTIME_NT_TIME_OFFSET_UNIX      (116444736000000000LL)


/**
 * Gets the time as NT time.
 *
 * @returns NT time.
 * @param   pTime       The time spec to interpret.
 */
DECLINLINE(int64_t) RTTimeSpecGetNtTime(PCRTTIMESPEC pTime)
{
    return pTime->i64NanosecondsRelativeToUnixEpoch / 100
        + RTTIME_NT_TIME_OFFSET_UNIX;
}


/**
 * Sets the time given by Nt time.
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 * @param   u64NtTime   The new time in Nt time.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecSetNtTime(PRTTIMESPEC pTime, uint64_t u64NtTime)
{
    pTime->i64NanosecondsRelativeToUnixEpoch =
        ((int64_t)u64NtTime - RTTIME_NT_TIME_OFFSET_UNIX) * 100;
    return pTime;
}


#ifdef _FILETIME_

/**
 * Gets the time as NT file time.
 *
 * @returns pFileTime.
 * @param   pTime       The time spec to interpret.
 * @param   pFileTime   Pointer to NT filetime structure.
 */
DECLINLINE(PFILETIME) RTTimeSpecGetNtFileTime(PCRTTIMESPEC pTime, PFILETIME pFileTime)
{
    *((uint64_t *)pFileTime) = RTTimeSpecGetNtTime(pTime);
    return pFileTime;
}

/**
 * Sets the time as NT file time.
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 * @param   pFileTime   Where to store the time as Nt file time.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecSetNtFileTime(PRTTIMESPEC pTime, const FILETIME *pFileTime)
{
    return RTTimeSpecSetNtTime(pTime, *(const uint64_t *)pFileTime);
}

#endif /* _FILETIME_ */


/** The offset to the start of DOS time.
 * DOS time starts 1980-01-01 00:00:00.  */
#define RTTIME_OFFSET_DOS_TIME          (315532800000000000LL)


/**
 * Gets the time as seconds relative to the start of dos time.
 *
 * @returns seconds relative to the start of dos time.
 * @param   pTime       The time spec to interpret.
 */
DECLINLINE(int64_t) RTTimeSpecGetDosSeconds(PCRTTIMESPEC pTime)
{
    return (pTime->i64NanosecondsRelativeToUnixEpoch - RTTIME_OFFSET_DOS_TIME)
        / RT_NS_1SEC;
}


/**
 * Sets the time given by seconds relative to the start of dos time.
 *
 * @returns pTime.
 * @param   pTime       The time spec to modify.
 * @param   i64Seconds  The new time in seconds relative to the start of dos time.
 */
DECLINLINE(PRTTIMESPEC) RTTimeSpecSetDosSeconds(PRTTIMESPEC pTime, int64_t i64Seconds)
{
    pTime->i64NanosecondsRelativeToUnixEpoch = i64Seconds * RT_NS_1SEC
        + RTTIME_OFFSET_DOS_TIME;
    return pTime;
}


/**
 * Compare two time specs.
 *
 * @returns true they are equal.
 * @returns false they are not equal.
 * @param   pTime1  The 1st time spec.
 * @param   pTime2  The 2nd time spec.
 */
DECLINLINE(bool) RTTimeSpecIsEqual(PCRTTIMESPEC pTime1, PCRTTIMESPEC pTime2)
{
    return pTime1->i64NanosecondsRelativeToUnixEpoch == pTime2->i64NanosecondsRelativeToUnixEpoch;
}


/**
 * Compare two time specs.
 *
 * @returns 0 if equal, -1 if @a pLeft is smaller, 1 if @a pLeft is larger.
 * @returns false they are not equal.
 * @param   pLeft       The 1st time spec.
 * @param   pRight      The 2nd time spec.
 */
DECLINLINE(int) RTTimeSpecCompare(PCRTTIMESPEC pLeft, PCRTTIMESPEC pRight)
{
    if (pLeft->i64NanosecondsRelativeToUnixEpoch == pRight->i64NanosecondsRelativeToUnixEpoch)
        return 0;
    return pLeft->i64NanosecondsRelativeToUnixEpoch < pRight->i64NanosecondsRelativeToUnixEpoch ? -1 : 1;
}


/**
 * Converts a time spec to a ISO date string.
 *
 * @returns psz on success.
 * @returns NULL on buffer underflow.
 * @param   pTime       The time spec.
 * @param   psz         Where to store the string.
 * @param   cb          The size of the buffer.
 */
RTDECL(char *) RTTimeSpecToString(PCRTTIMESPEC pTime, char *psz, size_t cb);

/**
 * Attempts to convert an ISO date string to a time structure.
 *
 * We're a little forgiving with zero padding, unspecified parts, and leading
 * and trailing spaces.
 *
 * @retval  pTime on success,
 * @retval  NULL on failure.
 * @param   pTime       The time spec.
 * @param   pszString   The ISO date string to convert.
 */
RTDECL(PRTTIMESPEC) RTTimeSpecFromString(PRTTIMESPEC pTime, const char *pszString);

/**
 * Formats duration as best we can according to ISO-8601, with no fraction.
 *
 * See RTTimeFormatDurationEx for details.
 *
 * @returns Number of characters in the output on success. VERR_BUFFER_OVEFLOW
 *          on failure.
 * @param   pszDst          Pointer to the output buffer.  In case of overflow,
 *                          the max number of characters will be written and
 *                          zero terminated, provided @a cbDst isn't zero.
 * @param   cbDst           The size of the output buffer.
 * @param   pDuration       The duration to format.
 */
RTDECL(int) RTTimeFormatDuration(char *pszDst, size_t cbDst, PCRTTIMESPEC pDuration);

/**
 * Formats duration as best we can according to ISO-8601.
 *
 * The returned value is on the form "[-]PnnnnnWnDTnnHnnMnn.fffffffffS", where a
 * sequence of 'n' can be between 1 and the given lenght, and all but the
 * "nn.fffffffffS" part is optional and will only be outputted when the duration
 * is sufficiently large.  The code currently does not omit any inbetween
 * elements other than the day count (D), so an exactly 7 day duration is
 * formatted as "P1WT0H0M0.000000000S" when @a cFractionDigits is 9.
 *
 * @returns Number of characters in the output on success. VERR_BUFFER_OVEFLOW
 *          on failure.
 * @retval  VERR_OUT_OF_RANGE if @a cFractionDigits is too large.
 * @param   pszDst          Pointer to the output buffer.  In case of overflow,
 *                          the max number of characters will be written and
 *                          zero terminated, provided @a cbDst isn't zero.
 * @param   cbDst           The size of the output buffer.
 * @param   pDuration       The duration to format.
 * @param   cFractionDigits Number of digits in the second fraction part. Zero
 *                          for whole no fraction. Max is 9 (nano seconds).
 */
RTDECL(ssize_t) RTTimeFormatDurationEx(char *pszDst, size_t cbDst, PCRTTIMESPEC pDuration, uint32_t cFractionDigits);

/** Max length of a RTTimeFormatDurationEx output string. */
#define RTTIME_DURATION_STR_LEN     (sizeof("-P99999W7D23H59M59.123456789S") + 2)

/** @} */


/**
 * Exploded time.
 */
typedef struct RTTIME
{
    /** The year number. */
    int32_t     i32Year;
    /** The month of the year (1-12). January is 1. */
    uint8_t     u8Month;
    /** The day of the week (0-6). Monday is 0. */
    uint8_t     u8WeekDay;
    /** The day of the year (1-366). January the 1st is 1. */
    uint16_t    u16YearDay;
    /** The day of the month (1-31). */
    uint8_t     u8MonthDay;
    /** Hour of the day (0-23). */
    uint8_t     u8Hour;
    /** The minute of the hour (0-59). */
    uint8_t     u8Minute;
    /** The second of the minute (0-60).
     * (u32Nanosecond / 1000000) */
    uint8_t     u8Second;
    /** The nanoseconds of the second (0-999999999). */
    uint32_t    u32Nanosecond;
    /** Flags, of the RTTIME_FLAGS_* \#defines. */
    uint32_t    fFlags;
    /** UTC time offset in minutes (-840-840).  Positive for timezones east of
     * UTC, negative for zones to the west.  Same as what RTTimeLocalDeltaNano
     * & RTTimeLocalDeltaNanoFor returns, just different unit. */
    int32_t     offUTC;
} RTTIME;
AssertCompileSize(RTTIME, 24);
/** Pointer to a exploded time structure. */
typedef RTTIME *PRTTIME;
/** Pointer to a const exploded time structure. */
typedef const RTTIME *PCRTTIME;

/** @name RTTIME::fFlags values.
 * @{ */
/** Set if the time is UTC. If clear the time local time. */
#define RTTIME_FLAGS_TYPE_MASK      3
/** the time is UTC time. */
#define RTTIME_FLAGS_TYPE_UTC       2
/** The time is local time. */
#define RTTIME_FLAGS_TYPE_LOCAL     3

/** Set if the time is local and daylight saving time is in effect.
 * Not bit is not valid if RTTIME_FLAGS_NO_DST_DATA is set. */
#define RTTIME_FLAGS_DST            RT_BIT(4)
/** Set if the time is local and there is no data available on daylight saving time. */
#define RTTIME_FLAGS_NO_DST_DATA    RT_BIT(5)
/** Set if the year is a leap year.
 * This is mutual exclusiv with RTTIME_FLAGS_COMMON_YEAR. */
#define RTTIME_FLAGS_LEAP_YEAR      RT_BIT(6)
/** Set if the year is a common year.
 * This is mutual exclusiv with RTTIME_FLAGS_LEAP_YEAR. */
#define RTTIME_FLAGS_COMMON_YEAR    RT_BIT(7)
/** The mask of valid flags. */
#define RTTIME_FLAGS_MASK           UINT32_C(0xff)
/** @} */


/**
 * Gets the current system time (UTC).
 *
 * @returns pTime.
 * @param   pTime       Where to store the time.
 */
RTDECL(PRTTIMESPEC) RTTimeNow(PRTTIMESPEC pTime);

/**
 * Sets the system time.
 *
 * @returns IPRT status code
 * @param   pTime       The new system time (UTC).
 *
 * @remarks This will usually fail because changing the wall time is usually
 *          requires extra privileges.
 */
RTDECL(int) RTTimeSet(PCRTTIMESPEC pTime);

/**
 * Explodes a time spec (UTC).
 *
 * @returns pTime.
 * @param   pTime       Where to store the exploded time.
 * @param   pTimeSpec   The time spec to exploded.
 */
RTDECL(PRTTIME) RTTimeExplode(PRTTIME pTime, PCRTTIMESPEC pTimeSpec);

/**
 * Implodes exploded time to a time spec (UTC).
 *
 * @returns pTime on success.
 * @returns NULL if the pTime data is invalid.
 * @param   pTimeSpec   Where to store the imploded UTC time.
 *                      If pTime specifies a time which outside the range, maximum or
 *                      minimum values will be returned.
 * @param   pTime       Pointer to the exploded time to implode.
 *                      The fields u8Month, u8WeekDay and u8MonthDay are not used,
 *                      and all the other fields are expected to be within their
 *                      bounds. Use RTTimeNormalize() to calculate u16YearDay and
 *                      normalize the ranges of the fields.
 */
RTDECL(PRTTIMESPEC) RTTimeImplode(PRTTIMESPEC pTimeSpec, PCRTTIME pTime);

/**
 * Normalizes the fields of a time structure.
 *
 * It is possible to calculate year-day from month/day and vice
 * versa. If you adjust any of of these, make sure to zero the
 * other so you make it clear which of the fields to use. If
 * it's ambiguous, the year-day field is used (and you get
 * assertions in debug builds).
 *
 * All the time fields and the year-day or month/day fields will
 * be adjusted for overflows. (Since all fields are unsigned, there
 * is no underflows.) It is possible to exploit this for simple
 * date math, though the recommended way of doing that to implode
 * the time into a timespec and do the math on that.
 *
 * @returns pTime on success.
 * @returns NULL if the data is invalid.
 *
 * @param   pTime       The time structure to normalize.
 *
 * @remarks This function doesn't work with local time, only with UTC time.
 */
RTDECL(PRTTIME) RTTimeNormalize(PRTTIME pTime);

/**
 * Gets the current local system time.
 *
 * @returns pTime.
 * @param   pTime   Where to store the local time.
 */
RTDECL(PRTTIMESPEC) RTTimeLocalNow(PRTTIMESPEC pTime);

/**
 * Gets the current delta between UTC and local time.
 *
 * @code
 *      RTTIMESPEC LocalTime;
 *      RTTimeSpecAddNano(RTTimeNow(&LocalTime), RTTimeLocalDeltaNano());
 * @endcode
 *
 * @returns Returns the nanosecond delta between UTC and local time.
 */
RTDECL(int64_t) RTTimeLocalDeltaNano(void);

/**
 * Gets the delta between UTC and local time at the given time.
 *
 * @code
 *      RTTIMESPEC LocalTime;
 *      RTTimeNow(&LocalTime);
 *      RTTimeSpecAddNano(&LocalTime, RTTimeLocalDeltaNanoFor(&LocalTime));
 * @endcode
 *
 * @param   pTimeSpec   The time spec giving the time to get the delta for.
 * @returns Returns the nanosecond delta between UTC and local time.
 */
RTDECL(int64_t) RTTimeLocalDeltaNanoFor(PCRTTIMESPEC pTimeSpec);

/**
 * Explodes a time spec to the localized timezone.
 *
 * @returns pTime.
 * @param   pTime       Where to store the exploded time.
 * @param   pTimeSpec   The time spec to exploded (UTC).
 */
RTDECL(PRTTIME) RTTimeLocalExplode(PRTTIME pTime, PCRTTIMESPEC pTimeSpec);

/**
 * Normalizes the fields of a time structure containing local time.
 *
 * See RTTimeNormalize for details.
 *
 * @returns pTime on success.
 * @returns NULL if the data is invalid.
 * @param   pTime       The time structure to normalize.
 */
RTDECL(PRTTIME) RTTimeLocalNormalize(PRTTIME pTime);

/**
 * Converts a time structure to UTC, relying on UTC offset information
 * if it contains local time.
 *
 * @returns pTime on success.
 * @returns NULL if the data is invalid.
 * @param   pTime       The time structure to convert.
 */
RTDECL(PRTTIME) RTTimeConvertToZulu(PRTTIME pTime);

/**
 * Converts a time spec to a ISO date string.
 *
 * @returns psz on success.
 * @returns NULL on buffer underflow.
 * @param   pTime       The time. Caller should've normalized this.
 * @param   psz         Where to store the string.
 * @param   cb          The size of the buffer.
 */
RTDECL(char *) RTTimeToString(PCRTTIME pTime, char *psz, size_t cb);

/**
 * Converts a time spec to a ISO date string, extended version.
 *
 * @returns Output string length on success (positive), VERR_BUFFER_OVERFLOW
 *          (negative) or VERR_OUT_OF_RANGE (negative) on failure.
 * @param   pTime           The time. Caller should've normalized this.
 * @param   psz             Where to store the string.
 * @param   cb              The size of the buffer.
 * @param   cFractionDigits Number of digits in the fraction.  Max is 9.
 */
RTDECL(ssize_t) RTTimeToStringEx(PCRTTIME pTime, char *psz, size_t cb, unsigned cFractionDigits);

/** Suggested buffer length for RTTimeToString and RTTimeToStringEx output, including terminator. */
#define RTTIME_STR_LEN      40

/**
 * Attempts to convert an ISO date string to a time structure.
 *
 * We're a little forgiving with zero padding, unspecified parts, and leading
 * and trailing spaces.
 *
 * @retval  pTime on success,
 * @retval  NULL on failure.
 * @param   pTime       Where to store the time on success.
 * @param   pszString   The ISO date string to convert.
 */
RTDECL(PRTTIME) RTTimeFromString(PRTTIME pTime, const char *pszString);

/**
 * Formats the given time on a RTC-2822 compliant format.
 *
 * @returns Output string length on success (positive), VERR_BUFFER_OVERFLOW
 *          (negative) on failure.
 * @param   pTime       The time. Caller should've normalized this.
 * @param   psz         Where to store the string.
 * @param   cb          The size of the buffer.
 * @param   fFlags      RTTIME_RFC2822_F_XXX
 * @sa      RTTIME_RFC2822_LEN
 */
RTDECL(ssize_t) RTTimeToRfc2822(PRTTIME pTime, char *psz, size_t cb, uint32_t fFlags);

/** Suggested buffer length for RTTimeToRfc2822 output, including terminator. */
#define RTTIME_RFC2822_LEN      40
/** @name RTTIME_RFC2822_F_XXX
 * @{ */
/** Use the deprecated GMT timezone instead of +/-0000.
 * This is required by the HTTP RFC-7231 7.1.1.1. */
#define RTTIME_RFC2822_F_GMT    RT_BIT_32(0)
/** @} */

/**
 * Attempts to convert an RFC-2822 date string to a time structure.
 *
 * We're a little forgiving with zero padding, unspecified parts, and leading
 * and trailing spaces.
 *
 * @retval  pTime on success,
 * @retval  NULL on failure.
 * @param   pTime       Where to store the time on success.
 * @param   pszString   The ISO date string to convert.
 */
RTDECL(PRTTIME) RTTimeFromRfc2822(PRTTIME pTime, const char *pszString);

/**
 * Checks if a year is a leap year or not.
 *
 * @returns true if it's a leap year.
 * @returns false if it's a common year.
 * @param   i32Year     The year in question.
 */
RTDECL(bool) RTTimeIsLeapYear(int32_t i32Year);

/**
 * Compares two normalized time structures.
 *
 * @retval  0 if equal.
 * @retval  -1 if @a pLeft is earlier than @a pRight.
 * @retval  1 if @a pRight is earlier than @a pLeft.
 *
 * @param   pLeft       The left side time.  NULL is accepted.
 * @param   pRight      The right side time.  NULL is accepted.
 *
 * @note    A NULL time is considered smaller than anything else.  If both are
 *          NULL, they are considered equal.
 */
RTDECL(int) RTTimeCompare(PCRTTIME pLeft, PCRTTIME pRight);

/**
 * Gets the current nanosecond timestamp.
 *
 * @returns nanosecond timestamp.
 */
RTDECL(uint64_t) RTTimeNanoTS(void);

/**
 * Gets the current millisecond timestamp.
 *
 * @returns millisecond timestamp.
 */
RTDECL(uint64_t) RTTimeMilliTS(void);

/**
 * Debugging the time api.
 *
 * @returns the number of 1ns steps which has been applied by RTTimeNanoTS().
 */
RTDECL(uint32_t) RTTimeDbgSteps(void);

/**
 * Debugging the time api.
 *
 * @returns the number of times the TSC interval expired RTTimeNanoTS().
 */
RTDECL(uint32_t) RTTimeDbgExpired(void);

/**
 * Debugging the time api.
 *
 * @returns the number of bad previous values encountered by RTTimeNanoTS().
 */
RTDECL(uint32_t) RTTimeDbgBad(void);

/**
 * Debugging the time api.
 *
 * @returns the number of update races in RTTimeNanoTS().
 */
RTDECL(uint32_t) RTTimeDbgRaces(void);


RTDECL(const char *) RTTimeNanoTSWorkerName(void);


/** @name RTTimeNanoTS GIP worker functions, for TM.
 * @{ */

/** Extra info optionally returned by the RTTimeNanoTS GIP workers. */
typedef struct RTITMENANOTSEXTRA
{
    /** The TSC value used (delta adjusted). */
    uint64_t    uTSCValue;
} RTITMENANOTSEXTRA;
/** Pointer to extra info optionally returned by the RTTimeNanoTS GIP workers. */
typedef RTITMENANOTSEXTRA *PRTITMENANOTSEXTRA;

/** Pointer to a RTTIMENANOTSDATA structure. */
typedef struct RTTIMENANOTSDATA *PRTTIMENANOTSDATA;

/**
 * Nanosecond timestamp data.
 *
 * This is used to keep track of statistics and callback so IPRT
 * and TM (VirtualBox) can share code.
 *
 * @remark Keep this in sync with the assembly version in timesupA.asm.
 */
typedef struct RTTIMENANOTSDATA
{
    /** Where the previous timestamp is stored.
     * This is maintained to ensure that time doesn't go backwards or anything. */
    uint64_t volatile  *pu64Prev;

    /**
     * Helper function that's used by the assembly routines when something goes bust.
     *
     * @param   pData           Pointer to this structure.
     * @param   u64NanoTS       The calculated nano ts.
     * @param   u64DeltaPrev    The delta relative to the previously returned timestamp.
     * @param   u64PrevNanoTS   The previously returned timestamp (as it was read it).
     */
    DECLCALLBACKMEMBER(void, pfnBad,(PRTTIMENANOTSDATA pData, uint64_t u64NanoTS, uint64_t u64DeltaPrev, uint64_t u64PrevNanoTS));

    /**
     * Callback for when rediscovery is required.
     *
     * @returns Nanosecond timestamp.
     * @param   pData           Pointer to this structure.
     * @param   pExtra          Where to return extra time info. Optional.
     */
    DECLCALLBACKMEMBER(uint64_t, pfnRediscover,(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra));

    /**
     * Callback for when some CPU index related stuff goes wrong.
     *
     * @returns Nanosecond timestamp.
     * @param   pData           Pointer to this structure.
     * @param   pExtra          Where to return extra time info. Optional.
     * @param   idApic          The APIC ID if available, otherwise (UINT16_MAX-1).
     * @param   iCpuSet         The CPU set index if available, otherwise
     *                          (UINT16_MAX-1).
     * @param   iGipCpu         The GIP CPU array index if available, otherwise
     *                          (UINT16_MAX-1).
     */
    DECLCALLBACKMEMBER(uint64_t, pfnBadCpuIndex,(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra,
                                                 uint16_t idApic, uint16_t iCpuSet, uint16_t iGipCpu));

    /** Number of 1ns steps because of overshooting the period. */
    uint32_t            c1nsSteps;
    /** The number of times the interval expired (overflow). */
    uint32_t            cExpired;
    /** Number of "bad" previous values. */
    uint32_t            cBadPrev;
    /** The number of update races. */
    uint32_t            cUpdateRaces;
} RTTIMENANOTSDATA;

#ifndef IN_RING3
/**
 * The Ring-3 layout of the RTTIMENANOTSDATA structure.
 */
typedef struct RTTIMENANOTSDATAR3
{
    R3PTRTYPE(uint64_t volatile  *) pu64Prev;
    DECLR3CALLBACKMEMBER(void, pfnBad,(PRTTIMENANOTSDATA pData, uint64_t u64NanoTS, uint64_t u64DeltaPrev, uint64_t u64PrevNanoTS));
    DECLR3CALLBACKMEMBER(uint64_t, pfnRediscover,(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra));
    DECLR3CALLBACKMEMBER(uint64_t, pfnBadCpuIndex,(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra,
                                                   uint16_t idApic, uint16_t iCpuSet, uint16_t iGipCpu));
    uint32_t            c1nsSteps;
    uint32_t            cExpired;
    uint32_t            cBadPrev;
    uint32_t            cUpdateRaces;
} RTTIMENANOTSDATAR3;
#else
typedef RTTIMENANOTSDATA RTTIMENANOTSDATAR3;
#endif

#ifndef IN_RING0
/**
 * The Ring-3 layout of the RTTIMENANOTSDATA structure.
 */
typedef struct RTTIMENANOTSDATAR0
{
    R0PTRTYPE(uint64_t volatile  *) pu64Prev;
    DECLR0CALLBACKMEMBER(void, pfnBad,(PRTTIMENANOTSDATA pData, uint64_t u64NanoTS, uint64_t u64DeltaPrev, uint64_t u64PrevNanoTS));
    DECLR0CALLBACKMEMBER(uint64_t, pfnRediscover,(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra));
    DECLR0CALLBACKMEMBER(uint64_t, pfnBadCpuIndex,(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra,
                                                   uint16_t idApic, uint16_t iCpuSet, uint16_t iGipCpu));
    uint32_t            c1nsSteps;
    uint32_t            cExpired;
    uint32_t            cBadPrev;
    uint32_t            cUpdateRaces;
} RTTIMENANOTSDATAR0;
#else
typedef RTTIMENANOTSDATA RTTIMENANOTSDATAR0;
#endif

#ifndef IN_RC
/**
 * The RC layout of the RTTIMENANOTSDATA structure.
 */
typedef struct RTTIMENANOTSDATARC
{
    RCPTRTYPE(uint64_t volatile  *) pu64Prev;
    DECLRCCALLBACKMEMBER(void, pfnBad,(PRTTIMENANOTSDATA pData, uint64_t u64NanoTS, uint64_t u64DeltaPrev, uint64_t u64PrevNanoTS));
    DECLRCCALLBACKMEMBER(uint64_t, pfnRediscover,(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra));
    DECLRCCALLBACKMEMBER(uint64_t, pfnBadCpuIndex,(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra,
                                                   uint16_t idApic, uint16_t iCpuSet, uint16_t iGipCpu));
    uint32_t            c1nsSteps;
    uint32_t            cExpired;
    uint32_t            cBadPrev;
    uint32_t            cUpdateRaces;
} RTTIMENANOTSDATARC;
#else
typedef RTTIMENANOTSDATA RTTIMENANOTSDATARC;
#endif

/** Internal RTTimeNanoTS worker (assembly). */
typedef DECLCALLBACKTYPE(uint64_t, FNTIMENANOTSINTERNAL,(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra));
/** Pointer to an internal RTTimeNanoTS worker (assembly). */
typedef FNTIMENANOTSINTERNAL *PFNTIMENANOTSINTERNAL;
RTDECL(uint64_t) RTTimeNanoTSLegacySyncInvarNoDelta(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLFenceSyncInvarNoDelta(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
#ifdef IN_RING3
RTDECL(uint64_t) RTTimeNanoTSLegacyAsyncUseApicId(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLegacyAsyncUseApicIdExt0B(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLegacyAsyncUseApicIdExt8000001E(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLegacyAsyncUseRdtscp(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLegacyAsyncUseRdtscpGroupChNumCl(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLegacyAsyncUseIdtrLim(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLegacySyncInvarWithDeltaUseApicId(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLegacySyncInvarWithDeltaUseApicIdExt0B(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLegacySyncInvarWithDeltaUseApicIdExt8000001E(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLegacySyncInvarWithDeltaUseRdtscp(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLegacySyncInvarWithDeltaUseIdtrLim(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLFenceAsyncUseApicId(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLFenceAsyncUseApicIdExt0B(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLFenceAsyncUseApicIdExt8000001E(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLFenceAsyncUseRdtscp(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLFenceAsyncUseRdtscpGroupChNumCl(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLFenceAsyncUseIdtrLim(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLFenceSyncInvarWithDeltaUseApicId(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLFenceSyncInvarWithDeltaUseApicIdExt0B(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLFenceSyncInvarWithDeltaUseApicIdExt8000001E(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLFenceSyncInvarWithDeltaUseRdtscp(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLFenceSyncInvarWithDeltaUseIdtrLim(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
#else
RTDECL(uint64_t) RTTimeNanoTSLegacyAsync(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLegacySyncInvarWithDelta(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLFenceAsync(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
RTDECL(uint64_t) RTTimeNanoTSLFenceSyncInvarWithDelta(PRTTIMENANOTSDATA pData, PRTITMENANOTSEXTRA pExtra);
#endif

/** @} */


/**
 * Gets the current nanosecond timestamp.
 *
 * This differs from RTTimeNanoTS in that it will use system APIs and not do any
 * resolution or performance optimizations.
 *
 * @returns nanosecond timestamp.
 */
RTDECL(uint64_t) RTTimeSystemNanoTS(void);

/**
 * Gets the current millisecond timestamp.
 *
 * This differs from RTTimeNanoTS in that it will use system APIs and not do any
 * resolution or performance optimizations.
 *
 * @returns millisecond timestamp.
 */
RTDECL(uint64_t) RTTimeSystemMilliTS(void);

/**
 * Get the nanosecond timestamp relative to program startup.
 *
 * @returns Timestamp relative to program startup.
 */
RTDECL(uint64_t)  RTTimeProgramNanoTS(void);

/**
 * Get the microsecond timestamp relative to program startup.
 *
 * @returns Timestamp relative to program startup.
 */
RTDECL(uint64_t)  RTTimeProgramMicroTS(void);

/**
 * Get the millisecond timestamp relative to program startup.
 *
 * @returns Timestamp relative to program startup.
 */
RTDECL(uint64_t)  RTTimeProgramMilliTS(void);

/**
 * Get the second timestamp relative to program startup.
 *
 * @returns Timestamp relative to program startup.
 */
RTDECL(uint32_t)  RTTimeProgramSecTS(void);

/**
 * Get the RTTimeNanoTS() of when the program started.
 *
 * @returns Program startup timestamp.
 */
RTDECL(uint64_t) RTTimeProgramStartNanoTS(void);


/**
 * Time zone information.
 */
typedef struct RTTIMEZONEINFO
{
    /** Unix time zone name (continent/country[/city]|). */
    const char     *pszUnixName;
    /** Windows time zone name.   */
    const char     *pszWindowsName;
    /** The length of the unix time zone name. */
    uint8_t         cchUnixName;
    /** The length of the windows time zone name. */
    uint8_t         cchWindowsName;
    /** Two letter country/territory code if applicable, otherwise 'ZZ'. */
    char            szCountry[3];
    /** Two letter windows country/territory code if applicable.
     * Empty string if no windows mapping. */
    char            szWindowsCountry[3];
#if 0 /* Add when needed and it's been extracted. */
    /** The standard delta in minutes (add to UTC). */
    int16_t         cMinStdDelta;
    /** The daylight saving time delta in minutes (add to UTC). */
    int16_t         cMinDstDelta;
#endif
    /** closest matching windows time zone index. */
    uint32_t        idxWindows;
    /** Flags, RTTIMEZONEINFO_F_XXX. */
    uint32_t        fFlags;
} RTTIMEZONEINFO;
/** Pointer to time zone info.   */
typedef RTTIMEZONEINFO const *PCRTTIMEZONEINFO;

/** @name RTTIMEZONEINFO_F_XXX - time zone info flags.
 *  @{ */
/** Indicates golden mapping entry for a windows time zone name. */
#define RTTIMEZONEINFO_F_GOLDEN         RT_BIT_32(0)
/** @} */

/**
 * Looks up static time zone information by unix name.
 *
 * @returns Pointer to info entry if found, NULL if not.
 * @param   pszName     The unix zone name (TZ).
 */
RTDECL(PCRTTIMEZONEINFO) RTTimeZoneGetInfoByUnixName(const char *pszName);

/**
 * Looks up static time zone information by window name.
 *
 * @returns Pointer to info entry if found, NULL if not.
 * @param   pszName     The windows zone name (reg key).
 */
RTDECL(PCRTTIMEZONEINFO) RTTimeZoneGetInfoByWindowsName(const char *pszName);

/**
 * Looks up static time zone information by windows index.
 *
 * @returns Pointer to info entry if found, NULL if not.
 * @param   idxZone     The windows timezone index.
 */
RTDECL(PCRTTIMEZONEINFO) RTTimeZoneGetInfoByWindowsIndex(uint32_t idxZone);

/**
 * Get the current time zone (TZ).
 *
 * @returns IPRT status code.
 * @param   pszName     Where to return the time zone name.
 * @param   cbName      The size of the name buffer.
 */
RTDECL(int) RTTimeZoneGetCurrent(char *pszName, size_t cbName);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_time_h */

