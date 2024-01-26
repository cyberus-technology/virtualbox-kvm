/** @file
 * IPRT - Assembly Functions.
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

#ifndef IPRT_INCLUDED_asm_h
#define IPRT_INCLUDED_asm_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/assert.h>
/** @def RT_INLINE_ASM_USES_INTRIN
 * Defined as 1 if we're using a _MSC_VER 1400.
 * Otherwise defined as 0.
 */

/* Solaris 10 header ugliness */
#ifdef u
# undef u
#endif

#if defined(_MSC_VER) && RT_INLINE_ASM_USES_INTRIN
/* Emit the intrinsics at all optimization levels. */
# include <iprt/sanitized/intrin.h>
# pragma intrinsic(_ReadWriteBarrier)
# pragma intrinsic(__cpuid)
# pragma intrinsic(__stosd)
# pragma intrinsic(__stosw)
# pragma intrinsic(__stosb)
# pragma intrinsic(_BitScanForward)
# pragma intrinsic(_BitScanReverse)
# pragma intrinsic(_bittest)
# pragma intrinsic(_bittestandset)
# pragma intrinsic(_bittestandreset)
# pragma intrinsic(_bittestandcomplement)
# pragma intrinsic(_byteswap_ushort)
# pragma intrinsic(_byteswap_ulong)
# pragma intrinsic(_interlockedbittestandset)
# pragma intrinsic(_interlockedbittestandreset)
# pragma intrinsic(_InterlockedAnd)
# pragma intrinsic(_InterlockedOr)
# pragma intrinsic(_InterlockedXor)
# pragma intrinsic(_InterlockedIncrement)
# pragma intrinsic(_InterlockedDecrement)
# pragma intrinsic(_InterlockedExchange)
# pragma intrinsic(_InterlockedExchangeAdd)
# pragma intrinsic(_InterlockedCompareExchange)
# pragma intrinsic(_InterlockedCompareExchange8)
# pragma intrinsic(_InterlockedCompareExchange16)
# pragma intrinsic(_InterlockedCompareExchange64)
# pragma intrinsic(_rotl)
# pragma intrinsic(_rotr)
# pragma intrinsic(_rotl64)
# pragma intrinsic(_rotr64)
# ifdef RT_ARCH_AMD64
#  pragma intrinsic(__stosq)
#  pragma intrinsic(_byteswap_uint64)
#  pragma intrinsic(_InterlockedCompareExchange128)
#  pragma intrinsic(_InterlockedExchange64)
#  pragma intrinsic(_InterlockedExchangeAdd64)
#  pragma intrinsic(_InterlockedAnd64)
#  pragma intrinsic(_InterlockedOr64)
#  pragma intrinsic(_InterlockedIncrement64)
#  pragma intrinsic(_InterlockedDecrement64)
# endif
#endif

/*
 * Undefine all symbols we have Watcom C/C++ #pragma aux'es for.
 */
#if defined(__WATCOMC__) && ARCH_BITS == 16 && defined(RT_ARCH_X86)
# include "asm-watcom-x86-16.h"
#elif defined(__WATCOMC__) && ARCH_BITS == 32 && defined(RT_ARCH_X86)
# include "asm-watcom-x86-32.h"
#endif


/** @defgroup grp_rt_asm    ASM - Assembly Routines
 * @ingroup grp_rt
 *
 * @remarks The difference between ordered and unordered atomic operations are
 *          that the former will complete outstanding reads and writes before
 *          continuing while the latter doesn't make any promises about the
 *          order.  Ordered operations doesn't, it seems, make any 100% promise
 *          wrt to whether the operation will complete before any subsequent
 *          memory access.  (please, correct if wrong.)
 *
 *          ASMAtomicSomething operations are all ordered, while
 *          ASMAtomicUoSomething are unordered (note the Uo).
 *
 *          Please note that ordered operations does not necessarily imply a
 *          compiler (memory) barrier.   The user has to use the
 *          ASMCompilerBarrier() macro when that is deemed necessary.
 *
 * @remarks Some remarks about __volatile__: Without this keyword gcc is allowed
 *          to reorder or even optimize assembler instructions away.  For
 *          instance, in the following code the second rdmsr instruction is
 *          optimized away because gcc treats that instruction as deterministic:
 *
 *            @code
 *            static inline uint64_t rdmsr_low(int idx)
 *            {
 *              uint32_t low;
 *              __asm__ ("rdmsr" : "=a"(low) : "c"(idx) : "edx");
 *            }
 *            ...
 *            uint32_t msr1 = rdmsr_low(1);
 *            foo(msr1);
 *            msr1 = rdmsr_low(1);
 *            bar(msr1);
 *            @endcode
 *
 *          The input parameter of rdmsr_low is the same for both calls and
 *          therefore gcc will use the result of the first call as input
 *          parameter for bar() as well. For rdmsr this is not acceptable as
 *          this instruction is _not_ deterministic. This applies to reading
 *          machine status information in general.
 *
 * @{
 */


/** @def RT_INLINE_ASM_GCC_4_3_X_X86
 * Used to work around some 4.3.x register allocation issues in this version of
 * the compiler. So far this workaround is still required for 4.4 and 4.5 but
 * definitely not for 5.x */
#if (RT_GNUC_PREREQ(4, 3) && !RT_GNUC_PREREQ(5, 0) && defined(__i386__))
# define RT_INLINE_ASM_GCC_4_3_X_X86 1
#else
# define RT_INLINE_ASM_GCC_4_3_X_X86 0
#endif

/** @def RT_INLINE_DONT_MIX_CMPXCHG8B_AND_PIC
 * i686-apple-darwin9-gcc-4.0.1 (GCC) 4.0.1 (Apple Inc. build 5493) screws up
 * RTSemRWRequestWrite semsemrw-lockless-generic.cpp in release builds. PIC
 * mode, x86.
 *
 * Some gcc 4.3.x versions may have register allocation issues with cmpxchg8b
 * when in PIC mode on x86.
 */
#ifndef RT_INLINE_DONT_MIX_CMPXCHG8B_AND_PIC
# if defined(DOXYGEN_RUNNING) || defined(__WATCOMC__) /* Watcom has trouble with the expression below */
#  define RT_INLINE_DONT_MIX_CMPXCHG8B_AND_PIC 1
# elif defined(_MSC_VER) /* Visual C++ has trouble too, but it'll only tell us when C4688 is enabled. */
#  define RT_INLINE_DONT_MIX_CMPXCHG8B_AND_PIC 0
# elif (   (defined(PIC) || defined(__PIC__)) \
        && defined(RT_ARCH_X86) \
        && (   RT_INLINE_ASM_GCC_4_3_X_X86 \
            || defined(RT_OS_DARWIN)) )
#  define RT_INLINE_DONT_MIX_CMPXCHG8B_AND_PIC 1
# else
#  define RT_INLINE_DONT_MIX_CMPXCHG8B_AND_PIC 0
# endif
#endif


/** @def RT_INLINE_ASM_EXTERNAL_TMP_ARM
 * Temporary version of RT_INLINE_ASM_EXTERNAL that excludes ARM. */
#if RT_INLINE_ASM_EXTERNAL && !(defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32))
# define RT_INLINE_ASM_EXTERNAL_TMP_ARM 1
#else
# define RT_INLINE_ASM_EXTERNAL_TMP_ARM 0
#endif

/*
 * ARM is great fun.
 */
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)

# define RTASM_ARM_NO_BARRIER
# ifdef RT_ARCH_ARM64
#  define RTASM_ARM_NO_BARRIER_IN_REG
#  define RTASM_ARM_NO_BARRIER_COMMA_IN_REG
#  define RTASM_ARM_DSB_SY              "dsb    sy\n\t"
#  define RTASM_ARM_DSB_SY_IN_REG
#  define RTASM_ARM_DSB_SY_COMMA_IN_REG
#  define RTASM_ARM_DMB_SY              "dmb    sy\n\t"
#  define RTASM_ARM_DMB_SY_IN_REG
#  define RTASM_ARM_DMB_SY_COMMA_IN_REG
#  define RTASM_ARM_DMB_ST              "dmb    st\n\t"
#  define RTASM_ARM_DMB_ST_IN_REG
#  define RTASM_ARM_DMB_ST_COMMA_IN_REG
#  define RTASM_ARM_DMB_LD              "dmb    ld\n\t"
#  define RTASM_ARM_DMB_LD_IN_REG
#  define RTASM_ARM_DMB_LD_COMMA_IN_REG
#  define RTASM_ARM_PICK_6432(expr64, expr32) expr64
#  define RTASM_ARM_LOAD_MODIFY_STORE_RET_NEW_32(name, a_pu32Mem, barrier_type, modify64, modify32, in_reg) \
    uint32_t rcSpill; \
    uint32_t u32NewRet; \
    __asm__ __volatile__(".Ltry_again_" #name "_%=:\n\t" \
                         RTASM_ARM_##barrier_type /* before lable? */ \
                         "ldaxr     %w[uNew], %[pMem]\n\t" \
                         modify64 \
                         "stlxr     %w[rc], %w[uNew], %[pMem]\n\t" \
                         "cbnz      %w[rc], .Ltry_again_" #name "_%=\n\t" \
                         : [pMem] "+Q"  (*a_pu32Mem) \
                         , [uNew] "=&r" (u32NewRet) \
                         , [rc]   "=&r" (rcSpill) \
                         : in_reg \
                         : "cc")
#  define RTASM_ARM_LOAD_MODIFY_STORE_RET_OLD_32(name, a_pu32Mem, barrier_type, modify64, modify32, in_reg) \
    uint32_t rcSpill; \
    uint32_t u32OldRet; \
    uint32_t u32NewSpill; \
    __asm__ __volatile__(".Ltry_again_" #name "_%=:\n\t" \
                         RTASM_ARM_##barrier_type /* before lable? */ \
                         "ldaxr     %w[uOld], %[pMem]\n\t" \
                         modify64 \
                         "stlxr     %w[rc], %w[uNew], %[pMem]\n\t" \
                         "cbnz      %w[rc], .Ltry_again_" #name "_%=\n\t" \
                         : [pMem] "+Q"  (*a_pu32Mem) \
                         , [uOld] "=&r" (u32OldRet) \
                         , [uNew] "=&r" (u32NewSpill) \
                         , [rc]   "=&r" (rcSpill) \
                         : in_reg \
                         : "cc")
#  define RTASM_ARM_LOAD_MODIFY_STORE_RET_NEW_64(name, a_pu64Mem, barrier_type, modify64, modify32, in_reg) \
    uint32_t rcSpill; \
    uint64_t u64NewRet; \
    __asm__ __volatile__(".Ltry_again_" #name "_%=:\n\t" \
                         RTASM_ARM_##barrier_type /* before lable? */ \
                         "ldaxr     %[uNew], %[pMem]\n\t" \
                         modify64 \
                         "stlxr     %w[rc], %[uNew], %[pMem]\n\t" \
                         "cbnz      %w[rc], .Ltry_again_" #name "_%=\n\t" \
                         : [pMem] "+Q"  (*a_pu64Mem) \
                         , [uNew] "=&r" (u64NewRet) \
                         , [rc]   "=&r" (rcSpill) \
                         : in_reg \
                         : "cc")
#  define RTASM_ARM_LOAD_MODIFY_STORE_RET_OLD_64(name, a_pu64Mem, barrier_type, modify64, modify32, in_reg) \
    uint32_t rcSpill; \
    uint64_t u64OldRet; \
    uint64_t u64NewSpill; \
    __asm__ __volatile__(".Ltry_again_" #name "_%=:\n\t" \
                         RTASM_ARM_##barrier_type /* before lable? */ \
                         "ldaxr     %[uOld], %[pMem]\n\t" \
                         modify64 \
                         "stlxr     %w[rc], %[uNew], %[pMem]\n\t" \
                         "cbnz      %w[rc], .Ltry_again_" #name "_%=\n\t" \
                         : [pMem] "+Q"  (*a_pu64Mem) \
                         , [uOld] "=&r" (u64OldRet) \
                         , [uNew] "=&r" (u64NewSpill) \
                         , [rc]   "=&r" (rcSpill) \
                         : in_reg \
                         : "cc")

# else /* RT_ARCH_ARM32 */
#  define RTASM_ARM_PICK_6432(expr64, expr32) expr32
#  if RT_ARCH_ARM32 >= 7
#   warning armv7
#   define RTASM_ARM_NO_BARRIER_IN_REG
#   define RTASM_ARM_NO_BARRIER_COMMA_IN_REG
#   define RTASM_ARM_DSB_SY             "dsb    sy\n\t"
#   define RTASM_ARM_DSB_SY_IN_REG      "X" (0xfade)
#   define RTASM_ARM_DMB_SY             "dmb    sy\n\t"
#   define RTASM_ARM_DMB_SY_IN_REG      "X" (0xfade)
#   define RTASM_ARM_DMB_ST             "dmb    st\n\t"
#   define RTASM_ARM_DMB_ST_IN_REG      "X" (0xfade)
#   define RTASM_ARM_DMB_LD             "dmb    ld\n\t"
#   define RTASM_ARM_DMB_LD_IN_REG      "X" (0xfade)

#  elif RT_ARCH_ARM32 >= 6
#   warning armv6
#   define RTASM_ARM_DSB_SY             "mcr p15, 0, %[uZero], c7, c10, 4\n\t"
#   define RTASM_ARM_DSB_SY_IN_REG      [uZero] "r" (0)
#   define RTASM_ARM_DMB_SY             "mcr p15, 0, %[uZero], c7, c10, 5\n\t"
#   define RTASM_ARM_DMB_SY_IN_REG      [uZero] "r" (0)
#   define RTASM_ARM_DMB_ST             RTASM_ARM_DMB_SY
#   define RTASM_ARM_DMB_ST_IN_REG      RTASM_ARM_DMB_SY_IN_REG
#   define RTASM_ARM_DMB_LD             RTASM_ARM_DMB_SY
#   define RTASM_ARM_DMB_LD_IN_REG      RTASM_ARM_DMB_SY_IN_REG
#  elif RT_ARCH_ARM32 >= 4
#   warning armv5 or older
#   define RTASM_ARM_DSB_SY             "mcr p15, 0, %[uZero], c7, c10, 4\n\t"
#   define RTASM_ARM_DSB_SY_IN_REG      [uZero] "r" (0)
#   define RTASM_ARM_DMB_SY             RTASM_ARM_DSB_SY
#   define RTASM_ARM_DMB_SY_IN_REG      RTASM_ARM_DSB_SY_IN_REG
#   define RTASM_ARM_DMB_ST             RTASM_ARM_DSB_SY
#   define RTASM_ARM_DMB_ST_IN_REG      RTASM_ARM_DSB_SY_IN_REG
#   define RTASM_ARM_DMB_LD             RTASM_ARM_DSB_SY
#   define RTASM_ARM_DMB_LD_IN_REG      RTASM_ARM_DSB_SY_IN_REG
#  else
#   error "huh? Odd RT_ARCH_ARM32 value!"
#  endif
#  define RTASM_ARM_DSB_SY_COMMA_IN_REG , RTASM_ARM_DSB_SY_IN_REG
#  define RTASM_ARM_DMB_SY_COMMA_IN_REG , RTASM_ARM_DMB_SY_IN_REG
#  define RTASM_ARM_DMB_ST_COMMA_IN_REG , RTASM_ARM_DMB_ST_IN_REG
#  define RTASM_ARM_DMB_LD_COMMA_IN_REG , RTASM_ARM_DMB_LD_IN_REG
#  define RTASM_ARM_LOAD_MODIFY_STORE_RET_NEW_32(name, a_pu32Mem, barrier_type, modify64, modify32, in_reg) \
    uint32_t rcSpill; \
    uint32_t u32NewRet; \
    __asm__ __volatile__(".Ltry_again_" #name "_%=:\n\t" \
                         RT_CONCAT(RTASM_ARM_,barrier_type) /* before lable? */ \
                         "ldrex     %[uNew], %[pMem]\n\t" \
                         modify32 \
                         "strex     %[rc], %[uNew], %[pMem]\n\t" \
                         "cmp       %[rc], #0\n\t" \
                         "bne       .Ltry_again_" #name "_%=\n\t" \
                         : [pMem] "+m"  (*a_pu32Mem) \
                         , [uNew] "=&r" (u32NewRet) \
                         , [rc]   "=&r" (rcSpill) \
                         : RT_CONCAT3(RTASM_ARM_,barrier_type,_IN_REG) \
                         , in_reg \
                         : "cc")
#  define RTASM_ARM_LOAD_MODIFY_STORE_RET_OLD_32(name, a_pu32Mem, barrier_type, modify64, modify32, in_reg) \
    uint32_t rcSpill; \
    uint32_t u32OldRet; \
    uint32_t u32NewSpill; \
    __asm__ __volatile__(".Ltry_again_" #name "_%=:\n\t" \
                         RT_CONCAT(RTASM_ARM_,barrier_type) /* before lable? */ \
                         "ldrex     %[uOld], %[pMem]\n\t" \
                         modify32 \
                         "strex     %[rc], %[uNew], %[pMem]\n\t" \
                         "cmp       %[rc], #0\n\t" \
                         "bne       .Ltry_again_" #name "_%=\n\t" \
                         : [pMem] "+m"  (*a_pu32Mem) \
                         , [uOld] "=&r" (u32OldRet) \
                         , [uNew] "=&r" (u32NewSpill) \
                         , [rc]   "=&r" (rcSpill) \
                         : RT_CONCAT3(RTASM_ARM_,barrier_type,_IN_REG) \
                         , in_reg \
                         : "cc")
#  define RTASM_ARM_LOAD_MODIFY_STORE_RET_NEW_64(name, a_pu64Mem, barrier_type, modify64, modify32, in_reg) \
    uint32_t rcSpill; \
    uint64_t u64NewRet; \
    __asm__ __volatile__(".Ltry_again_" #name "_%=:\n\t" \
                         RT_CONCAT(RTASM_ARM_,barrier_type) /* before lable? */ \
                         "ldrexd    %[uNew], %H[uNew], %[pMem]\n\t" \
                         modify32 \
                         "strexd    %[rc], %[uNew], %H[uNew], %[pMem]\n\t" \
                         "cmp       %[rc], #0\n\t" \
                         "bne       .Ltry_again_" #name "_%=\n\t" \
                         : [pMem] "+m"  (*a_pu64Mem), \
                           [uNew] "=&r" (u64NewRet), \
                           [rc]   "=&r" (rcSpill) \
                         : RT_CONCAT3(RTASM_ARM_,barrier_type,_IN_REG) \
                         , in_reg \
                         : "cc")
#  define RTASM_ARM_LOAD_MODIFY_STORE_RET_OLD_64(name, a_pu64Mem, barrier_type, modify64, modify32, in_reg) \
    uint32_t rcSpill; \
    uint64_t u64OldRet; \
    uint64_t u64NewSpill; \
    __asm__ __volatile__(".Ltry_again_" #name "_%=:\n\t" \
                         RT_CONCAT(RTASM_ARM_,barrier_type) /* before lable? */ \
                         "ldrexd    %[uOld], %H[uOld], %[pMem]\n\t" \
                         modify32 \
                         "strexd    %[rc], %[uNew], %H[uNew], %[pMem]\n\t" \
                         "cmp       %[rc], #0\n\t" \
                         "bne       .Ltry_again_" #name "_%=\n\t" \
                         : [pMem] "+m"  (*a_pu64Mem), \
                           [uOld] "=&r" (u64OldRet), \
                           [uNew] "=&r" (u64NewSpill), \
                           [rc]   "=&r" (rcSpill) \
                         : RT_CONCAT3(RTASM_ARM_,barrier_type,_IN_REG) \
                         , in_reg \
                         : "cc")
# endif /* RT_ARCH_ARM32 */
#endif


/** @def ASMReturnAddress
 * Gets the return address of the current (or calling if you like) function or method.
 */
#ifdef _MSC_VER
# ifdef __cplusplus
extern "C"
# endif
void * _ReturnAddress(void);
# pragma intrinsic(_ReturnAddress)
# define ASMReturnAddress() _ReturnAddress()
#elif defined(__GNUC__) || defined(DOXYGEN_RUNNING)
# define ASMReturnAddress() __builtin_return_address(0)
#elif defined(__WATCOMC__)
# define ASMReturnAddress() Watcom_does_not_appear_to_have_intrinsic_return_address_function()
#else
# error "Unsupported compiler."
#endif


/**
 * Compiler memory barrier.
 *
 * Ensure that the compiler does not use any cached (register/tmp stack) memory
 * values or any outstanding writes when returning from this function.
 *
 * This function must be used if non-volatile data is modified by a
 * device or the VMM. Typical cases are port access, MMIO access,
 * trapping instruction, etc.
 */
#if RT_INLINE_ASM_GNU_STYLE
# define ASMCompilerBarrier()   do { __asm__ __volatile__("" : : : "memory"); } while (0)
#elif RT_INLINE_ASM_USES_INTRIN
# define ASMCompilerBarrier()   do { _ReadWriteBarrier(); } while (0)
#elif defined(__WATCOMC__)
void ASMCompilerBarrier(void);
#else /* 2003 should have _ReadWriteBarrier() but I guess we're at 2002 level then... */
DECLINLINE(void) ASMCompilerBarrier(void) RT_NOTHROW_DEF
{
    __asm
    {
    }
}
#endif


/** @def ASMBreakpoint
 * Debugger Breakpoint.
 * @deprecated Use RT_BREAKPOINT instead.
 * @internal
 */
#define ASMBreakpoint() RT_BREAKPOINT()


/**
 * Spinloop hint for platforms that have these, empty function on the other
 * platforms.
 *
 * x86 & AMD64: The PAUSE variant of NOP for helping hyperthreaded CPUs detecting
 * spin locks.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && (defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86))
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMNopPause(void) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMNopPause(void) RT_NOTHROW_DEF
{
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__(".byte 0xf3,0x90\n\t");
#  else
    __asm {
        _emit 0f3h
        _emit 090h
    }
#  endif

# elif defined(RT_ARCH_ARM32) || defined(RT_ARCH_ARM64)
    __asm__ __volatile__("yield\n\t"); /* ARMv6K+ */

# else
    /* dummy */
# endif
}
#endif


/**
 * Atomically Exchange an unsigned 8-bit value, ordered.
 *
 * @returns Current *pu8 value
 * @param   pu8    Pointer to the 8-bit variable to update.
 * @param   u8     The 8-bit value to assign to *pu8.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM
RT_ASM_DECL_PRAGMA_WATCOM(uint8_t) ASMAtomicXchgU8(volatile uint8_t RT_FAR *pu8, uint8_t u8) RT_NOTHROW_PROTO;
#else
DECLINLINE(uint8_t) ASMAtomicXchgU8(volatile uint8_t RT_FAR *pu8, uint8_t u8) RT_NOTHROW_DEF
{
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("xchgb %0, %1\n\t"
                         : "=m" (*pu8)
                         , "=q" (u8) /* =r - busted on g++ (GCC) 3.4.4 20050721 (Red Hat 3.4.4-2) */
                         : "1" (u8)
                         , "m" (*pu8));
#  else
    __asm
    {
#   ifdef RT_ARCH_AMD64
        mov     rdx, [pu8]
        mov     al, [u8]
        xchg    [rdx], al
        mov     [u8], al
#   else
        mov     edx, [pu8]
        mov     al, [u8]
        xchg    [edx], al
        mov     [u8], al
#   endif
    }
#  endif
    return u8;

# elif defined(RT_ARCH_ARM32) || defined(RT_ARCH_ARM64)
    uint32_t uOld;
    uint32_t rcSpill;
    __asm__ __volatile__(".Ltry_again_ASMAtomicXchgU8_%=:\n\t"
                         RTASM_ARM_DMB_SY
#  if defined(RT_ARCH_ARM64)
                         "ldaxrb    %w[uOld], %[pMem]\n\t"
                         "stlxrb    %w[rc], %w[uNew], %[pMem]\n\t"
                         "cbnz      %w[rc], .Ltry_again_ASMAtomicXchgU8_%=\n\t"
#  else
                         "ldrexb    %[uOld], %[pMem]\n\t"      /* ARMv6+ */
                         "strexb    %[rc], %[uNew], %[pMem]\n\t"
                         "cmp       %[rc], #0\n\t"
                         "bne       .Ltry_again_ASMAtomicXchgU8_%=\n\t"
#  endif
                         : [pMem] "+Q" (*pu8)
                         , [uOld] "=&r" (uOld)
                         , [rc]   "=&r" (rcSpill)
                         : [uNew] "r" ((uint32_t)u8)
                           RTASM_ARM_DMB_SY_COMMA_IN_REG
                         : "cc");
    return (uint8_t)uOld;

# else
#  error "Port me"
# endif
}
#endif


/**
 * Atomically Exchange a signed 8-bit value, ordered.
 *
 * @returns Current *pu8 value
 * @param   pi8     Pointer to the 8-bit variable to update.
 * @param   i8      The 8-bit value to assign to *pi8.
 */
DECLINLINE(int8_t) ASMAtomicXchgS8(volatile int8_t RT_FAR *pi8, int8_t i8) RT_NOTHROW_DEF
{
    return (int8_t)ASMAtomicXchgU8((volatile uint8_t RT_FAR *)pi8, (uint8_t)i8);
}


/**
 * Atomically Exchange a bool value, ordered.
 *
 * @returns Current *pf value
 * @param   pf      Pointer to the 8-bit variable to update.
 * @param   f       The 8-bit value to assign to *pi8.
 */
DECLINLINE(bool) ASMAtomicXchgBool(volatile bool RT_FAR *pf, bool f) RT_NOTHROW_DEF
{
#ifdef _MSC_VER
    return !!ASMAtomicXchgU8((volatile uint8_t RT_FAR *)pf, (uint8_t)f);
#else
    return (bool)ASMAtomicXchgU8((volatile uint8_t RT_FAR *)pf, (uint8_t)f);
#endif
}


/**
 * Atomically Exchange an unsigned 16-bit value, ordered.
 *
 * @returns Current *pu16 value
 * @param   pu16    Pointer to the 16-bit variable to update.
 * @param   u16     The 16-bit value to assign to *pu16.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM
RT_ASM_DECL_PRAGMA_WATCOM(uint16_t) ASMAtomicXchgU16(volatile uint16_t RT_FAR *pu16, uint16_t u16) RT_NOTHROW_PROTO;
#else
DECLINLINE(uint16_t) ASMAtomicXchgU16(volatile uint16_t RT_FAR *pu16, uint16_t u16) RT_NOTHROW_DEF
{
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("xchgw %0, %1\n\t"
                         : "=m" (*pu16)
                         , "=r" (u16)
                         : "1" (u16)
                         , "m" (*pu16));
#  else
    __asm
    {
#   ifdef RT_ARCH_AMD64
        mov     rdx, [pu16]
        mov     ax, [u16]
        xchg    [rdx], ax
        mov     [u16], ax
#   else
        mov     edx, [pu16]
        mov     ax, [u16]
        xchg    [edx], ax
        mov     [u16], ax
#   endif
    }
#  endif
    return u16;

# elif defined(RT_ARCH_ARM32) || defined(RT_ARCH_ARM64)
    uint32_t uOld;
    uint32_t rcSpill;
    __asm__ __volatile__(".Ltry_again_ASMAtomicXchgU16_%=:\n\t"
                         RTASM_ARM_DMB_SY
#  if defined(RT_ARCH_ARM64)
                         "ldaxrh    %w[uOld], %[pMem]\n\t"
                         "stlxrh    %w[rc], %w[uNew], %[pMem]\n\t"
                         "cbnz      %w[rc], .Ltry_again_ASMAtomicXchgU16_%=\n\t"
#  else
                         "ldrexh    %[uOld], %[pMem]\n\t"      /* ARMv6+ */
                         "strexh    %[rc], %[uNew], %[pMem]\n\t"
                         "cmp       %[rc], #0\n\t"
                         "bne       .Ltry_again_ASMAtomicXchgU16_%=\n\t"
#  endif
                         : [pMem] "+Q" (*pu16)
                         , [uOld] "=&r" (uOld)
                         , [rc]   "=&r" (rcSpill)
                         : [uNew] "r" ((uint32_t)u16)
                           RTASM_ARM_DMB_SY_COMMA_IN_REG
                         : "cc");
    return (uint16_t)uOld;

# else
#  error "Port me"
# endif
}
#endif


/**
 * Atomically Exchange a signed 16-bit value, ordered.
 *
 * @returns Current *pu16 value
 * @param   pi16    Pointer to the 16-bit variable to update.
 * @param   i16     The 16-bit value to assign to *pi16.
 */
DECLINLINE(int16_t) ASMAtomicXchgS16(volatile int16_t RT_FAR *pi16, int16_t i16) RT_NOTHROW_DEF
{
    return (int16_t)ASMAtomicXchgU16((volatile uint16_t RT_FAR *)pi16, (uint16_t)i16);
}


/**
 * Atomically Exchange an unsigned 32-bit value, ordered.
 *
 * @returns Current *pu32 value
 * @param   pu32    Pointer to the 32-bit variable to update.
 * @param   u32     The 32-bit value to assign to *pu32.
 *
 * @remarks Does not work on 286 and earlier.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(uint32_t) ASMAtomicXchgU32(volatile uint32_t RT_FAR *pu32, uint32_t u32) RT_NOTHROW_PROTO;
#else
DECLINLINE(uint32_t) ASMAtomicXchgU32(volatile uint32_t RT_FAR *pu32, uint32_t u32) RT_NOTHROW_DEF
{
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("xchgl %0, %1\n\t"
                         : "=m" (*pu32) /** @todo r=bird: +m rather than =m here? */
                         , "=r" (u32)
                         : "1" (u32)
                         , "m" (*pu32));

#  elif RT_INLINE_ASM_USES_INTRIN
   u32 = _InterlockedExchange((long RT_FAR *)pu32, u32);

#  else
    __asm
    {
#   ifdef RT_ARCH_AMD64
        mov     rdx, [pu32]
        mov     eax, u32
        xchg    [rdx], eax
        mov     [u32], eax
#   else
        mov     edx, [pu32]
        mov     eax, u32
        xchg    [edx], eax
        mov     [u32], eax
#   endif
    }
#  endif
    return u32;

# elif defined(RT_ARCH_ARM32) || defined(RT_ARCH_ARM64)
    uint32_t uOld;
    uint32_t rcSpill;
    __asm__ __volatile__(".Ltry_again_ASMAtomicXchgU32_%=:\n\t"
                         RTASM_ARM_DMB_SY
#  if defined(RT_ARCH_ARM64)
                         "ldaxr     %w[uOld], %[pMem]\n\t"
                         "stlxr     %w[rc], %w[uNew], %[pMem]\n\t"
                         "cbnz      %w[rc], .Ltry_again_ASMAtomicXchgU32_%=\n\t"
#  else
                         "ldrex     %[uOld], %[pMem]\n\t"      /* ARMv6+ */
                         "strex     %[rc], %[uNew], %[pMem]\n\t"
                         "cmp       %[rc], #0\n\t"
                         "bne       .Ltry_again_ASMAtomicXchgU32_%=\n\t"
#  endif
                         : [pMem] "+Q"  (*pu32)
                         , [uOld] "=&r" (uOld)
                         , [rc]   "=&r" (rcSpill)
                         : [uNew] "r"   (u32)
                           RTASM_ARM_DMB_SY_COMMA_IN_REG
                         : "cc");
    return uOld;

# else
#  error "Port me"
# endif
}
#endif


/**
 * Atomically Exchange a signed 32-bit value, ordered.
 *
 * @returns Current *pu32 value
 * @param   pi32    Pointer to the 32-bit variable to update.
 * @param   i32     The 32-bit value to assign to *pi32.
 */
DECLINLINE(int32_t) ASMAtomicXchgS32(volatile int32_t RT_FAR *pi32, int32_t i32) RT_NOTHROW_DEF
{
    return (int32_t)ASMAtomicXchgU32((volatile uint32_t RT_FAR *)pi32, (uint32_t)i32);
}


/**
 * Atomically Exchange an unsigned 64-bit value, ordered.
 *
 * @returns Current *pu64 value
 * @param   pu64    Pointer to the 64-bit variable to update.
 * @param   u64     The 64-bit value to assign to *pu64.
 *
 * @remarks Works on 32-bit x86 CPUs starting with Pentium.
 */
#if (RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN) \
 || RT_INLINE_DONT_MIX_CMPXCHG8B_AND_PIC
RT_ASM_DECL_PRAGMA_WATCOM(uint64_t) ASMAtomicXchgU64(volatile uint64_t RT_FAR *pu64, uint64_t u64) RT_NOTHROW_PROTO;
#else
DECLINLINE(uint64_t) ASMAtomicXchgU64(volatile uint64_t RT_FAR *pu64, uint64_t u64) RT_NOTHROW_DEF
{
# if defined(RT_ARCH_AMD64)
#  if RT_INLINE_ASM_USES_INTRIN
   return _InterlockedExchange64((__int64 *)pu64, u64);

#  elif RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("xchgq %0, %1\n\t"
                         : "=m" (*pu64)
                         , "=r" (u64)
                         : "1" (u64)
                         , "m" (*pu64));
    return u64;
#  else
    __asm
    {
        mov     rdx, [pu64]
        mov     rax, [u64]
        xchg    [rdx], rax
        mov     [u64], rax
    }
    return u64;
#  endif

# elif defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
#   if defined(PIC) || defined(__PIC__)
    uint32_t u32EBX = (uint32_t)u64;
    __asm__ __volatile__(/*"xchgl %%esi, %5\n\t"*/
                         "xchgl %%ebx, %3\n\t"
                         "1:\n\t"
                         "lock; cmpxchg8b (%5)\n\t"
                         "jnz 1b\n\t"
                         "movl %3, %%ebx\n\t"
                         /*"xchgl %%esi, %5\n\t"*/
                         : "=A" (u64)
                         , "=m" (*pu64)
                         : "0" (*pu64)
                         , "m" ( u32EBX )
                         , "c" ( (uint32_t)(u64 >> 32) )
                         , "S" (pu64)
                         : "cc");
#   else /* !PIC */
    __asm__ __volatile__("1:\n\t"
                         "lock; cmpxchg8b %1\n\t"
                         "jnz 1b\n\t"
                         : "=A" (u64)
                         , "=m" (*pu64)
                         : "0" (*pu64)
                         , "b" ( (uint32_t)u64 )
                         , "c" ( (uint32_t)(u64 >> 32) )
                         : "cc");
#   endif
#  else
    __asm
    {
        mov     ebx, dword ptr [u64]
        mov     ecx, dword ptr [u64 + 4]
        mov     edi, pu64
        mov     eax, dword ptr [edi]
        mov     edx, dword ptr [edi + 4]
    retry:
        lock cmpxchg8b [edi]
        jnz retry
        mov     dword ptr [u64], eax
        mov     dword ptr [u64 + 4], edx
    }
#  endif
    return u64;

# elif defined(RT_ARCH_ARM32) || defined(RT_ARCH_ARM64)
    uint32_t rcSpill;
    uint64_t uOld;
    __asm__ __volatile__(".Ltry_again_ASMAtomicXchgU64_%=:\n\t"
                         RTASM_ARM_DMB_SY
#  if defined(RT_ARCH_ARM64)
                         "ldaxr     %[uOld], %[pMem]\n\t"
                         "stlxr     %w[rc], %[uNew], %[pMem]\n\t"
                         "cbnz      %w[rc], .Ltry_again_ASMAtomicXchgU64_%=\n\t"
#  else
                         "ldrexd    %[uOld], %H[uOld], %[pMem]\n\t"      /* ARMv6+ */
                         "strexd    %[rc], %[uNew], %H[uNew], %[pMem]\n\t"
                         "cmp       %[rc], #0\n\t"
                         "bne       .Ltry_again_ASMAtomicXchgU64_%=\n\t"
#  endif
                         : [pMem] "+Q"  (*pu64)
                         , [uOld] "=&r" (uOld)
                         , [rc]   "=&r" (rcSpill)
                         : [uNew] "r"   (u64)
                           RTASM_ARM_DMB_SY_COMMA_IN_REG
                         : "cc");
    return uOld;

# else
#  error "Port me"
# endif
}
#endif


/**
 * Atomically Exchange an signed 64-bit value, ordered.
 *
 * @returns Current *pi64 value
 * @param   pi64    Pointer to the 64-bit variable to update.
 * @param   i64     The 64-bit value to assign to *pi64.
 */
DECLINLINE(int64_t) ASMAtomicXchgS64(volatile int64_t RT_FAR *pi64, int64_t i64) RT_NOTHROW_DEF
{
    return (int64_t)ASMAtomicXchgU64((volatile uint64_t RT_FAR *)pi64, (uint64_t)i64);
}


/**
 * Atomically Exchange a size_t value, ordered.
 *
 * @returns Current *ppv value
 * @param   puDst   Pointer to the size_t variable to update.
 * @param   uNew    The new value to assign to *puDst.
 */
DECLINLINE(size_t) ASMAtomicXchgZ(size_t volatile RT_FAR *puDst, const size_t uNew) RT_NOTHROW_DEF
{
#if ARCH_BITS == 16
    AssertCompile(sizeof(size_t) == 2);
    return ASMAtomicXchgU16((volatile uint16_t RT_FAR *)puDst, uNew);
#elif ARCH_BITS == 32
    return ASMAtomicXchgU32((volatile uint32_t RT_FAR *)puDst, uNew);
#elif ARCH_BITS == 64
    return ASMAtomicXchgU64((volatile uint64_t RT_FAR *)puDst, uNew);
#else
# error "ARCH_BITS is bogus"
#endif
}


