/** @file
 * GVM - The Global VM Data.
 */

/*
 * Copyright (C) 2007-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_vmm_uvm_h
#define VBOX_INCLUDED_vmm_uvm_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#include <iprt/assert.h>

/** @addtogroup grp_vm
 * @{ */


/**
 * Per virtual CPU ring-3 (user mode) data.
 */
typedef struct UVMCPU
{
    /** Pointer to the UVM structure.  */
    PUVM                            pUVM;
    /** Pointer to the VM structure.  */
    PVM                             pVM;
    /** Pointer to the VMCPU structure.  */
    PVMCPU                          pVCpu;
    /** The virtual CPU ID.  */
    RTCPUID                         idCpu;
    /** Alignment padding. */
    uint8_t                         abAlignment0[HC_ARCH_BITS == 32 ? 16 : 4];

    /** The VM internal data. */
    union
    {
#ifdef VMM_INCLUDED_SRC_include_VMInternal_h
        struct VMINTUSERPERVMCPU    s;
#endif
        uint8_t                     padding[512];
    } vm;

    /** The DBGF data. */
    union
    {
#ifdef VMM_INCLUDED_SRC_include_DBGFInternal_h
        struct DBGFUSERPERVMCPU     s;
#endif
        uint8_t                     padding[64];
    } dbgf;

} UVMCPU;
AssertCompileMemberAlignment(UVMCPU, vm, 32);


/**
 * The ring-3 (user mode) VM structure.
 *
 * This structure is similar to VM and GVM except that it resides in swappable
 * user memory. The main purpose is to assist bootstrapping, where it allows us
 * to start EMT much earlier and gives PDMLdr somewhere to put it's VMMR0 data.
 * It is also a nice place to put big things that are user mode only.
 */
typedef struct UVM
{
    /** Magic / eye-catcher (UVM_MAGIC). */
    uint32_t            u32Magic;
    /** The number of virtual CPUs. */
    uint32_t            cCpus;
    /** The ring-3 mapping of the shared VM structure. */
    PVM                 pVM;
    /** Pointer to the next VM.
     * We keep a per process list of VM for the event that a process could
     * contain more than one VM.
     * @todo move this into vm.s!
     */
    struct UVM         *pNext;

    /** Pointer to the optional method table provided by the VMM user. */
    PCVMM2USERMETHODS   pVmm2UserMethods;

#if HC_ARCH_BITS == 32
    /** Align the next member on a 32 byte boundary. */
    uint8_t             abAlignment0[HC_ARCH_BITS == 32 ? 12 : 0];
#endif

    /** The VM internal data. */
    union
    {
#ifdef VMM_INCLUDED_SRC_include_VMInternal_h
        struct VMINTUSERPERVM   s;
#endif
        uint8_t                 padding[512];
    } vm;

    /** The MM data. */
    union
    {
#ifdef VMM_INCLUDED_SRC_include_MMInternal_h
        struct MMUSERPERVM      s;
#endif
        uint8_t                 padding[32];
    } mm;

    /** The PDM data. */
    union
    {
#ifdef VMM_INCLUDED_SRC_include_PDMInternal_h
        struct PDMUSERPERVM     s;
#endif
        uint8_t                 padding[256];
    } pdm;

    /** The STAM data. */
    union
    {
#ifdef VMM_INCLUDED_SRC_include_STAMInternal_h
        struct STAMUSERPERVM    s;
#endif
        uint8_t                 padding[30208];
    } stam;

    /** The DBGF data. */
    union
    {
#ifdef VMM_INCLUDED_SRC_include_DBGFInternal_h
        struct DBGFUSERPERVM    s;
#endif
        uint8_t                 padding[1024];
    } dbgf;

    /** Per virtual CPU data. */
    UVMCPU                      aCpus[1];
} UVM;
AssertCompileMemberAlignment(UVM, vm, 32);
AssertCompileMemberAlignment(UVM, mm, 32);
AssertCompileMemberAlignment(UVM, pdm, 32);
AssertCompileMemberAlignment(UVM, stam, 32);
AssertCompileMemberAlignment(UVM, aCpus, 32);

/** The UVM::u32Magic value (Brad Mehldau). */
#define UVM_MAGIC       0x19700823

/** @def UVM_ASSERT_VALID_EXT_RETURN
 * Asserts a user mode VM handle is valid for external access.
 */
#define UVM_ASSERT_VALID_EXT_RETURN(a_pUVM, a_rc) \
        AssertMsgReturn(    RT_VALID_ALIGNED_PTR(a_pUVM, PAGE_SIZE) \
                        &&  (a_pUVM)->u32Magic == UVM_MAGIC, \
                        ("a_pUVM=%p u32Magic=%#x\n", (a_pUVM), \
                         RT_VALID_ALIGNED_PTR(a_pUVM, PAGE_SIZE) ? (a_pUVM)->u32Magic : 0), \
                        (a_rc))
/** @def UVM_ASSERT_VALID_EXT_RETURN
 * Asserts a user mode VM handle is valid for external access.
 */
#define UVM_ASSERT_VALID_EXT_RETURN_VOID(a_pUVM) \
        AssertMsgReturnVoid(    RT_VALID_ALIGNED_PTR(a_pUVM, PAGE_SIZE) \
                            &&  (a_pUVM)->u32Magic == UVM_MAGIC, \
                            ("a_pUVM=%p u32Magic=%#x\n", (a_pUVM), \
                             RT_VALID_ALIGNED_PTR(a_pUVM, PAGE_SIZE) ? (a_pUVM)->u32Magic : 0))

/** @} */
#endif /* !VBOX_INCLUDED_vmm_uvm_h */

