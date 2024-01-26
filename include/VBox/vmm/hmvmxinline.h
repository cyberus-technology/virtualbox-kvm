/** @file
 * HM - VMX Structures and Definitions. (VMM)
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

#ifndef VBOX_INCLUDED_vmm_hmvmxinline_h
#define VBOX_INCLUDED_vmm_hmvmxinline_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/vmm/hm_vmx.h>
#include <VBox/err.h>

/* In Visual C++ versions prior to 2012, the vmx intrinsics are only available
   when targeting AMD64. */
#if RT_INLINE_ASM_USES_INTRIN >= RT_MSC_VER_VS2010 && defined(RT_ARCH_AMD64)
# include <iprt/sanitized/intrin.h>
/* We always want them as intrinsics, no functions. */
# pragma intrinsic(__vmx_on)
# pragma intrinsic(__vmx_off)
# pragma intrinsic(__vmx_vmclear)
# pragma intrinsic(__vmx_vmptrld)
# pragma intrinsic(__vmx_vmread)
# pragma intrinsic(__vmx_vmwrite)
# define VMX_USE_MSC_INTRINSICS 1
#else
# define VMX_USE_MSC_INTRINSICS 0
#endif

/**
 * Whether we think the assembler supports VMX instructions.
 *
 * Guess that GCC 5 should have sufficient recent enough binutils.
 */
#if RT_INLINE_ASM_GNU_STYLE && RT_GNUC_PREREQ(5,0)
# define VMX_USE_GNU_STYLE_INLINE_VMX_INSTRUCTIONS 1
#else
# define VMX_USE_GNU_STYLE_INLINE_VMX_INSTRUCTIONS 0
#endif

/** Whether we can use the subsection trick to put error handling code
 *  elsewhere. */
#if VMX_USE_GNU_STYLE_INLINE_VMX_INSTRUCTIONS && defined(__ELF__)
# define VMX_USE_GNU_STYLE_INLINE_SECTION_TRICK 1
#else
# define VMX_USE_GNU_STYLE_INLINE_SECTION_TRICK 0
#endif

/* Skip checking VMREAD/VMWRITE failures on non-strict builds. */
#ifndef VBOX_STRICT
# define VBOX_WITH_VMREAD_VMWRITE_NOCHECK
#endif


/** @defgroup grp_hm_vmx_inline    VMX Inline Helpers
 * @ingroup grp_hm_vmx
 * @{
 */
/**
 * Gets the effective width of a VMCS field given it's encoding adjusted for
 * HIGH/FULL access for 64-bit fields.
 *
 * @returns The effective VMCS field width.
 * @param   uFieldEnc   The VMCS field encoding.
 *
 * @remarks Warning! This function does not verify the encoding is for a valid and
 *          supported VMCS field.
 */
DECLINLINE(uint8_t) VMXGetVmcsFieldWidthEff(uint32_t uFieldEnc)
{
    /* Only the "HIGH" parts of all 64-bit fields have bit 0 set. */
    if (uFieldEnc & RT_BIT(0))
        return VMXVMCSFIELDWIDTH_32BIT;

    /* Bits 13:14 contains the width of the VMCS field, see VMXVMCSFIELDWIDTH_XXX. */
    return (uFieldEnc >> 13) & 0x3;
}


/**
 * Returns whether the given VMCS field is a read-only VMCS field or not.
 *
 * @returns @c true if it's a read-only field, @c false otherwise.
 * @param   uFieldEnc   The VMCS field encoding.
 *
 * @remarks Warning! This function does not verify that the encoding is for a valid
 *          and/or supported VMCS field.
 */
DECLINLINE(bool) VMXIsVmcsFieldReadOnly(uint32_t uFieldEnc)
{
    /* See Intel spec. B.4.2 "Natural-Width Read-Only Data Fields". */
    return (RT_BF_GET(uFieldEnc, VMX_BF_VMCSFIELD_TYPE) == VMXVMCSFIELDTYPE_VMEXIT_INFO);
}


/**
 * Returns whether the given VM-entry interruption-information type is valid or not.
 *
 * @returns @c true if it's a valid type, @c false otherwise.
 * @param   fSupportsMTF    Whether the Monitor-Trap Flag CPU feature is supported.
 * @param   uType           The VM-entry interruption-information type.
 */
DECLINLINE(bool) VMXIsEntryIntInfoTypeValid(bool fSupportsMTF, uint8_t uType)
{
    /* See Intel spec. 26.2.1.3 "VM-Entry Control Fields". */
    switch (uType)
    {
        case VMX_ENTRY_INT_INFO_TYPE_EXT_INT:
        case VMX_ENTRY_INT_INFO_TYPE_NMI:
        case VMX_ENTRY_INT_INFO_TYPE_HW_XCPT:
        case VMX_ENTRY_INT_INFO_TYPE_SW_INT:
        case VMX_ENTRY_INT_INFO_TYPE_PRIV_SW_XCPT:
        case VMX_ENTRY_INT_INFO_TYPE_SW_XCPT:           return true;
        case VMX_ENTRY_INT_INFO_TYPE_OTHER_EVENT:       return fSupportsMTF;
        default:
            return false;
    }
}


/**
 * Returns whether the given VM-entry interruption-information vector and type
 * combination is valid or not.
 *
 * @returns @c true if it's a valid vector/type combination, @c false otherwise.
 * @param   uVector     The VM-entry interruption-information vector.
 * @param   uType       The VM-entry interruption-information type.
 *
 * @remarks Warning! This function does not validate the type field individually.
 *          Use it after verifying type is valid using HMVmxIsEntryIntInfoTypeValid.
 */
