/** @file
 * IPRT - Assembly Functions, x86 32-bit Watcom C/C++ pragma aux.
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

#ifndef IPRT_INCLUDED_asm_watcom_x86_32_h
#define IPRT_INCLUDED_asm_watcom_x86_32_h
/* no pragma once */

#ifndef IPRT_INCLUDED_asm_h
# error "Don't include this header directly."
#endif

#ifndef __FLAT__
# error "Only works with flat pointers! (-mf)"
#endif

/*
 * Note! The #undef that preceds the #pragma aux statements is for undoing
 *       the mangling, because the symbol in #pragma aux [symbol] statements
 *       doesn't get subjected to preprocessing.  This is also why we include
 *       the watcom header at both the top and the bottom of asm.h file.
 */

#undef       ASMCompilerBarrier
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
# if 0 /* overkill version. */
#  pragma aux ASMCompilerBarrier = \
    "nop" \
    parm [] \
    modify exact [eax ebx ecx edx es ds fs gs];
# else
#  pragma aux ASMCompilerBarrier = \
    "" \
    parm [] \
    modify exact [];
# endif
#endif

#undef      ASMNopPause
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMNopPause = \
    ".686p" \
    ".xmm2" \
    "pause" \
    parm [] nomemory \
    modify exact [] nomemory;
#endif

#undef      ASMAtomicXchgU8
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicXchgU8 = \
    "xchg [ecx], al" \
    parm [ecx] [al] \
    value [al] \
    modify exact [al];
#endif

#undef      ASMAtomicXchgU16
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicXchgU16 = \
    "xchg [ecx], ax" \
    parm [ecx] [ax] \
    value [ax] \
    modify exact [ax];
#endif

#undef      ASMAtomicXchgU32
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicXchgU32 = \
    "xchg [ecx], eax" \
    parm [ecx] [eax] \
    value [eax] \
    modify exact [eax];
#endif

#undef      ASMAtomicXchgU64
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicXchgU64 = \
    ".586" \
    "try_again:" \
    "lock cmpxchg8b [esi]" \
    "jnz try_again" \
    parm [esi] [ebx ecx] \
    value [eax edx] \
    modify exact [edx ecx ebx eax];
#endif

#undef      ASMAtomicCmpXchgU8
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicCmpXchgU8 = \
    ".486" \
    "lock cmpxchg [edx], cl" \
    "setz al" \
    parm [edx] [cl] [al] \
    value [al] \
    modify exact [al];
#endif

#undef      ASMAtomicCmpXchgU16
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicCmpXchgU16 = \
    ".486" \
    "lock cmpxchg [edx], cx" \
    "setz al" \
    parm [edx] [cx] [ax] \
    value [al] \
    modify exact [ax];
#endif

#undef      ASMAtomicCmpXchgU32
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicCmpXchgU32 = \
    ".486" \
    "lock cmpxchg [edx], ecx" \
    "setz al" \
    parm [edx] [ecx] [eax] \
    value [al] \
    modify exact [eax];
#endif

#undef      ASMAtomicCmpXchgU64
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicCmpXchgU64 = \
    ".586" \
    "lock cmpxchg8b [edi]" \
    "setz al" \
    parm [edi] [ebx ecx] [eax edx] \
    value [al] \
    modify exact [eax edx];
#endif

#undef      ASMAtomicCmpXchgExU32
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicCmpXchgExU32 = \
    ".586" \
    "lock cmpxchg [edx], ecx" \
    "mov [edi], eax" \
    "setz al" \
    parm [edx] [ecx] [eax] [edi] \
    value [al] \
    modify exact [eax];
#endif

#undef      ASMAtomicCmpXchgExU64
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicCmpXchgExU64 = \
    ".586" \
    "lock cmpxchg8b [edi]" \
    "mov [esi], eax" \
    "mov [esi + 4], edx" \
    "setz al" \
    parm [edi] [ebx ecx] [eax edx] [esi] \
    value [al] \
    modify exact [eax edx];
#endif

#undef      ASMSerializeInstructionCpuId
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMSerializeInstructionCpuId = \
    ".586" \
    "xor eax, eax" \
    "cpuid" \
    parm [] \
    modify exact [eax ebx ecx edx];
#endif

