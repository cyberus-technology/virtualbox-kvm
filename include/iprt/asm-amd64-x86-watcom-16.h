/** @file
 * IPRT - AMD64 and x86 Specific Assembly Functions, 16-bit Watcom C pragma aux.
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

#ifndef IPRT_INCLUDED_asm_amd64_x86_watcom_16_h
#define IPRT_INCLUDED_asm_amd64_x86_watcom_16_h
/* no pragma once */

#ifndef IPRT_INCLUDED_asm_amd64_x86_h
# error "Don't include this header directly."
#endif

/*
 * Turns out we cannot use 'ds' for segment stuff here because the compiler
 * seems to insists on loading the DGROUP segment into 'ds' before calling
 * stuff when using -ecc.  Using 'es' instead as this seems to work fine.
 *
 * Note! The #undef that preceds the #pragma aux statements is for undoing
 *       the mangling, because the symbol in #pragma aux [symbol] statements
 *       doesn't get subjected to preprocessing.  This is also why we include
 *       the watcom header at both the top and the bottom of asm-amd64-x86.h file.
 */

#undef      ASMGetIDTR
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetIDTR = \
    ".286p" \
    "sidt fword ptr es:[bx]" \
    parm [es bx] \
    modify exact [];
#endif

#undef      ASMGetIdtrLimit
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetIdtrLimit = \
    ".286p" \
    "sub  sp, 8" \
    "mov  bx, sp" \
    "sidt fword ptr ss:[bx]" \
    "mov  bx, ss:[bx]" \
    "add  sp, 8" \
    parm [] \
    value [bx] \
    modify exact [bx];
#endif

#undef      ASMSetIDTR
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMSetIDTR = \
    ".286p" \
    "lidt fword ptr es:[bx]" \
    parm [es bx] nomemory \
    modify nomemory;
#endif

#undef      ASMGetGDTR
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetGDTR = \
    ".286p" \
    "sgdt fword ptr es:[bx]" \
    parm [es bx] \
    modify exact [];
#endif

#undef      ASMSetGDTR
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMSetGDTR = \
    ".286p" \
    "lgdt fword ptr es:[bx]" \
    parm [es bx] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMGetCS
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetCS = \
    "mov ax, cs" \
    parm [] nomemory \
    value [ax] \
    modify exact [ax] nomemory;
#endif

#undef      ASMGetDS
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetDS = \
    "mov ax, ds" \
    parm [] nomemory \
    value [ax] \
    modify exact [ax] nomemory;
#endif

#undef      ASMGetES
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetES = \
    "mov ax, es" \
    parm [] nomemory \
    value [ax] \
    modify exact [ax] nomemory;
#endif

#undef      ASMGetFS
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetFS = \
    ".386" \
    "mov ax, fs" \
    parm [] nomemory \
    value [ax] \
    modify exact [ax] nomemory;
#endif

#undef      ASMGetGS
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetGS = \
    ".386" \
    "mov ax, gs" \
    parm [] nomemory \
    value [ax] \
    modify exact [ax] nomemory;
#endif

#undef      ASMGetSS
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetSS = \
    "mov ax, ss" \
    parm [] nomemory \
    value [ax] \
    modify exact [ax] nomemory;
#endif

#undef      ASMGetTR
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetTR = \
    ".286" \
    "str ax" \
    parm [] nomemory \
    value [ax] \
    modify exact [ax] nomemory;
#endif

#undef      ASMGetLDTR
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetLDTR = \
    ".286" \
    "sldt ax" \
    parm [] nomemory \
    value [ax] \
    modify exact [ax] nomemory;
#endif

/** @todo ASMGetSegAttr   */

#undef      ASMGetFlags
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetFlags = \
    "pushf" \
    "pop ax" \
    parm [] nomemory \
    value [ax] \
    modify exact [ax] nomemory;
#endif

