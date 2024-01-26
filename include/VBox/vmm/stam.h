/** @file
 * STAM - Statistics Manager.
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

#ifndef VBOX_INCLUDED_vmm_stam_h
#define VBOX_INCLUDED_vmm_stam_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#include <iprt/stdarg.h>
#ifdef _MSC_VER
# if RT_MSC_PREREQ(RT_MSC_VER_VS2005)
#  include <iprt/sanitized/intrin.h>
# endif
#endif
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
# include <iprt/asm-arm.h>
#endif

RT_C_DECLS_BEGIN

/** @defgroup grp_stam     The Statistics Manager API
 * @ingroup grp_vmm
 * @{
 */

#if defined(VBOX_WITHOUT_RELEASE_STATISTICS) && defined(VBOX_WITH_STATISTICS)
# error "Both VBOX_WITHOUT_RELEASE_STATISTICS and VBOX_WITH_STATISTICS are defined! Make up your mind!"
#endif


/** @def STAM_GET_TS
 * Gets the CPU timestamp counter.
 *
 * @param   u64     The 64-bit variable which the timestamp shall be saved in.
 */
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
#  define STAM_GET_TS(u64) do { (u64) = ASMReadTSC(); } while (0)
#elif defined(__GNUC__)
# if defined(RT_ARCH_X86)
   /* This produces optimal assembler code for x86 but does not work for AMD64 ('A' means 'either rax or rdx') */
#  define STAM_GET_TS(u64) __asm__ __volatile__ ("rdtsc\n\t" : "=A" (u64))
# elif defined(RT_ARCH_AMD64)
#  define STAM_GET_TS(u64) \
    do { uint64_t low; uint64_t high; \
         __asm__ __volatile__ ("rdtsc\n\t" : "=a"(low), "=d"(high)); \
         (u64) = ((high << 32) | low); \
    } while (0)
# endif
#else
# if RT_MSC_PREREQ(RT_MSC_VER_VS2005)
#  pragma intrinsic(__rdtsc)
#  define STAM_GET_TS(u64)    \
     do { (u64) = __rdtsc(); } while (0)
# else
#  define STAM_GET_TS(u64)    \
     do {                               \
         uint64_t u64Tmp;               \
         __asm {                        \
             __asm rdtsc                \
             __asm mov dword ptr [u64Tmp],     eax   \
             __asm mov dword ptr [u64Tmp + 4], edx   \
         }                              \
         (u64) = u64Tmp; \
     } while (0)
# endif
#endif


/** @def STAM_REL_STATS
 * Code for inclusion only when VBOX_WITH_STATISTICS is defined.
 * @param   code    A code block enclosed in {}.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_STATS(code) do code while(0)
#else
# define STAM_REL_STATS(code) do {} while(0)
#endif
/** @def STAM_STATS
 * Code for inclusion only when VBOX_WITH_STATISTICS is defined.
 * @param   code    A code block enclosed in {}.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_STATS(code) STAM_REL_STATS(code)
#else
# define STAM_STATS(code) do {} while(0)
#endif


/**
 * Sample type.
 */
typedef enum STAMTYPE
{
    /** Invalid entry. */
    STAMTYPE_INVALID = 0,
    /** Generic counter. */
    STAMTYPE_COUNTER,
    /** Profiling of an function. */
    STAMTYPE_PROFILE,
    /** Profiling of an operation. */
    STAMTYPE_PROFILE_ADV,
    /** Ratio of A to B, uint32_t types. Not reset. */
    STAMTYPE_RATIO_U32,
    /** Ratio of A to B, uint32_t types. Reset both to 0. */
    STAMTYPE_RATIO_U32_RESET,
    /** Callback. */
    STAMTYPE_CALLBACK,
    /** Generic unsigned 8-bit value. Not reset. */
    STAMTYPE_U8,
    /** Generic unsigned 8-bit value. Reset to 0. */
    STAMTYPE_U8_RESET,
    /** Generic hexadecimal unsigned 8-bit value. Not reset. */
    STAMTYPE_X8,
    /** Generic hexadecimal unsigned 8-bit value. Reset to 0. */
    STAMTYPE_X8_RESET,
    /** Generic unsigned 16-bit value. Not reset. */
    STAMTYPE_U16,
    /** Generic unsigned 16-bit value. Reset to 0. */
    STAMTYPE_U16_RESET,
    /** Generic hexadecimal unsigned 16-bit value. Not reset. */
    STAMTYPE_X16,
    /** Generic hexadecimal unsigned 16-bit value. Reset to 0. */
    STAMTYPE_X16_RESET,
    /** Generic unsigned 32-bit value. Not reset. */
    STAMTYPE_U32,
    /** Generic unsigned 32-bit value. Reset to 0. */
    STAMTYPE_U32_RESET,
    /** Generic hexadecimal unsigned 32-bit value. Not reset. */
    STAMTYPE_X32,
    /** Generic hexadecimal unsigned 32-bit value. Reset to 0. */
    STAMTYPE_X32_RESET,
    /** Generic unsigned 64-bit value. Not reset. */
    STAMTYPE_U64,
    /** Generic unsigned 64-bit value. Reset to 0. */
    STAMTYPE_U64_RESET,
    /** Generic hexadecimal unsigned 64-bit value. Not reset. */
    STAMTYPE_X64,
    /** Generic hexadecimal unsigned 64-bit value. Reset to 0. */
    STAMTYPE_X64_RESET,
    /** Generic boolean value. Not reset. */
    STAMTYPE_BOOL,
    /** Generic boolean value. Reset to false. */
    STAMTYPE_BOOL_RESET,
    /** The end (exclusive). */
    STAMTYPE_END
} STAMTYPE;

/**
 * Sample visibility type.
 */
typedef enum STAMVISIBILITY
{
    /** Invalid entry. */
    STAMVISIBILITY_INVALID = 0,
    /** Always visible. */
    STAMVISIBILITY_ALWAYS,
    /** Only visible when used (/hit). */
    STAMVISIBILITY_USED,
    /** Not visible in the GUI. */
    STAMVISIBILITY_NOT_GUI,
    /** The end (exclusive). */
    STAMVISIBILITY_END
} STAMVISIBILITY;

/**
 * Sample unit.
 */
