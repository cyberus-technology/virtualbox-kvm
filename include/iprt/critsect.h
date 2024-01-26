/** @file
 * IPRT - Critical Sections.
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

#ifndef IPRT_INCLUDED_critsect_h
#define IPRT_INCLUDED_critsect_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/assert.h>
#if defined(IN_RING3) || defined(IN_RING0)
# include <iprt/thread.h>
#endif
#ifdef RT_LOCK_STRICT_ORDER
# include <iprt/lockvalidator.h>
#endif

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_critsect       RTCritSect - Critical Sections
 *
 * "Critical section" synchronization primitives can be used to
 * protect a section of code or data to which access must be exclusive;
 * only one thread can hold access to a critical section at one time.
 *
 * A critical section is a fast recursive write lock; if the critical
 * section is not acquired, then entering it is fast (requires no system
 * call). IPRT uses the Windows terminology here; on other platform, this
 * might be called a "futex" or a "fast mutex". As opposed to IPRT
 * "fast mutexes" (see @ref grp_rt_sems_fast_mutex ), critical sections
 * are recursive.
 *
 * Use RTCritSectInit to initialize a critical section; use RTCritSectEnter
 * and RTCritSectLeave to acquire and release access.
 *
 * For an overview of all types of synchronization primitives provided
 * by IPRT (event, mutex/fast mutex/read-write mutex semaphores), see
 * @ref grp_rt_sems .
 *
 * @ingroup grp_rt
 * @{
 */

/**
 * Critical section.
 */
typedef struct RTCRITSECT
{
    /** Magic used to validate the section state.
     * RTCRITSECT_MAGIC is the value of an initialized & operational section. */
    volatile uint32_t                   u32Magic;
    /** Number of lockers.
     * -1 if the section is free. */
    volatile int32_t                    cLockers;
    /** The owner thread. */
    volatile RTNATIVETHREAD             NativeThreadOwner;
    /** Number of nested enter operations performed.
     * Greater or equal to 1 if owned, 0 when free.
     */
    volatile int32_t                    cNestings;
    /** Section flags - the RTCRITSECT_FLAGS_* \#defines. */
    uint32_t                            fFlags;
    /** The semaphore to block on. */
    RTSEMEVENT                          EventSem;
    /** Lock validator record.  Only used in strict builds. */
    R3R0PTRTYPE(PRTLOCKVALRECEXCL)      pValidatorRec;
    /** Alignment padding. */
    RTHCPTR                             Alignment;
} RTCRITSECT;
AssertCompileSize(RTCRITSECT, HC_ARCH_BITS == 32 ? 32 : 48);

/** RTCRITSECT::u32Magic value. (Hiromi Uehara) */
#define RTCRITSECT_MAGIC                UINT32_C(0x19790326)

/** @name RTCritSectInitEx flags / RTCRITSECT::fFlags
 * @{ */
/** If set, nesting(/recursion) is not allowed. */
#define RTCRITSECT_FLAGS_NO_NESTING     UINT32_C(0x00000001)
/** Disables lock validation. */
#define RTCRITSECT_FLAGS_NO_LOCK_VAL    UINT32_C(0x00000002)
/** Bootstrap hack for use with certain memory allocator locks only! */
#define RTCRITSECT_FLAGS_BOOTSTRAP_HACK UINT32_C(0x00000004)
/** If set, the critical section becomes a dummy that doesn't serialize any
 * threads.  This flag can only be set at creation time.
 *
 * The intended use is avoiding lots of conditional code where some component
 * might or might not require entering a critical section before access. */
#define RTCRITSECT_FLAGS_NOP            UINT32_C(0x00000008)
/** Indicates that this is a ring-0 critical section. */
#define RTCRITSECT_FLAGS_RING0          UINT32_C(0x00000010)
/** @} */


#if defined(IN_RING3) || defined(IN_RING0)

/**
 * Initialize a critical section.
 */
RTDECL(int) RTCritSectInit(PRTCRITSECT pCritSect);

