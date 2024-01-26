/** @file
 * CPUM - CPU Monitor(/ Manager), Context Structures.
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

#ifndef VBOX_INCLUDED_vmm_cpumctx_h
#define VBOX_INCLUDED_vmm_cpumctx_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#ifndef VBOX_FOR_DTRACE_LIB
# include <iprt/x86.h>
# include <VBox/types.h>
# include <VBox/vmm/hm_svm.h>
# include <VBox/vmm/hm_vmx.h>
#else
# pragma D depends_on library x86.d
#endif


RT_C_DECLS_BEGIN

/** @defgroup grp_cpum_ctx  The CPUM Context Structures
 * @ingroup grp_cpum
 * @{
 */

/**
 * Selector hidden registers.
 */
typedef struct CPUMSELREG
{
    /** The selector register. */
    RTSEL       Sel;
    /** Padding, don't use. */
    RTSEL       PaddingSel;
    /** The selector which info resides in u64Base, u32Limit and Attr, provided
     * that CPUMSELREG_FLAGS_VALID is set. */
    RTSEL       ValidSel;
    /** Flags, see CPUMSELREG_FLAGS_XXX. */
    uint16_t    fFlags;

    /** Base register.
     *
     * Long mode remarks:
     *  - Unused in long mode for CS, DS, ES, SS
     *  - 32 bits for FS & GS; FS(GS)_BASE msr used for the base address
     *  - 64 bits for TR & LDTR
     */
    uint64_t    u64Base;
    /** Limit (expanded). */
    uint32_t    u32Limit;
    /** Flags.
     * This is the high 32-bit word of the descriptor entry.
     * Only the flags, dpl and type are used. */
    X86DESCATTR Attr;
} CPUMSELREG;
#ifndef VBOX_FOR_DTRACE_LIB
AssertCompileSize(CPUMSELREG, 24);
#endif

/** @name CPUMSELREG_FLAGS_XXX - CPUMSELREG::fFlags values.
 * @{ */
#define CPUMSELREG_FLAGS_VALID      UINT16_C(0x0001)
#define CPUMSELREG_FLAGS_STALE      UINT16_C(0x0002)
#define CPUMSELREG_FLAGS_VALID_MASK UINT16_C(0x0003)
/** @} */

/** Checks if the hidden parts of the selector register are valid. */
#define CPUMSELREG_ARE_HIDDEN_PARTS_VALID(a_pVCpu, a_pSelReg) \
    (   ((a_pSelReg)->fFlags & CPUMSELREG_FLAGS_VALID) \
     && (a_pSelReg)->ValidSel == (a_pSelReg)->Sel  )

/** Old type used for the hidden register part.
 * @deprecated  */
typedef CPUMSELREG CPUMSELREGHID;

/**
 * The sysenter register set.
 */
typedef struct CPUMSYSENTER
{
    /** Ring 0 cs.
     * This value +  8 is the Ring 0 ss.
     * This value + 16 is the Ring 3 cs.
     * This value + 24 is the Ring 3 ss.
     */
    uint64_t    cs;
    /** Ring 0 eip. */
    uint64_t    eip;
    /** Ring 0 esp. */
    uint64_t    esp;
} CPUMSYSENTER;

/** @def CPUM_UNION_NM
 * For compilers (like DTrace) that does not grok nameless unions, we have a
 * little hack to make them palatable.
 */
/** @def CPUM_STRUCT_NM
 * For compilers (like DTrace) that does not grok nameless structs (it is
 * non-standard C++), we have a little hack to make them palatable.
 */
#ifdef VBOX_FOR_DTRACE_LIB
# define CPUM_UNION_NM(a_Nm)  a_Nm
# define CPUM_STRUCT_NM(a_Nm) a_Nm
#elif defined(IPRT_WITHOUT_NAMED_UNIONS_AND_STRUCTS)
# define CPUM_UNION_NM(a_Nm)  a_Nm
# define CPUM_STRUCT_NM(a_Nm) a_Nm
#else
# define CPUM_UNION_NM(a_Nm)
# define CPUM_STRUCT_NM(a_Nm)
#endif
/** @def CPUM_UNION_STRUCT_NM
 * Combines CPUM_UNION_NM and CPUM_STRUCT_NM to avoid hitting the right side of
 * the screen in the compile time assertions.
 */
#define CPUM_UNION_STRUCT_NM(a_UnionNm, a_StructNm) CPUM_UNION_NM(a_UnionNm .) CPUM_STRUCT_NM(a_StructNm)

/** A general register (union). */
typedef union CPUMCTXGREG
{
    /** Natural unsigned integer view. */
    uint64_t            u;
    /** 64-bit view. */
    uint64_t            u64;
    /** 32-bit view. */
    uint32_t            u32;
    /** 16-bit view. */
    uint16_t            u16;
    /** 8-bit view. */
    uint8_t             u8;
    /** 8-bit low/high view.    */
    RT_GCC_EXTENSION struct
    {
        /** Low byte (al, cl, dl, bl, ++). */
        uint8_t         bLo;
        /** High byte in the first word - ah, ch, dh, bh. */
        uint8_t         bHi;
    } CPUM_STRUCT_NM(s);
} CPUMCTXGREG;
#ifndef VBOX_FOR_DTRACE_LIB
AssertCompileSize(CPUMCTXGREG, 8);
AssertCompileMemberOffset(CPUMCTXGREG, CPUM_STRUCT_NM(s.) bLo, 0);
AssertCompileMemberOffset(CPUMCTXGREG, CPUM_STRUCT_NM(s.) bHi, 1);
#endif



/**
 * SVM Host-state area (Nested Hw.virt - VirtualBox's layout).
 *
 * @warning Exercise caution while modifying the layout of this struct. It's
 *          part of VM saved states.
 */
#pragma pack(1)
typedef struct SVMHOSTSTATE
{
    uint64_t    uEferMsr;
    uint64_t    uCr0;
    uint64_t    uCr4;
    uint64_t    uCr3;
    uint64_t    uRip;
    uint64_t    uRsp;
    uint64_t    uRax;
    X86RFLAGS   rflags;
    CPUMSELREG  es;
    CPUMSELREG  cs;
    CPUMSELREG  ss;
    CPUMSELREG  ds;
    VBOXGDTR    gdtr;
    VBOXIDTR    idtr;
    uint8_t     abPadding[4];
} SVMHOSTSTATE;
#pragma pack()
/** Pointer to the SVMHOSTSTATE structure. */
typedef SVMHOSTSTATE *PSVMHOSTSTATE;
/** Pointer to a const SVMHOSTSTATE structure. */
typedef const SVMHOSTSTATE *PCSVMHOSTSTATE;
#ifndef VBOX_FOR_DTRACE_LIB
AssertCompileSizeAlignment(SVMHOSTSTATE, 8);
AssertCompileSize(SVMHOSTSTATE, 184);
#endif


/**
 * CPU hardware virtualization types.
 */
typedef enum
{
    CPUMHWVIRT_NONE = 0,
    CPUMHWVIRT_VMX,
    CPUMHWVIRT_SVM,
    CPUMHWVIRT_32BIT_HACK = 0x7fffffff
} CPUMHWVIRT;
#ifndef VBOX_FOR_DTRACE_LIB
AssertCompileSize(CPUMHWVIRT, 4);
#endif

/** Number of EFLAGS bits we put aside for the hardware EFLAGS, with the bits
 * above this we use for storing internal state not visible to the guest.
 *
 * Using a value less than 32 here means some code bloat when loading and
 * fetching the hardware EFLAGS value.  Comparing VMMR0.r0 text size when
 * compiling release build using gcc 11.3.1 on linux:
 *      - 32 bits: 2475709 bytes
 *      - 24 bits: 2482069 bytes; +6360 bytes.
 *      - 22 bits: 2482261 bytes; +6552 bytes.
 * Same for windows (virtual size of .text):
 *      - 32 bits: 1498502 bytes
 *      - 24 bits: 1502278 bytes; +3776 bytes.
 *      - 22 bits: 1502198 bytes; +3696 bytes.
 *
 * In addition we pass pointer the 32-bit EFLAGS to a number of IEM assembly
 * functions, so it would be safer to not store anything in the lower 32 bits.
 * OTOH, we'd sooner discover buggy assembly code by doing so, as we've had one
 * example of accidental EFLAGS trashing by these functions already.
 *
 * It would be more efficient for IEM to store the interrupt shadow bit (and
 * anything else that needs to be cleared at the same time) in the 30:22 bit
 * range, because that would allow using a simple AND imm32 instruction on x86
 * and a MOVN imm16,16 instruction to load the constant on ARM64 (assuming the
 * other flag needing clearing is RF (bit 16)).  Putting it in the 63:32 range
 * means we that on x86 we'll either use a memory variant of AND or require a
 * separate load instruction for the immediate, whereas on ARM we'll need more
 * instructions to construct the immediate value.
 *
 * Comparing the instruction exit thruput via the bs2-test-1 testcase, there
 * seems to be little difference between 32 and 24 here (best results out of 9
 * runs on Linux/VT-x).  So, unless the results are really wrong and there is
 * clear drop in thruput, it would on the whole make the most sense to use 24
 * here.
 *
 * Update: We need more than 8 bits because of DBGF, so using 22 now.
 */