/**
 * Atomically Exchange a pointer value, ordered.
 *
 * @returns Current *ppv value
 * @param   ppv    Pointer to the pointer variable to update.
 * @param   pv     The pointer value to assign to *ppv.
 */
DECLINLINE(void RT_FAR *) ASMAtomicXchgPtr(void RT_FAR * volatile RT_FAR *ppv, const void RT_FAR *pv) RT_NOTHROW_DEF
{
#if ARCH_BITS == 32 || ARCH_BITS == 16
    return (void RT_FAR *)ASMAtomicXchgU32((volatile uint32_t RT_FAR *)(void RT_FAR *)ppv, (uint32_t)pv);
#elif ARCH_BITS == 64
    return (void RT_FAR *)ASMAtomicXchgU64((volatile uint64_t RT_FAR *)(void RT_FAR *)ppv, (uint64_t)pv);
#else
# error "ARCH_BITS is bogus"
#endif
}


/**
 * Convenience macro for avoiding the annoying casting with ASMAtomicXchgPtr.
 *
 * @returns Current *pv value
 * @param   ppv     Pointer to the pointer variable to update.
 * @param   pv      The pointer value to assign to *ppv.
 * @param   Type    The type of *ppv, sans volatile.
 */
#ifdef __GNUC__ /* 8.2.0 requires -Wno-ignored-qualifiers */
# define ASMAtomicXchgPtrT(ppv, pv, Type) \
    __extension__ \
    ({\
        __typeof__(*(ppv)) volatile * const ppvTypeChecked = (ppv); \
        Type                          const pvTypeChecked  = (pv); \
        Type pvTypeCheckedRet = (__typeof__(*(ppv))) ASMAtomicXchgPtr((void * volatile *)ppvTypeChecked, (void *)pvTypeChecked); \
        pvTypeCheckedRet; \
     })
#else
# define ASMAtomicXchgPtrT(ppv, pv, Type) \
    (Type)ASMAtomicXchgPtr((void RT_FAR * volatile RT_FAR *)(ppv), (void RT_FAR *)(pv))
#endif


/**
 * Atomically Exchange a raw-mode context pointer value, ordered.
 *
 * @returns Current *ppv value
 * @param   ppvRC   Pointer to the pointer variable to update.
 * @param   pvRC    The pointer value to assign to *ppv.
 */
DECLINLINE(RTRCPTR) ASMAtomicXchgRCPtr(RTRCPTR volatile RT_FAR *ppvRC, RTRCPTR pvRC) RT_NOTHROW_DEF
{
    return (RTRCPTR)ASMAtomicXchgU32((uint32_t volatile RT_FAR *)(void RT_FAR *)ppvRC, (uint32_t)pvRC);
}


/**
 * Atomically Exchange a ring-0 pointer value, ordered.
 *
 * @returns Current *ppv value
 * @param   ppvR0  Pointer to the pointer variable to update.
 * @param   pvR0   The pointer value to assign to *ppv.
 */
DECLINLINE(RTR0PTR) ASMAtomicXchgR0Ptr(RTR0PTR volatile RT_FAR *ppvR0, RTR0PTR pvR0) RT_NOTHROW_DEF
{
#if R0_ARCH_BITS == 32 || ARCH_BITS == 16
    return (RTR0PTR)ASMAtomicXchgU32((volatile uint32_t RT_FAR *)(void RT_FAR *)ppvR0, (uint32_t)pvR0);
#elif R0_ARCH_BITS == 64
    return (RTR0PTR)ASMAtomicXchgU64((volatile uint64_t RT_FAR *)(void RT_FAR *)ppvR0, (uint64_t)pvR0);
#else
# error "R0_ARCH_BITS is bogus"
#endif
}


/**
 * Atomically Exchange a ring-3 pointer value, ordered.
 *
 * @returns Current *ppv value
 * @param   ppvR3  Pointer to the pointer variable to update.
 * @param   pvR3   The pointer value to assign to *ppv.
 */
DECLINLINE(RTR3PTR) ASMAtomicXchgR3Ptr(RTR3PTR volatile RT_FAR *ppvR3, RTR3PTR pvR3) RT_NOTHROW_DEF
{
#if R3_ARCH_BITS == 32 || ARCH_BITS == 16
    return (RTR3PTR)ASMAtomicXchgU32((volatile uint32_t RT_FAR *)(void RT_FAR *)ppvR3, (uint32_t)pvR3);
#elif R3_ARCH_BITS == 64
    return (RTR3PTR)ASMAtomicXchgU64((volatile uint64_t RT_FAR *)(void RT_FAR *)ppvR3, (uint64_t)pvR3);
#else
# error "R3_ARCH_BITS is bogus"
#endif
}


/** @def ASMAtomicXchgHandle
 * Atomically Exchange a typical IPRT handle value, ordered.
 *
 * @param   ph          Pointer to the value to update.
 * @param   hNew        The new value to assigned to *pu.
 * @param   phRes       Where to store the current *ph value.
 *
 * @remarks This doesn't currently work for all handles (like RTFILE).
 */
#if HC_ARCH_BITS == 32 || ARCH_BITS == 16
# define ASMAtomicXchgHandle(ph, hNew, phRes) \
   do { \
       AssertCompile(sizeof(*(ph))    == sizeof(uint32_t)); \
       AssertCompile(sizeof(*(phRes)) == sizeof(uint32_t)); \
       *(uint32_t RT_FAR *)(phRes) = ASMAtomicXchgU32((uint32_t volatile RT_FAR *)(ph), (const uint32_t)(hNew)); \
   } while (0)
#elif HC_ARCH_BITS == 64
# define ASMAtomicXchgHandle(ph, hNew, phRes) \
   do { \
       AssertCompile(sizeof(*(ph))    == sizeof(uint64_t)); \
       AssertCompile(sizeof(*(phRes)) == sizeof(uint64_t)); \
       *(uint64_t RT_FAR *)(phRes) = ASMAtomicXchgU64((uint64_t volatile RT_FAR *)(ph), (const uint64_t)(hNew)); \
   } while (0)
#else
# error HC_ARCH_BITS
#endif


/**
 * Atomically Exchange a value which size might differ
 * between platforms or compilers, ordered.
 *
 * @param   pu      Pointer to the variable to update.
 * @param   uNew    The value to assign to *pu.
 * @todo This is busted as its missing the result argument.
 */
#define ASMAtomicXchgSize(pu, uNew) \
    do { \
        switch (sizeof(*(pu))) { \
            case 1: ASMAtomicXchgU8( (volatile uint8_t  RT_FAR *)(void RT_FAR *)(pu), (uint8_t)(uNew)); break; \
            case 2: ASMAtomicXchgU16((volatile uint16_t RT_FAR *)(void RT_FAR *)(pu), (uint16_t)(uNew)); break; \
            case 4: ASMAtomicXchgU32((volatile uint32_t RT_FAR *)(void RT_FAR *)(pu), (uint32_t)(uNew)); break; \
            case 8: ASMAtomicXchgU64((volatile uint64_t RT_FAR *)(void RT_FAR *)(pu), (uint64_t)(uNew)); break; \
            default: AssertMsgFailed(("ASMAtomicXchgSize: size %d is not supported\n", sizeof(*(pu)))); \
        } \
    } while (0)

/**
 * Atomically Exchange a value which size might differ
 * between platforms or compilers, ordered.
 *
 * @param   pu      Pointer to the variable to update.
 * @param   uNew    The value to assign to *pu.
 * @param   puRes   Where to store the current *pu value.
 */
#define ASMAtomicXchgSizeCorrect(pu, uNew, puRes) \
    do { \
        switch (sizeof(*(pu))) { \
            case 1: *(uint8_t  RT_FAR *)(puRes) = ASMAtomicXchgU8( (volatile uint8_t  RT_FAR *)(void RT_FAR *)(pu), (uint8_t)(uNew)); break; \
            case 2: *(uint16_t RT_FAR *)(puRes) = ASMAtomicXchgU16((volatile uint16_t RT_FAR *)(void RT_FAR *)(pu), (uint16_t)(uNew)); break; \
            case 4: *(uint32_t RT_FAR *)(puRes) = ASMAtomicXchgU32((volatile uint32_t RT_FAR *)(void RT_FAR *)(pu), (uint32_t)(uNew)); break; \
            case 8: *(uint64_t RT_FAR *)(puRes) = ASMAtomicXchgU64((volatile uint64_t RT_FAR *)(void RT_FAR *)(pu), (uint64_t)(uNew)); break; \
            default: AssertMsgFailed(("ASMAtomicXchgSize: size %d is not supported\n", sizeof(*(pu)))); \
        } \
    } while (0)



/**
 * Atomically Compare and Exchange an unsigned 8-bit value, ordered.
 *
 * @returns true if xchg was done.
 * @returns false if xchg wasn't done.
 *
 * @param   pu8         Pointer to the value to update.
 * @param   u8New       The new value to assigned to *pu8.
 * @param   u8Old       The old value to *pu8 compare with.
 *
 * @remarks x86: Requires a 486 or later.
 * @todo Rename ASMAtomicCmpWriteU8
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM || !RT_INLINE_ASM_GNU_STYLE
RT_ASM_DECL_PRAGMA_WATCOM(bool) ASMAtomicCmpXchgU8(volatile uint8_t RT_FAR *pu8, const uint8_t u8New, const uint8_t u8Old) RT_NOTHROW_PROTO;
#else
DECLINLINE(bool) ASMAtomicCmpXchgU8(volatile uint8_t RT_FAR *pu8, const uint8_t u8New, uint8_t u8Old) RT_NOTHROW_DEF
{
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
    uint8_t u8Ret;
    __asm__ __volatile__("lock; cmpxchgb %3, %0\n\t"
                         "setz  %1\n\t"
                         : "=m" (*pu8)
                         , "=qm" (u8Ret)
                         , "=a" (u8Old)
                         : "q" (u8New)
                         , "2" (u8Old)
                         , "m" (*pu8)
                         : "cc");
    return (bool)u8Ret;

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    union { uint32_t u; bool f; } fXchg;
    uint32_t u32Spill;
    uint32_t rcSpill;
    __asm__ __volatile__(".Ltry_again_ASMAtomicCmpXchgU8_%=:\n\t"
                         RTASM_ARM_DMB_SY
#  if defined(RT_ARCH_ARM64)
                         "ldaxrb    %w[uOld], %[pMem]\n\t"
                         "cmp       %w[uOld], %w[uCmp]\n\t"
                         "bne       1f\n\t"   /* stop here if not equal */
                         "stlxrb    %w[rc], %w[uNew], %[pMem]\n\t"
                         "cbnz      %w[rc], .Ltry_again_ASMAtomicCmpXchgU8_%=\n\t"
                         "mov       %w[fXchg], #1\n\t"
#  else
                         "ldrexb    %[uOld], %[pMem]\n\t"
                         "teq       %[uOld], %[uCmp]\n\t"
                         "strexbeq  %[rc], %[uNew], %[pMem]\n\t"
                         "bne       1f\n\t"   /* stop here if not equal */
                         "cmp       %[rc], #0\n\t"
                         "bne       .Ltry_again_ASMAtomicCmpXchgU8_%=\n\t"
                         "mov       %[fXchg], #1\n\t"
#  endif
                         "1:\n\t"
                         : [pMem]   "+Q"  (*pu8)
                         , [uOld]   "=&r" (u32Spill)
                         , [rc]     "=&r" (rcSpill)
                         , [fXchg]  "=&r" (fXchg.u)
                         : [uCmp]   "r"  ((uint32_t)u8Old)
                         , [uNew]   "r"  ((uint32_t)u8New)
                         , "[fXchg]" (0)
                           RTASM_ARM_DMB_SY_COMMA_IN_REG
                         : "cc");
    return fXchg.f;

# else
#  error "Port me"
# endif
}
#endif


/**
 * Atomically Compare and Exchange a signed 8-bit value, ordered.
 *
 * @returns true if xchg was done.
 * @returns false if xchg wasn't done.
 *
 * @param   pi8         Pointer to the value to update.
 * @param   i8New       The new value to assigned to *pi8.
 * @param   i8Old       The old value to *pi8 compare with.
 *
 * @remarks x86: Requires a 486 or later.
 * @todo Rename ASMAtomicCmpWriteS8
 */
DECLINLINE(bool) ASMAtomicCmpXchgS8(volatile int8_t RT_FAR *pi8, const int8_t i8New, const int8_t i8Old) RT_NOTHROW_DEF
{
    return ASMAtomicCmpXchgU8((volatile uint8_t RT_FAR *)pi8, (uint8_t)i8New, (uint8_t)i8Old);
}


/**
 * Atomically Compare and Exchange a bool value, ordered.
 *
 * @returns true if xchg was done.
 * @returns false if xchg wasn't done.
 *
 * @param   pf          Pointer to the value to update.
 * @param   fNew        The new value to assigned to *pf.
 * @param   fOld        The old value to *pf compare with.
 *
 * @remarks x86: Requires a 486 or later.
 * @todo Rename ASMAtomicCmpWriteBool
 */
DECLINLINE(bool) ASMAtomicCmpXchgBool(volatile bool RT_FAR *pf, const bool fNew, const bool fOld) RT_NOTHROW_DEF
{
    return ASMAtomicCmpXchgU8((volatile uint8_t RT_FAR *)pf, (uint8_t)fNew, (uint8_t)fOld);
}


/**
 * Atomically Compare and Exchange an unsigned 32-bit value, ordered.
 *
 * @returns true if xchg was done.
 * @returns false if xchg wasn't done.
 *
 * @param   pu32        Pointer to the value to update.
 * @param   u32New      The new value to assigned to *pu32.
 * @param   u32Old      The old value to *pu32 compare with.
 *
 * @remarks x86: Requires a 486 or later.
 * @todo Rename ASMAtomicCmpWriteU32
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(bool) ASMAtomicCmpXchgU32(volatile uint32_t RT_FAR *pu32, const uint32_t u32New, const uint32_t u32Old) RT_NOTHROW_PROTO;
#else
DECLINLINE(bool) ASMAtomicCmpXchgU32(volatile uint32_t RT_FAR *pu32, const uint32_t u32New, uint32_t u32Old) RT_NOTHROW_DEF
{
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    uint8_t u8Ret;
    __asm__ __volatile__("lock; cmpxchgl %3, %0\n\t"
                         "setz  %1\n\t"
                         : "=m" (*pu32)
                         , "=qm" (u8Ret)
                         , "=a" (u32Old)
                         : "r" (u32New)
                         , "2" (u32Old)
                         , "m" (*pu32)
                         : "cc");
    return (bool)u8Ret;

#  elif RT_INLINE_ASM_USES_INTRIN
    return (uint32_t)_InterlockedCompareExchange((long RT_FAR *)pu32, u32New, u32Old) == u32Old;

#  else
    uint32_t u32Ret;
    __asm
    {
#   ifdef RT_ARCH_AMD64
        mov     rdx, [pu32]
#   else
        mov     edx, [pu32]
#   endif
        mov     eax, [u32Old]
        mov     ecx, [u32New]
#   ifdef RT_ARCH_AMD64
        lock cmpxchg [rdx], ecx
#   else
        lock cmpxchg [edx], ecx
#   endif
        setz    al
        movzx   eax, al
        mov     [u32Ret], eax
    }
    return !!u32Ret;
#  endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    union { uint32_t u; bool f; } fXchg;
    uint32_t u32Spill;
    uint32_t rcSpill;
    __asm__ __volatile__(".Ltry_again_ASMAtomicCmpXchgU32_%=:\n\t"
                         RTASM_ARM_DMB_SY
#  if defined(RT_ARCH_ARM64)
                         "ldaxr     %w[uOld], %[pMem]\n\t"
                         "cmp       %w[uOld], %w[uCmp]\n\t"
                         "bne       1f\n\t"   /* stop here if not equal */
                         "stlxr     %w[rc], %w[uNew], %[pMem]\n\t"
                         "cbnz      %w[rc], .Ltry_again_ASMAtomicCmpXchgU32_%=\n\t"
                         "mov       %w[fXchg], #1\n\t"
#  else
                         "ldrex     %[uOld], %[pMem]\n\t"
                         "teq       %[uOld], %[uCmp]\n\t"
                         "strexeq   %[rc], %[uNew], %[pMem]\n\t"
                         "bne       1f\n\t"   /* stop here if not equal */
                         "cmp       %[rc], #0\n\t"
                         "bne       .Ltry_again_ASMAtomicCmpXchgU32_%=\n\t"
                         "mov       %[fXchg], #1\n\t"
#  endif
                         "1:\n\t"
                         : [pMem]   "+Q"  (*pu32)
                         , [uOld]   "=&r" (u32Spill)
                         , [rc]     "=&r" (rcSpill)
                         , [fXchg]  "=&r" (fXchg.u)
                         : [uCmp]   "r"  (u32Old)
                         , [uNew]   "r"  (u32New)
                         , "[fXchg]" (0)
                           RTASM_ARM_DMB_SY_COMMA_IN_REG
                         : "cc");
    return fXchg.f;

# else
#  error "Port me"
# endif
}
#endif


/**
 * Atomically Compare and Exchange a signed 32-bit value, ordered.
 *
 * @returns true if xchg was done.
 * @returns false if xchg wasn't done.
 *
 * @param   pi32        Pointer to the value to update.
 * @param   i32New      The new value to assigned to *pi32.
 * @param   i32Old      The old value to *pi32 compare with.
 *
 * @remarks x86: Requires a 486 or later.
 * @todo Rename ASMAtomicCmpWriteS32
 */
DECLINLINE(bool) ASMAtomicCmpXchgS32(volatile int32_t RT_FAR *pi32, const int32_t i32New, const int32_t i32Old) RT_NOTHROW_DEF
{
    return ASMAtomicCmpXchgU32((volatile uint32_t RT_FAR *)pi32, (uint32_t)i32New, (uint32_t)i32Old);
}


/**
 * Atomically Compare and exchange an unsigned 64-bit value, ordered.
 *
 * @returns true if xchg was done.
 * @returns false if xchg wasn't done.
 *
 * @param   pu64    Pointer to the 64-bit variable to update.
 * @param   u64New  The 64-bit value to assign to *pu64.
 * @param   u64Old  The value to compare with.
 *
 * @remarks x86: Requires a Pentium or later.
 * @todo Rename ASMAtomicCmpWriteU64
 */
#if (RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN) \
 || RT_INLINE_DONT_MIX_CMPXCHG8B_AND_PIC
RT_ASM_DECL_PRAGMA_WATCOM(bool) ASMAtomicCmpXchgU64(volatile uint64_t RT_FAR *pu64, const uint64_t u64New, const uint64_t u64Old) RT_NOTHROW_PROTO;
#else
DECLINLINE(bool) ASMAtomicCmpXchgU64(volatile uint64_t RT_FAR *pu64, uint64_t u64New, uint64_t u64Old) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
   return (uint64_t)_InterlockedCompareExchange64((__int64 RT_FAR *)pu64, u64New, u64Old) == u64Old;

# elif defined(RT_ARCH_AMD64)
#  if RT_INLINE_ASM_GNU_STYLE
    uint8_t u8Ret;
    __asm__ __volatile__("lock; cmpxchgq %3, %0\n\t"
                         "setz  %1\n\t"
                         : "=m" (*pu64)
                         , "=qm" (u8Ret)
                         , "=a" (u64Old)
                         : "r" (u64New)
                         , "2" (u64Old)
                         , "m" (*pu64)
                         : "cc");
    return (bool)u8Ret;
#  else
    bool fRet;
    __asm
    {
        mov     rdx, [pu32]
        mov     rax, [u64Old]
        mov     rcx, [u64New]
        lock cmpxchg [rdx], rcx
        setz    al
        mov     [fRet], al
    }
    return fRet;
#  endif

# elif defined(RT_ARCH_X86)
    uint32_t u32Ret;
#  if RT_INLINE_ASM_GNU_STYLE
#   if defined(PIC) || defined(__PIC__)
    uint32_t u32EBX = (uint32_t)u64New;
    uint32_t u32Spill;
    __asm__ __volatile__("xchgl %%ebx, %4\n\t"
                         "lock; cmpxchg8b (%6)\n\t"
                         "setz  %%al\n\t"
                         "movl  %4, %%ebx\n\t"
                         "movzbl %%al, %%eax\n\t"
                         : "=a" (u32Ret)
                         , "=d" (u32Spill)
#    if RT_GNUC_PREREQ(4, 3)
                         , "+m" (*pu64)
#    else
                         , "=m" (*pu64)
#    endif
                         : "A" (u64Old)
                         , "m" ( u32EBX )
                         , "c" ( (uint32_t)(u64New >> 32) )
                         , "S" (pu64)
                         : "cc");
#   else /* !PIC */
    uint32_t u32Spill;
    __asm__ __volatile__("lock; cmpxchg8b %2\n\t"
                         "setz  %%al\n\t"
                         "movzbl %%al, %%eax\n\t"
                         : "=a" (u32Ret)
                         , "=d" (u32Spill)
                         , "+m" (*pu64)
                         : "A" (u64Old)
                         , "b" ( (uint32_t)u64New )
                         , "c" ( (uint32_t)(u64New >> 32) )
                         : "cc");
#   endif
    return (bool)u32Ret;
#  else
    __asm
    {
        mov     ebx, dword ptr [u64New]
        mov     ecx, dword ptr [u64New + 4]
        mov     edi, [pu64]
        mov     eax, dword ptr [u64Old]
        mov     edx, dword ptr [u64Old + 4]
        lock cmpxchg8b [edi]
        setz    al
        movzx   eax, al
        mov     dword ptr [u32Ret], eax
    }
    return !!u32Ret;
#  endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    union { uint32_t u; bool f; } fXchg;
    uint64_t u64Spill;
    uint32_t rcSpill;
    __asm__ __volatile__(".Ltry_again_ASMAtomicCmpXchgU64_%=:\n\t"
                         RTASM_ARM_DMB_SY
#  if defined(RT_ARCH_ARM64)
                         "ldaxr     %[uOld], %[pMem]\n\t"
                         "cmp       %[uOld], %[uCmp]\n\t"
                         "bne       1f\n\t"   /* stop here if not equal */
                         "stlxr     %w[rc], %[uNew], %[pMem]\n\t"
                         "cbnz      %w[rc], .Ltry_again_ASMAtomicCmpXchgU64_%=\n\t"
                         "mov       %w[fXchg], #1\n\t"
#  else
                         "ldrexd    %[uOld], %H[uOld], %[pMem]\n\t"
                         "teq       %[uOld], %[uCmp]\n\t"
                         "teqeq     %H[uOld], %H[uCmp]\n\t"
                         "strexdeq  %[rc], %[uNew], %H[uNew], %[pMem]\n\t"
                         "bne       1f\n\t"   /* stop here if not equal */
                         "cmp       %[rc], #0\n\t"
                         "bne       .Ltry_again_ASMAtomicCmpXchgU64_%=\n\t"
                         "mov       %[fXchg], #1\n\t"
#  endif
                         "1:\n\t"
                         : [pMem]   "+Q"  (*pu64)
                         , [uOld]   "=&r" (u64Spill)
                         , [rc]     "=&r" (rcSpill)
                         , [fXchg]  "=&r" (fXchg.u)
                         : [uCmp]   "r"  (u64Old)
                         , [uNew]   "r"  (u64New)
                         , "[fXchg]" (0)
                           RTASM_ARM_DMB_SY_COMMA_IN_REG
                         : "cc");
    return fXchg.f;

# else
#  error "Port me"
# endif
}
#endif


/**
 * Atomically Compare and exchange a signed 64-bit value, ordered.
 *
 * @returns true if xchg was done.
 * @returns false if xchg wasn't done.
 *
 * @param   pi64    Pointer to the 64-bit variable to update.
 * @param   i64     The 64-bit value to assign to *pu64.
 * @param   i64Old  The value to compare with.
 *
 * @remarks x86: Requires a Pentium or later.
 * @todo Rename ASMAtomicCmpWriteS64
 */
DECLINLINE(bool) ASMAtomicCmpXchgS64(volatile int64_t RT_FAR *pi64, const int64_t i64, const int64_t i64Old) RT_NOTHROW_DEF
{
    return ASMAtomicCmpXchgU64((volatile uint64_t RT_FAR *)pi64, (uint64_t)i64, (uint64_t)i64Old);
}

#if defined(RT_ARCH_AMD64) || defined(RT_ARCH_ARM64) || defined(DOXYGEN_RUNNING)

/** @def RTASM_HAVE_CMP_WRITE_U128
 * Indicates that we've got ASMAtomicCmpWriteU128(), ASMAtomicCmpWriteU128v2()
 * and ASMAtomicCmpWriteExU128() available. */
# define RTASM_HAVE_CMP_WRITE_U128 1


/**
 * Atomically compare and write an unsigned 128-bit value, ordered.
 *
 * @returns true if write was done.
 * @returns false if write wasn't done.
 *
 * @param   pu128       Pointer to the 128-bit variable to update.
 * @param   u64NewHi    The high 64 bits of the value to assign to *pu128.
 * @param   u64NewLo    The low 64 bits of the value to assign to *pu128.
 * @param   u64OldHi    The high 64-bit of the value to compare with.
 * @param   u64OldLo    The low 64-bit of the value to compare with.
 *
 * @remarks AMD64: Not present in the earliest CPUs, so check CPUID.
 */
# if (RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN)
DECLASM(bool) ASMAtomicCmpWriteU128v2(volatile uint128_t *pu128, const uint64_t u64NewHi, const uint64_t u64NewLo,
                                      const uint64_t u64OldHi, const uint64_t u64OldLo) RT_NOTHROW_PROTO;
# else
DECLINLINE(bool) ASMAtomicCmpWriteU128v2(volatile uint128_t *pu128, const uint64_t u64NewHi, const uint64_t u64NewLo,
                                         const uint64_t u64OldHi, const uint64_t u64OldLo) RT_NOTHROW_DEF
{
#  if RT_INLINE_ASM_USES_INTRIN
    __int64 ai64Cmp[2];
    ai64Cmp[0] = u64OldLo;
    ai64Cmp[1] = u64OldHi;
    return _InterlockedCompareExchange128((__int64 volatile *)pu128, u64NewHi, u64NewLo, ai64Cmp) != 0;

#  elif (defined(__clang_major__) || defined(__GNUC__)) && defined(RT_ARCH_ARM64)
    return __sync_bool_compare_and_swap(pu128, ((uint128_t)u64OldHi << 64) | u64OldLo, ((uint128_t)u64NewHi << 64) | u64NewLo);

#  elif defined(RT_ARCH_AMD64)
#   if RT_INLINE_ASM_GNU_STYLE
    uint64_t u64Ret;
    uint64_t u64Spill;
    __asm__ __volatile__("lock; cmpxchg16b %2\n\t"
                         "setz  %%al\n\t"
                         "movzbl %%al, %%eax\n\t"
                         : "=a" (u64Ret)
                         , "=d" (u64Spill)
                         , "+m" (*pu128)
                         : "a" (u64OldLo)
                         , "d" (u64OldHi)
                         , "b" (u64NewLo)
                         , "c" (u64NewHi)
                         : "cc");

    return (bool)u64Ret;
#   else
#    error "Port me"
#   endif
#  else
#   error "Port me"
#  endif
}
# endif


/**
 * Atomically compare and write an unsigned 128-bit value, ordered.
 *
 * @returns true if write was done.
 * @returns false if write wasn't done.
 *
 * @param   pu128       Pointer to the 128-bit variable to update.
 * @param   u128New     The 128-bit value to assign to *pu128.
 * @param   u128Old     The value to compare with.
 *
 * @remarks AMD64: Not present in the earliest CPUs, so check CPUID.
 */
DECLINLINE(bool) ASMAtomicCmpWriteU128(volatile uint128_t *pu128, const uint128_t u128New, const uint128_t u128Old) RT_NOTHROW_DEF
{
# ifdef RT_COMPILER_WITH_128BIT_INT_TYPES
#  if (defined(__clang_major__) || defined(__GNUC__)) && defined(RT_ARCH_ARM64)
    return __sync_bool_compare_and_swap(pu128, u128Old, u128New);
#  else
    return ASMAtomicCmpWriteU128v2(pu128, (uint64_t)(u128New >> 64), (uint64_t)u128New,
                                   (uint64_t)(u128Old >> 64), (uint64_t)u128Old);
#  endif
# else
    return ASMAtomicCmpWriteU128v2(pu128, u128New.Hi, u128New.Lo, u128Old.Hi, u128Old.Lo);
# endif
}


/**
 * RTUINT128U wrapper for ASMAtomicCmpWriteU128.
 */
DECLINLINE(bool) ASMAtomicCmpWriteU128U(volatile RTUINT128U *pu128, const RTUINT128U u128New,
                                        const RTUINT128U u128Old) RT_NOTHROW_DEF
{
# if (defined(__clang_major__) || defined(__GNUC__)) && defined(RT_ARCH_ARM64)
    return ASMAtomicCmpWriteU128(&pu128->u, u128New.u, u128Old.u);
# else
    return ASMAtomicCmpWriteU128v2(&pu128->u, u128New.s.Hi, u128New.s.Lo, u128Old.s.Hi, u128Old.s.Lo);
# endif
}

#endif /* RT_ARCH_AMD64 || RT_ARCH_ARM64 */

/**
 * Atomically Compare and Exchange a pointer value, ordered.
 *
 * @returns true if xchg was done.
 * @returns false if xchg wasn't done.
 *
 * @param   ppv         Pointer to the value to update.
 * @param   pvNew       The new value to assigned to *ppv.
 * @param   pvOld       The old value to *ppv compare with.
 *
 * @remarks x86: Requires a 486 or later.
 * @todo Rename ASMAtomicCmpWritePtrVoid
 */
DECLINLINE(bool) ASMAtomicCmpXchgPtrVoid(void RT_FAR * volatile RT_FAR *ppv, const void RT_FAR *pvNew, const void RT_FAR *pvOld) RT_NOTHROW_DEF
{
#if ARCH_BITS == 32 || ARCH_BITS == 16
    return ASMAtomicCmpXchgU32((volatile uint32_t RT_FAR *)(void RT_FAR *)ppv, (uint32_t)pvNew, (uint32_t)pvOld);
#elif ARCH_BITS == 64
    return ASMAtomicCmpXchgU64((volatile uint64_t RT_FAR *)(void RT_FAR *)ppv, (uint64_t)pvNew, (uint64_t)pvOld);
#else
# error "ARCH_BITS is bogus"
#endif
}


/**
 * Atomically Compare and Exchange a pointer value, ordered.
 *
 * @returns true if xchg was done.
 * @returns false if xchg wasn't done.
 *
 * @param   ppv         Pointer to the value to update.
 * @param   pvNew       The new value to assigned to *ppv.
 * @param   pvOld       The old value to *ppv compare with.
 *
 * @remarks This is relatively type safe on GCC platforms.
 * @remarks x86: Requires a 486 or later.
 * @todo Rename ASMAtomicCmpWritePtr
 */
#ifdef __GNUC__
# define ASMAtomicCmpXchgPtr(ppv, pvNew, pvOld) \
    __extension__ \
    ({\
        __typeof__(*(ppv)) volatile * const ppvTypeChecked   = (ppv); \
        __typeof__(*(ppv))            const pvNewTypeChecked = (pvNew); \
        __typeof__(*(ppv))            const pvOldTypeChecked = (pvOld); \
        bool fMacroRet = ASMAtomicCmpXchgPtrVoid((void * volatile *)ppvTypeChecked, \
                                                 (void *)pvNewTypeChecked, (void *)pvOldTypeChecked); \
        fMacroRet; \
     })
#else
# define ASMAtomicCmpXchgPtr(ppv, pvNew, pvOld) \
    ASMAtomicCmpXchgPtrVoid((void RT_FAR * volatile RT_FAR *)(ppv), (void RT_FAR *)(pvNew), (void RT_FAR *)(pvOld))
#endif


/** @def ASMAtomicCmpXchgHandle
 * Atomically Compare and Exchange a typical IPRT handle value, ordered.
 *
 * @param   ph          Pointer to the value to update.
 * @param   hNew        The new value to assigned to *pu.
 * @param   hOld        The old value to *pu compare with.
 * @param   fRc         Where to store the result.
 *
 * @remarks This doesn't currently work for all handles (like RTFILE).
 * @remarks x86: Requires a 486 or later.
 * @todo Rename ASMAtomicCmpWriteHandle
 */
#if HC_ARCH_BITS == 32 || ARCH_BITS == 16
# define ASMAtomicCmpXchgHandle(ph, hNew, hOld, fRc) \
   do { \
       AssertCompile(sizeof(*(ph)) == sizeof(uint32_t)); \
       (fRc) = ASMAtomicCmpXchgU32((uint32_t volatile RT_FAR *)(ph), (const uint32_t)(hNew), (const uint32_t)(hOld)); \
   } while (0)
#elif HC_ARCH_BITS == 64
# define ASMAtomicCmpXchgHandle(ph, hNew, hOld, fRc) \
   do { \
       AssertCompile(sizeof(*(ph)) == sizeof(uint64_t)); \
       (fRc) = ASMAtomicCmpXchgU64((uint64_t volatile RT_FAR *)(ph), (const uint64_t)(hNew), (const uint64_t)(hOld)); \
   } while (0)
#else
# error HC_ARCH_BITS
#endif


/** @def ASMAtomicCmpXchgSize
 * Atomically Compare and Exchange a value which size might differ
 * between platforms or compilers, ordered.
 *
 * @param   pu          Pointer to the value to update.
 * @param   uNew        The new value to assigned to *pu.
 * @param   uOld        The old value to *pu compare with.
 * @param   fRc         Where to store the result.
 *
 * @remarks x86: Requires a 486 or later.
 * @todo Rename ASMAtomicCmpWriteSize
 */
#define ASMAtomicCmpXchgSize(pu, uNew, uOld, fRc) \
    do { \
        switch (sizeof(*(pu))) { \
            case 4: (fRc) = ASMAtomicCmpXchgU32((volatile uint32_t RT_FAR *)(void RT_FAR *)(pu), (uint32_t)(uNew), (uint32_t)(uOld)); \
                break; \
            case 8: (fRc) = ASMAtomicCmpXchgU64((volatile uint64_t RT_FAR *)(void RT_FAR *)(pu), (uint64_t)(uNew), (uint64_t)(uOld)); \
                break; \
            default: AssertMsgFailed(("ASMAtomicCmpXchgSize: size %d is not supported\n", sizeof(*(pu)))); \
                (fRc) = false; \
                break; \
        } \
    } while (0)


/**
 * Atomically Compare and Exchange an unsigned 8-bit value, additionally passes
 * back old value, ordered.
 *
 * @returns true if xchg was done.
 * @returns false if xchg wasn't done.
 *
 * @param   pu8         Pointer to the value to update.
 * @param   u8New       The new value to assigned to *pu32.
 * @param   u8Old       The old value to *pu8 compare with.
 * @param   pu8Old      Pointer store the old value at.
 *
 * @remarks x86: Requires a 486 or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(bool) ASMAtomicCmpXchgExU8(volatile uint8_t RT_FAR *pu8, const uint8_t u8New, const uint8_t u8Old, uint8_t RT_FAR *pu8Old) RT_NOTHROW_PROTO;
#else
DECLINLINE(bool) ASMAtomicCmpXchgExU8(volatile uint8_t RT_FAR *pu8, const uint8_t u8New, const uint8_t u8Old, uint8_t RT_FAR *pu8Old) RT_NOTHROW_DEF
{
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    uint8_t u8Ret;
    __asm__ __volatile__("lock; cmpxchgb %3, %0\n\t"
                         "setz  %1\n\t"
                         : "=m" (*pu8)
                         , "=qm" (u8Ret)
                         , "=a" (*pu8Old)
#  if defined(RT_ARCH_X86)
                         : "q" (u8New)
#  else
                         : "r" (u8New)
#  endif
                         , "a" (u8Old)
                         , "m" (*pu8)
                         : "cc");
    return (bool)u8Ret;

#  elif RT_INLINE_ASM_USES_INTRIN
    return (*pu8Old = _InterlockedCompareExchange8((char RT_FAR *)pu8, u8New, u8Old)) == u8Old;

#  else
    uint8_t u8Ret;
    __asm
    {
#   ifdef RT_ARCH_AMD64
        mov     rdx, [pu8]
#   else
        mov     edx, [pu8]
#   endif
        mov     eax, [u8Old]
        mov     ecx, [u8New]
#   ifdef RT_ARCH_AMD64
        lock cmpxchg [rdx], ecx
        mov     rdx, [pu8Old]
        mov     [rdx], eax
#   else
        lock cmpxchg [edx], ecx
        mov     edx, [pu8Old]
        mov     [edx], eax
#   endif
        setz    al
        movzx   eax, al
        mov     [u8Ret], eax
    }
    return !!u8Ret;
#  endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    union { uint8_t u; bool f; } fXchg;
    uint8_t u8ActualOld;
    uint8_t rcSpill;
    __asm__ __volatile__(".Ltry_again_ASMAtomicCmpXchgExU8_%=:\n\t"
                         RTASM_ARM_DMB_SY
#  if defined(RT_ARCH_ARM64)
                         "ldaxrb    %w[uOld], %[pMem]\n\t"
                         "cmp       %w[uOld], %w[uCmp]\n\t"
                         "bne       1f\n\t"   /* stop here if not equal */
                         "stlxrb    %w[rc], %w[uNew], %[pMem]\n\t"
                         "cbnz      %w[rc], .Ltry_again_ASMAtomicCmpXchgExU8_%=\n\t"
                         "mov       %w[fXchg], #1\n\t"
