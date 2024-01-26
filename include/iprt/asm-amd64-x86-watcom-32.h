/** @file
 * IPRT - AMD64 and x86 Specific Assembly Functions, 32-bit Watcom C pragma aux.
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

#ifndef IPRT_INCLUDED_asm_amd64_x86_watcom_32_h
#define IPRT_INCLUDED_asm_amd64_x86_watcom_32_h
/* no pragma once */

#ifndef IPRT_INCLUDED_asm_amd64_x86_h
# error "Don't include this header directly."
#endif

#ifndef __FLAT__
# error "Only works with flat pointers! (-mf)"
#endif

/*
 * Note! The #undef that preceds the #pragma aux statements is for undoing
 *       the mangling, because the symbol in #pragma aux [symbol] statements
 *       doesn't get subjected to preprocessing.  This is also why we include
 *       the watcom header at both the top and the bottom of asm-amd64-x86.h file.
 */

#undef      ASMGetIDTR
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetIDTR = \
    "sidt fword ptr [ecx]" \
    parm [ecx] \
    modify exact [];
#endif

#undef      ASMGetIdtrLimit
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetIdtrLimit = \
    "sub  esp, 8" \
    "sidt fword ptr [esp]" \
    "mov  cx, [esp]" \
    "add  esp, 8" \
    parm [] \
    value [cx] \
    modify exact [ecx];
#endif

#undef      ASMSetIDTR
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMSetIDTR = \
    "lidt fword ptr [ecx]" \
    parm [ecx] nomemory \
    modify nomemory;
#endif

#undef      ASMGetGDTR
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetGDTR = \
    "sgdt fword ptr [ecx]" \
    parm [ecx] \
    modify exact [];
#endif