#undef ASMSerializeInstructionIRet
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMSerializeInstructionIRet = \
    "pushf" \
    "push cs" \
    "call foo" /* 'push offset done' doesn't work */ \
    "jmp  done" \
    "foo:" \
    "iret" \
    "done:" \
    parm [] \
    modify exact [];
#endif

#undef      ASMSerializeInstructionRdTscp
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMSerializeInstructionRdTscp = \
    0x0f 0x01 0xf9 \
    parm [] \
    modify exact [eax edx ecx];
#endif

#undef      ASMAtomicReadU64
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicReadU64 = \
    ".586" \
    "xor eax, eax" \
    "mov edx, eax" \
    "mov ebx, eax" \
    "mov ecx, eax" \
    "lock cmpxchg8b [edi]" \
    parm [edi] \
    value [eax edx] \
    modify exact [eax ebx ecx edx];
#endif

#undef      ASMAtomicUoReadU64
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicUoReadU64 = \
    ".586" \
    "xor eax, eax" \
    "mov edx, eax" \
    "mov ebx, eax" \
    "mov ecx, eax" \
    "lock cmpxchg8b [edi]" \
    parm [edi] \
    value [eax edx] \
    modify exact [eax ebx ecx edx];
#endif

#undef      ASMAtomicAddU16
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicAddU16 = \
    ".486" \
    "lock xadd [ecx], ax" \
    parm [ecx] [ax] \
    value [ax] \
    modify exact [ax];
#endif

#undef      ASMAtomicAddU32
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicAddU32 = \
    ".486" \
    "lock xadd [ecx], eax" \
    parm [ecx] [eax] \
    value [eax] \
    modify exact [eax];
#endif

#undef      ASMAtomicIncU16
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicIncU16 = \
    ".486" \
    "mov ax, 1" \
    "lock xadd [ecx], ax" \
    "inc ax" \
    parm [ecx] \
    value [ax] \
    modify exact [ax];
#endif

#undef      ASMAtomicIncU32
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicIncU32 = \
    ".486" \
    "mov eax, 1" \
    "lock xadd [ecx], eax" \
    "inc eax" \
    parm [ecx] \
    value [eax] \
    modify exact [eax];
#endif

/* ASMAtomicIncU64: Should be done by C inline or in external file. */

#undef      ASMAtomicDecU16
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicDecU16 = \
    ".486" \
    "mov ax, 0ffffh" \
    "lock xadd [ecx], ax" \
    "dec ax" \
    parm [ecx] \
    value [ax] \
    modify exact [ax];
#endif

#undef      ASMAtomicDecU32
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicDecU32 = \
    ".486" \
    "mov eax, 0ffffffffh" \
    "lock xadd [ecx], eax" \
    "dec eax" \
    parm [ecx] \
    value [eax] \
    modify exact [eax];
#endif

/* ASMAtomicDecU64: Should be done by C inline or in external file. */

#undef      ASMAtomicOrU32
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicOrU32 = \
    "lock or [ecx], eax" \
    parm [ecx] [eax] \
    modify exact [];
#endif

/* ASMAtomicOrU64: Should be done by C inline or in external file. */

#undef      ASMAtomicAndU32
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicAndU32 = \
    "lock and [ecx], eax" \
    parm [ecx] [eax] \
    modify exact [];
#endif

/* ASMAtomicAndU64: Should be done by C inline or in external file. */

#undef      ASMAtomicUoOrU32
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicUoOrU32 = \
    "or [ecx], eax" \
    parm [ecx] [eax] \
    modify exact [];
#endif

/* ASMAtomicUoOrU64: Should be done by C inline or in external file. */

#undef      ASMAtomicUoAndU32
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicUoAndU32 = \
    "and [ecx], eax" \
    parm [ecx] [eax] \
    modify exact [];
#endif

/* ASMAtomicUoAndU64: Should be done by C inline or in external file. */

#undef      ASMAtomicUoIncU32
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicUoIncU32 = \
    ".486" \
    "xadd [ecx], eax" \
    "inc eax" \
    parm [ecx] \
    value [eax] \
    modify exact [eax];
#endif

#undef      ASMAtomicUoDecU32
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicUoDecU32 = \
    ".486" \
    "mov eax, 0ffffffffh" \
    "xadd [ecx], eax" \
    "dec eax" \
    parm [ecx] \
    value [eax] \
    modify exact [eax];
#endif