#  else
                         "ldrexb     %[uOld], %[pMem]\n\t"
                         "teq       %[uOld], %[uCmp]\n\t"
                         "strexbeq  %[rc], %[uNew], %[pMem]\n\t"
                         "bne       1f\n\t"   /* stop here if not equal */
                         "cmp       %[rc], #0\n\t"
                         "bne       .Ltry_again_ASMAtomicCmpXchgExU8_%=\n\t"
                         "mov       %[fXchg], #1\n\t"
#  endif
                         "1:\n\t"
                         : [pMem]   "+Q"  (*pu8)
                         , [uOld]   "=&r" (u8ActualOld)
                         , [rc]     "=&r" (rcSpill)
                         , [fXchg]  "=&r" (fXchg.u)
                         : [uCmp]   "r"  (u8Old)
                         , [uNew]   "r"  (u8New)
                         , "[fXchg]" (0)
                           RTASM_ARM_DMB_SY_COMMA_IN_REG
                         : "cc");
    *pu8Old = u8ActualOld;
    return fXchg.f;

# else
#  error "Port me"
# endif
}
#endif


/**
 * Atomically Compare and Exchange a signed 8-bit value, additionally
 * passes back old value, ordered.
 *
 * @returns true if xchg was done.
 * @returns false if xchg wasn't done.
 *
 * @param   pi8         Pointer to the value to update.
 * @param   i8New       The new value to assigned to *pi8.
 * @param   i8Old       The old value to *pi8 compare with.
 * @param   pi8Old      Pointer store the old value at.
 *
 * @remarks x86: Requires a 486 or later.
 */
DECLINLINE(bool) ASMAtomicCmpXchgExS8(volatile int8_t RT_FAR *pi8, const int8_t i8New, const int8_t i8Old, int8_t RT_FAR *pi8Old) RT_NOTHROW_DEF
{
    return ASMAtomicCmpXchgExU8((volatile uint8_t RT_FAR *)pi8, (uint8_t)i8New, (uint8_t)i8Old, (uint8_t RT_FAR *)pi8Old);
}


/**
 * Atomically Compare and Exchange an unsigned 16-bit value, additionally passes
 * back old value, ordered.
 *
 * @returns true if xchg was done.
 * @returns false if xchg wasn't done.
 *
 * @param   pu16        Pointer to the value to update.
 * @param   u16New      The new value to assigned to *pu16.
 * @param   u16Old      The old value to *pu32 compare with.
 * @param   pu16Old     Pointer store the old value at.
 *
 * @remarks x86: Requires a 486 or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(bool) ASMAtomicCmpXchgExU16(volatile uint16_t RT_FAR *pu16, const uint16_t u16New, const uint16_t u16Old, uint16_t RT_FAR *pu16Old) RT_NOTHROW_PROTO;
#else
DECLINLINE(bool) ASMAtomicCmpXchgExU16(volatile uint16_t RT_FAR *pu16, const uint16_t u16New, const uint16_t u16Old, uint16_t RT_FAR *pu16Old) RT_NOTHROW_DEF
{
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    uint8_t u8Ret;
    __asm__ __volatile__("lock; cmpxchgw %3, %0\n\t"
                         "setz  %1\n\t"
                         : "=m" (*pu16)
                         , "=qm" (u8Ret)
                         , "=a" (*pu16Old)
                         : "r" (u16New)
                         , "a" (u16Old)
                         , "m" (*pu16)
                         : "cc");
    return (bool)u8Ret;

#  elif RT_INLINE_ASM_USES_INTRIN
    return (*pu16Old = _InterlockedCompareExchange16((short RT_FAR *)pu16, u16New, u16Old)) == u16Old;

#  else
    uint16_t u16Ret;
    __asm
    {
#   ifdef RT_ARCH_AMD64
        mov     rdx, [pu16]
#   else
        mov     edx, [pu16]
#   endif
        mov     eax, [u16Old]
        mov     ecx, [u16New]
#   ifdef RT_ARCH_AMD64
        lock cmpxchg [rdx], ecx
        mov     rdx, [pu16Old]
        mov     [rdx], eax
#   else
        lock cmpxchg [edx], ecx
        mov     edx, [pu16Old]
        mov     [edx], eax
#   endif
        setz    al
        movzx   eax, al
        mov     [u16Ret], eax
    }
    return !!u16Ret;
#  endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    union { uint16_t u; bool f; } fXchg;
    uint16_t u16ActualOld;
    uint16_t rcSpill;
    __asm__ __volatile__(".Ltry_again_ASMAtomicCmpXchgExU16_%=:\n\t"
                         RTASM_ARM_DMB_SY
#  if defined(RT_ARCH_ARM64)
                         "ldaxrh    %w[uOld], %[pMem]\n\t"
                         "cmp       %w[uOld], %w[uCmp]\n\t"
                         "bne       1f\n\t"   /* stop here if not equal */
                         "stlxrh    %w[rc], %w[uNew], %[pMem]\n\t"
                         "cbnz      %w[rc], .Ltry_again_ASMAtomicCmpXchgExU16_%=\n\t"
                         "mov       %w[fXchg], #1\n\t"
#  else
                         "ldrexh     %[uOld], %[pMem]\n\t"
                         "teq       %[uOld], %[uCmp]\n\t"
                         "strexheq  %[rc], %[uNew], %[pMem]\n\t"
                         "bne       1f\n\t"   /* stop here if not equal */
                         "cmp       %[rc], #0\n\t"
                         "bne       .Ltry_again_ASMAtomicCmpXchgExU16_%=\n\t"
                         "mov       %[fXchg], #1\n\t"
#  endif
                         "1:\n\t"
                         : [pMem]   "+Q"  (*pu16)
                         , [uOld]   "=&r" (u16ActualOld)
                         , [rc]     "=&r" (rcSpill)
                         , [fXchg]  "=&r" (fXchg.u)
                         : [uCmp]   "r"  (u16Old)
                         , [uNew]   "r"  (u16New)
                         , "[fXchg]" (0)
                           RTASM_ARM_DMB_SY_COMMA_IN_REG
                         : "cc");
    *pu16Old = u16ActualOld;
    return fXchg.f;

# else
#  error "Port me"
# endif
}
#endif


/**
 * Atomically Compare and Exchange a signed 16-bit value, additionally
 * passes back old value, ordered.
 *
 * @returns true if xchg was done.
 * @returns false if xchg wasn't done.
 *
 * @param   pi16        Pointer to the value to update.
 * @param   i16New      The new value to assigned to *pi16.
 * @param   i16Old      The old value to *pi16 compare with.
 * @param   pi16Old     Pointer store the old value at.
 *
 * @remarks x86: Requires a 486 or later.
 */
DECLINLINE(bool) ASMAtomicCmpXchgExS16(volatile int16_t RT_FAR *pi16, const int16_t i16New, const int16_t i16Old, int16_t RT_FAR *pi16Old) RT_NOTHROW_DEF
{
    return ASMAtomicCmpXchgExU16((volatile uint16_t RT_FAR *)pi16, (uint16_t)i16New, (uint16_t)i16Old, (uint16_t RT_FAR *)pi16Old);
}


/**
 * Atomically Compare and Exchange an unsigned 32-bit value, additionally
 * passes back old value, ordered.
 *
 * @returns true if xchg was done.
 * @returns false if xchg wasn't done.
 *
 * @param   pu32        Pointer to the value to update.
 * @param   u32New      The new value to assigned to *pu32.
 * @param   u32Old      The old value to *pu32 compare with.
 * @param   pu32Old     Pointer store the old value at.
 *
 * @remarks x86: Requires a 486 or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(bool) ASMAtomicCmpXchgExU32(volatile uint32_t RT_FAR *pu32, const uint32_t u32New, const uint32_t u32Old, uint32_t RT_FAR *pu32Old) RT_NOTHROW_PROTO;
#else
DECLINLINE(bool) ASMAtomicCmpXchgExU32(volatile uint32_t RT_FAR *pu32, const uint32_t u32New, const uint32_t u32Old, uint32_t RT_FAR *pu32Old) RT_NOTHROW_DEF
{
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    uint8_t u8Ret;
    __asm__ __volatile__("lock; cmpxchgl %3, %0\n\t"
                         "setz  %1\n\t"
                         : "=m" (*pu32)
                         , "=qm" (u8Ret)
                         , "=a" (*pu32Old)
                         : "r" (u32New)
                         , "a" (u32Old)
                         , "m" (*pu32)
                         : "cc");
    return (bool)u8Ret;

#  elif RT_INLINE_ASM_USES_INTRIN
    return (*pu32Old = _InterlockedCompareExchange((long RT_FAR *)pu32, u32New, u32Old)) == u32Old;

#  else
    uint32_t u32Ret;
    __asm
    {
#   ifdef RT_ARCH_AMD64
        mov     rdx, [pu32]
#   else
        mov     edx, [pu32]
#   endif
        mov     eax, [u32Old]
        mov     ecx, [u32New]
#   ifdef RT_ARCH_AMD64
        lock cmpxchg [rdx], ecx
        mov     rdx, [pu32Old]
        mov     [rdx], eax
#   else
        lock cmpxchg [edx], ecx
        mov     edx, [pu32Old]
        mov     [edx], eax
#   endif
        setz    al
        movzx   eax, al
        mov     [u32Ret], eax
    }
    return !!u32Ret;
#  endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    union { uint32_t u; bool f; } fXchg;
    uint32_t u32ActualOld;
    uint32_t rcSpill;
    __asm__ __volatile__(".Ltry_again_ASMAtomicCmpXchgExU32_%=:\n\t"
                         RTASM_ARM_DMB_SY
#  if defined(RT_ARCH_ARM64)
                         "ldaxr     %w[uOld], %[pMem]\n\t"
                         "cmp       %w[uOld], %w[uCmp]\n\t"
                         "bne       1f\n\t"   /* stop here if not equal */
                         "stlxr     %w[rc], %w[uNew], %[pMem]\n\t"
                         "cbnz      %w[rc], .Ltry_again_ASMAtomicCmpXchgExU32_%=\n\t"
                         "mov       %w[fXchg], #1\n\t"
#  else
                         "ldrex     %[uOld], %[pMem]\n\t"
                         "teq       %[uOld], %[uCmp]\n\t"
                         "strexeq   %[rc], %[uNew], %[pMem]\n\t"
                         "bne       1f\n\t"   /* stop here if not equal */
                         "cmp       %[rc], #0\n\t"
                         "bne       .Ltry_again_ASMAtomicCmpXchgExU32_%=\n\t"
                         "mov       %[fXchg], #1\n\t"
#  endif
                         "1:\n\t"
                         : [pMem]   "+Q"  (*pu32)
                         , [uOld]   "=&r" (u32ActualOld)
                         , [rc]     "=&r" (rcSpill)
                         , [fXchg]  "=&r" (fXchg.u)
                         : [uCmp]   "r"  (u32Old)
                         , [uNew]   "r"  (u32New)
                         , "[fXchg]" (0)
                           RTASM_ARM_DMB_SY_COMMA_IN_REG
                         : "cc");
    *pu32Old = u32ActualOld;
    return fXchg.f;

# else
#  error "Port me"
# endif
}
#endif


/**
 * Atomically Compare and Exchange a signed 32-bit value, additionally
 * passes back old value, ordered.
 *
 * @returns true if xchg was done.
 * @returns false if xchg wasn't done.
 *
 * @param   pi32        Pointer to the value to update.
 * @param   i32New      The new value to assigned to *pi32.
 * @param   i32Old      The old value to *pi32 compare with.
 * @param   pi32Old     Pointer store the old value at.
 *
 * @remarks x86: Requires a 486 or later.
 */
DECLINLINE(bool) ASMAtomicCmpXchgExS32(volatile int32_t RT_FAR *pi32, const int32_t i32New, const int32_t i32Old, int32_t RT_FAR *pi32Old) RT_NOTHROW_DEF
{
    return ASMAtomicCmpXchgExU32((volatile uint32_t RT_FAR *)pi32, (uint32_t)i32New, (uint32_t)i32Old, (uint32_t RT_FAR *)pi32Old);
}


/**
 * Atomically Compare and exchange an unsigned 64-bit value, additionally
 * passing back old value, ordered.
 *
 * @returns true if xchg was done.
 * @returns false if xchg wasn't done.
 *
 * @param   pu64    Pointer to the 64-bit variable to update.
 * @param   u64New  The 64-bit value to assign to *pu64.
 * @param   u64Old  The value to compare with.
 * @param   pu64Old     Pointer store the old value at.
 *
 * @remarks x86: Requires a Pentium or later.
 */
#if (RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN) \
 || RT_INLINE_DONT_MIX_CMPXCHG8B_AND_PIC
RT_ASM_DECL_PRAGMA_WATCOM(bool) ASMAtomicCmpXchgExU64(volatile uint64_t RT_FAR *pu64, const uint64_t u64New, const uint64_t u64Old, uint64_t RT_FAR *pu64Old) RT_NOTHROW_PROTO;
#else
DECLINLINE(bool) ASMAtomicCmpXchgExU64(volatile uint64_t RT_FAR *pu64, const uint64_t u64New, const uint64_t u64Old, uint64_t RT_FAR *pu64Old) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
   return (*pu64Old =_InterlockedCompareExchange64((__int64 RT_FAR *)pu64, u64New, u64Old)) == u64Old;

# elif defined(RT_ARCH_AMD64)
#  if RT_INLINE_ASM_GNU_STYLE
    uint8_t u8Ret;
    __asm__ __volatile__("lock; cmpxchgq %3, %0\n\t"
                         "setz  %1\n\t"
                         : "=m" (*pu64)
                         , "=qm" (u8Ret)
                         , "=a" (*pu64Old)
                         : "r" (u64New)
                         , "a" (u64Old)
                         , "m" (*pu64)
                         : "cc");
    return (bool)u8Ret;
#  else
    bool fRet;
    __asm
    {
        mov     rdx, [pu32]
        mov     rax, [u64Old]
        mov     rcx, [u64New]
        lock cmpxchg [rdx], rcx
        mov     rdx, [pu64Old]
        mov     [rdx], rax
        setz    al
        mov     [fRet], al
    }
    return fRet;
#  endif

# elif defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    uint64_t u64Ret;
#   if defined(PIC) || defined(__PIC__)
    /* Note #1: This code uses a memory clobber description, because the clean
                solution with an output value for *pu64 makes gcc run out of
                registers.  This will cause suboptimal code, and anyone with a
                better solution is welcome to improve this.

       Note #2: We must prevent gcc from encoding the memory access, as it
                may go via the GOT if we're working on a global variable (like
                in the testcase).  Thus we request a register (%3) and
                dereference it ourselves. */
    __asm__ __volatile__("xchgl %%ebx, %1\n\t"
                         "lock; cmpxchg8b (%3)\n\t"
                         "xchgl %%ebx, %1\n\t"
                         : "=A" (u64Ret)
                         : "DS" ((uint32_t)u64New)
                         , "c" ((uint32_t)(u64New >> 32))
                         , "r" (pu64) /* Do not use "m" here*/
                         , "0" (u64Old)
                         : "memory"
                         , "cc" );
#   else /* !PIC */
    __asm__ __volatile__("lock; cmpxchg8b %4\n\t"
                         : "=A" (u64Ret)
                         , "=m" (*pu64)
                         : "b" ((uint32_t)u64New)
                         , "c" ((uint32_t)(u64New >> 32))
                         , "m" (*pu64)
                         , "0" (u64Old)
                         : "cc");
#   endif
    *pu64Old = u64Ret;
    return u64Ret == u64Old;
#  else
    uint32_t u32Ret;
    __asm
    {
        mov     ebx, dword ptr [u64New]
        mov     ecx, dword ptr [u64New + 4]
        mov     edi, [pu64]
        mov     eax, dword ptr [u64Old]
        mov     edx, dword ptr [u64Old + 4]
        lock cmpxchg8b [edi]
        mov     ebx, [pu64Old]
        mov     [ebx], eax
        setz    al
        movzx   eax, al
        add     ebx, 4
        mov     [ebx], edx
        mov     dword ptr [u32Ret], eax
    }
    return !!u32Ret;
#  endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    union { uint32_t u; bool f; } fXchg;
    uint64_t u64ActualOld;
    uint32_t rcSpill;
    __asm__ __volatile__(".Ltry_again_ASMAtomicCmpXchgU64_%=:\n\t"
                         RTASM_ARM_DMB_SY
#  if defined(RT_ARCH_ARM64)
                         "ldaxr     %[uOld], %[pMem]\n\t"
                         "cmp       %[uOld], %[uCmp]\n\t"
                         "bne       1f\n\t"   /* stop here if not equal */
                         "stlxr     %w[rc], %[uNew], %[pMem]\n\t"
                         "cbnz      %w[rc], .Ltry_again_ASMAtomicCmpXchgU64_%=\n\t"
                         "mov       %w[fXchg], #1\n\t"
#  else
                         "ldrexd    %[uOld], %H[uOld], %[pMem]\n\t"
                         "teq       %[uOld], %[uCmp]\n\t"
                         "teqeq     %H[uOld], %H[uCmp]\n\t"
                         "strexdeq  %[rc], %[uNew], %H[uNew], %[pMem]\n\t"
                         "bne       1f\n\t"   /* stop here if not equal */
                         "cmp       %[rc], #0\n\t"
                         "bne       .Ltry_again_ASMAtomicCmpXchgU64_%=\n\t"
                         "mov       %[fXchg], #1\n\t"
#  endif
                         "1:\n\t"
                         : [pMem]   "+Q"  (*pu64)
                         , [uOld]   "=&r" (u64ActualOld)
                         , [rc]     "=&r" (rcSpill)
                         , [fXchg]  "=&r" (fXchg.u)
                         : [uCmp]   "r"  (u64Old)
                         , [uNew]   "r"  (u64New)
                         , "[fXchg]" (0)
                         RTASM_ARM_DMB_SY_COMMA_IN_REG
                         : "cc");
    *pu64Old = u64ActualOld;
    return fXchg.f;

# else
#  error "Port me"
# endif
}
#endif


/**
 * Atomically Compare and exchange a signed 64-bit value, additionally
 * passing back old value, ordered.
 *
 * @returns true if xchg was done.
 * @returns false if xchg wasn't done.
 *
 * @param   pi64    Pointer to the 64-bit variable to update.
 * @param   i64     The 64-bit value to assign to *pu64.
 * @param   i64Old  The value to compare with.
 * @param   pi64Old Pointer store the old value at.
 *
 * @remarks x86: Requires a Pentium or later.
 */
DECLINLINE(bool) ASMAtomicCmpXchgExS64(volatile int64_t RT_FAR *pi64, const int64_t i64, const int64_t i64Old, int64_t RT_FAR *pi64Old) RT_NOTHROW_DEF
{
    return ASMAtomicCmpXchgExU64((volatile uint64_t RT_FAR *)pi64, (uint64_t)i64, (uint64_t)i64Old, (uint64_t RT_FAR *)pi64Old);
}

#if defined(RT_ARCH_AMD64) || defined(RT_ARCH_ARM64) || defined(DOXYGEN_RUNNING)

/** @def RTASM_HAVE_CMP_XCHG_U128
 * Indicates that we've got ASMAtomicCmpSwapU128(), ASMAtomicCmpSwapU128v2()
 * and ASMAtomicCmpSwapExU128() available. */
# define RTASM_HAVE_CMP_XCHG_U128 1


/**
 * Atomically compare and exchange an unsigned 128-bit value, ordered.
 *
 * @returns true if exchange was done.
 * @returns false if exchange wasn't done.
 *
 * @param   pu128       Pointer to the 128-bit variable to update.
 * @param   u64NewHi    The high 64 bits of the value to assign to *pu128.
 * @param   u64NewLo    The low 64 bits of the value to assign to *pu128.
 * @param   u64OldHi    The high 64-bit of the value to compare with.
 * @param   u64OldLo    The low 64-bit of the value to compare with.
 * @param   pu128Old    Where to return the old value.
 *
 * @remarks AMD64: Not present in the earliest CPUs, so check CPUID.
 */
# if (RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN)
DECLASM(bool) ASMAtomicCmpXchgU128v2(volatile uint128_t *pu128, const uint64_t u64NewHi, const uint64_t u64NewLo,
                                     const uint64_t u64OldHi, const uint64_t u64OldLo, uint128_t *pu128Old) RT_NOTHROW_PROTO;
# else
DECLINLINE(bool) ASMAtomicCmpXchgU128v2(volatile uint128_t *pu128, const uint64_t u64NewHi, const uint64_t u64NewLo,
                                        const uint64_t u64OldHi, const uint64_t u64OldLo, uint128_t *pu128Old) RT_NOTHROW_DEF
{
#  if RT_INLINE_ASM_USES_INTRIN
    pu128Old->Hi = u64OldHi;
    pu128Old->Lo = u64OldLo;
    AssertCompileMemberOffset(uint128_t, Lo, 0);
    return _InterlockedCompareExchange128((__int64 volatile *)pu128, u64NewHi, u64NewLo, (__int64 *)&pu128Old->Lo) != 0;

#  elif (defined(__clang_major__) || defined(__GNUC__)) && defined(RT_ARCH_ARM64)
    uint128_t const uCmp = ((uint128_t)u64OldHi << 64) | u64OldLo;
    uint128_t const uOld = __sync_val_compare_and_swap(pu128, uCmp, ((uint128_t)u64NewHi << 64) | u64NewLo);
    *pu128Old = uOld;
    return uCmp == uOld;

#  elif defined(RT_ARCH_AMD64)
#   if RT_INLINE_ASM_GNU_STYLE
    uint8_t bRet;
    uint64_t u64RetHi, u64RetLo;
    __asm__ __volatile__("lock; cmpxchg16b %3\n\t"
                         "setz  %b0\n\t"
                         : "=r" (bRet)
                         , "=a" (u64RetLo)
                         , "=d" (u64RetHi)
                         , "+m" (*pu128)
                         : "a" (u64OldLo)
                         , "d" (u64OldHi)
                         , "b" (u64NewLo)
                         , "c" (u64NewHi)
                         : "cc");
    *pu128Old = ((uint128_t)u64RetHi << 64) | u64RetLo;
    return (bool)bRet;
#   else
#    error "Port me"
#   endif
#  else
#   error "Port me"
#  endif
}
# endif


/**
 * Atomically compare and exchange an unsigned 128-bit value, ordered.
 *
 * @returns true if exchange was done.
 * @returns false if exchange wasn't done.
 *
 * @param   pu128       Pointer to the 128-bit variable to update.
 * @param   u128New     The 128-bit value to assign to *pu128.
 * @param   u128Old     The value to compare with.
 * @param   pu128Old    Where to return the old value.
 *
 * @remarks AMD64: Not present in the earliest CPUs, so check CPUID.
 */
DECLINLINE(bool) ASMAtomicCmpXchgU128(volatile uint128_t *pu128, const uint128_t u128New,
                                      const uint128_t u128Old, uint128_t *pu128Old) RT_NOTHROW_DEF
{
# ifdef RT_COMPILER_WITH_128BIT_INT_TYPES
#  if (defined(__clang_major__) || defined(__GNUC__)) && defined(RT_ARCH_ARM64)
    uint128_t const uSwapped = __sync_val_compare_and_swap(pu128, u128Old, u128New);
    *pu128Old = uSwapped;
    return uSwapped == u128Old;
#  else
    return ASMAtomicCmpXchgU128v2(pu128, (uint64_t)(u128New >> 64), (uint64_t)u128New,
                                  (uint64_t)(u128Old >> 64), (uint64_t)u128Old, pu128Old);
#  endif
# else
    return ASMAtomicCmpXchgU128v2(pu128, u128New.Hi, u128New.Lo, u128Old.Hi, u128Old.Lo, pu128Old);
# endif
}


/**
 * RTUINT128U wrapper for ASMAtomicCmpXchgU128.
 */
DECLINLINE(bool) ASMAtomicCmpXchgU128U(volatile RTUINT128U *pu128, const RTUINT128U u128New,
                                        const RTUINT128U u128Old, PRTUINT128U pu128Old) RT_NOTHROW_DEF
{
# if (defined(__clang_major__) || defined(__GNUC__)) && defined(RT_ARCH_ARM64)
    return ASMAtomicCmpXchgU128(&pu128->u, u128New.u, u128Old.u, &pu128Old->u);
# else
    return ASMAtomicCmpXchgU128v2(&pu128->u, u128New.s.Hi, u128New.s.Lo, u128Old.s.Hi, u128Old.s.Lo, &pu128Old->u);
# endif
}

#endif /* RT_ARCH_AMD64 || RT_ARCH_ARM64 */



/** @def ASMAtomicCmpXchgExHandle
 * Atomically Compare and Exchange a typical IPRT handle value, ordered.
 *
 * @param   ph          Pointer to the value to update.
 * @param   hNew        The new value to assigned to *pu.
 * @param   hOld        The old value to *pu compare with.
 * @param   fRc         Where to store the result.
 * @param   phOldVal    Pointer to where to store the old value.
 *
 * @remarks This doesn't currently work for all handles (like RTFILE).
 */
#if HC_ARCH_BITS == 32 || ARCH_BITS == 16
# define ASMAtomicCmpXchgExHandle(ph, hNew, hOld, fRc, phOldVal) \
    do { \
        AssertCompile(sizeof(*ph)       == sizeof(uint32_t)); \
        AssertCompile(sizeof(*phOldVal) == sizeof(uint32_t)); \
        (fRc) = ASMAtomicCmpXchgExU32((volatile uint32_t RT_FAR *)(ph), (uint32_t)(hNew), (uint32_t)(hOld), (uint32_t RT_FAR *)(phOldVal)); \
    } while (0)
#elif HC_ARCH_BITS == 64
# define ASMAtomicCmpXchgExHandle(ph, hNew, hOld, fRc, phOldVal) \
    do { \
        AssertCompile(sizeof(*(ph))       == sizeof(uint64_t)); \
        AssertCompile(sizeof(*(phOldVal)) == sizeof(uint64_t)); \
        (fRc) = ASMAtomicCmpXchgExU64((volatile uint64_t RT_FAR *)(ph), (uint64_t)(hNew), (uint64_t)(hOld), (uint64_t RT_FAR *)(phOldVal)); \
    } while (0)
#else
# error HC_ARCH_BITS
#endif


/** @def ASMAtomicCmpXchgExSize
 * Atomically Compare and Exchange a value which size might differ
 * between platforms or compilers. Additionally passes back old value.
 *
 * @param   pu          Pointer to the value to update.
 * @param   uNew        The new value to assigned to *pu.
 * @param   uOld        The old value to *pu compare with.
 * @param   fRc         Where to store the result.
 * @param   puOldVal    Pointer to where to store the old value.
 *
 * @remarks x86: Requires a 486 or later.
 */
#define ASMAtomicCmpXchgExSize(pu, uNew, uOld, fRc, puOldVal) \
    do { \
        switch (sizeof(*(pu))) { \
            case 4: (fRc) = ASMAtomicCmpXchgExU32((volatile uint32_t RT_FAR *)(void RT_FAR *)(pu), (uint32_t)(uNew), (uint32_t)(uOld), (uint32_t RT_FAR *)(uOldVal)); \
                break; \
            case 8: (fRc) = ASMAtomicCmpXchgExU64((volatile uint64_t RT_FAR *)(void RT_FAR *)(pu), (uint64_t)(uNew), (uint64_t)(uOld), (uint64_t RT_FAR *)(uOldVal)); \
                break; \
            default: AssertMsgFailed(("ASMAtomicCmpXchgSize: size %d is not supported\n", sizeof(*(pu)))); \
                (fRc) = false; \
                (uOldVal) = 0; \
                break; \
        } \
    } while (0)


/**
 * Atomically Compare and Exchange a pointer value, additionally
 * passing back old value, ordered.
 *
 * @returns true if xchg was done.
 * @returns false if xchg wasn't done.
 *
 * @param   ppv         Pointer to the value to update.
 * @param   pvNew       The new value to assigned to *ppv.
 * @param   pvOld       The old value to *ppv compare with.
 * @param   ppvOld      Pointer store the old value at.
 *
 * @remarks x86: Requires a 486 or later.
 */
DECLINLINE(bool) ASMAtomicCmpXchgExPtrVoid(void RT_FAR * volatile RT_FAR *ppv, const void RT_FAR *pvNew, const void RT_FAR *pvOld,
                                           void RT_FAR * RT_FAR *ppvOld) RT_NOTHROW_DEF
{
#if ARCH_BITS == 32 || ARCH_BITS == 16
    return ASMAtomicCmpXchgExU32((volatile uint32_t RT_FAR *)(void RT_FAR *)ppv, (uint32_t)pvNew, (uint32_t)pvOld, (uint32_t RT_FAR *)ppvOld);
#elif ARCH_BITS == 64
    return ASMAtomicCmpXchgExU64((volatile uint64_t RT_FAR *)(void RT_FAR *)ppv, (uint64_t)pvNew, (uint64_t)pvOld, (uint64_t RT_FAR *)ppvOld);
#else
# error "ARCH_BITS is bogus"
#endif
}


/**
 * Atomically Compare and Exchange a pointer value, additionally
 * passing back old value, ordered.
 *
 * @returns true if xchg was done.
 * @returns false if xchg wasn't done.
 *
 * @param   ppv         Pointer to the value to update.
 * @param   pvNew       The new value to assigned to *ppv.
 * @param   pvOld       The old value to *ppv compare with.
 * @param   ppvOld      Pointer store the old value at.
 *
 * @remarks This is relatively type safe on GCC platforms.
 * @remarks x86: Requires a 486 or later.
 */
#ifdef __GNUC__
# define ASMAtomicCmpXchgExPtr(ppv, pvNew, pvOld, ppvOld) \
    __extension__ \
    ({\
        __typeof__(*(ppv)) volatile * const ppvTypeChecked    = (ppv); \
        __typeof__(*(ppv))            const pvNewTypeChecked  = (pvNew); \
        __typeof__(*(ppv))            const pvOldTypeChecked  = (pvOld); \
        __typeof__(*(ppv)) *          const ppvOldTypeChecked = (ppvOld); \
        bool fMacroRet = ASMAtomicCmpXchgExPtrVoid((void * volatile *)ppvTypeChecked, \
                                                   (void *)pvNewTypeChecked, (void *)pvOldTypeChecked, \
                                                   (void **)ppvOldTypeChecked); \
        fMacroRet; \
     })
#else
# define ASMAtomicCmpXchgExPtr(ppv, pvNew, pvOld, ppvOld) \
    ASMAtomicCmpXchgExPtrVoid((void RT_FAR * volatile RT_FAR *)(ppv), (void RT_FAR *)(pvNew), (void RT_FAR *)(pvOld), (void RT_FAR * RT_FAR *)(ppvOld))
#endif


/**
 * Virtualization unfriendly serializing instruction, always exits.
 */
#if (RT_INLINE_ASM_EXTERNAL && !RT_INLINE_ASM_USES_INTRIN) || (!defined(RT_ARCH_AMD64) && !defined(RT_ARCH_X86))
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMSerializeInstructionCpuId(void) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMSerializeInstructionCpuId(void) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_GNU_STYLE
    RTCCUINTREG xAX = 0;
#  ifdef RT_ARCH_AMD64
    __asm__ __volatile__ ("cpuid"
                          : "=a" (xAX)
                          : "0" (xAX)
                          : "rbx", "rcx", "rdx", "memory");
#  elif (defined(PIC) || defined(__PIC__)) && defined(__i386__)
    __asm__ __volatile__ ("push  %%ebx\n\t"
                          "cpuid\n\t"
                          "pop   %%ebx\n\t"
                          : "=a" (xAX)
                          : "0" (xAX)
                          : "ecx", "edx", "memory");
#  else
    __asm__ __volatile__ ("cpuid"
                          : "=a" (xAX)
                          : "0" (xAX)
                          : "ebx", "ecx", "edx", "memory");
#  endif

# elif RT_INLINE_ASM_USES_INTRIN
    int aInfo[4];
    _ReadWriteBarrier();
    __cpuid(aInfo, 0);

# else
    __asm
    {
        push    ebx
        xor     eax, eax
        cpuid
        pop     ebx
    }
# endif
}
#endif

/**
 * Virtualization friendly serializing instruction, though more expensive.
 */
#if RT_INLINE_ASM_EXTERNAL || (!defined(RT_ARCH_AMD64) && !defined(RT_ARCH_X86))
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMSerializeInstructionIRet(void) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMSerializeInstructionIRet(void) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_GNU_STYLE
#  ifdef RT_ARCH_AMD64
    __asm__ __volatile__ ("movq  %%rsp,%%r10\n\t"
                          "subq  $128, %%rsp\n\t" /*redzone*/
                          "mov   %%ss, %%eax\n\t"
                          "pushq %%rax\n\t"
                          "pushq %%r10\n\t"
                          "pushfq\n\t"
                          "movl  %%cs, %%eax\n\t"
                          "pushq %%rax\n\t"
                          "leaq  1f(%%rip), %%rax\n\t"
                          "pushq %%rax\n\t"
                          "iretq\n\t"
                          "1:\n\t"
                          ::: "rax", "r10", "memory", "cc");
#  else
    __asm__ __volatile__ ("pushfl\n\t"
                          "pushl %%cs\n\t"
                          "pushl $1f\n\t"
                          "iretl\n\t"
                          "1:\n\t"
                          ::: "memory");
#  endif

# else
    __asm
    {
        pushfd
        push    cs
        push    la_ret
        iretd
    la_ret:
    }
# endif
}
#endif

/**
 * Virtualization friendlier serializing instruction, may still cause exits.
 */
#if (RT_INLINE_ASM_EXTERNAL && RT_INLINE_ASM_USES_INTRIN < RT_MSC_VER_VS2008) || (!defined(RT_ARCH_AMD64) && !defined(RT_ARCH_X86))
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMSerializeInstructionRdTscp(void) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMSerializeInstructionRdTscp(void) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_GNU_STYLE
    /* rdtscp is not supported by ancient linux build VM of course :-( */
#  ifdef RT_ARCH_AMD64
    /*__asm__ __volatile__("rdtscp\n\t" ::: "rax", "rdx, "rcx"); */
    __asm__ __volatile__(".byte 0x0f,0x01,0xf9\n\t" ::: "rax", "rdx", "rcx", "memory");
#  else
    /*__asm__ __volatile__("rdtscp\n\t" ::: "eax", "edx, "ecx"); */
    __asm__ __volatile__(".byte 0x0f,0x01,0xf9\n\t" ::: "eax", "edx", "ecx", "memory");
#  endif
# else
#  if RT_INLINE_ASM_USES_INTRIN >= RT_MSC_VER_VS2008
    uint32_t uIgnore;
    _ReadWriteBarrier();
    (void)__rdtscp(&uIgnore);
    (void)uIgnore;
#  else
    __asm
    {
        rdtscp
    }
#  endif
# endif
}
#endif


/**
 * Serialize Instruction (both data store and instruction flush).
 */
#if (defined(RT_ARCH_X86) && ARCH_BITS == 16) || defined(IN_GUEST)
# define ASMSerializeInstruction() ASMSerializeInstructionIRet()
#elif defined(RT_ARCH_X86) || defined(RT_ARCH_AMD64)
# define ASMSerializeInstruction() ASMSerializeInstructionCpuId()
#elif defined(RT_ARCH_SPARC64)
RTDECL(void) ASMSerializeInstruction(void) RT_NOTHROW_PROTO;
#elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
DECLINLINE(void) ASMSerializeInstruction(void) RT_NOTHROW_DEF
{
    __asm__ __volatile__ (RTASM_ARM_DSB_SY :: RTASM_ARM_DSB_SY_IN_REG :);
}
#else
# error "Port me"
#endif


/**
 * Memory fence, waits for any pending writes and reads to complete.
 * @note No implicit compiler barrier (which is probably stupid).
 */
DECLINLINE(void) ASMMemoryFence(void) RT_NOTHROW_DEF
{
#if defined(RT_ARCH_AMD64) || (defined(RT_ARCH_X86) && !defined(RT_WITH_OLD_CPU_SUPPORT))
# if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__ (".byte 0x0f,0xae,0xf0\n\t");
# elif RT_INLINE_ASM_USES_INTRIN
    _mm_mfence();
# else
    __asm
    {
        _emit   0x0f
        _emit   0xae
        _emit   0xf0
    }
# endif
#elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    __asm__ __volatile__ (RTASM_ARM_DMB_SY :: RTASM_ARM_DMB_SY_IN_REG :);
#elif ARCH_BITS == 16
    uint16_t volatile u16;
    ASMAtomicXchgU16(&u16, 0);
#else
    uint32_t volatile u32;
    ASMAtomicXchgU32(&u32, 0);
#endif
}