typedef enum STAMUNIT
{
    /** Invalid entry .*/
    STAMUNIT_INVALID = 0,
    /** No unit. */
    STAMUNIT_NONE,
    /** Number of calls. */
    STAMUNIT_CALLS,
    /** Count of whatever. */
    STAMUNIT_COUNT,
    /** Count of bytes. */
    STAMUNIT_BYTES,
    /** Count of bytes per call. */
    STAMUNIT_BYTES_PER_CALL,
    /** Count of bytes. */
    STAMUNIT_PAGES,
    /** Error count. */
    STAMUNIT_ERRORS,
    /** Number of occurences. */
    STAMUNIT_OCCURENCES,
    /** Ticks. */
    STAMUNIT_TICKS,
    /** Ticks per call. */
    STAMUNIT_TICKS_PER_CALL,
    /** Ticks per occurence. */
    STAMUNIT_TICKS_PER_OCCURENCE,
    /** Ratio of good vs. bad. */
    STAMUNIT_GOOD_BAD,
    /** Megabytes. */
    STAMUNIT_MEGABYTES,
    /** Kilobytes. */
    STAMUNIT_KILOBYTES,
    /** Nano seconds. */
    STAMUNIT_NS,
    /** Nanoseconds per call. */
    STAMUNIT_NS_PER_CALL,
    /** Nanoseconds per call. */
    STAMUNIT_NS_PER_OCCURENCE,
    /** Percentage. */
    STAMUNIT_PCT,
    /** Hertz. */
    STAMUNIT_HZ,
    /** The end (exclusive). */
    STAMUNIT_END
} STAMUNIT;

/** @name STAM_REFRESH_GRP_XXX - STAM refresh groups
 * @{ */
#define STAM_REFRESH_GRP_NONE       UINT8_MAX
#define STAM_REFRESH_GRP_GVMM       0
#define STAM_REFRESH_GRP_GMM        1
#define STAM_REFRESH_GRP_NEM        2
/** @} */


/** @def STAM_REL_U8_INC
 * Increments a uint8_t sample by one.
 *
 * @param   pCounter    Pointer to the uint8_t variable to operate on.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_U8_INC(pCounter) \
    do { ++*(pCounter); } while (0)
#else
# define STAM_REL_U8_INC(pCounter) do { } while (0)
#endif
/** @def STAM_U8_INC
 * Increments a uint8_t sample by one.
 *
 * @param   pCounter    Pointer to the uint8_t variable to operate on.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_U8_INC(pCounter) STAM_REL_U8_INC(pCounter)
#else
# define STAM_U8_INC(pCounter) do { } while (0)
#endif


/** @def STAM_REL_U8_DEC
 * Decrements a uint8_t sample by one.
 *
 * @param   pCounter    Pointer to the uint8_t variable to operate on.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_U8_DEC(pCounter) \
    do { --*(pCounter); } while (0)
#else
# define STAM_REL_U8_DEC(pCounter) do { } while (0)
#endif
/** @def STAM_U8_DEC
 * Decrements a uint8_t sample by one.
 *
 * @param   pCounter    Pointer to the uint8_t variable to operate on.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_U8_DEC(pCounter) STAM_REL_U8_DEC(pCounter)
#else
# define STAM_U8_DEC(pCounter) do { } while (0)
#endif


/** @def STAM_REL_U8_ADD
 * Increments a uint8_t sample by a value.
 *
 * @param   pCounter    Pointer to the uint8_t variable to operate on.
 * @param   Addend      The value to add.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_U8_ADD(pCounter, Addend) \
    do { *(pCounter) += (Addend); } while (0)
#else
# define STAM_REL_U8_ADD(pCounter, Addend) do { } while (0)
#endif
/** @def STAM_U8_ADD
 * Increments a uint8_t sample by a value.
 *
 * @param   pCounter    Pointer to the uint8_t variable to operate on.
 * @param   Addend      The value to add.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_U8_ADD(pCounter, Addend) STAM_REL_U8_ADD(pCounter, Addend
#else
# define STAM_U8_ADD(pCounter, Addend) do { } while (0)
#endif


/** @def STAM_REL_U16_INC
 * Increments a uint16_t sample by one.
 *
 * @param   pCounter    Pointer to the uint16_t variable to operate on.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_U16_INC(pCounter) \
    do { ++*(pCounter); } while (0)
#else
# define STAM_REL_U16_INC(pCounter) do { } while (0)
#endif
/** @def STAM_U16_INC
 * Increments a uint16_t sample by one.
 *
 * @param   pCounter    Pointer to the uint16_t variable to operate on.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_U16_INC(pCounter) STAM_REL_U16_INC(pCounter)
#else
# define STAM_U16_INC(pCounter) do { } while (0)
#endif


/** @def STAM_REL_U16_DEC
 * Decrements a uint16_t sample by one.
 *
 * @param   pCounter    Pointer to the uint16_t variable to operate on.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_U16_DEC(pCounter) \
    do { --*(pCounter); } while (0)
#else
# define STAM_REL_U16_DEC(pCounter) do { } while (0)
#endif
/** @def STAM_U16_DEC
 * Decrements a uint16_t sample by one.
 *
 * @param   pCounter    Pointer to the uint16_t variable to operate on.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_U16_DEC(pCounter) STAM_REL_U16_DEC(pCounter)
#else
# define STAM_U16_DEC(pCounter) do { } while (0)
#endif


/** @def STAM_REL_U16_ADD
 * Increments a uint16_t sample by a value.
 *
 * @param   pCounter    Pointer to the uint16_t variable to operate on.
 * @param   Addend      The value to add.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_U16_ADD(pCounter, Addend) \
    do { *(pCounter) += (Addend); } while (0)
#else
# define STAM_REL_U16_ADD(pCounter, Addend) do { } while (0)
#endif
/** @def STAM_U16_ADD
 * Increments a uint16_t sample by a value.
 *
 * @param   pCounter    Pointer to the uint16_t variable to operate on.
 * @param   Addend      The value to add.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_U16_ADD(pCounter, Addend) STAM_REL_U16_ADD(pCounter, Addend)
#else
# define STAM_U16_ADD(pCounter, Addend) do { } while (0)
#endif


/** @def STAM_REL_U32_INC
 * Increments a uint32_t sample by one.
 *
 * @param   pCounter    Pointer to the uint32_t variable to operate on.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_U32_INC(pCounter) \
    do { ++*(pCounter); } while (0)
#else
# define STAM_REL_U32_INC(pCounter) do { } while (0)
#endif
/** @def STAM_U32_INC
 * Increments a uint32_t sample by one.
 *
 * @param   pCounter    Pointer to the uint32_t variable to operate on.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_U32_INC(pCounter) STAM_REL_U32_INC(pCounter)
#else
# define STAM_U32_INC(pCounter) do { } while (0)
#endif


/** @def STAM_REL_U32_DEC
 * Decrements a uint32_t sample by one.
 *
 * @param   pCounter    Pointer to the uint32_t variable to operate on.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_U32_DEC(pCounter) \
    do { --*(pCounter); } while (0)
#else
# define STAM_REL_U32_DEC(pCounter) do { } while (0)
#endif
/** @def STAM_U32_DEC
 * Decrements a uint32_t sample by one.
 *
 * @param   pCounter    Pointer to the uint32_t variable to operate on.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_U32_DEC(pCounter) STAM_REL_U32_DEC(pCounter)
#else
# define STAM_U32_DEC(pCounter) do { } while (0)
#endif


/** @def STAM_REL_U32_ADD
 * Increments a uint32_t sample by value.
 *
 * @param   pCounter    Pointer to the uint32_t variable to operate on.
 * @param   Addend      The value to add.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_U32_ADD(pCounter, Addend) \
    do { *(pCounter) += (Addend); } while (0)
#else
# define STAM_REL_U32_ADD(pCounter, Addend) do { } while (0)
#endif
/** @def STAM_U32_ADD
 * Increments a uint32_t sample by value.
 *
 * @param   pCounter    Pointer to the uint32_t variable to operate on.
 * @param   Addend      The value to add.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_U32_ADD(pCounter, Addend) STAM_REL_U32_ADD(pCounter, Addend)
#else
# define STAM_U32_ADD(pCounter, Addend) do { } while (0)
#endif


/** @def STAM_REL_U64_INC
 * Increments a uint64_t sample by one.
 *
 * @param   pCounter    Pointer to the uint64_t variable to operate on.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_U64_INC(pCounter) \
    do { ++*(pCounter); } while (0)
#else
# define STAM_REL_U64_INC(pCounter) do { } while (0)
#endif
/** @def STAM_U64_INC
 * Increments a uint64_t sample by one.
 *
 * @param   pCounter    Pointer to the uint64_t variable to operate on.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_U64_INC(pCounter) STAM_REL_U64_INC(pCounter)
#else
# define STAM_U64_INC(pCounter) do { } while (0)
#endif


/** @def STAM_REL_U64_DEC
 * Decrements a uint64_t sample by one.
 *
 * @param   pCounter    Pointer to the uint64_t variable to operate on.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_U64_DEC(pCounter) \
    do { --*(pCounter); } while (0)
#else
# define STAM_REL_U64_DEC(pCounter) do { } while (0)
#endif
/** @def STAM_U64_DEC
 * Decrements a uint64_t sample by one.
 *
 * @param   pCounter    Pointer to the uint64_t variable to operate on.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_U64_DEC(pCounter) STAM_REL_U64_DEC(pCounter)
#else
# define STAM_U64_DEC(pCounter) do { } while (0)
#endif


/** @def STAM_REL_U64_ADD
 * Increments a uint64_t sample by a value.
 *
 * @param   pCounter    Pointer to the uint64_t variable to operate on.
 * @param   Addend      The value to add.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_U64_ADD(pCounter, Addend) \
    do { *(pCounter) += (Addend); } while (0)
#else
# define STAM_REL_U64_ADD(pCounter, Addend) do { } while (0)
#endif
/** @def STAM_U64_ADD
 * Increments a uint64_t sample by a value.
 *
 * @param   pCounter    Pointer to the uint64_t variable to operate on.
 * @param   Addend      The value to add.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_U64_ADD(pCounter, Addend) STAM_REL_U64_ADD(pCounter, Addend)
#else
# define STAM_U64_ADD(pCounter, Addend) do { } while (0)
#endif


/**
 * Counter sample - STAMTYPE_COUNTER.
 */