DECLINLINE(bool) VMXIsEntryIntInfoVectorValid(uint8_t uVector, uint8_t uType)
{
    /* See Intel spec. 26.2.1.3 "VM-Entry Control Fields". */
    if (   uType == VMX_ENTRY_INT_INFO_TYPE_NMI
        && uVector != X86_XCPT_NMI)
        return false;
    if (   uType == VMX_ENTRY_INT_INFO_TYPE_HW_XCPT
        && uVector > X86_XCPT_LAST)
        return false;
    if (   uType == VMX_ENTRY_INT_INFO_TYPE_OTHER_EVENT
        && uVector != VMX_ENTRY_INT_INFO_VECTOR_MTF)
        return false;
    return true;
}


/**
 * Returns whether or not the VM-exit is trap-like or fault-like.
 *
 * @returns @c true if it's a trap-like VM-exit, @c false otherwise.
 * @param   uExitReason     The VM-exit reason.
 *
 * @remarks Warning! This does not validate the VM-exit reason.
 */
DECLINLINE(bool) VMXIsVmexitTrapLike(uint32_t uExitReason)
{
    /*
     * Trap-like VM-exits - The instruction causing the VM-exit completes before the
     * VM-exit occurs.
     *
     * Fault-like VM-exits - The instruction causing the VM-exit is not completed before
     * the VM-exit occurs.
     *
     * See Intel spec. 25.5.2 "Monitor Trap Flag".
     * See Intel spec. 29.1.4 "EOI Virtualization".
     * See Intel spec. 29.4.3.3 "APIC-Write VM Exits".
     * See Intel spec. 29.1.2 "TPR Virtualization".
     */
    /** @todo NSTVMX: r=ramshankar: What about VM-exits due to debug traps (single-step,
     *        I/O breakpoints, data breakpoints), debug exceptions (data breakpoint)
     *        delayed by MovSS blocking, machine-check exceptions. */
    switch (uExitReason)
    {
        case VMX_EXIT_MTF:
        case VMX_EXIT_VIRTUALIZED_EOI:
        case VMX_EXIT_APIC_WRITE:
        case VMX_EXIT_TPR_BELOW_THRESHOLD:
            return true;
    }
    return false;
}


/**
 * Returns whether the VM-entry is vectoring or not given the VM-entry interruption
 * information field.
 *
 * @returns @c true if the VM-entry is vectoring, @c false otherwise.
 * @param   uEntryIntInfo       The VM-entry interruption information field.
 * @param   pEntryIntInfoType   The VM-entry interruption information type field.
 *                              Optional, can be NULL. Only updated when this
 *                              function returns @c true.
 */
DECLINLINE(bool) VMXIsVmentryVectoring(uint32_t uEntryIntInfo, uint8_t *pEntryIntInfoType)
{
    /*
     * The definition of what is a vectoring VM-entry is taken
     * from Intel spec. 26.6 "Special Features of VM Entry".
     */
    if (!VMX_ENTRY_INT_INFO_IS_VALID(uEntryIntInfo))
        return false;

    /* Scope and keep variable defines on top to satisy archaic c89 nonsense. */
    {
        uint8_t const uType = VMX_ENTRY_INT_INFO_TYPE(uEntryIntInfo);
        switch (uType)
        {
            case VMX_ENTRY_INT_INFO_TYPE_EXT_INT:
            case VMX_ENTRY_INT_INFO_TYPE_NMI:
            case VMX_ENTRY_INT_INFO_TYPE_HW_XCPT:
            case VMX_ENTRY_INT_INFO_TYPE_SW_INT:
            case VMX_ENTRY_INT_INFO_TYPE_PRIV_SW_XCPT:
            case VMX_ENTRY_INT_INFO_TYPE_SW_XCPT:
            {
                if (pEntryIntInfoType)
                    *pEntryIntInfoType = uType;
                return true;
            }
        }
    }
    return false;
}


/**
 * Gets the description for a VMX abort reason.
 *
 * @returns The descriptive string.
 * @param   enmAbort    The VMX abort reason.
 */
DECLINLINE(const char *) VMXGetAbortDesc(VMXABORT enmAbort)
{
    switch (enmAbort)
    {
        case VMXABORT_NONE:                     return "VMXABORT_NONE";
        case VMXABORT_SAVE_GUEST_MSRS:          return "VMXABORT_SAVE_GUEST_MSRS";
        case VMXBOART_HOST_PDPTE:               return "VMXBOART_HOST_PDPTE";
        case VMXABORT_CURRENT_VMCS_CORRUPT:     return "VMXABORT_CURRENT_VMCS_CORRUPT";
        case VMXABORT_LOAD_HOST_MSR:            return "VMXABORT_LOAD_HOST_MSR";
        case VMXABORT_MACHINE_CHECK_XCPT:       return "VMXABORT_MACHINE_CHECK_XCPT";
        case VMXABORT_HOST_NOT_IN_LONG_MODE:    return "VMXABORT_HOST_NOT_IN_LONG_MODE";
        default:
            break;
    }
    return "Unknown/invalid";
}


/**
 * Gets the description for a virtual VMCS state.
 *
 * @returns The descriptive string.
 * @param   fVmcsState      The virtual-VMCS state.
 */
DECLINLINE(const char *) VMXGetVmcsStateDesc(uint8_t fVmcsState)
{
    switch (fVmcsState)
    {
        case VMX_V_VMCS_LAUNCH_STATE_CLEAR:     return "Clear";
        case VMX_V_VMCS_LAUNCH_STATE_LAUNCHED:  return "Launched";
        default:                                return "Unknown";
    }
}


/**
 * Gets the description for a VM-entry interruption information event type.
 *
 * @returns The descriptive string.
 * @param   uType    The event type.
 */