/**
 * Write fence, waits for any pending writes to complete.
 * @note No implicit compiler barrier (which is probably stupid).
 */
DECLINLINE(void) ASMWriteFence(void) RT_NOTHROW_DEF
{
#if defined(RT_ARCH_AMD64) || (defined(RT_ARCH_X86) && !defined(RT_WITH_OLD_CPU_SUPPORT))
# if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__ (".byte 0x0f,0xae,0xf8\n\t");
# elif RT_INLINE_ASM_USES_INTRIN
    _mm_sfence();
# else
    __asm
    {
        _emit   0x0f
        _emit   0xae
        _emit   0xf8
    }
# endif
#elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    __asm__ __volatile__ (RTASM_ARM_DMB_ST :: RTASM_ARM_DMB_ST_IN_REG :);
#else
    ASMMemoryFence();
#endif
}


/**
 * Read fence, waits for any pending reads to complete.
 * @note No implicit compiler barrier (which is probably stupid).
 */
DECLINLINE(void) ASMReadFence(void) RT_NOTHROW_DEF
{
#if defined(RT_ARCH_AMD64) || (defined(RT_ARCH_X86) && !defined(RT_WITH_OLD_CPU_SUPPORT))
# if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__ (".byte 0x0f,0xae,0xe8\n\t");
# elif RT_INLINE_ASM_USES_INTRIN
    _mm_lfence();
# else
    __asm
    {
        _emit   0x0f
        _emit   0xae
        _emit   0xe8
    }
# endif
#elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    __asm__ __volatile__ (RTASM_ARM_DMB_LD :: RTASM_ARM_DMB_LD_IN_REG :);
#else
    ASMMemoryFence();
#endif
}


/**
 * Atomically reads an unsigned 8-bit value, ordered.
 *
 * @returns Current *pu8 value
 * @param   pu8    Pointer to the 8-bit variable to read.
 */
DECLINLINE(uint8_t) ASMAtomicReadU8(volatile uint8_t RT_FAR *pu8) RT_NOTHROW_DEF
{
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    uint32_t u32;
    __asm__ __volatile__(".Lstart_ASMAtomicReadU8_%=:\n\t"
                         RTASM_ARM_DMB_SY
# if defined(RT_ARCH_ARM64)
                         "ldxrb     %w[uDst], %[pMem]\n\t"
# else
                         "ldrexb    %[uDst], %[pMem]\n\t"
# endif
                         : [uDst] "=&r" (u32)
                         : [pMem] "Q" (*pu8)
                           RTASM_ARM_DMB_SY_COMMA_IN_REG);
    return (uint8_t)u32;
#else
    ASMMemoryFence();
    return *pu8;    /* byte reads are atomic on x86 */
#endif
}


/**
 * Atomically reads an unsigned 8-bit value, unordered.
 *
 * @returns Current *pu8 value
 * @param   pu8    Pointer to the 8-bit variable to read.
 */
DECLINLINE(uint8_t) ASMAtomicUoReadU8(volatile uint8_t RT_FAR *pu8) RT_NOTHROW_DEF
{
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    uint32_t u32;
    __asm__ __volatile__(".Lstart_ASMAtomicUoReadU8_%=:\n\t"
# if defined(RT_ARCH_ARM64)
                         "ldxrb     %w[uDst], %[pMem]\n\t"
# else
                         "ldrexb    %[uDst], %[pMem]\n\t"
# endif
                         : [uDst] "=&r" (u32)
                         : [pMem] "Q" (*pu8));
    return (uint8_t)u32;
#else
    return *pu8;    /* byte reads are atomic on x86 */
#endif
}


/**
 * Atomically reads a signed 8-bit value, ordered.
 *
 * @returns Current *pi8 value
 * @param   pi8    Pointer to the 8-bit variable to read.
 */
DECLINLINE(int8_t) ASMAtomicReadS8(volatile int8_t RT_FAR *pi8) RT_NOTHROW_DEF
{
    ASMMemoryFence();
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    int32_t i32;
    __asm__ __volatile__(".Lstart_ASMAtomicReadS8_%=:\n\t"
                         RTASM_ARM_DMB_SY
# if defined(RT_ARCH_ARM64)
                         "ldxrb     %w[iDst], %[pMem]\n\t"
# else
                         "ldrexb    %[iDst], %[pMem]\n\t"
# endif
                         : [iDst] "=&r" (i32)
                         : [pMem] "Q" (*pi8)
                           RTASM_ARM_DMB_SY_COMMA_IN_REG);
    return (int8_t)i32;
#else
    return *pi8;    /* byte reads are atomic on x86 */
#endif
}


/**
 * Atomically reads a signed 8-bit value, unordered.
 *
 * @returns Current *pi8 value
 * @param   pi8    Pointer to the 8-bit variable to read.
 */
DECLINLINE(int8_t) ASMAtomicUoReadS8(volatile int8_t RT_FAR *pi8) RT_NOTHROW_DEF
{
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    int32_t i32;
    __asm__ __volatile__(".Lstart_ASMAtomicUoReadS8_%=:\n\t"
# if defined(RT_ARCH_ARM64)
                         "ldxrb     %w[iDst], %[pMem]\n\t"
# else
                         "ldrexb    %[iDst], %[pMem]\n\t"
# endif
                         : [iDst] "=&r" (i32)
                         : [pMem] "Q" (*pi8));
    return (int8_t)i32;
#else
    return *pi8;    /* byte reads are atomic on x86 */
#endif
}


/**
 * Atomically reads an unsigned 16-bit value, ordered.
 *
 * @returns Current *pu16 value
 * @param   pu16    Pointer to the 16-bit variable to read.
 */
DECLINLINE(uint16_t) ASMAtomicReadU16(volatile uint16_t RT_FAR *pu16) RT_NOTHROW_DEF
{
    Assert(!((uintptr_t)pu16 & 1));
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    uint32_t u32;
    __asm__ __volatile__(".Lstart_ASMAtomicReadU16_%=:\n\t"
                         RTASM_ARM_DMB_SY
# if defined(RT_ARCH_ARM64)
                         "ldxrh     %w[uDst], %[pMem]\n\t"
# else
                         "ldrexh    %[uDst], %[pMem]\n\t"
# endif
                         : [uDst] "=&r" (u32)
                         : [pMem] "Q" (*pu16)
                           RTASM_ARM_DMB_SY_COMMA_IN_REG);
    return (uint16_t)u32;
#else
    ASMMemoryFence();
    return *pu16;
#endif
}


/**
 * Atomically reads an unsigned 16-bit value, unordered.
 *
 * @returns Current *pu16 value
 * @param   pu16    Pointer to the 16-bit variable to read.
 */
DECLINLINE(uint16_t) ASMAtomicUoReadU16(volatile uint16_t RT_FAR *pu16) RT_NOTHROW_DEF
{
    Assert(!((uintptr_t)pu16 & 1));
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    uint32_t u32;
    __asm__ __volatile__(".Lstart_ASMAtomicUoReadU16_%=:\n\t"
# if defined(RT_ARCH_ARM64)
                         "ldxrh     %w[uDst], %[pMem]\n\t"
# else
                         "ldrexh    %[uDst], %[pMem]\n\t"
# endif
                         : [uDst] "=&r" (u32)
                         : [pMem] "Q" (*pu16));
    return (uint16_t)u32;
#else
    return *pu16;
#endif
}


/**
 * Atomically reads a signed 16-bit value, ordered.
 *
 * @returns Current *pi16 value
 * @param   pi16    Pointer to the 16-bit variable to read.
 */
DECLINLINE(int16_t) ASMAtomicReadS16(volatile int16_t RT_FAR *pi16) RT_NOTHROW_DEF
{
    Assert(!((uintptr_t)pi16 & 1));
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    int32_t i32;
    __asm__ __volatile__(".Lstart_ASMAtomicReadS16_%=:\n\t"
                         RTASM_ARM_DMB_SY
# if defined(RT_ARCH_ARM64)
                         "ldxrh     %w[iDst], %[pMem]\n\t"
# else
                         "ldrexh    %[iDst], %[pMem]\n\t"
# endif
                         : [iDst] "=&r" (i32)
                         : [pMem] "Q" (*pi16)
                           RTASM_ARM_DMB_SY_COMMA_IN_REG);
    return (int16_t)i32;
#else
    ASMMemoryFence();
    return *pi16;
#endif
}


/**
 * Atomically reads a signed 16-bit value, unordered.
 *
 * @returns Current *pi16 value
 * @param   pi16    Pointer to the 16-bit variable to read.
 */
DECLINLINE(int16_t) ASMAtomicUoReadS16(volatile int16_t RT_FAR *pi16) RT_NOTHROW_DEF
{
    Assert(!((uintptr_t)pi16 & 1));
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    int32_t i32;
    __asm__ __volatile__(".Lstart_ASMAtomicUoReadS16_%=:\n\t"
# if defined(RT_ARCH_ARM64)
                         "ldxrh     %w[iDst], %[pMem]\n\t"
# else
                         "ldrexh    %[iDst], %[pMem]\n\t"
# endif
                         : [iDst] "=&r" (i32)
                         : [pMem] "Q" (*pi16));
    return (int16_t)i32;
#else
    return *pi16;
#endif
}


/**
 * Atomically reads an unsigned 32-bit value, ordered.
 *
 * @returns Current *pu32 value
 * @param   pu32    Pointer to the 32-bit variable to read.
 */
DECLINLINE(uint32_t) ASMAtomicReadU32(volatile uint32_t RT_FAR *pu32) RT_NOTHROW_DEF
{
    Assert(!((uintptr_t)pu32 & 3));
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    uint32_t u32;
    __asm__ __volatile__(".Lstart_ASMAtomicReadU32_%=:\n\t"
                         RTASM_ARM_DMB_SY
# if defined(RT_ARCH_ARM64)
                         "ldxr      %w[uDst], %[pMem]\n\t"
# else
                         "ldrex    %[uDst], %[pMem]\n\t"
# endif
                         : [uDst] "=&r" (u32)
                         : [pMem] "Q" (*pu32)
                           RTASM_ARM_DMB_SY_COMMA_IN_REG);
    return u32;
#else
    ASMMemoryFence();
# if ARCH_BITS == 16
    AssertFailed();  /** @todo 16-bit */
# endif
    return *pu32;
#endif
}


/**
 * Atomically reads an unsigned 32-bit value, unordered.
 *
 * @returns Current *pu32 value
 * @param   pu32    Pointer to the 32-bit variable to read.
 */
DECLINLINE(uint32_t) ASMAtomicUoReadU32(volatile uint32_t RT_FAR *pu32) RT_NOTHROW_DEF
{
    Assert(!((uintptr_t)pu32 & 3));
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    uint32_t u32;
    __asm__ __volatile__(".Lstart_ASMAtomicUoReadU32_%=:\n\t"
# if defined(RT_ARCH_ARM64)
                         "ldxr      %w[uDst], %[pMem]\n\t"
# else
                         "ldrex    %[uDst], %[pMem]\n\t"
# endif
                         : [uDst] "=&r" (u32)
                         : [pMem] "Q" (*pu32));
    return u32;
#else
# if ARCH_BITS == 16
    AssertFailed();  /** @todo 16-bit */
# endif
    return *pu32;
#endif
}


/**
 * Atomically reads a signed 32-bit value, ordered.
 *
 * @returns Current *pi32 value
 * @param   pi32    Pointer to the 32-bit variable to read.
 */
DECLINLINE(int32_t) ASMAtomicReadS32(volatile int32_t RT_FAR *pi32) RT_NOTHROW_DEF
{
    Assert(!((uintptr_t)pi32 & 3));
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    int32_t i32;
    __asm__ __volatile__(".Lstart_ASMAtomicReadS32_%=:\n\t"
                         RTASM_ARM_DMB_SY
# if defined(RT_ARCH_ARM64)
                         "ldxr      %w[iDst], %[pMem]\n\t"
# else
                         "ldrex    %[iDst], %[pMem]\n\t"
# endif
                         : [iDst] "=&r" (i32)
                         : [pMem] "Q" (*pi32)
                           RTASM_ARM_DMB_SY_COMMA_IN_REG);
    return i32;
#else
    ASMMemoryFence();
# if ARCH_BITS == 16
    AssertFailed();  /** @todo 16-bit */
# endif
    return *pi32;
#endif
}


/**
 * Atomically reads a signed 32-bit value, unordered.
 *
 * @returns Current *pi32 value
 * @param   pi32    Pointer to the 32-bit variable to read.
 */
DECLINLINE(int32_t) ASMAtomicUoReadS32(volatile int32_t RT_FAR *pi32) RT_NOTHROW_DEF
{
    Assert(!((uintptr_t)pi32 & 3));
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    int32_t i32;
    __asm__ __volatile__(".Lstart_ASMAtomicUoReadS32_%=:\n\t"
# if defined(RT_ARCH_ARM64)
                         "ldxr      %w[iDst], %[pMem]\n\t"
# else
                         "ldrex    %[iDst], %[pMem]\n\t"
# endif
                         : [iDst] "=&r" (i32)
                         : [pMem] "Q" (*pi32));
    return i32;

#else
# if ARCH_BITS == 16
    AssertFailed();  /** @todo 16-bit */
# endif
    return *pi32;
#endif
}


/**
 * Atomically reads an unsigned 64-bit value, ordered.
 *
 * @returns Current *pu64 value
 * @param   pu64    Pointer to the 64-bit variable to read.
 *                  The memory pointed to must be writable.
 *
 * @remarks This may fault if the memory is read-only!
 * @remarks x86: Requires a Pentium or later.
 */
#if (RT_INLINE_ASM_EXTERNAL_TMP_ARM && !defined(RT_ARCH_AMD64)) \
 || RT_INLINE_DONT_MIX_CMPXCHG8B_AND_PIC
RT_ASM_DECL_PRAGMA_WATCOM(uint64_t) ASMAtomicReadU64(volatile uint64_t RT_FAR *pu64) RT_NOTHROW_PROTO;
#else
DECLINLINE(uint64_t) ASMAtomicReadU64(volatile uint64_t RT_FAR *pu64) RT_NOTHROW_DEF
{
    uint64_t u64;
# ifdef RT_ARCH_AMD64
    Assert(!((uintptr_t)pu64 & 7));
/*#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__(  "mfence\n\t"
                           "movq %1, %0\n\t"
                         : "=r" (u64)
                         : "m" (*pu64));
#  else
    __asm
    {
        mfence
        mov     rdx, [pu64]
        mov     rax, [rdx]
        mov     [u64], rax
    }
#  endif*/
    ASMMemoryFence();
    u64 = *pu64;

# elif defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
#   if defined(PIC) || defined(__PIC__)
    uint32_t u32EBX = 0;
    Assert(!((uintptr_t)pu64 & 7));
    __asm__ __volatile__("xchgl %%ebx, %3\n\t"
                         "lock; cmpxchg8b (%5)\n\t"
                         "movl %3, %%ebx\n\t"
                         : "=A" (u64)
#    if RT_GNUC_PREREQ(4, 3)
                         , "+m" (*pu64)
#    else
                         , "=m" (*pu64)
#    endif
                         : "0" (0ULL)
                         , "m" (u32EBX)
                         , "c" (0)
                         , "S" (pu64)
                         : "cc");
#   else /* !PIC */
    __asm__ __volatile__("lock; cmpxchg8b %1\n\t"
                         : "=A" (u64)
                         , "+m" (*pu64)
                         : "0" (0ULL)
                         , "b" (0)
                         , "c" (0)
                         : "cc");
#   endif
#  else
    Assert(!((uintptr_t)pu64 & 7));
    __asm
    {
        xor     eax, eax
        xor     edx, edx
        mov     edi, pu64
        xor     ecx, ecx
        xor     ebx, ebx
        lock cmpxchg8b [edi]
        mov     dword ptr [u64], eax
        mov     dword ptr [u64 + 4], edx
    }
#  endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    Assert(!((uintptr_t)pu64 & 7));
    __asm__ __volatile__(".Lstart_ASMAtomicReadU64_%=:\n\t"
                         RTASM_ARM_DMB_SY
# if defined(RT_ARCH_ARM64)
                         "ldxr      %[uDst], %[pMem]\n\t"
# else
                         "ldrexd    %[uDst], %H[uDst], %[pMem]\n\t"
# endif
                         : [uDst] "=&r" (u64)
                         : [pMem] "Q" (*pu64)
                           RTASM_ARM_DMB_SY_COMMA_IN_REG);

# else
#  error "Port me"
# endif
    return u64;
}
#endif


/**
 * Atomically reads an unsigned 64-bit value, unordered.
 *
 * @returns Current *pu64 value
 * @param   pu64    Pointer to the 64-bit variable to read.
 *                  The memory pointed to must be writable.
 *
 * @remarks This may fault if the memory is read-only!
 * @remarks x86: Requires a Pentium or later.
 */
#if !defined(RT_ARCH_AMD64) \
  && (   (RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN) \
      || RT_INLINE_DONT_MIX_CMPXCHG8B_AND_PIC)
RT_ASM_DECL_PRAGMA_WATCOM(uint64_t) ASMAtomicUoReadU64(volatile uint64_t RT_FAR *pu64) RT_NOTHROW_PROTO;
#else
DECLINLINE(uint64_t) ASMAtomicUoReadU64(volatile uint64_t RT_FAR *pu64) RT_NOTHROW_DEF
{
    uint64_t u64;
# ifdef RT_ARCH_AMD64
    Assert(!((uintptr_t)pu64 & 7));
/*#  if RT_INLINE_ASM_GNU_STYLE
    Assert(!((uintptr_t)pu64 & 7));
    __asm__ __volatile__("movq %1, %0\n\t"
                         : "=r" (u64)
                         : "m" (*pu64));
#  else
    __asm
    {
        mov     rdx, [pu64]
        mov     rax, [rdx]
        mov     [u64], rax
    }
#  endif */
    u64 = *pu64;

# elif defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
#   if defined(PIC) || defined(__PIC__)
    uint32_t u32EBX = 0;
    uint32_t u32Spill;
    Assert(!((uintptr_t)pu64 & 7));
    __asm__ __volatile__("xor   %%eax,%%eax\n\t"
                         "xor   %%ecx,%%ecx\n\t"
                         "xor   %%edx,%%edx\n\t"
                         "xchgl %%ebx, %3\n\t"
                         "lock; cmpxchg8b (%4)\n\t"
                         "movl %3, %%ebx\n\t"
                         : "=A" (u64)
#    if RT_GNUC_PREREQ(4, 3)
                         , "+m" (*pu64)
#    else
                         , "=m" (*pu64)
#    endif
                         , "=c" (u32Spill)
                         : "m" (u32EBX)
                         , "S" (pu64)
                         : "cc");
#   else /* !PIC */
    __asm__ __volatile__("lock; cmpxchg8b %1\n\t"
                         : "=A" (u64)
                         , "+m" (*pu64)
                         : "0" (0ULL)
                         , "b" (0)
                         , "c" (0)
                         : "cc");
#   endif
#  else
    Assert(!((uintptr_t)pu64 & 7));
    __asm
    {
        xor     eax, eax
        xor     edx, edx
        mov     edi, pu64
        xor     ecx, ecx
        xor     ebx, ebx
        lock cmpxchg8b [edi]
        mov     dword ptr [u64], eax
        mov     dword ptr [u64 + 4], edx
    }
#  endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    Assert(!((uintptr_t)pu64 & 7));
    __asm__ __volatile__(".Lstart_ASMAtomicUoReadU64_%=:\n\t"
# if defined(RT_ARCH_ARM64)
                         "ldxr      %[uDst], %[pMem]\n\t"
# else
                         "ldrexd    %[uDst], %H[uDst], %[pMem]\n\t"
# endif
                         : [uDst] "=&r" (u64)
                         : [pMem] "Q" (*pu64));

# else
#  error "Port me"
# endif
    return u64;
}
#endif


/**
 * Atomically reads a signed 64-bit value, ordered.
 *
 * @returns Current *pi64 value
 * @param   pi64    Pointer to the 64-bit variable to read.
 *                  The memory pointed to must be writable.
 *
 * @remarks This may fault if the memory is read-only!
 * @remarks x86: Requires a Pentium or later.
 */
DECLINLINE(int64_t) ASMAtomicReadS64(volatile int64_t RT_FAR *pi64) RT_NOTHROW_DEF
{
    return (int64_t)ASMAtomicReadU64((volatile uint64_t RT_FAR *)pi64);
}


/**
 * Atomically reads a signed 64-bit value, unordered.
 *
 * @returns Current *pi64 value
 * @param   pi64    Pointer to the 64-bit variable to read.
 *                  The memory pointed to must be writable.
 *
 * @remarks This will fault if the memory is read-only!
 * @remarks x86: Requires a Pentium or later.
 */
DECLINLINE(int64_t) ASMAtomicUoReadS64(volatile int64_t RT_FAR *pi64) RT_NOTHROW_DEF
{
    return (int64_t)ASMAtomicUoReadU64((volatile uint64_t RT_FAR *)pi64);
}


/**
 * Atomically reads a size_t value, ordered.
 *
 * @returns Current *pcb value
 * @param   pcb     Pointer to the size_t variable to read.
 */
DECLINLINE(size_t) ASMAtomicReadZ(size_t volatile RT_FAR *pcb) RT_NOTHROW_DEF
{
#if ARCH_BITS == 64
    return ASMAtomicReadU64((uint64_t volatile RT_FAR *)pcb);
#elif ARCH_BITS == 32
    return ASMAtomicReadU32((uint32_t volatile RT_FAR *)pcb);
#elif ARCH_BITS == 16
    AssertCompileSize(size_t, 2);
    return ASMAtomicReadU16((uint16_t volatile RT_FAR *)pcb);
#else
# error "Unsupported ARCH_BITS value"
#endif
}


/**
 * Atomically reads a size_t value, unordered.
 *
 * @returns Current *pcb value
 * @param   pcb     Pointer to the size_t variable to read.
 */
DECLINLINE(size_t) ASMAtomicUoReadZ(size_t volatile RT_FAR *pcb) RT_NOTHROW_DEF
{
#if ARCH_BITS == 64 || ARCH_BITS == 16
    return ASMAtomicUoReadU64((uint64_t volatile RT_FAR *)pcb);
#elif ARCH_BITS == 32
    return ASMAtomicUoReadU32((uint32_t volatile RT_FAR *)pcb);
#elif ARCH_BITS == 16
    AssertCompileSize(size_t, 2);
    return ASMAtomicUoReadU16((uint16_t volatile RT_FAR *)pcb);
#else
# error "Unsupported ARCH_BITS value"
#endif
}


/**
 * Atomically reads a pointer value, ordered.
 *
 * @returns Current *pv value
 * @param   ppv     Pointer to the pointer variable to read.
 *
 * @remarks Please use ASMAtomicReadPtrT, it provides better type safety and
 *          requires less typing (no casts).
 */
DECLINLINE(void RT_FAR *) ASMAtomicReadPtr(void RT_FAR * volatile RT_FAR *ppv) RT_NOTHROW_DEF
{
#if ARCH_BITS == 32 || ARCH_BITS == 16
    return (void RT_FAR *)ASMAtomicReadU32((volatile uint32_t RT_FAR *)(void RT_FAR *)ppv);
#elif ARCH_BITS == 64
    return (void RT_FAR *)ASMAtomicReadU64((volatile uint64_t RT_FAR *)(void RT_FAR *)ppv);
#else
# error "ARCH_BITS is bogus"
#endif
}

/**
 * Convenience macro for avoiding the annoying casting with ASMAtomicReadPtr.
 *
 * @returns Current *pv value
 * @param   ppv     Pointer to the pointer variable to read.
 * @param   Type    The type of *ppv, sans volatile.
 */
#ifdef __GNUC__ /* 8.2.0 requires -Wno-ignored-qualifiers */
# define ASMAtomicReadPtrT(ppv, Type) \
    __extension__ \
    ({\
        __typeof__(*(ppv)) volatile *ppvTypeChecked = (ppv); \
        Type pvTypeChecked = (__typeof__(*(ppv))) ASMAtomicReadPtr((void * volatile *)ppvTypeChecked); \
        pvTypeChecked; \
     })
#else
# define ASMAtomicReadPtrT(ppv, Type) \
    (Type)ASMAtomicReadPtr((void RT_FAR * volatile RT_FAR *)(ppv))
#endif


/**
 * Atomically reads a pointer value, unordered.
 *
 * @returns Current *pv value
 * @param   ppv     Pointer to the pointer variable to read.
 *
 * @remarks Please use ASMAtomicUoReadPtrT, it provides better type safety and
 *          requires less typing (no casts).
 */
DECLINLINE(void RT_FAR *) ASMAtomicUoReadPtr(void RT_FAR * volatile RT_FAR *ppv) RT_NOTHROW_DEF
{
#if ARCH_BITS == 32 || ARCH_BITS == 16
    return (void RT_FAR *)ASMAtomicUoReadU32((volatile uint32_t RT_FAR *)(void RT_FAR *)ppv);
#elif ARCH_BITS == 64
    return (void RT_FAR *)ASMAtomicUoReadU64((volatile uint64_t RT_FAR *)(void RT_FAR *)ppv);
#else
# error "ARCH_BITS is bogus"
#endif
}


/**
 * Convenience macro for avoiding the annoying casting with ASMAtomicUoReadPtr.
 *
 * @returns Current *pv value
 * @param   ppv     Pointer to the pointer variable to read.
 * @param   Type    The type of *ppv, sans volatile.
 */
#ifdef __GNUC__ /* 8.2.0 requires -Wno-ignored-qualifiers */
# define ASMAtomicUoReadPtrT(ppv, Type) \
    __extension__ \
    ({\
        __typeof__(*(ppv)) volatile * const ppvTypeChecked = (ppv); \
        Type pvTypeChecked = (__typeof__(*(ppv))) ASMAtomicUoReadPtr((void * volatile *)ppvTypeChecked); \
        pvTypeChecked; \
     })
#else
# define ASMAtomicUoReadPtrT(ppv, Type) \
    (Type)ASMAtomicUoReadPtr((void RT_FAR * volatile RT_FAR *)(ppv))
#endif


/**
 * Atomically reads a boolean value, ordered.
 *
 * @returns Current *pf value
 * @param   pf      Pointer to the boolean variable to read.
 */
DECLINLINE(bool) ASMAtomicReadBool(volatile bool RT_FAR *pf) RT_NOTHROW_DEF
{
    ASMMemoryFence();
    return *pf;     /* byte reads are atomic on x86 */
}


/**
 * Atomically reads a boolean value, unordered.
 *
 * @returns Current *pf value
 * @param   pf      Pointer to the boolean variable to read.
 */
DECLINLINE(bool) ASMAtomicUoReadBool(volatile bool RT_FAR *pf) RT_NOTHROW_DEF
{
    return *pf;     /* byte reads are atomic on x86 */
}


/**
 * Atomically read a typical IPRT handle value, ordered.
 *
 * @param   ph      Pointer to the handle variable to read.
 * @param   phRes   Where to store the result.
 *
 * @remarks This doesn't currently work for all handles (like RTFILE).
 */
#if HC_ARCH_BITS == 32 || ARCH_BITS == 16
# define ASMAtomicReadHandle(ph, phRes) \
    do { \
        AssertCompile(sizeof(*(ph))    == sizeof(uint32_t)); \
        AssertCompile(sizeof(*(phRes)) == sizeof(uint32_t)); \
        *(uint32_t RT_FAR *)(phRes) = ASMAtomicReadU32((uint32_t volatile RT_FAR *)(ph)); \
    } while (0)
#elif HC_ARCH_BITS == 64
# define ASMAtomicReadHandle(ph, phRes) \
    do { \
        AssertCompile(sizeof(*(ph))    == sizeof(uint64_t)); \
        AssertCompile(sizeof(*(phRes)) == sizeof(uint64_t)); \
        *(uint64_t RT_FAR *)(phRes) = ASMAtomicReadU64((uint64_t volatile RT_FAR *)(ph)); \
    } while (0)
#else
# error HC_ARCH_BITS
#endif


/**
 * Atomically read a typical IPRT handle value, unordered.
 *
 * @param   ph      Pointer to the handle variable to read.
 * @param   phRes   Where to store the result.
 *
 * @remarks This doesn't currently work for all handles (like RTFILE).
 */
#if HC_ARCH_BITS == 32 || ARCH_BITS == 16
# define ASMAtomicUoReadHandle(ph, phRes) \
    do { \
        AssertCompile(sizeof(*(ph))    == sizeof(uint32_t)); \
        AssertCompile(sizeof(*(phRes)) == sizeof(uint32_t)); \
        *(uint32_t RT_FAR *)(phRes) = ASMAtomicUoReadU32((uint32_t volatile RT_FAR *)(ph)); \
    } while (0)
#elif HC_ARCH_BITS == 64
# define ASMAtomicUoReadHandle(ph, phRes) \
    do { \
        AssertCompile(sizeof(*(ph))    == sizeof(uint64_t)); \
        AssertCompile(sizeof(*(phRes)) == sizeof(uint64_t)); \
        *(uint64_t RT_FAR *)(phRes) = ASMAtomicUoReadU64((uint64_t volatile RT_FAR *)(ph)); \
    } while (0)
#else
# error HC_ARCH_BITS
#endif


/**
 * Atomically read a value which size might differ
 * between platforms or compilers, ordered.
 *
 * @param   pu      Pointer to the variable to read.
 * @param   puRes   Where to store the result.
 */
#define ASMAtomicReadSize(pu, puRes) \
    do { \
        switch (sizeof(*(pu))) { \
            case 1: *(uint8_t  RT_FAR *)(puRes) = ASMAtomicReadU8( (volatile uint8_t  RT_FAR *)(void RT_FAR *)(pu)); break; \
            case 2: *(uint16_t RT_FAR *)(puRes) = ASMAtomicReadU16((volatile uint16_t RT_FAR *)(void RT_FAR *)(pu)); break; \
            case 4: *(uint32_t RT_FAR *)(puRes) = ASMAtomicReadU32((volatile uint32_t RT_FAR *)(void RT_FAR *)(pu)); break; \
            case 8: *(uint64_t RT_FAR *)(puRes) = ASMAtomicReadU64((volatile uint64_t RT_FAR *)(void RT_FAR *)(pu)); break; \
            default: AssertMsgFailed(("ASMAtomicReadSize: size %d is not supported\n", sizeof(*(pu)))); \
        } \
    } while (0)


/**
 * Atomically read a value which size might differ
 * between platforms or compilers, unordered.
 *
 * @param   pu      Pointer to the variable to read.
 * @param   puRes   Where to store the result.
 */
#define ASMAtomicUoReadSize(pu, puRes) \
    do { \
        switch (sizeof(*(pu))) { \
            case 1: *(uint8_t  RT_FAR *)(puRes) = ASMAtomicUoReadU8( (volatile uint8_t  RT_FAR *)(void RT_FAR *)(pu)); break; \
            case 2: *(uint16_t RT_FAR *)(puRes) = ASMAtomicUoReadU16((volatile uint16_t RT_FAR *)(void RT_FAR *)(pu)); break; \
            case 4: *(uint32_t RT_FAR *)(puRes) = ASMAtomicUoReadU32((volatile uint32_t RT_FAR *)(void RT_FAR *)(pu)); break; \
            case 8: *(uint64_t RT_FAR *)(puRes) = ASMAtomicUoReadU64((volatile uint64_t RT_FAR *)(void RT_FAR *)(pu)); break; \
            default: AssertMsgFailed(("ASMAtomicReadSize: size %d is not supported\n", sizeof(*(pu)))); \
        } \
    } while (0)


/**
 * Atomically writes an unsigned 8-bit value, ordered.
 *
 * @param   pu8     Pointer to the 8-bit variable.
 * @param   u8      The 8-bit value to assign to *pu8.
 */
DECLINLINE(void) ASMAtomicWriteU8(volatile uint8_t RT_FAR *pu8, uint8_t u8) RT_NOTHROW_DEF
{
    /** @todo Any possible ARM32/ARM64 optimizations here? */
    ASMAtomicXchgU8(pu8, u8);
}


/**
 * Atomically writes an unsigned 8-bit value, unordered.
 *
 * @param   pu8     Pointer to the 8-bit variable.
 * @param   u8      The 8-bit value to assign to *pu8.
 */
DECLINLINE(void) ASMAtomicUoWriteU8(volatile uint8_t RT_FAR *pu8, uint8_t u8) RT_NOTHROW_DEF
{
    /** @todo Any possible ARM32/ARM64 improvements here? */
    *pu8 = u8;      /* byte writes are atomic on x86 */
}


/**
 * Atomically writes a signed 8-bit value, ordered.
 *
 * @param   pi8     Pointer to the 8-bit variable to read.
 * @param   i8      The 8-bit value to assign to *pi8.
 */
DECLINLINE(void) ASMAtomicWriteS8(volatile int8_t RT_FAR *pi8, int8_t i8) RT_NOTHROW_DEF
{
    /** @todo Any possible ARM32/ARM64 optimizations here? */
    ASMAtomicXchgS8(pi8, i8);
}


/**
 * Atomically writes a signed 8-bit value, unordered.
 *
 * @param   pi8     Pointer to the 8-bit variable to write.
 * @param   i8      The 8-bit value to assign to *pi8.
 */
DECLINLINE(void) ASMAtomicUoWriteS8(volatile int8_t RT_FAR *pi8, int8_t i8) RT_NOTHROW_DEF
{
    *pi8 = i8;      /* byte writes are atomic on x86 */
}


/**
 * Atomically writes an unsigned 16-bit value, ordered.
 *
 * @param   pu16    Pointer to the 16-bit variable to write.
 * @param   u16     The 16-bit value to assign to *pu16.
 */
DECLINLINE(void) ASMAtomicWriteU16(volatile uint16_t RT_FAR *pu16, uint16_t u16) RT_NOTHROW_DEF
{
    /** @todo Any possible ARM32/ARM64 optimizations here? */
    ASMAtomicXchgU16(pu16, u16);
}


/**
 * Atomically writes an unsigned 16-bit value, unordered.
 *
 * @param   pu16    Pointer to the 16-bit variable to write.
 * @param   u16     The 16-bit value to assign to *pu16.
 */
DECLINLINE(void) ASMAtomicUoWriteU16(volatile uint16_t RT_FAR *pu16, uint16_t u16) RT_NOTHROW_DEF
{
    Assert(!((uintptr_t)pu16 & 1));
    *pu16 = u16;
}


/**
 * Atomically writes a signed 16-bit value, ordered.
 *
 * @param   pi16    Pointer to the 16-bit variable to write.
 * @param   i16     The 16-bit value to assign to *pi16.
 */
DECLINLINE(void) ASMAtomicWriteS16(volatile int16_t RT_FAR *pi16, int16_t i16) RT_NOTHROW_DEF
{
    /** @todo Any possible ARM32/ARM64 optimizations here? */
    ASMAtomicXchgS16(pi16, i16);
}


/**
 * Atomically writes a signed 16-bit value, unordered.
 *
 * @param   pi16    Pointer to the 16-bit variable to write.
 * @param   i16     The 16-bit value to assign to *pi16.
 */
DECLINLINE(void) ASMAtomicUoWriteS16(volatile int16_t RT_FAR *pi16, int16_t i16) RT_NOTHROW_DEF
{
    Assert(!((uintptr_t)pi16 & 1));
    *pi16 = i16;
}


/**
 * Atomically writes an unsigned 32-bit value, ordered.
 *
 * @param   pu32    Pointer to the 32-bit variable to write.
 * @param   u32     The 32-bit value to assign to *pu32.
 */
DECLINLINE(void) ASMAtomicWriteU32(volatile uint32_t RT_FAR *pu32, uint32_t u32) RT_NOTHROW_DEF
{
    /** @todo Any possible ARM32/ARM64 optimizations here? */
    ASMAtomicXchgU32(pu32, u32);
}


/**
 * Atomically writes an unsigned 32-bit value, unordered.
 *
 * @param   pu32    Pointer to the 32-bit variable to write.
 * @param   u32     The 32-bit value to assign to *pu32.
 */
DECLINLINE(void) ASMAtomicUoWriteU32(volatile uint32_t RT_FAR *pu32, uint32_t u32) RT_NOTHROW_DEF
{
    Assert(!((uintptr_t)pu32 & 3));
#if ARCH_BITS >= 32
    *pu32 = u32;
#else
    ASMAtomicXchgU32(pu32, u32);
#endif
}


/**
 * Atomically writes a signed 32-bit value, ordered.
 *
 * @param   pi32    Pointer to the 32-bit variable to write.
 * @param   i32     The 32-bit value to assign to *pi32.
 */
DECLINLINE(void) ASMAtomicWriteS32(volatile int32_t RT_FAR *pi32, int32_t i32) RT_NOTHROW_DEF
{
    ASMAtomicXchgS32(pi32, i32);
}


/**
 * Atomically writes a signed 32-bit value, unordered.
 *
 * @param   pi32    Pointer to the 32-bit variable to write.
 * @param   i32     The 32-bit value to assign to *pi32.
 */
DECLINLINE(void) ASMAtomicUoWriteS32(volatile int32_t RT_FAR *pi32, int32_t i32) RT_NOTHROW_DEF
{
    Assert(!((uintptr_t)pi32 & 3));
#if ARCH_BITS >= 32
    *pi32 = i32;
#else
    ASMAtomicXchgS32(pi32, i32);
#endif
}