#undef      ASMSetGDTR
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMSetGDTR = \
    "lgdt fword ptr [ecx]" \
    parm [ecx] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMGetCS
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetCS = \
    "mov ax, cs" \
    parm [] nomemory \
    value [ax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMGetDS
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetDS = \
    "mov ax, ds" \
    parm [] nomemory \
    value [ax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMGetES
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetES = \
    "mov ax, es" \
    parm [] nomemory \
    value [ax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMGetFS
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetFS = \
    "mov ax, fs" \
    parm [] nomemory \
    value [ax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMGetGS
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetGS = \
    "mov ax, gs" \
    parm [] nomemory \
    value [ax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMGetSS
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetSS = \
    "mov ax, ss" \
    parm [] nomemory \
    value [ax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMGetTR
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetTR = \
    "str ax" \
    parm [] nomemory \
    value [ax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMGetLDTR
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetLDTR = \
    "sldt ax" \
    parm [] nomemory \
    value [ax] \
    modify exact [eax] nomemory;
#endif

/** @todo ASMGetSegAttr   */

#undef      ASMGetFlags
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetFlags = \
    "pushfd" \
    "pop eax" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMSetFlags
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMSetFlags = \
    "push eax" \
    "popfd" \
    parm [eax] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMChangeFlags
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMChangeFlags = \
    "pushfd" \
    "pop eax" \
    "and edx, eax" \
    "or  edx, ecx" \
    "push edx" \
    "popfd" \
    parm [edx] [ecx] nomemory \
    value [eax] \
    modify exact [edx] nomemory;
#endif

#undef      ASMAddFlags
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMAddFlags = \
    "pushfd" \
    "pop eax" \
    "or  edx, eax" \
    "push edx" \
    "popfd" \
    parm [edx] nomemory \
    value [eax] \
    modify exact [edx] nomemory;
#endif

#undef      ASMClearFlags
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMClearFlags = \
    "pushfd" \
    "pop eax" \
    "and edx, eax" \
    "push edx" \
    "popfd" \
    parm [edx] nomemory \
    value [eax] \
    modify exact [edx] nomemory;
#endif

/* Note! Must use the 64-bit integer return value convension.
         The order of registers in the value [set] does not seem to mean anything. */
#undef      ASMReadTSC
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMReadTSC = \
    ".586" \
    "rdtsc" \
    parm [] nomemory \
    value [eax edx] \
    modify exact [edx eax] nomemory;
#endif

#undef      ASMReadTscWithAux
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMReadTscWithAux = \
    0x0f 0x01 0xf9 \
    "mov [ebx], ecx" \
    parm [ebx] \
    value [eax edx] \
    modify exact [eax edx ecx];
#endif

/* ASMCpuId: Implemented externally, too many parameters. */
/* ASMCpuId_Idx_ECX: Implemented externally, too many parameters. */
/* ASMCpuIdExSlow: Always implemented externally. */

#undef      ASMCpuId_ECX_EDX
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMCpuId_ECX_EDX = \
    ".586" \
    "cpuid" \
    "mov [edi], ecx" \
    "mov [esi], edx" \
    parm [eax] [edi] [esi] \
    modify exact [eax ebx ecx edx];
#endif

#undef      ASMCpuId_EAX
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMCpuId_EAX = \
    ".586" \
    "cpuid" \
    parm [eax] \
    value [eax] \
    modify exact [eax ebx ecx edx];
#endif

#undef      ASMCpuId_EBX
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMCpuId_EBX = \
    ".586" \
    "cpuid" \
    parm [eax] \
    value [ebx] \
    modify exact [eax ebx ecx edx];
#endif

#undef      ASMCpuId_ECX
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMCpuId_ECX = \
    ".586" \
    "cpuid" \
    parm [eax] \
    value [ecx] \
    modify exact [eax ebx ecx edx];
#endif

#undef      ASMCpuId_EDX
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMCpuId_EDX = \
    ".586" \
    "cpuid" \
    parm [eax] \
    value [edx] \
    modify exact [eax ebx ecx edx];
#endif

/* ASMHasCpuId: MSC inline in main source file. */

#undef ASMGetApicId
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetApicId = \
    ".586" \
    "xor eax, eax" \
    "cpuid" \
    "shr ebx,24" \
    parm [] \
    value [bl] \
    modify exact [eax ebx ecx edx];
#endif

#undef      ASMGetCR0
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetCR0 = \
    "mov eax, cr0" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMSetCR0
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMSetCR0 = \
    "mov cr0, eax" \
    parm [eax] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMGetCR2
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetCR2 = \
    "mov eax, cr2" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMSetCR2
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMSetCR2 = \
    "mov cr2, eax" \
    parm [eax] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMGetCR3
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetCR3 = \
    "mov eax, cr3" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMSetCR3
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMSetCR3 = \
    "mov cr3, eax" \
    parm [eax] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMReloadCR3
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMReloadCR3 = \
    "mov eax, cr3" \
    "mov cr3, eax" \
    parm [] nomemory \
    modify exact [eax] nomemory;
#endif

#undef      ASMGetCR4
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetCR4 = \
    "mov eax, cr4" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMSetCR4
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMSetCR4 = \
    "mov cr4, eax" \
    parm [eax] nomemory \
    modify exact [] nomemory;
#endif

/* ASMGetCR8: Don't bother for 32-bit. */
/* ASMSetCR8: Don't bother for 32-bit. */

#undef      ASMIntEnable
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMIntEnable = \
    "sti" \
    parm [] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMIntDisable
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMIntDisable = \
    "cli" \
    parm [] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMIntDisableFlags
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMIntDisableFlags = \
    "pushfd" \
    "cli" \
    "pop eax" \
    parm [] nomemory \
    value [eax] \
    modify exact [] nomemory;
#endif

#undef      ASMHalt
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMHalt = \
    "hlt" \
    parm [] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMRdMsr
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMRdMsr = \
    ".586" \
    "rdmsr" \
    parm [ecx] nomemory \
    value [eax edx] \
    modify exact [eax edx] nomemory;
#endif

#undef      ASMWrMsr
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMWrMsr = \
    ".586" \
    "wrmsr" \
    parm [ecx] [eax edx] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMRdMsrEx
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMRdMsrEx = \
    ".586" \
    "rdmsr" \
    parm [ecx] [edi] nomemory \
    value [eax edx] \
    modify exact [eax edx] nomemory;
#endif

#undef      ASMWrMsrEx
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMWrMsrEx = \
    ".586" \
    "wrmsr" \
    parm [ecx] [edi] [eax edx] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMRdMsr_Low
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMRdMsr_Low = \
    ".586" \
    "rdmsr" \
    parm [ecx] nomemory \
    value [eax] \
    modify exact [eax edx] nomemory;
#endif

#undef      ASMRdMsr_High
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMRdMsr_High = \
    ".586" \
    "rdmsr" \
    parm [ecx] nomemory \
    value [edx] \
    modify exact [eax edx] nomemory;
#endif


#undef      ASMGetDR0
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetDR0 = \
    "mov eax, dr0" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMGetDR1
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetDR1 = \
    "mov eax, dr1" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMGetDR2
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetDR2 = \
    "mov eax, dr2" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMGetDR3
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetDR3 = \
    "mov eax, dr3" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMGetDR6
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetDR6 = \
    "mov eax, dr6" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMGetAndClearDR6
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetAndClearDR6 = \
    "mov edx, 0ffff0ff0h" \
    "mov eax, dr6" \
    "mov dr6, edx" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax edx] nomemory;
#endif

#undef      ASMGetDR7
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMGetDR7 = \
    "mov eax, dr7" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMSetDR0
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMSetDR0 = \
    "mov dr0, eax" \
    parm [eax] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMSetDR1
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMSetDR1 = \
    "mov dr1, eax" \
    parm [eax] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMSetDR2
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMSetDR2 = \
    "mov dr2, eax" \
    parm [eax] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMSetDR3
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMSetDR3 = \
    "mov dr3, eax" \
    parm [eax] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMSetDR6
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMSetDR6 = \
    "mov dr6, eax" \
    parm [eax] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMSetDR7
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMSetDR7 = \
    "mov dr7, eax" \
    parm [eax] nomemory \
    modify exact [] nomemory;
#endif

/* Yeah, could've used outp here, but this keeps the main file simpler. */
#undef      ASMOutU8
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMOutU8 = \
    "out dx, al" \
    parm [dx] [al] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMInU8
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMInU8 = \
    "in al, dx" \
    parm [dx] nomemory \
    value [al] \
    modify exact [] nomemory;
#endif

#undef      ASMOutU16
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMOutU16 = \
    "out dx, ax" \
    parm [dx] [ax] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMInU16
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMInU16 = \
    "in ax, dx" \
    parm [dx] nomemory \
    value [ax] \
    modify exact [] nomemory;
#endif

#undef      ASMOutU32
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMOutU32 = \
    "out dx, eax" \
    parm [dx] [eax] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMInU32
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMInU32 = \
    "in eax, dx" \
    parm [dx] nomemory \
    value [eax] \
    modify exact [] nomemory;
#endif

#undef      ASMOutStrU8
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMOutStrU8 = \
    "rep outsb" \
    parm [dx] [esi] [ecx] nomemory \
    modify exact [esi ecx] nomemory;
#endif

#undef      ASMInStrU8
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMInStrU8 = \
    "rep insb" \
    parm [dx] [edi] [ecx] \
    modify exact [edi ecx];
#endif

#undef      ASMOutStrU16
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMOutStrU16 = \
    "rep outsw" \
    parm [dx] [esi] [ecx] nomemory \
    modify exact [esi ecx] nomemory;
#endif

#undef      ASMInStrU16
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMInStrU16 = \
    "rep insw" \
    parm [dx] [edi] [ecx] \
    modify exact [edi ecx];
#endif

#undef      ASMOutStrU32
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMOutStrU32 = \
    "rep outsd" \
    parm [dx] [esi] [ecx] nomemory \
    modify exact [esi ecx] nomemory;
#endif

#undef      ASMInStrU32
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMInStrU32 = \
    "rep insd" \
    parm [dx] [edi] [ecx] \
    modify exact [edi ecx];
#endif

#undef      ASMInvalidatePage
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMInvalidatePage = \
    "invlpg [eax]" \
    parm [eax] \
    modify exact [];
#endif

#undef      ASMWriteBackAndInvalidateCaches
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMWriteBackAndInvalidateCaches = \
    ".486" \
    "wbinvd" \
    parm [] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMInvalidateInternalCaches
#ifdef IPRT_ASM_AMD64_X86_WATCOM_32_INSTANTIATE
#pragma aux ASMInvalidateInternalCaches = \
    ".486" \
    "invd" \
    parm [] \
    modify exact [];
#endif

#endif /* !IPRT_INCLUDED_asm_amd64_x86_watcom_32_h */

