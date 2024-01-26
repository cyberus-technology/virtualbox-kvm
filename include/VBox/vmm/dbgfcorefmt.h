/** @file
 * DBGF - Debugger Facility, VM Core File Format.
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

#ifndef VBOX_INCLUDED_vmm_dbgfcorefmt_h
#define VBOX_INCLUDED_vmm_dbgfcorefmt_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#include <VBox/vmm/cpumctx.h>
#include <iprt/assertcompile.h>


RT_C_DECLS_BEGIN


/** @defgroup grp_dbgf_corefmt  VM Core File Format
 * @ingroup grp_dbgf
 *
 * @todo Add description of the core file format and how the structures in this
 *       file relate to it.  Point to X86XSAVEAREA in x86.h for the CPU's
 *       FPU/SSE/AVX/XXX state.
 * @todo Add the note names.
 *
 * @{
 */

/** DBGCORECOREDESCRIPTOR::u32Magic. */
#define DBGFCORE_MAGIC          UINT32_C(0xc01ac0de)
/** DBGCORECOREDESCRIPTOR::u32FmtVersion. */
#define DBGFCORE_FMT_VERSION    UINT32_C(0x00010006)

/**
 * An x86 segment selector.
 */
typedef struct DBGFCORESEL
{
    uint64_t        uBase;
    uint32_t        uLimit;
    uint32_t        uAttr;
    uint16_t        uSel;
    uint16_t        uReserved0;
    uint32_t        uReserved1;
} DBGFCORESEL;
AssertCompileSizeAlignment(DBGFCORESEL, 8);

/**
 * A gdtr/ldtr descriptor.
 */
typedef struct DBGFCOREXDTR
{
    uint64_t        uAddr;
    uint32_t        cb;
    uint32_t        uReserved0;
} DBGFCOREXDTR;
AssertCompileSizeAlignment(DBGFCOREXDTR, 8);

/**
 * A simpler to parse CPU dump than CPUMCTX.
 *
 * Please bump DBGFCORE_FMT_VERSION by 1 if you make any changes to this
 * structure.
 */
typedef struct DBGFCORECPU
{
    uint64_t            rax;
    uint64_t            rbx;
    uint64_t            rcx;
    uint64_t            rdx;
    uint64_t            rsi;
    uint64_t            rdi;
    uint64_t            r8;
    uint64_t            r9;
    uint64_t            r10;
    uint64_t            r11;
    uint64_t            r12;
    uint64_t            r13;
    uint64_t            r14;
    uint64_t            r15;
    uint64_t            rip;
    uint64_t            rsp;
    uint64_t            rbp;
    uint64_t            rflags;
    DBGFCORESEL         cs;
    DBGFCORESEL         ds;
    DBGFCORESEL         es;
    DBGFCORESEL         fs;
    DBGFCORESEL         gs;
    DBGFCORESEL         ss;
    uint64_t            cr0;
    uint64_t            cr2;
    uint64_t            cr3;
    uint64_t            cr4;
    uint64_t            dr[8];
    DBGFCOREXDTR        gdtr;
    DBGFCOREXDTR        idtr;
    DBGFCORESEL         ldtr;
    DBGFCORESEL         tr;
    struct
    {
        uint64_t        cs;
        uint64_t        eip;
        uint64_t        esp;
    } sysenter;
    uint64_t            msrEFER;
    uint64_t            msrSTAR;
    uint64_t            msrPAT;
    uint64_t            msrLSTAR;
    uint64_t            msrCSTAR;
    uint64_t            msrSFMASK;
    uint64_t            msrKernelGSBase;
    uint64_t            msrApicBase;
    uint64_t            msrTscAux;
    uint64_t            aXcr[2];
    uint32_t            cbExt;
    uint32_t            uPadding0;
    X86XSAVEAREA        ext;
} DBGFCORECPU;
/** Pointer to a DBGF-core CPU. */
typedef DBGFCORECPU *PDBGFCORECPU;
/** Pointer to the const DBGF-core CPU. */
typedef const DBGFCORECPU *PCDBGFCORECPU;
AssertCompileMemberAlignment(DBGFCORECPU, cr0,     8);
AssertCompileMemberAlignment(DBGFCORECPU, msrEFER, 8);
AssertCompileMemberAlignment(DBGFCORECPU, ext,     8);
AssertCompileSizeAlignment(DBGFCORECPU, 8);

/**
 * The DBGF Core descriptor.
 */
typedef struct DBGFCOREDESCRIPTOR
{
    /** The core file magic (DBGFCORE_MAGIC) */
    uint32_t                u32Magic;
    /** The core file format version (DBGFCORE_FMT_VERSION). */
    uint32_t                u32FmtVersion;
    /** Size of this structure (sizeof(DBGFCOREDESCRIPTOR)). */
    uint32_t                cbSelf;
    /** VirtualBox version. */
    uint32_t                u32VBoxVersion;
    /** VirtualBox revision. */
    uint32_t                u32VBoxRevision;
    /** Number of CPUs. */
    uint32_t                cCpus;
} DBGFCOREDESCRIPTOR;
AssertCompileSizeAlignment(DBGFCOREDESCRIPTOR, 8);
/** Pointer to DBGFCOREDESCRIPTOR data. */
typedef DBGFCOREDESCRIPTOR  *PDBGFCOREDESCRIPTOR;

/** @}  */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_dbgfcorefmt_h */