/**
 * Initialize a critical section.
 *
 * @returns iprt status code.
 * @param   pCritSect       Pointer to the critical section structure.
 * @param   fFlags          Flags, any combination of the RTCRITSECT_FLAGS
 *                          \#defines.
 * @param   hClass          The class (no reference consumed).  If NIL, no lock
 *                          order validation will be performed on this lock.
 * @param   uSubClass       The sub-class.  This is used to define lock order
 *                          within a class.  RTLOCKVAL_SUB_CLASS_NONE is the
 *                          recommended value here.
 * @param   pszNameFmt      Name format string for the lock validator, optional
 *                          (NULL).  Max length is 32 bytes.
 * @param   ...             Format string arguments.
 */
RTDECL(int) RTCritSectInitEx(PRTCRITSECT pCritSect, uint32_t fFlags, RTLOCKVALCLASS hClass, uint32_t uSubClass,
                             const char *pszNameFmt, ...) RT_IPRT_FORMAT_ATTR_MAYBE_NULL(5, 6);

/**
 * Changes the lock validator sub-class of the critical section.
 *
 * It is recommended to try make sure that nobody is using this critical section
 * while changing the value.
 *
 * @returns The old sub-class.  RTLOCKVAL_SUB_CLASS_INVALID is returns if the
 *          lock validator isn't compiled in or either of the parameters are
 *          invalid.
 * @param   pCritSect       The critical section.
 * @param   uSubClass       The new sub-class value.
 */
RTDECL(uint32_t) RTCritSectSetSubClass(PRTCRITSECT pCritSect, uint32_t uSubClass);

/**
 * Enter a critical section.
 *
 * @returns VINF_SUCCESS on success.
 * @returns VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 * @param   pCritSect       The critical section.
 */
RTDECL(int) RTCritSectEnter(PRTCRITSECT pCritSect);

/**
 * Enter a critical section.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 *
 * @param   pCritSect       The critical section.
 * @param   uId             Where we're entering the section.
 * @param   SRC_POS         The source position where call is being made from.
 *                          Use RT_SRC_POS when possible.  Optional.
 */
RTDECL(int) RTCritSectEnterDebug(PRTCRITSECT pCritSect, RTHCUINTPTR uId, RT_SRC_POS_DECL);

/**
 * Try enter a critical section.
 *
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_SEM_BUSY if the critsect was owned.
 * @retval  VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 *
 * @param   pCritSect   The critical section.
 */
RTDECL(int) RTCritSectTryEnter(PRTCRITSECT pCritSect);

/**
 * Try enter a critical section.
 *
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_SEM_BUSY if the critsect was owned.
 * @retval  VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 *
 * @param   pCritSect       The critical section.
 * @param   uId             Where we're entering the section.
 * @param   SRC_POS         The source position where call is being made from.
 *                          Use RT_SRC_POS when possible.  Optional.
 */
RTDECL(int) RTCritSectTryEnterDebug(PRTCRITSECT pCritSect, RTHCUINTPTR uId, RT_SRC_POS_DECL);

# ifdef IN_RING3 /* Crazy APIs: ring-3 only. */

/**
 * Enter multiple critical sections.
 *
 * This function will enter ALL the specified critical sections before returning.
 *
 * @returns VINF_SUCCESS on success.
 * @returns VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 * @param   cCritSects      Number of critical sections in the array.
 * @param   papCritSects    Array of critical section pointers.
 *
 * @remark  Please note that this function will not necessarily come out favourable in a
 *          fight with other threads which are using the normal RTCritSectEnter() function.
 *          Therefore, avoid having to enter multiple critical sections!
 */
RTDECL(int) RTCritSectEnterMultiple(size_t cCritSects, PRTCRITSECT *papCritSects);

/**
 * Enter multiple critical sections.
 *
 * This function will enter ALL the specified critical sections before returning.
 *
 * @returns VINF_SUCCESS on success.
 * @returns VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 *
 * @param   cCritSects      Number of critical sections in the array.
 * @param   papCritSects    Array of critical section pointers.
 * @param   uId             Where we're entering the section.
 * @param   SRC_POS         The source position where call is being made from.
 *                          Use RT_SRC_POS when possible.  Optional.
 *
 * @remark  See RTCritSectEnterMultiple().
 */