#define CPUMX86EFLAGS_HW_BITS       22
/** Mask for the hardware EFLAGS bits, 64-bit version. */
#define CPUMX86EFLAGS_HW_MASK_64    (RT_BIT_64(CPUMX86EFLAGS_HW_BITS) - UINT64_C(1))
/** Mask for the hardware EFLAGS bits, 32-bit version. */
#if CPUMX86EFLAGS_HW_BITS == 32
# define CPUMX86EFLAGS_HW_MASK_32   UINT32_MAX
#elif CPUMX86EFLAGS_HW_BITS < 32 && CPUMX86EFLAGS_HW_BITS >= 22
# define CPUMX86EFLAGS_HW_MASK_32   (RT_BIT_32(CPUMX86EFLAGS_HW_BITS) - UINT32_C(1))
#else
# error "Misconfigured CPUMX86EFLAGS_HW_BITS value!"
#endif

/** Mask of internal flags kept with EFLAGS, 64-bit version.
 * Bits 22-24 are taken by CPUMCTX_INHIBIT_SHADOW_SS, CPUMCTX_INHIBIT_SHADOW_STI
 * and CPUMCTX_INHIBIT_NMI, bits 25-28 are for CPUMCTX_DBG_HIT_DRX_MASK, and
 * bits 29-30 are for DBGF events and breakpoints.
 *
 * @todo The two DBGF bits could be merged.  The NMI inhibiting could move to
 *       bit 32 or higher as it isn't automatically cleared on instruction
 *       completion (except for iret).
 */
#define CPUMX86EFLAGS_INT_MASK_64   UINT64_C(0x00000000ffc00000)
/** Mask of internal flags kept with EFLAGS, 32-bit version. */
#define CPUMX86EFLAGS_INT_MASK_32           UINT32_C(0xffc00000)


/**
 * CPUM EFLAGS.
 *
 * This differs from X86EFLAGS in that we could use bits 31:22 for internal
 * purposes, see CPUMX86EFLAGS_HW_BITS.
 */
typedef union CPUMX86EFLAGS
{
    /** The full unsigned view, both hardware and VBox bits. */
    uint32_t        uBoth;
    /** The plain unsigned view of the hardware bits. */
#if CPUMX86EFLAGS_HW_BITS == 32
    uint32_t        u;
#else
    uint32_t        u : CPUMX86EFLAGS_HW_BITS;
#endif
#ifndef VBOX_FOR_DTRACE_LIB
    /** The bitfield view. */
    X86EFLAGSBITS   Bits;
#endif
} CPUMX86EFLAGS;
/** Pointer to CPUM EFLAGS. */
typedef CPUMX86EFLAGS *PCPUMX86EFLAGS;
/** Pointer to const CPUM EFLAGS. */
typedef const CPUMX86EFLAGS *PCCPUMX86EFLAGS;

/**
 * CPUM RFLAGS.
 *
 * This differs from X86EFLAGS in that we use could be using bits 63:22 for
 * internal purposes, see CPUMX86EFLAGS_HW_BITS.
 */
typedef union CPUMX86RFLAGS
{
    /** The full unsigned view, both hardware and VBox bits. */
    uint64_t        uBoth;
    /** The plain unsigned view of the hardware bits. */
#if CPUMX86EFLAGS_HW_BITS == 32
    uint32_t        u;
#else
    uint32_t        u : CPUMX86EFLAGS_HW_BITS;
#endif
#ifndef VBOX_FOR_DTRACE_LIB
    /** The bitfield view. */
    X86EFLAGSBITS   Bits;
#endif
} CPUMX86RFLAGS;
/** Pointer to CPUM RFLAGS. */
typedef CPUMX86RFLAGS *PCPUMX86RFLAGS;
/** Pointer to const CPUM RFLAGS. */
typedef const CPUMX86RFLAGS *PCCPUMX86RFLAGS;


/**
 * CPU context.
 */