#undef      ASMSetFlags
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMSetFlags = \
    "push ax" \
    "popf" \
    parm [ax] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMChangeFlags
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMChangeFlags = \
    "pushf" \
    "pop ax" \
    "and dx, ax" \
    "or  dx, cx" \
    "push dx" \
    "popf" \
    parm [dx] [cx] nomemory \
    value [ax] \
    modify exact [dx] nomemory;
#endif

#undef      ASMAddFlags
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMAddFlags = \
    "pushf" \
    "pop ax" \
    "or  dx, ax" \
    "push dx" \
    "popf" \
    parm [dx] nomemory \
    value [ax] \
    modify exact [dx] nomemory;
#endif

#undef      ASMClearFlags
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMClearFlags = \
    "pushf" \
    "pop ax" \
    "and dx, ax" \
    "push dx" \
    "popf" \
    parm [dx] nomemory \
    value [ax] \
    modify exact [dx] nomemory;
#endif

/* Note! Must use the 64-bit integer return value convension.
         The order of registers in the value [set] does not seem to mean anything. */
#undef      ASMReadTSC
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMReadTSC = \
    ".586" \
    "rdtsc" \
    "mov ebx, edx" \
    "mov ecx, eax" \
    "shr ecx, 16" \
    "xchg eax, edx" \
    "shr eax, 16" \
    parm [] nomemory \
    value [dx cx bx ax] \
    modify exact [ax bx cx dx] nomemory;
#endif

/** @todo ASMReadTscWithAux if needed (rdtscp not recognized by compiler)   */


/* ASMCpuId: Implemented externally, too many parameters. */
/* ASMCpuId_Idx_ECX: Implemented externally, too many parameters. */
/* ASMCpuIdExSlow: Always implemented externally. */
/* ASMCpuId_ECX_EDX: Implemented externally, too many parameters. */

#undef      ASMCpuId_EAX
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMCpuId_EAX = \
    ".586" \
    "xchg ax, dx" \
    "shl eax, 16" \
    "mov ax, dx" \
    "cpuid" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [ax dx] \
    value [ax dx] \
    modify exact [ax bx cx dx];
#endif

#undef      ASMCpuId_EBX
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMCpuId_EBX = \
    ".586" \
    "xchg ax, dx" \
    "shl eax, 16" \
    "mov ax, dx" \
    "cpuid" \
    "mov ax, bx" \
    "shr ebx, 16" \
    parm [ax dx] \
    value [ax bx] \
    modify exact [ax bx cx dx];
#endif

#undef      ASMCpuId_ECX
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMCpuId_ECX = \
    ".586" \
    "xchg ax, dx" \
    "shl eax, 16" \
    "mov ax, dx" \
    "cpuid" \
    "mov ax, cx" \
    "shr ecx, 16" \
    parm [ax dx] \
    value [ax cx] \
    modify exact [ax bx cx dx];
#endif

#undef      ASMCpuId_EDX
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMCpuId_EDX = \
    ".586" \
    "xchg ax, dx" \
    "shl eax, 16" \
    "mov ax, dx" \
    "cpuid" \
    "mov ax, dx" \
    "shr edx, 16" \
    parm [ax dx] \
    value [ax dx] \
    modify exact [ax bx cx dx];
#endif

/* ASMHasCpuId: MSC inline in main source file. */
/* ASMGetApicId: Implemented externally, lazy bird. */

/* Note! Again, when returning two registers, watcom have certain fixed ordering rules (low:high):
            ax:bx, ax:cx, ax:dx, ax:si, ax:di
            bx:cx, bx:dx, bx:si, bx:di
            dx:cx, si:cx, di:cx
            si:dx, di:dx
            si:di
         This ordering seems to apply to parameter values too. */
#undef      ASMGetCR0
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetCR0 = \
    ".386" \
    "mov eax, cr0" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;
#endif

#undef      ASMSetCR0
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMSetCR0 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "mov cr0, edx" \
    parm [ax dx] nomemory \
    modify exact [dx] nomemory;
#endif

#undef      ASMGetCR2
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetCR2 = \
    ".386" \
    "mov eax, cr2" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;
#endif