RTDECL(int) RTCritSectEnterMultipleDebug(size_t cCritSects, PRTCRITSECT *papCritSects, RTHCUINTPTR uId, RT_SRC_POS_DECL);

# endif /* IN_RING3 */

/**
 * Leave a critical section.
 *
 * @returns VINF_SUCCESS.
 * @param   pCritSect       The critical section.
 */
RTDECL(int) RTCritSectLeave(PRTCRITSECT pCritSect);

/**
 * Leave multiple critical sections.
 *
 * @returns VINF_SUCCESS.
 * @param   cCritSects      Number of critical sections in the array.
 * @param   papCritSects    Array of critical section pointers.
 */
RTDECL(int) RTCritSectLeaveMultiple(size_t cCritSects, PRTCRITSECT *papCritSects);

/**
 * Deletes a critical section.
 *
 * @returns VINF_SUCCESS.
 * @param   pCritSect       The critical section.
 */
RTDECL(int) RTCritSectDelete(PRTCRITSECT pCritSect);

/**
 * Checks the caller is the owner of the critical section.
 *
 * @returns true if owner.
 * @returns false if not owner.
 * @param   pCritSect       The critical section.
 */
DECLINLINE(bool) RTCritSectIsOwner(PCRTCRITSECT pCritSect)
{
    return pCritSect->NativeThreadOwner == RTThreadNativeSelf();
}

#endif /* IN_RING3 || IN_RING0 */

/**
 * Checks the section is owned by anyone.
 *
 * @returns true if owned.
 * @returns false if not owned.
 * @param   pCritSect       The critical section.
 */
DECLINLINE(bool) RTCritSectIsOwned(PCRTCRITSECT pCritSect)
{
    return pCritSect->NativeThreadOwner != NIL_RTNATIVETHREAD;
}

/**
 * Gets the thread id of the critical section owner.
 *
 * @returns Thread id of the owner thread if owned.
 * @returns NIL_RTNATIVETHREAD is not owned.
 * @param   pCritSect       The critical section.
 */
DECLINLINE(RTNATIVETHREAD) RTCritSectGetOwner(PCRTCRITSECT pCritSect)
{
    return pCritSect->NativeThreadOwner;
}

/**
 * Checks if a critical section is initialized or not.
 *
 * @returns true if initialized.
 * @returns false if not initialized.
 * @param   pCritSect       The critical section.
 */
DECLINLINE(bool) RTCritSectIsInitialized(PCRTCRITSECT pCritSect)
{
    return pCritSect->u32Magic == RTCRITSECT_MAGIC;
}

/**
 * Gets the recursion depth.
 *
 * @returns The recursion depth.
 * @param   pCritSect       The Critical section
 */
DECLINLINE(uint32_t) RTCritSectGetRecursion(PCRTCRITSECT pCritSect)
{
    return (uint32_t)pCritSect->cNestings;
}

/**
 * Gets the waiter count
 *
 * @returns The waiter count
 * @param   pCritSect       The Critical section
 */
DECLINLINE(int32_t) RTCritSectGetWaiters(PCRTCRITSECT pCritSect)
{
    return pCritSect->cLockers;
}

/* Lock strict build: Remap the three enter calls to the debug versions. */
#if defined(RT_LOCK_STRICT) && !defined(RTCRITSECT_WITHOUT_REMAPPING) && !defined(RT_WITH_MANGLING)
# ifdef IPRT_INCLUDED_asm_h
#  define RTCritSectEnter(pCritSect)                        RTCritSectEnterDebug(pCritSect, (uintptr_t)ASMReturnAddress(), RT_SRC_POS)
#  define RTCritSectTryEnter(pCritSect)                     RTCritSectTryEnterDebug(pCritSect, (uintptr_t)ASMReturnAddress(), RT_SRC_POS)
#  define RTCritSectEnterMultiple(cCritSects, pCritSect)    RTCritSectEnterMultipleDebug((cCritSects), (pCritSect), (uintptr_t)ASMReturnAddress(), RT_SRC_POS)
# else
#  define RTCritSectEnter(pCritSect)                        RTCritSectEnterDebug(pCritSect, 0, RT_SRC_POS)
#  define RTCritSectTryEnter(pCritSect)                     RTCritSectTryEnterDebug(pCritSect, 0, RT_SRC_POS)
#  define RTCritSectEnterMultiple(cCritSects, pCritSect)    RTCritSectEnterMultipleDebug((cCritSects), (pCritSect), 0, RT_SRC_POS)
# endif
#endif