#pragma pack(1) /* for VBOXIDTR / VBOXGDTR. */
typedef struct CPUMCTX
{
    /** General purpose registers. */
    union /* no tag! */
    {
        /** The general purpose register array view, indexed by X86_GREG_XXX. */
        CPUMCTXGREG     aGRegs[16];

        /** 64-bit general purpose register view. */
        RT_GCC_EXTENSION struct /* no tag! */
        {
            uint64_t    rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15;
        } CPUM_STRUCT_NM(qw);
        /** 64-bit general purpose register view. */
        RT_GCC_EXTENSION struct /* no tag! */
        {
            uint64_t    r0, r1, r2, r3, r4, r5, r6, r7;
        } CPUM_STRUCT_NM(qw2);
        /** 32-bit general purpose register view. */
        RT_GCC_EXTENSION struct /* no tag! */
        {
            uint32_t     eax, u32Pad00,      ecx, u32Pad01,      edx, u32Pad02,      ebx, u32Pad03,
                         esp, u32Pad04,      ebp, u32Pad05,      esi, u32Pad06,      edi, u32Pad07,
                         r8d, u32Pad08,      r9d, u32Pad09,     r10d, u32Pad10,     r11d, u32Pad11,
                        r12d, u32Pad12,     r13d, u32Pad13,     r14d, u32Pad14,     r15d, u32Pad15;
        } CPUM_STRUCT_NM(dw);
        /** 16-bit general purpose register view. */
        RT_GCC_EXTENSION struct /* no tag! */
        {
            uint16_t      ax, au16Pad00[3],   cx, au16Pad01[3],   dx, au16Pad02[3],   bx, au16Pad03[3],
                          sp, au16Pad04[3],   bp, au16Pad05[3],   si, au16Pad06[3],   di, au16Pad07[3],
                         r8w, au16Pad08[3],  r9w, au16Pad09[3], r10w, au16Pad10[3], r11w, au16Pad11[3],
                        r12w, au16Pad12[3], r13w, au16Pad13[3], r14w, au16Pad14[3], r15w, au16Pad15[3];
        } CPUM_STRUCT_NM(w);
        RT_GCC_EXTENSION struct /* no tag! */
        {
            uint8_t   al, ah, abPad00[6], cl, ch, abPad01[6], dl, dh, abPad02[6], bl, bh, abPad03[6],
                         spl, abPad04[7],    bpl, abPad05[7],    sil, abPad06[7],    dil, abPad07[7],
                         r8l, abPad08[7],    r9l, abPad09[7],   r10l, abPad10[7],   r11l, abPad11[7],
                        r12l, abPad12[7],   r13l, abPad13[7],   r14l, abPad14[7],   r15l, abPad15[7];
        } CPUM_STRUCT_NM(b);
    } CPUM_UNION_NM(g);

    /** Segment registers. */
    union /* no tag! */
    {
        /** The segment register array view, indexed by X86_SREG_XXX. */
        CPUMSELREG      aSRegs[6];
        /** The named segment register view. */
        RT_GCC_EXTENSION struct /* no tag! */
        {
            CPUMSELREG  es, cs, ss, ds, fs, gs;
        } CPUM_STRUCT_NM(n);
    } CPUM_UNION_NM(s);

    /** The task register.
     * Only the guest context uses all the members. */
    CPUMSELREG          ldtr;
    /** The task register.
     * Only the guest context uses all the members. */
    CPUMSELREG          tr;

    /** The program counter. */
    union
    {
        uint16_t        ip;
        uint32_t        eip;
        uint64_t        rip;
    } CPUM_UNION_NM(rip);

    /** The flags register. */
    union
    {
        CPUMX86EFLAGS   eflags;
        CPUMX86RFLAGS   rflags;
    } CPUM_UNION_NM(rflags);

    /** 0x150 - Externalized state tracker, CPUMCTX_EXTRN_XXX. */
    uint64_t            fExtrn;

    /** The RIP value an interrupt shadow is/was valid for. */
    uint64_t            uRipInhibitInt;

    /** @name Control registers.
     * @{ */
    uint64_t            cr0;
    uint64_t            cr2;
    uint64_t            cr3;
    uint64_t            cr4;
    /** @} */

    /** Debug registers.
     * @remarks DR4 and DR5 should not be used since they are aliases for
     *          DR6 and DR7 respectively on both AMD and Intel CPUs.
     * @remarks DR8-15 are currently not supported by AMD or Intel, so
     *          neither do we.
     */
    uint64_t            dr[8];

    /** Padding before the structure so the 64-bit member is correctly aligned.
     * @todo fix this structure!  */
    uint16_t            gdtrPadding[3];
    /** Global Descriptor Table register. */
    VBOXGDTR            gdtr;

    /** Padding before the structure so the 64-bit member is correctly aligned.
     * @todo fix this structure!  */
    uint16_t            idtrPadding[3];
    /** Interrupt Descriptor Table register. */
    VBOXIDTR            idtr;

    /** The sysenter msr registers.
     * This member is not used by the hypervisor context. */
    CPUMSYSENTER        SysEnter;

    /** @name System MSRs.
     * @{ */
    uint64_t            msrEFER; /**< @todo move EFER up to the crX registers for better cacheline mojo */
    uint64_t            msrSTAR;            /**< Legacy syscall eip, cs & ss. */
    uint64_t            msrPAT;             /**< Page attribute table. */
    uint64_t            msrLSTAR;           /**< 64 bits mode syscall rip. */
    uint64_t            msrCSTAR;           /**< Compatibility mode syscall rip. */
    uint64_t            msrSFMASK;          /**< syscall flag mask. */
    uint64_t            msrKERNELGSBASE;    /**< swapgs exchange value. */
    /** @} */

    uint64_t            au64Unused[2];

    /** 0x240 - PAE PDPTEs. */
    X86PDPE             aPaePdpes[4];

    /** 0x260 - The XCR0..XCR1 registers. */
    uint64_t            aXcr[2];
    /** 0x270 - The mask to pass to XSAVE/XRSTOR in EDX:EAX.  If zero we use
     *  FXSAVE/FXRSTOR (since bit 0 will always be set, we only need to test it). */
    uint64_t            fXStateMask;
    /** 0x278 - Mirror of CPUMCPU::fUseFlags[CPUM_USED_FPU_GUEST]. */
    bool                fUsedFpuGuest;
    uint8_t             afUnused[7];

    /* ---- Start of members not zeroed at reset. ---- */

    /** 0x280 - State component offsets into pXState, UINT16_MAX if not present.
     * @note Everything before this member will be memset to zero during reset. */
    uint16_t            aoffXState[64];
    /** 0x300 - The extended state (FPU/SSE/AVX/AVX-2/XXXX).
     * Aligned on 256 byte boundrary (min req is currently 64 bytes). */
    union /* no tag */
    {
        X86XSAVEAREA    XState;
        /** Byte view for simple indexing and space allocation. */
        uint8_t         abXState[0x4000 - 0x300];
    } CPUM_UNION_NM(u);

    /** 0x4000 - Hardware virtualization state.
     * @note This is page aligned, so an full page member comes first in the
     *       substructures. */
    struct
    {
        union   /* no tag! */
        {
            struct
            {
                /** 0x4000 - Cache of the nested-guest VMCB. */
                SVMVMCB                 Vmcb;
                /** 0x5000 - The MSRPM (MSR Permission bitmap).
                 *
                 * This need not be physically contiguous pages because we use the one from
                 * HMPHYSCPU while executing the nested-guest using hardware-assisted SVM.
                 * This one is just used for caching the bitmap from guest physical memory.
                 *
                 * @todo r=bird: This is not used directly by AMD-V hardware, so it doesn't
                 *       really need to even be page aligned.
                 *
                 *       Also, couldn't we just access the guest page directly when we need to,
                 *       or do we have to use a cached copy of it? */
                uint8_t                 abMsrBitmap[SVM_MSRPM_PAGES * X86_PAGE_SIZE];
                /** 0x7000 - The IOPM (IO Permission bitmap).
                 *
                 * This need not be physically contiguous pages because we re-use the ring-0
                 * allocated IOPM while executing the nested-guest using hardware-assisted SVM
                 * because it's identical (we trap all IO accesses).
                 *
                 * This one is just used for caching the IOPM from guest physical memory in
                 * case the guest hypervisor allows direct access to some IO ports.
                 *
                 * @todo r=bird: This is not used directly by AMD-V hardware, so it doesn't
                 *       really need to even be page aligned.
                 *
                 *       Also, couldn't we just access the guest page directly when we need to,
                 *       or do we have to use a cached copy of it? */
                uint8_t                 abIoBitmap[SVM_IOPM_PAGES * X86_PAGE_SIZE];

                /** 0xa000 - MSR holding physical address of the Guest's Host-state. */
                uint64_t                uMsrHSavePa;
                /** 0xa008 - Guest physical address of the nested-guest VMCB. */
                RTGCPHYS                GCPhysVmcb;
                /** 0xa010 - Guest's host-state save area. */
                SVMHOSTSTATE            HostState;
                /** 0xa0c8 - Guest TSC time-stamp of when the previous PAUSE instr. was
                 *  executed. */
                uint64_t                uPrevPauseTick;
                /** 0xa0d0 - Pause filter count. */
                uint16_t                cPauseFilter;
                /** 0xa0d2 - Pause filter threshold. */
                uint16_t                cPauseFilterThreshold;
                /** 0xa0d4 - Whether the injected event is subject to event intercepts. */
                bool                    fInterceptEvents;
                /** 0xa0d5 - Padding. */
                bool                    afPadding[3];
            } svm;

            struct
            {
                /** 0x4000 - The current VMCS. */
                VMXVVMCS                Vmcs;
                /** 0X5000 - The shadow VMCS. */
                VMXVVMCS                ShadowVmcs;
                /** 0x6000 - The VMREAD bitmap.
                 * @todo r=bird: Do we really need to keep copies for these?  Couldn't we just
                 *       access the guest memory directly as needed?   */
                uint8_t                 abVmreadBitmap[VMX_V_VMREAD_VMWRITE_BITMAP_SIZE];
                /** 0x7000 - The VMWRITE bitmap.
                 * @todo r=bird: Do we really need to keep copies for these?  Couldn't we just
                 *       access the guest memory directly as needed?  */
                uint8_t                 abVmwriteBitmap[VMX_V_VMREAD_VMWRITE_BITMAP_SIZE];
                /** 0x8000 - The VM-entry MSR-load area. */
                VMXAUTOMSR              aEntryMsrLoadArea[VMX_V_AUTOMSR_AREA_SIZE / sizeof(VMXAUTOMSR)];
                /** 0xa000 - The VM-exit MSR-store area. */
                VMXAUTOMSR              aExitMsrStoreArea[VMX_V_AUTOMSR_AREA_SIZE / sizeof(VMXAUTOMSR)];
                /** 0xc000 - The VM-exit MSR-load area. */
                VMXAUTOMSR              aExitMsrLoadArea[VMX_V_AUTOMSR_AREA_SIZE / sizeof(VMXAUTOMSR)];
                /** 0xe000 - The MSR permission bitmap.
                 * @todo r=bird: Do we really need to keep copies for these?  Couldn't we just
                 *       access the guest memory directly as needed?  */
                uint8_t                 abMsrBitmap[VMX_V_MSR_BITMAP_SIZE];
                /** 0xf000 - The I/O permission bitmap.
                 * @todo r=bird: Do we really need to keep copies for these?  Couldn't we just
                 *       access the guest memory directly as needed? */
                uint8_t                 abIoBitmap[VMX_V_IO_BITMAP_A_SIZE + VMX_V_IO_BITMAP_B_SIZE];

                /** 0x11000 - Guest physical address of the VMXON region. */
                RTGCPHYS                GCPhysVmxon;
                /** 0x11008 - Guest physical address of the current VMCS pointer. */
                RTGCPHYS                GCPhysVmcs;
                /** 0x11010 - Guest physical address of the shadow VMCS pointer. */
                RTGCPHYS                GCPhysShadowVmcs;
                /** 0x11018 - Last emulated VMX instruction/VM-exit diagnostic. */
                VMXVDIAG                enmDiag;
                /** 0x1101c - VMX abort reason. */
                VMXABORT                enmAbort;
                /** 0x11020 - Last emulated VMX instruction/VM-exit diagnostic auxiliary info.
                 *  (mainly used for info. that's not part of the VMCS). */
                uint64_t                uDiagAux;
                /** 0x11028 - VMX abort auxiliary info. */
                uint32_t                uAbortAux;
                /** 0x1102c - Whether the guest is in VMX root mode. */
                bool                    fInVmxRootMode;
                /** 0x1102d - Whether the guest is in VMX non-root mode. */
                bool                    fInVmxNonRootMode;
                /** 0x1102e - Whether the injected events are subjected to event intercepts.  */
                bool                    fInterceptEvents;
                /** 0x1102f - Whether blocking of NMI (or virtual-NMIs) was in effect in VMX
                 *  non-root mode before execution of IRET. */
                bool                    fNmiUnblockingIret;
                /** 0x11030 - Guest TSC timestamp of the first PAUSE instruction that is
                 *  considered to be the first in a loop. */
                uint64_t                uFirstPauseLoopTick;
                /** 0x11038 - Guest TSC timestamp of the previous PAUSE instruction. */
                uint64_t                uPrevPauseTick;
                /** 0x11040 - Guest TSC timestamp of VM-entry (used for VMX-preemption
                 *  timer). */
                uint64_t                uEntryTick;
                /** 0x11048 - Virtual-APIC write offset (until trap-like VM-exit). */
                uint16_t                offVirtApicWrite;
                /** 0x1104a - Whether virtual-NMI blocking is in effect. */
                bool                    fVirtNmiBlocking;
                /** 0x1104b - Padding. */
                uint8_t                 abPadding0[5];
                /** 0x11050 - Guest VMX MSRs. */
                VMXMSRS                 Msrs;
            } vmx;
        } CPUM_UNION_NM(s);

        /** 0x11130 - Hardware virtualization type currently in use. */
        CPUMHWVIRT              enmHwvirt;
        /** 0x11134 - Global interrupt flag - AMD only (always true on Intel). */
        bool                    fGif;
        /** 0x11135 - Padding. */
        bool                    afPadding0[3];
        /** 0x11138 - A subset of guest inhibit flags (CPUMCTX_INHIBIT_XXX) that are
         *  saved while running the nested-guest. */
        uint32_t                fSavedInhibit;
        /** 0x1113c - Pad to 64 byte boundary. */
        uint8_t                 abPadding1[4];
    } hwvirt;
} CPUMCTX;
#pragma pack()