#undef      ASMSetCR2
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMSetCR2 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "mov cr2, edx" \
    parm [ax dx] nomemory \
    modify exact [dx] nomemory;
#endif

#undef      ASMGetCR3
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetCR3 = \
    ".386" \
    "mov eax, cr3" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;
#endif

#undef      ASMSetCR3
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMSetCR3 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "mov cr3, edx" \
    parm [ax dx] nomemory \
    modify exact [dx] nomemory;
#endif

#undef      ASMReloadCR3
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMReloadCR3 = \
    ".386" \
    "mov eax, cr3" \
    "mov cr3, eax" \
    parm [] nomemory \
    modify exact [ax] nomemory;
#endif

#undef      ASMGetCR4
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetCR4 = \
    ".386" \
    "mov eax, cr4" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;
#endif

#undef      ASMSetCR4
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMSetCR4 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "mov cr4, edx" \
    parm [ax dx] nomemory \
    modify exact [dx] nomemory;
#endif

/* ASMGetCR8: Don't bother for 16-bit. */
/* ASMSetCR8: Don't bother for 16-bit. */

#undef      ASMIntEnable
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMIntEnable = \
    "sti" \
    parm [] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMIntDisable
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMIntDisable = \
    "cli" \
    parm [] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMIntDisableFlags
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMIntDisableFlags = \
    "pushf" \
    "cli" \
    "pop ax" \
    parm [] nomemory \
    value [ax] \
    modify exact [] nomemory;
#endif

#undef      ASMHalt
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMHalt = \
    "hlt" \
    parm [] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMRdMsr
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMRdMsr = \
    ".586" \
    "shl ecx, 16" \
    "mov cx, ax" \
    "rdmsr" \
    "mov ebx, edx" \
    "mov ecx, eax" \
    "shr ecx, 16" \
    "xchg eax, edx" \
    "shr eax, 16" \
    parm [ax cx] nomemory \
    value [dx cx bx ax] \
    modify exact [ax bx cx dx] nomemory;
#endif

/* ASMWrMsr: Implemented externally, lazy bird. */
/* ASMRdMsrEx: Implemented externally, lazy bird. */
/* ASMWrMsrEx: Implemented externally, lazy bird. */

#undef      ASMRdMsr_Low
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMRdMsr_Low = \
    ".586" \
    "shl ecx, 16" \
    "mov cx, ax" \
    "rdmsr" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [ax cx] nomemory \
    value [ax dx] \
    modify exact [ax bx cx dx] nomemory;
#endif

#undef      ASMRdMsr_High
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMRdMsr_High = \
    ".586" \
    "shl ecx, 16" \
    "mov cx, ax" \
    "rdmsr" \
    "mov eax, edx" \
    "shr edx, 16" \
    parm [ax cx] nomemory \
    value [ax dx] \
    modify exact [ax bx cx dx] nomemory;
#endif


#undef      ASMGetDR0
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetDR0 = \
    ".386" \
    "mov eax, dr0" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;
#endif

#undef      ASMGetDR1
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetDR1 = \
    ".386" \
    "mov eax, dr1" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;
#endif

#undef      ASMGetDR2
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetDR2 = \
    ".386" \
    "mov eax, dr2" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;
#endif

#undef      ASMGetDR3
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetDR3 = \
    ".386" \
    "mov eax, dr3" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;
#endif

#undef      ASMGetDR6
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetDR6 = \
    ".386" \
    "mov eax, dr6" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;
#endif

#undef      ASMGetAndClearDR6
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetAndClearDR6 = \
    ".386" \
    "mov edx, 0ffff0ff0h" \
    "mov eax, dr6" \
    "mov dr6, edx" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;
#endif

#undef      ASMGetDR7
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMGetDR7 = \
    ".386" \
    "mov eax, dr7" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;
#endif

#undef      ASMSetDR0
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMSetDR0 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "mov dr0, edx" \
    parm [ax dx] nomemory \
    modify exact [dx] nomemory;
#endif

#undef      ASMSetDR1
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMSetDR1 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "mov dr1, edx" \
    parm [ax dx] nomemory \
    modify exact [dx] nomemory;