typedef struct STAMCOUNTER
{
    /** The current count. */
    volatile uint64_t   c;
} STAMCOUNTER;
/** Pointer to a counter. */
typedef STAMCOUNTER *PSTAMCOUNTER;
/** Pointer to a const counter. */
typedef const STAMCOUNTER *PCSTAMCOUNTER;


/** @def STAM_REL_COUNTER_INC
 * Increments a counter sample by one.
 *
 * @param   pCounter    Pointer to the STAMCOUNTER structure to operate on.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_COUNTER_INC(pCounter) \
    do { (pCounter)->c++; } while (0)
#else
# define STAM_REL_COUNTER_INC(pCounter) do { } while (0)
#endif
/** @def STAM_COUNTER_INC
 * Increments a counter sample by one.
 *
 * @param   pCounter    Pointer to the STAMCOUNTER structure to operate on.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_COUNTER_INC(pCounter) STAM_REL_COUNTER_INC(pCounter)
#else
# define STAM_COUNTER_INC(pCounter) do { } while (0)
#endif


/** @def STAM_REL_COUNTER_DEC
 * Decrements a counter sample by one.
 *
 * @param   pCounter    Pointer to the STAMCOUNTER structure to operate on.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_COUNTER_DEC(pCounter) \
    do { (pCounter)->c--; } while (0)
#else
# define STAM_REL_COUNTER_DEC(pCounter) do { } while (0)
#endif
/** @def STAM_COUNTER_DEC
 * Decrements a counter sample by one.
 *
 * @param   pCounter    Pointer to the STAMCOUNTER structure to operate on.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_COUNTER_DEC(pCounter) STAM_REL_COUNTER_DEC(pCounter)
#else
# define STAM_COUNTER_DEC(pCounter) do { } while (0)
#endif


/** @def STAM_REL_COUNTER_ADD
 * Increments a counter sample by a value.
 *
 * @param   pCounter    Pointer to the STAMCOUNTER structure to operate on.
 * @param   Addend      The value to add to the counter.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_COUNTER_ADD(pCounter, Addend) \
    do { (pCounter)->c += (Addend); } while (0)
#else
# define STAM_REL_COUNTER_ADD(pCounter, Addend) do { } while (0)
#endif
/** @def STAM_COUNTER_ADD
 * Increments a counter sample by a value.
 *
 * @param   pCounter    Pointer to the STAMCOUNTER structure to operate on.
 * @param   Addend      The value to add to the counter.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_COUNTER_ADD(pCounter, Addend) STAM_REL_COUNTER_ADD(pCounter, Addend)
#else
# define STAM_COUNTER_ADD(pCounter, Addend) do { } while (0)
#endif


/** @def STAM_REL_COUNTER_RESET
 * Resets the statistics sample.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_COUNTER_RESET(pCounter) do { (pCounter)->c = 0; } while (0)
#else
# define STAM_REL_COUNTER_RESET(pCounter) do { } while (0)
#endif
/** @def STAM_COUNTER_RESET
 * Resets the statistics sample.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_COUNTER_RESET(pCounter) STAM_REL_COUNTER_RESET(pCounter)
#else
# define STAM_COUNTER_RESET(pCounter) do { } while (0)
#endif



/**
 * Profiling sample - STAMTYPE_PROFILE.
 */
typedef struct STAMPROFILE
{
    /** Number of periods. */
    volatile uint64_t   cPeriods;
    /** Total count of ticks. */
    volatile uint64_t   cTicks;
    /** Maximum tick count during a sampling. */
    volatile uint64_t   cTicksMax;
    /** Minimum tick count during a sampling. */
    volatile uint64_t   cTicksMin;
} STAMPROFILE;
/** Pointer to a profile sample. */
typedef STAMPROFILE *PSTAMPROFILE;
/** Pointer to a const profile sample. */
typedef const STAMPROFILE *PCSTAMPROFILE;