#ifndef VBOX_FOR_DTRACE_LIB
AssertCompileSizeAlignment(CPUMCTX, 64);
AssertCompileSizeAlignment(CPUMCTX, 32);
AssertCompileSizeAlignment(CPUMCTX, 16);
AssertCompileSizeAlignment(CPUMCTX, 8);
AssertCompileMemberOffset(CPUMCTX, CPUM_UNION_NM(g.) CPUM_STRUCT_NM(qw.) rax, 0x0000);
AssertCompileMemberOffset(CPUMCTX, CPUM_UNION_NM(g.) CPUM_STRUCT_NM(qw.) rcx, 0x0008);
AssertCompileMemberOffset(CPUMCTX, CPUM_UNION_NM(g.) CPUM_STRUCT_NM(qw.) rdx, 0x0010);
AssertCompileMemberOffset(CPUMCTX, CPUM_UNION_NM(g.) CPUM_STRUCT_NM(qw.) rbx, 0x0018);
AssertCompileMemberOffset(CPUMCTX, CPUM_UNION_NM(g.) CPUM_STRUCT_NM(qw.) rsp, 0x0020);
AssertCompileMemberOffset(CPUMCTX, CPUM_UNION_NM(g.) CPUM_STRUCT_NM(qw.) rbp, 0x0028);
AssertCompileMemberOffset(CPUMCTX, CPUM_UNION_NM(g.) CPUM_STRUCT_NM(qw.) rsi, 0x0030);
AssertCompileMemberOffset(CPUMCTX, CPUM_UNION_NM(g.) CPUM_STRUCT_NM(qw.) rdi, 0x0038);
AssertCompileMemberOffset(CPUMCTX, CPUM_UNION_NM(g.) CPUM_STRUCT_NM(qw.)  r8, 0x0040);
AssertCompileMemberOffset(CPUMCTX, CPUM_UNION_NM(g.) CPUM_STRUCT_NM(qw.)  r9, 0x0048);
AssertCompileMemberOffset(CPUMCTX, CPUM_UNION_NM(g.) CPUM_STRUCT_NM(qw.) r10, 0x0050);
AssertCompileMemberOffset(CPUMCTX, CPUM_UNION_NM(g.) CPUM_STRUCT_NM(qw.) r11, 0x0058);
AssertCompileMemberOffset(CPUMCTX, CPUM_UNION_NM(g.) CPUM_STRUCT_NM(qw.) r12, 0x0060);
AssertCompileMemberOffset(CPUMCTX, CPUM_UNION_NM(g.) CPUM_STRUCT_NM(qw.) r13, 0x0068);
AssertCompileMemberOffset(CPUMCTX, CPUM_UNION_NM(g.) CPUM_STRUCT_NM(qw.) r14, 0x0070);
AssertCompileMemberOffset(CPUMCTX, CPUM_UNION_NM(g.) CPUM_STRUCT_NM(qw.) r15, 0x0078);
AssertCompileMemberOffset(CPUMCTX,   CPUM_UNION_NM(s.) CPUM_STRUCT_NM(n.) es, 0x0080);
AssertCompileMemberOffset(CPUMCTX,   CPUM_UNION_NM(s.) CPUM_STRUCT_NM(n.) cs, 0x0098);
AssertCompileMemberOffset(CPUMCTX,   CPUM_UNION_NM(s.) CPUM_STRUCT_NM(n.) ss, 0x00b0);
AssertCompileMemberOffset(CPUMCTX,   CPUM_UNION_NM(s.) CPUM_STRUCT_NM(n.) ds, 0x00c8);
AssertCompileMemberOffset(CPUMCTX,   CPUM_UNION_NM(s.) CPUM_STRUCT_NM(n.) fs, 0x00e0);
AssertCompileMemberOffset(CPUMCTX,   CPUM_UNION_NM(s.) CPUM_STRUCT_NM(n.) gs, 0x00f8);
AssertCompileMemberOffset(CPUMCTX,                                      ldtr, 0x0110);
AssertCompileMemberOffset(CPUMCTX,                                        tr, 0x0128);
AssertCompileMemberOffset(CPUMCTX,                                       rip, 0x0140);
AssertCompileMemberOffset(CPUMCTX,                                    rflags, 0x0148);
AssertCompileMemberOffset(CPUMCTX,                                    fExtrn, 0x0150);
AssertCompileMemberOffset(CPUMCTX,                            uRipInhibitInt, 0x0158);
AssertCompileMemberOffset(CPUMCTX,                                       cr0, 0x0160);
AssertCompileMemberOffset(CPUMCTX,                                       cr2, 0x0168);
AssertCompileMemberOffset(CPUMCTX,                                       cr3, 0x0170);
AssertCompileMemberOffset(CPUMCTX,                                       cr4, 0x0178);
AssertCompileMemberOffset(CPUMCTX,                                        dr, 0x0180);
AssertCompileMemberOffset(CPUMCTX,                                      gdtr, 0x01c0+6);
AssertCompileMemberOffset(CPUMCTX,                                      idtr, 0x01d0+6);
AssertCompileMemberOffset(CPUMCTX,                                  SysEnter, 0x01e0);
AssertCompileMemberOffset(CPUMCTX,                                   msrEFER, 0x01f8);
AssertCompileMemberOffset(CPUMCTX,                                   msrSTAR, 0x0200);
AssertCompileMemberOffset(CPUMCTX,                                    msrPAT, 0x0208);
AssertCompileMemberOffset(CPUMCTX,                                  msrLSTAR, 0x0210);
AssertCompileMemberOffset(CPUMCTX,                                  msrCSTAR, 0x0218);
AssertCompileMemberOffset(CPUMCTX,                                 msrSFMASK, 0x0220);
AssertCompileMemberOffset(CPUMCTX,                           msrKERNELGSBASE, 0x0228);
AssertCompileMemberOffset(CPUMCTX,                                 aPaePdpes, 0x0240);
AssertCompileMemberOffset(CPUMCTX,                                      aXcr, 0x0260);
AssertCompileMemberOffset(CPUMCTX,                               fXStateMask, 0x0270);
AssertCompileMemberOffset(CPUMCTX,                             fUsedFpuGuest, 0x0278);
AssertCompileMemberOffset(CPUMCTX,                  CPUM_UNION_NM(u.) XState, 0x0300);
AssertCompileMemberOffset(CPUMCTX,                CPUM_UNION_NM(u.) abXState, 0x0300);
AssertCompileMemberAlignment(CPUMCTX,               CPUM_UNION_NM(u.) XState, 0x0100);
/* Only do spot checks for hwvirt */
AssertCompileMemberAlignment(CPUMCTX,                                 hwvirt, 0x1000);
AssertCompileMemberAlignment(CPUMCTX, hwvirt.CPUM_UNION_NM(s.) svm.Vmcb,                  X86_PAGE_SIZE);
AssertCompileMemberAlignment(CPUMCTX, hwvirt.CPUM_UNION_NM(s.) svm.abMsrBitmap,           X86_PAGE_SIZE);
AssertCompileMemberAlignment(CPUMCTX, hwvirt.CPUM_UNION_NM(s.) svm.abIoBitmap,            X86_PAGE_SIZE);
AssertCompileMemberAlignment(CPUMCTX, hwvirt.CPUM_UNION_NM(s.) vmx.Vmcs,                  X86_PAGE_SIZE);
AssertCompileMemberAlignment(CPUMCTX, hwvirt.CPUM_UNION_NM(s.) vmx.ShadowVmcs,            X86_PAGE_SIZE);
AssertCompileMemberAlignment(CPUMCTX, hwvirt.CPUM_UNION_NM(s.) vmx.abVmreadBitmap,        X86_PAGE_SIZE);
AssertCompileMemberAlignment(CPUMCTX, hwvirt.CPUM_UNION_NM(s.) vmx.abVmwriteBitmap,       X86_PAGE_SIZE);
AssertCompileMemberAlignment(CPUMCTX, hwvirt.CPUM_UNION_NM(s.) vmx.aEntryMsrLoadArea,     X86_PAGE_SIZE);
AssertCompileMemberAlignment(CPUMCTX, hwvirt.CPUM_UNION_NM(s.) vmx.aExitMsrStoreArea,     X86_PAGE_SIZE);
AssertCompileMemberAlignment(CPUMCTX, hwvirt.CPUM_UNION_NM(s.) vmx.aExitMsrLoadArea,      X86_PAGE_SIZE);
AssertCompileMemberAlignment(CPUMCTX, hwvirt.CPUM_UNION_NM(s.) vmx.abMsrBitmap,           X86_PAGE_SIZE);
AssertCompileMemberAlignment(CPUMCTX, hwvirt.CPUM_UNION_NM(s.) vmx.abIoBitmap,            X86_PAGE_SIZE);
AssertCompileMemberAlignment(CPUMCTX, hwvirt.CPUM_UNION_NM(s.) vmx.Msrs,                  8);
AssertCompileMemberOffset(CPUMCTX,    hwvirt.CPUM_UNION_NM(s.) svm.abIoBitmap,            0x7000);
AssertCompileMemberOffset(CPUMCTX,    hwvirt.CPUM_UNION_NM(s.) svm.fInterceptEvents,      0xa0d4);
AssertCompileMemberOffset(CPUMCTX,    hwvirt.CPUM_UNION_NM(s.) vmx.abIoBitmap,            0xf000);
AssertCompileMemberOffset(CPUMCTX,    hwvirt.CPUM_UNION_NM(s.) vmx.fVirtNmiBlocking,      0x1104a);
AssertCompileMemberOffset(CPUMCTX,    hwvirt.enmHwvirt,                                   0x11130);
AssertCompileMemberOffset(CPUMCTX,    hwvirt.fGif,                                        0x11134);
AssertCompileMemberOffset(CPUMCTX,    hwvirt.fSavedInhibit,                               0x11138);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rax, CPUMCTX, CPUM_UNION_NM(g.) aGRegs);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rax, CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw2.)  r0);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rcx, CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw2.)  r1);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rdx, CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw2.)  r2);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rbx, CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw2.)  r3);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rsp, CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw2.)  r4);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rbp, CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw2.)  r5);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rsi, CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw2.)  r6);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rdi, CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw2.)  r7);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rax, CPUMCTX, CPUM_UNION_STRUCT_NM(g,dw.)  eax);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rcx, CPUMCTX, CPUM_UNION_STRUCT_NM(g,dw.)  ecx);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rdx, CPUMCTX, CPUM_UNION_STRUCT_NM(g,dw.)  edx);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rbx, CPUMCTX, CPUM_UNION_STRUCT_NM(g,dw.)  ebx);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rsp, CPUMCTX, CPUM_UNION_STRUCT_NM(g,dw.)  esp);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rbp, CPUMCTX, CPUM_UNION_STRUCT_NM(g,dw.)  ebp);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rsi, CPUMCTX, CPUM_UNION_STRUCT_NM(g,dw.)  esi);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rdi, CPUMCTX, CPUM_UNION_STRUCT_NM(g,dw.)  edi);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.)  r8, CPUMCTX, CPUM_UNION_STRUCT_NM(g,dw.)  r8d);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.)  r9, CPUMCTX, CPUM_UNION_STRUCT_NM(g,dw.)  r9d);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r10, CPUMCTX, CPUM_UNION_STRUCT_NM(g,dw.) r10d);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r11, CPUMCTX, CPUM_UNION_STRUCT_NM(g,dw.) r11d);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r12, CPUMCTX, CPUM_UNION_STRUCT_NM(g,dw.) r12d);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r13, CPUMCTX, CPUM_UNION_STRUCT_NM(g,dw.) r13d);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r14, CPUMCTX, CPUM_UNION_STRUCT_NM(g,dw.) r14d);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r15, CPUMCTX, CPUM_UNION_STRUCT_NM(g,dw.) r15d);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rax, CPUMCTX, CPUM_UNION_STRUCT_NM(g,w.)    ax);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rcx, CPUMCTX, CPUM_UNION_STRUCT_NM(g,w.)    cx);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rdx, CPUMCTX, CPUM_UNION_STRUCT_NM(g,w.)    dx);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rbx, CPUMCTX, CPUM_UNION_STRUCT_NM(g,w.)    bx);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rsp, CPUMCTX, CPUM_UNION_STRUCT_NM(g,w.)    sp);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rbp, CPUMCTX, CPUM_UNION_STRUCT_NM(g,w.)    bp);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rsi, CPUMCTX, CPUM_UNION_STRUCT_NM(g,w.)    si);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rdi, CPUMCTX, CPUM_UNION_STRUCT_NM(g,w.)    di);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.)  r8, CPUMCTX, CPUM_UNION_STRUCT_NM(g,w.)   r8w);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.)  r9, CPUMCTX, CPUM_UNION_STRUCT_NM(g,w.)   r9w);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r10, CPUMCTX, CPUM_UNION_STRUCT_NM(g,w.)  r10w);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r11, CPUMCTX, CPUM_UNION_STRUCT_NM(g,w.)  r11w);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r12, CPUMCTX, CPUM_UNION_STRUCT_NM(g,w.)  r12w);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r13, CPUMCTX, CPUM_UNION_STRUCT_NM(g,w.)  r13w);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r14, CPUMCTX, CPUM_UNION_STRUCT_NM(g,w.)  r14w);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r15, CPUMCTX, CPUM_UNION_STRUCT_NM(g,w.)  r15w);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rax, CPUMCTX, CPUM_UNION_STRUCT_NM(g,b.)    al);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rcx, CPUMCTX, CPUM_UNION_STRUCT_NM(g,b.)    cl);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rdx, CPUMCTX, CPUM_UNION_STRUCT_NM(g,b.)    dl);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rbx, CPUMCTX, CPUM_UNION_STRUCT_NM(g,b.)    bl);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rsp, CPUMCTX, CPUM_UNION_STRUCT_NM(g,b.)   spl);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rbp, CPUMCTX, CPUM_UNION_STRUCT_NM(g,b.)   bpl);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rsi, CPUMCTX, CPUM_UNION_STRUCT_NM(g,b.)   sil);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rdi, CPUMCTX, CPUM_UNION_STRUCT_NM(g,b.)   dil);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.)  r8, CPUMCTX, CPUM_UNION_STRUCT_NM(g,b.)   r8l);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.)  r9, CPUMCTX, CPUM_UNION_STRUCT_NM(g,b.)   r9l);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r10, CPUMCTX, CPUM_UNION_STRUCT_NM(g,b.)  r10l);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r11, CPUMCTX, CPUM_UNION_STRUCT_NM(g,b.)  r11l);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r12, CPUMCTX, CPUM_UNION_STRUCT_NM(g,b.)  r12l);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r13, CPUMCTX, CPUM_UNION_STRUCT_NM(g,b.)  r13l);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r14, CPUMCTX, CPUM_UNION_STRUCT_NM(g,b.)  r14l);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r15, CPUMCTX, CPUM_UNION_STRUCT_NM(g,b.)  r15l);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_NM(s.) CPUM_STRUCT_NM(n.) es,   CPUMCTX, CPUM_UNION_NM(s.) aSRegs);
# ifndef _MSC_VER
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rax, CPUMCTX, CPUM_UNION_NM(g.) aGRegs[X86_GREG_xAX]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rcx, CPUMCTX, CPUM_UNION_NM(g.) aGRegs[X86_GREG_xCX]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rdx, CPUMCTX, CPUM_UNION_NM(g.) aGRegs[X86_GREG_xDX]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rbx, CPUMCTX, CPUM_UNION_NM(g.) aGRegs[X86_GREG_xBX]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rsp, CPUMCTX, CPUM_UNION_NM(g.) aGRegs[X86_GREG_xSP]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rbp, CPUMCTX, CPUM_UNION_NM(g.) aGRegs[X86_GREG_xBP]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rsi, CPUMCTX, CPUM_UNION_NM(g.) aGRegs[X86_GREG_xSI]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) rdi, CPUMCTX, CPUM_UNION_NM(g.) aGRegs[X86_GREG_xDI]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.)  r8, CPUMCTX, CPUM_UNION_NM(g.) aGRegs[X86_GREG_x8]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.)  r9, CPUMCTX, CPUM_UNION_NM(g.) aGRegs[X86_GREG_x9]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r10, CPUMCTX, CPUM_UNION_NM(g.) aGRegs[X86_GREG_x10]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r11, CPUMCTX, CPUM_UNION_NM(g.) aGRegs[X86_GREG_x11]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r12, CPUMCTX, CPUM_UNION_NM(g.) aGRegs[X86_GREG_x12]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r13, CPUMCTX, CPUM_UNION_NM(g.) aGRegs[X86_GREG_x13]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r14, CPUMCTX, CPUM_UNION_NM(g.) aGRegs[X86_GREG_x14]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(g,qw.) r15, CPUMCTX, CPUM_UNION_NM(g.) aGRegs[X86_GREG_x15]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(s,n.) es,   CPUMCTX, CPUM_UNION_NM(s.) aSRegs[X86_SREG_ES]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(s,n.) cs,   CPUMCTX, CPUM_UNION_NM(s.) aSRegs[X86_SREG_CS]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(s,n.) ss,   CPUMCTX, CPUM_UNION_NM(s.) aSRegs[X86_SREG_SS]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(s,n.) ds,   CPUMCTX, CPUM_UNION_NM(s.) aSRegs[X86_SREG_DS]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(s,n.) fs,   CPUMCTX, CPUM_UNION_NM(s.) aSRegs[X86_SREG_FS]);
AssertCompileMembersAtSameOffset(CPUMCTX, CPUM_UNION_STRUCT_NM(s,n.) gs,   CPUMCTX, CPUM_UNION_NM(s.) aSRegs[X86_SREG_GS]);
# endif