DECLINLINE(const char *) VMXGetEntryIntInfoTypeDesc(uint8_t uType)
{
    switch (uType)
    {
        case VMX_ENTRY_INT_INFO_TYPE_EXT_INT:       return "External Interrupt";
        case VMX_ENTRY_INT_INFO_TYPE_NMI:           return "NMI";
        case VMX_ENTRY_INT_INFO_TYPE_HW_XCPT:       return "Hardware Exception";
        case VMX_ENTRY_INT_INFO_TYPE_SW_INT:        return "Software Interrupt";
        case VMX_ENTRY_INT_INFO_TYPE_PRIV_SW_XCPT:  return "Priv. Software Exception";
        case VMX_ENTRY_INT_INFO_TYPE_SW_XCPT:       return "Software Exception";
        case VMX_ENTRY_INT_INFO_TYPE_OTHER_EVENT:   return "Other Event";
        default:
            break;
    }
    return "Unknown/invalid";
}


/**
 * Gets the description for a VM-exit interruption information event type.
 *
 * @returns The descriptive string.
 * @param   uType    The event type.
 */
DECLINLINE(const char *) VMXGetExitIntInfoTypeDesc(uint8_t uType)
{
    switch (uType)
    {
        case VMX_EXIT_INT_INFO_TYPE_EXT_INT:       return "External Interrupt";
        case VMX_EXIT_INT_INFO_TYPE_NMI:           return "NMI";
        case VMX_EXIT_INT_INFO_TYPE_HW_XCPT:       return "Hardware Exception";
        case VMX_EXIT_INT_INFO_TYPE_SW_INT:        return "Software Interrupt";
        case VMX_EXIT_INT_INFO_TYPE_PRIV_SW_XCPT:  return "Priv. Software Exception";
        case VMX_EXIT_INT_INFO_TYPE_SW_XCPT:       return "Software Exception";
        default:
            break;
    }
    return "Unknown/invalid";
}


/**
 * Gets the description for an IDT-vectoring information event type.
 *
 * @returns The descriptive string.
 * @param   uType    The event type.
 */
DECLINLINE(const char *) VMXGetIdtVectoringInfoTypeDesc(uint8_t uType)
{
    switch (uType)
    {
        case VMX_IDT_VECTORING_INFO_TYPE_EXT_INT:       return "External Interrupt";
        case VMX_IDT_VECTORING_INFO_TYPE_NMI:           return "NMI";
        case VMX_IDT_VECTORING_INFO_TYPE_HW_XCPT:       return "Hardware Exception";
        case VMX_IDT_VECTORING_INFO_TYPE_SW_INT:        return "Software Interrupt";
        case VMX_IDT_VECTORING_INFO_TYPE_PRIV_SW_XCPT:  return "Priv. Software Exception";
        case VMX_IDT_VECTORING_INFO_TYPE_SW_XCPT:       return "Software Exception";
        default:
            break;
    }
    return "Unknown/invalid";
}


/** @} */


/** @defgroup grp_hm_vmx_asm    VMX Assembly Helpers
 * @{
 */
#if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)

/**
 * Dispatches an NMI to the host.
 */
DECLASM(int) VMXDispatchHostNmi(void);


/**
 * Executes VMXON.
 *
 * @returns VBox status code.
 * @param   HCPhysVmxOn      Physical address of VMXON structure.
 */
#if RT_INLINE_ASM_EXTERNAL && !VMX_USE_MSC_INTRINSICS
DECLASM(int) VMXEnable(RTHCPHYS HCPhysVmxOn);
#else
DECLINLINE(int) VMXEnable(RTHCPHYS HCPhysVmxOn)
{
# if VMX_USE_MSC_INTRINSICS
    unsigned char rcMsc = __vmx_on(&HCPhysVmxOn);
    if (RT_LIKELY(rcMsc == 0))
        return VINF_SUCCESS;
    return rcMsc == 2 ? VERR_VMX_INVALID_VMXON_PTR : VERR_VMX_VMXON_FAILED;

# elif RT_INLINE_ASM_GNU_STYLE
#  ifdef RT_ARCH_AMD64
    int rc;
    __asm__ __volatile__ (
       "pushq    %2                                             \n\t"
       ".byte    0xf3, 0x0f, 0xc7, 0x34, 0x24  # VMXON [esp]    \n\t"
       "ja       2f                                             \n\t"
       "je       1f                                             \n\t"
       "movl     $" RT_XSTR(VERR_VMX_INVALID_VMXON_PTR)", %0    \n\t"
       "jmp      2f                                             \n\t"
       "1:                                                      \n\t"
       "movl     $" RT_XSTR(VERR_VMX_VMXON_FAILED)", %0         \n\t"
       "2:                                                      \n\t"
       "add      $8, %%rsp                                      \n\t"
       :"=rm"(rc)
       :"0"(VINF_SUCCESS),
        "ir"(HCPhysVmxOn)                   /* don't allow direct memory reference here, */
                                            /* this would not work with -fomit-frame-pointer */
       :"memory"
       );
    return rc;
#  else
    int rc;
    __asm__ __volatile__ (
       "push     %3                                             \n\t"
       "push     %2                                             \n\t"
       ".byte    0xf3, 0x0f, 0xc7, 0x34, 0x24  # VMXON [esp]    \n\t"
       "ja       2f                                             \n\t"
       "je       1f                                             \n\t"
       "movl     $" RT_XSTR(VERR_VMX_INVALID_VMXON_PTR)", %0    \n\t"
       "jmp      2f                                             \n\t"
       "1:                                                      \n\t"
       "movl     $" RT_XSTR(VERR_VMX_VMXON_FAILED)", %0         \n\t"
       "2:                                                      \n\t"
       "add      $8, %%esp                                      \n\t"
       :"=rm"(rc)
       :"0"(VINF_SUCCESS),
        "ir"((uint32_t)HCPhysVmxOn),        /* don't allow direct memory reference here, */
        "ir"((uint32_t)(HCPhysVmxOn >> 32)) /* this would not work with -fomit-frame-pointer */
       :"memory"
       );
    return rc;
#  endif

# elif defined(RT_ARCH_X86)
    int rc = VINF_SUCCESS;
    __asm
    {
        push    dword ptr [HCPhysVmxOn + 4]
        push    dword ptr [HCPhysVmxOn]
        _emit   0xf3
        _emit   0x0f
        _emit   0xc7
        _emit   0x34
        _emit   0x24     /* VMXON [esp] */
        jnc     vmxon_good
        mov     dword ptr [rc], VERR_VMX_INVALID_VMXON_PTR
        jmp     the_end

vmxon_good:
        jnz     the_end
        mov     dword ptr [rc], VERR_VMX_VMXON_FAILED
the_end:
        add     esp, 8
    }
    return rc;

# else
#  error "Shouldn't be here..."
# endif
}
#endif