/* Strict lock order: Automatically classify locks by init location. */
#if defined(RT_LOCK_STRICT_ORDER) && defined(IN_RING3) && !defined(RTCRITSECT_WITHOUT_REMAPPING) && !defined(RT_WITH_MANGLING)
# define RTCritSectInit(pCritSect) \
    RTCritSectInitEx((pCritSect), 0 /*fFlags*/, \
                     RTLockValidatorClassForSrcPos(RT_SRC_POS, NULL), \
                     RTLOCKVAL_SUB_CLASS_NONE, NULL)
#endif

/** @}  */



/** @defgroup grp_rt_critsectrw     RTCritSectRw - Read/Write Critical Sections
 * @ingroup grp_rt
 * @{
 */

/**
 * Union that allows us to atomically update both the state and
 * exclusive owner if the hardware supports cmpxchg16b or similar.
 */
typedef union RTCRITSECTRWSTATE
{
    struct
    {
        /** The state variable.
         * All accesses are atomic and it bits are defined like this:
         *      Bits 0..14  - cReads.
         *      Bit 15      - Unused.
         *      Bits 16..31 - cWrites.
         *      Bit 31      - fDirection; 0=Read, 1=Write.
         *      Bits 32..46 - cWaitingReads
         *      Bit 47      - Unused.
         *      Bits 48..62 - cWaitingWrites - doesn't make sense here, not used.
         *      Bit 63      - Unused.
         */
        uint64_t                    u64State;
        /** The write owner. */
        RTNATIVETHREAD              hNativeWriter;
    } s;
    RTUINT128U                      u128;
} RTCRITSECTRWSTATE;


/**
 * Read/write critical section.
 */
typedef struct RTCRITSECTRW
{
    /** Magic used to validate the section state.
     * RTCRITSECTRW_MAGIC is the value of an initialized & operational section. */
    volatile uint32_t                   u32Magic;

    /** Indicates whether hEvtRead needs resetting. */
    bool volatile                       fNeedReset;
    /** Explicit alignment padding. */
    bool volatile                       afPadding[1];
    /** Section flags - the RTCRITSECT_FLAGS_* \#defines. */
    uint16_t                            fFlags;

    /** The number of reads made by the current writer. */
    uint32_t volatile                   cWriterReads;
    /** The number of recursions made by the current writer. (The initial grabbing
     *  of the lock counts as the first one.) */
    uint32_t volatile                   cWriteRecursions;
    /** The core state. */
    RTCRITSECTRWSTATE volatile          u;

    /** What the writer threads are blocking on. */
    RTSEMEVENT                          hEvtWrite;
    /** What the read threads are blocking on when waiting for the writer to
     * finish. */
    RTSEMEVENTMULTI                     hEvtRead;

    /** The validator record for the writer. */
    R3R0PTRTYPE(PRTLOCKVALRECEXCL)      pValidatorWrite;
    /** The validator record for the readers. */
    R3R0PTRTYPE(PRTLOCKVALRECSHRD)      pValidatorRead;
} RTCRITSECTRW;
AssertCompileSize(RTCRITSECTRW, HC_ARCH_BITS == 32 ? 48 : 64);

/** RTCRITSECTRW::u32Magic value. (Eric Allan Dolphy, Jr.) */
#define RTCRITSECTRW_MAGIC              UINT32_C(0x19280620)
/** RTCRITSECTRW::u32Magic dead value. */
#define RTCRITSECTRW_MAGIC_DEAD         UINT32_C(0x19640629)

/** @name RTCRITSECTRW::u64State values.
 * @note Using RTCSRW instead of RTCRITSECTRW to save space.
 * @{ */