/**
 * Atomically writes an unsigned 64-bit value, ordered.
 *
 * @param   pu64    Pointer to the 64-bit variable to write.
 * @param   u64     The 64-bit value to assign to *pu64.
 */
DECLINLINE(void) ASMAtomicWriteU64(volatile uint64_t RT_FAR *pu64, uint64_t u64) RT_NOTHROW_DEF
{
    /** @todo Any possible ARM32/ARM64 optimizations here? */
    ASMAtomicXchgU64(pu64, u64);
}


/**
 * Atomically writes an unsigned 64-bit value, unordered.
 *
 * @param   pu64    Pointer to the 64-bit variable to write.
 * @param   u64     The 64-bit value to assign to *pu64.
 */
DECLINLINE(void) ASMAtomicUoWriteU64(volatile uint64_t RT_FAR *pu64, uint64_t u64) RT_NOTHROW_DEF
{
    Assert(!((uintptr_t)pu64 & 7));
#if ARCH_BITS == 64
    *pu64 = u64;
#else
    ASMAtomicXchgU64(pu64, u64);
#endif
}


/**
 * Atomically writes a signed 64-bit value, ordered.
 *
 * @param   pi64    Pointer to the 64-bit variable to write.
 * @param   i64     The 64-bit value to assign to *pi64.
 */
DECLINLINE(void) ASMAtomicWriteS64(volatile int64_t RT_FAR *pi64, int64_t i64) RT_NOTHROW_DEF
{
    /** @todo Any possible ARM32/ARM64 optimizations here? */
    ASMAtomicXchgS64(pi64, i64);
}


/**
 * Atomically writes a signed 64-bit value, unordered.
 *
 * @param   pi64    Pointer to the 64-bit variable to write.
 * @param   i64     The 64-bit value to assign to *pi64.
 */
DECLINLINE(void) ASMAtomicUoWriteS64(volatile int64_t RT_FAR *pi64, int64_t i64) RT_NOTHROW_DEF
{
    Assert(!((uintptr_t)pi64 & 7));
#if ARCH_BITS == 64
    *pi64 = i64;
#else
    ASMAtomicXchgS64(pi64, i64);
#endif
}


/**
 * Atomically writes a size_t value, ordered.
 *
 * @param   pcb     Pointer to the size_t variable to write.
 * @param   cb      The value to assign to *pcb.
 */
DECLINLINE(void) ASMAtomicWriteZ(volatile size_t RT_FAR *pcb, size_t cb) RT_NOTHROW_DEF
{
#if ARCH_BITS == 64
    ASMAtomicWriteU64((uint64_t volatile *)pcb, cb);
#elif ARCH_BITS == 32
    ASMAtomicWriteU32((uint32_t volatile *)pcb, cb);
#elif ARCH_BITS == 16
    AssertCompileSize(size_t, 2);
    ASMAtomicWriteU16((uint16_t volatile *)pcb, cb);
#else
# error "Unsupported ARCH_BITS value"
#endif
}


/**
 * Atomically writes a size_t value, unordered.
 *
 * @param   pcb     Pointer to the size_t variable to write.
 * @param   cb      The value to assign to *pcb.
 */
DECLINLINE(void) ASMAtomicUoWriteZ(volatile size_t RT_FAR *pcb, size_t cb) RT_NOTHROW_DEF
{
#if ARCH_BITS == 64
    ASMAtomicUoWriteU64((uint64_t volatile *)pcb, cb);
#elif ARCH_BITS == 32
    ASMAtomicUoWriteU32((uint32_t volatile *)pcb, cb);
#elif ARCH_BITS == 16
    AssertCompileSize(size_t, 2);
    ASMAtomicUoWriteU16((uint16_t volatile *)pcb, cb);
#else
# error "Unsupported ARCH_BITS value"
#endif
}


/**
 * Atomically writes a boolean value, unordered.
 *
 * @param   pf      Pointer to the boolean variable to write.
 * @param   f       The boolean value to assign to *pf.
 */
DECLINLINE(void) ASMAtomicWriteBool(volatile bool RT_FAR *pf, bool f) RT_NOTHROW_DEF
{
    ASMAtomicWriteU8((uint8_t volatile RT_FAR *)pf, f);
}


/**
 * Atomically writes a boolean value, unordered.
 *
 * @param   pf      Pointer to the boolean variable to write.
 * @param   f       The boolean value to assign to *pf.
 */
DECLINLINE(void) ASMAtomicUoWriteBool(volatile bool RT_FAR *pf, bool f) RT_NOTHROW_DEF
{
    *pf = f;    /* byte writes are atomic on x86 */
}


/**
 * Atomically writes a pointer value, ordered.
 *
 * @param   ppv     Pointer to the pointer variable to write.
 * @param   pv      The pointer value to assign to *ppv.
 */
DECLINLINE(void) ASMAtomicWritePtrVoid(void RT_FAR * volatile RT_FAR *ppv, const void *pv) RT_NOTHROW_DEF
{
#if ARCH_BITS == 32 || ARCH_BITS == 16
    ASMAtomicWriteU32((volatile uint32_t RT_FAR *)(void RT_FAR *)ppv, (uint32_t)pv);
#elif ARCH_BITS == 64
    ASMAtomicWriteU64((volatile uint64_t RT_FAR *)(void RT_FAR *)ppv, (uint64_t)pv);
#else
# error "ARCH_BITS is bogus"
#endif
}


/**
 * Atomically writes a pointer value, unordered.
 *
 * @param   ppv     Pointer to the pointer variable to write.
 * @param   pv      The pointer value to assign to *ppv.
 */
DECLINLINE(void) ASMAtomicUoWritePtrVoid(void RT_FAR * volatile RT_FAR *ppv, const void *pv) RT_NOTHROW_DEF
{
#if ARCH_BITS == 32 || ARCH_BITS == 16
    ASMAtomicUoWriteU32((volatile uint32_t RT_FAR *)(void RT_FAR *)ppv, (uint32_t)pv);
#elif ARCH_BITS == 64
    ASMAtomicUoWriteU64((volatile uint64_t RT_FAR *)(void RT_FAR *)ppv, (uint64_t)pv);
#else
# error "ARCH_BITS is bogus"
#endif
}


/**
 * Atomically writes a pointer value, ordered.
 *
 * @param   ppv     Pointer to the pointer variable to write.
 * @param   pv      The pointer value to assign to *ppv. If NULL use
 *                  ASMAtomicWriteNullPtr or you'll land in trouble.
 *
 * @remarks This is relatively type safe on GCC platforms when @a pv isn't
 *          NULL.
 */
#ifdef __GNUC__
# define ASMAtomicWritePtr(ppv, pv) \
    do \
    { \
        __typeof__(*(ppv)) volatile RT_FAR * const ppvTypeChecked = (ppv); \
        __typeof__(*(ppv))                   const pvTypeChecked  = (pv); \
        \
        AssertCompile(sizeof(*ppv) == sizeof(void RT_FAR *)); \
        AssertCompile(sizeof(pv) == sizeof(void RT_FAR *)); \
        Assert(!( (uintptr_t)ppv & ((ARCH_BITS / 8) - 1) )); \
        \
        ASMAtomicWritePtrVoid((void RT_FAR * volatile RT_FAR *)(ppvTypeChecked), (void RT_FAR *)(pvTypeChecked)); \
    } while (0)
#else
# define ASMAtomicWritePtr(ppv, pv) \
    do \
    { \
        AssertCompile(sizeof(*ppv) == sizeof(void RT_FAR *)); \
        AssertCompile(sizeof(pv) == sizeof(void RT_FAR *)); \
        Assert(!( (uintptr_t)ppv & ((ARCH_BITS / 8) - 1) )); \
        \
        ASMAtomicWritePtrVoid((void RT_FAR * volatile RT_FAR *)(ppv), (void RT_FAR *)(pv)); \
    } while (0)
#endif


/**
 * Atomically sets a pointer to NULL, ordered.
 *
 * @param   ppv     Pointer to the pointer variable that should be set to NULL.
 *
 * @remarks This is relatively type safe on GCC platforms.
 */
#if RT_GNUC_PREREQ(4, 2)
# define ASMAtomicWriteNullPtr(ppv) \
    do \
    { \
        __typeof__(*(ppv)) * const ppvTypeChecked = (ppv); \
        AssertCompile(sizeof(*ppv) == sizeof(void RT_FAR *)); \
        Assert(!( (uintptr_t)ppv & ((ARCH_BITS / 8) - 1) )); \
        ASMAtomicWritePtrVoid((void RT_FAR * volatile RT_FAR *)(ppvTypeChecked), NULL); \
    } while (0)
#else
# define ASMAtomicWriteNullPtr(ppv) \
    do \
    { \
        AssertCompile(sizeof(*ppv) == sizeof(void RT_FAR *)); \
        Assert(!( (uintptr_t)ppv & ((ARCH_BITS / 8) - 1) )); \
        ASMAtomicWritePtrVoid((void RT_FAR * volatile RT_FAR *)(ppv), NULL); \
    } while (0)
#endif


/**
 * Atomically writes a pointer value, unordered.
 *
 * @returns Current *pv value
 * @param   ppv     Pointer to the pointer variable.
 * @param   pv      The pointer value to assign to *ppv. If NULL use
 *                  ASMAtomicUoWriteNullPtr or you'll land in trouble.
 *
 * @remarks This is relatively type safe on GCC platforms when @a pv isn't
 *          NULL.
 */
#if RT_GNUC_PREREQ(4, 2)
# define ASMAtomicUoWritePtr(ppv, pv) \
    do \
    { \
        __typeof__(*(ppv)) volatile * const ppvTypeChecked = (ppv); \
        __typeof__(*(ppv))            const pvTypeChecked  = (pv); \
        \
        AssertCompile(sizeof(*ppv) == sizeof(void *)); \
        AssertCompile(sizeof(pv) == sizeof(void *)); \
        Assert(!( (uintptr_t)ppv & ((ARCH_BITS / 8) - 1) )); \
        \
        *(ppvTypeChecked) = pvTypeChecked; \
    } while (0)
#else
# define ASMAtomicUoWritePtr(ppv, pv) \
    do \
    { \
        AssertCompile(sizeof(*ppv) == sizeof(void RT_FAR *)); \
        AssertCompile(sizeof(pv) == sizeof(void RT_FAR *)); \
        Assert(!( (uintptr_t)ppv & ((ARCH_BITS / 8) - 1) )); \
        *(ppv) = pv; \
    } while (0)
#endif


/**
 * Atomically sets a pointer to NULL, unordered.
 *
 * @param   ppv     Pointer to the pointer variable that should be set to NULL.
 *
 * @remarks This is relatively type safe on GCC platforms.
 */
#ifdef __GNUC__
# define ASMAtomicUoWriteNullPtr(ppv) \
    do \
    { \
        __typeof__(*(ppv)) volatile * const ppvTypeChecked = (ppv); \
        AssertCompile(sizeof(*ppv) == sizeof(void *)); \
        Assert(!( (uintptr_t)ppv & ((ARCH_BITS / 8) - 1) )); \
        *(ppvTypeChecked) = NULL; \
    } while (0)
#else
# define ASMAtomicUoWriteNullPtr(ppv) \
    do \
    { \
        AssertCompile(sizeof(*ppv) == sizeof(void RT_FAR *)); \
        Assert(!( (uintptr_t)ppv & ((ARCH_BITS / 8) - 1) )); \
        *(ppv) = NULL; \
    } while (0)
#endif


/**
 * Atomically write a typical IPRT handle value, ordered.
 *
 * @param   ph      Pointer to the variable to update.
 * @param   hNew    The value to assign to *ph.
 *
 * @remarks This doesn't currently work for all handles (like RTFILE).
 */
#if HC_ARCH_BITS == 32 || ARCH_BITS == 16
# define ASMAtomicWriteHandle(ph, hNew) \
    do { \
        AssertCompile(sizeof(*(ph)) == sizeof(uint32_t)); \
        ASMAtomicWriteU32((uint32_t volatile RT_FAR *)(ph), (const uint32_t)(hNew)); \
    } while (0)
#elif HC_ARCH_BITS == 64
# define ASMAtomicWriteHandle(ph, hNew) \
    do { \
        AssertCompile(sizeof(*(ph)) == sizeof(uint64_t)); \
        ASMAtomicWriteU64((uint64_t volatile RT_FAR *)(ph), (const uint64_t)(hNew)); \
    } while (0)
#else
# error HC_ARCH_BITS
#endif


/**
 * Atomically write a typical IPRT handle value, unordered.
 *
 * @param   ph      Pointer to the variable to update.
 * @param   hNew    The value to assign to *ph.
 *
 * @remarks This doesn't currently work for all handles (like RTFILE).
 */
#if HC_ARCH_BITS == 32 || ARCH_BITS == 16
# define ASMAtomicUoWriteHandle(ph, hNew) \
    do { \
        AssertCompile(sizeof(*(ph)) == sizeof(uint32_t)); \
        ASMAtomicUoWriteU32((uint32_t volatile RT_FAR *)(ph), (const uint32_t)hNew); \
    } while (0)
#elif HC_ARCH_BITS == 64
# define ASMAtomicUoWriteHandle(ph, hNew) \
    do { \
        AssertCompile(sizeof(*(ph)) == sizeof(uint64_t)); \
        ASMAtomicUoWriteU64((uint64_t volatile RT_FAR *)(ph), (const uint64_t)hNew); \
    } while (0)
#else
# error HC_ARCH_BITS
#endif


/**
 * Atomically write a value which size might differ
 * between platforms or compilers, ordered.
 *
 * @param   pu      Pointer to the variable to update.
 * @param   uNew    The value to assign to *pu.
 */
#define ASMAtomicWriteSize(pu, uNew) \
    do { \
        switch (sizeof(*(pu))) { \
            case 1: ASMAtomicWriteU8( (volatile uint8_t  RT_FAR *)(void RT_FAR *)(pu), (uint8_t )(uNew)); break; \
            case 2: ASMAtomicWriteU16((volatile uint16_t RT_FAR *)(void RT_FAR *)(pu), (uint16_t)(uNew)); break; \
            case 4: ASMAtomicWriteU32((volatile uint32_t RT_FAR *)(void RT_FAR *)(pu), (uint32_t)(uNew)); break; \
            case 8: ASMAtomicWriteU64((volatile uint64_t RT_FAR *)(void RT_FAR *)(pu), (uint64_t)(uNew)); break; \
            default: AssertMsgFailed(("ASMAtomicWriteSize: size %d is not supported\n", sizeof(*(pu)))); \
        } \
    } while (0)

/**
 * Atomically write a value which size might differ
 * between platforms or compilers, unordered.
 *
 * @param   pu      Pointer to the variable to update.
 * @param   uNew    The value to assign to *pu.
 */
#define ASMAtomicUoWriteSize(pu, uNew) \
    do { \
        switch (sizeof(*(pu))) { \
            case 1: ASMAtomicUoWriteU8( (volatile uint8_t  RT_FAR *)(void RT_FAR *)(pu), (uint8_t )(uNew)); break; \
            case 2: ASMAtomicUoWriteU16((volatile uint16_t RT_FAR *)(void RT_FAR *)(pu), (uint16_t)(uNew)); break; \
            case 4: ASMAtomicUoWriteU32((volatile uint32_t RT_FAR *)(void RT_FAR *)(pu), (uint32_t)(uNew)); break; \
            case 8: ASMAtomicUoWriteU64((volatile uint64_t RT_FAR *)(void RT_FAR *)(pu), (uint64_t)(uNew)); break; \
            default: AssertMsgFailed(("ASMAtomicWriteSize: size %d is not supported\n", sizeof(*(pu)))); \
        } \
    } while (0)



/**
 * Atomically exchanges and adds to a 16-bit value, ordered.
 *
 * @returns The old value.
 * @param   pu16        Pointer to the value.
 * @param   u16         Number to add.
 *
 * @remarks Currently not implemented, just to make 16-bit code happy.
 * @remarks x86: Requires a 486 or later.
 */
RT_ASM_DECL_PRAGMA_WATCOM(uint16_t) ASMAtomicAddU16(uint16_t volatile RT_FAR *pu16, uint32_t u16) RT_NOTHROW_PROTO;


/**
 * Atomically exchanges and adds to a 32-bit value, ordered.
 *
 * @returns The old value.
 * @param   pu32        Pointer to the value.
 * @param   u32         Number to add.
 *
 * @remarks x86: Requires a 486 or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(uint32_t) ASMAtomicAddU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_PROTO;
#else
DECLINLINE(uint32_t) ASMAtomicAddU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    u32 = _InterlockedExchangeAdd((long RT_FAR *)pu32, u32);
    return u32;

# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("lock; xaddl %0, %1\n\t"
                         : "=r" (u32)
                         , "=m" (*pu32)
                         : "0" (u32)
                         , "m" (*pu32)
                         : "memory"
                         , "cc");
    return u32;
#  else
    __asm
    {
        mov     eax, [u32]
#   ifdef RT_ARCH_AMD64
        mov     rdx, [pu32]
        lock xadd [rdx], eax
#   else
        mov     edx, [pu32]
        lock xadd [edx], eax
#   endif
        mov     [u32], eax
    }
    return u32;
#  endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_OLD_32(ASMAtomicAddU32, pu32, DMB_SY,
                                           "add %w[uNew], %w[uOld], %w[uVal]\n\t",
                                           "add %[uNew], %[uOld], %[uVal]\n\t",
                                           [uVal] "r" (u32));
    return u32OldRet;

# else
#  error "Port me"
# endif
}
#endif


/**
 * Atomically exchanges and adds to a signed 32-bit value, ordered.
 *
 * @returns The old value.
 * @param   pi32        Pointer to the value.
 * @param   i32         Number to add.
 *
 * @remarks x86: Requires a 486 or later.
 */
DECLINLINE(int32_t) ASMAtomicAddS32(int32_t volatile RT_FAR *pi32, int32_t i32) RT_NOTHROW_DEF
{
    return (int32_t)ASMAtomicAddU32((uint32_t volatile RT_FAR *)pi32, (uint32_t)i32);
}


/**
 * Atomically exchanges and adds to a 64-bit value, ordered.
 *
 * @returns The old value.
 * @param   pu64        Pointer to the value.
 * @param   u64         Number to add.
 *
 * @remarks x86: Requires a Pentium or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
DECLASM(uint64_t) ASMAtomicAddU64(uint64_t volatile RT_FAR *pu64, uint64_t u64) RT_NOTHROW_PROTO;
#else
DECLINLINE(uint64_t) ASMAtomicAddU64(uint64_t volatile RT_FAR *pu64, uint64_t u64) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN && defined(RT_ARCH_AMD64)
    u64 = _InterlockedExchangeAdd64((__int64 RT_FAR *)pu64, u64);
    return u64;

# elif RT_INLINE_ASM_GNU_STYLE && defined(RT_ARCH_AMD64)
    __asm__ __volatile__("lock; xaddq %0, %1\n\t"
                         : "=r" (u64)
                         , "=m" (*pu64)
                         : "0" (u64)
                         , "m" (*pu64)
                         : "memory"
                         , "cc");
    return u64;

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_OLD_64(ASMAtomicAddU64, pu64, DMB_SY,
                                           "add %[uNew], %[uOld], %[uVal]\n\t"
                                           ,
                                           "add %[uNew], %[uOld], %[uVal]\n\t"
                                           "adc %H[uNew], %H[uOld], %H[uVal]\n\t",
                                           [uVal] "r" (u64));
    return u64OldRet;

# else
    uint64_t u64Old;
    for (;;)
    {
        uint64_t u64New;
        u64Old = ASMAtomicUoReadU64(pu64);
        u64New = u64Old + u64;
        if (ASMAtomicCmpXchgU64(pu64, u64New, u64Old))
            break;
        ASMNopPause();
    }
    return u64Old;
# endif
}
#endif


/**
 * Atomically exchanges and adds to a signed 64-bit value, ordered.
 *
 * @returns The old value.
 * @param   pi64        Pointer to the value.
 * @param   i64         Number to add.
 *
 * @remarks x86: Requires a Pentium or later.
 */
DECLINLINE(int64_t) ASMAtomicAddS64(int64_t volatile RT_FAR *pi64, int64_t i64) RT_NOTHROW_DEF
{
    return (int64_t)ASMAtomicAddU64((uint64_t volatile RT_FAR *)pi64, (uint64_t)i64);
}


/**
 * Atomically exchanges and adds to a size_t value, ordered.
 *
 * @returns The old value.
 * @param   pcb         Pointer to the size_t value.
 * @param   cb          Number to add.
 */
DECLINLINE(size_t) ASMAtomicAddZ(size_t volatile RT_FAR *pcb, size_t cb) RT_NOTHROW_DEF
{
#if ARCH_BITS == 64
    AssertCompileSize(size_t, 8);
    return ASMAtomicAddU64((uint64_t volatile RT_FAR *)pcb, cb);
#elif ARCH_BITS == 32
    AssertCompileSize(size_t, 4);
    return ASMAtomicAddU32((uint32_t volatile RT_FAR *)pcb, cb);
#elif ARCH_BITS == 16
    AssertCompileSize(size_t, 2);
    return ASMAtomicAddU16((uint16_t volatile RT_FAR *)pcb, cb);
#else
# error "Unsupported ARCH_BITS value"
#endif
}


/**
 * Atomically exchanges and adds a value which size might differ between
 * platforms or compilers, ordered.
 *
 * @param   pu      Pointer to the variable to update.
 * @param   uNew    The value to add to *pu.
 * @param   puOld   Where to store the old value.
 */
#define ASMAtomicAddSize(pu, uNew, puOld) \
    do { \
        switch (sizeof(*(pu))) { \
            case 4: *(uint32_t  *)(puOld) = ASMAtomicAddU32((volatile uint32_t RT_FAR *)(void RT_FAR *)(pu), (uint32_t)(uNew)); break; \
            case 8: *(uint64_t  *)(puOld) = ASMAtomicAddU64((volatile uint64_t RT_FAR *)(void RT_FAR *)(pu), (uint64_t)(uNew)); break; \
            default: AssertMsgFailed(("ASMAtomicAddSize: size %d is not supported\n", sizeof(*(pu)))); \
        } \
    } while (0)



/**
 * Atomically exchanges and subtracts to an unsigned 16-bit value, ordered.
 *
 * @returns The old value.
 * @param   pu16        Pointer to the value.
 * @param   u16         Number to subtract.
 *
 * @remarks x86: Requires a 486 or later.
 */
DECLINLINE(uint16_t) ASMAtomicSubU16(uint16_t volatile RT_FAR *pu16, uint32_t u16) RT_NOTHROW_DEF
{
    return ASMAtomicAddU16(pu16, (uint16_t)-(int16_t)u16);
}


/**
 * Atomically exchanges and subtracts to a signed 16-bit value, ordered.
 *
 * @returns The old value.
 * @param   pi16        Pointer to the value.
 * @param   i16         Number to subtract.
 *
 * @remarks x86: Requires a 486 or later.
 */
DECLINLINE(int16_t) ASMAtomicSubS16(int16_t volatile RT_FAR *pi16, int16_t i16) RT_NOTHROW_DEF
{
    return (int16_t)ASMAtomicAddU16((uint16_t volatile RT_FAR *)pi16, (uint16_t)-i16);
}


/**
 * Atomically exchanges and subtracts to an unsigned 32-bit value, ordered.
 *
 * @returns The old value.
 * @param   pu32        Pointer to the value.
 * @param   u32         Number to subtract.
 *
 * @remarks x86: Requires a 486 or later.
 */
DECLINLINE(uint32_t) ASMAtomicSubU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_DEF
{
    return ASMAtomicAddU32(pu32, (uint32_t)-(int32_t)u32);
}


/**
 * Atomically exchanges and subtracts to a signed 32-bit value, ordered.
 *
 * @returns The old value.
 * @param   pi32        Pointer to the value.
 * @param   i32         Number to subtract.
 *
 * @remarks x86: Requires a 486 or later.
 */
DECLINLINE(int32_t) ASMAtomicSubS32(int32_t volatile RT_FAR *pi32, int32_t i32) RT_NOTHROW_DEF
{
    return (int32_t)ASMAtomicAddU32((uint32_t volatile RT_FAR *)pi32, (uint32_t)-i32);
}


/**
 * Atomically exchanges and subtracts to an unsigned 64-bit value, ordered.
 *
 * @returns The old value.
 * @param   pu64        Pointer to the value.
 * @param   u64         Number to subtract.
 *
 * @remarks x86: Requires a Pentium or later.
 */
DECLINLINE(uint64_t) ASMAtomicSubU64(uint64_t volatile RT_FAR *pu64, uint64_t u64) RT_NOTHROW_DEF
{
    return ASMAtomicAddU64(pu64, (uint64_t)-(int64_t)u64);
}


/**
 * Atomically exchanges and subtracts to a signed 64-bit value, ordered.
 *
 * @returns The old value.
 * @param   pi64        Pointer to the value.
 * @param   i64         Number to subtract.
 *
 * @remarks x86: Requires a Pentium or later.
 */
DECLINLINE(int64_t) ASMAtomicSubS64(int64_t volatile RT_FAR *pi64, int64_t i64) RT_NOTHROW_DEF
{
    return (int64_t)ASMAtomicAddU64((uint64_t volatile RT_FAR *)pi64, (uint64_t)-i64);
}


/**
 * Atomically exchanges and subtracts to a size_t value, ordered.
 *
 * @returns The old value.
 * @param   pcb         Pointer to the size_t value.
 * @param   cb          Number to subtract.
 *
 * @remarks x86: Requires a 486 or later.
 */
DECLINLINE(size_t) ASMAtomicSubZ(size_t volatile RT_FAR *pcb, size_t cb) RT_NOTHROW_DEF
{
#if ARCH_BITS == 64
    return ASMAtomicSubU64((uint64_t volatile RT_FAR *)pcb, cb);
#elif ARCH_BITS == 32
    return ASMAtomicSubU32((uint32_t volatile RT_FAR *)pcb, cb);
#elif ARCH_BITS == 16
    AssertCompileSize(size_t, 2);
    return ASMAtomicSubU16((uint16_t volatile RT_FAR *)pcb, cb);
#else
# error "Unsupported ARCH_BITS value"
#endif
}


/**
 * Atomically exchanges and subtracts a value which size might differ between
 * platforms or compilers, ordered.
 *
 * @param   pu      Pointer to the variable to update.
 * @param   uNew    The value to subtract to *pu.
 * @param   puOld   Where to store the old value.
 *
 * @remarks x86: Requires a 486 or later.
 */
#define ASMAtomicSubSize(pu, uNew, puOld) \
    do { \
        switch (sizeof(*(pu))) { \
            case 4: *(uint32_t RT_FAR *)(puOld) = ASMAtomicSubU32((volatile uint32_t RT_FAR *)(void RT_FAR *)(pu), (uint32_t)(uNew)); break; \
            case 8: *(uint64_t RT_FAR *)(puOld) = ASMAtomicSubU64((volatile uint64_t RT_FAR *)(void RT_FAR *)(pu), (uint64_t)(uNew)); break; \
            default: AssertMsgFailed(("ASMAtomicSubSize: size %d is not supported\n", sizeof(*(pu)))); \
        } \
    } while (0)



/**
 * Atomically increment a 16-bit value, ordered.
 *
 * @returns The new value.
 * @param   pu16        Pointer to the value to increment.
 * @remarks Not implemented. Just to make 16-bit code happy.
 *
 * @remarks x86: Requires a 486 or later.
 */
RT_ASM_DECL_PRAGMA_WATCOM(uint16_t) ASMAtomicIncU16(uint16_t volatile RT_FAR *pu16) RT_NOTHROW_PROTO;


/**
 * Atomically increment a 32-bit value, ordered.
 *
 * @returns The new value.
 * @param   pu32        Pointer to the value to increment.
 *
 * @remarks x86: Requires a 486 or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(uint32_t) ASMAtomicIncU32(uint32_t volatile RT_FAR *pu32) RT_NOTHROW_PROTO;
#else
DECLINLINE(uint32_t) ASMAtomicIncU32(uint32_t volatile RT_FAR *pu32) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    return (uint32_t)_InterlockedIncrement((long RT_FAR *)pu32);

# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    uint32_t u32;
    __asm__ __volatile__("lock; xaddl %0, %1\n\t"
                         : "=r" (u32)
                         , "=m" (*pu32)
                         : "0" (1)
                         , "m" (*pu32)
                         : "memory"
                         , "cc");
    return u32+1;
#  else
    __asm
    {
        mov     eax, 1
#   ifdef RT_ARCH_AMD64
        mov     rdx, [pu32]
        lock xadd [rdx], eax
#   else
        mov     edx, [pu32]
        lock xadd [edx], eax
#   endif
        mov     u32, eax
    }
    return u32+1;
#  endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_NEW_32(ASMAtomicIncU32, pu32, DMB_SY,
                                           "add %w[uNew], %w[uNew], #1\n\t",
                                           "add %[uNew], %[uNew], #1\n\t" /* arm6 / thumb2+ */,
                                           "X" (0) /* dummy */);
    return u32NewRet;

# else
    return ASMAtomicAddU32(pu32, 1) + 1;
# endif
}
#endif


/**
 * Atomically increment a signed 32-bit value, ordered.
 *
 * @returns The new value.
 * @param   pi32        Pointer to the value to increment.
 *
 * @remarks x86: Requires a 486 or later.
 */
DECLINLINE(int32_t) ASMAtomicIncS32(int32_t volatile RT_FAR *pi32) RT_NOTHROW_DEF
{
    return (int32_t)ASMAtomicIncU32((uint32_t volatile RT_FAR *)pi32);
}


/**
 * Atomically increment a 64-bit value, ordered.
 *
 * @returns The new value.
 * @param   pu64        Pointer to the value to increment.
 *
 * @remarks x86: Requires a Pentium or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
DECLASM(uint64_t) ASMAtomicIncU64(uint64_t volatile RT_FAR *pu64) RT_NOTHROW_PROTO;
#else
DECLINLINE(uint64_t) ASMAtomicIncU64(uint64_t volatile RT_FAR *pu64) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN && defined(RT_ARCH_AMD64)
    return (uint64_t)_InterlockedIncrement64((__int64 RT_FAR *)pu64);

# elif RT_INLINE_ASM_GNU_STYLE && defined(RT_ARCH_AMD64)
    uint64_t u64;
    __asm__ __volatile__("lock; xaddq %0, %1\n\t"
                         : "=r" (u64)
                         , "=m" (*pu64)
                         : "0" (1)
                         , "m" (*pu64)
                         : "memory"
                         , "cc");
    return u64 + 1;

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_NEW_64(ASMAtomicIncU64, pu64, DMB_SY,
                                           "add %[uNew], %[uNew], #1\n\t"
                                           ,
                                           "add %[uNew], %[uNew], #1\n\t" /* arm6 / thumb2+ */
                                           "adc %H[uNew], %H[uNew], %[uZeroVal]\n\t",
                                           RTASM_ARM_PICK_6432("X" (0) /* dummy */, [uZeroVal] "r" (0)) );
    return u64NewRet;

# else
    return ASMAtomicAddU64(pu64, 1) + 1;
# endif
}
#endif


/**
 * Atomically increment a signed 64-bit value, ordered.
 *
 * @returns The new value.
 * @param   pi64        Pointer to the value to increment.
 *
 * @remarks x86: Requires a Pentium or later.
 */
DECLINLINE(int64_t) ASMAtomicIncS64(int64_t volatile RT_FAR *pi64) RT_NOTHROW_DEF
{
    return (int64_t)ASMAtomicIncU64((uint64_t volatile RT_FAR *)pi64);
}


/**
 * Atomically increment a size_t value, ordered.
 *
 * @returns The new value.
 * @param   pcb         Pointer to the value to increment.
 *
 * @remarks x86: Requires a 486 or later.
 */
DECLINLINE(size_t) ASMAtomicIncZ(size_t volatile RT_FAR *pcb) RT_NOTHROW_DEF
{
#if ARCH_BITS == 64
    return ASMAtomicIncU64((uint64_t volatile RT_FAR *)pcb);
#elif ARCH_BITS == 32
    return ASMAtomicIncU32((uint32_t volatile RT_FAR *)pcb);
#elif ARCH_BITS == 16
    return ASMAtomicIncU16((uint16_t volatile RT_FAR *)pcb);
#else
# error "Unsupported ARCH_BITS value"
#endif
}



/**
 * Atomically decrement an unsigned 32-bit value, ordered.
 *
 * @returns The new value.
 * @param   pu16        Pointer to the value to decrement.
 * @remarks Not implemented. Just to make 16-bit code happy.
 *
 * @remarks x86: Requires a 486 or later.
 */
RT_ASM_DECL_PRAGMA_WATCOM(uint32_t) ASMAtomicDecU16(uint16_t volatile RT_FAR *pu16) RT_NOTHROW_PROTO;


/**
 * Atomically decrement an unsigned 32-bit value, ordered.
 *
 * @returns The new value.
 * @param   pu32        Pointer to the value to decrement.
 *
 * @remarks x86: Requires a 486 or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(uint32_t) ASMAtomicDecU32(uint32_t volatile RT_FAR *pu32) RT_NOTHROW_PROTO;
#else
DECLINLINE(uint32_t) ASMAtomicDecU32(uint32_t volatile RT_FAR *pu32) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    return (uint32_t)_InterlockedDecrement((long RT_FAR *)pu32);

# elif RT_INLINE_ASM_GNU_STYLE && defined(RT_ARCH_AMD64)
#  if RT_INLINE_ASM_GNU_STYLE
    uint32_t u32;
    __asm__ __volatile__("lock; xaddl %0, %1\n\t"
                         : "=r" (u32)
                         , "=m" (*pu32)
                         : "0" (-1)
                         , "m" (*pu32)
                         : "memory"
                         , "cc");
    return u32-1;
# else
    uint32_t u32;
    __asm
    {
        mov     eax, -1
#  ifdef RT_ARCH_AMD64
        mov     rdx, [pu32]
        lock xadd [rdx], eax
#  else
        mov     edx, [pu32]
        lock xadd [edx], eax
#  endif
        mov     u32, eax
    }
    return u32-1;
# endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_NEW_32(ASMAtomicDecU32, pu32, DMB_SY,
                                           "sub %w[uNew], %w[uNew], #1\n\t",
                                           "sub %[uNew], %[uNew], #1\n\t" /* arm6 / thumb2+ */,
                                           "X" (0) /* dummy */);
    return u32NewRet;

# else
    return ASMAtomicSubU32(pu32, 1) - (uint32_t)1;
# endif
}
#endif


/**
 * Atomically decrement a signed 32-bit value, ordered.
 *
 * @returns The new value.
 * @param   pi32        Pointer to the value to decrement.
 *
 * @remarks x86: Requires a 486 or later.
 */
DECLINLINE(int32_t) ASMAtomicDecS32(int32_t volatile RT_FAR *pi32) RT_NOTHROW_DEF
{
    return (int32_t)ASMAtomicDecU32((uint32_t volatile RT_FAR *)pi32);
}


/**
 * Atomically decrement an unsigned 64-bit value, ordered.
 *
 * @returns The new value.
 * @param   pu64        Pointer to the value to decrement.
 *
 * @remarks x86: Requires a Pentium or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(uint64_t) ASMAtomicDecU64(uint64_t volatile RT_FAR *pu64) RT_NOTHROW_PROTO;
#else
DECLINLINE(uint64_t) ASMAtomicDecU64(uint64_t volatile RT_FAR *pu64) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN && defined(RT_ARCH_AMD64)
    return (uint64_t)_InterlockedDecrement64((__int64 volatile RT_FAR *)pu64);

# elif RT_INLINE_ASM_GNU_STYLE && defined(RT_ARCH_AMD64)
    uint64_t u64;
    __asm__ __volatile__("lock; xaddq %q0, %1\n\t"
                         : "=r" (u64)
                         , "=m" (*pu64)
                         : "0" (~(uint64_t)0)
                         , "m" (*pu64)
                         : "memory"
                         , "cc");
    return u64-1;

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_NEW_64(ASMAtomicDecU64, pu64, DMB_SY,
                                           "sub %[uNew], %[uNew], #1\n\t"
                                           ,
                                           "sub %[uNew], %[uNew], #1\n\t" /* arm6 / thumb2+ */
                                           "sbc %H[uNew], %H[uNew], %[uZeroVal]\n\t",
                                           RTASM_ARM_PICK_6432("X" (0) /* dummy */, [uZeroVal] "r" (0)) );
    return u64NewRet;

# else
    return ASMAtomicAddU64(pu64, UINT64_MAX) - 1;
# endif
}
#endif


/**
 * Atomically decrement a signed 64-bit value, ordered.
 *
 * @returns The new value.
 * @param   pi64        Pointer to the value to decrement.
 *
 * @remarks x86: Requires a Pentium or later.
 */
DECLINLINE(int64_t) ASMAtomicDecS64(int64_t volatile RT_FAR *pi64) RT_NOTHROW_DEF
{
    return (int64_t)ASMAtomicDecU64((uint64_t volatile RT_FAR *)pi64);
}


/**
 * Atomically decrement a size_t value, ordered.
 *
 * @returns The new value.
 * @param   pcb         Pointer to the value to decrement.
 *
 * @remarks x86: Requires a 486 or later.
 */