/** @def STAM_REL_PROFILE_ADD_PERIOD
 * Adds a period.
 *
 * @param   pProfile        Pointer to the STAMPROFILE structure to operate on.
 * @param   cTicksInPeriod  The number of tick (or whatever) of the preiod
 *                          being added.  This is only referenced once.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_PROFILE_ADD_PERIOD(pProfile, cTicksInPeriod) \
    do { \
        uint64_t const StamPrefix_cTicks = (cTicksInPeriod); \
        (pProfile)->cTicks += StamPrefix_cTicks; \
        (pProfile)->cPeriods++; \
        if ((pProfile)->cTicksMax < StamPrefix_cTicks) \
            (pProfile)->cTicksMax = StamPrefix_cTicks; \
        if ((pProfile)->cTicksMin > StamPrefix_cTicks) \
            (pProfile)->cTicksMin = StamPrefix_cTicks; \
    } while (0)
#else
# define STAM_REL_PROFILE_ADD_PERIOD(pProfile, cTicksInPeriod) do { } while (0)
#endif
/** @def STAM_PROFILE_ADD_PERIOD
 * Adds a period.
 *
 * @param   pProfile        Pointer to the STAMPROFILE structure to operate on.
 * @param   cTicksInPeriod  The number of tick (or whatever) of the preiod
 *                          being added.  This is only referenced once.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_PROFILE_ADD_PERIOD(pProfile, cTicksInPeriod) STAM_REL_PROFILE_ADD_PERIOD(pProfile, cTicksInPeriod)
#else
# define STAM_PROFILE_ADD_PERIOD(pProfile, cTicksInPeriod) do { } while (0)
#endif


/** @def STAM_REL_PROFILE_START
 * Samples the start time of a profiling period.
 *
 * @param   pProfile    Pointer to the STAMPROFILE structure to operate on.
 * @param   Prefix      Identifier prefix used to internal variables.
 *
 * @remarks Declears a stack variable that will be used by related macros.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_PROFILE_START(pProfile, Prefix) \
    uint64_t Prefix##_tsStart; \
    STAM_GET_TS(Prefix##_tsStart)
#else
# define STAM_REL_PROFILE_START(pProfile, Prefix) do { } while (0)
#endif
/** @def STAM_PROFILE_START
 * Samples the start time of a profiling period.
 *
 * @param   pProfile    Pointer to the STAMPROFILE structure to operate on.
 * @param   Prefix      Identifier prefix used to internal variables.
 *
 * @remarks Declears a stack variable that will be used by related macros.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_PROFILE_START(pProfile, Prefix) STAM_REL_PROFILE_START(pProfile, Prefix)
#else
# define STAM_PROFILE_START(pProfile, Prefix) do { } while (0)
#endif

/** @def STAM_REL_PROFILE_STOP
 * Samples the stop time of a profiling period and updates the sample.
 *
 * @param   pProfile    Pointer to the STAMPROFILE structure to operate on.
 * @param   Prefix      Identifier prefix used to internal variables.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_PROFILE_STOP(pProfile, Prefix) \
    do { \
        uint64_t Prefix##_cTicks; \
        STAM_GET_TS(Prefix##_cTicks); \
        Prefix##_cTicks -= Prefix##_tsStart; \
        (pProfile)->cTicks += Prefix##_cTicks; \
        (pProfile)->cPeriods++; \
        if ((pProfile)->cTicksMax < Prefix##_cTicks) \
            (pProfile)->cTicksMax = Prefix##_cTicks; \
        if ((pProfile)->cTicksMin > Prefix##_cTicks) \
            (pProfile)->cTicksMin = Prefix##_cTicks; \
    } while (0)
#else
# define STAM_REL_PROFILE_STOP(pProfile, Prefix) do { } while (0)
#endif
/** @def STAM_PROFILE_STOP
 * Samples the stop time of a profiling period and updates the sample.
 *
 * @param   pProfile    Pointer to the STAMPROFILE structure to operate on.
 * @param   Prefix      Identifier prefix used to internal variables.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_PROFILE_STOP(pProfile, Prefix) STAM_REL_PROFILE_STOP(pProfile, Prefix)
#else
# define STAM_PROFILE_STOP(pProfile, Prefix) do { } while (0)
#endif


/** @def STAM_REL_PROFILE_STOP_EX
 * Samples the stop time of a profiling period and updates both the sample
 * and an attribution sample.
 *
 * @param   pProfile    Pointer to the STAMPROFILE structure to operate on.
 * @param   pProfile2   Pointer to the STAMPROFILE structure which this
 *                      interval should be attributed to as well. This may be NULL.
 * @param   Prefix      Identifier prefix used to internal variables.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_PROFILE_STOP_EX(pProfile, pProfile2, Prefix) \
    do { \
        uint64_t Prefix##_cTicks; \
        STAM_GET_TS(Prefix##_cTicks); \
        Prefix##_cTicks -= Prefix##_tsStart; \
        (pProfile)->cTicks += Prefix##_cTicks; \
        (pProfile)->cPeriods++; \
        if ((pProfile)->cTicksMax < Prefix##_cTicks) \
            (pProfile)->cTicksMax = Prefix##_cTicks; \
        if ((pProfile)->cTicksMin > Prefix##_cTicks) \
            (pProfile)->cTicksMin = Prefix##_cTicks; \
        \
        if ((pProfile2)) \
        { \
            (pProfile2)->cTicks += Prefix##_cTicks; \
            (pProfile2)->cPeriods++; \
            if ((pProfile2)->cTicksMax < Prefix##_cTicks) \
                (pProfile2)->cTicksMax = Prefix##_cTicks; \
            if ((pProfile2)->cTicksMin > Prefix##_cTicks) \
                (pProfile2)->cTicksMin = Prefix##_cTicks; \
        } \
    } while (0)
#else
# define STAM_REL_PROFILE_STOP_EX(pProfile, pProfile2, Prefix) do { } while (0)
#endif
/** @def STAM_PROFILE_STOP_EX
 * Samples the stop time of a profiling period and updates both the sample
 * and an attribution sample.
 *
 * @param   pProfile    Pointer to the STAMPROFILE structure to operate on.
 * @param   pProfile2   Pointer to the STAMPROFILE structure which this
 *                      interval should be attributed to as well. This may be NULL.
 * @param   Prefix      Identifier prefix used to internal variables.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_PROFILE_STOP_EX(pProfile, pProfile2, Prefix) STAM_REL_PROFILE_STOP_EX(pProfile, pProfile2, Prefix)
#else
# define STAM_PROFILE_STOP_EX(pProfile, pProfile2, Prefix) do { } while (0)
#endif


/** @def STAM_REL_PROFILE_STOP_START
 * Stops one profile counter (if running) and starts another one.
 *
 * @param   pProfile1   Pointer to the STAMPROFILE structure to stop.
 * @param   pProfile2   Pointer to the STAMPROFILE structure to start.
 * @param   Prefix      Identifier prefix used to internal variables.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_PROFILE_STOP_START(pProfile1, pProfile2, Prefix) \
    do { \
        uint64_t Prefix##_tsStop; \
        STAM_GET_TS(Prefix##_tsStop); \
        STAM_REL_PROFILE_ADD_PERIOD(pProfile1, Prefix##_tsStop - Prefix##_tsStart); \
        Prefix##_tsStart = Prefix##_tsStop; \
    } while (0)
#else
# define STAM_REL_PROFILE_STOP_START(pProfile1, pProfile2, Prefix) \
    do { } while (0)
#endif
/** @def STAM_PROFILE_STOP_START
 * Samples the stop time of a profiling period (if running) and updates the
 * sample.
 *
 * @param   pProfile1   Pointer to the STAMPROFILE structure to stop.
 * @param   pProfile2   Pointer to the STAMPROFILE structure to start.
 * @param   Prefix      Identifier prefix used to internal variables.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_PROFILE_STOP_START(pProfile1, pProfile2, Prefix) \
    STAM_REL_PROFILE_STOP_START(pProfile1, pProfile2, Prefix)
#else
# define STAM_PROFILE_STOP_START(pProfile1, pProfile2, Prefix) \
    do { } while (0)
#endif


/** @def STAM_REL_PROFILE_START_NS
 * Samples the start time of a profiling period, using RTTimeNanoTS().
 *
 * @param   pProfile    Pointer to the STAMPROFILE structure to operate on.
 * @param   Prefix      Identifier prefix used to internal variables.
 *
 * @remarks Declears a stack variable that will be used by related macros.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_PROFILE_START_NS(pProfile, Prefix) \
    uint64_t const Prefix##_tsStart = RTTimeNanoTS()
#else
# define STAM_REL_PROFILE_START_NS(pProfile, Prefix) do { } while (0)
#endif
/** @def STAM_PROFILE_START_NS
 * Samples the start time of a profiling period, using RTTimeNanoTS().
 *
 * @param   pProfile    Pointer to the STAMPROFILE structure to operate on.
 * @param   Prefix      Identifier prefix used to internal variables.
 *
 * @remarks Declears a stack variable that will be used by related macros.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_PROFILE_START_NS(pProfile, Prefix) STAM_REL_PROFILE_START_NS(pProfile, Prefix)
#else
# define STAM_PROFILE_START_NS(pProfile, Prefix) do { } while (0)
#endif

/** @def STAM_REL_PROFILE_STOP_NS
 * Samples the stop time of a profiling period and updates the sample, using
 * RTTimeNanoTS().
 *
 * @param   pProfile    Pointer to the STAMPROFILE structure to operate on.
 * @param   Prefix      Identifier prefix used to internal variables.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_PROFILE_STOP_NS(pProfile, Prefix) \
    STAM_REL_PROFILE_ADD_PERIOD(pProfile, RTTimeNanoTS() - Prefix##_tsStart)
#else
# define STAM_REL_PROFILE_STOP_NS(pProfile, Prefix) do { } while (0)
#endif
/** @def STAM_PROFILE_STOP_NS
 * Samples the stop time of a profiling period and updates the sample, using
 * RTTimeNanoTS().
 *
 * @param   pProfile    Pointer to the STAMPROFILE structure to operate on.
 * @param   Prefix      Identifier prefix used to internal variables.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_PROFILE_STOP_NS(pProfile, Prefix) STAM_REL_PROFILE_STOP_NS(pProfile, Prefix)
#else
# define STAM_PROFILE_STOP_NS(pProfile, Prefix) do { } while (0)
#endif


/**
 * Advanced profiling sample - STAMTYPE_PROFILE_ADV.
 *
 * Identical to a STAMPROFILE sample, but the start timestamp
 * is stored after the STAMPROFILE structure so the sampling
 * can start and stop in different functions.
 */