#define RTCSRW_CNT_BITS            15
#define RTCSRW_CNT_MASK            UINT64_C(0x00007fff)

#define RTCSRW_CNT_RD_SHIFT        0
#define RTCSRW_CNT_RD_MASK         (RTCSRW_CNT_MASK << RTCSRW_CNT_RD_SHIFT)
#define RTCSRW_CNT_WR_SHIFT        16
#define RTCSRW_CNT_WR_MASK         (RTCSRW_CNT_MASK << RTCSRW_CNT_WR_SHIFT)

#define RTCSRW_DIR_SHIFT           31
#define RTCSRW_DIR_MASK            RT_BIT_64(RTCSRW_DIR_SHIFT)
#define RTCSRW_DIR_READ            UINT64_C(0)
#define RTCSRW_DIR_WRITE           UINT64_C(1)

#define RTCSRW_WAIT_CNT_RD_SHIFT   32
#define RTCSRW_WAIT_CNT_RD_MASK    (RTCSRW_CNT_MASK << RTCSRW_WAIT_CNT_RD_SHIFT)
/* #define RTCSRW_WAIT_CNT_WR_SHIFT   48 */
/* #define RTCSRW_WAIT_CNT_WR_MASK    (RTCSRW_CNT_MASK << RTCSRW_WAIT_CNT_WR_SHIFT) */
/** @} */

#if defined(IN_RING3) || defined(IN_RING0)

/**
 * Initialize a critical section.
 */
RTDECL(int) RTCritSectRwInit(PRTCRITSECTRW pThis);

/**
 * Initialize a critical section.
 *
 * @returns IPRT status code.
 * @param   pThis           Pointer to the read/write critical section.
 * @param   fFlags          Flags, any combination of the RTCRITSECT_FLAGS
 *                          \#defines.
 * @param   hClass          The class (no reference consumed).  If NIL, no lock
 *                          order validation will be performed on this lock.
 * @param   uSubClass       The sub-class.  This is used to define lock order
 *                          within a class.  RTLOCKVAL_SUB_CLASS_NONE is the
 *                          recommended value here.
 * @param   pszNameFmt      Name format string for the lock validator, optional
 *                          (NULL).  Max length is 32 bytes.
 * @param   ...             Format string arguments.
 */
RTDECL(int) RTCritSectRwInitEx(PRTCRITSECTRW pThis, uint32_t fFlags, RTLOCKVALCLASS hClass, uint32_t uSubClass,
                               const char *pszNameFmt, ...) RT_IPRT_FORMAT_ATTR_MAYBE_NULL(5, 6);

/**
 * Changes the lock validator sub-class of the critical section.
 *
 * It is recommended to try make sure that nobody is using this critical section
 * while changing the value.
 *
 * @returns The old sub-class.  RTLOCKVAL_SUB_CLASS_INVALID is returns if the
 *          lock validator isn't compiled in or either of the parameters are
 *          invalid.
 * @param   pThis           Pointer to the read/write critical section.
 * @param   uSubClass       The new sub-class value.
 */
RTDECL(uint32_t) RTCritSectRwSetSubClass(PRTCRITSECTRW pThis, uint32_t uSubClass);


/**
 * Enter a critical section with shared (read) access.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 * @param   pThis           Pointer to the read/write critical section.
 */
RTDECL(int) RTCritSectRwEnterShared(PRTCRITSECTRW pThis);

/**
 * Enter a critical section with shared (read) access.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 *
 * @param   pThis           Pointer to the read/write critical section.
 * @param   uId             Where we're entering the section.
 * @param   SRC_POS         The source position where call is being made from.
 *                          Use RT_SRC_POS when possible.  Optional.
 */
RTDECL(int) RTCritSectRwEnterSharedDebug(PRTCRITSECTRW pThis, RTHCUINTPTR uId, RT_SRC_POS_DECL);

/**
 * Try enter a critical section with shared (read) access.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_SEM_BUSY if the critsect was owned.
 * @retval  VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 *
 * @param   pThis           Pointer to the read/write critical section.
 */
RTDECL(int) RTCritSectRwTryEnterShared(PRTCRITSECTRW pThis);