#undef      ASMMemZeroPage
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMMemZeroPage = \
    "mov ecx, 1024" \
    "xor eax, eax" \
    "rep stosd"  \
    parm [edi] \
    modify exact [eax ecx edi];
#endif

#undef      ASMMemZero32
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMMemZero32 = \
    "shr ecx, 2" \
    "xor eax, eax" \
    "rep stosd"  \
    parm [edi] [ecx] \
    modify exact [eax ecx edi];
#endif

#undef      ASMMemFill32
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMMemFill32 = \
    "shr ecx, 2" \
    "rep stosd"  \
    parm [edi] [ecx] [eax]\
    modify exact [ecx edi];
#endif

#undef      ASMProbeReadByte
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMProbeReadByte = \
    "mov al, [ecx]" \
    parm [ecx] \
    value [al] \
    modify exact [al];
#endif

#undef      ASMBitSet
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMBitSet = \
    "bts [ecx], eax" \
    parm [ecx] [eax] \
    modify exact [];
#endif

#undef      ASMAtomicBitSet
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicBitSet = \
    "lock bts [ecx], eax" \
    parm [ecx] [eax] \
    modify exact [];
#endif

#undef      ASMBitClear
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMBitClear = \
    "btr [ecx], eax" \
    parm [ecx] [eax] \
    modify exact [];
#endif

#undef      ASMAtomicBitClear
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicBitClear = \
    "lock btr [ecx], eax" \
    parm [ecx] [eax] \
    modify exact [];
#endif

#undef      ASMBitToggle
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMBitToggle = \
    "btc [ecx], eax" \
    parm [ecx] [eax] \
    modify exact [];
#endif

#undef      ASMAtomicBitToggle
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicBitToggle = \
    "lock btc [ecx], eax" \
    parm [ecx] [eax] \
    modify exact [];
#endif


#undef      ASMBitTestAndSet
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMBitTestAndSet = \
    "bts [ecx], eax" \
    "setc al" \
    parm [ecx] [eax] \
    value [al] \
    modify exact [eax];
#endif

#undef      ASMAtomicBitTestAndSet
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicBitTestAndSet = \
    "lock bts [ecx], eax" \
    "setc al" \
    parm [ecx] [eax] \
    value [al] \
    modify exact [eax];
#endif

#undef      ASMBitTestAndClear
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMBitTestAndClear = \
    "btr [ecx], eax" \
    "setc al" \
    parm [ecx] [eax] \
    value [al] \
    modify exact [eax];
#endif

#undef      ASMAtomicBitTestAndClear
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicBitTestAndClear = \
    "lock btr [ecx], eax" \
    "setc al" \
    parm [ecx] [eax] \
    value [al] \
    modify exact [eax];
#endif

#undef      ASMBitTestAndToggle
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMBitTestAndToggle = \
    "btc [ecx], eax" \
    "setc al" \
    parm [ecx] [eax] \
    value [al] \
    modify exact [eax];
#endif

#undef      ASMAtomicBitTestAndToggle
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMAtomicBitTestAndToggle = \
    "lock btc [ecx], eax" \
    "setc al" \
    parm [ecx] [eax] \
    value [al] \
    modify exact [eax];
#endif

#undef      ASMBitTest
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMBitTest = \
    "bt  [ecx], eax" \
    "setc al" \
    parm [ecx] [eax] nomemory \
    value [al] \
    modify exact [eax] nomemory;
#endif

#if 0
/** @todo this is way to much inline assembly, better off in an external function. */
#undef      ASMBitFirstClear
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMBitFirstClear = \
    "mov edx, edi" /* save start of bitmap for later */ \
    "add ecx, 31" \
    "shr ecx, 5" /* cDWord = RT_ALIGN_32(cBits, 32) / 32; */  \
    "mov eax, 0ffffffffh" \
    "repe scasd" \
    "je done" \
    "lea edi, [edi - 4]" /* rewind edi */ \
    "xor eax, [edi]" /* load inverted bits */ \
    "sub edi, edx" /* calc byte offset */ \
    "shl edi, 3" /* convert byte to bit offset */ \
    "mov edx, eax" \
    "bsf eax, edx" \
    "add eax, edi" \
    "done:" \
    parm [edi] [ecx] \
    value [eax] \
    modify exact [eax ecx edx edi];
#endif

/* ASMBitNextClear: Too much work, do when needed. */

