/** @file
 * IPRT - ARM Specific Assembly Functions.
 */

/*
 * Copyright (C) 2015-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_asm_arm_h
#define IPRT_INCLUDED_asm_arm_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>
#if !defined(RT_ARCH_ARM64) && !defined(RT_ARCH_ARM32)
# error "Not on ARM64 or ARM32"
#endif

/** @defgroup grp_rt_asm_arm  ARM Specific ASM Routines
 * @ingroup grp_rt_asm
 * @{
 */


#if 0 /* figure out arm64 */

/**
 * Get the CPSR (Current Program Status) register.
 * @returns CPSR.
 */
#if RT_INLINE_ASM_EXTERNAL
DECLASM(RTCCUINTREG) ASMGetFlags(void);
#else
DECLINLINE(RTCCUINTREG) ASMGetFlags(void)
{
    RTCCUINTREG uFlags;
# if RT_INLINE_ASM_GNU_STYLE
#  ifdef RT_ARCH_ARM64
    __asm__ __volatile__("mrs %0, nzcv\n\t"  : "=r" (uFlags));
#  else
    __asm__ __volatile__("mrs %0, cpsr\n\t"  : "=r" (uFlags));
#  endif
# else
#  error "Unsupported compiler"
# endif
    return uFlags;
}
#endif


/**
 * Set the CPSR register.
 * @param   uFlags      The new CPSR value.
 */
#if RT_INLINE_ASM_EXTERNAL
DECLASM(void) ASMSetFlags(RTCCUINTREG uFlags);
#else
DECLINLINE(void) ASMSetFlags(RTCCUINTREG uFlags)
{
# if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("msr cpsr_c, %0\n\t"
                         : : "r" (uFlags));
# else
#  error "Unsupported compiler"
# endif
}
#endif

#endif


/**
 * Gets the content of the CNTVCT_EL0 (or CNTPCT) register.
 *
 * @returns CNTVCT_EL0 value.
 * @note    We call this TSC to better fit in with existing x86/amd64 based code.
 */
#if RT_INLINE_ASM_EXTERNAL
DECLASM(uint64_t) ASMReadTSC(void);
#else
DECLINLINE(uint64_t) ASMReadTSC(void)
{
# if RT_INLINE_ASM_GNU_STYLE
    uint64_t u64;
#  ifdef RT_ARCH_ARM64
    __asm__ __volatile__("isb\n\t"
                         "mrs %0, CNTVCT_EL0\n\t"
                         : "=r" (u64));
#  else
    uint32_t u32Spill;
    uint32_t u32Comp;
    __asm__ __volatile__("isb\n"
                         "Lagain:\n\t"
                         "mrrc p15, 0, %[uSpill], %H[uRet],   c14\n\t"  /* CNTPCT high into uRet.hi */
                         "mrrc p15, 0, %[uRet],   %[uSpill],  c14\n\t"  /* CNTPCT low  into uRet.lo */
                         "mrrc p15, 0, %[uSpill], %[uHiComp], c14\n\t"  /* CNTPCT high into uHiComp */
                         "cmp  %H[uRet], %[uHiComp]\n\t"
                         "b.eq Lagain\n\t"                              /* Redo if high value changed. */
                         : [uRet] "=r" (u64)
                         , "=r" (uHiComp)
                         , "=r" (uSpill));
#  endif
    return u64;

# else
#  error "Unsupported compiler"
# endif
}
#endif

#if 0 /* port to arm64, armv7 and check */

/**
 * Enables interrupts (IRQ and FIQ).
 */
#if RT_INLINE_ASM_EXTERNAL
DECLASM(void) ASMIntEnable(void);
#else
DECLINLINE(void) ASMIntEnable(void)
{
    RTCCUINTREG uFlags;
# if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("mrs %0, cpsr\n\t"
                         "bic %0, %0, #0xc0\n\t"
                         "msr cpsr_c, %0\n\t"
                         : "=r" (uFlags));
# else
#  error "Unsupported compiler"
# endif
}
#endif


/**
 * Disables interrupts (IRQ and FIQ).
 */