typedef struct STAMPROFILEADV
{
    /** The STAMPROFILE core. */
    STAMPROFILE         Core;
    /** The start timestamp. */
    volatile uint64_t   tsStart;
} STAMPROFILEADV;
/** Pointer to a advanced profile sample. */
typedef STAMPROFILEADV *PSTAMPROFILEADV;
/** Pointer to a const advanced profile sample. */
typedef const STAMPROFILEADV *PCSTAMPROFILEADV;


/** @def STAM_REL_PROFILE_ADV_START
 * Samples the start time of a profiling period.
 *
 * @param   pProfileAdv Pointer to the STAMPROFILEADV structure to operate on.
 * @param   Prefix      Identifier prefix used to internal variables.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_PROFILE_ADV_START(pProfileAdv, Prefix) \
    STAM_GET_TS((pProfileAdv)->tsStart)
#else
# define STAM_REL_PROFILE_ADV_START(pProfileAdv, Prefix) do { } while (0)
#endif
/** @def STAM_PROFILE_ADV_START
 * Samples the start time of a profiling period.
 *
 * @param   pProfileAdv Pointer to the STAMPROFILEADV structure to operate on.
 * @param   Prefix      Identifier prefix used to internal variables.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_PROFILE_ADV_START(pProfileAdv, Prefix) STAM_REL_PROFILE_ADV_START(pProfileAdv, Prefix)
#else
# define STAM_PROFILE_ADV_START(pProfileAdv, Prefix) do { } while (0)
#endif


/** @def STAM_REL_PROFILE_ADV_STOP
 * Samples the stop time of a profiling period (if running) and updates the
 * sample.
 *
 * @param   pProfileAdv Pointer to the STAMPROFILEADV structure to operate on.
 * @param   Prefix      Identifier prefix used to internal variables.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_PROFILE_ADV_STOP(pProfileAdv, Prefix) \
    do { \
        if ((pProfileAdv)->tsStart) \
        { \
            uint64_t Prefix##_cTicks; \
            STAM_GET_TS(Prefix##_cTicks); \
            Prefix##_cTicks -= (pProfileAdv)->tsStart; \
            (pProfileAdv)->tsStart = 0; \
            (pProfileAdv)->Core.cTicks += Prefix##_cTicks; \
            (pProfileAdv)->Core.cPeriods++; \
            if ((pProfileAdv)->Core.cTicksMax < Prefix##_cTicks) \
                (pProfileAdv)->Core.cTicksMax = Prefix##_cTicks; \
            if ((pProfileAdv)->Core.cTicksMin > Prefix##_cTicks) \
                (pProfileAdv)->Core.cTicksMin = Prefix##_cTicks; \
        } \
    } while (0)
#else
# define STAM_REL_PROFILE_ADV_STOP(pProfileAdv, Prefix) do { } while (0)
#endif
/** @def STAM_PROFILE_ADV_STOP
 * Samples the stop time of a profiling period (if running) and updates the
 * sample.
 *
 * @param   pProfileAdv Pointer to the STAMPROFILEADV structure to operate on.
 * @param   Prefix      Identifier prefix used to internal variables.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_PROFILE_ADV_STOP(pProfileAdv, Prefix) STAM_REL_PROFILE_ADV_STOP(pProfileAdv, Prefix)
#else
# define STAM_PROFILE_ADV_STOP(pProfileAdv, Prefix) do { } while (0)
#endif


/** @def STAM_REL_PROFILE_ADV_STOP_START
 * Stops one profile counter (if running) and starts another one.
 *
 * @param   pProfileAdv1    Pointer to the STAMPROFILEADV structure to stop.
 * @param   pProfileAdv2    Pointer to the STAMPROFILEADV structure to start.
 * @param   Prefix          Identifier prefix used to internal variables.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_PROFILE_ADV_STOP_START(pProfileAdv1, pProfileAdv2, Prefix) \
    do { \
        uint64_t Prefix##_cTicks; \
        STAM_GET_TS(Prefix##_cTicks); \
        (pProfileAdv2)->tsStart = Prefix##_cTicks; \
        if ((pProfileAdv1)->tsStart) \
        { \
            Prefix##_cTicks -= (pProfileAdv1)->tsStart; \
            (pProfileAdv1)->tsStart = 0; \
            (pProfileAdv1)->Core.cTicks += Prefix##_cTicks; \
            (pProfileAdv1)->Core.cPeriods++; \
            if ((pProfileAdv1)->Core.cTicksMax < Prefix##_cTicks) \
                (pProfileAdv1)->Core.cTicksMax = Prefix##_cTicks; \
            if ((pProfileAdv1)->Core.cTicksMin > Prefix##_cTicks) \
                (pProfileAdv1)->Core.cTicksMin = Prefix##_cTicks; \
        } \
    } while (0)
#else
# define STAM_REL_PROFILE_ADV_STOP_START(pProfileAdv1, pProfileAdv2, Prefix) \
    do { } while (0)
#endif
/** @def STAM_PROFILE_ADV_STOP_START
 * Samples the stop time of a profiling period (if running) and updates the
 * sample.
 *
 * @param   pProfileAdv1    Pointer to the STAMPROFILEADV structure to stop.
 * @param   pProfileAdv2    Pointer to the STAMPROFILEADV structure to start.
 * @param   Prefix          Identifier prefix used to internal variables.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_PROFILE_ADV_STOP_START(pProfileAdv1, pProfileAdv2, Prefix) \
    STAM_REL_PROFILE_ADV_STOP_START(pProfileAdv1, pProfileAdv2, Prefix)
#else
# define STAM_PROFILE_ADV_STOP_START(pProfileAdv1, pProfileAdv2, Prefix) \
    do { } while (0)
#endif


/** @def STAM_REL_PROFILE_ADV_SUSPEND
 * Suspends the sampling for a while. This can be useful to exclude parts
 * covered by other samples without screwing up the count, and average+min times.
 *
 * @param   pProfileAdv Pointer to the STAMPROFILEADV structure to operate on.
 * @param   Prefix      Identifier prefix used to internal variables. The prefix
 *                      must match that of the resume one since it stores the
 *                      suspend time in a stack variable.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_PROFILE_ADV_SUSPEND(pProfileAdv, Prefix) \
    uint64_t Prefix##_tsSuspend; \
    STAM_GET_TS(Prefix##_tsSuspend)
#else
# define STAM_REL_PROFILE_ADV_SUSPEND(pProfileAdv, Prefix) do { } while (0)
#endif
/** @def STAM_PROFILE_ADV_SUSPEND
 * Suspends the sampling for a while. This can be useful to exclude parts
 * covered by other samples without screwing up the count, and average+min times.
 *
 * @param   pProfileAdv Pointer to the STAMPROFILEADV structure to operate on.
 * @param   Prefix      Identifier prefix used to internal variables. The prefix
 *                      must match that of the resume one since it stores the
 *                      suspend time in a stack variable.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_PROFILE_ADV_SUSPEND(pProfileAdv, Prefix) STAM_REL_PROFILE_ADV_SUSPEND(pProfileAdv, Prefix)
#else
# define STAM_PROFILE_ADV_SUSPEND(pProfileAdv, Prefix) do { } while (0)
#endif


/** @def STAM_REL_PROFILE_ADV_RESUME
 * Counter to STAM_REL_PROFILE_ADV_SUSPEND.
 *
 * @param   pProfileAdv Pointer to the STAMPROFILEADV structure to operate on.
 * @param   Prefix      Identifier prefix used to internal variables. This must
 *                      match the one used with the SUSPEND!
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_PROFILE_ADV_RESUME(pProfileAdv, Prefix) \
    do { \
        uint64_t Prefix##_tsNow; \
        STAM_GET_TS(Prefix##_tsNow); \
        (pProfileAdv)->tsStart += Prefix##_tsNow - Prefix##_tsSuspend; \
    } while (0)
#else
# define STAM_REL_PROFILE_ADV_RESUME(pProfileAdv, Prefix) do { } while (0)
#endif
/** @def STAM_PROFILE_ADV_RESUME
 * Counter to STAM_PROFILE_ADV_SUSPEND.
 *
 * @param   pProfileAdv Pointer to the STAMPROFILEADV structure to operate on.
 * @param   Prefix      Identifier prefix used to internal variables. This must
 *                      match the one used with the SUSPEND!
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_PROFILE_ADV_RESUME(pProfileAdv, Prefix) STAM_REL_PROFILE_ADV_RESUME(pProfileAdv, Prefix)
#else
# define STAM_PROFILE_ADV_RESUME(pProfileAdv, Prefix) do { } while (0)
#endif


/** @def STAM_REL_PROFILE_ADV_STOP_EX
 * Samples the stop time of a profiling period (if running) and updates both
 * the sample and an attribution sample.
 *
 * @param   pProfileAdv Pointer to the STAMPROFILEADV structure to operate on.
 * @param   pProfile2   Pointer to the STAMPROFILE structure which this
 *                      interval should be attributed to as well. This may be NULL.
 * @param   Prefix      Identifier prefix used to internal variables.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_PROFILE_ADV_STOP_EX(pProfileAdv, pProfile2, Prefix) \
    do { \
        if ((pProfileAdv)->tsStart) \
        { \
            uint64_t Prefix##_cTicks; \
            STAM_GET_TS(Prefix##_cTicks); \
            Prefix##_cTicks -= (pProfileAdv)->tsStart; \
            (pProfileAdv)->tsStart = 0; \
            (pProfileAdv)->Core.cTicks += Prefix##_cTicks; \
            (pProfileAdv)->Core.cPeriods++; \
            if ((pProfileAdv)->Core.cTicksMax < Prefix##_cTicks) \
                (pProfileAdv)->Core.cTicksMax = Prefix##_cTicks; \
            if ((pProfileAdv)->Core.cTicksMin > Prefix##_cTicks) \
                (pProfileAdv)->Core.cTicksMin = Prefix##_cTicks; \
            if ((pProfile2)) \
            { \
                (pProfile2)->cTicks += Prefix##_cTicks; \
                (pProfile2)->cPeriods++; \
                if ((pProfile2)->cTicksMax < Prefix##_cTicks) \
                    (pProfile2)->cTicksMax = Prefix##_cTicks; \
                if ((pProfile2)->cTicksMin > Prefix##_cTicks) \
                    (pProfile2)->cTicksMin = Prefix##_cTicks; \
            } \
        } \
    } while (0)
#else
# define STAM_REL_PROFILE_ADV_STOP_EX(pProfileAdv, pProfile2, Prefix) do { } while (0)
#endif
/** @def STAM_PROFILE_ADV_STOP_EX
 * Samples the stop time of a profiling period (if running) and updates both
 * the sample and an attribution sample.
 *
 * @param   pProfileAdv Pointer to the STAMPROFILEADV structure to operate on.
 * @param   pProfile2   Pointer to the STAMPROFILE structure which this
 *                      interval should be attributed to as well. This may be NULL.
 * @param   Prefix      Identifier prefix used to internal variables.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_PROFILE_ADV_STOP_EX(pProfileAdv, pProfile2, Prefix) STAM_REL_PROFILE_ADV_STOP_EX(pProfileAdv, pProfile2, Prefix)
#else
# define STAM_PROFILE_ADV_STOP_EX(pProfileAdv, pProfile2, Prefix) do { } while (0)
#endif

/** @def STAM_REL_PROFILE_ADV_IS_RUNNING
 * Checks if it is running.
 *
 * @param   pProfileAdv Pointer to the STAMPROFILEADV structure to operate on.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_PROFILE_ADV_IS_RUNNING(pProfileAdv)   (pProfileAdv)->tsStart
#else
# define STAM_REL_PROFILE_ADV_IS_RUNNING(pProfileAdv)   (false)
#endif
/** @def STAM_PROFILE_ADV_IS_RUNNING
 * Checks if it is running.
 *
 * @param   pProfileAdv Pointer to the STAMPROFILEADV structure to operate on.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_PROFILE_ADV_IS_RUNNING(pProfileAdv) STAM_REL_PROFILE_ADV_IS_RUNNING(pProfileAdv)
#else
# define STAM_PROFILE_ADV_IS_RUNNING(pProfileAdv) (false)
#endif

/** @def STAM_REL_PROFILE_ADV_SET_STOPPED
 * Marks the profile counter as stopped.
 *
 * This is for avoiding screwups in twisty code.
 *
 * @param   pProfileAdv Pointer to the STAMPROFILEADV structure to operate on.
 */