DECLINLINE(size_t) ASMAtomicDecZ(size_t volatile RT_FAR *pcb) RT_NOTHROW_DEF
{
#if ARCH_BITS == 64
    return ASMAtomicDecU64((uint64_t volatile RT_FAR *)pcb);
#elif ARCH_BITS == 32
    return ASMAtomicDecU32((uint32_t volatile RT_FAR *)pcb);
#elif ARCH_BITS == 16
    return ASMAtomicDecU16((uint16_t volatile RT_FAR *)pcb);
#else
# error "Unsupported ARCH_BITS value"
#endif
}


/**
 * Atomically Or an unsigned 32-bit value, ordered.
 *
 * @param   pu32   Pointer to the pointer variable to OR u32 with.
 * @param   u32    The value to OR *pu32 with.
 *
 * @remarks x86: Requires a 386 or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMAtomicOrU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMAtomicOrU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    _InterlockedOr((long volatile RT_FAR *)pu32, (long)u32);

# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("lock; orl %1, %0\n\t"
                         : "=m" (*pu32)
                         : "ir" (u32)
                         , "m" (*pu32)
                         : "cc");
#  else
    __asm
    {
        mov     eax, [u32]
#   ifdef RT_ARCH_AMD64
        mov     rdx, [pu32]
        lock    or [rdx], eax
#   else
        mov     edx, [pu32]
        lock    or [edx], eax
#   endif
    }
#  endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    /* For more on Orr see https://en.wikipedia.org/wiki/Orr_(Catch-22) ;-) */
    RTASM_ARM_LOAD_MODIFY_STORE_RET_NEW_32(ASMAtomicOr32, pu32, DMB_SY,
                                           "orr %w[uNew], %w[uNew], %w[uVal]\n\t",
                                           "orr %[uNew], %[uNew], %[uVal]\n\t",
                                           [uVal] "r" (u32));

# else
#  error "Port me"
# endif
}
#endif


/**
 * Atomically OR an unsigned 32-bit value, ordered, extended version (for bitmap
 * fallback).
 *
 * @returns Old value.
 * @param   pu32   Pointer to the variable to OR @a u32 with.
 * @param   u32    The value to OR @a *pu32 with.
 */
DECLINLINE(uint32_t) ASMAtomicOrExU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_DEF
{
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_OLD_32(ASMAtomicOrEx32, pu32, DMB_SY,
                                           "orr %w[uNew], %w[uOld], %w[uVal]\n\t",
                                           "orr %[uNew], %[uOld], %[uVal]\n\t",
                                           [uVal] "r" (u32));
    return u32OldRet;

#else
    uint32_t u32RetOld = ASMAtomicUoReadU32(pu32);
    uint32_t u32New;
    do
        u32New = u32RetOld | u32;
    while (!ASMAtomicCmpXchgExU32(pu32, u32New, u32RetOld, &u32RetOld));
    return u32RetOld;
#endif
}


/**
 * Atomically Or a signed 32-bit value, ordered.
 *
 * @param   pi32   Pointer to the pointer variable to OR u32 with.
 * @param   i32    The value to OR *pu32 with.
 *
 * @remarks x86: Requires a 386 or later.
 */
DECLINLINE(void) ASMAtomicOrS32(int32_t volatile RT_FAR *pi32, int32_t i32) RT_NOTHROW_DEF
{
    ASMAtomicOrU32((uint32_t volatile RT_FAR *)pi32, (uint32_t)i32);
}


/**
 * Atomically Or an unsigned 64-bit value, ordered.
 *
 * @param   pu64   Pointer to the pointer variable to OR u64 with.
 * @param   u64    The value to OR *pu64 with.
 *
 * @remarks x86: Requires a Pentium or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
DECLASM(void) ASMAtomicOrU64(uint64_t volatile RT_FAR *pu64, uint64_t u64) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMAtomicOrU64(uint64_t volatile RT_FAR *pu64, uint64_t u64) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN && defined(RT_ARCH_AMD64)
    _InterlockedOr64((__int64 volatile RT_FAR *)pu64, (__int64)u64);

# elif RT_INLINE_ASM_GNU_STYLE && defined(RT_ARCH_AMD64)
    __asm__ __volatile__("lock; orq %1, %q0\n\t"
                         : "=m" (*pu64)
                         : "r" (u64)
                         , "m" (*pu64)
                         : "cc");

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_NEW_64(ASMAtomicOrU64, pu64, DMB_SY,
                                           "orr %[uNew], %[uNew], %[uVal]\n\t"
                                           ,
                                           "orr %[uNew], %[uNew], %[uVal]\n\t"
                                           "orr %H[uNew], %H[uNew], %H[uVal]\n\t",
                                           [uVal] "r" (u64));

# else
    for (;;)
    {
        uint64_t u64Old = ASMAtomicUoReadU64(pu64);
        uint64_t u64New = u64Old | u64;
        if (ASMAtomicCmpXchgU64(pu64, u64New, u64Old))
            break;
        ASMNopPause();
    }
# endif
}
#endif


/**
 * Atomically Or a signed 64-bit value, ordered.
 *
 * @param   pi64   Pointer to the pointer variable to OR u64 with.
 * @param   i64    The value to OR *pu64 with.
 *
 * @remarks x86: Requires a Pentium or later.
 */
DECLINLINE(void) ASMAtomicOrS64(int64_t volatile RT_FAR *pi64, int64_t i64) RT_NOTHROW_DEF
{
    ASMAtomicOrU64((uint64_t volatile RT_FAR *)pi64, (uint64_t)i64);
}


/**
 * Atomically And an unsigned 32-bit value, ordered.
 *
 * @param   pu32   Pointer to the pointer variable to AND u32 with.
 * @param   u32    The value to AND *pu32 with.
 *
 * @remarks x86: Requires a 386 or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMAtomicAndU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMAtomicAndU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    _InterlockedAnd((long volatile RT_FAR *)pu32, u32);

# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("lock; andl %1, %0\n\t"
                         : "=m" (*pu32)
                         : "ir" (u32)
                         , "m" (*pu32)
                         : "cc");
#  else
    __asm
    {
        mov     eax, [u32]
#   ifdef RT_ARCH_AMD64
        mov     rdx, [pu32]
        lock and [rdx], eax
#   else
        mov     edx, [pu32]
        lock and [edx], eax
#   endif
    }
#  endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_NEW_32(ASMAtomicAnd32, pu32, DMB_SY,
                                           "and %w[uNew], %w[uNew], %w[uVal]\n\t",
                                           "and %[uNew], %[uNew], %[uVal]\n\t",
                                           [uVal] "r" (u32));

# else
#  error "Port me"
# endif
}
#endif


/**
 * Atomically AND an unsigned 32-bit value, ordered, extended version.
 *
 * @returns Old value.
 * @param   pu32   Pointer to the variable to AND @a u32 with.
 * @param   u32    The value to AND @a *pu32 with.
 */
DECLINLINE(uint32_t) ASMAtomicAndExU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_DEF
{
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_OLD_32(ASMAtomicAndEx32, pu32, DMB_SY,
                                           "and %w[uNew], %w[uOld], %w[uVal]\n\t",
                                           "and %[uNew], %[uOld], %[uVal]\n\t",
                                           [uVal] "r" (u32));
    return u32OldRet;

#else
    uint32_t u32RetOld = ASMAtomicUoReadU32(pu32);
    uint32_t u32New;
    do
        u32New = u32RetOld & u32;
    while (!ASMAtomicCmpXchgExU32(pu32, u32New, u32RetOld, &u32RetOld));
    return u32RetOld;
#endif
}


/**
 * Atomically And a signed 32-bit value, ordered.
 *
 * @param   pi32   Pointer to the pointer variable to AND i32 with.
 * @param   i32    The value to AND *pi32 with.
 *
 * @remarks x86: Requires a 386 or later.
 */
DECLINLINE(void) ASMAtomicAndS32(int32_t volatile RT_FAR *pi32, int32_t i32) RT_NOTHROW_DEF
{
    ASMAtomicAndU32((uint32_t volatile RT_FAR *)pi32, (uint32_t)i32);
}


/**
 * Atomically And an unsigned 64-bit value, ordered.
 *
 * @param   pu64   Pointer to the pointer variable to AND u64 with.
 * @param   u64    The value to AND *pu64 with.
 *
 * @remarks x86: Requires a Pentium or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
DECLASM(void) ASMAtomicAndU64(uint64_t volatile RT_FAR *pu64, uint64_t u64) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMAtomicAndU64(uint64_t volatile RT_FAR *pu64, uint64_t u64) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN && defined(RT_ARCH_AMD64)
    _InterlockedAnd64((__int64 volatile RT_FAR *)pu64, u64);

# elif RT_INLINE_ASM_GNU_STYLE && defined(RT_ARCH_AMD64)
    __asm__ __volatile__("lock; andq %1, %0\n\t"
                         : "=m" (*pu64)
                         : "r" (u64)
                         , "m" (*pu64)
                         : "cc");

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_NEW_64(ASMAtomicAndU64, pu64, DMB_SY,
                                           "and %[uNew], %[uNew], %[uVal]\n\t"
                                           ,
                                           "and %[uNew], %[uNew], %[uVal]\n\t"
                                           "and %H[uNew], %H[uNew], %H[uVal]\n\t",
                                           [uVal] "r" (u64));

# else
    for (;;)
    {
        uint64_t u64Old = ASMAtomicUoReadU64(pu64);
        uint64_t u64New = u64Old & u64;
        if (ASMAtomicCmpXchgU64(pu64, u64New, u64Old))
            break;
        ASMNopPause();
    }
# endif
}
#endif


/**
 * Atomically And a signed 64-bit value, ordered.
 *
 * @param   pi64   Pointer to the pointer variable to AND i64 with.
 * @param   i64    The value to AND *pi64 with.
 *
 * @remarks x86: Requires a Pentium or later.
 */
DECLINLINE(void) ASMAtomicAndS64(int64_t volatile RT_FAR *pi64, int64_t i64) RT_NOTHROW_DEF
{
    ASMAtomicAndU64((uint64_t volatile RT_FAR *)pi64, (uint64_t)i64);
}


/**
 * Atomically XOR an unsigned 32-bit value and a memory location, ordered.
 *
 * @param   pu32   Pointer to the variable to XOR @a u32 with.
 * @param   u32    The value to XOR @a *pu32 with.
 *
 * @remarks x86: Requires a 386 or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMAtomicXorU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMAtomicXorU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    _InterlockedXor((long volatile RT_FAR *)pu32, u32);

# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("lock; xorl %1, %0\n\t"
                         : "=m" (*pu32)
                         : "ir" (u32)
                         , "m" (*pu32)
                         : "cc");
#  else
    __asm
    {
        mov     eax, [u32]
#   ifdef RT_ARCH_AMD64
        mov     rdx, [pu32]
        lock xor [rdx], eax
#   else
        mov     edx, [pu32]
        lock xor [edx], eax
#   endif
    }
#  endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_NEW_32(ASMAtomicXor32, pu32, DMB_SY,
                                           "eor %w[uNew], %w[uNew], %w[uVal]\n\t",
                                           "eor %[uNew], %[uNew], %[uVal]\n\t",
                                           [uVal] "r" (u32));

# else
#  error "Port me"
# endif
}
#endif


/**
 * Atomically XOR an unsigned 32-bit value and a memory location, ordered,
 * extended version (for bitmaps).
 *
 * @returns Old value.
 * @param   pu32   Pointer to the variable to XOR @a u32 with.
 * @param   u32    The value to XOR @a *pu32 with.
 */
DECLINLINE(uint32_t) ASMAtomicXorExU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_DEF
{
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_OLD_32(ASMAtomicXorEx32, pu32, DMB_SY,
                                           "eor %w[uNew], %w[uOld], %w[uVal]\n\t",
                                           "eor %[uNew], %[uOld], %[uVal]\n\t",
                                           [uVal] "r" (u32));
    return u32OldRet;

#else
    uint32_t u32RetOld = ASMAtomicUoReadU32(pu32);
    uint32_t u32New;
    do
        u32New = u32RetOld ^ u32;
    while (!ASMAtomicCmpXchgExU32(pu32, u32New, u32RetOld, &u32RetOld));
    return u32RetOld;
#endif
}


/**
 * Atomically XOR a signed 32-bit value, ordered.
 *
 * @param   pi32   Pointer to the variable to XOR i32 with.
 * @param   i32    The value to XOR *pi32 with.
 *
 * @remarks x86: Requires a 386 or later.
 */
DECLINLINE(void) ASMAtomicXorS32(int32_t volatile RT_FAR *pi32, int32_t i32) RT_NOTHROW_DEF
{
    ASMAtomicXorU32((uint32_t volatile RT_FAR *)pi32, (uint32_t)i32);
}


/**
 * Atomically OR an unsigned 32-bit value, unordered but interrupt safe.
 *
 * @param   pu32   Pointer to the pointer variable to OR u32 with.
 * @param   u32    The value to OR *pu32 with.
 *
 * @remarks x86: Requires a 386 or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMAtomicUoOrU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMAtomicUoOrU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_DEF
{
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("orl %1, %0\n\t"
                         : "=m" (*pu32)
                         : "ir" (u32)
                         , "m" (*pu32)
                         : "cc");
#  else
    __asm
    {
        mov     eax, [u32]
#   ifdef RT_ARCH_AMD64
        mov     rdx, [pu32]
        or      [rdx], eax
#   else
        mov     edx, [pu32]
        or      [edx], eax
#   endif
    }
#  endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_NEW_32(ASMAtomicUoOrU32, pu32, NO_BARRIER,
                                           "orr %w[uNew], %w[uNew], %w[uVal]\n\t",
                                           "orr %[uNew], %[uNew], %[uVal]\n\t",
                                           [uVal] "r" (u32));

# else
#  error "Port me"
# endif
}
#endif


/**
 * Atomically OR an unsigned 32-bit value, unordered but interrupt safe,
 * extended version (for bitmap fallback).
 *
 * @returns Old value.
 * @param   pu32   Pointer to the variable to OR @a u32 with.
 * @param   u32    The value to OR @a *pu32 with.
 */
DECLINLINE(uint32_t) ASMAtomicUoOrExU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_DEF
{
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_OLD_32(ASMAtomicUoOrExU32, pu32, NO_BARRIER,
                                           "orr %w[uNew], %w[uOld], %w[uVal]\n\t",
                                           "orr %[uNew], %[uOld], %[uVal]\n\t",
                                           [uVal] "r" (u32));
    return u32OldRet;

#else
    return ASMAtomicOrExU32(pu32, u32); /* (we have no unordered cmpxchg primitive atm.) */
#endif
}


/**
 * Atomically OR a signed 32-bit value, unordered.
 *
 * @param   pi32   Pointer to the pointer variable to OR u32 with.
 * @param   i32    The value to OR *pu32 with.
 *
 * @remarks x86: Requires a 386 or later.
 */
DECLINLINE(void) ASMAtomicUoOrS32(int32_t volatile RT_FAR *pi32, int32_t i32) RT_NOTHROW_DEF
{
    ASMAtomicUoOrU32((uint32_t volatile RT_FAR *)pi32, (uint32_t)i32);
}


/**
 * Atomically OR an unsigned 64-bit value, unordered.
 *
 * @param   pu64   Pointer to the pointer variable to OR u64 with.
 * @param   u64    The value to OR *pu64 with.
 *
 * @remarks x86: Requires a Pentium or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM
DECLASM(void) ASMAtomicUoOrU64(uint64_t volatile RT_FAR *pu64, uint64_t u64) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMAtomicUoOrU64(uint64_t volatile RT_FAR *pu64, uint64_t u64) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_GNU_STYLE && defined(RT_ARCH_AMD64)
    __asm__ __volatile__("orq %1, %q0\n\t"
                         : "=m" (*pu64)
                         : "r" (u64)
                         , "m" (*pu64)
                         : "cc");

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_NEW_64(ASMAtomicUoOrU64, pu64, NO_BARRIER,
                                           "orr %[uNew], %[uNew], %[uVal]\n\t"
                                           ,
                                           "orr %[uNew], %[uNew], %[uVal]\n\t"
                                           "orr %H[uNew], %H[uNew], %H[uVal]\n\t",
                                           [uVal] "r" (u64));

# else
    for (;;)
    {
        uint64_t u64Old = ASMAtomicUoReadU64(pu64);
        uint64_t u64New = u64Old | u64;
        if (ASMAtomicCmpXchgU64(pu64, u64New, u64Old))
            break;
        ASMNopPause();
    }
# endif
}
#endif


/**
 * Atomically Or a signed 64-bit value, unordered.
 *
 * @param   pi64   Pointer to the pointer variable to OR u64 with.
 * @param   i64    The value to OR *pu64 with.
 *
 * @remarks x86: Requires a Pentium or later.
 */
DECLINLINE(void) ASMAtomicUoOrS64(int64_t volatile RT_FAR *pi64, int64_t i64) RT_NOTHROW_DEF
{
    ASMAtomicUoOrU64((uint64_t volatile RT_FAR *)pi64, (uint64_t)i64);
}


/**
 * Atomically And an unsigned 32-bit value, unordered.
 *
 * @param   pu32   Pointer to the pointer variable to AND u32 with.
 * @param   u32    The value to AND *pu32 with.
 *
 * @remarks x86: Requires a 386 or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMAtomicUoAndU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMAtomicUoAndU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_DEF
{
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("andl %1, %0\n\t"
                         : "=m" (*pu32)
                         : "ir" (u32)
                         , "m" (*pu32)
                         : "cc");
#  else
    __asm
    {
        mov     eax, [u32]
#   ifdef RT_ARCH_AMD64
        mov     rdx, [pu32]
        and     [rdx], eax
#   else
        mov     edx, [pu32]
        and     [edx], eax
#   endif
    }
#  endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_NEW_32(ASMAtomicUoAnd32, pu32, NO_BARRIER,
                                           "and %w[uNew], %w[uNew], %w[uVal]\n\t",
                                           "and %[uNew], %[uNew], %[uVal]\n\t",
                                           [uVal] "r" (u32));

# else
#  error "Port me"
# endif
}
#endif


/**
 * Atomically AND an unsigned 32-bit value, unordered, extended version (for
 * bitmap fallback).
 *
 * @returns Old value.
 * @param   pu32   Pointer to the pointer to AND @a u32 with.
 * @param   u32    The value to AND @a *pu32 with.
 */
DECLINLINE(uint32_t) ASMAtomicUoAndExU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_DEF
{
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_OLD_32(ASMAtomicUoAndEx32, pu32, NO_BARRIER,
                                           "and %w[uNew], %w[uOld], %w[uVal]\n\t",
                                           "and %[uNew], %[uOld], %[uVal]\n\t",
                                           [uVal] "r" (u32));
    return u32OldRet;

#else
    return ASMAtomicAndExU32(pu32, u32); /* (we have no unordered cmpxchg primitive atm.) */
#endif
}


/**
 * Atomically And a signed 32-bit value, unordered.
 *
 * @param   pi32   Pointer to the pointer variable to AND i32 with.
 * @param   i32    The value to AND *pi32 with.
 *
 * @remarks x86: Requires a 386 or later.
 */
DECLINLINE(void) ASMAtomicUoAndS32(int32_t volatile RT_FAR *pi32, int32_t i32) RT_NOTHROW_DEF
{
    ASMAtomicUoAndU32((uint32_t volatile RT_FAR *)pi32, (uint32_t)i32);
}


/**
 * Atomically And an unsigned 64-bit value, unordered.
 *
 * @param   pu64   Pointer to the pointer variable to AND u64 with.
 * @param   u64    The value to AND *pu64 with.
 *
 * @remarks x86: Requires a Pentium or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM
DECLASM(void) ASMAtomicUoAndU64(uint64_t volatile RT_FAR *pu64, uint64_t u64) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMAtomicUoAndU64(uint64_t volatile RT_FAR *pu64, uint64_t u64) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_GNU_STYLE && defined(RT_ARCH_AMD64)
    __asm__ __volatile__("andq %1, %0\n\t"
                         : "=m" (*pu64)
                         : "r" (u64)
                         , "m" (*pu64)
                         : "cc");

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_NEW_64(ASMAtomicUoAndU64, pu64, NO_BARRIER,
                                           "and %[uNew], %[uNew], %[uVal]\n\t"
                                           ,
                                           "and %[uNew], %[uNew], %[uVal]\n\t"
                                           "and %H[uNew], %H[uNew], %H[uVal]\n\t",
                                           [uVal] "r" (u64));

# else
    for (;;)
    {
        uint64_t u64Old = ASMAtomicUoReadU64(pu64);
        uint64_t u64New = u64Old & u64;
        if (ASMAtomicCmpXchgU64(pu64, u64New, u64Old))
            break;
        ASMNopPause();
    }
# endif
}
#endif


/**
 * Atomically And a signed 64-bit value, unordered.
 *
 * @param   pi64   Pointer to the pointer variable to AND i64 with.
 * @param   i64    The value to AND *pi64 with.
 *
 * @remarks x86: Requires a Pentium or later.
 */
DECLINLINE(void) ASMAtomicUoAndS64(int64_t volatile RT_FAR *pi64, int64_t i64) RT_NOTHROW_DEF
{
    ASMAtomicUoAndU64((uint64_t volatile RT_FAR *)pi64, (uint64_t)i64);
}


/**
 * Atomically XOR an unsigned 32-bit value, unordered but interrupt safe.
 *
 * @param   pu32   Pointer to the variable to XOR @a u32 with.
 * @param   u32    The value to OR @a *pu32 with.
 *
 * @remarks x86: Requires a 386 or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMAtomicUoXorU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMAtomicUoXorU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_DEF
{
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("xorl %1, %0\n\t"
                         : "=m" (*pu32)
                         : "ir" (u32)
                         , "m" (*pu32)
                         : "cc");
#  else
    __asm
    {
        mov     eax, [u32]
#   ifdef RT_ARCH_AMD64
        mov     rdx, [pu32]
        xor     [rdx], eax
#   else
        mov     edx, [pu32]
        xor     [edx], eax
#   endif
    }
#  endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_NEW_32(ASMAtomicUoXorU32, pu32, NO_BARRIER,
                                           "eor %w[uNew], %w[uNew], %w[uVal]\n\t",
                                           "eor %[uNew], %[uNew], %[uVal]\n\t",
                                           [uVal] "r" (u32));

# else
#  error "Port me"
# endif
}
#endif


/**
 * Atomically XOR an unsigned 32-bit value, unordered but interrupt safe,
 * extended version (for bitmap fallback).
 *
 * @returns Old value.
 * @param   pu32   Pointer to the variable to XOR @a u32 with.
 * @param   u32    The value to OR @a *pu32 with.
 */
DECLINLINE(uint32_t) ASMAtomicUoXorExU32(uint32_t volatile RT_FAR *pu32, uint32_t u32) RT_NOTHROW_DEF
{
#if defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_OLD_32(ASMAtomicUoXorExU32, pu32, NO_BARRIER,
                                           "eor %w[uNew], %w[uOld], %w[uVal]\n\t",
                                           "eor %[uNew], %[uOld], %[uVal]\n\t",
                                           [uVal] "r" (u32));
    return u32OldRet;

#else
    return ASMAtomicXorExU32(pu32, u32); /* (we have no unordered cmpxchg primitive atm.) */
#endif
}


/**
 * Atomically XOR a signed 32-bit value, unordered.
 *
 * @param   pi32   Pointer to the variable to XOR @a u32 with.
 * @param   i32    The value to XOR @a *pu32 with.
 *
 * @remarks x86: Requires a 386 or later.
 */
DECLINLINE(void) ASMAtomicUoXorS32(int32_t volatile RT_FAR *pi32, int32_t i32) RT_NOTHROW_DEF
{
    ASMAtomicUoXorU32((uint32_t volatile RT_FAR *)pi32, (uint32_t)i32);
}


/**
 * Atomically increment an unsigned 32-bit value, unordered.
 *
 * @returns the new value.
 * @param   pu32   Pointer to the variable to increment.
 *
 * @remarks x86: Requires a 486 or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM
RT_ASM_DECL_PRAGMA_WATCOM(uint32_t) ASMAtomicUoIncU32(uint32_t volatile RT_FAR *pu32) RT_NOTHROW_PROTO;
#else
DECLINLINE(uint32_t) ASMAtomicUoIncU32(uint32_t volatile RT_FAR *pu32) RT_NOTHROW_DEF
{
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
    uint32_t u32;
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("xaddl %0, %1\n\t"
                         : "=r" (u32)
                         , "=m" (*pu32)
                         : "0" (1)
                         , "m" (*pu32)
                         : "memory" /** @todo why 'memory'? */
                         , "cc");
    return u32 + 1;
#  else
    __asm
    {
        mov     eax, 1
#   ifdef RT_ARCH_AMD64
        mov     rdx, [pu32]
        xadd    [rdx], eax
#   else
        mov     edx, [pu32]
        xadd    [edx], eax
#   endif
        mov     u32, eax
    }
    return u32 + 1;
#  endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_NEW_32(ASMAtomicUoIncU32, pu32, NO_BARRIER,
                                           "add %w[uNew], %w[uNew], #1\n\t",
                                           "add %[uNew], %[uNew], #1\n\t" /* arm6 / thumb2+ */,
                                           "X" (0) /* dummy */);
    return u32NewRet;

# else
#  error "Port me"
# endif
}
#endif


/**
 * Atomically decrement an unsigned 32-bit value, unordered.
 *
 * @returns the new value.
 * @param   pu32   Pointer to the variable to decrement.
 *
 * @remarks x86: Requires a 486 or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM
RT_ASM_DECL_PRAGMA_WATCOM(uint32_t) ASMAtomicUoDecU32(uint32_t volatile RT_FAR *pu32) RT_NOTHROW_PROTO;
#else
DECLINLINE(uint32_t) ASMAtomicUoDecU32(uint32_t volatile RT_FAR *pu32) RT_NOTHROW_DEF
{
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
    uint32_t u32;
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("lock; xaddl %0, %1\n\t"
                         : "=r" (u32)
                         , "=m" (*pu32)
                         : "0" (-1)
                         , "m" (*pu32)
                         : "memory"
                         , "cc");
    return u32 - 1;
#  else
    __asm
    {
        mov     eax, -1
#   ifdef RT_ARCH_AMD64
        mov     rdx, [pu32]
        xadd    [rdx], eax
#   else
        mov     edx, [pu32]
        xadd    [edx], eax
#   endif
        mov     u32, eax
    }
    return u32 - 1;
#  endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    RTASM_ARM_LOAD_MODIFY_STORE_RET_NEW_32(ASMAtomicUoDecU32, pu32, NO_BARRIER,
                                           "sub %w[uNew], %w[uNew], #1\n\t",
                                           "sub %[uNew], %[uNew], #1\n\t" /* arm6 / thumb2+ */,
                                           "X" (0) /* dummy */);
    return u32NewRet;

# else
#  error "Port me"
# endif
}
#endif


/** @def RT_ASM_PAGE_SIZE
 * We try avoid dragging in iprt/param.h here.
 * @internal
 */
#if defined(RT_ARCH_SPARC64)
# define RT_ASM_PAGE_SIZE   0x2000
# if defined(PAGE_SIZE) && !defined(NT_INCLUDED)
#  if PAGE_SIZE != 0x2000
#   error "PAGE_SIZE is not 0x2000!"
#  endif
# endif
#elif defined(RT_ARCH_ARM64)
# define RT_ASM_PAGE_SIZE   0x4000
# if defined(PAGE_SIZE) && !defined(NT_INCLUDED) && !defined(_MACH_ARM_VM_PARAM_H_)
#  if PAGE_SIZE != 0x4000
#   error "PAGE_SIZE is not 0x4000!"
#  endif
# endif
#else
# define RT_ASM_PAGE_SIZE   0x1000
# if defined(PAGE_SIZE) && !defined(NT_INCLUDED)
#  if PAGE_SIZE != 0x1000
#   error "PAGE_SIZE is not 0x1000!"
#  endif
# endif
#endif

/**
 * Zeros a 4K memory page.
 *
 * @param   pv  Pointer to the memory block. This must be page aligned.
 */
#if (RT_INLINE_ASM_EXTERNAL && !RT_INLINE_ASM_USES_INTRIN) || (!defined(RT_ARCH_AMD64) && !defined(RT_ARCH_X86))
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMMemZeroPage(volatile void RT_FAR *pv) RT_NOTHROW_PROTO;
# else
DECLINLINE(void) ASMMemZeroPage(volatile void RT_FAR *pv) RT_NOTHROW_DEF
{
#  if RT_INLINE_ASM_USES_INTRIN
#   ifdef RT_ARCH_AMD64
    __stosq((unsigned __int64 *)pv, 0, RT_ASM_PAGE_SIZE / 8);
#   else
    __stosd((unsigned long *)pv, 0, RT_ASM_PAGE_SIZE / 4);
#   endif

#  elif RT_INLINE_ASM_GNU_STYLE
    RTCCUINTREG uDummy;
#   ifdef RT_ARCH_AMD64
    __asm__ __volatile__("rep stosq"
                         : "=D" (pv),
                           "=c" (uDummy)
                         : "0" (pv),
                           "c" (RT_ASM_PAGE_SIZE >> 3),
                           "a" (0)
                         : "memory");
#   else
    __asm__ __volatile__("rep stosl"
                         : "=D" (pv),
                           "=c" (uDummy)
                         : "0" (pv),
                           "c" (RT_ASM_PAGE_SIZE >> 2),
                           "a" (0)
                         : "memory");
#   endif
#  else
    __asm
    {
#   ifdef RT_ARCH_AMD64
        xor     rax, rax
        mov     ecx, 0200h
        mov     rdi, [pv]
        rep     stosq
#   else
        xor     eax, eax
        mov     ecx, 0400h
        mov     edi, [pv]
        rep     stosd
#   endif
    }
#  endif
}
# endif


/**
 * Zeros a memory block with a 32-bit aligned size.
 *
 * @param   pv  Pointer to the memory block.
 * @param   cb  Number of bytes in the block. This MUST be aligned on 32-bit!
 */
#if (RT_INLINE_ASM_EXTERNAL && !RT_INLINE_ASM_USES_INTRIN) || (!defined(RT_ARCH_AMD64) && !defined(RT_ARCH_X86))
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMMemZero32(volatile void RT_FAR *pv, size_t cb) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMMemZero32(volatile void RT_FAR *pv, size_t cb) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
#  ifdef RT_ARCH_AMD64
    if (!(cb & 7))
        __stosq((unsigned __int64 RT_FAR *)pv, 0, cb / 8);
    else
#  endif
        __stosd((unsigned long RT_FAR *)pv, 0, cb / 4);

# elif RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("rep stosl"
                         : "=D" (pv),
                           "=c" (cb)
                         : "0" (pv),
                           "1" (cb >> 2),
                           "a" (0)
                         : "memory");
# else
    __asm
    {
        xor     eax, eax
#  ifdef RT_ARCH_AMD64
        mov     rcx, [cb]
        shr     rcx, 2
        mov     rdi, [pv]
#  else
        mov     ecx, [cb]
        shr     ecx, 2
        mov     edi, [pv]
#  endif
        rep stosd
    }
# endif
}
#endif


/**
 * Fills a memory block with a 32-bit aligned size.
 *
 * @param   pv  Pointer to the memory block.
 * @param   cb  Number of bytes in the block. This MUST be aligned on 32-bit!
 * @param   u32 The value to fill with.
 */
#if (RT_INLINE_ASM_EXTERNAL && !RT_INLINE_ASM_USES_INTRIN) || (!defined(RT_ARCH_AMD64) && !defined(RT_ARCH_X86))
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMMemFill32(volatile void RT_FAR *pv, size_t cb, uint32_t u32) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMMemFill32(volatile void RT_FAR *pv, size_t cb, uint32_t u32) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
#  ifdef RT_ARCH_AMD64
    if (!(cb & 7))
        __stosq((unsigned __int64 RT_FAR *)pv, RT_MAKE_U64(u32, u32), cb / 8);
    else
#  endif
        __stosd((unsigned long RT_FAR *)pv, u32, cb / 4);

# elif RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("rep stosl"
                         : "=D" (pv),
                           "=c" (cb)
                         : "0" (pv),
                           "1" (cb >> 2),
                           "a" (u32)
                         : "memory");
# else
    __asm
    {
#  ifdef RT_ARCH_AMD64
        mov     rcx, [cb]
        shr     rcx, 2
        mov     rdi, [pv]
#  else
        mov     ecx, [cb]
        shr     ecx, 2
        mov     edi, [pv]
#  endif
        mov     eax, [u32]
        rep stosd
    }
# endif
}
#endif


/**
 * Checks if a memory block is all zeros.
 *
 * @returns Pointer to the first non-zero byte.
 * @returns NULL if all zero.
 *
 * @param   pv      Pointer to the memory block.
 * @param   cb      Number of bytes in the block.
 */
#if !defined(RDESKTOP) && (!defined(RT_OS_LINUX) || !defined(__KERNEL__))
DECLASM(void RT_FAR *) ASMMemFirstNonZero(void const RT_FAR *pv, size_t cb) RT_NOTHROW_PROTO;
#else
DECLINLINE(void RT_FAR *) ASMMemFirstNonZero(void const RT_FAR *pv, size_t cb) RT_NOTHROW_DEF
{
/** @todo replace with ASMMemFirstNonZero-generic.cpp in kernel modules. */
    uint8_t const *pb = (uint8_t const RT_FAR *)pv;
    for (; cb; cb--, pb++)
        if (RT_LIKELY(*pb == 0))
        { /* likely */ }
        else
            return (void RT_FAR *)pb;
    return NULL;
}
#endif


/**
 * Checks if a memory block is all zeros.
 *
 * @returns true if zero, false if not.
 *
 * @param   pv      Pointer to the memory block.
 * @param   cb      Number of bytes in the block.
 *
 * @sa      ASMMemFirstNonZero
 */
DECLINLINE(bool) ASMMemIsZero(void const RT_FAR *pv, size_t cb) RT_NOTHROW_DEF
{
    return ASMMemFirstNonZero(pv, cb) == NULL;
}


/**
 * Checks if a memory page is all zeros.
 *
 * @returns true / false.
 *
 * @param   pvPage      Pointer to the page.  Must be aligned on 16 byte
 *                      boundary
 */
DECLINLINE(bool) ASMMemIsZeroPage(void const RT_FAR *pvPage) RT_NOTHROW_DEF
{
# if 0 /*RT_INLINE_ASM_GNU_STYLE - this is actually slower... */
    union { RTCCUINTREG r; bool f; } uAX;
    RTCCUINTREG xCX, xDI;
   Assert(!((uintptr_t)pvPage & 15));
    __asm__ __volatile__("repe; "
#  ifdef RT_ARCH_AMD64
                         "scasq\n\t"
#  else
                         "scasl\n\t"
#  endif
                         "setnc %%al\n\t"
                         : "=&c" (xCX)
                         , "=&D" (xDI)
                         , "=&a" (uAX.r)
                         : "mr" (pvPage)
#  ifdef RT_ARCH_AMD64
                         , "0" (RT_ASM_PAGE_SIZE/8)
#  else
                         , "0" (RT_ASM_PAGE_SIZE/4)
#  endif
                         , "1" (pvPage)
                         , "2" (0)
                         : "cc");
    return uAX.f;
# else
   uintptr_t const RT_FAR *puPtr = (uintptr_t const RT_FAR *)pvPage;
   size_t                  cLeft = RT_ASM_PAGE_SIZE / sizeof(uintptr_t) / 8;
   Assert(!((uintptr_t)pvPage & 15));
   for (;;)
   {
       if (puPtr[0])        return false;
       if (puPtr[4])        return false;

       if (puPtr[2])        return false;
       if (puPtr[6])        return false;

       if (puPtr[1])        return false;
       if (puPtr[5])        return false;

       if (puPtr[3])        return false;
       if (puPtr[7])        return false;

       if (!--cLeft)
           return true;
       puPtr += 8;
   }
# endif
}


/**
 * Checks if a memory block is filled with the specified byte, returning the
 * first mismatch.
 *
 * This is sort of an inverted memchr.
 *
 * @returns Pointer to the byte which doesn't equal u8.
 * @returns NULL if all equal to u8.
 *
 * @param   pv      Pointer to the memory block.
 * @param   cb      Number of bytes in the block.
 * @param   u8      The value it's supposed to be filled with.
 *
 * @remarks No alignment requirements.
 */
#if    (!defined(RT_OS_LINUX) || !defined(__KERNEL__)) \
    && (!defined(RT_OS_FREEBSD) || !defined(_KERNEL))
DECLASM(void *) ASMMemFirstMismatchingU8(void const RT_FAR *pv, size_t cb, uint8_t u8) RT_NOTHROW_PROTO;
#else
DECLINLINE(void *) ASMMemFirstMismatchingU8(void const RT_FAR *pv, size_t cb, uint8_t u8) RT_NOTHROW_DEF
{
/** @todo replace with ASMMemFirstMismatchingU8-generic.cpp in kernel modules. */
    uint8_t const *pb = (uint8_t const RT_FAR *)pv;
    for (; cb; cb--, pb++)
        if (RT_LIKELY(*pb == u8))
        { /* likely */ }
        else
            return (void *)pb;
    return NULL;
}
#endif