#endif

#undef      ASMSetDR2
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMSetDR2 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "mov dr2, edx" \
    parm [ax dx] nomemory \
    modify exact [dx] nomemory;
#endif

#undef      ASMSetDR3
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMSetDR3 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "mov dr3, edx" \
    parm [ax dx] nomemory \
    modify exact [dx] nomemory;
#endif

#undef      ASMSetDR6
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMSetDR6 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "mov dr6, edx" \
    parm [ax dx] nomemory \
    modify exact [dx] nomemory;
#endif

#undef      ASMSetDR7
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMSetDR7 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "mov dr7, edx" \
    parm [ax dx] nomemory \
    modify exact [dx] nomemory;
#endif

/* Yeah, could've used outp here, but this keeps the main file simpler. */
#undef      ASMOutU8
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMOutU8 = \
    "out dx, al" \
    parm [dx] [al] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMInU8
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMInU8 = \
    "in al, dx" \
    parm [dx] nomemory \
    value [al] \
    modify exact [] nomemory;
#endif

#undef      ASMOutU16
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMOutU16 = \
    "out dx, ax" \
    parm [dx] [ax] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMInU16
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMInU16 = \
    "in ax, dx" \
    parm [dx] nomemory \
    value [ax] \
    modify exact [] nomemory;
#endif

#undef      ASMOutU32
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMOutU32 = \
    ".386" \
    "shl ecx, 16" \
    "mov cx, ax" \
    "mov eax, ecx" \
    "out dx, eax" \
    parm [dx] [ax cx] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMInU32
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMInU32 = \
    ".386" \
    "in eax, dx" \
    "mov ecx, eax" \
    "shr ecx, 16" \
    parm [dx] nomemory \
    value [ax cx] \
    modify exact [] nomemory;
#endif

#undef      ASMOutStrU8
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMOutStrU8 = \
    ".186" \
    "mov ax, ds" \
    "mov ds, di" \
    "rep outsb" \
    "mov ds, ax" \
    parm [dx] [si di] [cx] nomemory \
    modify exact [si cx ax] nomemory;
#endif

#undef      ASMInStrU8
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMInStrU8 = \
    ".186" \
    "rep insb" \
    parm [dx] [di es] [cx] \
    modify exact [di cx];
#endif

#undef      ASMOutStrU16
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMOutStrU16 = \
    ".186" \
    "mov ax, ds" \
    "mov ds, di" \
    "rep outsw" \
    "mov ds, ax" \
    parm [dx] [si di] [cx] nomemory \
    modify exact [si cx ax] nomemory;
#endif

#undef      ASMInStrU16
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMInStrU16 = \
    ".186" \
    "rep insw" \
    parm [dx] [di es] [cx] \
    modify exact [di cx];
#endif

#undef      ASMOutStrU32
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMOutStrU32 = \
    ".386" \
    "mov ax, ds" \
    "mov ds, di" \
    "rep outsd" \
    "mov ds, ax" \
    parm [dx] [si di] [cx] nomemory \
    modify exact [si cx ax] nomemory;
#endif

#undef      ASMInStrU32
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMInStrU32 = \
    ".386" \
    "rep insd" \
    parm [dx] [es di] [cx] \
    modify exact [di cx];
#endif

#undef ASMInvalidatePage
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMInvalidatePage = \
    ".486" \
    "shl edx, 16" \
    "mov dx, ax" \
    "invlpg [edx]" \
    parm [ax dx] \
    modify exact [dx];
#endif

#undef      ASMWriteBackAndInvalidateCaches
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMWriteBackAndInvalidateCaches = \
    ".486" \
    "wbinvd" \
    parm [] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMInvalidateInternalCaches
#ifdef IPRT_ASM_AMD64_X86_WATCOM_16_INSTANTIATE
#pragma aux ASMInvalidateInternalCaches = \
    ".486" \
    "invd" \
    parm [] \
    modify exact [];
#endif

#endif /* !IPRT_INCLUDED_asm_amd64_x86_watcom_16_h */