/**
 * Executes VMXOFF.
 */
#if RT_INLINE_ASM_EXTERNAL && !VMX_USE_MSC_INTRINSICS
DECLASM(void) VMXDisable(void);
#else
DECLINLINE(void) VMXDisable(void)
{
# if VMX_USE_MSC_INTRINSICS
    __vmx_off();

# elif RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__ (
       ".byte 0x0f, 0x01, 0xc4  # VMXOFF                        \n\t"
       );

# elif defined(RT_ARCH_X86)
    __asm
    {
        _emit   0x0f
        _emit   0x01
        _emit   0xc4   /* VMXOFF */
    }

# else
#  error "Shouldn't be here..."
# endif
}
#endif


/**
 * Executes VMCLEAR.
 *
 * @returns VBox status code.
 * @param   HCPhysVmcs       Physical address of VM control structure.
 */
#if RT_INLINE_ASM_EXTERNAL && !VMX_USE_MSC_INTRINSICS
DECLASM(int) VMXClearVmcs(RTHCPHYS HCPhysVmcs);
#else
DECLINLINE(int) VMXClearVmcs(RTHCPHYS HCPhysVmcs)
{
# if VMX_USE_MSC_INTRINSICS
    unsigned char rcMsc = __vmx_vmclear(&HCPhysVmcs);
    if (RT_LIKELY(rcMsc == 0))
        return VINF_SUCCESS;
    return VERR_VMX_INVALID_VMCS_PTR;

# elif RT_INLINE_ASM_GNU_STYLE
#  ifdef RT_ARCH_AMD64
    int rc;
    __asm__ __volatile__ (
       "pushq   %2                                              \n\t"
       ".byte   0x66, 0x0f, 0xc7, 0x34, 0x24  # VMCLEAR [esp]   \n\t"
       "jnc     1f                                              \n\t"
       "movl    $" RT_XSTR(VERR_VMX_INVALID_VMCS_PTR)", %0      \n\t"
       "1:                                                      \n\t"
       "add     $8, %%rsp                                       \n\t"
       :"=rm"(rc)
       :"0"(VINF_SUCCESS),
        "ir"(HCPhysVmcs)                    /* don't allow direct memory reference here, */
                                            /* this would not work with -fomit-frame-pointer */
       :"memory"
       );
    return rc;
#  else
    int rc;
    __asm__ __volatile__ (
       "push    %3                                              \n\t"
       "push    %2                                              \n\t"
       ".byte   0x66, 0x0f, 0xc7, 0x34, 0x24  # VMCLEAR [esp]   \n\t"
       "jnc     1f                                              \n\t"
       "movl    $" RT_XSTR(VERR_VMX_INVALID_VMCS_PTR)", %0      \n\t"
       "1:                                                      \n\t"
       "add     $8, %%esp                                       \n\t"
       :"=rm"(rc)
       :"0"(VINF_SUCCESS),
        "ir"((uint32_t)HCPhysVmcs),         /* don't allow direct memory reference here, */
        "ir"((uint32_t)(HCPhysVmcs >> 32))  /* this would not work with -fomit-frame-pointer */
       :"memory"
       );
    return rc;
#  endif

# elif defined(RT_ARCH_X86)
    int rc = VINF_SUCCESS;
    __asm
    {
        push    dword ptr [HCPhysVmcs + 4]
        push    dword ptr [HCPhysVmcs]
        _emit   0x66
        _emit   0x0f
        _emit   0xc7
        _emit   0x34
        _emit   0x24     /* VMCLEAR [esp] */
        jnc     success
        mov     dword ptr [rc], VERR_VMX_INVALID_VMCS_PTR
success:
        add     esp, 8
    }
    return rc;

# else
#  error "Shouldn't be here..."
# endif
}
#endif


/**
 * Executes VMPTRLD.
 *
 * @returns VBox status code.
 * @param   HCPhysVmcs       Physical address of VMCS structure.
 */