/** @todo this is way to much inline assembly, better off in an external function. */
#undef      ASMBitFirstSet
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMBitFirstSet = \
    "mov edx, edi" /* save start of bitmap for later */ \
    "add ecx, 31" \
    "shr ecx, 5" /* cDWord = RT_ALIGN_32(cBits, 32) / 32; */  \
    "mov eax, 0ffffffffh" \
    "repe scasd" \
    "je done" \
    "lea edi, [edi - 4]" /* rewind edi */ \
    "mov eax, [edi]" /* reload previous dword */ \
    "sub edi, edx" /* calc byte offset */ \
    "shl edi, 3" /* convert byte to bit offset */ \
    "mov edx, eax" \
    "bsf eax, edx" \
    "add eax, edi" \
    "done:" \
    parm [edi] [ecx] \
    value [eax] \
    modify exact [eax ecx edx edi];
#endif

/* ASMBitNextSet: Too much work, do when needed. */
#else
/* ASMBitFirstClear: External file.  */
/* ASMBitNextClear:  External file.  */
/* ASMBitFirstSet:   External file.  */
/* ASMBitNextSet:    External file.  */
#endif

#undef      ASMBitFirstSetU32
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMBitFirstSetU32 = \
    "bsf eax, eax" \
    "jz  not_found" \
    "inc eax" \
    "jmp done" \
    "not_found:" \
    "xor eax, eax" \
    "done:" \
    parm [eax] nomemory \
    value [eax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMBitFirstSetU64
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMBitFirstSetU64 = \
    "bsf eax, eax" \
    "jz  not_found_low" \
    "inc eax" \
    "jmp done" \
    \
    "not_found_low:" \
    "bsf eax, edx" \
    "jz  not_found_high" \
    "add eax, 33" \
    "jmp done" \
    \
    "not_found_high:" \
    "xor eax, eax" \
    "done:" \
    parm [eax edx] nomemory \
    value [eax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMBitFirstSetU16
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMBitFirstSetU16 = \
    "movzx eax, ax" \
    "bsf eax, eax" \
    "jz  not_found" \
    "inc eax" \
    "jmp done" \
    "not_found:" \
    "xor eax, eax" \
    "done:" \
    parm [ax] nomemory \
    value [eax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMBitLastSetU32
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMBitLastSetU32 = \
    "bsr eax, eax" \
    "jz  not_found" \
    "inc eax" \
    "jmp done" \
    "not_found:" \
    "xor eax, eax" \
    "done:" \
    parm [eax] nomemory \
    value [eax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMBitLastSetU64
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMBitLastSetU64 = \
    "xchg eax, edx" \
    "bsr eax, eax" \
    "jz  not_found_high" \
    "add eax, 33" \
    "jmp done" \
    \
    "not_found_high:" \
    "bsr eax, edx" \
    "jz  not_found" \
    "inc eax" \
    "jmp done" \
    \
    "not_found:" \
    "xor eax, eax" \
    "done:" \
    parm [eax edx] nomemory \
    value [eax] \
    modify exact [eax edx] nomemory;
#endif

#undef      ASMBitLastSetU16
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMBitLastSetU16 = \
    "movzx eax, ax" \
    "bsr eax, eax" \
    "jz  not_found" \
    "inc eax" \
    "jmp done" \
    "not_found:" \
    "xor eax, eax" \
    "done:" \
    parm [ax] nomemory \
    value [eax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMByteSwapU16
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMByteSwapU16 = \
    "ror ax, 8" \
    parm [ax] nomemory \
    value [ax] \
    modify exact [ax] nomemory;
#endif

#undef      ASMByteSwapU32
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMByteSwapU32 = \
    "bswap eax" \
    parm [eax] nomemory \
    value [eax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMRotateLeftU32
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMRotateLeftU32 = \
    "rol    eax, cl" \
    parm [eax] [ecx] nomemory \
    value [eax] \
    modify exact [eax] nomemory;
#endif

#undef      ASMRotateRightU32
#ifdef IPRT_ASM_WATCOM_X86_32_WITH_PRAGMAS
#pragma aux ASMRotateRightU32 = \
    "ror    eax, cl" \
    parm [eax] [ecx] nomemory \
    value [eax] \
    modify exact [eax] nomemory;
#endif

#endif /* !IPRT_INCLUDED_asm_watcom_x86_32_h */