/**
 * Calculates the pointer to the given extended state component.
 *
 * @returns Pointer of type @a a_PtrType
 * @param   a_pCtx          Pointer to the context.
 * @param   a_iCompBit      The extended state component bit number.  This bit
 *                          must be set in CPUMCTX::fXStateMask.
 * @param   a_PtrType       The pointer type of the extended state component.
 *
 */
#if defined(VBOX_STRICT) && defined(RT_COMPILER_SUPPORTS_LAMBDA)
# define CPUMCTX_XSAVE_C_PTR(a_pCtx, a_iCompBit, a_PtrType) \
    ([](PCCPUMCTX a_pLambdaCtx) -> a_PtrType \
    { \
        AssertCompile((a_iCompBit) < 64U); \
        AssertMsg(a_pLambdaCtx->fXStateMask & RT_BIT_64(a_iCompBit), (#a_iCompBit "\n")); \
        AssertMsg(a_pLambdaCtx->aoffXState[(a_iCompBit)] != UINT16_MAX, (#a_iCompBit "\n")); \
        return (a_PtrType)(&a_pLambdaCtx->abXState[a_pLambdaCtx->aoffXState[(a_iCompBit)]]); \
    }(a_pCtx))
#elif defined(VBOX_STRICT) && defined(__GNUC__)
# define CPUMCTX_XSAVE_C_PTR(a_pCtx, a_iCompBit, a_PtrType) \
    __extension__ (\
    { \
        AssertCompile((a_iCompBit) < 64U); \
        AssertMsg((a_pCtx)->fXStateMask & RT_BIT_64(a_iCompBit), (#a_iCompBit "\n")); \
        AssertMsg((a_pCtx)->aoffXState[(a_iCompBit)] != UINT16_MAX, (#a_iCompBit "\n")); \
        (a_PtrType)(&(a_pCtx)->abXState[(a_pCtx)->aoffXState[(a_iCompBit)]]); \
    })
#else
# define CPUMCTX_XSAVE_C_PTR(a_pCtx, a_iCompBit, a_PtrType) \
    ((a_PtrType)(&(a_pCtx)->abXState[(a_pCtx)->aoffXState[(a_iCompBit)]]))
#endif

/**
 * Gets the first selector register of a CPUMCTX.
 *
 * Use this with X86_SREG_COUNT to loop thru the selector registers.
 */
# define CPUMCTX_FIRST_SREG(a_pCtx) (&(a_pCtx)->es)

#endif /* !VBOX_FOR_DTRACE_LIB */


/** @name CPUMCTX_EXTRN_XXX
 * Used for parts of the CPUM state that is externalized and needs fetching
 * before use.
 *
 * @{ */
/** External state keeper: Invalid.  */
#define CPUMCTX_EXTRN_KEEPER_INVALID            UINT64_C(0x0000000000000000)
/** External state keeper: HM. */
#define CPUMCTX_EXTRN_KEEPER_HM                 UINT64_C(0x0000000000000001)
/** External state keeper: NEM. */
#define CPUMCTX_EXTRN_KEEPER_NEM                UINT64_C(0x0000000000000002)
/** External state keeper: REM. */
#define CPUMCTX_EXTRN_KEEPER_REM                UINT64_C(0x0000000000000003)
/** External state keeper mask. */
#define CPUMCTX_EXTRN_KEEPER_MASK               UINT64_C(0x0000000000000003)

/** The RIP register value is kept externally. */
#define CPUMCTX_EXTRN_RIP                       UINT64_C(0x0000000000000004)
/** The RFLAGS register values are kept externally. */
#define CPUMCTX_EXTRN_RFLAGS                    UINT64_C(0x0000000000000008)

/** The RAX register value is kept externally. */
#define CPUMCTX_EXTRN_RAX                       UINT64_C(0x0000000000000010)
/** The RCX register value is kept externally. */
#define CPUMCTX_EXTRN_RCX                       UINT64_C(0x0000000000000020)
/** The RDX register value is kept externally. */
#define CPUMCTX_EXTRN_RDX                       UINT64_C(0x0000000000000040)
/** The RBX register value is kept externally. */
#define CPUMCTX_EXTRN_RBX                       UINT64_C(0x0000000000000080)
/** The RSP register value is kept externally. */
#define CPUMCTX_EXTRN_RSP                       UINT64_C(0x0000000000000100)
/** The RBP register value is kept externally. */
#define CPUMCTX_EXTRN_RBP                       UINT64_C(0x0000000000000200)
/** The RSI register value is kept externally. */
#define CPUMCTX_EXTRN_RSI                       UINT64_C(0x0000000000000400)
/** The RDI register value is kept externally. */
#define CPUMCTX_EXTRN_RDI                       UINT64_C(0x0000000000000800)
/** The R8 thru R15 register values are kept externally. */
#define CPUMCTX_EXTRN_R8_R15                    UINT64_C(0x0000000000001000)
/** General purpose registers mask. */
#define CPUMCTX_EXTRN_GPRS_MASK                 UINT64_C(0x0000000000001ff0)

/** The ES register values are kept externally. */
#define CPUMCTX_EXTRN_ES                        UINT64_C(0x0000000000002000)
/** The CS register values are kept externally. */
#define CPUMCTX_EXTRN_CS                        UINT64_C(0x0000000000004000)
/** The SS register values are kept externally. */
#define CPUMCTX_EXTRN_SS                        UINT64_C(0x0000000000008000)
/** The DS register values are kept externally. */
#define CPUMCTX_EXTRN_DS                        UINT64_C(0x0000000000010000)
/** The FS register values are kept externally. */
#define CPUMCTX_EXTRN_FS                        UINT64_C(0x0000000000020000)
/** The GS register values are kept externally. */
#define CPUMCTX_EXTRN_GS                        UINT64_C(0x0000000000040000)
/** Segment registers (includes CS). */
#define CPUMCTX_EXTRN_SREG_MASK                 UINT64_C(0x000000000007e000)
/** Converts a X86_XREG_XXX index to a CPUMCTX_EXTRN_xS mask. */
#define CPUMCTX_EXTRN_SREG_FROM_IDX(a_SRegIdx)  RT_BIT_64((a_SRegIdx) + 13)
#ifndef VBOX_FOR_DTRACE_LIB
AssertCompile(CPUMCTX_EXTRN_SREG_FROM_IDX(X86_SREG_ES) == CPUMCTX_EXTRN_ES);
AssertCompile(CPUMCTX_EXTRN_SREG_FROM_IDX(X86_SREG_CS) == CPUMCTX_EXTRN_CS);
AssertCompile(CPUMCTX_EXTRN_SREG_FROM_IDX(X86_SREG_DS) == CPUMCTX_EXTRN_DS);
AssertCompile(CPUMCTX_EXTRN_SREG_FROM_IDX(X86_SREG_FS) == CPUMCTX_EXTRN_FS);
AssertCompile(CPUMCTX_EXTRN_SREG_FROM_IDX(X86_SREG_GS) == CPUMCTX_EXTRN_GS);
#endif

/** The GDTR register values are kept externally. */
#define CPUMCTX_EXTRN_GDTR                      UINT64_C(0x0000000000080000)
/** The IDTR register values are kept externally. */
#define CPUMCTX_EXTRN_IDTR                      UINT64_C(0x0000000000100000)
/** The LDTR register values are kept externally. */
#define CPUMCTX_EXTRN_LDTR                      UINT64_C(0x0000000000200000)
/** The TR register values are kept externally. */
#define CPUMCTX_EXTRN_TR                        UINT64_C(0x0000000000400000)
/** Table register mask. */
#define CPUMCTX_EXTRN_TABLE_MASK                UINT64_C(0x0000000000780000)

/** The CR0 register value is kept externally. */
#define CPUMCTX_EXTRN_CR0                       UINT64_C(0x0000000000800000)
/** The CR2 register value is kept externally. */
#define CPUMCTX_EXTRN_CR2                       UINT64_C(0x0000000001000000)
/** The CR3 register value is kept externally. */
#define CPUMCTX_EXTRN_CR3                       UINT64_C(0x0000000002000000)
/** The CR4 register value is kept externally. */
#define CPUMCTX_EXTRN_CR4                       UINT64_C(0x0000000004000000)
/** Control register mask. */
#define CPUMCTX_EXTRN_CR_MASK                   UINT64_C(0x0000000007800000)
/** The TPR/CR8 register value is kept externally. */
#define CPUMCTX_EXTRN_APIC_TPR                  UINT64_C(0x0000000008000000)
/** The EFER register value is kept externally. */
#define CPUMCTX_EXTRN_EFER                      UINT64_C(0x0000000010000000)

/** The DR0, DR1, DR2 and DR3 register values are kept externally. */
#define CPUMCTX_EXTRN_DR0_DR3                   UINT64_C(0x0000000020000000)
/** The DR6 register value is kept externally. */
#define CPUMCTX_EXTRN_DR6                       UINT64_C(0x0000000040000000)
/** The DR7 register value is kept externally. */
#define CPUMCTX_EXTRN_DR7                       UINT64_C(0x0000000080000000)
/** Debug register mask. */
#define CPUMCTX_EXTRN_DR_MASK                   UINT64_C(0x00000000e0000000)

/** The XSAVE_C_X87 state is kept externally. */
#define CPUMCTX_EXTRN_X87                       UINT64_C(0x0000000100000000)
/** The XSAVE_C_SSE, XSAVE_C_YMM, XSAVE_C_ZMM_HI256, XSAVE_C_ZMM_16HI and
 * XSAVE_C_OPMASK state is kept externally. */
#define CPUMCTX_EXTRN_SSE_AVX                   UINT64_C(0x0000000200000000)
/** The state of XSAVE components not covered by CPUMCTX_EXTRN_X87 and
 * CPUMCTX_EXTRN_SEE_AVX is kept externally. */
#define CPUMCTX_EXTRN_OTHER_XSAVE               UINT64_C(0x0000000400000000)
/** The state of XCR0 and XCR1 register values are kept externally. */
#define CPUMCTX_EXTRN_XCRx                      UINT64_C(0x0000000800000000)


/** The KERNEL GS BASE MSR value is kept externally. */
#define CPUMCTX_EXTRN_KERNEL_GS_BASE            UINT64_C(0x0000001000000000)
/** The STAR, LSTAR, CSTAR and SFMASK MSR values are kept externally. */
#define CPUMCTX_EXTRN_SYSCALL_MSRS              UINT64_C(0x0000002000000000)
/** The SYSENTER_CS, SYSENTER_EIP and SYSENTER_ESP MSR values are kept externally. */
#define CPUMCTX_EXTRN_SYSENTER_MSRS             UINT64_C(0x0000004000000000)
/** The TSC_AUX MSR is kept externally. */
#define CPUMCTX_EXTRN_TSC_AUX                   UINT64_C(0x0000008000000000)
/** All other stateful MSRs not covered by CPUMCTX_EXTRN_EFER,
 * CPUMCTX_EXTRN_KERNEL_GS_BASE, CPUMCTX_EXTRN_SYSCALL_MSRS,
 * CPUMCTX_EXTRN_SYSENTER_MSRS, and CPUMCTX_EXTRN_TSC_AUX.  */
#define CPUMCTX_EXTRN_OTHER_MSRS                UINT64_C(0x0000010000000000)

/** Mask of all the MSRs. */
#define CPUMCTX_EXTRN_ALL_MSRS                  (  CPUMCTX_EXTRN_EFER | CPUMCTX_EXTRN_KERNEL_GS_BASE | CPUMCTX_EXTRN_SYSCALL_MSRS \
                                                 | CPUMCTX_EXTRN_SYSENTER_MSRS | CPUMCTX_EXTRN_TSC_AUX | CPUMCTX_EXTRN_OTHER_MSRS)

/** Hardware-virtualization (SVM or VMX) state is kept externally. */
#define CPUMCTX_EXTRN_HWVIRT                    UINT64_C(0x0000020000000000)

/** Inhibit maskable interrupts (VMCPU_FF_INHIBIT_INTERRUPTS) */
#define CPUMCTX_EXTRN_INHIBIT_INT               UINT64_C(0x0000040000000000)
/** Inhibit non-maskable interrupts (VMCPU_FF_BLOCK_NMIS). */
#define CPUMCTX_EXTRN_INHIBIT_NMI               UINT64_C(0x0000080000000000)

/** Mask of bits the keepers can use for state tracking. */
#define CPUMCTX_EXTRN_KEEPER_STATE_MASK         UINT64_C(0xffff000000000000)

/** NEM/Win: Event injection (known was interruption) pending state. */
#define CPUMCTX_EXTRN_NEM_WIN_EVENT_INJECT      UINT64_C(0x0001000000000000)
/** NEM/Win: Mask. */
#define CPUMCTX_EXTRN_NEM_WIN_MASK              UINT64_C(0x0001000000000000)

/** HM/SVM: Nested-guest interrupt pending (VMCPU_FF_INTERRUPT_NESTED_GUEST). */
#define CPUMCTX_EXTRN_HM_SVM_HWVIRT_VIRQ        UINT64_C(0x0001000000000000)
/** HM/SVM: Mask. */
#define CPUMCTX_EXTRN_HM_SVM_MASK               UINT64_C(0x0001000000000000)

/** All CPUM state bits, not including keeper specific ones. */
#define CPUMCTX_EXTRN_ALL                       UINT64_C(0x00000ffffffffffc)
/** All CPUM state bits, including keeper specific ones. */
#define CPUMCTX_EXTRN_ABSOLUTELY_ALL            UINT64_C(0xfffffffffffffffc)
/** @} */


/** @name CPUMCTX_INHIBIT_XXX - Interrupt inhibiting flags.
 * @{ */
/** Interrupt shadow following MOV SS or POP SS.
 *
 * When this in effect, both maskable and non-maskable interrupts are blocked
 * from delivery for one instruction.  Same for certain debug exceptions too,
 * unlike the STI variant.
 *
 * It is implementation specific whether a sequence of two or more of these
 * instructions will have any effect on the instruction following the last one
 * of them. */
#define CPUMCTX_INHIBIT_SHADOW_SS       RT_BIT_32(0 + CPUMX86EFLAGS_HW_BITS)
/** Interrupt shadow following STI.
 * Same as CPUMCTX_INHIBIT_SHADOW_SS but without blocking any debug exceptions. */
#define CPUMCTX_INHIBIT_SHADOW_STI      RT_BIT_32(1 + CPUMX86EFLAGS_HW_BITS)
/** Mask combining STI and SS shadowing. */
#define CPUMCTX_INHIBIT_SHADOW          (CPUMCTX_INHIBIT_SHADOW_SS | CPUMCTX_INHIBIT_SHADOW_STI)

/** Interrupts blocked by NMI delivery.  This condition is cleared by IRET.
 *
 * Section "6.7 NONMASKABLE INTERRUPT (NMI)" in Intel SDM Vol 3A states that
 * "The processor also invokes certain hardware conditions to ensure that no
 * other interrupts, including NMI interrupts, are received until the NMI
 * handler has completed executing."  This flag indicates that these
 * conditions are currently active.
 *
 * @todo this does not really need to be in the lower 32-bits of EFLAGS.
 */
#define CPUMCTX_INHIBIT_NMI             RT_BIT_32(2 + CPUMX86EFLAGS_HW_BITS)

/** Mask containing all the interrupt inhibit bits. */
#define CPUMCTX_INHIBIT_ALL_MASK        (CPUMCTX_INHIBIT_SHADOW_SS | CPUMCTX_INHIBIT_SHADOW_STI | CPUMCTX_INHIBIT_NMI)
AssertCompile(CPUMCTX_INHIBIT_ALL_MASK < UINT32_MAX);
/** @} */

/** @name CPUMCTX_DBG_XXX - Pending debug events.
 * @{ */
/** Hit guest DR0 breakpoint. */
#define CPUMCTX_DBG_HIT_DR0             RT_BIT_32(CPUMCTX_DBG_HIT_DR0_BIT)
#define CPUMCTX_DBG_HIT_DR0_BIT         (3 + CPUMX86EFLAGS_HW_BITS)
/** Hit guest DR1 breakpoint. */
#define CPUMCTX_DBG_HIT_DR1             RT_BIT_32(CPUMCTX_DBG_HIT_DR1_BIT)
#define CPUMCTX_DBG_HIT_DR1_BIT         (4 + CPUMX86EFLAGS_HW_BITS)
/** Hit guest DR2 breakpoint. */
#define CPUMCTX_DBG_HIT_DR2             RT_BIT_32(CPUMCTX_DBG_HIT_DR2_BIT)
#define CPUMCTX_DBG_HIT_DR2_BIT         (5 + CPUMX86EFLAGS_HW_BITS)
/** Hit guest DR3 breakpoint. */
#define CPUMCTX_DBG_HIT_DR3             RT_BIT_32(CPUMCTX_DBG_HIT_DR3_BIT)
#define CPUMCTX_DBG_HIT_DR3_BIT         (6 + CPUMX86EFLAGS_HW_BITS)
/** Shift for the CPUMCTX_DBG_HIT_DRx bits. */
#define CPUMCTX_DBG_HIT_DRX_SHIFT       CPUMCTX_DBG_HIT_DR0_BIT
/** Mask of all guest pending DR0-DR3 breakpoint indicators. */
#define CPUMCTX_DBG_HIT_DRX_MASK        (CPUMCTX_DBG_HIT_DR0 | CPUMCTX_DBG_HIT_DR1 | CPUMCTX_DBG_HIT_DR2 | CPUMCTX_DBG_HIT_DR3)
/** DBGF event/breakpoint pending. */
#define CPUMCTX_DBG_DBGF_EVENT          RT_BIT_32(CPUMCTX_DBG_DBGF_EVENT_BIT)
#define CPUMCTX_DBG_DBGF_EVENT_BIT      (7 + CPUMX86EFLAGS_HW_BITS)
/** DBGF event/breakpoint pending. */
#define CPUMCTX_DBG_DBGF_BP             RT_BIT_32(CPUMCTX_DBG_DBGF_BP_BIT)
#define CPUMCTX_DBG_DBGF_BP_BIT         (8 + CPUMX86EFLAGS_HW_BITS)
/** Mask of all DBGF indicators. */
#define CPUMCTX_DBG_DBGF_MASK           (CPUMCTX_DBG_DBGF_EVENT | CPUMCTX_DBG_DBGF_BP)
AssertCompile((CPUMCTX_DBG_HIT_DRX_MASK | CPUMCTX_DBG_DBGF_MASK) < UINT32_MAX);
/** @}  */



/**
 * Additional guest MSRs (i.e. not part of the CPU context structure).
 *
 * @remarks Never change the order here because of the saved stated!  The size
 *          can in theory be changed, but keep older VBox versions in mind.
 */
typedef union CPUMCTXMSRS
{
    struct
    {
        uint64_t    TscAux;             /**< MSR_K8_TSC_AUX */
        uint64_t    MiscEnable;         /**< MSR_IA32_MISC_ENABLE */
        uint64_t    MtrrDefType;        /**< IA32_MTRR_DEF_TYPE */
        uint64_t    MtrrFix64K_00000;   /**< IA32_MTRR_FIX16K_80000 */
        uint64_t    MtrrFix16K_80000;   /**< IA32_MTRR_FIX16K_80000 */
        uint64_t    MtrrFix16K_A0000;   /**< IA32_MTRR_FIX16K_A0000 */
        uint64_t    MtrrFix4K_C0000;    /**< IA32_MTRR_FIX4K_C0000 */
        uint64_t    MtrrFix4K_C8000;    /**< IA32_MTRR_FIX4K_C8000 */
        uint64_t    MtrrFix4K_D0000;    /**< IA32_MTRR_FIX4K_D0000 */
        uint64_t    MtrrFix4K_D8000;    /**< IA32_MTRR_FIX4K_D8000 */
        uint64_t    MtrrFix4K_E0000;    /**< IA32_MTRR_FIX4K_E0000 */
        uint64_t    MtrrFix4K_E8000;    /**< IA32_MTRR_FIX4K_E8000 */
        uint64_t    MtrrFix4K_F0000;    /**< IA32_MTRR_FIX4K_F0000 */
        uint64_t    MtrrFix4K_F8000;    /**< IA32_MTRR_FIX4K_F8000 */
        uint64_t    PkgCStateCfgCtrl;   /**< MSR_PKG_CST_CONFIG_CONTROL */
        uint64_t    SpecCtrl;           /**< IA32_SPEC_CTRL */
        uint64_t    ArchCaps;           /**< IA32_ARCH_CAPABILITIES */
    } msr;
    uint64_t    au64[64];
} CPUMCTXMSRS;
/** Pointer to the guest MSR state. */
typedef CPUMCTXMSRS *PCPUMCTXMSRS;
/** Pointer to the const guest MSR state. */
typedef const CPUMCTXMSRS *PCCPUMCTXMSRS;

/** @}  */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_cpumctx_h */