#if RT_INLINE_ASM_EXTERNAL && !VMX_USE_MSC_INTRINSICS
DECLASM(int) VMXLoadVmcs(RTHCPHYS HCPhysVmcs);
#else
DECLINLINE(int) VMXLoadVmcs(RTHCPHYS HCPhysVmcs)
{
# if VMX_USE_MSC_INTRINSICS
    unsigned char rcMsc = __vmx_vmptrld(&HCPhysVmcs);
    if (RT_LIKELY(rcMsc == 0))
        return VINF_SUCCESS;
    return VERR_VMX_INVALID_VMCS_PTR;

# elif RT_INLINE_ASM_GNU_STYLE
#  ifdef RT_ARCH_AMD64
    int rc;
    __asm__ __volatile__ (
       "pushq   %2                                              \n\t"
       ".byte   0x0f, 0xc7, 0x34, 0x24  # VMPTRLD [esp]         \n\t"
       "jnc     1f                                              \n\t"
       "movl    $" RT_XSTR(VERR_VMX_INVALID_VMCS_PTR)", %0      \n\t"
       "1:                                                      \n\t"
       "add     $8, %%rsp                                       \n\t"
       :"=rm"(rc)
       :"0"(VINF_SUCCESS),
        "ir"(HCPhysVmcs)                    /* don't allow direct memory reference here, */
                                            /* this will not work with -fomit-frame-pointer */
       :"memory"
       );
    return rc;
#  else
    int rc;
    __asm__ __volatile__ (
       "push    %3                                              \n\t"
       "push    %2                                              \n\t"
       ".byte   0x0f, 0xc7, 0x34, 0x24  # VMPTRLD [esp]         \n\t"
       "jnc     1f                                              \n\t"
       "movl    $" RT_XSTR(VERR_VMX_INVALID_VMCS_PTR)", %0      \n\t"
       "1:                                                      \n\t"
       "add     $8, %%esp                                       \n\t"
       :"=rm"(rc)
       :"0"(VINF_SUCCESS),
        "ir"((uint32_t)HCPhysVmcs),         /* don't allow direct memory reference here, */
        "ir"((uint32_t)(HCPhysVmcs >> 32))  /* this will not work with -fomit-frame-pointer */
       :"memory"
       );
    return rc;
#  endif

# elif defined(RT_ARCH_X86)
    int rc = VINF_SUCCESS;
    __asm
    {
        push    dword ptr [HCPhysVmcs + 4]
        push    dword ptr [HCPhysVmcs]
        _emit   0x0f
        _emit   0xc7
        _emit   0x34
        _emit   0x24     /* VMPTRLD [esp] */
        jnc     success
        mov     dword ptr [rc], VERR_VMX_INVALID_VMCS_PTR
success:
        add     esp, 8
    }
    return rc;

# else
#  error "Shouldn't be here..."
# endif
}
#endif


/**
 * Executes VMPTRST.
 *
 * @returns VBox status code.
 * @param   pHCPhysVmcs    Where to store the physical address of the current
 *                         VMCS.
 */
DECLASM(int) VMXGetCurrentVmcs(RTHCPHYS *pHCPhysVmcs);


/**
 * Executes VMWRITE for a 32-bit field.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS.
 * @retval  VERR_VMX_INVALID_VMCS_PTR.
 * @retval  VERR_VMX_INVALID_VMCS_FIELD.
 *
 * @param   uFieldEnc       VMCS field encoding.
 * @param   u32Val          The 32-bit value to set.
 *
 * @remarks The values of the two status codes can be OR'ed together, the result
 *          will be VERR_VMX_INVALID_VMCS_PTR.
 */
#if RT_INLINE_ASM_EXTERNAL && !VMX_USE_MSC_INTRINSICS
DECLASM(int) VMXWriteVmcs32(uint32_t uFieldEnc, uint32_t u32Val);
#else
DECLINLINE(int) VMXWriteVmcs32(uint32_t uFieldEnc, uint32_t u32Val)
{
# if VMX_USE_MSC_INTRINSICS
#  ifdef VBOX_WITH_VMREAD_VMWRITE_NOCHECK
    __vmx_vmwrite(uFieldEnc, u32Val);
    return VINF_SUCCESS;
#  else
    unsigned char rcMsc = __vmx_vmwrite(uFieldEnc, u32Val);
     if (RT_LIKELY(rcMsc == 0))
         return VINF_SUCCESS;
     return rcMsc == 2 ? VERR_VMX_INVALID_VMCS_PTR : VERR_VMX_INVALID_VMCS_FIELD;
#  endif

# elif RT_INLINE_ASM_GNU_STYLE
#  ifdef VBOX_WITH_VMREAD_VMWRITE_NOCHECK
    __asm__ __volatile__ (
       ".byte  0x0f, 0x79, 0xc2        # VMWRITE eax, edx       \n\t"
       :
       :"a"(uFieldEnc),
        "d"(u32Val)
       );
    return VINF_SUCCESS;
#  else
     int rc;
    __asm__ __volatile__ (
       ".byte  0x0f, 0x79, 0xc2        # VMWRITE eax, edx       \n\t"
       "ja     2f                                               \n\t"
       "je     1f                                               \n\t"
       "movl   $" RT_XSTR(VERR_VMX_INVALID_VMCS_PTR)", %0       \n\t"
       "jmp    2f                                               \n\t"
       "1:                                                      \n\t"
       "movl   $" RT_XSTR(VERR_VMX_INVALID_VMCS_FIELD)", %0     \n\t"
       "2:                                                      \n\t"
       :"=rm"(rc)
       :"0"(VINF_SUCCESS),
        "a"(uFieldEnc),
        "d"(u32Val)
       );
    return rc;
#  endif

# elif defined(RT_ARCH_X86)
    int rc = VINF_SUCCESS;
    __asm
    {
        push   dword ptr [u32Val]
        mov    eax, [uFieldEnc]
        _emit  0x0f
        _emit  0x79
        _emit  0x04
        _emit  0x24     /* VMWRITE eax, [esp] */
        jnc    valid_vmcs
        mov    dword ptr [rc], VERR_VMX_INVALID_VMCS_PTR
        jmp    the_end
valid_vmcs:
        jnz    the_end
        mov    dword ptr [rc], VERR_VMX_INVALID_VMCS_FIELD
the_end:
        add    esp, 4
    }
    return rc;

# else
#  error "Shouldn't be here..."
# endif
}
#endif


/**
 * Executes VMWRITE for a 64-bit field.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS.
 * @retval  VERR_VMX_INVALID_VMCS_PTR.
 * @retval  VERR_VMX_INVALID_VMCS_FIELD.
 *
 * @param   uFieldEnc       The VMCS field encoding.
 * @param   u64Val          The 16, 32 or 64-bit value to set.
 *
 * @remarks The values of the two status codes can be OR'ed together, the result
 *          will be VERR_VMX_INVALID_VMCS_PTR.
 */
