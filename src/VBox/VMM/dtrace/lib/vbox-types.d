/** @file
 * VBox & DTrace - Types and Constants.
 */

/*
 * Copyright (C) 2012-2023 Oracle and/or its affiliates.
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
 * SPDX-License-Identifier: GPL-3.0-only
 */


/*
 * Types used by the other D structure and type definitions.
 *
 * These are taken from a variation of VBox and IPRT headers.
 */
#pragma D depends_on library vbox-arch-types.d

typedef uint64_t                RTGCPHYS;
typedef uint64_t                RTHCPHYS;
typedef uint16_t                RTSEL;
typedef uint32_t                RTRCPTR;
typedef uintptr_t               RTNATIVETHREAD;
typedef struct RTTHREADINT     *RTTHREAD;
typedef struct RTTRACEBUFINT   *RTTRACEBUF;


typedef uint32_t                VMSTATE;
typedef uint32_t                VMCPUID;
typedef uint32_t                RTCPUID;
typedef struct UVMCPU          *PUVMCPU;
typedef uintptr_t               PVMR3;
typedef uint32_t                PVMRC;
typedef struct VM              *PVMR0;
typedef struct SUPDRVSESSION   *PSUPDRVSESSION;
typedef struct UVM             *PUVM;
typedef struct CPUMCTX         *PCPUMCTX;
typedef struct SVMVMCB         *PSVMVMCB;
typedef uint32_t                VMXVDIAG;
typedef struct VMXVVMCS        *PVMXVVMCS;

typedef struct VBOXGDTR
{
    uint16_t    cb;
    uint16_t    au16Addr[4];
} VBOXGDTR, VBOXIDTR;

typedef struct STAMPROFILEADV
{
    uint64_t            au64[5];
} STAMPROFILEADV;