#ifndef VBOX_WITHOUT_RELEASE_STATISTICS
# define STAM_REL_PROFILE_ADV_SET_STOPPED(pProfileAdv)   do { (pProfileAdv)->tsStart = 0; } while (0)
#else
# define STAM_REL_PROFILE_ADV_SET_STOPPED(pProfileAdv)   do { } while (0)
#endif
/** @def STAM_PROFILE_ADV_SET_STOPPED
 * Marks the profile counter as stopped.
 *
 * This is for avoiding screwups in twisty code.
 *
 * @param   pProfileAdv Pointer to the STAMPROFILEADV structure to operate on.
 */
#ifdef VBOX_WITH_STATISTICS
# define STAM_PROFILE_ADV_SET_STOPPED(pProfileAdv)      STAM_REL_PROFILE_ADV_SET_STOPPED(pProfileAdv)
#else
# define STAM_PROFILE_ADV_SET_STOPPED(pProfileAdv)      do { } while (0)
#endif


/**
 * Ratio of A to B, uint32_t types.
 * @remark Use STAM_STATS or STAM_REL_STATS for modifying A & B values.
 */
typedef struct STAMRATIOU32
{
    /** Sample A. */
    uint32_t volatile   u32A;
    /** Sample B. */
    uint32_t volatile   u32B;
} STAMRATIOU32;
/** Pointer to a uint32_t ratio. */
typedef STAMRATIOU32 *PSTAMRATIOU32;
/** Pointer to const a uint32_t ratio. */
typedef const STAMRATIOU32 *PCSTAMRATIOU32;




