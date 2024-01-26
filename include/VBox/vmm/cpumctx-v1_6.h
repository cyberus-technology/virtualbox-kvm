/** @file
 * CPUM - CPU Monitor(/ Manager), Context Structures from v1.6 (saved state).
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

#ifndef VBOX_INCLUDED_vmm_cpumctx_v1_6_h
#define VBOX_INCLUDED_vmm_cpumctx_v1_6_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/x86.h>
#include <VBox/vmm/cpumctx.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_cpum_ctx_v1_6  The CPUM Context Structures from v1.6
 * @ingroup grp_cpum
 * @{
 */

#pragma pack(1)
/** IDTR from version 1.6 */
typedef struct VBOXIDTR_VER1_6
{
    /** Size of the IDT. */
    uint16_t    cbIdt;
    /** Address of the IDT. */
    uint32_t    pIdt;
} VBOXIDTR_VER1_6;
#pragma pack()

#pragma pack(1)
/** GDTR from version 1.6 */
typedef struct VBOXGDTR_VER1_6
{
    /** Size of the GDT. */
    uint16_t    cbGdt;
    /** Address of the GDT. */
    uint32_t    pGdt;
} VBOXGDTR_VER1_6;
#pragma pack()


/**
 * Selector hidden registers, for version 1.6 saved state.
 */
typedef struct CPUMSELREGHID_VER1_6
{
    /** Base register. */
    uint32_t    u32Base;
    /** Limit (expanded). */
    uint32_t    u32Limit;
    /** Flags.
     * This is the high 32-bit word of the descriptor entry.
     * Only the flags, dpl and type are used. */
    X86DESCATTR Attr;
} CPUMSELREGHID_VER1_6;

/**
 * CPU context, for version 1.6 saved state.
 * @remarks PATM uses this, which is why it has to be here.
 */
# pragma pack(1)
typedef struct CPUMCTX_VER1_6
{
    /** FPU state. (16-byte alignment)
     * @todo This doesn't have to be in X86FXSTATE on CPUs without fxsr - we need a type for the
     *       actual format or convert it (waste of time).  */
    X86FXSTATE      fpu;

    /** CPUMCTXCORE Part.
     * @{ */
    union
    {
        uint32_t        edi;
        uint64_t        rdi;
    } CPUM_UNION_NM(rdi);
    union
    {
        uint32_t        esi;
        uint64_t        rsi;
    } CPUM_UNION_NM(rsi);
    union
    {
        uint32_t        ebp;
        uint64_t        rbp;
    } CPUM_UNION_NM(rbp);
    union
    {
        uint32_t        eax;
        uint64_t        rax;
    } CPUM_UNION_NM(rax);
    union
    {
        uint32_t        ebx;
        uint64_t        rbx;
    } CPUM_UNION_NM(rbx);
    union
    {
        uint32_t        edx;
        uint64_t        rdx;
    } CPUM_UNION_NM(rdx);
    union
    {
        uint32_t        ecx;
        uint64_t        rcx;
    } CPUM_UNION_NM(rcx);
    /** @note We rely on the exact layout, because we use lss esp, [] in the
     *        switcher. */
    uint32_t        esp;
    RTSEL           ss;
    RTSEL           ssPadding;
    /* Note: no overlap with esp here. */
    uint64_t        rsp_notused;

    RTSEL           gs;
    RTSEL           gsPadding;
    RTSEL           fs;
    RTSEL           fsPadding;
    RTSEL           es;
    RTSEL           esPadding;
    RTSEL           ds;
    RTSEL           dsPadding;
    RTSEL           cs;
    RTSEL           csPadding[3];   /**< 3 words to force 8 byte alignment for the remainder. */

    union
    {
        X86EFLAGS       eflags;
        X86RFLAGS       rflags;
    } CPUM_UNION_NM(rflags);
    union
    {
        uint32_t        eip;
        uint64_t        rip;
    } CPUM_UNION_NM(rip);

    uint64_t            r8;
    uint64_t            r9;
    uint64_t            r10;
    uint64_t            r11;
    uint64_t            r12;
    uint64_t            r13;
    uint64_t            r14;
    uint64_t            r15;

    /** Hidden selector registers.
     * @{ */
    CPUMSELREGHID_VER1_6   esHid;
    CPUMSELREGHID_VER1_6   csHid;
    CPUMSELREGHID_VER1_6   ssHid;
    CPUMSELREGHID_VER1_6   dsHid;
    CPUMSELREGHID_VER1_6   fsHid;
    CPUMSELREGHID_VER1_6   gsHid;
    /** @} */

    /** @} */

    /** Control registers.
     * @{ */
    uint64_t        cr0;
    uint64_t        cr2;
    uint64_t        cr3;
    uint64_t        cr4;
    uint64_t        cr8;
    /** @} */

    /** Debug registers.
     * @{ */
    uint64_t        dr0;
    uint64_t        dr1;
    uint64_t        dr2;
    uint64_t        dr3;
    uint64_t        dr4; /**< @todo remove dr4 and dr5. */
    uint64_t        dr5;
    uint64_t        dr6;
    uint64_t        dr7;
    /* DR8-15 are currently not supported */
    /** @} */

    /** Global Descriptor Table register. */
    VBOXGDTR_VER1_6 gdtr;
    uint16_t        gdtrPadding;
    uint32_t        gdtrPadding64;/** @todo fix this hack */
    /** Interrupt Descriptor Table register. */
    VBOXIDTR_VER1_6 idtr;
    uint16_t        idtrPadding;
    uint32_t        idtrPadding64;/** @todo fix this hack */
    /** The task register.
     * Only the guest context uses all the members. */
    RTSEL           ldtr;
    RTSEL           ldtrPadding;
    /** The task register.
     * Only the guest context uses all the members. */
    RTSEL           tr;
    RTSEL           trPadding;

    /** The sysenter msr registers.
     * This member is not used by the hypervisor context. */
    CPUMSYSENTER    SysEnter;

    /** System MSRs.
     * @{ */
    uint64_t        msrEFER;
    uint64_t        msrSTAR;
    uint64_t        msrPAT;
    uint64_t        msrLSTAR;
    uint64_t        msrCSTAR;
    uint64_t        msrSFMASK;
    uint64_t        msrFSBASE;
    uint64_t        msrGSBASE;
    uint64_t        msrKERNELGSBASE;
    /** @} */

    /** Hidden selector registers.
     * @{ */
    CPUMSELREGHID_VER1_6   ldtrHid;
    CPUMSELREGHID_VER1_6   trHid;
    /** @} */

    /** padding to get 32byte aligned size. */
    uint32_t        padding[2];
} CPUMCTX_VER1_6;
# pragma pack()

/** @}  */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_cpumctx_v1_6_h */