/**
 * Checks if a memory block is filled with the specified byte.
 *
 * @returns true if all matching, false if not.
 *
 * @param   pv      Pointer to the memory block.
 * @param   cb      Number of bytes in the block.
 * @param   u8      The value it's supposed to be filled with.
 *
 * @remarks No alignment requirements.
 */
DECLINLINE(bool) ASMMemIsAllU8(void const RT_FAR *pv, size_t cb, uint8_t u8) RT_NOTHROW_DEF
{
    return ASMMemFirstMismatchingU8(pv, cb, u8) == NULL;
}


/**
 * Checks if a memory block is filled with the specified 32-bit value.
 *
 * This is a sort of inverted memchr.
 *
 * @returns Pointer to the first value which doesn't equal u32.
 * @returns NULL if all equal to u32.
 *
 * @param   pv      Pointer to the memory block.
 * @param   cb      Number of bytes in the block. This MUST be aligned on 32-bit!
 * @param   u32     The value it's supposed to be filled with.
 */
DECLINLINE(uint32_t RT_FAR *) ASMMemFirstMismatchingU32(void const RT_FAR *pv, size_t cb, uint32_t u32) RT_NOTHROW_DEF
{
/** @todo rewrite this in inline assembly? */
    uint32_t const RT_FAR *pu32 = (uint32_t const RT_FAR *)pv;
    for (; cb; cb -= 4, pu32++)
        if (RT_LIKELY(*pu32 == u32))
        { /* likely */ }
        else
            return (uint32_t RT_FAR *)pu32;
    return NULL;
}


/**
 * Probes a byte pointer for read access.
 *
 * While the function will not fault if the byte is not read accessible,
 * the idea is to do this in a safe place like before acquiring locks
 * and such like.
 *
 * Also, this functions guarantees that an eager compiler is not going
 * to optimize the probing away.
 *
 * @param   pvByte      Pointer to the byte.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM
RT_ASM_DECL_PRAGMA_WATCOM(uint8_t) ASMProbeReadByte(const void RT_FAR *pvByte) RT_NOTHROW_PROTO;
#else
DECLINLINE(uint8_t) ASMProbeReadByte(const void RT_FAR *pvByte) RT_NOTHROW_DEF
{
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
    uint8_t u8;
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("movb %1, %0\n\t"
                         : "=q" (u8)
                         : "m" (*(const uint8_t *)pvByte));
#  else
    __asm
    {
#   ifdef RT_ARCH_AMD64
        mov     rax, [pvByte]
        mov     al, [rax]
#   else
        mov     eax, [pvByte]
        mov     al, [eax]
#   endif
        mov     [u8], al
    }
#  endif
    return u8;

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    uint32_t u32;
    __asm__ __volatile__(".Lstart_ASMProbeReadByte_%=:\n\t"
#  if defined(RT_ARCH_ARM64)
                         "ldxrb     %w[uDst], %[pMem]\n\t"
#  else
                         "ldrexb    %[uDst], %[pMem]\n\t"
#  endif
                         : [uDst] "=&r" (u32)
                         : [pMem] "Q" (*(uint8_t const *)pvByte));
    return (uint8_t)u32;

# else
#  error "Port me"
# endif
}
#endif

/**
 * Probes a buffer for read access page by page.
 *
 * While the function will fault if the buffer is not fully read
 * accessible, the idea is to do this in a safe place like before
 * acquiring locks and such like.
 *
 * Also, this functions guarantees that an eager compiler is not going
 * to optimize the probing away.
 *
 * @param   pvBuf       Pointer to the buffer.
 * @param   cbBuf       The size of the buffer in bytes. Must be >= 1.
 */
DECLINLINE(void) ASMProbeReadBuffer(const void RT_FAR *pvBuf, size_t cbBuf) RT_NOTHROW_DEF
{
    /** @todo verify that the compiler actually doesn't optimize this away. (intel & gcc) */
    /* the first byte */
    const uint8_t RT_FAR *pu8 = (const uint8_t RT_FAR *)pvBuf;
    ASMProbeReadByte(pu8);

    /* the pages in between pages. */
    while (cbBuf > RT_ASM_PAGE_SIZE)
    {
        ASMProbeReadByte(pu8);
        cbBuf -= RT_ASM_PAGE_SIZE;
        pu8   += RT_ASM_PAGE_SIZE;
    }

    /* the last byte */
    ASMProbeReadByte(pu8 + cbBuf - 1);
}


/**
 * Reverse the byte order of the given 16-bit integer.
 *
 * @returns Revert
 * @param   u16     16-bit integer value.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(uint16_t) ASMByteSwapU16(uint16_t u16) RT_NOTHROW_PROTO;
#else
DECLINLINE(uint16_t) ASMByteSwapU16(uint16_t u16) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    return _byteswap_ushort(u16);

# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ ("rorw $8, %0" : "=r" (u16) : "0" (u16) : "cc");
#  else
    _asm
    {
        mov     ax, [u16]
        ror     ax, 8
        mov     [u16], ax
    }
#  endif
    return u16;

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    uint32_t u32Ret;
    __asm__ __volatile__(
#  if defined(RT_ARCH_ARM64)
                         "rev16     %w[uRet], %w[uVal]\n\t"
#  else
                         "rev16     %[uRet], %[uVal]\n\t"
#  endif
                         : [uRet] "=r" (u32Ret)
                         : [uVal] "r" (u16));
    return (uint16_t)u32Ret;

# else
#  error "Port me"
# endif
}
#endif


/**
 * Reverse the byte order of the given 32-bit integer.
 *
 * @returns Revert
 * @param   u32     32-bit integer value.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(uint32_t) ASMByteSwapU32(uint32_t u32) RT_NOTHROW_PROTO;
#else
DECLINLINE(uint32_t) ASMByteSwapU32(uint32_t u32) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    return _byteswap_ulong(u32);

# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ ("bswapl %0" : "=r" (u32) : "0" (u32));
#  else
    _asm
    {
        mov     eax, [u32]
        bswap   eax
        mov     [u32], eax
    }
#  endif
    return u32;

# elif defined(RT_ARCH_ARM64)
    uint64_t u64Ret;
    __asm__ __volatile__("rev32     %[uRet], %[uVal]\n\t"
                         : [uRet] "=r" (u64Ret)
                         : [uVal] "r" ((uint64_t)u32));
    return (uint32_t)u64Ret;

# elif defined(RT_ARCH_ARM32)
    __asm__ __volatile__("rev       %[uRet], %[uVal]\n\t"
                         : [uRet] "=r" (u32)
                         : [uVal] "[uRet]" (u32));
    return u32;

# else
#  error "Port me"
# endif
}
#endif


/**
 * Reverse the byte order of the given 64-bit integer.
 *
 * @returns Revert
 * @param   u64     64-bit integer value.
 */
DECLINLINE(uint64_t) ASMByteSwapU64(uint64_t u64) RT_NOTHROW_DEF
{
#if defined(RT_ARCH_AMD64) && RT_INLINE_ASM_USES_INTRIN
    return _byteswap_uint64(u64);

# elif RT_INLINE_ASM_GNU_STYLE && defined(RT_ARCH_AMD64)
    __asm__ ("bswapq %0" : "=r" (u64) : "0" (u64));
    return u64;

# elif defined(RT_ARCH_ARM64)
    __asm__ __volatile__("rev       %[uRet], %[uVal]\n\t"
                         : [uRet] "=r" (u64)
                         : [uVal] "[uRet]" (u64));
    return u64;

#else
    return (uint64_t)ASMByteSwapU32((uint32_t)u64) << 32
         | (uint64_t)ASMByteSwapU32((uint32_t)(u64 >> 32));
#endif
}



/** @defgroup grp_inline_bits   Bit Operations
 * @{
 */


/**
 * Sets a bit in a bitmap.
 *
 * @param   pvBitmap    Pointer to the bitmap (little endian).  This should be
 *                      32-bit aligned.
 * @param   iBit        The bit to set.
 *
 * @remarks The 32-bit aligning of pvBitmap is not a strict requirement.
 *          However, doing so will yield better performance as well as avoiding
 *          traps accessing the last bits in the bitmap.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMBitSet(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMBitSet(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    _bittestandset((long RT_FAR *)pvBitmap, iBit);

# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("btsl %1, %0"
                         : "=m" (*(volatile long RT_FAR *)pvBitmap)
                         : "Ir" (iBit)
                         , "m" (*(volatile long RT_FAR *)pvBitmap)
                         : "memory"
                         , "cc");
#  else
    __asm
    {
#   ifdef RT_ARCH_AMD64
        mov     rax, [pvBitmap]
        mov     edx, [iBit]
        bts     [rax], edx
#   else
        mov     eax, [pvBitmap]
        mov     edx, [iBit]
        bts     [eax], edx
#   endif
    }
#  endif

# else
    int32_t offBitmap = iBit / 32;
    AssertStmt(!((uintptr_t)pvBitmap & 3), offBitmap += (uintptr_t)pvBitmap & 3; iBit += ((uintptr_t)pvBitmap & 3) * 8);
    ASMAtomicUoOrU32(&((uint32_t volatile *)pvBitmap)[offBitmap], RT_H2LE_U32(RT_BIT_32(iBit & 31)));
# endif
}
#endif


/**
 * Atomically sets a bit in a bitmap, ordered.
 *
 * @param   pvBitmap    Pointer to the bitmap (little endian).  Must be 32-bit
 *                      aligned, otherwise the memory access isn't atomic!
 * @param   iBit        The bit to set.
 *
 * @remarks x86: Requires a 386 or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMAtomicBitSet(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMAtomicBitSet(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_DEF
{
    AssertMsg(!((uintptr_t)pvBitmap & 3), ("address %p not 32-bit aligned", pvBitmap));
# if RT_INLINE_ASM_USES_INTRIN
    _interlockedbittestandset((long RT_FAR *)pvBitmap, iBit);
# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("lock; btsl %1, %0"
                         : "=m" (*(volatile long *)pvBitmap)
                         : "Ir" (iBit)
                         , "m" (*(volatile long *)pvBitmap)
                         : "memory"
                         , "cc");
#  else
    __asm
    {
#   ifdef RT_ARCH_AMD64
        mov     rax, [pvBitmap]
        mov     edx, [iBit]
        lock bts [rax], edx
#   else
        mov     eax, [pvBitmap]
        mov     edx, [iBit]
        lock bts [eax], edx
#   endif
    }
#  endif

# else
    ASMAtomicOrU32(&((uint32_t volatile *)pvBitmap)[iBit / 32], RT_H2LE_U32(RT_BIT_32(iBit & 31)));
# endif
}
#endif


/**
 * Clears a bit in a bitmap.
 *
 * @param   pvBitmap    Pointer to the bitmap (little endian).
 * @param   iBit        The bit to clear.
 *
 * @remarks The 32-bit aligning of pvBitmap is not a strict requirement.
 *          However, doing so will yield better performance as well as avoiding
 *          traps accessing the last bits in the bitmap.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMBitClear(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMBitClear(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    _bittestandreset((long RT_FAR *)pvBitmap, iBit);

# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("btrl %1, %0"
                         : "=m" (*(volatile long RT_FAR *)pvBitmap)
                         : "Ir" (iBit)
                         , "m" (*(volatile long RT_FAR *)pvBitmap)
                         : "memory"
                         , "cc");
#  else
    __asm
    {
#   ifdef RT_ARCH_AMD64
        mov     rax, [pvBitmap]
        mov     edx, [iBit]
        btr     [rax], edx
#   else
        mov     eax, [pvBitmap]
        mov     edx, [iBit]
        btr     [eax], edx
#   endif
    }
#  endif

# else
    int32_t offBitmap = iBit / 32;
    AssertStmt(!((uintptr_t)pvBitmap & 3), offBitmap += (uintptr_t)pvBitmap & 3; iBit += ((uintptr_t)pvBitmap & 3) * 8);
    ASMAtomicUoAndU32(&((uint32_t volatile *)pvBitmap)[offBitmap], RT_H2LE_U32(~RT_BIT_32(iBit & 31)));
# endif
}
#endif


/**
 * Atomically clears a bit in a bitmap, ordered.
 *
 * @param   pvBitmap    Pointer to the bitmap (little endian).  Must be 32-bit
 *                      aligned, otherwise the memory access isn't atomic!
 * @param   iBit        The bit to toggle set.
 *
 * @remarks No memory barrier, take care on smp.
 * @remarks x86: Requires a 386 or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMAtomicBitClear(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMAtomicBitClear(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_DEF
{
    AssertMsg(!((uintptr_t)pvBitmap & 3), ("address %p not 32-bit aligned", pvBitmap));
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("lock; btrl %1, %0"
                         : "=m" (*(volatile long RT_FAR *)pvBitmap)
                         : "Ir" (iBit)
                         , "m" (*(volatile long RT_FAR *)pvBitmap)
                         : "memory"
                         , "cc");
#  else
    __asm
    {
#   ifdef RT_ARCH_AMD64
        mov     rax, [pvBitmap]
        mov     edx, [iBit]
        lock btr [rax], edx
#   else
        mov     eax, [pvBitmap]
        mov     edx, [iBit]
        lock btr [eax], edx
#   endif
    }
#  endif
# else
    ASMAtomicAndU32(&((uint32_t volatile *)pvBitmap)[iBit / 32], RT_H2LE_U32(~RT_BIT_32(iBit & 31)));
# endif
}
#endif


/**
 * Toggles a bit in a bitmap.
 *
 * @param   pvBitmap    Pointer to the bitmap (little endian).
 * @param   iBit        The bit to toggle.
 *
 * @remarks The 32-bit aligning of pvBitmap is not a strict requirement.
 *          However, doing so will yield better performance as well as avoiding
 *          traps accessing the last bits in the bitmap.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMBitToggle(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMBitToggle(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    _bittestandcomplement((long RT_FAR *)pvBitmap, iBit);
# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("btcl %1, %0"
                         : "=m" (*(volatile long *)pvBitmap)
                         : "Ir" (iBit)
                         , "m" (*(volatile long *)pvBitmap)
                         : "memory"
                         , "cc");
#  else
    __asm
    {
#   ifdef RT_ARCH_AMD64
        mov     rax, [pvBitmap]
        mov     edx, [iBit]
        btc     [rax], edx
#   else
        mov     eax, [pvBitmap]
        mov     edx, [iBit]
        btc     [eax], edx
#   endif
    }
#  endif
# else
    int32_t offBitmap = iBit / 32;
    AssertStmt(!((uintptr_t)pvBitmap & 3), offBitmap += (uintptr_t)pvBitmap & 3; iBit += ((uintptr_t)pvBitmap & 3) * 8);
    ASMAtomicUoXorU32(&((uint32_t volatile *)pvBitmap)[offBitmap], RT_H2LE_U32(RT_BIT_32(iBit & 31)));
# endif
}
#endif


/**
 * Atomically toggles a bit in a bitmap, ordered.
 *
 * @param   pvBitmap    Pointer to the bitmap (little endian).  Must be 32-bit
 *                      aligned, otherwise the memory access isn't atomic!
 * @param   iBit        The bit to test and set.
 *
 * @remarks x86: Requires a 386 or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM
RT_ASM_DECL_PRAGMA_WATCOM(void) ASMAtomicBitToggle(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_PROTO;
#else
DECLINLINE(void) ASMAtomicBitToggle(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_DEF
{
    AssertMsg(!((uintptr_t)pvBitmap & 3), ("address %p not 32-bit aligned", pvBitmap));
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("lock; btcl %1, %0"
                         : "=m" (*(volatile long RT_FAR *)pvBitmap)
                         : "Ir" (iBit)
                         , "m" (*(volatile long RT_FAR *)pvBitmap)
                         : "memory"
                         , "cc");
#  else
    __asm
    {
#   ifdef RT_ARCH_AMD64
        mov     rax, [pvBitmap]
        mov     edx, [iBit]
        lock btc [rax], edx
#   else
        mov     eax, [pvBitmap]
        mov     edx, [iBit]
        lock btc [eax], edx
#   endif
    }
#  endif
# else
    ASMAtomicXorU32(&((uint32_t volatile *)pvBitmap)[iBit / 32], RT_H2LE_U32(RT_BIT_32(iBit & 31)));
# endif
}
#endif


/**
 * Tests and sets a bit in a bitmap.
 *
 * @returns true if the bit was set.
 * @returns false if the bit was clear.
 *
 * @param   pvBitmap    Pointer to the bitmap (little endian).
 * @param   iBit        The bit to test and set.
 *
 * @remarks The 32-bit aligning of pvBitmap is not a strict requirement.
 *          However, doing so will yield better performance as well as avoiding
 *          traps accessing the last bits in the bitmap.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(bool) ASMBitTestAndSet(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_PROTO;
#else
DECLINLINE(bool) ASMBitTestAndSet(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_DEF
{
    union { bool f; uint32_t u32; uint8_t u8; } rc;
# if RT_INLINE_ASM_USES_INTRIN
    rc.u8 = _bittestandset((long RT_FAR *)pvBitmap, iBit);

# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("btsl %2, %1\n\t"
                         "setc %b0\n\t"
                         "andl $1, %0\n\t"
                         : "=q" (rc.u32)
                         , "=m" (*(volatile long RT_FAR *)pvBitmap)
                         : "Ir" (iBit)
                         , "m" (*(volatile long RT_FAR *)pvBitmap)
                         : "memory"
                         , "cc");
#  else
    __asm
    {
        mov     edx, [iBit]
#   ifdef RT_ARCH_AMD64
        mov     rax, [pvBitmap]
        bts     [rax], edx
#   else
        mov     eax, [pvBitmap]
        bts     [eax], edx
#   endif
        setc    al
        and     eax, 1
        mov     [rc.u32], eax
    }
#  endif

# else
    int32_t offBitmap = iBit / 32;
    AssertStmt(!((uintptr_t)pvBitmap & 3), offBitmap += (uintptr_t)pvBitmap & 3; iBit += ((uintptr_t)pvBitmap & 3) * 8);
    rc.u32 = RT_LE2H_U32(ASMAtomicUoOrExU32(&((uint32_t volatile *)pvBitmap)[offBitmap], RT_H2LE_U32(RT_BIT_32(iBit & 31))))
          >> (iBit & 31);
    rc.u32 &= 1;
# endif
    return rc.f;
}
#endif


/**
 * Atomically tests and sets a bit in a bitmap, ordered.
 *
 * @returns true if the bit was set.
 * @returns false if the bit was clear.
 *
 * @param   pvBitmap    Pointer to the bitmap (little endian).  Must be 32-bit
 *                      aligned, otherwise the memory access isn't atomic!
 * @param   iBit        The bit to set.
 *
 * @remarks x86: Requires a 386 or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(bool) ASMAtomicBitTestAndSet(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_PROTO;
#else
DECLINLINE(bool) ASMAtomicBitTestAndSet(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_DEF
{
    union { bool f; uint32_t u32; uint8_t u8; } rc;
    AssertMsg(!((uintptr_t)pvBitmap & 3), ("address %p not 32-bit aligned", pvBitmap));
# if RT_INLINE_ASM_USES_INTRIN
    rc.u8 = _interlockedbittestandset((long RT_FAR *)pvBitmap, iBit);
# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("lock; btsl %2, %1\n\t"
                         "setc %b0\n\t"
                         "andl $1, %0\n\t"
                         : "=q" (rc.u32)
                         , "=m" (*(volatile long RT_FAR *)pvBitmap)
                         : "Ir" (iBit)
                         , "m" (*(volatile long RT_FAR *)pvBitmap)
                         : "memory"
                         , "cc");
#  else
    __asm
    {
        mov     edx, [iBit]
#   ifdef RT_ARCH_AMD64
        mov     rax, [pvBitmap]
        lock bts [rax], edx
#   else
        mov     eax, [pvBitmap]
        lock bts [eax], edx
#   endif
        setc    al
        and     eax, 1
        mov     [rc.u32], eax
    }
#  endif

# else
    rc.u32 = RT_LE2H_U32(ASMAtomicOrExU32(&((uint32_t volatile *)pvBitmap)[iBit / 32], RT_H2LE_U32(RT_BIT_32(iBit & 31))))
          >> (iBit & 31);
    rc.u32 &= 1;
# endif
    return rc.f;
}
#endif


/**
 * Tests and clears a bit in a bitmap.
 *
 * @returns true if the bit was set.
 * @returns false if the bit was clear.
 *
 * @param   pvBitmap    Pointer to the bitmap (little endian).
 * @param   iBit        The bit to test and clear.
 *
 * @remarks The 32-bit aligning of pvBitmap is not a strict requirement.
 *          However, doing so will yield better performance as well as avoiding
 *          traps accessing the last bits in the bitmap.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(bool) ASMBitTestAndClear(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_PROTO;
#else
DECLINLINE(bool) ASMBitTestAndClear(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_DEF
{
    union { bool f; uint32_t u32; uint8_t u8; } rc;
# if RT_INLINE_ASM_USES_INTRIN
    rc.u8 = _bittestandreset((long RT_FAR *)pvBitmap, iBit);

# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("btrl %2, %1\n\t"
                         "setc %b0\n\t"
                         "andl $1, %0\n\t"
                         : "=q" (rc.u32)
                         , "=m" (*(volatile long RT_FAR *)pvBitmap)
                         : "Ir" (iBit)
                         , "m" (*(volatile long RT_FAR *)pvBitmap)
                         : "memory"
                         , "cc");
#  else
    __asm
    {
        mov     edx, [iBit]
#   ifdef RT_ARCH_AMD64
        mov     rax, [pvBitmap]
        btr     [rax], edx
#   else
        mov     eax, [pvBitmap]
        btr     [eax], edx
#   endif
        setc    al
        and     eax, 1
        mov     [rc.u32], eax
    }
#  endif

# else
    int32_t offBitmap = iBit / 32;
    AssertStmt(!((uintptr_t)pvBitmap & 3), offBitmap += (uintptr_t)pvBitmap & 3; iBit += ((uintptr_t)pvBitmap & 3) * 8);
    rc.u32 = RT_LE2H_U32(ASMAtomicUoAndExU32(&((uint32_t volatile *)pvBitmap)[offBitmap], RT_H2LE_U32(~RT_BIT_32(iBit & 31))))
          >> (iBit & 31);
    rc.u32 &= 1;
# endif
    return rc.f;
}
#endif


/**
 * Atomically tests and clears a bit in a bitmap, ordered.
 *
 * @returns true if the bit was set.
 * @returns false if the bit was clear.
 *
 * @param   pvBitmap    Pointer to the bitmap (little endian).  Must be 32-bit
 *                      aligned, otherwise the memory access isn't atomic!
 * @param   iBit        The bit to test and clear.
 *
 * @remarks No memory barrier, take care on smp.
 * @remarks x86: Requires a 386 or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(bool) ASMAtomicBitTestAndClear(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_PROTO;
#else
DECLINLINE(bool) ASMAtomicBitTestAndClear(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_DEF
{
    union { bool f; uint32_t u32; uint8_t u8; } rc;
    AssertMsg(!((uintptr_t)pvBitmap & 3), ("address %p not 32-bit aligned", pvBitmap));
# if RT_INLINE_ASM_USES_INTRIN
    rc.u8 = _interlockedbittestandreset((long RT_FAR *)pvBitmap, iBit);

# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("lock; btrl %2, %1\n\t"
                         "setc %b0\n\t"
                         "andl $1, %0\n\t"
                         : "=q" (rc.u32)
                         , "=m" (*(volatile long RT_FAR *)pvBitmap)
                         : "Ir" (iBit)
                         , "m" (*(volatile long RT_FAR *)pvBitmap)
                         : "memory"
                         , "cc");
#  else
    __asm
    {
        mov     edx, [iBit]
#   ifdef RT_ARCH_AMD64
        mov     rax, [pvBitmap]
        lock btr [rax], edx
#   else
        mov     eax, [pvBitmap]
        lock btr [eax], edx
#   endif
        setc    al
        and     eax, 1
        mov     [rc.u32], eax
    }
#  endif

# else
    rc.u32 = RT_LE2H_U32(ASMAtomicAndExU32(&((uint32_t volatile *)pvBitmap)[iBit / 32], RT_H2LE_U32(~RT_BIT_32(iBit & 31))))
          >> (iBit & 31);
    rc.u32 &= 1;
# endif
    return rc.f;
}
#endif


/**
 * Tests and toggles a bit in a bitmap.
 *
 * @returns true if the bit was set.
 * @returns false if the bit was clear.
 *
 * @param   pvBitmap    Pointer to the bitmap (little endian).
 * @param   iBit        The bit to test and toggle.
 *
 * @remarks The 32-bit aligning of pvBitmap is not a strict requirement.
 *          However, doing so will yield better performance as well as avoiding
 *          traps accessing the last bits in the bitmap.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(bool) ASMBitTestAndToggle(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_PROTO;
#else
DECLINLINE(bool) ASMBitTestAndToggle(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_DEF
{
    union { bool f; uint32_t u32; uint8_t u8; } rc;
# if RT_INLINE_ASM_USES_INTRIN
    rc.u8 = _bittestandcomplement((long RT_FAR *)pvBitmap, iBit);

# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("btcl %2, %1\n\t"
                         "setc %b0\n\t"
                         "andl $1, %0\n\t"
                         : "=q" (rc.u32)
                         , "=m" (*(volatile long RT_FAR *)pvBitmap)
                         : "Ir" (iBit)
                         , "m" (*(volatile long RT_FAR *)pvBitmap)
                         : "memory"
                         , "cc");
#  else
    __asm
    {
        mov   edx, [iBit]
#   ifdef RT_ARCH_AMD64
        mov   rax, [pvBitmap]
        btc   [rax], edx
#   else
        mov   eax, [pvBitmap]
        btc   [eax], edx
#   endif
        setc  al
        and   eax, 1
        mov   [rc.u32], eax
    }
#  endif

# else
    int32_t offBitmap = iBit / 32;
    AssertStmt(!((uintptr_t)pvBitmap & 3), offBitmap += (uintptr_t)pvBitmap & 3; iBit += ((uintptr_t)pvBitmap & 3) * 8);
    rc.u32 = RT_LE2H_U32(ASMAtomicUoXorExU32(&((uint32_t volatile *)pvBitmap)[offBitmap], RT_H2LE_U32(RT_BIT_32(iBit & 31))))
          >> (iBit & 31);
    rc.u32 &= 1;
# endif
    return rc.f;
}
#endif


/**
 * Atomically tests and toggles a bit in a bitmap, ordered.
 *
 * @returns true if the bit was set.
 * @returns false if the bit was clear.
 *
 * @param   pvBitmap    Pointer to the bitmap (little endian).  Must be 32-bit
 *                      aligned, otherwise the memory access isn't atomic!
 * @param   iBit        The bit to test and toggle.
 *
 * @remarks x86: Requires a 386 or later.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM
RT_ASM_DECL_PRAGMA_WATCOM(bool) ASMAtomicBitTestAndToggle(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_PROTO;
#else
DECLINLINE(bool) ASMAtomicBitTestAndToggle(volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_DEF
{
    union { bool f; uint32_t u32; uint8_t u8; } rc;
    AssertMsg(!((uintptr_t)pvBitmap & 3), ("address %p not 32-bit aligned", pvBitmap));
# if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("lock; btcl %2, %1\n\t"
                         "setc %b0\n\t"
                         "andl $1, %0\n\t"
                         : "=q" (rc.u32)
                         , "=m" (*(volatile long RT_FAR *)pvBitmap)
                         : "Ir" (iBit)
                         , "m" (*(volatile long RT_FAR *)pvBitmap)
                         : "memory"
                         , "cc");
#  else
    __asm
    {
        mov     edx, [iBit]
#   ifdef RT_ARCH_AMD64
        mov     rax, [pvBitmap]
        lock btc [rax], edx
#   else
        mov     eax, [pvBitmap]
        lock btc [eax], edx
#   endif
        setc    al
        and     eax, 1
        mov     [rc.u32], eax
    }
#  endif

# else
    rc.u32 = RT_H2LE_U32(ASMAtomicXorExU32(&((uint32_t volatile *)pvBitmap)[iBit / 32], RT_LE2H_U32(RT_BIT_32(iBit & 31))))
          >> (iBit & 31);
    rc.u32 &= 1;
# endif
    return rc.f;
}
#endif


/**
 * Tests if a bit in a bitmap is set.
 *
 * @returns true if the bit is set.
 * @returns false if the bit is clear.
 *
 * @param   pvBitmap    Pointer to the bitmap (little endian).
 * @param   iBit        The bit to test.
 *
 * @remarks The 32-bit aligning of pvBitmap is not a strict requirement.
 *          However, doing so will yield better performance as well as avoiding
 *          traps accessing the last bits in the bitmap.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM(bool) ASMBitTest(const volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_PROTO;
#else
DECLINLINE(bool) ASMBitTest(const volatile void RT_FAR *pvBitmap, int32_t iBit) RT_NOTHROW_DEF
{
    union { bool f; uint32_t u32; uint8_t u8; } rc;
# if RT_INLINE_ASM_USES_INTRIN
    rc.u32 = _bittest((long *)pvBitmap, iBit);

# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE

    __asm__ __volatile__("btl %2, %1\n\t"
                         "setc %b0\n\t"
                         "andl $1, %0\n\t"
                         : "=q" (rc.u32)
                         : "m" (*(const volatile long RT_FAR *)pvBitmap)
                         , "Ir" (iBit)
                         : "memory"
                         , "cc");
#  else
    __asm
    {
        mov   edx, [iBit]
#   ifdef RT_ARCH_AMD64
        mov   rax, [pvBitmap]
        bt    [rax], edx
#   else
        mov   eax, [pvBitmap]
        bt    [eax], edx
#   endif
        setc  al
        and   eax, 1
        mov   [rc.u32], eax
    }
#  endif

# else
    int32_t offBitmap = iBit / 32;
    AssertStmt(!((uintptr_t)pvBitmap & 3), offBitmap += (uintptr_t)pvBitmap & 3; iBit += ((uintptr_t)pvBitmap & 3) * 8);
    rc.u32 = RT_LE2H_U32(ASMAtomicUoReadU32(&((uint32_t volatile *)pvBitmap)[offBitmap])) >> (iBit & 31);
    rc.u32 &= 1;
# endif
    return rc.f;
}
#endif


/**
 * Clears a bit range within a bitmap.
 *
 * @param   pvBitmap    Pointer to the bitmap (little endian).
 * @param   iBitStart   The First bit to clear.
 * @param   iBitEnd     The first bit not to clear.
 */
DECLINLINE(void) ASMBitClearRange(volatile void RT_FAR *pvBitmap, size_t iBitStart, size_t iBitEnd) RT_NOTHROW_DEF
{
    if (iBitStart < iBitEnd)
    {
        uint32_t volatile RT_FAR *pu32    = (volatile uint32_t RT_FAR *)pvBitmap + (iBitStart >> 5);
        size_t                    iStart = iBitStart & ~(size_t)31;
        size_t                    iEnd   = iBitEnd   & ~(size_t)31;
        if (iStart == iEnd)
            *pu32 &= RT_H2LE_U32(((UINT32_C(1) << (iBitStart & 31)) - 1) | ~((UINT32_C(1) << (iBitEnd & 31)) - 1));
        else
        {
            /* bits in first dword. */
            if (iBitStart & 31)
            {
                *pu32 &= RT_H2LE_U32((UINT32_C(1) << (iBitStart & 31)) - 1);
                pu32++;
                iBitStart = iStart + 32;
            }

            /* whole dwords. */
            if (iBitStart != iEnd)
                ASMMemZero32(pu32, (iEnd - iBitStart) >> 3);

            /* bits in last dword. */
            if (iBitEnd & 31)
            {
                pu32 = (volatile uint32_t RT_FAR *)pvBitmap + (iBitEnd >> 5);
                *pu32 &= RT_H2LE_U32(~((UINT32_C(1) << (iBitEnd & 31)) - 1));
            }
        }
    }
}


/**
 * Sets a bit range within a bitmap.
 *
 * @param   pvBitmap    Pointer to the bitmap (little endian).
 * @param   iBitStart   The First bit to set.
 * @param   iBitEnd     The first bit not to set.
 */
DECLINLINE(void) ASMBitSetRange(volatile void RT_FAR *pvBitmap, size_t iBitStart, size_t iBitEnd) RT_NOTHROW_DEF
{
    if (iBitStart < iBitEnd)
    {
        uint32_t volatile RT_FAR *pu32   = (volatile uint32_t RT_FAR *)pvBitmap + (iBitStart >> 5);
        size_t                    iStart = iBitStart & ~(size_t)31;
        size_t                    iEnd   = iBitEnd   & ~(size_t)31;
        if (iStart == iEnd)
            *pu32 |= RT_H2LE_U32(((UINT32_C(1) << (iBitEnd - iBitStart)) - 1) << (iBitStart & 31));
        else
        {
            /* bits in first dword. */
            if (iBitStart & 31)
            {
                *pu32 |= RT_H2LE_U32(~((UINT32_C(1) << (iBitStart & 31)) - 1));
                pu32++;
                iBitStart = iStart + 32;
            }

            /* whole dword. */
            if (iBitStart != iEnd)
                ASMMemFill32(pu32, (iEnd - iBitStart) >> 3, ~UINT32_C(0));

            /* bits in last dword. */
            if (iBitEnd & 31)
            {
                pu32 = (volatile uint32_t RT_FAR *)pvBitmap + (iBitEnd >> 5);
                *pu32 |= RT_H2LE_U32((UINT32_C(1) << (iBitEnd & 31)) - 1);
            }
        }
    }
}


/**
 * Finds the first clear bit in a bitmap.
 *
 * @returns Index of the first zero bit.
 * @returns -1 if no clear bit was found.
 * @param   pvBitmap    Pointer to the bitmap (little endian).
 * @param   cBits       The number of bits in the bitmap. Multiple of 32.
 */
#if RT_INLINE_ASM_EXTERNAL || (!defined(RT_ARCH_AMD64) && !defined(RT_ARCH_X86))
DECLASM(int32_t) ASMBitFirstClear(const volatile void RT_FAR *pvBitmap, uint32_t cBits) RT_NOTHROW_PROTO;
#else
DECLINLINE(int32_t) ASMBitFirstClear(const volatile void RT_FAR *pvBitmap, uint32_t cBits) RT_NOTHROW_DEF
{
    if (cBits)
    {
        int32_t iBit;
# if RT_INLINE_ASM_GNU_STYLE
        RTCCUINTREG uEAX, uECX, uEDI;
        cBits = RT_ALIGN_32(cBits, 32);
        __asm__ __volatile__("repe; scasl\n\t"
                             "je    1f\n\t"
#  ifdef RT_ARCH_AMD64
                             "lea   -4(%%rdi), %%rdi\n\t"
                             "xorl  (%%rdi), %%eax\n\t"
                             "subq  %5, %%rdi\n\t"
#  else
                             "lea   -4(%%edi), %%edi\n\t"
                             "xorl  (%%edi), %%eax\n\t"
                             "subl  %5, %%edi\n\t"
#  endif
                             "shll  $3, %%edi\n\t"
                             "bsfl  %%eax, %%edx\n\t"
                             "addl  %%edi, %%edx\n\t"
                             "1:\t\n"
                             : "=d" (iBit)
                             , "=&c" (uECX)
                             , "=&D" (uEDI)
                             , "=&a" (uEAX)
                             : "0" (0xffffffff)
                             , "mr" (pvBitmap)
                             , "1" (cBits >> 5)
                             , "2" (pvBitmap)
                             , "3" (0xffffffff)
                             : "cc");
# else
        cBits = RT_ALIGN_32(cBits, 32);
        __asm
        {
#  ifdef RT_ARCH_AMD64
            mov     rdi, [pvBitmap]
            mov     rbx, rdi
#  else
            mov     edi, [pvBitmap]
            mov     ebx, edi
#  endif
            mov     edx, 0ffffffffh
            mov     eax, edx
            mov     ecx, [cBits]
            shr     ecx, 5
            repe    scasd
            je      done

#  ifdef RT_ARCH_AMD64
            lea     rdi, [rdi - 4]
            xor     eax, [rdi]
            sub     rdi, rbx
#  else
            lea     edi, [edi - 4]
            xor     eax, [edi]
            sub     edi, ebx
#  endif
            shl     edi, 3
            bsf     edx, eax
            add     edx, edi
        done:
            mov     [iBit], edx
        }
# endif
        return iBit;
    }
    return -1;
}
#endif


/**
 * Finds the next clear bit in a bitmap.
 *
 * @returns Index of the first zero bit.
 * @returns -1 if no clear bit was found.
 * @param   pvBitmap    Pointer to the bitmap (little endian).
 * @param   cBits       The number of bits in the bitmap. Multiple of 32.
 * @param   iBitPrev    The bit returned from the last search.
 *                      The search will start at iBitPrev + 1.
 */