/**
 * Try enter a critical section with shared (read) access.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_SEM_BUSY if the critsect was owned.
 * @retval  VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 *
 * @param   pThis           Pointer to the read/write critical section.
 * @param   uId             Where we're entering the section.
 * @param   SRC_POS         The source position where call is being made from.
 *                          Use RT_SRC_POS when possible.  Optional.
 */
RTDECL(int) RTCritSectRwTryEnterSharedDebug(PRTCRITSECTRW pThis, RTHCUINTPTR uId, RT_SRC_POS_DECL);

/**
 * Leave a critical section held with shared access.
 *
 * @returns IPRT status code.
 * @param   pThis           Pointer to the read/write critical section.
 */
RTDECL(int) RTCritSectRwLeaveShared(PRTCRITSECTRW pThis);


/**
 * Enter a critical section with exclusive (write) access.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 * @param   pThis           Pointer to the read/write critical section.
 */
RTDECL(int) RTCritSectRwEnterExcl(PRTCRITSECTRW pThis);

/**
 * Enter a critical section with exclusive (write) access.
 *
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 *
 * @param   pThis           Pointer to the read/write critical section.
 * @param   uId             Where we're entering the section.
 * @param   SRC_POS         The source position where call is being made from.
 *                          Use RT_SRC_POS when possible.  Optional.
 */
RTDECL(int) RTCritSectRwEnterExclDebug(PRTCRITSECTRW pThis, RTHCUINTPTR uId, RT_SRC_POS_DECL);

/**
 * Try enter a critical section with exclusive (write) access.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_SEM_BUSY if the critsect was owned.
 * @retval  VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 *
 * @param   pThis           Pointer to the read/write critical section.
 */
RTDECL(int) RTCritSectRwTryEnterExcl(PRTCRITSECTRW pThis);

/**
 * Try enter a critical section with exclusive (write) access.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_SEM_BUSY if the critsect was owned.
 * @retval  VERR_SEM_NESTED if nested enter on a no nesting section. (Asserted.)
 * @retval  VERR_SEM_DESTROYED if the critical section is delete before or
 *          during the operation.
 *
 * @param   pThis           Pointer to the read/write critical section.
 * @param   uId             Where we're entering the section.
 * @param   SRC_POS         The source position where call is being made from.
 *                          Use RT_SRC_POS when possible.  Optional.
 */
RTDECL(int) RTCritSectRwTryEnterExclDebug(PRTCRITSECTRW pThis, RTHCUINTPTR uId, RT_SRC_POS_DECL);

/**
 * Leave a critical section held exclusively.
 *
 * @returns IPRT status code; VINF_SUCCESS, VERR_NOT_OWNER, VERR_SEM_DESTROYED,
 *          or VERR_WRONG_ORDER.
 * @param   pThis           Pointer to the read/write critical section.
 */
RTDECL(int) RTCritSectRwLeaveExcl(PRTCRITSECTRW pThis);


/**
 * Deletes a critical section.
 *
 * @returns VINF_SUCCESS.
 * @param   pThis           Pointer to the read/write critical section.
 */
RTDECL(int) RTCritSectRwDelete(PRTCRITSECTRW pThis);

/**
 * Checks the caller is the exclusive (write) owner of the critical section.
 *
 * @retval  true if owner.
 * @retval  false if not owner.
 * @param   pThis           Pointer to the read/write critical section.
 */
RTDECL(bool) RTCritSectRwIsWriteOwner(PRTCRITSECTRW pThis);

/**
 * Checks if the caller is one of the read owners of the critical section.
 *
 * @note    !CAUTION!  This API doesn't work reliably if lock validation isn't
 *          enabled. Meaning, the answer is not trustworhty unless
 *          RT_LOCK_STRICT or RTCRITSECTRW_STRICT was defined at build time.
 *          Also, make sure you do not use RTCRITSECTRW_FLAGS_NO_LOCK_VAL when
 *          creating the semaphore.  And finally, if you used a locking class,
 *          don't disable deadlock detection by setting cMsMinDeadlock to
 *          RT_INDEFINITE_WAIT.
 *
 *          In short, only use this for assertions.
 *
 * @returns @c true if reader, @c false if not.
 * @param   pThis           Pointer to the read/write critical section.
 * @param   fWannaHear      What you'd like to hear when lock validation is not
 *                          available.  (For avoiding asserting all over the
 *                          place.)
 */