#if defined(RT_ARCH_X86) || (RT_INLINE_ASM_EXTERNAL && !VMX_USE_MSC_INTRINSICS)
DECLASM(int) VMXWriteVmcs64(uint32_t uFieldEnc, uint64_t u64Val);
#else
DECLINLINE(int) VMXWriteVmcs64(uint32_t uFieldEnc, uint64_t u64Val)
{
# if VMX_USE_MSC_INTRINSICS
#  ifdef VBOX_WITH_VMREAD_VMWRITE_NOCHECK
    __vmx_vmwrite(uFieldEnc, u64Val);
    return VINF_SUCCESS;
#  else
    unsigned char rcMsc = __vmx_vmwrite(uFieldEnc, u64Val);
    if (RT_LIKELY(rcMsc == 0))
        return VINF_SUCCESS;
    return rcMsc == 2 ? VERR_VMX_INVALID_VMCS_PTR : VERR_VMX_INVALID_VMCS_FIELD;
#  endif

# elif RT_INLINE_ASM_GNU_STYLE
#  ifdef VBOX_WITH_VMREAD_VMWRITE_NOCHECK
    __asm__ __volatile__ (
       ".byte  0x0f, 0x79, 0xc2        # VMWRITE eax, edx        \n\t"
       :
       :"a"(uFieldEnc),
        "d"(u64Val)
       );
    return VINF_SUCCESS;
#  else
    int rc;
    __asm__ __volatile__ (
       ".byte  0x0f, 0x79, 0xc2        # VMWRITE eax, edx        \n\t"
       "ja     2f                                                \n\t"
       "je     1f                                                \n\t"
       "movl   $" RT_XSTR(VERR_VMX_INVALID_VMCS_PTR)", %0        \n\t"
       "jmp    2f                                                \n\t"
       "1:                                                       \n\t"
       "movl   $" RT_XSTR(VERR_VMX_INVALID_VMCS_FIELD)", %0      \n\t"
       "2:                                                       \n\t"
       :"=rm"(rc)
       :"0"(VINF_SUCCESS),
        "a"(uFieldEnc),
        "d"(u64Val)
       );
    return rc;
#  endif

# else
#  error "Shouldn't be here..."
# endif
}
#endif


/**
 * Executes VMWRITE for a 16-bit VMCS field.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS.
 * @retval  VERR_VMX_INVALID_VMCS_PTR.
 * @retval  VERR_VMX_INVALID_VMCS_FIELD.
 *
 * @param   uVmcsField  The VMCS field.
 * @param   u16Val      The 16-bit value to set.
 *
 * @remarks The values of the two status codes can be OR'ed together, the result
 *          will be VERR_VMX_INVALID_VMCS_PTR.
 */
DECLINLINE(int) VMXWriteVmcs16(uint32_t uVmcsField, uint16_t u16Val)
{
    AssertMsg(RT_BF_GET(uVmcsField, VMX_BF_VMCSFIELD_WIDTH) == VMX_VMCSFIELD_WIDTH_16BIT, ("%#RX32\n", uVmcsField));
    return VMXWriteVmcs32(uVmcsField, u16Val);
}


/**
 * Executes VMWRITE for a natural-width VMCS field.
 */
#ifdef RT_ARCH_AMD64
# define VMXWriteVmcsNw         VMXWriteVmcs64
#else
# define VMXWriteVmcsNw         VMXWriteVmcs32
#endif


/**
 * Invalidate a page using INVEPT.
 *
 * @returns VBox status code.
 * @param   enmFlush        Type of flush.
 * @param   pDescriptor     Pointer to the descriptor.
 */
DECLASM(int) VMXR0InvEPT(VMXTLBFLUSHEPT enmFlush, uint64_t *pDescriptor);


/**
 * Invalidate a page using INVVPID.
 *
 * @returns VBox status code.
 * @param   enmFlush        Type of flush.
 * @param   pDescriptor     Pointer to the descriptor.
 */
DECLASM(int) VMXR0InvVPID(VMXTLBFLUSHVPID enmFlush, uint64_t *pDescriptor);


/**
 * Executes VMREAD for a 32-bit field.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS.
 * @retval  VERR_VMX_INVALID_VMCS_PTR.
 * @retval  VERR_VMX_INVALID_VMCS_FIELD.
 *
 * @param   uFieldEnc       The VMCS field encoding.
 * @param   pData           Where to store VMCS field value.
 *
 * @remarks The values of the two status codes can be OR'ed together, the result
 *          will be VERR_VMX_INVALID_VMCS_PTR.
 */
