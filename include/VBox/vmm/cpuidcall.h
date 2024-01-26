/** @file
 * VM - The Virtual Machine, CPU Host Call Interface (AMD64 & x86 only).
 *
 * @note cpuidcall.mac is generated from this file by running 'kmk incs' in the root.
 */

/*
 * Copyright (C) 2022-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_vmm_cpuidcall_h
#define VBOX_INCLUDED_vmm_cpuidcall_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>


/** @defgroup grp_cpuidcall     VBox CPUID Host Call Interface (AMD64 & x86)
 *
 * This describes an interface using CPUID for calling the host from within the
 * VM.  This is chiefly intended for nested VM debugging at present and is
 * therefore disabled by default.
 *
 * @{ */

/** Fixed EAX value for all requests (big-endian 'VBox'). */
#define VBOX_CPUID_REQ_EAX_FIXED            UINT32_C(0x56426f78)
/** Fixed portion of ECX for all requests. */
#define VBOX_CPUID_REQ_ECX_FIXED            UINT32_C(0xc0de0000)
/** Fixed portion of ECX for all requests. */
#define VBOX_CPUID_REQ_ECX_FIXED_MASK       UINT32_C(0xffff0000)
/** Function part of ECX for requests. */
#define VBOX_CPUID_REQ_ECX_FN_MASK          UINT32_C(0x0000ffff)

/** Generic ECX return value. */
#define VBOX_CPUID_RESP_GEN_ECX             UINT32_C(0x19410612)
/** Generic EDX return value. */
#define VBOX_CPUID_RESP_GEN_EDX             UINT32_C(0x19400412)
/** Generic EBX return value. */
#define VBOX_CPUID_RESP_GEN_EBX             UINT32_C(0x19450508)

/** @name Function \#1: Interface ID check and max function.
 *
 * Input: EDX & EBX content is unused and ignored. Best set to zero.
 *
 * Result: EAX:EDX:EBX forms the little endian string "VBox RuleZ!\0".
 *         ECX contains the max function number acccepted.
 * @{  */
#define VBOX_CPUID_FN_ID                    UINT32_C(0x0001)
#define VBOX_CPUID_RESP_ID_EAX              UINT32_C(0x786f4256)
#define VBOX_CPUID_RESP_ID_EDX              UINT32_C(0x6c755220)
#define VBOX_CPUID_RESP_ID_EBX              UINT32_C(0x00215A65)
#define VBOX_CPUID_RESP_ID_ECX              UINT32_C(0x00000002)
/** @} */

/** Function \#2: Write string to host Log.
 *
 * Input:   EDX gives the number of bytes to log (max 2MB).
 *          EBX indicates the log to write to: 0 for debug, 1 for release.
 *          RSI is the FLAT pointer to the UTF-8 string to log.
 *
 * Output:  EAX contains IPRT status code. ECX, EDX and EBX are set to the
 *          generic their response values (VBOX_CPUID_RESP_GEN_XXX). RSI is
 *          advanced EDX bytes on success.
 *
 * Except:  May raise \#PF when reading the string.  RSI and EDX is then be
 *          updated to the point where the page fault triggered, allowing paging
 *          in of logging buffer and such like.
 *
 * @note    Buffer is not accessed if the target logger isn't enabled.
 */
#define VBOX_CPUID_FN_LOG                   UINT32_C(0x0002)


/** @} */

#endif /* !VBOX_INCLUDED_vmm_cpuidcall_h */