/** @defgroup grp_stam_r3   The STAM Host Context Ring 3 API
 * @{
 */

VMMR3DECL(int)  STAMR3InitUVM(PUVM pUVM);
VMMR3DECL(void) STAMR3TermUVM(PUVM pUVM);
VMMR3DECL(int)  STAMR3RegisterU(PUVM pUVM, void *pvSample, STAMTYPE enmType, STAMVISIBILITY enmVisibility,
                                const char *pszName, STAMUNIT enmUnit, const char *pszDesc);
VMMR3DECL(int)  STAMR3Register(PVM pVM, void *pvSample, STAMTYPE enmType, STAMVISIBILITY enmVisibility,
                               const char *pszName, STAMUNIT enmUnit, const char *pszDesc);

/** @def STAM_REL_REG
 * Registers a statistics sample.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pvSample    Pointer to the sample.
 * @param   enmType     Sample type. This indicates what pvSample is pointing at.
 * @param   pszName     Sample name. The name is on this form "/<component>/<sample>".
 *                      Further nesting is possible.
 * @param   enmUnit     Sample unit.
 * @param   pszDesc     Sample description.
 */
#define STAM_REL_REG(pVM, pvSample, enmType, pszName, enmUnit, pszDesc) \
    STAM_REL_STATS({ int rcStam = STAMR3Register(pVM, pvSample, enmType, STAMVISIBILITY_ALWAYS, pszName, enmUnit, pszDesc); \
                     AssertRC(rcStam); })
/** @def STAM_REG
 * Registers a statistics sample if statistics are enabled.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pvSample    Pointer to the sample.
 * @param   enmType     Sample type. This indicates what pvSample is pointing at.
 * @param   pszName     Sample name. The name is on this form "/<component>/<sample>".
 *                      Further nesting is possible.
 * @param   enmUnit     Sample unit.
 * @param   pszDesc     Sample description.
 */
#define STAM_REG(pVM, pvSample, enmType, pszName, enmUnit, pszDesc) \
    STAM_STATS({STAM_REL_REG(pVM, pvSample, enmType, pszName, enmUnit, pszDesc);})

/** @def STAM_REL_REG_USED
 * Registers a statistics sample which only shows when used.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pvSample    Pointer to the sample.
 * @param   enmType     Sample type. This indicates what pvSample is pointing at.
 * @param   pszName     Sample name. The name is on this form "/<component>/<sample>".
 *                      Further nesting is possible.
 * @param   enmUnit     Sample unit.
 * @param   pszDesc     Sample description.
 */
#define STAM_REL_REG_USED(pVM, pvSample, enmType, pszName, enmUnit, pszDesc) \
    STAM_REL_STATS({ int rcStam = STAMR3Register(pVM, pvSample, enmType, STAMVISIBILITY_USED, pszName, enmUnit, pszDesc); \
                     AssertRC(rcStam);})
/** @def STAM_REG_USED
 * Registers a statistics sample which only shows when used, if statistics are enabled.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pvSample    Pointer to the sample.
 * @param   enmType     Sample type. This indicates what pvSample is pointing at.
 * @param   pszName     Sample name. The name is on this form "/<component>/<sample>".
 *                      Further nesting is possible.
 * @param   enmUnit     Sample unit.
 * @param   pszDesc     Sample description.
 */