#if RT_INLINE_ASM_EXTERNAL && !VMX_USE_MSC_INTRINSICS
DECLASM(int) VMXReadVmcs32(uint32_t uFieldEnc, uint32_t *pData);
#else
DECLINLINE(int) VMXReadVmcs32(uint32_t uFieldEnc, uint32_t *pData)
{
# if VMX_USE_MSC_INTRINSICS
#  ifdef VBOX_WITH_VMREAD_VMWRITE_NOCHECK
    uint64_t u64Tmp = 0;
    __vmx_vmread(uFieldEnc, &u64Tmp);
    *pData = (uint32_t)u64Tmp;
    return VINF_SUCCESS;
#  else
    unsigned char rcMsc;
    uint64_t u64Tmp;
    rcMsc = __vmx_vmread(uFieldEnc, &u64Tmp);
    *pData = (uint32_t)u64Tmp;
    if (RT_LIKELY(rcMsc == 0))
        return VINF_SUCCESS;
    return rcMsc == 2 ? VERR_VMX_INVALID_VMCS_PTR : VERR_VMX_INVALID_VMCS_FIELD;
#  endif

# elif VMX_USE_GNU_STYLE_INLINE_VMX_INSTRUCTIONS
    RTCCUINTREG uTmp = 0;
#  ifdef VBOX_WITH_VMREAD_VMWRITE_NOCHECK
    __asm__ __volatile__("vmread  %[uField],%[uDst]"
                         : [uDst] "=mr" (uTmp)
                         : [uField] "r" ((RTCCUINTREG)uFieldEnc));
    *pData = (uint32_t)uTmp;
    return VINF_SUCCESS;
#  else
#if 0
    int      rc;
    __asm__ __volatile__("vmread  %[uField],%[uDst]\n\t"
                         "movl    %[rcSuccess],%[rc]\n\t"
#    if VMX_USE_GNU_STYLE_INLINE_SECTION_TRICK
                         "jna     1f\n\t"
                         ".section .text.vmread_failures, \"ax?\"\n\t"
                         "1:\n\t"
                         "movl    %[rcInvalidVmcsPtr],%[rc]\n\t"
                         "jnz     2f\n\t"
                         "movl    %[rcInvalidVmcsField],%[rc]\n\t"
                         "2:\n\t"
                         "jmp     3f\n\t"
                         ".previous\n\t"
                         "3:\n\t"
#    else
                         "ja      1f\n\t"
                         "movl    %[rcInvalidVmcsPtr],%[rc]\n\t"
                         "jnz     1f\n\t"
                         "movl    %[rcInvalidVmcsField],%[rc]\n\t"
                         "1:\n\t"
#    endif
                         : [uDst] "=mr" (uTmp)
                         , [rc] "=r" (rc)
                         : [uField] "r" ((RTCCUINTREG)uFieldEnc)
                         , [rcSuccess] "i" (VINF_SUCCESS)
                         , [rcInvalidVmcsPtr] "i" (VERR_VMX_INVALID_VMCS_PTR)
                         , [rcInvalidVmcsField] "i" (VERR_VMX_INVALID_VMCS_FIELD));
    *pData = uTmp;
    return rc;
#else
    int fSuccess, fFieldError;
    __asm__ __volatile__("vmread  %[uField],%[uDst]"
                         : [uDst] "=mr" (uTmp)
                         , "=@cca" (fSuccess)
                         , "=@ccnc" (fFieldError)
                         : [uField] "r" ((RTCCUINTREG)uFieldEnc));
    *pData = uTmp;
    return RT_LIKELY(fSuccess) ? VINF_SUCCESS : fFieldError ? VERR_VMX_INVALID_VMCS_FIELD : VERR_VMX_INVALID_VMCS_PTR;
#endif
#  endif

# elif RT_INLINE_ASM_GNU_STYLE
#  ifdef VBOX_WITH_VMREAD_VMWRITE_NOCHECK
    __asm__ __volatile__ (
       ".byte  0x0f, 0x78, 0xc2        # VMREAD eax, edx         \n\t"
       :"=d"(*pData)
       :"a"(uFieldEnc),
        "d"(0)
       );
    return VINF_SUCCESS;
#  else
    int rc;
    __asm__ __volatile__ (
       "movl   $" RT_XSTR(VINF_SUCCESS)", %0                     \n\t"
       ".byte  0x0f, 0x78, 0xc2        # VMREAD eax, edx         \n\t"
       "ja     2f                                                \n\t"
       "je     1f                                                \n\t"
       "movl   $" RT_XSTR(VERR_VMX_INVALID_VMCS_PTR)", %0        \n\t"
       "jmp    2f                                                \n\t"
       "1:                                                       \n\t"
       "movl   $" RT_XSTR(VERR_VMX_INVALID_VMCS_FIELD)", %0      \n\t"
       "2:                                                       \n\t"
       :"=&r"(rc),
        "=d"(*pData)
       :"a"(uFieldEnc),
        "d"(0)
       );
    return rc;
#  endif

# elif defined(RT_ARCH_X86)
    int rc = VINF_SUCCESS;
    __asm
    {
        sub     esp, 4
        mov     dword ptr [esp], 0
        mov     eax, [uFieldEnc]
        _emit   0x0f
        _emit   0x78
        _emit   0x04
        _emit   0x24     /* VMREAD eax, [esp] */
        mov     edx, pData
        pop     dword ptr [edx]
        jnc     valid_vmcs
        mov     dword ptr [rc], VERR_VMX_INVALID_VMCS_PTR
        jmp     the_end
valid_vmcs:
        jnz     the_end
        mov     dword ptr [rc], VERR_VMX_INVALID_VMCS_FIELD
the_end:
    }
    return rc;

# else
#  error "Shouldn't be here..."
# endif
}
#endif


/**
 * Executes VMREAD for a 64-bit field.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS.
 * @retval  VERR_VMX_INVALID_VMCS_PTR.
 * @retval  VERR_VMX_INVALID_VMCS_FIELD.
 *
 * @param   uFieldEnc       The VMCS field encoding.
 * @param   pData           Where to store VMCS field value.
 *
 * @remarks The values of the two status codes can be OR'ed together, the result
 *          will be VERR_VMX_INVALID_VMCS_PTR.
 */