#if RT_INLINE_ASM_EXTERNAL || (!defined(RT_ARCH_AMD64) && !defined(RT_ARCH_X86))
DECLASM(int) ASMBitNextClear(const volatile void RT_FAR *pvBitmap, uint32_t cBits, uint32_t iBitPrev) RT_NOTHROW_PROTO;
#else
DECLINLINE(int) ASMBitNextClear(const volatile void RT_FAR *pvBitmap, uint32_t cBits, uint32_t iBitPrev) RT_NOTHROW_DEF
{
    const volatile uint32_t RT_FAR *pau32Bitmap = (const volatile uint32_t RT_FAR *)pvBitmap;
    int                             iBit = ++iBitPrev & 31;
    if (iBit)
    {
        /*
         * Inspect the 32-bit word containing the unaligned bit.
         */
        uint32_t  u32 = ~pau32Bitmap[iBitPrev / 32] >> iBit;

# if RT_INLINE_ASM_USES_INTRIN
        unsigned long ulBit = 0;
        if (_BitScanForward(&ulBit, u32))
            return ulBit + iBitPrev;
# else
#  if RT_INLINE_ASM_GNU_STYLE
        __asm__ __volatile__("bsf %1, %0\n\t"
                             "jnz 1f\n\t"
                             "movl $-1, %0\n\t" /** @todo use conditional move for 64-bit? */
                             "1:\n\t"
                             : "=r" (iBit)
                             : "r" (u32)
                             : "cc");
#  else
        __asm
        {
            mov     edx, [u32]
            bsf     eax, edx
            jnz     done
            mov     eax, 0ffffffffh
        done:
            mov     [iBit], eax
        }
#  endif
        if (iBit >= 0)
            return iBit + (int)iBitPrev;
# endif

        /*
         * Skip ahead and see if there is anything left to search.
         */
        iBitPrev |= 31;
        iBitPrev++;
        if (cBits <= (uint32_t)iBitPrev)
            return -1;
    }

    /*
     * 32-bit aligned search, let ASMBitFirstClear do the dirty work.
     */
    iBit = ASMBitFirstClear(&pau32Bitmap[iBitPrev / 32], cBits - iBitPrev);
    if (iBit >= 0)
        iBit += iBitPrev;
    return iBit;
}
#endif


/**
 * Finds the first set bit in a bitmap.
 *
 * @returns Index of the first set bit.
 * @returns -1 if no clear bit was found.
 * @param   pvBitmap    Pointer to the bitmap (little endian).
 * @param   cBits       The number of bits in the bitmap. Multiple of 32.
 */
#if RT_INLINE_ASM_EXTERNAL || (!defined(RT_ARCH_AMD64) && !defined(RT_ARCH_X86))
DECLASM(int32_t) ASMBitFirstSet(const volatile void RT_FAR *pvBitmap, uint32_t cBits) RT_NOTHROW_PROTO;
#else
DECLINLINE(int32_t) ASMBitFirstSet(const volatile void RT_FAR *pvBitmap, uint32_t cBits) RT_NOTHROW_DEF
{
    if (cBits)
    {
        int32_t iBit;
# if RT_INLINE_ASM_GNU_STYLE
        RTCCUINTREG uEAX, uECX, uEDI;
        cBits = RT_ALIGN_32(cBits, 32);
        __asm__ __volatile__("repe; scasl\n\t"
                             "je    1f\n\t"
#  ifdef RT_ARCH_AMD64
                             "lea   -4(%%rdi), %%rdi\n\t"
                             "movl  (%%rdi), %%eax\n\t"
                             "subq  %5, %%rdi\n\t"
#  else
                             "lea   -4(%%edi), %%edi\n\t"
                             "movl  (%%edi), %%eax\n\t"
                             "subl  %5, %%edi\n\t"
#  endif
                             "shll  $3, %%edi\n\t"
                             "bsfl  %%eax, %%edx\n\t"
                             "addl  %%edi, %%edx\n\t"
                             "1:\t\n"
                             : "=d" (iBit)
                             , "=&c" (uECX)
                             , "=&D" (uEDI)
                             , "=&a" (uEAX)
                             : "0" (0xffffffff)
                             , "mr" (pvBitmap)
                             , "1" (cBits >> 5)
                             , "2" (pvBitmap)
                             , "3" (0)
                             : "cc");
# else
        cBits = RT_ALIGN_32(cBits, 32);
        __asm
        {
#  ifdef RT_ARCH_AMD64
            mov     rdi, [pvBitmap]
            mov     rbx, rdi
#  else
            mov     edi, [pvBitmap]
            mov     ebx, edi
#  endif
            mov     edx, 0ffffffffh
            xor     eax, eax
            mov     ecx, [cBits]
            shr     ecx, 5
            repe    scasd
            je      done
#  ifdef RT_ARCH_AMD64
            lea     rdi, [rdi - 4]
            mov     eax, [rdi]
            sub     rdi, rbx
#  else
            lea     edi, [edi - 4]
            mov     eax, [edi]
            sub     edi, ebx
#  endif
            shl     edi, 3
            bsf     edx, eax
            add     edx, edi
        done:
            mov   [iBit], edx
        }
# endif
        return iBit;
    }
    return -1;
}
#endif


/**
 * Finds the next set bit in a bitmap.
 *
 * @returns Index of the next set bit.
 * @returns -1 if no set bit was found.
 * @param   pvBitmap    Pointer to the bitmap (little endian).
 * @param   cBits       The number of bits in the bitmap. Multiple of 32.
 * @param   iBitPrev    The bit returned from the last search.
 *                      The search will start at iBitPrev + 1.
 */
#if RT_INLINE_ASM_EXTERNAL || (!defined(RT_ARCH_AMD64) && !defined(RT_ARCH_X86))
DECLASM(int) ASMBitNextSet(const volatile void RT_FAR *pvBitmap, uint32_t cBits, uint32_t iBitPrev) RT_NOTHROW_PROTO;
#else
DECLINLINE(int) ASMBitNextSet(const volatile void RT_FAR *pvBitmap, uint32_t cBits, uint32_t iBitPrev) RT_NOTHROW_DEF
{
    const volatile uint32_t RT_FAR *pau32Bitmap = (const volatile uint32_t RT_FAR *)pvBitmap;
    int                             iBit = ++iBitPrev & 31;
    if (iBit)
    {
        /*
         * Inspect the 32-bit word containing the unaligned bit.
         */
        uint32_t  u32 = pau32Bitmap[iBitPrev / 32] >> iBit;

# if RT_INLINE_ASM_USES_INTRIN
        unsigned long ulBit = 0;
        if (_BitScanForward(&ulBit, u32))
            return ulBit + iBitPrev;
# else
#  if RT_INLINE_ASM_GNU_STYLE
        __asm__ __volatile__("bsf %1, %0\n\t"
                             "jnz 1f\n\t"      /** @todo use conditional move for 64-bit? */
                             "movl $-1, %0\n\t"
                             "1:\n\t"
                             : "=r" (iBit)
                             : "r" (u32)
                             : "cc");
#  else
        __asm
        {
            mov     edx, [u32]
            bsf     eax, edx
            jnz     done
            mov     eax, 0ffffffffh
        done:
            mov     [iBit], eax
        }
#  endif
        if (iBit >= 0)
            return iBit + (int)iBitPrev;
# endif

        /*
         * Skip ahead and see if there is anything left to search.
         */
        iBitPrev |= 31;
        iBitPrev++;
        if (cBits <= (uint32_t)iBitPrev)
            return -1;
    }

    /*
     * 32-bit aligned search, let ASMBitFirstClear do the dirty work.
     */
    iBit = ASMBitFirstSet(&pau32Bitmap[iBitPrev / 32], cBits - iBitPrev);
    if (iBit >= 0)
        iBit += iBitPrev;
    return iBit;
}
#endif


/**
 * Finds the first bit which is set in the given 32-bit integer.
 * Bits are numbered from 1 (least significant) to 32.
 *
 * @returns index [1..32] of the first set bit.
 * @returns 0 if all bits are cleared.
 * @param   u32     Integer to search for set bits.
 * @remarks Similar to ffs() in BSD.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM_386(unsigned) ASMBitFirstSetU32(uint32_t u32) RT_NOTHROW_PROTO;
#else
DECLINLINE(unsigned) ASMBitFirstSetU32(uint32_t u32) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    unsigned long iBit;
    if (_BitScanForward(&iBit, u32))
        iBit++;
    else
        iBit = 0;

# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    uint32_t iBit;
    __asm__ __volatile__("bsf  %1, %0\n\t"
                         "jnz  1f\n\t"
                         "xorl %0, %0\n\t"
                         "jmp  2f\n"
                         "1:\n\t"
                         "incl %0\n"
                         "2:\n\t"
                         : "=r" (iBit)
                         : "rm" (u32)
                         : "cc");
#  else
    uint32_t iBit;
    _asm
    {
        bsf     eax, [u32]
        jnz     found
        xor     eax, eax
        jmp     done
    found:
        inc     eax
    done:
        mov     [iBit], eax
    }
#  endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    /*
     * Using the "count leading zeros (clz)" instruction here because there
     * is no dedicated instruction to get the first set bit.
     * Need to reverse the bits in the value with "rbit" first because
     * "clz" starts counting from the most significant bit.
     */
    uint32_t iBit;
    __asm__ __volatile__(
#  if defined(RT_ARCH_ARM64)
                         "rbit %w[uVal], %w[uVal]\n\t"
                         "clz  %w[iBit], %w[uVal]\n\t"
#  else
                         "rbit %[uVal], %[uVal]\n\t"
                         "clz  %[iBit], %[uVal]\n\t"
#  endif
                         : [uVal] "=r" (u32)
                         , [iBit] "=r" (iBit)
                         : "[uVal]" (u32));
    if (iBit != 32)
        iBit++;
    else
        iBit = 0; /* No bit set. */

# else
#  error "Port me"
# endif
    return iBit;
}
#endif


/**
 * Finds the first bit which is set in the given 32-bit integer.
 * Bits are numbered from 1 (least significant) to 32.
 *
 * @returns index [1..32] of the first set bit.
 * @returns 0 if all bits are cleared.
 * @param   i32     Integer to search for set bits.
 * @remark  Similar to ffs() in BSD.
 */
DECLINLINE(unsigned) ASMBitFirstSetS32(int32_t i32) RT_NOTHROW_DEF
{
    return ASMBitFirstSetU32((uint32_t)i32);
}


/**
 * Finds the first bit which is set in the given 64-bit integer.
 *
 * Bits are numbered from 1 (least significant) to 64.
 *
 * @returns index [1..64] of the first set bit.
 * @returns 0 if all bits are cleared.
 * @param   u64     Integer to search for set bits.
 * @remarks Similar to ffs() in BSD.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM_386(unsigned) ASMBitFirstSetU64(uint64_t u64) RT_NOTHROW_PROTO;
#else
DECLINLINE(unsigned) ASMBitFirstSetU64(uint64_t u64) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    unsigned long iBit;
#  if ARCH_BITS == 64
    if (_BitScanForward64(&iBit, u64))
        iBit++;
    else
        iBit = 0;
#  else
    if (_BitScanForward(&iBit, (uint32_t)u64))
        iBit++;
    else if (_BitScanForward(&iBit, (uint32_t)(u64 >> 32)))
        iBit += 33;
    else
        iBit = 0;
#  endif

# elif RT_INLINE_ASM_GNU_STYLE && defined(RT_ARCH_AMD64)
    uint64_t iBit;
    __asm__ __volatile__("bsfq %1, %0\n\t"
                         "jnz  1f\n\t"
                         "xorl %k0, %k0\n\t"
                         "jmp  2f\n"
                         "1:\n\t"
                         "incl %k0\n"
                         "2:\n\t"
                         : "=r" (iBit)
                         : "rm" (u64)
                         : "cc");

# elif defined(RT_ARCH_ARM64)
    uint64_t iBit;
    __asm__ __volatile__("rbit %[uVal], %[uVal]\n\t"
                         "clz  %[iBit], %[uVal]\n\t"
                         : [uVal] "=r" (u64)
                         , [iBit] "=r" (iBit)
                         : "[uVal]" (u64));
    if (iBit != 64)
        iBit++;
    else
        iBit = 0; /* No bit set. */

# else
    unsigned iBit = ASMBitFirstSetU32((uint32_t)u64);
    if (!iBit)
    {
        iBit = ASMBitFirstSetU32((uint32_t)(u64 >> 32));
        if (iBit)
            iBit += 32;
    }
# endif
    return (unsigned)iBit;
}
#endif


/**
 * Finds the first bit which is set in the given 16-bit integer.
 *
 * Bits are numbered from 1 (least significant) to 16.
 *
 * @returns index [1..16] of the first set bit.
 * @returns 0 if all bits are cleared.
 * @param   u16     Integer to search for set bits.
 * @remarks For 16-bit bs3kit code.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM_386(unsigned) ASMBitFirstSetU16(uint16_t u16) RT_NOTHROW_PROTO;
#else
DECLINLINE(unsigned) ASMBitFirstSetU16(uint16_t u16) RT_NOTHROW_DEF
{
    return ASMBitFirstSetU32((uint32_t)u16);
}
#endif


/**
 * Finds the last bit which is set in the given 32-bit integer.
 * Bits are numbered from 1 (least significant) to 32.
 *
 * @returns index [1..32] of the last set bit.
 * @returns 0 if all bits are cleared.
 * @param   u32     Integer to search for set bits.
 * @remark  Similar to fls() in BSD.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM_386(unsigned) ASMBitLastSetU32(uint32_t u32) RT_NOTHROW_PROTO;
#else
DECLINLINE(unsigned) ASMBitLastSetU32(uint32_t u32) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    unsigned long iBit;
    if (_BitScanReverse(&iBit, u32))
        iBit++;
    else
        iBit = 0;

# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#  if RT_INLINE_ASM_GNU_STYLE
    uint32_t iBit;
    __asm__ __volatile__("bsrl %1, %0\n\t"
                         "jnz   1f\n\t"
                         "xorl %0, %0\n\t"
                         "jmp  2f\n"
                         "1:\n\t"
                         "incl %0\n"
                         "2:\n\t"
                         : "=r" (iBit)
                         : "rm" (u32)
                         : "cc");
#  else
    uint32_t iBit;
    _asm
    {
        bsr     eax, [u32]
        jnz     found
        xor     eax, eax
        jmp     done
    found:
        inc     eax
    done:
        mov     [iBit], eax
    }
#  endif

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    uint32_t iBit;
    __asm__ __volatile__(
#  if defined(RT_ARCH_ARM64)
                         "clz  %w[iBit], %w[uVal]\n\t"
#  else
                         "clz  %[iBit], %[uVal]\n\t"
#  endif
                         : [iBit] "=r" (iBit)
                         : [uVal] "r" (u32));
    iBit = 32 - iBit;

# else
#  error "Port me"
# endif
    return iBit;
}
#endif


/**
 * Finds the last bit which is set in the given 32-bit integer.
 * Bits are numbered from 1 (least significant) to 32.
 *
 * @returns index [1..32] of the last set bit.
 * @returns 0 if all bits are cleared.
 * @param   i32     Integer to search for set bits.
 * @remark  Similar to fls() in BSD.
 */
DECLINLINE(unsigned) ASMBitLastSetS32(int32_t i32) RT_NOTHROW_DEF
{
    return ASMBitLastSetU32((uint32_t)i32);
}


/**
 * Finds the last bit which is set in the given 64-bit integer.
 *
 * Bits are numbered from 1 (least significant) to 64.
 *
 * @returns index [1..64] of the last set bit.
 * @returns 0 if all bits are cleared.
 * @param   u64     Integer to search for set bits.
 * @remark  Similar to fls() in BSD.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM_386(unsigned) ASMBitLastSetU64(uint64_t u64) RT_NOTHROW_PROTO;
#else
DECLINLINE(unsigned) ASMBitLastSetU64(uint64_t u64) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    unsigned long iBit;
#  if ARCH_BITS == 64
    if (_BitScanReverse64(&iBit, u64))
        iBit++;
    else
        iBit = 0;
#  else
    if (_BitScanReverse(&iBit, (uint32_t)(u64 >> 32)))
        iBit += 33;
    else if (_BitScanReverse(&iBit, (uint32_t)u64))
        iBit++;
    else
        iBit = 0;
#  endif

# elif RT_INLINE_ASM_GNU_STYLE && defined(RT_ARCH_AMD64)
    uint64_t iBit;
    __asm__ __volatile__("bsrq %1, %0\n\t"
                         "jnz  1f\n\t"
                         "xorl %k0, %k0\n\t"
                         "jmp  2f\n"
                         "1:\n\t"
                         "incl %k0\n"
                         "2:\n\t"
                         : "=r" (iBit)
                         : "rm" (u64)
                         : "cc");

# elif defined(RT_ARCH_ARM64)
    uint64_t iBit;
    __asm__ __volatile__("clz  %[iBit], %[uVal]\n\t"
                         : [iBit] "=r" (iBit)
                         : [uVal] "r" (u64));
    iBit = 64 - iBit;

# else
    unsigned iBit = ASMBitLastSetU32((uint32_t)(u64 >> 32));
    if (iBit)
        iBit += 32;
    else
        iBit = ASMBitLastSetU32((uint32_t)u64);
# endif
    return (unsigned)iBit;
}
#endif


/**
 * Finds the last bit which is set in the given 16-bit integer.
 *
 * Bits are numbered from 1 (least significant) to 16.
 *
 * @returns index [1..16] of the last set bit.
 * @returns 0 if all bits are cleared.
 * @param   u16     Integer to search for set bits.
 * @remarks For 16-bit bs3kit code.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM_386(unsigned) ASMBitLastSetU16(uint16_t u16) RT_NOTHROW_PROTO;
#else
DECLINLINE(unsigned) ASMBitLastSetU16(uint16_t u16) RT_NOTHROW_DEF
{
    return ASMBitLastSetU32((uint32_t)u16);
}
#endif


/**
 * Count the number of leading zero bits in the given 32-bit integer.
 *
 * The counting starts with the most significate bit.
 *
 * @returns Number of most significant zero bits.
 * @returns 32 if all bits are cleared.
 * @param   u32     Integer to consider.
 * @remarks Similar to __builtin_clz() in gcc, except defined zero input result.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM_386(unsigned) ASMCountLeadingZerosU32(uint32_t u32) RT_NOTHROW_PROTO;
#else
DECLINLINE(unsigned) ASMCountLeadingZerosU32(uint32_t u32) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    unsigned long iBit;
    if (!_BitScanReverse(&iBit, u32))
        return 32;
    return 31 - (unsigned)iBit;

# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
    uint32_t iBit;
#  if RT_INLINE_ASM_GNU_STYLE && defined(RT_ARCH_AMD64) && 0 /* significantly slower on 10980xe; 929 vs 237 ps/call */
    __asm__ __volatile__("bsrl   %1, %0\n\t"
                         "cmovzl %2, %0\n\t"
                         : "=&r" (iBit)
                         : "rm" (u32)
                         , "rm" ((int32_t)-1)
                         : "cc");
#  elif RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("bsr  %1, %0\n\t"
                         "jnz  1f\n\t"
                         "mov  $-1, %0\n\t"
                         "1:\n\t"
                         : "=r" (iBit)
                         : "rm" (u32)
                         : "cc");
#  else
    _asm
    {
        bsr     eax, [u32]
        jnz     found
        mov     eax, -1
    found:
        mov     [iBit], eax
    }
#  endif
    return 31 - iBit;

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    uint32_t iBit;
    __asm__ __volatile__(
#  if defined(RT_ARCH_ARM64)
                         "clz  %w[iBit], %w[uVal]\n\t"
#  else
                         "clz  %[iBit], %[uVal]\n\t"
#  endif
                         : [uVal] "=r" (u32)
                         , [iBit] "=r" (iBit)
                         : "[uVal]" (u32));
    return iBit;

# elif defined(__GNUC__)
    AssertCompile(sizeof(u32) == sizeof(unsigned int));
    return u32 ? __builtin_clz(u32) : 32;

# else
#  error "Port me"
# endif
}
#endif


/**
 * Count the number of leading zero bits in the given 64-bit integer.
 *
 * The counting starts with the most significate bit.
 *
 * @returns Number of most significant zero bits.
 * @returns 64 if all bits are cleared.
 * @param   u64     Integer to consider.
 * @remarks Similar to __builtin_clzl() in gcc, except defined zero input
 *          result.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM_386(unsigned) ASMCountLeadingZerosU64(uint64_t u64) RT_NOTHROW_PROTO;
#else
DECLINLINE(unsigned) ASMCountLeadingZerosU64(uint64_t u64) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    unsigned long iBit;
#  if ARCH_BITS == 64
    if (_BitScanReverse64(&iBit, u64))
        return 63 - (unsigned)iBit;
#  else
    if (_BitScanReverse(&iBit, (uint32_t)(u64 >> 32)))
        return 31 - (unsigned)iBit;
    if (_BitScanReverse(&iBit, (uint32_t)u64))
        return 63 - (unsigned)iBit;
#  endif
    return 64;

# elif RT_INLINE_ASM_GNU_STYLE && defined(RT_ARCH_AMD64)
    uint64_t iBit;
#  if 0 /* 10980xe benchmark: 932 ps/call - the slower variant */
    __asm__ __volatile__("bsrq   %1, %0\n\t"
                         "cmovzq %2, %0\n\t"
                         : "=&r" (iBit)
                         : "rm" (u64)
                         , "rm" ((int64_t)-1)
                         : "cc");
#  else /* 10980xe benchmark: 262 ps/call */
    __asm__ __volatile__("bsrq   %1, %0\n\t"
                         "jnz    1f\n\t"
                         "mov    $-1, %0\n\t"
                         "1:\n\t"
                         : "=&r" (iBit)
                         : "rm" (u64)
                         : "cc");
#  endif
    return 63 - (unsigned)iBit;

# elif defined(RT_ARCH_ARM64)
    uint64_t iBit;
    __asm__ __volatile__("clz  %[iBit], %[uVal]\n\t"
                         : [uVal] "=r" (u64)
                         , [iBit] "=r" (iBit)
                         : "[uVal]" (u64));
    return (unsigned)iBit;

# elif defined(__GNUC__) && ARCH_BITS == 64
    AssertCompile(sizeof(u64) == sizeof(unsigned long));
    return u64 ? __builtin_clzl(u64) : 64;

# else
    unsigned iBit = ASMCountLeadingZerosU32((uint32_t)(u64 >> 32));
    if (iBit == 32)
        iBit = ASMCountLeadingZerosU32((uint32_t)u64) + 32;
    return iBit;
# endif
}
#endif


/**
 * Count the number of leading zero bits in the given 16-bit integer.
 *
 * The counting starts with the most significate bit.
 *
 * @returns Number of most significant zero bits.
 * @returns 16 if all bits are cleared.
 * @param   u16     Integer to consider.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM_386(unsigned) ASMCountLeadingZerosU16(uint16_t u16) RT_NOTHROW_PROTO;
#else
DECLINLINE(unsigned) ASMCountLeadingZerosU16(uint16_t u16) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_GNU_STYLE && (defined(RT_ARCH_X86) || defined(RT_ARCH_AMD64)) && 0 /* slower (10980xe: 987 vs 292 ps/call) */
    uint16_t iBit;
    __asm__ __volatile__("bsrw %1, %0\n\t"
                         "jnz  1f\n\t"
                         "mov  $-1, %0\n\t"
                         "1:\n\t"
                         : "=r" (iBit)
                         : "rm" (u16)
                         : "cc");
    return 15 - (int16_t)iBit;
# else
    return ASMCountLeadingZerosU32((uint32_t)u16) - 16;
# endif
}
#endif


/**
 * Count the number of trailing zero bits in the given 32-bit integer.
 *
 * The counting starts with the least significate bit, i.e. the zero bit.
 *
 * @returns Number of lest significant zero bits.
 * @returns 32 if all bits are cleared.
 * @param   u32     Integer to consider.
 * @remarks Similar to __builtin_ctz() in gcc, except defined zero input result.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM_386(unsigned) ASMCountTrailingZerosU32(uint32_t u32) RT_NOTHROW_PROTO;
#else
DECLINLINE(unsigned) ASMCountTrailingZerosU32(uint32_t u32) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    unsigned long iBit;
    if (!_BitScanForward(&iBit, u32))
        return 32;
    return (unsigned)iBit;

# elif defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
    uint32_t iBit;
#  if RT_INLINE_ASM_GNU_STYLE && defined(RT_ARCH_AMD64) && 0 /* significantly slower on 10980xe; 932 vs 240 ps/call */
    __asm__ __volatile__("bsfl   %1, %0\n\t"
                         "cmovzl %2, %0\n\t"
                         : "=&r" (iBit)
                         : "rm" (u32)
                         , "rm" ((int32_t)32)
                         : "cc");
#  elif RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("bsfl %1, %0\n\t"
                         "jnz  1f\n\t"
                         "mov  $32, %0\n\t"
                         "1:\n\t"
                         : "=r" (iBit)
                         : "rm" (u32)
                         : "cc");
#  else
    _asm
    {
        bsf     eax, [u32]
        jnz     found
        mov     eax, 32
    found:
        mov     [iBit], eax
    }
#  endif
    return iBit;

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    /* Invert the bits and use clz. */
    uint32_t iBit;
    __asm__ __volatile__(
#  if defined(RT_ARCH_ARM64)
                         "rbit %w[uVal], %w[uVal]\n\t"
                         "clz  %w[iBit], %w[uVal]\n\t"
#  else
                         "rbit %[uVal], %[uVal]\n\t"
                         "clz  %[iBit], %[uVal]\n\t"
#  endif
                         : [uVal] "=r" (u32)
                         , [iBit] "=r" (iBit)
                         : "[uVal]" (u32));
    return iBit;

# elif defined(__GNUC__)
    AssertCompile(sizeof(u32) == sizeof(unsigned int));
    return u32 ? __builtin_ctz(u32) : 32;

# else
#  error "Port me"
# endif
}
#endif


/**
 * Count the number of trailing zero bits in the given 64-bit integer.
 *
 * The counting starts with the least significate bit.
 *
 * @returns Number of least significant zero bits.
 * @returns 64 if all bits are cleared.
 * @param   u64     Integer to consider.
 * @remarks Similar to __builtin_ctzl() in gcc, except defined zero input
 *          result.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM_386(unsigned) ASMCountTrailingZerosU64(uint64_t u64) RT_NOTHROW_PROTO;
#else
DECLINLINE(unsigned) ASMCountTrailingZerosU64(uint64_t u64) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    unsigned long iBit;
#  if ARCH_BITS == 64
    if (_BitScanForward64(&iBit, u64))
        return (unsigned)iBit;
#  else
    if (_BitScanForward(&iBit, (uint32_t)u64))
        return (unsigned)iBit;
    if (_BitScanForward(&iBit, (uint32_t)(u64 >> 32)))
        return (unsigned)iBit + 32;
#  endif
    return 64;

# elif RT_INLINE_ASM_GNU_STYLE && defined(RT_ARCH_AMD64)
    uint64_t iBit;
#  if 0 /* 10980xe benchmark: 932 ps/call - the slower variant */
    __asm__ __volatile__("bsfq   %1, %0\n\t"
                         "cmovzq %2, %0\n\t"
                         : "=&r" (iBit)
                         : "rm" (u64)
                         , "rm" ((int64_t)64)
                         : "cc");
#  else /* 10980xe benchmark: 262 ps/call */
    __asm__ __volatile__("bsfq   %1, %0\n\t"
                         "jnz    1f\n\t"
                         "mov    $64, %0\n\t"
                         "1:\n\t"
                         : "=&r" (iBit)
                         : "rm" (u64)
                         : "cc");
#  endif
    return (unsigned)iBit;

# elif defined(RT_ARCH_ARM64)
    /* Invert the bits and use clz. */
    uint64_t iBit;
    __asm__ __volatile__("rbit %[uVal], %[uVal]\n\t"
                         "clz  %[iBit], %[uVal]\n\t"
                         : [uVal] "=r" (u64)
                         , [iBit] "=r" (iBit)
                         : "[uVal]" (u64));
    return (unsigned)iBit;

# elif defined(__GNUC__) && ARCH_BITS == 64
    AssertCompile(sizeof(u64) == sizeof(unsigned long));
    return u64 ? __builtin_ctzl(u64) : 64;

# else
    unsigned iBit = ASMCountTrailingZerosU32((uint32_t)u64);
    if (iBit == 32)
        iBit = ASMCountTrailingZerosU32((uint32_t)(u64 >> 32)) + 32;
    return iBit;
# endif
}
#endif


/**
 * Count the number of trailing zero bits in the given 16-bit integer.
 *
 * The counting starts with the most significate bit.
 *
 * @returns Number of most significant zero bits.
 * @returns 16 if all bits are cleared.
 * @param   u16     Integer to consider.
 */
#if RT_INLINE_ASM_EXTERNAL_TMP_ARM && !RT_INLINE_ASM_USES_INTRIN
RT_ASM_DECL_PRAGMA_WATCOM_386(unsigned) ASMCountTrailingZerosU16(uint16_t u16) RT_NOTHROW_PROTO;
#else
DECLINLINE(unsigned) ASMCountTrailingZerosU16(uint16_t u16) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_GNU_STYLE && (defined(RT_ARCH_X86) || defined(RT_ARCH_AMD64)) && 0 /* slower (10980xe: 992 vs 349 ps/call) */
    uint16_t iBit;
    __asm__ __volatile__("bsfw %1, %0\n\t"
                         "jnz  1f\n\t"
                         "mov  $16, %0\n\t"
                         "1:\n\t"
                         : "=r" (iBit)
                         : "rm" (u16)
                         : "cc");
    return iBit;
# else
    return ASMCountTrailingZerosU32((uint32_t)u16 | UINT32_C(0x10000));
#endif
}
#endif


/**
 * Rotate 32-bit unsigned value to the left by @a cShift.
 *
 * @returns Rotated value.
 * @param   u32                 The value to rotate.
 * @param   cShift              How many bits to rotate by.
 */
#ifdef __WATCOMC__
RT_ASM_DECL_PRAGMA_WATCOM(uint32_t) ASMRotateLeftU32(uint32_t u32, unsigned cShift) RT_NOTHROW_PROTO;
#else
DECLINLINE(uint32_t) ASMRotateLeftU32(uint32_t u32, uint32_t cShift) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    return _rotl(u32, cShift);

# elif RT_INLINE_ASM_GNU_STYLE && (defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86))
    __asm__ __volatile__("roll %b1, %0" : "=g" (u32) : "Ic" (cShift), "0" (u32) : "cc");
    return u32;

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    __asm__ __volatile__(
#  if defined(RT_ARCH_ARM64)
                         "ror       %w[uRet], %w[uVal], %w[cShift]\n\t"
#  else
                         "ror       %[uRet], %[uVal], %[cShift]\n\t"
#  endif
                         : [uRet] "=r" (u32)
                         : [uVal] "[uRet]" (u32)
                         , [cShift] "r" (32 - (cShift & 31))); /** @todo there is an immediate form here */
    return u32;

# else
    cShift &= 31;
    return (u32 << cShift) | (u32 >> (32 - cShift));
# endif
}
#endif


/**
 * Rotate 32-bit unsigned value to the right by @a cShift.
 *
 * @returns Rotated value.
 * @param   u32                 The value to rotate.
 * @param   cShift              How many bits to rotate by.
 */
#ifdef __WATCOMC__
RT_ASM_DECL_PRAGMA_WATCOM(uint32_t) ASMRotateRightU32(uint32_t u32, unsigned cShift) RT_NOTHROW_PROTO;
#else
DECLINLINE(uint32_t) ASMRotateRightU32(uint32_t u32, uint32_t cShift) RT_NOTHROW_DEF
{
# if RT_INLINE_ASM_USES_INTRIN
    return _rotr(u32, cShift);

# elif RT_INLINE_ASM_GNU_STYLE && (defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86))
    __asm__ __volatile__("rorl %b1, %0" : "=g" (u32) : "Ic" (cShift), "0" (u32) : "cc");
    return u32;

# elif defined(RT_ARCH_ARM64) || defined(RT_ARCH_ARM32)
    __asm__ __volatile__(
#  if defined(RT_ARCH_ARM64)
                         "ror       %w[uRet], %w[uVal], %w[cShift]\n\t"
#  else
                         "ror       %[uRet], %[uVal], %[cShift]\n\t"
#  endif
                         : [uRet] "=r" (u32)
                         : [uVal] "[uRet]" (u32)
                         , [cShift] "r" (cShift & 31)); /** @todo there is an immediate form here */
    return u32;

# else
    cShift &= 31;
    return (u32 >> cShift) | (u32 << (32 - cShift));
# endif
}
#endif


/**
 * Rotate 64-bit unsigned value to the left by @a cShift.
 *
 * @returns Rotated value.
 * @param   u64                 The value to rotate.
 * @param   cShift              How many bits to rotate by.
 */
DECLINLINE(uint64_t) ASMRotateLeftU64(uint64_t u64, uint32_t cShift) RT_NOTHROW_DEF
{
#if RT_INLINE_ASM_USES_INTRIN
    return _rotl64(u64, cShift);

#elif RT_INLINE_ASM_GNU_STYLE && defined(RT_ARCH_AMD64)
    __asm__ __volatile__("rolq %b1, %0" : "=g" (u64) : "Jc" (cShift), "0" (u64) : "cc");
    return u64;

#elif RT_INLINE_ASM_GNU_STYLE && defined(RT_ARCH_X86)
    uint32_t uSpill;
    __asm__ __volatile__("testb $0x20, %%cl\n\t"        /* if (cShift >= 0x20) { swap(u64.hi, u64lo); cShift -= 0x20; } */
                         "jz    1f\n\t"
                         "xchgl %%eax, %%edx\n\t"
                         "1:\n\t"
                         "andb  $0x1f, %%cl\n\t"        /* if (cShift & 0x1f) { */
                         "jz    2f\n\t"
                         "movl  %%edx, %2\n\t"          /*   save the hi value in %3. */
                         "shldl %%cl,%%eax,%%edx\n\t"   /*   shift the hi value left, feeding MSBits from the low value. */
                         "shldl %%cl,%2,%%eax\n\t"      /*   shift the lo value left, feeding MSBits from the saved hi value. */
                         "2:\n\t"                       /* } */
                         : "=A" (u64)
                         , "=c" (cShift)
                         , "=r" (uSpill)
                         : "0" (u64)
                         , "1" (cShift)
                         : "cc");
    return u64;

# elif defined(RT_ARCH_ARM64)
    __asm__ __volatile__("ror       %[uRet], %[uVal], %[cShift]\n\t"
                         : [uRet] "=r" (u64)
                         : [uVal] "[uRet]" (u64)
                         , [cShift] "r" ((uint64_t)(64 - (cShift & 63)))); /** @todo there is an immediate form here */
    return u64;

#else
    cShift &= 63;
    return (u64 << cShift) | (u64 >> (64 - cShift));
#endif
}


/**
 * Rotate 64-bit unsigned value to the right by @a cShift.
 *
 * @returns Rotated value.
 * @param   u64                 The value to rotate.
 * @param   cShift              How many bits to rotate by.
 */
DECLINLINE(uint64_t) ASMRotateRightU64(uint64_t u64, uint32_t cShift) RT_NOTHROW_DEF
{
#if RT_INLINE_ASM_USES_INTRIN
    return _rotr64(u64, cShift);

#elif RT_INLINE_ASM_GNU_STYLE && defined(RT_ARCH_AMD64)
    __asm__ __volatile__("rorq %b1, %0" : "=g" (u64) : "Jc" (cShift), "0" (u64) : "cc");
    return u64;

#elif RT_INLINE_ASM_GNU_STYLE && defined(RT_ARCH_X86)
    uint32_t uSpill;
    __asm__ __volatile__("testb $0x20, %%cl\n\t"        /* if (cShift >= 0x20) { swap(u64.hi, u64lo); cShift -= 0x20; } */
                         "jz    1f\n\t"
                         "xchgl %%eax, %%edx\n\t"
                         "1:\n\t"
                         "andb  $0x1f, %%cl\n\t"        /* if (cShift & 0x1f) { */
                         "jz    2f\n\t"
                         "movl  %%edx, %2\n\t"          /*   save the hi value in %3. */
                         "shrdl %%cl,%%eax,%%edx\n\t"   /*   shift the hi value right, feeding LSBits from the low value. */
                         "shrdl %%cl,%2,%%eax\n\t"      /*   shift the lo value right, feeding LSBits from the saved hi value. */
                         "2:\n\t"                       /* } */
                         : "=A" (u64)
                         , "=c" (cShift)
                         , "=r" (uSpill)
                         : "0" (u64)
                         , "1" (cShift)
                         : "cc");
    return u64;

# elif defined(RT_ARCH_ARM64)
    __asm__ __volatile__("ror       %[uRet], %[uVal], %[cShift]\n\t"
                         : [uRet] "=r" (u64)
                         : [uVal] "[uRet]" (u64)
                         , [cShift] "r" ((uint64_t)(cShift & 63))); /** @todo there is an immediate form here */
    return u64;

#else
    cShift &= 63;
    return (u64 >> cShift) | (u64 << (64 - cShift));
#endif
}

/** @} */


/** @} */

/*
 * Include #pragma aux definitions for Watcom C/C++.
 */
#if defined(__WATCOMC__) && ARCH_BITS == 16 && defined(RT_ARCH_X86)
# define IPRT_ASM_WATCOM_X86_16_WITH_PRAGMAS
# undef IPRT_INCLUDED_asm_watcom_x86_16_h
# include "asm-watcom-x86-16.h"
#elif defined(__WATCOMC__) && ARCH_BITS == 32 && defined(RT_ARCH_X86)
# define IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
# undef IPRT_INCLUDED_asm_watcom_x86_32_h
# include "asm-watcom-x86-32.h"
#endif

#endif /* !IPRT_INCLUDED_asm_h */