#define STAM_REG_USED(pVM, pvSample, enmType, pszName, enmUnit, pszDesc) \
    STAM_STATS({ STAM_REL_REG_USED(pVM, pvSample, enmType, pszName, enmUnit, pszDesc); })

VMMR3DECL(int)  STAMR3RegisterFU(PUVM pUVM, void *pvSample, STAMTYPE enmType, STAMVISIBILITY enmVisibility, STAMUNIT enmUnit,
                                 const char *pszDesc, const char *pszName, ...) RT_IPRT_FORMAT_ATTR(7, 8);
VMMR3DECL(int)  STAMR3RegisterF(PVM pVM, void *pvSample, STAMTYPE enmType, STAMVISIBILITY enmVisibility, STAMUNIT enmUnit,
                                const char *pszDesc, const char *pszName, ...) RT_IPRT_FORMAT_ATTR(7, 8);
VMMR3DECL(int)  STAMR3RegisterVU(PUVM pUVM, void *pvSample, STAMTYPE enmType, STAMVISIBILITY enmVisibility, STAMUNIT enmUnit,
                                 const char *pszDesc, const char *pszName, va_list args) RT_IPRT_FORMAT_ATTR(7, 0);
VMMR3DECL(int)  STAMR3RegisterV(PVM pVM, void *pvSample, STAMTYPE enmType, STAMVISIBILITY enmVisibility, STAMUNIT enmUnit,
                                const char *pszDesc, const char *pszName, va_list args) RT_IPRT_FORMAT_ATTR(7, 0);

/**
 * Resets the sample.
 * @param   pVM         The cross context VM structure.
 * @param   pvSample    The sample registered using STAMR3RegisterCallback.
 */
typedef DECLCALLBACKTYPE(void, FNSTAMR3CALLBACKRESET,(PVM pVM, void *pvSample));
/** Pointer to a STAM sample reset callback. */
typedef FNSTAMR3CALLBACKRESET *PFNSTAMR3CALLBACKRESET;

/**
 * Prints the sample into the buffer.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pvSample    The sample registered using STAMR3RegisterCallback.
 * @param   pszBuf      The buffer to print into.
 * @param   cchBuf      The size of the buffer.
 */
typedef DECLCALLBACKTYPE(void, FNSTAMR3CALLBACKPRINT,(PVM pVM, void *pvSample, char *pszBuf, size_t cchBuf));
/** Pointer to a STAM sample print callback. */
typedef FNSTAMR3CALLBACKPRINT *PFNSTAMR3CALLBACKPRINT;

VMMR3DECL(int)  STAMR3RegisterCallback(PVM pVM, void *pvSample, STAMVISIBILITY enmVisibility, STAMUNIT enmUnit,
                                       PFNSTAMR3CALLBACKRESET pfnReset, PFNSTAMR3CALLBACKPRINT pfnPrint,
                                       const char *pszDesc, const char *pszName, ...) RT_IPRT_FORMAT_ATTR(8, 9);
VMMR3DECL(int)  STAMR3RegisterCallbackV(PVM pVM, void *pvSample, STAMVISIBILITY enmVisibility, STAMUNIT enmUnit,
                                        PFNSTAMR3CALLBACKRESET pfnReset, PFNSTAMR3CALLBACKPRINT pfnPrint,
                                        const char *pszDesc, const char *pszName, va_list args) RT_IPRT_FORMAT_ATTR(8, 0);

VMMR3DECL(int)  STAMR3RegisterRefresh(PUVM pUVM, void *pvSample, STAMTYPE enmType, STAMVISIBILITY enmVisibility,
                                      STAMUNIT enmUnit, uint8_t iRefreshGrp, const char *pszDesc,
                                      const char *pszName, ...) RT_IPRT_FORMAT_ATTR(8, 9);
VMMR3DECL(int)  STAMR3RegisterRefreshV(PUVM pUVM, void *pvSample, STAMTYPE enmType, STAMVISIBILITY enmVisibility,
                                       STAMUNIT enmUnit, uint8_t iRefreshGrp, const char *pszDesc,
                                       const char *pszName, va_list va) RT_IPRT_FORMAT_ATTR(8, 0);

VMMR3DECL(int)  STAMR3Deregister(PUVM pUVM, const char *pszPat);
VMMR3DECL(int)  STAMR3DeregisterF(PUVM pUVM, const char *pszPatFmt, ...) RT_IPRT_FORMAT_ATTR(2, 3);
VMMR3DECL(int)  STAMR3DeregisterV(PUVM pUVM, const char *pszPatFmt, va_list va) RT_IPRT_FORMAT_ATTR(2, 0);
VMMR3DECL(int)  STAMR3DeregisterByPrefix(PUVM pUVM, const char *pszPrefix);
VMMR3DECL(int)  STAMR3DeregisterByAddr(PUVM pUVM, void *pvSample);

VMMR3DECL(int)  STAMR3Reset(PUVM pUVM, const char *pszPat);
VMMR3DECL(int)  STAMR3Snapshot(PUVM pUVM, const char *pszPat, char **ppszSnapshot, size_t *pcchSnapshot, bool fWithDesc);
VMMR3DECL(int)  STAMR3SnapshotFree(PUVM pUVM, char *pszSnapshot);
VMMR3DECL(int)  STAMR3Dump(PUVM pUVM, const char *pszPat);
VMMR3DECL(int)  STAMR3DumpToReleaseLog(PUVM pUVM, const char *pszPat);
VMMR3DECL(int)  STAMR3Print(PUVM pUVM, const char *pszPat);

/**
 * Callback function for STAMR3Enum().
 *
 * @returns non-zero to halt the enumeration.
 *
 * @param   pszName         The name of the sample.
 * @param   enmType         The type.
 * @param   pvSample        Pointer to the data. enmType indicates the format of this data.
 * @param   enmUnit         The unit.
 * @param   pszUnit         The unit as string.  This is a permanent string,
 *                          same as returned by STAMR3GetUnit().
 * @param   enmVisibility   The visibility.
 * @param   pszDesc         The description.
 * @param   pvUser          The pvUser argument given to STAMR3Enum().
 */
typedef DECLCALLBACKTYPE(int, FNSTAMR3ENUM,(const char *pszName, STAMTYPE enmType, void *pvSample, STAMUNIT enmUnit,
                                            const char *pszUnit, STAMVISIBILITY enmVisibility, const char *pszDesc, void *pvUser));
/** Pointer to a FNSTAMR3ENUM(). */
typedef FNSTAMR3ENUM *PFNSTAMR3ENUM;

VMMR3DECL(int)  STAMR3Enum(PUVM pUVM, const char *pszPat, PFNSTAMR3ENUM pfnEnum, void *pvUser);
VMMR3DECL(const char *) STAMR3GetUnit(STAMUNIT enmUnit);
VMMR3DECL(const char *) STAMR3GetUnit1(STAMUNIT enmUnit);
VMMR3DECL(const char *) STAMR3GetUnit2(STAMUNIT enmUnit);

/** @} */

/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_stam_h */