RTDECL(bool) RTCritSectRwIsReadOwner(PRTCRITSECTRW pThis, bool fWannaHear);

/**
 * Gets the write recursion count.
 *
 * @returns The write recursion count (0 if bad critsect).
 * @param   pThis           Pointer to the read/write critical section.
 */
RTDECL(uint32_t) RTCritSectRwGetWriteRecursion(PRTCRITSECTRW pThis);

/**
 * Gets the read recursion count of the current writer.
 *
 * @returns The read recursion count (0 if bad critsect).
 * @param   pThis           Pointer to the read/write critical section.
 */
RTDECL(uint32_t) RTCritSectRwGetWriterReadRecursion(PRTCRITSECTRW pThis);

/**
 * Gets the current number of reads.
 *
 * This includes all read recursions, so it might be higher than the number of
 * read owners.  It does not include reads done by the current writer.
 *
 * @returns The read count (0 if bad critsect).
 * @param   pThis           Pointer to the read/write critical section.
 */
RTDECL(uint32_t) RTCritSectRwGetReadCount(PRTCRITSECTRW pThis);

#endif /* IN_RING3 || IN_RING0 */

/**
 * Checks if a critical section is initialized or not.
 *
 * @retval  true if initialized.
 * @retval  false if not initialized.
 * @param   pThis           Pointer to the read/write critical section.
 */
DECLINLINE(bool) RTCritSectRwIsInitialized(PCRTCRITSECTRW pThis)
{
    return pThis->u32Magic == RTCRITSECTRW_MAGIC;
}

/* Lock strict build: Remap the three enter calls to the debug versions. */
#if defined(RT_LOCK_STRICT) && !defined(RTCRITSECTRW_WITHOUT_REMAPPING) && !defined(RT_WITH_MANGLING)
# ifdef IPRT_INCLUDED_asm_h
#  define RTCritSectRwEnterExcl(pThis)      RTCritSectRwEnterExclDebug(pThis, (uintptr_t)ASMReturnAddress(), RT_SRC_POS)
#  define RTCritSectRwTryEnterExcl(pThis)   RTCritSectRwTryEnterExclDebug(pThis, (uintptr_t)ASMReturnAddress(), RT_SRC_POS)
#  define RTCritSectRwEnterShared(pThis)    RTCritSectRwEnterSharedDebug(pThis, (uintptr_t)ASMReturnAddress(), RT_SRC_POS)
#  define RTCritSectRwTryEnterShared(pThis) RTCritSectRwTryEnterSharedDebug(pThis, (uintptr_t)ASMReturnAddress(), RT_SRC_POS)
# else
#  define RTCritSectRwEnterExcl(pThis)      RTCritSectRwEnterExclDebug(pThis, 0, RT_SRC_POS)
#  define RTCritSectRwTryEnterExcl(pThis)   RTCritSectRwTryEnterExclDebug(pThis, 0, RT_SRC_POS)
#  define RTCritSectRwEnterShared(pThis)    RTCritSectRwEnterSharedDebug(pThis, 0, RT_SRC_POS)
#  define RTCritSectRwTryEnterShared(pThis) RTCritSectRwTryEnterSharedDebug(pThis, 0, RT_SRC_POS)
# endif
#endif

/* Strict lock order: Automatically classify locks by init location. */
#if defined(RT_LOCK_STRICT_ORDER) && defined(IN_RING3) && !defined(RTCRITSECTRW_WITHOUT_REMAPPING) && !defined(RT_WITH_MANGLING)
# define RTCritSectRwInit(a_pThis) \
    RTCritSectRwInitEx((a_pThis), 0 /*fFlags*/, \
                       RTLockValidatorClassForSrcPos(RT_SRC_POS, NULL), \
                        RTLOCKVAL_SUB_CLASS_NONE, NULL)
#endif

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_critsect_h */