#if RT_INLINE_ASM_EXTERNAL
DECLASM(void) ASMIntDisable(void);
#else
DECLINLINE(void) ASMIntDisable(void)
{
    RTCCUINTREG uFlags;
# if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__("mrs %0, cpsr\n\t"
                         "orr %0, %0, #0xc0\n\t"
                         "msr cpsr_c, %0\n\t"
                         : "=r" (uFlags));
# else
#  error "Unsupported compiler"
# endif
}
#endif


/**
 * Disables interrupts and returns previous uFLAGS.
 */
#if RT_INLINE_ASM_EXTERNAL
DECLASM(RTCCUINTREG) ASMIntDisableFlags(void);
#else
DECLINLINE(RTCCUINTREG) ASMIntDisableFlags(void)
{
    RTCCUINTREG uFlags;
# if RT_INLINE_ASM_GNU_STYLE
    RTCCUINTREG uNewFlags;
    __asm__ __volatile__("mrs %0, cpsr\n\t"
                         "orr %1, %0, #0xc0\n\t"
                         "msr cpsr_c, %1\n\t"
                         : "=r" (uFlags)
                         , "=r" (uNewFlags));
# else
#  error "Unsupported compiler"
# endif
    return uFlags;
}
#endif


/**
 * Are interrupts enabled?
 *
 * @returns true / false.
 */
DECLINLINE(bool) ASMIntAreEnabled(void)
{
/** @todo r=bird: reversed, but does both need to be enabled? */
    return ASMGetFlags() & 0xc0 /* IRQ and FIQ bits */ ? true : false;
}

#endif

/**
 * Halts the CPU until interrupted.
 */
#if RT_INLINE_ASM_EXTERNAL
DECLASM(void) ASMHalt(void);
#else
DECLINLINE(void) ASMHalt(void)
{
# if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__ ("wfi\n\t"); /* wait for interrupt */
# else
#  error "Unsupported compiler"
# endif
}
#endif

#if 0
/**
 * Gets the CPU ID of the current CPU.
 *
 * @returns the CPU ID.
 * @note the name of this method is a bit misleading but serves the purpose
 *       and prevents #ifdef orgies in other places.
 */
#if RT_INLINE_ASM_EXTERNAL
DECLASM(uint8_t) ASMGetApicId(void);
#else
DECLINLINE(uint8_t) ASMGetApicId(void)
{
# if RT_INLINE_ASM_GNU_STYLE
    RTCCUINTREG uCpuId;
    __asm__ ("mrc p15, 0, %0, c0, c0, 5\n\t" /*  CPU ID Register, privileged */
             : "=r" (uCpuId));
    return uCpuId;
# else
#  error "Unsupported compiler"
# endif
}
#endif
#endif

#if 0

/**
 * Invalidate page.
 *
 * @param   pv      Address of the page to invalidate.
 */
#if RT_INLINE_ASM_EXTERNAL
DECLASM(void) ASMInvalidatePage(void *pv);
#else
DECLINLINE(void) ASMInvalidatePage(void *pv)
{
# if RT_INLINE_ASM_GNU_STYLE

# else
#  error "Unsupported compiler"
# endif
}
#endif


/**
 * Write back the internal caches and invalidate them.
 */
#if RT_INLINE_ASM_EXTERNAL
DECLASM(void) ASMWriteBackAndInvalidateCaches(void);
#else
DECLINLINE(void) ASMWriteBackAndInvalidateCaches(void)
{
# if RT_INLINE_ASM_GNU_STYLE

# else
#  error "Unsupported compiler"
# endif
}
#endif


/**
 * Invalidate internal and (perhaps) external caches without first
 * flushing dirty cache lines. Use with extreme care.
 */
#if RT_INLINE_ASM_EXTERNAL
DECLASM(void) ASMInvalidateInternalCaches(void);
#else
DECLINLINE(void) ASMInvalidateInternalCaches(void)
{
# if RT_INLINE_ASM_GNU_STYLE

# else
#  error "Unsupported compiler"
# endif
}
#endif

#endif


/** @} */
#endif /* !IPRT_INCLUDED_asm_arm_h */