#if defined(RT_ARCH_X86) || (RT_INLINE_ASM_EXTERNAL && !VMX_USE_MSC_INTRINSICS)
DECLASM(int) VMXReadVmcs64(uint32_t uFieldEnc, uint64_t *pData);
#else
DECLINLINE(int) VMXReadVmcs64(uint32_t uFieldEnc, uint64_t *pData)
{
# if VMX_USE_MSC_INTRINSICS
#  ifdef VBOX_WITH_VMREAD_VMWRITE_NOCHECK
    __vmx_vmread(uFieldEnc, pData);
    return VINF_SUCCESS;
#  else
    unsigned char rcMsc;
    rcMsc = __vmx_vmread(uFieldEnc, pData);
    if (RT_LIKELY(rcMsc == 0))
        return VINF_SUCCESS;
    return rcMsc == 2 ? VERR_VMX_INVALID_VMCS_PTR : VERR_VMX_INVALID_VMCS_FIELD;
#  endif

# elif VMX_USE_GNU_STYLE_INLINE_VMX_INSTRUCTIONS
    uint64_t uTmp = 0;
#  ifdef VBOX_WITH_VMREAD_VMWRITE_NOCHECK
    __asm__ __volatile__("vmreadq %[uField],%[uDst]"
                         : [uDst] "=m" (uTmp)
                         : [uField] "r" ((uint64_t)uFieldEnc));
    *pData = uTmp;
    return VINF_SUCCESS;
#  elif 0
    int      rc;
    __asm__ __volatile__("vmreadq %[uField],%[uDst]\n\t"
                         "movl    %[rcSuccess],%[rc]\n\t"
#    if VMX_USE_GNU_STYLE_INLINE_SECTION_TRICK
                         "jna     1f\n\t"
                         ".section .text.vmread_failures, \"ax?\"\n\t"
                         "1:\n\t"
                         "movl    %[rcInvalidVmcsPtr],%[rc]\n\t"
                         "jnz     2f\n\t"
                         "movl    %[rcInvalidVmcsField],%[rc]\n\t"
                         "2:\n\t"
                         "jmp     3f\n\t"
                         ".previous\n\t"
                         "3:\n\t"
#    else
                         "ja      1f\n\t"
                         "movl    %[rcInvalidVmcsPtr],%[rc]\n\t"
                         "jnz     1f\n\t"
                         "movl    %[rcInvalidVmcsField],%[rc]\n\t"
                         "1:\n\t"
#    endif
                         : [uDst] "=mr" (uTmp)
                         , [rc] "=r" (rc)
                         : [uField] "r" ((uint64_t)uFieldEnc)
                         , [rcSuccess] "i" (VINF_SUCCESS)
                         , [rcInvalidVmcsPtr] "i" (VERR_VMX_INVALID_VMCS_PTR)
                         , [rcInvalidVmcsField] "i" (VERR_VMX_INVALID_VMCS_FIELD)
                         );
    *pData = uTmp;
    return rc;
#  else
    int fSuccess, fFieldError;
    __asm__ __volatile__("vmread  %[uField],%[uDst]"
                         : [uDst] "=mr" (uTmp)
                         , "=@cca" (fSuccess)
                         , "=@ccnc" (fFieldError)
                         : [uField] "r" ((RTCCUINTREG)uFieldEnc));
    *pData = uTmp;
    return RT_LIKELY(fSuccess) ? VINF_SUCCESS : fFieldError ? VERR_VMX_INVALID_VMCS_FIELD : VERR_VMX_INVALID_VMCS_PTR;
#  endif

# elif RT_INLINE_ASM_GNU_STYLE
#  ifdef VBOX_WITH_VMREAD_VMWRITE_NOCHECK
    __asm__ __volatile__ (
       ".byte  0x0f, 0x78, 0xc2        # VMREAD eax, edx         \n\t"
       :"=d"(*pData)
       :"a"(uFieldEnc),
        "d"(0)
       );
    return VINF_SUCCESS;
#  else
    int rc;
    __asm__ __volatile__ (
       "movl   $" RT_XSTR(VINF_SUCCESS)", %0                     \n\t"
       ".byte  0x0f, 0x78, 0xc2        # VMREAD eax, edx         \n\t"
       "ja     2f                                                \n\t"
       "je     1f                                                \n\t"
       "movl   $" RT_XSTR(VERR_VMX_INVALID_VMCS_PTR)", %0        \n\t"
       "jmp    2f                                                \n\t"
       "1:                                                       \n\t"
       "movl   $" RT_XSTR(VERR_VMX_INVALID_VMCS_FIELD)", %0      \n\t"
       "2:                                                       \n\t"
       :"=&r"(rc),
        "=d"(*pData)
       :"a"(uFieldEnc),
        "d"(0)
       );
    return rc;
#  endif

# else
#  error "Shouldn't be here..."
# endif
}
#endif


/**
 * Executes VMREAD for a 16-bit field.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS.
 * @retval  VERR_VMX_INVALID_VMCS_PTR.
 * @retval  VERR_VMX_INVALID_VMCS_FIELD.
 *
 * @param   uVmcsField  The VMCS field.
 * @param   pData       Where to store VMCS field value.
 *
 * @remarks The values of the two status codes can be OR'ed together, the result
 *          will be VERR_VMX_INVALID_VMCS_PTR.
 */
DECLINLINE(int) VMXReadVmcs16(uint32_t uVmcsField, uint16_t *pData)
{
    uint32_t u32Tmp;
    int      rc;
    AssertMsg(RT_BF_GET(uVmcsField, VMX_BF_VMCSFIELD_WIDTH) == VMX_VMCSFIELD_WIDTH_16BIT, ("%#RX32\n", uVmcsField));
    rc = VMXReadVmcs32(uVmcsField, &u32Tmp);
    *pData = (uint16_t)u32Tmp;
    return rc;
}


/**
 * Executes VMREAD for a natural-width VMCS field.
 */
#ifdef RT_ARCH_AMD64
# define VMXReadVmcsNw          VMXReadVmcs64
#else
# define VMXReadVmcsNw          VMXReadVmcs32
#endif

#endif /* RT_ARCH_AMD64 || RT_ARCH_X86 */

/** @} */

#endif /* !VBOX_INCLUDED_vmm_hmvmxinline_h */

